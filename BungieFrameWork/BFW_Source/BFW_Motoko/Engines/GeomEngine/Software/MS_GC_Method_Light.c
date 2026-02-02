/*
	FILE:	MS_GC_Method_Light.c

	AUTHOR:	Brent H. Pease

	CREATED: Sept 19, 1997

	PURPOSE:

	Copyright 1997

*/
#include "BFW.h"
#include "BFW_Motoko.h"

#include "MS_GC_Private.h"
#include "MS_GC_Method_Light.h"

void
MSrGeomContext_Method_LightList_Ambient(
	M3tLight_Ambient*		inLightList)
{

	MSgGeomContextPrivate->light_Ambient = *inLightList;

}

void
MSrGeomContext_Method_LightList_Directional(
	UUtUns32				inNumLights,
	M3tLight_Directional*	inLightList)
{
	UUtUns16	itr;
	float		mag;
	float		nx, ny, nz;

	if(inNumLights > MScDirectionalLight_Max) inNumLights = MScDirectionalLight_Max;

	MSgGeomContextPrivate->light_NumDirectionalLights = inNumLights;

	for(itr = 0; itr < inNumLights; itr++)
	{
		nx = inLightList[itr].direction.x;
		ny = inLightList[itr].direction.y;
		nz = inLightList[itr].direction.x;

		mag = MUrSqrt(nx * nx + ny * ny + nz * nz);

		MSgGeomContextPrivate->light_DirectionalList[itr].color.r =
			inLightList[itr].color.r * mag;
		MSgGeomContextPrivate->light_DirectionalList[itr].color.g =
			inLightList[itr].color.g * mag;
		MSgGeomContextPrivate->light_DirectionalList[itr].color.b =
			inLightList[itr].color.b * mag;

		mag = 1.0f / mag;

		MSgGeomContextPrivate->light_DirectionalList[itr].normalizedDirection.x = nx * mag;
		MSgGeomContextPrivate->light_DirectionalList[itr].normalizedDirection.y = ny * mag;
		MSgGeomContextPrivate->light_DirectionalList[itr].normalizedDirection.z = nz * mag;

	}

}

void
MSrGeomContext_Method_LightList_Point(
	UUtUns32		inNumLights,
	M3tLight_Point*	inLightList)
{
	if(inNumLights > MScPointLight_Max) inNumLights = MScPointLight_Max;



}

void
MSrGeomContext_Method_LightList_Cone(
	UUtUns32				inNumLights,
	M3tLight_Cone*	inLightList)
{
	// add these when they are supported
	//MSgGeomContextPrivate->light_NumConeLights = inNumLights;
	//MSgGeomContextPrivate->light_ConeList = inLightList;

}

