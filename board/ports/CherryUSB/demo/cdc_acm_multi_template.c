/*
 * Copyright (c) 2024, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "usbd_core.h"
#include "usbd_cdc_acm.h"
#include <board.h>

/*!< endpoint address */
#ifdef BSP_USING_USB_OTG_FS
#define CDC_IN_EP  0x81
#define CDC_OUT_EP 0x01
#define CDC_INT_EP 0x84

#define CDC_IN_EP2  0x82
#define CDC_OUT_EP2 0x02
#define CDC_INT_EP2 0x85

#define CDC_IN_EP3  0x83
#define CDC_OUT_EP3 0x03
#define CDC_INT_EP3 0x86
#else
#define CDC_IN_EP  0x81
#define CDC_OUT_EP 0x01
#define CDC_INT_EP 0x86

#define CDC_IN_EP2  0x82
#define CDC_OUT_EP2 0x02
#define CDC_INT_EP2 0x87

#define CDC_IN_EP3  0x83
#define CDC_OUT_EP3 0x03
#define CDC_INT_EP3 0x88

#define CDC_IN_EP4  0x84
#define CDC_OUT_EP4 0x04
#define CDC_INT_EP4 0x89

#define CDC_IN_EP5  0x85
#define CDC_OUT_EP5 0x05
#define CDC_INT_EP5 0x8A
#endif


#define USBD_VID           0xFFFF
#define USBD_PID           0xFFFF
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

/*!< config descriptor size */
#ifdef BSP_USING_USB_OTG_FS
#define USB_CONFIG_SIZE (9 + CDC_ACM_DESCRIPTOR_LEN * 3)
#else
#define USB_CONFIG_SIZE (9 + CDC_ACM_DESCRIPTOR_LEN * 5)
#endif

#ifdef CONFIG_USB_HS
#define CDC_MAX_MPS 512
#else
#define CDC_MAX_MPS 64
#endif

#define DBG_TAG               "cdc_acm_multi"
#define DBG_LVL               DBG_INFO
#include <rtdbg.h>

/*!< global descriptor */
static const uint8_t cdc_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0100, 0x01),
#ifdef BSP_USING_USB_OTG_FS
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x06, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
#else
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x0A, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
#endif
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, CDC_MAX_MPS, 0x02),
    CDC_ACM_DESCRIPTOR_INIT(0x02, CDC_INT_EP2, CDC_OUT_EP2, CDC_IN_EP2, CDC_MAX_MPS, 0x02),
    CDC_ACM_DESCRIPTOR_INIT(0x04, CDC_INT_EP3, CDC_OUT_EP3, CDC_IN_EP3, CDC_MAX_MPS, 0x02),
#ifdef BSP_USING_USB_OTG_HS
    CDC_ACM_DESCRIPTOR_INIT(0x06, CDC_INT_EP4, CDC_OUT_EP4, CDC_IN_EP4, CDC_MAX_MPS, 0x02),
    CDC_ACM_DESCRIPTOR_INIT(0x08, CDC_INT_EP5, CDC_OUT_EP5, CDC_IN_EP5, CDC_MAX_MPS, 0x02),
#endif
    ///////////////////////////////////////
    /// string0 descriptor
    ///////////////////////////////////////
    USB_LANGID_INIT(USBD_LANGID_STRING),
    ///////////////////////////////////////
    /// string1 descriptor
    ///////////////////////////////////////
    0x14,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'h', 0x00,                  /* wcChar1 */
    'e', 0x00,                  /* wcChar2 */
    'r', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    ///////////////////////////////////////
    /// string2 descriptor
    ///////////////////////////////////////
    0x26,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'h', 0x00,                  /* wcChar1 */
    'e', 0x00,                  /* wcChar2 */
    'r', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    ' ', 0x00,                  /* wcChar9 */
    'C', 0x00,                  /* wcChar10 */
    'D', 0x00,                  /* wcChar11 */
    'C', 0x00,                  /* wcChar12 */
    ' ', 0x00,                  /* wcChar13 */
    'D', 0x00,                  /* wcChar14 */
    'E', 0x00,                  /* wcChar15 */
    'M', 0x00,                  /* wcChar16 */
    'O', 0x00,                  /* wcChar17 */
    ///////////////////////////////////////
    /// string3 descriptor
    ///////////////////////////////////////
    0x16,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    '2', 0x00,                  /* wcChar0 */
    '0', 0x00,                  /* wcChar1 */
    '2', 0x00,                  /* wcChar2 */
    '2', 0x00,                  /* wcChar3 */
    '1', 0x00,                  /* wcChar4 */
    '2', 0x00,                  /* wcChar5 */
    '3', 0x00,                  /* wcChar6 */
    '4', 0x00,                  /* wcChar7 */
    '5', 0x00,                  /* wcChar8 */
    '6', 0x00,                  /* wcChar9 */
#ifdef CONFIG_USB_HS
    ///////////////////////////////////////
    /// device qualifier descriptor
    ///////////////////////////////////////
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x02,
    0x02,
    0x01,
    0x40,
    0x01,
    0x00,
#endif
    0x00
};

static rt_sem_t _tx_complete[5];
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t read_buffer[5][2048];
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t write_buffer[5][2048];

static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    switch (event) {
        case USBD_EVENT_RESET:
            break;
        case USBD_EVENT_CONNECTED:
            break;
        case USBD_EVENT_DISCONNECTED:
            break;
        case USBD_EVENT_RESUME:
            break;
        case USBD_EVENT_SUSPEND:
            break;
        case USBD_EVENT_CONFIGURED:
            /* setup first out ep read transfer */
            usbd_ep_start_read(busid, CDC_OUT_EP, read_buffer[0], 2048);
            usbd_ep_start_read(busid, CDC_OUT_EP2, read_buffer[1], 2048);
            usbd_ep_start_read(busid, CDC_OUT_EP3, read_buffer[2], 2048);
#ifdef BSP_USING_USB_OTG_HS
            usbd_ep_start_read(busid, CDC_OUT_EP4, read_buffer[3], 2048);
            usbd_ep_start_read(busid, CDC_OUT_EP5, read_buffer[4], 2048);
#endif
            break;
        case USBD_EVENT_SET_REMOTE_WAKEUP:
            break;
        case USBD_EVENT_CLR_REMOTE_WAKEUP:
            break;

        default:
            break;
    }
}

static void usbd_cdc_acm_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    USB_LOG_RAW("actual out ep:0x%2X len:%d\r\n", ep, nbytes);
    /* setup next out ep read transfer */
    int index = 0;
    switch(ep) {
        case CDC_OUT_EP:
            index = 0;
            break;

        case CDC_OUT_EP2:
            index = 1;
            break;

        case CDC_OUT_EP3:
            index = 2;
            break;
#ifdef BSP_USING_USB_OTG_HS
        case CDC_OUT_EP4:
            index = 3;
            break;

        case CDC_OUT_EP5:
            index = 4;
            break;
#endif
    }

    usbd_ep_start_read(busid, ep, read_buffer[index], 2048);
}

static void usbd_cdc_acm_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    USB_LOG_RAW("actual in ep:0x%02X len:%d\r\n", ep, nbytes);
    int index = 0;
    switch(ep) {
        case CDC_IN_EP:
            index = 0;
            break;

        case CDC_IN_EP2:
            index = 1;
            break;

        case CDC_IN_EP3:
            index = 2;
            break;

#ifdef BSP_USING_USB_OTG_HS
        case CDC_IN_EP4:
            index = 3;
            break;

        case CDC_IN_EP5:
            index = 4;
            break;
#endif
    }

    if ((nbytes % CDC_MAX_MPS) == 0 && nbytes) {
        /* send zlp */
        usbd_ep_start_write(busid, ep, NULL, 0);
    } else {
        rt_sem_release(_tx_complete[index]);
    }
}

static struct usbd_endpoint cdc_out_ep1 = {
    .ep_addr = CDC_OUT_EP,
    .ep_cb = usbd_cdc_acm_bulk_out
};

static struct usbd_endpoint cdc_in_ep1 = {
    .ep_addr = CDC_IN_EP,
    .ep_cb = usbd_cdc_acm_bulk_in
};

static struct usbd_endpoint cdc_out_ep2 = {
    .ep_addr = CDC_OUT_EP2,
    .ep_cb = usbd_cdc_acm_bulk_out
};

static struct usbd_endpoint cdc_in_ep2 = {
    .ep_addr = CDC_IN_EP2,
    .ep_cb = usbd_cdc_acm_bulk_in
};

static struct usbd_endpoint cdc_out_ep3 = {
    .ep_addr = CDC_OUT_EP3,
    .ep_cb = usbd_cdc_acm_bulk_out
};

static struct usbd_endpoint cdc_in_ep3 = {
    .ep_addr = CDC_IN_EP3,
    .ep_cb = usbd_cdc_acm_bulk_in
};

#ifdef BSP_USING_USB_OTG_HS
static struct usbd_endpoint cdc_out_ep4 = {
    .ep_addr = CDC_OUT_EP4,
    .ep_cb = usbd_cdc_acm_bulk_out
};

static struct usbd_endpoint cdc_in_ep4 = {
    .ep_addr = CDC_IN_EP4,
    .ep_cb = usbd_cdc_acm_bulk_in
};

static struct usbd_endpoint cdc_out_ep5 = {
    .ep_addr = CDC_OUT_EP5,
    .ep_cb = usbd_cdc_acm_bulk_out
};

static struct usbd_endpoint cdc_in_ep5 = {
    .ep_addr = CDC_IN_EP5,
    .ep_cb = usbd_cdc_acm_bulk_in
};
#endif

struct usbd_interface intf0;
struct usbd_interface intf1;
struct usbd_interface intf2;
struct usbd_interface intf3;
struct usbd_interface intf4;
struct usbd_interface intf5;

#ifdef BSP_USING_USB_OTG_HS
struct usbd_interface intf6;
struct usbd_interface intf7;
struct usbd_interface intf8;
struct usbd_interface intf9;
#endif

void cdc_acm_multi_init(uint8_t busid, uintptr_t reg_base)
{
    memset(write_buffer[0], '1111', 4);
    memset(write_buffer[1], '2222', 4);
    memset(write_buffer[2], '3333', 4);
    memset(write_buffer[3], '4444', 4);
    memset(write_buffer[4], '5555', 4);

    usbd_desc_register(busid, cdc_descriptor);

    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf0));
    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf1));
    usbd_add_endpoint(busid, &cdc_out_ep1);
    usbd_add_endpoint(busid, &cdc_in_ep1);

    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf2));
    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf3));
    usbd_add_endpoint(busid, &cdc_out_ep2);
    usbd_add_endpoint(busid, &cdc_in_ep2);

    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf4));
    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf5));
    usbd_add_endpoint(busid, &cdc_out_ep3);
    usbd_add_endpoint(busid, &cdc_in_ep3);

#ifdef BSP_USING_USB_OTG_HS
    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf6));
    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf7));
    usbd_add_endpoint(busid, &cdc_out_ep4);
    usbd_add_endpoint(busid, &cdc_in_ep4);

    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf8));
    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf9));
    usbd_add_endpoint(busid, &cdc_out_ep5);
    usbd_add_endpoint(busid, &cdc_in_ep5);
#endif

    usbd_initialize(busid, reg_base, usbd_event_handler);
}

static void cdc_acm_entry(void *parameter) {
    int index = (int)parameter;
    while (1) {
        rt_sem_control(_tx_complete[index], RT_IPC_CMD_RESET, RT_NULL);
        if (usbd_ep_start_write(0, CDC_IN_EP + index, write_buffer[index], 4) < 0) {
            LOG_E("write failed");
        } else {
            if (rt_sem_take(_tx_complete[index], 3000) != RT_EOK) {
                LOG_W("tx timeout");
            }
        }
        rt_thread_mdelay(100);
    }
}

int cdc_acm_multi_demo(void)
{
    char tmp[10];
    rt_thread_t tid[5];
    uint8_t num = 5;
#ifdef BSP_USING_USB_OTG_FS
    num = 3;
#endif

    for (int i = 0; i < num; i++) {
        rt_snprintf(tmp, sizeof(tmp), "cdc_tx%d", i);
        _tx_complete[i] = rt_sem_create(tmp, 0, RT_IPC_FLAG_FIFO);
        if (_tx_complete[i] == RT_NULL) {
            LOG_E("create tx sem %d failed", i);
            return -RT_ERROR;
        }

        rt_snprintf(tmp, sizeof(tmp), "cdc_acm%d", i);
        tid[i] = rt_thread_create(tmp, cdc_acm_entry, (void *)i, 1024, 5, 10);
        if (tid[i] == RT_NULL) {
            LOG_E("create cdc_acm thread %d failed", i);
            return -RT_ERROR;
        }
    }

#ifdef BSP_USING_USB_OTG_FS
    cdc_acm_multi_init(0, USB_OTG_FS_PERIPH_BASE);
#else
    cdc_acm_multi_init(0, USB_OTG_HS_PERIPH_BASE);
#endif

    for (int i = 0; i < num; i++) {
        rt_thread_startup(tid[i]);
    }

    return RT_EOK;
}
