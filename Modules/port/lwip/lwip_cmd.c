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
#include "lwip/etharp.h"
#include "lwip/tcpip.h"
#include "lwip/ip_addr.h"
#include "lwip/tcpip.h"
#include "lwip/api.h"
#include <string.h>

#ifdef RT_USING_FINSH

static void cmd_list_arp(void) {
    ip4_addr_t *ip;
    struct netif *netif;
    struct eth_addr *ethaddr;

    rt_kprintf("%-20s %-25s netif\r\n", "IP", "MAC");

    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (etharp_get_entry(i, &ip, &netif, &ethaddr)) {
            rt_kprintf("%-20s %02x-%02x-%02x-%02x-%02x-%02x         %c%c-%d\r\n", ipaddr_ntoa(ip), ethaddr->addr[0], ethaddr->addr[1],
                       ethaddr->addr[2], ethaddr->addr[3], ethaddr->addr[4], ethaddr->addr[5], netif->name[0], netif->name[1], netif->num);
        }
    }
}
MSH_CMD_EXPORT_ALIAS(cmd_list_arp, list_arp, list ARP cache);

#if LWIP_TCP
#include <lwip/tcp.h>
#include <lwip/priv/tcp_priv.h>
static void list_tcps(void) {
    rt_uint32_t num = 0;
    struct tcp_pcb *pcb;
    char local_ip_str[16];
    char remote_ip_str[16];

    extern struct tcp_pcb *tcp_active_pcbs;
    extern union tcp_listen_pcbs_t tcp_listen_pcbs;
    extern struct tcp_pcb *tcp_tw_pcbs;

    rt_enter_critical();
    rt_kprintf("Active PCB states:\n");
    for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
        strcpy(local_ip_str, ipaddr_ntoa(&(pcb->local_ip)));
        strcpy(remote_ip_str, ipaddr_ntoa(&(pcb->remote_ip)));

        rt_kprintf("#%d %s:%d <==> %s:%d snd_nxt 0x%08X rcv_nxt 0x%08X ", num++, local_ip_str, pcb->local_port, remote_ip_str, pcb->remote_port,
                   pcb->snd_nxt, pcb->rcv_nxt);
        rt_kprintf("state: %s\n", tcp_debug_state_str(pcb->state));
    }

    rt_kprintf("Listen PCB states:\n");
    num = 0;
    for (pcb = (struct tcp_pcb *)tcp_listen_pcbs.pcbs; pcb != NULL; pcb = pcb->next) {
        rt_kprintf("#%d local port %d ", num++, pcb->local_port);
        rt_kprintf("state: %s\n", tcp_debug_state_str(pcb->state));
    }

    rt_kprintf("TIME-WAIT PCB states:\n");
    num = 0;
    for (pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
        strcpy(local_ip_str, ipaddr_ntoa(&(pcb->local_ip)));
        strcpy(remote_ip_str, ipaddr_ntoa(&(pcb->remote_ip)));

        rt_kprintf("#%d %s:%d <==> %s:%d snd_nxt 0x%08X rcv_nxt 0x%08X ", num++, local_ip_str, pcb->local_port, remote_ip_str, pcb->remote_port,
                   pcb->snd_nxt, pcb->rcv_nxt);
        rt_kprintf("state: %s\n", tcp_debug_state_str(pcb->state));
    }
    rt_exit_critical();
}
MSH_CMD_EXPORT(list_tcps, list all of tcp connections);
#endif /* LWIP_TCP */

#if LWIP_UDP
#include "lwip/udp.h"
static void list_udps(void) {
    struct udp_pcb *pcb;
    rt_uint32_t num = 0;
    char local_ip_str[16];
    char remote_ip_str[16];

    rt_enter_critical();
    rt_kprintf("Active UDP PCB states:\n");
    for (pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
        strcpy(local_ip_str, ipaddr_ntoa(&(pcb->local_ip)));
        strcpy(remote_ip_str, ipaddr_ntoa(&(pcb->remote_ip)));

        rt_kprintf("#%d %d %s:%d <==> %s:%d \n", num, (int)pcb->flags, local_ip_str, pcb->local_port, remote_ip_str, pcb->remote_port);

        num++;
    }
    rt_exit_critical();
}
MSH_CMD_EXPORT(list_udps, list all of udp connections);
#endif /* LWIP_UDP */

static void cmd_netstat(void) {
#ifdef LWIP_TCP
    list_tcps();
#endif
#ifdef LWIP_UDP
    list_udps();
#endif
}
MSH_CMD_EXPORT_ALIAS(cmd_netstat, netstat, show network statistics);

static void cmd_list_if(int argc, char *agrv[]) {
    uint32_t index;
    struct netif *netif;

    rt_enter_critical();

    netif = netif_list;

    while (netif != NULL) {
        rt_kprintf("network interface: %c%c-%d%s\r\n", netif->name[0], netif->name[1], netif->num, (netif == netif_default) ? " (Default)" : "");
        rt_kprintf("MTU: %d\r\n", netif->mtu);
        rt_kprintf("MAC: ");
        for (index = 0; index < netif->hwaddr_len; index++) rt_kprintf("%02x ", netif->hwaddr[index]);
        rt_kprintf("\r\nFLAGS:");
        if (netif->flags & NETIF_FLAG_UP)
            rt_kprintf(" UP");
        else
            rt_kprintf(" DOWN");
        if (netif->flags & NETIF_FLAG_LINK_UP)
            rt_kprintf(" LINK_UP");
        else
            rt_kprintf(" LINK_DOWN");
        if (netif->flags & NETIF_FLAG_ETHARP) rt_kprintf(" ETHARP");
        if (netif->flags & NETIF_FLAG_BROADCAST) rt_kprintf(" BROADCAST");
        if (netif->flags & NETIF_FLAG_IGMP) rt_kprintf(" IGMP");
        rt_kprintf("\r\n");
        rt_kprintf("ip address: %s\r\n", ipaddr_ntoa(&(netif->ip_addr)));
        rt_kprintf("gw address: %s\r\n", ipaddr_ntoa(&(netif->gw)));
        rt_kprintf("net mask  : %s\r\n", ipaddr_ntoa(&(netif->netmask)));
#if LWIP_IPV6
        {
            ip6_addr_t *addr;
            int addr_state;
            int i;

            addr = (ip6_addr_t *)&netif->ip6_addr[0];
            addr_state = netif->ip6_addr_state[0];

            rt_kprintf("\r\nipv6 link-local: %s state:%02X %s\r\n", ip6addr_ntoa(addr), addr_state,
                       ip6_addr_isvalid(addr_state) ? "VALID" : "INVALID");

            for (i = 1; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
                addr = (ip6_addr_t *)&netif->ip6_addr[i];
                addr_state = netif->ip6_addr_state[i];

                rt_kprintf("ipv6[%d] address: %s state:%02X %s\r\n", i, ip6addr_ntoa(addr), addr_state,
                           ip6_addr_isvalid(addr_state) ? "VALID" : "INVALID");
            }
        }
        rt_kprintf("\r\n");
#endif /* LWIP_IPV6 */
        netif = netif->next;
    }

#if LWIP_DNS
    {
        const ip_addr_t *ip_addr;

        for (index = 0; index < DNS_MAX_SERVERS; index++) {
            ip_addr = dns_getserver(index);
            rt_kprintf("dns server #%d: %s\r\n", index, ipaddr_ntoa(ip_addr));
        }
    }
#endif /**< #if LWIP_DNS */

    rt_exit_critical();
}

static void cmd_netif_link(int argc, char *agrv[]) {
    if (argc < 2) {
        rt_kprintf("Usage: netif link <up|down> <if>\r\n");
        return;
    }

    LOCK_TCPIP_CORE();
    do {
        struct netif *netif = netif_find(agrv[1]);
        if (netif == NULL) {
            rt_kprintf("no such interface: %s\r\n", agrv[1]);
            break;
        }

        if (strcmp(agrv[0], "up") == 0) {
            netif_set_link_up(netif);
        } else if (strcmp(agrv[0], "down") == 0) {
            netif_set_link_down(netif);
        } else {
            rt_kprintf("unknown cmd: %s\r\n", agrv[0]);
        }
    } while (0);
    UNLOCK_TCPIP_CORE();
}

static void cmd_netif_ud(int argc, char *agrv[]) {
    if (argc < 2) {
        rt_kprintf("Usage: netif ud <up|down> <if>\r\n");
        return;
    }

    LOCK_TCPIP_CORE();
    do {
        struct netif *netif = netif_find(agrv[1]);
        if (netif == NULL) {
            rt_kprintf("no such interface: %s\r\n", agrv[1]);
            break;
        }

        if (strcmp(agrv[0], "up") == 0) {
            netif_set_up(netif);
        } else if (strcmp(agrv[0], "down") == 0) {
            netif_set_down(netif);
        } else {
            rt_kprintf("unknown cmd: %s\r\n", agrv[0]);
        }
    } while (0);
    UNLOCK_TCPIP_CORE();
}

static void cmd_netif_remove(int argc, char *agrv[]) {
    if (argc < 1) {
        rt_kprintf("Usage: netif remove <if>\r\n");
        return;
    }

    LOCK_TCPIP_CORE();
    do {
        struct netif *netif = netif_find(agrv[0]);
        if (netif == NULL) {
            rt_kprintf("no such interface: %s\r\n", agrv[0]);
            break;
        }

        netif_remove(netif);
    } while (0);
    UNLOCK_TCPIP_CORE();
}

static void cmd_netif(int argc, char *agrv[]) {
    if (argc < 2) {
        rt_kprintf("Usage: netif <cmd>\r\n");
        return;
    }

    void (*cmd_fn)(int, char *[]) = NULL;

    if (strcmp(agrv[1], "list") == 0) {
        cmd_fn = cmd_list_if;
    } else if (strcmp(agrv[1], "link") == 0) {
        cmd_fn = cmd_netif_link;
    } else if (strcmp(agrv[1], "ud") == 0) {
        cmd_fn = cmd_netif_ud;
    } else if (strcmp(agrv[1], "remove") == 0) {
        cmd_fn = cmd_netif_remove;
    } else {
        rt_kprintf("unknown cmd: %s\r\n", agrv[1]);
        return;
    }

    if (cmd_fn != NULL) {
        cmd_fn(argc - 2, &agrv[2]);
    }
}
MSH_CMD_EXPORT_ALIAS(cmd_netif, netif, network interface control);

#define PING_ID        0xAFAF
#define PING_DATA_SIZE 32
#define PING_TIMES     4
#define PING_RCV_TIMEO 1000
#define PING_DELAY     1000

static u16_t ping_seq_num = 0;

static void ping_recv(struct netconn *conn, u8_t index) {
    u32_t start_time = sys_now();
    u32_t elapsed_time = 0;
    struct netbuf *buf = NULL;

    err_t err = netconn_recv(conn, &buf);
    elapsed_time = sys_now() - start_time;

    if (err != ERR_OK) {
        rt_kprintf("ping: recv failed: %d, time=%d ms\r\n", err, elapsed_time);
        return;
    }

    do {
        struct ip_hdr *iphdr = (struct ip_hdr *)buf->p->payload;
        if (buf->p->len < IPH_HL_BYTES(iphdr) + sizeof(struct icmp_echo_hdr)) break;

        struct icmp_echo_hdr *iecho = (struct icmp_echo_hdr *)((char *)buf->p->payload + IPH_HL_BYTES(iphdr));
        if ((iecho->id == PING_ID) && (iecho->seqno == lwip_htons(ping_seq_num))) {
            if (IPH_TTL(iphdr) == 0) {
                rt_kprintf("%d bytes from %s icmp_seq=%d time=%d ms\r\n", buf->p->tot_len - sizeof(struct icmp_echo_hdr) - IPH_HL_BYTES(iphdr),
                           ipaddr_ntoa(netbuf_fromaddr(buf)), index, elapsed_time);
            } else {
                rt_kprintf("%d bytes from %s icmp_seq=%d ttl=%d time=%d ms\r\n", buf->p->tot_len - sizeof(struct icmp_echo_hdr) - IPH_HL_BYTES(iphdr),
                           ipaddr_ntoa(netbuf_fromaddr(buf)), index, IPH_TTL(iphdr), elapsed_time);
            }
        }
    } while (0);

    netbuf_delete(buf);
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

static void cmd_ping(int argc, char *agrv[]) {
    if (argc != 2) {
        rt_kprintf("Usage: ping <host>\r\n");
        return;
    }

    ip_addr_t target;
    struct netbuf buf = {0};
    uint8_t echo_data[sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE] = {0};
    struct icmp_echo_hdr *iecho = (struct icmp_echo_hdr *)echo_data;
    struct netconn *ping_conn = NULL;
    err_t err;

    if (ipaddr_aton(agrv[1], &target) == 0) {
        rt_kprintf("bad ip address\r\n");
        return;
    }

    if (netbuf_ref(&buf, echo_data, sizeof(echo_data)) != ERR_OK) {
        rt_kprintf("netbuf_ref failed\r\n");
        return;
    }

    ping_conn = netconn_new_with_proto_and_callback(NETCONN_RAW, IP_PROTO_ICMP, NULL);
    if (ping_conn == NULL) {
        netbuf_free(&buf);
        rt_kprintf("create raw netconn failed\r\n");
        return;
    }

    netconn_set_recvtimeout(ping_conn, PING_RCV_TIMEO);

    for (int i = 0; i < PING_TIMES; i++) {
        ping_prepare_echo(iecho, sizeof(echo_data));
        err = netconn_sendto(ping_conn, &buf, &target, 0);
        if (err != ERR_OK) {
            rt_kprintf("sendto failed\r\n");
        } else {
            ping_recv(ping_conn, i + 1);
        }

        sys_msleep(PING_DELAY);
    }

    netbuf_free(&buf);
    netconn_delete(ping_conn);
}
MSH_CMD_EXPORT_ALIAS(cmd_ping, ping, ping a host);

#endif /* RT_USING_FINSH */
