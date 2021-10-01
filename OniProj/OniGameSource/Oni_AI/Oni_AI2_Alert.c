/*
	FILE:	 Oni_AI2_Alert.c
	
	AUTHOR:	 Chris Butcher
	
	CREATED: April 19, 2000
	
	PURPOSE: Alert Status for Oni's AI
	
	Copyright (c) 2000

*/

#include "BFW.h"
#include "BFW_MathLib.h"

#include "Oni_AI2.h"
#include "Oni_Character.h"
#include "Oni_Character_Animation.h"

// ------------------------------------------------------------------------------------
// -- constants

const char AI2cAlertStatusName[AI2cAlertStatus_Max][16]
	 = {"lull", "low", "medium", "high", "combat"};

const AI2tAlertStatus AI2cContact_ChangeAlert[AI2cContactType_Max] =
	{AI2cAlertStatus_Lull, AI2cAlertStatus_Low, AI2cAlertStatus_Medium, AI2cAlertStatus_Medium, AI2cAlertStatus_High,		// sounds
	 AI2cAlertStatus_Low, AI2cAlertStatus_Combat,													// visual
	 AI2cAlertStatus_Combat, AI2cAlertStatus_Combat,												// hit
	 AI2cAlertStatus_High};																			// alarm

const UUtUns32 AI2cContact_GlanceLength[AI2cContactType_Max] =
	{30, 60, 90, 120, 120,					// sounds
	180, 180,								// visual
	600, 600,								// hit
	300};									// alarm

#define AI2cAlert_StartleFrameTolerance			60
#define AI2cAlert_VocalizationTimer				60

// ------------------------------------------------------------------------------------
// -- internal function prototypes

static UUtBool AI2iAlert_Startle(ONtCharacter *ioCharacter, AI2tAlertStatus inFromStatus, AI2tKnowledgeEntry *inContact);

// ------------------------------------------------------------------------------------
// -- function definitions

UUtUns32 AI2rAlert_GetGlanceDuration(AI2tContactType inContactType)
{
	UUmAssert((inContactType >= 0) && (inContactType < AI2cContactType_Max));
	return AI2cContact_GlanceLength[inContactType];
}

// new-knowledge notification callback for all non-alerted states (idle, guard, patrol)
void AI2rAlert_NotifyKnowledge(ONtCharacter *ioCharacter, AI2tKnowledgeEntry *inEntry,
								UUtBool inDegrade)
{
	AI2tKnowledgeState *knowledge_state;
	AI2tAlertStatus new_status, old_status;
	UUtUns32 glance_length;
	UUtBool startled, non_threat;

	UUmAssert(ioCharacter->charType == ONcChar_AI2);
	UUmAssert(ioCharacter == inEntry->owner);
	knowledge_state = &ioCharacter->ai2State.knowledgeState;

	if (inDegrade) {
		// means nothing to us that the contact's strength is fading - alert status degrades elsewhere
		return;
	}

	if ((ioCharacter->ai2State.currentGoal == AI2cGoal_Patrol) && (ioCharacter->ai2State.currentState->state.patrol.locked_in)) {
		// we are locked onto our current path. don't startle, investigate, go into combat or change alert status
		return;
	}

	if (inEntry != knowledge_state->contacts) {
		// we may well have new info, but it's not about our currently highest-priority
		// contact. if we have the highest-priority contact in sight, then don't even look
		// over there.
		if (knowledge_state->contacts->strength == AI2cContactStrength_Definite)
			return;
	}

	if ((inEntry->last_type < 0) || (inEntry->last_type >= AI2cContactType_Max)) {
		AI2_ERROR(AI2cBug, AI2cSubsystem_Knowledge, AI2cError_Knowledge_InvalidContact, ioCharacter,
					inEntry->last_type, 0, 0, 0);
		return;
	}
	glance_length = AI2rAlert_GetGlanceDuration(inEntry->last_type);

	non_threat = UUcTrue;
	if (AI2rKnowledge_IsCombatSound(inEntry->last_type)) {
		non_threat = UUcFalse;

	} else if (inEntry->priority >= AI2cContactPriority_Hostile_NoThreat) {
		non_threat = UUcFalse;

	} else  {
		// this contact is not a hostile.
		if (inEntry->strength > inEntry->last_strength) {
			// if we just got an upgrade in our contact strength then it's
			// a new thing for us. turn our head lazily to glance at them.
			AI2rMovement_GlanceAtPoint(ioCharacter, &inEntry->last_location, glance_length, UUcFalse, 0);
			ioCharacter->ai2State.knowledgeState.vision_delay_timer = glance_length;
		}
		return;
	}

	// this is a hostile that we have to react to.
	if (inEntry != knowledge_state->contacts) {
		// this is not the most important threat, don't do anything
		return;
	}

	if (inEntry->strength <= AI2cContactStrength_Forgotten) {
		// don't do anything about contacts of this strength
		return;
	}

	// this is greater-or-equal-than because we need to keep alert if we're getting
	// repeated knowledge entries
	startled = UUcFalse;
	new_status = AI2cContact_ChangeAlert[inEntry->last_type];
	old_status = ioCharacter->ai2State.alertStatus;

	if (new_status >= old_status) {
		startled = AI2rAlert_UpgradeStatus(ioCharacter, new_status, inEntry);

		if ((ioCharacter->ai2State.alertStatus > old_status) && ((ioCharacter->ai2State.flags & AI2cFlag_SaidAlertSound) == 0)) {
			if ((startled) || (ioCharacter->ai2State.alertStatus == AI2cAlertStatus_Combat)) {
				// say our startle sound immediately
				AI2rVocalize(ioCharacter, AI2cVocalization_Startle, UUcFalse);
				ioCharacter->ai2State.alertVocalizationTimer = 0;
				ioCharacter->ai2State.flags |= AI2cFlag_SaidAlertSound;

			} else if (ioCharacter->ai2State.alertVocalizationTimer == 0) {
				// set up a timer so we will say our alert sound soon if we don't get fully alerted first
				ioCharacter->ai2State.alertVocalizationTimer = AI2cAlert_VocalizationTimer + UUmRandomRange(0, AI2cAlert_VocalizationTimer / 2);
			}
		}
	}

	if (ioCharacter->ai2State.alertStatus == AI2cAlertStatus_Lull) {
		// don't investigate contacts when we're in the lull alert state
		return;
	}

	if (ioCharacter->ai2State.currentGoal == AI2cGoal_Neutral) {
		M3tPoint3D current_pt;

		ONrCharacter_GetPathfindingLocation(ioCharacter, &current_pt);
		if (AI2rCharacter_IsDangerousContact(ioCharacter, ioCharacter->ai2State.currentState->state.neutral.target_character, UUcTrue, inEntry, &current_pt)) {
			// abort our interaction
			AI2rNeutral_Stop(ioCharacter, UUcFalse, UUcTrue, UUcTrue, UUcTrue);

		} else {
			// skip this contact - it's not relevant to us
			return;
		}
	}

	// should we investigate this contact? only check if we aren't in one of the combat states already
	if ((ioCharacter->ai2State.currentGoal != AI2cGoal_Pursuit) &&
		(ioCharacter->ai2State.currentGoal != AI2cGoal_Combat) &&
		(ioCharacter->ai2State.currentGoal != AI2cGoal_Alarm)) {
		// enter pursuit mode
		AI2rExitState(ioCharacter);

		ioCharacter->ai2State.currentGoal = AI2cGoal_Pursuit;
		ioCharacter->ai2State.currentStateBuffer.state.pursuit.target = inEntry;
		ioCharacter->ai2State.currentState = &(ioCharacter->ai2State.currentStateBuffer);
		ioCharacter->ai2State.currentState->begun = UUcFalse;

		AI2rEnterState(ioCharacter);
	}
}

// change alert state
UUtBool AI2rAlert_UpgradeStatus(ONtCharacter *ioCharacter, AI2tAlertStatus inStatus, AI2tKnowledgeEntry *inContact)
{
	UUtBool startled = UUcFalse;
	AI2tAlertStatus old_status;

	UUmAssertReadPtr(ioCharacter, sizeof(ONtCharacter));
	UUmAssert(ioCharacter->charType == ONcChar_AI2);

	old_status = ioCharacter->ai2State.alertStatus;
	if (inStatus != old_status) {
		ioCharacter->ai2State.alertStatus = inStatus;
		AI2rMovement_NotifyAlertChange(ioCharacter);

		if ((inStatus >= AI2cAlertStatus_Medium) && (ioCharacter->ai2State.alertInitiallyRaisedTime == 0)) {
			// we have been at a low alert before this time; we will only startle if we get alerted within the
			// next AI2cAlert_StartleFrameTolerance frames, otherwise we are mentally
			ioCharacter->ai2State.alertInitiallyRaisedTime = (inContact == NULL) ? ONrGameState_GetGameTime() : inContact->last_time;
		}

		// don't startle if we received a direct order to attack
		startled = AI2iAlert_Startle(ioCharacter, old_status, inContact);
		if (startled) {
			UUmAssert(inContact != NULL);
			ioCharacter->ai2State.startleTimer = inContact->last_time;
		}
	}
	ioCharacter->ai2State.alertTimer = 0;

	if (inStatus == AI2cAlertStatus_Combat && (inContact != NULL) && (inContact->enemy != NULL)) {
		// enter combat mode
		AI2rExitState(ioCharacter);

		ioCharacter->ai2State.currentGoal = AI2cGoal_Combat;
		ioCharacter->ai2State.currentStateBuffer.state.combat.target = inContact->enemy;
		ioCharacter->ai2State.currentStateBuffer.state.combat.target_knowledge = inContact;
		ioCharacter->ai2State.currentState = &(ioCharacter->ai2State.currentStateBuffer);
		ioCharacter->ai2State.currentState->begun = UUcFalse;

		AI2rEnterState(ioCharacter);
	}

	return startled;
}

void AI2rAlert_DowngradeStatus(ONtCharacter *ioCharacter, AI2tAlertStatus inStatus, AI2tKnowledgeEntry *inContact,
							   UUtBool inLeavingCombat)
{
	UUmAssertReadPtr(ioCharacter, sizeof(ONtCharacter));
	UUmAssert(ioCharacter->charType == ONcChar_AI2);

	// don't leave combat alert if we are still in combat mode
	if ((!inLeavingCombat) && (ioCharacter->ai2State.currentGoal == AI2cGoal_Combat))
		return;

	if (ioCharacter->ai2State.alertStatus != inStatus) {
		ioCharacter->ai2State.alertStatus = inStatus;
		AI2rMovement_NotifyAlertChange(ioCharacter);

		if (inStatus < AI2cAlertStatus_Medium) {
			// go back to low alert; we will startle again now
			ioCharacter->ai2State.alertInitiallyRaisedTime = 0;
		}
	}
}

void AI2rAlert_Decay(ONtCharacter *ioCharacter)
{
	if (ioCharacter->ai2State.alertStatus <= ioCharacter->ai2State.minimumAlert)
		return;

	if (ioCharacter->ai2State.alertTimer == 0) {
		switch(ioCharacter->ai2State.alertStatus) {
		case AI2cAlertStatus_Combat:		// should never happen! this routine is only called from casual modes
		case AI2cAlertStatus_High:
			ioCharacter->ai2State.alertTimer = ioCharacter->characterClass->memory.high_to_med;
			break;

		case AI2cAlertStatus_Medium:
			ioCharacter->ai2State.alertTimer = ioCharacter->characterClass->memory.med_to_low;
			break;

		case AI2cAlertStatus_Low:
			ioCharacter->ai2State.alertTimer = ioCharacter->characterClass->memory.low_to_lull;
			break;

		case AI2cAlertStatus_Lull:			// should never happen, can't drop below lull
		default:
			UUmAssert(!"AI2rAlert_Decay: unknown alert status");
			ioCharacter->ai2State.alertTimer = 0;
		}

		if (ioCharacter->ai2State.alertTimer == 0) {
			// we don't decay from this alert status
			return;
		}
	}

	ioCharacter->ai2State.alertTimer--;
	if (ioCharacter->ai2State.alertTimer == 0) {
		// drop our alert state by one
		UUmAssert(ioCharacter->ai2State.alertStatus > AI2cAlertStatus_Lull);
		AI2rAlert_DowngradeStatus(ioCharacter, (AI2tAlertStatus)(ioCharacter->ai2State.alertStatus - 1), NULL, UUcFalse);
	}
}

static UUtBool AI2iAlert_Startle(ONtCharacter *ioCharacter, AI2tAlertStatus inFromStatus, AI2tKnowledgeEntry *inContact)
{
	M3tVector3D direction;
	float angle, contact_angle;
	UUtUns16 startle_type;
	const TRtAnimation *startle_anim;

	// don't startle if we are set to never do so
	if (ioCharacter->characterClass->ai2_behavior.flags & AI2cBehaviorFlag_NeverStartle)
		return UUcFalse;

	// don't startle if this isn't a high alert that we're going to
	if (ioCharacter->ai2State.alertStatus < AI2cAlertStatus_High)
		return UUcFalse;
	
	// don't startle if this is not a result of a contact, but an alarm
	if (inContact == NULL)
		return UUcFalse;

	// don't startle if we received a direct order to attack
	if (inContact->priority == AI2cContactPriority_ForceTarget)
		return UUcFalse;

	// don't startle if we have previously actually seen the character
	if ((!inContact->first_sighting) && (inContact->highest_strength >= AI2cContactStrength_Definite))
		return UUcFalse;

	// don't startle if we have been alerted more than AI2cAlert_StartleFrameTolerance ago
	// by yelling, combat sounds or gunfire
	if ((ioCharacter->ai2State.alertInitiallyRaisedTime != 0) &&
		(inContact->last_time > ioCharacter->ai2State.alertInitiallyRaisedTime + AI2cAlert_StartleFrameTolerance))
		return UUcFalse;

	// work out whereabouts the contact is
	MUmVector_Subtract(direction, inContact->last_location, ioCharacter->location);
	contact_angle = MUrATan2(direction.x, direction.z);
	UUmTrig_ClipLow(contact_angle);

	// work out which quadrant relative to us this is... add 45 degrees to make it easy to determine which quadrant
	angle = contact_angle - ioCharacter->facing + M3cQuarterPi;
	UUmTrig_Clip(angle);

	if (angle < M3cHalfPi) {
		startle_type = ONcAnimType_Startle_Forward;
	} else if (angle < M3cPi) {
		startle_type = ONcAnimType_Startle_Left;
	} else if (angle < M3cPi + M3cHalfPi) {
		startle_type = ONcAnimType_Startle_Back;
	} else {
		startle_type = ONcAnimType_Startle_Right;
	}

	// send the character into fight mode and enable our weapon variant so that we find the startle anim
	ONrCharacter_FightMode(ioCharacter);
	ONrCharacter_DisableWeaponVarient(ioCharacter, UUcFalse);

	startle_anim = ONrCharacter_TryImmediateAnimation(ioCharacter, startle_type);
	if (startle_anim == NULL) {
		AI2_ERROR(AI2cWarning, AI2cSubsystem_Knowledge, AI2cError_Knowledge_NoStartle,
					ioCharacter, startle_type, 0, 0, 0);
		return UUcFalse;
	}

	// set up for the startle
	ioCharacter->ai2State.startle_length		= TRrAnimation_GetDuration(startle_anim);
	ioCharacter->ai2State.startle_startfacing	= ioCharacter->facing;
	ioCharacter->ai2State.startle_deltafacing	= contact_angle - ioCharacter->facing
													- TRrAnimation_GetFinalRotation(startle_anim);
	UUmTrig_ClipAbsPi(ioCharacter->ai2State.startle_deltafacing);

/*	COrConsole_Printf("%s: set up startle animation, length %d, from %f to %f (delta %f)", ioCharacter->player_name,
		ioCharacter->ai2State.startle_length, ioCharacter->ai2State.startle_startfacing,
		contact_angle, ioCharacter->ai2State.startle_deltafacing);*/

	ONrCharacter_SetAnimation_External(ioCharacter, TRrAnimation_GetFrom(startle_anim), startle_anim, 8);
	return UUcTrue;
}

// change alert status of all AIs
void AI2rAlert_DebugAlertAll(AI2tAlertStatus inAlert)
{
	ONtCharacter **charptr;
	UUtUns32 num_chars, itr;

	charptr = ONrGameState_LivingCharacterList_Get();
	num_chars = ONrGameState_LivingCharacterList_Count();

	for (itr = 0; itr < num_chars; itr++, charptr++) {
		if (((*charptr)->charType != ONcChar_AI2) ||
			(((*charptr)->flags & ONcCharacterFlag_InUse) == 0) ||
			((*charptr)->flags & ONcCharacterFlag_Dead))
			continue;

		if ((*charptr)->ai2State.alertStatus < inAlert)
			AI2rAlert_UpgradeStatus(*charptr, inAlert, NULL);
		else
			AI2rAlert_DowngradeStatus(*charptr, inAlert, NULL, UUcTrue);
	}
}


