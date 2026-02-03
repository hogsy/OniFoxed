/*
	FILE:	Motoko_Sort.h

	AUTHOR:	Brent H. Pease

	CREATED: Dec 12, 1998

	PURPOSE: Interface to the Motoko 3D engine

	Copyright 1997

*/
#ifndef MOTOKO_DRAW_H
#define MOTOKO_DRAW_H

UUtError
M3rSort_Frame_Start(
	void);

UUtError
M3rSort_Frame_End(
	void);

void
M3rSort_Draw_Triangle(
	M3tTri*	inTri);

void
M3rSort_Draw_TriSplit(
	M3tTriSplit*	inTri);

void
M3rSort_Draw_Quad(
	M3tQuad*	inQuad);

void
M3rSort_Draw_QuadSplit(
	M3tQuadSplit*	inQuad);

void
M3rSort_Draw_Pent(
	M3tPent*	inPent);

void
M3rSort_Draw_PentSplit(
	M3tPentSplit*	inPent);

void
M3rSort_Draw_Sprite(
	const M3tPointScreen	*inPoints,
	const M3tTextureCoord	*inTextureCoords);		// tl, tr, bl, br

UUtError
M3rSort_Initialize(
	void);

void
M3rSort_Terminate(
	void);

#endif /* MOTOKO_DRAW_H */
