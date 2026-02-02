// ======================================================================
// Imp_Material.h
// ======================================================================
#ifndef IMP_MATERIAL_H
#define IMP_MATERIAL_H

#pragma once

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

// ======================================================================
// prototypes
// ======================================================================
UUtError
Imp_AddMaterial(
	BFtFileRef			*inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup			*inGroup,
	char				*inInstanceName);

UUtError
Imp_AddImpact(
	BFtFileRef			*inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup			*inGroup,
	char				*inInstanceName);

UUtError
Imp_AddMaterialList(
	BFtFileRef			*inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup			*inGroup,
	char				*inInstanceName);

// ======================================================================
#endif /* IMP_MATERIAL_H */
