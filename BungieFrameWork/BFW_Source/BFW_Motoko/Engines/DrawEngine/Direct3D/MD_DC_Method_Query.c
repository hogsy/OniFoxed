// ======================================================================
// MD_DC_Method_Query.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "MD_DC_Private.h"
#include "MD_DC_Method_Query.h"
#include "DriverManager.h"
#include <ddraw.h>

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
UUtBool
MDrDrawContext_TextureFormatAvailable(
	M3tTexelType		inTexelType)
{
	DDPIXELFORMAT		ddpf;
	
	// set up the ddsd
	UUrMemory_Clear(&ddpf, sizeof(DDPIXELFORMAT));
	ddpf.dwSize				= sizeof(DDPIXELFORMAT);
	ddpf.dwFlags			= DDPF_RGB;
	
	switch (inTexelType)
	{
		case M3cTextureType_ARGB4444:
			ddpf.dwRGBBitCount		= 16;
			ddpf.dwFlags			|= DDPF_ALPHAPIXELS;
			ddpf.dwRBitMask			= 0x00000F00;
			ddpf.dwGBitMask			= 0x000000F0;
			ddpf.dwBBitMask			= 0x0000000F;
			ddpf.dwRGBAlphaBitMask	= 0x0000F000;
			break;
			
		case M3cTextureType_RGB555:
			ddpf.dwRGBBitCount		= 16;
			ddpf.dwRBitMask			= 0x00007C00;
			ddpf.dwGBitMask			= 0x000003E0;
			ddpf.dwBBitMask			= 0x0000001F;
			ddpf.dwRGBAlphaBitMask	= 0x00000000;
			break;
			
		case M3cTextureType_ARGB1555:
			ddpf.dwRGBBitCount		= 16;
			ddpf.dwFlags			|= DDPF_ALPHAPIXELS;
			ddpf.dwRBitMask			= 0x00007C00;
			ddpf.dwGBitMask			= 0x000003E0;
			ddpf.dwBBitMask			= 0x0000001F;
			ddpf.dwRGBAlphaBitMask	= 0x00008000;
			break;
			
		case M3cTextureType_ARGB8888:
			ddpf.dwRGBBitCount		= 32;
			ddpf.dwFlags			|= DDPF_ALPHAPIXELS;
			ddpf.dwRBitMask			= 0x00FF0000;
			ddpf.dwGBitMask			= 0x0000FF00;
			ddpf.dwBBitMask			= 0x000000FF;
			ddpf.dwRGBAlphaBitMask	= 0xFF000000;
			break;
	}
	
	// search the available texture formats to see if the desired one
	// is available
	if (MDrTextureFormatAvailable(&ddpf))
		return UUcTrue;
	
	return UUcFalse;
}

// ----------------------------------------------------------------------
UUtUns16
MDrDrawContext_GetWidth(
	M3tDrawContext		*inDrawContext)
{
	MDtDrawContextPrivate	*drawContextPrivate;
	drawContextPrivate = (MDtDrawContextPrivate *)inDrawContext->privateContext;
	
	return (UUtUns16)drawContextPrivate->width;
}

// ----------------------------------------------------------------------
UUtUns16
MDrDrawContext_GetHeight(
	M3tDrawContext		*inDrawContext)
{
	MDtDrawContextPrivate	*drawContextPrivate;
	drawContextPrivate = (MDtDrawContextPrivate *)inDrawContext->privateContext;
	
	return (UUtUns16)drawContextPrivate->height;
}


