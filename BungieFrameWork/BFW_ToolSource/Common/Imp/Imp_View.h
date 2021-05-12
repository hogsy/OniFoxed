// ======================================================================
// Imp_View.h
// ======================================================================
#ifndef IMP_VIEW_H
#define IMP_VIEW_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ViewManager.h"

// ======================================================================
// prototypes
// ======================================================================
UUtError
Imp_AddPartSpecification(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);
	
UUtError
Imp_AddView(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

// ======================================================================
#endif /* IMP_VIEW_H */
