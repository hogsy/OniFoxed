/*
	FILE:	OG_GC_Method_Pick.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: June 10, 1999
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef OG_GC_METHOD_PICK_H
#define OG_GC_METHOD_PICK_H

void
OGrGeomContext_Method_Pick_ScreenToWorld(
	M3tGeomCamera*		inCamera,
	UUtUns16			inScreenX,
	UUtUns16			inScreenY,
	M3tPoint3D			*outWorldZNearPoint);


#endif /* OG_GC_METHOD_PICK_H */
