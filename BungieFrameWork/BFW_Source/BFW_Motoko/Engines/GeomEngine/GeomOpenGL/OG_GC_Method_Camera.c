/*
	FILE:	OG_GC_Method_Camera.c

	AUTHOR:	Brent H. Pease

	CREATED: June 10, 1999

	PURPOSE:

	Copyright 1997

*/
#if 0

#include "BFW.h"
#include "BFW_Motoko.h"

#include "OG_GC_Private.h"
#include "OG_GC_Method_Camera.h"

UUtError
OGrGeomContext_Method_Camera_New(
	M3tGeomCamera*		*outCamera)
{

	return UUcError_None;
}

void
OGrGeomContext_Method_Camera_Delete(
	M3tGeomCamera*		inCamera)
{
}

void
OGrGeomContext_Method_Camera_SetActive(
	M3tGeomCamera*		inCamera)
{
}

UUtError
OGrGeomContext_Method_Camera_GetActive(
	M3tGeomCamera*		*outCamera)
{

	return UUcError_None;
}

void
OGrGeomContext_Method_Camera_SetStaticData(
	M3tGeomCamera*		inCamera,
	float				inFOVy,
	float				inAspect,
	float				inZNear,
	float				inZFar,
	UUtBool				inMirror)
{

}

void
OGrGeomContext_Method_Camera_GetStaticData(
	M3tGeomCamera*		inCamera,
	float				*outFovy,
	float				*outAspect,
	float				*outZNear,
	float				*outZFar,
	UUtBool				*outMirror)
{

}

void
OGrGeomContext_Method_Camera_SetViewData(
	M3tGeomCamera*		inCamera,
	M3tPoint3D*			inCameraLocation,
	M3tVector3D*		inViewVector,
	M3tVector3D*		inUpVector)
{

}

void
OGrGeomContext_Method_Camera_GetViewData(
	M3tGeomCamera*		inCamera,
	M3tPoint3D			*outCameraLocation,
	M3tVector3D			*outViewVector,
	M3tVector3D			*outUpVector)
{

}

void
OGrGeomContext_Method_Camera_GetWorldFrustum(
	M3tGeomCamera*		inCamera,
	M3tPoint3D			*outPointList,
	M3tPlaneEquation	*outPlaneEquList)
{

}

void
OGrGeomContext_Method_Camera_DrawWorldFrustum(
	M3tGeomCamera*		inCamera)
{

}
#endif
