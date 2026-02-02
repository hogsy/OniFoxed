// ======================================================================
// WM_Button.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_WindowManager.h"
#include "BFW_TextSystem.h"
#include "BFW_LocalInput.h"
#include "BFW_SoundSystem2.h"

#include "WM_Button.h"

// ======================================================================
// defines
// ======================================================================
#define WMcButton_ClickSound	"button_click"

// ======================================================================
// enums
// ======================================================================
enum
{
	WMcButtonFlag_None			= 0x0000 << 16,
	WMcButtonFlag_Hilite		= 0x0001 << 16,
	WMcButtonFlag_Toggle		= 0x0002 << 16

};

// ======================================================================
// globals
// ======================================================================
static SStImpulse			*WMgWindow_ClickSound;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
WMiButton_Create(
	WMtButton				*inButton)
{
	UUtUns32				button_data;

	// initialize
	button_data = WMrWindow_GetLong(inButton, 0);
	button_data = LIcMouseState_None;
	WMrWindow_SetLong(inButton, 0, button_data);

	if ((WMrWindow_GetStyle(inButton) & WMcButtonStyle_Default) != 0)
	{
		WMrMessage_Send(
			WMrWindow_GetParent(inButton),
			WMcMessage_SetDefaultID,
			WMrWindow_GetID(inButton),
			0);
	}

	if (WMgWindow_ClickSound == NULL)
	{
		WMgWindow_ClickSound = SSrImpulse_GetByName(WMcButton_ClickSound);
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
WMiButton_HandleMouseEvent(
	WMtButton				*inButton,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtRect					button_rect;
	UUtBool					mouse_over_button;
	IMtPoint2D				global_mouse;
	UUtUns32				button_data;
	UUtUns32				style;

	// get the button data
	button_data = WMrWindow_GetLong(inButton, 0);

	// get the button style
	style = WMrWindow_GetStyle(inButton);

	// get the mouse location
	global_mouse.x = (UUtInt16)UUmHighWord(inParam1);
	global_mouse.y = (UUtInt16)UUmLowWord(inParam1);

	// determine if the mouse is over the button
	WMrWindow_GetRect(inButton, &button_rect);
	mouse_over_button = IMrRect_PointIn(&button_rect, &global_mouse);

	switch (inMessage)
	{
		case WMcMessage_MouseMove:
			if ((UUmLowWord(button_data) == LIcMouseState_LButtonDown) &&
				(inParam2 & LIcMouseState_LButtonDown))
			{
				if (mouse_over_button)
				{
					button_data |= WMcButtonFlag_Hilite;
				}
				else
				{
					button_data &= ~WMcButtonFlag_Hilite;
				}
				WMrWindow_SetLong(inButton, 0, button_data);
			}
		break;

		case WMcMessage_LMouseDown:
			// save the button data changes
			button_data |= WMcButtonFlag_Hilite;
			button_data |= LIcMouseState_LButtonDown;
			WMrWindow_SetLong(inButton, 0, button_data);

			WMrWindow_SetFocus(inButton);
			WMrWindow_CaptureMouse(inButton);
		break;

		case WMcMessage_LMouseUp:
		{
			UUtBool						was_down;

			was_down = (UUmLowWord(button_data) == LIcMouseState_LButtonDown);

			// save the button data changes
			button_data &= ~WMcButtonFlag_Hilite;
			button_data &= ~LIcMouseState_LButtonDown;
			WMrWindow_SetLong(inButton, 0, button_data);

			WMrWindow_CaptureMouse(NULL);
			if ((mouse_over_button) && (was_down == UUcTrue))
			{
#if SHIPPING_VERSION
				if (WMgWindow_ClickSound != NULL)
				{
					SSrImpulse_Play(WMgWindow_ClickSound, NULL, NULL, NULL, NULL);
				}
#endif

				// tell the parent
				WMrMessage_Send(
					WMrWindow_GetParent(inButton),
					WMcMessage_Command,
					UUmMakeLong(WMcNotify_Click, WMrWindow_GetID(inButton)),
					(UUtUns32)inButton);
			}
		}
		break;
	}
}

// ----------------------------------------------------------------------
static void
WMiButton_Paint(
	WMtButton				*inButton)
{
	DCtDrawContext			*draw_context;
	PStPartSpecUI			*partspec_ui;
	IMtPoint2D				dest;
	UUtInt16				width;
	UUtInt16				height;
	UUtUns32				button_data;
	UUtUns32				style;
	PStPartSpec				*part;
	UUtBool					enabled;

	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) return;

	draw_context = DCrDraw_Begin(inButton);

	enabled = WMrWindow_GetEnabled(inButton);
	style = WMrWindow_GetStyle(inButton);

	// set the dest
	dest.x = 0;
	dest.y = 0;

	// get the window rect
	WMrWindow_GetSize(inButton, &width, &height);

	// get the button data
	button_data = WMrWindow_GetLong(inButton, 0);

	// draw the background
	if (style & WMcButtonStyle_HasBackground)
	{
		if (((button_data & WMcButtonFlag_Hilite) != 0) ||
			((button_data & WMcButtonFlag_Toggle) != 0))
		{
			part = partspec_ui->button_on;
		}
		else
		{
			if ((style & WMcButtonStyle_Default) != 0)
			{
				part = partspec_ui->default_button;
			}
			else
			{
				part = partspec_ui->button_off;
			}
		}

		DCrDraw_PartSpec(
			draw_context,
			part,
			PScPart_All,
			&dest,
			width,
			height,
			enabled ? (UUtUns16)WMcAlpha_Enabled : (UUtUns16)WMcAlpha_Disabled);
	}

	// draw the title
	if (style & WMcButtonStyle_HasTitle)
	{
		UUtRect			bounds;
		TStFontInfo		font_info;

		WMrWindow_GetFontInfo(inButton, &font_info);
		DCrText_SetFontInfo(&font_info);

		WMrWindow_GetClientRect(inButton, &bounds);

		DCrText_SetShade(enabled ? font_info.font_shade : IMcShade_Gray50);
		DCrText_SetFormat(TSc_HCenter | TSc_VCenter);
		DCrDraw_String(draw_context, WMrWindow_GetTitlePtr(inButton), &bounds, &dest);
	}

	DCrDraw_End(draw_context, inButton);
}

// ----------------------------------------------------------------------
static void
WMiButton_HandleSetToggle(
	WMtButton				*inButton,
	UUtBool					inToggle)
{
	UUtUns32				style;
	UUtUns32				button_data;

	// make sure this is a toggle button
	style = WMrWindow_GetStyle(inButton);
	if ((style & WMcButtonStyle_Toggle) == 0) { return; }

	// get the button data
	button_data = WMrWindow_GetLong(inButton, 0);

	if (inToggle == UUcTrue)
	{
		button_data |= WMcButtonFlag_Toggle;
	}
	else
	{
		button_data &= ~WMcButtonFlag_Toggle;
	}

	// set the button data
	WMrWindow_SetLong(inButton, 0, button_data);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
WMiButton_Callback(
	WMtButton				*inButton,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtError				error;

	switch(inMessage)
	{
		case WMcMessage_NC_HitTest:
		return WMcWindowArea_Client;

		case WMcMessage_Create:
			error = WMiButton_Create(inButton);
			if (error != UUcError_None)
			{
				return WMcResult_Error;
			}
		return WMcResult_Handled;

		case WMcMessage_MouseMove:
		case WMcMessage_LMouseDown:
		case WMcMessage_LMouseUp:
			WMiButton_HandleMouseEvent(inButton, inMessage, inParam1, inParam2);
		return WMcResult_Handled;

		case WMcMessage_Paint:
			WMiButton_Paint(inButton);
		return WMcResult_Handled;

		case WMcMessage_GetDialogCode:
			if ((WMrWindow_GetStyle(inButton) & WMcButtonStyle_Default) != 0)
			{
				return WMcDialogCode_Default;
			}
		break;

		case TBcMessage_SetToggle:
			WMiButton_HandleSetToggle(inButton, (UUtBool)inParam1);
		return WMcResult_Handled;
	}

	return WMrWindow_DefaultCallback(inButton, inMessage, inParam1, inParam2);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
WMrButton_Initialize(
	void)
{
	UUtError				error;
	WMtWindowClass			window_class;

	// register the window class
	UUrMemory_Clear(&window_class, sizeof(WMtWindowClass));
	window_class.type = WMcWindowType_Button;
	window_class.callback = WMiButton_Callback;
	window_class.private_data_size = sizeof(UUtUns32);

	error = WMrWindowClass_Register(&window_class);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtBool
WMrButton_IsHighlighted(
	WMtWindow *inWindow)
{
	UUtUns32 button_data;

	button_data = WMrWindow_GetLong(inWindow, 0);
	return ((button_data & WMcButtonFlag_Hilite) > 0);
}
