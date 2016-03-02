#include <stm32f10x.h>
#include "button.h"
#include "display.h"

#define BUTTON_GPIO 		GPIOG
#define BUTTON_GPIO_CLK		RCC_APB2Periph_GPIOG

#define BUTTON_VCC_PIN		GPIO_Pin_6
#define BUTTON_GND_PIN		GPIO_Pin_7

#define BUTTON1_PIN			GPIO_Pin_3
#define BUTTON2_PIN			GPIO_Pin_4
#define BUTTON3_PIN			GPIO_Pin_5

unsigned short ButtonPins[BUTTON_AMOUNT] = {BUTTON1_PIN, BUTTON2_PIN, BUTTON3_PIN};


ButtonHandlerType handlers[BUTTON_AMOUNT];


ButtonInfoType ButtonInfos[BUTTON_AMOUNT] = 
{
	{0, BUTTON_GPIO, BUTTON1_PIN, 0, 0, 0, 0, 0},
	{1, BUTTON_GPIO, BUTTON2_PIN, 0, 0, 0, 0, 0},
	{2, BUTTON_GPIO, BUTTON3_PIN, 0, 1, 0, 0, 0}
};

void BTN_init()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(BUTTON_GPIO_CLK, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = BUTTON1_PIN | BUTTON2_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(BUTTON_GPIO, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = BUTTON3_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(BUTTON_GPIO, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = BUTTON_VCC_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(BUTTON_GPIO, &GPIO_InitStructure);
	
	GPIO_WriteBit(BUTTON_GPIO, BUTTON_VCC_PIN, Bit_SET);
}


unsigned char BTN_CheckState(unsigned short btnIndex)
{
	return !GPIO_ReadInputDataBit(BUTTON_GPIO, ButtonPins[btnIndex]);
}

void BTN_SetHandler(unsigned short btnIndex, ButtonHandlerType pressHandler, ButtonHandlerType releaseHandler, ButtonHandlerType holdHandler)
{
	ButtonInfos[btnIndex].PressHandler = pressHandler;
	ButtonInfos[btnIndex].ReleaseHandler = releaseHandler;
	ButtonInfos[btnIndex].HoldHandler = holdHandler;
}

void BTN_HandlerProcess()
{
	int index;
	for (index = 0; index < BUTTON_AMOUNT; index++)
	{
		if(ButtonInfos[index].Type == 0)
		{
			if (BTN_CheckState(index))
			{
				if (ButtonInfos[index].State && ButtonInfos[index].HoldHandler)
				{
					ButtonInfos[index].HoldHandler(1);
				}
				else if (!ButtonInfos[index].State && ButtonInfos[index].PressHandler)
				{
					ButtonInfos[index].PressHandler(1);
				}
				ButtonInfos[index].State = 1;
			}
			else
			{
				if (ButtonInfos[index].State && ButtonInfos[index].ReleaseHandler)
				{
					ButtonInfos[index].ReleaseHandler(0);
				}
				ButtonInfos[index].State = 0;
			}
		}
		else
		{
			if(ButtonInfos[index].State != BTN_CheckState(index) && ButtonInfos[index].PressHandler)
			{
				ButtonInfos[index].PressHandler(BTN_CheckState(index));
				ButtonInfos[index].State =  BTN_CheckState(index);
			}
			
		}
		
	}
}


void BTN1_PressHandler(unsigned char state)
{
	OLED_PutFormat(0, 0, "Pressed    ");
}

void BTN1_ReleaseHandler(unsigned char state)
{
	OLED_PutFormat(0, 0, "Released   ");
}

void BTN1_HoldHandler(unsigned char state)
{
	OLED_PutFormat(0, 0, "Hold       ");
}


void BTN2_PressHandler(unsigned char state)
{
	OLED_PutFormat(1, 0, "Pressed    ");
}

void BTN2_ReleaseHandler(unsigned char state)
{
	OLED_PutFormat(1, 0, "Released   ");
}

void BTN2_HoldHandler(unsigned char state)
{
	OLED_PutFormat(1, 0, "Hold       ");
}

void BTN3_PressHandler(unsigned char state)
{
	OLED_PutFormat(2, 0, "SWICH%d", state);
}

void BTN3_ReleaseHandler(unsigned char state)
{
	OLED_PutFormat(2, 0, "Released   ");
}

void BTN3_HoldHandler(unsigned char state)
{
	OLED_PutFormat(2, 0, "Hold       ");
}

void BTN_Test()
{
	BTN_init();
	
	BTN_SetHandler(0, BTN1_PressHandler, BTN1_ReleaseHandler, BTN1_HoldHandler);
	BTN_SetHandler(1, BTN2_PressHandler, BTN2_ReleaseHandler, BTN2_HoldHandler);
	BTN_SetHandler(2, BTN3_PressHandler, BTN3_ReleaseHandler, BTN3_HoldHandler);
}





















