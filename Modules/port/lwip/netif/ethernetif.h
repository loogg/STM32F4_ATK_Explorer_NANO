#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__

#include <stdint.h>
#include <rtthread.h>
#include "lwip/netif.h"

#define ETHERNET_MTU    1500

#define ETH_VITRUAL_NETIF_MAX_NUM 6

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

enum {
    ETH_DEVICE_DHCP_OFF,
    ETH_DEVICE_DHCP_START,
    ETH_DEVICE_DHCP_WAIT_ADDRESS,
    ETH_DEVICE_DHCP_ADDRESS_ASSIGNED,
    ETH_DEVICE_DHCP_TIMEOUT,
    ETH_DEVICE_DHCP_LINK_DOWN,
};

struct eth_device_config {
    uint8_t dhcp_enable;
    uint8_t dhcp_timeout;
    uint8_t virtual_num;

    ip_addr_t ip[ETH_VITRUAL_NETIF_MAX_NUM + 1];
    ip_addr_t netmask[ETH_VITRUAL_NETIF_MAX_NUM + 1];
    ip_addr_t gw[ETH_VITRUAL_NETIF_MAX_NUM + 1];
};

struct eth_device {
    /* inherit from rt_device */
    struct rt_device parent;

    /* network interface for lwip */
    struct netif* netif;
    uint8_t link_changed;
    uint8_t link_status;
    uint8_t  rx_notice;
    uint32_t phy_an_ar;
    struct rt_spinlock spinlock;
    uint8_t dhcp_state;
    struct eth_device_config config;

    struct netif* virt_netif[ETH_VITRUAL_NETIF_MAX_NUM];

    uint8_t macaddr[NETIF_MAX_HWADDR_LEN];

    /* eth device interface */
    struct pbuf* (*eth_rx)(rt_device_t dev);
    err_t (*eth_tx)(rt_device_t dev, struct pbuf* p);
    rt_err_t (*eth_link_change)(rt_device_t dev, uint8_t link_status);
};

rt_err_t eth_device_init(const char *name, struct eth_device_config *config);

#endif /* __ETHERNETIF_H__ */
