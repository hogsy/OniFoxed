#pragma once
#ifndef ONI_AI2_COMBAT_H
#define ONI_AI2_COMBAT_H

/*
	FILE:	Oni_AI2_Combat.h
	
	AUTHOR:	Chris Butcher
	
	CREATED: April 20, 2000
	
	PURPOSE: Combat Manager for Oni AI system
	
	Copyright (c) 2000

*/

#include "Oni_AI2_Knowledge.h"
#include "Oni_AI2_Targeting.h"
#include "Oni_AI2_Maneuver.h"
#include "Oni_AI2_Melee.h"

// ------------------------------------------------------------------------------------
// -- constants

// ranges of engagement
typedef enum AI2tCombatRange {
	AI2cCombatRange_None			= 0,
	AI2cCombatRange_Long			= 1,
	AI2cCombatRange_Medium			= 2,
	AI2cCombatRange_Short			= 3,
	AI2cCombatRange_MediumRetreat	= 4,
	AI2cCombatRange_LongRetreat		= 5,

	AI2cCombatRange_Max				= 6,
	AI2cCombatRange_NumStoredRanges	= AI2cCombatRange_Max - 1
} AI2tCombatRange;

// combat behaviors
typedef enum AI2tBehaviorType {
	AI2cBehavior_None				= 0,
	AI2cBehavior_Stare				= 1,
	AI2cBehavior_HoldAndFire		= 2,
	AI2cBehavior_FiringCharge		= 3,
	AI2cBehavior_Melee				= 4,
	AI2cBehavior_BarabbasShoot		= 5,
	AI2cBehavior_BarabbasAdvance	= 6,
	AI2cBehavior_BarabbasMelee		= 7,
	AI2cBehavior_SuperNinjaFireball	= 8,
	AI2cBehavior_SuperNinjaAdvance	= 9,
	AI2cBehavior_SuperNinjaMelee	= 10,
	AI2cBehavior_RunForAlarm		= 11,
	AI2cBehavior_MutantMuroMelee	= 12,
	AI2cBehavior_MutantMuroZeus		= 13,

	AI2cBehavior_Max				= 14
} AI2tBehaviorType;

// combat messages
typedef enum AI2tCombatMessage {
	AI2cCombatMessage_BeginBehavior		= 0,
	AI2cCombatMessage_PrimaryMovement	= 1,
	AI2cCombatMessage_EndBehavior		= 2,
	AI2cCombatMessage_Report			= 3,
	AI2cCombatMessage_SecondaryMovement	= 4,
	AI2cCombatMessage_Targeting			= 5,
	AI2cCombatMessage_Fallen			= 6,
	AI2cCombatMessage_TooClose			= 7,
	AI2cCombatMessage_Fire				= 8,
	AI2cCombatMessage_LookAtTarget		= 9,
	AI2cCombatMessage_Hurt				= 10,
	AI2cCombatMessage_ActionFrame		= 11,
	AI2cCombatMessage_NewAnimation		= 12,
	AI2cCombatMessage_FallDetected		= 13,
	AI2cCombatMessage_FoundPrimaryMovement= 14,
	AI2cCombatMessage_SwitchBehavior	= 15,
	AI2cCombatMessage_ShieldCharging	= 16,

	AI2cCombatMessage_Max				= 17
} AI2tCombatMessage;

// when to fight rather than doing non-melee setting
typedef enum AI2tFightType {
	AI2cFight_Never					= 0,
	AI2cFight_IfPunched				= 1,
	AI2cFight_IfAttackOpportunity	= 2,
	AI2cFight_IfShortRange			= 3,
	AI2cFight_IfMediumRange			= 4,
	AI2cFight_Always				= 5,

	AI2cFight_Max					= 6
} AI2tFightType;

// what to do if no gun is available
typedef enum AI2tNoGunBehavior {
	AI2cNoGun_Melee						= 0,
	AI2cNoGun_Retreat					= 1,
	AI2cNoGun_Alarm						= 2,

	AI2cNoGun_Max						= 3
} AI2tNoGunBehavior;

extern const char *AI2cCombatRangeName[];
extern const char *AI2cBehaviorName[];
extern const char *AI2cFightName[];
extern const char *AI2cNoGunName[];

#define AI2cCombat_UpdateFrames						5

// default combat settings
#define AI2cCombatSettings_DefaultShortRange		40.0f
#define AI2cCombatSettings_DefaultMediumRange		200.0f
#define AI2cCombatSettings_DefaultBehavior			AI2cBehavior_HoldAndFire
#define AI2cCombatSettings_DefaultMeleeWhen			AI2cFight_IfShortRange
#define AI2cCombatSettings_DefaultNoGun				AI2cNoGun_Melee
#define AI2cCombatSettings_DefaultPursuitDistance	200.0f

// how close to get when checking out characters
#define AI2cCombat_InvestigateInterpRange			30.0f
#define AI2cCombat_InvestigateMinRange				12.0f

#if TOOL_VERSION
// TEMPORARY DEBUGGING - buffer for storing LOS points in
#define AI2cCombat_LOSPointBufferSize		1000
extern M3tPoint3D AI2gCombat_LOSPoints[];
extern UUtBool AI2gCombat_LOSClear[];
extern UUtUns32 AI2gCombat_LOSUsedPoints, AI2gCombat_LOSNextPoint;
#endif

// ------------------------------------------------------------------------------------
// -- structure definitions

/*
 * individual behavior parameter definitions - stored in AI2tBehavior
 * at the class level
 */

// currently no behaviors that require parameters

// holds a character class's parameters for its combat behavior
typedef tm_struct AI2tCombatParameters
{
	// parameters for weapon prediction, aiming, and firing
	AI2tTargetingParameters		targeting_params;

	// shooting skill with various weapons... index is shootskill_index stored in weapon class
	AI2tShootingSkill			shooting_skill[13];		// must match AI2cCombat_MaxWeapons

	// delay frames in switching away from dead characters, and loss of definite contact
	UUtUns32					dead_makesure_delay;
	UUtUns32					investigate_body_delay;
	UUtUns32					lost_contact_delay;
	UUtUns32					dead_taunt_percentage;

	// parameters for picking up guns
	UUtUns32					gun_pickup_percentage;
	UUtUns32					gun_runningpickup_percentage;

} AI2tCombatParameters;

/*
 * behavior-switching definitions
 */

// combat parameters for an individual AI character - stored in ai2State even
// when not in combat mode
typedef struct AI2tCombatSettings {
	UUtUns16					flags;
	UUtUns16					pad;
	float						medium_range;
	float						short_range;
	AI2tBehaviorType			behavior[AI2cCombatRange_Max];
	AI2tFightType				melee_when;
	AI2tNoGunBehavior			no_gun;
	float						pursuit_distance;
	UUtUns32					panic_melee;
	UUtUns32					panic_gunfire;
	UUtUns32					panic_sight;
	UUtUns32					panic_hurt;

	// FIXME: additional parameters:
	//    how to use hypos?
} AI2tCombatSettings;

// a combat behavior is a function that accepts messages from the rest of the AI system,
// and calls combat actions as necessary.
typedef UUtUns32 (*AI2tBehaviorFunction)(ONtCharacter *ioCharacter, struct AI2tCombatState *ioCombatState,
										 AI2tCombatMessage inMsg, UUtBool *inHandled,
										 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3);

typedef struct AI2tBehaviorDefinition {
	AI2tBehaviorType			type;
	AI2tBehaviorFunction		function;
} AI2tBehaviorDefinition;

extern const AI2tBehaviorDefinition AI2gBehaviorTable[];

/*
 * current state storage
 */

typedef struct AI2tBehaviorState_Stare {
	UUtUns32	pad;
} AI2tBehaviorState_Stare;

typedef struct AI2tBehaviorState_HoldAndFire {
	UUtUns32	pad;
} AI2tBehaviorState_HoldAndFire;

typedef struct AI2tBehaviorState_FiringCharge {
	UUtUns32	pad;
} AI2tBehaviorState_FiringCharge;

typedef struct AI2tBehaviorState_Melee {
	UUtUns32	pad;
} AI2tBehaviorState_Melee;

typedef struct AI2tBehaviorState_RunForAlarm {
	UUtBool		tried_alarm;
} AI2tBehaviorState_RunForAlarm;

// barabbas' behavior modes
enum {
	AI2cBarabbas_None				= 0,
	AI2cBarabbas_ShootBeam			= 1,
	AI2cBarabbas_WaitForWeapon		= 2,
	AI2cBarabbas_Advance			= 3,
	AI2cBarabbas_AdvanceGrenading	= 4,
	AI2cBarabbas_GetGun				= 5,
	AI2cBarabbas_Regenerate			= 6,
	AI2cBarabbas_Melee				= 7,
	AI2cBarabbas_Flee				= 8,

	AI2cBarabbas_Max				= 9
};

typedef struct AI2tBehaviorState_Barabbas {
	UUtUns32	barabbas_mode;
	UUtUns32	prev_barabbas_mode;
	UUtUns32	disable_beam_timer;
	UUtUns32	regen_damage_taken;
	UUtUns32	regen_disable_timer;
	UUtUns32	fleeing_timer;
} AI2tBehaviorState_Barabbas;

// superninja's behavior modes
enum {
	AI2cSuperNinja_None				= 0,
	AI2cSuperNinja_Melee			= 1,
	AI2cSuperNinja_Fireball			= 2,
	AI2cSuperNinja_Advance			= 3,
	AI2cSuperNinja_Invisible		= 4,
	AI2cSuperNinja_TeleportFlee		= 5,
	AI2cSuperNinja_TeleportAdvance	= 6,
	AI2cSuperNinja_TeleportFromFall	= 7,

	AI2cSuperNinja_Max				= 8
};

typedef struct AI2tBehaviorState_SuperNinja {
	// mode switching
	UUtUns32	superninja_mode;
	UUtUns32	prev_superninja_mode;

	// super move timers and flag variables
	UUtUns32	disable_fireball_timer;
	UUtUns32	disable_invis_timer;
	UUtUns32	disable_teleport_timer;
	UUtUns32	last_teleport_time;
	UUtBool		fireball_started;
	UUtBool		invis_started;
	UUtBool		teleport_started;
	
	// medium-range controls
	UUtBool		set_mediumrange_behavior;
	UUtBool		mediumrange_enable_teleport;

	// fleeing controls
	UUtUns32	flee_tick_counter;
	UUtUns32	flee_damagestay_timer;
	UUtUns32	flee_damage_taken;

	// falling controls
	UUtUns32	fall_timer;
} AI2tBehaviorState_SuperNinja;

typedef struct AI2tBehaviorState_MutantMuro {
	// special attack timers
	UUtUns32	beam_maximum_timer;
	UUtUns32	beam_continue_timer;
	UUtUns32	beam_disable_timer;

	// super shield control
	UUtUns32	shield_decay_max;
	UUtUns32	shield_decay_timer;
	UUtUns32	shield_disable_timer;
	UUtUns32	shield_regen_timer;
	UUtUns32	shield_regen_max;
	UUtUns32	shield_active_frames;

	// mode switching control
	UUtUns32	total_damage;
	UUtUns32	total_damage_decay_timer;

} AI2tBehaviorState_MutantMuro;

typedef struct AI2tCombatState {
	// global variables for all combat behaviors
	ONtCharacter *						target;
	UUtBool								target_dead;
	AI2tKnowledgeEntry *				target_knowledge;
	AI2tContactStrength					target_strength;		// will be AI2cContactStrength_Definite always unless
																// we are currently running down our lost_contact_timer

	// current behavior
	float								distance_to_target;
	AI2tCombatRange						current_range;
	AI2tBehaviorDefinition				current_behavior;
	AI2tCombatParameters *				combat_parameters;		// pointer to character class's parameters

	union {
		AI2tBehaviorState_Stare				stare;
		AI2tBehaviorState_HoldAndFire		holdAndFire;
		AI2tBehaviorState_FiringCharge		firingCharge;
		AI2tBehaviorState_Melee				melee;
		AI2tBehaviorState_Barabbas			barabbas;
		AI2tBehaviorState_SuperNinja		superNinja;
		AI2tBehaviorState_RunForAlarm		alarm;
		AI2tBehaviorState_MutantMuro		mutantMuro;
	} behavior_state;

	/*
	 * control variables set by behavior
	 */

	// line-of-sight determination - what constitutes blocked LOS?
	UUtBool								desire_env_los;
	UUtBool								desire_friendly_los;

	// targeting
	UUtBool								aim_weapon;
	UUtBool								shoot_weapon;
	UUtBool								alternate_fire;


	// ======================
	// == internal state

	/*
	 * behavior control and modification
	 */

	UUtUns32							weaponcaused_melee_timer;
	UUtUns32							retreatfailed_melee_timer;
	UUtUns32							retreat_frames;
	UUtBool								melee_override;
	UUtBool								guncheck_set;
	UUtBool								guncheck_enable;
	UUtBool								runningpickup_set;
	UUtBool								runningpickup_enable;

	/*
	 * knowledge
	 */

	UUtBool								dead_taunt_enable;
	UUtBool								dead_done_taunt;
	UUtUns32							dead_makesure_timer;
	UUtUns32							investigate_body_timer;
	UUtUns32							lost_contact_timer;
	UUtUns32							last_hit_by_target;
	UUtUns32							last_melee_hit_by_target;

	/*
	 * targeting
	 */

	UUtBool								run_targeting;
	AI2tTargetingState					targeting;
	WPtWeapon							*current_weapon;
	UUtBool								current_alternate_fire;
	UUtBool								trigger_pressed;
	UUtBool								alternate_trigger_pressed;

	/*
	 * maneuvering
	 */

	UUtBool								have_los;
	UUtBool								can_find_los;
	UUtBool								currently_dazed;
	UUtBool								no_longer_dazed;
	AI2tManeuverState					maneuver;
	UUtBool								dodge_enable;

	/*
	 * melee
	 */

	AI2tMeleeState						melee;
	UUtUns32							no_technique_timer;
	UUtUns32							try_advance_timer;

	/*
	 * special-case settings
	 */

	UUtBool								fall_detect_enable;
	float								fall_height;

} AI2tCombatState;

// ------------------------------------------------------------------------------------
// -- globals

extern AI2tTargetingCallbacks AI2gCombatAI_TargetingCallbacks;
extern AI2tTargetingCallbacks AI2gPatrolAI_TargetingCallbacks;

// ------------------------------------------------------------------------------------
// -- high-level combat manager control

// set up a combat state - called when the character goes into "combat mode".
void AI2rCombat_Enter(ONtCharacter *ioCharacter);

// exit a combat state - called when the character goes out of "combat mode".
void AI2rCombat_Exit(ONtCharacter *ioCharacter);

// abort a combat state - called when the character dies while in "combat mode".
void AI2rCombat_NotifyDeath(ONtCharacter *ioCharacter);

// called every tick while in combat
void AI2rCombat_Update(ONtCharacter *ioCharacter);

// change targets
void AI2rCombat_SetNewTarget(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
									  AI2tKnowledgeEntry *inEntry, UUtBool inInitialTarget);

// check to see if we lost our knowledge about our target
UUtBool AI2rCombat_NotifyKnowledgeRemoved(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
											AI2tKnowledgeEntry *inEntry);

// is a new range longer than our current one?
UUtBool AI2rCombat_RangeIsLonger(AI2tCombatRange inRange, AI2tCombatRange inNewRange);

// get current range (note: does NOT use 'retreat', will only ever be Long, Medium or Short)
AI2tCombatRange AI2rCombat_GetRange(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
									AI2tCombatRange inOldRange);

// new-knowledge notification callback for the combat state
void AI2rCombat_NotifyKnowledge(ONtCharacter *ioCharacter, AI2tKnowledgeEntry *inEntry,
								UUtBool inDegrade);

// handle a message that wasn't handled by our current behavior
UUtUns32 AI2rBehavior_Default(ONtCharacter *ioCharacter, struct AI2tCombatState *ioCombatState,
							   AI2tCombatMessage inMsg, UUtBool *inHandled,
							   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3);

// handle whether or not we must wait for our super shield to recharge
UUtBool AI2rCombat_WaitingForShieldCharge(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState);

// ------------------------------------------------------------------------------------
// -- debugging commands

// report in
void AI2rCombat_Report(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
					   UUtBool inVerbose, AI2tReportFunction inFunction);

// ------------------------------------------------------------------------------------
// -- inline functions

// call a behavior with a particular message
static UUcInline UUtUns32 AI2rCombat_Behavior(ONtCharacter *ioCharacter, struct AI2tCombatState *ioCombatState,
										  AI2tCombatMessage inMsg, UUtBool *inHandled,
										  UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3)
{
	UUtUns32 returnval;
	AI2tBehaviorFunction function = ioCombatState->current_behavior.function;
	
	if (function == NULL) {
		*inHandled = UUcFalse;
		return 0;
	}

	returnval = function(ioCharacter, ioCombatState, inMsg, inHandled, inParam1, inParam2, inParam3);

	if (*inHandled)
		return returnval;
	else
		return AI2rBehavior_Default(ioCharacter, ioCombatState, inMsg, inHandled, inParam1, inParam2, inParam3);
}

#endif  // ONI_AI2_COMBAT_H
