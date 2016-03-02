#ifndef __GUI_H__
#define __GUI_H__
#include "stm32f10x.h"
void GUI_DrawPoint(u16 x,u16 y,u16 color);
void LCD_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 color);
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2);
void LCD_DrawRectangle(u16 xStart, u16 yStart, u16 xEnd, u16 yEnd, u16 color, unsigned char fillOrEmpty);

void Draw_Circle(u16 x0,u16 y0,u16 fc,u8 r);
void LCD_ShowChar(u16 x,u16 y,u16 fc, u16 bc, u8 num,u8 size,u8 mode);
void LCD_ShowNum(u16 x,u16 y,u32 num,u8 len,u8 size);
void LCD_Show2Num(u16 x,u16 y,u16 num,u8 len,u8 size,u8 mode);
void LCD_ShowString(u16 x,u16 y,u8 size,u8 *p,u8 mode);

void Show_Str(u16 x, u16 y, u16 fc, u16 bc, u8 *str,u8 size,u8 mode);
void Gui_Drawbmp16(u16 x,u16 y,const unsigned char *p); //显示40*40 QQ图片
void gui_circle(int xc, int yc, u16 c,int r, int fill);
void Gui_StrCenter(u16 x, u16 y, u16 fc, u16 bc, u8 *str,u8 size,u8 mode);



/*
	以下三个函数为一个集团
*/
unsigned int LCD_printf(unsigned int x, unsigned int y, const char *format, ...);
unsigned int LCD_SetFontColor(unsigned int color);
unsigned int LCD_SetFontBackColor(unsigned int color);
void 		 LCD_WritePoint(unsigned short x, unsigned short y, unsigned short color);
void 		 LCD_DrawBMP(unsigned short x, unsigned short y, unsigned short sizeX, unsigned short sizeY, const char *pic);
#endif

