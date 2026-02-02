// ======================================================================
// WM_ListBox.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_WindowManager.h"
#include "BFW_TextSystem.h"
#include "BFW_LocalInput.h"
#include "BFW_Timer.h"

#include "WM_ListBox.h"
#include "WM_Scrollbar.h"

#include <ctype.h>

// ======================================================================
// defines
// ======================================================================
#define LBcDefaultNumItems			32
#define LBcDefaultCharsPerItem		127

#define LBcVScrollbarID				10
#define LBcHScrollbarID				11

#define LBcDefaultTextOffset_X		3

#define LBcSelected					0xFFFFFFFF

#define LBcKeyDownDelta				(45)		/* in ticks */

// ======================================================================
// typedefs
// ======================================================================
typedef struct WMtListBox_PrivateData
{
	UUtUns16				mouse_state;
	UUtBool					double_click;
	UUtBool					has_focus;

	UUtUns32				key_down_delta;
	char					prefix_buffer[LBcDefaultCharsPerItem];

	// item data
	UUtMemory_Array			*item_array;
	UUtUns32				num_visible_items;

	UUtInt16				item_width;
	UUtInt16				item_height;

	UUtUns32				start_index;	/* index of first item displayed */

	// scrollbar data
	WMtScrollbar			*v_scrollbar;
	UUtInt32				v_scroll_max;
	UUtInt32				v_scroll_pos;

	float					start_ratio;	/* used to convert from v_scroll_pos to start_index */

} WMtListBox_PrivateData;

// ----------------------------------------------------------------------
typedef struct WMtListBox_Item
{
	char					*string;
	UUtUns32				data;

	UUtInt16				width;
	UUtInt16				height;

	UUtBool					selected;

} WMtListBox_Item;

// ======================================================================
// prototypes
// ======================================================================
static void
WMiListBox_HandleFontInfoChanged(
	WMtListBox				*inListBox);

static UUtUns32
WMiListBox_GetSelection(
	WMtListBox				*inListBox);

static UUtUns32
WMiListBox_InsertString(
	WMtListBox				*inListBox,
	UUtUns32				inItemIndex,
	UUtUns32				inParam1);

static UUtUns32
WMiListBox_Reset(
	WMtListBox				*inListBox);

static UUtUns32
WMiListBox_SetItemData(
	WMtListBox				*inListBox,
	UUtUns32				inItemIndex,
	UUtUns32				inParam1);

static UUtUns32
WMiListBox_SetSelection(
	WMtListBox				*inListBox,
	UUtUns32				inItemIndex,
	UUtBool					inNotifyParent);

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
WMiListBox_AdjustScrollbar(
	WMtListBox				*inListBox,
	WMtListBox_PrivateData	*inPrivateData)
{
	UUtUns32				num_items;

	num_items = UUrMemory_Array_GetUsedElems(inPrivateData->item_array);

	// make sure the start_index is the proper distance from the last item
	if ((num_items > inPrivateData->num_visible_items) &&
		((inPrivateData->start_index + inPrivateData->num_visible_items) > num_items))
	{
		inPrivateData->start_index =  num_items - inPrivateData->num_visible_items;
		UUmAssert((UUtInt32)inPrivateData->start_index >= 0);
	}

	// calculate the scroll max
	inPrivateData->v_scroll_max = UUmMax(0, num_items);

	// calculate the scroll position
	inPrivateData->v_scroll_pos =
		(UUtInt32)((float)inPrivateData->start_index *
		((float)num_items / (float)inPrivateData->num_visible_items));

	// set the scrollbar range and position
	WMrScrollbar_SetRange(
		inPrivateData->v_scrollbar,
		0,
		inPrivateData->v_scroll_max,
		inPrivateData->num_visible_items);
	WMrScrollbar_SetPosition(inPrivateData->v_scrollbar, inPrivateData->v_scroll_pos);

	// calculate the start ratio
	inPrivateData->start_ratio =
		((float)num_items - (float)inPrivateData->num_visible_items) /
		(float)num_items;
	inPrivateData->start_ratio = UUmMax(0.0f, inPrivateData->start_ratio);
}

// ----------------------------------------------------------------------
static UUtUns32
WMiListBox_FindInsertPos(
	WMtListBox				*inListBox,
	WMtListBox_PrivateData	*inPrivateData,
	UUtUns32				inParam1,
	UUtInt32				*outInsertIndex)
{
	WMtListBox_PrivateData	*private_data;
	UUtUns32				style;
	WMtListBox_Item			*items;
	UUtUns32				i;
	UUtUns32				num_items;

	// get the private data
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return LBcError; }

	style = WMrWindow_GetStyle(inListBox);

	// get the array pointer
	items = (WMtListBox_Item*)UUrMemory_Array_GetMemory(private_data->item_array);
	if (items == NULL)
	{
		// there are no items to compare against so insert into the first position
		*outInsertIndex = -1;
		return LBcNoError;
	}

	num_items = UUrMemory_Array_GetUsedElems(private_data->item_array);
	for (i = 0; i <  num_items; i++)
	{
		UUtInt32				result;

		// compare the items based on the style of the listbox
		if (style & WMcListBoxStyle_HasStrings)
		{
			result = (UUtInt32)strcmp((char*)inParam1, items[i].string);
		}
		else if (style & WMcListBoxStyle_OwnerDraw)
		{
			WMtCompareItems		compare_items;

			compare_items.window = inListBox;
			compare_items.window_id = WMrWindow_GetID(inListBox);
			compare_items.item1_index = (UUtUns32)-1;
			compare_items.item1_data = inParam1;
			compare_items.item2_index = i;
			compare_items.item2_data = items[i].data;

			result =
				(UUtInt8)WMrMessage_Send(
					WMrWindow_GetOwner(inListBox),
					WMcMessage_CompareItems,
					(UUtUns32)compare_items.window_id,
					(UUtUns32)&compare_items);
		}
		else
		{
			return LBcError;
		}

		// stop if inParam1 goes before the item
		if (result < 0) { break; }
	}

	*outInsertIndex = i;

	return LBcNoError;
}

// ----------------------------------------------------------------------
static void
WMiListBox_FindPrefix(
	WMtListBox				*inListBox,
	char					*inPrefix)
{
	WMtListBox_PrivateData	*private_data;
	WMtListBox_Item			*items;
	UUtUns32				style;
	UUtUns32				i;

	// get the private data
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return; }

	style = WMrWindow_GetStyle(inListBox);
	if ((style & WMcListBoxStyle_HasStrings) == 0) { return; }

	// get the array pointer
	items = (WMtListBox_Item*)UUrMemory_Array_GetMemory(private_data->item_array);
	for (i = 0; i < UUrMemory_Array_GetUsedElems(private_data->item_array); i++)
	{
		if (UUrString_Compare_NoCase((char*)items[i].string, inPrefix) >= 0)
		{
			WMiListBox_SetSelection(inListBox, i, UUcTrue);
			break;
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
WMiListBox_NC_Create(
	WMtListBox				*inListBox)
{
	WMtListBox_PrivateData	*private_data;

	// create the private data
	private_data =
		(WMtListBox_PrivateData*)UUrMemory_Block_NewClear(sizeof(WMtListBox_PrivateData));
	if (private_data == NULL) { return WMcResult_Error; }

	// save the private data
	WMrWindow_SetLong(inListBox, 0, (UUtUns32)private_data);

	return WMcResult_Handled;
}

// ----------------------------------------------------------------------
static void
WMiListBox_NC_Destroy(
	WMtListBox				*inListBox)
{
	WMtListBox_PrivateData	*private_data;

	// get the private data
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return; }

	UUrMemory_Block_Delete(private_data);
	WMrWindow_SetLong(inListBox, 0, 0);
}

// ----------------------------------------------------------------------
static void
WMiListBox_NC_Paint(
	WMtListBox				*inListBox)
{
	DCtDrawContext			*draw_context;
	PStPartSpecUI			*partspec_ui;
	IMtPoint2D				dest;
	UUtInt16				width;
	UUtInt16				height;

	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) { return; }

	WMrWindow_GetSize(inListBox, &width, &height);

	draw_context = DCrDraw_NC_Begin(inListBox);

	// set the dest
	dest.x = 0;
	dest.y = 0;

	// draw the outline
	DCrDraw_PartSpec(
		draw_context,
		partspec_ui->editfield,
		PScPart_All,
		&dest,
		width,
		height,
		M3cMaxAlpha);

	DCrDraw_NC_End(draw_context, inListBox);
}

// ----------------------------------------------------------------------
static void
WMiListBox_NC_CalcClientSize(
	WMtListBox				*inListBox,
	UUtRect					*outRect)
{
	PStPartSpecUI			*partspec_ui;
	UUtInt16				window_width;
	UUtInt16				window_height;
	UUtInt16				part_width;
	UUtInt16				part_height;

	WMrWindow_GetSize(inListBox, &window_width, &window_height);

	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) { return; }

	PSrPartSpec_GetSize(
		partspec_ui->editfield,
		PScPart_LeftTop,
		&part_width,
		&part_height);

	outRect->left	= part_width;
	outRect->top	= part_height;

	PSrPartSpec_GetSize(
		partspec_ui->editfield,
		PScPart_RightBottom,
		&part_width,
		&part_height);

	outRect->right	= window_width - part_width;
	outRect->bottom	= window_height - part_height;
}

// ----------------------------------------------------------------------
static UUtUns32
WMiListBox_NC_HitTest(
	WMtListBox				*inListBox,
	UUtUns32				inParam1)
{
	UUtRect					bounds;
	IMtPoint2D				global_mouse;
	IMtPoint2D				local_mouse;
	WMtWindowArea			part;

	part = WMcWindowArea_None;

	global_mouse.x = (UUtInt16)UUmHighWord(inParam1);
	global_mouse.y = (UUtInt16)UUmLowWord(inParam1);

	WMrWindow_GlobalToLocal(inListBox, &global_mouse, &local_mouse);

	WMrWindow_GetClientRect(inListBox, &bounds);
	if (IMrRect_PointIn(&bounds, &local_mouse))
	{
		part = WMcWindowArea_Client;
	}

	return (UUtUns32)part;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
WMiListBox_Create(
	WMtListBox				*inListBox)
{
	WMtListBox_PrivateData	*private_data;
	UUtUns32				style;

	// ------------------------------
	// create the private data
	// ------------------------------
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { goto cleanup; }

	// ------------------------------
	// initialize the item array
	// ------------------------------
	// allocate the text array
	private_data->item_array =
		UUrMemory_Array_New(
			sizeof(WMtListBox_Item),
			LBcDefaultNumItems,
			0,
			LBcDefaultNumItems);
	if (private_data->item_array == NULL) { goto cleanup; }

	// ------------------------------
	// create the scrollbars
	// ------------------------------
	style = WMrWindow_GetStyle(inListBox);
	if (style & WMcListBoxStyle_HasScrollbar)
	{
		private_data->v_scrollbar =
			WMrWindow_New(
				WMcWindowType_Scrollbar,
				"listbox_v_scrollbar",
				WMcWindowFlag_Visible | WMcWindowFlag_Child,
				WMcScrollbarStyle_Vertical,
				10,
				0,					// x
				-1,					// y
				-1,					// width
				16,					// height
				inListBox,
				0);
		if (private_data->v_scrollbar == NULL) { goto cleanup; }
	}

	// ------------------------------
	// calculate the sizes
	// ------------------------------
	WMiListBox_HandleFontInfoChanged(inListBox);

	return WMcResult_Handled;

cleanup:
	UUmAssert(0);

	if (private_data)
	{
		if (private_data->item_array)
		{
			UUrMemory_Array_Delete(private_data->item_array);
			private_data->item_array = NULL;
		}

		UUrMemory_Block_Delete(private_data);
		private_data = NULL;
		WMrWindow_SetLong(inListBox, 0, 0);
	}

	return WMcResult_Error;
}
/*
static UUtUns32
WMiListBox_Create(
	WMtListBox				*inListBox)
{
	UUtRect					client_bounds;
	UUtInt16				width;
	UUtInt16				height;
	WMtListBox_PrivateData	*private_data;
	UUtInt16				scrollbar_width;
	UUtUns32				style;
	UUtRect					window_bounds;
	UUtInt16				window_width;
	UUtInt16				window_height;
	UUtInt16				diff;
	TStFontInfo				font_info;

	// ------------------------------
	// create the private data
	// ------------------------------
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { goto cleanup; }

	// ------------------------------
	// initialize
	// ------------------------------
	// get the client bounds
	WMrWindow_GetClientRect(inListBox, &client_bounds);
	width = client_bounds.right - client_bounds.left;
	height = client_bounds.bottom - client_bounds.top;

	WMrWindow_GetFontInfo(inListBox, &font_info);
	DCrText_SetFontInfo(&font_info);

	private_data->item_width = width;
	private_data->item_height = (UUtInt16)DCrText_GetLineHeight();

	private_data->start_index = 0;

	// initialize the private data
	private_data->double_click = UUcFalse;

	private_data->num_visible_items = height / private_data->item_height;

	private_data->v_scroll_pos = 0;

	// adjust the height of the listbox
	WMrWindow_GetRect(inListBox, &window_bounds);
	window_width = window_bounds.right - window_bounds.left;
	window_height = window_bounds.bottom - window_bounds.top;
	diff = window_height - height;
	window_height = (private_data->item_height * (UUtInt16)private_data->num_visible_items) + diff;
	WMrWindow_SetSize(inListBox, window_width, window_height);

	// get the client bounds again
	WMrWindow_GetClientRect(inListBox, &client_bounds);
	width = client_bounds.right - client_bounds.left;
	height = client_bounds.bottom - client_bounds.top;

	// ------------------------------
	// initialize the item array
	// ------------------------------
	// allocate the text array
	private_data->item_array =
		UUrMemory_Array_New(
			sizeof(WMtListBox_Item),
			LBcDefaultNumItems,
			0,
			LBcDefaultNumItems);
	if (private_data->item_array == NULL) { goto cleanup; }

	// ------------------------------
	// create the scrollbars
	// ------------------------------
	style = WMrWindow_GetStyle(inListBox);
	if (style & WMcListBoxStyle_HasScrollbar)
	{
		private_data->v_scrollbar =
			WMrWindow_New(
				WMcWindowType_Scrollbar,
				"listbox_v_scrollbar",
				WMcWindowFlag_Visible | WMcWindowFlag_Child,
				WMcScrollbarStyle_Vertical,
				10,
				0,					// x
				-1,					// y
				-1,					// width
				height + 2,			// height
				inListBox,
				0);
		if (private_data->v_scrollbar == NULL) { goto cleanup; }

		// set the location of the v_scrollbar within the listbox
		WMrWindow_GetSize(private_data->v_scrollbar, &scrollbar_width, NULL);
		WMrWindow_SetLocation(private_data->v_scrollbar, width - scrollbar_width + 1, -1);

		// adjust the scrollbar
		WMiListBox_AdjustScrollbar(inListBox, private_data);

		// adjust the item_width
		private_data->item_width -= scrollbar_width - 1;
	}

	return WMcResult_Handled;

cleanup:
	UUmAssert(0);

	if (private_data)
	{
		if (private_data->item_array)
		{
			UUrMemory_Array_Delete(private_data->item_array);
			private_data->item_array = NULL;
		}

		UUrMemory_Block_Delete(private_data);
		private_data = NULL;
		WMrWindow_SetLong(inListBox, 0, 0);
	}

	return WMcResult_Error;
}
*/
// ----------------------------------------------------------------------
static void
WMiListBox_Destroy(
	WMtListBox				*inListBox)
{
	WMtListBox_PrivateData	*private_data;

	// get the private data
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return; }

	// reset the contents
	WMiListBox_Reset(inListBox);

	// delete the item array
	if (private_data->item_array)
	{
		UUrMemory_Array_Delete(private_data->item_array);
		private_data->item_array = NULL;
	}
}

// ----------------------------------------------------------------------
static void
WMiListBox_Focus(
	WMtListBox				*inListBox,
	WMtMessage				inMessage)
{
	WMtListBox_PrivateData	*private_data;

	// get the private data
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return; }

	if (inMessage == WMcMessage_SetFocus)
	{
		private_data->has_focus = UUcTrue;
	}
	else
	{
		private_data->has_focus = UUcFalse;
	}
}

// ----------------------------------------------------------------------
static void
WMiListBox_HandleFontInfoChanged(
	WMtListBox				*inListBox)
{
	UUtRect					client_bounds;
	UUtInt16				width;
	UUtInt16				height;
	WMtListBox_PrivateData	*private_data;
	UUtInt16				scrollbar_width;
	UUtUns32				style;
	UUtRect					window_bounds;
	UUtInt16				window_width;
	UUtInt16				window_height;
	UUtInt16				diff;
	TStFontInfo				font_info;
	WMtListBox_Item			*items;
	UUtUns32				num_items;
	UUtUns32				i;

	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return; }

	// ------------------------------
	// setup the window size based on the font info
	// ------------------------------
	// get the client bounds
	WMrWindow_GetClientRect(inListBox, &client_bounds);
	width = client_bounds.right - client_bounds.left;
	height = client_bounds.bottom - client_bounds.top;

	WMrWindow_GetFontInfo(inListBox, &font_info);
	DCrText_SetFontInfo(&font_info);

	private_data->item_width = width;
	private_data->item_height = (UUtInt16)DCrText_GetLineHeight();

	private_data->start_index = 0;

	// initialize the private data
	private_data->double_click = UUcFalse;

	private_data->num_visible_items = height / private_data->item_height;

	private_data->v_scroll_pos = 0;

	// adjust the height of the listbox
	WMrWindow_GetRect(inListBox, &window_bounds);
	window_width = window_bounds.right - window_bounds.left;
	window_height = window_bounds.bottom - window_bounds.top;
	diff = window_height - height;
	window_height = (private_data->item_height * (UUtInt16)private_data->num_visible_items) + diff;
	WMrWindow_SetSize(inListBox, window_width, window_height);

	// get the client bounds
	WMrWindow_GetClientRect(inListBox, &client_bounds);
	height = client_bounds.bottom - client_bounds.top;

	// ------------------------------
	// adjust the scrollbar
	// ------------------------------
	style = WMrWindow_GetStyle(inListBox);
	if (style & WMcListBoxStyle_HasScrollbar)
	{
		// set the location of the v_scrollbar within the listbox
		WMrWindow_GetSize(private_data->v_scrollbar, &scrollbar_width, NULL);
		WMrWindow_SetLocation(private_data->v_scrollbar, width - scrollbar_width + 1, -1);

		WMrWindow_GetSize(private_data->v_scrollbar, &width, NULL);
		WMrWindow_SetSize(private_data->v_scrollbar, width, height + 2);

		// adjust the scrollbar
		WMiListBox_AdjustScrollbar(inListBox, private_data);

		// adjust the item_width
		private_data->item_width -= scrollbar_width - 1;
	}

	// ------------------------------
	// adjust the height of each item
	// ------------------------------
	// get the array pointer
	items = (WMtListBox_Item*)UUrMemory_Array_GetMemory(private_data->item_array);
	num_items = UUrMemory_Array_GetUsedElems(private_data->item_array);
	for (i = 0; i < num_items; i++)
	{
		items[i].width = private_data->item_width;
		items[i].height = private_data->item_height;
	}
}

// ----------------------------------------------------------------------
static void
WMiListBox_HandleKeyEvent(
	WMtListBox				*inListBox,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	WMtListBox_PrivateData	*private_data;

	// get the private data
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return; }

	switch (inMessage)
	{
		case WMcMessage_KeyDown:
		{
			UUtUns32		index;
			UUtUns32		num_items;

			index = WMiListBox_GetSelection(inListBox);
			num_items = UUrMemory_Array_GetUsedElems(private_data->item_array);

			switch (inParam1)
			{
				case LIcKeyCode_UpArrow:
					index--;
					if (index > num_items) { index = 0; }
					WMiListBox_SetSelection(inListBox, index, UUcTrue);
				break;

				case LIcKeyCode_DownArrow:
					index++;
					if (index >= num_items) { index = num_items - 1; }
					WMiListBox_SetSelection(inListBox, index, UUcTrue);
				break;

				case LIcKeyCode_Home:
					WMiListBox_SetSelection(inListBox, 0, UUcTrue);
				break;

				case LIcKeyCode_End:
					WMiListBox_SetSelection(inListBox, (num_items - 1), UUcTrue);
				break;

				case LIcKeyCode_PageUp:
					index -= private_data->num_visible_items;
					if (index > num_items) { index = 0; }
					WMiListBox_SetSelection(inListBox, index, UUcTrue);
				break;

				case LIcKeyCode_PageDown:
					index += private_data->num_visible_items;
					if (index >= num_items) { index = num_items - 1; }
					WMiListBox_SetSelection(inListBox, index, UUcTrue);
				break;

				default:
				{
					char			char_string[2];

					if (private_data->key_down_delta < UUrMachineTime_Sixtieths())
					{
						// reset the buffer
						private_data->prefix_buffer[0] = '\0';
					}

					// concatenate inParam1 to the end of the buffer
					char_string[0] = (char)inParam1;
					char_string[1] = '\0';

					UUrString_Cat(
						private_data->prefix_buffer,
						char_string,
						LBcDefaultCharsPerItem);

					// search for that file
					WMiListBox_FindPrefix(inListBox, private_data->prefix_buffer);
				}
				break;
			}

			private_data->key_down_delta = UUrMachineTime_Sixtieths() + LBcKeyDownDelta;
		}
		break;
	}
}

// ----------------------------------------------------------------------
static void
WMiListBox_HandleMouseEvent(
	WMtListBox				*inListBox,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	WMtListBox_PrivateData	*private_data;
	IMtPoint2D				global_mouse;
	IMtPoint2D				local_mouse;

	// disabled list boxes don't handle mouse events
	if (WMrWindow_GetEnabled(inListBox) == UUcFalse) { return; }

	// get the private data
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return; }

	// get the local mouse position
	global_mouse.x = (UUtInt16)UUmHighWord(inParam1);
	global_mouse.y = (UUtInt16)UUmLowWord(inParam1);
	WMrWindow_GlobalToLocal(inListBox, &global_mouse, &local_mouse);

	// handle the message
	switch (inMessage)
	{
		case WMcMessage_LMouseDown:
			WMrWindow_SetFocus(inListBox);
			private_data->mouse_state = LIcMouseState_LButtonDown;
			private_data->double_click = UUcFalse;
		break;

		case WMcMessage_LMouseDblClck:
			private_data->double_click = UUcTrue;
		break;

		case WMcMessage_LMouseUp:
		{
			UUtInt32			i;
			UUtInt32			start_index;
			UUtInt32			stop_index;
			UUtRect				bounds;
			WMtListBox_Item		*items;

			// make sure the mouse went down in the list box
			if (private_data->mouse_state != LIcMouseState_LButtonDown) { break; }

			private_data->mouse_state &= ~LIcMouseState_LButtonDown;

			// get the array pointer
			items = (WMtListBox_Item*)UUrMemory_Array_GetMemory(private_data->item_array);

			// calculate the start and stop index
			start_index = private_data->start_index;
			stop_index =
				UUmMin(
					UUrMemory_Array_GetUsedElems(private_data->item_array),
					(private_data->start_index + private_data->num_visible_items));

			bounds.left = 0;
			bounds.top = 0;
			bounds.right = 0;
			bounds.bottom = 0;

			for (i = start_index; i < stop_index; i++)
			{
				bounds.right = items[i].width;
				bounds.bottom = bounds.top + items[i].height;

				if (IMrRect_PointIn(&bounds, &local_mouse))
				{
					WMiListBox_SetSelection(inListBox, i, UUcTrue);
					break;
				}

				bounds.top += items[i].height;
			}

			if ((i != stop_index) && (private_data->double_click))
			{
				// tell the parent
				WMrMessage_Send(
					WMrWindow_GetParent(inListBox),
					WMcMessage_Command,
					UUmMakeLong(WMcNotify_DoubleClick, WMrWindow_GetID(inListBox)),
					(UUtUns32)inListBox);
			}
		}
		break;
	}
}

// ----------------------------------------------------------------------
static void
WMiListBox_HandleVerticalScroll(
	WMtListBox				*inListBox,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtInt32				scroll_increment;
	WMtListBox_PrivateData	*private_data;
	UUtInt32				client_height;
	UUtRect					client_rect;

	// get the private data
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return; }

	// calculate the client height
	WMrWindow_GetClientRect(inListBox, &client_rect);
	client_height = client_rect.bottom - client_rect.top;

	// interpret the parameters of the message
	switch (UUmHighWord(inParam1))
	{
		case SBcNotify_LineUp:
			scroll_increment = -1;
		break;

		case SBcNotify_LineDown:
			scroll_increment = 1;
		break;

		case SBcNotify_PageUp:
			scroll_increment =
				UUmMin(
					-1,
					-client_height / private_data->item_height);
		break;

		case SBcNotify_PageDown:
			scroll_increment =
				UUmMax(
					1,
					client_height / private_data->item_height);
		break;

		case SBcNotify_ThumbPosition:
			scroll_increment = inParam2 - private_data->v_scroll_pos;
		break;

		default:
			scroll_increment = 0;
		break;
	}

	// calculate the increment
	scroll_increment =
		UUmMax(
			-private_data->v_scroll_pos,
			UUmMin(
				scroll_increment,
				private_data->v_scroll_max - private_data->v_scroll_pos));

	// adjust the the thumb position
	if (scroll_increment != 0)
	{
		private_data->v_scroll_pos += scroll_increment;
		private_data->start_index =
			(UUtInt32)(((float)private_data->v_scroll_pos * private_data->start_ratio) + 0.5f);
		UUmAssert((UUtInt32)private_data->start_index >= 0);
		WMrScrollbar_SetPosition(private_data->v_scrollbar, private_data->v_scroll_pos);
	}
}

// ----------------------------------------------------------------------
static void
WMiListBox_Paint(
	WMtListBox				*inListBox)
{
	WMtListBox_PrivateData	*private_data;
	DCtDrawContext			*draw_context;
	PStPartSpecUI			*partspec_ui;
	IMtPoint2D				dest;
	UUtRect					bounds;
	WMtListBox_Item			*items;
	UUtInt32				i;
	UUtInt32				start_index;
	UUtInt32				stop_index;
	UUtInt16				draw_line;
	UUtUns16				ascending_height;
	UUtBool					enabled;
	UUtUns32				style;
	TStFontInfo				font_info;

	// get the private data
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return; }

	// get the array pointer
	items = (WMtListBox_Item*)UUrMemory_Array_GetMemory(private_data->item_array);

	// get the partspec ui
	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) { return; }

	style = WMrWindow_GetStyle(inListBox);

	WMrWindow_GetFontInfo(inListBox, &font_info);
	DCrText_SetFontInfo(&font_info);

	ascending_height = DCrText_GetAscendingHeight();

	draw_context = DCrDraw_Begin(inListBox);

	enabled = WMrWindow_GetEnabled(inListBox);

	// set the dest
	dest.x = 0;
	dest.y = 0;

	WMrWindow_GetClientRect(inListBox, &bounds);

	// calculate the start and stop index
	start_index = (UUtInt32) private_data->start_index;
	start_index = UUmMax(0, start_index);
	stop_index =
		UUmMin(
			UUrMemory_Array_GetUsedElems(private_data->item_array),
			(private_data->start_index + private_data->num_visible_items));

	// draw the contents
	for (i = start_index, draw_line = 0; i < stop_index; i++, draw_line++)
	{
		if (enabled && (items[i].selected))
		{
			DCrDraw_PartSpec(
				draw_context,
				private_data->has_focus ? partspec_ui->hilite : partspec_ui->background,
				PScPart_All,
				&dest,
				items[i].width,
				items[i].height,
				M3cMaxAlpha);
		}

		if (style & WMcListBoxStyle_OwnerDraw)
		{
			WMtDrawItem					draw_item;

			// set up the owner draw struct
			draw_item.draw_context		= draw_context;
			draw_item.window			= inListBox;
			draw_item.window_id			= WMrWindow_GetID(inListBox);
			draw_item.state				= WMcDrawItemState_None;
			draw_item.item_id 			= i;
			draw_item.rect.left			= dest.x;
			draw_item.rect.top			= dest.y;
			draw_item.rect.right		= dest.x + items[i].width;
			draw_item.rect.bottom		= dest.y + items[i].height;
			draw_item.string			= items[i].string;
			draw_item.data				= items[i].data;

			if (items[i].selected)
			{
				draw_item.state = WMcDrawItemState_Selected;
			}

			WMrMessage_Send(
				WMrWindow_GetOwner(inListBox),
				WMcMessage_DrawItem,
				(UUtUns32)draw_item.window_id,
				(UUtUns32)&draw_item);
		}
		else if (style & WMcListBoxStyle_HasStrings)
		{
			IMtPoint2D				item_dest;

			item_dest = dest;

			if ((style & WMcListBoxStyle_Directory) == WMcListBoxStyle_Directory)
			{
				PStPartSpec			*icon;

				switch (items[i].data)
				{
					case LBcDirItemType_File:
						icon = partspec_ui->file;
					break;

					case LBcDirItemType_Directory:
						icon = partspec_ui->folder;
					break;

					case LBcDirItemType_Volume:
					break;
				}

				item_dest.x += 2;
				item_dest.y += 1;

				DCrDraw_PartSpec(
					draw_context,
					icon,
					PScPart_All,
					&item_dest,
					(items[i].height - 3),
					(items[i].height - 3),
					M3cMaxAlpha);

				item_dest.x += items[i].height;
				item_dest.y -= 1;
			}
			else
			{
				item_dest.x += LBcDefaultTextOffset_X;
			}

			item_dest.y += (UUtInt16)ascending_height;

			// always set these in case some per line text formatting needs to be done
			DCrText_SetShade(enabled ? font_info.font_shade : IMcShade_Gray50);
			DCrText_SetFormat(TSc_HLeft);

			DCrDraw_String(
				draw_context,
				(char*)items[i].string,
				NULL,
				&item_dest);
		}

		dest.y += items[i].height;
	}

	DCrDraw_End(draw_context, inListBox);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
WMiListBox_AddString(
	WMtListBox				*inListBox,
	UUtUns32				inParam1)
{
	WMtListBox_PrivateData	*private_data;
	UUtInt32				item_index;
	UUtUns32				style;

	// get the private data
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return LBcError; }

	item_index = -1;
	style = WMrWindow_GetStyle(inListBox);

	// sort the array
	if (style & WMcListBoxStyle_Sort)
	{
		UUtUns32			result;

		result = WMiListBox_FindInsertPos(inListBox, private_data, inParam1, &item_index);
		if (result != LBcNoError) { return result; }
	}

	return WMiListBox_InsertString(inListBox, item_index, inParam1);
}

// ----------------------------------------------------------------------
static UUtUns32
WMiListBox_DeleteString(
	WMtListBox				*inListBox,
	UUtUns32				inItemIndex)
{
	WMtListBox_PrivateData	*private_data;
	WMtListBox_Item			*items;
	UUtUns32				style;

	// get the private data
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return LBcError; }

	style = WMrWindow_GetStyle(inListBox);

	// check the range
	if ((inItemIndex < 0) ||
		(inItemIndex >= UUrMemory_Array_GetUsedElems(private_data->item_array)))
	{
		return LBcError;
	}

	// get the array pointer
	items = (WMtListBox_Item*)UUrMemory_Array_GetMemory(private_data->item_array);

	// delete the item data
	if ((style & WMcListBoxStyle_HasStrings) != 0)
	{
		UUrMemory_Block_Delete((char*)items[inItemIndex].string);
		items[inItemIndex].string = NULL;
	}

	// delete the item
	UUrMemory_Array_DeleteElement(private_data->item_array, inItemIndex);

	// adjust the scrollbar
	if (style & WMcListBoxStyle_HasScrollbar)
	{
		WMiListBox_AdjustScrollbar(inListBox, private_data);
	}

	return LBcNoError;
}

// ----------------------------------------------------------------------
static UUtUns32
WMiListBox_GetItemData(
	WMtListBox				*inListBox,
	UUtUns32				inItemIndex)
{
	WMtListBox_PrivateData	*private_data;
	WMtListBox_Item			*items;
	UUtUns32				index;
	UUtUns32				data;

	// get the private data
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return LBcError; }

	if (inItemIndex == LBcSelected)
	{
		index = WMiListBox_GetSelection(inListBox);
	}
	else
	{
		index = inItemIndex;
	}

	// check the bounds on the index
	if ((index < 0) ||
		(index >= UUrMemory_Array_GetUsedElems(private_data->item_array)))
	{
		return LBcError;
	}

	// get a pointer to the items array
	items = (WMtListBox_Item*)UUrMemory_Array_GetMemory(private_data->item_array);
	if (items == NULL) { return LBcError; }

	// get the data
	data = items[index].data;

	return data;
}

// ----------------------------------------------------------------------
static UUtUns32
WMiListBox_GetNumLines(
	WMtListBox				*inListBox)
{
	WMtListBox_PrivateData	*private_data;

	// get the private data
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return LBcError; }

	return UUrMemory_Array_GetUsedElems(private_data->item_array);
}

// ----------------------------------------------------------------------
static UUtUns32
WMiListBox_GetText(
	WMtListBox				*inListBox,
	UUtUns32				inItemIndex,
	char					*outString)
{
	WMtListBox_PrivateData	*private_data;
	UUtUns32				index;
	WMtListBox_Item			*items;
	UUtUns32				style;

	// get the private data
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return LBcError; }

	style = WMrWindow_GetStyle(inListBox);

	// set the index
	if (inItemIndex == LBcSelected)
	{
		index = WMiListBox_GetSelection(inListBox);
	}
	else
	{
		index = inItemIndex;
	}

	// check the bounds on the index
	if ((index < 0) ||
		(index >= UUrMemory_Array_GetUsedElems(private_data->item_array)))
	{
		return LBcError;
	}

	// get the array pointer
	items = (WMtListBox_Item*)UUrMemory_Array_GetMemory(private_data->item_array);

	// assume that outString is big enough to hold the contents of the array
	if ((style & WMcListBoxStyle_HasStrings) != 0)
	{
		UUrString_Copy_Length(
			outString,
			(char*)items[index].string,
			LBcDefaultCharsPerItem,
			TSrString_GetLength((char*)items[index].string));
	}
	else
	{
		*((UUtUns32*)outString) = items[index].data;
	}

	return LBcNoError;
}

// ----------------------------------------------------------------------
static UUtUns32
WMiListBox_GetTextLength(
	WMtListBox				*inListBox,
	UUtUns32				inItemIndex)
{
	WMtListBox_PrivateData	*private_data;
	UUtUns32				index;
	WMtListBox_Item			*items;
	UUtUns16				string_length;
	UUtUns32				style;

	// get the private data
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return LBcError; }

	style = WMrWindow_GetStyle(inListBox);
	if ((style & WMcListBoxStyle_HasStrings) == 0) { return LBcError; }

	// set the index
	if (inItemIndex == LBcSelected)
	{
		index = WMiListBox_GetSelection(inListBox);
	}
	else
	{
		index = inItemIndex;
	}

	// check the bounds on the index
	if ((index < 0) ||
		(index >= UUrMemory_Array_GetUsedElems(private_data->item_array)))
	{
		return LBcError;
	}

	// get the array pointer
	items = (WMtListBox_Item*)UUrMemory_Array_GetMemory(private_data->item_array);

	// get the length of the string
	string_length = TSrString_GetLength((char*)items[index].data);

	return string_length;
}

// ----------------------------------------------------------------------
static UUtUns32
WMiListBox_GetSelection(
	WMtListBox				*inListBox)
{
	WMtListBox_PrivateData	*private_data;
	WMtListBox_Item			*items;
	UUtUns32				i;
	UUtUns32				style;

	// get the private data
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return LBcError; }

	style = WMrWindow_GetStyle(inListBox);
	if (style & WMcListBoxStyle_MultipleSelection) { return LBcError; }

	// get the array pointer
	items = (WMtListBox_Item*)UUrMemory_Array_GetMemory(private_data->item_array);

	for (i = 0; i < UUrMemory_Array_GetUsedElems(private_data->item_array); i++)
	{
		if (items[i].selected)
		{
			return i;
		}
	}

	return LBcError;
}

// ----------------------------------------------------------------------
static UUtUns32
WMiListBox_InsertString(
	WMtListBox				*inListBox,
	UUtUns32				inItemIndex,
	UUtUns32				inParam1)
{
	WMtListBox_PrivateData	*private_data;
	UUtError				error;
	UUtBool					mem_moved;
	WMtListBox_Item			*items;
	UUtUns32				index;
	UUtUns32				style;

	// get the private data
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return LBcError; }

	style = WMrWindow_GetStyle(inListBox);

	// make room in the array
	if (inItemIndex != -1)
	{
		// insert the item into the array
		error =
			UUrMemory_Array_InsertElement(
				private_data->item_array,
				inItemIndex,
				&mem_moved);
		if (error != UUcError_None) { return LBcError; }

		index = inItemIndex;
	}
	else
	{
		// add the item to the array
		error =
			UUrMemory_Array_GetNewElement(
				private_data->item_array,
				(UUtUns32*)&index,
				&mem_moved);
		if (error != UUcError_None) { return LBcError; }
	}

	// get the array pointer
	items = (WMtListBox_Item*)UUrMemory_Array_GetMemory(private_data->item_array);

	// initialize the element
	items[index].data = 0;
	items[index].width = private_data->item_width;
	items[index].height = private_data->item_height;
	items[index].selected = UUcFalse;

	if ((style & WMcListBoxStyle_HasStrings) != 0)
	{
		items[index].string = (char*)UUrMemory_Block_New(LBcDefaultCharsPerItem + 1);
		if (items[index].string == NULL)
		{
			UUrMemory_Array_DeleteElement(private_data->item_array, index);
			return LBcError;
		}

		UUrString_Copy(
			(char*)items[index].string,
			(char*)inParam1,
			LBcDefaultCharsPerItem);
	}
	else
	{
		items[index].data = inParam1;
	}

	// adjust the scrollbar
	if (style & WMcListBoxStyle_HasScrollbar)
	{
		WMiListBox_AdjustScrollbar(inListBox, private_data);
	}

	return (UUtUns32)index;
}

// ----------------------------------------------------------------------
static UUtUns32
WMiListBox_ReplaceString(
	WMtListBox				*inListBox,
	UUtUns32				inItemIndex,
	UUtUns32				inParam1)
{
	WMtListBox_PrivateData	*private_data;
	WMtListBox_Item			*items;
	UUtUns32				style;

	// get the private data
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return LBcError; }

	style = WMrWindow_GetStyle(inListBox);

	// CB: check that we are within the bounds of the array
	if ((inItemIndex < 0) ||
		(inItemIndex >= UUrMemory_Array_GetUsedElems(private_data->item_array)))
	{
		return LBcError;
	}

	// get the array pointer
	items = (WMtListBox_Item*)UUrMemory_Array_GetMemory(private_data->item_array);

	// set the new item for the element
	if ((style & WMcListBoxStyle_HasStrings) != 0)
	{
		if (items[inItemIndex].string == NULL)
		{
			items[inItemIndex].string = (char*)UUrMemory_Block_New(LBcDefaultCharsPerItem + 1);
		}

		// copy the string into the item
		UUrString_Copy(
			(char*)items[inItemIndex].string,
			(char*)inParam1,
			LBcDefaultCharsPerItem);
	}
	else
	{
		items[inItemIndex].data = inParam1;
	}

	return (UUtUns32)inItemIndex;
}

// ----------------------------------------------------------------------
static UUtUns32
WMiListBox_Reset(
	WMtListBox				*inListBox)
{
	WMtListBox_PrivateData	*private_data;
	UUtUns32				num_items;
	WMtListBox_Item			*items;
	UUtUns32				style;

	// get the private data
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return LBcError; }

	style = WMrWindow_GetStyle(inListBox);

	// clear the array
	num_items = UUrMemory_Array_GetUsedElems(private_data->item_array);
	if (num_items == 0) { return LBcNoError; }

	// get the array pointer
	items = (WMtListBox_Item*)UUrMemory_Array_GetMemory(private_data->item_array);

	// delete all the items in the array
	while (num_items--)
	{
		if ((style & WMcListBoxStyle_HasStrings) != 0)
		{
			// delete the item string
			UUrMemory_Block_Delete((char*)items[num_items].string);
			items[num_items].string = NULL;
		}

		UUrMemory_Array_DeleteElement(private_data->item_array, num_items);
	}

	private_data->start_index = 0;
	private_data->start_ratio = 0.0f;

	// adjust the scrollbar
	if (style & WMcListBoxStyle_HasScrollbar)
	{
		WMiListBox_AdjustScrollbar(inListBox, private_data);
	}

	return LBcNoError;
}

// ----------------------------------------------------------------------
static UUtUns32
WMiListBox_SelectString(
	WMtListBox				*inListBox,
	UUtUns32				inItemIndex,
	char					*inString)
{
	WMtListBox_PrivateData	*private_data;
	WMtListBox_Item			*items;
	UUtUns32				style;
	UUtUns32				i;
	UUtUns32				start_index;
	UUtBool					found;

	// get the private data
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return LBcError; }

	style = WMrWindow_GetStyle(inListBox);
	if ((style & WMcListBoxStyle_HasStrings) == 0) { return LBcError; }

	// get the array pointer
	items = (WMtListBox_Item*)UUrMemory_Array_GetMemory(private_data->item_array);

	found = UUcFalse;
	start_index = inItemIndex;
	if (start_index == LBcSelected)
	{
		start_index = 0;
	}

	for (i = start_index; i < UUrMemory_Array_GetUsedElems(private_data->item_array); i++)
	{
		if (strcmp((char*)items[i].string, inString) == 0)
		{
			WMiListBox_SetSelection(inListBox, i, UUcTrue);
			found = UUcTrue;
			break;
		}
	}

	if (i == UUrMemory_Array_GetUsedElems(private_data->item_array))
	{
		for (i = 0; i < start_index; i++)
		{
			if (strcmp((char*)items[i].string, inString) == 0)
			{
				WMiListBox_SetSelection(inListBox, i, UUcTrue);
				found = UUcTrue;
				break;
			}
		}
	}

	if (!found)
	{
		// CB: we could not find the string. deselect.
		WMiListBox_SetSelection(inListBox, LBcSelected, UUcTrue);
	}

	return LBcNoError;
}

// ----------------------------------------------------------------------
static UUtUns32
WMiListBox_SetDirectoryInfo(
	WMtListBox				*inListBox,
	UUtUns32				inParam1,
	UUtBool					inReset)
{
	WMtListBox_PrivateData		*private_data;
	UUtUns32					style;
	WMtListBox_DirectoryInfo	*directory_info;
	UUtUns32					result;
	BFtFileIterator				*file_iterator;
	UUtError					error;

	// get the private data
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return LBcError; }

	style = WMrWindow_GetStyle(inListBox);
	if ((style & WMcListBoxStyle_Directory) != WMcListBoxStyle_Directory) { return LBcError; }

	// get a pointer to the directory info
	directory_info = (WMtListBox_DirectoryInfo*)inParam1;
	if (directory_info == NULL) { return LBcError; }

	// clear the listbox
	if (inReset)
	{
		result = WMiListBox_Reset(inListBox);
		if (result == LBcError) { return LBcError; }
	}

	// make a directory file iterator
	if ((directory_info->flags & LBcDirectoryInfoFlag_Files) != 0)
	{
		error =
			BFrDirectory_FileIterator_New(
				directory_info->directory_ref,
				(directory_info->prefix[0] == '\0') ? NULL : directory_info->prefix,
				(directory_info->suffix[0] == '\0') ? NULL : directory_info->suffix,
				&file_iterator);
		if (error != UUcError_None) { return LBcError; }

		// fill the listbox with files
		while (1)
		{
			BFtFileRef file_ref;

			// get a file ref
			error = BFrDirectory_FileIterator_Next(file_iterator, &file_ref);
			if (error != UUcError_None) { break; }

			// add the name of the file to the listbox
			result = WMiListBox_AddString(inListBox, (UUtUns32)BFrFileRef_GetLeafName(&file_ref));
			if (result == LBcError) { break; }

			// add the file type
			result = WMiListBox_SetItemData(inListBox, result, LBcDirItemType_File);
		}

		// delete the file_iterator
		UUrMemory_Block_Delete(file_iterator);
		file_iterator = NULL;
	}

	// fill the listbox with directories
	if ((directory_info->flags & LBcDirectoryInfoFlag_Directory) != 0)
	{
		error =
			BFrDirectory_FileIterator_New(
				directory_info->directory_ref,
				NULL,
				NULL,
				&file_iterator);
		if (error != UUcError_None) { return LBcError; }

		// fill the listbox with directories
		while (1)
		{
			BFtFileRef dir_ref;

			// get a directory ref
			error = BFrDirectory_FileIterator_Next(file_iterator, &dir_ref);
			if (error != UUcError_None) { break; }

			// determine if the dir_ref is really a directory
			if (BFrFileRef_IsDirectory(&dir_ref) == UUcFalse)
			{
				continue;
			}

			// add the name of the directory to the listbox
			result = WMiListBox_AddString(inListBox, (UUtUns32)BFrFileRef_GetLeafName(&dir_ref));
			if (result == LBcError) { break; }

			// add the directory type
			result = WMiListBox_SetItemData(inListBox, result, LBcDirItemType_Directory);
		}

		// delete the file_iterator
		UUrMemory_Block_Delete(file_iterator);
		file_iterator = NULL;
	}

	return LBcNoError;
}

// ----------------------------------------------------------------------
static UUtUns32
WMiListBox_SetItemData(
	WMtListBox				*inListBox,
	UUtUns32				inItemIndex,
	UUtUns32				inParam1)
{
	WMtListBox_PrivateData		*private_data;
	WMtListBox_Item				*items;

	// get the private data
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return LBcError; }

	// check the range of inItemIndex
	if ((inItemIndex < 0) ||
		(inItemIndex >= UUrMemory_Array_GetUsedElems(private_data->item_array)))
	{
		return LBcError;
	}

	// get the array pointer
	items = (WMtListBox_Item*)UUrMemory_Array_GetMemory(private_data->item_array);

	// set the item data for the specified item
	items[inItemIndex].data = inParam1;

	return LBcNoError;
}

// ----------------------------------------------------------------------
static UUtUns32
WMiListBox_SetNumLines(
	WMtListBox				*inListBox,
	UUtUns32				inNumLines)
{
	WMtListBox_PrivateData	*private_data;
	UUtUns32				result;

	// get the private data
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return LBcError; }

	// reset the listbox
	result = WMiListBox_Reset(inListBox);
	if (result == LBcError) { return result; }

	// set the number of elements in the private_data->item_array to inNumLines
	UUrMemory_Array_MakeRoom(private_data->item_array, inNumLines, NULL);

	return inNumLines;
}

// ----------------------------------------------------------------------
static UUtUns32
WMiListBox_SetSelection(
	WMtListBox				*inListBox,
	UUtUns32				inItemIndex,
	UUtBool					inNotifyParent)
{
	WMtListBox_PrivateData	*private_data;
	WMtListBox_Item			*items;
	UUtUns32				style;
	UUtUns32				i;

	// get the private data
	private_data = (WMtListBox_PrivateData*)WMrWindow_GetLong(inListBox, 0);
	if (private_data == NULL) { return LBcError; }

	style = WMrWindow_GetStyle(inListBox);
	if (style & WMcListBoxStyle_MultipleSelection) { return LBcError; }

	// get the array pointer
	items = (WMtListBox_Item*)UUrMemory_Array_GetMemory(private_data->item_array);

	if (inItemIndex == LBcSelected)
	{
		// clear the selection
		for (i = 0; i < UUrMemory_Array_GetUsedElems(private_data->item_array); i++)
		{
			items[i].selected = UUcFalse;
		}
	}
	else
	{
		UUtInt32			diff;

		// clear the previous selection
		for (i = 0; i < UUrMemory_Array_GetUsedElems(private_data->item_array); i++)
		{
			items[i].selected = UUcFalse;
		}

		// check the bounds on the index
		if ((inItemIndex < 0) ||
			(inItemIndex >= UUrMemory_Array_GetUsedElems(private_data->item_array)))
		{
			return LBcError;
		}

		// set the selection
		items[inItemIndex].selected = UUcTrue;

		// adjust the list to see the newly selected item
		diff = (UUtInt32)(private_data->start_index - inItemIndex);
		if (diff > 0)
		{
			// current selection is above the scroll pos in the list
			if (((UUtInt32)inItemIndex - diff) < (UUtInt32)private_data->start_index)
			{
				// set the new start index
				private_data->start_index = inItemIndex;
				UUmAssert((UUtInt32)private_data->start_index >= 0);

				// the scroll position needs to be changed to make the
				// current selection visible
				if (private_data->start_ratio != 0.0f)
				{
					private_data->v_scroll_pos =
						(UUtInt32)((float)private_data->start_index / private_data->start_ratio);
				}

				WMrScrollbar_SetPosition(private_data->v_scrollbar, private_data->v_scroll_pos);
			}
		}
		else if (diff < 0)
		{
			// current selection is below the scroll pos in the list
			if ((UUtUns16)UUmABS(diff) >= private_data->num_visible_items)
			{
				// set the new start index
				private_data->start_index =
					inItemIndex - private_data->num_visible_items + 1;
				UUmAssert((UUtInt32)private_data->start_index >= 0);

				// the scroll position needs to be changed to make the
				// current selection visible
				if (private_data->start_ratio)
				{
					private_data->v_scroll_pos =
						(UUtInt32)((float)private_data->start_index / private_data->start_ratio);
				}

				WMrScrollbar_SetPosition(private_data->v_scrollbar, private_data->v_scroll_pos);
			}
		}
	}

	if (inNotifyParent)
	{
		// tell the parent
		WMrMessage_Send(
			WMrWindow_GetParent(inListBox),
			WMcMessage_Command,
			UUmMakeLong(LBcNotify_SelectionChanged, WMrWindow_GetID(inListBox)),
			(UUtUns32)inListBox);
	}

	return LBcNoError;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
WMiListBox_Callback(
	WMtListBox				*inListBox,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtUns32				result;

	switch(inMessage)
	{
		case WMcMessage_NC_Create:
			result = WMiListBox_NC_Create(inListBox);
			if (result == WMcResult_Error) { return WMcResult_Error; }
		break; /* the default window needs to handle the message as well */

		case WMcMessage_NC_Destroy:
			WMiListBox_NC_Destroy(inListBox);
		break;

		case WMcMessage_NC_HitTest:
			result = WMiListBox_NC_HitTest(inListBox, inParam1);
		return result;

		case WMcMessage_NC_Paint:
			WMiListBox_NC_Paint(inListBox);
		return WMcResult_Handled;

		case WMcMessage_NC_CalcClientSize:
			WMiListBox_NC_CalcClientSize(inListBox, (UUtRect*)inParam1);
		return WMcResult_Handled;

		case WMcMessage_Create:
			result = WMiListBox_Create(inListBox);
		return result;

		case WMcMessage_Destroy:
			WMiListBox_Destroy(inListBox);
		return WMcResult_Handled;

		case WMcMessage_Paint:
			WMiListBox_Paint(inListBox);
		return WMcResult_Handled;

		case WMcMessage_LMouseDown:
		case WMcMessage_LMouseUp:
		case WMcMessage_LMouseDblClck:
			WMiListBox_HandleMouseEvent(
				inListBox,
				inMessage,
				inParam1,
				inParam2);
		return WMcResult_Handled;

		case WMcMessage_KeyDown:
			WMiListBox_HandleKeyEvent(
				inListBox,
				inMessage,
				inParam1,
				inParam1);
		return WMcResult_Handled;

		case WMcMessage_SetFocus:
		case WMcMessage_KillFocus:
			WMiListBox_Focus(inListBox, inMessage);
		break;

		case WMcMessage_GetDialogCode:
			result =
				WMcDialogCode_WantAlphas |
				WMcDialogCode_WantDigits |
				WMcDialogCode_WantNavigation |
				WMcDialogCode_WantArrows;
		return result;

		case WMcMessage_FontInfoChanged:
			WMiListBox_HandleFontInfoChanged(inListBox);
		break;

		case LBcMessage_AddString:
			result = WMiListBox_AddString(inListBox, inParam1);
		return result;

		case LBcMessage_DeleteString:
			result = WMiListBox_DeleteString(inListBox, inParam2);
		return result;

		case LBcMessage_GetItemData:
			result = WMiListBox_GetItemData(inListBox, inParam2);
		return result;

		case LBcMessage_GetNumLines:
			result = WMiListBox_GetNumLines(inListBox);
		return result;

		case LBcMessage_GetSelection:
			result = WMiListBox_GetSelection(inListBox);
		return result;

		case LBcMessage_GetText:
			result = WMiListBox_GetText(inListBox, inParam2, (char*)inParam1);
		return result;

		case LBcMessage_GetTextLength:
			result = WMiListBox_GetTextLength(inListBox, inParam2);
		return result;

		case LBcMessage_InsertString:
			result = WMiListBox_InsertString(inListBox, inParam2, inParam1);
		return result;

		case LBcMessage_ReplaceString:
			result = WMiListBox_ReplaceString(inListBox, inParam2, inParam1);
		return result;

		case LBcMessage_Reset:
			result = WMiListBox_Reset(inListBox);
		return result;

		case LBcMessage_SelectString:
			result = WMiListBox_SelectString(inListBox, inParam1, (char*)inParam2);
		return result;

		case LBcMessage_SetDirectoryInfo:
			result = WMiListBox_SetDirectoryInfo(inListBox, inParam1, (UUtBool)inParam2);
		return result;

		case LBcMessage_SetItemData:
			result = WMiListBox_SetItemData(inListBox, inParam2, inParam1);
		return result;

		case LBcMessage_SetNumLines:
			result = WMiListBox_SetNumLines(inListBox, inParam1);
		break;

		case LBcMessage_SetSelection:
			result = WMiListBox_SetSelection(inListBox, inParam2, (UUtBool)inParam1);
		return result;

		case LBcMessage_SetLineColor:
		return LBcError;

		case SBcMessage_VerticalScroll:
			WMiListBox_HandleVerticalScroll(inListBox, inParam1, inParam2);
			WMrWindow_SetFocus(inListBox);
		return WMcResult_Handled;
	}

	return WMrWindow_DefaultCallback(inListBox, inMessage, inParam1, inParam2);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
WMrListBox_Initialize(
	void)
{
	UUtError				error;
	WMtWindowClass			window_class;

	// register the window class
	UUrMemory_Clear(&window_class, sizeof(WMtWindowClass));
	window_class.type = WMcWindowType_ListBox;
	window_class.callback = WMiListBox_Callback;
	window_class.private_data_size = sizeof(WMtListBox_PrivateData*);

	error = WMrWindowClass_Register(&window_class);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

void
WMrListBox_Reset(WMtListBox *inListBox)
{
	UUtUns32 result;

	result = WMrMessage_Send(inListBox, LBcMessage_Reset, 0, 0);

	return;
}

void
WMrListBox_SetSelection(WMtListBox *inListBox, UUtBool inNotifyParent, UUtUns32 inIndex)
{
	UUtUns32 result;

	result = WMrMessage_Send(inListBox, LBcMessage_SetSelection, (UUtUns32) inNotifyParent, inIndex);

	return;
}

UUtUns32
WMrListBox_AddString(WMtListBox *inListBox, const char *inString)
{
	UUtUns32 result;

	result = WMrMessage_Send(inListBox, LBcMessage_AddString, (UUtUns32) inString, 0);

	return result;
}

void
WMrListBox_SetItemData(WMtListBox *inListBox, UUtUns32 inItemData, UUtUns32 inIndex)
{
	UUtUns32 result;

	result = WMrMessage_Send(inListBox, LBcMessage_SetItemData, inItemData, inIndex);

	return;
}

void
WMrListBox_GetText(WMtListBox *inListBox, char *ioBuffer, UUtUns32 inIndex)
{
	UUtUns32 result;

	result = WMrMessage_Send(inListBox, LBcMessage_GetText, (UUtUns32) ioBuffer, inIndex);

	return;
}

UUtUns32
WMrListBox_GetItemData(WMtListBox *inListBox, UUtUns32 inIndex)
{
	UUtUns32 result;

	result = WMrMessage_Send(inListBox, LBcMessage_GetItemData, 0, inIndex);

	return result;
}

UUtUns32
WMrListBox_GetNumLines(WMtListBox *inListBox)
{
	UUtUns32 result;

	result = WMrMessage_Send(inListBox, LBcMessage_GetNumLines, 0, 0);

	return result;
}

UUtUns32
WMrListBox_GetSelection(
	WMtListBox					*inListBox)
{
	UUtUns32 result;

	result = WMrMessage_Send(inListBox, LBcMessage_GetSelection, 0, 0);

	return result;
}

UUtUns32
WMrListBox_SetDirectoryInfo(
	WMtListBox					*inListBox,
	BFtFileRef					*inDirectoryRef,
	UUtUns32					inFlags,
	const char					*inPrefix,
	const char					*inSuffix,
	UUtBool						inReset)
{
	UUtUns32					result;
	WMtListBox_DirectoryInfo	dir_info;

	dir_info.directory_ref = inDirectoryRef;
	dir_info.flags = inFlags;
	UUrString_Copy(dir_info.prefix, inPrefix, BFcMaxFileNameLength);
	UUrString_Copy(dir_info.suffix, inSuffix, BFcMaxFileNameLength);

	result =
		WMrMessage_Send(
			inListBox,
			LBcMessage_SetDirectoryInfo,
			(UUtUns32)&dir_info,
			(UUtUns32)inReset);

	return result;
}

UUtUns32
WMrListBox_SelectString(
	WMtListBox					*inListBox,
	const char					*inString)
{
	UUtUns32					result;

	result = WMrMessage_Send(inListBox, LBcMessage_SelectString, LBcSelected, (UUtUns32)inString);

	return result;
}
