/*
	FILE:	GL_DC_Method_Frame.h
	
	AUTHOR:	Michael Evans
	
	CREATED: August 10, 1999
	
	PURPOSE: 
	
	Copyright 1999

*/

#ifndef GL_DRAW_FUNCTIONS_H
#define GL_DRAW_FUNCTIONS_H

#include "BFW.h"

void
GLrDrawContext_Method_TriSprite(
	const M3tPointScreen	*inPoints,				// points[3]
	const M3tTextureCoord	*inTextureCoords);		// UVs[3]
	
void
GLrDrawContext_Method_Sprite(
	const M3tPointScreen	*inPoints,				// tl, br
	const M3tTextureCoord	*inTextureCoords);		// tl, tr, bl, br

void
GLrDrawContext_Method_SpriteArray(
	const M3tPointScreen	*inPoints,				// tl, br
	const M3tTextureCoord	*inTextureCoords,		// tl, tr, bl, br
	const UUtUns32			*inColors,
	const UUtUns32			inCount );

UUtError GLrDrawContext_Method_ScreenCapture(
	const UUtRect			*inRect, 
	void					*outBuffer);

UUtBool GLrDrawContext_Method_PointVisible(
	const M3tPointScreen	*inPoint,
	float					inTolerance);

void GLrLine(
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1);

void GLrTriangle_Flat(M3tTri *inTri);
void GLrQuad_Flat(M3tQuad *inQuad);
void GLrPent_Flat(M3tPent *inPent);

void GLrTriangle(M3tTri *inTri);
void GLrQuad(M3tQuad *inQuad);
void GLrPent(M3tPent *inPent);

void GLrTriangle_Gourand(M3tTri *inTri);
void GLrQuad_Gourand(M3tQuad *inQuad);
void GLrPent_Gourand(M3tPent *inPent);

void GLrTriangle_Wireframe(M3tTri *inTri);
void GLrQuad_Wireframe(M3tQuad *inQuad);
void GLrPent_Wireframe(M3tPent *inPent);

void GLrTriangleSplit(M3tTriSplit *inTri);
void GLrQuadSplit(M3tQuadSplit *inQuad);
void GLrPentSplit(M3tPentSplit *inPent);

void GLrTriangleSplit_VertexLighting(M3tTriSplit *inTri);
void GLrQuadSplit_VertexLighting(M3tQuadSplit *inQuad);
void GLrPentSplit_VertexLighting(M3tPentSplit *inPent);

void GLrPoint(M3tPointScreen*	invCoord);

void GLrTriangle_EnvironmentMap_Only(M3tTri *inTri);
void GLrQuad_EnvironmentMap_Only(M3tQuad *inQuad);
void GLrPent_EnvironmentMap_Only(M3tPent *inPent);

void GLrTriangle_EnvironmentMap(M3tTri *inTri);
void GLrQuad_EnvironmentMap(M3tQuad *inQuad);
void GLrPent_EnvironmentMap(M3tPent *inPent);

void GLrTriangle_EnvironmentMap_1TMU(M3tTri *inTri);
void GLrQuad_EnvironmentMap_1TMU(M3tQuad *inQuad);
void GLrPent_EnvironmentMap_1TMU(M3tPent *inPent);

#endif