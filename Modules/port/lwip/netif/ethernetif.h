#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__

#include <stdint.h>
#include <rtthread.h>
#include "lwip/netif.h"

#define ETHERNET_MTU    1500

#define ETH_DEVICE_RX_NOTIFY_TYPE_MASK 0x0f
enum {
    ETH_DEVICE_RX_NOTIFY_TYPE_INPUT = 0,
    ETH_DEVICE_RX_NOTIFY_TYPE_LINK = 1,
};

enum {
    ETH_DEVICE_PHY_LINK = (1 << 0),
    ETH_DEVICE_PHY_100M = (1 << 1),
    ETH_DEVICE_PHY_FULL_DUPLEX = (1 << 2),
};

#if LWIP_DHCP
enum {
    ETH_DEVICE_DHCP_OFF,
    ETH_DEVICE_DHCP_START,
    ETH_DEVICE_DHCP_WAIT_ADDRESS,
    ETH_DEVICE_DHCP_ADDRESS_ASSIGNED,
    ETH_DEVICE_DHCP_TIMEOUT,
    ETH_DEVICE_DHCP_LINK_DOWN,
};
#define ETH_DEVICE_DHCP_MAX_TRIES 3
#endif /* LWIP_DHCP */

struct eth_device {
    /* inherit from rt_device */
    struct rt_device parent;

    /* network interface for lwip */
    struct netif* netif;
    uint8_t link_changed;
    uint8_t link_status;
    uint8_t  rx_notice;
    struct rt_spinlock spinlock;
#if LWIP_DHCP
    uint8_t dhcp_state;
    ip_addr_t default_ip;
    ip_addr_t default_netmask;
    ip_addr_t default_gw;
#endif /* LWIP_DHCP */

    uint8_t macaddr[NETIF_MAX_HWADDR_LEN];

    /* eth device interface */
    struct pbuf* (*eth_rx)(rt_device_t dev);
    err_t (*eth_tx)(rt_device_t dev, struct pbuf* p);
    rt_err_t (*eth_link_change)(rt_device_t dev, uint8_t link_status);
};

rt_err_t eth_device_init(const char *name, uint8_t *default_ip, uint8_t *default_netmask, uint8_t *default_gw);

#endif /* __ETHERNETIF_H__ */
