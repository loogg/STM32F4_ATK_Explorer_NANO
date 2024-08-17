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

struct eth_device {
    /* inherit from rt_device */
    struct rt_device parent;

    /* network interface for lwip */
    struct netif* netif;
    uint8_t link_changed;
    uint8_t link_status;
    uint8_t  rx_notice;
    struct rt_spinlock spinlock;

    uint8_t macaddr[NETIF_MAX_HWADDR_LEN];

    /* eth device interface */
    struct pbuf* (*eth_rx)(rt_device_t dev);
    err_t (*eth_tx)(rt_device_t dev, struct pbuf* p);
    rt_err_t (*eth_link_change)(rt_device_t dev, uint8_t link_status);
};

rt_err_t eth_device_init(const char *name, uint8_t *default_ip, uint8_t *default_netmask, uint8_t *default_gw);

#endif /* __ETHERNETIF_H__ */
