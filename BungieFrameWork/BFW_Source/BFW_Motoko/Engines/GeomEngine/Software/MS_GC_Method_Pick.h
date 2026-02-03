/*
	FILE:	MS_GC_Method_Pick.h

	AUTHOR:	Brent H. Pease

	CREATED: Jan 14, 1998

	PURPOSE:

	Copyright 1997

*/
#ifndef MS_GC_METHOD_PICK_H
#define MS_GC_METHOD_PICK_H

void
MSrGeomContext_Method_Pick_ScreenToWorld(
	M3tGeomCamera*		inCamera,
	UUtUns16			inScreenX,
	UUtUns16			inScreenY,
	M3tPoint3D			*outWorldZNearPoint);


#endif /* MS_GC_METHOD_PICK_H */
