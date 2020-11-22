/*
*********************************************************************************************************
*	                                  
*	ģ������ : GUI����������
*	�ļ����� : MainTask.c
*	��    �� : V1.0
*	˵    �� : OLCD����
*              
*	�޸ļ�¼ :
*		�汾��   ����         ����          ˵��
*		V1.0    2020-11-15   Eric2013  	    �װ�    
*                                     
*	Copyright (C), 2020-2030, ���������� www.armfly.com
*
*********************************************************************************************************
*/
#include "MainTask.h"

/*
*********************************************************************************************************
*                                             �궨��
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                               ����
*********************************************************************************************************
*/
GX_WINDOW *pScreen;
GX_WINDOW_ROOT *root;
extern ULONG display_1_canvas_memory[512];

uint8_t oled_canvas_memory[2048];
extern void lcd_address(uint8_t page, uint8_t column);
extern void transfer_data(uint8_t *data, uint8_t len);

/**
 *@brief �򻺳�������
 *@param  x,y ����(x = 0-127 | y = 0-127)
 *@param  color ��ɫ <Ŀǰֻ�к�:CBLACK ��:CWHITLE>
 *@retval None
 */
void GuiDrawPoint(uint8_t x, uint8_t y, uint8_t color)
{
    uint16_t i = y % 8;
    uint16_t page = y / 8;
    /* ����û����� */
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
*	�� �� ��: stm32_monochrome_buffer_toggle
*	����˵��: ��ɫ�����ƣ�ֱ�������������ػ�
*	��    ��: 
*	�� �� ֵ: ��
*********************************************************************************************************
*/
static void stm32_monochrome_buffer_toggle(GX_CANVAS *canvas, GX_RECTANGLE *dirty)
{
    uint8_t *p;

    /* ��ֹ���� */
    (void)canvas;
    (void)dirty;

    /* ���OLED�����ĵ�ַ */
    p = (uint8_t *)display_1_canvas_memory;

    /* �����������ݻ��Ƶ�OLED�Դ� */

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

    /* ��OLED�Դ������ʵ�ʻ��Ƶ�OLED */

    for (int i = 0; i < 16; i++)
    {
        lcd_address(i + 1, 1);
        transfer_data(&oled_canvas_memory[i * 128], 128);
    }
}

/*
*********************************************************************************************************
*	�� �� ��: stm32_graphics_driver_setup_monochrome
*	����˵��: ��ɫ�������ӿ�
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
UINT stm32_graphics_driver_setup_monochrome(GX_DISPLAY *display)
{
    _gx_display_driver_monochrome_setup(display, (VOID *)display_1_canvas_memory, stm32_monochrome_buffer_toggle);

    return (GX_SUCCESS);
}

/*
*********************************************************************************************************
*	�� �� ��: MainTask
*	����˵��: GUI������
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void MainTask(void)
{
    extern void lcd_init(void);

    lcd_init();

    /*��ʼ������ */
    gx_system_initialize();

    /* ������ʾ�� */
    gx_studio_display_configure(DISPLAY_1, stm32_graphics_driver_setup_monochrome,
                                LANGUAGE_ENGLISH, DISPLAY_1_THEME_2, &root);
    /* �������� */
    gx_studio_named_widget_create("window", (GX_WIDGET *)root, (GX_WIDGET **)&pScreen);

    /* ��ʾ������ */
    gx_widget_show(root);

    /* ����GUIX */
    gx_system_start();

    while (1)
    {
        rt_thread_mdelay(20);
    }
}

/* ���ô˺����ɸ������� */
void gui_theme_switching(void)
{
    static USHORT theme_id = DISPLAY_1_THEME_2;

    if (++theme_id >= DISPLAY_1_THEME_TABLE_SIZE)
        theme_id = 0;

    gx_studio_display_configure(DISPLAY_1, stm32_graphics_driver_setup_monochrome,
                                LANGUAGE_ENGLISH, theme_id, &root);
}
MSH_CMD_EXPORT(gui_theme_switching, gui_theme_switching);
/***************************** ���������� www.armfly.com (END OF FILE) *********************************/
