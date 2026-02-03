// ======================================================================
// VM_View_Tab.h
// ======================================================================
#ifndef VM_VIEW_TAB_H
#define VM_VIEW_TAB_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ViewManager.h"

// ======================================================================
// typedefs
// ======================================================================
typedef UUtBool
(*VMtTabCallback)(
	VMtView					*inTab,
	VMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

#define VMcTemplate_View_Tab			UUm4CharToUns32('V', 'M', 'T', 'B')
typedef tm_template('V', 'M', 'T', 'B', "VM View Tab")
VMtView_Tab
{

	VMtPartSpec			*tab_button;
	VMtPartSpec			*outline;

} VMtView_Tab;

typedef struct VMtView_Tab_PrivateData
{
	VMtView					*focused_view;
	VMtTabCallback			tab_callback;

	IMtPoint2D				outline_location;
	UUtUns16				outline_width;
	UUtUns16				outline_height;

	IMtPoint2D				tab_button_location;

} VMtView_Tab_PrivateData;

extern TMtPrivateData*	DMgTemplate_Tab_PrivateData;

// ======================================================================
// prototypes
// ======================================================================
UUtBool
VMrView_Tab_SetTabCallback(
	VMtView				*inView,
	VMtTabCallback		inTabCallback);

UUtUns32
VMrView_Tab_Callback(
	VMtView				*inView,
	VMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2);

UUtError
VMrView_Tab_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData);

// ======================================================================
#endif /* VM_VIEW_TAB_H */
