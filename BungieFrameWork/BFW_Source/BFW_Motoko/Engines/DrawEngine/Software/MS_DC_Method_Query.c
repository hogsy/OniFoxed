// ======================================================================
// MS_DC_Method_Query.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "MS_DC_Private.h"
#include "MS_DC_Method_Query.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
UUtBool
MSrDrawContext_TextureFormatAvailable(
	M3tTexelType		inTexelType)
{
	switch (inTexelType)
	{
		case M3cTextureType_ARGB4444:
		case M3cTextureType_ARGB1555:
		case M3cTextureType_RGB555:
		case M3cTextureType_ARGB8888:
			return UUcTrue;
			break;
		
		default:
			return UUcFalse;
			break;
	}
	
	return UUcFalse;
}

// ----------------------------------------------------------------------
UUtUns16
MSrDrawContext_GetWidth(
	M3tDrawContext		*inDrawContext)
{
	MStDrawContextPrivate	*drawContextPrivate;
	drawContextPrivate = (MStDrawContextPrivate *)inDrawContext->privateContext;
	
	return (UUtUns16)drawContextPrivate->width;
}

// ----------------------------------------------------------------------
UUtUns16
MSrDrawContext_GetHeight(
	M3tDrawContext		*inDrawContext)
{
	MStDrawContextPrivate	*drawContextPrivate;
	drawContextPrivate = (MStDrawContextPrivate *)inDrawContext->privateContext;
	
	return (UUtUns16)drawContextPrivate->height;
}

