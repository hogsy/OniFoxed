/*
	FILE:	Oni_AI2_Script.c
	
	AUTHOR:	Chris Butcher
	
	CREATED: April 04, 2000
	
	PURPOSE: AI interface to the scripting system for Oni
	
	Copyright (c) Bungie Software, 2000

*/

#include "Oni_AI2.h"
#include "Oni_Character.h"
#include "Oni_GameStatePrivate.h"
#include "BFW_ScriptLang.h"
#include "Oni_Mechanics.h"
#include "Oni_AI.h"
#include "Oni_AI_Script.h"
#include "Oni_Persistance.h"

#define ENABLE_AI2_DEBUGGING_COMMANDS		1

#define AI2cScript_TalkBufferSize		8

// ------------------------------------------------------------------------------------
// -- globals

// shows characters' names
UUtBool AI2gShowNames = UUcFalse;

// shows AI health bars
UUtBool AI2gShowHealth = UUcFalse;

// shows AI job locations
UUtBool AI2gShowJobLocations = UUcFalse;

// shows characters' combat ranges
UUtBool AI2gShowCombatRanges = UUcFalse;

// shows characters' vision cones
UUtBool AI2gShowVisionCones = UUcFalse;

// draws a line from player character to every AI
UUtBool AI2gShowLineToChar = UUcFalse;

// show pathfinding errors graphically
UUtBool AI2gShowPathfindingErrors = UUcFalse;
UUtBool AI2gShowPathfindingGrids = UUcFalse;

// show AI projectile knowledge
UUtBool AI2gShowProjectiles = UUcFalse;

// show AI firing spread knowledge
UUtBool AI2gShowFiringSpreads = UUcFalse;

// show AI sounds when they are generated
UUtBool AI2gShowSounds = UUcFalse;

// show fights that are in progress
UUtBool AI2gShowFights = UUcFalse;

// AI knowledge control
UUtBool AI2gBlind = UUcFalse;
UUtBool AI2gDeaf = UUcFalse;
UUtBool AI2gIgnorePlayer = UUcFalse;

// lets Barabbas run
UUtBool AI2gBarabbasRun = UUcTrue;

// enables 'boss' target selection behavior
UUtBool AI2gBossBattle = UUcFalse;

// makes all characters passive
UUtBool AI2gEveryonePassive = UUcFalse;

// stops the chump
UUtBool AI2gChumpStop = UUcFalse;

// turns on and off non-attack melee-technique weight normalization
UUtBool AI2gMeleeWeightCorrection = UUcTrue;

// debugging display
UUtBool AI2gDrawLaserSights = UUcFalse;
UUtBool AI2gDebug_ShowPrediction = UUcFalse;
UUtBool AI2gDebug_ShowTargeting = UUcFalse;
UUtBool AI2gDebug_LocalMovement = UUcFalse;
UUtBool AI2gDebug_LocalPathfinding = UUcFalse;
UUtBool AI2gDebugDisplayGraph = UUcFalse;
UUtBool AI2gDebugShowSettingIDs = UUcFalse;
UUtBool AI2gDebugSpawning = UUcFalse;
UUtBool AI2gDebug_ShowLOS = UUcFalse;
UUtBool AI2gDebug_ShowLocalMelee = UUcFalse;
UUtBool AI2gDebug_ShowActivationPaths = UUcFalse;
UUtBool AI2gDebug_ShowAstarEvaluation = UUcFalse;

// spacing behavior controls
UUtBool AI2gSpacingEnable = UUcTrue;
UUtInt32 AI2gSpacingMaxCookies = 2;
UUtBool AI2gSpacingWeights = UUcTrue;

// shooting skill editing
UUtBool AI2gSkillEdit_Active = UUcFalse;
ONtCharacterClass *AI2gSkillEdit_TargetClass;
WPtWeaponClass *AI2gSkillEdit_TargetWeapon;
UUtUns32 AI2gSkillEdit_TargetIndex;
AI2tShootingSkill *AI2gSkillEdit_TargetPtr;
AI2tShootingSkill AI2gSkillEdit_SavedCopy;

// string storage space for chr_talk script command
UUtBool AI2gScript_TalkBufferInUse[AI2cScript_TalkBufferSize];
char AI2gScript_TalkBuffer[AI2cScript_TalkBufferSize][BFcMaxFileNameLength];

// AI knowledge promotion controls (from ignore to interesting)
UUtInt32 AI2gSound_StopIgnoring_Events = 6;
UUtInt32 AI2gSound_StopIgnoring_Time = 240;

#if TOOL_VERSION
UUtUns32 AI2gScript_TriggerVolume_Callback_Count;
#endif

// ------------------------------------------------------------------------------------
// -- internal function prototypes

static void AI2iScript_CancelEnvAnim(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);

// ------------------------------------------------------------------------------------
// -- controller code

typedef struct AI2tCommandTable
{
	const char *name;
	const char *name2;
	const char *name3;
	const char *description;
	const char *parameters;
	SLtEngineCommand function;
} AI2tCommandTable;

typedef struct AI2tVariableTable
{
	const char *name;
	SLtType type;
	const char *description;
	void *ptr;
} AI2tVariableTable;



// ------------------------------------------------------------------------------------
// -- utility functions

ONtCharacter *AI2rScript_ParseCharacter(SLtParameter_Actual *inParameter, UUtBool inExhaustive)
{
	UUtUns16 script_id;
	UUtError error;

	if (inParameter->type == SLcType_Int32) {
		error = UUcError_None;
		script_id = (UUtUns16) inParameter->val.i;

	} else if (inParameter->type == SLcType_String) {
		error = UUrString_To_Uns16(inParameter->val.str, &script_id);

	} else {
		// not a character
		return NULL;
	}

	if (error == UUcError_None) {
		return AIrUtility_New_GetCharacterFromID(script_id, inExhaustive);
	} else {
		return AI2rFindCharacter(inParameter->val.str, inExhaustive);
	}
}

void AI2rScript_NoCharacterError(SLtParameter_Actual *inParameter, char *inName, SLtErrorContext *ioContext)
{
	AI2_ERROR(AI2cWarning, AI2cSubsystem_HighLevel, AI2cError_HighLevel_NoCharacter, NULL, inName, inParameter, 0, 0);
}

static UUcInline ONtCharacter *AI2rScript_ParseAI(UUtUns32 inNumParams, SLtParameter_Actual *inParameterList)
{
	ONtCharacter *character;

	UUmAssert((inNumParams > 0) && (inNumParams < 100));
	character = AI2rScript_ParseCharacter(inParameterList, UUcFalse);

	if ((character != NULL) && (character->charType == ONcChar_AI2)) {
		return character;
	} else {
		return NULL;
	}
}

static UUcInline ONtCharacter *AI2rScript_ParseOptionalAI(UUtUns32 inNumParams, SLtParameter_Actual *inParameterList)
{
	return ((inNumParams > 0) && (inNumParams < 100)) ? AI2rScript_ParseAI(inNumParams, inParameterList) : NULL;
}

// ------------------------------------------------------------------------------------
// -- weapon skill editing functions

void AI2rScript_ClearSkillEdit(void)
{
	AI2gSkillEdit_Active = UUcFalse;
	AI2gSkillEdit_TargetClass = NULL;
	AI2gSkillEdit_TargetWeapon = NULL;
	AI2gSkillEdit_TargetPtr = NULL;
	AI2gSkillEdit_TargetIndex = 0;
	UUrMemory_Clear(&AI2gSkillEdit_SavedCopy, sizeof(AI2tShootingSkill));
}

// select a character and weapon to edit
// syntax: ai2_skill_select striker_easy_1 w5_sbg
static UUtError	AI2iScript_Skill_Select(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
								SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
								UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	UUtError error;

	// clear skill editing
	AI2rScript_ClearSkillEdit();

	error = TMrInstance_GetDataPtr(TRcTemplate_CharacterClass, inParameterList[0].val.str, &AI2gSkillEdit_TargetClass);
	if (error != UUcError_None) {
		COrConsole_Printf("### ai2_skill_select: can't find character class '%s'", inParameterList[0].val.str);
		return UUcError_None;
	}

	error = TMrInstance_GetDataPtr(WPcTemplate_WeaponClass, inParameterList[1].val.str, &AI2gSkillEdit_TargetWeapon);
	if (error != UUcError_None) {
		COrConsole_Printf("### ai2_skill_select: can't find weapon class '%s'", inParameterList[1].val.str);
		return UUcError_None;
	}

	AI2gSkillEdit_TargetIndex = AI2gSkillEdit_TargetWeapon->ai_parameters.shootskill_index;
	if ((AI2gSkillEdit_TargetIndex < 0) || (AI2gSkillEdit_TargetIndex >= AI2cCombat_MaxWeapons)) {
		COrConsole_Printf("### ai2_skill_select: weapon '%s' has shootskill_index %d which exceeds maxweapons %d",
							inParameterList[1].val.str, AI2gSkillEdit_TargetIndex, AI2cCombat_MaxWeapons - 1);
		return UUcError_None;
	}

	AI2gSkillEdit_TargetPtr = &AI2gSkillEdit_TargetClass->ai2_behavior.combat_parameters.shooting_skill[AI2gSkillEdit_TargetIndex];
	UUrMemory_MoveFast(AI2gSkillEdit_TargetPtr, &AI2gSkillEdit_SavedCopy, sizeof(AI2tShootingSkill));

	AI2gSkillEdit_Active = UUcTrue;
	COrConsole_Printf("Now editing '%s' shooting skill with weapon %d '%s'", 
		TMrInstance_GetInstanceName(AI2gSkillEdit_TargetClass), AI2gSkillEdit_TargetIndex, TMrInstance_GetInstanceName(AI2gSkillEdit_TargetWeapon));

	return UUcError_None;
}

// show current skill set being edited
// syntax: ai2_skill_show
static UUtError	AI2iScript_Skill_Show(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
								SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
								UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	if (!AI2gSkillEdit_Active) {
		COrConsole_Printf("cannot show skill, no shooting skill currently selected for editing");
		return UUcError_None;
	}

	COrConsole_Printf("'%s' shooting skill with weapon %d '%s':", TMrInstance_GetInstanceName(AI2gSkillEdit_TargetClass),
					AI2gSkillEdit_TargetIndex, TMrInstance_GetInstanceName(AI2gSkillEdit_TargetWeapon));
	COrConsole_Printf("  recoil compensation: %.2f", AI2gSkillEdit_TargetPtr->recoil_compensation);
	COrConsole_Printf("  best aiming angle: %.2f degrees", MUrASin(AI2gSkillEdit_TargetPtr->best_aiming_angle) * M3cRadToDeg);
	COrConsole_Printf("  shot error: %.2f", AI2gSkillEdit_TargetPtr->shot_error);
	COrConsole_Printf("  shot decay: %.2f", AI2gSkillEdit_TargetPtr->shot_decay);
	COrConsole_Printf("  gun inaccuracy: %.2f", AI2gSkillEdit_TargetPtr->gun_inaccuracy);
	COrConsole_Printf("  delay frames: %d - %d", AI2gSkillEdit_TargetPtr->delay_min, AI2gSkillEdit_TargetPtr->delay_max);

	return UUcError_None;
}

// revert current skill set being edited
// syntax: ai2_skill_revert
static UUtError	AI2iScript_Skill_Revert(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
								SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
								UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	if (!AI2gSkillEdit_Active) {
		COrConsole_Printf("cannot revert, no shooting skill currently selected for editing");
		return UUcError_None;
	}

	COrConsole_Printf("reverting '%s' shooting skill with weapon %d '%s' to saved version", TMrInstance_GetInstanceName(AI2gSkillEdit_TargetClass),
					AI2gSkillEdit_TargetIndex, TMrInstance_GetInstanceName(AI2gSkillEdit_TargetWeapon));
	UUrMemory_MoveFast(&AI2gSkillEdit_SavedCopy, AI2gSkillEdit_TargetPtr, sizeof(AI2tShootingSkill));

	return UUcError_None;
}

// write the current skill set being edited out to a text file
// syntax: ai2_skill_save
static UUtError	AI2iScript_Skill_Save(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
								SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
								UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	FILE *outfile;

	if (!AI2gSkillEdit_Active) {
		COrConsole_Printf("cannot save skill, no shooting skill currently selected for editing");
		return UUcError_None;
	}

	outfile = fopen("shooting_skill.txt", "wt");
	if (outfile == NULL) {
		COrConsole_Printf("### cannot write to file shooting_skill.txt, disk may be full");
		return UUcError_None;
	}

	fprintf(outfile, "# AI2 shooting skill parameters automatically generated from in-engine\n");
	fprintf(outfile, "# skill for %s with weapon %s:\n", TMrInstance_GetInstanceName(AI2gSkillEdit_TargetClass),
			TMrInstance_GetInstanceName(AI2gSkillEdit_TargetWeapon));
	fprintf(outfile, "$wp%d_recoil_compensation = %.2f\n", AI2gSkillEdit_TargetIndex, AI2gSkillEdit_TargetPtr->recoil_compensation);
	fprintf(outfile, "$wp%d_best_aiming_angle = %.2f\n", AI2gSkillEdit_TargetIndex, MUrASin(AI2gSkillEdit_TargetPtr->best_aiming_angle) * M3cRadToDeg);
	fprintf(outfile, "$wp%d_shot_error = %.2f\n", AI2gSkillEdit_TargetIndex, AI2gSkillEdit_TargetPtr->shot_error);
	fprintf(outfile, "$wp%d_shot_decay = %.2f\n", AI2gSkillEdit_TargetIndex, AI2gSkillEdit_TargetPtr->shot_decay);
	fprintf(outfile, "$wp%d_inaccuracy = %.2f\n", AI2gSkillEdit_TargetIndex, AI2gSkillEdit_TargetPtr->gun_inaccuracy);
	fprintf(outfile, "$wp%d_delay_min = %d\n", AI2gSkillEdit_TargetIndex, AI2gSkillEdit_TargetPtr->delay_min);
	fprintf(outfile, "$wp%d_delay_max = %d\n", AI2gSkillEdit_TargetIndex, AI2gSkillEdit_TargetPtr->delay_max);
	fprintf(outfile, "\n");
	fclose(outfile);

	// save the current settings so that we can revert to them if we have to
	UUrMemory_MoveFast(AI2gSkillEdit_TargetPtr, &AI2gSkillEdit_SavedCopy, sizeof(AI2tShootingSkill));
	COrConsole_Printf("wrote current settings out to shooting_skill.txt");

	return UUcError_None;
}

// set shooting skill recoil compensation
// syntax: ai2_skill_recoil 0.2
static UUtError	AI2iScript_Skill_Recoil(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
								SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
								UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	if (!AI2gSkillEdit_Active) {
		COrConsole_Printf("cannot set recoil compensation, no shooting skill currently selected for editing");
		return UUcError_None;
	}

	AI2gSkillEdit_TargetPtr->recoil_compensation = inParameterList[0].val.f;
	COrConsole_Printf("recoil compensation now %.2f", AI2gSkillEdit_TargetPtr->recoil_compensation);

	return UUcError_None;
}

// set shooting skill best aiming angle
// syntax: ai2_skill_bestangle 0.5
static UUtError	AI2iScript_Skill_BestAngle(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
								SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
								UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	float theta;

	if (!AI2gSkillEdit_Active) {
		COrConsole_Printf("cannot set best aiming angle, no shooting skill currently selected for editing");
		return UUcError_None;
	}

	theta = UUmPin(inParameterList[0].val.f, 0.0f, 30.0f) * M3cDegToRad;
	AI2gSkillEdit_TargetPtr->best_aiming_angle = MUrSin(theta);
	COrConsole_Printf("best aiming angle now %.2f degrees", MUrASin(AI2gSkillEdit_TargetPtr->best_aiming_angle) * M3cRadToDeg);

	return UUcError_None;
}

// set shooting skill shot error
// syntax: ai2_skill_error 0.5
static UUtError	AI2iScript_Skill_Error(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
								SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
								UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	if (!AI2gSkillEdit_Active) {
		COrConsole_Printf("cannot set shot error, no shooting skill currently selected for editing");
		return UUcError_None;
	}

	AI2gSkillEdit_TargetPtr->shot_error = inParameterList[0].val.f;
	COrConsole_Printf("shot error now %.2f", AI2gSkillEdit_TargetPtr->shot_error);

	return UUcError_None;
}

// set shooting skill shot decay
// syntax: ai2_skill_decay 0.5
static UUtError	AI2iScript_Skill_Decay(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
								SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
								UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	if (!AI2gSkillEdit_Active) {
		COrConsole_Printf("cannot set shot decay, no shooting skill currently selected for editing");
		return UUcError_None;
	}

	AI2gSkillEdit_TargetPtr->shot_decay = inParameterList[0].val.f;
	COrConsole_Printf("shot decay now %.2f", AI2gSkillEdit_TargetPtr->shot_decay);

	return UUcError_None;
}

// set shooting skill inaccuracy
// syntax: ai2_skill_inaccuracy 1.5
static UUtError	AI2iScript_Skill_Inaccuracy(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
								SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
								UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	if (!AI2gSkillEdit_Active) {
		COrConsole_Printf("cannot set inaccuracy, no shooting skill currently selected for editing");
		return UUcError_None;
	}

	AI2gSkillEdit_TargetPtr->gun_inaccuracy = inParameterList[0].val.f;
	COrConsole_Printf("inaccuracy now %.2f", AI2gSkillEdit_TargetPtr->gun_inaccuracy);

	return UUcError_None;
}

// set shooting skill min delay frames
// syntax: ai2_skill_delaymin 30
static UUtError	AI2iScript_Skill_DelayMin(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
								SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
								UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	if (!AI2gSkillEdit_Active) {
		COrConsole_Printf("cannot set delay min frames, no shooting skill currently selected for editing");
		return UUcError_None;
	}

	AI2gSkillEdit_TargetPtr->delay_min = (UUtUns16) UUmMax(0, inParameterList[0].val.i);
	AI2gSkillEdit_TargetPtr->delay_max = UUmMax(AI2gSkillEdit_TargetPtr->delay_min, AI2gSkillEdit_TargetPtr->delay_max);
	COrConsole_Printf("delay frames now %d - %d", AI2gSkillEdit_TargetPtr->delay_min, AI2gSkillEdit_TargetPtr->delay_max);

	return UUcError_None;
}

// set shooting skill max delay frames
// syntax: ai2_skill_delaymax 90
static UUtError	AI2iScript_Skill_DelayMax(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
								SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
								UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	if (!AI2gSkillEdit_Active) {
		COrConsole_Printf("cannot set delay max frames, no shooting skill currently selected for editing");
		return UUcError_None;
	}

	AI2gSkillEdit_TargetPtr->delay_max = (UUtUns16) UUmMax(0, inParameterList[0].val.i);
	AI2gSkillEdit_TargetPtr->delay_min = UUmMin(AI2gSkillEdit_TargetPtr->delay_min, AI2gSkillEdit_TargetPtr->delay_max);
	COrConsole_Printf("delay frames now %d - %d", AI2gSkillEdit_TargetPtr->delay_min, AI2gSkillEdit_TargetPtr->delay_max);

	return UUcError_None;
}

// ------------------------------------------------------------------------------------
// -- callback functions

// reset all mechanics
static UUtError
AI2iScript_Mechanics_Reset(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	ONrMechanics_Reset();
	return UUcError_None;
}


// reset all AIs
// syntax: ai2_reset
static UUtError	AI2iScript_Reset(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
								SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
								UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	UUtBool reset_player = UUcFalse;
	UUtError error;

	if (inParameterListLength >= 1) {
		reset_player = (UUtBool) inParameterList[0].val.i;
	}
	
	AI2rDeleteAllCharacters(reset_player);

	AI2rKnowledge_Reset();

	error = AI2rStartAllCharacters(reset_player, UUcFalse);
	UUmError_ReturnOnError(error);

	error = AI2iScript_Mechanics_Reset( inErrorContext, 0, inParameterList, outTicksTillCompletion, outStall, ioReturnValue);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// start an AI
// syntax: ai2_spawn thug2
static UUtError	AI2iScript_Spawn(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
								SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
								UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	UUtBool reset_player = UUcFalse, force_spawn;
	UUtError error;

	UUmAssert(inParameterListLength >= 1);

	force_spawn = ((inParameterListLength >= 2) && UUmString_IsEqual(inParameterList[1].val.str, "force"));

	error = AI2rSpawnCharacter(inParameterList[0].val.str, force_spawn);
	UUmError_ReturnOnError(error);

	if (AI2gDebugSpawning) {
		char buffer[256], tempbuf[128];

		sprintf(buffer, "SPAWN: %s", inParameterList[0].val.str);
		if (ONgCurrentlyActiveTrigger != NULL) {
			sprintf(tempbuf, " (called from trigger %s, script %s, caused by %s)", ONgCurrentlyActiveTrigger->name,
					ONgCurrentlyActiveTrigger_Script, ONgCurrentlyActiveTrigger_Cause->player_name);
			strcat(buffer, tempbuf);
		}
		COrConsole_Printf_Color(UUcTrue, 0xFFC06000, 0x806030, buffer);
	}

	return UUcError_None;
}

// spawn all AIs (used for debugging)
// syntax: ai2_spawnall
static UUtError	AI2iScript_SpawnAll(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
								SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
								UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	UUtError error;

	AI2rDeleteAllCharacters(UUcFalse);

	error = AI2rStartAllCharacters(UUcFalse, UUcTrue);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// tell one or more AIs to come here
// syntax: ai2_comehere [character]
static UUtError	AI2iScript_Debug_ComeHere(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
								SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
								UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	AI2rDebug_ComeHere(AI2rScript_ParseOptionalAI(inParameterListLength, inParameterList));
	return UUcError_None;
}

// tell one or more AIs to look at me
// syntax: ai2_lookatme {character list}
static UUtError	AI2iScript_Debug_LookHere(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
								SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
								UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	AI2rDebug_LookHere(AI2rScript_ParseOptionalAI(inParameterListLength, inParameterList));
	return UUcError_None;
}

// tell one or more AIs to follow the player character
// syntax: ai2_followme				(everyone)
//         ai2_followme thug_28
static UUtError	AI2iScript_Debug_FollowMe(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
								SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
								UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseOptionalAI(inParameterListLength, inParameterList);

	if (character == NULL) {
		AI2rScript_NoCharacterError(&inParameterList[0], "ai2_followme", inErrorContext);
	} else {
		AI2rDebug_FollowMe(character);
	}

	return UUcError_None;
}

// set a character's current state to be its job
// syntax: ai2_make_current_state_job mr_man
static UUtError	AI2iScript_MakeCurrentStateJob(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseAI(inParameterListLength, inParameterList);

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "ai2_setjobstate", inErrorContext);
		return UUcError_None;
	} else {
		if (character->ai2State.currentState == &(character->ai2State.jobState)) {
			UUmAssert(character->ai2State.currentGoal == character->ai2State.jobGoal);

		} else if (character->ai2State.currentState != NULL) {
			character->ai2State.jobGoal = character->ai2State.currentGoal;
			character->ai2State.jobState = *(character->ai2State.currentState);

			// we need to reset our job location for our new job
			character->ai2State.has_job_location = UUcFalse;
		}

		return UUcError_None;
	}
}

// make all characters passive
// syntax: ai2_allpassive 1
static UUtError	AI2iScript_AllPassive(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	UUmAssert(inParameterListLength >= 1);

	AI2rAllPassive((UUtBool) inParameterList[0].val.i);
	return UUcError_None;
}

// make a character passive
// syntax: ai2_passive mr_man 1
static UUtError	AI2iScript_Passive(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseAI(inParameterListLength, inParameterList);

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "ai2_passive", inErrorContext);
		return UUcError_None;
	} else {
		UUmAssert(inParameterListLength >= 2);
		AI2rPassive(character, (UUtBool) inParameterList[1].val.i);
		return UUcError_None;
	}
}

// force a character to transition into a patrol state
// syntax: ai2_dopath mr_man patrol_17
static UUtError	AI2iScript_SetState_Patrol(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseAI(inParameterListLength, inParameterList);
	UUtError error;

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "ai2_dopath", inErrorContext);
		return UUcError_None;
	} else {
		UUmAssert(inParameterListLength >= 2);

		if (character->ai2State.flags & AI2cFlag_Passive) {
			// we can't set this character's state
			COrConsole_Printf("### AI %s is currently passive, cannot use ai_dopath", character->player_name);
			return UUcError_None;
		}

		error = AI2rPatrol_FindByName(inParameterList[1].val.str, NULL);
		if (error != UUcError_None) {
			COrConsole_Printf("### ai2_dopath cannot find patrol path '%s'", inParameterList[1].val.str);
			return UUcError_None;
		}

		AI2rExitState(character);

		// force the character to start a patrol
		character->ai2State.currentGoal = AI2cGoal_Patrol;
		AI2rPatrol_Reset(character, &character->ai2State.currentStateBuffer.state.patrol);
		error = AI2rPatrol_FindByName(inParameterList[1].val.str, &character->ai2State.currentStateBuffer.state.patrol.path);
		if (error != UUcError_None) {
			COrConsole_Printf("### error reading data from patrol path '%s', setting into idle", inParameterList[1].val.str);
			character->ai2State.currentGoal = AI2cGoal_Idle;
		}
		character->ai2State.currentState = &(character->ai2State.currentStateBuffer);
		character->ai2State.currentState->begun = UUcFalse;

		AI2rEnterState(character);

		return UUcError_None;
	}
}

// set up a character's alarm behavior
// syntax: ai2_doalarm mr_man patrol_17
static UUtError	AI2iScript_SetState_Alarm(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseAI(inParameterListLength, inParameterList);
	ONtActionMarker *action_marker;
	OBJtObject *console_object;
	OBJtOSD_Console *console_osd;

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "ai2_doalarm", inErrorContext);
		return UUcError_None;
	} else {
		if (character->ai2State.flags & AI2cFlag_Passive) {
			// we can't set this character's state
			COrConsole_Printf("### AI %s is currently passive, ai2_doalarm cannot execute", character->player_name);
			return UUcError_None;
		}

#if DEBUG_VERBOSE_ALARM
		COrConsole_Printf("%s: running ai2_doalarm", character->player_name);
#endif

		if ((inParameterListLength < 2) || (inParameterList[1].val.i == 0)) {
			// no alarm console specified, find the nearest
			action_marker = AI2rAlarm_FindAlarmConsole(character);

#if DEBUG_VERBOSE_ALARM
			COrConsole_Printf("%s: action_marker %sfound (%f %f %f)", character->player_name, 
				(action_marker == NULL) ? "not " : "",
				(action_marker == NULL) ? 0 : action_marker->position.x,
				(action_marker == NULL) ? 0 : action_marker->position.y,
				(action_marker == NULL) ? 0 : action_marker->position.z);
#endif
		} else {
			// a specific console ID was given
			console_object = OBJrConsole_GetByID((UUtUns16) inParameterList[1].val.i);
			if (console_object == NULL) {
				// we can't set this character's state
				COrConsole_Printf("### ai2_doalarm could not find console %d", inParameterList[1].val.i);
				return UUcError_None;
			}

			UUmAssert(console_object->object_type == OBJcType_Console);
			console_osd = (OBJtOSD_Console *) console_object->object_data;

			if (AI2rAlarm_IsValidConsole(character, console_osd)) {
#if DEBUG_VERBOSE_ALARM
				COrConsole_Printf("%s: specified console %d, is valid (%d)",
									character->player_name, inParameterList[1].val.i, console_osd->id);
#endif
				action_marker = ((OBJtOSD_Console *) console_object->object_data)->action_marker;
			} else {
#if DEBUG_VERBOSE_ALARM
				COrConsole_Printf("%s: specified console %d, not a valid console (inactive or not triggered)",
								character->player_name, inParameterList[1].val.i);
#endif
				action_marker = NULL;
				return UUcError_None;
			}
		}

		if (action_marker != NULL) {
			// set up an alarm behavior
#if DEBUG_VERBOSE_ALARM
			if (character->ai2State.alarmStatus.action_marker == NULL) {
				COrConsole_Printf("%s: not currently running for alarm, setting up new alarm behavior", character->player_name);
			} else if (character->ai2State.alarmStatus.action_marker == action_marker) {
				COrConsole_Printf("%s: we are already running for this alarm", character->player_name);
			} else {
				COrConsole_Printf("%s: already running for an alarm, changing target console", character->player_name);
			}
#endif
			AI2rAlarm_Setup(character, action_marker);
		} else {
#if DEBUG_VERBOSE_ALARM
			COrConsole_Printf("%s: no console found, could not set up alarm behavior", character->player_name);
#endif
		}

#if DEBUG_VERBOSE_ALARM
		if (character->ai2State.currentGoal == AI2cGoal_Alarm) {
			UUmAssertReadPtr(character->ai2State.alarmStatus.action_marker, sizeof(ONtActionMarker));
			COrConsole_Printf("%s: ai2_doalarm completed, running for alarm at (%f %f %f)", 
				character->player_name, character->ai2State.alarmStatus.action_marker->position.x,
				character->ai2State.alarmStatus.action_marker->position.y, character->ai2State.alarmStatus.action_marker->position.z);
		} else {
			COrConsole_Printf("%s: ai2_doalarm completed, but not running for alarm (behavior is %s)", 
				character->player_name, AI2cGoalName[character->ai2State.currentGoal]);
		}
#endif
		return UUcError_None;
	}
}

// makes a character aware of another character
// syntax: ai2_makeaware mr_man target
static UUtError	AI2iScript_MakeAware(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseAI(inParameterListLength, inParameterList);
	ONtCharacter *target = AI2rScript_ParseCharacter(&inParameterList[1], UUcFalse);

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "ai2_makeaware", inErrorContext);
		return UUcError_None;

	} else if (target == NULL) {
		AI2rScript_NoCharacterError(&inParameterList[1], "ai2_makeaware (target)", inErrorContext);
		return UUcError_None;
	}

	AI2rKnowledge_CreateMagicAwareness(character, target, UUcFalse, UUcFalse);

	return UUcError_None;
}

// force a character to attack another character. bypasses startle.
// syntax: ai2_attack mr_man target
static UUtError	AI2iScript_Attack(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseAI(inParameterListLength, inParameterList);
	ONtCharacter *target = AI2rScript_ParseCharacter(&inParameterList[1], UUcFalse);

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "ai2_attack", inErrorContext);
		return UUcError_None;

	} else if (target == NULL) {
		AI2rScript_NoCharacterError(&inParameterList[1], "ai2_attack (target)", inErrorContext);
		return UUcError_None;
	}

	AI2rKnowledge_CreateMagicAwareness(character, target, UUcFalse, UUcTrue);

	return UUcError_None;
}

// makes barabbas retrieve his gun if it is lost off the roof of TCTF HQ
// syntax: ai2_barabbas_retrievegun barabbas
static UUtError	AI2iScript_BarabbasRetrieveGun(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseAI(inParameterListLength, inParameterList);
	M3tVector3D weapon_pos;
	WPtWeapon *weapon;

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "ai2_barabbas_retrievegun", inErrorContext);
		return UUcError_None;
	}
	if (character->inventory.weapons[0] != NULL) {
		return UUcError_None;
	}

	weapon = WPrFindBarabbasWeapon();
	if (weapon == NULL) {
		COrConsole_Printf("### ai2_barabbas_retrievegun: can't find barabbas's weapon");
		return UUcError_None;

	} else if (WPrGetOwner(weapon) != NULL) {
		// can't teleport the weapon, someone is holding it
		return UUcError_None;
	}

	WPrGetPosition(weapon, &weapon_pos);
	if (weapon_pos.y > 700.0f) {
		// can't teleport the weapon, it is already on the top floor of the building
		return UUcError_None;
	}

	ONrCharacter_PickupWeapon(character, weapon, UUcFalse);

	return UUcError_None;
}

// force a character to transition into an idle state
// syntax: ai2_idle mr_man
static UUtError	AI2iScript_SetState_Idle(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseAI(inParameterListLength, inParameterList);

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "ai2_idle", inErrorContext);
		return UUcError_None;
	} else {
		if (character->ai2State.flags & AI2cFlag_Passive) {
			// we can't set this character's state
			SLrScript_ReportError(inErrorContext, "AI %s is currently passive, cannot set state", inParameterList[0].val.str);
			return UUcError_None;
		}

		AI2rExitState(character);

		// force the character to enter the idle state
		character->ai2State.currentGoal = AI2cGoal_Idle;
		character->ai2State.currentState = &(character->ai2State.currentStateBuffer);
		character->ai2State.currentState->begun = UUcFalse;

		AI2rEnterState(character);

		return UUcError_None;
	}
}

// manually specify a character's movement mode
// syntax: ai2_setmovementmode mr_man run_noaim
static UUtError	AI2iScript_Debug_SetMovementMode(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(inParameterList, UUcFalse);
	UUtUns32 itr;

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "ai2_setmovementmode", inErrorContext);
		return UUcError_None;
	} else {
		if ((character->flags & ONcCharacterFlag_AIMovement) == 0) {
			COrConsole_Printf("### ai2_setmovementmode: cannot move player character without first using ai2_takecontrol 1");
			return UUcError_None;
		}
		
		UUmAssert(inParameterListLength >= 2);

		for (itr = 0; itr < AI2cMovementMode_Max; itr++) {
			if (strcmp(inParameterList[1].val.str, AI2cMovementModeName[itr]) == 0) {
				AI2rMovement_ChangeMovementMode(character, (AI2tMovementMode) itr);
				return UUcError_None;
			}
		}

		SLrScript_ReportError(inErrorContext, "unknown movement mode '%s'", inParameterList[1].val.str);
		return UUcError_None;
	}
}

// force an AI to move to a flag
// syntax: ai2_movetoflag mr_man 1007
//         ai2_movetoflag mr_man 1009 setfacing
static UUtError	AI2iScript_MoveToFlag(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(inParameterList, UUcFalse);
	UUtInt32 flag_id = inParameterList[1].val.i;
	ONtFlag	flag;
	UUtBool found, setfacing = UUcFalse;
	
	found = ONrLevel_Flag_ID_To_Flag((UUtUns16) flag_id, &flag);
	
	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "ai2_movetoflag", inErrorContext);
		return UUcError_None;

	} else if (!found) {
		SLrScript_ReportError(inErrorContext, "flag %d does not exist", flag_id);
		return UUcError_None;

	} else {
		if ((character->flags & ONcCharacterFlag_AIMovement) == 0) {
			COrConsole_Printf("### ai2_movetoflag: cannot move player character without first using ai2_takecontrol 1");
			return UUcError_None;
		}

		if (inParameterListLength >= 3) {
			setfacing = UUmString_IsEqual(inParameterList[2].val.str, "setfacing");
		}

		AI2rPath_GoToPoint(character, &flag.location, AI2cMoveToPoint_DefaultDistance, UUcTrue, (setfacing) ? flag.rotation : AI2cAngle_None, NULL);
		return UUcError_None;
	}
}

// force an AI to look at a character
// syntax: ai2_lookatchar mr_man konoko
static UUtError	AI2iScript_LookAtChar(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(&inParameterList[0], UUcFalse);
	ONtCharacter *target = AI2rScript_ParseCharacter(&inParameterList[1], UUcTrue);
	float angle;

	if (character == NULL) {
		AI2rScript_NoCharacterError(&inParameterList[0], "ai2_lookatchar", inErrorContext);
		return UUcError_None;

	} else if (target == NULL) {
		AI2rScript_NoCharacterError(&inParameterList[1], "ai2_lookatchar", inErrorContext);
		return UUcError_None;

	} else {
		if ((character->flags & ONcCharacterFlag_AIMovement) == 0) {
			COrConsole_Printf("### ai2_lookatchar: cannot move player character without first using ai2_takecontrol 1");
			return UUcError_None;
		}

		// calculate the relative angle
		angle = MUrATan2(target->actual_position.x - character->actual_position.x, 
						 target->actual_position.z - character->actual_position.z);
		UUmTrig_ClipLow(angle);

		// set the facing and look at the character
		AI2rPath_GoToPoint(character, &character->actual_position, AI2cMoveToPoint_DefaultDistance, UUcTrue, angle, NULL);
		AI2rMovement_LookAtCharacter(character, target);
		return UUcError_None;
	}
}

// give a character a weapon
// syntax: chr_giveweapon mr_man w3_phr
static UUtError	AI2iScript_GiveWeapon(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(inParameterList, UUcFalse);
	WPtWeaponClass *weapon_class;
	UUtError error;

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "chr_giveweapon", inErrorContext);
		return UUcError_None;
	} else {
		UUmAssert(inParameterListLength >= 2);

		error = TMrInstance_GetDataPtr(WPcTemplate_WeaponClass, inParameterList[1].val.str, (void **) &weapon_class);
		if (error != UUcError_None) {
			SLrScript_ReportError(inErrorContext, "no such weapon name '%s'!", inParameterList[1].val.str);
			return UUcError_None;
		}

		ONrCharacter_UseNewWeapon(character, weapon_class);
		return UUcError_None;
	}
}

// give a character a powerup
// syntax: chr_givepowerup mr_man ammo
static UUtError	AI2iScript_GivePowerup(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(inParameterList, UUcFalse);
	UUtUns16 itr, amount;

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "chr_givepowerup", inErrorContext);
		return UUcError_None;
	} else {
		UUmAssert(inParameterListLength >= 2);

		for (itr = 0; itr < WPcPowerup_NumTypes; itr++) {
			if (UUmString_IsEqual(inParameterList[1].val.str, WPgPowerupName[itr])) {
				break;
			}
		}

		if (itr >= WPcPowerup_NumTypes) {
			COrConsole_Printf("### unknown powerup type! valid powerup types are:");
			for (itr = 0; itr < WPcPowerup_NumTypes; itr++) {
				COrConsole_Printf("  %s", WPgPowerupName[itr]);
			}
			return UUcError_None;
		}

		if (inParameterListLength < 3) {
			amount = WPrPowerup_DefaultAmount(itr);
		} 
		else {
			amount = (UUtUns16) inParameterList[2].val.i;
		}

		WPrPowerup_Give(character, itr, amount, UUcTrue, UUcFalse);
		return UUcError_None;
	}
}

// set the severity threshold for error reporting
// syntax: ai2_set_reporterror error
//         ai2_set_reporterror status pathfinding
static UUtError			AI2iScript_Set_ReportError(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	AI2tErrorSeverity severity;
	AI2tErrorSubsystem subsystem;

	UUmAssert(inParameterListLength >= 1);

	severity = AI2rError_GetSeverityFromString(inParameterList[0].val.str);

	if (inParameterListLength >= 2) {
		subsystem = AI2rError_GetSubsystemFromString(inParameterList[1].val.str);
	} else {
		subsystem = AI2cSubsystem_All;
	}

	AI2rError_SetReportLevel(subsystem, severity);

	return UUcError_None;
}

// set the severity threshold for error logging
// syntax: ai2_set_logerror status
//         ai2_set_logerror error movement
static UUtError			AI2iScript_Set_LogError(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	AI2tErrorSeverity severity;
	AI2tErrorSubsystem subsystem;

	UUmAssert(inParameterListLength >= 1);

	severity = AI2rError_GetSeverityFromString(inParameterList[0].val.str);

	if (inParameterListLength >= 2) {
		subsystem = AI2rError_GetSubsystemFromString(inParameterList[1].val.str);
	} else {
		subsystem = AI2cSubsystem_All;
	}

	AI2rError_SetLogLevel(subsystem, severity);

	return UUcError_None;
}

// set the severity threshold for error logging
// syntax: ai2_set_handlesilenterror 1
//         ai2_set_handlesilenterror 0 pathfinding
static UUtError			AI2iScript_Set_HandleSilentError(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	AI2tErrorSubsystem subsystem;
	UUtBool handle_silent;

	UUmAssert(inParameterListLength >= 1);

	handle_silent = inParameterList[0].val.b;

	if (inParameterListLength >= 2) {
		subsystem = AI2rError_GetSubsystemFromString(inParameterList[1].val.str);
	} else {
		subsystem = AI2cSubsystem_All;
	}

	AI2rError_SetSilentHandling(subsystem, handle_silent);

	return UUcError_None;
}

// traverse the BNV graph in the local region (starting from the player)
// syntax: ai2_debug_traversegraph
static UUtError			AI2iScript_Debug_TraverseGraph(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = ONrGameState_GetPlayerCharacter();
	UUtUns32 distance;

	if (inParameterListLength >= 1) {
		distance = inParameterList[0].val.i;
	} else {
		distance = 10;
	}

	if (character == NULL) {
		SLrScript_ReportError(inErrorContext, "no player character!");
		return UUcError_None;
	}

	if (character->currentPathnode == NULL) {
		SLrScript_ReportError(inErrorContext, "no path node for player's BNV!");
		return UUcError_None;
	}

#if TOOL_VERSION
	PHrDebugGraph_TraverseConnections(ONrGameState_GetGraph(), character->currentPathnode, NULL, distance);
	AI2gDebugDisplayGraph = UUcTrue;
#else
	COrConsole_Printf("can only display pathfinding graph in tool version");
#endif

	return UUcError_None;
}

static char AI2iScript_ReportLinebuf[2048];
static void AI2iScript_ReportCallback(char *inLine)
{
	if (inLine[0] == '\0') {
		AI2rError_Line(AI2iScript_ReportLinebuf);
		strcpy(AI2iScript_ReportLinebuf, "");
	} else {
		strcat(AI2iScript_ReportLinebuf, inLine);
	}
}

// tell a character (or all characters) to report in
// syntax: ai2_report
//         ai2_report mr_man
static UUtError	AI2iScript_Report(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseOptionalAI(inParameterListLength, inParameterList);

	AI2gError_ReportThisError = UUcTrue;
	AI2gError_LogThisError = UUcTrue;
	if (character == NULL) {
		// report in list form
		strcpy(AI2iScript_ReportLinebuf, "");
		AI2rReport(character, UUcFalse, &AI2iScript_ReportCallback);
	} else {
		AI2rReport(character, UUcFalse, &AI2rError_Line);
	}

	return UUcError_None;
}

// tell a character (or all characters) to report in detail
// syntax: ai2_report_verbose
//         ai2_report_verbose mr_man
static UUtError	AI2iScript_Report_Verbose(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseOptionalAI(inParameterListLength, inParameterList);

	AI2gError_ReportThisError = UUcTrue;
	AI2gError_LogThisError = UUcTrue;
	AI2rReport(character, UUcTrue, &AI2rError_Line);

	return UUcError_None;
}

// prints the BNV index that the player character is standing in
// syntax: ai2_debug_printbnvindex
static UUtError			AI2iScript_Debug_PrintBNVIndex(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = ONrGameState_GetPlayerCharacter();

	if (character->currentNode == NULL) {
		COrConsole_Printf("### player is not standing in a BNV!");
	} else {
		COrConsole_Printf("player is standing in bnv #%d", character->currentNode->index);
	}

	return UUcError_None;
}

// makes a sound and alerts the AIs
// syntax: ai2_debug_makesound
//         ai2_debug_makesound gunshot
static UUtError			AI2iScript_Debug_MakeSound(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = ONrGameState_GetPlayerCharacter();
	float volume;
	AI2tContactType contact_type;
	M3tPoint3D location;

	if (inParameterListLength >= 1) {
		if (strcmp(inParameterList[0].val.str, "ignore") == 0) {
			contact_type = AI2cContactType_Sound_Ignore;

		} else if (strncmp(inParameterList[0].val.str, "interest", 8) == 0) {
			contact_type = AI2cContactType_Sound_Interesting;

		} else if (strcmp(inParameterList[0].val.str, "danger") == 0) {
			contact_type = AI2cContactType_Sound_Danger;

		} else if (strcmp(inParameterList[0].val.str, "melee") == 0) {
			contact_type = AI2cContactType_Sound_Melee;

		} else if (strcmp(inParameterList[0].val.str, "gunshot") == 0) {
			contact_type = AI2cContactType_Sound_Gunshot;

		} else {
			SLrScript_ReportError(inErrorContext, "unknown sound type '%s'", inParameterList[0].val.str);
			return UUcError_None;
		}
	} else {
		contact_type = AI2cContactType_Sound_Interesting;
	}

	if (inParameterListLength >= 2) {
		volume = inParameterList[1].val.f;
	} else {
		volume = 1.0f;
	}

	location = character->location;
	location.y += character->heightThisFrame;

	AI2rKnowledge_Sound(contact_type, &location, volume, character, NULL);
	return UUcError_None;
}

// trips an alarm
// syntax: ai2_tripalarm alarm_id
//         ai2_tripalarm alarm_id char_name
static UUtError			AI2iScript_TripAlarm(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseOptionalAI(inParameterListLength - 1, inParameterList + 1);

	if (character == NULL) {
		character = ONrGameState_GetPlayerCharacter();
	}

	UUmAssert(inParameterListLength >= 1);
	AI2rKnowledge_AlarmTripped((UUtUns8) inParameterList[0].val.i, character);
	return UUcError_None;
}

// smite one or many AIs
// syntax: ai2_smite
//         ai2_smite ai_7
//         ai2_smite except ai_5			// kills all but ai_5
static UUtError			AI2iScript_Debug_Smite(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	UUtBool spare_selected;
	ONtCharacter *selected_character = NULL;

	if (inParameterListLength >= 1) {
		if (strcmp(inParameterList[0].val.str, "except") == 0) {
			spare_selected = UUcTrue;
			selected_character = AI2rScript_ParseOptionalAI(inParameterListLength - 1, inParameterList + 1);
		} else {
			spare_selected = UUcFalse;
			selected_character = AI2rScript_ParseOptionalAI(inParameterListLength, inParameterList);
			if (selected_character == NULL) {
				// the AI in question is already dead, do nothing
				return UUcError_None;
			}
		}
	}

	AI2rSmite(selected_character, spare_selected);
	return UUcError_None;
}

// make one or many AIs forget part of their knowledge base
// syntax: ai2_forget
//         ai2_forget ai_7				// ai_7 forgets everything
//         ai2_forget all ai_5			// everyone forgets about ai_5
//         ai2_forget ai_9 ai_5			// ai_9 forgets about ai_5
static UUtError			AI2iScript_Debug_Forget(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character, *forget_character;

	character = AI2rScript_ParseOptionalAI(inParameterListLength, inParameterList);
	forget_character = AI2rScript_ParseOptionalAI(inParameterListLength - 1, inParameterList + 1);

	AI2rForget(character, forget_character);

	return UUcError_None;
}

// set the alert state of an AI
// syntax: ai2_setalert ai_5 lull
static UUtError	AI2iScript_Debug_SetAlert(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseAI(inParameterListLength, inParameterList);
	AI2tAlertStatus alert;
	UUtUns32 itr;

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "ai2_setalert", inErrorContext);
		return UUcError_None;
	}
	
	UUmAssert(inParameterListLength >= 2);
	for (itr = 0; itr < AI2cAlertStatus_Max; itr++) {
		if (strcmp(inParameterList[1].val.str, AI2cAlertStatusName[itr]) == 0) {
			alert = (AI2tAlertStatus) itr;
			break;
		}
	}
	if (itr >= AI2cAlertStatus_Max) {
		SLrScript_ReportError(inErrorContext, "could not find alert status '%s' (try '%s', '%s', etc)",
			inParameterList[1].val.str, AI2cAlertStatusName[0], AI2cAlertStatusName[1]);
		return UUcError_None;
	}

	if (character->ai2State.alertStatus < alert) {
		AI2rAlert_UpgradeStatus(character, alert, NULL);

	} else {
		AI2rAlert_DowngradeStatus(character, alert, NULL, UUcTrue);
	}

	return UUcError_None;
}

// Game mechanics 
// triggers
static UUtError
AI2iScript_Trigger_Activate(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns16		trigger_id;

	trigger_id = (UUtUns16) inParameterList[0].val.i;

	OBJrTrigger_Activate_ID( trigger_id );

	return UUcError_None;
}

static UUtError
AI2iScript_Trigger_Deactivate(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns16		trigger_id;

	trigger_id = (UUtUns16) inParameterList[0].val.i;

	OBJrTrigger_Deactivate_ID( trigger_id );

	return UUcError_None;
}

static UUtError
AI2iScript_Trigger_Reset(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns16		trigger_id;

	trigger_id = (UUtUns16) inParameterList[0].val.i;

	OBJrTrigger_Reset_ID( trigger_id );

	return UUcError_None;
}

static UUtError			AI2iScript_Trigger_Hide(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	UUtUns16 trigger_id = (UUtUns16) inParameterList[0].val.i;
	
	OBJrTrigger_Hide_ID(trigger_id);

	return UUcError_None;
}

static UUtError			AI2iScript_Trigger_Show(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	UUtUns16 trigger_id = (UUtUns16) inParameterList[0].val.i;
	
	OBJrTrigger_Show_ID(trigger_id);

	return UUcError_None;
}

static UUtError			AI2iScript_Trigger_Speed(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	UUtUns16 trigger_id = (UUtUns16) inParameterList[0].val.i;
	float speed = inParameterList[1].val.f;
	
	OBJrTrigger_SetSpeed_ID(trigger_id, speed);

	return UUcError_None;
}

// turrets
static UUtError
AI2iScript_Console_Activate(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns16		id;

	id = (UUtUns16) inParameterList[0].val.i;

	OBJrConsole_Activate_ID( id );

	return UUcError_None;
}

static UUtError
AI2iScript_Console_Deactivate(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns16		id;

	if( inParameterListLength < 1 )
		id = (UUtUns16) 0xFFFF;
	else
		id = (UUtUns16) inParameterList[0].val.i;

	OBJrConsole_Deactivate_ID( id );

	return UUcError_None;
}

static UUtError
AI2iScript_Console_Reset(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns16		id;

	if (inParameterListLength < 1) {
		id = (UUtUns16) 0xFFFF;
	}
	else {
		id = (UUtUns16) inParameterList[0].val.i;
	}

	OBJrConsole_Reset_ID( id );

	return UUcError_None;
}

// turrets
static UUtError
AI2iScript_Turret_Activate(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns16		trigger_id;

	trigger_id = (UUtUns16) inParameterList[0].val.i;

	return OBJrTurret_Activate_ID( trigger_id );
}

static UUtError
AI2iScript_Turret_Deactivate(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns16		trigger_id;

	if( inParameterListLength < 1 )
		trigger_id = (UUtUns16) -1;
	else
		trigger_id = (UUtUns16) inParameterList[0].val.i;

	return OBJrTurret_Deactivate_ID( trigger_id );
}

static UUtError
AI2iScript_Turret_Reset(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns16		trigger_id;

	if( inParameterListLength < 1 )
		trigger_id = (UUtUns16) -1;
	else
		trigger_id = (UUtUns16) inParameterList[0].val.i;

	return OBJrTurret_Reset_ID( trigger_id );
}

// create a chump to follow the player
// syntax: ai2_debug_chump
static UUtError
AI2iScript_Debug_Chump(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	OBJtObject *		temp_object;
	OBJtOSD_Character *	temp_osd;
	ONtCharacter 		*player_char, *chump;
	M3tVector3D			facing_vector;
	UUtUns16			chump_index;
	UUtUns32			itr;
	UUtError			error;

	player_char = ONrGameState_GetPlayerCharacter();
	if (player_char == NULL) {
		return UUcError_None;
	}

	temp_object = UUrMemory_Block_NewClear(sizeof(OBJtObject) + sizeof(OBJtOSD_Character));
	temp_osd = (OBJtOSD_Character *) temp_object->object_data;

	// set up a dummy object just in front of the player
	temp_object->object_type = OBJcType_Character;
	ONrCharacter_GetPathfindingLocation(player_char, &temp_object->position);
	ONrCharacter_GetFacingVector(player_char, &facing_vector);
	MUmVector_ScaleIncrement(temp_object->position, 20.0f, facing_vector);
	MUmVector_Set(temp_object->rotation, 0, 0, 0);

	// set up this object's OSD
	UUrString_Copy(temp_osd->character_name, "chump", sizeof(temp_osd->character_name));
	temp_osd->flags = 0;
	UUrString_Copy(temp_osd->character_class, AI2cDefaultClassName, 64);
	temp_osd->weapon_class_name[0] = '\0';
	temp_osd->spawn_script[0] = '\0';
	temp_osd->die_script[0] = '\0';
	temp_osd->combat_script[0] = '\0';
	temp_osd->alarm_script[0] = '\0';
	temp_osd->hurt_script[0] = '\0';
	temp_osd->defeated_script[0] = '\0';
	temp_osd->hit_points = 0;				// offset from class default
	for (itr = 0; itr < OBJcCharInv_Max; itr++) {
		UUtUns32 itr2;

		for (itr2 = 0; itr2 < OBJcCharSlot_Max; itr2++) {
			temp_osd->inventory[itr][itr2] = 0;
		}
	}
	temp_osd->team_number = 0;
	temp_osd->ammo_percentage = 100;
	temp_osd->initial_alert = AI2cAlertStatus_Lull;
	temp_osd->minimum_alert = AI2cAlertStatus_Lull;
	temp_osd->jobstart_alert = AI2cAlertStatus_Low;
	temp_osd->investigate_alert = AI2cAlertStatus_Medium;
	temp_osd->combat_id = 0;
	temp_osd->melee_id = 0;
	temp_osd->neutral_id = 0;
	
	// create the chump
	error = ONrGameState_NewCharacter(temp_object, NULL, NULL, &chump_index);

	UUrMemory_Block_Delete(temp_object);

	UUmError_ReturnOnError(error);

	chump = ONrGameState_GetCharacterList() + chump_index;
	if (strcmp(chump->player_name, "chump") != 0) {
		UUmAssert(0);
	}

	chump->ai2State.flags |= AI2cFlag_Passive;
	chump->ai2State.currentGoal = AI2cGoal_Idle;
	chump->ai2State.currentStateBuffer.begun = UUcFalse;
	chump->ai2State.currentState = &(chump->ai2State.currentStateBuffer);
	AI2rMovement_ChangeMovementMode(chump, AI2cMovementMode_NoAim_Run);
	AI2rPath_FollowCharacter(chump, player_char);

	AI2gShowPathfindingGrids = UUcTrue;
	AI2gDrawPaths = UUcTrue;

	return UUcError_None;
}

static UUtError	AI2iScript_PathDebugSquare(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *player_character = ONrGameState_GetPlayerCharacter();
	PHtNode *node;
	PHtRoomData *room;
	UUtUns32 itr;
	PHtDebugInfo *debug_info;
	char event_name[32], weight_name[32], quad_name[32];

	// find the player's current BNV
	if (player_character == NULL) {
		COrConsole_Printf("ai2_pathdebugsquare: can't find player character");
		return UUcError_None;
	}
	if (player_character->currentNode == NULL) {
		COrConsole_Printf("ai2_pathdebugsquare: player is not in a BNV");
		return UUcError_None;
	}
	room = &player_character->currentNode->roomData;
	node = player_character->currentPathnode;
	if (node == NULL) {
		COrConsole_Printf("ai2_pathdebugsquare: player's BNV has no pathfinding node");
		return UUcError_None;
	}

	if ((inParameterList[0].val.i < 0) || (inParameterList[0].val.i > (UUtInt32) room->gridX - 1) ||
		(inParameterList[1].val.i < 0) || (inParameterList[1].val.i > (UUtInt32) room->gridY - 1)) {
		PHgDebugSquareX = (UUtUns16) -1;
		PHgDebugSquareY = (UUtUns16) -1;
		COrConsole_Printf("ai2_pathdebugsquare: invalid square (%d, %d) - BNV #%d is size %dx%d",
						inParameterList[0].val.i, inParameterList[1].val.i, player_character->currentNode->index, room->gridX, room->gridY);
		return UUcError_None;
	}
	// set the square to debug
	PHgDebugSquareX = (UUtUns16) inParameterList[0].val.i;
	PHgDebugSquareY = (UUtUns16) inParameterList[1].val.i;

	if (room->debug_info == NULL) {
		COrConsole_Printf("ai2_pathdebugsquare: this BNV has no path debugging info!");
		COrConsole_Printf("  you might have to import with the -pathdebug switch");
		return UUcError_None;
	}

	COrConsole_Printf("Printing path debug info for BNV #%d, square (%d, %d)", player_character->currentNode->index, PHgDebugSquareX, PHgDebugSquareY);
	for (itr = 0, debug_info = room->debug_info; itr < room->debug_info_count; itr++, debug_info++) {
		if ((debug_info->x != PHgDebugSquareX) || (debug_info->y != PHgDebugSquareY))
			continue;
		
		if ((debug_info->event >= 0) && (debug_info->event < PHcDebugEvent_Max)) {
			UUrString_Copy(event_name, PHcDebugEventName[debug_info->event], 32);
		} else {
			sprintf(event_name, "unknown-event %d", debug_info->event);
		}
		if ((debug_info->weight >= 0) && (debug_info->weight < PHcMax)) {
			UUrString_Copy(weight_name, PHcWeightName[debug_info->weight], 32);
		} else {
			sprintf(weight_name, "unknown-weight %d", debug_info->weight);
		}
		if (debug_info->gq_index == (UUtUns32) -1) {
			UUrString_Copy(quad_name, "none", 32);
		} else {
			sprintf(quad_name, "#%d", debug_info->gq_index);
		}

		COrConsole_Printf("  event %s, weight %s, quad %s", event_name, weight_name, quad_name);
	}

	return UUcError_None;
}

static UUtError	AI2iScript_Door_Jam(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	OBJrDoor_Jam((UUtUns16) inParameterList[0].val.i, UUcTrue);
	return UUcError_None;
}

static UUtError	AI2iScript_Door_Unjam(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	OBJrDoor_Jam((UUtUns16) inParameterList[0].val.i, UUcFalse);
	return UUcError_None;
}

static UUtError	AI2iScript_Door_Lock(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	return OBJrDoor_Lock_ID((UUtUns16) inParameterList[0].val.i);
}

static UUtError	AI2iScript_Door_Unlock(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	return OBJrDoor_Unlock_ID((UUtUns16) inParameterList[0].val.i);
}

static UUtError	AI2iScript_Door_Open(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	OBJrDoor_ForceOpen((UUtUns16) inParameterList[0].val.i);
	return UUcError_None;
}

static UUtError	AI2iScript_Door_Close(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	OBJrDoor_ForceClose((UUtUns16) inParameterList[0].val.i);
	return UUcError_None;
}

#if TOOL_VERSION
static UUtError	AI2iScript_Door_DebugDump(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	UUtError error;

	error = OBJrDoor_DumpAll();
	if (error != UUcError_None) {
		COrConsole_Printf("### could not dump door info");
	}

	return UUcError_None;
}

static UUtError	AI2iScript_Door_DebugDumpNearby(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	UUtError error;
	ONtCharacter *player_character = ONrGameState_GetPlayerCharacter();

	if (player_character == NULL) {
		COrConsole_Printf("### could not get player character");
		return UUcError_None;
	}

	error = OBJrDoor_DumpNearby(&player_character->actual_position, 50.0f);
	if (error != UUcError_None) {
		COrConsole_Printf("### could not dump door info");
	}

	return UUcError_None;
}
#endif



// ------------------------------------------------------------------------------------
// -- cinematic functions (from Oni_AI_Script.c)

// snap a character's facing to that of a flag
// syntax: chr_facetoflag mr_man 1004
static UUtError
AI2iScript_FaceToFlag(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(&inParameterList[0], UUcFalse);
	UUtInt32 flag_id = inParameterList[1].val.i;
	ONtFlag	flag;
	UUtBool found;
	
	found = ONrLevel_Flag_ID_To_Flag((UUtUns16) flag_id, &flag);
	
	if (NULL == character) {
		AI2rScript_NoCharacterError(&inParameterList[0], "chr_facetoflag", inErrorContext);
		return UUcError_None;

	} else if (UUcFalse == found) {
		SLrScript_ReportError(inErrorContext, "flag %d does not exist", flag_id);
		return UUcError_None;

	} else {
		character->facing = flag.rotation;
		character->facingModifier = 0;
		character->desiredFacing = flag.rotation;
	}
		
	return UUcError_None;
}

// teleport a character to a flag
// syntax: chr_teleport mr_man 1004
static UUtError
AI2iScript_Teleport(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(&inParameterList[0], UUcFalse);
	UUtInt32 flag_id = inParameterList[1].val.i;
	ONtFlag	flag;
	UUtBool found;
	
	found = ONrLevel_Flag_ID_To_Flag((UUtUns16) flag_id, &flag);
	
	if (NULL == character) {
		AI2rScript_NoCharacterError(&inParameterList[0], "chr_teleport", inErrorContext);
		return UUcError_None;

	} else if (UUcFalse == found) {
		SLrScript_ReportError(inErrorContext, "flag %d does not exist", flag_id);
		return UUcError_None;

	} else {
		ONrCharacter_Teleport(character, &flag.location, UUcTrue);
	}
		
	return UUcError_None;
}

// disable collision for a character
// syntax: chr_nocollision mr_man 1
static UUtError AI2iScript_NoCollision(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(&inParameterList[0], UUcFalse);

	if (NULL == character) {
		AI2rScript_NoCharacterError(&inParameterList[0], "chr_nocollision", inErrorContext);
		return UUcError_None;
	}
	
	if (inParameterList[1].val.i) {
		character->flags |= ONcCharacterFlag_NoCollision;
	} else {
		character->flags &= ~ONcCharacterFlag_NoCollision;
	}

	ONrCharacter_BuildPhysicsFlags(character, ONrGetActiveCharacter(character));

	return UUcError_None;
}

static ONtFilm *AI2iScript_Playback_Internal(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
											SLtParameter_Actual *inParameterList)
{
	ONtCharacter*	character = AI2rScript_ParseCharacter(inParameterList, UUcFalse);
	ONtActiveCharacter*	active_character;
	UUtError		error;
	ONtFilm*		film;
	char*			film_name;
	UUtUns32		film_mode, interp_frames;
	UUtBool			played_film;

	if (character == NULL) { 
		AI2rScript_NoCharacterError(inParameterList, "playback", inErrorContext);
		return NULL;
	}

	active_character = ONrForceActiveCharacter(character);
	if (active_character == NULL) {
		SLrScript_ReportError(inErrorContext, "character (%s) cannot be made active!", character->player_name);
		return NULL;
	}
	
	AI2iScript_CancelEnvAnim(character, active_character);

	film_name = inParameterList[1].val.str;
	error = TMrInstance_GetDataPtr(ONcTemplate_Film,film_name,&film);

	if (error != UUcError_None) {
		SLrScript_ReportError(inErrorContext, "can not find any film named \"%s\"", film_name);
		if ((character->charType == ONcChar_AI2) && (character->ai2State.currentGoal == AI2cGoal_Patrol)) {
			// the character is doing a patrol and cannot play its film, force it to fall off the patrol
			AI2rPatrol_Finish(character, &character->ai2State.currentState->state.patrol);
		}
		return NULL;
	}

	film_mode = ONcFilmMode_Normal;
	interp_frames = 0;
	if (inParameterListLength >= 3) {
		UUmAssert(inParameterList[2].type == SLcType_String);

		if (UUmString_IsEqual(inParameterList[2].val.str, "fromhere")) {
			film_mode = ONcFilmMode_FromHere;

		} else if (UUmString_IsEqual(inParameterList[2].val.str, "interp")) {
			film_mode = ONcFilmMode_Interp;
			interp_frames = ONcFilm_DefaultInterpFrames;

			if (inParameterListLength >= 4) {
				UUmAssert(inParameterList[3].type == SLcType_Int32);
				interp_frames = (UUtUns32) inParameterList[3].val.i;
			}
		}
	}

	played_film = ONrFilm_Start(character, film, film_mode, interp_frames);
	if (!played_film) {
		if ((character->charType == ONcChar_AI2) && (character->ai2State.currentGoal == AI2cGoal_Patrol)) {
			// the character is doing a patrol and cannot play its film, force it to fall off the patrol
			AI2rPatrol_Finish(character, &character->ai2State.currentState->state.patrol);
		}
		return NULL;
	} else {
		return film;
	}
}

// play back a film on a character
// syntax: playback mr_man jumping_film
//         playback mr_man jumping_film fromhere
//         playback mr_man jumping_film interp
//         playback mr_man jumping_film interp 60
static UUtError AI2iScript_Playback(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtFilm*		film;

	film = AI2iScript_Playback_Internal(inErrorContext, inParameterListLength, inParameterList);
	if (film == NULL) { 
		return UUcError_None;
	} else {
		return UUcError_None;
	}
}

// play back a film on a character and block
// syntax: playback_block mr_man jumping_film
//         playback_block mr_man jumping_film fromhere
//         playback_block mr_man jumping_film interp
//         playback_block mr_man jumping_film interp 60
static UUtError AI2iScript_Playback_Block(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtFilm*		film;

	film = AI2iScript_Playback_Internal(inErrorContext, inParameterListLength, inParameterList);
	if (film == NULL) { 
		return UUcError_None;
	} else {
		*outTicksTillCompletion = film->length;
		return UUcError_None;
	}
}

// debug a recorded film
// syntax: playback_debug jumping_film
static UUtError AI2iScript_Playback_Debug(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	char*			film_name = inParameterList[0].val.str;
	UUtError		error;
	ONtFilm*		film;

	error = TMrInstance_GetDataPtr(ONcTemplate_Film, film_name, &film);

	if (error != UUcError_None) {
		SLrScript_ReportError(inErrorContext, "failed to find film %s", film_name);
		return UUcError_None;
	} else {
		ONrFilm_DisplayStats(film);
		return UUcError_None;
	}
}

// internal - play an animation on a character
static UUtError AI2iScript_Animate_Internal(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue, UUtBool inBlock)
{
	ONtCharacter* character = AI2rScript_ParseCharacter(inParameterList, UUcFalse);
	const TRtAnimation*	animation = NULL;
	UUtInt32			numFramesOfAnimation = 0;
	UUtInt32			numFramesOfInterpolation = -1;
	ONtActiveCharacter *active_character;

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "chr_animate or chr_animate_block", inErrorContext);
		return UUcError_None;
	}

	active_character = ONrForceActiveCharacter(character);
	if (active_character == NULL) {
		COrConsole_Printf("### can't make character %s active", character->player_name);
		return UUcError_None;
	}

	if ((active_character->animationLockFrames > 1) && (inBlock))
	{
		// stall until this character is unlocked
		*outStall = UUcTrue;		
		return UUcError_None;
	}
	
	{
		char *animString;		
		
		animString = inParameterList[1].val.str;
		animation = TRrAnimation_GetFromName(animString);

		if (NULL == animation)
		{ 
			SLrScript_ReportError(inErrorContext, "failed to find animation \"%s\"", animString);
			return UUcError_None;
		}
	}

	if (inParameterListLength >= 3)
	{
		numFramesOfAnimation = inParameterList[2].val.i;
	}

	if (inParameterListLength >= 4)
	{
		numFramesOfInterpolation = (UUtUns16)inParameterList[3].val.i;
	}

//	COrConsole_Printf("chr_animate %s %s %d %d", character->player_name, TRrAnimation_GetName(animation), numFramesOfAnimation, numFramesOfInterpolation);
	
	ONrCharacter_SetAnimation_External(character, active_character->curFromState, animation, numFramesOfInterpolation);
	character->flags |= ONcCharacterFlag_ChrAnimate;

	if (-1 == numFramesOfAnimation) {
		active_character->animationLockFrames = TRrAnimation_GetDuration(animation) + 1;
	}
	else if (numFramesOfAnimation > 0) {
		active_character->animationLockFrames = numFramesOfAnimation + 1;
	}
	else {
		active_character->animationLockFrames = 0;
	}
	
	return UUcError_None;
}

// play back an animation on a character, and block
// syntax: chr_animateblock mr_man STRCOMgetup_kick
//         chr_animateblock mr_man STRCOMfallen_front 3000
//         chr_animateblock mr_man STRIKEtaunt        60     8
static UUtError AI2iScript_Animate_Block(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	return AI2iScript_Animate_Internal(inErrorContext, inParameterListLength, inParameterList, 
										outTicksTillCompletion, outStall, ioReturnValue, UUcTrue);
}

// play back an animation on a character, without blocking
// syntax: chr_animate mr_man STRCOMgetup_kick
//         chr_animate mr_man STRCOMfallen_front 3000
//         chr_animate mr_man STRIKEtaunt        60     8
static UUtError AI2iScript_Animate_NoBlock(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	return AI2iScript_Animate_Internal(inErrorContext, inParameterListLength, inParameterList, 
										outTicksTillCompletion, outStall, ioReturnValue, UUcFalse);
}

static void AI2iScript_CancelEnvAnim(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	// if a character is undergoing a chr_env_animate, stop it
	if ((ioActiveCharacter->physics != NULL) && (ioActiveCharacter->physics->animContext.animation != NULL)) {
		ioActiveCharacter->physics->animContext.animation = NULL;
		ioActiveCharacter->physics->animContext.animContextFlags = 0;
		ioActiveCharacter->physics->animContext.animationFrame = 0;
		ioActiveCharacter->physics->animContext.animationStep = 0;
		ioActiveCharacter->physics->animContext.frameStartTime = 0;

		ONrCharacter_BuildPhysicsFlags(ioCharacter, ioActiveCharacter);
	}
}

// internal - play an environmental animation on a character
static UUtError AI2iScript_EnvAnimate_Internal(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue, UUtBool inBlock)
{
	ONtCharacter* character = AI2rScript_ParseCharacter(inParameterList, UUcFalse);
	ONtActiveCharacter *active_character;
	PHtPhysicsContext *physics;
	UUtError error;
	OBtAnimation *anim;
	UUtBool noRotation = UUcFalse;

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "chr_envanim or chr_envanim_block", inErrorContext);
		return UUcError_None;
	}

	active_character = ONrForceActiveCharacter(character);
	if (active_character == NULL) {
		SLrScript_ReportError(inErrorContext, "character (%s) cannot be made active!", character->player_name);
		return UUcError_None;
	}

	physics = active_character->physics;
	if (physics == NULL) {
		SLrScript_ReportError(inErrorContext, "character (%s) has no physics context!", character->player_name);
		return UUcError_None;
	}

	if (physics->animContext.animation != NULL) {
		if (inBlock && ((physics->animContext.animationFrame+1) < physics->animContext.animation->numFrames)) {
			*outStall = UUcTrue;
			return UUcError_None;
		}
	}

#if TOOL_VERSION
	COrConsole_Printf("chr_envanim %s", inParameterList[1].val.str);
#endif

	// Load the named animation
	error = TMrInstance_GetDataPtr(
		OBcTemplate_Animation,
		inParameterList[1].val.str,
		&anim);

	if (error != UUcError_None)
	{
		SLrScript_ReportError(inErrorContext, "failed to find animation \"%s\"", inParameterList[1].val.str);
		return UUcError_None;
	}

	if (inParameterListLength > 2)
	{
		const char *flag_string;
		
		flag_string = inParameterList[2].val.str;

		if (UUmString_IsEqual(flag_string, "norotation"))
		{
			noRotation = UUcTrue;
		}
	}

	physics->animContext.animation = anim;
	physics->animContext.animContextFlags = 0;
	physics->animContext.animationFrame = 1;
	physics->animContext.animationStep = 1;
	physics->animContext.frameStartTime = 0;

	if (noRotation)
	{
		physics->animContext.animContextFlags |= OBcAnimContextFlags_NoRotation;
	}

	OBrAnim_Start(&physics->animContext);

	physics->animContext.animationFrame += 1;

	ONrCharacter_BuildPhysicsFlags(character, active_character);

	return UUcError_None;
}

// play back an animation on a character without blocking
// syntax: chr_envanim mr_man Outro11_batline
static UUtError AI2iScript_EnvAnimate_NoBlock(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	return AI2iScript_EnvAnimate_Internal(inErrorContext, inParameterListLength, inParameterList, outTicksTillCompletion, outStall, ioReturnValue, UUcFalse);
}

// play back an animation on a character, and block
// syntax: chr_envanim_block mr_man Outro11_batline
static UUtError AI2iScript_EnvAnimate_Block(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	return AI2iScript_EnvAnimate_Internal(inErrorContext, inParameterListLength, inParameterList, outTicksTillCompletion, outStall, ioReturnValue, UUcTrue);
}

// cancel any envanim currently running on a character
// syntax: chr_envanim_stop mr_man
static UUtError AI2iScript_EnvAnimate_Stop(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(inParameterList, UUcTrue);
	ONtActiveCharacter *active_character;

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "chr_envanim or chr_envanim_block", inErrorContext);
		return UUcError_None;
	}

	active_character = ONrGetActiveCharacter(character);
	if (active_character != NULL) {
		AI2iScript_CancelEnvAnim(character, active_character);
	}

	return UUcError_None;
}

// wait for a character to play a given animation
// syntax: chr_wait_animation konoko KONOKOidle_spec2 KONOKOidle_spec3
static UUtError AI2iScript_WaitForAnimation(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtActiveCharacter *active_character;
	ONtCharacter *character;
	UUtUns32 itr;

	if (inParameterListLength < 2) {
		// can't read character
		return UUcError_None;
	}

	character = AI2rScript_ParseCharacter(&inParameterList[0], UUcTrue);
	if (character == NULL) {
		AI2rScript_NoCharacterError(&inParameterList[0], "chr_wait_animation", inErrorContext);
		return UUcError_None;
	}
	active_character = ONrGetActiveCharacter(character);
	if (active_character == NULL) {
		// character is not active, we can't delay on their animations
		return UUcError_None;
	}

	// work out if we are playing any of these animations
	for (itr = 1; itr < inParameterListLength; itr++) {
		if (inParameterList[itr].type != SLcType_String)
			continue;

		if (ONrActiveCharacter_IsPlayingThisAnimationName(active_character, inParameterList[itr].val.str)) {
			*outStall = UUcFalse;
			return UUcError_None;
		}
	}

	// we are not playing any of the given animations, wait until we do
	*outStall = UUcTrue;
	return UUcError_None;
}

// wait for a character to play a given animation type
// syntax: chr_wait_animtype konoko Land_Left Land_Right Land_Forward Land_Back  (max of 16 animtypes)
static UUtError AI2iScript_WaitForAnimType(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtActiveCharacter *active_character;
	ONtCharacter *character;
	TRtAnimType this_type, cur_animtypes[ONcOverlayIndex_Count + 1], desired_animtypes[16];
	UUtUns32 itr, itr2, num_cur_animtypes, num_desired_animtypes;

	if (inParameterListLength < 2) {
		// can't read character
		return UUcError_None;
	}

	character = AI2rScript_ParseCharacter(&inParameterList[0], UUcTrue);
	if (character == NULL) {
		AI2rScript_NoCharacterError(&inParameterList[0], "chr_wait_animtype", inErrorContext);
		return UUcError_None;
	}
	active_character = ONrGetActiveCharacter(character);
	if (active_character == NULL) {
		// character is not active, we can't delay on their animtypes
		return UUcError_None;
	}

	// work out which animtypes we are looking for
	num_desired_animtypes = 0;
	for (itr = 1; itr < inParameterListLength; itr++) {
		if (inParameterList[itr].type != SLcType_String)
			continue;

		this_type = ONrStringToAnimType(inParameterList[itr].val.str);
		if (this_type == ONcAnimType_None) {
			COrConsole_Printf("chr_wait_animtype: unknown anim type '%s'", inParameterList[itr].val.str);
		} else {
			desired_animtypes[num_desired_animtypes++] = this_type;
		}

		if (num_desired_animtypes >= 16) {
			// the array is full, cannot add any more desired animtypes
			COrConsole_Printf("chr_wait_animtype: too many anim types (max 16)");
			break;
		}
	}

	if (num_desired_animtypes == 0) {
		// no valid animtypes
		return UUcError_None;
	}

	num_cur_animtypes = 0;
	cur_animtypes[num_cur_animtypes++] = active_character->curAnimType;

	for (itr = 0; itr < ONcOverlayIndex_Count; itr++) {
		if (ONrOverlay_IsActive(&active_character->overlay[itr])) {
			cur_animtypes[num_cur_animtypes++] = TRrAnimation_GetType(active_character->overlay[itr].animation);
		}
	}

	for (itr = 0; itr < num_desired_animtypes; itr++) {
		for (itr2 = 0; itr2 < num_cur_animtypes; itr2++) {
			if (desired_animtypes[itr] == cur_animtypes[itr2]) {
				// success! stop stalling
				*outStall = UUcFalse;
				return UUcError_None;
			}
		}
	}

	// the character is not yet playing one of the desired animtypes, stall until they are
	*outStall = UUcTrue;
	return UUcError_None;
}

// wait for a character to play a given animation type
// syntax: chr_wait_animstate konoko Run_Slide  (max of 16 animstates)
static UUtError AI2iScript_WaitForAnimState(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtActiveCharacter *active_character;
	ONtCharacter *character;
	TRtAnimState this_state, desired_animstates[16];
	UUtUns32 itr, num_desired_animstates;

	if (inParameterListLength < 2) {
		// can't read character
		return UUcError_None;
	}

	character = AI2rScript_ParseCharacter(&inParameterList[0], UUcTrue);
	if (character == NULL) {
		AI2rScript_NoCharacterError(&inParameterList[0], "chr_wait_animstate", inErrorContext);
		return UUcError_None;
	}
	active_character = ONrGetActiveCharacter(character);
	if (active_character == NULL) {
		// character is not active, we can't delay on their animstate
		return UUcError_None;
	}

	// work out which animstates we are looking for
	num_desired_animstates = 0;
	for (itr = 1; itr < inParameterListLength; itr++) {
		if (inParameterList[itr].type != SLcType_String)
			continue;

		this_state = ONrStringToAnimState(inParameterList[itr].val.str);
		if (this_state == ONcAnimState_None) {
			COrConsole_Printf("chr_wait_animstate: unknown anim state '%s'", inParameterList[itr].val.str);
		} else {
			desired_animstates[num_desired_animstates++] = this_state;
		}

		if (num_desired_animstates >= 16) {
			// the array is full, cannot add any more desired animstates
			COrConsole_Printf("chr_wait_animstate: too many anim states (max 16)");
			break;
		}
	}

	if (num_desired_animstates == 0) {
		// no valid animstates
		return UUcError_None;
	}

	for (itr = 0; itr < num_desired_animstates; itr++) {
		if (active_character->nextAnimState == desired_animstates[itr]) {
			// success! stop stalling
			*outStall = UUcFalse;
			return UUcError_None;
		}
	}

	// the character has not yet reached one of the desired animstates, stall until they do
	*outStall = UUcTrue;
	return UUcError_None;
}

// create a character from an AItSetup
// syntax: chr_create 105 start
//         chr_create 105 create 2001 2002 2003 2004
static UUtError AI2iScript_ChrCreate(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	UUtError					error;
	UUtInt16 					setupID;
	const AItCharacterSetup*	setup;
	M3tPoint3D					offset_location;
	ONtFlag						flag;
	UUtBool						found;

	// parameter 1 is character setup id
	setupID = (UUtUns16)inParameterList[0].val.i;
	setup = ONrLevel_CharacterSetup_ID_To_Pointer(setupID);
	if (NULL == setup)
	{ 
		SLrScript_ReportError(inErrorContext, "can not find character id %d", inParameterList[0].val.i);

		{
			AItCharacterSetupArray *characterSetupArray = ONgLevel->characterSetupArray;
			UUtUns16 itr;

			for(itr = 0; itr < characterSetupArray->numCharacterSetups; itr++) {
				const AItCharacterSetup *curSetup = characterSetupArray->characterSetups + itr;

				COrConsole_Printf("[%d]", curSetup->defaultScriptID);
			}
		}

		return UUcError_None;
	}
		
	// CB: optional parameter 2 was start/create. 'create' was only ever used when spawning characters
	// as part of gameplay (so the player doesn't see them). this command is only ever used for cinematics now
	// so it's irrelevant.

	// find the flag from our AI setup structure
	found = ONrLevel_Flag_ID_To_Flag(setup->defaultFlagID, &flag);
	if (!found) {
		SLrScript_ReportError(inErrorContext, "default flag %d does not exist", setup->defaultFlagID);
		return UUcError_None;
	}

	offset_location = flag.location;
	offset_location.y += ONcCharacterOffsetToBNV;

	if (NULL == AKrNodeFromPoint(&offset_location)) {
		SLrScript_ReportError(
			inErrorContext,
			"flag was not inside a BNV %f %f %f",
			flag.location.x,
			flag.location.y,
			flag.location.z);

		return UUcError_None;
	}
	
	error = ONrGameState_NewCharacter(NULL, setup, &flag, NULL);
	if (error != UUcError_None) {
		SLrScript_ReportError(inErrorContext, "failed to create a new character");
		return UUcError_None;
	}
	
	return UUcError_None;
}

// delete a character
// syntax: chr_delete mr_man
static UUtError AI2iScript_ChrDelete(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(inParameterList, UUcFalse);

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "chr_delete", inErrorContext);
		return UUcError_None;
	} else {
		ONrGameState_DeleteCharacter(character);
		return UUcError_None;
	}
}

// disable the aiming screen for a character
// syntax: chr_dontaim mr_man 1
static UUtError AI2iScript_DontAim(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(inParameterList, UUcFalse);

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "chr_dontaim", inErrorContext);
		return UUcError_None;
	}

	ONrCharacter_DisableWeaponVarient(character, (UUtBool) inParameterList[1].val.i);
	return UUcError_None;
}

// set a character's health to full
// syntax: chr_full_health mr_man
static UUtError AI2iScript_SetFullHealth(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(inParameterList, UUcTrue);

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "chr_full_health", inErrorContext);
		return UUcError_None;
	}

	COrConsole_Printf("setting %s to full health (%d)", character->player_name, character->maxHitPoints);

	ONrCharacter_SetHitPoints(character, character->maxHitPoints);

	return UUcError_None;
}

// set a character's health
// syntax: chr_set_health mr_man 100
static UUtError AI2iScript_SetHealth(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(inParameterList, UUcTrue);

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "chr_set_health", inErrorContext);
		return UUcError_None;
	}

	ONrCharacter_SetHitPoints(character, (UUtUns32) inParameterList[1].val.i);
	return UUcError_None;
}

// wait for a character's health to fall below a certain level
// syntax: chr_wait_health konoko 40
static UUtError AI2iScript_WaitForHealth(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character;

	character = AI2rScript_ParseCharacter(&inParameterList[0], UUcTrue);
	if (character == NULL) {
		AI2rScript_NoCharacterError(&inParameterList[0], "chr_wait_health", inErrorContext);
		return UUcError_None;
	}

	if ((UUtInt32) character->hitPoints > inParameterList[1].val.i) {
		// stall until their hit points drop below the desired level
		*outStall = UUcTrue;
	}

	return UUcError_None;
}

// get a keymask from a script key name
static LItButtonBits AI2iScript_GetKeyMask(const char *inKeyName)
{
	typedef struct AI2tScript_KeyMaskName
	{
		char *name;
		LItButtonBits mask;

	} AI2tScript_KeyMaskName;

	AI2tScript_KeyMaskName *tableptr, keytable[] = {
		{"forward",		LIc_BitMask_Forward},
		{"back",		LIc_BitMask_Backward},
		{"stepleft",	LIc_BitMask_StepLeft},
		{"stepright",	LIc_BitMask_StepRight},
		{"turnleft",	LIc_BitMask_TurnLeft},
		{"turnright",	LIc_BitMask_TurnRight},
		{"crouch",		LIc_BitMask_Crouch},
		{"jump",		LIc_BitMask_Jump},
		{"fire",		LIc_BitMask_Fire1},
		{"altfire",		LIc_BitMask_Fire2},
		{"punch",		LIc_BitMask_Punch},
		{"kick",		LIc_BitMask_Kick},
		{"action",		LIc_BitMask_Action},
		{"",			0}};

	for (tableptr = keytable; tableptr->mask != 0; tableptr++) {
		if (strcmp(inKeyName, tableptr->name) == 0) {
			break;
		}
	}
	
	return tableptr->mask;
}

// force a character to hold keys down for a number of frames
// syntax: chr_holdkey mr_man action 30
static UUtError AI2iScript_HoldKey(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(inParameterList, UUcFalse);
	ONtActiveCharacter *active_character;
	const char *keyname;
	
	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "chr_holdkey", inErrorContext);
		return UUcError_None;
	}

	active_character = ONrForceActiveCharacter(character);
	if (active_character == NULL) {
		COrConsole_Printf("### can't make character active");
		return UUcError_None;
	}

	if (active_character->keyHoldFrames > 0) {
		// stall until this character is unlocked
		*outStall = UUcTrue;
		return UUcError_None;
	}
	
	// part 2 is what keys (or clear)
	keyname = inParameterList[1].val.str;
	if (UUmString_IsEqual("clear", keyname))
	{
		*outTicksTillCompletion = 1;
		active_character->keyHoldMask = 0;
		character->flags &= ~ONcCharacterFlag_ScriptControl;
	}
	else
	{
		active_character->keyHoldMask |= AI2iScript_GetKeyMask(keyname);
		character->flags |= ONcCharacterFlag_ScriptControl;
		active_character->keyHoldFrames = (UUtUns16)inParameterList[2].val.i;
	}

	return UUcError_None;
}

enum {
	AI2cTriggerVolumeCallbackType_Enable			= 0,
	AI2cTriggerVolumeCallbackType_SetEnterScript	= 1,
	AI2cTriggerVolumeCallbackType_SetInsideScript	= 2,
	AI2cTriggerVolumeCallbackType_SetExitScript		= 3,
	AI2cTriggerVolumeCallbackType_EnableEnterScript	= 4,
	AI2cTriggerVolumeCallbackType_EnableInsideScript= 5,
	AI2cTriggerVolumeCallbackType_EnableExitScript	= 6,
	AI2cTriggerVolumeCallbackType_TriggerEntry		= 7,
	AI2cTriggerVolumeCallbackType_TriggerInside		= 8,
	AI2cTriggerVolumeCallbackType_TriggerExit		= 9,
	AI2cTriggerVolumeCallbackType_Reset				= 10,

	AI2cTriggerVolumeCallbackType_Max				= 11
};

typedef struct AI2tTriggerVolumeCallbackData {
	char		*name;
	UUtUns32	type;
	UUtBool		enable;
	UUtUns32	found;
	char		*script;
	ONtCharacter *character;
} AI2tTriggerVolumeCallbackData;

static UUtBool AI2iScript_TriggerVolume_Callback(OBJtObject *inObject, UUtUns32 inUserData)
{
	OBJtOSD_TriggerVolume *trig_osd;
	AI2tTriggerVolumeCallbackData *data = (AI2tTriggerVolumeCallbackData *) inUserData;

	UUmAssertReadPtr(data, sizeof(*data));
	UUmAssert(inObject->object_type == OBJcType_TriggerVolume);

#if TOOL_VERSION
	AI2gScript_TriggerVolume_Callback_Count++;
#endif

	trig_osd = (OBJtOSD_TriggerVolume *) inObject->object_data;
	
	if (UUmString_IsEqual(data->name, trig_osd->name)) {
		data->found++;

		switch(data->type) {
			case AI2cTriggerVolumeCallbackType_Enable:
				if (data->enable) {
					trig_osd->in_game_flags &= ~OBJcOSD_TriggerVolume_Disabled;
				} else {
					ONrCharacter_NotifyTriggerVolumeDisable(trig_osd->id);
					trig_osd->in_game_flags |= OBJcOSD_TriggerVolume_Disabled;
				}
			break;

			case AI2cTriggerVolumeCallbackType_SetEnterScript:
				UUmAssertReadPtr(data->script, sizeof(char));
				UUrString_Copy(trig_osd->cur_entry_script, data->script, SLcScript_MaxNameLength);
			break;

			case AI2cTriggerVolumeCallbackType_SetInsideScript:
				UUmAssertReadPtr(data->script, sizeof(char));
				UUrString_Copy(trig_osd->cur_inside_script, data->script, SLcScript_MaxNameLength);
			break;

			case AI2cTriggerVolumeCallbackType_SetExitScript:
				UUmAssertReadPtr(data->script, sizeof(char));
				UUrString_Copy(trig_osd->cur_exit_script, data->script, SLcScript_MaxNameLength);
			break;

			case AI2cTriggerVolumeCallbackType_EnableEnterScript:
				if (data->enable) {
					trig_osd->in_game_flags &= ~OBJcOSD_TriggerVolume_Enter_Disabled;
				} else {
					trig_osd->in_game_flags |= OBJcOSD_TriggerVolume_Enter_Disabled;
				}
			break;

			case AI2cTriggerVolumeCallbackType_EnableInsideScript:
				if (data->enable) {
					trig_osd->in_game_flags &= ~OBJcOSD_TriggerVolume_Inside_Disabled;
				} else {
					trig_osd->in_game_flags |= OBJcOSD_TriggerVolume_Inside_Disabled;
				}
			break;

			case AI2cTriggerVolumeCallbackType_EnableExitScript:
				if (data->enable) {
					trig_osd->in_game_flags &= ~OBJcOSD_TriggerVolume_Exit_Disabled;
				} else {
					trig_osd->in_game_flags |= OBJcOSD_TriggerVolume_Exit_Disabled;
				}
			break;

			case AI2cTriggerVolumeCallbackType_TriggerEntry:
				ONrGameState_FireTrigger_Entry(trig_osd, data->character);
			break;

			case AI2cTriggerVolumeCallbackType_TriggerInside:
				ONrGameState_FireTrigger_Inside(trig_osd, data->character);
			break;

			case AI2cTriggerVolumeCallbackType_TriggerExit:
				ONrGameState_FireTrigger_Exit(trig_osd, data->character);
			break;

			case AI2cTriggerVolumeCallbackType_Reset:
				OBJrTriggerVolume_Reset(inObject);
			break;

			default:
				UUmAssert(!"AI2iScript_TriggerVolume_Callback: unknown function type");
			break;
		}
	}

	// keep looking
	return UUcTrue;
}

// enable or disable a trigger volume
// syntax: trigvolume_enable trigvol1 0
//         trigvolume_enable trigvol1 0 all
//         trigvolume_enable trigvol1 0 entry
//         trigvolume_enable trigvol2 1 inside
//         trigvolume_enable trigvol1 0 exit
static UUtError AI2iScript_TriggerVolume_Enable(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	AI2tTriggerVolumeCallbackData data;

	data.name = inParameterList[0].val.str;
	data.enable = (inParameterList[1].val.i > 0) ? UUcTrue : UUcFalse;
	data.found = 0;
	data.script = NULL;

	if (inParameterListLength < 3) {
		// this isn't an enable for a specific kind of script, but a global one
		data.type = AI2cTriggerVolumeCallbackType_Enable;

	} else if (UUmString_IsEqual(inParameterList[2].val.str, "all")) {
		data.type = AI2cTriggerVolumeCallbackType_Enable;

	} else if (UUmString_IsEqual(inParameterList[2].val.str, "entry")) {
		data.type = AI2cTriggerVolumeCallbackType_EnableEnterScript;

	} else if (UUmString_IsEqual(inParameterList[2].val.str, "inside")) {
		data.type = AI2cTriggerVolumeCallbackType_EnableInsideScript;

	} else if (UUmString_IsEqual(inParameterList[2].val.str, "exit")) {
		data.type = AI2cTriggerVolumeCallbackType_EnableExitScript;

	} else {
		SLrScript_ReportError(inErrorContext, "trigvolume_enable: enable type '%s' must be all, entry, exit or inside",
								inParameterList[2].val.str);
		return UUcError_None;
	}

#if TOOL_VERSION
	AI2gScript_TriggerVolume_Callback_Count = 0;
#endif

	OBJrObjectType_EnumerateObjects(OBJcType_TriggerVolume, AI2iScript_TriggerVolume_Callback, (UUtUns32) &data);

#if TOOL_VERSION
	if (0 == AI2gScript_TriggerVolume_Callback_Count) {
		COrConsole_Printf("failed to find trigvolume_enable %s", data.name);
	}
#endif

	if (data.found) {
		return UUcError_None;
	} else {
		SLrScript_ReportError(inErrorContext, "trigvolume_enable: no trigger volume '%s' found", data.name);
		return UUcError_None;
	}
}

// set the script for a trigger volume
// syntax: trigvolume_setscript trigvol1 spawn_badguys entry
static UUtError AI2iScript_TriggerVolume_SetScript(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	AI2tTriggerVolumeCallbackData data;

	data.name = inParameterList[0].val.str;
	data.enable = UUcFalse;
	data.found = 0;
	data.script = inParameterList[1].val.str;

	if (UUmString_IsEqual(inParameterList[2].val.str, "entry")) {
		data.type = AI2cTriggerVolumeCallbackType_SetEnterScript;

	} else if (UUmString_IsEqual(inParameterList[2].val.str, "inside")) {
		data.type = AI2cTriggerVolumeCallbackType_SetInsideScript;

	} else if (UUmString_IsEqual(inParameterList[2].val.str, "exit")) {
		data.type = AI2cTriggerVolumeCallbackType_SetExitScript;

	} else {
		SLrScript_ReportError(inErrorContext, "trigvolume_setscript: type '%s' must be entry, exit or inside",
								inParameterList[2].val.str);
		return UUcError_None;
	}

	OBJrObjectType_EnumerateObjects(OBJcType_TriggerVolume, AI2iScript_TriggerVolume_Callback, (UUtUns32) &data);

	if (data.found) {
		return UUcError_None;
	} else {
		SLrScript_ReportError(inErrorContext, "trigvolume_setscript: no trigger volume '%s' found", data.name);
		return UUcError_None;
	}
}

// fire off a trigger volume
// syntax: trigvolume_trigger trigvol1 entry			// uses player character
//         trigvolume_trigger trigvol1 inside 1004
//         trigvolume_trigger trigvol1 exit mr_man
static UUtError	AI2iScript_TriggerVolume_Trigger(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	AI2tTriggerVolumeCallbackData data;
	ONtCharacter *character;

	data.name = inParameterList[0].val.str;
	data.enable = UUcFalse;
	data.found = 0;
	data.script = NULL;

	if (UUmString_IsEqual(inParameterList[1].val.str, "entry")) {
		data.type = AI2cTriggerVolumeCallbackType_TriggerEntry;

	} else if (UUmString_IsEqual(inParameterList[1].val.str, "inside")) {
		data.type = AI2cTriggerVolumeCallbackType_TriggerInside;

	} else if (UUmString_IsEqual(inParameterList[1].val.str, "exit")) {
		data.type = AI2cTriggerVolumeCallbackType_TriggerExit;

	} else {
		SLrScript_ReportError(inErrorContext, "trigvolume_trigger: type '%s' must be entry, exit or inside",
								inParameterList[1].val.str);
		return UUcError_None;
	}

	if (inParameterListLength < 3) {
		// no character specified, use player character
		character = ONrGameState_GetPlayerCharacter();
	} else {
		character = AI2rScript_ParseCharacter(&inParameterList[2], UUcFalse);
		if (character == NULL) {
			AI2rScript_NoCharacterError(&inParameterList[2], "trigvolume_trigger", inErrorContext);
			return UUcError_None;
		}
	}

	UUmAssertReadPtr(character, sizeof(ONtCharacter));
	data.character = character;

	OBJrObjectType_EnumerateObjects(OBJcType_TriggerVolume, AI2iScript_TriggerVolume_Callback, (UUtUns32) &data);

	if (data.found) {
		return UUcError_None;
	} else {
		SLrScript_ReportError(inErrorContext, "trigvolume_trigger: no trigger volume '%s' found", data.name);
		return UUcError_None;
	}
}

// reset a trigger volume to its default state
// syntax: trigvolume_reset trigvol1
static UUtError	AI2iScript_TriggerVolume_Reset(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	AI2tTriggerVolumeCallbackData data;

	data.name = inParameterList[0].val.str;
	data.type = AI2cTriggerVolumeCallbackType_Reset;
	data.enable = UUcFalse;
	data.found = 0;
	data.script = NULL;

	OBJrObjectType_EnumerateObjects(OBJcType_TriggerVolume, AI2iScript_TriggerVolume_Callback, (UUtUns32) &data);

	if (data.found) {
		return UUcError_None;
	} else {
		SLrScript_ReportError(inErrorContext, "trigvolume_trigger: no trigger volume '%s' found", data.name);
		return UUcError_None;
	}
}

// create a new weapon at a flag
// syntax: weapon_spawn w5_sbg 1013
static UUtError AI2iScript_WeaponSpawn(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	WPtWeaponClass *weapon_class;
	WPtWeapon *weapon;
	ONtFlag flag;
	UUtError error;

	// get the weapon class
	error = TMrInstance_GetDataPtr(WPcTemplate_WeaponClass, inParameterList[0].val.str, &weapon_class);
	if (error != UUcError_None) {
		COrConsole_Printf("### weapon_spawn: unknown weapon type '%s'", inParameterList[0].val.str);
		return UUcError_None;
	}

	// find the flag
	if (!ONrLevel_Flag_ID_To_Flag((UUtUns16) inParameterList[1].val.i, &flag)) {
		COrConsole_Printf("### weapon_spawn: could not find flag %d", inParameterList[1].val.i);
		return UUcError_None;
	}

	weapon = WPrSpawnAtFlag(weapon_class, &flag);
	if (weapon == NULL) {
		COrConsole_Printf("### weapon_spawn: could not create weapon");
	}
	return UUcError_None;
}

// create a new powerup at a flag
// syntax: powerup_spawn invis 1099
static UUtError AI2iScript_PowerupSpawn(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	WPtPowerup *powerup;
	UUtUns32 itr;
	UUtUns16 amount;
	ONtFlag flag;

	// get the powerup type
	for (itr = 0; itr < WPcPowerup_NumTypes; itr++) {
		if (UUmString_IsEqual(inParameterList[0].val.str, WPgPowerupName[itr])) {
			break;
		}
	}
	if (itr >= WPcPowerup_NumTypes) {
		COrConsole_Printf("### powerup_spawn: unknown powerup type '%s'... valid powerup types are:", inParameterList[0].val.str);
		for (itr = 0; itr < WPcPowerup_NumTypes; itr++) {
			COrConsole_Printf("  %s", WPgPowerupName[itr]);
		}
		return UUcError_None;
	}

	// find the flag
	if (!ONrLevel_Flag_ID_To_Flag((UUtUns16) inParameterList[1].val.i, &flag)) {
		COrConsole_Printf("### powerup_spawn: could not find flag %d", inParameterList[1].val.i);
		return UUcError_None;
	}

	if (inParameterListLength < 3) {
		amount = WPrPowerup_DefaultAmount((WPtPowerupType) itr);
	} else {
		amount = (UUtUns16) inParameterList[2].val.i;
	}

	powerup = WPrPowerup_SpawnAtFlag((WPtPowerupType) itr, amount, &flag);
	if (powerup == NULL) {
		COrConsole_Printf("### powerup_spawn: could not create powerup");
	}
	return UUcError_None;
}

// force a character to holster their weapon
// syntax: chr_forceholster mr_man 1
//         chr_forceholster mr_man 0 1             // second parameter is whether to force a draw
static UUtError	AI2iScript_ForceHolster(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(inParameterList, UUcFalse);

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "chr_forceholster", inErrorContext);
		return UUcError_None;
	}

	if (inParameterList[1].val.i > 0) {
		ONrCharacter_CinematicHolster(character);
	} else {
		UUtBool force_draw = ((inParameterListLength >= 3) && (inParameterList[2].val.i > 0));

		ONrCharacter_CinematicEndHolster(character, force_draw);
	}

	return UUcError_None;
}

// set a character's neutral behavior
// syntax: ai2_neutralbehavior mr_man none
//         ai2_neutralbehavior mr_man tctf_basement_1
static UUtError	AI2iScript_SetNeutralBehavior(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseAI(inParameterListLength, inParameterList);
	UUtUns16 neutral_id = (UUtUns16) -1;
	OBJtOSD_All osd;
	OBJtObject *object;

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "ai2_neutralbehavior", inErrorContext);
		return UUcError_None;
	}

	if (!UUmString_IsEqual(inParameterList[1].val.str, "none")) {
		UUrString_Copy(osd.osd.neutral_osd.name, inParameterList[1].val.str, sizeof(osd.osd.neutral_osd.name));

		object = OBJrObjectType_Search(OBJcType_Neutral, OBJcSearch_NeutralName, &osd);
		if (object == NULL) {
			COrConsole_Printf("### ai2_neutralbehavior for %s can't find behavior '%s'", character->player_name, inParameterList[1].val.str);
			return UUcError_None;
		} else {
			UUmAssert(object->object_type == OBJcType_Neutral);
			neutral_id = ((OBJtOSD_Neutral *) object->object_data)->id;
		}
	}

	AI2rNeutral_ChangeBehavior(character, neutral_id, UUcFalse);
	return UUcError_None;
}

// show memory usage
// syntax: ai2_showmem
static UUtError	AI2iScript_ShowMem(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	COrConsole_Printf("%d characters in array... ontcharacter %d of which %d is ai2state", ONcMaxCharacters, sizeof(ONtCharacter), sizeof(AI2tState));

	COrConsole_Printf("movement %d, knowledge %d, executor %d, job %d x2, pursuit %d, combat %d", sizeof(AI2tMovementState), sizeof(AI2tKnowledgeState),
						sizeof(AI2tExecutorState), sizeof(AI2tGoalState), sizeof(AI2tPursuitSettings), sizeof(AI2tCombatSettings));
	return UUcError_None;
}

// makes the AI take control of player movement
// syntax: ai2_takecontrol 1
static UUtError	AI2iScript_TakeControl(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *player = ONrGameState_GetPlayerCharacter();
	ONtActiveCharacter *active_character;

	if (player == NULL)
		return UUcError_None;

	if (inParameterList[0].val.i) {

		// disable sprinting and facingModifier, because the AI movement code doesn't expect to be able to do it
		ONrCharacter_SetSprintVarient(player, UUcFalse);
		active_character = ONrForceActiveCharacter(player);
		if (active_character != NULL) {
			active_character->last_forward_tap = 0;
		}
		ONrDesireFacingModifier(player, 0);

		// take control
		player->flags |= ONcCharacterFlag_AIMovement;
		AI2rPath_Initialize(player);
		AI2rMovement_Initialize(player);
		AI2rExecutor_Initialize(player);
	} else {
		// relinquish control
		player->flags &= ~ONcCharacterFlag_AIMovement;
	}

	return UUcError_None;
}

// force an AI to be active
// syntax: ai2_active mr_man
static UUtError AI2iScript_MakeActive(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseAI(inParameterListLength, inParameterList);
	UUtError error;

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "ai2_active", inErrorContext);
		return UUcError_None;
	} else {
		error = ONrCharacter_MakeActive(character, UUcTrue);
		if (error != UUcError_None) {
			COrConsole_Printf("### maximum number of characters exceeded");
			return UUcError_None;
		} else {
			return UUcError_None;
		}
	}
}

// force an AI to be inactive
// syntax: ai2_inactive mr_man
static UUtError AI2iScript_MakeInactive(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseAI(inParameterListLength, inParameterList);
	UUtError error;

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "ai2_inactive", inErrorContext);
		return UUcError_None;
	} else {
		error = ONrCharacter_MakeInactive(character);
		if (error != UUcError_None) {
			COrConsole_Printf("### cannot make character inactive");
			return UUcError_None;
		} else {
			return UUcError_None;
		}
	}
}

// change the team of an AI
// syntax: ai2_changeteam mr_man syndicate
static UUtError AI2iScript_ChangeTeam(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(&inParameterList[0], UUcFalse);
	UUtUns32 itr;

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "ai2_changeteam", inErrorContext);
		return UUcError_None;
	} else {
		for (itr = 0; itr < OBJcCharacter_TeamMaxNamed; itr++) {
			if (UUmString_IsEqual(inParameterList[1].val.str, OBJgCharacter_TeamName[itr])) {
				character->teamNumber = (UUtUns16) itr;
				return UUcError_None;
			}
		}
 		SLrScript_ReportError(inErrorContext, "### ai2_changeteam: couldn't find team '%s'", inParameterList[1].val.str);
		return UUcError_None;
	}
}

static UUtError	AI2iScript_HasLSI(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
									SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
									UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(&inParameterList[0], UUcFalse);

	ioReturnValue->type = SLcType_Bool;
	ioReturnValue->val.b = 0;

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "chr_has_lsi", inErrorContext);
	}
	else {
		ioReturnValue->val.b = character->inventory.has_lsi > 0;
	}

	return UUcError_None;
}

static UUtError	AI2iScript_HasEmptyWeapon(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
									SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
									UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(&inParameterList[0], UUcFalse);

	ioReturnValue->type = SLcType_Bool;
	ioReturnValue->val.b = 0;

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "chr_has_empty_weapon", inErrorContext);
	}
	else {
		if (character->inventory.weapons[0] == NULL) {
			ioReturnValue->val.b = UUcFalse;

		} else if (WPrHasAmmo(character->inventory.weapons[0])) {
			ioReturnValue->val.b = UUcFalse;

		} else if (ONrCharacter_HasAmmoForReload(character, character->inventory.weapons[0])) {
			ioReturnValue->val.b = UUcFalse;

		} else if (ONrCharacter_IsReloading(character)) {
			ioReturnValue->val.b = UUcFalse;

		} else {
			// character has a weapon that is empty, and no reloads for it, and they're not currently
			// reloading
			ioReturnValue->val.b = UUcTrue;
		}
	}

	return UUcError_None;
}

static UUtError	AI2iScript_IsPlayer(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
									SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
									UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(&inParameterList[0], UUcFalse);

	ioReturnValue->type = SLcType_Bool;
	ioReturnValue->val.b = 0;

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "chr_is_player", inErrorContext);
	}
	else {
		ioReturnValue->val.b = ONgGameState->local.playerCharacter == character;
	}

	return UUcError_None;
}

// set an AI to be noncombatant
static UUtError AI2iScript_NonCombatant(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseAI(inParameterListLength, &inParameterList[0]);

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "ai2_noncombatant", inErrorContext);
		return UUcError_None;
	} else {
		if (inParameterList[1].val.i > 0) {
			character->ai2State.flags |= AI2cFlag_NonCombatant;

			// transition out of any combatant-only states
			switch(character->ai2State.currentGoal) {
				case AI2cGoal_Combat:
				case AI2cGoal_Pursuit:
					AI2rReturnToJob(character, UUcTrue, UUcTrue);
				break;
			}
			character->ai2State.knowledgeState.callback = &AI2rNonCombatant_NotifyKnowledge;
		} else {
			character->ai2State.flags &= ~AI2cFlag_NonCombatant;

			// transition out of any noncombatant-only states
			if (character->ai2State.currentGoal == AI2cGoal_Panic) {
				AI2rReturnToJob(character, UUcTrue, UUcTrue);
			}
			character->ai2State.knowledgeState.callback = &AI2rAlert_NotifyKnowledge;
		}
	}
	return UUcError_None;
}

// set an AI to panic
static UUtError AI2iScript_Panic(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseAI(inParameterListLength, &inParameterList[0]);
	UUtUns32 timer;

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "ai2_panic", inErrorContext);
		return UUcError_None;
	} else {
		if (inParameterListLength < 2) {
			timer = 1000000;		// approximation to forever

		} else if (inParameterList[1].val.i == 0) {
			if (character->ai2State.currentGoal == AI2cGoal_Panic) {
				AI2rReturnToJob(character, UUcTrue, UUcTrue);
			}
		} else {
			timer = inParameterList[1].val.i;
		}

		AI2rCharacter_Panic(character, NULL, timer);
	}
	return UUcError_None;
}

// set an AI to be blind
static UUtError AI2iScript_MakeBlind(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseAI(inParameterListLength, &inParameterList[0]);

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "ai2_makeblind", inErrorContext);
		return UUcError_None;
	} else {
		if (inParameterList[1].val.i) {
			character->ai2State.flags |= AI2cFlag_Blind;
		} else {
			character->ai2State.flags &= ~AI2cFlag_Blind;
		}
	}
	return UUcError_None;
}

// set an AI to be deaf
static UUtError AI2iScript_MakeDeaf(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseAI(inParameterListLength, &inParameterList[0]);

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "ai2_makedeaf", inErrorContext);
		return UUcError_None;
	} else {
		if (inParameterList[1].val.i) {
			character->ai2State.flags |= AI2cFlag_Deaf;
		} else {
			character->ai2State.flags &= ~AI2cFlag_Deaf;
		}
	}
	return UUcError_None;
}

// set an AI to ignore the player
static UUtError AI2iScript_MakeIgnorePlayer(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseAI(inParameterListLength, &inParameterList[0]);

	if (character == NULL) {
		AI2rScript_NoCharacterError(inParameterList, "ai2_makeignoreplayer", inErrorContext);
		return UUcError_None;
	} else {
		if (inParameterList[1].val.i) {
			character->ai2State.flags |= AI2cFlag_IgnorePlayer;
		} else {
			character->ai2State.flags &= ~AI2cFlag_IgnorePlayer;
		}
	}
	return UUcError_None;
}

static UUtError AI2iScript_BarabusCamera(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	UUtBool valid_parameter_list = UUcFalse;

	if (4 != inParameterListLength) {
		COrConsole_Printf("invalid # of parameters to cm_barabus");
	}
	else if (SLcType_Float != inParameterList[1].type) {
		COrConsole_Printf("invalid away paramter to cm_barabus");
	}
	else if (SLcType_Float != inParameterList[2].type) {
		COrConsole_Printf("invalid up paramter to cm_barabus");
	}
	else if (SLcType_Int32 != inParameterList[3].type) {
		COrConsole_Printf("invalid time paramter to cm_barabus");
	}
	else {
		valid_parameter_list = UUcTrue;
	}

	if (!valid_parameter_list) {
		COrConsole_Printf("expected: cm_barabus <id> <away> <up> <time>");
		goto exit;
	}

	{
		ONtCharacter *character = AI2rScript_ParseAI(inParameterListLength, &inParameterList[0]);
		CAtViewData view_data;
		float away = inParameterList[1].val.f;
		float up = inParameterList[2].val.f;
		UUtInt32 time = inParameterList[3].val.i;
		M3tPoint3D target_location;

		if (NULL == character) {
			COrConsole_Printf("failed to find character");
			goto exit;
		}

		target_location = character->actual_position;
		target_location.y += 4.f;

		view_data.location = ONgGameState->local.camera->viewData.location;
		view_data.location.y = target_location.y;


		// view vector = normalize(target_location - location)
		MUmVector_Subtract(view_data.viewVector, target_location, view_data.location);
		MUmVector_Normalize(view_data.viewVector);

		// location is 25 units away (9 in the xz, 16 in y)
		view_data.location = target_location;
		MUmVector_IncrementMultiplied(view_data.location, view_data.viewVector, -away);
		view_data.location.y += up;

		// view vector = normalize(target_location - location)
		MUmVector_Subtract(view_data.viewVector, target_location, view_data.location);
		MUmVector_Normalize(view_data.viewVector);

		view_data.upVector = CArBuildUpVector(&view_data.viewVector);

		CArInterpolate_Enter_ViewData(&view_data, time);
	}

exit:
	return UUcError_None;
}


static UUtError			AI2iScript_LockActive(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseAI(inParameterListLength, &inParameterList[0]);

	if (NULL == character) {
		COrConsole_Printf("chr_lock_active failed to find a character to lock");
	}
	else {
		{
			ONtActiveCharacter *active_character = ONrForceActiveCharacter(character);

			if (NULL == active_character) {
				COrConsole_Printf("failed to force character %s active", character->player_name);
			}
		}

		character->flags |= ONcCharacterFlag_ActiveLock;
	}
	
	return UUcError_None;
}

static UUtError			AI2iScript_UnlockActive(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseAI(inParameterListLength, &inParameterList[0]);

	if (NULL == character) {
		COrConsole_Printf("chr_lock_active failed to find a character to unlock");
	}
	else {
		character->flags &= ~ONcCharacterFlag_ActiveLock;
	}
	
	return UUcError_None;
}

// makes a character unstoppable
// syntax: chr_unstoppable punchingbag 1
static UUtError	AI2iScript_Unstoppable(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character;
	UUtBool unstoppable = UUcTrue;
	
	character = AI2rScript_ParseCharacter(&inParameterList[0], UUcFalse);
	if (character == NULL) {
		AI2rScript_NoCharacterError(&inParameterList[0], "chr_unstoppable", inErrorContext);
		return UUcError_None;
	}

	if (inParameterList[1].val.i > 0) {
		character->flags |= ONcCharacterFlag_Unstoppable;
	} else {
		character->flags &= ~ONcCharacterFlag_Unstoppable;
	}
	
	return UUcError_None;
}

// makes a character invincible
// syntax: chr_invincible punchingbag 1
static UUtError	AI2iScript_Invincible(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character;
	UUtBool unstoppable = UUcTrue;
	
	character = AI2rScript_ParseCharacter(&inParameterList[0], UUcFalse);
	if (character == NULL) {
		AI2rScript_NoCharacterError(&inParameterList[0], "chr_invincible", inErrorContext);
		return UUcError_None;
	}

	if (inParameterList[1].val.i > 0) {
		character->flags |= ONcCharacterFlag_Invincible;
	} else {
		character->flags &= ~ONcCharacterFlag_Invincible;
	}
	
	return UUcError_None;
}

// makes a character unkillable
// syntax: chr_unkillable punchingbag 1
static UUtError	AI2iScript_Unkillable(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character;
	UUtBool unstoppable = UUcTrue;
	
	character = AI2rScript_ParseCharacter(&inParameterList[0], UUcFalse);
	if (character == NULL) {
		AI2rScript_NoCharacterError(&inParameterList[0], "chr_unkillable", inErrorContext);
		return UUcError_None;
	}

	if (inParameterList[1].val.i > 0) {
		character->flags |= ONcCharacterFlag_Unkillable;
	} else {
		character->flags &= ~ONcCharacterFlag_Unkillable;
	}
	
	return UUcError_None;
}

// prints the location of a character
static UUtError	AI2iScript_Where(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseOptionalAI(inParameterListLength, inParameterList);
	char msgbuf[256], tempbuf[64];

	if ((character != NULL) || (CAgCamera.mode == CAcMode_Follow)) {
		if (character == NULL) {
			character = ONrGameState_GetPlayerCharacter();
		}

		sprintf(msgbuf, "Character '%s' is at (%f, %f, %f), facing %03.0f degrees",
			character->player_name, character->location.x, character->location.y, character->location.z, character->facing * M3cRadToDeg);
		if (character->currentNode == NULL) {
			sprintf(tempbuf, ", not in a BNV.");
		} else {
			sprintf(tempbuf, ", in BNV #%d.", character->currentNode->index);
		}
		COrConsole_Printf("%s%s", msgbuf, tempbuf);
	} else {
		COrConsole_Printf("Camera is at (%f, %f, %f), looking in direction (%f %f %f)",
							CAgCamera.viewData.location.x, CAgCamera.viewData.location.y, CAgCamera.viewData.location.z,
							CAgCamera.viewData.viewVector.x, CAgCamera.viewData.viewVector.y, CAgCamera.viewData.viewVector.z);
	}

	return UUcError_None;
}

// prints a list of nearby characters
static UUtError	AI2iScript_Who(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	M3tPoint3D cur_location;
	ONtCharacter *character, **character_list;
	const char *location_type;
	char msgbuf[256], tempbuf[64];
	UUtUns32 itr, character_list_count;

	if (CAgCamera.mode == CAcMode_Follow) {
		character = ONrGameState_GetPlayerCharacter();
		cur_location = character->actual_position;
		location_type = "player";
	} else {
		cur_location = CAgCamera.viewData.location;
		location_type = "camera";
	}

	COrConsole_Printf("Listing all AIs near %s's location (%f %f %f)", location_type, cur_location.x, cur_location.y, cur_location.z);
	
	character_list = ONrGameState_PresentCharacterList_Get();
	character_list_count = ONrGameState_PresentCharacterList_Count();

	for (itr = 0; itr < character_list_count; itr++) {
		character = character_list[itr];

		if (MUrPoint_Distance_Squared(&cur_location, &character->actual_position) > UUmSQR(100.0f))
			continue;

		sprintf(msgbuf, "  %s (class %s, #%d)", character->player_name, TMrInstance_GetInstanceName(character->characterClass),
				ONrCharacter_GetIndex(character));
		
		if (character->charType == ONcChar_AI2) {
			// print out a tiny amount of AI debugging information
			sprintf(tempbuf, " (state %s)", AI2cGoalName[character->ai2State.currentGoal]);
			strcat(msgbuf, tempbuf);

			if (character->pathState.last_pathfinding_error != AI2cError_Pathfinding_NoError) {
				strcat(msgbuf, " (pathfinding error ");
				switch(character->pathState.last_pathfinding_error) {
					case AI2cError_Pathfinding_NoBNVAtStart:
						strcat(msgbuf, "no-bnv-here");
					break;

					case AI2cError_Pathfinding_NoBNVAtDest:
						strcat(msgbuf, "no-bnv-at-target");
					break;

					case AI2cError_Pathfinding_NoConnectionsFromStart:
						strcat(msgbuf, "graph-noconnections");
					break;

					case AI2cError_Pathfinding_NoPath:
						strcat(msgbuf, "graph-nopath");
					break;

					default:
						strcat(msgbuf, "unknown");
					break;
				}
				strcat(msgbuf, ")");
			}
		}
		COrConsole_Printf("%s", msgbuf);
	}

	return UUcError_None;
}


static UUtError			AI2iScript_KilledGriffen(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONrWeKilledGriffen(inParameterList->val.b);

	return UUcError_None;
}

static UUtError			AI2iScript_DidKillGriffen(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ioReturnValue->val.b = ONrDidWeKillGriffen();
	ioReturnValue->type = SLcType_Bool;

	return UUcError_None;
}

static UUtError			AI2iScript_Difficulty(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ioReturnValue->val.i = ONrPersist_GetDifficulty();
	ioReturnValue->type = SLcType_Int32;

	return UUcError_None;
}

#if 0
static UUtError			AI2iScript_Holograph(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character;
	UUtBool holograph_mode;
	
	if (inParameterListLength != 2) {
		COrConsole_Printf("chr_holograph had an invalid parameter list length");
		goto exit;
	}

	if (SLcType_Int32 == inParameterList[1].type) {
		holograph_mode = inParameterList[1].val.i > 0;
	}
	else if (SLcType_Bool == inParameterList[1].type) {
		holograph_mode = inParameterList[1].val.b;
	}
	else {
		COrConsole_Printf("chr_holograph had an invalid parameter list length");
		goto exit;
	}

	character = AI2rScript_ParseCharacter(&inParameterList[0], UUcFalse);

	if (NULL == character) {
		COrConsole_Printf("chr_holograph failed to find character");
		goto exit;
	}

	// CB: I needed a character flag and removed ONcCharacterFlag_Holograph
/*	if (holograph_mode) {
		character->flags |= ONcCharacterFlag_Holograph;
	}
	else {
		character->flags &= ~ONcCharacterFlag_Holograph;
	}*/

exit:
	return UUcError_None;
}
#endif



// makes a character say a line of dialogue
// syntax: chr_talk konoko c11_32_61 0 30
//         chr_talk red red_grunt1 0 0 pain
static UUtError	AI2iScript_Talk(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(&inParameterList[0], UUcFalse);
	ONtSpeech speech;
	UUtUns32 itr, itr2, itr3, speech_type;

	// find the character
	if (character == NULL) {
		AI2rScript_NoCharacterError(&inParameterList[0], "chr_talk", inErrorContext);
		character = ONrGameState_GetPlayerCharacter();
	}

	// get a free entry in the talk buffer to store the sound name in
	for (itr = 0; itr < AI2cScript_TalkBufferSize; itr++) {
		if (!AI2gScript_TalkBufferInUse[itr]) {
			break;
		}
	}
	if (itr >= AI2cScript_TalkBufferSize) {
		// there are no free entries in the talk buffer
		AI2_ERROR(AI2cBug, AI2cSubsystem_HighLevel, AI2cError_HighLevel_TalkBufferFull, character,
					AI2cScript_TalkBufferSize, inParameterList[1].val.str, 0, 0);
		return UUcError_None;
	}

	UUrString_Copy(AI2gScript_TalkBuffer[itr], inParameterList[1].val.str, BFcMaxFileNameLength);	

	if (inParameterListLength < 5) {
		speech_type = ONcSpeechType_Override;
	} else {
		// determine the speech type
		for (itr2 = 0; itr2 < ONcSpeechType_Max; itr2++) {
			if (UUmString_IsEqual(ONcSpeechTypeName[itr2], inParameterList[4].val.str)) {
				break;
			}
		}

		if (itr2 >= ONcSpeechType_Max) {
			// this is not a valid speech type
			COrConsole_Printf("### chr_talk %s: unknown speech type %s. valid types are:",
				character->player_name, inParameterList[4].val.str);
			for (itr3 = 0; itr2 < ONcSpeechType_Max; itr2++) {
				COrConsole_Printf("  %s", ONcSpeechTypeName[itr2]);
			}
			return UUcError_None;
		}
	}

	speech.notify_string_inuse = &AI2gScript_TalkBufferInUse[itr];
	speech.pre_pause = inParameterList[2].val.i;
	speech.post_pause = inParameterList[3].val.i;
	speech.sound_name = AI2gScript_TalkBuffer[itr];
	speech.speech_type = speech_type;

	ONrSpeech_Say(character, &speech, UUcFalse);
	return UUcError_None;
}

// make a character play a vocalization
// syntax: chr_vocalize konoko taunt
static UUtError AI2iScript_Vocalize(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(&inParameterList[0], UUcFalse);
	UUtUns32 itr;
	UUtBool result;

	if (character == NULL) {
		AI2rScript_NoCharacterError(&inParameterList[0], "chr_vocalize", inErrorContext);
		character = ONrGameState_GetPlayerCharacter();
	}

	// get the type of vocalization
	for (itr = 0; itr < AI2cVocalization_Max; itr++) {
		if (UUmString_IsEqual(AI2cVocalizationTypeName[itr], inParameterList[1].val.str)) {
			break;
		}
	}
	if (itr >= AI2cVocalization_Max) {
		COrConsole_Printf("### chr_vocalize: unknown vocalization type %s... valid types are:", inParameterList[1].val.str);
		for (itr = 0; itr < AI2cVocalization_Max; itr++) {
			COrConsole_Printf("  %s", AI2cVocalizationTypeName[itr]);
		}
		return UUcError_None;
	}		

	result = AI2rVocalize(character, itr, UUcTrue);
	if (!result) {
		COrConsole_Printf("### character %s (variant %s) does not have a vocalization for '%s'",
			character->player_name, (character->characterClass->variant == NULL) ? "none" :
			character->characterClass->variant->name, inParameterList[1].val.str);
	}

	return UUcError_None;
}

static UUtError AI2iScript_ChrDeathLock(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(&inParameterList[0], UUcFalse);

	if (NULL == character) {
		AI2rScript_NoCharacterError(&inParameterList[0], "chr_death_lock", inErrorContext);
		goto exit;
	}

	if (inParameterList[1].val.i > 0) {
		character->flags |= ONcCharacterFlag_DeathLock;
	}
	else {
		character->flags &= ~ONcCharacterFlag_DeathLock;
	}

exit:
	return UUcError_None;
}

static UUtError AI2iScript_ChrUltraMode(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(&inParameterList[0], UUcFalse);

	if (NULL == character) {
		AI2rScript_NoCharacterError(&inParameterList[0], "chr_ultra_mode", inErrorContext);
		goto exit;
	}

	if (inParameterList[1].val.i > 0) {
		character->flags2 |= ONcCharacterFlag2_UltraMode;
	}
	else {
		character->flags2 &= ~ONcCharacterFlag2_UltraMode;
	}

exit:
	return UUcError_None;
}

static UUtError AI2iScript_ChrShadow(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(&inParameterList[0], UUcFalse);

	if (NULL == character) {
		AI2rScript_NoCharacterError(&inParameterList[0], "chr_shadow", inErrorContext);
		goto exit;
	}

	if (inParameterList[1].val.i > 0) {
		character->flags &= ~ONcCharacterFlag_NoShadow;
	}
	else {
		character->flags |= ONcCharacterFlag_NoShadow;
	}

exit:
	return UUcError_None;
}


static UUtError AI2iScript_BossShield(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(&inParameterList[0], UUcFalse);

	if (NULL == character) {
		AI2rScript_NoCharacterError(&inParameterList[0], "chr_boss_shield", inErrorContext);
	}
	else {
		character->flags |= ONcCharacterFlag_BossShield;
	}

	return UUcError_None;
}

static UUtError AI2iScript_WeaponImmune(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(&inParameterList[0], UUcFalse);

	if (NULL == character) {
		AI2rScript_NoCharacterError(&inParameterList[0], "chr_weapon_immnue", inErrorContext);
	}
	else {
		character->flags |= ONcCharacterFlag_WeaponImmune;
	}

	return UUcError_None;
}

static UUtError AI2iScript_Reset_Inventory(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(&inParameterList[0], UUcFalse);

	if (character == NULL) {
		AI2rScript_NoCharacterError(&inParameterList[0], "chr_inv_reset", inErrorContext);
	}
	else {
		character->inventory.ammo = 0;
		character->inventory.hypo = 0;
		character->inventory.cell = 0;
		character->inventory.has_lsi = 0;
		character->inventory.hypoRemaining = 0;
		character->inventory.invisibilityRemaining = 0;
		character->inventory.shieldRemaining = 0;

		UUmAssert(character->inventory.weapons[WPcPrimarySlot] == character->inventory.weapons[0]);

		if (NULL != character->inventory.weapons[0]) {
			ONrCharacter_UseNewWeapon(character, NULL);
		}
		
		UUmAssert(NULL == character->inventory.weapons[WPcPrimarySlot]);

		{
			UUtInt32 itr;

			for(itr = 0; itr < WPcMaxSlots; itr++)
			{
				WPtWeapon *weapon = character->inventory.weapons[itr];

				if (weapon != NULL) {			
					WPrDelete(weapon);
				}

				character->inventory.weapons[itr] = NULL;
			}
		}
	}

	return UUcError_None;
}

static UUtError AI2iScript_Leave_FightMode(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(&inParameterList[0], UUcFalse);

	if (character == NULL) {
		AI2rScript_NoCharacterError(&inParameterList[0], "chr_peace", inErrorContext);
	}
	else {
		ONrCharacter_ExitFightMode(character);
	}

	return UUcError_None;
}


static UUtError AI2iScript_TriggerVolume_Count(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	UUtInt32 count = OBJrTriggerVolumeID_CountInside(inParameterList[0].val.i);

	ioReturnValue->val.i = count;
	ioReturnValue->type = SLcType_Int32;

	return UUcError_None;
}

static UUtError AI2iScript_TriggerVolume_DeleteCharacters(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	UUtInt32 trigger_volume_id = inParameterList[0].val.i;

	COrConsole_Printf("deleting characters inside trigger volume %d", trigger_volume_id);

	OBJrTriggerVolumeID_DeleteCharactersInside(trigger_volume_id);

	return UUcError_None;
}


static UUtError AI2iScript_TriggerVolume_DeleteCorpses(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	UUtInt32 trigger_volume_id = inParameterList[0].val.i;

	COrConsole_Printf("deleting corpse inside trigger volume %d", trigger_volume_id);

	OBJrTriggerVolumeID_DeleteCorpsesInside(trigger_volume_id);

	return UUcError_None;
}



// poison a character over time
// syntax: chr_poison konoko 10 60			hurts konoko by 10 every 60 frames
//         chr_poison konoko 10 60 120		hurts konoko by 10 every 60 frames, starting after 120 frames
static UUtError AI2iScript_Poison(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	UUtUns32 initial_frames;
	ONtCharacter *character = AI2rScript_ParseCharacter(&inParameterList[0], UUcFalse);

	if (character == NULL) {
		AI2rScript_NoCharacterError(&inParameterList[0], "chr_poison", inErrorContext);
		return UUcError_None;
	}

	if (inParameterListLength < 4) {
		// initial-frames is the same as damage-frames
		UUmAssert(inParameterListLength >= 3);
		initial_frames = (UUtUns32) inParameterList[2].val.i;
	} else {
		// there is a separate initial-frames value specified
		initial_frames = (UUtUns32) inParameterList[3].val.i;
	}

	ONrCharacter_Poison(character, inParameterList[1].val.i, inParameterList[2].val.i, initial_frames);
	return UUcError_None;
}

// make a character play a pain sound
// syntax: chr_pain konoko medium
static UUtError AI2iScript_Pain(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	UUtUns32 itr;
	ONtCharacter *character = AI2rScript_ParseCharacter(&inParameterList[0], UUcFalse);

	if (character == NULL) {
		AI2rScript_NoCharacterError(&inParameterList[0], "chr_pain", inErrorContext);
		return UUcError_None;
	}

	for (itr = 0; itr < ONcPain_Max; itr++) {
		if (UUmString_IsEqual(inParameterList[1].val.str, ONcPainTypeName[itr])) {
			break;
		}
	}

	if (itr >= ONcPain_Max) {
		// can't find the pain type
		COrConsole_Printf("chr_pain: unknown pain type '%s'. possible pain types are:", inParameterList[1].val.str);
		for (itr = 0; itr < ONcPain_Max; itr++) {
			COrConsole_Printf("  %s", ONcPainTypeName[itr]);
		}
		return UUcError_None;
	}

	// play the pain sound
	ONrCharacter_PlayHurtSound(character, itr, NULL);
	return UUcError_None;
}

// make a character play a pain sound
// syntax: chr_pain mutantmuro 1.0
static UUtError AI2iScript_Super(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue)
{
	ONtCharacter *character = AI2rScript_ParseCharacter(&inParameterList[0], UUcFalse);

	if (character == NULL) {
		AI2rScript_NoCharacterError(&inParameterList[0], "chr_pain", inErrorContext);
		return UUcError_None;
	}

	character->external_super_amount = inParameterList[1].val.f;
	return UUcError_None;
}

UUtError AI2rInstallConsoleVariables(void)
{
	UUtError error;
	const AI2tCommandTable *current_command;
	const AI2tCommandTable command_table[] = {
		/*
		 * commands that work on all characters
		 */

		{"cm_barabus",			NULL,		NULL,			"special camera for barabus",								"[ai_name:string | script_id:int] away:float up:float time:int",									AI2iScript_BarabusCamera},

		// creation and deletion
		{"chr_create",			NULL,		NULL,			"creates a character from a .txt file",						"script_id:int [start_create:string{\"start\"} | ]",												AI2iScript_ChrCreate},
		{"chr_delete",			NULL,		NULL,			"deletes a character",										"[ai_name:string | script_id:int]",																	AI2iScript_ChrDelete},

		// controlling the active state
		{"chr_lock_active",		NULL,		NULL,			"locks the character active",								"[ai_name:string | script_id:int]",																	AI2iScript_LockActive},
		{"chr_unlock_active",	NULL,		NULL,			"locks the character active",								"[ai_name:string | script_id:int]",																	AI2iScript_UnlockActive},


		// movement
		{"chr_facetoflag",		NULL,		NULL,			"snaps a character's facing to a flag's facing",			"[ai_name:string | script_id:int] flag_id:int",														AI2iScript_FaceToFlag},
		{"chr_teleport",		NULL,		NULL,			"teleports a character to a flag",							"[ai_name:string | script_id:int] flag_id:int",														AI2iScript_Teleport},
		{"chr_nocollision",		NULL,		NULL,			"disables collision for a character",						"[ai_name:string | script_id:int] on_off:int{0 | 1}",												AI2iScript_NoCollision},
		{"ai2_takecontrol",		NULL,		NULL,			"makes the AI movement system take control of the player",	"on_off:int{0 | 1}",																				AI2iScript_TakeControl},

		// animation
		{"chr_playback",		"playback", NULL,			"plays back a film",										"[ai_name:string | script_id:int] film_name:string [mode:string{\"fromhere\" | \"interp\"} | ] [num_frames:int | ]",AI2iScript_Playback},
		{"chr_playback_block",	"playback_block", NULL,		"plays back a film and blocks until complete",				"[ai_name:string | script_id:int] film_name:string [mode:string{\"fromhere\" | \"interp\"} | ] [num_frames:int | ]",AI2iScript_Playback_Block},
		{"chr_playback_debug",	"playback_debug", NULL,		"dumps stats for a playback film",							"film_name:string",																					AI2iScript_Playback_Debug},
		{"chr_holdkey",			NULL,		NULL,			"forces a character to hold a key down for some frames",	"[ai_name:string | script_id:int] key_name:string num_frames:int",									AI2iScript_HoldKey},
		{"chr_animate",			NULL,		NULL,			"plays an animation on a character",						"[ai_name:string | script_id:int] anim_name:string [num_frames:int | ] [interp_frames:int | ]",		AI2iScript_Animate_NoBlock},
		{"chr_animate_block",	NULL,		NULL,			"plays an animation on a character and blocks until done",	"[ai_name:string | script_id:int] anim_name:string [num_frames:int | ] [interp_frames:int | ]",		AI2iScript_Animate_Block},
		{"chr_envanim",			NULL,		NULL,			"plays an environment animation on a character",			"[ai_name:string | script_id:int] anim:string [norotation:string{\"norotation\"} | ]",				AI2iScript_EnvAnimate_NoBlock},
		{"chr_envanim_block",	NULL,		NULL,			"plays an environment animation on a character and blocks",	"[ai_name:string | script_id:int] anim:string [norotation:string{\"norotation\"} | ]",				AI2iScript_EnvAnimate_Block},
		{"chr_envanim_stop",	NULL,		NULL,			"stops any environment animation on a character",			"[ai_name:string | script_id:int]",																	AI2iScript_EnvAnimate_Stop},
		{"chr_wait_animation",	NULL,		NULL,			"waits for a character to play a specific animation",		NULL,																								AI2iScript_WaitForAnimation},
		{"chr_wait_animtype",	NULL,		NULL,			"waits for a character to play a specific animation type",	NULL,																								AI2iScript_WaitForAnimType},
		{"chr_wait_animstate",	NULL,		NULL,			"waits for a character to reach a specific animation state",NULL,																								AI2iScript_WaitForAnimState},

		// character properties
		{"chr_boss_shield",		NULL,		NULL,			"turns on the boss shield for a character",					"[ai_name:string | script_id:int]",																	AI2iScript_BossShield},
		{"chr_weapon_immune",	NULL,		NULL,			"makes a character weapon immune",							"[ai_name:string | script_id:int]",																	AI2iScript_WeaponImmune},
		{"chr_dontaim",			NULL,		NULL,			"disables the weapon variant for a character",				"[ai_name:string | script_id:int] on_off:int{0 | 1}",												AI2iScript_DontAim},
		{"chr_set_health",		NULL,		NULL,			"sets a characters health by script id",					"[ai_name:string | script_id:int] hit_points:int",													AI2iScript_SetHealth},
		{"chr_full_health",		NULL,		NULL,			"sets a characters health to full by script id",			"[ai_name:string | script_id:int]",																	AI2iScript_SetFullHealth},
		{"chr_giveweapon",		NULL,		NULL,			"gives a character a new weapon",							"[ai_name:string | script_id:int] weapon_name:string",												AI2iScript_GiveWeapon},
		{"chr_givepowerup",		NULL,		NULL,			"gives a character a powerup",								"[ai_name:string | script_id:int] powerup:string [amount:int | ]",									AI2iScript_GivePowerup},
		{"chr_forceholster",	NULL,		NULL,			"forces a character to holster their weapon",				"[ai_name:string | script_id:int] holster:int{0 | 1} [force_draw:int{0 | 1} | ]",					AI2iScript_ForceHolster},
		{"chr_changeteam",		NULL,		NULL,			"changes a character's team",								"[ai_name:string | script_id:int] team_name:string",												AI2iScript_ChangeTeam},
		{"chr_unstoppable",		NULL,		NULL,			"makes a character unstoppable",							"[ai_name:string | script_id:int] [on_off:int{0 | 1}]",												AI2iScript_Unstoppable},
		{"chr_invincible",		NULL,		NULL,			"makes a character invincible",								"[ai_name:string | script_id:int] [on_off:int{0 | 1}]",												AI2iScript_Invincible},		
		{"chr_unkillable",		NULL,		NULL,			"makes a character unkillable",								"[ai_name:string | script_id:int] [on_off:int{0 | 1}]",												AI2iScript_Unkillable},		
		{"chr_talk",			NULL,		NULL,			"causes a character to play a line of dialogue",			"[ai_name:string | script_id:int] sound_name:string pre_pause:int post_pause:int [priority:string | ]",	AI2iScript_Talk},
		{"chr_vocalize",		NULL,		NULL,			"makes a character utter a vocalization",					"[ai_name:string | script_id:int] type:string",														AI2iScript_Vocalize},
		{"chr_inv_reset",		NULL,		NULL,			"clears a characters inventory",							"[ai_name:string | script_id:int]",																	AI2iScript_Reset_Inventory},
		{"chr_peace",			NULL,		NULL,			"turns off fight mode",										"[ai_name:string | script_id:int]",																	AI2iScript_Leave_FightMode},
		{"chr_poison",			NULL,		NULL,			"slowly poisons a character",								"[ai_name:string | script_id:int] damage:int interval:int [initial_interval:int | ]",				AI2iScript_Poison},
		{"chr_pain",			NULL,		NULL,			"forces a character to play a pain sound",					"[ai_name:string | script_id:int] pain_type:string",												AI2iScript_Pain},
		{"chr_super",			NULL,		NULL,			"set's a character's super value",							"[ai_name:string | script_id:int] super_amount:float",												AI2iScript_Super},
		{"chr_wait_health",		NULL,		NULL,			"waits until a character's health falls below a value",		"[ai_name:string | script_id:int] hit_points:int",													AI2iScript_WaitForHealth},
		{"chr_shadow",			NULL,		NULL,			"turns of the shadow for this character",					"[ai_name:string | script_id:int] on_off:int{0 | 1}",												AI2iScript_ChrShadow},
		{"chr_death_lock",		NULL,		NULL,			"this character will not die too much",						"[ai_name:string | script_id:int] on_off:int{0 | 1}",												AI2iScript_ChrDeathLock},
		{"chr_ultra_mode",		NULL,		NULL,			"enables ultra mode for a character",						"[ai_name:string | script_id:int] on_off:int{0 | 1}",												AI2iScript_ChrUltraMode},

		/*
		 * commands that only work on AIs
		 */

		// creation and deletion
		{"ai2_reset",			NULL,		NULL,			"resets AI as if start of level",							"[reset_player:int | ]",																			AI2iScript_Reset},
		{"ai2_spawn",			NULL,		NULL,			"creates and starts an AI from a character object",			"ai_name:string [force_spawn:string{\"force\"} | ]",												AI2iScript_Spawn},
		{"ai2_spawnall",		NULL,		NULL,			"spawns all AI, even those not initially present",			"",																									AI2iScript_SpawnAll},
		{"ai2_kill",			NULL,		NULL,			"kills one or more AIs",									"[param1:string | ] [param2:string | ]",															AI2iScript_Debug_Smite},

		// state changing
		{"ai2_allpassive",		NULL,		NULL,			"stops all AIs from thinking for themselves",				"passive:int{0 | 1}",																				AI2iScript_AllPassive},
		{"ai2_passive",			"chr_freeze","chr_neutral",	"stops an AI from thinking for itself",						"[ai_name:string | script_id:int] passive:int{0 | 1}",												AI2iScript_Passive},
		{"ai2_active",			NULL,		NULL,			"forces an AI into active mode",							"[ai_name:string | script_id:int]",																	AI2iScript_MakeActive},
		{"ai2_inactive",		NULL,		NULL,			"forces an AI into inactive mode",							"[ai_name:string | script_id:int]",																	AI2iScript_MakeInactive},
		{"ai2_setjobstate",		NULL,		NULL,			"tells an AI to take its current state as its job",			"[ai_name:string | script_id:int]",																	AI2iScript_MakeCurrentStateJob},
		{"ai2_dopath",			NULL,		NULL,			"tells an AI to run a particular path",						"[ai_name:string | script_id:int] path_name:string",												AI2iScript_SetState_Patrol},
		{"ai2_doalarm",			NULL,		NULL,			"tells an AI to run for an alarm",							"[ai_name:string | script_id:int] [console_id:int | ]",												AI2iScript_SetState_Alarm},
		{"ai2_idle",			NULL,		NULL,			"tells an AI to become idle",								"[ai_name:string | script_id:int]",																	AI2iScript_SetState_Idle},
		{"ai2_neutralbehavior",	NULL,		NULL,			"sets up an AI's neutral-interaction",						"[ai_name:string | script_id:int] behavior:string",													AI2iScript_SetNeutralBehavior},
		{"ai2_noncombatant",	NULL,		NULL,			"sets or clears an AI's non-combatant state",				"[ai_name:string | script_id:int] non_combatant:int{0 | 1}",										AI2iScript_NonCombatant},
		{"ai2_panic",			NULL,		NULL,			"makes an AI panic or not panic",							"[ai_name:string | script_id:int] timer:int",														AI2iScript_Panic},
		{"ai2_makeaware",		NULL,		NULL,			"makes an AI aware of another character",					"[ai_name:string | script_id:int] [target_name:string | target_script_id:int]",						AI2iScript_MakeAware},
		{"ai2_attack",			NULL,		NULL,			"forces an AI to attack another character",					"[ai_name:string | script_id:int] [target_name:string | target_script_id:int]",						AI2iScript_Attack},

		// knowledge
		{"ai2_forget",			NULL,		NULL,			"makes one or all AIs forget they saw anything",			"[ai_name:string | script_id:int | ]  [forget_char:string | ]",										AI2iScript_Debug_Forget},
		{"ai2_setalert",		NULL,		NULL,			"sets the alert state of an AI",							"[ai_name:string | script_id:int] alert:string",													AI2iScript_Debug_SetAlert},
		{"ai2_tripalarm",		NULL,		NULL,			"trips an alarm",											"alarm_id:int [ai_name:string | script_id:int | ]",													AI2iScript_TripAlarm},
		{"ai2_makeblind",		NULL,		NULL,			"makes a single AI blind",									"[ai_name:string | script_id:int] on_off:int{0 | 1}",												AI2iScript_MakeBlind},
		{"ai2_makedeaf",		NULL,		NULL,			"makes a single AI deaf",									"[ai_name:string | script_id:int] on_off:int{0 | 1}",												AI2iScript_MakeDeaf},
		{"ai2_makeignoreplayer",NULL,		NULL,			"makes a single AI ignore the player",						"[ai_name:string | script_id:int] on_off:int{0 | 1}",												AI2iScript_MakeIgnorePlayer},

		// movement
		{"ai2_setmovementmode",	NULL,		NULL,			"sets an AI's current movement mode",						"[ai_name:string | script_id:int] mode:string",														AI2iScript_Debug_SetMovementMode},
		{"ai2_movetoflag",		NULL,		NULL,			"tells an AI to move to a flag",							"[ai_name:string | script_id:int] flag_id:int [setfacing:string{\"setfacing\"} | ]",				AI2iScript_MoveToFlag},
		{"ai2_lookatchar",		NULL,		NULL,			"tells an AI to look at a character",						"[ai_name:string | script_id:int] [ai_name:string | script_id:int]",								AI2iScript_LookAtChar},

		// error reporting
		{"ai2_set_reporterror",	NULL,		NULL,			"sets the level of errors to report at console",			"error_level:string [subsystem:string | ]",															AI2iScript_Set_ReportError},
		{"ai2_set_logerror",	NULL,		NULL,			"sets the level of errors to log to file",					"error_level:string [subsystem:string | ]",															AI2iScript_Set_LogError},
		{"ai2_set_handlesilenterror",NULL,	NULL,			"sets whether handled errors are silent",					"handle_silent:bool [subsystem:string | ]",															AI2iScript_Set_HandleSilentError},

		// special-case scripting commands
		{"ai2_barabbas_retrievegun", NULL,	NULL,			"makes barabbas retrieve his gun if it is lost",			"[ai_name:string | script_id:int]",																	AI2iScript_BarabbasRetrieveGun},

		/*
		 * shooting skill editing
		 */

		{"ai2_skill_select",	NULL,		NULL,			"selects a shooting skill to edit",							"char_class:string weapon_name:string",																AI2iScript_Skill_Select},
		{"ai2_skill_show",		NULL,		NULL,			"shows the currently selected shooting skill",				"",																									AI2iScript_Skill_Show},
		{"ai2_skill_revert",	NULL,		NULL,			"reverts the shooting skill being edited to the saved copy","",																									AI2iScript_Skill_Revert},
		{"ai2_skill_save",		NULL,		NULL,			"saves the shooting skill being edited out as a text file",	"",																									AI2iScript_Skill_Save},
		{"ai2_skill_recoil",	NULL,		NULL,			"sets an AI's recoil compensation amount (0-1)",			"recoil_compensation:float",																		AI2iScript_Skill_Recoil},
		{"ai2_skill_bestangle",	NULL,		NULL,			"sets an AI's best aiming angle in degrees",				"best_angle:float",																					AI2iScript_Skill_BestAngle},
		{"ai2_skill_error",		NULL,		NULL,			"sets an AI's grouping error",								"error_amount:float",																				AI2iScript_Skill_Error},
		{"ai2_skill_decay",		NULL,		NULL,			"sets an AI's grouping decay",								"decay_amount:float",																				AI2iScript_Skill_Decay},
		{"ai2_skill_inaccuracy",NULL,		NULL,			"sets an AI's shooting inaccuracy multiplier",				"inaccuracy:float",																					AI2iScript_Skill_Inaccuracy},
		{"ai2_skill_delaymin",	NULL,		NULL,			"sets an AI's min delay between shots, in frames",			"mindelay:int",																						AI2iScript_Skill_DelayMin},
		{"ai2_skill_delaymax",	NULL,		NULL,			"sets an AI's max delay between shots, in frames",			"maxdelay:int",																						AI2iScript_Skill_DelayMax},

		/*
		 * game mechanics
		 */

		{"reset_mechanics",		NULL,		NULL,			"resets all level mechanics (triggers, turrets, consoles, etc...) to initial state",	"",																		AI2iScript_Mechanics_Reset},

		// doors
		{"door_jam",			NULL,		NULL,			"jams a door in its current state",							"door_id:int",																						AI2iScript_Door_Jam},
		{"door_unjam",			NULL,		NULL,			"unjams a door so characters can open it",					"door_id:int",																						AI2iScript_Door_Unjam},
		{"door_lock",			NULL,		NULL,			"locks a door",												"door_id:int",																						AI2iScript_Door_Lock},
		{"door_unlock",			NULL,		NULL,			"unlocks a door",											"door_id:int",																						AI2iScript_Door_Unlock},
		{"door_open",			NULL,		NULL,			"opens a door (may not stay open)",							"door_id:int",																						AI2iScript_Door_Open},
		{"door_close",			NULL,		NULL,			"closes a door (may not stay open)",						"door_id:int",																						AI2iScript_Door_Close},
#if TOOL_VERSION
		{"door_printall",		NULL,		NULL,			"prints all doors in the level to file",					"",																									AI2iScript_Door_DebugDump},
		{"door_printnearby",	NULL,		NULL,			"prints all nearby doors",									"",																									AI2iScript_Door_DebugDumpNearby},
#endif

		// consoles
		{"console_activate",	NULL,		NULL,			"activates a console",										"console_id:int",																					AI2iScript_Console_Activate},
		{"console_deactivate",	NULL,		NULL,			"deactivates a console",									"console_id:int",																					AI2iScript_Console_Deactivate},
		{"console_reset",		NULL,		NULL,			"resets a console to initial state",						"console_id:int",																					AI2iScript_Console_Reset},

		// turrets
		{"turret_activate",		NULL,		NULL,			"activates a turret",										"turret_id:int",																					AI2iScript_Turret_Activate},
		{"turret_deactivate",	NULL,		NULL,			"deactivates a turret",										"turret_id:int",																					AI2iScript_Turret_Deactivate},
		{"turret_reset",		NULL,		NULL,			"resets a turret to initial state",							"turret_id:int",																					AI2iScript_Turret_Reset},

		// laser triggers
		{"trig_activate",		NULL,		NULL,			"activates a trigger",										"trigger_id:int",																					AI2iScript_Trigger_Activate},
		{"trig_deactivate",		NULL,		NULL,			"deactivates a trigger",									"trigger_id:int",																					AI2iScript_Trigger_Deactivate},
		{"trig_reset",			NULL,		NULL,			"resets a trigger to non-triggered state",					"trigger_id:int",																					AI2iScript_Trigger_Reset},
		{"trig_hide",			NULL,		NULL,			"hides a trigger",											"trigger_id:int",																					AI2iScript_Trigger_Hide},
		{"trig_show",			NULL,		NULL,			"shows a trigger",											"trigger_id:int",																					AI2iScript_Trigger_Show},
		{"trig_speed",			NULL,		NULL,			"sets a triggers speed",									"trigger_id:int volume:float",																		AI2iScript_Trigger_Speed},

		// trigger volumes
		{"trigvolume_enable",	NULL,		NULL,			"enable or disable a trigger volume",						"name:string enable:int{0 | 1} [type:string{\"all\" | \"entry\" | \"inside\" | \"exit\"} | ]",		AI2iScript_TriggerVolume_Enable},
		{"trigvolume_setscript",NULL,		NULL,			"set the script to call from a trigger volume",				"name:string script:string type:string{\"entry\" | \"inside\" | \"exit\"}",							AI2iScript_TriggerVolume_SetScript},
		{"trigvolume_trigger",	NULL,		NULL,			"fire off a trigger volume",								"name:string type:string{\"entry\" | \"inside\" | \"exit\"} [ai_name:string | script_id:int | ]",	AI2iScript_TriggerVolume_Trigger},
		{"trigvolume_reset",	NULL,		NULL,			"reset a trigger volume to its level-load state",			"name:string",																						AI2iScript_TriggerVolume_Reset},
		{"trigvolume_corpse",	NULL,		NULL,			"kills all the corpses inside a trigger volume",			"trig_id:int",																						AI2iScript_TriggerVolume_DeleteCorpses},
		{"trigvolume_kill",		NULL,		NULL,			"kills all the characters inside a trigger volume",			"trig_id:int",																						AI2iScript_TriggerVolume_DeleteCharacters},
	
		// powerups
		{"weapon_spawn",		NULL,		NULL,			"creates a new weapon",										"weapontype:string flag:int",																		AI2iScript_WeaponSpawn},
		{"powerup_spawn",		NULL,		NULL,			"creates a new powerup",									"poweruptype:string flag:int",																		AI2iScript_PowerupSpawn},

		{"killed_griffen",		NULL,		NULL,			"sets if we killed griffen",								"murder:bool",																						AI2iScript_KilledGriffen},

		/*
		 * debugging commands
		 */

		 // status reporting
		{"ai2_report",			NULL,		NULL,			"tells an AI to report in",									"[ai_name:string | script_id:int | ]",																AI2iScript_Report},
		{"ai2_report_verbose",	NULL,		NULL,			"tells an AI to report in verbosely",						"[ai_name:string | script_id:int | ]",																AI2iScript_Report_Verbose},
		{"where",				NULL,		NULL,			"prints location of a character",							"[ai_name:string | script_id:int | ]",																AI2iScript_Where},
		{"who",					NULL,		NULL,			"prints AIs that are nearby",								"",																									AI2iScript_Who},

		// test commands
		{"ai2_comehere",		NULL,		NULL,			"tells an AI to come to the player",						"[ai_name:string | script_id:int | ]",																AI2iScript_Debug_ComeHere},
		{"ai2_lookatme",		NULL,		NULL,			"tells an AI to look at the player",						"[ai_name:string | script_id:int | ]",																AI2iScript_Debug_LookHere},
		{"ai2_followme",		NULL,		NULL,			"tells an AI to follow the player",							"[ai_name:string | script_id:int | ]",																AI2iScript_Debug_FollowMe},
		{"ai2_showmem",			NULL,		NULL,			"shows AI memory usage",									"",																									AI2iScript_ShowMem},

		// pathfinding
		{"ai2_findconnections",	NULL,		NULL,			"finds all BNV connections from the player's location",		"[distance:int | ]",																				AI2iScript_Debug_TraverseGraph},
		{"ai2_printbnvindex",	NULL,		NULL,			"prints the index of the player's BNV",						"",																									AI2iScript_Debug_PrintBNVIndex},
		{"ai2_chump",			NULL,		NULL,			"creates a chump",											"",																									AI2iScript_Debug_Chump},
		{"ai2_pathdebugsquare",	NULL,		NULL,			"selects a path square in the player's BNV for debugging",	"x:int y:int",																						AI2iScript_PathDebugSquare},

#if ENABLE_AI2_DEBUGGING_COMMANDS
		// technical debugging
		{"ai2_debug_makesound", NULL,		NULL,			"causes the player to make a sound (alerts nearby AIs)",	"[sound_type:string | ] [volume:float | ]",															AI2iScript_Debug_MakeSound},
#endif

		{NULL,					NULL,		NULL,			NULL,														NULL,																								NULL}

	};

	const AI2tVariableTable *current_variable;
	const AI2tVariableTable variable_table[] = {
		/*
		 * debugging information display
		 */

		// pathfinding
		{"ai2_showpaths",				SLcType_Bool,	"shows the path each AI is following",							&AI2gDrawPaths},
		{"ai2_showgrids",				SLcType_Bool,	"turns on pathfinding grid rendering",							&AI2gShowPathfindingGrids},
		{"ai2_showdynamicgrids",		SLcType_Bool,	"turns on pathfinding dynamic grid display",					&PHgRenderDynamicGrid},		
		{"ai2_showpathfindingerrors",	SLcType_Bool,	"enables visual display of pathfinding errors",					&AI2gShowPathfindingErrors},
		{"ai2_showconnections",			SLcType_Bool,	"turns on pathfinding connection rendering",					&AI2gDebugDisplayGraph},
		{"ai2_showactivationpaths",		SLcType_Bool,	"turns on inactive -> active pathfinding rendering",			&AI2gDebug_ShowActivationPaths},
		{"ai2_showastar",				SLcType_Bool,	"shows grid squares that are evaluated by the A* pathfinding",	&AI2gDebug_ShowAstarEvaluation},

		// combat
		{"ai2_showcombatranges",		SLcType_Bool,	"shows circles that represent the combat ranges of each AI",	&AI2gShowCombatRanges},
		{"ai2_showvision",				SLcType_Bool,	"shows each AI's vision cones (central and peripheral)",		&AI2gShowVisionCones},
		{"ai2_showprediction",			SLcType_Bool,	"shows prediction info for AI2 characters",						&AI2gDebug_ShowPrediction},
		{"ai2_showtargeting",			SLcType_Bool,	"shows targeting info for AI2 characters",						&AI2gDebug_ShowTargeting},
		{"ai2_showlasers",				SLcType_Bool,	"turns on laser sights for AI2 characters",						&AI2gDrawLaserSights},
		{"ai2_showlos",					SLcType_Bool,	"shows AI line-of-sight checking",								&AI2gDebug_ShowLOS},
		{"ai2_showprojectiles",			SLcType_Bool,	"shows AI projectile knowledge",								&AI2gShowProjectiles},
		{"ai2_showfiringspreads",		SLcType_Bool,	"shows AI firing spread knowledge",								&AI2gShowFiringSpreads},

		// melee
		{"ai2_showlocalmelee",			SLcType_Bool,	"shows local-environment melee awareness",						&AI2gDebug_ShowLocalMelee},
		{"ai2_showfights",				SLcType_Bool,	"shows fights in progress",										&AI2gShowFights},

		// misc
		{"ai2_shownames",				SLcType_Bool,	"draws the name of every AI above their head",					&AI2gShowNames},
		{"ai2_showhealth",				SLcType_Bool,	"draws a health bar for every AI above their head",				&AI2gShowHealth},
		{"ai2_showjoblocations",		SLcType_Bool,	"draws a green cross at each AI's job location",				&AI2gShowJobLocations},
		{"ai2_showlinetochar",			SLcType_Bool,	"draws a line from the player to every AI",						&AI2gShowLineToChar},
		{"ai2_showsounds",				SLcType_Bool,	"shows AI sounds as they are generated",						&AI2gShowSounds},
		{"ai2_printspawn",				SLcType_Bool,	"prints information about each AI spawn",						&AI2gDebugSpawning},

		/*
		 * debugging behavior control
		 */

		// knowledge
		{"ai2_blind",					SLcType_Bool,	"turns off the AI's visual sensing system",						&AI2gBlind},
		{"ai2_deaf",					SLcType_Bool,	"turns off the AI's sound sensing system",						&AI2gDeaf},
		{"ai2_ignore_player",			SLcType_Bool,	"makes all AI ignore the player",								&AI2gIgnorePlayer},
		{"ai2_chump_stop",				SLcType_Bool,	"stops the chump",												&AI2gChumpStop},
		{"ai2_stopignoring_count",		SLcType_Int32,	"set the number of events before the AI will stop ignoring",	&AI2gSound_StopIgnoring_Events},
		{"ai2_stopignoring_time",		SLcType_Int32,	"set the delay timer before the AI forgets about ignored events",&AI2gSound_StopIgnoring_Time},

		// melee
		{"ai2_melee_weightcorrection",	SLcType_Bool,	"weights down non-attack techniques so they are never more than attacks", &AI2gMeleeWeightCorrection},
		{"ai2_spacing_enable",			SLcType_Bool,	"master enable switch for spacing behavior",					&AI2gSpacingEnable},
		{"ai2_spacing_cookies",			SLcType_Int32,	"number of cookies per fight",									&AI2gSpacingMaxCookies},
		{"ai2_spacing_weights",			SLcType_Bool,	"enables position-sensitive weighting of spacing behaviors",	&AI2gSpacingWeights},
		{"ai2_barabbas_run",			SLcType_Bool,	"lets Barabbas run while carrying his gun",						&AI2gBarabbasRun},
		{"ai2_boss_battle",				SLcType_Bool,	"enables AI boss-battle target selection",						&AI2gBossBattle},

		/*
		 * technical debugging variables
		 */
#if DEBUG_SHOWINTERSECTIONS
		{"ai2_showintersections",		SLcType_Bool,	"debug AI's melee intersections",								&TRgDisplayIntersections},
#endif
#if ENABLE_AI2_DEBUGGING_COMMANDS
		{"ai2_debug_localmovement",		SLcType_Bool,	"debug local-movement code from player's position",				&AI2gDebug_LocalMovement},
		{"ai2_debug_localpathfinding",	SLcType_Bool,	"debug local-path code from player's position and facing",		&AI2gDebug_LocalPathfinding},
#if TOOL_VERSION
		{"ai2_debug_localmove_lines",	SLcType_Int32,	"number of lines to cast to debug local-movement code",			&AI2gDebugLocalPath_LineCount},
#endif
		{"ai2_debug_showsettingIDs",	SLcType_Bool,	"shows ID numbers for combat, melee and neutral settings",		&AI2gDebugShowSettingIDs},
#endif	

		{NULL,							SLcType_Void,	NULL,															NULL}
	};

	SLrScript_Command_Register_ReturnType("chr_is_player", "returns if this character is the player", "[ai_name:string | script_id:int]", SLcType_Bool, AI2iScript_IsPlayer);
	SLrScript_Command_Register_ReturnType("chr_has_lsi", "records that the character has the lsi", "[ai_name:string | script_id:int]", SLcType_Bool, AI2iScript_HasLSI);
	SLrScript_Command_Register_ReturnType("chr_has_empty_weapon", "returns if this character is holding a weapon that is empty", "[ai_name:string | script_id:int]", SLcType_Bool, AI2iScript_HasEmptyWeapon);
	SLrScript_Command_Register_ReturnType("did_kill_griffen", "returns if we did kill griffen", "", SLcType_Int32, AI2iScript_DidKillGriffen);
	SLrScript_Command_Register_ReturnType("trigvolume_count", "counts the number of people in a trigger volume", "trig_id:int", SLcType_Int32, AI2iScript_TriggerVolume_Count);
	SLrScript_Command_Register_ReturnType("difficulty", "returns the difficulty level", "", SLcType_Int32, AI2iScript_Difficulty);

	// register all scripting commands
	for (current_command = command_table; current_command->name != NULL; current_command++) {
		error = SLrScript_Command_Register_Void(current_command->name, current_command->description, current_command->parameters, current_command->function);
		UUmError_ReturnOnErrorMsg(error, "AI2rInstallConsoleVariables: failed to register command");

		if (current_command->name2 != NULL) {
			// second command name - for legacy scripts
			error = SLrScript_Command_Register_Void(current_command->name2, current_command->description, current_command->parameters, current_command->function);
			UUmError_ReturnOnErrorMsg(error, "AI2rInstallConsoleVariables: failed to register command");
		}

		if (current_command->name3 != NULL) {
			// third command name - for legacy scripts
			error = SLrScript_Command_Register_Void(current_command->name3, current_command->description, current_command->parameters, current_command->function);
			UUmError_ReturnOnErrorMsg(error, "AI2rInstallConsoleVariables: failed to register command");
		}
	}

	// register all scripting variables
	for (current_variable = variable_table; current_variable->name != NULL; current_variable++) {
		switch (current_variable->type) {
			case SLcType_Bool:
				error = SLrGlobalVariable_Register_Bool(current_variable->name, current_variable->description, (UUtBool *) current_variable->ptr);
			break;

			case SLcType_Int32:
				error = SLrGlobalVariable_Register_Int32(current_variable->name, current_variable->description, (UUtInt32 *) current_variable->ptr);
			break;

			case SLcType_Float:
				error = SLrGlobalVariable_Register_Float(current_variable->name, current_variable->description, (float *) current_variable->ptr);
			break;

			default:
				UUmAssert(!"AI2rInstallConsoleVariables: unknown variable type");
				error = UUcError_None;
			break;
		}
		UUmError_ReturnOnErrorMsg(error, "AI2rInstallConsoleVariables: failed to register variable");
	}

	// set up global variables
	UUrMemory_Clear(AI2gScript_TalkBufferInUse, sizeof(AI2gScript_TalkBufferInUse));

	return UUcError_None;
}