/*
	FILE:	MD_DC_Method_SmallQuad.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 18, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"

#include "BFW_Shared_TriRaster.h"

#include "MD_DC_Private.h"
#include "MD_DC_Method_Triangle.h"
#include "MD_DC_Method_SmallQuad.h"

void 
MDrDrawContext_Method_SmallQuadGouraudInterpolate(
	M3tDrawContext*	inDrawContext,
	UUtUns32		inIndices)
{
	MDrDrawContext_Method_TriGouraudInterpolate(
		inDrawContext,
		(UUtUns16)(inIndices >> 24),
		(UUtUns16)((inIndices >> 16) & 0xFF),
		(UUtUns16)((inIndices >> 8) & 0xFF));
	MDrDrawContext_Method_TriGouraudInterpolate(
		inDrawContext,
		(UUtUns16)(inIndices >> 24),
		(UUtUns16)((inIndices >> 8) & 0xFF),
		(UUtUns16)(inIndices & 0xFF));
}

void 
MDrDrawContext_Method_SmallQuadGouraudFlat(
	M3tDrawContext*	inDrawContext,
	UUtUns32		inIndices,
	UUtUns16		inFaceShade)
{
	MDrDrawContext_Method_TriGouraudFlat(
		inDrawContext,
		(UUtUns16)(inIndices >> 24),
		(UUtUns16)((inIndices >> 16) & 0xFF),
		(UUtUns16)((inIndices >> 8) & 0xFF),
		inFaceShade);
	MDrDrawContext_Method_TriGouraudFlat(
		inDrawContext,
		(UUtUns16)(inIndices >> 24),
		(UUtUns16)((inIndices >> 8) & 0xFF),
		(UUtUns16)(inIndices & 0xFF),
		inFaceShade);
}

void 
MDrDrawContext_Method_SmallQuadTextureInterpolate(
	M3tDrawContext*	inDrawContext,
	UUtUns32		inIndices)
{
	MDrDrawContext_Method_TriTextureInterpolate(
		inDrawContext,
		(UUtUns16)(inIndices >> 24),
		(UUtUns16)((inIndices >> 16) & 0xFF),
		(UUtUns16)((inIndices >> 8) & 0xFF));
	MDrDrawContext_Method_TriTextureInterpolate(
		inDrawContext,
		(UUtUns16)(inIndices >> 24),
		(UUtUns16)((inIndices >> 8) & 0xFF),
		(UUtUns16)(inIndices & 0xFF));
}

void
MDrDrawContext_Method_SmallQuadTextureFlat(
	M3tDrawContext*	inDrawContext,
	UUtUns32		inIndices,
	UUtUns16		inFaceShade)
{
	MDrDrawContext_Method_TriTextureFlat(
		inDrawContext,
		(UUtUns16)(inIndices >> 24),
		(UUtUns16)((inIndices >> 16) & 0xFF),
		(UUtUns16)((inIndices >> 8) & 0xFF),
		inFaceShade);
	MDrDrawContext_Method_TriTextureFlat(
		inDrawContext,
		(UUtUns16)(inIndices >> 24),
		(UUtUns16)((inIndices >> 8) & 0xFF),
		(UUtUns16)(inIndices & 0xFF),
		inFaceShade);
}
