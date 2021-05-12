// ======================================================================
// WM_MenuBar.h
// ======================================================================
#ifndef WM_MENUBAR_H
#define WM_MENUBAR_H

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
	WMcMenuBarStyle_None			= (0x0000 << 16),

	WMcMenuBarStyle_Standard		= (WMcWindowStyle_HasBackground)
};

// ======================================================================
// typedefs
// ======================================================================
typedef WMtWindow				WMtMenuBar;

// ----------------------------------------------------------------------
#define WMcTemplate_MenuBarData				UUm4CharToUns32('W', 'M', 'M', 'B')
typedef tm_template('W', 'M', 'M', 'B', "WM Menu Bar")
WMtMenuBarData
{
	tm_pad								pad[18];
	
	UUtUns16							id;
	
	tm_varindex UUtUns32				num_menus;
	tm_vararray WMtMenuData				*menus[1];
	
} WMtMenuBarData;

// ======================================================================
// prototypes
// ======================================================================
UUtError
WMrMenuBar_AppendMenu(
	WMtMenuBar				*inMenuBar,
	WMtMenu					*inMenu);

WMtMenuBar*
WMrMenuBar_Create(
	WMtWindow				*inParent,
	char					*inMenuBarName);
	
WMtMenu*
WMrMenuBar_GetMenu(
	WMtMenuBar				*inMenuBar,
	UUtUns16				inMenuID);
	
UUtError
WMrMenuBar_Initialize(
	void);

UUtError
WMrMenuBar_InsertMenu(
	WMtMenuBar				*inMenuBar,
	WMtMenu					*inMenu,
	UUtInt16				inIndex);

UUtError
WMrMenuBar_RegisterTemplates(
	void);

UUtError
WMrMenuBar_RemoveMenu(
	WMtMenuBar				*inMenuBar,
	UUtUns16				inMenuID);

// ======================================================================
#endif /* WM_MENUBAR_H */