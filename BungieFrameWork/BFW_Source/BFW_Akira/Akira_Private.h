/*
	FILE:	BFW_Akira_Private.h

	AUTHOR:	Brent H. Pease, Chris Butcher, Michael Evans

	CREATED: October 23, 1997

	PURPOSE: environment engine

	Copyright 1997, 2000

*/
#ifndef BFW_AKIRA_PRIVATE_H
#define BFW_AKIRA_PRIVATE_H

#include "BFW_AppUtilities.h"
#include "BFW_Timer.h"

#define	AKcMaxElements				(5 * 1024)

#define AKcDynamicLM_Num			10

#define AKcMaxReflectionQuads		32

#define AKcMaxTransparentGQs		(2 * 1024)

// CB: increased from 2048 to 4096 to handle powerplant I skipouts
// ME: increased from 4096 to 8k to handle the rooftops
#define AKcMaxVisibleGQs			(8 * 1024)

#define AKcTextureMap_Dynamic_Name	"akira dyn cm"

#define AKcOctNodeIndex_Uncached	((UUtUns32) -1)

#if PERFORMANCE_TIMER
// performance timers
extern UUtPerformanceTimer *AKg_RayCastOctTree_Timer;
extern UUtPerformanceTimer *AKg_Collision_Sphere_Timer;
extern UUtPerformanceTimer *AKg_ComputeVis_Timer;
extern UUtPerformanceTimer *AKg_BBox_Visible_Timer;
extern UUtPerformanceTimer *AKg_GQ_SphereVector_Timer;
extern UUtPerformanceTimer *AKg_Collision_Point_Timer;
extern UUtPerformanceTimer *AKg_IsBoundingBoxMinMaxVisible_Recursive_Timer;
#endif


typedef struct AKtRay
{
	float	u, v;

} AKtRay;

typedef struct AKtFrame
{
	UUtUns32	numRays;
	AKtRay*		rays;
} AKtFrame;

typedef struct AKtEnvironment_Private
{

	UUtUns32*		gqComputedBackFaceBV;
	UUtUns32*		gqBackFaceBV;
	UUtUns32*		gq2VisibilityVector;
	UUtUns32*		ot2LeafNodeVisibility;

	M3tGeomCamera*	geomCamera;

	//UUtUns32*		alphaGQVisBV;

	UUtUns32		visGQ_Num;
	UUtUns32*		visGQ_List;
	UUtUns32		sky_visibility;

} AKtEnvironment_Private;

extern UUtBool 			gEnvironmentCollision;
extern M3tTextureMap*	AKgNoneTexture;
extern UUtBool			AKgDebug_ShowGQsInOctNode;
extern UUtBool			AKgDebug_DrawAllGunks;
extern UUtBool			AKgDebug_DrawVisOnly;
extern UUtBool			AKgDebug_ShowOctTree;
extern UUtBool			AKgDebug_ShowLeafNodes;
//extern float			AKgDebug_VisIgnoreThreshold;
extern UUtBool			AKgDebug_ShowRays;
extern UUtBool			AKgShowStairFlagged,AKgShowMaterials;
extern UUtUns32			AKgHighlightGQIndex;

extern float			AKgIntToFloatXYZ[];
extern float			AKgIntToFloatDim[];

extern UUtUns32		gFrameNum;
extern AUtDict		*AKgDebugLeafNodes;

#define AKcMaxColDebugEntries	(100)
extern UUtUns16				AKgNumColDebugEntries;
extern M3tBoundingSphere	AKgDebugEntry_Sphere[AKcMaxColDebugEntries];
extern M3tVector3D			AKgDebugEntry_Vector[AKcMaxColDebugEntries];
extern UUtBool				AKgDrawGhostGQs;
extern UUtBool				AKgUseSIMD;

extern UUtUns16	AKgReflectedPlaneIndex;
extern UUtUns16	AKgReflectionNum;
extern UUtUns16	AKgReflectionList[AKcMaxReflectionQuads];

extern TMtPrivateData*		AKgTemplate_PrivateData;

void
AKrEnvironment_PrintStats(
	void);
void
AKrEnvironment_Collision_PrintStats(
	void);


void
AKiBoundingBox_Draw(
	M3tPoint3D*			inMinPoint,
	M3tPoint3D*			inMaxPoint,
	UUtUns16			inColor);

UUtUns32
AKrFindOctTreeNodeIndex(
	AKtOctTree_InteriorNode*	inInteriorNodeArray,
	float						inPointX,
	float						inPointY,
	float						inPointZ,
	AKtOctNodeIndexCache*		ioCache);

UUtError
AKrRayCastCoefficients_Initialize(UUtUns32 inNumDivs);

void
AKrRayCastCoefficients_Terminate(
	void);

UUtError AKrCollision_LevelBegin(AKtEnvironment *inEnvironment);
void AKrCollision_LevelEnd(void);


#endif /* BFW_AKIRA_PRIVATE_H */

