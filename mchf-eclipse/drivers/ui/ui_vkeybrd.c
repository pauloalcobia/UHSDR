/*  -*-  mode: c; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4; coding: utf-8  -*-  */
/************************************************************************************
 **                                                                                **
 **                                        UHSDR                                   **
 **               a powerful firmware for STM32 based SDR transceivers             **
 **                                                                                **
 **--------------------------------------------------------------------------------**
 **                                                                                **
 **  File name:		ui_vkeybrd.c                                                   **
 **  Description:   Virtual keyboard functions                                     **
 **  Licence:		GNU GPLv3                                                      **
 **  Author: 		Slawomir Balon/SP9BSL                                          **
 ************************************************************************************/

#include "uhsdr_board.h"
#include "ui_spectrum.h"
#include "ui_driver.h"
#include "stdlib.h"
#include "radio_management.h"

#define Col_BtnForeCol RGB(0x90,0x90,0x90)
#define Col_BtnForePressed RGB(0x70,0x70,0x70)
#define Col_BtnLightLeftTop RGB(0xd0,0xd0,0xd0)
#define Col_BtnLightRightBot RGB(0x50,0x50,0x50)
#define Col_BtnDisabled RGB(0x60,0x60,0x60)
#define Col_Warning RGB(0xC0,0xC0,0x50)


/**
 * @brief Draws basic button shape
 * @param Ka	pointer to button description area
 * @param col_Bcgr colour of background (inside button)
 * @param col_LeftUP	colour of Left/up edge (highlighted)
 * @param col_RightBot  colour of Right/bottom (shadowed)
 */

static void UiVk_DrawButtonBody(UiArea_t* Ka, uint16_t col_Bcgr, uint16_t col_LeftUP, uint16_t col_RightBot)
{
	UiLcdHy28_DrawFullRect(Ka->x+1,Ka->y+1,Ka->h-1,Ka->w-1,col_Bcgr);
	UiLcdHy28_DrawStraightLine(Ka->x,Ka->y,Ka->h+1,LCD_DIR_VERTICAL,col_LeftUP);	//left
	UiLcdHy28_DrawStraightLine(Ka->x+1,Ka->y,Ka->w,LCD_DIR_HORIZONTAL,col_LeftUP);	//top
	UiLcdHy28_DrawStraightLine(Ka->x+Ka->w,Ka->y+1,Ka->h-1,LCD_DIR_VERTICAL,col_RightBot);	//right
	UiLcdHy28_DrawStraightLine(Ka->x+1,Ka->y+Ka->h,Ka->w,LCD_DIR_HORIZONTAL,col_RightBot); //bottom
}

/**
 * @brief Function draws button, with described state and text
 * @param Btn_area rectangle describing button position and size
 * @param Warning 1=Warning sign
 * @param Vbtn_State see @ref Vbtn_State_
 * @param txt Text to be printed on button
 * @param text_color_NotPressed text colour of not pressed button
 * @param text_color_Pressed text colour of pressed (normal) button
 */
static void UiVk_DrawButton(UiArea_t* Btn_area, bool Warning, uint8_t Vbtn_State, const char* txt, uint16_t text_color_NotPressed, uint16_t text_color_Pressed, uint8_t font)
{
	uint16_t col_LeftUP, col_RightBot, col_Bcgr, col_Text;

	switch(Vbtn_State)
	{
	case Vbtn_State_Disabled:
		col_LeftUP=Col_BtnDisabled;
		col_RightBot=Col_BtnDisabled;
		col_Bcgr=Col_BtnForeCol;
		col_Text=Col_BtnDisabled;
		break;
	case Vbtn_State_Pressed:
		col_LeftUP=Col_BtnLightRightBot;
		col_RightBot=Col_BtnLightLeftTop;
		col_Bcgr=Col_BtnForePressed;
		col_Text=text_color_Pressed;
		break;
	case Vbtn_State_Normal:
		col_LeftUP=Col_BtnLightLeftTop;
		col_RightBot=Col_BtnLightRightBot;
		col_Bcgr=Warning?Col_Warning:Col_BtnForeCol;
		col_Text=text_color_NotPressed;
		break;
	}

    uint8_t FontHeight=UiLcdHy28_TextHeight(font);
    uint16_t TextHeight =  FontHeight;

    const char* txt_=txt;
    while(*txt_!=0)
    {
    	if(*txt_=='\n')
    	{
    		TextHeight+=FontHeight;
    	}
    	txt_++;
    }

	UiVk_DrawButtonBody(Btn_area,col_Bcgr,col_LeftUP,col_RightBot);
	UiLcdHy28_PrintTextCentered(Btn_area->x+2,Btn_area->y+Btn_area->h/2-TextHeight/2,Btn_area->w-4,txt,col_Text,col_Bcgr,font);
}

/**
 * @brief Returns dimension of virtual keyboard draw area
 * @param KeybArea pointer to virtual keypad draw area
 */
static void UiVk_GetKeybArea(UiArea_t* KeybArea)
{
	const VKeypad* VKpad=ts.VirtualKeyPad;

	KeybArea->w=VKpad->Columns*VKpad->KeyWidth+
			(VKpad->Columns-1)*VKpad->KeySpacing;
	KeybArea->h=VKpad->Rows*(VKpad->KeyHeight)+
			(VKpad->Rows-1)*VKpad->KeySpacing+VKpad->YtopMargin;

	KeybArea->x=sd.Slayout->full.x+sd.Slayout->full.w/2-KeybArea->w/2;
	KeybArea->y=sd.Slayout->full.y+sd.Slayout->full.h/2-KeybArea->h/2;
}

/**
 * @brief Returns dimension and location of selected key
 * @param Key number of key in keypad to locate
 * @param bp returned pointer to key area
 */
static void UiVk_GetButtonRgn(uint8_t Key, UiArea_t *bp)
{
	const VKeypad* VKpad=ts.VirtualKeyPad;

	int row, col;
	row=Key/VKpad->Columns;
	col=Key-row*VKpad->Columns;
	UiArea_t KeybArea;
	UiVk_GetKeybArea(&KeybArea);
	uint16_t KeyExpX=VKpad->Keys[Key].SizeX;
	uint16_t KeyExpY=VKpad->Keys[Key].SizeY;

	bp->x=KeybArea.x+col*(VKpad->KeyWidth+VKpad->KeySpacing);
	bp->y=KeybArea.y+row*(VKpad->KeyHeight+VKpad->KeySpacing)+VKpad->YtopMargin;
	bp->w=VKpad->KeyWidth*(KeyExpX+1)+(VKpad->KeySpacing*KeyExpX);
	bp->h=VKpad->KeyHeight*(KeyExpY+1)+(VKpad->KeySpacing*KeyExpY);
}

/**
 * @brief Draws Virtual keypad
 */
static void UiVk_DrawVKeypad()
{
	const VKeypad* VKpad=ts.VirtualKeyPad;
	int row, col, keycnt=0, Warning=0;
	UiArea_t b_area;
	for(row=0;row<VKpad->Rows;row++)
	{
		for(col=0;col<VKpad->Columns;col++)
		{
			if(keycnt==VKpad->NumberOfKeys)
			{
				return;
			}
			UiVk_GetButtonRgn(keycnt,&b_area);


			if(VKpad->Keys[keycnt].KeyWarning)
				Warning=VKpad->Keys[keycnt].KeyWarning(keycnt,0);

			UiVk_DrawButton(&b_area,
					Warning,VKpad->VKeyStateCallBack(keycnt,VKpad->Keys[keycnt].ShortPar),VKpad->Keys[keycnt].KeyText,
					VKpad->Keys[keycnt].TextColor,VKpad->Keys[keycnt].PressedTextColor,VKpad->Keys[keycnt].KeyFont);

			keycnt++;
		}
	}
}

/**
 * @brief Main touch respose processing function
 * @param is_long_press
 * @return
 */

bool UiVk_Process_VirtualKeypad(bool is_long_press)
{
	bool TouchProcessed=UiDriver_CheckTouchRegion(&sd.Slayout->full);	//if the area of virtual keys pressed, mark as processed for other actions

	UiArea_t bp;
	for(int i=0;i<ts.VirtualKeyPad->NumberOfKeys;i++)
	{
		UiVk_GetButtonRgn(i,&bp);
		if(UiDriver_CheckTouchRegion(&bp))
		{
			if(is_long_press && ts.VirtualKeyPad->Keys[i].LongFnc)
			{
					ts.VirtualKeyPad->Keys[i].LongFnc(i,ts.VirtualKeyPad->Keys[i].LongPar);
			}
			else if(ts.VirtualKeyPad->Keys[i].ShortFnc)
			{
					ts.VirtualKeyPad->Keys[i].ShortFnc(i,ts.VirtualKeyPad->Keys[i].ShortPar);
			}
		}
	}

	return TouchProcessed;
}

/**
 * @brief Draws Background of virtual keyboard
 * @param X_enlarge Keyboard background enlarge in pixels (real X size is enlarged by this value multiplied by 2)
 * @param Y_enlarge Keyboard background enlarge in pixels (real Y size is enlarged by this value multiplied by 2)
 */
static void UiVk_DrawBackGround()
{
	UiArea_t Ka;//drawing the background
	UiVk_GetKeybArea(&Ka);

	Ka.x-=ts.VirtualKeyPad->Backgr_Wnlarge;
	Ka.y-=ts.VirtualKeyPad->Backgr_Hnlarge;
	Ka.w+=2*ts.VirtualKeyPad->Backgr_Wnlarge;
	Ka.h+=2*ts.VirtualKeyPad->Backgr_Hnlarge;

	UiVk_DrawButtonBody(&Ka,Col_BtnForeCol,Col_BtnLightLeftTop,Col_BtnLightRightBot);

}
//End of main body of virtual keyboard
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//The real definitions of virtual keypads starts here


//DSP box VKeypad=================================================================
uint32_t prev_dsp_functions_active;	//used for virtual DSP keys redraw detections
//this array is needed because different bit definitions are used for ts.dsp_mode and ts.dsp_active, so we cannot simply pass the CallBackShort parameter
const uint32_t dsp_functions[]={0, DSP_NR_ENABLE, DSP_NOTCH_ENABLE, DSP_NOTCH_ENABLE|DSP_NR_ENABLE, DSP_MNOTCH_ENABLE, DSP_MPEAK_ENABLE};

static void UiVk_DSPVKeyCallBackShort(uint8_t KeyNum, uint32_t param)
{
	if(((ts.dsp_mode_mask&(1<<KeyNum))!=0) || (KeyNum==0))
	{
		ts.dsp_mode=param;
	}
	UiDriver_UpdateDSPmode();
}

static void UiVk_DSPVKeyCallBackLong(uint8_t KeyNum, uint32_t param)
{
	ts.dsp_mode_mask^=1<<KeyNum;
	prev_dsp_functions_active=-1;
	UiDriver_UpdateDSPmode();
}

static uint8_t UiVk_DSPVKeyCallBackWarning(uint8_t KeyNum, uint32_t param)
{
	uint8_t result=0;

	if((KeyNum==3) && mchf_display.use_spi)
		result=1;

	return result;
}

static uint8_t UiVk_DSPVKeyInitTypeDraw(uint8_t KeyNum, uint32_t param)
{
	uint8_t Keystate=Vbtn_State_Normal;
	uint32_t dsp_functions_active =UiDriver_GetActiveDSPFunctions();
	if(((ts.dsp_mode_mask&(1<<KeyNum))==0) && (KeyNum>0))
	{
		Keystate=Vbtn_State_Disabled;
	}
	else if(dsp_functions_active==dsp_functions[KeyNum])
	{
		Keystate=Vbtn_State_Pressed;
	}

	return Keystate;
}
#define col_Keys_DSP_pr RGB(0xff,0xff,0xff)		//text color when pressed
#define col_Keys_DSP_npr Black		//text color when in normal state

const VKey Keys_DSP[]={
		{.ShortFnc=UiVk_DSPVKeyCallBackShort, .ShortPar=DSP_SWITCH_OFF, .KeyText="DSP\nOFF", .TextColor=Black, .PressedTextColor=RGB(0,0xff,0xff)},
		{.ShortFnc=UiVk_DSPVKeyCallBackShort, .ShortPar=DSP_SWITCH_NR, .LongFnc=UiVk_DSPVKeyCallBackLong,.KeyText="NR", .TextColor=col_Keys_DSP_npr, .PressedTextColor=col_Keys_DSP_pr},
		{.ShortFnc=UiVk_DSPVKeyCallBackShort, .ShortPar=DSP_SWITCH_NOTCH, .LongFnc=UiVk_DSPVKeyCallBackLong,.KeyText="AUTO\nNOTCH", .TextColor=col_Keys_DSP_npr, .PressedTextColor=col_Keys_DSP_pr},
		{.ShortFnc=UiVk_DSPVKeyCallBackShort, .ShortPar=DSP_SWITCH_NR_AND_NOTCH,.KeyWarning=UiVk_DSPVKeyCallBackWarning, .LongFnc=UiVk_DSPVKeyCallBackLong,.KeyText="NR\n+NOTCH", .TextColor=col_Keys_DSP_npr, .PressedTextColor=col_Keys_DSP_pr},
		{.ShortFnc=UiVk_DSPVKeyCallBackShort, .ShortPar=DSP_SWITCH_NOTCH_MANUAL, .LongFnc=UiVk_DSPVKeyCallBackLong,.KeyText="MAN\nNOTCH", .TextColor=col_Keys_DSP_npr, .PressedTextColor=col_Keys_DSP_pr},
		{.ShortFnc=UiVk_DSPVKeyCallBackShort, .ShortPar=DSP_SWITCH_PEAK_FILTER, .LongFnc=UiVk_DSPVKeyCallBackLong,.KeyText="PEAK", .TextColor=col_Keys_DSP_npr, .PressedTextColor=col_Keys_DSP_pr}
};

const VKeypad Keypad_DSP480x320={
	.NumberOfKeys=6,
	.Rows=2,
	.Columns=3,
	.Keys=Keys_DSP,
	.KeyWidth=60,
	.KeyHeight=40,
	.KeySpacing=8,
	.Backgr_Wnlarge=4,
	.Backgr_Hnlarge=4,
	.VKeyGroupMode=Vkey_Group_OneAllowed,
	.VKeyStateCallBack=UiVk_DSPVKeyInitTypeDraw
};

const VKeypad Keypad_DSP320x240={
	.NumberOfKeys=6,
	.Rows=2,
	.Columns=3,
	.Keys=Keys_DSP,
	.KeyWidth=52,
	.KeyHeight=32,
	.KeySpacing=4,
	.Backgr_Wnlarge=4,
	.Backgr_Hnlarge=4,
	.VKeyGroupMode=Vkey_Group_OneAllowed,
	.VKeyStateCallBack=UiVk_DSPVKeyInitTypeDraw
};

void UiVk_RedrawDSPVirtualKeys()
{
	if(ts.VirtualKeysShown_flag)
	{
		uint32_t dsp_functions_active =UiDriver_GetActiveDSPFunctions();
		if(prev_dsp_functions_active!=dsp_functions_active)
		{
			prev_dsp_functions_active=dsp_functions_active;
			UiVk_DrawVKeypad();
		}
	}
}

void UiVk_DSPVirtualKeys()
{
	if(ts.VirtualKeysShown_flag && ((ts.VirtualKeyPad==&Keypad_DSP320x240) || (ts.VirtualKeyPad==&Keypad_DSP480x320)))
	{
		ts.VirtualKeysShown_flag=false;
		UiSpectrum_Init();
	}
	else
	{
		if(disp_resolution==RESOLUTION_480_320)
			ts.VirtualKeyPad=&Keypad_DSP480x320;
		else
			ts.VirtualKeyPad=&Keypad_DSP320x240;

		prev_dsp_functions_active=-1;
		UiSpectrum_Clear();
		ts.VirtualKeysShown_flag=true;	//always after UiSpectrum_Clear, because it is cleared there
		UiVk_DrawBackGround();
		UiVk_RedrawDSPVirtualKeys();
	}
}
//DSP box VKeypad END=================================================================

//Band selection VKeypad==============================================================
uint8_t prev_BndSel;
static void UiVk_BndSelVKeyCallBackShort(uint8_t KeyNum, uint32_t param)
{
	if(param==255)
	{
		UiVk_BndFreqSetVirtualKeys();		//direct frequency set pressed
	}
	else
	{
		uint16_t vfo_sel = is_vfo_b()?VFO_B:VFO_A;
		if(vfo[vfo_sel].enabled[param])
		{
			UiDriver_UpdateBand(vfo_sel,ts.band,param);
		}
	}
}

static uint8_t UiVk_BndSelVKeyInitTypeDraw(uint8_t KeyNum, uint32_t param)
{
	uint16_t vfo_sel = is_vfo_b()?VFO_B:VFO_A;
	uint8_t Keystate=Vbtn_State_Normal;
	if((!vfo[vfo_sel].enabled[param]) && (param!=255))
	{
		Keystate=Vbtn_State_Disabled;
	}
	else if(param==ts.band)
	{
		Keystate=Vbtn_State_Pressed;
	}

	return Keystate;
}
#define col_Keys_BndSel_pr RGB(0xff,0xff,0xff)		//text color when pressed
#define col_Keys_BndSel_npr Black		//text color when in normal state

const VKey Keys_BndSel480x320[]={
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=14, .KeyText="2200m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=15, .KeyText="630m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=16, .KeyText="160m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=0, .KeyText="80m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=1, .KeyText="60m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=2, .KeyText="40m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=3, .KeyText="30m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=4, .KeyText="20m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=5, .KeyText="17m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=6, .KeyText="15m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=7, .KeyText="12m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=8, .KeyText="10m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=9, .KeyText="6m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=10, .KeyText="4m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=11, .KeyText="2m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=255, .KeyText="Frequency Set", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr, .SizeX=2},
};

const VKeypad Keypad_BndSel480x320={
	.NumberOfKeys=16,
	.Rows=3,
	.Columns=6,
	.Keys=Keys_BndSel480x320,
	.KeyWidth=60,
	.KeyHeight=40,
	.KeySpacing=8,
	.Backgr_Wnlarge=4,
	.Backgr_Hnlarge=4,
	.VKeyGroupMode=Vkey_Group_OneAllowed,
	.VKeyStateCallBack=UiVk_BndSelVKeyInitTypeDraw
};

const VKey Keys_BndSel320x240[]={
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=16, .KeyText="160m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=0, .KeyText="80m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=1, .KeyText="60m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=2, .KeyText="40m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=3, .KeyText="30m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=4, .KeyText="20m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=5, .KeyText="17m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=6, .KeyText="15m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=7, .KeyText="12m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_BndSelVKeyCallBackShort, .ShortPar=8, .KeyText="10m", .TextColor=col_Keys_BndSel_npr, .PressedTextColor=col_Keys_BndSel_pr}
};

const VKeypad Keypad_BndSel320x240={
	.NumberOfKeys=10,
	.Rows=2,
	.Columns=5,
	.Keys=Keys_BndSel320x240,
	.KeyWidth=45,
	.KeyHeight=32,
	.KeySpacing=4,
	.Backgr_Wnlarge=4,
	.Backgr_Hnlarge=4,
	.VKeyGroupMode=Vkey_Group_OneAllowed,
	.VKeyStateCallBack=UiVk_BndSelVKeyInitTypeDraw
};


void UiVk_RedrawBndSelVirtualKeys()
{
	if(ts.VirtualKeysShown_flag)
	{
		if(prev_BndSel!=ts.band)
		{
			prev_BndSel=ts.band;
			UiVk_DrawVKeypad();
		}
	}
}

void UiVk_BndSelVirtualKeys()
{
	if(ts.VirtualKeysShown_flag &&
			((ts.VirtualKeyPad==&Keypad_BndSel480x320) || (ts.VirtualKeyPad==&Keypad_BndSel320x240)) )
	{
		ts.VirtualKeysShown_flag=false;
		UiSpectrum_Init();
	}
	else
	{
		if(disp_resolution==RESOLUTION_480_320)
			ts.VirtualKeyPad=&Keypad_BndSel480x320;
		else
			ts.VirtualKeyPad=&Keypad_BndSel320x240;

		prev_BndSel=255;
		UiSpectrum_Clear();
		ts.VirtualKeysShown_flag=true;	//always after UiSpectrum_Clear, because it is cleared there
		UiVk_DrawBackGround();
		UiVk_RedrawBndSelVirtualKeys();
	}
}

//Band selection direct frequency enter keypad========================================
#define UiVk_MaxFreqText 11
char UiVk_FreqEditText[UiVk_MaxFreqText];
uint8_t UiVk_FreqEditTextCntr=0;
uint8_t UiVk_prevFreqEditTextCntr=0xff;
uint8_t UiVk_BandChangeEN=0;
uint8_t UiVk_EnableFrequencySet=0;
#define UiVk_InfoWindowWidth 120
#define BackSpaceChar 8
#define EscChar 0x1b
#ifdef USE_8bit_FONT
#define FSet_font 5
#else
#define FSet_font 1
#endif


static void UiVk_FSetNumKeyUpdate()
{
	UiArea_t KeybArea;
	UiVk_GetKeybArea(&KeybArea);

	char txtEdit[14];
	char txt[14];
	int i, n=0, t=0;

	for(i=UiVk_FreqEditTextCntr;i>0;i--)
	{
		txtEdit[t++]=UiVk_FreqEditText[i-1];

		n++;
		if((n==3) && (i>1))
		{
			txtEdit[t++]='.';
			n=0;
		}
	}
	t--;
	n=0;
	for(;t>=0;t--)
	{
		txt[n++]=txtEdit[t];
	}
	txt[n]=0;

	char txtInfo[16]="               ";
	uint32_t freq=atoi(UiVk_FreqEditText);
	uint32_t textColor=RGB(0xff,0xff,0xff);
	UiVk_EnableFrequencySet=1;
	if(UiVk_BandChangeEN==0)
	{
		if(!RadioManagement_FreqIsInBand(&bandInfo[ts.band],freq*TUNE_MULT))
		{
			textColor=RGB(0xff,0xff,0x00);
			UiVk_EnableFrequencySet=0;		//disable frequency setting because out of current band is not allowed
			sprintf(txtInfo,"Out of range");
		}
	}

	UiLcdHy28_PrintTextRight(KeybArea.x+KeybArea.w-4,KeybArea.y+4,txt,textColor,Col_BtnForeCol,FSet_font);
	UiLcdHy28_PrintText(KeybArea.x+4,KeybArea.y+4,txtInfo,RGB(0xff,0xff,0x00),Col_BtnForeCol,4);
}

static void UiVk_FSetEraseFreqArea()
{
	UiArea_t KeybArea;
	UiVk_GetKeybArea(&KeybArea);
	UiLcdHy28_DrawFullRect(KeybArea.x+2+UiVk_InfoWindowWidth,KeybArea.y+4,28,KeybArea.w-UiVk_InfoWindowWidth-4,Col_BtnForeCol);
}

static void UiVk_FSetNumKeyVKeyCallBackLong(uint8_t KeyNum, uint32_t param)
{
	switch(param)
	{
	case BackSpaceChar:
		UiVk_FreqEditTextCntr=0;
		UiVk_FreqEditText[0]='\0';
		UiVk_FSetEraseFreqArea();
		UiVk_FSetNumKeyUpdate();
		break;
	}
}

static void UiVk_FSetNumKeyVKeyCallBackShort(uint8_t KeyNum, uint32_t param)
{
	bool valueChanged=false;
	switch(param)
	{
	case 1000:
	{
		int i=0;
		for(i=0;i<3;i++)
		{
			if(UiVk_FreqEditTextCntr<(UiVk_MaxFreqText-1))
			{
				UiVk_FreqEditText[UiVk_FreqEditTextCntr++]='0';
			}
			else
				break;

			UiVk_FreqEditText[UiVk_FreqEditTextCntr]=0;
			valueChanged=true;
		}
	}
	break;
	case 'B':
		UiVk_BandChangeEN^=1;
		UiVk_RedrawBndFreqSetVirtualKeys();
		valueChanged=true;
		break;
	case BackSpaceChar:
		if(UiVk_FreqEditTextCntr>0)
		{
			UiVk_FreqEditTextCntr--;
			UiVk_FreqEditText[UiVk_FreqEditTextCntr]=0;
			UiVk_FSetEraseFreqArea();
			//UiLcdHy28_DrawFullRect(KeybArea.x+2+UiVk_InfoWindowWidth,KeybArea.y+4,28,KeybArea.w-UiVk_InfoWindowWidth-4,Col_BtnForeCol);
			valueChanged=true;
		}
		break;
	case EscChar:
		ts.VirtualKeysShown_flag=false;
		UiSpectrum_Init();

		break;
	case 0x0d:
		if(UiVk_EnableFrequencySet)
		{
			uint32_t freq=atoi(UiVk_FreqEditText);
			freq*=TUNE_MULT;
			uint8_t band_scan=ts.band;

			if(UiVk_BandChangeEN)
			{
				uint8_t old_band_scan=band_scan;
				for(band_scan = 0; band_scan < MAX_BANDS; band_scan++)
				{
					if(RadioManagement_FreqIsInBand(&bandInfo[band_scan],freq))   // Is this frequency within this band?
					{
						break;  // yes - stop the scan
					}
				}
				if(band_scan==MAX_BANDS)
				{
					band_scan=old_band_scan;		//out of band, so we change frequency of current band
				}
			}

			uint16_t vfo_sel = is_vfo_b()?VFO_B:VFO_A;
			vfo[vfo_sel].band[band_scan].dial_value=freq;
			UiDriver_UpdateBand(vfo_sel,ts.band,band_scan);
		}
		break;
	default:
		if(UiVk_FreqEditTextCntr<(UiVk_MaxFreqText-1))
		{
			UiVk_FreqEditText[UiVk_FreqEditTextCntr++]=param;
			UiVk_FreqEditText[UiVk_FreqEditTextCntr]=0;
			valueChanged=true;
		}
	}

	if(valueChanged)
	{
		UiVk_FSetNumKeyUpdate();
	}
}

static uint8_t UiVk_BndFreqSetVKeyInitTypeDraw(uint8_t KeyNum, uint32_t param)
{
	uint8_t Keystate=Vbtn_State_Normal;
	if((param=='B') && (UiVk_BandChangeEN==1))
	{
		Keystate=Vbtn_State_Pressed;
	}
	return Keystate;
}
#ifdef USE_8bit_FONT
#define UiVk_FSetNumFont 5
#else
#define UiVk_FSetNumFont 1
#endif
#define UiVk_FSetNumCol (RGB(0xff,0xff,0xff))

const VKey Keys_BndFreqSet[]={
		{.ShortFnc=UiVk_FSetNumKeyVKeyCallBackShort, .ShortPar=0x31, .KeyText="1", .TextColor=UiVk_FSetNumCol, .PressedTextColor=col_Keys_BndSel_pr, .KeyFont=UiVk_FSetNumFont},
		{.ShortFnc=UiVk_FSetNumKeyVKeyCallBackShort, .ShortPar=0x32, .KeyText="2", .TextColor=UiVk_FSetNumCol, .PressedTextColor=col_Keys_BndSel_pr, .KeyFont=UiVk_FSetNumFont},
		{.ShortFnc=UiVk_FSetNumKeyVKeyCallBackShort, .ShortPar=0x33, .KeyText="3", .TextColor=UiVk_FSetNumCol, .PressedTextColor=col_Keys_BndSel_pr, .KeyFont=UiVk_FSetNumFont},
		{.ShortFnc=UiVk_FSetNumKeyVKeyCallBackShort, .ShortPar='B', .KeyText="BAND\nchange", .TextColor=0, .PressedTextColor=RGB(0xff,0xff,0x00)},
		{.ShortFnc=UiVk_FSetNumKeyVKeyCallBackShort, .ShortPar=BackSpaceChar, .LongFnc=UiVk_FSetNumKeyVKeyCallBackLong,.LongPar=BackSpaceChar,.KeyText=">>", .TextColor=UiVk_FSetNumCol, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_FSetNumKeyVKeyCallBackShort, .ShortPar=0x34, .KeyText="4", .TextColor=UiVk_FSetNumCol, .PressedTextColor=col_Keys_BndSel_pr, .KeyFont=UiVk_FSetNumFont},
		{.ShortFnc=UiVk_FSetNumKeyVKeyCallBackShort, .ShortPar=0x35, .KeyText="5", .TextColor=UiVk_FSetNumCol, .PressedTextColor=col_Keys_BndSel_pr, .KeyFont=UiVk_FSetNumFont},
		{.ShortFnc=UiVk_FSetNumKeyVKeyCallBackShort, .ShortPar=0x36, .KeyText="6", .TextColor=UiVk_FSetNumCol, .PressedTextColor=col_Keys_BndSel_pr, .KeyFont=UiVk_FSetNumFont},
		{.ShortFnc=UiVk_FSetNumKeyVKeyCallBackShort, .ShortPar=1000, .KeyText="000", .TextColor=UiVk_FSetNumCol, .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_FSetNumKeyVKeyCallBackShort, .ShortPar=EscChar, .KeyText="CLOSE", .TextColor=RGB(0xff,0xe0,0x00), .PressedTextColor=col_Keys_BndSel_pr},
		{.ShortFnc=UiVk_FSetNumKeyVKeyCallBackShort, .ShortPar=0x37, .KeyText="7", .TextColor=UiVk_FSetNumCol, .PressedTextColor=col_Keys_BndSel_pr, .KeyFont=UiVk_FSetNumFont},
		{.ShortFnc=UiVk_FSetNumKeyVKeyCallBackShort, .ShortPar=0x38, .KeyText="8", .TextColor=UiVk_FSetNumCol, .PressedTextColor=col_Keys_BndSel_pr, .KeyFont=UiVk_FSetNumFont},
		{.ShortFnc=UiVk_FSetNumKeyVKeyCallBackShort, .ShortPar=0x39, .KeyText="9", .TextColor=UiVk_FSetNumCol, .PressedTextColor=col_Keys_BndSel_pr, .KeyFont=UiVk_FSetNumFont},
		{.ShortFnc=UiVk_FSetNumKeyVKeyCallBackShort, .ShortPar=0x30, .KeyText="0", .TextColor=UiVk_FSetNumCol, .PressedTextColor=col_Keys_BndSel_pr, .KeyFont=UiVk_FSetNumFont},
		{.ShortFnc=UiVk_FSetNumKeyVKeyCallBackShort, .ShortPar=0x0d, .KeyText="OK", .TextColor=RGB(0xc0,0xff,0xc0), .PressedTextColor=col_Keys_BndSel_pr},
};

const VKeypad Keypad_BndFreqSet480x320={
	.NumberOfKeys=15,
	.Rows=3,
	.Columns=5,
	.Keys=Keys_BndFreqSet,
	.KeyWidth=60,
	.KeyHeight=36,
	.KeySpacing=8,
	.Backgr_Wnlarge=4,
	.Backgr_Hnlarge=4,
	.VKeyGroupMode=Vkey_Group_OneAllowed,
	.VKeyStateCallBack=UiVk_BndFreqSetVKeyInitTypeDraw,
	.YtopMargin=38
};

void UiVk_RedrawBndFreqSetVirtualKeys()
{
	if(ts.VirtualKeysShown_flag)
	{
		//if(UiVk_prevFreqEditTextCntr!=UiVk_FreqEditTextCntr)
		{
			UiVk_prevFreqEditTextCntr=UiVk_FreqEditTextCntr;
			UiVk_DrawVKeypad();
		}
	}
}

void UiVk_BndFreqSetVirtualKeys()
{
	if(ts.VirtualKeysShown_flag && (ts.VirtualKeyPad==&Keypad_BndFreqSet480x320))
	{
		ts.VirtualKeysShown_flag=false;
		UiSpectrum_Init();
	}
	else
	{
		if(disp_resolution==RESOLUTION_480_320)
			ts.VirtualKeyPad=&Keypad_BndFreqSet480x320;

		UiVk_FreqEditTextCntr=0;

		for(int i=0;i<UiVk_MaxFreqText;i++)
		{
			UiVk_FreqEditText[i]=0;
		}
		UiVk_prevFreqEditTextCntr=0xff;

		UiSpectrum_Clear();
		ts.VirtualKeysShown_flag=true;	//always after UiSpectrum_Clear, because it is cleared there
		UiVk_DrawBackGround();
		UiVk_RedrawBndFreqSetVirtualKeys();
	}
}
//Band selection VKeypad END==========================================================

