/*
	FILE:	BFW_Akira_Render.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: October 12, 1998

	PURPOSE: environment engine
	
	Copyright 1997

*/

#include <stdlib.h>
#include <ctype.h>

#include "BFW.h"
#include "BFW_TemplateManager.h"
#ifdef UUmPlatform_SonyPSX2_T
#include "PSX2_Motoko.h"
#else
#include "BFW_Motoko.h"
#endif

#include "BFW_Akira.h"
#include "BFW_Console.h"
#include "BFW_BitVector.h"
#include "BFW_AppUtilities.h"
#include "BFW_Collision.h"
#include "BFW_Shared_Math.h"
#include "BFW_BitVector.h"
#include "BFW_Object.h"
#include "BFW_Timer.h"

			
#include "Akira_Private.h"

#include "Oni_GameStatePrivate.h"
#include "Oni_Motoko.h"

//#define OCT_TREE_TOOL_SUPPORT 0

#ifndef OCT_TREE_TOOL_SUPPORT

#if TOOL_VERSION
#define OCT_TREE_TOOL_SUPPORT 1
#else
#define OCT_TREE_TOOL_SUPPORT 0
#endif

#endif

#ifdef ROCKSTAR
#include	"PSX2_InstanceObjects.h"
#endif

#if UUmPlatform == UUmPlatform_SonyPSX2
#include "PSX2_Stack.h"
#endif
#ifdef ROCKSTAR
#include "PSX2_InstanceObjects.h"
#endif


UUtBool		AKgDraw_Occl = UUcFalse;
UUtBool		AKgDraw_SoundOccl = UUcFalse;
UUtBool		AKgDraw_Invis = UUcFalse;
UUtBool		AKgDraw_ObjectCollision = UUcFalse;
UUtBool		AKgDraw_CharacterCollision = UUcFalse;

AKtFrame	AKgFrame = {0, NULL};

UUtUns32	gFrameNum = 0;


AUtDict	*AKgDebugLeafNodes;

typedef struct AKtRayOffset
{
	float u_offset;
	float v_offset;
} AKtRayOffset;

#define AKcNumTemperalFrames	25

AKtRayOffset AKgSourceRayOffset[AKcNumTemperalFrames] =
{
	{ 0.00f, 0.00f },
	{ 0.60f, 0.60f },
	{ 0.00f, 0.60f },
	{ 0.60f, 0.00f },

	{ 0.00f, 0.20f },
	{ 0.00f, 0.40f },
	{ 0.00f, 0.80f },

	{ 0.20f, 0.00f },
	{ 0.20f, 0.20f },
	{ 0.20f, 0.40f },
	{ 0.20f, 0.60f },
	{ 0.20f, 0.80f },

	{ 0.40f, 0.00f },
	{ 0.40f, 0.20f },
	{ 0.40f, 0.40f },
	{ 0.40f, 0.60f },
	{ 0.40f, 0.80f },

	{ 0.60f, 0.20f },
	{ 0.60f, 0.40f },
	{ 0.60f, 0.80f },

	{ 0.80f, 0.00f },
	{ 0.80f, 0.20f },
	{ 0.80f, 0.40f },
	{ 0.80f, 0.60f },
	{ 0.80f, 0.80f }
};

AKtRayOffset AKgRayOffset[AKcNumTemperalFrames];



UUtUns16			AKgNumColDebugEntries = 0;
M3tBoundingSphere	AKgDebugEntry_Sphere[AKcMaxColDebugEntries];
M3tVector3D			AKgDebugEntry_Vector[AKcMaxColDebugEntries];

void
AKiBoundingBox_Draw(
	M3tPoint3D*			inMinPoint,
	M3tPoint3D*			inMaxPoint,
	UUtUns16			inColor)
{
	UUtUns8 block[sizeof(M3tPoint3D) * 5 + 2 * UUcProcessor_CacheLineSize];
	M3tPoint3D *aligned = (M3tPoint3D *) UUrAlignMemory(block);

	/*
		0 - min, 1 - max
		
		v	X	Y	Z
		==	=========
		0	0	0	0
		1	0	0	1
		2	0	1	0
		3	0	1	1
		4	1	0	0
		5	1	0	1
		6	1	1	0
		7	1	1	1
		
		Edges
		=====
		0 -> 1
		0 -> 2
		0 -> 4
		1 -> 3
		1 -> 5
		2 -> 3
		2 -> 6
		3 -> 7
		4 -> 5
		4 -> 6
		5 -> 7
		6 -> 7
	*/

	// 0 -> 1
		aligned[0].x = inMinPoint->x;
		aligned[0].y = inMinPoint->y;
		aligned[0].z = inMinPoint->z;
		
		aligned[1].x = inMinPoint->x;
		aligned[1].y = inMinPoint->y;
		aligned[1].z = inMaxPoint->z;
		M3rGeometry_LineDraw(
			2,
			aligned,
			inColor);
		
		
	// 0 -> 2
		aligned[0].x = inMinPoint->x;
		aligned[0].y = inMinPoint->y;
		aligned[0].z = inMinPoint->z;
		
		aligned[1].x = inMinPoint->x;
		aligned[1].y = inMaxPoint->y;
		aligned[1].z = inMinPoint->z;
		M3rGeometry_LineDraw(
			2,
			aligned,
			inColor);
		
		
	// 0 -> 4
		aligned[0].x = inMinPoint->x;
		aligned[0].y = inMinPoint->y;
		aligned[0].z = inMinPoint->z;
		
		aligned[1].x = inMaxPoint->x;
		aligned[1].y = inMinPoint->y;
		aligned[1].z = inMinPoint->z;
		M3rGeometry_LineDraw(
			2,
			aligned,
			inColor);
		
		
	// 1 -> 3
		aligned[0].x = inMinPoint->x;
		aligned[0].y = inMinPoint->y;
		aligned[0].z = inMaxPoint->z;
		
		aligned[1].x = inMinPoint->x;
		aligned[1].y = inMaxPoint->y;
		aligned[1].z = inMaxPoint->z;
		M3rGeometry_LineDraw(
			2,
			aligned,
			inColor);
		
		
	// 1 -> 5
		aligned[0].x = inMinPoint->x;
		aligned[0].y = inMinPoint->y;
		aligned[0].z = inMaxPoint->z;
		
		aligned[1].x = inMaxPoint->x;
		aligned[1].y = inMinPoint->y;
		aligned[1].z = inMaxPoint->z;
		M3rGeometry_LineDraw(
			2,
			aligned,
			inColor);
		
	// 2 -> 3
		aligned[0].x = inMinPoint->x;
		aligned[0].y = inMaxPoint->y;
		aligned[0].z = inMinPoint->z;
		
		aligned[1].x = inMinPoint->x;
		aligned[1].y = inMaxPoint->y;
		aligned[1].z = inMaxPoint->z;
		M3rGeometry_LineDraw(
			2,
			aligned,
			inColor);
		
		
	// 2 -> 6
		aligned[0].x = inMinPoint->x;
		aligned[0].y = inMaxPoint->y;
		aligned[0].z = inMinPoint->z;
		
		aligned[1].x = inMaxPoint->x;
		aligned[1].y = inMaxPoint->y;
		aligned[1].z = inMinPoint->z;
		M3rGeometry_LineDraw(
			2,
			aligned,
			inColor);
		
		
	// 3 -> 7
		aligned[0].x = inMinPoint->x;
		aligned[0].y = inMaxPoint->y;
		aligned[0].z = inMaxPoint->z;
		
		aligned[1].x = inMaxPoint->x;
		aligned[1].y = inMaxPoint->y;
		aligned[1].z = inMaxPoint->z;
		M3rGeometry_LineDraw(
			2,
			aligned,
			inColor);
		
		
	// 4 -> 5
		aligned[0].x = inMaxPoint->x;
		aligned[0].y = inMinPoint->y;
		aligned[0].z = inMinPoint->z;
		
		aligned[1].x = inMaxPoint->x;
		aligned[1].y = inMinPoint->y;
		aligned[1].z = inMaxPoint->z;
		M3rGeometry_LineDraw(
			2,
			aligned,
			inColor);
		
		
	// 4 -> 6
		aligned[0].x = inMaxPoint->x;
		aligned[0].y = inMinPoint->y;
		aligned[0].z = inMinPoint->z;
		
		aligned[1].x = inMaxPoint->x;
		aligned[1].y = inMaxPoint->y;
		aligned[1].z = inMinPoint->z;
		M3rGeometry_LineDraw(
			2,
			aligned,
			inColor);
		
		
	// 5 -> 7
		aligned[0].x = inMaxPoint->x;
		aligned[0].y = inMinPoint->y;
		aligned[0].z = inMaxPoint->z;
		
		aligned[1].x = inMaxPoint->x;
		aligned[1].y = inMaxPoint->y;
		aligned[1].z = inMaxPoint->z;
		M3rGeometry_LineDraw(
			2,
			aligned,
			inColor);
		
		
	// 6 -> 7
		aligned[0].x = inMaxPoint->x;
		aligned[0].y = inMaxPoint->y;
		aligned[0].z = inMinPoint->z;
		
		aligned[1].x = inMaxPoint->x;
		aligned[1].y = inMaxPoint->y;
		aligned[1].z = inMaxPoint->z;
		M3rGeometry_LineDraw(
			2,
			aligned,
			inColor);
}

#define BRENTS_CHEESY_PROFILE_ONLY_FUNC 0
#define BRENTS_CHEESY_PROFILE 0

	
	UUtUns32	funcTime = 0;
	UUtUns32	funcNum = 0;

	UUtUns32	funcRayTime = 0;
	UUtUns32	funcNumRays = 0;
	
	UUtUns32	funcOctantTime = 0;
	UUtUns32	funcNumMissOctants = 0;
	UUtUns32	funcNumHitOctants = 0;
	UUtUns32	funcNumSkippedOctants = 0;
	
	UUtUns32	funcMissOctant_QuadTraversalTime = 0;
	UUtUns32	funcMissOctant_NumQuadTraversals = 0;
	UUtUns32	funcMissOctant_NumQuadChecks = 0;
	
	UUtUns32	funcHitOctant_QuadTraversalTime = 0;
	UUtUns32	funcHitOctant_NumQuadTraversals = 0;
	UUtUns32	funcHitOctant_NumQuadChecks = 0;
	
	UUtUns32	funcHitOctant_NumQuadsTillHit = 0;

float	AKgIntToFloatDim[9] = 
{
	AKcMinOTDim,
	AKcMinOTDim * 2.0f,
	AKcMinOTDim * 4.0f,
	AKcMinOTDim * 8.0f,
	AKcMinOTDim * 16.0f,
	AKcMinOTDim * 32.0f,
	AKcMinOTDim * 64.0f,
	AKcMinOTDim * 128.0f,
	AKcMinOTDim * 256.0f,
};

float	AKgIntToFloatXYZ[512] =
{
    -255.000000 * AKcMinOTDim,  -254.000000 * AKcMinOTDim,  -253.000000 * AKcMinOTDim,  -252.000000 * AKcMinOTDim, 
    -251.000000 * AKcMinOTDim,  -250.000000 * AKcMinOTDim,  -249.000000 * AKcMinOTDim,  -248.000000 * AKcMinOTDim, 
    -247.000000 * AKcMinOTDim,  -246.000000 * AKcMinOTDim,  -245.000000 * AKcMinOTDim,  -244.000000 * AKcMinOTDim, 
    -243.000000 * AKcMinOTDim,  -242.000000 * AKcMinOTDim,  -241.000000 * AKcMinOTDim,  -240.000000 * AKcMinOTDim, 
    -239.000000 * AKcMinOTDim,  -238.000000 * AKcMinOTDim,  -237.000000 * AKcMinOTDim,  -236.000000 * AKcMinOTDim, 
    -235.000000 * AKcMinOTDim,  -234.000000 * AKcMinOTDim,  -233.000000 * AKcMinOTDim,  -232.000000 * AKcMinOTDim, 
    -231.000000 * AKcMinOTDim,  -230.000000 * AKcMinOTDim,  -229.000000 * AKcMinOTDim,  -228.000000 * AKcMinOTDim, 
    -227.000000 * AKcMinOTDim,  -226.000000 * AKcMinOTDim,  -225.000000 * AKcMinOTDim,  -224.000000 * AKcMinOTDim, 
    -223.000000 * AKcMinOTDim,  -222.000000 * AKcMinOTDim,  -221.000000 * AKcMinOTDim,  -220.000000 * AKcMinOTDim, 
    -219.000000 * AKcMinOTDim,  -218.000000 * AKcMinOTDim,  -217.000000 * AKcMinOTDim,  -216.000000 * AKcMinOTDim, 
    -215.000000 * AKcMinOTDim,  -214.000000 * AKcMinOTDim,  -213.000000 * AKcMinOTDim,  -212.000000 * AKcMinOTDim, 
    -211.000000 * AKcMinOTDim,  -210.000000 * AKcMinOTDim,  -209.000000 * AKcMinOTDim,  -208.000000 * AKcMinOTDim, 
    -207.000000 * AKcMinOTDim,  -206.000000 * AKcMinOTDim,  -205.000000 * AKcMinOTDim,  -204.000000 * AKcMinOTDim, 
    -203.000000 * AKcMinOTDim,  -202.000000 * AKcMinOTDim,  -201.000000 * AKcMinOTDim,  -200.000000 * AKcMinOTDim, 
    -199.000000 * AKcMinOTDim,  -198.000000 * AKcMinOTDim,  -197.000000 * AKcMinOTDim,  -196.000000 * AKcMinOTDim, 
    -195.000000 * AKcMinOTDim,  -194.000000 * AKcMinOTDim,  -193.000000 * AKcMinOTDim,  -192.000000 * AKcMinOTDim, 
    -191.000000 * AKcMinOTDim,  -190.000000 * AKcMinOTDim,  -189.000000 * AKcMinOTDim,  -188.000000 * AKcMinOTDim, 
    -187.000000 * AKcMinOTDim,  -186.000000 * AKcMinOTDim,  -185.000000 * AKcMinOTDim,  -184.000000 * AKcMinOTDim, 
    -183.000000 * AKcMinOTDim,  -182.000000 * AKcMinOTDim,  -181.000000 * AKcMinOTDim,  -180.000000 * AKcMinOTDim, 
    -179.000000 * AKcMinOTDim,  -178.000000 * AKcMinOTDim,  -177.000000 * AKcMinOTDim,  -176.000000 * AKcMinOTDim, 
    -175.000000 * AKcMinOTDim,  -174.000000 * AKcMinOTDim,  -173.000000 * AKcMinOTDim,  -172.000000 * AKcMinOTDim, 
    -171.000000 * AKcMinOTDim,  -170.000000 * AKcMinOTDim,  -169.000000 * AKcMinOTDim,  -168.000000 * AKcMinOTDim, 
    -167.000000 * AKcMinOTDim,  -166.000000 * AKcMinOTDim,  -165.000000 * AKcMinOTDim,  -164.000000 * AKcMinOTDim, 
    -163.000000 * AKcMinOTDim,  -162.000000 * AKcMinOTDim,  -161.000000 * AKcMinOTDim,  -160.000000 * AKcMinOTDim, 
    -159.000000 * AKcMinOTDim,  -158.000000 * AKcMinOTDim,  -157.000000 * AKcMinOTDim,  -156.000000 * AKcMinOTDim, 
    -155.000000 * AKcMinOTDim,  -154.000000 * AKcMinOTDim,  -153.000000 * AKcMinOTDim,  -152.000000 * AKcMinOTDim, 
    -151.000000 * AKcMinOTDim,  -150.000000 * AKcMinOTDim,  -149.000000 * AKcMinOTDim,  -148.000000 * AKcMinOTDim, 
    -147.000000 * AKcMinOTDim,  -146.000000 * AKcMinOTDim,  -145.000000 * AKcMinOTDim,  -144.000000 * AKcMinOTDim, 
    -143.000000 * AKcMinOTDim,  -142.000000 * AKcMinOTDim,  -141.000000 * AKcMinOTDim,  -140.000000 * AKcMinOTDim, 
    -139.000000 * AKcMinOTDim,  -138.000000 * AKcMinOTDim,  -137.000000 * AKcMinOTDim,  -136.000000 * AKcMinOTDim, 
    -135.000000 * AKcMinOTDim,  -134.000000 * AKcMinOTDim,  -133.000000 * AKcMinOTDim,  -132.000000 * AKcMinOTDim, 
    -131.000000 * AKcMinOTDim,  -130.000000 * AKcMinOTDim,  -129.000000 * AKcMinOTDim,  -128.000000 * AKcMinOTDim, 
    -127.000000 * AKcMinOTDim,  -126.000000 * AKcMinOTDim,  -125.000000 * AKcMinOTDim,  -124.000000 * AKcMinOTDim, 
    -123.000000 * AKcMinOTDim,  -122.000000 * AKcMinOTDim,  -121.000000 * AKcMinOTDim,  -120.000000 * AKcMinOTDim, 
    -119.000000 * AKcMinOTDim,  -118.000000 * AKcMinOTDim,  -117.000000 * AKcMinOTDim,  -116.000000 * AKcMinOTDim, 
    -115.000000 * AKcMinOTDim,  -114.000000 * AKcMinOTDim,  -113.000000 * AKcMinOTDim,  -112.000000 * AKcMinOTDim, 
    -111.000000 * AKcMinOTDim,  -110.000000 * AKcMinOTDim,  -109.000000 * AKcMinOTDim,  -108.000000 * AKcMinOTDim, 
    -107.000000 * AKcMinOTDim,  -106.000000 * AKcMinOTDim,  -105.000000 * AKcMinOTDim,  -104.000000 * AKcMinOTDim, 
    -103.000000 * AKcMinOTDim,  -102.000000 * AKcMinOTDim,  -101.000000 * AKcMinOTDim,  -100.000000 * AKcMinOTDim, 
    -99.000000 * AKcMinOTDim,   -98.000000 * AKcMinOTDim,   -97.000000 * AKcMinOTDim,   -96.000000 * AKcMinOTDim, 
    -95.000000 * AKcMinOTDim,   -94.000000 * AKcMinOTDim,   -93.000000 * AKcMinOTDim,   -92.000000 * AKcMinOTDim, 
    -91.000000 * AKcMinOTDim,   -90.000000 * AKcMinOTDim,   -89.000000 * AKcMinOTDim,   -88.000000 * AKcMinOTDim, 
    -87.000000 * AKcMinOTDim,   -86.000000 * AKcMinOTDim,   -85.000000 * AKcMinOTDim,   -84.000000 * AKcMinOTDim, 
    -83.000000 * AKcMinOTDim,   -82.000000 * AKcMinOTDim,   -81.000000 * AKcMinOTDim,   -80.000000 * AKcMinOTDim, 
    -79.000000 * AKcMinOTDim,   -78.000000 * AKcMinOTDim,   -77.000000 * AKcMinOTDim,   -76.000000 * AKcMinOTDim, 
    -75.000000 * AKcMinOTDim,   -74.000000 * AKcMinOTDim,   -73.000000 * AKcMinOTDim,   -72.000000 * AKcMinOTDim, 
    -71.000000 * AKcMinOTDim,   -70.000000 * AKcMinOTDim,   -69.000000 * AKcMinOTDim,   -68.000000 * AKcMinOTDim, 
    -67.000000 * AKcMinOTDim,   -66.000000 * AKcMinOTDim,   -65.000000 * AKcMinOTDim,   -64.000000 * AKcMinOTDim, 
    -63.000000 * AKcMinOTDim,   -62.000000 * AKcMinOTDim,   -61.000000 * AKcMinOTDim,   -60.000000 * AKcMinOTDim, 
    -59.000000 * AKcMinOTDim,   -58.000000 * AKcMinOTDim,   -57.000000 * AKcMinOTDim,   -56.000000 * AKcMinOTDim, 
    -55.000000 * AKcMinOTDim,   -54.000000 * AKcMinOTDim,   -53.000000 * AKcMinOTDim,   -52.000000 * AKcMinOTDim, 
    -51.000000 * AKcMinOTDim,   -50.000000 * AKcMinOTDim,   -49.000000 * AKcMinOTDim,   -48.000000 * AKcMinOTDim, 
    -47.000000 * AKcMinOTDim,   -46.000000 * AKcMinOTDim,   -45.000000 * AKcMinOTDim,   -44.000000 * AKcMinOTDim, 
    -43.000000 * AKcMinOTDim,   -42.000000 * AKcMinOTDim,   -41.000000 * AKcMinOTDim,   -40.000000 * AKcMinOTDim, 
    -39.000000 * AKcMinOTDim,   -38.000000 * AKcMinOTDim,   -37.000000 * AKcMinOTDim,   -36.000000 * AKcMinOTDim, 
    -35.000000 * AKcMinOTDim,   -34.000000 * AKcMinOTDim,   -33.000000 * AKcMinOTDim,   -32.000000 * AKcMinOTDim, 
    -31.000000 * AKcMinOTDim,   -30.000000 * AKcMinOTDim,   -29.000000 * AKcMinOTDim,   -28.000000 * AKcMinOTDim, 
    -27.000000 * AKcMinOTDim,   -26.000000 * AKcMinOTDim,   -25.000000 * AKcMinOTDim,   -24.000000 * AKcMinOTDim, 
    -23.000000 * AKcMinOTDim,   -22.000000 * AKcMinOTDim,   -21.000000 * AKcMinOTDim,   -20.000000 * AKcMinOTDim, 
    -19.000000 * AKcMinOTDim,   -18.000000 * AKcMinOTDim,   -17.000000 * AKcMinOTDim,   -16.000000 * AKcMinOTDim, 
    -15.000000 * AKcMinOTDim,   -14.000000 * AKcMinOTDim,   -13.000000 * AKcMinOTDim,   -12.000000 * AKcMinOTDim, 
    -11.000000 * AKcMinOTDim,   -10.000000 * AKcMinOTDim,   -9.000000 * AKcMinOTDim,    -8.000000 * AKcMinOTDim, 
    -7.000000 * AKcMinOTDim,    -6.000000 * AKcMinOTDim,    -5.000000 * AKcMinOTDim,    -4.000000 * AKcMinOTDim, 
    -3.000000 * AKcMinOTDim,    -2.000000 * AKcMinOTDim,    -1.000000 * AKcMinOTDim,    0.000000 * AKcMinOTDim, 
    1.000000 * AKcMinOTDim,     2.000000 * AKcMinOTDim,     3.000000 * AKcMinOTDim,     4.000000 * AKcMinOTDim, 
    5.000000 * AKcMinOTDim,     6.000000 * AKcMinOTDim,     7.000000 * AKcMinOTDim,     8.000000 * AKcMinOTDim, 
    9.000000 * AKcMinOTDim,     10.000000 * AKcMinOTDim,    11.000000 * AKcMinOTDim,    12.000000 * AKcMinOTDim, 
    13.000000 * AKcMinOTDim,    14.000000 * AKcMinOTDim,    15.000000 * AKcMinOTDim,    16.000000 * AKcMinOTDim, 
    17.000000 * AKcMinOTDim,    18.000000 * AKcMinOTDim,    19.000000 * AKcMinOTDim,    20.000000 * AKcMinOTDim, 
    21.000000 * AKcMinOTDim,    22.000000 * AKcMinOTDim,    23.000000 * AKcMinOTDim,    24.000000 * AKcMinOTDim, 
    25.000000 * AKcMinOTDim,    26.000000 * AKcMinOTDim,    27.000000 * AKcMinOTDim,    28.000000 * AKcMinOTDim, 
    29.000000 * AKcMinOTDim,    30.000000 * AKcMinOTDim,    31.000000 * AKcMinOTDim,    32.000000 * AKcMinOTDim, 
    33.000000 * AKcMinOTDim,    34.000000 * AKcMinOTDim,    35.000000 * AKcMinOTDim,    36.000000 * AKcMinOTDim, 
    37.000000 * AKcMinOTDim,    38.000000 * AKcMinOTDim,    39.000000 * AKcMinOTDim,    40.000000 * AKcMinOTDim, 
    41.000000 * AKcMinOTDim,    42.000000 * AKcMinOTDim,    43.000000 * AKcMinOTDim,    44.000000 * AKcMinOTDim, 
    45.000000 * AKcMinOTDim,    46.000000 * AKcMinOTDim,    47.000000 * AKcMinOTDim,    48.000000 * AKcMinOTDim, 
    49.000000 * AKcMinOTDim,    50.000000 * AKcMinOTDim,    51.000000 * AKcMinOTDim,    52.000000 * AKcMinOTDim, 
    53.000000 * AKcMinOTDim,    54.000000 * AKcMinOTDim,    55.000000 * AKcMinOTDim,    56.000000 * AKcMinOTDim, 
    57.000000 * AKcMinOTDim,    58.000000 * AKcMinOTDim,    59.000000 * AKcMinOTDim,    60.000000 * AKcMinOTDim, 
    61.000000 * AKcMinOTDim,    62.000000 * AKcMinOTDim,    63.000000 * AKcMinOTDim,    64.000000 * AKcMinOTDim, 
    65.000000 * AKcMinOTDim,    66.000000 * AKcMinOTDim,    67.000000 * AKcMinOTDim,    68.000000 * AKcMinOTDim, 
    69.000000 * AKcMinOTDim,    70.000000 * AKcMinOTDim,    71.000000 * AKcMinOTDim,    72.000000 * AKcMinOTDim, 
    73.000000 * AKcMinOTDim,    74.000000 * AKcMinOTDim,    75.000000 * AKcMinOTDim,    76.000000 * AKcMinOTDim, 
    77.000000 * AKcMinOTDim,    78.000000 * AKcMinOTDim,    79.000000 * AKcMinOTDim,    80.000000 * AKcMinOTDim, 
    81.000000 * AKcMinOTDim,    82.000000 * AKcMinOTDim,    83.000000 * AKcMinOTDim,    84.000000 * AKcMinOTDim, 
    85.000000 * AKcMinOTDim,    86.000000 * AKcMinOTDim,    87.000000 * AKcMinOTDim,    88.000000 * AKcMinOTDim, 
    89.000000 * AKcMinOTDim,    90.000000 * AKcMinOTDim,    91.000000 * AKcMinOTDim,    92.000000 * AKcMinOTDim, 
    93.000000 * AKcMinOTDim,    94.000000 * AKcMinOTDim,    95.000000 * AKcMinOTDim,    96.000000 * AKcMinOTDim, 
    97.000000 * AKcMinOTDim,    98.000000 * AKcMinOTDim,    99.000000 * AKcMinOTDim,    100.000000 * AKcMinOTDim, 
    101.000000 * AKcMinOTDim,   102.000000 * AKcMinOTDim,   103.000000 * AKcMinOTDim,   104.000000 * AKcMinOTDim, 
    105.000000 * AKcMinOTDim,   106.000000 * AKcMinOTDim,   107.000000 * AKcMinOTDim,   108.000000 * AKcMinOTDim, 
    109.000000 * AKcMinOTDim,   110.000000 * AKcMinOTDim,   111.000000 * AKcMinOTDim,   112.000000 * AKcMinOTDim, 
    113.000000 * AKcMinOTDim,   114.000000 * AKcMinOTDim,   115.000000 * AKcMinOTDim,   116.000000 * AKcMinOTDim, 
    117.000000 * AKcMinOTDim,   118.000000 * AKcMinOTDim,   119.000000 * AKcMinOTDim,   120.000000 * AKcMinOTDim, 
    121.000000 * AKcMinOTDim,   122.000000 * AKcMinOTDim,   123.000000 * AKcMinOTDim,   124.000000 * AKcMinOTDim, 
    125.000000 * AKcMinOTDim,   126.000000 * AKcMinOTDim,   127.000000 * AKcMinOTDim,   128.000000 * AKcMinOTDim, 
    129.000000 * AKcMinOTDim,   130.000000 * AKcMinOTDim,   131.000000 * AKcMinOTDim,   132.000000 * AKcMinOTDim, 
    133.000000 * AKcMinOTDim,   134.000000 * AKcMinOTDim,   135.000000 * AKcMinOTDim,   136.000000 * AKcMinOTDim, 
    137.000000 * AKcMinOTDim,   138.000000 * AKcMinOTDim,   139.000000 * AKcMinOTDim,   140.000000 * AKcMinOTDim, 
    141.000000 * AKcMinOTDim,   142.000000 * AKcMinOTDim,   143.000000 * AKcMinOTDim,   144.000000 * AKcMinOTDim, 
    145.000000 * AKcMinOTDim,   146.000000 * AKcMinOTDim,   147.000000 * AKcMinOTDim,   148.000000 * AKcMinOTDim, 
    149.000000 * AKcMinOTDim,   150.000000 * AKcMinOTDim,   151.000000 * AKcMinOTDim,   152.000000 * AKcMinOTDim, 
    153.000000 * AKcMinOTDim,   154.000000 * AKcMinOTDim,   155.000000 * AKcMinOTDim,   156.000000 * AKcMinOTDim, 
    157.000000 * AKcMinOTDim,   158.000000 * AKcMinOTDim,   159.000000 * AKcMinOTDim,   160.000000 * AKcMinOTDim, 
    161.000000 * AKcMinOTDim,   162.000000 * AKcMinOTDim,   163.000000 * AKcMinOTDim,   164.000000 * AKcMinOTDim, 
    165.000000 * AKcMinOTDim,   166.000000 * AKcMinOTDim,   167.000000 * AKcMinOTDim,   168.000000 * AKcMinOTDim, 
    169.000000 * AKcMinOTDim,   170.000000 * AKcMinOTDim,   171.000000 * AKcMinOTDim,   172.000000 * AKcMinOTDim, 
    173.000000 * AKcMinOTDim,   174.000000 * AKcMinOTDim,   175.000000 * AKcMinOTDim,   176.000000 * AKcMinOTDim, 
    177.000000 * AKcMinOTDim,   178.000000 * AKcMinOTDim,   179.000000 * AKcMinOTDim,   180.000000 * AKcMinOTDim, 
    181.000000 * AKcMinOTDim,   182.000000 * AKcMinOTDim,   183.000000 * AKcMinOTDim,   184.000000 * AKcMinOTDim, 
    185.000000 * AKcMinOTDim,   186.000000 * AKcMinOTDim,   187.000000 * AKcMinOTDim,   188.000000 * AKcMinOTDim, 
    189.000000 * AKcMinOTDim,   190.000000 * AKcMinOTDim,   191.000000 * AKcMinOTDim,   192.000000 * AKcMinOTDim, 
    193.000000 * AKcMinOTDim,   194.000000 * AKcMinOTDim,   195.000000 * AKcMinOTDim,   196.000000 * AKcMinOTDim, 
    197.000000 * AKcMinOTDim,   198.000000 * AKcMinOTDim,   199.000000 * AKcMinOTDim,   200.000000 * AKcMinOTDim, 
    201.000000 * AKcMinOTDim,   202.000000 * AKcMinOTDim,   203.000000 * AKcMinOTDim,   204.000000 * AKcMinOTDim, 
    205.000000 * AKcMinOTDim,   206.000000 * AKcMinOTDim,   207.000000 * AKcMinOTDim,   208.000000 * AKcMinOTDim, 
    209.000000 * AKcMinOTDim,   210.000000 * AKcMinOTDim,   211.000000 * AKcMinOTDim,   212.000000 * AKcMinOTDim, 
    213.000000 * AKcMinOTDim,   214.000000 * AKcMinOTDim,   215.000000 * AKcMinOTDim,   216.000000 * AKcMinOTDim, 
    217.000000 * AKcMinOTDim,   218.000000 * AKcMinOTDim,   219.000000 * AKcMinOTDim,   220.000000 * AKcMinOTDim, 
    221.000000 * AKcMinOTDim,   222.000000 * AKcMinOTDim,   223.000000 * AKcMinOTDim,   224.000000 * AKcMinOTDim, 
    225.000000 * AKcMinOTDim,   226.000000 * AKcMinOTDim,   227.000000 * AKcMinOTDim,   228.000000 * AKcMinOTDim, 
    229.000000 * AKcMinOTDim,   230.000000 * AKcMinOTDim,   231.000000 * AKcMinOTDim,   232.000000 * AKcMinOTDim, 
    233.000000 * AKcMinOTDim,   234.000000 * AKcMinOTDim,   235.000000 * AKcMinOTDim,   236.000000 * AKcMinOTDim, 
    237.000000 * AKcMinOTDim,   238.000000 * AKcMinOTDim,   239.000000 * AKcMinOTDim,   240.000000 * AKcMinOTDim, 
    241.000000 * AKcMinOTDim,   242.000000 * AKcMinOTDim,   243.000000 * AKcMinOTDim,   244.000000 * AKcMinOTDim, 
    245.000000 * AKcMinOTDim,   246.000000 * AKcMinOTDim,   247.000000 * AKcMinOTDim,   248.000000 * AKcMinOTDim, 
    249.000000 * AKcMinOTDim,   250.000000 * AKcMinOTDim,   251.000000 * AKcMinOTDim,   252.000000 * AKcMinOTDim, 
    253.000000 * AKcMinOTDim,   254.000000 * AKcMinOTDim,   255.000000 * AKcMinOTDim,   256.000000 * AKcMinOTDim
};

#define DEBUG_COLLISION2_TRAVERSAL 0

#define AKcMaxRayCastStackElems	200

#define MAX_AKIRA_RAYS 20
static AKtRay AKgInternalRayStorage[MAX_AKIRA_RAYS * MAX_AKIRA_RAYS];

UUtError
AKrRayCastCoefficients_Initialize(UUtUns32 inDivs)
{
	if (inDivs > MAX_AKIRA_RAYS) {
		COrConsole_Printf("allocated too many akira rays");
	}
	
	inDivs = UUmMin(inDivs, MAX_AKIRA_RAYS);

	{
		UUtUns16		numTotalRays = 0;
		AKtRay*			curDestRay;
			
		UUtUns16	itr;
		UUtUns32	x,y;
		
		float		dimOffset = 1.f / ((float) inDivs);
		UUtUns32	table[4] = { 0,2,1,3 };

		
		AKgFrame.rays = NULL;
		
		UUmAssert(AKgFrame.rays == NULL);
		AKgFrame.numRays = inDivs * inDivs;
		AKgFrame.rays = AKgInternalRayStorage;
		UUmError_ReturnOnNull(AKgFrame.rays);
		
		curDestRay = AKgFrame.rays;
					
		for(x = 0; x < inDivs; x++)
		{
			for(y = 0; y < inDivs; y++)
			{
				curDestRay->u = dimOffset * ((float) x) + (dimOffset * table[y % 4] * 0.25f);
				curDestRay->v = dimOffset * ((float) y) + (dimOffset * table[x % 4] * 0.25f);
				curDestRay++;
				numTotalRays++;
			}
		}
		
		UUmAssert(numTotalRays == inDivs * inDivs);
		
		for(itr = 0; itr < AKcNumTemperalFrames; itr++)
		{
			AKgRayOffset[itr].u_offset = AKgSourceRayOffset[itr].u_offset * dimOffset;
			AKgRayOffset[itr].v_offset = AKgSourceRayOffset[itr].v_offset * dimOffset;
		}
	}
	
	return UUcError_None;
}

void
AKrRayCastCoefficients_Terminate(
	void)
{

	if(AKgFrame.rays != NULL) {
		AKgFrame.rays = NULL;
	}
	
	#if 0//UUmSIMD != UUmSIMD_None
	
		AKrRayCastCoefficients_Terminate_SIMD();
	
	#endif
}

//
// This is the current compare function.
//

typedef struct raycast_block_type
{
	// constants for all rays
	const M3tPoint3D			*pointArray;
	M3tPlaneEquation			*planeEquArray;
	AKtOctTree_InteriorNode		*interiorNodeArray;
	const AKtOctTree_LeafNode	*leafNodeArray;
	const AKtQuadTree_Node		*qtInteriorNodeArray;
	const AKtGQ_General			*gqGeneralArray;
	const AKtGQ_Collision		*gqCollisionArray;
	const UUtUns32				*octTreeGQIndices;
	UUtUns32					*ot2LeafNodeVisibility;
	UUtUns32					*gq2VisibilityVector;

	UUtUns32*					gqComputedBackFaceBV;
	UUtUns32*					gqBackFaceBV;

	UUtUns32					startNodeIndex;

	// constants per ray

	float startPointX;
	float startPointY;
	float startPointZ;

	float endPointX;
	float endPointY;
	float endPointZ;

	float vX;
	float vY;
	float vZ;
	
	float invVX;
	float invVY;
	float invVZ;

	UUtUns32 xSideFlag;
	UUtUns32 ySideFlag;
	UUtUns32 zSideFlag;


	// calculated values
	M3tPoint3D					testPoint;

	float						total_min_x;
	float						total_min_y;
	float						total_min_z;

	float						total_max_x;
	float						total_max_y;
	float						total_max_z;

	UUtUns32					hit_sky;
} raycast_block_type;

static void traverse_single_ray(raycast_block_type *in_block)
{
	UUtUns32 cur_node_index = in_block->startNodeIndex;
	float parametric_start = 0.f;
	float parametric_end = 0.f;

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Enter(AKg_RayCastOctTree_Timer_Single_Ray);
#endif

	while(1)
	{
		UUtUns32 sideCrossing;

		float xPlane;
		float yPlane;
		float zPlane;

		float minX, minY, minZ;
		float maxX, maxY, maxZ;

		UUtUns32 dim_Encode;
		float curFullDim;
		UUtUns32 length;
		UUtUns32 curGQIndIndex;
		UUtUns32 startIndirectIndex;
		UUtBool	stopChecking;
		UUtUns32 oct_tree_leaf_index = AKmOctTree_GetLeafIndex(cur_node_index);
		const AKtOctTree_LeafNode *curOTLeafNode;
		UUtBool far_clipped;

		parametric_start = parametric_end;
		
		UUmAssert(AKmOctTree_IsLeafNode(cur_node_index));

		if(0xFFFFFFFF == cur_node_index) {
			in_block->hit_sky = UUcTrue;

			break;
		}
		
		//UUmAssert(oct_tree_leaf_index < inEnvironment->octTree->leafNodeArray->numNodes);
		
		curOTLeafNode = in_block->leafNodeArray + oct_tree_leaf_index;
		
		UUr2BitVector_Set(in_block->ot2LeafNodeVisibility, oct_tree_leaf_index);
		
		//
		dim_Encode = curOTLeafNode->dim_Encode;
					
		curFullDim = (float)(1 << ((dim_Encode >> AKcOctTree_Shift_Dim) & AKcOctTree_Mask_Dim));
		curFullDim *= AKcMinOTDim;
		
		maxX = (float)((dim_Encode >> AKcOctTree_Shift_X) & AKcOctTree_Mask_X);
		maxY = (float)((dim_Encode >> AKcOctTree_Shift_Y) & AKcOctTree_Mask_Y);
		maxZ = (float)((dim_Encode >> AKcOctTree_Shift_Z) & AKcOctTree_Mask_Z);
		
		maxX -= 255.0f;
		maxY -= 255.0f;
		maxZ -= 255.0f;
		
		maxX *= AKcMinOTDim;
		maxY *= AKcMinOTDim;
		maxZ *= AKcMinOTDim;
		
		minX = maxX - curFullDim;
		minY = maxY - curFullDim;
		minZ = maxZ - curFullDim;

		// expand the total visible volume
		in_block->total_min_x = UUmMin(in_block->total_min_x, minX);
		in_block->total_min_y = UUmMin(in_block->total_min_y, minY);
		in_block->total_min_z = UUmMin(in_block->total_min_z, minZ);

		in_block->total_max_x = UUmMax(in_block->total_max_x, maxX);
		in_block->total_max_y = UUmMax(in_block->total_max_y, maxY);
		in_block->total_max_z = UUmMax(in_block->total_max_z, maxZ);


		// Find out which plane I cross
							
		// project onto XY plane
		xPlane = (in_block->xSideFlag == AKcOctTree_Side_PosX) ? maxX : minX;
		yPlane = (in_block->ySideFlag == AKcOctTree_Side_PosY) ? maxY : minY;
		zPlane = (in_block->zSideFlag == AKcOctTree_Side_PosZ) ? maxZ : minZ;
		
		// calculate sideCrossing
		{
			float parametric_distance_to_x_plane = in_block->invVX * (xPlane - in_block->startPointX);
			float parametric_distance_to_y_plane = in_block->invVY * (yPlane - in_block->startPointY);
			float parametric_distance_to_z_plane = in_block->invVZ * (zPlane - in_block->startPointZ);
			

			if ((parametric_distance_to_x_plane <= parametric_distance_to_y_plane) && (parametric_distance_to_x_plane <= parametric_distance_to_z_plane)) {
				sideCrossing = in_block->xSideFlag;
				far_clipped = parametric_distance_to_x_plane > 1.f;
				parametric_end = parametric_distance_to_x_plane;
			}
			else if (parametric_distance_to_y_plane <= parametric_distance_to_z_plane) {
				sideCrossing = in_block->ySideFlag;
				far_clipped = parametric_distance_to_y_plane > 1.f;
				parametric_end = parametric_distance_to_y_plane;
			}
			else {
				sideCrossing = in_block->zSideFlag;
				far_clipped = parametric_distance_to_z_plane > 1.f;
				parametric_end = parametric_distance_to_z_plane;
			}

			if (far_clipped) {
				parametric_end = 1.f;
			}
		}					
		
		#if defined(DEBUG_COLLISION2_TRAVERSAL) && DEBUG_COLLISION2_TRAVERSAL
			
			bBox.minPoint.x = minX;
			bBox.minPoint.y = minY;
			bBox.minPoint.z = minZ;
			bBox.maxPoint.x = maxX;
			bBox.maxPoint.y = maxY;
			bBox.maxPoint.z = maxZ;
			
			start.x = startPointX;
			start.y = startPointY;
			start.z = startPointZ;
			end.x = endPointX;
			end.y = endPointY;
			end.z = endPointZ;
			
			in = CLrBox_Line(&bBox, &start, &end);
			
			UUmAssert(in);

		#endif
		
		stopChecking = UUcFalse;
		
		if(curOTLeafNode->gqIndirectIndex_Encode == 0)
		{
			#if BRENTS_CHEESY_PROFILE && !BRENTS_CHEESY_PROFILE_ONLY_FUNC
			
				funcNumSkippedOctants++;
				
			#endif
			
			goto skip;
		}
		
		AKmOctTree_DecodeGQIndIndices(curOTLeafNode->gqIndirectIndex_Encode, startIndirectIndex, length);
		
		#if BRENTS_CHEESY_PROFILE && !BRENTS_CHEESY_PROFILE_ONLY_FUNC
			numQuadChecks = 0;
			funcQuadTraversalStart = UUrMachineTime_High();
		#endif
		
		for(curGQIndIndex = 0; curGQIndIndex < length; curGQIndIndex++)
		{
			const AKtGQ_General*				curGQGeneral;
			const AKtGQ_Collision*			curGQCollision;
			UUtBool	gotPlaneEqu;
			float a, b, c, d;
			UUtUns32 curGQIndex;

			UUtUns32 cur_gq_bv_offset;
			UUtUns32 cur_gq_bv_bit_mask;
			UUtUns32 old_uns_32;

			curGQIndex = in_block->octTreeGQIndices[curGQIndIndex + startIndirectIndex];
			
			//UUmAssert(curGQIndex < inEnvironment->gqGeneralArray->numGQs);
			
			gotPlaneEqu = UUcFalse;
			cur_gq_bv_offset = BV_INDEX(curGQIndex);
			cur_gq_bv_bit_mask = BV_BIT(curGQIndex);
			curGQGeneral = in_block->gqGeneralArray + curGQIndex;
			curGQCollision = in_block->gqCollisionArray + curGQIndex;
			
			
			//Mark this gq as visible
			old_uns_32 = in_block->gqComputedBackFaceBV[cur_gq_bv_offset];
			
			if (old_uns_32 & cur_gq_bv_bit_mask)
			{
				if (in_block->gqBackFaceBV[cur_gq_bv_offset] & cur_gq_bv_bit_mask)
				{
					continue;
				}
			}

			
			//Mark this gq as visible
			//if(UUrBitVector_TestAndSetBit(in_block->gqComputedBackFaceBV, curGQIndex))
			//{
				//if(UUrBitVector_TestBit(in_block->gqBackFaceBV, curGQIndex))
				//{
					//continue;
				//}
			//}
			else
			{
				in_block->gqComputedBackFaceBV[cur_gq_bv_offset] = old_uns_32 | cur_gq_bv_bit_mask;

				// test for backfacing
	 			if(!(curGQGeneral->flags & AKcGQ_Flag_2Sided))
	 			{
					float viewVectorX;
					float viewVectorY;
					float viewVectorZ;

					const M3tPoint3D *curPoint;

	 				gotPlaneEqu = UUcTrue;
	 				// Remove backfacing stuff
	 				AKmPlaneEqu_GetComponents(
	 					curGQCollision->planeEquIndex,
						in_block->planeEquArray,
						a, b, c, d);
					
					curPoint = in_block->pointArray + curGQGeneral->m3Quad.vertexIndices.indices[0];
	 				viewVectorX = in_block->startPointX - curPoint->x;
	 				viewVectorY = in_block->startPointY - curPoint->y;
	 				viewVectorZ = in_block->startPointZ - curPoint->z;
	 							 				
					if ((viewVectorX * a + viewVectorY * b + viewVectorZ * c) < 0.0f) {
						in_block->gqBackFaceBV[cur_gq_bv_offset] |= cur_gq_bv_bit_mask;
	 					//UUrBitVector_SetBit(in_block->gqBackFaceBV, curGQIndex);
	 					continue;
					}
	 			}
			}

			UUr2BitVector_Set(in_block->gq2VisibilityVector, curGQIndex);					

			if (curGQGeneral->flags & AKcGQ_Flag_Occlude_Skip) continue;
			
			if(!stopChecking)
			{
				#if BRENTS_CHEESY_PROFILE && !BRENTS_CHEESY_PROFILE_ONLY_FUNC
					
					numQuadChecks++;
					
				#endif
				
				if(!gotPlaneEqu) {
					AKmPlaneEqu_GetComponents(curGQCollision->planeEquIndex, in_block->planeEquArray, a, b, c, d);
				}
				
				{
					float t, num1, denom;

					denom = a * in_block->vX + b * in_block->vY + c * in_block->vZ;
					
					if(denom == 0.0f) continue;
					
					num1 = -(a * in_block->startPointX + b * in_block->startPointY + c * in_block->startPointZ + d);
					
					t = num1 / denom;
					
					if (t < parametric_start || t > parametric_end) continue;

					in_block->testPoint.x = in_block->vX * t + in_block->startPointX;
					in_block->testPoint.y = in_block->vY * t + in_block->startPointY;
					in_block->testPoint.z = in_block->vZ * t + in_block->startPointZ;
				}


				// test a single quad
				{
					UUtUns32 quad_vitr;
					UUtUns32 quad_vcount = (curGQGeneral->flags & AKcGQ_Flag_Triangle) ? 3 : 4;
					const M3tPoint3D *quad_v0;
					const M3tPoint3D *quad_v1;
					UUtBool always_positive = UUcTrue;
					UUtBool always_negative = UUcTrue;

					quad_v1 = curGQGeneral->m3Quad.vertexIndices.indices[0] + in_block->pointArray;

					for(quad_vitr = 0; quad_vitr < quad_vcount; quad_vitr++)
					{
						M3tPoint3D quad_edge_vector;
						M3tPoint3D v0_test_vector;
						M3tVector3D cross_product;
						float dot_product;

						quad_v0 = quad_v1;
						quad_v1 = curGQGeneral->m3Quad.vertexIndices.indices[(quad_vitr + 1) % quad_vcount] + in_block->pointArray;

						MUmVector_Subtract(quad_edge_vector, *quad_v1, *quad_v0);
						MUmVector_Subtract(v0_test_vector, in_block->testPoint, *quad_v0);

						//  compute the cross product
						cross_product.x = v0_test_vector.y * quad_edge_vector.z - v0_test_vector.z * quad_edge_vector.y;
						cross_product.y = v0_test_vector.z * quad_edge_vector.x - v0_test_vector.x * quad_edge_vector.z;
						cross_product.z = v0_test_vector.x * quad_edge_vector.y - v0_test_vector.y * quad_edge_vector.x;

						// compute the dot product
						dot_product = a * cross_product.x + b * cross_product.y + c * cross_product.z;

						always_positive &= dot_product >= 0;
						always_negative &= dot_product <= 0;
					}

					stopChecking = always_positive | always_negative;
				}			
			}
		}
		
		#if BRENTS_CHEESY_PROFILE && !BRENTS_CHEESY_PROFILE_ONLY_FUNC
		
			if(stopChecking)
			{
				funcHitOctant_QuadTraversalTime += UUrMachineTime_High() - funcQuadTraversalStart;
				funcHitOctant_NumQuadTraversals += length;
				funcHitOctant_NumQuadChecks += numQuadChecks;
				funcNumHitOctants++;
			}
			else
			{
				funcMissOctant_QuadTraversalTime += UUrMachineTime_High() - funcQuadTraversalStart;
				funcMissOctant_NumQuadTraversals += length;
				funcMissOctant_NumQuadChecks += numQuadChecks;
				funcNumMissOctants++;
			}
		#endif
		
	skip:

	#if OCT_TREE_TOOL_SUPPORT
		// CB: env_show_leafnodes only draws the leafnodes that rays hit
		//     env_show_traversal draws all leafnodes that rays pass through
		if((AKgDebug_ShowLeafNodes == UUcTrue) || (AKgDebug_ShowOctTree == UUcTrue))
		{
			M3tPoint3D	minPoint;
			M3tPoint3D	maxPoint;
			
			minPoint.x = minX + 0.1f;
			minPoint.y = minY + 0.1f;
			minPoint.z = minZ + 0.1f;
			maxPoint.x = maxX - 0.1f;
			maxPoint.y = maxY - 0.1f;
			maxPoint.z = maxZ - 0.1f;
			
			if(/*endNodeIndex == curNodeIndex ||*/ stopChecking)
			{
				// CB: hit leaf nodes are red
				AKiBoundingBox_Draw(
									&minPoint,
					&maxPoint,
					0xFFFF0000);
			}
			else if (AKgDebug_ShowOctTree == UUcTrue)
			{
				// CB: traversed leaf nodes are white
				AKiBoundingBox_Draw(&minPoint,
					&maxPoint,
					0xFFFFFFFF);
			}
		}
	#endif

		// If we are finished then bail
		if(stopChecking) break;

		if (far_clipped) {
			in_block->hit_sky = UUcTrue;
			break;
		}

		// Lets move to the next node
		cur_node_index = curOTLeafNode->adjInfo[sideCrossing];
		
		if(!AKmOctTree_IsLeafNode(cur_node_index))
		{
			float curCenterPointU;
			float curCenterPointV;
			float magic_temp;
			float curHalfDim;
			float uc;
			float vc;

			// need to traverse quad tree
			
			curHalfDim = curFullDim * 0.5f;
			
			// First compute the intersection point
			switch(sideCrossing)
			{
				case AKcOctTree_Side_NegX:
				case AKcOctTree_Side_PosX:
					curCenterPointU = minY + curHalfDim;
					curCenterPointV = minZ + curHalfDim;
					magic_temp = in_block->invVX * (xPlane - in_block->startPointX);
					uc = in_block->startPointY + in_block->vY * magic_temp;
					vc = in_block->startPointZ + in_block->vZ * magic_temp;
					break;
					
				case AKcOctTree_Side_NegY:
				case AKcOctTree_Side_PosY:
					curCenterPointU = minX + curHalfDim;
					curCenterPointV = minZ + curHalfDim;
					magic_temp = in_block->invVY * (yPlane - in_block->startPointY);
					uc = in_block->startPointX + in_block->vX * magic_temp;
					vc = in_block->startPointZ + in_block->vZ * magic_temp;
					break;
					
				case AKcOctTree_Side_NegZ:
				case AKcOctTree_Side_PosZ:
					curCenterPointU = minX + curHalfDim;
					curCenterPointV = minY + curHalfDim;
					magic_temp = in_block->invVZ * (zPlane - in_block->startPointZ);
					uc = in_block->startPointX + in_block->vX * magic_temp;
					vc = in_block->startPointY + in_block->vY * magic_temp;
					break;
				
				default:
					UUmAssert(0);
			}
				
			// next traverse the tree
			while(1)
			{
				const AKtQuadTree_Node*			curQTIntNode;		
				UUtUns32 curOctant;

				curQTIntNode = in_block->qtInteriorNodeArray + cur_node_index;
				
				curOctant = 0;
				if(uc > curCenterPointU) curOctant |= AKcQuadTree_SideBV_U;
				if(vc > curCenterPointV) curOctant |= AKcQuadTree_SideBV_V;
					
				cur_node_index = curQTIntNode->children[curOctant];
				
				if(AKmOctTree_IsLeafNode(cur_node_index)) break;
				curHalfDim *= 0.5f;
				curCenterPointU += (curOctant & AKcQuadTree_SideBV_U) ? curHalfDim : -curHalfDim;
				curCenterPointV += (curOctant & AKcQuadTree_SideBV_V) ? curHalfDim : -curHalfDim;
			}
		}
	}

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Exit(AKg_RayCastOctTree_Timer_Single_Ray);
#endif

	return;
}

static UUtBool
AKiEnvironment_RayCastOctTree(
	AKtEnvironment*		inEnvironment,
	M3tGeomCamera*		inCamera)
{
#if BRENTS_CHEESY_PROFILE
	UUtUns64	funcStart;
	
	#if !BRENTS_CHEESY_PROFILE_ONLY_FUNC
	
	UUtUns64	funcRayStart;
	UUtUns64	funcOctantStart;
	UUtUns64	funcQuadTraversalStart;
	
	UUtUns32	numQuadChecks;
	
	#endif
	
	UUtUns64	funcEnd;
	
#endif



	
	

	float						uv_far_initial_x, uv_far_initial_y, uv_far_initial_z;
	float						u_far_delta_x, u_far_delta_y, u_far_delta_z;
	float						v_far_delta_x, v_far_delta_y, v_far_delta_z;
	float						uv_near_initial_x, uv_near_initial_y, uv_near_initial_z;
	float						u_near_delta_x, u_near_delta_y, u_near_delta_z;
	float						v_near_delta_x, v_near_delta_y, v_near_delta_z;
	
	UUtUns32					itr, numRays;
	AKtRay*						curRay;
	AKtRayOffset				*curRayOffset = AKgRayOffset + (gFrameNum % AKcNumTemperalFrames);				
	
	AKtEnvironment_Private*	environmentPrivate = (AKtEnvironment_Private*)(TMrTemplate_PrivateData_GetDataPtr(AKgTemplate_PrivateData, inEnvironment));
	

	raycast_block_type				raycast_block;

	#if defined(DEBUG_COLLISION2_TRAVERSAL) && DEBUG_COLLISION2_TRAVERSAL
		M3tBoundingBox_MinMax	bBox;
		M3tPoint3D				start, end;
		UUtBool					in;
	#endif
	
	#if BRENTS_CHEESY_PROFILE
		funcNum++;
		funcStart = UUrMachineTime_High();
	#endif

	raycast_block.hit_sky = UUcFalse;

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Enter(AKg_RayCastOctTree_Timer);
#endif
	
	raycast_block.pointArray = inEnvironment->pointArray->points;
	raycast_block.planeEquArray = inEnvironment->planeArray->planes;
	raycast_block.interiorNodeArray = inEnvironment->octTree->interiorNodeArray->nodes;
	raycast_block.leafNodeArray = inEnvironment->octTree->leafNodeArray->nodes;
	raycast_block.qtInteriorNodeArray = inEnvironment->octTree->qtNodeArray->nodes;
	raycast_block.gqGeneralArray = inEnvironment->gqGeneralArray->gqGeneral;
	raycast_block.gqCollisionArray = inEnvironment->gqCollisionArray->gqCollision;
	raycast_block.octTreeGQIndices = inEnvironment->octTree->gqIndices->indices;
	raycast_block.ot2LeafNodeVisibility = environmentPrivate->ot2LeafNodeVisibility;
	raycast_block.gqComputedBackFaceBV = environmentPrivate->gqComputedBackFaceBV;
	raycast_block.gqBackFaceBV = environmentPrivate->gqBackFaceBV;

	raycast_block.gq2VisibilityVector = environmentPrivate->gq2VisibilityVector;
	
	//UUrMemory_Clear(environmentPrivate->gqVisibilityBV, (inEnvironment->gqArray->numGQs + 3) >> 2);
	
	UUrBitVector_ClearBitAll(
		environmentPrivate->gqComputedBackFaceBV,
		inEnvironment->gqGeneralArray->numGQs);

	UUrBitVector_ClearBitAll(
		environmentPrivate->gqBackFaceBV,
		inEnvironment->gqGeneralArray->numGQs);

	// go into the start node
	{
		M3tPoint3D					cameraLocation;

		M3rCamera_GetViewData(
			inCamera,
			&cameraLocation,
			NULL,
			NULL);


		raycast_block.startNodeIndex = AKrFindOctTreeNodeIndex(raycast_block.interiorNodeArray, cameraLocation.x, cameraLocation.y, cameraLocation.z, NULL);		

		raycast_block.total_min_x = cameraLocation.x;
		raycast_block.total_min_y = cameraLocation.y;
		raycast_block.total_min_z = cameraLocation.z;

		raycast_block.total_max_x = cameraLocation.x;
		raycast_block.total_max_y = cameraLocation.y;
		raycast_block.total_max_z = cameraLocation.z;
	}	

	// cache the interpolation bases
	{
		M3tPoint3D frustumWorldPoints[8];
		M3tPlaneEquation frustumWorldPlanes[6];

		M3rCamera_GetWorldFrustum(
			inCamera,
			frustumWorldPoints,		
			frustumWorldPlanes);

		uv_far_initial_x = frustumWorldPoints[4].x;
		uv_far_initial_y = frustumWorldPoints[4].y;
		uv_far_initial_z = frustumWorldPoints[4].z;
		
		u_far_delta_x = frustumWorldPoints[5].x - uv_far_initial_x;
		u_far_delta_y = frustumWorldPoints[5].y - uv_far_initial_y;
		u_far_delta_z = frustumWorldPoints[5].z - uv_far_initial_z;
		
		v_far_delta_x = frustumWorldPoints[6].x - uv_far_initial_x;
		v_far_delta_y = frustumWorldPoints[6].y - uv_far_initial_y;
		v_far_delta_z = frustumWorldPoints[6].z - uv_far_initial_z;
		
		uv_near_initial_x = frustumWorldPoints[0].x;
		uv_near_initial_y = frustumWorldPoints[0].y;
		uv_near_initial_z = frustumWorldPoints[0].z;
		
		u_near_delta_x = frustumWorldPoints[1].x - uv_near_initial_x;
		u_near_delta_y = frustumWorldPoints[1].y - uv_near_initial_y;
		u_near_delta_z = frustumWorldPoints[1].z - uv_near_initial_z;
		
		v_near_delta_x = frustumWorldPoints[2].x - uv_near_initial_x;
		v_near_delta_y = frustumWorldPoints[2].y - uv_near_initial_y;
		v_near_delta_z = frustumWorldPoints[2].z - uv_near_initial_z;
	}
		
	// Begin traversing all the rays
		numRays = AKgFrame.numRays;
		curRay = AKgFrame.rays;
		
	#if BRENTS_CHEESY_PROFILE && !BRENTS_CHEESY_PROFILE_ONLY_FUNC
		funcNumRays += numRays;
		funcRayStart = UUrMachineTime_High();
	#endif
	
		for(itr = 0; itr < numRays; itr++, curRay++)
		{
			float u, v;

			u = curRay->u + curRayOffset->u_offset;
			v = curRay->v + curRayOffset->v_offset;
			
			raycast_block.endPointX = uv_far_initial_x + u * u_far_delta_x + v * v_far_delta_x;
			raycast_block.endPointY = uv_far_initial_y + u * u_far_delta_y + v * v_far_delta_y;
			raycast_block.endPointZ = uv_far_initial_z + u * u_far_delta_z + v * v_far_delta_z;
			
			raycast_block.startPointX = uv_near_initial_x + u * u_near_delta_x + v * v_near_delta_x;
			raycast_block.startPointY = uv_near_initial_y + u * u_near_delta_y + v * v_near_delta_y;
			raycast_block.startPointZ = uv_near_initial_z + u * u_near_delta_z + v * v_near_delta_z;
			
			//numGQsChecked = 0;
#if OCT_TREE_TOOL_SUPPORT
			raycast_block.testPoint.x = raycast_block.endPointX;
			raycast_block.testPoint.y = raycast_block.endPointY;
			raycast_block.testPoint.z = raycast_block.endPointZ;
#endif
			
			raycast_block.vX = raycast_block.endPointX - raycast_block.startPointX;
			raycast_block.vY = raycast_block.endPointY - raycast_block.startPointY;
			raycast_block.vZ = raycast_block.endPointZ - raycast_block.startPointZ;
			
			//UUmAssert(vX != 0.0f);
			//UUmAssert(vY != 0.0f);
			//UUmAssert(vZ != 0.0f);
			
			raycast_block.invVX = 1.0f / raycast_block.vX;
			raycast_block.invVY = 1.0f / raycast_block.vY;
			raycast_block.invVZ = 1.0f / raycast_block.vZ;
			
			// Compute the side flags
			raycast_block.xSideFlag = AKcOctTree_Side_NegX + (raycast_block.vX > 0.0f);
			raycast_block.ySideFlag = AKcOctTree_Side_NegY + (raycast_block.vY > 0.0f);
			raycast_block.zSideFlag = AKcOctTree_Side_NegZ + (raycast_block.vZ > 0.0f);
						
			#if BRENTS_CHEESY_PROFILE && !BRENTS_CHEESY_PROFILE_ONLY_FUNC
				funcOctantStart = UUrMachineTime_High();
			#endif
			
			traverse_single_ray(&raycast_block);
			
			#if BRENTS_CHEESY_PROFILE && !BRENTS_CHEESY_PROFILE_ONLY_FUNC
				funcOctantTime += UUrMachineTime_High() - funcOctantStart;
			#endif
			
#if OCT_TREE_TOOL_SUPPORT
			if(AKgDebug_ShowRays)
			{
				M3tPoint3D	points[2];
				UUtUns16	colorLine;
				
				points[0].x = raycast_block.startPointX + 0.01f;
				points[0].y = raycast_block.startPointY + 0.01f;
				points[0].z = raycast_block.startPointZ + 0.01f;
				
				points[1] = raycast_block.testPoint;
				
				switch(gFrameNum % AKcNumTemperalFrames)
				{
					case 0:
						colorLine = 0xff0000;
						break;
					case 1:
						colorLine = 0x00ff00;
						break;
					case 2:
						colorLine = 0xff;
						break;
						
				}
				
				M3rGeom_Line_Light(
					&points[0],
					&points[1],
					0xFFFFFFFF);
			}
#endif
		}

	#if BRENTS_CHEESY_PROFILE
		funcEnd = UUrMachineTime_High();
		
		#if !BRENTS_CHEESY_PROFILE_ONLY_FUNC
			funcRayTime += funcEnd - funcRayStart;
		#endif
		
		funcTime += funcEnd - funcStart;
		
	#endif

	inEnvironment->visible_bbox.minPoint.x = raycast_block.total_min_x; 
	inEnvironment->visible_bbox.minPoint.y = raycast_block.total_min_y; 
	inEnvironment->visible_bbox.minPoint.z = raycast_block.total_min_z; 

	inEnvironment->visible_bbox.maxPoint.x = raycast_block.total_max_x; 
	inEnvironment->visible_bbox.maxPoint.y = raycast_block.total_max_y; 
	inEnvironment->visible_bbox.maxPoint.z = raycast_block.total_max_z; 

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Exit(AKg_RayCastOctTree_Timer);
#endif

	return (UUtBool) raycast_block.hit_sky;
}

static void AgeEnvironment(AKtEnvironment *inEnvironment)
{
	AKtEnvironment_Private*	environmentPrivate = (AKtEnvironment_Private*)(TMrTemplate_PrivateData_GetDataPtr(AKgTemplate_PrivateData, inEnvironment));

	// CB: removing environment aging for debugging test - 12/2/2000
	UUr2BitVector_Decrement(environmentPrivate->gq2VisibilityVector, inEnvironment->gqGeneralArray->numGQs);
	UUr2BitVector_Decrement(environmentPrivate->ot2LeafNodeVisibility, inEnvironment->octTree->leafNodeArray->numNodes);

	return;
}

static UUtUns32 AKgNumEqualFrames = 0;

extern void OBJrTrigger_Dirty(void); // OT_Trigger.c
void AKrEnvironment_GunkChanged(void)
{
	// If we change the way gunk on the level
	// occludes then we need to make sure that
	// we are recaculating the visibility

	AKgNumEqualFrames = 0;

	OBJrTrigger_Dirty();

	return;
}

UUtError
AKrEnvironment_StartFrame(
	AKtEnvironment*	inEnvironment,
	M3tGeomCamera*	inCamera,
	UUtBool			*outRenderSky)
{
	UUtError				error;
	AKtEnvironment_Private*	environmentPrivate = (AKtEnvironment_Private*)(TMrTemplate_PrivateData_GetDataPtr(AKgTemplate_PrivateData, inEnvironment));
	UUtUns32				debugState;
	
	//return UUcError_None;
	
	debugState = M3rGeom_State_Get(M3cGeomStateIntType_DebugMode);
	
	if(AKgDrawGhostGQs)
	{
		debugState |= M3cGeomState_DebugMode_DrawGhostGQs;
	}
	else
	{
		debugState &= ~M3cGeomState_DebugMode_DrawGhostGQs;
	}
	
	if(AKgDebug_DebugMaps)
	{
		debugState |= M3cGeomState_DebugMode_UseEnvDbgTexture;
	}
	else
	{
		debugState &= ~M3cGeomState_DebugMode_UseEnvDbgTexture;
	}
	
	M3rGeom_State_Set(M3cGeomStateIntType_DebugMode, debugState);
		
	if(AKgNoneTexture == NULL) {
		TMrInstance_GetDataPtr(M3cTemplate_TextureMap, "NONE", (void **) (&AKgNoneTexture));
	}
		
	error = M3rEnv_SetCamera(inCamera);
	UUmError_ReturnOnError(error);
		
	{
		UUtBool is_equal;
		UUtBool cast_rays = UUcTrue;
		static M3tPoint3D oldCameraLocation;
		static M3tPoint3D oldCameraViewVector;
		static M3tPoint3D oldCameraUpVector;

		M3tPoint3D newCameraLocation;
		M3tPoint3D newCameraViewVector;
		M3tPoint3D newCameraUpVector;

		M3rCamera_GetViewData(inCamera, &newCameraLocation, &newCameraViewVector, &newCameraUpVector);

		is_equal = 
			(MUmVector_GetDistanceSquared(newCameraLocation, oldCameraLocation) < UUmSQR(0.1f)) &&
			(MUmVector_GetDistanceSquared(newCameraViewVector, oldCameraViewVector) < UUmSQR(0.05f)) &&
			(MUmVector_GetDistanceSquared(newCameraUpVector, oldCameraUpVector) < UUmSQR(0.05f));

		if (is_equal) {
			AKgNumEqualFrames += 1;

			if (AKgNumEqualFrames > AKcNumTemperalFrames) {
				cast_rays = UUcFalse;
			}
		}
		else {
			AKgNumEqualFrames = 0;
		}
					
		if (cast_rays) {
			static UUtUns32 slow_aging = 0;
			UUtBool sky_is_visible;
			UUtInt32 ray_cast_itr;
			UUtInt32 ray_cast_count = ONrMotoko_GraphicsQuality_RayCastCount();

			for(ray_cast_itr = 0; ray_cast_itr < ray_cast_count; ray_cast_itr++)
			{
				// S.S. slow_aging = (slow_aging + 1) % (AKcNumTemperalFrames / 2);
				slow_aging= (slow_aging + 1) % (AKcNumTemperalFrames >> 1); // temporal?

				if (0 == slow_aging) {
					AgeEnvironment(inEnvironment);
				}
			}

			if (environmentPrivate->sky_visibility > 0) {
				environmentPrivate->sky_visibility--;
			}

			sky_is_visible = UUcFalse;
			
			{

				for(ray_cast_itr = 0; ray_cast_itr < ray_cast_count; ray_cast_itr++)
				{
					sky_is_visible |= AKiEnvironment_RayCastOctTree(inEnvironment, inCamera);
					gFrameNum++;
				}
			}


			if (sky_is_visible) {
				environmentPrivate->sky_visibility = AKcNumTemperalFrames;
			}
			
			oldCameraLocation = newCameraLocation;
			oldCameraViewVector = newCameraViewVector;
			oldCameraUpVector = newCameraUpVector;
		}
		else  {
			// COrConsole_Printf("environment is still %d", ONgGameState->gameTime);
		}
	}
			
	if(AKgDebug_ShowOctTree == UUcTrue)
	{
//		AKiOctTree_Show(inEnvironment);
	}


	*outRenderSky = (environmentPrivate->sky_visibility > 0);
	*outRenderSky = 1;

	return UUcError_None;
}

AKtGQ_Render*			AKgGQRenderArray;

//
// AKrEnvironment_VisibleSort
//
// A special case sort routine to put the environment GQs in polygon order.
//
// Shortly after I started writing a funky hash table bin sort thing I
// discovered something wonderful. We're sorting on texture ID numbers,
// which have a limited range, perfect material for a bin sort. 
//
// The algorithm makes multiple passes on the visible GQ array. In the
// first pass it counts the number of times each texture is used.
// In the second pass it constructs a table of offsets, one offset for
// each texture bin, into what will be the final visible GQ array. 
// In the third pass it uses those offsets to populate a the final
// visible GQ array.
//
// Disadantages over quicksort: More memory use is needed.
// Advantages: We're running about 30 times faster :)
//
// Note that this subroutine assumes we're running with vertex lighting 
// only. 
//

#define MAX_TEXTURES 0x3780
#define MAX_BINS 0x1000

//
// Note to Bungie, I'm grabbing the memory I need for this from 
// psx2_stack_allocate. The memory should probably be declared inside 
// AKrEvnvironment_VisibleSort, but I'm trying to keep my stack space under
// 8k so I can run the game with the stack on the scratch pad.
//

// I am moving this off the stack so I can build on the Mac. -S.S.
UUtUns16 *AKgCountTable= NULL; //[MAX_BINS];
UUtUns32 *AKgRemapTable= NULL; //[MAX_TEXTURES];

#if (UUmPlatform == UUmPlatform_Win32)
static void __cdecl UUcExternal_Call AKgFreeMemory(
#else
static void UUcExternal_Call AKgFreeMemory(
#endif
	void)
{
	if (AKgCountTable)
	{
		UUrMemory_Block_Delete(AKgCountTable);
		AKgCountTable= NULL;
	}
	if (AKgRemapTable)
	{
		UUrMemory_Block_Delete(AKgRemapTable);
		AKgRemapTable= NULL;
	}
	
	return;
}

static void AKrEnvironment_VisibleSort(UUtUns32 *visibleArray, UUtUns32 numVisible)
{
	int i, offset;
	UUtUns32 *ptr;
	UUtUns32 *ptr_end;

//#if UUmPlatform==UUmPlatform_SonyPSX2
//	AKgCountTable=(UUtUns16 *) psx2_stack_allocate(MAX_BINS*sizeof(UUtUns16));
//	AKgRemapTable=(UUtUns32 *) psx2_stack_allocate(MAX_TEXTURES*sizeof(UUtUns32));
//#else
	if (AKgCountTable == NULL)
	{
		AKgCountTable= (UUtUns16 *)UUrMemory_Block_New(MAX_BINS * sizeof(UUtUns16));
		AKgRemapTable= (UUtUns32 *)UUrMemory_Block_New(MAX_TEXTURES * sizeof(UUtUns32));
		atexit(AKgFreeMemory);
	}
//#endif

	UUmAssert(numVisible < MAX_TEXTURES);

	// clear the bin sort table.

	UUrMemory_Clear(AKgCountTable, sizeof(UUtUns16) * MAX_BINS);

	// First pass, we count the number of entries in each bin by going
	// through the GQs and updating the appropriate bin.

	for (ptr=visibleArray, ptr_end=visibleArray+numVisible; ptr<ptr_end;ptr++) {
		AKtGQ_Render *gq_a = AKgGQRenderArray + *ptr;
		UUtUns32 a_value = (gq_a->textureMapIndex);
		UUmAssert(a_value < MAX_BINS);

		AKgCountTable[a_value]++;
	}

	// Second pass, create a table of offsets. We're going to use these
	// to figure out where each GQ goes in the new list. 

	offset=0;

	for (i=0;i<MAX_BINS;++i) {
		int toffset=AKgCountTable[i]+offset;
		AKgCountTable[i]=offset;
		offset=toffset;
	}

	// Third pass, use the offsets to make the make a new list. Note that
	// each time a new entry is inserted the offset is incremented to
	// reflect the next location of a GQ that falls into that bin.

	for (ptr=visibleArray, ptr_end=visibleArray+numVisible; ptr<ptr_end;ptr++) {
		AKtGQ_Render *gq_a = AKgGQRenderArray + *ptr;
		UUtUns32 a_value = (gq_a->textureMapIndex);
		int remap_to=AKgCountTable[a_value]++;
		AKgRemapTable[remap_to]=*ptr;
	}

	// Finally, copy it all over.
	UUrMemory_MoveFast(AKgRemapTable, visibleArray, sizeof(UUtUns32)*numVisible);

//#if UUmPlatform==UUmPlatform_SonyPSX2
//	psx2_stack_pop();
//	psx2_stack_pop();
//#endif

	return;
}


static void
AKiEnvironment_ComputeVis(
	AKtEnvironment*	inEnvironment)
{
	AKtEnvironment_Private*	environmentPrivate = (AKtEnvironment_Private*)(TMrTemplate_PrivateData_GetDataPtr(AKgTemplate_PrivateData, inEnvironment));

	AKtGQ_General*			gqGeneralArray;
	AKtGQ_General*			curGQGeneral;
	
	UUtUns32				numGQs;

	UUtUns16				curByteIndex;
	UUtUns8*				curByte;
	UUtUns8					curByteValue;
	UUtUns8					cur2BitMask;
	UUtUns32				curGQIndex;
	UUtUns16				cur2BitIndex;
	UUtUns32				numVisible = 0;

	UUtBool					drawGhostGQs = UUcFalse;
	UUtUns32				debugState;
	UUtUns32*				visibleArray;
	UUtUns32				exceeded_max_visible_gqs_count = 0;
	
	numGQs			= inEnvironment->gqGeneralArray->numGQs;
	
	gqGeneralArray		= inEnvironment->gqGeneralArray->gqGeneral;
	AKgGQRenderArray	= inEnvironment->gqRenderArray->gqRender;

	debugState		= M3rGeom_State_Get(M3cGeomStateIntType_DebugMode);
	drawGhostGQs	= (UUtBool)((debugState & M3cGeomState_DebugMode_DrawGhostGQs) != 0);
	
	visibleArray = environmentPrivate->visGQ_List;
	
	// Traverse the bit vector
	for(curByteIndex = 0, curByte = (UUtUns8*)environmentPrivate->gq2VisibilityVector, curGQIndex = 0;
		curByteIndex < (numGQs + 3) >> 2;
		curByteIndex++, curByte++)
	{
		curByteValue = *curByte;
		
		if(curByteValue == 0)
		{
			curGQIndex += 4;
			continue;
		}
		
		for(cur2BitIndex = 0, cur2BitMask = 3;
			(cur2BitIndex < 4) && (curGQIndex < numGQs);
			cur2BitIndex++, curGQIndex++, cur2BitMask <<= 2)
		{
			curGQGeneral = gqGeneralArray + curGQIndex;
			
			if (drawGhostGQs && (curGQGeneral->flags & AKcGQ_Flag_NoTextureMask)) goto forceDraw;
			
			if (!(curByteValue & cur2BitMask)) continue;

			if (curGQGeneral->flags & AKcGQ_Flag_NoTextureMask) continue;
			
forceDraw:
#ifdef ROCKSTAR
			if((curGQGeneral->flags & AKcGQ_Flag_Invisible)&&(curGQGeneral->flags & AKcGQ_Flag_Furniture))
			{
				if(curGQGeneral->flags & AKcGQ_Flag_kevlar)
				{
					kevlar_addtodrawlist(curGQGeneral->object_tag,0);
				}
			}
#endif

#if OCT_TREE_TOOL_SUPPORT
			if (AKgDraw_ObjectCollision & AKgDraw_CharacterCollision) {
				if ((curGQGeneral->flags & AKcGQ_Flag_Chr_Col_Skip) && (curGQGeneral->flags & AKcGQ_Flag_Obj_Col_Skip)) continue;
			}
			else if (AKgDraw_ObjectCollision) {
				if (curGQGeneral->flags & AKcGQ_Flag_Obj_Col_Skip) continue;
			}
			else if (AKgDraw_CharacterCollision) {
				if (curGQGeneral->flags & AKcGQ_Flag_Chr_Col_Skip) continue;
			}
			else {
				if ((!AKgDraw_Invis) && (curGQGeneral->flags & (AKcGQ_Flag_Invisible | AKcGQ_Flag_BrokenGlass))) continue;
				if ((AKgDraw_Occl) && (curGQGeneral->flags & AKcGQ_Flag_No_Occlusion)) continue;
				if ((AKgDraw_SoundOccl) && (curGQGeneral->flags & AKcGQ_Flag_SoundTransparent)) continue;
			}
#else
			if (curGQGeneral->flags & (AKcGQ_Flag_Invisible | AKcGQ_Flag_BrokenGlass)) {
				continue;
			}
#endif
			
			if (numVisible < AKcMaxVisibleGQs) {
				visibleArray[numVisible++] = curGQIndex;
			}
			else {
				exceeded_max_visible_gqs_count += 1;
			}
		}
	}
	
	AKrEnvironment_VisibleSort(visibleArray, numVisible);

	environmentPrivate->visGQ_Num = numVisible;

	if (exceeded_max_visible_gqs_count > 0) {
		COrConsole_Printf("Exceeded max visible GQs %d", exceeded_max_visible_gqs_count);
	}

	return;
}

UUtUns32 AKrEnvironment_GetVisCount(AKtEnvironment*	inEnvironment)
{
	AKtEnvironment_Private*	environmentPrivate = (AKtEnvironment_Private*)(TMrTemplate_PrivateData_GetDataPtr(AKgTemplate_PrivateData, inEnvironment));

	return environmentPrivate->visGQ_Num;
}

UUtUns32 *AKrEnvironment_GetVisVector(AKtEnvironment* inEnvironment)
{
	AKtEnvironment_Private*	environmentPrivate = (AKtEnvironment_Private*)(TMrTemplate_PrivateData_GetDataPtr(AKgTemplate_PrivateData, inEnvironment));

	return environmentPrivate->gq2VisibilityVector;
}

void AKrEnvironment_EndFrame_Tool(AKtEnvironment *inEnvironment)
{
	AKtEnvironment_Private*	environmentPrivate = (AKtEnvironment_Private*)(TMrTemplate_PrivateData_GetDataPtr(AKgTemplate_PrivateData, inEnvironment));
	AKtOctTree_LeafNode*	leafNodeArray;
	AKtOctTree_LeafNode*	curLeafNode;
	UUtUns32				itr;
	UUtUns32				curLeafIndex;
	float					minX, minY, minZ;
	float					maxX, maxY, maxZ;
	UUtUns32				dim_Encode;
	M3tPoint3D				minPoint, maxPoint;
	float					curFullDim;

	leafNodeArray = inEnvironment->octTree->leafNodeArray->nodes;
	
	if (AKgDebugLeafNodes != NULL)
	{
		for(itr = 0; itr < AKgDebugLeafNodes->numPages; itr++)
		{
			curLeafIndex = AKgDebugLeafNodes->pages[itr];
			curLeafNode = leafNodeArray + curLeafIndex;
			
			dim_Encode = curLeafNode->dim_Encode;
			curFullDim = AKgIntToFloatDim[(dim_Encode >> AKcOctTree_Shift_Dim) & AKcOctTree_Mask_Dim];
			maxX = AKgIntToFloatXYZ[(dim_Encode >> AKcOctTree_Shift_X) & AKcOctTree_Mask_X];
			maxY = AKgIntToFloatXYZ[(dim_Encode >> AKcOctTree_Shift_Y) & AKcOctTree_Mask_Y];
			maxZ = AKgIntToFloatXYZ[(dim_Encode >> AKcOctTree_Shift_Z) & AKcOctTree_Mask_Z];
			minX = maxX - curFullDim;
			minY = maxY - curFullDim;
			minZ = maxZ - curFullDim;
		
			minPoint.x = minX;
			minPoint.y = minY;
			minPoint.z = minZ;
			maxPoint.x = maxX;
			maxPoint.y = maxY;
			maxPoint.z = maxZ;
			
			AKiBoundingBox_Draw(
				&minPoint,
				&maxPoint,
				0xFF00);

		}

		AUrDict_Clear(AKgDebugLeafNodes);
	}
	
	for(itr = 0; itr < AKgNumColDebugEntries; itr++)
	{
		M3rGeom_State_Push();
			M3rGeom_State_Set(M3cGeomStateIntType_Fill, M3cGeomState_Fill_Line);
			M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor,IMcShade_White);
			M3rGeom_State_Set(M3cGeomStateIntType_Appearance, M3cGeomState_Appearance_Gouraud);
	
			ONrDrawSphere(
				NULL, 
				AKgDebugEntry_Sphere[itr].radius,
				&AKgDebugEntry_Sphere[itr].center);
		M3rGeom_State_Pop();
	}

	AKgNumColDebugEntries = 0;
}

UUtError
AKrEnvironment_EndFrame(
	AKtEnvironment*	inEnvironment)
{
	AKtEnvironment_Private*	environmentPrivate = (AKtEnvironment_Private*)(TMrTemplate_PrivateData_GetDataPtr(AKgTemplate_PrivateData, inEnvironment));
	UUtError				error;

	gFrameNum++;
	
	M3rGeom_State_Push();
	
	M3rGeom_State_Set(
		M3cGeomStateIntType_Alpha,
		0xFF);
		
	M3rGeom_State_Commit();
	
#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Enter(AKg_ComputeVis_Timer);
#endif

	AKiEnvironment_ComputeVis(inEnvironment);

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Exit(AKg_ComputeVis_Timer);
#endif
	
	error = M3rEnv_DrawGQList(environmentPrivate->visGQ_Num, environmentPrivate->visGQ_List);
	UUmError_ReturnOnError(error);

#if OCT_TREE_TOOL_SUPPORT
	AKrEnvironment_EndFrame_Tool(inEnvironment);
#endif

	M3rGeom_State_Pop();

	return UUcError_None;
}

void AKrEnvironment_PrintStats(void)
{
#if SUPPORT_DEBUG_FILES
	BFtDebugFile *statsFile = BFrDebugFile_Open("raycastStats.txt");
		
	BFrDebugFile_Printf(statsFile, "time per func: %f"UUmNL, (float)funcTime / (float)funcNum);
	
	#if !BRENTS_CHEESY_PROFILE_ONLY_FUNC
		BFrDebugFile_Printf(statsFile, "time per raycast: %f"UUmNL, (float)funcRayTime / (float)funcNumRays);
		
		BFrDebugFile_Printf(statsFile, "time per total octant: %f"UUmNL, (float)funcOctantTime / (float)(funcNumMissOctants + funcNumHitOctants + funcNumSkippedOctants));
		BFrDebugFile_Printf(statsFile, "skipped / total octants: %f"UUmNL, (float)funcNumSkippedOctants / (float)(funcNumMissOctants + funcNumHitOctants + funcNumSkippedOctants));
		
		BFrDebugFile_Printf(statsFile, "missed octant:"UUmNL);
			BFrDebugFile_Printf(statsFile, "\ttarversals per: %f"UUmNL, (float)funcMissOctant_NumQuadTraversals / (float)funcNumMissOctants);
			BFrDebugFile_Printf(statsFile, "\ttime per quadTraversal: %f"UUmNL, (float)funcMissOctant_QuadTraversalTime / (float)funcMissOctant_NumQuadTraversals);
			BFrDebugFile_Printf(statsFile, "\tchecks / traversal ratio: %f"UUmNL, (float)funcMissOctant_NumQuadChecks / (float)funcMissOctant_NumQuadTraversals);
		
		BFrDebugFile_Printf(statsFile, "hit octant:"UUmNL);
			BFrDebugFile_Printf(statsFile, "\ttarversals per: %f"UUmNL, (float)funcHitOctant_NumQuadTraversals / (float)funcNumHitOctants);
			BFrDebugFile_Printf(statsFile, "\ttime per quadTraversal: %f"UUmNL, (float)funcHitOctant_QuadTraversalTime / (float)funcHitOctant_NumQuadTraversals);
			BFrDebugFile_Printf(statsFile, "\tchecks / traversal ratio: %f"UUmNL, (float)funcHitOctant_NumQuadChecks / (float)funcHitOctant_NumQuadTraversals);
			BFrDebugFile_Printf(statsFile, "\tquads till hit: %f"UUmNL, (float)funcHitOctant_NumQuadsTillHit / (float)funcNumHitOctants);
		

		BFrDebugFile_Printf(statsFile, "raycasts per func: %f"UUmNL, (float)funcNumRays / (float)funcNum);
		BFrDebugFile_Printf(statsFile, "total octants per raycast: %f"UUmNL, (float)(funcNumMissOctants + funcNumHitOctants + funcNumSkippedOctants) / (float)funcNumRays);
		BFrDebugFile_Printf(statsFile, "missed octants per raycast: %f"UUmNL, (float)funcNumMissOctants / (float)funcNumRays);
		BFrDebugFile_Printf(statsFile, "hit octants per raycast: %f"UUmNL, (float)funcNumHitOctants / (float)funcNumRays);
		BFrDebugFile_Printf(statsFile, "skipped octants per raycast: %f"UUmNL, (float)funcNumSkippedOctants / (float)funcNumRays);
		
		BFrDebugFile_Printf(statsFile, "quadChecks per raycast: %f"UUmNL, (float)(funcMissOctant_NumQuadTraversals + funcHitOctant_NumQuadTraversals) / (float)funcNumRays);
		
	#endif
	
	BFrDebugFile_Close(statsFile);

	return;
#endif
}

