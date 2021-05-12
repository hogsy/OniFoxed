/*
	FILE:	RV_DC_Method_LinePoint.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 13, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef RV_DC_METHOD_LINEPOINT_H
#define RV_DC_METHOD_LINEPOINT_H

void 
RVrDrawContext_Method_Point(
	M3tPointScreen*	invCoord);

void 
RVrDrawContext_Method_Line_Interpolate(
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1);

void 
RVrDrawContext_Method_Line_Flat(
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1);


#endif
