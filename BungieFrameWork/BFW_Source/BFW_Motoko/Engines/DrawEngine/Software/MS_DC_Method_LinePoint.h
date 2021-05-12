/*
	FILE:	MS_DC_Method_LinePoint.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 13, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef MS_DC_METHOD_LINEPOINT_H
#define MS_DC_METHOD_LINEPOINT_H

void 
MSrDrawContext_Method_Point(
	M3tDrawContext*	inDrawContext,
	M3tPointScreen*	invCoord,
	UUtUns16		inVShade);

void 
MSrDrawContext_Method_Line_Interpolate(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1);

void 
MSrDrawContext_Method_Line_Flat(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inShade);


#endif
