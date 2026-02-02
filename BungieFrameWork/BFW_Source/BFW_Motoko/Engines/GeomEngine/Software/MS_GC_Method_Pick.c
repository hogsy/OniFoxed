/*
	FILE:	MS_GC_Method_Pick.c

	AUTHOR:	Brent H. Pease

	CREATED: Jan 14, 1998

	PURPOSE:

	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"

#include "BFW_Shared_Math.h"

#include "MS_GC_Private.h"
#include "MS_GC_Method_Pick.h"
#include "MS_Geom_Transform.h"
#include "MS_Geom_Clip.h"
#include "MS_GC_Method_Camera.h"

void
MSrGeomContext_Method_Pick_ScreenToWorld(
	M3tGeomCamera*		inCamera,
	UUtUns16			inScreenX,
	UUtUns16			inScreenY,
	M3tPoint3D			*outWorldZNearPoint)
{
	M3tMatrix4x4*			matrix_frustumToWorld;
	M3tManager_GeomCamera*	targetCamera;

	float		hX, hY;

	float		invW;

	/*
		Frustum to screen code is

		sX = (hX * invW + 1.0f) * scaleX;
		sY = (hY * invW - 1.0f) * scaleY;

		so the screen to frustum code is

		hX = (sX / scaleX - 1.0) * hW
		hY = (sY / scaleY + 1.0) * hW
		hZ = 0.0

		hW = zNear

		XXX - The below code has been optimized more or less beyond recognition - cool.
	*/

	if(inCamera == NULL)
	{
		targetCamera = MSgGeomContextPrivate->activeCamera;
	}
	else
	{
		targetCamera = (M3tManager_GeomCamera*)inCamera;
	}

	M3rManager_Camera_UpdateMatrices(targetCamera);

	matrix_frustumToWorld = &targetCamera->matrix_frustumToWorld;

	invW = 1.0f / matrix_frustumToWorld->m[3][3];
	hX = (inScreenX / MSgGeomContextPrivate->scaleX - 1.0f);
	hY = (inScreenY / MSgGeomContextPrivate->scaleY + 1.0f);

	outWorldZNearPoint->x = (matrix_frustumToWorld->m[0][0] * hX +
								matrix_frustumToWorld->m[1][0] * hY +
								matrix_frustumToWorld->m[3][0]) * invW;
	outWorldZNearPoint->y = (matrix_frustumToWorld->m[0][1] * hX +
								matrix_frustumToWorld->m[1][1] * hY +
								matrix_frustumToWorld->m[3][1]) * invW;
	outWorldZNearPoint->z = (matrix_frustumToWorld->m[0][2] * hX +
								matrix_frustumToWorld->m[1][2] * hY +
								matrix_frustumToWorld->m[3][2]) * invW;

}
