/*
*********************************************************************************************************
*	                                  
*	模块名称 : GUI界面主函数
*	文件名称 : MainTask.c
*	版    本 : V1.0
*	说    明 : GUI界面主函数
*
*		版本号   日期         作者            说明
*		v1.0    2020-07-06  Eric2013  	      首版
*
*	Copyright (C), 2020-2030, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#ifndef __MainTask_H
#define __MainTask_H
#include "main.h"
//#include "bsp.h"
#include "gx_user.h"
#include "guiapp_resources.h"
#include "guiapp_specifications.h"
//#include "tx_api.h"
#include "gx_system.h"
#include "gx_display.h"
#include "gx_utility.h"


/*
************************************************************************
*						宏定义
************************************************************************
*/



/*
************************************************************************
*						供外部文件调用
************************************************************************
*/
extern void MainTask(void);
extern void TOUCH_Calibration(uint8_t _PointCount);
extern void gx_initconfig(void);

#endif

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
