/*
	FILE:	BFW_Motoko_Template.c

	AUTHOR:	Brent H. Pease

	CREATED: May 5, 1997

	PURPOSE: Interface to the Motoko 3D engine

	Copyright 1997

*/
#include "BFW.h"
#include "BFW_TemplateManager.h"

#include "BFW_Motoko.h"

UUtError
M3rRegisterTemplates(
	void)
{
	UUtError	error;

	error = TMrTemplate_Register(M3cTemplate_PlaneEquationArray, sizeof(M3tPlaneEquationArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);
	error = TMrTemplate_Register(M3cTemplate_TextureMap, sizeof(M3tTextureMap), TMcFolding_Allow);
	UUmError_ReturnOnError(error);
	error = TMrTemplate_Register(M3cTemplate_TextureMap_Big, sizeof(M3tTextureMap_Big), TMcFolding_Allow);
	UUmError_ReturnOnError(error);
	error = TMrTemplate_Register(M3cTemplate_TextureMapAnimation, sizeof(M3tTextureMapAnimation), TMcFolding_Allow);
	UUmError_ReturnOnError(error);
	error = TMrTemplate_Register(M3cTemplate_Point3DArray, sizeof(M3tPoint3DArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);
	error = TMrTemplate_Register(M3cTemplate_Vector3DArray, sizeof(M3tVector3DArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);
	error = TMrTemplate_Register(M3cTemplate_ColorRGBArray, sizeof(M3tColorRGBArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);
	error = TMrTemplate_Register(M3cTemplate_EdgeIndexArray, sizeof(M3tEdgeIndexArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);
	error = TMrTemplate_Register(M3cTemplate_TextureCoordArray, sizeof(M3tTextureCoordArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);
	//error = TMrTemplate_Register(M3cTemplate_EnvTextureCoordArray, sizeof(M3tEnvTextureCoordArray), TMcFolding_Allow);
	//UUmError_ReturnOnError(error);
	error = TMrTemplate_Register(M3cTemplate_TriArray, sizeof(M3tTriArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);
	error = TMrTemplate_Register(M3cTemplate_QuadArray, sizeof(M3tQuadArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);
	error = TMrTemplate_Register(M3cTemplate_Geometry, sizeof(M3tGeometry), TMcFolding_Allow);
	UUmError_ReturnOnError(error);
	error = TMrTemplate_Register(M3cTemplate_TextureMapArray, sizeof(M3tTextureMapArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);
	error = TMrTemplate_Register(M3cTemplate_GeometryArray, sizeof(M3tGeometryArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);
	error = TMrTemplate_Register(M3cTemplate_GeometryAnimation, sizeof(M3tGeometryAnimation), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}
