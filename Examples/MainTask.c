/*
*********************************************************************************************************
*	                                  
*	模块名称 : GUI界面主函数
*	文件名称 : MainTask.c
*	版    本 : V1.0
*	说    明 : OLCD界面
*              
*	修改记录 :
*		版本号   日期         作者          说明
*		V1.0    2020-11-15   Eric2013  	    首版    
*                                     
*	Copyright (C), 2020-2030, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/
#include "MainTask.h"

/*
*********************************************************************************************************
*                                             宏定义
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                               变量
*********************************************************************************************************
*/
GX_WINDOW *pScreen;
GX_WINDOW_ROOT *root;
extern ULONG display_1_canvas_memory[512];

uint8_t oled_canvas_memory[2048];
extern void lcd_address(uint8_t page, uint8_t column);
extern void transfer_data(uint8_t *data, uint8_t len);

/**
 *@brief 向缓冲区画点
 *@param  x,y 坐标(x = 0-127 | y = 0-127)
 *@param  color 颜色 <目前只有黑:CBLACK 白:CWHITLE>
 *@retval None
 */
void GuiDrawPoint(uint8_t x, uint8_t y, uint8_t color)
{
    uint16_t i = y % 8;
    uint16_t page = y / 8;
    /* 添加用户代码 */
    if (color == 1)
        oled_canvas_memory[page * 128 + x] &= ~(color << i);
    else
    {
        color = 1;
        oled_canvas_memory[page * 128 + x] |= color << i;
    }
}

/*
*********************************************************************************************************
*	函 数 名: stm32_monochrome_buffer_toggle
*	功能说明: 单色屏绘制，直接做整个屏的重绘
*	形    参: 
*	返 回 值: 无
*********************************************************************************************************
*/
static void stm32_monochrome_buffer_toggle(GX_CANVAS *canvas, GX_RECTANGLE *dirty)
{
    uint8_t *p;

    /* 防止警告 */
    (void)canvas;
    (void)dirty;

    /* 获得OLED画布的地址 */
    p = (uint8_t *)display_1_canvas_memory;

    /* 将画布的内容绘制到OLED显存 */

    for (int y = 0; y < 128; y++)
    {
        for (int x = 0; x < 128; x += 8)
        {
            GuiDrawPoint(x, y, (p[16 * y + x / 8] & (0x80)) >> 7);
            GuiDrawPoint(x + 1, y, (p[16 * y + x / 8] & (0x40)) >> 6);
            GuiDrawPoint(x + 2, y, (p[16 * y + x / 8] & (0x20)) >> 5);
            GuiDrawPoint(x + 3, y, (p[16 * y + x / 8] & (0x10)) >> 4);
            GuiDrawPoint(x + 4, y, (p[16 * y + x / 8] & (0x08)) >> 3);
            GuiDrawPoint(x + 5, y, (p[16 * y + x / 8] & (0x04)) >> 2);
            GuiDrawPoint(x + 6, y, (p[16 * y + x / 8] & (0x02)) >> 1);
            GuiDrawPoint(x + 7, y, (p[16 * y + x / 8] & (0x01)) >> 0);
        }
    }

    /* 将OLED显存的内容实际绘制到OLED */

    for (int i = 0; i < 16; i++)
    {
        lcd_address(i + 1, 1);
        transfer_data(&oled_canvas_memory[i * 128], 128);
    }
}

/*
*********************************************************************************************************
*	函 数 名: stm32_graphics_driver_setup_monochrome
*	功能说明: 单色屏驱动接口
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
UINT stm32_graphics_driver_setup_monochrome(GX_DISPLAY *display)
{
    _gx_display_driver_monochrome_setup(display, (VOID *)display_1_canvas_memory, stm32_monochrome_buffer_toggle);

    return (GX_SUCCESS);
}

/*
*********************************************************************************************************
*	函 数 名: MainTask
*	功能说明: GUI主函数
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void MainTask(void)
{
    extern void lcd_init(void);

    lcd_init();

    /*初始化配置 */
    gx_system_initialize();

    /* 配置显示屏 */
    gx_studio_display_configure(DISPLAY_1, stm32_graphics_driver_setup_monochrome,
                                LANGUAGE_ENGLISH, DISPLAY_1_THEME_2, &root);
    /* 创建窗口 */
    gx_studio_named_widget_create("window", (GX_WIDGET *)root, (GX_WIDGET **)&pScreen);

    /* 显示根窗口 */
    gx_widget_show(root);

    /* 启动GUIX */
    gx_system_start();

    while (1)
    {
        rt_thread_mdelay(20);
    }
}

/* 调用此函数可更换主题 */
void gui_theme_switching(void)
{
    static USHORT theme_id = DISPLAY_1_THEME_2;

    if (++theme_id >= DISPLAY_1_THEME_TABLE_SIZE)
        theme_id = 0;

    gx_studio_display_configure(DISPLAY_1, stm32_graphics_driver_setup_monochrome,
                                LANGUAGE_ENGLISH, theme_id, &root);
}
MSH_CMD_EXPORT(gui_theme_switching, gui_theme_switching);
/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
