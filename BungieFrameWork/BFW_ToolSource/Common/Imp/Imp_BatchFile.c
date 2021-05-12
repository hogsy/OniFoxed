// Imp_BatchFile.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Akira.h"
#include "BFW_FileManager.h"
#include "BFW_TemplateManager.h"
#include "BFW_TM_Construction.h"
#include "BFW_LocalInput.h"
#include "BFW_Totoro.h"
#include "BFW_SoundSystem.h"
#include "BFW_TextSystem.h"
#include "BFW_Group.h"
#include "BFW_Path.h"
#include "BFW_AppUtilities.h"
#include "BFW_Timer.h"
#include "BFW_BinaryData.h"

#include "Imp_AI_HHT.h"
#include "Imp_AI_Script.h"
#include "Imp_BatchFile.h"
#include "Imp_Character.h"
#include "Imp_Common.h"
#include "Imp_Cursor.h"
#include "Imp_Descriptor.h"
#include "Imp_DialogData.h"
#include "Imp_Env2.h"
#include "Imp_Font.h"
#include "Imp_Furniture.h"
#include "Imp_Input.h"
#include "Imp_Model.h"
#include "Imp_MenuStuff.h"
#include "Imp_ParseEnvFile.h"
#include "Imp_PartSpec.h"
#include "Imp_Sound.h"
#include "Imp_Sound2.h"
#include "Imp_Texture.h"
#include "Imp_Weapon.h"
#include "Imp_Particle2.h"
#include "Imp_FX.h"
#include "Imp_Trigger.h"
#include "Imp_Turret.h"
#include "Imp_Console.h"
#include "Imp_Door.h"
#include "Imp_Sky.h"
#include "Imp_Material.h"
#include "Imp_GameSettings.h"
#include "Imp_InGameUI.h"

#include "ONI_Character.h"

GRtGroup *gPreferencesGroup = NULL;	// just used by character stuff for now

#define IMPcMaxDirInclude 1024

typedef UUtError
(*tParseFunction)(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

typedef struct tParseData
{
	char				*name;
	tParseFunction		parseFunction;
	UUtBool				parseThisType;
} tParseData;

tParseData gParseData[] = 
{
	{ "texture", Imp_AddTexture, UUcTrue },
	{ "texture_big", Imp_AddTexture_Big, UUcTrue },
	{ "texture_directory", Imp_AddTextureDirectory, UUcTrue},
	{ "geometry", Imp_AddModel, UUcTrue },
	{ "environment", IMPrEnv_Add, UUcTrue },
	{ "font_family", Imp_AddFontFamily, UUcTrue },
	{ "font_language", Imp_AddLanguage, UUcTrue },
	{ "body", Imp_AddBody, UUcTrue },
	{ "animation", Imp_AddAnimation, UUcTrue },
	{ "body_textures", Imp_AddBodyTextures, UUcTrue },
	{ "anim_collection", Imp_AddAnimationCollection, UUcTrue },
	{ "screen_collection", Imp_AddScreenCollection, UUcTrue },
	{ "character_class", Imp_AddCharacterClass, UUcTrue },
	{ "string_list", Imp_AddStringList, UUcTrue },
	{ "sound_data", IMPrImport_AddSound2, UUcTrue },
	{ "weapon", Imp_AddWeapon, UUcTrue },
	{ "geometry_list", Imp_AddModelArray, UUcTrue },
	{ "cursor_list", Imp_AddCursorList, UUcTrue },
	{ "convert_env_file", Imp_ConvertENVFile, UUcTrue },
	{ "aimimg_screen", Imp_AddAimingScreen, UUcTrue },
	{ "part_spec", Imp_AddPartSpec, UUcTrue },
	{ "partspec_list", Imp_AddPartSpecList, UUcTrue },
	{ "partspec_ui", Imp_AddPartSpecUI, UUcTrue },
	{ "env_anim", IMPrEnvAnim_Add, UUcTrue },
	{ "anim_cache", Imp_AddAnimCache, UUcTrue },
	{ "level_descriptor", Imp_AddLevelDescriptor, UUcTrue },
	{ "ai_script_table", Imp_AddAIScriptTable, UUcTrue },
	{ "savedfilm", Imp_AddSavedFilm, UUcTrue },
	{ "material", Imp_AddQuadMaterial, UUcTrue },
	{ "menu", Imp_AddMenu, UUcTrue },
	{ "menubar", Imp_AddMenuBar, UUcTrue },
	{ "dialog_data", Imp_AddDialogData, UUcTrue },
	{ "furniture", Imp_AddFurniture, UUcTrue },
	{ "trigger", Imp_AddTrigger, UUcTrue },
	{ "trigger_emitter", Imp_AddTriggerEmitter, UUcTrue },
	{ "turret", Imp_AddTurret, UUcTrue },
	{ "console", Imp_AddConsole, UUcTrue },
	{ "door", Imp_AddDoor, UUcTrue },
	{ "character_variant", Imp_AddVariant, UUcTrue },
	{ "variant_list", Imp_AddVariantList, UUcTrue },
	{ "sky", Imp_AddSky, UUcTrue },
	{ "materials", Imp_AddMaterial, UUcTrue },
	{ "impacts", Imp_AddImpact, UUcTrue },
	{ "biped", Imp_AddBiped, UUcTrue },
	{ "game_settings", Imp_AddGameSettings, UUcTrue },
	{ "DiaryPage", Imp_AddDiaryPage, UUcTrue },
	{ "HelpPage", Imp_AddHelpPage, UUcTrue },
	{ "ItemPage", Imp_AddItemPage, UUcTrue },
	{ "ObjectivePage", Imp_AddObjectivePage, UUcTrue },
	{ "WeaponPage", Imp_AddWeaponPage, UUcTrue },
	{ "EnemyScannerUI", Imp_AddEnemyScannerUI, UUcTrue },
	{ "KeyIcons", Imp_AddKeyIcons, UUcTrue },
	{ "TextConsole", Imp_AddTextConsole, UUcTrue },
	{ "sound_binary_files", IMPrImport_AddSoundBin, UUcTrue },
	{ "subtitles", IMPrImport_AddSubtitles, UUcTrue },
	{ "hud_help_info", Imp_AddHUDHelp, UUcTrue },


	{ NULL, NULL }
};

static void
iStripNonFileNameStuff(
	char*	inStr)
{
	char*	p = inStr;
	
	while((*p != 0) && !UUrIsSpace(*p) && *p != '#') p++;
	
	*p = 0;
}

static UUtError Imp_Process_InsTxt(BFtFileRef *inSourceFileRef)
{
	GRtGroup_Context	*fileGroupContext = NULL;
	GRtGroup			*fileGroup;
	GRtElementType		elementType;
	UUtError			error;
	char				*parserNameStr;
	const char			*curFileName;
	char				*instanceNameStr;
	UUtUns32			sourceModDate;
	tParseData			*curParseData;
	
		
	curFileName = BFrFileRef_GetLeafName(inSourceFileRef); 
	
	UUrMemory_Leak_Start(curFileName);
	
	error =
		GRrGroup_Context_NewFromFileRef(
			inSourceFileRef,
			gPreferencesGroup,
			NULL,
			&fileGroupContext,
			&fileGroup);
	if (error) 
	{
		Imp_PrintWarning("Could not create group");
		goto errorExit;
	}
	
	/*
	 * Get the parse name
	 */
		error =
			GRrGroup_GetElement(
				fileGroup,
				"template",
				&elementType,
				&parserNameStr);
		if(error != UUcError_None || elementType != GRcElementType_String)
		{
			Imp_PrintWarning("File \"%s\": Could not get template string",  (UUtUns32)curFileName, 0, 0);
			goto errorExit;
		}
		
	/*
	 * Get the instance name
	 */
		error =
			GRrGroup_GetElement(
				fileGroup,
				"instance",
				&elementType,
				&instanceNameStr);
		if(error != UUcError_None || elementType != GRcElementType_String)
		{
			Imp_PrintWarning("File \"%s\": Could not get instance string",  (UUtUns32)curFileName, 0, 0);
			goto errorExit;
		}
				
	/*
	 * Get the instance source file mod date
	 */
		error =
			BFrFileRef_GetModTime(
				inSourceFileRef,
				&sourceModDate);
		if (error) {
			Imp_PrintWarning("Could not get instance source mod date");
			goto errorExit;
		}
		
				
	/*
	 * Parse the data
	 */
		curParseData = gParseData;
		while(curParseData->name != 0)
		{
			if(!strcmp(curParseData->name, parserNameStr)) break;
			
			curParseData++;
		}	
		
		if ((curParseData->name != 0) && (curParseData->parseThisType))
		{
			Imp_PrintMessage(IMPcMsg_Cosmetic, "template: %s"UUmNL, parserNameStr);
			Imp_PrintMessage(IMPcMsg_Important, "\tinstanceName: %s"UUmNL, instanceNameStr);
			Imp_PrintMessage(IMPcMsg_Cosmetic, "\tprocessing %s"UUmNL, curParseData->name);
			
			if(IMPgConstructing) TMmConstruction_MajorCheck();
			
			// cheesy hack to only parse environments when outputting lightmaps
			if(IMPgLightmaps && IMPgLightmap_Output &&
				(curParseData->parseFunction != IMPrEnv_Add))
			{
				goto errorExit;
			}
			
			error = curParseData->parseFunction(
				inSourceFileRef,
				sourceModDate,
				fileGroup,
				instanceNameStr);

			if(error != UUcError_None)
			{
				Imp_PrintWarning("error %d Could not parse instance file %s.", 
					error, 
					BFrFileRef_GetLeafName(inSourceFileRef));
			}

			if(IMPgConstructing) TMmConstruction_MajorCheck();
		}
		else if (NULL == curParseData->name)
		{
			fprintf(stderr, "\tNO PARSER FOUND"UUmNL);
		}
	
errorExit:
		if (NULL != fileGroupContext) {
			GRrGroup_Context_Delete(fileGroupContext);
		}

	UUrMemory_Leak_StopAndReport();
	
	return error;
}

UUtError Imp_Process_Bin(BFtFileRef *inSourceFileRef)
{
	const char			*curFileName;
	char				file_name[128];
	BDtBinaryData		*binary_data;
	BDtHeader			*input_data_header;
	void				*data;
	void				*separate_data;
	UUtUns32			data_length;
	UUtError			error;
	
	// don't processes bin files when outputting lightmaps
	if ((IMPgLightmaps) && (IMPgLightmap_Output))
	{
		return UUcError_None;
	}
	
	curFileName = BFrFileRef_GetLeafName(inSourceFileRef);

	UUrMemory_Leak_Start(curFileName);
	
	// allocate memory and copy the file's contents into that memory
	error =
		BFrFileRef_LoadIntoMemory(
			inSourceFileRef,
			&data_length,
			&data);
	IMPmError_ReturnOnErrorMsg(error, "Unable to load binary file into memory");
	
	if (data_length > 0)
	{
		Imp_PrintMessage(IMPcMsg_Important, "\tbinaryFileName: %s"UUmNL, curFileName);

		if(IMPgConstructing) { TMmConstruction_MajorCheck(); }

		// CB: prepend the binary data type to the instance name, to avoid name collision in the
		// template manager
		input_data_header = (BDtHeader *) data;
		*((UUtUns32 *) file_name) = input_data_header->class_type;
		file_name[4] = '\0';

		UUrString_Copy(&file_name[4], curFileName, sizeof(file_name) - 4);
		UUrString_StripExtension(file_name);

		// make sure that we don't exceed the maximum template data size... this means that we
		// must never exceed 27 characters on any binary data instance name
		if (strlen(file_name) >= BFcMaxFileNameLength) {
			file_name[BFcMaxFileNameLength - 1] = '\0';
			Imp_PrintWarning("Binary data file '%s' exceeds maximum name length, truncating instance name to '%s'", curFileName, file_name);
		}
		Imp_PrintMessage(IMPcMsg_Important, "\t\tinstanceName: %s"UUmNL, file_name);

		// create an instance of a binary data template
		error =
			TMrConstruction_Instance_Renew(
				BDcTemplate_BinaryData,
				file_name,
				0,
				&binary_data);
		IMPmError_ReturnOnErrorMsg(error, "Unable to create binary data instance");
		
		// copy the file data into the raw template data
		separate_data = TMrConstruction_Separate_New(data_length, BDcTemplate_BinaryData);
		if (separate_data == NULL) {
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to make room in separate binary file");
		}

		UUrMemory_MoveFast(
			data,
			separate_data,
			data_length);
		
		binary_data->data_size = data_length;
		binary_data->data_index = (TMtSeparateFileOffset) TMrConstruction_Separate_Write(separate_data);

		if(IMPgConstructing) { TMmConstruction_MajorCheck(); }
	}
	
	// dispose of the allocated memory
	UUrMemory_Block_Delete(data);
	
	UUrMemory_Leak_StopAndReport();
	
	return UUcError_None;
}

static UUtError Imp_Process_OneSourceFile(BFtFileRef *inSourceFileRef)
{
	UUtError			error;
	const char			*curFileName;
	char				*suffix;
	
	curFileName = BFrFileRef_GetLeafName(inSourceFileRef); 
	
	if(BFrFileRef_FileExists(inSourceFileRef) == UUcFalse)
	{
		Imp_PrintWarning(
			"Batch file referred to instance file \"%s\" but that file does not exist!", 
			curFileName);
		return UUcError_Generic;
	}
	
	suffix = strrchr(curFileName, '.');
	if (strcmp(suffix, ".bin") == 0)
	{
		// process .bin file
		error = Imp_Process_Bin(inSourceFileRef);
	}
	else if (strcmp(suffix, ".txt") == 0)
	{
		// process _ins.txt file
		error = Imp_Process_InsTxt(inSourceFileRef);
	}
	IMPmError_ReturnOnError(error);
	
	return UUcError_None;
}

UUtError
Imp_BatchFile_Process(
	BFtFileRef*			inBatchFileRef)
{	
	UUtError		error;
	BFtTextFile*	batchFile;
	char*			curFileName;
	BFtFileRef*		sourceFileRef;	
	UUtBool			retry,processList,abort;
	UUtUns16 		numFiles,i;
	BFtFileRef 		*dir[IMPcMaxDirInclude];
	UUtInt64		startTime = UUrMachineTime_High();
	UUtUns32		startInstances = TMrConstruction_GetNumInstances();
	char*			includeName;
		
	error = BFrTextFile_OpenForRead(inBatchFileRef, &batchFile);
	UUmError_ReturnOnErrorMsg(error, "Could not open batch file");
	
	while(1)
	{
		curFileName = BFrTextFile_GetNextStr(batchFile);
		if(curFileName == NULL) {
			break;
		}
		processList = UUcFalse;
		
		if ('+' == curFileName[0]) {
			UUtBool retryFindBatchFile = UUcFalse;
			includeName = curFileName + 1;
	
			iStripNonFileNameStuff(includeName);
			Imp_PrintMessage(IMPcMsg_Important, "include"UUmNL);
			Imp_PrintMessage(IMPcMsg_Important, "\tfileName: %s"UUmNL, includeName);
			Imp_PrintMessage(IMPcMsg_Important, "*************begin*include*************"UUmNL);

			abort = UUcFalse;
			do 
			{
				error =
					BFrFileRef_DuplicateAndReplaceName(
						inBatchFileRef,
						includeName,
						&sourceFileRef);
				UUmError_ReturnOnErrorMsgP(error, "Include File \"%s\" was not found", (UUtUns32)curFileName, 0, 0);
				
				if(BFrFileRef_FileExists(sourceFileRef)) {
					retryFindBatchFile = UUcFalse;
				}
				else if (IMPgSuppressErrors) {
					IMPgSuppressedErrors++;
					Imp_PrintMessage(IMPcMsg_Important, "ERROR: File \"%s\" was not found", curFileName);
					retryFindBatchFile = UUcFalse;
					abort = UUcTrue;
				} else {
					AUtMB_ButtonChoice findBatchFileFailedChoice;

					findBatchFileFailedChoice = AUrMessageBox(AUcMBType_RetryCancel, "File \"%s\" was not found", curFileName);

					if (AUcMBChoice_Retry == findBatchFileFailedChoice) {
						retryFindBatchFile = UUcTrue;
					}
					else {
						error = BFcError_FileNotFound;
						UUmError_ReturnOnErrorMsgP(error, "Include File \"%s\" was not found", (UUtUns32)curFileName, 0, 0);
					}
				}
			} while(retryFindBatchFile);

			if (!abort) {
				Imp_BatchFile_Process(sourceFileRef);
			}

			BFrFileRef_Dispose(sourceFileRef);

			Imp_PrintMessage(IMPcMsg_Important, "****************end*include************"UUmNL);

			continue;
		}
		else if ('*' == curFileName[0])
		{
			// include directory of _ins.txt or .bin files
			char			*ins_txt_suffix = "_ins.txt";
			char			*bin_suffix = ".bin";
			UUtUns16		num_bin_files;
			
			includeName = curFileName + 1;
			
			iStripNonFileNameStuff(includeName);
			Imp_PrintMessage(IMPcMsg_Important, "include multiple"UUmNL);
			Imp_PrintMessage(IMPcMsg_Important, "\tfileName: %s"UUmNL, includeName);
			
			error =	BFrFileRef_DuplicateAndReplaceName(
				inBatchFileRef,
				includeName,
				&sourceFileRef);
			UUmError_ReturnOnErrorMsgP(error, "Unable to open directory \"%s\"", (UUtUns32)includeName,0,0);
			
			error = BFrDirectory_GetFileList(
				sourceFileRef,
				NULL,
				ins_txt_suffix,
				IMPcMaxDirInclude,
				&numFiles,
				dir);
			UUmError_ReturnOnErrorMsgP(error, "Unable to get file list \"%s\"", (UUtUns32)includeName,0,0);

			error = BFrDirectory_GetFileList(
				sourceFileRef,
				NULL,
				bin_suffix,
				IMPcMaxDirInclude - numFiles,
				&num_bin_files,
				(dir + numFiles));
			UUmError_ReturnOnErrorMsgP(error, "Unable to get file list \"%s\"", (UUtUns32)includeName,0,0);
			
			numFiles += num_bin_files;
			processList = UUcTrue;
			
			BFrFileRef_Dispose(sourceFileRef);
		}
		else if ('!' == curFileName[0]) {
			Imp_PrintMessage(IMPcMsg_Critical, "%s"UUmNL, curFileName + 1);

			continue;
		}
		else if ('#' == curFileName[0]) {
			continue;
		}
		
		if (!processList)
		{
			iStripNonFileNameStuff(curFileName);
		
			if (0 == curFileName[0]) {
				continue;
			}

			error =
				BFrFileRef_DuplicateAndReplaceName(
					inBatchFileRef,
					curFileName,
					&dir[0]);
			UUmError_ReturnOnErrorMsgP(error, "File \"%s\" was not found", (UUtUns32)curFileName, 0, 0);
			numFiles = 1;
		}
		
		for (i=0; i<numFiles; i++)
		{
			do {
				retry = UUcFalse;
				error = Imp_Process_OneSourceFile(dir[i]);
				
				if (error) {
					if (IMPgSuppressErrors) {
						Imp_PrintMessage(IMPcMsg_Important, "ERROR: failed to process this instance! Error %d", error);
						IMPgSuppressedErrors++;
					} else {
						AUtMB_ButtonChoice choice = AUrMessageBox(AUcMBType_AbortRetryIgnore, "Failed to process this instance! Error %d [hex = %d]", error, error);

						switch (choice)
						{
							case AUcMBChoice_Abort:
								exit(1);
							break;

							case AUcMBChoice_Retry:
								retry = UUcTrue;
							break;

							case AUcMBChoice_Ignore:
							break;

							default:
								UUmAssert(0);
						}
					}
				}
			} while(retry);
		
			BFrFileRef_Dispose(dir[i]);
		}
		
		Imp_PrintMessage(IMPcMsg_Cosmetic, "***************************************"UUmNL);
	}
	
	BFrTextFile_Close(batchFile);

	{
		UUtUns32 endInstances = TMrConstruction_GetNumInstances();
		double timeInSeconds;
		
		timeInSeconds = (double) (UUrMachineTime_High() - startTime);
		timeInSeconds /= UUgMachineTime_High_Frequency;

		Imp_PrintMessage(IMPcMsg_Important, "%s processed in %f seconds."UUmNL, 
			BFrFileRef_GetLeafName(inBatchFileRef),
			timeInSeconds);	

		Imp_PrintMessage(IMPcMsg_Important, "%d instances, %d total instances"UUmNL, 
				endInstances - startInstances,
				endInstances);

	}
	
	return UUcError_None;
}

void
Imp_BatchFile_Activate_All(UUtBool inImport)
{
	tParseData	*curParseData;

	for(curParseData = gParseData; curParseData->name != 0; curParseData++)
	{
		curParseData->parseThisType = inImport;
	}

	return;
}

UUtError
Imp_BatchFile_Activate_Type(const char *inType, UUtBool inImport)
{
	tParseData	*curParseData;

	for(curParseData = gParseData; curParseData->name != 0; curParseData++)
	{
		if (UUmString_IsEqual(curParseData->name, inType))
		{
			curParseData->parseThisType = inImport;
			return UUcError_None;
		}
	}

	return UUcError_Generic;
}