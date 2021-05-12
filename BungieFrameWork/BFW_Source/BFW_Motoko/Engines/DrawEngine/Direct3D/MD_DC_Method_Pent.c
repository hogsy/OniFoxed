/*
	FILE:	MD_DC_Method_Pent.c

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
#include "MD_DC_Method_Pent.h"

void 
MDrDrawContext_Method_PentGouraudInterpolate(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inVIndex3,
	UUtUns16		inVIndex4)
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
	MDrDrawContext_Method_TriGouraudInterpolate(
		inDrawContext,
		inVIndex0,
		inVIndex3,
		inVIndex4);

}

void 
MDrDrawContext_Method_PentGouraudFlat(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inVIndex3,
	UUtUns16		inVIndex4,
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
	MDrDrawContext_Method_TriGouraudFlat(
		inDrawContext,
		inVIndex0,
		inVIndex3,
		inVIndex4,
		inFaceShade);

}

void 
MDrDrawContext_Method_PentTextureInterpolate(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inVIndex3,
	UUtUns16		inVIndex4)
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
	MDrDrawContext_Method_TriTextureInterpolate(
		inDrawContext,
		inVIndex0,
		inVIndex3,
		inVIndex4);

}

void
MDrDrawContext_Method_PentTextureFlat(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inVIndex3,
	UUtUns16		inVIndex4,
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
	MDrDrawContext_Method_TriTextureFlat(
		inDrawContext,
		inVIndex0,
		inVIndex3,
		inVIndex4,
		inFaceShade);

}

void
MDrDrawContext_Method_PentTextureSplit(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inVIndex3,
	UUtUns16		inVIndex4,
	UUtUns16		inBaseUVIndex0,
	UUtUns16		inBaseUVIndex1,
	UUtUns16		inBaseUVIndex2,
	UUtUns16		inBaseUVIndex3,
	UUtUns16		inBaseUVIndex4,
	UUtUns16		inLightUVIndex0,
	UUtUns16		inLightUVIndex1,
	UUtUns16		inLightUVIndex2,
	UUtUns16		inLightUVIndex3,
	UUtUns16		inLightUVIndex4)
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
	MDrDrawContext_Method_TriTextureSplit(
		inDrawContext,
		inVIndex0,
		inVIndex3,
		inVIndex4,
		inBaseUVIndex0,
		inBaseUVIndex3,
		inBaseUVIndex4,
		inLightUVIndex0,
		inLightUVIndex3,
		inLightUVIndex4);
}

