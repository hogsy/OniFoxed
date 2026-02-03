// ======================================================================
// Imp_MenuStuff.h
// ======================================================================
#ifndef IMP_MENUSTUFF_H
#define IMP_MENUSTUFF_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

// ======================================================================
// prototypes
// ======================================================================
UUtError
Imp_AddMenu(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddMenuBar(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

// ======================================================================
#endif /* IMP_MENUSTUFF_H */
