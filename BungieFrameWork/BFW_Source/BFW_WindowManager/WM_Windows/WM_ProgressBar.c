// ======================================================================
// WM_ProgressBar.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_WindowManager.h"

#include "WM_ProgressBar.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
WMiProgressBar_Create(
	WMtProgressBar			*inProgressBar)
{
	WMrWindow_SetLong(inProgressBar, 0, 0);
	
	return WMcResult_Handled;
}

// ----------------------------------------------------------------------
static void
WMiProgressBar_Paint(
	WMtProgressBar			*inProgressBar)
{
	DCtDrawContext			*draw_context;
	UUtUns32				progressbar_data;
	PStPartSpecUI			*partspec_ui;
	IMtPoint2D				dest;
	UUtInt16				window_width;
	UUtInt16				window_height;
	UUtBool					enabled;
	
	// get the progressbar data
	progressbar_data = WMrWindow_GetLong(inProgressBar, 0);
	
	// get the partspec ui
	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) { return; }
	
	// get the width and height of the window
	WMrWindow_GetSize(inProgressBar, &window_width, &window_height);
	
	draw_context = DCrDraw_Begin(inProgressBar);
	
	enabled = WMrWindow_GetEnabled(inProgressBar);
	
	dest.x = 0;
	dest.y = 0;
	
	DCrDraw_PartSpec(
		draw_context,
		partspec_ui->progressbar_track,
		PScPart_All,
		&dest,
		window_width,
		window_height,
		enabled ? (UUtUns16)WMcAlpha_Enabled : (UUtUns16)WMcAlpha_Disabled);
	
	if (enabled && (progressbar_data > 0))
	{
		UUtInt16			lt_part_width;
		UUtInt16			lt_part_height;
		UUtInt16			rb_part_width;
		UUtInt16			rb_part_height;
		UUtInt16			width;
		
		PSrPartSpec_GetSize(
			partspec_ui->progressbar_track,
			PScPart_LeftTop,
			&lt_part_width,
			&lt_part_height);
		
		PSrPartSpec_GetSize(
			partspec_ui->progressbar_track,
			PScPart_RightBottom,
			&rb_part_width,
			&rb_part_height);
		
		dest.x = lt_part_width - 1;
		dest.y = lt_part_height - 1;
		
		window_width = window_width - (lt_part_width + rb_part_width) + 2;
		window_height = window_height - (lt_part_height + rb_part_height) + 2;
		
		width = ((UUtInt16)progressbar_data * window_width) / 100;
		
		if (width < (lt_part_width + rb_part_width))
		{
			width = lt_part_width + rb_part_width;
		}

		DCrDraw_PartSpec(
			draw_context,
			partspec_ui->progressbar_fill,
			PScPart_All,
			&dest,
			width,
			window_height,
			M3cMaxAlpha);
	}
		
	DCrDraw_End(draw_context, inProgressBar);
}

// ----------------------------------------------------------------------
static UUtUns32
WMiProgressBar_HandleGetValue(
	WMtProgressBar			*inProgressBar)
{
	UUtUns32				progressbar_data;
	
	// get the progressbar data
	progressbar_data = WMrWindow_GetLong(inProgressBar, 0);
	
	return progressbar_data;
}

// ----------------------------------------------------------------------
static void
WMiProgressBar_HandleSetValue(
	WMtProgressBar			*inProgressBar,
	UUtUns32				inPercent)
{
	UUtUns32				progressbar_data;
	
	// get the progressbar data
	progressbar_data = WMrWindow_GetLong(inProgressBar, 0);
	
	if (inPercent < 100)
	{
		progressbar_data = inPercent;
	}
	else
	{
		progressbar_data = 100;
	}
	
	// save the progressbar data
	WMrWindow_SetLong(inProgressBar, 0, progressbar_data);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
WMiProgressBar_Callback(
	WMtProgressBar			*inProgressBar,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtUns32				result;
	
	switch(inMessage)
	{
		case WMcMessage_Create:
			result = WMiProgressBar_Create(inProgressBar);
		return result;
		
		case WMcMessage_Paint:
			WMiProgressBar_Paint(inProgressBar);
		return WMcResult_Handled;
		
		case WMcMessage_GetValue:
			result = WMiProgressBar_HandleGetValue(inProgressBar);
		return result;
		
		case WMcMessage_SetValue:
			WMiProgressBar_HandleSetValue(inProgressBar, inParam1);
		return WMcResult_Handled;
	}
	
	return WMrWindow_DefaultCallback(inProgressBar, inMessage, inParam1, inParam2);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
WMrProgressBar_Initialize(
	void)
{
	UUtError				error;
	WMtWindowClass			window_class;
	
	// register the window class
	UUrMemory_Clear(&window_class, sizeof(WMtWindowClass));
	window_class.type = WMcWindowType_ProgressBar;
	window_class.callback = WMiProgressBar_Callback;
	window_class.private_data_size = sizeof(UUtUns32);
	
	error = WMrWindowClass_Register(&window_class);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}