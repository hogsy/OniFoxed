/*
	FILE:	MS_GC_Method_Camera.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 19, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef MS_GC_METHOD_CAMERA_H
#define MS_GC_METHOD_CAMERA_H

#if 0
void
MSrCamera_Initialize(
	void);

void
MSrCamera_UpdateMatrices(
	MStGeomCamera*			inCameraPrivate);
	
UUtError
MSrGeomContext_Method_Camera_New(
	M3tGeomCamera*		*outCamera);

void
MSrGeomContext_Method_Camera_Delete(
	M3tGeomCamera*		inCamera);

void
MSrGeomContext_Method_Camera_SetActive(
	M3tGeomCamera*		inCamera);

UUtError
MSrGeomContext_Method_Camera_GetActive(
	M3tGeomCamera*		*outCamera);

void
MSrGeomContext_Method_Camera_SetStaticData(
	M3tGeomCamera*		inCamera,
	float				inFovy,
	float				inAspect,
	float				inZNear,
	float				inZFar,
	UUtBool				inMirror);

void
MSrGeomContext_Method_Camera_GetStaticData(
	M3tGeomCamera*		inCamera,
	float				*outFovy,
	float				*outAspect,
	float				*outZNear,
	float				*ouZFar,
	UUtBool				*outMirror);

void
MSrGeomContext_Method_Camera_SetViewData(
	M3tGeomCamera*		inCamera,
	M3tPoint3D*			inCameraLocation,
	M3tVector3D*		inCiewVector,
	M3tVector3D*		inUpVector);

void
MSrGeomContext_Method_Camera_GetViewData(
	M3tGeomCamera*		inCamera,
	M3tPoint3D			*outCameraLocation,
	M3tVector3D			*outViewVector,
	M3tVector3D			*outUpVector);

void
MSrGeomContext_Method_Camera_GetWorldFrustum(
	M3tGeomCamera*		inCamera,
	M3tPoint3D			*outPointList,
	M3tPlaneEquation	*outPlaneEquList);

void
MSrGeomContext_Method_Camera_DrawWorldFrustum(
	M3tGeomCamera*		inCamera);
#endif

#endif /* MS_GC_METHOD_CAMERA_H */
