// ======================================================================
// WM_Dialog.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_LocalInput.h"
#include "BFW_WindowManager.h"
#include "BFW_WindowManager_Private.h"

#include "WM_Dialog.h"
#include "WM_RadioButton.h"

// ======================================================================
// enums
// ======================================================================
enum
{
	WMcMessageBox			= 50,
	WMcGetString			= 51,
	
	MBcTxt_Message			= 100,
	MBcBtn_1				= 101,
	MBcBtn_2				= 102,
	MBcBtn_3				= 103
	
};

// ======================================================================
// typedefs
// ======================================================================
typedef struct WMtDialog_CreationData
{
	WMtDialogData		*dialog_data;
	WMtDialogCallback	dialog_callback;
	UUtUns32			user_data;
	
} WMtDialog_CreationData;

// ----------------------------------------------------------------------
typedef struct WMtDialog_PrivateData
{
	WMtDialogCallback	dialog_callback;
	
	UUtBool				quit_dialog;
	UUtUns32			out_message;
	
	UUtUns32			default_item_id;
	
	UUtUns32			user_data;
	
} WMtDialog_PrivateData;

// ----------------------------------------------------------------------
typedef struct WMtMessageBox_Init
{
	char				*title;
	char				*message;
	WMtMessageBoxStyle	style;	
} WMtMessageBox_Init;

typedef struct WMtGetString_Private
{
	WMtDialog			*dialog;
	const char			*inTitle;
	const char			*inPrompt;
	const char			*inInitialString;
	char				*ioBuffer;
	UUtUns32			inBufferSize;
	WMtGetString_Hook	inHook;
} WMtGetString_Private;


// ======================================================================
// globals
// ======================================================================
static WMtMessageBox_Init	WMgMessageBox_Init;
static WMtDialog_ModalDrawCallback WMgModalDrawCallback = NULL;
// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
WMiDialog_Center(
	WMtDialog				*inDialog)
{
	WMtWindow				*parent;
			
	parent = WMrWindow_GetParent(inDialog);
	if (parent == NULL)
	{
		parent = WMrGetDesktop();
	}
	
	if (parent != NULL)
	{
		UUtInt16			parent_width;
		UUtInt16			parent_height;
		UUtRect				parent_rect;
		UUtInt16			dialog_width;
		UUtInt16			dialog_height;
		UUtInt16			x;
		UUtInt16			y;

		WMrWindow_GetRect(parent, &parent_rect);
		WMrWindow_GetSize(parent, &parent_width, &parent_height);
		WMrWindow_GetSize(inDialog, &dialog_width, &dialog_height);
		
		x = parent_rect.left + ((parent_width - dialog_width) >> 1);
		y = parent_rect.top + ((parent_height - dialog_height) >> 1);
		
		WMrWindow_SetLocation(inDialog, x, y);
	}
}

// ----------------------------------------------------------------------
static UUtUns32
WMiDialog_Create(
	WMtDialog				*inDialog,
	WMtDialog_CreationData	*inDialogCreationData)
{
	UUtUns32				i;
	WMtDialog_PrivateData	*private_data;
	
	// get the private data
	private_data = (WMtDialog_PrivateData*)UUrMemory_Block_NewClear(sizeof(WMtDialog_PrivateData));
	if (private_data == NULL) { goto cleanup; }
	WMrWindow_SetLong(inDialog, 0, (UUtUns32)private_data);
	
	if (inDialogCreationData == NULL) return WMcResult_Handled;
	
	private_data->dialog_callback = inDialogCreationData->dialog_callback;
	private_data->user_data = inDialogCreationData->user_data;
	
	for (i = 0; i < inDialogCreationData->dialog_data->num_items; i++)
	{
		WMtWindow			*dialog_item;
		
		dialog_item =
			WMrWindow_New(
				inDialogCreationData->dialog_data->items[i].windowtype,
				inDialogCreationData->dialog_data->items[i].title,
				inDialogCreationData->dialog_data->items[i].flags | WMcWindowFlag_Child,
				inDialogCreationData->dialog_data->items[i].style,
				inDialogCreationData->dialog_data->items[i].id,
				inDialogCreationData->dialog_data->items[i].x,
				inDialogCreationData->dialog_data->items[i].y,
				inDialogCreationData->dialog_data->items[i].width,
				inDialogCreationData->dialog_data->items[i].height,
				inDialog,
				0);
		if (dialog_item == NULL)
		{
			UUmAssert(!"Unable to create the dialog item");
			goto cleanup;
		}
		
		WMrWindow_SetFontInfo(
			dialog_item,
			&inDialogCreationData->dialog_data->items[i].font_info);
	}
	
	// set the position of the dialog
	if (inDialogCreationData->dialog_data->style & WMcDialogStyle_Centered)
	{
		WMiDialog_Center(inDialog);
	}
	
	return WMcResult_Handled;

cleanup:
	
	if (private_data)
	{
		UUrMemory_Block_Delete(private_data);
		WMrWindow_SetLong(inDialog, 0, 0);
	}
	
	return WMcResult_Error;
}

// ----------------------------------------------------------------------
static void
WMiDialog_Destroy(
	WMtDialog				*inDialog)
{
	WMtDialog_PrivateData	*private_data;
	
	// get the private data
	private_data = (WMtDialog_PrivateData*)WMrWindow_GetLong(inDialog, 0);
	if (private_data == NULL) { return; }
	
	UUrMemory_Block_Delete(private_data);
	WMrWindow_SetLong(inDialog, 0, 0);
}

// ----------------------------------------------------------------------
static UUtUns32
WMiDialog_GetDefaultID(
	WMtDialog				*inDialog)
{
	WMtDialog_PrivateData	*private_data;
	
	// get the private data
	private_data = (WMtDialog_PrivateData*)WMrWindow_GetLong(inDialog, 0);
	if (private_data == NULL) { return WMcResult_Error; }
	
	return private_data->default_item_id;
}

// ----------------------------------------------------------------------
static UUtUns32
WMiDialog_HandleCommand(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool					user_result;
	WMtDialog_PrivateData	*private_data;
	
	// get the private data
	private_data = (WMtDialog_PrivateData*)WMrWindow_GetLong(inDialog, 0);
	if (private_data == NULL) { return WMcResult_Error; }

	// let the user's dialog callback attempt to handle the command
	user_result =
		private_data->dialog_callback(
			inDialog,
			inMessage,
			inParam1,
			inParam2);
	if (user_result == UUcFalse)
	{
		if (UUmHighWord(inParam1) == WMcNotify_Click)
		{
			if (UUmLowWord(inParam1) == WMcDialogItem_OK)
			{
				WMrDialog_ModalEnd(inDialog, WMcDialogItem_OK);
			}
			else if (UUmLowWord(inParam1) == WMcDialogItem_Cancel)
			{
				WMrDialog_ModalEnd(inDialog, WMcDialogItem_Cancel);
			}
		}
	}
	
	return WMcResult_Handled;
}

// ----------------------------------------------------------------------
static void
WMiDialog_HandleKeyEvent(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtUns32				result;
	
	if (inMessage == WMcMessage_KeyUp) { return; }
	
	switch (inParam1)
	{
		case LIcKeyCode_Return:
		case LIcKeyCode_NumPadEnter:
			result = WMrMessage_Send(inDialog, WMcMessage_GetDefaultID, 0, 0);
			if (result != WMcDialogItem_None)
			{
				WMtWindow			*default_dialog_item;
				
				default_dialog_item = WMrDialog_GetItemByID(inDialog, (UUtUns16)result);
				if ((default_dialog_item) &&
					(WMrWindow_GetEnabled(default_dialog_item) == UUcTrue))
				{
					WMrMessage_Post(
						inDialog,
						WMcMessage_Command,
						UUmMakeLong(WMcNotify_Click, WMrWindow_GetID(default_dialog_item)),
						(UUtUns32)default_dialog_item);
				}
			}
		break;
		
		case LIcKeyCode_Escape:
		case LIcKeyCode_Star:
			WMrMessage_Post(inDialog, WMcMessage_Close, 0, 0);
		break;
		
/*		case LIcKeyCode_Tab:
		{
			WMtWindow				*has_focus;
			
			has_focus = WMrWindow_GetFocus();
			if (WMrWindow_IsChild(has_focus, inDialog))
			{
				if (has_focus->next)
				{
					WMrWindow_SetFocus(has_focus->next);
				}
				else
				{
					WMrWindow_SetFocus(inDialog->child);
				}
			}
		}
		break;*/
	}
}

// ----------------------------------------------------------------------
static UUtBool
WMiDialog_HandleDefault(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	WMtDialog_PrivateData	*private_data;
	UUtBool					result;
	
	result = UUcFalse;
	
	// get the private data
	private_data = (WMtDialog_PrivateData*)WMrWindow_GetLong(inDialog, 0);
	if (private_data == NULL) { return result; }
	
	if (private_data->dialog_callback)
	{
		result = private_data->dialog_callback(inDialog, inMessage, inParam1, inParam2);
	}
	
	return result;
}

// ----------------------------------------------------------------------
static void
WMiDialog_HandleResolutionChanged(
	WMtDialog				*inDialog,
	UUtInt16				inWidth,
	UUtInt16				inHeight)
{
	UUtUns32				style;
	UUtRect					rect;
	UUtInt16				new_x;
	UUtInt16				new_y;
	
	// if the window is suppose to be centered, re-center it
	style = WMrWindow_GetStyle(inDialog);
	if ((style & WMcDialogStyle_Centered) != 0)
	{
		WMiDialog_Center(inDialog);
	}
	else
	{
		// make sure the dialog isn't off screen
		WMrWindow_GetRect(inDialog, &rect);
		new_x = rect.left;
		new_y = rect.top;
		
		if (rect.left >= inWidth)
		{
			new_x = inWidth - (rect.right - rect.left);
		}
		if (rect.right <= 0)
		{
			new_y = 0;
		}
		
		if (rect.top >= inHeight)
		{
			new_y = inHeight - (rect.bottom - rect.top);
		}
		if (rect.top <= 0)
		{
			new_y = 0;
		}
		
		WMrWindow_SetLocation(inDialog, new_x, new_y);
	}
}

// ----------------------------------------------------------------------
static void
WMiDialog_SetDefaultID(
	WMtDialog				*inDialog,
	UUtUns32				inParam1)
{
	WMtDialog_PrivateData	*private_data;
	
	// get the private data
	private_data = (WMtDialog_PrivateData*)WMrWindow_GetLong(inDialog, 0);
	if (private_data == NULL) { return; }
	
	private_data->default_item_id = inParam1;
}

// ----------------------------------------------------------------------
static void
WMiDialog_TranslateModalEvent(
	WMtDialog				*inDialog,
	WMtEvent				*ioEvent)
{
	// the event is already going to the dialog
	if (ioEvent->window == inDialog) { return; }
	
	// if the event is going to a child window, don't interfere
	if (WMrWindow_IsChild(ioEvent->window, inDialog)) { return; }
	
	// make any mouse events go to the dialog
	switch (ioEvent->message)
	{
		case WMcMessage_NC_LMouseDown:
		case WMcMessage_NC_LMouseUp:
		case WMcMessage_NC_LMouseDblClck:
		case WMcMessage_NC_MMouseDown:
		case WMcMessage_NC_MMouseUp:
		case WMcMessage_NC_MMouseDblClck:
		case WMcMessage_NC_RMouseDown:
		case WMcMessage_NC_RMouseUp:
		case WMcMessage_NC_RMouseDblClck:
		case WMcMessage_MouseMove:
		case WMcMessage_LMouseDown:
		case WMcMessage_LMouseUp:
		case WMcMessage_LMouseDblClck:
		case WMcMessage_MMouseDown:
		case WMcMessage_MMouseUp:
		case WMcMessage_MMouseDblClck:
		case WMcMessage_RMouseDown:
		case WMcMessage_RMouseUp:
		case WMcMessage_RMouseDblClck:
			ioEvent->window = NULL;
		break;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
WMiDialog_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtUns32				result;
	
	switch (inMessage)
	{
		case WMcMessage_NC_Create:
		case WMcMessage_NC_Destroy:
		case WMcMessage_NC_HitTest:
		case WMcMessage_NC_Paint:
		case WMcMessage_NC_MouseMove:
		case WMcMessage_NC_LMouseDown:
		case WMcMessage_NC_LMouseUp:
		case WMcMessage_NC_LMouseDblClck:
		case WMcMessage_NC_MMouseDown:
		case WMcMessage_NC_MMouseUp:
		case WMcMessage_NC_MMouseDblClck:
		case WMcMessage_NC_RMouseDown:
		case WMcMessage_NC_RMouseUp:
		case WMcMessage_NC_RMouseDblClck:
		case WMcMessage_NC_Activate:
		case WMcMessage_NC_Update:
		case WMcMessage_NC_CalcClientSize:
		break;
		
		case WMcMessage_KeyDown:
		case WMcMessage_KeyUp:
			WMiDialog_HandleKeyEvent(inDialog, inMessage, inParam1, inParam2);
		return WMcResult_Handled;
		
		case WMcMessage_Close:
			WMrMessage_Send(
				inDialog,
				WMcMessage_Command,
				UUmMakeLong(WMcNotify_Click, WMcDialogItem_Cancel),
				(UUtUns32)NULL);
		return WMcResult_Handled;
		
		case WMcMessage_Create:
			result = WMiDialog_Create(inDialog, (WMtDialog_CreationData*)inParam1);
			WMmResult_ReturnOnError(result);
		return WMcResult_Handled;
		
		case WMcMessage_Destroy:
			WMiDialog_HandleDefault(inDialog, inMessage, inParam1, inParam2);
			WMiDialog_Destroy(inDialog);
		return WMcResult_Handled;
		
		case WMcMessage_Command:
			result = WMiDialog_HandleCommand(inDialog, inMessage, inParam1, inParam2);
			WMmResult_ReturnOnError(result);
		return WMcResult_Handled;
		
		case WMcMessage_CompareItems:
			result = WMiDialog_HandleDefault(inDialog, inMessage, inParam1, inParam2);
			return result;
		break;
		
		case WMcMessage_GetDefaultID:
			result = WMiDialog_GetDefaultID(inDialog);
			return result;
		break;
		
		case WMcMessage_SetDefaultID:
			WMiDialog_SetDefaultID(inDialog, inParam1);
		return WMcResult_Handled;
		
		case WMcMessage_ResolutionChanged:
			WMiDialog_HandleResolutionChanged(
				inDialog,
				(UUtInt16)UUmHighWord(inParam1),
				(UUtInt16)UUmLowWord(inParam1));
		return WMcResult_Handled;
		
		default:
			result = WMiDialog_HandleDefault(inDialog, inMessage, inParam1, inParam2);
			if (result == UUcTrue) { return WMcResult_Handled; }
		break;
	}
	
	return WMrWindow_DefaultCallback(inDialog, inMessage, inParam1, inParam2);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
WMiMessageBox_InitDialog(
	WMtDialog				*inDialog)
{
	WMtWindow				*message;
	WMtWindow				*btn_1;
	WMtWindow				*btn_2;
	WMtWindow				*btn_3;
	
	// save the dialog's style
	WMrDialog_SetUserData(inDialog, WMgMessageBox_Init.style);

	// set the title of the dialog
	WMrWindow_SetTitle(inDialog, WMgMessageBox_Init.title, WMcMaxTitleLength);
	
	// get pointers to the child windows
	message = WMrDialog_GetItemByID(inDialog, MBcTxt_Message);
	btn_1 = WMrDialog_GetItemByID(inDialog, MBcBtn_1);
	btn_2 = WMrDialog_GetItemByID(inDialog, MBcBtn_2);
	btn_3 = WMrDialog_GetItemByID(inDialog, MBcBtn_3);
	
	// set up the child windows
	WMrWindow_SetTitle(message, WMgMessageBox_Init.message, WMcMaxTitleLength);
	WMrWindow_SetVisible(btn_2, UUcFalse);
	WMrWindow_SetVisible(btn_3, UUcFalse);

	// set the text style
	switch (WMgMessageBox_Init.style)
	{
		case WMcMessageBoxStyle_OK_Cancel:
			WMrWindow_SetTitle(btn_2, "OK", WMcMaxTitleLength);
			WMrWindow_SetVisible(btn_2, UUcTrue);

			WMrWindow_SetTitle(btn_1, "Cancel", WMcMaxTitleLength);
			WMrWindow_SetVisible(btn_1, UUcTrue);
		break;
			
		case WMcMessageBoxStyle_OK:
			WMrWindow_SetTitle(btn_1, "OK", WMcMaxTitleLength);
			WMrWindow_SetVisible(btn_1, UUcTrue);
		break;
		
		case WMcMessageBoxStyle_Yes_No_Cancel:
			WMrWindow_SetTitle(btn_3, "Yes", WMcMaxTitleLength);
			WMrWindow_SetVisible(btn_3, UUcTrue);

			WMrWindow_SetTitle(btn_2, "No", WMcMaxTitleLength);
			WMrWindow_SetVisible(btn_2, UUcTrue);

			WMrWindow_SetTitle(btn_1, "Cancel", WMcMaxTitleLength);
			WMrWindow_SetVisible(btn_1, UUcTrue);
		break;
			
		case WMcMessageBoxStyle_Yes_No:
			WMrWindow_SetTitle(btn_2, "Yes", WMcMaxTitleLength);
			WMrWindow_SetVisible(btn_2, UUcTrue);

			WMrWindow_SetTitle(btn_1, "No", WMcMaxTitleLength);
			WMrWindow_SetVisible(btn_1, UUcTrue);
		break;
	}
}

// ----------------------------------------------------------------------
static void
WMiMessageBox_HandleCommand(
	WMtDialog				*inDialog,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtUns32				style;
	
	style = WMrDialog_GetUserData(inDialog);
	
	switch (style)
	{
		case WMcMessageBoxStyle_OK:
			switch (UUmLowWord(inParam1))
			{
				case MBcBtn_1:
					WMrDialog_ModalEnd(inDialog, WMcDialogItem_OK);
				break;

				case MBcBtn_2:
				case MBcBtn_3:
					UUmAssert(!"BLAM");
				break;
			}
		break;

		case WMcMessageBoxStyle_OK_Cancel:
			switch (UUmLowWord(inParam1))
			{
				case MBcBtn_2:
					WMrDialog_ModalEnd(inDialog, WMcDialogItem_OK);
				break;
				
				case MBcBtn_1:
					WMrDialog_ModalEnd(inDialog, WMcDialogItem_Cancel);
				break;
				
				case MBcBtn_3:
					UUmAssert(!"BLAM");
				break;
			}
		break;
		
		case WMcMessageBoxStyle_Yes_No:
			switch (UUmLowWord(inParam1))
			{
				case MBcBtn_2:
					WMrDialog_ModalEnd(inDialog, WMcDialogItem_Yes);
				break;
				
				case MBcBtn_1:
					WMrDialog_ModalEnd(inDialog, WMcDialogItem_No);
				break;
				
				case MBcBtn_3:
					UUmAssert(!"BLAM");
				break;
			}
		break;

		case WMcMessageBoxStyle_Yes_No_Cancel:
			switch (UUmLowWord(inParam1))
			{
				case MBcBtn_3:
					WMrDialog_ModalEnd(inDialog, WMcDialogItem_Yes);
				break;
				
				case MBcBtn_2:
					WMrDialog_ModalEnd(inDialog, WMcDialogItem_No);
				break;
				
				case MBcBtn_1:
					WMrDialog_ModalEnd(inDialog, WMcDialogItem_Cancel);
				break;
			}
		break;
	}	
}

// ----------------------------------------------------------------------
static UUtBool
WMiMessageBox_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool					handled;
	
	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			WMiMessageBox_InitDialog(inDialog);
		break;
		
		case WMcMessage_Command:
			WMiMessageBox_HandleCommand(inDialog, inParam1, inParam2);
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}


static UUtBool WMiGetString_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool					handled;
	WMtGetString_Private	*privateData = (WMtGetString_Private *) WMrDialog_GetUserData(inDialog);
	WMtWindow				*prompt = WMrDialog_GetItemByID(inDialog, 0);
	WMtWindow				*edit_field = WMrDialog_GetItemByID(inDialog, 100);

	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			UUrString_Copy(privateData->ioBuffer, privateData->inInitialString, privateData->inBufferSize);
			WMrWindow_SetTitle(inDialog, privateData->inTitle, WMcMaxTitleLength);

			if (NULL == privateData->inPrompt) {
				WMrWindow_SetVisible(prompt, UUcFalse);
			}
			else {
				WMrWindow_SetTitle(prompt, privateData->inPrompt, WMcMaxTitleLength);
				WMrWindow_SetVisible(prompt, UUcTrue);
			}
			
			WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32) privateData->ioBuffer, 0);
			WMrMessage_Send(edit_field, EFcMessage_SetMaxChars, privateData->inBufferSize, 0);
		break;
		
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrDialog_ModalEnd(inDialog, (UUtUns32) NULL);
				break;

				case WMcDialogItem_OK:
					WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) privateData->ioBuffer, privateData->inBufferSize);

					if ((NULL == privateData->inHook) || (privateData->inHook(inDialog, privateData->ioBuffer))) {
						WMrDialog_ModalEnd(inDialog, (UUtUns32) privateData->ioBuffer);
					}
				break;
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
UUtError
WMrDialog_Create(
	WMtDialogID				inDialogID,
	WMtWindow				*inParent,
	WMtDialogCallback		inDialogCallback,
	UUtUns32				inUserData,
	WMtDialog				**outDialog)
{
	UUtError				error;
	WMtDialogData			*dialog_data[1024];
	WMtDialog				*dialog;
	UUtUns32				num_dialog_data;
	UUtUns32				i;
	WMtDialog_CreationData	creation_data;
	
	// create the dialog
	dialog = NULL;
	
	// get a list of dialogs
	error =
		TMrInstance_GetDataPtr_List(
			WMcTemplate_DialogData,
			1024,
			&num_dialog_data,
			dialog_data);
	UUmError_ReturnOnError(error);
	
	// find the desired dialog
	for (i = 0; i < num_dialog_data; i++)
	{
		if (dialog_data[i]->id == inDialogID)
		{
			break;
		}
	}
	
	if (i == num_dialog_data)
	{
		UUmAssert(!"dialog not found");
		return UUcError_Generic;
	}
	
	// create the dialog
	creation_data.dialog_data = dialog_data[i];
	creation_data.dialog_callback = inDialogCallback;
	creation_data.user_data = inUserData;
	
	dialog =
		WMrWindow_New(
			WMcWindowType_Dialog,
			dialog_data[i]->title,
			dialog_data[i]->flags,
			dialog_data[i]->style,
			dialog_data[i]->id,
			dialog_data[i]->x,
			dialog_data[i]->y,
			dialog_data[i]->width,
			dialog_data[i]->height,
			inParent,
			(UUtUns32)&creation_data);
	if (dialog == NULL)
	{
		UUmAssert(!"unable to create the dialog");
		return UUcError_Generic;
	}
	
	// tell the dialog callback that the dialog has been created
	inDialogCallback(
		dialog,
		WMcMessage_InitDialog,
		0,
		0);
	
	*outDialog = dialog;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
WMtWindow*
WMrDialog_GetItemByID(
	WMtDialog				*inDialog,
	UUtUns16				inID)
{
	WMtWindow				*child;
	
	child = inDialog->child;
	while (child)
	{
		if (child->id == inID)
		{
			break;
		}
		
		child = child->next;
	}
	
	return child;
}

// ----------------------------------------------------------------------
UUtUns32
WMrDialog_GetUserData(
	WMtDialog				*inDialog)
{
	WMtDialog_PrivateData	*private_data;
	
	UUmAssert(inDialog);
	
	// get the private data
	private_data = (WMtDialog_PrivateData*)WMrWindow_GetLong(inDialog, 0);
	UUmAssert(private_data);
	
	return private_data->user_data;
}

// ----------------------------------------------------------------------
UUtUns32
WMrDialog_MessageBox(
	WMtWindow				*inParent,
	char					*inTitle,
	char					*inMessage,
	WMtMessageBoxStyle		inStyle)
{
	UUtError				error;
	UUtUns32				message;
	
	// set the initialization parameters
	WMgMessageBox_Init.title = inTitle;
	WMgMessageBox_Init.message = inMessage;
	WMgMessageBox_Init.style = inStyle;
	
	// run the message box
	error =
		WMrDialog_ModalBegin(
			WMcMessageBox,
			inParent,
			WMiMessageBox_Callback,
			(UUtUns32) -1,
			&message);
	
	return message;
}

char *WMrDialog_GetString(
	WMtWindow				*inParent,
	const char				*inTitle,
	const char				*inPrompt,
	const char				*inInitialString,
	char					*ioBuffer,
	UUtUns32				inBufferSize,
	WMtGetString_Hook		inHook)
{
	UUtError error;
	UUtUns32 message;
	char *result;

	WMtGetString_Private init;

	init.inTitle = inTitle;
	init.inPrompt = inPrompt;
	init.inInitialString = inInitialString;
	init.ioBuffer = ioBuffer;
	init.inHook = inHook;
	init.inBufferSize = inBufferSize;

	error = WMrDialog_ModalBegin(WMcGetString, inParent, WMiGetString_Callback, (UUtUns32) &init, &message);

	if (error != UUcError_None) {
		result = NULL;
	}
	else {
		result = (char *) message;
	}

	return result;
}


// ----------------------------------------------------------------------
UUtError
WMrDialog_ModalBegin(
	WMtDialogID				inDialogID,
	WMtWindow				*inParent,
	WMtDialogCallback		inDialogCallback,
	UUtUns32				inUserData,
	UUtUns32				*outMessage)
{
	UUtError				error;
	WMtDialog				*dialog;
	WMtDialog_PrivateData	*private_data;
	
	// load the dialog
	error =
		WMrDialog_Create(
			inDialogID,
			inParent,
			inDialogCallback,
			inUserData,
			&dialog);
	UUmError_ReturnOnError(error);
	
	// get the private data
	private_data = (WMtDialog_PrivateData*)WMrWindow_GetLong(dialog, 0);
	if (private_data == NULL) { return UUcError_Generic; }
	
	while (!private_data->quit_dialog)
	{
		WMtEvent			event;
		
		// setup state information so frame start will be happy
		M3rDraw_State_SetInt(M3cDrawStateIntType_ZWrite, M3cDrawState_ZWrite_On);
		M3rDraw_State_SetInt(M3cDrawStateIntType_ZCompare,	M3cDrawState_ZCompare_On);
		M3rDraw_State_SetInt(M3cDrawStateIntType_BufferClear, M3cDrawState_BufferClear_On);
		M3rDraw_State_SetInt(M3cDrawStateIntType_DoubleBuffer, M3cDrawState_DoubleBuffer_On);
		M3rDraw_State_SetInt(M3cDrawStateIntType_ClearColor, 0x00000000);
		M3rDraw_State_Commit();

		M3rGeom_Frame_Start(0);

		if (WMgModalDrawCallback != NULL) {
			WMgModalDrawCallback();
		}

		WMrDisplay();
		M3rGeom_Frame_End();
		
		// update the window manager
		WMrUpdate();
		
		while (WMrMessage_Get(&event))
		{
			WMiDialog_TranslateModalEvent(dialog, &event);
			WMrMessage_Dispatch(&event);
		}

		// most likely error here is you are calling 
		// WMrWindow_Delete instead of WMrDialog_ModalEnd
		UUrMemory_Block_Verify(private_data);
	}
	
	// set the output message
	if (outMessage)
	{
		*outMessage = private_data->out_message;
	}
	
	// close the dialog
	WMrWindow_Delete(dialog);
	dialog = NULL;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
WMrDialog_ModalEnd(
	WMtDialog				*inDialog,
	UUtUns32				inOutMessage)
{
	WMtDialog_PrivateData	*private_data;
	
	UUmAssert(inDialog);
	
	// get the private data
	private_data = (WMtDialog_PrivateData*)WMrWindow_GetLong(inDialog, 0);
	UUmAssert(private_data);
	
	private_data->out_message = inOutMessage;
	private_data->quit_dialog = UUcTrue;
}

// ----------------------------------------------------------------------
void
WMrDialog_RadioButtonCheck(
	WMtDialog				*inDialog,
	UUtUns16				inFirstRadioButtonID,
	UUtUns16				inLastRadioButtonID,
	UUtUns16				inCheckRadioButtonID)
{
	UUtUns16				i;
	WMtRadioButton			*radiobutton;
	
	for (i = inFirstRadioButtonID; i <= inLastRadioButtonID; i++)
	{
		radiobutton = WMrDialog_GetItemByID(inDialog, i);
		if (radiobutton == NULL) continue;
		
		WMrMessage_Send(
			radiobutton,
			RBcMessage_SetCheck,
			(UUtUns32)UUcFalse,
			0);
	}
	
	radiobutton = WMrDialog_GetItemByID(inDialog, inCheckRadioButtonID);
	if (radiobutton != NULL)
	{
		WMrMessage_Send(
			radiobutton,
			RBcMessage_SetCheck,
			(UUtUns32)UUcTrue,
			0);
	}
}

// ----------------------------------------------------------------------
void
WMrDialog_ToggleButtonCheck(
	WMtDialog				*inDialog,
	UUtUns16				inFirstToggleButtonID,
	UUtUns16				inLastToggleButtonID,
	UUtUns16				inSetToggleButtonID)
{
	UUtUns16				i;
	WMtWindow				*togglebutton;
	
	for (i = inFirstToggleButtonID; i <= inLastToggleButtonID; i++)
	{
		togglebutton = WMrDialog_GetItemByID(inDialog, i);
		if (togglebutton == NULL) continue;
		
		WMrMessage_Send(
			togglebutton,
			TBcMessage_SetToggle,
			(UUtUns32)UUcFalse,
			0);
	}
	
	togglebutton = WMrDialog_GetItemByID(inDialog, inSetToggleButtonID);
	if (togglebutton != NULL)
	{
		WMrMessage_Send(
			togglebutton,
			TBcMessage_SetToggle,
			(UUtUns32)UUcTrue,
			0);
	}
}

// ----------------------------------------------------------------------
void
WMrDialog_SetUserData(
	WMtDialog				*inDialog,
	UUtUns32				inUserData)
{
	WMtDialog_PrivateData	*private_data;
	
	UUmAssert(inDialog);
	
	// get the private data
	private_data = (WMtDialog_PrivateData*)WMrWindow_GetLong(inDialog, 0);
	UUmAssert(private_data);
	
	private_data->user_data = inUserData;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
WMrDialog_Initialize(
	void)
{
	UUtError				error;
	WMtWindowClass			window_class;
	
	// register the window class
	UUrMemory_Clear(&window_class, sizeof(WMtWindowClass));
	window_class.type = WMcWindowType_Dialog;
	window_class.callback = WMiDialog_Callback;
	window_class.private_data_size = sizeof(WMtDialog_PrivateData*);
	
	error = WMrWindowClass_Register(&window_class);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
WMrDialog_RegisterTemplates(
	void)
{
	UUtError				error;
	
	error =
		TMrTemplate_Register(
			WMcTemplate_DialogData,
			sizeof(WMtDialogData),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
void WMrDialog_SetDrawCallback(WMtDialog_ModalDrawCallback inModalDrawCallback)
{
	WMgModalDrawCallback = inModalDrawCallback;

	return;
}
