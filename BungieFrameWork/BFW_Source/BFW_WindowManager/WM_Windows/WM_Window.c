// ======================================================================
// WM_Window.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_WindowManager.h"
#include "BFW_WindowManager_Private.h"
#include "BFW_LocalInput.h"
#include "BFW_TextSystem.h"

#include "WM_Window.h"

#include "WM_PartSpecification.h"
#include "WM_DrawContext.h"
#include "WM_Utilities.h"


// ======================================================================
// defines
// ======================================================================
#define WMcWindow_Control_Buffer		2

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
WMiWindow_GetBorderSizes(
	WMtWindow				*inWindow,
	UUtInt16				*outLeftWidth,
	UUtInt16				*outTopHeight,
	UUtInt16				*outRightWidth,
	UUtInt16				*outBottomHeight)
{
	PStPartSpecUI			*partspec_ui;

	// get the partspec ui
	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) { return; }

	if (inWindow->style & WMcWindowStyle_HasBorder)
	{
		PSrPartSpec_GetSize(
			partspec_ui->border,
			PScPart_LeftTop,
			outLeftWidth,
			outTopHeight);

		PSrPartSpec_GetSize(
			partspec_ui->border,
			PScPart_RightBottom,
			outRightWidth,
			outBottomHeight);
	}
	else if (inWindow->style & WMcWindowStyle_HasBackground)
	{
		PSrPartSpec_GetSize(
			partspec_ui->background,
			PScPart_LeftTop,
			outLeftWidth,
			outTopHeight);

		PSrPartSpec_GetSize(
			partspec_ui->background,
			PScPart_RightBottom,
			outRightWidth,
			outBottomHeight);
	}
	else
	{
		*outLeftWidth = 0;
		*outTopHeight = 0;
		*outRightWidth = 0;
		*outBottomHeight = 0;
	}
}

// ----------------------------------------------------------------------
static void
WMiWindow_GetPartSizes(
	WMtWindow				*inWindow,
	UUtUns32				inStylePart,
	UUtInt16				*outWidth,
	UUtInt16				*outHeight)
{
	PStPartSpecUI			*partspec_ui;

	UUmAssert(outWidth);
	UUmAssert(outHeight);

	// get the partspec ui
	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) { return; }

	switch (inWindow->style & inStylePart)
	{
		case WMcWindowStyle_None:
			*outWidth = 0;
			*outHeight	= 0;
		break;

		case WMcWindowStyle_HasClose:
		case WMcWindowStyle_HasZoom:
		case WMcWindowStyle_HasFlatten:
			*outWidth = inWindow->title_height - WMcWindow_Control_Buffer - WMcWindow_Control_Buffer;
			*outHeight = inWindow->title_height - WMcWindow_Control_Buffer - WMcWindow_Control_Buffer;
		break;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static WMtWindowArea
WMiWindow_NC_HitTest(
	WMtWindow				*inWindow,
	UUtUns32				inParam1)
{
	UUtRect					window_rect;
	IMtPoint2D				global_mouse;

	UUtUns16				close_width;
	UUtUns16				zoom_width;
	UUtUns16				flatten_width;

	UUtInt16				border_left_width;
	UUtInt16				border_top_height;
	UUtInt16				border_right_width;
	UUtInt16				border_bottom_height;

	WMtWindowArea			part;

	// get the mouse
	global_mouse.x = (UUtInt16)UUmHighWord(inParam1);
	global_mouse.y = (UUtInt16)UUmLowWord(inParam1);

	// get the window rect
	WMrWindow_GetRect(inWindow, &window_rect);

	// check to see if mouse is even inside the window
	if (IMrRect_PointIn(&window_rect, &global_mouse) == UUcFalse)
	{
		return WMcWindowArea_None;
	}

	// initialize
	close_width = 0;
	zoom_width = 0;
	flatten_width = 0;
	part = WMcWindowArea_None;

	// remove the border from the window rect
	WMiWindow_GetBorderSizes(
		inWindow,
		&border_left_width,
		&border_top_height,
		&border_right_width,
		&border_bottom_height);

	window_rect.left	+= border_left_width;
	window_rect.top		+= border_top_height;
	window_rect.right	-= border_right_width;
	window_rect.bottom	-= border_bottom_height;

	// test the border
	if (inWindow->style & WMcWindowStyle_HasBorder)
	{
		if (global_mouse.x <= window_rect.left)
			part |= WMcWindowArea_NC_Left;

		if (global_mouse.y <= window_rect.top)
			part |= WMcWindowArea_NC_Top;

		if (global_mouse.x >= window_rect.right)
			part |= WMcWindowArea_NC_Right;

		if (global_mouse.y >= window_rect.bottom)
			part |= WMcWindowArea_NC_Bottom;
	}

	// test the title
	if ((part == WMcWindowArea_None) &&
		(inWindow->style & WMcWindowStyle_HasTitle))
	{
		window_rect.bottom = window_rect.top + inWindow->title_height;

		if (IMrRect_PointIn(&window_rect, &global_mouse))
		{
			UUtInt16			part_width;
			UUtInt16			part_height;

			// test the close box
			if ((part == WMcWindowArea_None) &&
				(inWindow->style & WMcWindowStyle_HasClose))
			{
				WMiWindow_GetPartSizes(inWindow, WMcWindowStyle_HasClose, &part_width, &part_height);

				window_rect.right -= WMcWindow_Control_Buffer;
				window_rect.left = window_rect.right - part_width;

				if (IMrRect_PointIn(&window_rect, &global_mouse))
				{
					part = WMcWindowArea_NC_Close;
				}
			}

			// test the flatten box
			if ((part == WMcWindowArea_None) &&
				(inWindow->style & WMcWindowStyle_HasFlatten))
			{
				WMiWindow_GetPartSizes(inWindow, WMcWindowStyle_HasFlatten, &part_width, &part_height);

				window_rect.right = window_rect.left - WMcWindow_Control_Buffer;
				window_rect.left = window_rect.right - part_width;

				if (IMrRect_PointIn(&window_rect, &global_mouse))
				{
					part = WMcWindowArea_NC_Flatten;
				}
			}

			// test the zoom box
			if ((part == WMcWindowArea_None) &&
				(inWindow->style & WMcWindowStyle_HasZoom))
			{
				WMiWindow_GetPartSizes(inWindow, WMcWindowStyle_HasZoom, &part_width, &part_height);

				window_rect.right = window_rect.left - WMcWindow_Control_Buffer;
				window_rect.left = window_rect.right - part_width;

				if (IMrRect_PointIn(&window_rect, &global_mouse))
				{
					part = WMcWindowArea_NC_Zoom;
				}
			}

			if (part == WMcWindowArea_None)
			{
				part = WMcWindowArea_NC_Drag;
			}
		}
	}

	// test the grow box
	if ((part == WMcWindowArea_None) &&
		(inWindow->style & WMcWindowStyle_HasGrowBox))
	{
	}

	// default to client area
	if ((part == WMcWindowArea_None) &&
		(inWindow->style & WMcWindowStyle_HasBackground))
	{
		part = WMcWindowArea_Client;
	}

	return part;
}

// ----------------------------------------------------------------------
static void
WMiWindow_NC_Paint(
	WMtWindow				*inWindow)
{
	DCtDrawContext			*draw_context;
	IMtPoint2D				dest;
	UUtInt16				width;
	UUtInt16				height;
	PStPartSpecUI			*partspec_ui;
	UUtBool					enabled;

	// get the active partspec ui
	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) return;

	draw_context = DCrDraw_NC_Begin(inWindow);

	// set the dest
	dest.x = 0;
	dest.y = 0;

	// get the window rect
	WMrWindow_GetSize(inWindow, &width, &height);

	enabled = WMrWindow_GetEnabled(inWindow);

	// draw the window background
	if (inWindow->style & WMcWindowStyle_HasBackground)
	{
		DCrDraw_PartSpec(
			draw_context,
			partspec_ui->background,
			PScPart_All,
			&dest,
			(UUtUns16)width,
			(UUtUns16)height,
			M3cMaxAlpha);
	}

	// draw the window border
	if (inWindow->style & WMcWindowStyle_HasBorder)
	{
		DCrDraw_PartSpec(
			draw_context,
			partspec_ui->border,
			PScPart_All,
			&dest,
			(UUtUns16)width,
			(UUtUns16)height,
			M3cMaxAlpha);
	}

	// draw the title
	if (inWindow->style & WMcWindowStyle_HasDrag)
	{
		PStPartSpec			*draw_me;
		UUtInt16			border_left_width;
		UUtInt16			border_top_height;
		UUtInt16			border_right_width;
		UUtInt16			border_bottom_height;
		UUtInt16			part_width;
		UUtInt16			part_height;

		WMiWindow_GetBorderSizes(
			inWindow,
			&border_left_width,
			&border_top_height,
			&border_right_width,
			&border_bottom_height);

		dest.x = border_left_width;
		dest.y = border_top_height;

		width -= (border_left_width + border_right_width);
		height = inWindow->title_height;

		// draw the draw area
		DCrDraw_PartSpec(
			draw_context,
			partspec_ui->title,
			PScPart_All,
			&dest,
			(UUtUns16)width,
			(UUtUns16)height,
			M3cMaxAlpha);

		if (inWindow->style & WMcWindowStyle_HasTitle)
		{
			UUtUns16			ascending_height;
			UUtRect				drag_bounds;

			drag_bounds.left = dest.x;
			drag_bounds.top = dest.y;
			drag_bounds.right = drag_bounds.left + width;
			drag_bounds.bottom = drag_bounds.top + height;

			ascending_height = DCrText_GetAscendingHeight();

			dest.x += 2;
			dest.y += ascending_height;

			DCrText_SetShade(enabled ? IMcShade_Black : IMcShade_Gray50);
			DCrText_SetFormat(TSc_HLeft);

			DCrDraw_String(
				draw_context,
				inWindow->title,
				&drag_bounds,
				&dest);

			dest.x -= 2;
			dest.y -= ascending_height;
		}

		// move the dest to the right hand side of the window
		dest.x += width;
		dest.y += WMcWindow_Control_Buffer;

		// draw the close box
		if (inWindow->style & WMcWindowStyle_HasClose)
		{
			WMiWindow_GetPartSizes(inWindow, WMcWindowStyle_HasClose, &part_width, &part_height);

			dest.x -= (part_width + WMcWindow_Control_Buffer);

			if ((UUmLowWord(inWindow->window_data) == WMcWindowArea_NC_Close) &&
				(inWindow->window_data & WMcWindowDataFlag_PartHilite))
			{
				draw_me = partspec_ui->close_pressed;
			}
			else
			{
				draw_me = partspec_ui->close_idle;
			}

			DCrDraw_PartSpec(
				draw_context,
				draw_me,
				PScPart_All,
				&dest,
				part_width,
				part_height,
				M3cMaxAlpha);
		}

		// draw the flatten box
		if (inWindow->style & WMcWindowStyle_HasFlatten)
		{
			WMiWindow_GetPartSizes(inWindow, WMcWindowStyle_HasFlatten, &part_width, &part_height);

			dest.x -= (part_width + WMcWindow_Control_Buffer);

			if ((UUmLowWord(inWindow->window_data) == WMcWindowArea_NC_Flatten) &&
				(inWindow->window_data & WMcWindowDataFlag_PartHilite))
			{
				draw_me = partspec_ui->flatten_pressed;
			}
			else
			{
				draw_me = partspec_ui->flatten_idle;
			}

			DCrDraw_PartSpec(
				draw_context,
				draw_me,
				PScPart_All,
				&dest,
				part_width,
				part_height,
				M3cMaxAlpha);
		}

		// draw the zoom box
		if (inWindow->style & WMcWindowStyle_HasZoom)
		{
			WMiWindow_GetPartSizes(inWindow, WMcWindowStyle_HasZoom, &part_width, &part_height);

			dest.x -= (part_width + WMcWindow_Control_Buffer);

			if ((UUmLowWord(inWindow->window_data) == WMcWindowArea_NC_Zoom) &&
				(inWindow->window_data & WMcWindowDataFlag_PartHilite))
			{
				draw_me = partspec_ui->zoom_pressed;
			}
			else
			{
				draw_me = partspec_ui->zoom_idle;
			}

			DCrDraw_PartSpec(
				draw_context,
				draw_me,
				PScPart_All,
				&dest,
				part_width,
				part_height,
				M3cMaxAlpha);
		}
	}

	DCrDraw_NC_End(draw_context, inWindow);
}

// ----------------------------------------------------------------------
static void
WMiWindow_NC_HandleMouseEvent(
	WMtWindow				*inWindow,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	IMtPoint2D				global_mouse;
	IMtPoint2D				local_mouse;
	WMtWindowArea			part;

	// get the mouse location
	global_mouse.x = (UUtInt16)UUmHighWord(inParam1);
	global_mouse.y = (UUtInt16)UUmLowWord(inParam1);

	// get the local mouse coordinates
	WMrWindow_GlobalToLocal(inWindow, &global_mouse, &local_mouse);

	// get the part
	part = (WMtWindowArea)inParam2;

	// handle the event
	switch (inMessage)
	{
		case WMcMessage_NC_LMouseDown:
			WMrWindow_CaptureMouse(inWindow);
			inWindow->window_data = (inWindow->window_data & 0xFFFF0000) | (part & 0x0000FFFF);
			inWindow->window_data |= WMcWindowDataFlag_PartHilite;
		break;

		case WMcMessage_NC_LMouseUp:
			if ((WMtWindowArea)UUmLowWord(inWindow->window_data) == part)
			{
				WMtMessage		message;

				switch (part)
				{
					case WMcWindowArea_NC_Close:	message = WMcMessage_Close; break;
					case WMcWindowArea_NC_Flatten:	message = WMcMessage_Flatten; break;
					case WMcWindowArea_NC_Zoom:		message = WMcMessage_Zoom; break;
					default:						message = WMcMessage_None; break;
				}

				if (message != WMcMessage_None)
				{
					WMrMessage_Send(inWindow, message, 0, 0);
				}
			}

			inWindow->window_data &= 0x7FFF0000;
			WMrWindow_CaptureMouse(NULL);
		break;
	}

	inWindow->prev_mouse_location = global_mouse;
}

// ----------------------------------------------------------------------
static void
WMiWindow_NC_CalcClientSize(
	WMtWindow				*inWindow,
	UUtRect					*outClientRect)
{
	UUtInt16				border_left_width;
	UUtInt16				border_top_height;
	UUtInt16				border_right_width;
	UUtInt16				border_bottom_height;

	// get the width and height of the border
	WMiWindow_GetBorderSizes(
		inWindow,
		&border_left_width,
		&border_top_height,
		&border_right_width,
		&border_bottom_height);

	// set the client rect
	outClientRect->left		= border_left_width;
	outClientRect->top		= border_top_height;
	outClientRect->right	= inWindow->width - border_right_width;
	outClientRect->bottom	= inWindow->height - border_bottom_height;

	if (inWindow->style & WMcWindowStyle_HasDrag)
	{
		outClientRect->top += inWindow->title_height;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
WMiWindow_Flatten(
	WMtWindow				*inWindow)
{
	UUtInt16				new_width;
	UUtInt16				new_height;

	if (inWindow->window_data & WMcWindowDataFlag_Flattened)
	{
		new_width = inWindow->restored_rect.right - inWindow->restored_rect.left;
		new_height = inWindow->restored_rect.bottom - inWindow->restored_rect.top;

		inWindow->window_data &= ~WMcWindowDataFlag_Flattened;
		inWindow->window_data &= ~WMcWindowDataFlag_Zoomed;
	}
	else
	{
		UUtInt16				border_left_width;
		UUtInt16				border_top_height;
		UUtInt16				border_right_width;
		UUtInt16				border_bottom_height;

		// get the width and height of the border
		WMiWindow_GetBorderSizes(
			inWindow,
			&border_left_width,
			&border_top_height,
			&border_right_width,
			&border_bottom_height);

		// calculate the min width and height
		new_width = border_left_width + border_right_width;
		new_height = border_top_height + border_bottom_height;

		if (inWindow->style & WMcWindowStyle_HasTitle)
		{
			UUtInt16			part_width;
			UUtInt16			part_height;

			new_width += inWindow->title_width;
			new_height += inWindow->title_height;

			if (inWindow->style & WMcWindowStyle_HasClose)
			{
				WMiWindow_GetPartSizes(inWindow, WMcWindowStyle_HasClose, &part_width, &part_height);

				new_width += (part_width + WMcWindow_Control_Buffer);
			}

			if (inWindow->style & WMcWindowStyle_HasFlatten)
			{
				WMiWindow_GetPartSizes(inWindow, WMcWindowStyle_HasFlatten, &part_width, &part_height);

				new_width += (part_width + WMcWindow_Control_Buffer);
			}

			if (inWindow->style & WMcWindowStyle_HasZoom)
			{
				WMiWindow_GetPartSizes(inWindow, WMcWindowStyle_HasZoom, &part_width, &part_height);

				new_width += (part_width + WMcWindow_Control_Buffer);
			}
		}

		// save the current bounds
		WMrWindow_GetRect(inWindow, &inWindow->restored_rect);

		inWindow->window_data |= WMcWindowDataFlag_Flattened;
		inWindow->window_data &= ~WMcWindowDataFlag_Zoomed;
	}

	// set the new size
	WMrWindow_SetSize(inWindow, new_width, new_height);
}

// ----------------------------------------------------------------------
static void
WMiWindow_HandleActivate(
	WMtWindow				*inWindow,
	UUtBool					inActivate)
{
	if ((inWindow->flags & WMcWindowFlag_Child) != 0) { return; }

	if (inActivate)
	{
		WMrWindow_SetFocus(inWindow->had_focus);
	}
	else
	{
		WMtWindow				*has_focus;

		// save the current focus
		has_focus = WMrWindow_GetFocus();
		if (WMrWindow_IsChild(has_focus, inWindow))
		{
			inWindow->had_focus = has_focus;
		}
		else
		{
			inWindow->had_focus = inWindow;
		}
	}
}

// ----------------------------------------------------------------------
static void
WMiWindow_HandleCaptureChanged(
	WMtWindow				*inWindow)
{
	inWindow->window_data &= 0x7FFF0000;
}

// ----------------------------------------------------------------------
static void
WMiWindow_HandleResolutionChanged(
	WMtWindow				*inWindow,
	UUtUns32				inParam1)
{
	WMtWindow				*child;

	// tell all the windows that the resolution changed
	if (inWindow->child == NULL) { return; }

	child = inWindow->child;
	while (child != NULL)
	{
		// windows flagged as child windows don't get notified
		if ((child->flags & WMcWindowFlag_Child) != 0) { continue; }

		// notify the child window
		WMrMessage_Send(child, WMcMessage_ResolutionChanged, inParam1, 0);

		child = child->next;
	}
}

// ----------------------------------------------------------------------
static void
WMiWindow_HandleSizeMove(
	WMtWindow				*inWindow,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	IMtPoint2D				global_mouse;
	IMtPoint2D				local_mouse;

	IMtPoint2D				new_dest;
	UUtInt16				new_width;
	UUtInt16				new_height;

	UUtInt16				min_width;
	UUtInt16				min_height;

	UUtInt16				diff_x;
	UUtInt16				diff_y;

	UUtInt16				mouse_diff;
	UUtInt32				i;

	UUtRect					window_rect;

	UUtInt16				border_left_width;
	UUtInt16				border_top_height;
	UUtInt16				border_right_width;
	UUtInt16				border_bottom_height;

	// initialize
	new_dest = inWindow->location;
	new_width = inWindow->width;
	new_height = inWindow->height;

	// get the width and height of the border
	WMiWindow_GetBorderSizes(
		inWindow,
		&border_left_width,
		&border_top_height,
		&border_right_width,
		&border_bottom_height);

	// calculate the min width and height
	min_width = border_left_width + border_right_width;
	min_height = border_top_height + border_bottom_height;

	// get the mouse positions
	global_mouse.x = (UUtInt16)UUmHighWord(inParam1);
	global_mouse.y = (UUtInt16)UUmLowWord(inParam1);

	WMrWindow_GetRect(inWindow, &window_rect);
	local_mouse.x = global_mouse.x - window_rect.left;
	local_mouse.y = global_mouse.y - window_rect.top;

	if (inWindow->style & WMcWindowStyle_HasTitle)
	{
		UUtInt16			part_width;
		UUtInt16			part_height;

		min_width += inWindow->title_width;
		min_height += inWindow->title_height;

		if (inWindow->style & WMcWindowStyle_HasClose)
		{
			WMiWindow_GetPartSizes(inWindow, WMcWindowStyle_HasClose, &part_width, &part_height);

			min_width += (part_width + WMcWindow_Control_Buffer);
		}

		if (inWindow->style & WMcWindowStyle_HasFlatten)
		{
			WMiWindow_GetPartSizes(inWindow, WMcWindowStyle_HasFlatten, &part_width, &part_height);

			min_width += (part_width + WMcWindow_Control_Buffer);
		}

		if (inWindow->style & WMcWindowStyle_HasZoom)
		{
			WMiWindow_GetPartSizes(inWindow, WMcWindowStyle_HasZoom, &part_width, &part_height);

			min_width += (part_width + WMcWindow_Control_Buffer);
		}
	}

	// calculate the difference between the previous and current mouse locations
	diff_x = global_mouse.x - inWindow->prev_mouse_location.x;
	diff_y = global_mouse.y - inWindow->prev_mouse_location.y;

	switch (UUmLowWord(inWindow->window_data))
	{
		case WMcWindowArea_NC_Drag:
			new_dest.x = inWindow->location.x + diff_x;
			new_dest.y = inWindow->location.y + diff_y;
		break;

		case WMcWindowArea_NC_Close:
		case WMcWindowArea_NC_Flatten:
		case WMcWindowArea_NC_Zoom:
			if ((WMtWindowArea)UUmLowWord(inWindow->window_data) ==
				WMiWindow_NC_HitTest(inWindow, inParam1))
			{
				inWindow->window_data |= WMcWindowDataFlag_PartHilite;
			}
			else
			{
				inWindow->window_data &= ~WMcWindowDataFlag_PartHilite;
			}
		break;

		default:
			for (i = WMcWindowArea_NC_Left; i <= WMcWindowArea_NC_Bottom; i = (i << 1))
			{
				switch (UUmLowWord(inWindow->window_data) & i)
				{
					case WMcWindowArea_NC_Left:
						if (((diff_x < 0) && (local_mouse.x > 0)) ||
							((diff_x > 0) && (local_mouse.x < 0)))
						{
							break;
						}

						// calculate the new width and dest
						new_width = inWindow->width - diff_x;
						new_dest.x = inWindow->location.x + diff_x;
					break;

					case WMcWindowArea_NC_Right:
						mouse_diff = local_mouse.x - inWindow->width;

						if (((diff_x < 0) && (mouse_diff > 0)) ||
							((diff_x > 0) && (mouse_diff < 0)))
						{
							break;
						}

						// calculate the new width
						new_width = inWindow->width + diff_x;
					break;

					case WMcWindowArea_NC_Top:
						if (((diff_y < 0) && (local_mouse.y > 0)) ||
							((diff_y > 0) && (local_mouse.y < 0)))
						{
							break;
						}

						// calculate the new height and dest
						new_height = inWindow->height - diff_y;
						new_dest.y = inWindow->location.y + diff_y;
					break;

					case WMcWindowArea_NC_Bottom:
						mouse_diff = local_mouse.y - inWindow->height;

						if (((diff_y < 0) && (mouse_diff > 0)) ||
							((diff_y > 0) && (mouse_diff < 0)))
						{
							break;
						}

						// calculate the new height
						new_height = inWindow->height + diff_y;
					break;
				}
			}
		break;
	}

	switch(UUmLowWord(inWindow->window_data))
	{
		case WMcWindowArea_NC_Drag:
			// set the windows new location
			WMrWindow_SetLocation(inWindow, new_dest.x, new_dest.y);
		break;

		case WMcWindowArea_NC_Left:
		case WMcWindowArea_NC_Top:
		case WMcWindowArea_NC_Right:
		case WMcWindowArea_NC_Bottom:
		case WMcWindowArea_NC_TopLeft:
		case WMcWindowArea_NC_TopRight:
		case WMcWindowArea_NC_BottomLeft:
		case WMcWindowArea_NC_BottomRight:
			if ((new_width >= min_width) && (new_height >= min_height))
			{
				// set the windows new location and size
				WMrWindow_SetPosition(
					inWindow,
					NULL,
					new_dest.x,
					new_dest.y,
					new_width,
					new_height,
					WMcPosChangeFlag_NoZOrder);

				// window is no longer in one of these states
				inWindow->window_data &= ~(WMcWindowDataFlag_Flattened | WMcWindowDataFlag_Zoomed);
			}
		break;

		default:
		{
			UUtBool		convenient;
			convenient = UUcTrue;
		}
		break;
	}

	// save the mouse position
	inWindow->prev_mouse_location = global_mouse;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtUns32
WMrWindow_DefaultCallback(
	WMtWindow				*inWindow,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtUns32				result;

	result = WMcResult_Handled;

	switch (inMessage)
	{
		// ------------------------------
		case WMcMessage_NC_HitTest:
			result = (UUtUns32)WMiWindow_NC_HitTest(inWindow, inParam1);
		break;

		// ------------------------------
		case WMcMessage_NC_Paint:
			WMiWindow_NC_Paint(inWindow);
		break;

		// ------------------------------
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
			WMiWindow_NC_HandleMouseEvent(inWindow, inMessage, inParam1, inParam2);
		break;

		// ------------------------------
		case WMcMessage_NC_CalcClientSize:
			WMiWindow_NC_CalcClientSize(inWindow, (UUtRect*)inParam1);
		break;

		// ------------------------------
		case WMcMessage_Close:
			WMrWindow_Delete(inWindow);
		break;

		// ------------------------------
		case WMcMessage_Flatten:
			WMiWindow_Flatten(inWindow);
		break;

		// ------------------------------
		case WMcMessage_Zoom:
		break;

		// ------------------------------
		case WMcMessage_MouseMove:
			if (UUmLowWord(inWindow->window_data) != WMcWindowArea_None)
			{
				WMiWindow_HandleSizeMove(inWindow, inParam1, inParam2);
			}
		break;

		// ------------------------------
		case WMcMessage_Activate:
			WMiWindow_HandleActivate(inWindow, (UUtBool)inParam1);
		break;

		// ------------------------------
		case WMcMessage_GetDialogCode:
			result = WMcDialogCode_None;
		break;

		// ------------------------------
		case WMcMessage_CaptureChanged:
			WMiWindow_HandleCaptureChanged(inWindow);
		break;

		// ------------------------------
		case WMcMessage_ResolutionChanged:
			WMiWindow_HandleResolutionChanged(inWindow, inParam1);
		break;
	}

	return result;
}
