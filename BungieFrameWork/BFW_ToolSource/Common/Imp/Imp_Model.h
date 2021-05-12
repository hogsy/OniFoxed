/*
	FILE:	Imp_Model.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: August 14, 1997
	
	PURPOSE: Header file for the BMP file format
	
	Copyright 1997

*/

#ifndef __IMP_MODEL_H__
#define __IMP_MODEL_H__

#include <stdio.h>

#include "BFW.h"
#include "BFW_TemplateManager.h"
#include "BFW_Motoko.h"
#include "EnvFileFormat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IMPcModel_MinTriArea		(1e-06)

UUtError
IMPrModel_Initialize(
	void);

void
IMPrModel_Terminate(
	void);


#if 0
void
Imp_Geometry_AddBoundingBox(
	M3tGeometry	*inGeometry);

UUtError
Imp_Geometry_Stripify(
	M3tGeometry *inGeometry);

UUtError Imp_Node_To_Geometry_Points(
	const MXtNode	*inNode,
	M3tGeometry		*inGeometry);

UUtError Imp_Node_To_Geometry_Triangles(
	const MXtNode	*inNode,
	M3tGeometry		*inGeometry);

UUtError Imp_Node_To_Geometry_Quads(
	const MXtNode	*inNode,
	M3tGeometry		*inGeometry);
#endif

UUtError Imp_Node_To_Geometry(
	const MXtNode	*inNode,
	M3tGeometry		*inGeometry);

UUtError Imp_Node_To_Geometry_ApplyMatrix(
	const MXtNode	*inNode,
	M3tGeometry		*inGeometry);
	
UUtError Imp_NodeMaterial_To_Geometry(
	const MXtNode	*inNode,
	UUtUns16		inMaterialIndex,
	M3tGeometry		*outGeometry);
	
UUtError Imp_NodeMaterial_To_Geometry_ApplyMatrix(
	const MXtNode	*inNode,
	UUtUns16		inMaterialIndex,
	M3tGeometry		*outGeometry);

UUtError
Imp_AddModel(
	BFtFileRef*			inSourceFileRef,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddModelArray(
	BFtFileRef*			inSourceFileRef,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);
	
#ifdef __cplusplus
}
#endif

#endif // __IMP_MODEL_H__
