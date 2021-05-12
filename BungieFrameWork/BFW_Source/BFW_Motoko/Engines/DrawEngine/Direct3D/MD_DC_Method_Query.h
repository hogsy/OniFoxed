// ======================================================================
// MD_DC_Method_Query.h
// ======================================================================
#ifndef MD_DC_METHOD_QUERY_H
#define MD_DC_METHOD_QUERY_H

// ======================================================================
// includes
// ======================================================================
#include "BFW_Motoko.h"

// ======================================================================
// prototypes
// ======================================================================
UUtBool
MDrDrawContext_TextureFormatAvailable(
	M3tTexelType		inTexelType);

UUtUns16
MDrDrawContext_GetWidth(
	M3tDrawContext		*inDrawContext);

UUtUns16
MDrDrawContext_GetHeight(
	M3tDrawContext		*inDrawContext);

// ======================================================================
#endif /* MD_DC_METHOD_QUERY_H */