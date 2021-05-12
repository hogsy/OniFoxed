// ======================================================================
// MS_DC_Method_Query.h
// ======================================================================
#ifndef MS_DC_METHOD_QUERY_H
#define MS_DC_METHOD_QUERY_H

// ======================================================================
// includes
// ======================================================================
#include "BFW_Motoko.h"

// ======================================================================
// prototypes
// ======================================================================
UUtBool
MSrDrawContext_TextureFormatAvailable(
	M3tTexelType		inTexelType);

UUtUns16
MSrDrawContext_GetWidth(
	M3tDrawContext		*inDrawContext);

UUtUns16
MSrDrawContext_GetHeight(
	M3tDrawContext		*inDrawContext);
	
// ======================================================================
#endif /* MS_DC_METHOD_QUERY_H */