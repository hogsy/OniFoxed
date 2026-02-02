#pragma once
// ======================================================================
// BFW_DialogManager.h
// ======================================================================
#ifndef BFW_DIALOGMANAGER_H
#define BFW_DIALOGMANAGER_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_LocalInput.h"
#include "BFW_ViewManager.h"

// ======================================================================
// defines
// ======================================================================
#define DEBUG_DIALOGS	1

// ======================================================================
// enums
// ======================================================================
enum
{
	DMcDialogFlag_None,
	DMcDialogFlag_ShowCursor,
	DMcDialogFlag_Centered
};

// Message Box Flags
enum
{
	DMcMBFlag_None			= 0x00000000,
	DMcMBFlag_Ok			= 0x00000001,
	DMcMBFlag_OkCancel		= 0x00000003

};

// Message Box Results
enum
{
	DMcMB_Ok				= 100,
	DMcMB_Cancel			= 101

};

// ======================================================================
// typedefs
// ======================================================================
typedef VMtView				DMtDialog;
typedef VMtMessage			DMtMessage;

typedef UUtBool
(*DMtDialogCallback)(
	DMtDialog				*inDialog,
	DMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

// ----------------------------------------------------------------------
typedef tm_struct DMtDialogRef
{
	tm_templateref			dialog_ref;

} DMtDialogRef;

#define DMcTemplate_DialogList		UUm4CharToUns32('D', 'M', 'D', 'L')
typedef tm_template('D', 'M', 'D', 'L', "DM Dialog List")
DMtDialogList
{
	tm_pad						pad0[22];

	tm_varindex UUtUns16		num_dialogs;
	tm_vararray DMtDialogRef	dialogs[1];

} DMtDialogList;

// ======================================================================
// typedefs
// ======================================================================
void
DMrDialog_ActivateTab(
	DMtDialog				*inDialog,
	UUtUns16				inTabID);

void
DMrDialog_Display(
	VMtView					*inView);

DMtDialog*
DMrDialog_GetParentDialog(
	VMtView					*inView);

VMtView*
DMrDialog_GetViewByID(
	DMtDialog				*inDialog,
	UUtUns16				inViewID);

UUtBool
DMrDialog_IsActive(
	VMtView					*inView);

UUtUns32
DMrDialog_MessageBox(
	DMtDialog				*inParent,
	char					*inTitle,
	char					*inMessage,
	UUtUns32				inFlags);

UUtError
DMrDialog_Load(
	UUtUns16				inDialogID,
	DMtDialogCallback		inDialogCallback,
	DMtDialog				*inParent,
	DMtDialog				**outDialog);

void
DMrDialog_ReleaseMouseFocusView(
	DMtDialog				*inDialog,
	VMtView					*inView);

UUtError
DMrDialog_Run(
	UUtUns16				inDialogID,
	DMtDialogCallback		inDialogCallback,
	VMtView					*inParent,
	UUtUns32				*outMessage);

void
DMrDialog_SetFocus(
	DMtDialog				*inDialog,
	VMtView					*inView);

void
DMrDialog_SetMouseFocusView(
	DMtDialog				*inDialog,
	VMtView					*inView);

void
DMrDialog_Stop(
	DMtDialog				*inDialog,
	UUtUns32				inMessage);

void
DMrDialog_Update(
	DMtDialog				*inDialog);

// ----------------------------------------------------------------------
UUtError
DMrInitialize(
	void);

void
DMrTerminate(
	void);

// ----------------------------------------------------------------------
UUtError
DMVMrRegisterTemplates(
	void);

// ======================================================================
#endif /* BFW_DIALOGMANAGER_H */
