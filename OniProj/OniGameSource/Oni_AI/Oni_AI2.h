#pragma once
#ifndef ONI_AI2_H
#define ONI_AI2_H

/*
	FILE:	Oni_AI2.h
	
	AUTHOR:	Michael Evans
	
	CREATED: November 15, 1999
	
	PURPOSE: AI for characters in Oni
	
	Copyright (c) 1999

*/

#include "BFW.h"
#include "Oni_GameState.h"
//#include "Oni_Character.h"
//#include "Oni_Object.h"

#include "Oni_AI2_Script.h"

#include "Oni_AI2_Knowledge.h"
#include "Oni_AI2_Path.h"
#include "Oni_AI2_Movement.h"
#include "Oni_AI2_MovementState.h"
#include "Oni_AI2_MovementStub.h"
#include "Oni_AI2_Executor.h"
#include "Oni_AI2_Alert.h"
#include "Oni_AI2_Targeting.h"

#include "Oni_AI2_Error.h"
#include "Oni_AI2_LocalPath.h"

#include "Oni_AI2_Idle.h"
#include "Oni_AI2_Guard.h"
#include "Oni_AI2_Patrol.h"
#include "Oni_AI2_TeamBattle.h"
#include "Oni_AI2_Combat.h"
#include "Oni_AI2_MeleeProfile.h"
#include "Oni_AI2_Fight.h"
#include "Oni_AI2_Melee.h"
#include "Oni_AI2_Alarm.h"
#include "Oni_AI2_Neutral.h"
#include "Oni_AI2_Pursuit.h"
#include "Oni_AI2_Panic.h"


// ------------------------------------------------------------------------------------
// -- constants

#define AI2cDefaultClassName	"striker_easy_1"

typedef enum AI2tGoal
{
	AI2cGoal_None		= 0,			// uninitialised or transition state - literally nothing
	AI2cGoal_Idle		= 1,
	AI2cGoal_Guard		= 2,			// unimplemented
	AI2cGoal_Patrol		= 3,
	AI2cGoal_TeamBattle	= 4,			// unimplemented
	AI2cGoal_Combat		= 5,
	AI2cGoal_Pursuit	= 6,
	AI2cGoal_Alarm		= 7,
	AI2cGoal_Neutral	= 8,
	AI2cGoal_Panic		= 9,

	AI2cGoal_Max		= 10
} AI2tGoal;

extern const char AI2cGoalName[AI2cGoal_Max][16];

// AI2tState->flags
enum {
	AI2cFlag_InUse				= (1 << 0),
	AI2cFlag_Passive			= (1 << 1),
	AI2cFlag_NonCombatant		= (1 << 2),
	AI2cFlag_RunCombatScript	= (1 << 3),		// these scripts are called in main AI update so that unexpected state 
	AI2cFlag_RunAlarmScript		= (1 << 4),		// changes don't happen in the knowledge code
	AI2cFlag_RunNeutralScript	= (1 << 5),		// similar rationale to combat and alarm scripts
	AI2cFlag_RunOutOfAmmoScript	= (1 << 6),	// ditto
	AI2cFlag_RunNoPathScript	= (1 << 7),	// ditto
	AI2cFlag_Omniscient			= (1 << 10),
	AI2cFlag_Blind				= (1 << 11),
	AI2cFlag_Deaf				= (1 << 12),
	AI2cFlag_IgnorePlayer		= (1 << 13),
	AI2cFlag_StateTransition	= (1 << 14),
	AI2cFlag_SaidAlertSound		= (1 << 15)
};

enum {
	AI2cTeam_Friend				= 0,
	AI2cTeam_Neutral			= 1,
	AI2cTeam_Hostile			= 2
};

enum {
	AI2cVocalization_Taunt		= 0,
	AI2cVocalization_Alert		= 1,
	AI2cVocalization_Startle	= 2,
	AI2cVocalization_CheckBody	= 3,
	AI2cVocalization_Pursue		= 4,
	AI2cVocalization_Cower		= 5,
	AI2cVocalization_SuperPunch	= 6,
	AI2cVocalization_SuperKick	= 7,
	AI2cVocalization_Super3		= 8,
	AI2cVocalization_Super4		= 9,

	AI2cVocalization_Max		= 10			// must match array sizes in AI2tVocalizationTable
};

extern const char AI2cVocalizationTypeName[AI2cVocalization_Max][16];
extern const UUtBool AI2cGoalIsCasual[AI2cGoal_Max];

// ------------------------------------------------------------------------------------
// -- structures

typedef tm_struct AI2tVocalizationTable {
	UUtUns8				sound_chance[10];		// must match AI2cVocalization_Max
	tm_pad				pad[2];
	char				sound_name[10][32];
} AI2tVocalizationTable;

typedef struct AI2tGoalState {
	UUtBool				begun;
	union {
		AI2tIdleState		idle;
		AI2tGuardState		guard;
		AI2tPatrolState		patrol;
		AI2tTeamBattleState	teamBattle;
		AI2tCombatState		combat;
		AI2tPursuitState	pursuit;
		AI2tAlarmState		alarm;
		AI2tNeutralState	neutral;
		AI2tPanicState		panic;
	} state;
} AI2tGoalState;

// an AI's persistent state
typedef struct AI2tState
{
	// global settings
	UUtUns32			flags;
	
	// goal and job control
	AI2tGoal			jobGoal;
	AI2tGoal			currentGoal;
	AI2tGoalState		jobState;
	AI2tGoalState		currentStateBuffer;
	AI2tGoalState *		currentState;

	// knowledge and alert
	AI2tKnowledgeState	knowledgeState;
	UUtUns32			alertTimer;
	UUtUns32			alertVocalizationTimer;
	UUtUns32			alertInitiallyRaisedTime;
	AI2tAlertStatus		alertStatus;
	AI2tAlertStatus		minimumAlert;
	AI2tAlertStatus		jobAlert;
	AI2tAlertStatus		investigateAlert;

	// startling
	UUtUns32			startleTimer;
	UUtUns32			startle_length;
	float				startle_startfacing;
	float				startle_deltafacing;

	// pursuit
	AI2tPursuitSettings pursuitSettings;
	UUtBool				has_job_location;
	M3tPoint3D			job_location;

	// alarms
	UUtUns32			alarmGroups;
	AI2tPersistentAlarmState alarmStatus;
	AI2tAlarmSettings	alarmSettings;

	// neutral behavior
	UUtUns32			neutralFlags;
	UUtUns32			neutralLastCheck;
	UUtUns32			neutralDisableTimer;
	UUtUns16			neutralBehaviorID;
	UUtUns16			neutralResumeAtLine;
	float				neutralTriggerRange;
	float				neutralEnemyRange;
	char *				neutralPendingScript;

	// combat
	UUtBool				already_done_daze;
	UUtUns32			daze_timer;
	UUtUns32			regenerated_amount;
	AI2tCombatSettings	combatSettings;
	AI2tMeleeProfile *	meleeProfile;
	UUtUns32			meleeDelayTimers[AI2cMeleeMaxDelayTimers];

} AI2tState;

// ------------------------------------------------------------------------------------
// -- globals

extern UUtBool AI2gLastManStanding;
extern UUtBool AI2gUltraMode;

// ------------------------------------------------------------------------------------
// -- major interface functions to oni

UUtError			AI2rInitialize(void);

void				AI2rTerminate(void);

void				AI2rUpdateAI(void);

UUtError			AI2rLevelBegin(void);

void				AI2rLevelEnd(void);



// ------------------------------------------------------------------------------------
// -- state changing code

void				AI2rEnterState(ONtCharacter *ioCharacter);

void				AI2rExitState(ONtCharacter *ioCharacter);

void				AI2rReturnToJob(ONtCharacter *ioCharacter, UUtBool inSetIdle, UUtBool inEvenIfAlerted);


// ------------------------------------------------------------------------------------
// -- character creation and deletion

UUtError			AI2rStartAllCharacters(UUtBool inStartPlayer, UUtBool inOverride);

UUtError			AI2rSpawnCharacter(char *inName, UUtBool inForce);

void				AI2rDeleteAllCharacters(UUtBool inDoPlayer);

// find a character with a particular name - used by the scripting system
ONtCharacter *		AI2rFindCharacter(const char *inCharacterName, UUtBool inExhaustive);

UUtError			AI2rInitializeCharacter(ONtCharacter *ioCharacter, const struct OBJtOSD_Character *inStartData,
											 const AItCharacterSetup *inSetup, const ONtFlag *inFlag, UUtBool inUseDefaultMelee);

// recreate the player character
ONtCharacter *		AI2rRecreatePlayer(void);

// delete AI2 internal state for a character
void				AI2rDeleteState(ONtCharacter *ioCharacter);

// smite one or many AIs
void				AI2rSmite(ONtCharacter *inCharacter, UUtBool inSpareCharacter);


// ------------------------------------------------------------------------------------
// -- query functions

// will a character be hostile towards another given character?
UUtBool AI2rCharacter_PotentiallyHostile(ONtCharacter *inCharacter, ONtCharacter *inTarget, UUtBool inCheckPreviousKnowledge);

// team IFF status
UUtUns32 AI2rTeam_FriendOrFoe(UUtUns16 inTeam, UUtUns16 inTarget);

// should a character try to pathfind through another character?
UUtBool AI2rTryToPathfindThrough(ONtCharacter *ioCharacter, ONtCharacter *inObstruction);

// is the AI trying to attack with the keys that it's currently pressing?
UUtBool AI2rTryingToAttack(ONtCharacter *ioCharacter);

// is the AI trying to throw with its current attack?
UUtBool AI2rTryingToThrow(ONtCharacter *ioCharacter);

// is an AI attacking someone?
ONtCharacter *AI2rGetCombatTarget(ONtCharacter *ioCharacter);

// is a character hurrying?
UUtBool AI2rIsHurrying(ONtCharacter *inCharacter);

// can we play an idle animation?
UUtBool AI2rEnableIdleAnimation(ONtCharacter *ioCharacter);

// get the number of frames to stay knocked down
UUtUns16 AI2rStartDazeTimer(ONtCharacter *ioCharacter);

// is a character casual?
UUtBool AI2rIsCasual(ONtCharacter *ioCharacter);

// is the AI doing something that means it shouldn't be deactivated?
UUtBool AI2rCurrentlyActive(ONtCharacter *ioCharacter);

// do we have a neutral interaction available?
UUtBool AI2rTryNeutralInteraction(ONtCharacter *ioCharacter, UUtBool inActivate);

// are we allowed to beat up on this character right now?
UUtBool AI2rAllowedToAttack(ONtCharacter *inCharacter, ONtCharacter *inTarget);

// ------------------------------------------------------------------------------------
// -- notify functions

// a character has been killed
void AI2rNotify_Death(ONtCharacter *ioCharacter, ONtCharacter *inKiller);

// a character has been hurt
void AI2rNotify_Damage(ONtCharacter *ioCharacter, UUtUns32 inHitPoints, UUtUns32 inDamageOwner);

// a character has fired
void AI2rNotifyAIFired(ONtCharacter *ioCharacter);

// a melee profile has been modified or deleted and all characters using it must be notified
void AI2rNotifyMeleeProfile(AI2tMeleeProfile *inProfile, UUtBool inDeleted);

// the AI has been knocked down
void AI2rNotifyKnockdown(ONtCharacter *ioCharacter);

// an attack has landed; if the defender was blocking, they should stop now
void AI2rNotifyAttackLanded(ONtCharacter *inAttacker, ONtCharacter *inDefender);

// a character has reached the action frame of its animation
void AI2rNotifyActionFrame(ONtCharacter *ioCharacter, UUtUns16 inAnimType);

// console action is complete
void AI2rNotifyConsoleAction(ONtCharacter *ioCharacter, UUtBool inSuccess);

// ------------------------------------------------------------------------------------
// -- command functions

// if we are dazing, snap out of it
void AI2rStopDaze(ONtCharacter *ioCharacter);

// an AI is playing a victim animation (and must stop its melee technique)
void AI2rStopMeleeTechnique(ONtCharacter *ioCharacter);

// we've just heard something - pause for a few seconds if we're not in an active goal state
UUtBool AI2rPause(ONtCharacter *ioCharacter, UUtUns32 inFrames);

// callback to see if we choose to block a given attack
UUtBool AI2rBlockFunction(ONtCharacter *inCharacter);

// set all characters to be passive
void AI2rAllPassive(UUtBool inPassive);

// set a character to be passive
void AI2rPassive(ONtCharacter *ioCharacter, UUtBool inPassive);

// say something
UUtBool AI2rVocalize(ONtCharacter *ioCharacter, UUtUns32 inVocalizationType, UUtBool inForce);

// ------------------------------------------------------------------------------------
// -- debugging / temporary code

// called once every frame by display code
void				AI2rDisplayGlobalDebuggingInfo(void);

// called every display update by character code
void				AI2rDisplayDebuggingInfo(ONtCharacter *ioCharacter);

// tell one or all characters to come to the player
void				AI2rDebug_ComeHere(ONtCharacter *ioCharacter);

// tell a character to look at the player - this overrides their patrol looking commands
void				AI2rDebug_LookHere(ONtCharacter *ioCharacter);

// tell one or all characters to follow the player
void				AI2rDebug_FollowMe(ONtCharacter *ioCharacter);

// tell a character to report in (NULL = all characters)
void				AI2rReport(ONtCharacter *ioCharacter, UUtBool inVerbose, AI2tReportFunction inFunction);

// make one or many AIs forget
void				AI2rForget(ONtCharacter *inCharacter, ONtCharacter *forgetCharacter);

// drop into the debugger next update loop
void				AI2rDebug_BreakPoint(void);
#endif // ONI_AI2_H