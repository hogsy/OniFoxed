// ======================================================================
// Imp_PartSpec.h
// ======================================================================
#ifndef IMP_PARTSPEC_H
#define IMP_PARTSPEC_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

// ======================================================================
// prototypes
// ======================================================================
UUtError
Imp_AddPartSpec(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);
	
UUtError
Imp_AddPartSpecList(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddPartSpecUI(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);
	
// ======================================================================
#endif /* IMP_PARTSPEC_H */