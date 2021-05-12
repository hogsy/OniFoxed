/*
	FILE:	RV_DC_Method_SmallQuad.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 18, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef RV_DC_METHOD_SMALLQUAD_H
#define RV_DC_METHOD_SMALLQUAD_H

void 
RVrDrawContext_Method_SmallQuadGouraudInterpolate(
	UUtUns32		inIndices);

void 
RVrDrawContext_Method_SmallQuadGouraudFlat(
	UUtUns32		inIndices);

void 
RVrDrawContext_Method_SmallQuadTextureInterpolate(
	UUtUns32		inIndices);

void
RVrDrawContext_Method_SmallQuadTextureFlat(
	UUtUns32		inIndices);

void 
RVrDrawContext_Method_SmallQuadLineFlat(
	UUtUns32		inIndices);

#endif /* RV_DC_METHOD_SMALLQUAD_H */
