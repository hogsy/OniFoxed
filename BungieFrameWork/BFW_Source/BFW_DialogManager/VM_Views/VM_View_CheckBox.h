// ======================================================================
// VM_View_CheckBox.h
// ======================================================================
#ifndef VM_VIEW_CHECKBOX_H
#define VM_VIEW_CHECKBOX_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ViewManager.h"
#include "BFW_Image.h"

// ======================================================================
// enums
// ======================================================================
enum
{
	VMcCheckBox_Off		= UUcFalse,
	VMcCheckBox_On		= UUcTrue
};

// ======================================================================
// typedefs
// ======================================================================
#define VMcTemplate_View_CheckBox		UUm4CharToUns32('V', 'M', 'C', 'B')
typedef tm_template('V', 'M', 'C', 'B', "VM View CheckBox")
VMtView_CheckBox
{
	
	UUtUns16			flags;
	UUtUns16			reserved;
	IMtPoint2D			title_location_offset;
	
	VMtPartSpec			*outline;
	VMtPartSpec			*off;
	VMtPartSpec			*on;
	
	char				title[32];
	
} VMtView_CheckBox;

typedef struct VMtView_CheckBox_PrivateData
{
	UUtBool				checkbox_state;
	IMtPoint2D			checkbox_location;
	UUtUns16			checkbox_width;
	UUtUns16			checkbox_height;
	
	// vars for title text
	tm_templateref		title_texture_ref;
	IMtPoint2D			title_location;
	UUtInt16			title_texture_width;
	UUtInt16			title_texture_height;
	
} VMtView_CheckBox_PrivateData;

extern TMtPrivateData*	DMgTemplate_CheckBox_PrivateData;

// ======================================================================
// prototypes
// ======================================================================
UUtUns32
VMrView_CheckBox_Callback(
	VMtView				*inView,
	VMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2);

UUtError
VMrView_CheckBox_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData);

// ======================================================================
#endif /* VM_VIEW_CHECKBOX_H */