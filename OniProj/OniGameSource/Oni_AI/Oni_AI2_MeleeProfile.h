#pragma once
#ifndef ONI_AI2_MeleeProfile_H
#define ONI_AI2_MeleeProfile_H

/*
	FILE:	Oni_AI2_MeleeProfile.h

	AUTHOR:	Chris Butcher

	CREATED: May 19, 2000

	PURPOSE: Definitions for Oni AI's Melee Profiles

	Copyright (c) 2000

*/

#include "BFW_Totoro.h"

// ------------------------------------------------------------------------------------
// -- constants

// user-specifiable flags
enum {
	AI2cTechniqueUserFlag_Interruptable				= (1 << 0),
	AI2cTechniqueUserFlag_GenerousDirection			= (1 << 1),
	AI2cTechniqueUserFlag_Fearless					= (1 << 2)
};

// load-time computed flags
//
// bits 0-7		melee move state which the technique starts from
// bits 8-11	move direction of the technique
#define AI2cTechniqueComputedMask_StartState		((1 << 8) - 1)
#define AI2cTechniqueComputedShift_MoveDirection	8
#define AI2cTechniqueComputedMask_MoveDirection		(((1 << 4) - 1) << AI2cTechniqueComputedShift_MoveDirection)

enum {
	AI2cTechniqueComputedFlag_IsAttack				= (1 << 12),
	AI2cTechniqueComputedFlag_IsDodge				= (1 << 13),
	AI2cTechniqueComputedFlag_Broken				= (1 << 14),
	AI2cTechniqueComputedFlag_IsHigh				= (1 << 15),
	AI2cTechniqueComputedFlag_IsLow					= (1 << 16),
	AI2cTechniqueComputedFlag_AllowTransition		= (1 << 17),
	AI2cTechniqueComputedFlag_IsAction				= (1 << 18),
	AI2cTechniqueComputedFlag_IsReaction			= (1 << 19),
	AI2cTechniqueComputedFlag_IsSpacing				= (1 << 20),
	AI2cTechniqueComputedFlag_Unblockable			= (1 << 21),
	AI2cTechniqueComputedFlag_HasBlockStun			= (1 << 22),
	AI2cTechniqueComputedFlag_HasStagger			= (1 << 23),
	AI2cTechniqueComputedFlag_IsThrow				= (1 << 24),
	AI2cTechniqueComputedFlag_FromFront				= (1 << 25),
	AI2cTechniqueComputedFlag_FromBehind			= (1 << 26),
	AI2cTechniqueComputedFlag_IsPistolDisarm		= (1 << 27),
	AI2cTechniqueComputedFlag_IsRifleDisarm			= (1 << 28),
	AI2cTechniqueComputedFlag_SpecialCaseMove		= (1 << 29),

	// below here; unimplemented
	AI2cTechniqueComputedFlag_RequireNoRifle		= (1 << 30),
	AI2cTechniqueComputedFlag_RequireRifle			= (1 << 31)
};
#define AI2cTechniqueComputedMask_IsDisarm				(AI2cTechniqueComputedFlag_IsPistolDisarm | AI2cTechniqueComputedFlag_IsRifleDisarm)
#define AI2cTechniqueComputedMask_TargetDirSensitive	(AI2cTechniqueComputedFlag_FromFront | AI2cTechniqueComputedFlag_FromBehind)
#define AI2cTechniqueComputedMask_Damaging				(AI2cTechniqueComputedFlag_IsThrow | AI2cTechniqueComputedFlag_IsAttack)
#define AI2cTechniqueComputedMask_RequiresAttackCookie	(AI2cTechniqueComputedMask_Damaging | AI2cTechniqueComputedFlag_SpecialCaseMove)

// type of move
#define AI2cMoveType_Shift				28
#define AI2cMoveType_Bits				(32 - AI2cMoveType_Shift)
#define AI2cMoveType_Mask				(((1u << AI2cMoveType_Bits) - 1) << AI2cMoveType_Shift)

enum {
	AI2cMoveType_Attack								= (0 << AI2cMoveType_Shift),
	AI2cMoveType_Position							= (1 << AI2cMoveType_Shift),
	AI2cMoveType_Maneuver							= (2 << AI2cMoveType_Shift),
	AI2cMoveType_Evade								= (3 << AI2cMoveType_Shift),
	AI2cMoveType_Throw								= (4 << AI2cMoveType_Shift),

	AI2cMoveType_Max								= (5 << AI2cMoveType_Shift)
};

#define AI2cMeleeProfile_MaxTransitions				55
#define AI2cMeleeProfile_TransitionMaxToStates		4
#define AI2cMelee_GroupNumAttackers					4

// kinds of jump that a character can execute
typedef enum {
	AI2cMeleeJumpType_None							= 0,
	AI2cMeleeJumpType_Vertical						= 1,
	AI2cMeleeJumpType_Standing						= 2,
	AI2cMeleeJumpType_RunForward					= 3,
	AI2cMeleeJumpType_RunBack						= 4,
	AI2cMeleeJumpType_RunLeft						= 5,
	AI2cMeleeJumpType_RunRight						= 6,

	AI2cMeleeJumpType_Max							= 7
} AI2tMeleeJumpType;

// ------------------------------------------------------------------------------------
// -- structure definitions

typedef struct AI2tMeleeMove
{
	UUtUns32		move;
	float			param[3];

	// remainder not stored on disk - calculated at pre-process time
	//
	// the pre-processor executes whenever an unused profile is first referenced by the AI,
	// or when the profile's character class changes, or the melee profile editing dialog is
	// displayed or updated
	TRtAnimState	current_state;		// approx state **before** the animation executes - one of the AI2cMeleeMoveStates
	TRtAnimation	*animation;
	float			attack_endpos;		// distance along line of technique - only set for attacks and throws
	TRtAnimation	*target_animation;	// for throws
	float			target_endpos;
	UUtBool			is_broken;
} AI2tMeleeMove;

typedef struct AI2tMeleeTechnique
{
	/*
	 * persistent parameters - stored on disk
	 */
	char		name[64];
	UUtUns32	user_flags;
	UUtUns32	base_weight;
	UUtUns32	importance;
	UUtUns32	num_moves;
	UUtUns32	start_index;
	UUtUns32	delay_frames;

	/*
	 * info about the move sequence - calculated at pre-process time
	 */
	UUtUns16	index_in_profile;
	UUtUns16	delaytimer_index;
	UUtUns32	computed_flags;
	UUtUns32	max_damage;

	float		attack_maxrange;
	float		attack_idealrange;
	UUtUns16	attack_initialframes;

	TRtAnimation *attack_anim;		// only set up for techniques that have an attack move that could be delayed to,
	UUtUns32	attackmove_itr;		// or is at the very start of the technique (i.e. not if guarded by multiple positioning or
									// throw moves)
	float		throw_distance;		// distance requirement to perform the throw

} AI2tMeleeTechnique;

typedef struct AI2tMeleeTransition
{
	UUtUns8			direction;
	UUtUns8			from_state;
	UUtUns8			num_to_states;
	UUtUns8			anim_frames;

	UUtUns8			to_state[AI2cMeleeProfile_TransitionMaxToStates];

	float			weight_modifier;
	LItButtonBits	move_keys;

	TRtAnimation	*animation;
	UUtUns16		anim_type;
	UUtUns16		anim_tostate;
	float			anim_distance;
	float			anim_endvelocity;

} AI2tMeleeTransition;

typedef struct AI2tMeleeProfile
{
	UUtUns32	id;
	char		name[64];
	char		char_classname[64];

	// general ability parameters
	UUtUns32	attacknotice_percentage;
	UUtUns32	dodge_base_percentage;			// attacks with dmg zero have chance = base of being dodged
	UUtUns32	dodge_additional_percentage;	// <- some fraction of this is added based on damage (linear interp)
	UUtUns32	dodge_damage_threshold;			// attacks with dmg >= threshold have chance = base + additional
	UUtUns32	blockskill_percentage;
	UUtUns32	blockgroup_percentage;

	// weight values which control technique selection... each attack is multiplied by exactly
	// one of these. typically the order given here will be their order from high to low.
	float		weight_notblocking_any;
	float		weight_notblocking_changestance;
	float		weight_blocking_unblockable;
	float		weight_blocking_stagger;
	float		weight_blocking_blockstun;
	float		weight_blocking;

	float		weight_throwdanger;				// throwing the target into danger

	// frames to be dazed for after a knockdown
	UUtUns16	dazed_minframes;
	UUtUns16	dazed_maxframes;

	// variable-length arrays. lengths of arrays stored on disk, but the pointers are set up
	// at load time (and point to dynamically allocated memory). the technique array stores
	// actions first, then reactions.
	UUtUns32	num_actions;
	UUtUns32	num_reactions;
	UUtUns32	num_spacing;
	UUtUns32	num_moves;

	AI2tMeleeTechnique	*technique;
	AI2tMeleeMove		*move;

	/*
	 * runtime-calculated data
	 */
	ONtCharacterClass	*char_class;
	UUtUns32			reference_count;
	UUtUns32			num_transitions;
	AI2tMeleeTransition	transitions[AI2cMeleeProfile_MaxTransitions];

	// jumping data, calculated at prepare time from character
	// class's jump parameters and animations
	float				min_jumpframes;
	float				max_jumpframes;
	float				jump_vel[AI2cMeleeJumpType_Max];

} AI2tMeleeProfile;

#endif	// ONI_AI2_MeleeProfile_H
