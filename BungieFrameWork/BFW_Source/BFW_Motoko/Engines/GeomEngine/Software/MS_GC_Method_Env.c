/*
	FILE:	MS_GC_Method_Env.c

	AUTHOR:	Brent H. Pease

	CREATED: Jan 14, 1998

	PURPOSE:

	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Akira.h"
#include "BFW_BitVector.h"

#include "BFW_Shared_Math.h"

#include "Motoko_Manager.h"
#include "MS_GC_Private.h"
#include "MS_GC_Method_Env.h"
#include "MS_GC_Method_Camera.h"
#include "MS_Geom_Transform.h"
#include "MS_Geom_Clip.h"
#include "MS_GC_Env_Clip.h"

#include "BFW_Shared_Clip.h"

#include "Akira_Private.h"

extern UUtUns16		MSgLightMapMode;

UUtError
MSrGeomContext_Method_Env_SetCamera(
	M3tGeomCamera*		inCamera)
{
	UUmAssertReadPtr(inCamera, sizeof(M3tManager_GeomCamera));

	MSgGeomContextPrivate->visCamera = (M3tManager_GeomCamera*)inCamera;

	return UUcError_None;
}

static void M3rDraw_Quad_Nvidia_Cheats(M3tQuadSplit *inQuad)
{
	M3tTriSplit tri;

	tri.baseUVIndices.indices[0] = inQuad->baseUVIndices.indices[0];
	tri.baseUVIndices.indices[1] = inQuad->baseUVIndices.indices[1];
	tri.baseUVIndices.indices[2] = inQuad->baseUVIndices.indices[2];

	tri.vertexIndices.indices[0] = inQuad->vertexIndices.indices[0];
	tri.vertexIndices.indices[1] = inQuad->vertexIndices.indices[1];
	tri.vertexIndices.indices[2] = inQuad->vertexIndices.indices[2];

	tri.shades[0] = inQuad->shades[0];
	tri.shades[1] = inQuad->shades[1];
	tri.shades[2] = inQuad->shades[2];

	M3rDraw_Triangle(&tri);

	tri.baseUVIndices.indices[0] = inQuad->baseUVIndices.indices[0];
	tri.baseUVIndices.indices[1] = inQuad->baseUVIndices.indices[2];
	tri.baseUVIndices.indices[2] = inQuad->baseUVIndices.indices[3];

	tri.vertexIndices.indices[0] = inQuad->vertexIndices.indices[0];
	tri.vertexIndices.indices[1] = inQuad->vertexIndices.indices[2];
	tri.vertexIndices.indices[2] = inQuad->vertexIndices.indices[3];

	tri.shades[0] = inQuad->shades[0];
	tri.shades[1] = inQuad->shades[2];
	tri.shades[2] = inQuad->shades[3];

	M3rDraw_Triangle(&tri);

	return;
}

UUtError
MSrGeomContext_Method_Env_DrawGQList(
	UUtUns32	inNumGQs,
	UUtUns32*	inGQIndices,
	UUtBool		inTransparentList)
{
	AKtEnvironment_Private*	environmentPrivate = TMrTemplate_PrivateData_GetDataPtr(AKgTemplate_PrivateData, MSgGeomContextPrivate->environment);

	M3tPointScreen*			screenPoints;
	M3tPoint4D*				frustumPoints;
	UUtUns8*				clipCodes;
	M3tPoint3D*				worldPoints;
	UUtUns32*				gqVertexNeededBV;

	UUtUns32				curVertexItr;
	UUtUns32				curVertexIndex;

	UUtUns8					clipCodeOR;
	UUtUns8					clipCodeAND;

	AKtGQ_General*			gqGeneralArray;
	AKtGQ_Render*			gqRenderArray;
	AKtGQ_Collision*		gqCollisionArray;
	AKtGQ_General*			curGQGeneral;
	AKtGQ_Render*			curGQRender;

	UUtUns32				numTextureCoords;
	UUtUns32				numPoints;

//	M3tTextureMap*			texMap;
	M3tTextureMap**			textureMapArray;

	UUtUns32				gqIndexItr;
	UUtUns32				curGQIndex;

	UUtBool					useDebugMaps = UUcFalse;
	UUtBool					drawGhostGQs = UUcFalse;
	UUtUns32				debugState;
	UUtUns16				reflectionPlaneIndex = 700;

	static UUtBool			flash = 0;

	UUmAssertReadPtr(MSgGeomContextPrivate, sizeof(MSgGeomContextPrivate));

	screenPoints		= MSgGeomContextPrivate->gqVertexData.screenPoints;
	clipCodes			= MSgGeomContextPrivate->gqVertexData.clipCodes;
	frustumPoints		= MSgGeomContextPrivate->gqVertexData.frustumPoints;
	gqVertexNeededBV	= MSgGeomContextPrivate->gqVertexData.bitVector;


	worldPoints			= MSgGeomContextPrivate->environment->pointArray->points;
	gqGeneralArray		= MSgGeomContextPrivate->environment->gqGeneralArray->gqGeneral;
	gqRenderArray		= MSgGeomContextPrivate->environment->gqRenderArray->gqRender;
	gqCollisionArray	= MSgGeomContextPrivate->environment->gqCollisionArray->gqCollision;
	textureMapArray		= MSgGeomContextPrivate->environment->textureMapArray->maps;

	// debugState = MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_DebugMode];
	debugState = 0;

	useDebugMaps = (UUtBool)((debugState & M3cGeomState_DebugMode_UseEnvDbgTexture) != 0);
	drawGhostGQs = (UUtBool)((debugState & M3cGeomState_DebugMode_DrawGhostGQs) != 0);

	M3rDraw_State_Push();


	#if 0

		M3rDraw_State_SetInt(
			M3cDrawStateIntType_Appearence,
			M3cDrawState_Appearence_Gouraud);

		M3rDraw_State_SetInt(
			M3cDrawStateIntType_Interpolation,
			M3cDrawState_Interpolation_None);

		M3rDraw_State_SetInt(
			M3cDrawStateIntType_Fill,
			M3cDrawState_Fill_Line);

	#else

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Appearence,
		M3cDrawState_Appearence_Texture_Unlit);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_ConstantColor,
		0xFFFFFFFF);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Alpha,
		M3cMaxAlpha);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Interpolation,
		M3cDrawState_Interpolation_None);

	#endif

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_ScreenPointArray,
		screenPoints);

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_TextureCoordArray,
		MSgGeomContextPrivate->environment->textureCoordArray->textureCoords);

	MSrTransform_UpdateMatrices();

	numPoints = MSgGeomContextPrivate->environment->pointArray->numPoints;
	numTextureCoords = MSgGeomContextPrivate->environment->textureCoordArray->numTextureCoords;

	MSgGeomContextPrivate->gqVertexData.maxClipVertices = numPoints + M3cExtraCoords;
	MSgGeomContextPrivate->gqVertexData.maxClipTextureCords = numTextureCoords + M3cExtraCoords;

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_NumRealVertices,
		numPoints);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_VertexFormat,
		M3cDrawStateVertex_Split);

	UUmAssertReadPtr(gqVertexNeededBV, sizeof(UUtUns32));

	UUrBitVector_ClearBitAll(
		gqVertexNeededBV,
		numPoints);

	// Traverse the bit vector
	for(gqIndexItr = 0; gqIndexItr < inNumGQs; gqIndexItr++)
	{
		curGQIndex = inGQIndices[gqIndexItr];

		curGQGeneral = gqGeneralArray + curGQIndex;

		// mark all 4 vertices as needed
		for(curVertexItr = 0; curVertexItr < 4; curVertexItr++)
		{
			curVertexIndex = curGQGeneral->m3Quad.vertexIndices.indices[curVertexItr];

			UUrBitVector_SetBit(gqVertexNeededBV, curVertexIndex);
		}

		//UUmAssert(!(curGQGeneral->flags & AKcGQ_Flag_Transparent));
	}

	// Transform all the vertices into frustum and screen

	MSgGeomContextPrivate->activeFunctions->transformEnvPointListToFrustumScreenActive(
		gqVertexNeededBV,
		frustumPoints,
		screenPoints,
		clipCodes);

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_VertexBitVector,
		gqVertexNeededBV);

	M3rDraw_State_Commit();

	for(gqIndexItr = 0; gqIndexItr < inNumGQs; gqIndexItr++)
	{
		curGQIndex = inGQIndices[gqIndexItr];

		curGQRender = gqRenderArray + curGQIndex;
		curGQGeneral = gqGeneralArray + curGQIndex;

		clipCodeOR = 0;
		clipCodeAND = 0xFF;

		for(curVertexItr = 0; curVertexItr < 4; curVertexItr++)
		{
			curVertexIndex = curGQGeneral->m3Quad.vertexIndices.indices[curVertexItr];

			clipCodeOR |= clipCodes[curVertexIndex];
			clipCodeAND &= clipCodes[curVertexIndex];

		}

		// trivial reject
		if(clipCodeAND != 0) continue;

#if TOOL_VERSION
		// CB: flashTexture only exists in the tool version now
		if (gqIndexItr == AKgHighlightGQIndex)
		{
			M3rDraw_State_SetPtr(
				M3cDrawStatePtrType_BaseTextureMap,
				MSgGeomContextPrivate->flashTexture);
		}
		else if ((curGQGeneral->flags & AKcGQ_Flag_Draw_Flash))
		{
			curGQGeneral->flags &= ~AKcGQ_Flag_Draw_Flash;

			if(curGQGeneral->flags & AKcGQ_Flag_Flash_State)
			{
				curGQGeneral->flags &= ~AKcGQ_Flag_Flash_State;
				M3rDraw_State_SetPtr(
					M3cDrawStatePtrType_BaseTextureMap,
					MSgGeomContextPrivate->flashTexture);
			}
			else
			{
				curGQGeneral->flags |= AKcGQ_Flag_Flash_State;
				M3rDraw_State_SetPtr(
					M3cDrawStatePtrType_BaseTextureMap,
					textureMapArray[curGQRender->textureMapIndex]);
			}
		}
		else if (AKgDraw_SoundOccl && (curGQGeneral->flags & AKcGQ_Flag_Furniture))
		{
			// CB: when drawing sound occlusion quads only, make furniture yellow
			// this is because furniture occludes sound subtly differently - october 05, 2000
			M3rDraw_State_SetPtr(
				M3cDrawStatePtrType_BaseTextureMap,
				MSgGeomContextPrivate->flashTexture);
		}
		else
		{
			M3rDraw_State_SetPtr(
				M3cDrawStatePtrType_BaseTextureMap,
				textureMapArray[curGQRender->textureMapIndex]);
		}
#else
		// set the proper base texture map
		M3rDraw_State_SetPtr(
			M3cDrawStatePtrType_BaseTextureMap,
			textureMapArray[curGQRender->textureMapIndex]);
#endif

		if (inTransparentList) {
			M3rDraw_State_SetInt(M3cDrawStateIntType_Alpha, curGQRender->alpha);
		}

		M3rDraw_State_SetInt(
			M3cDrawStateIntType_Clipping,
			clipCodeOR != 0);

		M3rDraw_State_SetInt(
			M3cDrawStateIntType_ConstantColor,
			0xFFFFFFFF);

		M3rDraw_State_Commit();

		if(clipCodeOR != 0)
		{
			M3tTriSplit tri;
			UUtUns8 clip_code[4];

			clip_code[0] = clipCodes[curGQGeneral->m3Quad.vertexIndices.indices[0]];
			clip_code[1] = clipCodes[curGQGeneral->m3Quad.vertexIndices.indices[1]];
			clip_code[2] = clipCodes[curGQGeneral->m3Quad.vertexIndices.indices[2]];
			clip_code[3] = clipCodes[curGQGeneral->m3Quad.vertexIndices.indices[3]];

			// Clip xxx we turn environment quads to triangles so we clip right here
			// S.S. vertices reordered to correspond to geometry type
			// GL_TRIANGLE_FAN; previously used GL_QUAD

			MSgGeomContextPrivate->gqVertexData.newClipTextureIndex = numTextureCoords;
			MSgGeomContextPrivate->gqVertexData.newClipVertexIndex = numPoints;

			tri.shades[0] = curGQGeneral->m3Quad.shades[0/*1*/];
			tri.shades[1] = curGQGeneral->m3Quad.shades[1/*2*/];
			tri.shades[2] = curGQGeneral->m3Quad.shades[2/*3*/];

			tri.baseUVIndices.indices[0] = curGQGeneral->m3Quad.baseUVIndices.indices[0/*1*/];
			tri.baseUVIndices.indices[1] = curGQGeneral->m3Quad.baseUVIndices.indices[1/*2*/];
			tri.baseUVIndices.indices[2] = curGQGeneral->m3Quad.baseUVIndices.indices[2/*3*/];

			tri.vertexIndices.indices[0] = curGQGeneral->m3Quad.vertexIndices.indices[0/*1*/];
			tri.vertexIndices.indices[1] = curGQGeneral->m3Quad.vertexIndices.indices[1/*2*/];
			tri.vertexIndices.indices[2] = curGQGeneral->m3Quad.vertexIndices.indices[2/*3*/];


			if (clip_code[0/*1*/] & clip_code[1/*2*/] & clip_code[2/*3*/]) {
			}
			else if (clip_code[0/*1*/] | clip_code[1/*2*/] | clip_code[2/*3*/]) {
				MSrEnv_Clip_Tri(
					&tri,
					clip_code[0/*1*/],
					clip_code[1/*2*/],
					clip_code[2/*3*/],
					0);
			}
			else {
				M3rDraw_Triangle(&tri);
			}

			MSgGeomContextPrivate->gqVertexData.newClipTextureIndex = numTextureCoords;
			MSgGeomContextPrivate->gqVertexData.newClipVertexIndex = numPoints;

			tri.shades[0] = curGQGeneral->m3Quad.shades[0/*3*/];
			tri.shades[1] = curGQGeneral->m3Quad.shades[2/*0*/];
			tri.shades[2] = curGQGeneral->m3Quad.shades[3/*1*/];

			tri.baseUVIndices.indices[0] = curGQGeneral->m3Quad.baseUVIndices.indices[0/*3*/];
			tri.baseUVIndices.indices[1] = curGQGeneral->m3Quad.baseUVIndices.indices[2/*0*/];
			tri.baseUVIndices.indices[2] = curGQGeneral->m3Quad.baseUVIndices.indices[3/*1*/];

			tri.vertexIndices.indices[0] = curGQGeneral->m3Quad.vertexIndices.indices[0/*3*/];
			tri.vertexIndices.indices[1] = curGQGeneral->m3Quad.vertexIndices.indices[2/*0*/];
			tri.vertexIndices.indices[2] = curGQGeneral->m3Quad.vertexIndices.indices[3/*1*/];

			if (clip_code[0/*3*/] & clip_code[2/*0*/] & clip_code[3/*1*/]) {
			}
			else if (clip_code[0/*3*/] | clip_code[2/*0*/] | clip_code[3/*1*/]) {
				MSrEnv_Clip_Tri(
					&tri,
					clip_code[0/*3*/],
					clip_code[2/*0*/],
					clip_code[3/*1*/],
					0);
			}
			else {
				M3rDraw_Triangle(&tri);
			}
		}
		else
		{
			M3rDraw_Quad(&curGQGeneral->m3Quad);
		}
	}

	M3rDraw_State_Pop();

	return UUcError_None;
}

M3tPointScreen* myScreenPoints;


