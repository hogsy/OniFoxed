// ======================================================================
// WM_Box.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_WindowManager.h"

#include "WM_Box.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
WMiBox_Paint(
	WMtBox					*inBox)
{
	DCtDrawContext			*draw_context;
	PStPartSpecUI			*partspec_ui;
	IMtPoint2D				dest;
	UUtInt16				width;
	UUtInt16				height;
	UUtUns32				style;
	UUtBool					enabled;
	
	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) { return; }
	
	WMrWindow_GetSize(inBox, &width, &height);
	style = WMrWindow_GetStyle(inBox);
	
	// set dest
	dest.x = 0;
	dest.y = 0;
	
	if (style & WMcBoxStyle_HasTitle)
	{
		dest.y = DCrText_GetLineHeight() >> 1;
		height -= dest.y;
	}
	
	draw_context = DCrDraw_Begin(inBox);
	
	enabled = WMrWindow_GetEnabled(inBox);
	
	DCrDraw_PartSpec(
		draw_context,
		partspec_ui->outline,
		PScPart_All,
		&dest,
		width,
		height,
		enabled ? (UUtUns16)WMcAlpha_Enabled : (UUtUns16)WMcAlpha_Disabled);
	
	if (style & WMcBoxStyle_HasTitle)
	{
		UUtRect				bounds;
		TStFontInfo			font_info;
		
		WMrWindow_GetFontInfo(inBox, &font_info);
		DCrText_SetFontInfo(&font_info);
		
		DCrText_GetStringRect(WMrWindow_GetTitlePtr(inBox), &bounds);
		IMrRect_Offset(&bounds, 5, 0);
		
		dest.x = bounds.left;
		dest.y = bounds.top;
		
		if (style & WMcBoxStyle_HasBackground)
		{
			width = bounds.right - bounds.left;
			height = bounds.bottom - bounds.top;
			
			DCrDraw_PartSpec(
				draw_context,
				partspec_ui->background,
				PScPart_MiddleMiddle,
				&dest,
				width,
				height,
				M3cMaxAlpha);
		}
		
		DCrText_SetShade(enabled ? font_info.font_shade : IMcShade_Gray50);
		DCrText_SetFormat(TSc_HLeft | TSc_VCenter);
		DCrDraw_String(draw_context, WMrWindow_GetTitlePtr(inBox), &bounds, &dest);
	}
	
	DCrDraw_End(draw_context, inBox);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
WMiBox_Callback(
	WMtBox					*inBox,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	switch(inMessage)
	{
		case WMcMessage_Paint:
			WMiBox_Paint(inBox);
		return WMcResult_Handled;
	}
	
	return WMrWindow_DefaultCallback(inBox, inMessage, inParam1, inParam2);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
WMrBox_Initialize(
	void)
{
	UUtError				error;
	WMtWindowClass			window_class;
	
	// register the window class
	UUrMemory_Clear(&window_class, sizeof(WMtWindowClass));
	window_class.type = WMcWindowType_Box;
	window_class.callback = WMiBox_Callback;
	window_class.private_data_size = 0;
	
	error = WMrWindowClass_Register(&window_class);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}