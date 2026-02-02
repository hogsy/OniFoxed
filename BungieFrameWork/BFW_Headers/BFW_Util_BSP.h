/*
	FILE:	BFW_Util_BSP.h

	AUTHOR:	Brent H. Pease

	CREATED: April 2, 1998

	PURPOSE:

	Copyright 1997

*/
#ifndef BFW_UTIL_BSP_H
#define BFW_UTIL_BSP_H

#include "BFW_BitVector.h"
#include "BFW_Motoko.h"

typedef struct	UUtBSP_Plane_System	UUtBSP_Plane_System;

enum
{
	UUcBSP_Negative = 0,
	UUcBSP_Positive = 1

};

typedef tm_struct UUtBSP_Plane_Node
{
	UUtUns16	planeEquIndex;
	UUtUns16	posNodeIndex;
	UUtUns16	negNodeIndex;

	UUtUns16	pad;

} UUtBSP_Plane_Node;

#define UUcTemplate_BSP_Plane	UUm4CharToUns32('U', 'B', 'P', 'P')
typedef tm_template('U', 'B', 'P', 'P', "BSP plane node array")
UUtBSP_Plane_Node_Array
{
	tm_pad(2);

	tm_varindex	UUtUns16			numNodes;
	tm_vararray	UUtBSP_Plane_Node	nodes[1];

} UUtBSP_Plane_NodeArray;

/*
 * Operations
 */
UUtBool
UUrBSP_Plane_PointInBSP(
	UUtBSP_Plane_Node*		inNodeArray,
	UUtUns16				inBSPRootNode,
	M3tPlaneEquation*		inPlanes,
	M3tPoint3D*				inPoint);

/*
 * Construction
 */
UUtError
UUrBSP_Plane_Build(
	UUtBSP_Plane_System*	inSystem,
	UUtUns16				inNumQuads,
	UUtUns16*				inQuadIndexList,
	UUtUns16*				inPlaneIndexList,
	UUtUns16				*outNodeIndex);

UUtBSP_Plane_System*
UUrBSP_Plane_System_New(
	M3tPoint3D*			inPointArray,
	M3tQuadLargeIndex*	inQuadArray,
	M3tPlaneEquation*	inPlanes);

void
UUrBSP_Plane_System_Delete(
	UUtBSP_Plane_System*	inSystem);

UUtBSP_Plane_Node*
UUrBSP_Plane_System_GetNodeArray(
	UUtBSP_Plane_System*	inSystem);

UUtError
UUrBSP_Plane_System_MakeTemplate(
	UUtBSP_Plane_System*		inSystem,
	char*						inName,
	UUtBSP_Plane_NodeArray*		*outTemplate);


#endif /* BFW_UTIL_BSP_H */
