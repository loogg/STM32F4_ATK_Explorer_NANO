#include "lwip/api.h"
#include <string.h>

#ifdef RT_USING_FINSH

/*****tcp client***** */
#define TCP_CLIENT_FLAG_CONNECT 0x01
#define TCP_CLIENT_FLAG_SEND    0x02
#define TCP_CLIENT_FLAG_CLOSE   0x04

struct tcp_client {
    struct netconn *conn;
    ip_addr_t ip;
    int port;
    uint8_t flags;
};

static struct tcp_client _tcp_client = {0};

static void tcp_client_event_callback(struct netconn *conn, enum netconn_evt evt, u16_t len) {
    rt_kprintf("event: %d, len: %d\r\n", evt, len);
}

static void tcp_client_thread_entry(void *parameter) {
    const char *send_data = "hello world";
    err_t err = ERR_OK;

    _tcp_client.conn = netconn_new_with_callback(NETCONN_TCP, tcp_client_event_callback);
    if (_tcp_client.conn == NULL) {
        rt_kprintf("create netconn failed\r\n");
        goto _exit;
    }

    // netconn_set_nonblocking(_tcp_client.conn, 1);
    netconn_set_recvtimeout(_tcp_client.conn, 100);
    err = netconn_connect(_tcp_client.conn, &_tcp_client.ip, _tcp_client.port);
    if (err != ERR_OK) {
        rt_kprintf("connect failed: %d\r\n", err);
        goto _exit;
    }

    while (1) {
        struct netbuf *buf;
        void *data;
        u16_t len;
        err = netconn_recv(_tcp_client.conn, &buf);
        if (err == ERR_OK) {
            do {
                netbuf_data(buf, &data, &len);
                rt_kprintf("recv: %.*s\r\n", len, (char *)data);
            } while (netbuf_next(buf) >= 0);
            netbuf_delete(buf);
        }

        if (_tcp_client.flags & TCP_CLIENT_FLAG_SEND) {
            err = netconn_write(_tcp_client.conn, send_data, strlen(send_data), NETCONN_COPY);
            if (err != ERR_OK) {
                rt_kprintf("send failed: %d\r\n", err);
            }
            _tcp_client.flags &= ~TCP_CLIENT_FLAG_SEND;
        }

        if (_tcp_client.flags & TCP_CLIENT_FLAG_CLOSE) {
            break;
        }
    }

_exit:
    if (_tcp_client.conn != NULL) {
        if (netconn_delete(_tcp_client.conn) != ERR_OK) {
            rt_kprintf("delete netconn failed\r\n");
        }
        _tcp_client.conn = NULL;
    }
    _tcp_client.flags = 0;
}

static int tcp_client_connect(int argc, char *agrv[]) {
    if (argc < 2) {
        rt_kprintf("Usage: tcp_client_netconn connect <ip> <port>\r\n");
        return -1;
    }

    if (_tcp_client.flags & TCP_CLIENT_FLAG_CONNECT) {
        rt_kprintf("already connecting\r\n");
        return -1;
    }

    if (ipaddr_aton(agrv[0], &_tcp_client.ip) == 0) {
        rt_kprintf("bad ip address\r\n");
        return -1;
    }
    _tcp_client.port = atoi(agrv[1]);

    rt_thread_t tid = rt_thread_create("tcp_client", tcp_client_thread_entry, NULL, 1024, 20, 10);
    if (tid == NULL) {
        rt_kprintf("create thread failed\r\n");
        return -1;
    }
    _tcp_client.flags |= TCP_CLIENT_FLAG_CONNECT;
    rt_thread_startup(tid);

    return 0;
}

static int tcp_client_send(int argc, char *agrv[]) {
    if (_tcp_client.flags & TCP_CLIENT_FLAG_CONNECT) {
        _tcp_client.flags |= TCP_CLIENT_FLAG_SEND;
    } else {
        rt_kprintf("not connect\r\n");
    }

    return 0;
}

static int tcp_client_close(int argc, char *agrv[]) {
    if (_tcp_client.flags & TCP_CLIENT_FLAG_CONNECT) {
        _tcp_client.flags |= TCP_CLIENT_FLAG_CLOSE;
    } else {
        rt_kprintf("not connect\r\n");
    }

    return 0;
}

static void tcp_client_netconn_test(int argc, char *agrv[]) {
    if (argc < 2) {
        rt_kprintf("Usage: tcp_client_netconn <cmd>\r\n");
        return;
    }

    int (*cmd_fn)(int, char *[]) = NULL;

    if (strcmp(agrv[1], "connect") == 0) {
        cmd_fn = tcp_client_connect;
    } else if (strcmp(agrv[1], "send") == 0) {
        cmd_fn = tcp_client_send;
    } else if (strcmp(agrv[1], "close") == 0) {
        cmd_fn = tcp_client_close;
    } else {
        rt_kprintf("unknown cmd: %s\r\n", agrv[1]);
        return;
    }

    if (cmd_fn != NULL) {
        cmd_fn(argc - 2, &agrv[2]);
    }
}
MSH_CMD_EXPORT_ALIAS(tcp_client_netconn_test, tcp_client_netconn, tcp client netconn test);
/*****tcp client***** */

#endif /* RT_USING_FINSH */
