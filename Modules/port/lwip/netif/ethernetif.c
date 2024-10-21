/**
 * @file
 * Ethernet Interface Skeleton
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/*
 * This file is a skeleton for developing Ethernet network interface
 * drivers for lwIP. Add code to the low_level functions and do a
 * search-and-replace for the word "ethernetif" to replace it with
 * something that better describes your network interface.
 */

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/ethip6.h"
#include "lwip/etharp.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/netifapi.h"
#include "netif/ethernetif.h"
#include <string.h>
#include <ipc/completion.h>

#define DBG_TAG "ethif"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#if LWIP_NETIF_HOSTNAME
#define LWIP_HOSTNAME_LEN 16
#endif /*LWIP_NETIF_HOSTNAME */

#define RT_ETHERNETIF_THREAD_PREORITY   6
// #define LWIP_NO_TX_THREAD
#define LWIP_ETHTHREAD_MBOX_SIZE 32
#define LWIP_ETHTHREAD_STACKSIZE 4096

#ifndef LWIP_NO_TX_THREAD
/**
 * Tx message structure for Ethernet interface
 */
struct eth_tx_msg
{
    struct netif    *netif;
    struct pbuf     *buf;
    struct rt_completion ack;
};

static struct rt_mailbox eth_tx_thread_mb;
static struct rt_thread eth_tx_thread;
static char eth_tx_thread_mb_pool[LWIP_ETHTHREAD_MBOX_SIZE * sizeof(rt_ubase_t)];
static char eth_tx_thread_stack[LWIP_ETHTHREAD_STACKSIZE];
#endif /* LWIP_NO_TX_THREAD */

static struct rt_mailbox eth_rx_thread_mb;
static struct rt_thread eth_rx_thread;
static char eth_rx_thread_mb_pool[LWIP_ETHTHREAD_MBOX_SIZE * sizeof(rt_ubase_t)];
static char eth_rx_thread_stack[LWIP_ETHTHREAD_STACKSIZE];

static err_t ethernetif_linkoutput(struct netif *netif, struct pbuf *p)
{
#ifndef LWIP_NO_TX_THREAD
    struct eth_tx_msg msg;

    RT_ASSERT(netif != RT_NULL);

    /* send a message to eth tx thread */
    msg.netif = netif;
    msg.buf   = p;
    rt_completion_init(&msg.ack);
    if (rt_mb_send(&eth_tx_thread_mb, (rt_ubase_t) &msg) == RT_EOK)
    {
        /* waiting for ack */
        rt_completion_wait(&msg.ack, RT_WAITING_FOREVER);
    }
#else
    struct eth_device* enetif;

    RT_ASSERT(netif != RT_NULL);
    enetif = (struct eth_device*)netif->state;

    if (enetif->eth_tx(&(enetif->parent), p) != RT_EOK)
    {
        return ERR_IF;
    }
#endif
    return ERR_OK;
}

static void ethernetif_link_changed(struct netif *netif) {
    struct eth_device *ethif = (struct eth_device *)netif->state;

    if (ethif->config.dhcp_enable) {
        if (netif_is_link_up(netif)) {
            ethif->dhcp_state = ETH_DEVICE_DHCP_START;
        } else {
            ethif->dhcp_state = ETH_DEVICE_DHCP_LINK_DOWN;
        }
    }

    for (int i = 0; i < ethif->config.virtual_num; i++) {
        if (ethif->virt_netif[i] != RT_NULL && netif_is_up(ethif->virt_netif[i])) {
            if (netif_is_link_up(netif)) {
                netif_set_link_up(ethif->virt_netif[i]);
            } else {
                netif_set_link_down(ethif->virt_netif[i]);
            }
        }
    }
}

static void ethernetif_dhcp_timeout(void *arg) {
    static rt_tick_t dhcp_timeout = 0;
    struct netif *netif = (struct netif *)arg;
    struct eth_device *ethif = (struct eth_device *)netif->state;
    uint8_t iptxt[20];

    switch (ethif->dhcp_state) {
        case ETH_DEVICE_DHCP_START: {
            ip_addr_set_zero(&netif->ip_addr);
            ip_addr_set_zero(&netif->netmask);
            ip_addr_set_zero(&netif->gw);
            ethif->dhcp_state = ETH_DEVICE_DHCP_WAIT_ADDRESS;
            dhcp_start(netif);
            dhcp_timeout = rt_tick_get() + rt_tick_from_millisecond(ethif->config.dhcp_timeout * 1000);

            LOG_I("  State: Looking for DHCP server ...");
        } break;

        case ETH_DEVICE_DHCP_WAIT_ADDRESS: {
            if (dhcp_supplied_address(netif)) {
                ethif->dhcp_state = ETH_DEVICE_DHCP_ADDRESS_ASSIGNED;

                snprintf((char *)iptxt, sizeof(iptxt), "%s", ip4addr_ntoa((const ip4_addr_t *)&netif->ip_addr));
                LOG_I("IP address assigned by a DHCP server: %s", iptxt);
            } else {


                /* DHCP timeout */
                if (rt_tick_get() - dhcp_timeout < (RT_TICK_MAX / 2)) {
                    ethif->dhcp_state = ETH_DEVICE_DHCP_TIMEOUT;

                    /* Stop DHCP */
                    dhcp_stop(netif);

                    /* Static address used */
                    netif_set_addr(netif, &ethif->config.ip[0], &ethif->config.netmask[0], &ethif->config.gw[0]);

                    snprintf((char *)iptxt, sizeof(iptxt), "%s", ip4addr_ntoa((const ip4_addr_t *)&netif->ip_addr));
                    LOG_I("DHCP Timeout !!");
                    LOG_I("Static IP address: %s", iptxt);
                }
            }
        } break;
        case ETH_DEVICE_DHCP_LINK_DOWN: {
            /* Stop DHCP */
            dhcp_stop(netif);
            ethif->dhcp_state = ETH_DEVICE_DHCP_OFF;
        } break;
        default:
            break;
    }

    sys_timeout(DHCP_FINE_TIMER_MSECS, ethernetif_dhcp_timeout, netif);
}

static err_t ethernetif_private_init(struct netif *netif, uint8_t is_virtual) {
    struct eth_device *ethif;

    LWIP_ASSERT("netif != NULL", (netif != NULL));

    ethif = (struct eth_device*)netif->state;
    if (ethif == NULL) return ERR_IF;

    /*
     * Initialize the snmp variables and counters inside the struct netif.
     * The last argument should be replaced with your link speed, in units
     * of bits per second.
     */
    MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, ETH_DEVICE_PHY_100M | ETH_DEVICE_PHY_FULL_DUPLEX);

    strncpy(netif->name, ethif->parent.parent.name, 2);

#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    char *hostname = (char *)netif + sizeof(struct netif);
    rt_sprintf(hostname, "rtt_%02x%02x_%02x", netif->name[0], netif->name[1], netif->num);
    netif->hostname = hostname;
#endif /* LWIP_NETIF_HOSTNAME */
    /* We directly use etharp_output() here to save a function call.
     * You can instead declare your own function an call etharp_output()
     * from it if you have to do some checks before sending (e.g. if link
     * is available...) */
#if LWIP_IPV4
    netif->output = etharp_output;
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
    netif->linkoutput = ethernetif_linkoutput;

    NETIF_SET_CHECKSUM_CTRL(netif, NETIF_CHECKSUM_DISABLE_ALL);

    /* set MAC hardware address length */
    netif->hwaddr_len = ETHARP_HWADDR_LEN;

    /* set MAC hardware address */
    netif->hwaddr[0] = ethif->macaddr[0];
    netif->hwaddr[1] = ethif->macaddr[1];
    netif->hwaddr[2] = ethif->macaddr[2];
    netif->hwaddr[3] = ethif->macaddr[3];
    netif->hwaddr[4] = ethif->macaddr[4];
    netif->hwaddr[5] = ethif->macaddr[5];

    /* maximum transfer unit */
    netif->mtu = ETHERNET_MTU;

    /* device capabilities */
    /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
    netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

#if LWIP_IPV6 && LWIP_IPV6_MLD
    /*
     * For hardware/netifs that implement MAC filtering.
     * All-nodes link-local is handled by default, so we must let the hardware know
     * to allow multicast packets in.
     * Should set mld_mac_filter previously. */
    if (netif->mld_mac_filter != NULL) {
        ip6_addr_t ip6_allnodes_ll;
        ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
        netif->mld_mac_filter(netif, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
    }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */

    if (!is_virtual) {
        if (ethif->config.dhcp_enable) {
            ethif->dhcp_state = ETH_DEVICE_DHCP_OFF;
            sys_timeout(DHCP_FINE_TIMER_MSECS, ethernetif_dhcp_timeout, netif);
        }

        /* initialize the hardware */
        if (rt_device_open(&(ethif->parent), RT_DEVICE_OFLAG_RDWR) != RT_EOK) return ERR_IF;

        netif_set_link_callback(netif, ethernetif_link_changed);

        if (netif_default == RT_NULL) netif_set_default(netif);
    }

    netif_set_up(netif);

    if (is_virtual) {
        if (netif_is_link_up(ethif->netif)) {
            netif_set_link_up(netif);
        }
    }

    return ERR_OK;
}

static err_t ethernetif_init(struct netif *netif) {
    return ethernetif_private_init(netif, 0);
}

static err_t ethernetif_virtual_init(struct netif *netif) {
    return ethernetif_private_init(netif, 1);
}


static rt_err_t eth_device_ready(struct eth_device *dev) {
    if (dev->netif) {
        if (dev->rx_notice == RT_FALSE) {
            dev->rx_notice = RT_TRUE;
            return rt_mb_send(&eth_rx_thread_mb, (rt_ubase_t)dev);
        } else
            return RT_EOK;
        /* post message to Ethernet thread */
    } else
        return -RT_ERROR; /* netif is not initialized yet, just return. */
}

static rt_err_t eth_device_linkchange(struct eth_device *dev, uint8_t link_status) {
    rt_base_t level;

    RT_ASSERT(dev != RT_NULL);

    level = rt_spin_lock_irqsave(&(dev->spinlock));
    dev->link_changed = 0x01;
    dev->link_status = link_status;
    rt_spin_unlock_irqrestore(&(dev->spinlock), level);

    /* post message to ethernet thread */
    return rt_mb_send(&eth_rx_thread_mb, (rt_ubase_t)dev);
}

static rt_err_t eth_rx_notify(rt_device_t dev, rt_size_t size) {
    struct eth_device *device = (struct eth_device *)dev;
    uint8_t notify_type = size & ETH_DEVICE_RX_NOTIFY_TYPE_MASK;
    rt_err_t ret = -RT_ERROR;

    switch (notify_type) {
        case ETH_DEVICE_RX_NOTIFY_TYPE_INPUT:
            ret = eth_device_ready(device);
            break;

        case ETH_DEVICE_RX_NOTIFY_TYPE_LINK:
            ret = eth_device_linkchange(device, (uint8_t)(size >> 8));
            break;

        default:
            break;
    }

    return ret;
}

rt_err_t eth_device_init(const char *name, struct eth_device_config *config) {
    struct eth_device *dev = (struct eth_device *)rt_device_find(name);
    if (dev == RT_NULL) {
        LOG_E("eth device %s not found", name);
        return -RT_ERROR;
    }

    dev->config = *config;
    if (dev->config.virtual_num > ETH_VITRUAL_NETIF_MAX_NUM) {
        LOG_W("virtual netif number is too large");
        dev->config.virtual_num = ETH_VITRUAL_NETIF_MAX_NUM;
    }

    rt_device_set_rx_indicate((rt_device_t)dev, eth_rx_notify);

    struct netif* netif;
#if LWIP_NETIF_HOSTNAME
    netif = (struct netif*) rt_calloc (1, sizeof(struct netif) + LWIP_HOSTNAME_LEN);
#else
    netif = (struct netif*) rt_calloc (1, sizeof(struct netif));
#endif
    if (netif == RT_NULL)
    {
        LOG_E("malloc netif failed");
        return -RT_ERROR;
    }

    dev->netif = netif;
    dev->link_changed = 0x00;
    dev->link_status = 0x00;
    dev->phy_an_ar = 0x00;
    /* avoid send the same mail to mailbox */
    dev->rx_notice = 0x00;
    rt_spin_lock_init(&(dev->spinlock));

    if (dev->config.dhcp_enable) {
        netifapi_netif_add(netif, NULL, NULL, NULL, dev, ethernetif_init, tcpip_input);
    } else {
        netifapi_netif_add(netif, &dev->config.ip[0], &dev->config.netmask[0], &dev->config.gw[0], dev, ethernetif_init, tcpip_input);
    }

    for (int i = 0; i < dev->config.virtual_num; i++) {
#if LWIP_NETIF_HOSTNAME
        netif = (struct netif*) rt_calloc (1, sizeof(struct netif) + LWIP_HOSTNAME_LEN);
#else
        netif = (struct netif*) rt_calloc (1, sizeof(struct netif));
#endif
        if (netif == RT_NULL) {
            LOG_E("malloc netif failed");
            return -RT_ERROR;
        }

        netifapi_netif_add(netif, &dev->config.ip[i + 1], &dev->config.netmask[i + 1], &dev->config.gw[i + 1], dev, ethernetif_virtual_init, tcpip_input);
        dev->virt_netif[i] = netif;
    }

    return RT_EOK;
}

#ifndef LWIP_NO_TX_THREAD
/* Ethernet Tx Thread */
static void eth_tx_thread_entry(void* parameter)
{
    struct eth_tx_msg* msg;

    while (1)
    {
        if (rt_mb_recv(&eth_tx_thread_mb, (rt_ubase_t *)&msg, RT_WAITING_FOREVER) == RT_EOK)
        {
            struct eth_device* enetif;

            RT_ASSERT(msg->netif != RT_NULL);
            RT_ASSERT(msg->buf   != RT_NULL);

            enetif = (struct eth_device*)msg->netif->state;
            if (enetif != RT_NULL)
            {
                /* call driver's interface */
                if (enetif->eth_tx(&(enetif->parent), msg->buf) != RT_EOK)
                {
                    /* transmit eth packet failed */
                }
            }

            /* send ACK */
            rt_completion_done(&msg->ack);
        }
    }
}
#endif /* LWIP_NO_TX_THREAD */

/* Ethernet Rx Thread */
static void eth_rx_thread_entry(void* parameter)
{
    struct eth_device* device;

    while (1)
    {
        if (rt_mb_recv(&eth_rx_thread_mb, (rt_ubase_t *)&device, RT_WAITING_FOREVER) == RT_EOK)
        {
            rt_base_t level;
            struct pbuf *p;

            /* check link status */
            if (device->link_changed)
            {
                int status;

                level = rt_spin_lock_irqsave(&(device->spinlock));
                status = device->link_status;
                device->link_changed = 0x00;
                rt_spin_unlock_irqrestore(&(device->spinlock), level);

                if (device->eth_link_change) device->eth_link_change(&(device->parent), status);

                if (status & ETH_DEVICE_PHY_LINK)
                    netifapi_netif_set_link_up(device->netif);
                else
                    netifapi_netif_set_link_down(device->netif);
            }

            level = rt_spin_lock_irqsave(&(device->spinlock));
            /* 'rx_notice' will be modify in the interrupt or here */
            device->rx_notice = RT_FALSE;
            rt_spin_unlock_irqrestore(&(device->spinlock), level);

            /* receive all of buffer */
            while (1)
            {
                if(device->eth_rx == RT_NULL) break;

                p = device->eth_rx(&(device->parent));
                if (p != RT_NULL)
                {
                    /* notify to upper layer */
                    if(device->netif->input(p, device->netif) != ERR_OK )
                    {
                        LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: Input error\n"));
                        pbuf_free(p);
                        p = NULL;
                    }
                }
                else break;
            }
        }
        else
        {
            LWIP_ASSERT("Should not happen!\n",0);
        }
    }
}

int eth_system_device_init_private(void)
{
    rt_err_t result = RT_EOK;

    /* initialize Rx thread. */
    /* initialize mailbox and create Ethernet Rx thread */
    result = rt_mb_init(&eth_rx_thread_mb, "erxmb",
                        &eth_rx_thread_mb_pool[0], sizeof(eth_rx_thread_mb_pool)/sizeof(rt_ubase_t),
                        RT_IPC_FLAG_FIFO);
    RT_ASSERT(result == RT_EOK);

    result = rt_thread_init(&eth_rx_thread, "erx", eth_rx_thread_entry, RT_NULL,
                            &eth_rx_thread_stack[0], sizeof(eth_rx_thread_stack),
                            RT_ETHERNETIF_THREAD_PREORITY, 16);
    RT_ASSERT(result == RT_EOK);
    result = rt_thread_startup(&eth_rx_thread);
    RT_ASSERT(result == RT_EOK);

    /* initialize Tx thread */
#ifndef LWIP_NO_TX_THREAD
    /* initialize mailbox and create Ethernet Tx thread */
    result = rt_mb_init(&eth_tx_thread_mb, "etxmb",
                        &eth_tx_thread_mb_pool[0], sizeof(eth_tx_thread_mb_pool)/sizeof(rt_ubase_t),
                        RT_IPC_FLAG_FIFO);
    RT_ASSERT(result == RT_EOK);

    result = rt_thread_init(&eth_tx_thread, "etx", eth_tx_thread_entry, RT_NULL,
                            &eth_tx_thread_stack[0], sizeof(eth_tx_thread_stack),
                            RT_ETHERNETIF_THREAD_PREORITY, 16);
    RT_ASSERT(result == RT_EOK);

    result = rt_thread_startup(&eth_tx_thread);
    RT_ASSERT(result == RT_EOK);
#endif /* LWIP_NO_TX_THREAD */

    return (int)result;
}
