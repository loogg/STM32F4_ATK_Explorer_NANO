#include "mbtcp.h"
#include <rtthread.h>
#include <agile_modbus.h>
#include <sys/socket.h>

#define DBG_TAG "mbtcp"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define MBTCP_SERVER_IP "192.168.2.124"
#define MBTCP_SERVER_PORT 502

uint16_t input_hold_registers[10];
uint16_t output_hold_registers[10];
uint8_t output_update_flag[10];

void mbtcp_copy_input_registers(uint16_t *src, int count)
{
    for (int i = 0; i < count; i++)
        src[i] = input_hold_registers[i];
}

void mbtcp_set_output_register(uint16_t *val, int count)
{
    for (int i = 0; i < count; i++) {
        if (output_hold_registers[i] != val[i]) {
            output_hold_registers[i] = val[i];
            output_update_flag[i] = 1;
        }
    }
}

static int tcp_connect(const char *ip, int port)
{
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == -1)
        return -1;

    int option = 1;
    int rc = setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const void *)&option, sizeof(int));
    if (rc == -1) {
        closesocket(s);
        s = -1;
        return -1;
    }

    struct timeval tv;
    tv.tv_sec = 20;
    tv.tv_usec = 0;
    rc = setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const void *)&tv, sizeof(struct timeval));
    if (rc == -1) {
        closesocket(s);
        s = -1;
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    rc = connect(s, (struct sockaddr *)&addr, sizeof(addr));
    if (rc == -1) {
        closesocket(s);
        s = -1;
        return -1;
    }

    return s;
}

static int tcp_flush(int s)
{
    int rc;

    do {
        uint8_t devnull[100];

        if (s == -1)
            break;

        rc = recv(s, devnull, sizeof(devnull), MSG_DONTWAIT);

    } while (rc == 100);

    return 0;
}

static int tcp_receive(int s, uint8_t *buf, int bufsz, int timeout)
{
    int len = 0;
    int rc = 0;
    fd_set readset, exceptset;
    struct timeval tv;

    while (bufsz > 0) {
        FD_ZERO(&readset);
        FD_ZERO(&exceptset);
        FD_SET(s, &readset);
        FD_SET(s, &exceptset);

        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
        rc = select(s + 1, &readset, NULL, &exceptset, &tv);
        if (rc == -1) {
            if (errno == EINTR) {
                continue;
            }
        }

        if (rc <= 0)
            break;

        if (FD_ISSET(s, &exceptset)) {
            rc = -1;
            break;
        }

        rc = recv(s, buf + len, bufsz, MSG_DONTWAIT);
        if (rc < 0)
            break;
        if (rc == 0) {
            if (len == 0)
                rc = -1;
            break;
        }

        len += rc;
        bufsz -= rc;

        timeout = 50;
    }

    if (rc >= 0)
        rc = len;

    return rc;
}

static int tcp_send(int s, const uint8_t *buf, int length)
{
    return send(s, buf, length, MSG_NOSIGNAL);
}

static void tcp_close(int s)
{
    if (s != -1) {
        shutdown(s, SHUT_RDWR);
        closesocket(s);
    }
}

static void cycle_entry(void *parameter) {
    int sock_fd = -1;
    int send_len, read_len, rc;
    uint8_t ctx_send_buf[AGILE_MODBUS_MAX_ADU_LENGTH];
    uint8_t ctx_read_buf[AGILE_MODBUS_MAX_ADU_LENGTH];

    agile_modbus_tcp_t ctx_tcp;
    agile_modbus_t *ctx = &ctx_tcp._ctx;
    agile_modbus_tcp_init(&ctx_tcp, ctx_send_buf, sizeof(ctx_send_buf), ctx_read_buf, sizeof(ctx_read_buf));
    agile_modbus_set_slave(ctx, 1);

    LOG_I("Running.");

_start:
    sock_fd = tcp_connect(MBTCP_SERVER_IP, MBTCP_SERVER_PORT);
    if (sock_fd < 0) {
        LOG_E("Connect to server failed.");
        goto _restart;
    }

    while(1) {
        rt_thread_mdelay(10);

        tcp_flush(sock_fd);
        send_len = agile_modbus_serialize_read_registers(ctx, 0, 10);
        tcp_send(sock_fd, ctx->send_buf, send_len);
        read_len = tcp_receive(sock_fd, ctx->read_buf, ctx->read_bufsz, 1000);
        if (read_len < 0) {
            LOG_E("Receive error, now exit.");
            break;
        }

        if (read_len == 0) {
            LOG_W("Receive timeout.");
        } else {
            rc = agile_modbus_deserialize_read_registers(ctx, read_len, input_hold_registers);
            if (rc < 0) {
                LOG_W("Receive failed.");
                if (rc != -1) LOG_W("Error code:%d", -128 - rc);
            }
        }

        for (int i = 0; i < 10; i++) {
            if (output_update_flag[i]) {
                output_update_flag[i] = 0;
                tcp_flush(sock_fd);
                send_len = agile_modbus_serialize_write_register(ctx, i, output_hold_registers[i]);
                tcp_send(sock_fd, ctx->send_buf, send_len);
                read_len = tcp_receive(sock_fd, ctx->read_buf, ctx->read_bufsz, 1000);
                if (read_len <= 0) {
                    LOG_E("Receive error, continue.");
                    continue;
                }

                rc = agile_modbus_deserialize_write_register(ctx, read_len);
                if (rc < 0) {
                    LOG_W("Receive failed.");
                    if (rc != -1) LOG_W("Error code:%d", -128 - rc);
                }
            }
        }
    }
_restart:
    LOG_W("restart");
    tcp_close(sock_fd);
    sock_fd = -1;
    rt_thread_mdelay(1000);
    goto _start;
}

int mbtcp_cycle_init(void) {
    rt_thread_t tid = rt_thread_create("mbtcp_cycle", cycle_entry, RT_NULL, 4096, 25, 10);
    if (tid == RT_NULL) {
        return -RT_ERROR;
    }
    rt_thread_startup(tid);

    return RT_EOK;
}

