#ifndef __OLED
#define __OLED


/*
����������
*/
typedef enum 
{
	RightHorizon = 0x26, 
	LeftHorizon = 0x27, 
	RightHorizonWithVertical = 0x29,
	LeftHorizonWithVertical = 0x2A
}OLED_ScrollDirectionDefType;

typedef enum
{
	FALSE = 0, 
	TRUE = !FALSE
}bool;

/*
	����ʱ��������
*/
typedef enum 
{
	Frames_2 = 0x07, 	
	Frames_3 = 0x04, 
	Frames_4 = 0x05, 
	Frames_5 = 0x00, 	
	Frames_25 = 0x06, 
	Frames_64 = 0x01,
	Frames_128 = 0x02, 
	Frames_256 = 0x03
}OLED_ScrollInternalFramesDefType;

/*
��ַ����ģʽ����
*/
typedef enum 
{
	HorizontalWrapMode = 0x0,	//ˮƽ�����Զ�����ģʽ
	VerticalWrapMode = 0x01,	//��ֱ�����Զ�����ģʽ
	PageMode = 0x2			//ˮƽ����ģʽ�������У�
}OLED_AddressModeDefType;

typedef struct 
{
	unsigned char row; 
	unsigned char column;
}OLED_PositionDefType;

typedef struct
{
	unsigned char startRow;
	unsigned char endRow;
	unsigned char startColumn;
	unsigned char endColumn;
}OLED_AreaDefType;

//��ǰ�ĵ�ַ����ģʽ
extern 	OLED_AddressModeDefType 	OLED_AddressMode;
extern 	OLED_PositionDefType 		OLED_Position;
extern 	OLED_AreaDefType			OLED_Area;
extern 	unsigned char 				OLED_CurrentContrast;
extern 	bool 						OLED_AutoWrap;				//��PageMode���Ƿ��������Զ�����

void OLED_Init(void);
void OLED_SetContrast(unsigned char contrast);
void OLED_InverseDisplay(bool inverse);	//GDDRAM��
void OLED_Sleep(bool OLED_Sleep);
void OLED_DisplayOn(void);
void OLED_ConfigScroll(OLED_ScrollDirectionDefType dir, unsigned char startPage, unsigned char endPage, OLED_ScrollInternalFramesDefType frames, unsigned char scrollRangeRowStart, unsigned char scrollRangeRowEnd, unsigned char verticalOffset);
void OLED_ActiveScroll(bool active);
void OLED_ConfigAddressMode(OLED_AddressModeDefType mode);
void OLED_FixPosition(unsigned char row, unsigned char colume);
void OLED_FixArea(unsigned char startRow, unsigned char endRow , unsigned char startColume, unsigned char endColume);
void OLED_PushLine(char *format, ...);
void OLED_Clear(void);
void OLED_PutChar(char c);
void OLED_PutString(char *str);
void OLED_PutFormat(unsigned char row, unsigned char column, char *format, ...);
void OLED_PlaceFormat(unsigned char row, float OLED_PositionDefType, char *format, ...);
void OLED_DrawBMP(unsigned char startRow, unsigned char endRow, unsigned char startCol, unsigned char endCol, const unsigned char datas[]);
#endif
