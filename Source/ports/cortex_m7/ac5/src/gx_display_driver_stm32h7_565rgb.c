/*
*********************************************************************************************************
*	                                  
*	ģ������ : ��ʾ����
*	�ļ����� : gx_display_driver_stm32h7_565rgb.c
*	��    �� : V1.0
*	˵    �� : �ײ������ӿ��ļ�
*              
*	�޸ļ�¼ :
*		�汾��    ����         ����          ˵��
*		V1.0    2020-07-05   Eric2013  	    �װ�    
*                                     
*	Copyright (C), 2020-2030, ���������� www.armfly.com
*
*********************************************************************************************************
*/
#include   "tx_api.h"
#include   "gx_api.h"
#include   "gx_system.h"
#include   "gx_display.h"
#include   "gx_utility.h"
#include   "gx_display_driver_stm32h7_565rgb.h"
#include   "bsp.h"



/*
*********************************************************************************************************
*                                           �����ͺ���
*********************************************************************************************************
*/
#define GX_CHROMEART_ENABLE           /* ����ʹ��DMA2D���� */
#define FrameBufer   SDRAM_LCD_BUF1   /* �Դ��ַ */       
 
static LTDC_HandleTypeDef  hLTDC; 
static void LCD_LL_Init(void);


/*
*********************************************************************************************************
*	�� �� ��: gx_chromeart_horizontal_line_draw
*	����˵��: ��ɫ�����
*	��    ��: ---
*	�� �� ֵ: ��
*********************************************************************************************************
*/
static VOID gx_chromeart_horizontal_line_draw(GX_DRAW_CONTEXT *context, INT xstart, INT xend, INT ypos, INT width, GX_COLOR color)
{
    uint32_t  put;
    int length;
    GX_CANVAS *canvas = context->gx_draw_context_canvas;

    put = (uint32_t) canvas->gx_canvas_memory;
    put += (canvas->gx_canvas_x_resolution * 2 * ypos) + (xstart * 2);

    length = xend - xstart + 1;


	DMA2D->CR = DMA2D_R2M;
	DMA2D->OCOLR = color;

	/* ����� */
	DMA2D->OMAR = (uint32_t)put;
	DMA2D->OOR = canvas->gx_canvas_x_resolution - length;
	DMA2D->OPFCCR = LTDC_PIXEL_FORMAT_RGB565;

	DMA2D->NLR = (uint32_t)(length << 16) | (uint16_t)width;
	SCB_CleanInvalidateDCache();
	DMA2D->CR |= DMA2D_CR_START;
	while (DMA2D->CR & DMA2D_CR_START) {}
}

/*
*********************************************************************************************************
*	�� �� ��: gx_chromeart_vertical_line_draw
*	����˵��: ��ɫ�����
*	��    ��: ---
*	�� �� ֵ: ��
*********************************************************************************************************
*/
static VOID gx_chromeart_vertical_line_draw(GX_DRAW_CONTEXT *context, INT ystart, INT yend, INT xpos, INT width, GX_COLOR color)
{
    uint32_t  put;
    GX_CANVAS *canvas = context->gx_draw_context_canvas;
  
    put = (uint32_t) canvas->gx_canvas_memory;

    put += (canvas->gx_canvas_x_resolution * 2 * ystart) + (xpos * 2);

	DMA2D->CR = DMA2D_R2M;
	DMA2D->OCOLR = color;

	/* ����� */
	DMA2D->OMAR = (uint32_t)put;
	DMA2D->OOR = canvas->gx_canvas_x_resolution - width;
	DMA2D->OPFCCR = LTDC_PIXEL_FORMAT_RGB565;

	DMA2D->NLR = (uint32_t)(width << 16) | (uint16_t)(yend - ystart + 1);
	SCB_CleanInvalidateDCache();
	DMA2D->CR |= DMA2D_CR_START;
	while (DMA2D->CR & DMA2D_CR_START) {}
}

/*
*********************************************************************************************************
*	�� �� ��: gx_chromeart_canvas_copy
*	����˵��: canvas���Ƽ���
*	��    ��: ---
*	�� �� ֵ: ��
*********************************************************************************************************
*/
static void gx_chromeart_canvas_copy(GX_CANVAS *canvas, GX_CANVAS *composite)
{
	GX_RECTANGLE dirty;
	GX_RECTANGLE overlap;
	USHORT      *read;
	USHORT      *write;
	INT          width;

	
    dirty.gx_rectangle_left = dirty.gx_rectangle_top = 0;
    dirty.gx_rectangle_right = canvas -> gx_canvas_x_resolution - 1;
    dirty.gx_rectangle_bottom = canvas -> gx_canvas_y_resolution - 1;

    _gx_utility_rectangle_shift(&dirty, canvas -> gx_canvas_display_offset_x, canvas -> gx_canvas_display_offset_y);

    if (_gx_utility_rectangle_overlap_detect(&dirty, &composite -> gx_canvas_dirty_area, &overlap))
    {
        width = overlap.gx_rectangle_right - overlap.gx_rectangle_left + 1;
        read = (USHORT *)canvas -> gx_canvas_memory;

        read += (overlap.gx_rectangle_top - dirty.gx_rectangle_top) * canvas -> gx_canvas_x_resolution;
        read += overlap.gx_rectangle_left - dirty.gx_rectangle_left;

        write = (USHORT *)composite -> gx_canvas_memory;
        write += overlap.gx_rectangle_top * composite -> gx_canvas_x_resolution;
        write += overlap.gx_rectangle_left;

		/* ǰ���� */
		DMA2D->CR = DMA2D_M2M;
		DMA2D->FGMAR = (uint32_t) read;             
		DMA2D->FGOR = canvas->gx_canvas_x_resolution - width; 
		DMA2D->FGPFCCR = (0xff000000) | LTDC_PIXEL_FORMAT_RGB565; 
	
		/* ����� */
		DMA2D->OMAR = (uint32_t)write;
		DMA2D->OOR = g_LcdWidth - width;
		DMA2D->OPFCCR = LTDC_PIXEL_FORMAT_RGB565;
		
		DMA2D->NLR = (uint32_t)(width << 16) | (uint16_t)(overlap.gx_rectangle_bottom - overlap.gx_rectangle_top + 1);
		SCB_CleanInvalidateDCache();
		DMA2D->CR |= DMA2D_CR_START;
		while (DMA2D->CR & DMA2D_CR_START) {}
    }
}

/*
*********************************************************************************************************
*	�� �� ��: gx_chromeart_pixelmap_draw
*	����˵��: pixelmap ���Ƽ��� 
*	��    ��: --
*	�� �� ֵ: ��
*********************************************************************************************************
*/
static void gx_chromeart_pixelmap_draw(GX_DRAW_CONTEXT *context,
                                INT xpos, INT ypos, GX_PIXELMAP *pixelmap)
{
	INT           width;
	INT           height;
	USHORT       *putrow;
	USHORT       *getshort;
	ULONG        *getlong;
	GX_CANVAS    *canvas;
	ULONG         reg_val;
	uint32_t      DMA2D_Mode;


    /* ���pixelmap��ѹ���ģ����������ʽ���� */
    if (pixelmap->gx_pixelmap_flags & GX_PIXELMAP_COMPRESSED)
    {
        _gx_display_driver_565rgb_pixelmap_draw(context, xpos, ypos, pixelmap);
        return;
    }

    /* ����Ǵ�alphaͨ����alpha��ʽ��Ҳ��Ҫ�������ʽ���� */
    if (pixelmap->gx_pixelmap_flags & GX_PIXELMAP_ALPHA &&
        pixelmap->gx_pixelmap_format == GX_COLOR_FORMAT_565RGB)
    {
        _gx_display_driver_565rgb_pixelmap_draw(context, xpos, ypos, pixelmap);
        return;
    }

    /* �����pixelmapԭʼ���ݣ�ʹ��DMA2D���� */
    GX_RECTANGLE *clip = context -> gx_draw_context_clip;
    canvas = context->gx_draw_context_canvas;

    putrow = (USHORT *)context -> gx_draw_context_memory;
    putrow += clip -> gx_rectangle_top * context -> gx_draw_context_pitch;
    putrow += clip -> gx_rectangle_left;
	
	
	if (pixelmap->gx_pixelmap_format == GX_COLOR_FORMAT_565RGB)
    {
        getshort = (USHORT *)(pixelmap -> gx_pixelmap_data);
        getshort += pixelmap -> gx_pixelmap_width * (clip -> gx_rectangle_top - ypos);
        getshort += (clip -> gx_rectangle_left - xpos);
        DMA2D->FGMAR = (uint32_t) getshort;           
    }
    else
    {
        getlong = (ULONG *)(pixelmap -> gx_pixelmap_data);
        getlong += pixelmap -> gx_pixelmap_width * (clip -> gx_rectangle_top - ypos);
        getlong += (clip -> gx_rectangle_left - xpos);
        DMA2D->FGMAR = (uint32_t) getlong;             
    }

    width = clip->gx_rectangle_right - clip -> gx_rectangle_left + 1;
    height = clip->gx_rectangle_bottom - clip->gx_rectangle_top + 1;

    if (pixelmap->gx_pixelmap_format == GX_COLOR_FORMAT_565RGB)
    {
        reg_val = LTDC_PIXEL_FORMAT_RGB565;
    }
    else
    {
        reg_val = LTDC_PIXEL_FORMAT_ARGB8888;
    }
	
	/* ǰ���� */
    if (pixelmap->gx_pixelmap_flags & GX_PIXELMAP_ALPHA)
    {
        reg_val |= 0xff000000;
        DMA2D->FGPFCCR = reg_val;
        DMA2D_Mode = DMA2D_M2M_BLEND;       
    }
    else
    {
        reg_val |= 0xff010000;  
        DMA2D->FGPFCCR = reg_val;
        DMA2D_Mode = DMA2D_M2M;       
    }
	
	/* ǰ���� */
	DMA2D->CR = DMA2D_Mode;;            
	DMA2D->FGOR = pixelmap->gx_pixelmap_width - width;  

	/* ������ */
	if (pixelmap->gx_pixelmap_flags & GX_PIXELMAP_ALPHA)
	{
		DMA2D->BGMAR = (uint32_t) putrow;       
		DMA2D->BGOR = canvas->gx_canvas_x_resolution - width; 
		DMA2D->BGPFCCR = 0xff010002;
	}


	/* ����� */
	DMA2D->OMAR = (uint32_t)putrow;
	DMA2D->OOR = canvas->gx_canvas_x_resolution - width;
	DMA2D->OPFCCR = LTDC_PIXEL_FORMAT_RGB565;

	DMA2D->NLR = (uint32_t)(width << 16) | (uint16_t)height;
	SCB_CleanInvalidateDCache();
	DMA2D->CR |= DMA2D_CR_START;
	while (DMA2D->CR & DMA2D_CR_START) {}
}

/*
*********************************************************************************************************
*	�� �� ��: gx_chromeart_pixelmap_blend
*	����˵��: pixelmap blend ����
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
static void gx_chromeart_pixelmap_blend(GX_DRAW_CONTEXT *context,
                                INT xpos, INT ypos, GX_PIXELMAP *pixelmap, UCHAR alpha)
{
	INT           width;
	INT           height;
	USHORT       *putrow;
	USHORT       *getshort;
	ULONG        *getlong;
	GX_CANVAS    *canvas;
	ULONG        blend_reg;

     /* ���pixelmap��ѹ���ģ����������ʽ���� */
    if (pixelmap->gx_pixelmap_flags & GX_PIXELMAP_COMPRESSED)
    {
        _gx_display_driver_565rgb_pixelmap_blend(context, xpos, ypos, pixelmap, alpha);
        return;
    }
    if (pixelmap->gx_pixelmap_flags & GX_PIXELMAP_ALPHA &&
        pixelmap->gx_pixelmap_format == GX_COLOR_FORMAT_565RGB)
    {
        _gx_display_driver_565rgb_pixelmap_blend(context, xpos, ypos, pixelmap, alpha);
        return;
    }

    /* ʹ��DMA2D���ٻ��� */
    GX_RECTANGLE *clip = context -> gx_draw_context_clip;
    canvas = context->gx_draw_context_canvas;

    putrow = (USHORT *)context -> gx_draw_context_memory;
    putrow += clip -> gx_rectangle_top * context->gx_draw_context_pitch;
    putrow += clip -> gx_rectangle_left;

    if (pixelmap->gx_pixelmap_format == GX_COLOR_FORMAT_565RGB)
    {
        getshort = (USHORT *)(pixelmap -> gx_pixelmap_data);
        getshort += pixelmap -> gx_pixelmap_width * (clip -> gx_rectangle_top - ypos);
        getshort += (clip -> gx_rectangle_left - xpos);
    }
    else
    {
        getlong = (ULONG *)(pixelmap -> gx_pixelmap_data);
        getlong += pixelmap -> gx_pixelmap_width * (clip -> gx_rectangle_top - ypos);
        getlong += (clip -> gx_rectangle_left - xpos);
    }

    width = clip->gx_rectangle_right - clip -> gx_rectangle_left + 1;
    height = clip->gx_rectangle_bottom - clip->gx_rectangle_top + 1;
	
    blend_reg = alpha;
    blend_reg <<= 24;
	
	if (pixelmap->gx_pixelmap_format == GX_COLOR_FORMAT_565RGB)
    {
        DMA2D->FGMAR = (uint32_t) getshort;             
        blend_reg |= LTDC_PIXEL_FORMAT_RGB565;
    }
    else
    {
        DMA2D->FGMAR = (uint32_t) getlong;             
        blend_reg |= LTDC_PIXEL_FORMAT_ARGB8888;
    }

    if (pixelmap->gx_pixelmap_flags & GX_PIXELMAP_ALPHA)
    {
        blend_reg |= 0x00020000;
    }
    else
    {
        blend_reg |= 0x00010000;
    }

	DMA2D->CR = 0x00020000UL | (1 << 9);

	/* ǰ���� */
	DMA2D->FGOR = pixelmap->gx_pixelmap_width - width; 
	DMA2D->FGPFCCR = blend_reg;  
	
	/* ������ */
	DMA2D->BGMAR = (uint32_t) putrow;
	DMA2D->BGOR = canvas->gx_canvas_x_resolution - width; 
	DMA2D->BGPFCCR = 0xff010002;

	/* ����� */
	DMA2D->OMAR = (uint32_t)putrow;
	DMA2D->OOR = canvas->gx_canvas_x_resolution - width;
	DMA2D->OPFCCR = LTDC_PIXEL_FORMAT_RGB565;

	DMA2D->NLR = (uint32_t)(width << 16) | (uint16_t)height;
	SCB_CleanInvalidateDCache();
	DMA2D->CR |= DMA2D_CR_START;
	while (DMA2D->CR & DMA2D_CR_START) {}
}

/*
*********************************************************************************************************
*	�� �� ��: gx_chromeart_glyph_8bit_draw
*	����˵��: glyph 8bit ����
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
static void gx_chromeart_glyph_8bit_draw(GX_DRAW_CONTEXT *context, GX_RECTANGLE *draw_area,
                                         GX_POINT *map_offset, const GX_GLYPH *glyph)
{
	GX_CANVAS  *canvas;
	GX_UBYTE   *read_glyph;
	USHORT     *write_row;
	GX_COLOR    text_color;
	UINT        height;
	UINT        width;
	UCHAR       red, green, blue;
	GX_UBYTE    alpha;

    text_color =  context -> gx_draw_context_brush.gx_brush_line_color;

    alpha = context ->gx_draw_context_brush.gx_brush_alpha;

    if (alpha == 0)
    {
        return;
    }

    red = (text_color & 0xf800) >> 8;
    green = (text_color & 0x07e0) >> 3;
    blue = (text_color & 0x001f) << 3;
    text_color = (red << 16) | (green << 8) | blue;
    width = draw_area -> gx_rectangle_right - draw_area -> gx_rectangle_left + 1;

    canvas = context->gx_draw_context_canvas;

    read_glyph = (GX_UBYTE *)glyph -> gx_glyph_map;

    if (map_offset -> gx_point_y)
    {
        read_glyph = read_glyph + (glyph -> gx_glyph_width * map_offset -> gx_point_y);
    }

    read_glyph += map_offset -> gx_point_x;

    width = draw_area->gx_rectangle_right - draw_area->gx_rectangle_left + 1;
    height = draw_area -> gx_rectangle_bottom - draw_area -> gx_rectangle_top + 1;

    write_row = (USHORT *) canvas->gx_canvas_memory;
    write_row += draw_area->gx_rectangle_top * context->gx_draw_context_pitch;
    write_row += draw_area->gx_rectangle_left;
   
	DMA2D->CR = DMA2D_M2M_BLEND;

	/* ǰ���� */
	DMA2D->FGMAR = (uint32_t)read_glyph;
	DMA2D->FGOR = glyph->gx_glyph_width - width; 
	DMA2D->FGCOLR = text_color;
	DMA2D->FGPFCCR =(alpha << 24) | 0x00020009;   /* ԭʼǰ����ͼ��� alpha ͨ��ֵ�滻Ϊ��ǰ���õ���ԭʼ alpha ͨ��ֵ�ĳ˻���AL88 */

	/* ������ */
	DMA2D->BGMAR = (uint32_t) write_row;         
	DMA2D->BGOR = canvas->gx_canvas_x_resolution - width;  
	DMA2D->BGPFCCR = 0xFF000002;	              /* ���޸ı�����ͼ��� alpha ͨ��ֵ��RGB565 */
	
	/* ����� */
	DMA2D->OMAR = (uint32_t)write_row;
	DMA2D->OOR = canvas->gx_canvas_x_resolution - width;
	DMA2D->OPFCCR = LTDC_PIXEL_FORMAT_RGB565;

	DMA2D->NLR = (uint32_t)(width << 16) | (uint16_t)height;
	SCB_CleanInvalidateDCache();
	DMA2D->CR |= DMA2D_CR_START;
	while (DMA2D->CR & DMA2D_CR_START) {}
}

/*
*********************************************************************************************************
*	�� �� ��: stm32h7_565rgb_buffer_toggle
*	����˵��: ����canvas���ݵ�LCD�Դ�
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
static void stm32h7_565rgb_buffer_toggle(GX_CANVAS *canvas, GX_RECTANGLE *dirty)
{
	GX_RECTANGLE    Limit;
	GX_RECTANGLE    Copy;
	ULONG           offset;
	INT             copy_width;
	INT             copy_height;

#if !defined(GX_CHROMEART_ENABLE)
	INT             row;
	INT             src_stride_ulongs;
	INT             dest_stride_ulongs;
#endif

	ULONG *get;
	ULONG *put;

	gx_utility_rectangle_define(&Limit, 0, 0,
						canvas->gx_canvas_x_resolution - 1,
						canvas->gx_canvas_y_resolution - 1);

	if (gx_utility_rectangle_overlap_detect(&Limit, &canvas->gx_canvas_dirty_area, &Copy))
	{
		Copy.gx_rectangle_left &= 0xfffe;
		Copy.gx_rectangle_right |= 0x01;
		copy_width = Copy.gx_rectangle_right - Copy.gx_rectangle_left + 1;
		copy_height = Copy.gx_rectangle_bottom - Copy.gx_rectangle_top + 1;

		/* ��canvas��ȡ������ */
        offset = Copy.gx_rectangle_top * canvas->gx_canvas_x_resolution;
        offset += Copy.gx_rectangle_left;
        offset /= 2; 
        get = canvas ->gx_canvas_memory;
        get += offset;

		/* ��LCD�Դ��ȡҪ���µ����򣬽�canvas���µ����ݸ��ƽ��� */
		put = (ULONG *) FrameBufer;
		offset = (canvas->gx_canvas_display_offset_y + Copy.gx_rectangle_top)* g_LcdWidth;
		offset += canvas->gx_canvas_display_offset_x + Copy.gx_rectangle_left;
		offset /= 2;
		put += offset;

#if !defined(GX_CHROMEART_ENABLE)	
		src_stride_ulongs = canvas ->gx_canvas_x_resolution / 2;
		dest_stride_ulongs = g_LcdWidth / 2;
		copy_width /= 2; 
		
		for(row = 0; row < copy_height; row++)
		{
			memcpy(put, get, copy_width * 4);
			put += dest_stride_ulongs;
			get += src_stride_ulongs;
		}
#else	
		DMA2D->CR = 0x00000000UL | (1 << 9);
		DMA2D->FGMAR = (uint32_t)get;
		DMA2D->OMAR = (uint32_t)put;
		DMA2D->FGOR = canvas->gx_canvas_x_resolution - copy_width;
		DMA2D->OOR = g_LcdWidth - copy_width;

		/* ǰ�����������򶼲���RGB565��ɫ��ʽ */
		DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_RGB565;
		DMA2D->OPFCCR = LTDC_PIXEL_FORMAT_RGB565;
		DMA2D->NLR = (uint32_t)(copy_width << 16) | (uint16_t)copy_height;
		DMA2D->CR |= DMA2D_CR_START;
		while (DMA2D->CR & DMA2D_CR_START) {}
#endif
	}
}

/*
*********************************************************************************************************
*	�� �� ��: stm32h7_graphics_driver_setup_565rgb
*	����˵��: �����ӿں���
*	��    ��: ---
*	�� �� ֵ: GX_SUCCESS
*********************************************************************************************************
*/
UINT stm32h7_graphics_driver_setup_565rgb(GX_DISPLAY *display)
{
	LCD_LL_Init();
	
    _gx_display_driver_565rgb_setup(display, (VOID*)STM32_SCREEN_HANDLE, stm32h7_565rgb_buffer_toggle);

#if defined(GX_CHROMEART_ENABLE)	
    display -> gx_display_driver_pixelmap_blend     = gx_chromeart_pixelmap_blend;
	display -> gx_display_driver_pixelmap_draw      = gx_chromeart_pixelmap_draw;
	display -> gx_display_driver_canvas_copy        = gx_chromeart_canvas_copy;

	display->gx_display_driver_horizontal_line_draw = gx_chromeart_horizontal_line_draw;
    display -> gx_display_driver_vertical_line_draw = gx_chromeart_vertical_line_draw;
	display -> gx_display_driver_8bit_glyph_draw    = gx_chromeart_glyph_8bit_draw;
#endif

    return(GX_SUCCESS);
}

/*
*********************************************************************************************************
*	�� �� ��: LCD_LL_Init
*	����˵��: ����LTDC
*	��    ��: ��
*	�� �� ֵ: ��
*   ��    ��:
*       LCD_TFT ͬ��ʱ�����ã������Թٷ�����һ����ͼ���Լ����ࣩ��
*       ----------------------------------------------------------------------------
*    
*                                                 Total Width
*                             <--------------------------------------------------->
*                       Hsync width HBP             Active Width                HFP
*                             <---><--><--------------------------------------><-->
*                         ____    ____|_______________________________________|____ 
*                             |___|   |                                       |    |
*                                     |                                       |    |
*                         __|         |                                       |    |
*            /|\    /|\  |            |                                       |    |
*             | VSYNC|   |            |                                       |    |
*             |Width\|/  |__          |                                       |    |
*             |     /|\     |         |                                       |    |
*             |  VBP |      |         |                                       |    |
*             |     \|/_____|_________|_______________________________________|    |
*             |     /|\     |         | / / / / / / / / / / / / / / / / / / / |    |
*             |      |      |         |/ / / / / / / / / / / / / / / / / / / /|    |
*    Total    |      |      |         |/ / / / / / / / / / / / / / / / / / / /|    |
*    Heigh    |      |      |         |/ / / / / / / / / / / / / / / / / / / /|    |
*             |Active|      |         |/ / / / / / / / / / / / / / / / / / / /|    |
*             |Heigh |      |         |/ / / / / / Active Display Area / / / /|    |
*             |      |      |         |/ / / / / / / / / / / / / / / / / / / /|    |
*             |      |      |         |/ / / / / / / / / / / / / / / / / / / /|    |
*             |      |      |         |/ / / / / / / / / / / / / / / / / / / /|    |
*             |      |      |         |/ / / / / / / / / / / / / / / / / / / /|    |
*             |      |      |         |/ / / / / / / / / / / / / / / / / / / /|    |
*             |     \|/_____|_________|_______________________________________|    |
*             |     /|\     |                                                      |
*             |  VFP |      |                                                      |
*            \|/    \|/_____|______________________________________________________|
*            
*     
*     ÿ��LCD�豸�����Լ���ͬ��ʱ��ֵ��
*     Horizontal Synchronization (Hsync) 
*     Horizontal Back Porch (HBP)       
*     Active Width                      
*     Horizontal Front Porch (HFP)     
*   
*     Vertical Synchronization (Vsync)  
*     Vertical Back Porch (VBP)         
*     Active Heigh                       
*     Vertical Front Porch (VFP)         
*     
*     LCD_TFT ����ˮƽ�ʹ�ֱ����ʼ�Լ�����λ�� :
*     ----------------------------------------------------------------
*   
*     HorizontalStart = (Offset_X + Hsync + HBP);
*     HorizontalStop  = (Offset_X + Hsync + HBP + Window_Width - 1); 
*     VarticalStart   = (Offset_Y + Vsync + VBP);
*     VerticalStop    = (Offset_Y + Vsync + VBP + Window_Heigh - 1);
*
*********************************************************************************************************
*/
static void LCD_LL_Init(void) 
{
	/* ����LCD��ص�GPIO */
	{
		/* GPIOs Configuration */
		/*
		+------------------------+-----------------------+----------------------------+
		+                       LCD pins assignment                                   +
		+------------------------+-----------------------+----------------------------+
		|  LCDH7_TFT R0 <-> PI.15  |  LCDH7_TFT G0 <-> PJ.07 |  LCDH7_TFT B0 <-> PJ.12      |
		|  LCDH7_TFT R1 <-> PJ.00  |  LCDH7_TFT G1 <-> PJ.08 |  LCDH7_TFT B1 <-> PJ.13      |
		|  LCDH7_TFT R2 <-> PJ.01  |  LCDH7_TFT G2 <-> PJ.09 |  LCDH7_TFT B2 <-> PJ.14      |
		|  LCDH7_TFT R3 <-> PJ.02  |  LCDH7_TFT G3 <-> PJ.10 |  LCDH7_TFT B3 <-> PJ.15      |
		|  LCDH7_TFT R4 <-> PJ.03  |  LCDH7_TFT G4 <-> PJ.11 |  LCDH7_TFT B4 <-> PK.03      |
		|  LCDH7_TFT R5 <-> PJ.04  |  LCDH7_TFT G5 <-> PK.00 |  LCDH7_TFT B5 <-> PK.04      |
		|  LCDH7_TFT R6 <-> PJ.05  |  LCDH7_TFT G6 <-> PK.01 |  LCDH7_TFT B6 <-> PK.05      |
		|  LCDH7_TFT R7 <-> PJ.06  |  LCDH7_TFT G7 <-> PK.02 |  LCDH7_TFT B7 <-> PK.06      |
		-------------------------------------------------------------------------------
		|  LCDH7_TFT HSYNC <-> PI.12  | LCDTFT VSYNC <->  PI.13 |
		|  LCDH7_TFT CLK   <-> PI.14  | LCDH7_TFT DE   <->  PK.07 |
		-----------------------------------------------------
		*/		
		GPIO_InitTypeDef GPIO_Init_Structure;

		/*##-1- Enable peripherals and GPIO Clocks #################################*/  
		/* ʹ��LTDC��DMA2Dʱ�� */
		__HAL_RCC_LTDC_CLK_ENABLE();
		__HAL_RCC_DMA2D_CLK_ENABLE();  
		
		/* ʹ��GPIOʱ�� */
		__HAL_RCC_GPIOI_CLK_ENABLE();
		__HAL_RCC_GPIOJ_CLK_ENABLE();
		__HAL_RCC_GPIOK_CLK_ENABLE();

		/* GPIOI ���� */
		GPIO_Init_Structure.Pin       = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15; 
		GPIO_Init_Structure.Mode      = GPIO_MODE_AF_PP;
		GPIO_Init_Structure.Pull      = GPIO_NOPULL;
		GPIO_Init_Structure.Speed     = GPIO_SPEED_FREQ_HIGH;
		GPIO_Init_Structure.Alternate = GPIO_AF14_LTDC;  
		HAL_GPIO_Init(GPIOI, &GPIO_Init_Structure);

		/* GPIOJ ���� */  
		GPIO_Init_Structure.Pin       = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | \
									  GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | \
									  GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | \
									  GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15; 
		GPIO_Init_Structure.Mode      = GPIO_MODE_AF_PP;
		GPIO_Init_Structure.Pull      = GPIO_NOPULL;
		GPIO_Init_Structure.Speed     = GPIO_SPEED_FREQ_HIGH;
		GPIO_Init_Structure.Alternate = GPIO_AF14_LTDC;  
		HAL_GPIO_Init(GPIOJ, &GPIO_Init_Structure);  

		/* GPIOK ���� */  
		GPIO_Init_Structure.Pin       = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | \
									    GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7; 
		GPIO_Init_Structure.Mode      = GPIO_MODE_AF_PP;
		GPIO_Init_Structure.Pull      = GPIO_NOPULL;
		GPIO_Init_Structure.Speed     = GPIO_SPEED_FREQ_HIGH;
		GPIO_Init_Structure.Alternate = GPIO_AF14_LTDC;  
		HAL_GPIO_Init(GPIOK, &GPIO_Init_Structure);  	
	}
	
	/*##-2- LTDC��ʼ�� #############################################################*/  
	{	
		uint16_t Width, Height, HSYNC_W, HBP, HFP, VSYNC_W, VBP, VFP;
		RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct = {0};
		LTDC_LayerCfgTypeDef     pLayerCfg = {0};

		/* ֧��6����� */
		switch (g_LcdType)
		{
			case LCD_35_480X320:	/* 3.5�� 480 * 320 */	
				Width = 480;
				Height = 272;
				HSYNC_W = 10;
				HBP = 20;
				HFP = 20;
				VSYNC_W = 20;
				VBP = 20;
				VFP = 20;
				break;
			
			case LCD_43_480X272:		/* 4.3�� 480 * 272 */			
				Width = 480;
				Height = 272;

				HSYNC_W = 40;
				HBP = 2;
				HFP = 2;
				VSYNC_W = 9;
				VBP = 2;
				VFP = 2;
		
				/* LCD ʱ������ */
				/* PLL3_VCO Input = HSE_VALUE/PLL3M = 25MHz/5 = 5MHz */
				/* PLL3_VCO Output = PLL3_VCO Input * PLL3N = 5MHz * 48 = 240MHz */
				/* PLLLCDCLK = PLL3_VCO Output/PLL3R = 240 / 10 = 24MHz */
				/* LTDC clock frequency = PLLLCDCLK = 24MHz */
				/*
					ˢ���� = 24MHz /((Width + HSYNC_W  + HBP  + HFP)*(Height + VSYNC_W +  VBP  + VFP))
                   		   = 24000000/((480 + 40  + 2  + 2)*(272 + 9 +  2  + 2)) 
			               = 24000000/(524*285)
                           = 160Hz	

					��ǰ������÷����û�ʹ��PLL3Q�����48MHzʱ�ӹ�USBʹ�á�
			    */
				PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
				PeriphClkInitStruct.PLL3.PLL3M = 5;
				PeriphClkInitStruct.PLL3.PLL3N = 48;
				PeriphClkInitStruct.PLL3.PLL3P = 2;
				PeriphClkInitStruct.PLL3.PLL3Q = 5;
				PeriphClkInitStruct.PLL3.PLL3R = 10;				
				HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);     			
				break;
			
			case LCD_50_480X272:		/* 5.0�� 480 * 272 */
				Width = 480;
				Height = 272;
			
				HSYNC_W = 40;
				HBP = 2;
				HFP = 2;
				VSYNC_W = 9;
				VBP = 2;
				VFP = 2;			
				break;
			
			case LCD_50_800X480:		/* 5.0�� 800 * 480 */
			case LCD_70_800X480:		/* 7.0�� 800 * 480 */					
				Width = 800;
				Height = 480;

				HSYNC_W = 96;	/* =10ʱ����ʾ��λ��20ʱ���������Ե�,80ʱȫ��OK */
				HBP = 10;
				HFP = 10;
				VSYNC_W = 2;
				VBP = 10;
				VFP = 10;			

				/* LCD ʱ������ */
				/* PLL3_VCO Input = HSE_VALUE/PLL3M = 25MHz/5 = 5MHz */
				/* PLL3_VCO Output = PLL3_VCO Input * PLL3N = 5MHz * 48 = 240MHz */
				/* PLLLCDCLK = PLL3_VCO Output/PLL3R = 240 / 10 = 24MHz */
				/* LTDC clock frequency = PLLLCDCLK = 24MHz */
				/*
					ˢ���� = 24MHz /((Width + HSYNC_W  + HBP  + HFP)*(Height + VSYNC_W +  VBP  + VFP))
                   		   = 24000000/((800 + 96  + 10  + 10)*(480 + 2 +  10  + 10)) 
			               = 24000000/(916*502)
                           = 52Hz	
			
					������Ҫ���ԼӴ�100Hzˢ������ȫû���⣬����PeriphClkInitStruct.PLL3.PLL3N = 100����
					��ʱ��LTDCʱ����50MHz
					ˢ���� = 50MHz /(��Width + HSYNC_W  + HBP  + HFP ��*(Height + VSYNC_W +  VBP  +VFP  )) 
					       = 5000000/(916*502) 
					       = 108.7Hz

					��ǰ������÷����û�ʹ��PLL3Q�����48MHzʱ�ӹ�USBʹ�á�
			    */ 
				PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
				PeriphClkInitStruct.PLL3.PLL3M = 5;
				PeriphClkInitStruct.PLL3.PLL3N = 48;
				PeriphClkInitStruct.PLL3.PLL3P = 2;
				PeriphClkInitStruct.PLL3.PLL3Q = 5;
				PeriphClkInitStruct.PLL3.PLL3R = 10; 
				HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);     			
				break;
			
			case LCD_70_1024X600:		/* 7.0�� 1024 * 600 */
				/* ʵ������ʱ�� = 53.7M */
				Width = 1024;
				Height = 600;

				HSYNC_W = 2;	/* =10ʱ����ʾ��λ��20ʱ���������Ե�,80ʱȫ��OK */
				HBP = 157;
				HFP = 160;
				VSYNC_W = 2;
				VBP = 20;
				VFP = 12;		
			
				PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
				PeriphClkInitStruct.PLL3.PLL3M = 5;
				PeriphClkInitStruct.PLL3.PLL3N = 48;
				PeriphClkInitStruct.PLL3.PLL3P = 2;
				PeriphClkInitStruct.PLL3.PLL3Q = 5;
				PeriphClkInitStruct.PLL3.PLL3R = 10;
				HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct); 			
				break;
			
			default:
				Width = 800;
				Height = 480;

				HSYNC_W = 80;	/* =10ʱ����ʾ��λ��20ʱ���������Ե�,80ʱȫ��OK */
				HBP = 10;
				HFP = 10;
				VSYNC_W = 10;
				VBP = 10;
				VFP = 10;		
			
				/* LCD ʱ������ */
				/* PLL3_VCO Input = HSE_VALUE/PLL3M = 25MHz/5 = 5MHz */
				/* PLL3_VCO Output = PLL3_VCO Input * PLL3N = 5MHz * 48 = 240MHz */
				/* PLLLCDCLK = PLL3_VCO Output/PLL3R = 240 / 10 = 24MHz */
				/* LTDC clock frequency = PLLLCDCLK = 24MHz */
				/*
					ˢ���� = 24MHz /((Width + HSYNC_W  + HBP  + HFP)*(Height + VSYNC_W +  VBP  + VFP))
                   		   = 24000000/((800 + 96  + 10  + 10)*(480 + 2 +  10  + 10)) 
			               = 24000000/(916*502)
                           = 52Hz

					������Ҫ���ԼӴ�100Hzˢ������ȫû���⣬����PeriphClkInitStruct.PLL3.PLL3N = 100����
					��ʱ��LTDCʱ����50MHz
					ˢ���� = 50MHz /(��Width + HSYNC_W  + HBP  + HFP ��*(Height + VSYNC_W +  VBP  +VFP  )) 
					       = 5000000/(916*502) 
					       = 108.7Hz

					��ǰ������÷����û�ʹ��PLL3Q�����48MHzʱ�ӹ�USBʹ�á�
			    */ 
				PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
				PeriphClkInitStruct.PLL3.PLL3M = 5;
				PeriphClkInitStruct.PLL3.PLL3N = 48;
				PeriphClkInitStruct.PLL3.PLL3P = 2;
				PeriphClkInitStruct.PLL3.PLL3Q = 5;
				PeriphClkInitStruct.PLL3.PLL3R = 10;  
				HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct); 			
				break;
		}		

		g_LcdHeight = Height;
		g_LcdWidth = Width;
		
		/* �����źż��� */	
		hLTDC.Init.HSPolarity = LTDC_HSPOLARITY_AL;	/* HSYNC �͵�ƽ��Ч */
		hLTDC.Init.VSPolarity = LTDC_VSPOLARITY_AL; 	/* VSYNC �͵�ƽ��Ч */
		hLTDC.Init.DEPolarity = LTDC_DEPOLARITY_AL; 	/* DE �͵�ƽ��Ч */
		hLTDC.Init.PCPolarity = LTDC_PCPOLARITY_IPC;

		/* ʱ������ */    
		hLTDC.Init.HorizontalSync = (HSYNC_W - 1);
		hLTDC.Init.VerticalSync = (VSYNC_W - 1);
		hLTDC.Init.AccumulatedHBP = (HSYNC_W + HBP - 1);
		hLTDC.Init.AccumulatedVBP = (VSYNC_W + VBP - 1);  
		hLTDC.Init.AccumulatedActiveH = (Height + VSYNC_W + VBP - 1);
		hLTDC.Init.AccumulatedActiveW = (Width + HSYNC_W + HBP - 1);
		hLTDC.Init.TotalHeigh = (Height + VSYNC_W + VBP + VFP - 1);
		hLTDC.Init.TotalWidth = (Width + HSYNC_W + HBP + HFP - 1); 

		/* ���ñ�������ɫ */
		hLTDC.Init.Backcolor.Blue = 0;
		hLTDC.Init.Backcolor.Green = 0;
		hLTDC.Init.Backcolor.Red = 0;

		hLTDC.Instance = LTDC;
		

		/* ��ʼ����ͼ�� ------------------------------------------------------*/
		/* ������ʾ������ */ 
		pLayerCfg.WindowX0 = 0;
		pLayerCfg.WindowX1 = Width;
		pLayerCfg.WindowY0 = 0;
		pLayerCfg.WindowY1 = Height;

		/* ������ɫ��ʽ */ 
		pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;

		/* �Դ��ַ */
		pLayerCfg.FBStartAdress = FrameBufer;	

		/* Alpha���� (255 ��ʾ��ȫ��͸��) */
		pLayerCfg.Alpha = 255;

		/* �ޱ���ɫ */
		pLayerCfg.Alpha0 = 0; 	/* ��ȫ͸�� */
		pLayerCfg.Backcolor.Blue = 0;
		pLayerCfg.Backcolor.Green = 0;
		pLayerCfg.Backcolor.Red = 0;

		/* ����ͼ�������� */
		pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
		pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;

		/* �������д�С */
		pLayerCfg.ImageWidth  = Width;
		pLayerCfg.ImageHeight = Height;

		/* ����LTDC  */  
		if (HAL_LTDC_Init(&hLTDC) != HAL_OK)
		{
			/* ��ʼ������ */
			Error_Handler(__FILE__, __LINE__);
		}

		/* ����ͼ��1 */
		if (HAL_LTDC_ConfigLayer(&hLTDC, &pLayerCfg, LTDC_LAYER_1) != HAL_OK)
		{
			/* ��ʼ������ */
			Error_Handler(__FILE__, __LINE__);
		}  
	}  
}

/***************************** ���������� www.armfly.com (END OF FILE) *********************************/
