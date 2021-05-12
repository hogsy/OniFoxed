// ======================================================================
// WM_CheckBox.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_WindowManager.h"
#include "BFW_TextSystem.h"
#include "BFW_LocalInput.h"

#include "WM_CheckBox.h"

// ======================================================================
// defines
// ======================================================================
#define WMcCheckBox_Buffer	2

// ======================================================================
// enums
// ======================================================================
enum
{
	WMcCheckBoxFlag_None			= 0x0000 << 16,
	WMcCheckBoxFlag_On				= 0x0001 << 16
	
};

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
WMiCheckBox_Create(
	WMtCheckBox				*inCheckBox)
{
	UUtUns32				checkbox_data;
	
	// initialize
	checkbox_data = WMrWindow_GetLong(inCheckBox, 0);
	checkbox_data = 0;
	WMrWindow_SetLong(inCheckBox, 0, checkbox_data);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
WMiCheckBox_HandleMouseEvent(
	WMtCheckBox				*inCheckBox,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtRect					checkbox_rect;
	UUtBool					mouse_over_checkbox;
	IMtPoint2D				global_mouse;
	UUtUns32				checkbox_data;
	UUtBool					notify;
	
	// get the checkbox data
	checkbox_data = WMrWindow_GetLong(inCheckBox, 0);
	
	// get the mouse location
	global_mouse.x = (UUtInt16)UUmHighWord(inParam1);
	global_mouse.y = (UUtInt16)UUmLowWord(inParam1);
	
	// determine if the mouse is over the checkbox
	WMrWindow_GetRect(inCheckBox, &checkbox_rect);
	mouse_over_checkbox = IMrRect_PointIn(&checkbox_rect, &global_mouse);
	
	// handle the message
	notify = UUcFalse;
	switch (inMessage)
	{
		case WMcMessage_LMouseDown:
			WMrWindow_SetFocus(inCheckBox);
			WMrWindow_CaptureMouse(inCheckBox);
			checkbox_data |= LIcMouseState_LButtonDown;
		break;
		
		case WMcMessage_LMouseUp:
			WMrWindow_CaptureMouse(NULL);
			if ((mouse_over_checkbox) &&
				(UUmLowWord(checkbox_data) == LIcMouseState_LButtonDown))
			{
				if (checkbox_data & WMcCheckBoxFlag_On)
				{
					checkbox_data &= ~WMcCheckBoxFlag_On;
				}
				else
				{
					checkbox_data |= WMcCheckBoxFlag_On;
				}
				
				notify = UUcTrue;
			}
			checkbox_data &= ~LIcMouseState_LButtonDown;
		break;
	}
	
	// save the checkbox data
	WMrWindow_SetLong(inCheckBox, 0, checkbox_data);

	if (notify)
	{
		// tell the parent
		WMrMessage_Send(
			WMrWindow_GetParent(inCheckBox),
			WMcMessage_Command,
			UUmMakeLong(WMcNotify_Click, WMrWindow_GetID(inCheckBox)),
			(UUtUns32)inCheckBox);
	}
}

// ----------------------------------------------------------------------
static void
WMiCheckBox_Paint(
	WMtCheckBox				*inCheckBox)
{
	DCtDrawContext			*draw_context;
	PStPartSpecUI			*partspec_ui;
	IMtPoint2D				dest;
	UUtInt16				part_width;
	UUtInt16				part_height;
	UUtUns32				checkbox_data;
	UUtInt16				height;
	UUtUns32				style;
	PStPartSpec				*part;
	UUtBool					enabled;
	
	// get the checkbox_data
	checkbox_data = WMrWindow_GetLong(inCheckBox, 0);
	
	// get partspec ui
	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) { return; }
	
	// get the window size
	WMrWindow_GetSize(inCheckBox, NULL, &height);
	
	// get the partspec size
	PSrPartSpec_GetSize(
		partspec_ui->checkbox_on,
		PScPart_LeftTop,
		&part_width,
		&part_height);
	
	// calculate the location for the part	
	dest.x = 0;
	dest.y = (height - part_height) >> 1;

	draw_context = DCrDraw_Begin(inCheckBox);
	
	enabled = WMrWindow_GetEnabled(inCheckBox);
	
	// draw the checkbox
	if (checkbox_data & WMcCheckBoxFlag_On)
	{
		part = partspec_ui->checkbox_on;
	}
	else
	{
		part = partspec_ui->checkbox_off;
	}	

	DCrDraw_PartSpec(
		draw_context,
		part,
		PScPart_LeftTop,
		&dest,
		part_width,
		part_height,
		enabled ? (UUtUns16)WMcAlpha_Enabled : (UUtUns16)WMcAlpha_Disabled);
	
	// draw the title
	style = WMrWindow_GetStyle(inCheckBox);
	if (style & WMcCheckBoxStyle_HasTitle)
	{
		UUtRect			bounds;
		TStFontInfo		font_info;
		
		WMrWindow_GetFontInfo(inCheckBox, &font_info);
		DCrText_SetFontInfo(&font_info);
		
		dest.x = part_width + WMcCheckBox_Buffer;
		
		WMrWindow_GetClientRect(inCheckBox, &bounds);
		
		DCrText_SetShade(enabled ? font_info.font_shade : IMcShade_Gray50);
		DCrText_SetFormat(TSc_HLeft | TSc_VCenter);
		DCrDraw_String(draw_context, WMrWindow_GetTitlePtr(inCheckBox), &bounds, &dest);
	}
	
	DCrDraw_End(draw_context, inCheckBox);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
WMiCheckBox_Callback(
	WMtCheckBox				*inCheckBox,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtError				error;
	UUtUns32				result;
	UUtUns32				checkbox_data;
	
	// get the checkbox_data
	checkbox_data = WMrWindow_GetLong(inCheckBox, 0);
	
	switch(inMessage)
	{
		case WMcMessage_NC_HitTest:
		return WMcWindowArea_Client;
		
		case WMcMessage_Create:
			error = WMiCheckBox_Create(inCheckBox);
			if (error != UUcError_None)
			{
				return WMcResult_Error;
			}
		return WMcResult_Handled;
		
		case WMcMessage_LMouseDown:
		case WMcMessage_LMouseUp:
			WMiCheckBox_HandleMouseEvent(
				inCheckBox,
				inMessage,
				inParam1,
				inParam2);
		return WMcResult_Handled;

		case WMcMessage_Paint:
			WMiCheckBox_Paint(inCheckBox);
		return WMcResult_Handled;
		
		case WMcMessage_GetValue:
			if (checkbox_data & WMcCheckBoxFlag_On)
			{
				result = (UUtUns32)UUcTrue;
			}
			else
			{
				result = (UUtUns32)UUcFalse;
			}
		return result;

		case CBcMessage_SetCheck:
			if ((UUtBool)inParam1)
			{
				checkbox_data |= WMcCheckBoxFlag_On;
			}
			else
			{
				checkbox_data &= ~WMcCheckBoxFlag_On;
			}
			WMrWindow_SetLong(inCheckBox, 0, checkbox_data);
		return WMcResult_Handled;
	}
	
	return WMrWindow_DefaultCallback(inCheckBox, inMessage, inParam1, inParam2);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
WMrCheckBox_Initialize(
	void)
{
	UUtError				error;
	WMtWindowClass			window_class;
	
	// register the window class
	UUrMemory_Clear(&window_class, sizeof(WMtWindowClass));
	window_class.type = WMcWindowType_CheckBox;
	window_class.callback = WMiCheckBox_Callback;
	window_class.private_data_size = sizeof(UUtUns32);
	
	error = WMrWindowClass_Register(&window_class);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

void 
WMrCheckBox_SetCheck(WMtCheckBox *inCheckBox, UUtBool inValue)
{
	WMrMessage_Send(inCheckBox, CBcMessage_SetCheck, inValue, 0);
	
	return;
}

UUtBool
WMrCheckBox_GetCheck(WMtCheckBox *inCheckBox)
{
	UUtBool value;

	value = (UUtBool) WMrMessage_Send(inCheckBox, WMcMessage_GetValue, 0, 0);

	return value;
}
