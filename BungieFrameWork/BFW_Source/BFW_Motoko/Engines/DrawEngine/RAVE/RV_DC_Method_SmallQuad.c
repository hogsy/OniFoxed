/*
	FILE:	RV_DC_Method_SmallQuad.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 18, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"

#include "BFW_Shared_TriRaster.h"

#include "RV_DC_Private.h"
#include "RV_DC_Method_Triangle.h"
#include "RV_DC_Method_SmallQuad.h"
#include "RV_DC_Method_Quad.h"

// eventually fill these in

void 
RVrDrawContext_Method_SmallQuadGouraudInterpolate(
	UUtUns32		inIndices)
{
	RVrDrawContext_Method_QuadGouraudInterpolate(
		(UUtUns16) (inIndices >> 24) & 0xff,
		(UUtUns16) (inIndices >> 16) & 0xff,
		(UUtUns16) (inIndices >>  8) & 0xff,
		(UUtUns16) (inIndices >>  0) & 0xff);
}

void 
RVrDrawContext_Method_SmallQuadGouraudFlat(
	UUtUns32		inIndices)
{
	RVrDrawContext_Method_QuadGouraudFlat(
		(UUtUns16) (inIndices >> 24) & 0xff,
		(UUtUns16) (inIndices >> 16) & 0xff,
		(UUtUns16) (inIndices >>  8) & 0xff,
		(UUtUns16) (inIndices >>  0) & 0xff);
}

void 
RVrDrawContext_Method_SmallQuadTextureInterpolate(
	UUtUns32		inIndices)
{
	RVrDrawContext_Method_QuadTextureInterpolate(
		(UUtUns16) (inIndices >> 24) & 0xff,
		(UUtUns16) (inIndices >> 16) & 0xff,
		(UUtUns16) (inIndices >>  8) & 0xff,
		(UUtUns16) (inIndices >>  0) & 0xff);
}

void
RVrDrawContext_Method_SmallQuadTextureFlat(
	UUtUns32		inIndices)
{
	RVrDrawContext_Method_QuadTextureFlat(
		(UUtUns16) (inIndices >> 24) & 0xff,
		(UUtUns16) (inIndices >> 16) & 0xff,
		(UUtUns16) (inIndices >>  8) & 0xff,
		(UUtUns16) (inIndices >>  0) & 0xff);
}

void 
RVrDrawContext_Method_SmallQuadLineFlat(
	UUtUns32		inIndices)
{
	RVrDrawContext_Method_QuadLineFlat(
		(UUtUns16) (inIndices >> 24) & 0xff,
		(UUtUns16) (inIndices >> 16) & 0xff,
		(UUtUns16) (inIndices >>  8) & 0xff,
		(UUtUns16) (inIndices >>  0) & 0xff);
}