/* 
*	模块名称 : GUI单色屏虚拟设备
*	文件名称 : guix_virtual_display_mono.c
*	版    本 : V1.0
*	说    明 : 
*              
*	修改记录 :
*		版本号   日期         作者          说明
*       V1.0    2020-11-26   HelloByeAll    首版           
*        
*
*/

#include <rtthread.h>
#include <board.h>
#include <gx_user.h>

#ifdef GUIX_VIRTUAL_DISPLAY_MONO

#ifndef GUIX_VIRTUAL_DISPLAY_SERIAL_DEV
#define GUIX_VIRTUAL_DISPLAY_SERIAL_DEV "uart2"
#endif // !GUIX_VIRTUAL_DISPLAY_SERIAL_DEV

    static rt_sem_t serial_sem;
static rt_device_t uart_device;

typedef struct
{
    rt_uint8_t addr[2];
    rt_uint16_t len;
} virtual_display_t;

static virtual_display_t virtual_serial = {0xBB, 0xCC, 2048};
void show_virtual_display(rt_uint8_t *p)
{
    rt_device_write(uart_device, 0, &virtual_serial, 4);
    rt_device_write(uart_device, 0, p, 2048);
}

static void key_cb(rt_uint8_t key, rt_uint8_t state)
{
    switch (key)
    {
    case 1:
        if (state)
        {
            /* 按键1长按 */
        }
        else
        {
            /* 按键1短按 */
        }
        break;

    case 2:
        if (state)
        {
            /* 按键2长按 */
        }
        else
        {
            /* 按键2短按 */
        }
        break;

    case 3:
        if (state)
        {
            /* 按键3长按 */
        }
        else
        {
            /* 按键3短按 */
        }
        break;
    }
}

static void send_coordinate_to_guix(rt_uint8_t x, rt_uint8_t y, rt_uint8_t state)
{
    extern ULONG display_1_canvas_memory[512];
    static rt_uint8_t x1, y1, state1;

    GX_EVENT event;
    event.gx_event_type = 0;

    if (x == 0xEE)
    {
        key_cb(y, state);
        return;
    }

    if (x1 != x || y1 != y)
    {
        x1 = x;
        y1 = y;

        if (state1 == state && state == 1)
        {
            event.gx_event_type = GX_EVENT_PEN_DRAG;
        }
    }

    if (state1 != state)
    {
        state1 = state;
        if (state == 1)
            event.gx_event_type = GX_EVENT_PEN_DOWN;
        else
            event.gx_event_type = GX_EVENT_PEN_UP;
    }

    if (event.gx_event_type != 0)
    {
        event.gx_event_payload.gx_event_pointdata.gx_point_x = x1;
        event.gx_event_payload.gx_event_pointdata.gx_point_y = y1;
        event.gx_event_sender = 0;
        event.gx_event_target = 0;
        event.gx_event_display_handle = (ULONG *)display_1_canvas_memory;
        gx_system_event_send(&event);
    }

    // rt_kprintf("x = %d, y = %d  state = %d\r\n", x, y, state);
}

static void check_data(rt_uint8_t *data)
{
    rt_uint8_t sum = 0;
    for (size_t i = 0; i < 6; i++)
    {
        sum += data[i];
    }
    if (data[6] == sum)
    {
        send_coordinate_to_guix(data[3], data[4], data[5]);
    }
}

static void receive_data_prepare(rt_uint8_t ch)
{
    static rt_uint8_t len;
    static rt_uint8_t date[10];
    date[len] = ch;
    if (len == 0 && ch == 0xBB)
        len++;

    else if (len == 1 && ch == 0xCC)
        len++;

    else if (len == 2 && ch == 0x02)
        len++;

    else if (len >= 3 && len <= 5)
        len++;

    else if (len == 6)
    {
        check_data(date);
        len = 0;
    }
    else
        len = 0;
}

static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(serial_sem);

    return RT_EOK;
}

static void virtual_serial_thread_entry(void *par)
{
    char ch;

    while (1)
    {
        while (rt_device_read(uart_device, -1, &ch, 1) != 1)
        {
            rt_sem_take(serial_sem, RT_WAITING_FOREVER);
        }
        receive_data_prepare((rt_uint8_t)ch);
    }
}

static void rt_virtual_serial_init(char *name)
{
    serial_sem = rt_sem_create("mono_sem", 0, RT_IPC_FLAG_FIFO);

    uart_device = rt_device_find(name);

    struct serial_configure use_config = {
        BAUD_RATE_921600,   /* 921600 bits/s */
        DATA_BITS_8,        /* 8 databits */
        STOP_BITS_1,        /* 1 stopbit */
        PARITY_NONE,        /* No parity  */
        BIT_ORDER_LSB,      /* LSB first sent */
        NRZ_NORMAL,         /* Normal mode */
        RT_SERIAL_RB_BUFSZ, /* Buffer size */
        0};

    rt_device_open(uart_device, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_DMA_RX | RT_DEVICE_FLAG_DMA_TX);

    if (RT_EOK != rt_device_control(uart_device, RT_DEVICE_CTRL_CONFIG, (void *)&use_config))
    {
        rt_kprintf("uart config baud rate failed.\n");
    }

    rt_device_set_rx_indicate(uart_device, uart_input);

    return;
}

static void virtual_display_init(void)
{
    rt_virtual_serial_init(GUIX_VIRTUAL_DISPLAY_SERIAL_DEV);

    rt_thread_t thread = RT_NULL;
    /* Create background ticks thread */
    thread = rt_thread_create("virtual", virtual_serial_thread_entry, RT_NULL, 512, 15, 10);
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
    }
}
INIT_APP_EXPORT(virtual_display_init);

#endif // GUIX_VIRTUAL_DISPLAY_MONO

/*************************************** (END OF FILE) ****************************************/

