// ======================================================================
// VM_View_Text.h
// ======================================================================
#ifndef VM_VIEW_TEXT_H
#define VM_VIEW_TEXT_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ViewManager.h"

// ======================================================================
// defines
// ======================================================================
#define VMcView_Text_MaxStringLength	128

// ======================================================================
// typedefs
// ======================================================================
#define VMcTemplate_View_Text			UUm4CharToUns32('V', 'M', 'T', 'X')
typedef tm_template('V', 'M', 'T', 'X', "VM View Text")
VMtView_Text
{
	
	UUtUns16			flags;
	UUtUns16			reserved;
	
	IMtPoint2D			text_location_offset;
	
	VMtPartSpec			*outline;
	
	char				string[128];
		
} VMtView_Text;

typedef struct VMtView_Text_PrivateData
{
	tm_templateref		text_texture_ref;
	TStTextContext		*text_context;
	UUtUns16			font_line_height;
	UUtUns16			font_descending_height;
	
} VMtView_Text_PrivateData;

extern TMtPrivateData*	DMgTemplate_Text_PrivateData;

// ======================================================================
// prototypes
// ======================================================================
UUtUns32
VMrView_Text_Callback(
	VMtView				*inView,
	VMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2);

UUtError
VMrView_Text_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData);

// ======================================================================
#endif /* VM_VIEW_TEXT_H */