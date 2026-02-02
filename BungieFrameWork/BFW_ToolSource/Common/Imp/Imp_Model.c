/*
	FILE:	Imp_Model.c

	AUTHOR:	Michael Evans

	CREATED: August 14, 1997

	PURPOSE: Loads models into the game

	Copyright 1997, 1998

*/

#include <stdio.h>
#include <string.h>

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_FileManager.h"
#include "BFW_TemplateManager.h"
#include "BFW_TM_Construction.h"
#include "BFW_Group.h"
#include "BFW_MathLib.h"
#include "BFW_AppUtilities.h"

#include "Imp_ParseEnvFile.h"
#include "Imp_Common.h"
#include "Imp_Model.h"

#define	IMPcModel_MaxTris			(1024 * 4)
#define IMPcModel_MaxStripLength	(512)
#define IMPcModel_MaxLevels			(256)

#define TRI_AREA_TEST			1

char*		gModelCompileTime = __DATE__" "__TIME__;
UUtUns32	gTotal_NumTris = 0;
UUtUns32	gTotal_NumStrips = 0;
UUtUns32	gTotal_NumObjects = 0;
UUtUns32	gTotal_StripDist[IMPcModel_MaxStripLength];

typedef struct IMPtModel_Tri
{
	UUtUns32	triIndex;
	UUtUns32	sharedPrevEdge;
	UUtUns32	sharedNextEdge;

} IMPtModel_Tri;

AUtSharedEdgeArray*	IMPgModel_TriEdgeArray = NULL;
M3tTri*		IMPgModel_TriEdgeList;
UUtUns32			IMPgModel_TriNumEdges;
UUtUns32*			IMPgModel_VertexFinalList;
UUtUns32			IMPgModel_VertexFinalListLength;
UUtUns32*			IMPgModel_TriFinalList;
UUtUns32			IMPgModel_TriFinalListLength;
UUtUns32*			IMPgModel_TriUsedBV;

M3tVector3D*		IMPgModel_TriNormalList;
M3tTri*				IMPgModel_TriList;
UUtUns32*			IMPgModel_TriNormalIndexList;
UUtUns32			IMPgModel_NumTriNormals;

UUtUns32			IMPgModel_NumTris;


UUtUns32		IMPgModel_Strip_Length;
UUtUns32		IMPgModel_StripIndices[IMPcModel_MaxStripLength];
UUtUns32		IMPgModel_TriIndices[IMPcModel_MaxStripLength];

UUtUns32		IMPgModel_ActiveStrip_Length;
UUtUns32		IMPgModel_ActiveStripIndices[IMPcModel_MaxStripLength];
UUtUns32		IMPgModel_ActiveTriIndices[IMPcModel_MaxStripLength];

AUtEdge*		IMPgModel_EdgeList;

static UUtUns32
IMPiModel_FindOtherVertexIndex(
	M3tTri*		inTri,
	AUtEdge*	inEdge)
{
	UUtUns32	itr;
	UUtUns32	bv = 0;

	for(itr = 0; itr < 3; itr++)
	{
		if(inTri->indices[itr] == inEdge->vIndex0 || inTri->indices[itr] == inEdge->vIndex1)
		{
			bv |= 1 << itr;
		}
	}

	if(bv == 3) return inTri->indices[2];
	else if(bv == 5) return inTri->indices[1];
	else if(bv == 6) return inTri->indices[0];

	UUmAssert(0);

	return 0;
}

static UUtError
IMPiStripify_BuildOneStrip_ForReal(
	UUtUns16	inTriIndex)
{
	UUtUns32		curTriIndex = inTriIndex;
	AUtEdge*		sharedEdge;
	UUtUns32		index0, index1;
	UUtUns32		itr;
	M3tTri*			curTri;
	UUtUns32		otherVertexIndex;

	index0 = IMPgModel_ActiveStripIndices[1];
	index1 = IMPgModel_ActiveStripIndices[2];

	// find the initial sharedEdge
	sharedEdge = AUrSharedEdge_Triangle_FindEdge(IMPgModel_TriEdgeArray, IMPgModel_TriEdgeList + inTriIndex, index0, index1);
	UUmAssert(sharedEdge != NULL);

	while(1)
	{
		if(IMPgModel_ActiveStrip_Length >= IMPcModel_MaxStripLength) UUmError_ReturnOnErrorMsg(UUcError_Generic, "Out of strip mem");

		// record the curTri
		UUrBitVector_SetBit(IMPgModel_TriUsedBV, curTriIndex);
		IMPgModel_ActiveTriIndices[IMPgModel_ActiveStrip_Length - 3] = curTriIndex;

		// try to move into the next tri
		curTriIndex = (UUtUns16)AUrSharedEdge_GetOtherFaceIndex(sharedEdge, curTriIndex);
		if(curTriIndex == UUcMaxUns16 || UUrBitVector_TestBit(IMPgModel_TriUsedBV, curTriIndex)) break;

		curTri = IMPgModel_TriList + curTriIndex;

		// find the vertex to add
		otherVertexIndex = IMPiModel_FindOtherVertexIndex(curTri, sharedEdge);

		IMPgModel_ActiveStripIndices[IMPgModel_ActiveStrip_Length++] = otherVertexIndex;

		// find the new shared edge
		sharedEdge = AUrSharedEdge_Triangle_FindEdge(IMPgModel_TriEdgeArray, IMPgModel_TriEdgeList + curTriIndex, index1, otherVertexIndex);
		UUmAssert(sharedEdge != NULL);

		index1 = otherVertexIndex;
	}

	// if this is the longest strip so far then save it
	if(IMPgModel_ActiveStrip_Length > IMPgModel_Strip_Length)
	{
		IMPgModel_Strip_Length = IMPgModel_ActiveStrip_Length;
		UUrMemory_MoveFast(IMPgModel_ActiveStripIndices, IMPgModel_StripIndices, IMPgModel_ActiveStrip_Length * sizeof(UUtUns32));
		UUrMemory_MoveFast(IMPgModel_ActiveTriIndices, IMPgModel_TriIndices, (IMPgModel_ActiveStrip_Length - 2) * sizeof(UUtUns32));
	}

	// undo all the set bits
	for(itr = 0; itr < IMPgModel_ActiveStrip_Length - 2; itr++)
	{
		UUrBitVector_ClearBit(IMPgModel_TriUsedBV, IMPgModel_ActiveTriIndices[itr]);
	}

	return UUcError_None;
}

static UUtError
IMPiStripify_BuildOneStrip(
	UUtBool		*outDone)
{
	UUtError	error;
	UUtUns16	itrTri;
	M3tTri*		curTri;
	UUtBool		done = UUcTrue;

	IMPgModel_Strip_Length = 0;

	for(itrTri = 0; itrTri < IMPgModel_NumTris; itrTri++)
	{
		// only consider tris that are not taken
		if(!UUrBitVector_TestBit(IMPgModel_TriUsedBV, itrTri))
		{
			done = UUcFalse;
			curTri = IMPgModel_TriList + itrTri;

			IMPgModel_ActiveStrip_Length = 3;
			IMPgModel_ActiveStripIndices[0] = curTri->indices[0];
			IMPgModel_ActiveStripIndices[1] = curTri->indices[1];
			IMPgModel_ActiveStripIndices[2] = curTri->indices[2];
			error = IMPiStripify_BuildOneStrip_ForReal(itrTri);
			UUmError_ReturnOnError(error);

			IMPgModel_ActiveStrip_Length = 3;
			IMPgModel_ActiveStripIndices[0] = curTri->indices[1];
			IMPgModel_ActiveStripIndices[1] = curTri->indices[2];
			IMPgModel_ActiveStripIndices[2] = curTri->indices[0];
			error = IMPiStripify_BuildOneStrip_ForReal(itrTri);
			UUmError_ReturnOnError(error);

			IMPgModel_ActiveStrip_Length = 3;
			IMPgModel_ActiveStripIndices[0] = curTri->indices[2];
			IMPgModel_ActiveStripIndices[1] = curTri->indices[0];
			IMPgModel_ActiveStripIndices[2] = curTri->indices[1];
			error = IMPiStripify_BuildOneStrip_ForReal(itrTri);
			UUmError_ReturnOnError(error);

		}
	}

	if(!done)
	{
		for(itrTri = 0; itrTri < IMPgModel_Strip_Length - 2; itrTri++)
		{
			UUrBitVector_SetBit(IMPgModel_TriUsedBV, IMPgModel_TriIndices[itrTri]);
		}
	}

	*outDone = done;

	return UUcError_None;
}

static UUtError
IMPiStripify_BuildStrips_Longest(
	void)
{
	UUtError	error;
	UUtBool		done;

	UUrBitVector_ClearBitAll(IMPgModel_TriUsedBV, IMPgModel_NumTris);

	IMPgModel_VertexFinalListLength = 0;
	IMPgModel_TriFinalListLength = 0;

	while(1)
	{
		IMPgModel_Strip_Length = 0;
		error = IMPiStripify_BuildOneStrip(&done);
		UUmError_ReturnOnError(error);

		if(done) break;

		if(IMPgModel_VertexFinalListLength >= IMPcModel_MaxTris) UUmError_ReturnOnErrorMsg(UUcError_Generic, "out of strip final mem");

		// copy the current list into the final list
		UUrMemory_MoveFast(
			IMPgModel_TriIndices,
			IMPgModel_TriFinalList + IMPgModel_TriFinalListLength,
			(IMPgModel_Strip_Length - 2) * sizeof(UUtUns32));

		IMPgModel_TriFinalListLength += IMPgModel_Strip_Length - 2;

		UUrMemory_MoveFast(
			IMPgModel_StripIndices,
			IMPgModel_VertexFinalList + IMPgModel_VertexFinalListLength,
			IMPgModel_Strip_Length * sizeof(UUtUns32));

		IMPgModel_VertexFinalList[IMPgModel_VertexFinalListLength] |= 0x80000000;	// signel the beggining of a strip

		IMPgModel_VertexFinalListLength += IMPgModel_Strip_Length;

		gTotal_StripDist[IMPgModel_Strip_Length]++;
		gTotal_NumStrips++;
	}

	return UUcError_None;
}

static UUtError
IMPiModel_Stripify(
	const MXtNode*	inNode)
{
	UUtError			error;
//	AUtSharedEdgeArray*	quadEdgeArray;
	UUtUns16			itrTri;
	M3tTri*				curTri;
	M3tTri*		curTriLarge;
	UUtUns32			tempList[4];

	AUrSharedEdgeArray_Reset(IMPgModel_TriEdgeArray);

	// compute the edge list
		for(itrTri = 0, curTri = IMPgModel_TriList, curTriLarge = IMPgModel_TriEdgeList;
			itrTri < IMPgModel_NumTris;
			itrTri++, curTri++, curTriLarge++)
		{
			tempList[0] = curTri->indices[0];
			tempList[1] = curTri->indices[1];
			tempList[2] = curTri->indices[2];

			error =
				AUrSharedEdgeArray_AddPoly(
					IMPgModel_TriEdgeArray,
					3,
					tempList,
					itrTri,
					curTriLarge->indices);
			UUmError_ReturnOnError(error);
		}

	IMPgModel_EdgeList = AUrSharedEdgeArray_GetList(IMPgModel_TriEdgeArray);
	IMPgModel_TriNumEdges = (UUtUns16)AUrSharedEdgeArray_GetNum(IMPgModel_TriEdgeArray);

	// build the list
		error =
			IMPiStripify_BuildStrips_Longest();
		UUmError_ReturnOnError(error);

	return UUcError_None;
}


static UUtError
IMPiModel_Process_Points(
	const MXtNode	*inNode,
	M3tGeometry		*inGeometry)
{
	UUtError			error;
	UUtUns16			i;

	M3tVector3DArray*		vertexNormalArray;
	M3tTextureCoordArray*	textureCoordArray;
	M3tPoint3DArray*		pointArray;

	M3tBoundingBox_MinMax minMax;
	float				centerX;
	float				centerY;
	float				centerZ;
	float				curX, curY, curZ;

	error =
		TMrConstruction_Instance_NewUnique(
			M3cTemplate_Point3DArray,
			inNode->numPoints,
			&inGeometry->pointArray);
	IMPmError_ReturnOnError(error);

	pointArray = inGeometry->pointArray;

	error =
		TMrConstruction_Instance_NewUnique(
			M3cTemplate_Vector3DArray,
			inNode->numPoints,
			&inGeometry->vertexNormalArray);
	IMPmError_ReturnOnError(error);

	vertexNormalArray = inGeometry->vertexNormalArray;

	error =
		TMrConstruction_Instance_NewUnique(
			M3cTemplate_TextureCoordArray,
			inNode->numPoints + M3cExtraCoords,
			&inGeometry->texCoordArray);
	IMPmError_ReturnOnError(error);

	textureCoordArray = inGeometry->texCoordArray;
	textureCoordArray->numTextureCoords = inNode->numPoints;

	minMax.minPoint.x = minMax.minPoint.y = minMax.minPoint.z = 1e13f;
	minMax.maxPoint.x = minMax.maxPoint.y = minMax.maxPoint.z = -1e13f;

	for(i = 0; i < inNode->numPoints; i++)
	{
		vertexNormalArray->vectors[i] = inNode->points[i].normal;
		textureCoordArray->textureCoords[i] = inNode->points[i].uv;

		pointArray->points[i] = inNode->points[i].point;

		curX = inNode->points[i].point.x;
		curY = inNode->points[i].point.y;
		curZ = inNode->points[i].point.z;

		minMax.minPoint.x = UUmMin(curX, minMax.minPoint.x);
		minMax.minPoint.y = UUmMin(curY, minMax.minPoint.y);
		minMax.minPoint.z = UUmMin(curZ, minMax.minPoint.z);

		minMax.maxPoint.x = UUmMax(curX, minMax.maxPoint.x);
		minMax.maxPoint.y = UUmMax(curY, minMax.maxPoint.y);
		minMax.maxPoint.z = UUmMax(curZ, minMax.maxPoint.z);
	}

	pointArray->minmax_boundingBox = minMax;

	centerX = (minMax.minPoint.x + minMax.maxPoint.x) / 2.f;
	centerY = (minMax.minPoint.y + minMax.maxPoint.y) / 2.f;
	centerZ = (minMax.minPoint.z + minMax.maxPoint.z) / 2.f;

	pointArray->boundingSphere.center.x = centerX;
	pointArray->boundingSphere.center.y = centerY;
	pointArray->boundingSphere.center.z = centerZ;

	pointArray->boundingSphere.radius =
		MUrPoint_Distance(&pointArray->minmax_boundingBox.minPoint,&pointArray->boundingSphere.center);

	UUmAssert(pointArray->boundingSphere.radius < 10000.0f);

	return UUcError_None;
}

static UUtError
IMPiModel_Process_MaterialPoints(
	const MXtNode			*inNode,
	UUtUns16				inMaterialIndex,
	M3tGeometry				*ioGeometry,
	UUtUns16				*ioFromTo)
{
	UUtError				error;
	UUtUns16				i;
	UUtUns16				j;

	UUtUns32				num_points;
	UUtUns16				index;
	UUtUns16				point_index;

	M3tVector3DArray*		vertexNormalArray;
	M3tTextureCoordArray*	textureCoordArray;
	M3tPoint3DArray*		pointArray;

	M3tBoundingBox_MinMax	minMax;
	float					centerX;
	float					centerY;
	float					centerZ;
	float					curX, curY, curZ;

	UUtUns32				*bitvector;

	// calculate the number of points associated with the material
	num_points = 0;
	bitvector = UUrBitVector_New(inNode->numPoints);
	UUrBitVector_ClearBitAll(bitvector, inNode->numPoints);

	for (i = 0; i < inNode->numTriangles; i++)
	{
		if (inNode->triangles[i].material != inMaterialIndex) { continue; }

		for (j = 0; j < 3; j++)
		{
			index = inNode->triangles[i].indices[j];

			if (UUrBitVector_TestBit(bitvector, index) == UUcFalse)
			{
				UUrBitVector_SetBit(bitvector, index);
				num_points++;
			}
		}
	}

	for (i = 0; i < inNode->numQuads; i++)
	{
		if (inNode->quads[i].material != inMaterialIndex) { continue; }

		for (j = 0; j < 4; j++)
		{
			index = inNode->quads[i].indices[j];

			if (UUrBitVector_TestBit(bitvector, index) == UUcFalse)
			{
				UUrBitVector_SetBit(bitvector, index);
				num_points++;
			}
		}
	}

	// create the template instances
	error =
		TMrConstruction_Instance_NewUnique(
			M3cTemplate_Point3DArray,
			num_points,
			&ioGeometry->pointArray);
	IMPmError_ReturnOnError(error);

	pointArray = ioGeometry->pointArray;

	error =
		TMrConstruction_Instance_NewUnique(
			M3cTemplate_Vector3DArray,
			num_points,
			&ioGeometry->vertexNormalArray);
	IMPmError_ReturnOnError(error);

	vertexNormalArray = ioGeometry->vertexNormalArray;

	error =
		TMrConstruction_Instance_NewUnique(
			M3cTemplate_TextureCoordArray,
			num_points + M3cExtraCoords,
			&ioGeometry->texCoordArray);
	IMPmError_ReturnOnError(error);

	textureCoordArray = ioGeometry->texCoordArray;
	textureCoordArray->numTextureCoords = (UUtUns16)num_points;

	// initialize minMax
	minMax.minPoint.x = minMax.minPoint.y = minMax.minPoint.z = 1e13f;
	minMax.maxPoint.x = minMax.maxPoint.y = minMax.maxPoint.z = -1e13f;

	index = 0;
	UUrBitVector_ClearBitAll(bitvector, inNode->numPoints);

	// put all the vertices associated with the triangles into the array
	for (i = 0; i < inNode->numTriangles; i++)
	{
		// if this triangle isn't associated with the material, skip it
		if (inNode->triangles[i].material != inMaterialIndex) { continue; }

		// process the points in the triangle
		for (j = 0; j < 3; j++)
		{
			// get the point index and make sure it hasn't already been processed
			point_index = inNode->triangles[i].indices[j];
			if (UUrBitVector_TestBit(bitvector, point_index) == UUcTrue) { continue; }
			UUrBitVector_SetBit(bitvector, point_index);
			ioFromTo[point_index] = index;

			// copy the point, normal, and texture coordinates
			pointArray->points[index] = inNode->points[point_index].point;
			vertexNormalArray->vectors[index] = inNode->points[point_index].normal;
			textureCoordArray->textureCoords[index] = inNode->points[point_index].uv;

			// adjust the min max points
			curX = inNode->points[point_index].point.x;
			curY = inNode->points[point_index].point.y;
			curZ = inNode->points[point_index].point.z;

			minMax.minPoint.x = UUmMin(curX, minMax.minPoint.x);
			minMax.minPoint.y = UUmMin(curY, minMax.minPoint.y);
			minMax.minPoint.z = UUmMin(curZ, minMax.minPoint.z);

			minMax.maxPoint.x = UUmMax(curX, minMax.maxPoint.x);
			minMax.maxPoint.y = UUmMax(curY, minMax.maxPoint.y);
			minMax.maxPoint.z = UUmMax(curZ, minMax.maxPoint.z);

			// go to next index
			index++;
		}
	}

	// put all the vertices associated with the quads into the array
	for (i = 0; i < inNode->numQuads; i++)
	{
		// if this quad isn't associated with the material, skip it
		if (inNode->quads[i].material != inMaterialIndex) { continue; }

		// process the points in the quad
		for (j = 0; j < 4; j++)
		{
			// get the point index and make sure it hasn't already been processed
			point_index = inNode->quads[i].indices[j];
			if (UUrBitVector_TestBit(bitvector, point_index) == UUcTrue) { continue; }
			UUrBitVector_SetBit(bitvector, point_index);
			ioFromTo[point_index] = index;

			// copy the point, normal, and texture coordinates
			pointArray->points[index] = inNode->points[point_index].point;
			vertexNormalArray->vectors[index] = inNode->points[point_index].normal;
			textureCoordArray->textureCoords[index] = inNode->points[point_index].uv;

			// adjust the min max points
			curX = inNode->points[point_index].point.x;
			curY = inNode->points[point_index].point.y;
			curZ = inNode->points[point_index].point.z;

			minMax.minPoint.x = UUmMin(curX, minMax.minPoint.x);
			minMax.minPoint.y = UUmMin(curY, minMax.minPoint.y);
			minMax.minPoint.z = UUmMin(curZ, minMax.minPoint.z);

			minMax.maxPoint.x = UUmMax(curX, minMax.maxPoint.x);
			minMax.maxPoint.y = UUmMax(curY, minMax.maxPoint.y);
			minMax.maxPoint.z = UUmMax(curZ, minMax.maxPoint.z);

			// go to next index
			index++;
		}
	}

	// save the min max bounding box
	pointArray->minmax_boundingBox = minMax;

	// calculate the center of the bounding sphere
	centerX = (minMax.minPoint.x + minMax.maxPoint.x) / 2.0f;
	centerY = (minMax.minPoint.y + minMax.maxPoint.y) / 2.0f;
	centerZ = (minMax.minPoint.z + minMax.maxPoint.z) / 2.0f;

	pointArray->boundingSphere.center.x = centerX;
	pointArray->boundingSphere.center.y = centerY;
	pointArray->boundingSphere.center.z = centerZ;

	// calculate the radius of the bounding sphere
	pointArray->boundingSphere.radius =
		MUrPoint_Distance(
			&pointArray->minmax_boundingBox.minPoint,
			&pointArray->boundingSphere.center);

	UUmAssert(pointArray->boundingSphere.radius < 10000.0f);

	UUrBitVector_Dispose(bitvector);

	return UUcError_None;
}

static UUtError
IMPiModel_Process_TrisAndQuads(
	const MXtNode		*inNode,
	M3tGeometry			*ioGeometry)
{
	UUtUns32			i, j;
	M3tTri*				curTri;
	M3tVector3D*		curTriNormal;
#if TRI_AREA_TEST
	M3tVector3D*		tri_point[3];
#endif
	UUtUns32*			curTriNormalIndexPtr;
	UUtUns32			curTriNormalIndex = 0;
	UUtBool				recordNormal;


	if(inNode->numTriangles + inNode->numQuads * 2 > IMPcModel_MaxTris) UUmError_ReturnOnErrorMsg(UUcError_Generic, "too many tris");

	curTri = IMPgModel_TriList;
	curTriNormal = IMPgModel_TriNormalList;
	curTriNormalIndexPtr = IMPgModel_TriNormalIndexList;

	for(i = 0; i < inNode->numTriangles; i++)
	{
		M3tVector3D this_triangles_normal;

		MUrVector_NormalFromPoints(
			ioGeometry->pointArray->points + inNode->triangles[i].indices[0],
			ioGeometry->pointArray->points + inNode->triangles[i].indices[1],
			ioGeometry->pointArray->points + inNode->triangles[i].indices[2],
			&this_triangles_normal);

		if (MUrVector_DotProduct(&inNode->triangles[i].dont_use_this_normal, &this_triangles_normal) < 0.f) {
			MUmVector_Negate(this_triangles_normal);
		}

		for(j = 0; j < 3; j++)
		{
			curTri->indices[j] = (UUtUns16) inNode->triangles[i].indices[j];
			UUmAssert(curTri->indices[j] != UUcMaxUns16);
#if TRI_AREA_TEST
			tri_point[j] = &ioGeometry->pointArray->points[curTri->indices[j]];
#endif
		}

		// don't import degenerate triangles
		if(!(curTri->indices[0] == curTri->indices[1] ||
			 curTri->indices[0] == curTri->indices[2] ||
			 curTri->indices[1] == curTri->indices[2])) {

#if TRI_AREA_TEST
			// don't import triangles with zero area
			if (MUrTriangle_Area(tri_point[0], tri_point[1], tri_point[2]) > IMPcModel_MinTriArea) {
#endif
				*curTriNormal++ = this_triangles_normal;
				*curTriNormalIndexPtr++ = curTriNormalIndex;
				curTri++;
				curTriNormalIndex++;
#if TRI_AREA_TEST
			}
#endif
		}
	}

	for(i = 0; i < inNode->numQuads; i++)
	{
		M3tVector3D this_quads_normal;

		MUrVector_NormalFromPoints(
			ioGeometry->pointArray->points + inNode->quads[i].indices[0],
			ioGeometry->pointArray->points + inNode->quads[i].indices[1],
			ioGeometry->pointArray->points + inNode->quads[i].indices[2],
			&this_quads_normal);

		if (MUrVector_DotProduct(&inNode->quads[i].dont_use_this_normal, &this_quads_normal) < 0.f) {
			MUmVector_Negate(this_quads_normal);
		}

		*curTriNormal = this_quads_normal;

		curTri->indices[0] = (UUtUns16) inNode->quads[i].indices[0];
		curTri->indices[1] = (UUtUns16) inNode->quads[i].indices[1];
		curTri->indices[2] = (UUtUns16) inNode->quads[i].indices[2];

		UUmAssert(curTri->indices[0] != UUcMaxUns16);
		UUmAssert(curTri->indices[1] != UUcMaxUns16);
		UUmAssert(curTri->indices[2] != UUcMaxUns16);

#if TRI_AREA_TEST
		tri_point[0] = &ioGeometry->pointArray->points[curTri->indices[0]];
		tri_point[1] = &ioGeometry->pointArray->points[curTri->indices[1]];
		tri_point[2] = &ioGeometry->pointArray->points[curTri->indices[2]];
#endif

		recordNormal = UUcFalse;

		// don't import degenerate triangles
		if(!(curTri->indices[0] == curTri->indices[1] ||
			 curTri->indices[0] == curTri->indices[2] ||
			 curTri->indices[1] == curTri->indices[2])) {

#if TRI_AREA_TEST
			// don't import triangles with zero area
			if (MUrTriangle_Area(tri_point[0], tri_point[1], tri_point[2]) > IMPcModel_MinTriArea) {
#endif
				*curTriNormalIndexPtr++ = curTriNormalIndex;
				curTri++;
				recordNormal = UUcTrue;
#if TRI_AREA_TEST
			}
#endif
		}

		curTri->indices[0] = (UUtUns16) inNode->quads[i].indices[0];
		curTri->indices[1] = (UUtUns16) inNode->quads[i].indices[2];
		curTri->indices[2] = (UUtUns16) inNode->quads[i].indices[3];

		UUmAssert(curTri->indices[0] != UUcMaxUns16);
		UUmAssert(curTri->indices[1] != UUcMaxUns16);
		UUmAssert(curTri->indices[2] != UUcMaxUns16);

#if TRI_AREA_TEST
		tri_point[0] = &ioGeometry->pointArray->points[curTri->indices[0]];
		tri_point[1] = &ioGeometry->pointArray->points[curTri->indices[1]];
		tri_point[2] = &ioGeometry->pointArray->points[curTri->indices[2]];
#endif

		recordNormal = UUcFalse;

		// don't import degenerate triangles
		if(!(curTri->indices[0] == curTri->indices[1] ||
			 curTri->indices[0] == curTri->indices[2] ||
			 curTri->indices[1] == curTri->indices[2])) {
#if TRI_AREA_TEST
			// don't import triangles with zero area
			if (MUrTriangle_Area(tri_point[0], tri_point[1], tri_point[2]) > IMPcModel_MinTriArea) {
#endif
				*curTriNormalIndexPtr++ = curTriNormalIndex;
				curTri++;
				recordNormal = UUcTrue;
#if TRI_AREA_TEST
			}
#endif
		}

		if(recordNormal == UUcTrue)
		{
			*curTriNormal++ = this_quads_normal;
			curTriNormalIndex++;
		}
	}

	IMPgModel_NumTris = curTri - IMPgModel_TriList;
	gTotal_NumTris += IMPgModel_NumTris;

	IMPgModel_NumTriNormals = curTriNormalIndex;

	return UUcError_None;
}

static UUtError
IMPiModel_Process_MaterialTrisAndQuads(
	const MXtNode		*inNode,
	UUtUns16			inMaterialIndex,
	UUtUns16			*inFromTo,
	M3tGeometry			*ioGeometry)
{
	UUtUns32			i, j;
	M3tTri*				curTri;
	M3tVector3D*		curTriNormal;
#if TRI_AREA_TEST
	M3tVector3D*		tri_point[3];
#endif
	UUtUns32*			curTriNormalIndexPtr;
	UUtUns32			curTriNormalIndex = 0;
	UUtBool				recordNormal;


	if (inNode->numTriangles + inNode->numQuads * 2 > IMPcModel_MaxTris)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "too many tris");
	}

	curTri = IMPgModel_TriList;
	curTriNormal = IMPgModel_TriNormalList;
	curTriNormalIndexPtr = IMPgModel_TriNormalIndexList;

	for (i = 0; i < inNode->numTriangles; i++)
	{
		M3tVector3D this_triangles_normal;

		// only process triangles associated with inMaterialIndex
		if (inNode->triangles[i].material != inMaterialIndex) { continue; }

		MUrVector_NormalFromPoints(
			ioGeometry->pointArray->points + inFromTo[inNode->triangles[i].indices[0]],
			ioGeometry->pointArray->points + inFromTo[inNode->triangles[i].indices[1]],
			ioGeometry->pointArray->points + inFromTo[inNode->triangles[i].indices[2]],
			&this_triangles_normal);

		if (MUrVector_DotProduct(&inNode->triangles[i].dont_use_this_normal, &this_triangles_normal) < 0.f) {
			MUmVector_Negate(this_triangles_normal);
		}

		for(j = 0; j < 3; j++)
		{
			curTri->indices[j] = inFromTo[inNode->triangles[i].indices[j]];
			UUmAssert(curTri->indices[j] != UUcMaxUns16);
#if TRI_AREA_TEST
			tri_point[j] = &ioGeometry->pointArray->points[curTri->indices[j]];
#endif
		}

		// don't import degenerate triangles
		if(!(curTri->indices[0] == curTri->indices[1] ||
			 curTri->indices[0] == curTri->indices[2] ||
			 curTri->indices[1] == curTri->indices[2])) {

#if TRI_AREA_TEST
			// don't import triangles with zero area
			if (MUrTriangle_Area(tri_point[0], tri_point[1], tri_point[2]) > IMPcModel_MinTriArea) {
#endif

				*curTriNormal++ = this_triangles_normal;
				*curTriNormalIndexPtr++ = curTriNormalIndex;
				curTri++;
				curTriNormalIndex++;
#if TRI_AREA_TEST
			}
#endif
		}
	}

	for (i = 0; i < inNode->numQuads; i++)
	{
		M3tVector3D this_quads_normal;

		// only process quads associated with inMaterialIndex
		if (inNode->quads[i].material != inMaterialIndex) { continue; }

		MUrVector_NormalFromPoints(
			ioGeometry->pointArray->points + inFromTo[inNode->quads[i].indices[0]],
			ioGeometry->pointArray->points + inFromTo[inNode->quads[i].indices[1]],
			ioGeometry->pointArray->points + inFromTo[inNode->quads[i].indices[2]],
			&this_quads_normal);

		if (MUrVector_DotProduct(&inNode->quads[i].dont_use_this_normal, &this_quads_normal) < 0.f) {
			MUmVector_Negate(this_quads_normal);
		}

		*curTriNormal = this_quads_normal;

		curTri->indices[0] = inFromTo[inNode->quads[i].indices[0]];
		curTri->indices[1] = inFromTo[inNode->quads[i].indices[1]];
		curTri->indices[2] = inFromTo[inNode->quads[i].indices[2]];

		UUmAssert(curTri->indices[0] != UUcMaxUns16);
		UUmAssert(curTri->indices[1] != UUcMaxUns16);
		UUmAssert(curTri->indices[2] != UUcMaxUns16);

#if TRI_AREA_TEST
		tri_point[0] = &ioGeometry->pointArray->points[curTri->indices[0]];
		tri_point[1] = &ioGeometry->pointArray->points[curTri->indices[1]];
		tri_point[2] = &ioGeometry->pointArray->points[curTri->indices[2]];
#endif

		recordNormal = UUcFalse;

		// don't import degenerate triangles
		if(!(curTri->indices[0] == curTri->indices[1] ||
			 curTri->indices[0] == curTri->indices[2] ||
			 curTri->indices[1] == curTri->indices[2])) {

#if TRI_AREA_TEST
			// don't import triangles with zero area
			if (MUrTriangle_Area(tri_point[0], tri_point[1], tri_point[2]) > IMPcModel_MinTriArea) {
#endif
				*curTriNormalIndexPtr++ = curTriNormalIndex;
				curTri++;
				recordNormal = UUcTrue;
#if TRI_AREA_TEST
			}
#endif
		}

		curTri->indices[0] = inFromTo[inNode->quads[i].indices[0]];
		curTri->indices[1] = inFromTo[inNode->quads[i].indices[2]];
		curTri->indices[2] = inFromTo[inNode->quads[i].indices[3]];

		UUmAssert(curTri->indices[0] != UUcMaxUns16);
		UUmAssert(curTri->indices[1] != UUcMaxUns16);
		UUmAssert(curTri->indices[2] != UUcMaxUns16);

#if TRI_AREA_TEST
		tri_point[0] = &ioGeometry->pointArray->points[curTri->indices[0]];
		tri_point[1] = &ioGeometry->pointArray->points[curTri->indices[1]];
		tri_point[2] = &ioGeometry->pointArray->points[curTri->indices[2]];
#endif

		// don't import degenerate triangles
		if(!(curTri->indices[0] == curTri->indices[1] ||
			 curTri->indices[0] == curTri->indices[2] ||
			 curTri->indices[1] == curTri->indices[2])) {

#if TRI_AREA_TEST
			// don't import triangles with zero area
			if (MUrTriangle_Area(tri_point[0], tri_point[1], tri_point[2]) > IMPcModel_MinTriArea) {
#endif
				*curTriNormalIndexPtr++ = curTriNormalIndex;
				curTri++;
				recordNormal = UUcTrue;
#if TRI_AREA_TEST
			}
#endif
		}

		if(recordNormal == UUcTrue)
		{
			*curTriNormal++ = this_quads_normal;
			curTriNormalIndex++;
		}
	}

	IMPgModel_NumTris = curTri - IMPgModel_TriList;
	gTotal_NumTris += IMPgModel_NumTris;

	IMPgModel_NumTriNormals = curTriNormalIndex;

	return UUcError_None;
}

static UUtError
IMPiModel_Finish(
	M3tGeometry		*ioGeometry)
{
	UUtError		error;
	UUtUns16		itr;
	//UUtUns16*		curIndexPtr;
	//UUtUns16		index0, index1, index2;
	//M3tPoint3D*		pointArray;

	error =
		TMrConstruction_Instance_NewUnique(
			M3cTemplate_Vector3DArray,
			IMPgModel_NumTriNormals,
			&ioGeometry->triNormalArray);
	UUmError_ReturnOnError(error);

	for(itr = 0; itr < IMPgModel_NumTriNormals; itr++)
	{
		ioGeometry->triNormalArray->vectors[itr] = IMPgModel_TriNormalList[itr];
	}

	error =
		TMrConstruction_Instance_NewUnique(
			TMcTemplate_IndexArray,
			IMPgModel_VertexFinalListLength,
			&ioGeometry->triStripArray);
	UUmError_ReturnOnError(error);

	UUrMemory_MoveFast(IMPgModel_VertexFinalList, ioGeometry->triStripArray->indices, IMPgModel_VertexFinalListLength * sizeof(UUtUns32));

	error =
		TMrConstruction_Instance_NewUnique(
			TMcTemplate_IndexArray,
			IMPgModel_TriFinalListLength,
			&ioGeometry->triNormalIndexArray);
	UUmError_ReturnOnError(error);

	//curIndexPtr = ioGeometry->triStripArray->indices;
	//pointArray = ioGeometry->pointArray->points;

	for(itr = 0; itr < IMPgModel_TriFinalListLength; itr++)
	{
		ioGeometry->triNormalIndexArray->indices[itr] = IMPgModel_TriNormalIndexList[IMPgModel_TriFinalList[itr]];

		#if 0
		{
			M3tVector3D		computedNormal;
			M3tVector3D*	realNormal;

			index2 = *curIndexPtr++;

			if(index2 & 0x80000000)
			{
				index0 = (index2 & 0x7FFF);
				index1 = *curIndexPtr++;
				index2 = *curIndexPtr++;
			}

			MUrVector_NormalFromPoints(
				pointArray + index0,
				pointArray + index1,
				pointArray + index2,
				&computedNormal);

			realNormal = ioGeometry->triNormalArray->vectors + ioGeometry->triNormalIndexArray->indices[itr];

			UUmAssert(computedNormal.x == realNormal->x);

			index0 = index1;
			index1 = index2;
		}
		#endif

	}

	return UUcError_None;
}

UUtError Imp_Node_To_Geometry(
	const MXtNode	*inNode,
	M3tGeometry		*outGeometry)
{
	UUtError error;

	UUmAssertReadPtr(inNode, sizeof(*inNode));
	UUmAssertWritePtr(outGeometry, sizeof(*outGeometry));

	UUrMemory_Clear(outGeometry, sizeof(*outGeometry));

	gTotal_NumObjects++;

	outGeometry->animation = NULL;

	error = IMPiModel_Process_Points(inNode, outGeometry);
	IMPmError_ReturnOnError(error);
	error = IMPiModel_Process_TrisAndQuads(inNode, outGeometry);
	IMPmError_ReturnOnError(error);

	IMPiModel_Stripify(inNode);

	error = IMPiModel_Finish(outGeometry);

	return UUcError_None;
}

UUtError Imp_NodeMaterial_To_Geometry(
	const MXtNode	*inNode,
	UUtUns16		inMaterialIndex,
	M3tGeometry		*outGeometry)
{
	UUtError 		error;
	UUtUns16		*from_to;

	UUmAssertReadPtr(inNode, sizeof(*inNode));
	UUmAssertWritePtr(outGeometry, sizeof(*outGeometry));

	UUrMemory_Clear(outGeometry, sizeof(*outGeometry));

	gTotal_NumObjects++;

	outGeometry->animation = NULL;

	// allocate memory for the from_to array
	from_to = UUrMemory_Block_New(sizeof(UUtUns16) * inNode->numPoints);
	UUmError_ReturnOnNull(from_to);
	UUrMemory_Set16(from_to, UUcMaxUns16, sizeof(UUtUns16) * inNode->numPoints);

	error = IMPiModel_Process_MaterialPoints(inNode, inMaterialIndex, outGeometry, from_to);
	IMPmError_ReturnOnError(error);
	error = IMPiModel_Process_MaterialTrisAndQuads(inNode, inMaterialIndex, from_to, outGeometry);
	IMPmError_ReturnOnError(error);

	// dispose of the used memory
	UUrMemory_Block_Delete(from_to);

	IMPiModel_Stripify(inNode);

	error = IMPiModel_Finish(outGeometry);

	return UUcError_None;
}

static void
Imp_RotateBoundBox(
	const MXtNode			*inNode,
	M3tGeometry				*ioGeometry)
{
	M3tBoundingBox_MinMax	*min_max;
	M3tBoundingBox			bbox;
	UUtUns32				i;

	min_max = &ioGeometry->pointArray->minmax_boundingBox;
	M3rMinMaxBBox_To_BBox(min_max, &bbox);

	for (i = 0; i < M3cNumBoundingPoints; i++)
	{
		MUrMatrix_MultiplyPoint(&bbox.localPoints[i], &inNode->matrix, &bbox.localPoints[i]);
	}

	MUmVector_Set(min_max->minPoint, 1e13f, 1e13f, 1e13f);
	MUmVector_Set(min_max->maxPoint, -1e13f, -1e13f, -1e13f);

	for (i = 0; i < M3cNumBoundingPoints; i++)
	{
		min_max->minPoint.x = UUmMin(min_max->minPoint.x, bbox.localPoints[i].x);
		min_max->minPoint.y = UUmMin(min_max->minPoint.y, bbox.localPoints[i].y);
		min_max->minPoint.z = UUmMin(min_max->minPoint.z, bbox.localPoints[i].z);

		min_max->maxPoint.x = UUmMax(min_max->maxPoint.x, bbox.localPoints[i].x);
		min_max->maxPoint.y = UUmMax(min_max->maxPoint.y, bbox.localPoints[i].y);
		min_max->maxPoint.z = UUmMax(min_max->maxPoint.z, bbox.localPoints[i].z);
	}
}

UUtError Imp_Node_To_Geometry_ApplyMatrix(
	const MXtNode	*inNode,
	M3tGeometry		*outGeometry)
{
	UUtError error;

	UUmAssertReadPtr(inNode, sizeof(*inNode));
	UUmAssertWritePtr(outGeometry, sizeof(*outGeometry));

	error = Imp_Node_To_Geometry(inNode, outGeometry);
	UUmError_ReturnOnError(error);

	MUrMatrix_MultiplyPoints(
		outGeometry->pointArray->numPoints,
		&inNode->matrix,
		outGeometry->pointArray->points,
		outGeometry->pointArray->points);

	MUrMatrix_MultiplyNormals(
		outGeometry->triNormalArray->numVectors,
		&inNode->matrix,
		outGeometry->triNormalArray->vectors,
		outGeometry->triNormalArray->vectors);

	MUrMatrix_MultiplyNormals(
		outGeometry->vertexNormalArray->numVectors,
		&inNode->matrix,
		outGeometry->vertexNormalArray->vectors,
		outGeometry->vertexNormalArray->vectors);

	Imp_RotateBoundBox(inNode, outGeometry);

	MUrMatrix_MultiplyPoint(
		&outGeometry->pointArray->boundingSphere.center,
		&inNode->matrix,
		&outGeometry->pointArray->boundingSphere.center);

	return UUcError_None;
}

UUtError Imp_NodeMaterial_To_Geometry_ApplyMatrix(
	const MXtNode	*inNode,
	UUtUns16		inMaterialIndex,
	M3tGeometry		*outGeometry)
{
	UUtError error;

	UUmAssertReadPtr(inNode, sizeof(*inNode));
	UUmAssertWritePtr(outGeometry, sizeof(*outGeometry));

	error = Imp_NodeMaterial_To_Geometry(inNode, inMaterialIndex, outGeometry);
	UUmError_ReturnOnError(error);

	MUrMatrix_MultiplyPoints(
		outGeometry->pointArray->numPoints,
		&inNode->matrix,
		outGeometry->pointArray->points,
		outGeometry->pointArray->points);

	MUrMatrix_MultiplyNormals(
		outGeometry->triNormalArray->numVectors,
		&inNode->matrix,
		outGeometry->triNormalArray->vectors,
		outGeometry->triNormalArray->vectors);

	MUrMatrix_MultiplyNormals(
		outGeometry->vertexNormalArray->numVectors,
		&inNode->matrix,
		outGeometry->vertexNormalArray->vectors,
		outGeometry->vertexNormalArray->vectors);

	Imp_RotateBoundBox(inNode, outGeometry);

	MUrMatrix_MultiplyPoint(
		&outGeometry->pointArray->boundingSphere.center,
		&inNode->matrix,
		&outGeometry->pointArray->boundingSphere.center);

	return UUcError_None;
}

UUtError
Imp_AddModel(
	BFtFileRef*			inSourceFileRef,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;
	M3tGeometry*		geometry;
	UUtBool				buildInstance;
	char				*textureName;
	UUtUns32			geometryFlags;

	BFtFileRef*			fileRef = NULL;
	MXtHeader			*header = NULL;

	UUtBool				noMatrix;
	char				mungedTextureName[BFcMaxFileNameLength];

	error =
		Imp_Common_BuildInstance(
			inSourceFileRef,
			inSourceFileModDate,
			inGroup,
			M3cTemplate_Geometry,
			inInstanceName,
			gModelCompileTime,
			&fileRef,
			&buildInstance);
	IMPmError_ReturnOnError(error);

	if (buildInstance != UUcTrue) {
		goto exit;
	}

	error = Imp_ParseEnvFile(fileRef, &header);
	IMPmError_ReturnOnError(error);

	error =
		TMrConstruction_Instance_Renew(
			M3cTemplate_Geometry,
			inInstanceName,
			0,
			&geometry);
	IMPmError_ReturnOnError(error);

	geometry->animation = NULL;

	if (header->numNodes > 1) {
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "model files can only have one node!");
	}

	error = GRrGroup_GetBool(inGroup, "noMatrix", &noMatrix);
	if (UUcError_None != error) {
		noMatrix = UUcFalse;
	}

	if (noMatrix)
	{
		Imp_Node_To_Geometry(header->nodes + 0, geometry);
	}
	else
	{
		Imp_Node_To_Geometry_ApplyMatrix(header->nodes + 0, geometry);
	}

	// Add the texture
	error = GRrGroup_GetString(inGroup, "texture", &textureName);
	if (error != UUcError_None) {
		textureName = header->nodes[0].materials[0].maps[MXcMapping_DI].name;
	}

	// strip off the extension
	UUrString_Copy(mungedTextureName, textureName, BFcMaxFileNameLength);
	UUrString_StripExtension(mungedTextureName);

	UUmAssert(strchr(mungedTextureName, '.') == NULL);

	geometry->baseMap = M3rTextureMap_GetPlaceholder(mungedTextureName);
	UUmAssert(geometry->baseMap);

	error = GRrGroup_GetUns32(inGroup, "flags", &geometryFlags);
	if (UUcError_None != error) {
		geometryFlags = 0;
	}

	geometry->geometryFlags = (M3tGeometryFlags) geometryFlags;

exit:
	if (NULL != fileRef) {
		BFrFileRef_Dispose(fileRef);
	}

	if (NULL != header) {
		Imp_EnvFile_Delete(header);
	}

	return UUcError_None;
}

UUtError
Imp_AddModelArray(
	BFtFileRef*			inSourceFileRef,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError error;
	GRtElementType	elementType;
	UUtUns16 itr;
	UUtUns32 numModels;
	GRtElementArray* modelList;
	M3tGeometryArray* geometryArray;

	Imp_PrintMessage(IMPcMsg_Cosmetic, "\tbuilding..."UUmNL);

	// open our group array
	error = GRrGroup_GetElement(
				inGroup,
				"modelList",
				&elementType,
				&modelList);

	if ((error != UUcError_None) || (elementType != GRcElementType_Array))
	{
		Imp_PrintWarning("Could not get model list group");
		return UUcError_Generic;
	}

	numModels = GRrGroup_Array_GetLength(modelList);

	// create our instance
	error = TMrConstruction_Instance_Renew(
				M3cTemplate_GeometryArray,
				inInstanceName,
				numModels,
				&geometryArray);
	IMPmError_ReturnOnError(error);

	for(itr = 0; itr < numModels; itr++)
	{
		char *instanceTag;
		M3tGeometry *model;

		error = GRrGroup_Array_GetElement(modelList, itr, &elementType,	&instanceTag);
		if (elementType != GRcElementType_String) { error = UUcError_Generic; }
		IMPmError_ReturnOnErrorMsg(error, "Imp_AddModelArray: could not get the tag");

		error = TMrConstruction_Instance_GetPlaceHolder(
			M3cTemplate_Geometry,
			instanceTag,
			(TMtPlaceHolder*)&model);
		IMPmError_ReturnOnErrorMsg(error, "Imp_AddAnimationCollection: could not find the animation");

		geometryArray->geometries[itr] = model;
	}

	return UUcError_None;
}

UUtError
IMPrModel_Initialize(
	void)
{
	IMPgModel_TriEdgeArray = AUrSharedEdgeArray_New();
	UUmError_ReturnOnNull(IMPgModel_TriEdgeArray);

	IMPgModel_TriEdgeList = UUrMemory_Block_New(IMPcModel_MaxTris * sizeof(M3tTri));
	UUmError_ReturnOnNull(IMPgModel_TriEdgeList);

	IMPgModel_VertexFinalList = UUrMemory_Block_New(IMPcModel_MaxTris * sizeof(IMPtModel_Tri));
	UUmError_ReturnOnNull(IMPgModel_VertexFinalList);

	IMPgModel_TriUsedBV = UUrBitVector_New(IMPcModel_MaxTris);
	UUmError_ReturnOnNull(IMPgModel_TriUsedBV);

	IMPgModel_TriNormalList = UUrMemory_Block_New(IMPcModel_MaxTris * sizeof(M3tVector3D));
	UUmError_ReturnOnNull(IMPgModel_TriNormalList);

	IMPgModel_TriList = UUrMemory_Block_New(IMPcModel_MaxTris * sizeof(M3tTri));
	UUmError_ReturnOnNull(IMPgModel_TriList);

	IMPgModel_TriNormalIndexList = UUrMemory_Block_New(IMPcModel_MaxTris * sizeof(UUtUns16));
	UUmError_ReturnOnNull(IMPgModel_TriNormalIndexList);

	IMPgModel_TriFinalList = UUrMemory_Block_New(IMPcModel_MaxTris * sizeof(UUtUns16));
	UUmError_ReturnOnNull(IMPgModel_TriFinalList);

	return UUcError_None;
}

void
IMPrModel_Terminate(
	void)
{

	fprintf(stderr, "tris/strips %d / %d = %f\n", gTotal_NumTris, gTotal_NumStrips, (float)gTotal_NumTris / (float)gTotal_NumStrips);
	fprintf(stderr, "tris/obj %d / %d = %f\n", gTotal_NumTris, gTotal_NumObjects, (float)gTotal_NumTris / (float)gTotal_NumObjects);
	fprintf(stderr, "strips/obj %d / %d = %f\n", gTotal_NumStrips, gTotal_NumObjects, (float)gTotal_NumStrips / (float)gTotal_NumObjects);

	#if 0
	{
		UUtUns16	itr;
		for(itr = 0; itr < IMPcModel_MaxStripLength; itr++)
		{
			fprintf(stderr, "stripDist[%04d] = %04d\n", itr, gTotal_StripDist[itr]);
		}
	}
	#endif

	AUrSharedEdgeArray_Delete(IMPgModel_TriEdgeArray);
	UUrMemory_Block_Delete(IMPgModel_TriEdgeList);
	UUrMemory_Block_Delete(IMPgModel_VertexFinalList);
	UUrBitVector_Dispose(IMPgModel_TriUsedBV);

	UUrMemory_Block_Delete(IMPgModel_TriNormalList);
	UUrMemory_Block_Delete(IMPgModel_TriList);
	UUrMemory_Block_Delete(IMPgModel_TriNormalIndexList);
	UUrMemory_Block_Delete(IMPgModel_TriFinalList);

}
