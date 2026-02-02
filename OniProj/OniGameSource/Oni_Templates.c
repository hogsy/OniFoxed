
/*
	FILE:	Oni_Templates.c

	AUTHOR:	Michael Evans

	CREATED: Jan, 1998

	PURPOSE:

	This contains code to register all the Oni templates.

	Copyright 1998

*/

#include "BFW.h"
#include "BFW_Object.h"

#include "Oni_Templates.h"
#include "Oni_Character.h"
#include "Oni_AI_Setup.h"
#include "Oni_Level.h"
#include "Oni_Object.h"
#include "Oni_Sky.h"
#include "Oni.h"

TMtPrivateData				*ONgCharacterClass_PrivateData = NULL;

UUtError
ONrRegisterTemplates(void)
{
	UUtError	error;

	error = TMrTemplate_Register(ONcTemplate_Film, sizeof(ONtFilm), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(ONcTemplate_FlagArray, sizeof(ONtFlagArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(TMcTemplate_String, sizeof(TMtString), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(TMcTemplate_StringArray, sizeof(TMtStringArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(ONcTemplate_Level, sizeof(ONtLevel), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(ONcTemplate_Level_Descriptor, sizeof(ONtLevel_Descriptor), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(ONcTemplate_MarkerArray, sizeof(ONtMarkerArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(TRcTemplate_CharacterClass, sizeof(ONtCharacterClass), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(ONcTemplate_CharacterVariant, sizeof(ONtCharacterVariant), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(ONcTemplate_BodyPartMaterials, sizeof(ONtBodyPartMaterials), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(ONcTemplate_BodyPartImpacts, sizeof(ONtBodyPartImpacts), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(ONcTemplate_VariantList, sizeof(ONtVariantList), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(ONcTemplate_CharacterImpactArray, sizeof(ONtCharacterImpactArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(ONcTemplate_CharacterParticleArray, sizeof(ONtCharacterParticleArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(ONcTemplate_SpawnArray, sizeof(ONtSpawnArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(ONcTemplate_TriggerArray, sizeof(ONtTriggerArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(ONcTemplate_SkyClass, sizeof(ONtSkyClass), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(ONcTemplate_ObjectGunkArray, sizeof(ONtObjectGunkArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(ONcTemplate_CorpseArray, sizeof(ONtCorpseArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(ONcTemplate_GameSettings, sizeof(ONtGameSettings), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_PrivateData_New(TRcTemplate_CharacterClass, 0, ONrCharacterClass_ProcHandler, &ONgCharacterClass_PrivateData);
	UUmError_ReturnOnError(error);

	error = WPrRegisterTemplates();
	UUmError_ReturnOnError(error);

	error = OBrRegisterTemplates();
	UUmError_ReturnOnError(error);

	error = AIrRegisterTemplates();
	UUmError_ReturnOnError(error);

	error = OBJrRegisterTemplates();
	UUmError_ReturnOnError(error);

	return UUcError_None;
}
