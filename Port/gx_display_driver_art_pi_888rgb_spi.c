/* 
*	模块名称 : art-pi rgb888显示驱动接口
*	文件名称 : gx_display_driver_art_pi_888rgb_spi.c
*	版    本 : V1.0
*	说    明 : 
*              
*	修改记录 :
*		版本号   日期         作者          说明
*       V1.0    2020-12-01   HelloByeAll    首版           
*        
*
*/

#include <rtdevice.h>
#include "lcd_spi_port.h"
#define DBG_LEVEL DBG_LOG
#include "rtdbg.h"

#include "gx_api.h"
#include "gx_system.h"
#include "gx_display.h"
#include "gx_utility.h"
#include "guiapp_resources.h"
#include "guiapp_specifications.h"

struct drv_lcd_device *lcd;
rt_uint8_t *Canvas_Memory;
/*
*********************************************************************************************************
*	函 数 名: gx_initconfig
*	功能说明: GUIX
*	形    参: 无   	
*	返 回 值: 无
*********************************************************************************************************
*/
void gx_initconfig(void)
{
    extern GX_STUDIO_DISPLAY_INFO guiapp_display_table[1];

    Canvas_Memory = rt_malloc(LCD_WIDTH * LCD_HEIGHT * 4);

    lcd = (struct drv_lcd_device *)rt_device_find("lcd");

    guiapp_display_table[0].canvas_memory = (GX_COLOR *)Canvas_Memory;
}

static int rgbx8888_to_rgb888(void *psrc, struct rt_device_rect_info *rect_info, void *pdst)
{
    int srclinesize = LCD_WIDTH * 4;
    int dstlinesize = LCD_WIDTH * LCD_BYTES_PER_PIXEL;
    const rt_uint8_t *psrcline;
    const rt_uint8_t *psrcdot;
    rt_uint8_t *pdstline;
    rt_uint8_t *pdstdot;

    int i, j;

    if (!psrc || !pdst || rect_info->width <= 0 || rect_info->height <= 0)
    {
        rt_kprintf("rgbx8888_to_rgb888 : parameter error\n");
        return -1;
    }

    psrcline = (rt_uint8_t *)psrc;
    pdstline = (rt_uint8_t *)pdst;

    for (i = 0; i < rect_info->height; i++)
    {
        psrcdot = psrcline;
        pdstdot = pdstline;

        for (j = 0; j < rect_info->width; j++)
        {
            *pdstdot++ = psrcdot[2]; //r
            *pdstdot++ = psrcdot[1]; //g
            *pdstdot++ = psrcdot[0]; //b
            psrcdot += 4;
        }

        psrcline += srclinesize;
        pdstline += dstlinesize;
    }

    return 0;
}

/*
*********************************************************************************************************
*	函 数 名: stm32h7_565rgb_buffer_toggle
*	功能说明: 更新canvas内容到LCD显存
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void art_pi_888rgb_spi_buffer_toggle(GX_CANVAS *canvas, GX_RECTANGLE *dirty)
{
    GX_RECTANGLE Limit;
    GX_RECTANGLE Copy;
    ULONG offset;
    INT copy_width;
    INT copy_height;
    INT row;
    INT src_stride_ulongs;
    INT dest_stride_ulongs;

    ULONG *get = 0;
    ULONG *put = 0;

    struct rt_device_rect_info rect_info;

    gx_utility_rectangle_define(&Limit, 0, 0,
                                canvas->gx_canvas_x_resolution - 1,
                                canvas->gx_canvas_y_resolution - 1);

    if (gx_utility_rectangle_overlap_detect(&Limit, &canvas->gx_canvas_dirty_area, &Copy))
    {
        Copy.gx_rectangle_left &= 0xfffe;
        Copy.gx_rectangle_right |= 0x01;
        copy_width = Copy.gx_rectangle_right - Copy.gx_rectangle_left + 1;
        copy_height = Copy.gx_rectangle_bottom - Copy.gx_rectangle_top + 1;

        /* 从canvas读取更新区 */
        offset = Copy.gx_rectangle_top * canvas->gx_canvas_x_resolution;
        offset += Copy.gx_rectangle_left;
        offset *= 4;
        get = canvas->gx_canvas_memory;
        get = (ULONG *)((UCHAR *)get + offset);

        /* 从LCD显存读取要更新的区域，将canvas更新的数据复制进来 */
        offset = (canvas->gx_canvas_display_offset_y + Copy.gx_rectangle_top) * LCD_WIDTH;
        offset += canvas->gx_canvas_display_offset_x + Copy.gx_rectangle_left;
        offset *= 3;
        put = (ULONG *)(lcd->lcd_info.framebuffer);
        put = (ULONG *)((UCHAR *)put + offset);

        rect_info.x = dirty->gx_rectangle_left;
        rect_info.y = dirty->gx_rectangle_top;
        rect_info.width = dirty->gx_rectangle_right - dirty->gx_rectangle_left + 1;
        rect_info.height = dirty->gx_rectangle_bottom - dirty->gx_rectangle_top + 1;

        rgbx8888_to_rgb888(get, &rect_info, put);
        lcd->parent.control(&lcd->parent, RTGRAPHIC_CTRL_RECT_UPDATE, &rect_info);
    }
}
