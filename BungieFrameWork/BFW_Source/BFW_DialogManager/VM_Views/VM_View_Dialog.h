// ======================================================================
// VM_View_Dialog.h
// ======================================================================
#ifndef VM_VIEW_DIALOG_H
#define VM_VIEW_DIALOG_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ViewManager.h"
#include "BFW_DialogManager.h"
#include "BFW_LocalInput.h"

#include "DM_DialogCursor.h"

// ======================================================================
// typedefs
// ======================================================================
#define DMcTemplate_DialogData		UUm4CharToUns32('D', 'M', 'D', 'D')
typedef tm_template('D', 'M', 'D', 'D', "VM View Dialog Data")
DMtDialogData
{

	UUtUns16				flags;
	UUtUns16				reserved;

	VMtPartSpec				*partspec;
	VMtTextureList			*b_textures;	// background textures

} DMtDialogData;

typedef struct DMtDialogData_PrivateData
{
	DMtDialogCallback		callback;

	UUtBool					quit_dialog;
	UUtUns32				out_message;

	VMtView					*text_focus_view;
	VMtView					*mouse_focus_view;
	VMtView					*mouse_over_view;

	DCtCursor				*cursor;
	IMtPoint2D				cursor_position;

	// for tracking double clicks
	VMtView					*mouse_clicked_view;
	UUtUns32				mouse_clicked_time;
	VMtMessage				mouse_clicked_type;
	IMtPoint2D				mouse_clicked_point;

	// local input mode
	LItMode					prev_localinput_mode;

} DMtDialogData_PrivateData;

extern TMtPrivateData*					DMgTemplate_Dialog_PrivateData;

// ======================================================================
// prototypes
// ======================================================================
UUtUns32
VMrView_Dialog_Callback(
	VMtView				*inView,
	VMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2);

UUtError
DMrDialogData_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData);

// ======================================================================
#endif /* VM_VIEW_DIALOG_H */
