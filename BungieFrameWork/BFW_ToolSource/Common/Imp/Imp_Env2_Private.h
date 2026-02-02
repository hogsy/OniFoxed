/*
	FILE:	Imp_Env_Private.h

	AUTHOR:	Brent H. Pease

	CREATED: Sept 3, 1997

	PURPOSE:

	Copyright 1997-1999

*/
#ifndef IMP_ENV_PRIVATE_H
#define IMP_ENV_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "BFW.h"
#include "BFW_Akira.h"
#include "BFW_AppUtilities.h"
#include "BFW_Collision.h"
#include "BFW_LSSolution.h"
#include "BFW_Particle2.h"
#include "Oni_AI_Setup.h"
#include "EnvFileFormat.h"
#include "Imp_TextureFlags.h"

#define IMPcGunkFlagNamePrefix		"flag_"
#define IMPcGunkObjectNamePrefix	"object_"
#define IMPcGunkDoorNameInfix		"door_"
#define IMPcParticle2Prefix			"p2_"
#define IMPcEnvParticleMarkerPrefix	"envp_"
#define IMPcFXPrefix				"fx_"

#define IMPcEnv_MaxFXSetups			(256)

#define IMPcEnv_MaxEnvParticles		(1024)

#define IMPcSATProjection 1.0f

#define IMPcEnv_SATTolerence		(2.0f)

#define IMPcEnv_MinBBoxVolume		(5000)

#define IMPcEnv_OT_MinGQsPerNode	(20)

#define IMPcEnv_OT_MaxGQsPerNode	(2048)
#define IMPcEnv_OT_MaxBNVsPerNode	(256)
#define IMPcEnv_OT_MaxOctTreeDepth	(14)

#define IMPcEnv_OT_NumLeafNodeDims	(9)

#define IMPcEnv_MaxNumDoors			(256)

#define IMPcEnv_MaxBNVQuads			(128)

#define IMPcEnv_MaxNumBNVBSPNodes	(1024)

#define IMPcEnv_MaxBNVSides			(128)

#define IMPcEnv_MaxNameLength		(64)

#define IMPcEnv_MaxQuadsPerSide		(32)
#define IMPcEnv_MaxAdjacencies		(32)

//#define IMPcEnv_MaxBNVs				(500)
//#define IMPcEnv_MaxGQs				(20000)

#define IMPcEnv_MaxLSs				(16)

#define IMPcEnv_MaxFlagPoints		(32)

#define	IMPcEnv_MaxInteriorNodes	(1024 * 20)
//#define	IMPcEnv_MaxLeafNodes		(0x8000)
#define	IMPcEnv_MaxLeafNodes		(64 * 1024)		// CB: increased, I hope this doesn't break 15-bit indices anywhere
#define	IMPcEnv_MaxGQIndexArray		(250 * 1024)
#define	IMPcEnv_MaxBNVIndexArray	(64 * 1024)
#define IMPcEnv_MaxQTNodes			(0x8000)

#define IMPcEnv_ErrorLogFile		"enverrors.txt"

#define IMPcEnv_MaxRemapPoints		(2048 * 4)

#define IMPcEnv_MaxFilesInDir		(512 * 1)

#define IMPcEnv_MaxGQsPerBNV		(1024 * 16)

#define IMPcEnv_MaxTextures			(512 * 4)

#define IMPcEnv_MaxMarkers			(512)

//#define IMPcLightMap_SuperSize		(32)
#define IMPcEnv_DefaultVertexLighting_Green 0xff00ff00
#define IMPcEnv_DefaultLightmapLighting_Green 0xff00ff00
#define IMPcEnv_DefaultVertexLighting_Normal 0xffcfcfcf
#define IMPcEnv_DefaultLightmapLighting_Normal 0xffcfcfcf
#define IMPcEnv_MinLighting 0x3f


#define IMPcEnv_MaxGQsOnLSPlane		(128)
#define IMPcEnv_MaxFlags			(256)
#define IMPcEnv_MaxObjects			(256)
#define IMPcEnv_MaxMarkerNodes		(1024)

#define IMPcEnv_MaxPrefixLength		(128)
#define IMPcEnv_MaxPiecesLists		(3)
#define IMPcEnv_MaxDefTextures		(5)
#define IMPcEnv_MaxDefGeometry		(5)

#define	IMPcEnv_MaxPatches			(5000)

#define IMPcEnv_MaxLMDim	62

//#define IMPcEnv_MaxTexturePages	(1024 * 2)
#define IMPcEnv_MaxGQsPerPage	(1024 * 5)

//#define IMPcMaxEnvDim			(1024.0f * 4.0f)

#define IMPcEnv_MaxObjectFX	(8)

#define IMPcEnv_MaxObjectParticles	(32)

#define	IMPcEnv_MaxNumBSPNodes			(10 * 1024)
#define IMPcEnv_MaxNewAlphaBSPPlanes	(12 * 1024)
#define IMPcEnv_MaxAlphaQuads			(12 * 1024)
#define IMPcEnv_MaxNumAlphaBSPGQs		(12 * 1024)

enum
{
	IMPcGQ_LM_None		= 0,
	IMPcGQ_LM_ForceOn	= (1 << 0),
	IMPcGQ_LM_ForceOff	= (1 << 1)

};

/*
 * BSP relates structs
 */
	typedef enum IMPtEnv_BSP_AbsoluteSideFlag
	{
		IMPcAbsSide_Postive,
		IMPcAbsSide_Negative,
		IMPcAbsSide_BothPosNeg

	} IMPtEnv_BSP_AbsoluteSideFlag;

	typedef struct IMPtEnv_BSP_Quad
	{
		UUtUns32	ref;		// user field
		UUtUns32	bspQuadIndex;

	} IMPtEnv_BSP_Quad;

	#define IMPcEnv_MaxQuadsPerPlane	(2048)

	typedef struct IMPtEnv_BSP_Plane
	{
		UUtUns32			planeEquIndex;
		UUtUns32			ref;			// user field

		UUtUns32			numQuads;
		IMPtEnv_BSP_Quad	quads[IMPcEnv_MaxQuadsPerPlane];

	} IMPtEnv_BSP_Plane;

	typedef struct IMPtEnv_BSP_Node
	{
		IMPtEnv_BSP_Plane*	splittingPlane;			// user data
		IMPtEnv_BSP_Quad	splittingQuad;

		UUtUns32			posNodeIndex;
		UUtUns32			negNodeIndex;

	} IMPtEnv_BSP_Node;

/*
 * Lightscape related stuff
 */
	typedef enum IMPtLS_LightType
	{
		IMPcLS_LightType_Point,
		IMPcLS_LightType_Linear,
		IMPcLS_LightType_Area

	} IMPtLS_LightType;

	typedef enum IMPtLS_LightParam
	{
		IMPcLS_LightParam_None			= 0,
		IMPcLS_LightParam_NotOccluding	= (1 << 0),
		IMPcLS_LightParam_Window		= (1 << 1),
		IMPcLS_LightParam_NotReflecting	= (1 << 2)

	} IMPtLS_LightParam;

	typedef enum IMPtLS_Distribution
	{
		IMPcLS_Distribution_Diffuse	= 0,
		IMPcLS_Distribution_Spot	= 1

	} IMPtLS_Distribution;

/*
 * GQ related structs
 */
	typedef struct IMPtEnv_GQ
	{
		M3tPoint3D			*hi1;	// \
		M3tPoint3D			*hi2;	//  | - Don't move these. VTuned
		M3tPoint3D			*lo1;	//  |
		M3tPoint3D			*lo2;	// /

		UUtUns32			shade[4];

		char				fileName[IMPcEnv_MaxNameLength];
		char				objName[IMPcEnv_MaxNameLength];
		char				materialName[IMPcEnv_MaxNameLength];
		char				gqMaterialName[IMPcEnv_MaxNameLength];

		UUtUns32			fileRelativeGQIndex;

		UUtUns32			scriptID;

		M3tQuad		visibleQuad;
		M3tQuad		baseMapIndices;		// Indices into the uv array for the base map

		M3tQuad		edgeIndices;
//		UUtUns32			adjGQIndices[4];

		UUtUns32			flags;
		UUtUns32			lmFlags;

		UUtUns32			planeEquIndex;

		CLtQuadProjection	projection;

		UUtUns16			textureMapIndex;

		UUtUns32			stairBNVIndex;		// CB: only valid for SAT quads

		UUtBool				used;
		UUtBool				ghostUsed;

		//UUtUns32			myPatchIndex;
		//UUtUns32			myPlaneIndex;

		UUtBool				isLuminaire;
		float				filterColor[3];
		IMPtLS_LightType	lightType;
		UUtUns32			intensity;
		IMPtLS_Distribution	distribution;
		float				beamAngle;
		float				fieldAngle;

		UUtUns32			lightParams;

		float				patchArea;

		M3tBoundingBox_MinMax	bBox;

		UUtUns32			object_tag;
	} IMPtEnv_GQ;

/*
 * BNV related structs
 */
	typedef struct IMPtEnv_BNV_Side
	{
		UUtUns32	planeEquIndex;

		UUtUns32	numBNVQuads;
		UUtUns32	bnvQuadList[IMPcEnv_MaxQuadsPerSide];

		UUtUns32	numGQGhostIndices;
		UUtUns32	gqGhostIndices[IMPcEnv_MaxQuadsPerSide];

		UUtUns32	numAdjacencies;
		AKtAdjacency adjacencyList[IMPcEnv_MaxAdjacencies];

	}  IMPtEnv_BNV_Side;

	typedef struct IMPtEnv_BNV
	{
		IMPtEnv_GQ			*sats[2];		// Don't move this, VTuned
											// down then up

		M3tPoint3D			sat_points[2][2];		// down 0, 1 then up 0, 1 - oriented counter clockwise

		float				minX;
		float				maxX;
		float				minY;
		float				maxY;
		float				minZ;
		float				maxZ;

		char				fileName[IMPcEnv_MaxNameLength];
		char				objName[IMPcEnv_MaxNameLength];

		UUtUns32			parent;
		UUtUns32			level;

		UUtUns32			child;
		UUtUns32			next;

		UUtBool				isLeaf;
		UUtUns32			flags;

		float				volume;

		UUtUns32			numSides;
		IMPtEnv_BNV_Side	sides[IMPcEnv_MaxBNVSides];

		UUtUns32			numNodes;
		AKtBNV_BSPNode		bspNodes[IMPcEnv_MaxNumBNVBSPNodes];

		UUtUns32			numGQs;
		UUtUns32			gqList[IMPcEnv_MaxGQsPerBNV];

		M3tPoint3D			origin;			// Bounding box
		M3tPoint3D			antiOrigin;
		M3tPlaneEquation	stairPlane;
		float				stairHeight;

		UUtBool				error;

	} IMPtEnv_BNV;

	enum
	{
		IMPcEnv_TexturePageFlags_None		= 0
	};


/*
 * lightscape stuff
 */

	typedef struct IMPtEnv_LSData
	{
		char			fileName[32];
		struct LStData*	lsData;

	} IMPtEnv_LSData;

/*
 * object stuff
 */
 	typedef struct IMPtEnv_Object
 	{
 		M3tGeometry				*geometry_list[16];
		UUtUns32				geometry_count;

		struct OBtAnimation		*animation;

		UUtUns32				numParticles;
		EPtEnvParticle			particles[IMPcEnv_MaxObjectParticles];

		UUtUns32				lifeSpan;
		UUtUns32				bounces;
		UUtUns32				flags;
		UUtUns32				damage;
		UUtUns32				hitpoints;
		UUtUns32				physicsLevel;
		UUtUns32				scriptID;
		UUtUns32				door_id;
		UUtUns32				door_gq;

		char					defPiecesPrefix[IMPcEnv_MaxPrefixLength][IMPcEnv_MaxPiecesLists];

		char					defTexturePrefix[IMPcEnv_MaxPrefixLength];
		AUtSharedString			defTextureList[IMPcEnv_MaxDefTextures];
		UUtUns32				numDefTextures;

		char					defGeomPrefix[IMPcEnv_MaxPrefixLength];
		M3tGeometry				*defGeomList[IMPcEnv_MaxDefGeometry];
		UUtUns32				numDefGeometry;

		M3tPoint3D				position;
		M3tQuaternion			orientation;
		float					scale;
		M3tMatrix4x3			debugOrigMatrix;

		char					objName[64];	// Must match OBcMaxObjectName in BFW_Object_Templates.h
		char					fileName[64];	// Must match OBcMaxObjectName in BFW_Object_Templates.h

	} IMPtEnv_Object;

/*
 * character setup stuff
 */

	typedef struct IMPtEnv_Flag
	{
		UUtInt32		idNumber;
		M3tMatrix4x3		matrix;
	} IMPtEnv_Flag;

/*
 * general build data
 */
	typedef struct IMPtEnv_OTExtraNodeInfo
	{
		UUtUns32	depth;
		UUtUns32	parent;
		UUtUns32	myOctant;
		UUtUns32	adjacent[6];	// Direct adjacent node - if no adjacent its 0xFFFFFFFF

		UUtInt16	minXIndex;
		UUtInt16	minYIndex;
		UUtInt16	minZIndex;
		UUtInt16	maxXIndex;
		UUtInt16	maxYIndex;
		UUtInt16	maxZIndex;

		UUtUns32	dimIndex;

	} IMPtEnv_OTExtraNodeInfo;

	typedef struct IMPtEnv_OTData
	{
		UUtUns32				maxDepth;
		UUtUns32				gqsPerNodeDist[IMPcEnv_OT_MaxGQsPerNode];
		UUtUns32				bnvsPerNodeDist[IMPcEnv_OT_MaxBNVsPerNode];
		UUtUns32				leafNodeSizeDist[IMPcEnv_OT_NumLeafNodeDims];

		UUtUns32				nextLeafNodeIndex;
		UUtUns32				nextInteriorNodeIndex;
		UUtUns32				nextQTNodeIndex;

		UUtUns32				nextGQIndex;
		UUtUns32				gqIndexArray[IMPcEnv_MaxGQIndexArray];

		AKtOctTree_LeafNode		leafNodes[IMPcEnv_MaxLeafNodes];
		IMPtEnv_OTExtraNodeInfo	leafNodeExtras[IMPcEnv_MaxLeafNodes];
		AKtOctTree_InteriorNode	interiorNodes[IMPcEnv_MaxInteriorNodes];
		IMPtEnv_OTExtraNodeInfo	interiorNodeExtras[IMPcEnv_MaxInteriorNodes];

		AKtQuadTree_Node		qtNodes[IMPcEnv_MaxQTNodes];

		UUtUns32				nextBNVIndex;
		UUtUns32				bnv_index_array[IMPcEnv_MaxBNVIndexArray];

	} IMPtEnv_OTData;

	typedef struct IMPtEnv_GQPatch
	{
		UUtUns32	gqStartIndirectIndex;	// This is an index into the gunk quad index list
		UUtUns32	gqEndIndirectIndex;

		//UUtUns32				numPlanes;
		//IMPtEnv_TexturePlane*	texturePlanes;

	} IMPtEnv_GQPatch;

	typedef struct IMPtEnv_Texture
	{
		tTextureFlags	flags;
		M3tTextureMap*	texture;

	} IMPtEnv_Texture;

#define IMPcFixedOctTreeNode_Count 16
#define IMPcFixedOctTreeNode_ChunkSize 32

	typedef struct IMPtEnv_BuildData
	{
		UUtUns32					maxGQs;

		UUtUns32					numGQs;
		IMPtEnv_GQ*					gqList;

		UUtUns32					maxBNVs;
		UUtUns32					numBNVs;
		IMPtEnv_BNV*				bnvList;

		AUtSharedStringArray*		textureMapArray;
		IMPtEnv_Texture				envTextureList[IMPcEnv_MaxTextures];

		UUtUns16					numFlags;
		IMPtEnv_Flag				flagList[IMPcEnv_MaxFlags];

		UUtUns32					numMarkers;
		MXtMarker					markerList[IMPcEnv_MaxMarkerNodes];

		UUtUns32					numObjects;
		IMPtEnv_Object				objectList[IMPcEnv_MaxObjects];	// Must be cache-aligned

		AUtSharedPointArray*		sharedPointArray;		// shared points
		AUtSharedPlaneEquArray*		sharedPlaneEquArray;	// shared plane equations
		AUtSharedQuadArray*			sharedBNVQuadArray;		// shared quads
		AUtSharedTexCoordArray*		sharedTextureCoordArray;	//

		AUtSharedPointArray*		bspPointArray;			// shared points for bsp construction
		AUtSharedQuadArray*			bspQuadArray;			// shared quads for bsp construction

		IMPtEnv_OTData				otData;

		UUtMemory_Pool*				tempMemoryPool;

		GRtGroup*					environmentGroup;

		// gq patch data
		UUtUns32					numPatches;
		IMPtEnv_GQPatch				patches[IMPcEnv_MaxPatches];

		UUtUns32					numPatchIndIndices;
		UUtUns32					patchIndIndices[IMPcEnv_MaxGQIndexArray];

		AUtSharedEdgeArray*			edgeArray;

		// lightmap data
		UUtUns32					numGQFiles;
		char						gqFileNames[IMPcEnv_MaxFilesInDir][BFcMaxFileNameLength];

		UUtUns32*					gqTakenBV; // generally useful

		UUtBool						reflectivity_specified;
		M3tColorRGB					reflectivity_color;

		// initial particle data
		UUtUns32					numEnvParticles;
		EPtEnvParticle				envParticles[IMPcEnv_MaxEnvParticles];

		AItCharacterSetupArray		*characterSetup;

		float						totalGQArea;

		AUtHashTable				*debugStringTable;
		UUtUns32					debugStringBytes;

		UUtUns32					numAlphaQuads;
		UUtUns32					alphaQuads[IMPcEnv_MaxAlphaQuads];

		UUtUns32					numBSPNodes;
		IMPtEnv_BSP_Node			bspNodes[IMPcEnv_MaxNumBSPNodes];

		UUtUns32					numAlphaBSPNodes;
		AKtAlphaBSPTree_Node		alphaBSPNodes[IMPcEnv_MaxNumBSPNodes];

		// extents
		float						minX, minY, minZ;
		float						maxX, maxY, maxZ;
		float						sizeX, sizeY, sizeZ;

		UUtUns32					*fixed_oct_tree_node[IMPcFixedOctTreeNode_Count][IMPcFixedOctTreeNode_Count][IMPcFixedOctTreeNode_Count];
		UUtUns32					fixed_oct_tree_node_count[IMPcFixedOctTreeNode_Count][IMPcFixedOctTreeNode_Count][IMPcFixedOctTreeNode_Count];
		AUtDict						*fixed_oct_tree_temp_list;

		// gunked objects tag list
		UUtMemory_Array				*object_tags;
		UUtMemory_Array				*object_quads;

		UUtUns16					door_frame_texture;
		UUtUns32					door_count;
		UUtUns32					door_gq_indexes[IMPcEnv_MaxNumDoors];

	} IMPtEnv_BuildData;

/*
 * extern globals
 */
	extern UUtMemory_Pool*	IMPgEnv_MemoryPool;
	extern FILE*			IMPgEnv_StatsFile;
	extern float			IMPgEnv_InchesPerPixel;

/*
 * some prototypes
 */

char *Imp_AddDebugString(
	IMPtEnv_BuildData *inBuildData,
	char *inString);

void
IMPrEnv_LogError(
	char*	inMsg,
	...);

UUtError
IMPrEnv_Parse(
	BFtFileRef*			inSourceFileRef,
	GRtGroup*			inGroup,
	IMPtEnv_BuildData*	inBuildData,
	UUtBool				*ioBuildInstance);

UUtError
IMPrEnv_Process(
	IMPtEnv_BuildData*	inBuildData,
	BFtFileRef*			inSourceFileRef);

UUtError
IMPrEnv_CreateInstance(
	char*				inInstanceName,
	BFtFileRef*			inSourceFileRef,
	IMPtEnv_BuildData*	inBuildData);

UUtError
IMPrEnv_CreateInstance_Debug(
	char*				inInstanceName,
	IMPtEnv_BuildData*	inBuildData);

UUtBool
IMPiEnv_Process_BNV_ContainsBNV(
	IMPtEnv_BuildData*	inBuildData,
	IMPtEnv_BNV*		inParentBNV,
	IMPtEnv_BNV*		inChildBNV);

UUtBool
IMPrEnv_Process_BNV_ContainsStairBNV(
	IMPtEnv_BuildData*	inBuildData,
	IMPtEnv_BNV*		inParentBNV,
	IMPtEnv_BNV*		inChildBNV);

UUtBool
IMPiEnv_Process_BSP_PointInBSP(
	IMPtEnv_BuildData*	inBuildData,
	IMPtEnv_BNV*		inBNV,
	M3tPoint3D*			inPoint,
	float				inTolerance);

UUtError
IMPrEnv_Process_BNV_ComputeProperties(
	IMPtEnv_BuildData*	inBuildData);

void IMPiCalculateNodeOrigins(
	IMPtEnv_BuildData *inBuildData,
	IMPtEnv_BNV *inNode,
	M3tPoint3D *outOrigin,
	M3tPoint3D *outAntiOrigin);

void Imp_Env_AddAdjacency(
	IMPtEnv_BNV_Side *inSrcSide,
	IMPtEnv_BNV *inSrcBNV,
	IMPtEnv_BNV_Side *inDestSide,
	UUtUns32 inDestBNVIndex,
	UUtUns32 inConnectingGQIndex,
	UUtUns16 inFlags,
	IMPtEnv_BuildData *inBuildData);

void Imp_Env_DoStairAdjacency(UUtUns32 ioCurNodeIndex, IMPtEnv_BuildData *inBuildData);
UUtUns32 Imp_Env_ConnectStairsToLanding(IMPtEnv_GQ *inSAT, IMPtEnv_BuildData *inBuildData);
UUtBool Imp_Env_PointInNode(M3tPoint3D* inPoint,IMPtEnv_BuildData* inBuildData,	IMPtEnv_BNV* inNode);

void
IMPrEnv_Process_BNV_ComputeAdjacency(
	IMPtEnv_BuildData*	inBuildData);

UUtError
IMPrEnv_Process_LightMap(
	IMPtEnv_BuildData*	inBuildData,
	BFtFileRef*			inSourceFileRef);

UUtError
IMPrEnv_Process_OctTrees(
	IMPtEnv_BuildData*	inBuildData);

UUtError
IMPrEnv_Process_OctTrees2(
	IMPtEnv_BuildData*	inBuildData);

UUtBool
IMPrLM_DontProcessGQ(
	IMPtEnv_GQ*	inGQ);

UUtUns32
IMPrEnv_GetNodeFlags(
	MXtNode *inNode);

UUtError
IMPrEnv_GetLSData(
	GRtGroup				*inLSGroup,
	float					*outFilterColor,
	IMPtLS_LightType		*outLightType,
	IMPtLS_Distribution		*outDistribution,
	UUtUns32				*outIntensity,
	float					*outBeamAngle,
	float					*outFieldAngle);

void IMPrFixedOctTree_Create(IMPtEnv_BuildData*	inBuildData);
void IMPrFixedOctTree_Delete(IMPtEnv_BuildData*	inBuildData);

void IMPrFixedOctTree_Test(
	IMPtEnv_BuildData *inBuildData,
	float minx,
	float maxx,
	float miny,
	float maxy,
	float minz,
	float maxz);

void PHrCreateRoomData(IMPtEnv_BuildData *inBuildData, IMPtEnv_BNV *inBNV, AKtBNVNode *node, AKtEnvironment *env);

void IMPrEnv_Add_ObjectTag( IMPtEnv_BuildData* inBuildData, UUtUns32 inTag, UUtUns32 inQuadIndex );


#ifdef __cplusplus
}
#endif

#endif /* IMP_ENV_PRIVATE_H */
