/*
	FILE:	OG_GC_Method_Geometry.c

	AUTHOR:	Brent H. Pease

	CREATED: June 10, 1999

	PURPOSE:

	Copyright 1997

*/

#ifndef __ONADA__

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_TemplateManager.h"
#include "BFW_BitVector.h"

#include "Motoko_Manager.h"
#include "OGL_DrawGeom_Common.h"

#include "OG_GC_Private.h"
#include "OG_GC_Method_Geometry.h"
#include "OG_GC_Method_State.h"

static void
OGiMatrix_Update(
	void)
{
	M3tMatrix4x3*			matrix_localToWorld;
	float					m[16];

	M3rMatrixStack_Get(&matrix_localToWorld);

	OGLrCommon_Matrix4x3_OniToGL(
		matrix_localToWorld,
		m);

	glMultMatrixf(m);

}

UUtError
OGrGeomContext_Method_Geometry_BoundingBox_Draw(
	UUtUns32			inRGB555,
	M3tGeometry*		inGeometryObject)
{

	return UUcError_None;
}

UUtError
OGrGeomContext_Method_Geometry_Draw(
	M3tGeometry*		inGeometryObject)
{
	GLuint	geomDisplayListIndex;


	OGLrCommon_Camera_Update(OGLcCameraMode_3D);

	glPushMatrix();

	OGiMatrix_Update();

	//OGLrCommon_glEnable(GL_CULL_FACE);

	OGLrCommon_TextureMap_Select(inGeometryObject->baseMap, GL_TEXTURE0_ARB);

	TMrInstance_PrepareForUse(inGeometryObject);
	geomDisplayListIndex = (GLuint)TMrTemplate_Cache_Simple_GetDataPtr(OGgGeomContextPrivate.geometryCache, inGeometryObject);
	UUmAssert(glIsList(geomDisplayListIndex) == GL_TRUE);

	glCallList(geomDisplayListIndex);

	//OGLrCommon_glDisable(GL_CULL_FACE);

	glPopMatrix();

	//OGLrCommon_glFlush();

	return UUcError_None;
}

UUtError
OGrGeomContext_Method_Geometry_PolyDraw(
	UUtUns32			inNumPoints,
	M3tPoint3D*			inPoints,
	UUtUns32			inShade)
{

	return UUcError_None;
}

UUtError
OGrGeomContext_Method_Geometry_LineDraw(
	UUtUns32			inNumPoints,
	M3tPoint3D*			inPoints,
	UUtUns32			inShade)
{

	return UUcError_None;
}

UUtError
OGrGeomContext_Method_Geometry_PointDraw(
	UUtUns32			inNumPoints,
	M3tPoint3D*			inPoints,
	UUtUns32			inShade)
{

	return UUcError_None;
}

UUtError
OGrGeomContext_Method_Sprite_Draw(
	M3tTextureMap*		inTextureMap,
	M3tPoint3D*			inPoint,
	float				inHorizSize,
	float				inVertSize,
	UUtUns32			inShade,
	UUtUns16			inAlpha,
	float				inRotation,
	M3tVector3D*		inDirection,
	M3tMatrix3x3*		inOrientation,
	float				inXOffset,
	float				inXShorten,
	float				inXChop)
{
	M3tVector3D		cross;
	M3tVector3D		up;
	M3tGeomCamera*	activeCamera;
	M3tPoint3D		tempPoints[4];
	M3tTextureCoord	tempCoords[4];
	float			xr, yr, zr;
	float			xd, yd, zd;

	// CB: because the OG GeomContext is going to be removed,
	// I have not implemented code that takes account of the new
	// inRotation, inDirection, inOrientation, inXOffset, inXShorten or inXChop parameters.

	OGLrCommon_Camera_Update(OGLcCameraMode_3D);

	M3rCamera_GetActive(&activeCamera);

	UUmAssert(activeCamera != NULL);

	M3rCamera_GetViewData_VxU(activeCamera, &cross);
	M3rCamera_GetViewData(activeCamera, NULL, NULL, &up);

	xr = cross.x * inHorizSize * OGcSpriteHorizWorldScale * 0.5f;
	yr = cross.y * inHorizSize * OGcSpriteHorizWorldScale * 0.5f;
	zr = cross.z * inHorizSize * OGcSpriteHorizWorldScale * 0.5f;

	xd = -up.x * inVertSize * OGcSpriteVertWorldScale * 0.5f;
	yd = -up.y * inVertSize * OGcSpriteVertWorldScale * 0.5f;
	zd = -up.z * inVertSize * OGcSpriteVertWorldScale * 0.5f;

	UUmAssert(UUmFloat_CompareEqu(
		MUrSqrt(cross.x * cross.x + cross.y * cross.y + cross.z * cross.z), 1.0f));
	UUmAssert(UUmFloat_CompareEqu(
		MUrSqrt(up.x * up.x + up.y * up.y + up.z * up.z), 1.0f));

	// tl
		tempPoints[0].x = inPoint->x - xr - xd;
		tempPoints[0].y = inPoint->y - yr - yd;
		tempPoints[0].z = inPoint->z - zr - zd;
		tempCoords[0].u = 0.0f;
		tempCoords[0].v = 1.0f;

	// tr
		tempPoints[1].x = inPoint->x + xr - xd;
		tempPoints[1].y = inPoint->y + yr - yd;
		tempPoints[1].z = inPoint->z + zr - zd;
		tempCoords[1].u = 1.0f;
		tempCoords[1].v = 1.0f;

	// br
		tempPoints[2].x = inPoint->x + xr + xd;
		tempPoints[2].y = inPoint->y + yr + yd;
		tempPoints[2].z = inPoint->z + zr + zd;
		tempCoords[2].u = 1.0f;
		tempCoords[2].v = 0.0f;

	// bl
		tempPoints[3].x = inPoint->x - xr + xd;
		tempPoints[3].y = inPoint->y - yr + yd;
		tempPoints[3].z = inPoint->z - zr + zd;
		tempCoords[3].u = 0.0f;
		tempCoords[3].v = 0.0f;

	OGLrCommon_glEnableClientState(GL_VERTEX_ARRAY);
	OGLrCommon_glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, tempPoints);
	glTexCoordPointer(2, GL_FLOAT, 0, tempCoords);

	OGLrCommon_TextureMap_Select(inTextureMap, GL_TEXTURE0_ARB);

	glPushMatrix();

	OGiMatrix_Update();

	glDrawArrays(GL_POLYGON, 0, 4);

	glPopMatrix();

	return UUcError_None;
}

UUtError
OGrGeomContext_Method_Contrail_Draw(
			M3tTextureMap*		inTextureMap,
			float				inV0,
			float				inV1,
			M3tContrailData*	inPoint0,
			M3tContrailData*	inPoint1)
{
	// CB: because the OG GeomContext is going to be removed,
	// I have not implemented this. It should be pretty easy if needed.

	return UUcError_None;
}

UUtBool
OGrGeomContext_Method_PointVisible(
	M3tPoint3D*			inPoint,
	float				inTolerance)
{
	// CB: because the OG GeomContext is going to be removed,
	// I have not implemented this. It should be pretty easy if needed.

	return UUcTrue;
}



#endif // __ONADA__
