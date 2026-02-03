// ======================================================================
// WM_ListBox.h
// ======================================================================
#ifndef WM_LISTBOX_H
#define WM_LISTBOX_H

#pragma once

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_WindowManager.h"

// ======================================================================
// enums
// ======================================================================
enum
{
	WMcListBoxStyle_None				= (0x0000 << 16),
	WMcListBoxStyle_HasScrollbar		= (0x0001 << 16),
	WMcListBoxStyle_Sort				= (0x0002 << 16),
	WMcListBoxStyle_MultipleSelection	= (0x0004 << 16),	/* not supported yet */
	WMcListBoxStyle_HasStrings			= (0x0008 << 16),
	WMcListBoxStyle_OwnerDraw			= (0x0010 << 16),
	WMcListBoxStyle_Directory			= ((0x0020 << 16) |
											WMcListBoxStyle_HasScrollbar |
											WMcListBoxStyle_HasStrings |
											WMcListBoxStyle_Sort),

	WMcListBoxStyle_SimpleListBox		= (WMcListBoxStyle_HasStrings | WMcListBoxStyle_HasScrollbar),
	WMcListBoxStyle_SortedListBox		= (WMcListBoxStyle_SimpleListBox | WMcListBoxStyle_Sort)
};

enum
{
	LBcNotify_SelectionChanged			= 1

};

enum
{
	LBcError							= 0xFFFFFFFF,
	LBcNoError							= 0x00000000
};

enum
{
	LBcDirectoryInfoFlag_None			= 0x0000,
	LBcDirectoryInfoFlag_Files			= 0x0001,
	LBcDirectoryInfoFlag_Directory		= 0x0002,
	LBcDirectoryInfoFlag_Volumes		= 0x0004	/* unsupported */
};

enum
{
	LBcDirItemType_None,
	LBcDirItemType_File,
	LBcDirItemType_Directory,
	LBcDirItemType_Volume
};

// ======================================================================
// typedefs
// ======================================================================
typedef WMtWindow				WMtListBox;

typedef struct WMtListBox_DirectoryInfo
{
	BFtFileRef							*directory_ref;
	UUtUns32							flags;
	char								prefix[BFcMaxFileNameLength];
	char								suffix[BFcMaxFileNameLength];

} WMtListBox_DirectoryInfo;

// ======================================================================
// prototypes
// ======================================================================
UUtError
WMrListBox_Initialize(
	void);

void
WMrListBox_Reset(WMtListBox *inListBox);

#define LBcNotifyParent UUcTrue
#define LBcDontNotifyParent UUcFalse

void
WMrListBox_SetSelection(WMtListBox *inListBox, UUtBool inNotifyParent, UUtUns32 inIndex);

UUtUns32
WMrListBox_AddString(WMtListBox *inListBox, const char *inString);

void
WMrListBox_SetItemData(WMtListBox *inListBox, UUtUns32 inItemData, UUtUns32 inIndex);

UUtUns32
WMrListBox_GetItemData(WMtListBox *inListBox, UUtUns32 inIndex);

void
WMrListBox_GetText(WMtListBox *inListBox, char *ioBuffer, UUtUns32 inIndex);

UUtUns32
WMrListBox_GetNumLines(WMtListBox *inListBox);

UUtUns32
WMrListBox_GetSelection(
	WMtListBox					*inListBox);

UUtUns32
WMrListBox_SetDirectoryInfo(
	WMtListBox					*inListBox,
	BFtFileRef					*inDirectoryRef,
	UUtUns32					inFlags,
	const char					*inPrefix,
	const char					*inSuffix,
	UUtBool						inReset);

UUtUns32
WMrListBox_SelectString(
	WMtListBox					*inListBox,
	const char					*inString);

// ======================================================================
#endif /* WM_LISTBOX_H */
