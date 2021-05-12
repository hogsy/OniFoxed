#pragma once
#ifndef ONI_AI2_Melee_H
#define ONI_AI2_Melee_H

/*
	FILE:	Oni_AI2_Melee.h
	
	AUTHOR:	Chris Butcher
	
	CREATED: May 19, 2000
	
	PURPOSE: Melee AI for Oni
	
	Copyright (c) 2000

*/

#include "Oni_Character_Animation.h"
#include "Oni_AI2_MeleeProfile.h"
#include "Oni_AI2_Fight.h"

// ------------------------------------------------------------------------------------
// -- constants

// state that a character can be in as far as the melee code is concerned
typedef enum {
	AI2cMeleeMoveState_None				= 0,		// any state that we can't execute moves from
	AI2cMeleeMoveState_Standing			= 1,
	AI2cMeleeMoveState_Crouching		= 2,
	AI2cMeleeMoveState_CrouchStart		= 3,
	AI2cMeleeMoveState_Prone			= 4,
	AI2cMeleeMoveState_RunStart			= 5,
	AI2cMeleeMoveState_RunBackStart		= 6,
	AI2cMeleeMoveState_RunLeftStart		= 7,
	AI2cMeleeMoveState_RunRightStart	= 8,
	AI2cMeleeMoveState_Running			= 9,
	AI2cMeleeMoveState_RunningBack		= 10,
	AI2cMeleeMoveState_RunningLeft		= 11,
	AI2cMeleeMoveState_RunningRight		= 12,
	AI2cMeleeMoveState_Jumping			= 13,
	AI2cMeleeMoveState_JumpingForwards	= 14,
	AI2cMeleeMoveState_JumpingBack		= 15,
	AI2cMeleeMoveState_JumpingLeft		= 16,
	AI2cMeleeMoveState_JumpingRight		= 17,

	AI2cMeleeMoveState_Max				= 18
} AI2tMeleeMoveState;

// used in ONrSetupAnimIntersectionContext when calculating target's bounding cylinder for attack checking
#define AI2cMelee_TargetCylinderSafetyScale		0.75

#define AI2cMelee_MinRange					8.0f
#define AI2cMelee_MaxRange					80.0f


// ------------------------------------------------------------------------------------
// -- structure definitions

// *** a character's melee state */

#define AI2cMeleeMaxTechniques				32
#define AI2cMeleeMaxDelayTimers				32

typedef struct AI2tMeleeState
{
	ONtCharacter		*target;
	ONtCharacter		*last_target;

	UUtUns32			action_index;
	AI2tMeleeTechnique	*current_technique;		// may be NULL
	UUtBool				current_technique_ends_in_jump;
	float				current_technique_jump_vel;
	UUtBool				lock_facing_for_technique;
	UUtBool				committed_to_technique;
	float				weight_threshold;		// amount that weight of current technique must decrease to
												// in order to make us choose an alternative
	AI2tFightInfo		fight_info;				// data stored for fight manager


	UUtUns32			move_index;
	AI2tMeleeMove		*current_move;
	TRtAnimation		*current_attackanim;	// expected attack animation from this move

	// attack move state
	UUtBool				attack_launched;
	UUtBool				attack_waspositioned;
	UUtBool				running_active_animation;	// can't change moves or techniques while running

	// positioning move state
	UUtUns16			positionmove_skip;
	UUtUns32			position_state_index;
	AI2tMeleeMoveState	position_desired_state;
	AI2tMeleeTransition	*position_current_transition;
	LItButtonBits		position_lastkeys;
	UUtUns32			position_notfacing_timer;
	UUtBool				position_skipped_delay;
	UUtBool				position_jumpkey_down;
	UUtBool				position_force_attack;
	UUtUns32			position_missbyheight_timer;

	// maneuver move state
	UUtBool				maneuver_started;
	UUtUns32			maneuver_endtime;

	// evasion move state
	UUtBool				evasion_launched;
	UUtUns32			evasion_endtime;

	// facing control
	UUtBool				face_target;
	float				distance_to_target;
	UUtUns32			last_targetanim_considered;
	float				angle_to_target;
	M3tVector3D			vector_to_target;
	float				technique_face_direction;
	float				facing_delta;

	// random seeds
	UUtUns16			action_seed;
	UUtUns16			reaction_seed;

	// weight adjustment values for action techniques... zero = normal, one = totally disabled
	float				weight_adjust[AI2cMeleeMaxTechniques];

	// reacting to enemy animations
	UUtUns32			frames_to_hit;
	UUtUns32			considered_react_anim_index;
	UUtBool				react_enable;
	UUtBool				evade_enable;

	// blocking
	UUtBool				currently_blocking;
	UUtUns32			current_react_anim_index;
	LItButtonBits		block_keys;

	// local environment awareness
	UUtBool				localpath_blocked;
	UUtBool				danger_ahead;
	UUtBool				danger_behind;
	float				danger_ahead_dist;
	float				danger_behind_dist;
	// TEMPORARY DEBUGGING
	UUtUns32			cast_frame;
	M3tVector3D			cast_pt;
	M3tVector3D			cast_ahead_end;
	M3tVector3D			cast_behind_end;

} AI2tMeleeState;

// *** melee move type definitions */

typedef UUtUns32 (*AI2tMoveTypeFunction_Iterate)(UUtUns32 inIterator);

typedef UUtBool (*AI2tMoveTypeFunction_GetName)(UUtUns32 inMove, char *outString, UUtUns32 inStringLength);

typedef UUtUns32 (*AI2tMoveTypeFunction_GetParams)(UUtUns32 inMove, UUtBool inSpacing, char *outStringPtrs[3]);

typedef UUtUns16 (*AI2tMoveTypeFunction_GetAnimType)(UUtUns32 inMove, TRtDirection *outDirection,
											AI2tMeleeMoveState *outFromState, AI2tMeleeMoveState *outToState,
											UUtUns16 *outTargetAnimState, UUtUns16 *outTargetAnimVarient);

typedef UUtBool (*AI2tMoveTypeFunction_CanFollow)(UUtUns32 inMove, UUtUns32 inPrevMove);

typedef UUtBool (*AI2tMoveTypeFunction_Start)(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
											  AI2tMeleeMove *inMove);

typedef UUtBool (*AI2tMoveTypeFunction_Update)(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
											  AI2tMeleeMove *inMove);

typedef void (*AI2tMoveTypeFunction_Finish)(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
											  AI2tMeleeMove *inMove);

typedef UUtBool (*AI2tMoveTypeFunction_FaceTarget)(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
											  AI2tMeleeMove *inMove);

typedef UUtBool (*AI2tMoveTypeFunction_CanEvade)(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
											  AI2tMeleeMove *inMove);

typedef struct AI2tMeleeMoveType
{
	char								name[32];
	AI2tMoveTypeFunction_Iterate		func_iterate;
	AI2tMoveTypeFunction_GetName		func_getname;
	AI2tMoveTypeFunction_GetParams		func_getparams;
	AI2tMoveTypeFunction_GetAnimType	func_getanimtype;
	AI2tMoveTypeFunction_CanFollow		func_canfollow;
	AI2tMoveTypeFunction_Start			func_start;
	AI2tMoveTypeFunction_Update			func_update;
	AI2tMoveTypeFunction_Finish			func_finish;
	AI2tMoveTypeFunction_FaceTarget		func_facetarget;
	AI2tMoveTypeFunction_CanEvade		func_canevade;

} AI2tMeleeMoveType;

// *** melee transition definitions */

// note that a transition has a number of 'to' states - this indicates whereabouts we expect to
// go in the state graph if we hold down the designated keys.
typedef struct AI2tMeleeTransitionDefinition
{
	TRtDirection		direction;
	AI2tMeleeMoveState	from_state;

	UUtBool				essential;
	UUtUns32			num_to_states;
	AI2tMeleeMoveState	to_state[AI2cMeleeProfile_TransitionMaxToStates];

	UUtUns16			anim_type;
	LItButtonBits		move_keys;
	float				weight;

} AI2tMeleeTransitionDefinition;

// ------------------------------------------------------------------------------------
// -- globals

extern const UUtUns32 AI2cMeleeNumMoveTypes;
extern const AI2tMeleeMoveType AI2cMeleeMoveTypes[];
extern const char *AI2cMeleeMoveStateName[];
extern const TRtAnimState AI2cMelee_TotoroAnimState[];

// ------------------------------------------------------------------------------------
// -- initialization and destruction

// prepare melee tables for use
UUtError AI2rMelee_Initialize(void);

// terminate melee subsystem
void AI2rMelee_Terminate(void);

// ------------------------------------------------------------------------------------
// -- reference counting

UUtError AI2rMelee_AddReference(AI2tMeleeProfile *ioMeleeProfile, ONtCharacter *inCharacter);
void AI2rMelee_RemoveReference(AI2tMeleeProfile *ioMeleeProfile, ONtCharacter *inCharacter);

// ------------------------------------------------------------------------------------
// -- interface, selection and update

// clear the melee state
void AI2rMeleeState_Clear(AI2tMeleeState *ioMeleeState, UUtBool inClearFightInfo);

// evaluate, select and execute melee techniques
UUtBool AI2rMelee_Update(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState, struct AI2tCombatState *ioCombatState);

// sample the local environment around a character
void AI2rMelee_TestLocalEnvironment(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState);

// notification from the character animation system that an action has been performed
void AI2rMelee_NotifyAnimation(ONtCharacter *ioCharacter, TRtAnimation *inAnimation,
							   UUtUns16 inAnimType, UUtUns16 inNextAnimType,
							   TRtAnimState inCurFromState, TRtAnimState inNextAnimState);

// control for the melee display window
void AI2rMelee_GlobalUpdate(void);
void AI2rMelee_GlobalFinish(void);
UUtBool AI2rMelee_ToggleStatusDisplay(void);

// ------------------------------------------------------------------------------------
// -- pre-processing

// pre-process a melee profile and prepare it for use
UUtError AI2rMelee_PrepareForUse(AI2tMeleeProfile *ioMeleeProfile);

// check whether a particular move would actually be valid by doing an animation lookup on it
UUtBool AI2rMelee_MoveIsValid(TRtAnimationCollection *inCollection, UUtUns32 inMove,
							   UUtUns16 *outAnimType, TRtDirection *outMoveDir,
							   AI2tMeleeMoveState *ioFromState, AI2tMeleeMoveState *outToState,
							   TRtAnimation **outAnimation, TRtAnimation **outTargetAnimation,
							   float *outThrowDistance);

// ------------------------------------------------------------------------------------
// -- miscellany

static UUcInline UUtBool AI2rMelee_StateIsJumping(AI2tMeleeMoveState inState)
{
	switch(inState) {
		case AI2cMeleeMoveState_Jumping:
		case AI2cMeleeMoveState_JumpingForwards:
		case AI2cMeleeMoveState_JumpingLeft:
		case AI2cMeleeMoveState_JumpingRight:
		case AI2cMeleeMoveState_JumpingBack:
			return UUcTrue;
                default:
                        return UUcFalse;
	}
}

#endif
