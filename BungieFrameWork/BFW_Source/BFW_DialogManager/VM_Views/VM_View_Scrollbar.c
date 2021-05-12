// ======================================================================
// VM_View_Scrollbar.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ViewManager.h"
#include "BFW_ViewUtilities.h"
#include "BFW_DialogManager.h"
#include "BFW_LocalInput.h"

#include "VM_View_Scrollbar.h"

// ======================================================================
// defines
// ======================================================================
#define VMcView_Scrollbar_Timer					1
#define VMcView_Scrollbar_TimerFrequency		10

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
VMiView_Scrollbar_SetThumbLocation(
	VMtView_Scrollbar_PrivateData	*inPrivateData)
{
	float							scale;
	float							range;
	float							current_val;
	
	// calculate the range
	range = (float)(inPrivateData->max - inPrivateData->min);
	current_val = (float)(inPrivateData->current_value - inPrivateData->min);
	
	if (inPrivateData->is_vertical)
	{
		float						bounds_height;
		
		// calculate the bounds height
		bounds_height =
			(float)(inPrivateData->thumb_move_bounds.bottom -
			inPrivateData->thumb_move_bounds.top -
			inPrivateData->thumb_height);
		
		// calculate the scale
		scale =	bounds_height / range;
		
		// set the thumb location
		inPrivateData->thumb_location.x = inPrivateData->thumb_x_track;
		inPrivateData->thumb_location.y = 
			inPrivateData->thumb_move_bounds.top +
			(UUtUns16)(current_val  * scale);
		
		// set the page up and page down bounds
		inPrivateData->page_up_bounds = inPrivateData->thumb_move_bounds;
		inPrivateData->page_up_bounds.bottom = inPrivateData->thumb_location.y;
		
		inPrivateData->page_dn_bounds = inPrivateData->thumb_move_bounds;
		inPrivateData->page_dn_bounds.top =
			inPrivateData->thumb_location.y +
			inPrivateData->thumb_height;
	}
	else
	{
		float						bounds_width;
		
		// calculate the bounds width
		bounds_width =
			(float)(inPrivateData->thumb_move_bounds.right -
			inPrivateData->thumb_move_bounds.left -
			inPrivateData->thumb_width);
		
		// calculate the scale
		scale =	bounds_width / range;
		
		// set the thumb location
		inPrivateData->thumb_location.x = 
			inPrivateData->thumb_move_bounds.left +
			(UUtUns16)(current_val * scale);
		inPrivateData->thumb_location.y = inPrivateData->thumb_y_track;
		
		// set the page up and page down bounds
		inPrivateData->page_up_bounds = inPrivateData->thumb_move_bounds;
		inPrivateData->page_up_bounds.right = inPrivateData->thumb_location.x;
		
		inPrivateData->page_dn_bounds = inPrivateData->thumb_move_bounds;
		inPrivateData->page_dn_bounds.left =
			inPrivateData->thumb_location.x +
			inPrivateData->thumb_width;
	}
}

// ----------------------------------------------------------------------
static UUtInt32
VMiView_Scrollbar_GetNewValue(
	VMtView_Scrollbar_PrivateData	*inPrivateData,
	IMtPoint2D						*inMouseLocation)
{
	float							scale;
	float							range;
	UUtInt32						new_value;
	
	// calculate the range
	range = (float)(inPrivateData->max - inPrivateData->min);
	
	if (inPrivateData->is_vertical)
	{
		float						bounds_height;
		UUtInt32					y_value;
		
		// calculate the new value
		y_value = 
			(UUtInt32)(inMouseLocation->y -
			inPrivateData->thumb_move_bounds.top);
		
		// calculate the bounds height
		bounds_height =
			(float)(inPrivateData->thumb_move_bounds.bottom -
			inPrivateData->thumb_move_bounds.top -
			inPrivateData->thumb_height);
		
		// calculate the scale
		scale =	range / bounds_height;
		
		// calculate the new value
		new_value = inPrivateData->min + (UUtInt32)((float)y_value * scale);
	}
	else
	{
		float						bounds_width;
		UUtInt32					x_value;
		
		// calculate the new value
		x_value = 
			(UUtInt32)(inMouseLocation->x -
			inPrivateData->thumb_move_bounds.left);
		
		// calculate the bounds width
		bounds_width =
			(float)(inPrivateData->thumb_move_bounds.right -
			inPrivateData->thumb_move_bounds.left -
			inPrivateData->thumb_width);
		
		// calculate the scale
		scale =	range / bounds_width;
		
		// calculate the new value
		new_value = inPrivateData->min + (UUtInt32)((float)x_value * scale);
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
VMiView_Scrollbar_SetBounds(
	VMtView							*inView,
	VMtView_Scrollbar				*inScrollbar,
	VMtView_Scrollbar_PrivateData	*inPrivateData)
{
	if (inPrivateData->is_vertical)
	{
		// ------------------------------
		// set the up and down arrow bounds
		// ------------------------------
		if (inScrollbar->vertical_scrollbar)
		{
			UUtUns16					up_arrow_height;
			UUtUns16					up_arrow_width;
			UUtUns16					dn_arrow_height;
			UUtUns16					dn_arrow_width;
			
			up_arrow_width =
				(inScrollbar->vertical_scrollbar->part_matrix_br[VMcColumn_Left][VMcRow_Top].x -
				inScrollbar->vertical_scrollbar->part_matrix_tl[VMcColumn_Left][VMcRow_Top].x);
			up_arrow_height =
				(inScrollbar->vertical_scrollbar->part_matrix_br[VMcColumn_Left][VMcRow_Top].y -
				inScrollbar->vertical_scrollbar->part_matrix_tl[VMcColumn_Left][VMcRow_Top].y);
			
			dn_arrow_width =
				(inScrollbar->vertical_scrollbar->part_matrix_br[VMcColumn_Left][VMcRow_Bottom].x -
				inScrollbar->vertical_scrollbar->part_matrix_tl[VMcColumn_Left][VMcRow_Bottom].x);
			dn_arrow_height =
				(inScrollbar->vertical_scrollbar->part_matrix_br[VMcColumn_Left][VMcRow_Bottom].y -
				inScrollbar->vertical_scrollbar->part_matrix_tl[VMcColumn_Left][VMcRow_Bottom].y);
			
			inPrivateData->up_arrow_bounds.left		= 0;
			inPrivateData->up_arrow_bounds.top		= 0;
			inPrivateData->up_arrow_bounds.right	= up_arrow_width;
			inPrivateData->up_arrow_bounds.bottom	= up_arrow_height;
			
			inPrivateData->dn_arrow_bounds.left		= inView->width - dn_arrow_width;
			inPrivateData->dn_arrow_bounds.top		= inView->height - dn_arrow_height;
			inPrivateData->dn_arrow_bounds.right	= inView->width;
			inPrivateData->dn_arrow_bounds.bottom	= inView->height;
		}
		
		// ------------------------------
		// initialize the thumb data
		// ------------------------------
		if (inScrollbar->vertical_thumb)
		{
			inPrivateData->thumb_width =
				inScrollbar->vertical_thumb->part_matrix_br[VMcColumn_Left][VMcRow_Top].x -
				inScrollbar->vertical_thumb->part_matrix_tl[VMcColumn_Left][VMcRow_Top].x;
			
			inPrivateData->thumb_height =
				inScrollbar->vertical_thumb->part_matrix_br[VMcColumn_Left][VMcRow_Top].y -
				inScrollbar->vertical_thumb->part_matrix_tl[VMcColumn_Left][VMcRow_Top].y;
			
			inPrivateData->thumb_move_bounds.left	= 0;
			inPrivateData->thumb_move_bounds.top	= inPrivateData->up_arrow_bounds.bottom;
			inPrivateData->thumb_move_bounds.right	= inView->width;
			inPrivateData->thumb_move_bounds.bottom	= inPrivateData->dn_arrow_bounds.top;
			
			IMrRect_Inset(&inPrivateData->thumb_move_bounds, 1, 1);
			
			inPrivateData->thumb_x_track = (inView->width - inPrivateData->thumb_width) >> 1;
			inPrivateData->thumb_y_track = (inView->height - inPrivateData->thumb_height) >> 1;
			
			VMiView_Scrollbar_SetThumbLocation(inPrivateData);
		}
	}
	else
	{
		// ------------------------------
		// set the up and down arrow bounds
		// ------------------------------
		if (inScrollbar->horizontal_scrollbar)
		{
			UUtUns16					up_arrow_height;
			UUtUns16					up_arrow_width;
			UUtUns16					dn_arrow_height;
			UUtUns16					dn_arrow_width;
			
			up_arrow_width =
				(inScrollbar->horizontal_scrollbar->part_matrix_br[VMcColumn_Left][VMcRow_Top].x -
				inScrollbar->horizontal_scrollbar->part_matrix_tl[VMcColumn_Left][VMcRow_Top].x);
			up_arrow_height =
				(inScrollbar->horizontal_scrollbar->part_matrix_br[VMcColumn_Left][VMcRow_Top].y -
				inScrollbar->horizontal_scrollbar->part_matrix_tl[VMcColumn_Left][VMcRow_Top].y);
			
			dn_arrow_width =
				(inScrollbar->horizontal_scrollbar->part_matrix_br[VMcColumn_Right][VMcRow_Top].x -
				inScrollbar->horizontal_scrollbar->part_matrix_tl[VMcColumn_Right][VMcRow_Top].x);
			dn_arrow_height =
				(inScrollbar->horizontal_scrollbar->part_matrix_br[VMcColumn_Right][VMcRow_Top].y -
				inScrollbar->horizontal_scrollbar->part_matrix_tl[VMcColumn_Right][VMcRow_Top].y);
			
			inPrivateData->up_arrow_bounds.left		= 0;
			inPrivateData->up_arrow_bounds.top		= 0;
			inPrivateData->up_arrow_bounds.right	= up_arrow_width;
			inPrivateData->up_arrow_bounds.bottom	= up_arrow_height;
			
			inPrivateData->dn_arrow_bounds.left		= inView->width - dn_arrow_width;
			inPrivateData->dn_arrow_bounds.top		= 0;
			inPrivateData->dn_arrow_bounds.right	= inView->width;
			inPrivateData->dn_arrow_bounds.bottom	= dn_arrow_height;
		}
		
		// ------------------------------
		// initialize the thumb data
		// ------------------------------
		if (inScrollbar->horizontal_thumb)
		{
			inPrivateData->thumb_width =
				inScrollbar->horizontal_thumb->part_matrix_br[VMcColumn_Left][VMcRow_Top].x -
				inScrollbar->horizontal_thumb->part_matrix_tl[VMcColumn_Left][VMcRow_Top].x;
			
			inPrivateData->thumb_height =
				inScrollbar->horizontal_thumb->part_matrix_br[VMcColumn_Left][VMcRow_Top].y -
				inScrollbar->horizontal_thumb->part_matrix_tl[VMcColumn_Left][VMcRow_Top].y;
			
			inPrivateData->thumb_move_bounds.left	= inPrivateData->up_arrow_bounds.right;
			inPrivateData->thumb_move_bounds.top	= 0;
			inPrivateData->thumb_move_bounds.right	= inPrivateData->dn_arrow_bounds.left;
			inPrivateData->thumb_move_bounds.bottom	= inView->height;
			
			IMrRect_Inset(&inPrivateData->thumb_move_bounds, 1, 1);
			
			inPrivateData->thumb_x_track = (inView->width - inPrivateData->thumb_width) >> 1;
			inPrivateData->thumb_y_track = (inView->height - inPrivateData->thumb_height) >> 1;
			
			VMiView_Scrollbar_SetThumbLocation(inPrivateData);
		}
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
VMiView_Scrollbar_Create(
	VMtView							*inView,
	VMtView_Scrollbar				*inScrollbar,
	VMtView_Scrollbar_PrivateData	*inPrivateData)
{
	// ------------------------------
	// initialize the scrollbar private data
	// ------------------------------
	UUrMemory_Clear(inPrivateData, sizeof(VMtView_Scrollbar_PrivateData));
	
	inPrivateData->is_vertical		= inView->height > inView->width;
	inPrivateData->min				= 0;
	inPrivateData->max				= 100;
	inPrivateData->current_value	= 0;
	
	VMiView_Scrollbar_SetBounds(inView, inScrollbar, inPrivateData);
	
	// ------------------------------
	// start the timer
	// ------------------------------
	VMrView_Timer_Start(inView, VMcView_Scrollbar_Timer, VMcView_Scrollbar_TimerFrequency);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
VMiView_Scrollbar_HandleMouseEvent(
	VMtView							*inView,
	VMtView_PrivateData				*inViewPrivateData,
	VMtView_Scrollbar_PrivateData	*inPrivateData,
	VMtMessage						inMessage,
	UUtUns32						inButtonState,
	IMtPoint2D						*inMouseLocation)
{
	UUtBool							in_page_up_bounds;
	UUtBool							in_page_dn_bounds;
	UUtBool							in_up_arrow_bounds;
	UUtBool							in_dn_arrow_bounds;
	UUtBool							in_thumb_bounds;
	UUtRect							thumb_bounds;
	UUtInt32						new_value;
					
	
	// set the thumb_bounds
	thumb_bounds.left	= inPrivateData->thumb_location.x;
	thumb_bounds.top	= inPrivateData->thumb_location.y;
	thumb_bounds.right	= thumb_bounds.left + inPrivateData->thumb_width;
	thumb_bounds.bottom	= thumb_bounds.top + inPrivateData->thumb_height;
	
	// determine where the mouse is
	in_page_up_bounds = IMrRect_PointIn(&inPrivateData->page_up_bounds, inMouseLocation);
	in_page_dn_bounds = IMrRect_PointIn(&inPrivateData->page_dn_bounds, inMouseLocation);
	in_up_arrow_bounds = IMrRect_PointIn(&inPrivateData->up_arrow_bounds, inMouseLocation);
	in_dn_arrow_bounds = IMrRect_PointIn(&inPrivateData->dn_arrow_bounds, inMouseLocation);
	in_thumb_bounds = IMrRect_PointIn(&thumb_bounds, inMouseLocation);
	
	switch (inMessage)
	{
		case VMcMessage_MouseMove:
			if ((inButtonState & LIcMouseState_LButtonDown) &&
				(inPrivateData->mouse_down_in_thumb))
			{
				new_value =
					VMiView_Scrollbar_GetNewValue(
						inPrivateData,
						inMouseLocation);
				
				// tell the parent of the thumb move
				VMrView_SendMessage(
					inViewPrivateData->parent,
					inPrivateData->is_vertical ? SBcMessage_VerticalScroll : SBcMessage_HorizontalScroll,
					(UUtUns32)UUmMakeLong(inView->id, SBcNotify_ThumbPosition),
					(UUtInt32)new_value);
			}
		break;
		
		case VMcMessage_LMouseDown:
			inPrivateData->mouse_state |= VMcMouseState_MouseDown;
			DMrDialog_SetMouseFocusView(NULL, inView);
			if (in_page_up_bounds)
			{
				// tell the parent of the thumb move
				VMrView_SendMessage(
					inViewPrivateData->parent,
					inPrivateData->is_vertical ? SBcMessage_VerticalScroll : SBcMessage_HorizontalScroll,
					(UUtUns32)UUmMakeLong(inView->id, SBcNotify_PageUp),
					(UUtUns32)inPrivateData->current_value);
			}
			else if (in_page_dn_bounds)
			{
				// tell the parent of the thumb move
				VMrView_SendMessage(
					inViewPrivateData->parent,
					inPrivateData->is_vertical ? SBcMessage_VerticalScroll : SBcMessage_HorizontalScroll,
					(UUtUns32)UUmMakeLong(inView->id, SBcNotify_PageDown),
					(UUtUns32)inPrivateData->current_value);
			}
			else if (in_up_arrow_bounds)
			{
				// tell the parent of the line up
				VMrView_SendMessage(
					inViewPrivateData->parent,
					inPrivateData->is_vertical ? SBcMessage_VerticalScroll : SBcMessage_HorizontalScroll,
					(UUtUns32)UUmMakeLong(inView->id, SBcNotify_LineUp),
					(UUtUns32)inPrivateData->current_value);
			}
			else if (in_dn_arrow_bounds)
			{
				// tell the parent of the line down
				VMrView_SendMessage(
					inViewPrivateData->parent,
					inPrivateData->is_vertical ? SBcMessage_VerticalScroll : SBcMessage_HorizontalScroll,
					(UUtUns32)UUmMakeLong(inView->id, SBcNotify_LineDown),
					(UUtUns32)inPrivateData->current_value);
			}
			else if (in_thumb_bounds)
			{
				if ((inButtonState & LIcMouseState_LButtonDown) &&
					(inPrivateData->mouse_down_in_thumb))
				{
					new_value =
						VMiView_Scrollbar_GetNewValue(
							inPrivateData,
							inMouseLocation);
					
					// tell the parent of the thumb move
					VMrView_SendMessage(
						inViewPrivateData->parent,
						inPrivateData->is_vertical ? SBcMessage_VerticalScroll : SBcMessage_HorizontalScroll,
						(UUtUns32)UUmMakeLong(inView->id, SBcNotify_ThumbPosition),
						(UUtInt32)new_value);
				}
				else
				{
					inPrivateData->mouse_down_in_thumb = UUcTrue;
				}
			}
		break;
		
		case VMcMessage_LMouseUp:
			inPrivateData->mouse_state &= ~VMcMouseState_MouseDown;
			inPrivateData->mouse_down_in_thumb = UUcFalse;
			DMrDialog_ReleaseMouseFocusView(NULL, inView);
		break;
	}
}

// ----------------------------------------------------------------------
static void
VMiView_Scrollbar_Paint(
	VMtView							*inView,
	VMtView_Scrollbar				*inScrollbar,
	VMtView_Scrollbar_PrivateData	*inPrivateData,
	M3tPointScreen					*inDestination)
{
	UUtUns16					alpha;
	M3tPointScreen				dest;

	// set the alpha
	if (inView->flags & VMcViewFlag_Enabled)
		alpha = VUcAlpha_Enabled;
	else
		alpha = VUcAlpha_Disabled;
	
	if (inPrivateData->is_vertical)
	{
		// draw the scrollbar
		if (inScrollbar->vertical_scrollbar)
		{
			VUrDrawPartSpecification(
				inScrollbar->vertical_scrollbar,
				VMcPart_LeftColumn,
				inDestination,
				inView->width,
				inView->height,
				alpha);
		}
		
		// draw the thumb
		if (inScrollbar->vertical_thumb)
		{
			dest = *inDestination;
			dest.x += (float)inPrivateData->thumb_location.x;
			dest.y += (float)inPrivateData->thumb_location.y;
			
			VUrDrawPartSpecification(
				inScrollbar->vertical_thumb,
				VMcPart_LeftTop,
				&dest,
				inPrivateData->thumb_width,
				inPrivateData->thumb_height,
				alpha);
		}
	}
	else
	{
		// draw the scrollbar
		if (inScrollbar->horizontal_scrollbar)
		{
			VUrDrawPartSpecification(
				inScrollbar->horizontal_scrollbar,
				VMcPart_TopRow,
				inDestination,
				inView->width,
				inView->height,
				alpha);
		}
		
		// draw the thumb
		if (inScrollbar->horizontal_thumb)
		{
			dest = *inDestination;
			dest.x += (float)inPrivateData->thumb_location.x;
			dest.y += (float)inPrivateData->thumb_location.y;
			
			VUrDrawPartSpecification(
				inScrollbar->horizontal_thumb,
				VMcPart_LeftTop,
				&dest,
				inPrivateData->thumb_width,
				inPrivateData->thumb_height,
				alpha);
		}
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtUns32
VMrView_Scrollbar_Callback(
	VMtView				*inView,
	VMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	VMtView_PrivateData				*view_private_data;
	VMtView_Scrollbar				*scrollbar;
	VMtView_Scrollbar_PrivateData	*private_data;
	
	IMtPoint2D			global_mouse_loc;
	IMtPoint2D			local_mouse_loc;
			
	// get the data
	view_private_data = (VMtView_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_View_PrivateData, inView);
	UUmAssert(view_private_data);

	scrollbar = (VMtView_Scrollbar*)inView->view_data;
	UUmAssert(scrollbar);

	private_data = (VMtView_Scrollbar_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Scrollbar_PrivateData, scrollbar);
	UUmAssert(private_data);
	
	switch (inMessage)
	{
		case VMcMessage_Create:
			VMiView_Scrollbar_Create(inView, scrollbar, private_data);
		return 0;
		
		case VMcMessage_Timer:
			if ((private_data->mouse_state & VMcMouseState_MouseDown) &&
				(inParam2 == VMcView_Scrollbar_Timer))
			{
				LItInputEvent		event;
				
				LIrInputEvent_GetMouse(&event);
				VMrView_PointGlobalToView(inView, &event.where, &local_mouse_loc);
				
				VMiView_Scrollbar_HandleMouseEvent(
					inView,
					view_private_data,
					private_data,
					VMcMessage_LMouseDown,
					(UUtUns32)event.modifiers,
					&local_mouse_loc);
			}
		return 0;
		
		case VMcMessage_MouseMove:
		case VMcMessage_MouseLeave:
		case VMcMessage_LMouseDown:
		case VMcMessage_LMouseUp:
		{
			// get the mouse location in local coordinates
			global_mouse_loc.x = (UUtInt16)UUmHighWord(inParam1);
			global_mouse_loc.y = (UUtInt16)UUmLowWord(inParam1);
			VMrView_PointGlobalToView(inView, &global_mouse_loc, &local_mouse_loc);
			
			VMiView_Scrollbar_HandleMouseEvent(
				inView,
				view_private_data,
				private_data,
				inMessage,
				inParam2,
				&local_mouse_loc);
		}
		return 0;
		
		case VMcMessage_Paint:
			// draw the slider
			VMiView_Scrollbar_Paint(
				inView,
				scrollbar,
				private_data,
				(M3tPointScreen*)inParam2);
		return 0;
		
		case VMcMessage_Size:
			VMiView_Scrollbar_SetBounds(
				inView,
				scrollbar,
				private_data);
		return 0;
	}
	
	return VMrView_DefaultCallback(inView, inMessage, inParam1, inParam2);
}

// ----------------------------------------------------------------------
UUtInt32
VMrView_Scrollbar_GetPosition(
	VMtView					*inView)
{
	VMtView_Scrollbar				*scrollbar;
	VMtView_Scrollbar_PrivateData	*private_data;
	
	if (inView == NULL) return 0;
	
	// get a pointer to the scrollbar data
	scrollbar = (VMtView_Scrollbar*)inView->view_data;
	private_data = (VMtView_Scrollbar_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Scrollbar_PrivateData, scrollbar);
	UUmAssert(private_data);
	
	// set the new range values
	return private_data->current_value;
}

// ----------------------------------------------------------------------
void
VMrView_Scrollbar_GetRange(
	VMtView					*inView,
	UUtInt32				*outMin,
	UUtInt32				*outMax)
{
	VMtView_Scrollbar				*scrollbar;
	VMtView_Scrollbar_PrivateData	*private_data;
	
	*outMin = 0;
	*outMax = 0;
	if (inView == NULL) return;
	
	// get a pointer to the scrollbar data
	scrollbar = (VMtView_Scrollbar*)inView->view_data;
	private_data = (VMtView_Scrollbar_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Scrollbar_PrivateData, scrollbar);
	UUmAssert(private_data);
	
	*outMin = private_data->min;
	*outMax = private_data->max;
}

// ----------------------------------------------------------------------
UUtError
VMrView_Scrollbar_ProcHandler(
	TMtTemplateProc_Message			inMessage,
	void							*inInstancePtr,
	void*							inPrivateData)
{
	VMtView_Scrollbar				*scrollbar;
	VMtView_Scrollbar_PrivateData	*private_data;
	
	// get a pointer to the scrollbar data
	scrollbar = (VMtView_Scrollbar*)inInstancePtr;
	private_data = (VMtView_Scrollbar_PrivateData*)inPrivateData;
	UUmAssert(private_data);

	switch (inMessage)
	{
		case TMcTemplateProcMessage_NewPostProcess:
		case TMcTemplateProcMessage_LoadPostProcess:
			// initalize the private data
			private_data->min						= 0;
			private_data->max						= 100;
			private_data->current_value				= 0;
		break;
		
		case TMcTemplateProcMessage_DisposePreProcess:
		break;
		
		case TMcTemplateProcMessage_Update:
		break;
		
		default:
			UUmAssert(!"Illegal message");
		break;
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
VMrView_Scrollbar_SetPosition(
	VMtView					*inView,
	UUtInt32				inPosition)
{
	VMtView_Scrollbar				*scrollbar;
	VMtView_Scrollbar_PrivateData	*private_data;
	
	if (inView == NULL) return;
	
	// get a pointer to the scrollbar data
	scrollbar = (VMtView_Scrollbar*)inView->view_data;
	private_data = (VMtView_Scrollbar_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Scrollbar_PrivateData, scrollbar);
	UUmAssert(private_data);
	
	// set the new range values
	private_data->current_value = inPosition;
	
	// make sure the new value is within the range
	if (private_data->current_value < private_data->min)
		private_data->current_value = private_data->min;
	else if (private_data->current_value > private_data->max)
		private_data->current_value = private_data->max;

	VMiView_Scrollbar_SetThumbLocation(private_data);
}

// ----------------------------------------------------------------------
void
VMrView_Scrollbar_SetRange(
	VMtView					*inView,
	UUtInt32				inMin,
	UUtInt32				inMax)
{
	VMtView_Scrollbar				*scrollbar;
	VMtView_Scrollbar_PrivateData	*private_data;
	
	if (inView == NULL) return;
	
	// get a pointer to the scrollbar data
	scrollbar = (VMtView_Scrollbar*)inView->view_data;
	private_data = (VMtView_Scrollbar_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Scrollbar_PrivateData, scrollbar);
	UUmAssert(private_data);
	
	// set the new range values
	private_data->min = inMin;
	private_data->max = inMax;
	private_data->current_value = inMin;
	
	VMiView_Scrollbar_SetThumbLocation(private_data);
}
