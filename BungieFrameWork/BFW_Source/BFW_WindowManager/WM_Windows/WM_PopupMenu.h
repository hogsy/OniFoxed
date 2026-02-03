// ======================================================================
// WM_PopupMenu.h
// ======================================================================
#ifndef WM_POPUPMENU_H
#define WM_POPUPMENU_H

#pragma once

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_WindowManager.h"

#include "WM_Menu.h"

// ======================================================================
// enums
// ======================================================================
enum
{
	WMcPopupMenuStyle_None				= (0x0000 << 16),
	WMcPopupMenuStyle_NoResize			= (0x0001 << 16),
	WMcPopupMenuStyle_BuildAtRuntime	= (0x0002 << 16)

};

// ======================================================================
// typedefs
// ======================================================================
typedef WMtWindow				WMtPopupMenu;

// ======================================================================
// prototypes
// ======================================================================
UUtError
WMrPopupMenu_AppendItem_Light(
	WMtPopupMenu			*inPopupMenu,
	UUtUns16				inId,
	const char				*inTitle);

UUtError
WMrPopupMenu_AppendItem_Divider(
	WMtPopupMenu			*inPopupMenu);

UUtError
WMrPopupMenu_AppendItem(
	WMtPopupMenu			*inPopupMenu,
	WMtMenuItemData			*inMenuItemData);

UUtError
WMrPopupMenu_AppendStringList(
	WMtPopupMenu			*inPopupMenu,
	UUtUns32				inCount,
	const char				**inList);

UUtBool
WMrPopupMenu_GetItemID(
	WMtPopupMenu			*inPopupMenu,
	UUtInt16				inItemIndex,
	UUtUns16				*outID);

UUtBool
WMrPopupMenu_GetItemText(
	WMtPopupMenu			*inPopupMenu,
	UUtUns16				inItemID,
	char					*outText);

UUtError
WMrPopupMenu_Initialize(
	void);

UUtError
WMrPopupMenu_InsertItem(
	WMtPopupMenu			*inPopupMenu,
	WMtMenuItemData			*inMenuItemData,
	UUtInt16				inPosition);

UUtError
WMrPopupMenu_RemoveItem(
	WMtPopupMenu			*inPopupMenu,
	UUtUns16				inItemID);

UUtError
WMrPopupMenu_Reset(
	WMtPopupMenu			*inPopupMenu);

UUtError
WMrPopupMenu_SetSelection(
	WMtPopupMenu			*inPopupMenu,
	UUtUns16				inItemID);

UUtError
WMrPopupMenu_SelectString(
	WMtPopupMenu			*inPopupMenu,
	char					*inString);

UUtError
WMrPopupMenu_SelectString_NoCase(
	WMtPopupMenu			*inPopupMenu,
	char					*inString);

// ======================================================================
#endif /* WM_POPUPMENU_H */
