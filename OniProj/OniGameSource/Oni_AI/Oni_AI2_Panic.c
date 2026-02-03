/*
	FILE:	Oni_AI2_Panic.c

	AUTHOR:	Chris Butcher

	CREATED: July 16, 2000

	PURPOSE: Noncombatant AI for Oni

	Copyright (c) Bungie Software, 2000

*/

#define AI_VERBOSE_PANIC		0

#include "bfw_math.h"

#include "Oni_AI2.h"
#include "Oni_Character.h"

// ------------------------------------------------------------------------------------
// -- constants

#define AI2cPanic_KneelDistance			20.0f
#define AI2cPanic_StopUponDeathTimer	180

#define AI2cPanic_InitialSayTimer		45
#define AI2cPanic_Damage_ReduceTimer	5
#define AI2cPanic_NextSayTimer_Min		1500
#define AI2cPanic_NextSayTimer_Max		2000

// ------------------------------------------------------------------------------------
// -- high-level state functions

// enter our panic behavior
void AI2rPanic_Enter(ONtCharacter *ioCharacter)
{
	AI2tPanicState *panic_state;

	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Panic);
	panic_state = &ioCharacter->ai2State.currentState->state.panic;

#if AI_VERBOSE_PANIC
	COrConsole_Printf("%s: entering panic behavior", ioCharacter->player_name);
#endif

	// cower in fear
	ONrCharacter_DisableFightVarient(ioCharacter, UUcTrue);
	ONrCharacter_DisableWeaponVarient(ioCharacter, UUcTrue);
	ONrCharacter_SetPanicVarient(ioCharacter, UUcTrue);
	panic_state->kneeling = UUcFalse;
	AI2rPath_Halt(ioCharacter);
	AI2rMovement_ChangeMovementMode(ioCharacter, AI2cMovementMode_NoAim_Run);
	AI2rMovement_FreeFacing(ioCharacter);
	AI2rMovement_StopAiming(ioCharacter);
	AI2rMovement_StopGlancing(ioCharacter);
	panic_state->panic_cause = NULL;
	panic_state->panic_endtime = 0;
	panic_state->currently_dazed = UUcFalse;
	panic_state->panicked_by_hostile = UUcFalse;
	panic_state->say_timer = AI2cPanic_InitialSayTimer;
}

// exit our panic behavior
void AI2rPanic_Exit(ONtCharacter *ioCharacter)
{
	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Panic);

#if AI_VERBOSE_PANIC
	COrConsole_Printf("%s: exiting panic behavior", ioCharacter->player_name);
#endif

	// reset our movement state, and return to normal
	AI2rMovement_ChangeMovementMode(ioCharacter, AI2cMovementMode_Default);
	AI2rMovement_StopAiming(ioCharacter);
	ONrCharacter_SetPanicVarient(ioCharacter, UUcFalse);
	ONrCharacter_DisableFightVarient(ioCharacter, UUcFalse);
	ONrCharacter_DisableWeaponVarient(ioCharacter, UUcFalse);
}

void AI2rPanic_Update(ONtCharacter *ioCharacter)
{
	AI2tPanicState *panic_state;
	M3tPoint3D current_point;
	UUtUns32 current_time;
	UUtBool enemy_behind, do_kneel;
	ONtCharacter *enemy;
	ONtActiveCharacter *active_character;

	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Panic);
	panic_state = &ioCharacter->ai2State.currentState->state.panic;

	current_time = ONrGameState_GetGameTime();
	if (current_time > panic_state->panic_endtime) {
		// finish panicking
		AI2rReturnToJob(ioCharacter, UUcTrue, UUcTrue);
		return;
	}

	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character == NULL) {
		AI2rReturnToJob(ioCharacter, UUcTrue, UUcTrue);
		return;
	}

	// randomly whimper
	if (panic_state->say_timer > 0) {
		panic_state->say_timer--;
	}
	if (panic_state->say_timer == 0) {
		AI2rVocalize(ioCharacter, AI2cVocalization_Cower, UUcFalse);
		panic_state->say_timer = UUmRandomRange(AI2cPanic_NextSayTimer_Min, AI2cPanic_NextSayTimer_Max - AI2cPanic_NextSayTimer_Min);
	}

	enemy = (panic_state->panic_cause) ? panic_state->panic_cause->enemy : NULL;

	if (!panic_state->kneeling) {
		do_kneel = ONrAnimState_IsCrouch(active_character->nextAnimState);

		if ((!do_kneel) && (panic_state->panic_cause != NULL)) {
			ONrCharacter_GetPathfindingLocation(ioCharacter, &current_point);
			if (MUmVector_GetDistanceSquared(current_point, panic_state->panic_cause->last_location) < UUmSQR(AI2cPanic_KneelDistance)) {
				do_kneel = UUcTrue;
			}
		}

		if (do_kneel) {
			panic_state->kneeling = UUcTrue;
			AI2rMovement_ChangeMovementMode(ioCharacter, AI2cMovementMode_Creep);
			AI2rMovement_StopAiming(ioCharacter);
		}

	} else if (ONrAnimState_IsFallen(active_character->nextAnimState)) {
		// we are falling down or already fallen.
		if (panic_state->currently_dazed) {
			if (ioCharacter->ai2State.daze_timer == 0) {
				// we can now get up
				panic_state->currently_dazed = UUcFalse;
			}

		} else if (ioCharacter->ai2State.already_done_daze) {
			enemy_behind = UUcFalse;
			if ((panic_state->panic_cause != NULL) && (panic_state->panic_cause->enemy != NULL)) {
				if (!panic_state->kneeling) {
					// start looking again at our enemy
					AI2rMovement_LookAtCharacter(ioCharacter, panic_state->panic_cause->enemy);
				}

				if ((float)fabs(ONrCharacter_RelativeAngleToCharacter(ioCharacter, panic_state->panic_cause->enemy)) > M3cHalfPi) {
					enemy_behind = UUcTrue;
				}
			}

			// send a keypress through to the executor telling us to get up
			AI2rExecutor_MoveOverride(ioCharacter,	((enemy_behind) ? LIc_BitMask_Forward : LIc_BitMask_Backward) |
													((panic_state->kneeling) ? LIc_BitMask_Crouch : 0));

		} else {
			// we have just been knocked down and haven't set up our daze yet
			panic_state->currently_dazed = UUcTrue;
			ioCharacter->ai2State.already_done_daze = UUcTrue;
			AI2rStartDazeTimer(ioCharacter);

			// we have been knocked down, kneel when we get up
			panic_state->kneeling = UUcTrue;
			AI2rMovement_ChangeMovementMode(ioCharacter, AI2cMovementMode_Creep);
			AI2rMovement_StopAiming(ioCharacter);
		}

		if (panic_state->currently_dazed) {
			// don't move
			AI2rPath_Halt(ioCharacter);
			AI2rMovement_StopAiming(ioCharacter);
			AI2rMovement_StopGlancing(ioCharacter);
		}
	}
}

// ------------------------------------------------------------------------------------
// -- contact and knowledge management

// new-knowledge notification callback for non-combatant characters
void AI2rNonCombatant_NotifyKnowledge(ONtCharacter *ioCharacter, AI2tKnowledgeEntry *inEntry,
										UUtBool inDegrade)
{
	AI2tKnowledgeState *knowledge_state;
	ONtCharacter *neutral_char;
	M3tPoint3D current_pt;
	UUtBool is_danger, only_panic_from_hostiles;

	UUmAssert(ioCharacter->charType == ONcChar_AI2);
	UUmAssert(ioCharacter == inEntry->owner);

	if (inDegrade) {
		if (ioCharacter->ai2State.currentGoal == AI2cGoal_Panic) {
			AI2tPanicState *panic_state = &ioCharacter->ai2State.currentState->state.panic;

			if ((inEntry == panic_state->panic_cause) && ((inEntry->strength <= AI2cContactStrength_Forgotten) || !(inEntry->has_been_hostile))) {
				// are we still in danger?
				if (AI2rCharacter_InDanger(ioCharacter, NULL, panic_state->panicked_by_hostile, &panic_state->panic_cause)) {
#if AI_VERBOSE_PANIC
					COrConsole_Printf("%s: changed panic cause", ioCharacter->player_name);
#endif
				} else {
					// we can stop panicking now
					panic_state->panic_cause = NULL;
					panic_state->panic_endtime = inEntry->last_time + AI2cPanic_StopUponDeathTimer;
#if AI_VERBOSE_PANIC
					COrConsole_Printf("%s: stop panic, the cause is gone", ioCharacter->player_name);
#endif
				}
			}
		}

		return;
	}

	knowledge_state = &ioCharacter->ai2State.knowledgeState;

	if ((inEntry->priority == AI2cContactPriority_Hostile_Threat) || (AI2rKnowledge_IsCombatSound(inEntry->last_type))) {
		// are we in a neutral interaction?
		if (ioCharacter->ai2State.currentGoal == AI2cGoal_Neutral) {
			neutral_char = ioCharacter->ai2State.currentState->state.neutral.target_character;
		} else {
			neutral_char = NULL;
		}

		// by default, we are panicked by gunfire or fighting from friendlies too
		only_panic_from_hostiles = UUcFalse;
		if (ioCharacter->ai2State.currentGoal == AI2cGoal_Panic) {
			only_panic_from_hostiles = ioCharacter->ai2State.currentState->state.panic.panicked_by_hostile;
		}

		// is this entry actually a danger to us or our friend?
		ONrCharacter_GetPathfindingLocation(ioCharacter, &current_pt);
		is_danger = AI2rCharacter_IsDangerousContact(ioCharacter, neutral_char, only_panic_from_hostiles, inEntry, &current_pt);

		if (is_danger) {
			if (ioCharacter->ai2State.currentGoal == AI2cGoal_Neutral) {
#if AI_VERBOSE_PANIC
				COrConsole_Printf("%s: danger, aborting neutral interaction", ioCharacter->player_name);
#endif
				// abort our interaction
				AI2rNeutral_Stop(ioCharacter, UUcFalse, UUcTrue, UUcTrue, UUcTrue);
			}

			// panic!
#if AI_VERBOSE_PANIC
			COrConsole_Printf("%s: cowering in fear", ioCharacter->player_name);
#endif
			AI2rCharacter_Panic(ioCharacter, inEntry, 0);
		}
	}

	if ((ioCharacter->ai2State.currentGoal != AI2cGoal_Panic) && (ioCharacter->neutral_interaction_char == NULL)) {
		// look towards the source of the contact - look fast if it's a dangerous sound
		AI2rMovement_GlanceAtPoint(ioCharacter, &inEntry->last_location,
									AI2rAlert_GetGlanceDuration(inEntry->last_type), UUcFalse, 0);
	}
}

// an entry in our knowledge base has just been removed, forcibly delete any references we had to it
UUtBool AI2rPanic_NotifyKnowledgeRemoved(ONtCharacter *ioCharacter, AI2tPanicState *ioPanicState, AI2tKnowledgeEntry *inEntry)
{
	if (inEntry != ioPanicState->panic_cause) {
		return UUcFalse;
	}

	ioPanicState->panic_cause = NULL;
	ioPanicState->panic_endtime = inEntry->last_time + AI2cPanic_StopUponDeathTimer;
#if AI_VERBOSE_PANIC
	COrConsole_Printf("%s: stop panic, the cause is gone", ioCharacter->player_name);
#endif
	return UUcTrue;
}

// check to see if a character needs to worry about a hostile contact
// optional inInteractingWithCharacter: also consider if these enemies are a danger to character we're talking to
UUtBool AI2rCharacter_IsDangerousContact(ONtCharacter *ioCharacter, ONtCharacter *inInteractingWithCharacter,
								   UUtBool inConsiderOnlyHostiles, AI2tKnowledgeEntry *inEntry, M3tPoint3D *inCurrentLocation)
{
	UUtBool is_hostile, will_panic;

	if (inEntry->strength <= AI2cContactStrength_Forgotten) {
		// this is no longer a factor
		return UUcFalse;

	} else if (AI2rKnowledge_IsHurtEvent(inEntry->last_type)) {
		// always worry about this
		return UUcTrue;

	}

	switch(inEntry->last_type) {
		case AI2cContactType_Sound_Melee:
		case AI2cContactType_Sound_Gunshot:
		case AI2cContactType_Sight_Definite:
		case AI2cContactType_Hit_Weapon:
		case AI2cContactType_Hit_Melee:
			will_panic = UUcTrue;
		break;

		default:
			will_panic = UUcFalse;
		break;
	}
	if (!will_panic) {
		// this isn't a sufficiently worrying kind of contact to make us panic
		return UUcFalse;
	}

	// skip knowledge entries about non-hostile events
	if ((inEntry->enemy == ioCharacter) || (inEntry->enemy == NULL)) {
		return UUcFalse;
	}

	// worry about characters that are interested in hurting us
	is_hostile = AI2rCharacter_PotentiallyHostile(inEntry->enemy, ioCharacter, UUcTrue);
	if (!inConsiderOnlyHostiles) {
		// also worry about sounds of combat from _any_ character
		is_hostile = is_hostile || AI2rKnowledge_IsCombatSound(inEntry->last_type);
	}

	if (!is_hostile) {
		// also worry about characters that are interested in hurting our interacting character
		if ((inInteractingWithCharacter != NULL) && (AI2rCharacter_PotentiallyHostile(inEntry->enemy, inInteractingWithCharacter, UUcTrue))) {
			is_hostile = UUcTrue;
		}
	}

	if (!is_hostile) {
		return UUcFalse;
	}

	// skip characters that are a long way away
	if (MUmVector_GetDistanceSquared(*inCurrentLocation, inEntry->last_location) > UUmSQR(ioCharacter->ai2State.neutralEnemyRange)) {
		return UUcFalse;
	}

	// FIXME: in future consider whether this enemy is actually attacking?
	// FIXME: get level of danger to determine our panic reaction (cower, crouch, run)?
	return UUcTrue;
}

// check to see if a neutral character can see enemies close enough to be scared
// optional inInteractingWithCharacter: also consider if these enemies are a danger to character we're talking to
UUtBool AI2rCharacter_InDanger(ONtCharacter *ioCharacter, ONtCharacter *inInteractingWithCharacter,
							   UUtBool inConsiderOnlyHostiles, AI2tKnowledgeEntry **outDangerousContact)
{
	AI2tKnowledgeEntry *entry;
	M3tPoint3D our_point;

	ONrCharacter_GetPathfindingLocation(ioCharacter, &our_point);

	for (entry = ioCharacter->ai2State.knowledgeState.contacts; entry != NULL; entry = entry->nextptr) {
		if (AI2rCharacter_IsDangerousContact(ioCharacter, inInteractingWithCharacter, inConsiderOnlyHostiles, entry, &our_point)) {
			if (outDangerousContact != NULL) {
				*outDangerousContact = entry;
			}
			return UUcTrue;
		}
	}

	return UUcFalse;
}

// we're in danger; panic
void AI2rCharacter_Panic(ONtCharacter *ioCharacter, AI2tKnowledgeEntry *inEntry, UUtUns32 inPanicTime)
{
	AI2tPanicState *panic_state;
	UUtUns32 new_endtime, decrease_timer;

	if (inEntry != NULL) {
		switch(inEntry->last_type) {
			case AI2cContactType_Sound_Melee:
				inPanicTime = ioCharacter->ai2State.combatSettings.panic_melee;
			break;

			case AI2cContactType_Sound_Gunshot:
				inPanicTime = ioCharacter->ai2State.combatSettings.panic_gunfire;
			break;

			case AI2cContactType_Sight_Definite:
				inPanicTime = ioCharacter->ai2State.combatSettings.panic_sight;
			break;

			case AI2cContactType_Hit_Weapon:
			case AI2cContactType_Hit_Melee:
				inPanicTime = ioCharacter->ai2State.combatSettings.panic_hurt;
			break;

			default:
				// don't panic
				return;
			break;
		}
	}

	if (ioCharacter->ai2State.currentGoal != AI2cGoal_Panic) {
		// enter the panic state
		AI2rExitState(ioCharacter);

		ioCharacter->ai2State.currentGoal = AI2cGoal_Panic;
		panic_state = &ioCharacter->ai2State.currentStateBuffer.state.panic;
		UUrMemory_Clear(panic_state, sizeof(AI2tPanicState));
		ioCharacter->ai2State.currentState = &(ioCharacter->ai2State.currentStateBuffer);
		ioCharacter->ai2State.currentState->begun = UUcFalse;

		AI2rEnterState(ioCharacter);
	} else {
		// we're already panicking
		panic_state = &ioCharacter->ai2State.currentStateBuffer.state.panic;
	}

	if (inEntry->priority >= AI2cContactPriority_Hostile_NoThreat) {
		panic_state->panicked_by_hostile = UUcTrue;
#if AI_VERBOSE_PANIC
		COrConsole_Printf("%s: panicked by hostile %s", ioCharacter->player_name, (inEntry->enemy == NULL) ? "none" : inEntry->enemy->player_name);
#endif
	}

	if ((inEntry != NULL) && ((inEntry->last_type == AI2cContactType_Hit_Weapon) ||
								(inEntry->last_type == AI2cContactType_Hit_Melee))) {
		// we are being hurt, make the AI more likely to scream
		decrease_timer = inEntry->last_user_data * AI2cPanic_Damage_ReduceTimer;

		if (panic_state->say_timer < decrease_timer) {
			panic_state->say_timer = 0;
		} else {
			panic_state->say_timer -= decrease_timer;
		}
	}

	// panic for the designated time period + 0-50%
	new_endtime = ONrGameState_GetGameTime() + UUmRandomRange(inPanicTime, inPanicTime / 2);
	panic_state->panic_endtime = UUmMax(panic_state->panic_endtime, new_endtime);

	if ((inEntry != NULL) && (inEntry->enemy != NULL)) {
		// is this now our most disturbing contact?
		if ((panic_state->panic_cause == NULL) || (inEntry->strength > panic_state->panic_cause->strength)) {
			if (panic_state->panic_cause != NULL) {
				if ((panic_state->panic_cause->enemy != NULL) && (inEntry->enemy == NULL)) {
					// stick with the previous cause, it at least has an actual character to go with it
					return;
				}

				if (panic_state->panic_cause->priority > inEntry->priority) {
					// stick with the previous cause, it is a higher priority
					return;
				}
			}

#if AI_VERBOSE_PANIC
			COrConsole_Printf("%s: now being panicked by %s (previous %s)", ioCharacter->player_name, inEntry->enemy->player_name,
					(panic_state->panic_cause == NULL) ? "none" :
					((panic_state->panic_cause->enemy == NULL) ? "none" : panic_state->panic_cause->enemy->player_name));
#endif
			// switch to being panicked by this character
			panic_state->panic_cause = inEntry;
			if ((inEntry->enemy != NULL) && (!panic_state->kneeling)) {
				AI2rMovement_LookAtCharacter(ioCharacter, inEntry->enemy);
			}
		}
	}
}
