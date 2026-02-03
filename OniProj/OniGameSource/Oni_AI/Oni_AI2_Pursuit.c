/*
	FILE:	Oni_AI2_Pursuit.c

	AUTHOR:	Chris Butcher

	CREATED: June 21, 2000

	PURPOSE: Pursuit AI for Oni

	Copyright (c) 2000

*/

#include "Oni_AI2.h"
#include "Oni_AI2_Pursuit.h"
#include "Oni_Character.h"

#define DEBUG_VERBOSE_PURSUIT		0

// ------------------------------------------------------------------------------------
// -- constants

#define AI2cPursuit_GoToContact_ScanDist		200.0f
#define AI2cPursuit_GoToContact_StopDist		60.0f
#define AI2cPursuit_GoToContact_CloseDist		AI2cCombat_InvestigateMinRange
#define AI2cPursuit_GoToContact_LOSFrames		15
#define AI2cPursuit_GoToContact_ScanMinFrames	40
#define AI2cPursuit_GoToContact_ScanMaxFrames	80
#define AI2cPursuit_GoToContact_ScanAngleRange	(70.0f * M3cDegToRad)
#define AI2cPursuit_RequiredLOSChecks			6

#define AI2cPursuit_Look_DirMinFrames			100
#define AI2cPursuit_Look_DirMaxFrames			200
#define AI2cPursuit_Look_AngleMinFrames			40
#define AI2cPursuit_Look_AngleMaxFrames			80
#define AI2cPursuit_Look_AngleRange				(45.0f * M3cDegToRad)

#define AI2cPursuit_Glance_Friendly				AI2cPursuit_Glance_InitialFrames
#define AI2cPursuit_Glance_InitialFrames		80
#define AI2cPursuit_Glance_AngleMinFrames		40
#define AI2cPursuit_Glance_AngleMaxFrames		80
#define AI2cPursuit_Glance_AngleRange			(45.0f * M3cDegToRad)

#define AI2cPursuit_Delay_ReachAreaLookFrames	600

#define AI2cPursuit_AimingSpeed					0.5f

#define AI2cPursuit_InitialSayTimer				120
#define AI2cPursuit_NextSayTimer_Min			720
#define AI2cPursuit_NextSayTimer_Max			900


enum {
	AI2cPursuitCheck_InitialContact,
	AI2cPursuitCheck_PostGlance,
	AI2cPursuitCheck_PostArrival,
	AI2cPursuitCheck_ChangeTarget,

	AI2cPursuitCheck_Max
};

const char *AI2cPursuitModeName[AI2cPursuitMode_Max] =
	{"None", "Forget", "GoTo", "Wait", "Look", "Move", "Hunt", "Glance"};

const char *AI2cPursuitLostName[AI2cPursuitLost_Max] =
	{"Return To Job", "Keep Looking", "Find Alarm"};

const char *AI2cPursuitCheckType[AI2cPursuitCheck_Max] = {"initial-contact", "post-glance", "post-arrival"};

// ------------------------------------------------------------------------------------
// -- internal function prototypes

// do we need to change pursuit modes?
static UUtBool AI2iPursuit_ChangeMode(ONtCharacter *ioCharacter, AI2tPursuitState *ioPursuitState);

// begin our new mode
static void AI2iPursuit_BeginMode(ONtCharacter *ioCharacter, AI2tPursuitState *ioPursuitState);

// do we want to pursue a target?
static UUtBool AI2iPursuit_DesirePursuit(ONtCharacter *ioCharacter, AI2tPursuitState *ioPursuitState,
										 UUtUns32 inCheckType);

// knowledge notification function
static void AI2iPursuit_NotifyKnowledge(ONtCharacter *ioCharacter, AI2tKnowledgeEntry *inEntry,
										UUtBool inDegrade);

// set the next state.
static void AI2iPursuit_NextMode(ONtCharacter *ioCharacter, AI2tPursuitState *ioPursuitState);

// individual pursuit mode update functions
static void AI2iPursuit_GoTo_Update(ONtCharacter *ioCharacter, AI2tPursuitState *ioPursuitState);
static void AI2iPursuit_Look_Update(ONtCharacter *ioCharacter, AI2tPursuitState *ioPursuitState);
static void AI2iPursuit_Glance_Update(ONtCharacter *ioCharacter, AI2tPursuitState *ioPursuitState);

// set up the scan state, if it isn't already
static void AI2iPursuit_SetupScanState(ONtCharacter *ioCharacter, AI2tPursuitState *ioPursuitState);

// ------------------------------------------------------------------------------------
// -- update and state change functions

// enter the pursuit state
void AI2rPursuit_Enter(ONtCharacter *ioCharacter)
{
	AI2tPursuitState *pursuit_state;

	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Pursuit);
	pursuit_state = &ioCharacter->ai2State.currentState->state.pursuit;

	// our target must be set up before entering pursuit state!
	UUmAssertReadPtr(pursuit_state->target, sizeof(AI2tKnowledgeEntry));
	pursuit_state->certain_of_enemy = (pursuit_state->target->highest_strength >= AI2cContactStrength_Definite);
	pursuit_state->pause_timer = 0;
	pursuit_state->delay_timer = 0;
	pursuit_state->last_los_check = 0;
	pursuit_state->been_close_to_contact = UUcFalse;

	// set up our knowledge notify function so that we can find stuff
	ioCharacter->ai2State.knowledgeState.callback = &AI2iPursuit_NotifyKnowledge;
	pursuit_state->pursue_danger = (ioCharacter->ai2State.alertStatus >= AI2cAlertStatus_Medium);

	// don't look about so quickly, since pursuit mode is more theatrical
	AI2rExecutor_SetAimingSpeed(ioCharacter, AI2cPursuit_AimingSpeed);

	// do we want to immediately go to the target or not?
	if ((pursuit_state->pursue_danger) && (AI2iPursuit_DesirePursuit(ioCharacter, pursuit_state, AI2cPursuitCheck_InitialContact))) {
		// run pursuit behavior
		pursuit_state->current_mode = AI2cPursuitMode_GoTo;
	} else {
		// look over in the target direction for a while, then consider whether we want to investigate further
		pursuit_state->current_mode = AI2cPursuitMode_Glance;
		pursuit_state->delay_timer = AI2rAlert_GetGlanceDuration(pursuit_state->target->last_type);
		if (!pursuit_state->pursue_danger) {
			// this is a lazy glance, don't register new visual contacts until we reach the desired direction
			ioCharacter->ai2State.knowledgeState.vision_delay_timer = pursuit_state->delay_timer;
		}
	}

	pursuit_state->say_timer = AI2cPursuit_InitialSayTimer;

	// set up our pursuit mode
	AI2iPursuit_ChangeMode(ioCharacter, pursuit_state);
	AI2iPursuit_BeginMode(ioCharacter, pursuit_state);
}

// exit a pursuit state - called when the character goes out of "pursuit mode" either
// because a combat target has been found or their contact strength has fallen below the level
// where they would continue pursuing.
void AI2rPursuit_Exit(ONtCharacter *ioCharacter)
{
	AI2tPursuitState *pursuit_state;

	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Pursuit);
	pursuit_state = &ioCharacter->ai2State.currentState->state.pursuit;

	// restore aiming speed to normal
	AI2rExecutor_SetAimingSpeed(ioCharacter, 1.0f);

	// stop any current movement
	pursuit_state->pause_timer = 0;
	pursuit_state->current_mode = AI2cPursuitMode_None;
	pursuit_state->delay_timer = 0;
	AI2rPath_Halt(ioCharacter);
	AI2rMovement_StopGlancing(ioCharacter);
	AI2rMovement_StopAiming(ioCharacter);
	AI2rMovement_FreeFacing(ioCharacter);
}

// pause a pursuit for some number of frames
void AI2rPursuit_Pause(ONtCharacter *ioCharacter, AI2tPursuitState *ioPursuitState,
					  UUtUns32 inPauseFrames)
{
	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Pursuit);
	ioPursuitState->pause_timer = UUmMax(ioPursuitState->pause_timer, inPauseFrames);
}

// is the character hurrying?
UUtBool AI2rPursuit_IsHurrying(ONtCharacter *ioCharacter, AI2tPursuitState *ioPursuitState)
{
	if ((ioPursuitState->current_mode == AI2cPursuitMode_GoTo) &&
		(ioPursuitState->running_to_contact)) {
		return UUcTrue;
	} else {
		return UUcFalse;
	}
}

void AI2rPursuit_Update(ONtCharacter *ioCharacter)
{
	AI2tPursuitState *pursuit_state;

	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Pursuit);
	UUmAssert(ioCharacter->ai2State.currentState->begun);
	pursuit_state = &ioCharacter->ai2State.currentState->state.pursuit;

	if (ONrCharacter_IsFallen(ioCharacter)) {
		// get up, don't lie on the ground
		AI2rExecutor_MoveOverride(ioCharacter, LIc_BitMask_Forward);
	}

	if (pursuit_state->pause_timer) {
		pursuit_state->pause_timer--;

		if (pursuit_state->pause_timer) {
			// pause for a while
			AI2rPath_Halt(ioCharacter);
			return;
		} else {
			// continue pursuing (from a stopped position)
			AI2iPursuit_BeginMode(ioCharacter, pursuit_state);
			if (ioCharacter->ai2State.currentGoal != AI2cGoal_Pursuit) {
				// we have dropped out of pursuit mode
				return;
			}
		}
	}

	if (pursuit_state->delay_timer) {
		pursuit_state->delay_timer--;

		if (pursuit_state->delay_timer == 0) {
			// set up the next state in the chain
			AI2iPursuit_NextMode(ioCharacter, pursuit_state);
		}
	}

	if (ioCharacter->ai2State.currentGoal != AI2cGoal_Pursuit) {
		// we have fallen out of the pursuit state.
		return;
	}

	switch(pursuit_state->current_mode) {
		case AI2cPursuitMode_Look:
		case AI2cPursuitMode_Hunt:
		case AI2cPursuitMode_Move:
		case AI2cPursuitMode_Wait:
			// in these modes we are actively pursuing and may say things
			if (pursuit_state->say_timer > 0) {
				pursuit_state->say_timer--;
				if (pursuit_state->say_timer == 0) {
					AI2rVocalize(ioCharacter, AI2cVocalization_Pursue, UUcFalse);
					pursuit_state->say_timer = UUmRandomRange(AI2cPursuit_NextSayTimer_Min,
						AI2cPursuit_NextSayTimer_Max - AI2cPursuit_NextSayTimer_Min);
				}
			}
		break;
	}

	switch(pursuit_state->current_mode) {
		case AI2cPursuitMode_GoTo:
			AI2iPursuit_GoTo_Update(ioCharacter, pursuit_state);
			break;

		case AI2cPursuitMode_Look:
			AI2iPursuit_Look_Update(ioCharacter, pursuit_state);
			break;

		case AI2cPursuitMode_Glance:
			AI2iPursuit_Glance_Update(ioCharacter, pursuit_state);
			break;

		case AI2cPursuitMode_Hunt:
		case AI2cPursuitMode_Move:
		case AI2cPursuitMode_Wait:
			// stand still
			break;

		default:
			UUmAssert(!"AI2rPursuit_Update: unknown pursuit mode!");
			break;
	}
}

// ------------------------------------------------------------------------------------
// -- pursuit mode switching functions

// do we need to change pursuit modes?
static UUtBool AI2iPursuit_ChangeMode(ONtCharacter *ioCharacter, AI2tPursuitState *ioPursuitState)
{
	AI2tPursuitMode new_mode;
	UUtBool force_change = UUcFalse;

	if (ioPursuitState->delay_timer > 0) {
		// we're staying in our current state, whatever it is, for a while
		return UUcFalse;
	}

	if (ioPursuitState->current_mode == AI2cPursuitMode_Forget) {
		// we _must_ forget - this is a command to return to our job
		force_change = UUcTrue;
		new_mode = ioPursuitState->current_mode;
#if DEBUG_VERBOSE_PURSUIT
		COrConsole_Printf("%s: cannot pursue, forced to forget", ioCharacter->player_name);
#endif
	} else {
#if DEBUG_VERBOSE_PURSUIT
		COrConsole_Printf("%s: checking change mode", ioCharacter->player_name);
#endif

		switch(ioPursuitState->target->strength) {
			case AI2cContactStrength_Definite:
				ioPursuitState->certain_of_enemy = UUcTrue;

			case AI2cContactStrength_Strong:
				new_mode = (ioCharacter->ai2State.alertStatus >= AI2cAlertStatus_Medium)
							? ioCharacter->ai2State.pursuitSettings.strong_seen
							: ioCharacter->ai2State.pursuitSettings.strong_unseen;
			break;

			case AI2cContactStrength_Weak:
				new_mode = (ioCharacter->ai2State.alertStatus >= AI2cAlertStatus_Medium)
							? ioCharacter->ai2State.pursuitSettings.weak_seen
							: ioCharacter->ai2State.pursuitSettings.weak_unseen;
			break;

			case AI2cContactStrength_Forgotten:
				if (ioCharacter->ai2State.pursuitSettings.pursue_lost == AI2cPursuitLost_KeepLooking) {
					// no change in mode
					new_mode = AI2cPursuitMode_None;
				} else {
					// forget about the target
					new_mode = AI2cPursuitMode_Forget;
					force_change = UUcTrue;
				}
			break;

			case AI2cContactStrength_Dead:
				// the target is dead. go check out their body, then return to our job.
#if DEBUG_VERBOSE_PURSUIT
				COrConsole_Printf("%s: target dead, checking body", ioCharacter->player_name);
#endif
				new_mode = AI2cPursuitMode_GoTo;
				force_change = UUcTrue;
				ioPursuitState->delay_timer = ioCharacter->characterClass->ai2_behavior.combat_parameters.investigate_body_delay;
				return UUcTrue;
			break;

			default:
				UUmAssert(!"AI2iPursuit_ChangeMode: unknown contact strength");
			break;
		}

		if (new_mode == AI2cPursuitMode_None) {
			// no change
#if DEBUG_VERBOSE_PURSUIT
			COrConsole_Printf("%s: no new mode", ioCharacter->player_name);
#endif
			return UUcFalse;
		}
	}

	UUmAssert((new_mode > AI2cPursuitMode_None) && (new_mode < AI2cPursuitMode_Max));
#if DEBUG_VERBOSE_PURSUIT
	COrConsole_Printf("%s: new mode is %s%s", ioCharacter->player_name,
						AI2cPursuitModeName[new_mode], force_change ? " (forced)" : "");
#endif

	if ((new_mode != AI2cPursuitMode_Forget) && (new_mode == ioPursuitState->current_mode)) {
		// we're already in this mode
#if DEBUG_VERBOSE_PURSUIT
		COrConsole_Printf("%s: already in this mode", ioCharacter->player_name);
#endif
		return UUcFalse;
	}

	if ((ioPursuitState->current_mode == AI2cPursuitMode_GoTo) && (!force_change)) {
		// don't interrupt a GoTo mode
#if DEBUG_VERBOSE_PURSUIT
		COrConsole_Printf("%s: still going to point, don't switch modes", ioCharacter->player_name);
#endif
		return UUcFalse;
	} else {
		// change to the new pursuit mode
#if DEBUG_VERBOSE_PURSUIT
		COrConsole_Printf("%s: changing to this mode", ioCharacter->player_name);
#endif
		ioPursuitState->current_mode = new_mode;
		return UUcTrue;
	}
}

// begin our new mode
static void AI2iPursuit_BeginMode(ONtCharacter *ioCharacter, AI2tPursuitState *ioPursuitState)
{
	AI2tPursuitLostBehavior lost_behavior;
	M3tPoint3D current_pt;
	M3tPoint3D vector_to_contact;
	float angle_to_contact;
	ONtActionMarker *found_alarm;

	UUmAssert((ioPursuitState->current_mode > AI2cPursuitMode_None) && (ioPursuitState->current_mode < AI2cPursuitMode_Max));

	if ((ioPursuitState->pause_timer > 0) && (ioPursuitState->current_mode != AI2cPursuitMode_Forget)) {
		// don't start yet
		return;
	}

#if DEBUG_VERBOSE_PURSUIT
	COrConsole_Printf("%s: starting pursuit mode '%s'",
			ioCharacter->player_name, AI2cPursuitModeName[ioPursuitState->current_mode]);
#endif

	// clear previous movement settings
	AI2rPath_Halt(ioCharacter);
	AI2rMovement_StopAiming(ioCharacter);

	switch(ioPursuitState->current_mode) {
		case AI2cPursuitMode_Forget:
			lost_behavior = ioCharacter->ai2State.pursuitSettings.pursue_lost;
			switch (lost_behavior) {
				case AI2cPursuitLost_Alarm:
					if (ioPursuitState->certain_of_enemy) {
						found_alarm = ioCharacter->ai2State.alarmStatus.action_marker;
						if (found_alarm == NULL) {
							found_alarm = AI2rAlarm_FindAlarmConsole(ioCharacter);
						}

						if (found_alarm != NULL) {
							AI2rAlarm_Setup(ioCharacter, found_alarm);
							return;
						}
					}

					// not certain there was an enemy, or no alarm console available; just return to job

				case AI2cPursuitLost_KeepLooking:
					// mostly handled in ChangeMode. if we ever get here, it means that
					// we cannot pursue the target any further, and we should run our return-to-job behavior

				case AI2cPursuitLost_Return:
					AI2rReturnToJob(ioCharacter, UUcTrue, UUcTrue);
					return;
				break;

				default:
					UUmAssert(!"AI2iPursuit_BeginMode: unknown pursuit-lost behavior");
				break;
			}
			break;

		case AI2cPursuitMode_GoTo:
			// set up state
			ioPursuitState->last_los_check = 0;
			ioPursuitState->los_successful_checks = 0;
			ioPursuitState->running_to_contact = ioPursuitState->pursue_danger;
			ioPursuitState->scanning_area = UUcFalse;

			// pathfind to the last known location of the target
			AI2rPath_GoToPoint(ioCharacter, &ioPursuitState->target->last_location, AI2cPursuit_GoToContact_CloseDist,
								UUcTrue, AI2cAngle_None, ioPursuitState->target->enemy);
			break;

		case AI2cPursuitMode_Look:
			// set up the scanning state for us standing still
			AI2iPursuit_SetupScanState(ioCharacter, ioPursuitState);

			ioPursuitState->scan_state.stay_facing = UUcFalse;
			ioPursuitState->scan_state.direction_minframes = AI2cPursuit_Look_DirMinFrames;
			ioPursuitState->scan_state.direction_deltaframes = AI2cPursuit_Look_DirMaxFrames - AI2cPursuit_Look_DirMinFrames;
			ioPursuitState->scan_state.angle_minframes = AI2cPursuit_Look_AngleMinFrames;
			ioPursuitState->scan_state.angle_deltaframes = AI2cPursuit_Look_AngleMaxFrames - AI2cPursuit_Look_AngleMinFrames;
			ioPursuitState->scan_state.angle_range = AI2cPursuit_Look_AngleRange;
			break;

		case AI2cPursuitMode_Glance:
			// set up the scanning state for us looking in the direction of the contact
			ONrCharacter_GetPathfindingLocation(ioCharacter, &current_pt);
			MUmVector_Subtract(vector_to_contact, ioPursuitState->target->last_location, current_pt);
			angle_to_contact = MUrATan2(vector_to_contact.x, vector_to_contact.z);

			ioPursuitState->scanning_area = UUcFalse;
			AI2rGuard_ResetScanState(&ioPursuitState->scan_state, angle_to_contact);
			ioPursuitState->scan_state.direction_minframes = 0;
			ioPursuitState->scan_state.direction_deltaframes = 0;
			ioPursuitState->scan_state.angle_minframes = AI2cPursuit_Glance_AngleMinFrames;
			ioPursuitState->scan_state.angle_deltaframes = AI2cPursuit_Glance_AngleMaxFrames - AI2cPursuit_Glance_AngleMinFrames;
			ioPursuitState->scan_state.angle_range = AI2cPursuit_Glance_AngleRange;

			// face in the direction of the contact
			ioPursuitState->scan_state.direction_timer = 10000;
			ioPursuitState->scan_state.dir_state.has_cur_direction = UUcTrue;
			ioPursuitState->scan_state.dir_state.cur_direction_isexit = UUcFalse;
			ioPursuitState->scan_state.dir_state.cur_direction_angle = angle_to_contact;
			MUmVector_Copy(ioPursuitState->scan_state.dir_state.cur_direction, vector_to_contact);
			MUmVector_Copy(ioPursuitState->scan_state.facing_vector, vector_to_contact);

			// look exactly in this direction for a while, then start scanning as normal
			ioPursuitState->scan_state.angle_timer = AI2cPursuit_Glance_InitialFrames;
			ioPursuitState->scan_state.cur_angle = 0;
			AI2rMovement_LookInDirection(ioCharacter, &vector_to_contact);
			break;

		case AI2cPursuitMode_Hunt:
		case AI2cPursuitMode_Move:
			COrConsole_Printf("### pursuit mode '%s' not yet implemented", AI2cPursuitModeName[ioPursuitState->current_mode]);
			// fall through to waiting

		case AI2cPursuitMode_Wait:
			// stand still
			break;

		default:
			UUmAssert(!"AI2iPursuit_BeginMode: unknown pursuit mode!");
			break;
	}
}

// set the next mode that our pursuit pattern will go through.
static void AI2iPursuit_NextMode(ONtCharacter *ioCharacter, AI2tPursuitState *ioPursuitState)
{
	AI2tPursuitMode prev_mode;

	prev_mode = ioPursuitState->current_mode;

	switch(ioPursuitState->current_mode) {
		case AI2cPursuitMode_GoTo:
			if ((ioPursuitState->target->priority == AI2cContactPriority_Friendly) && (ioPursuitState->target->strength == AI2cContactStrength_Definite)) {
				// we can see the friendly character, just look at them
				ioPursuitState->current_mode = AI2cPursuitMode_Glance;
				ioPursuitState->delay_timer = AI2cPursuit_Glance_Friendly;
			} else {
				// we have reached the contact area. look around for a little while.
				ioPursuitState->current_mode = AI2cPursuitMode_Look;
				ioPursuitState->delay_timer = AI2cPursuit_Delay_ReachAreaLookFrames;
			}
		break;

		case AI2cPursuitMode_Glance:
			// we've finished glancing in the direction of a (non-danger) contact, do we want to pursue?
			if (AI2iPursuit_DesirePursuit(ioCharacter, ioPursuitState, AI2cPursuitCheck_PostGlance)) {
#if DEBUG_VERBOSE_PURSUIT
				COrConsole_Printf("%s: finished glancing, pursuing", ioCharacter->player_name);
#endif
				ioPursuitState->current_mode = AI2cPursuitMode_GoTo;
			} else {
#if DEBUG_VERBOSE_PURSUIT
				COrConsole_Printf("%s: finished glancing, forgetting", ioCharacter->player_name);
#endif
				ioPursuitState->current_mode = AI2cPursuitMode_Forget;
			}
		break;

		case AI2cPursuitMode_Look:
			// we've finished looking around upon reaching the contact area.
			if (!AI2iPursuit_DesirePursuit(ioCharacter, ioPursuitState, AI2cPursuitCheck_PostArrival)) {
#if DEBUG_VERBOSE_PURSUIT
				COrConsole_Printf("%s: finished looking around, forgetting", ioCharacter->player_name);
#endif
				ioPursuitState->current_mode = AI2cPursuitMode_Forget;
			} else {
#if DEBUG_VERBOSE_PURSUIT
				COrConsole_Printf("%s: finished looking around, continuing with next pursuit mode", ioCharacter->player_name);
#endif
			}

			// run the rest of our behavior pattern.
			ioPursuitState->delay_timer = 0;
			AI2iPursuit_ChangeMode(ioCharacter, ioPursuitState);
		break;

		default:
			// unimplemented node in the pursuit pattern; just default to standing
			// still
			ioPursuitState->delay_timer = 0;
			ioPursuitState->current_mode = AI2cPursuitMode_Wait;
		break;
	}

	if (ioPursuitState->current_mode != prev_mode) {
		// start the new mode
		AI2iPursuit_BeginMode(ioCharacter, ioPursuitState);
	}
}

// ------------------------------------------------------------------------------------
// -- go to contact routines

static void AI2iPursuit_GoTo_Update(ONtCharacter *ioCharacter, AI2tPursuitState *ioPursuitState)
{
	M3tPoint3D look_point, look_dir;
	UUtUns32 current_time;
	UUtBool finished_goto, have_los, do_scan;

	// while we are going to the location of a contact, our knowledge of it does not
	// degrade!
	if (ioPursuitState->target->degrade_time != 0) {
		ioPursuitState->target->degrade_time++;
	}

	// have we reached a point where we should be able to see the source of the contact?
	finished_goto = UUcFalse;
	if ((ioCharacter->pathState.at_destination) ||
		(ioCharacter->pathState.distance_to_destination_squared < UUmSQR(AI2cPursuit_GoToContact_CloseDist))) {
		// we are very close to the contact.
		finished_goto = UUcTrue;

	} else if (ioCharacter->pathState.distance_to_destination_squared > UUmSQR(AI2cPursuit_GoToContact_ScanDist)) {
		// we are a long way from the contact.
		ioPursuitState->running_to_contact = ioPursuitState->pursue_danger;
		do_scan = UUcFalse;

	} else {
		// we aren't close enough that we will definitely transition to the next state, and we are close
		// enough that we might want to start scanning the area.

		current_time = ONrGameState_GetGameTime();
		// don't perform LOS checks every frame!
		have_los = UUcFalse;
		if (current_time >= ioPursuitState->last_los_check + AI2cPursuit_GoToContact_LOSFrames) {
			// can we see the point that we are heading to?
			ONrCharacter_GetEyePosition(ioCharacter, &look_point);
			MUmVector_Subtract(look_dir, ioPursuitState->target->last_location, look_point);

			have_los = (!AKrCollision_Point(ONrGameState_GetEnvironment(), &look_point, &look_dir,
											AKcGQ_Flag_LOS_CanSee_Skip_AI, 0));
		}

		if (have_los) {
			do_scan = UUcTrue;

			if (ioCharacter->pathState.distance_to_destination_squared < UUmSQR(AI2cPursuit_GoToContact_StopDist)) {
				// we can see the contact, and we're close enough that we could stop here.
				ioPursuitState->los_successful_checks++;
				if (ioPursuitState->los_successful_checks >= AI2cPursuit_RequiredLOSChecks) {
					finished_goto = UUcTrue;
				}
			} else {
				// we can see the contact - proceed towards it, scanning. we run towards it if we
				// have been alerted by danger and haven't yet been close to it.
				ioPursuitState->running_to_contact = (!ioPursuitState->been_close_to_contact) &&
													(ioPursuitState->pursue_danger);
			}
		} else {
			ioPursuitState->los_successful_checks = 0;
			do_scan = UUcFalse;
		}
	}

	if (finished_goto) {
		// we've reached the location of the contact. work out what we should do next
		ioPursuitState->been_close_to_contact = UUcTrue;
		ioPursuitState->running_to_contact = UUcFalse;
		ioPursuitState->scanning_area = UUcFalse;
		AI2iPursuit_NextMode(ioCharacter, ioPursuitState);
		if (ioCharacter->ai2State.currentGoal != AI2cGoal_Pursuit) {
			// we have dropped out of pursuit mode
			return;
		}
	}

	if (do_scan) {
		if (!ioPursuitState->scanning_area) {
			// set up the scanning state
			AI2iPursuit_SetupScanState(ioCharacter, ioPursuitState);

			ioPursuitState->scan_state.stay_facing = UUcTrue;
			ioPursuitState->scan_state.stay_facing_direction = ioCharacter->facing;
			ioPursuitState->scan_state.stay_facing_range = AI2cPursuit_GoToContact_ScanAngleRange;	// use character's facing
			ioPursuitState->scan_state.direction_timer = 0;
			ioPursuitState->scan_state.direction_minframes = AI2cPursuit_GoToContact_ScanMinFrames;
			ioPursuitState->scan_state.direction_deltaframes = AI2cPursuit_GoToContact_ScanMaxFrames - AI2cPursuit_GoToContact_ScanMinFrames;

			// no secondary 'angle' motion
			ioPursuitState->scan_state.angle_range = 0;
			ioPursuitState->scan_state.angle_timer = 0;
		}

		// scan the area as we move
		AI2rGuard_UpdateScan(ioCharacter, &ioPursuitState->scan_state);
	} else {
		// stop scanning
		ioPursuitState->scanning_area = UUcFalse;
		AI2rMovement_StopAiming(ioCharacter);
	}
}

// ------------------------------------------------------------------------------------
// -- scanning routines

static void AI2iPursuit_SetupScanState(ONtCharacter *ioCharacter, AI2tPursuitState *ioPursuitState)
{
	float angle_to_contact;

	if (ioPursuitState->scanning_area)
		return;

	angle_to_contact = MUrATan2(ioPursuitState->target->last_location.x - ioCharacter->location.x,
								ioPursuitState->target->last_location.z - ioCharacter->location.z);
	AI2rGuard_ResetScanState(&ioPursuitState->scan_state, angle_to_contact);
	ioPursuitState->scanning_area = UUcTrue;
}

static void AI2iPursuit_Look_Update(ONtCharacter *ioCharacter, AI2tPursuitState *ioPursuitState)
{
	AI2rGuard_UpdateScan(ioCharacter, &ioPursuitState->scan_state);
}

static void AI2iPursuit_Glance_Update(ONtCharacter *ioCharacter, AI2tPursuitState *ioPursuitState)
{
	AI2rGuard_UpdateScan(ioCharacter, &ioPursuitState->scan_state);
}

// ------------------------------------------------------------------------------------
// -- knowledge and detection

// do we want to pursue a target?
static UUtBool AI2iPursuit_DesirePursuit(ONtCharacter *ioCharacter, AI2tPursuitState *ioPursuitState,
										 UUtUns32 inCheckType)
{
	M3tPoint3D current_pt;
	UUtError error;
	float path_distance;
	UUtBool pursue;

	if ((inCheckType != AI2cPursuitCheck_InitialContact) &&
		(ioPursuitState->target->priority == AI2cContactPriority_Friendly)) {
		// don't pursue, unless we're running to a combat sound
#if DEBUG_VERBOSE_PURSUIT
		COrConsole_Printf("%s: not a threat, don't pursue", ioCharacter->player_name);
#endif
		return UUcFalse;
	}

	if (ioCharacter->ai2State.alertStatus < ioCharacter->ai2State.investigateAlert) {
		// not yet alerted enough to pursue
#if DEBUG_VERBOSE_PURSUIT
		COrConsole_Printf("%s: not alerted enough to pursue - curalert %s, pursalert %s",
					ioCharacter->player_name, AI2cAlertStatusName[ioCharacter->ai2State.alertStatus],
					AI2cAlertStatusName[ioCharacter->ai2State.investigateAlert]);
#endif
		return UUcFalse;
	}

	if (ioCharacter->ai2State.combatSettings.pursuit_distance == 0) {
		// always interested in pursuing
#if DEBUG_VERBOSE_PURSUIT
		COrConsole_Printf("%s: we pursue everything, no decision necessary", ioCharacter->player_name);
#endif
		return UUcTrue;
	}

	ONrCharacter_GetPathfindingLocation(ioCharacter, &current_pt);
	if ((inCheckType == AI2cPursuitCheck_PostArrival) || (inCheckType == AI2cPursuitCheck_ChangeTarget)) {
		// we will only pursue up to a certain distance away from where our job was
		path_distance = 0;
		error = UUcError_None;
		if (ioCharacter->ai2State.has_job_location) {
			error = PHrPathDistance(ioCharacter, (UUtUns32) -1, &current_pt, &ioCharacter->ai2State.job_location,
									ioCharacter->currentPathnode, NULL, &path_distance);

			// pursue if no path back, or path is short enough that we're happy
			pursue = ((error != UUcError_None) || (path_distance < ioCharacter->ai2State.combatSettings.pursuit_distance));
		}
	} else {
		// we will only pursue a certain distance away from our current location
		path_distance = ioCharacter->ai2State.combatSettings.pursuit_distance;
		error = PHrPathDistance(ioCharacter, (UUtUns32) -1, &current_pt, &ioPursuitState->target->last_location,
								ioCharacter->currentPathnode, NULL, &path_distance);

		// pursue if we found a path within the desired distance
		pursue = (error == UUcError_None);
		UUmAssert(!pursue || (path_distance < ioCharacter->ai2State.combatSettings.pursuit_distance));
	}

	if (pursue) {
		// don't pursue the target.
#if DEBUG_VERBOSE_PURSUIT
		COrConsole_Printf("%s: decided to pursue [%s] (dist %f, pursuedist %f)",
			ioCharacter->player_name, AI2cPursuitCheckType[inCheckType],
			path_distance, ioCharacter->ai2State.combatSettings.pursuit_distance);
#endif
	} else {
#if DEBUG_VERBOSE_PURSUIT
		COrConsole_Printf("%s: decided not to pursue [%s] (path %s, dist %f, pursuedist %f)",
			ioCharacter->player_name, AI2cPursuitCheckType[inCheckType], (error == UUcError_None) ? "yes" : "no",
			(error == UUcError_None) ? path_distance : 0, ioCharacter->ai2State.combatSettings.pursuit_distance);
#endif
	}

	return pursue;
}

// knowledge notification function
static void AI2iPursuit_NotifyKnowledge(ONtCharacter *ioCharacter, AI2tKnowledgeEntry *inEntry,
										UUtBool inDegrade)
{
	AI2tKnowledgeEntry *best_contact;
	AI2tPursuitState *pursuit_state;
	UUtBool investigate_entry = UUcFalse;

	// this knowledge function can only execute on an AI character that is in a pursuit state
	if ((ioCharacter->charType != ONcChar_AI2) || (ioCharacter->ai2State.currentGoal != AI2cGoal_Pursuit)) {
		return;
	}

	pursuit_state = &ioCharacter->ai2State.currentState->state.pursuit;

	// first: check to see whether our alert status changes
	AI2rAlert_NotifyKnowledge(ioCharacter, inEntry, inDegrade);

	if (ioCharacter->ai2State.currentGoal != AI2cGoal_Pursuit) {
		// we have seen an enemy; drop out of pursuit mode
		return;
	}

	pursuit_state->pursue_danger = (ioCharacter->ai2State.alertStatus >= AI2cAlertStatus_Medium);

	if (inDegrade && (inEntry == pursuit_state->target)) {
#if DEBUG_VERBOSE_PURSUIT
		COrConsole_Printf("%s: target knowledge degraded", ioCharacter->player_name);
#endif

		if ((ioCharacter->ai2State.alarmStatus.action_marker != NULL) &&
			(inEntry->strength <= AI2cContactStrength_Weak)) {
			// we were previously running for an alarm, and we just lost immediate contact
			// with this target; continue running for the alarm
			AI2rAlarm_Setup(ioCharacter, NULL);
			return;
		}

		// check to see if we need to change pursuit modes
		if (AI2iPursuit_ChangeMode(ioCharacter, pursuit_state)) {
			AI2iPursuit_BeginMode(ioCharacter, pursuit_state);
			if (ioCharacter->ai2State.currentGoal != AI2cGoal_Pursuit) {
				// we have dropped out of pursuit mode
				return;
			}
		}
	}

	best_contact = ioCharacter->ai2State.knowledgeState.contacts;
	if (best_contact == NULL) {
		// this should never happen, but if it does then investigate the target, since we
		// have no current entries
		UUmAssert(ioCharacter->ai2State.knowledgeState.contacts != NULL);
		investigate_entry = UUcTrue;

	} else if (best_contact == pursuit_state->target) {
		if (!inDegrade) {
#if DEBUG_VERBOSE_PURSUIT
			COrConsole_Printf("%s: new information about target", ioCharacter->player_name);
#endif
			investigate_entry = UUcTrue;
		}

	} else if (best_contact->priority >= AI2cContactPriority_Hostile_NoThreat) {
#if DEBUG_VERBOSE_PURSUIT
		COrConsole_Printf("%s: new best contact '%s' is a threat",
			ioCharacter->player_name, (best_contact->enemy == NULL) ? "NULL" : best_contact->enemy->player_name);
#endif
		// we have a new target to follow!
		investigate_entry = UUcTrue;
		pursuit_state->target = best_contact;
	}

	if (!investigate_entry) {
#if DEBUG_VERBOSE_PURSUIT
		COrConsole_Printf("%s: not interested in investigating contact", ioCharacter->player_name);
#endif
		return;
	}

	if (!AI2iPursuit_DesirePursuit(ioCharacter, pursuit_state, AI2cPursuitCheck_ChangeTarget)) {
		// too far to go, don't pursue this new contact
#if DEBUG_VERBOSE_PURSUIT
		COrConsole_Printf("%s: too far to go to investigate new location, continuing behavior",
							ioCharacter->player_name);
#endif
		return;
	}

#if DEBUG_VERBOSE_PURSUIT
	COrConsole_Printf("%s: investigating new location", ioCharacter->player_name);
#endif
	pursuit_state->current_mode = AI2cPursuitMode_GoTo;
	AI2iPursuit_BeginMode(ioCharacter, pursuit_state);
}

// an entry in our knowledge base has just been removed, forcibly delete any references we had to it
UUtBool AI2rPursuit_NotifyKnowledgeRemoved(ONtCharacter *ioCharacter, AI2tPursuitState *ioPursuitState,
										   AI2tKnowledgeEntry *inEntry)
{
	AI2tKnowledgeEntry *new_entry;

	if (ioPursuitState->target != inEntry) {
		return UUcFalse;
	}

	// the knowledge entry that we were pursuing was just deleted.
#if DEBUG_VERBOSE_PURSUIT
	COrConsole_Printf("%s: knowledge entry that we were pursuing (%s) was removed",
						ioCharacter->player_name, (inEntry->enemy == NULL) ? "none" : inEntry->enemy->player_name);
#endif
	if ((ioCharacter->ai2State.alarmStatus.action_marker != NULL) &&
		(inEntry->strength <= AI2cContactStrength_Weak)) {
		// we were previously running for an alarm, and we just lost immediate contact
		// with this target; continue running for the alarm
#if DEBUG_VERBOSE_PURSUIT
		COrConsole_Printf("%s: drop out of pursuit to continue alarm behavior", ioCharacter->player_name);
#endif
		AI2rAlarm_Setup(ioCharacter, NULL);
		return UUcTrue;
	}

	// look for new knowledge entries to pursue
	for (new_entry = ioCharacter->ai2State.knowledgeState.contacts; new_entry != NULL; new_entry = new_entry->nextptr) {
		if (new_entry == inEntry)
			continue;

		ioPursuitState->target = new_entry;
		if (AI2iPursuit_DesirePursuit(ioCharacter, ioPursuitState, AI2cPursuitCheck_ChangeTarget)) {
			// switch targets to this entry
			break;
		}
	}

	if (new_entry == NULL) {
		// give up and return to our job
#if DEBUG_VERBOSE_PURSUIT
		COrConsole_Printf("%s: couldn't find new entry to pursue, returning to job", ioCharacter->player_name);
#endif
		ioPursuitState->current_mode = AI2cPursuitMode_Forget;
		AI2iPursuit_BeginMode(ioCharacter, ioPursuitState);
		return UUcTrue;
	} else {
		// investigate the entry that we found
#if DEBUG_VERBOSE_PURSUIT
		COrConsole_Printf("%s: found new entry to pursue", ioCharacter->player_name);
#endif
		ioPursuitState->current_mode = AI2cPursuitMode_GoTo;
		AI2iPursuit_BeginMode(ioCharacter, ioPursuitState);
		return UUcTrue;
	}
}

// ------------------------------------------------------------------------------------
// -- debugging functions

// report in
void AI2rPursuit_Report(ONtCharacter *ioCharacter, AI2tPursuitState *ioPursuitState,
					   UUtBool inVerbose, AI2tReportFunction inFunction)
{
	char reportbuf[256], tempbuf[64];

	sprintf(reportbuf, "  strength %s (max %s)",
		AI2cContactStrengthName[ioPursuitState->target->strength],
		AI2cContactStrengthName[ioPursuitState->target->highest_strength]);

	if (ioPursuitState->pause_timer > 0) {
		sprintf(tempbuf, ", paused: %d frames", ioPursuitState->pause_timer);
		strcat(reportbuf, tempbuf);
	}

	if ((ioPursuitState->current_mode < 0) || (ioPursuitState->current_mode >= AI2cPursuitMode_Max)) {
		sprintf(tempbuf, ", <unknown mode %d>", ioPursuitState->current_mode);
	} else {
		sprintf(tempbuf, ", mode=%s", AI2cPursuitModeName[ioPursuitState->current_mode]);
	}
	strcat(reportbuf, tempbuf);

	if (ioPursuitState->delay_timer > 0) {
		sprintf(tempbuf, ", timer: %d frames", ioPursuitState->delay_timer);
		strcat(reportbuf, tempbuf);
	}

	inFunction(reportbuf);

	if (inVerbose) {
		switch(ioPursuitState->current_mode) {
			case AI2cPursuitMode_GoTo:
				sprintf(reportbuf, "  going to location %f %f %f",
					ioPursuitState->target->last_location.x, ioPursuitState->target->last_location.y, ioPursuitState->target->last_location.z);
				inFunction(reportbuf);
				break;

			case AI2cPursuitMode_Glance:
				// FIXME

			case AI2cPursuitMode_Hunt:
			case AI2cPursuitMode_Move:
			case AI2cPursuitMode_Look:
			case AI2cPursuitMode_Wait:
			default:
				break;
		}
	}
}

