/*
	FILE:	MS_GC_Method_Light.h

	AUTHOR:	Brent H. Pease

	CREATED: Sept 19, 1997

	PURPOSE:

	Copyright 1997

*/
#ifndef MS_GC_METHOD_LIGHT_H
#define MS_GC_METHOD_LIGHT_H

void
MSrGeomContext_Method_LightList_Ambient(
	M3tLight_Ambient*		inLightList);

void
MSrGeomContext_Method_LightList_Directional(
	UUtUns32				inNumLights,
	M3tLight_Directional*	inLightList);

void
MSrGeomContext_Method_LightList_Point(
	UUtUns32				inNumLights,
	M3tLight_Point*			inLightList);

void
MSrGeomContext_Method_LightList_Cone(
	UUtUns32				inNumLights,
	M3tLight_Cone*			inLightList);


#endif /* MS_GC_METHOD_LIGHT_H */
