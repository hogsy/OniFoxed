// ======================================================================
// WM_Text.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_WindowManager.h"
#include "BFW_TextSystem.h"

#include "WM_Text.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
WMiText_Paint(
	WMtText					*inText)
{
	DCtDrawContext			*draw_context;
	UUtRect					bounds;
	IMtPoint2D				dest;
	UUtUns32				style;
	TStFormat				format;
	
	draw_context = DCrDraw_Begin(inText);
		
	// get the style flags
	style = WMrWindow_GetStyle(inText);
	if (style & WMcTextStyle_OwnerDraw)
	{
		WMtDrawItem					draw_item;
		
		// set up the owner draw struct
		draw_item.draw_context		= draw_context;
		draw_item.window			= inText;
		draw_item.window_id			= WMrWindow_GetID(inText);
		draw_item.state				= WMcDrawItemState_None;
		draw_item.item_id 			= 0;
		WMrWindow_GetClientRect(inText, &draw_item.rect);
		draw_item.string			= WMrWindow_GetTitlePtr(inText);
		draw_item.data				= 0;
		
		WMrMessage_Send(
			WMrWindow_GetOwner(inText),
			WMcMessage_DrawItem,
			(UUtUns32)draw_item.window_id,
			(UUtUns32)&draw_item);
	}
	else
	{
		TStFontInfo				font_info;
		
		// set the format
		format = TSc_HLeft;
		if (style & WMcTextStyle_HCenter)
		{
			format = TSc_HCenter;
		}
		else if (style & WMcTextStyle_HRight)
		{
			format = TSc_HRight;
		}
		
		if (style & WMcTextStyle_VCenter)
		{
			format |= TSc_VCenter;
		}
		
		// set dest
		dest.x = 0;
		dest.y = DCrText_GetAscendingHeight();
		
		WMrWindow_GetFontInfo(inText, &font_info);
		DCrText_SetFontInfo(&font_info);
		
		WMrWindow_GetClientRect(inText, &bounds);
		DCrText_SetShade(WMrWindow_GetEnabled(inText) ? font_info.font_shade : IMcShade_Gray50);
		DCrText_SetFormat(format);
		DCrDraw_String(draw_context, WMrWindow_GetTitlePtr(inText), &bounds, &dest);
	}
		
	DCrDraw_End(draw_context, inText);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
WMiText_Callback(
	WMtText					*inText,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	switch(inMessage)
	{
		case WMcMessage_Paint:
			WMiText_Paint(inText);
		return WMcResult_Handled;
	}
	
	return WMrWindow_DefaultCallback(inText, inMessage, inParam1, inParam2);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
WMrText_Initialize(
	void)
{
	UUtError				error;
	WMtWindowClass			window_class;
	
	// register the window class
	UUrMemory_Clear(&window_class, sizeof(WMtWindowClass));
	window_class.type = WMcWindowType_Text;
	window_class.callback = WMiText_Callback;
	window_class.private_data_size = 0;
	
	error = WMrWindowClass_Register(&window_class);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtBool
WMrText_SetShade(
	WMtWindow				*inWindow,
	IMtShade				inShade)
{
	UUtBool					result;
	TStFontInfo				font_info;
	
	result = WMrWindow_GetFontInfo(inWindow, &font_info);
	if (result != UUcTrue) { return result; }
	
	font_info.font_shade = inShade;
	
	result = WMrWindow_SetFontInfo(inWindow, &font_info);
	if (result != UUcTrue) { return result; }
	
	return UUcTrue;
}