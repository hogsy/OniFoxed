/*
	FILE:	EM_DC_Method_LinePoint.h

	AUTHOR:	Brent H. Pease

	CREATED: Sept 13, 1997

	PURPOSE:

	Copyright 1997

*/
#ifndef EM_DC_METHOD_LINEPOINT_H
#define EM_DC_METHOD_LINEPOINT_H

void
EDrDrawContext_Method_Point(
	M3tPointScreen*	invCoord);

void
EDrDrawContext_Method_Line_Interpolate(
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1);

void
EDrDrawContext_Method_Line_Flat(
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1);


#endif
