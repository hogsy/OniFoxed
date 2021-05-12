// ======================================================================
// Imp_Furniture.h
// ======================================================================
#ifndef IMP_FURNITURE_H
#define IMP_FURNITURE_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

#include "Oni_Object.h"

// ======================================================================
// prototypes
// ======================================================================
void
IMPrFurniture_GetLightData(
	MXtNode					*inNode,
	UUtUns32				inIndex,
	OBJtFurnGeom			*ioFurnGeom);
	
UUtError
Imp_AddFurniture(
	BFtFileRef*			inSourceFileRef,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

// ======================================================================
#endif /* IMP_FURNITURE_H */