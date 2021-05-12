/*
	FILE:	MD_DC_Method_Quad.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 18, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef MD_DC_METHOD_QUAD_H
#define MD_DC_METHOD_QUAD_H

void 
MDrDrawContext_Method_QuadGouraudInterpolate(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inVIndex3);

void 
MDrDrawContext_Method_QuadGouraudFlat(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inVIndex3,
	UUtUns16		inFaceShade);

void 
MDrDrawContext_Method_QuadTextureInterpolate(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inVIndex3);

void
MDrDrawContext_Method_QuadTextureFlat(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inVIndex3,
	UUtUns16		inFaceShade);

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
	UUtUns16		inLightUVIndex3);

#endif /* MD_DC_METHOD_QUAD_H */
