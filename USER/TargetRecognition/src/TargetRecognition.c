
#include <string.h>
#include <math.h>

#include "ov7670.h"
#include "display.h"
#include "TargetRecognition.h"
#include "button.h"
#include "PixelData.h"
#include "delay.h"

/***************************************************************************************/
/*----------------------------Private Globle variable Start Here-----------------------*/


#define BACKGROUND_THREAD_MIN_RED_DEFAULT		0x1F
#define BACKGROUND_THREAD_MIN_GREEN_DEFAULT		0x3F
#define BACKGROUND_THREAD_MIN_BLUE_DEFAULT		0x1F



									//-MAX--MIN---//	
ThreadType BackgroundThreadStruct = { 0x1F, BACKGROUND_THREAD_MIN_RED_DEFAULT,		//RED 
									  0x3F, BACKGROUND_THREAD_MIN_GREEN_DEFAULT, 	//GREEN
									  0x1F, BACKGROUND_THREAD_MIN_BLUE_DEFAULT		//BLUE
};

unsigned short BackgroundThreadMinRedDefault = 0x1F;
unsigned short BackgroundThreadMinGreenDefault = 0x3F;
unsigned short BackgroundThreadMinBlueDefault = 0x1F;

unsigned short BackgroundThreadMinRedMax = 0x1F;
unsigned short BackgroundThreadMinRedMin = 0;
unsigned short BackgroundThreadMinBlueMax = 0x1F;
unsigned short BackgroundThreadMinBlueMin = 0;
unsigned short BackgroundThreadMinGreenMax = 0x3F;
unsigned short BackgroundThreadMinGreenMin = 0;

signed short BackgroundThreadRedOffset = 0;
signed short BackgroundThreadBlueOffset = 0;

ThreadType LineThreadStruct	= { 0x09, 0x00,
								0x12, 0x00,
								0x09, 0x00
};

#define BACKGROUND_THREAD_MIN_RED_WITH_OFFSET		(BackgroundThreadStruct.MinRed + BackgroundThreadRedOffset)
#define BACKGROUND_THREAD_MIN_BLUE_WITH_OFFSET		(BackgroundThreadStruct.MinBlue + BackgroundThreadBlueOffset)


#define IS_IN_THREAD(rgb, thread)		(R(rgb) >= BACKGROUND_THREAD_MIN_RED_WITH_OFFSET ) && (B(rgb) >= BACKGROUND_THREAD_MIN_BLUE_WITH_OFFSET) 

#define MAX(x, y)	((x) > (y) ? (x) : (y))
#define MIN(x, y)	((x) < (y) ? (x) : (y))

RunModeType 						RunMode = DEBUG_MODE;	  							//WorkingMode : 0:Normal Mode 1:Debug Mode 2:Video Mode
DebugModeType						DebugMode = DEBUG_ADJUST_MODE;
unsigned short 						PixelStatesColor[5]	= {RED, BLUE, YELLOW, BLACK};	//The Colors correspond diffirent pixel state. 0: BACKGROUND 1:LINE 2:TARGET 0:OTHER
unsigned short						ColorsInHorizontalMiddle[320];						//colors store the horizontal middle line is used to thread amending in TR_ThreadAmend function.
unsigned char 						FourStateArray[19200];								//pixel state of current frame stored here

//Write a pixel state to FourStateArray
#define RECORD_PIXEL(x, y, p)		FourStateArray[((y) * 80) + ((x) / 4)] = (FourStateArray[((y) * 80) + ((x) / 4)] & (~(0x3 << ((3 - (x)%4) << 1)))) | (((p)&3) << ((3 - (x)%4) << 1))	
//read a pixel state from FourStateArray
#define READ_PIXEL(x, y)			(PixelStateType)((FourStateArray[((y) * 80) + ((x) / 4)] >> ((3 - (x)%4) << 1)) & 0x03)	

TargetRegionType 					TargetRegion; //define and store the target region
//test whether point (x, y) is in Target Region defined in TargetRegion
#define IS_IN_REGION(x, y)			((x) > TargetRegion.XStart && (x) < TargetRegion.XEnd) && ((y) > TargetRegion.YStart && (y) < TargetRegion.YEnd)	

#define CIRCLE_AMOUNT	6	
#define CIRCLE_AMOUNT_MAX	10								//define the maxmum amount of circle shape
unsigned short		CircleShapeAmount = 0;					//amount of circle shapes in the target region 
CircleShapeType 	CircleShapes[CIRCLE_AMOUNT_MAX];		//stores the circle shapes in the target region
PointType			TargetPoint;

PointType 		SamplePoint = {160, 120};	//Indicates the sample point when simple function enabled
unsigned short 	SampleColor = 0;			//Defining the color displayed on the TargetRegion to indicates the sample point when simple function enabled.
unsigned short 	XCross; 					//the middle of target region in horizontal, updated by TR_CircleDetect
unsigned short 	YCross;						//the middle of target region in vertical, updated by TR_CircleDetect

unsigned char 	IsCirclesReady = 0;			//indicate circles in the CircleShapes is ready

/*----------------------------Private Globle variable End Here-------------------------*/
/***************************************************************************************/



/*****************************************************************************************/
/*----------------------------Private Function Declaration Start Here--------------------*/
signed char 		TR_TargetRegionDetect(void);
void 						TR_NoiseExclude(void);
unsigned char 	TR_BorderFix(void);
signed char	 		TR_CircleDetect(void);
unsigned char 	TR_TargetDetect(void);
signed char 		TR_ThreadRedChannelAmend(void);
signed char 		TR_ThreadBlueChannelAmend(void);
signed char 		TR_ThreadGreenChannelAmend(void);
unsigned int 		TR_NoiseDetect(void);
void 						TR_DisplayWholeFrame(void);
signed char 		TR_BorderFix_v2(void);
unsigned char 	TR_AnalyseTarget(TargetInfoType* targetInfo);
void 						TR_DrawTarget(void);
void 						TR_DisplayNumbers(unsigned long int number, unsigned short xSize, unsigned short ySize , unsigned short x, unsigned short y, unsigned short color);
void 						TR_DrawLine(PointType* pStart, PointType* pEnd, short thickness, unsigned short color);
/*----------------------------Private Function Declaration End Here----------------------*/
/*****************************************************************************************/

#define SHOW_BORDER
#define SHOW_CROSS
#define SHOW_CIRCLE_TANGENT
//#define SHOW_SAMPLE_POINT

#define DISPLAY_XSTART	0
#define DISPLAY_XEND	320
#define DISPLAY_YSTART	0
#define DISPLAY_YEND	220

#define DISPLAY_WIDTH	(DISPLAY_XEND - DISPLAY_XSTART)
#define DISPLAY_HEIGHT	(DISPLAY_YEND - DISPLAY_YSTART)

//#define TARGET_CENTER_AUTO_DETECT_ON					//uncomment this macro to enable Target Center Auto Detect  

#define IS_ON_VERTICAL_TANGENT_OF_CIRCLE(x, circle)		((x) == (circle.Center.x - circle.Radius) || (x) == (circle.Center.x + circle.Radius))
#define IS_ON_HORIZONTAL_TANGENT_OF_CIRCLE(y, circle)	((y) == (circle.Center.y - circle.Radius) || (y) == (circle.Center.y + circle.Radius))

/**
  *@Brief Display current frame by rules defined uppon
  *
  **/
	
void TR_DisplayFrame()
{
	unsigned short x, y;
	
	if(RunMode != DEBUG_MODE)
		return;
	
	LCD_SetCursor(0, 0); 
	LCD_WriteRAM_Prepare();
	
	for (y = DISPLAY_YSTART; y < DISPLAY_YEND; y ++)
	{
		for (x = DISPLAY_XSTART; x < DISPLAY_XEND; x ++)
		{
#ifdef SHOW_CROSS
			if (x == XCross || y == YCross)
				LCD_WR_DATA(WHITE);
			else
#endif

#ifdef	SHOW_SAMPLE_POINT
			if (x == SamplePoint.x && y == SamplePoint.y)
			{
				OLED_PutFormat(6, 0, "Smp:%-2x %-2x %-2x", R(SampleColor), G(SampleColor), B(SampleColor));
				LCD_WR_DATA(WHITE);
			}
			else
#endif
			
#ifdef SHOW_BORDER
			if (x == TargetRegion.XStart || x == TargetRegion.XEnd || y == TargetRegion.YStart || y == TargetRegion.YEnd)
				LCD_WR_DATA(GREEN);
			else
#endif		
			{
				
#ifdef SHOW_CIRCLE_TANGENT
				unsigned short index;
				for (index = 0; index < CircleShapeAmount; index++)
				{
					if (IS_ON_HORIZONTAL_TANGENT_OF_CIRCLE(y, CircleShapes[index]) || IS_ON_VERTICAL_TANGENT_OF_CIRCLE(x, CircleShapes[index]))
					{
						LCD_WR_DATA(WHITE);
						break;
					}
				}
				if (index == CircleShapeAmount)
					
#endif
					LCD_WR_DATA(PixelStatesColor[IS_IN_REGION(x, y ) ? READ_PIXEL(x, y) : OTHER]);
			}
		}
	}
}

/**
 *@brief This function do nothing but draw a circle
 *@param circle
 *          the pointer point to circle
 */

#define CIRCLE_COLOR	WHITE

void TR_DrawCirle(CircleShapeType *circle, unsigned short color)
{
	int x, y;
	
	for (x = ((int)circle->Radius) * -1; x < circle->Radius; x ++ )
	{
		y = sqrt(circle->Radius * circle->Radius - x * x);
		LCD_WritePoint(x + circle->Center.x, y + circle->Center.y, color);
		LCD_WritePoint(x + circle->Center.x, - y + circle->Center.y, color);
	}
	
	for (y = ((int)circle->Radius) * -1; y < circle->Radius; y ++ )
	{
		x = sqrt(circle->Radius * circle->Radius - y * y);
		LCD_WritePoint(x + circle->Center.x, y + circle->Center.y, color);
		LCD_WritePoint(-x + circle->Center.x, y + circle->Center.y, color);
	}
}


/**
 *@brief  show a msg to the bottom of the screen
 */
void TR_ShowState(char * msg)
{
	OLED_PushLine(msg);
	LCD_printf(0, 220, "%-40s", msg);
}

/****
	*@Brief  This function inject the Circles stored in the global variate CirleShapes to current frame array named FourStateArray which 
	* 		 is also a globle vairate.
	*@Notice This function mustn't be called until the TR_CircleDetect returns no-zero.
*/
void TR_InjectCircleToArray()
{
	
}

/**
*@brief: This function clear display area defined in DISPLAY_XSTART DISPLAY_XEND DISPLAY_YSTART	DISPLAY_YEND
**/

void TR_ClearLCD(unsigned short color)
{
	int x, y;
	LCD_SetCursor(0, 0);
	LCD_WriteRAM_Prepare();
	
	for (y = DISPLAY_YSTART; y < DISPLAY_YEND; y++)
		for (x = DISPLAY_XSTART; x < DISPLAY_XEND; x++)
			LCD_WR_DATA(color);
}

/**
*@brief: This function draw the target region according to the CircleShapes which should be updated by function TR_CircleDetecte.
**/

#define TARGET_REGION_POSITION_X	110
#define TARGET_REGION_POSITION_Y	110
#define TARGET_REGION_RADIUS		100

#define TARGET_REGION_TEXT_NUMBER_X	 230
#define TARGET_REGION_TEXT_NUMBER_Y  55

#define TARGET_REGION_TEXT_SIZE_X	 35
#define TARGET_REGION_TEXT_SIZE_Y 	 100

#define TARGET_REGION_TEXT_DIRECTION_X 225
#define TARGET_REGION_TEXT_DIRECTION_Y 200

#define TARGET_REGION_TEXT_COLOR	RED
#define TARGET_REGION_TEXT_DIRECTION_COLOR		RED

#define TARGET_REGION_EXTRACTED_BACKCOLOR 		BLACK
#define TARGET_REGION_EXTRACTED_FORECOLOR		GREEN
#define TARGET_REGION_EXTRACTED_TARGET_COLOR	YELLOW

float 		StandardRatio;
PointType	StandardOffset;

CircleShapeType PreCircleShapesBackup[CIRCLE_AMOUNT_MAX];

void TR_DrawTargetRegion()
{
	int index;
	
	for(index = 0; index < CircleShapeAmount; index ++)
	{
		if(PreCircleShapesBackup[index].Center.x != CircleShapes[index].Center.x || PreCircleShapesBackup[index].Center.y != CircleShapes[index].Center.y || PreCircleShapesBackup[index].Radius != CircleShapes[index].Radius)
			break;
	}
	
	if(index == CircleShapeAmount)
		return;
	
	StandardRatio = TARGET_REGION_RADIUS/(float)PreCircleShapesBackup[0].Radius;
	StandardOffset.x = TARGET_REGION_POSITION_X - PreCircleShapesBackup[0].Center.x;
	StandardOffset.y = TARGET_REGION_POSITION_Y - PreCircleShapesBackup[0].Center.y;
	
	for (index = 0; index < CircleShapeAmount; index ++)
	{
		CircleShapeType circle = PreCircleShapesBackup[index];
		circle.Center.x = TARGET_REGION_POSITION_X;
		circle.Center.y = TARGET_REGION_POSITION_Y;
		circle.Radius *= StandardRatio;
		
		TR_DrawCirle(&circle, TARGET_REGION_EXTRACTED_BACKCOLOR);
	}
	
	StandardRatio = TARGET_REGION_RADIUS/(float)CircleShapes[0].Radius;
	StandardOffset.x = TARGET_REGION_POSITION_X - CircleShapes[0].Center.x;
	StandardOffset.y = TARGET_REGION_POSITION_Y - CircleShapes[0].Center.y;
	
	for (index = 0; index < CircleShapeAmount; index++)
	{
		CircleShapeType circle = CircleShapes[index];
		circle.Center.x = TARGET_REGION_POSITION_X;
		circle.Center.y = TARGET_REGION_POSITION_Y;
		circle.Radius *= StandardRatio;
		
		TR_DrawCirle(&circle, TARGET_REGION_EXTRACTED_FORECOLOR);
		
		PreCircleShapesBackup[index] = CircleShapes[index];
	}
}

/**
 *@brief draw the target point and the two direction line arount to indicate the direction of target point
 */
void TR_DrawTarget()
{
	static PointType preLine1End, preLine2End, preCenterPoint;
	static CircleShapeType preCircleCenter;
	static unsigned short PreCylinderBackup;
	
	unsigned short backColor;
	
	TargetInfoType targetInfo;
	
	unsigned short colorBack = POINT_COLOR;
	CircleShapeType circle;
	
	PointType line1End, line2End;
	PointType circlePoint = CircleShapes[0].Center;
	
	float length = CircleShapes[0].Radius;
	
	/*Clear the previous drawing-------------------------------------------*/
	POINT_COLOR = TARGET_REGION_EXTRACTED_BACKCOLOR;
	LCD_DrawLine(preCenterPoint.x, preCenterPoint.y, preLine1End.x, preLine1End.y);
	LCD_DrawLine(preCenterPoint.x, preCenterPoint.y, preLine2End.x, preLine2End.y);
	POINT_COLOR = colorBack;
	
	preCircleCenter.Radius = 3;
	TR_DrawCirle(&preCircleCenter, TARGET_REGION_EXTRACTED_BACKCOLOR);
	preCircleCenter.Radius = 2;
	TR_DrawCirle(&preCircleCenter, TARGET_REGION_EXTRACTED_BACKCOLOR);
	preCircleCenter.Radius = 1;
	TR_DrawCirle(&preCircleCenter, TARGET_REGION_EXTRACTED_BACKCOLOR);
	
	TR_DisplayNumbers(PreCylinderBackup, TARGET_REGION_TEXT_SIZE_X, TARGET_REGION_TEXT_SIZE_Y, TARGET_REGION_TEXT_NUMBER_X, TARGET_REGION_TEXT_NUMBER_Y, TARGET_REGION_EXTRACTED_BACKCOLOR);
	
	/*Start drawing the new------------------------------------------------*/
	if(0 == TR_AnalyseTarget(&targetInfo))
		return;
	
	circlePoint.x += StandardOffset.x;
	circlePoint.y += StandardOffset.y;
	
	length *= StandardRatio;
	
	line1End.x = cos(targetInfo.Direction->AngleMin/180*3.1415926) * length + circlePoint.x;
	line1End.y = sin(targetInfo.Direction->AngleMin/180*3.1415926) * length + circlePoint.y;
	
	line2End.x = cos(targetInfo.Direction->AngleMax/180*3.1415926) * length + circlePoint.x;
	line2End.y = sin(targetInfo.Direction->AngleMax/180*3.1415926) * length + circlePoint.y;
	
	POINT_COLOR = YELLOW;
	LCD_DrawLine(circlePoint.x, circlePoint.y, line1End.x, line1End.y);
	LCD_DrawLine(circlePoint.x, circlePoint.y, line2End.x, line2End.y);
	POINT_COLOR = colorBack;
	preCenterPoint = circlePoint;
	
	circle.Center = TargetPoint;
	circle.Center.x = StandardRatio * (circle.Center.x - CircleShapes[0].Center.x) + CircleShapes[0].Center.x;
	circle.Center.y = StandardRatio * (circle.Center.y - CircleShapes[0].Center.y) + CircleShapes[0].Center.y;
	
	circle.Center.x += StandardOffset.x;
	circle.Center.y += StandardOffset.y;
	circle.Radius = 3;
	TR_DrawCirle(&circle, WHITE);
	circle.Radius = 2;
	TR_DrawCirle(&circle, WHITE);
	circle.Radius = 1;
	TR_DrawCirle(&circle, WHITE);
	
	preLine1End = line1End;
	preLine2End = line2End;
	preCircleCenter = circle;
	PreCylinderBackup = targetInfo.cylinderCount;
	
	TR_DisplayNumbers(targetInfo.cylinderCount, TARGET_REGION_TEXT_SIZE_X, TARGET_REGION_TEXT_SIZE_Y, TARGET_REGION_TEXT_NUMBER_X, TARGET_REGION_TEXT_NUMBER_Y, TARGET_REGION_TEXT_COLOR);
	backColor = LCD_SetFontColor(TARGET_REGION_TEXT_DIRECTION_COLOR);
	LCD_printf(TARGET_REGION_TEXT_DIRECTION_X, TARGET_REGION_TEXT_DIRECTION_Y, "%-10s", targetInfo.Direction->Text);
	LCD_SetFontColor(backColor);
}

#define WORK_MODE_LINE_COLOR	GREEN

void TR_DrawWorkModeUI()
{
	unsigned short backColor;
	PointType p1, p2;
	p1.x = 0;
	p1.y = 220;
	p2.x = 319;
	p2.y = 220;
	
	TR_DrawLine(&p1, &p2, 1, WORK_MODE_LINE_COLOR);
	
	p1.x = 220;
	p1.y = 0;
	p2.x = 220;
	p2.y = 220;
	
	TR_DrawLine(&p1, &p2, 2, WORK_MODE_LINE_COLOR);
	
	p1.x = 220;
	p1.y = 20;
	p2.x = 319;
	p2.y = 20;
	
	TR_DrawLine(&p1, &p2, 1, WORK_MODE_LINE_COLOR);
	
	p1.x = 220;
	p1.y = 190;
	p2.x = 319;
	p2.y = 190;
	
	TR_DrawLine(&p1, &p2, 1, WORK_MODE_LINE_COLOR);
	
	backColor = LCD_SetFontColor(GREEN);
	LCD_printf(225, 0, "Cylinder:");
	LCD_SetFontColor(backColor);
	

	
}

/*The arrays stores the stroke infomation of numbers 0 - 9, the first element per array is the sum count of point which means 
there has count * 2 + 1 number of elements per array*/

float Number0[] = {5, 	0,		0, 		1,		0, 		1,		1, 			0,		1, 		0,		0};
float Number1[] = {2, 	0.5, 	0, 		0.5, 	1};
float Number2[] = {6, 	0,		0, 		1, 		0, 		1, 		0.5, 		0, 		0.5, 	0, 	1, 		1, 	1};
float Number3[]	= {7, 	0, 		0, 		1, 		0, 		1, 		0.5, 		0, 		0.5, 	1, 	0.5, 	1, 	1, 		0, 		1};
float Number4[]	= {5, 	0, 		0, 		0, 		0.5, 	1, 		0.5, 		1, 		0, 		1, 	1};
float Number5[] = {6, 	1, 		0, 		0, 		0, 		0, 		0.5, 		1, 	0.5, 	1, 	1, 		0, 	1};
float Number6[] = {6, 	1, 		0, 		0, 		0, 		0, 		1, 			1, 		1, 		1, 	0.5, 	0, 	0.5};
float Number7[] = {3, 	0, 		0, 		1, 		0, 		1, 		1};
float Number8[] = {7, 	1, 		0.5, 	1, 		0, 		0, 		0, 			0, 		1, 		1, 	1, 		1, 	0.5, 	0, 		0.5};
float Number9[] = {6, 	1, 		0.5, 	0, 		0.5, 	0, 		0, 			1, 		0, 		1, 	1, 		0, 	1};

float *(Numbers[]) = {Number0, Number1, Number2, Number3, Number4, Number5, Number6, Number7, Number8, Number9};

#define PI (3.1415926)

void TR_DrawLine(PointType* pStart, PointType* pEnd, short thickness, unsigned short color)
{
	float radian;
	int r;
	unsigned short colorBackup = POINT_COLOR;
	POINT_COLOR = color;
	
	if(pStart->x == pEnd->x)
		radian = PI/2;
	else
	{
		radian = atan( (float)(pStart->y - pEnd->y) / (float)(pStart->x - pEnd->x));
	}
	
	if(radian >= 0)
		radian += PI/2;
	else
		radian -= PI/2;
		
	for(r = -thickness; r <= thickness; r ++)
	{
		int xStart, yStart, xEnd, yEnd;
		
		
		xStart = pStart->x + r * cos(radian);
		yStart = pStart->y + r * sin(radian);
		
		xEnd = pEnd->x + r * cos(radian);
		yEnd = pEnd->y + r * sin(radian);
		
		xStart = MAX(0, xStart);
		yStart = MAX(0, yStart);
		xEnd = MAX(0, xEnd);
		yEnd = MAX(0, yEnd);
		
		xStart = MIN(319, xStart);
		yStart = MIN(220, yStart);
		xEnd = MIN(319, xEnd);
		yEnd = MIN(220, yEnd);
		
		LCD_DrawLine(xStart, yStart, xEnd, yEnd);
	}
	
	POINT_COLOR = colorBackup;
}

/**
  *@brief this function display a number using just line, the number should be in 0 - 9
  */
void TR_DisplayNumber(unsigned short num, unsigned short xSize, unsigned short ySize, unsigned short x, unsigned short y, unsigned short color)
{
	float *pNumber = Numbers[num];
	unsigned short amount = *pNumber;
	unsigned short index;
	
	if(x + xSize > DISPLAY_WIDTH)
		return;
	
	if(y + ySize > DISPLAY_HEIGHT)
		return;
	
	for(index = 1; index <= amount * 2 - 3; index += 2)
	{
		PointType p1, p2;
		
		p1.x = pNumber[index] * xSize + x;
		p1.y = pNumber[index + 1] * ySize + y;
		p2.x = pNumber[index + 2] * xSize + x;
		p2.y = pNumber[index + 3] * ySize + y;
		
		TR_DrawLine(&p1, &p2, 3, color);
	}
}

/**
 *@brief Display numbers in the way digital calculator displays.
*/

void TR_DisplayNumbers(unsigned long int number, unsigned short xSize, unsigned short ySize , unsigned short x, unsigned short y, unsigned short color)
{
	unsigned long int i;
	
	for(i = 10; i <= 1000000000; i*= 10)
	{
		if(number/i == 0)
		{
			i /= 10;
			break;
		}
	}
	
	for(;i > 0; i /= 10)
	{
		unsigned short ii = number/i;
		number = number % i;
		TR_DisplayNumber(ii, xSize, ySize, x, y, color);
		x += (xSize + 10);
	}
}

/**
*@brief	this function just show the whole fram.
**/
void TR_DisplayWholeFrame()
{
	unsigned short x, y;
	
	LCD_SetCursor(0, 0); 
	LCD_WriteRAM_Prepare();
	
	for(y = 0; y < DISPLAY_HEIGHT; y ++)
	{
		for(x = 0; x < DISPLAY_WIDTH; x ++)
		{
			LCD_WR_DATA(PixelStatesColor[READ_PIXEL(x, y)]);
		}
	}
}

/**
  *@Brief: Proccess current frame include Niose-Excluded and so on and display current frame for current work mode -
  *        - defined in global variate WorkMode.
  **/
#define TRY_TIMES	10

void TR_FrameProcess()
{
	static unsigned char 	errorFlag = 0;
	static unsigned char 	startMode = 1;
	static signed 	char 	circleDetectErrorFlag, regionDetectErrorFlag;
	static unsigned short 	flag1 = 0, flag2 = 0;
	static RunModeType 		PreWorkModeBackup = DEBUG_MODE;
	static unsigned short 	tryTimes= 0;	//try times when ajusting for target region detection, the max value is  TRY_TIMES
	static unsigned char 	RefreshScreenRequire = 0;
	
		
	if(startMode)
	{
		TR_DisplayWholeFrame();
		TR_ShowState("Finding target...");
		if(errorFlag)
		{
			BackgroundThreadMinRedDefault = BACKGROUND_THREAD_MIN_RED_DEFAULT;
			BackgroundThreadMinBlueDefault = BACKGROUND_THREAD_MIN_BLUE_DEFAULT;
			
			BackgroundThreadStruct.MinRed = BackgroundThreadMinRedDefault;
			BackgroundThreadStruct.MinBlue = BackgroundThreadMinBlueDefault;
			
			BackgroundThreadRedOffset = 0;
			BackgroundThreadBlueOffset = 0;
			
			errorFlag = 0;
		}
		else if(TR_TargetRegionDetect() != 0)
		{
			if (BackgroundThreadStruct.MinRed == 0 || BackgroundThreadStruct.MinBlue == 0)
			{
				errorFlag = 1;
				TR_ShowState("Starting fails!");
			}
			else
			{
				BackgroundThreadStruct.MinRed -= 1;
				BackgroundThreadStruct.MinBlue -= 1;
				
				OLED_PushLine("Try:%d %d", BackgroundThreadStruct.MinRed, BackgroundThreadStruct.MinBlue);
			}
		}
		else
		{
			OLED_PushLine("Starting succeed!", BackgroundThreadStruct.MinRed, BackgroundThreadStruct.MinBlue);
			LCD_printf(0, 220, "Finding target...Get it!!");
			BackgroundThreadMinRedDefault =  BackgroundThreadStruct.MinRed;
			BackgroundThreadMinBlueDefault = BackgroundThreadStruct.MinBlue;
			
			startMode = 0;
		}
	}
	else
	{
		//TR_DisplayWholeFrame();
		
		if((regionDetectErrorFlag = TR_TargetRegionDetect()) != 0)
		{
			//TR_DisplayWholeFrame();
			
			TR_ShowState("Target Region Lost, Adjusting...");
			
			tryTimes ++;
			
			if(regionDetectErrorFlag == -1)
			{
				BackgroundThreadRedOffset -= 1;
				BackgroundThreadBlueOffset -= 1;
			}
			else
			{
				BackgroundThreadRedOffset += 1;
				BackgroundThreadBlueOffset += 1;
			}
			
			if(BackgroundThreadRedOffset > 0x1F || BackgroundThreadRedOffset < -0x1F || BackgroundThreadBlueOffset > 0x1F || BackgroundThreadBlueOffset < -0x1F || tryTimes == TRY_TIMES)
			{
				RefreshScreenRequire = 1;
				tryTimes = 0;
				errorFlag = 1;
				startMode = 1;
				TR_ShowState("Adjusting Fail, Restart!");
				return;
			}
			
			OLED_PushLine("Adjusting:%d %d",BackgroundThreadRedOffset, BackgroundThreadBlueOffset);
		}
		else
		{
			tryTimes = 0;
			
			OLED_PushLine("Ready. %d %d", BackgroundThreadRedOffset, BackgroundThreadBlueOffset);
			
			TR_ShowState("Ready");
			
			if (!TR_TargetDetect() && (circleDetectErrorFlag = TR_CircleDetect()) != 0)
			{
				IsCirclesReady = 0;
				
				/*extra circle exists, expand range of backgroud--------------------*/
				if (circleDetectErrorFlag == 1)
				{
					if(flag1)
					{
						BackgroundThreadRedOffset --;
						flag1 = 0;
					}
					else
					{
						BackgroundThreadBlueOffset --;
						flag1 = 1;
					}
					OLED_PushLine("Expand:%d %d",BackgroundThreadRedOffset, BackgroundThreadBlueOffset);
				}
				/* circle losts , shrink range of background------------------------*/
				else if(circleDetectErrorFlag == -1)
				{
					if(flag2)
					{
						BackgroundThreadRedOffset ++;
						flag2 = 0;
					}
					else
					{
						BackgroundThreadBlueOffset ++;
						flag2 = 1;
					}
				}
				OLED_PushLine("Shrink:%d %d",BackgroundThreadRedOffset, BackgroundThreadBlueOffset);
			}
			else
			{
				IsCirclesReady = 1;
				
				if (RunMode == DEBUG_MODE && DebugMode == DEBUG_ADJUST_MODE)
				{
					TR_DisplayFrame();
					RefreshScreenRequire = 0;
				}
				else if (RunMode == WORK_MODE)
				{
					if (PreWorkModeBackup != WORK_MODE || RefreshScreenRequire)
					{
						unsigned short backColor;
						RefreshScreenRequire = 0;
						TR_ClearLCD(TARGET_REGION_EXTRACTED_BACKCOLOR);

						TR_DisplayNumbers(0, TARGET_REGION_TEXT_SIZE_X, TARGET_REGION_TEXT_SIZE_Y, TARGET_REGION_TEXT_NUMBER_X, TARGET_REGION_TEXT_NUMBER_Y, TARGET_REGION_TEXT_COLOR);
						backColor = LCD_SetFontColor(TARGET_REGION_TEXT_DIRECTION_COLOR);
						LCD_printf(TARGET_REGION_TEXT_DIRECTION_X, TARGET_REGION_TEXT_DIRECTION_Y, "%-10s", "Stand by...");
						LCD_SetFontColor(backColor);
					}
					
					TR_DrawWorkModeUI();
					
					TR_DrawTargetRegion();
					
					if(TR_TargetDetect())
					{
						TR_DrawTarget();
					}
				}
				
				PreWorkModeBackup = RunMode;
			}
		}
		
	}
}

/****
	*@Brief: This function is invoked when a pixel  readed from OV7670 and turn the pixel to it's correspondingly PixelState by the 
	*		 threads defined in the BackgroundThread and LineThread then store the pixel to global variate FourStateArray in which 
	*		 one Pixel holds just 2 bits.
	*@params:
	*		 rgb :the pixel color ov7670 read. 
	*		 x,y :the position of rgb in the area of 320 * 240. 
	*@return:Pixel State the rgb in @params corresponded.
	**/

u16 TR_TakePoint(u16 rgb, unsigned short x, unsigned short y)
{
	PixelStateType currentPixelState;

	if (rgb == 0xFFFF)
			currentPixelState = TARGET;
	//else if (IS_IN_THREAD(rgb, LineThreadStruct))
	//	currentPixelState = LINE;
	else if (IS_IN_THREAD(rgb, BackgroundThreadStruct))
		currentPixelState = BACKGROUND;
	else
		currentPixelState = LINE;
	
	if(y == SamplePoint.y)
		ColorsInHorizontalMiddle[x] = rgb;
	
	RECORD_PIXEL(x, y, currentPixelState);
	
	if (x == 319 && y == 219)
		//TR_ProcessFrame();
		TR_FrameProcess();
	if (x == SamplePoint.x && y == SamplePoint.y)
		SampleColor = rgb;

	return PixelStatesColor[currentPixelState];
}

/************************************************************************************************************/
/*--------------------------------Target Recognition's Button Module Start----------------------------------*/

#define WIDTH_BUTTON	20
#define HEIGHT_BUTTON	20
#define WIDTH_SCREEN	320
#define HEIGHT_SCREEN	240
#define WIDTH_FONT		8
#define HEIGHT_FONT		16

#define COLOR_BUTTONDOWN			WHITE
#define COLOR_BUTTONUP				BLACK
#define COLOR_TEXT_BUTTONDOWN		BLACK
#define COLOR_TEXT_BUTTONUP			WHITE

typedef struct
{
	u16 XStart;
	u16 YStart;

	u16 Width;
	u16 Height;
	
	u16 ColorDown;
	u16 ColorUp;
	u16 ColorFontDown;
	u16 ColorFontUp;
	const char *Content;
	
	unsigned char StateDown;	// 0:Released  	1:Down
	
	void (*TouchHandler)(PointType* p);
	void (*ReleaseHandler)();
	void (*HoldHandler)(PointType *p);
}TouchButtonInfoType;

void TR_DrawButton(TouchButtonInfoType *btn)
{
	u16 oldFontColor, oldFontBackColor;
	
	u16 colorBack = btn->StateDown ? btn->ColorDown : btn->ColorUp;
	u16 colorFront = btn->StateDown ? btn->ColorFontDown : btn->ColorFontUp;
	
	u16 contentPixelLength = strlen(btn->Content) * WIDTH_FONT;
	
	LCD_DrawRectangle(btn->XStart, btn->YStart, btn->XStart + btn->Width, btn->YStart + btn->Height, colorBack, 1);
	
	oldFontBackColor = LCD_SetFontBackColor(colorBack);
	oldFontColor = LCD_SetFontColor(colorFront);
	
	LCD_printf(btn->XStart + (btn->Width - contentPixelLength)/2, btn->YStart + (btn->Height - HEIGHT_FONT)/2, btn->Content);
	
	LCD_SetFontBackColor(oldFontBackColor);
	LCD_SetFontColor(oldFontColor);	
}

/*------------------------------Target Recognition's Button Module End Here---------------------------*/
/******************************************************************************************************/


/******************************************************************************************************/
/*-----------------------------Target Recognition's Button Handler Starts Here------------------------*/


unsigned char ButtonFunction = 0; //Buttons Function 0: Button used to adjust Backgroud Thread. 1: Button used to adjust Line Thread


void TR_IncreaseRedThread_ButtonHoldHandler(PointType* point)
{
	if (!ButtonFunction)
	{
		BackgroundThreadStruct.MinRed++;
		OLED_PutFormat(1, 0, "Red: Max:%2x Min:%2x", BackgroundThreadStruct.MaxRed, BackgroundThreadStruct.MinRed);
	}
	else
	{
		LineThreadStruct.MaxRed++;
		OLED_PutFormat(1, 0, "Red: Max:%2x Min:%2x", LineThreadStruct.MaxRed, LineThreadStruct.MinRed);
	}	
}

void TR_DecreaseRedThread_ButtonHoldHandler(PointType* point)
{
	if (!ButtonFunction)
	{
		BackgroundThreadStruct.MinRed--;
		OLED_PutFormat(1, 0, "Red: Max:%2x Min:%2x", BackgroundThreadStruct.MaxRed, BackgroundThreadStruct.MinRed);
	}
	else
	{
		LineThreadStruct.MaxRed--;
		OLED_PutFormat(1, 0, "Red: Max:%2x Min:%2x", LineThreadStruct.MaxRed, LineThreadStruct.MinRed);
	}
}

void TR_IncreaseGreenThread_ButtonHoldHandler(PointType* point)
{
	if (!ButtonFunction)
	{
		BackgroundThreadStruct.MinGreen++;
		OLED_PutFormat(2, 0, "Green: Max:%2x Min:%2x", BackgroundThreadStruct.MaxGreen, BackgroundThreadStruct.MinGreen);
	}
	else
	{
		LineThreadStruct.MaxGreen++;
		OLED_PutFormat(2, 0, "Green: Max:%2x Min:%2x", LineThreadStruct.MaxGreen, LineThreadStruct.MinGreen);
	}
}

void TR_DecreaseGreenThread_ButtonHoldHandler(PointType* point)
{
	if (!ButtonFunction)
	{
		BackgroundThreadStruct.MinGreen--;
		OLED_PutFormat(2, 0, "Green: Max:%2x Min:%2x", BackgroundThreadStruct.MaxGreen, BackgroundThreadStruct.MinGreen);
	}
	else
	{
		LineThreadStruct.MaxGreen--;
		OLED_PutFormat(2, 0, "Green: Max:%2x Min:%2x", LineThreadStruct.MaxGreen, LineThreadStruct.MinGreen);
	}
}

void TR_IncreaseBlueThread_ButtonHoldHandler(PointType* point)
{
	if (!ButtonFunction)
	{
		BackgroundThreadStruct.MinBlue++;
		OLED_PutFormat(3, 0, "Blue: Max:%2x Min:%2x", BackgroundThreadStruct.MaxBlue, BackgroundThreadStruct.MinBlue);
	}
	else
	{
		LineThreadStruct.MaxBlue++;
		OLED_PutFormat(3, 0, "Blue: Max:%2x Min:%2x", LineThreadStruct.MaxBlue, LineThreadStruct.MinBlue);
	}
}

void TR_DecreaseBlueThread_ButtonHoldHandler(PointType* point)
{
	if (!ButtonFunction)
	{
		BackgroundThreadStruct.MinBlue--;
		OLED_PutFormat(3, 0, "Blue: Max:%2x Min:%2x", BackgroundThreadStruct.MaxBlue, BackgroundThreadStruct.MinBlue);
	}
	else
	{
		LineThreadStruct.MaxBlue--;
		OLED_PutFormat(3, 0, "Blue: Max:%2x Min:%2x", LineThreadStruct.MaxBlue, LineThreadStruct.MinBlue);
	}
}

void TR_IncreaseOV7670Contrast_ButtonHoldHandler(PointType *point)
{
	OV7670_AdjustContrast(OV7670_Contrast + 1);
	OLED_PutFormat(4, 0, "Contrast: %2x", OV7670_Contrast);
}

void TR_DecreaseOV7670Contrast_ButtonHoldHandler(PointType* point)
{
	OV7670_AdjustContrast(OV7670_Contrast - 1);
	OLED_PutFormat(4, 0, "Contrast: %2x", OV7670_Contrast);
}

void TR_IncreaseOV7670Brightness_ButtonHoldHandler(PointType *point)
{
	OV7670_AdjustBrightness(OV7670_Brightness + 1);
	OLED_PutFormat(5, 0, "Brightness: %2x", OV7670_Brightness);
}

void TR_DecreaseOV7670Brightness_ButtonHoldHandler(PointType* point)
{
	OV7670_AdjustBrightness(OV7670_Brightness - 1);
	OLED_PutFormat(5, 0, "Brightness: %2x", OV7670_Brightness);
}

void TR_ToggleMode_ButtonReleaseHandler(PointType* point)
{
	RunMode = (RunModeType)(((unsigned short)RunMode + 1)%3);
	OLED_PutFormat(6, 0, "WorkM:%6s", RunMode == 0 ? "Video" : (RunMode == 1 ? "Debug" : "NORMAL"));
}

void TR_ButtonFunctionToggle_ButtonReleaseHandler(PointType* point)
{
	ButtonFunction = !ButtonFunction;
	OLED_PutFormat(6, 7, "BF:%C", ButtonFunction ? 'L' : 'B');
}

/*--------------------------Target Recognition's Button Handler Ends here--------------*/
/***************************************************************************************/



/****************************************************************************************/
/*----------------------------Button Function Starts Here-------------------------------*/

#define TOUCH_BUTTON_AMOUNT	12

void (*(ButtonHandlers[TOUCH_BUTTON_AMOUNT]))() = {
									TR_IncreaseRedThread_ButtonHoldHandler, TR_DecreaseRedThread_ButtonHoldHandler, 				//Red thread button handler
									TR_IncreaseGreenThread_ButtonHoldHandler, TR_DecreaseGreenThread_ButtonHoldHandler, 			//Green thread button handler
									TR_IncreaseBlueThread_ButtonHoldHandler, TR_DecreaseBlueThread_ButtonHoldHandler,				//Blue thread button handler
									TR_IncreaseOV7670Contrast_ButtonHoldHandler, TR_DecreaseOV7670Contrast_ButtonHoldHandler,		//OV7670's Contrast thread button handler
									TR_IncreaseOV7670Brightness_ButtonHoldHandler,TR_DecreaseOV7670Brightness_ButtonHoldHandler,		//OV7670's Brightness thread button handler
									TR_ToggleMode_ButtonReleaseHandler, TR_ButtonFunctionToggle_ButtonReleaseHandler
								 };

char *(ButtonContents[TOUCH_BUTTON_AMOUNT]) = { "UR", "DR",	//Up Red   Thread && Down Red Thread 
										  "UG", "DG", 	//Up Green Thread && Down Green Thread
										  "UB", "DB", 	//Up Blue  Thread && Down Blue Thread
										  "UC", "DC", 	//Up Contrast && Down Contrast
										  "UI", "DI", 	//Up brIghtness && Down brIghtness
										  "CM", 		//Change Display Mode	0 :Normal Display 1: Debug Display
	                                      "CB"			//Change Button Mode 	1 :Line Thread Function 0:Background Thread Function
										};

TouchButtonInfoType Buttons[TOUCH_BUTTON_AMOUNT];
		

/**
  *@brief: Check whether the point is in Button Region or not.
  **/
bool TR_IsButtonDown(PointType* point, TouchButtonInfoType* pBtn)
{
	if ((point->x <= pBtn->XStart + pBtn->Width) && (point->x > pBtn->XStart) && (point->y > pBtn->YStart) && (point->y < pBtn->YStart + pBtn->Height))
		return TRUE;
	else
		return FALSE;
}


/****@Brief: This function maintains the function of all button whitch stored in array Buttons and their Handler stored
	*	     in the ButtonHandlers, their Contents stored in the ButtonContents uppon here.
	*@Return:Just 0 means nothing
	****/

//u16 TR_TouchHandler()
//{
//	static unsigned char IsButtonDrawed = 0;
//	
//	TouchButtonInfoType buttonTemplate = {0, HEIGHT_SCREEN - HEIGHT_BUTTON, 
//		WIDTH_BUTTON, HEIGHT_BUTTON, COLOR_BUTTONDOWN, COLOR_BUTTONUP, 
//		COLOR_TEXT_BUTTONDOWN, COLOR_TEXT_BUTTONUP, "BUTTON", 0, 
//		0, 0, 0};
//	
//	if (!IsButtonDrawed)		//If Buttons are not drawed, then draw them
//	{
//		/*----------Draw Button-------------*/
//		int index;
//		OLED_Clear();
//		
//		for (index = 0; index < TOUCH_BUTTON_AMOUNT; index ++)
//		{
//			Buttons[index] = buttonTemplate;
//			
//			Buttons[index].XStart = index * Buttons[index].Width;
//			Buttons[index].Content = ButtonContents[index];
//			
//			TR_DrawButton(&Buttons[index]);
//			
//			if (index < 10)
//				Buttons[index].HoldHandler = ButtonHandlers[index];
//			else
//				Buttons[index].ReleaseHandler = ButtonHandlers[index];
//		}
//		
//		IsButtonDrawed = 1;
//	}
//	
//	if (TP_Scan(0))	//If the user touch screen , the buttons will be update to its corresponding state and may be redrawed
//	{
//		int index;
//		
//		PointType touchPoint;
//		TP_GetTouchPosition(&touchPoint.x, &touchPoint.y);
//		
//		for (index = 0; index < TOUCH_BUTTON_AMOUNT; index ++)
//		{
//			if (TR_IsButtonDown(&touchPoint, &Buttons[index]))	
//			{
//				if (Buttons[index].StateDown == 0)		//If point both falling in button and the button's StateDown is 0, draw the button with StateDown
//				{
//					Buttons[index].StateDown = 1;
//					TR_DrawButton(&Buttons[index]);
//					
//					if (Buttons[index].TouchHandler)		
//						Buttons[index].TouchHandler(&touchPoint);
//				}
//				else									//If Point both falling in button and the button's StateDown is 1, the HoldHandler should be excuted.
//				{
//					if (Buttons[index].HoldHandler)
//						Buttons[index].HoldHandler(&touchPoint);
//				}
//			}
//			else if (!TR_IsButtonDown(&touchPoint, &Buttons[index]) && Buttons[index].StateDown == 1)
//			{
//				Buttons[index].StateDown = 0;
//				TR_DrawButton(&Buttons[index]);
//				
//				if (Buttons[index].ReleaseHandler)
//					Buttons[index].ReleaseHandler(&touchPoint);
//			}
//		}
//	}
//	else 			//If user currently dont touch the screen while touched at previous time, some button state updating maybe required.
//	{
//		int index;
//		
//		for (index = 0; index < TOUCH_BUTTON_AMOUNT; index ++)
//		{
//			if (Buttons[index].StateDown)
//			{
//				Buttons[index].StateDown = 0;
//				TR_DrawButton(Buttons + index);
//				
//				if (Buttons[index].ReleaseHandler)
//					Buttons[index].ReleaseHandler();
//			}
//		}
//	}
//	
//	return 0;
//}

/*----------------------------Button Function Ends Here---------------------------------*/
/****************************************************************************************/




/***************************************************************************************/
/*----------------------------Target Detection Starts here-----------------------------*/


#define VALID_WIDTH 	320
#define VALID_HEIGHT 	220
#define VALID_PART		1/2

/**
 *@Brief: 
 *	This function detects the target region and store it to globle variase TargetRegion.
 * 
 *@Return: 
 *		1: Detection is success. 
 *		0: Detection is failed.
 **/
//  
//unsigned char TR_TargetRegionDetect()
//{
//	unsigned short x, y;
//	TargetRegionType region;
//	unsigned int targetColorCount = 0;
//	
//	for (y = 0; y < VALID_HEIGHT/2; y ++)
//	{
//		targetColorCount = 0;
//		
//		for (x = 0; x < VALID_WIDTH; x ++)
//		{
//			if (READ_PIXEL(x, y) == BACKGROUND)
//				targetColorCount ++;
//		}
//		
//		if (targetColorCount >  VALID_WIDTH  * VALID_PART)
//		{
//			region.YStart = y;
//			break;
//		}
//	}
//	
//	if (y == VALID_HEIGHT/2)
//		return 0;
//	

//	for (y = VALID_HEIGHT - 1; y > VALID_HEIGHT/2; y --)
//	{
//		targetColorCount = 0;
//		for (x = 0; x < VALID_WIDTH; x ++)
//		{
//			if (READ_PIXEL(x, y) == BACKGROUND)
//				targetColorCount ++;
//		}
//		
//		if (targetColorCount > VALID_WIDTH * VALID_PART)
//		{
//			region.YEnd = y;
//			break;
//		}
//	}
//	
//	if (y == VALID_HEIGHT/2)
//		return 0;
//	
//	
//	for (x = 0; x < VALID_WIDTH/2; x ++)
//	{
//		targetColorCount = 0;
//		for (y = 0; y < VALID_HEIGHT; y ++)
//		{
//			if (READ_PIXEL(x, y) == BACKGROUND)
//				targetColorCount ++;
//		}
//		
//		if (targetColorCount > VALID_HEIGHT  * VALID_PART)
//		{
//			region.XStart = x;
//			break;
//		}
//	}
//	
//	if (x == VALID_WIDTH/2)
//		return 0;
//	
//	for (x = VALID_WIDTH - 1; x > VALID_WIDTH/2; x --)
//	{
//		targetColorCount = 0;
//		for (y = 0; y < VALID_HEIGHT; y ++)
//		{
//			if (READ_PIXEL(x, y) == BACKGROUND)
//				targetColorCount ++;
//		}
//		
//		if (targetColorCount > VALID_HEIGHT  * VALID_PART)
//		{
//			region.XEnd = x;
//			break;
//		}
//	}
//	
//	if (x == VALID_WIDTH/2)
//		return 0;
//	
//	
////	if((region.XEnd - region.XStart) < VALID_WIDTH/2 || region.YEnd - region.YStart < VALID_HEIGHT/2)
////		return 0;
//	
//	TargetRegion = region;
//	
//	return 1;
//}

/*stores the amount of background color per column or row.*/
unsigned short BackgroundCount[320];

/**
  *@brief	detect the target region
  *@return 	-1 	too less backgroung
  *			1	too much background
  *			0	ok
  */		

signed char TR_TargetRegionDetect()
{
	unsigned short xStart, 	xStartCount = 0;
	unsigned short xEnd, 	xEndCount = 0;
	unsigned short yStart, 	yStartCount= 0;
	unsigned short yEnd, 	yEndCount = 0;
	
	unsigned short x, y, count = 0, contiCountMax = 0, countMax = 0;
	/*----------------------------------------------------------------*/
	for (x = 0; x < VALID_WIDTH/2; x ++)
	{
		count = 0;
		contiCountMax = 0;
		
		for (y = 0; y < VALID_HEIGHT; y ++)
		{
			if(READ_PIXEL(x, y) == BACKGROUND)
				count++;
			else
			{
				if(count > contiCountMax)
					contiCountMax = count;
				
				count = 0;
			}
		}
		
		if(contiCountMax > countMax)
			countMax = contiCountMax;
		
		BackgroundCount[x] = contiCountMax;
	}
	
	if(countMax < 100)
		return -1;
	
	for(x = 0; x < VALID_WIDTH/2; x ++)
	{
		if(abs(countMax - BackgroundCount[x]) < 10)
		{
			xStart = x;
			xStartCount = BackgroundCount[x];
			break;
		}
	}
	/*----------------------------------------------------------------*/
	countMax = 0;
	
	for (x = VALID_WIDTH - 1; x > VALID_WIDTH/2; x --)
	{
		count = 0;
		contiCountMax = 0;
		for (y = 0; y < VALID_HEIGHT; y ++)
		{
			if(READ_PIXEL(x, y) == BACKGROUND)
				count++;
			else
			{
				if(count > contiCountMax)
					contiCountMax = count;
				
				count = 0;
			}
		}
		
		if(contiCountMax > countMax)
			countMax = contiCountMax;
		
		BackgroundCount[x] = contiCountMax;
	}
	
	if(countMax < 100)
		return -1;
	
	for(x = VALID_WIDTH - 1; x > VALID_WIDTH/2; x --)
	{
		if(abs(countMax - BackgroundCount[x]) < 10)
		{
			xEnd = x;
			xEndCount = BackgroundCount[x];
			break;
		}
	}
	
	/*----------------------------------------------------------------*/
	countMax = 0;
	for (y = 0; y < VALID_HEIGHT/2; y ++)
	{
		count = 0;
		contiCountMax = 0;
		for (x = 0; x < VALID_WIDTH; x ++)
		{
			if(READ_PIXEL(x, y) == BACKGROUND)
				count++;
			else
			{
				if(count > contiCountMax)
					contiCountMax = count;
				
				count = 0;
			}
		}
		
		if(contiCountMax > countMax)
			countMax = contiCountMax;
		
		BackgroundCount[y] = contiCountMax;
	}
	
	if(countMax < 100)
		return -1;
	
	for (y = 0; y < VALID_HEIGHT/2; y ++)
	{
		if(abs(countMax - BackgroundCount[y]) < 10)
		{
			yStart = y;
			yStartCount = BackgroundCount[y];
			break;
		}
	}
	
	/*----------------------------------------------------------------*/
	countMax = 0;
	for (y = VALID_HEIGHT - 1; y > VALID_HEIGHT/2; y --)
	{
		count = 0;
		contiCountMax = 0;
		for (x = 0; x < VALID_WIDTH; x ++)
		{
			if(READ_PIXEL(x, y) == BACKGROUND)
				count++;
			else
			{
				if(count > contiCountMax)
					contiCountMax = count;
				
				count = 0;
			}
		}
		
		if(contiCountMax > countMax)
			countMax = contiCountMax;
		
		BackgroundCount[y] = contiCountMax;
	}
	
	if(countMax < 100)
		return -1;
	
	for(y = VALID_HEIGHT - 1; y > VALID_HEIGHT/2; y --)
	{
		if(abs(countMax - BackgroundCount[y]) < 10)
		{
			yEnd = y;
			yEndCount = BackgroundCount[y];
			break;
		}
	}
	
	if(abs(yEndCount - yStartCount) > 30 || abs(xStartCount - xEndCount) > 30)
		return -1;
	
	TargetRegion.XStart = xStart;
	TargetRegion.XEnd = xEnd;
	TargetRegion.YStart = yStart;
	TargetRegion.YEnd = yEnd;
	
	return TR_BorderFix_v2();
}

/**
 *@Brief: This function excludes the noise in current frame.
 *@Error: But some bugs still exist. 
 **/
void TR_NoiseExclude()
{
	unsigned short x, y;
	PixelStateType curPoint;
	PixelStateType prevPoint;
	unsigned short fixColorCount = 0;
	
	for (y = TargetRegion.YStart; y < TargetRegion.YEnd; y ++)
	{
		
		for (x = TargetRegion.XStart; x < TargetRegion.XEnd; x ++)
		{
			if (x == TargetRegion.XStart)
			{
				prevPoint = READ_PIXEL(x, y);
				fixColorCount = 1;
			}
			else
			{
				curPoint = READ_PIXEL(x, y);
				
				if (curPoint == prevPoint)
					fixColorCount ++;
				else
				{
					if (fixColorCount < 2)
					{
						unsigned short indexX;
						for (indexX = x - fixColorCount; indexX < x; indexX ++)
							RECORD_PIXEL(indexX, y, curPoint);						
					}
					
					fixColorCount = 0;
				}
				
				prevPoint = curPoint;
			}
		}
	}
	
	for (x = TargetRegion.XStart; x < TargetRegion.XEnd; x ++)
	{
		
		for (y = TargetRegion.YStart; y < TargetRegion.YEnd; y ++)
		{
			if (y == TargetRegion.YStart)
			{
				prevPoint = READ_PIXEL(x, y);
				fixColorCount = 1;
			}
			else
			{
				curPoint = READ_PIXEL(x, y);
				
				if (curPoint == prevPoint)
					fixColorCount ++;
				else
				{
					if (fixColorCount < 2)
					{
						unsigned short indexY;
						for (indexY = y - fixColorCount; indexY < y; indexY ++)
							RECORD_PIXEL(x, indexY, curPoint);						
					}
					
					fixColorCount = 0;
				}
				
				prevPoint = curPoint;
			}
		}
	}
}

/**
  *@Brief:  These macro definitions define the margin of valid area of TargetRegion.
  *  		Amend them according your actual TargetRegion.
  **/

#define BORDER_MARGIN_UP		0
#define BORDER_MARGIN_BOTTOM 	10
#define BORDER_MARGIN_LEFT  	10
#define BORDER_MARGIN_RIGHT		10

/**
 *@Brief: 	This function fixs the TargetRegion through shrinking it to no-line-begin state.
 *@Notice:	It's recommonded that use this function after function TargetRegionDetect r-
 *		  	-eturns 1.
 *@Return	1 border fix done
 *			0 border fix undone
 **/
unsigned char TR_BorderFix()
{
	unsigned short x, y;
	unsigned char isAllBackgroundFlag;	//indicate whether pixels of rows or call is all background or other
	
	for (y = TargetRegion.YStart; y < TargetRegion.YStart + BORDER_MARGIN_UP; y ++)	//Uppon
	{
		isAllBackgroundFlag = 1;
		
		for (x = TargetRegion.XStart + BORDER_MARGIN_LEFT; x < TargetRegion.XEnd - BORDER_MARGIN_RIGHT; x ++)
		{
			if(READ_PIXEL(x, y) != BACKGROUND)
			{
				isAllBackgroundFlag = 0;
				break;
			}
		}
		
		if (isAllBackgroundFlag)
		{
			TargetRegion.YStart = y;	
			break;
		}
	}
	
	
	for (y = TargetRegion.YEnd; y > TargetRegion.YEnd - BORDER_MARGIN_BOTTOM; y--)	//Bottom
	{
		isAllBackgroundFlag = 1;
		for (x = TargetRegion.XStart + BORDER_MARGIN_LEFT; x < TargetRegion.XEnd - BORDER_MARGIN_RIGHT; x ++)
		{
			if(READ_PIXEL(x, y) != BACKGROUND)
			{
				isAllBackgroundFlag = 0;
				break;
			}
		}
		
		if (isAllBackgroundFlag)
		{
			TargetRegion.YEnd = y;	
			break;
		}
	}
	
	if (TargetRegion.YStart >= TargetRegion.YEnd)
		return 0;
	
	for (x = TargetRegion.XStart; x < TargetRegion.XStart + BORDER_MARGIN_LEFT; x++)	//Left
	{
		isAllBackgroundFlag = 1;
		for (y = TargetRegion.YStart; y < TargetRegion.YEnd; y ++)
		{
			if(READ_PIXEL(x, y) != BACKGROUND)
			{
				isAllBackgroundFlag = 0;
				break;
			}
		}
		
		if (isAllBackgroundFlag)
		{
			TargetRegion.XStart = x;
			break;
		}
	}
	
	for (x = TargetRegion.XEnd; x > TargetRegion.XEnd - BORDER_MARGIN_RIGHT; x--)	//Right
	{
		isAllBackgroundFlag = 1;
		for (y = TargetRegion.YStart; y < TargetRegion.YEnd; y ++)
		{
			if(READ_PIXEL(x, y) != BACKGROUND)
			{
				isAllBackgroundFlag = 0;
				break;
			}
		}
		
		if (isAllBackgroundFlag)
		{
			TargetRegion.XEnd = x;
			break;
		}
	}
	
	if(TargetRegion.XStart > TargetRegion.XEnd)
		return 0;
	
	return 1;
}


signed char TR_BorderFix_v2()
{
	unsigned short x, y;
	unsigned short xStart, xEnd, yStart, yEnd;
	unsigned char  lineExistsFlag = 0, isInLineArea = 0;
	unsigned short lineExistsTimes = 0, lineExistsMaxTimes = 0;
	unsigned short start, end = TargetRegion.XEnd;
	
	OLED_PushLine("TR_BorderFix_v2");
	
	isInLineArea = 0;
	for (x = TargetRegion.XStart; x <= TargetRegion.XEnd; x ++)
	{
		lineExistsFlag = 0;
		
		for(y = TargetRegion.YStart; y < TargetRegion.YEnd; y ++)
		{
			if (READ_PIXEL(x, y) == LINE)
			{
				lineExistsFlag = 1;
				break;
			}
		}
		
		if (isInLineArea)
		{
			if (lineExistsFlag)
				lineExistsTimes ++;
			
			if (lineExistsFlag == 0 || x == TargetRegion.XEnd)
			{
				end = x;
				
				if (lineExistsTimes > lineExistsMaxTimes)
				{
					xStart = start;
					xEnd = end;
					
					lineExistsMaxTimes = lineExistsTimes;
				}
				
				lineExistsTimes = 0;
				isInLineArea = 0;
			}
		}
		else
		{
			if (lineExistsFlag)
			{
				lineExistsTimes ++;
				isInLineArea = 1;
				start = x;
			}
		}
	}
	
	TargetRegion.XStart = MAX(xStart - 1, TargetRegion.XStart);
	TargetRegion.XEnd = MIN(xEnd + 1, TargetRegion.XEnd);
	
	isInLineArea = 0;
	lineExistsTimes = 0;
	lineExistsMaxTimes = 0;
	
	for (y = TargetRegion.YStart; y <= TargetRegion.YEnd; y ++)
	{
		lineExistsFlag = 0;
		
		for(x = TargetRegion.XStart; x < TargetRegion.XEnd; x ++)
		{
			if (READ_PIXEL(x, y) == LINE)
			{
				lineExistsFlag = 1;
				break;
			}
		}
		
		if (isInLineArea)
		{
			if (lineExistsFlag)
				lineExistsTimes ++;
			
			if (lineExistsFlag == 0 || y == TargetRegion.YEnd)
			{
				end = y;
				
				if (lineExistsTimes > lineExistsMaxTimes)
				{
					yStart = start;
					yEnd = end;
					
					lineExistsMaxTimes = lineExistsTimes;
				}
				
				lineExistsTimes = 0;
				isInLineArea = 0;
			}
		}
		else
		{
			if (lineExistsFlag)
			{
				lineExistsTimes ++;
				isInLineArea = 1;
				start = y;
			}
		}
	}
	
	if(yEnd - yStart < 100 || xEnd - xStart < 100)
		return 1;
	
	TargetRegion.YStart = MAX(yStart - 1, TargetRegion.YStart);
	TargetRegion.YEnd = MIN(yEnd + 1, TargetRegion.YEnd);
	
	TR_DisplayFrame();
	
	return 0;
}


/*count the amount of circles is in an area not a line, this macro define the area range, count begin at Middle - TRY_RANGE end at Middle + TRY_RANGE.*/
#define TRY_RANGE 4

/**
 *@Brief: This function dectects circles in the Target Region then store them in glo-
 *  -bal variase CircleShapes.
 *@Return:	0 	successfully
			1	extra circle exists
			-1	circle lost
 *@Notice:This function must be used when function TargetRegionDetect returns 1.
 **/

signed char TR_CircleDetect()
{
	unsigned short x, y;
	unsigned short circleAmountInX = 0, circleAmountInY = 0, circleAmount;
	unsigned short circlePointsInX[CIRCLE_AMOUNT_MAX * 2], circlePointsInY[CIRCLE_AMOUNT_MAX * 2];
	unsigned short circleBorderStart = 0, circleBorderEnd = 0;
	unsigned char  isInCircleBorder = 0;
	unsigned short index;
	unsigned short xMiddle, yMiddle;
	signed	 short tryDistance, tryDirection; //-1 decrease 1 increase
	unsigned short maxRadius = 0, maxRadiusPos;
	
	PointType circleCenterSum = {0, 0};

#ifdef TARGET_CENTER_AUTO_DETECT_ON	
	
	for (y = TargetRegion.YStart; y < TargetRegion.YEnd; y ++)
	{
		for (x = TargetRegion.XStart; x < TargetRegion.XEnd; x ++)
		{
			if (READ_PIXEL(x, y) == LINE)
			{
				xMiddle = x;
				goto goto_Flag1;
			}
		}
	}
	
goto_Flag1:	if (y == TargetRegion.YEnd)
		return 0;
	
	for (x = TargetRegion.XStart; x < TargetRegion.XEnd; x ++)
	{
		for (y = TargetRegion.YStart; y < TargetRegion.YEnd; y ++)
		{
			if (READ_PIXEL(x, y) == LINE)
			{
				yMiddle = y;
				goto goto_Flag2;
			}
		}
	}
	
goto_Flag2:	if (x == TargetRegion.XEnd)
		return 0;
#else
	xMiddle = (TargetRegion.XStart + TargetRegion.XEnd)/2;
	yMiddle = (TargetRegion.YStart + TargetRegion.YEnd)/2;
#endif


	/*Find the center of circle----------------------------------------------------------------*/
	/*Strategy: find the max height from 1/3 to 2/3 of targetregion is more near the radius-----*/
	maxRadius = 0;
	for(x = TargetRegion.XStart + (TargetRegion.XEnd - TargetRegion.XStart)/3; x < TargetRegion.XStart + (TargetRegion.XEnd - TargetRegion.XStart) * 2 / 3; x ++)
	{
		unsigned short yUp, radius, yBottom;
		
		for(y = TargetRegion.YStart; y < TargetRegion.YEnd; y ++)
		{
			if(RunMode == DEBUG_MODE)
				LCD_WritePoint(x, y, WHITE);	
			
			if(READ_PIXEL(x, y) == LINE)
			{
				yUp = y;
				break;
			}
		}
		
		for(y = TargetRegion.YEnd; y > TargetRegion.YStart; y --)
		{
			if(RunMode == DEBUG_MODE)
				LCD_WritePoint(x, y, WHITE);
			
			if(READ_PIXEL(x, y) == LINE)
			{
				yBottom = y;
				break;
			}
		}
		
		if(yBottom <= yUp)
			return -1;

		radius = yBottom - yUp;
		
		if(maxRadius < radius)
		{
			maxRadius = radius;
			maxRadiusPos = x;
		}
	}
	
	xMiddle = maxRadiusPos;
	
	if(RunMode == DEBUG_MODE)
		LCD_DrawLine(maxRadiusPos, 0, maxRadiusPos, 200);
	
	maxRadius = 0;
	for(y = TargetRegion.YStart + (TargetRegion.YEnd - TargetRegion.YStart)/3; y < TargetRegion.YStart + (TargetRegion.YEnd - TargetRegion.YStart) * 2 / 3; y ++)
	{
		unsigned short xLeft, radius, xRight;
		
		for(x = TargetRegion.XStart; x < TargetRegion.XEnd; x ++)
		{
			if(RunMode == DEBUG_MODE)
				LCD_WritePoint(x, y, WHITE);
			
			if(READ_PIXEL(x, y) == LINE)
			{
				xLeft = x;
				break;
			}
		}
		
		for(x = TargetRegion.XEnd; x > TargetRegion.XStart; x --)
		{
			if(RunMode == DEBUG_MODE)
				LCD_WritePoint(x, y, WHITE);
			
			if(READ_PIXEL(x, y) == LINE)
			{
				xRight = x;
				break;
			}
		}
		
		if(xRight <= xLeft)
			return -1;
		
		radius = xRight - xLeft;
		
		if(maxRadius < radius)
		{
			maxRadius = radius;
			maxRadiusPos = y;
		}
	}
	
	yMiddle = maxRadiusPos;
	
	if(RunMode == DEBUG_MODE)
		LCD_DrawLine(0, yMiddle, 200, yMiddle);
	
	tryDirection  = 1;
	for(tryDistance = 0; abs(tryDistance) < TRY_RANGE; tryDistance += tryDirection)
	{
		circleAmountInY = 0;
		
		for ( y = TargetRegion.YStart; y < TargetRegion.YEnd; y ++ )
		{
			PixelStateType curColor = READ_PIXEL(xMiddle + tryDistance, y);
			
			if (curColor == BACKGROUND)
			{
				if (isInCircleBorder == 0)
					continue;
				else
				{
					circleBorderEnd = y - 1;
					circlePointsInY[circleAmountInY++] = (circleBorderEnd + circleBorderStart)/2;
					isInCircleBorder = 0;
				}
			}
			
			if ( curColor == LINE && isInCircleBorder == 0 )
			{
				circleBorderStart = y;
				isInCircleBorder = 1;
			}
		}
		
		if(circleAmountInY == CIRCLE_AMOUNT * 2)
			break;
		
		if(tryDistance == TRY_RANGE - 1)
		{
			tryDistance = 0;
			tryDirection = -1;
		}
	}
	
	tryDirection  = 1;
	for(tryDistance = 0; abs(tryDistance) < TRY_RANGE; tryDistance += tryDirection)
	{
		circleAmountInX = 0;
		
		for (x = TargetRegion.XStart; x < TargetRegion.XEnd; x ++)
		{
			PixelStateType curColor = READ_PIXEL(x, yMiddle + tryDistance);
			
			if (curColor == BACKGROUND)
			{
				if (isInCircleBorder == 0)
					continue;
				else
				{
					circleBorderEnd = x - 1;
					circlePointsInX[circleAmountInX++] = (circleBorderEnd + circleBorderStart)/2;
					isInCircleBorder = 0;
				}
			}
			
			if (curColor == LINE && isInCircleBorder == 0)
			{
				circleBorderStart = x;
				isInCircleBorder = 1;
			}
		}
		
		if(circleAmountInX == CIRCLE_AMOUNT * 2)
			break;
		
		if(tryDistance == TRY_RANGE - 1)
		{
			tryDistance = 0;
			tryDirection = -1;
		}
	}
	
	OLED_PushLine("CirN:y:%2d x: %2d", circleAmountInY, circleAmountInX);
	
	if (circleAmountInX != CIRCLE_AMOUNT * 2 || circleAmountInY != CIRCLE_AMOUNT * 2)
	{
		if (circleAmountInX > CIRCLE_AMOUNT * 2 || circleAmountInY > CIRCLE_AMOUNT * 2)
		{
			return 1;
		}
		else
		{
			return -1;
		}
	}
	
	circleAmount = circleAmountInX/2;
	
	for (index = 0; index < circleAmount; index ++)
	{
		short radiusX = (circlePointsInX[circleAmountInX - index - 1] - circlePointsInX[index])/2;
		short radiusY = (circlePointsInY[circleAmountInY - index - 1] - circlePointsInY[index])/2;
		
		CircleShapes[index].Radius = (radiusX + radiusY) / 2;
		
		CircleShapes[index].Center.x = (circlePointsInX[circleAmountInX - index - 1] + circlePointsInX[index])/2;
		CircleShapes[index].Center.y = (circlePointsInY[circleAmountInY - index - 1] + circlePointsInY[index])/2;
		
		circleCenterSum.x += CircleShapes[index].Center.x;
		circleCenterSum.y += CircleShapes[index].Center.y;
	}
	
	circleCenterSum.x /= circleAmount;	//get average x
	circleCenterSum.y /= circleAmount;	//get average y
	
	for (index = 0; index < circleAmount; index ++)
		CircleShapes[index].Center = circleCenterSum;
	
	CircleShapeAmount = circleAmount;
	
#ifndef SHOW_CORSS
	XCross = circleCenterSum.x;
	YCross = circleCenterSum.y;
#endif
	
	for(index = 0; index < circleAmount - 1; index ++)
	{
		if(CircleShapes[index].Radius <= CircleShapes[index + 1].Radius)
			break;
	}
	
	if(index != circleAmount - 1)
		return -1;
	
	return 0;
}

#define TARGET_SIZE_MAX	50
#define TARGET_SIZE_MIN	3

unsigned char TR_TargetDetect()
{
	unsigned short x, y;
	unsigned short xStart, xEnd, yStart, yEnd;
	unsigned char findFlag = 0;
	
	for (x = TargetRegion.XStart; x < TargetRegion.XEnd; x++)
	{
		
		for (y = TargetRegion.YStart; y < TargetRegion.YEnd; y++)
		{
			if (READ_PIXEL(x,y) == TARGET)
			{
					xStart = x;
					findFlag = 1;
					break;
			}
		}
		if (findFlag == 1)
			break;
	}
	
	if (findFlag == 0)
		return 0;
	
	findFlag = 0;
	
	for (x = TargetRegion.XEnd - 1; x >= TargetRegion.XStart; x--)
	{
		for (y = TargetRegion.YStart; y < TargetRegion.YEnd; y++)
		{
			if (READ_PIXEL(x,y) == TARGET)
			{
					xEnd = x;
					findFlag = 1;
					break;
			}
		}
		if (findFlag == 1)
			break;
	}
	
	if (findFlag == 0)
		return 0;
	
	findFlag = 0;
	
	for (y = TargetRegion.YStart; y < TargetRegion.YEnd; y++)
	{
		for (x = TargetRegion.XStart; x < TargetRegion.XEnd; x++)
		{
			if (READ_PIXEL(x,y) == TARGET)
			{
					yStart = y;
					findFlag = 1;
					break;
			}
		}
		if (findFlag == 1)
			break;
	}
	
	if (findFlag == 0)
		return 0;
	
	findFlag = 0;
	
	for (y = TargetRegion.YEnd - 1; y >= TargetRegion.YStart; y--)
	{
		for (x = TargetRegion.XStart; x < TargetRegion.XEnd; x++)
		{
			if (READ_PIXEL(x,y) == TARGET)
			{
					yEnd = y;
					findFlag = 1;
					break;
			}
		}
		if (findFlag == 1)
			break;
	}
	
	if (findFlag == 0)
		return 0;
	
	if (xEnd - xStart > TARGET_SIZE_MAX || yEnd - yStart > TARGET_SIZE_MAX || xEnd - xStart < TARGET_SIZE_MIN || yEnd - yStart < TARGET_SIZE_MIN)
		return 0;
	
	TargetPoint.x = (xStart + xEnd) / 2;
	TargetPoint.y = (yStart + yEnd) / 2;
	
	return 1;
}


#define DIRECTION_AMOUNT	8

TargetDirectionInfoType TargetDirectionInfos[DIRECTION_AMOUNT] = 
{
	{RIGHT,  -22.5,  	22.5, 	"Right"},
	{RIGHTUP, -67.5,  	-22.5, 	"Right Up"},
	{UP,	  -112.5,  	-67.5,	"UP"},
	{LEFTUP,  -157.5, 	-112.5, "Left Up"},
	{LEFT,	  157.5, 	-157.5,  "Left"},		//this is correct
	{LEFTDOWN, 112.5, 	157.5, 	"Left Down"},
	{DOWN,	   67.5,	112.5,  "Down"},
	{RIGHTDOWN, 22.5,   67.5,	"Right Down"}
};



/**
 *@brief	This is function analyse the target position, and return the flag of result to indicate the success or fail.
 *@return   1 success
 *		    0 fails
 */
unsigned char TR_AnalyseTarget(TargetInfoType* targetInfo)
{
	PointType circleCenter = CircleShapes[0].Center;
	PointType absoluteTargetPosition;
	float	  relativeAngle;
	
	short index;
	unsigned short distance;
	
	distance = sqrt(pow(circleCenter.x - TargetPoint.x, 2) + pow(circleCenter.y - TargetPoint.y, 2));
	
	for(index = CIRCLE_AMOUNT - 1; index >= 0; index --)
	{
		if(distance < CircleShapes[index].Radius)
		{
			targetInfo->cylinderCount = 5 + index;
			break;
		}
	}
	
	if(index == -1)
	{
		targetInfo->cylinderCount = 4;
	}
	
	absoluteTargetPosition.x = TargetPoint.x - circleCenter.x;
	absoluteTargetPosition.y = TargetPoint.y - circleCenter.y;
	
	if(absoluteTargetPosition.x == 0)
	{
		if(absoluteTargetPosition.y >= 0)
			relativeAngle = 90.0;
		else
			relativeAngle = - 90.0;
	}
	else
	{
		relativeAngle = atan(((float)absoluteTargetPosition.y)/((float)absoluteTargetPosition.x)) / 3.14159 * 180;
	
		if(absoluteTargetPosition.x > 0 && absoluteTargetPosition.y > 0)
			relativeAngle = relativeAngle;
		else if(absoluteTargetPosition.x < 0 && absoluteTargetPosition.y > 0)
			relativeAngle = 180.0 + relativeAngle;
		else if(absoluteTargetPosition.x < 0 && absoluteTargetPosition.y < 0)
			relativeAngle = -180 + relativeAngle;
		else if(absoluteTargetPosition.x > 0 && absoluteTargetPosition.y < 0)
			relativeAngle = relativeAngle;
	}
	
	for(index = 0; index < DIRECTION_AMOUNT; index ++)
	{
		if(TargetDirectionInfos[index].AngleMax < TargetDirectionInfos[index].AngleMin)
		{
			if(relativeAngle < TargetDirectionInfos[index].AngleMax || relativeAngle >= TargetDirectionInfos[index].AngleMin)
			{
				targetInfo->Direction = TargetDirectionInfos + index;
				break;
			}
		}
		else if(relativeAngle <= TargetDirectionInfos[index].AngleMax && relativeAngle > TargetDirectionInfos[index].AngleMin)
		{
			targetInfo->Direction = TargetDirectionInfos + index;
			break;
		}
	}
	
	if(index == DIRECTION_AMOUNT)
		return 0;
	
	return 1;
}

/**
  *@Brief	This function auto amend the thread which divide the line and background of the target region.
  *			
  *@return	0  	amend sccussefully
  *         1  	amount of circles detected is more than the circle amount may cause of noise, this is used to ajust the offset of thread
  *			-1 	amount of circles detected is less than the circle amount may cause of too width threand of background, this is used to 
  *				ajust the offset of thread
  *			2	error occurs when verify the thread bettwen minThread and maxThread may cause of noise
  *@notice 	This function is useless untile TR_TargetRegionDetect returns 1.
  **/

signed char TR_ThreadRedChannelAmend()
{
	unsigned short index;
	unsigned short lineCounter = 0;
	
	unsigned short redThread, redMaxThread, redMinThread, maxRed, minRed, tmpRed;
	
	unsigned char  curState, preState = 2; 	// 0: backgroundstate 	1:lineState 		2: noneState
	signed char    errorFlag = 0;			// 0: no erro 			1:error occured
	
	/*find the max rea and min red-------------------------------------------------------------*/
	for (index = TargetRegion.XStart; index <= TargetRegion.XEnd; index++ )
	{
		tmpRed = R(ColorsInHorizontalMiddle[index]);
		
		if (index == TargetRegion.XStart)
		{
			minRed = maxRed = tmpRed;
			continue;
		}
		
		if (minRed > tmpRed)
			minRed = tmpRed;
		
		if (maxRed < tmpRed)
			maxRed = tmpRed;
	}
	
	redThread = minRed;
	/*From bottom to top, get the min redThread -------------------------------------------------*/
	for (redThread = minRed; redThread < maxRed && lineCounter != CIRCLE_AMOUNT * 2; redThread ++)
	{
		lineCounter = 0;
		preState = 2;
		for (index = TargetRegion.XStart; index <= TargetRegion.XEnd; index ++)
		{
			tmpRed = R(ColorsInHorizontalMiddle[index]);
			
			if (tmpRed > redThread)
			{
				curState = 0;
			}
			
			else if (tmpRed < redThread)
			{
				curState = 1;
			}
			else
			{
				continue;
			}
			
			if (preState == 1 && curState == 0)
			{
				lineCounter ++;
			}
			
			preState = curState;
		}
	}
	
	if (lineCounter > CIRCLE_AMOUNT * 2)
		return 1;
	
	if (lineCounter < CIRCLE_AMOUNT * 2)
		return -1;
	
	redMinThread = redThread;
	
	lineCounter = 0;
	
	/*From top to botton, get the max redThread -------------------------------------------------*/
	for (redThread = maxRed; redThread > minRed && lineCounter != CIRCLE_AMOUNT * 2; redThread --)
	{
		lineCounter = 0;
		preState = 2;
		for (index = TargetRegion.XStart; index <= TargetRegion.XEnd; index ++)
		{
			tmpRed = R(ColorsInHorizontalMiddle[index]);
			
			if (tmpRed > redThread)
			{
				curState = 0;
			}
			
			else if (tmpRed < redThread)
			{
				curState = 1;
			}
			else
			{
				continue;
			}
			
			if (preState == 1 && curState == 0)
			{
				lineCounter ++;
			}
			
			preState = curState;
		}
	}
	
	if (lineCounter > CIRCLE_AMOUNT * 2)
		return 1;
	
	if (lineCounter < CIRCLE_AMOUNT * 2)
		return -1;
	
	redMaxThread = redThread;
	
	/*test whether threads bettwen maxThread and min thread is all work correctly--------------------*/
	for (redThread = redMinThread; redThread <= redMaxThread && !errorFlag; redThread ++)
	{
		lineCounter = 0;
		preState = 2;
		for (index = TargetRegion.XStart; index <= TargetRegion.XEnd; index ++)
		{
			tmpRed = R(ColorsInHorizontalMiddle[index]);
			
			if (tmpRed > redThread)
			{
				curState = 0;
			}
			
			else if (tmpRed < redThread)
			{
				curState = 1;
			}
			else
			{
				continue;
			}
			
			if (preState == 1 && curState == 0)
			{
				lineCounter ++;
			}
			
			preState = curState;
		}
		
		
		if (lineCounter > CIRCLE_AMOUNT * 2)
			errorFlag = 1;
		
		if (lineCounter < CIRCLE_AMOUNT * 2)
			errorFlag = -1;
	}
	
	if (errorFlag)
		return errorFlag;
	
	BackgroundThreadStruct.MinRed = (redMaxThread + redMinThread)/2;
	
	BackgroundThreadMinRedMax = redMaxThread;
	BackgroundThreadMinRedMin = redMinThread;
	
	return 0;
}

/**
 *@brief	refer to TR_ThreadRedChannelAmend function
 **/

signed char TR_ThreadBlueChannelAmend()
{
	unsigned short index;
	unsigned short lineCounter = 0;
	
	unsigned short blueThread, blueMaxThread, blueMinThread, maxBlue, minBlue, tmpBlue;
	
	unsigned char  curState, preState = 2; // 0: backgroundstate 1:lineState 2: noneState
	signed char  errorFlag = 0;			//0 : no erro 		1:error occublue
	
	/*find the max blue and min blue---------------------------------------------------------------*/
	for (index = TargetRegion.XStart; index <= TargetRegion.XEnd; index++ )
	{
		tmpBlue = B(ColorsInHorizontalMiddle[index]);
		
		if (index == TargetRegion.XStart)
		{
			minBlue = maxBlue = tmpBlue;
			continue;
		}
		
		if (minBlue > tmpBlue)
			minBlue = tmpBlue;
		
		if (maxBlue < tmpBlue)
			maxBlue = tmpBlue;
	}
	
	blueThread = minBlue;
	/*From bottom to top, get the min blueThread -------------------------------------------------*/
	for (blueThread = minBlue; blueThread < maxBlue && lineCounter != CIRCLE_AMOUNT * 2; blueThread ++)
	{
		lineCounter = 0;
		preState = 2;
		for (index = TargetRegion.XStart; index <= TargetRegion.XEnd; index ++)
		{
			tmpBlue = B(ColorsInHorizontalMiddle[index]);
			
			if (tmpBlue > blueThread)
			{
				curState = 0;
			}
			
			else if (tmpBlue < blueThread)
			{
				curState = 1;
			}
			else
			{
				continue;
			}
			
			if (preState == 1 && curState == 0)
			{
				lineCounter ++;
			}
			
			preState = curState;
		}
	}
	
	if (lineCounter > CIRCLE_AMOUNT * 2)
		return 1;
	
	if (lineCounter < CIRCLE_AMOUNT * 2)
		return -1;
	
	blueMinThread = blueThread;
	
	lineCounter = 0;
	
	/*From top to botton, get the man blueThread -------------------------------------------------*/
	for (blueThread = maxBlue; blueThread > minBlue && lineCounter != CIRCLE_AMOUNT * 2; blueThread --)
	{
		lineCounter = 0;
		preState = 2;
		for (index = TargetRegion.XStart; index <= TargetRegion.XEnd; index ++)
		{
			tmpBlue = B(ColorsInHorizontalMiddle[index]);
			
			if (tmpBlue > blueThread)
			{
				curState = 0;
			}
			
			else if (tmpBlue < blueThread)
			{
				curState = 1;
			}
			else
			{
				continue;
			}
			
			if (preState == 1 && curState == 0)
			{
				lineCounter ++;
			}
			
			preState = curState;
		}
	}
	
	if (lineCounter > CIRCLE_AMOUNT * 2)
		return 1;
	
	if (lineCounter < CIRCLE_AMOUNT * 2)
		return -1;
	
	blueMaxThread = blueThread;
	errorFlag = 0;
	
	for(blueThread = blueMinThread; blueThread <= blueMaxThread && !errorFlag; blueThread ++)
	{
		lineCounter = 0;
		preState = 2;
		for (index = TargetRegion.XStart; index <= TargetRegion.XEnd; index ++)
		{
			tmpBlue = B(ColorsInHorizontalMiddle[index]);
			
			if (tmpBlue > blueThread)
			{
				curState = 0;
			}
			
			else if (tmpBlue < blueThread)
			{
				curState = 1;
			}
			else
			{
				continue;
			}
			
			if (preState == 1 && curState == 0)
			{
				lineCounter ++;
			}
			
			preState = curState;
		}
		
		if(lineCounter > CIRCLE_AMOUNT * 2)
			errorFlag = 1;
		
		if(lineCounter < CIRCLE_AMOUNT * 2)
			errorFlag = -1;
	}
	
	if (errorFlag)
		return errorFlag;
	
	BackgroundThreadStruct.MinBlue = (blueMaxThread + blueMinThread)/2;
	
	BackgroundThreadMinBlueMax = blueMaxThread;
	BackgroundThreadMinBlueMin = blueMinThread;
	
	return 0;
}


/**
 *@brief	refer to TR_ThreadRedChannelAmend function
 *@notice	the green channel doesn't have reference value for dividing background and circle line. So , don't use this function until you 
 *			change the texture of circle line. One thing should be knew is the color of black pen is absolutely no-black.
 **/
signed char TR_ThreadGreenChannelAmend()
{
	unsigned short index;
	unsigned short lineCounter = 0;
	
	unsigned short greenThread, greenMaxThread, greenMinThread, maxGreen, minGreen, tmpGreen;
	
	unsigned char  curState, preState = 2; // 0: backgroundstate 1:lineState 2: noneState
	signed char  errorFlag = 0;			//0 : no erro 		1:error occugreen
	
	/*find the max rea and min green*/
	for (index = TargetRegion.XStart; index <= TargetRegion.XEnd; index++ )
	{
		tmpGreen = G(ColorsInHorizontalMiddle[index]);
		
		if (index == TargetRegion.XStart)
		{
			minGreen = maxGreen = tmpGreen;
			continue;
		}
		
		if (minGreen > tmpGreen)
			minGreen = tmpGreen;
		
		if (maxGreen < tmpGreen)
			maxGreen = tmpGreen;
	}
	
	greenThread = minGreen;
	/*From bottom to top, get the min greenThread -------------------------------------------------*/
	for (greenThread = minGreen; greenThread < maxGreen && lineCounter != CIRCLE_AMOUNT * 2; greenThread ++)
	{
		lineCounter = 0;
		preState = 2;
		for (index = TargetRegion.XStart; index <= TargetRegion.XEnd; index ++)
		{
			tmpGreen = G(ColorsInHorizontalMiddle[index]);
			
			if (tmpGreen > greenThread)
			{
				curState = 0;
			}
			
			else if (tmpGreen < greenThread)
			{
				curState = 1;
			}
			else
			{
				continue;
			}
			
			if (preState == 1 && curState == 0)
			{
				lineCounter ++;
			}
			
			preState = curState;
		}
	}
	
	if (lineCounter > CIRCLE_AMOUNT * 2)
		return 1;
	
	if (lineCounter < CIRCLE_AMOUNT * 2)
		return -1;
	
	greenMinThread = greenThread;
	
	lineCounter = 0;
	
	/*From top to botton, get the man greenThread -------------------------------------------------*/
	for (greenThread = maxGreen; greenThread > minGreen && lineCounter != CIRCLE_AMOUNT * 2; greenThread --)
	{
		lineCounter = 0;
		preState = 2;
		for (index = TargetRegion.XStart; index <= TargetRegion.XEnd; index ++)
		{
			tmpGreen = G(ColorsInHorizontalMiddle[index]);
			
			if (tmpGreen > greenThread)
			{
				curState = 0;
			}
			
			else if (tmpGreen < greenThread)
			{
				curState = 1;
			}
			else
			{
				continue;
			}
			
			if (preState == 1 && curState == 0)
			{
				lineCounter ++;
			}
			
			preState = curState;
		}
	}
	
	if (lineCounter > CIRCLE_AMOUNT * 2)
		return 1;
	
	if (lineCounter < CIRCLE_AMOUNT * 2)
		return -1;
	
	greenMaxThread = greenThread;
	
	for(greenThread = greenMinThread; greenThread <= greenMaxThread && !errorFlag; greenThread ++)
	{
		lineCounter = 0;
		preState = 2;
		for (index = TargetRegion.XStart; index <= TargetRegion.XEnd; index ++)
		{
			tmpGreen = G(ColorsInHorizontalMiddle[index]);
			
			if (tmpGreen > greenThread)
			{
				curState = 0;
			}
			
			else if (tmpGreen < greenThread)
			{
				curState = 1;
			}
			else
			{
				continue;
			}
			
			if (preState == 1 && curState == 0)
			{
				lineCounter ++;
			}
			
			preState = curState;
		}
		
		if(lineCounter > CIRCLE_AMOUNT * 2)
			errorFlag = 1;
		
		if(lineCounter < CIRCLE_AMOUNT * 2)
			errorFlag = -1;
	}
	
	if (errorFlag)
		return errorFlag;
	
	BackgroundThreadStruct.MinGreen = (greenMaxThread + greenMinThread)/2;
	
	BackgroundThreadMinGreenMax = greenMaxThread;
	BackgroundThreadMinGreenMin = greenMinThread;
	
	return 0;
}

/**
 *@brief 	this function return the amount of continuous linestate in currentframe which can be used for noise detecting.
 *@return	the max amount of continous linestate.
 *@note		this function regard line state of current frame as noise.
*/
unsigned int TR_NoiseDetect()
{
	unsigned int maxNoise = 0, maxNoiseLine = 0, noiseCount = 0;
	unsigned short x, y;
	PixelStateType curState;
	
	/*count in horizontal direction----------------------------------------------------*/
	for (y = TargetRegion.YStart; y <= TargetRegion.YEnd; y ++)
	{
		for (x = TargetRegion.XStart; x < TargetRegion.XEnd + 1; x ++)
		{
			curState = READ_PIXEL(x, y);
			
			if(curState == LINE)
				noiseCount ++;
		}
		
		if(noiseCount > maxNoise) 
		{
			maxNoise = noiseCount;
			maxNoiseLine = y;
		}
		noiseCount = 0;
	}
	
	LCD_DrawLine(0, maxNoiseLine, 300, maxNoiseLine);
	OLED_PutFormat(4, 0, "maxNoi:%3d line:%3d", maxNoise, maxNoiseLine);
	
	return maxNoise;
}

/*--------------------------Target Region Detection Ends Here--------------------------*/
/***************************************************************************************/

/****************************************************************************************/
/*---------------------------------------Other------------------------------------------*/

#define RUN_MODE_SELECT_SWITCH		BUTTON3
#define DEBUG_MODE_SELECT_BUTTON	BUTTON1

void TR_ButtonSwitchHandler_WorkMode(unsigned char state)
{
	RunMode = state ? DEBUG_MODE : WORK_MODE;
	OLED_PutFormat(0,0, "%s", RunMode == DEBUG_MODE ? "Debug Mode" : "WorK mode");
}

void TR_ButtonReleaseHandler_DebugMode(unsigned char state)
{
	if(RunMode != DEBUG_MODE)
		return;
	
	if(DebugMode == DEBUG_VIDEO_MODE)
		DebugMode = DEBUG_ADJUST_MODE;
	else
		DebugMode = DEBUG_VIDEO_MODE;
	
	OLED_PutFormat(0,0, "%s", DebugMode == DEBUG_VIDEO_MODE ? "VIDEO Mode" : "Adjust mode");
}

void delay(unsigned int ms)
{
	unsigned int i = 10000;
	while(i = 10000, ms--)
		while(i--);
}

void TR_Init()
{
	PointType pStart = {0, 100};
	PointType pEnd = {319, 100};
	BTN_init();
	
	OLED_AutoWrap = TRUE;
	LCD_SetFontColor(0x0000);
	LCD_SetFontBackColor(0xFFFF);
	LCD_printf(50, 80, "Target Recognition by");
	TR_DrawLine(&pStart, &pEnd, 3, RED);
	LCD_DrawBMP(200, 110, 55, 29, gImage_name);
	
	LCD_SetFontColor(0x0F31);
	LCD_printf(3, 220, "Press Button N");
	
	while(!BTN_CheckState(BUTTON2));
	BTN_SetHandler(RUN_MODE_SELECT_SWITCH, 	 TR_ButtonSwitchHandler_WorkMode, 0, 0);
	BTN_SetHandler(DEBUG_MODE_SELECT_BUTTON, 0, TR_ButtonReleaseHandler_DebugMode, 0);
	
	LCD_SetFontBackColor(BLACK);
	LCD_SetFontColor(YELLOW);
	LCD_Clear(BLACK);
	
	LCD_printf(0, 220, "Init ok.");
	
	TR_ButtonSwitchHandler_WorkMode(BTN_CheckState(SWITCH));
}
/*---------------------------------------Other------------------------------------------*/
/****************************************************************************************/


unsigned char ReadPixel;
void TR_Test()
{
	LCD_DrawBMP(0, 0, 55, 29, gImage_name);
}



