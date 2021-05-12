// ======================================================================
// WM_Scrollbar.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_WindowManager.h"
#include "BFW_TextSystem.h"
#include "BFW_LocalInput.h"

#include "WM_Scrollbar.h"

// ======================================================================
// defines
// ======================================================================
#define WMcScrollbar_TimerID			1
#define WMcScrollbar_TimerFrequency		10

// ======================================================================
// enums
// ======================================================================
enum
{
	WMcSBPart_None,
	WMcSBPart_Track,
	WMcSBPart_PageUp,
	WMcSBPart_PageDown,
	WMcSBPart_LineUp,
	WMcSBPart_LineDown,
	WMcSBPart_Thumb
};

// ======================================================================
// typedefs
// ======================================================================
typedef struct WMtScrollbar_PrivateData
{
	UUtUns16				mouse_state;
	UUtBool					is_vertical;
	UUtUns8					mouse_down_part;
	
	// scrollbar values
	UUtInt32				min;
	UUtInt32				max;
	UUtInt32				current_value;
	float					range_visible;
			
	// thumb data
	IMtPoint2D				thumb_location;
	UUtInt16				thumb_width;
	UUtInt16				thumb_height;
	
	// mouse offsets
	UUtInt16				mouse_x_offset;
	UUtInt16				mouse_y_offset;

} WMtScrollbar_PrivateData;

// ======================================================================
// globals
// ======================================================================
static UUtInt16				WMgScrollbar_MinThumbWidth;
static UUtInt16				WMgScrollbar_MinThumbHeight;

// ======================================================================
// prototypes
// ======================================================================
static void
WMiScrollbar_GetBounds(
	WMtScrollbar				*inScrollbar,
	WMtScrollbar_PrivateData	*inPrivateData,
	UUtUns16					inScrollbarPart,
	UUtRect						*outBounds);

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
WMiScrollbar_CalculateThumbSize(
	WMtScrollbar					*inScrollbar,
	WMtScrollbar_PrivateData		*inPrivateData)
{
	UUtRect							track_bounds;
	UUtInt16						int_track_bounds_width;
	UUtInt16						int_track_bounds_height;

	float							range;

	// get the thumb move bounds
	WMiScrollbar_GetBounds(inScrollbar, inPrivateData, WMcSBPart_Track, &track_bounds);
	int_track_bounds_width = (track_bounds.right - track_bounds.left);
	int_track_bounds_height = (track_bounds.bottom - track_bounds.top);
	
	// calculate the range
	range = (float)(inPrivateData->max - inPrivateData->min);
		
	if (UUmFloat_CompareEqu(range, 0.0f))
	{
		// set the thumb height and width
		inPrivateData->thumb_height = int_track_bounds_height;
		inPrivateData->thumb_width = int_track_bounds_width;
	}
	else
	{
		float						float_track_bounds_width;
		float						float_track_bounds_height;

		// convert width and height to float
		float_track_bounds_width = (float)int_track_bounds_width;
		float_track_bounds_height = (float)int_track_bounds_height;
		
		if (inPrivateData->is_vertical)
		{
			// calculate the thumb height
			inPrivateData->thumb_height =
				(UUtInt16)(((float)inPrivateData->range_visible * float_track_bounds_height) / range);
			if (inPrivateData->thumb_height < WMgScrollbar_MinThumbHeight)
			{
				inPrivateData->thumb_height = WMgScrollbar_MinThumbHeight;
			}
			else if (inPrivateData->thumb_height > int_track_bounds_height)
			{
				inPrivateData->thumb_height = int_track_bounds_height;
			}
		}
		else
		{
			// calculate the thumb width
			inPrivateData->thumb_width =
				(UUtInt16)((inPrivateData->range_visible * float_track_bounds_width) / range);
			if (inPrivateData->thumb_width < WMgScrollbar_MinThumbWidth)
			{
				inPrivateData->thumb_width = WMgScrollbar_MinThumbWidth;
			}
			else if (inPrivateData->thumb_width > int_track_bounds_width)
			{
				inPrivateData->thumb_width = int_track_bounds_width;
			}
		}
	}
}

// ----------------------------------------------------------------------
static void
WMiScrollbar_GetBounds(
	WMtScrollbar				*inScrollbar,
	WMtScrollbar_PrivateData	*inPrivateData,
	UUtUns16					inScrollbarPart,
	UUtRect						*outBounds)
{
	PStPartSpecUI				*partspec_ui;
	UUtInt16					window_width;
	UUtInt16					window_height;
	UUtInt16					part_width;
	UUtInt16					part_height;
	UUtInt16					track_height;
	UUtInt16					track_width;
	
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
	WMrWindow_GetSize(inScrollbar, &window_width, &window_height);
	track_height = window_height;
	track_width = window_width;
	
	if (inPrivateData->is_vertical)
	{
		switch (inScrollbarPart)
		{
			case WMcSBPart_Track:
				PSrPartSpec_GetSize(partspec_ui->up_arrow, PScPart_LeftTop, &part_width, &part_height);
				track_height		-= part_height;
				outBounds->right	= part_width;
				outBounds->top		= part_height - 1;
				
				PSrPartSpec_GetSize(partspec_ui->dn_arrow, PScPart_LeftTop, NULL, &part_height);
				track_height		-= part_height;
				
				outBounds->bottom	= outBounds->top + track_height + 2;
			break;
			
			case WMcSBPart_PageUp:
				WMiScrollbar_GetBounds(inScrollbar, inPrivateData, WMcSBPart_Track, &track_bounds);
				WMiScrollbar_GetBounds(inScrollbar, inPrivateData, WMcSBPart_Thumb, &thumb_bounds);
				
				outBounds->left		= track_bounds.left;
				outBounds->top		= track_bounds.top;
				outBounds->right	= track_bounds.right;
				outBounds->bottom	= thumb_bounds.top + 1;
			break;
			
			case WMcSBPart_PageDown:
				WMiScrollbar_GetBounds(inScrollbar, inPrivateData, WMcSBPart_Track, &track_bounds);
				WMiScrollbar_GetBounds(inScrollbar, inPrivateData, WMcSBPart_Thumb, &thumb_bounds);
				
				outBounds->left		= track_bounds.left;
				outBounds->top		= thumb_bounds.bottom - 1;
				outBounds->right	= track_bounds.right;
				outBounds->bottom	= track_bounds.bottom;
			break;
			
			case WMcSBPart_LineUp:
				PSrPartSpec_GetSize(partspec_ui->up_arrow, PScPart_LeftTop, NULL, &part_height);
				
				outBounds->right	= window_width;
				outBounds->bottom	= part_height;
			break;

			case WMcSBPart_LineDown:
				PSrPartSpec_GetSize(partspec_ui->dn_arrow, PScPart_LeftTop, NULL, &part_height);
				
				outBounds->top		= window_height - part_height;
				outBounds->right	= window_width;
				outBounds->bottom	= window_height;
			break;

			case WMcSBPart_Thumb:
				PSrPartSpec_GetSize(partspec_ui->up_arrow, PScPart_LeftTop, &part_width, NULL);
				outBounds->left		= inPrivateData->thumb_location.x;
				outBounds->top		= inPrivateData->thumb_location.y;
				outBounds->right	= outBounds->left + part_width;
				outBounds->bottom	= outBounds->top + inPrivateData->thumb_height;
			break;
		}
	}
	else
	{
		switch (inScrollbarPart)
		{
			case WMcSBPart_Track:
				PSrPartSpec_GetSize(partspec_ui->lt_arrow, PScPart_LeftTop, &part_width, &part_height);
				track_width			-= part_width;
				outBounds->left		= part_width - 1;
				outBounds->bottom	= part_height;
				
				PSrPartSpec_GetSize(partspec_ui->rt_arrow, PScPart_LeftTop, &part_width, NULL);
				track_width			-= part_width;
				
				outBounds->right	= outBounds->left + track_width + 2;
			break;
		
			case WMcSBPart_PageUp:
				WMiScrollbar_GetBounds(inScrollbar, inPrivateData, WMcSBPart_Track, &track_bounds);
				WMiScrollbar_GetBounds(inScrollbar, inPrivateData, WMcSBPart_Thumb, &thumb_bounds);
				
				outBounds->left		= track_bounds.left;
				outBounds->top		= track_bounds.top;
				outBounds->right	= thumb_bounds.left + 1;
				outBounds->bottom	= track_bounds.bottom;
			break;
			
			case WMcSBPart_PageDown:
				WMiScrollbar_GetBounds(inScrollbar, inPrivateData, WMcSBPart_Track, &track_bounds);
				WMiScrollbar_GetBounds(inScrollbar, inPrivateData, WMcSBPart_Thumb, &thumb_bounds);
				
				outBounds->left		= thumb_bounds.right - 1;
				outBounds->top		= track_bounds.top;
				outBounds->right	= track_bounds.right;
				outBounds->bottom	= track_bounds.bottom;
			break;
			
			case WMcSBPart_LineUp:
				PSrPartSpec_GetSize(partspec_ui->lt_arrow, PScPart_LeftTop, &part_width, NULL);
				
				outBounds->right	= part_width;
				outBounds->bottom	= window_height;
			break;
			
			case WMcSBPart_LineDown:
				PSrPartSpec_GetSize(partspec_ui->rt_arrow, PScPart_LeftTop, &part_width, NULL);
				
				outBounds->left		= window_width - part_width;
				outBounds->right	= window_width;
				outBounds->bottom	= window_height;
			break;

			case WMcSBPart_Thumb:
				PSrPartSpec_GetSize(partspec_ui->lt_arrow, PScPart_LeftTop, NULL, &part_height);
				outBounds->left		= inPrivateData->thumb_location.x;
				outBounds->top		= inPrivateData->thumb_location.y;
				outBounds->right	= outBounds->left + inPrivateData->thumb_width;
				outBounds->bottom	= outBounds->top + part_height;
			break;
		}
	}
}

// ----------------------------------------------------------------------
static UUtUns8
WMiScrollBar_GetMouseOverPart(
	WMtScrollbar				*inScrollbar,
	WMtScrollbar_PrivateData	*inPrivateData,
	IMtPoint2D					*inLocalMouse)
{
	UUtBool						in_lineup;
	UUtBool						in_linedn;
	UUtBool						in_pageup;
	UUtBool						in_pagedn;
	UUtBool						in_thumb;
	
	UUtRect						bounds;
	
	// find out what part the mouse is over
	WMiScrollbar_GetBounds(inScrollbar, inPrivateData, WMcSBPart_Thumb, &bounds);
	in_thumb = IMrRect_PointIn(&bounds, inLocalMouse);
	if (in_thumb) { return WMcSBPart_Thumb; }
	
	WMiScrollbar_GetBounds(inScrollbar, inPrivateData, WMcSBPart_PageUp, &bounds);
	in_pageup = IMrRect_PointIn(&bounds, inLocalMouse);
	if (in_pageup) { return WMcSBPart_PageUp; }
		
	WMiScrollbar_GetBounds(inScrollbar, inPrivateData, WMcSBPart_PageDown, &bounds);
	in_pagedn = IMrRect_PointIn(&bounds, inLocalMouse);
	if (in_pagedn) { return WMcSBPart_PageDown; }

	WMiScrollbar_GetBounds(inScrollbar, inPrivateData, WMcSBPart_LineUp, &bounds);
	in_lineup = IMrRect_PointIn(&bounds, inLocalMouse);
	if (in_lineup) { return WMcSBPart_LineUp; }
	
	WMiScrollbar_GetBounds(inScrollbar, inPrivateData, WMcSBPart_LineDown, &bounds);
	in_linedn = IMrRect_PointIn(&bounds, inLocalMouse);
	if (in_linedn) { return WMcSBPart_LineDown; }
	
	return WMcSBPart_None;			
}

// ----------------------------------------------------------------------
static UUtInt32
WMiScrollbar_GetNewValue(
	WMtScrollbar					*inScrollbar,
	WMtScrollbar_PrivateData		*inPrivateData,
	IMtPoint2D						*inMouseLocation)
{
	float							scale;
	float							range;
	UUtInt32						new_value;
	UUtRect							track_bounds;
	
	// calculate the range
	range = (float)(inPrivateData->max - inPrivateData->min);
	
	// get the track bounds
	WMiScrollbar_GetBounds(inScrollbar, inPrivateData, WMcSBPart_Track, &track_bounds);
	
	if (inPrivateData->is_vertical)
	{
		float						track_height;
		float						y_value;
		
		// calculate the track height minus the thumb height
		track_height = (float)(track_bounds.bottom - track_bounds.top - inPrivateData->thumb_height);
		
		// calculate the value of y
		y_value = (float)((inMouseLocation->y - track_bounds.top) + inPrivateData->mouse_y_offset);
		
		// calculate the scale
		scale =	range / track_height;
		
		// calculate the new value
		new_value = inPrivateData->min + (UUtInt32)(y_value * scale);
	}
	else
	{
		float						track_width;
		float						x_value;
		
		// calculate the track width minus the thumb width
		track_width = (float)(track_bounds.right - track_bounds.left - inPrivateData->thumb_width);
		
		// calculate the new value of x
		x_value = (float)((inMouseLocation->x - track_bounds.left) + inPrivateData->mouse_x_offset);
		
		// calculate the scale
		scale =	range / track_width;
		
		// calculate the new value
		new_value = inPrivateData->min + (UUtInt32)(x_value * scale);
	}
	
	// keep new_value within range
	if (new_value < inPrivateData->min)
		new_value = inPrivateData->min;
	if (new_value > inPrivateData->max)
		new_value = inPrivateData->max;

	return new_value;
}

// ----------------------------------------------------------------------
static void
WMiScrollbar_SetThumbLocation(
	WMtScrollbar					*inScrollbar,
	WMtScrollbar_PrivateData		*inPrivateData)
{
	float							scale;
	float							range;
	float							current_val;
	
	UUtRect							track_bounds;
	float							track_width;
	float							track_height;
	
	// get the track bounds
	WMiScrollbar_GetBounds(inScrollbar, inPrivateData, WMcSBPart_Track, &track_bounds);
	
	// calculate the width minuse the thumb width and height minuse the thumb height
	track_width = (float)(track_bounds.right - track_bounds.left - inPrivateData->thumb_width);
	track_height = (float)(track_bounds.bottom - track_bounds.top - inPrivateData->thumb_height);
	
	// calculate the range
	range = (float)(inPrivateData->max - inPrivateData->min);
	current_val = (float)(inPrivateData->current_value - inPrivateData->min);
	
	if (UUmFloat_CompareEqu(range, 0.0f))
	{
		if (inPrivateData->is_vertical)
		{
			inPrivateData->thumb_location.x = track_bounds.left;
			inPrivateData->thumb_location.y = track_bounds.top;
		}
		else
		{
			inPrivateData->thumb_location.x = track_bounds.left;
			inPrivateData->thumb_location.y = track_bounds.top;
		}
	}
	else if (inPrivateData->is_vertical)
	{
		// calculate the scale
		scale =	track_height / range;
		
		// set the thumb location
		inPrivateData->thumb_location.x = track_bounds.left;
		inPrivateData->thumb_location.y = 
			track_bounds.top +
			(UUtUns16)(current_val * scale);
	}
	else
	{
		// calculate the scale
		scale = track_width / range;
		
		// set the thumb location
		inPrivateData->thumb_location.x = 
			track_bounds.left +
			(UUtUns16)(current_val * scale);
		inPrivateData->thumb_location.y = track_bounds.top;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
WMiScrollbar_Create(
	WMtScrollbar				*inScrollbar)
{
	PStPartSpecUI				*partspec_ui;
	UUtInt16					window_width;
	UUtInt16					window_height;
	UUtInt16					part_width;
	UUtInt16					part_height;
	WMtScrollbar_PrivateData	*private_data;
	UUtUns32					style;
	
	// create the private data
	private_data = (WMtScrollbar_PrivateData*)UUrMemory_Block_NewClear(sizeof(WMtScrollbar_PrivateData));
	if (private_data == NULL) { goto cleanup; }
	WMrWindow_SetLong(inScrollbar, 0, (UUtUns32)private_data);
	
	// get the active UI
	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) { goto cleanup; }
	
	style = WMrWindow_GetStyle(inScrollbar);
		
	// get the size of the up and left arrows
	PSrPartSpec_GetSize(partspec_ui->up_arrow, PScPart_LeftTop, &part_width, NULL);
	PSrPartSpec_GetSize(partspec_ui->lt_arrow, PScPart_LeftTop, NULL, &part_height);
	
	// the minumum thumb height is the same as the thumb width
	WMgScrollbar_MinThumbHeight = part_width;
	WMgScrollbar_MinThumbWidth = part_height;
	
	// Set the size of the scrollbar
	WMrWindow_GetSize(inScrollbar, &window_width, &window_height);
	if ((window_width == -1) && (window_height == -1))
	{
		UUmAssert(!"Width and height can't both be -1");
	}
	else if (window_width == -1)
	{
		// set the window width
		WMrWindow_SetSize(inScrollbar, part_width, window_height);
	}
	else if (window_height == -1)
	{
		// set the window height
		WMrWindow_SetSize(inScrollbar, window_width, part_height);
	}
	
	// initialize
	private_data->mouse_state		= LIcMouseState_None;
	private_data->is_vertical 		= window_height > window_width;
	private_data->min				= 0;
	private_data->max				= 0;
	private_data->current_value		= 0;
	private_data->range_visible		= 0;
	
	WMiScrollbar_CalculateThumbSize(inScrollbar, private_data);
	WMiScrollbar_SetThumbLocation(inScrollbar, private_data);
	
	// start the timer
	WMrTimer_Start(WMcScrollbar_TimerID, WMcScrollbar_TimerFrequency, inScrollbar);
	
	return WMcResult_Handled;

cleanup:
	UUmAssert(0);
	
	if (private_data)
	{
		UUrMemory_Block_Delete(private_data);
		WMrWindow_SetLong(inScrollbar, 0, 0);	
	}
	return WMcResult_Error;
}

// ----------------------------------------------------------------------
static void
WMiScrollbar_Destroy(
	WMtScrollbar				*inScrollbar)
{
	WMtScrollbar_PrivateData	*private_data;
	
	// get the private data
	private_data = (WMtScrollbar_PrivateData*)WMrWindow_GetLong(inScrollbar, 0);
	if (private_data == NULL) { return; }
	
	UUrMemory_Block_Delete(private_data);
	WMrWindow_SetLong(inScrollbar, 0, 0);
	
	// stop the timer
	WMrTimer_Stop(WMcScrollbar_TimerID, inScrollbar);
}

// ----------------------------------------------------------------------
static void
WMiScrollbar_HandleMouseEvent(
	WMtScrollbar				*inScrollbar,
	WMtMessage					inMessage,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	IMtPoint2D					global_mouse;
	IMtPoint2D					local_mouse;
	UUtInt32					new_value;

	WMtMessage					message;
	UUtUns16					scrollbar_id;
	UUtUns32					param1;
	UUtUns32					param2;
	
	WMtScrollbar_PrivateData	*private_data;
	
	UUtUns8						mouse_over_part;
	
	// get the private data
	private_data = (WMtScrollbar_PrivateData*)WMrWindow_GetLong(inScrollbar, 0);
	if (private_data == NULL) { return; }
	
	// get the local mouse position
	global_mouse.x = (UUtInt16)UUmHighWord(inParam1);
	global_mouse.y = (UUtInt16)UUmLowWord(inParam1);
	
	WMrWindow_GlobalToLocal(inScrollbar, &global_mouse, &local_mouse);
	
	// set the message pieces
	message = private_data->is_vertical ? SBcMessage_VerticalScroll : SBcMessage_HorizontalScroll;
	scrollbar_id = WMrWindow_GetID(inScrollbar);
			
	// get the part the mouse is over
	mouse_over_part = WMiScrollBar_GetMouseOverPart(inScrollbar, private_data, &local_mouse);
			
	switch(inMessage)
	{
		case WMcMessage_MouseMove:
			if ((inParam2 & LIcMouseState_LButtonDown) &&
				(private_data->mouse_down_part == WMcSBPart_Thumb))
			{
				new_value = WMiScrollbar_GetNewValue(inScrollbar, private_data, &local_mouse);
				
				// tell the parent of the thumb move
				WMrMessage_Send(
					WMrWindow_GetParent(inScrollbar),
					message,
					(UUtUns32)UUmMakeLong(SBcNotify_ThumbPosition, scrollbar_id),
					(UUtUns32)new_value);
			}
		break;
		
		case WMcMessage_LMouseDown:
			private_data->mouse_state |= LIcMouseState_LButtonDown;
			WMrWindow_SetFocus(inScrollbar);
			WMrWindow_CaptureMouse(inScrollbar);
		
			if ((private_data->mouse_down_part == WMcSBPart_None) ||
				(private_data->mouse_down_part == mouse_over_part))
			{
				// if the mouse over part is WMcSBPart_None then the user clicked a part and has
				// since moved the mouse off the part, only the thumb continues to function
				// in this case, for all others, break
				if (mouse_over_part == WMcSBPart_None)
				{
					if (private_data->mouse_down_part != WMcSBPart_Thumb)
					{
						break;
					}
					
					mouse_over_part = private_data->mouse_down_part;
				}
				
				// set the message for the parts the mouse was in
				switch (mouse_over_part)
				{
					case WMcSBPart_Thumb:
						if (private_data->mouse_down_part != WMcSBPart_Thumb)
						{
							private_data->mouse_down_part = WMcSBPart_Thumb;
							private_data->mouse_x_offset = private_data->thumb_location.x - local_mouse.x;
							private_data->mouse_y_offset = private_data->thumb_location.y - local_mouse.y;
						}
					break;
					
					case WMcSBPart_PageUp:
						param1 = (UUtUns32)UUmMakeLong(SBcNotify_PageUp, scrollbar_id);
						param2 = (UUtUns32)private_data->current_value;
						private_data->mouse_down_part = WMcSBPart_PageUp;
					break;
					
					case WMcSBPart_PageDown:
						param1 = (UUtUns32)UUmMakeLong(SBcNotify_PageDown, scrollbar_id);
						param2 = (UUtUns32)private_data->current_value;
						private_data->mouse_down_part = WMcSBPart_PageDown;
					break;
					
					case WMcSBPart_LineUp:
						param1 = (UUtUns32)UUmMakeLong(SBcNotify_LineUp, scrollbar_id);
						param2 = (UUtUns32)private_data->current_value;
						private_data->mouse_down_part = WMcSBPart_LineUp;
					break;
					
					case WMcSBPart_LineDown:
						param1 = (UUtUns32)UUmMakeLong(SBcNotify_LineDown, scrollbar_id);
						param2 = (UUtUns32)private_data->current_value;
						private_data->mouse_down_part = WMcSBPart_LineDown;
					break;
				}

				// tell the parent of the thumb move
				WMrMessage_Send(
					WMrWindow_GetParent(inScrollbar),
					message,
					param1,
					param2);
			}
		break;
		
		case WMcMessage_LMouseUp:
			private_data->mouse_state &= ~LIcMouseState_LButtonDown;
			private_data->mouse_down_part = WMcSBPart_None;
			WMrWindow_CaptureMouse(NULL);
		break;
	}
}

// ----------------------------------------------------------------------
static void
WMiScrollbar_HandlePositionChanged(
	WMtScrollbar				*inScrollbar)
{
	WMtScrollbar_PrivateData	*private_data;
	UUtInt16					width;
	UUtInt16					height;
	
	// get the private data
	private_data = (WMtScrollbar_PrivateData*)WMrWindow_GetLong(inScrollbar, 0);
	if (private_data == NULL) { return; }
	
	WMrWindow_GetSize(inScrollbar, &width, &height);
	private_data->is_vertical = height > width;
}

// ----------------------------------------------------------------------
static void
WMiScrollbar_HandleTimer(
	WMtScrollbar				*inScrollbar)
{			
	WMtScrollbar_PrivateData	*private_data;
	
	// get the private data
	private_data = (WMtScrollbar_PrivateData*)WMrWindow_GetLong(inScrollbar, 0);
	if (private_data == NULL) { return; }

	if (private_data->mouse_state & LIcMouseState_LButtonDown)
	{
		LItInputEvent	event;
		
		LIrInputEvent_GetMouse(&event);
		
		WMiScrollbar_HandleMouseEvent(
			inScrollbar,
			WMcMessage_LMouseDown,
			(UUtUns32)UUmMakeLong(event.where.x, event.where.y),
			(UUtUns32)event.modifiers);
	}
}

// ----------------------------------------------------------------------
static void
WMiScrollbar_Paint(
	WMtScrollbar				*inScrollbar)
{
	DCtDrawContext				*draw_context;
	PStPartSpecUI				*partspec_ui;
	PStPartSpec					*page_up;
	PStPartSpec					*page_dn;
	PStPartSpec					*line_up;
	PStPartSpec					*line_dn;
	PStPartSpec					*thumb;
	UUtRect						bounds;
	IMtPoint2D					dest;
	UUtInt16					width;
	UUtInt16					height;
	WMtScrollbar_PrivateData	*private_data;
	UUtBool						enabled;
	
	// get the private data
	private_data = (WMtScrollbar_PrivateData*)WMrWindow_GetLong(inScrollbar, 0);
	if (private_data == NULL) { return; }
	
	// get the active ui
	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) { return; }
	
	enabled = WMrWindow_GetEnabled(inScrollbar);
	
	// get the appropriate parts for the orientation
	if (private_data->is_vertical)
	{
		page_up = partspec_ui->sb_v_track;
		page_dn = partspec_ui->sb_v_track;
		line_up = (private_data->mouse_down_part == WMcSBPart_LineUp) ? partspec_ui->up_arrow_pressed : partspec_ui->up_arrow;
		line_dn = (private_data->mouse_down_part == WMcSBPart_LineDown) ? partspec_ui->dn_arrow_pressed : partspec_ui->dn_arrow;
		thumb = partspec_ui->sb_thumb;
	}
	else
	{
		page_up = partspec_ui->sb_h_track;
		page_dn = partspec_ui->sb_h_track;
		line_up = (private_data->mouse_down_part == WMcSBPart_LineUp) ? partspec_ui->lt_arrow_pressed : partspec_ui->lt_arrow;
		line_dn = (private_data->mouse_down_part == WMcSBPart_LineDown) ? partspec_ui->rt_arrow_pressed : partspec_ui->rt_arrow;
		thumb = partspec_ui->sb_thumb;
	}
	
	draw_context = DCrDraw_Begin(inScrollbar);
	
	// draw the page up part of the track
	WMiScrollbar_GetBounds(inScrollbar, private_data, WMcSBPart_PageUp, &bounds);
	if ((bounds.top != bounds.bottom) && (bounds.left != bounds.right))
	{
		dest.x = bounds.left;
		dest.y = bounds.top;
		
		width = bounds.right - bounds.left;
		height = bounds.bottom - bounds.top;
		
		DCrDraw_PartSpec(
			draw_context,
			page_up,
			PScPart_All,
			&dest,
			width,
			height,
			enabled ? (UUtUns16)WMcAlpha_Enabled : (UUtUns16)WMcAlpha_Disabled);
	}
	
	// draw the page dn part of the track
	WMiScrollbar_GetBounds(inScrollbar, private_data, WMcSBPart_PageDown, &bounds);
	if ((bounds.top != bounds.bottom) && (bounds.left != bounds.right))
	{
		dest.x = bounds.left;
		dest.y = bounds.top;
		
		width = bounds.right - bounds.left;
		height = bounds.bottom - bounds.top;
		
		DCrDraw_PartSpec(
			draw_context,
			page_dn,
			PScPart_All,
			&dest,
			width,
			height,
			enabled ? (UUtUns16)WMcAlpha_Enabled : (UUtUns16)WMcAlpha_Disabled);
	}
	
	// draw the line up
	WMiScrollbar_GetBounds(inScrollbar, private_data, WMcSBPart_LineUp, &bounds);
	
	dest.x = bounds.left;
	dest.y = bounds.top;
	
	width = bounds.right - bounds.left;
	height = bounds.bottom - bounds.top;
	
	DCrDraw_PartSpec(
		draw_context,
		line_up,
		PScPart_LeftTop,
		&dest,
		width,
		height,
		enabled ? (UUtUns16)WMcAlpha_Enabled : (UUtUns16)WMcAlpha_Disabled);
	
	// draw the line down
	WMiScrollbar_GetBounds(inScrollbar, private_data, WMcSBPart_LineDown, &bounds);
	
	dest.x = bounds.left;
	dest.y = bounds.top;
	
	width = bounds.right - bounds.left;
	height = bounds.bottom - bounds.top;
	
	DCrDraw_PartSpec(
		draw_context,
		line_dn,
		PScPart_LeftTop,
		&dest,
		width,
		height,
		enabled ? (UUtUns16)WMcAlpha_Enabled : (UUtUns16)WMcAlpha_Disabled);
	
	// draw the thumb
	WMiScrollbar_GetBounds(inScrollbar, private_data, WMcSBPart_Thumb, &bounds);
	
	dest.x = bounds.left;
	dest.y = bounds.top;
	
	DCrDraw_PartSpec(
		draw_context,
		thumb,
		PScPart_All,
		&dest,
		private_data->thumb_width,
		private_data->thumb_height,
		enabled ? (UUtUns16)WMcAlpha_Enabled : (UUtUns16)WMcAlpha_Disabled);
	
	DCrDraw_End(draw_context, inScrollbar);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
WMiScrollbar_Callback(
	WMtScrollbar			*inScrollbar,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtUns32					result;
	
	switch(inMessage)
	{
		case WMcMessage_NC_HitTest:
		return WMcWindowArea_Client;
		
		case WMcMessage_Create:
			result = WMiScrollbar_Create(inScrollbar);
		return result;
		
		case WMcMessage_Destroy:
			WMiScrollbar_Destroy(inScrollbar);
		return WMcResult_Handled;
		
		case WMcMessage_MouseMove:
		case WMcMessage_LMouseDown:
		case WMcMessage_LMouseUp:
			WMiScrollbar_HandleMouseEvent(
				inScrollbar,
				inMessage,
				inParam1,
				inParam2);
		return WMcResult_Handled;
		
		case WMcMessage_Paint:
			WMiScrollbar_Paint(inScrollbar);
		return WMcResult_Handled;
		
		case WMcMessage_PositionChanged:
			WMiScrollbar_HandlePositionChanged(inScrollbar);
		return WMcResult_Handled;
		
		case WMcMessage_Timer:
			if (inParam1 == WMcScrollbar_TimerID)
			{
				WMiScrollbar_HandleTimer(inScrollbar);
			}
		return WMcResult_Handled;
	}
	
	return WMrWindow_DefaultCallback(inScrollbar, inMessage, inParam1, inParam2);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtInt32
WMrScrollbar_GetPosition(
	WMtScrollbar			*inScrollbar)
{
	WMtScrollbar_PrivateData	*private_data;
	
	UUmAssert(inScrollbar);
	
	// get the private data
	private_data = (WMtScrollbar_PrivateData*)WMrWindow_GetLong(inScrollbar, 0);
	if (private_data == NULL) { return 0; }
	
	// return the current position
	return private_data->current_value;
}

// ----------------------------------------------------------------------
void
WMrScrollbar_GetRange(
	WMtScrollbar			*inScrollbar,
	UUtInt32				*outMin,
	UUtInt32				*outMax,
	UUtInt32				*outRangeVisible)
{
	WMtScrollbar_PrivateData	*private_data;
	
	UUmAssert(inScrollbar);
	UUmAssert(outMin);
	UUmAssert(outMax);
	
	// get the private data
	private_data = (WMtScrollbar_PrivateData*)WMrWindow_GetLong(inScrollbar, 0);
	if (private_data == NULL) { return; }
	
	*outMin = private_data->min;
	*outMax = private_data->max;
	*outRangeVisible = (UUtInt32)private_data->range_visible;
}

// ----------------------------------------------------------------------
void
WMrScrollbar_SetPosition(
	WMtScrollbar			*inScrollbar,
	UUtInt32				inPosition)
{
	WMtScrollbar_PrivateData	*private_data;
	
	UUmAssert(inScrollbar);
	
	// get the private data
	private_data = (WMtScrollbar_PrivateData*)WMrWindow_GetLong(inScrollbar, 0);
	if (private_data == NULL) { return; }
	
	// set the new position
	private_data->current_value = inPosition;
	
	// make sure the new value is within the range
	if (private_data->current_value < private_data->min)
		private_data->current_value = private_data->min;
	else if (private_data->current_value > private_data->max)
		private_data->current_value = private_data->max;

	WMiScrollbar_SetThumbLocation(inScrollbar, private_data);
}

// ----------------------------------------------------------------------
void
WMrScrollbar_SetRange(
	WMtScrollbar			*inScrollbar,
	UUtInt32				inMin,
	UUtInt32				inMax,
	UUtInt32				inRangeVisible)
{
	WMtScrollbar_PrivateData	*private_data;
	
	UUmAssert(inScrollbar);
	
	// get the private data
	private_data = (WMtScrollbar_PrivateData*)WMrWindow_GetLong(inScrollbar, 0);
	if (private_data == NULL) { return; }
	
	// set the new range values
	private_data->min = inMin;
	private_data->max = inMax;
	private_data->current_value = inMin;
	private_data->range_visible = (float)inRangeVisible;
	
	WMiScrollbar_CalculateThumbSize(inScrollbar, private_data);
	WMiScrollbar_SetThumbLocation(inScrollbar, private_data);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
WMrScrollbar_Initialize(
	void)
{
	UUtError				error;
	WMtWindowClass			window_class;
	
	// register the window class
	UUrMemory_Clear(&window_class, sizeof(WMtWindowClass));
	window_class.type = WMcWindowType_Scrollbar;
	window_class.callback = WMiScrollbar_Callback;
	window_class.private_data_size = sizeof(WMtScrollbar_PrivateData*);
	
	error = WMrWindowClass_Register(&window_class);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}