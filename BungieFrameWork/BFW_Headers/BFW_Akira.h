#pragma once
/*
	FILE:	BFW_Akira.h
	
	AUTHOR:	Brent H. Pease, Michael Evans
	
	CREATED: July 15, 1998
	
	PURPOSE: environment engine
	
	Copyright 1997-2000

*/
#ifndef BFW_AKIRA_H
#define BFW_AKIRA_H

#include "BFW.h"
#include "BFW_Types.h"
#include "BFW_TemplateManager.h"
#include "BFW_Motoko.h"
#include "BFW_Collision.h"
#include "BFW_Path.h"

#define AKcTexturePageDim	(128)
#define AKcInchesPerUnit	(4.0f)
//#define AKcInchesPerPixel	(6.0f)

#define AKcNumTexturePasses	(2)


//#define AKcMaxAdjacency 32			/* Maximum number of nodes that can share a partition */
#define AKcSphereCollisionPasses 3		/* Maximum number of supported multiple collisions */
#define AKcSphereCollisionFudge 0.001f	// How much extra we push out to avoid repeat collision
#define AKcPointCollisionFudge 0.01f	

#define AKcBNV_OutsideAcceptDistance 5.0f	/* so that we never reject a BNV that we are just a fraction outside of */

#define AKcMaxStackElems	160 // S.S. (20 * 8)
#define AKcMaxTreeDepth		(16)
#define AKcMaxHalfOTDim		4096.f // S.S.(1024.0f * 4.0f)

#define AKcMinOTDim			(16.0f)

#define AKmNoIntersection(p) (!(p).x && !(p).y && !(p).z)

#define AKmPlaneEqu_GetIndex(x) ((x) & 0x7FFFFFFF)
#define AKmPlaneEqu_IsNegative(x) ((x) & 0x80000000)
#define AKmPlaneEqu_Negate(x)		\
		do {						\
			x.a = -x.a;				\
			x.b = -x.b;				\
			x.c = -x.c;				\
			x.d = -x.d;				\
		} while (0);
#define AKmPlaneEqu_GetNormal(p,n)	\
		do {						\
			(n).x = (p).a;				\
			(n).y = (p).b;				\
			(n).z = (p).c;				\
		} while (0);
#define AKmPlaneEqu_GetComponents(index, planeArray, planeA, planeB, planeC, planeD) 	\
		do { 														\
			const M3tPlaneEquation*	p = planeArray + AKmPlaneEqu_GetIndex(index);	\
			planeA = p->a;							\
			planeB = p->b;							\
			planeC = p->c;							\
			planeD = p->d;							\
			if(AKmPlaneEqu_IsNegative(index))	\
			{									\
				planeA = -(planeA);						\
				planeB = -(planeB);						\
				planeC = -(planeC);						\
				planeD = -(planeD);						\
			}									\
		} while(0);
#define AKmPlaneEqu_GetRawComponents(index, planeArray, planeA, planeB, planeC, planeD) 	\
		do { 														\
			M3tPlaneEquation*	p = planeArray + AKmPlaneEqu_GetIndex(index);	\
			planeA = p->a;							\
			planeB = p->b;							\
			planeC = p->c;							\
			planeD = p->d;							\
		} while(0);
		
// Given that x, y, and z are 0 or 1 compute the index into the node child array
//#define AKmOctTreeChildIndex(x, y, z) ((x << 2) | (y << 1) | z)
#define AKmOctTree_IsLeafNode(x)	((x) & 0x80000000)
#define AKmOctTree_GetLeafIndex(x)	((x) & 0x7FFFFFFF)
#define AKmOctTree_MakeLeafIndex(x)	((x) | 0x80000000)

#define	AKcOctTree_GQInd_Start_Shift	(12)
#define AKcOctTree_GQInd_Start_Mask		(0xFFFFF)
#define AKcOctTree_GQInd_Len_Shift		(0)
#define AKcOctTree_GQInd_Len_Mask		(0xFFF)

#define AKmOctTree_DecodeGQIndIndices(gqIndirectIndex_Encode, start, len) \
		start = (UUtUns32)(((gqIndirectIndex_Encode) >> AKcOctTree_GQInd_Start_Shift) & AKcOctTree_GQInd_Start_Mask); \
		len = (UUtUns32)(((gqIndirectIndex_Encode) >> AKcOctTree_GQInd_Len_Shift) & AKcOctTree_GQInd_Len_Mask)

#define AKcOctTree_SideAxis_X	(0)
#define AKcOctTree_SideAxis_Y	(1)
#define AKcOctTree_SideAxis_Z	(2)

#define AKcOctTree_SideBV_X		1 //(1 << AKcOctTree_SideAxis_X)
#define AKcOctTree_SideBV_Y		2 //(1 << AKcOctTree_SideAxis_Y)
#define AKcOctTree_SideBV_Z		4 //(1 << AKcOctTree_SideAxis_Z)

#define AKcOctTree_Side_NegX	(0)
#define AKcOctTree_Side_PosX	(1)
#define AKcOctTree_Side_NegY	(2)
#define AKcOctTree_Side_PosY	(3)
#define AKcOctTree_Side_NegZ	(4)
#define AKcOctTree_Side_PosZ	(5)

#define AKcOctTree_SideBV_PosX	(1 << AKcOctTree_Side_PosX)
#define AKcOctTree_SideBV_NegX	(1 << AKcOctTree_Side_NegX)
#define AKcOctTree_SideBV_PosY	(1 << AKcOctTree_Side_PosY)
#define AKcOctTree_SideBV_NegY	(1 << AKcOctTree_Side_NegY)
#define AKcOctTree_SideBV_PosZ	(1 << AKcOctTree_Side_PosZ)
#define AKcOctTree_SideBV_NegZ	(1 << AKcOctTree_Side_NegZ)

#define AKcOctTree_Edge_PXPY	(0)	// Edge shared by the pos x and pos y plane
#define AKcOctTree_Edge_PXNY	(1)	// Edge shared by the pos x and neg y plane
#define AKcOctTree_Edge_PXPZ	(2)	// Edge shared by the pos x and pos z plane
#define AKcOctTree_Edge_PXNZ	(3)	// Edge shared by the pos x and neg z plane
#define AKcOctTree_Edge_NXPY	(4)	// Edge shared by the neg x and pos y plane
#define AKcOctTree_Edge_NXNY	(5)	// Edge shared by the neg x and neg y plane
#define AKcOctTree_Edge_NXPZ	(6)	// Edge shared by the neg x and pos z plane
#define AKcOctTree_Edge_NXNZ	(7)	// Edge shared by the neg x and neg z plane
#define AKcOctTree_Edge_PYPZ	(8)	// Edge shared by the pos y and pos z plane
#define AKcOctTree_Edge_PYNZ	(9)	// Edge shared by the pos y and neg z plane
#define AKcOctTree_Edge_NYPZ	(10)	// Edge shared by the neg y and pos z plane
#define AKcOctTree_Edge_NYNZ	(11)	// Edge shared by the neg y and neg z plane

#define AKcOctTree_EdgeBV_PXPY	1 //(1 << 0)	// Edge shared by the pos x and pos y plane
#define AKcOctTree_EdgeBV_PXNY	2 //(1 << 1)	// Edge shared by the pos x and neg y plane
#define AKcOctTree_EdgeBV_PXPZ	4 //(1 << 2)	// Edge shared by the pos x and pos z plane
#define AKcOctTree_EdgeBV_PXNZ	8 //(1 << 3)	// Edge shared by the pos x and neg z plane
#define AKcOctTree_EdgeBV_NXPY	16 //(1 << 4)	// Edge shared by the neg x and pos y plane
#define AKcOctTree_EdgeBV_NXNY	32 //(1 << 5)	// Edge shared by the neg x and neg y plane
#define AKcOctTree_EdgeBV_NXPZ	64 //(1 << 6)	// Edge shared by the neg x and pos z plane
#define AKcOctTree_EdgeBV_NXNZ	128 //(1 << 7)	// Edge shared by the neg x and neg z plane
#define AKcOctTree_EdgeBV_PYPZ	256 //(1 << 8)	// Edge shared by the pos y and pos z plane
#define AKcOctTree_EdgeBV_PYNZ	512 //(1 << 9)	// Edge shared by the pos y and neg z plane
#define AKcOctTree_EdgeBV_NYPZ	1024 //(1 << 10)	// Edge shared by the neg y and pos z plane
#define AKcOctTree_EdgeBV_NYNZ	2048 //(1 << 11)	// Edge shared by the neg y and neg z plane

#define AKcQuadTree_SideAxis_U	(0)
#define AKcQuadTree_SideAxis_V	(1)

#define AKcQuadTree_SideBV_U	(1 << AKcQuadTree_SideAxis_U)
#define AKcQuadTree_SideBV_V	(1 << AKcQuadTree_SideAxis_V)

extern UUtBool		AKgDebug_DebugMaps;
extern UUtBool		AKgDraw_Occl;
extern UUtBool		AKgDraw_SoundOccl;
extern UUtBool		AKgDraw_Invis;
extern UUtBool		AKgDraw_ObjectCollision;
extern UUtBool		AKgDraw_CharacterCollision;

typedef tm_struct AKtAlphaBSPTree_Node
{
	UUtUns32	gqIndex;
	UUtUns32	planeEquIndex;
	UUtUns32	posNodeIndex;
	UUtUns32	negNodeIndex;
	
} AKtAlphaBSPTree_Node;

#define AKcTemplate_AlphaBSPTree_NodeArray	UUm4CharToUns32('A', 'B', 'N', 'A')
typedef tm_template('A', 'B', 'N', 'A', "bsp tree node Array")
AKtAlphaBSPTree_NodeArray
{
	tm_pad								pad0[20];
	
	tm_varindex	UUtUns32				numNodes;
	tm_vararray	AKtAlphaBSPTree_Node	nodes[1];
	
} AKtAlphaBSPTree_NodeArray;

typedef tm_struct AKtQuadTree_Node
{
	UUtUns32 children[4];
	
} AKtQuadTree_Node;

#define AKcOctTree_Shift_Dim	27
#define AKcOctTree_Mask_Dim		0x1F
#define AKcOctTree_Shift_X		18
#define AKcOctTree_Mask_X		0x1FF
#define AKcOctTree_Shift_Y		9
#define AKcOctTree_Mask_Y		0x1FF
#define AKcOctTree_Shift_Z		0
#define AKcOctTree_Mask_Z		0x1FF

typedef tm_struct AKtOctTree_LeafNode
{
	UUtUns32	gqIndirectIndex_Encode;	// length & base packed as AKcOctTree_GQInd_Start_Mask & AKcOctTree_GQInd_Len_Mask

	UUtUns32	adjInfo[6];				// If the high bit it set this points to a octtree leaf node
										// otherwise it points to a quad tree

	UUtUns32	dim_Encode;				// 5 bits - leaf node size
										// 9 bits - max x position
										// 9 bits - max y position
										// 9 bits - max z position
	UUtUns32	bnvIndirectIndex_Encode;		// high order 24 bits is the base - low order 8 is the length
	
} AKtOctTree_LeafNode;

typedef tm_struct AKtOctTree_InteriorNode
{
	UUtUns32		children[8];

} AKtOctTree_InteriorNode;

#define AKcTemplate_QuadTree_NodeArray	UUm4CharToUns32('Q', 'T', 'N', 'A')
typedef tm_template('Q', 'T', 'N', 'A', "Quad tree node Array")
AKtQuadTree_NodeArray
{
	tm_pad								pad0[20];
	
	tm_varindex	UUtUns32				numNodes;
	tm_vararray	AKtQuadTree_Node		nodes[1];
	
} AKtQuadTree_NodeArray;

#define AKcTemplate_OctTree_LeafNodeArray	UUm4CharToUns32('O', 'T', 'L', 'F')
typedef tm_template('O', 'T', 'L', 'F', "Oct tree leaf node Array")
AKtOctTree_LeafNodeArray
{
	tm_pad								pad0[20];
	
	tm_varindex	UUtUns32				numNodes;
	tm_vararray	AKtOctTree_LeafNode		nodes[1];
	
} AKtOctTree_LeafNodeArray;

#define AKcTemplate_OctTree_InteriorNodeArray	UUm4CharToUns32('O', 'T', 'I', 'T')
typedef tm_template('O', 'T', 'I', 'T', "Oct tree interior node Array")
AKtOctTree_InteriorNodeArray
{
	tm_pad								pad0[20];
	
	tm_varindex	UUtUns32				numNodes;
	tm_vararray	AKtOctTree_InteriorNode	nodes[1];
	
} AKtOctTree_InteriorNodeArray;

#define AKcTemplate_OctTree	UUm4CharToUns32('A', 'K', 'O', 'T')
typedef tm_template('A', 'K', 'O', 'T', "Oct tree")
AKtOctTree
{
	AKtOctTree_InteriorNodeArray*	interiorNodeArray;
	AKtOctTree_LeafNodeArray*		leafNodeArray;
	AKtQuadTree_NodeArray*			qtNodeArray;
	TMtIndexArray*					gqIndices;
	TMtIndexArray*					bnvIndices;
	
} AKtOctTree;

/*
 * BSP Tree
 */
typedef tm_struct AKtBNV_BSPNode
{
	UUtUns32	planeEquIndex;
	UUtUns32	posNodeIndex;
	UUtUns32	negNodeIndex;
} AKtBNV_BSPNode;

#define AKcTemplate_BSPNodeArray	UUm4CharToUns32('A', 'K', 'B', 'P')
typedef tm_template('A', 'K', 'B', 'P', "BSP node Array")
AKtBSPNodeArray
{
	tm_pad						pad0[22];
	
	tm_varindex	UUtUns16		numNodes;
	tm_vararray	AKtBNV_BSPNode	nodes[1];
	
} AKtBSPNodeArray;

/*
 * A GQ quad is the basic unit of the environment
 */
	enum	// GQ quad flags
	{
		AKcGQ_Flag_None						= 0,
		AKcGQ_Flag_Door						= (1 << 0),
		AKcGQ_Flag_Ghost					= (1 << 1),
		AKcGQ_Flag_SAT_Up					= (1 << 2),
		AKcGQ_Flag_SAT_Down					= (1 << 3),
		AKcGQ_Flag_Stairs					= (1 << 4),
		AKcGQ_Flag_Jello					= (1 << 5),
		AKcGQ_Flag_Triangle					= (1 << 6),	
		AKcGQ_Flag_Transparent				= (1 << 7),
		AKcGQ_Flag_Draw_Flash				= (1 << 8),
		AKcGQ_Flag_DrawBothSides			= (1 << 9),
		AKcGQ_Flag_Trigger					= (1 << 10),
		AKcGQ_Flag_No_Collision				= (1 << 11),
		AKcGQ_Flag_Flash_State				= (1 << 12),
		AKcGQ_Flag_Invisible				= (1 << 13),
		AKcGQ_Flag_No_Object_Collision		= (1 << 14),
		AKcGQ_Flag_No_Character_Collision	= (1 << 15),
		AKcGQ_Flag_No_Occlusion				= (1 << 16),
		AKcGQ_Flag_AIDanger  				= (1 << 17),
		AKcGQ_Flag_BrokenGlass				= (1 << 18),		// walking on, walking on, broken glass (could not resist)
		AKcGQ_Flag_FloorCeiling				= (1 << 19),
		AKcGQ_Flag_Wall						= (1 << 20),
		AKcGQ_Flag_Breakable				= (1 << 21),		// calculated at runtime from texture materials
		AKcGQ_Flag_AIGridIgnore				= (1 << 22),
		AKcGQ_Flag_NoDecal					= (1 << 23),
		AKcGQ_Flag_Furniture				= (1 << 24),
		AKcGQ_Flag_Projection0				= (1 << 25),
		AKcGQ_Flag_Projection1				= (1 << 26),
		AKcGQ_Flag_SoundTransparent			= (1 << 27),
		AKcGQ_Flag_AIImpassable				= (1 << 28)
	};

	#define AKcGQ_Flag_Projection_Shift		25
	#define AKcGQ_Flag_Projection_Mask		(AKcGQ_Flag_Projection0 | AKcGQ_Flag_Projection1)

	#define AKcGQ_Flag_Col_Skip		(AKcGQ_Flag_No_Collision | AKcGQ_Flag_BrokenGlass)
	#define AKcGQ_Flag_Chr_Col_Skip (AKcGQ_Flag_No_Collision | AKcGQ_Flag_No_Character_Collision | AKcGQ_Flag_BrokenGlass)
	#define AKcGQ_Flag_Obj_Col_Skip (AKcGQ_Flag_No_Collision | AKcGQ_Flag_No_Object_Collision | AKcGQ_Flag_BrokenGlass)
	#define AKcGQ_Flag_LOS_CanSee_Skip_Player (AKcGQ_Flag_Invisible | AKcGQ_Flag_Transparent | AKcGQ_Flag_Jello | AKcGQ_Flag_BrokenGlass)
	#define AKcGQ_Flag_LOS_CanSee_Skip_AI (AKcGQ_Flag_Invisible | AKcGQ_Flag_Transparent | AKcGQ_Flag_BrokenGlass)
	#define AKcGQ_Flag_LOS_CanShoot_Skip_AI (AKcGQ_Flag_Invisible | AKcGQ_Flag_Breakable | AKcGQ_Flag_BrokenGlass)
	#define AKcGQ_Flag_Jello_Skip (AKcGQ_Flag_Invisible | AKcGQ_Flag_BrokenGlass)
	#define AKcGQ_Flag_Occlude_Skip	(AKcGQ_Flag_No_Occlusion | AKcGQ_Flag_Jello | AKcGQ_Flag_Transparent | AKcGQ_Flag_Invisible | AKcGQ_Flag_BrokenGlass)
	#define AKcGQ_Flag_SoundOcclude_Skip (AKcGQ_Flag_No_Collision | AKcGQ_Flag_No_Object_Collision | AKcGQ_Flag_BrokenGlass | AKcGQ_Flag_SoundTransparent)

	#define AKcGQ_Flag_2Sided	(AKcGQ_Flag_Transparent | AKcGQ_Flag_DrawBothSides | AKcGQ_Flag_Jello)
	
	#define AKcGQ_Flag_SAT_Mask (	\
			AKcGQ_Flag_SAT_Up |			\
			AKcGQ_Flag_SAT_Down)

	#define AKcGQ_Flag_PathfindingMask (	\
			AKcGQ_Flag_Ghost |			\
			AKcGQ_Flag_SAT_Mask |		\
			AKcGQ_Flag_Door)
			
	#define AKcGQ_Flag_NoTextureMask (	\
			AKcGQ_Flag_PathfindingMask)
	
	#define AKcGQ_Flag_DontLight	(AKcGQ_Flag_Invisible | AKcGQ_Flag_NoTextureMask | AKcGQ_Flag_2Sided)

	#define AKmGetQuadProjection(flags) ((flags & AKcGQ_Flag_Projection_Mask) >> AKcGQ_Flag_Projection_Shift)

	typedef UUtUns32 AKtGQ_Flags;
	
	#define AKcTemplate_GQ_Material UUm4CharToUns32('A', 'G', 'Q', 'M')
	typedef tm_template('A', 'G', 'Q', 'M', "Gunk Quad Material")
	AKtGQMaterial
	{
		UUtUns16 damage;
		UUtUns16 effectTime;

		UUtUns16 stateTimer;		// Imported as 0
		UUtUns16 contactTimer;		// Imported as 0
	} AKtGQMaterial;

	tm_struct AKtGQ_General
	{
		M3tQuadSplit		m3Quad;		// This stores the motoko info
		UUtUns32			flags;		// These are the flags
		UUtUns32			object_tag;	// -1 if not an object (furniture, console, turret, door, etc)
	};
	
	tm_struct AKtGQ_Render
	{
		UUtUns16	textureMapIndex;
		UUtUns16	alpha;
		
		// CB: these are no longer used anywhere (they were never reliable anyway) so I'm nuking them
		//UUtUns32	adjGQIndices[4];
	};
	

	typedef tm_struct AKtGQ_Debug
	{
		tm_raw(char *)	object_name;
		tm_raw(char *)	file_name;
	} AKtGQ_Debug;
	
	typedef tm_struct AKtGQ_Collision
	{
		// CB: projection only had two significant bits! they are now stored in gqGeneral->flags.
		//UUtUns32			projection;
		UUtUns32			planeEquIndex;		// This is the index into the plane equation array

		// These are used to quickly compute the bbox for this quad
		M3tBoundingBox_MinMax	bBox;	
	} AKtGQ_Collision;

	#define AKcTemplate_GQ_General	UUm4CharToUns32('A', 'G', 'Q', 'G')
	typedef tm_template('A', 'G', 'Q', 'G', "Gunk Quad General Array")
	AKtGQGeneralArray
	{
		tm_pad						pad0[20];
		
		tm_varindex	UUtUns32		numGQs;
		tm_vararray	AKtGQ_General	gqGeneral[1];
		
	} AKtGQGeneralArray;

	#define AKcTemplate_GQ_Render	UUm4CharToUns32('A', 'G', 'Q', 'R')
	typedef tm_template('A', 'G', 'Q', 'R', "Gunk Quad Render Array")
	AKtGQRenderArray
	{
		tm_pad						pad0[20];
		
		tm_varindex	UUtUns32		numGQs;
		tm_vararray	AKtGQ_Render	gqRender[1];
		
	} AKtGQRenderArray;

	#define AKcTemplate_GQ_Collision	UUm4CharToUns32('A', 'G', 'Q', 'C')
	typedef tm_template('A', 'G', 'Q', 'C', "Gunk Quad Collision Array")
	AKtGQCollisionArray
	{
		tm_pad						pad0[20];
		
		tm_varindex	UUtUns32		numGQs;
		tm_vararray	AKtGQ_Collision	gqCollision[1];
		
	} AKtGQCollisionArray;

	#define AKcTemplate_GQ_Debug	UUm4CharToUns32('A', 'G', 'D', 'B')
	typedef tm_template('A', 'G', 'D', 'B', "Gunk Quad Debug Array")
	AKtGQDebugArray
	{
		tm_pad						pad0[20];
		
		tm_varindex	UUtUns32		numGQs;
		tm_vararray	AKtGQ_Debug		gqDebug[1];
		
	} AKtGQDebugArray;

/*
 *
 */
	enum	// BNV flags
	{
		AKcBNV_Flag_None				= 0,
		AKcBNV_Flag_Stairs_Standard		= (1 << 0),
		AKcBNV_Flag_Stairs_Spiral		= (1 << 1),
		AKcBNV_Flag_Room				= (1 << 2),	// This means that this node needs path grid
		AKcBNV_Flag_NonAI				= (1 << 3),	// This BNV will not have pathfinding
		AKcBNV_Flag_SimplePathfinding	= (1 << 4)	// gridless pathfinding in this room
	};
	#define AKcBNV_Flag_Stairs (AKcBNV_Flag_Stairs_Standard | AKcBNV_Flag_Stairs_Spiral)
	
/*
 *
 */
	typedef tm_struct AKtAdjacency
	{
		UUtUns32	adjacentBNVIndex;	// Node connection data
		UUtUns32	adjacentGQIndex;	// Absolute gunk quad indices of shared ???s
		UUtUns32	adjacencyFlags;		
	} AKtAdjacency;
	
	#define AKcTemplate_AdjacencyArray	UUm4CharToUns32('A', 'K', 'A', 'A')
	typedef tm_template('A', 'K', 'A', 'A', "Adjacency Array")
	AKtAdjacencyArray
	{
		tm_pad						pad0[20];
		
		tm_varindex	UUtUns32		numAdjacenies;
		tm_vararray	AKtAdjacency	adjacencies[1];
		
	} AKtAdjacencyArray;

	typedef tm_struct AKtBNVNode_Side
	{
		UUtUns32	planeEquIndex;
		
		UUtUns32	adjacencyStartIndex;
		UUtUns32	adjacencyEndIndex;
		
		UUtUns32	ghostGQStartIndIndex;
		UUtUns32	ghostGQEndIndIndex;
		
		UUtUns32	bnvQuadStartIndIndex;
		UUtUns32	bnvQuadEndIndIndex;
	} AKtBNVNode_Side;
	
	#define AKcTemplate_BNVNodeSideArray	UUm4CharToUns32('A', 'K', 'B', 'A')
	typedef tm_template('A', 'K', 'B', 'A', "Side Array")
	AKtBNVNodeSideArray
	{
		tm_pad						pad0[20];
		
		tm_varindex	UUtUns32		numSides;
		tm_vararray	AKtBNVNode_Side	sides[1];
		
	} AKtBNVNodeSideArray;

/*
 * 
 */
	typedef tm_struct AKtBNVNode
	{	
		UUtUns32			bspRootNode;
		UUtUns32			index;				// Our index in the environment
		
		UUtUns32			sideStartIndex;		// Index into the volume side array
		UUtUns32			sideEndIndex;		//
		
		UUtUns32			childIndex;
		UUtUns32			nextIndex;
		
		UUtUns32			pathnodeIndex;		// set at runtime
		PHtRoomData			roomData;			// Pathfinding data
		UUtUns32			flags;
		M3tPlaneEquation	stairPlane;
		float				stairHeight;
				
	} AKtBNVNode;

/*
 * 
 */
	#define AKcTemplate_BNVNodeArray	UUm4CharToUns32('A', 'K', 'V', 'A')
	typedef tm_template('A', 'K', 'V', 'A', "BNV Node Array")
	AKtBNVNodeArray
	{
		tm_pad					pad0[20];
		
		tm_varindex	UUtUns32	numNodes;
		tm_vararray	AKtBNVNode	nodes[1];
		
	} AKtBNVNodeArray;

/*
 *	Tbe Doors
 */
	typedef tm_struct AKtDoorFrame
	{
		UUtUns32					gq_index;
		M3tBoundingBox_MinMax		bBox;
		M3tPoint3D					position;
		float						facing;
		float						width;
		float						height;
	} AKtDoorFrame;

	#define AKcTemplate_DoorFrameArray		UUm4CharToUns32('A', 'K', 'D', 'A')
	typedef tm_template('A', 'K', 'D', 'A', "Door Frame Array")
	AKtDoorFrameArray
	{
		tm_pad						pad0[20];
		
		tm_varindex	UUtUns32		door_count;
		tm_vararray	AKtDoorFrame	door_frames[1];
		
	} AKtDoorFrameArray;

/*
 * 
 */
	#define AKcTemplate_Environment	UUm4CharToUns32('A', 'K', 'E', 'V')

	tm_template('A', 'K', 'E', 'V', "Akira Environment")
	AKtEnvironment
	{	
		M3tPoint3DArray*			pointArray;				// List of points used by the nodes of this object
		M3tPlaneEquationArray*		planeArray;				// List of planes used by the nodes of this object
		M3tTextureCoordArray*		textureCoordArray;		// List of texture coordinates for base and light coords

		AKtGQGeneralArray*			gqGeneralArray;			// These are parallel arrays of gunk quad info
		AKtGQRenderArray*			gqRenderArray;
		AKtGQCollisionArray*		gqCollisionArray;
		AKtGQDebugArray*			gqDebugArray;			// CB: this may be NULL if shipping version
	
		M3tTextureMapArray*			textureMapArray;		// Arrays of texture map
		
		AKtBNVNodeArray*			bnvNodeArray;			// Array of volume nodes
		AKtBNVNodeSideArray*		bnvSideArray;			// List of volume node sides

		TMtIndexArray*				envQuadRemapIndices;	// List of quad indices that have script remaps
		TMtIndexArray*				envQuadRemaps;			// Script remap IDs of those quads

		AKtBSPNodeArray*			bspNodeArray;			// List of BSP Nodes
		
		AKtAlphaBSPTree_NodeArray*	alphaBSPNodeArray;		// list of BSP nodes for alpha quads
		
		AKtOctTree*					octTree;
		
		AKtAdjacencyArray* 			adjacencyArray;
		
		AKtDoorFrameArray*			door_frames;			// Array of doors frames

		M3tBoundingBox_MinMax		bbox;					// bounding box for the environment
		M3tBoundingBox_MinMax		visible_bbox;

		float						inchesPerPixel;
	};


/*
 * Collision stuff
 */
	#define AKcMaxNumCollisions	255	// reducing less then 100 is known to break character collision
	
	typedef struct AKtCollision
	{
		UUtUns32			compare_distance;
		CLtCollisionType	collisionType;
		UUtUns16			contactType;
		
		M3tPlaneEquation	plane;
		M3tPoint3D			collisionPoint;
		
		UUtUns32			gqIndex;		// For face collisions
		M3tPoint3D			bEdgeL,bEdgeR;	// For bbox collisions (which edge (Collision) or vertex (Contact) of bbox collided)
		
		float				float_distance;
		
	} AKtCollision;
	
	extern AKtCollision	AKgCollisionList[AKcMaxNumCollisions];
	extern UUtUns16		AKgNumCollisions;

	extern AKtEnvironment*	AKgEnvironment;
	
/*
 * Spatial locality
 */

	typedef struct AKtOctNodeIndexCache
	{
		UUtUns32			node_index;
		M3tVector3D			node_center;
		float				node_dim;

	} AKtOctNodeIndexCache;

UUtError
AKrLevel_Begin(
	AKtEnvironment*	inEnvironment);

void
AKrLevel_End(
	void);
	
UUtError
AKrInitialize(
	void);

void
AKrTerminate(
	void);

UUtError
AKrRegisterTemplates(
	void);

UUtError
AKrEnvironment_SetContextDimensions(
	AKtEnvironment*	inEnvironment,
	UUtUns16		inWidth,
	UUtUns16		inHeight);

void AKrEnvironment_FastMode(UUtBool inFast);
void AKrEnvironment_GunkChanged(void);
	
UUtError
AKrEnvironment_StartFrame(
	AKtEnvironment*	inEnvironment,
	M3tGeomCamera*	inCamera,
	UUtBool			*outRenderSky);

UUtError
AKrEnvironment_EndFrame(
	AKtEnvironment*	inEnvironment);

void AKrEnvironment_NodeList_Get(
	M3tBoundingBox_MinMax *inBoundingBox,
	UUtUns32 inMaxCount,
	UUtUns32 *outList,
	UUtUns32 startIndex);

UUtBool AKrEnvironment_NodeList_Visible(
	const UUtUns32 *inList);

UUtBool
AKrEnvironment_IsBoundingBoxMinMaxVisible(
	M3tBoundingBox_MinMax *inBoundingBox);

UUtBool AKrEnvironment_IsBoundingBoxMinMaxVisible_WithNodeIndex(
	M3tBoundingBox_MinMax *bounding_box,
	UUtUns32 start_node, UUtUns32 *visible_node);

UUtBool
AKrEnvironment_PointIsVisible(
	const M3tPoint3D *inPoint);

UUtBool
AKrEnvironment_IsGeometryVisible(
		const M3tGeometry *inGeometry,
		const M3tMatrix4x3 *inMatrix);

void
AKrEnvironment_DrawIfVisible(
		M3tGeometry *inGeometry,
		const M3tMatrix4x3 *inMatrix);

void
AKrEnvironment_DrawIfVisible_Point(
		M3tGeometry *inGeometry,
		const M3tMatrix4x3 *inMatrix);
								
AKtBNVNode*
AKrNodeFromPoint(
	const M3tPoint3D*		inPoint);

UUtBool
AKrCollision_Point(
	const AKtEnvironment*	inEnvironment,
	const M3tPoint3D*		inPoint,
	M3tVector3D*			inVector,
	UUtUns32				inSkipFlags,
	UUtUns16				inNumCollisions);

UUtBool
AKrCollision_Point_SpatialCache(
	const AKtEnvironment*	inEnvironment,
	const M3tPoint3D*		inPoint,
	M3tVector3D*			inVector,
	UUtUns32				inSkipFlags,
	UUtUns16				inNumCollisions,
	AKtOctNodeIndexCache*	ioStartCache,		// note: start and end cache may be the same
	AKtOctNodeIndexCache*	ioEndCache);		// (recommended for moving objects)

UUtBool
AKrCollision_Sphere(
	const AKtEnvironment*		inEnvironment,
	const M3tBoundingSphere*	inSphere,
	M3tVector3D*				inVector,
	UUtUns32					inSkipFlags,
	UUtUns16					inNumCollisions);

UUtBool
AKrLineOfSight(
	M3tPoint3D *inPointA,
	M3tPoint3D *inPointB);
		
UUtUns32
AKrGQToIndex(
	AKtEnvironment*		inEnvironment,
	AKtGQ_General*		inGQ);

UUtBool
AKrPointInNode(
	const M3tPoint3D*		inViewpoint,
	const AKtEnvironment*	inEnvironment,
	UUtUns32				inNodeIndex,
	M3tPlaneEquation*		outRejectingPlane,
	float*					outRejectionValue);
	
UUtBool
AKrPointInNodeVertically(
	const M3tPoint3D*			inPoint,
	const AKtEnvironment*		inEnv,
	const AKtBNVNode*			inNode);

UUtBool
AKrGQ_SphereVector(
	const AKtEnvironment*		inEnvironment,
	UUtUns32					inGQIndex,
	const M3tBoundingSphere*	inSphere,
	const M3tVector3D*			inVector,
	M3tPoint3D					*outIntersection);	// NULL if you don't care

UUtUns32 AKrEnvironment_GetVisCount(AKtEnvironment*	inEnvironment);
UUtUns32 *AKrEnvironment_GetVisVector(AKtEnvironment* inEnvironment);

void AKrCollision_InitializeSpatialCache(
	AKtOctNodeIndexCache*		outCache);

#endif /* BFW_AKIRA_H */
