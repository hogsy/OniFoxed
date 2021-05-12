// ======================================================================
// BFW_WindowManager_Private.h
// ======================================================================
#ifndef BFW_WINDOWMANAGER_PRIVATE_H
#define BFW_WINDOWMANAGER_PRIVATE_H

#pragma once

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_WindowManager.h"

// ======================================================================
// enums
// ======================================================================
enum
{
	WMcDesktopStyle_None		= 0x0000
	
};

enum
{
	WMcWindowDataFlag_None			= 0x00000000,
	WMcWindowDataFlag_PartHilite	= 0x80000000,
	WMcWindowDataFlag_Flattened		= 0x40000000,
	WMcWindowDataFlag_Zoomed		= 0x20000000

};

// ======================================================================
// typedefs
// ======================================================================
// ----------------------------------------------------------------------
struct WMtCaret
{
	WMtWindow			*owner;
	
	IMtPoint2D			position;
	UUtInt16			width;
	UUtInt16			height;
	
	UUtBool				visible;
	
	UUtBool				blink_visible;
	UUtUns32			blink_time;
	
};

// ----------------------------------------------------------------------
struct WMtWindow
{
	WMtWindow			*prev;
	WMtWindow			*next;
	
	WMtWindow			*parent;
	WMtWindow			*owner;
	WMtWindow			*child;
	
	WMtWindowClass		*window_class;
	UUtUns32			flags;
	UUtUns32			style;
	UUtUns16			id;
	
	UUtUns32			index;
	
	IMtPoint2D			location;
	UUtInt16			width;
	UUtInt16			height;
	
	UUtRect				client_rect;
	UUtRect				restored_rect;
	
	char				title[WMcMaxTitleLength + 1];
	UUtInt16			title_width;
	UUtInt16			title_height;
	
	WMtWindow			*had_focus;
	
	TStFontInfo			font_info;
	
	// nc mouse event data
	IMtPoint2D			prev_mouse_location;
	UUtUns32			window_data;	// bits 31 - 0,  31 = part hilite, 30 = flattened, 29 = zoomed, 15 - 0 = window area
	
};

// ======================================================================
#endif /* BFW_WINDOWMANAGER_PRIVATE_H */