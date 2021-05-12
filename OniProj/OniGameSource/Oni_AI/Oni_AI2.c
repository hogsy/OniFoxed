/*
	FILE:	Oni_AI2.c
	
	AUTHOR:	Michael Evans, Chris Butcher
	
	CREATED: November 15, 1999
	
	PURPOSE: AI for characters in Oni
	
	Copyright (c) 1999

*/

#include "Oni_AI2.h"
#include "Oni_Character.h"
#include "Oni_AI_Setup.h"
#include "Oni_Level.h"
#include "Oni_Path.h"
#include "Oni_GameStatePrivate.h"
#include "BFW_ScriptLang.h"
#include "Oni_Character_Animation.h"
#include "Oni_Persistance.h"
#include "Oni_Script.h"

#define AI_DEBUG_DAZE			0

// ------------------------------------------------------------------------------------
// -- external globals

const char AI2cGoalName[AI2cGoal_Max][16]
 = {"none", "idle", "guard", "patrol", "teambattle", "combat", "pursuit", "alarm", "neutral", "panic"};

const UUtBool AI2cGoalIsCasual[AI2cGoal_Max]
 = {UUcTrue, UUcTrue, UUcTrue, UUcTrue, UUcFalse, UUcFalse, UUcFalse, UUcFalse, UUcTrue, UUcFalse};

const char AI2cVocalizationTypeName[AI2cVocalization_Max][16]
 = {"taunt", "alert", "startle", "checkbody", "pursue", "cower", "superpunch", "superkick", "super3", "super4"};

UUtBool AI2gLastManStanding = UUcFalse;
UUtBool AI2gUltraMode = UUcFalse;

// ------------------------------------------------------------------------------------
// -- internal globals

static UUtBool AI2gDebug_BreakOnUpdate = UUcFalse;

#if TOOL_VERSION
// for debugging name display
static UUtBool				AI2gDebugName_Initialized;
static UUtRect				AI2gDebugName_TextureBounds;
static TStTextContext		*AI2gDebugName_TextContext;
static M3tTextureMap		*AI2gDebugName_Texture;
static IMtPoint2D			AI2gDebugName_Dest;
static IMtPixel				AI2gDebugName_WhiteColor;
static UUtInt16				AI2gDebugName_TextureWidth;
static UUtInt16				AI2gDebugName_TextureHeight;
static float				AI2gDebugName_WidthRatio;
#endif

const UUtUns32 AI2cVocalizationPriority[AI2cVocalization_Max]
 = {ONcSpeechType_Call, ONcSpeechType_Say, ONcSpeechType_Yell, ONcSpeechType_Say, ONcSpeechType_Say, ONcSpeechType_Say,
	ONcSpeechType_Override, ONcSpeechType_Override, ONcSpeechType_Override, ONcSpeechType_Override};

// ------------------------------------------------------------------------------------
// -- internal structures

typedef struct AI2tSpawnData {
	UUtBool spawn_player;
	UUtBool spawn_all;
} AI2tSpawnData;

typedef struct AI2tSpawnCharacterData {
	char *name;
	UUtBool force;
} AI2tSpawnCharacterData;

// ------------------------------------------------------------------------------------
// -- internal function prototypes

// character creation and deletion
static UUtBool AI2iEnumCallback_StartCharacters(OBJtObject *inObject, UUtUns32 inUserData);
static UUtBool AI2iEnumCallback_Spawn(OBJtObject *inObject, UUtUns32 inUserData);
static UUtBool AI2iEnumCallback_SpawnPlayer(OBJtObject *inObject, UUtUns32 inUserData);

// debugging / temporary code
static void AI2rReportState(ONtCharacter *ioCharacter, char *inName, AI2tGoal inGoal, AI2tGoalState *inState,
							UUtBool inVerbose, AI2tReportFunction inFunction);

#if TOOL_VERSION
static UUtError AI2rDebugName_Initialize(void);
static void AI2rDebugName_Terminate(void);
#endif

// ------------------------------------------------------------------------------------
// -- major oni interface functions

// initialize the AI2 system
UUtError AI2rInitialize(void)
{
	UUtError error;

#if TOOL_VERSION
	AI2gDebugName_Initialized = UUcFalse;
#endif

	error = AIrScript_Initialize();
	UUmError_ReturnOnError(error);

	error = ASrInitialize();
	UUmError_ReturnOnError(error);

	error = AI2rError_Initialize();
	UUmError_ReturnOnError(error);

	error = AI2rLocalPath_Initialize();
	UUmError_ReturnOnError(error);

	error = AI2rFight_Initialize();
	UUmError_ReturnOnError(error);

	error = AI2rMelee_Initialize();
	UUmError_ReturnOnError(error);

#if TOOL_VERSION
	// TEMPORARY DEBUGGING - initialize the buffer for storing LOS points in
	UUrMemory_Clear(AI2gCombat_LOSPoints, AI2cCombat_LOSPointBufferSize * sizeof(M3tPoint3D));
	UUrMemory_Clear(AI2gCombat_LOSClear, AI2cCombat_LOSPointBufferSize * sizeof(UUtBool));
	AI2gCombat_LOSUsedPoints = 0;
	AI2gCombat_LOSNextPoint = 0;
#endif

	return UUcError_None;
}

// shut down the AI2 system
void AI2rTerminate(void)
{
#if TOOL_VERSION
	if (AI2gDebugName_Initialized) {
		AI2rDebugName_Terminate();
	}
#endif

	AI2rMelee_Terminate();

	AI2rLocalPath_Terminate();

	AI2rError_Terminate();

#if TOOL_VERSION
	PHrDebugGraph_Terminate();
#endif

	ASrTerminate();
}

// update all AI characters for a frame
void AI2rUpdateAI(void)
{
	UUtUns32 character_index;
	ONtCharacter *character, *player_character;
	ONtCharacter **living_character_list;
	UUtUns32 living_character_count;
	ONtInputState input;


	if (AI2gDebug_BreakOnUpdate) {
		AI2gDebug_BreakOnUpdate = UUcFalse;
		UUrEnterDebugger("Dropping into AI2rUpdateAI()...");
	}

	if (!AI2gEveryonePassive) {
		AI2rKnowledge_Update();
		AI2rKnowledge_ImmediatePost(UUcTrue);

		AI2rMelee_GlobalUpdate();
	}

	player_character = ONrGameState_GetPlayerCharacter();
	living_character_list = ONrGameState_LivingCharacterList_Get();
	living_character_count = ONrGameState_LivingCharacterList_Count();
	
	for (character_index = 0; character_index < living_character_count; character_index++)
	{
		character = living_character_list[character_index];
		if (((character->flags & ONcCharacterFlag_InUse) == 0) || (character->flags & ONcCharacterFlag_Dead)) {
			// the character has been killed or deleted by a script that was run since the start of this update
			continue;
		}

		if (character->charType == ONcChar_AI2) {
			// call any pending scripts now (rather than from inside the knowledge code, which
			// is where they're likely to be called from)
			if (character->ai2State.flags & AI2cFlag_RunAlarmScript) {
				character->ai2State.flags &= ~AI2cFlag_RunAlarmScript;
				if (character->scripts.alarm_name[0] != '\0') {
					if ((character->scripts.flags & AIcScriptTableFlag_AlarmCalled) == 0) {
						ONrScript_ExecuteSimple(character->scripts.alarm_name, character->player_name);
						character->scripts.flags |= AIcScriptTableFlag_AlarmCalled;
					}
				}
			}

			if (character->ai2State.flags & AI2cFlag_RunCombatScript) {
				character->ai2State.flags &= ~AI2cFlag_RunCombatScript;
				if (character->scripts.combat_name[0] != '\0') {
					if ((character->scripts.flags & AIcScriptTableFlag_CombatCalled) == 0) {
						ONrScript_ExecuteSimple(character->scripts.combat_name, character->player_name);
						character->scripts.flags |= AIcScriptTableFlag_CombatCalled;
					}
				}
			}

			if (character->ai2State.flags & AI2cFlag_RunOutOfAmmoScript) {
				character->ai2State.flags &= ~AI2cFlag_RunOutOfAmmoScript;
				if (character->scripts.outofammo_name[0] != '\0') {
					if ((character->scripts.flags & AIcScriptTableFlag_OutOfAmmoCalled) == 0) {
						ONrScript_ExecuteSimple(character->scripts.outofammo_name, character->player_name);
						character->scripts.flags |= AIcScriptTableFlag_OutOfAmmoCalled;
					}
				}
			}

			if (character->ai2State.flags & AI2cFlag_RunNoPathScript) {
				character->ai2State.flags &= ~AI2cFlag_RunNoPathScript;
				if (character->scripts.nopath_name[0] != '\0') {
					if ((character->scripts.flags & AIcScriptTableFlag_NoPathCalled) == 0) {
						ONrScript_ExecuteSimple(character->scripts.nopath_name, character->player_name);
						character->scripts.flags |= AIcScriptTableFlag_NoPathCalled;
					}
				}
			}

			if (character->ai2State.flags & AI2cFlag_RunNeutralScript) {
				UUmAssert(character->ai2State.neutralPendingScript != NULL);
				if (character->ai2State.neutralPendingScript != NULL) {
					ONrScript_ExecuteSimple(character->ai2State.neutralPendingScript, character->player_name);
					character->ai2State.neutralPendingScript = NULL;
				}
				character->ai2State.flags &= ~AI2cFlag_RunNeutralScript;
			}

			if (character->ai2State.startle_length > 0) {
				UUtBool startling;
				UUtUns16 startle_frame;
				float current_facing;

				startling = ONrCharacter_IsStartling(character, &startle_frame);
				if (startling) {
					// work out whereabouts we are facing at this frame of the startle anim
					current_facing = character->ai2State.startle_startfacing +
						startle_frame * (character->ai2State.startle_deltafacing / character->ai2State.startle_length);
					UUmTrig_Clip(current_facing);

					// just turn on the spot and play our animation, nothing more
					AI2rExecutor_AimStop(character);
					AI2rExecutor_Move(character, current_facing, AI2cMovementMode_Default, AI2cMovementDirection_Stopped, UUcTrue);
					AI2rExecutor_Update(character);
					continue;
				} else {
					// no longer startling!
					character->ai2State.startle_length = 0;
				}
			}

			if (character->ai2State.alertVocalizationTimer > 0) {
				character->ai2State.alertVocalizationTimer--;

				if (character->ai2State.alertVocalizationTimer == 0) {
					// our alert vocalization timer has run out without us actually
					// going into combat - say our alert sound.
					AI2rVocalize(character, AI2cVocalization_Alert, UUcFalse);
					character->ai2State.flags |= AI2cFlag_SaidAlertSound;
				}
			}				

			if (character->ai2State.daze_timer > 0) {
				ONtActiveCharacter *active_character = ONrGetActiveCharacter(character);

				if ((active_character != NULL) && (!ONrAnimState_IsFallen(active_character->curFromState) ||
												   !ONrAnimState_IsFallen(active_character->nextAnimState))) {
					// we are not lying on the ground, don't reduce our daze timer
#if AI_DEBUG_DAZE
					if (active_character != NULL) {
						COrConsole_Printf("%s: %d: not yet on ground (states %s -> %s)",
									character->player_name, ONrGameState_GetGameTime(), 
									ONrAnimStateToString(active_character->curFromState), 
									ONrAnimStateToString(active_character->nextAnimState));
					}
#endif
				} else {
					character->ai2State.daze_timer--;

#if AI_DEBUG_DAZE
					if (character->ai2State.daze_timer == 0) {
						COrConsole_Printf("%s: %d: no longer dazed", character->player_name, ONrGameState_GetGameTime());
					} else {
						COrConsole_Printf("%s: %d: dazed, %d", character->player_name, ONrGameState_GetGameTime(), character->ai2State.daze_timer);
					}
#endif
				}
			}

			if ((!AI2gEveryonePassive) && ((character->ai2State.flags & AI2cFlag_Passive) == 0)) {
				// if we have no goal, set up our default as idle or our job
				if (character->ai2State.currentGoal == AI2cGoal_None) {
					AI2rReturnToJob(character, UUcTrue, UUcFalse);
				}

				// check to see if we should start neutral-character interaction with player
				if ((character->ai2State.currentGoal != AI2cGoal_Neutral) &&
					(AI2rNeutral_CheckTrigger(character, player_character, UUcTrue))) {
					AI2rExitState(character);

					character->ai2State.currentGoal = AI2cGoal_Neutral;
					UUrMemory_Clear(&character->ai2State.currentStateBuffer.state.neutral, sizeof(AI2tNeutralState));
					character->ai2State.currentState = &(character->ai2State.currentStateBuffer);
					character->ai2State.currentState->begun = UUcFalse;

					AI2rEnterState(character);
				} else if (character->ai2State.neutralDisableTimer > 0) {
					character->ai2State.neutralDisableTimer--;
				}

				if ((character->ai2State.alarmStatus.action_marker != NULL) && (character->ai2State.currentGoal != AI2cGoal_Alarm)) {
					if (character->ai2State.alarmSettings.fight_timer == 0) {
#if DEBUG_VERBOSE_ALARM
						COrConsole_Printf("fight timer setting is zero, fighting indefinitely");
#endif
					} else {
						if (character->ai2State.alarmStatus.fight_timer > 0) {
							// we are currently fighting for a limited duration instead of running for an alarm
							character->ai2State.alarmStatus.fight_timer--;
						}

						if (character->ai2State.alarmStatus.fight_timer == 0) {
							// go back to running for the alarm
#if DEBUG_VERBOSE_ALARM
							COrConsole_Printf("fight timer is zero, returning to alarm");
#endif
							AI2rAlarm_Setup(character, NULL);
						} else {
#if DEBUG_VERBOSE_ALARM
							COrConsole_Printf("fight timer is %d, continuing to fight", character->ai2State.alarmStatus.fight_timer);
#endif
						}
					}
				}

				UUmAssert(character->ai2State.currentState != NULL);

				if (character->ai2State.currentState == &character->ai2State.jobState) {
					UUtBool store_location = UUcFalse;

					// we are performing our job... should we store this point as our
					// job location?
					switch(character->ai2State.jobGoal) {
						case AI2cGoal_Idle:
							// characters with idle as a job don't have a fixed location
							store_location = UUcFalse;
						break;

						case AI2cGoal_Patrol:
							// we should only store this location if we're actually walking our path right now
							store_location = AI2rPatrol_AtJobLocation(character, &character->ai2State.jobState.state.patrol);
						break;
					}

					if (store_location) {
						character->ai2State.has_job_location = UUcTrue;
						ONrCharacter_GetPathfindingLocation(character, &character->ai2State.job_location);
					}
				}

				switch(character->ai2State.currentGoal)
				{
					case AI2cGoal_Idle:
						AI2rIdle_Update(character);
					break;
					
					case AI2cGoal_Patrol:
						AI2rPatrol_Update(character);
					break;
					
					case AI2cGoal_Pursuit:
						AI2rPursuit_Update(character);
					break;
					
					case AI2cGoal_Alarm:
						AI2rAlarm_Update(character);
					break;
					
					case AI2cGoal_Combat:
						AI2rCombat_Update(character);
					break;

					case AI2cGoal_Neutral:
						AI2rNeutral_Update(character);
					break;

					case AI2cGoal_Panic:
						AI2rPanic_Update(character);
					break;
				}
			}
		}

		if (character->flags & ONcCharacterFlag_AIMovement) {
			// AI movement
			if (ONrCharacter_IsPlayingFilm(character)) {
				UUrMemory_Clear(&input, sizeof(input));
				ONrCharacter_UpdateInput(character, &input);
				AI2rExecutor_SetCurrentState(character);

			} else {
				AI2rPath_Update(character);
				AI2rMovement_Update(character);
			}
		}
	}

	if (!AI2gEveryonePassive) {
		AI2rKnowledge_ImmediatePost(UUcFalse);
		AI2rMelee_GlobalFinish();
	}
}	

// start the AI2 system for a particular level
UUtError AI2rLevelBegin(void)
{
	UUtError error;

	// reset global AI scripting control variables
	AI2gEveryonePassive = UUcFalse;
	AI2gBlind = UUcFalse;
	AI2gDeaf = UUcFalse;
	AI2gIgnorePlayer = UUcFalse;
	AI2gChumpStop = UUcFalse;
	AI2gBossBattle = UUcFalse;

	error = AI2rKnowledge_LevelBegin();
	UUmError_ReturnOnError(error);

	AI2rScript_ClearSkillEdit();

	AI2rError_ClearPathfindingErrors();

	AI2rStartAllCharacters(UUcTrue, UUcFalse);

	return UUcError_None;
}

#if TOOL_VERSION
// debugging only
extern UUtUns32 PHgDebugGraph_NumConnections;
#endif

// end the AI2 system for a particular level
void AI2rLevelEnd(void)
{
	// characters will be deleted by the character code
#if TOOL_VERSION
	PHgDebugGraph_NumConnections = 0;
#endif

	AI2rKnowledge_LevelEnd();
}


// ------------------------------------------------------------------------------------
// -- character creation and deletion

// create all characters
static UUtBool AI2iEnumCallback_StartCharacters(OBJtObject *inObject, UUtUns32 inUserData)
{
	OBJtOSD_Character *char_osd;
	AI2tSpawnData *spawn_data = (AI2tSpawnData *) inUserData;

	UUmAssert(inObject->object_type == OBJcType_Character);
	char_osd = (OBJtOSD_Character *) inObject->object_data;
	
	// reset 'spawned' flag for this level
	char_osd->flags &= ~OBJcCharFlags_Spawned;

	if ((!spawn_data->spawn_all) && (char_osd->flags & OBJcCharFlags_NotInitiallyPresent))
		return UUcTrue;

	if (spawn_data->spawn_player || ((char_osd->flags & OBJcCharFlags_Player) == 0)) {
		char_osd->flags |= OBJcCharFlags_Spawned;
		ONrGameState_NewCharacter(inObject, NULL, NULL, NULL);
	}

	// keep enumerating
	return UUcTrue;
}

// create a new character
static UUtBool AI2iEnumCallback_Spawn(OBJtObject *inObject, UUtUns32 inUserData)
{
	OBJtOSD_Character *char_osd;
	AI2tSpawnCharacterData *user_data = (AI2tSpawnCharacterData *) inUserData;

	UUmAssert(inObject->object_type == OBJcType_Character);
	char_osd = (OBJtOSD_Character *) inObject->object_data;
	
	if (strcmp(char_osd->character_name, user_data->name) == 0) {
		if ((!user_data->force) && (char_osd->flags & OBJcCharFlags_Spawned) && ((char_osd->flags & OBJcCharFlags_SpawnMultiple) == 0)) {
			// can't spawn this character multiple times
			return UUcTrue;
		} else {
			char_osd->flags |= OBJcCharFlags_Spawned;
		}

		ONrGameState_NewCharacter(inObject, NULL, NULL, NULL);
		return UUcFalse;
	}

	// keep looking
	return UUcTrue;
}

// recreate the player character
static UUtBool AI2iEnumCallback_SpawnPlayer(OBJtObject *inObject, UUtUns32 inUserData)
{
	OBJtOSD_Character *char_osd;
	ONtCharacter **charptr;
	UUtUns16 char_index;
	UUtError error;

	UUmAssert(inObject->object_type == OBJcType_Character);
	char_osd = (OBJtOSD_Character *) inObject->object_data;
	
	if ((char_osd->flags & OBJcCharFlags_Player) == 0) {
		return UUcTrue;
	}

	// spawn player
	char_osd->flags |= OBJcCharFlags_Spawned;
	charptr = (ONtCharacter **) inUserData;
	error = ONrGameState_NewCharacter(inObject, NULL, NULL, &char_index);
	if (error != UUcError_None) {
		*charptr = NULL;
		return UUcTrue;
	}

	*charptr = ONrGameState_GetCharacterList() + char_index;
	return UUcFalse;
}

// start all characters that should be present at the start of the level
UUtError AI2rStartAllCharacters(UUtBool inStartPlayer, UUtBool inOverride)
{
	AI2tSpawnData user_data;
	ONtCharacter *player;

	// we spawn the player first so that they are always character 0 (makes
	// debugging easier)
	if (inStartPlayer) {
		OBJrObjectType_EnumerateObjects(OBJcType_Character, AI2iEnumCallback_SpawnPlayer, (UUtUns32) &player);	
	}

	// enumerate all characters
	user_data.spawn_player = UUcFalse;
	user_data.spawn_all = inOverride;
	OBJrObjectType_EnumerateObjects(OBJcType_Character, AI2iEnumCallback_StartCharacters, (UUtUns32) &user_data);	

	return UUcError_None;
}

// start a particular character
UUtError AI2rSpawnCharacter(char *inName, UUtBool inForce)
{
	AI2tSpawnCharacterData user_data;

	UUmAssertReadPtr(inName, sizeof(char));

	// enumerate until we find this character
	user_data.name = inName;
	user_data.force = inForce;
	OBJrObjectType_EnumerateObjects(OBJcType_Character, AI2iEnumCallback_Spawn, (UUtUns32) &user_data);	

	return UUcError_None;
}

// recreate the player character
ONtCharacter *AI2rRecreatePlayer(void)
{
	ONtCharacter *player = NULL;

	// enumerate until we find the player
	OBJrObjectType_EnumerateObjects(OBJcType_Character, AI2iEnumCallback_SpawnPlayer, (UUtUns32) &player);	
	return player;
}

// delete all characters
void AI2rDeleteAllCharacters(UUtBool inDeletePlayer)
{
	ONtCharacter *character;
	UUtUns32 itr;

	// delete all characters that were generated from object nodes (_not_ those generated from a legacy script)
	for (itr = 0, character = ONgGameState->characters; itr < ONgGameState->numCharacters; itr++, character++) {
		if ((character->scriptID == UUcMaxUns16) && (inDeletePlayer || (character->charType != ONcChar_Player))) {
			// delete the character
			ONrGameState_DeleteCharacter(character);
		}
	}

	if (inDeletePlayer) {
		character = ONrGameState_GetPlayerCharacter();
		if (character != NULL) {
			ONrGameState_DeleteCharacter(character);
		}
	}
}

// find a character with a particular name - used by the scripting system
ONtCharacter *AI2rFindCharacter(const char *inCharacterName, UUtBool inExhaustive)
{
	ONtCharacter *current_chr, **character_list = ONrGameState_LivingCharacterList_Get();
	UUtUns32 character_list_count = ONrGameState_LivingCharacterList_Count();
	UUtUns32 itr;

	// look for living characters
	for(itr = 0; itr < character_list_count; itr++)
	{
		if (strcmp(character_list[itr]->player_name, inCharacterName) == 0)
		{
			return character_list[itr];
		}
	}

	if (inExhaustive) {
		// search the entire character array, not just all living characters
		character_list_count = ONrGameState_GetNumCharacters();
		current_chr = ONrGameState_GetCharacterList();

		for (itr = 0; itr < character_list_count; itr++, current_chr++) {
			if (current_chr->flags & ONcCharacterFlag_InUse) {
				if (UUmString_IsEqual(current_chr->player_name, inCharacterName)) {
					return current_chr;
				}
			}
		}
	}

	return NULL;
}

// set up values for a newly created character
UUtError AI2rInitializeCharacter(ONtCharacter *ioCharacter, const OBJtOSD_Character *inStartData,
								 const AItCharacterSetup *inSetup, const ONtFlag *inFlag, UUtBool inUseDefaultMelee)
{
	UUtUns32 itr;
	UUtError error;
	UUtBool melee_found, combat_found, path_found, non_combatant, omniscient;
	OBJtOSD_Combat combat_data;
	AI2tMeleeProfile *melee_profile;
	UUtUns16 neutral_id, melee_id;

	/*************************
	* Initializes a character's AI state for the level, according
	* to setup data
	*/
	UUmAssertReadPtr(ioCharacter, sizeof(*ioCharacter));

	if (inStartData != NULL) {
		/**** read data from a character object placed in the editor */

		// set up script names
		UUrString_Copy(ioCharacter->scripts.spawn_name,		inStartData->spawn_script,		SLcScript_MaxNameLength);
		UUrString_Copy(ioCharacter->scripts.combat_name,	inStartData->combat_script,		SLcScript_MaxNameLength);
		UUrString_Copy(ioCharacter->scripts.die_name,		inStartData->die_script,		SLcScript_MaxNameLength);
		UUrString_Copy(ioCharacter->scripts.alarm_name,		inStartData->alarm_script,		SLcScript_MaxNameLength);
		UUrString_Copy(ioCharacter->scripts.hurt_name,		inStartData->hurt_script,		SLcScript_MaxNameLength);
		UUrString_Copy(ioCharacter->scripts.defeated_name,	inStartData->defeated_script,	SLcScript_MaxNameLength);
		UUrString_Copy(ioCharacter->scripts.outofammo_name,	inStartData->outofammo_script,	SLcScript_MaxNameLength);
		UUrString_Copy(ioCharacter->scripts.nopath_name,	inStartData->nopath_script,		SLcScript_MaxNameLength);

		// set up special-case flags that apply to all characters
		if (inStartData->flags & OBJcCharFlags_Unkillable) {
			ioCharacter->flags |= ONcCharacterFlag_Unkillable;
		}
		if (inStartData->flags & OBJcCharFlags_InfiniteAmmo) {
			ioCharacter->flags |= ONcCharacterFlag_InfiniteAmmo;
		}

		if (inStartData->flags & OBJcCharFlags_Boss) {
			ioCharacter->flags |= ONcCharacterFlag_Boss;
		}

		if (inStartData->flags & OBJcCharFlags_NoAutoDrop) {
			ioCharacter->flags |= ONcCharacterFlag_NoAutoDrop;
		}

		if (inStartData->flags & OBJcCharFlags_Player) {
			// player
			ioCharacter->charType = ONcChar_Player;
			ioCharacter->blockFunction = NULL;
			ioCharacter->scriptID = 0;
			ioCharacter->flags |= ONcCharacterFlag_Boss;
		} else {
			// AI
			ioCharacter->charType = ONcChar_AI2;
			ioCharacter->blockFunction = AI2rBlockFunction;

			// this character cannot be controlled by the old script interface (the one that requires
			// ID numbers)... so set a nonsense value
			ioCharacter->scriptID = UUcMaxUns16;

			// get the character's combat settings
			combat_found = ONrLevel_Combat_ID_To_Combat(inStartData->combat_id, &combat_data);
			if (!combat_found) {
				AI2_ERROR(AI2cWarning, AI2cSubsystem_HighLevel, AI2cError_HighLevel_CombatSettings,
							ioCharacter, inStartData->combat_id, 0, 0, 0);
			}

			// get the character's melee profile
			if (inUseDefaultMelee) {
				melee_id = ioCharacter->characterClass->ai2_behavior.default_melee_id;
			} else {
				melee_id = inStartData->melee_id;
			}

			if (melee_id == (UUtUns16) -1) {
				melee_found = UUcFalse;
			} else {
				melee_found = ONrLevel_Melee_ID_To_Melee_Ptr(melee_id, &melee_profile);
				if ((ioCharacter->charType == ONcChar_AI2) && (!melee_found)) {
					AI2_ERROR(AI2cWarning, AI2cSubsystem_HighLevel, AI2cError_HighLevel_NoMeleeProfile,
								ioCharacter, melee_id, 0, 0, 0);
				}
			}

			// get the character's neutral behavior
			neutral_id = inStartData->neutral_id;
			non_combatant = ((inStartData->flags & OBJcCharFlags_NonCombatant) > 0);

			omniscient = ((inStartData->flags & OBJcCharFlags_Omniscient) > 0);
		}

	} else if (inSetup != NULL) {
		/**** read data from an excel spreadsheet entry stored as an AItCharacterSetup */

		// scripts
		ioCharacter->scripts = inSetup->scripts;
		ioCharacter->scriptID = inSetup->defaultScriptID;

		if (ioCharacter->scriptID == 0) {
			// player
			ioCharacter->charType = ONcChar_Player;
			ioCharacter->blockFunction = NULL;
			ioCharacter->flags |= ONcCharacterFlag_Boss;
		} else {
			// AI
			ioCharacter->charType = ONcChar_AI2;
			ioCharacter->blockFunction = AI2rBlockFunction;

			combat_found = UUcFalse;
			melee_found = UUcFalse;
			neutral_id = (UUtUns16) -1;
			non_combatant = UUcFalse;
			omniscient = UUcFalse;
		}
	}
	
	if (ioCharacter->charType != ONcChar_AI2)
		return UUcError_None;

	/**** set up the AI2 state */

	ioCharacter->flags |= ONcCharacterFlag_AIMovement;
	ioCharacter->ai2State.flags = AI2cFlag_InUse;
	if (AI2gEveryonePassive) {
		ioCharacter->ai2State.flags |= AI2cFlag_Passive;
	}
	if (non_combatant) {
		ioCharacter->ai2State.flags |= AI2cFlag_NonCombatant;
	}
	if (omniscient) {
		ioCharacter->ai2State.flags |= AI2cFlag_Omniscient;
	}
	AI2rExecutor_Initialize(ioCharacter);
	AI2rKnowledgeState_Initialize(&ioCharacter->ai2State.knowledgeState);		
	AI2rPath_Initialize(ioCharacter);

	/**** set up alert status */

	if (inStartData == NULL) {
		// set up a default alert status
		ioCharacter->ai2State.alertStatus		= AI2cAlertStatus_Lull;
		ioCharacter->ai2State.minimumAlert		= AI2cAlertStatus_Lull;
		ioCharacter->ai2State.jobAlert			= AI2cAlertStatus_Low;
		ioCharacter->ai2State.investigateAlert	= AI2cAlertStatus_Medium;
		ioCharacter->ai2State.alarmGroups		= 0;
	} else {
		// set up alert status
		ioCharacter->ai2State.alertStatus		= inStartData->initial_alert;
		ioCharacter->ai2State.minimumAlert		= inStartData->minimum_alert;
		ioCharacter->ai2State.jobAlert			= inStartData->jobstart_alert;
		ioCharacter->ai2State.investigateAlert	= inStartData->investigate_alert;
		ioCharacter->ai2State.alarmGroups		= inStartData->alarm_groups;
	}
	ioCharacter->ai2State.alertTimer = 0;
	ioCharacter->ai2State.alertVocalizationTimer = 0;
	ioCharacter->ai2State.startleTimer = 0;
	ioCharacter->ai2State.alertInitiallyRaisedTime = 0;
	ioCharacter->ai2State.already_done_daze = UUcFalse;

	/**** set up pursuit settings */

	if (inStartData == NULL) {
		// set up defaults
		ioCharacter->ai2State.pursuitSettings.strong_unseen = AI2cPursuitMode_Move;
		ioCharacter->ai2State.pursuitSettings.weak_unseen	= AI2cPursuitMode_Forget;
		ioCharacter->ai2State.pursuitSettings.strong_seen	= AI2cPursuitMode_Hunt;
		ioCharacter->ai2State.pursuitSettings.weak_seen		= AI2cPursuitMode_Look;
		ioCharacter->ai2State.pursuitSettings.pursue_lost	= AI2cPursuitLost_Return;
	} else {
		// set up alert status
		ioCharacter->ai2State.pursuitSettings.strong_unseen = inStartData->pursue_strong_unseen;
		ioCharacter->ai2State.pursuitSettings.weak_unseen	= inStartData->pursue_weak_unseen;
		ioCharacter->ai2State.pursuitSettings.strong_seen	= inStartData->pursue_strong_seen;
		ioCharacter->ai2State.pursuitSettings.weak_seen		= inStartData->pursue_weak_seen;
		ioCharacter->ai2State.pursuitSettings.pursue_lost	= inStartData->pursue_lost;
	}

	/**** set up the job */

	if (inStartData == NULL) {
		ioCharacter->ai2State.jobGoal = AI2cGoal_Idle;
	} else {
		ioCharacter->ai2State.jobGoal = (AI2tGoal)inStartData->job;

		switch(inStartData->job) {
			case AI2cGoal_Patrol:
				ioCharacter->ai2State.jobState.state.patrol.current_waypoint = 0;
				ioCharacter->ai2State.jobState.state.patrol.pause_timer = 0;

				path_found = ONrLevel_Path_ID_To_Path(inStartData->job_specific_id,
										&ioCharacter->ai2State.jobState.state.patrol.path);
				if (!path_found) {
					// no path to find!
					AI2_ERROR(AI2cWarning, AI2cSubsystem_HighLevel, AI2cError_HighLevel_NoPath,
							ioCharacter, inStartData->job_specific_id, 0, 0, 0);
					ioCharacter->ai2State.jobGoal = AI2cGoal_Idle;
				}
			break;

			case AI2cGoal_Idle:
				ioCharacter->ai2State.jobGoal = AI2cGoal_Idle;
			break;

			// none of the other states should ever be jobs!
			default:
				AI2_ERROR(AI2cWarning, AI2cSubsystem_HighLevel, AI2cError_HighLevel_ImproperJob,
							ioCharacter, inStartData->job, 0, 0, 0);
				ioCharacter->ai2State.jobGoal = AI2cGoal_Idle;
			break;
		}
	}
	ioCharacter->ai2State.jobState.begun = UUcFalse;
	ioCharacter->ai2State.has_job_location = UUcFalse;

	// character starts off with no current state - will be set into either idle or its job in the
	// main update loop. if it's passive then it might not be set in the main update loop just yet.
	ioCharacter->ai2State.currentGoal = AI2cGoal_None;
	ioCharacter->ai2State.currentState = NULL;

	/**** set up the combat settings */

	if (combat_found) {
		ioCharacter->ai2State.combatSettings.flags = combat_data.flags;
		ioCharacter->ai2State.combatSettings.medium_range = combat_data.medium_range;
		
		for (itr = 0; itr < AI2cCombatRange_Max; itr++) {
			ioCharacter->ai2State.combatSettings.behavior[itr] = (AI2tBehaviorType)combat_data.behavior[itr];
		}

		ioCharacter->ai2State.combatSettings.melee_when = (AI2tFightType)combat_data.melee_when;
		ioCharacter->ai2State.combatSettings.no_gun = (AI2tNoGunBehavior)combat_data.no_gun;
		ioCharacter->ai2State.combatSettings.short_range = combat_data.short_range;
		ioCharacter->ai2State.combatSettings.pursuit_distance = combat_data.pursuit_distance;
		ioCharacter->ai2State.combatSettings.panic_melee = combat_data.panic_melee;
		ioCharacter->ai2State.combatSettings.panic_gunfire = combat_data.panic_gunfire;
		ioCharacter->ai2State.combatSettings.panic_sight = combat_data.panic_sight;
		ioCharacter->ai2State.combatSettings.panic_hurt = combat_data.panic_hurt;

		ioCharacter->ai2State.alarmSettings.find_distance = combat_data.alarm_find_distance;
		ioCharacter->ai2State.alarmSettings.ignore_enemy_dist = combat_data.alarm_ignore_enemy_dist;
		ioCharacter->ai2State.alarmSettings.chase_enemy_dist = combat_data.alarm_chase_enemy_dist;
		ioCharacter->ai2State.alarmSettings.damage_threshold = combat_data.alarm_damage_threshold;
		ioCharacter->ai2State.alarmSettings.fight_timer = combat_data.alarm_fight_timer;
	} else {
		// set up default combat settings
		ioCharacter->ai2State.combatSettings.flags = 0;
		ioCharacter->ai2State.combatSettings.medium_range = AI2cCombatSettings_DefaultMediumRange;
		for (itr = 0; itr < AI2cCombatRange_Max; itr++) {
			ioCharacter->ai2State.combatSettings.behavior[itr] = AI2cCombatSettings_DefaultBehavior;
		}
		ioCharacter->ai2State.combatSettings.melee_when = AI2cCombatSettings_DefaultMeleeWhen;
		ioCharacter->ai2State.combatSettings.no_gun = AI2cCombatSettings_DefaultNoGun;
		ioCharacter->ai2State.combatSettings.short_range = AI2cCombatSettings_DefaultShortRange;
		ioCharacter->ai2State.combatSettings.pursuit_distance = AI2cCombatSettings_DefaultPursuitDistance;
		ioCharacter->ai2State.combatSettings.panic_melee = AI2cPanic_DefaultMelee;
		ioCharacter->ai2State.combatSettings.panic_gunfire = AI2cPanic_DefaultGunfire;
		ioCharacter->ai2State.combatSettings.panic_sight = AI2cPanic_DefaultSight;
		ioCharacter->ai2State.combatSettings.panic_hurt = AI2cPanic_DefaultHurt;

		ioCharacter->ai2State.alarmSettings.find_distance = AI2cAlarmSettings_DefaultFindDistance;
		ioCharacter->ai2State.alarmSettings.ignore_enemy_dist = AI2cAlarmSettings_DefaultIgnoreEnemy;
		ioCharacter->ai2State.alarmSettings.chase_enemy_dist = AI2cAlarmSettings_DefaultChaseEnemy;
		ioCharacter->ai2State.alarmSettings.damage_threshold = AI2cAlarmSettings_DefaultDamageThreshold;
		ioCharacter->ai2State.alarmSettings.fight_timer = AI2cAlarmSettings_DefaultFightTimer;
	}
	ioCharacter->ai2State.regenerated_amount = 0;

	/**** set up the melee profile */

	if (melee_found) {
		ioCharacter->ai2State.meleeProfile = melee_profile;

		error = AI2rMelee_AddReference(melee_profile, ioCharacter);
		if (error != UUcError_None) {
			// couldn't set up the melee profile for use, don't use it
			ioCharacter->ai2State.meleeProfile = NULL;
		} else {
			// check that this melee profile is for the right variant
			UUmAssertReadPtr(melee_profile->char_class, sizeof(ONtCharacterClass));
			if (!UUmString_IsEqual(ioCharacter->characterClass->variant->name, melee_profile->char_class->variant->name)) {
				UUmAssertReadPtr(inStartData, sizeof(*inStartData));

				AI2_ERROR(AI2cWarning, AI2cSubsystem_HighLevel, AI2cError_HighLevel_MeleeVariant, ioCharacter,
					inStartData->melee_id, melee_profile, ioCharacter->characterClass, 0);
			}
		}
	} else {
		// this character has no melee profile
		ioCharacter->ai2State.meleeProfile = NULL;
	}
	UUrMemory_Clear(ioCharacter->ai2State.meleeDelayTimers, sizeof(ioCharacter->ai2State.meleeDelayTimers));

	/**** set up the neutral behavior */

	AI2rNeutral_ChangeBehavior(ioCharacter, neutral_id, UUcTrue);
	ioCharacter->ai2State.neutralPendingScript = NULL;
	ioCharacter->ai2State.neutralDisableTimer = 0;

	/**** set up the alarm status */

	ioCharacter->ai2State.alarmStatus.action_marker = NULL;
	ioCharacter->ai2State.alarmStatus.last_action_marker = NULL;
	ioCharacter->ai2State.alarmStatus.fight_timer = 0;

	return UUcError_None;
}


// ------------------------------------------------------------------------------------
// -- high-level job manager

// the character needs to enter its current state
void AI2rEnterState(ONtCharacter *ioCharacter)
{
	if (ioCharacter->ai2State.flags & AI2cFlag_StateTransition) {
		UUmAssert(!"AI2rEnterState called while in state transition");
		UUrDebuggerMessage("*** AI ERROR: AI %s tried to enter-state while in the middle of a state transition", ioCharacter->player_name);
	}
	if (ioCharacter->ai2State.currentState == NULL)
		return;

	ioCharacter->ai2State.flags |= AI2cFlag_StateTransition;

	if (!AI2rIsCasual(ioCharacter)) {
		ONrCharacter_MakeActive(ioCharacter, UUcTrue);
	}

	switch(ioCharacter->ai2State.currentGoal)
	{
		case AI2cGoal_None:		// should never happen
		case AI2cGoal_Idle:
			AI2rIdle_Enter(ioCharacter);
		break;

		case AI2cGoal_Patrol:
			AI2rPatrol_Enter(ioCharacter);
		break;

		case AI2cGoal_Pursuit:
			AI2rPursuit_Enter(ioCharacter);
		break;

		case AI2cGoal_Alarm:
			AI2rAlarm_Enter(ioCharacter);
		break;

		case AI2cGoal_Combat:
			AI2rCombat_Enter(ioCharacter);
		break;

		case AI2cGoal_Neutral:
			AI2rNeutral_Enter(ioCharacter);
		break;

		case AI2cGoal_Panic:
			AI2rPanic_Enter(ioCharacter);
		break;

		default:
			AI2_ERROR(AI2cBug, AI2cSubsystem_HighLevel, AI2cError_HighLevel_UnknownJob,
					ioCharacter, ioCharacter->ai2State.currentGoal, 0, 0, 0);

			ioCharacter->ai2State.currentGoal = AI2cGoal_Idle;
			ioCharacter->ai2State.currentState = &ioCharacter->ai2State.currentStateBuffer;
			AI2rIdle_Enter(ioCharacter);
		break;
	}

	ioCharacter->ai2State.flags &= ~AI2cFlag_StateTransition;
	ioCharacter->ai2State.currentState->begun = UUcTrue;
}

// the character needs to exit its current state
void AI2rExitState(ONtCharacter *ioCharacter)
{
//	UUrDebuggerMessage("*** exiting state %s\n", AI2cGoalName[ioCharacter->ai2State.currentGoal]);
	
	if (ioCharacter->ai2State.flags & AI2cFlag_StateTransition) {
		UUmAssert(!"AI2rExitState called while in state transition");
		UUrDebuggerMessage("*** AI ERROR: AI %s tried to exit-state while in the middle of a state transition", ioCharacter->player_name);
	}
	if ((ioCharacter->ai2State.currentGoal == AI2cGoal_None) ||
		(ioCharacter->ai2State.currentState == NULL) ||
		(!ioCharacter->ai2State.currentState->begun))
		return;

	ioCharacter->ai2State.flags |= AI2cFlag_StateTransition;

	switch(ioCharacter->ai2State.currentGoal)
	{
		case AI2cGoal_Idle:
			AI2rIdle_Exit(ioCharacter);
		break;

		case AI2cGoal_Patrol:
			AI2rPatrol_Exit(ioCharacter);
		break;

		case AI2cGoal_Pursuit:
			AI2rPursuit_Exit(ioCharacter);
		break;

		case AI2cGoal_Alarm:
			AI2rAlarm_Exit(ioCharacter);
		break;

		case AI2cGoal_Combat:
			AI2rCombat_Exit(ioCharacter);
		break;

		case AI2cGoal_Neutral:
			AI2rNeutral_Exit(ioCharacter);
		break;

		case AI2cGoal_Panic:
			AI2rPanic_Exit(ioCharacter);
		break;

		default:
			AI2_ERROR(AI2cBug, AI2cSubsystem_HighLevel, AI2cError_HighLevel_UnknownJob,
					ioCharacter, ioCharacter->ai2State.currentGoal, 0, 0, 0);
		break;
	}

	ioCharacter->ai2State.flags &= ~AI2cFlag_StateTransition;
	ioCharacter->ai2State.currentGoal = AI2cGoal_None;
	ioCharacter->ai2State.currentState = NULL;
}

// the character needs to start its 'job' state - newly created, or has finished other activity
void AI2rReturnToJob(ONtCharacter *ioCharacter, UUtBool inSetIdle, UUtBool inEvenIfAlerted)
{
	UUtBool is_idle =(ioCharacter->ai2State.alertStatus < ioCharacter->ai2State.jobAlert);

	if (ioCharacter->ai2State.currentState == &ioCharacter->ai2State.jobState) {
		// already in job state
		return;
	}

	if (is_idle && (!inSetIdle) && (ioCharacter->ai2State.currentGoal != AI2cGoal_None)) {
		// don't overwrite our current state needlessly with idle
		return;
	}

	if ((!AI2cGoalIsCasual[ioCharacter->ai2State.currentGoal]) && (!inEvenIfAlerted)) {
		// we are alerted - don't return to our job
		return;
	}

	// exit our current state
	AI2rExitState(ioCharacter);

	if (is_idle) {
		// idle, since we are not yet at our job alert status
		ioCharacter->ai2State.currentGoal = AI2cGoal_Idle;
		ioCharacter->ai2State.currentState = &ioCharacter->ai2State.currentStateBuffer;
		ioCharacter->ai2State.currentState->begun = UUcFalse;
	} else {
		// set up our job - don't overwrite previous value of begun!
		ioCharacter->ai2State.currentGoal = ioCharacter->ai2State.jobGoal;
		ioCharacter->ai2State.currentState = &ioCharacter->ai2State.jobState;
	}

	// start this new state
	AI2rEnterState(ioCharacter);
}

// is a character hurrying?
UUtBool AI2rIsHurrying(ONtCharacter *inCharacter)
{
	AI2tGoal current_goal;

	if (inCharacter->charType != ONcChar_AI2) {
		return UUcTrue;
	}

	current_goal = inCharacter->ai2State.currentGoal;
	UUmAssert((current_goal >= AI2cGoal_None) && (current_goal < AI2cGoal_Max));

	if (AI2cGoalIsCasual[current_goal])
		return UUcFalse;

	if (current_goal == AI2cGoal_Pursuit) {
		return AI2rPursuit_IsHurrying(inCharacter, &inCharacter->ai2State.currentState->state.pursuit);
	} else {
		return UUcTrue;
	}
}

UUtBool AI2rIsCasual(ONtCharacter *ioCharacter)
{
	UUmAssert(ioCharacter->charType == ONcChar_AI2);

	if (!AI2cGoalIsCasual[ioCharacter->ai2State.currentGoal]) {
		// this is not a casual state
		return UUcFalse;
	}

	if ((ioCharacter->ai2State.currentGoal == AI2cGoal_Patrol) &&
		(ioCharacter->ai2State.currentState->state.patrol.locked_in)) {
		// this is not a casual patrol
		return UUcFalse;
	}

	// this is a casual (i.e. unalerted) state
	return UUcTrue;
}

UUtBool AI2rCurrentlyActive(ONtCharacter *ioCharacter)
{
	if ((!AI2cGoalIsCasual[ioCharacter->ai2State.currentGoal]) && (ioCharacter->ai2State.currentGoal != AI2cGoal_Panic)) {
		// our current goal is not casual, we shouldn't be deactivated
		//
		// note that panic is not a casual goal for the purposes of turning etc, but we can be made inactive while
		// panicking
		return UUcTrue;
	}

	if ((ioCharacter->ai2State.currentGoal == AI2cGoal_Neutral) || (ioCharacter->neutral_interaction_char != NULL)) {
		return UUcTrue;
	}

	if (ioCharacter->ai2State.currentGoal == AI2cGoal_Patrol) {
		if (ioCharacter->ai2State.currentState->state.patrol.run_targeting) {
			// we are shooting at something, don't go inactive
			return UUcTrue;

/*		// CB: since we now have tools (chr_lock_active and chr_unlock_active) to manage AI activity
		// manually for particularly stubborn patrol paths, I'm going to remove this general clause
		// because it is causing large numbers of AIs to be active when they shouldn't be on a number
		// of different levels - 11 october, 2000

		} else if (ioCharacter->ai2State.currentState->state.patrol.locked_in) {
			// we are locked onto patrol, don't fall off by going inactive
			return UUcTrue;*/
		}
	}

	return UUcFalse;
}

// ------------------------------------------------------------------------------------
// -- miscellaneous functions

// set all characters to be passive
void AI2rAllPassive(UUtBool inPassive)
{
	ONtCharacter **charptr;
	UUtUns32 num_chars, itr;

	AI2gEveryonePassive = inPassive;

	charptr = ONrGameState_LivingCharacterList_Get();
	num_chars = ONrGameState_LivingCharacterList_Count();

	for (itr = 0; itr < num_chars; itr++, charptr++) {
		AI2rPassive(*charptr, inPassive);
	}
}

// set a character to be passive
void AI2rPassive(ONtCharacter *ioCharacter, UUtBool inPassive)
{
	if (!inPassive) {
		// the character is returning out of scripting control, reset its last active animation time
		// so that everyone doesn't play idle anims at the end of a long scripting sequence
		ioCharacter->lastActiveAnimationTime = ONrGameState_GetGameTime();
	}
	
	if (ioCharacter->charType != ONcChar_AI2)
		return;

	if (inPassive) {
		if (ioCharacter->ai2State.flags & AI2cFlag_Passive)
			return;

		ioCharacter->ai2State.flags |= AI2cFlag_Passive;

		AI2rExitState(ioCharacter);

		AI2rExecutor_ClearAttackOverride(ioCharacter);
		AI2rPath_Halt(ioCharacter);
	} else {
		if ((ioCharacter->ai2State.flags & AI2cFlag_Passive) == 0)
			return;

		ioCharacter->ai2State.flags &= ~AI2cFlag_Passive;

		AI2rEnterState(ioCharacter);
	}
}

// will a character be hostile towards a given team?
UUtBool AI2rCharacter_PotentiallyHostile(ONtCharacter *inCharacter, ONtCharacter *inTarget, UUtBool inCheckPreviousKnowledge)
{
	UUtUns32 team_hostile_status;
	AI2tKnowledgeEntry *entry = NULL;

	if ((inCharacter == NULL) || (inTarget == NULL)) {
		return UUcFalse;
	}

	if ((AI2gEveryonePassive) || (inCharacter->ai2State.flags & AI2cFlag_Passive)) {
		return UUcFalse;
	}

	if (inCharacter == inTarget) {
		return UUcFalse;
	}

	team_hostile_status = AI2rTeam_FriendOrFoe(inCharacter->teamNumber, inTarget->teamNumber);
	if (team_hostile_status == AI2cTeam_Friend) {
		return UUcFalse;
	} else if (team_hostile_status == AI2cTeam_Hostile) {
		return UUcTrue;
	}

	if (inCharacter->charType == ONcChar_AI2) {
		// look to see if this character wants to attack the target
		if (inCharacter->ai2State.flags & AI2cFlag_NonCombatant) {
			return UUcFalse;
		}

		if (inCheckPreviousKnowledge) {
			entry = AI2rKnowledge_FindEntry(inCharacter, &inCharacter->ai2State.knowledgeState, inTarget);
			if ((entry != NULL) && ((entry->has_been_hostile) || (entry->priority >= AI2cContactPriority_Hostile_Threat))) {
				return UUcTrue;
			}		
		}
	}

	if (inCheckPreviousKnowledge) {
		if (inTarget->charType == ONcChar_AI2) {
			// the character is supposedly neutral towards us (the target)... work out whether they have in fact hurt us
			entry = AI2rKnowledge_FindEntry(inTarget, &inTarget->ai2State.knowledgeState, inCharacter);
		}

		if ((entry != NULL) && ((entry->has_been_hostile) || (entry->priority >= AI2cContactPriority_Hostile_Threat))) {
			return UUcTrue;
		}
	}
	
	return UUcFalse;
}

// team friendship rules
UUtUns32 AI2rTeam_FriendOrFoe(UUtUns16 inTeam, UUtUns16 inTargetTeam)
{
	if (AI2gLastManStanding) {
		// CB: this is a cheat code
		return AI2cTeam_Hostile;
	}

	if ((inTeam == inTargetTeam) || (inTeam == OBJcCharacter_TeamSwitzerland) || (inTargetTeam == OBJcCharacter_TeamSwitzerland)) {
		return AI2cTeam_Friend;
	}

	switch(inTeam) {
		case OBJcCharacter_TeamKonoko:
			if (inTargetTeam == OBJcCharacter_TeamTCTF) {
				return AI2cTeam_Friend;

			} else if (inTargetTeam == OBJcCharacter_TeamSyndicate) {
				return AI2cTeam_Hostile;

			} else {
				return AI2cTeam_Neutral;
			}

		case OBJcCharacter_TeamTCTF:
			if (inTargetTeam == OBJcCharacter_TeamKonoko) {
				return AI2cTeam_Friend;

			} else if ((inTargetTeam == OBJcCharacter_TeamSyndicate) || (inTargetTeam == OBJcCharacter_TeamRogueKonoko)) {
				return AI2cTeam_Hostile;

			} else {
				return AI2cTeam_Neutral;
			}

		case OBJcCharacter_TeamSyndicate:
			if (inTargetTeam == OBJcCharacter_TeamSyndicateAccessory) {
				return AI2cTeam_Friend;

			} else {
				return AI2cTeam_Hostile;
			}

		case OBJcCharacter_TeamSyndicateAccessory:
			if (inTargetTeam == OBJcCharacter_TeamSyndicate) {
				return AI2cTeam_Friend;

			} else {
				return AI2cTeam_Neutral;
			}

		case OBJcCharacter_TeamNeutral:
			return AI2cTeam_Neutral;

		case OBJcCharacter_TeamSecurityGuard:
			if ((inTargetTeam == OBJcCharacter_TeamSyndicate) || (inTargetTeam == OBJcCharacter_TeamRogueKonoko)) {
				return AI2cTeam_Hostile;

			} else {
				return AI2cTeam_Neutral;
			}

		case OBJcCharacter_TeamRogueKonoko:
			if ((inTargetTeam == OBJcCharacter_TeamSyndicate) || (inTargetTeam == OBJcCharacter_TeamTCTF)) {
				return AI2cTeam_Hostile;

			} else {
				return AI2cTeam_Neutral;
			}

		default:
			return AI2cTeam_Neutral;
	}
}

// a character has been killed
void AI2rNotify_Death(ONtCharacter *ioCharacter, ONtCharacter *inKiller)
{
	if ((inKiller != NULL) && (inKiller->charType == ONcChar_AI2) && (inKiller->ai2State.currentGoal == AI2cGoal_Combat) &&
		(inKiller->ai2State.currentState->state.combat.maneuver.primary_movement == AI2cPrimaryMovement_Melee)) {
		// we just killed someone in melee and are allowed to taunt them
		inKiller->ai2State.currentState->state.combat.dead_taunt_enable = UUcTrue;
	}

	AI2rKnowledge_CharacterDeath(ioCharacter);

	if (ioCharacter->melee_fight != NULL) {
		AI2rFight_StopFight(ioCharacter);
	}

	if (ioCharacter->neutral_interaction_char != NULL) {
		AI2rNeutral_Stop(ioCharacter, UUcTrue, UUcTrue, UUcTrue, UUcTrue);
	}
}

// delete AI2 internal state for a character
void AI2rDeleteState(ONtCharacter *ioCharacter)
{
	UUmAssert(ioCharacter->charType == ONcChar_AI2);

	if ((ioCharacter->ai2State.flags & AI2cFlag_InUse) == 0) {
//		COrConsole_Printf("TEMPKNOWLEDGEDEBUG: can't delete ai2state not in use");
		return;
	} else {
//		COrConsole_Printf("TEMPKNOWLEDGEDEBUG: deleting ai2state");
	}

	if (ioCharacter->neutral_interaction_char) {
		// abort neutral interaction, character is being deleted. don't call AI2rNeutral_Stop because this requires
		// that the characters are both still valid.
		ioCharacter->flags								&= ~ONcCharacterFlag_NeutralUninterruptable;
		ioCharacter->neutral_interaction_char->flags	&= ~ONcCharacterFlag_NeutralUninterruptable;

		ioCharacter->neutral_interaction_char->neutral_interaction_char = NULL;
		ioCharacter->neutral_interaction_char = NULL;
	}

	switch(ioCharacter->ai2State.currentGoal) {
		case AI2cGoal_Patrol:
			AI2rPatrol_NotifyDeath(ioCharacter);
		break;

		case AI2cGoal_Combat:
			AI2rCombat_NotifyDeath(ioCharacter);
		break;
	}

	// force a character to exit their current state, which will free up anything
	// allocated by their state
	AI2rExitState(ioCharacter);

	if (ioCharacter->ai2State.meleeProfile != NULL) {
		AI2rMelee_RemoveReference(ioCharacter->ai2State.meleeProfile, ioCharacter);
		ioCharacter->ai2State.meleeProfile = NULL;
	}

	AI2rKnowledge_Delete(ioCharacter);

	ioCharacter->ai2State.flags &= ~AI2cFlag_InUse;

	return;
}

// a character has been hurt
void AI2rNotify_Damage(ONtCharacter *ioCharacter, UUtUns32 inHitPoints, UUtUns32 inDamageOwner)
{
	WPtDamageOwnerType damage_type = WPrOwner_GetType(inDamageOwner);
	UUtBool melee_damage;
	ONtCharacter *attacker;

	// don't react if you're passive
	if ((AI2gEveryonePassive) || (ioCharacter->ai2State.flags & AI2cFlag_Passive)) {
		return;
	}

	// if we were going to say our alert sound, don't... we will be playing a hurt sound instead
	ioCharacter->ai2State.alertVocalizationTimer = 0;

	switch(damage_type) {
	case WPcDamageOwner_CharacterWeapon:
		AI2rStopDaze(ioCharacter);

	case WPcDamageOwner_CharacterMelee:
		melee_damage = (damage_type == WPcDamageOwner_CharacterMelee);
		attacker = WPrOwner_GetCharacter(inDamageOwner);

		if ((ioCharacter->ai2State.alarmStatus.action_marker != NULL) && (ioCharacter->ai2State.currentGoal != AI2cGoal_Alarm)) {
			// we are currently fighting instead of running for an alarm; reset our fight timer
			ioCharacter->ai2State.alarmStatus.fight_timer = ioCharacter->ai2State.alarmSettings.fight_timer;
		}

		// do we need to prosecute this character?
		AI2rKnowledge_HurtBy(ioCharacter, attacker, inHitPoints, melee_damage);
		break;

	case WPcDamageOwner_Turret:
	case WPcDamageOwner_Environmental:
		// FIXME: run away from this source of damage
		break;

	case WPcDamageOwner_None:
	case WPcDamageOwner_Falling:
	case WPcDamageOwner_ActOfGod:
	default:
		// don't do anything special to react to this kind of damage
		break;
	}
}

// the AI has been knocked down
void AI2rNotifyKnockdown(ONtCharacter *ioCharacter)
{
	UUmAssert(ioCharacter->charType == ONcChar_AI2);

	AI2rFight_ReleaseAttackCookie(ioCharacter);

	if (!ONrCharacter_IsFallen(ioCharacter)) {
		// we just got knocked down, reset our daze timer
		ioCharacter->ai2State.already_done_daze = UUcFalse;
	}

	if ((ONrCharacter_IsPlayingFilm(ioCharacter)) && (ioCharacter->ai2State.currentGoal == AI2cGoal_Patrol)) {
		// we are playing a film as part of our path; it's not going to happen, because
		// we've gotten knocked down and the keystroke sequence will no longer be
		// valid. stop the film and abort the path.
		ONrFilm_Stop(ioCharacter);
		AI2rReturnToJob(ioCharacter, UUcTrue, UUcTrue);

	} else if (ioCharacter->ai2State.currentGoal == AI2cGoal_Alarm) {
		// we may want to go after the person that just knocked us down
		AI2rAlarm_NotifyKnockdown(ioCharacter);
	}
}

// is an AI attacking someone?
ONtCharacter *AI2rGetCombatTarget(ONtCharacter *ioCharacter)
{
	if (ioCharacter->charType != ONcChar_AI2)
		return NULL;

	if (ioCharacter->ai2State.currentGoal != AI2cGoal_Combat)
		return NULL;

	return ioCharacter->ai2State.currentState->state.combat.target;
}

// a character has fired
void AI2rNotifyAIFired(ONtCharacter *ioCharacter)
{
	UUmAssert(ioCharacter->charType == ONcChar_AI2);
	if (ioCharacter->ai2State.currentGoal == AI2cGoal_Combat) {
		AI2rNotifyFireSuccess(&ioCharacter->ai2State.currentState->state.combat.targeting);

	} else if ((ioCharacter->ai2State.currentGoal == AI2cGoal_Patrol) &&
				(ioCharacter->ai2State.currentState->state.patrol.run_targeting)) {
		AI2rNotifyFireSuccess(&ioCharacter->ai2State.currentState->state.patrol.targeting);
	}
}

// console action is complete
void AI2rNotifyConsoleAction(ONtCharacter *ioCharacter, UUtBool inSuccess)
{
	UUmAssert(ioCharacter->charType == ONcChar_AI2);

	if (!inSuccess) {
		// we have been knocked out of our console action
		return;
	}

	// alarm is complete
	AI2rAlarm_Stop(ioCharacter, UUcTrue);
}

// should a character try to pathfind through another character?
UUtBool AI2rTryToPathfindThrough(ONtCharacter *ioCharacter, ONtCharacter *inObstruction)
{
	if (AI2rMovement_ShouldIgnoreObstruction(ioCharacter, inObstruction))
		return UUcTrue;

	// remaining conditions require actual AI state
	if (ioCharacter->charType != ONcChar_AI2)
		return UUcFalse;

	if ((ioCharacter->ai2State.currentGoal == AI2cGoal_Combat) &&
		(ioCharacter->ai2State.currentState->state.combat.target == inObstruction)) {
		// combat behaviors should take care of not wanting to get too close to target
		return UUcTrue;
	}

	if ((ioCharacter->ai2State.currentGoal == AI2cGoal_Neutral) &&
		(inObstruction->charType == ONcChar_Player)) {
		// we are running a neutral behavior and want to move up to character
		return UUcTrue;
	}

	return UUcFalse;
}

// an AI is playing a victim animation (and must stop its melee technique)
void AI2rStopMeleeTechnique(ONtCharacter *ioCharacter)
{
	UUmAssert(ioCharacter->charType == ONcChar_AI2);

	if (ioCharacter->ai2State.currentGoal != AI2cGoal_Combat)
		return;

	AI2rMeleeState_Clear(&ioCharacter->ai2State.currentState->state.combat.melee, UUcFalse);
	AI2rExecutor_ClearAttackOverride(ioCharacter);
}

// an attack has landed; if the defender was blocking, they should stop now
void AI2rNotifyAttackLanded(ONtCharacter *inAttacker, ONtCharacter *inDefender)
{
	AI2tMeleeState *melee_state;

	if ((inDefender->charType != ONcChar_AI2) || (inDefender->ai2State.currentGoal != AI2cGoal_Combat))
		return;
	
	melee_state = &inDefender->ai2State.currentState->state.combat.melee;

	if ((melee_state->target == inAttacker) && (melee_state->currently_blocking)) {

		if (inAttacker->animCounter != melee_state->current_react_anim_index) {
			// we thought that we were blocking another attack by this character.
			AI2_ERROR(AI2cWarning, AI2cSubsystem_Melee, AI2cError_Melee_BlockAnimIndex, inDefender,
						inAttacker, inAttacker->animCounter, melee_state->current_react_anim_index, 0);
		}

		melee_state->currently_blocking = UUcFalse;
	}
}

// a character has reached the action frame of its animation
void AI2rNotifyActionFrame(ONtCharacter *ioCharacter, UUtUns16 inAnimType)
{
	UUtBool handled;

	UUmAssert(ioCharacter->charType == ONcChar_AI2);

	if (ioCharacter->ai2State.currentGoal == AI2cGoal_Combat) {
		AI2rCombat_Behavior(ioCharacter, &ioCharacter->ai2State.currentState->state.combat,
							AI2cCombatMessage_ActionFrame, &handled, inAnimType, 0, 0);
	}
}

// a melee profile has been modified or deleted and all characters using it must be notified
void AI2rNotifyMeleeProfile(AI2tMeleeProfile *inProfile, UUtBool inDeleted)
{
	ONtCharacter **charptr;
	UUtUns32 num_chars, itr;

	charptr = ONrGameState_LivingCharacterList_Get();
	num_chars = ONrGameState_LivingCharacterList_Count();

	for (itr = 0; itr < num_chars; itr++, charptr++) {
		if ((*charptr)->charType != ONcChar_AI2)
			continue;

		if ((*charptr)->ai2State.meleeProfile != inProfile)
			continue;

		if (inDeleted) {
			// remove this character's melee profile
			(*charptr)->ai2State.meleeProfile = NULL;
		} else {
			// the melee profile has been changed; stop any techniques currently in progress
			AI2rStopMeleeTechnique(*charptr);
		}
	}

}

// we've just heard something - pause for a few seconds if we're not in an active goal state
UUtBool AI2rPause(ONtCharacter *ioCharacter, UUtUns32 inFrames)
{
	UUmAssert(ioCharacter->charType == ONcChar_AI2);

	switch(ioCharacter->ai2State.currentGoal) {
		case AI2cGoal_Patrol:
			AI2rPatrol_Pause(ioCharacter, &ioCharacter->ai2State.currentState->state.patrol, inFrames);
		return UUcTrue;

		case AI2cGoal_Pursuit:
			AI2rPursuit_Pause(ioCharacter, &ioCharacter->ai2State.currentState->state.pursuit, inFrames);
		return UUcTrue;

		case AI2cGoal_Idle:
			// FIXME: pause
		return UUcTrue;
	}

	return UUcFalse;
}

// is the AI trying to attack with the keys that it's currently pressing?
UUtBool AI2rTryingToAttack(ONtCharacter *ioCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);

	UUmAssert(ioCharacter->flags & ONcCharacterFlag_AIMovement);
	return ((active_character != NULL) && (active_character->executor_state.attack_override));
}

// callback to see if we choose to block a given attack
UUtBool AI2rBlockFunction(ONtCharacter *inCharacter)
{
	UUtUns32 random_val, num_attackers;
	UUtInt32 delta_block;
	float block_percentage;
	AI2tMeleeState *melee_state;
	AI2tMeleeProfile *profile = inCharacter->ai2State.meleeProfile;

	// must have a melee profile to block
	if (profile == NULL) {
		return UUcFalse;
	}

	// don't block if you're passive
	if ((AI2gEveryonePassive) || (inCharacter->ai2State.flags & AI2cFlag_Passive)) {
		return UUcFalse;
	}

	// don't block if you're a noncombatant or panicking
	if ((inCharacter->ai2State.flags & AI2cFlag_NonCombatant) || (inCharacter->ai2State.currentGoal == AI2cGoal_Panic)) {
		return UUcFalse;
	}

	// by default, use the 1v1 block percentage
	block_percentage = (float) profile->blockskill_percentage;

	if (inCharacter->ai2State.currentGoal == AI2cGoal_Combat) {
		melee_state = &inCharacter->ai2State.currentState->state.combat.melee;

		if (melee_state->fight_info.fight_owner != NULL) {
			num_attackers = melee_state->fight_info.fight_owner->num_attackers;
			
			if (num_attackers >= AI2cMelee_GroupNumAttackers) {
				// we are fighting in a large group
				block_percentage = (float) profile->blockgroup_percentage;
			} else {
				// we are fighting in a small group, interpolate our block skill
				delta_block = (UUtInt32) (profile->blockgroup_percentage - profile->blockskill_percentage);
				block_percentage = profile->blockskill_percentage +
							((float) delta_block * ((UUtInt32) num_attackers - 1)) / (AI2cMelee_GroupNumAttackers - 1);
			}
		}
	}

	// adjust blocking percentage by difficulty level
	block_percentage *= ONgGameSettings->blocking_multipliers[ONrPersist_GetDifficulty()];

	// we block some random fraction of the time
	random_val = (100 * ((UUtUns32) UUrRandom())) / UUcMaxUns16;
	return (random_val < block_percentage);
}

// is the AI trying to throw with its current attack?
UUtBool AI2rTryingToThrow(ONtCharacter *ioCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);

	UUmAssert(ioCharacter->flags & ONcCharacterFlag_AIMovement);
	if ((active_character == NULL) || (!active_character->executor_state.attack_override))
		return UUcFalse;

	return ONrAnimType_IsThrowSource(active_character->executor_state.attack_override_desiredanimtype);
}

// can we play an idle animation?
UUtBool AI2rEnableIdleAnimation(ONtCharacter *ioCharacter)
{
	// AIs don't play idle anims when in passive mode
	if (AI2gEveryonePassive)
		return UUcFalse;

	if ((ioCharacter->charType == ONcChar_AI2) && (ioCharacter->ai2State.flags & AI2cFlag_Passive))
		return UUcFalse;
	else
		return UUcTrue;
}

// if we are dazing, snap out of it
void AI2rStopDaze(ONtCharacter *ioCharacter)
{
	if (ioCharacter->charType != ONcChar_AI2) {
		return;
	}

	// if we are dazed, make sure we snap out of it and don't lie on the ground like a chump
	if (ioCharacter->ai2State.daze_timer > 0) {
		ioCharacter->ai2State.daze_timer = 1;
	}
}

// get the number of frames to stay knocked down
UUtUns16 AI2rStartDazeTimer(ONtCharacter *ioCharacter)
{
	UUtUns16 min, max;

	UUmAssert(ioCharacter->charType == ONcChar_AI2);

	min = ioCharacter->characterClass->ai2_behavior.dazed_minframes;
	max = ioCharacter->characterClass->ai2_behavior.dazed_maxframes;

	if (max <= min) {
		ioCharacter->ai2State.daze_timer = min;
	} else {
		ioCharacter->ai2State.daze_timer = UUmRandomRange(min, max - min);
	}

#if AI_DEBUG_DAZE
	COrConsole_Printf("%s: %d: daze random [%d, %d) for %d", ioCharacter->player_name, ONrGameState_GetGameTime(),
						min, max, ioCharacter->ai2State.daze_timer);
#endif

	return (UUtUns16) ioCharacter->ai2State.daze_timer;
}

// say something
UUtBool AI2rVocalize(ONtCharacter *ioCharacter, UUtUns32 inVocalizationType, UUtBool inForce)
{
	UUtUns16 random_val;

	UUmAssert((inVocalizationType >= 0) && (inVocalizationType < AI2cVocalization_Max));

	if ((ioCharacter->charType == ONcChar_AI2) && (inVocalizationType == AI2cVocalization_Taunt)) {
		// check to see this taunt is part of a check-body sound... if so then don't play it
		if ((ioCharacter->ai2State.currentGoal == AI2cGoal_Combat) && 
			(ioCharacter->ai2State.currentState->state.combat.dead_done_taunt)) {
			return UUcFalse;
		}
	}

	if (!inForce) {
		// check the random play chance
		random_val = (UUtUns16) ((((UUtUns32) UUrRandom()) * 100) >> 16);
		if (random_val >= ioCharacter->characterClass->ai2_behavior.vocalizations.sound_chance[inVocalizationType]) {
			return UUcFalse;
		}
	}

	if (ioCharacter->characterClass->ai2_behavior.vocalizations.sound_name[inVocalizationType][0] == '\0') {
		// we have no sound for this vocalization
		return UUcFalse;
	} else {
		ONtSpeech speech;

		speech.played = UUcFalse;
		speech.post_pause = 0;
		speech.pre_pause = 0;
		speech.sound_name = ioCharacter->characterClass->ai2_behavior.vocalizations.sound_name[inVocalizationType];
		speech.notify_string_inuse = NULL;
		speech.speech_type = inForce ? ONcSpeechType_Override : AI2cVocalizationPriority[inVocalizationType];

		return (ONrSpeech_Say(ioCharacter, &speech, UUcFalse) != ONcSpeechResult_Discarded);
	}
}

// do we have a neutral interaction available?
UUtBool AI2rTryNeutralInteraction(ONtCharacter *ioCharacter, UUtBool inActivate)
{
	ONtCharacter **charptr, *found_char;
	UUtUns32 num_chars, itr;
	AI2tNeutralState *neutral_state;
	UUtBool found_char_already_triggered;
	
	if (ioCharacter->neutral_interaction_char != NULL) {
		// we are already involved in a neutral interaction
		return UUcTrue;
	}

	charptr = ONrGameState_LivingCharacterList_Get();
	num_chars = ONrGameState_LivingCharacterList_Count();

	// check all AIs for possible neutral interactions
	found_char_already_triggered = UUcFalse;
	found_char = NULL;
	for (itr = 0; itr < num_chars; itr++, charptr++) {
		if ((*charptr)->charType != ONcChar_AI2)
			continue;

		if ((*charptr)->ai2State.currentGoal == AI2cGoal_Neutral) {
			// this character is already trying to run a neutral interaction
			neutral_state = &(*charptr)->ai2State.currentState->state.neutral;
			if (!AI2rNeutral_CheckActionAcceptance(*charptr, ioCharacter)) {
				// we can't accept this triggering
				continue;
			}

			if (!found_char_already_triggered) {
				found_char = *charptr;
				found_char_already_triggered = UUcTrue;
			}
		} else if (found_char == NULL) {
			if (AI2rNeutral_CheckTrigger(*charptr, ioCharacter, UUcFalse)) {
				// this character can run a neutral interaction
				found_char = *charptr;
			}
		}
	}

	if (found_char == NULL)
		return UUcFalse;

	if (inActivate) {
		if (!found_char_already_triggered) {
			// trigger the character to enter its neutral state
			AI2rExitState(found_char);

			found_char->ai2State.currentGoal = AI2cGoal_Neutral;
			UUrMemory_Clear(&found_char->ai2State.currentStateBuffer.state.neutral, sizeof(AI2tNeutralState));
			found_char->ai2State.currentState = &(found_char->ai2State.currentStateBuffer);
			found_char->ai2State.currentState->begun = UUcFalse;

			AI2rEnterState(found_char);
		}

		// the target is actually looking to enter a neutral interaction, set up to allow it
		UUmAssert(found_char->ai2State.currentGoal == AI2cGoal_Neutral);
		neutral_state = &found_char->ai2State.currentState->state.neutral;
		UUmAssert(neutral_state->target_character == ioCharacter);
		neutral_state->target_accepted = UUcTrue;
	}

	return UUcTrue;
}

// are we allowed to beat up on this character right now?
UUtBool AI2rAllowedToAttack(ONtCharacter *inCharacter, ONtCharacter *inTarget)
{
	ONtCharacter *player_character = ONrGameState_GetPlayerCharacter();

	if (inTarget == player_character) {
		// AIs are not allowed to attack the player directly during a cutscene
		// (which is defined as when the player loses input and the camera is
		// detached)
		if (!ONrGameState_IsInputEnabled() && (CArGetMode() != CAcMode_Follow)) {
			return UUcFalse;
		}
	}

	return UUcTrue;
}

// ------------------------------------------------------------------------------------
// -- debugging / temporary code

// tell one or all characters to come to the player
void AI2rDebug_ComeHere(ONtCharacter *ioCharacter)
{
	ONtCharacter *player_character = ONrGameState_GetPlayerCharacter();
	ONtCharacter **charptr;
	UUtUns32 num_chars, itr;
	M3tPoint3D destination;

	if (player_character == NULL)
		return;

	ONrCharacter_GetPathfindingLocation(player_character, &destination);

	if (ioCharacter == NULL) {
		charptr = ONrGameState_LivingCharacterList_Get();
		num_chars = ONrGameState_LivingCharacterList_Count();
	} else {
		charptr = &ioCharacter;
		num_chars = 1;
	}

	for (itr = 0; itr < num_chars; itr++, charptr++) {
		if ((*charptr)->charType != ONcChar_AI2)
			continue;

		if (player_character == NULL) {
			AI2rPath_Halt(*charptr);
		} else {
			AI2rPath_GoToPoint(*charptr, &destination, 0, UUcFalse, AI2cAngle_None, player_character);
		}
	}
}

// tell a character to look at the player - this overrides any other facing commands
void AI2rDebug_LookHere(ONtCharacter *ioCharacter)
{
	ONtCharacter *player_character = ONrGameState_GetPlayerCharacter();
	ONtCharacter **charptr;
	UUtUns32 num_chars, itr;

	if (ioCharacter == NULL) {
		charptr = ONrGameState_LivingCharacterList_Get();
		num_chars = ONrGameState_LivingCharacterList_Count();
	} else {
		charptr = &ioCharacter;
		num_chars = 1;
	}

	for (itr = 0; itr < num_chars; itr++, charptr++) {
		if ((*charptr)->charType != ONcChar_AI2)
			continue;

		if (player_character == NULL) {
			AI2rMovement_StopAiming(*charptr);
		} else {
			AI2rMovement_LookAtCharacter(*charptr, player_character);
		}
	}
}

// tell one or all characters to follow the player
void AI2rDebug_FollowMe(ONtCharacter *ioCharacter)
{
	ONtCharacter *player_character = ONrGameState_GetPlayerCharacter();
	ONtCharacter **charptr;
	UUtUns32 num_chars, itr;

	if (player_character == NULL)
		return;

	if (ioCharacter == NULL) {
		charptr = ONrGameState_LivingCharacterList_Get();
		num_chars = ONrGameState_LivingCharacterList_Count();
	} else {
		charptr = &ioCharacter;
		num_chars = 1;
	}

	for (itr = 0; itr < num_chars; itr++, charptr++) {
		if ((*charptr)->charType != ONcChar_AI2)
			continue;

		AI2rPath_FollowCharacter(*charptr, player_character);
	}
}

// smite one or many AIs
void AI2rSmite(ONtCharacter *inCharacter, UUtBool inSpareCharacter)
{
	ONtCharacter **charptr;
	UUtUns32 num_chars, itr;

	if ((inCharacter != NULL) && (!inSpareCharacter)) {
		if ((inCharacter->charType == ONcChar_AI2) &&
			(inCharacter->flags & ONcCharacterFlag_InUse) &&
			((inCharacter->flags & ONcCharacterFlag_Dead) == 0)) {
			// smite one character
			ONrCharacter_TakeDamage(inCharacter, inCharacter->hitPoints, 0.f, NULL, NULL,
									WPcDamageOwner_ActOfGod, ONcAnimType_None);
		}
		return;
	}

	charptr = ONrGameState_LivingCharacterList_Get();
	num_chars = ONrGameState_LivingCharacterList_Count();

	for (itr = 0; itr < num_chars; itr++, charptr++) {
		if (((*charptr)->charType != ONcChar_AI2) ||
			(((*charptr)->flags & ONcCharacterFlag_InUse) == 0) ||
			((*charptr)->flags & ONcCharacterFlag_Dead) ||
			((*charptr) == inCharacter))
			continue;

		// smite one character
		ONrCharacter_TakeDamage(*charptr, (*charptr)->hitPoints, 0.f, NULL, NULL, 
								WPcDamageOwner_ActOfGod, ONcAnimType_None);
	}
}

// make one or many AIs forget
void AI2rForget(ONtCharacter *inCharacter, ONtCharacter *forgetCharacter)
{
	ONtCharacter **charptr;
	UUtUns32 num_chars, itr;

	if (inCharacter == NULL) {
		charptr = ONrGameState_LivingCharacterList_Get();
		num_chars = ONrGameState_LivingCharacterList_Count();
	} else {
		charptr = &inCharacter;
		num_chars = 1;
	}

	for (itr = 0; itr < num_chars; itr++, charptr++) {
		if (((*charptr)->charType != ONcChar_AI2) ||
			(((*charptr)->flags & ONcCharacterFlag_InUse) == 0) ||
			((*charptr)->flags & ONcCharacterFlag_Dead))
			continue;

		// make this character forget
		AI2rKnowledgeState_Forget(*charptr, &(*charptr)->ai2State.knowledgeState, forgetCharacter);
	}
}

// tell a character to report in
void AI2rReport(ONtCharacter *ioCharacter, UUtBool inVerbose, AI2tReportFunction inFunction)
{
	ONtCharacter **charptr;
	UUtUns32 num_chars, itr;
	AI2tAlertStatus alert_status;
	char reportbuf[256], tempbuf[64];

	if (ioCharacter == NULL) {
		charptr = ONrGameState_LivingCharacterList_Get();
		num_chars = ONrGameState_LivingCharacterList_Count();
	} else {
		charptr = &ioCharacter;
		num_chars = 1;
	}

	for (itr = 0; itr < num_chars; itr++, charptr++) {
		if ((*charptr)->charType != ONcChar_AI2)
			continue;

		sprintf(reportbuf, "%s:", (*charptr)->player_name);
		inFunction(reportbuf);

		alert_status = (*charptr)->ai2State.alertStatus;
		if ((alert_status >= 0) && (alert_status < AI2cAlertStatus_Max)) {
			sprintf(reportbuf, "  alert=%s", AI2cAlertStatusName[alert_status]);
		} else {
			sprintf(reportbuf, "  <unknown alert status %d>", alert_status);
		}

		if ((*charptr)->ai2State.flags & AI2cFlag_Passive) {
			strcat(reportbuf, " (passive)");
		}

		sprintf(tempbuf, " (timer %d)", (*charptr)->ai2State.alertTimer);
		strcat(reportbuf, tempbuf);
		if ((*charptr)->ai2State.startleTimer) {
			sprintf(tempbuf, " (startle time %d)", (*charptr)->ai2State.startleTimer);
			strcat(reportbuf, tempbuf);
		}
		inFunction(reportbuf);

		AI2rReportState(*charptr, "job", (*charptr)->ai2State.jobGoal, &(*charptr)->ai2State.jobState, inVerbose, inFunction);
		AI2rReportState(*charptr, "current", (*charptr)->ai2State.currentGoal, (*charptr)->ai2State.currentState, inVerbose, inFunction);

		AI2rKnowledgeState_Report(*charptr, &(*charptr)->ai2State.knowledgeState, inVerbose, inFunction);

		AI2rPath_Report(*charptr, inVerbose, inFunction);
		AI2rMovement_Report(*charptr, inVerbose, inFunction);
		AI2rExecutor_Report(*charptr, inVerbose, inFunction);

		inFunction("");
	}
}

// report a character's state
static void AI2rReportState(ONtCharacter *ioCharacter, char *inName, AI2tGoal inGoal, AI2tGoalState *inState,
							UUtBool inVerbose, AI2tReportFunction inFunction)
{
	char reportbuf[256];

	switch(inGoal) {
		case AI2cGoal_None:
			sprintf(reportbuf, "  %s - none!", inName);
			inFunction(reportbuf);
		break;

		case AI2cGoal_Idle:
			sprintf(reportbuf, "  %s - idle", inName);
			inFunction(reportbuf);
		break;

		case AI2cGoal_Patrol:
		{
			AI2tPatrolState *patrol_state = &inState->state.patrol;

			sprintf(reportbuf, "  %s - patrol(%s, %d of %d)", inName,
					patrol_state->path.name, patrol_state->current_waypoint, patrol_state->path.num_waypoints);
			inFunction(reportbuf);

			AI2rPatrol_Report(ioCharacter, patrol_state, inVerbose, inFunction);
		}
		break;

		case AI2cGoal_Pursuit:
		{
			AI2tPursuitState *pursuit_state = &inState->state.pursuit;

			sprintf(reportbuf, "  %s - pursuing %s", inName, (pursuit_state->target->enemy == NULL) ? "NULL"
				: (pursuit_state->target->enemy->player_name));
			inFunction(reportbuf);

			AI2rPursuit_Report(ioCharacter, pursuit_state, inVerbose, inFunction);
		}
		break;

		case AI2cGoal_Alarm:
			sprintf(reportbuf, "  %s - alarm", inName);
			inFunction(reportbuf);

			AI2rAlarm_Report(ioCharacter, &inState->state.alarm, inVerbose, inFunction);
		break;

		case AI2cGoal_Combat:
			sprintf(reportbuf, "  %s - combat(%s)", inName, (inState->state.combat.target == NULL)
						? "nobody" : inState->state.combat.target->player_name);
			inFunction(reportbuf);

			AI2rCombat_Report(ioCharacter, &inState->state.combat, inVerbose, inFunction);
		break;

		case AI2cGoal_Neutral:
			sprintf(reportbuf, "  %s - neutral", inName);
			inFunction(reportbuf);
		break;

		case AI2cGoal_Panic:
			sprintf(reportbuf, "  %s - panic", inName);
			inFunction(reportbuf);
		break;

		default:
			sprintf(reportbuf, "  %s - unknown job type %d", inName, inGoal);
			inFunction(reportbuf);
		break;
	}
}

// drop into the debugger next update loop
void AI2rDebug_BreakPoint(void)
{
	AI2gDebug_BreakOnUpdate = UUcTrue;
}

// called once every frame by display code
void AI2rDisplayGlobalDebuggingInfo(void)
{
#if TOOL_VERSION
	if ((AI2gDebug_ShowActivationPaths) || (AI2gDebug_ShowAstarEvaluation)) {
		ASrDisplayDebuggingInfo();
	}
#endif

	if (AI2gShowPathfindingErrors) {
		AI2rError_DisplayPathfindingErrors();
	}

	if (AI2gShowProjectiles) {
		AI2rKnowledge_DisplayProjectiles();
	}

	if (AI2gShowFiringSpreads) {
		AI2rKnowledge_DisplayFiringSpreads();
	}

	if (AI2gShowSounds) {
		AI2rKnowledge_DisplaySounds();
	}

#if TOOL_VERSION
	if (AI2gDebugDisplayGraph || (AI2gShowPathfindingErrors && AI2gPathfindingErrorStorage.no_path)) {
		// draw path-graph connection lines
		PHrDebugGraph_Render();
	}
#endif

	if (AI2gDebug_LocalMovement || AI2gDebug_LocalPathfinding) {
		// set up for testing local movement
		AI2rLocalPath_DebugLines(ONrGameState_GetPlayerCharacter());
	}

#if TOOL_VERSION
	if (AI2gDebug_ShowLOS) {
		UUtUns32 itr;

		// show the line-of-sight checks that have been performed
		for (itr = 0; itr < AI2gCombat_LOSUsedPoints; itr += 2) {
			M3rGeom_Line_Light(&AI2gCombat_LOSPoints[itr], &AI2gCombat_LOSPoints[itr + 1],
								AI2gCombat_LOSClear[itr] ? IMcShade_Green : IMcShade_Red);
		}
	}

	if ((AI2gDebug_LocalPathfinding || AI2gDebug_LocalMovement) && AI2gDebugLocalPath_Stored) {
		UUtUns32 itr, count;
		M3tPoint3D p0, p1, x0, x1;
		M3tVector3D dir, vertical, horizontal;
		IMtShade shade;

		if (AI2gDebug_LocalPathfinding) {
			count = 1;
		} else {
			count = AI2gDebugLocalPath_LineCount;
		}

		MUmVector_Set(vertical, 0, 2, 0);

		for (itr = 0; itr < count; itr++) {
			if ((AI2gDebugLocalPath_Weight[itr] < 0) || (AI2gDebugLocalPath_Weight[itr] >= PHcMax)) {
				// unknown weight value!
				shade = IMcShade_Yellow;
			} else {
				shade = PHgPathfindingWeightColor[AI2gDebugLocalPath_Weight[itr]];
			}
			
			if (count == 1) {
				MUmVector_Copy(p0, AI2gDebugLocalPath_Point);
				p0.y += 0.5f;

				MUmVector_Copy(p1, AI2gDebugLocalPath_EndPoint[itr]);
				p1.y += 0.5f;

				M3rGeom_Line_Light(&p0, &p1, shade);
			} else {	
				MUmVector_Copy(p1, AI2gDebugLocalPath_EndPoint[itr]);
				p1.y += 9.0f;

				M3rGeom_Line_Light(&AI2gDebugLocalPath_EndPoint[itr], &p1, shade);
			}

			if (!AI2gDebugLocalPath_Success[itr]) {
				// failure - draw a cross on top of the line
				MUmVector_Subtract(dir, AI2gDebugLocalPath_EndPoint[itr], AI2gDebugLocalPath_Point);
				MUrVector_CrossProduct(&dir, &vertical, &horizontal);
				MUrVector_SetLength(&horizontal, 2.0f);

				MUmVector_Copy(x0, p1);
				MUmVector_ScaleIncrement(x0, +1, vertical);
				MUmVector_ScaleIncrement(x0, +1, horizontal);

				MUmVector_Copy(x1, p1);
				MUmVector_ScaleIncrement(x1, -1, vertical);
				MUmVector_ScaleIncrement(x1, -1, horizontal);
				M3rGeom_Line_Light(&x0, &x1, shade);

				MUmVector_ScaleIncrement(x0, -2, horizontal);
				MUmVector_ScaleIncrement(x1, +2, horizontal);
				M3rGeom_Line_Light(&x0, &x1, shade);
			}
		}
	}
#endif
}

// called every frame by display code for every AI2 character
void AI2rDisplayDebuggingInfo(ONtCharacter *ioCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);

	if (AI2gShowHealth) {
		M3tPoint3D p0, p1, l0, l1;
		UUtUns32 itr;
		IMtShade shade = ONrCharacter_GetHealthShade(ioCharacter->hitPoints, ioCharacter->maxHitPoints);

		ONrCharacter_GetEyePosition(ioCharacter, &p0);
		p0.y += 10.0f;
		l0 = l1 = p1 = p0;

		p0.x -= 3.0f * MUrCos(ioCharacter->facing);
		p0.z += 3.0f * MUrSin(ioCharacter->facing);
		p1.x += 3.0f * MUrCos(ioCharacter->facing);
		p1.z -= 3.0f * MUrSin(ioCharacter->facing);

		for (itr = 0; itr <= ioCharacter->hitPoints; ) {
			M3rGeom_Line_Light(&p0, &p1, shade);

			p0.y += 1.0f;
			p1.y += 1.0f;
			itr += 5;
		}

		l1.y += ioCharacter->maxHitPoints / 5.0f;
		M3rGeom_Line_Light(&l0, &l1, IMcShade_White);
	}

#if TOOL_VERSION
	// only available in tool because we don't want to allocate
	// debugging textures in the ship version
	if (AI2gShowNames || AI2gShowHealth) {
		UUtRect					string_bounds;
		UUtUns16				string_width;
		IMtPoint2D				text_dest;
		float					sprite_size;
		M3tPoint3D				position;
		UUtError				error;
		char					name[96];
	
		if (!AI2gDebugName_Initialized) {
			error = AI2rDebugName_Initialize();
			if (error != UUcError_None)
				return;
		}

		// erase the texture and set the text contexts shade
		M3rTextureMap_Fill(AI2gDebugName_Texture, AI2gDebugName_WhiteColor, NULL);
		TSrContext_SetShade(AI2gDebugName_TextContext, IMcShade_Black);
	
		// get the string
		if (AI2gShowHealth) {
			if (AI2gShowNames) {
				sprintf(name, "%s [%d/%d]", ioCharacter->player_name, ioCharacter->hitPoints, ioCharacter->maxHitPoints);
			} else {
				sprintf(name, "[%d/%d]", ioCharacter->hitPoints, ioCharacter->maxHitPoints);
			}
		} else {
			UUrString_Copy(name, ioCharacter->player_name, 96);
		}

		// work out how big the string is going to be
		TSrContext_GetStringRect(AI2gDebugName_TextContext, name, &string_bounds);
		string_width = string_bounds.right - string_bounds.left;

		text_dest = AI2gDebugName_Dest;
		text_dest.x += (AI2gDebugName_TextureWidth - string_width) / 2;

		sprite_size = ((float) string_width) / AI2gDebugName_TextureWidth;

		// draw the string to the texture
		TSrContext_DrawString(
			AI2gDebugName_TextContext,
			AI2gDebugName_Texture,
			name,
			&AI2gDebugName_TextureBounds,
			&text_dest);

		ONrCharacter_GetEyePosition(ioCharacter, &position);
		position.y += 6.0f;

		// draw the sprite
		M3rGeom_State_Set(M3cGeomStateIntType_SpriteMode, M3cGeomState_SpriteMode_Normal);
		M3rGeom_State_Commit();

		M3rSprite_Draw(
			AI2gDebugName_Texture,
			&position,
			4.0f * AI2gDebugName_WidthRatio,
			4.0f,
			IMcShade_White,
			M3cMaxAlpha,
			0,
			NULL,
			NULL,
			0,
			1.0f - sprite_size, 0);
	}
#endif

	if (AI2gShowCombatRanges) {
		M3tVector3D center_pt, p0, p1, p2, p3;
		float theta, m_rad, s_rad;
		UUtUns32 itr;

		ONrCharacter_GetPathfindingLocation(ioCharacter, &center_pt);
		center_pt.y += ioCharacter->heightThisFrame;
		m_rad = ioCharacter->ai2State.combatSettings.medium_range;
		s_rad = ioCharacter->ai2State.combatSettings.short_range;
		MUmVector_Copy(p1, center_pt);
		MUmVector_Copy(p3, center_pt);
		p1.z += m_rad;
		p3.z += s_rad;
		for (itr = 1; itr <= 36; itr++) {
			theta = itr * M3c2Pi / 36;
			MUmVector_Copy(p0, p1);
			MUmVector_Copy(p1, center_pt);
			p1.x += m_rad * MUrSin(theta);
			p1.z += m_rad * MUrCos(theta);

			MUmVector_Copy(p2, p3);
			MUmVector_Copy(p3, center_pt);
			p3.x += s_rad * MUrSin(theta);
			p3.z += s_rad * MUrCos(theta);

			M3rGeom_Line_Light(&p0, &p1, IMcShade_Blue);
			M3rGeom_Line_Light(&p2, &p3, IMcShade_Blue);
		}
	}

	if (AI2gShowPathfindingGrids && (ioCharacter->currentNode != NULL) && (ioCharacter->currentPathnode != NULL)) {
		PHrRenderGrid(ioCharacter->currentPathnode, &ioCharacter->currentNode->roomData, NULL, NULL);
	}

	if (AI2gShowFights && (ioCharacter->melee_fight != NULL)) {
		ONtCharacter *attacker;
		IMtShade shade;
		M3tVector3D center_pt, line_pt;
		AI2tFightInfo *attacker_info;
		UUtUns32 time, current_time;
		UUtUns8 strength, colorval;

		// draw the state of the melee fight centred on this character
		ONrCharacter_GetPelvisPosition(ioCharacter, &center_pt);
		current_time = ONrGameState_GetGameTime();

		attacker = ioCharacter->melee_fight->attacker_head;
		while (attacker != NULL) {
			UUmAssert(attacker->charType == ONcChar_AI2);
			UUmAssert(attacker->ai2State.currentGoal == AI2cGoal_Combat);

			attacker_info = &attacker->ai2State.currentState->state.combat.melee.fight_info;
			UUmAssert(attacker_info->fight_owner == ioCharacter->melee_fight);

			ONrCharacter_GetPelvisPosition(attacker, &line_pt);
			if (attacker_info->has_attack_cookie) {
				if (attacker_info->cookie_expire_time > current_time) {
					time = attacker_info->cookie_expire_time - current_time;
					strength = (UUtUns8) (time / 90);
					colorval = (UUtUns8) ((256 * (time % 90)) / 90);
				} else {
					strength = 0;
					colorval = 0;
				}

				if (strength == 0) {
					shade = 0xFF000000 | (colorval << 16);
				} else if (strength == 1) {
					shade = 0xFFFF0000 | (colorval << 8);
				} else if (strength == 2) {
					shade = 0xFFFFFF00 | colorval;
				} else {
					shade = IMcShade_White;
				}
			} else {
				shade = IMcShade_Blue;
			}
			M3rGeom_Line_Light(&center_pt, &line_pt, shade);

			attacker = attacker_info->next;
		}
	}

	// *** everything beyond here is only done for AI-movement-controlled characters
	if ((ioCharacter->flags & ONcCharacterFlag_AIMovement) == 0) {
		return;
	}

	if (AI2gDrawPaths) {
		AI2rPath_RenderPath(ioCharacter);
		AI2rMovement_RenderPath(ioCharacter);
	}
		
	// *** everything beyond here is only done for AI characters
	if (ioCharacter->charType != ONcChar_AI2) {
		return;
	}

#if TOOL_VERSION
	if (AI2gDebug_ShowActivationPaths && (active_character != NULL)) {
		if (active_character->movement_state.activation_valid) {
			M3tPoint3D p0, p1;

			// draw the distance that we snapped when we became active
			M3rGeom_Line_Light(&active_character->movement_state.activation_rawpoint,
								&active_character->movement_state.activation_position, IMcShade_Pink);

			p0 = p1 = active_character->movement_state.activation_position;
			p0.y += 4.0f; p1.y += 4.0f;
			p0.x += 4.0f; p1.x -= 4.0f;
			M3rGeom_Line_Light(&p0, &p1, IMcShade_Pink);

			p0.x -= 4.0f; p1.x += 4.0f;
			p0.z += 4.0f; p1.z -= 4.0f;
			M3rGeom_Line_Light(&p0, &p1, IMcShade_Pink);

			if (active_character->movement_state.activation_frompath) {
				// draw the path that we snapped onto when we became active
				p0 = active_character->movement_state.activation_pathsegment[0];
				p1 = active_character->movement_state.activation_pathsegment[1];
				p0.y += 5.0f; p1.y += 5.0f;
				M3rGeom_Line_Light(&p0, &p1, IMcShade_LightBlue);
			}
		}
	}
#endif

	if (AI2gShowLineToChar) {
		ONtCharacter *player;

		player = ONrGameState_GetPlayerCharacter();
		if (player != NULL) {
			M3tPoint3D	p0, p1;

			ONrCharacter_GetPathfindingLocation(player, &p0);
			p0.y += ONcCharacterHeight / 2;
			ONrCharacter_GetPathfindingLocation(ioCharacter, &p1);
			p1.y += ONcCharacterHeight / 2;

			M3rGeom_Line_Light(&p0, &p1, (ioCharacter->flags & ONcCharacterFlag_Draw) ? IMcShade_Green :
										ONrCharacter_IsActive(ioCharacter) ? IMcShade_Blue : IMcShade_Red);
		}
	}

	if (AI2gShowJobLocations) {
		if (ioCharacter->ai2State.has_job_location) {
			M3tVector3D p0, p1;

			// draw a green cross at character's job location
			p0 = p1 = ioCharacter->ai2State.job_location;
			p0.x += 6.0f;
			p1.x -= 6.0f;
			M3rGeom_Line_Light(&p0, &p1, IMcShade_Green);
			
			p0.x -= 6.0f;
			p1.x += 6.0f;
			p0.z += 6.0f;
			p1.z -= 6.0f;
			M3rGeom_Line_Light(&p0, &p1, IMcShade_Green);
		}
	}

	if (ioCharacter->ai2State.currentGoal == AI2cGoal_Combat) {
		AI2tCombatState *combat_state = &ioCharacter->ai2State.currentState->state.combat;
		AI2tTargetingState *targeting_state = &combat_state->targeting;
		M3tVector3D p0, p1, y_ax, z_ax;
		UUtUns32 itr;
		IMtShade shade;

		// display combat debugging info

		if (AI2gDebug_ShowTargeting && (combat_state->current_weapon != NULL) && (targeting_state->last_computation_success)) {
			M3tMatrix3x3 shooter_matrix, worldmatrix;
			M3tPoint3D weapon_dir, shoot_at, aim_at;

			/*
			 * draw debugging info about our targeting vectors
			 */
			
			// draw a cross at the current aim point (where we want to hit)
			MUmVector_Copy(p0, targeting_state->current_aim_pt);
			MUmVector_Copy(p1, targeting_state->current_aim_pt);
			
			p0.x += 3.0f;
			p1.x -= 3.0f;
			M3rGeom_Line_Light(&p0, &p1, IMcShade_Blue);
			
			p0.x -= 3.0f;
			p1.x += 3.0f;
			p0.y += 3.0f;
			p1.y -= 3.0f;
			M3rGeom_Line_Light(&p0, &p1, IMcShade_Blue);
			
			p0.y -= 3.0f;
			p1.y += 3.0f;
			p0.z += 3.0f;
			p1.z -= 3.0f;
			M3rGeom_Line_Light(&p0, &p1, IMcShade_Blue);

			// draw a cross at the current point at which we are pointing our weapon
			MUmVector_Add(aim_at, targeting_state->weapon_pt, targeting_state->desired_shooting_vector);
			MUmVector_Copy(p0, aim_at);
			MUmVector_Copy(p1, aim_at);
			
			p0.x += 3.0f;
			p1.x -= 3.0f;
			M3rGeom_Line_Light(&p0, &p1, IMcShade_LightBlue);
			
			p0.x -= 3.0f;
			p1.x += 3.0f;
			p0.y += 3.0f;
			p1.y -= 3.0f;
			M3rGeom_Line_Light(&p0, &p1, IMcShade_LightBlue);
			
			p0.y -= 3.0f;
			p1.y += 3.0f;
			p0.z += 3.0f;
			p1.z -= 3.0f;
			M3rGeom_Line_Light(&p0, &p1, IMcShade_LightBlue);



			shade = (targeting_state->last_computed_ontarget) ? IMcShade_Green : IMcShade_Red;

			// calculate the current matrix from shooterspace to worldspace
			MUrMatrix3x3_Transpose((M3tMatrix3x3 *) &targeting_state->weapon_parameters->shooter_inversematrix, &shooter_matrix);
			MUrMatrix3x3_Multiply((M3tMatrix3x3 *) targeting_state->weapon_matrix, &shooter_matrix, &worldmatrix);
			MUrMatrix_GetCol((M3tMatrix4x3 *) &worldmatrix, 1, &y_ax);
			MUrMatrix_GetCol((M3tMatrix4x3 *) &worldmatrix, 2, &z_ax);

			// work out where this points our gun
			MUrMatrix_GetCol((M3tMatrix4x3 *) &worldmatrix, 0, &weapon_dir);
			MUmVector_Copy(shoot_at, targeting_state->weapon_pt);
			MUmVector_ScaleIncrement(shoot_at, targeting_state->aiming_distance, weapon_dir);

			M3rGeom_Line_Light(&targeting_state->weapon_pt, &aim_at, IMcShade_Blue);

			// draw our targeting vector
			MUmVector_Copy(p0, targeting_state->targeting_frompt);
			MUmVector_Add(p1, p0, targeting_state->targeting_vector);
			M3rGeom_Line_Light(&p0, &p1, IMcShade_Yellow);

			// draw our actual aiming vector
			MUmVector_Copy(p0, ioCharacter->location);
			p0.y += ONcCharacterHeight;
			M3rGeom_Line_Light(&p0, &active_character->aimingTarget, IMcShade_Purple);

			// draw our gun's aiming vector
			MUmVector_Copy(p0, shoot_at);
			MUmVector_Copy(p1, shoot_at);

			M3rGeom_Line_Light(&targeting_state->weapon_pt, &shoot_at, shade);
			MUmVector_ScaleIncrement(p0,  1.0f, y_ax);
			MUmVector_ScaleIncrement(p1, -1.0f, y_ax);
			M3rGeom_Line_Light(&p0, &p1, shade);

			MUmVector_ScaleIncrement(p0, -1.0f, y_ax);
			MUmVector_ScaleIncrement(p1,  1.0f, y_ax);
			MUmVector_ScaleIncrement(p0,  1.0f, z_ax);
			MUmVector_ScaleIncrement(p1, -1.0f, z_ax);
			M3rGeom_Line_Light(&p0, &p1, shade);

			// draw accuracy ring around the target point
			MUmVector_Copy(p1, aim_at);
			MUmVector_ScaleIncrement(p1, targeting_state->last_computed_accuracy, y_ax);
			for (itr = 0; itr <= 32; itr++) {
				MUmVector_Copy(p0, p1);
				MUmVector_Copy(p1, aim_at);
				MUmVector_ScaleIncrement(p1, targeting_state->last_computed_accuracy * MUrCos(itr * M3c2Pi / 32), y_ax);
				MUmVector_ScaleIncrement(p1, targeting_state->last_computed_accuracy * MUrSin(itr * M3c2Pi / 32), z_ax);
				M3rGeom_Line_Light(&p0, &p1, shade);
			}

			// draw tolerance ring around the target point
			MUmVector_Copy(p1, aim_at);
			MUmVector_ScaleIncrement(p1, targeting_state->last_computed_tolerance, y_ax);
			for (itr = 0; itr <= 32; itr++) {
				MUmVector_Copy(p0, p1);
				MUmVector_Copy(p1, aim_at);
				MUmVector_ScaleIncrement(p1, targeting_state->last_computed_tolerance * MUrCos(itr * M3c2Pi / 32), y_ax);
				MUmVector_ScaleIncrement(p1, targeting_state->last_computed_tolerance * MUrSin(itr * M3c2Pi / 32), z_ax);
				M3rGeom_Line_Light(&p0, &p1, IMcShade_Blue);
			}
		}
		
		if ((AI2gDebug_ShowPrediction) && (targeting_state->predictionbuf != NULL)) {
			/*
			 * draw debugging info about our prediction vectors
			 */
			
			UUtUns32 itr, sample_trend, sample_vel, sample_now;
			UUtUns32 trend_frames, velocity_frames, delay_frames;			
			
			trend_frames = targeting_state->targeting_params->predict_trendframes;
			velocity_frames = targeting_state->targeting_params->predict_velocityframes;
			delay_frames = targeting_state->targeting_params->predict_delayframes;
			
			// draw our prediction buffer
			for (itr = 0; itr < trend_frames; itr++) {
				// work out which indices we're using to build the current motion estimate
				sample_trend = targeting_state->next_sample + UUmMin(targeting_state->num_samples_taken, trend_frames);
				sample_vel   = targeting_state->next_sample + UUmMin(targeting_state->num_samples_taken, velocity_frames);
				sample_now   = targeting_state->next_sample + UUmMin(targeting_state->num_samples_taken, delay_frames);
				
				// the buffer is predict_trendframes long
				sample_trend %= trend_frames;
				sample_vel   %= trend_frames;
				sample_now   %= trend_frames;
				
				if ((targeting_state->num_samples_taken >= trend_frames) ||
					(itr > targeting_state->next_sample)) {
					MUmVector_Copy(p0, targeting_state->predictionbuf[itr]);
					MUmVector_Copy(p1, targeting_state->predictionbuf[itr]);
					p0.y += 5.0f;
					p1.y -= 5.0f;
					
					if (itr == sample_now) {
						shade = IMcShade_Yellow;
					} else if (itr == sample_vel) {
						shade = IMcShade_Green;
					} else if (itr == sample_trend) {
						shade = IMcShade_Red;
					} else {
						shade = IMcShade_White;
					}
					
					M3rGeom_Line_Light(&p0, &p1, shade);
				}
			}
			
			if (targeting_state->valid_target_pt) {
				// draw our target point
				MUmVector_Copy(p0, targeting_state->target_pt);
				MUmVector_Copy(p1, p0);
				
				p0.x += 3.0f;
				p1.x -= 3.0f;
				M3rGeom_Line_Light(&p0, &p1, IMcShade_Purple);
				
				p0.x -= 3.0f;
				p1.x += 3.0f;
				p0.y += 3.0f;
				p1.y -= 3.0f;
				M3rGeom_Line_Light(&p0, &p1, IMcShade_Purple);
				
				p0.y -= 3.0f;
				p1.y += 3.0f;
				p0.z += 3.0f;
				p1.z -= 3.0f;
				M3rGeom_Line_Light(&p0, &p1, IMcShade_Purple);
			}
			
			if (combat_state->target != NULL) {
				// draw the target's predicted velocity and trend vectors
				MUmVector_Copy(p0, targeting_state->target->location);
				p0.y += targeting_state->target->heightThisFrame;
				
				MUmVector_Add(p1, p0, targeting_state->predicted_trend);
				M3rGeom_Line_Light(&p0, &p1, IMcShade_LightBlue);
				
				MUmVector_Add(p1, p0, targeting_state->predicted_velocity);
				M3rGeom_Line_Light(&p0, &p1, IMcShade_Blue);
				
				// draw a little tick on the velocity vector
				MUmVector_ScaleIncrement(p0, targeting_state->current_prediction_accuracy, targeting_state->predicted_velocity);
				MUmVector_Copy(p1, p0);
				p0.y += 2.0f;
				p1.y -= 2.0f;
				M3rGeom_Line_Light(&p0, &p1, IMcShade_Orange);
			}
		}

		if (AI2gDebug_ShowLocalMelee && (combat_state->maneuver.primary_movement == AI2cPrimaryMovement_Melee)) {
			AI2tMeleeState *melee_state = &combat_state->melee;
			IMtShade shade;

			if (ONrGameState_GetGameTime() - melee_state->cast_frame < 60) {
				// draw the local-environment awareness lines
				shade = melee_state->danger_ahead ? IMcShade_Orange : (melee_state->localpath_blocked ? IMcShade_Red : IMcShade_White);
				M3rGeom_Line_Light(&melee_state->cast_pt, &melee_state->cast_ahead_end, shade);

				shade = melee_state->danger_behind ? IMcShade_Orange : IMcShade_White;
				M3rGeom_Line_Light(&melee_state->cast_pt, &melee_state->cast_behind_end, shade);
			}
		}
	}

	if (active_character && AI2gShowVisionCones) {
		const ONtVisionConstants *vision = &ioCharacter->characterClass->vision;
		UUtUns32 itr;
		float x, y, delta_x, vertical_max, vertical_min, vertical_range, horiz_angle, dist;
		M3tPoint3D points[9], p0, p1;
		AI2tExecutorState *executor = &active_character->executor_state;
		
		ONrCharacter_GetEyePosition(ioCharacter, &points[0]);
	
		vertical_range = MUrACos(vision->vertical_range);

		if (active_character->isAiming) {
			horiz_angle = active_character->aimingVectorLR;
			vertical_min = -active_character->aimingVectorUD - vertical_range;
			vertical_max = -active_character->aimingVectorUD + vertical_range;
		} else {
			horiz_angle = ioCharacter->facing;
			vertical_min = -vertical_range;
			vertical_max = +vertical_range;
		}
		UUmTrig_Clip(vertical_min);
		UUmTrig_Clip(vertical_max);

		// draw peripheral range
		delta_x = (1.0f - vision->periph_max) / 20;
		x = vision->periph_max;
		MUmVector_Copy(points[5], points[0]);
		MUmVector_Copy(points[6], points[0]);
		MUmVector_Copy(points[7], points[0]);
		MUmVector_Copy(points[8], points[0]);
		for (itr = 0; itr <= 20; itr++, x += delta_x) {
			y = MUrSqrt(1.0f - UUmSQR(x));

			if (x > vision->periph_range) {
				dist = vision->periph_distance;
			} else {
				dist = vision->periph_distance * (1.0f - (vision->periph_range - x) /
					(vision->periph_range - vision->periph_max));
			}

			MUmVector_Set(p0,  y * dist, 0, x * dist);
			MUmVector_Set(p1, -y * dist, 0, x * dist);
			MUrPoint_RotateXAxis(&p0, vertical_min, &points[1]);
			MUrPoint_RotateXAxis(&p0, vertical_max, &points[2]);
			MUrPoint_RotateXAxis(&p1, vertical_min, &points[3]);
			MUrPoint_RotateXAxis(&p1, vertical_max, &points[4]);
			MUrPoint_RotateYAxis(&points[1], horiz_angle, &points[1]);
			MUrPoint_RotateYAxis(&points[2], horiz_angle, &points[2]);
			MUrPoint_RotateYAxis(&points[3], horiz_angle, &points[3]);
			MUrPoint_RotateYAxis(&points[4], horiz_angle, &points[4]);

			MUmVector_Add(points[1], points[1], points[0]);
			MUmVector_Add(points[2], points[2], points[0]);
			MUmVector_Add(points[3], points[3], points[0]);
			MUmVector_Add(points[4], points[4], points[0]);

			// draw lines vertically
			M3rGeom_Line_Light(&points[1], &points[2], IMcShade_Blue);
			M3rGeom_Line_Light(&points[3], &points[4], IMcShade_Blue);

			// draw lines horizontally
			M3rGeom_Line_Light(&points[1], &points[5], IMcShade_Blue);
			M3rGeom_Line_Light(&points[2], &points[6], IMcShade_Blue);
			M3rGeom_Line_Light(&points[3], &points[7], IMcShade_Blue);
			M3rGeom_Line_Light(&points[4], &points[8], IMcShade_Blue);

			// store points for next loop
			points[5] = points[1];
			points[6] = points[2];
			points[7] = points[3];
			points[8] = points[4];
		}

		// draw central range
		delta_x = (1.0f - vision->central_max) / 20;
		x = vision->central_max;
		MUmVector_Copy(points[5], points[0]);
		MUmVector_Copy(points[6], points[0]);
		MUmVector_Copy(points[7], points[0]);
		MUmVector_Copy(points[8], points[0]);
		for (itr = 0; itr <= 20; itr++, x += delta_x) {
			y = MUrSqrt(1.0f - UUmSQR(x));

			if (x > vision->central_range) {
				dist = vision->central_distance;
			} else {
				dist = vision->central_distance * (1.0f - (vision->central_range - x) /
					(vision->central_range - vision->central_max));
			}

			MUmVector_Set(p0,  y * dist, 0, x * dist);
			MUmVector_Set(p1, -y * dist, 0, x * dist);
			MUrPoint_RotateXAxis(&p0, vertical_min, &points[1]);
			MUrPoint_RotateXAxis(&p0, vertical_max, &points[2]);
			MUrPoint_RotateXAxis(&p1, vertical_min, &points[3]);
			MUrPoint_RotateXAxis(&p1, vertical_max, &points[4]);
			MUrPoint_RotateYAxis(&points[1], horiz_angle, &points[1]);
			MUrPoint_RotateYAxis(&points[2], horiz_angle, &points[2]);
			MUrPoint_RotateYAxis(&points[3], horiz_angle, &points[3]);
			MUrPoint_RotateYAxis(&points[4], horiz_angle, &points[4]);

			MUmVector_Add(points[1], points[1], points[0]);
			MUmVector_Add(points[2], points[2], points[0]);
			MUmVector_Add(points[3], points[3], points[0]);
			MUmVector_Add(points[4], points[4], points[0]);

			// draw lines vertically
			M3rGeom_Line_Light(&points[1], &points[2], IMcShade_LightBlue);
			M3rGeom_Line_Light(&points[3], &points[4], IMcShade_LightBlue);

			// draw lines horizontally
			M3rGeom_Line_Light(&points[1], &points[5], IMcShade_LightBlue);
			M3rGeom_Line_Light(&points[2], &points[6], IMcShade_LightBlue);
			M3rGeom_Line_Light(&points[3], &points[7], IMcShade_LightBlue);
			M3rGeom_Line_Light(&points[4], &points[8], IMcShade_LightBlue);

			// store points for next loop
			points[5] = points[1];
			points[6] = points[2];
			points[7] = points[3];
			points[8] = points[4];
		}
	}
}

#if TOOL_VERSION
static UUtError AI2rDebugName_Initialize(void)
{
	UUtError error;
	TStFontFamily			*font_family;
	IMtPixelType			textureFormat;

	if (AI2gDebugName_Initialized)
		return UUcError_None;

	/*
	 * initialize the name drawing code
	 */

	M3rDrawEngine_FindGrayscalePixelType(&textureFormat);
	
	AI2gDebugName_WhiteColor = IMrPixel_FromShade(textureFormat, IMcShade_White);

	error = TSrFontFamily_Get(TScFontFamily_Default, &font_family);
	if (error != UUcError_None)
	{
		goto cleanup;
	}
	
	error = TSrContext_New(font_family, TScFontSize_Default, TScStyle_Bold, TSc_SingleLine, UUcFalse, &AI2gDebugName_TextContext);
	if (error != UUcError_None)
	{
		goto cleanup;
	}
	
	AI2gDebugName_Dest.x = 2;
	AI2gDebugName_Dest.y =
		TSrFont_GetLeadingHeight(TSrContext_GetFont(AI2gDebugName_TextContext, TScStyle_InUse)) +
		TSrFont_GetAscendingHeight(TSrContext_GetFont(AI2gDebugName_TextContext, TScStyle_InUse));
	
	TSrContext_GetStringRect(AI2gDebugName_TextContext, "maximum_length_of_character_name", &AI2gDebugName_TextureBounds);
	
	error =
		M3rTextureMap_New(
			AI2gDebugName_TextureBounds.right,
			AI2gDebugName_TextureBounds.bottom,
			textureFormat,
			M3cTexture_AllocMemory,
			M3cTextureFlags_None,
			"character name texture",
			&AI2gDebugName_Texture);
	if (error != UUcError_None)
	{
		goto cleanup;
	}

	// get the REAL size of the texture that motoko allocated for us
	M3rTextureRef_GetSize((void *) AI2gDebugName_Texture,
					(UUtUns16*)&AI2gDebugName_TextureBounds.right,
					(UUtUns16*)&AI2gDebugName_TextureBounds.bottom);
	
	AI2gDebugName_TextureWidth = AI2gDebugName_TextureBounds.right;
	AI2gDebugName_TextureHeight = AI2gDebugName_TextureBounds.bottom;
	
	AI2gDebugName_WidthRatio = (float)AI2gDebugName_TextureWidth / (float)AI2gDebugName_TextureHeight;
	
	AI2gDebugName_Initialized = UUcTrue;

	return UUcError_None;


cleanup:
	AI2rDebugName_Terminate();

	return UUcError_Generic;
}

static void AI2rDebugName_Terminate(void)
{
	if (AI2gDebugName_TextContext)
	{
		TSrContext_Delete(AI2gDebugName_TextContext);
		AI2gDebugName_TextContext = NULL;
	}

	AI2gDebugName_Initialized = UUcFalse;
}
#endif
