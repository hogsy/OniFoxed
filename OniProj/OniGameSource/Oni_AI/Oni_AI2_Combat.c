/*
	FILE:	Oni_AI2_Combat.c
	
	AUTHOR:	Chris Butcher
	
	CREATED: April 20, 2000
	
	PURPOSE: Combat Manager for Oni AI system
	
	Copyright (c) 2000

*/

#include "bfw_math.h"

#include "Oni_AI2.h"
#include "Oni_Character.h"
#include "Oni_Character_Animation.h"
#include "Oni_Aiming.h"

// ------------------------------------------------------------------------------------
// -- global constants and macros

#define AI_DEBUG_COMBAT												0
#define AI_DEBUG_BARABBAS											0
#define AI_DEBUG_SUPERNINJA											0
#define AI_DEBUG_MUTANTMURO											0

#define AI2cCombat_LOS_FriendlyClearRadius							4.0f
#define AI2cCombat_LOS_FriendlyClearHeight							8.0f

#define AI2cCombat_Targeting_SwitchDistanceRatio					0.8f
#define AI2cCombat_Targeting_DontSwitchDistanceRatio				3.5f
#define AI2cCombat_Targeting_SwitchIfNotHit							150
#define AI2cCombat_Targeting_AttackMeleeShooterUnlessAlsoHitTime	90

#define AI2cCombat_StartleAimingMaxSpeed							0.7f

#define AI2cCombat_LOSMinRange										12.0f
#define AI2cCombat_LOSInterpRange									30.0f

#define AI2cCombat_NoLOSMeleeRange									(1.5f * AI2cCombat_LOSMinRange)
#define AI2cCombat_MeleeNoTechniqueTimer							30
#define AI2cCombat_MeleeTryAdvanceTimer								90
#define AI2cCombat_MeleeRecentlyHurtTimer							240
#define AI2cCombat_TooClose_MeleeTimer								120
#define AI2cCombat_TooClose_MaxRetreatTimer							180
#define AI2cCombat_TooClose_RetreatFailedMeleeTimer					300

#define AI2cCombat_BallisticLOS_StepSize							20.0f

#define AI2cBarabbas_StationaryBeamTimer							180
#define AI2cBarabbas_AdvancingBeamTimer								360
#define AI2cBarabbas_AdvancingBeamDeltaTimer						90
#define AI2cBarabbas_RegenHurtDisableTimerMax						180
#define AI2cBarabbas_RegenHurtDisableTimerMin						90
#define AI2cBarabbas_RegenDamageStopFraction						5
#define AI2cBarabbas_HealthCriticalFraction							15
#define AI2cBarabbas_RegenHealthFraction							40
#define AI2cBarabbas_RegenFullFraction								80
#define AI2cBarabbas_RegenRestartDisableTimer						240
#define AI2cBarabbas_RegenAbortDistance								20.0f
#define AI2cBarabbas_RegenMinDistance								40.0f
#define AI2cBarabbas_RegenFleeReqDistance							80.0f
#define AI2cBarabbas_RegenHealFractionPerSecond						8
#define AI2cBarabbas_RegenCantFleeDisableTimer						600
#define AI2cBarabbas_RegenMaxHitPointPool							100
#define AI2cBarabbas_FleeMaxTimer									240
#define AI2cBarabbas_FleeMinDist									30.0f
#define AI2cBarabbas_FleeMaxDist									100.0f
#define AI2cBarabbas_GunMinDist										100.0f
#define AI2cBarabbas_GunMaxDist										350.0f
#define AI2cBarabbas_MeleeInsteadOfGunRange							30.0f

#define AI2cSuperNinja_UnsuccessfulSpecialMoveTimer					300
#define AI2cSuperNinja_FireballTimer								1200
#define AI2cSuperNinja_InvisMinDistance								40.0f
#define AI2cSuperNinja_InvisibleDisableTimer						1200
#define AI2cSuperNinja_InvisibilityAmount							(10 * 60)
#define AI2cSuperNinja_FallTeleportHeight							550.0f
#define AI2cSuperNinja_FallTeleportFrames							150
#define AI2cSuperNinja_DamageDecayAmount							3
#define AI2cSuperNinja_DamageDecayFrames							30
#define AI2cSuperNinja_DamageStayFrames								240
#define AI2cSuperNinja_FleeTeleportDamage							35
#define AI2cSuperNinja_FleeTeleportRange							50.0f
#define AI2cSuperNinja_AdvanceTeleportRange							80.0f
#define AI2cSuperNinja_AdvanceTeleportChance						0.5f
#define AI2cSuperNinja_MinimumTeleportDelay							600
#define AI2cSuperNinja_TeleportBehindDistance						35.0f
#define AI2cSuperNinja_TeleportBehindMinDistance					15.0f
#define AI2cSuperNinja_TeleportEdgeRandomAngle						(30.0f * M3cDegToRad)
#define AI2cSuperNinja_TeleportEdgeMoveDistance						40.0f
#define AI2cSuperNinja_TeleportRetreatAngleDelta					(15.0f * M3cDegToRad)
#define AI2cSuperNinja_TeleportRetreatRange							200.0f
#define AI2cSuperNinja_TeleportRetreatRangeDelta					100.0f
#define AI2cSuperNinja_EdgeFlagBase									9000
#define AI2cSuperNinja_RetreatFlagBase								9500

#define AI2cMutantMuro_ZeusContinueTimer							120
#define AI2cMutantMuro_ZeusDisableTimer								300
#define AI2cMutantMuro_ZeusTargetDistance							60.0f
#define AI2cMutantMuro_ZeusDisableDistance							40.0f
#define AI2cMutantMuro_ZeusMaximumTimer								720
#define AI2cMutantMuro_ShieldRegenTimer								240
#define AI2cMutantMuro_ShieldZeusDecay								90
#define AI2cMutantMuro_ShieldZeusDisable							600
#define AI2cMutantMuro_ShieldAttackDecay							30
#define AI2cMutantMuro_ShieldAttackDisable							360
#define AI2cMutantMuro_ShieldRequiredFrames							360
#define AI2cMutantMuro_DamageThresholdStopZeus						35
#define AI2cMutantMuro_DamageThresholdTurtle						11
#define AI2cMutantMuro_TurtleForgetTimer							75
#define AI2cMutantMuro_TurtleTargetDistance							30.0f

const char *AI2cCombatRangeName[AI2cCombatRange_Max] =
	{"None", "Long", "Medium", "Short", "Medium Retreat", "Long Retreat"};

const char *AI2cBehaviorName[AI2cBehavior_Max] =
	{"None", "Stare", "Hold And Fire", "Firing Charge", "Melee",
	 "Barabbas Shoot", "Barabbas Advance", "Barabbas Melee", "SNinja Fireball", "SNinja Advance", "SNinja Melee",
	 "Run For Alarm", "MMuro Melee", "MMuro Thunderbolt"};

const char *AI2cFightName[AI2cFight_Max] =
	{"No", "If Punched", "---", "Short Range", "Medium Range", "Always Melee"};

const char *AI2cNoGunName[AI2cNoGun_Max] =
	{"Melee", "Retreat", "Run To Alarm"};

const AI2tCombatRange AI2cFightRange[AI2cFight_Max] =
	{AI2cCombatRange_None, AI2cCombatRange_None, AI2cCombatRange_None, AI2cCombatRange_Short,
		AI2cCombatRange_Medium, AI2cCombatRange_Long};

const float AI2cCombatLookRange[AI2cPrimaryMovement_Max] =
	{80.0f,		// hold
	120.0f,		// advance
	140.0f,		// retreat
	 80.0f,		// gun
	 60.0f,		// alarm
	140.0f,		// melee
	160.0f};	// getup

#if TOOL_VERSION
// TEMPORARY DEBUGGING - buffer for storing LOS points in
M3tPoint3D AI2gCombat_LOSPoints[AI2cCombat_LOSPointBufferSize];
UUtBool AI2gCombat_LOSClear[AI2cCombat_LOSPointBufferSize];
UUtUns32 AI2gCombat_LOSUsedPoints, AI2gCombat_LOSNextPoint;
#endif

// ------------------------------------------------------------------------------------
// -- internal function prototypes

// behavior control
static void AI2iCombat_ChangeBehavior(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState);	
static void AI2iCombat_RevertToBasicBehavior(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState);

// contact and knowledge handling
static void AI2iCombat_ClearTarget(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState);
static void AI2rCombat_CheckTargetingWeapon(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState);
static UUtBool AI2iCombat_CheckTarget(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState);
static void AI2iCombat_TargetDied(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState);
static AI2tKnowledgeEntry *AI2iCombat_FindNewTarget(ONtCharacter *ioCharacter, AI2tContactStrength inRequiredStrength);
static void	AI2iCombat_LostContact(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState);
static void AI2iCombat_Pursue(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState);

// LOS
static void AI2iCombat_PerformLOS(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState);

// targeting control
static void AI2iCombat_Trigger(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState, UUtBool inTrigger);

// spacing behavior and fight control
static void AI2iCombat_RemoveFromFight(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState);

// shared behavior functions
static UUtUns32 AI2iTryToFindGunBehavior(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3);
static UUtUns32 AI2iNoGunBehavior(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3);

// specific behavior functions
static UUtUns32 AI2iBehavior_Stare(ONtCharacter *ioCharacter, struct AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3);
static UUtUns32 AI2iBehavior_HoldAndFire(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3);
static UUtUns32 AI2iBehavior_FiringCharge(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3);
static UUtUns32 AI2iBehavior_Melee(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3);
static UUtUns32 AI2iBehavior_RunForAlarm(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3);

// barabbas's special case AI functions
static UUtBool AI2iBehavior_BarabbasCheckRegen(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
											   AI2tBehaviorState_Barabbas *ioBarabbasState);
static UUtUns32 AI2iBehavior_BarabbasMelee(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3);
static UUtUns32 AI2iBehavior_BarabbasShoot(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3);
static UUtUns32 AI2iBehavior_BarabbasAdvance(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3);

// super ninja's special case AI functions
static UUtUns32 AI2iBehavior_SuperNinjaMelee(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3);
static UUtUns32 AI2iBehavior_SuperNinjaFireball(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3);
static UUtUns32 AI2iBehavior_SuperNinjaAdvance(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3);

// mutant muro's special case AI functions
static UUtUns32 AI2iBehavior_MutantMuroMelee(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3);
static UUtUns32 AI2iBehavior_MutantMuroZeus(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3);

// ------------------------------------------------------------------------------------
// -- global combat behavior table

const AI2tBehaviorDefinition AI2gBehaviorTable[AI2cBehavior_Max] =
{
	{AI2cBehavior_None,					NULL},
	{AI2cBehavior_Stare,				AI2iBehavior_Stare},
	{AI2cBehavior_HoldAndFire,			AI2iBehavior_HoldAndFire},
	{AI2cBehavior_FiringCharge,			AI2iBehavior_FiringCharge},
	{AI2cBehavior_Melee,				AI2iBehavior_Melee},
	{AI2cBehavior_BarabbasShoot,		AI2iBehavior_BarabbasShoot},
	{AI2cBehavior_BarabbasAdvance,		AI2iBehavior_BarabbasAdvance},
	{AI2cBehavior_BarabbasMelee,		AI2iBehavior_BarabbasMelee},
	{AI2cBehavior_SuperNinjaFireball,	AI2iBehavior_SuperNinjaFireball},
	{AI2cBehavior_SuperNinjaAdvance,	AI2iBehavior_SuperNinjaAdvance},
	{AI2cBehavior_SuperNinjaMelee,		AI2iBehavior_SuperNinjaMelee},
	{AI2cBehavior_RunForAlarm,			AI2iBehavior_RunForAlarm},
	{AI2cBehavior_MutantMuroMelee,		AI2iBehavior_MutantMuroMelee},
	{AI2cBehavior_MutantMuroZeus,		AI2iBehavior_MutantMuroZeus}
};

// ------------------------------------------------------------------------------------
// -- targeting callbacks for character AIs

static void AI2iCombatAI_TargetingCallback_Fire(AI2tTargetingState *inTargetingState, UUtBool inFire);
static void AI2iAI_TargetingCallback_SetAimSpeed(AI2tTargetingState *inTargetingState, float inSpeed);
static void AI2iAI_TargetingCallback_ForceAimWithWeapon(AI2tTargetingState *inTargetingState, UUtBool inWithWeapon);
static void AI2iAI_TargetingCallback_EndForceAim(AI2tTargetingState *inTargetingState);
static void AI2iAI_TargetingCallback_GetPosition(AI2tTargetingState *inTargetingState, M3tPoint3D *inPoint);
static void AI2iAI_TargetingCallback_AimVector(AI2tTargetingState *inTargetingState, M3tVector3D *inDirection);
static void AI2iAI_TargetingCallback_AimPoint(AI2tTargetingState *inTargetingState, M3tPoint3D *inPoint);
static void AI2iAI_TargetingCallback_AimCharacter(AI2tTargetingState *inTargetingState, ONtCharacter *inTarget);
static void AI2iAI_TargetingCallback_UpdateAiming(AI2tTargetingState *inTargetingState);

AI2tTargetingCallbacks AI2gCombatAI_TargetingCallbacks = {
	AI2iCombatAI_TargetingCallback_Fire,
	AI2iAI_TargetingCallback_SetAimSpeed,
	AI2iAI_TargetingCallback_ForceAimWithWeapon,
	AI2iAI_TargetingCallback_EndForceAim,
	AI2iAI_TargetingCallback_GetPosition,
	AI2iAI_TargetingCallback_AimVector,
	AI2iAI_TargetingCallback_AimPoint,
	AI2iAI_TargetingCallback_AimCharacter,
	AI2iAI_TargetingCallback_UpdateAiming
};

AI2tTargetingCallbacks AI2gPatrolAI_TargetingCallbacks = {
	AI2rPatrolAI_TargetingCallback_Fire,
	AI2iAI_TargetingCallback_SetAimSpeed,
	AI2iAI_TargetingCallback_ForceAimWithWeapon,
	AI2iAI_TargetingCallback_EndForceAim,
	AI2iAI_TargetingCallback_GetPosition,
	AI2iAI_TargetingCallback_AimVector,
	AI2iAI_TargetingCallback_AimPoint,
	AI2iAI_TargetingCallback_AimCharacter,
	AI2iAI_TargetingCallback_UpdateAiming
};

// ------------------------------------------------------------------------------------
// -- high-level combat manager control

// set up a combat state - called when the character goes into "combat mode".
void AI2rCombat_Enter(ONtCharacter *ioCharacter)
{
	AI2tCombatState *combat_state;
	AI2tTargetingOwner targeting_owner;
        
	UUmAssert(ioCharacter->charType == ONcChar_AI2);
	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Combat);
	combat_state = &ioCharacter->ai2State.currentState->state.combat;

#if AI_DEBUG_COMBAT
	COrConsole_Printf("%s: entering combat mode", ioCharacter->player_name);
#endif

        if (combat_state->target_knowledge == NULL) {
            AI2tKnowledgeEntry *entry = AI2iCombat_FindNewTarget(ioCharacter, AI2cContactStrength_Strong);
            if (entry != NULL)
                combat_state->target_knowledge = entry;
            else
                return; // Can't find any targets to fight
        }
//	UUrDebuggerMessage("AI2rCombat_Enter: target knowledge ptr is 0x%08X\n", (UUtUns32) combat_state->target_knowledge);

	// initialize combat state variables
	combat_state->combat_parameters = &ioCharacter->characterClass->ai2_behavior.combat_parameters;
	combat_state->currently_dazed = UUcFalse;
	combat_state->have_los = UUcFalse;
	combat_state->desire_env_los = UUcFalse;
	combat_state->desire_friendly_los = UUcFalse;
	combat_state->aim_weapon = UUcFalse;
	combat_state->shoot_weapon = UUcFalse;
	combat_state->run_targeting = UUcTrue;
	combat_state->trigger_pressed = UUcFalse;
	combat_state->alternate_trigger_pressed = UUcFalse;
	combat_state->no_technique_timer = 0;
	combat_state->try_advance_timer = 0;
	
	// we don't respond to stimuli in the same way any more
	ioCharacter->ai2State.knowledgeState.callback = &AI2rCombat_NotifyKnowledge;

	// reset our movement mode to default - make sure we aren't walking or creeping!
	AI2rMovement_ChangeMovementMode(ioCharacter, AI2cMovementMode_Default);

	// set up our sub-states (maneuvering, melee, targeting)
	AI2rManeuverState_Clear(ioCharacter, combat_state);
	AI2rMeleeState_Clear(&combat_state->melee, UUcTrue);

	targeting_owner.type = AI2cTargetingOwnerType_Character;
	targeting_owner.owner.character = ioCharacter;
	AI2rTargeting_Initialize(targeting_owner, &combat_state->targeting, &AI2gCombatAI_TargetingCallbacks, NULL, NULL,
			&combat_state->combat_parameters->targeting_params, combat_state->combat_parameters->shooting_skill);

	combat_state->current_weapon = NULL;
	combat_state->alternate_fire = UUcFalse;
	combat_state->current_alternate_fire = UUcFalse;
	AI2rCombat_CheckTargetingWeapon(ioCharacter, combat_state);

	// clear the behavior-modification variables
	combat_state->weaponcaused_melee_timer = 0;
	combat_state->retreatfailed_melee_timer = 0;
	combat_state->retreat_frames = 0;
	combat_state->melee_override = UUcFalse;
	combat_state->guncheck_set = UUcFalse;
	combat_state->guncheck_enable = UUcFalse;
	combat_state->runningpickup_set = UUcFalse;
	combat_state->runningpickup_enable = UUcFalse;

	// set up the combat state
#if defined(DEBUGGING) && DEBUGGING
	{
		UUtUns32 itr;
		AI2tKnowledgeEntry *entry;

		UUmAssert(combat_state->target_knowledge != NULL);
		for (itr = 0, entry = ioCharacter->ai2State.knowledgeState.contacts; entry != NULL; entry = entry->nextptr, itr++) {
			if (entry == combat_state->target_knowledge) {
				break;
			}
		}
		UUmAssert(entry != NULL);
	}
#endif
	AI2rCombat_SetNewTarget(ioCharacter, combat_state, combat_state->target_knowledge,
								combat_state->target_knowledge->first_sighting);
	AI2rCombat_NotifyKnowledge(ioCharacter, combat_state->target_knowledge, UUcFalse);

	ioCharacter->ai2State.flags |= AI2cFlag_RunCombatScript;

	return;
}

// TEMPORARY DEBUGGING
extern UUtUns32 AI2gKnowledge_ClearSpaceIndex;
extern AI2tKnowledgeEntry *AI2gKnowledgeBase;

// called every tick while in combat
void AI2rCombat_Update(ONtCharacter *ioCharacter)
{
	AI2tCombatState *combat_state;
	AI2tFightState *fight_state;
	UUtUns16 startle_frame;
	UUtUns32 current_time;
	UUtBool looking_at_target, investigating_body, got_target, handled, able_to_melee, targeting_updated;
	UUtBool disable_shooting, disable_dodge, dodge_disable_firing, lock_into_behavior;
	float too_close_weight, max_range;
	AI2tCombatRange target_range, old_range;
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);

	UUmAssert(ioCharacter->charType == ONcChar_AI2);
	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Combat);
	UUmAssertReadPtr(active_character, sizeof(*active_character));
	combat_state = &ioCharacter->ai2State.currentState->state.combat;
	current_time = ONrGameState_GetGameTime();

	// TEMPORARY DEBUGGING
//	UUrDebuggerMessage("AI2rCombat_Update: target knowledge ptr is 0x%08X\n", (UUtUns32) combat_state->target_knowledge);

//	UUmAssert((combat_state->target_knowledge >= AI2gKnowledgeBase) && 
//				(combat_state->target_knowledge < AI2gKnowledgeBase + AI2gKnowledge_ClearSpaceIndex));

	// check that our target is still valid... may also change targets or drop us out of the combat manager
	got_target = AI2iCombat_CheckTarget(ioCharacter, combat_state);

	// AI2iCombat_CheckTarget and AI2iCombat_LostContact may cause us to transition out of
	// the combat state... if so then exit early.
	if (ioCharacter->ai2State.currentGoal != AI2cGoal_Combat)
		return;

	/*
	 * target acquisition and tracking
	 */

	too_close_weight = 0;
	AI2rCombat_CheckTargetingWeapon(ioCharacter, combat_state);
	if (combat_state->run_targeting) {
		targeting_updated = UUcTrue;
		AI2rTargeting_Update(&combat_state->targeting, got_target, combat_state->aim_weapon, combat_state->shoot_weapon,
							&too_close_weight);
	} else {
		targeting_updated = UUcFalse;
		if (combat_state->target != NULL) {
			MUmVector_Copy(combat_state->targeting.target_pt, combat_state->target->location);
			combat_state->targeting.target_pt.y += combat_state->target->heightThisFrame;
			combat_state->targeting.valid_target_pt = UUcTrue;
		}
		AI2rMovement_StopAiming(ioCharacter);
	}

	/*
	 * combat override detection
	 */

	if ((combat_state->fall_detect_enable) && (ioCharacter->location.y < combat_state->fall_height)) {
		if (AI2rCombat_Behavior(ioCharacter, combat_state, AI2cCombatMessage_FallDetected, &handled, 0, 0, 0)) {
			// our update has been handled by our falling behavior
			return;
		}
	}

	combat_state->maneuver.getup_direction = AI2cMovementDirection_Stopped;
	if (ONrAnimState_IsFallen(active_character->nextAnimState)) {
		// check to see if we are dazed from a fall
		if (AI2rCombat_Behavior(ioCharacter, combat_state, AI2cCombatMessage_Fallen, &handled, 0, 0, 0)) {
			// lie still
			AI2rPath_Halt(ioCharacter); 
			AI2rMovement_StopAiming(ioCharacter);
			AI2iCombat_Trigger(ioCharacter, combat_state, UUcFalse);
			return;
		}
	}

	/*
	 * PRIMARY MOVEMENT
	 *
	 * takes place once every AI2cCombat_UpdateFrames - uses fuzzy weights to determine a single primary
	 * goal for the combat state
	 */

	if (ioCharacter->index % AI2cCombat_UpdateFrames == (UUtInt32) (current_time % AI2cCombat_UpdateFrames)) {

		/*
		 * ranging and LOS
		 */

		old_range = combat_state->current_range;
		target_range = AI2rCombat_GetRange(ioCharacter, combat_state, old_range);

		if (target_range != old_range) {
			// check to see if we can switch behaviors
			lock_into_behavior = (UUtBool) AI2rCombat_Behavior(ioCharacter, combat_state, AI2cCombatMessage_SwitchBehavior, &handled,
																target_range, 0, 0);
			if (!lock_into_behavior) {
				if (AI2rCombat_RangeIsLonger(old_range, target_range)) {
					// we are moving out to a longer range, we can now check again to see if
					// we can pick up guns
					combat_state->guncheck_set = UUcFalse;
					combat_state->guncheck_enable = UUcFalse;
				}

				combat_state->current_range = target_range;

				AI2iCombat_ChangeBehavior(ioCharacter, combat_state);
			}
/*		COrConsole_Printf("%s: combat range %s -> %s, new behavior %s", ioCharacter->player_name, 
			AI2cCombatRangeName[old_range], AI2cCombatRangeName[target_range],
			AI2cBehaviorName[ioCharacter->ai2State.combatSettings.behavior[target_range - 1]]);*/
		}

		if (combat_state->current_weapon != NULL) {
			max_range = combat_state->targeting.weapon_parameters->max_range;
			if ((max_range > 0) && (combat_state->distance_to_target > max_range)) {
				// we are too far away from our target to shoot them
				combat_state->have_los = UUcFalse;
				combat_state->can_find_los = UUcFalse;
#if AI2_DEBUG_COMBAT
				COrConsole_Printf("range to target exceeds weapon max range %f", max_range);
#endif
			} else {
				// check our current LOS to where we're aiming and set up maneuvers to maintain it if necessary
				AI2iCombat_PerformLOS(ioCharacter, combat_state);
			}
		} else {
			// we have no weapon, we need no LOS
			combat_state->have_los = UUcTrue;
		}

		/*
		 * movement determination on the behavior level
		 */

		// clear primary movement in preparation for the determination phase
		AI2rManeuver_ClearPrimaryMovement(ioCharacter, combat_state);

		// calculate weights for our primary movement
		AI2rCombat_Behavior(ioCharacter, combat_state, AI2cCombatMessage_PrimaryMovement, &handled, 0, 0, 0);

		if (combat_state->maneuver.getup_direction != AI2cMovementDirection_Stopped) {
			// we are a gunplay character that is fallen; set our getup weight
			combat_state->maneuver.primary_movement_weights[AI2cPrimaryMovement_GetUp] = 1.5f;
		}

		investigating_body = ((combat_state->investigate_body_timer) && !(combat_state->dead_makesure_timer));
		if (investigating_body) {
			combat_state->weaponcaused_melee_timer = 0;

		} else {
			// check melee-helper weapons to see if we should go into melee now
			if ((combat_state->current_weapon != NULL) && (combat_state->target != NULL) &&
				(ioCharacter->ai2State.meleeProfile != NULL) && (ioCharacter->ai2State.combatSettings.melee_when != AI2cFight_Never)) {
				WPtWeaponClass *weapon_class = WPrGetClass(combat_state->current_weapon);

				UUmAssertReadPtr(weapon_class, sizeof(WPtWeaponClass));
				if (weapon_class->flags & WPcWeaponClassFlag_AIStun) {
					if (ONrCharacter_IsDizzy(combat_state->target) || ONrCharacter_IsFallen(combat_state->target)) {
						if (combat_state->weaponcaused_melee_timer == 0) {
							combat_state->weaponcaused_melee_timer = combat_state->targeting.weapon_parameters->melee_assist_timer
								/ AI2cCombat_UpdateFrames;
						}
					}
				} else if (weapon_class->flags & WPcWeaponClassFlag_AIKnockdown) {
					if (ONrCharacter_IsFallen(combat_state->target)) {
						if (combat_state->weaponcaused_melee_timer == 0) {
							combat_state->weaponcaused_melee_timer = combat_state->targeting.weapon_parameters->melee_assist_timer
								/ AI2cCombat_UpdateFrames;
						}
					}
				}
			}
		}

		if (combat_state->weaponcaused_melee_timer > 0) {
			combat_state->weaponcaused_melee_timer--;

			// we are NOT running our weapon behavior, we are attacking in melee instead
			combat_state->maneuver.primary_movement_weights[AI2cPrimaryMovement_Melee] = 1.2f;
			combat_state->maneuver.primary_movement_weights[AI2cPrimaryMovement_Hold] = 0.2f;
			combat_state->have_los = UUcTrue;
			combat_state->melee_override = UUcTrue;
			too_close_weight = 0;

		} else {
			combat_state->melee_override = UUcFalse;

			if ((investigating_body) || (too_close_weight < 0.5f)) {
				// we can shoot our gun freely if we want to
				combat_state->retreat_frames = 0;
				combat_state->retreatfailed_melee_timer = 0;

			} else {
				// we are too close to fire our gun. perhaps do something else.
				AI2rCombat_Behavior(ioCharacter, combat_state, AI2cCombatMessage_TooClose, &handled, (UUtUns32) &too_close_weight, 0, 0);
			}
		}

		if (investigating_body) {
			if ((combat_state->target != NULL) && ((combat_state->target->flags & ONcCharacterFlag_Dead) == 0)) {
				// the target has come back to life!
				combat_state->investigate_body_timer = 0;

			} else if ((combat_state->target != NULL) && (combat_state->target->death_delete_timer > 0)) {
				// the target is dead, and is going to explode... rely on our dodge behavior to get us out
				// of the way
				combat_state->maneuver.primary_movement_weights[AI2cPrimaryMovement_Hold] = 1.0f;

			} else {
				// walk towards the target's body (close towards)
				combat_state->maneuver.advance_maxdist = AI2cCombat_InvestigateInterpRange;
				combat_state->maneuver.advance_mindist = AI2cCombat_InvestigateMinRange;
				combat_state->maneuver.primary_movement_weights[AI2cPrimaryMovement_Advance] = 1.2f;
				combat_state->maneuver.primary_movement_weights[AI2cPrimaryMovement_Hold] = 0.8f;
				combat_state->maneuver.primary_movement_weights[AI2cPrimaryMovement_Melee] = 0.0f;
			}
		}

		if ((!combat_state->have_los) && (!combat_state->melee_override)) {
			if (combat_state->can_find_los) {
				// FIXME: apply maneuvers set up in the LOS detection code
			} else if (1) {
				// FIXME: examine our pursuit behavior. if pursue on strong
				// (i.e. we chase target if out of sight) then run to them, otherwise hold position.
				combat_state->maneuver.advance_maxdist = AI2cCombat_LOSInterpRange;
				combat_state->maneuver.advance_mindist = AI2cCombat_LOSMinRange;
				combat_state->maneuver.primary_movement_weights[AI2cPrimaryMovement_Advance] = 1.2f;
				combat_state->maneuver.primary_movement_weights[AI2cPrimaryMovement_Hold] = 0.4f;

				if (combat_state->distance_to_target < AI2cCombat_NoLOSMeleeRange) {
					combat_state->maneuver.primary_movement_weights[AI2cPrimaryMovement_Melee] = 0.7f;
				}
			}
		}

		// decide what our primary movement mode should be
		AI2rManeuver_DecidePrimaryMovement(ioCharacter, combat_state);

		// primary movement code may cause us to exit the combat state
		if (ioCharacter->ai2State.currentGoal != AI2cGoal_Combat)
			return;

		combat_state->maneuver.gun_running_pickup = UUcFalse;
		if ((combat_state->maneuver.primary_movement == AI2cPrimaryMovement_Gun) && (combat_state->maneuver.gun_target != NULL)) {
			// we are going for a gun
			if (!combat_state->runningpickup_set) {
				UUtUns16 random_chance;

				// check to see if we can do a running pickup on this gun
				random_chance = (UUtUns16) ((combat_state->combat_parameters->gun_runningpickup_percentage * UUcMaxUns16) / 100);
				combat_state->runningpickup_enable = (UUrRandom() < random_chance);
				combat_state->runningpickup_set = UUcTrue;

				if (gDebugGunBehavior) {
					COrConsole_Printf("%s: running pickup chance %02d%%: %s", ioCharacter->player_name,
										combat_state->combat_parameters->gun_runningpickup_percentage,
										combat_state->runningpickup_enable ? "YES" : "NO");
				}
			}
			
			// maybe try to pick up the gun that we are going for 'on the run'
			combat_state->maneuver.gun_running_pickup = combat_state->runningpickup_enable && AI2rManeuver_TryRunningGunPickup(ioCharacter, combat_state);
		} else {
			// we are not going for a gun, check random chance again next time
			combat_state->runningpickup_set = UUcFalse;
			combat_state->runningpickup_enable = UUcFalse;

			// also clear our thwarted timer
			combat_state->maneuver.gun_thwart_timer = 0;
		}

		// calculate weights for our secondary movement
		AI2rCombat_Behavior(ioCharacter, combat_state, AI2cCombatMessage_SecondaryMovement, &handled, 0, 0, 0);

		dodge_disable_firing = UUcFalse;
		if ((combat_state->dodge_enable) && (!combat_state->maneuver.gun_running_pickup) &&
			(ioCharacter->characterClass->ai2_behavior.flags & AI2cBehaviorFlag_DodgeMasterEnable)) {
			disable_dodge = (ioCharacter->inventory.weapons[0] != NULL) && (WPrIsFiring(ioCharacter->inventory.weapons[0]) || ONrCharacter_IsBusyInventory(ioCharacter));
			if (ioCharacter->characterClass->ai2_behavior.flags & (AI2cBehaviorFlag_DodgeWhileFiring | AI2cBehaviorFlag_DodgeStopFiring)) {
				// we can consider dodging even if we're firing
				disable_dodge = UUcFalse;
			}

			if (!disable_dodge) {
				// set up dodges away from danger
				AI2rManeuver_FindDodge(ioCharacter, combat_state);
			}

			if (combat_state->maneuver.dodge_max_weight > 0) {
				if (ioCharacter->characterClass->ai2_behavior.flags & AI2cBehaviorFlag_DodgeStopFiring) {
					// stop firing while we dodge
					dodge_disable_firing = UUcTrue;
				}
			}
		}

		// calculate our targeting parameters
		AI2rCombat_Behavior(ioCharacter, combat_state, AI2cCombatMessage_Targeting, &handled, 0, 0, 0);

		if (dodge_disable_firing || combat_state->maneuver.gun_running_pickup || combat_state->melee_override ||
			(combat_state->maneuver.primary_movement == AI2cPrimaryMovement_Melee)) {
			// don't aim or shoot our weapon
			combat_state->aim_weapon = UUcFalse;
			combat_state->shoot_weapon = UUcFalse;

		} else if (investigating_body) {
			// don't shoot people if investigating corpse
			combat_state->shoot_weapon = UUcFalse;
		}
	}

	/*
	 * SECONDARY MOVEMENT
	 *
	 * takes place once every frame - all movement through movement modifiers, no high-level pathfinding allowed
	 */

	// clear any of our movement modifiers, in preparation for secondary movement
	AI2rMovement_ClearMovementModifiers(ioCharacter);

	if (combat_state->maneuver.primary_movement_modifier_weight > 1e-03f) {
		// apply the movement modifier desired by primary movement
		AI2rMovement_MovementModifier(ioCharacter, combat_state->maneuver.primary_movement_modifier_angle,
													combat_state->maneuver.primary_movement_modifier_weight);
	}

	// we run the melee update code every frame, even if the rest of the combat system isn't running.
	able_to_melee = UUcFalse;
	if (combat_state->maneuver.primary_movement == AI2cPrimaryMovement_Melee) {
		if (combat_state->melee.fight_info.fight_owner == NULL) {
			// set up our fight spacing behavior
			fight_state = AI2rFight_FindByTarget(combat_state->target, UUcTrue);
			if (fight_state != NULL) {
				AI2rFight_AddAttacker(fight_state, ioCharacter);
			}
		}

		combat_state->melee.target = combat_state->target;
		able_to_melee = AI2rMelee_Update(ioCharacter, &combat_state->melee, combat_state);
#if AI2_DEBUG_COMBAT
		COrConsole_Printf("%d: melee update %s", ONrGameState_GetGameTime(), able_to_melee ? "SUCCESS" : "FAIL");
#endif

		if (able_to_melee) {
			// we are in melee
			combat_state->no_technique_timer = 0;
			combat_state->try_advance_timer = 0;
		} else {
			// do nothing and increment our no-technique timer
			combat_state->no_technique_timer++;
			if (combat_state->no_technique_timer >= AI2cCombat_MeleeNoTechniqueTimer) {
				// we can't throw any melee techniques from here, try advancing instead
				combat_state->try_advance_timer = AI2cCombat_MeleeTryAdvanceTimer;
			}
		}
	} else {
		// we are not in melee
		AI2iCombat_RemoveFromFight(ioCharacter, combat_state);
	}

	// apply the dodge that we found, if any
	AI2rManeuver_ApplyDodge(ioCharacter, combat_state);

	if (ioCharacter->ai2State.alarmStatus.action_marker != NULL) {
		// we are in combat instead of going for our alarm
		if (combat_state->distance_to_target < ioCharacter->ai2State.alarmSettings.chase_enemy_dist) {
			// we are close enough to reset our timer 
#if DEBUG_VERBOSE_ALARM
			COrConsole_Printf("in combat, close enough (%f < %f) to reset fight timer",
				combat_state->distance_to_target, ioCharacter->ai2State.alarmSettings.chase_enemy_dist);
#endif
			ioCharacter->ai2State.alarmStatus.fight_timer = ioCharacter->ai2State.alarmSettings.fight_timer;

		} else if (combat_state->distance_to_target > ioCharacter->ai2State.alarmSettings.ignore_enemy_dist) {
			// we are far enough away that it's safe to go back to hitting the alarm
#if DEBUG_VERBOSE_ALARM
			COrConsole_Printf("in combat, far enough (%f > %f) to give up and hit alarm",
				combat_state->distance_to_target, ioCharacter->ai2State.alarmSettings.ignore_enemy_dist);
#endif
			AI2rAlarm_Setup(ioCharacter, NULL);
			return;
		}
	}

	// disable slow-down correction if we are trying to go into melee
	AI2rMovement_DisableSlowdown(ioCharacter, (combat_state->maneuver.primary_movement == AI2cPrimaryMovement_Melee));

	/*
	 * targeting update
	 */
	
	disable_shooting = able_to_melee;
	disable_shooting = disable_shooting || ONrCharacter_IsStartling(ioCharacter, &startle_frame);
	disable_shooting = disable_shooting || ((too_close_weight > 0.2f) && (combat_state->maneuver.primary_movement == AI2cPrimaryMovement_Retreat));
	disable_shooting = disable_shooting || TRrAnimation_IsAttack(active_character->animation);
	disable_shooting = disable_shooting || TRrAnimation_TestFlag(active_character->animation, ONcAnimFlag_ThrowSource);
	disable_shooting = disable_shooting || AI2rExecutor_HasAttackOverride(ioCharacter, NULL);
	disable_shooting = disable_shooting || (!AI2rAllowedToAttack(ioCharacter, combat_state->target));

	if (disable_shooting) {
		combat_state->shoot_weapon = UUcFalse;
	}

	if (able_to_melee) {
		// the melee manager has handled all of our looking and aiming
		AI2iCombat_Trigger(ioCharacter, combat_state, UUcFalse);
	} else {
		if (combat_state->maneuver.gun_running_pickup) {
			// don't look at the target, look at the gun
			looking_at_target = UUcFalse;

		} else if (combat_state->run_targeting) {
			// make sure we don't shoot our weapon unless we updated targeting at the start of this combat tick
			AI2rTargeting_Execute(&combat_state->targeting, combat_state->aim_weapon,
								(targeting_updated && combat_state->shoot_weapon), combat_state->have_los);
			if (too_close_weight > 0) {
				looking_at_target = UUcTrue;
			} else {
				looking_at_target = (UUtBool) AI2rCombat_Behavior(ioCharacter, combat_state, AI2cCombatMessage_LookAtTarget, &handled, 0, 0, 0);
			}

		} else {
			// don't run targeting at all
			looking_at_target = UUcFalse;
		}
		
		if (!looking_at_target) {
			// just look in our direction of motion
			AI2rMovement_StopAiming(ioCharacter);
			AI2iCombat_Trigger(ioCharacter, combat_state, UUcFalse);
		}
	}
}

// ensure that our targeting has the correct weapon parameters
static void AI2rCombat_CheckTargetingWeapon(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState)
{
	AI2tWeaponParameters *weapon_params;
	M3tMatrix4x3 *weapon_matrix;
	WPtWeaponClass *weapon_class;

	if ((ioCombatState->current_weapon != ioCharacter->inventory.weapons[0]) ||
		(ioCombatState->current_alternate_fire != ioCombatState->alternate_fire)) {
		// get the parameters for this new weapon
		if (ioCharacter->inventory.weapons[0] == NULL) {
			weapon_params = NULL;
			weapon_matrix = NULL;
		} else {
			weapon_class = WPrGetClass(ioCharacter->inventory.weapons[0]);
			weapon_params = ioCombatState->alternate_fire ? &weapon_class->ai_parameters_alt : &weapon_class->ai_parameters;
			weapon_matrix = ONrCharacter_GetMatrix(ioCharacter, ONcWeapon_Index);
		}

		AI2rTargeting_ChangeWeapon(&ioCombatState->targeting, weapon_params, weapon_matrix);
		ioCombatState->current_weapon = ioCharacter->inventory.weapons[0];
		ioCombatState->current_alternate_fire = ioCombatState->alternate_fire;
	}
}

// remove this character from any fight state
static void AI2iCombat_RemoveFromFight(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState)
{
	AI2tFightState *fight_state;

	fight_state = ioCombatState->melee.fight_info.fight_owner;
	if (fight_state == NULL)
		return;

	// remove us from the fight spacing behavior
	AI2rFight_RemoveAttacker(fight_state, ioCharacter, UUcTrue);
}

// exit a combat state - called when the character goes out of "combat mode".
void AI2rCombat_Exit(ONtCharacter *ioCharacter)
{
	AI2tCombatState *combat_state;

	UUmAssert(ioCharacter->charType == ONcChar_AI2);
	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Combat);
	combat_state = &ioCharacter->ai2State.currentState->state.combat;

#if AI_DEBUG_COMBAT
	COrConsole_Printf("%s: exiting combat mode", ioCharacter->player_name);
#endif

	// re-enable slowdown if we disabled it
	AI2rMovement_DisableSlowdown(ioCharacter, UUcFalse);

	// release the trigger on our weapon
	AI2iCombat_Trigger(ioCharacter, combat_state, UUcFalse);

	// reset our facing and movement modifiers
	AI2rMovement_FreeFacing(ioCharacter);
	AI2rMovement_ClearMovementModifiers(ioCharacter);

	// remove us from any fight that we are participating in
	AI2iCombat_RemoveFromFight(ioCharacter, combat_state);

	// stop targeting
	AI2rTargeting_Terminate(&combat_state->targeting);

	// we can't be in combat alert state any more
	AI2rAlert_DowngradeStatus(ioCharacter, AI2cAlertStatus_High, NULL, UUcTrue);
}

// abort a combat state - called when the character dies while in "combat mode".
void AI2rCombat_NotifyDeath(ONtCharacter *ioCharacter)
{
	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Combat);
	AI2iCombat_RemoveFromFight(ioCharacter, &ioCharacter->ai2State.currentState->state.combat);
}

// ------------------------------------------------------------------------------------
// -- behavior control

UUtBool AI2rCombat_RangeIsLonger(AI2tCombatRange inRange, AI2tCombatRange inNewRange)
{
	if ((inRange == AI2cCombatRange_None) || (inRange == AI2cCombatRange_Short)) {
		return (inNewRange != inRange);

	} else if (inRange < AI2cCombatRange_Short) {
		return (inNewRange < inRange);

	} else {
		return (inNewRange > inRange);
	}
}

// get current range (note: does NOT use 'retreat', will only ever be Long, Medium or Short)
AI2tCombatRange AI2rCombat_GetRange(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
									AI2tCombatRange inOldRange)
{
	AI2tCombatRange new_range;

	if (!ioCombatState->targeting.valid_target_pt) {
		// this is the first frame of combat and we have not yet run our targeting
		return AI2cCombatRange_Long;
	}

	ioCombatState->distance_to_target = MUmVector_GetDistance(ioCharacter->location, ioCombatState->targeting.target_pt);

	if (ioCombatState->distance_to_target < ioCharacter->ai2State.combatSettings.short_range) {
		new_range = AI2cCombatRange_Short;

	} else if (ioCombatState->distance_to_target < ioCharacter->ai2State.combatSettings.medium_range) {
		if (inOldRange > AI2cCombatRange_Medium) {
			new_range = AI2cCombatRange_MediumRetreat;
		} else {
			new_range = AI2cCombatRange_Medium;
		}

	} else {
		if (inOldRange > AI2cCombatRange_Long) {
			new_range = AI2cCombatRange_LongRetreat;
		} else {
			new_range = AI2cCombatRange_Long;
		}
	}

	return new_range;
}

static void AI2iCombat_ChangeBehavior(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState)
{
	AI2tBehaviorType old_type, new_type;
	UUtBool handled;

	// NB: ranges are 0 = none, 1-5 = long<->long retreat. these parameters are stored
	// in an array that goes from 0-4 so subtract one
	new_type = ioCharacter->ai2State.combatSettings.behavior[ioCombatState->current_range - 1];
	if ((new_type <= 0) || (new_type >= AI2cBehavior_Max)) {
		AI2_ERROR(AI2cBug, AI2cSubsystem_Combat, AI2cError_Combat_UnknownBehavior, ioCharacter,
					ioCombatState->current_range, new_type, AI2cBehaviorName[new_type], 0);
		AI2iCombat_RevertToBasicBehavior(ioCharacter, ioCombatState);
		return;
	}

	// end the old behavior
	old_type = ioCombatState->current_behavior.type;
	AI2rCombat_Behavior(ioCharacter, ioCombatState, AI2cCombatMessage_EndBehavior, &handled, new_type, 0, 0);

	// find and then start the new behavior
	ioCombatState->current_behavior = AI2gBehaviorTable[new_type];
	UUmAssert(ioCombatState->current_behavior.type == new_type);

	// set up for the new behavior
	ioCombatState->fall_detect_enable = UUcFalse;
	AI2rCombat_Behavior(ioCharacter, ioCombatState, AI2cCombatMessage_BeginBehavior, &handled, old_type, 0, 0);

	if (!handled) {
		// unimplemented behavior! cannot use this behavior
		AI2_ERROR(AI2cError, AI2cSubsystem_Combat, AI2cError_Combat_UnimplementedBehavior, ioCharacter,
					ioCombatState->current_range, new_type, 0, 0);
		AI2iCombat_RevertToBasicBehavior(ioCharacter, ioCombatState);
	}

	// update for the first time
	AI2rCombat_Behavior(ioCharacter, ioCombatState, AI2cCombatMessage_PrimaryMovement, &handled, 0, 0, 0);
	ioCombatState->maneuver.primary_movement = AI2cPrimaryMovement_Hold;

	AI2rCombat_Behavior(ioCharacter, ioCombatState, AI2cCombatMessage_SecondaryMovement, &handled, 0, 0, 0);

	AI2rCombat_Behavior(ioCharacter, ioCombatState, AI2cCombatMessage_Targeting, &handled, 0, 0, 0);
}

// there has been an error trying to change behaviors - revert to a simple behavior
static void AI2iCombat_RevertToBasicBehavior(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState)
{
	AI2tBehaviorType old_type;
	UUtBool handled;

	old_type = ioCombatState->current_behavior.type;

	ioCombatState->current_behavior = AI2gBehaviorTable[AI2cBehavior_HoldAndFire];
	AI2rCombat_Behavior(ioCharacter, ioCombatState, AI2cCombatMessage_BeginBehavior, &handled, old_type, 0, 0);
	UUmAssert(handled);
}

UUtBool AI2rCombat_WaitingForShieldCharge(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState)
{
	UUtBool handled, returnval;

	returnval = (UUtBool) AI2rCombat_Behavior(ioCharacter, ioCombatState, AI2cCombatMessage_ShieldCharging, &handled, 0, 0, 0);

	if (handled) {
		return returnval;
	} else {
		return UUcFalse;
	}
}

// ------------------------------------------------------------------------------------
// -- contact and knowledge handling

// new-knowledge notification callback for the combat state
void AI2rCombat_NotifyKnowledge(ONtCharacter *ioCharacter, AI2tKnowledgeEntry *inEntry,
								UUtBool inDegrade)
{
	AI2tKnowledgeState *knowledge_state;
	AI2tCombatState *combat_state;
	UUtBool change_targets = UUcFalse, clear_hurt_me = UUcFalse;
	UUtInt32 priority_differential;
	float newcontact_distance, oldcontact_distance;
	UUtBool handled;

	// this routine should only ever be called for a character that is in combat
	if ((ioCharacter->charType != ONcChar_AI2) || (ioCharacter->ai2State.currentGoal != AI2cGoal_Combat)) {
		return;
	}

	knowledge_state = &ioCharacter->ai2State.knowledgeState;
	combat_state = &ioCharacter->ai2State.currentState->state.combat;

	if (inDegrade) {
		if ((inEntry->enemy != combat_state->target) || (combat_state->target == NULL)) {
			// we are not interested in degrade notifications that don't come from our target
			return;
		}

		if (inEntry != combat_state->target_knowledge) {
			// this contact is not the enemy that we are currently fighting - ignore degraded info
#if AI_DEBUG_COMBAT
			COrConsole_Printf("%s: WARNING: degrade entry for %s != target %s", ioCharacter->player_name,
						(inEntry->enemy == NULL) ? "NULL" : inEntry->enemy->player_name,
						(combat_state->target == NULL) ? "NULL" : combat_state->target->player_name);
			AI2rKnowledgeState_Report(ioCharacter, &ioCharacter->ai2State.knowledgeState, UUcTrue, COrConsole_Print);
#endif
			return;
		}

		// our perception of the target has changed negatively
		if (inEntry->strength == AI2cContactStrength_Dead) {
			AI2iCombat_TargetDied(ioCharacter, combat_state);

		} else if (inEntry->strength < AI2cContactStrength_Definite) {
			if (combat_state->target_strength == AI2cContactStrength_Definite) {
				if ((inEntry->enemy != NULL) && (inEntry->enemy->inventory.invisibilityRemaining > 0)) {
					// we lose invisible characters much more easily
					combat_state->lost_contact_timer = 0;
					AI2iCombat_LostContact(ioCharacter, combat_state);
				} else {
					if (combat_state->target_knowledge != NULL) {
						// store the target's current position (rather than the last position that we
						// saw them at) so that we don't lose people around corners as easily.
						ONrCharacter_GetPelvisPosition(combat_state->target, &combat_state->target_knowledge->last_location);
					}

					// we have just lost contact with the target - start our timer and after
					// a few seconds transition into the pursuit AI
					combat_state->lost_contact_timer = combat_state->combat_parameters->lost_contact_delay;

#if AI_DEBUG_COMBAT
					COrConsole_Printf("%s: lost contact, timer = %d",
						ioCharacter->player_name, combat_state->lost_contact_timer);
#endif
					if (combat_state->lost_contact_timer == 0) {
						// lose contact immediately
						AI2iCombat_LostContact(ioCharacter, combat_state);
					}
				}
			}
		} else {
			// we have contact with the target
			combat_state->lost_contact_timer = 0;
		}
	} else {
		if ((inEntry->last_type == AI2cContactType_Hit_Weapon) ||
			(inEntry->last_type == AI2cContactType_Hit_Melee)) {
			AI2rCombat_Behavior(ioCharacter, combat_state, AI2cCombatMessage_Hurt, &handled, inEntry->last_user_data, (UUtUns32) inEntry, 0);
		}

		if (inEntry == combat_state->target_knowledge) {
			// maintain our target strength
			combat_state->target_strength = inEntry->strength;
		} else {
			// determine whether we want to change to fighting this other target
			if (inEntry->priority == AI2cContactPriority_Friendly) {
				return;
			}

			if ((NULL == inEntry->enemy) || (0 == (ONcCharacterFlag_InUse & inEntry->enemy->flags))) {
				return;
			}

			if (combat_state->target_knowledge == NULL) {
				// we aren't fighting anyone, of course change
				change_targets = UUcTrue;

			} else {
				priority_differential = AI2rKnowledge_CompareFunc(combat_state->target_knowledge, inEntry, UUcTrue, ioCharacter);
				
				if (priority_differential > 0) {
					// higher priority target overrules
					change_targets = UUcTrue;

				} else if (priority_differential < 0) {
					// lower priority target, ignore
					return;
				}
			}

			if ((combat_state->target_knowledge != NULL) && (inEntry->strength == AI2cContactStrength_Definite)) {
				// attack people who are significantly closer than our current target - don't attack 
				// people who are a long distance further away
				newcontact_distance = MUmVector_GetDistanceSquared(combat_state->targeting.targeting_frompt, inEntry->last_location);
				oldcontact_distance = MUmVector_GetDistanceSquared(combat_state->targeting.targeting_frompt, combat_state->target_knowledge->last_location);

				if (newcontact_distance < UUmSQR(AI2cCombat_Targeting_SwitchDistanceRatio) * oldcontact_distance) {
					change_targets = UUcTrue;
				} else if (newcontact_distance > UUmSQR(AI2cCombat_Targeting_DontSwitchDistanceRatio) * oldcontact_distance) {
					change_targets = UUcFalse;
				}
			}

			if (inEntry->last_type == AI2cContactType_Hit_Weapon) {
				// we were shot by this new target
				if (inEntry->last_time > combat_state->last_hit_by_target + AI2cCombat_Targeting_SwitchIfNotHit) {
					// it has been a while since our current target hurt us - so change
					change_targets = UUcTrue;

					// also, make sure that our current target isn't still flagged as having hurt us, so we
					// don't keep switching targets back and forth
					clear_hurt_me = UUcTrue;
				}

			} else if (inEntry->last_type == AI2cContactType_Hit_Melee) {
				// always start attacking people who punch us, unless we were also recently hurt by our current target
				if (inEntry->last_time > combat_state->last_hit_by_target + AI2cCombat_Targeting_AttackMeleeShooterUnlessAlsoHitTime) {
					change_targets = UUcTrue;
				}
			}
			
			if (change_targets) {
				if (clear_hurt_me && (combat_state->target_knowledge != NULL)) {
					combat_state->target_knowledge->has_hurt_me = UUcFalse;
					combat_state->target_knowledge->has_hurt_me_in_melee = UUcFalse;
				}
				AI2rCombat_SetNewTarget(ioCharacter, combat_state, inEntry, UUcFalse);
			}
		}
	}
}

// an entry in our knowledge base has just been removed, forcibly delete any references we had to it
UUtBool AI2rCombat_NotifyKnowledgeRemoved(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
											AI2tKnowledgeEntry *inEntry)
{
	if (ioCombatState->target_knowledge == inEntry) {
		AI2iCombat_ClearTarget(ioCharacter, ioCombatState);
		return UUcTrue;
	} else {
		return UUcFalse;
	}
}

// clear our target information
static void AI2iCombat_ClearTarget(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState)
{
	ioCombatState->target = NULL;
	ioCombatState->target_knowledge = NULL;
	ioCombatState->target_strength = AI2cContactStrength_Dead;
	ioCombatState->last_hit_by_target = 0;
	ioCombatState->last_melee_hit_by_target = 0;
	ioCombatState->targeting.target = NULL;
}

static UUtBool AI2iCombat_CheckTarget(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState)
{
	if (!ioCombatState->target_dead) {
		// check that our target is still alive
		UUtBool target_currently_dead = UUcFalse;

		if (ioCombatState->target == NULL) {
			target_currently_dead = UUcTrue;
		} else if ((ioCombatState->target->flags & ONcCharacterFlag_InUse) == 0) {
			target_currently_dead = UUcTrue;
		} else if (ioCombatState->target->flags & ONcCharacterFlag_Dead) {
			target_currently_dead = UUcTrue;
		}

		if (target_currently_dead) {
			// this should never happen... the knowledge system should warn us if our target
			// dies!
			AI2_ERROR(AI2cWarning, AI2cSubsystem_Combat, AI2cError_Combat_TargetDead, ioCharacter,
						ioCombatState->target, 0, 0, 0);
			AI2iCombat_TargetDied(ioCharacter, ioCombatState);

			if (ioCharacter->ai2State.currentGoal != AI2cGoal_Combat) {
				// we have exited the combat state
				return UUcFalse;
			}
		}
	}

	if (ioCombatState->target_dead) {
		// if the target has been deleted, then we should no longer store a pointer to them.
		if ((ioCombatState->target != NULL) && ((ioCombatState->target->flags & ONcCharacterFlag_InUse) == 0)) {
			AI2iCombat_ClearTarget(ioCharacter, ioCombatState);
		}

		// our target is already dead.
		if (ioCombatState->dead_makesure_timer) {
			ioCombatState->dead_makesure_timer--;
		}

		if (ioCombatState->dead_makesure_timer == 0) {
			// go investigate the dead target's body
			if ((ioCombatState->maneuver.primary_movement != AI2cPrimaryMovement_Advance) &&
				(ioCombatState->investigate_body_timer) && ((ioCombatState->target == NULL) || (ioCombatState->target->death_delete_timer == 0))) {
				// we aren't actually moving right now - decrement our investigation timer
				// this is so that we run the delay time from the point where we actually
				// reach the target.
				ioCombatState->investigate_body_timer--;

				// check to see if we want to taunt the dead body of the target
				if (ioCombatState->dead_taunt_enable && !ioCombatState->dead_done_taunt && ONrCharacter_IsStanding(ioCharacter) &&
					(ioCombatState->investigate_body_timer < ioCombatState->combat_parameters->investigate_body_delay / 2)) {
					UUtUns32 random_val, random_chance;
					ONtActiveCharacter *active_character = ONrForceActiveCharacter(ioCharacter);
	
					ioCombatState->dead_done_taunt = UUcTrue;
					ioCombatState->dead_taunt_enable = UUcFalse;

					// we have a random chance of taunting over their dead body
					random_val = (UUtUns32) UUrRandom();
					random_chance = (((UUtUns32) UUcMaxUns16) * ioCharacter->characterClass->ai2_behavior.combat_parameters.dead_taunt_percentage) / 100;
					if ((random_val < random_chance) && (active_character != NULL)) {
						ONrCharacter_DoAnimation(ioCharacter, active_character, ONcAnimPriority_Appropriate, ONcAnimType_Taunt);
					}

					// in any case, run the "check body" vocalization
					AI2rVocalize(ioCharacter, AI2cVocalization_CheckBody, UUcFalse);
				}
			}

			if (ioCombatState->investigate_body_timer == 0) {
				// we're done here.
				AI2rReturnToJob(ioCharacter, UUcTrue, UUcTrue);
			}
		}

		// we don't need to predict the target any more
		return UUcFalse;
	} else {
		UUmAssertReadPtr(ioCombatState->target, sizeof(ONtCharacter));
	}

	if (ioCombatState->target != NULL) {
		// update the target's current location whenever we're in combat mode,
		// so that we don't lose people around corners as easily. this means that we magically
		// know where enemies are, until we lose contact with them (after about 5 seconds)
		UUmAssert(ioCombatState->target_knowledge != NULL);
		ONrCharacter_GetPelvisPosition(ioCombatState->target, &ioCombatState->target_knowledge->last_location);
	}

	if (ioCombatState->lost_contact_timer) {
		ioCombatState->lost_contact_timer--;

		// FIXME: if we want to try and move in closer, try to find an LOS position here rather than transitioning
		// straight to pursuit mode
		if (1) {
			AI2iCombat_Pursue(ioCharacter, ioCombatState);
			return UUcFalse;
		}

		if (ioCombatState->lost_contact_timer == 0) {
#if AI_DEBUG_COMBAT
			COrConsole_Printf("%s: lost-contact timer ran out", ioCharacter->player_name);
#endif
			// our lost-contact timer has run out without us reacquiring the target
			AI2iCombat_LostContact(ioCharacter, ioCombatState);
		}
	}

	return UUcTrue;
}

void AI2rCombat_SetNewTarget(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
									  AI2tKnowledgeEntry *inEntry, UUtBool inInitialTarget)
{
	UUmAssert(inEntry->enemy != NULL);
	ioCombatState->target = inEntry->enemy;
	ioCombatState->target_dead = UUcFalse;
	ioCombatState->target_knowledge = inEntry;
	ioCombatState->target_strength = inEntry->strength;

	// set up internal state for the new target
	ioCombatState->weaponcaused_melee_timer = 0;
	ioCombatState->retreatfailed_melee_timer = 0;
	ioCombatState->retreat_frames = 0;
	ioCombatState->melee_override = UUcFalse;
	ioCombatState->dead_makesure_timer = 0;
	ioCombatState->dead_done_taunt = UUcFalse;
	ioCombatState->dead_taunt_enable = UUcFalse;
	ioCombatState->investigate_body_timer = 0;
	ioCombatState->lost_contact_timer = 0;
	ioCombatState->last_hit_by_target = 0;
	ioCombatState->last_melee_hit_by_target = 0;

	if (inEntry->last_type == AI2cContactType_Hit_Weapon) {
		ioCombatState->last_hit_by_target = inEntry->last_time;

	} else if (inEntry->last_type == AI2cContactType_Hit_Melee) {
		ioCombatState->last_hit_by_target = inEntry->last_time;
		ioCombatState->last_melee_hit_by_target = inEntry->last_time;
	}

	if (ioCombatState->target != NULL) {
		// force the target character to become active, because we want to hit them
		ONrForceActiveCharacter(ioCombatState->target);
	}

	// set up our targeting
	AI2rTargeting_SetupNewTarget(&ioCombatState->targeting, ioCombatState->target, inInitialTarget);

	// compute the combat range to our target
	ioCombatState->current_range = AI2rCombat_GetRange(ioCharacter, ioCombatState, AI2cCombatRange_None);

	// switch to the default behavior for the range.
	ioCombatState->current_behavior = AI2gBehaviorTable[AI2cBehavior_None];
	AI2iCombat_ChangeBehavior(ioCharacter, ioCombatState);
}

// transition into the pursuit state
static void AI2iCombat_Pursue(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState)
{
	AI2tKnowledgeEntry *entry;

	entry = AI2rKnowledge_FindEntry(ioCharacter, &ioCharacter->ai2State.knowledgeState, ioCombatState->target);
	if (entry == NULL) {
		// we seem to have lost the entry completely!
		AI2_ERROR(AI2cWarning, AI2cSubsystem_Combat, AI2cError_Combat_ContactDisappeared, ioCharacter,
					ioCombatState->target, 0, 0, 0);
		AI2rReturnToJob(ioCharacter, UUcTrue, UUcTrue);
	} else {
		// transition into the pursuit state and follow target.
		AI2rExitState(ioCharacter);

		ioCharacter->ai2State.currentGoal = AI2cGoal_Pursuit;
		ioCharacter->ai2State.currentStateBuffer.state.pursuit.target = entry;
		ioCharacter->ai2State.currentState = &(ioCharacter->ai2State.currentStateBuffer);
		ioCharacter->ai2State.currentState->begun = UUcFalse;

		AI2rEnterState(ioCharacter);
	}
}

static void AI2iCombat_TargetDied(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState)
{
	AI2tKnowledgeEntry *entry = AI2iCombat_FindNewTarget(ioCharacter, AI2cContactStrength_Strong);

	if (entry == NULL) {
		// there are no other targets that we know of.
		ioCombatState->target_dead = UUcTrue;
		if ((ioCharacter->inventory.weapons[0] != NULL) && (WPrHasAmmo(ioCharacter->inventory.weapons[0])) &&
			(ioCombatState->maneuver.primary_movement != AI2cPrimaryMovement_Melee)) {
			// keep shooting the current target for a little while just to make sure
			ioCombatState->dead_makesure_timer = ioCombatState->combat_parameters->dead_makesure_delay;
		}

		// FIXME: only one enemy should go investigate the body. use cookie manager when it comes
		// online?
		ioCombatState->investigate_body_timer = ioCombatState->combat_parameters->investigate_body_delay;

		if ((ioCombatState->target != NULL) && ((ioCombatState->target->flags & ONcCharacterFlag_InUse) == 0)) {
			// character has been deleted - can no longer access their fields
			AI2iCombat_ClearTarget(ioCharacter, ioCombatState);
		}

	} else if (entry->strength == AI2cContactStrength_Definite) {
		// transition to the new target without leaving the combat state
		AI2rCombat_SetNewTarget(ioCharacter, ioCombatState, entry, UUcFalse);

	} else {
		// we can't actually see the new target, but we know that they're nearby - follow our last contact
		ioCombatState->target = entry->enemy;
		ioCombatState->target_knowledge = entry;
		AI2iCombat_Pursue(ioCharacter, ioCombatState);
	}
}

// find a potential target for us to change to
static AI2tKnowledgeEntry *AI2iCombat_FindNewTarget(ONtCharacter *ioCharacter, AI2tContactStrength inRequiredStrength)
{
	AI2tKnowledgeEntry *newtarget, *entry;

	// look for other potential targets
	newtarget = NULL;
	for (entry = ioCharacter->ai2State.knowledgeState.contacts; entry != NULL; entry = entry->nextptr) {
		if ((entry->enemy == NULL) || (entry->strength < inRequiredStrength) || (entry->priority == AI2cContactPriority_Friendly))
			continue;

		if ((newtarget == NULL) || (AI2rKnowledge_CompareFunc(newtarget, entry, UUcFalse, ioCharacter) > 0)) {
			newtarget = entry;
		}
	}

	return entry;
}

// we have lost contact with the target for a short period.
static void	AI2iCombat_LostContact(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState)
{
	AI2tKnowledgeEntry *entry;

	entry = AI2iCombat_FindNewTarget(ioCharacter, AI2cContactStrength_Definite);

	if (entry != NULL) {
		// there is a definite target in view. switch to attacking them instead.
		AI2rCombat_SetNewTarget(ioCharacter, ioCombatState, entry, UUcFalse);
	} else {
		// follow our current target
		AI2iCombat_Pursue(ioCharacter, ioCombatState);
	}
}

// ------------------------------------------------------------------------------------
// -- targeting control

static void AI2iCombat_Trigger(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState, UUtBool inTrigger)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
	UUtBool handled;

	if (active_character == NULL) {
		// fail safe
		return;
	}

	if (ONrCharacter_IsVictimAnimation(ioCharacter) || ONrCharacter_IsPickingUp(ioCharacter)) {
		// don't fire our weapon in these cases
		inTrigger = UUcFalse;
	}

	if (ioCharacter->inventory.weapons[0] == NULL) {
		inTrigger = UUcFalse;
	}

	if (inTrigger) {
		WPtWeaponClass *weapon_class = WPrGetClass(ioCharacter->inventory.weapons[0]);

		if (weapon_class->flags & WPcWeaponClassFlag_AISingleShot) {
			if (WPrMustWaitToFire(ioCharacter->inventory.weapons[0])) {
				// don't press trigger while we wait for chamber time
				inTrigger = UUcFalse;
			}
		}
	}

	if (inTrigger) {
		ioCombatState->trigger_pressed = UUcFalse;
		ioCombatState->alternate_trigger_pressed = UUcFalse;

		if (ioCombatState->alternate_fire) {
			active_character->pleaseFireAlternate = UUcTrue;
			ioCombatState->alternate_trigger_pressed = UUcTrue;
		} else {
			active_character->pleaseFire = UUcTrue;
			ioCombatState->trigger_pressed = UUcTrue;
		}
	} else {
		// release our triggers
		if (ioCombatState->trigger_pressed) {
			active_character->pleaseReleaseTrigger = UUcTrue;
			ioCombatState->trigger_pressed = UUcFalse;
		}

		if (ioCombatState->alternate_trigger_pressed) {
			active_character->pleaseReleaseAlternateTrigger = UUcTrue;
		}
		active_character->pleaseFire = UUcFalse;
		active_character->pleaseFireAlternate = UUcFalse;
	}

	// tell the behavior that we have fired
	AI2rCombat_Behavior(ioCharacter, ioCombatState, AI2cCombatMessage_Fire, &handled, inTrigger, ioCombatState->alternate_fire, 0);
}

// ------------------------------------------------------------------------------------
// -- targeting callbacks

static void AI2iCombatAI_TargetingCallback_Fire(AI2tTargetingState *inTargetingState, UUtBool inFire)
{
	ONtCharacter *character;

	UUmAssert(inTargetingState->owner.type == AI2cTargetingOwnerType_Character);
	character = inTargetingState->owner.owner.character;

	UUmAssert(character->charType == ONcChar_AI2);
	UUmAssert(character->ai2State.currentGoal == AI2cGoal_Combat);

	AI2iCombat_Trigger(character, &character->ai2State.currentState->state.combat, inFire);
}

static void AI2iAI_TargetingCallback_SetAimSpeed(AI2tTargetingState *inTargetingState, float inSpeed)
{
	UUmAssert(inTargetingState->owner.type == AI2cTargetingOwnerType_Character);
	AI2rExecutor_SetAimingSpeed(inTargetingState->owner.owner.character, inSpeed);
}

static void AI2iAI_TargetingCallback_ForceAimWithWeapon(AI2tTargetingState *inTargetingState, UUtBool inWithWeapon)
{
	UUmAssert(inTargetingState->owner.type == AI2cTargetingOwnerType_Character);
	AI2rMovement_Force_AimWithWeapon(inTargetingState->owner.owner.character, inWithWeapon);
}

static void AI2iAI_TargetingCallback_EndForceAim(AI2tTargetingState *inTargetingState)
{
	UUmAssert(inTargetingState->owner.type == AI2cTargetingOwnerType_Character);
	AI2rMovement_DontForceAim(inTargetingState->owner.owner.character);
	AI2rMovement_StopAiming(inTargetingState->owner.owner.character);
}

static void AI2iAI_TargetingCallback_GetPosition(AI2tTargetingState *inTargetingState, M3tPoint3D *inPoint)
{
	UUmAssert(inTargetingState->owner.type == AI2cTargetingOwnerType_Character);
	MUmVector_Copy(*inPoint, inTargetingState->owner.owner.character->location);
	inPoint->y += inTargetingState->owner.owner.character->heightThisFrame;
}

static void AI2iAI_TargetingCallback_AimVector(AI2tTargetingState *inTargetingState, M3tVector3D *inDirection)
{
	UUmAssert(inTargetingState->owner.type == AI2cTargetingOwnerType_Character);
	AI2rMovement_LookInDirection(inTargetingState->owner.owner.character, inDirection);
}

static void AI2iAI_TargetingCallback_AimPoint(AI2tTargetingState *inTargetingState, M3tPoint3D *inPoint)
{
	UUmAssert(inTargetingState->owner.type == AI2cTargetingOwnerType_Character);
	AI2rMovement_LookAtPoint(inTargetingState->owner.owner.character, inPoint);
}

static void AI2iAI_TargetingCallback_AimCharacter(AI2tTargetingState *inTargetingState, ONtCharacter *inCharacter)
{
	UUmAssert(inTargetingState->owner.type == AI2cTargetingOwnerType_Character);
	AI2rMovement_LookAtCharacter(inTargetingState->owner.owner.character, inCharacter);
}

static void AI2iAI_TargetingCallback_UpdateAiming(AI2tTargetingState *inTargetingState)
{
	// now that we are looking in combat, override our previous glancing instructions
	UUmAssert(inTargetingState->owner.type == AI2cTargetingOwnerType_Character);
	AI2rMovement_StopGlancing(inTargetingState->owner.owner.character);
}

// ------------------------------------------------------------------------------------
// -- line-of-sight

static UUtBool AI2iCombat_CheckLOS(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   M3tPoint3D *inFrom, M3tVector3D *inDir)
{
	AKtEnvironment *env;
	M3tVector3D cyl_fromvector;
	float a, b, c, t, disc, height, test_dist;
	ONtCharacter **charptr;
	M3tVector3D unit_direction;
	UUtUns32 num_chars, itr;
	UUtUns16 our_team;
	UUtBool have_los;

	have_los = UUcTrue;

	if (ioCombatState->desire_env_los) {
		// check to see if our targeting vector is blocked by geometry
		// note: don't use AKrLineOfSight because it checks against characters

		// FIXME: modify eventually to get location of collision for maneuvering?
		env = ONrGameState_GetEnvironment();
		if (AKrCollision_Point(env, inFrom, inDir, AKcGQ_Flag_LOS_CanShoot_Skip_AI, 0)) {
#if AI2_DEBUG_COMBAT
			COrConsole_Printf("LOS blocked by environment");
#endif
			have_los = UUcFalse;
			goto exit;
		}

		// we must normalize this direction in order to use AMrRayToObject
		MUmVector_Copy(unit_direction, *inDir);

		test_dist = MUmVector_GetLength(*inDir);
		if (test_dist > 1e-03f) {
			MUmVector_ScaleCopy(unit_direction, 1.0f / test_dist, *inDir);

			if (AMrRayToObject(inFrom, &unit_direction, test_dist, NULL, NULL)) {
				// there is an intersection; our LOS is blocked
#if AI2_DEBUG_COMBAT
				COrConsole_Printf("LOS blocked by object %d", hit_object->index);
#endif
				have_los = UUcFalse;
				goto exit;
			}
		}
	}

	if (ioCombatState->desire_friendly_los) {
		// check to see if our targeting vector is blocked by friendly characters
		charptr = ONrGameState_LivingCharacterList_Get();
		num_chars = ONrGameState_LivingCharacterList_Count();

		our_team = ioCharacter->teamNumber;
		for (itr = 0; itr < num_chars; itr++, charptr++) {
			// don't check LOS vs ourselves or target character
			if ((*charptr == ioCharacter) || (*charptr == ioCombatState->target))
				continue;

			// we don't care about not shooting hostile characters that get in the way
			if (AI2rTeam_FriendOrFoe(our_team, (*charptr)->teamNumber) == AI2cTeam_Hostile) {
				continue;
			}

			// determine whether the target vector intersects a cylinder around our friend
			//
			// quadratic equation: v^2 t^2 + 2uvt + u^2 - r^2 = 0
			MUmVector_Subtract(cyl_fromvector, *inFrom, (*charptr)->location);
			a = UUmSQR(inDir->x) + UUmSQR(inDir->z);
			b = 2 * (inDir->x * cyl_fromvector.x + inDir->z * cyl_fromvector.z);
			c = UUmSQR(cyl_fromvector.x) + UUmSQR(cyl_fromvector.z) - UUmSQR(AI2cCombat_LOS_FriendlyClearRadius);
			disc = UUmSQR(b) - 4 * a * c;

			if (disc < 0)
				continue;

			// determine the t-value of the first intersection
			t = (- b - MUrSqrt(disc)) / (2 * a);
			if ((t < 0) || (t > 1))
				continue;

			// what height is this relative to the character?
			height = inFrom->y + t * inDir->y - (*charptr)->location.y;
			if ((height < 0) || (height > (*charptr)->heightThisFrame + AI2cCombat_LOS_FriendlyClearHeight))
				continue;

			// otherwise, our LOS is blocked
#if AI2_DEBUG_COMBAT
			COrConsole_Printf("LOS blocked by friendly '%s'", (*charptr)->player_name);
#endif
			have_los = UUcFalse;
			goto exit;
		}
	}

exit:
#if TOOL_VERSION
	if (AI2gDebug_ShowLOS) {
		// store this line of sight in the global debugging buffer
		AI2gCombat_LOSPoints[AI2gCombat_LOSNextPoint] = *inFrom;
		MUmVector_Add(AI2gCombat_LOSPoints[AI2gCombat_LOSNextPoint + 1], *inFrom, *inDir);
		AI2gCombat_LOSClear[AI2gCombat_LOSNextPoint] = have_los;

		AI2gCombat_LOSNextPoint += 2;
		AI2gCombat_LOSUsedPoints = UUmMax(AI2gCombat_LOSUsedPoints, AI2gCombat_LOSNextPoint);
		if (AI2gCombat_LOSNextPoint >= AI2cCombat_LOSPointBufferSize) {
			AI2gCombat_LOSNextPoint = 0;
		}
	}
#endif

	return have_los;
}

static void AI2iCombat_PerformLOS(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState)
{
	M3tVector3D los_from, cur_pos, vector_to_target, ballistic_velocity;
	float delta_t, dist, gravity_per_step;
	UUtUns32 num_steps, itr;
	UUtBool ballistic_weapon, check_los;

	UUmAssert(ioCharacter->inventory.weapons[0] != NULL);
	UUmAssert(ioCombatState->targeting.weapon_matrix != NULL);

	ioCombatState->have_los = UUcTrue;
	ioCombatState->can_find_los = UUcFalse;

	if (!ioCombatState->targeting.valid_target_pt) {
		ioCombatState->have_los = UUcTrue;
		return;
	}

	MUmVector_Copy(los_from, ioCombatState->targeting.weapon_pt);
	MUmVector_Subtract(vector_to_target, ioCombatState->targeting.target_pt, los_from);

	ballistic_weapon = (ioCombatState->targeting.weapon_parameters->ballistic_speed > 0);
	if (ballistic_weapon) {
		if (ioCombatState->targeting.firing_solution) {
			// build the ballistic trajectory and check it in sections
			ballistic_velocity.x = ioCombatState->targeting.solution_xvel * MUrSin(ioCombatState->targeting.solution_theta);
			ballistic_velocity.z = ioCombatState->targeting.solution_xvel * MUrCos(ioCombatState->targeting.solution_theta);
			ballistic_velocity.y = ioCombatState->targeting.solution_yvel;

			dist = MUrSqrt(UUmSQR(vector_to_target.x) + UUmSQR(vector_to_target.z));
			num_steps = ((UUtUns32) (dist / AI2cCombat_BallisticLOS_StepSize)) + 1;
			delta_t = dist / (ioCombatState->targeting.solution_xvel * num_steps);

			MUmVector_Scale(ballistic_velocity, delta_t);
			MUmVector_Copy(cur_pos, los_from);
			gravity_per_step = ioCombatState->targeting.weapon_parameters->ballistic_gravity * P3gGravityAccel * UUmSQR(delta_t);
			ballistic_velocity.y -= 0.5f * gravity_per_step;			// average velocity over a step

			for (itr = 0; itr < num_steps; itr++) {
				check_los = AI2iCombat_CheckLOS(ioCharacter, ioCombatState, &cur_pos, &ballistic_velocity);

				if (!check_los) {
					ioCombatState->have_los = UUcFalse;
					break;
				}

				MUmVector_Add(cur_pos, cur_pos, ballistic_velocity);
				ballistic_velocity.y -= gravity_per_step;
			}
		} else {
			// no firing solution was found... we must be too far away
			ioCombatState->have_los = UUcFalse;
		}
	} else {
		// work out position and vector for LOS calculation
		ioCombatState->have_los = AI2iCombat_CheckLOS(ioCharacter, ioCombatState, &los_from, &vector_to_target);
	}

	if (!ioCombatState->have_los) {
		// FIXME: if maneuver_for_los is set, find maneuvers to get line-of-sight (walk around pillars, etc).
		ioCombatState->can_find_los = UUcFalse;
	}
}

// ------------------------------------------------------------------------------------
// -- shared behavior functions

static UUtBool AI2iCheckFightBack(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								  AI2tFightType inFightType)
{
	AI2tCombatRange fight_range, current_range;

	if (inFightType == AI2cFight_Always)
		return UUcTrue;
	else if (inFightType == AI2cFight_Never)
		return UUcFalse;

	if (inFightType >= AI2cFight_IfShortRange) {
		// determine at what range we will go into fighting (AI2cCombatRange_None means that
		// range is never the determining factor)
		UUmAssert((inFightType >= 0) && (inFightType < AI2cFight_Max));
		fight_range = AI2cFightRange[inFightType];

		// determine our current range (by stripping retreat bits)
		current_range = ioCombatState->current_range;
		if (current_range == AI2cCombatRange_LongRetreat) {
			current_range = AI2cCombatRange_Long;
		} else if (current_range == AI2cCombatRange_MediumRetreat) {
			current_range = AI2cCombatRange_Medium;
		}

		if (current_range >= fight_range)
			return UUcTrue;
	}

	if (inFightType == AI2cFight_IfPunched) {
		UUtUns32 current_time = ONrGameState_GetGameTime();

		if (ioCombatState->last_melee_hit_by_target + AI2cCombat_MeleeRecentlyHurtTimer > current_time) {
			// we have been hit in melee by the target recently
			return UUcTrue;
		}
	}

	return UUcFalse;
}
			
static UUtUns32 AI2iTryToFindGunBehavior(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3)
{
	UUtBool nogun_handled, fight_instead;

	*inHandled = UUcTrue;

	switch(inMsg) {
	case AI2cCombatMessage_PrimaryMovement:
		fight_instead = AI2iCheckFightBack(ioCharacter, ioCombatState, ioCharacter->ai2State.combatSettings.melee_when);

		if (fight_instead) {
			// go into melee instead of doing some other passive action
			// (going for gun, alarm, retreating)
			ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Melee] = 1.0f;
			return 0;
		}

		// look for nearby guns
		ioCombatState->maneuver.gun_search_minradius = 30.0f;
		ioCombatState->maneuver.gun_search_maxradius = 200.0f;

		ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Gun] = 1.0f;

		// call our no-gun behavior to set up alternatives for if we can't find a gun
		return AI2iNoGunBehavior(ioCharacter, ioCombatState, inMsg, &nogun_handled, inParam1, inParam2, inParam3);

	case AI2cCombatMessage_SecondaryMovement:
		if (ioCombatState->maneuver.primary_movement == AI2cPrimaryMovement_Gun) {
			// we are focused on going for a gun and don't care about LOS to target
			return 0;
		} else {
			return AI2iNoGunBehavior(ioCharacter, ioCombatState, inMsg, &nogun_handled, inParam1, inParam2, inParam3);
		}

	case AI2cCombatMessage_Targeting:
		if (ioCombatState->maneuver.primary_movement == AI2cPrimaryMovement_Gun) {
			// we are focused on going for a gun and don't care about LOS to target.
			ioCombatState->desire_env_los = UUcFalse;
			ioCombatState->desire_friendly_los = UUcFalse;
			ioCombatState->aim_weapon = UUcFalse;

			if (ioCombatState->have_los) {
				// FIXME: look at the target unless it would cause us to run backwards
				ioCombatState->run_targeting = UUcTrue;
			} else {
				// can't see the target, don't look at them
				ioCombatState->run_targeting = UUcFalse;
			}
			return 0;
		} else {
			return AI2iNoGunBehavior(ioCharacter, ioCombatState, inMsg, &nogun_handled, inParam1, inParam2, inParam3);
		}

	default:
		*inHandled = UUcFalse;
		return 0;
	}
}

static UUtUns32 AI2iNoGunBehavior(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3)
{
	*inHandled = UUcTrue;

	switch(inMsg) {
	case AI2cCombatMessage_PrimaryMovement:
		// note that if we are trying to go for a gun, that will already have been entered
		// with a weight of 1.0f. these other kinds of actions should have appropriate weights to
		// work well with this.

		switch(ioCharacter->ai2State.combatSettings.no_gun) {
		case AI2cNoGun_Alarm:
			ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_FindAlarm] = 1.0f;
			// if we can't find an alarm, go into melee by default

		case AI2cNoGun_Melee:
			ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Melee] = 0.8f;
			break;

		case AI2cNoGun_Retreat:
			ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Retreat] = 0.8f;
			ioCombatState->maneuver.retreat_mindist = 100.0f;
			ioCombatState->maneuver.retreat_maxdist = 1000.0f;
			break;

		default:
			UUmAssert(!"AI2iNoGunBehavior: unknown enum type in combatSettings.no_gun");
		}
		return 0;

	case AI2cCombatMessage_SecondaryMovement:
		ioCombatState->dodge_enable = UUcTrue;
		return 0;

	case AI2cCombatMessage_Targeting:
		// we aren't shooting at the target
		ioCombatState->desire_env_los = UUcFalse;
		ioCombatState->desire_friendly_los = UUcFalse;
		ioCombatState->aim_weapon = UUcFalse;
		ioCombatState->run_targeting = UUcTrue;

		return 0;

	default:
		*inHandled = UUcFalse;
		return 0;
	}
}

static UUtBool AI2iBehavior_FindEdgeFlag(M3tPoint3D *inPoint, UUtUns16 inBaseFlagID,
										M3tPoint3D *outPoint, float *outFacing)
{
	UUtUns16 flag_id;
	ONtFlag flag;
	UUtBool found, abort, first_flag;
	M3tPoint3D found_pt, prev_flag_pt, first_flag_pt, cur_pt;
	float found_distsq, found_facing, px, pz, dx, dz, t, one_minus_t, cur_distsq;

	// loop over flags, trying to find ones that exist in the level
	flag_id = inBaseFlagID;
	first_flag = UUcTrue;
	found = abort = UUcFalse;
	do {
		if (!ONrLevel_Flag_ID_To_Flag(flag_id, &flag)) {
			if (first_flag) {
				// could not find any flags, abort
				return UUcFalse;
			} else {
				// abort the loop after we process this last edge
				abort = UUcTrue;
				flag.location = first_flag_pt;
			}
		}

		if (!first_flag) {
			// check the line that we have found, and find the nearest point on it
			px = inPoint->x - prev_flag_pt.x;
			pz = inPoint->z - prev_flag_pt.z;
			dx = flag.location.x - prev_flag_pt.x;
			dz = flag.location.z - prev_flag_pt.z;

			t = (px * dx + pz * dz) / (dx * dx + dz * dz);
			if (t <= 0.0f) {
				MUmVector_Copy(cur_pt, prev_flag_pt);

			} else if (t >= 1.0f) {
				MUmVector_Copy(cur_pt, flag.location);

			} else {
				one_minus_t = 1.0f - t;
				MUmVector_ScaleCopy(cur_pt, one_minus_t, prev_flag_pt);
				MUmVector_ScaleIncrement(cur_pt, t, flag.location);
			}

			// work out how far this point is from our location
			cur_distsq = UUmSQR(cur_pt.x - inPoint->x) + UUmSQR(cur_pt.z - inPoint->z);
			if (!found || (cur_distsq < found_distsq)) {
				// this point is closer than our current best point
				found_pt = cur_pt;
				found_distsq = cur_distsq;
				if (outFacing != NULL) {
					found_facing = MUrATan2(dz, -dx);
					UUmTrig_ClipLow(found_facing);
				}
				found = UUcTrue;
			}

		} else {
			// store this first flag point
			first_flag_pt = flag.location;
		}

		// store the current point
		prev_flag_pt = flag.location;
		first_flag = UUcFalse;
		flag_id++;

	} while (!abort);

	if (!found) {
		// could not find any flags, abort
		return UUcFalse;
	} else {
		// return the found point
		*outPoint = found_pt;
		if (outFacing != NULL) {
			*outFacing = found_facing;
		}
		return UUcTrue;
	}
}

static UUtBool AI2iBehavior_FindRetreatFlag(M3tPoint3D *inEnemy, float inDesiredFacing, float inFacingDelta,
											float inDesiredRange, float inRangeDelta, UUtUns16 inBaseFlagID, M3tPoint3D *outPoint)
{
	UUtUns16 flag_id;
	ONtFlag flag;
	UUtBool found = UUcFalse;
	M3tPoint3D found_pt, delta_vec;
	float score, found_score, range, angle, delta;

	for (flag_id = inBaseFlagID; ; flag_id++) {
		if (!ONrLevel_Flag_ID_To_Flag(flag_id, &flag)) {
			// no more flags, return the result of our search
			if (found) {
				*outPoint = found_pt;
			}
			return found;
		}

		// calculate range and angle to this flag from our enemy
		MUmVector_Subtract(delta_vec, flag.location, *inEnemy);
		range = MUmVector_GetLength(delta_vec);
		angle = MUrATan2(delta_vec.x, delta_vec.z) - inDesiredFacing;
		UUmTrig_ClipAbsPi(angle);

		// determine score
		delta = (float)fabs(range - inDesiredRange);
		if (delta > inRangeDelta) {
			score = 0.2f;
		} else {
			score = 1.0f - 0.8f * delta / inRangeDelta;
		}
		delta = (float)fabs(angle);
		if (delta > inFacingDelta) {
			score *= 0.2f;
		} else {
			score *= 1.0f - 0.8f * delta / inFacingDelta;
		}

		if (!found || (score > found_score)) {
			found_score = score;
			found_pt = flag.location;
			found = UUcTrue;
		}
	}
}

// ------------------------------------------------------------------------------------
// -- specific behavior functions

static UUtUns32 AI2iBehavior_Stare(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3)
{
	*inHandled = UUcTrue;

	switch(inMsg) {
	case AI2cCombatMessage_BeginBehavior:
		// nothing to set up in our internal state
		return 0;
	
	case AI2cCombatMessage_PrimaryMovement:
		// stand still
		ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Hold] = 1.0f;
		return 0;

	case AI2cCombatMessage_SecondaryMovement:
		// dodge incoming fire
		ioCombatState->dodge_enable = UUcTrue;
		return 0;

	case AI2cCombatMessage_Targeting:
		// we want to look at the character and will move to follow as necessary
		ioCombatState->desire_env_los = UUcTrue;
		ioCombatState->desire_friendly_los = UUcFalse;
		ioCombatState->run_targeting = UUcTrue;
		ioCombatState->aim_weapon = UUcFalse;
		return 0;

	case AI2cCombatMessage_EndBehavior:
		// nothing to deallocate in our internal state
		return 0;

	default:
		*inHandled = UUcFalse;
		return 0;
	}
}

static UUtUns32 AI2iBehavior_HoldAndFire(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3)
{
	*inHandled = UUcTrue;

	if ((inMsg != AI2cCombatMessage_BeginBehavior) && (inMsg != AI2cCombatMessage_EndBehavior)) {
		// this behavior can only execute if we have a gun
		if ((ioCharacter->inventory.weapons[0] == NULL) || ONrCharacter_OutOfAmmo(ioCharacter)) {
			return AI2iTryToFindGunBehavior(ioCharacter, ioCombatState, inMsg, inHandled, inParam1, inParam2, inParam3);
		}
	}

	switch(inMsg) {
	case AI2cCombatMessage_BeginBehavior:
		// nothing to set up in our internal state
		return 0;
	
	case AI2cCombatMessage_PrimaryMovement:
		// stand still
		ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Hold] = 1.0f;
		return 0;

	case AI2cCombatMessage_SecondaryMovement:
		// dodge incoming fire
		ioCombatState->dodge_enable = UUcTrue;
		return 0;

	case AI2cCombatMessage_Targeting:
		// shoot at the target
		ioCombatState->desire_env_los = UUcTrue;
		ioCombatState->desire_friendly_los = UUcTrue;
		ioCombatState->run_targeting = UUcTrue;
		ioCombatState->aim_weapon = UUcTrue;
		ioCombatState->shoot_weapon = UUcTrue;
		return 0;

	case AI2cCombatMessage_EndBehavior:
		// nothing to deallocate in our internal state
		return 0;

	default:
		*inHandled = UUcFalse;
		return 0;
	}
}

static UUtUns32 AI2iBehavior_FiringCharge(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3)
{
	*inHandled = UUcTrue;

	if ((inMsg != AI2cCombatMessage_BeginBehavior) && (inMsg != AI2cCombatMessage_EndBehavior)) {
		// this behavior can only execute if we have a gun
		if ((ioCharacter->inventory.weapons[0] == NULL) || ONrCharacter_OutOfAmmo(ioCharacter)) {
			return AI2iTryToFindGunBehavior(ioCharacter, ioCombatState, inMsg, inHandled, inParam1, inParam2, inParam3);
		}
	}

	switch(inMsg) {
	case AI2cCombatMessage_BeginBehavior:
		// nothing to set up in our internal state
		return 0;
	
	case AI2cCombatMessage_PrimaryMovement:
		// move towards the target
		ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Advance] = 1.0f;
		ioCombatState->maneuver.advance_maxdist = 80.0f;
		ioCombatState->maneuver.advance_mindist = 20.0f;
		ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Hold] = 0.2f;
		return 0;

	case AI2cCombatMessage_SecondaryMovement:
		// dodge incoming fire
		ioCombatState->dodge_enable = UUcTrue;
		return 0;

	case AI2cCombatMessage_Targeting:
		// shoot at the target
		ioCombatState->desire_env_los = UUcTrue;
		ioCombatState->desire_friendly_los = UUcTrue;
		ioCombatState->run_targeting = UUcTrue;
		ioCombatState->aim_weapon = UUcTrue;
		ioCombatState->shoot_weapon = UUcTrue;
		return 0;

	case AI2cCombatMessage_EndBehavior:
		// nothing to deallocate in our internal state
		return 0;

	default:
		*inHandled = UUcFalse;
		return 0;
	}
}

static UUtUns32 AI2iBehavior_Melee(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3)
{
	*inHandled = UUcTrue;

	switch(inMsg) {
	case AI2cCombatMessage_BeginBehavior:
		// nothing to set up in our internal state
		return 0;
	
	case AI2cCombatMessage_PrimaryMovement:
		// go into melee
		ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Melee] = 1.0f;
		return 0;

	case AI2cCombatMessage_SecondaryMovement:
		// dodge incoming fire
		ioCombatState->dodge_enable = (ioCombatState->maneuver.primary_movement != AI2cPrimaryMovement_Melee);
		return 0;

	case AI2cCombatMessage_Targeting:
		// don't aim or shoot at the target, but look at them if they're in view
		ioCombatState->desire_env_los = UUcFalse;
		ioCombatState->desire_friendly_los = UUcFalse;
		ioCombatState->run_targeting = UUcTrue;
		ioCombatState->aim_weapon = UUcFalse;
		ioCombatState->shoot_weapon = UUcFalse;
		return 0;

	case AI2cCombatMessage_EndBehavior:
		// nothing to deallocate in our internal state
		return 0;

	default:
		*inHandled = UUcFalse;
		return 0;
	}
}

static UUtUns32 AI2iBehavior_RunForAlarm(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3)
{
	*inHandled = UUcTrue;

	if (inMsg == AI2cCombatMessage_PrimaryMovement) {
		// always try running for an alarm
		ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_FindAlarm] = 1.2f;
	}

	if ((ioCombatState->behavior_state.alarm.tried_alarm) && 
		(inMsg != AI2cCombatMessage_BeginBehavior) && (inMsg != AI2cCombatMessage_EndBehavior)) {
		// we can't find an alarm - run our try to find gun behavior
		if ((ioCharacter->inventory.weapons[0] == NULL) || ONrCharacter_OutOfAmmo(ioCharacter)) {
			return AI2iTryToFindGunBehavior(ioCharacter, ioCombatState, inMsg, inHandled, inParam1, inParam2, inParam3);
		}
	}

	switch(inMsg) {
	case AI2cCombatMessage_BeginBehavior:
		ioCombatState->behavior_state.alarm.tried_alarm = UUcFalse;
		return 0;
	
	case AI2cCombatMessage_PrimaryMovement:
		// if we can't find an alarm, run away
		ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Retreat] = 0.8f;
		return 0;

	case AI2cCombatMessage_FoundPrimaryMovement:
		if (ioCombatState->maneuver.primary_movement != AI2cPrimaryMovement_FindAlarm) {
			// we have been unable to find an alarm, so now we can enable our try-to-find gun behaviors
			ioCombatState->behavior_state.alarm.tried_alarm = UUcTrue;
		}
		return 0;

	case AI2cCombatMessage_SecondaryMovement:
		// dodge incoming fire
		ioCombatState->dodge_enable = UUcTrue;
		return 0;

	case AI2cCombatMessage_Targeting:
		// we will only get here if we can't find an alarm. shoot at the target.
		ioCombatState->desire_env_los = UUcTrue;
		ioCombatState->desire_friendly_los = UUcTrue;
		ioCombatState->run_targeting = UUcTrue;
		ioCombatState->aim_weapon = UUcTrue;
		ioCombatState->shoot_weapon = UUcTrue;
		return 0;

	case AI2cCombatMessage_EndBehavior:
		// nothing to deallocate in our internal state
		return 0;

	default:
		*inHandled = UUcFalse;
		return 0;
	}
}

// ------------------------------------------------------------------------------------
// -- barabbas' behavior functions

static void AI2iBehavior_BarabbasHurtDisableRegen(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
											   AI2tBehaviorState_Barabbas *ioBarabbasState)
{
	ioBarabbasState->regen_disable_timer = AI2cBarabbas_RegenHurtDisableTimerMin +
		ioCharacter->hitPoints * (AI2cBarabbas_RegenHurtDisableTimerMax - AI2cBarabbas_RegenHurtDisableTimerMin) / ioCharacter->maxHitPoints;
}

// does barabbas want to flee and regenerate?
static UUtBool AI2iBehavior_BarabbasCheckRegen(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
											   AI2tBehaviorState_Barabbas *ioBarabbasState)
{
	float health_fraction, damage_fraction, required_distance;

	health_fraction = (100.0f * ioCharacter->hitPoints) / ioCharacter->maxHitPoints;
#if AI_DEBUG_BARABBAS
	COrConsole_Printf("barabbas health %02.1f%%", health_fraction);
#endif
	if (ioBarabbasState->prev_barabbas_mode == AI2cBarabbas_Regenerate) {
		// if we've used up all our regeneration hit points, stop regenerating.
		// this only applies to barabbas' research lab incarnation (with the unkillable flag).
		if (ioCharacter->flags & ONcCharacterFlag_Unkillable) {
			if (ioCharacter->ai2State.regenerated_amount >= (AI2cBarabbas_RegenMaxHitPointPool * ioCharacter->maxHitPoints) / 100) {
				ioBarabbasState->regen_disable_timer = 10000;
#if AI_DEBUG_BARABBAS
				COrConsole_Printf("barabbas ran out of regeneration hitpoints");
#endif
				return UUcFalse;
			}
		}

		// stop regenerating if we take enough damage while doing so
		damage_fraction = (100.0f * ioBarabbasState->regen_damage_taken) / ioCharacter->maxHitPoints;
		if (damage_fraction >= AI2cBarabbas_RegenDamageStopFraction) {
#if AI_DEBUG_BARABBAS
			COrConsole_Printf("barabbas hurt %d, stopping regen", ioBarabbasState->regen_damage_taken);
#endif
			AI2iBehavior_BarabbasHurtDisableRegen(ioCharacter, ioCombatState, ioBarabbasState);
			ioBarabbasState->regen_damage_taken = 0;
			return UUcFalse;
		}

		// regenerate only up to a certain hit point level
		if (health_fraction > AI2cBarabbas_RegenFullFraction) {
			// we are done; don't regenerate again any time soon
			ioBarabbasState->regen_disable_timer = AI2cBarabbas_RegenRestartDisableTimer;
#if AI_DEBUG_BARABBAS
			COrConsole_Printf("barabbas healed");
#endif
			return UUcFalse;
		}

		if (ioCombatState->distance_to_target < AI2cBarabbas_RegenAbortDistance) {
#if AI_DEBUG_BARABBAS
			COrConsole_Printf("barabbas stop regenerating, too close (%f < %f)", ioCombatState->distance_to_target, AI2cBarabbas_RegenAbortDistance);
#endif
			if (health_fraction < AI2cBarabbas_HealthCriticalFraction) {
				// we are critically wounded, flee
#if AI_DEBUG_BARABBAS
				COrConsole_Printf("barabbas critically wounded, fleeing to regenerate more");
#endif
				ioBarabbasState->barabbas_mode = AI2cBarabbas_Flee;
				return UUcTrue;
			} else {
				// perform our main combat mode as set up outside this call
				return UUcFalse;
			}
		} else {
#if AI_DEBUG_BARABBAS
			COrConsole_Printf("barabbas continue healing");
#endif
			ioBarabbasState->barabbas_mode = AI2cBarabbas_Regenerate;
			return UUcTrue;
		}
	} else {
		if (ioCharacter->flags & ONcCharacterFlag_Unkillable) {
			// don't start regenerating if we've already used up all our regeneration hit points.
			// this only applies to barabbas' research lab incarnation (with the unkillable flag).
			if (ioCharacter->ai2State.regenerated_amount >= (AI2cBarabbas_RegenMaxHitPointPool * ioCharacter->maxHitPoints) / 100) {
				return UUcFalse;
			}
		}

		// don't start regenerating unless we are hurt
		if (health_fraction > AI2cBarabbas_RegenHealthFraction) {
			return UUcFalse;
		}

		// don't start regenerating if we have been hurt recently
		if (ioBarabbasState->regen_disable_timer > 0) {
#if AI_DEBUG_BARABBAS
			COrConsole_Printf("barabbas can't regen yet (timer %d)", ioBarabbasState->regen_disable_timer);
#endif
			return UUcFalse;
		}

		// don't start regenerating if we are too close to the target
		required_distance = (ioBarabbasState->barabbas_mode == AI2cBarabbas_Flee) ? AI2cBarabbas_RegenFleeReqDistance : AI2cBarabbas_RegenMinDistance;
		if (ioCombatState->distance_to_target < required_distance) {
#if AI_DEBUG_BARABBAS
			COrConsole_Printf("barabbas too close to start regen (%f < %f), fleeing", ioCombatState->distance_to_target, required_distance);
#endif
			ioBarabbasState->barabbas_mode = AI2cBarabbas_Flee;
			return UUcTrue;
		}

		// don't start regenerating if we aren't standing up
		if (!ONrCharacter_IsInactiveUpright(ioCharacter)) {
#if AI_DEBUG_BARABBAS
			COrConsole_Printf("barabbas can't regen from this state (%s), fleeing", ONrAnimTypeToString(ONrForceActiveCharacter(ioCharacter)->curAnimType));
#endif
			ioBarabbasState->barabbas_mode = AI2cBarabbas_Flee;
			return UUcTrue;
		}

		// start regenerating!
#if AI_DEBUG_BARABBAS
		COrConsole_Printf("barabbas starting regen");
#endif
		ioBarabbasState->barabbas_mode = AI2cBarabbas_Regenerate;
		return UUcTrue;
	}
}

static UUtUns32 AI2iBehavior_Barabbas(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3)
{
	ONtActiveCharacter *active_character = ONrForceActiveCharacter(ioCharacter);
	AI2tBehaviorState_Barabbas *barabbas_state;
	UUtUns32 heal_amount;

	barabbas_state = &ioCombatState->behavior_state.barabbas;
	*inHandled = UUcTrue;

	switch(inMsg) {
	case AI2cCombatMessage_BeginBehavior:
		switch((AI2tBehaviorType) inParam1) {
			case AI2cBehavior_BarabbasShoot:
			case AI2cBehavior_BarabbasAdvance:
			case AI2cBehavior_BarabbasMelee:
				// we were already in a barabbas combat behavior, don't reset this stuff
				break;

			default:
				// initialise barabbas's combat behavior
				barabbas_state->prev_barabbas_mode = AI2cBarabbas_None;
				barabbas_state->disable_beam_timer = 0;
				barabbas_state->regen_disable_timer = 0;
				barabbas_state->regen_damage_taken = 0;
				barabbas_state->fleeing_timer = 0;
				break;
		}
		return 0;
	
	case AI2cCombatMessage_PrimaryMovement:
		if (barabbas_state->disable_beam_timer > AI2cCombat_UpdateFrames) {
			barabbas_state->disable_beam_timer -= AI2cCombat_UpdateFrames;
		} else {
			barabbas_state->disable_beam_timer = 0;
		}

		if (barabbas_state->regen_disable_timer > AI2cCombat_UpdateFrames) {
			barabbas_state->regen_disable_timer -= AI2cCombat_UpdateFrames;
		} else {
			barabbas_state->regen_disable_timer = 0;
		}

		if (ONrCharacter_IsInactiveUpright(ioCharacter)) {
			// we are (or could be) fleeing
			if (barabbas_state->fleeing_timer > AI2cCombat_UpdateFrames) {
#if AI_DEBUG_BARABBAS
				COrConsole_Printf("barabbas flee timer %d", barabbas_state->fleeing_timer);
#endif
				barabbas_state->fleeing_timer -= AI2cCombat_UpdateFrames;
			} else {
				if (barabbas_state->fleeing_timer > 0) {
					// our fleeing timer has run out, we can't run away successfully. stop trying to regenerate.
					barabbas_state->fleeing_timer = 0;
					barabbas_state->regen_disable_timer = AI2cBarabbas_RegenCantFleeDisableTimer;
#if AI_DEBUG_BARABBAS
					COrConsole_Printf("barabbas flee timer ran out, disabling regen (%d)", barabbas_state->regen_disable_timer);
#endif
				}
			}
		}

		if ((ioCharacter->inventory.weapons[0] != NULL) && (WPrIsFiring(ioCharacter->inventory.weapons[0])) && (!ioCombatState->alternate_fire)) {
			// barabbas has to wait for his beam weapon to finish firing
			barabbas_state->barabbas_mode = AI2cBarabbas_WaitForWeapon;

		} else if (AI2iBehavior_BarabbasCheckRegen(ioCharacter, ioCombatState, barabbas_state)) {
			// barabbas wants to flee and regenerate

		} else if ((ioCharacter->inventory.weapons[0] == NULL) || ONrCharacter_OutOfAmmo(ioCharacter)) {
			// barabbas wants to get his gun
			barabbas_state->barabbas_mode = AI2cBarabbas_GetGun;

		} else {
			// otherwise, use the behavior mode set up by whatever called us (the more specific barabbas behavior)
		}

		if (AI2gBarabbasRun ||
			(barabbas_state->barabbas_mode == AI2cBarabbas_GetGun) ||
			(barabbas_state->barabbas_mode == AI2cBarabbas_Regenerate) ||
			(barabbas_state->barabbas_mode == AI2cBarabbas_Flee)) {
			AI2rMovement_ChangeMovementMode(ioCharacter, AI2cMovementMode_Run);
		} else {
			AI2rMovement_ChangeMovementMode(ioCharacter, AI2cMovementMode_Walk);
		}

		if (barabbas_state->barabbas_mode != barabbas_state->prev_barabbas_mode) {
#if AI_DEBUG_BARABBAS
			{
				// TEMPORARY DEBUGGING
				const char *barabbas_mode[] = {"none", "shoot-beam", "wait-for-weapon", "advance", "advance-grenading", "get-gun", "regenerate", "melee", "flee"};

				COrConsole_Printf("barabbas mode switch to %s", barabbas_mode[barabbas_state->barabbas_mode]);
			}
#endif
			if (barabbas_state->prev_barabbas_mode == AI2cBarabbas_Regenerate) {
				// drop out of the powerup animation
				ONrCharacter_DoAnimation(ioCharacter, active_character, ONcAnimPriority_Force, ONcAnimType_Stand);
			}

			// we are switching to a new mode, reset internal state
			if (barabbas_state->barabbas_mode == AI2cBarabbas_Regenerate) {
				barabbas_state->regen_damage_taken = 0;
				barabbas_state->fleeing_timer = 0;
			} else if (barabbas_state->barabbas_mode == AI2cBarabbas_Flee) {
				barabbas_state->fleeing_timer = AI2cBarabbas_FleeMaxTimer;
			}
		}
		barabbas_state->prev_barabbas_mode = barabbas_state->barabbas_mode;

		switch(barabbas_state->barabbas_mode) {
			case AI2cBarabbas_WaitForWeapon:
			case AI2cBarabbas_ShootBeam:
				ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Hold] = 1.0f;
				break;

			case AI2cBarabbas_Advance:
			case AI2cBarabbas_AdvanceGrenading:
				ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Hold] = 0.3f;
				ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Advance] = 1.0f;
				break;

			case AI2cBarabbas_GetGun:
				if (ioCombatState->distance_to_target > AI2cBarabbas_MeleeInsteadOfGunRange) {
					ioCombatState->maneuver.gun_search_minradius = AI2cBarabbas_GunMinDist;
					ioCombatState->maneuver.gun_search_maxradius = AI2cBarabbas_GunMaxDist;
					ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Gun] = 1.0f;
				}
				ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Melee] = 0.8f;
				break;

			case AI2cBarabbas_Regenerate:
				ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Hold] = 1.0f;
				if (ONrCharacter_IsPoweringUp(ioCharacter)) {
					heal_amount = (ioCharacter->maxHitPoints * AI2cBarabbas_RegenHealFractionPerSecond) / (100 * (UUcFramesPerSecond / AI2cCombat_UpdateFrames));
					ONrCharacter_Heal(ioCharacter, heal_amount, UUcFalse);
					ioCharacter->ai2State.regenerated_amount += heal_amount;
				}
				ONrCharacter_FightMode(ioCharacter);
				ONrCharacter_DoAnimation(ioCharacter, active_character, ONcAnimPriority_Appropriate, ONcAnimType_Powerup);
				break;

			case AI2cBarabbas_Flee:
				ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Hold] = 0.3f;
				ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Retreat] = 1.0f;
				ioCombatState->maneuver.retreat_mindist = AI2cBarabbas_FleeMinDist;
				ioCombatState->maneuver.retreat_maxdist = AI2cBarabbas_FleeMaxDist;
				break;

			case AI2cBarabbas_Melee:
				ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Melee] = 1.0f;
				break;
		}

		return 0;

	case AI2cCombatMessage_SecondaryMovement:
		// don't dodge incoming fire
		ioCombatState->dodge_enable = UUcFalse;
		return 0;

	case AI2cCombatMessage_Targeting:
		if ((barabbas_state->barabbas_mode == AI2cBarabbas_Melee) ||
			(barabbas_state->barabbas_mode == AI2cBarabbas_Regenerate) ||
			(barabbas_state->barabbas_mode == AI2cBarabbas_Flee)) {
			// don't aim weapon
			ioCombatState->desire_env_los = UUcFalse;
			ioCombatState->desire_friendly_los = UUcFalse;
			ioCombatState->run_targeting = UUcTrue;
			ioCombatState->aim_weapon = UUcFalse;
			ioCombatState->shoot_weapon = UUcFalse;
			return 0;
		}

		ioCombatState->desire_env_los = UUcTrue;
		ioCombatState->desire_friendly_los = UUcTrue;
		ioCombatState->run_targeting = UUcTrue;
		ioCombatState->aim_weapon = UUcTrue;

		switch(barabbas_state->barabbas_mode) {
			case AI2cBarabbas_Advance:
			case AI2cBarabbas_WaitForWeapon:
			case AI2cBarabbas_GetGun:
				ioCombatState->shoot_weapon = UUcFalse;
				break;

			case AI2cBarabbas_ShootBeam:
				ioCombatState->shoot_weapon = UUcTrue;
				ioCombatState->alternate_fire = UUcFalse;
				break;

			case AI2cBarabbas_AdvanceGrenading:
				ioCombatState->shoot_weapon = UUcTrue;
				ioCombatState->alternate_fire = UUcTrue;
				break;
		}
		return 0;

	case AI2cCombatMessage_LookAtTarget:
		switch(barabbas_state->barabbas_mode) {
			case AI2cBarabbas_WaitForWeapon:
			case AI2cBarabbas_ShootBeam:
			case AI2cBarabbas_AdvanceGrenading:
				// always look at the target
				return UUcTrue;
		}

		// use default behavior
		*inHandled = UUcFalse;
		return 0;

	case AI2cCombatMessage_Hurt:
		if (barabbas_state->barabbas_mode == AI2cBarabbas_Regenerate) {
			barabbas_state->regen_damage_taken += inParam1;
		} else {
			AI2iBehavior_BarabbasHurtDisableRegen(ioCharacter, ioCombatState, barabbas_state);
		}
		return 0;

	case AI2cCombatMessage_Fallen:
		AI2iBehavior_BarabbasHurtDisableRegen(ioCharacter, ioCombatState, barabbas_state);

		// fall through to default behavior for daze handling
		*inHandled = UUcFalse;
		return 0;

	case AI2cCombatMessage_TooClose:
		switch(barabbas_state->barabbas_mode) {
			case AI2cBarabbas_WaitForWeapon:
			case AI2cBarabbas_GetGun:
			case AI2cBarabbas_Regenerate:
			case AI2cBarabbas_Flee:
			case AI2cBarabbas_Melee:
				// don't do anything about this
				return 0;
		}
		// fall through to default behavior
		*inHandled = UUcFalse;
		return 0;

	case AI2cCombatMessage_EndBehavior:
		// nothing to deallocate in our internal state
		return 0;

	default:
		*inHandled = UUcFalse;
		return 0;
	}
}

static UUtUns32 AI2iBehavior_BarabbasShoot(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3)
{
	UUtBool fired, alternate;

	*inHandled = UUcFalse;

	switch(inMsg) {
	case AI2cCombatMessage_BeginBehavior:
		ioCombatState->behavior_state.barabbas.barabbas_mode = AI2cBarabbas_ShootBeam;
		if (!AI2gBarabbasRun) {
			AI2rMovement_ChangeMovementMode(ioCharacter, AI2cMovementMode_Walk);
		}
		break;
	
	case AI2cCombatMessage_PrimaryMovement:
		if (ioCombatState->behavior_state.barabbas.disable_beam_timer) {
			ioCombatState->behavior_state.barabbas.barabbas_mode = AI2cBarabbas_WaitForWeapon;
		} else {
			ioCombatState->behavior_state.barabbas.barabbas_mode = AI2cBarabbas_ShootBeam;
		}
		break;

	case AI2cCombatMessage_Fire:
		fired = (UUtBool) inParam1;
		alternate = (UUtBool) inParam2;

		if (fired && !alternate) {
			ioCombatState->behavior_state.barabbas.disable_beam_timer = AI2cBarabbas_StationaryBeamTimer;
		}
		*inHandled = UUcTrue;
		return 0;

	case AI2cCombatMessage_EndBehavior:
		AI2rMovement_ChangeMovementMode(ioCharacter, AI2cMovementMode_Default);
		break;
	}

	if (!*inHandled) {
		return AI2iBehavior_Barabbas(ioCharacter, ioCombatState, inMsg, inHandled, inParam1, inParam2, inParam3);
	} else {
		return 0;
	}
}

static UUtUns32 AI2iBehavior_BarabbasAdvance(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3)
{
	UUtBool fired, alternate;

	*inHandled = UUcFalse;

	switch(inMsg) {
	case AI2cCombatMessage_BeginBehavior:
		ioCombatState->behavior_state.barabbas.barabbas_mode = AI2cBarabbas_ShootBeam;
		break;
	
	case AI2cCombatMessage_PrimaryMovement:
		if (ioCombatState->behavior_state.barabbas.disable_beam_timer) {
			ioCombatState->behavior_state.barabbas.barabbas_mode = AI2cBarabbas_AdvanceGrenading;
		} else {
			ioCombatState->behavior_state.barabbas.barabbas_mode = AI2cBarabbas_ShootBeam;
		}
		break;

	case AI2cCombatMessage_Fire:
		fired = (UUtBool) inParam1;
		alternate = (UUtBool) inParam2;

		if (fired && !alternate) {
			ioCombatState->behavior_state.barabbas.disable_beam_timer = UUmRandomRange(AI2cBarabbas_AdvancingBeamTimer, AI2cBarabbas_AdvancingBeamDeltaTimer);
		}
		*inHandled = UUcTrue;
		return 0;

	case AI2cCombatMessage_EndBehavior:
		AI2rMovement_ChangeMovementMode(ioCharacter, AI2cMovementMode_Default);
		break;
	}

	if (!*inHandled) {
		return AI2iBehavior_Barabbas(ioCharacter, ioCombatState, inMsg, inHandled, inParam1, inParam2, inParam3);
	} else {
		return 0;
	}
}

static UUtUns32 AI2iBehavior_BarabbasMelee(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3)
{
	*inHandled = UUcFalse;

	switch(inMsg) {
	case AI2cCombatMessage_BeginBehavior:
		ioCombatState->behavior_state.barabbas.barabbas_mode = AI2cBarabbas_Melee;
		break;
	
	case AI2cCombatMessage_PrimaryMovement:
		if ((ioCharacter->inventory.weapons[0] != NULL) && (WPrIsFiring(ioCharacter->inventory.weapons[0]))) {
			// barabbas has to wait for his gun to finish firing
			ioCombatState->behavior_state.barabbas.barabbas_mode = AI2cBarabbas_WaitForWeapon;
		} else {
			// barabbas wants to go into melee
			ioCombatState->behavior_state.barabbas.barabbas_mode = AI2cBarabbas_Melee;
		}
		break;
	}

	if (!*inHandled) {
		return AI2iBehavior_Barabbas(ioCharacter, ioCombatState, inMsg, inHandled, inParam1, inParam2, inParam3);
	} else {
		return 0;
	}
}

// ------------------------------------------------------------------------------------
// -- super ninja's behavior functions

// is a super-ninja mode a teleport?
static UUtBool AI2iSuperNinjaMode_IsTeleport(UUtUns32 inMode)
{
	switch(inMode) {
		case AI2cSuperNinja_TeleportFlee:
		case AI2cSuperNinja_TeleportAdvance:
			return UUcTrue;

		default:
			return UUcFalse;
	}
}

// is super ninja busy teleporting?
static UUtBool AI2iBehavior_SuperNinjaContinueTeleport(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
														AI2tBehaviorState_SuperNinja *ioNinjaState)
{
	if (!AI2iSuperNinjaMode_IsTeleport(ioNinjaState->superninja_mode)) {
		return UUcFalse;
	}

	if (ONrCharacter_IsFallen(ioCharacter) || ONrCharacter_IsVictimAnimation(ioCharacter)) {
		return UUcFalse;
	}

	return UUcTrue;
}

// is super ninja's medium-range advancing teleport behavior enabled?
static UUtBool AI2iBehavior_SuperNinjaTeleportAdvanceEnabled(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
											   AI2tBehaviorState_SuperNinja *ioNinjaState)
{
	UUtUns16 random_val;

	if (!ioNinjaState->set_mediumrange_behavior) {
		// use random chance to work out whether we will teleport behind target, or use invisibility then run in close
		random_val = UUrRandom();
		ioNinjaState->mediumrange_enable_teleport = (random_val < (UUtUns16) (AI2cSuperNinja_AdvanceTeleportChance * UUcMaxUns16));
		ioNinjaState->set_mediumrange_behavior = UUcTrue;

#if AI_DEBUG_SUPERNINJA
		COrConsole_Printf("superninja: medium-range teleport %s (rand-val %f, threshold %f)",
				ioNinjaState->mediumrange_enable_teleport ? "enabled" : "disabled", ((float) random_val) / UUcMaxUns16, AI2cSuperNinja_AdvanceTeleportChance);
#endif
	}

	return ioNinjaState->mediumrange_enable_teleport;
}

// does super ninja want to turn invisible?
static UUtBool AI2iBehavior_SuperNinjaCheckInvis(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
											   AI2tBehaviorState_SuperNinja *ioNinjaState)
{
	if ((ioCharacter->inventory.invisibilityRemaining > 0) || (ioNinjaState->disable_invis_timer > 0)) {
		return UUcFalse;
	}

	// don't try to turn invisible if we aren't standing up
	if (!ONrCharacter_IsInactiveUpright(ioCharacter)) {
#if AI_DEBUG_SUPERNINJA
		COrConsole_Printf("superninja can't turn invisible from this animtype (%s)", ONrAnimTypeToString(ONrForceActiveCharacter(ioCharacter)->curAnimType));
#endif
		return UUcFalse;
	}

	// don't override more important behaviors
	switch(ioNinjaState->superninja_mode) {
		case AI2cSuperNinja_Fireball:
			return UUcFalse;
	}

	if (ioCombatState->distance_to_target < AI2cSuperNinja_InvisMinDistance) {
#if AI_DEBUG_SUPERNINJA
		COrConsole_Printf("superninja invis tooclose (%f < %f)", ioCombatState->distance_to_target, AI2cSuperNinja_InvisMinDistance);
#endif
		return UUcFalse;
	}

	if (AI2iBehavior_SuperNinjaTeleportAdvanceEnabled(ioCharacter, ioCombatState, ioNinjaState)) {
		// our medium-range behavior is to teleport closer instead of turning invisible
#if AI_DEBUG_SUPERNINJA
		COrConsole_Printf("superninja: don't turn invisible, teleport at medium range instead");
#endif
		return UUcFalse;
	} else {
		// try to run our invisibility move
#if AI_DEBUG_SUPERNINJA
		COrConsole_Printf("superninja trying to turn invisible");
#endif
		return UUcTrue;
	}
}

// does super ninja want to teleport to flee?
static UUtBool AI2iBehavior_SuperNinjaCheckTeleportFlee(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
											   AI2tBehaviorState_SuperNinja *ioNinjaState)
{
	// don't teleport while invisible
	if (ioCharacter->inventory.invisibilityRemaining > 0) 
		return UUcFalse;

	if (ioNinjaState->flee_damage_taken < AI2cSuperNinja_FleeTeleportDamage)
		return UUcFalse;
	
	if (ioCombatState->distance_to_target >= AI2cSuperNinja_FleeTeleportRange) {
		return UUcFalse;
	}

	// don't try to teleport if we aren't standing up
	return ONrCharacter_IsInactiveUpright(ioCharacter);
}

// does super ninja want to teleport in close to the target?
static UUtBool AI2iBehavior_SuperNinjaCheckTeleportAdvance(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
											   AI2tBehaviorState_SuperNinja *ioNinjaState)
{
	// don't teleport while invisible
	if (ioCharacter->inventory.invisibilityRemaining > 0) 
		return UUcFalse;

	if (ioNinjaState->superninja_mode != AI2cSuperNinja_Advance)
		return UUcFalse;
	
	if (ioCombatState->distance_to_target < AI2cSuperNinja_AdvanceTeleportRange) {
#if AI_DEBUG_SUPERNINJA
		COrConsole_Printf("superninja advance: too close to teleport (%f < %f)",
			ioCombatState->distance_to_target, AI2cSuperNinja_AdvanceTeleportRange);
#endif
		return UUcFalse;
	}

	// don't try to teleport if we aren't standing up
	if (!ONrCharacter_IsInactiveUpright(ioCharacter))
		return UUcFalse;

	return AI2iBehavior_SuperNinjaTeleportAdvanceEnabled(ioCharacter, ioCombatState, ioNinjaState);
}

static UUtUns32 AI2iBehavior_SuperNinja(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3)
{
	ONtActiveCharacter *active_character = ONrForceActiveCharacter(ioCharacter);
	AI2tBehaviorState_SuperNinja *superninja_state;
	const TRtAnimation *animation;
	UUtBool localpath_success, localpath_escape, found_point;
	M3tPoint3D localpath_endpoint, desired_destination;
	UUtUns8 localpath_weight;
	float theta, desired_facing;
	UUtError error;

	superninja_state = &ioCombatState->behavior_state.superNinja;
	*inHandled = UUcTrue;

	switch(inMsg) {
	case AI2cCombatMessage_BeginBehavior:
		// initialise generic behavior control variables
		ioCombatState->fall_detect_enable = UUcTrue;
		ioCombatState->fall_height = AI2cSuperNinja_FallTeleportHeight;

		switch((AI2tBehaviorType) inParam1) {
			case AI2cBehavior_SuperNinjaFireball:
			case AI2cBehavior_SuperNinjaAdvance:
			case AI2cBehavior_SuperNinjaMelee:
				// we were already in a superninja combat behavior, don't reset this stuff
				break;

			default:
				// initialise super ninja's combat behavior
				UUrMemory_Clear(superninja_state, sizeof(*superninja_state));
				break;
		}
		return 0;
	
	case AI2cCombatMessage_PrimaryMovement:
		if (superninja_state->disable_fireball_timer > AI2cCombat_UpdateFrames) {
			superninja_state->disable_fireball_timer -= AI2cCombat_UpdateFrames;
		} else {
			superninja_state->disable_fireball_timer = 0;
		}

		if (superninja_state->disable_invis_timer > AI2cCombat_UpdateFrames) {
			superninja_state->disable_invis_timer -= AI2cCombat_UpdateFrames;
		} else {
			superninja_state->disable_invis_timer = 0;
		}

		if (superninja_state->disable_teleport_timer > AI2cCombat_UpdateFrames) {
			superninja_state->disable_teleport_timer -= AI2cCombat_UpdateFrames;
		} else {
			superninja_state->disable_teleport_timer = 0;
		}

		if (superninja_state->flee_tick_counter == 0) {
			superninja_state->flee_tick_counter = AI2cSuperNinja_DamageDecayFrames / AI2cCombat_UpdateFrames;

			if (superninja_state->flee_damagestay_timer > AI2cSuperNinja_DamageDecayFrames) {
				superninja_state->flee_damagestay_timer -= AI2cSuperNinja_DamageDecayFrames;
#if AI_DEBUG_SUPERNINJA
				COrConsole_Printf("superninja: flee damage staying at %d (%d more frames)",
							superninja_state->flee_damage_taken, superninja_state->flee_damagestay_timer);
#endif						
			} else {
				if (superninja_state->flee_damage_taken > AI2cSuperNinja_DamageDecayAmount) {
					superninja_state->flee_damage_taken -= AI2cSuperNinja_DamageDecayAmount;
#if AI_DEBUG_SUPERNINJA
					COrConsole_Printf("superninja: flee damage decays to %d", superninja_state->flee_damage_taken);
#endif						
				} else {
					superninja_state->flee_damage_taken = 0;
				}
			}
		}
		superninja_state->flee_tick_counter--;

		if (AI2iBehavior_SuperNinjaContinueTeleport(ioCharacter, ioCombatState, superninja_state)) {
			// we are in the middle of a teleportation move, keep doing it regardless of what
			// our higher-level behavior says
			superninja_state->superninja_mode = superninja_state->prev_superninja_mode;

		} else if (AI2iBehavior_SuperNinjaCheckTeleportFlee(ioCharacter, ioCombatState, superninja_state)) {
			// super ninja is being pummelled and wants to flee
			superninja_state->superninja_mode = AI2cSuperNinja_TeleportFlee;
#if AI_DEBUG_SUPERNINJA
			COrConsole_Printf("superninja: starting flee teleport");
#endif						

		} else if (AI2iBehavior_SuperNinjaCheckTeleportAdvance(ioCharacter, ioCombatState, superninja_state)) {
			// super ninja would rather close the distance to target by teleporting in
			superninja_state->superninja_mode = AI2cSuperNinja_TeleportAdvance;
#if AI_DEBUG_SUPERNINJA
			COrConsole_Printf("superninja: starting close teleport");
#endif						

		} else if (AI2iBehavior_SuperNinjaCheckInvis(ioCharacter, ioCombatState, superninja_state)) {
			// super ninja wants to turn invisible
			superninja_state->superninja_mode = AI2cSuperNinja_Invisible;

		} else {
			// super ninja uses behavior mode set up by higher-level behavior control
		}


		if (superninja_state->superninja_mode != superninja_state->prev_superninja_mode) {
#if AI_DEBUG_SUPERNINJA
			{
				// TEMPORARY DEBUGGING
				const char *superninja_mode[] = {"none", "melee", "fireball", "advance", "invisible", "teleport-flee", "teleport-advance", "teleport-fall"};

				COrConsole_Printf("superninja mode switch to %s", superninja_mode[superninja_state->superninja_mode]);
			}
#endif
			// handle exit from previous mode
			if (superninja_state->prev_superninja_mode == AI2cSuperNinja_Fireball) {
				superninja_state->fireball_started = UUcFalse;

			} else if (superninja_state->prev_superninja_mode == AI2cSuperNinja_Invisible) {
				superninja_state->invis_started = UUcFalse;
			}

			// handle entry to new mode
			switch(superninja_state->superninja_mode) {
				case AI2cSuperNinja_TeleportFlee:
					// find a retreat flag which is in the right direction and distance
					active_character->teleportEnable = AI2iBehavior_FindRetreatFlag(&ioCombatState->target->actual_position,
												ONrCharacter_GetCosmeticFacing(ioCombatState->target), AI2cSuperNinja_TeleportRetreatAngleDelta,
												AI2cSuperNinja_TeleportRetreatRange, AI2cSuperNinja_TeleportRetreatRangeDelta,
												AI2cSuperNinja_RetreatFlagBase, &active_character->teleportDestination);
					if (active_character->teleportEnable) {
						theta = MUrATan2(ioCombatState->target->actual_position.x - active_character->teleportDestination.x,
										 ioCombatState->target->actual_position.z - active_character->teleportDestination.z);
						UUmTrig_ClipLow(theta);

						active_character->teleportDestFacing = theta;
					} else {
						// no suitable points behind target were found - abort the teleport and continue to fight in melee
						superninja_state->superninja_mode = AI2cSuperNinja_Melee;
#if AI_DEBUG_SUPERNINJA
						COrConsole_Printf("superninja: found no suitable points to teleport retreat to, aborting");
#endif						
					}
					break;

				case AI2cSuperNinja_TeleportAdvance:
					// try to find a point behind the target and relatively close by, that isn't blocked by a wall or the edge
					theta = ONrCharacter_GetCosmeticFacing(ioCombatState->target) + M3cPi;
					UUmTrig_ClipHigh(theta);

					error = AI2rFindNearbyPoint(ioCharacter, PHcPathMode_CasualMovement, &ioCombatState->target->actual_position,
												AI2cSuperNinja_TeleportBehindDistance, AI2cSuperNinja_TeleportBehindMinDistance, theta, M3cHalfPi / 6, 6,
												PHcSemiPassable, 3, &localpath_success, &localpath_endpoint, &localpath_weight, &localpath_escape);
					if (error == UUcError_None) {
						// set up a teleport to this location
						theta = MUrATan2(ioCombatState->target->actual_position.x - localpath_endpoint.x,
										 ioCombatState->target->actual_position.z - localpath_endpoint.z);
						UUmTrig_ClipLow(theta);

						active_character->teleportEnable = UUcTrue;
						active_character->teleportDestination = localpath_endpoint;
						active_character->teleportDestFacing = theta;
					} else {
						// no suitable points behind target were found - abort the teleport and just walk up normally
						superninja_state->superninja_mode = AI2cSuperNinja_Advance;
#if AI_DEBUG_SUPERNINJA
						COrConsole_Printf("superninja: found no suitable points to teleport behind, aborting");
#endif						
					}
					break;
			}
		}
		superninja_state->prev_superninja_mode = superninja_state->superninja_mode;

		switch(superninja_state->superninja_mode) {
			case AI2cSuperNinja_Fireball:
				// stand still
				ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Hold] = 1.0f;

				if (!superninja_state->fireball_started) {
					if (ONrCharacter_IsStandingStill(ioCharacter)) {
#if AI_DEBUG_SUPERNINJA
						COrConsole_Printf("superninja: animtype %s curstate %s fromstate %s [queued %s] -> inactive and upright, do special move",
											ONrAnimTypeToString(active_character->curAnimType), ONrAnimStateToString(active_character->curFromState),
											ONrAnimStateToString(active_character->nextAnimState), ONrAnimTypeToString(active_character->nextAnimType));
#endif						
						// perform our fireball attack
						ONrCharacter_FightMode(ioCharacter);
						animation = ONrCharacter_DoAnimation(ioCharacter, active_character, ONcAnimPriority_Appropriate, ONcAnimType_Ninja_Fireball);
						if (animation != NULL) {
							superninja_state->fireball_started = UUcTrue;
#if AI_DEBUG_SUPERNINJA
							COrConsole_Printf("superninja: started fireball animation");
#endif
						}
					} else if (ONrCharacter_IsFallen(ioCharacter) || ONrAnimType_IsVictimType(active_character->curAnimType)) {
						// abort our fireball attack
						superninja_state->disable_fireball_timer = AI2cSuperNinja_UnsuccessfulSpecialMoveTimer;
#if AI_DEBUG_SUPERNINJA
						COrConsole_Printf("superninja: aborting fireball, somehow seem to be fallen or victim");
#endif
					}
				}
				break;

			case AI2cSuperNinja_Invisible:
				// stand still
				ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Hold] = 1.0f;

				if (!superninja_state->invis_started) {
					if (ONrCharacter_IsStandingStill(ioCharacter)) {
#if AI_DEBUG_SUPERNINJA
						COrConsole_Printf("superninja: animtype %s curstate %s fromstate %s [queued %s] -> inactive and upright, do special move",
											ONrAnimTypeToString(active_character->curAnimType), ONrAnimStateToString(active_character->curFromState),
											ONrAnimStateToString(active_character->nextAnimState), ONrAnimTypeToString(active_character->nextAnimType));
#endif						
						// perform our invisibility move
						ONrCharacter_FightMode(ioCharacter);
						animation = ONrCharacter_DoAnimation(ioCharacter, active_character, ONcAnimPriority_Appropriate, ONcAnimType_Ninja_Invisible);
						if (animation != NULL) {
							superninja_state->invis_started = UUcTrue;
#if AI_DEBUG_SUPERNINJA
							COrConsole_Printf("superninja: started invisibility animation");
#endif
						}
					} else if (ONrCharacter_IsFallen(ioCharacter) || ONrAnimType_IsVictimType(active_character->curAnimType)) {
						// abort our invisibility move
						superninja_state->disable_invis_timer = AI2cSuperNinja_UnsuccessfulSpecialMoveTimer;
#if AI_DEBUG_SUPERNINJA
						COrConsole_Printf("superninja: aborting invis, somehow seem to be fallen or victim");
#endif
					}
				}
				break;

			case AI2cSuperNinja_TeleportFlee:
			case AI2cSuperNinja_TeleportAdvance:
				// stand still
				ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Hold] = 1.0f;

				if (!superninja_state->teleport_started) {
					if (ONrCharacter_IsStandingStill(ioCharacter)) {
#if AI_DEBUG_SUPERNINJA
						COrConsole_Printf("superninja: animtype %s curstate %s fromstate %s [queued %s] -> inactive and upright, do special move",
											ONrAnimTypeToString(active_character->curAnimType), ONrAnimStateToString(active_character->curFromState),
											ONrAnimStateToString(active_character->nextAnimState), ONrAnimTypeToString(active_character->nextAnimType));
#endif						
						// perform our teleport-in move
						ONrCharacter_FightMode(ioCharacter);
						animation = ONrCharacter_DoAnimation(ioCharacter, active_character, ONcAnimPriority_Appropriate, ONcAnimType_Teleport_In);
						if (animation != NULL) {
							superninja_state->teleport_started = UUcTrue;
#if AI_DEBUG_SUPERNINJA
							COrConsole_Printf("superninja: started teleport-in animation");
#endif
						}
					} else if (ONrCharacter_IsFallen(ioCharacter) || ONrAnimType_IsVictimType(active_character->curAnimType)) {
						// abort our teleport move
						superninja_state->disable_teleport_timer = AI2cSuperNinja_UnsuccessfulSpecialMoveTimer;
						superninja_state->superninja_mode = AI2cSuperNinja_Melee;
						superninja_state->prev_superninja_mode = AI2cSuperNinja_None;
						ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Melee] = 1.2f;
#if AI_DEBUG_SUPERNINJA
						COrConsole_Printf("superninja: aborting teleport, somehow seem to be fallen or victim");
#endif
					}
				}
				break;

			case AI2cSuperNinja_Advance:
				ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Advance] = 1.0f;
				break;

			case AI2cSuperNinja_Melee:
				// we are close to the enemy, reset our medium-range random behavior
				superninja_state->set_mediumrange_behavior = UUcFalse;
				ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Melee] = 1.0f;
				break;
		}

		return 0;

	case AI2cCombatMessage_SecondaryMovement:
		ioCombatState->dodge_enable = UUcTrue;
		return 0;

	case AI2cCombatMessage_Targeting:
		ioCombatState->desire_env_los = UUcTrue;
		ioCombatState->desire_friendly_los = UUcTrue;
		ioCombatState->run_targeting = UUcTrue;
		ioCombatState->aim_weapon = UUcFalse;
		return 0;

	case AI2cCombatMessage_LookAtTarget:
		switch(superninja_state->superninja_mode) {
			case AI2cSuperNinja_Fireball:
				// always look at the target
				return UUcTrue;
		}

		// use default behavior
		*inHandled = UUcFalse;
		return 0;

	case AI2cCombatMessage_Hurt:
		// record the damage taken
		superninja_state->flee_damage_taken += inParam1;
		superninja_state->flee_damagestay_timer = AI2cSuperNinja_DamageStayFrames;
		return 0;

	case AI2cCombatMessage_NewAnimation:
		// handle super-ninja being knocked out of his various behaviors
		if (superninja_state->fireball_started) {
			if (inParam1 != ONcAnimType_Ninja_Fireball) {
				superninja_state->disable_fireball_timer = AI2cSuperNinja_UnsuccessfulSpecialMoveTimer;
				superninja_state->fireball_started = UUcFalse;
#if AI_DEBUG_SUPERNINJA
				COrConsole_Printf("superninja: knocked out of starting fireball by animtype %s", ONrAnimTypeToString((UUtUns16) inParam1));
#endif
			}
		} else if (superninja_state->invis_started) {
			if (inParam1 != ONcAnimType_Ninja_Invisible) {
				superninja_state->disable_invis_timer = AI2cSuperNinja_UnsuccessfulSpecialMoveTimer;
				superninja_state->invis_started = UUcFalse;
#if AI_DEBUG_SUPERNINJA
				COrConsole_Printf("superninja: knocked out of starting invisibility by animtype %s", ONrAnimTypeToString((UUtUns16) inParam1));
#endif
			}
		} else if ((superninja_state->teleport_started) || (inParam1 == ONcAnimType_Teleport_Out)) {
			// our teleport is finished, successfully or unsuccessfully
			superninja_state->superninja_mode = AI2cSuperNinja_None;
			superninja_state->prev_superninja_mode = AI2cSuperNinja_None;
			superninja_state->teleport_started = UUcFalse;
			if (inParam1 == ONcAnimType_Teleport_Out) {
				// we successfully teleported
				superninja_state->flee_tick_counter = 0;
				superninja_state->flee_damage_taken = 0;
				superninja_state->set_mediumrange_behavior = UUcFalse;
			} else {
				// we did not successfully teleport
				superninja_state->disable_teleport_timer = AI2cSuperNinja_UnsuccessfulSpecialMoveTimer;
			}
#if AI_DEBUG_SUPERNINJA
			COrConsole_Printf("superninja: teleport %s (animtype %s)", (inParam1 == ONcAnimType_Teleport_Out) ? "succeeded" : "aborted",
								ONrAnimTypeToString((UUtUns16) inParam1));
#endif
		}
		return 0;

	case AI2cCombatMessage_ActionFrame:
		if (inParam1 == ONcAnimType_Ninja_Fireball) {
			// we just launched a fireball, finish this mode
			superninja_state->disable_fireball_timer = AI2cSuperNinja_FireballTimer;
			superninja_state->fireball_started = UUcFalse;
#if AI_DEBUG_SUPERNINJA
			COrConsole_Printf("superninja: launched fireball");
#endif

		} else if (inParam1 == ONcAnimType_Ninja_Invisible) {
			// we just turned invisible, finish this mode
			WPrPowerup_Give(ioCharacter, WPcPowerup_Invisibility, AI2cSuperNinja_InvisibilityAmount, UUcTrue, UUcFalse);

			superninja_state->disable_invis_timer = AI2cSuperNinja_InvisibleDisableTimer;
			superninja_state->invis_started = UUcFalse;
#if AI_DEBUG_SUPERNINJA
			COrConsole_Printf("superninja: turned invisible");
#endif
		}
		return 0;

	case AI2cCombatMessage_FallDetected:
		superninja_state->fall_timer++;
		if (superninja_state->fall_timer < AI2cSuperNinja_FallTeleportFrames) {
			// wait a number of frames, suspended in midair, until we can teleport back up
			ioCharacter->location.y = AI2cSuperNinja_FallTeleportHeight;
			ioCharacter->prev_location.y = AI2cSuperNinja_FallTeleportHeight;
			ioCharacter->actual_position.y = AI2cSuperNinja_FallTeleportHeight;
			active_character->physics->position.y = AI2cSuperNinja_FallTeleportHeight;
			return UUcTrue;
		}

		// find a teleport destination
		found_point = AI2iBehavior_FindEdgeFlag(&ioCharacter->location, AI2cSuperNinja_EdgeFlagBase, &desired_destination, &desired_facing);
		if (!found_point) {
#if AI_DEBUG_SUPERNINJA
			COrConsole_Printf("superninja: unable to build edge point successfully, cannot teleport from fall");
#endif
			*inHandled = UUcFalse;
			return 0;
		}

		// try to find a location near this which is unobstructed
		theta = desired_facing + UUmRandomRangeFloat(-AI2cSuperNinja_TeleportEdgeRandomAngle, AI2cSuperNinja_TeleportEdgeRandomAngle);
		UUmTrig_Clip(theta);
		error = AI2rFindNearbyPoint(ioCharacter, PHcPathMode_CasualMovement, &desired_destination,
									AI2cSuperNinja_TeleportEdgeMoveDistance, 0.0f, theta, M3cQuarterPi / 4, 4,
									PHcSemiPassable, 3, &localpath_success, &localpath_endpoint, &localpath_weight, &localpath_escape);
		if (error == UUcError_None) {
			active_character->teleportDestination = localpath_endpoint;
		} else {
#if AI_DEBUG_SUPERNINJA
			COrConsole_Printf("superninja: unable to find safe point near edge, teleporting further in as last-ditch effort");
#endif

			active_character->teleportDestination = desired_destination;
			active_character->teleportDestination.x += 2 * AI2cSuperNinja_TeleportEdgeMoveDistance * MUrSin(desired_facing);
			active_character->teleportDestination.z += 2 * AI2cSuperNinja_TeleportEdgeMoveDistance * MUrCos(desired_facing);
		}
		active_character->teleportDestFacing = desired_facing;
		active_character->teleportEnable = UUcTrue;

		// make super-ninja exit from this teleport immediately - no time for us to run the teleport-in animation
		animation = TRrCollection_Lookup(ioCharacter->characterClass->animations, ONcAnimType_Teleport_Out, ONcAnimState_Standing,
											active_character->animVarient | ONcAnimVarientMask_Fight);
		if (animation == NULL) {
#if AI_DEBUG_SUPERNINJA
			COrConsole_Printf("superninja: unable to teleport from fall, no teleport-out animation");
#endif
			*inHandled = UUcFalse;
			return 0;
		}

		// force super-ninja to begin his teleport-out animation immediately
		ONrCharacter_SetAnimation_External(ioCharacter, ONcAnimState_Standing, animation, 0);

		// super-ninja can no longer be invisible, we need to see him teleport back up
		ioCharacter->inventory.invisibilityRemaining = 0;

		// kill all velocity and zero fall variables
		active_character->inAirControl.numFramesInAir = 0;
		MUmVector_Set(active_character->inAirControl.velocity, 0, 0, 0);

		// zero the fall-timer for next time
		superninja_state->fall_timer = 0;

		return UUcTrue;

	case AI2cCombatMessage_EndBehavior:
		// nothing to deallocate in our internal state
		return 0;

	default:
		*inHandled = UUcFalse;
		return 0;
	}
}

static UUtUns32 AI2iBehavior_SuperNinjaFireball(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3)
{
	*inHandled = UUcFalse;

	switch(inMsg) {
	case AI2cCombatMessage_BeginBehavior:
		ioCombatState->behavior_state.superNinja.superninja_mode = AI2cSuperNinja_Fireball;
		break;
	
	case AI2cCombatMessage_PrimaryMovement:
		if ((ioCombatState->behavior_state.superNinja.disable_fireball_timer == 0) && (ONrCharacter_IsInactiveUpright(ioCharacter))) {
			ioCombatState->behavior_state.superNinja.superninja_mode = AI2cSuperNinja_Fireball;
		} else {
			ioCombatState->behavior_state.superNinja.superninja_mode = AI2cSuperNinja_Advance;
		}
		break;
	}

	if (!*inHandled) {
		return AI2iBehavior_SuperNinja(ioCharacter, ioCombatState, inMsg, inHandled, inParam1, inParam2, inParam3);
	} else {
		return 0;
	}
}

static UUtUns32 AI2iBehavior_SuperNinjaAdvance(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3)
{
	*inHandled = UUcFalse;

	switch(inMsg) {
	case AI2cCombatMessage_BeginBehavior:
		ioCombatState->behavior_state.superNinja.superninja_mode = AI2cSuperNinja_Advance;
		break;
	
	case AI2cCombatMessage_PrimaryMovement:
		ioCombatState->behavior_state.superNinja.superninja_mode = AI2cSuperNinja_Advance;
		break;
	}

	if (!*inHandled) {
		return AI2iBehavior_SuperNinja(ioCharacter, ioCombatState, inMsg, inHandled, inParam1, inParam2, inParam3);
	} else {
		return 0;
	}
}

static UUtUns32 AI2iBehavior_SuperNinjaMelee(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3)
{
	*inHandled = UUcFalse;

	switch(inMsg) {
	case AI2cCombatMessage_BeginBehavior:
		ioCombatState->behavior_state.superNinja.superninja_mode = AI2cSuperNinja_Melee;
		break;
	
	case AI2cCombatMessage_PrimaryMovement:
		// FIXME: trigger off super ninja's damage taken field to see if we want to escape
		ioCombatState->behavior_state.superNinja.superninja_mode = AI2cSuperNinja_Melee;
		break;
	}

	if (!*inHandled) {
		return AI2iBehavior_SuperNinja(ioCharacter, ioCombatState, inMsg, inHandled, inParam1, inParam2, inParam3);
	} else {
		return 0;
	}
}

// ------------------------------------------------------------------------------------
// -- mutant muro's special-case behavior functions

static void AI2iMutantMuro_DisableShield(AI2tBehaviorState_MutantMuro *ioMuroState, 
										 UUtUns32 inDecayTime, UUtUns32 inDisableTime)
{
	if ((ioMuroState->shield_disable_timer > 0) || (ioMuroState->shield_decay_timer > 0)) {
		// muro's shield is already disabled
		ioMuroState->shield_disable_timer = UUmMax(ioMuroState->shield_disable_timer, inDisableTime);
		if (ioMuroState->shield_decay_timer > 0) {
			// set the decay timer so that we decay from the point at which we currently are
			ioMuroState->shield_decay_timer = (ioMuroState->shield_decay_timer * inDecayTime) / ioMuroState->shield_decay_max;
			ioMuroState->shield_decay_max = inDecayTime;
		}
	} else if (ioMuroState->shield_regen_timer > 0) {
		// muro's shield is regenerating... set the decay timer so that we decay from the point at which
		// we currently are
		ioMuroState->shield_decay_timer = (ioMuroState->shield_regen_timer * inDecayTime) / ioMuroState->shield_regen_max;
		ioMuroState->shield_decay_max = inDecayTime;
		ioMuroState->shield_disable_timer = inDisableTime;
	} else {
		// muro's shield is active
		ioMuroState->shield_decay_timer = inDecayTime;
		ioMuroState->shield_decay_max = inDecayTime;
		ioMuroState->shield_disable_timer = inDisableTime;
	}
	ioMuroState->shield_active_frames = 0;
}

static UUtUns32 AI2iBehavior_MutantMuro(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3)
{
	ONtActiveCharacter *active_character = ONrForceActiveCharacter(ioCharacter);
	AI2tBehaviorState_MutantMuro *muro_state;
	TRtAnimation *animation;

	muro_state = &ioCombatState->behavior_state.mutantMuro;

	switch(inMsg) {
	case AI2cCombatMessage_BeginBehavior:
		switch((AI2tBehaviorType) inParam1) {
			case AI2cBehavior_MutantMuroZeus:
			case AI2cBehavior_MutantMuroMelee:
				// we were already in a mutant muro combat behavior, don't reset this stuff
				break;

			default:
				// initialise mutant muro's combat behavior
				UUrMemory_Clear(muro_state, sizeof(*muro_state));
				break;
		}
		return 0;
	
	case AI2cCombatMessage_PrimaryMovement:
		// track muro's shield

		if (muro_state->total_damage > 0) {
			if (muro_state->total_damage_decay_timer > AI2cCombat_UpdateFrames) {
				muro_state->total_damage_decay_timer -= AI2cCombat_UpdateFrames;
			} else {
				muro_state->total_damage_decay_timer = 0;
				muro_state->total_damage = 0;
			}
		}

		if (muro_state->shield_decay_timer > 0) {
			// shield is still decaying
			ioCharacter->super_amount = ((float) muro_state->shield_decay_timer) / muro_state->shield_decay_max;

			if (muro_state->shield_decay_timer > AI2cCombat_UpdateFrames) {
				muro_state->shield_decay_timer -= AI2cCombat_UpdateFrames;
			} else {
				muro_state->shield_decay_timer = 0;
			}
			muro_state->shield_active_frames = 0;

		} else if (muro_state->shield_disable_timer > 0) {
			// shield is disabled
			ioCharacter->super_amount = 0.0f;

			if (muro_state->shield_disable_timer > AI2cCombat_UpdateFrames) {
				muro_state->shield_disable_timer -= AI2cCombat_UpdateFrames;
			} else {
				muro_state->shield_disable_timer = 0;
				muro_state->shield_regen_timer = AI2cMutantMuro_ShieldRegenTimer;
				muro_state->shield_regen_max = muro_state->shield_regen_timer;
			}
			muro_state->shield_active_frames = 0;

		} else if (muro_state->shield_regen_timer > 0) {
			// shield is regenerating
			ioCharacter->super_amount = 1.0f - ((float) muro_state->shield_regen_timer) / muro_state->shield_regen_max;

			if (muro_state->shield_regen_timer > AI2cCombat_UpdateFrames) {
				muro_state->shield_regen_timer -= AI2cCombat_UpdateFrames;
			} else {
				muro_state->shield_regen_timer = 0;
			}
			muro_state->shield_active_frames = 0;

		} else {
			// shield is at full strength
			ioCharacter->super_amount = 1.0f;
			muro_state->shield_active_frames += AI2cCombat_UpdateFrames;
		}

		if (muro_state->beam_maximum_timer > AI2cCombat_UpdateFrames) {
			muro_state->beam_maximum_timer -= AI2cCombat_UpdateFrames;
		} else {
			muro_state->beam_maximum_timer = 0;
		}
		if (muro_state->beam_continue_timer > AI2cCombat_UpdateFrames) {
			muro_state->beam_continue_timer -= AI2cCombat_UpdateFrames;
		} else {
			muro_state->beam_continue_timer = 0;
		}
		if (muro_state->beam_disable_timer > AI2cCombat_UpdateFrames) {
			muro_state->beam_disable_timer -= AI2cCombat_UpdateFrames;
		} else {
			muro_state->beam_disable_timer = 0;
		}
#if AI_DEBUG_MUTANTMURO
		COrConsole_Printf("murostate: super %f active %d | decay %d/%d | disable %d | regen %d/%d",
							ioCharacter->super_amount, muro_state->shield_active_frames, muro_state->shield_decay_timer, muro_state->shield_decay_max,
							muro_state->shield_disable_timer, muro_state->shield_regen_timer, muro_state->shield_regen_max);
#endif
		return 0;

	case AI2cCombatMessage_ShieldCharging:
		// we cannot throw super moves until our shield has been active for some time
		*inHandled = UUcTrue;
		if (muro_state->shield_active_frames < AI2cMutantMuro_ShieldRequiredFrames) {
			return (UUtUns32) UUcTrue;
		} else {
			return (UUtUns32) UUcFalse;
		}

	case AI2cCombatMessage_Hurt:
		muro_state->total_damage += inParam1;
		muro_state->total_damage_decay_timer = AI2cMutantMuro_TurtleForgetTimer;
		return 0;

	case AI2cCombatMessage_SecondaryMovement:
		ioCombatState->dodge_enable = UUcTrue;
		return 0;

	case AI2cCombatMessage_NewAnimation:
		animation = (TRtAnimation *) inParam3;
		UUmAssertReadPtr(animation, sizeof(TRtAnimation));

		if (TRrAnimation_TestFlag(animation, ONcAnimFlag_DisableShield)) {
			// disable our invulnerability
			AI2iMutantMuro_DisableShield(muro_state, AI2cMutantMuro_ShieldAttackDecay, AI2cMutantMuro_ShieldAttackDisable);
		}
		return 0;

	default:
		return 0;
	}
}

static UUtUns32 AI2iBehavior_MutantMuroZeus(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3)
{
	UUtUns32 returnval;
	AI2tBehaviorState_MutantMuro *muro_state = &ioCombatState->behavior_state.mutantMuro;
	ONtActiveCharacter *active_character = ONrForceActiveCharacter(ioCharacter);
	const TRtAnimation *animation;
	UUtBool lightning_enable, lightning_disable;

	// call our generic behaviors first
	*inHandled = UUcFalse;
	returnval = AI2iBehavior_MutantMuro(ioCharacter, ioCombatState, inMsg, inHandled, inParam1, inParam2, inParam3);
	if (*inHandled) {
		// our generic behavior has handled this message
		return returnval;
	}

	// handle the combat message ourselves
	*inHandled = UUcTrue;

	switch(inMsg) {
		// these are handled by our generic behavior
		case AI2cCombatMessage_BeginBehavior:
		case AI2cCombatMessage_EndBehavior:
			break;

		case AI2cCombatMessage_PrimaryMovement:
			if (ONrCharacter_IsFallen(ioCharacter)) {
				ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_GetUp] = 1.0f;
				break;
			}
			
			lightning_enable = (ioCombatState->distance_to_target > AI2cMutantMuro_ZeusTargetDistance);
			lightning_enable = lightning_enable || (ioCombatState->have_los);

			lightning_disable = (ioCombatState->distance_to_target < AI2cMutantMuro_ZeusDisableDistance);
			if (muro_state->total_damage > AI2cMutantMuro_DamageThresholdStopZeus) {
				lightning_disable = UUcTrue;
#if AI_DEBUG_MUTANTMURO
				COrConsole_Printf("mutantmuro: stop lightning, damage %d > %d", muro_state->total_damage, AI2cMutantMuro_DamageThresholdStopZeus);
#endif
			}

			if (lightning_disable) {
				// we want to stop throwing lightning at our target
				muro_state->beam_disable_timer = AI2cMutantMuro_ZeusDisableTimer;

			} else if (lightning_enable) {
				// we want to throw lightning at our target
				muro_state->beam_continue_timer = AI2cMutantMuro_ZeusContinueTimer;

			} else {
				// just stay as we are, let our timers run down
			}

			if ((muro_state->beam_maximum_timer == 0) || (muro_state->beam_continue_timer == 0) ||
				(muro_state->beam_disable_timer > 0)) {

				if ((muro_state->beam_maximum_timer == 0) &&
					(muro_state->beam_continue_timer > 0) &&
					(muro_state->beam_disable_timer == 0)) {
					// we were knocked out of the lightning state by exceeding our maximum timer,
					// don't immediately start it again
					muro_state->beam_disable_timer = AI2cMutantMuro_ZeusDisableTimer;
				}

				// we can't throw lightning at our target, advance
				muro_state->beam_maximum_timer = AI2cMutantMuro_ZeusMaximumTimer;
				muro_state->beam_continue_timer = 0;

				// we are advancing into melee, reset our damage threshold
				muro_state->total_damage = 0;
				muro_state->total_damage_decay_timer = 0;

				ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Advance] = 1.0f;
				break;
			}

			// stand still
			ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Hold] = 1.0f;

			if (active_character->nextAnimType == ONcAnimType_Muro_Thunderbolt) {
				// we are already flinging lightning, this is fine
#if AI_DEBUG_MUTANTMURO
				COrConsole_Printf("mutantmuro: flinging lightning");
#endif

			} else if (!ONrCharacter_IsStandingStill(ioCharacter) && (active_character->curAnimType != ONcAnimType_Muro_Thunderbolt)) {
				// stand still and wait for an opportunity to fling the thunderbolt
#if AI_DEBUG_MUTANTMURO
				COrConsole_Printf("mutantmuro: standing still and waiting to fling thunderbolt");
#endif

			} else {
				// perform our thunderbolt attack
#if AI_DEBUG_MUTANTMURO
				COrConsole_Printf("mutantmuro: animtype %s curstate %s fromstate %s [queued %s] -> inactive and upright, do special move",
									ONrAnimTypeToString(active_character->curAnimType), ONrAnimStateToString(active_character->curFromState),
									ONrAnimStateToString(active_character->nextAnimState), ONrAnimTypeToString(active_character->nextAnimType));
#endif						
				ONrCharacter_FightMode(ioCharacter);
				animation = ONrCharacter_DoAnimation(ioCharacter, active_character, ONcAnimPriority_Appropriate, ONcAnimType_Muro_Thunderbolt);
				if (animation == NULL) {
					// the animation lookup failed for an unknown reason
					muro_state->beam_disable_timer = AI2cMutantMuro_ZeusDisableTimer;
#if AI_DEBUG_MUTANTMURO
					COrConsole_Printf("mutantmuro: thunderbolt animation lookup failed");
#endif
				} else {
#if AI_DEBUG_MUTANTMURO
					COrConsole_Printf("mutantmuro: started thunderbolt animation");
#endif
					AI2iMutantMuro_DisableShield(muro_state, AI2cMutantMuro_ShieldZeusDecay, AI2cMutantMuro_ShieldZeusDisable);
				}
			}
			break;

		case AI2cCombatMessage_Targeting:
			ioCombatState->desire_env_los = UUcTrue;
			ioCombatState->desire_friendly_los = UUcTrue;
			ioCombatState->run_targeting = UUcTrue;
			ioCombatState->aim_weapon = UUcFalse;
			break;

		case AI2cCombatMessage_SwitchBehavior:
			if ((muro_state->beam_continue_timer > 0) && (muro_state->beam_disable_timer == 0)) {
				// we can't change behaviors yet
#if AI_DEBUG_MUTANTMURO
				COrConsole_Printf("mutantmuro: can't switch behaviors, firing thunderbolt");
#endif
				return UUcTrue;
			} else {
				// we can switch behaviors
#if AI_DEBUG_MUTANTMURO
				COrConsole_Printf("mutantmuro: okay to switch behaviors");
#endif
				return UUcFalse;
			}
			break;

		case AI2cCombatMessage_LookAtTarget:
			return UUcTrue;

		default:
			*inHandled = UUcFalse;
			break;
	}

	return 0;
}

static UUtUns32 AI2iBehavior_MutantMuroMelee(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3)
{
	UUtUns32 returnval;
	AI2tBehaviorState_MutantMuro *muro_state = &ioCombatState->behavior_state.mutantMuro;
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
	const TRtAnimation *animation;

	// call our generic behaviors first
	*inHandled = UUcFalse;
	returnval = AI2iBehavior_MutantMuro(ioCharacter, ioCombatState, inMsg, inHandled, inParam1, inParam2, inParam3);
	if (*inHandled) {
		// our generic behavior has handled this message
		return returnval;
	}

	// handle the combat message ourselves
	*inHandled = UUcTrue;

	switch(inMsg) {
		// these are handled by our generic behavior
		case AI2cCombatMessage_BeginBehavior:
		case AI2cCombatMessage_EndBehavior:
			break;

		case AI2cCombatMessage_PrimaryMovement:
			muro_state->beam_maximum_timer = AI2cMutantMuro_ZeusMaximumTimer;
			muro_state->beam_continue_timer = 0;

			if (ioCharacter->super_amount > 0.5f) {
				// we are shielded, reset our damage timer and try to melee
				muro_state->total_damage = 0;
				muro_state->total_damage_decay_timer = 0;
			} else if ((muro_state->total_damage > AI2cMutantMuro_DamageThresholdTurtle) &&
				(ioCombatState->distance_to_target < AI2cMutantMuro_TurtleTargetDistance) &&
				(active_character != NULL)) {
				// we have taken enough damage to want to turtle, and the target is close to us
#if AI_DEBUG_MUTANTMURO
				COrConsole_Printf("mutantmuro: dmg %d > %d, dist %f < %f, we want to turtle",
								muro_state->total_damage, AI2cMutantMuro_DamageThresholdTurtle,
								ioCombatState->distance_to_target, AI2cMutantMuro_TurtleTargetDistance);
#endif
				if (ONrCharacter_IsFallen(ioCharacter)) {
					// get up so that we can turtle
					ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_GetUp] = 1.0f;

				} else {
					// try to turtle
					ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Hold] = 1.0f;

					if (!ONrCharacter_IsStandingStill(ioCharacter)) {
						// stand still and wait for an opportunity to turtle
#if AI_DEBUG_MUTANTMURO
						COrConsole_Printf("mutantmuro: standing still and waiting to turtle");
#endif

					} else {
						// perform our turtle maneuver
#if AI_DEBUG_MUTANTMURO
						COrConsole_Printf("mutantmuro: animtype %s curstate %s fromstate %s [queued %s] -> inactive and upright, do special move",
											ONrAnimTypeToString(active_character->curAnimType), ONrAnimStateToString(active_character->curFromState),
											ONrAnimStateToString(active_character->nextAnimState), ONrAnimTypeToString(active_character->nextAnimType));
#endif						

						// kill stun while trying to turtle 
						active_character->hitStun = 0;
						active_character->blockStun = 0;
						active_character->staggerStun = 0;
						active_character->softPauseFrames = 0;
						active_character->hardPauseFrames = 0;

						ONrCharacter_FightMode(ioCharacter);
						animation = ONrCharacter_DoAnimation(ioCharacter, active_character, ONcAnimPriority_Appropriate, ONcAnimType_Crouch_Back);
						if (animation == NULL) {
							// the animation lookup failed for an unknown reason
#if AI_DEBUG_MUTANTMURO
							COrConsole_Printf("mutantmuro: turtle animation lookup failed");
#endif
						} else {
#if AI_DEBUG_MUTANTMURO
							COrConsole_Printf("mutantmuro: started turtle animation");
#endif
						}

						// reset our damage threshold so we don't still want to do this
						muro_state->total_damage = 0;
						muro_state->total_damage_decay_timer = 0;
					}
				}

				break;
			}

			// handle fallthrough with the melee system
			ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Melee] = 1.0f;
			break;

		case AI2cCombatMessage_Targeting:
			// don't aim or shoot at the target, but look at them if they're in view
			ioCombatState->desire_env_los = UUcFalse;
			ioCombatState->desire_friendly_los = UUcFalse;
			ioCombatState->run_targeting = UUcTrue;
			ioCombatState->aim_weapon = UUcFalse;
			ioCombatState->shoot_weapon = UUcFalse;
			break;

		case AI2cCombatMessage_LookAtTarget:
			return UUcTrue;

		default:
			*inHandled = UUcFalse;
			break;
	}

	return 0;
}

// ------------------------------------------------------------------------------------
// -- default behavior

// handle any messages that weren't covered by our current behavior function
UUtUns32 AI2rBehavior_Default(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
								   AI2tCombatMessage inMsg, UUtBool *inHandled,
								   UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
	AI2tKnowledgeEntry *entry;
	AI2tReportFunction report_function;
	char reportbuf[256];
	float too_close, look_distance;
	UUtBool engage_in_melee;

	*inHandled = UUcTrue;

	switch(inMsg) {
	case AI2cCombatMessage_BeginBehavior:
	case AI2cCombatMessage_EndBehavior:
		// every behavior must handle these messages
		AI2_ERROR(AI2cBug, AI2cSubsystem_Combat, AI2cError_Combat_UnhandledMessage, ioCharacter,
				  inMsg, inParam1, inParam2, inParam3);
		*inHandled = UUcFalse;
		return 0;

	case AI2cCombatMessage_Report:
		if ((ioCombatState->current_behavior.type >= 0) && (ioCombatState->current_behavior.type < AI2cBehavior_Max)) {
			sprintf(reportbuf, "%s: nothing to report", AI2cBehaviorName[ioCombatState->current_behavior.type]);
		} else {
			strcpy(reportbuf, "<error>: nothing to report");
		}
		report_function = (AI2tReportFunction) inParam1;
		report_function(reportbuf);
		return 0;

	case AI2cCombatMessage_PrimaryMovement:
		// no movement; will stand still.
		return 0;

	case AI2cCombatMessage_SecondaryMovement:
		ioCombatState->dodge_enable = UUcTrue;
		return 0;

	case AI2cCombatMessage_Targeting:
		// look at the target, but don't perform LOS calculations
		ioCombatState->run_targeting = UUcTrue;
		ioCombatState->desire_env_los = UUcFalse;
		ioCombatState->desire_friendly_los = UUcFalse;
		ioCombatState->aim_weapon = UUcFalse;
		return 0;

	case AI2cCombatMessage_Fallen:
		if (ioCombatState->currently_dazed) {
			if (ioCharacter->ai2State.daze_timer > 0) {
				// we are dazed. don't aim and return true to stop movement code.
				ioCombatState->run_targeting = UUcFalse;
				ioCombatState->desire_env_los = UUcFalse;
				ioCombatState->desire_friendly_los = UUcFalse;
				ioCombatState->aim_weapon = UUcFalse;

				return (UUtUns32) UUcTrue;
			}

			// we can now get up
			ioCombatState->currently_dazed = UUcFalse;
			return (UUtUns32) UUcFalse;

		} else if (ioCharacter->ai2State.already_done_daze) {
			// get up! just get up forwards. this code only executes if we are a purely gunplay character, as if
			// we are in melee, whenever no_longer_dazed is set the melee code determines a maneuver to stand up with.
			// note that setting the primary movement weight here is ineffective as the weights will be cleared
			// after this time. instead the weights are set inside the main combat update loop.
			ioCombatState->maneuver.getup_direction = AI2cMovementDirection_Forward;

			// we can now move normally
			return (UUtUns32) UUcFalse;

		} else {
			// we have just been knocked down and haven't set up our daze yet
			ioCombatState->currently_dazed = UUcTrue;
			ioCharacter->ai2State.already_done_daze = UUcTrue;
			AI2rStartDazeTimer(ioCharacter);

			// we are dazed. don't aim and return true to stop movement code.
			return (UUtUns32) UUcTrue;
		}

	case AI2cCombatMessage_TooClose:
		too_close = *((float *) inParam1);

		// we ramp up the weights from 0 to 1 as we head towards 2/3 of the weapon's
		// minimum safe distance.
		too_close = UUmMin(too_close * 1.5f, 1.0f);

		// determine whether we want to fight the target instead of backing away
		engage_in_melee = AI2iCheckFightBack(ioCharacter, ioCombatState, ioCharacter->ai2State.combatSettings.melee_when);

		UUmAssertReadPtr(ioCombatState->targeting.weapon_parameters, sizeof(AI2tWeaponParameters));
		ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Advance] *= 1.0f - too_close;
		ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Hold] *= 1.0f - too_close;

		if (engage_in_melee) {
			// charge into melee
			ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Melee] = too_close;
			ioCombatState->maneuver.advance_maxdist = ioCombatState->targeting.weapon_parameters->min_safe_distance;
			ioCombatState->maneuver.advance_mindist = ioCombatState->maneuver.advance_maxdist * 0.75f;

			ioCombatState->weaponcaused_melee_timer = UUmMax(ioCombatState->weaponcaused_melee_timer, AI2cCombat_TooClose_MeleeTimer);
			ioCombatState->retreatfailed_melee_timer = 0;
			ioCombatState->retreat_frames = 0;
		} else {
			UUtBool temporary_melee = UUcFalse;

			if (too_close > 0.5f) {
				if (ioCombatState->retreat_frames >= AI2cCombat_TooClose_MaxRetreatTimer) {
					temporary_melee = UUcTrue;
				} else if ((ioCombatState->retreatfailed_melee_timer > 0) &&
							(ioCombatState->retreatfailed_melee_timer < AI2cCombat_TooClose_RetreatFailedMeleeTimer)) {
					temporary_melee = UUcTrue;
				}
			}

			if (temporary_melee) {
				// we must now go into melee
				ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Melee] = too_close;
				ioCombatState->maneuver.advance_maxdist = ioCombatState->targeting.weapon_parameters->min_safe_distance;
				ioCombatState->maneuver.advance_mindist = ioCombatState->maneuver.advance_maxdist * 0.75f;

				ioCombatState->retreatfailed_melee_timer += AI2cCombat_UpdateFrames;
				ioCombatState->retreat_frames = 0;
			} else {
				// keep retreating
				ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Retreat] = too_close;
				ioCombatState->maneuver.retreat_mindist = ioCombatState->targeting.weapon_parameters->min_safe_distance;
				ioCombatState->maneuver.retreat_maxdist = ioCombatState->maneuver.retreat_mindist * 1.5f;

				ioCombatState->retreatfailed_melee_timer = 0;
				if (too_close > 0.9f) {
					ioCombatState->retreat_frames += AI2cCombat_UpdateFrames;
				}
			}
		}
		return 0;

	case AI2cCombatMessage_LookAtTarget:
		if ((ioCombatState->trigger_pressed) || (ioCombatState->alternate_trigger_pressed) || (ioCombatState->shoot_weapon) ||
			((ioCharacter->inventory.weapons[0] != NULL) && (WPrIsFiring(ioCharacter->inventory.weapons[0])))) {
			// always look at target while shooting
			return UUcTrue;
		}

		// how close do we have to be in order to look at target?
		if ((ioCombatState->maneuver.primary_movement < 0) || (ioCombatState->maneuver.primary_movement > AI2cPrimaryMovement_Max)) {
			look_distance = 500.0f;
		} else {
			look_distance = AI2cCombatLookRange[ioCombatState->maneuver.primary_movement];
		}

		// look at them if we're within the threshold
		return (ioCombatState->distance_to_target <= look_distance);

	case AI2cCombatMessage_Hurt:
		entry = (AI2tKnowledgeEntry *) inParam2;
		if (entry->enemy == ioCombatState->target) {
			ioCombatState->last_hit_by_target = entry->last_time;
			if (entry->last_type == AI2cContactType_Hit_Melee) {
				ioCombatState->last_melee_hit_by_target = entry->last_time;
			}
		}
		return 0;

	default:
		return 0;
	}
}

// ------------------------------------------------------------------------------------
// -- debugging functions

// report in
void AI2rCombat_Report(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
					   UUtBool inVerbose, AI2tReportFunction inFunction)
{
	char reportbuf[256];
	UUtBool handled;

	if ((ioCombatState->current_range >= 0) && (ioCombatState->current_range < AI2cCombatRange_Max)) {
		sprintf(reportbuf, "  range %s", AI2cCombatRangeName[ioCombatState->current_range]);
	} else {
		sprintf(reportbuf, "  <unknown range %d>", ioCombatState->current_range);
	}
	inFunction(reportbuf);

	if ((ioCombatState->current_behavior.type >= 0) && (ioCombatState->current_behavior.type < AI2cBehavior_Max)) {
		sprintf(reportbuf, "  behavior %s", AI2cBehaviorName[ioCombatState->current_behavior.type]);
	} else {
		sprintf(reportbuf, "  <unknown behavior %d>", ioCombatState->current_behavior.type);
	}
	inFunction(reportbuf);

	if (inVerbose) {
		AI2rCombat_Behavior(ioCharacter, ioCombatState, AI2cCombatMessage_Report, &handled, 0, 0, 0);
	}
}	
