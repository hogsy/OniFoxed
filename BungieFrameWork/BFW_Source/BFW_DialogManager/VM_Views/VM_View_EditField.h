// ======================================================================
// VM_View_EditField.h
// ======================================================================
#ifndef VM_VIEW_EDITFIELD_H
#define VM_VIEW_EDITFIELD_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ViewManager.h"
#include "BFW_TextSystem.h"
#include "BFW_Image.h"

// ======================================================================
// defines
// ======================================================================
#define DMcControl_EditField_MaxChars		256

// ======================================================================
// typedefs
// ======================================================================
#define VMcTemplate_View_EditField			UUm4CharToUns32('V', 'M', 'E', 'F')
typedef tm_template('V', 'M', 'E', 'F', "VM View EditField")
VMtView_EditField
{
	
	UUtUns16			flags;
	UUtUns16			max_chars;
	
	IMtPoint2D			text_location_offset;
	
	VMtPartSpec			*outline;
	
} VMtView_EditField;

typedef struct VMtView_EditField_PrivateData
{
	TStTextContext		*text_context;
	char				*string;
	
	IMtPoint2D			text_location;
	
	tm_templateref		caret_texture;
	UUtInt16			caret_width;
	UUtInt16			caret_height;
	UUtBool				caret_displayed;
	UUtUns32			caret_time;
	
	tm_templateref		string_texture;
	UUtInt16			string_width;
	UUtInt16			string_height;
	IMtPixel			erase_color;
	
	UUtBool				has_focus;
	
} VMtView_EditField_PrivateData;

extern TMtPrivateData*	DMgTemplate_EditField_PrivateData;

// ======================================================================
// prototypes
// ======================================================================
UUtUns32
VMrView_EditField_Callback(
	VMtView				*inView,
	VMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2);

UUtError
VMrView_EditField_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData);

// ======================================================================
#endif /* VM_VIEW_EDITFIELD_H */