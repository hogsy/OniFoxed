// ======================================================================
// BFW_ViewManager.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_TemplateManager.h"
#include "BFW_LocalInput.h"
#include "BFW_ViewManager.h"
#include "BFW_Timer.h"

#include "VM_View_Box.h"
#include "VM_View_Button.h"
#include "VM_View_CheckBox.h"
#include "VM_View_Dialog.h"
#include "VM_View_EditField.h"
#include "VM_View_ListBox.h"
#include "VM_View_Picture.h"
#include "VM_View_RadioButton.h"
#include "VM_View_RadioGroup.h"
#include "VM_View_Scrollbar.h"
#include "VM_View_Slider.h"
#include "VM_View_Tab.h"
#include "VM_View_TabGroup.h"
#include "VM_View_Text.h"

TMtPrivateData*	DMgTemplate_PartSpec_PrivateData = NULL;
TMtPrivateData*	DMgTemplate_Button_PrivateData = NULL;
TMtPrivateData*	DMgTemplate_CheckBox_PrivateData = NULL;
TMtPrivateData*	DMgTemplate_EditField_PrivateData = NULL;
TMtPrivateData*	DMgTemplate_ListBox_PrivateData = NULL;
TMtPrivateData*	DMgTemplate_RadioButton_PrivateData = NULL;
TMtPrivateData*	DMgTemplate_RadioGroup_PrivateData = NULL;
TMtPrivateData*	DMgTemplate_Tab_PrivateData = NULL;
TMtPrivateData*	DMgTemplate_Text_PrivateData = NULL;
TMtPrivateData*	DMgTemplate_Slider_PrivateData = NULL;
TMtPrivateData*	DMgTemplate_Scrollbar_PrivateData = NULL;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
VMiView_Create(
	VMtView					*inView,
	VMtView					*inParent,
	UUtUns16				inLayer)
{
	UUtUns16				i;
	UUtBool					result;
	VMtView_PrivateData		*private_data;

	// get the private data of the view
	private_data = (VMtView_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_View_PrivateData, inView);
	UUmAssert(private_data);

	// set the view's parent
	private_data->parent = inParent;
	private_data->layer	= inLayer;

	// tell this view that it was created
	result = (UUtBool)VMrView_SendMessage(inView, VMcMessage_Create, 0, 0);

	// load the child views
	for (i = 0; i < inView->num_child_views; i++)
	{
		VMiView_Create(
			(VMtView*)(inView->child_views[i].view_ref),
			inView,
			inLayer + 1);
	}

	return result;
}

// ----------------------------------------------------------------------
static void
VMiView_Destory(
	VMtView					*inView)
{
	UUtUns16				i;

	VMrView_SendMessage(inView, VMcMessage_Destroy, 0, 0);

	for (i = 0; i < inView->num_child_views; i++)
	{
		VMtView				*child = (VMtView*)inView->child_views[i].view_ref;

		VMiView_Destory(child);
	}
}

// ----------------------------------------------------------------------
static UUtBool
VMiView_IsGroup(
	VMtView					*inView)
{
	UUtBool					is_group;

	is_group = UUcFalse;
	switch (inView->type)
	{
		case VMcViewType_RadioGroup:
		case VMcViewType_Tab:
		case VMcViewType_TabGroup:
			is_group = UUcTrue;
		break;
	}

	return is_group;
}

// ----------------------------------------------------------------------
static UUtBool
VMiView_GetViewUnderPoint(
	VMtView					*inView,
	IMtPoint2D				*inPoint,
	UUtUns16				inFlags,
	VMtView					**outView,
	UUtUns16				*outLayer)
{
	VMtView_PrivateData		*private_data;
	UUtRect					bounds_rect;
	VMtView					*found_view;
	UUtUns16				deepest_layer;
	UUtBool					view_found;

	UUmAssert(inView);

	// get the data
	private_data = (VMtView_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_View_PrivateData, inView);
	UUmAssert(private_data);

	// initalize the data
	found_view = NULL;
	deepest_layer = 0;
	view_found = UUcFalse;

	// initialize the bounds_rect
	bounds_rect.left = inView->location.x;
	bounds_rect.top = inView->location.y;
	bounds_rect.right = bounds_rect.left + inView->width;
	bounds_rect.bottom = bounds_rect.top + inView->height;

	// check to see if the point is in the bounds of the view
	if (IMrRect_PointIn(&bounds_rect, inPoint) ||
		VMiView_IsGroup(inView))
	{
		UUtUns16			i;
		IMtPoint2D			new_point;

		// make sure the flags match
		if (((inView->flags & inFlags) == inFlags) &&
			!VMiView_IsGroup(inView))
		{
			// set the found_view and deepest_layer
			found_view = inView;
			deepest_layer = private_data->layer;
			view_found = UUcTrue;
		}

		// calculate a parent relative point
		new_point = *inPoint;
		new_point.x -= inView->location.x;
		new_point.y -= inView->location.y;

		// search the child views
		for (i = 0; i < inView->num_child_views; i++)
		{
			VMtView		*child = (VMtView*)inView->child_views[i].view_ref;
			UUtBool		result;
			VMtView		*out_view;
			UUtUns16	out_layer;

			// don't look in views that aren't visible
			if (!(child->flags & VMcViewFlag_Visible)) continue;

			// is the point in the child view?
			result =
				VMiView_GetViewUnderPoint(
					child,
					&new_point,
					inFlags,
					&out_view,
					&out_layer);
			if (result)
			{
				if (out_layer > deepest_layer)
				{
					// this child view contains the point or has a child which does
					// and that child is deeper that the deepest view containing
					// the point so far
					deepest_layer = out_layer;
					found_view = out_view;
					view_found = UUcTrue;
				}
			}
		}
	}

	// set the outgoing data
	*outView = found_view;
	*outLayer = deepest_layer;
	return view_found;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtUns32
VMrView_DefaultCallback(
	VMtView					*inView,
	VMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	VMtView_PrivateData		*private_data;
	UUtUns16				i;

	// get a pointer to the private data
	private_data = (VMtView_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_View_PrivateData, inView);
	UUmAssert(private_data);

	// handle the message
	switch (inMessage)
	{
		case VMcMessage_Paint:
		{
			M3tPointScreen		*destination;

			// tell the child view to draw
			destination = (M3tPointScreen*)inParam2;

			// draw the child windows
			for (i = 0; i < inView->num_child_views; i++)
			{
				VMtView			*child_view;
				M3tPointScreen	dest;

				// get a pointer to the child view
				child_view = inView->child_views[i].view_ref;

				dest = *destination;
				dest.x += (float)child_view->location.x;
				dest.y += (float)child_view->location.y;

				VMrView_Draw(child_view, &dest);
			}
		}
		break;
	}

	return 0;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
VMrView_Timer_Start(
	VMtView					*inView,
	UUtUns16				inTimerID,
	UUtUns32				inTimerFrequency)
{
	VMtView_PrivateData		*private_data;
	VMtViewTimer			*timer;

	UUmAssert(inView);

	// get the private data of the view
	private_data = (VMtView_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_View_PrivateData, inView);
	UUmAssert(private_data);

	// look for timers with the same ID number
	timer = private_data->timers;
	while (timer)
	{
		if (timer->timer_id == inTimerID)
		{
			return UUcError_Generic;
		}
	}

	// create a new timer
	timer = (VMtViewTimer*)UUrMemory_Block_New(sizeof(VMtViewTimer));
	UUmError_ReturnOnNull(timer);

	// initialize the timer
	timer->next				= private_data->timers;
	timer->timer_id			= inTimerID;
	timer->timer_frequency	= inTimerFrequency;
	timer->next_update		= 0;

	// add the timer to the list
	private_data->timers = timer;

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
VMrView_Timer_Stop(
	VMtView					*inView,
	UUtUns16				inTimerID)
{
	VMtView_PrivateData		*private_data;
	VMtViewTimer			*current;
	VMtViewTimer			*previous;

	UUmAssert(inView);

	// get the private data of the view
	private_data = (VMtView_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_View_PrivateData, inView);
	UUmAssert(private_data);

	// find the timer in the list
	previous = NULL;
	current = private_data->timers;
	while (current)
	{
		// stop if the timer with inTimerID has been found
		if (current->timer_id == inTimerID)
			break;

		// move to the next timer
		previous = current;
		current = current->next;
	}

	// if current == NULL then no matching timer was found
	if (current)
	{
		// remove current from the list
		if (previous)
			previous->next = current->next;
		else
			private_data->timers = current->next;

		UUrMemory_Block_Delete(current);
	}
}

// ----------------------------------------------------------------------
void
VMrView_Timer_Update(
	VMtView					*inView)
{
	VMtView_PrivateData		*private_data;
	UUtUns16				i;

	UUmAssert(inView);

	// get the private data of the view
	private_data = (VMtView_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_View_PrivateData, inView);
	UUmAssert(private_data);

	if (private_data->timers)
	{
		VMtViewTimer			*timer;
		UUtUns32				time;

		// get the current time
		time = UUrMachineTime_Sixtieths();

		// update the timers
		timer = private_data->timers;
		while (timer)
		{
			if (timer->next_update < time)
			{
				// set the next update
				timer->next_update = time + timer->timer_frequency;

				// send VMcMessage_Timer to the view
				VMrView_SendMessage(
					inView,
					VMcMessage_Timer,
					time,
					timer->timer_id);
			}

			timer = timer->next;
		}
	}

	// update the children of this view
	for (i = 0; i < inView->num_child_views; i++)
	{
		VMrView_Timer_Update((VMtView*)inView->child_views[i].view_ref);
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
VMrView_Draw(
	VMtView					*inView,
	M3tPointScreen			*inDestination)
{
	UUmAssert(inView);

	if (inView->flags & VMcViewFlag_Visible)
	{
		VMrView_SendMessage(
			inView,
			VMcMessage_Paint,
			0,
			(UUtUns32)inDestination);
	}
}

// ----------------------------------------------------------------------
VMtView*
VMrView_GetNextFocusableView(
	VMtView					*inView,
	VMtView					*inCurrentFocus)
{
	UUtUns16				i;
	UUtUns16				current_focus_index;
	UUtUns16				desired_flags;
	VMtView					*next_focusable_view;
	VMtView					*child;

	// setup the variables
	current_focus_index = 0;
	next_focusable_view = NULL;
	desired_flags =
		VMcViewFlag_Visible |
		VMcViewFlag_Enabled |
		VMcViewFlag_Active |
		VMcViewFlag_CanFocus;

	// find the current view in the list
	if (inCurrentFocus)
	{
		for (i = 0; i < inView->num_child_views; i++)
		{
			if (inCurrentFocus == (VMtView*)inView->child_views[i].view_ref)
				break;
		}

		current_focus_index = i;
	}

	// search the rest of the list for the view
	for (i = current_focus_index + 1; i < inView->num_child_views; i++)
	{
		child = inView->child_views[i].view_ref;

		// check the flags
		if ((child->flags & desired_flags) == desired_flags)
		{
			// child is the next focusable view, stop now
			next_focusable_view = child;
			break;
		}
	}

	// NOTE: if inCurrentFocus == NULL then the whole list has already been searched
	if ((next_focusable_view == NULL) && (inCurrentFocus != NULL))
	{
		// search the list from the beginning through the current focused view
		// the current focused view may be the only view in the list that can be
		// focused on and needs to be returned if it is
		for (i = 0; i < current_focus_index + 1; i++)
		{
			child = inView->child_views[i].view_ref;

			// check the flags
			if ((child->flags & desired_flags) == desired_flags)
			{
				// child is the next focusable view, stop now
				next_focusable_view = child;
				break;
			}
		}
	}

	return next_focusable_view;
}

// ----------------------------------------------------------------------
VMtView*
VMrView_GetParent(
	VMtView					*inView)
{
	VMtView_PrivateData		*private_data;

	// get the data
	private_data = (VMtView_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_View_PrivateData, inView);
	UUmAssert(private_data);

	return private_data->parent;
}

// ----------------------------------------------------------------------
UUtUns32
VMrView_GetValue(
	VMtView					*inView)
{
	UUtUns32				result;

	UUmAssert(inView);

	result =
		VMrView_SendMessage(
			inView,
			VMcMessage_GetValue,
			0,
			0);

	return result;
}

// ----------------------------------------------------------------------
VMtView*
VMrView_GetViewUnderPoint(
	VMtView					*inView,
	IMtPoint2D				*inPoint,
	UUtUns16				inFlags)
{
	VMtView					*view;
	UUtUns16				layer;

	VMiView_GetViewUnderPoint(inView, inPoint, inFlags, &view, &layer);

	return view;
}

// ----------------------------------------------------------------------
UUtError
VMrView_Load(
	VMtView					*inView,
	VMtView					*inParent)
{
	// load and initialize the views
	VMiView_Create(inView, inParent, 0);

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
VMrView_PointGlobalToView(
	const VMtView			*inView,
	const IMtPoint2D		*inPoint,
	IMtPoint2D				*outPoint)
{
	VMtView_PrivateData		*private_data;
	IMtPoint2D				new_point;

	// get a pointer to the view's private data
	private_data = (VMtView_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_View_PrivateData, inView);
	UUmAssert(private_data);

	// calculate the new point
	new_point = *inPoint;
	new_point.x -= inView->location.x;
	new_point.y -= inView->location.y;

	// save the new_point
	*outPoint = new_point;

	// continue calculating with the parent
	if ((inView->type != VMcViewType_Dialog) &&
		(private_data->parent))
	{
		VMrView_PointGlobalToView(
			private_data->parent,
			&new_point,
			outPoint);
	}
}

// ----------------------------------------------------------------------
UUtError
VMrView_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData)
{
	VMtView					*view;
	VMtView_PrivateData		*private_data;

	UUmAssert(inInstancePtr);

	// get a pointer to the view
	view = (VMtView*)inInstancePtr;
//	UUmAssert(view->view_data);

	// get a pointer to the view's private data
	private_data = (VMtView_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_View_PrivateData, view);
	UUmAssert(private_data);

	switch(inMessage)
	{
		case TMcTemplateProcMessage_NewPostProcess:
		case TMcTemplateProcMessage_LoadPostProcess:
			// initialize the private data
			private_data->callback = NULL;
			private_data->parent = NULL;
			private_data->timers = NULL;
			private_data->layer = 0;

			// add dialog views to the view list
			switch (view->type)
			{
				case VMcViewType_Box:
					private_data->callback = VMrView_Box_Callback;
				break;

				case VMcViewType_Button:
					private_data->callback = VMrView_Button_Callback;
				break;

				case VMcViewType_CheckBox:
					private_data->callback = VMrView_CheckBox_Callback;
				break;

				case VMcViewType_Dialog:
					private_data->callback = VMrView_Dialog_Callback;
				break;

				case VMcViewType_EditField:
					private_data->callback = VMrView_EditField_Callback;
				break;

				case VMcViewType_ListBox:
					private_data->callback = VMrView_ListBox_Callback;
				break;

				case VMcViewType_Picture:
					private_data->callback = VMrView_Picture_Callback;
				break;

				case VMcViewType_RadioButton:
					private_data->callback = VMrView_RadioButton_Callback;
				break;

				case VMcViewType_RadioGroup:
					private_data->callback = VMrView_RadioGroup_Callback;
				break;

				case VMcViewType_Scrollbar:
					private_data->callback = VMrView_Scrollbar_Callback;
				break;

				case VMcViewType_Slider:
					private_data->callback = VMrView_Slider_Callback;
				break;

				case VMcViewType_Tab:
					private_data->callback = VMrView_Tab_Callback;
				break;

				case VMcViewType_TabGroup:
					private_data->callback = VMrView_TabGroup_Callback;
				break;

				case VMcViewType_Text:
					private_data->callback = VMrView_Text_Callback;
				break;

				default:
					UUmAssert(!"Unknown View Type");
				break;
			}
		break;

		case TMcTemplateProcMessage_DisposePreProcess:
			while (private_data->timers)
			{
				VMrView_Timer_Stop(view, private_data->timers->timer_id);
			}
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
UUtUns32
VMrView_SendMessage(
	VMtView					*inView,
	VMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	VMtView_PrivateData		*private_data;

	UUmAssert(inView);

	// get a pointer to the view's private data
	private_data = (VMtView_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_View_PrivateData, inView);
	UUmAssert(private_data);
	UUmAssert(private_data->callback);

	return private_data->callback(inView, inMessage, inParam1, inParam2);
}

// ----------------------------------------------------------------------
void
VMrView_SetActive(
	VMtView					*inView,
	UUtBool					inIsActive)
{
	UUmAssert(inView);

	if (inIsActive)
		inView->flags |= VMcViewFlag_Active;
	else
		inView->flags &= ~VMcViewFlag_Active;
}

// ----------------------------------------------------------------------
void
VMrView_SetCallback(
	VMtView					*inView,
	VMtViewCallback			inCallback)
{
	VMtView_PrivateData		*private_data;

	UUmAssert(inView);
	UUmAssert(inCallback);

	// get the private data
	private_data = (VMtView_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_View_PrivateData, inView);
	UUmAssert(private_data);

	private_data->callback = inCallback;
}

// ----------------------------------------------------------------------
void
VMrView_SetEnabled(
	VMtView					*inView,
	UUtBool					inIsEnabled)
{
	UUmAssert(inView);

	if (inIsEnabled)
		inView->flags |= VMcViewFlag_Enabled;
	else
		inView->flags &= ~VMcViewFlag_Enabled;
}

// ----------------------------------------------------------------------
void
VMrView_SetFocus(
	VMtView					*inView,
	UUtBool					inIsFocused)
{
	UUtUns16				desired_flags;

	UUmAssert(inView);

	desired_flags =
		VMcViewFlag_Visible |
		VMcViewFlag_Enabled |
		VMcViewFlag_Active |
		VMcViewFlag_CanFocus;

	if ((inView->flags & desired_flags) == desired_flags)
	{
		VMrView_SendMessage(
			inView,
			VMcMessage_SetFocus,
			(UUtUns32)inIsFocused,
			0);
	}
}

// ----------------------------------------------------------------------
void
VMrView_SetLocation(
	VMtView					*inView,
	IMtPoint2D				*inLocation)
{
	UUmAssert(inView);

	inView->location = *inLocation;

	VMrView_SendMessage(
		inView,
		VMcMessage_Location,
		UUmMakeLong(inLocation->x, inLocation->y),
		0);
}

// ----------------------------------------------------------------------
void
VMrView_SetSize(
	VMtView					*inView,
	UUtUns16				inWidth,
	UUtUns16				inHeight)
{
	UUmAssert(inView);

	inView->width = inWidth;
	inView->height = inHeight;

	VMrView_SendMessage(
		inView,
		VMcMessage_Size,
		UUmMakeLong(inWidth, inHeight),
		0);
}

// ----------------------------------------------------------------------
void
VMrView_SetValue(
	VMtView					*inView,
	UUtUns32				inValue)
{
	UUmAssert(inView);

	VMrView_SendMessage(
		inView,
		VMcMessage_SetValue,
		inValue,
		0);
}

// ----------------------------------------------------------------------
void
VMrView_SetVisible(
	VMtView					*inView,
	UUtBool					inIsVisible)
{
	UUmAssert(inView);

	if (inIsVisible)
		inView->flags |= VMcViewFlag_Visible;
	else
		inView->flags &= ~VMcViewFlag_Visible;
}

// ----------------------------------------------------------------------
void
VMrView_Unload(
	VMtView					*inView)
{
	UUmAssert(inView);

	VMiView_Destory(inView);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
VMiPartSpec_CalcUV(
	VMtPartSpec				*inPartSpec,
	M3tTextureCoord			*ioUVs,
	UUtUns16				inColumn,
	UUtUns16				inRow)
{
	M3tTextureMap			*texture;
	float					invTextureWidth;
	float					invTextureHeight;
	float					lu;
	float					ru;
	float					tv;
	float					bv;

	texture = (M3tTextureMap*)inPartSpec->texture;

	invTextureWidth = 1.0f / (float)texture->width;
	invTextureHeight = 1.0f / (float)texture->height;

	lu = (float)inPartSpec->part_matrix_tl[inColumn][inRow].x * invTextureWidth;
	tv = (float)inPartSpec->part_matrix_tl[inColumn][inRow].y * invTextureHeight;

	bv = (float)inPartSpec->part_matrix_br[inColumn][inRow].y * invTextureHeight;
	ru = (float)inPartSpec->part_matrix_br[inColumn][inRow].x * invTextureWidth;

	// tl
	ioUVs[0].u = lu;
	ioUVs[0].v = tv;

	// tr
	ioUVs[1].u = ru;
	ioUVs[1].v = tv;

	// bl
	ioUVs[2].u = lu;
	ioUVs[2].v = bv;

	// br
	ioUVs[3].u = ru;
	ioUVs[3].v = bv;
}

// ----------------------------------------------------------------------
static UUtError
VMiPartSpec_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData)
{
	VMtPartSpec				*part_spec;
	VMtPartSpec_PrivateData	*private_data;

	UUmAssert(inInstancePtr);

	// get a pointer to the part spec
	part_spec = (VMtPartSpec*)inInstancePtr;

	// get a pointer to the view's private data
	private_data = (VMtPartSpec_PrivateData*)inPrivateData;
	UUmAssert(private_data);

	switch(inMessage)
	{
		case TMcTemplateProcMessage_NewPostProcess:
		case TMcTemplateProcMessage_LoadPostProcess:
			// precalculate all of the UVs
			VMiPartSpec_CalcUV(part_spec, private_data->lt, 0, 0);
			VMiPartSpec_CalcUV(part_spec, private_data->lm, 0, 1);
			VMiPartSpec_CalcUV(part_spec, private_data->lb, 0, 2);

			VMiPartSpec_CalcUV(part_spec, private_data->mt, 1, 0);
			VMiPartSpec_CalcUV(part_spec, private_data->mm, 1, 1);
			VMiPartSpec_CalcUV(part_spec, private_data->mb, 1, 2);

			VMiPartSpec_CalcUV(part_spec, private_data->rt, 2, 0);
			VMiPartSpec_CalcUV(part_spec, private_data->rm, 2, 1);
			VMiPartSpec_CalcUV(part_spec, private_data->rb, 2, 2);
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

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
VMrInitialize(
	void)
{
	UUtError				error;

	// install the private data and/or procs
	error =
		TMrTemplate_PrivateData_New(
			VMcTemplate_View,
			sizeof(VMtView_PrivateData),
			VMrView_ProcHandler,
			&DMgTemplate_View_PrivateData);
	UUmError_ReturnOnErrorMsg(error, "Could not install the proc handler");

	error =
		TMrTemplate_PrivateData_New(
			VMcTemplate_PartSpecification,
			sizeof(VMtPartSpec_PrivateData),
			VMiPartSpec_ProcHandler,
			&DMgTemplate_PartSpec_PrivateData);
	UUmError_ReturnOnErrorMsg(error, "Could not install the proc handler");

	error =
		TMrTemplate_PrivateData_New(
			VMcTemplate_View_Button,
			sizeof(VMtView_Button_PrivateData),
			VMrView_Button_ProcHandler,
			&DMgTemplate_Button_PrivateData);
	UUmError_ReturnOnErrorMsg(error, "Could not install the proc handler");

	error =
		TMrTemplate_PrivateData_New(
			VMcTemplate_View_CheckBox,
			sizeof(VMtView_CheckBox_PrivateData),
			VMrView_CheckBox_ProcHandler,
			&DMgTemplate_CheckBox_PrivateData);
	UUmError_ReturnOnErrorMsg(error, "Could not install the proc handler");

	error =
		TMrTemplate_PrivateData_New(
			VMcTemplate_View_EditField,
			sizeof(VMtView_EditField_PrivateData),
			VMrView_EditField_ProcHandler,
			&DMgTemplate_EditField_PrivateData);
	UUmError_ReturnOnErrorMsg(error, "Could not install the proc handler");

	error =
		TMrTemplate_PrivateData_New(
			VMcTemplate_View_ListBox,
			sizeof(VMtView_ListBox_PrivateData),
			VMrView_ListBox_ProcHandler,
			&DMgTemplate_ListBox_PrivateData);
	UUmError_ReturnOnErrorMsg(error, "Could not install the proc handler");

	error =
		TMrTemplate_PrivateData_New(
			VMcTemplate_View_RadioButton,
			sizeof(VMtView_RadioButton_PrivateData),
			VMrView_RadioButton_ProcHandler,
			&DMgTemplate_RadioButton_PrivateData);
	UUmError_ReturnOnErrorMsg(error, "Could not install the proc handler");

	error =
		TMrTemplate_PrivateData_New(
			VMcTemplate_View_RadioGroup,
			sizeof(VMtView_RadioGroup_PrivateData),
			VMrView_RadioGroup_ProcHandler,
			&DMgTemplate_RadioGroup_PrivateData);
	UUmError_ReturnOnErrorMsg(error, "Could not install the proc handler");

	error =
		TMrTemplate_PrivateData_New(
			VMcTemplate_View_Tab,
			sizeof(VMtView_Tab_PrivateData),
			VMrView_Tab_ProcHandler,
			&DMgTemplate_Tab_PrivateData);
	UUmError_ReturnOnErrorMsg(error, "Could not install the proc handler");

	error =
		TMrTemplate_PrivateData_New(
			VMcTemplate_View_Text,
			sizeof(VMtView_Text_PrivateData),
			VMrView_Text_ProcHandler,
			&DMgTemplate_Text_PrivateData);
	UUmError_ReturnOnErrorMsg(error, "Could not install the proc handler");

	error =
		TMrTemplate_PrivateData_New(
			VMcTemplate_View_Slider,
			sizeof(VMtView_Slider_PrivateData),
			VMrView_Slider_ProcHandler,
			&DMgTemplate_Slider_PrivateData);
	UUmError_ReturnOnErrorMsg(error, "Could not install the proc handler");

	error =
		TMrTemplate_PrivateData_New(
			VMcTemplate_View_Scrollbar,
			sizeof(VMtView_Scrollbar_PrivateData),
			VMrView_Scrollbar_ProcHandler,
			&DMgTemplate_Scrollbar_PrivateData);
	UUmError_ReturnOnErrorMsg(error, "Could not install the proc handler");


	return UUcError_None;
}

// ----------------------------------------------------------------------
void
VMrTerminate(
	void)
{
}
