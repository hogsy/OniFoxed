/*
	FILE:	BFW_Akira.c

	AUTHOR:	Brent H. Pease

	CREATED: October 22, 1997

	PURPOSE: environment engine

	Copyright 1997

*/

#include "bfw_math.h"

#include <stdlib.h>
#include <ctype.h>

#include "BFW.h"
#include "BFW_TemplateManager.h"
#include "BFW_Motoko.h"
#include "BFW_Akira.h"
#include "BFW_Console.h"
#include "BFW_BitVector.h"
#include "BFW_AppUtilities.h"
#include "BFW_Collision.h"
#include "BFW_Shared_Math.h"
#include "BFW_BitVector.h"
#include "BFW_Object.h"
#include "BFW_TextSystem.h"
#include "BFW_ScriptLang.h"
#include "BFW_Timer.h"
#include "Akira_Private.h"
#include "Oni_Motoko.h"

#define AKcMaxBNVs	(2024)
#define AKcMaxGQs	(2024)


enum
{
	AKcEnvShow_BNV	= (1 << 0),
	AKcEnvShow_GQ	= (1 << 1)
};

#define AKcDebugTextureWidth	64
#define AKcDebugTextureHeight	64

#if PERFORMANCE_TIMER
// performance timers
UUtPerformanceTimer *AKg_RayCastOctTree_Timer = NULL;
UUtPerformanceTimer *AKg_Collision_Sphere_Timer = NULL;
UUtPerformanceTimer *AKg_ComputeVis_Timer = NULL;
UUtPerformanceTimer *AKg_BBox_Visible_Timer = NULL;
UUtPerformanceTimer *AKg_GQ_SphereVector_Timer = NULL;
UUtPerformanceTimer *AKg_Collision_Point_Timer = NULL;
UUtPerformanceTimer *AKg_IsBoundingBoxMinMaxVisible_Recursive_Timer= NULL;
#endif

// console variable
extern UUtBool gEnvironmentCollision;

UUtUns32*	AKgGQBitVector = NULL;

UUtBool		AKgDebug_ShowOctTree = UUcFalse;
UUtBool		AKgDebug_ShowLeafNodes = UUcFalse;
UUtBool		AKgDebug_ShowGQsInOctNode = UUcFalse;
UUtBool		AKgDebug_DrawFrustum = UUcFalse;
UUtBool		AKgDebug_DrawAllGunks = UUcFalse;
UUtBool		AKgDebug_DrawVisOnly = UUcFalse;
UUtBool		AKgDebug_ShowRays = UUcFalse;
UUtBool		AKgDebug_DebugMaps;
UUtBool		AKgShowStairFlagged = UUcFalse;
UUtUns32	AKgHighlightGQIndex = (UUtUns32) -1;

extern AKtCollision	AKgCollisionList[AKcMaxNumCollisions];
extern UUtUns16		AKgNumCollisions;

M3tTextureMap*			AKgNoneTexture;

//UUtWindow	AKgDebugWindow = NULL;

//float		AKgDebug_VisIgnoreThreshold;
UUtBool		AKgDrawGhostGQs = UUcFalse;

AKtEnvironment*	AKgEnvironment = NULL;

TMtPrivateData*		AKgTemplate_PrivateData = NULL;

UUtInt32	AKgRayCastNumber = 20;
UUtBool		AKgFastMode = UUcFalse;

static void AKiComputeBreakableFlags(AKtEnvironment *inEnvironment);

static void AKiEnvironment_DebugMaps_Dispose(
	AKtEnvironment *inEnv)
{
	// Disposes of the debug maps from the level
	if (!AKgDebug_DebugMaps) return;
}

static void AKiPrepareGrids(AKtEnvironment *inEnvironment)
{
	UUtUns32 itr, itr2;
	UUtUns32 count = inEnvironment->bnvNodeArray->numNodes;
	AKtBNVNode *nodes = inEnvironment->bnvNodeArray->nodes;
	UUtUns32 offset = (UUtUns32) TMrInstance_GetRawOffset(inEnvironment);
	PHtDebugInfo *debug_info;

	for(itr = 0; itr < count; itr++)
	{
		if (nodes[itr].roomData.compressed_grid != NULL) {
			nodes[itr].roomData.compressed_grid = UUmOffsetPtr(nodes[itr].roomData.compressed_grid, offset);
		}

		if (nodes[itr].roomData.debug_info != NULL) {
			// read and byte-swap the raw debugging info
			nodes[itr].roomData.debug_info = UUmOffsetPtr(nodes[itr].roomData.debug_info, offset);
			UUmAssertReadPtr(nodes[itr].roomData.debug_info, nodes[itr].roomData.debug_info_count * sizeof(PHtDebugInfo));


			for (itr2 = 0, debug_info = nodes[itr].roomData.debug_info; itr2 < nodes[itr].roomData.debug_info_count; itr2++, debug_info++) {
				UUmSwapLittle_2Byte(&debug_info->x);
				UUmSwapLittle_2Byte(&debug_info->y);
				UUmSwapLittle_4Byte(&debug_info->gq_index);
			}
		}
	}

	return;
}


static UUtError
AKrEnvironment_TemplateHandler(
	TMtTemplateProc_Message	inMessage,
	void*					inDataPtr,
	void*					inPrivateData)
{
	AKtEnvironment_Private*	environmentPrivate = (AKtEnvironment_Private*)inPrivateData;
	AKtEnvironment*			environment = (AKtEnvironment*)inDataPtr;
	UUtUns32				itr;

	UUmAssert(environmentPrivate != NULL);

	// debugging code to verify
	#if 0
	for(itr = 0; itr < environment->octTreeNodeArray->numNodes; itr++)
	{
		AKtOctTreeNode *node = environment->octTreeNodeArray->nodes + itr;

		UUmAssert(node->leaf == 0 || node->leaf == 1);
		UUmAssert(node->leaf == 0 || node->gqStartIndirectIndex <= environment->octTreeGQIndices->numIndices);
	}
	#endif

	switch(inMessage)
	{
		case TMcTemplateProcMessage_NewPostProcess:
			environmentPrivate->gqComputedBackFaceBV = NULL;
			environmentPrivate->gqBackFaceBV = NULL;
			environmentPrivate->gq2VisibilityVector = NULL;
			environmentPrivate->ot2LeafNodeVisibility = NULL;
			environmentPrivate->sky_visibility = 0;
			break;

		case TMcTemplateProcMessage_Update:
			if(environmentPrivate->gqComputedBackFaceBV != NULL) {
				UUrBitVector_Dispose(environmentPrivate->gqComputedBackFaceBV);
			}

			if(environmentPrivate->gqBackFaceBV != NULL) {
				UUrBitVector_Dispose(environmentPrivate->gqBackFaceBV);
			}

			if(environmentPrivate->gq2VisibilityVector != NULL) {
				UUr2BitVector_Dispose(environmentPrivate->gq2VisibilityVector);
			}

			if(environmentPrivate->ot2LeafNodeVisibility != NULL) {
				UUr2BitVector_Dispose(environmentPrivate->ot2LeafNodeVisibility);
			}

			UUmAssert(!"WHAT!");

			/* no break */

		case TMcTemplateProcMessage_LoadPostProcess:

			environmentPrivate->gqComputedBackFaceBV =
				UUrBitVector_New(environment->gqGeneralArray->numGQs);
			environmentPrivate->gqBackFaceBV =
				UUrBitVector_New(environment->gqGeneralArray->numGQs);

			environmentPrivate->gq2VisibilityVector = UUr2BitVector_Allocate(environment->gqGeneralArray->numGQs);
			environmentPrivate->ot2LeafNodeVisibility = UUr2BitVector_Allocate(environment->octTree->leafNodeArray->numNodes);

			environmentPrivate->visGQ_List = UUrMemory_Block_NewClear(sizeof(UUtUns32) * AKcMaxVisibleGQs);

			if(environmentPrivate->gqComputedBackFaceBV == NULL ||
				environmentPrivate->gqBackFaceBV == NULL ||
				environmentPrivate->gq2VisibilityVector == NULL ||
				environmentPrivate->ot2LeafNodeVisibility == NULL ||
				environmentPrivate->visGQ_List == NULL)
			{
				return UUcError_OutOfMemory;
			}

			UUr2BitVector_Clear(environmentPrivate->gq2VisibilityVector, environment->gqGeneralArray->numGQs);
			UUr2BitVector_Clear(environmentPrivate->ot2LeafNodeVisibility, environment->octTree->leafNodeArray->numNodes);

			// clear sky persist-frames timer
			environmentPrivate->sky_visibility = 0;

			// process texture maps for 2 sided flag based on transparency
			for(itr = 0; itr < environment->gqGeneralArray->numGQs; itr++)
			{
				if(environment->gqRenderArray->gqRender[itr].textureMapIndex != 0xFFFF)
				{
					if(IMrPixelType_HasAlpha(environment->textureMapArray->maps[environment->gqRenderArray->gqRender[itr].textureMapIndex]->texelType))
					{
						UUmAssert(environment->gqGeneralArray->gqGeneral[itr].flags & (AKcGQ_Flag_DrawBothSides | AKcGQ_Flag_Jello | AKcGQ_Flag_Transparent));
					}
					else
					{
						UUmAssert(!(environment->gqGeneralArray->gqGeneral[itr].flags & (AKcGQ_Flag_Transparent | AKcGQ_Flag_Jello)));
					}
				}
			}

			if (environment->gqDebugArray != NULL)
			{
				AKtGQ_Debug *gqDebug;
				UUtUns32 offset = (UUtUns32) TMrInstance_GetRawOffset(environment);

				gqDebug = environment->gqDebugArray->gqDebug;

				for(itr = 0; itr < environment->gqDebugArray->numGQs; itr++)
				{
					gqDebug->object_name = ((char *) gqDebug->object_name) + offset;
					gqDebug->file_name = ((char *) gqDebug->file_name) + offset;

					gqDebug++;
				}
			}

			AKiPrepareGrids(environment);

			break;

		case TMcTemplateProcMessage_DisposePreProcess:

			AKiEnvironment_DebugMaps_Dispose((AKtEnvironment *)inDataPtr);

			if(environmentPrivate->gqComputedBackFaceBV != NULL) {
				UUrBitVector_Dispose(environmentPrivate->gqComputedBackFaceBV);
			}

			if(environmentPrivate->gqBackFaceBV != NULL) {
				UUrBitVector_Dispose(environmentPrivate->gqBackFaceBV);
			}

			if(environmentPrivate->gq2VisibilityVector != NULL) {
				UUr2BitVector_Dispose(environmentPrivate->gq2VisibilityVector);
			}

			if(environmentPrivate->ot2LeafNodeVisibility != NULL) {
				UUr2BitVector_Dispose(environmentPrivate->ot2LeafNodeVisibility);
			}

			if (environmentPrivate->visGQ_List != NULL) {
				UUrMemory_Block_Delete(environmentPrivate->visGQ_List);
			}
			break;

		default:
			UUmAssert(!"Illegal message");
	}

	return UUcError_None;
}


static UUtError
AKiNodeProcHandler(
	TMtTemplateProc_Message	inMessage,
	TMtTemplateTag			inTheCausingTemplate,	// The template tag of the changing instance
	void*					inInstancePtr)
{
	UUtError error = UUcError_None;

	switch(inMessage)
	{
		case TMcTemplateProcMessage_NewPostProcess:
			break;

		case TMcTemplateProcMessage_LoadPostProcess:
			break;

		case TMcTemplateProcMessage_DisposePreProcess:

		case TMcTemplateProcMessage_Update:
			break;

		default:
			UUmAssert(!"Illegal message");
	}

	return UUcError_None;
}

#if 0
static void
AKiEnvShow(
	UUtUns32			inArgC,
	char**				inArgV,
	void*				inRefcon)
{
	UUtUns16	i;
	UUtUns32	newState = AKgEnvShowState;
	UUtBool		add;
	char*		p;

	if(inArgC < 2)
	{
		goto printDesc;
	}

	for(i = 1; i < inArgC; i++)
	{
		p = inArgV[i];

		if(*p == '+')
		{
			add = UUcTrue;
		}
		else if(*p == '-')
		{
			add = UUcFalse;
		}
		else
		{
			goto printDesc;
		}

		p++;

		if(strcmp(p, "all") == 0)
		{
			if(add)
			{
				newState = AKcEnvShow_BNV | AKcEnvShow_GQ;
			}
			else
			{
				newState = 0;
			}
		}
		else if(strcmp(p, "bnv") == 0)
		{
			if(add)
			{
				newState |= AKcEnvShow_BNV;
			}
			else
			{
				newState &=~ AKcEnvShow_BNV;
			}
		}
		else if(strcmp(p, "gq") == 0)
		{
			if(add)
			{
				newState |= AKcEnvShow_GQ;
			}
			else
			{
				newState &=~ AKcEnvShow_GQ;
			}
		}
		else
		{
			goto printDesc;
		}
	}

	AKgEnvShowState = newState;

	return;

printDesc:
	COrConsole_Printf("%s [ + | - ] [bnv | prt | psg | all]", inArgV[0]);

}

static void
AKiEnvShow_Generic_ParseExpr(
	UUtUns32*					inBitVector,
	UUtUns32					inMaxBits,
	UUtBitVector_BitRangeFunc	inFunc,
	char*						p)
{
	char		buffer[16];
	UUtUns32	start, end;
	char		c;
	char*		bp;

	while(1)
	{
		bp = buffer;
		c = *p++;
		while(isdigit(c))
		{
			*bp++ = c;
			c = *p++;
		}
		*bp = 0;

		sscanf(buffer, "%d", &start);

		if(c == '.')
		{
			c = *p++;
			if(c != '.')
			{
				return;
			}
			c = *p++;
			while(isdigit(c))
			{
				*bp++ = c;
				c = *p++;
			}
			*bp = 0;

			sscanf(buffer, "%d", &end);
		}
		else
		{
			end = start;
		}

		inFunc(inBitVector, start, end);

		if(c != ',')
		{
			break;
		}
	}
}
#endif

#if 0
static void
AKiEnvShow_Generic(
	UUtUns32*	inBitVector,
	UUtUns32	inMaxBits,
	UUtUns32	inArgC,
	char**		inArgV)
{
	char*		p;
	char		c;
	UUtUns16	i;

	UUtBitVector_BitRangeFunc	func;

	if(inArgC <= 1)
	{
		goto printDesc;
	}

	for(i = 1; i < inArgC; i++)
	{
		p = inArgV[i];

		c = *p;

		if(c == '+')
		{
			func = UUrBitVector_SetBitRange;
		}
		else if(c == '-')
		{
			func = UUrBitVector_ClearBitRange;
		}
		else if(c == 'i')
		{
			func = UUrBitVector_ToggleBitRange;
		}
		else
		{
			goto printDesc;
		}

		p++;

		if(p[0] == 'a' && p[1] == 'l' && p[2] == 'l')
		{
			func(inBitVector, 0, inMaxBits);
		}
		else
		{
			AKiEnvShow_Generic_ParseExpr(inBitVector, inMaxBits, func, p);
		}
	}

	return;

printDesc:
	COrConsole_Printf("%s [ + | - ] [all | expr; expr -> num | num..num | expr,expr", inArgV[0]);

}

static void
AKiEnvShow_BNV(
	UUtUns32			inArgC,
	char**				inArgV,
	void*				inRefcon)
{
	AKiEnvShow_Generic(AKgBNVBitVector, AKcMaxBNVs, inArgC, inArgV);
}

static void
AKiEnvShow_BNV_Level(
	UUtUns32			inArgC,
	char**				inArgV,
	void*				inRefcon)
{
	AKiEnvShow_Generic(AKgBNVLevelBitVector, 10, inArgC, inArgV);
}

static void
AKiEnvShow_GQ(
	UUtUns32			inArgC,
	char**				inArgV,
	void*				inRefcon)
{
	AKiEnvShow_Generic(AKgGQBitVector, AKcMaxGQs, inArgC, inArgV);
}
#endif

#if 0
static void
AKiEnvShowFileNames(
	UUtUns32			inArgC,
	char**				inArgV,
	void*				inRefcon)
{
	UUtUns16		showState;
	UUtInt32		i;
	AKtBNV_Debug*	curBNVDebug;
	//AKtGQ_Debug*	curGQDebug;

	showState = 0;

	for(i = 1; i < (UUtInt32)inArgC; i++)
	{
		if(strcmp(inArgV[i], "all") == 0)
		{
			showState = 0xFF;
		}
		if(strcmp(inArgV[i], "bnv") == 0)
		{
			showState |= AKcEnvShow_BNV;
		}
		else if(strcmp(inArgV[i], "gq") == 0)
		{
			showState |= AKcEnvShow_GQ;
		}
		else
		{
			goto printDesc;
		}
	}

	// Only print out stuff for the displayed objects
	showState &= AKgEnvShowState;

	if(showState & AKcEnvShow_BNV)
	{
		UUmBitVector_ForEachBitSetDo(i, AKgBNVBitVector, (UUtInt32)AKgEnvDebug->bnvNodeArray->numBNVs - 1)
		{
			curBNVDebug = AKgEnvDebug->bnvNodeArray->bnvs + i;
			COrConsole_Printf("BNV[%d]: %s", i, curBNVDebug->fileName);
		}
	}

	#if 0
	if(showState & AKcEnvShow_GQT)
	{
		UUmBitVector_ForEachBitSetDo(i, AKgPRTBitVector, (UUtInt32)AKgEnvDebug->prtArray->numPRTs - 1)
		{
			curPRTDebug = AKgEnvDebug->prtArray->prts + i;
			COrConsole_Printf("PRT[%d]: %s", i, curPRTDebug->fileName);
		}
	}
	#endif

	return;

printDesc:
	COrConsole_Printf("%s [bnv | prt | psg | all]", inArgV[0]);

}
#endif


UUtError
AKrLevel_Begin(
	AKtEnvironment*	inEnvironment)
{
	UUtError error;
	UUtInt32 ray_cast_count = ONrMotoko_GraphicsQuality_RayCount();

	AKgNoneTexture = NULL;
	AKgEnvironment = inEnvironment;

	error = AKrRayCastCoefficients_Initialize(ray_cast_count);
	UUmError_ReturnOnError(error);

	error = AKrCollision_LevelBegin(inEnvironment);
	UUmError_ReturnOnError(error);

	AKiComputeBreakableFlags(inEnvironment);

	return UUcError_None;
}

void
AKrLevel_End(
	void)
{
	AKrCollision_LevelEnd();

	AKrRayCastCoefficients_Terminate();
}

static void AKiVariable_Changed_RayCastNumber(void)
{
	UUtUns32 num_rays = AKgFastMode ? (AKgRayCastNumber / 2) : AKgRayCastNumber;

	AKrRayCastCoefficients_Terminate();
	AKrRayCastCoefficients_Initialize(num_rays);

	return;
}

void AKrEnvironment_FastMode(UUtBool inFast)
{
	if (inFast != AKgFastMode) {
		AKgFastMode = inFast;
		AKiVariable_Changed_RayCastNumber();
	}

	return;
}


UUtError
AKrInitialize(
	void)
{
	UUtError	error;

	error = AKrRegisterTemplates();
	UUmError_ReturnOnError(error);

	/*
	 * Register my template handlers with the TemplateManager
	 */

	error = TMrTemplate_PrivateData_New(AKcTemplate_Environment, sizeof(AKtEnvironment_Private), AKrEnvironment_TemplateHandler, &AKgTemplate_PrivateData);
	UUmError_ReturnOnError(error);


#if 1
	AKgDebugLeafNodes = NULL;
#else
	AKgDebugLeafNodes = AUrDict_New(32000);
	UUmError_ReturnOnNull(AKgDebugLeafNodes);
#endif

#if CONSOLE_DEBUGGING_COMMANDS
	SLrGlobalVariable_Register_Bool("env_show_stairflagged", "Show specially flagged noncollision quads", &AKgShowStairFlagged);
	SLrGlobalVariable_Register_Bool("env_show_rays", "Draw the rays", &AKgDebug_ShowRays);
	SLrGlobalVariable_Register_Bool("env_drawallgqs", "Draw all the GQs", &AKgDebug_DrawAllGunks);
	SLrGlobalVariable_Register_Bool("env_drawvisonly", "Draw only the GQs used for visibility", &AKgDebug_DrawVisOnly);
	SLrGlobalVariable_Register_Bool("env_collision", "Enables environment collision", &gEnvironmentCollision);
	SLrGlobalVariable_Register_Bool("env_drawfrustum", "Draw the frustum around the environment camera", &AKgDebug_DrawFrustum);
	SLrGlobalVariable_Register_Bool("env_show_octtree", "Enables display of environment octtree traversal", &AKgDebug_ShowOctTree);
	SLrGlobalVariable_Register_Bool("env_show_leafnodes", "Enables display of environment octtree leaf nodes", &AKgDebug_ShowLeafNodes);
	SLrGlobalVariable_Register_Bool("env_show_octnode_gqs", "When true the environment only renders triangles in the octnode that contains the manual camera", &AKgDebug_ShowGQsInOctNode);
	SLrGlobalVariable_Register_Bool("env_show_ghostgqs", "When true show ghost GQs", &AKgDrawGhostGQs);
	SLrGlobalVariable_Register_Int32("env_highlight_gq", "highlights a particular gq", (UUtInt32*)&AKgHighlightGQIndex);
		SLrGlobalVariable_Register(
			"env_ray_number",
			"sets the number of rays to cast",
			SLcReadWrite_ReadWrite,
			SLcType_Int32,
			(SLtValue*)&AKgRayCastNumber,
			AKiVariable_Changed_RayCastNumber);
	UUmError_ReturnOnError(error);
#endif

#if PERFORMANCE_TIMER
	AKg_RayCastOctTree_Timer = UUrPerformanceTimer_New("AK_RayCastOctTree");
	AKg_Collision_Sphere_Timer = UUrPerformanceTimer_New("AK_Collision_Sphere");
	AKg_ComputeVis_Timer = UUrPerformanceTimer_New("AK_ComputeVis");
	AKg_BBox_Visible_Timer = UUrPerformanceTimer_New("AK_BBox_Visible");
	AKg_GQ_SphereVector_Timer = UUrPerformanceTimer_New("AK_GQ_SphereVector");
	AKg_Collision_Point_Timer = UUrPerformanceTimer_New("AK_Collision_Point");
	AKg_IsBoundingBoxMinMaxVisible_Recursive_Timer= UUrPerformanceTimer_New("AK_IsBBoxMMVisRecursv");
#endif

	return UUcError_None;
}

void
AKrTerminate(
	void)
{
	if(AKgDebugLeafNodes != NULL) {
		AUrDict_Dispose(AKgDebugLeafNodes);
	}

	AKrEnvironment_PrintStats();
	AKrEnvironment_Collision_PrintStats();
}

static UUtBool
ARiPointInBSP(
	const M3tPlaneEquation*	inPlaneEquArray,
	const AKtBNV_BSPNode*	inBSPNodeArray,
	UUtUns32				inRootNode,
	const M3tPoint3D*		inPoint,
	M3tPlaneEquation*		outRejectingPlane,
	float*					outRejectionValue)
{
	const AKtBNV_BSPNode *curNode;
	float				a, b, c, d, plane_val;
	UUtUns32			curNodeIndex;

	curNodeIndex = inRootNode;

	while(1)
	{
		curNode = inBSPNodeArray + curNodeIndex;

		AKmPlaneEqu_GetComponents(curNode->planeEquIndex, inPlaneEquArray, a, b, c, d);

		plane_val = inPoint->x * a + inPoint->y * b + inPoint->z * c + d;
		if(UUmFloat_CompareLE(plane_val, 0.0f)) {
			if(0xFFFFFFFF == curNode->posNodeIndex) {
				return UUcTrue;
			}
			else {
				UUmAssert(curNode->posNodeIndex < 10000);
				curNodeIndex = curNode->posNodeIndex;
			}
		}
		else
		{
			if(0xFFFFFFFF == curNode->negNodeIndex) {
				if (outRejectingPlane) *outRejectingPlane = inPlaneEquArray[curNode->planeEquIndex];
				if (outRejectionValue) *outRejectionValue = plane_val;
				return UUcFalse;
			}
			else {
				UUmAssert(curNode->negNodeIndex < 10000);
				curNodeIndex = curNode->negNodeIndex;
			}
		}
	}
}



UUtBool
AKrPointInNode(
	const M3tPoint3D*		inViewpoint,
	const AKtEnvironment*	inEnvironment,
	UUtUns32		inNodeIndex,
	M3tPlaneEquation*		outRejectingPlane,
	float*					outRejectionValue)
{
	AKtBNVNode*	node;
	node = &inEnvironment->bnvNodeArray->nodes[inNodeIndex];

	return ARiPointInBSP(
			inEnvironment->planeArray->planes,
			inEnvironment->bspNodeArray->nodes,
			node->bspRootNode,
			inViewpoint,
			outRejectingPlane,
			outRejectionValue);

}

UUtBool
AKrPointInNodeVertically(
	const M3tPoint3D	*inPoint,
	const AKtEnvironment *inEnv,
	const AKtBNVNode *inNode)
{
	// Returns TRUE if the point is in the vertical space of the node
	const PHtRoomData *room = &inNode->roomData;

	return (inPoint->y >= room->origin.y && inPoint->y <= room->antiOrigin.y);
}

void
ONrPlatform_CopyAkiraToScreen(
	UUtUns16	inBufferWidth,
	UUtUns16	inBufferHeight,
	UUtUns16	inRowBytes,
	UUtUns16*	inBaseAdddr);

UUtError
AKrEnvironment_SetContextDimensions(
	AKtEnvironment*	inEnvironment,
	UUtUns16		inWidth,
	UUtUns16		inHeight)
{

	return UUcError_None;
}

enum
{
	AKcOctTreeFrustum_TrivAccept,
	AKcOctTreeFrustum_TrivReject,
	AKcOctTreeFrustum_Partial
};

AKtBNVNode*
AKrNodeFromPoint(
	const M3tPoint3D*		inPoint)
{
	float			v1,v2,d,dy,distance = 1e9f, stair_rel_y;
	AKtBNVNode		*closestNode = NULL,*bnv;
	AKtOctTree_LeafNode*	curLeafNode;
	UUtUns32				bnvIndexStart;
	UUtUns32				bnvIndexLength;
	UUtUns32				curBNVIndIndex;
	UUtUns32*				octTreeBNVIndices;
	UUtUns32				curBNVIndex;
	UUtBool					pointIsInNode, isStairNode, closestIsOutside = UUcTrue, closestIsStairs = UUcFalse;
	UUtUns32				curLeafNodeIndex;
	M3tPlaneEquation		reject_plane;

	UUmAssertReadPtr(inPoint, sizeof(*inPoint));
	UUmAssertReadPtr(AKgEnvironment, sizeof(*AKgEnvironment));
	UUmAssert(UUcError_None == M3rVerify_Point3D(inPoint));

	octTreeBNVIndices = AKgEnvironment->octTree->bnvIndices->indices;

	curLeafNodeIndex =
		AKrFindOctTreeNodeIndex(
			AKgEnvironment->octTree->interiorNodeArray->nodes,
			inPoint->x,
			inPoint->y,
			inPoint->z,
			NULL);

	curLeafNode = AKgEnvironment->octTree->leafNodeArray->nodes + AKmOctTree_GetLeafIndex(curLeafNodeIndex);

	bnvIndexStart = (UUtUns16)(curLeafNode->bnvIndirectIndex_Encode >> 8);
	bnvIndexLength = (UUtUns16)(curLeafNode->bnvIndirectIndex_Encode & 0xFF);

	for(curBNVIndIndex = bnvIndexStart;
		curBNVIndIndex < bnvIndexStart + bnvIndexLength;
		curBNVIndIndex++)
	{
		UUmAssert(curBNVIndIndex < AKgEnvironment->octTree->bnvIndices->numIndices);

		curBNVIndex = octTreeBNVIndices[curBNVIndIndex];

		pointIsInNode = AKrPointInNode(inPoint, AKgEnvironment, curBNVIndex,
										&reject_plane, &d);

		if (pointIsInNode) {
			d = 0;

		} else if (d >= AKcBNV_OutsideAcceptDistance) {
			// we are too far outside this node
			continue;

		} else if ((float)fabs(reject_plane.b) > PHcFlatNormal) {
			// we are outside a horizontal plane - reject
			continue;
		}

		bnv = &AKgEnvironment->bnvNodeArray->nodes[curBNVIndex];
		dy = inPoint->y - bnv->roomData.origin.y;
		if (dy < -AKcBNV_OutsideAcceptDistance) {
			// we are located under the floor of the BNV
			continue;
		}

		d += (float)fabs(dy);

		isStairNode = ((bnv->flags & AKcBNV_Flag_Stairs_Standard) != 0);

		if (isStairNode) {
			// reject stair nodes if we're outside their vertical extent
			stair_rel_y = inPoint->x * bnv->stairPlane.a + inPoint->y * bnv->stairPlane.b +
						inPoint->z * bnv->stairPlane.c + bnv->stairPlane.d;
			if (UUmFloat_CompareLT(stair_rel_y, 0.0f) || UUmFloat_CompareGT(stair_rel_y, bnv->stairHeight)) {
				continue;
			}
		}

		if (pointIsInNode) {
			if (closestIsOutside) {
				// inside nodes override outside ones
				distance = d;
				closestNode = bnv;
				closestIsOutside = UUcFalse;
				closestIsStairs = isStairNode;
				continue;
			}
		} else {
			if (!closestIsOutside) {
				// outside nodes have no effect on inside ones
				continue;
			}
		}

		// stair nodes override non-stair nodes
		if (isStairNode) {
			if (!closestIsStairs) {
				distance = d;
				closestNode = bnv;
				closestIsOutside = UUcFalse;
				closestIsStairs = isStairNode;
				continue;
			}
		} else {
			if (closestIsStairs) {
				continue;
			}
		}

		if (UUmFloat_CompareEqu(d,distance)) {
			// If the distance is the same, then check volumes and accept the smaller one
			UUmAssertReadPtr(closestNode, sizeof(*closestNode));
			UUmAssertReadPtr(bnv, sizeof(*bnv));

			v1 =(float)(fabs(closestNode->roomData.origin.x - closestNode->roomData.antiOrigin.x) *
				fabs(closestNode->roomData.origin.y - closestNode->roomData.antiOrigin.y) *
				fabs(closestNode->roomData.origin.z - closestNode->roomData.antiOrigin.z));
			v2 =(float)(fabs(bnv->roomData.origin.x - bnv->roomData.antiOrigin.x) *
				fabs(bnv->roomData.origin.y - bnv->roomData.antiOrigin.y) *
				fabs(bnv->roomData.origin.z - bnv->roomData.antiOrigin.z));

			if (v2<v1) closestNode = bnv;

		} else if (d < distance) {
			// pick the closer of the two nodes
			distance = d;
			closestNode = bnv;
		}
	}

	return closestNode;
}

UUtUns32
AKrGQToIndex(
	AKtEnvironment*		inEnvironment,
	AKtGQ_General*		inGQ)
{
	// Returns the absolute index corrosponding to a GQ

	return inGQ - inEnvironment->gqGeneralArray->gqGeneral;
}

const M3tBoundingBox_MinMax *g_nlgr_bounding_box;
UUtUns32 g_nlgr_max_count;
UUtUns32 g_nlgr_count;
UUtUns32 *g_nlgr_list;


static void AKrEnvironment_NodeList_Get_Recursive(
	AKtOctTree_InteriorNode*	inInteriorNodeArray,
	UUtUns32*					inLeafVisible2BV,
	M3tBoundingBox_MinMax*		inCurBox,
	UUtUns32					inCurNodeIndex)
{
	M3tBoundingBox_MinMax	newBox;
	float					curDim;
	UUtUns32				itr;

	if (g_nlgr_bounding_box->maxPoint.x < inCurBox->minPoint.x ||
		g_nlgr_bounding_box->maxPoint.y < inCurBox->minPoint.y ||
		g_nlgr_bounding_box->maxPoint.z < inCurBox->minPoint.z ||
		inCurBox->maxPoint.x < g_nlgr_bounding_box->minPoint.x ||
		inCurBox->maxPoint.y < g_nlgr_bounding_box->minPoint.y ||
		inCurBox->maxPoint.z < g_nlgr_bounding_box->minPoint.z)
	{
		goto exit;
	}

	if (AKmOctTree_IsLeafNode(inCurNodeIndex))
	{
		if (g_nlgr_count < g_nlgr_max_count) {
			g_nlgr_list[g_nlgr_count] = AKmOctTree_GetLeafIndex(inCurNodeIndex);
		}

		g_nlgr_count++;

		goto exit;
	}

	curDim = (inCurBox->maxPoint.x - inCurBox->minPoint.x) * 0.5f;

	for(itr = 0; itr < 8; itr++)
	{
		if(itr & 0x1) {
			newBox.minPoint.x = inCurBox->minPoint.x + curDim;
			newBox.maxPoint.x = inCurBox->maxPoint.x;
		}
		else {
			newBox.minPoint.x = inCurBox->minPoint.x;
			newBox.maxPoint.x = inCurBox->maxPoint.x - curDim;
		}

		if(itr & 0x2) {
			newBox.minPoint.y = inCurBox->minPoint.y + curDim;
			newBox.maxPoint.y = inCurBox->maxPoint.y;
		}
		else {
			newBox.minPoint.y = inCurBox->minPoint.y;
			newBox.maxPoint.y = inCurBox->maxPoint.y - curDim;
		}

		if(itr & 0x4) {
			newBox.minPoint.z = inCurBox->minPoint.z + curDim;
			newBox.maxPoint.z = inCurBox->maxPoint.z;
		}
		else {
			newBox.minPoint.z = inCurBox->minPoint.z;
			newBox.maxPoint.z = inCurBox->maxPoint.z - curDim;
		}

		AKrEnvironment_NodeList_Get_Recursive(
			inInteriorNodeArray,
			inLeafVisible2BV,
			&newBox,
			inInteriorNodeArray[inCurNodeIndex].children[itr]);

		if (g_nlgr_count > g_nlgr_max_count) {
			goto exit;
		}
	}

exit:
	return;
}

void AKrEnvironment_NodeList_Get(
	M3tBoundingBox_MinMax *inBoundingBox,
	UUtUns32 inMaxCount,
	UUtUns32 *outList,
	UUtUns32 startIndex)
{
	AKtEnvironment_Private *environmentPrivate= TMrTemplate_PrivateData_GetDataPtr(AKgTemplate_PrivateData, AKgEnvironment);
	M3tBoundingBox_MinMax box;

	if ((NULL != AKgEnvironment) && (inMaxCount != 0))
	{
		box.minPoint.x= -AKcMaxHalfOTDim;
		box.minPoint.y= -AKcMaxHalfOTDim;
		box.minPoint.z= -AKcMaxHalfOTDim;
		box.maxPoint.x= AKcMaxHalfOTDim;
		box.maxPoint.y= AKcMaxHalfOTDim;
		box.maxPoint.z= AKcMaxHalfOTDim;

		g_nlgr_bounding_box= inBoundingBox;
		g_nlgr_max_count= inMaxCount - 1;
		g_nlgr_count= 0;
		g_nlgr_list= outList + 1;

		AKrEnvironment_NodeList_Get_Recursive(
			AKgEnvironment->octTree->interiorNodeArray->nodes,
			environmentPrivate->ot2LeafNodeVisibility,
			&box, startIndex);

		outList[0]= (g_nlgr_count < g_nlgr_max_count) ? g_nlgr_count : 0xFFFFFFFF;
	}

	return;
}

UUtBool AKrEnvironment_NodeList_Visible(
	const UUtUns32 *inList)
{
	AKtEnvironment_Private*	environmentPrivate = TMrTemplate_PrivateData_GetDataPtr(AKgTemplate_PrivateData, AKgEnvironment);
	UUtUns32 *leaf_visible_bv = environmentPrivate->ot2LeafNodeVisibility;
	UUtBool is_visible = UUcFalse;

	if (0xFFFFFFFF == inList[0]) {
		is_visible = UUcTrue;
		goto exit;
	}

	{
		UUtUns32 itr;
		UUtUns32 count = inList[0];

		for(itr = 0; itr < count; itr++)
		{
			UUtUns32 node_index = inList[itr + 1];

			is_visible = UUr2BitVector_Test(leaf_visible_bv, node_index) > 0;

			if (is_visible) {
				break;
			}
		}
	}

exit:
	return is_visible;
}



#define AKcMaxStack	20

static UUtBool
AKrEnvironment_IsBoundingBoxMinMaxVisible_Recursive(
	AKtOctTree_InteriorNode*	inInteriorNodeArray,
	UUtUns32*					inLeafVisible2BV,
	M3tBoundingBox_MinMax*		inBoundingBox,
	M3tBoundingBox_MinMax*		inCurBox,
	UUtUns32					inCurNodeIndex,
	UUtUns32					*outVisibleNodeIndex) // optionally returned node index (only set if visible == TRUE)
{
	// the idea is for the caller to be able to use outVisibleNodeIndex
	// as inCurNodeIndex the next time they call this so as to save some work
	UUtBool visible;

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Enter(AKg_IsBoundingBoxMinMaxVisible_Recursive_Timer);
#endif

	//if(!CLrBox_MinMaxBox(inBoundingBox, inCurBox))
	// S.S. inlined because this is the only spot it is used and is getting called thousands of times per frame
	if (inBoundingBox->maxPoint.x < inCurBox->minPoint.x ||
		inBoundingBox->maxPoint.y < inCurBox->minPoint.y ||
		inBoundingBox->maxPoint.z < inCurBox->minPoint.z ||
		inCurBox->maxPoint.x < inBoundingBox->minPoint.x ||
		inCurBox->maxPoint.y < inBoundingBox->minPoint.y ||
		inCurBox->maxPoint.z < inBoundingBox->minPoint.z)
	{
		visible= UUcFalse;
	}
	else if (AKmOctTree_IsLeafNode(inCurNodeIndex))
	{
		visible= UUr2BitVector_Test(inLeafVisible2BV, AKmOctTree_GetLeafIndex(inCurNodeIndex)) > 0;
		if (visible && outVisibleNodeIndex)
		{
			*outVisibleNodeIndex= inCurNodeIndex;
		}
	}
	else
	{
		M3tBoundingBox_MinMax newBox;
		float cur_dim= (inCurBox->maxPoint.x - inCurBox->minPoint.x) * 0.5f;
		int itr;

		visible= UUcFalse;

		for (itr= 0; itr<8; itr++)
		{
			if(itr & 0x1)
			{
				newBox.minPoint.x= inCurBox->minPoint.x + cur_dim;
				newBox.maxPoint.x= inCurBox->maxPoint.x;
			}
			else
			{
				newBox.minPoint.x= inCurBox->minPoint.x;
				newBox.maxPoint.x= inCurBox->maxPoint.x - cur_dim;
			}

			if(itr & 0x2)
			{
				newBox.minPoint.y= inCurBox->minPoint.y + cur_dim;
				newBox.maxPoint.y= inCurBox->maxPoint.y;
			}
			else
			{
				newBox.minPoint.y= inCurBox->minPoint.y;
				newBox.maxPoint.y= inCurBox->maxPoint.y - cur_dim;
			}

			if(itr & 0x4)
			{
				newBox.minPoint.z= inCurBox->minPoint.z + cur_dim;
				newBox.maxPoint.z= inCurBox->maxPoint.z;
			}
			else
			{
				newBox.minPoint.z= inCurBox->minPoint.z;
				newBox.maxPoint.z= inCurBox->maxPoint.z - cur_dim;
			}

			if (AKrEnvironment_IsBoundingBoxMinMaxVisible_Recursive(
				inInteriorNodeArray,
				inLeafVisible2BV,
				inBoundingBox,
				&newBox,
				inInteriorNodeArray[inCurNodeIndex].children[itr],
				outVisibleNodeIndex))
			{
				visible= UUcTrue;
				break;
			}
		}
	}

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Exit(AKg_IsBoundingBoxMinMaxVisible_Recursive_Timer);
#endif

	return visible;
}

UUtBool AKrEnvironment_IsBoundingBoxMinMaxVisible(
	M3tBoundingBox_MinMax *bounding_box)
{
	// NOTE:
	// if you change this function make sure to also change
	// AKrEnvironment_IsBoundingBoxMinMaxVisible_WithNodeIndex() below!!!
	UUtBool visible;
	AKtEnvironment_Private *environment_private= TMrTemplate_PrivateData_GetDataPtr(AKgTemplate_PrivateData, AKgEnvironment);
	M3tBoundingBox_MinMax box;

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Enter(AKg_BBox_Visible_Timer);
#endif

	box.minPoint.x= -AKcMaxHalfOTDim;
	box.minPoint.y= -AKcMaxHalfOTDim;
	box.minPoint.z= -AKcMaxHalfOTDim;
	box.maxPoint.x= AKcMaxHalfOTDim;
	box.maxPoint.y= AKcMaxHalfOTDim;
	box.maxPoint.z= AKcMaxHalfOTDim;

	visible= AKrEnvironment_IsBoundingBoxMinMaxVisible_Recursive(
			AKgEnvironment->octTree->interiorNodeArray->nodes,
			environment_private->ot2LeafNodeVisibility,
			bounding_box, &box, 0, NULL);

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Exit(AKg_BBox_Visible_Timer);
#endif

	return visible;
}

// this must work exactly as AKrEnvironment_IsBoundingBoxMinMaxVisible
// except for the start & found oct-tree node indices!
UUtBool AKrEnvironment_IsBoundingBoxMinMaxVisible_WithNodeIndex(
	M3tBoundingBox_MinMax *bounding_box,
	UUtUns32 start_node,
	UUtUns32 *visible_node) // can be NULL
{
	UUtBool visible;
	AKtEnvironment_Private *environment_private= TMrTemplate_PrivateData_GetDataPtr(AKgTemplate_PrivateData, AKgEnvironment);
	M3tBoundingBox_MinMax box;

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Enter(AKg_BBox_Visible_Timer);
#endif

	box.minPoint.x= -AKcMaxHalfOTDim;
	box.minPoint.y= -AKcMaxHalfOTDim;
	box.minPoint.z= -AKcMaxHalfOTDim;
	box.maxPoint.x= AKcMaxHalfOTDim;
	box.maxPoint.y= AKcMaxHalfOTDim;
	box.maxPoint.z= AKcMaxHalfOTDim;

	visible= AKrEnvironment_IsBoundingBoxMinMaxVisible_Recursive(
			AKgEnvironment->octTree->interiorNodeArray->nodes,
			environment_private->ot2LeafNodeVisibility,
			bounding_box, &box, start_node, visible_node);

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Exit(AKg_BBox_Visible_Timer);
#endif

	return visible;
}

UUtBool
AKrEnvironment_PointIsVisible(
	const M3tPoint3D *inPoint)
{
	AKtEnvironment_Private*	environmentPrivate = TMrTemplate_PrivateData_GetDataPtr(AKgTemplate_PrivateData, AKgEnvironment);
	UUtUns32 *leaf_visible_bv = environmentPrivate->ot2LeafNodeVisibility;
	UUtUns32 node_index;
	UUtBool is_visible;

	node_index = AKrFindOctTreeNodeIndex(AKgEnvironment->octTree->interiorNodeArray->nodes, inPoint->x, inPoint->y, inPoint->z, NULL);

	is_visible = UUr2BitVector_Test(leaf_visible_bv, AKmOctTree_GetLeafIndex(node_index)) > 0;

	return is_visible;
}

UUtBool
AKrEnvironment_IsGeometryVisible(
		const M3tGeometry *inGeometry,
		const M3tMatrix4x3 *inMatrix)
{
	M3tBoundingBox_MinMax bbox;
	UUtBool is_visible;

	M3rGeometry_GetBBox(inGeometry, inMatrix, &bbox);

	is_visible = AKrEnvironment_IsBoundingBoxMinMaxVisible(&bbox);

	return is_visible;
}

void
AKrEnvironment_DrawIfVisible_Point(
		M3tGeometry *inGeometry,
		const M3tMatrix4x3 *inMatrix)
{
	M3tPoint3D center;

	MUrMatrix_MultiplyPoint(&inGeometry->pointArray->boundingSphere.center, inMatrix, &center);

	if (AKrEnvironment_PointIsVisible(&center)) {
		M3rGeometry_MultiplyAndDraw(inGeometry, inMatrix);
	}

	return;
}

void
AKrEnvironment_DrawIfVisible(
		M3tGeometry *inGeometry,
		const M3tMatrix4x3 *inMatrix)
{
	M3tBoundingBox_MinMax bbox;
	UUtBool is_visible;

	M3rGeometry_GetBBox(inGeometry, inMatrix, &bbox);

	is_visible = AKrEnvironment_IsBoundingBoxMinMaxVisible(&bbox);

	if (is_visible) {
		M3rGeometry_MultiplyAndDraw(inGeometry, inMatrix);
	}

	return;
}

static void AKiComputeBreakableFlags(AKtEnvironment *inEnvironment)
{
	UUtUns32 itr;
	UUtUns16 tex_index;
	M3tTextureMap *texturemap;
	MAtMaterialType materialtype;

	// process texture maps for 2 sided flag based on transparency
	for(itr = 0; itr < inEnvironment->gqGeneralArray->numGQs; itr++) {
		tex_index = inEnvironment->gqRenderArray->gqRender[itr].textureMapIndex;

		if (tex_index != 0xFFFF) {
			texturemap = inEnvironment->textureMapArray->maps[tex_index];
			materialtype = (MAtMaterialType) texturemap->materialType;

			if (MArMaterialType_IsBreakable(materialtype)) {
				inEnvironment->gqGeneralArray->gqGeneral[itr].flags |= AKcGQ_Flag_Breakable;
			}
		}
	}
}

