/*
	FILE:	MD_DC_Method_Quad.c

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
#include "MD_DC_Method_Quad.h"

void
MDrDrawContext_Method_QuadGouraudInterpolate(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inVIndex3)
{
	MDrDrawContext_Method_TriGouraudInterpolate(
		inDrawContext,
		inVIndex0,
		inVIndex1,
		inVIndex2);
	MDrDrawContext_Method_TriGouraudInterpolate(
		inDrawContext,
		inVIndex0,
		inVIndex2,
		inVIndex3);
}

void
MDrDrawContext_Method_QuadGouraudFlat(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inVIndex3,
	UUtUns16		inFaceShade)
{
	MDrDrawContext_Method_TriGouraudFlat(
		inDrawContext,
		inVIndex0,
		inVIndex1,
		inVIndex2,
		inFaceShade);
	MDrDrawContext_Method_TriGouraudFlat(
		inDrawContext,
		inVIndex0,
		inVIndex2,
		inVIndex3,
		inFaceShade);

}

void
MDrDrawContext_Method_QuadTextureInterpolate(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inVIndex3)
{
	MDrDrawContext_Method_TriTextureInterpolate(
		inDrawContext,
		inVIndex0,
		inVIndex1,
		inVIndex2);
	MDrDrawContext_Method_TriTextureInterpolate(
		inDrawContext,
		inVIndex0,
		inVIndex2,
		inVIndex3);

}

void
MDrDrawContext_Method_QuadTextureFlat(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inVIndex3,
	UUtUns16		inFaceShade)
{
	MDrDrawContext_Method_TriTextureFlat(
		inDrawContext,
		inVIndex0,
		inVIndex1,
		inVIndex2,
		inFaceShade);
	MDrDrawContext_Method_TriTextureFlat(
		inDrawContext,
		inVIndex0,
		inVIndex2,
		inVIndex3,
		inFaceShade);

}

void
MDrDrawContext_Method_QuadTextureSplit(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inVIndex3,
	UUtUns16		inBaseUVIndex0,
	UUtUns16		inBaseUVIndex1,
	UUtUns16		inBaseUVIndex2,
	UUtUns16		inBaseUVIndex3,
	UUtUns16		inLightUVIndex0,
	UUtUns16		inLightUVIndex1,
	UUtUns16		inLightUVIndex2,
	UUtUns16		inLightUVIndex3)
{
	MDrDrawContext_Method_TriTextureSplit(
		inDrawContext,
		inVIndex0,
		inVIndex1,
		inVIndex2,
		inBaseUVIndex0,
		inBaseUVIndex1,
		inBaseUVIndex2,
		inLightUVIndex0,
		inLightUVIndex1,
		inLightUVIndex2);
	MDrDrawContext_Method_TriTextureSplit(
		inDrawContext,
		inVIndex0,
		inVIndex2,
		inVIndex3,
		inBaseUVIndex0,
		inBaseUVIndex2,
		inBaseUVIndex3,
		inLightUVIndex0,
		inLightUVIndex2,
		inLightUVIndex3);
}
