/*
	FILE:	EM_GC_Method_Light.h

	AUTHOR:	Brent H. Pease

	CREATED: June 10, 1999

	PURPOSE:

	Copyright 1997

*/
#ifndef EM_GC_METHOD_LIGHT_H
#define EM_GC_METHOD_LIGHT_H

void
EGrGeomContext_Method_LightList_Ambient(
	M3tLight_Ambient*		inLightList);

void
EGrGeomContext_Method_LightList_Directional(
	UUtUns32				inNumLights,
	M3tLight_Directional*	inLightList);

void
EGrGeomContext_Method_LightList_Point(
	UUtUns32				inNumLights,
	M3tLight_Point*			inLightList);

void
EGrGeomContext_Method_LightList_Cone(
	UUtUns32				inNumLights,
	M3tLight_Cone*			inLightList);


#endif /* EM_GC_METHOD_LIGHT_H */
