#include "delay.h"
//#include "sys.h"

#include "display.h"
#include "ov7670.h"
#include "TargetRecognition.h"
#include "button.h"

void BTN_Test(void);

int main(void)
{	
	delay_init();	 			//延时初始化
	
	OLED_Init();
	OLED_PushLine("System Init Ok");
	
	BTN_init();
	OLED_PushLine("Buttons online");
	
	LCD_Init();	   			//液晶屏初始化
	LCD_SetFontBackColor(RED);
	LCD_SetFontColor(WHITE);
//	LCD_printf(320/2, 200, "Welcome!");
	OLED_PushLine("LCD Online");
//	
//	TP_Init();
//	OLED_PushLine("%s", TP_Init() ? "Touch Online" : "Touch Fail");
//	
	TR_Init();
//	
	OLED_PushLine("OV7670 Init...");
	OV7670_Init();
	OLED_PushLine("OV7670 OnLine");
//	
//	TR_Test();
	
//	BTN_Test();
	while(1)
	{
		//BTN_HandlerProcess();
	}
}
