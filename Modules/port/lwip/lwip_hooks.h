#ifndef LWIP_HOOKS_H
#define LWIP_HOOKS_H

err_enum_t lwip_hook_unknown_eth_protocol (struct pbuf * pbuf, struct netif * netif);
struct netif *lwip_arp_filter_netif(struct pbuf *p, struct netif *netif, u16_t type);
struct netif *lwip_hook_ip4_route_src(const ip4_addr_t *src, const ip4_addr_t *dest);

#endif /* LWIP_HOOKS_H */
