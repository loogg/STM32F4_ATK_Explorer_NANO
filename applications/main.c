#include <board.h>
#include <rtthread.h>
#include "netif/ethernetif.h"
#include "dhcp_server.h"

#define LED0_Pin       GPIO_PIN_9
#define LED0_GPIO_Port GPIOF
#define LED1_Pin       GPIO_PIN_10
#define LED1_GPIO_Port GPIOF

static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* USER CODE BEGIN MX_GPIO_Init_1 */
    /* USER CODE END MX_GPIO_Init_1 */

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOF, LED0_Pin | LED1_Pin, GPIO_PIN_SET);

    /*Configure GPIO pins : LED0_Pin LED1_Pin */
    GPIO_InitStruct.Pin = LED0_Pin | LED1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    /* USER CODE BEGIN MX_GPIO_Init_2 */
    /* USER CODE END MX_GPIO_Init_2 */
}

int ppp_sample_start(void);
void cmd_lwip_nat(void);

int main(void) {
    MX_GPIO_Init();

    struct eth_device_config config = {0};
    config.dhcp_enable = 0;
    config.dhcp_timeout = 20;
    config.virtual_num = 0;

    IP_ADDR4(&config.ip[0], 192, 168, 2, 32);
    IP_ADDR4(&config.netmask[0], 255, 255, 255, 0);
    IP_ADDR4(&config.gw[0], 192, 168, 2, 1);

    IP_ADDR4(&config.ip[1], 192, 168, 2, 33);
    IP_ADDR4(&config.netmask[1], 255, 255, 255, 0);
    IP_ADDR4(&config.gw[1], 192, 168, 2, 1);

    eth_device_init(ETH_DEVICE_NAME, &config);
    dhcpd_start(ETH_DEVICE_NAME);
    ppp_sample_start();
    cmd_lwip_nat();
    while (1) {
        HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
        rt_thread_mdelay(1000);
        HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
        rt_thread_mdelay(1000);
    }
}
