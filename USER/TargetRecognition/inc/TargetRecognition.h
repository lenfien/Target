#ifndef __TARGET_RECOGNITION
#define __TARGET_RECOGNITION


#define DRAW_POS(x, y)			(LCD->LCD_REG = 0x21, LCD->LCD_RAM = x, LCD->LCD_REG = 0x20, LCD->LCD_RAM = y)
#define DRAW_PRE				LCD->LCD_REG = 0x22
#define DRAW(c)					LCD->LCD_RAM = c			

#define RGB(r, g, b)			((r&0x1F) << 11 | (g&0x3F) << 5 | (b&0x1F) )

#define R(c)					((c>>11) & 0x1F)
#define G(c)					((c >> 5) & 0x3F)
#define B(c)					(c & 0x1F)


/***************************************************************************************/
/*-------------------------Private Type Definition Begin Here ------------------------*/

typedef struct
{
	signed short x; 
	signed short y;
}PointType;


typedef struct
{
	unsigned short XStart;
	unsigned short YStart;
	unsigned short XEnd;
	unsigned short YEnd;
}TargetRegionType;


typedef struct
{
	signed char MaxRed;
	signed char MinRed;
	
	signed char MaxGreen;
	signed char MinGreen;
	
	signed char MaxBlue;
	signed char MinBlue;
}ThreadType;



typedef enum
{
	WORK_MODE = 0,
	DEBUG_MODE = 1
}RunModeType;

typedef enum
{
	DEBUG_ADJUST_MODE = 0,
	DEBUG_VIDEO_MODE = 1
}DebugModeType;



typedef enum
{
	RIGHT,
	RIGHTUP,
	UP,
	LEFTUP,
	LEFT,
	LEFTDOWN,
	DOWN,
	RIGHTDOWN
}TargetDirectionType;

typedef struct
{
	TargetDirectionType Dir;
	float				AngleMin;
	float 				AngleMax;
	char				*Text;
}TargetDirectionInfoType;

typedef struct
{
	unsigned short 			 cylinderCount;
	TargetDirectionInfoType* Direction;
}TargetInfoType;


/*
	The pixel state defined
*/
typedef enum
{
	BACKGROUND = 0,		//background
	LINE = 1,			//line
	TARGET = 2,			//target
	OTHER = 3			//others
}PixelStateType;


typedef struct
{
	unsigned short 	Radius;
	PointType		Center;
}CircleShapeType;

/*----------------------------Private Type Definition End Here-------------------------*/
/***************************************************************************************/

extern RunModeType 		RunMode;
extern DebugModeType 	DebugMode;
extern PointType		SamplePoint;

void 			TR_Init(void);
void 			TR_Test(void);
unsigned short 	TR_TakePoint(unsigned short rgb,  unsigned short x, unsigned short y);
unsigned short 	TR_TouchHandler(void);
#endif
