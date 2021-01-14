# AzureGUIX

2021-01-14
增加ART-PI SPI屏驱动


调用MainTask.c中的MainTask();即可完成初始化

开发者使用时需自行更改Examples中MainTask.c中的下列参数

```
extern ULONG display_1_canvas_memory[512]; 
此处数组大小应与Studio生成的大小相同

uint8_t oled_canvas_memory[2048];
此处大小与上面数组大小相同  目的是为了存储转换扫描方式后的数据
实际所用屏幕可能和Studio中的界面扫描方式不同 （Studio中界面扫描方式为 从左到右，从上到下）


自行实现oled打点函数
void GuiDrawPoint(uint8_t x, uint8_t y, uint8_t color)
{
    .....
}

在外部文件中自行实现下列函数  
extern void lcd_address(uint8_t page, uint8_t column);
extern void transfer_data(uint8_t *data, uint8_t len);

```



使用GUIX Studio可打开GUIX Studio OLED Demo中的工程
GUIX Studio生成的文件需要替换Example文件夹中的同名文件才能生效



##### 虚拟设备 GUIX_VIRTUAL_DISPLAY_MONO

**虚拟设备目前仅为128*128的单色设备**

虚拟设备使用串口发送至上位机，开发者需要根据自身硬件选择波特率，若不使用DMA建议最大波特率设置成460800！ 并用中断接收方式打开设备

```
rt_device_open(uart_device, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_INT_TX);
```



```
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
```

![Virtual Display](./Virtual Display\Virtual Display.jpg)

1.上位机使用鼠标模拟按键操作 右上角显示为当前坐标和触摸状态。

2.上位机右上方三个按键已绑定数字键盘 ”1",  2“,  "3",已实现长按短按。

用户在

```
guix_virtual_display_mono.c 文件中加入按键回调即可
```



```
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

```