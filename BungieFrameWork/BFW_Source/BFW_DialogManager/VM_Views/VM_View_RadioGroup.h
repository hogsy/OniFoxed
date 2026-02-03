// ======================================================================
// VM_View_RadioGroup.h
// ======================================================================
#ifndef VM_VIEW_RADIOGROUP_H
#define VM_VIEW_RADIOGROUP_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ViewManager.h"

// ======================================================================
// typedefs
// ======================================================================
#define VMcTemplate_View_RadioGroup			UUm4CharToUns32('V', 'M', 'R', 'G')
typedef tm_template('V', 'M', 'R', 'G', "VM View RadioGroup")
VMtView_RadioGroup
{

	VMtPartSpec				*outline;

} VMtView_RadioGroup;

typedef struct VMtView_RadioGroup_PrivateData
{
	VMtView					*current_radiobutton;

} VMtView_RadioGroup_PrivateData;

extern TMtPrivateData*	DMgTemplate_RadioGroup_PrivateData;

// ======================================================================
// prototypes
// ======================================================================
UUtUns32
VMrView_RadioGroup_Callback(
	VMtView				*inView,
	VMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2);

UUtError
VMrView_RadioGroup_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData);

// ======================================================================
#endif /* VM_VIEW_RADIOGROUP_H */
