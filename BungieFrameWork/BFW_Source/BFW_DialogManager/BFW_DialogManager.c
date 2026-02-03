// ======================================================================
// BFW_DialogManager.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_LocalInput.h"
#include "BFW_ViewManager.h"
#include "BFW_DialogManager.h"
#include "BFW_ViewManager.h"
#include "BFW_Timer.h"
#include "Motoko_Manager.h"

#include "DM_DialogCursor.h"

#include "VM_View_Dialog.h"
#include "VM_View_TabGroup.h"

// ======================================================================
// defines
// ======================================================================
#define DMcMaxEventsPerUpdate		10
#define DMcMaxViewsOnStack			500

#define DMcDialogLayer				0.5f

#define DMcDoubleClickDelta			30		// ticks

#define DMcMaxDoubleClickDistance	3.0f

// ======================================================================
// enums
// ======================================================================
enum
{
	DMcDialog_MessageBox			= 100,

	DMcMB_Title						= 1,
	DMcMB_Message					= 2
};

// ======================================================================
// typedefs
// ======================================================================
typedef struct DMtMouseEventMap
{
	LItInputEventType			event_type;
	UUtUns16					view_flags;
	VMtMessage					view_message;

} DMtMouseEventMap;

// ======================================================================
// globals
// ======================================================================
static DMtMouseEventMap 		DMgMouseEventMap[] =
{
	{ LIcInputEvent_MouseMove,	VMcViewFlag_Enabled | VMcViewFlag_Visible,						VMcMessage_MouseMove },
	{ LIcInputEvent_LMouseDown,	VMcViewFlag_Enabled | VMcViewFlag_Visible | VMcViewFlag_Active, VMcMessage_LMouseDown },
	{ LIcInputEvent_LMouseUp,	VMcViewFlag_Enabled | VMcViewFlag_Visible | VMcViewFlag_Active, VMcMessage_LMouseUp },
	{ LIcInputEvent_MMouseDown,	VMcViewFlag_Enabled | VMcViewFlag_Visible | VMcViewFlag_Active, VMcMessage_MMouseDown },
	{ LIcInputEvent_MMouseUp,	VMcViewFlag_Enabled | VMcViewFlag_Visible | VMcViewFlag_Active, VMcMessage_MMouseUp },
	{ LIcInputEvent_RMouseDown,	VMcViewFlag_Enabled | VMcViewFlag_Visible | VMcViewFlag_Active, VMcMessage_RMouseDown },
	{ LIcInputEvent_RMouseUp,	VMcViewFlag_Enabled | VMcViewFlag_Visible | VMcViewFlag_Active, VMcMessage_RMouseUp },
	{ LIcInputEvent_None,		VMcViewFlag_None,												VMcMessage_None }
};

static char						*ONgMessageBox_Title;
static char						*ONgMessageBox_Message;
static UUtUns32					ONgMessageBox_Flags;
TMtPrivateData*					DMgTemplate_Dialog_PrivateData = NULL;
TMtPrivateData*					DMgTemplate_View_PrivateData = NULL;
TMtPrivateData*					DMgTemplate_Cursor_PrivateData = NULL;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static VMtView*
DMiDialog_FindViewToFocus(
	DMtDialog			*inDialog,
	VMtView				*inCurrentFocus)
{
	UUtUns16			i;
	UUtUns16			desired_flags;
	VMtView				*found_view;
	UUtBool				stop_on_next;

	// init the vars
	found_view		= NULL;
	stop_on_next	= UUcTrue;

	// these are the flags that are needed
	desired_flags =
		VMcViewFlag_Visible |
		VMcViewFlag_Enabled |
		VMcViewFlag_Active |
		VMcViewFlag_CanFocus;

	for (i = 0; i < inDialog->num_child_views; i++)
	{
		VMtView			*child = inDialog->child_views[i].view_ref;

		// do the child's flags match the desired flags
		if ((child->flags & desired_flags) == desired_flags)
		{
			if ((child == inCurrentFocus) && (stop_on_next == UUcFalse))
				stop_on_next = UUcTrue;
			else
			{
				found_view = child;
				break;
			}
		}
	}

	return found_view;
}

// ----------------------------------------------------------------------
static void
DMiDialog_HandleKeyEvent(
	DMtDialog					*inDialog,
	DMtDialogData_PrivateData	*inPrivateData,
	LItInputEvent				*inInputEvent)
{
	DMtMessage					message;
	UUtUns32					param1;
	UUtUns32					param2;

	// set param1 and param2
	param1 = (UUtUns32)inInputEvent->key;
	param2 = 0;

	// set the message
	switch (inInputEvent->type)
	{
		case LIcInputEvent_KeyDown:
		case LIcInputEvent_KeyRepeat:
			message = VMcMessage_KeyDown;
		break;

		case LIcInputEvent_KeyUp:
			message = VMcMessage_KeyUp;
		break;
	}

	// send a message to the focused view
	VMrView_SendMessage(inDialog, message, param1, param2);
}

// ----------------------------------------------------------------------
static void
DMiDialog_HandleMouseEvent(
	DMtDialog					*inDialog,
	DMtDialogData_PrivateData	*inPrivateData,
	LItInputEvent				*inInputEvent)
{
	VMtView						*view;
	DMtMessage					message;
	UUtUns32					param1;
	UUtUns32					param2;

	DMtMouseEventMap			*event_map;
	UUtUns16					flags;

	message = VMcMessage_None;

	// record the cursor position for the cursor drawing to happen later
	inPrivateData->cursor_position = inInputEvent->where;

	// set the flags to test the control against
	for (event_map = DMgMouseEventMap;
		 event_map->view_message != VMcMessage_None;
		 event_map++)
	{
		if (event_map->event_type == inInputEvent->type)
		{
			flags = event_map->view_flags;
			message = event_map->view_message;
			break;
		}
	}

	// set param1 and param2
	param1 = UUmMakeLong(inInputEvent->where.x, inInputEvent->where.y);
	param2 = inInputEvent->modifiers;

	// set the view to send mouse messages to
	if (inPrivateData->mouse_focus_view)
	{
		view = inPrivateData->mouse_focus_view;
	}
	else
	{
		// get the view the mouse is currently over
		view = VMrView_GetViewUnderPoint(inDialog, &inInputEvent->where, flags);
	}

	// don't send messages to nothing
	if (view == NULL) return;

	// check for double clicks
	if ((message == VMcMessage_LMouseUp) ||
		(message == VMcMessage_MMouseUp) ||
		(message == VMcMessage_RMouseUp))
	{
		UUtUns32			time;
		float				distance;

		time = UUrMachineTime_Sixtieths();
		distance =
			IMrPoint2D_Distance(
				&inPrivateData->mouse_clicked_point,
				&inInputEvent->where);

		if ((inPrivateData->mouse_clicked_view == view) &&
			(inPrivateData->mouse_clicked_type == message) &&
			(time < (inPrivateData->mouse_clicked_time + DMcDoubleClickDelta)) &&
			(distance <= DMcMaxDoubleClickDistance))
		{
			if (message == VMcMessage_LMouseUp)
				message = VMcMessage_LMouseDblClck;
			else if (message == VMcMessage_MMouseUp)
				message = VMcMessage_MMouseDblClck;
			else if (message == VMcMessage_RMouseUp)
				message = VMcMessage_RMouseDblClck;

			// clear the info
			inPrivateData->mouse_clicked_view = NULL;
			inPrivateData->mouse_clicked_time = 0;
			inPrivateData->mouse_clicked_type = VMcMessage_None;
		}
		else
		{
			// record the necessary info
			inPrivateData->mouse_clicked_view = view;
			inPrivateData->mouse_clicked_time = time;
			inPrivateData->mouse_clicked_type = message;
			inPrivateData->mouse_clicked_point = inInputEvent->where;
		}
	}

	// send a message to the view
	VMrView_SendMessage(view, message, param1, param2);

	// send a mouse up message if message is a double clicked message
	if ((message == VMcMessage_LMouseDblClck) ||
		(message == VMcMessage_MMouseDblClck) ||
		(message == VMcMessage_RMouseDblClck))
	{
		if (message == VMcMessage_LMouseDblClck)
			message = VMcMessage_LMouseUp;
		else if (message == VMcMessage_MMouseDblClck)
			message = VMcMessage_MMouseUp;
		else if (message == VMcMessage_RMouseDblClck)
			message = VMcMessage_RMouseUp;

		VMrView_SendMessage(view, message, param1, param2);
	}

	// send appropriate mouse_leave messages
	if (inPrivateData->mouse_over_view != view)
	{
		if (inPrivateData->mouse_over_view)
		{
			// tell the previous mouse_over_view that the mouse has left
			VMrView_SendMessage(
				inPrivateData->mouse_over_view,
				VMcMessage_MouseLeave,
				0,
				0);
		}

		// save the new mouse_over_view
		inPrivateData->mouse_over_view = view;
	}
}

// ----------------------------------------------------------------------
static UUtError
DMiDialog_Load(
	UUtUns16			inDialogID,
	VMtView				*inParent,
	DMtDialog			**outDialog)
{
	UUtError			error;
	UUtUns16			i;
	DMtDialogList		*dialog_list;

	UUmAssert(TMrInstance_GetTagCount(DMcTemplate_DialogList) == 1);

	*outDialog = NULL;

	// load the dialog list
	error =
		TMrInstance_GetDataPtr(
			DMcTemplate_DialogList,
			"dialoglist",
			&dialog_list);
	UUmError_ReturnOnError(error);

	for (i = 0; i < dialog_list->num_dialogs; i++)
	{
		DMtDialog			*dialog;

		dialog = (DMtDialog*)dialog_list->dialogs[i].dialog_ref;

		if (dialog->id == inDialogID)
		{
			error = VMrView_Load(dialog, inParent);
			UUmError_ReturnOnError(error);

			*outDialog = dialog;
			break;
		}
	}

	if (*outDialog == NULL)	return UUcError_Generic;

	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
DMiDialog_MessageBox_Initialize(
	DMtDialog				*inDialog)
{
	VMtView					*child;

	if ((ONgMessageBox_Flags == DMcMBFlag_None) ||
		(ONgMessageBox_Flags & DMcMBFlag_Ok) == DMcMBFlag_Ok)
	{
		child = DMrDialog_GetViewByID(inDialog, DMcMB_Ok);
		VMrView_SetVisible(child, UUcTrue);
	}

	if ((ONgMessageBox_Flags & DMcMBFlag_OkCancel) == DMcMBFlag_OkCancel)
	{
		child = DMrDialog_GetViewByID(inDialog, DMcMB_Cancel);
		VMrView_SetVisible(child, UUcTrue);
	}

	child = DMrDialog_GetViewByID(inDialog, DMcMB_Title);
	VMrView_SetValue(child, (UUtUns32)ONgMessageBox_Title);

	child = DMrDialog_GetViewByID(inDialog, DMcMB_Message);
	VMrView_SetValue(child, (UUtUns32)ONgMessageBox_Message);
}

// ----------------------------------------------------------------------
static void
DMiDialog_MessageBox_HandleCommand(
	DMtDialog				*inDialog,
	UUtUns16				inCommandType,
	UUtUns16				inViewID)
{
	if (inCommandType == VMcNotify_Click)
	{
		DMrDialog_Stop(inDialog, inViewID);
	}
}

// ----------------------------------------------------------------------
static UUtBool
DMiDialog_MessageBox_Callback(
	DMtDialog				*inDialog,
	DMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool					handled;

	handled = UUcTrue;

	switch (inMessage)
	{
		case DMcMessage_InitDialog:
			DMiDialog_MessageBox_Initialize(inDialog);
		break;

		case VMcMessage_Command:
			DMiDialog_MessageBox_HandleCommand(
				inDialog,
				(UUtUns16)UUmHighWord(inParam1),
				(UUtUns16)UUmLowWord(inParam1));
		break;

		case VMcMessage_KeyDown:
			if (inParam1 == LIcKeyCode_Return)
			{
				DMrDialog_Stop(inDialog, DMcMB_Ok);
			}
			else if (inParam1 == LIcKeyCode_Escape)
			{
				VMtView		*cancel;

				// if the cancel button is visible, then escape returns
				// cancel, otherwise it returns ok
				cancel = DMrDialog_GetViewByID(inDialog, DMcMB_Cancel);
				if (cancel->flags & VMcViewFlag_Visible)
				{
					DMrDialog_Stop(inDialog, DMcMB_Cancel);
				}
				else
				{
					DMrDialog_Stop(inDialog, DMcMB_Ok);
				}
			}
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
DMrDialog_ActivateTab(
	DMtDialog				*inDialog,
	UUtUns16				inTabID)
{
	DMtDialogData			*dialog_data;
	DMtDialogData_PrivateData	*private_data;
	VMtView					*tab_group;
	UUtUns16				i;

	UUmAssert(inDialog);

	// get a pointer to the dialog data
	dialog_data = (DMtDialogData*)inDialog->view_data;
	private_data = (DMtDialogData_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Dialog_PrivateData, dialog_data);
	UUmAssert(private_data);

	// find the tab group of the dialog
	tab_group = NULL;
	for (i = 0; i < inDialog->num_child_views; i++)
	{
		VMtView				*child = (VMtView*)inDialog->child_views[i].view_ref;

		if (child->type == VMcViewType_TabGroup)
		{
			tab_group = child;
			break;
		}
	}
	if (tab_group == NULL) return;

	// enable the desired tab
	VMrView_TabGroup_ActivateTab(tab_group, inTabID);
}

// ----------------------------------------------------------------------
void
DMrDialog_Display(
	DMtDialog				*inDialog)
{
	M3tPointScreen			dest;
	DMtDialogData			*dialog_data;
	DMtDialogData_PrivateData	*private_data;

	UUmAssert(inDialog);

	M3rDraw_State_Push();
	M3rDraw_State_SetInt(M3cDrawStateIntType_SubmitMode, M3cDrawState_SubmitMode_Normal);
	M3rDraw_State_SetInt(M3cDrawStateIntType_ZWrite, M3cDrawState_ZWrite_Off);

	// set the destination
	dest.x = (float)inDialog->location.x;
	dest.y = (float)inDialog->location.y;
	dest.z = DMcDialogLayer;
	dest.invW = 1 / dest.z;

	// draw the view
	VMrView_Draw(inDialog, &dest);

	// get a pointer to the dialog data
	dialog_data = (DMtDialogData*)inDialog->view_data;
	private_data = (DMtDialogData_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Dialog_PrivateData, dialog_data);
	UUmAssert(private_data);

	// draw the cursor
	if (dialog_data->flags & DMcDialogFlag_ShowCursor)
	{
		DCrCursor_Draw(
			private_data->cursor,
			&private_data->cursor_position,
			DMcDialogLayer);
	}

	M3rDraw_State_Pop();

	return;
}

// ----------------------------------------------------------------------
DMtDialog*
DMrDialog_GetParentDialog(
	VMtView					*inView)
{
	DMtDialog				*dialog;
	VMtView					*parent;

	UUmAssert(inView);

	// set the dialog
	dialog = NULL;

	// go up the chain of parents looking for the first dialog
	parent = VMrView_GetParent(inView);
	while (parent)
	{
		// if the parent is a dialog, stop searching
		if (parent->type == VMcViewType_Dialog)
		{
			dialog = parent;
			break;
		}

		// get the parent of the parent
		parent = VMrView_GetParent(parent);
	}

	return dialog;
}

// ----------------------------------------------------------------------
VMtView*
DMrDialog_GetViewByID(
	DMtDialog				*inDialog,
	UUtUns16				inViewID)
{
	VMtView					*found_view;
	UUtUns16				i;

	UUmAssert(inDialog);

	found_view = NULL;

	// go through the child views looking for one with this
	for (i = 0; i < inDialog->num_child_views; i++)
	{
		VMtView		*child = (VMtView*)inDialog->child_views[i].view_ref;

		if (child->id == inViewID)
		{
			found_view = child;
		}
		else
		{
			found_view = DMrDialog_GetViewByID(child, inViewID);
		}
		if (found_view) break;
	}

	return found_view;
}

// ----------------------------------------------------------------------
UUtBool
DMrDialog_IsActive(
	DMtDialog				*inDialog)
{
	DMtDialogData			*dialog_data;
	DMtDialogData_PrivateData	*private_data;

	UUmAssert(inDialog);

	// get a pointer to the dialog data
	dialog_data = (DMtDialogData*)inDialog->view_data;
	private_data = (DMtDialogData_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Dialog_PrivateData, dialog_data);
	UUmAssert(private_data);

	return !private_data->quit_dialog;
}

// ----------------------------------------------------------------------
UUtUns32
DMrDialog_MessageBox(
	DMtDialog				*inParent,
	char					*inTitle,
	char					*inMessage,
	UUtUns32				inFlags)
{
	UUtError				error;
	UUtUns32				message;

	ONgMessageBox_Title		= inTitle;
	ONgMessageBox_Message	= inMessage;
	ONgMessageBox_Flags		= inFlags;

	error =
		DMrDialog_Run(
			DMcDialog_MessageBox,
			DMiDialog_MessageBox_Callback,
			inParent,
			&message);
	if (error != UUcError_None) return 0;

	return message;
}

// ----------------------------------------------------------------------
UUtError
DMrDialog_Load(
	UUtUns16				inDialogID,
	DMtDialogCallback		inDialogCallback,
	DMtDialog				*inParent,
	DMtDialog				**outDialog)
{
	UUtError				error;
	DMtDialog				*dialog;
	DMtDialogData				*dialog_data;
	DMtDialogData_PrivateData	*private_data;
	UUtUns16				activeDrawEngine;
	UUtUns16				activeDevice;
	UUtUns16				activeMode;
	M3tDrawEngineCaps*		drawEngineCaps;
	UUtUns16				screen_width;
	UUtUns16				screen_height;
	IMtPoint2D				loc;

	UUmAssert(outDialog);

	// load the view
	error = DMiDialog_Load(inDialogID, inParent, &dialog);
	UUmError_ReturnOnErrorMsg(error, "Unable to load the dialog");

	// set the location of the dialog
	M3rManager_GetActiveDrawEngine(
		&activeDrawEngine,
		&activeDevice,
		&activeMode);

 	drawEngineCaps = M3rDrawEngine_GetCaps(activeDrawEngine);

	screen_width = drawEngineCaps->displayDevices[activeDevice].displayModes[activeMode].width;
	screen_height = drawEngineCaps->displayDevices[activeDevice].displayModes[activeMode].height;

	UUmAssert(screen_width > 0);
	UUmAssert(screen_height > 0);

	loc.x = (screen_width - dialog->width) >> 1;
	loc.y = (screen_height - dialog->height) >> 1;

	if(loc.x < 0) loc.x = 0;
	if(loc.y < 0) loc.y = 0;

	VMrView_SetLocation(dialog, &loc);

	// get the dialog data
	dialog_data = (DMtDialogData*)dialog->view_data;
	private_data = (DMtDialogData_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Dialog_PrivateData, dialog_data);
	UUmAssert(private_data);

	// initialize the private data
	private_data->callback				= inDialogCallback;
	private_data->quit_dialog			= UUcFalse;
	private_data->out_message			= 0;
	private_data->text_focus_view		= NULL;
	private_data->mouse_focus_view		= NULL;
	private_data->mouse_over_view		= NULL;
	private_data->cursor				= NULL;
	private_data->mouse_clicked_view	= NULL;
	private_data->mouse_clicked_time	= 0;
	private_data->mouse_clicked_type	= VMcMessage_None;
	private_data->prev_localinput_mode	= LIrMode_Get();

	// set the input to normal
	LIrMode_Set(LIcMode_Normal);

	// load the cursor if necessary
	if (dialog_data->flags & DMcDialogFlag_ShowCursor)
	{
		LItInputEvent	event;

		// load the cursor
		error = DCrCursor_Load(DCcCursorType_Arrow, &private_data->cursor);
		UUmError_ReturnOnErrorMsg(error, "Unable to laod the cursor");

		// set the cursor position
		LIrInputEvent_GetMouse(&event);
		private_data->cursor_position = event.where;
	}

	// tell the dialog to initialize itself
	if (inDialogCallback)
	{
		UUtBool				result;

		result = inDialogCallback(dialog, DMcMessage_InitDialog, 0, 0);

		// if the dialog returns UUcTrue, that means it set which
		// view it wants to be the focus
		if (result == UUcFalse)
		{
			DMrDialog_SetFocus(dialog, NULL);
		}
	}

	// set outDialog
	*outDialog = dialog;

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
DMrDialog_ReleaseMouseFocusView(
	DMtDialog				*inDialog,
	VMtView					*inView)
{
	DMtDialog				*dialog;

	UUmAssert(inView);

	// set dialog
	if (inDialog)
		dialog = inDialog;
	else
		dialog = DMrDialog_GetParentDialog(inView);

	// clear the mouse focus
	if (dialog)
	{
		DMtDialogData				*dialog_data;
		DMtDialogData_PrivateData	*private_data;

		// get a pointer to the dialog data
		dialog_data = (DMtDialogData*)dialog->view_data;
		private_data = (DMtDialogData_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Dialog_PrivateData, dialog_data);
		UUmAssert(private_data);

		private_data->mouse_focus_view = NULL;
	}
}

// ----------------------------------------------------------------------
UUtError
DMrDialog_Run(
	UUtUns16				inDialogID,
	DMtDialogCallback		inDialogCallback,
	VMtView					*inParent,
	UUtUns32				*outMessage)
{
	UUtError				error;
	DMtDialog				*dialog;
	DMtDialogData				*dialog_data;
	DMtDialogData_PrivateData	*private_data;

	UUmAssert(outMessage);

	*outMessage = 0;

	// load the dialog
	error =
		DMrDialog_Load(
			inDialogID,
			inDialogCallback,
			inParent,
			&dialog);
	UUmError_ReturnOnErrorMsg(error, "Unable to load the dialog");

	// get the dialog data
	dialog_data = (DMtDialogData*)dialog->view_data;
	private_data = (DMtDialogData_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Dialog_PrivateData, dialog_data);
	UUmAssert(private_data);

	// event loop
	while (!private_data->quit_dialog)
	{
		// draw the dialog
		M3rGeom_Frame_Start(0);	// XXX - Eventually pass in some real game time
		DMrDialog_Display(dialog);
		M3rGeom_Frame_End();

		// update the dialog
		DMrDialog_Update(dialog);
	}

	// set the output message
	*outMessage = private_data->out_message;

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
DMrDialog_SetFocus(
	DMtDialog				*inDialog,
	VMtView					*inView)
{
	DMtDialogData				*dialog_data;
	DMtDialogData_PrivateData	*private_data;
	VMtView						*view;

	UUmAssert(inDialog);

	// get the dialog data
	dialog_data = (DMtDialogData*)inDialog->view_data;
	private_data = (DMtDialogData_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Dialog_PrivateData, dialog_data);
	UUmAssert(private_data);

	// find the view to focus
	if (inView)
		view = inView;
	else
		view = VMrView_GetNextFocusableView(inDialog, private_data->text_focus_view);

	// don't refocus on the same view
	if (view != private_data->text_focus_view)
	{
		// set the previous view's focus to false
		if (private_data->text_focus_view)
		{
			VMrView_SetFocus(private_data->text_focus_view, UUcFalse);
		}

		// set the found view's focus to true
		if (view)
		{
			VMrView_SetFocus(view, UUcTrue);
		}

		// set the focused view
		private_data->text_focus_view = view;
	}
}

// ----------------------------------------------------------------------
void
DMrDialog_SetMouseFocusView(
	DMtDialog				*inDialog,
	VMtView					*inView)
{
	DMtDialog				*dialog;

	UUmAssert(inView);

	// set dialog
	if (inDialog)
		dialog = inDialog;
	else
		dialog = DMrDialog_GetParentDialog(inView);

	// set the mouse focus
	if (dialog)
	{
		DMtDialogData				*dialog_data;
		DMtDialogData_PrivateData	*private_data;

		// get a pointer to the dialog data
		dialog_data = (DMtDialogData*)dialog->view_data;
		private_data = (DMtDialogData_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Dialog_PrivateData, dialog_data);
		UUmAssert(private_data);

		private_data->mouse_focus_view = inView;
	}
}

// ----------------------------------------------------------------------
void
DMrDialog_Stop(
	DMtDialog				*inDialog,
	UUtUns32				inMessage)
{
	DMtDialogData				*dialog_data;
	DMtDialogData_PrivateData	*private_data;

	UUmAssert(inDialog);

	// get the dialog data
	dialog_data = (DMtDialogData*)inDialog->view_data;
	private_data = (DMtDialogData_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Dialog_PrivateData, dialog_data);
	UUmAssert(private_data);

	// set the variables
	private_data->quit_dialog = UUcTrue;
	private_data->out_message = inMessage;
}

// ----------------------------------------------------------------------
void
DMrDialog_Update(
	DMtDialog				*inDialog)
{
	UUtUns16				i;
	LItInputEvent			input_event;
	DMtDialogData				*dialog_data;
	DMtDialogData_PrivateData	*private_data;

	UUmAssert(inDialog);

	// get the dialog data
	dialog_data = (DMtDialogData*)inDialog->view_data;
	private_data = (DMtDialogData_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Dialog_PrivateData, dialog_data);
	UUmAssert(private_data);

	// update the timer
	VMrView_Timer_Update(inDialog);

	// get the events and send them to the dialog
	for (i = 0; i < DMcMaxEventsPerUpdate; i++)
	{
		UUtBool				event_avail;

		// get an event from the input context
		event_avail =
			LIrInputEvent_Get(
				&input_event);
		if (event_avail)
		{
			// handle the input event
			switch (input_event.type)
			{
				case LIcInputEvent_MouseMove:
				case LIcInputEvent_LMouseDown:
				case LIcInputEvent_LMouseUp:
				case LIcInputEvent_MMouseDown:
				case LIcInputEvent_MMouseUp:
				case LIcInputEvent_RMouseDown:
				case LIcInputEvent_RMouseUp:
					DMiDialog_HandleMouseEvent(inDialog, private_data, &input_event);
				break;

				case LIcInputEvent_KeyDown:
		 		case LIcInputEvent_KeyRepeat:
					DMiDialog_HandleKeyEvent(inDialog, private_data, &input_event);
				break;
			}
		}
		else
		{
			// no input events are available, exit input event processing
			break;
		}

		// if the dialog was closed, then exit the loop
		if (private_data->quit_dialog) break;
	}

	// reset the input context on closed dialogs
	if (private_data->quit_dialog)
	{
		VMtView_PrivateData		*view_private_data;

		// get a pointer the the view's private data
		view_private_data = (VMtView_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_View_PrivateData, inDialog);
		if (view_private_data == NULL) return;

		// clean up
		if (view_private_data->parent == NULL)
		{
			// set the input to the previous input mode
			LIrMode_Set(private_data->prev_localinput_mode);
		}
		else
		{
			// get the current position of the mouse
			LIrInputEvent_GetMouse(&input_event);

			// update the parent
			VMrView_SendMessage(
				view_private_data->parent,
				VMcMessage_Update,
				UUmMakeLong(input_event.where.x, input_event.where.y),
				0);
		}

		// unload the dialog
		VMrView_Unload(inDialog);
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
DMrInitialize(
	void)
{
	UUtError				error;

	// register the templates
	error = DMVMrRegisterTemplates();
	UUmError_ReturnOnError(error);

	// initialize the cursor
	error = DCrInitialize();
	UUmError_ReturnOnError(error);

	// initialize the view manager
	error = VMrInitialize();
	UUmError_ReturnOnError(error);

	// install the private data and/or procs
	error =
		TMrTemplate_PrivateData_New(
			DMcTemplate_DialogData,
			sizeof(DMtDialogData_PrivateData),
			DMrDialogData_ProcHandler,
			&DMgTemplate_Dialog_PrivateData);
	UUmError_ReturnOnErrorMsg(error, "Could not install dialog proc handler");

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
DMrTerminate(
	void)
{
}

