/*
	FILE:	MG_DC_Method_LinePoint.h

	AUTHOR:	Brent H. Pease

	CREATED: Sept 13, 1997

	PURPOSE:

	Copyright 1997

*/
#ifndef MG_DC_METHOD_LINEPOINT_H
#define MG_DC_METHOD_LINEPOINT_H

void
MGrPoint(
	M3tPointScreen*	invCoord);

void
MGrLine_InterpVertex(
	UUtUns32		inVIndex0,
	UUtUns32		inVIndex1);

void
MGrLine_InterpNone(
	UUtUns32		inVIndex0,
	UUtUns32		inVIndex1);


#endif
