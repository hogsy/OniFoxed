// ======================================================================
// Imp_DialogList.h
// ======================================================================
#ifndef IMP_DIALOGLIST_H
#define IMP_DIALOGLIST_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_Group.h"

// ======================================================================
// prototypes
// ======================================================================
UUtError
Imp_AddDialogList(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

// ======================================================================
#endif /* IMP_DIALOGLIST_H */
