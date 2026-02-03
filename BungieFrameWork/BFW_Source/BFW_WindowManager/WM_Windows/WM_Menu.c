// ======================================================================
// WM_Menu.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_WindowManager.h"
#include "BFW_TextSystem.h"

#include "WM_Menu.h"
#include "WM_DrawContext.h"
#include "WM_PartSpecification.h"
#include "WM_Utilities.h"

// ======================================================================
// defines
// ======================================================================
#define WMcMenu_DefaultNumItems		7
#define WMcMenu_NoItem				0xFFFFFFFF

// spacing applied to sides of menu
#define WMcMenu_Buffer_Left			12
#define WMcMenu_Buffer_Right		10

// minimum distance to leave between menu and top/bottom of desktop
#define WMcMenu_Buffer_Top			3
#define WMcMenu_Buffer_Bottom		3

// ======================================================================
// typedefs
// ======================================================================
typedef struct WMtMenuItem
{
	WMtMenuItemData			item_data;
	UUtInt16				line_height;
	UUtInt16				line_width;

} WMtMenuItem;

typedef struct WMtMenuColumn
{
	UUtInt16				item_baseindex;
	UUtInt16				item_count;

	UUtInt16				column_width;
	UUtInt16				column_height;
} WMtMenuColumn;

typedef struct WMtMenu_PrivateData
{
	UUtInt16				rect_width;			// width of the rect behind menu items
	UUtInt16				rect_height;		// height of the rect behind menu items

	UUtBool					recalc_layout;
	UUtBool					pad;
	UUtInt16				max_width;

	UUtUns32				hilited_item;		// index of menu_item that the mouse is currently over

	UUtMemory_Array			*menu_items;		// array of menu items
	UUtMemory_Array			*menu_columns;		// array of menu columns

} WMtMenu_PrivateData;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
UUtInt16
WMrMenu_GetWidth(
	WMtMenu					*inMenu)
{
	WMtMenu_PrivateData		*private_data;

	// get the private data
	private_data = (WMtMenu_PrivateData*)WMrWindow_GetLong(inMenu, 0);
	if (private_data == NULL) { return 0; }

	return private_data->max_width;
}

// ----------------------------------------------------------------------
static void
WMiMenu_Layout(
	WMtMenu					*inMenu,
	WMtMenu_PrivateData		*inPrivateData)
{
	UUtUns32				i, c, num_items, num_columns;
	WMtMenuItem				*menu_items;
	WMtMenuColumn			*column;
	PStPartSpecUI			*partspec_ui;
	UUtInt16				border_width;
	UUtInt16				border_height;
	WMtMenu_PrivateData		*private_data;

	// get the private data
	private_data = (WMtMenu_PrivateData*)WMrWindow_GetLong(inMenu, 0);
	if (private_data == NULL) { return; }

	// get the partspec ui
	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) {
		return;
	}

	// get the width and height of the border
	PSrPartSpec_GetSize(partspec_ui->background, PScPart_LeftTop, &border_width, &border_height);

	// no columns yet
	num_columns = 0;
	UUrMemory_Array_SetUsedElems(private_data->menu_columns, 0, NULL);

	num_items = UUrMemory_Array_GetUsedElems(inPrivateData->menu_items);
	menu_items = (WMtMenuItem *)UUrMemory_Array_GetMemory(inPrivateData->menu_items);
	if (menu_items == NULL) {
		return;
	}

	// loop over all menu items and break them into columns
	c = 0;
	private_data->max_width = 0;
	for (i = 0; i < num_items; ) {
		if (c >= num_columns) {
			UUrMemory_Array_GetNewElement(inPrivateData->menu_columns, &c, NULL);
			UUmAssert(c == num_columns);

			column = ((WMtMenuColumn *) UUrMemory_Array_GetMemory(inPrivateData->menu_columns)) + num_columns;
			num_columns++;

			column->column_width = 0;
			column->column_height = 2*border_height;
			column->item_baseindex = (UUtInt16) i;
			column->item_count = 0;
		}

		if ((column->column_height + menu_items[i].line_height > private_data->rect_height) &&
			(column->item_count > 0)) {
			// go to the next column
			private_data->max_width = UUmMax(private_data->max_width, column->column_width);
			c++;
		} else {
			// add this item to the column
			column->column_height += menu_items[i].line_height;
			column->column_width = UUmMax(column->column_width, menu_items[i].line_width
											+ 2*border_width + WMcMenu_Buffer_Left + WMcMenu_Buffer_Right);
			column->item_count++;
			i++;
		}
	}

	// layout is done, don't need to recalculate again
	inPrivateData->recalc_layout = UUcFalse;
}

// ----------------------------------------------------------------------
void
WMrMenu_Locate(
	WMtMenu					*inMenu,
	IMtPoint2D				*ioLocation,
	UUtBool					inFixedLocation)
{
	UUtUns32				i, c, num_items;
	UUtInt16				total_height, total_width, max_width;
	WMtMenuItem				*menu_items;
	WMtWindow				*desktop;
	UUtRect					desktop_rect;
	PStPartSpecUI			*partspec_ui;
	UUtInt16				border_width;
	UUtInt16				border_height;
	WMtMenu_PrivateData		*private_data;

	// get the private data
	private_data = (WMtMenu_PrivateData*)WMrWindow_GetLong(inMenu, 0);
	if (private_data == NULL) { return; }

	/*
	 * determine where the menu should go vertically
	 */

	// get the desktop rect
	desktop = WMrGetDesktop();
	if (desktop == NULL) {
		goto exit;
	}
	WMrWindow_GetRect(desktop, &desktop_rect);

	// get the partspec ui
	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) {
		goto exit;
	}

	// get the width and height of the border
	PSrPartSpec_GetSize(partspec_ui->background, PScPart_LeftTop, &border_width, &border_height);

	// calculate total height of the menu
	num_items = UUrMemory_Array_GetUsedElems(private_data->menu_items);
	menu_items = (WMtMenuItem *)UUrMemory_Array_GetMemory(private_data->menu_items);
	if (menu_items == NULL) {
		return;
	}

	total_height = 2 * border_height;
	for (i = 0; i < num_items; i++) {
		total_height += menu_items[i].line_height;
	}

	if (total_height + ioLocation->y + WMcMenu_Buffer_Bottom < desktop_rect.bottom) {
		// we can place the menu at the desired location
		private_data->rect_height = total_height;
	} else {
		if (inFixedLocation) {
			// we can't move the menu, we have to split it into multiple columns
			private_data->rect_height = desktop_rect.bottom - WMcMenu_Buffer_Bottom - ioLocation->y;
		} else {
			// maybe we can move the menu up in order to fit it on the screen
			if (desktop_rect.bottom - total_height - WMcMenu_Buffer_Bottom >= desktop_rect.top + WMcMenu_Buffer_Top) {
				// we can place the whole menu in a single column on the screen
				private_data->rect_height = total_height;
				ioLocation->y = desktop_rect.bottom - total_height - WMcMenu_Buffer_Bottom;
			} else {
				// we must split the menu - start the first column at the top of the screen
				private_data->rect_height = desktop_rect.bottom - WMcMenu_Buffer_Top;
				ioLocation->y = WMcMenu_Buffer_Top;
			}
		}
	}

	/*
	 * we cannot fit the menu on the screen vertically - calculate the horizontal space required
	 */

	total_height = 2*border_height;
	total_width = 0;
	max_width = 0;
	c = 0;
	for (i = 0; i < num_items; i++) {
		if (total_height + menu_items[i].line_height > private_data->rect_height) {
			// go to the next column
			c++;
			total_width += max_width;
			max_width = 0;
			total_height = 2*border_height;
		}

		total_height += menu_items[i].line_height;
		max_width = UUmMax(max_width, menu_items[i].line_width + 2*border_width + WMcMenu_Buffer_Left + WMcMenu_Buffer_Right);
	}
	total_width += max_width;
	c++;

	if (ioLocation->x + total_width < desktop_rect.right) {
		// the menu fits on the screen horizontally
		private_data->rect_width = total_width;

	} else {
		if (inFixedLocation) {
			// the menu is truncated at the edge of the screen
			private_data->rect_width = (desktop_rect.right - ioLocation->x);
		} else {
			// shift the menu over to the left in order to make room
			private_data->rect_width = UUmMin(total_width, desktop_rect.right);
			ioLocation->x = desktop_rect.right - private_data->rect_width;
		}
	}

exit:
	WMrWindow_SetLocation(inMenu, ioLocation->x, ioLocation->y);
	WMrWindow_SetSize(inMenu, private_data->rect_width, private_data->rect_height);

	private_data->recalc_layout = UUcTrue;
	return;
}

// ----------------------------------------------------------------------
static UUtUns32
WMiMenu_GetItemUnderPoint(
	WMtMenu					*inMenu,
	WMtMenu_PrivateData		*inPrivateData,
	IMtPoint2D				*inPoint)
{
	UUtUns32				c, i, num_cols, num_items;
	UUtRect					column_bounds, item_bounds;
	WMtMenuItem				*menu_items;
	WMtMenuColumn			*menu_columns;
	PStPartSpecUI			*partspec_ui;
	UUtInt16				border_width;
	UUtInt16				border_height;

	if (inPrivateData->recalc_layout) {
		WMiMenu_Layout(inMenu, inPrivateData);
	}

	// get the partspec ui
	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) { return WMcMenu_NoItem; }

	// get the width and height of the border
	PSrPartSpec_GetSize(partspec_ui->background, PScPart_LeftTop, &border_width, &border_height);

	// set the bounds
	column_bounds.left = 0;
	column_bounds.top = border_height;
	column_bounds.right = inPrivateData->rect_width;
	column_bounds.bottom = 0;	// set up inside loop

	// get the array pointers
	num_cols = UUrMemory_Array_GetUsedElems(inPrivateData->menu_columns);
	menu_columns = (WMtMenuColumn *)UUrMemory_Array_GetMemory(inPrivateData->menu_columns);
	if (menu_columns == NULL) { return WMcMenu_NoItem; }

	num_items = UUrMemory_Array_GetUsedElems(inPrivateData->menu_items);
	menu_items = (WMtMenuItem *)UUrMemory_Array_GetMemory(inPrivateData->menu_items);
	if (menu_items == NULL) { return WMcMenu_NoItem; }

	// find the menu column
	for (c = 0; c < num_cols; c++)
	{
		column_bounds.right = column_bounds.left + menu_columns[c].column_width;
		column_bounds.bottom = column_bounds.top + menu_columns[c].column_height;

		if (IMrRect_PointIn(&column_bounds, inPoint)) {
			// find the menu item
			item_bounds = column_bounds;

			for (i = menu_columns[c].item_baseindex; i < (UUtUns32) (menu_columns[c].item_baseindex + menu_columns[c].item_count); i++)
			{
				item_bounds.bottom = item_bounds.top + menu_items[i].line_height;

				// check the point
				if (IMrRect_PointIn(&item_bounds, inPoint))
				{
					return i;
				}

				item_bounds.top = item_bounds.bottom;
			}
		}

		column_bounds.left = column_bounds.right;
	}

	return WMcMenu_NoItem;
}

// ----------------------------------------------------------------------
static void
WMiMenu_RecalcLineSizes(
	WMtMenu					*inMenu,
	WMtMenu_PrivateData		*inPrivateData)
{
	TStFontInfo				font_info;
	UUtUns32				num_items;
	WMtMenuItem				*menu_items;
	UUtUns32				i;

	WMrWindow_GetFontInfo(inMenu, &font_info);
	DCrText_SetFontInfo(&font_info);

	num_items = UUrMemory_Array_GetUsedElems(inPrivateData->menu_items);
	menu_items = (WMtMenuItem *)UUrMemory_Array_GetMemory(inPrivateData->menu_items);
	for (i = 0; i < num_items; i++)
	{
		UUtRect					rect;

		// calculate the dimensions of the string
		DCrText_GetStringRect(menu_items[i].item_data.title, &rect);
		menu_items[i].line_width = rect.right - rect.left;
		menu_items[i].line_height = rect.bottom - rect.top;
	}

	inPrivateData->recalc_layout = UUcTrue;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
WMiMenu_Create(
	WMtMenu 				*inMenu,
	WMtMenuData				*inMenuData)
{
	UUtError				error;
	WMtMenu_PrivateData		*private_data;

	// create the private data
	private_data = (WMtMenu_PrivateData*)UUrMemory_Block_NewClear(sizeof(WMtMenu_PrivateData));
	if (private_data == NULL) { goto cleanup; }
	WMrWindow_SetLong(inMenu, 0, (UUtUns32)private_data);

	// initialize
	private_data->rect_width		= 0;
	private_data->rect_height		= 0;
	private_data->recalc_layout		= UUcTrue;
	private_data->hilited_item		= WMcMenu_NoItem;

	// ------------------------------
	// allocate memory for the menu items
	// ------------------------------
	private_data->menu_items =
		UUrMemory_Array_New(
			sizeof(WMtMenuItem),
			WMcMenu_DefaultNumItems,
			0,
			WMcMenu_DefaultNumItems);
	if (private_data->menu_items == NULL) { goto cleanup; };

	private_data->menu_columns = UUrMemory_Array_New(sizeof(WMtMenuColumn), 1, 0, 1);
	if (private_data->menu_columns == NULL) { goto cleanup; };

	// ------------------------------
	// setup the item display rect
	// ------------------------------
	if ((inMenuData != NULL) && (UUrMemory_Array_GetUsedElems(private_data->menu_items) == 0))
	{
		UUtUns32				i;

		// insert each item to the menu
		for (i = 0; i < inMenuData->num_items; i++)
		{
			error = WMrMenu_AppendItem(inMenu, &inMenuData->items[i]);
			if (error != UUcError_None) { goto cleanup; }
		}
	}

	return WMcResult_Handled;

cleanup:
	UUmAssert(0);

	if (private_data)
	{
		if (private_data->menu_items)
		{
			UUrMemory_Array_Delete(private_data->menu_items);
			private_data->menu_items = NULL;
		}

		if (private_data->menu_columns)
		{
			UUrMemory_Array_Delete(private_data->menu_columns);
			private_data->menu_columns = NULL;
		}

		UUrMemory_Block_Delete(private_data);
		private_data = NULL;
	}
	WMrWindow_SetLong(inMenu, 0, 0);

	return WMcResult_Error;
}

// ----------------------------------------------------------------------
static void
WMiMenu_Destroy(
	WMtMenu					*inMenu)
{
	WMtMenu_PrivateData		*private_data;

	// get the private data
	private_data = (WMtMenu_PrivateData*)WMrWindow_GetLong(inMenu, 0);
	if (private_data == NULL) { return; }

	if (private_data->menu_items)
	{
		UUrMemory_Array_Delete(private_data->menu_items);
		private_data->menu_items = NULL;
	}

	if (private_data->menu_columns)
	{
		UUrMemory_Array_Delete(private_data->menu_columns);
		private_data->menu_columns = NULL;
	}

	UUrMemory_Block_Delete(private_data);
	WMrWindow_SetLong(inMenu, 0, 0);
}

// ----------------------------------------------------------------------
static void
WMiMenu_HandleMouseEvent(
	WMtMenu					*inMenu,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	IMtPoint2D				global_mouse_location;
	IMtPoint2D				mouse_location;
	WMtMenu_PrivateData		*private_data;

	// only handle mouse events if the window is enabled
	if (WMrWindow_GetEnabled(inMenu) == UUcFalse) { return; }

	// get the private data
	private_data = (WMtMenu_PrivateData*)WMrWindow_GetLong(inMenu, 0);
	if (private_data == NULL) { return; }

	// get the global mouse location
	global_mouse_location.x = (UUtInt16)UUmHighWord(inParam1);
	global_mouse_location.y = (UUtInt16)UUmLowWord(inParam1);

	// make sure the mouse is within the menu's bounds
	if (WMrWindow_PointInWindow(inMenu, &global_mouse_location) == UUcFalse)
	{
		return;
	}

	WMrWindow_GlobalToLocal(inMenu, &global_mouse_location, &mouse_location);

	switch (inMessage)
	{
		case WMcMessage_MouseMove:
			private_data->hilited_item = WMiMenu_GetItemUnderPoint(inMenu, private_data, &mouse_location);
		break;

		case WMcMessage_LMouseDown:
			WMrWindow_SetFocus(inMenu);
			// fall through

		case WMcMessage_LMouseUp:
		{
			UUtUns32		selected_item;
			WMtMenuItem		*menu_items;

			// get the array pointer
			menu_items = (WMtMenuItem*)UUrMemory_Array_GetMemory(private_data->menu_items);
			if (menu_items == NULL) { break; }

			// send a MenuSelect message to the menu's parent if an enabled menu item was
			// hilited when the mousebutton went up
			selected_item = WMiMenu_GetItemUnderPoint(inMenu, private_data, &mouse_location);
			if ((selected_item != WMcMenu_NoItem) &&
				(menu_items[selected_item].item_data.flags & WMcMenuItemFlag_Enabled))
			{
				// tell parent
				WMrMessage_Send(
					WMrWindow_GetOwner(inMenu),
					WMcMessage_MenuCommand,
					UUmMakeLong(
						menu_items[selected_item].item_data.flags,
						menu_items[selected_item].item_data.id),
					(UUtUns32)inMenu);
			}
		}
		break;
	}
}

// ----------------------------------------------------------------------
static void
WMiMenu_HandleVisible(
	WMtMenu					*inMenu)
{
	WMtMenu_PrivateData		*private_data;

	// get the private data
	private_data = (WMtMenu_PrivateData*)WMrWindow_GetLong(inMenu, 0);
	if (private_data == NULL) { return; }

	private_data->hilited_item = WMcMenu_NoItem;
}

// ----------------------------------------------------------------------
static void
WMiMenu_HandleFontInfoChanged(
	WMtMenu					*inMenu)
{
	WMtMenu_PrivateData		*private_data;

	// get the private data
	private_data = (WMtMenu_PrivateData*)WMrWindow_GetLong(inMenu, 0);
	if (private_data == NULL) { return; }

	WMiMenu_RecalcLineSizes(inMenu, private_data);
}

// ----------------------------------------------------------------------
static void
WMiMenu_Paint(
	WMtMenu					*inMenu)
{
	DCtDrawContext			*draw_context;
	IMtPoint2D				column_dest, item_dest;
	UUtUns32				i, c, num_items, num_columns;
	PStPartSpecUI			*partspec_ui;
	WMtMenu_PrivateData		*private_data;
	WMtMenuItem				*menu_items;
	WMtMenuColumn			*menu_columns;
	UUtInt16				border_width;
	UUtInt16				border_height;
	TStFontInfo				font_info;

	// get the private data
	private_data = (WMtMenu_PrivateData*)WMrWindow_GetLong(inMenu, 0);
	if (private_data == NULL) { return; }

	// lay out our items if we have to
	if (private_data->recalc_layout) {
		WMiMenu_Layout(inMenu, private_data);
	}

	// get the array pointers
	num_items = UUrMemory_Array_GetUsedElems(private_data->menu_items);
	menu_items = (WMtMenuItem *)UUrMemory_Array_GetMemory(private_data->menu_items);
	if (menu_items == NULL) { return; }
	num_columns = UUrMemory_Array_GetUsedElems(private_data->menu_columns);
	menu_columns = (WMtMenuColumn *)UUrMemory_Array_GetMemory(private_data->menu_columns);
	if (menu_columns == NULL) { return; }

	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) { return; }

	// get the width and height of the border
	PSrPartSpec_GetSize(partspec_ui->background, PScPart_LeftTop, &border_width, &border_height);

	draw_context = DCrDraw_Begin(inMenu);

	// get the font info
	WMrWindow_GetFontInfo(inMenu, &font_info);
	DCrText_SetFontInfo(&font_info);

	column_dest.x = 0;
	column_dest.y = 0;
	for (c = 0; c < num_columns; c++) {
		// draw the column's background and border
		DCrDraw_PartSpec(draw_context, partspec_ui->background, PScPart_All, &column_dest,
			(UUtUns16) menu_columns[c].column_width, (UUtUns16) menu_columns[c].column_height, M3cMaxAlpha);

		item_dest.x = column_dest.x + border_width + WMcMenu_Buffer_Left;
		item_dest.y = column_dest.y + border_height;
		for (i = menu_columns[c].item_baseindex; i < (UUtUns32) (menu_columns[c].item_baseindex + menu_columns[c].item_count); i++) {

			if (menu_items[i].item_data.flags & WMcMenuItemFlag_Divider)
			{
				IMtPoint2D		divider_dest;

				item_dest.y += (menu_items[i].line_height >> 1);

				divider_dest.x = column_dest.x + border_width;
				divider_dest.y = item_dest.y;

				// draw divider
				DCrDraw_PartSpec(
					draw_context,
					partspec_ui->divider,
					PScPart_All,
					&divider_dest,
					menu_columns[c].column_width - 2*border_width,
					menu_items[i].line_height,
					M3cMaxAlpha);

				item_dest.y += (menu_items[i].line_height >> 1);
			}
			else
			{
				UUtBool						enabled;

				if (WMrWindow_GetEnabled(inMenu))
				{
					enabled = (menu_items[i].item_data.flags & WMcMenuItemFlag_Enabled) != 0;
				}
				else
				{
					enabled = UUcFalse;
				}

				// draw hilite
				if ((private_data->hilited_item == i) && (enabled))
				{
					IMtPoint2D				hilite_dest;

					hilite_dest.x = column_dest.x + border_width;
					hilite_dest.y = item_dest.y;

					DCrDraw_PartSpec(
						draw_context,
						partspec_ui->hilite,
						PScPart_All,
						&hilite_dest,
						menu_columns[c].column_width - 2*border_width,
						menu_items[i].line_height,
						M3cMaxAlpha);
				}

				// draw check
				if ((menu_items[i].item_data.flags & WMcMenuItemFlag_Checked) != 0)
				{
					UUtInt16				check_width;
					UUtInt16				check_height;
					IMtPoint2D				check_dest;

					PSrPartSpec_GetSize(partspec_ui->check, PScPart_LeftTop, &check_width, &check_height);
					if (check_height > menu_items[i].line_height)
					{
						check_height = menu_items[i].line_height;
					}

					check_dest.x = column_dest.x + border_width;
					check_dest.y = item_dest.y + ((menu_items[i].line_height - check_height) >> 1);

					DCrDraw_PartSpec(
						draw_context,
						partspec_ui->check,
						PScPart_MiddleMiddle,
						&check_dest,
						check_width,
						check_height,
						enabled ? (UUtUns16)WMcAlpha_Enabled : (UUtUns16)WMcAlpha_Disabled);
				}

				item_dest.y += DCrText_GetAscendingHeight();
				DCrText_SetShade(enabled ? font_info.font_shade : IMcShade_Gray50);
				DCrText_SetFormat(TSc_HLeft);
				DCrDraw_String(
					draw_context,
					menu_items[i].item_data.title,
					NULL,
					&item_dest);
				item_dest.y -= DCrText_GetAscendingHeight();

				item_dest.y += menu_items[i].line_height;
			}
		}

		column_dest.x += menu_columns[c].column_width;
	}

	DCrDraw_End(draw_context, inMenu);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
WMiMenu_Callback(
	WMtMenu					*inMenu,
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
			result = WMiMenu_Create(inMenu, (WMtMenuData*)inParam1);
		return result;

		case WMcMessage_Destroy:
			WMiMenu_Destroy(inMenu);
		return WMcResult_Handled;

		case WMcMessage_MouseMove:
		case WMcMessage_LMouseDown:
		case WMcMessage_LMouseUp:
			WMiMenu_HandleMouseEvent(inMenu, inMessage, inParam1, inParam2);
		return WMcResult_Handled;

		case WMcMessage_Paint:
			WMiMenu_Paint(inMenu);
		return WMcResult_Handled;

		case WMcMessage_Visible:
			WMiMenu_HandleVisible(inMenu);
		return WMcResult_Handled;

		case WMcMessage_FontInfoChanged:
			WMiMenu_HandleFontInfoChanged(inMenu);
		break;
	}

	return WMrWindow_DefaultCallback(inMenu, inMessage, inParam1, inParam2);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
WMrMenu_AppendItem(
	WMtMenu					*inMenu,
	WMtMenuItemData			*inMenuItemData)
{
	return WMrMenu_InsertItem(inMenu, inMenuItemData, -1);
}

// ----------------------------------------------------------------------
WMtMenu*
WMrMenu_Create(
	WMtMenuData				*inMenuData,
	WMtWindow				*inParent)
{
	WMtMenu					*menu;

	menu =
		WMrWindow_New(
			WMcWindowType_Menu,
			(inMenuData == NULL) ? "" : inMenuData->title,
			WMcWindowFlag_PopUp,
			WMcMenuStyle_Standard,
			(inMenuData == NULL) ? 0 : inMenuData->id,
			0,
			0,
			0,
			0,
			inParent,
			(UUtUns32)inMenuData);

	return menu;
}

// ----------------------------------------------------------------------
UUtBool
WMrMenu_CheckItem(
	WMtMenu					*inMenu,
	UUtUns16				inItemID,
	UUtBool					inCheck)
{
	WMtMenu_PrivateData		*private_data;
	WMtMenuItem				*menu_items;
	UUtUns32				i;

	// get the private data
	private_data = (WMtMenu_PrivateData*)WMrWindow_GetLong(inMenu, 0);
	if (private_data == NULL) { return UUcFalse; }
	if (private_data->menu_items == NULL) { return UUcFalse; }

	// get the array pointer
	menu_items = (WMtMenuItem*)UUrMemory_Array_GetMemory(private_data->menu_items);
	if (menu_items == NULL) { return UUcFalse; }

	// find the menu item with item id
	for (i = 0; i < UUrMemory_Array_GetUsedElems(private_data->menu_items); i++)
	{
		if (menu_items[i].item_data.id == inItemID) { break; }
	}
	if (i == UUrMemory_Array_GetUsedElems(private_data->menu_items)) { return UUcFalse; }

	// check/uncheck the menu item
	if (inCheck)
	{
		menu_items[i].item_data.flags |= WMcMenuItemFlag_Checked;
	}
	else
	{
		menu_items[i].item_data.flags &= ~WMcMenuItemFlag_Checked;
	}

	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtBool
WMrMenu_EnableItem(
	WMtMenu					*inMenu,
	UUtUns16				inItemID,
	UUtBool					inEnable)
{
	WMtMenu_PrivateData		*private_data;
	WMtMenuItem				*menu_items;
	UUtUns32				i;

	// get the private data
	private_data = (WMtMenu_PrivateData*)WMrWindow_GetLong(inMenu, 0);
	if (private_data == NULL) { return UUcFalse; }
	if (private_data->menu_items == NULL) { return UUcFalse; }

	// get the array pointer
	menu_items = (WMtMenuItem*)UUrMemory_Array_GetMemory(private_data->menu_items);
	if (menu_items == NULL) { return UUcFalse; }

	// find the menu item with item id
	for (i = 0; i < UUrMemory_Array_GetUsedElems(private_data->menu_items); i++)
	{
		if (menu_items[i].item_data.id == inItemID) { break; }
	}
	if (i == UUrMemory_Array_GetUsedElems(private_data->menu_items)) { return UUcFalse; }

	// enable/disable the menu item
	if (inEnable)
	{
		menu_items[i].item_data.flags |= WMcMenuItemFlag_Enabled;
	}
	else
	{
		menu_items[i].item_data.flags &= ~WMcMenuItemFlag_Enabled;
	}

	return UUcTrue;
}

// ----------------------------------------------------------------------

typedef enum {
	WMcMenu_FindItemByText_ObeyCase,
	WMcMenu_FindItemByText_IgnoreCase
} WMtMenu_FindItemByText_Case;

static UUtBool
WMrMenu_FindItemByText_Internal(
	WMtMenu					*inMenu,
	char					*inText,
	UUtUns16				*outItemID,
	WMtMenu_FindItemByText_Case inCase)
{
	WMtMenu_PrivateData		*private_data;
	WMtMenuItem				*menu_items;
	UUtUns32				i;

	// get the private data
	private_data = (WMtMenu_PrivateData*)WMrWindow_GetLong(inMenu, 0);
	if (private_data == NULL) { return UUcFalse; }
	if (private_data->menu_items == NULL) { return UUcFalse; }

	// get the array pointer
	menu_items = (WMtMenuItem*)UUrMemory_Array_GetMemory(private_data->menu_items);
	if (menu_items == NULL) { return UUcFalse; }

	// find the menu item with this text
	for (i = 0; i < UUrMemory_Array_GetUsedElems(private_data->menu_items); i++)
	{
		if (menu_items[i].item_data.flags & WMcMenuItemFlag_Divider)
			continue;

		if (WMcMenu_FindItemByText_ObeyCase == inCase) {
			if (strcmp(menu_items[i].item_data.title, inText) == 0)
				break;
		}
		else {
			if (UUrString_Compare_NoCase(menu_items[i].item_data.title, inText) == 0)
				break;
		}
	}
	if (i == UUrMemory_Array_GetUsedElems(private_data->menu_items)) { return UUcFalse; }

	*outItemID = menu_items[i].item_data.id;
	return UUcTrue;
}

UUtBool
WMrMenu_FindItemByText(
	WMtMenu					*inMenu,
	char					*inText,
	UUtUns16				*outItemID)
{
	UUtBool result;

	result = WMrMenu_FindItemByText_Internal(inMenu, inText, outItemID, WMcMenu_FindItemByText_ObeyCase);

	return result;
}

UUtBool
WMrMenu_FindItemByText_NoCase(
	WMtMenu					*inMenu,
	char					*inText,
	UUtUns16				*outItemID)
{
	UUtBool result;

	result = WMrMenu_FindItemByText_Internal(inMenu, inText, outItemID, WMcMenu_FindItemByText_IgnoreCase);

	return result;
}


// ----------------------------------------------------------------------
UUtBool
WMrMenu_GetItemFlags(
	WMtMenu					*inMenu,
	UUtUns16				inItemID,
	UUtUns16				*outFlags)
{
	WMtMenu_PrivateData		*private_data;
	WMtMenuItem				*menu_items;
	UUtUns32				i;

	UUmAssert(inMenu);

	*outFlags = WMcMenuItemFlag_None;

	// get the private data
	private_data = (WMtMenu_PrivateData*)WMrWindow_GetLong(inMenu, 0);
	if (private_data == NULL) { return UUcFalse; }
	if (private_data->menu_items == NULL) { return UUcFalse; }

	// get the array pointer
	menu_items = (WMtMenuItem*)UUrMemory_Array_GetMemory(private_data->menu_items);
	if (menu_items == NULL) { return UUcFalse; }

	// find the menu item with item id
	for (i = 0; i < UUrMemory_Array_GetUsedElems(private_data->menu_items); i++)
	{
		if (menu_items[i].item_data.id == inItemID) { break; }
	}
	if (i == UUrMemory_Array_GetUsedElems(private_data->menu_items)) { return UUcFalse; }

	// set outFlags
	*outFlags = menu_items[i].item_data.flags;

	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtBool
WMrMenu_GetItemID(
	WMtMenu					*inMenu,
	UUtInt16				inItemIndex,
	UUtUns16				*outID)
{
	WMtMenu_PrivateData		*private_data;
	WMtMenuItem				*menu_items;

	UUmAssert(inMenu);

	*outID = 0;

	// get the private data
	private_data = (WMtMenu_PrivateData*)WMrWindow_GetLong(inMenu, 0);
	if (private_data == NULL) { return UUcFalse; }
	if (private_data->menu_items == NULL) { return UUcFalse; }

	// make sure item number is in range
	if ((inItemIndex < 0) || (inItemIndex >= (UUtInt16)UUrMemory_Array_GetUsedElems(private_data->menu_items)))
	{
		return UUcFalse;
	}

	// get the array pointer
	menu_items = (WMtMenuItem*)UUrMemory_Array_GetMemory(private_data->menu_items);
	if (menu_items == NULL) { return UUcFalse; }

	// set the id
	*outID = menu_items[inItemIndex].item_data.id;

	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtBool
WMrMenu_GetItemText(
	WMtMenu					*inMenu,
	UUtUns16				inItemID,
	char					*outText)
{
	WMtMenu_PrivateData		*private_data;
	WMtMenuItem				*menu_items;
	UUtUns32				i;

	UUmAssert(inMenu);
	UUmAssert(outText);

	outText[0] = '\0';

	// get the private data
	private_data = (WMtMenu_PrivateData*)WMrWindow_GetLong(inMenu, 0);
	if (private_data == NULL) { return UUcFalse; }
	if (private_data->menu_items == NULL) { return UUcFalse; }

	// get the array pointer
	menu_items = (WMtMenuItem*)UUrMemory_Array_GetMemory(private_data->menu_items);
	if (menu_items == NULL) { return UUcFalse; }

	// find the menu item with item id
	for (i = 0; i < UUrMemory_Array_GetUsedElems(private_data->menu_items); i++)
	{
		if (menu_items[i].item_data.id == inItemID) { break; }
	}
	if (i == UUrMemory_Array_GetUsedElems(private_data->menu_items)) { return UUcFalse; }

	// copy the title
	UUrString_Copy(
		outText,
		menu_items[i].item_data.title,
		WMcMaxTitleLength);

	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtError
WMrMenu_Initialize(
	void)
{
	UUtError				error;
	WMtWindowClass			window_class;

	// register the window class
	UUrMemory_Clear(&window_class, sizeof(WMtWindowClass));
	window_class.type = WMcWindowType_Menu;
	window_class.callback = WMiMenu_Callback;
	window_class.private_data_size = sizeof(WMtMenu_PrivateData*);

	error = WMrWindowClass_Register(&window_class);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
WMrMenu_InsertItem(
	WMtMenu					*inMenu,
	WMtMenuItemData			*inMenuItemData,
	UUtInt16				inPosition)
{
	UUtError				error;
	WMtMenu_PrivateData		*private_data;
	PStPartSpecUI			*partspec_ui;
	WMtMenuItem				*menu_items;
	UUtUns32				index;
	UUtBool					mem_moved;
	UUtInt16				border_width;
	UUtInt16				border_height;

	// get the private data
	private_data = (WMtMenu_PrivateData*)WMrWindow_GetLong(inMenu, 0);
	if (private_data == NULL) { return UUcError_Generic; }
	if (private_data->menu_items == NULL) { return UUcError_Generic; }

	// make sure inPosition is a valid number
	if ((inPosition < -1) || (inPosition > (UUtInt16)UUrMemory_Array_GetUsedElems(private_data->menu_items)))
	{
		return UUcError_Generic;
	}

	// get the partspec ui
	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) { return UUcError_Generic; }

	// get the width and height of the border
	PSrPartSpec_GetSize(partspec_ui->background, PScPart_LeftTop, &border_width, &border_height);

	// insert the item at the specified position in the menu item list or
	// append it to the end
	index = WMcMenu_NoItem;
	if (inPosition == -1)
	{
		// add the item to the array
		error =
			UUrMemory_Array_GetNewElement(
				private_data->menu_items,
				&index,
				&mem_moved);
		UUmError_ReturnOnError(error);
	}
	else
	{
		// set the index
		index = (UUtUns32)inPosition;

		// insert the item into the array
		error =
			UUrMemory_Array_InsertElement(
				private_data->menu_items,
				index,
				&mem_moved);
		UUmError_ReturnOnError(error);
	}

	// get the array pointer
	menu_items = (WMtMenuItem*)UUrMemory_Array_GetMemory(private_data->menu_items);
	if (menu_items == NULL) { goto cleanup; }

	// initialize the menu item
	menu_items[index].item_data		= *inMenuItemData;
	menu_items[index].line_width	= 0;
	menu_items[index].line_height	= 0;

	if (menu_items[index].item_data.flags & WMcMenuItemFlag_Divider)
	{
		UUtInt16				divider_height;

		// calculate the divider height
		PSrPartSpec_GetSize(partspec_ui->divider, PScPart_LeftTop, NULL, &divider_height);

		// calculate the height
		menu_items[index].line_height = divider_height + divider_height;
	}
	else
	{
		TStFontInfo			font_info;
		UUtRect				rect;

		// get the font info
		WMrWindow_GetFontInfo(inMenu, &font_info);
		DCrText_SetFontInfo(&font_info);

		// get the dimensions of the string
		DCrText_GetStringRect(menu_items[index].item_data.title, &rect);
		menu_items[index].line_width = rect.right - rect.left;
		menu_items[index].line_height = rect.bottom - rect.top;
	}

	// we must recalculate our layout before we can draw or accept mouse events
	private_data->recalc_layout = UUcTrue;

	UUrMemory_Block_VerifyList();

	return UUcError_None;

cleanup:
	if (index != WMcMenu_NoItem)
	{
		UUrMemory_Array_DeleteElement(private_data->menu_items, index);
	}

	return UUcError_Generic;
}

// ----------------------------------------------------------------------
UUtError
WMrMenu_RegisterTemplates(
	void)
{
	UUtError				error;

	error =
		TMrTemplate_Register(
			WMcTemplate_MenuData,
			sizeof(WMtMenuData),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
WMrMenu_RemoveItem(
	WMtMenu					*inMenu,
	UUtUns16				inItemID)
{
	WMtMenu_PrivateData		*private_data;
	WMtMenuItem				*menu_items;
	UUtUns32				i;

	// get the private data
	private_data = (WMtMenu_PrivateData*)WMrWindow_GetLong(inMenu, 0);
	if (private_data == NULL) { return UUcError_Generic; }
	if (private_data->menu_items == NULL) { return UUcError_Generic; }

	// get the array pointer
	menu_items = (WMtMenuItem*)UUrMemory_Array_GetMemory(private_data->menu_items);
	if (menu_items == NULL) { return UUcError_Generic; }

	// find the specified item by id
	for (i = 0; i < UUrMemory_Array_GetUsedElems(private_data->menu_items); i++)
	{
		if (menu_items[i].item_data.id == inItemID) { break; }
	}
	if (i == UUrMemory_Array_GetUsedElems(private_data->menu_items)) { return UUcError_None; }

	// we must recalculate our layout before we can draw or accept mouse events
	private_data->recalc_layout = UUcTrue;

	// remove the specified item
	UUrMemory_Array_DeleteElement(private_data->menu_items, i);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
WMrMenu_Reset(
	WMtMenu					*inMenu)
{
	WMtMenu_PrivateData		*private_data;

	// get the private data
	private_data = (WMtMenu_PrivateData*)WMrWindow_GetLong(inMenu, 0);
	if (private_data == NULL) { return UUcError_Generic; }
	if (private_data->menu_items == NULL) { return UUcError_Generic; }

	// remove all of the menu items
	UUrMemory_Array_SetUsedElems(private_data->menu_items, 0, NULL);

	// we must recalculate our layout before we can draw or accept mouse events
	private_data->recalc_layout = UUcTrue;

	return UUcError_None;
}
