/*
	FILE:	BFW_Akira_Template.c

	AUTHOR:	Brent H. Pease

	CREATED: October 22, 1997

	PURPOSE: environment engine

	Copyright 1997

*/

#include "BFW.h"
#include "BFW_TemplateManager.h"

#include "BFW_Akira.h"

UUtError
AKrRegisterTemplates(
	void)
{
	UUtError	error;

	error = TMrTemplate_Register(AKcTemplate_OctTree, sizeof(AKtOctTree), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(AKcTemplate_OctTree_InteriorNodeArray, sizeof(AKtOctTree_InteriorNodeArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(AKcTemplate_OctTree_LeafNodeArray, sizeof(AKtOctTree_LeafNodeArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(AKcTemplate_QuadTree_NodeArray, sizeof(AKtQuadTree_NodeArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(AKcTemplate_BSPNodeArray, sizeof(AKtBSPNodeArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(AKcTemplate_GQ_Material, sizeof(AKtGQMaterial), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(AKcTemplate_GQ_General, sizeof(AKtGQGeneralArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(AKcTemplate_GQ_Render, sizeof(AKtGQRenderArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(AKcTemplate_GQ_Collision, sizeof(AKtGQCollisionArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(AKcTemplate_GQ_Debug, sizeof(AKtGQDebugArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(AKcTemplate_AdjacencyArray, sizeof(AKtAdjacencyArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(AKcTemplate_BNVNodeSideArray, sizeof(AKtBNVNodeSideArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(AKcTemplate_BNVNodeArray, sizeof(AKtBNVNodeArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(AKcTemplate_AlphaBSPTree_NodeArray, sizeof(AKtAlphaBSPTree_NodeArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(AKcTemplate_DoorFrameArray, sizeof(AKtDoorFrameArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(AKcTemplate_Environment, sizeof(AKtEnvironment), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

