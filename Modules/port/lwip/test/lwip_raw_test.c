#include "lwip/udp.h"
#include "lwip/tcpip.h"
#include "lwip/ip_addr.h"
#include <string.h>

#ifdef RT_USING_FINSH

/********UDP*********/
static struct udp_pcb *_udp = NULL;

void _udp_recv_fn(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    rt_kprintf("udp recv: %s:%d, len:%d\r\n", ipaddr_ntoa(addr), port, p->tot_len);
    pbuf_free(p);
}

static void udp_raw_new(int argc, char *agrv[]) {
    if (argc < 2) {
        rt_kprintf("Usage: udp_raw_new <ip>\r\n");
        return;
    }

    ip_addr_t ip;
    if (ipaddr_aton(agrv[1], &ip) == 0) {
        rt_kprintf("bad ip address\r\n");
        return;
    }

    LOCK_TCPIP_CORE();
    if (_udp != NULL) {
        udp_remove(_udp);
    }
    _udp = udp_new();
    udp_bind(_udp, &ip, 8080);
    udp_recv(_udp, _udp_recv_fn, NULL);
    UNLOCK_TCPIP_CORE();
}
MSH_CMD_EXPORT(udp_raw_new, udp raw new);

static void udp_raw_send(int argc, char *agrv[]) {
    if (_udp == NULL) {
        rt_kprintf("udp pcb is not created\r\n");
        return;
    }

    if (argc < 2) {
        rt_kprintf("Usage: udp_raw_send <ip>\r\n");
        return;
    }

    ip_addr_t ip;
    if (ipaddr_aton(agrv[1], &ip) == 0) {
        rt_kprintf("bad ip address\r\n");
        return;
    }

    struct pbuf *p;
    char *data = "hello world";

    p = pbuf_alloc(PBUF_TRANSPORT, strlen(data), PBUF_RAM);
    if (p != NULL) {
        pbuf_take(p, data, strlen(data));

        LOCK_TCPIP_CORE();
        udp_sendto(_udp, p, &ip, 8080);
        UNLOCK_TCPIP_CORE();

        pbuf_free(p);
    }
}
MSH_CMD_EXPORT(udp_raw_send, udp raw send);
/********UDP*********/

#endif /* RT_USING_FINSH */
