
#include <stdio.h>
#include <stdarg.h>


#include "stm32f10x.h"
#include "delay.h"
#include "oled.h"

//当前的地址递增模式
OLED_AddressModeDefType 			OLED_AddressMode;
OLED_PositionDefType 				OLED_Position;
OLED_AreaDefType					OLED_Area;

unsigned char 	OLED_CurrentContrast;
bool 			OLED_AutoWrap;				//在PageMode下是否开启文字自动换行

extern const 	unsigned char fonts[][6];	
static char 	buffer[125];				//用于OLED_PutFormat的输出缓存

#define SPI_MASTER 						SPI2
#define SPI_MASTER_CLK					RCC_APB1Periph_SPI2
#define SPI_MASTER_GPIO					GPIOB
#define SPI_MASTER_GPIO_CLK				RCC_APB2Periph_GPIOB
#define SPI_MASTER_PIN_SCL				GPIO_Pin_13			//PB.13	as Clk
#define SPI_MASTER_PIN_MOSI				GPIO_Pin_15			//PB.15 as MOSI
#define SPI_MASTER_PIN_MISO				GPIO_Pin_14			//Not in use
#define SPI_MASTER_CTR_PIN_CS			GPIO_Pin_10
#define SPI_MASTER_CTR_PIN_DC			GPIO_Pin_11
#define SPI_MASTER_CTR_PIN_RST			GPIO_Pin_12

#define SPI_MASTER_WAIT_FOR_SEND		while(SPI_I2S_GetFlagStatus(SPI_MASTER, SPI_I2S_FLAG_BSY))	//SPI waits until Tx register empty
#define SPI_MASTER_SEND_BYTE(byte)		SPI_I2S_SendData(SPI_MASTER, byte)

#define CS_ACT							GPIO_WriteBit(SPI_MASTER_GPIO, SPI_MASTER_CTR_PIN_CS, Bit_RESET)
#define CS_DEACT						GPIO_WriteBit(SPI_MASTER_GPIO, SPI_MASTER_CTR_PIN_CS, Bit_SET)

#define DC_CMD							GPIO_WriteBit(SPI_MASTER_GPIO, SPI_MASTER_CTR_PIN_DC, Bit_RESET)
#define DC_DATA							GPIO_WriteBit(SPI_MASTER_GPIO, SPI_MASTER_CTR_PIN_DC, Bit_SET)

#define RST_ACT							GPIO_WriteBit(SPI_MASTER_GPIO, SPI_MASTER_CTR_PIN_RST, Bit_RESET)
#define RST_DEACT						GPIO_WriteBit(SPI_MASTER_GPIO, SPI_MASTER_CTR_PIN_RST, Bit_SET)

SPI_InitTypeDef 						SPI_InitStructure;
GPIO_InitTypeDef 						GPIO_InitStructure;

void test(void);
void OLED_SSD1306_Init(void);

void OLED_Init()
{
	/*SPI_MASTER_CLK and SPI_MASTER_GPIO and AFIO init--------------------------*/
	RCC_APB1PeriphClockCmd(SPI_MASTER_CLK, ENABLE);
	RCC_APB2PeriphClockCmd(SPI_MASTER_GPIO_CLK | RCC_APB2Periph_AFIO, ENABLE);
	
	/* SPI Pins ----------------------------------------------------------------*/
	GPIO_InitStructure.GPIO_Pin = SPI_MASTER_PIN_SCL | SPI_MASTER_PIN_MOSI;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(SPI_MASTER_GPIO, &GPIO_InitStructure);
	
	/* SPI Control Pins ------------------------------------------------------ */
	GPIO_InitStructure.GPIO_Pin = SPI_MASTER_CTR_PIN_CS | SPI_MASTER_CTR_PIN_DC | SPI_MASTER_CTR_PIN_RST;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(SPI_MASTER_GPIO, &GPIO_InitStructure);
	
	/* SPI_MASTER configuration ------------------------------------------------*/
	SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI_MASTER, &SPI_InitStructure);
	
	SPI_Cmd(SPI_MASTER, ENABLE);
	
	OLED_SSD1306_Init();
	
	OLED_Clear();
}


void OLED_SendCmd(unsigned char cmd)
{
     DC_CMD;
     CS_ACT;
     SPI_MASTER_WAIT_FOR_SEND;
     SPI_MASTER_SEND_BYTE(cmd);
     SPI_MASTER_WAIT_FOR_SEND;
     CS_DEACT;
}


void OLED_SendData(unsigned char data)
{
     DC_DATA;
     CS_ACT;
     SPI_MASTER_WAIT_FOR_SEND;
     SPI_MASTER_SEND_BYTE(data);
     SPI_MASTER_WAIT_FOR_SEND;
     CS_DEACT;
}                                                                                                           

/*
	设置点阵亮度
	contrast = 0 ~ 255
*/

void OLED_SetContrast(unsigned char contrast)
{
     OLED_SendCmd(0x81);
     OLED_SendCmd(contrast);
	
     OLED_CurrentContrast = contrast;
}

/************************************************
 *翻转GDDRAM显示
 *	inverse = true
 *	0 显示 1 不显示
 *	inverse = false
 *	1 显示 0 不显示
 **************************************************/

void OLED_InverseDisplay(bool inverse)
{
     OLED_SendCmd(inverse ? 0xA7 : 0xA6);
}

void OLED_DisplayOn()
{
     OLED_SendCmd(0xAF);
}
/*
OLED_DisplayOn：打开显示
display_OLED_Sleep:
OLED_Sleep = true: 关闭显示，进入睡眠模式
OLED_Sleep = false： 开机显示
*/
void OLED_Sleep(bool OLED_Sleep)
{
     OLED_SendCmd(OLED_Sleep ? 0xAE : 0xAF);
}
/*
	OLED_ConfigScroll 配置滚动参数

	参数：

		dir: 设置滚动的方向
		取值：
			RightHorizon, 					//水平向右
			LeftHorizon, 					//水平向左
			RightHorizonWithVertical,	//水平右+垂直上
			LeftHorizonWithVertical		//水平左+垂直上

		startPage :滚动的水平范围起始页
		取值: 0 ~ 7
		
		endPage :  滚动的水平范围结束页
		取值：0 ~ 7 (endPage >= startPage)

		frames : 滚动间隙帧数 数值越大越
		取值:
			Frames_2 = 0x07, 	
			Frames_3 = 0x04, 
			Frames_4 = 0x05, 
			Frames_5 = 0x00, 	
			Frames_25 = 0x06, 
			Frames_64 = 0x01,
			Frames_128 = 0x02, 
			Frames_256 = 0x03

		scrollRangeRowStart, scrollRangeRowEnd:  垂直滚动的范围区域
		取值： 0 ~ 63 scrollRangeRowEnd > scrollRangeRowStart

		verticalOffset : 垂直滚动偏移量，每次向上移动的点阵数目
		取值：0~63
*/
void OLED_ConfigScroll(OLED_ScrollDirectionDefType dir, 
		 unsigned char startPage,
		 unsigned char endPage, 
		 OLED_ScrollInternalFramesDefType frames,
		 unsigned char scrollRangeRowStart,
		 unsigned char scrollRangeRowEnd,
		 unsigned char verticalOffset)
{
     OLED_SendCmd(dir);		//设置滚动方向
     OLED_SendCmd(0x00);		//dummy byte
     OLED_SendCmd(startPage);
     OLED_SendCmd(frames);
     OLED_SendCmd(endPage);
     
     if(dir == LeftHorizon || dir == RightHorizon)
     {
		OLED_SendCmd(0x00);
		OLED_SendCmd(0xFF);
     }
     else
     {
		OLED_SendCmd(verticalOffset); //设置垂直偏移

		/*
		设置垂直滚动范围
		*/
		OLED_SendCmd(0xA3);
		OLED_SendCmd(scrollRangeRowStart);
		OLED_SendCmd(scrollRangeRowEnd);
     }
}
/*
	OLED_ActiveScroll: 开启或关闭滚动，开启之后不可向GDDRAM中写入数据,关闭滚动后应重写GDDRAM
	参数:
		avtive: 设置滚动状态
	取值: true false
*/	
void OLED_ActiveScroll(bool active)
{
     OLED_SendCmd(active ? 0x2F : 0x2E);
}
/*********************************************************************
	OLED_ConfigAddressModeDefType: 配置显示的地址递增模式
	参数：
	mode = 
	HorizontalWrapMode	//水平递增自动换行模式
	VerticalWrapMode	//垂直递增自动换行模式
	PageMode		//水平递增模式（不换行）
***********************************************************************/
void OLED_ConfigAddressMode(OLED_AddressModeDefType mode)
{
     OLED_SendCmd(0x20);
     OLED_SendCmd(mode);
	OLED_AddressMode = mode;
	
	if(OLED_AddressMode != PageMode)
	{
		OLED_FixArea(0, 7, 0, 127);
	}
}
/*
	OLED_FixPositionDefType : 在Page addressing mode下定位显示位置
	参数:
	row = 0 ~ 7
	colume = 0 ~ 127
*/
void OLED_FixPosition(unsigned char row, unsigned char column)
{
	if(OLED_AddressMode != PageMode)
	{
		OLED_ConfigAddressMode(PageMode);	
	}

	OLED_SendCmd(0xB0 | (row%8));
	OLED_SendCmd(column & 0x0F);
	OLED_SendCmd((column>>4)|0x10);
	
	OLED_Position.row = row;
	OLED_Position.column = column;
}
/*
OLED_FixAreaDefType : 在 Horizontal 或者 Vertical模式下设置将要显示的区域
参数：
startRow， endRow = 0 ~ 7		(startRow <= endRow)
startColume, endColume = 0 ~ 127	(startColume <= endColume) 
*/
void OLED_FixArea(unsigned char startRow, unsigned char endRow, unsigned char startColumn, unsigned char endColumn)
{
	if(OLED_AddressMode == PageMode)
	{
		OLED_ConfigAddressMode(HorizontalWrapMode);
	}

	OLED_SendCmd(0x22);
	OLED_SendCmd(startRow);
	OLED_SendCmd(endRow);
	
	OLED_SendCmd(0x21);
	OLED_SendCmd(startColumn);
	OLED_SendCmd(endColumn);
	
	OLED_Area.startRow = startRow;
	OLED_Area.endRow = endRow;
	OLED_Area.startColumn = startColumn;
	OLED_Area.endColumn = endColumn;
}


void OLED_SSD1306_Init()
{
	RST_ACT;
	delay_us(10);
	RST_DEACT;
	
	OLED_SendCmd(0xAE);   //display off
	OLED_SendCmd(0x20);	//Set Memory Addressing Mode	
	OLED_SendCmd(0x10);	//00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
	OLED_SendCmd(0xb0);	//Set Page Start Address for Page Addressing Mode,0-7
	OLED_SendCmd(0xc8);	//Set COM Output Scan Direction
	OLED_SendCmd(0x00);//---set low column address
	OLED_SendCmd(0x10);//---set high column address
	OLED_SendCmd(0x40);//--set start line address
	OLED_SendCmd(0x81);//--set contrast control register
	OLED_SendCmd(0x7f);
	OLED_SendCmd(0xa1);//--set segment re-map 0 to 127
	OLED_SendCmd(0xa6);//--set normal display
	OLED_SendCmd(0xa8);//--set multiplex ratio(1 to 64)
	OLED_SendCmd(0x3F);//
	OLED_SendCmd(0xa4);//0xa4,Output follows RAM content;0xa5,Output ignores RAM content
	OLED_SendCmd(0xd3);//-set display offset
	OLED_SendCmd(0x00);//-not offset
	OLED_SendCmd(0xd5);//--set display clock divide ratio/oscillator frequency
	OLED_SendCmd(0xf0);//--set divide ratio
	OLED_SendCmd(0xd9);//--set pre-charge period
	OLED_SendCmd(0x22); //
	OLED_SendCmd(0xda);//--set com pins hardware configuration
	OLED_SendCmd(0x12);
	OLED_SendCmd(0xdb);//--set vcomh
	OLED_SendCmd(0x20);//0x20,0.77xVcc
	OLED_SendCmd(0x8d);//--set DC-DC enable
	OLED_SendCmd(0x14);//
	OLED_SendCmd(0xaf);//--turn on oled panel 
}

/*
OLED_Clear : 为GDDRAM中填满0
注意： 此函数会自动将地址模式改为 HorizontalWrapMode
*/
void OLED_Clear()
{
	int ci ;
	if(OLED_AddressMode != HorizontalWrapMode)
		OLED_ConfigAddressMode(HorizontalWrapMode);
	
	OLED_FixArea(0, 7, 0, 127);
	
	ci = 128 * 64;
	
	while(ci--)
		OLED_SendData(0);
}

void OLED_PutChar(char c)
{
	int i;
	c = c - ' ';
	
	for(i = 0; i < 6; i ++)
		OLED_SendData(fonts[c][i]);
}

void OLED_PutString(char *str)
{
	if(OLED_AddressMode == PageMode)
	{
		OLED_PositionDefType OLED_PositionDefType = OLED_Position;
	
		while(*str)
		{
			if((OLED_AutoWrap && OLED_PositionDefType.column + 6 > 127) || *str == '\n')
			{
				OLED_PositionDefType.row += 1;
				OLED_PositionDefType.column = 0;
				
				if(OLED_PositionDefType.row == 8)
				{
					OLED_PositionDefType.row = 0;
					
				}
					
				OLED_FixPosition(OLED_PositionDefType.row, OLED_PositionDefType.column);
			}
			OLED_PutChar(*str++);
			OLED_PositionDefType.column += 6;
		}
	}
	
	else if(OLED_AddressMode == HorizontalWrapMode)
	{
		OLED_AreaDefType backOLED_AreaDefType = OLED_Area;
		OLED_AreaDefType OLED_AreaDefType = OLED_Area;
		
		while(*str)
		{
			if((OLED_AutoWrap && OLED_AreaDefType.startColumn + 6 > OLED_AreaDefType.endColumn) || *str == '\n')
			{
				OLED_AreaDefType.startRow += 1;
				OLED_AreaDefType.startColumn = OLED_AreaDefType.startColumn;
				
				if(OLED_AreaDefType.startRow == OLED_AreaDefType.endRow + 1)
					OLED_AreaDefType = backOLED_AreaDefType;
				
				OLED_FixArea(OLED_AreaDefType.startRow, OLED_AreaDefType.endRow, OLED_AreaDefType.startColumn, OLED_AreaDefType.endColumn);
			}
			
			OLED_PutChar(*str++);
			OLED_AreaDefType.startColumn += 6;
		}
	}
	else
	{
		//VerticalWrapMode暂时还不支持
	}
	
}

/* * *
	* 显示一行文字
	* 每调用一次,显示的文字会落在最后显示的那一行之后。	
	*/
void OLED_PushLine(char *format, ...)
{
	static int line = 1;
	unsigned int displayCharCount;
	va_list list;
	
	if(line >= 8)
	{
		OLED_Clear();
		line = 1;
	}
	
	OLED_FixPosition(line, 0);
	va_start(list, format);
	displayCharCount = vsprintf(buffer, format, list);
	va_end(list);
	OLED_PutString(buffer);
	
	line += displayCharCount * 7 / 128 + 1;
}

void OLED_PutFormat(unsigned char row, unsigned char col, char *format, ...)
{
	va_list list;
	va_start(list, format);
	vsprintf(buffer, format, list);
	va_end(list);
	
	if(OLED_AddressMode != PageMode)
		OLED_ConfigAddressMode(PageMode);
	
	OLED_FixPosition(row, col);
	OLED_PutString(buffer);
}

void OLED_PlaceFormat(unsigned char row, float OLED_PositionDefType, char *format, ...)
{
     unsigned char len;
     va_list list;
     va_start(list, format);
     len = vsprintf(buffer, format, list);
     va_end(list);
     
     OLED_FixPosition(row, (unsigned char)(127 * OLED_PositionDefType - len * 3));
     OLED_PutString(buffer);
}

char* OLED_GetFormat(char *format, ...)
{
     va_list args;
     
     va_start(args, format);
     
     vsprintf(buffer, format, args);
     
     va_end(args);
     
     return buffer;
}



void OLED_DrawBMP(unsigned char startRow, unsigned char endRow, unsigned char startCol, unsigned char endCol, const unsigned char datas[])
{
	int i;
	unsigned int count;
	OLED_ConfigAddressMode(HorizontalWrapMode);

	OLED_FixArea(startRow, endRow, startCol, endCol);
	
	count = (endRow - startRow + 1) * (endCol - startCol + 1);
	
	for(i = 0; i < count; i ++)
		OLED_SendData(datas[i]);
}

void test()
{
	OLED_ConfigAddressMode(HorizontalWrapMode);
	
	OLED_FixArea(1, 5, 30, 80);
	
	OLED_PutString("len\nfien");
}













static const unsigned char fonts[][6] = 
{
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   //sp0
	{ 0x00, 0x00, 0x00, 0x2f, 0x00, 0x00 },   // !1
	{ 0x00, 0x00, 0x07, 0x00, 0x07, 0x00 },   // "2
	{ 0x00, 0x14, 0x7f, 0x14, 0x7f, 0x14 },   // #3
	{ 0x00, 0x24, 0x2a, 0x7f, 0x2a, 0x12 },   // $4
	{ 0x00, 0x62, 0x64, 0x08, 0x13, 0x23 },   // %5
	{ 0x00, 0x36, 0x49, 0x55, 0x22, 0x50 },   // &6
	{ 0x00, 0x00, 0x05, 0x03, 0x00, 0x00 },   // '7
	{ 0x00, 0x00, 0x1c, 0x22, 0x41, 0x00 },   // (8
	{ 0x00, 0x00, 0x41, 0x22, 0x1c, 0x00 },   // )9
	{ 0x00, 0x14, 0x08, 0x3E, 0x08, 0x14 },   // *10
	{ 0x00, 0x08, 0x08, 0x3E, 0x08, 0x08 },   // +11
	{ 0x00, 0x00, 0x00, 0xA0, 0x60, 0x00 },   // ,12
	{ 0x00, 0x08, 0x08, 0x08, 0x08, 0x08 },   // -13
	{ 0x00, 0x00, 0x60, 0x60, 0x00, 0x00 },   // .14
	{ 0x00, 0x20, 0x10, 0x08, 0x04, 0x02 },   // /15
	{ 0x00, 0x3E, 0x51, 0x49, 0x45, 0x3E },   // 016
	{ 0x00, 0x00, 0x42, 0x7F, 0x40, 0x00 },   // 117
	{ 0x00, 0x42, 0x61, 0x51, 0x49, 0x46 },   // 218
	{ 0x00, 0x21, 0x41, 0x45, 0x4B, 0x31 },   // 319
	{ 0x00, 0x18, 0x14, 0x12, 0x7F, 0x10 },   // 420
	{ 0x00, 0x27, 0x45, 0x45, 0x45, 0x39 },   // 521
	{ 0x00, 0x3C, 0x4A, 0x49, 0x49, 0x30 },   // 622
	{ 0x00, 0x01, 0x71, 0x09, 0x05, 0x03 },   // 723
	{ 0x00, 0x36, 0x49, 0x49, 0x49, 0x36 },   // 824
	{ 0x00, 0x06, 0x49, 0x49, 0x29, 0x1E },   // 925
	{ 0x00, 0x00, 0x36, 0x36, 0x00, 0x00 },   // :26
	{ 0x00, 0x00, 0x56, 0x36, 0x00, 0x00 },   // ;27
	{ 0x00, 0x08, 0x14, 0x22, 0x41, 0x00 },   // <28
	{ 0x00, 0x14, 0x14, 0x14, 0x14, 0x14 },   // =29
	{ 0x00, 0x00, 0x41, 0x22, 0x14, 0x08 },   // >30
	{ 0x00, 0x02, 0x01, 0x51, 0x09, 0x06 },   // ?31
	{ 0x00, 0x32, 0x49, 0x59, 0x51, 0x3E },   // @32
	{ 0x00, 0x7C, 0x12, 0x11, 0x12, 0x7C },   // A33
	{ 0x00, 0x7F, 0x49, 0x49, 0x49, 0x36 },   // B34
	{ 0x00, 0x3E, 0x41, 0x41, 0x41, 0x22 },   // C35
	{ 0x00, 0x7F, 0x41, 0x41, 0x22, 0x1C },   // D36
	{ 0x00, 0x7F, 0x49, 0x49, 0x49, 0x41 },   // E37
	{ 0x00, 0x7F, 0x09, 0x09, 0x09, 0x01 },   // F38
	{ 0x00, 0x3E, 0x41, 0x49, 0x49, 0x7A },   // G39
	{ 0x00, 0x7F, 0x08, 0x08, 0x08, 0x7F },   // H40
	{ 0x00, 0x00, 0x41, 0x7F, 0x41, 0x00 },   // I41
	{ 0x00, 0x20, 0x40, 0x41, 0x3F, 0x01 },   // J42
	{ 0x00, 0x7F, 0x08, 0x14, 0x22, 0x41 },   // K43
	{ 0x00, 0x7F, 0x40, 0x40, 0x40, 0x40 },   // L44
	{ 0x00, 0x7F, 0x02, 0x0C, 0x02, 0x7F },   // M45
	{ 0x00, 0x7F, 0x04, 0x08, 0x10, 0x7F },   // N46
	{ 0x00, 0x3E, 0x41, 0x41, 0x41, 0x3E },   // O47
	{ 0x00, 0x7F, 0x09, 0x09, 0x09, 0x06 },   // P48
	{ 0x00, 0x3E, 0x41, 0x51, 0x21, 0x5E },   // Q49
	{ 0x00, 0x7F, 0x09, 0x19, 0x29, 0x46 },   // R50
	{ 0x00, 0x46, 0x49, 0x49, 0x49, 0x31 },   // S51
	{ 0x00, 0x01, 0x01, 0x7F, 0x01, 0x01 },   // T52
	{ 0x00, 0x3F, 0x40, 0x40, 0x40, 0x3F },   // U53
	{ 0x00, 0x1F, 0x20, 0x40, 0x20, 0x1F },   // V54
	{ 0x00, 0x3F, 0x40, 0x38, 0x40, 0x3F },   // W55
	{ 0x00, 0x63, 0x14, 0x08, 0x14, 0x63 },   // X56
	{ 0x00, 0x07, 0x08, 0x70, 0x08, 0x07 },   // Y57
	{ 0x00, 0x61, 0x51, 0x49, 0x45, 0x43 },   // Z58
	{ 0x00, 0x00, 0x7F, 0x41, 0x41, 0x00 },   // [59
	{ 0x00, 0x02, 0x04, 0x08, 0x10, 0x20 },   // \60
	{ 0x00, 0x00, 0x41, 0x41, 0x7F, 0x00 },   // ]61
	{ 0x00, 0x04, 0x02, 0x01, 0x02, 0x04 },   // ^62
	{ 0x00, 0x40, 0x40, 0x40, 0x40, 0x40 },   // _63
	{ 0x00, 0x00, 0x01, 0x02, 0x04, 0x00 },   // '64
	{ 0x00, 0x20, 0x54, 0x54, 0x54, 0x78 },   // a65
	{ 0x00, 0x7F, 0x48, 0x44, 0x44, 0x38 },   // b66
	{ 0x00, 0x38, 0x44, 0x44, 0x44, 0x20 },   // c67
	{ 0x00, 0x38, 0x44, 0x44, 0x48, 0x7F },   // d68
	{ 0x00, 0x38, 0x54, 0x54, 0x54, 0x18 },   // e69
	{ 0x00, 0x08, 0x7E, 0x09, 0x01, 0x02 },   // f70
	{ 0x00, 0x18, 0xA4, 0xA4, 0xA4, 0x7C },   // g71
	{ 0x00, 0x7F, 0x08, 0x04, 0x04, 0x78 },   // h72
	{ 0x00, 0x00, 0x44, 0x7D, 0x40, 0x00 },   // i73
	{ 0x00, 0x40, 0x80, 0x84, 0x7D, 0x00 },   // j74
	{ 0x00, 0x7F, 0x10, 0x28, 0x44, 0x00 },   // k75
	{ 0x00, 0x00, 0x41, 0x7F, 0x40, 0x00 },   // l76
	{ 0x00, 0x7C, 0x04, 0x18, 0x04, 0x78 },   // m77
	{ 0x00, 0x7C, 0x08, 0x04, 0x04, 0x78 },   // n78
	{ 0x00, 0x38, 0x44, 0x44, 0x44, 0x38 },   // o79
	{ 0x00, 0xFC, 0x24, 0x24, 0x24, 0x18 },   // p80
	{ 0x00, 0x18, 0x24, 0x24, 0x18, 0xFC },   // q81
	{ 0x00, 0x7C, 0x08, 0x04, 0x04, 0x08 },   // r82
	{ 0x00, 0x48, 0x54, 0x54, 0x54, 0x20 },   // s83
	{ 0x00, 0x04, 0x3F, 0x44, 0x40, 0x20 },   // t84
	{ 0x00, 0x3C, 0x40, 0x40, 0x20, 0x7C },   // u85
	{ 0x00, 0x1C, 0x20, 0x40, 0x20, 0x1C },   // v86
	{ 0x00, 0x3C, 0x40, 0x30, 0x40, 0x3C },   // w87
	{ 0x00, 0x44, 0x28, 0x10, 0x28, 0x44 },   // x88
	{ 0x00, 0x1C, 0xA0, 0xA0, 0xA0, 0x7C },   // y89
	{ 0x00, 0x44, 0x64, 0x54, 0x4C, 0x44 },   // z90
	{ 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 },   // horiz lines91
    { 0x80, 0x80, 0x80, 0x80, 0x80, 0x80 },   //上划线 92
    { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },   //____下划线 93
    { 0x08, 0x08, 0x08, 0x08, 0x08, 0x08 }    //中划线 94
};

