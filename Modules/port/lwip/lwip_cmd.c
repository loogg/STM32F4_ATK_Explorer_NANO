#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/netif.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/ip.h"
#include "lwip/prot/ip.h"
#include "lwip/prot/ip4.h"
#include "lwip/timeouts.h"
#include "lwip/sys.h"
#include "lwip/dns.h"

#ifdef SYSTEM_USING_CONSOLE_SHELL

static void cmd_list_if(void) {
    uint32_t index;
    struct netif *netif;

    netif = netif_list;

    while (netif != NULL) {
        SYSTEM_PRINTF("network interface: %c%c%s\r\n", netif->name[0], netif->name[1], (netif == netif_default) ? " (Default)" : "");
        SYSTEM_PRINTF("MTU: %d\r\n", netif->mtu);
        SYSTEM_PRINTF("MAC: ");
        for (index = 0; index < netif->hwaddr_len; index++) SYSTEM_PRINTF("%02x ", netif->hwaddr[index]);
        SYSTEM_PRINTF("\r\nFLAGS:");
        if (netif->flags & NETIF_FLAG_UP)
            SYSTEM_PRINTF(" UP");
        else
            SYSTEM_PRINTF(" DOWN");
        if (netif->flags & NETIF_FLAG_LINK_UP)
            SYSTEM_PRINTF(" LINK_UP");
        else
            SYSTEM_PRINTF(" LINK_DOWN");
        if (netif->flags & NETIF_FLAG_ETHARP) SYSTEM_PRINTF(" ETHARP");
        if (netif->flags & NETIF_FLAG_BROADCAST) SYSTEM_PRINTF(" BROADCAST");
        if (netif->flags & NETIF_FLAG_IGMP) SYSTEM_PRINTF(" IGMP");
        SYSTEM_PRINTF("\r\n");
        SYSTEM_PRINTF("ip address: %s\r\n", ipaddr_ntoa(&(netif->ip_addr)));
        SYSTEM_PRINTF("gw address: %s\r\n", ipaddr_ntoa(&(netif->gw)));
        SYSTEM_PRINTF("net mask  : %s\r\n", ipaddr_ntoa(&(netif->netmask)));
#if LWIP_IPV6
        {
            ip6_addr_t *addr;
            int addr_state;
            int i;

            addr = (ip6_addr_t *)&netif->ip6_addr[0];
            addr_state = netif->ip6_addr_state[0];

            SYSTEM_PRINTF("\r\nipv6 link-local: %s state:%02X %s\r\n", ip6addr_ntoa(addr), addr_state,
                          ip6_addr_isvalid(addr_state) ? "VALID" : "INVALID");

            for (i = 1; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
                addr = (ip6_addr_t *)&netif->ip6_addr[i];
                addr_state = netif->ip6_addr_state[i];

                SYSTEM_PRINTF("ipv6[%d] address: %s state:%02X %s\r\n", i, ip6addr_ntoa(addr), addr_state,
                              ip6_addr_isvalid(addr_state) ? "VALID" : "INVALID");
            }
        }
        SYSTEM_PRINTF("\r\n");
#endif /* LWIP_IPV6 */
        netif = netif->next;
    }

#if LWIP_DNS
    {
        const ip_addr_t *ip_addr;

        for (index = 0; index < DNS_MAX_SERVERS; index++) {
            ip_addr = dns_getserver(index);
            SYSTEM_PRINTF("dns server #%d: %s\r\n", index, ipaddr_ntoa(ip_addr));
        }
    }
#endif /**< #if LWIP_DNS */
}
SHELL_EXPORT_CMD(list_if, cmd_list_if, list network interface);

#define PING_ID        0xAFAF
#define PING_DATA_SIZE 32
#define PING_TIMES     4
#define PING_RCV_TIMEO 1000

static u16_t ping_seq_num = 0;
static u32_t ping_time = 0;
static u8_t ping_flag = 0;
static u16_t ping_index = 0;
static ip_addr_t ping_target;
static struct raw_pcb *ping_pcb = NULL;

static u8_t ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr) {
    struct icmp_echo_hdr *iecho;
    struct ip_hdr *iphdr;

    LWIP_UNUSED_ARG(arg);
    LWIP_UNUSED_ARG(pcb);
    LWIP_UNUSED_ARG(addr);
    LWIP_ASSERT("p != NULL", p != NULL);

    if (p->tot_len >= PBUF_IP_HLEN) {
        iphdr = (struct ip_hdr *)p->payload;

        if ((p->tot_len >= (PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr))) && pbuf_remove_header(p, PBUF_IP_HLEN) == 0) {
            iecho = (struct icmp_echo_hdr *)p->payload;

            if ((iecho->id == PING_ID) && (iecho->seqno == lwip_htons(ping_seq_num))) {
                if (IPH_TTL(iphdr) == 0) {
                    SYSTEM_PRINTF("%d bytes from %s icmp_seq=%d time=%d ms\r\n", p->tot_len - sizeof(struct icmp_echo_hdr), ipaddr_ntoa(addr),
                                  ping_index, sys_now() - ping_time);
                } else {
                    SYSTEM_PRINTF("%d bytes from %s icmp_seq=%d ttl=%d time=%d ms\r\n", p->tot_len - sizeof(struct icmp_echo_hdr), ipaddr_ntoa(addr),
                                  ping_index, IPH_TTL(iphdr), sys_now() - ping_time);
                }

                ping_flag = 1;
                /* do some ping result processing */
                pbuf_free(p);
                return 1; /* eat the packet */
            }
            /* not eaten, restore original packet */
            pbuf_add_header(p, PBUF_IP_HLEN);
        }
    }

    return 0; /* don't eat the packet */
}

/** Prepare a echo ICMP request */
static void ping_prepare_echo(struct icmp_echo_hdr *iecho, u16_t len) {
    size_t i;
    size_t data_len = len - sizeof(struct icmp_echo_hdr);

    ICMPH_TYPE_SET(iecho, ICMP_ECHO);
    ICMPH_CODE_SET(iecho, 0);
    iecho->chksum = 0;
    iecho->id = PING_ID;
    iecho->seqno = lwip_htons(++ping_seq_num);

    /* fill the additional data buffer with some data */
    for (i = 0; i < data_len; i++) {
        ((char *)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
    }

#ifdef CHECKSUM_BY_HARDWARE
    iecho->chksum = 0;
#else
    iecho->chksum = inet_chksum(iecho, len);
#endif
}

static void ping_send(struct raw_pcb *raw, const ip_addr_t *addr) {
    struct pbuf *p;
    struct icmp_echo_hdr *iecho;
    size_t ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;
    LWIP_ASSERT("ping_size is too big", ping_size <= 0xffff);

    p = pbuf_alloc(PBUF_IP, (u16_t)ping_size, PBUF_RAM);
    if (!p) {
        return;
    }
    if ((p->len == p->tot_len) && (p->next == NULL)) {
        iecho = (struct icmp_echo_hdr *)p->payload;

        ping_prepare_echo(iecho, (u16_t)ping_size);

        ping_flag = 0;
        raw_sendto(raw, p, addr);

        ping_time = sys_now();
    }
    pbuf_free(p);
}

static void ping_timeout(void *arg) {
    if (ping_flag == 0) {
        SYSTEM_PRINTF("ping: unknown address %s\r\n", ipaddr_ntoa(&ping_target));
    }

    ping_index++;
    if (ping_index < PING_TIMES) {
        ping_send(ping_pcb, &ping_target);
        sys_timeout(PING_RCV_TIMEO, ping_timeout, NULL);
    } else {
        raw_remove(ping_pcb);
        ping_pcb = NULL;
    }
}

static void cmd_ping(int argc, char *agrv[]) {
    if (argc != 2) {
        SYSTEM_PRINTF("Usage: ping <host>\r\n");
        return;
    }

    if (ping_pcb == NULL) {
        ping_pcb = raw_new(IP_PROTO_ICMP);
        if (ping_pcb == NULL) {
            SYSTEM_PRINTF("create raw pcb failed\r\n");
            return;
        }
    } else {
        SYSTEM_PRINTF("ping is running\r\n");
        return;
    }

    if (ipaddr_aton(agrv[1], &ping_target) == 0) {
        SYSTEM_PRINTF("bad ip address\r\n");
        raw_remove(ping_pcb);
        ping_pcb = NULL;
        return;
    }

    ping_index = 0;
    raw_recv(ping_pcb, ping_recv, NULL);
    raw_bind(ping_pcb, IP_ADDR_ANY);

    ping_send(ping_pcb, &ping_target);
    sys_timeout(PING_RCV_TIMEO, ping_timeout, NULL);
}
SHELL_EXPORT_CMD(ping, cmd_ping, ping a host);

#endif /* SYSTEM_USING_CONSOLE_SHELL */
