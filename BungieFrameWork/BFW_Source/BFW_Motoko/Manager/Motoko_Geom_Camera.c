/*
	FILE:	Motoko_Geom_Camera.c

	AUTHOR:	Brent H. Pease

	CREATED: OCt 26, 1999

	PURPOSE: Interface to the Motoko 3D engine

	Copyright 1997

*/

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_MathLib.h"
#include "BFW_Shared_Math.h"

#include "Motoko_Manager.h"
#include "Motoko_Private.h"

/*
The following derivation is based on Jim Blinn's given in "Where Am I?
What Am I Looking At?" July 1988. With a few corrections, of course.

Given:
	* camera location, Cl = (Clx, Cly, Clz) in world space
	* normalized viewing vector, Vv = (Vvx, Vvy, Vvz)
	* normalized up vector, Uv = (Uvx, Uvy, Uvz)
    * Vv and Uv are perpendicular

Wanted:
	A world space to view space 4x4 transformation matrix such that the
	input world vertices are transformed into the view space where the camera
	is located at the origin, the up vector is (0, 1, 0), and the view
	vector is (0, 0, -1). ie up is the positive y direction and we are
	looking down the negative Z axis. The right hand rule dictates that
	positive Z is comming out of the screen

	In equation form we want a matrix that satisfies the following constraints

	M * (Cl + Vv) = Transpose[0, 0, -1, 1]
	M * (Cl + Uv) = Transpose[0, 1, 0, 1]
	M * Cl = Transpose[0, 0, 0, 1]

Derivation:

	The resulting matrix can be broken down into 2 matrices, one for translation
	and one for rotation.

	M = Mrot * Mtrans

	This means that the translation happens *before* the rotation.

	Translating the vertex to the camera origin is trivial so lets move on to
	the more interesting problem of rotation.

	Since we only need a pure rotation matrix lets just consider Mrot to be
	a 3x3 matrix for now. Later we will but it back into a 4x4 matrix.

	Now the constraining equations are:

	Mrot * Vv = Transpose[0, 0, -1]
	Mrot * Uv = Transpose[0, 1, 0]

	Being a resourceful engineer in the gamming business, I happened to have
	read Jim Blinn's article "Where Am I? What Am I Looking At?", so I know that
	the transpose of M equals the inverse of M when M is a purley rotational
	matrix. This is good news.

	Therefor,

		Vv = Transpose[Mrot] * Transpose[0, 0, -1]
		Uv = Transpose[Mrot] * Transpose[0, 1, 0]

	This means that the 3rd column of Transpose[Mrot] is -Vv and the second
	column of Transpose[Mrot] is Uv.

	Now we still have one column left. This is where Jim and I are going to
	wave our hands a little bit. Since the y axis cross the z axis equals
	the x axis we know that Uv cross Vv belongs in the first column of
	Transpose[M]. ta-da!

	Nv = Uv cross -Vv

	Transpose[Mrot] = [ Nv Uv -Vv ]

	Mrot = --               --
	       |  Nvx  Nvy  Nvz  |
	       |  Uvx  Uvy  Uvz  |
	       | -Vvx -Vvy -Vvz  |
	       --               --

	Mtrans = --            --
	         |  1 0 0 -Clx  |
	         |  0 1 0 -Cly  |
	         |  0 0 1 -Clz  |
	         |  0 0 0 1     |
	         --            --

	M = --                                            --
	    |  Nvx  Nvy  Nvz  -(NvxClx + NvyCly + NvzClz)  |
	    |  Uvx  Uvy  Uvz  -(UvxClx + UvyCly + UvzClz)  |
	    | -Vvx -Vvy -Vvz   (VvxClx + VvyCly + VvzClz)  |
	    |  0    0    0     1                           |
	    --                                            --

	If you take the time to solve M * (Cl + Vv), etc you will
	find that you get the right answers(providing that you remember
	that UvxVvx + UvyVvy + UvzVvz is equal to Uv • Vv which by definition
	is 0)

*/

	/* camera gunk */
M3tManager_GeomCamera	M3gManager_Cameras[M3cManager_MaxCameras];
M3tManager_GeomCamera*	M3gManager_ActiveCamera = NULL;

static void
MSiCamera_UpdateWorldFrustum(
	M3tManager_GeomCamera*			inCameraPrivate)
{
	float					cos_fovyo2;
	float					sin_fovyo2;
	float					fovx02;
	float					cos_fovxo2;
	float					sin_fovxo2;

	float		m00, m10, m20, m30;
	float		m01, m11, m21, m31;
	float		m02, m12, m22, m32;
	float		m03, m13, m23, m33;

	float		t0, t1, t2;
	float		s0, s1, s2;
	float		u0, u1, u2;
	float		invW;

	UUtUns16	curIndex;
	float		vx, vy, vz;
	float		wx, wy, wz;
	float		d;

	UUmAssert(inCameraPrivate != NULL);

	M3rManager_Camera_UpdateMatrices(inCameraPrivate);

	if(inCameraPrivate->vfnDirty == UUcTrue)
	{
		inCameraPrivate->vfnDirty = UUcFalse;

		/*
		 	0		perp to zNear plane(toward camera)
		 	1		right side
		 	2		left side
		 	3		bottom side
		 	4		top side
		 	5		perp to zFar plane(away from camera)
		 */

		cos_fovyo2 = MUrCos(inCameraPrivate->fovy * 0.5f);
		sin_fovyo2 = MUrSin(inCameraPrivate->fovy * 0.5f);

		fovx02 = MUrATan(inCameraPrivate->aspect * sin_fovyo2 / cos_fovyo2);

		cos_fovxo2 = MUrCos(fovx02);
		sin_fovxo2 = MUrSin(fovx02);

		// perp to zNear plane(toward camera)
		inCameraPrivate->viewFrustumNormals[0].x = 0.0f;
		inCameraPrivate->viewFrustumNormals[0].y = 0.0f;
		inCameraPrivate->viewFrustumNormals[0].z = 1.0f;

		//right side
		inCameraPrivate->viewFrustumNormals[1].x = cos_fovxo2;
		inCameraPrivate->viewFrustumNormals[1].y = 0.0f;
		inCameraPrivate->viewFrustumNormals[1].z = sin_fovxo2;

		//left side
		inCameraPrivate->viewFrustumNormals[2].x = -cos_fovxo2;
		inCameraPrivate->viewFrustumNormals[2].y = 0.0f;
		inCameraPrivate->viewFrustumNormals[2].z = sin_fovxo2;

		//bottom side
		inCameraPrivate->viewFrustumNormals[3].x = 0.0f;
		inCameraPrivate->viewFrustumNormals[3].y = -cos_fovyo2;
		inCameraPrivate->viewFrustumNormals[3].z = sin_fovyo2;

		//top side
		inCameraPrivate->viewFrustumNormals[4].x = 0.0f;
		inCameraPrivate->viewFrustumNormals[4].y = cos_fovyo2;
		inCameraPrivate->viewFrustumNormals[4].z = sin_fovyo2;

		//perp to zFar plane(away from camera)
		inCameraPrivate->viewFrustumNormals[5].x = 0.0f;
		inCameraPrivate->viewFrustumNormals[5].y = 0.0f;
		inCameraPrivate->viewFrustumNormals[5].z = -1.0f;

		inCameraPrivate->wfpDirty = UUcTrue;
	}

	if(inCameraPrivate->wfpDirty == UUcTrue)
	{
		inCameraPrivate->wfpDirty = UUcFalse;

		MSmTransform_Matrix4x4ToRegisters(
			inCameraPrivate->matrix_frustumToWorld,
			m00, m10, m20, m30,
			m01, m11, m21, m31,
			m02, m12, m22, m32,
			m03, m13, m23, m33);

		invW = 1.0f / m33;

		s0 = (m30 + m10) * invW;
		s1 = (m31 + m11) * invW;
		s2 = (m32 + m12) * invW;

		u0 = m00 * invW;
		u1 = m01 * invW;
		u2 = m02 * invW;

		//0		left	up		near
			inCameraPrivate->worldFrustumPoints[0].x = s0 - u0;
			inCameraPrivate->worldFrustumPoints[0].y = s1 - u1;
			inCameraPrivate->worldFrustumPoints[0].z = s2 - u2;

		//1		right	up		near
			inCameraPrivate->worldFrustumPoints[1].x = s0 + u0;
			inCameraPrivate->worldFrustumPoints[1].y = s1 + u1;
			inCameraPrivate->worldFrustumPoints[1].z = s2 + u2;

		s0 = (m30 - m10) * invW;
		s1 = (m31 - m11) * invW;
		s2 = (m32 - m12) * invW;

		//2		left	down	near
			inCameraPrivate->worldFrustumPoints[2].x = s0 - u0;
			inCameraPrivate->worldFrustumPoints[2].y = s1 - u1;
			inCameraPrivate->worldFrustumPoints[2].z = s2 - u2;

		//3		right	down	near
			inCameraPrivate->worldFrustumPoints[3].x = s0 + u0;
			inCameraPrivate->worldFrustumPoints[3].y = s1 + u1;
			inCameraPrivate->worldFrustumPoints[3].z = s2 + u2;

		invW = 1.0f / ((m23 + m33));
		t0 = (m20 + m30);
		t1 = (m21 + m31);
		t2 = (m22 + m32);

		s0 = (t0 + m10) * invW;
		s1 = (t1 + m11) * invW;
		s2 = (t2 + m12) * invW;

		u0 = m00 * invW;
		u1 = m01 * invW;
		u2 = m02 * invW;

		//4		left	up		far
			inCameraPrivate->worldFrustumPoints[4].x = s0 - u0;
			inCameraPrivate->worldFrustumPoints[4].y = s1 - u1;
			inCameraPrivate->worldFrustumPoints[4].z = s2 - u2;

		//5		right	up		far
			inCameraPrivate->worldFrustumPoints[5].x = s0 + u0;
			inCameraPrivate->worldFrustumPoints[5].y = s1 + u1;
			inCameraPrivate->worldFrustumPoints[5].z = s2 + u2;

		s0 = (t0 - m10) * invW;
		s1 = (t1 - m11) * invW;
		s2 = (t2 - m12) * invW;

		//6		left	down	far
			inCameraPrivate->worldFrustumPoints[6].x = s0 - u0;
			inCameraPrivate->worldFrustumPoints[6].y = s1 - u1;
			inCameraPrivate->worldFrustumPoints[6].z = s2 - u2;

		//7		right	down	far
			inCameraPrivate->worldFrustumPoints[7].x = s0 + u0;
			inCameraPrivate->worldFrustumPoints[7].y = s1 + u1;
			inCameraPrivate->worldFrustumPoints[7].z = s2 + u2;

		/*
		 * now transform all the normals and compute the plane equations
		 */
			MSmTransform_Matrix3x3ToRegisters(
				inCameraPrivate->matrix_viewToWorld,
				m00, m10, m20,
				m01, m11, m21,
				m02, m12, m22);

		 	for(curIndex = 0; curIndex < 6; curIndex++)
		 	{
				vx = inCameraPrivate->viewFrustumNormals[curIndex].x;
				vy = inCameraPrivate->viewFrustumNormals[curIndex].y;
				vz = inCameraPrivate->viewFrustumNormals[curIndex].z;

				wx = m00 * vx + m10 * vy + m20 * vz;
				wy = m01 * vx + m11 * vy + m21 * vz;
				wz = m02 * vx + m12 * vy + m22 * vz;

				d = -(inCameraPrivate->worldFrustumPoints[curIndex].x * wx +
						inCameraPrivate->worldFrustumPoints[curIndex].y * wy +
						inCameraPrivate->worldFrustumPoints[curIndex].z * wz);

				inCameraPrivate->worldFrustumPlanes[curIndex].a = wx;
				inCameraPrivate->worldFrustumPlanes[curIndex].b = wy;
				inCameraPrivate->worldFrustumPlanes[curIndex].c = wz;
				inCameraPrivate->worldFrustumPlanes[curIndex].d = d;
			}
	}
}

void
M3rManager_Camera_UpdateMatrices(
	M3tManager_GeomCamera*	inCamera)
{

	UUmAssert(inCamera != NULL);
	UUmAssert(inCamera->staticSet == UUcTrue && inCamera->viewSet == UUcTrue);

	if(inCamera->staticDirty == UUcTrue)
	{
		inCamera->staticDirty = UUcFalse;

		{
			float			fi, f;
			float			ti, t;
			M3tMatrix4x4*	viewToFrustum = &inCamera->matrix_viewToFrustum;
			M3tMatrix4x4*	frustumToView = &inCamera->matrix_frustumToView;

			fi = (float)tan(inCamera->fovy * 0.5f);
			f = 1.0f / fi;
			ti = (inCamera->zNear - inCamera->zFar);
			t = 1.0f / ti;

			/* column 1 */
			viewToFrustum->m[0][0] = f / inCamera->aspect;
			viewToFrustum->m[0][1] = 0.0f;
			viewToFrustum->m[0][2] = 0.0f;
			viewToFrustum->m[0][3] = 0.0f;

			/* column 2 */
			viewToFrustum->m[1][0] = 0.0f;
			viewToFrustum->m[1][1] = f;
			viewToFrustum->m[1][2] = 0.0f;
			viewToFrustum->m[1][3] = 0.0f;

			/* column 3 */
			viewToFrustum->m[2][0] = 0.0f;
			viewToFrustum->m[2][1] = 0.0f;
			viewToFrustum->m[2][2] = (inCamera->zFar + inCamera->zNear) * t;
			viewToFrustum->m[2][3] = -1.0f;

			/* column 4 */
			viewToFrustum->m[3][0] = 0.0f;
			viewToFrustum->m[3][1] = 0.0f;
			viewToFrustum->m[3][2] = 2.0f * inCamera->zFar * inCamera->zNear * t;
			viewToFrustum->m[3][3] = 0.0f;

			/* column 1 */
			frustumToView->m[0][0] = fi * inCamera->aspect;
			frustumToView->m[0][1] = 0.0f;
			frustumToView->m[0][2] = 0.0f;
			frustumToView->m[0][3] = 0.0f;

			/* column 2 */
			frustumToView->m[1][0] = 0.0f;
			frustumToView->m[1][1] = fi;
			frustumToView->m[1][2] = 0.0f;
			frustumToView->m[1][3] = 0.0f;

			/* column 3 */
			frustumToView->m[2][0] = 0.0f;
			frustumToView->m[2][1] = 0.0f;
			frustumToView->m[2][2] = 0.0f;
			frustumToView->m[2][3] = 1.0f / viewToFrustum->m[3][2];

			/* column 4 */
			frustumToView->m[3][0] = 0.0f;
			frustumToView->m[3][1] = 0.0f;
			frustumToView->m[3][2] = -1.0f;
			frustumToView->m[3][3] = (inCamera->zFar + inCamera->zNear) / (2.0f * inCamera->zFar * inCamera->zNear);
		}

		inCamera->viewDirty = UUcTrue;
		inCamera->vfnDirty = UUcTrue;
	}

	if(inCamera->viewDirty == UUcTrue)
	{
		inCamera->viewDirty = UUcFalse;

		{
			float			Nvx, Nvy, Nvz;
			float			Uvx, Uvy, Uvz;
			float			Vvx, Vvy, Vvz;
			float			Clx, Cly, Clz;
			float			invDenom;
			M3tMatrix4x4*	worldToView = &inCamera->matrix_worldToView;
			M3tMatrix4x4*	viewToWorld = &inCamera->matrix_viewToWorld;

			Vvx = inCamera->viewVector.x;
			Vvy = inCamera->viewVector.y;
			Vvz = inCamera->viewVector.z;

			Uvx = inCamera->upVector.x;
			Uvy = inCamera->upVector.y;
			Uvz = inCamera->upVector.z;

			Clx = inCamera->cameraLocation.x;
			Cly = inCamera->cameraLocation.y;
			Clz = inCamera->cameraLocation.z;

			Nvx = Uvz * Vvy - Uvy * Vvz;
			Nvy = Uvx * Vvz - Uvz * Vvx;
			Nvz = Uvy * Vvx - Uvx * Vvy;

			inCamera->crossVector.x = Nvx;
			inCamera->crossVector.y = Nvy;
			inCamera->crossVector.z = Nvz;

			invDenom = 1.0f / (Nvy * Uvz * Vvx - Nvz * Uvy * Vvx + Nvz * Uvx * Vvy -
					Nvx * Uvz * Vvy - Nvy * Uvx * Vvz + Nvx * Uvy * Vvz);

			/* column 1 */
			worldToView->m[0][0] = Nvx;
			worldToView->m[0][1] = Uvx;
			worldToView->m[0][2] = -Vvx;
			worldToView->m[0][3] = 0.0f;

			/* column 2 */
			worldToView->m[1][0] = Nvy;
			worldToView->m[1][1] = Uvy;
			worldToView->m[1][2] = -Vvy;
			worldToView->m[1][3] = 0.0f;

			/* column 3 */
			worldToView->m[2][0] = Nvz;
			worldToView->m[2][1] = Uvz;
			worldToView->m[2][2] = -Vvz;
			worldToView->m[2][3] = 0.0f;

			/* column 4 */
			worldToView->m[3][0] = - (Nvx * Clx + Nvy * Cly + Nvz * Clz);
			worldToView->m[3][1] = - (Uvx * Clx + Uvy * Cly + Uvz * Clz);
			worldToView->m[3][2] =   (Vvx * Clx + Vvy * Cly + Vvz * Clz);
			worldToView->m[3][3] = 1.0f;

			/* column 1 */
			viewToWorld->m[0][0] = (Uvy * Vvz - Uvz * Vvy) * invDenom;
			viewToWorld->m[0][1] = (Uvz * Vvx - Uvx * Vvz) * invDenom;
			viewToWorld->m[0][2] = (Uvx * Vvy - Uvy * Vvx) * invDenom;
			viewToWorld->m[0][3] = 0.0f;

			/* column 2 */
			viewToWorld->m[1][0] = (Nvz * Vvy - Nvy * Vvz) * invDenom;
			viewToWorld->m[1][1] = (Nvx * Vvz - Nvz * Vvx) * invDenom;
			viewToWorld->m[1][2] = (Nvy * Vvx - Nvx * Vvy) * invDenom;
			viewToWorld->m[1][3] = 0.0f;

			/* column 3 */
			viewToWorld->m[2][0] = (Nvz * Uvy - Nvy * Uvz) * invDenom;
			viewToWorld->m[2][1] = (Nvx * Uvz - Nvz * Uvx) * invDenom;
			viewToWorld->m[2][2] = (Nvy * Uvx - Nvx * Uvy) * invDenom;
			viewToWorld->m[2][3] = 0.0f;

			/* column 4 */
			viewToWorld->m[3][0] = Clx;
			viewToWorld->m[3][1] = Cly;
			viewToWorld->m[3][2] = Clz;
			viewToWorld->m[3][3] = 1.0f;
		}

		MUrMath_Matrix4x4Multiply(
			&inCamera->matrix_viewToFrustum,
			&inCamera->matrix_worldToView,
			&inCamera->matrix_worldToFrustum);

		MUrMath_Matrix4x4Multiply(
			&inCamera->matrix_viewToWorld,
			&inCamera->matrix_frustumToView,
			&inCamera->matrix_frustumToWorld);

		inCamera->wfpDirty = UUcTrue;

		#if defined(DEBUGGING) && DEBUGGING
		{
			UUtUns16	i, j;

			for(i = 0; i < 4; i++)
			{
				for(j = 0; j < 4; j++)
				{
					UUmAssert(inCamera->matrix_viewToFrustum.m[i][j] > -1e9f);
					UUmAssert(inCamera->matrix_viewToFrustum.m[i][j] <  1e9f);
					UUmAssert(inCamera->matrix_frustumToView.m[i][j] > -1e9f);
					UUmAssert(inCamera->matrix_frustumToView.m[i][j] <  1e9f);
					UUmAssert(inCamera->matrix_worldToFrustum.m[i][j] > -1e9f);
					UUmAssert(inCamera->matrix_worldToFrustum.m[i][j] <  1e9f);
				}
			}
		}
		#endif

	}
}

void
M3rManager_Camera_Initialize(
	void)
{
	UUtUns32				curCameraIndex;
	M3tManager_GeomCamera*	curCamera;

	M3gManager_ActiveCamera = NULL;

	for(curCameraIndex = 0, curCamera = M3gManager_Cameras;
		curCameraIndex < M3cManager_MaxCameras;
		curCameraIndex++, curCamera++)
	{
		curCamera->inUse = UUcFalse;
	}
}

UUtError
M3rCamera_New(
	M3tGeomCamera*		*outNewCamera)
{
	UUtUns32				curCameraIndex;
	M3tManager_GeomCamera*	curCamera;

	for(curCameraIndex = 0, curCamera = M3gManager_Cameras;
		curCameraIndex < M3cManager_MaxCameras;
		curCameraIndex++, curCamera++)
	{
		if(curCamera->inUse == UUcFalse)
		{
			curCamera->inUse = UUcTrue;

			curCamera->staticSet = UUcFalse;
			curCamera->viewSet = UUcFalse;

			*outNewCamera = (M3tGeomCamera*)curCamera;
			return UUcError_None;
		}
	}

	UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Ran out of camera space");
}

void
M3rCamera_Delete(
	M3tGeomCamera*		inCamera)
{
	M3tManager_GeomCamera*		curCamera;

	curCamera = (M3tManager_GeomCamera*)inCamera;
	UUmAssert(curCamera != NULL && curCamera->inUse == UUcTrue);
	curCamera->inUse = UUcFalse;
}

void
M3rCamera_SetActive(
	M3tGeomCamera*		inCamera)
{
	M3tManager_GeomCamera*		curCamera;

	curCamera = (M3tManager_GeomCamera*)inCamera;

	UUmAssertReadPtr(curCamera, sizeof(*curCamera));
	UUmAssert(curCamera->inUse == UUcTrue);

	M3gManager_ActiveCamera = curCamera;
}

UUtError
M3rCamera_GetActive(
	M3tGeomCamera*		*outCamera)
{
	if(M3gManager_ActiveCamera == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "No active camera");
	}

	*outCamera = (M3tGeomCamera*)M3gManager_ActiveCamera;

	return UUcError_None;
}

void
M3rCamera_SetStaticData(
	M3tGeomCamera*		inCamera,
	float				inFOVy,
	float				inAspect,
	float				inZNear,
	float				inZFar)
{
	M3tManager_GeomCamera*			curCamera;

	curCamera = (M3tManager_GeomCamera*)inCamera;

	UUmAssert(curCamera != NULL && curCamera->inUse == UUcTrue);

	curCamera->fovy = inFOVy;
	curCamera->aspect = inAspect;
	curCamera->zNear = inZNear;
	curCamera->zFar = inZFar;

	curCamera->staticSet = UUcTrue;
	curCamera->staticDirty = UUcTrue;

	UUmAssert(M3gActiveGeomEngine != UUcMaxUns16);
	M3gGeomEngineList[M3gActiveGeomEngine].methods.cameraStaticUpdate();
}

void
M3rCamera_SetViewData(
	M3tGeomCamera*		inCamera,
	M3tPoint3D*			inCameraLocation,
	M3tVector3D*		inViewDirection,
	M3tVector3D*		inUpDirection)
{
	M3tManager_GeomCamera*			curCamera;

	curCamera = (M3tManager_GeomCamera*)inCamera;

	UUmAssert(curCamera != NULL && curCamera->inUse == UUcTrue);

	UUmAssert(curCamera->staticSet == UUcTrue);

	curCamera->cameraLocation = *inCameraLocation;
	curCamera->viewVector = *inViewDirection;
	curCamera->upVector = *inUpDirection;

	curCamera->viewDirty = UUcTrue;
	curCamera->viewSet = UUcTrue;

	UUmAssert(M3gActiveGeomEngine != UUcMaxUns16);
	M3gGeomEngineList[M3gActiveGeomEngine].methods.cameraViewUpdate();
}

void
M3rCamera_GetStaticData(
	M3tGeomCamera*		inCamera,
	float				*outFOVy,
	float				*outAspect,
	float				*outZNear,
	float				*outZFar)
{
	M3tManager_GeomCamera*			curCamera;

	curCamera = (M3tManager_GeomCamera*)inCamera;

	UUmAssert(curCamera != NULL && curCamera->inUse == UUcTrue);

	if(outFOVy != NULL) *outFOVy = curCamera->fovy;
	if(outAspect != NULL) *outAspect = curCamera->aspect;
	if(outZNear != NULL) *outZNear = curCamera->zNear;
	if(outZFar != NULL) *outZFar = curCamera->zFar;
}

void
M3rCamera_GetViewData(
	M3tGeomCamera*		inCamera,
	M3tPoint3D			*outCameraLocation,
	M3tVector3D			*outViewDirection,
	M3tVector3D			*outUpDirection)
{
	M3tManager_GeomCamera*			curCamera;

	curCamera = (M3tManager_GeomCamera*)inCamera;
	UUmAssert(curCamera != NULL && curCamera->inUse == UUcTrue);

	if(outCameraLocation != NULL) *outCameraLocation = curCamera->cameraLocation;
	if(outViewDirection != NULL) *outViewDirection = curCamera->viewVector;
	if(outUpDirection != NULL) *outUpDirection = curCamera->upVector;
}

void
M3rCamera_GetViewData_VxU(
	M3tGeomCamera*		inCamera,
	M3tVector3D			*outViewXUp)
{
	M3tManager_GeomCamera*			curCamera;

	curCamera = (M3tManager_GeomCamera*)inCamera;
	UUmAssert(curCamera != NULL && curCamera->inUse == UUcTrue);

	UUmAssertReadPtr(outViewXUp, sizeof(*outViewXUp));

	*outViewXUp = curCamera->crossVector;
}

void
M3rCamera_GetWorldFrustum(
	M3tGeomCamera*		inCamera,
	M3tPoint3D			*outPointList,
	M3tPlaneEquation	*outPlaneEquList)
{
	M3tManager_GeomCamera*			curCamera;
	UUtUns16				i;

	curCamera = (M3tManager_GeomCamera*)inCamera;
	UUmAssert(curCamera != NULL && curCamera->inUse == UUcTrue);
	UUmAssert(curCamera->staticSet == UUcTrue && curCamera->viewSet == UUcTrue);

	MSiCamera_UpdateWorldFrustum(curCamera);

	for(i = 0; i < 6; i++)
	{
		outPointList[i] = curCamera->worldFrustumPoints[i];
		outPlaneEquList[i] = curCamera->worldFrustumPlanes[i];
	}

	outPointList[6] = curCamera->worldFrustumPoints[6];
	outPointList[7] = curCamera->worldFrustumPoints[7];
}

void
M3rCamera_DrawWorldFrustum(
	M3tGeomCamera*		inCamera)
{
	M3tManager_GeomCamera*	curCamera;

	curCamera = (M3tManager_GeomCamera*)inCamera;
	UUmAssert(curCamera != NULL && curCamera->inUse == UUcTrue);

	MSiCamera_UpdateWorldFrustum(curCamera);

	// Draw the near plane
		M3rGeom_Line_Light(
			curCamera->worldFrustumPoints + 0,
			curCamera->worldFrustumPoints + 1,
			IMcShade_Blue);
		M3rGeom_Line_Light(
			curCamera->worldFrustumPoints + 1,
			curCamera->worldFrustumPoints + 2,
			IMcShade_Blue);
		M3rGeom_Line_Light(
			curCamera->worldFrustumPoints + 2,
			curCamera->worldFrustumPoints + 3,
			IMcShade_Blue);
		M3rGeom_Line_Light(
			curCamera->worldFrustumPoints + 3,
			curCamera->worldFrustumPoints + 0,
			IMcShade_Blue);

	// Draw the far plane
		M3rGeom_Line_Light(
			curCamera->worldFrustumPoints + 4,
			curCamera->worldFrustumPoints + 5,
			IMcShade_Red);
		M3rGeom_Line_Light(
			curCamera->worldFrustumPoints + 5,
			curCamera->worldFrustumPoints + 6,
			IMcShade_Red);
		M3rGeom_Line_Light(
			curCamera->worldFrustumPoints + 6,
			curCamera->worldFrustumPoints + 7,
			IMcShade_Red);
		M3rGeom_Line_Light(
			curCamera->worldFrustumPoints + 7,
			curCamera->worldFrustumPoints + 4,
			IMcShade_Red);

	// Draw the frustum from the viewpoint to the near plane
		M3rGeom_Line_Light(
			&curCamera->cameraLocation,
			curCamera->worldFrustumPoints + 0,
			IMcShade_White);

		M3rGeom_Line_Light(
			&curCamera->cameraLocation,
			curCamera->worldFrustumPoints + 1,
			IMcShade_White);

		M3rGeom_Line_Light(
			&curCamera->cameraLocation,
			curCamera->worldFrustumPoints + 2,
			IMcShade_White);

		M3rGeom_Line_Light(
			&curCamera->cameraLocation,
			curCamera->worldFrustumPoints +3,
			IMcShade_White);

	// Draw the frustum from the near plane to the far plane
		M3rGeom_Line_Light(
			curCamera->worldFrustumPoints + 4,
			curCamera->worldFrustumPoints + 0,
			IMcShade_White);

		M3rGeom_Line_Light(
			curCamera->worldFrustumPoints + 5,
			curCamera->worldFrustumPoints + 1,
			IMcShade_White);

		M3rGeom_Line_Light(
			curCamera->worldFrustumPoints + 6,
			curCamera->worldFrustumPoints + 2,
			IMcShade_White);

		M3rGeom_Line_Light(
			curCamera->worldFrustumPoints + 7,
			curCamera->worldFrustumPoints + 3,
			IMcShade_White);
}

