/*
	FILE:	Motoko_Geom.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Dec 8, 1999
	
	PURPOSE: Interface to the Motoko 3D engine
	
	Copyright 1997-1999

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Akira.h"
#include "BFW_TemplateManager.h"

#include "Akira_Private.h"

#include "Motoko_Manager.h"
#include "Motoko_Private.h"

// CB: I have snipped this partially-implemented
// alpha sorting mechanism because it isn't being used by
// the rest of the code
#define UNUSED_ALPHASORTING        0

#define M3cGeom_Debug_RingPoints	16

#if UNUSED_ALPHASORTING
#define M3cGeom_Alpha_MaxNumbers	(512)

typedef enum M3tGeom_Alpha_Kind
{
	M3cGeomAlphaKind_Sprite,
	M3cGeomAlphaKind_Object
	
} M3tGeom_Alpha_Kind;

typedef struct M3tGeom_Alpha_Sprite
{
	M3tTextureMap*	map;
	M3tPoint3D		point;
	float			horzScale;
	float			vertScale;
	UUtUns32		shade;
	UUtUns16		alpha;
	
} M3tGeom_Alpha_Sprite;

typedef struct M3tGeom_Alpha_Geom
{
	M3tGeometry*	geom;
	
} M3tGeom_Alpha_Geom;


typedef struct M3tGeom_Alpha_Object
{
	M3tGeom_Alpha_Kind	kind;
	UUtUns16			bspNodeIndex;
	float				distanceToCamera;
	
	M3tMatrix4x3		localToWorld;
	
	union
	{
		M3tGeom_Alpha_Geom		geom;
		M3tGeom_Alpha_Sprite	sprite;
		
	} u;
	
} M3tGeom_Alpha_Object;
#endif


typedef struct M3tGeomGlobals
{
	AKtEnvironment*	environment;
	
	UUtUns32	numSolidGQs;
	UUtUns32	solidGQs[AKcMaxVisibleGQs];
	
	UUtUns32	numJelloGQs;
	UUtUns32	jelloGQs[AKcMaxTransparentGQs];
	
	UUtUns32	numTransparentGQs;
	UUtUns32	transparentGQs[AKcMaxTransparentGQs];
	
	UUtUns32*	alphaGQBV;
	
#if UNUSED_ALPHASORTING
	UUtUns16				numAlphaObjects;
	M3tGeom_Alpha_Object	alphaObjects[M3cGeom_Alpha_MaxNumbers];
#endif
	
} M3tGeomGlobals;

M3tGeomGlobals	M3gGeomGlobals;

#if UNUSED_ALPHASORTING
static M3tGeom_Alpha_Object*
M3iGeom_AlphaObject_Get(
	void)
{
	if(M3gGeomGlobals.numAlphaObjects >= M3cGeom_Alpha_MaxNumbers) return NULL;
	
	return M3gGeomGlobals.alphaObjects + M3gGeomGlobals.numAlphaObjects++;
}
#endif

static void
M3iGeom_TraverseBSP(
	M3tPoint3D*				inCameraLoc,
	UUtUns32				inNodeIndex)
{
	AKtAlphaBSPTree_Node*	bspNode;
	AKtGQ_Collision*		gqCollision;	
	float					a, b, c, d;
	
	if (0xFFFFFFFF == inNodeIndex) return;
	
	bspNode = M3gGeomGlobals.environment->alphaBSPNodeArray->nodes + inNodeIndex;
	
	UUmAssert(bspNode->gqIndex < M3gGeomGlobals.environment->gqCollisionArray->numGQs);
	
	gqCollision = M3gGeomGlobals.environment->gqCollisionArray->gqCollision + bspNode->gqIndex;
	
	// get the plane equation
		AKmPlaneEqu_GetComponents(
			gqCollision->planeEquIndex,
			M3gGeomGlobals.environment->planeArray->planes,
			a, b, c, d);
	
	if(a * inCameraLoc->x + b * inCameraLoc->y + c * inCameraLoc->z + d <= 0.0f)
	{
		// "Pos" side
		
		M3iGeom_TraverseBSP(inCameraLoc, bspNode->negNodeIndex);
		
		// process this quad
		if(UUrBitVector_TestAndClearBit(M3gGeomGlobals.alphaGQBV, bspNode->gqIndex)) {		
			M3gGeomGlobals.transparentGQs[M3gGeomGlobals.numTransparentGQs++] = bspNode->gqIndex;
		}
		
		M3iGeom_TraverseBSP(inCameraLoc, bspNode->posNodeIndex);
	}
	else
	{
		// "Neg" side
	
		M3iGeom_TraverseBSP(inCameraLoc, bspNode->posNodeIndex);
		
		// process this quad
		if(UUrBitVector_TestAndClearBit(M3gGeomGlobals.alphaGQBV, bspNode->gqIndex)) {		
			M3gGeomGlobals.transparentGQs[M3gGeomGlobals.numTransparentGQs++] = bspNode->gqIndex;
		}
		
		M3iGeom_TraverseBSP(inCameraLoc, bspNode->negNodeIndex);
	}

	return;
}

UUtError
M3rGeomContext_SetEnvironment(
	struct AKtEnvironment*		inEnvironment)
{
	UUtError error;

	M3gGeomGlobals.environment = inEnvironment;

	if(M3gGeomGlobals.alphaGQBV != NULL) {
		UUrBitVector_Dispose(M3gGeomGlobals.alphaGQBV);
		M3gGeomGlobals.alphaGQBV = NULL;
	}
	
	if(inEnvironment != NULL) {
		M3gGeomGlobals.alphaGQBV = UUrBitVector_New(inEnvironment->gqGeneralArray->numGQs);
		UUmError_ReturnOnNull(M3gGeomGlobals.alphaGQBV);
	}
	
	M3gGeomGlobals.numJelloGQs = 0;
	M3gGeomGlobals.numSolidGQs = 0;
	M3gGeomGlobals.numTransparentGQs = 0;

	error = M3gGeomEngineList[M3gActiveGeomEngine].methods.contextSetEnvironment(inEnvironment);

	return error;
}

UUtError
M3rGeometry_Draw(
	M3tGeometry*		inGeometryObject)
{
	UUtError error = UUcError_None;
	UUtUns32 is_fast_mode = M3rGeom_State_Get(M3cGeomStateIntType_FastMode);

	if (!is_fast_mode)
	{
		UUtBool multitextured;
		extern UUtBool ONrMotoko_GraphicsQuality_SupportReflectionMapping(void); // Oni_Motoko.h

		if (NULL == inGeometryObject->baseMap)
		{
			TMrInstance_GetDataPtr(M3cTemplate_TextureMap, "NONE", (void **) &inGeometryObject->baseMap);
		}

		multitextured= ((inGeometryObject->baseMap != NULL) && (inGeometryObject->baseMap->envMap != NULL)) ? UUcTrue : UUcFalse;
		
		// determine how many passes to use for rendering this object
		if ((multitextured == UUcFalse) ||
			M3rSinglePassMultitexturingAvailable() ||
			(ONrMotoko_GraphicsQuality_SupportReflectionMapping() == UUcFalse))
		{
			// draw geometry in a single pass
			error= (M3gGeomContext)->geometryDraw(inGeometryObject);
		}
		else
		{
			/*
				Using the Motoko state machine doesn't work at this level, because I can't do a
				Commit() / Draw() / Commit() / Draw() sequence without the states getting hosed.
				That's why I call these gl_engine routines directly; it's faster this way anyhow.
			*/
			// these are in gl_engine.c
			extern void gl_prepare_multipass_env_map(void);
			extern void gl_prepare_multipass_base_map(void);
			extern void gl_finish_multipass(void);
			// draw geometry in 2 passes
			// pass 1: env map
			gl_prepare_multipass_env_map();
			error= (M3gGeomContext)->geometryDraw(inGeometryObject);
			if (error == UUcError_None)
			{
				// pass 2: base map
				gl_prepare_multipass_base_map();
				error= (M3gGeomContext)->geometryDraw(inGeometryObject);
			}
			gl_finish_multipass();
		}
	}
	
	return error;
}

UUtError
M3rSimpleSprite_Draw(
	M3tTextureMap*		inTextureMap,
	M3tPoint3D*			inPoint,
	float				horizSize,
	float				vertSize,
	UUtUns32			inShade,
	UUtUns16			inAlpha)
{
	M3rGeom_State_Set(M3cGeomStateIntType_SpriteMode, M3cGeomState_SpriteMode_Normal);
	M3rGeom_State_Commit();

	return M3rSprite_Draw(inTextureMap, inPoint, horizSize, vertSize, inShade, inAlpha, 0, NULL, NULL, 0, 0, 0);
}

UUtError
M3rSprite_Draw(
	M3tTextureMap*		inTextureMap,
	M3tPoint3D*			inPoint,
	float				horizSize,
	float				vertSize,
	UUtUns32			inShade,
	UUtUns16			inAlpha,
	float				inRotation,
	M3tVector3D*		inDirection,
	M3tMatrix3x3*		inOrientation,
	float				inXOffset,
	float				inXShortening,
	float				inXChop)
{
#if UNUSED_ALPHASORTING
	UUtError				error;
	M3tGeom_Alpha_Object*	alphaObject;
	M3tGeomCamera*			activeCamera;
	M3tMatrix4x3*			matrix;
	M3tPoint3D				worldPoint;
	M3tPoint3D				cameraLocation;
	float					dx, dy, dz;
	
	alphaObject = M3iGeom_AlphaObject_Get();
	if(alphaObject == NULL) return UUcError_None;
	
	// get the active camera and location
		M3rCamera_GetActive(&activeCamera);
		
		M3rCamera_GetViewData(activeCamera, &cameraLocation, NULL, NULL);
		
	// compute the world point
		error = M3rMatrixStack_Get(&matrix);
		UUmError_ReturnOnError(error);
		
		MUrMatrix_MultiplyPoint(inPoint, matrix, &worldPoint);
		
	// compute the distance to the camera
		dx = cameraLocation.x - worldPoint.x;
		dy = cameraLocation.y - worldPoint.y;
		dz = cameraLocation.z - worldPoint.z;
		
		alphaObject->distanceToCamera = MUrSqrt(dx * dx + dy * dy + dz * dz);
		
	// save the matrix
		alphaObject->localToWorld = *matrix;
		
	// save the object
		

	// add it to the tree
#endif
		
	return (M3gGeomContext)->spriteDraw(inTextureMap, inPoint, horizSize, vertSize, inShade, inAlpha, inRotation, inDirection, inOrientation, inXOffset, inXShortening, inXChop);
}

UUtError
M3rSpriteArray_Draw(
	M3tSpriteArray*		inSpriteArray )
{		
	return (M3gGeomContext)->spriteArrayDraw(inSpriteArray);
}

UUtError 
M3rDraw_Skybox( 
	M3tSkyboxData		*inSkybox )
{
	UUtUns32 is_fast_mode = M3rGeom_State_Get(M3cGeomStateIntType_FastMode);
	UUtError error = UUcError_None;

	if (!is_fast_mode) {
		error = (M3gGeomContext)->skyboxDraw(inSkybox);
	}

	return error;
}

UUtError
M3rDraw_CreateSkybox(
	M3tSkyboxData		*inSkybox,
	M3tTextureMap**		inTextures )
{
	return (M3gGeomContext)->skyboxCreate(inSkybox, inTextures);
}

UUtError
M3rDraw_DestroySkybox(
	M3tSkyboxData		*inSkybox )
{
	return (M3gGeomContext)->skyboxDestroy(inSkybox);
}

UUtError
M3rDecal_Draw( 
	M3tDecalHeader*		inDecal,
	UUtUns16			inAlpha )
{
	return (M3gGeomContext)->decalDraw(inDecal, inAlpha);
}

UUtError
M3rContrail_Draw(
	M3tTextureMap*		inTextureMap,
	float				inV0,
	float				inV1,
	M3tContrailData*	inPoint0,
	M3tContrailData*	inPoint1)
{		
	return (M3gGeomContext)->contrailDraw(inTextureMap, inV0, inV1, inPoint0, inPoint1);
}

UUtBool
M3rPointVisible(
	M3tPoint3D*			inPoint,
	float				inTolerance)
{
	return (M3gGeomContext)->pointVisible(inPoint, inTolerance);
}

float
M3rPointVisibleScale(
	M3tPoint3D*			inPoint,
	M3tPoint2D*			inTestOffsets,
	UUtUns32			inTestOffsetsCount )
{
	return (M3gGeomContext)->pointVisibleScale(inPoint, inTestOffsets, inTestOffsetsCount);
}

UUtError
M3rEnv_DrawGQList(
	UUtUns32	inNumGQs,
	UUtUns32*	inGQIndices)
{
	UUtError	error;
//	UUtUns16	numJelloGQs = 0;
//	UUtUns16	jelloGQs[AKcMaxTransparentGQs];

//	UUtUns16	numSolidGQs = 0;
//	UUtUns16	solidGQs[AKcMaxVisibleGQs];

	UUtUns32		gqItr;
	AKtGQ_General*	gqGeneralArray;
	
	AKtGQ_General*	curGQGeneral;
	UUtUns32 is_fast_mode = M3rGeom_State_Get(M3cGeomStateIntType_FastMode);

	if (is_fast_mode) {
		return UUcError_None;
	}
		
	gqGeneralArray	= M3gGeomGlobals.environment->gqGeneralArray->gqGeneral;
	
	// traverse the list and seperate out the solid quads from the transparent quads
	M3gGeomGlobals.numTransparentGQs	= 0;
	M3gGeomGlobals.numJelloGQs			= 0;
	M3gGeomGlobals.numSolidGQs			= 0;
	
	for(gqItr = 0; gqItr < inNumGQs; gqItr++)
	{
		curGQGeneral = gqGeneralArray + inGQIndices[gqItr];
		
		if (curGQGeneral->flags & AKcGQ_Flag_Jello) {
			M3gGeomGlobals.jelloGQs[M3gGeomGlobals.numJelloGQs++] = inGQIndices[gqItr];
		}
		else if (curGQGeneral->flags & AKcGQ_Flag_Transparent) {
			UUrBitVector_SetBit(M3gGeomGlobals.alphaGQBV, inGQIndices[gqItr]);
		}
		else {
			M3gGeomGlobals.solidGQs[M3gGeomGlobals.numSolidGQs++] = inGQIndices[gqItr];
		}
	}
	
	error = (M3gGeomContext)->envDrawGQList(M3gGeomGlobals.numSolidGQs, M3gGeomGlobals.solidGQs, UUcFalse);
	UUmError_ReturnOnErrorMsg(error, "failed to draw the solid GQs");

	error = (M3gGeomContext)->envDrawGQList(M3gGeomGlobals.numJelloGQs, M3gGeomGlobals.jelloGQs, UUcTrue);
	UUmError_ReturnOnErrorMsg(error, "failed to draw the jello GQs");

	return error;
}

void
M3rGeom_Clear_Jello(
	void)
{
	UUtUns32 itr;
	AKtGQ_General *curGQ, *gqGeneralArray = M3gGeomGlobals.environment->gqGeneralArray->gqGeneral;

	for (itr = 0; itr < M3gGeomGlobals.numJelloGQs; itr++) {
		curGQ = gqGeneralArray + M3gGeomGlobals.jelloGQs[itr];

		curGQ->flags &= ~AKcGQ_Flag_Jello;
	}
}

UUtError 
M3rGeom_Frame_Start(
	UUtUns32			inGameTicksElapsed)
{
#if UNUSED_ALPHASORTING
	M3gGeomGlobals.numAlphaObjects = 0;
#endif
	
	return (M3gGeomContext)->frameStart(inGameTicksElapsed);
}

UUtError
M3rGeom_Draw_Environment_Alpha(
	void)
{
	UUtError		error;
	M3tGeomCamera*	activeCamera;
	M3tPoint3D		cameraLocation;
	UUtUns32 is_fast_mode = M3rGeom_State_Get(M3cGeomStateIntType_FastMode);

	if (is_fast_mode) {
		return UUcError_None;
	}
	
	if(M3gGeomGlobals.environment != NULL)
	{
		// get the camera location
		M3rCamera_GetActive(&activeCamera);
		
		M3rCamera_GetViewData(
			activeCamera,
			&cameraLocation,
			NULL,
			NULL);
	
		if (M3gGeomGlobals.environment->alphaBSPNodeArray->numNodes > 0) {
			M3iGeom_TraverseBSP(
				&cameraLocation,
				0);
		}
			
		M3rDraw_State_SetInt(M3cDrawStateIntType_ZWrite, M3cDrawState_ZWrite_Off);
		
		M3rDraw_State_Commit();
		
		error = (M3gGeomContext)->envDrawGQList(M3gGeomGlobals.numTransparentGQs, M3gGeomGlobals.transparentGQs, UUcFalse);
		UUmError_ReturnOnError(error);
	}

	return UUcError_None;
}

	
UUtError 
M3rGeom_Frame_End(
	void)
{
	UUtError		error;
	
	error = (M3gGeomContext)->frameEnd();
	
	return error;
}

void
M3rGeom_Draw_DebugSphere(
	M3tPoint3D *			inPoint,
	float					inRadius,
	IMtShade				inShade)
{
	// one times cache line size should be plenty but why risk it
	UUtUns8					block_XZ[(sizeof(M3tPoint3D) * (M3cGeom_Debug_RingPoints + 1)) + (2 * UUcProcessor_CacheLineSize)];
	UUtUns8					block_XY[(sizeof(M3tPoint3D) * (M3cGeom_Debug_RingPoints + 1)) + (2 * UUcProcessor_CacheLineSize)];
	UUtUns8					block_YZ[(sizeof(M3tPoint3D) * (M3cGeom_Debug_RingPoints + 1)) + (2 * UUcProcessor_CacheLineSize)];
	M3tPoint3D				*ring_XZ = UUrAlignMemory(block_XZ);
	M3tPoint3D				*ring_XY = UUrAlignMemory(block_XY);
	M3tPoint3D				*ring_YZ = UUrAlignMemory(block_YZ);
	UUtUns32				itr;
	
	for(itr = 0; itr < M3cGeom_Debug_RingPoints; itr++)
	{
		float		theta;
		float		cos_theta_radius;
		float		sin_theta_radius;
		
		theta = M3c2Pi * (((float) itr) / M3cGeom_Debug_RingPoints);
		cos_theta_radius = MUrCos(theta) * inRadius;
		sin_theta_radius = MUrSin(theta) * inRadius;
		
		ring_XZ[itr].x = cos_theta_radius + inPoint->x;
		ring_XZ[itr].y = inPoint->y;
		ring_XZ[itr].z = sin_theta_radius + inPoint->z;
		
		ring_XY[itr].x = cos_theta_radius + inPoint->x;
		ring_XY[itr].y = sin_theta_radius + inPoint->y;
		ring_XY[itr].z = inPoint->z;

		ring_YZ[itr].x = inPoint->x;
		ring_YZ[itr].y = cos_theta_radius + inPoint->y;
		ring_YZ[itr].z = sin_theta_radius + inPoint->z;
	}

	// close the rings
	ring_XZ[M3cGeom_Debug_RingPoints] = ring_XZ[0];
	ring_XY[M3cGeom_Debug_RingPoints] = ring_XY[0];
	ring_YZ[M3cGeom_Debug_RingPoints] = ring_YZ[0];
	
	// draw the rings
	M3rGeometry_LineDraw((M3cGeom_Debug_RingPoints + 1), ring_XZ, inShade);
	M3rGeometry_LineDraw((M3cGeom_Debug_RingPoints + 1), ring_XY, inShade);
	M3rGeometry_LineDraw((M3cGeom_Debug_RingPoints + 1), ring_YZ, inShade);
}

