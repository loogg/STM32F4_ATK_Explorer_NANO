#include "netif/ethernet.h"
#include "lwip/etharp.h"
#include "lwip/ip.h"
#include "netif/ethernetif.h"
#include <string.h>

extern struct eth_device stm32_eth_device;

#if LWIP_ARP_FILTER_NETIF
struct netif *lwip_arp_filter_netif(struct pbuf *p, struct netif *netif, u16_t type) {
#if LWIP_ARP || ETHARP_SUPPORT_VLAN || LWIP_IPV6
    u16_t next_hdr_offset = SIZEOF_ETH_HDR;
#endif /* LWIP_ARP || ETHARP_SUPPORT_VLAN */

#if ETHARP_SUPPORT_VLAN
    struct eth_hdr *ethhdr = (struct eth_hdr *)p->payload;
    u16_t etype;
    etype = ethhdr->type;
    if (etype == PP_HTONS(ETHTYPE_VLAN)) {
        next_hdr_offset = SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR;
    }
#endif /* ETHARP_SUPPORT_VLAN */

    if (netif != stm32_eth_device.netif) {
        return netif;
    }

    switch (type) {
#if LWIP_IPV4 && LWIP_ARP
        case ETHTYPE_IP: {
            ip_addr_t dipaddr;
            const struct ip_hdr *iphdr = (struct ip_hdr *)((u8_t *)p->payload + next_hdr_offset);
            ip_addr_copy_from_ip4(dipaddr, iphdr->dest);

            if (ip4_addr_cmp(&dipaddr, netif_ip4_addr(netif)))
                break;

            for (int i = 0; i < stm32_eth_device.config.virtual_num; i++) {
                if (netif_is_up(stm32_eth_device.virt_netif[i])) {
                    if (ip4_addr_cmp(&dipaddr, netif_ip4_addr(stm32_eth_device.virt_netif[i]))) {
                        return stm32_eth_device.virt_netif[i];
                    }
                }
            }
        } break;

        case ETHTYPE_ARP: {
            ip4_addr_t dipaddr;
            struct etharp_hdr *hdr = (struct etharp_hdr *)((u8_t *)p->payload + next_hdr_offset);
            IPADDR_WORDALIGNED_COPY_TO_IP4_ADDR_T(&dipaddr, &hdr->dipaddr);

            if (ip4_addr_cmp(&dipaddr, netif_ip4_addr(netif)))
                break;

            for (int i = 0; i < stm32_eth_device.config.virtual_num; i++) {
                if (netif_is_up(stm32_eth_device.virt_netif[i])) {
                    if (ip4_addr_cmp(&dipaddr, netif_ip4_addr(stm32_eth_device.virt_netif[i]))) {
                        return stm32_eth_device.virt_netif[i];
                    }
                }
            }
        } break;
#endif /* LWIP_IPV4 && LWIP_ARP */

        default:
            break;
    }

    return netif;
}
#endif

struct netif *lwip_hook_ip4_route_src(const ip4_addr_t *src, const ip4_addr_t *dest) {
    if (ip4_addr_isany(src)) return NULL;

    struct netif *netif = stm32_eth_device.netif;
    if (netif_is_up(netif) && netif_is_link_up(netif) && !ip4_addr_isany_val(*netif_ip4_addr(netif))) {
        if (ip4_addr_cmp(src, netif_ip4_addr(netif)) && ip4_addr_netcmp(dest, netif_ip4_addr(netif), netif_ip4_netmask(netif))) {
            /* return netif on which to forward IP packet */
            return netif;
        }
    }

    for (int i = 0; i < stm32_eth_device.config.virtual_num; i++) {
        netif = stm32_eth_device.virt_netif[i];
        if (netif_is_up(netif) && netif_is_link_up(netif) && !ip4_addr_isany_val(*netif_ip4_addr(netif))) {
            if (ip4_addr_cmp(src, netif_ip4_addr(netif)) && ip4_addr_netcmp(dest, netif_ip4_addr(netif), netif_ip4_netmask(netif))) {
                /* return netif on which to forward IP packet */
                return netif;
            }
        }
    }

    return NULL;
}
