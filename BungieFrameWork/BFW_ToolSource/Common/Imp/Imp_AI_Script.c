/*
	Imp_AI_Script.c
	
	Script level AI stuff
	
	Author: Quinn Dunki
	c1998 Bungie
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
	
#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_TemplateManager.h"
#include "BFW_TM_Construction.h"
#include "BFW_Group.h"
#include "BFW_MathLib.h"
#include "BFW_AppUtilities.h"
#include "BFW_Totoro.h"
#include "BFW_ScriptLang.h"

#include "Imp_Common.h"
#include "Imp_TextureFlags.h"
#include "Imp_Texture.h"
#include "Imp_AI_Script.h"
#include "Imp_AI_HHT.h"
#include "Imp_Character.h"
#include "Oni_AI_Script.h"
#include "Oni_Character.h"
#include "Oni_Level.h"


UUtUns16	IMPgAI_QuadRefRemap[2][IMPcMaxQuadRemaps];
UUtUns16	IMPgAI_QuadRemapCount = 0;
char		IMPgAI_EnvTextureDirectory[128];


static UUtError AIiScript_LoadRemapTable(
	void)
{
	char filename[64];
	UUtError error = UUcError_None;
	BFtFileRef *fileRef;
	BFtFile *outFile;
	
	// If there are no quads remapped in memory, look for them on disk
	if (!IMPgAI_QuadRemapCount)
	{
		sprintf(filename,"%s%d.tmp",IMPcRemapFilePrefix,IMPgCurLevel);
		error = BFrFileRef_MakeFromName(filename,&fileRef);
		if (error == UUcError_None)
		{
			error = BFrFile_Open(fileRef,"r",&outFile);
			if (error == UUcError_None)
			{
				error = BFrFile_Read(outFile,sizeof(UUtUns32),&IMPgAI_QuadRemapCount);
				if (error == UUcError_None)
				{
					error = BFrFile_Read(outFile,sizeof(UUtUns32)*IMPgAI_QuadRemapCount*2,IMPgAI_QuadRefRemap);
					if (error!=UUcError_None)
					{
						IMPrEnv_LogError("Unable to read quad remaps from %s",filename);
					}
					BFrFile_Close(outFile);
				}
				else IMPrEnv_LogError("Unable to read quad count from %s",filename);
			}
			else IMPrEnv_LogError("Unable to open file %s",filename);
			BFrFileRef_Dispose(fileRef);
		}
		else IMPrEnv_LogError("Unable to set up file ref for %s",filename);
	}

	return error;
}

static void AIiScript_StatementProcess_QuadRefToIndex(
	char *inParm,
	UUtBool inLowest)
{
	// Replaces a quad reference number with a quad index (either highest or lowest)
	UUtUns32 ref,lowestIdx;
	UUtUns16 i;
	UUtError error;
	
	lowestIdx = inLowest ? 0xFFFFFFFF : 0;
	error = AIiScript_LoadRemapTable();

	sscanf(inParm,"%ld",&ref);
	for (i=0; i<IMPgAI_QuadRemapCount; i++)
	{
		if (IMPgAI_QuadRefRemap[1][i] == ref)
		{
			if ((inLowest && IMPgAI_QuadRefRemap[0][i] < lowestIdx) ||
				(!inLowest && IMPgAI_QuadRefRemap[0][i] >= lowestIdx))
			{
				lowestIdx = IMPgAI_QuadRefRemap[0][i];
			}
		}
	}
	
	if ((inLowest && lowestIdx == 0xFFFFFFFF) ||
		(!inLowest && lowestIdx == 0) ||
		error != UUcError_None)
	{
		IMPrEnv_LogError("Unable to find object with ref number %d. Either that object doesn't"
			" exist or you need to import the environment again, or the remap quad file is missing",ref);
		return;
	}
	
	sprintf(inParm,"%d",lowestIdx);
}

void IMPrScript_AddQuadRemap(
	UUtUns32 inQuadIndex,
	UUtUns32 inRefNumber,
	GRtGroup *inEnvGroup)
{
	char *string;

	// Adds a quad remapping index
	if (IMPgAI_QuadRemapCount >= IMPcMaxQuadRemaps) return;
	
	UUmAssert(inQuadIndex < UUcMaxUns16);
	IMPgAI_QuadRefRemap[0][IMPgAI_QuadRemapCount] = (UUtUns16)inQuadIndex;
	IMPgAI_QuadRefRemap[1][IMPgAI_QuadRemapCount] = (UUtUns16)inRefNumber;
	IMPgAI_QuadRemapCount++;
	
	// Note the location of the texture
	GRrGroup_GetString(inEnvGroup,"textureDirectory",&string);
	UUrString_Copy(IMPgAI_EnvTextureDirectory,string,128);
}

UUtError IMPrScript_GetRemapData(
	UUtUns16 *outCount)
{
	UUtError error;

	// Force array to load
	error = AIiScript_LoadRemapTable();
	if (error != UUcError_None) return error;

	*outCount = IMPgAI_QuadRemapCount;
	return UUcError_None;
}

void IMPrScript_UnzipRemapArray(
	UUtUns32 *outRemapIndices,
	UUtUns32 *outRemaps)
{
	/**********
	* Unzips a 2D remap array into 2 1D arrays
	*/

	UUtUns32 i;

	for (i=0; i<IMPgAI_QuadRemapCount; i++)
	{
		outRemapIndices[i] = IMPgAI_QuadRefRemap[0][i];
		outRemaps[i] = IMPgAI_QuadRefRemap[1][i];
	}
}

static char *UUrTableGetNextEntry(char *curLine, char **internal)
{
	char *curField;

	curField = UUrString_Tokenize1(curLine, "\t", internal);

	if (NULL == curField) { curField = ""; }
	if (UUrString_HasPrefix(curField, "#")) { curField = ""; }

	return curField;
}


static UUtError Imp_Parse_SuperTable(
			BFtFileRef *inTableFileRef,
			AItCharacterSetupArray **outCharacterSetup,
			ONtSpawnArray **outSpawnArray,
			AItScriptTriggerClassArray **outTriggerArray,
			OBtDoorClassArray **outDoorArray)
{
	BFtTextFile *textFile;
	UUtError error = UUcError_None;
	AItCharacterSetupArray *setups;
	ONtSpawnArray *spawnArray;
	AItScriptTriggerClassArray *scriptTriggerArray;
	OBtDoorClassArray *doorArray;
	char *curLine,*string;
	UUtBool skipDoors = UUcFalse;
	UUtBool	haveID0 = UUcFalse;
	
	UUmAssertReadPtr(inTableFileRef, 1);
	UUmAssertWritePtr(outCharacterSetup, sizeof(*outCharacterSetup));

	if (!BFrFileRef_FileExists(inTableFileRef)) {
		Imp_PrintWarning("Could not find super table %s", BFrFileRef_GetLeafName(inTableFileRef));
		goto failure;
	}

	error = BFrTextFile_OpenForRead(inTableFileRef, &textFile);
	UUmError_ReturnOnError(error);

	
	// 1. arbitray white lines
	// 2. line the contains 'characters'
	// 3

	while(1)
	{
		curLine = BFrTextFile_GetNextStr(textFile);

		if (NULL == curLine) {
			Imp_PrintWarning("Unexpected end of file %s", BFrFileRef_GetLeafName(inTableFileRef));
			goto failure;
		}

		if (UUrString_IsSpace(curLine)) { continue; }

		break;
	}

	if (!UUrString_HasPrefix(curLine, "characters"))
	{
		if (NULL == curLine) {
			Imp_PrintWarning("Was expecting 'characters' found '%s'", curLine);
			goto failure;
		}
	}

	setups = UUrMemory_Block_New(sizeof(AItCharacterSetupArray));
	if (NULL == setups) {
		goto failure;
	}
	setups->numCharacterSetups = 0;

	// next line should be info line
	curLine = BFrTextFile_GetNextStr(textFile);

	while((curLine = BFrTextFile_GetNextStr(textFile)) != NULL)
	{
		char *curField;
		AItCharacterSetup *curSetup;
		char *internal = NULL;

		if (UUrString_IsSpace(curLine)) { continue; }
		if (UUrString_HasPrefix(curLine, "#")) { continue; }

		if (UUrString_HasPrefix(curLine, "spawn points")) break;

		// this line is a setup, grow the table
		setups = UUrMemory_Block_Realloc(setups, sizeof(AItCharacterSetupArray) + sizeof(AItCharacterSetup) * (setups->numCharacterSetups));
		UUrMemory_Block_VerifyList();

		curSetup = setups->characterSetups + setups->numCharacterSetups;
		setups->numCharacterSetups += 1;
		UUrMemory_Clear(curSetup, sizeof(*curSetup));
		UUrMemory_Block_VerifyList();

		// default script id
		curField = UUrTableGetNextEntry(curLine,  &internal);
		error = UUrString_To_Uns16(curField, &curSetup->defaultScriptID);
		
		if(curSetup->defaultScriptID == 0) haveID0 = UUcTrue;
		
		// default flag id
		curField = UUrTableGetNextEntry(NULL,  &internal);
		error = UUrString_To_Int16(curField, &curSetup->defaultFlagID);

		// autofreeze
		curField = UUrTableGetNextEntry(NULL,  &internal);
		if (0 == UUrString_Compare_NoCase_NoSpace(curField, "")) { }
		else if (0 == UUrString_Compare_NoCase_NoSpace(curField, "n")) { }
		else if (0 == UUrString_Compare_NoCase_NoSpace(curField, "y")) { 
			curSetup->flags |= AIcSetup_Flag_AutoFreeze; 
		}
		else {
			Imp_PrintWarning("looking for y/n got %s.  I will asssume n.", curField);
		}

		// class
		curField = UUrTableGetNextEntry(NULL,  &internal);
		error = TMrConstruction_Instance_GetPlaceHolder(TRcTemplate_CharacterClass, curField, (TMtPlaceHolder*)&(curSetup->characterClass));
		if (UUcError_None != error) {
			Imp_PrintWarning("invalid character class setup %d (id %d) str %s", setups->numCharacterSetups, curSetup->defaultScriptID, curField);
			return UUcError_Generic;
		}

		// team
		curField = UUrTableGetNextEntry(NULL,  &internal);
		error = UUrString_To_Uns16(curField, &curSetup->teamNumber);
		if (curSetup->teamNumber >= AIcMaxTeams)
		{
			Imp_PrintWarning("ERROR: Maximum team number is %d",AIcMaxTeams-1);
			curSetup->teamNumber = AIcMaxTeams-1;
		}

		// variable
		curField = UUrTableGetNextEntry(NULL,  &internal);
		strcpy(curSetup->variable, curField);

		// weapon
		curField = UUrTableGetNextEntry(NULL,  &internal);
		if (!UUrString_IsSpace(curField)) {
			error = TMrConstruction_Instance_GetPlaceHolder(WPcTemplate_WeaponClass, curField, (TMtPlaceHolder*)&(curSetup->weapon));
			UUmError_ReturnOnError(error);
		}

		// ammo
		curField = UUrTableGetNextEntry(NULL,  &internal);
		{
			char *subInternal = NULL;
			
			string = UUrString_Tokenize(curField,"/",&subInternal);
			error = UUrString_To_Uns16(string, &curSetup->ammo);
			if (error!=UUcError_None) curSetup->ammo = 0;
			
			string = UUrString_Tokenize(NULL,"/",&subInternal);
			error = UUrString_To_Uns16(string, &curSetup->leaveAmmo);
			if (error!=UUcError_None) curSetup->leaveAmmo = UUcMaxUns16;
			
			string = UUrString_Tokenize(NULL,"/",&subInternal);
			error = UUrString_To_Uns16(string, &curSetup->leaveClips);
			if (error!=UUcError_None) curSetup->leaveClips = 0;
			
			string = UUrString_Tokenize(NULL,"/",&subInternal);
			error = UUrString_To_Uns16(string, &curSetup->leaveCells);
			if (error!=UUcError_None) curSetup->leaveCells = 0;

			string = UUrString_Tokenize(NULL,"/",&subInternal);
			error = UUrString_To_Uns16(string, &curSetup->leaveHypos);
			if (error!=UUcError_None) curSetup->leaveHypos = 0;
		}
		
		/*
		 * ADD THESE BACK IN AS THEY ARE NEEDED
		 */
		
		// idle
		curField = UUrTableGetNextEntry(NULL,  &internal);
		if (!UUrString_IsSpace(curField)) {
			//error = TMrConstruction_Instance_GetPlaceHolder(AIcTemplate_Script, curField, (TMtPlaceHolder*)&(curSetup->scripts.idle));
			//UUmError_ReturnOnError(error);
		}
		// spawn
		curField = UUrTableGetNextEntry(NULL,  &internal);
		if (!UUrString_IsSpace(curField))
		{
			UUrString_Copy(curSetup->scripts.spawn_name, curField, SLcScript_MaxNameLength);
		}

		// CB: since I added several new scripts to this table they must be set up as empty here
		curSetup->scripts.flags = 0;
		curSetup->scripts.combat_name[0] = '\0';
		curSetup->scripts.die_name[0] = '\0';
		curSetup->scripts.alarm_name[0] = '\0';
		curSetup->scripts.hurt_name[0] = '\0';
		curSetup->scripts.defeated_name[0] = '\0';
		curSetup->scripts.outofammo_name[0] = '\0';
		curSetup->scripts.nopath_name[0] = '\0';

		// aggression
		curField = UUrTableGetNextEntry(NULL,  &internal);
		if (!UUrString_IsSpace(curField)) {
			//error = TMrConstruction_Instance_GetPlaceHolder(AIcTemplate_Script, curField, (TMtPlaceHolder*)&(curSetup->scripts.aggression));
			//UUmError_ReturnOnError(error);
		}

		// sound
		curField = UUrTableGetNextEntry(NULL,  &internal);
		if (!UUrString_IsSpace(curField)) {
			//error = TMrConstruction_Instance_GetPlaceHolder(AIcTemplate_Script, curField, (TMtPlaceHolder*)&(curSetup->scripts.sound));
			//UUmError_ReturnOnError(error);
		}

		// bullet
		curField = UUrTableGetNextEntry(NULL,  &internal);
		if (!UUrString_IsSpace(curField)) {
			//error = TMrConstruction_Instance_GetPlaceHolder(AIcTemplate_Script, curField, (TMtPlaceHolder*)&(curSetup->scripts.bullet));
			//UUmError_ReturnOnError(error);
		}

		// panic
		curField = UUrTableGetNextEntry(NULL,  &internal);
		if (!UUrString_IsSpace(curField)) {
			//error = TMrConstruction_Instance_GetPlaceHolder(AIcTemplate_Script, curField, (TMtPlaceHolder*)&(curSetup->scripts.panic));
			//UUmError_ReturnOnError(error);
		}

		// help
		curField = UUrTableGetNextEntry(NULL,  &internal);
		if (!UUrString_IsSpace(curField)) {
			//error = TMrConstruction_Instance_GetPlaceHolder(AIcTemplate_Script, curField, (TMtPlaceHolder*)&(curSetup->scripts.help));
			//UUmError_ReturnOnError(error);
		}

		// death
		curField = UUrTableGetNextEntry(NULL,  &internal);
		if (!UUrString_IsSpace(curField)) {
			//error = TMrConstruction_Instance_GetPlaceHolder(AIcTemplate_Script, curField, (TMtPlaceHolder*)&(curSetup->scripts.death));
			//UUmError_ReturnOnError(error);
		}

		// damage
		curField = UUrTableGetNextEntry(NULL,  &internal);
		if (!UUrString_IsSpace(curField)) {
			//error = TMrConstruction_Instance_GetPlaceHolder(AIcTemplate_Script, curField, (TMtPlaceHolder*)&(curSetup->scripts.damage));
			//UUmError_ReturnOnError(error);
		}
	}
	
	// CB: this is not an error any more, it is handled by the AI2 system in the engine
/*	if(haveID0 == UUcFalse)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Did not find character with ID 0");
	}*/
	
	spawnArray = UUrMemory_Block_New(sizeof(*spawnArray));
	if (NULL == spawnArray) {
		goto failure;
	}
	spawnArray->numSpawnFlagIDs = 0;

	while((curLine = BFrTextFile_GetNextStr(textFile)) != NULL)
	{
		char *strtokPrivate;
		char *curField;
		UUtInt16 thisSpawnPoint;

		if (UUrString_IsSpace(curLine)) { continue; }
		if (UUrString_HasPrefix(curLine, "triggers")) break;

		for(curField = UUrString_Tokenize(curLine, " \t", &strtokPrivate); 
			curField != NULL; 
			curField = UUrString_Tokenize(NULL, " \t", &strtokPrivate)) 
		{
			if (UUrString_IsSpace(curLine)) { continue; }

			error = UUrString_To_Int16(curField, &thisSpawnPoint);

			if (UUcError_None != error) {
				Imp_PrintWarning("invalid field %s in spawn points list", curField);
				continue;
			}

			spawnArray = UUrMemory_Block_Realloc(spawnArray, sizeof(ONtSpawnArray) + sizeof(UUtInt16) * spawnArray->numSpawnFlagIDs);

			if (NULL == spawnArray) {
				error = UUcError_Generic;
				UUmError_ReturnOnError(error);
			}

			spawnArray->spawnFlagIDs[spawnArray->numSpawnFlagIDs++] = thisSpawnPoint;			
		}
	}

	scriptTriggerArray = UUrMemory_Block_New(sizeof(AItScriptTriggerClassArray));
	scriptTriggerArray->numTriggers = 0;

	// parse triggers
	BFrTextFile_GetNextStr(textFile);	// skip the explanation line

	while((curLine = BFrTextFile_GetNextStr(textFile)) != NULL)
	{
		AItScriptTriggerClass curTrigger;
		char *curField;
		char *internal;
		
		if (UUrString_IsSpace(curLine)) { continue; }
		if (UUrString_HasPrefix(curLine, "doors")) break;
		if (UUrString_HasPrefix(curLine, "scripts"))
		{
			skipDoors = UUcTrue;
			break;
		}

		UUrMemory_Clear(&curTrigger, sizeof(curTrigger));

		// id	flag	radius/other	onscript	offscript	type	team	character

		// id
		curField = UUrTableGetNextEntry(curLine,  &internal);
		error = UUrString_To_Int16(curField, &curTrigger.id);

		if (UUcError_None != error || !curTrigger.id) {
			Imp_PrintWarning("trigger had invalid id");
		}

		// flag
		curField = UUrTableGetNextEntry(NULL,  &internal);
		error = UUrString_To_Int16(curField, &curTrigger.flagID);

		if (UUcError_None != error) {
			Imp_PrintWarning("trigger had invalid flag ID");
		}

		// radius/bnv/sight/refname
		curField = UUrTableGetNextEntry(NULL,  &internal);

		curTrigger.radius_h = curTrigger.radius_v = 0.f;

		if (UUmString_IsEqual_NoCase_NoSpace(curField, "bnv")) {
			curTrigger.flags |= AIcTriggerFlag_BNV;
		}
		else if (UUmString_IsEqual_NoCase_NoSpace(curField, "sight")) {
			curTrigger.flags |= AIcTriggerFlag_Sight;
		}
		else if (!isdigit(*curField))
		{
			// Unidentified string- save for later use based on type
			UUrString_Copy(curTrigger.refName,curField,AIcMaxRefName);
		}
		else {
			error = UUrString_To_FloatRange(curField, &curTrigger.radius_h,&curTrigger.radius_v);
			UUmError_ReturnOnError(error);
		}

		// onscript
		curField = UUrTableGetNextEntry(NULL,  &internal);

		if (!UUrString_IsSpace(curField)) {
			//error = TMrConstruction_Instance_GetPlaceHolder(AIcTemplate_Script, curField, (TMtPlaceHolder*)&(curTrigger.script));
			//UUmError_ReturnOnError(error);
		}

		// ADD TRIGGER SCRIPTS BACK IN WHEN NEW TOOL IS READY - BHP
		
		// offscript
		curField = UUrTableGetNextEntry(NULL,  &internal);

		if (!UUrString_IsSpace(curField)) {
			//error = TMrConstruction_Instance_GetPlaceHolder(AIcTemplate_Script, curField, (TMtPlaceHolder*)&(curTrigger.offscript));
			//UUmError_ReturnOnError(error);
		}


		// type
		curField = UUrTableGetNextEntry(NULL,  &internal);

		if (UUmString_IsEqual_NoCase_NoSpace(curField, "once")) {
			curTrigger.flags |= AIcTriggerFlag_Once;
			curTrigger.type = AIcTriggerType_Flag;
		}
		else if (UUmString_IsEqual_NoCase_NoSpace(curField, "toggle")) {
			curTrigger.flags |= AIcTriggerFlag_Toggle;
			curTrigger.type = AIcTriggerType_Flag;
		}
		else if (UUmString_IsEqual_NoCase_NoSpace(curField, "laser"))
		{
			// Refname (which is name of FX emitter to match) stored earlier
			curTrigger.type = AIcTriggerType_Laser;
			curTrigger.flags |= AIcTriggerFlag_Toggle;
		}
		else {
			//Imp_PrintWarning("trigger had an invalid type %s", curField);
			curTrigger.type = AIcTriggerType_Flag;
		}

		// team
		curField = UUrTableGetNextEntry(NULL,  &internal);

		if (UUrString_IsSpace(curField)) {
			curTrigger.team = 0xFFFF;
		}
		else {
			error = UUrString_To_Uns16(curField, &curTrigger.team);

			if (error) { 
				Imp_PrintWarning("trigger had an invalid team %s", curField);
			}
		}

		// character
		curField = UUrTableGetNextEntry(NULL,  &internal);

		if (UUrString_IsSpace(curField)) {
			curTrigger.refRestrict = 0xffff;
		}
		else {
			error = UUrString_To_Uns16(curField, &curTrigger.refRestrict);

			if (error) { 
				Imp_PrintWarning("trigger had an invalid character %s", curField);
			}
		}

		scriptTriggerArray = UUrMemory_Block_Realloc(scriptTriggerArray, sizeof(AItScriptTriggerClassArray) + sizeof(AItScriptTriggerClass) * scriptTriggerArray->numTriggers);
		scriptTriggerArray->triggers[scriptTriggerArray->numTriggers++] = curTrigger;			
	}

	// Parse doors

	doorArray = UUrMemory_Block_New(sizeof(OBtDoorClassArray));
	doorArray->numDoors = 0;
	if (!skipDoors)
	{
		BFrTextFile_GetNextStr(textFile);	// skip the explanation line

		while((curLine = BFrTextFile_GetNextStr(textFile)) != NULL)
		{
			OBtDoorClass curDoor;
			char *curField;
			char *internal;
			UUtUns16 flags = 0;

			if (UUrString_HasPrefix(curLine, "#")) { continue; }
			if (UUrString_IsSpace(curLine)) { continue; }
			if (UUrString_HasPrefix(curLine, "scripts")) break;
			
			UUrMemory_Clear(&curDoor, sizeof(curDoor));
			
			// id	open type	open anim	open key	close type	close key	radius	open snd	close snd	invis teams	flags	link
			
			
			// id
			curField = UUrTableGetNextEntry(curLine,  &internal);
			error = UUrString_To_Uns16(curField, &curDoor.id);
			if (UUcError_None != error || curDoor.id == 0) {
				Imp_PrintWarning("door had invalid id");
			}
			
			// open type
			curField = UUrTableGetNextEntry(NULL,  &internal);
			if (UUmString_IsEqual_NoCase_NoSpace(curField, "manual"))
			{
				curDoor.flags |= OBcDoorClassFlag_OpenManual;
			}
			
			// open anim
			curField = UUrTableGetNextEntry(NULL,  &internal);
			error = TMrConstruction_Instance_GetPlaceHolder(OBcTemplate_Animation,curField,(TMtPlaceHolder *)&curDoor.openCloseAnim);
			if (UUcError_None != error) {
				Imp_PrintWarning("door had invalid open animation");
				
			}
			
			// open key
			curField = UUrTableGetNextEntry(NULL,  &internal);
			error = UUrString_To_Uns16(curField, &curDoor.openKey);
			if (error!=UUcError_None) curDoor.openKey = 0;
			if (curDoor.openKey>31) Imp_PrintWarning("Error: keys must be in range 1-31");

			// close type
			curField = UUrTableGetNextEntry(NULL,  &internal);
			if (UUmString_IsEqual_NoCase_NoSpace(curField, "manual"))
			{
				curDoor.flags |= OBcDoorClassFlag_CloseManual;
			}
		
#if 0
			// close anim
			curField = UUrTableGetNextEntry(NULL,  &internal);
			error = TMrConstruction_Instance_GetPlaceHolder(OBcTemplate_Animation,curField,(TMtPlaceHolder *)&curDoor.closeAnim);
			if (UUcError_None != error) {
				Imp_PrintWarning("door had invalid close animation");
			}
#endif
			
			// close key
			curField = UUrTableGetNextEntry(NULL,  &internal);
			error = UUrString_To_Uns16(curField, &curDoor.closeKey);
			if (error!=UUcError_None) curDoor.closeKey = 0;
			if (curDoor.closeKey>31) Imp_PrintWarning("Error: keys must be in range 1-31");

			// radius
			curField = UUrTableGetNextEntry(NULL,  &internal);
			error = UUrString_To_Float(curField, &curDoor.activationRadiusSquared);
			if (error!=UUcError_None) Imp_PrintWarning("Door had invalid activation radius");
			curDoor.activationRadiusSquared *= curDoor.activationRadiusSquared;
						
			// invisible teams
			curField = UUrTableGetNextEntry(NULL,  &internal);
			UUrString_StripDoubleQuotes(curField,strlen(curField)+1);
			error = AUrParseNumericalRangeString(curField,&curDoor.invisibility,AIcMaxTeams);
			if (UUcError_None != error) curDoor.invisibility = 0;
			
			// flags
			curField = UUrTableGetNextEntry(NULL,  &internal);
			error = UUrString_To_Uns16(curField, &flags);
			if (error!=UUcError_None) flags = 0;
			curDoor.flags |= flags;

			// link
			curField = UUrTableGetNextEntry(NULL,  &internal);
			error = UUrString_To_Uns16(curField, &curDoor.linkID);
			if (error!=UUcError_None) curDoor.linkID = 0;
			
			// Create the instance
			doorArray = UUrMemory_Block_Realloc(doorArray, sizeof(OBtDoorClassArray) + sizeof(OBtDoorClass) * doorArray->numDoors);
			doorArray->doors[doorArray->numDoors++] = curDoor;			
		}
	}

	// Parse scripts
	while((curLine = BFrTextFile_GetNextStr(textFile)) != NULL)
	{
		char *curField;
		char *internal;

		if (UUrString_IsSpace(curLine)) { continue; }
		if (UUrString_HasPrefix(curLine, "#")) { continue; }

		curField = UUrString_Tokenize1(curLine, "\t", &internal);
		curField = curField ? curField : "";
	}

	// close the file
	BFrTextFile_Close(textFile);

	// return the setups
	*outCharacterSetup = setups;
	*outSpawnArray = spawnArray;
	*outTriggerArray = scriptTriggerArray;
	*outDoorArray = doorArray;

	return UUcError_None;

failure:

	return UUcError_Generic;
}

UUtError Imp_AddAIScriptTable(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtBool buildInstance;
	BFtFileRef *tableFileRef;
	UUtError error;
	AItCharacterSetupArray *srcCharacterSetup;
	ONtSpawnArray *srcSpawnArray;
	AItScriptTriggerClassArray *srcTriggerArray;
	OBtDoorClassArray *srcDoorArray;

	AItCharacterSetupArray *dstCharacterSetup;
	ONtSpawnArray *dstSpawnArray;
	AItScriptTriggerClassArray *dstTriggerArray;
	OBtDoorClassArray *dstDoorArray;

	// 1. call group to get our file
	// 2. parse file
	// 3. create instance

	error = 
		Imp_Common_BuildInstance(
			inSourceFile,
			inSourceFileModDate,
			inGroup,
			TRcTemplate_Animation,
			inInstanceName,
			UUmCompileTimeString,
			&tableFileRef,
			&buildInstance);
	UUmError_ReturnOnError(error);

	if(buildInstance != UUcTrue)
	{
		BFrFileRef_Dispose(tableFileRef);
		return UUcError_None;
	}

	error = 
		Imp_Parse_SuperTable(
			tableFileRef,
			&srcCharacterSetup,
			&srcSpawnArray,
			&srcTriggerArray,
			&srcDoorArray);
	UUmError_ReturnOnError(error);
	
	BFrFileRef_Dispose(tableFileRef);
	
	// character setup
	error = TMrConstruction_Instance_Renew(
				AIcTemplate_CharacterSetupArray,
				inInstanceName,
				srcCharacterSetup->numCharacterSetups,
				&dstCharacterSetup);
	UUmError_ReturnOnError(error);

	UUrMemory_MoveFast(
		srcCharacterSetup, 
		dstCharacterSetup, 
		sizeof(AItCharacterSetupArray) + 
		sizeof(AItCharacterSetup) * (srcCharacterSetup->numCharacterSetups - 1));

	// spawn points
	error = TMrConstruction_Instance_Renew(
				ONcTemplate_SpawnArray,
				inInstanceName,
				srcSpawnArray->numSpawnFlagIDs,
				&dstSpawnArray);
	UUmError_ReturnOnError(error);

	UUrMemory_MoveFast(
		srcSpawnArray, 
		dstSpawnArray, 
		sizeof(ONtSpawnArray) + 
		sizeof(UUtInt16) * (srcSpawnArray->numSpawnFlagIDs - 1));

	// triggers
	error = TMrConstruction_Instance_Renew(
				AIcTemplate_ScriptTriggerClassArray,
				inInstanceName,
				srcTriggerArray->numTriggers,
				&dstTriggerArray);
	UUmError_ReturnOnError(error);

	UUrMemory_MoveFast(
		srcTriggerArray, 
		dstTriggerArray, 
		sizeof(AItScriptTriggerClassArray) + 
		sizeof(AItScriptTriggerClass) * (srcTriggerArray->numTriggers - 1));

	// doors
	error = TMrConstruction_Instance_Renew(
				OBcTemplate_DoorClassArray,
				inInstanceName,
				srcDoorArray->numDoors,
				&dstDoorArray);
	UUmError_ReturnOnError(error);

	UUrMemory_MoveFast(
		srcDoorArray, 
		dstDoorArray, 
		sizeof(OBtDoorClassArray) + 
		sizeof(OBtDoorClass) * (srcDoorArray->numDoors - 1));

// free up memory
	UUrMemory_Block_Delete(srcCharacterSetup);
	UUrMemory_Block_Delete(srcSpawnArray);
	UUrMemory_Block_Delete(srcTriggerArray);
	UUrMemory_Block_Delete(srcDoorArray);

	return UUcError_None;
}
