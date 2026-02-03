// ======================================================================
// VM_View_TabGroup.h
// ======================================================================
#ifndef VM_VIEW_TABGROUP_H
#define VM_VIEW_TABGROUP_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ViewManager.h"

// ======================================================================
// typedefs
// ======================================================================
typedef struct VMtView_TabGroup_PrivateData
{
	VMtView				*current_tab;

} VMtView_TabGroup_PrivateData;

// ======================================================================
// prototypes
// ======================================================================
void
VMrView_TabGroup_ActivateTab(
	VMtView				*inTabGroup,
	UUtUns16			inTabID);

UUtUns32
VMrView_TabGroup_Callback(
	VMtView				*inView,
	VMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2);

// ======================================================================
#endif /* VM_VIEW_TABGROUP_H */
