// ======================================================================
// WM_MenuBar.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_WindowManager.h"
#include "BFW_TextSystem.h"
#include "BFW_LocalInput.h"
#include "BFW_Timer.h"

#include "WM_MenuBar.h"
#include "WM_DrawContext.h"
#include "WM_PartSpecification.h"
#include "WM_Utilities.h"


// ======================================================================
// typedefs
// ======================================================================
#define WMcMenuBar_MaxNumMenus			32

#define WMcMenuBar_Unused_Left			5
#define WMcMenuBar_Unused_Right			5

#define WMcMenuBar_Spacing_Left			5
#define WMcMenuBar_Spacing_Right		5
#define WMcMenuBar_Spacing				(WMcMenuBar_Spacing_Left + WMcMenuBar_Spacing_Right)

#define WMcMenuBar_StickyDelay			(15)	/* in ticks */

// ======================================================================
// typedefs
// ======================================================================
typedef struct WMtMenuBarItem
{
	WMtMenu					*menu;

	// menu title data
	IMtPoint2D				location;
	UUtUns16				width;		// text width
	UUtUns16				height;		// text height

} WMtMenuBarItem;


typedef struct WMtMenuBar_PrivateData
{
	UUtUns32				mouse_sticky_time;
	UUtInt32				hilited_menu;
	UUtBool					is_sticky;
	UUtBool					active;

	UUtInt32				num_menubar_items;
	WMtMenuBarItem			menubar_items[WMcMenuBar_MaxNumMenus];

} WMtMenuBar_PrivateData;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
WMiMenuBar_GetMenuUnderPoint(
	WMtMenuBar				*inMenuBar,
	WMtMenuBar_PrivateData	*inPrivateData,
	IMtPoint2D				*inPoint)
{
	UUtInt32				i;
	UUtInt16				menubar_height;

	WMrWindow_GetSize(inMenuBar, NULL, &menubar_height);

	for (i = 0; i < inPrivateData->num_menubar_items; i++)
	{
		UUtRect				bounds;

		// setup the bounds
		bounds.left = inPrivateData->menubar_items[i].location.x;
		bounds.top = 0;
		bounds.right = bounds.left + inPrivateData->menubar_items[i].width + WMcMenuBar_Spacing;
		bounds.bottom = menubar_height;

		// check the point
		if (IMrRect_PointIn(&bounds, inPoint))
		{
			return i;
		}
	}

	return WMcMenuBar_MaxNumMenus;
}

// ----------------------------------------------------------------------
static void
WMiMenuBar_RecalcMenuBarItemPositions(
	WMtMenuBar				*inMenuBar,
	WMtMenuBar_PrivateData	*inPrivateData)
{
	UUtInt32				i;
	IMtPoint2D				title_location;

	// make sure that there are menubar items and a text context
	if (inPrivateData->num_menubar_items == 0)
	{
		return;
	}

	// set the start location for the menu titles
	title_location.x = WMcMenuBar_Unused_Left;
	title_location.y = 0;

	// set the location of the menus
	for (i = 0; i < inPrivateData->num_menubar_items; i++)
	{
		UUtRect				bounds;

		// move to the location of the title
		title_location.x += WMcMenuBar_Spacing_Left;

		// set the location of the menu's title
		inPrivateData->menubar_items[i].location = title_location;

		// get the string bounding the title
		DCrText_GetStringRect(
			WMrWindow_GetTitlePtr(inPrivateData->menubar_items[i].menu),
			&bounds);

		// add in the width of the title string
		title_location.x += bounds.right;

		// move to the location of the next menu title
		title_location.x += WMcMenuBar_Spacing_Right;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
WMiMenuBar_Create(
	WMtMenuBar				*inMenuBar,
	WMtMenuBarData			*inMenuBarData)
{
	UUtError				error;
	UUtUns32				i;
	UUtInt16				height;
	PStPartSpecUI			*partspec_ui;
	UUtInt16				top_height;
	UUtInt16				bottom_height;
	UUtInt16				menubar_width;
	UUtInt16				menubar_height;
	WMtMenuBar_PrivateData	*private_data;
	TStFontInfo				font_info;

	// get the private data
	private_data = (WMtMenuBar_PrivateData*)UUrMemory_Block_NewClear(sizeof(WMtMenuBar_PrivateData));
	if (private_data == NULL) { return WMcResult_Error; }
	WMrWindow_SetLong(inMenuBar, 0, (UUtUns32)private_data);

	if (inMenuBarData->num_menus > WMcMenuBar_MaxNumMenus)
	{
		return UUcError_Generic;
	}

	// get the height of the background's borders
	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) { goto cleanup; }

	PSrPartSpec_GetSize(partspec_ui->background, PScPart_LeftTop, NULL, &top_height);
	PSrPartSpec_GetSize(partspec_ui->background, PScPart_RightBottom, NULL, &bottom_height);

	WMrWindow_GetFontInfo(inMenuBar, &font_info);
	DCrText_SetFontInfo(&font_info);

	// set the height of the menubar
	height =
		top_height +
		bottom_height +
		DCrText_GetLineHeight();

	WMrWindow_GetSize(inMenuBar, &menubar_width, &menubar_height);

	if ((UUtUns16)menubar_height < height)
	{
		WMrWindow_SetSize(inMenuBar, menubar_width, height);
	}

	// create the menu windows
	private_data->num_menubar_items = 0;
	for (i = 0; i < inMenuBarData->num_menus; i++)
	{
		WMtMenu				*menu;

		menu = WMrMenu_Create(inMenuBarData->menus[i], inMenuBar);
		if (menu == NULL) { goto cleanup; }

		error = WMrMenuBar_AppendMenu(inMenuBar, menu);
		if (error != UUcError_None) { goto cleanup; }
	}

	private_data->hilited_menu = WMcMenuBar_MaxNumMenus;
	private_data->is_sticky = UUcFalse;
	private_data->active = UUcFalse;

	return WMcResult_Handled;

cleanup:
	UUmAssert(0);

	if (private_data)
	{
		UUrMemory_Block_Delete(private_data);
	}

	return WMcResult_Error;
}

// ----------------------------------------------------------------------
static void
WMiMenuBar_Destroy(
	WMtMenuBar				*inMenuBar)
{
	WMtMenuBar_PrivateData	*private_data;

	// get the private data
	private_data = (WMtMenuBar_PrivateData*)WMrWindow_GetLong(inMenuBar, 0);
	if (private_data == NULL) { return; }

	UUrMemory_Block_Delete(private_data);
	WMrWindow_SetLong(inMenuBar, 0, 0);
}

// ----------------------------------------------------------------------
static void
WMiMenuBar_HiliteMenu(
	WMtMenuBar				*inMenuBar,
	IMtPoint2D				*inLocalMouse)
{
	UUtUns32				menu_index;
	WMtMenuBar_PrivateData	*private_data;

	// get the private data
	private_data = (WMtMenuBar_PrivateData*)WMrWindow_GetLong(inMenuBar, 0);
	if (private_data == NULL) { return; }

	menu_index = WMiMenuBar_GetMenuUnderPoint(inMenuBar, private_data, inLocalMouse);
	if (menu_index != WMcMenuBar_MaxNumMenus)
	{
		UUtRect				window_rect;
		IMtPoint2D			menu_location;

		// set the location of the menu
		WMrWindow_GetRect(inMenuBar, &window_rect);
		WMrWindow_LocalToGlobal(
			inMenuBar,
			&private_data->menubar_items[menu_index].location,
			&menu_location);
		menu_location.y = window_rect.bottom - 1;

		WMrMenu_Locate((WMtMenu *) private_data->menubar_items[menu_index].menu, &menu_location, UUcTrue);

		// tell the parent that the menu is about to become active
		WMrMessage_Send(
			WMrWindow_GetParent(inMenuBar),
			WMcMessage_MenuInit,
			(UUtUns32)private_data->menubar_items[menu_index].menu,
			UUmMakeLong(WMrWindow_GetID(private_data->menubar_items[menu_index].menu), menu_index));

		// make the menu visible
		WMrWindow_SetVisible(private_data->menubar_items[menu_index].menu, UUcTrue);
		private_data->hilited_menu = menu_index;
	}
}

// ----------------------------------------------------------------------
static void
WMiMenuBar_UnhiliteMenu(
	WMtMenuBar				*inMenuBar,
	UUtBool					inIsSticky)
{
	WMtMenuBar_PrivateData	*private_data;

	// get the private data
	private_data = (WMtMenuBar_PrivateData*)WMrWindow_GetLong(inMenuBar, 0);
	if (private_data == NULL) { return; }

	if (private_data->hilited_menu != WMcMenuBar_MaxNumMenus)
	{
		WMrWindow_SetVisible(
			private_data->menubar_items[private_data->hilited_menu].menu,
			UUcFalse);
		private_data->hilited_menu = WMcMenuBar_MaxNumMenus;
	}

	private_data->is_sticky = inIsSticky;
}

// ----------------------------------------------------------------------
static void
WMiMenuBar_ForwardMessage(
	WMtMenuBar				*inMenuBar,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	WMtMenuBar_PrivateData	*private_data;

	// get the private data
	private_data = (WMtMenuBar_PrivateData*)WMrWindow_GetLong(inMenuBar, 0);
	if (private_data == NULL) { return; }

	if (private_data->hilited_menu != WMcMenuBar_MaxNumMenus)
	{
		WMrMessage_Send(
			private_data->menubar_items[private_data->hilited_menu].menu,
			inMessage,
			inParam1,
			inParam2);
	}
}


// ----------------------------------------------------------------------
static void
WMiMenuBar_HandleCaptureChanged(
	WMtMenuBar				*inMenuBar)
{
	WMtMenuBar_PrivateData	*private_data;

	// get the private data
	private_data = (WMtMenuBar_PrivateData*)WMrWindow_GetLong(inMenuBar, 0);
	if (private_data == NULL) { return; }

	// turn off the previously hilited menu
	WMiMenuBar_UnhiliteMenu(inMenuBar, UUcFalse);

	private_data->active = UUcFalse;
}

// ----------------------------------------------------------------------
static void
WMiMenuBar_HandleMouseEvent(
	WMtMenuBar				*inMenuBar,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	IMtPoint2D				global_mouse;
	IMtPoint2D				local_mouse;
	WMtMenuBar_PrivateData	*private_data;

	// get the private data
	private_data = (WMtMenuBar_PrivateData*)WMrWindow_GetLong(inMenuBar, 0);
	if (private_data == NULL) { return; }

	// get the global mouse location
	global_mouse.x = (UUtInt16)UUmHighWord(inParam1);
	global_mouse.y = (UUtInt16)UUmLowWord(inParam1);

	WMrWindow_GlobalToLocal(inMenuBar, &global_mouse, &local_mouse);

	switch (inMessage)
	{
		case WMcMessage_MouseMove:
			if ((private_data->active == UUcTrue) &&
				((private_data->is_sticky) || (inParam2 & LIcMouseState_LButtonDown)))
			{
				if (WMrWindow_PointInWindow(inMenuBar, &global_mouse))
				{
					// unhilite the currently hilited menu
					WMiMenuBar_UnhiliteMenu(inMenuBar, UUcTrue);

					// hilite the newly hilited menu
					WMiMenuBar_HiliteMenu(inMenuBar, &local_mouse);
				}
				else
				{
					// forward the message
					WMiMenuBar_ForwardMessage(inMenuBar, inMessage, inParam1, inParam2);
				}
			}
		break;

		case WMcMessage_LMouseDown:
			WMrWindow_SetFocus(inMenuBar);

			if (private_data->active)
			{
				// forward the message
				WMiMenuBar_ForwardMessage(inMenuBar, inMessage, inParam1, inParam2);
			}
			else
			{
				// the menu bar is active
				private_data->active = UUcTrue;

				if (private_data->is_sticky)
				{
					// no longer sticky
					private_data->is_sticky = UUcFalse;

					// release the mouse (this will cause the menu to close because
					// it will send WMcMessage_CaptureChanged to the menubar callback)
					WMrWindow_CaptureMouse(NULL);
				}
				else
				{
					// capture the mouse
					WMrWindow_CaptureMouse(inMenuBar);

					// hilite the menu under the mouse point visible
					WMiMenuBar_HiliteMenu(inMenuBar, &local_mouse);
				}

				// save the time the mouse went down
				private_data->mouse_sticky_time = UUrMachineTime_Sixtieths() + WMcMenuBar_StickyDelay;
			}
		break;

		case WMcMessage_LMouseUp:
		{
			UUtUns32			time;

			time = UUrMachineTime_Sixtieths();

			// check for sticky menu
			if ((private_data->mouse_sticky_time > time ) &&
				(private_data->hilited_menu != WMcMenuBar_MaxNumMenus))
			{
				private_data->is_sticky = UUcTrue;
			}
			else
			{
				// forward the message to the open menu
				WMiMenuBar_ForwardMessage(inMenuBar, inMessage, inParam1, inParam2);

				// release the mouse (this will cause the menu to close because
				// it will send WMcMessage_CaptureChanged to the menubar callback)
				WMrWindow_CaptureMouse(NULL);

				// the menu bar is no longer active
				private_data->active = UUcFalse;
			}
		}
		break;
	}
}

// ----------------------------------------------------------------------
static void
WMiMenuBar_Paint(
	WMtMenuBar				*inMenuBar)
{
	DCtDrawContext			*draw_context;
	UUtInt32				i;
	PStPartSpecUI			*partspec_ui;
	WMtMenuBar_PrivateData	*private_data;
	TStFontInfo				font_info;

	// get the private data
	private_data = (WMtMenuBar_PrivateData*)WMrWindow_GetLong(inMenuBar, 0);
	if (private_data == NULL) { return; }

	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) return;

	draw_context = DCrDraw_Begin(inMenuBar);

	WMrWindow_GetFontInfo(inMenuBar, &font_info);
	DCrText_SetFontInfo(&font_info);

	// draw the menu titles
	for (i = 0; i < private_data->num_menubar_items; i++)
	{
		UUtBool					enabled;
		IMtPoint2D				dest;

		dest.x = private_data->menubar_items[i].location.x;
		dest.y = 0;

		enabled = WMrWindow_GetEnabled(private_data->menubar_items[i].menu);

		if ((private_data->hilited_menu == i) && (enabled))
		{
			UUtUns16			width;
			UUtUns16			height;

			// set the width and height
			width = private_data->menubar_items[i].width + WMcMenuBar_Spacing;
			height = private_data->menubar_items[i].height;

			DCrDraw_PartSpec(
				draw_context,
				partspec_ui->hilite,
				PScPart_All,
				&dest,
				(UUtInt16)width,
				(UUtInt16)height,
				M3cMaxAlpha);
		}

		dest.x += WMcMenuBar_Spacing_Left;
		dest.y += DCrText_GetAscendingHeight();

		DCrText_SetShade(enabled ? font_info.font_shade : IMcShade_Gray50);
		DCrText_SetFormat(TSc_HLeft);
		DCrDraw_String(
			draw_context,
			WMrWindow_GetTitlePtr(private_data->menubar_items[i].menu),
			NULL,
			&dest);
	}

	DCrDraw_End(draw_context, inMenuBar);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
WMiMenuBar_Callback(
	WMtMenuBar				*inMenuBar,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtUns32				result;

	// handle the message
	switch (inMessage)
	{
		case WMcMessage_NC_HitTest:
		return WMcWindowArea_Client;

		case WMcMessage_Create:
			// create the menubar
			result = WMiMenuBar_Create(inMenuBar, (WMtMenuBarData*)inParam1);
		return result;

		case WMcMessage_Destroy:
			WMiMenuBar_Destroy(inMenuBar);
		return WMcResult_Handled;

		case WMcMessage_MouseMove:
		case WMcMessage_LMouseDown:
		case WMcMessage_LMouseUp:
			WMiMenuBar_HandleMouseEvent(inMenuBar, inMessage, inParam1, inParam2);
		return WMcResult_Handled;

		case WMcMessage_Paint:
			WMiMenuBar_Paint(inMenuBar);
		return WMcResult_Handled;

		case WMcMessage_CaptureChanged:
			WMiMenuBar_HandleCaptureChanged(inMenuBar);
		return WMcResult_Handled;

		case WMcMessage_MenuCommand:
			// turn off the previously hilited menu
			WMiMenuBar_UnhiliteMenu(inMenuBar, UUcFalse);

			// turn off mouse capture
			WMrWindow_CaptureMouse(NULL);

			// forward message to parent
			result =
				WMrMessage_Send(
					WMrWindow_GetParent(inMenuBar),
					inMessage,
					inParam1,
					inParam2);
		return result;
	}

	return WMrWindow_DefaultCallback(inMenuBar, inMessage, inParam1, inParam2);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
WMrMenuBar_AppendMenu(
	WMtMenuBar				*inMenuBar,
	WMtMenu					*inMenu)
{
	return WMrMenuBar_InsertMenu(inMenuBar, inMenu, -1);
}

// ----------------------------------------------------------------------
WMtMenuBar*
WMrMenuBar_Create(
	WMtWindow				*inParent,
	char					*inMenuBarName)
{
	UUtError				error;
	WMtMenuBar				*menubar;
	UUtRect					client_rect;
	UUtInt16				width;
	WMtMenuBarData			*menubar_data;
	UUtUns32				flags;

	// get the menubar data
	error =
		TMrInstance_GetDataPtr(
			WMcTemplate_MenuBarData,
			inMenuBarName,
			&menubar_data);
	if (error != UUcError_None) { return NULL; }

	// set the flags
	flags = WMcWindowFlag_Visible | WMcWindowFlag_TopMost;
	if (inParent) { flags |= WMcWindowFlag_Child; }

	// get the width of the parent
	WMrWindow_GetClientRect(inParent, &client_rect);

	// calculate the width of the client rect
	width = client_rect.right - client_rect.left;

	// create the menubar
	menubar =
		WMrWindow_New(
			WMcWindowType_MenuBar,
			"menubar",
			flags,
			WMcMenuBarStyle_Standard,
			menubar_data->id,
			0,
			0,
			width,
			0,
			inParent,
			(UUtUns32)menubar_data);
	return menubar;
}

// ----------------------------------------------------------------------
WMtMenu*
WMrMenuBar_GetMenu(
	WMtMenuBar				*inMenuBar,
	UUtUns16				inMenuID)
{
	WMtMenu					*menu;
	WMtMenuBar_PrivateData	*private_data;
	UUtInt32				i;

	// get the private data
	private_data = (WMtMenuBar_PrivateData*)WMrWindow_GetLong(inMenuBar, 0);
	UUmAssert(private_data);

	menu = NULL;

	// find the menu with id == inMenuID
	for (i = 0; i < private_data->num_menubar_items; i++)
	{
		UUtUns16			id;

		id = WMrWindow_GetID(private_data->menubar_items[i].menu);
		if (id == inMenuID)
		{
			menu = private_data->menubar_items[i].menu;
			break;
		}
	}

	return menu;
}

// ----------------------------------------------------------------------
UUtError
WMrMenuBar_InsertMenu(
	WMtMenuBar				*inMenuBar,
	WMtMenu					*inMenu,
	UUtInt16				inIndex)
{
	UUtInt32				index;
	WMtMenuBar_PrivateData	*private_data;
	UUtRect					bounds;

	// get the private data
	private_data = (WMtMenuBar_PrivateData*)WMrWindow_GetLong(inMenuBar, 0);
	UUmAssert(private_data);

	if (private_data->num_menubar_items == WMcMenuBar_MaxNumMenus)
	{
		return UUcError_Generic;
	}

	// insert the menu
	if (inIndex == -1)
	{
		index = private_data->num_menubar_items;
	}
	else
	{
		// insert the menu at the specified position in the menu bar items list
		if (inIndex > private_data->num_menubar_items)
		{
			index = private_data->num_menubar_items;
		}
		else
		{
			UUtInt32				i;

			for (i = (UUtInt32)inIndex; i < private_data->num_menubar_items; i++)
			{
				private_data->menubar_items[i + 1] = private_data->menubar_items[i];
			}

			index = (UUtInt32)inIndex;
		}
	}

	// set the menubar item info
	private_data->menubar_items[index].location.x = 0;
	private_data->menubar_items[index].location.y = 0;
	private_data->menubar_items[index].menu = inMenu;
	private_data->num_menubar_items++;

	DCrText_GetStringRect(
		WMrWindow_GetTitlePtr(private_data->menubar_items[index].menu),
		&bounds);

	private_data->menubar_items[index].width = bounds.right - bounds.left;
	private_data->menubar_items[index].height = bounds.bottom - bounds.top;

	// recalc menu positions
	WMiMenuBar_RecalcMenuBarItemPositions(inMenuBar, private_data);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
WMrMenuBar_Initialize(
	void)
{
	UUtError				error;
	WMtWindowClass			window_class;

	// register the window class
	UUrMemory_Clear(&window_class, sizeof(WMtWindowClass));
	window_class.type = WMcWindowType_MenuBar;
	window_class.callback = WMiMenuBar_Callback;
	window_class.private_data_size = sizeof(WMtMenuBar_PrivateData*);

	error = WMrWindowClass_Register(&window_class);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
WMrMenuBar_RegisterTemplates(
	void)
{
	UUtError				error;

	error =
		TMrTemplate_Register(
			WMcTemplate_MenuBarData,
			sizeof(WMtMenuBarData),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
WMrMenuBar_RemoveMenu(
	WMtMenuBar				*inMenuBar,
	UUtUns16				inMenuID)
{
	WMtMenuBar_PrivateData	*private_data;

	// get the private data
	private_data = (WMtMenuBar_PrivateData*)WMrWindow_GetLong(inMenuBar, 0);
	UUmAssert(private_data);

	UUmAssert(!"The rest isn't written yet");

	return UUcError_None;
}


