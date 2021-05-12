// ======================================================================
// MG_DC_Method_Query.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "MG_DC_Private.h"
#include "MG_DC_Method_Query.h"
#include "rasterizer_3dfx.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
UUtBool
MGrDrawContext_TextureFormatAvailable(
	IMtPixelType		inTexelType)
{
	const MGtTexelTypeInfo *texelInfo = MGrTexelType_GetInfo(inTexelType);
	UUtBool texelTypeSupported = (texelInfo->textureValid == MGcTextureValid);

	return texelTypeSupported;
}
