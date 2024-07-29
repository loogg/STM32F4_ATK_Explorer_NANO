#include <rtthread.h>
#include <board.h>
#ifdef RT_USING_FINSH
#include "chry_ringbuffer.h"
#endif /* RT_USING_FINSH */

#ifdef RT_USING_CONSOLE

ALIGN(RT_ALIGN_SIZE)
#ifndef RT_USING_DEVICE
#ifdef RT_USING_FINSH
static struct rt_semaphore _rx_sem;
#endif /* RT_USING_FINSH */
#else
static struct rt_device _console_dev = {0};
#endif /* RT_USING_DEVICE */

#ifdef RT_USING_FINSH
static chry_ringbuffer_t _rx_rb = {0};
static uint8_t _rx_rb_pool[64];
#endif /* RT_USING_FINSH */

UART_HandleTypeDef huart1;

static void console_hw_putc(char c)
{
    __HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_TC);
    huart1.Instance->DR = (uint8_t)c;
    while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC) == RESET)
        ;
}

#ifdef RT_USING_DEVICE

static rt_err_t _console_dev_open(rt_device_t dev, rt_uint16_t oflag)
{
    rt_uint16_t stream_flag = 0;

    /* keep steam flag */
    if ((oflag & RT_DEVICE_FLAG_STREAM) || (dev->open_flag & RT_DEVICE_FLAG_STREAM))
        stream_flag = RT_DEVICE_FLAG_STREAM;

    /* get open flags */
    dev->open_flag = (oflag & RT_DEVICE_OFLAG_MASK);

    /* set stream flag */
    dev->open_flag |= stream_flag;

    return RT_EOK;
}

static rt_size_t _console_dev_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    if (size == 0)
        return 0;

#ifdef RT_USING_FINSH
    return chry_ringbuffer_read(&_rx_rb, buffer, size);
#else
    return 0;
#endif /* RT_USING_FINSH */
}

static rt_size_t _console_dev_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    if (size == 0)
        return 0;

    for (int i = 0; i < size; i++)
    {
        char ch = ((char *)buffer)[i];
        if (ch == '\n')
        {
            if (dev->open_flag & RT_DEVICE_FLAG_STREAM)
            {
                console_hw_putc('\r');
            }
        }
        console_hw_putc(ch);
    }

    return size;
}

static rt_err_t _console_dev_control(rt_device_t dev, int cmd, void *args)
{
    rt_err_t result = RT_EOK;

    return result;
}

#ifdef RT_USING_DEVICE_OPS

const static struct rt_device_ops _console_ops =
    {
        RT_NULL,
        _console_dev_open,
        RT_NULL,
        _console_dev_read,
        _console_dev_write,
        _console_dev_control};
#endif

#else

void rt_hw_console_output(const char *str)
{
    rt_size_t i = 0, size = 0;

    size = rt_strlen(str);
    for (i = 0; i < size; i++)
    {
        if (*(str + i) == '\n')
        {
            console_hw_putc('\r');
        }
        console_hw_putc(*(str + i));
    }
}

#ifdef RT_USING_FINSH
char rt_hw_console_getchar(void)
{
    char ch = 0;

    rt_sem_control(&_rx_sem, RT_IPC_CMD_RESET, RT_NULL);
    while (!chry_ringbuffer_read_byte(&_rx_rb, (uint8_t *)&ch))
    {
        rt_sem_take(&_rx_sem, RT_WAITING_FOREVER);
    }
    return ch;
}
#endif /* RT_USING_FINSH */

#endif /* RT_USING_DEVICE */

static int console_board_init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }

#ifdef RT_USING_DEVICE
    _console_dev.type = RT_Device_Class_Char;
    _console_dev.rx_indicate = RT_NULL;
    _console_dev.tx_complete = RT_NULL;

#ifdef RT_USING_DEVICE_OPS
    _console_dev.ops = &_console_ops;
#else
    _console_dev.init = RT_NULL;
    _console_dev.open = _console_dev_open;
    _console_dev.close = RT_NULL;
    _console_dev.read = _console_dev_read;
    _console_dev.write = _console_dev_write;
    _console_dev.control = _console_dev_control;
#endif
    _console_dev.user_data = RT_NULL;

    /* register a character device */
    rt_device_register(&_console_dev, RT_CONSOLE_DEVICE_NAME, RT_DEVICE_FLAG_RDWR);

    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif /* RT_USING_DEVICE */

    return RT_EOK;
}
INIT_BOARD_EXPORT(console_board_init);

#ifdef RT_USING_FINSH

void USART1_IRQHandler(void)
{
    if ((__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE) != RESET) && (__HAL_UART_GET_IT_SOURCE(&huart1, UART_IT_RXNE) != RESET))
    {
        uint8_t ch = (uint8_t)(huart1.Instance->DR & 0x00FF);
        chry_ringbuffer_write_byte(&_rx_rb, ch);
#ifndef RT_USING_DEVICE
        rt_sem_release(&_rx_sem);
#else
        if (_console_dev.rx_indicate)
        {
            _console_dev.rx_indicate(&_console_dev, chry_ringbuffer_get_used(&_rx_rb));
        }
#endif /* RT_USING_DEVICE */

        __HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_RXNE);
    }
    else
    {
        if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_ORE) != RESET)
        {
            __HAL_UART_CLEAR_OREFLAG(&huart1);
        }
        if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_NE) != RESET)
        {
            __HAL_UART_CLEAR_NEFLAG(&huart1);
        }
        if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_FE) != RESET)
        {
            __HAL_UART_CLEAR_FEFLAG(&huart1);
        }
        if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_PE) != RESET)
        {
            __HAL_UART_CLEAR_PEFLAG(&huart1);
        }

        if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_LBD) != RESET)
        {
            __HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_LBD);
        }
        if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_CTS) != RESET)
        {
            __HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_CTS);
        }
    }
}

static int console_shell_init(void)
{
#ifndef RT_USING_DEVICE
    rt_sem_init(&_rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);
#endif

    if (chry_ringbuffer_init(&_rx_rb, _rx_rb_pool, sizeof(_rx_rb_pool)) < 0)
        return -RT_ERROR;

    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);

    return RT_EOK;
}
INIT_PREV_EXPORT(console_shell_init);

#endif /* RT_USING_FINSH */

#endif /* RT_USING_CONSOLE */
