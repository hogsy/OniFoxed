/*
	FILE:	MG_DC_Method_SmallQuad.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 18, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef MG_DC_METHOD_SMALLQUAD_H
#define MG_DC_METHOD_SMALLQUAD_H

void 
MGrDrawContext_Method_SmallQuadGouraudInterpolate(
	UUtUns32		inIndices);

void 
MGrDrawContext_Method_SmallQuadGouraudFlat(
	UUtUns32		inIndices);

void 
MGrDrawContext_Method_SmallQuadTextureInterpolate(
	UUtUns32		inIndices);

void
MGrDrawContext_Method_SmallQuadTextureFlat(
	UUtUns32		inIndices);

void 
MGrDrawContext_Method_SmallQuadLineFlat(
	UUtUns32		inIndices);

void 
MGrDrawContext_Method_SmallQuadLineInterp(
	UUtUns32		inIndices);

#endif /* MG_DC_METHOD_SMALLQUAD_H */
