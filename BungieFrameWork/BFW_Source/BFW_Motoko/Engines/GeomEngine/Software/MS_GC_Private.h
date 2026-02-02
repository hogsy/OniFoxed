/*
	FILE:	MS_GC_Private.h

	AUTHOR:	Brent H. Pease

	CREATED: Sept 19, 1997

	PURPOSE:

	Copyright 1997

*/
#ifndef MS_GC_PRIVATE_H
#define MS_GC_PRIVATE_H

#include "BFW_Akira.h"
#include "BFW_Shared_Math.h"
#include "BFW_Shared_Clip.h"
#include "BFW_MathLib.h"
#include "Motoko_Manager.h"

//#include "MS_Geom_Clip.h"

#define MScStateStack_MaxDepth	(3)

typedef struct MStGeomContextPrivate	MStGeomContextPrivate;

#define MScMaxGQs		(32 * 1024)

typedef void
(*MSrClip_PolyComputeVertexProc)(
	UUtUns32				inClipPlane,

	UUtUns32				inIndexIn0,
	UUtUns32				inIndexOut0,
	UUtUns32				inIndexNew0,

	UUtUns32				inIndexIn1,
	UUtUns32				inIndexOut1,
	UUtUns32				inIndexNew1,

	UUtUns8					*outClipCodeNew0,
	UUtUns8					*outClipCodeNew1);

typedef void
(*MSrClip_LineComputeVertexProc)(
	UUtUns32				inClipPlane,

	UUtUns32				inIndexIn,
	UUtUns32				inIndexOut,
	UUtUns32				inIndexNew,

	UUtUns8					*outClipCodeNew);

typedef void
(*MStTransform_PointListToScreen)(
	M3tGeometry*			inGeometry,
	M3tPointScreen			*outResultScreenPoints);

typedef void
(*MStTransform_PointListToScreen_ActiveVertices)(
	M3tGeometry*			inGeometry,
	UUtUns32*				inActiveVerticesBV,
	M3tPointScreen			*outResultScreenPoints);

typedef void
(*MStTransform_PointListToFrustumScreen)(
	M3tGeometry*			inGeometry,
	M3tPoint4D				*outFrustumPoints,
	M3tPointScreen			*outScreenPoints,
	UUtUns8					*outClipCodeList);

typedef void
(*MStTransform_PointListLocalToFrustumScreen_ActiveVertices)(
	M3tGeometry*			inGeometry,
	const UUtUns32*			inActiveVerticesBV,
	M3tPoint4D				*outFrustumPoints,
	M3tPointScreen			*outScreenPoints,
	UUtUns8					*outClipCodeList);

typedef void
(*MStTransform_PointListAndVertexNormalToWorld)(
	M3tGeometry*			inGeometry,
	M3tPoint3D				*outResultWorldPoints,
	M3tVector3D				*outResultWorldVertexNormals);

typedef void
(*MStTransform_PointListAndVertexNormalToWorldComputeViewVector)(
	M3tGeometry*			inGeometry,
	M3tPoint3D				*outResultWorldPoints,
	M3tVector3D				*outResultWorldViewVectors,
	M3tVector3D				*outResultWorldVertexNormals);

typedef void
(*MStTransform_FaceNormalToWorld)(
	M3tGeometry*			inGeometry,
	M3tVector3D				*outResultWorldTriNormals);

typedef void
(*MStTransform_EnvPointListToFrustumScreen_ActiveVertices)(
	const UUtUns32*			inActiveVerticesBV,
	M3tPoint4D				*outFrustumPoints,
	M3tPointScreen			*outScreenPoints,
	UUtUns8					*outClipCodeList);

typedef void
(*MStShade_Vertices_Gouraud)(
	M3tGeometry*			inGeometry,
	M3tPoint3D*				inWorldPoints,
	M3tVector3D*			inWorldVertexNormals,
	UUtUns32				*outResultScreenShades);

typedef void
(*MStShade_Vertices_GouraudActive)(
	M3tGeometry*			inGeometry,
	M3tPoint3D*				inWorldPoints,
	M3tVector3D*			inWorldVertexNormals,
	UUtUns32*				inActiveVerticesBV,
	UUtUns32				*outResultScreenShades);

typedef void
(*MStShade_Faces_Gouraud)(
	M3tGeometry*			inGeometry,
	M3tPoint3D*				inWorldPoints,
	UUtUns32				inNumFaces,
	M3tVector3D*			inWorldFaceNormals,
	UUtUns32*				inResultFaceScreenShades);

typedef MStClipStatus
(*MStTransform_BoundingBoxToFrustumScreen)(
	M3tGeometry*			inGeometry,
	M3tPoint4D				*outFrustumPoints,
	M3tPointScreen			*outScreenPoints,
	UUtUns8					*outClipCodeList);

typedef MStClipStatus
(*MStTransform_BoundingBoxClipStatus)(
	M3tGeometry*			inGeometry);

typedef void
(*MStBackface_Remove)(
	M3tGeometry*			inGeometry,
	M3tPoint3D*				inWorldPoints,
	M3tVector3D*			inWorldViewVectors,
	M3tVector3D*			inWorldTriNormals,
	UUtUns32*				outActiveTrisBV,
	UUtUns32*				outActiveVerticesBV);

typedef struct MStProcessingFuncs
{
	MStTransform_PointListToScreen								transformPointListToScreen;
	MStTransform_PointListToScreen_ActiveVertices				transformPointListToScreenActive;
	MStTransform_PointListToFrustumScreen						transformPointListToFrustumScreen;
	MStTransform_PointListLocalToFrustumScreen_ActiveVertices	transformPointListToFrustumScreenActive;
	MStTransform_EnvPointListToFrustumScreen_ActiveVertices		transformEnvPointListToFrustumScreenActive;
	MStTransform_PointListAndVertexNormalToWorld				transformPointListAndVertexNormalToWorld;
	MStTransform_PointListAndVertexNormalToWorldComputeViewVector	transformPointListAndVertexNormalToWorldComputeViewVector;
	MStTransform_FaceNormalToWorld								transformFaceNormalToWorld;
	MStTransform_BoundingBoxToFrustumScreen						transformBoundingBoxToFrustumScreen;
	MStTransform_BoundingBoxClipStatus							transformBoundingBoxClipStatus;

	MStShade_Vertices_Gouraud									shadeVerticesGouraud;
	MStShade_Vertices_GouraudActive								shadeVerticesGouraudActive;
	MStShade_Faces_Gouraud										shadeFacesGouraud;

	MStBackface_Remove											backfaceRemove;

} MStProcessingFuncs;

typedef struct MStDebugEnvMap
{
	M3tPoint3D	origin;
	M3tPoint3D	normal;
	M3tPoint3D	reflection;
	M3tPoint3D	view;

} MStDebugEnvMap;

typedef struct MStTransformedVertexData
{
	UUtUns32			arrayLength;
	UUtUns32*			bitVector;
	M3tPoint4D*			frustumPoints;
	M3tPointScreen*		screenPoints;
	UUtUns8*			clipCodes;
	M3tTextureCoord*	textureCoords;
	M3tPoint3D*			worldPoints;

	// New indices - needs to be reset every time a clipping function is called
	UUtUns32 newClipTextureIndex;
	UUtUns32 newClipVertexIndex;

	// maximum extra vertices - used for debugging
	UUtUns32 maxClipVertices;
	UUtUns32 maxClipTextureCords;

} MStTransformedVertexData;

typedef struct MStLight_Directional
{
	M3tVector3D	normalizedDirection;
	M3tColorRGB	color;

} MStLight_Directional;

#define MScDirectionalLight_Max (4)
#define MScPointLight_Max		(4)

struct MStGeomContextPrivate
{
	M3tMatrix4x4			matrix_localToFrustum;

	//M3tPoint3D		submitPoints[5];

	long			width;
	long			height;

	float			scaleX;
	float			scaleY;

	UUtUns32		curFrame;

	const UUtInt32*		stateInt;

	M3tManager_GeomCamera*	activeCamera;

	// matrix
		M3tMatrix4x3*			matrix_localToWorld;

	/* Light info */
		M3tLight_Ambient		light_Ambient;

		UUtUns32				light_NumDirectionalLights;
		MStLight_Directional	light_DirectionalList[MScDirectionalLight_Max];

		UUtUns32				light_NumPointLights;
		M3tLight_Point			light_PointList[MScPointLight_Max];

		// re-add these when they are supported
		//UUtUns32				light_NumConeLights;
		//M3tLight_Cone*			light_ConeList;

	/* Temporary memory */
		MStTransformedVertexData	objectVertexData;

		M3tPoint3D*				worldPoints;
		M3tVector3D*			worldViewVectors;	// Only used for SIMD
		M3tVector3D*			worldTriNormals;
		M3tVector3D*			worldVertexNormals;

		UUtUns32*				activeVerticesBV;
		UUtUns32*				activeTrisBV;

		void*					shades_vertex;
		void*					shades_tris;

	// Points to the texture coord array
		//M3tTextureCoord*		textureCoords;
		//UUtUns32				maxTextureCoords;

	/* Attribute info */
		M3tColorRGB				diffuseColor;

	// Environment and visibility crap
		AKtEnvironment*			environment;
		M3tManager_GeomCamera*	visCamera;
		M3tBoundingBox_MinMax	visBBox;

		MStTransformedVertexData	gqVertexData;
		//MStTransformedVertexData	otVertexData;

	// clipper function pointers
		MSrClip_PolyComputeVertexProc	polyComputeVertexProc;
		MSrClip_LineComputeVertexProc	lineComputeVertexProc;

	// debugging texture - NULL in ship version
		M3tTextureMap*					flashTexture;

	// debugging array - NULL in ship version
		UUtUns32			numDebugEnvMap;
		MStDebugEnvMap*		debugEnvMap;

	//
		MStProcessingFuncs	sisdFunctions;

		#if UUmSIMD != UUmSIMD_None

			MStProcessingFuncs	simdFunctions;
			float*				envPointSIMD;

		#endif

		MStProcessingFuncs*	activeFunctions;

	// template env map texture cords
		M3tTextureCoord*	envMapCoords;

};

extern MStGeomContextPrivate*	MSgGeomContextPrivate;

#endif /* MS_GC_PRIVATE_H */

