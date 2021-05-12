/*
	FILE:	MD_DC_Method_SmallQuad.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 18, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef MD_DC_METHOD_SMALLQUAD_H
#define MD_DC_METHOD_SMALLQUAD_H

void 
MDrDrawContext_Method_SmallQuadGouraudInterpolate(
	M3tDrawContext*	inDrawContext,
	UUtUns32		inIndices);

void 
MDrDrawContext_Method_SmallQuadGouraudFlat(
	M3tDrawContext*	inDrawContext,
	UUtUns32		inIndices,
	UUtUns16		inFaceShade);

void 
MDrDrawContext_Method_SmallQuadTextureInterpolate(
	M3tDrawContext*	inDrawContext,
	UUtUns32		inIndices);

void
MDrDrawContext_Method_SmallQuadTextureFlat(
	M3tDrawContext*	inDrawContext,
	UUtUns32		inIndices,
	UUtUns16		inFaceShade);

#endif /* MD_DC_METHOD_SMALLQUAD_H */
