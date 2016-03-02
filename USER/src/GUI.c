#include "lcd.h"
#include "string.h"
#include "font.h" 
#include "delay.h"
#include "gui.h"

void GUI_DrawPoint(u16 x,u16 y,u16 color)
{
	LCD_SetCursor(x,y);//设置光标位置 
	LCD_DrawPoint_16Bit(color); 
}


void LCD_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 color)
{  	

	u32 i;				
	u16 width=ex-sx+1; 		//得到填充的宽度
	u16 height=ey-sy+1;		//高度
	u32 totalpoint=height*width;
	LCD_SetWindows(sx,sy,ex-1,ey-1);//设置显示窗口


	for(i=0;i<totalpoint;i++)
	{
		LCD->LCD_RAM=color;	//写入数据 	 
	}

	LCD_SetWindows(0,0,lcddev.width-1,lcddev.height-1);//恢复窗口设置为全屏
}


void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)
{
	u16 t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance; 
	int incx,incy,uRow,uCol; 

	
	
	delta_x=x2-x1; //计算坐标增量 
	delta_y=y2-y1; 
	uRow=x1; 
	uCol=y1; 
	if(delta_x>0)incx=1; //设置单步方向 
	else if(delta_x==0)incx=0;//垂直线 
	else {incx=-1;delta_x=-delta_x;} 
	if(delta_y>0)incy=1; 
	else if(delta_y==0)incy=0;//水平线 
	else{incy=-1;delta_y=-delta_y;} 
	if( delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴 
	else distance=delta_y; 
	for(t=0;t<=distance+1;t++ )//画线输出 
	{  
		LCD_DrawPoint(uRow,uCol);//画点 
		xerr+=delta_x ; 
		yerr+=delta_y ; 
		if(xerr>distance) 
		{ 
			xerr-=distance; 
			uRow+=incx; 
		} 
		if(yerr>distance) 
		{ 
			yerr-=distance; 
			uCol+=incy; 
		} 
	}  
} 


void LCD_DrawEmptyRectangle(u16 x1, u16 y1, u16 x2, u16 y2)
{
	LCD_DrawLine(x1,y1,x2,y1);
	LCD_DrawLine(x1,y1,x1,y2);
	LCD_DrawLine(x1,y2,x2,y2);
	LCD_DrawLine(x2,y1,x2,y2);
}


void LCD_DrawFilledRectangle(u16 x1, u16 y1, u16 x2, u16 y2)
{
	LCD_Fill(x1,y1,x2,y2, POINT_COLOR);
}

void LCD_DrawRectangle(u16 xStart, u16 yStart, u16 xEnd, u16 yEnd, u16 color, unsigned char fillOrEmpty)
{
	u16 oldPointColor = POINT_COLOR;
	
	POINT_COLOR = color;
	
	if(fillOrEmpty)
		LCD_DrawFilledRectangle(xStart, yStart, xEnd, yEnd);
	else
		LCD_DrawEmptyRectangle(xStart, yStart, xEnd, yEnd);
	
	POINT_COLOR = oldPointColor;
}
 

void _draw_circle_8(int xc, int yc, int x, int y, u16 c)
{
	GUI_DrawPoint(xc + x, yc + y, c);

	GUI_DrawPoint(xc - x, yc + y, c);

	GUI_DrawPoint(xc + x, yc - y, c);

	GUI_DrawPoint(xc - x, yc - y, c);

	GUI_DrawPoint(xc + y, yc + x, c);

	GUI_DrawPoint(xc - y, yc + x, c);

	GUI_DrawPoint(xc + y, yc - x, c);

	GUI_DrawPoint(xc - y, yc - x, c);
}


void gui_circle(int xc, int yc, u16 c, int r, int fill)
{
	int x = 0, y = r, yi, d;

	d = 3 - 2 * r;

	if (fill) 
	{
		// 如果填充（画实心圆）
		while (x <= y) {
			for (yi = x; yi <= y; yi++)
				_draw_circle_8(xc, yc, x, yi, c);

			if (d < 0) {
				d = d + 4 * x + 6;
			} else {
				d = d + 4 * (x - y) + 10;
				y--;
			}
			x++;
		}
	} else 
	{
		// 如果不填充（画空心圆）
		while (x <= y) {
			_draw_circle_8(xc, yc, x, y, c);
			if (d < 0) {
				d = d + 4 * x + 6;
			} else {
				d = d + 4 * (x - y) + 10;
				y--;
			}
			x++;
		}
	}
}

void LCD_ShowChar(u16 x,u16 y,u16 fc, u16 bc, u8 num,u8 size, u8 mode)
{  
    u8 temp;
    u8 pos,t;
	u16 colortemp=POINT_COLOR;      
	
	num=num-' ';//得到偏移后的值
	LCD_SetWindows(x,y,x+size/2-1,y+size-1);//设置单个文字显示窗口
	if(!mode) //非叠加方式
	{
		for(pos=0;pos<size;pos++)
		{
			if(size==12)temp=asc2_1206[num][pos];//调用1206字体
			else temp=asc2_1608[num][pos];		 //调用1608字体
			for(t=0;t<size/2;t++)
		    {                 
		        if(temp&0x01)
					LCD_WR_DATA(fc); 
				else 
					LCD_WR_DATA(bc); 
				temp>>=1; 
		    }
		}	
	}
	else//叠加方式
	{
		for(pos=0;pos<size;pos++)
		{
			if(size==12)temp=asc2_1206[num][pos];//调用1206字体
			else temp=asc2_1608[num][pos];		 //调用1608字体
			for(t=0;t<size/2;t++)
		    {   
				POINT_COLOR=fc;              
		        if(temp&0x01)
					LCD_DrawPoint(x+t,y+pos);//画一个点
			    
		        temp>>=1; 
		    }
		}
	}
	POINT_COLOR=colortemp;	
	LCD_SetWindows(0,0,lcddev.width-1,lcddev.height-1);//恢复窗口为全屏    	   	 	  
} 
	  
void LCD_ShowString(u16 x,u16 y,u8 size,u8 *p,u8 mode)
{         
    while((*p<='~')&&(*p>=' '))//判断是不是非法字符!
    {   
		if(x>(lcddev.width-1)||y>(lcddev.height-1)) 
		return;     
        LCD_ShowChar(x,y,POINT_COLOR,BACK_COLOR,*p,size,mode);
        x+=size/2;
        p++;
    }  
} 

 
u32 mypow(u8 m,u8 n)
{
	u32 result=1;	 
	while(n--)result*=m;    
	return result;
}

 			 
void LCD_ShowNum(u16 x,u16 y,u32 num,u8 len,u8 size)
{         	
	u8 t,temp;
	u8 enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/mypow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				LCD_ShowChar(x+(size/2)*t,y,POINT_COLOR,BACK_COLOR,' ',size,0);
				continue;
			}
			else 
				enshow=1; 
		 	 
		}
	 	LCD_ShowChar(x+(size/2)*t,y,POINT_COLOR,BACK_COLOR,temp+'0',size,0); 
	}
}

  	   		   
void Show_Str(u16 x, u16 y, u16 fc, u16 bc, u8 *str,u8 size,u8 mode)
{					
	u16 x0=x;							  	  
  	u8 bHz=0;     //字符或者中文 
	
    while(*str!=0)//数据未结束
    { 
        if(!bHz)
        {
			if(x>(lcddev.width-size/2)||y>(lcddev.height-size)) 
			return; 
	        if(*str>0x80)bHz=1;	//中文 
	        else              	//字符
	        {          
		        if(*str==0x0D)//换行符号
		        {         
		            y+=size;
					x=x0;
		            str++; 
		        }  
		        else
				{
					if(size>16)	//字库中没有集成12X24 16X32的英文字体,用8X16代替
					{  
					LCD_ShowChar(x,y,fc,bc,*str,16,mode);
					x+=8; //字符,为全字的一半 
					}
					else
					{
					LCD_ShowChar(x,y,fc,bc,*str,size,mode);
					x+=size/2; //字符,为全字的一半 
					}
				} 
				str++; 
		        
	        }
        }				 
    }   
}

//******************************************************************
//函数名：  Gui_StrCenter
//作者：    xiao冯@全动电子
//日期：    2013-02-22
//功能：    居中显示一个字符串,包含中英文显示
//输入参数：x,y :起点坐标
// 			fc:前置画笔颜色
//			bc:背景颜色
//			str :字符串	 
//			size:字体大小
//			mode:模式	0,填充模式;1,叠加模式
//返回值：  无
//修改记录：无
//******************************************************************   
void Gui_StrCenter(u16 x, u16 y, u16 fc, u16 bc, u8 *str,u8 size,u8 mode)
{
	u16 len=strlen((const char *)str);
	u16 x1=(lcddev.width-len*8)/2;
	Show_Str(x+x1,y,fc,bc,str,size,mode);
} 
 
//******************************************************************
//函数名：  Gui_Drawbmp16
//作者：    xiao冯@全动电子
//日期：    2013-02-22
//功能：    显示一副16位BMP图像
//输入参数：x,y :起点坐标
// 			*p :图像数组起始地址
//返回值：  无
//修改记录：无
//******************************************************************  
void Gui_Drawbmp16(u16 x,u16 y,const unsigned char *p) //显示40*40 QQ图片
{
  	int i; 
	unsigned char picH,picL; 
	LCD_SetWindows(x,y,x+40-1,y+40-1);//窗口设置
    for(i=0;i<40*40;i++)
	{	
	 	picL=*(p+i*2);	//数据低位在前
		picH=*(p+i*2+1);				
		LCD_WR_DATA(picH<<8|picL);  						
	}	
	LCD_SetWindows(0,0,lcddev.width-1,lcddev.height-1);//恢复显示窗口为全屏
}


#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static char buffer[255];

unsigned short FrontColor = WHITE;
unsigned short BackColor = BLACK;

unsigned int LCD_SetFontColor(unsigned int color)
{
	unsigned int oldColor = FrontColor;
	FrontColor = color;
	return oldColor;
}

unsigned int LCD_SetFontBackColor(unsigned int color)
{
	unsigned int oldColor = FrontColor;
	BackColor = color;
	return oldColor;
}

unsigned int LCD_printf(unsigned int x, unsigned int y, const char *format, ...)
{
	unsigned len;
	va_list args;
	va_start(args, format);
	
	len = vsprintf(buffer, format, args);
	
	Show_Str(x, y, FrontColor, BackColor,(u8*) buffer, 16, 0);
	
	va_end(args);
	
	return len; 
}

void LCD_WritePoint(unsigned short x, unsigned short y, unsigned short color)
{
	LCD_SetCursor(x, y);
	LCD_WR_DATA(color);
}


void LCD_DrawBMP(unsigned short x, unsigned short y, unsigned short sizeX, unsigned short sizeY, const char *pic)
{
	unsigned index = sizeX * sizeY;
	unsigned short rbg = 0;
	LCD_SetWindows(x, y, x + sizeX - 1, y + sizeY - 1);
	LCD_WriteRAM_Prepare();
	
	while(index--)
	{
		rbg = 0;
		rbg |= *pic++;
		rbg <<= 8;
		rbg |= *pic++;
		LCD_WR_DATA(rbg);
	}
	
}
