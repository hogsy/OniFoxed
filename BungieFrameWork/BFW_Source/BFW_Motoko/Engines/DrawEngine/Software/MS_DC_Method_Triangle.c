/*
	FILE:	MS_DC_Method_Triangle.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 18, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"

#include "Motoko_Manager.h"

#include "BFW_Shared_TriRaster.h"

#include "MS_DC_Private.h"
#include "MS_DC_Method_Triangle.h"

#if defined(MSmPixelTouch) && MSmPixelTouch

	#define MSmTriRaster_PixelTouch	1

#else

	#define MSmTriRaster_PixelTouch 0

#endif

#undef	MSmTriRaster_FunctionName
#undef	MSmTriRaster_Texture
#undef	MSmTriRaster_TextureNoShade
#undef	MSmTriRaster_InterpShading
#undef	MSmTriRaster_Alpha
#undef	MSmTriRaster_Dither
#undef	MSmTriRaster_RGB565
#undef	MSmTriRaster_IsVisible
#undef	MSmTriRaster_RasterZOnly
#undef	MSmTriRaster_Split
#undef	MSmTriRaster_Lightmap
#define	MSmTriRaster_FunctionName	MSrDrawContext_Method_TriGouraudInterpolate
#define	MSmTriRaster_Texture		0
#define	MSmTriRaster_TextureNoShade	0
#define	MSmTriRaster_InterpShading	1
#define	MSmTriRaster_Alpha			0
#define	MSmTriRaster_Dither			0
#define MSmTriRaster_RGB565			(UUmPlatform == UUmPlatform_Win32)
#define MSmTriRaster_IsVisible		0
#define MSmTriRaster_RasterZOnly	0
#define MSmTriRaster_Split			0
#define MSmTriRaster_Lightmap		0

#include "BFW_Shared_TriRaster_c.h"

#undef	MSmTriRaster_FunctionName
#undef	MSmTriRaster_Texture
#undef	MSmTriRaster_TextureNoShade
#undef	MSmTriRaster_InterpShading
#undef	MSmTriRaster_Alpha
#undef	MSmTriRaster_Dither
#undef	MSmTriRaster_RGB565
#undef	MSmTriRaster_IsVisible
#undef	MSmTriRaster_RasterZOnly
#undef	MSmTriRaster_Split
#undef	MSmTriRaster_Lightmap
#define	MSmTriRaster_FunctionName	MSrDrawContext_Method_TriGouraudFlat
#define	MSmTriRaster_Texture		0
#define	MSmTriRaster_TextureNoShade	0
#define	MSmTriRaster_InterpShading	0
#define	MSmTriRaster_Alpha			0
#define	MSmTriRaster_Dither			0
#define MSmTriRaster_RGB565			(UUmPlatform == UUmPlatform_Win32)
#define MSmTriRaster_IsVisible		0
#define MSmTriRaster_RasterZOnly	0
#define MSmTriRaster_Split			0
#define MSmTriRaster_Lightmap		0

#include "BFW_Shared_TriRaster_c.h"

#undef	MSmTriRaster_FunctionName
#undef	MSmTriRaster_Texture
#undef	MSmTriRaster_TextureNoShade
#undef	MSmTriRaster_InterpShading
#undef	MSmTriRaster_Alpha
#undef	MSmTriRaster_Dither
#undef	MSmTriRaster_RGB565
#undef	MSmTriRaster_IsVisible
#undef	MSmTriRaster_RasterZOnly
#undef	MSmTriRaster_Split
#undef	MSmTriRaster_Lightmap
#define	MSmTriRaster_FunctionName	MSrDrawContext_Method_TriTextureInterpolate
#define	MSmTriRaster_Texture		1
#define	MSmTriRaster_TextureNoShade	0
#define	MSmTriRaster_InterpShading	1
#define	MSmTriRaster_Alpha			0
#define	MSmTriRaster_Dither			0
#define MSmTriRaster_RGB565			(UUmPlatform == UUmPlatform_Win32)
#define MSmTriRaster_IsVisible		0
#define MSmTriRaster_RasterZOnly	0
#define MSmTriRaster_Split			0
#define MSmTriRaster_Lightmap		0

#include "BFW_Shared_TriRaster_c.h"

#undef	MSmTriRaster_FunctionName
#undef	MSmTriRaster_Texture
#undef	MSmTriRaster_TextureNoShade
#undef	MSmTriRaster_InterpShading
#undef	MSmTriRaster_Alpha
#undef	MSmTriRaster_Dither
#undef	MSmTriRaster_RGB565
#undef	MSmTriRaster_IsVisible
#undef	MSmTriRaster_RasterZOnly
#undef	MSmTriRaster_Split
#undef	MSmTriRaster_Lightmap
#define	MSmTriRaster_FunctionName	MSrDrawContext_Method_TriTextureFlat
#define	MSmTriRaster_Texture		1
#define	MSmTriRaster_TextureNoShade	0
#define	MSmTriRaster_InterpShading	0
#define	MSmTriRaster_Alpha			0
#define	MSmTriRaster_Dither			0
#define MSmTriRaster_RGB565			(UUmPlatform == UUmPlatform_Win32)
#define MSmTriRaster_IsVisible		0
#define MSmTriRaster_RasterZOnly	0
#define MSmTriRaster_Split			0
#define MSmTriRaster_Lightmap		0

#include "BFW_Shared_TriRaster_c.h"

#if 0
#undef	MSmTriRaster_FunctionName
#undef	MSmTriRaster_Texture
#undef	MSmTriRaster_TextureNoShade
#undef	MSmTriRaster_InterpShading
#undef	MSmTriRaster_Alpha
#undef	MSmTriRaster_Dither
#undef	MSmTriRaster_RGB565
#undef	MSmTriRaster_IsVisible
#undef	MSmTriRaster_RasterZOnly
#undef	MSmTriRaster_Split
#undef	MSmTriRaster_Lightmap
#define	MSmTriRaster_FunctionName	MSrDrawContext_Method_TriTextureSplit
#define	MSmTriRaster_Texture		1
#define	MSmTriRaster_TextureNoShade	0
#define	MSmTriRaster_InterpShading	0
#define	MSmTriRaster_Alpha			0
#define	MSmTriRaster_Dither			0
#define MSmTriRaster_RGB565			(UUmPlatform == UUmPlatform_Win32)
#define MSmTriRaster_IsVisible		0
#define MSmTriRaster_RasterZOnly	0
#define MSmTriRaster_Split			1
#define MSmTriRaster_Lightmap		0

#include "BFW_Shared_TriRaster_c.h"
#else
void
MSrDrawContext_Method_TriTextureSplit(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inBaseUVIndex0,
	UUtUns16		inBaseUVIndex1,
	UUtUns16		inBaseUVIndex2,
	UUtUns16		inLightUVIndex0,
	UUtUns16		inLightUVIndex1,
	UUtUns16		inLightUVIndex2)
{
}

#endif
