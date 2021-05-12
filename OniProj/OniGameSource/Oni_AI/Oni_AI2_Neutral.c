/*
	FILE:	Oni_AI2_Neutral.c
	
	AUTHOR:	Chris Butcher
	
	CREATED: July 13, 2000
	
	PURPOSE: Neutral AI for Oni
	
	Copyright (c) Bungie Software, 2000

*/

#define AI_VERBOSE_NEUTRAL		0

#include "bfw_math.h"

#include "Oni_AI2.h"
#include "Oni_Character.h"

// ------------------------------------------------------------------------------------
// -- constants

#define AI2cNeutral_CheckFrames				30
#define AI2cNeutral_RunToFrames				20
#define AI2cNeutral_GiveItemDist			20.0f
#define AI2cNeutral_ReceiveDelay			15
#define AI2cNeutral_EnemyScareFrames		300
#define AI2cNeutral_EnemyCheckFrames		10

#define AI2cNeutral_ClosestRange			25.0f
#define AI2cNeutral_ManualTriggerRange		35.0f
#define AI2cNeutral_ManualTriggerAngle		(60.0f * M3cDegToRad)

#define AI2cNeutral_DefaultEnemyRange		100.0f
#define AI2cNeutral_DisableTimer			180
#define AI2cNeutral_GiveUpTimer				600
#define AI2cNeutral_HailInitialTimer		300
#define AI2cNeutral_HailRepeatTimer			360
#define AI2cNeutral_HailRandomTimer			90

// ------------------------------------------------------------------------------------
// -- internal function prototypes

static void AI2iNeutral_ResetCharacterState(ONtCharacter *ioCharacter, UUtBool inAbort, UUtBool inHostileAbort, UUtBool inResetGoalState);
static UUtBool AI2iNeutral_ConversationAnimation(ONtCharacter *ioCharacter, UUtUns16 inAnimType, UUtBool inAnimateOnce,
												UUtBool inAnimateRepeat, UUtBool *ioAnimStarted);
static UUtUns16 AI2iNeutral_GiveOrDropItem(ONtCharacter *ioCharacter, ONtCharacter *ioRecipient, WPtPowerupType inType, UUtUns16 inAmount);
static UUtBool AI2iNeutral_CheckStraightLinePath(ONtCharacter *ioCharacter, ONtCharacter *inTarget);
static void AI2iNeutral_PlayTriggerSound(ONtCharacter *ioCharacter, AI2tNeutralState *ioNeutralState);

// ------------------------------------------------------------------------------------
// -- high-level setup

// switch to a new kind of neutral behavior
UUtBool AI2rNeutral_ChangeBehavior(ONtCharacter *ioCharacter, UUtUns16 inNeutralID, UUtBool inInitialize)
{
	AI2tNeutralBehavior behavior;
	UUtBool found_behavior = UUcTrue;

	if (ioCharacter->charType != ONcChar_AI2) {
		return UUcFalse;
	}
	
	if (!inInitialize && (ioCharacter->ai2State.neutralBehaviorID == inNeutralID)) {
		// we already have this neutral behavior
		return (ioCharacter->ai2State.neutralBehaviorID != (UUtUns16) -1);
	}

	if (inNeutralID != (UUtUns16) -1) {
		// try to find the neutral behavior
		found_behavior = ONrLevel_Neutral_ID_To_Neutral(inNeutralID, &behavior);
		if (found_behavior) {
			ioCharacter->ai2State.neutralBehaviorID = inNeutralID;
			ioCharacter->ai2State.neutralResumeAtLine = 0;
			ioCharacter->ai2State.neutralFlags = 0;
			ioCharacter->ai2State.neutralLastCheck = 0;
			ioCharacter->ai2State.neutralTriggerRange = behavior.trigger_range;
			ioCharacter->ai2State.neutralEnemyRange = behavior.abort_enemy_range;
			return UUcTrue;
		}
	}

	// no neutral behavior specified or ID was not found.. clear neutral behavior settings
	ioCharacter->ai2State.neutralBehaviorID = (UUtUns16) -1;
	ioCharacter->ai2State.neutralResumeAtLine = 0;
	ioCharacter->ai2State.neutralFlags = 0;
	ioCharacter->ai2State.neutralLastCheck = 0;
	ioCharacter->ai2State.neutralTriggerRange = 0;
	ioCharacter->ai2State.neutralEnemyRange = AI2cNeutral_DefaultEnemyRange;
	return UUcFalse;
}

static UUtBool AI2iNeutral_CheckStraightLinePath(ONtCharacter *ioCharacter, ONtCharacter *inTarget)
{
	UUtError error;
	M3tPoint3D stop_point, our_pt, player_pt;
	UUtUns8 stop_weight;
	UUtBool found_path, localmove_escape;
	ONtCharacter *prev_ignorechar;
	
	ONrCharacter_GetPathfindingLocation(ioCharacter, &our_pt);
	ONrCharacter_GetPathfindingLocation(inTarget, &player_pt);

	// set up pathfindIgnoreChar so that we don't view the target as an obstruction
	// NB: don't trash the previous value of pathfindIgnoreChar!
	prev_ignorechar = ioCharacter->movementOrders.curPathIgnoreChar;
	ioCharacter->movementOrders.curPathIgnoreChar = inTarget;

	// check to see if we have a straight-line path to the target of our neutral interaction
	error = AI2rTryLocalMovement(ioCharacter, PHcPathMode_CasualMovement, &our_pt, &player_pt,
								3, &found_path, &stop_point, &stop_weight, &localmove_escape);

	// restore pathfindIgnoreChar before we're done
	ioCharacter->movementOrders.curPathIgnoreChar = prev_ignorechar;

	return ((error == UUcError_None) && (found_path));
}

static UUtBool AI2iNeutral_CheckEnemies(ONtCharacter *ioCharacter, ONtCharacter *inTarget)
{
	ONtCharacter **char_list, *enemy, *enemy_target;
	UUtUns32 num_chars, itr;
	float dist_sq;

	// we are currently playing the neutral interaction, look for enemies with hostile intent nearby
	num_chars = ONrGameState_LivingCharacterList_Count();
	char_list = ONrGameState_LivingCharacterList_Get();

	for (itr = 0; itr < num_chars; itr++) {
		enemy = char_list[itr];

		// only trigger off AIs in combat mode
		if ((enemy->charType != ONcChar_AI2) || (enemy->ai2State.currentGoal != AI2cGoal_Combat))
			continue;

		enemy_target = enemy->ai2State.currentState->state.combat.target;
		if ((enemy_target != ioCharacter) && (enemy_target != inTarget))
			continue;

		// only trigger when hostiles get within a certain range of us or the target
		dist_sq = MUrPoint_Distance_Squared(&ioCharacter->actual_position, &enemy->actual_position);
		if (dist_sq > UUmSQR(ioCharacter->ai2State.neutralEnemyRange)) {
			dist_sq = MUrPoint_Distance_Squared(&inTarget->actual_position, &enemy->actual_position);
			if (dist_sq > UUmSQR(ioCharacter->ai2State.neutralEnemyRange)) {
				continue;
			}
		}

		return UUcTrue;
	}

	return UUcFalse;
}

// check to see if we want to run our neutral behavior
UUtBool AI2rNeutral_CheckTrigger(ONtCharacter *ioCharacter, ONtCharacter *inTarget, UUtBool inAutomaticCheck)
{
	M3tPoint3D target_pt, our_pt, delta_pos;
	AI2tKnowledgeEntry *entry;
	float angle;
	UUtUns32 current_time;
	UUtBool enemies_nearby;

	if ((ioCharacter->charType != ONcChar_AI2) || (ioCharacter->ai2State.neutralBehaviorID == (UUtUns16) -1)) {
		return UUcFalse;
	}

	if ((ioCharacter->neutral_interaction_char != NULL) || (ioCharacter->ai2State.currentGoal == AI2cGoal_Panic)) {
		// don't trigger if we are already running a neutral character interaction, or if we're panicking
		return UUcFalse;
	}

	if (ioCharacter->ai2State.neutralFlags & AI2cNeutralFlag_DisableResume) {
		// we cannot run this NCI again
		return UUcFalse;
	}

	if (inAutomaticCheck && (ioCharacter->ai2State.neutralDisableTimer > 0)) {
		// don't trigger automatically if we've been disabled
		return UUcFalse;
	}

	// check distance bound, first
	ONrCharacter_GetPathfindingLocation(ioCharacter, &our_pt);
	ONrCharacter_GetPathfindingLocation(inTarget, &target_pt);
	MUmVector_Subtract(delta_pos, our_pt, target_pt);

	if (inAutomaticCheck) {
		// we trigger if we're anywhere near the target
		if (MUmVector_GetLengthSquared(delta_pos) > UUmSQR(ioCharacter->ai2State.neutralTriggerRange)) {
			return UUcFalse;
		}
	} else {
		// we have to be much closer to be manually triggered
		if (MUmVector_GetLengthSquared(delta_pos) > UUmSQR(AI2cNeutral_ManualTriggerRange)) {
			return UUcFalse;
		}

		// check that we're in the right direction relative to the target
		angle = MUrATan2(delta_pos.x, delta_pos.z);
		UUmTrig_ClipLow(angle);
		angle -= ONrCharacter_GetCosmeticFacing(inTarget);
		UUmTrig_ClipAbsPi(angle);

		if (fabs(angle) > AI2cNeutral_ManualTriggerAngle) {
			return UUcFalse;
		}
	}

	// hostile characters can't play neutral behavior
	if (AI2rCharacter_PotentiallyHostile(ioCharacter, inTarget, UUcTrue)) {
		return UUcFalse;
	}

	if (inAutomaticCheck) {
		// the rest of the check gets expensive... check only every so often
		current_time = ONrGameState_GetGameTime();
		if (current_time < ioCharacter->ai2State.neutralLastCheck + AI2cNeutral_CheckFrames) {
			return UUcFalse;
		} else {
			ioCharacter->ai2State.neutralLastCheck = current_time;
		}

		// check that we can see player
		for (entry = ioCharacter->ai2State.knowledgeState.contacts; entry != NULL; entry = entry->nextptr) {
			if ((entry->enemy == inTarget) && (entry->strength == AI2cContactStrength_Definite)) {
				break;
			}
		}
		if (entry == NULL) {
			return UUcFalse;
		}

		// check that there are no enemies to disturb us
		if (ioCharacter->ai2State.neutralEnemyRange > 0) {
			enemies_nearby = AI2rCharacter_InDanger(ioCharacter, inTarget, UUcFalse, NULL);
			enemies_nearby = enemies_nearby || AI2iNeutral_CheckEnemies(ioCharacter, inTarget);

			if (enemies_nearby) {
				// don't run now, or for a number of frames in the future
				ioCharacter->ai2State.neutralLastCheck = current_time + AI2cNeutral_EnemyScareFrames;
				return UUcFalse;
			}
		}		
	}

#if 0
	{
		ONtCharacter *prev_ignorechar;
		float path_distance;
		UUtError error;

		// set up pathfindIgnoreChar so that we don't view the player as an obstruction
		// NB: don't trash the previous value of pathfindIgnoreChar!
		prev_ignorechar = ioCharacter->movementOrders.curPathIgnoreChar;
		ioCharacter->movementOrders.curPathIgnoreChar = inTarget;

		if (!AI2iNeutral_CheckStraightLinePath(ioCharacter, inTarget)) {
			// check to see if we can reach player within distance by going through multiple BNVs
			path_distance = ioCharacter->ai2State.neutralTriggerRange;

			// set up pathfindIgnoreChar so that we don't view the player as an obstruction
			// NB: don't trash the previous value of pathfindIgnoreChar!
			prev_ignorechar = ioCharacter->movementOrders.curPathIgnoreChar;
			ioCharacter->movementOrders.curPathIgnoreChar = inTarget;

			error = PHrPathDistance(ioCharacter, (UUtUns32) -1, &our_pt, &target_pt, ioCharacter->currentPathnode, inTarget->currentPathnode, &path_distance);

			// restore pathfindIgnoreChar before we're done
			ioCharacter->movementOrders.curPathIgnoreChar = prev_ignorechar;

			if (error != UUcError_None) {
				// no path is available
				return UUcFalse;
			}
		}
	}
#else
	ONrCharacter_GetPelvisPosition(inTarget, &target_pt);
	if (AI2rManeuver_CheckBlockage(ioCharacter, &target_pt)) {
		return UUcFalse;
	}
#endif

	return UUcTrue;
}

// check to see if we want to accept an action event from a character
UUtBool AI2rNeutral_CheckActionAcceptance(ONtCharacter *ioCharacter, ONtCharacter *inTarget)
{
	M3tPoint3D our_pt, target_pt;
	M3tVector3D delta_pos;
	float angle;

	if ((ioCharacter->charType != ONcChar_AI2) || (ioCharacter->ai2State.currentGoal != AI2cGoal_Neutral)) {
		return UUcFalse;
	}

	if (ioCharacter->ai2State.currentState->state.neutral.target_character != inTarget) {
		return UUcFalse;
	}

	// check distance bound, first
	ONrCharacter_GetPathfindingLocation(ioCharacter, &our_pt);
	ONrCharacter_GetPathfindingLocation(inTarget, &target_pt);
	MUmVector_Subtract(delta_pos, our_pt, target_pt);

	// we have to be much closer to be manually triggered
	if (MUmVector_GetLengthSquared(delta_pos) > UUmSQR(AI2cNeutral_ManualTriggerRange)) {
		return UUcFalse;
	}

	// check that we're in the right direction relative to the target
	angle = MUrATan2(delta_pos.x, delta_pos.z);
	UUmTrig_ClipLow(angle);
	angle -= ONrCharacter_GetCosmeticFacing(inTarget);
	UUmTrig_ClipAbsPi(angle);

	if (fabs(angle) > AI2cNeutral_ManualTriggerAngle) {
		return UUcFalse;
	}

	// we are in the right point to be triggered
	return UUcTrue;
}

// ------------------------------------------------------------------------------------
// -- high-level neutral interaction functions

// enter our neutral behavior
void AI2rNeutral_Enter(ONtCharacter *ioCharacter)
{
	UUtBool found_behavior;
	AI2tNeutralState *neutral_state;

	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Neutral);
	neutral_state = &ioCharacter->ai2State.currentState->state.neutral;

#if AI_VERBOSE_NEUTRAL
	COrConsole_Printf("%s: entering neutral behavior", ioCharacter->player_name);
#endif

	// we must be active before entering our neutral behavior
	ONrCharacter_MakeActive(ioCharacter, UUcTrue);

	// we must only ever enter the neutral behavior state if we actually have
	// a neutral behavior to run!
	UUmAssert(ioCharacter->ai2State.neutralBehaviorID != (UUtUns16) -1);
	found_behavior = ONrLevel_Neutral_ID_To_Neutral(ioCharacter->ai2State.neutralBehaviorID, &neutral_state->behavior);
	if (!found_behavior) {
		AI2_ERROR(AI2cBug, AI2cSubsystem_HighLevel, AI2cError_HighLevel_NoNeutralBehavior, ioCharacter,
					ioCharacter->ai2State.neutralBehaviorID, 0, 0, 0);
		ioCharacter->ai2State.neutralBehaviorID = (UUtUns16) -1;
		return;
	}

	// change our knowledge notification function
	if (ioCharacter->ai2State.flags & AI2cFlag_NonCombatant) {
		ioCharacter->ai2State.knowledgeState.callback = &AI2rNonCombatant_NotifyKnowledge;
	} else {
		ioCharacter->ai2State.knowledgeState.callback = &AI2rAlert_NotifyKnowledge;
	}

	// begin our neutral behavior
	ONrCharacter_GetPathfindingLocation(ioCharacter, &neutral_state->triggered_point);
	neutral_state->current_line = (UUtUns32) -1;
	ioCharacter->ai2State.neutralFlags |= AI2cNeutralFlag_Triggered;

	// set up our movement state to run to the player character
	neutral_state->last_runto_update = 0;
	neutral_state->last_enemy_check = 0;
	neutral_state->initiated_give = UUcFalse;
	neutral_state->completed_give = UUcFalse;
	neutral_state->target_accepted = UUcFalse;
	neutral_state->target_character = ONrGameState_GetPlayerCharacter();
	neutral_state->giveup_timer = AI2cNeutral_GiveUpTimer;
	neutral_state->hail_timer = AI2cNeutral_HailInitialTimer + UUmRandomRange(0, AI2cNeutral_HailRandomTimer);
	neutral_state->played_abort = UUcFalse;

	AI2iNeutral_PlayTriggerSound(ioCharacter, neutral_state);

	AI2rMovement_ChangeMovementMode(ioCharacter, AI2cMovementMode_NoAim_Run);
	AI2rMovement_FreeFacing(ioCharacter);
	AI2rMovement_StopAiming(ioCharacter);
	AI2rMovement_StopGlancing(ioCharacter);
}

void AI2rNeutral_Exit(ONtCharacter *ioCharacter)
{
	AI2tNeutralState *neutral_state;

	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Neutral);
	neutral_state = &ioCharacter->ai2State.currentState->state.neutral;

#if AI_VERBOSE_NEUTRAL
	COrConsole_Printf("%s: exiting neutral behavior", ioCharacter->player_name);
#endif

	// reset our movement state
	AI2rMovement_ChangeMovementMode(ioCharacter, AI2cMovementMode_Default);
	AI2rMovement_StopAiming(ioCharacter);

	{
		UUtBool we_are_aborting;
		
		we_are_aborting = ((neutral_state->current_line == -1) || (neutral_state->current_line < neutral_state->behavior.num_lines));

		AI2rNeutral_Stop(ioCharacter, UUcTrue, we_are_aborting, UUcFalse, UUcFalse);
	}
}

static void AI2iNeutral_ResetCharacterState(ONtCharacter *ioCharacter, UUtBool inAbort, UUtBool inHostileAbort, UUtBool inResetGoalState)
{
	ONrCharacter_DisableFightVarient(ioCharacter, UUcFalse);
	ONrCharacter_DisableWeaponVarient(ioCharacter, UUcFalse);

	if ((ioCharacter->charType == ONcChar_AI2) && (inAbort) && (ioCharacter->ai2State.currentGoal == AI2cGoal_Neutral)) {
		AI2tNeutralState *neutral_state = &ioCharacter->ai2State.currentState->state.neutral;
		char *stop_sound;
		UUtUns32 stop_type;

		if (inHostileAbort) {
			stop_sound = neutral_state->behavior.enemy_sound;
			stop_type = ONcSpeechType_Yell;
		} else if (ioCharacter->ai2State.neutralFlags & AI2cNeutralFlag_DisableResume) {
			// the interaction is being aborted, but we don't want to say "come back here" because
			// we wouldn't restart it anyway
			stop_sound = "";
			stop_type = ONcSpeechType_None;
		} else {
			stop_sound = neutral_state->behavior.abort_sound;
			stop_type = ONcSpeechType_Call;
		}

		if (!neutral_state->played_abort) {
			if (!UUmString_IsEqual(stop_sound, "")) {
				ONtSpeech speech;

				speech.played = UUcFalse;
				speech.post_pause = 0;
				speech.pre_pause = 0;
				speech.notify_string_inuse = NULL;
				speech.sound_name = stop_sound;
				speech.speech_type = stop_type;

				ONrSpeech_Say(ioCharacter, &speech, UUcFalse);
			}
			neutral_state->played_abort = UUcTrue;
		}

		// snap out of the interaction
		AI2rMovement_StopAiming(ioCharacter);
		AI2rMovement_StopGlancing(ioCharacter);
		AI2rPath_Halt(ioCharacter);
		if (inResetGoalState) {
			AI2rReturnToJob(ioCharacter, UUcTrue, UUcTrue);
		}
	}
}

// abort a neutral interaction. note: may be called with either one of the characters involved
UUtBool AI2rNeutral_Stop(ONtCharacter *ioCharacter, UUtBool inForce, UUtBool inAbort, UUtBool inHostileAbort, UUtBool inResetState)
{
	ONtCharacter *neutral_target = ioCharacter->neutral_interaction_char;
	UUtBool dead = UUcFalse;
	ONtCharacter *neutral_ai = NULL;
	AI2tNeutralState *neutral_state = NULL;

	if (neutral_target == NULL) {
		// the neutral character interaction never really started anyway, nothing to stop
		return UUcTrue;
	}

	dead = ((ioCharacter->flags & ONcCharacterFlag_Dead) || (neutral_target->flags & ONcCharacterFlag_Dead));
	if (!dead) {
		if ((ioCharacter->charType == ONcChar_AI2) && (ioCharacter->ai2State.currentGoal == AI2cGoal_Neutral)) {
			neutral_ai = ioCharacter;
			neutral_state = &ioCharacter->ai2State.currentState->state.neutral;

		} else if ((neutral_target->charType == ONcChar_AI2) && (neutral_target->ai2State.currentGoal == AI2cGoal_Neutral)) {
			neutral_ai = neutral_target;
			neutral_state = &neutral_target->ai2State.currentState->state.neutral;
		}
	}

	if ((neutral_state != NULL) && (neutral_state->initiated_give) && (!neutral_state->completed_give) && (!inForce)) {
		// we are in the middle of a give animation, we can't stop
		return UUcFalse;
	}

	if (inAbort && (neutral_ai != NULL)) {
		// disable our neutral behavior from starting again within a short space of time
		neutral_ai->ai2State.neutralDisableTimer = AI2cNeutral_DisableTimer;
	}

	if ((!inAbort) && (neutral_state != NULL) && (neutral_state->behavior.end_script[0] != '\0')) {
		// set up the neutral end-script to run next tick
		ioCharacter->ai2State.flags |= AI2cFlag_RunNeutralScript;
		ioCharacter->ai2State.neutralPendingScript = neutral_state->behavior.end_script;
	}

	if ((ioCharacter->flags & ONcCharacterFlag_Dead) == 0) {
		AI2iNeutral_ResetCharacterState(ioCharacter, inAbort, inHostileAbort, inResetState);
	}
	if ((neutral_target->flags & ONcCharacterFlag_Dead) == 0) {
		AI2iNeutral_ResetCharacterState(neutral_target, inAbort, inHostileAbort, inResetState);
	}

	// remove the neutral-interaction links
	neutral_target->neutral_interaction_char = NULL;
	ioCharacter->neutral_interaction_char = NULL;
	neutral_target->flags	&= ~ONcCharacterFlag_NeutralUninterruptable;
	ioCharacter->flags		&= ~ONcCharacterFlag_NeutralUninterruptable;

	return UUcTrue;
}

UUtBool AI2rNeutral_PathfindingHandler(ONtCharacter *inCharacter, UUtUns32 inErrorID,
										UUtUns32 inParam1, UUtUns32 inParam2,
										UUtUns32 inParam3, UUtUns32 inParam4)
{
	// silently stop neutral interaction
	AI2rNeutral_Stop(inCharacter, UUcTrue, UUcFalse, UUcFalse, UUcTrue);
	AI2rReturnToJob(inCharacter, UUcTrue, UUcTrue);

	// error is NOT handled - so it still gets reported
	return UUcFalse;
}

// ------------------------------------------------------------------------------------
// -- neutral conversation

static UUtUns16 AI2iNeutral_GiveOrDropItem(ONtCharacter *ioCharacter, ONtCharacter *ioRecipient,
									 WPtPowerupType inType, UUtUns16 inAmount)
{
	UUtUns16 amount_given, amount_notgiven;

	if (inAmount == 0) {
		return 0;
	}

	amount_given = WPrPowerup_Give(ioRecipient, inType, inAmount, UUcFalse, UUcTrue);

	// drop any overflow powerups on the floor
	amount_notgiven = inAmount - amount_given;
	if (amount_notgiven > 0) {
		WPrPowerup_Drop(inType, amount_notgiven, ioCharacter, UUcTrue);
	}

	return amount_given;
}

static void AI2iNeutral_GiveItems(ONtCharacter *ioCharacter, AI2tNeutralState *ioNeutralState)
{
	ONtCharacter *recipient = ioNeutralState->target_character;
	UUtError error;
	WPtWeapon *weapon;
	WPtWeaponClass *weapon_class;

	AI2iNeutral_GiveOrDropItem(ioCharacter, recipient, WPcPowerup_AmmoBallistic,	ioNeutralState->behavior.give_ballistic_ammo);
	AI2iNeutral_GiveOrDropItem(ioCharacter, recipient, WPcPowerup_AmmoEnergy,		ioNeutralState->behavior.give_energy_ammo);
	AI2iNeutral_GiveOrDropItem(ioCharacter, recipient, WPcPowerup_Hypo,				ioNeutralState->behavior.give_hypos);

	if (ioNeutralState->behavior.give_flags & AI2cNeutralGive_Shield) {
		WPrPowerup_Give(recipient, WPcPowerup_ShieldBelt, WPrPowerup_DefaultAmount(WPcPowerup_ShieldBelt), UUcFalse, UUcTrue);
	}

	if (ioNeutralState->behavior.give_flags & AI2cNeutralGive_Invisibility) {
		WPrPowerup_Give(recipient, WPcPowerup_Invisibility, WPrPowerup_DefaultAmount(WPcPowerup_Invisibility), UUcFalse, UUcTrue);
	}

	if (ioNeutralState->behavior.give_flags & AI2cNeutralGive_LSI) {
		WPrPowerup_Give(recipient, WPcPowerup_LSI, WPrPowerup_DefaultAmount(AI2cNeutralGive_LSI), UUcFalse, UUcTrue);
		ioCharacter->inventory.has_lsi = UUcFalse;
	}

	error = TMrInstance_GetDataPtr(WPcTemplate_WeaponClass, ioNeutralState->behavior.give_weaponclass, &weapon_class);
	if ((error == UUcError_None) && (weapon_class != NULL)) {
		weapon = WPrNew(weapon_class, WPcNormalWeapon);
		if (weapon != NULL) {
			if (ONrCharacter_IsCaryingWeapon(recipient)) {
				// drop weapon to ground
				WPrMagicDrop(ioCharacter, weapon, UUcFalse, UUcTrue);
			} else {
				// place weapon in target's hand
				ONrCharacter_UseWeapon(recipient, weapon);
			}
		}
	}
}

static void AI2iNeutral_PlayTriggerSound(ONtCharacter *ioCharacter, AI2tNeutralState *ioNeutralState)
{
	if (ioNeutralState->behavior.trigger_sound[0] != '\0') {
		ONtSpeech speech;

		speech.played = UUcFalse;
		speech.post_pause = 0;
		speech.pre_pause = 0;
		speech.sound_name = ioNeutralState->behavior.trigger_sound;
		speech.notify_string_inuse = NULL;
		speech.speech_type = ONcSpeechType_Call;

		ONrSpeech_Say(ioCharacter, &speech, UUcFalse);
	}
}

void AI2rNeutral_Update(ONtCharacter *ioCharacter)
{
	M3tPoint3D our_point, target_point;
	UUtUns32 current_time;
	AI2tNeutralState *neutral_state;
	AI2tNeutralLine *line;
	float distance_sq, facing;
	ONtCharacter *line_character, *other_character;
	UUtBool block, close_enough, aborted;
	ONtActiveCharacter *target_active, *active_character = ONrGetActiveCharacter(ioCharacter);

	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Neutral);
	neutral_state = &ioCharacter->ai2State.currentState->state.neutral;	
	UUmAssertReadPtr(active_character, sizeof(ONtActiveCharacter));

	if ((active_character == NULL) ||
		(neutral_state->target_character->flags & ONcCharacterFlag_Dead) ||
		((neutral_state->target_character->flags & ONcCharacterFlag_InUse) == 0) ||
		(ioCharacter->ai2State.neutralBehaviorID == (UUtUns16) -1)) {
		// we can't continue the neutral interaction
		AI2rPath_Halt(ioCharacter);
		AI2rReturnToJob(ioCharacter, UUcTrue, UUcTrue);
		return;
	}

	target_active = ONrForceActiveCharacter(neutral_state->target_character);
	UUmAssertReadPtr(target_active, sizeof(ONtActiveCharacter));
	if (target_active == NULL) {
		// we can't continue the neutral interaction
		AI2rPath_Halt(ioCharacter);
		AI2rReturnToJob(ioCharacter, UUcTrue, UUcTrue);
		return;
	}

	current_time = ONrGameState_GetGameTime();
	if (neutral_state->initiated_give && !neutral_state->completed_give) {
		// we are busy giving an item, ignore enemies

	} else if (current_time > neutral_state->last_enemy_check + AI2cNeutral_EnemyCheckFrames) {
		if (AI2iNeutral_CheckEnemies(ioCharacter, neutral_state->target_character)) {
			// enemies are nearby!
			aborted = AI2rNeutral_Stop(ioCharacter, UUcFalse, UUcTrue, UUcTrue, UUcTrue);
			if (aborted) {
				return;
			}
		}
		neutral_state->last_enemy_check = current_time;
	}

	if (neutral_state->current_line == (UUtUns32) -1) {
		// we have not yet entered the neutral interaction
		ONrCharacter_GetPathfindingLocation(ioCharacter, &our_point);
		ONrCharacter_GetPelvisPosition(neutral_state->target_character, &target_point);

		distance_sq = MUmVector_GetDistanceSquared(our_point, target_point);
		close_enough = (distance_sq < UUmSQR(neutral_state->behavior.conversation_range));
		close_enough = close_enough && (!AI2rManeuver_CheckBlockage(ioCharacter, &target_point));

		if (close_enough && neutral_state->target_accepted) {
			// enter the neutral interaction

			// turn towards target and halt
			facing = MUrATan2(target_point.x - our_point.x, target_point.z - our_point.z);
			UUmTrig_ClipLow(facing);
			AI2rPath_GoToPoint(ioCharacter, &our_point, 0, UUcTrue, facing, NULL);
			AI2rMovement_LookAtCharacter(ioCharacter, neutral_state->target_character);

			if (neutral_state->target_character->neutral_interaction_char != NULL) {
				// wait for the previous neutral interaction to finish
				return;
			}

			if (TRrAnimation_GetTo(active_character->animation) != ONcAnimState_Standing) {
				// wait for us to reach a standing state before we start the conversation
				return;
			}

#if AI_VERBOSE_NEUTRAL
			COrConsole_Printf("%s: close enough (range %f), entering conversation", ioCharacter->player_name,
								neutral_state->behavior.conversation_range);
#endif

			ioCharacter->ai2State.neutralFlags |= AI2cNeutralFlag_ConversationBegun;
			if (neutral_state->behavior.flags & AI2cNeutralBehaviorFlag_NoResume) {
				ioCharacter->ai2State.neutralFlags |= AI2cNeutralFlag_DisableResume;
			}
			
			// make note of the neutral interaction
			ioCharacter->neutral_interaction_char = neutral_state->target_character;
			neutral_state->target_character->neutral_interaction_char = ioCharacter;
			if (neutral_state->behavior.flags & AI2cNeutralBehaviorFlag_Uninterruptable) {
				ioCharacter->flags						|= ONcCharacterFlag_NeutralUninterruptable;
				neutral_state->target_character->flags	|= ONcCharacterFlag_NeutralUninterruptable;
			}

			// turn the target towards us
			facing += M3cPi;
			UUmTrig_ClipHigh(facing);
			ONrCharacter_SetDesiredFacing(neutral_state->target_character, facing);

			// force them to turn towards us instead of looking
			if (ONrCharacter_GetDesiredFacingOffset(neutral_state->target_character) > 0) {
				target_active->pleaseTurnRight = UUcTrue;
			} else {
				target_active->pleaseTurnLeft = UUcTrue;
			}

			// set up partial input blocking
			neutral_state->target_character->neutral_keyabort_frames = 0;

			// change variants appropriately
			ONrCharacter_DisableFightVarient(ioCharacter, UUcTrue);
			ONrCharacter_DisableWeaponVarient(ioCharacter, UUcTrue);
			ONrCharacter_DisableFightVarient(neutral_state->target_character, UUcTrue);
			ONrCharacter_DisableWeaponVarient(neutral_state->target_character, UUcTrue);

			neutral_state->played_line = UUcFalse;
			neutral_state->current_line = ioCharacter->ai2State.neutralResumeAtLine;
		} else {
			// don't follow too far away from our original point
			if (MUmVector_GetDistanceSquared(our_point, neutral_state->triggered_point) > UUmSQR(neutral_state->behavior.follow_range)) {
	#if AI_VERBOSE_NEUTRAL
				COrConsole_Printf("%s: too far from original point (%f), no longer following", ioCharacter->player_name,
									neutral_state->behavior.follow_range);
	#endif
				ioCharacter->ai2State.neutralDisableTimer = AI2cNeutral_DisableTimer;
				AI2rPath_GoToPoint(ioCharacter, &neutral_state->triggered_point, 0, UUcTrue, AI2cAngle_None, NULL);
				AI2rReturnToJob(ioCharacter, UUcTrue, UUcTrue);
				return;
			}

			if (close_enough) {
				AI2rMovement_LookAtCharacter(ioCharacter, neutral_state->target_character);
				AI2rPath_Halt(ioCharacter);

				if (neutral_state->giveup_timer > 0) {
					neutral_state->giveup_timer--;
				} else {
					// it has been too long, we aren't interested in this any more
					ioCharacter->ai2State.neutralDisableTimer = AI2cNeutral_DisableTimer;
				}

				if (neutral_state->hail_timer > 0) {
					neutral_state->hail_timer--;
				} else {
					// reset our timer, and hail the target
					neutral_state->hail_timer = AI2cNeutral_HailRepeatTimer + UUmRandomRange(0, AI2cNeutral_HailRandomTimer);
					
					AI2iNeutral_PlayTriggerSound(ioCharacter, neutral_state);

					if (active_character != NULL) {
						ONrCharacter_DoAnimation(ioCharacter, active_character, ONcAnimPriority_Appropriate, ONcAnimType_Hail);
					}
				}

			} else if (distance_sq > UUmSQR(1.5f * neutral_state->behavior.trigger_range)) {
				// we are too far away from the target to be interested in following them
#if AI_VERBOSE_NEUTRAL
				COrConsole_Printf("%s: range > 1.5 * trigger (%f), no longer following", ioCharacter->player_name,
									1.5f * neutral_state->behavior.trigger_range);
#endif
				AI2rReturnToJob(ioCharacter, UUcTrue, UUcTrue);
				return;

			} else {
				if (current_time > neutral_state->last_runto_update + AI2cNeutral_RunToFrames) {
					// pathfind to target character
#if AI_VERBOSE_NEUTRAL
					COrConsole_Printf("%s: repathing to target %s", ioCharacter->player_name,
										neutral_state->target_character->player_name);
#endif

					AI2rPath_GoToPoint(ioCharacter, &target_point, AI2cNeutral_ClosestRange,
										UUcTrue, AI2cAngle_None, neutral_state->target_character);
					neutral_state->last_runto_update = current_time;
				}
			}

			// we can't enter the neutral interaction yet
			return;
		}
	}

	if (neutral_state->current_line >= neutral_state->behavior.num_lines) {
		// our interaction is finished.
		ioCharacter->ai2State.neutralFlags |= (AI2cNeutralFlag_Completed | AI2cNeutralFlag_DisableResume);
		AI2rReturnToJob(ioCharacter, UUcTrue, UUcTrue);
		return;
	}

	UUmAssert((neutral_state->current_line >= 0) && (neutral_state->current_line < neutral_state->behavior.num_lines));
	line = &neutral_state->behavior.lines[neutral_state->current_line];

	if (line->flags & AI2cNeutralLineFlag_PlayerLine) {
		// player says this line
		line_character = neutral_state->target_character;
		other_character = ioCharacter;
	} else {
		// we say this line
		line_character = ioCharacter;
		other_character = neutral_state->target_character;
	}

	if (!neutral_state->played_line) {
#if AI_VERBOSE_NEUTRAL
		COrConsole_Printf("%s: saying neutral line %d", ioCharacter->player_name, neutral_state->current_line);
#endif

		if (line->sound[0] != '\0') {
			ONtSpeech speech;

			speech.played = UUcFalse;
			speech.post_pause = 0;
			speech.pre_pause = 0;
			speech.notify_string_inuse = NULL;
			speech.sound_name = line->sound;
			speech.speech_type = ONcSpeechType_Say;

			ONrSpeech_Say(line_character, &speech, UUcFalse);
		}

		if (line->flags & AI2cNeutralLineFlag_GiveItem) {
#if AI_VERBOSE_NEUTRAL
			COrConsole_Printf("%s: walking up to target %s", ioCharacter->player_name,
								neutral_state->target_character->player_name);
#endif
			// walk towards player
			AI2rMovement_ChangeMovementMode(ioCharacter, AI2cMovementMode_NoAim_Walk);
			AI2rPath_GoToPoint(ioCharacter, &neutral_state->target_character->actual_position,
								AI2cNeutral_GiveItemDist, UUcTrue, AI2cAngle_None, neutral_state->target_character);
		} else {
			neutral_state->linechar_animstarted = UUcFalse;
			neutral_state->otherchar_animstarted = UUcFalse;
		}

		neutral_state->played_line = UUcTrue;
	}

	block = UUcFalse;

	if (line->flags & AI2cNeutralLineFlag_GiveItem) {
		if (!neutral_state->initiated_give) {
			// walk towards player
			ONrCharacter_GetPathfindingLocation(ioCharacter, &our_point);
			ONrCharacter_GetPathfindingLocation(neutral_state->target_character, &target_point);
			if (MUmVector_GetDistanceSquared(our_point, target_point) < UUmSQR(AI2cNeutral_GiveItemDist)) {
				// start the give animations
				neutral_state->initiated_give = UUcTrue;
				neutral_state->giveanim1_started = UUcFalse;
				neutral_state->giveanim2_started = UUcFalse;
				neutral_state->receive_delay = AI2cNeutral_ReceiveDelay;
			} else {
				// block until we get close enough to player
				block = UUcTrue;
			}
		}

		if (neutral_state->initiated_give) {
			// block until the give animations are done
			block |= AI2iNeutral_ConversationAnimation(ioCharacter, ONcAnimType_Act_Give, UUcTrue,
														!neutral_state->completed_give, &neutral_state->giveanim1_started);
			if (neutral_state->giveanim1_started) {
				if (neutral_state->receive_delay > 0) {
					neutral_state->receive_delay--;
				}

				if (neutral_state->receive_delay > 0) {
					// wait a few frames before starting receive animation
					block = UUcTrue;
				} else {
					block |= AI2iNeutral_ConversationAnimation(neutral_state->target_character, ONcAnimType_Act_Give,
																UUcTrue, !neutral_state->completed_give, &neutral_state->giveanim2_started);

					if ((!neutral_state->completed_give) && (active_character->curAnimType == ONcAnimType_Act_Give) &&
						(active_character->animFrame >= TRrAnimation_GetActionFrame(active_character->animation))) {
						// give the goods!
						ioCharacter->ai2State.neutralFlags |= AI2cNeutralFlag_ItemsGiven;
						if (neutral_state->behavior.flags & AI2cNeutralBehaviorFlag_NoResumeAfterItems) {
							ioCharacter->ai2State.neutralFlags |= AI2cNeutralFlag_DisableResume;
						}

						AI2iNeutral_GiveItems(ioCharacter, neutral_state);

						// this line is effectively over, don't bother resuming from here next time
						ioCharacter->ai2State.neutralResumeAtLine = (UUtUns16) (neutral_state->current_line + 1);
						neutral_state->initiated_give = UUcFalse;
						neutral_state->completed_give = UUcTrue;
					}
				}
			}
		}
	} else {
		// play animations if appropriate
		if (line->anim_type != ONcAnimType_None) {
			block |= AI2iNeutral_ConversationAnimation(line_character, line->anim_type, ((line->flags & AI2cNeutralLineFlag_AnimOnce) > 0),
														UUcFalse, &neutral_state->linechar_animstarted);
		}

		if (line->other_anim_type != ONcAnimType_None) {
			block |= AI2iNeutral_ConversationAnimation(other_character, line->other_anim_type, ((line->flags & AI2cNeutralLineFlag_OtherAnimOnce) > 0),
														UUcFalse, &neutral_state->otherchar_animstarted);
		}
	}

	if (line->sound[0] != '\0') {
		// block until our current line is finished
		block |= ONrSpeech_Saying(line_character);
	}

	if (!block) {
		// we have finished our line, go to the next
		neutral_state->current_line++;
		neutral_state->played_line = UUcFalse;
		ioCharacter->ai2State.neutralResumeAtLine = (UUtUns16) neutral_state->current_line;
	}
}

static UUtBool AI2iNeutral_ConversationAnimation(ONtCharacter *ioCharacter, UUtUns16 inAnimType, 
												UUtBool inAnimateOnce, UUtBool inRepeatAnimation, UUtBool *ioAnimStarted)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
	const TRtAnimation *result_anim;

	if (active_character == NULL) {
		return UUcFalse;
	}

	if (inAnimateOnce) {
		if (active_character->curAnimType == inAnimType) {
			// we're playing the animation
			*ioAnimStarted = UUcTrue;
			
			// block until animation finishes
			return UUcTrue;

		} else if (*ioAnimStarted && !inRepeatAnimation) {
			// our animation has finished!
			return UUcFalse;

		} else if (active_character->nextAnimType == inAnimType) {
			// our animation is queued up, block until it arrives
			return UUcTrue;

		} else {
			// the animation isn't queued up, play it
			result_anim = ONrCharacter_DoAnimation(ioCharacter, active_character, ONcAnimPriority_Appropriate, inAnimType);
			if (result_anim == NULL) {
				if (TRrAnimation_GetTo(active_character->animation) == ONcAnimState_Standing) {
					// we can't play this animation, abort
					COrConsole_Printf("### %s: can't play animation of type %s from a standing state",
									ioCharacter->player_name, ONrAnimTypeToString(inAnimType));
					*ioAnimStarted = UUcTrue;
					return UUcFalse;
				} else {
					// keep trying to play the animation (we will eventually be standing)
				}
			}

			// block until we get a chance to play the animation, or decide to abort
			return UUcTrue;
		}
	} else {
		if (active_character->curAnimType == inAnimType) {
			// make sure that we remain playing this animation for the duration... this will be reset every tick
			active_character->animationLockFrames = 2;

		} else {
			// play the animation
			ONrCharacter_DoAnimation(ioCharacter, active_character, ONcAnimPriority_Appropriate, inAnimType);
		}

		// don't block
		return UUcFalse;
	}
}

