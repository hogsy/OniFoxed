// ======================================================================
// WM_Slider.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_WindowManager.h"
#include "BFW_LocalInput.h"

#include "WM_Slider.h"

// ======================================================================
// defines
// ======================================================================
#define WMcSlider_TimerID				1
#define WMcSlider_TimerFrequency		10

// ======================================================================
// enums
// ======================================================================
enum
{
	WMcSLPart_None,
	WMcSLPart_Track,
	WMcSLPart_PageUp,
	WMcSLPart_PageDown,
	WMcSLPart_Thumb
};

// ======================================================================
// typedefs
// ======================================================================
typedef struct WMtSlider_PrivateData
{
	UUtInt32				min;
	UUtInt32				max;
	UUtInt32				current_value;

	UUtUns8					mouse_state;
	UUtBool					mouse_down_in_thumb;
	UUtInt16				mouse_x_offset;

	// thumb data
	IMtPoint2D				thumb_location;
	UUtInt16				thumb_width;
	UUtInt16				thumb_height;

} WMtSlider_PrivateData;


// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
WMiSlider_GetBounds(
	WMtSlider					*inSlider,
	WMtSlider_PrivateData		*inPrivateData,
	UUtUns16					inSliderPart,
	UUtRect						*outBounds)
{
	PStPartSpecUI				*partspec_ui;
	UUtInt16					window_width;
	UUtInt16					window_height;
	UUtInt16					part_width;
	UUtInt16					part_height;

	UUtRect						track_bounds;
	UUtRect						thumb_bounds;

	UUmAssert(outBounds);

	outBounds->left		= 0;
	outBounds->top		= 0;
	outBounds->right	= 0;
	outBounds->bottom	= 0;

	// get the active UI
	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) { return; }

	// get the window size
	WMrWindow_GetSize(inSlider, &window_width, &window_height);

	switch (inSliderPart)
	{
		case WMcSLPart_Track:
			PSrPartSpec_GetSize(partspec_ui->sl_track, PScPart_LeftTop, &part_width, &part_height);

			outBounds->top		= (window_height - part_height) >> 1;
			outBounds->right	= window_width;
			outBounds->bottom	= outBounds->top + part_height;
		break;

		case WMcSLPart_PageUp:
			WMiSlider_GetBounds(inSlider, inPrivateData, WMcSLPart_Track, &track_bounds);
			WMiSlider_GetBounds(inSlider, inPrivateData, WMcSLPart_Thumb, &thumb_bounds);

			outBounds->left		= track_bounds.left;
			outBounds->right	= thumb_bounds.left + 1;
			outBounds->bottom	= window_height;
		break;

		case WMcSLPart_PageDown:
			WMiSlider_GetBounds(inSlider, inPrivateData, WMcSLPart_Track, &track_bounds);
			WMiSlider_GetBounds(inSlider, inPrivateData, WMcSLPart_Thumb, &thumb_bounds);

			outBounds->left		= thumb_bounds.right - 1;
			outBounds->right	= track_bounds.right;
			outBounds->bottom	= window_height;
		break;

		case WMcSLPart_Thumb:
			PSrPartSpec_GetSize(partspec_ui->sl_thumb, PScPart_LeftTop, &part_width, &part_height);

			outBounds->left		= inPrivateData->thumb_location.x;
			outBounds->top		= inPrivateData->thumb_location.y - (part_height >> 1);
			outBounds->right	= outBounds->left + part_width;
			outBounds->bottom	= outBounds->top + part_height;
		break;
	}
}

// ----------------------------------------------------------------------
static void
WMiSlider_GetTrackBounds(
	WMtSlider					*inSlider,
	WMtSlider_PrivateData		*inPrivateData,
	UUtRect						*outMoveBounds)
{
	PStPartSpecUI				*partspec_ui;
	UUtInt16					left_width;
	UUtInt16					right_width;

	// initialize
	outMoveBounds->left = 0;
	outMoveBounds->top = 0;
	outMoveBounds->right = 0;
	outMoveBounds->bottom = 0;

	// get the partspec ui
	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) { return; }

	// get the size of the track left_width, and right_width of the track part
	PSrPartSpec_GetSize(partspec_ui->sl_track, PScPart_LeftTop, &left_width, NULL);
	PSrPartSpec_GetSize(partspec_ui->sl_track, PScPart_RightTop, &right_width, NULL);

	// get the track bounds
	WMiSlider_GetBounds(inSlider, inPrivateData, WMcSLPart_Track, outMoveBounds);

	// add in the left and right buffer areas of the move bounds
	outMoveBounds->left += left_width;// + (inPrivateData->thumb_width >> 1);
	outMoveBounds->right -= right_width;// + (inPrivateData->thumb_width >> 1);
}

// ----------------------------------------------------------------------
static UUtInt32
WMiSlider_GetNewValue(
	WMtSlider					*inSlider,
	WMtSlider_PrivateData		*inPrivateData,
	IMtPoint2D					*inMouseLocation)
{
	UUtRect						track_bounds;
	float						track_width;

	float						scale;
	float						range;

	float						x_value;
	UUtInt32					new_value;

	// calculate the range
	range = (float)(inPrivateData->max - inPrivateData->min);

	// get the track bounds
	WMiSlider_GetTrackBounds(inSlider, inPrivateData, &track_bounds);

	// calculate the track width minus the thumb width
	track_width = (float)(track_bounds.right - track_bounds.left - inPrivateData->thumb_width);

	// calculate the value of x
	x_value = (float)((inMouseLocation->x - track_bounds.left) + inPrivateData->mouse_x_offset);

	// calculate the scale
	scale =	range / track_width;

	// calculate the new value
	new_value = inPrivateData->min + (UUtInt32)(x_value * scale);

	// keep new_value within range
	if (new_value < inPrivateData->min)
		new_value = inPrivateData->min;
	if (new_value > inPrivateData->max)
		new_value = inPrivateData->max;

	return new_value;
}

// ----------------------------------------------------------------------
static void
WMiSlider_SetThumbLocation(
	WMtSlider					*inSlider,
	WMtSlider_PrivateData		*inPrivateData)
{
	float						scale;
	float						range;
	float						current_val;

	UUtRect						track_bounds;
	float						track_width;
	UUtInt16					track_height;

	// get the track bounds
	WMiSlider_GetTrackBounds(inSlider, inPrivateData, &track_bounds);

	// calculate the track width minus thumb width and track height
	track_width = (float)(track_bounds.right - track_bounds.left - inPrivateData->thumb_width);
	track_height = track_bounds.bottom - track_bounds.top;

	// calculate the range and current_val
	range = (float)(inPrivateData->max - inPrivateData->min);
	current_val = (float)(inPrivateData->current_value - inPrivateData->min);

	// calculate the scale
	scale =	track_width / range;

	// set the thumb location
	inPrivateData->thumb_location.x =
		track_bounds.left +
		(UUtUns16)(current_val * scale);
	inPrivateData->thumb_location.y = track_bounds.top + (track_height >> 1);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
WMiSlider_Create(
	WMtSlider				*inSlider)
{
	WMtSlider_PrivateData	*private_data;
	UUtRect					thumb_bounds;

	// create the private data
	private_data =
		(WMtSlider_PrivateData*)UUrMemory_Block_NewClear(sizeof(WMtSlider_PrivateData));
	if (private_data == NULL) { goto cleanup; }

	// save the private data
	WMrWindow_SetLong(inSlider, 0, (UUtUns32)private_data);

	// calculate the thumb width and height
	WMiSlider_GetBounds(inSlider, private_data, WMcSLPart_Thumb, &thumb_bounds);
	private_data->thumb_width = thumb_bounds.right - thumb_bounds.left;
	private_data->thumb_height = thumb_bounds.bottom - thumb_bounds.top;

	// initialize
	WMiSlider_SetThumbLocation(inSlider, private_data);

	// start the timer
	WMrTimer_Start(WMcSlider_TimerID, WMcSlider_TimerFrequency, inSlider);

	return WMcResult_Handled;

cleanup:
	if (private_data)
	{
		UUrMemory_Block_Delete(private_data);
		private_data = NULL;
	}
	WMrWindow_SetLong(inSlider, 0, 0);

	return WMcResult_Error;
}

// ----------------------------------------------------------------------
static void
WMiSlider_Destroy(
	WMtSlider				*inSlider)
{
	WMtSlider_PrivateData	*private_data;

	private_data = (WMtSlider_PrivateData*)WMrWindow_GetLong(inSlider, 0);
	if (private_data)
	{
		UUrMemory_Block_Delete(private_data);
		private_data = NULL;
	}

	WMrWindow_SetLong(inSlider, 0, 0);

	// stop the timer
	WMrTimer_Stop(WMcSlider_TimerID, inSlider);
}

// ----------------------------------------------------------------------
static void
WMiSlider_HandleKeyEvent(
	WMtSlider				*inSlider,
	UUtUns32				inParam1)
{
	WMtSlider_PrivateData	*private_data;

	UUtInt32				scroll_increment;
	UUtInt32				scroll_range;

	// disabled sliders don't move
	if (WMrWindow_GetEnabled(inSlider) == UUcFalse) { return; }

	// get the private data
	private_data = (WMtSlider_PrivateData*)WMrWindow_GetLong(inSlider, 0);
	if (private_data == NULL) { return; }

	// get the scroll range
	scroll_range = private_data->max - private_data->min;
	scroll_increment = 0;

	switch (inParam1)
	{
		case LIcKeyCode_LeftArrow:
			scroll_increment = -1;
		break;

		case LIcKeyCode_RightArrow:
			scroll_increment = 1;
		break;

		case LIcKeyCode_PageUp:
			scroll_increment = UUmMin(-1, -scroll_range / 10);
		break;

		case LIcKeyCode_PageDown:
			scroll_increment = UUmMax(1, scroll_range / 10);
		break;

		case LIcKeyCode_Home:
			scroll_increment = private_data->min - private_data->current_value;
		break;

		case LIcKeyCode_End:
			scroll_increment = private_data->max - private_data->current_value;
		break;
	}

	if (scroll_increment != 0)
	{
		WMrSlider_SetPosition(inSlider,	private_data->current_value + scroll_increment);

		// tell the parent
		WMrMessage_Send(
			WMrWindow_GetParent(inSlider),
			WMcMessage_Command,
			(UUtUns32)UUmMakeLong(SLcNotify_NewPosition, WMrWindow_GetID(inSlider)),
			(UUtUns32)inSlider);
	}
}

// ----------------------------------------------------------------------
static void
WMiSlider_HandleMouseEvent(
	WMtSlider				*inSlider,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	IMtPoint2D				global_mouse;
	IMtPoint2D				local_mouse;
	UUtRect					bounds;

	UUtBool					in_pageup;
	UUtBool					in_pagedn;
	UUtBool					in_thumb;

	WMtSlider_PrivateData	*private_data;

	UUtInt32				scroll_increment;
	UUtInt32				scroll_range;

	// disabled sliders don't move
	if (WMrWindow_GetEnabled(inSlider) == UUcFalse) { return; }

	// get the private data
	private_data = (WMtSlider_PrivateData*)WMrWindow_GetLong(inSlider, 0);
	if (private_data == NULL) { return; }

	// get the local mouse position
	global_mouse.x = (UUtInt16)UUmHighWord(inParam1);
	global_mouse.y = (UUtInt16)UUmLowWord(inParam1);

	WMrWindow_GlobalToLocal(inSlider, &global_mouse, &local_mouse);

	// get the scroll range
	scroll_range = private_data->max - private_data->min;
	scroll_increment = 0;

	switch(inMessage)
	{
		case WMcMessage_MouseMove:
			if ((inParam2 & LIcMouseState_LButtonDown) &&
				(private_data->mouse_down_in_thumb))
			{
				scroll_increment =
					WMiSlider_GetNewValue(inSlider, private_data, &local_mouse) -
					private_data->current_value;
			}
		break;

		case WMcMessage_LMouseDown:
			private_data->mouse_state |= LIcMouseState_LButtonDown;
			WMrWindow_SetFocus(inSlider);
			WMrWindow_CaptureMouse(inSlider);

			// find out what part the mouse is over
			WMiSlider_GetBounds(inSlider, private_data, WMcSLPart_Thumb, &bounds);
			in_thumb = IMrRect_PointIn(&bounds, &local_mouse);

			WMiSlider_GetBounds(inSlider, private_data, WMcSLPart_PageUp, &bounds);
			in_pageup = IMrRect_PointIn(&bounds, &local_mouse);

			WMiSlider_GetBounds(inSlider, private_data, WMcSLPart_PageDown, &bounds);
			in_pagedn = IMrRect_PointIn(&bounds, &local_mouse);

			// set the message for the parts the mouse was in
			if (in_thumb && (private_data->mouse_down_in_thumb == UUcFalse))
			{
				private_data->mouse_down_in_thumb = UUcTrue;
				private_data->mouse_x_offset = private_data->thumb_location.x - local_mouse.x;
			}
			else if (in_pageup)
			{
				scroll_increment = UUmMin(-1, -scroll_range / 10);
			}
			else if (in_pagedn)
			{
				scroll_increment = UUmMax(1, scroll_range / 10);
			}
		break;

		case WMcMessage_LMouseUp:
			private_data->mouse_state &= ~LIcMouseState_LButtonDown;
			private_data->mouse_down_in_thumb = UUcFalse;
			WMrWindow_CaptureMouse(NULL);
		break;
	}

	if (scroll_increment != 0)
	{
		WMrSlider_SetPosition(inSlider,	private_data->current_value + scroll_increment);

		// tell the parent
		WMrMessage_Send(
			WMrWindow_GetParent(inSlider),
			WMcMessage_Command,
			(UUtUns32)UUmMakeLong(SLcNotify_NewPosition, WMrWindow_GetID(inSlider)),
			(UUtUns32)inSlider);
	}
}

// ----------------------------------------------------------------------
static void
WMiSlider_HandleTimer(
	WMtSlider				*inSlider)
{
	WMtSlider_PrivateData	*private_data;

	// get the private data
	private_data = (WMtSlider_PrivateData*)WMrWindow_GetLong(inSlider, 0);
	if (private_data == NULL) { return; }

	if (private_data->mouse_state & LIcMouseState_LButtonDown)
	{
		LItInputEvent	event;

		LIrInputEvent_GetMouse(&event);

		WMiSlider_HandleMouseEvent(
			inSlider,
			WMcMessage_LMouseDown,
			(UUtUns32)UUmMakeLong(event.where.x, event.where.y),
			(UUtUns32)event.modifiers);
	}
}

// ----------------------------------------------------------------------
static void
WMiSlider_Paint(
	WMtSlider				*inSlider)
{
	WMtSlider_PrivateData	*private_data;
	DCtDrawContext			*draw_context;
	PStPartSpecUI			*partspec_ui;
	UUtBool					enabled;

	IMtPoint2D				dest;
	UUtRect					thumb_bounds;
	UUtRect					track_bounds;

	// get the private data
	private_data = (WMtSlider_PrivateData*)WMrWindow_GetLong(inSlider, 0);
	if (private_data == NULL) { return; }

	// get the partspec ui
	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) { return; }

	draw_context = DCrDraw_Begin(inSlider);

	enabled = WMrWindow_GetEnabled(inSlider);

	// get the bounds of the tracks
	WMiSlider_GetBounds(inSlider, private_data, WMcSLPart_Track, &track_bounds);

	// set the dest for the track
	dest.x = track_bounds.left;
	dest.y = track_bounds.top;

	// draw the track
	DCrDraw_PartSpec(
		draw_context,
		partspec_ui->sl_track,
		PScPart_TopRow,
		&dest,
		(UUtUns16)(track_bounds.right - track_bounds.left),
		(UUtUns16)(track_bounds.bottom - track_bounds.top),
		M3cMaxAlpha);

	// get the bounds of the thumb
	WMiSlider_GetBounds(inSlider, private_data, WMcSLPart_Thumb, &thumb_bounds);

	dest.x = thumb_bounds.left;
	dest.y = thumb_bounds.top;

	DCrDraw_PartSpec(
		draw_context,
		partspec_ui->sl_thumb,
		PScPart_LeftTop,
		&dest,
		private_data->thumb_width,
		private_data->thumb_height,
		enabled ? (UUtUns16)WMcAlpha_Enabled : (UUtUns16)WMcAlpha_Disabled);

	DCrDraw_End(draw_context, inSlider);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
WMiSlider_Callback(
	WMtSlider				*inSlider,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtUns32				result;

	switch(inMessage)
	{
		case WMcMessage_NC_HitTest:
		return WMcWindowArea_Client;

		case WMcMessage_Create:
			result = WMiSlider_Create(inSlider);
		return result;

		case WMcMessage_Destroy:
			WMiSlider_Destroy(inSlider);
		return WMcResult_Handled;

		case WMcMessage_Paint:
			WMiSlider_Paint(inSlider);
		return WMcResult_Handled;

		case WMcMessage_KeyDown:
			WMiSlider_HandleKeyEvent(inSlider, inParam1);
		break;

		case WMcMessage_MouseMove:
		case WMcMessage_LMouseDown:
		case WMcMessage_LMouseUp:
			WMiSlider_HandleMouseEvent(inSlider, inMessage, inParam1, inParam2);
		break;

		case WMcMessage_Timer:
			if (inParam1 == WMcSlider_TimerID)
			{
				WMiSlider_HandleTimer(inSlider);
			}
		return WMcResult_Handled;

		case WMcMessage_GetDialogCode:
			return (WMcDialogCode_WantNavigation | WMcDialogCode_WantArrows);
		break;
	}

	return WMrWindow_DefaultCallback(inSlider, inMessage, inParam1, inParam2);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtInt32
WMrSlider_GetPosition(
	WMtSlider				*inSlider)
{
	WMtSlider_PrivateData	*private_data;

	UUmAssert(inSlider);

	// get the private data
	private_data = (WMtSlider_PrivateData*)WMrWindow_GetLong(inSlider, 0);
	if (private_data == NULL) { return 0; }

	// return the current position
	return private_data->current_value;
}

// ----------------------------------------------------------------------
void
WMrSlider_GetRange(
	WMtSlider				*inSlider,
	UUtInt32				*outMin,
	UUtInt32				*outMax)
{
	WMtSlider_PrivateData	*private_data;

	UUmAssert(inSlider);
	UUmAssert(outMin);
	UUmAssert(outMax);

	// get the private data
	private_data = (WMtSlider_PrivateData*)WMrWindow_GetLong(inSlider, 0);
	if (private_data == NULL) { return; }

	*outMin = private_data->min;
	*outMax = private_data->max;
}

// ----------------------------------------------------------------------
void
WMrSlider_SetPosition(
	WMtSlider				*inSlider,
	UUtInt32				inPosition)
{
	WMtSlider_PrivateData	*private_data;

	UUmAssert(inSlider);

	// get the private data
	private_data = (WMtSlider_PrivateData*)WMrWindow_GetLong(inSlider, 0);
	if (private_data == NULL) { return; }

	// set the new position
	private_data->current_value = inPosition;

	// make sure the new value is within the range
	if (private_data->current_value < private_data->min)
		private_data->current_value = private_data->min;
	else if (private_data->current_value > private_data->max)
		private_data->current_value = private_data->max;

	WMiSlider_SetThumbLocation(inSlider, private_data);
}

// ----------------------------------------------------------------------
void
WMrSlider_SetRange(
	WMtSlider				*inSlider,
	UUtInt32				inMin,
	UUtInt32				inMax)
{
	WMtSlider_PrivateData	*private_data;

	UUmAssert(inSlider);

	// get the private data
	private_data = (WMtSlider_PrivateData*)WMrWindow_GetLong(inSlider, 0);
	if (private_data == NULL) { return; }

	// set the new range values
	private_data->min = inMin;
	private_data->max = inMax;
	private_data->current_value = inMin;

	WMiSlider_SetThumbLocation(inSlider, private_data);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
WMrSlider_Initialize(
	void)
{
	UUtError				error;
	WMtWindowClass			window_class;

	// register the window class
	UUrMemory_Clear(&window_class, sizeof(WMtWindowClass));
	window_class.type = WMcWindowType_Slider;
	window_class.callback = WMiSlider_Callback;
	window_class.private_data_size = sizeof(WMtSlider_PrivateData*);

	error = WMrWindowClass_Register(&window_class);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}
