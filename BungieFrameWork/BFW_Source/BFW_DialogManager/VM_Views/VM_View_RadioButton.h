// ======================================================================
// VM_View_RadioButton.h
// ======================================================================
#ifndef VM_VIEW_RADIOBUTTON_H
#define VM_VIEW_RADIOBUTTON_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ViewManager.h"
#include "BFW_Image.h"

// ======================================================================
// defines
// ======================================================================
#define VMcRadioButton_TextOffset_X		2

// ======================================================================
// enums
// ======================================================================
enum
{
	VMcRadioButton_Off		= UUcFalse,
	VMcRadioButton_On		= UUcTrue
};

// ======================================================================
// typedefs
// ======================================================================
#define VMcTemplate_View_RadioButton		UUm4CharToUns32('V', 'M', 'R', 'B')
typedef tm_template('V', 'M', 'R', 'B', "VM View RadioButton")
VMtView_RadioButton
{

	UUtUns16			flags;
	UUtUns16			reserved;

	VMtPartSpec			*off;
	VMtPartSpec			*on;

	char				title[32];

} VMtView_RadioButton;

typedef struct VMtView_RadioButton_PrivateData
{
	UUtBool				radiobutton_state;
	UUtUns16			radiobutton_width;
	UUtUns16			radiobutton_height;

	// vars for title text
	tm_templateref		title_texture_ref;
	UUtInt16			title_texture_width;
	UUtInt16			title_texture_height;
	IMtPoint2D			title_location;

} VMtView_RadioButton_PrivateData;

extern TMtPrivateData*	DMgTemplate_RadioButton_PrivateData;


// ======================================================================
// prototypes
// ======================================================================
UUtUns32
VMrView_RadioButton_Callback(
	VMtView				*inView,
	VMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2);

UUtError
VMrView_RadioButton_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData);

// ======================================================================
#endif /* VM_VIEW_RADIOBUTTON_H */
