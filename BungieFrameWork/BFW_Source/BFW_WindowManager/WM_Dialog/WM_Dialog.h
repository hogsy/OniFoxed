// ======================================================================
// WM_Dialog.c
// ======================================================================
#ifndef WM_DIALOG_H
#define WM_DIALOG_H

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
	WMcDialogStyle_None			= (0x0000 << 16),
	WMcDialogStyle_Centered		= (0x0001 << 16),

	WMcDialogStyle_Modal		= (WMcWindowStyle_HasBackground | WMcWindowStyle_HasDrag | WMcWindowStyle_HasTitle | WMcDialogStyle_Centered),
	WMcDialogStyle_Standard		= (WMcWindowStyle_Basic | WMcDialogStyle_Centered)

};

// standard dialog item IDs
enum
{
	WMcDialogItem_None			= 0x0000,
	WMcDialogItem_OK			= 0x0001,
	WMcDialogItem_Cancel		= 0x0002,
	WMcDialogItem_Yes			= 0x0004,
	WMcDialogItem_No			= 0x0008

};

typedef enum WMtMessageBoxStyle
{
	WMcMessageBoxStyle_OK,
	WMcMessageBoxStyle_OK_Cancel,
	WMcMessageBoxStyle_Yes_No,
	WMcMessageBoxStyle_Yes_No_Cancel

} WMtMessageBoxStyle;


// dialog codes
enum
{
	WMcDialogCode_None				= 0x0000,
	WMcDialogCode_WantAlphas		= 0x0001,
	WMcDialogCode_WantDigits		= 0x0002,
	WMcDialogCode_WantPunctuation	= 0x0004,
	WMcDialogCode_WantDelete		= 0x0008,
	WMcDialogCode_WantArrows		= 0x0010,
	WMcDialogCode_WantNavigation	= 0x0020,
	WMcDialogCode_Default			= 0x0040
};

// ======================================================================
// typedefs
// ======================================================================
typedef WMtWindow				WMtDialog;
typedef UUtUns32				WMtDialogID;

// ----------------------------------------------------------------------
typedef UUtBool
(*WMtDialogCallback)(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

// ----------------------------------------------------------------------
typedef tm_struct WMtDialogItemData
{
	char				title[256];		// WMcMaxTitleLength + 1

	UUtUns16			windowtype;		// WMtWindowType
	UUtUns16			id;

	UUtUns32			flags;
	UUtUns32			style;

	UUtInt16			x;
	UUtInt16			y;
	UUtInt16			width;
	UUtInt16			height;

	TStFontInfo			font_info;

} WMtDialogItemData;

#define WMcTemplate_DialogData					UUm4CharToUns32('W', 'M', 'D', 'D')
typedef tm_template('W', 'M', 'D', 'D', "WM Dialog Data")
WMtDialogData
{
	char				title[256];		// WMcMaxTitleLength + 1

	UUtUns16			id;
	UUtUns16			unused;

	UUtUns32			flags;
	UUtUns32			style;

	UUtInt16			x;
	UUtInt16			y;
	UUtInt16			width;
	UUtInt16			height;

	tm_varindex UUtUns32				num_items;
	tm_vararray WMtDialogItemData		items[1];

} WMtDialogData;

// ======================================================================
// prototypes
// ======================================================================
UUtError
WMrDialog_Create(
	WMtDialogID				inDialogID,
	WMtWindow				*inParent,
	WMtDialogCallback		inDialogCallback,
	UUtUns32				inUserData,
	WMtDialog				**outDialog);

WMtWindow*
WMrDialog_GetItemByID(
	WMtDialog				*inDialog,
	UUtUns16				inID);

UUtUns32
WMrDialog_GetUserData(
	WMtDialog				*inDialog);

typedef UUtBool (*WMtGetString_Hook)(WMtWindow *inDialog, char *inString);

char *WMrDialog_GetString(
	WMtWindow				*inParent,
	const char				*inTitle,
	const char				*inPrompt,
	const char				*inInitialString,
	char					*ioBuffer,
	UUtUns32				inBufferSize,
	WMtGetString_Hook		inHook);

UUtUns32
WMrDialog_MessageBox(
	WMtWindow				*inDialog,
	char					*inTitle,
	char					*inMessage,
	WMtMessageBoxStyle		inStyle);

UUtError
WMrDialog_ModalBegin(
	WMtDialogID				inDialogID,
	WMtWindow				*inParent,
	WMtDialogCallback		inDialogCallback,
	UUtUns32				inUserData,
	UUtUns32				*outMessage);

void
WMrDialog_ModalEnd(
	WMtDialog				*inDialog,
	UUtUns32				inOutMessage);

void
WMrDialog_RadioButtonCheck(
	WMtDialog				*inDialog,
	UUtUns16				inFirstRadioButtonID,
	UUtUns16				inLastRadioButtonID,
	UUtUns16				inCheckRadioButtonID);

void
WMrDialog_ToggleButtonCheck(
	WMtDialog				*inDialog,
	UUtUns16				inFirstToggleButtonID,
	UUtUns16				inLastToggleButtonID,
	UUtUns16				inSetToggleButtonID);

void
WMrDialog_SetUserData(
	WMtDialog				*inDialog,
	UUtUns32				inUserData);

// ----------------------------------------------------------------------

typedef void
(*WMtDialog_ModalDrawCallback)(void);

void WMrDialog_SetDrawCallback(WMtDialog_ModalDrawCallback inModalDrawCallback);

UUtError
WMrDialog_Initialize(
	void);

UUtError
WMrDialog_RegisterTemplates(
	void);

// ======================================================================
#endif /* WM_DIALOG_H */
