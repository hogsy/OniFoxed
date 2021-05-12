// ======================================================================
// WM_PopupMenu.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_WindowManager.h"
#include "BFW_TextSystem.h"
#include "BFW_LocalInput.h"
#include "BFW_Timer.h"

#include "WM_PopupMenu.h"
#include "WM_Menu.h"

// ======================================================================
// defines
// ======================================================================
#define WMcPopupMenu_StickyDelay			(15)	/* in ticks */

// ======================================================================
// typedefs
// ======================================================================
typedef struct WMtPopupMenu_PrivateData
{
	UUtUns32				mouse_sticky_time;
	UUtBool					is_sticky;
	
	WMtMenu					*menu;
	UUtUns16				visible_itemID;
	
} WMtPopupMenu_PrivateData;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
WMiPopupMenu_SetMenuPosition(
	WMtPopupMenu			*inPopupMenu)
{
	WMtPopupMenu_PrivateData	*private_data;
	UUtRect						popup_menu_rect;
	IMtPoint2D					dest;
	
	// get the private data
	private_data = (WMtPopupMenu_PrivateData*)WMrWindow_GetLong(inPopupMenu, 0);
	if (private_data == NULL) { return; }
	
	// make sure there is a menu
	if (private_data->menu == NULL) { return; }
	
	// get the popup menu rect
	WMrWindow_GetRect(inPopupMenu, &popup_menu_rect);
	
	// calculate the position of the menu
	dest.x = popup_menu_rect.left;
	dest.y = popup_menu_rect.top;
	WMrMenu_Locate((WMtMenu *) private_data->menu, &dest, UUcFalse);
}

// ----------------------------------------------------------------------
static void
WMiPopupMenu_Update(
	WMtPopupMenu			*inPopupMenu)
{
	WMtPopupMenu_PrivateData	*private_data;
	PStPartSpecUI				*partspec_ui;
	UUtInt16					popup_width;
	UUtInt16					popup_height;
	UUtInt16					part_left_width;
	UUtInt16					part_top_height;
	UUtInt16					part_right_width;
	UUtInt16					part_bottom_height;
	char						title[WMcMaxTitleLength + 1];
	UUtUns32					style;
	TStFontInfo					font_info;
	
	// get the font info
	WMrWindow_GetFontInfo(inPopupMenu, &font_info);
	DCrText_SetFontInfo(&font_info);
	
	// get the private data
	private_data = (WMtPopupMenu_PrivateData*)WMrWindow_GetLong(inPopupMenu, 0);
	if (private_data == NULL) { return; }
	
	// make sure there is a menu
	if (private_data->menu == NULL) { return; }
	
	// get the active partspec ui
	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) { return; }
	
	// get the part sizes
	PSrPartSpec_GetSize(
		partspec_ui->popup_menu,
		PScPart_LeftTop,
		&part_left_width,
		&part_top_height);
	PSrPartSpec_GetSize(
		partspec_ui->popup_menu,
		PScPart_RightBottom,
		&part_right_width,
		&part_bottom_height);
	
	// get the height of the popup
	popup_height =
		part_top_height +
		part_bottom_height +
		DCrText_GetLineHeight();
	
	// get the text of the currently selected menu item
	WMrMenu_GetItemText(private_data->menu, private_data->visible_itemID, title);
	
	// set the title text
	WMrWindow_SetTitle(inPopupMenu, title, WMcMaxTitleLength);
	
	// adjust the width of the popup
	style = WMrWindow_GetStyle(inPopupMenu);
	if (((style & WMcPopupMenuStyle_NoResize) == 0) &&
		(popup_height > 0))
	{
		// get the width of the menu
		popup_width = WMrMenu_GetWidth(private_data->menu);
		
		if (popup_width > 0)
		{
			// set the size of the popup
			WMrWindow_SetSize(inPopupMenu, popup_width, popup_height);
		}
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
WMiPopupMenu_Create(
	WMtPopupMenu			*inPopupMenu)
{
	UUtError					error;
	WMtPopupMenu_PrivateData	*private_data;
	WMtMenuData					*menu_data;
	char						title[WMcMaxTitleLength + 1];
	UUtUns32					style;
	TStFontInfo					font_info;
	
	// get the private data
	private_data =
		(WMtPopupMenu_PrivateData*)UUrMemory_Block_NewClear(
			sizeof(WMtPopupMenu_PrivateData));
	if (private_data == NULL) { return WMcResult_Error; }
	WMrWindow_SetLong(inPopupMenu, 0, (UUtUns32)private_data);
	
	// initialize
	private_data->mouse_sticky_time	= 0;
	private_data->is_sticky			= UUcFalse;
	private_data->menu				= NULL;
	private_data->visible_itemID	= 0;
	
	menu_data = NULL;
	
	style = WMrWindow_GetStyle(inPopupMenu);
	if ((style & WMcPopupMenuStyle_BuildAtRuntime) == 0)
	{
		// get the menu instance name
		WMrWindow_GetTitle(inPopupMenu, title, WMcMaxTitleLength);
	
		// get the menu data
		error =
			TMrInstance_GetDataPtr(
				WMcTemplate_MenuData,
				title,
				&menu_data);
		if (error != UUcError_None) { goto cleanup; }
	}
		
	// create the data
	private_data->menu = WMrMenu_Create(menu_data, inPopupMenu);
	if (private_data->menu == NULL) { goto cleanup; }
	
	// set the font info of the menu
	WMrWindow_GetFontInfo(inPopupMenu, &font_info);
	WMrWindow_SetFontInfo(private_data->menu, &font_info);
	
	// get the item id of the first item in the list
	WMrMenu_GetItemID(private_data->menu, 0, &private_data->visible_itemID);
	
	// update the popup menu
	WMiPopupMenu_Update(inPopupMenu);
	
	return WMcResult_Handled;

cleanup:
	
	if (private_data)
	{
		UUrMemory_Block_Delete(private_data);
		private_data = NULL;
		WMrWindow_SetLong(inPopupMenu, 0, 0);
	}

	return WMcResult_Error;
}

// ----------------------------------------------------------------------
static void
WMiPopupMenu_Destroy(
	WMtPopupMenu			*inPopupMenu)
{
	WMtPopupMenu_PrivateData	*private_data;
	
	// get the private data
	private_data = (WMtPopupMenu_PrivateData*)WMrWindow_GetLong(inPopupMenu, 0);
	if (private_data == NULL) { return; }
	
	// delete the menu
	if (private_data->menu)
	{
		WMrWindow_Delete(private_data->menu);
		private_data->menu = NULL;
	}
	
	UUrMemory_Block_Delete(private_data);
	private_data = NULL;
	
	WMrWindow_SetLong(inPopupMenu, 0, 0);
}

// ----------------------------------------------------------------------
static void
WMiPopupMenu_HandleCaptureChanged(
	WMtPopupMenu			*inPopupMenu)
{
	WMtPopupMenu_PrivateData	*private_data;

	// get the private data
	private_data = (WMtPopupMenu_PrivateData*)WMrWindow_GetLong(inPopupMenu, 0);
	if (private_data == NULL) { return; }
	
	if (private_data->menu)
	{
		WMrWindow_SetVisible(private_data->menu, UUcFalse);
	}
	
	private_data->is_sticky = UUcFalse;
}

// ----------------------------------------------------------------------
static UUtUns32
WMiPopupMenu_HandleMenuCommand(
	WMtPopupMenu			*inPopupMenu,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	WMtPopupMenu_PrivateData	*private_data;
	UUtUns32					result;
	
	// get the private data
	private_data = (WMtPopupMenu_PrivateData*)WMrWindow_GetLong(inPopupMenu, 0);
	if (private_data == NULL) { return WMcResult_Error; }
	
	// hide the menu
	if (private_data->menu)
	{
		WMrWindow_SetVisible(private_data->menu, UUcFalse);
	}
	private_data->is_sticky = UUcFalse;
	
	// save the menu item selected
	private_data->visible_itemID = (UUtUns16)UUmLowWord(inParam1);
	
	// update the popup menu
	WMiPopupMenu_Update(inPopupMenu);
	
	// turn off the mouse capture
	WMrWindow_CaptureMouse(NULL);

	// forward the message to the parent
	result =
		WMrMessage_Send(
			WMrWindow_GetParent(inPopupMenu),
			WMcMessage_MenuCommand,
			inParam1,
			(UUtUns32)inPopupMenu);
	return result;
}

// ----------------------------------------------------------------------
static void
WMiPopupMenu_HandleMouseEvent(
	WMtPopupMenu			*inPopupMenu,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	WMtPopupMenu_PrivateData	*private_data;
	IMtPoint2D					global_mouse;
	IMtPoint2D					local_mouse;
	
	// disabled popup menus don't handle mouse events
	if (WMrWindow_GetEnabled(inPopupMenu) == UUcFalse) { return; }
	
	// get the private data
	private_data = (WMtPopupMenu_PrivateData*)WMrWindow_GetLong(inPopupMenu, 0);
	if (private_data == NULL) { return; }
	
	// there is no point in running the rest of the function if there is no menu
	if (private_data->menu == NULL) { return; }
	
	// get the local mouse location
	global_mouse.x = (UUtInt16)UUmHighWord(inParam1);
	global_mouse.y = (UUtInt16)UUmLowWord(inParam2);
	
	WMrWindow_GlobalToLocal(inPopupMenu, &global_mouse, &local_mouse);
	
	switch (inMessage)
	{
		case WMcMessage_MouseMove:
			if ((private_data->is_sticky) || (inParam2 & LIcMouseState_LButtonDown))
			{
				// forward the message
				WMrMessage_Send(private_data->menu, inMessage, inParam1, inParam2);
			}
		break;
		
		case WMcMessage_LMouseDown:
			WMrWindow_SetFocus(inPopupMenu);
			
			if (WMrWindow_GetVisible(private_data->menu) == UUcTrue)
			{
				// forward the message
				WMrMessage_Send(private_data->menu, inMessage, inParam1, inParam2);				
				break;
			}
			
			if (private_data->is_sticky)
			{
				// no longer sticky
				private_data->is_sticky = UUcFalse;
				
				// release the mouse (this will cause the menu to close because it
				// will send WMcMessage_CaptureChanged to the popup menu callback)
				WMrWindow_CaptureMouse(NULL);
			}
			else
			{
				// capture the mouse
				WMrWindow_CaptureMouse(inPopupMenu);
				
				// set the position of the menu
				WMiPopupMenu_SetMenuPosition(inPopupMenu);
	
				// tell the parent that the menu is about to become active
				WMrMessage_Send(
					WMrWindow_GetParent(inPopupMenu),
					WMcMessage_MenuInit,
					(UUtUns32)private_data->menu,
					UUmMakeLong(WMrWindow_GetID(private_data->menu), 0));
					
				// make the menu visible
				WMrWindow_SetVisible(private_data->menu, UUcTrue);
			}
			
			// save the time the mouse went down
			private_data->mouse_sticky_time = UUrMachineTime_Sixtieths() + WMcPopupMenu_StickyDelay;
		break;
		
		case WMcMessage_LMouseUp:
		{
			UUtUns32			time;
			
			time = UUrMachineTime_Sixtieths();
			
			// check for sticky menu
			if (private_data->mouse_sticky_time > time)
			{
				private_data->is_sticky = UUcTrue;
			}
			else
			{
				// forward the message
				WMrMessage_Send(private_data->menu, inMessage, inParam1, inParam2);
				
				// release the mouse (this will cause the menu to close because it
				// will send WMcMessage_CaptureChanged to the popup menu callback)
				WMrWindow_CaptureMouse(NULL);
			}
		}
		break;
	}
}

// ----------------------------------------------------------------------
static void
WMiPopupMenu_HandleFontInfoChanged(
	WMtPopupMenu			*inPopupMenu)
{
	WMtPopupMenu_PrivateData	*private_data;
	TStFontInfo					font_info;
	
	// get the private data
	private_data = (WMtPopupMenu_PrivateData*)WMrWindow_GetLong(inPopupMenu, 0);
	if (private_data == NULL) { return; }
	
	WMrWindow_GetFontInfo(inPopupMenu, &font_info);
	WMiPopupMenu_Update(inPopupMenu);
	
	if (private_data->menu != NULL)
	{
		WMrWindow_SetFontInfo(private_data->menu, &font_info);
	}
}

// ----------------------------------------------------------------------
static void
WMiPopupMenu_Paint(
	WMtPopupMenu			*inPopupMenu)
{
	DCtDrawContext			*draw_context;
	PStPartSpecUI			*partspec_ui;
	WMtPopupMenu_PrivateData	*private_data;
	IMtPoint2D				dest;
	UUtInt16				width;
	UUtInt16				height;
	UUtBool					enabled;
	TStFontInfo				font_info;
	
	// get the active part spec ui
	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) { return; }
	
	// get the private data
	private_data = (WMtPopupMenu_PrivateData*)WMrWindow_GetLong(inPopupMenu, 0);
	if (private_data == NULL) { return; }
	
	// get a draw context
	draw_context = DCrDraw_Begin(inPopupMenu);
	
	// set the font info
	WMrWindow_GetFontInfo(inPopupMenu, &font_info);
	DCrText_SetFontInfo(&font_info);
	
	// set the dest
	dest.x = 0;
	dest.y = 0;
	
	enabled = WMrWindow_GetEnabled(inPopupMenu);
	
	// get the width and height
	WMrWindow_GetSize(inPopupMenu, &width, &height);
	
	// draw the popup menu's popup menu
	DCrDraw_PartSpec(
		draw_context,
		partspec_ui->popup_menu,
		PScPart_All,
		&dest,
		width,
		height,
		M3cMaxAlpha);
	
	// get the size of the left column of the popupmenu partspec
	PSrPartSpec_GetSize(
		partspec_ui->popup_menu,
		PScPart_LeftTop,
		&width,
		NULL);
	
	// draw the popup menu's title
	dest.x += 2 + width;
	DCrText_SetShade(enabled ? font_info.font_shade : IMcShade_Gray50);
	DCrText_SetFormat(TSc_HLeft | TSc_VCenter);
	DCrDraw_String(
		draw_context,
		WMrWindow_GetTitlePtr(inPopupMenu),
		NULL,
		&dest);
	
	// stop drawing
	DCrDraw_End(draw_context, inPopupMenu);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
WMiPopupMenu_Callback(
	WMtWindow				*inPopupMenu,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtUns32				result;

	switch (inMessage)
	{
		case WMcMessage_NC_HitTest:
		return WMcWindowArea_Client;
	
		case WMcMessage_Create:
			result = WMiPopupMenu_Create(inPopupMenu);
		return result;
		
		case WMcMessage_Destroy:
			WMiPopupMenu_Destroy(inPopupMenu);
		return WMcResult_Handled;
		
		case WMcMessage_Paint:
			WMiPopupMenu_Paint(inPopupMenu);
		return WMcResult_Handled;
		
		case WMcMessage_MouseMove:
		case WMcMessage_LMouseDown:
		case WMcMessage_LMouseUp:
			WMiPopupMenu_HandleMouseEvent(inPopupMenu, inMessage, inParam1, inParam2);
		return WMcResult_Handled;
		
		case WMcMessage_CaptureChanged:
			WMiPopupMenu_HandleCaptureChanged(inPopupMenu);
		return WMcResult_Handled;
		
		case WMcMessage_FontInfoChanged:
			WMiPopupMenu_HandleFontInfoChanged(inPopupMenu);
		return WMcResult_Handled;
		
		case WMcMessage_MenuCommand:
			result = WMiPopupMenu_HandleMenuCommand(inPopupMenu, inParam1, inParam2);
		return result;
	}
	
	return WMrWindow_DefaultCallback(inPopupMenu, inMessage, inParam1, inParam2);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
WMrPopupMenu_AppendItem(
	WMtPopupMenu			*inPopupMenu,
	WMtMenuItemData			*inMenuItemData)
{
	WMtPopupMenu_PrivateData	*private_data;

	// get the private data
	private_data = (WMtPopupMenu_PrivateData*)WMrWindow_GetLong(inPopupMenu, 0);
	if (private_data == NULL) { return UUcError_Generic; }
	if (private_data->menu == NULL) { return UUcError_Generic; }
	
	return WMrMenu_AppendItem(private_data->menu, inMenuItemData);
}

UUtError
WMrPopupMenu_AppendItem_Light(
	WMtPopupMenu			*inPopupMenu,
	UUtUns16				inId,
	const char				*inTitle)
{
	UUtError error;
	WMtMenuItemData itemData;
	UUtUns32 max_title_length = WMcMaxTitleLength;

	itemData.flags = WMcMenuItemFlag_Enabled;
	itemData.id = inId;

	UUrString_Copy(itemData.title, inTitle, WMcMaxTitleLength);

	error = WMrPopupMenu_AppendItem(inPopupMenu, &itemData);

	return error;
}

UUtError
WMrPopupMenu_AppendItem_Divider(
	WMtPopupMenu			*inPopupMenu)
{
	UUtError error;
	WMtMenuItemData	menu_item;

	menu_item.id = -1;
	menu_item.flags = WMcMenuItemFlag_Divider;
	menu_item.title[0] = '\0';

	error = WMrPopupMenu_AppendItem(inPopupMenu, &menu_item);

	return error;
}



// ----------------------------------------------------------------------
UUtBool
WMrPopupMenu_GetItemID(
	WMtPopupMenu			*inPopupMenu,
	UUtInt16				inItemIndex,
	UUtUns16				*outID)
{
	WMtPopupMenu_PrivateData	*private_data;
	UUtBool						result;
	
	// get the private data
	private_data = (WMtPopupMenu_PrivateData*)WMrWindow_GetLong(inPopupMenu, 0);
	if (private_data == NULL) { return UUcFalse; }
	if (private_data->menu == NULL) { return UUcFalse; }
	
	// get the item id
	if (inItemIndex == -1)
	{
		*outID = private_data->visible_itemID;
		result = UUcTrue;
	}
	else
	{
		result = WMrMenu_GetItemID(private_data->menu, inItemIndex, outID);
	}

	return result;
}

// ----------------------------------------------------------------------
UUtBool
WMrPopupMenu_GetItemText(
	WMtPopupMenu			*inPopupMenu,
	UUtUns16				inItemID,
	char					*outText)
{
	WMtPopupMenu_PrivateData	*private_data;

	// get the private data
	private_data = (WMtPopupMenu_PrivateData*)WMrWindow_GetLong(inPopupMenu, 0);
	if (private_data == NULL) { return UUcFalse; }
	if (private_data->menu == NULL) { return UUcFalse; }
	
	return WMrMenu_GetItemText(private_data->menu, inItemID, outText);
}

// ----------------------------------------------------------------------
UUtError
WMrPopupMenu_Initialize(
	void)
{
	UUtError				error;
	WMtWindowClass			window_class;
	
	// register the window class
	UUrMemory_Clear(&window_class, sizeof(WMtWindowClass));
	window_class.type = WMcWindowType_PopupMenu;
	window_class.callback = WMiPopupMenu_Callback;
	window_class.private_data_size = sizeof(UUtUns32);
	
	error = WMrWindowClass_Register(&window_class);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
WMrPopupMenu_InsertItem(
	WMtPopupMenu			*inPopupMenu,
	WMtMenuItemData			*inMenuItemData,
	UUtInt16				inPosition)
{
	WMtPopupMenu_PrivateData	*private_data;

	// get the private data
	private_data = (WMtPopupMenu_PrivateData*)WMrWindow_GetLong(inPopupMenu, 0);
	if (private_data == NULL) { return UUcError_Generic; }
	if (private_data->menu == NULL) { return UUcError_Generic; }
	
	return WMrMenu_InsertItem(private_data->menu, inMenuItemData, inPosition);
}

// ----------------------------------------------------------------------
UUtError
WMrPopupMenu_RemoveItem(
	WMtPopupMenu			*inPopupMenu,
	UUtUns16				inItemID)
{
	WMtPopupMenu_PrivateData	*private_data;
	UUtError					error;

	// get the private data
	private_data = (WMtPopupMenu_PrivateData*)WMrWindow_GetLong(inPopupMenu, 0);
	if (private_data == NULL) { return UUcError_Generic; }
	if (private_data->menu == NULL) { return UUcError_Generic; }
	
	// reset the popup menu
	error = WMrMenu_RemoveItem(private_data->menu, inItemID);
	UUmError_ReturnOnError(error);
	
	// update the popup menu
	WMiPopupMenu_Update(inPopupMenu);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
WMrPopupMenu_Reset(
	WMtPopupMenu			*inPopupMenu)
{
	WMtPopupMenu_PrivateData	*private_data;
	UUtError					error;

	// get the private data
	private_data = (WMtPopupMenu_PrivateData*)WMrWindow_GetLong(inPopupMenu, 0);
	if (private_data == NULL) { return UUcError_Generic; }
	if (private_data->menu == NULL) { return UUcError_Generic; }
	
	// reset the popup menu
	error = WMrMenu_Reset(private_data->menu);
	UUmError_ReturnOnError(error);
	
	// update the popup menu
	WMiPopupMenu_Update(inPopupMenu);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
WMrPopupMenu_SetSelection(
	WMtPopupMenu			*inPopupMenu,
	UUtUns16				inItemID)
{
	WMtPopupMenu_PrivateData	*private_data;
	UUtUns16					flags;
	
	// get the private data
	private_data = (WMtPopupMenu_PrivateData*)WMrWindow_GetLong(inPopupMenu, 0);
	if (private_data == NULL) { return UUcError_Generic; }
	if (private_data->menu == NULL) { return UUcError_Generic; }
	
	// save the menu item selected
	private_data->visible_itemID = inItemID;
	
	// get the item flags
	WMrMenu_GetItemFlags(private_data->menu, inItemID, &flags);
	
	// update the popup menu
	WMiPopupMenu_Update(inPopupMenu);
	
	// tell the parent that a menu was selected
	WMrMessage_Send(
		WMrWindow_GetParent(inPopupMenu),
		WMcMessage_MenuCommand,
		UUmMakeLong(flags, inItemID),
		(UUtUns32)inPopupMenu);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
WMrPopupMenu_SelectString(
	WMtPopupMenu			*inPopupMenu,
	char					*inString)
{
	WMtPopupMenu_PrivateData	*private_data;
	UUtUns16					itemID;
	
	// get the private data
	private_data = (WMtPopupMenu_PrivateData*)WMrWindow_GetLong(inPopupMenu, 0);
	if (private_data == NULL) { return UUcError_Generic; }
	if (private_data->menu == NULL) { return UUcError_Generic; }
	
	// find the item to select
	if (WMrMenu_FindItemByText(private_data->menu, inString, &itemID) == UUcFalse) {
		itemID = (UUtUns16) -1;
	}

	return WMrPopupMenu_SetSelection(inPopupMenu, itemID);
}

// ----------------------------------------------------------------------

UUtError
WMrPopupMenu_SelectString_NoCase(
	WMtPopupMenu			*inPopupMenu,
	char					*inString)
{
	WMtPopupMenu_PrivateData	*private_data;
	UUtUns16					itemID;
	
	// get the private data
	private_data = (WMtPopupMenu_PrivateData*)WMrWindow_GetLong(inPopupMenu, 0);
	if (private_data == NULL) { return UUcError_Generic; }
	if (private_data->menu == NULL) { return UUcError_Generic; }
	
	// find the item to select
	if (WMrMenu_FindItemByText_NoCase(private_data->menu, inString, &itemID) == UUcFalse) {
		itemID = (UUtUns16) -1;
	}

	return WMrPopupMenu_SetSelection(inPopupMenu, itemID);
}
	

UUtError
WMrPopupMenu_AppendStringList(
	WMtPopupMenu			*inPopupMenu,
	UUtUns32				inCount,
	const char				**inList)
{
	UUtUns32 itr;
	UUtError error = UUcError_None;

	if (NULL == inPopupMenu) {
		return UUcError_Generic;
	}

	for (itr = 0; itr < inCount; itr++) 
	{
		error = WMrPopupMenu_AppendItem_Light(inPopupMenu, (UUtUns16) itr, inList[itr]);

		if (error != UUcError_None) {
			break;
		}

	}

	return error;
}