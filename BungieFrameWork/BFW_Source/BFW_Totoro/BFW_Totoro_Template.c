/*
	FILE:	BFW_Totoro_Template.c
	
	AUTHOR:	Michael Evans
	
	CREATED: Sept 25, 1997
	
	PURPOSE: animation engine
	
	Copyright 1997

*/

#include "BFW.h"
#include "BFW_TemplateManager.h"

#include "BFW_Totoro.h"
#include "BFW_Totoro_Private.h"


UUtError
TRrRegisterTemplates(
	void)
{
	UUtError	error;
	
	error = TMrTemplate_Register(TRcTemplate_Body, sizeof(TRtBody), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(TRcTemplate_BodySet, sizeof(TRtBodySet), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(TRcTemplate_Animation, sizeof(TRtAnimation), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(TRcTemplate_BodyTextures, sizeof(TRtBodyTextures), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(TRcTemplate_AnimationCollection, sizeof(TRtAnimationCollection), TMcFolding_Allow);	
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(TRcTemplate_FacingTable, sizeof(TRtFacingTable), TMcFolding_Allow);	
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(TRcTemplate_AimingScreen, sizeof(TRtAimingScreen), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(TRcTemplate_ScreenCollection, sizeof(TRtScreenCollection), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(TRcTemplate_GeometryArray, sizeof(TRtGeometryArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(TRcTemplate_TranslationArray, sizeof(TRtTranslationArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(TRcTemplate_IndexArray, sizeof(TRtIndexBlockArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}
