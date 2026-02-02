/*
	FILE:	Imp_ParseEnvFile.c

	AUTHOR:	Brent H. Pease, Michael Evans

	CREATED: Nov 17, 1998

	PURPOSE:

	Copyright 1998

*/


#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_TemplateManager.h"
#include "BFW_TM_Construction.h"
#include "BFW_Group.h"
#include "BFW_MathLib.h"
#include "BFW_Motoko.h"
#include "BFW_AppUtilities.h"
#include "EulerAngles.h"
#include "ONI_Character.h"
#include "Imp_Common.h"
#include "Imp_Character.h"
#include "Imp_Model.h"
#include "Imp_Texture.h"
#include "Imp_AI_HHT.h"
#include "EnvFileFormat.h"

#include "Imp_ParseEnvFile.h"

static UUtError iParseMXtMarker(char **ioPtr, MXtMarker *outMarker);
static UUtError iParseMXtNode(const char *inFileName, char **ioPtr, MXtNode *outNode);
static void		iMXtHeader_BuildHierarchy(MXtHeader	*ioHeader);
static void		iClampTextureNames(MXtNode *outNode);

void iClampTextureNames(MXtNode *ioNode)
{
	UUtUns32 materialItr;
	UUtUns32 count = ioNode->numMaterials;
	char *internal = NULL;
	MXtMaterial *curMaterial;

	for(materialItr = 0, curMaterial = ioNode->materials; materialItr < count; curMaterial++, materialItr++)
	{
		UUtUns32 mapItr;

		for(mapItr = 0; mapItr < MXcMapping_Count; mapItr++)
		{
			char *pString = curMaterial->maps[mapItr].name;

			for(pString = curMaterial->maps[mapItr].name; *pString != '\0'; pString++)
			{
				if ('.' == *pString) {
					*pString = '\0';
					break;
				}
			}
		}
	}

	return;
}

UUtError
Imp_ParseEnvFile(
	BFtFileRef*			inFileRef,
	MXtHeader*			*outHeader)
{
	UUtUns16 	nodeItr;
	UUtError	error;
	MXtHeader*	newHeader;
	char*		curPtr;
	UUtUns32	length;

	error =
		BFrFileRef_LoadIntoMemory(
			inFileRef,
			&length,
			&newHeader);

	if (UUcError_None != error) {
		Imp_PrintWarning("failed to open file %s", BFrFileRef_GetLeafName(inFileRef));

		return UUcError_Generic;
	}

	#if UUmEndian == UUmEndian_Big

		UUmSwapLittle_4Byte(&newHeader->version);
		UUmSwapLittle_2Byte(&newHeader->numNodes);

	#endif

	if(strncmp(newHeader->stringENVF, "ENVF", 4))
	{
		UUmError_ReturnOnErrorMsgP(
			UUcError_Generic,
			"file %s is not a binary file",
			(UUtUns32) BFrFileRef_GetLeafName(inFileRef),
			0,
			0);
	}

	if(newHeader->version != 0) {
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "wrong version");
	}

	if (0 == newHeader->numNodes) {
		newHeader->nodes = NULL;
		Imp_PrintWarning("This file (%s) had 0 nodes.", BFrFileRef_GetLeafName(inFileRef));
	}
	else {
		newHeader->nodes = UUrMemory_Block_New(sizeof(MXtNode) * newHeader->numNodes);
		UUmError_ReturnOnNull(newHeader->nodes);
	}

	curPtr = (char*)newHeader + sizeof(MXtHeader);

	for(nodeItr = 0; nodeItr < newHeader->numNodes; nodeItr++)
	{
		error = iParseMXtNode(BFrFileRef_GetLeafName(inFileRef), &curPtr, newHeader->nodes + nodeItr);
		UUmError_ReturnOnError(error);
	}

	iMXtHeader_BuildHierarchy(newHeader);

	*outHeader = newHeader;

	if (IMPgBuildTextFiles)
	{
		BFtFileRef* newFileRef;
		const char *leafName = BFrFileRef_GetLeafName(inFileRef);
		char newName[256];

		UUrString_Copy(newName, leafName, 256);

		newName[strlen(leafName) - 4] = '_';

		UUrString_Cat(newName, ".txt", 256);

		error = BFrFileRef_DuplicateAndReplaceName(
			inFileRef,
			newName,
			&newFileRef);

		if (error != UUcError_None) {
			AUrMessageBox(AUcMBType_OK,
				"Could not make file ref for %s\n",
				newName);
		}

		if (UUcError_None == error) {
			error = Imp_WriteTextEnvFile(newFileRef, newHeader);

			BFrFileRef_Dispose(newFileRef);
		}
	}

	return UUcError_None;
}

static UUtError iParseMXtNode(const char *inFileName, char	**ioPtr, MXtNode *outNode)
{
	UUtError error;
	UUtUns16 itr;
	MXtFace*		curFace;
	char*			curPtr;
	MXtPoint*		curPoint;

	#if UUmEndian == UUmEndian_Big
		UUtUns16		itr2;
		float*			p;
		MXtMaterial*	curMaterial;
		MXtMapping*		curMap;
	#endif

	curFace;

	UUmAssertReadPtr(outNode, sizeof(*outNode));


	curPtr = *ioPtr;

	*outNode = *(MXtNode*)curPtr;

	curPtr += sizeof(MXtNode);

	#if UUmEndian == UUmEndian_Big

		p = (float*)&outNode->matrix;
		for(itr = 0; itr < sizeof(M3tMatrix4x3) / sizeof(float); itr++)
		{
			UUmSwapLittle_4Byte(p);
			p++;
		}

		UUmSwapLittle_2Byte(&outNode->numPoints);
		UUmSwapLittle_2Byte(&outNode->numTriangles);
		UUmSwapLittle_2Byte(&outNode->numQuads);
		UUmSwapLittle_2Byte(&outNode->numMarkers);
		UUmSwapLittle_2Byte(&outNode->numMaterials);
		UUmSwapLittle_4Byte(&outNode->userDataCount);

	#endif

	outNode->userData = curPtr;
	curPtr += sizeof(char) * outNode->userDataCount;

	outNode->points = (MXtPoint*)curPtr;
	curPtr += sizeof(MXtPoint) * outNode->numPoints;

	outNode->triangles = (MXtFace*)curPtr;
	curPtr += sizeof(MXtFace) * outNode->numTriangles;

	outNode->quads = (MXtFace*)curPtr;
	curPtr += sizeof(MXtFace) * outNode->numQuads;

	if(outNode->numMarkers > 0)
	{
		outNode->markers = UUrMemory_Block_New(sizeof(MXtMarker) * outNode->numMarkers);
		UUmError_ReturnOnNull(outNode->markers);

		for(itr = 0; itr < outNode->numMarkers; itr++)
		{
			error = iParseMXtMarker(&curPtr, outNode->markers + itr);
			UUmError_ReturnOnError(error);
		}
	}

	UUmAssertReadPtr(outNode->points, sizeof(M3tPoint3D) * outNode->numPoints);

	outNode->materials = (MXtMaterial*)curPtr;
	curPtr += sizeof(MXtMaterial) * outNode->numMaterials;

	for(itr = 0, curPoint = outNode->points;
		itr < outNode->numPoints;
		itr++, curPoint++)
	{
		UUmSwapLittle_4Byte(&curPoint->point.x);
		UUmSwapLittle_4Byte(&curPoint->point.y);
		UUmSwapLittle_4Byte(&curPoint->point.z);
		UUmSwapLittle_4Byte(&curPoint->normal.x);
		UUmSwapLittle_4Byte(&curPoint->normal.y);
		UUmSwapLittle_4Byte(&curPoint->normal.z);
		UUmSwapLittle_4Byte(&curPoint->uv.u);
		UUmSwapLittle_4Byte(&curPoint->uv.v);

		curPoint->uv.v = 1.f - curPoint->uv.v;
	}

	#if UUmEndian == UUmEndian_Big

		for(itr = 0, curFace = outNode->triangles;
			itr < outNode->numTriangles;
			itr++, curFace++)
		{
			UUmSwapLittle_2Byte(&curFace->indices[0]);
			UUmSwapLittle_2Byte(&curFace->indices[1]);
			UUmSwapLittle_2Byte(&curFace->indices[2]);
			UUmSwapLittle_2Byte(&curFace->indices[3]);

			UUmSwapLittle_4Byte(&curFace->dont_use_this_normal.x);
			UUmSwapLittle_4Byte(&curFace->dont_use_this_normal.y);
			UUmSwapLittle_4Byte(&curFace->dont_use_this_normal.z);

			UUmSwapLittle_2Byte(&curFace->material);
		}

		for(itr = 0, curFace = outNode->quads;
			itr < outNode->numQuads;
			itr++, curFace++)
		{
			UUmSwapLittle_2Byte(&curFace->indices[0]);
			UUmSwapLittle_2Byte(&curFace->indices[1]);
			UUmSwapLittle_2Byte(&curFace->indices[2]);
			UUmSwapLittle_2Byte(&curFace->indices[3]);

			UUmSwapLittle_4Byte(&curFace->dont_use_this_normal.x);
			UUmSwapLittle_4Byte(&curFace->dont_use_this_normal.y);
			UUmSwapLittle_4Byte(&curFace->dont_use_this_normal.z);

			UUmSwapLittle_2Byte(&curFace->material);
		}

		for(itr = 0, curMaterial = outNode->materials;
			itr < outNode->numMaterials;
			itr++, curMaterial++)
		{
			UUmSwapLittle_4Byte(&curMaterial->alpha);
			UUmSwapLittle_4Byte(&curMaterial->selfIllumination);
			UUmSwapLittle_4Byte(&curMaterial->shininess);
			UUmSwapLittle_4Byte(&curMaterial->shininessStrength);
			UUmSwapLittle_4Byte(&curMaterial->ambient[0]);
			UUmSwapLittle_4Byte(&curMaterial->ambient[1]);
			UUmSwapLittle_4Byte(&curMaterial->ambient[2]);
			UUmSwapLittle_4Byte(&curMaterial->diffuse[0]);
			UUmSwapLittle_4Byte(&curMaterial->diffuse[1]);
			UUmSwapLittle_4Byte(&curMaterial->diffuse[2]);
			UUmSwapLittle_4Byte(&curMaterial->specular[0]);
			UUmSwapLittle_4Byte(&curMaterial->specular[1]);
			UUmSwapLittle_4Byte(&curMaterial->specular[2]);

			for(itr2 = 0, curMap = curMaterial->maps;
				itr2 < 3; /*MXcMapping_Count;*/			// XXX - hack to fix temp bug
				itr2++, curMap++)
			{
				UUmSwapLittle_4Byte(&curMap->amount);
			}

			UUmSwapLittle_4Byte(&curMaterial->requirements);
		}

	#endif

#if 0
		for(itr = 0, curFace = outNode->quads;
		itr < outNode->numQuads;
		itr++, curFace++)
	{
		UUtBool goodFace = UUcTrue;

		goodFace = goodFace && (curFace->indices[0] < outNode->numPoints);
		goodFace = goodFace && (curFace->indices[1] < outNode->numPoints);
		goodFace = goodFace && (curFace->indices[2] < outNode->numPoints);
		goodFace = goodFace && (curFace->indices[3] < outNode->numPoints);
		goodFace = goodFace && (curFace->material < outNode->numMaterials);

		if (!goodFace) {
			AUtMB_ButtonChoice choice;

			choice = AUrMessageBox(AUcMBType_OKCancel,
				"Bad face in file %s node %s (%d,%d,%d,%d) of %d material %d of %d.  Should I try to continue (most likely will crash later)?\n",
				inFileName,
				outNode->name,
				curFace->indices[0],
				curFace->indices[1],
				curFace->indices[2],
				curFace->indices[3],
				outNode->numPoints,
				curFace->material,
				outNode->numMaterials);

			if (AUcMBChoice_Cancel == choice) {
				return UUcError_Generic;
			}

			curFace->indices[0] = UUmMax(curFace->indices[0], outNode->numPoints);
			curFace->indices[1] = UUmMax(curFace->indices[1], outNode->numPoints);
			curFace->indices[2] = UUmMax(curFace->indices[2], outNode->numPoints);
			curFace->indices[3] = UUmMax(curFace->indices[3], outNode->numPoints);
			curFace->material = UUmMax(curFace->material, outNode->numMaterials);
		}
	}
#endif

	*ioPtr = curPtr;

	iClampTextureNames(outNode);

	return UUcError_None;
}

static UUtError iParseMXtMarker(char **ioPtr, MXtMarker *outMarker)
{
	char*		curPtr;

	#if UUmEndian == UUmEndian_Big

		UUtUns16		itr;
		float*			p;

	#endif

	curPtr = *ioPtr;

	*outMarker = *(MXtMarker*)curPtr;
	curPtr += sizeof(MXtMarker);

	#if UUmEndian == UUmEndian_Big

		p = (float*)&outMarker->matrix;
		for(itr = 0; itr < sizeof(M3tMatrix4x3) / sizeof(float); itr++)
		{
			UUmSwapLittle_4Byte(p);
			p++;
		}

		UUmSwapLittle_4Byte(&outMarker->userDataCount);

	#endif

	outMarker->userData = curPtr;
	curPtr += outMarker->userDataCount * sizeof(char);

	*ioPtr = curPtr;

	return UUcError_None;
}

/*
 *
 * Builds the hierarchy
 *
 */

static void
iMXtHeader_BuildHierarchy(
	MXtHeader*			ioHeader)
{
	UUtUns16 parentItr;
	UUtUns16 childItr;

	MXtNode *siblingNode;

	for(parentItr = 0; parentItr < ioHeader->numNodes; parentItr++)
	{
		MXtNode *parentNode = ioHeader->nodes + parentItr;

		siblingNode = NULL;

		for(childItr = 0; childItr < ioHeader->numNodes; childItr++)
		{
			MXtNode *childNode = ioHeader->nodes + childItr;

			// are we this parents child ?
			if (0 == strcmp(parentNode->name, childNode->parentName)) {
				childNode->parentNode = parentNode;

				if (NULL == siblingNode) {
					parentNode->child = childItr;
				}
				else {
					siblingNode->sibling = childItr;
				}

				siblingNode = childNode;
			}  // if we are this parents child
		} // loop through children
	} // loop through parents
}  // build hierarchy

static void
Imp_WriteHierarchyRecursive(
	BFtFile*			inFile,
	const MXtHeader*	inHeader,
	UUtUns16			numSpaces,
	UUtUns16			nodeIndex)
{
	UUtUns16 itr;
	MXtNode *node = inHeader->nodes + nodeIndex;

	// draw sibling
	if (0 != node->sibling)
	{
		Imp_WriteHierarchyRecursive(inFile, inHeader, numSpaces, node->sibling);
	}

	for(itr = 0; itr < numSpaces; itr++) {
		BFrFile_Printf(inFile, "  ");
	}

	BFrFile_Printf(inFile, "%s"UUmNL, node->name);

	// draw chlidren
	if (0 != node->child)
	{
		Imp_WriteHierarchyRecursive(inFile, inHeader, numSpaces + 1, node->child);
	}

	return;
}

static void
DumpNodeSummery(
	BFtFile*			inFile,
	UUtUns16			inIndex,
	const MXtNode*		inNode)
{
	BFrFile_Printf(inFile, "node %d"UUmNL, inIndex);
	BFrFile_Printf(inFile, "name %s"UUmNL, inNode->name);
	BFrFile_Printf(inFile, "parentName %s"UUmNL, inNode->parentName);
	BFrFile_Printf(inFile, "sibling %d"UUmNL, inNode->sibling);
	BFrFile_Printf(inFile, "child %d"UUmNL, inNode->child);
	BFrFile_Printf(inFile, "matrix:"UUmNL);

	BFrFile_Printf(inFile, "%f %f %f %f"UUmNL,
		inNode->matrix.m[0][0],
		inNode->matrix.m[1][0],
		inNode->matrix.m[2][0],
		inNode->matrix.m[3][0]);
	BFrFile_Printf(inFile, "%f %f %f %f"UUmNL,
		inNode->matrix.m[0][1],
		inNode->matrix.m[1][1],
		inNode->matrix.m[2][1],
		inNode->matrix.m[3][1]);
	BFrFile_Printf(inFile, "%f %f %f %f"UUmNL,
		inNode->matrix.m[0][2],
		inNode->matrix.m[1][2],
		inNode->matrix.m[2][2],
		inNode->matrix.m[3][2]);

	BFrFile_Printf(inFile, "numPoints %d"UUmNL, inNode->numPoints);
	BFrFile_Printf(inFile, "numTriangles %d"UUmNL, inNode->numTriangles);
	BFrFile_Printf(inFile, "numQuads %d"UUmNL, inNode->numQuads);
	BFrFile_Printf(inFile, "numMarkers %d"UUmNL, inNode->numMarkers);
	BFrFile_Printf(inFile, "numMaterials %d"UUmNL, inNode->numMaterials);
	BFrFile_Printf(inFile, UUmNL);

	return;
}

static void
DumpNodeDetails(
	BFtFile*			inFile,
	UUtUns16			inIndex,
	const MXtNode*		inNode)
{
	UUtUns16 itr;

	DumpNodeSummery(inFile, inIndex, inNode);

	BFrFile_Printf(inFile, "points (%d)"UUmNL, inNode->numPoints);
	for(itr = 0; itr < inNode->numPoints; itr++)
	{
		MXtPoint *curPoint = inNode->points + itr;
		BFrFile_Printf(inFile, "%d"UUmNL, itr);

		BFrFile_Printf(inFile, "%f %f %f"UUmNL,
			curPoint->point.x,
			curPoint->point.y,
			curPoint->point.z);

		BFrFile_Printf(inFile, "%f %f %f"UUmNL,
			curPoint->normal.x,
			curPoint->normal.y,
			curPoint->normal.z);

		BFrFile_Printf(inFile, "%f %f"UUmNL,
			curPoint->uv.u,
			curPoint->uv.v);

		BFrFile_Printf(inFile, ""UUmNL);
	}

	BFrFile_Printf(inFile, "triangles (%d)"UUmNL, inNode->numTriangles);
	for(itr = 0; itr < inNode->numTriangles; itr++)
	{
		MXtFace *curTriangle = inNode->triangles + itr;

		BFrFile_Printf(inFile, "%d"UUmNL, itr);

		BFrFile_Printf(inFile, "%d %d %d"UUmNL,
			curTriangle->indices[0],
			curTriangle->indices[1],
			curTriangle->indices[2]);

//		BFrFile_Printf(inFile, "%f %f %f"UUmNL,
//			curTriangle->normal.x,
//			curTriangle->normal.y,
//			curTriangle->normal.z);

		BFrFile_Printf(inFile, "%d"UUmNL,
			curTriangle->material);

		BFrFile_Printf(inFile, ""UUmNL);
	}

	BFrFile_Printf(inFile, "quads (%d)"UUmNL, inNode->numQuads);
	for(itr = 0; itr < inNode->numQuads; itr++)
	{
		MXtFace *curQuad = inNode->quads + itr;

		BFrFile_Printf(inFile, "%d"UUmNL, itr);

		BFrFile_Printf(inFile, "%d %d %d %d"UUmNL,
			curQuad->indices[0],
			curQuad->indices[1],
			curQuad->indices[2],
			curQuad->indices[3]);

//		BFrFile_Printf(inFile, "%f %f %f"UUmNL,
//			curQuad->normal.x,
//			curQuad->normal.y,
//			curQuad->normal.z);

		BFrFile_Printf(inFile, "%d"UUmNL,
			curQuad->material);

		BFrFile_Printf(inFile, ""UUmNL);
	}

	BFrFile_Printf(inFile, "materials (%d)"UUmNL, inNode->numMaterials);
	for(itr = 0; itr < inNode->numMaterials; itr++)
	{
		UUtUns16 mapItr;

		MXtMaterial *curMaterial = inNode->materials + itr;

		BFrFile_Printf(inFile, UUmNL);
		BFrFile_Printf(inFile, "name = %s"UUmNL, curMaterial->name);
		BFrFile_Printf(inFile, "alpha = %f"UUmNL, curMaterial->alpha);
		BFrFile_Printf(inFile, "selfIllumination = %f"UUmNL, curMaterial->selfIllumination);
		BFrFile_Printf(inFile, "shininess = %f"UUmNL, curMaterial->name);
		BFrFile_Printf(inFile, "shininessStrength = %f"UUmNL, curMaterial->name);
		BFrFile_Printf(inFile, "ambient (%f,%f,%f)"UUmNL, curMaterial->ambient[0], curMaterial->ambient[1], curMaterial->ambient[2]);
		BFrFile_Printf(inFile, "diffuse (%f,%f,%f)"UUmNL, curMaterial->diffuse[0], curMaterial->diffuse[1], curMaterial->diffuse[2]);
		BFrFile_Printf(inFile, "specular (%f,%f,%f)"UUmNL, curMaterial->specular[0], curMaterial->specular[1], curMaterial->specular[2]);
		BFrFile_Printf(inFile, "requirements %08x"UUmNL, curMaterial->requirements);
		BFrFile_Printf(inFile, UUmNL);

		for(mapItr = 0; mapItr < MXcMapping_Count; mapItr++)
		{
			MXtMapping *curMapping = curMaterial->maps + mapItr;

			BFrFile_Printf(inFile, "%02d %f %s"UUmNL, mapItr, curMapping->amount, curMapping->name);
		}
	}

	BFrFile_Printf(inFile, ""UUmNL);
}


UUtError
Imp_WriteTextEnvFile(
	BFtFileRef*			inFileRef,
	const MXtHeader*	inHeader)
{
	UUtUns16 itr;
	UUtError error;
	BFtFile *file;

	error = BFrFile_Open(inFileRef, "w", &file);
	UUmAssert(UUcError_None == error);

	BFrFile_Printf(file, UUmNL);

	// overview
	BFrFile_Printf(file, "overview:"UUmNL);
	{
		UUtUns32 pointCount = 0;
		UUtUns32 quadCount = 0;
		UUtUns32 triCount = 0;

		BFrFile_Printf(file, "env/mme dump"UUmNL);
		BFrFile_Printf(file, "version %d"UUmNL, inHeader->version);
		BFrFile_Printf(file, "numNodes %d"UUmNL, inHeader->numNodes);

		for(itr = 0; itr < inHeader->numNodes; itr++) {
			MXtNode *curNode = inHeader->nodes + itr;

			pointCount += curNode->numPoints;
			quadCount += curNode->numQuads;
			triCount += curNode->numTriangles;
		}

		BFrFile_Printf(file, "numPoints %d"UUmNL, pointCount);
		BFrFile_Printf(file, "numTriangles %d"UUmNL, triCount);
		BFrFile_Printf(file, "numQuads %d"UUmNL, quadCount);
		BFrFile_Printf(file, "numFaces %d"UUmNL, triCount + quadCount);
		BFrFile_Printf(file, UUmNL);
	}

	// hierarchy
	BFrFile_Printf(file, "hierarchy:"UUmNL);
	for(itr = 0; itr < inHeader->numNodes; itr++) {
		if (inHeader->nodes[itr].parentNode != NULL) {
			continue;
		}

		Imp_WriteHierarchyRecursive(file, inHeader, 0, itr);
	}
	BFrFile_Printf(file, UUmNL);

	// summary printout
	BFrFile_Printf(file, "dump:"UUmNL);
	for(itr = 0; itr < inHeader->numNodes; itr++) {
		DumpNodeSummery(file, itr, inHeader->nodes + itr);
	}

	for(itr = 0; itr < inHeader->numNodes; itr++) {
		DumpNodeDetails(file, itr, inHeader->nodes + itr);
	}

	return UUcError_None;
}

UUtError
Imp_ConvertENVFile(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError error;
	MXtHeader *header;
	BFtFileRef *srcRef;
	BFtFileRef *dstRef;
	char *srcName;
	char *dstName;

	Imp_PrintMessage(IMPcMsg_Cosmetic, "\tbuilding..."UUmNL);

	error = GRrGroup_GetString(inGroup, "srcName", &srcName);
	UUmError_ReturnOnErrorMsg(error, "failed to find 'srcName'");

	error = GRrGroup_GetString(inGroup, "dstName", &dstName);
	UUmError_ReturnOnErrorMsg(error, "failed to find 'dstName'");

	error = BFrFileRef_MakeFromName(srcName, &srcRef);
	UUmError_ReturnOnErrorMsg(error, "failed to open source file");

	error = BFrFileRef_MakeFromName(dstName, &dstRef);
	UUmError_ReturnOnErrorMsg(error, "failed to open dest file");

	error = Imp_ParseEnvFile(srcRef, &header);
	UUmError_ReturnOnErrorMsg(error, "failed to parse env file");

	error = Imp_WriteTextEnvFile(dstRef, header);
	UUmError_ReturnOnErrorMsg(error, "failed to write text env file");

	return UUcError_None;
}

void
Imp_EnvFile_Delete(
	MXtHeader*			inHeader)
{
	UUtUns16	nodeItr;

	MXtNode*	curNode;

	for(nodeItr = 0, curNode = inHeader->nodes;
		nodeItr < inHeader->numNodes;
		nodeItr++, curNode++)
	{
		if(curNode->numMarkers > 0)
		{
			UUrMemory_Block_Delete(curNode->markers);
		}
	}

	UUrMemory_Block_Delete(inHeader->nodes);

	UUrMemory_Block_Delete(inHeader);
}


MXtMarker *
Imp_EnvFile_GetMarker(
	MXtNode				*inNode,
	const char			*inMarkerName)
{
	MXtMarker *result = NULL;
	UUtUns16 itr;

	for(itr = 0; itr < inNode->numMarkers; itr++)
	{
		MXtMarker *curMarker = inNode->markers + itr;

		if (0 == strcmp(curMarker->name, inMarkerName)) {
			result = curMarker;
			break;
		}
	}

	return result;
}

static void iSwapLittle_Matrix(M3tMatrix4x3 *pMatrix)
{
	UUtUns16 itr;
	float *p = (float *) pMatrix;

	for(itr = 0; itr < sizeof(M3tMatrix4x3) / sizeof(float); itr++)
	{
		UUmSwapLittle_4Byte(p);
		p++;
	}

	return;
}


static UUtError Imp_ParseEvaNode(BFtFile *file, UUtUns16 count, AXtNode *pNode, M3tMatrix4x3 *pMatrix)
{
	UUtError error;
	UUtUns16 itr;

	error = BFrFile_Read(file, sizeof(*pNode), pNode);
	UUmError_ReturnOnError(error);

	iSwapLittle_Matrix(&(pNode->objectTM));
	pNode->matricies = pMatrix;

	error = BFrFile_Read(file, sizeof(M3tMatrix4x3) * count, pMatrix);
	UUmError_ReturnOnError(error);

	for(itr = 0; itr < count; itr++)
	{
		iSwapLittle_Matrix(pMatrix);
		pMatrix++;
	}

	return UUcError_None;
}


UUtError
Imp_ParseEvaFile(
	BFtFileRef*			inFileRef,
	AXtHeader**			outHeader)
{
	UUtUns8 *pDataBlock;
	UUtError error;
	BFtFile *file;
	UUtUns32 length;
	UUtUns16 itr;
	AXtHeader header;
	AXtNode *pNode;
	M3tMatrix4x3 *pMatrix;

	error = BFrFile_Open(inFileRef, "r", &file);
	UUmError_ReturnOnError(error);

	error = BFrFile_Read(file, sizeof(header), &header);
	UUmError_ReturnOnError(error);

	UUmSwapLittle_4Byte(&header.version);
	UUmSwapLittle_2Byte(&header.numNodes);
	UUmSwapLittle_2Byte(&header.numFrames);
	UUmSwapLittle_2Byte(&header.startFrame);
	UUmSwapLittle_2Byte(&header.endFrame);
	UUmSwapLittle_2Byte(&header.ticksPerFrame);
	UUmSwapLittle_2Byte(&header.startTime);
	UUmSwapLittle_2Byte(&header.endTime);
	UUmSwapLittle_2Byte(&header.pad);

	length =
		sizeof(AXtHeader) +
		sizeof(AXtNode) * header.numNodes +
		sizeof(M3tMatrix4x3) * header.numNodes * header.numFrames;

	pDataBlock = UUrMemory_Block_New(length);
	error = (NULL == pDataBlock) ? UUcError_Generic : UUcError_None;
	UUmError_ReturnOnError(error);

	pNode = (AXtNode *) (pDataBlock + sizeof(header));
	pMatrix = (M3tMatrix4x3 *) (pDataBlock + sizeof(header) + sizeof(AXtNode) * header.numNodes);

	// attach node list to the header
	header.nodes = pNode;

	for(itr = 0; itr < header.numNodes; itr++)
	{
		error = Imp_ParseEvaNode(file, header.numFrames, pNode, pMatrix);
		UUmError_ReturnOnError(error);

		pNode += 1;
		pMatrix += header.numFrames;
	}

	UUrMemory_MoveFast(&header, pDataBlock, sizeof(header));
	*outHeader = (AXtHeader *) pDataBlock;

	return UUcError_None;
}

void
Imp_EvaFile_Delete(
	AXtHeader*			inHeader)
{
	UUrMemory_Block_Delete(inHeader);

	return;
}
