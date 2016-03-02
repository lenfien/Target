
#ifndef __BUTTON
#define __BUTTON

#include "stm32f10x.h"

#define BUTTON_AMOUNT		3

#define BUTTON1				0
#define BUTTON2				1
#define BUTTON3				2

#define SWITCH				BUTTON3

typedef void (*ButtonHandlerType)(unsigned char state); 

typedef struct
{
	unsigned short    Index;
	GPIO_TypeDef*     GPIOPort;
	unsigned char	  GPIOPin;
	unsigned char 	  State;			//0. released	1.pressed 
	unsigned char	  Type;				//0. normal button 1.switch
	ButtonHandlerType PressHandler;
	ButtonHandlerType ReleaseHandler;
	ButtonHandlerType HoldHandler;
}ButtonInfoType;


void 			BTN_init(void);
unsigned char 	BTN_CheckState(unsigned short btn);
void 			BTN_HandlerProcess(void);
void 			BTN_SetHandler(unsigned short btnIndex, ButtonHandlerType pressHandler, ButtonHandlerType releaseHandler, ButtonHandlerType holdHandler);



#endif
