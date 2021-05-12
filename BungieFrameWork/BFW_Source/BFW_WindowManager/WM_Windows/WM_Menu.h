// ======================================================================
// WM_Menu.h
// ======================================================================
#ifndef WM_MENU_H
#define WM_MENU_H

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
	WMcMenuStyle_None			= (0x0000 << 16),
	
	// CB: menus now do their own background drawing, rather than using NC drawing code
	WMcMenuStyle_Standard		= 0
};

enum
{
	WMcMenuItemFlag_None		= 0x0000,
	WMcMenuItemFlag_Divider		= 0x0001,
	WMcMenuItemFlag_Enabled		= 0x0002,
	WMcMenuItemFlag_Checked		= 0x0004
	
};

// ======================================================================
// typedefs
// ======================================================================
typedef WMtWindow				WMtMenu;

// ----------------------------------------------------------------------
typedef tm_struct WMtMenuItemData
{
	UUtUns16			flags;
	UUtUns16			id;
	char				title[64];		// WMcMaxTitleLength + 1
	
} WMtMenuItemData;

#define WMcTemplate_MenuData				UUm4CharToUns32('W', 'M', 'M', '_')
typedef tm_template('W', 'M', 'M', '_', "WM Menu")
WMtMenuData
{
	tm_pad				pad0[18];
	
	UUtUns16			id;
	char				title[64];		// WMcMaxTitleLength + 1
	
	tm_varindex UUtUns32				num_items;
	tm_vararray WMtMenuItemData			items[1];
	
} WMtMenuData;

// ======================================================================
// prototypes
// ======================================================================
UUtError
WMrMenu_AppendItem(
	WMtMenu					*inMenu,
	WMtMenuItemData			*inMenuItemData);
	
WMtMenu*
WMrMenu_Create(
	WMtMenuData				*inMenuData,
	WMtWindow				*inParent);
	
UUtBool
WMrMenu_EnableItem(
	WMtMenu					*inMenu,
	UUtUns16				inItemID,
	UUtBool					inEnable);
	
UUtBool
WMrMenu_CheckItem(
	WMtMenu					*inMenu,
	UUtUns16				inItemID,
	UUtBool					inCheck);
	
UUtBool
WMrMenu_FindItemByText(
	WMtMenu					*inMenu,
	char					*inText,
	UUtUns16				*outItemID);

UUtBool
WMrMenu_FindItemByText_NoCase(
	WMtMenu					*inMenu,
	char					*inText,
	UUtUns16				*outItemID);
	
UUtBool
WMrMenu_GetItemFlags(
	WMtMenu					*inMenu,
	UUtUns16				inItemID,
	UUtUns16				*outFlags);
	
UUtBool
WMrMenu_GetItemID(
	WMtMenu					*inMenu,
	UUtInt16				inItemIndex,
	UUtUns16				*outID);
	
UUtBool
WMrMenu_GetItemText(
	WMtMenu					*inMenu,
	UUtUns16				inItemID,
	char					*outText);
	
UUtError
WMrMenu_Initialize(
	void);

UUtError
WMrMenu_InsertItem(
	WMtMenu					*inMenu,
	WMtMenuItemData			*inMenuItemData,
	UUtInt16				inPosition);

UUtError
WMrMenu_RegisterTemplates(
	void);
	
UUtError
WMrMenu_RemoveItem(
	WMtMenu					*inMenu,
	UUtUns16				inItemID);
	
UUtError
WMrMenu_Reset(
	WMtMenu					*inMenu);
	
void
WMrMenu_Locate(
	WMtMenu					*inMenu,
	IMtPoint2D				*ioLocation,
	UUtBool					inFixedLocation);

UUtInt16
WMrMenu_GetWidth(
	WMtMenu					*inMenu);

// ======================================================================
#endif /* WM_MENU_H */