/*
	FILE:	OG_GC_Method_Light.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: June 10, 1999
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef OG_GC_METHOD_LIGHT_H
#define OG_GC_METHOD_LIGHT_H

void
OGrGeomContext_Method_LightList_Ambient(
	M3tLight_Ambient*		inLightList);

void
OGrGeomContext_Method_LightList_Directional(
	UUtUns32				inNumLights,
	M3tLight_Directional*	inLightList);

void
OGrGeomContext_Method_LightList_Point(
	UUtUns32				inNumLights,
	M3tLight_Point*			inLightList);

void
OGrGeomContext_Method_LightList_Cone(
	UUtUns32				inNumLights,
	M3tLight_Cone*			inLightList);


#endif /* OG_GC_METHOD_LIGHT_H */
