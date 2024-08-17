#include "netif/ethernetif.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include <board.h>
#include <string.h>

#define DBG_TAG "drv.eth"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

// #define ETH_RX_DUMP
// #define ETH_TX_DUMP

/*Static IP ADDRESS: IP_ADDR0.IP_ADDR1.IP_ADDR2.IP_ADDR3 */
#define IP_ADDR0 (uint8_t)192
#define IP_ADDR1 (uint8_t)168
#define IP_ADDR2 (uint8_t)2
#define IP_ADDR3 (uint8_t)32

/*NETMASK*/
#define NETMASK_ADDR0 (uint8_t)255
#define NETMASK_ADDR1 (uint8_t)255
#define NETMASK_ADDR2 (uint8_t)255
#define NETMASK_ADDR3 (uint8_t)0

/*Gateway Address*/
#define GW_ADDR0 (uint8_t)192
#define GW_ADDR1 (uint8_t)168
#define GW_ADDR2 (uint8_t)2
#define GW_ADDR3 (uint8_t)1

#define ETH_RESET_Pin       GPIO_PIN_3
#define ETH_RESET_GPIO_Port GPIOD

static ETH_DMADescTypeDef *DMARxDscrTab, *DMATxDscrTab;
static rt_uint8_t *Rx_Buff, *Tx_Buff;
static ETH_HandleTypeDef EthHandle;
static struct eth_device _device = {0};

#if defined(ETH_RX_DUMP) || defined(ETH_TX_DUMP)
#define __is_print(ch) ((unsigned int)((ch) - ' ') < 127u - ' ')
static void dump_hex(const rt_uint8_t *ptr, rt_size_t buflen)
{
    unsigned char *buf = (unsigned char *)ptr;
    int i, j;

    for (i = 0; i < buflen; i += 16)
    {
        rt_kprintf("%08X: ", i);

        for (j = 0; j < 16; j++)
            if (i + j < buflen)
                rt_kprintf("%02X ", buf[i + j]);
            else
                rt_kprintf("   ");
        rt_kprintf(" ");

        for (j = 0; j < 16; j++)
            if (i + j < buflen)
                rt_kprintf("%c", __is_print(buf[i + j]) ? buf[i + j] : '.');
        rt_kprintf("\n");
    }
}
#endif

static void phy_init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOD_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(ETH_RESET_GPIO_Port, ETH_RESET_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin : ETH_RESET_Pin */
    GPIO_InitStruct.Pin = ETH_RESET_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(ETH_RESET_GPIO_Port, &GPIO_InitStruct);
}

static void phy_reset(void) {
    HAL_GPIO_WritePin(ETH_RESET_GPIO_Port, ETH_RESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(ETH_RESET_GPIO_Port, ETH_RESET_Pin, GPIO_PIN_SET);
}

static void phy_linkchange(void) {
    static uint8_t phy_speed = 0;
    uint8_t phy_speed_new = 0;
    uint32_t phyreg = 0;

    if (HAL_ETH_ReadPHYRegister(&EthHandle, PHY_BSR, &phyreg) != HAL_OK) {
        LOG_E("read phy basic reg faild");
        return;
    }

    LOG_D("phy basic status reg is 0x%X", phyreg);

    if (phyreg & (PHY_LINKED_STATUS | PHY_AUTONEGO_COMPLETE)) {
        phyreg = 0;

        phy_speed_new |= ETH_DEVICE_PHY_LINK;

        if (HAL_ETH_ReadPHYRegister(&EthHandle, PHY_SR, &phyreg) != HAL_OK) {
            LOG_E("read phy status reg faild");
            return;
        }
        LOG_D("phy control status reg is 0x%X", phyreg);

        if ((phyreg & PHY_SPEED_STATUS) != PHY_SPEED_STATUS) {
            phy_speed_new |= ETH_DEVICE_PHY_100M;
        }

        if (phyreg & PHY_DUPLEX_STATUS) {
            phy_speed_new |= ETH_DEVICE_PHY_FULL_DUPLEX;
        }
    }

    if (phy_speed != phy_speed_new) {
        phy_speed = phy_speed_new;

        if (_device.parent.rx_indicate != RT_NULL) {
            _device.parent.rx_indicate(&_device.parent, ETH_DEVICE_RX_NOTIFY_TYPE_LINK | ((uint32_t)phy_speed << 8));
        }
    }
}

static void phy_monitor_thread_entry(void *parameter) {
    LOG_D("RESET PHY!");

    HAL_ETH_WritePHYRegister(&EthHandle, PHY_BCR, PHY_RESET);
    HAL_Delay(2000);
    HAL_ETH_WritePHYRegister(&EthHandle, PHY_BCR, PHY_AUTONEGOTIATION);

    while (1) {
        rt_thread_mdelay(500);
        phy_linkchange();
    }
}

static rt_err_t rt_stm32_eth_init(rt_device_t dev) {
    rt_err_t state = RT_EOK;
    rt_thread_t tid = RT_NULL;

    /* Prepare receive and send buffers */
    Rx_Buff = (rt_uint8_t *)rt_calloc(ETH_RXBUFNB, ETH_MAX_PACKET_SIZE);
    if (Rx_Buff == RT_NULL) {
        LOG_E("No memory");
        state = -RT_ENOMEM;
        goto __exit;
    }

    Tx_Buff = (rt_uint8_t *)rt_calloc(ETH_TXBUFNB, ETH_MAX_PACKET_SIZE);
    if (Tx_Buff == RT_NULL) {
        LOG_E("No memory");
        state = -RT_ENOMEM;
        goto __exit;
    }

    DMARxDscrTab = (ETH_DMADescTypeDef *)rt_calloc(ETH_RXBUFNB, sizeof(ETH_DMADescTypeDef));
    if (DMARxDscrTab == RT_NULL) {
        LOG_E("No memory");
        state = -RT_ENOMEM;
        goto __exit;
    }

    DMATxDscrTab = (ETH_DMADescTypeDef *)rt_calloc(ETH_TXBUFNB, sizeof(ETH_DMADescTypeDef));
    if (DMATxDscrTab == RT_NULL) {
        LOG_E("No memory");
        state = -RT_ENOMEM;
        goto __exit;
    }

    phy_init();
    phy_reset();

    EthHandle.Instance = ETH;
    EthHandle.Init.MACAddr = _device.macaddr;
    EthHandle.Init.AutoNegotiation = ETH_AUTONEGOTIATION_DISABLE;
    EthHandle.Init.Speed = ETH_SPEED_100M;
    EthHandle.Init.DuplexMode = ETH_MODE_FULLDUPLEX;
    EthHandle.Init.MediaInterface = ETH_MEDIA_INTERFACE_RMII;
    EthHandle.Init.RxMode = ETH_RXINTERRUPT_MODE;
    EthHandle.Init.ChecksumMode = ETH_CHECKSUM_BY_HARDWARE;
    EthHandle.Init.PhyAddress = LAN8720A_PHY_ADDRESS;

    HAL_ETH_DeInit(&EthHandle);

    /* configure ethernet peripheral (GPIOs, clocks, MAC, DMA) */
    if (HAL_ETH_Init(&EthHandle) != HAL_OK) {
        LOG_E("eth hardware init failed");
    } else {
        LOG_D("eth hardware init success");
    }

    /* Initialize Tx Descriptors list: Chain Mode */
    HAL_ETH_DMATxDescListInit(&EthHandle, DMATxDscrTab, Tx_Buff, ETH_TXBUFNB);

    /* Initialize Rx Descriptors list: Chain Mode  */
    HAL_ETH_DMARxDescListInit(&EthHandle, DMARxDscrTab, Rx_Buff, ETH_RXBUFNB);

    tid = rt_thread_create("phy", phy_monitor_thread_entry, RT_NULL, 1024, RT_THREAD_PRIORITY_MAX - 5, 2);
    if (tid != RT_NULL) {
        rt_thread_startup(tid);
    } else {
        LOG_E("create phy monitor thread failed");
        state = -RT_ERROR;
    }

__exit:
    if (state != RT_EOK) {
        if (Rx_Buff) {
            rt_free(Rx_Buff);
        }

        if (Tx_Buff) {
            rt_free(Tx_Buff);
        }

        if (DMARxDscrTab) {
            rt_free(DMARxDscrTab);
        }

        if (DMATxDscrTab) {
            rt_free(DMATxDscrTab);
        }
    }

    return state;
}

static err_t stm32_eth_tx(rt_device_t dev, struct pbuf *p) {
    struct pbuf *q;
    err_t errval;
    uint8_t *buffer = (uint8_t *)(EthHandle.TxDesc->Buffer1Addr);
    __IO ETH_DMADescTypeDef *DmaTxDesc;
    uint32_t framelength = 0;
    uint32_t bufferoffset = 0;
    uint32_t byteslefttocopy = 0;
    uint32_t payloadoffset = 0;
    HAL_StatusTypeDef hal_eth_trans_status;

    DmaTxDesc = EthHandle.TxDesc;
    bufferoffset = 0;

#if ETH_PAD_SIZE
    pbuf_remove_header(p, ETH_PAD_SIZE); /* drop the padding word */
#endif

    for (q = p; q != NULL; q = q->next) {
        /* Send the data from the pbuf to the interface, one pbuf at a
           time. The size of the data in each pbuf is kept in the ->len
           variable. */
        /* Is this buffer available? If not, goto error */
        if ((DmaTxDesc->Status & ETH_DMATXDESC_OWN) != (uint32_t)RESET) {
            LOG_E("buffer not valid");
            errval = ERR_USE;
            goto error;
        }

        /* Get bytes in current lwIP buffer */
        byteslefttocopy = q->len;
        payloadoffset = 0;

        /* Check if the length of data to copy is bigger than Tx buffer size*/
        while ((byteslefttocopy + bufferoffset) > ETH_TX_BUF_SIZE) {
            /* Copy data to Tx buffer*/
            memcpy((uint8_t *)((uint8_t *)buffer + bufferoffset), (uint8_t *)((uint8_t *)q->payload + payloadoffset),
                   (ETH_TX_BUF_SIZE - bufferoffset));

            /* Point to next descriptor */
            DmaTxDesc = (ETH_DMADescTypeDef *)(DmaTxDesc->Buffer2NextDescAddr);

            /* Check if the buffer is available */
            if ((DmaTxDesc->Status & ETH_DMATXDESC_OWN) != (uint32_t)RESET) {
                LOG_E("dma tx desc buffer is not valid");
                errval = ERR_USE;
                goto error;
            }

            buffer = (uint8_t *)(DmaTxDesc->Buffer1Addr);

            byteslefttocopy = byteslefttocopy - (ETH_TX_BUF_SIZE - bufferoffset);
            payloadoffset = payloadoffset + (ETH_TX_BUF_SIZE - bufferoffset);
            framelength = framelength + (ETH_TX_BUF_SIZE - bufferoffset);
            bufferoffset = 0;
        }

        /* Copy the remaining bytes */
        memcpy((uint8_t *)((uint8_t *)buffer + bufferoffset), (uint8_t *)((uint8_t *)q->payload + payloadoffset), byteslefttocopy);
        bufferoffset = bufferoffset + byteslefttocopy;
        framelength = framelength + byteslefttocopy;
    }

#ifdef ETH_TX_DUMP
    dump_hex(buffer, p->tot_len);
#endif

    LOG_D("transmit frame length :%d", framelength);

    /* wait for unlocked */
    while (EthHandle.Lock == HAL_LOCKED);

    /* Prepare transmit descriptors to give to DMA */
    hal_eth_trans_status = HAL_ETH_TransmitFrame(&EthHandle, framelength);
    if (hal_eth_trans_status != HAL_OK) {
        LOG_E("eth transmit frame faild: %d", hal_eth_trans_status);
    }

    errval = ERR_OK;

error:

    /* When Transmit Underflow flag is set, clear it and issue a Transmit Poll Demand to resume transmission */
    if ((EthHandle.Instance->DMASR & ETH_DMASR_TUS) != (uint32_t)RESET) {
        /* Clear TUS ETHERNET DMA flag */
        EthHandle.Instance->DMASR = ETH_DMASR_TUS;

        /* Resume DMA transmission*/
        EthHandle.Instance->DMATPDR = 0;
    }

    MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p->tot_len);
    if (((u8_t *)p->payload)[0] & 1) {
        /* broadcast or multicast packet*/
        MIB2_STATS_NETIF_INC(netif, ifoutnucastpkts);
    } else {
        /* unicast packet */
        MIB2_STATS_NETIF_INC(netif, ifoutucastpkts);
    }
    /* increase ifoutdiscards or ifouterrors on error */
    if (errval != ERR_OK) {
        MIB2_STATS_NETIF_INC(netif, ifouterrors);
    }

#if ETH_PAD_SIZE
    pbuf_add_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

    LINK_STATS_INC(link.xmit);

    return errval;
}

struct pbuf *stm32_eth_rx(rt_device_t dev) {
    struct pbuf *p = NULL;
    struct pbuf *q;
    uint16_t len;
    uint8_t *buffer;
    __IO ETH_DMADescTypeDef *dmarxdesc;
    uint32_t bufferoffset = 0;
    uint32_t payloadoffset = 0;
    uint32_t byteslefttocopy = 0;
    uint32_t i = 0;

    if (HAL_ETH_GetReceivedFrame_IT(&EthHandle) != HAL_OK) {
        LOG_D("receive frame faild");
        return NULL;
    }

    /* Obtain the size of the packet and put it into the "len"
       variable. */
    len = EthHandle.RxFrameInfos.length;
    buffer = (uint8_t *)EthHandle.RxFrameInfos.buffer;

    LOG_D("receive frame len : %d", len);

    if (len > 0) {
#if ETH_PAD_SIZE
        len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif

        /* We allocate a pbuf chain of pbufs from the pool. */
        p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    }

    if (p != NULL) {
#if ETH_PAD_SIZE
        pbuf_remove_header(p, ETH_PAD_SIZE); /* drop the padding word */
#endif

#ifdef ETH_RX_DUMP
        dump_hex(buffer, p->tot_len);
#endif

        dmarxdesc = EthHandle.RxFrameInfos.FSRxDesc;
        bufferoffset = 0;

        /* We iterate over the pbuf chain until we have read the entire
         * packet into the pbuf. */
        for (q = p; q != NULL; q = q->next) {
            /* Read enough bytes to fill this pbuf in the chain. The
             * available data in the pbuf is given by the q->len
             * variable.
             * This does not necessarily have to be a memcpy, you can also preallocate
             * pbufs for a DMA-enabled MAC and after receiving truncate it to the
             * actually received size. In this case, ensure the tot_len member of the
             * pbuf is the sum of the chained pbuf len members.
             */
            byteslefttocopy = q->len;
            payloadoffset = 0;

            /* Check if the length of bytes to copy in current pbuf is bigger than Rx buffer size */
            while ((byteslefttocopy + bufferoffset) > ETH_RX_BUF_SIZE) {
                /* Copy data to pbuf */
                memcpy((uint8_t *)((uint8_t *)q->payload + payloadoffset), (uint8_t *)((uint8_t *)buffer + bufferoffset),
                       (ETH_RX_BUF_SIZE - bufferoffset));

                /* Point to next descriptor */
                dmarxdesc = (ETH_DMADescTypeDef *)(dmarxdesc->Buffer2NextDescAddr);
                buffer = (uint8_t *)(dmarxdesc->Buffer1Addr);

                byteslefttocopy = byteslefttocopy - (ETH_RX_BUF_SIZE - bufferoffset);
                payloadoffset = payloadoffset + (ETH_RX_BUF_SIZE - bufferoffset);
                bufferoffset = 0;
            }

            /* Copy remaining data in pbuf */
            memcpy((uint8_t *)((uint8_t *)q->payload + payloadoffset), (uint8_t *)((uint8_t *)buffer + bufferoffset), byteslefttocopy);
            bufferoffset = bufferoffset + byteslefttocopy;
        }

        MIB2_STATS_NETIF_ADD(netif, ifinoctets, p->tot_len);
        if (((u8_t *)p->payload)[0] & 1) {
            /* broadcast or multicast packet*/
            MIB2_STATS_NETIF_INC(netif, ifinnucastpkts);
        } else {
            /* unicast packet*/
            MIB2_STATS_NETIF_INC(netif, ifinucastpkts);
        }
#if ETH_PAD_SIZE
        pbuf_add_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

        LINK_STATS_INC(link.recv);
    } else {
        LINK_STATS_INC(link.memerr);
        LINK_STATS_INC(link.drop);
        MIB2_STATS_NETIF_INC(netif, ifindiscards);
    }

    /* Release descriptors to DMA */
    /* Point to first descriptor */
    dmarxdesc = EthHandle.RxFrameInfos.FSRxDesc;
    /* Set Own bit in Rx descriptors: gives the buffers back to DMA */
    for (i = 0; i < EthHandle.RxFrameInfos.SegCount; i++) {
        dmarxdesc->Status |= ETH_DMARXDESC_OWN;
        dmarxdesc = (ETH_DMADescTypeDef *)(dmarxdesc->Buffer2NextDescAddr);
    }

    /* Clear Segment_Count */
    EthHandle.RxFrameInfos.SegCount = 0;

    /* When Rx Buffer unavailable flag is set: clear it and resume reception */
    if ((EthHandle.Instance->DMASR & ETH_DMASR_RBUS) != (uint32_t)RESET) {
        /* Clear RBUS ETHERNET DMA flag */
        EthHandle.Instance->DMASR = ETH_DMASR_RBUS;
        /* Resume DMA reception */
        EthHandle.Instance->DMARPDR = 0;
    }
    return p;
}

static rt_err_t stm32_eth_link_change(rt_device_t dev, uint8_t link_status) {
    LOG_I("update ethernet config");

    if (link_status & ETH_DEVICE_PHY_LINK) {
        LOG_I("link up");
        if (link_status & ETH_DEVICE_PHY_100M) {
            LOG_I("100Mbps");
            EthHandle.Init.Speed = ETH_SPEED_100M;
        } else {
            LOG_I("10Mbps");
            EthHandle.Init.Speed = ETH_SPEED_10M;
        }

        if (link_status & ETH_DEVICE_PHY_FULL_DUPLEX) {
            LOG_I("full-duplex");
            EthHandle.Init.DuplexMode = ETH_MODE_FULLDUPLEX;
        } else {
            LOG_I("half-duplex");
            EthHandle.Init.DuplexMode = ETH_MODE_HALFDUPLEX;
        }

        /* ETHERNET MAC Re-Configuration */
        HAL_ETH_ConfigMAC(&EthHandle, (ETH_MACInitTypeDef *)NULL);

        /* Restart MAC interface */
        HAL_ETH_Start(&EthHandle);
    } else {
        LOG_I("link down");

        /* Stop MAC interface */
        HAL_ETH_Stop(&EthHandle);
    }

    return RT_EOK;
}

/* interrupt service routine */
void ETH_IRQHandler(void) {
    /* enter interrupt */
    rt_interrupt_enter();

    HAL_ETH_IRQHandler(&EthHandle);

    /* leave interrupt */
    rt_interrupt_leave();
}

void HAL_ETH_ErrorCallback(ETH_HandleTypeDef *heth) { LOG_E("eth err"); }

void HAL_ETH_TxCpltCallback(ETH_HandleTypeDef *heth) { LOG_D("eth tx cplt"); }

void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *heth) {
    rt_err_t ret = -RT_ERROR;

    if (_device.parent.rx_indicate != RT_NULL) {
        ret = _device.parent.rx_indicate(&_device.parent, ETH_DEVICE_RX_NOTIFY_TYPE_INPUT);
    }

    if (ret != RT_EOK)
    {
        LOG_I("RxCpltCallback err = %d", ret);
    }
}

static int rt_hw_stm32_eth_init(void) {
    rt_device_t dev = &_device.parent;

    dev->type = RT_Device_Class_NetIf;
    dev->rx_indicate = RT_NULL;
    dev->tx_complete = RT_NULL;

    dev->init = rt_stm32_eth_init;
    dev->open = RT_NULL;
    dev->close = RT_NULL;
    dev->read = RT_NULL;
    dev->write = RT_NULL;
    dev->control = RT_NULL;

    rt_device_register(dev, "e0", RT_DEVICE_FLAG_RDWR);

    /* OUI 00-80-E1 STMICROELECTRONICS. */
    _device.macaddr[0] = 0x00;
    _device.macaddr[1] = 0x80;
    _device.macaddr[2] = 0xE1;
    /* generate MAC addr from 96bit unique ID (only for test). */
    _device.macaddr[3] = *(rt_uint8_t *)(UID_BASE + 4);
    _device.macaddr[4] = *(rt_uint8_t *)(UID_BASE + 2);
    _device.macaddr[5] = *(rt_uint8_t *)(UID_BASE + 0);

    _device.eth_rx = stm32_eth_rx;
    _device.eth_tx = stm32_eth_tx;
    _device.eth_link_change = stm32_eth_link_change;

    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_stm32_eth_init);
