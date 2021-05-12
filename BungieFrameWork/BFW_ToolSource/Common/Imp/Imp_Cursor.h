// ======================================================================
// Imp_Cursor.h
// ======================================================================
#ifndef IMP_CURSOR_H
#define IMP_CURSOR_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_TemplateManager.h"
#include "BFW_Group.h"

// ======================================================================
// prototypes
// ======================================================================
UUtError
Imp_AddCursor(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddCursorTypeList(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddCursorList(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

// ======================================================================
#endif /* IMP_VIEW_H */
