/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef LWIP_LWIPOPTS_H
#define LWIP_LWIPOPTS_H

#ifdef LWIP_OPTTEST_FILE
#include "lwipopts_test.h"
#else /* LWIP_OPTTEST_FILE */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include <sys/errno.h>
#define LWIP_ERRNO_INCLUDE "sys/errno.h"

#if defined (__GNUC__) & !defined (__CC_ARM)

#define LWIP_TIMEVAL_PRIVATE 0
#include <sys/time.h>

#endif

#define ETH_DEVICE_NAME "e0"

/* Define random number generator function */
#define LWIP_RAND() ((u32_t)rand())

#ifndef SSIZE_MAX
#define SSIZE_MAX INT_MAX
#endif

/* some errno not defined in newlib */
#ifndef ENSRNOTFOUND
#define ENSRNOTFOUND 163  /* Domain name not found */
#endif

#define LWIP_IPV4                  1
#define LWIP_IPV6                  0

#define NO_SYS                     0
#define LWIP_SOCKET                (NO_SYS==0)
#define LWIP_NETCONN               (NO_SYS==0)
#define LWIP_NETIF_API             (NO_SYS==0)

#define LWIP_IGMP                  0
#define LWIP_ICMP                  1

#define LWIP_SNMP                  0
#ifdef LWIP_HAVE_MBEDTLS
#define LWIP_SNMP_V3               (LWIP_SNMP)
#endif
#define SNMP_USE_NETCONN           0
#define SNMP_USE_RAW               0

#define LWIP_NETIF_HOSTNAME        1

#define LWIP_DNS                   1
#define LWIP_MDNS_RESPONDER        0

#define LWIP_NUM_NETIF_CLIENT_DATA (LWIP_MDNS_RESPONDER)

#define LWIP_HAVE_LOOPIF           1
#define LWIP_NETIF_LOOPBACK        0
#define LWIP_LOOPBACK_MAX_PBUFS    10

#define TCP_LISTEN_BACKLOG         0

#define LWIP_COMPAT_SOCKETS        1
#define LWIP_POSIX_SOCKETS_IO_NAMES 0
#define LWIP_SO_SNDTIMEO           1
#define LWIP_SO_RCVTIMEO           1
#define LWIP_SO_RCVBUF             1
#define RECV_BUFSIZE_DEFAULT       2000000000
#define SO_REUSE                   1

#define LWIP_TCPIP_CORE_LOCKING    1

#define LWIP_NETIF_LINK_CALLBACK        1
#define LWIP_NETIF_STATUS_CALLBACK      1
#define LWIP_NETIF_EXT_STATUS_CALLBACK  1

// #define LWIP_DEBUG
#ifdef LWIP_DEBUG

#define LWIP_DBG_MIN_LEVEL         0
#define PPP_DEBUG                  LWIP_DBG_OFF
#define MEM_DEBUG                  LWIP_DBG_OFF
#define MEMP_DEBUG                 LWIP_DBG_OFF
#define PBUF_DEBUG                 LWIP_DBG_OFF
#define API_LIB_DEBUG              LWIP_DBG_OFF
#define API_MSG_DEBUG              LWIP_DBG_OFF
#define TCPIP_DEBUG                LWIP_DBG_OFF
#define NETIF_DEBUG                LWIP_DBG_OFF
#define SOCKETS_DEBUG              LWIP_DBG_OFF
#define DNS_DEBUG                  LWIP_DBG_OFF
#define AUTOIP_DEBUG               LWIP_DBG_OFF
#define DHCP_DEBUG                 LWIP_DBG_OFF
#define ETHARP_DEBUG               LWIP_DBG_OFF
#define IP_DEBUG                   LWIP_DBG_OFF
#define IP_REASS_DEBUG             LWIP_DBG_OFF
#define ICMP_DEBUG                 LWIP_DBG_OFF
#define IGMP_DEBUG                 LWIP_DBG_OFF
#define UDP_DEBUG                  LWIP_DBG_OFF
#define TCP_DEBUG                  LWIP_DBG_OFF
#define TCP_INPUT_DEBUG            LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG           LWIP_DBG_OFF
#define TCP_RTO_DEBUG              LWIP_DBG_OFF
#define TCP_CWND_DEBUG             LWIP_DBG_OFF
#define TCP_WND_DEBUG              LWIP_DBG_OFF
#define TCP_FR_DEBUG               LWIP_DBG_OFF
#define TCP_QLEN_DEBUG             LWIP_DBG_OFF
#define TCP_RST_DEBUG              LWIP_DBG_OFF
#define LWIP_NAT_DEBUG             LWIP_DBG_OFF
#endif

#define LWIP_DBG_TYPES_ON         (LWIP_DBG_ON|LWIP_DBG_TRACE|LWIP_DBG_STATE|LWIP_DBG_FRESH|LWIP_DBG_HALT)


/* ---------- Memory options ---------- */
/* MEM_ALIGNMENT: should be set to the alignment of the CPU for which
   lwIP is compiled. 4 byte alignment -> define MEM_ALIGNMENT to 4, 2
   byte alignment -> define MEM_ALIGNMENT to 2. */
/* MSVC port: intel processors don't need 4-byte alignment,
   but are faster that way! */
#define MEM_ALIGNMENT           4U

#define MEMP_OVERFLOW_CHECK         1
#define LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT 1
#define MEM_LIBC_MALLOC             1

/* MEM_SIZE: the size of the heap memory. If the application will send
a lot of data that needs to be copied, this should be set high. */
// #define MEM_SIZE               (30 * 1024)

/* MEMP_NUM_PBUF: the number of memp struct pbufs. If the application
   sends a lot of data out of ROM (or other static memory), this
   should be set high. */
#define MEMP_NUM_PBUF           32 //16
/* MEMP_NUM_RAW_PCB: the number of UDP protocol control blocks. One
   per active RAW "connection". */
#define MEMP_NUM_RAW_PCB        3
/* MEMP_NUM_UDP_PCB: the number of UDP protocol control blocks. One
   per active UDP "connection". */
#define MEMP_NUM_UDP_PCB        6
/* MEMP_NUM_TCP_PCB: the number of simulatenously active TCP
   connections. */
#define MEMP_NUM_TCP_PCB        5
/* MEMP_NUM_TCP_PCB_LISTEN: the number of listening TCP
   connections. */
#define MEMP_NUM_TCP_PCB_LISTEN 8
/* MEMP_NUM_TCP_SEG: the number of simultaneously queued TCP
   segments. */
#define MEMP_NUM_TCP_SEG        16
/* MEMP_NUM_SYS_TIMEOUT: the number of simulateously active
   timeouts. */
#define MEMP_NUM_SYS_TIMEOUT    (LWIP_NUM_SYS_TIMEOUT_INTERNAL + 4)

/* The following four are used only with the sequential API and can be
   set to 0 if the application only will use the raw API. */
/* MEMP_NUM_NETBUF: the number of struct netbufs. */
#define MEMP_NUM_NETBUF         5
/* MEMP_NUM_NETCONN: the number of struct netconns. */
#define MEMP_NUM_NETCONN        16
/* MEMP_NUM_TCPIP_MSG_*: the number of struct tcpip_msg, which is used
   for sequential API communication and incoming packets. Used in
   src/api/tcpip.c. */
#define MEMP_NUM_TCPIP_MSG_API   16
#define MEMP_NUM_TCPIP_MSG_INPKT 16


/* ---------- Pbuf options ---------- */
/* PBUF_POOL_SIZE: the number of buffers in the pbuf pool. */
#define PBUF_POOL_SIZE          32

// /* PBUF_POOL_BUFSIZE: the size of each pbuf in the pbuf pool. */
// #define PBUF_POOL_BUFSIZE       256
#define PBUF_POOL_BUFSIZE       1536

/** SYS_LIGHTWEIGHT_PROT
 * define SYS_LIGHTWEIGHT_PROT in lwipopts.h if you want inter-task protection
 * for certain critical regions during buffer allocation, deallocation and memory
 * allocation and deallocation.
 */
#define SYS_LIGHTWEIGHT_PROT    (NO_SYS==0)


/* ---------- TCP options ---------- */
#define LWIP_TCP                1
#define TCP_TTL                 255

#define LWIP_ALTCP              0 //(LWIP_TCP)
#ifdef LWIP_HAVE_MBEDTLS
#define LWIP_ALTCP_TLS          (LWIP_TCP)
#define LWIP_ALTCP_TLS_MBEDTLS  (LWIP_TCP)
#endif


/* Controls if TCP should queue segments that arrive out of
   order. Define to 0 if your device is low on memory. */
#define TCP_QUEUE_OOSEQ         1

/* TCP Maximum segment size. */
#define TCP_MSS                 1460

/* TCP sender buffer space (bytes). */
#define TCP_SND_BUF             4096 //(4*TCP_MSS)

/* TCP sender buffer space (pbufs). This must be at least = 2 *
   TCP_SND_BUF/TCP_MSS for things to work. */
#define TCP_SND_QUEUELEN       (4 * TCP_SND_BUF/TCP_MSS)

/* TCP writable space (bytes). This must be less than or equal
   to TCP_SND_BUF. It is the amount of space which must be
   available in the tcp snd_buf for select to return writable */
#define TCP_SNDLOWAT           (TCP_SND_BUF/2)

/* TCP receive window. */
#define TCP_WND                 (8*TCP_MSS) //(2*TCP_MSS)

/* Maximum number of retransmissions of data segments. */
#define TCP_MAXRTX              12

/* Maximum number of retransmissions of SYN segments. */
#define TCP_SYNMAXRTX           4

#define LWIP_TCP_KEEPALIVE              1

/* ---------- DHCP options ---------- */
/* Define LWIP_DHCP to 1 if you want DHCP configuration of
   interfaces. */
#define LWIP_DHCP               1

/* 1 if you want to do an ARP check on the offered address
   (recommended). */
#define DHCP_DOES_ARP_CHECK    (LWIP_DHCP)

/* ---------- DHCPD options ---------- */
#define DHCPD_CLIENT_IP_MIN      10
#define DHCPD_CLIENT_IP_MAX      100
#define DHCPD_SERVER_IP          "192.168.2.32"
#define LWIP_NETIF_LOCK()        LOCK_TCPIP_CORE()
#define LWIP_NETIF_UNLOCK()      UNLOCK_TCPIP_CORE()

/* ---------- NAT options ---------- */
#define LWIP_USING_NAT
#ifdef LWIP_USING_NAT
#define IP_NAT                      1
#else
#define IP_NAT                      0
#endif


/* ---------- AUTOIP options ------- */
#define LWIP_AUTOIP            0
#define LWIP_DHCP_AUTOIP_COOP  (LWIP_DHCP && LWIP_AUTOIP)

/* ---------- ARP options ---------- */
#define LWIP_ARP                1
#define ARP_TABLE_SIZE          10
#define ARP_QUEUEING            1
#define LWIP_ARP_FILTER_NETIF   1

/* ---------- IP options ---------- */
/* Define IP_FORWARD to 1 if you wish to have the ability to forward
   IP packets across network interfaces. If you are going to run lwIP
   on a device with only one network interface, define this to 0. */
#define IP_FORWARD              0

/* IP reassembly and segmentation.These are orthogonal even
 * if they both deal with IP fragments */
#define IP_REASSEMBLY           0
#define IP_REASS_MAX_PBUFS      (10 * ((1500 + PBUF_POOL_BUFSIZE - 1) / PBUF_POOL_BUFSIZE))
#define MEMP_NUM_REASSDATA      IP_REASS_MAX_PBUFS
#define IP_FRAG                 0
#define IPV6_FRAG_COPYHEADER    1

/* ---------- ICMP options ---------- */
#define ICMP_TTL                255


/* ---------- UDP options ---------- */
#define LWIP_UDP                  1
// #define LWIP_UDPLITE              LWIP_UDP
#define UDP_TTL                   255

/* ---------- RAW options ---------- */
#define LWIP_RAW                1


/* ---------- Statistics options ---------- */

#define LWIP_STATS              1
#define LWIP_STATS_DISPLAY      0

#if LWIP_STATS
#define LINK_STATS              0
#define IP_STATS                0
#define ICMP_STATS              0
#define IGMP_STATS              0
#define IPFRAG_STATS            0
#define UDP_STATS               0
#define TCP_STATS               0
#define MEM_STATS               0
#define MEMP_STATS              0
#define PBUF_STATS              0
#define SYS_STATS               0
#define MIB2_STATS              1
#endif /* LWIP_STATS */

/* ---------- PPP options ---------- */

#define PPP_SUPPORT             1      /* Set > 0 for PPP */

#if PPP_SUPPORT

#define NUM_PPP                 1      /* Max PPP sessions. */


/* Select modules to enable.  Ideally these would be set in the makefile but
 * we're limited by the command line length so you need to modify the settings
 * in this file.
 */
#define PPPOE_SUPPORT           1
#define PPPOS_SUPPORT           1

#define PAP_SUPPORT             1      /* Set > 0 for PAP. */
#define CHAP_SUPPORT            1      /* Set > 0 for CHAP. */
#define MSCHAP_SUPPORT          0      /* Set > 0 for MSCHAP */
#define CBCP_SUPPORT            0      /* Set > 0 for CBCP (NOT FUNCTIONAL!) */
#define CCP_SUPPORT             0      /* Set > 0 for CCP */
#define VJ_SUPPORT              1      /* Set > 0 for VJ header compression. */
#define MD5_SUPPORT             1      /* Set > 0 for MD5 (see also CHAP) */

/* ---------- PPP device ---------- */
#define PPP_USING_PUBLIC_APN
#define PPP_APN_CMCC
#define PPP_CLIENT_NAME "uart3"
#define PPP_DEVICE_USING_SIM800
// #define PPP_DEVICE_DEBUG
// #define PPP_DEVICE_DEBUG_RX
// #define PPP_DEVICE_DEBUG_TX
// #define SIM800_POWER_PIN GET_PIN()
#endif /* PPP_SUPPORT */

#endif /* LWIP_OPTTEST_FILE */


#define LWIP_CHECKSUM_CTRL_PER_NETIF 1

// #define CHECKSUM_BY_HARDWARE
// #ifdef CHECKSUM_BY_HARDWARE
//   /* CHECKSUM_GEN_IP==0: Generate checksums by hardware for outgoing IP packets.*/
//   #define CHECKSUM_GEN_IP                 0
//   /* CHECKSUM_GEN_UDP==0: Generate checksums by hardware for outgoing UDP packets.*/
//   #define CHECKSUM_GEN_UDP                0
//   /* CHECKSUM_GEN_TCP==0: Generate checksums by hardware for outgoing TCP packets.*/
//   #define CHECKSUM_GEN_TCP                0
//   /* CHECKSUM_CHECK_IP==0: Check checksums by hardware for incoming IP packets.*/
//   #define CHECKSUM_CHECK_IP               0
//   /* CHECKSUM_CHECK_UDP==0: Check checksums by hardware for incoming UDP packets.*/
//   #define CHECKSUM_CHECK_UDP              0
//   /* CHECKSUM_CHECK_TCP==0: Check checksums by hardware for incoming TCP packets.*/
//   #define CHECKSUM_CHECK_TCP              0
//   /* CHECKSUM_CHECK_ICMP==0: Check checksums by hardware for incoming ICMP packets.*/
//   #define CHECKSUM_GEN_ICMP               0
// #else
//   /* CHECKSUM_GEN_IP==1: Generate checksums in software for outgoing IP packets.*/
//   #define CHECKSUM_GEN_IP                 1
//   /* CHECKSUM_GEN_UDP==1: Generate checksums in software for outgoing UDP packets.*/
//   #define CHECKSUM_GEN_UDP                1
//   /* CHECKSUM_GEN_TCP==1: Generate checksums in software for outgoing TCP packets.*/
//   #define CHECKSUM_GEN_TCP                1
//   /* CHECKSUM_CHECK_IP==1: Check checksums in software for incoming IP packets.*/
//   #define CHECKSUM_CHECK_IP               1
//   /* CHECKSUM_CHECK_UDP==1: Check checksums in software for incoming UDP packets.*/
//   #define CHECKSUM_CHECK_UDP              1
//   /* CHECKSUM_CHECK_TCP==1: Check checksums in software for incoming TCP packets.*/
//   #define CHECKSUM_CHECK_TCP              1
//   /* CHECKSUM_CHECK_ICMP==1: Check checksums by hardware for incoming ICMP packets.*/
//   #define CHECKSUM_GEN_ICMP               1
// #endif

/* ---------- NETBIOS options ---------- */
#define NETBIOS_LWIP_NAME "NETBIOSLWIPDEV"
#define LWIP_NETBIOS_RESPOND_NAME_QUERY 1

/* ---------- OS options ---------- */
#define DEFAULT_RAW_RECVMBOX_SIZE 2
#define DEFAULT_UDP_RECVMBOX_SIZE 10
#define DEFAULT_TCP_RECVMBOX_SIZE 10
#define DEFAULT_ACCEPTMBOX_SIZE   10

#define TCPIP_MBOX_SIZE             8
#define TCPIP_THREAD_PRIO           5
#define TCPIP_THREAD_STACKSIZE      2048
#define TCPIP_THREAD_NAME           "tcpip"

/* The following defines must be done even in OPTTEST mode: */

#if !defined(NO_SYS) || !NO_SYS /* default is 0 */

// #define LWIP_TCPIP_CORE_LOCKING_INPUT 1

void sys_check_core_locking(const char *file, int line);
#define LWIP_ASSERT_CORE_LOCKED()  sys_check_core_locking(__FILE__, __LINE__)
void sys_mark_tcpip_thread(void);
#define LWIP_MARK_TCPIP_THREAD()   sys_mark_tcpip_thread()

// #if !defined(LWIP_TCPIP_CORE_LOCKING) || LWIP_TCPIP_CORE_LOCKING /* default is 1 */
// void sys_lock_tcpip_core(void);
// #define LOCK_TCPIP_CORE()          sys_lock_tcpip_core()
// void sys_unlock_tcpip_core(void);
// #define UNLOCK_TCPIP_CORE()        sys_unlock_tcpip_core()
// #endif
#endif

/* ---------- Hook options ---------- */
#define LWIP_HOOK_FILENAME "lwip_hooks.h"

#if LWIP_ARP_FILTER_NETIF
#define LWIP_ARP_FILTER_NETIF_FN(p, n, t) lwip_arp_filter_netif((p), (n), (t))
#endif /* LWIP_ARP_FILTER_NETIF */

#define LWIP_HOOK_UNKNOWN_ETH_PROTOCOL lwip_hook_unknown_eth_protocol

#define LWIP_HOOK_IP4_ROUTE_SRC(src, dest) lwip_hook_ip4_route_src((src), (dest))

#endif /* LWIP_LWIPOPTS_H */