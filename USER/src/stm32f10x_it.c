/******************** (C) COPYRIGHT 2010 STMicroelectronics ********************
* File Name          : stm32f10x_it.c
* Author             : MCD Application Team
* Version            : V3.2.1
* Date               : 07/05/2010
* Description        : Main Interrupt Service Routines.
*                      This file provides template for all exceptions handler
*                      and peripherals interrupt service routine.
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <stm32f10x_it.h>
#include <ov7670.h>
#include "sys.h"
#include "lcd.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/*******************************************************************************
* Function Name  : NMI_Handler
* Description    : This function handles NMI exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void NMI_Handler(void)
{
}

/*******************************************************************************
* Function Name  : HardFault_Handler
* Description    : This function handles Hard Fault exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/*******************************************************************************
* Function Name  : MemManage_Handler
* Description    : This function handles Memory Manage exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/*******************************************************************************
* Function Name  : BusFault_Handler
* Description    : This function handles Bus Fault exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/*******************************************************************************
* Function Name  : UsageFault_Handler
* Description    : This function handles Usage Fault exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/*******************************************************************************
* Function Name  : SVC_Handler
* Description    : This function handles SVCall exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SVC_Handler(void)
{
}

/*******************************************************************************
* Function Name  : DebugMon_Handler
* Description    : This function handles Debug Monitor exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DebugMon_Handler(void)
{
}

/*******************************************************************************
* Function Name  : PendSV_Handler
* Description    : This function handles PendSVC exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void PendSV_Handler(void)
{
}

/*******************************************************************************
* Function Name  : SysTick_Handler
* Description    : This function handles SysTick Handler.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SysTick_Handler(void)
{
}

/******************************************************************************/
/*            STM32F10x Peripherals Interrupt Handlers                        */
/******************************************************************************/

#ifndef STM32F10X_CL
/*******************************************************************************
* Function Name  : USB_HP_CAN1_TX_IRQHandler
* Description    : This function handles USB High Priority or CAN TX interrupts requests
*                  requests.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USB_HP_CAN1_TX_IRQHandler(void)
{
  //CTR_HP();
}

/*******************************************************************************
* Function Name  : USB_LP_CAN1_RX0_IRQHandler
* Description    : This function handles USB Low Priority or CAN RX0 interrupts
*                  requests.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USB_LP_CAN1_RX0_IRQHandler(void)
{
  //USB_Istr();
}
#endif /* STM32F10X_CL */

#if defined(STM32F10X_HD) || defined(STM32F10X_XL) 
/*******************************************************************************
* Function Name  : SDIO_IRQHandler
* Description    : This function handles SDIO global interrupt request.
*                  requests.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SDIO_IRQHandler(void)
{ 
  /* Process All SDIO Interrupt Sources */
  //SD_ProcessIRQSrc();
  
}
#endif /* STM32F10X_HD | STM32F10X_XL*/

#ifdef STM32F10X_CL
/*******************************************************************************
* Function Name  : OTG_FS_IRQHandler
* Description    : This function handles USB-On-The-Go FS global interrupt request.
*                  requests.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void OTG_FS_IRQHandler(void)
{
  STM32_PCD_OTG_ISR_Handler(); 
}
#endif /* STM32F10X_CL */

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/*------------------------    User Include  Start      ------------------------*/

#include "display.h"
#include "TargetRecognition.h"
#include "button.h"
/*------------------------     User Include End        ------------------------*/


u8 Vsync_Flag=0;

u16 x,y,t1,t2;

void EXTI4_IRQHandler(void)
{ 
	EXTI_ClearITPendingBit(EXTI_Line4);  		
	
	Vsync_Flag++;
	
	if(Vsync_Flag == 1)    //FIFO写指针复位
	{
		FIFO_WRST = 1;
		FIFO_WRST = 0;
 		for(x=0; x<100; x++);
		FIFO_WRST = 1;      
		FIFO_WR = 1;	   		//允许CMOS数据写入FIFO
	}
	
	if(Vsync_Flag==2)
	{
	 	FIFO_WR=0;     						//禁止CMOS数据写入FIFO
		EXTI->IMR&=~(1<<4);	 				//禁止外部中断，准备从FIFO中取数据
		EXTI->EMR&=~(1<<4);
		
		FIFO_RRST=0; //FIFO读指针复位 
		FIFO_RCK=0;               
		FIFO_RCK=1;
		FIFO_RCK=0;
		FIFO_RCK=1;
		FIFO_RRST=1;
		
		//TR_TouchHandler();

		BTN_HandlerProcess();
		
		if(RunMode == DEBUG_MODE && DebugMode == DEBUG_VIDEO_MODE)
		{
			LCD_SetCursor(0, 0);
			LCD_WriteRAM_Prepare();
		}
		
		FIFO_OE=0;			  				//允许FIFO输出
		for(y = 0; y < 220; y ++)	 		//该开发板配套的TFT2.8屏最大显示尺寸320*240
		{	
		 	for(x = 0; x < 320; x ++)
			{		
				u16 color;
				
				FIFO_RCK=0;				
				FIFO_RCK=1;	 				
				t1=(0x00ff&GPIOF->IDR);	 				
				FIFO_RCK=0;			
				FIFO_RCK=1;				
				t2=(0x00ff&GPIOF->IDR); 				
				color = ((t1 << 8) | t2);
				
				if(RunMode == DEBUG_MODE && DebugMode == DEBUG_VIDEO_MODE)
				{
					if(x == SamplePoint.x && y == SamplePoint.y)
					{
						LCD_WR_DATA(WHITE);
						OLED_PutFormat(6, 0, "Smp:%-2x %-2x %-2x %4x", R(color), G(color), B(color), color);
					}
					else
						LCD_WR_DATA(color);
				}
				else
					TR_TakePoint((color), x, y);
			}
		}
		
		FIFO_OE=1;		 	  //禁止FIFO输出
		Vsync_Flag=0;	   
		EXTI->IMR|=(1<<4);	  //允许外部中断，以便接收下帧图像数据
		EXTI->EMR|=(1<<4);
	} 		
		
}


