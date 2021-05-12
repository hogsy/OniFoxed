/*
	FILE:	Motoko_Draw.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Dec 12, 1998
	
	PURPOSE: Interface to the Motoko 3D engine
	
	Copyright 1997

*/

#include <stdlib.h>

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Console.h"
#include "BFW_Timer.h"

#include "Motoko_Manager.h"
#include "Motoko_Private.h"
#include "Motoko_Sort.h"
#include "BFW_AppUtilities.h"

#define M3cMaxSortedObjects	(4 * 1024)

typedef struct M3tSort_State
{
	UUtUns8			appearence;
	UUtUns8			interpolation;
	UUtUns8			fill;
	UUtUns8			zCompare;
	UUtUns32		constantColor;
	M3tTextureMap*	textureMap;
	UUtUns8			alpha;
	UUtBool			fog; // S.S.
	UUtBool			blend_with_constant_alpha;
	
} M3tSort_State;

//---------------------------------------------------
	typedef struct M3tSort_Triangle
	{
		M3tSort_State		state;
		M3tPointScreen		points[3];
		M3tTextureCoord		baseUVs[3];
		UUtUns32			shades[3];

	} M3tSort_Triangle;

//---------------------------------------------------
	typedef struct M3tSort_Quad
	{
		M3tSort_State		state;
		M3tPointScreen		points[4];
		M3tTextureCoord		baseUVs[4];
		UUtUns32			shades[4];
		
	} M3tSort_Quad;

//---------------------------------------------------
	typedef struct M3tSort_Pent
	{
		M3tSort_State		state;
		M3tPointScreen		points[5];
		M3tTextureCoord		baseUVs[5];
		UUtUns32			shades[5];
		
	} M3tSort_Pent;

//---------------------------------------------------
	typedef struct M3tSort_Sprite
	{
		M3tSort_State		state;
		M3tPointScreen		points[2];
		M3tTextureCoord		textureCoords[4];
		
	} M3tSort_Sprite;

typedef void
(*M3tSort_DrawFunc)(
	void*			inObjPtr);

typedef struct M3tSort_Object
{
	M3tSort_DrawFunc	drawFunc;
	void*				objPtr;
	
} M3tSort_Object;

static UUtUns16			gNumObjects = 0;
static M3tSort_Object	gObjList[M3cMaxSortedObjects];
static UUtUns16			gSortedObjIndexList[M3cMaxSortedObjects];
static float			gSortedObjDistList[M3cMaxSortedObjects];
static UUtMemory_Pool*	gMemoryPool = NULL;

extern UUtUns32		M3gDrawState_IntFlags;
extern UUtUns32		M3gDrawState_PtrFlags;
extern void**		M3gDrawStatePtr;
extern UUtInt32*	M3gDrawStateInt;
extern void*		M3gDrawStatePrivateStack;
extern UUtUns16		M3gDrawStateTOS;

static void
M3iSort_Draw_SetState(

	M3tSort_State*			inState)
{

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Appearence,
		inState->appearence);
	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Interpolation,
		inState->interpolation);
	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Fill,
		inState->fill);
	M3rDraw_State_SetInt(
		M3cDrawStateIntType_ZCompare,
		inState->zCompare);
	M3rDraw_State_SetInt(
		M3cDrawStateIntType_ConstantColor,
		inState->constantColor);
	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Alpha,
		inState->alpha);
	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Fog,
		inState->fog);
	M3rDraw_State_SetInt(
		M3cDrawStateIntType_FrameBufferBlendWithConstantAlpha,
		inState->blend_with_constant_alpha);
	if(inState->textureMap != NULL)
	{
		M3rDraw_State_SetPtr(
			M3cDrawStatePtrType_BaseTextureMap,
			inState->textureMap);
	}
}

static void
M3iSort_Draw_Triangle(

	M3tSort_Triangle*		inTriangle)
{
	M3tTri	triangle;
	
	M3iSort_Draw_SetState(&inTriangle->state);

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_ScreenPointArray,
		inTriangle->points);
	
	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_TextureCoordArray,
		inTriangle->baseUVs);
	
	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_ScreenShadeArray_DC,
		inTriangle->shades);
	
	M3rDraw_State_SetInt(
		M3cDrawStateIntType_NumRealVertices,
		3);					

	M3gDrawEngineList[M3gActiveDrawEngine].methods.privateStateUpdate(
		(char*)M3gDrawStatePrivateStack + M3gDrawStateTOS * M3gDrawEngineList[M3gActiveDrawEngine].methods.privateStateSize,
		M3gDrawState_IntFlags,
		M3gDrawStateInt,
		M3gDrawState_PtrFlags,
		M3gDrawStatePtr);
	
	M3gDrawState_IntFlags = 0;
	M3gDrawState_PtrFlags = 0;
	
	triangle.indices[0] = 0;
	triangle.indices[1] = 1;
	triangle.indices[2] = 2;
	
	M3gManagerDrawContext.drawFuncs->triangle(
		&triangle);
}

static void
M3iSort_Draw_Quad(

	M3tSort_Quad*			inQuad)
{
	M3tQuad	quad;
	
	M3iSort_Draw_SetState(&inQuad->state);

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_ScreenPointArray,
		inQuad->points);
	
	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_TextureCoordArray,
		inQuad->baseUVs);
	
	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_ScreenShadeArray_DC,
		inQuad->shades);
	
	M3rDraw_State_SetInt(
		M3cDrawStateIntType_NumRealVertices,
		4);						

	M3gDrawEngineList[M3gActiveDrawEngine].methods.privateStateUpdate(
		(char*)M3gDrawStatePrivateStack + M3gDrawStateTOS * M3gDrawEngineList[M3gActiveDrawEngine].methods.privateStateSize,
		M3gDrawState_IntFlags,
		M3gDrawStateInt,
		M3gDrawState_PtrFlags,
		M3gDrawStatePtr);
	
	M3gDrawState_IntFlags = 0;
	M3gDrawState_PtrFlags = 0;
	
	quad.indices[0] = 0;
	quad.indices[1] = 1;
	quad.indices[2] = 2;
	quad.indices[3] = 3;
	
	M3gManagerDrawContext.drawFuncs->quad(
		&quad);
}

static void
M3iSort_Draw_Pent(

	M3tSort_Pent*			inPent)
{
	M3tPent	pent;
	
	M3iSort_Draw_SetState(&inPent->state);

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_ScreenPointArray,
		inPent->points);
	
	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_TextureCoordArray,
		inPent->baseUVs);
	
	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_ScreenShadeArray_DC,
		inPent->shades);
	
	M3rDraw_State_SetInt(
		M3cDrawStateIntType_NumRealVertices,
		5);						// we know that this does not matter

	M3gDrawEngineList[M3gActiveDrawEngine].methods.privateStateUpdate(
		(char*)M3gDrawStatePrivateStack + M3gDrawStateTOS * M3gDrawEngineList[M3gActiveDrawEngine].methods.privateStateSize,
		M3gDrawState_IntFlags,
		M3gDrawStateInt,
		M3gDrawState_PtrFlags,
		M3gDrawStatePtr);
	
	M3gDrawState_IntFlags = 0;
	M3gDrawState_PtrFlags = 0;
	
	pent.indices[0] = 0;
	pent.indices[1] = 1;
	pent.indices[2] = 2;
	pent.indices[3] = 3;
	pent.indices[4] = 4;
	
	M3gManagerDrawContext.drawFuncs->pent(
		&pent);
}

static void
M3iSort_Draw_Sprite(

	M3tSort_Sprite*			inSprite)
{

	M3iSort_Draw_SetState(&inSprite->state);
	
	M3gDrawEngineList[M3gActiveDrawEngine].methods.privateStateUpdate(
		(char*)M3gDrawStatePrivateStack + M3gDrawStateTOS * M3gDrawEngineList[M3gActiveDrawEngine].methods.privateStateSize,
		M3gDrawState_IntFlags,
		M3gDrawStateInt,
		M3gDrawState_PtrFlags,
		M3gDrawStatePtr);
	
	M3gDrawState_IntFlags = 0;
	M3gDrawState_PtrFlags = 0;

	M3gManagerDrawContext.drawFuncs->sprite(
		inSprite->points,
		inSprite->textureCoords);
}

static void M3iSort_Draw_BuildState(M3tSort_State *outState)
{	
	outState->appearence = (UUtUns8)M3rDraw_State_GetInt(M3cDrawStateIntType_Appearence);
	outState->interpolation = (UUtUns8)M3rDraw_State_GetInt(M3cDrawStateIntType_Interpolation);
	outState->fill = (UUtUns8)M3rDraw_State_GetInt(M3cDrawStateIntType_Fill);
	outState->zCompare = (UUtUns8)M3rDraw_State_GetInt(M3cDrawStateIntType_ZCompare);
	outState->constantColor = M3rDraw_State_GetInt(M3cDrawStateIntType_ConstantColor);
	outState->alpha = (UUtUns8)M3rDraw_State_GetInt(M3cDrawStateIntType_Alpha);		
	outState->textureMap = M3rDraw_State_GetPtr(M3cDrawStatePtrType_BaseTextureMap);
	outState->fog= (UUtBool)M3rDraw_State_GetInt(M3cDrawStateIntType_Fog);
	outState->blend_with_constant_alpha = (UUtBool) M3rDraw_State_GetInt(M3cDrawStateIntType_FrameBufferBlendWithConstantAlpha);

	return;
}

static void
M3iSort_AddObject(
	M3tSort_DrawFunc	inDrawFunc,
	void*				inObjPtr,
	float				inW)
{
	if (gNumObjects >= M3cMaxSortedObjects) 
	{
		static UUtUns32 last_time = 0;

		if (UUrMachineTime_Sixtieths() > last_time)
		{
			COrConsole_Printf("sorting buffer overflowed (only warns once per second)");
			last_time = UUrMachineTime_Sixtieths() + 60;
		}

		return;
	}

	gSortedObjIndexList[gNumObjects] = gNumObjects;
	gSortedObjDistList[gNumObjects] = inW;
	gObjList[gNumObjects].drawFunc = inDrawFunc;
	gObjList[gNumObjects].objPtr = inObjPtr;

	gNumObjects++;
}

UUtError
M3rSort_Initialize(
	void)
{
	gNumObjects = 0;
	gMemoryPool = UUrMemory_Pool_New(M3cMaxSortedObjects * sizeof(M3tSort_Pent), UUcPool_Growable);
	UUmError_ReturnOnNull(gMemoryPool);
	
	return UUcError_None;
}

void
M3rSort_Terminate(
	void)
{
	if(gMemoryPool != NULL)
	{
		UUrMemory_Pool_Delete(gMemoryPool);
	}
}

UUtError
M3rSort_Frame_Start(
	void)
{
	
	gNumObjects = 0;
	
	UUrMemory_Pool_Reset(gMemoryPool);
	
	return UUcError_None;
}

static UUtBool iObject_List_Compare(UUtUns16 a, UUtUns16 b)
{
	float wa = gSortedObjDistList[a];
	float wb = gSortedObjDistList[b];
	UUtBool result;

	result = wb > wa;

	return result;
}

UUtError
M3rSort_Frame_End(
	void)
{
	UUtUns16				itr;
	M3tSort_Object*			curObj;
	//UUtBool					oldSorting;

	if(gNumObjects == 0) return UUcError_None;
	
	M3gDrawContext_Counters.numAlphaSortedObjs += gNumObjects;
	
	M3rDraw_State_Push();
	
	M3rDraw_State_SetInt(
		M3cDrawStateIntType_VertexFormat,
		M3cDrawStateVertex_Unified);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Clipping,
		0);
	
	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_VertexBitVector,
		NULL);
	
	AUrQSort_16(gSortedObjIndexList, gNumObjects, iObject_List_Compare);
	
	for(itr = 0; itr < gNumObjects; itr++)
	{
		UUtUns16 index = gSortedObjIndexList[itr];

		UUmAssert(index < gNumObjects);

		curObj = gObjList + index;
		
		curObj->drawFunc(
			curObj->objPtr);
	}
	
	M3rDraw_State_Pop();
	
	return UUcError_None;
}

void 
M3rSort_Draw_Triangle(
	M3tTri*	inTri)
{
	float					w;
	M3tTextureCoord*		baseUVArray;
	M3tPointScreen*			pointArray;
	UUtUns32*				shadeArray;
	M3tSort_Triangle*		triangle;
	
	pointArray = M3gManagerDrawContext.pointArray;
	baseUVArray = M3gManagerDrawContext.baseUVArray;
	shadeArray = M3gManagerDrawContext.shadeArray;
	
	UUmAssertReadPtr(pointArray, sizeof(M3tPoint3D));

	triangle = UUrMemory_Pool_Block_New(gMemoryPool, sizeof(M3tSort_Triangle));
	if(triangle == NULL)
	{
		UUmAssert(!"Out of sorting memory");
		return;
	}

	M3iSort_Draw_BuildState(&triangle->state);

	w = UUmMin3(
		pointArray[inTri->indices[0]].invW, 
		pointArray[inTri->indices[1]].invW, 
		pointArray[inTri->indices[2]].invW);
	
	UUmAssert(pointArray[inTri->indices[0]].x < 2000.0f);
	UUmAssert(pointArray[inTri->indices[1]].x < 2000.0f);
	UUmAssert(pointArray[inTri->indices[2]].x < 2000.0f);
	UUmAssert(pointArray[inTri->indices[0]].y < 2000.0f);
	UUmAssert(pointArray[inTri->indices[1]].y < 2000.0f);
	UUmAssert(pointArray[inTri->indices[2]].y < 2000.0f);
	
	triangle->points[0] = pointArray[inTri->indices[0]];
	triangle->points[1] = pointArray[inTri->indices[1]];
	triangle->points[2] = pointArray[inTri->indices[2]];

	if(triangle->state.appearence != M3cDrawState_Appearence_Gouraud)
	{
		UUmAssertReadPtr(baseUVArray, sizeof(M3tTextureCoord));

		triangle->baseUVs[0] = baseUVArray[inTri->indices[0]];
		triangle->baseUVs[1] = baseUVArray[inTri->indices[1]];
		triangle->baseUVs[2] = baseUVArray[inTri->indices[2]];
	}
	
	if(triangle->state.interpolation == M3cDrawState_Interpolation_Vertex)
	{
		UUmAssertReadPtr(shadeArray, sizeof(UUtUns16));

		triangle->shades[0] = shadeArray[inTri->indices[0]];
		triangle->shades[1] = shadeArray[inTri->indices[1]];
		triangle->shades[2] = shadeArray[inTri->indices[2]];
	}

	UUmAssert(M3rVerify_PointScreen(triangle->points + 0) == UUcError_None);
	UUmAssert(M3rVerify_PointScreen(triangle->points + 1) == UUcError_None);
	UUmAssert(M3rVerify_PointScreen(triangle->points + 2) == UUcError_None);
	
	M3iSort_AddObject(
		(M3tSort_DrawFunc)M3iSort_Draw_Triangle,
		triangle,
		w);
}

void
M3rSort_Draw_TriSplit(
	M3tTriSplit*	inTriSplit)
{
	float					w;
	M3tTextureCoord*		baseUVArray;
	M3tPointScreen*			pointArray;
	M3tSort_Triangle*		triangle;
	
	pointArray = M3gManagerDrawContext.pointArray;
	baseUVArray = M3gManagerDrawContext.baseUVArray;
	
	UUmAssertReadPtr(pointArray, sizeof(M3tPoint3D));
	UUmAssertReadPtr(baseUVArray, sizeof(M3tTextureCoord));

	triangle = UUrMemory_Pool_Block_New(gMemoryPool, sizeof(M3tSort_Triangle));
	if(triangle == NULL)
	{
		UUmAssert(!"Out of sorting memory");
		return;
	}

	M3iSort_Draw_BuildState(&triangle->state);

	w = UUmMin3(
		pointArray[inTriSplit->vertexIndices.indices[0]].invW, 
		pointArray[inTriSplit->vertexIndices.indices[1]].invW, 
		pointArray[inTriSplit->vertexIndices.indices[2]].invW);

	triangle->points[0] = pointArray[inTriSplit->vertexIndices.indices[0]];
	triangle->points[1] = pointArray[inTriSplit->vertexIndices.indices[1]];
	triangle->points[2] = pointArray[inTriSplit->vertexIndices.indices[2]];
	
	triangle->baseUVs[0] = baseUVArray[inTriSplit->baseUVIndices.indices[0]];
	triangle->baseUVs[1] = baseUVArray[inTriSplit->baseUVIndices.indices[1]];
	triangle->baseUVs[2] = baseUVArray[inTriSplit->baseUVIndices.indices[2]];

	UUmAssert(M3rVerify_PointScreen(triangle->points + 0) == UUcError_None);
	UUmAssert(M3rVerify_PointScreen(triangle->points + 1) == UUcError_None);
	UUmAssert(M3rVerify_PointScreen(triangle->points + 2) == UUcError_None);
	
	M3iSort_AddObject(
		(M3tSort_DrawFunc)M3iSort_Draw_Triangle,
		triangle,
		w);
}

void 
M3rSort_Draw_Quad(
	M3tQuad*	inQuad)
{
	float					w;
	M3tTextureCoord*		baseUVArray;
	M3tPointScreen*			pointArray;
	UUtUns32*				shadeArray;
	M3tSort_Quad*			quad;
	
	pointArray = M3gManagerDrawContext.pointArray;
	baseUVArray = M3gManagerDrawContext.baseUVArray;
	shadeArray = M3gManagerDrawContext.shadeArray;
	
	UUmAssertReadPtr(pointArray, sizeof(M3tPoint3D));

	quad = UUrMemory_Pool_Block_New(gMemoryPool, sizeof(M3tSort_Quad));
	if(quad == NULL)
	{
		UUmAssert(!"Out of sorting memory");
		return;
	}

	M3iSort_Draw_BuildState(&quad->state);

	w = UUmMin4(
		pointArray[inQuad->indices[0]].invW, 
		pointArray[inQuad->indices[1]].invW, 
		pointArray[inQuad->indices[2]].invW,
		pointArray[inQuad->indices[3]].invW);
	
	quad->points[0] = pointArray[inQuad->indices[0]];
	quad->points[1] = pointArray[inQuad->indices[1]];
	quad->points[2] = pointArray[inQuad->indices[2]];
	quad->points[3] = pointArray[inQuad->indices[3]];

	if(quad->state.appearence != M3cDrawState_Appearence_Gouraud)
	{
		UUmAssertReadPtr(baseUVArray, sizeof(M3tTextureCoord));

		quad->baseUVs[0] = baseUVArray[inQuad->indices[0]];
		quad->baseUVs[1] = baseUVArray[inQuad->indices[1]];
		quad->baseUVs[2] = baseUVArray[inQuad->indices[2]];
		quad->baseUVs[3] = baseUVArray[inQuad->indices[3]];
	}
	
	if(quad->state.interpolation == M3cDrawState_Interpolation_Vertex)
	{
		UUmAssertReadPtr(shadeArray, sizeof(UUtUns16));

		quad->shades[0] = shadeArray[inQuad->indices[0]];
		quad->shades[1] = shadeArray[inQuad->indices[1]];
		quad->shades[2] = shadeArray[inQuad->indices[2]];
		quad->shades[3] = shadeArray[inQuad->indices[3]];
	}

	UUmAssert(M3rVerify_PointScreen(quad->points + 0) == UUcError_None);
	UUmAssert(M3rVerify_PointScreen(quad->points + 1) == UUcError_None);
	UUmAssert(M3rVerify_PointScreen(quad->points + 2) == UUcError_None);
	UUmAssert(M3rVerify_PointScreen(quad->points + 3) == UUcError_None);

	M3iSort_AddObject(
		(M3tSort_DrawFunc)M3iSort_Draw_Quad,
		quad,
		w);
}

void
M3rSort_Draw_QuadSplit(
	M3tQuadSplit*	inQuadSplit)
{
	float					w;
	M3tTextureCoord*		baseUVArray;
	M3tPointScreen*			pointArray;
	M3tSort_Quad*			quad;
	
	pointArray = M3gManagerDrawContext.pointArray;
	baseUVArray = M3gManagerDrawContext.baseUVArray;
	
	UUmAssertReadPtr(pointArray, sizeof(M3tPoint3D));
	UUmAssertReadPtr(baseUVArray, sizeof(M3tTextureCoord));

	quad = UUrMemory_Pool_Block_New(gMemoryPool, sizeof(M3tSort_Quad));
	if(quad == NULL)
	{
		UUmAssert(!"Out of sorting memory");
		return;
	}

	M3iSort_Draw_BuildState(&quad->state);

	w = UUmMin4(
		pointArray[inQuadSplit->vertexIndices.indices[0]].invW, 
		pointArray[inQuadSplit->vertexIndices.indices[1]].invW, 
		pointArray[inQuadSplit->vertexIndices.indices[2]].invW,
		pointArray[inQuadSplit->vertexIndices.indices[3]].invW);

	quad->points[0] = pointArray[inQuadSplit->vertexIndices.indices[0]];
	quad->points[1] = pointArray[inQuadSplit->vertexIndices.indices[1]];
	quad->points[2] = pointArray[inQuadSplit->vertexIndices.indices[2]];
	quad->points[3] = pointArray[inQuadSplit->vertexIndices.indices[3]];

	quad->baseUVs[0] = baseUVArray[inQuadSplit->baseUVIndices.indices[0]];
	quad->baseUVs[1] = baseUVArray[inQuadSplit->baseUVIndices.indices[1]];
	quad->baseUVs[2] = baseUVArray[inQuadSplit->baseUVIndices.indices[2]];
	quad->baseUVs[3] = baseUVArray[inQuadSplit->baseUVIndices.indices[3]];

	UUmAssert(M3rVerify_PointScreen(quad->points + 0) == UUcError_None);
	UUmAssert(M3rVerify_PointScreen(quad->points + 1) == UUcError_None);
	UUmAssert(M3rVerify_PointScreen(quad->points + 2) == UUcError_None);
	UUmAssert(M3rVerify_PointScreen(quad->points + 3) == UUcError_None);
	
	M3iSort_AddObject(
		(M3tSort_DrawFunc)M3iSort_Draw_Quad,
		quad,
		w);
}

void 
M3rSort_Draw_Pent(
	M3tPent*	inPent)
{
	float					w;
	M3tTextureCoord*		baseUVArray;
	M3tPointScreen*			pointArray;
	UUtUns32*				shadeArray;
	M3tSort_Pent*			pent;
	
	pointArray = M3gManagerDrawContext.pointArray;
	baseUVArray = M3gManagerDrawContext.baseUVArray;
	shadeArray = M3gManagerDrawContext.shadeArray;
	
	UUmAssertReadPtr(pointArray, sizeof(M3tPoint3D));

	pent = UUrMemory_Pool_Block_New(gMemoryPool, sizeof(M3tSort_Pent));
	if(pent == NULL)
	{
		UUmAssert(!"Out of sorting memory");
		return;
	}

	M3iSort_Draw_BuildState(&pent->state);

	w = UUmMin5(
		pointArray[inPent->indices[0]].invW, 
		pointArray[inPent->indices[1]].invW, 
		pointArray[inPent->indices[2]].invW,
		pointArray[inPent->indices[3]].invW,
		pointArray[inPent->indices[4]].invW);

	pent->points[0] = pointArray[inPent->indices[0]];
	pent->points[1] = pointArray[inPent->indices[1]];
	pent->points[2] = pointArray[inPent->indices[2]];
	pent->points[3] = pointArray[inPent->indices[3]];
	pent->points[4] = pointArray[inPent->indices[4]];
			
	if(pent->state.appearence != M3cDrawState_Appearence_Gouraud)
	{
		UUmAssertReadPtr(baseUVArray, sizeof(M3tTextureCoord));

		pent->baseUVs[0] = baseUVArray[inPent->indices[0]];
		pent->baseUVs[1] = baseUVArray[inPent->indices[1]];
		pent->baseUVs[2] = baseUVArray[inPent->indices[2]];
		pent->baseUVs[3] = baseUVArray[inPent->indices[3]];
		pent->baseUVs[4] = baseUVArray[inPent->indices[4]];
	}
	
	if(pent->state.interpolation == M3cDrawState_Interpolation_Vertex)
	{
		UUmAssertReadPtr(shadeArray, sizeof(UUtUns16));

		pent->shades[0] = shadeArray[inPent->indices[0]];
		pent->shades[1] = shadeArray[inPent->indices[1]];
		pent->shades[2] = shadeArray[inPent->indices[2]];
		pent->shades[3] = shadeArray[inPent->indices[3]];
		pent->shades[4] = shadeArray[inPent->indices[4]];
	}

	UUmAssert(M3rVerify_PointScreen(pent->points + 0) == UUcError_None);
	UUmAssert(M3rVerify_PointScreen(pent->points + 1) == UUcError_None);
	UUmAssert(M3rVerify_PointScreen(pent->points + 2) == UUcError_None);
	UUmAssert(M3rVerify_PointScreen(pent->points + 3) == UUcError_None);
	UUmAssert(M3rVerify_PointScreen(pent->points + 4) == UUcError_None);
	
	M3iSort_AddObject(
		(M3tSort_DrawFunc)M3iSort_Draw_Pent,
		pent,
		w);
}

void
M3rSort_Draw_PentSplit(
	M3tPentSplit*	inPentSplit)
{
	float					w;
	M3tTextureCoord*		baseUVArray;
	M3tPointScreen*			pointArray;
	M3tSort_Pent*			pent;
	
	pointArray = M3gManagerDrawContext.pointArray;
	baseUVArray = M3gManagerDrawContext.baseUVArray;
	
	UUmAssertReadPtr(pointArray, sizeof(M3tPoint3D));
	UUmAssertReadPtr(baseUVArray, sizeof(M3tTextureCoord));
	
	pent = UUrMemory_Pool_Block_New(gMemoryPool, sizeof(M3tSort_Pent));
	if(pent == NULL)
	{
		UUmAssert(!"Out of sorting memory");
		return;
	}

	M3iSort_Draw_BuildState(&pent->state);

	w = UUmMin5(
		pointArray[inPentSplit->vertexIndices.indices[0]].invW, 
		pointArray[inPentSplit->vertexIndices.indices[1]].invW, 
		pointArray[inPentSplit->vertexIndices.indices[2]].invW,
		pointArray[inPentSplit->vertexIndices.indices[3]].invW,
		pointArray[inPentSplit->vertexIndices.indices[4]].invW);

	pent->points[0] = pointArray[inPentSplit->vertexIndices.indices[0]];
	pent->points[1] = pointArray[inPentSplit->vertexIndices.indices[1]];
	pent->points[2] = pointArray[inPentSplit->vertexIndices.indices[2]];
	pent->points[3] = pointArray[inPentSplit->vertexIndices.indices[3]];
	pent->points[4] = pointArray[inPentSplit->vertexIndices.indices[4]];
	
	pent->baseUVs[0] = baseUVArray[inPentSplit->baseUVIndices.indices[0]];
	pent->baseUVs[1] = baseUVArray[inPentSplit->baseUVIndices.indices[1]];
	pent->baseUVs[2] = baseUVArray[inPentSplit->baseUVIndices.indices[2]];
	pent->baseUVs[3] = baseUVArray[inPentSplit->baseUVIndices.indices[3]];
	pent->baseUVs[4] = baseUVArray[inPentSplit->baseUVIndices.indices[4]];

	UUmAssert(M3rVerify_PointScreen(pent->points + 0) == UUcError_None);
	UUmAssert(M3rVerify_PointScreen(pent->points + 1) == UUcError_None);
	UUmAssert(M3rVerify_PointScreen(pent->points + 2) == UUcError_None);
	UUmAssert(M3rVerify_PointScreen(pent->points + 3) == UUcError_None);
	UUmAssert(M3rVerify_PointScreen(pent->points + 4) == UUcError_None);
	
	M3iSort_AddObject(
		(M3tSort_DrawFunc)M3iSort_Draw_Pent,
		pent,
		w);
}

void
M3rSort_Draw_Sprite(
	const M3tPointScreen	*inPoints,
	const M3tTextureCoord	*inTextureCoords)
{
	M3tSort_Sprite*			sprite;
	
	sprite = UUrMemory_Pool_Block_New(gMemoryPool, sizeof(M3tSort_Sprite));
	if(sprite == NULL)
	{
		UUmAssert(!"Out of sorting memory");
		return;
	}

	M3iSort_Draw_BuildState(&sprite->state);
	
	sprite->points[0] = inPoints[0];
	sprite->points[1] = inPoints[1];
	sprite->textureCoords[0] = inTextureCoords[0];
	sprite->textureCoords[1] = inTextureCoords[1];
	sprite->textureCoords[2] = inTextureCoords[2];
	sprite->textureCoords[3] = inTextureCoords[3];
	
	M3iSort_AddObject(
		(M3tSort_DrawFunc)M3iSort_Draw_Sprite,
		sprite,
		inPoints[0].invW);
}

void M3rDraw_Commit_Alpha(
	void)
{
	UUtError error;

	error = M3rSort_Frame_End();
	UUmAssert(UUcError_None == error);

	error = M3rSort_Frame_Start();
	UUmAssert(UUcError_None == error);
}
