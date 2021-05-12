// ======================================================================
// BFW_DMVM_Templates.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW_DialogManager.h"
#include "BFW_ViewManager.h"

#include "DM_DialogCursor.h"

#include "VM_View_Box.h"
#include "VM_View_Button.h"
#include "VM_View_CheckBox.h"
#include "VM_View_Dialog.h"
#include "VM_View_EditField.h"
#include "VM_View_ListBox.h"
#include "VM_View_Picture.h"
#include "VM_View_RadioButton.h"
#include "VM_View_RadioGroup.h"
#include "VM_View_Scrollbar.h"
#include "VM_View_Slider.h"
#include "VM_View_Tab.h"
#include "VM_View_Text.h"

// ======================================================================
// functions
// ======================================================================
UUtError
DMVMrRegisterTemplates(
	void)
{
	UUtError	error;
	
	// templates without procs
	error =
		TMrTemplate_Register(
			VMcTemplate_TextureList,
			sizeof(VMtTextureList),
			TMcFolding_Forbid);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			DCcTemplate_CursorTypeList,
			sizeof(DCtCursorTypeList),
			TMcFolding_Forbid);
	UUmError_ReturnOnError(error);
	
	error =
		TMrTemplate_Register(
			DMcTemplate_DialogList,
			sizeof(DMtDialogList),
			TMcFolding_Forbid);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			VMcTemplate_View_Box,
			sizeof(VMtView_Box),
			TMcFolding_Forbid);
	UUmError_ReturnOnError(error);
	
	// template with procs
	error =
		TMrTemplate_Register(
			DCcTemplate_Cursor,
			sizeof(DCtCursor),
			TMcFolding_Forbid);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			VMcTemplate_View,
			sizeof(VMtView),
			TMcFolding_Forbid);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			VMcTemplate_PartSpecification,
			sizeof(VMtPartSpec),
			TMcFolding_Forbid);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			VMcTemplate_View_Button,
			sizeof(VMtView_Button),
			TMcFolding_Forbid);
	UUmError_ReturnOnError(error);
	
	error =
		TMrTemplate_Register(
			VMcTemplate_View_CheckBox,
			sizeof(VMtView_CheckBox),
			TMcFolding_Forbid);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			DMcTemplate_DialogData,
			sizeof(DMtDialogData),
			TMcFolding_Forbid);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			VMcTemplate_View_EditField,
			sizeof(VMtView_EditField),
			TMcFolding_Forbid);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			VMcTemplate_View_ListBox,
			sizeof(VMtView_ListBox),
			TMcFolding_Forbid);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			VMcTemplate_View_Picture,
			sizeof(VMtView_Picture),
			TMcFolding_Forbid);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			VMcTemplate_View_Scrollbar,
			sizeof(VMtView_Scrollbar),
			TMcFolding_Forbid);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			VMcTemplate_View_Slider,
			sizeof(VMtView_Slider),
			TMcFolding_Forbid);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			VMcTemplate_View_RadioButton,
			sizeof(VMtView_RadioButton),
			TMcFolding_Forbid);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			VMcTemplate_View_RadioGroup,
			sizeof(VMtView_RadioGroup),
			TMcFolding_Forbid);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			VMcTemplate_View_Tab,
			sizeof(VMtView_Tab),
			TMcFolding_Forbid);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			VMcTemplate_View_Text,
			sizeof(VMtView_Text),
			TMcFolding_Forbid);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}