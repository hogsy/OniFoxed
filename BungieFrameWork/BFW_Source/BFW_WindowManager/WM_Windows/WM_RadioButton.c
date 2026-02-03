// ======================================================================
// WM_RadioButton.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_WindowManager.h"
#include "BFW_TextSystem.h"
#include "BFW_LocalInput.h"

#include "WM_RadioButton.h"

// ======================================================================
// defines
// ======================================================================
#define WMcRadioButton_Buffer		2

// ======================================================================
// enums
// ======================================================================
enum
{
	WMcRadioButtonFlag_None			= 0x0000 << 16,
	WMcRadioButtonFlag_On			= 0x0001 << 16

};

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
WMiRadioButton_Create(
	WMtRadioButton			*inRadioButton)
{
	UUtUns32				radiobutton_data;

	// initialize
	radiobutton_data = WMrWindow_GetLong(inRadioButton, 0);
	radiobutton_data = 0;
	WMrWindow_SetLong(inRadioButton, 0, radiobutton_data);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
WMiRadioButton_HandleMouseEvent(
	WMtRadioButton			*inRadioButton,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtRect					radiobutton_rect;
	UUtBool					mouse_over_radiobutton;
	IMtPoint2D				global_mouse;
	UUtUns32				radiobutton_data;
	UUtBool					notify;

	// get the mouse location
	global_mouse.x = (UUtInt16)UUmHighWord(inParam1);
	global_mouse.y = (UUtInt16)UUmLowWord(inParam1);

	// determine if the mouse is over the radiobutton
	WMrWindow_GetRect(inRadioButton, &radiobutton_rect);
	mouse_over_radiobutton = IMrRect_PointIn(&radiobutton_rect, &global_mouse);

	notify = UUcFalse;

	switch (inMessage)
	{
		case WMcMessage_LMouseDown:
			WMrWindow_SetFocus(inRadioButton);
			WMrWindow_CaptureMouse(inRadioButton);
			radiobutton_data = WMrWindow_GetLong(inRadioButton, 0);
			radiobutton_data |= LIcMouseState_LButtonDown;
		break;

		case WMcMessage_LMouseUp:
			WMrWindow_CaptureMouse(NULL);
			radiobutton_data = WMrWindow_GetLong(inRadioButton, 0);
			if ((mouse_over_radiobutton) &&
				(UUmLowWord(radiobutton_data) == LIcMouseState_LButtonDown))
			{
				notify = UUcTrue;
			}

			// the radio button data may have changed
			radiobutton_data = WMrWindow_GetLong(inRadioButton, 0);
			radiobutton_data &= ~LIcMouseState_LButtonDown;
		break;
	}

	// save the checkbox data
	WMrWindow_SetLong(inRadioButton, 0, radiobutton_data);

	// tell the parent
	if (notify)
	{
		WMrMessage_Send(
			WMrWindow_GetParent(inRadioButton),
			WMcMessage_Command,
			UUmMakeLong(WMcNotify_Click, WMrWindow_GetID(inRadioButton)),
			(UUtUns32)inRadioButton);
	}
}

// ----------------------------------------------------------------------
static void
WMiRadioButton_Paint(
	WMtRadioButton			*inRadioButton)
{
	DCtDrawContext			*draw_context;
	PStPartSpecUI			*partspec_ui;
	IMtPoint2D				dest;
	UUtInt16				part_width;
	UUtInt16				part_height;
	UUtUns32				radiobutton_data;
	UUtInt16				height;
	PStPartSpec				*part;
	UUtUns32				style;
	UUtBool					enabled;

	// get the radiobutton_data
	radiobutton_data = WMrWindow_GetLong(inRadioButton, 0);

	// get partspec ui
	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) { return; }

	// get the window size
	WMrWindow_GetSize(inRadioButton, NULL, &height);

	// get the partspec size
	PSrPartSpec_GetSize(
		partspec_ui->radiobutton_on,
		PScPart_LeftTop,
		&part_width,
		&part_height);

	// calculate the location for the part
	dest.x = 0;
	dest.y = (height - part_height) >> 1;

	enabled = WMrWindow_GetEnabled(inRadioButton);

	draw_context = DCrDraw_Begin(inRadioButton);

	// draw the checkbox
	if (radiobutton_data & WMcRadioButtonFlag_On)
	{
		part = partspec_ui->radiobutton_on;
	}
	else
	{
		part = partspec_ui->radiobutton_off;
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
	style = WMrWindow_GetStyle(inRadioButton);
	if (style & WMcRadioButtonStyle_HasTitle)
	{
		UUtRect			bounds;
		TStFontInfo		font_info;

		WMrWindow_GetFontInfo(inRadioButton, &font_info);
		DCrText_SetFontInfo(&font_info);

		dest.x = part_width + WMcRadioButton_Buffer;

		WMrWindow_GetClientRect(inRadioButton, &bounds);
		DCrText_SetShade(enabled ? font_info.font_shade : IMcShade_Gray50);
		DCrText_SetFormat(TSc_HLeft | TSc_VCenter);
		DCrDraw_String(draw_context, WMrWindow_GetTitlePtr(inRadioButton), &bounds, &dest);
	}

	DCrDraw_End(draw_context, inRadioButton);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
WMiRadioButton_Callback(
	WMtRadioButton			*inRadioButton,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtError				error;
	UUtUns32				result;
	UUtUns32				radiobutton_data;

	// get the radiobutton_data
	radiobutton_data = WMrWindow_GetLong(inRadioButton, 0);

	switch(inMessage)
	{
		case WMcMessage_NC_HitTest:
		return WMcWindowArea_Client;

		case WMcMessage_Create:
			error = WMiRadioButton_Create(inRadioButton);
			if (error != UUcError_None)
			{
				return WMcResult_Error;
			}
		return WMcResult_Handled;

		case WMcMessage_LMouseDown:
		case WMcMessage_LMouseUp:
			WMiRadioButton_HandleMouseEvent(
				inRadioButton,
				inMessage,
				inParam1,
				inParam2);
		return WMcResult_Handled;

		case WMcMessage_Paint:
			WMiRadioButton_Paint(inRadioButton);
		return WMcResult_Handled;

		case WMcMessage_GetValue:
			if (radiobutton_data & WMcRadioButtonFlag_On)
			{
				result = (UUtUns32)UUcTrue;
			}
			else
			{
				result = (UUtUns32)UUcFalse;
			}
		return result;

		case RBcMessage_SetCheck:
			if ((UUtBool)inParam1)
			{
				radiobutton_data |= WMcRadioButtonFlag_On;
			}
			else
			{
				radiobutton_data &= ~WMcRadioButtonFlag_On;
			}
			WMrWindow_SetLong(inRadioButton, 0, radiobutton_data);
		return WMcResult_Handled;
	}

	return WMrWindow_DefaultCallback(inRadioButton, inMessage, inParam1, inParam2);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
WMrRadioButton_Initialize(
	void)
{
	UUtError				error;
	WMtWindowClass			window_class;

	// register the window class
	UUrMemory_Clear(&window_class, sizeof(WMtWindowClass));
	window_class.type = WMcWindowType_RadioButton;
	window_class.callback = WMiRadioButton_Callback;
	window_class.private_data_size = sizeof(UUtUns32);

	error = WMrWindowClass_Register(&window_class);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}
