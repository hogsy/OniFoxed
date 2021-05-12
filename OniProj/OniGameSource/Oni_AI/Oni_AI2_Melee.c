/*
	FILE:	Oni_AI2_Melee.c
	
	AUTHOR:	Chris Butcher
	
	CREATED: May 19, 2000
	
	PURPOSE: Melee AI for Oni
	
	Copyright (c) 2000

*/

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_Totoro.h"
#include "BFW_Totoro_Private.h"		// ugh - needed to use TRrCollection_Lookup_Range for throws

#include "Oni_AI2.h"
#include "Oni_Character.h"
#include "Oni_Character_Animation.h"
#include "Oni_Combos.h"
#include "Oni_Aiming.h"
#include "Oni_Persistance.h"

#define DEBUG_VERBOSE_TECHNIQUE		0
#define DEBUG_VERBOSE_MELEE			0
#define DEBUG_VERBOSE_WEIGHT		0
#define DEBUG_VERBOSE_WEIGHTVAL		0
#define DEBUG_VERBOSE_SPACINGWEIGHT 0
#define DEBUG_VERBOSE_ATTACK		0
#define DEBUG_VERBOSE_POSITION		0
#define DEBUG_VERBOSE_MANEUVER		0
#define DEBUG_VERBOSE_EVADE			0
#define DEBUG_VERBOSE_THROW			0
#define DEBUG_VERBOSE_TRANSITION	0

#define MELEE_ACTINTERSECTIONS		UUcFalse
#define MELEE_REACTINTERSECTIONS	UUcFalse
#define MELEE_CHECKINTERSECTIONS	UUcFalse

// ------------------------------------------------------------------------------------
// -- constants

#define AI2cMeleeAttackQueue_HardFrames		4
#define AI2cMeleeAttackQueue_SoftFrames		8

#define AI2cMelee_BlockStunThreshold		15

#define AI2cMeleeDirectionalAttackMin		(45.0f * M3cDegToRad)
#define AI2cMeleeDirectionalAttackMax		(60.0f * M3cDegToRad)
#define AI2cMeleeGenerousDirectionalMin		(70.0f * M3cDegToRad)
#define AI2cMeleeGenerousDirectionalMax		(110.0f * M3cDegToRad)
#define AI2cMeleeDirectionalMinFwdWeight	0.35f

#define AI2cMeleeStandingTurnAngle			(120.0f * M3cDegToRad)
#define AI2cMeleeMinStartAttackAngle		(15.0f * M3cDegToRad)
#define AI2cMeleeGiveUpAttackAngle			(3.0f * M3cDegToRad)

#define AI2cMeleeCurrentTechniqueMultiplier	2.0f

#define AI2cMelee_IdealRangeSubtractFraction 0.5f
#define AI2cMelee_AdditionalDelayTolerance	5.0f

#define AI2cMelee_PositionIgnoreMinDistance	1.0f
#define AI2cMelee_SkipPositioningDistance	3.0f
#define AI2cMelee_PositionLocalMoveDist		15.0f
#define AI2cMelee_PositionNotFacingTimer	60
#define AI2cMelee_PositionMissByHeightTimer 4
#define AI2cMelee_AbortDisableFrames		60
#define AI2cMelee_PositionTransition_ToFrames 3
#define AI2cMelee_MaxDelayFrames			120
#define AI2cMelee_JumpLaunchFraction		0.25f	// 0,5 = apex, 1.0 = hit ground
#define AI2cMelee_StandingJumpRequiresOffset 7.0f

#define AI2cMelee_CircleAngleTolerance		10.0f
#define AI2cMelee_TurningRadiusSafety		0.9f

#define AI2cMelee_WeightAdjustAborted		1000.0f
#define AI2cMelee_WeightAdjustDecay			0.6f	// applied each time we check actions
#define AI2cMelee_MaxWeightAdjust			0.8f
#define AI2cMelee_RepeatWeightPenalty		0.4f

#define AI2cMelee_ThrowWeightInterpAngle	(30.0f * M3cDegToRad)
#define AI2cMelee_ThrowMaxDeltaFacing		(45.0f * M3cDegToRad)
#define AI2cMelee_ThrowDistSafety			0.9f

#define AI2cMelee_LocalAwareness_Dist		50.0f
#define AI2cMelee_LocalAwareness_Safety		10.0f

#define AI2cMelee_Spacing_MinToleranceBand	5.0f
#define AI2cMelee_Spacing_SameDistTolerance	10.0f
#define AI2cMelee_Spacing_SameAngleTolerance (20.0f * M3cDegToRad)
#define AI2cMelee_Spacing_MaxUrgencyAngle	(30.0f * M3cDegToRad)

// ------------------------------------------------------------------------------------
// -- internal function prototypes

/*
 * movetype functions
 */

static UUtUns32 AI2iMelee_Attack_Iterate(UUtUns32 inIterator);
static UUtUns32 AI2iMelee_Position_Iterate(UUtUns32 inIterator);
static UUtUns32 AI2iMelee_Maneuver_Iterate(UUtUns32 inIterator);
static UUtUns32 AI2iMelee_Evade_Iterate(UUtUns32 inIterator);
static UUtUns32 AI2iMelee_Throw_Iterate(UUtUns32 inIterator);

static UUtBool AI2iMelee_Attack_GetName(UUtUns32 inMove, char *outString, UUtUns32 inStringLength);
static UUtBool AI2iMelee_Position_GetName(UUtUns32 inMove, char *outString, UUtUns32 inStringLength);
static UUtBool AI2iMelee_Maneuver_GetName(UUtUns32 inMove, char *outString, UUtUns32 inStringLength);
static UUtBool AI2iMelee_Evade_GetName(UUtUns32 inMove, char *outString, UUtUns32 inStringLength);
static UUtBool AI2iMelee_Throw_GetName(UUtUns32 inMove, char *outString, UUtUns32 inStringLength);

static UUtUns32 AI2iMelee_Attack_GetParams(UUtUns32 inMove, UUtBool inSpacing, char *outStringPtrs[3]);
static UUtUns32 AI2iMelee_Position_GetParams(UUtUns32 inMove, UUtBool inSpacing, char *outStringPtrs[3]);
static UUtUns32 AI2iMelee_Maneuver_GetParams(UUtUns32 inMove, UUtBool inSpacing, char *outStringPtrs[3]);
static UUtUns32 AI2iMelee_Evade_GetParams(UUtUns32 inMove, UUtBool inSpacing, char *outStringPtrs[3]);
static UUtUns32 AI2iMelee_Throw_GetParams(UUtUns32 inMove, UUtBool inSpacing, char *outStringPtrs[3]);

static UUtUns16 AI2iMelee_Attack_GetAnimType(UUtUns32 inMove, TRtDirection *outDirection,
											AI2tMeleeMoveState *outFromState, AI2tMeleeMoveState *outToState,
											UUtUns16 *outTargetAnimState, UUtUns16 *outTargetAnimVarient);
static UUtUns16 AI2iMelee_Position_GetAnimType(UUtUns32 inMove, TRtDirection *outDirection,
											AI2tMeleeMoveState *outFromState, AI2tMeleeMoveState *outToState,
											UUtUns16 *outTargetAnimState, UUtUns16 *outTargetAnimVarient);
static UUtUns16 AI2iMelee_Maneuver_GetAnimType(UUtUns32 inMove, TRtDirection *outDirection,
											AI2tMeleeMoveState *outFromState, AI2tMeleeMoveState *outToState,
											UUtUns16 *outTargetAnimState, UUtUns16 *outTargetAnimVarient);
static UUtUns16 AI2iMelee_Evade_GetAnimType(UUtUns32 inMove, TRtDirection *outDirection,
											AI2tMeleeMoveState *outFromState, AI2tMeleeMoveState *outToState,
											UUtUns16 *outTargetAnimState, UUtUns16 *outTargetAnimVarient);
static UUtUns16 AI2iMelee_Throw_GetAnimType(UUtUns32 inMove, TRtDirection *outDirection,
											AI2tMeleeMoveState *outFromState, AI2tMeleeMoveState *outToState,
											UUtUns16 *outTargetAnimState, UUtUns16 *outTargetAnimVarient);

static UUtBool AI2iMelee_Attack_CanFollow(UUtUns32 inMove, UUtUns32 inPrevMove);
static UUtBool AI2iMelee_Position_CanFollow(UUtUns32 inMove, UUtUns32 inPrevMove);
static UUtBool AI2iMelee_Maneuver_CanFollow(UUtUns32 inMove, UUtUns32 inPrevMove);
static UUtBool AI2iMelee_Evade_CanFollow(UUtUns32 inMove, UUtUns32 inPrevMove);
static UUtBool AI2iMelee_Throw_CanFollow(UUtUns32 inMove, UUtUns32 inPrevMove);

static UUtBool AI2iMelee_Attack_Start(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove);
static UUtBool AI2iMelee_Position_Start(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove);
static UUtBool AI2iMelee_Maneuver_Start(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove);
static UUtBool AI2iMelee_Evade_Start(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove);
static UUtBool AI2iMelee_Throw_Start(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove);

static UUtBool AI2iMelee_Attack_Update(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove);
static UUtBool AI2iMelee_Position_Update(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove);
static UUtBool AI2iMelee_Maneuver_Update(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove);
static UUtBool AI2iMelee_Evade_Update(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove);
static UUtBool AI2iMelee_Throw_Update(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove);

static void AI2iMelee_Attack_Finish(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove);
static void AI2iMelee_Position_Finish(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove);
static void AI2iMelee_Maneuver_Finish(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove);
static void AI2iMelee_Evade_Finish(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove);
static void AI2iMelee_Throw_Finish(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove);

static UUtBool AI2iMelee_Attack_FaceTarget(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
										  AI2tMeleeMove *inMove);
static UUtBool AI2iMelee_Position_FaceTarget(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
										  AI2tMeleeMove *inMove);
static UUtBool AI2iMelee_Maneuver_FaceTarget(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
										  AI2tMeleeMove *inMove);
static UUtBool AI2iMelee_Evade_FaceTarget(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
										  AI2tMeleeMove *inMove);
static UUtBool AI2iMelee_Throw_FaceTarget(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
										  AI2tMeleeMove *inMove);

static UUtBool AI2iMelee_Attack_CanEvade(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
										  AI2tMeleeMove *inMove);
static UUtBool AI2iMelee_Position_CanEvade(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
										  AI2tMeleeMove *inMove);
static UUtBool AI2iMelee_Maneuver_CanEvade(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
										  AI2tMeleeMove *inMove);
static UUtBool AI2iMelee_Evade_CanEvade(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
										  AI2tMeleeMove *inMove);
static UUtBool AI2iMelee_Throw_CanEvade(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
										  AI2tMeleeMove *inMove);

/*
 * melee move manager functions
 */

// reset for a new target
static void AI2iMelee_SetupNewTarget(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState);

// reset for a new technique
static void AI2iMeleeState_NewTechnique(AI2tMeleeState *ioMeleeState);

// abort a technique in progress
static void AI2iMeleeState_AbortTechnique(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState);

// finish the current move
static UUtBool AI2iMelee_StartNextMove(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState, AI2tMeleeMove *inMove);

// finish the current technique
static void AI2iMelee_FinishTechnique(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState);

// get the next move
static UUtBool AI2iMelee_NextMove(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState, UUtUns32 *outMove);

// can we transition to a new state? (note: character may be NULL for a 'theoretical' check)
static AI2tMeleeTransition *AI2iMelee_CheckStateTransition(ONtCharacter *ioCharacter, AI2tMeleeProfile *ioMeleeProfile,
														AI2tMeleeMoveState inFromState, AI2tMeleeMoveState inDesiredState,
														TRtAnimState inTestState);

// check to see whether we will react to or evade an opponent's attack
static void AI2iMelee_CheckEvasionChance(AI2tMeleeState *ioMeleeState, AI2tMeleeProfile *ioMeleeProfile,
										 const TRtAnimation *inAnimation);

/*
 * move weighting functions
 */

// recalculate random seeds
static void AI2iMelee_NewRandomSeeds(AI2tMeleeState *ioMeleeState);

// calculate absolute angle to target
static float AI2iMelee_AngleToTarget(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState);

// determine angle to target, and weight multipliers based on directions
static void AI2iMelee_CalculateDirectionWeights(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
												float inAngle, float *outMainWeights, float *outGenerousWeights);

// set up the attacker's state in preparation for a call to TRrCheckAnimationBounds
static void AI2iMelee_SetupAttackerState(ONtCharacter *ioCharacter, AI2tMeleeProfile *ioMeleeProfile, TRtCharacterIntersectState *ioState,
										 UUtBool inStartJump, float inJumpVelocity, TRtDirection inDirection, TRtAnimation *inAnimation);

// find a linked series of transitions from one state to another, counting up their length
static UUtBool AI2iMelee_GetTransitionLength(AI2tMeleeProfile *ioMeleeProfile, AI2tMeleeMoveState *inCurrentState,
											  UUtUns16 *inCurrentAnimState, AI2tMeleeMoveState inDesiredState, float *ioWeight,
											  float *ioTransitionDistance, float *ioTransitionTime, UUtUns32 *outNumTransitions);

// consider and weight a technique
struct AI2tConsideredTechnique;
static void AI2iMelee_WeightTechnique(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState, AI2tMeleeProfile *ioMeleeProfile,
									  TRtAnimIntersectContext *ioContext, UUtBool inConsiderAttack, TRtDirection inDirection,
									  AI2tMeleeMoveState inCurrentState, TRtAnimState inCurrentAnimState,
									  UUtBool inCanBlock, UUtBool inBlockLow, UUtBool inBlockHigh,
									  struct AI2tConsideredTechnique *ioTechnique);

// consider and weight a spacing behavior
static void AI2iMelee_WeightSpacingBehavior(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState, AI2tMeleeProfile *ioMeleeProfile,
									  TRtDirection inDirection, AI2tMeleeMoveState inCurrentState, TRtAnimState inCurrentAnimState,
									  struct AI2tConsideredTechnique *ioTechnique);

// consider the movement required by an attack or throw along the line of the technique
static UUtBool AI2iMelee_WeightLocalMovement(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
											 AI2tMeleeTechnique *inTechnique, float inMovement, float *outMultiplier);

// check the viability of an attacking technique
static UUtBool AI2iCheckTechniqueViability(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState, AI2tMeleeProfile *ioMeleeProfile,
									  UUtBool inIsAction, TRtAnimIntersectContext *ioContext, struct AI2tConsideredTechnique *ioTechnique);

// try to make the character move in a local direction this frame
static UUtBool AI2iMelee_AddLocalMovement(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
										  TRtDirection inDirection, UUtBool inLookAside);

// check to see if the target is throwable
static UUtBool AI2iMelee_TargetIsThrowable(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);

/*
 * update functions
 */

// get a character's current animation state (accounting for shortcuts, etc)
static UUtUns16 AI2iMelee_Position_GetCurrentAnimState(ONtCharacter *inCharacter, AI2tMeleeMoveState *outState);

// convert a totoro animation state into a melee state
static AI2tMeleeMoveState AI2iMelee_TranslateTotoroAnimState(ONtCharacter *inCharacter, UUtUns16 inAnimState);

// update the character's facing
static void AI2iMelee_UpdateFacing(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState);
static void AI2iMelee_ApplyFacing(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState);

// catch errors fired off when trying to find a path
static UUtBool AI2iMelee_PathfindingErrorHandler(ONtCharacter *inCharacter, UUtUns32 inErrorID,
													UUtUns32 inParam1, UUtUns32 inParam2,
													UUtUns32 inParam3, UUtUns32 inParam4);

// set up orders for the situation when we don't have any melee techniques
static UUtBool AI2iMelee_HandleNoTechnique(ONtCharacter *ioCharater, AI2tCombatState *ioCombatState, AI2tMeleeState *ioMeleeState);

// ------------------------------------------------------------------------------------
// -- internal structure definitions

typedef struct AI2tMelee_AttackMove
{
	char			name[32];
	UUtUns32		attackmove_flags;
	UUtUns16		anim_type;
	UUtUns16		combo_prev_animtype;
	LItButtonBits	keys_isdown;
	LItButtonBits	keys_wentdown;

} AI2tMelee_AttackMove;

// attackmove_flags is made from:
//   bits 0-7:		AI2tMeleeMoveState (from)
//   bits 8-15:		AI2tMeleeMoveState (to)
//   bits 16-19:	TRtDirection
#define ATTACK_FLAGS(from, to, dir)			(AI2cMeleeMoveState_##from | (AI2cMeleeMoveState_##to << 8) | \
																		(TRcDirection_##dir << 16))
#define AI2mMelee_AttackFromState(flags)	 (flags & 0x000000ff)
#define AI2mMelee_AttackToState(flags)		((flags & 0x0000ff00) >> 8)
#define AI2mMelee_AttackDirection(flags)	((flags & 0x000f0000) >> 16)

typedef struct AI2tMelee_PositionMove
{
	char					name[32];
	TRtDirection			direction;

	UUtBool					can_delay;				// if there is a delay, we delay once we get to state 0
	UUtBool					can_wait_for_facing;	// can wait at the conclusion of the move until facing is correct
	UUtBool					ignore_endtrans_dist;
	AI2tMeleeJumpType		jump_type;

	UUtUns32				num_states;
	AI2tMeleeMoveState		states[AI2cMeleeProfile_TransitionMaxToStates];

} AI2tMelee_PositionMove;

typedef struct AI2tMeleeStateTranslator
{
	TRtAnimState		totoro_state;
	AI2tMeleeMoveState	melee_state;

} AI2tMeleeStateTranslator;

typedef struct AI2tMelee_EvadeMove
{
	char			name[32];
	UUtUns32		evademove_flags;
	LItButtonBits	keys_isdown;
	LItButtonBits	keys_wentdown;

} AI2tMelee_EvadeMove;

// evademove_flags is made from:
//   bits 0-7:		AI2tMeleeMoveState (from)
//   bits 8-15:		AI2tMeleeMoveState (to)
//   bits 16-31:	UUtUns16 (anim_type)
#define EVADE_FLAGS(from, to, type)			(AI2cMeleeMoveState_##from | (AI2cMeleeMoveState_##to << 8) | \
																		(ONcAnimType_##type << 16))
#define AI2mMelee_EvadeFromState(flags)		(flags & 0x000000ff)
#define AI2mMelee_EvadeToState(flags)		((flags & 0x0000ff00) >> 8)
#define AI2mMelee_EvadeAnimType(flags)		((flags & 0xffff0000) >> 16)

typedef struct AI2tMelee_ThrowMove
{
	char			name[32];
	UUtUns32		throwmove_flags;

} AI2tMelee_ThrowMove;

enum {
	AI2cMelee_Throw_FromBehind		= (1 << 0),
	AI2cMelee_Throw_FromRunning		= (1 << 1),
	AI2cMelee_Throw_DisarmPistol	= (1 << 2),
	AI2cMelee_Throw_DisarmRifle		= (1 << 3),
	AI2cMelee_Throw_VictimRunning	= (1 << 4)
};
#define AI2cMelee_Throw_FromFront		0
#define AI2cMelee_Throw_FromStanding	0
#define AI2cMelee_Throw_DisarmNo		0
#define AI2cMelee_Throw_VictimStanding	0

#define AI2mMelee_ThrowAnimType(flags)		((flags & 0xffff0000) >> 16)

#define THROW_FLAGS(animtype, direction, ourstate, disarmtype, victimstate)	(AI2cMelee_Throw_From##direction | AI2cMelee_Throw_From##ourstate | \
									AI2cMelee_Throw_Disarm##disarmtype | AI2cMelee_Throw_Victim##victimstate | (ONcAnimType_##animtype << 16))

// ------------------------------------------------------------------------------------
// -- external globals

const UUtUns32 AI2cMeleeNumMoveTypes = (AI2cMoveType_Max >> AI2cMoveType_Shift);
const AI2tMeleeMoveType AI2cMeleeMoveTypes[AI2cMoveType_Max >> AI2cMoveType_Shift] = {
	{"Attack",			
		AI2iMelee_Attack_Iterate,		
		AI2iMelee_Attack_GetName,		
		AI2iMelee_Attack_GetParams,			
		AI2iMelee_Attack_GetAnimType,		
		AI2iMelee_Attack_CanFollow,
		AI2iMelee_Attack_Start,
		AI2iMelee_Attack_Update,
		AI2iMelee_Attack_Finish,
		AI2iMelee_Attack_FaceTarget,
		AI2iMelee_Attack_CanEvade},
	
	{"Position",			
		AI2iMelee_Position_Iterate,		
		AI2iMelee_Position_GetName,		
		AI2iMelee_Position_GetParams,			
		AI2iMelee_Position_GetAnimType,		
		AI2iMelee_Position_CanFollow,
		AI2iMelee_Position_Start,
		AI2iMelee_Position_Update,
		AI2iMelee_Position_Finish,
		AI2iMelee_Position_FaceTarget,
		AI2iMelee_Position_CanEvade},
	
	{"Maneuver",			
		AI2iMelee_Maneuver_Iterate,		
		AI2iMelee_Maneuver_GetName,		
		AI2iMelee_Maneuver_GetParams,			
		AI2iMelee_Maneuver_GetAnimType,		
		AI2iMelee_Maneuver_CanFollow,
		AI2iMelee_Maneuver_Start,
		AI2iMelee_Maneuver_Update,
		AI2iMelee_Maneuver_Finish,
		AI2iMelee_Maneuver_FaceTarget,
		AI2iMelee_Maneuver_CanEvade},

	{"Evade",			
		AI2iMelee_Evade_Iterate,		
		AI2iMelee_Evade_GetName,		
		AI2iMelee_Evade_GetParams,			
		AI2iMelee_Evade_GetAnimType,		
		AI2iMelee_Evade_CanFollow,
		AI2iMelee_Evade_Start,
		AI2iMelee_Evade_Update,
		AI2iMelee_Evade_Finish,
		AI2iMelee_Evade_FaceTarget,
		AI2iMelee_Evade_CanEvade},

	{"Throw",			
		AI2iMelee_Throw_Iterate,		
		AI2iMelee_Throw_GetName,		
		AI2iMelee_Throw_GetParams,			
		AI2iMelee_Throw_GetAnimType,		
		AI2iMelee_Throw_CanFollow,
		AI2iMelee_Throw_Start,
		AI2iMelee_Throw_Update,
		AI2iMelee_Throw_Finish,
		AI2iMelee_Throw_FaceTarget,
		AI2iMelee_Throw_CanEvade}};
	

// *** translation from AI2tMeleeMoveState to TRtAnimState
const TRtAnimState AI2cMelee_TotoroAnimState[AI2cMeleeMoveState_Max] =
	{ONcAnimState_None, ONcAnimState_Standing, ONcAnimState_Crouch, ONcAnimState_Crouch_Start, ONcAnimState_Fallen_Back,
	ONcAnimState_Run_Start, ONcAnimState_Run_Back_Start, ONcAnimState_Sidestep_Left_Start_Long_Name, ONcAnimState_Sidestep_Right_Start_Long_Name,
	ONcAnimState_Running_Right_Down, ONcAnimState_Running_Back_Right_Down, ONcAnimState_Sidestep_Left_Right_Down, ONcAnimState_Sidestep_Right_Right_Down,
	ONcAnimState_Falling, ONcAnimState_Falling_Forward, ONcAnimState_Falling_Back, ONcAnimState_Falling_Left, ONcAnimState_Falling_Right};

// ------------------------------------------------------------------------------------
// -- internal globals

// *** anim types for our delaying moves - from AI2tMeleeMoveState to TRtAnimType
const TRtAnimType AI2cMelee_DelayAnimType[AI2cMeleeMoveState_Max] =
	{ONcAnimType_None, ONcAnimType_Stand, ONcAnimType_Crouch, ONcAnimType_Crouch, ONcAnimType_Hit_Fallen,
	ONcAnimType_Run, ONcAnimType_Run_Backwards, ONcAnimType_Run_Sidestep_Left, ONcAnimType_Run_Sidestep_Right,
	ONcAnimType_Run, ONcAnimType_Run_Backwards, ONcAnimType_Run_Sidestep_Left, ONcAnimType_Run_Sidestep_Right,
	ONcAnimType_Fly, ONcAnimType_Fly, ONcAnimType_Fly, ONcAnimType_Fly, ONcAnimType_Fly};

// *** whether we block crouching by preference when we're in a given melee state
const UUtBool AI2cMeleeState_BlockCrouching[AI2cMeleeMoveState_Max] = 
	{UUcFalse, UUcFalse, UUcTrue, UUcTrue, UUcFalse,
	UUcFalse, UUcFalse, UUcFalse, UUcFalse,
	UUcFalse, UUcFalse, UUcFalse, UUcFalse,
	UUcFalse, UUcFalse, UUcFalse, UUcFalse, UUcFalse};

// *** whether we can turn on the spot in order to line up an attack while in this move state
const UUtBool AI2cMeleeState_CanTurnForAttack[AI2cMeleeMoveState_Max] = 
	{UUcTrue, UUcTrue, UUcTrue, UUcFalse, UUcFalse,
	UUcTrue, UUcTrue, UUcTrue, UUcTrue,
	UUcTrue, UUcTrue, UUcTrue, UUcTrue,
	UUcFalse, UUcFalse, UUcFalse, UUcFalse, UUcFalse};

// *** keys to hold down in order to stay in this melee state
const LItButtonBits AI2cMeleeState_MaintainStateKeys[AI2cMeleeMoveState_Max] = 
	{0, 0, LIc_BitMask_Crouch, LIc_BitMask_Crouch, 0,
	LIc_BitMask_Forward, LIc_BitMask_Backward, LIc_BitMask_StepLeft, LIc_BitMask_StepRight,
	LIc_BitMask_Forward, LIc_BitMask_Backward, LIc_BitMask_StepLeft, LIc_BitMask_StepRight,
	LIc_BitMask_Jump, LIc_BitMask_Jump | LIc_BitMask_Forward, LIc_BitMask_Jump | LIc_BitMask_Backward,
	LIc_BitMask_Jump | LIc_BitMask_StepLeft, LIc_BitMask_Jump | LIc_BitMask_StepRight};

// *** translation from TRtAnimState to AI2tMeleeMoveState

const AI2tMeleeStateTranslator AI2cMelee_StateHash[] = {
	{ONcAnimState_Standing,							AI2cMeleeMoveState_Standing},
	{ONcAnimState_Walking_Left_Down,				AI2cMeleeMoveState_Standing},
	{ONcAnimState_Walking_Right_Down,				AI2cMeleeMoveState_Standing},

	{ONcAnimState_Crouch,							AI2cMeleeMoveState_Crouching},

	{ONcAnimState_Crouch_Start,						AI2cMeleeMoveState_CrouchStart},

	{ONcAnimState_Fallen_Back,						AI2cMeleeMoveState_Prone},
	{ONcAnimState_Fallen_Front,						AI2cMeleeMoveState_Prone},
	{ONcAnimState_Lie_Back,							AI2cMeleeMoveState_Prone},

	{ONcAnimState_Running_Left_Down,				AI2cMeleeMoveState_Running},
	{ONcAnimState_Running_Right_Down,				AI2cMeleeMoveState_Running},
	{ONcAnimState_Run_Start,						AI2cMeleeMoveState_RunStart},
	{ONcAnimState_Run_Accel,						AI2cMeleeMoveState_RunStart},
	{ONcAnimState_Running_Upstairs_Right_Down,		AI2cMeleeMoveState_Running},
	{ONcAnimState_Running_Upstairs_Left_Down,		AI2cMeleeMoveState_Running},
	{ONcAnimState_Run_Jump_Land,					AI2cMeleeMoveState_RunStart},

	{ONcAnimState_Run_Back_Start,					AI2cMeleeMoveState_RunBackStart},
	{ONcAnimState_Running_Back_Right_Down,			AI2cMeleeMoveState_RunningBack},
	{ONcAnimState_Running_Back_Left_Down,			AI2cMeleeMoveState_RunningBack},

	{ONcAnimState_Run_Sidestep_Left,				AI2cMeleeMoveState_RunningLeft},
	{ONcAnimState_Sidestep_Left_Left_Down,			AI2cMeleeMoveState_RunningLeft},
	{ONcAnimState_Sidestep_Left_Right_Down,			AI2cMeleeMoveState_RunningLeft},
	{ONcAnimState_Sidestep_Left_Start,				AI2cMeleeMoveState_RunLeftStart},
	{ONcAnimState_Sidestep_Left_Start_Long_Name,	AI2cMeleeMoveState_RunLeftStart},

	{ONcAnimState_Run_Sidestep_Right,				AI2cMeleeMoveState_RunningRight},
	{ONcAnimState_Sidestep_Right_Left_Down,			AI2cMeleeMoveState_RunningRight},
	{ONcAnimState_Sidestep_Right_Right_Down,		AI2cMeleeMoveState_RunningRight},
	{ONcAnimState_Sidestep_Right_Start,				AI2cMeleeMoveState_RunRightStart},
	{ONcAnimState_Sidestep_Right_Start_Long_Name,	AI2cMeleeMoveState_RunRightStart},

	{ONcAnimState_Jump_Up,							AI2cMeleeMoveState_Jumping},
	{ONcAnimState_Flying,							AI2cMeleeMoveState_Jumping},
	{ONcAnimState_Falling,							AI2cMeleeMoveState_Jumping},

	{ONcAnimState_Run_Jump,							AI2cMeleeMoveState_JumpingForwards},
	{ONcAnimState_Jump_Forward,						AI2cMeleeMoveState_JumpingForwards},
	{ONcAnimState_Falling_Forward,					AI2cMeleeMoveState_JumpingForwards},
	{ONcAnimState_Flying_Forward,					AI2cMeleeMoveState_JumpingForwards},

	{ONcAnimState_Falling_Back,						AI2cMeleeMoveState_JumpingBack},
	{ONcAnimState_Flying_Back,						AI2cMeleeMoveState_JumpingBack},

	{ONcAnimState_Falling_Left,						AI2cMeleeMoveState_JumpingLeft},
	{ONcAnimState_Flying_Left,						AI2cMeleeMoveState_JumpingLeft},
	{ONcAnimState_Sidestep_Left_Jump,				AI2cMeleeMoveState_JumpingLeft},

	{ONcAnimState_Falling_Right,					AI2cMeleeMoveState_JumpingRight},
	{ONcAnimState_Flying_Right,						AI2cMeleeMoveState_JumpingRight},
	{ONcAnimState_Sidestep_Right_Jump,				AI2cMeleeMoveState_JumpingRight},

	{(UUtUns16) -1,				(UUtUns32) -1}};

// this lookup table is built from the associative hash at initialization time
AI2tMeleeMoveState AI2gMelee_StateFromAnimState[ONcAnimState_Max];

// *** melee attack table

// the melee attack table will be recursed and filled out from the global combo table at initialization time
UUtUns32 AI2gNumMeleeAttacks = 0;
AI2tMelee_AttackMove AI2gMeleeAttackTable[] = {
	{"P",			ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_Punch,					ONcAnimType_None, 0,	0},
	{"P P",			ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_Punch2,					ONcAnimType_None, 0,	0},
	{"P P P",		ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_Punch3,					ONcAnimType_None, 0,	0},
	{"P P P P",		ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_Punch4,					ONcAnimType_None, 0,	0},
	{"PF",			ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_Punch_Forward,			ONcAnimType_None, 0,	0},
	{"PL",			ATTACK_FLAGS(Standing, Standing, Left),				ONcAnimType_Punch_Left,				ONcAnimType_None, 0,	0},
	{"PR",			ATTACK_FLAGS(Standing, Standing, Right),			ONcAnimType_Punch_Right,			ONcAnimType_None, 0,	0},
	{"PB",			ATTACK_FLAGS(Standing, Standing, Back),				ONcAnimType_Punch_Back,				ONcAnimType_None, 0,	0},
	{"PD",			ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_Punch_Low,				ONcAnimType_None, 0,	0},

	{"PF PF",		ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_PF_PF,					ONcAnimType_None, 0,	0},
	{"PF PF PF",	ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_PF_PF_PF,				ONcAnimType_None, 0,	0},

	{"PL PL",		ATTACK_FLAGS(Standing, Standing, Left),				ONcAnimType_PL_PL,					ONcAnimType_None, 0,	0},
	{"PL PL PL",	ATTACK_FLAGS(Standing, Standing, Left),				ONcAnimType_PL_PL_PL,				ONcAnimType_None, 0,	0},

	{"PR PR",		ATTACK_FLAGS(Standing, Standing, Right),			ONcAnimType_PR_PR,					ONcAnimType_None, 0,	0},
	{"PR PR PR",	ATTACK_FLAGS(Standing, Standing, Right),			ONcAnimType_PR_PR_PR,				ONcAnimType_None, 0,	0},

	{"PB PB",		ATTACK_FLAGS(Standing, Standing, Back),				ONcAnimType_PB_PB,					ONcAnimType_None, 0,	0},
	{"PB PB PB",	ATTACK_FLAGS(Standing, Standing, Back),				ONcAnimType_PB_PB_PB,				ONcAnimType_None, 0,	0},

	{"PD PD",		ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_PD_PD,					ONcAnimType_None, 0,	0},
	{"PD PD PD",	ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_PD_PD_PD,				ONcAnimType_None, 0,	0},

	{"K",			ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_Kick,					ONcAnimType_None, 0,	0},
	{"K K",			ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_Kick2,					ONcAnimType_None, 0,	0},
	{"K K K",		ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_Kick3,					ONcAnimType_None, 0,	0},
	{"K K KF",		ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_Kick3_Forward,			ONcAnimType_None, 0,	0},
	{"KF",			ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_Kick_Forward,			ONcAnimType_None, 0,	0},
	{"KL",			ATTACK_FLAGS(Standing, Standing, Left),				ONcAnimType_Kick_Left,				ONcAnimType_None, 0,	0},
	{"KR",			ATTACK_FLAGS(Standing, Standing, Right),			ONcAnimType_Kick_Right,				ONcAnimType_None, 0,	0},
	{"KB",			ATTACK_FLAGS(Standing, Standing, Back),				ONcAnimType_Kick_Back,				ONcAnimType_None, 0,	0},
	{"KD",			ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_Kick_Low,				ONcAnimType_None, 0,	0},

	{"KF KF",		ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_KF_KF,					ONcAnimType_None, 0,	0},
	{"KF KF KF",	ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_KF_KF_KF,				ONcAnimType_None, 0,	0},

	{"KL KL",		ATTACK_FLAGS(Standing, Standing, Left),				ONcAnimType_KL_KL,					ONcAnimType_None, 0,	0},
	{"KL KL KL",	ATTACK_FLAGS(Standing, Standing, Left),				ONcAnimType_KL_KL_KL,				ONcAnimType_None, 0,	0},

	{"KR KR",		ATTACK_FLAGS(Standing, Standing, Right),			ONcAnimType_KR_KR,					ONcAnimType_None, 0,	0},
	{"KR KR KR",	ATTACK_FLAGS(Standing, Standing, Right),			ONcAnimType_KR_KR_KR,				ONcAnimType_None, 0,	0},

	{"KB KB",		ATTACK_FLAGS(Standing, Standing, Back),				ONcAnimType_KB_KB,					ONcAnimType_None, 0,	0},
	{"KB KB KB",	ATTACK_FLAGS(Standing, Standing, Back),				ONcAnimType_KB_KB_KB,				ONcAnimType_None, 0,	0},

	{"KD KD",		ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_KD_KD,					ONcAnimType_None, 0,	0},
	{"KD KD KD",	ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_KD_KD_KD,				ONcAnimType_None, 0,	0},

	{"P P K",		ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_PPK,					ONcAnimType_None, 0,	0},
	{"P K K",		ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_PKK,					ONcAnimType_None, 0,	0},
	{"P K P",		ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_PKP,					ONcAnimType_None, 0,	0},

	{"K P K",		ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_KPK,					ONcAnimType_None, 0,	0},
	{"K P P",		ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_KPP,					ONcAnimType_None, 0,	0},
	{"K K P",		ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_KKP,					ONcAnimType_None, 0,	0},

	{"P K",			ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_PK,						ONcAnimType_None, 0,	0},
	{"K P",			ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_KP,						ONcAnimType_None, 0,	0},

	{"P P K K",		ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_PPKK,					ONcAnimType_None, 0,	0},
	{"P P K K K",	ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_PPKKK,					ONcAnimType_None, 0,	0},
	{"P P K K K K",	ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_PPKKKK,					ONcAnimType_None, 0,	0},

	// super moves
	{"HP",			ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_Punch_Heavy,			ONcAnimType_None, 0,	0},
	{"HPF",			ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_Punch_Forward_Heavy,	ONcAnimType_None, 0,	0},

	{"HK",			ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_Kick_Heavy,				ONcAnimType_None, 0,	0},
	{"HKF",			ATTACK_FLAGS(Standing, Standing, Forwards),			ONcAnimType_Kick_Forward_Heavy,		ONcAnimType_None, 0,	0},

	// crouch-start moves
	{"CS-P",		ATTACK_FLAGS(CrouchStart, Standing, Forwards),		ONcAnimType_Punch,					ONcAnimType_None, 0,	0},
	{"CS-K",		ATTACK_FLAGS(CrouchStart, Standing, Forwards),		ONcAnimType_Kick,					ONcAnimType_None, 0,	0},

	// crouching moves
	{"C-P",			ATTACK_FLAGS(Crouching, Crouching, Forwards),		ONcAnimType_Punch,					ONcAnimType_None, 0,	0},
	{"C-P",			ATTACK_FLAGS(Crouching, Crouching, Forwards),		ONcAnimType_Punch_Low,				ONcAnimType_None, 0,	0},
	{"C-PF",		ATTACK_FLAGS(Crouching, Crouching, Forwards),		ONcAnimType_Punch_Forward,			ONcAnimType_None, 0,	0},
	{"C-K",			ATTACK_FLAGS(Crouching, Crouching, Forwards),		ONcAnimType_Kick,					ONcAnimType_None, 0,	0},
	{"C-K",			ATTACK_FLAGS(Crouching, Crouching, Forwards),		ONcAnimType_Kick_Low,				ONcAnimType_None, 0,	0},
	{"C-KF",		ATTACK_FLAGS(Crouching, Crouching, Forwards),		ONcAnimType_Kick_Forward,			ONcAnimType_None, 0,	0},

	// get-up kicks
	{"GETUP-KF",	ATTACK_FLAGS(Prone, Standing, Forwards),			ONcAnimType_Kick,					ONcAnimType_None, 0,	0},
	{"GETUP-KB",	ATTACK_FLAGS(Prone, Standing, Back),				ONcAnimType_Getup_Kick_Back,		ONcAnimType_None, 0,	0},

	// running moves
	{"R-P",			ATTACK_FLAGS(Running, Running, Forwards),			ONcAnimType_Run_Punch,				ONcAnimType_None, 0,	0},
	{"R-K",			ATTACK_FLAGS(Running, Running, Forwards),			ONcAnimType_Run_Kick,				ONcAnimType_None, 0,	0},

	{"RB-P",		ATTACK_FLAGS(RunningBack, RunningBack, Back),		ONcAnimType_Run_Back_Punch,			ONcAnimType_None, 0,	0},
	{"RB-K",		ATTACK_FLAGS(RunningBack, RunningBack, Back),		ONcAnimType_Run_Back_Kick,			ONcAnimType_None, 0,	0},

	{"RL-P",		ATTACK_FLAGS(RunningLeft, RunningLeft, Left),		ONcAnimType_Sidestep_Left_Punch,	ONcAnimType_None, 0,	0},
	{"RL-K",		ATTACK_FLAGS(RunningLeft, RunningLeft, Left),		ONcAnimType_Sidestep_Left_Kick,		ONcAnimType_None, 0,	0},

	{"RR-P",		ATTACK_FLAGS(RunningRight, RunningRight, Right),	ONcAnimType_Sidestep_Right_Punch,	ONcAnimType_None, 0,	0},
	{"RR-K",		ATTACK_FLAGS(RunningRight, RunningRight, Right),	ONcAnimType_Sidestep_Right_Kick,	ONcAnimType_None, 0,	0},

	{"R-SLIDE",		ATTACK_FLAGS(Running, Standing, Forwards),			ONcAnimType_Slide,					ONcAnimType_None, LIc_BitMask_Forward, LIc_BitMask_Crouch},

	// jumping moves
	{"J-P",			ATTACK_FLAGS(Jumping, Jumping, Forwards),			ONcAnimType_Punch,					ONcAnimType_None, 0,	0},
	{"J-K",			ATTACK_FLAGS(Jumping, Jumping, Forwards),			ONcAnimType_Kick,					ONcAnimType_None, 0,	0},

	{"JF-P",		ATTACK_FLAGS(JumpingForwards, JumpingForwards, Forwards),	ONcAnimType_Punch,			ONcAnimType_None, 0,	0},
	{"JF-PB",		ATTACK_FLAGS(JumpingForwards, JumpingForwards, Back),		ONcAnimType_Punch_Back,		ONcAnimType_None, 0,	0},
	{"JF-K",		ATTACK_FLAGS(JumpingForwards, JumpingForwards, Forwards),	ONcAnimType_Kick,			ONcAnimType_None, 0,	0},
	{"JF-KB",		ATTACK_FLAGS(JumpingForwards, JumpingForwards, Back),		ONcAnimType_Kick_Back,		ONcAnimType_None, 0,	0},

	{"JB-P",		ATTACK_FLAGS(JumpingBack, JumpingBack, Back),		ONcAnimType_Punch_Back,				ONcAnimType_None, 0,	0},
	{"JB-K",		ATTACK_FLAGS(JumpingBack, JumpingBack, Back),		ONcAnimType_Kick_Back,				ONcAnimType_None, 0,	0},

	{"JL-P",		ATTACK_FLAGS(JumpingLeft, JumpingLeft, Left),		ONcAnimType_Punch,					ONcAnimType_None, 0,	0},
	{"JL-K",		ATTACK_FLAGS(JumpingLeft, JumpingLeft, Left),		ONcAnimType_Kick,					ONcAnimType_None, 0,	0},

	{"JR-P",		ATTACK_FLAGS(JumpingRight, JumpingRight, Right),	ONcAnimType_Punch,					ONcAnimType_None, 0,	0},
	{"JR-K",		ATTACK_FLAGS(JumpingRight, JumpingRight, Right),	ONcAnimType_Kick,					ONcAnimType_None, 0,	0},

	{"",			(UUtUns32) -1,										0,									0,	0}
};

// *** melee state transitions

const char *AI2cMeleeMoveStateName[AI2cMeleeMoveState_Max] = {
	"<none>",
	"Stand",
	"Crouch",
	"Crouch Start",
	"Prone",
	"Run Start",
	"Run Back Start",
	"Run Left Start",
	"Run Right Start",
	"Run Forwards",
	"Run Back",
	"Run Left",
	"Run Right",
	"Jump",
	"Jump Forwards",
	"Jump Back",
	"Jump Left",
	"Jump Right"
};

#define STATELIST0			0, {AI2cMeleeMoveState_None,AI2cMeleeMoveState_None,	AI2cMeleeMoveState_None,	AI2cMeleeMoveState_None}
#define STATELIST1(a)		1, {AI2cMeleeMoveState_##a, AI2cMeleeMoveState_None,	AI2cMeleeMoveState_None,	AI2cMeleeMoveState_None}
#define STATELIST2(a,b)		2, {AI2cMeleeMoveState_##a, AI2cMeleeMoveState_##b,		AI2cMeleeMoveState_None,	AI2cMeleeMoveState_None}
#define STATELIST3(a,b,c)	3, {AI2cMeleeMoveState_##a, AI2cMeleeMoveState_##b,		AI2cMeleeMoveState_##c,		AI2cMeleeMoveState_None}
#define STATELIST4(a,b,c,d)	4, {AI2cMeleeMoveState_##a, AI2cMeleeMoveState_##b,		AI2cMeleeMoveState_##c,		AI2cMeleeMoveState_##d}

#define NEXT_TRANS			{TRcDirection_None, AI2cMeleeMoveState_None, UUcFalse, STATELIST0, ONcAnimType_None, 0, 0.0f}

// note that the Run*Start states never make an appearance in the statelists because they are never destination states
//
// transition weights: those from run-start have 1.0 weights because we don't want to inflict a double penalty on
//   run <-> stand transitions. most of the jumping transitions also have 1.0 weights because they will only ever
//   show up in the delay-end sequences of moves, where they aren't useful anyway. getting up from prone is weighted
//   way down so that the AI will tend to use its from-prone attacks wherever possible. the crouch-start position has no
//   penalty to and from stand so that super moves aren't penalised unduly.
//
//   also note that all 'preferred' transitions must come first in this table!
//   note that some transitions appear several times, as different characters have different animtypes (agh!)
//   note that JumpingForwards etc appear in the accelerate from running and runstart, and in both accelerated and decelerated from standing
static AI2tMeleeTransitionDefinition AI2gMeleeTransitionTable[] = {
	// standing jump
	{TRcDirection_Forwards,	AI2cMeleeMoveState_Standing,		UUcTrue,	STATELIST1(Jumping),								ONcAnimType_Jump,						LIc_BitMask_Jump,							0.7f},
	NEXT_TRANS,
	{TRcDirection_Forwards,	AI2cMeleeMoveState_Standing,		UUcTrue,	STATELIST1(JumpingForwards),						ONcAnimType_Jump,						LIc_BitMask_Forward		| LIc_BitMask_Jump,	0.7f},
	{TRcDirection_Forwards,	AI2cMeleeMoveState_Standing,		UUcTrue,	STATELIST1(JumpingForwards),						ONcAnimType_Jump_Forward,				LIc_BitMask_Forward		| LIc_BitMask_Jump,	0.7f},
	NEXT_TRANS,
	{TRcDirection_Left,		AI2cMeleeMoveState_Standing,		UUcTrue,	STATELIST1(JumpingLeft),							ONcAnimType_Jump,						LIc_BitMask_StepLeft	| LIc_BitMask_Jump,	0.7f},
	{TRcDirection_Left,		AI2cMeleeMoveState_Standing,		UUcTrue,	STATELIST1(JumpingLeft),							ONcAnimType_Jump_Left,					LIc_BitMask_StepLeft	| LIc_BitMask_Jump,	0.7f},
	NEXT_TRANS,
	{TRcDirection_Right,	AI2cMeleeMoveState_Standing,		UUcTrue,	STATELIST1(JumpingRight),							ONcAnimType_Jump,						LIc_BitMask_StepRight	| LIc_BitMask_Jump,	0.7f},
	{TRcDirection_Right,	AI2cMeleeMoveState_Standing,		UUcTrue,	STATELIST1(JumpingRight),							ONcAnimType_Jump_Right,					LIc_BitMask_StepRight	| LIc_BitMask_Jump,	0.7f},
	NEXT_TRANS,
	{TRcDirection_Back,		AI2cMeleeMoveState_Standing,		UUcTrue,	STATELIST1(JumpingBack),							ONcAnimType_Jump,						LIc_BitMask_Backward	| LIc_BitMask_Jump,	0.7f},
	{TRcDirection_Back,		AI2cMeleeMoveState_Standing,		UUcTrue,	STATELIST1(JumpingBack),							ONcAnimType_Jump_Backward,				LIc_BitMask_Backward	| LIc_BitMask_Jump,	0.7f},
	NEXT_TRANS,

	// running jump
	{TRcDirection_Forwards,	AI2cMeleeMoveState_Running,			UUcTrue,	STATELIST1(JumpingForwards),						ONcAnimType_Jump,						LIc_BitMask_Forward		| LIc_BitMask_Jump,	0.7f},
	{TRcDirection_Forwards,	AI2cMeleeMoveState_Running,			UUcTrue,	STATELIST1(JumpingForwards),						ONcAnimType_Jump_Forward,				LIc_BitMask_Forward		| LIc_BitMask_Jump,	0.7f},
	NEXT_TRANS,
	{TRcDirection_Left,		AI2cMeleeMoveState_RunningLeft,		UUcTrue,	STATELIST1(JumpingLeft),							ONcAnimType_Jump,						LIc_BitMask_StepLeft	| LIc_BitMask_Jump,	0.7f},
	{TRcDirection_Left,		AI2cMeleeMoveState_RunningLeft,		UUcTrue,	STATELIST1(JumpingLeft),							ONcAnimType_Jump_Left,					LIc_BitMask_StepLeft	| LIc_BitMask_Jump,	0.7f},
	NEXT_TRANS,
	{TRcDirection_Right,	AI2cMeleeMoveState_RunningRight,	UUcTrue,	STATELIST1(JumpingRight),							ONcAnimType_Jump,						LIc_BitMask_StepRight	| LIc_BitMask_Jump,	0.7f},
	{TRcDirection_Right,	AI2cMeleeMoveState_RunningRight,	UUcTrue,	STATELIST1(JumpingRight),							ONcAnimType_Jump_Right,					LIc_BitMask_StepRight	| LIc_BitMask_Jump,	0.7f},
	NEXT_TRANS,
	{TRcDirection_Back,		AI2cMeleeMoveState_RunningBack,		UUcTrue,	STATELIST1(JumpingBack),							ONcAnimType_Jump,						LIc_BitMask_Backward	| LIc_BitMask_Jump,	0.7f},
	{TRcDirection_Back,		AI2cMeleeMoveState_RunningBack,		UUcTrue,	STATELIST1(JumpingBack),							ONcAnimType_Jump_Backward,				LIc_BitMask_Backward	| LIc_BitMask_Jump,	0.7f},
	NEXT_TRANS,

	// get up (can lead to running)
	{TRcDirection_Forwards,	AI2cMeleeMoveState_Prone,			UUcTrue,	STATELIST2(Standing,	Running),					ONcAnimType_Run,						LIc_BitMask_Forward,						0.2f},
	NEXT_TRANS,
	{TRcDirection_Left,		AI2cMeleeMoveState_Prone,			UUcTrue,	STATELIST2(Standing,	RunningLeft),				ONcAnimType_Standing_Turn_Left,			LIc_BitMask_TurnLeft,						0.2f},
	NEXT_TRANS,
	{TRcDirection_Right,	AI2cMeleeMoveState_Prone,			UUcTrue,	STATELIST2(Standing,	RunningRight),				ONcAnimType_Standing_Turn_Right,		LIc_BitMask_TurnRight,						0.2f},
	NEXT_TRANS,
	{TRcDirection_Back,		AI2cMeleeMoveState_Prone,			UUcTrue,	STATELIST2(Standing,	RunningBack),				ONcAnimType_Run_Backwards,				LIc_BitMask_Backward,						0.2f},
	NEXT_TRANS,

	// start running (leads to run and possibly to a running jump)
	{TRcDirection_Forwards,	AI2cMeleeMoveState_Standing,		UUcTrue,	STATELIST2(Running,		JumpingForwards),			ONcAnimType_Run,						LIc_BitMask_Forward,						0.7f},
	{TRcDirection_Forwards,	AI2cMeleeMoveState_Standing,		UUcTrue,	STATELIST2(Running,		JumpingForwards),			ONcAnimType_Run_Start,					LIc_BitMask_Forward,						0.7f},
	NEXT_TRANS,
	{TRcDirection_Left,		AI2cMeleeMoveState_Standing,		UUcTrue,	STATELIST2(RunningLeft,	JumpingLeft),				ONcAnimType_Run_Sidestep_Left,			LIc_BitMask_StepLeft,						0.7f},
	{TRcDirection_Left,		AI2cMeleeMoveState_Standing,		UUcTrue,	STATELIST2(RunningLeft,	JumpingLeft),				ONcAnimType_Run_Sidestep_Left_Start,	LIc_BitMask_StepLeft,						0.7f},
	NEXT_TRANS,
	{TRcDirection_Right,	AI2cMeleeMoveState_Standing,		UUcTrue,	STATELIST2(RunningRight,JumpingRight),				ONcAnimType_Run_Sidestep_Right,			LIc_BitMask_StepRight,						0.7f},
	{TRcDirection_Right,	AI2cMeleeMoveState_Standing,		UUcTrue,	STATELIST2(RunningRight,JumpingRight),				ONcAnimType_Run_Sidestep_Right_Start,	LIc_BitMask_StepRight,						0.7f},
	NEXT_TRANS,
	{TRcDirection_Back,		AI2cMeleeMoveState_Standing,		UUcTrue,	STATELIST2(RunningBack,	JumpingBack),				ONcAnimType_Run_Backwards,				LIc_BitMask_Backward,						0.7f},
	{TRcDirection_Back,		AI2cMeleeMoveState_Standing,		UUcTrue,	STATELIST2(RunningBack,	JumpingBack),				ONcAnimType_Run_Backwards_Start,		LIc_BitMask_Backward,						0.7f},
	NEXT_TRANS,

	// accelerate (leads possibly to a running jump)
	{TRcDirection_Forwards,	AI2cMeleeMoveState_RunStart,		UUcTrue,	STATELIST2(Running,		JumpingForwards),			ONcAnimType_Run,						LIc_BitMask_Forward,						1.0f},
	{TRcDirection_Forwards,	AI2cMeleeMoveState_RunStart,		UUcTrue,	STATELIST2(Running,		JumpingForwards),			ONcAnimType_Run_Start,					LIc_BitMask_Forward,						1.0f},
	NEXT_TRANS,
	{TRcDirection_Left,		AI2cMeleeMoveState_RunLeftStart,	UUcFalse,	STATELIST2(RunningLeft,	JumpingLeft),				ONcAnimType_Run_Sidestep_Left,			LIc_BitMask_StepLeft,						1.0f},
	{TRcDirection_Left,		AI2cMeleeMoveState_RunLeftStart,	UUcFalse,	STATELIST2(RunningLeft,	JumpingLeft),				ONcAnimType_Run_Sidestep_Left_Start,	LIc_BitMask_StepLeft,						1.0f},
	NEXT_TRANS,
	{TRcDirection_Right,	AI2cMeleeMoveState_RunRightStart,	UUcFalse,	STATELIST2(RunningRight,JumpingRight),				ONcAnimType_Run_Sidestep_Right,			LIc_BitMask_StepRight,						1.0f},
	{TRcDirection_Right,	AI2cMeleeMoveState_RunRightStart,	UUcFalse,	STATELIST2(RunningRight,JumpingRight),				ONcAnimType_Run_Sidestep_Right_Start,	LIc_BitMask_StepRight,						1.0f},
	NEXT_TRANS,
	{TRcDirection_Back,		AI2cMeleeMoveState_RunBackStart,	UUcTrue,	STATELIST2(RunningBack,	JumpingBack),				ONcAnimType_Run_Backwards,				LIc_BitMask_Backward,						1.0f},
	{TRcDirection_Back,		AI2cMeleeMoveState_RunBackStart,	UUcTrue,	STATELIST2(RunningBack,	JumpingBack),				ONcAnimType_Run_Backwards_Start,		LIc_BitMask_Backward,						1.0f},
	NEXT_TRANS,

	// start slowing from a run (leads possibly to a crouchstart super move, or a crouch, or a standing jump)
	{TRcDirection_Forwards,	AI2cMeleeMoveState_Running,			UUcTrue,	STATELIST4(Standing,	CrouchStart,	Crouching,	Jumping),	ONcAnimType_Stand,			0,											0.7f},
	{TRcDirection_Forwards,	AI2cMeleeMoveState_Running,			UUcTrue,	STATELIST4(Standing,	CrouchStart,	Crouching,	Jumping),	ONcAnimType_Run_Stop,		0,											0.7f},
	NEXT_TRANS,
	{TRcDirection_Left,		AI2cMeleeMoveState_RunningLeft,		UUcTrue,	STATELIST4(Standing,	CrouchStart,	Crouching,	Jumping),	ONcAnimType_Stand,			0,											0.7f},
	{TRcDirection_Left,		AI2cMeleeMoveState_RunningLeft,		UUcTrue,	STATELIST4(Standing,	CrouchStart,	Crouching,	Jumping),	ONcAnimType_Run_Stop,		0,											0.7f},
	NEXT_TRANS,
	{TRcDirection_Right,	AI2cMeleeMoveState_RunningRight,	UUcTrue,	STATELIST4(Standing,	CrouchStart,	Crouching,	Jumping),	ONcAnimType_Stand,			0,											0.7f},
	{TRcDirection_Right,	AI2cMeleeMoveState_RunningRight,	UUcTrue,	STATELIST4(Standing,	CrouchStart,	Crouching,	Jumping),	ONcAnimType_Run_Stop,		0,											0.7f},
	NEXT_TRANS,
	{TRcDirection_Back,		AI2cMeleeMoveState_RunningBack,		UUcTrue,	STATELIST4(Standing,	CrouchStart,	Crouching,	Jumping),	ONcAnimType_Stand,			0,											0.7f},
	{TRcDirection_Back,		AI2cMeleeMoveState_RunningBack,		UUcTrue,	STATELIST4(Standing,	CrouchStart,	Crouching,	Jumping),	ONcAnimType_Run_Stop,		0,											0.7f},
	NEXT_TRANS,

	// decelerate to a stop (leads possibly to a crouchstart super move, or a crouch, or a standing jump)
	{TRcDirection_Forwards,	AI2cMeleeMoveState_RunStart,		UUcTrue,	STATELIST4(Standing,	CrouchStart,	Crouching,	Jumping),	ONcAnimType_Stand,			0,											1.0f},
	{TRcDirection_Forwards,	AI2cMeleeMoveState_RunStart,		UUcTrue,	STATELIST4(Standing,	CrouchStart,	Crouching,	Jumping),	ONcAnimType_Run_Stop,		0,											1.0f},
	{TRcDirection_Forwards,	AI2cMeleeMoveState_RunStart,		UUcTrue,	STATELIST4(Standing,	CrouchStart,	Crouching,	Jumping),	ONcAnimType_1Step_Stop,		0,											1.0f},
	NEXT_TRANS,
	{TRcDirection_Left,		AI2cMeleeMoveState_RunLeftStart,	UUcFalse,	STATELIST4(Standing,	CrouchStart,	Crouching,	Jumping),	ONcAnimType_Stand,			0,											1.0f},
	{TRcDirection_Left,		AI2cMeleeMoveState_RunLeftStart,	UUcFalse,	STATELIST4(Standing,	CrouchStart,	Crouching,	Jumping),	ONcAnimType_Run_Stop,		0,											1.0f},
	{TRcDirection_Left,		AI2cMeleeMoveState_RunLeftStart,	UUcFalse,	STATELIST4(Standing,	CrouchStart,	Crouching,	Jumping),	ONcAnimType_1Step_Stop,		0,											1.0f},
	NEXT_TRANS,
	{TRcDirection_Right,	AI2cMeleeMoveState_RunRightStart,	UUcFalse,	STATELIST4(Standing,	CrouchStart,	Crouching,	Jumping),	ONcAnimType_Stand,			0,											1.0f},
	{TRcDirection_Right,	AI2cMeleeMoveState_RunRightStart,	UUcFalse,	STATELIST4(Standing,	CrouchStart,	Crouching,	Jumping),	ONcAnimType_Run_Stop,		0,											1.0f},
	{TRcDirection_Right,	AI2cMeleeMoveState_RunRightStart,	UUcFalse,	STATELIST4(Standing,	CrouchStart,	Crouching,	Jumping),	ONcAnimType_1Step_Stop,		0,											1.0f},
	NEXT_TRANS,
	{TRcDirection_Back,		AI2cMeleeMoveState_RunBackStart,	UUcTrue,	STATELIST4(Standing,	CrouchStart,	Crouching,	Jumping),	ONcAnimType_Stand,			0,											1.0f},
	{TRcDirection_Back,		AI2cMeleeMoveState_RunBackStart,	UUcTrue,	STATELIST4(Standing,	CrouchStart,	Crouching,	Jumping),	ONcAnimType_Run_Stop,		0,											1.0f},
	{TRcDirection_Back,		AI2cMeleeMoveState_RunBackStart,	UUcTrue,	STATELIST4(Standing,	CrouchStart,	Crouching,	Jumping),	ONcAnimType_1Step_Stop,		0,											1.0f},
	NEXT_TRANS,

	// crouching
	{TRcDirection_Forwards,	AI2cMeleeMoveState_Standing,		UUcTrue,	STATELIST2(CrouchStart,	Crouching),					ONcAnimType_Crouch,				LIc_BitMask_Crouch,							1.0f},
	NEXT_TRANS,
	{TRcDirection_Forwards,	AI2cMeleeMoveState_CrouchStart,		UUcTrue,	STATELIST1(Crouching),								ONcAnimType_Crouch,				LIc_BitMask_Crouch,							0.5f},
	NEXT_TRANS,

	// standing from a crouch (can lead to running)
	{TRcDirection_Forwards,	AI2cMeleeMoveState_Crouching,		UUcTrue,	STATELIST4(CrouchStart,	Standing, Running, JumpingForwards),ONcAnimType_Stand,		0,											0.8f},
	NEXT_TRANS,
	{TRcDirection_Left,		AI2cMeleeMoveState_Crouching,		UUcTrue,	STATELIST4(CrouchStart,	Standing, Running, JumpingForwards),ONcAnimType_Stand,		0,											0.8f},
	NEXT_TRANS,
	{TRcDirection_Right,	AI2cMeleeMoveState_Crouching,		UUcTrue,	STATELIST4(CrouchStart,	Standing, Running, JumpingForwards),ONcAnimType_Stand,		0,											0.8f},
	NEXT_TRANS,
	{TRcDirection_Back,		AI2cMeleeMoveState_Crouching,		UUcTrue,	STATELIST4(CrouchStart,	Standing, Running, JumpingForwards),ONcAnimType_Stand,		0,											0.8f},
	NEXT_TRANS,
	{TRcDirection_Forwards,	AI2cMeleeMoveState_Crouching,		UUcTrue,	STATELIST2(Standing,	Jumping),					ONcAnimType_Stand,				0,											1.0f},
	NEXT_TRANS,

	{TRcDirection_Forwards,	AI2cMeleeMoveState_CrouchStart,		UUcTrue,	STATELIST3(Standing, Running, JumpingForwards),		ONcAnimType_Stand,				0,											0.8f},
	NEXT_TRANS,
	{TRcDirection_Left,		AI2cMeleeMoveState_CrouchStart,		UUcTrue,	STATELIST3(Standing, Running, JumpingForwards),		ONcAnimType_Stand,				0,											0.8f},
	NEXT_TRANS,
	{TRcDirection_Right,	AI2cMeleeMoveState_CrouchStart,		UUcTrue,	STATELIST3(Standing, Running, JumpingForwards),		ONcAnimType_Stand,				0,											0.8f},
	NEXT_TRANS,
	{TRcDirection_Back,		AI2cMeleeMoveState_CrouchStart,		UUcTrue,	STATELIST3(Standing, Running, JumpingForwards),		ONcAnimType_Stand,				0,											0.8f},
	NEXT_TRANS,
	{TRcDirection_Forwards,	AI2cMeleeMoveState_CrouchStart,		UUcTrue,	STATELIST2(Standing,	Jumping),					ONcAnimType_Stand,				0,											1.0f},
	NEXT_TRANS,

	// end of table
	NEXT_TRANS
};

#define AI2cMeleeMaxPositionMoves		20

AI2tMelee_PositionMove AI2gMeleePositionTable[AI2cMeleeMaxPositionMoves] = {
	// position for running attacks
	{"Run Forward",		TRcDirection_Forwards,	UUcTrue,	UUcTrue,	UUcFalse,	AI2cMeleeJumpType_None,			STATELIST1(Running)},
	{"Run Left",		TRcDirection_Left,		UUcTrue,	UUcTrue,	UUcFalse,	AI2cMeleeJumpType_None,			STATELIST1(RunningLeft)},
	{"Run Right",		TRcDirection_Right,		UUcTrue,	UUcTrue,	UUcFalse,	AI2cMeleeJumpType_None,			STATELIST1(RunningRight)},
	{"Run Back",		TRcDirection_Back,		UUcTrue,	UUcTrue,	UUcFalse,	AI2cMeleeJumpType_None,			STATELIST1(RunningBack)},

	// jumping attacks from standing
	{"Jump Up",			TRcDirection_None,		UUcFalse,	UUcFalse,	UUcFalse,	AI2cMeleeJumpType_Vertical,		STATELIST2(Standing,	Jumping)},
	{"Jump Forward",	TRcDirection_Forwards,	UUcFalse,	UUcFalse,	UUcFalse,	AI2cMeleeJumpType_Standing,		STATELIST1(JumpingForwards)},
	{"Jump Left",		TRcDirection_Left,		UUcFalse,	UUcFalse,	UUcFalse,	AI2cMeleeJumpType_Standing,		STATELIST1(JumpingLeft)},
	{"Jump Right",		TRcDirection_Right,		UUcFalse,	UUcFalse,	UUcFalse,	AI2cMeleeJumpType_Standing,		STATELIST1(JumpingRight)},
	{"Jump Back",		TRcDirection_Back,		UUcFalse,	UUcFalse,	UUcFalse,	AI2cMeleeJumpType_Standing,		STATELIST1(JumpingBack)},

	// position for super moves
	{"Start to Crouch",	TRcDirection_None,		UUcFalse,	UUcFalse,	UUcFalse,	AI2cMeleeJumpType_None,			STATELIST1(CrouchStart)},

	// position for crouching moves
	{"Crouch",			TRcDirection_None,		UUcFalse,	UUcTrue,	UUcFalse,	AI2cMeleeJumpType_None,			STATELIST1(Crouching)},

	// position for standing attacks
	{"Stand",			TRcDirection_None,		UUcFalse,	UUcTrue,	UUcFalse,	AI2cMeleeJumpType_None,			STATELIST1(Standing)},
	{"Close Forward",	TRcDirection_Forwards,	UUcTrue,	UUcTrue,	UUcTrue,	AI2cMeleeJumpType_None,			STATELIST2(Running,		Standing)},
	{"Close Left",		TRcDirection_Left,		UUcTrue,	UUcTrue,	UUcTrue,	AI2cMeleeJumpType_None,			STATELIST2(RunningLeft,	Standing)},
	{"Close Right",		TRcDirection_Right,		UUcTrue,	UUcTrue,	UUcTrue,	AI2cMeleeJumpType_None,			STATELIST2(RunningRight,Standing)},
	{"Close Back",		TRcDirection_Back,		UUcTrue,	UUcTrue,	UUcTrue,	AI2cMeleeJumpType_None,			STATELIST2(RunningBack,	Standing)},

	// charging jumping attacks
	{"RunJump Forward",	TRcDirection_Forwards,	UUcTrue,	UUcFalse,	UUcFalse,	AI2cMeleeJumpType_RunForward,	STATELIST2(Running,		JumpingForwards)},
	{"RunJump Left",	TRcDirection_Left,		UUcTrue,	UUcFalse,	UUcFalse,	AI2cMeleeJumpType_RunLeft,		STATELIST2(RunningLeft,	JumpingLeft)},
	{"RunJump Right",	TRcDirection_Right,		UUcTrue,	UUcFalse,	UUcFalse,	AI2cMeleeJumpType_RunRight,		STATELIST2(RunningRight,JumpingRight)},
	{"RunJump Back",	TRcDirection_Back,		UUcTrue,	UUcFalse,	UUcFalse,	AI2cMeleeJumpType_RunBack,		STATELIST2(RunningBack,	JumpingBack)}
};

// *** kinds of maneuvers
enum {
	AI2cMeleeManeuver_Advance		= 0,
	AI2cMeleeManeuver_Retreat		= 1,
	AI2cMeleeManeuver_CircleLeft	= 2,
	AI2cMeleeManeuver_CircleRight	= 3,
	AI2cMeleeManeuver_Pause			= 4,
	AI2cMeleeManeuver_Crouch		= 5,
	AI2cMeleeManeuver_Jump			= 6,
	AI2cMeleeManeuver_Taunt			= 7,
	AI2cMeleeManeuver_RandomStop	= 8,
	AI2cMeleeManeuver_GetupFwd		= 9,
	AI2cMeleeManeuver_GetupBack		= 10,
	AI2cMeleeManeuver_GetupLeft		= 11,
	AI2cMeleeManeuver_GetupRight	= 12,
	AI2cMeleeManeuver_BarabbasWave	= 13,

	AI2cMeleeManeuver_Max			= 14
};

typedef struct AI2tMeleeManeuverDefinition {
	char *				name;

	UUtUns32			num_normal_params;			// note: these are the params not counting param[0] which is stored for
	char *				normal_param_names[2];		// every maneuver move, and is the max time it will execute for.
	UUtUns32			num_spacing_params;
	char *				spacing_param_names[2];

	UUtBool				is_maneuver;
	TRtDirection		maneuver_direction;
	LItButtonBits		nonmaneuver_keys;

	AI2tMeleeMoveState	from_state;
	AI2tMeleeMoveState	to_state;
	UUtUns16			anim_type;
} AI2tMeleeManeuverDefinition;

AI2tMeleeManeuverDefinition AI2gMeleeManeuverDefinition[AI2cMeleeManeuver_Max] = {
	{"Advance",			2,	{"Min Range", "Threshold Range"},	2,	{"Target Range", "Tolerance Dist"},UUcTrue,	TRcDirection_Forwards,0,AI2cMeleeMoveState_None,	AI2cMeleeMoveState_Standing,	ONcAnimType_Run},
	{"Retreat",			2,	{"Max Range", "Threshold Range"},	2,	{"Target Range", "Tolerance Dist"},UUcTrue,	TRcDirection_Back,	0,	AI2cMeleeMoveState_None,	AI2cMeleeMoveState_Standing,	ONcAnimType_Run_Backwards},
	{"Circle Left",		2,	{"Min Angle", "Max Angle"},			1,	{"Closeness Angle", ""},		UUcTrue,	TRcDirection_Left,	0,	AI2cMeleeMoveState_None,	AI2cMeleeMoveState_Standing,	ONcAnimType_Run_Sidestep_Left},
	{"Circle Right",	2,	{"Min Angle", "Max Angle"},			1,	{"Closeness Angle",	""},		UUcTrue,	TRcDirection_Right,	0,	AI2cMeleeMoveState_None,	AI2cMeleeMoveState_Standing,	ONcAnimType_Run_Sidestep_Right},
	{"Pause",			0,	{"", ""},							0,	{"", ""},						UUcFalse,	0, 0,					AI2cMeleeMoveState_None,	AI2cMeleeMoveState_Standing,	ONcAnimType_Stand},
	{"Crouch",			0,	{"", ""},							0,	{"", ""},						UUcFalse,	0, LIc_BitMask_Crouch,	AI2cMeleeMoveState_Standing,AI2cMeleeMoveState_Crouching,	ONcAnimType_Crouch},
	{"Jump",			0,	{"", ""},							0,	{"", ""},						UUcFalse,	0, LIc_BitMask_Jump,	AI2cMeleeMoveState_None,	AI2cMeleeMoveState_Jumping,		ONcAnimType_Jump},
	{"Taunt",			0,	{"", ""},							0,	{"", ""},						UUcFalse,	0, LIc_BitMask_Action,	AI2cMeleeMoveState_None,	AI2cMeleeMoveState_Standing,	ONcAnimType_Taunt},
	{"Random Stop",		0,	{"", ""},							0,	{"", ""},						UUcFalse,	0, 0,					AI2cMeleeMoveState_None,	AI2cMeleeMoveState_Standing,	ONcAnimType_Stand},
	{"Get-up Fwd",		0,	{"", ""},							0,	{"", ""},						UUcFalse,	0, LIc_BitMask_Forward,	AI2cMeleeMoveState_Prone,	AI2cMeleeMoveState_Standing,	ONcAnimType_Run},
	{"Get-up Back",		0,	{"", ""},							0,	{"", ""},						UUcFalse,	0, LIc_BitMask_Backward,AI2cMeleeMoveState_Prone,	AI2cMeleeMoveState_Standing,	ONcAnimType_Run_Backwards},
	{"Get-up RollLeft",	0,	{"", ""},							0,	{"", ""},						UUcFalse,	0, LIc_BitMask_TurnLeft,AI2cMeleeMoveState_Prone,	AI2cMeleeMoveState_Standing,	ONcAnimType_Standing_Turn_Left},
	{"Get-up RollRight",0,	{"", ""},							0,	{"", ""},						UUcFalse,	0, LIc_BitMask_TurnRight,AI2cMeleeMoveState_Prone,	AI2cMeleeMoveState_Standing,	ONcAnimType_Standing_Turn_Right},
	{"Barabbas Wave",	1,	{"", ""},							0,	{"", ""},						UUcFalse,	0, LIc_BitMask_Kick,	AI2cMeleeMoveState_CrouchStart,	AI2cMeleeMoveState_Standing,ONcAnimType_Kick}
};

// *** melee evasion table

#define AI2gNumMeleeEvasions 24
AI2tMelee_EvadeMove AI2gMeleeEvadeTable[AI2gNumMeleeEvasions] = {
	{"Jump Forwards",	EVADE_FLAGS(Standing,		JumpingForwards,	Jump),				LIc_BitMask_Forward		| LIc_BitMask_Jump,	0},
	{"Jump Forwards",	EVADE_FLAGS(Standing,		JumpingForwards,	Jump_Forward),		LIc_BitMask_Forward		| LIc_BitMask_Jump,	0},
	{"Jump Backards",	EVADE_FLAGS(Standing,		JumpingBack,		Jump),				LIc_BitMask_Backward	| LIc_BitMask_Jump,	0},
	{"Jump Backards",	EVADE_FLAGS(Standing,		JumpingBack,		Jump_Backward),		LIc_BitMask_Backward	| LIc_BitMask_Jump,	0},
	{"Jump Left",		EVADE_FLAGS(Standing,		JumpingLeft,		Jump),				LIc_BitMask_StepLeft	| LIc_BitMask_Jump,	0},
	{"Jump Left",		EVADE_FLAGS(Standing,		JumpingLeft,		Jump_Left),			LIc_BitMask_StepLeft	| LIc_BitMask_Jump,	0},
	{"Jump Right",		EVADE_FLAGS(Standing,		JumpingRight,		Jump),				LIc_BitMask_StepRight	| LIc_BitMask_Jump,	0},
	{"Jump Right",		EVADE_FLAGS(Standing,		JumpingRight,		Jump_Right),		LIc_BitMask_StepRight	| LIc_BitMask_Jump,	0},

	{"RunJump Forwards",EVADE_FLAGS(Running,		JumpingForwards,	Jump),				LIc_BitMask_Jump,		LIc_BitMask_Forward},
	{"RunJump Forwards",EVADE_FLAGS(Running,		JumpingForwards,	Jump_Forward),		LIc_BitMask_Jump,		LIc_BitMask_Forward},
	{"RunJump Backards",EVADE_FLAGS(RunningBack,	JumpingBack,		Jump),				LIc_BitMask_Jump,		LIc_BitMask_Backward},
	{"RunJump Backards",EVADE_FLAGS(RunningBack,	JumpingBack,		Jump_Backward),		LIc_BitMask_Jump,		LIc_BitMask_Backward},
	{"RunJump Left",	EVADE_FLAGS(RunningLeft,	JumpingLeft,		Jump),				LIc_BitMask_Jump,		LIc_BitMask_StepLeft},
	{"RunJump Left",	EVADE_FLAGS(RunningLeft,	JumpingLeft,		Jump_Left),			LIc_BitMask_Jump,		LIc_BitMask_StepLeft},
	{"RunJump Right",	EVADE_FLAGS(RunningRight,	JumpingRight,		Jump),				LIc_BitMask_Jump,		LIc_BitMask_StepRight},
	{"RunJump Right",	EVADE_FLAGS(RunningRight,	JumpingRight,		Jump_Right),		LIc_BitMask_Jump,		LIc_BitMask_StepRight},

	{"Roll Forwards",	EVADE_FLAGS(Standing,		Standing,			Crouch_Forward),	LIc_BitMask_Forward,	LIc_BitMask_Crouch},
	{"Roll Backards",	EVADE_FLAGS(Standing,		Standing,			Crouch_Back),		LIc_BitMask_Backward,	LIc_BitMask_Crouch},
	{"Roll Left",		EVADE_FLAGS(Standing,		Standing,			Crouch_Left),		LIc_BitMask_StepLeft,	LIc_BitMask_Crouch},
	{"Roll Right",		EVADE_FLAGS(Standing,		Standing,			Crouch_Right),		LIc_BitMask_StepRight,	LIc_BitMask_Crouch},

	{"Slide Forwards",	EVADE_FLAGS(Running,		Running,			Slide),				LIc_BitMask_Forward,	LIc_BitMask_Crouch},
	{"Slide Backards",	EVADE_FLAGS(RunningBack,	RunningBack,		Slide),				LIc_BitMask_Backward,	LIc_BitMask_Crouch},
	{"Slide Left",		EVADE_FLAGS(RunningLeft,	RunningLeft,		Slide),				LIc_BitMask_StepLeft,	LIc_BitMask_Crouch},
	{"Slide Right",		EVADE_FLAGS(RunningRight,	RunningRight,		Slide),				LIc_BitMask_StepRight,	LIc_BitMask_Crouch}
};

// *** melee throw table

#define AI2cNumMeleeThrows 25
AI2tMelee_ThrowMove AI2gMeleeThrowTable[AI2cNumMeleeThrows] = {
	{"Throw-P Front",				THROW_FLAGS(Throw_Forward_Punch,	Front,	Standing,	No,		Standing)},
	{"Throw-K Front",				THROW_FLAGS(Throw_Forward_Kick,		Front,	Standing,	No,		Standing)},
	{"Throw-P Behind",				THROW_FLAGS(Throw_Behind_Punch,		Behind,	Standing,	No,		Standing)},
	{"Throw-K Behind",				THROW_FLAGS(Throw_Behind_Kick,		Behind,	Standing,	No,		Standing)},

	{"Throw-RP Front",				THROW_FLAGS(Run_Throw_Forward_Punch,Front,	Running,	No,		Standing)},
	{"Throw-RK Front",				THROW_FLAGS(Run_Throw_Forward_Kick,	Front,	Running,	No,		Standing)},
	{"Throw-RP Behind",				THROW_FLAGS(Run_Throw_Behind_Punch,	Behind,	Running,	No,		Standing)},
	{"Throw-RK Behind",				THROW_FLAGS(Run_Throw_Behind_Kick,	Behind,	Running,	No,		Standing)},

	{"Throw-P Front Disarm",		THROW_FLAGS(Throw_Forward_Punch,	Front,	Standing,	Pistol,	Standing)},
	{"Throw-K Front Disarm",		THROW_FLAGS(Throw_Forward_Kick,		Front,	Standing,	Pistol,	Standing)},
	{"Throw-P Behind Disarm",		THROW_FLAGS(Throw_Behind_Punch,		Behind,	Standing,	Pistol,	Standing)},
	{"Throw-K Behind Disarm",		THROW_FLAGS(Throw_Behind_Kick,		Behind,	Standing,	Pistol,	Standing)},

	{"Throw-RP Front Disarm",		THROW_FLAGS(Run_Throw_Forward_Punch,Front,	Running,	Pistol,	Standing)},
	{"Throw-RK Front Disarm",		THROW_FLAGS(Run_Throw_Forward_Kick,	Front,	Running,	Pistol,	Standing)},
	{"Throw-RP Behind Disarm",		THROW_FLAGS(Run_Throw_Behind_Punch,	Behind,	Running,	Pistol,	Standing)},
	{"Throw-RK Behind Disarm",		THROW_FLAGS(Run_Throw_Behind_Kick,	Behind,	Running,	Pistol,	Standing)},

	{"Throw-P Front RifDisarm",		THROW_FLAGS(Throw_Forward_Punch,	Front,	Standing,	Rifle,	Standing)},
	{"Throw-K Front RifDisarm",		THROW_FLAGS(Throw_Forward_Kick,		Front,	Standing,	Rifle,	Standing)},
	{"Throw-P Behind RifDisarm",	THROW_FLAGS(Throw_Behind_Punch,		Behind,	Standing,	Rifle,	Standing)},
	{"Throw-K Behind RifDisarm",	THROW_FLAGS(Throw_Behind_Kick,		Behind,	Standing,	Rifle,	Standing)},

	{"Throw-RP Front RifDisarm",	THROW_FLAGS(Run_Throw_Forward_Punch,Front,	Running,	Rifle,	Standing)},
	{"Throw-RK Front RifDisarm",	THROW_FLAGS(Run_Throw_Forward_Kick,	Front,	Running,	Rifle,	Standing)},
	{"Throw-RP Behind RifDisarm",	THROW_FLAGS(Run_Throw_Behind_Punch,Behind,	Running,	Rifle,	Standing)},
	{"Throw-RK Behind RifDisarm",	THROW_FLAGS(Run_Throw_Behind_Kick,	Behind,	Running,	Rifle,	Standing)},

	{"Tackle",						THROW_FLAGS(Run_Throw_Behind_Punch,	Behind,	Running,	No,		Running)}
};

// *** weights for all current actions or reactions
typedef struct AI2tConsideredTechnique {
	AI2tMeleeTechnique	*technique;
	float				weight;

	// only valid for actions
	UUtUns32			attack_moveindex;
	AI2tMeleeMove		*attack_move;
	UUtBool				ends_in_jump;
	UUtBool				attack_before_hit;
	float				jump_velocity;
	UUtUns16			num_jump_frames;
	UUtUns16			positionmove_skip;

	// temporary storage for the intersection checking
	float				sin_rotate, cos_rotate;
	float				action_location_offset;
	TRtDirection		rotation_direction;
} AI2tConsideredTechnique;

AI2tConsideredTechnique AI2gMeleeCurrentChoices[AI2cMeleeMaxTechniques];

// *** debugging status definitions
#define AI2cMelee_StatusLines		16
UUtBool				AI2gMelee_ShowStatus;
UUtBool				AI2gMelee_ShowCurrentProfile;
UUtUns32			AI2gMelee_NextStatusLine;
AI2tMeleeProfile *	AI2gMelee_ShowProfile;
char				AI2gMelee_TechniqueWeightDescription[64];
COtStatusLine		AI2gMelee_StatusLine[AI2cMelee_StatusLines + 2];

// last pathfinding error
UUtUns32			AI2gMelee_PathfindingError;

// ------------------------------------------------------------------------------------
// -- initialization and termination

UUtError AI2rMelee_Initialize(void)
{
	AI2tMelee_AttackMove *attack_move;
	const AI2tMeleeStateTranslator *translator;
	ONtMoveLookup *move;

	/*
	 * debugging display is not enabled
	 */

	AI2gMelee_ShowStatus = UUcFalse;
	AI2gMelee_ShowProfile = NULL;
	COrConsole_StatusLines_Begin(AI2gMelee_StatusLine, &AI2gMelee_ShowStatus, AI2cMelee_StatusLines + 2);

	/*
	 * build the melee attack table from the global combo list
	 */

	UUrMemory_Clear(AI2gMeleeAttackTable, AI2gNumMeleeAttacks * sizeof(AI2tMelee_AttackMove));

	for (AI2gNumMeleeAttacks = 0, attack_move = AI2gMeleeAttackTable; attack_move->attackmove_flags != (UUtUns32) -1;
			AI2gNumMeleeAttacks++, attack_move++) {

		// look for this animtype in the global combo table
		for (move = ONgComboTable; move->newMask != 0; move++) {
			if (move->moveType == attack_move->anim_type)
				break;
		}

		if (move->newMask == 0) {
			// this animation type is not part of the combo table... we don't know what keys to press to
			// invoke it!
			attack_move->combo_prev_animtype = ONcAnimType_None;

			if (attack_move->keys_wentdown == 0) {
				// not only that, it doesn't come with any hardcoded keys to press as given in AI2gMeleeAttackTable.
				// something is very wrong.
				AI2_ERROR(AI2cWarning, AI2cSubsystem_Melee, AI2cError_Melee_AttackMoveNotInCombo,
							NULL, AI2gNumMeleeAttacks, attack_move->anim_type, 0, 0);
			} else {
				// we use the keys already set up in AI2gMeleeAttackTable to invoke the attack.
			}
		} else {
			// set up the previous combo type and the keys required
			attack_move->combo_prev_animtype = move->oldType;
			attack_move->keys_isdown |= move->moveMask;
			attack_move->keys_wentdown |= move->newMask;
		}
	}

	/*
	 * build the state translation table
	 */

	UUrMemory_Clear(AI2gMelee_StateFromAnimState, ONcAnimState_Max * sizeof(AI2tMeleeMoveState));

	for (translator = AI2cMelee_StateHash; translator->totoro_state != (UUtUns16) -1; translator++) {
		UUmAssert((translator->totoro_state >= 0) && (translator->totoro_state < ONcAnimState_Max));
		UUmAssert((translator->melee_state > AI2cMeleeMoveState_None) && (translator->melee_state < AI2cMeleeMoveState_Max));

		AI2gMelee_StateFromAnimState[translator->totoro_state] = translator->melee_state;
	}

	return UUcError_None;
}

void AI2rMelee_Terminate(void)
{
}

// ------------------------------------------------------------------------------------
// -- reference counting

UUtError AI2rMelee_AddReference(AI2tMeleeProfile *ioMeleeProfile, ONtCharacter *inCharacter)
{
	UUtError error;

	if (ioMeleeProfile->reference_count == 0) {
		error = AI2rMelee_PrepareForUse(ioMeleeProfile);
		if (error != UUcError_None)
			return error;
	}

	ioMeleeProfile->reference_count++;
/*	COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: adding reference from character %s - total now %d",
				ioMeleeProfile->name, inCharacter->player_name, ioMeleeProfile->reference_count);*/
	return UUcError_None;
}

void AI2rMelee_RemoveReference(AI2tMeleeProfile *ioMeleeProfile, ONtCharacter *inCharacter)
{
/*	COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: removing reference from character %s - total now %d",
				ioMeleeProfile->name, inCharacter->player_name, ioMeleeProfile->reference_count - 1);*/

	UUmAssert(ioMeleeProfile->reference_count > 0);
	if (ioMeleeProfile->reference_count > 0)
		ioMeleeProfile->reference_count--;
}

// ------------------------------------------------------------------------------------
// -- interface, selection and update

// clear the global variables that control which melee profile is being displayed
void AI2rMelee_GlobalUpdate(void)
{
	AI2gMelee_ShowProfile = NULL;
}

void AI2rMelee_GlobalFinish(void)
{
	if (AI2gMelee_ShowProfile == NULL) {
		// write the top two lines
		UUrString_Copy(AI2gMelee_StatusLine[0].text, "*** no melee being run", COcMaxStatusLine);
		UUrString_Copy(AI2gMelee_StatusLine[1].text, "", COcMaxStatusLine);
	}

	return;
}

// enable or disable the debugging status display window
UUtBool AI2rMelee_ToggleStatusDisplay(void)
{
	AI2gMelee_ShowStatus = (AI2gMelee_ShowStatus) ? UUcFalse : UUcTrue;

	return AI2gMelee_ShowStatus;
}

// reset for a new technique
static void AI2iMeleeState_NewTechnique(AI2tMeleeState *ioMeleeState)
{
	ioMeleeState->move_index = 0;
	ioMeleeState->weight_threshold = 0;

	ioMeleeState->attack_waspositioned = UUcFalse;
	ioMeleeState->position_lastkeys = 0;
	ioMeleeState->position_force_attack = UUcFalse;
	ioMeleeState->last_targetanim_considered = (UUtUns32) -1;
	ioMeleeState->technique_face_direction = AI2cAngle_None;
	ioMeleeState->lock_facing_for_technique = UUcFalse;
	ioMeleeState->committed_to_technique = UUcFalse;

	ioMeleeState->position_notfacing_timer = 0;
}

// abort a technique in progress
static void AI2iMeleeState_AbortTechnique(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState)
{
	UUtUns32 index;

	if (ioMeleeState->current_technique == NULL)
		return;

	if (ioMeleeState->current_technique->computed_flags & AI2cTechniqueComputedFlag_IsAction) {
		// we must flag this action so that we don't immediately choose it again after aborting
		index = ioMeleeState->current_technique->index_in_profile;
		UUmAssert((index >= 0) && (index < ioCharacter->ai2State.meleeProfile->num_actions));

		// disable this technique.
		ioMeleeState->weight_adjust[index] = AI2cMelee_WeightAdjustAborted;
	}

	ioMeleeState->current_technique = NULL;
	ioMeleeState->current_move = NULL;
	ioMeleeState->current_attackanim = NULL;

	AI2iMelee_NewRandomSeeds(ioMeleeState);
	AI2iMeleeState_NewTechnique(ioMeleeState);
}

// apply delay for a technique
static void AI2iMeleeState_ApplyTechniqueDelay(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState)
{
	UUtUns16 index;
	UUtUns32 delay_frames;

	if (ioMeleeState->current_technique == NULL)
		return;

	delay_frames = ioMeleeState->current_technique->delay_frames;
	if (delay_frames == 0) {
		return;
	}

	// we must apply a delay to this technique so that we don't choose it again too soon
	index = ioMeleeState->current_technique->delaytimer_index;
	UUmAssert(index != (UUtUns16) -1);
	UUmAssert((index >= 0) && (index < AI2cMeleeMaxDelayTimers));

	ioCharacter->ai2State.meleeDelayTimers[index] = ONrGameState_GetGameTime() + delay_frames;
}

// clear the melee state
void AI2rMeleeState_Clear(AI2tMeleeState *ioMeleeState, UUtBool inClearFightInfo)
{
	AI2iMelee_NewRandomSeeds(ioMeleeState);

	ioMeleeState->considered_react_anim_index = (UUtUns32) -1;
	ioMeleeState->frames_to_hit = (UUtUns32) -1;
	ioMeleeState->current_technique = NULL;
	ioMeleeState->current_move = NULL;
	ioMeleeState->current_attackanim = NULL;
	ioMeleeState->currently_blocking = UUcFalse;
	ioMeleeState->distance_to_target = 1e+09f;
	ioMeleeState->angle_to_target = AI2cAngle_None;
	ioMeleeState->cast_frame = 0;

	AI2iMeleeState_NewTechnique(ioMeleeState);
	UUrMemory_Clear(ioMeleeState->weight_adjust, AI2cMeleeMaxTechniques * sizeof(float));

	if (inClearFightInfo) {
		ioMeleeState->fight_info.fight_owner = NULL;
		ioMeleeState->fight_info.prev = NULL;
		ioMeleeState->fight_info.next = NULL;
	}
}

static void AI2iMelee_SetupNewTarget(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState)
{
	ioMeleeState->currently_blocking = UUcFalse;
	ioMeleeState->considered_react_anim_index = (UUtUns32) -1;
}

// notification from the character animation system that an action has been performed
void AI2rMelee_NotifyAnimation(ONtCharacter *ioCharacter, TRtAnimation *inAnimation,
							   UUtUns16 inAnimType, UUtUns16 inNextAnimType,
							   TRtAnimState inCurFromState, TRtAnimState inNextAnimState)
{
	UUtUns16 attack_type;
	UUtBool handled;

	UUmAssert(ioCharacter->charType == ONcChar_AI2);

	if (ioCharacter->ai2State.currentGoal != AI2cGoal_Combat)
		return;

	// send a more general combat behavior notification
	AI2rCombat_Behavior(ioCharacter, &ioCharacter->ai2State.currentState->state.combat,
						AI2cCombatMessage_NewAnimation, &handled, inAnimType, inCurFromState, (UUtUns32) inAnimation);

	if (ioCharacter->ai2State.currentState->state.combat.maneuver.primary_movement != AI2cPrimaryMovement_Melee)
		return;

	if (ONrCharacter_IsVictimAnimation(ioCharacter)) {
		// stop any melee techniques that are currently executing
		AI2rStopMeleeTechnique(ioCharacter);

	} else if (AI2rExecutor_HasThrowOverride(ioCharacter)) {
		if (TRrAnimation_IsAttack(inAnimation) || TRrAnimation_TestFlag(inAnimation, ONcAnimFlag_ThrowSource)) {
#if DEBUG_VERBOSE_THROW
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: clearing throw-attack based on anim %s",
							ioCharacter->player_name, TMrInstance_GetInstanceName(inAnimation));
#endif
			AI2rExecutor_ClearAttackOverride(ioCharacter);
		}
	} else if (AI2rExecutor_HasAttackOverride(ioCharacter, &attack_type)) {
		if (attack_type == inAnimType) {
			// we have successfully started our attack
#if DEBUG_VERBOSE_ATTACK
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: clearing attack due to starting desired animtype %s",
							ioCharacter->player_name, ONrAnimTypeToString(inAnimType));
#endif
			AI2rExecutor_ClearAttackOverride(ioCharacter);
		} else {
#if DEBUG_VERBOSE_ATTACK
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: new animation of incorrect type %s != desired %s",
						ioCharacter->player_name, ONrAnimTypeToString(inAnimType), ONrAnimTypeToString(attack_type));
#endif

			if (inNextAnimType != attack_type) {
				AI2_ERROR(AI2cWarning, AI2cSubsystem_Melee, AI2cError_Melee_Failed_Attack, ioCharacter,
					attack_type, inAnimType, inNextAnimType, inNextAnimState);

				// give up on this attack, it's not going to work
				AI2rExecutor_ClearAttackOverride(ioCharacter);
				AI2rStopMeleeTechnique(ioCharacter);
			}
		}
	}
}

// calculate local environment awareness
void AI2rMelee_TestLocalEnvironment(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState)
{
	M3tVector3D localmove_pt, stop_pt, target_position, our_position;
	UUtUns8 localmove_weight;
	UUtBool localmove_ok, localmove_escape;
	float costheta, sintheta;
	UUtError error;
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);

	ioMeleeState->angle_to_target = AI2iMelee_AngleToTarget(ioCharacter, ioMeleeState);

	costheta = MUrCos(ioMeleeState->angle_to_target);		sintheta = MUrSin(ioMeleeState->angle_to_target);

	// look forwards and past the target
	MUmVector_Copy(localmove_pt, ioCharacter->actual_position);
	localmove_pt.x += sintheta * (ioMeleeState->distance_to_target + AI2cMelee_LocalAwareness_Dist);
	localmove_pt.z += costheta * (ioMeleeState->distance_to_target + AI2cMelee_LocalAwareness_Dist);

	// TEMPORARY DEBUGGING
	ioMeleeState->cast_frame = ONrGameState_GetGameTime();
	ioMeleeState->cast_pt = ioCharacter->actual_position;
	error = AI2rTryLocalMovement(ioCharacter, PHcPathMode_CasualMovement, &ioCharacter->actual_position, &localmove_pt, 3, &localmove_ok,
								&stop_pt, &localmove_weight, &localmove_escape);
	if ((error != UUcError_None) || (localmove_escape)) {
		ioMeleeState->localpath_blocked = UUcTrue;
		ioMeleeState->danger_ahead = UUcFalse;

		// TEMPORARY DEBUGGING
		ioMeleeState->cast_ahead_end = ioMeleeState->cast_pt;
		ioMeleeState->cast_ahead_end.x += sintheta * 6.0f;
		ioMeleeState->cast_ahead_end.z += costheta * 6.0f;

	} else if (localmove_ok) {
		ioMeleeState->localpath_blocked = UUcFalse;
		ioMeleeState->danger_ahead_dist = 0;
		ioMeleeState->danger_ahead = UUcFalse;

		// TEMPORARY DEBUGGING
		ioMeleeState->cast_ahead_end = localmove_pt;

	} else {
		ioMeleeState->danger_ahead_dist = sintheta * (stop_pt.x - ioCharacter->actual_position.x) + costheta * (stop_pt.z - ioCharacter->actual_position.z);
		ioMeleeState->localpath_blocked = (ioMeleeState->danger_ahead_dist < ioMeleeState->distance_to_target);
		ioMeleeState->danger_ahead = (localmove_weight == PHcDanger);

		// TEMPORARY DEBUGGING
		ioMeleeState->cast_ahead_end = stop_pt;
	}

	// look behind us
	MUmVector_Copy(localmove_pt, ioCharacter->actual_position);
	localmove_pt.x -= sintheta * AI2cMelee_LocalAwareness_Dist;
	localmove_pt.z -= costheta * AI2cMelee_LocalAwareness_Dist;

	error = AI2rTryLocalMovement(ioCharacter, PHcPathMode_CasualMovement, &ioCharacter->actual_position, &localmove_pt, 3, &localmove_ok,
								&stop_pt, &localmove_weight, &localmove_escape);
	if (error != UUcError_None) {
		ioMeleeState->danger_behind_dist = 0;
		ioMeleeState->danger_behind = UUcFalse;

		// TEMPORARY DEBUGGING
		ioMeleeState->cast_behind_end = ioMeleeState->cast_pt;
		ioMeleeState->cast_behind_end.x += sintheta * 6.0f;
		ioMeleeState->cast_behind_end.z += costheta * 6.0f;

	} else if (localmove_ok) {
		ioMeleeState->danger_behind_dist = 0;
		ioMeleeState->danger_behind = UUcFalse;

		// TEMPORARY DEBUGGING
		ioMeleeState->cast_behind_end = localmove_pt;

	} else {
		ioMeleeState->danger_behind_dist = sintheta * (stop_pt.x - ioCharacter->actual_position.x) + costheta * (stop_pt.z - ioCharacter->actual_position.z);
		ioMeleeState->danger_behind = (localmove_weight == PHcDanger);

		// TEMPORARY DEBUGGING
		ioMeleeState->cast_behind_end = stop_pt;
	}

	if (ioMeleeState->localpath_blocked) {
#if DEBUG_VERBOSE_MELEE
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: local movement says we are blocked from target", ioCharacter->player_name);
#endif
		if ((ioMeleeState->distance_to_target < AI2cMelee_MinRange) || ((active_character != NULL) && (active_character->collidingWithTarget))) {
			// ignore the fact that pathfinding says we can't get to the character, we are really close
			ioMeleeState->localpath_blocked = UUcFalse;
#if DEBUG_VERBOSE_MELEE
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: sufficiently close, ignoring blockage", ioCharacter->player_name);
#endif
		}
	}

	if (ioMeleeState->localpath_blocked) {
		// if there is a clear ray between us and the target, at least try our melee techniques
		ONrCharacter_GetPelvisPosition(ioCharacter, &our_position);
		ONrCharacter_GetPelvisPosition(ioMeleeState->target, &target_position);
		target_position.y = UUmMin(target_position.y, our_position.y);
		ioMeleeState->localpath_blocked = AI2rManeuver_CheckBlockage(ioCharacter, &target_position);

#if DEBUG_VERBOSE_MELEE
		if (!ioMeleeState->localpath_blocked) {
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: ray check is clear, continuing with melee", ioCharacter->player_name);
		}
#endif
	}
}

// set up orders for the situation when we don't have any melee techniques
static UUtBool AI2iMelee_HandleNoTechnique(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState, AI2tMeleeState *ioMeleeState)
{
	M3tPoint3D target_point, localmove_point;
	UUtUns8 localmove_weight;
	UUtBool localmove_ok, localmove_escape;
	UUtError error;
	float costheta, sintheta, theta, distance;

	// we have no melee techniques!
	AI2rFight_ReleaseAttackCookie(ioCharacter);

	AI2rMovement_FreeFacing(ioCharacter);

	if ((ioMeleeState->target != NULL) && (ioMeleeState->distance_to_target > AI2cMelee_MinRange)) {
		AI2rPath_Halt(ioCharacter);

		// build a point towards the target and try to move in that direction
		costheta = MUrCos(ioMeleeState->angle_to_target);		sintheta = MUrSin(ioMeleeState->angle_to_target);
		distance = UUmMin(AI2cMelee_LocalAwareness_Dist, ioMeleeState->distance_to_target);

		MUmVector_Copy(target_point, ioCharacter->actual_position);
		target_point.x += sintheta * distance;
		target_point.z += costheta * distance;

		error = AI2rTryLocalMovement(ioCharacter, PHcPathMode_CasualMovement, &ioCharacter->actual_position, &target_point, 3,
									&localmove_ok, &localmove_point, &localmove_weight, &localmove_escape);
		if ((error == UUcError_None) && (localmove_ok)) {
			theta = MUrATan2(localmove_point.x - ioCharacter->actual_position.x, localmove_point.z - ioCharacter->actual_position.z);
			UUmTrig_ClipLow(theta);
			AI2rMovement_MovementModifier(ioCharacter, theta, ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Melee]);
			return UUcTrue;
		}
	}

	return UUcFalse;
}

// every tick, evaluate potential techniques if necessary
UUtBool AI2rMelee_Update(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState, AI2tCombatState *ioCombatState)
{
	UUtUns32 move_type, num_reactions, itr, num_choices, current_time, index, technique_dir, num_available_techniques;
	UUtBool finished, can_block, block_low, block_high, executing_move, can_react, try_react;
	UUtBool found_evasion, fallen, have_attack_cookie, check_previous_technique, trim_nondamaging_techniques;
	AI2tMeleeProfile *profile;
	AI2tMeleeTechnique *new_technique, *technique_itr;
	AI2tMeleeMoveState cur_state, technique_fromstate;
	TRtAnimState cur_to_state;
	float direction_weight[TRcDirection_Max], generous_dir_weight[TRcDirection_Max];
	float total_weight, random_weight, cumulative_weight, max_attack_weight;
	AI2tConsideredTechnique *current_choice, *choice_itr;
	TRtAnimIntersectContext intersect_context, evade_context;
	char status_line[256];
	ONtActiveCharacter *active_character = ONrForceActiveCharacter(ioCharacter);
	ONtActiveCharacter *active_target;

#if DEBUG_VERBOSE_MELEE
	COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%d: running melee update for %s", ONrGameState_GetGameTime(), ioCharacter->player_name);
#endif

	/*
	 * PRELIMINARY SETUP
	 */

	// check that our target is valid
	if ((ioMeleeState->target == NULL) || ((ioMeleeState->target->flags & ONcCharacterFlag_InUse) == 0)) {
		// our target is invalid! this should never happen.
		return AI2iMelee_HandleNoTechnique(ioCharacter, ioCombatState, ioMeleeState);
	}

	// make sure that we're in fight mode, and work out where the target is
	ONrCharacter_FightMode(ioCharacter);
	AI2iMelee_UpdateFacing(ioCharacter, ioMeleeState);
	ioMeleeState->distance_to_target = MUmVector_GetDistance(ioCharacter->location, ioMeleeState->target->location);

	// if we aren't allowed to attack, just stand still
	if (!AI2rAllowedToAttack(ioCharacter, ioMeleeState->target)) {
		AI2rExecutor_ClearAttackOverride(ioCharacter);
		return AI2iMelee_HandleNoTechnique(ioCharacter, ioCombatState, ioMeleeState);
	}

	active_character = ONrForceActiveCharacter(ioCharacter);
	active_target = ONrForceActiveCharacter(ioMeleeState->target);
	profile = ioCharacter->ai2State.meleeProfile;

	if ((active_character == NULL) || (active_target == NULL) || (profile == NULL)) {
		// we are inactive, there are problems - can't melee
		return AI2iMelee_HandleNoTechnique(ioCharacter, ioCombatState, ioMeleeState);
	}

	if (ioMeleeState->target != ioMeleeState->last_target) {
		AI2iMelee_SetupNewTarget(ioCharacter, ioMeleeState);
		ioMeleeState->last_target = ioMeleeState->target;
	}

	if (AI2gMelee_ShowStatus) {
		// check to see if we are the first character to run melee this frame; if so,
		// then we have to print into the status window.
		if (AI2gMelee_ShowProfile == NULL) {
			AI2gMelee_ShowProfile = profile;
			AI2gMelee_ShowCurrentProfile = UUcTrue;

			// write the title line
			sprintf(status_line, "*** melee %s (%s)", ioCharacter->player_name, profile->name);
			UUrString_Copy(AI2gMelee_StatusLine[0].text, status_line, COcMaxStatusLine);
		} else {
			AI2gMelee_ShowCurrentProfile = UUcFalse;
		}
	}

	if (ioMeleeState->currently_blocking) {
		// we are blocking an incoming attack; wait until the target's animation is finished
		if (ioMeleeState->target->animCounter == ioMeleeState->current_react_anim_index) {
			if (active_target != NULL) {
				const TRtAnimation *animation = active_target->animation;
				
				if (TRrAnimation_IsAttack(animation) && (TRrAnimation_GetThrowType(animation) == 0)) {
					// this is an attack, block until it lands
					if (active_target->animFrame < TRrAnimation_GetLastAttackFrame(animation)) {
						// just stand still and block
//						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: blocking %s", ioCharacter->player_name, (ioMeleeState->block_keys == 0) ? "high" : "low");
						AI2iMelee_ApplyFacing(ioCharacter, ioMeleeState);
						AI2rExecutor_MoveOverride(ioCharacter, ioMeleeState->block_keys);
						return UUcTrue;
					}
				}
			}
		}

		ioMeleeState->currently_blocking = UUcFalse;
		// FIXME: put a timer here to make AIs easier to beat?
	}

	/*
	 * CURRENT MOVE UPDATE
	 */

	// run the update for our current move
	executing_move = UUcFalse;
	ioMeleeState->running_active_animation = ((TRrAnimation_IsAttack(active_character->animation)) || 
											  (TRrAnimation_TestFlag(active_character->animation, ONcAnimFlag_ThrowSource)) ||
											  (AI2rExecutor_HasAttackOverride(ioCharacter, NULL)) || 
											  (ONrCharacter_InAir(ioCharacter)));
	if (ioMeleeState->current_move != NULL) {
		UUmAssert(ioMeleeState->current_technique != NULL);

		// clear outstanding movement modifiers and do facing
		AI2rPath_Halt(ioCharacter);
		AI2iMelee_ApplyFacing(ioCharacter, ioMeleeState);
		AI2rMovement_FreeFacing(ioCharacter);

		// update this move, and determine whether it has finished
		move_type = (ioMeleeState->current_move->move & AI2cMoveType_Mask) >> AI2cMoveType_Shift;
		UUmAssert((move_type >= 0) && (move_type < AI2cMeleeNumMoveTypes));
		
		if (AI2gMelee_ShowCurrentProfile) {
			sprintf(status_line, "%s move #%d", ioMeleeState->current_technique->name, ioMeleeState->move_index + 1);
		}

		finished = AI2cMeleeMoveTypes[move_type].func_update(ioCharacter, ioMeleeState, ioMeleeState->current_move);
		
		if (AI2gMelee_ShowCurrentProfile) {
			if (finished) {
				strcat(status_line, " (done)");
			}

			UUrString_Copy(AI2gMelee_StatusLine[1].text, status_line, COcMaxStatusLine);
		}

		executing_move = !finished;
	}

	// don't consider changing moves / techniques until all of our attack moves are done and we're on the ground
	if (ioMeleeState->running_active_animation) {
#if DEBUG_VERBOSE_MELEE
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: can't change technique, waiting for animation %s to complete", 
			ioCharacter->player_name, TMrInstance_GetInstanceName(active_character->animation));
#endif
		if (ioMeleeState->current_technique == NULL) {
			if (AI2gMelee_ShowCurrentProfile) {
				sprintf(AI2gMelee_StatusLine[1].text, "waiting for moves to finish");
			}
		} else {
			if (AI2gMelee_ShowCurrentProfile) {
				sprintf(AI2gMelee_StatusLine[1].text, "%s waiting to finish", ioMeleeState->current_technique->name);
			}
		}
		AI2iMelee_ApplyFacing(ioCharacter, ioMeleeState);
		return UUcTrue;
	}

	if ((ioMeleeState->current_move == NULL) && (ioMeleeState->current_technique != NULL)) {
		// we have already finished our technique and were just waiting for its attack moves
		// to end... but now it's really done.
		AI2iMelee_FinishTechnique(ioCharacter, ioMeleeState);
	}

	if (executing_move) {
		// can we try a dodge from where we currently are?
		UUmAssert((move_type >= 0) && (move_type < AI2cMeleeNumMoveTypes));
		can_react = AI2cMeleeMoveTypes[move_type].func_canevade(ioCharacter, ioMeleeState, ioMeleeState->current_move);
	} else {
		// we are in between moves, we can of course try a dodge
		can_react = UUcTrue;
	}

	/*
	 * REACTION DETERMINATION
	 */

	try_react = UUcFalse;
	if (can_react) {
		// check to see whether we're being attacked
		if ((active_target != NULL) && TRrAnimation_IsAttack(active_target->animation)) {
			AI2iMelee_CheckEvasionChance(ioMeleeState, profile, active_target->animation);

			if (ioMeleeState->react_enable) {
				// set up for the intersection phase
#if DEBUG_SHOWINTERSECTIONS
				TRgStoreThisIntersection = MELEE_REACTINTERSECTIONS;
#endif
				ONrSetupAnimIntersectionContext(ioMeleeState->target, ioCharacter, UUcFalse, &evade_context);

				/*
				// CB: this breaks stuff! is this logic correct? I believe it may not be




				
				// we are considering a defense. we need animcheck_location_matrix to account for if the opponent
				// has moved since starting their attack. basically this matrix must give our position
				// relative to where the start of the opponent's animation would have been.
				//
				// the delta-position (in negated-Y-3DMax space) from the start of the animation to this frame of the
				// animation is stored in the positionPoint array. we apply this so that the target's position is known
				// relative to where the animation started, rather than where it currently is.
				UUmAssert((ioMeleeState->target->animFrame >= 0) &&
						(ioMeleeState->target->animFrame < TRrAnimation_GetDuration(ioMeleeState->target->animation)));
				position_pt = TRrAnimation_GetPositionPoint(ioMeleeState->target->animation, ioMeleeState->target->animFrame);

				intersect_context.current_location_matrix.m[3][0] += position_pt->location_x * TRcPositionGranularity;
				intersect_context.current_location_matrix.m[3][1] += position_pt->location_y * TRcPositionGranularity;
				*/
				
				// look to see whether the attack will hit us
				try_react = TRrCheckAnimationBounds(&evade_context);
				if (try_react) {
					try_react = TRrIntersectAnimations(&evade_context);
				}

				if (try_react) {
					// we will be hit! store the number of frames until we are next hit
					ioMeleeState->frames_to_hit = evade_context.frames_to_hit;
				}
			}
		}
	}

	if (executing_move && !try_react) {
		// we are in the middle of a move and we don't want to evade or block.
		return UUcTrue;
	}

	cur_to_state = TRrAnimation_GetTo(active_character->animation);
	if (cur_to_state >= ONcAnimState_Max) {
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "### WARNING: character '%s' has anim %s with unknown tostate %d, add to Oni_Character_Animation.h",
						ioCharacter->player_name, TMrInstance_GetInstanceName(active_character->animation), cur_to_state);
		cur_to_state = ONcAnimState_None;
	}
	fallen = ONrAnimState_IsFallen(cur_to_state);
	cur_state = AI2iMelee_TranslateTotoroAnimState(ioCharacter, cur_to_state);

#if DEBUG_VERBOSE_MELEE
	COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: to_state %s -> current melee state '%s'", 
		ioCharacter->player_name, ONrAnimStateToString(cur_to_state), AI2cMeleeMoveStateName[cur_state]);
#endif

	if (AI2gSpacingEnable) {
		// we want an attack cookie if possible
		have_attack_cookie = AI2rFight_GetAttackCookie(ioCharacter);
	} else {
		have_attack_cookie = UUcTrue;
	}

	// stay with the current technique if possible
	new_technique = ioMeleeState->current_technique;

	if ((ioMeleeState->current_technique == NULL) || (AI2gUltraMode) || 
		(ioCharacter->flags2 & ONcCharacterFlag2_UltraMode) || (!ioMeleeState->committed_to_technique)) {
		// we either have no technique, or are between moves in an interruptable technique (or Ultra Mode is on,
		// which makes all techniques interruptable).
#if DEBUG_VERBOSE_MELEE
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: considering technique switch from %s",
			ioCharacter->player_name, (ioMeleeState->current_technique == NULL) ? "<null>" :
			ioMeleeState->current_technique->name);
#endif

		// calculate the angle to our target, since we're beginning a new technique
		ioMeleeState->angle_to_target = AI2iMelee_AngleToTarget(ioCharacter, ioMeleeState);

		// get the direction to our target, which determines which moves are viable.
		AI2iMelee_CalculateDirectionWeights(ioCharacter, ioMeleeState, ioMeleeState->angle_to_target, direction_weight, generous_dir_weight);

		if (have_attack_cookie) {

			/*
			 * ACTION SELECTION
			 */

			// set up an intersection context for us and our target
			ONrSetupAnimIntersectionContext(ioCharacter, ioMeleeState->target, UUcTrue, &intersect_context);

			// work out whether our target can block our attacks
			can_block = ONrCharacter_CouldBlock(ioMeleeState->target, ioCharacter, &block_low, &block_high);

			if (AI2gMelee_ShowCurrentProfile) {
				// clear the technique display area
				for (itr = 0; itr < AI2cMelee_StatusLines; itr++) {
					UUrString_Copy(AI2gMelee_StatusLine[itr + 2].text, "", COcMaxStatusLine);
				}

				AI2gMelee_NextStatusLine = 0;
			}

			num_choices = 0;
			total_weight = 0;
			max_attack_weight = 0;
			current_choice = AI2gMeleeCurrentChoices;
			technique_itr = profile->technique;
			num_available_techniques = profile->num_actions;
			current_time = ONrGameState_GetGameTime();
			if (num_available_techniques > AI2cMeleeMaxTechniques) {
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "### melee profile %s exceeds max number of actions (%d > %d)",
									num_available_techniques, AI2cMeleeMaxTechniques);
				num_available_techniques = AI2cMeleeMaxTechniques;
			}

			for (itr = 0; itr < num_available_techniques; itr++, technique_itr++, current_choice++) {
				// start evaluating this technique
				current_choice->technique = technique_itr;
				current_choice->weight = (float) technique_itr->base_weight;
				current_choice->attack_moveindex = (UUtUns32) -1;
				current_choice->attack_move = NULL;
				current_choice->attack_before_hit = UUcFalse;
				current_choice->positionmove_skip = (UUtUns16) -1;

				if (!have_attack_cookie && (technique_itr->computed_flags & AI2cTechniqueComputedMask_RequiresAttackCookie)) {
					// we cannot perform this technique without an attack cookie
#if DEBUG_VERBOSE_WEIGHTVAL
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: weight %s - requires attack cookie, skip",
						ioCharacter->player_name, current_choice->technique->name);
#endif
					current_choice->weight = 0;
					continue;
				}

				if (ioMeleeState->weight_adjust[itr] == AI2cMelee_WeightAdjustAborted) {
					// this technique was just aborted, don't consider it again
#if DEBUG_VERBOSE_WEIGHTVAL
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: weight %s - just aborted, skip", ioCharacter->player_name,
						current_choice->technique->name);
#endif
					current_choice->weight = 0;
					continue;
				}

				if ((technique_itr != ioMeleeState->current_technique) && (technique_itr->delaytimer_index != (UUtUns16) -1)) {
					// this technique has a timer on it
					UUmAssert((technique_itr->delaytimer_index >= 0) && (technique_itr->delaytimer_index < AI2cMeleeMaxDelayTimers));
					if (ioCharacter->ai2State.meleeDelayTimers[technique_itr->delaytimer_index] > current_time) {
						// we must wait before we can consider this technique again
#if DEBUG_VERBOSE_WEIGHTVAL
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: weight %s - delay (%d more), skip", ioCharacter->player_name,
							current_choice->technique->name, current_time - ioCharacter->ai2State.meleeDelayTimers[technique_itr->delaytimer_index]);
#endif
						current_choice->weight = 0;
						continue;
					}
				}

#if DEBUG_VERBOSE_WEIGHTVAL
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: weight %s - base is %f", ioCharacter->player_name,
					current_choice->technique->name, current_choice->weight);
#endif
				// apply the weight adjustment and decay it
				current_choice->weight *= 1.0f - UUmMin(AI2cMelee_MaxWeightAdjust, ioMeleeState->weight_adjust[itr]);
#if DEBUG_VERBOSE_WEIGHTVAL
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - adjust %f, multiply %f -> current %f", ioMeleeState->weight_adjust[itr],
					1.0f - UUmMin(AI2cMelee_MaxWeightAdjust, ioMeleeState->weight_adjust[itr]), current_choice->weight);
#endif

				// check the direction of this technique is valid
				technique_dir = (technique_itr->computed_flags & AI2cTechniqueComputedMask_MoveDirection)
										>> AI2cTechniqueComputedShift_MoveDirection;
				if (technique_itr->user_flags & AI2cTechniqueUserFlag_GenerousDirection) {
					// use the more generous direction weights
					current_choice->weight *= generous_dir_weight[technique_dir];
#if DEBUG_VERBOSE_WEIGHTVAL
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - generous dirweight %f -> current %f", generous_dir_weight[technique_dir], current_choice->weight);
#endif
				} else {
					// use the conservative direction weights
					current_choice->weight *= direction_weight[technique_dir];
#if DEBUG_VERBOSE_WEIGHTVAL
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - dirweight %f -> current %f", direction_weight[technique_dir], current_choice->weight);
#endif
				}

				if (current_choice->weight == 0) {
#if DEBUG_VERBOSE_MELEE
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: technique %s incorrect direction (%d has weight 0).",
									ioCharacter->player_name, technique_itr->name, technique_dir);
#endif
					continue;
				}

				// check that the technique starts in the right state
				technique_fromstate = technique_itr->computed_flags & AI2cTechniqueComputedMask_StartState;
				UUmAssert((technique_fromstate >= 0) && (technique_fromstate < AI2cMeleeMoveState_Max));

				if (technique_fromstate != cur_state) {
					if (technique_fromstate == AI2cMeleeMoveState_None) {
						AI2_ERROR(AI2cBug, AI2cSubsystem_Melee, AI2cError_Melee_BrokenTechnique, ioCharacter,
									ioMeleeState, ioMeleeState->current_technique, 0, 0);
#if DEBUG_VERBOSE_MELEE
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: technique %s start state NONE, error.", 
												ioCharacter->player_name, technique_itr->name);
#endif
						current_choice->weight = 0;
						continue;
					} else {
						// we would need to execute a state transition in order to run this technique
						if ((technique_itr->computed_flags & AI2cTechniqueComputedFlag_AllowTransition) == 0) {
#if DEBUG_VERBOSE_MELEE
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: technique %s start state %s requires exact match. don't consider.", 
								ioCharacter->player_name, technique_itr->name, AI2cMeleeMoveStateName[technique_fromstate]);
#endif
							current_choice->weight = 0;
							continue;
						}
					}
				}

				if (AI2gMelee_ShowCurrentProfile) {
					UUrString_Copy(AI2gMelee_TechniqueWeightDescription, "", 64);
				}

				// evaluate and weight this technique with respect to the current position and animation state of the target
				AI2iMelee_WeightTechnique(ioCharacter, ioMeleeState, profile, &intersect_context, UUcTrue, technique_dir,
											cur_state, cur_to_state, can_block, block_low, block_high, current_choice);

				if (AI2gMelee_ShowCurrentProfile && (AI2gMelee_NextStatusLine < AI2cMelee_StatusLines)) {
					sprintf(status_line, "%-12s", current_choice->technique->name);
					sprintf(&status_line[12], " %4.1f [%4.2f] ", ioMeleeState->weight_adjust[itr], current_choice->weight);

					strcat(status_line, AI2gMelee_TechniqueWeightDescription);

					UUrString_Copy(AI2gMelee_StatusLine[AI2gMelee_NextStatusLine + 2].text, status_line, COcMaxStatusLine);

					AI2gMelee_NextStatusLine++;
				}

				if (current_choice->weight == 0)
					continue;

				// add a bias towards not switching techniques
				if (technique_itr == ioMeleeState->current_technique) {
					current_choice->weight *= AI2cMeleeCurrentTechniqueMultiplier;
				}

				// we've found this technique.
				total_weight += current_choice->weight;
				UUmAssert(current_choice->weight > 0);
				num_choices++;
				if (current_choice->technique->computed_flags & AI2cTechniqueComputedMask_Damaging) {
					// store this technique's weight
					max_attack_weight = UUmMax(max_attack_weight, current_choice->weight);
				}
			}
		} else {

			/*
			 * SPACING BEHAVIOR SELECTION
			 */

			if (AI2gMelee_ShowCurrentProfile) {
				// clear the technique display area
				for (itr = 0; itr < AI2cMelee_StatusLines; itr++) {
					UUrString_Copy(AI2gMelee_StatusLine[itr + 2].text, "", COcMaxStatusLine);
				}

				AI2gMelee_NextStatusLine = 0;
			}

			num_choices = 0;
			total_weight = 0;
			max_attack_weight = 0;
			current_choice = AI2gMeleeCurrentChoices;
			technique_itr = &profile->technique[profile->num_actions + profile->num_reactions];
			num_available_techniques = profile->num_spacing;
			current_time = ONrGameState_GetGameTime();
			if (num_available_techniques > AI2cMeleeMaxTechniques) {
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "### melee profile %s exceeds max number of spacing behaviors (%d > %d)",
									num_available_techniques, AI2cMeleeMaxTechniques);
				num_available_techniques = AI2cMeleeMaxTechniques;
			}

			for (itr = 0; itr < num_available_techniques; itr++, technique_itr++, current_choice++) {
				// start evaluating this technique
				current_choice->technique = technique_itr;
				current_choice->weight = (float) technique_itr->base_weight;
				current_choice->attack_moveindex = (UUtUns32) -1;
				current_choice->attack_move = NULL;
				current_choice->attack_before_hit = UUcFalse;
				current_choice->positionmove_skip = (UUtUns16) -1;

				if ((technique_itr != ioMeleeState->current_technique) && (technique_itr->delaytimer_index != (UUtUns16) -1)) {
					// this technique has a timer on it
					UUmAssert((technique_itr->delaytimer_index >= 0) && (technique_itr->delaytimer_index < AI2cMeleeMaxDelayTimers));
					if (ioCharacter->ai2State.meleeDelayTimers[technique_itr->delaytimer_index] > current_time) {
						// we must wait before we can consider this spacing behavior again
#if DEBUG_VERBOSE_SPACINGWEIGHT
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: weight %s - delay (%d more), skip", ioCharacter->player_name,
							current_choice->technique->name, current_time - ioCharacter->ai2State.meleeDelayTimers[technique_itr->delaytimer_index]);
#endif
						current_choice->weight = 0;
						continue;
					}
				}

#if DEBUG_VERBOSE_SPACINGWEIGHT
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: weight %s - base is %f", ioCharacter->player_name,
					current_choice->technique->name, current_choice->weight);
#endif

				// check the direction of this technique is valid
				technique_dir = (technique_itr->computed_flags & AI2cTechniqueComputedMask_MoveDirection)
										>> AI2cTechniqueComputedShift_MoveDirection;
				if (technique_itr->user_flags & AI2cTechniqueUserFlag_GenerousDirection) {
					// use the more generous direction weights
					current_choice->weight *= generous_dir_weight[technique_dir];
#if DEBUG_VERBOSE_SPACINGWEIGHT
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - generous dirweight %f -> current %f", generous_dir_weight[technique_dir], current_choice->weight);
#endif
				} else {
					// use the conservative direction weights
					current_choice->weight *= direction_weight[technique_dir];
#if DEBUG_VERBOSE_SPACINGWEIGHT
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - dirweight %f -> current %f", direction_weight[technique_dir], current_choice->weight);
#endif
				}

				if (current_choice->weight == 0) {
#if DEBUG_VERBOSE_MELEE
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: technique %s incorrect direction (%d has weight 0).",
									ioCharacter->player_name, technique_itr->name, technique_dir);
#endif
					continue;
				}

				// check that the technique starts in the right state
				technique_fromstate = technique_itr->computed_flags & AI2cTechniqueComputedMask_StartState;
				UUmAssert((technique_fromstate >= 0) && (technique_fromstate < AI2cMeleeMoveState_Max));

				if (technique_fromstate != cur_state) {
					UUmAssert(technique_fromstate != AI2cMeleeMoveState_None);

					// we would need to execute a state transition in order to run this technique
					if ((technique_itr->computed_flags & AI2cTechniqueComputedFlag_AllowTransition) == 0) {
#if DEBUG_VERBOSE_MELEE
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: technique %s start state %s requires exact match. don't consider.", 
							ioCharacter->player_name, technique_itr->name, AI2cMeleeMoveStateName[technique_fromstate]);
#endif
						current_choice->weight = 0;
						continue;
					}
				}

				if (AI2gMelee_ShowCurrentProfile) {
					UUrString_Copy(AI2gMelee_TechniqueWeightDescription, "", 64);
				}

				// evaluate and weight this technique with respect to the current position and animation state of the target
				AI2iMelee_WeightSpacingBehavior(ioCharacter, ioMeleeState, profile, technique_dir, cur_state, cur_to_state, current_choice);

				if (AI2gMelee_ShowCurrentProfile && (AI2gMelee_NextStatusLine < AI2cMelee_StatusLines)) {
					sprintf(status_line, "%-12s", current_choice->technique->name);
					sprintf(&status_line[12], " [%4.2f] ", current_choice->weight);

					strcat(status_line, AI2gMelee_TechniqueWeightDescription);

					UUrString_Copy(AI2gMelee_StatusLine[AI2gMelee_NextStatusLine + 2].text, status_line, COcMaxStatusLine);

					AI2gMelee_NextStatusLine++;
				}

				if (current_choice->weight == 0)
					continue;

				// add a bias towards not switching spacing behaviors
				if (technique_itr == ioMeleeState->current_technique) {
					current_choice->weight *= AI2cMeleeCurrentTechniqueMultiplier;
				}

				// we've found this technique.
				total_weight += current_choice->weight;
				UUmAssert(current_choice->weight > 0);
				num_choices++;
			}
		}
		
		// no selected technique yet
		new_technique = NULL;

		// check to see if our current technique is still the one that we would pick
		if ((ioMeleeState->current_technique == NULL) || (ioMeleeState->current_technique->computed_flags & AI2cTechniqueComputedFlag_IsReaction)) {
			check_previous_technique = UUcFalse;

		} else if (have_attack_cookie) {
			check_previous_technique = ((ioMeleeState->current_technique->computed_flags & AI2cTechniqueComputedFlag_IsAction) > 0);

		} else {
			check_previous_technique = ((ioMeleeState->current_technique->computed_flags & AI2cTechniqueComputedFlag_IsSpacing) > 0);
		}

		if (check_previous_technique) {
			index = ioMeleeState->current_technique->index_in_profile;
			UUmAssert((index >= 0) && (index < num_available_techniques));

			if (ioMeleeState->weight_threshold <= AI2gMeleeCurrentChoices[index].weight) {
				// our old technique is still sufficiently attractive for us to keep using it.
				UUmAssert(AI2gMeleeCurrentChoices[index].technique == ioMeleeState->current_technique);
				new_technique = ioMeleeState->current_technique;

				if (have_attack_cookie && try_react && AI2gMeleeCurrentChoices[index].attack_before_hit) {
					// we can hit our target before they hit us. act, don't react.
					try_react = UUcFalse;
				}
			}
		}

		trim_nondamaging_techniques = AI2gMeleeWeightCorrection && !fallen && have_attack_cookie;

		if ((num_choices == 0) || (trim_nondamaging_techniques && (max_attack_weight == 0))) {
			// no viable techniques were found
			current_choice = NULL;

		} else {
			if (trim_nondamaging_techniques) {
				// re-weight any non-damaging techniques so that they aren't weighted higher than the highest-weighted
				// damaging technique. don't do this if we are lying on the ground and have to get up; also don't do this
				// if we don't have the attack cookie.
				for (itr = 0, choice_itr = AI2gMeleeCurrentChoices; itr < num_available_techniques; itr++, choice_itr++) {
					if (((choice_itr->technique->computed_flags & AI2cTechniqueComputedMask_Damaging) == 0) &&
						((choice_itr->technique->computed_flags & AI2cTechniqueComputedFlag_SpecialCaseMove) == 0)) {
						if (choice_itr->weight > max_attack_weight) {
							// trim this choice's weight down
							total_weight -= choice_itr->weight - max_attack_weight;
							UUmAssert(total_weight > -1e-03f);
							choice_itr->weight = max_attack_weight;
						}
					}
				}
			}

			while (new_technique == NULL) {
				if (num_choices == 0) {
					current_choice = NULL;
					break;
				}

				// pick techniques at random, using weight factors to bias
				random_weight = (total_weight * ioMeleeState->action_seed) / UUcMaxUns16 - 0.01f;	// fudge to ensure always < total_weight
				cumulative_weight = 0;
				current_choice = NULL;
				for (itr = 0; itr < num_available_techniques; itr++) {
					cumulative_weight += AI2gMeleeCurrentChoices[itr].weight;
					if (cumulative_weight >= random_weight) {
						// the random index indicates that this is the technique we want.
						// set up weight_threshold so that if it's a heavily weighted technique, we
						// don't just skip right off it the next time we check
						current_choice = &AI2gMeleeCurrentChoices[itr];
						new_technique = current_choice->technique;
						ioMeleeState->weight_threshold = cumulative_weight - random_weight;
						break;
					}
				}
				UUmAssert(itr < num_available_techniques);
				UUmAssert(current_choice != NULL);

				if (AI2iCheckTechniqueViability(ioCharacter, ioMeleeState, profile, have_attack_cookie, &intersect_context, current_choice)) {
					// choose this technique
					ioMeleeState->current_technique_ends_in_jump = current_choice->ends_in_jump;
					ioMeleeState->current_technique_jump_vel = current_choice->jump_velocity;
					ioMeleeState->positionmove_skip = current_choice->positionmove_skip;

					if (try_react && current_choice->attack_before_hit) {
						// we can hit our target before they hit us. act, don't react.
						try_react = UUcFalse;
					}
					
				} else {
					// this technique is not viable. forget about it.
#if DEBUG_VERBOSE_MELEE
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: technique %s is not viable, reject this choice.", 
						ioCharacter->player_name, new_technique->name);
#endif
					total_weight -= current_choice->weight;
					current_choice->weight = 0;
					num_choices--;
					new_technique = NULL;
				}				
			}
		}
	}
	
	/*
	 * REACTION SELECTION
	 */

	if (try_react) {
		UUmAssertReadPtr(active_target, sizeof(*active_target));
		found_evasion = UUcFalse;

		// FIXME: maybe if we could already block the attack, don't try dodging?

		if (ioMeleeState->evade_enable) {
#if DEBUG_VERBOSE_MELEE
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: considering reaction techniques", ioCharacter->player_name);
#endif
			// consider all of our reaction techniques, and try to find one that gets us
			// out of the way
			num_choices = 0;
			total_weight = 0;
			current_choice = AI2gMeleeCurrentChoices;
			technique_itr = profile->technique + profile->num_actions;
			num_reactions = profile->num_reactions;
			current_time = ONrGameState_GetGameTime();
			if (num_reactions > AI2cMeleeMaxTechniques) {
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "### melee profile %s exceeds max number of reactions (%d > %d)",
									num_reactions, AI2cMeleeMaxTechniques);
				num_reactions = AI2cMeleeMaxTechniques;
			}

			for (itr = 0; itr < num_reactions; itr++, technique_itr++, current_choice++) {
				// start evaluating this technique
				current_choice->technique = technique_itr;
				current_choice->weight = (float) technique_itr->base_weight;
				current_choice->positionmove_skip = (UUtUns16) -1;

				// check that the technique starts in the right state
				technique_fromstate = technique_itr->computed_flags & AI2cTechniqueComputedMask_StartState;
				UUmAssert((technique_fromstate >= 0) && (technique_fromstate < AI2cMeleeMoveState_Max));

				if (technique_fromstate != cur_state) {
					// state transitions are not possible, reactions must come from the right state
					current_choice->weight = 0;
					continue;
				}

				if (technique_itr->delaytimer_index != (UUtUns16) -1) {
					// this evasion technique has a timer on it
					UUmAssert((technique_itr->delaytimer_index >= 0) && (technique_itr->delaytimer_index < AI2cMeleeMaxDelayTimers));
					if (ioCharacter->ai2State.meleeDelayTimers[technique_itr->delaytimer_index] > current_time) {
						// we must wait before we can consider this reaction again
						current_choice->weight = 0;
						continue;
					}
				}

				// evaluate this technique; will it get us out of the way?
				AI2iMelee_WeightTechnique(ioCharacter, ioMeleeState, profile, &evade_context, UUcFalse, TRcDirection_None,
											cur_state, cur_to_state, UUcFalse, UUcFalse, UUcFalse, current_choice);

				if (current_choice->weight == 0)
					continue;

				// we've found this technique.
				total_weight += current_choice->weight;
				UUmAssert(current_choice->weight > 0);
				num_choices++;
			}
		
			if (num_choices > 0) {
				// pick techniques at random, using weight factors to bias
				random_weight = (total_weight * ioMeleeState->reaction_seed) / UUcMaxUns16 - 0.01f;	// fudge to ensure always < total_weight
				cumulative_weight = 0;
				current_choice = NULL;
				for (itr = 0; itr < num_reactions; itr++) {
					cumulative_weight += AI2gMeleeCurrentChoices[itr].weight;
					if (cumulative_weight >= random_weight) {
						// the random index indicates that this is the technique we want.
						// set up weight_threshold so that if it's a heavily weighted technique, we
						// don't just skip right off it the next time we check
						current_choice = &AI2gMeleeCurrentChoices[itr];
						new_technique = current_choice->technique;
						ioMeleeState->weight_threshold = cumulative_weight - random_weight;
#if DEBUG_VERBOSE_TECHNIQUE
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: choosing reaction %s", ioCharacter->player_name, new_technique->name);
#endif
						break;
					}
				}
				UUmAssert(itr < num_reactions);
				UUmAssert(current_choice != NULL);

				// choose this technique
				ioMeleeState->current_technique_ends_in_jump = current_choice->ends_in_jump;
				ioMeleeState->current_technique_jump_vel = current_choice->jump_velocity;
				ioMeleeState->positionmove_skip = current_choice->positionmove_skip;

				found_evasion = UUcTrue;
				ioMeleeState->current_react_anim_index = ioMeleeState->target->animCounter;
			}
		}

		if (!found_evasion) {
			// we can't (or won't) dodge. try to block this incoming attack
			ioMeleeState->currently_blocking = UUcTrue;
			ioMeleeState->current_react_anim_index = ioMeleeState->target->animCounter;

			if (TRrAnimation_TestAttackFlag(active_target->animation, (TRtAnimTime) -1, (UUtUns32) -1, ONcAttackFlag_AttackHigh)) {
				// we must be standing to block this attack
				ioMeleeState->block_keys = 0;

			} else if (TRrAnimation_TestAttackFlag(active_target->animation, (TRtAnimTime) -1, (UUtUns32) -1, ONcAttackFlag_AttackLow)) {
				// we must be crouching to block this attack
				ioMeleeState->block_keys = LIc_BitMask_Crouch;

			} else if (AI2cMeleeState_BlockCrouching[cur_state]) {
				// we are closer to a crouched state, try to block there
				ioMeleeState->block_keys = LIc_BitMask_Crouch;

			} else {
				ioMeleeState->block_keys = 0;
			}

			// we are not running a technique any longer
			ioMeleeState->current_technique = NULL;
			ioMeleeState->current_move = NULL;
			ioMeleeState->current_attackanim = NULL;
			AI2iMeleeState_NewTechnique(ioMeleeState);
#if DEBUG_VERBOSE_TECHNIQUE
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: blocking %s", ioCharacter->player_name, (ioMeleeState->block_keys == 0) ? "high" : "low");
#endif

			// start the block
			AI2iMelee_UpdateFacing(ioCharacter, ioMeleeState);
			AI2iMelee_ApplyFacing(ioCharacter, ioMeleeState);
			AI2rExecutor_MoveOverride(ioCharacter, ioMeleeState->block_keys);
			return UUcTrue;
		}
	}

	/*
	 * NEW-TECHNIQUE UPDATE
	 */

	if (new_technique == NULL) {
		if (fallen) {
			// we are fallen and must get up - but didn't find any get-up maneuvers
			AI2rExecutor_MoveOverride(ioCharacter, LIc_BitMask_Forward);			
			return UUcTrue;
		} else {
			// we are standing and we didn't find any viable techniques. return to the combat manager,
			// and try to find something else to do.
#if DEBUG_VERBOSE_TECHNIQUE
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: no technique found!", ioCharacter->player_name);
#endif
			ioMeleeState->current_technique = NULL;
			ioMeleeState->current_move = NULL;
			ioMeleeState->current_attackanim = NULL;
			AI2iMeleeState_NewTechnique(ioMeleeState);

			// we should never get stuck with no available techniques because we've aborted previously; clear any
			// abort values.
			UUrMemory_Clear(ioMeleeState->weight_adjust, AI2cMeleeMaxTechniques * sizeof(float));

			return AI2iMelee_HandleNoTechnique(ioCharacter, ioCombatState, ioMeleeState);
		}

	} else if (new_technique != ioMeleeState->current_technique) {
		// we have found a new technique!
		ioMeleeState->current_technique = new_technique;
		ioMeleeState->current_move = &profile->move[new_technique->start_index];
		if ((new_technique == current_choice->technique) && (current_choice->attack_move != NULL)) {
			ioMeleeState->current_attackanim = current_choice->attack_move->animation;
		} else {
			ioMeleeState->current_attackanim = NULL;
		}
		AI2iMeleeState_NewTechnique(ioMeleeState);

#if DEBUG_VERBOSE_TECHNIQUE
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: starting technique %s",
			ioCharacter->player_name, (ioMeleeState->current_technique == NULL) ? "<null>" :
			ioMeleeState->current_technique->name);
#endif
	
		AI2iMelee_UpdateFacing(ioCharacter, ioMeleeState);
		AI2iMelee_ApplyFacing(ioCharacter, ioMeleeState);

		if (new_technique->computed_flags & AI2cTechniqueComputedFlag_IsAction) {
			// we should be less likely to pick this technique next time around.
			ioMeleeState->weight_adjust[new_technique->index_in_profile] += AI2cMelee_RepeatWeightPenalty;
		}

		if ((new_technique->computed_flags & AI2cTechniqueComputedMask_RequiresAttackCookie) == 0) {
			// we don't require the attack cookie for this move, release it
			AI2rFight_ReleaseAttackCookie(ioCharacter);
		}
	}

	// if we get to here, we must be changing moves, and we must have found a
	// technique.
	UUmAssert((ioMeleeState->current_move != NULL) && (ioMeleeState->current_technique != NULL));

#if DEBUG_VERBOSE_MELEE
	COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: beginning move 0x%08X (%s)",
		ioCharacter->player_name, ioMeleeState->current_move->move, 
		(ioMeleeState->current_move->animation == NULL) ? "<null>" :
			TMrInstance_GetInstanceName(ioMeleeState->current_move->animation));
#endif

	if (ioMeleeState->current_move->is_broken) {
		// we are trying to execute a broken move. bail.
		AI2_ERROR(AI2cBug, AI2cSubsystem_Melee, AI2cError_Melee_BrokenMove, ioCharacter,
			ioMeleeState, ioMeleeState->current_technique, ioMeleeState->current_move, 0);
		return AI2iMelee_StartNextMove(ioCharacter, ioMeleeState, ioMeleeState->current_move);
	} else {
		// start the selected next move. note that func_start returns true if the move is able to immediately
		// execute.
		move_type = (ioMeleeState->current_move->move & AI2cMoveType_Mask) >> AI2cMoveType_Shift;
		UUmAssert((move_type >= 0) && (move_type < AI2cMeleeNumMoveTypes));
		
#if DEBUG_VERBOSE_MELEE
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: calling start routine for move 0x%08X",
			ioCharacter->player_name, ioMeleeState->current_move->move);
#endif
		AI2cMeleeMoveTypes[move_type].func_start(ioCharacter, ioMeleeState, ioMeleeState->current_move);

		// note that the update function just called could force its technique to abort, in which case we will
		// have current_technique of NULL. in this case we do NOT want to return UUcTrue which would indicate
		// that we found a technique successfully.
		return (ioMeleeState->current_technique != NULL);
	}
}

static UUtBool AI2iMelee_PathfindingErrorHandler(ONtCharacter *inCharacter, UUtUns32 inErrorID,
													UUtUns32 inParam1, UUtUns32 inParam2,
													UUtUns32 inParam3, UUtUns32 inParam4)
{
	// just store the error ID and flag error as handled
	AI2gMelee_PathfindingError = inErrorID;
	return UUcTrue;
}

static void AI2iMelee_UpdateFacing(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState)
{
	UUtUns32 technique_dir, move_type;

	if (ioMeleeState->currently_blocking) {
		// face directly at our target
		ioMeleeState->angle_to_target = AI2iMelee_AngleToTarget(ioCharacter, ioMeleeState);
		ioMeleeState->technique_face_direction = ioMeleeState->angle_to_target;
		ioMeleeState->face_target = UUcTrue;

	} else if (ioMeleeState->current_technique == NULL) {
		// no technique - just look at the target
		ioMeleeState->angle_to_target = AI2iMelee_AngleToTarget(ioCharacter, ioMeleeState);
		ioMeleeState->technique_face_direction = ioMeleeState->angle_to_target;
		ioMeleeState->face_target = UUcFalse;

	} else if (!ioMeleeState->lock_facing_for_technique) {
		// recalculate angle to target
		ioMeleeState->angle_to_target = AI2iMelee_AngleToTarget(ioCharacter, ioMeleeState);

		// do we face the target?
		if (ioMeleeState->current_move == NULL) {
			ioMeleeState->face_target = UUcTrue;
		} else {
			move_type = (ioMeleeState->current_move->move & AI2cMoveType_Mask) >> AI2cMoveType_Shift;
			UUmAssert((move_type >= 0) && (move_type < AI2cMeleeNumMoveTypes));
			ioMeleeState->face_target = AI2cMeleeMoveTypes[move_type].func_facetarget(ioCharacter, ioMeleeState,
																					ioMeleeState->current_move);
		}

		// update our facing delta if necessary
		technique_dir = (ioMeleeState->current_technique->computed_flags & AI2cTechniqueComputedMask_MoveDirection)
			>> AI2cTechniqueComputedShift_MoveDirection;

		// store the angle to the target.
		switch(technique_dir) {
			case TRcDirection_None:
			case TRcDirection_360:
				// we don't lock facing
				ioMeleeState->technique_face_direction = AI2cAngle_None;
			break;

			case TRcDirection_Forwards:
				ioMeleeState->technique_face_direction = ioMeleeState->angle_to_target;
			break;

			case TRcDirection_Left:
				ioMeleeState->technique_face_direction = ioMeleeState->angle_to_target - M3cHalfPi;
				UUmTrig_ClipHigh(ioMeleeState->technique_face_direction);
			break;

			case TRcDirection_Right:
				ioMeleeState->technique_face_direction = ioMeleeState->angle_to_target + M3cHalfPi;
				UUmTrig_ClipLow(ioMeleeState->technique_face_direction);
			break;

			case TRcDirection_Back:
				ioMeleeState->technique_face_direction = ioMeleeState->angle_to_target + M3cPi;
				UUmTrig_ClipHigh(ioMeleeState->technique_face_direction);
			break;

			default:
				UUmAssert(0);
		}
	}

	if (ioMeleeState->technique_face_direction == AI2cAngle_None) {
		ioMeleeState->facing_delta = 0;
	} else {
		// work out how close we are to our technique's direction
		ioMeleeState->facing_delta = ioMeleeState->technique_face_direction - ioCharacter->facing;
		UUmTrig_ClipAbsPi(ioMeleeState->facing_delta);
		ioMeleeState->facing_delta = (float)fabs(ioMeleeState->facing_delta);
	}
}

static void AI2iMelee_ApplyFacing(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState)
{
	if ((ioMeleeState->face_target) && (ioMeleeState->technique_face_direction != AI2cAngle_None)) {
		// face in the correct direction for our attack
		AI2rExecutor_FacingOverride(ioCharacter, ioMeleeState->technique_face_direction, UUcTrue);
		AI2rMovement_StopAiming(ioCharacter);
	} else {
		// we don't need any particular facing for this technique. look at the target though.
		AI2rMovement_LookAtCharacter(ioCharacter, ioMeleeState->target);
	}
}

// finish the current technique
static void AI2iMelee_FinishTechnique(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState)
{
	UUtUns32 itr;

	if (ioMeleeState->current_technique == NULL)
		return;

	ioMeleeState->current_technique = NULL;
	AI2iMeleeState_NewTechnique(ioMeleeState);

	// we have successfully completed a technique! decay the weight factors
	for (itr = 0; itr < ioCharacter->ai2State.meleeProfile->num_actions; itr++) {
		if (ioMeleeState->weight_adjust[itr] == AI2cMelee_WeightAdjustAborted) {
			// if a technique was aborted, we only ignore it for a single iteration
			ioMeleeState->weight_adjust[itr] = 0;
		} else {
			// reduce the penalty iteratively
			ioMeleeState->weight_adjust[itr] *= AI2cMelee_WeightAdjustDecay;
		}
	}
}

// finish the current move and start the next one
static UUtBool AI2iMelee_StartNextMove(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState, AI2tMeleeMove *inMove)
{
	UUtUns32 move_type;
	UUtBool finished;

	if (!inMove->is_broken) {
		move_type = (inMove->move & AI2cMoveType_Mask) >> AI2cMoveType_Shift;
		UUmAssert((move_type >= 0) && (move_type < AI2cMeleeNumMoveTypes));

		// call the move's finish function
		AI2cMeleeMoveTypes[move_type].func_finish(ioCharacter, ioMeleeState, inMove);
	}

	ioMeleeState->move_index++;

	// update our random seeds here, because we've just finished a move
	AI2iMelee_NewRandomSeeds(ioMeleeState);

	if (ioMeleeState->move_index >= ioMeleeState->current_technique->num_moves) {
#if DEBUG_VERBOSE_MELEE
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: done %d moves, technique %s is complete", ioCharacter->player_name,
			ioMeleeState->move_index, ioMeleeState->current_technique->name);
#endif
		// the technique is complete and has no moves left. however we don't finish the technique until 
		// all of our attack moves have completed executing.
		ioMeleeState->current_move = NULL;
		ioMeleeState->current_attackanim = NULL;
		finished = UUcTrue;

	} else {
#if DEBUG_VERBOSE_MELEE
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: continue on to move #%d of technique %s", ioCharacter->player_name,
			ioMeleeState->move_index + 1, ioMeleeState->current_technique->name);
#endif
		// advance to the next move in the technique.
		ioMeleeState->current_move++;

		// start the next move
		move_type = (ioMeleeState->current_move->move & AI2cMoveType_Mask) >> AI2cMoveType_Shift;
		UUmAssert((move_type >= 0) && (move_type < AI2cMeleeNumMoveTypes));

		finished = AI2cMeleeMoveTypes[move_type].func_start(ioCharacter, ioMeleeState, ioMeleeState->current_move);
	}

	AI2iMelee_UpdateFacing(ioCharacter, ioMeleeState);
	AI2iMelee_ApplyFacing(ioCharacter, ioMeleeState);

	return finished;
}

// get the next move
static UUtBool AI2iMelee_NextMove(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState, UUtUns32 *outMove)
{
	if (ioMeleeState->current_technique == NULL) {
		return UUcFalse;
	}

	if (ioMeleeState->move_index + 1 >= ioMeleeState->current_technique->num_moves) {
		return UUcFalse;
	}

	UUmAssert(ioMeleeState->current_move != NULL);
	if (outMove != NULL) {
		*outMove = (ioMeleeState->current_move + 1)->move;
	}
	return UUcTrue;
}

// recalculate random seeds
static void AI2iMelee_NewRandomSeeds(AI2tMeleeState *ioMeleeState)
{
	ioMeleeState->action_seed = UUrRandom();
	ioMeleeState->reaction_seed = UUrRandom();
}

// calculate absolute angle to target
static float AI2iMelee_AngleToTarget(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState)
{
	float angle, delta_x, delta_z;
	UUtUns16 lead_frames;
	M3tVector3D target_vel;

	UUmAssertReadPtr(ioMeleeState->target, sizeof(ONtCharacter));

	// where is the target with respect to us
	MUmVector_Subtract(ioMeleeState->vector_to_target, ioMeleeState->target->location, ioCharacter->location);
	delta_x = ioMeleeState->vector_to_target.x;
	delta_z = ioMeleeState->vector_to_target.z;

	if (ioMeleeState->current_attackanim != NULL) {
		// we must lead the target by some amount in order to strike them
		lead_frames = TRrAnimation_GetFirstAttackFrame(ioMeleeState->current_attackanim);

		ONrCharacter_GetCurrentWorldVelocity(ioMeleeState->target, &target_vel);
		delta_x += target_vel.x * lead_frames;
		delta_z += target_vel.z * lead_frames;
	}

	// calculate the angle to this point
	angle = MUrATan2(delta_x, delta_z);
	UUmTrig_ClipLow(angle);

	return angle;
}

// calculate weighting factors for each attack direction, based on the target's location relative to us
static void AI2iMelee_CalculateDirectionWeights(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
												float inAngle, float *outMainWeights, float *outGenerousWeights)
{
	UUtUns32 itr;
	float attack_angle;

	// work out how far an attack in each direction would be from where we're currently facing
	for (itr = 0; itr < TRcDirection_Max; itr++) {
		if ((itr == TRcDirection_None) || (itr == TRcDirection_360)) {
			outMainWeights[itr] = 1.0f;
			outGenerousWeights[itr] = 1.0f;
			continue;
		}

		attack_angle = inAngle - ioCharacter->facing;
		switch(itr) {
			case TRcDirection_Forwards:
			break;

			case TRcDirection_Left:
				attack_angle -= M3cHalfPi;
			break;

			case TRcDirection_Right:
				attack_angle += M3cHalfPi;
			break;

			case TRcDirection_Back:
				attack_angle -= M3cPi;
			break;

			default:
				UUmAssert(0);
				outMainWeights[itr] = 0;
				outGenerousWeights[itr] = 0;
				continue;
			break;
		}

		UUmTrig_ClipAbsPi(attack_angle);
		attack_angle = (float)fabs(attack_angle);

		if (attack_angle < AI2cMeleeDirectionalAttackMin) {
			outMainWeights[itr] = 1.0f;

		} else if (attack_angle > AI2cMeleeDirectionalAttackMax) {
			outMainWeights[itr] = 0;

		} else {
			outMainWeights[itr] = (attack_angle - AI2cMeleeDirectionalAttackMax) /
				(AI2cMeleeDirectionalAttackMin - AI2cMeleeDirectionalAttackMax);
		}

		if (attack_angle < AI2cMeleeGenerousDirectionalMin) {
			outGenerousWeights[itr] = 1.0f;

		} else if (attack_angle > AI2cMeleeGenerousDirectionalMax) {
			outGenerousWeights[itr] = 0;

		} else {
			outGenerousWeights[itr] = (attack_angle - AI2cMeleeGenerousDirectionalMax) /
				(AI2cMeleeGenerousDirectionalMin - AI2cMeleeGenerousDirectionalMax);
		}
	}

	// we should always at least consider turning to face the target
	outMainWeights[TRcDirection_Forwards] = UUmMax(outMainWeights[TRcDirection_Forwards], AI2cMeleeDirectionalMinFwdWeight);
	outGenerousWeights[TRcDirection_Forwards] = UUmMax(outGenerousWeights[TRcDirection_Forwards], AI2cMeleeDirectionalMinFwdWeight);
}

// check to see whether we will react to or evade an opponent's attack
static void AI2iMelee_CheckEvasionChance(AI2tMeleeState *ioMeleeState, AI2tMeleeProfile *ioMeleeProfile,
										 const TRtAnimation *inAnimation)
{
	UUtUns32 randomval, attack_dmg;
	float notice_percentage, dodge_percentage;
	ONtDifficultyLevel difficulty = ONrPersist_GetDifficulty();

	if (ioMeleeState->target->animCounter != ioMeleeState->considered_react_anim_index) {
		// the target is running an attack move that we haven't yet decided whether
		// to react to.
		ioMeleeState->frames_to_hit = (UUtUns32) -1;
		notice_percentage = ioMeleeProfile->attacknotice_percentage * ONgGameSettings->notice_multipliers[difficulty];

		randomval = (100 * ((UUtUns32) UUrRandom())) / UUcMaxUns16;
		if (randomval >= notice_percentage) {
			// we don't even see this attack coming
			ioMeleeState->react_enable = UUcFalse;
		} else {
			ioMeleeState->react_enable = UUcTrue;
			attack_dmg = TRrAnimation_GetMaximumDamage(inAnimation);

			// work out what our chance of dodging this attack is
			dodge_percentage = (float) ioMeleeProfile->dodge_base_percentage;
			if (attack_dmg < ioMeleeProfile->dodge_damage_threshold) {
				dodge_percentage += (attack_dmg * ioMeleeProfile->dodge_additional_percentage)
											/ ioMeleeProfile->dodge_damage_threshold;
			} else {
				dodge_percentage += ioMeleeProfile->dodge_additional_percentage;
			}

			dodge_percentage *= ONgGameSettings->dodge_multipliers[difficulty];

			// do we try to dodge the attack?
			randomval = (100 * ((UUtUns32) UUrRandom())) / UUcMaxUns16;
			ioMeleeState->evade_enable = (randomval < dodge_percentage);
		}

		ioMeleeState->considered_react_anim_index = ioMeleeState->target->animCounter;
	}
}

// set up the attacker's state in preparation for a call to TRrCheckAnimationBounds
static void AI2iMelee_SetupAttackerState(ONtCharacter *ioCharacter, AI2tMeleeProfile *ioMeleeProfile, TRtCharacterIntersectState *ioState,
										 UUtBool inStartJump, float inJumpVelocity, TRtDirection inDirection, TRtAnimation *inAnimation)
{
	ONtActiveCharacter *active_character;
	float frames_remaining;

	active_character = ONrGetActiveCharacter(ioCharacter);
	UUmAssertReadPtr(active_character, sizeof(*active_character));

	ioState->dirty_position = UUcTrue;

	if (ONrCharacter_InAir(ioCharacter)) {
		// we are already in the air
		ioState->position.inAir = UUcTrue;
		ioState->position.jumped = ((active_character->inAirControl.flags & ONcInAirFlag_Jumped) != 0);
		ioState->position.jumpKeyDown = ((active_character->inputState.buttonIsDown & LIc_BitMask_Jump) != 0);
		ioState->position.numFramesInAir = active_character->inAirControl.numFramesInAir;
		ioState->position.airVelocity = active_character->inAirControl.velocity;
		frames_remaining = ioMeleeProfile->max_jumpframes - active_character->inAirControl.numFramesInAir;
		if (frames_remaining < 0) {
			ioState->position.expectedJumpFrames = 0;
		} else {
			ioState->position.expectedJumpFrames = (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(frames_remaining);
		}
	} else {
		if (inStartJump) {
			// we are considering a jump
			ioState->position.inAir = UUcTrue;
			ioState->position.jumped = UUcTrue;
			ioState->position.numFramesInAir = 0;
			ioState->position.expectedJumpFrames = (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(0.5f *
					(ioMeleeProfile->max_jumpframes + ioMeleeProfile->min_jumpframes));

			ONrCharacter_GetVelocityEstimate(ioCharacter, active_character, &ioState->position.airVelocity);
			ioState->position.airVelocity.y += ioState->jumpAmount;

			switch(inDirection) {
				case TRcDirection_None:
				break;

				case TRcDirection_Forwards:
					ioState->position.airVelocity.z = UUmMax(ioState->position.airVelocity.z, inJumpVelocity);
				break;

				case TRcDirection_Left:
					ioState->position.airVelocity.x = UUmMax(ioState->position.airVelocity.x, inJumpVelocity);
				break;

				case TRcDirection_Right:
					ioState->position.airVelocity.x = UUmMin(ioState->position.airVelocity.x, -inJumpVelocity);
				break;

				case TRcDirection_Back:
					ioState->position.airVelocity.z = UUmMin(ioState->position.airVelocity.z, -inJumpVelocity);
				break;

				default:
					UUmAssert(0);
				break;
			}
		} else {
			// we are on the ground
			ioState->position.inAir = UUcFalse;
		}
	}

	ioState->animFrame = 0;
	ioState->animation = inAnimation;
}

static UUtBool AI2iMelee_GetTransitionLength(AI2tMeleeProfile *ioMeleeProfile, AI2tMeleeMoveState *inCurrentState,
											  UUtUns16 *inCurrentAnimState, AI2tMeleeMoveState inDesiredState, float *ioWeight,
											  float *ioTransitionDistance, float *ioTransitionTime, UUtUns32 *outNumTransitions)
{
	AI2tMeleeTransition *transition;
	AI2tMeleeMoveState desired_state, current_state;
	UUtUns16 anim_state;
	UUtUns32 num_transitions;
	float weight, transition_time, transition_distance;
	UUtBool found, disallow_standing;

	desired_state = inDesiredState;
	num_transitions = 0;
	current_state = *inCurrentState;
	anim_state = *inCurrentAnimState;
	weight = (ioWeight == NULL) ? 0 : *ioWeight;
	transition_time = (ioTransitionTime == NULL) ? 0 : *ioTransitionTime;
	transition_distance = (ioTransitionDistance == NULL) ? 0 : *ioTransitionDistance;

	disallow_standing = UUcFalse;
	do {
		// try to find a transition to the desired state
		found = UUcTrue;
		while (current_state != desired_state) {
			// find an appropriate transition
			transition = AI2iMelee_CheckStateTransition(NULL, ioMeleeProfile, current_state, desired_state, anim_state);
			if (transition == NULL) {
				found = UUcFalse;
				break;
			}

			// apply this transition
			transition_distance += transition->anim_distance;
			transition_time += transition->anim_frames;
			current_state = transition->to_state[0];
			anim_state = transition->anim_tostate;
			weight *= transition->weight_modifier;
			num_transitions++;
		}

		if (found) {
			if (desired_state == inDesiredState) {
				// we found a transition chain successfully
				if (ioWeight != NULL)				*ioWeight = weight;
				if (ioTransitionDistance != NULL)	*ioTransitionDistance = transition_distance;
				if (ioTransitionTime != NULL)		*ioTransitionTime = transition_time;
				if (outNumTransitions != NULL)		*outNumTransitions = num_transitions;

				return UUcTrue;
			} else {
				// we made it to some state, now try getting to the actual desired state
				desired_state = inDesiredState;
			}
		} else {
			if ((!disallow_standing) && (current_state != AI2cMeleeMoveState_Standing) && (desired_state != AI2cMeleeMoveState_Standing)) {
				// try a transition through the standing state, if we can't get there directly
				desired_state = AI2cMeleeMoveState_Standing;
				disallow_standing = UUcTrue;
			} else {
				// give up
#if DEBUG_VERBOSE_TRANSITION
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "* transition search failed from %s (%s) -> %s\n",
					AI2cMeleeMoveStateName[*inCurrentState], ONrAnimStateToString(*inCurrentAnimState), AI2cMeleeMoveStateName[inDesiredState]);
#endif
				return UUcFalse;
			}
		}
	} while (1);
}

static void AI2iMelee_RotateAnimationContext(TRtAnimIntersectContext *ioContext, AI2tConsideredTechnique *ioTechnique)
{
	UUtUns32 itr;

	if (ioTechnique->rotation_direction != TRcDirection_None) {
		// calculate the rotated target matrix that we will use to check the animation hits
		for (itr = 0; itr < 4; itr++) {
			ioContext->current_location_matrix.m[itr][0] = ioTechnique->cos_rotate * ioContext->original_location_matrix.m[itr][0] - ioTechnique->sin_rotate * ioContext->original_location_matrix.m[itr][1];
			ioContext->current_location_matrix.m[itr][1] = ioTechnique->sin_rotate * ioContext->original_location_matrix.m[itr][0] + ioTechnique->cos_rotate * ioContext->original_location_matrix.m[itr][1];
			ioContext->current_location_matrix.m[itr][2] = ioContext->original_location_matrix.m[itr][2];
		}
	}

	// apply the target's location offset to the matrix
	switch(ioTechnique->rotation_direction) {
		case TRcDirection_360:
		break;

		case TRcDirection_None:
		case TRcDirection_Forwards:
			ioContext->current_location_matrix.m[3][1] -= ioTechnique->action_location_offset;
		break;

		case TRcDirection_Left:
			ioContext->current_location_matrix.m[3][0] -= ioTechnique->action_location_offset;
		break;

		case TRcDirection_Right:
			ioContext->current_location_matrix.m[3][0] += ioTechnique->action_location_offset;
		break;

		case TRcDirection_Back:
			ioContext->current_location_matrix.m[3][1] += ioTechnique->action_location_offset;
		break;

		default:
			UUmAssert(0);
		break;
	}

	ioContext->dirty_matrix = UUcTrue;
}

// *** weight factors for melee techniques
#define AI2cWeight_HitBeforeAttackHits			0.3f
#define AI2cWeight_EvasionStillHit				0.3f

// consider the movement required by an attack or throw along the line of the technique, and determine whether it is acceptable
// or would lead us into danger. return of TRUE means discard.
static UUtBool AI2iMelee_WeightLocalMovement(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
											 AI2tMeleeTechnique *inTechnique, float inMovement, float *outMultiplier)
{
	float danger_dist;
	float safety_dist, multiplier = 1.0f;
	UUtBool is_danger, reject = UUcFalse;
#if DEBUG_VERBOSE_WEIGHT || DEBUG_VERBOSE_WEIGHTVAL
	char *dir;
#endif

	if (inTechnique->computed_flags & AI2cTechniqueUserFlag_Fearless) {
		// we ignore danger
		if (outMultiplier) {
			*outMultiplier = 1.0f;
		}
		return UUcFalse;
	}

	if (inMovement > 0) {
		is_danger = ioMeleeState->danger_ahead;
		danger_dist = ioMeleeState->danger_ahead_dist;
#if DEBUG_VERBOSE_WEIGHT || DEBUG_VERBOSE_WEIGHTVAL
		dir = "FWD";
#endif
	} else {
		inMovement = -inMovement;
		is_danger = ioMeleeState->danger_behind;
		danger_dist = ioMeleeState->danger_behind_dist;
#if DEBUG_VERBOSE_WEIGHT || DEBUG_VERBOSE_WEIGHTVAL
		dir = "BACK";
#endif
	}

	if (!is_danger) {
		// nothing to worry about
#if DEBUG_VERBOSE_WEIGHT || DEBUG_VERBOSE_WEIGHTVAL
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - %s: no danger, no change", dir);
#endif
		if (outMultiplier) {
			*outMultiplier = 1.0f;
		}
		return UUcFalse;
	}

	// how close will we get to the danger?
	safety_dist = danger_dist - inMovement;
	if (safety_dist < 0) {
		if (AI2gMelee_ShowCurrentProfile) {
			strcat(AI2gMelee_TechniqueWeightDescription, " DANGER-FWD");
		}
#if DEBUG_VERBOSE_WEIGHT || DEBUG_VERBOSE_WEIGHTVAL
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - %s: movement %f > dangerdist %f -> current 0", dir, inMovement, danger_dist);
#endif
		multiplier = 0.0f;
		reject = UUcTrue;

	} else if (safety_dist < AI2cMelee_LocalAwareness_Safety) {
		if (AI2gMelee_ShowCurrentProfile) {
			strcat(AI2gMelee_TechniqueWeightDescription, " WORRY-FWD");
		}
		multiplier = safety_dist / AI2cMelee_LocalAwareness_Safety;
#if DEBUG_VERBOSE_WEIGHT || DEBUG_VERBOSE_WEIGHTVAL
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - %s: movement %f < dangerdist %f -> multiplier %f",
								dir, inMovement, danger_dist, multiplier);
#endif

	} else {
#if DEBUG_VERBOSE_WEIGHT || DEBUG_VERBOSE_WEIGHTVAL
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - %s: movement %f << dangerdist %f -> no change", dir, inMovement, danger_dist);
#endif
	}

	if (outMultiplier) {
		*outMultiplier = multiplier;
	}

	return reject;
}

// check to see if the target is throwable
static UUtBool AI2iMelee_TargetIsThrowable(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	if (ioCharacter->characterClass->knockdownResistant) {
		return UUcFalse;
	}

	if (ONrCharacter_IsUnstoppable(ioCharacter)) {
		return UUcFalse;
	}

	if (TRrAnimation_TestFlag(ioActiveCharacter->animation, ONcAnimFlag_Invulnerable)) {
		return UUcFalse;
	}

	if (TRrAnimation_IsInvulnerable(ioActiveCharacter->animation, ioActiveCharacter->animFrame)) {
		return UUcFalse;
	}

	return UUcTrue;
}

// consider and weight an attacking technique
static void AI2iMelee_WeightTechnique(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState, AI2tMeleeProfile *ioMeleeProfile,
									  TRtAnimIntersectContext *ioContext, UUtBool inConsiderAttack, TRtDirection inDirection,
									  AI2tMeleeMoveState inCurrentState,  TRtAnimState inCurrentAnimState, 
									  UUtBool inCanBlock, UUtBool inBlockLow, UUtBool inBlockHigh,
									  struct AI2tConsideredTechnique *ioTechnique)
{
	UUtError error;
	UUtUns8 localmove_weight;
	UUtUns16 target_state, target_varient, animtype;
	UUtUns32 itr, try_state, move_index, move_type, move_internalindex, num_transitions;
	UUtBool already_have_weapon, has_pistol, has_rifle, do_rotation, can_delay, found_transition, skip_positioning, reject;
	UUtBool thrown_into_danger, localmove_ok, localmove_escape, currently_behind_target, ignore_target_velocity;
	float adjust_distance, adjust_frames, target_rel_dir, costheta, sintheta;
	float predelay_frames, postdelay_frames, delay_frames;
	float predelay_distance, postdelay_distance, delay_distance, multiplier;
	float test_dist, horiz_dist, target_dist, delay_velocity, target_closing_velocity;
	float min_delay, max_delay, delay_tolerance, move_dist, target_rel_angle, angle_to_edge, d_fwd, d_perp;
	M3tVector3D target_pt, target_vel, vel_estimate, localmove_pt, localmove_stop;
	TRtAnimState current_animstate;
	TRtAnimation *try_animation;
	TRtPosition *position_pt;
	AI2tMeleeManeuverDefinition *maneuver_def;
	AI2tMeleeMoveState current_movestate;
	AI2tMeleeMove *move, *position_move;
	AI2tMelee_PositionMove *position_movedata, *new_pos_move;
	AI2tMeleeTechnique *technique = ioTechnique->technique;
	ONtActiveCharacter *active_target;
#if DEBUG_VERBOSE_WEIGHT
	AI2tMeleeMoveState orig_state;
#endif

#if DEBUG_VERBOSE_WEIGHT
	if (inConsiderAttack) {
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "*** %s: target %s blocking%s%s, cur state %s / animstate %s",
				technique->name, (inCanBlock) ? "is" : "not",
				(inCanBlock && inBlockLow) ? " (low)" : "",
				(inCanBlock && inBlockHigh) ? " (high)" : "", AI2cMeleeMoveStateName[inCurrentState],
				ONrAnimStateToString(inCurrentAnimState));
	}
#endif

	// don't bother with empty techniques
	if (technique->num_moves == 0) {
		if (inConsiderAttack && AI2gMelee_ShowCurrentProfile) {
			strcat(AI2gMelee_TechniqueWeightDescription, " EMPTY");
		}
#if DEBUG_VERBOSE_WEIGHTVAL
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - empty -> current 0");
#endif
		ioTechnique->weight = 0;
		return;
	}

	if (technique->computed_flags & AI2cTechniqueComputedFlag_IsAttack) {
		if (!inConsiderAttack) {
			// don't consider attacks as reactions
#if DEBUG_VERBOSE_WEIGHTVAL
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - attack/cons react -> current 0");
#endif
			ioTechnique->weight = 0;
			return;
		}

		// weight this technique based on whether we think we'll be blocked
		if (inCanBlock) {
			// our opponent is looking defensive and might block us
			if (((!inBlockHigh) && (technique->computed_flags & AI2cTechniqueComputedFlag_IsHigh)) ||
				((!inBlockLow)	&& (technique->computed_flags & AI2cTechniqueComputedFlag_IsLow))) {
				// our opponent can't block this attack without changing stance
				ioTechnique->weight *= ioMeleeProfile->weight_notblocking_changestance;
#if DEBUG_VERBOSE_WEIGHTVAL
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - changestance %f -> current %f",
					ioMeleeProfile->weight_notblocking_changestance, ioTechnique->weight);
#endif
				if (AI2gMelee_ShowCurrentProfile) {
					strcat(AI2gMelee_TechniqueWeightDescription, " CHANGESTANCE");
				}
#if DEBUG_VERBOSE_WEIGHT
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  would have to change stance to block\n");
#endif
			} else if (technique->computed_flags & AI2cTechniqueComputedFlag_Unblockable) {
				// our opponent could block us, but this attack is unblockable
				ioTechnique->weight *= ioMeleeProfile->weight_blocking_unblockable;
#if DEBUG_VERBOSE_WEIGHTVAL
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - unblockable %f -> current %f",
					ioMeleeProfile->weight_blocking_unblockable, ioTechnique->weight);
#endif
				if (AI2gMelee_ShowCurrentProfile) {
					strcat(AI2gMelee_TechniqueWeightDescription, " UNBLOCKABLE");
				}
#if DEBUG_VERBOSE_WEIGHT
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  is unblockable\n");
#endif
			} else if (technique->computed_flags & AI2cTechniqueComputedFlag_HasStagger) {
				// our opponent could block us, but this attack has a stagger animation
				ioTechnique->weight *= ioMeleeProfile->weight_blocking_stagger;
#if DEBUG_VERBOSE_WEIGHTVAL
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - stagger %f -> current %f",
					ioMeleeProfile->weight_blocking_stagger, ioTechnique->weight);
#endif
				if (AI2gMelee_ShowCurrentProfile) {
					strcat(AI2gMelee_TechniqueWeightDescription, " STAGGER");
				}
#if DEBUG_VERBOSE_WEIGHT
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  has stagger\n");
#endif
			} else if (technique->computed_flags & AI2cTechniqueComputedFlag_HasBlockStun) {
				// our opponent could block us, but this attack has some blockstun
				ioTechnique->weight *= ioMeleeProfile->weight_blocking_blockstun;
#if DEBUG_VERBOSE_WEIGHTVAL
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - blockstun %f -> current %f",
					ioMeleeProfile->weight_blocking_blockstun, ioTechnique->weight);
#endif
				if (AI2gMelee_ShowCurrentProfile) {
					strcat(AI2gMelee_TechniqueWeightDescription, " BLOCKSTUN");
				}
#if DEBUG_VERBOSE_WEIGHT
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  has blockstun\n");
#endif
			} else {
				// our opponent could block us.
				ioTechnique->weight *= ioMeleeProfile->weight_blocking;
#if DEBUG_VERBOSE_WEIGHTVAL
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - blocked %f -> current %f",
					ioMeleeProfile->weight_blocking, ioTechnique->weight);
#endif
				if (AI2gMelee_ShowCurrentProfile) {
					strcat(AI2gMelee_TechniqueWeightDescription, " BLOCKED");
				}
#if DEBUG_VERBOSE_WEIGHT
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  would currently be blocked\n");
#endif
			}
		} else {
			// our opponent is not currently in a position to block us
			ioTechnique->weight *= ioMeleeProfile->weight_notblocking_any;
#if DEBUG_VERBOSE_WEIGHTVAL
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - notblocked %f -> current %f",
				ioMeleeProfile->weight_notblocking_any, ioTechnique->weight);
#endif
			if (AI2gMelee_ShowCurrentProfile) {
				strcat(AI2gMelee_TechniqueWeightDescription, " UNBLOCKED");
			}
#if DEBUG_VERBOSE_WEIGHT
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  could not currently be blocked\n");
#endif
		}

		// FIXME: knockback - check for danger behind them and weight up

	} else if (technique->computed_flags & AI2cTechniqueComputedFlag_IsThrow) {
		if (!inConsiderAttack) {
			// don't consider throws as reactions
#if DEBUG_VERBOSE_WEIGHTVAL
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - throw/cons react -> current 0");
#endif
			ioTechnique->weight = 0;
			return;
		}

		has_pistol = has_rifle = UUcFalse;
		already_have_weapon = (ioCharacter->inventory.weapons[0] != NULL);
		if (ioMeleeState->target->inventory.weapons[0] != NULL) {
			if (WPrGetClass(ioMeleeState->target->inventory.weapons[0])->flags & WPcWeaponClassFlag_TwoHanded) {
				has_rifle = UUcTrue;
			} else {
				has_pistol = UUcTrue;
			}
		}

		if (technique->computed_flags & AI2cTechniqueComputedFlag_IsPistolDisarm) {
			if (!has_pistol || already_have_weapon) {
				// we can't run this disarm move
#if DEBUG_VERBOSE_WEIGHTVAL
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - pistol disarm, weapon criteria unfulfilled -> current 0");
#endif
				ioTechnique->weight = 0;
				return;
			}
		} else if (technique->computed_flags & AI2cTechniqueComputedFlag_IsRifleDisarm) {
			if (!has_pistol || already_have_weapon) {
				// we can't run this disarm move
#if DEBUG_VERBOSE_WEIGHTVAL
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - rifle disarm, weapon criteria unfulfilled -> current 0");
#endif
				ioTechnique->weight = 0;
				return;
			}
		}

		// FIXME: throw distance - check for danger behind them and weight up
	}

	// work out whereabouts we would be approaching the target from
	target_rel_dir = ioMeleeState->target->facing - ioCharacter->facing;
	UUmTrig_ClipAbsPi(target_rel_dir);
	target_rel_dir = (float)fabs(target_rel_dir);

	currently_behind_target = (target_rel_dir < M3cHalfPi);

	if (technique->computed_flags & AI2cTechniqueComputedMask_TargetDirSensitive) {
		if (currently_behind_target) {
			// approaching from behind
			if (technique->computed_flags & AI2cTechniqueComputedFlag_FromFront) {
#if DEBUG_VERBOSE_WEIGHTVAL
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - from front but we're behind -> current 0");
#endif
				if (AI2gMelee_ShowCurrentProfile) {
					strcat(AI2gMelee_TechniqueWeightDescription, " NOT-FRONT");
				}
				ioTechnique->weight = 0;
				return;
			}

			multiplier = (M3cHalfPi - target_rel_dir) / AI2cMelee_ThrowWeightInterpAngle;
			multiplier = UUmMax(multiplier, 0);
			if (multiplier < 1.0f) {
				ioTechnique->weight *= multiplier;
#if DEBUG_VERBOSE_WEIGHTVAL
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - from behind, angle %f almost side, mult %f -> current %f",
						target_rel_dir * M3cRadToDeg, multiplier, ioTechnique->weight);
#endif
				if (AI2gMelee_ShowCurrentProfile) {
					strcat(AI2gMelee_TechniqueWeightDescription, " ALMOST-FRONT");
				}
			}
		} else {
			// approaching from the front
			if (technique->computed_flags & AI2cTechniqueComputedFlag_FromBehind) {
#if DEBUG_VERBOSE_WEIGHTVAL
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - from behind but we're front -> current 0");
#endif
				if (AI2gMelee_ShowCurrentProfile) {
					strcat(AI2gMelee_TechniqueWeightDescription, " NOT-BEHIND");
				}
				ioTechnique->weight = 0;
				return;
			}

			multiplier = (target_rel_dir - M3cHalfPi) / AI2cMelee_ThrowWeightInterpAngle;
			multiplier = UUmMax(multiplier, 0);
			if (multiplier < 1.0f) {
				ioTechnique->weight *= multiplier;
#if DEBUG_VERBOSE_WEIGHTVAL
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - from front, angle %f almost side, mult %f -> current %f",
						target_rel_dir * M3cRadToDeg, multiplier, ioTechnique->weight);
#endif
				if (AI2gMelee_ShowCurrentProfile) {
					strcat(AI2gMelee_TechniqueWeightDescription, " ALMOST-FRONT");
				}
			}
		}
	}

	// FIXME: check AI2cTechniqueComputedFlag_RequireNoRifle and AI2cTechniqueComputedFlag_RequireRifle if this ever gets added to animations

	// copy the original matrix and position into our intersection context
	ONrRevertAnimIntersectionContext(ioContext);

	if (inConsiderAttack) {
		// where is our target relative to us?
		target_pt = MUrMatrix_GetTranslation(&ioContext->current_location_matrix);
		target_dist = ioMeleeState->distance_to_target - ioContext->target_cylinder_radius;
		MUrMatrix3x3_MultiplyPoint(&ioContext->target_velocity_estimate, (M3tMatrix3x3 *) &ioContext->current_location_matrix,
									&target_vel);

		// work out how fast our target is closing with us.
		switch(inDirection) {
			case TRcDirection_360:
				target_closing_velocity = 0;
			break;

			case TRcDirection_None:
			case TRcDirection_Forwards:
				target_closing_velocity = -target_vel.y;
			break;

			case TRcDirection_Left:
				target_closing_velocity = -target_vel.x;
			break;

			case TRcDirection_Right:
				target_closing_velocity = target_vel.x;
			break;

			case TRcDirection_Back:
				target_closing_velocity = target_vel.y;
			break;

			default:
				UUmAssert(0);
			break;
		}
	}

	// zero the cached information about how we set up the matrix for bounds checking
	ioTechnique->action_location_offset = 0;
	ioTechnique->num_jump_frames = (ioContext->attacker.position.inAir) ? ioContext->attacker.position.numFramesInAir : 0;
	ioTechnique->ends_in_jump = UUcFalse;
	ioTechnique->jump_velocity = 0;
	ioTechnique->cos_rotate = 1.0f;
	ioTechnique->sin_rotate = 0.0f;
	ioTechnique->rotation_direction = TRcDirection_None;

	// set up initial values for the start of the consideration process
	position_movedata = NULL;
	can_delay = UUcFalse;
	ignore_target_velocity = UUcFalse;
	predelay_frames = postdelay_frames = delay_frames = 0;
	predelay_distance = postdelay_distance = delay_distance = 0;
	delay_velocity = 0;
	current_movestate = inCurrentState;
	current_animstate = inCurrentAnimState;

	// step through the technique modifying its weight according to each kind of move
	for (itr = 0; itr < technique->num_moves; itr++) {
		move_index = technique->start_index + itr;
		UUmAssert((move_index >= 0) && (move_index < ioMeleeProfile->num_moves));

		move = ioMeleeProfile->move + move_index;
		move_type = move->move & AI2cMoveType_Mask;

#if DEBUG_VERBOSE_WEIGHT
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  + check move #%d (0x%08X) (%s)\n", itr + 1, move->move,
			(move->animation == NULL) ? "<null>" : TMrInstance_GetInstanceName(move->animation));
#endif
		switch(move_type) {
			case AI2cMoveType_Attack:
				if (!inConsiderAttack) {
					// attack moves have no place in reaction techniques
#if DEBUG_VERBOSE_WEIGHTVAL
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - attack/consreact -> current 0");
#endif
					ioTechnique->weight = 0;
					return;
				}

				if ((move->animation == NULL) || (!TRrAnimation_IsAttack(move->animation))) {
					// this animation is broken!
					AI2_ERROR(AI2cBug, AI2cSubsystem_Melee, AI2cError_Melee_BrokenAnim, ioCharacter,
						ioMeleeState, ioTechnique->technique, move_index, ioMeleeProfile);
					if (AI2gMelee_ShowCurrentProfile) {
						strcat(AI2gMelee_TechniqueWeightDescription, " BROKEN");
					}
#if DEBUG_VERBOSE_WEIGHTVAL
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - broken -> current 0");
#endif
					ioTechnique->weight = 0;
					return;
				}
#if DEBUG_VERBOSE_WEIGHT
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "    move is an attack\n");
#endif

				if (TRrAnimation_TestFlag(move->animation, ONcAnimFlag_DisableShield)) {
					UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Combat);
					if (AI2rCombat_WaitingForShieldCharge(ioCharacter, &ioCharacter->ai2State.currentState->state.combat)) {
						// we cannot throw this move, we are waiting for our shield to recharge
						if (AI2gMelee_ShowCurrentProfile) {
							strcat(AI2gMelee_TechniqueWeightDescription, " SHIELDCHARGE");
						}
#if DEBUG_VERBOSE_WEIGHTVAL
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - waiting for shield to charge -> current 0");
#endif
						ioTechnique->weight = 0;
						return;
					}
				}

				// store this attack animation so that we know how much to lead our target by
				if (ioTechnique->attack_move == NULL) {
					ioTechnique->attack_moveindex = itr;
					ioTechnique->attack_move = move;
				}

				// we need to adjust our position by this much when computing bounds, because we will
				// be computing bounds at some hypothetical moment in the future
				adjust_frames = predelay_frames + postdelay_frames;
				adjust_distance = predelay_distance + postdelay_distance;

				/*
				 * determine whether to delay, and by how much
				 */

				if (can_delay) {
					// work out the minimum distance that we need to the target in order to run this attack
					test_dist = target_dist - adjust_distance;
					if (ioTechnique->ends_in_jump) {
						// this is a jumping attack, and we must have room to at least get to the midpoint of our jump
						test_dist -= ioTechnique->jump_velocity * 0.5f * ioMeleeProfile->min_jumpframes;
					}

					if (ioTechnique->positionmove_skip != (UUtUns16) -1) {
#if DEBUG_VERBOSE_WEIGHTVAL
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - maybe skip-position, don't check mindelay");
#endif
					} else if (min_delay < AI2cMelee_PositionIgnoreMinDistance) {
#if DEBUG_VERBOSE_WEIGHTVAL
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - mindelay 0, don't check");
#endif
					} else {
						if (test_dist < min_delay) {
							if (AI2gMelee_ShowCurrentProfile) {
								strcat(AI2gMelee_TechniqueWeightDescription, " <<DELAY");
							}
#if DEBUG_VERBOSE_WEIGHT
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "    delay %f < min %f\n", test_dist, min_delay);
#endif
#if DEBUG_VERBOSE_WEIGHTVAL
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - <<delay (%f, %f) -> current 0", test_dist, min_delay);
#endif
							ioTechnique->weight = 0;
							return;
						} else if (target_dist < min_delay + delay_tolerance) {
							if (AI2gMelee_ShowCurrentProfile) {
								strcat(AI2gMelee_TechniqueWeightDescription, " <DELAY");
							}
#if DEBUG_VERBOSE_WEIGHTVAL
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - <delay (%f, %f [%f]), mult %f -> current %f",
								test_dist, min_delay, delay_tolerance, (test_dist - min_delay) / delay_tolerance,
								ioTechnique->weight);
#endif
							ioTechnique->weight *= (test_dist - min_delay) / delay_tolerance;
						}
					}

					// work out how far we would have to delay in order to hit the target nicely. this is the distance
					// that we would have to close minus the distance that the target would cover while
					// we're outside the delay period minus the distance that we want to be at when we attack. we cover this
					// distance at a speed of delay_velocity + target_closing_velocity.
					skip_positioning = UUcFalse;
					delay_distance = target_dist - ioContext->target_cylinder_radius;
					if (ioTechnique->ends_in_jump) {
						// this is a jumping attack, and we want to be at roughly the midpoint of our jump range from
						// the target when we attack.
						// FIXME: do we want to be some fraction of our jump range away? like 0.75?
						delay_distance -= ioTechnique->jump_velocity *
							0.5f * (ioMeleeProfile->min_jumpframes + ioMeleeProfile->max_jumpframes);
					} else {
						// this is a ground-based attack, so we want to be at a set distance when we attack
						delay_distance -= technique->attack_idealrange;

						if ((ioTechnique->positionmove_skip != (UUtUns16) -1) && (delay_distance < -AI2cMelee_SkipPositioningDistance)) {
							// we do not run the positioning move, we are already close enough to run our attack now
							skip_positioning = UUcTrue;
#if DEBUG_VERBOSE_WEIGHTVAL
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - delay-distance %f < %f, skip positioning move (%d)",
													delay_distance, -AI2cMelee_SkipPositioningDistance, ioTechnique->positionmove_skip);
#endif
						}						
					}

					if (skip_positioning) {
						adjust_distance = 0;
						adjust_frames = 0;
#if DEBUG_VERBOSE_WEIGHTVAL
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - skipping positioning move, no delay check");
#endif
					} else {
						if (ioTechnique->positionmove_skip != (UUtUns16) -1) {
							// do not skip this positioning move after all, we are too far away
#if DEBUG_VERBOSE_WEIGHTVAL
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - attack is too far from here DON'T skip positioning move (%d)",
													delay_distance, -AI2cMelee_SkipPositioningDistance, ioTechnique->positionmove_skip);
#endif
							ioTechnique->positionmove_skip = (UUtUns16) -1;
						}
						// do not skip the positioning move
						ioTechnique->positionmove_skip = (UUtUns16) -1;

						// subtract the distance caused by the start and end of the positioning move
						delay_distance -= adjust_distance;
						if (!ignore_target_velocity) {
							delay_distance -= adjust_frames * target_closing_velocity;
						}
						delay_distance = UUmMax(delay_distance, 0);
						
						// how does this affect our technique's weight?
						if (delay_distance > max_delay) {
							// outside acceptable bounds
							if (AI2gMelee_ShowCurrentProfile) {
								strcat(AI2gMelee_TechniqueWeightDescription, " >>DELAY");
							}
#if DEBUG_VERBOSE_WEIGHT
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "    delay %f > max %f\n", delay_distance, max_delay);
#endif
#if DEBUG_VERBOSE_WEIGHTVAL
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - >>delay (%f, %f) -> current 0", delay_distance, max_delay);
#endif
							ioTechnique->weight = 0;
							return;

						} else if (delay_distance > max_delay - delay_tolerance) {
							if (AI2gMelee_ShowCurrentProfile) {
								strcat(AI2gMelee_TechniqueWeightDescription, " >DELAY");
							}
							ioTechnique->weight *= (max_delay - delay_distance) / delay_tolerance;
#if DEBUG_VERBOSE_WEIGHTVAL
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - >delay (%f, %f [%f]), mult %f -> current %f",
								delay_distance, max_delay, delay_tolerance, (max_delay - delay_distance) / delay_tolerance,
								ioTechnique->weight);
#endif
						}

						// work out how long (in frames) we would have to delay for, in order to achieve this distance of delay.
						if (ignore_target_velocity) {
							delay_frames = delay_distance / delay_velocity;
						} else {
							delay_frames = delay_distance / (delay_velocity + target_closing_velocity);
						}

						if (delay_frames < 0) {
							if (AI2gMelee_ShowCurrentProfile) {
								strcat(AI2gMelee_TechniqueWeightDescription, " CANTCATCH");
							}
							ioTechnique->weight = 0;
#if DEBUG_VERBOSE_WEIGHT
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "    target is heading away from us at %f, can't catch up delay distance %f at vel %f\n",
													(ignore_target_velocity) ? 0 : target_closing_velocity, delay_distance, delay_velocity);
#endif
#if DEBUG_VERBOSE_WEIGHTVAL
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - nocatch -> current 0");
#endif
							return;
						} else if (delay_frames > AI2cMelee_MaxDelayFrames) {
							if (AI2gMelee_ShowCurrentProfile) {
								strcat(AI2gMelee_TechniqueWeightDescription, " >DELAYTIME");
							}
							ioTechnique->weight = 0;
#if DEBUG_VERBOSE_WEIGHT
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "    delay %f w/ ourvel %f closing %f requires time %f > max %d\n", delay_distance,
													delay_velocity, (ignore_target_velocity) ? 0 : target_closing_velocity, delay_frames, AI2cMelee_MaxDelayFrames);
#endif
#if DEBUG_VERBOSE_WEIGHTVAL
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - catchframes %d > %d -> current 0", delay_frames, AI2cMelee_MaxDelayFrames);
#endif
							return;
						}
						
						adjust_distance += delay_distance;
						adjust_frames += delay_frames;
					}

					// check to make sure that we won't encounter danger in performing this move
					test_dist = adjust_distance + move->attack_endpos;
					reject = AI2iMelee_WeightLocalMovement(ioCharacter, ioMeleeState, ioTechnique->technique, test_dist, &multiplier);
					ioTechnique->weight *= multiplier;
					if (reject) {
						return;
					}

				} else if (ioTechnique->ends_in_jump) {
					// this is a jumping technique without a delay; we must at least have room to get to
					// the midpoint of our jump
					test_dist = adjust_distance + ioTechnique->jump_velocity * 0.5f * ioMeleeProfile->min_jumpframes + ioContext->target_cylinder_radius;
					if (!ignore_target_velocity) {
						test_dist += adjust_frames * target_closing_velocity;
					}

					if (target_dist < test_dist) {
						if (AI2gMelee_ShowCurrentProfile) {
							strcat(AI2gMelee_TechniqueWeightDescription, " <REQDIST");
						}
#if DEBUG_VERBOSE_WEIGHT
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "    distance %f < required %f (%f %f %f %f)\n", target_dist, test_dist,
							adjust_distance, (ignore_target_velocity) ? 0 : (adjust_frames * target_closing_velocity),
							ioTechnique->jump_velocity * 0.5f * ioMeleeProfile->min_jumpframes,
							ioContext->target_cylinder_radius);
#endif
#if DEBUG_VERBOSE_WEIGHTVAL
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - jumpdistcheck %f < %f -> current 0", target_dist, test_dist);
#endif
						ioTechnique->weight = 0;
						return;
					}					

					// check to make sure that we won't encounter danger in performing this move
					test_dist = adjust_distance + ioTechnique->jump_velocity * ioMeleeProfile->min_jumpframes + move->attack_endpos;
					reject = AI2iMelee_WeightLocalMovement(ioCharacter, ioMeleeState, ioTechnique->technique, test_dist, &multiplier);
					ioTechnique->weight *= multiplier;
					if (reject) {
						return;
					}
				}

				if (ioMeleeState->frames_to_hit != (UUtUns32) -1) {
					if (adjust_frames + TRrAnimation_GetFirstAttackFrame(move->animation) < ioMeleeState->frames_to_hit) {
						// this attack will hit before our opponent's attack hits us
						ioTechnique->attack_before_hit = UUcTrue;
					} else {
						// this attack will not have time to execute before we get hit by our opponent
						ioTechnique->weight *= AI2cWeight_HitBeforeAttackHits;
#if DEBUG_VERBOSE_WEIGHTVAL
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - hitbeforestrike, mult %f -> current %f",
							AI2cWeight_HitBeforeAttackHits, ioTechnique->weight);
#endif
					}
				}

				if ((adjust_frames > 0) && (!ignore_target_velocity)) {
					// take account of how we expect the target to move between now and when we perform the check
					// in the future
					MUmVector_ScaleIncrement(target_pt, adjust_frames, target_vel);
				}

				/*
				 * rotate the animation-checking matrix to face the target
				 */

				// we can't rotate our facing in midair.
				do_rotation = (!ioContext->attacker.position.inAir);
				if (do_rotation) {
					ioTechnique->rotation_direction = inDirection;

					// based on the direction of our attack, determine what the rotation is to point our attack
					// directly at the target
					horiz_dist = MUrSqrt(UUmSQR(target_pt.x) + UUmSQR(target_pt.y));
					switch(ioTechnique->rotation_direction) {
						case TRcDirection_360:
						case TRcDirection_Forwards:
							ioTechnique->sin_rotate =  target_pt.x / horiz_dist;
							ioTechnique->cos_rotate =  target_pt.y / horiz_dist;
						break;

						case TRcDirection_Left:
							ioTechnique->sin_rotate = -target_pt.y / horiz_dist;
							ioTechnique->cos_rotate =  target_pt.x / horiz_dist;
						break;

						case TRcDirection_Back:
							ioTechnique->sin_rotate = -target_pt.x / horiz_dist;
							ioTechnique->cos_rotate = -target_pt.y / horiz_dist;
						break;

						case TRcDirection_Right:
							ioTechnique->sin_rotate =  target_pt.y / horiz_dist;
							ioTechnique->cos_rotate = -target_pt.x / horiz_dist;
						break;

						default:
							UUmAssert(0);
						break;
					}

					// weight down the technique based on how far we would have to turn
					ioTechnique->weight *= (float)fabs(ioTechnique->cos_rotate);
				} else {
					ioTechnique->rotation_direction = TRcDirection_None;
					ioTechnique->sin_rotate = 0.0f;
					ioTechnique->cos_rotate = 1.0f;
				}

				// calculate the offset of the target's location matrix... this is
				// the amount of distance towards them that our positioning moves
				// will eat up before we get a chance to attack. also offset this by the distance that we expect them to
				// move before we get a chance to attack.
				ioTechnique->action_location_offset = adjust_distance;
				if (!ignore_target_velocity) {
					ioTechnique->action_location_offset += adjust_frames * target_closing_velocity;
				}

				// FIXME: modify our estimated velocity so that we check for enemy attacks properly

				// set up the attacker's state
#if DEBUG_VERBOSE_WEIGHT
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "    checking bounds: jump %s, velocity %f\n", ioTechnique->ends_in_jump ? "yes" : "no",
									ioTechnique->jump_velocity);
#endif
				AI2iMelee_RotateAnimationContext(ioContext, ioTechnique);
				AI2iMelee_SetupAttackerState(ioCharacter, ioMeleeProfile, &ioContext->attacker,
											ioTechnique->ends_in_jump, ioTechnique->jump_velocity,
											inDirection, move->animation);

#if DEBUG_SHOWINTERSECTIONS
				TRgStoreThisIntersection = MELEE_ACTINTERSECTIONS;
#endif
				// check to see if we might hit with this attack - TRrIntersectAnimations will be done later on as a
				// second pass
				if (!TRrCheckAnimationBounds(ioContext)) {
					if (AI2gMelee_ShowCurrentProfile) {
						strcat(AI2gMelee_TechniqueWeightDescription, " MISSBOUNDS");
					}
					ioTechnique->weight = 0;
#if DEBUG_VERBOSE_WEIGHTVAL
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - boundsmiss -> current 0");
#endif
					return;
				}

				// we can't check anything useful past the first attack in a technique. exit with the
				// current weight.
				return;
			break;

			case AI2cMoveType_Position:
				// get data for this move
				position_move = move;
				move_internalindex = move->move & ~AI2cMoveType_Mask;
				UUmAssert((move_internalindex >= 0) && (move_internalindex < AI2cMeleeMaxPositionMoves));
				new_pos_move = &AI2gMeleePositionTable[move_internalindex];
				UUmAssert(new_pos_move->num_states > 0);

				if (!inConsiderAttack && new_pos_move->can_delay) {
					// delaying positioning moves have no place in a reaction technique
#if DEBUG_VERBOSE_WEIGHTVAL
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - position/consreact -> current 0");
#endif
					ioTechnique->weight = 0;
					return;
				}

				if ((position_movedata != NULL) && (new_pos_move->can_delay)) {
					// a delaying positioning move must always be the first positioning move in a technique - bail.
#if DEBUG_VERBOSE_WEIGHT
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "    ERROR: multiple positioning moves in technique!\n");
#endif
#if DEBUG_VERBOSE_WEIGHTVAL
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - multi-posn -> current 0");
#endif
					ioTechnique->weight = 0;
					return;
				}

				position_movedata = new_pos_move;

#if DEBUG_VERBOSE_WEIGHT
				{
					UUtUns32 itr2;
					char printline[256];

					strcpy(printline, "    move is positioning(");
					for (itr2 = 0; itr2 < position_movedata->num_states; itr2++) {
						if (itr2 > 0) {
							strcat(printline, ", ");
						}
						strcat(printline, AI2cMeleeMoveStateName[position_movedata->states[itr2]]);
					}
					strcat(printline, ")\n");

					COrConsole_Print(printline);
				}
#endif
				// if this positioning move has no attack attached to it, bail.
				if (((technique->computed_flags & AI2cTechniqueComputedFlag_SpecialCaseMove) == 0) &&
					(technique->attack_anim == NULL) && (technique->throw_distance == 0)) {
					if (AI2gMelee_ShowCurrentProfile) {
						strcat(AI2gMelee_TechniqueWeightDescription, " NOATTACKNOTHROW");
					}
#if DEBUG_VERBOSE_WEIGHT
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "    technique has no attack anim or throw, abort\n");
#endif
#if DEBUG_VERBOSE_WEIGHTVAL
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - noattackanim/nothrow -> current 0");
#endif
					ioTechnique->weight = 0;
					return;
				}

				if ((position_movedata->jump_type == AI2cMeleeJumpType_Vertical) || (position_movedata->jump_type == AI2cMeleeJumpType_Standing)) {
					// don't take the target's motion into account when computing standing jumps, otherwise it produces
					// stupid-appearing bunny hopping behavior
					ignore_target_velocity = UUcTrue;

					if (position_movedata->jump_type == AI2cMeleeJumpType_Vertical) {
						// we must not perform vertical jumps unless our target is higher than we are by a significant amount,
						// this also prevents needless jumping behavior in melee
						if (ioMeleeState->target->actual_position.y < ioCharacter->actual_position.y + AI2cMelee_StandingJumpRequiresOffset) {
							if (AI2gMelee_ShowCurrentProfile) {
								strcat(AI2gMelee_TechniqueWeightDescription, " NOTLOWER");
							}
#if DEBUG_VERBOSE_WEIGHTVAL
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - not lower than target -> current 0");
#endif
							ioTechnique->weight = 0;
							return;

						}
					}
				}

				// work out what the next state is that we have to get to.
 				for (try_state = 0; try_state < position_movedata->num_states; try_state++) {
					if (current_movestate == position_movedata->states[try_state])
						break;
				}
				if (try_state >= position_movedata->num_states) {
					// our current state is not one of the states in the move. don't bother.
					try_state = 0;
				} else {
					// we are at try_state; the next one we need to get to is try_state + 1
					try_state = try_state + 1;
					if (try_state == position_movedata->num_states) {
						// we are already in the end-state of the positioning move
						if ((!position_movedata->can_delay) || (position_move->param[0] < AI2cMelee_PositionIgnoreMinDistance)) {
							// we can skip this move; either it has no delay, or the delay can be zero length
#if DEBUG_VERBOSE_WEIGHTVAL
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - candelay %s min %f: already in finalstate %d = %d (%s), consider skipping (curskip %d)",
								position_movedata->can_delay ? "yes" : "no", position_move->param[0], try_state, position_movedata->states[try_state],
								AI2cMeleeMoveStateName[position_movedata->states[try_state]], ioTechnique->positionmove_skip);
#endif
							if (ioTechnique->positionmove_skip == (UUtUns16) -1) {
								ioTechnique->positionmove_skip = (UUtUns16) itr;
							}
						}
						try_state = 0;
					}
				}

				if (try_state == 0) {
#if DEBUG_VERBOSE_WEIGHT
					orig_state = current_movestate;
#endif
					found_transition = AI2iMelee_GetTransitionLength(ioMeleeProfile, &current_movestate, &current_animstate,
																	position_movedata->states[try_state], &ioTechnique->weight,
																	&predelay_distance, &predelay_frames, &num_transitions);
					if (!found_transition) {
						// we can't do this positioning move, as we don't have a required transition
						AI2_ERROR(AI2cWarning, AI2cSubsystem_Melee, AI2cError_Melee_NoTransitionToStart, ioCharacter,
							technique, current_movestate, position_movedata->states[try_state], current_animstate);
						if (AI2gMelee_ShowCurrentProfile) {
							strcat(AI2gMelee_TechniqueWeightDescription, " NOTRANS-PRE");
						}
#if DEBUG_VERBOSE_WEIGHT
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "      PREDELAY: no transition chain from %s -> %s, abort\n",
							AI2cMeleeMoveStateName[orig_state], AI2cMeleeMoveStateName[position_movedata->states[try_state]]);
#endif
#if DEBUG_VERBOSE_WEIGHTVAL
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - pretrans -> current 0");
#endif
						ioTechnique->weight = 0;
						return;
					}	

#if DEBUG_VERBOSE_WEIGHT
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "      PREDELAY: transition chain %s -> %s, length %d dist %f frames %f\n",
						AI2cMeleeMoveStateName[orig_state], AI2cMeleeMoveStateName[current_movestate],
						num_transitions, predelay_distance, predelay_frames);
#endif

					// move on to considering the rest of the states
					try_state++;
				}

				if (try_state == 1) {
					// now we are in state 1; here would go any delay (in time and space) that we have to add in in order
					// to make our attack hit.
					if ((can_delay = position_movedata->can_delay) == UUcTrue) {
						if (move->animation == NULL) {
							// we can't do this positioning move, as we don't have the delay animation
#if DEBUG_VERBOSE_WEIGHT
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "    ERROR: positioning move is missing delay animation\n");
#endif
							if (AI2gMelee_ShowCurrentProfile) {
								strcat(AI2gMelee_TechniqueWeightDescription, " NODELAYANIM");
							}
#if DEBUG_VERBOSE_WEIGHTVAL
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - nodelayanim -> current 0");
#endif
							ioTechnique->weight = 0;
							return;
						}

						// positioning move has three parameters. 0 = min dist, 1 = max dist, 2 = fuzzy tolerance
						min_delay = position_move->param[0];
						max_delay = position_move->param[1];
						delay_tolerance = position_move->param[2];

						// work out how fast we will be travelling when we start the delay.
						TRrAnimation_GetPosition(move->animation, 0, &vel_estimate);
						switch(position_movedata->direction) {
							case TRcDirection_None:
								// delay moves must always have a direction
								UUmAssert(0);
							break;

							case TRcDirection_Forwards:
								delay_velocity = vel_estimate.z;
							break;

							case TRcDirection_Left:
								delay_velocity = vel_estimate.x;
							break;

							case TRcDirection_Right:
								delay_velocity = -vel_estimate.x;
							break;

							case TRcDirection_Back:
								delay_velocity = -vel_estimate.z;
							break;

							default:
								UUmAssert(0);
							break;
						}
#if DEBUG_VERBOSE_WEIGHT
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "    positioning move delayanim %s, velocity (%f %f %f) -> %f\n",
							TMrInstance_GetInstanceName(move->animation), vel_estimate.x, vel_estimate.y, vel_estimate.z,
							delay_velocity);
#endif
					}
				}

				// evaluate the transitions required to get to all the states that come after our delay period
				for ( ; try_state < position_movedata->num_states; try_state++) {
#if DEBUG_VERBOSE_WEIGHT
					orig_state = current_movestate;
#endif
					found_transition = AI2iMelee_GetTransitionLength(ioMeleeProfile, &current_movestate, &current_animstate,
																	position_movedata->states[try_state], &ioTechnique->weight,
																	&postdelay_distance, &postdelay_frames, &num_transitions);
					if (!found_transition) {
						// we can't do this positioning move, as we don't have a required transition
						AI2_ERROR(AI2cWarning, AI2cSubsystem_Melee, AI2cError_Melee_NoTransitionToStart, ioCharacter,
							technique, current_movestate, position_movedata->states[try_state], current_animstate);
						if (AI2gMelee_ShowCurrentProfile) {
							strcat(AI2gMelee_TechniqueWeightDescription, " NOTRANS-POST");
						}
#if DEBUG_VERBOSE_WEIGHTVAL
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - noposttrans -> current 0");
#endif
						ioTechnique->weight = 0;
						return;
					}	

#if DEBUG_VERBOSE_WEIGHT
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "      POSTDELAY: transition chain %s -> %s, length %d dist %f frames %f\n",
						AI2cMeleeMoveStateName[orig_state], AI2cMeleeMoveStateName[current_movestate],
						num_transitions, postdelay_distance, postdelay_frames);
#endif
				}

				// work out if we are a jumping move
				if (position_movedata->jump_type != AI2cMeleeJumpType_None) {
					ioTechnique->ends_in_jump = UUcTrue;
					ioTechnique->jump_velocity = ioMeleeProfile->jump_vel[position_movedata->jump_type];
				}

#if DEBUG_VERBOSE_WEIGHT
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "    predelay %f over %f, delay %s (%f), postdelay %f over %f, jump %s (vel %f [frames %f-%f])",
								predelay_distance, predelay_frames, (can_delay) ? "yes" : "no", delay_velocity,
								postdelay_distance, postdelay_frames, ioTechnique->ends_in_jump ? "yes" : "no",
								ioTechnique->jump_velocity, ioMeleeProfile->min_jumpframes, ioMeleeProfile->max_jumpframes);
#endif
			break;

			case AI2cMoveType_Maneuver:
				move_internalindex = move->move & ~AI2cMoveType_Mask;
				UUmAssert((move_internalindex >= 0) && (move_internalindex < AI2cMeleeMaxPositionMoves));
				maneuver_def = &AI2gMeleeManeuverDefinition[move_internalindex];

				if (inConsiderAttack) {
					// consider the ranges of this maneuver move to see if it's appropriate here.
					switch (move_internalindex) {
						case AI2cMeleeManeuver_BarabbasWave:
							if (move->animation == NULL) {
								// this move is broken!
								if (AI2gMelee_ShowCurrentProfile) {
									strcat(AI2gMelee_TechniqueWeightDescription, " BROKEN");
								}
#if DEBUG_VERBOSE_WEIGHTVAL
								COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - barabbaswave noanim, broken -> 0");
#endif
								ioTechnique->weight = 0;
								ioTechnique->technique->computed_flags |= AI2cTechniqueComputedFlag_Broken;
								return;
							}
							if (TRrAnimation_TestFlag(move->animation, ONcAnimFlag_DisableShield)) {
								UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Combat);
								if (AI2rCombat_WaitingForShieldCharge(ioCharacter, &ioCharacter->ai2State.currentState->state.combat)) {
									// we cannot throw this move, we are waiting for our shield to recharge
									if (AI2gMelee_ShowCurrentProfile) {
										strcat(AI2gMelee_TechniqueWeightDescription, " SHIELDCHARGE");
									}
#if DEBUG_VERBOSE_WEIGHTVAL
									COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - waiting for shield to charge -> current 0");
#endif
									ioTechnique->weight = 0;
									return;
								}
							}

							if (ioMeleeState->distance_to_target > move->param[0]) {
								// target is too far away
								if (AI2gMelee_ShowCurrentProfile) {
									strcat(AI2gMelee_TechniqueWeightDescription, " TOOFAR");
								}
#if DEBUG_VERBOSE_WEIGHTVAL
								COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - barabbaswave toofar %f > %f -> 0",
									ioMeleeState->distance_to_target, move->param[0]);
#endif
								ioTechnique->weight = 0;
							}
						break;

						case AI2cMeleeManeuver_Advance:
							move_dist = ioMeleeState->distance_to_target - move->param[1] - move->param[2];

							if (move_dist < 0) {
								// we're already close enough
								if (AI2gMelee_ShowCurrentProfile) {
									strcat(AI2gMelee_TechniqueWeightDescription, " <DIST");
								}
#if DEBUG_VERBOSE_WEIGHTVAL
								COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - advance tooclose %f < %f-%f -> 0",
									ioMeleeState->distance_to_target, move->param[1], move->param[2]);
#endif
								ioTechnique->weight = 0;
								return;
							} else if (move_dist < move->param[2]) {
								// we're almost close enough
								ioTechnique->weight *= move_dist / move->param[2];
#if DEBUG_VERBOSE_WEIGHTVAL
								COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - advance close %f < %f, mult %f -> %f",
									ioMeleeState->distance_to_target, move->param[1],
									move_dist / move->param[2], ioTechnique->weight);
#endif
							} else {
								// this technique's weight is unchanged
							}
						break;

						case AI2cMeleeManeuver_Retreat:
							move_dist = move->param[1] + move->param[2] - ioMeleeState->distance_to_target;

							if (move_dist < 0) {
								// we're already far enough away
								if (AI2gMelee_ShowCurrentProfile) {
									strcat(AI2gMelee_TechniqueWeightDescription, " >DIST");
								}
#if DEBUG_VERBOSE_WEIGHTVAL
								COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - retreat toofar %f > %f+%f -> 0",
									ioMeleeState->distance_to_target, move->param[1], move->param[2]);
#endif
								ioTechnique->weight = 0;
								return;
							} else if (move_dist < move->param[2]) {
								// we're almost far enough away
								ioTechnique->weight *= move_dist / move->param[2];
#if DEBUG_VERBOSE_WEIGHTVAL
								COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - retreat far %f > %f, mult %f -> %f",
									ioMeleeState->distance_to_target, move->param[1],
									move_dist / move->param[2], ioTechnique->weight);
#endif
							} else {
								// this technique's weight is unchanged
							}
						break;

						case AI2cMeleeManeuver_CircleLeft:
						case AI2cMeleeManeuver_CircleRight:
							// work out whereabouts we are relative to the target
							target_rel_angle = ONrCharacter_GetCosmeticFacing(ioMeleeState->target)
								- (ioMeleeState->angle_to_target - M3cPi);
							UUmTrig_ClipAbsPi(target_rel_angle);
							target_rel_angle *= M3cRadToDeg;

							// if we are between the two angles given in the move, then this is a good idea
							if (move->param[1] < move->param[2]) {
								angle_to_edge = UUmMin(target_rel_angle - move->param[1],
														move->param[2] - target_rel_angle);
							} else {
								angle_to_edge = UUmMin(target_rel_angle - move->param[2], 
														move->param[1] - target_rel_angle);
							}

							if (angle_to_edge < 0) {
								// we are outside the valid angle range
								if (AI2gMelee_ShowCurrentProfile) {
									strcat(AI2gMelee_TechniqueWeightDescription, " OUT-ANGLE");
								}
#if DEBUG_VERBOSE_WEIGHTVAL
								COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - angle out %f -> 0", angle_to_edge);
#endif
								ioTechnique->weight = 0;
								return;

							} else if (angle_to_edge < AI2cMelee_CircleAngleTolerance) {
								// we are close to the edge of the valid angle range
								ioTechnique->weight *= angle_to_edge / AI2cMelee_CircleAngleTolerance;
#if DEBUG_VERBOSE_WEIGHTVAL
								COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - angle out %f, mult %f -> %f", angle_to_edge,
									angle_to_edge / AI2cMelee_CircleAngleTolerance, ioTechnique->weight);
#endif

							} else {
								// we are in the middle of the valid angle range. circle.
							}
						break;

						case AI2cMeleeManeuver_RandomStop:
							// immediately stop weighting the technique; return whatever weight we already have
							return;
					
						case AI2cMeleeManeuver_Pause:
						case AI2cMeleeManeuver_Crouch:
						case AI2cMeleeManeuver_Jump:
						case AI2cMeleeManeuver_Taunt:
						case AI2cMeleeManeuver_GetupFwd:
						case AI2cMeleeManeuver_GetupBack:
						case AI2cMeleeManeuver_GetupLeft:
						case AI2cMeleeManeuver_GetupRight:
							// this move is always equally appropriate
						break;

						default:
							UUmAssert(0);
						break;
					}
				} else {
					// maneuvers cannot come as part of a reaction technique.
					ioTechnique->weight = 0;
					return;
				}
			break;

			case AI2cMoveType_Evade:
				if (inConsiderAttack) {
					// evasion moves have no place in an attacking technique
					if (AI2gMelee_ShowCurrentProfile) {
						strcat(AI2gMelee_TechniqueWeightDescription, " EVADE");
					}
					ioTechnique->weight = 0;
#if DEBUG_VERBOSE_WEIGHTVAL
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - evade/consattack -> 0");
#endif
					return;
				}

				// set up the animation context to see what happens if we do this evasion move
				if (move->animation == NULL) {
					// this animation is broken!
					AI2_ERROR(AI2cBug, AI2cSubsystem_Melee, AI2cError_Melee_BrokenAnim, ioCharacter,
						ioMeleeState, ioTechnique->technique, move_index, ioMeleeProfile);
#if DEBUG_VERBOSE_WEIGHTVAL
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - evade-broken -> 0");
#endif
					ioTechnique->weight = 0;
					return;
				}

				// work out where this evasion will take us
				costheta = MUrCos(ioCharacter->facing);				sintheta = MUrSin(ioCharacter->facing);
				position_pt = TRrAnimation_GetPositionPoint(move->animation, TRrAnimation_GetDuration(move->animation) - 1);
				localmove_pt.x = (costheta * position_pt->location_x + sintheta * position_pt->location_y) * TRcPositionGranularity;
				localmove_pt.z = (costheta * position_pt->location_y - sintheta * position_pt->location_x) * TRcPositionGranularity;
				localmove_pt.y = 0;

				// don't choose evasions that take us closer to the target
				costheta = MUrCos(ioMeleeState->target->facing);	sintheta = MUrSin(ioMeleeState->target->facing);
				d_fwd  = sintheta * localmove_pt.x + costheta * localmove_pt.z;
				d_perp = costheta * localmove_pt.x - sintheta * localmove_pt.z;
				multiplier = 1.0f + 2 * d_fwd / (float)fabs(d_perp);
				if (multiplier < 0) {
					// this evasion takes us significantly towards the target
#if DEBUG_VERBOSE_WEIGHTVAL
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - evade is too close to target (fwd %f perp %f) -> mult %f -> current 0",
						d_fwd, d_perp, multiplier);
#endif
					ioTechnique->weight = 0;
					return;

				} else if (multiplier < 1.0f) {
					// this evasion takes us somewhat towards the target
					ioTechnique->weight *= multiplier;
#if DEBUG_VERBOSE_WEIGHTVAL
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - evade is close to target (fwd %f perp %f) -> mult %f -> current %f",
						d_fwd, d_perp, multiplier, ioTechnique->weight);
#endif
				}

				// is it safe to get there?
				MUmVector_Add(localmove_pt, localmove_pt, ioCharacter->actual_position);
				error = AI2rTryLocalMovement(ioCharacter, PHcPathMode_CasualMovement, &ioCharacter->actual_position,
											&localmove_pt, 3, &localmove_ok, &localmove_stop,
											&localmove_weight, &localmove_escape);
				if (error == UUcError_None) {
					if (ioTechnique->technique->computed_flags & AI2cTechniqueUserFlag_Fearless) {
						// we ignore danger squares
						if ((!localmove_ok) && (localmove_weight == PHcDanger)) {
							localmove_ok = UUcTrue;
						}
					}
				} else {
					localmove_ok = UUcFalse;
				}

				if (!localmove_ok) {
					// we cannot move in this direction
#if DEBUG_VERBOSE_WEIGHTVAL
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - evade fails local movement (due to %s)",
						(error == UUcError_None) ? "wall" : "error");
#endif
					ioTechnique->weight = 0;
					return;
				}

				ioContext->target.animFrame = 0;
				ioContext->target.animation = move->animation;

#if DEBUG_SHOWINTERSECTIONS
				TRgStoreThisIntersection = MELEE_REACTINTERSECTIONS;
#endif
				if (TRrIntersectAnimations(ioContext)) {
					// we will get hit anyway if we do this evasion move.
					ioTechnique->weight *= AI2cWeight_EvasionStillHit;
#if DEBUG_VERBOSE_WEIGHTVAL
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - evade stillhit, mult %f -> %f",
						AI2cWeight_EvasionStillHit, ioTechnique->weight);
#endif
				}
			break;

			case AI2cMoveType_Throw:
				if (!inConsiderAttack) {
					// throw moves have no place in reaction techniques
#if DEBUG_VERBOSE_WEIGHTVAL
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - throw/consreact -> current 0");
#endif
					ioTechnique->weight = 0;
					return;
				}

				if ((move->animation == NULL) || (!TRrAnimation_TestFlag(move->animation, ONcAnimFlag_ThrowSource)) ||
					(move->target_animation == NULL) || (!TRrAnimation_TestFlag(move->target_animation, ONcAnimFlag_ThrowTarget))) {
					// this animation is broken!
					AI2_ERROR(AI2cBug, AI2cSubsystem_Melee, AI2cError_Melee_BrokenAnim, ioCharacter,
						ioMeleeState, ioTechnique->technique, move_index, ioMeleeProfile);
					if (AI2gMelee_ShowCurrentProfile) {
						strcat(AI2gMelee_TechniqueWeightDescription, " BROKEN");
					}
#if DEBUG_VERBOSE_WEIGHTVAL
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - broken -> current 0");
#endif
					ioTechnique->weight = 0;
					return;
				}
#if DEBUG_VERBOSE_WEIGHT
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "    move is a throw\n");
#endif

				/*
				 * double-check that we are in the right position with respect to the target
				 */
				
				animtype = TRrAnimation_GetType(move->animation);

				if (currently_behind_target) {
					if (ONrAnimType_ThrowIsFromFront(animtype)) {
						UUrDebuggerMessage("%s: melee profile %s: technique %s has throw of type %s but wasn't flagged as required to be from front\n",
											ioCharacter->player_name, ioMeleeProfile->name, ioTechnique->technique->name, ONrAnimTypeToString(animtype));
						ioTechnique->weight = 0;
						return;
					}
				} else {
					if (!ONrAnimType_ThrowIsFromFront(animtype)) {
						UUrDebuggerMessage("%s: melee profile %s: technique %s has throw of type %s but wasn't flagged as required to be from behind\n",
											ioCharacter->player_name, ioMeleeProfile->name, ioTechnique->technique->name, ONrAnimTypeToString(animtype));
						ioTechnique->weight = 0;
						return;
					}
				}

				/*
				 * can we run this throw from the target's current state?
				 */

				active_target = ONrForceActiveCharacter(ioMeleeState->target);
				if (active_target == NULL) {
					UUmAssert(0);
					ioTechnique->weight = 0;
					return;
				}
				target_state = active_target->nextAnimState;
				target_varient = active_target->animVarient;
				try_animation = (TRtAnimation *) TRrCollection_Lookup(ioCharacter->characterClass->animations, animtype,
																	target_state, target_varient | ONcAnimVarientMask_Fight);
				if (try_animation == NULL) {
					if (AI2gMelee_ShowCurrentProfile) {
						strcat(AI2gMelee_TechniqueWeightDescription, " CANTTHROW");
					}
#if DEBUG_VERBOSE_WEIGHTVAL
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - can't do throwtype %s from state %s varient 0x%04X -> current 0",
										ONrAnimTypeToString(TRrAnimation_GetType(move->target_animation)), ONrAnimStateToString(target_state), target_varient);
#endif
				}

				if (!AI2iMelee_TargetIsThrowable(ioMeleeState->target, active_target)) {
					// target is unthrowable
					if (AI2gMelee_ShowCurrentProfile) {
						strcat(AI2gMelee_TechniqueWeightDescription, " UNTHROWABLE");
					}
#if DEBUG_VERBOSE_WEIGHTVAL
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - target is unthrowable -> current 0");
#endif
				}

				/*
				 * determine whether to delay, and by how much
				 */

				if (can_delay) {
					// determine how far we would have to delay for, in order to get into range
					delay_distance = target_dist - AI2cMelee_ThrowDistSafety * TRrAnimation_GetThrowDistance(move->animation);

					if ((ioTechnique->positionmove_skip != (UUtUns16) -1) && (delay_distance < -AI2cMelee_SkipPositioningDistance)) {
#if DEBUG_VERBOSE_WEIGHTVAL
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - delay-distance %f < %f, skip positioning move (%d)",
												delay_distance, -AI2cMelee_SkipPositioningDistance, ioTechnique->positionmove_skip);
#endif
						// skip the positioning move
						predelay_distance = 0;
						postdelay_distance = 0;
					} else {
						if (ioTechnique->positionmove_skip != (UUtUns16) -1) {
							// do not skip this positioning move after all, we are too far away
#if DEBUG_VERBOSE_WEIGHTVAL
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - delay-distance %f > %f, DON'T skip positioning move (%d)",
													delay_distance, -AI2cMelee_SkipPositioningDistance, ioTechnique->positionmove_skip);
#endif
							ioTechnique->positionmove_skip = (UUtUns16) -1;
						}
						delay_distance -= predelay_distance + postdelay_distance;

						delay_distance = UUmMax(delay_distance, AI2cMelee_PositionIgnoreMinDistance);
						if (delay_distance < min_delay) {
							if (AI2gMelee_ShowCurrentProfile) {
								strcat(AI2gMelee_TechniqueWeightDescription, " <<DELAY");
							}
#if DEBUG_VERBOSE_WEIGHT
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "    delay %f < min %f\n", delay_distance, min_delay);
#endif
#if DEBUG_VERBOSE_WEIGHTVAL
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - <<delay (%f, %f) -> current 0", delay_distance, min_delay);
#endif
							ioTechnique->weight = 0;
							return;
						} else if (delay_distance < min_delay + delay_tolerance) {
							if (AI2gMelee_ShowCurrentProfile) {
								strcat(AI2gMelee_TechniqueWeightDescription, " <DELAY");
							}
#if DEBUG_VERBOSE_WEIGHTVAL
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - <delay (%f, %f [%f]), mult %f -> current %f",
								delay_distance, min_delay, delay_tolerance, (delay_distance - min_delay) / delay_tolerance,
								ioTechnique->weight);
#endif
							ioTechnique->weight *= (delay_distance - min_delay) / delay_tolerance;
						}

						// how does this affect our technique's weight?
						if (delay_distance > max_delay) {
							// outside acceptable bounds
							if (AI2gMelee_ShowCurrentProfile) {
								strcat(AI2gMelee_TechniqueWeightDescription, " >>DELAY");
							}
#if DEBUG_VERBOSE_WEIGHT
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "    delay %f > max %f\n", delay_distance, max_delay);
#endif
#if DEBUG_VERBOSE_WEIGHTVAL
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - >>delay (%f, %f) -> current 0", delay_distance, max_delay);
#endif
							ioTechnique->weight = 0;
							return;

						} else if (delay_distance > max_delay - delay_tolerance) {
							if (AI2gMelee_ShowCurrentProfile) {
								strcat(AI2gMelee_TechniqueWeightDescription, " >DELAY");
							}
							ioTechnique->weight *= (max_delay - delay_distance) / delay_tolerance;
#if DEBUG_VERBOSE_WEIGHTVAL
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - >delay (%f, %f [%f]), mult %f -> current %f",
								delay_distance, max_delay, delay_tolerance, (max_delay - delay_distance) / delay_tolerance,
								ioTechnique->weight);
#endif
						}

						// work out how long (in frames) we would have to delay for, in order to reach the target
						if (ignore_target_velocity) {
							delay_frames = delay_distance / delay_velocity;
						} else {
							delay_frames = delay_distance / (delay_velocity + target_closing_velocity);
						}
						if (delay_frames < 0) {
							if (AI2gMelee_ShowCurrentProfile) {
								strcat(AI2gMelee_TechniqueWeightDescription, " CANTCATCH");
							}
							ioTechnique->weight = 0;
#if DEBUG_VERBOSE_WEIGHT
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "    target is heading away from us at %f, can't catch up delay distance %f at vel %f\n",
										ignore_target_velocity ? 0 : target_closing_velocity, delay_distance, delay_velocity);
#endif
#if DEBUG_VERBOSE_WEIGHTVAL
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - nocatch -> current 0");
#endif
							return;
						} else if (delay_frames > AI2cMelee_MaxDelayFrames) {
							if (AI2gMelee_ShowCurrentProfile) {
								strcat(AI2gMelee_TechniqueWeightDescription, " >DELAYTIME");
							}
							ioTechnique->weight = 0;
#if DEBUG_VERBOSE_WEIGHT
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "    delay %f w/ ourvel %f closing %f requires time %f > max %d\n", delay_distance,
												delay_velocity, (ignore_target_velocity) ? 0 : target_closing_velocity, delay_frames, AI2cMelee_MaxDelayFrames);
#endif
#if DEBUG_VERBOSE_WEIGHTVAL
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - catchframes %d > %d -> current 0", delay_frames, AI2cMelee_MaxDelayFrames);
#endif
							return;
						}
					}
				} else {
					delay_distance = 0;
				}

				// check to make sure that we won't encounter danger in performing this move
#if DEBUG_VERBOSE_WEIGHTVAL
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - throw: check our movement");
#endif
				reject = AI2iMelee_WeightLocalMovement(ioCharacter, ioMeleeState, ioTechnique->technique, delay_distance + move->attack_endpos, &multiplier);
				ioTechnique->weight *= multiplier;
				if (reject) {
					return;
				}

				// check to see if the target would get thrown into danger by us
#if DEBUG_VERBOSE_WEIGHTVAL
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - throw: check target's movement");
#endif
				test_dist = target_dist + move->target_endpos;
				thrown_into_danger = AI2iMelee_WeightLocalMovement(ioCharacter, ioMeleeState, ioTechnique->technique, test_dist, NULL);
				if (thrown_into_danger) {
					if (AI2gMelee_ShowCurrentProfile) {
						strcat(AI2gMelee_TechniqueWeightDescription, " THROW->DANGER");
					}
					ioTechnique->weight *= ioMeleeProfile->weight_throwdanger;
#if DEBUG_VERBOSE_WEIGHTVAL
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - throw: target is thrown into danger, multiplier %f -> current %f",
										ioMeleeProfile->weight_throwdanger, ioTechnique->weight);
#endif
				}

				// we can't check anything useful past the first throw in a technique. exit with the current weight.
				return;
			break;
		}
	}
}

// consider and weight a spacing behavior
static void AI2iMelee_WeightSpacingBehavior(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState, AI2tMeleeProfile *ioMeleeProfile,
									  TRtDirection inDirection, AI2tMeleeMoveState inCurrentState, TRtAnimState inCurrentAnimState,
									  struct AI2tConsideredTechnique *ioTechnique)
{
	UUtUns32 itr, move_index, move_internalindex, move_type;
	float move_dist, move_tolerance, circle_weighting, delta_dist, delta_angle, ignore_tolerance, comfort_angle, multiplier;
	float sintheta, costheta;
	UUtError error;
	UUtBool localmove_ok, localmove_escape;
	UUtUns8 localmove_weight;
	M3tPoint3D localmove_pt, localmove_stop;
	ONtCharacter *attacker;
	AI2tMeleeMove *move;
	TRtPosition *position_pt;
	AI2tMeleeManeuverDefinition *maneuver_def;
	AI2tFightState *fight_state;
	AI2tMeleeState *other_meleestate;
	AI2tMeleeTechnique *technique = ioTechnique->technique;

	if (technique->computed_flags & AI2cTechniqueComputedMask_RequiresAttackCookie) {
		// attack moves cannot be part of a spacing behavior
#if DEBUG_VERBOSE_SPACINGWEIGHT
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - requires cookie -> current 0");
#endif
		ioTechnique->weight = 0;
		return;
	}

	if (!AI2gSpacingWeights) {
		// do not modify weights at all
		return;
	}

	// step through the technique modifying its weight according to each kind of move
	for (itr = 0; itr < technique->num_moves; itr++) {
		move_index = technique->start_index + itr;
		UUmAssert((move_index >= 0) && (move_index < ioMeleeProfile->num_moves));

		move = ioMeleeProfile->move + move_index;
		move_type = move->move & AI2cMoveType_Mask;

#if DEBUG_VERBOSE_SPACINGWEIGHT
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  + check move #%d (0x%08X) (%s)\n", itr + 1, move->move,
			(move->animation == NULL) ? "<null>" : TMrInstance_GetInstanceName(move->animation));
#endif
		switch(move_type) {
			case AI2cMoveType_Attack:
			case AI2cMoveType_Position:
			case AI2cMoveType_Throw:
				// these types of moves have no place in spacing behaviors
#if DEBUG_VERBOSE_SPACINGWEIGHT
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - attack/position -> current 0");
#endif
				ioTechnique->weight = 0;
				return;
			break;

			case AI2cMoveType_Evade:
				if (move->animation == NULL) {
#if DEBUG_VERBOSE_SPACINGWEIGHT
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - evasion is broken -> current 0");
#endif
					ioTechnique->weight = 0;
					return;
				}

				// work out where this evasion will take us
				costheta = MUrCos(ioCharacter->facing);				sintheta = MUrSin(ioCharacter->facing);
				position_pt = TRrAnimation_GetPositionPoint(move->animation, TRrAnimation_GetDuration(move->animation) - 1);
				localmove_pt.x = (costheta * position_pt->location_x + sintheta * position_pt->location_y) * TRcPositionGranularity;
				localmove_pt.z = (costheta * position_pt->location_y - sintheta * position_pt->location_x) * TRcPositionGranularity;
				localmove_pt.y = 0;
				MUmVector_Add(localmove_pt, localmove_pt, ioCharacter->actual_position);

				// is it safe to get there?
				error = AI2rTryLocalMovement(ioCharacter, PHcPathMode_CasualMovement, &ioCharacter->actual_position, &localmove_pt,
											3, &localmove_ok, &localmove_stop,
											&localmove_weight, &localmove_escape);
				if (error == UUcError_None) {
					if (ioTechnique->technique->computed_flags & AI2cTechniqueUserFlag_Fearless) {
						// we ignore danger squares
						if ((!localmove_ok) && (localmove_weight == PHcDanger)) {
							localmove_ok = UUcTrue;
						}
					}
				} else {
					localmove_ok = UUcFalse;
				}

				if (!localmove_ok) {
					// we cannot move in this direction
#if DEBUG_VERBOSE_WEIGHTVAL
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - evade fails local movement (due to %s)",
						(error == UUcError_None) ? "wall" : "error");
#endif
					ioTechnique->weight = 0;
					return;
				}
			break;

			case AI2cMoveType_Maneuver:
				move_internalindex = move->move & ~AI2cMoveType_Mask;
				UUmAssert((move_internalindex >= 0) && (move_internalindex < AI2cMeleeMaxPositionMoves));
				maneuver_def = &AI2gMeleeManeuverDefinition[move_internalindex];

				// consider the ranges of this maneuver move to see if it's appropriate here.
				switch (move_internalindex) {
					case AI2cMeleeManeuver_Advance:
						move_tolerance = UUmMax(AI2cMelee_Spacing_MinToleranceBand, move->param[2]);
						move_dist = ioMeleeState->distance_to_target - move->param[1];
						move_dist /= move_tolerance;

						if (move_dist < 0) {
							// we're already close enough
							if (AI2gMelee_ShowCurrentProfile) {
								strcat(AI2gMelee_TechniqueWeightDescription, " <DIST");
							}
#if DEBUG_VERBOSE_SPACINGWEIGHT
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - advance closeenough %f < %f -> 0",
								ioMeleeState->distance_to_target, move->param[1]);
#endif
							ioTechnique->weight = 0;
							return;
						} else {
							// we need to advance
							ioTechnique->weight *= move_dist;
#if DEBUG_VERBOSE_SPACINGWEIGHT
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - advance required %f > %f (delta %f), mult %f -> %f",
								ioMeleeState->distance_to_target, move->param[1], move_tolerance, move_dist, ioTechnique->weight);
#endif
						}
					break;

					case AI2cMeleeManeuver_Retreat:
						move_tolerance = UUmMax(AI2cMelee_Spacing_MinToleranceBand, move->param[2]);
						move_dist = move->param[1] - ioMeleeState->distance_to_target;
						move_dist /= move_tolerance;

						if (move_dist < 0) {
							// we're already far enough away
							if (AI2gMelee_ShowCurrentProfile) {
								strcat(AI2gMelee_TechniqueWeightDescription, " >DIST");
							}
#if DEBUG_VERBOSE_SPACINGWEIGHT
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - retreat farenough %f > %f -> 0",
								ioMeleeState->distance_to_target, move->param[1]);
#endif
							ioTechnique->weight = 0;
							return;
						} else {
							// we need to retreat
							ioTechnique->weight *= move_dist;
#if DEBUG_VERBOSE_SPACINGWEIGHT
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - retreat required %f < %f (delta %f), mult %f -> %f",
								ioMeleeState->distance_to_target, move->param[1], move_tolerance, move_dist, ioTechnique->weight);
#endif
						}
					break;

					case AI2cMeleeManeuver_CircleLeft:
					case AI2cMeleeManeuver_CircleRight:
						fight_state = ioMeleeState->fight_info.fight_owner;
						if (fight_state != NULL) {
							circle_weighting = 0;

							// loop over all other attackers in the fight state
							attacker = fight_state->attacker_head;
							while (attacker != NULL) {
								UUmAssert(attacker->charType == ONcChar_AI2);
								UUmAssert(attacker->ai2State.currentGoal == AI2cGoal_Combat);
								other_meleestate = &attacker->ai2State.currentState->state.combat.melee;

								if (attacker != ioCharacter) {
									delta_dist = other_meleestate->distance_to_target - ioMeleeState->distance_to_target;
									if ((delta_dist > AI2cMelee_Spacing_SameDistTolerance) && (other_meleestate->fight_info.has_attack_cookie)) {
										// ignore this character, they are behind us and don't have the attack cookie
									} else {
										comfort_angle = move->param[1] * M3cDegToRad;
										delta_angle = other_meleestate->angle_to_target - ioMeleeState->angle_to_target;
										UUmTrig_ClipAbsPi(delta_angle);

										if ((float)fabs(delta_angle) > comfort_angle) {
											// ignore this character, they are outside our comfort angle
										} else {
											if (move_internalindex == AI2cMeleeManeuver_CircleLeft) {
												delta_angle = -delta_angle;
											}

#if DEBUG_VERBOSE_SPACINGWEIGHT
											COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - circle %s consider attacker %s at (%f, %f) -> delta dist %f, angle %f",
												(move_internalindex == AI2cMeleeManeuver_CircleRight) ? "right" : "left", attacker->player_name, other_meleestate->distance_to_target,
												other_meleestate->angle_to_target, delta_dist, delta_angle);
#endif
											if ((float)fabs(delta_dist) < AI2cMelee_Spacing_SameDistTolerance) {
												// this character is at the same distance as us, always avoid them
												ignore_tolerance = 0;
											} else {
												// this character is at a different distance to us; don't avoid them if we are
												// almost collinear
												ignore_tolerance = AI2cMelee_Spacing_SameAngleTolerance;
											}

											if (delta_angle > ignore_tolerance) {
												// the character is ahead of us, we must weight down this maneuver because we would
												// run into or be occluded by them
												if (delta_angle < AI2cMelee_Spacing_MaxUrgencyAngle) {
													circle_weighting -= 1.0f;
												} else {
													circle_weighting -= 1.0f - (delta_angle - AI2cMelee_Spacing_MaxUrgencyAngle)
																		/ (comfort_angle - AI2cMelee_Spacing_MaxUrgencyAngle);
												}

											} else if (delta_angle < 0) {
												// the character is behind us, we weight up this maneuver so that we move away from them
												if (delta_angle > -AI2cMelee_Spacing_MaxUrgencyAngle) {
													circle_weighting += 1.0f;
												} else {
													circle_weighting += 1.0f - (-delta_angle - AI2cMelee_Spacing_MaxUrgencyAngle)
																			/ (comfort_angle - AI2cMelee_Spacing_MaxUrgencyAngle);
												}
											}
										}
									}
								}

								attacker = other_meleestate->fight_info.next;
							}

							multiplier = 1.0f + circle_weighting;
							if (multiplier <= 0) {
								if (AI2gMelee_ShowCurrentProfile) {
									strcat(AI2gMelee_TechniqueWeightDescription, " <<TOOCLOSE");
								}
#if DEBUG_VERBOSE_SPACINGWEIGHT
								COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - angular spacing overall: %f -> 0", circle_weighting);
#endif
								ioTechnique->weight = 0;
								return;
							} else {
								if (AI2gMelee_ShowCurrentProfile) {
									if (multiplier > 1.0f) {
										strcat(AI2gMelee_TechniqueWeightDescription, " >GETAWAY");
									} else {
										strcat(AI2gMelee_TechniqueWeightDescription, " <CLOSE");
									}
								}
								ioTechnique->weight *= multiplier;
#if DEBUG_VERBOSE_SPACINGWEIGHT
								COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - angular spacing overall: %f, mult %f -> %f",
									circle_weighting, multiplier, ioTechnique->weight);
#endif
							}
						}
					break;

					case AI2cMeleeManeuver_RandomStop:
						// immediately stop weighting the technique; return whatever weight we already have
						return;
				
					case AI2cMeleeManeuver_Pause:
					case AI2cMeleeManeuver_Crouch:
					case AI2cMeleeManeuver_Jump:
					case AI2cMeleeManeuver_Taunt:
					case AI2cMeleeManeuver_GetupFwd:
					case AI2cMeleeManeuver_GetupBack:
					case AI2cMeleeManeuver_GetupLeft:
					case AI2cMeleeManeuver_GetupRight:
						// this move is always equally appropriate
					break;

					case AI2cMeleeManeuver_BarabbasWave:
						// this type of maneuver has no place in spacing behaviors
#if DEBUG_VERBOSE_SPACINGWEIGHT
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  - barabbaswave -> current 0");
#endif
						ioTechnique->weight = 0;
						return;
					break;

					default:
						UUmAssert(0);
					break;
				}
			break;
		}
	}
}

// check the viability of an attacking technique
static UUtBool AI2iCheckTechniqueViability(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState, AI2tMeleeProfile *ioMeleeProfile,
									  UUtBool inIsAction, TRtAnimIntersectContext *ioContext, struct AI2tConsideredTechnique *ioTechnique)
{
	AI2tMeleeMove *move;

	if (!inIsAction) {
		// we are considering spacing behavior, everything is always viable for now
		return UUcTrue;
	}

	if (ioTechnique->ends_in_jump) {
		// jumping attacks are always viable as long as the range is right
		return UUcTrue;
	}

	move = ioTechnique->attack_move;
	if (move == NULL) {
		// non-attack moves always pass this test
		return UUcTrue;
	} else if ((move->move & AI2cMoveType_Mask) != AI2cMoveType_Attack) {
#if DEBUG_VERBOSE_MELEE
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: technique %s has 'attack move' %d that isn't an attack",
					ioCharacter->player_name, ioTechnique->technique->name, ioTechnique->attack_moveindex + 1);
#endif
		return UUcTrue;
	}

	// TEMPORARY DEBUGGING
/*	if (UUrString_Substring(ioTechnique->technique->name, "Dive", 100)) {
#if DEBUG_VERBOSE_MELEE
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: evaluating technique %s: rotate c=%f s=%f offset=%f dir %s",
					ioCharacter->player_name, ioTechnique->technique->name, ioTechnique->cos_rotate, ioTechnique->sin_rotate,
					ioTechnique->action_location_offset, ((ioTechnique->rotation_direction < 0) || (ioTechnique->rotation_direction >= TRcDirection_Max))
					? "<error>" : TRcDirectionName[ioTechnique->rotation_direction]);
#endif
	}*/

	// return to the original intersection context, and then apply the target matrix that we computed when
	// first weighting the technique
	ONrRevertAnimIntersectionContext(ioContext);
	AI2iMelee_RotateAnimationContext(ioContext, ioTechnique);
	AI2iMelee_SetupAttackerState(ioCharacter, ioMeleeProfile, &ioContext->attacker,
								ioTechnique->ends_in_jump, ioTechnique->jump_velocity,
								ioTechnique->rotation_direction, move->animation);

#if DEBUG_SHOWINTERSECTIONS
	TRgStoreThisIntersection = MELEE_ACTINTERSECTIONS;
#endif
	return TRrIntersectAnimations(ioContext);
}

// can we transition to a new state?
static AI2tMeleeTransition *AI2iMelee_CheckStateTransition(ONtCharacter *ioCharacter, AI2tMeleeProfile *ioMeleeProfile,
														AI2tMeleeMoveState inFromState, AI2tMeleeMoveState inDesiredState,
														TRtAnimState inTestAnimState)
{
	AI2tMeleeTransition *transition;
	const TRtAnimation *test_animation;
	const TRtAnimationCollection *anim_collection;
	TRtAnimVarient varient;
	ONtActiveCharacter *active_character;
	UUtUns32 itr, itr2;

	// these cases must be handled elsewhere
	UUmAssert(inDesiredState != inFromState);
	UUmAssertReadPtr(ioMeleeProfile, sizeof(AI2tMeleeProfile));

	if (ioCharacter == NULL) {
		anim_collection = ioMeleeProfile->char_class->animations;
		varient = ONcAnimVarientMask_None;
	} else {
		active_character = ONrGetActiveCharacter(ioCharacter);
		UUmAssertReadPtr(active_character, sizeof(*active_character));

		anim_collection = ioCharacter->characterClass->animations;
		varient = active_character->animVarient;
	}
	varient |= ONcAnimVarientMask_Fight;

#if DEBUG_VERBOSE_TRANSITION
	COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  * transition search %s -> %s\n", AI2cMeleeMoveStateName[inFromState], AI2cMeleeMoveStateName[inDesiredState]);
#endif

	// look for transitions that either go straight to the desired state, or have a continuation to there.
	for (itr = 0, transition = ioMeleeProfile->transitions; itr < ioMeleeProfile->num_transitions; itr++, transition++) {
		if (transition->from_state != (UUtUns8) inFromState)
			continue;

#if DEBUG_VERBOSE_TRANSITION
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  ... found transition to %s %s\n", AI2cMeleeMoveStateName[transition->to_state[0]],
			(transition->to_state[1] == AI2cMeleeMoveState_None) ? "" : AI2cMeleeMoveStateName[transition->to_state[1]]);
#endif

		for (itr2 = 0; itr2 < transition->num_to_states; itr2++) {
			if (transition->to_state[itr2] == (UUtUns8) inDesiredState) {
				break;
			}
		}

		if (itr2 >= transition->num_to_states) {
			// this transition doesn't lead to the desired state!
			continue;
		}

		// check to see if we could run the transition from where we are
		test_animation = TRrCollection_Lookup(anim_collection, transition->anim_type, inTestAnimState, varient);
		if (test_animation == NULL) {
#if DEBUG_VERBOSE_TRANSITION
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: abort state-transition check to %s because can't do animtype %s from animstate %s",
				(ioCharacter == NULL) ? ioMeleeProfile->name : ioCharacter->player_name,
				AI2cMeleeMoveStateName[transition->to_state[0]], ONrAnimTypeToString(transition->anim_type),
				ONrAnimStateToString(inTestAnimState));
#endif
			continue;
		}

		// we can run this transition!
		return transition;
	}

	// there are no transitions that take us to the desired state
	return NULL;
}

// try to make the character move in a local direction this frame
static UUtBool AI2iMelee_AddLocalMovement(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
										  TRtDirection inDirection, UUtBool inLookAside)
{
	UUtError error;
	UUtUns8 localmove_weight;
	UUtBool localmove_success, localmove_escape, found_dir;
	float sin_facing, cos_facing, theta;
	M3tVector3D localmove_dir, localmove_dest, localmove_pt, from_pt;
	AI2tMovementDirection move_direction;

	sin_facing = MUrSin(ioCharacter->facing);
	cos_facing = MUrCos(ioCharacter->facing);

	switch(inDirection) {
		case TRcDirection_Forwards:
			MUmVector_Set(localmove_dir,  sin_facing, 0,  cos_facing);
			theta = 0;
			move_direction = AI2cMovementDirection_Forward;
			break;

		case TRcDirection_Left:
			MUmVector_Set(localmove_dir,  cos_facing, 0, -sin_facing);
			theta = M3cHalfPi;
			move_direction = AI2cMovementDirection_Sidestep_Left;
			break;

		case TRcDirection_Back:
			MUmVector_Set(localmove_dir, -sin_facing, 0, -cos_facing);
			theta = M3cPi;
			move_direction = AI2cMovementDirection_Backpedal;
			break;

		case TRcDirection_Right:
			MUmVector_Set(localmove_dir, -cos_facing, 0,  sin_facing);
			theta = M3cPi + M3cHalfPi;
			move_direction = AI2cMovementDirection_Sidestep_Right;
			break;

		default:
			UUmAssert(0);
			return UUcFalse;
	}

	if (inLookAside) {
		theta += ioMeleeState->angle_to_target;
	} else {
		theta += ioCharacter->facing;
	}
	UUmTrig_ClipHigh(theta);

	found_dir = UUcFalse;
	if (inLookAside) {
		// find local movement for us somewhere in the general direction
		error = AI2rFindLocalPath(ioCharacter, PHcPathMode_CasualMovement, NULL, NULL, NULL, AI2cMelee_PositionLocalMoveDist, theta, M3cPi / 16.0f, 4, PHcDanger, 3,
									&localmove_success, &localmove_pt, &localmove_weight, &localmove_escape);
		found_dir = ((error == UUcError_None) && (localmove_success));
		if (found_dir) {
			// we successfully found a path
			ONrCharacter_GetPathfindingLocation(ioCharacter, &from_pt);
			theta = MUrATan2(localmove_pt.x - from_pt.x, localmove_pt.z - from_pt.z);
			UUmTrig_ClipLow(theta);
			found_dir = UUcTrue;
		}

	} else {
		// try local movement along the exact path specified
		ONrCharacter_GetPathfindingLocation(ioCharacter, &from_pt);
		MUmVector_Copy(localmove_dest, from_pt);
		MUmVector_ScaleIncrement(localmove_dest, AI2cMelee_PositionLocalMoveDist, localmove_dir);

		error = AI2rTryLocalMovement(ioCharacter, PHcPathMode_CasualMovement, &from_pt, &localmove_dest, 
									3, &localmove_success, &localmove_pt,
									&localmove_weight, &localmove_escape);
		found_dir = ((error == UUcError_None) && (localmove_success));
	}

	if (!found_dir) {
		if ((inDirection == TRcDirection_Forwards) && (!ioMeleeState->localpath_blocked)) {
			ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);

			if (active_character->movement_state.numSeriousCollisions > 0) {
				// we should not override the result of the local movement check
				return UUcFalse;
			} else {
				// we have been told that we can move towards the target anyway, do so
				theta = ioMeleeState->angle_to_target;
			}
		} else {
			// we cannot move in this direction
			return UUcFalse;
		}
	}

	// apply a movement modifier in this direction. NB! this won't work if there has been an executor-level
	// movement override applied this frame.
	AI2rPath_Halt(ioCharacter);
	AI2rMovement_LockFacing(ioCharacter, move_direction);
	AI2rMovement_MovementModifier(ioCharacter, theta, 1.0f);

	return UUcTrue;
}

// ------------------------------------------------------------------------------------
// -- pre-processing

// pre-process a melee profile and prepare it for use
UUtError AI2rMelee_PrepareForUse(AI2tMeleeProfile *ioMeleeProfile)
{
	ONtCharacterClass *new_class;
	UUtError error;
	AI2tMeleeMove *move;
	AI2tMeleeTechnique *technique;
	TRtAnimationCollection *anim_collection;
	TRtAnimType anim_type;
	const TRtExtentInfo *extent_info;
	TRtAnimState anim_state;
	AI2tMeleeMoveState current_state, new_state;
	TRtDirection technique_dir, move_dir;
	TRtPosition *end_position;
	AI2tMeleeTransitionDefinition *transition_def;
	AI2tMeleeTransition *transition;
	TRtAnimation *lookup_anim;
	M3tVector3D vel_estimate, thisframe_vel, attack_endvec, target_endvec;
	UUtUns16 frame_itr;
	UUtUns32 itr, itr2, move_itr, move_index, total_techniques, move_type, vel_estimate_points, num_alternatives, alternatives_found;
	UUtUns32 target_varient, num_delay_timers, overflowed_transitions, overflowed_timers;
	float throw_distance;
	UUtBool is_first_move, valid_move, overflowed;
	ONtAirConstants *air_constants;

	error = TMrInstance_GetDataPtr(TRcTemplate_CharacterClass, ioMeleeProfile->char_classname, &new_class);
	if (error != UUcError_None) {
		// this character class does not exist... find a default one
		AI2_ERROR(AI2cBug, AI2cSubsystem_Melee, AI2cError_Melee_NoCharacterClass, NULL,
				ioMeleeProfile->char_classname, ioMeleeProfile->id, ioMeleeProfile->name, 0);
		error = TMrInstance_GetDataPtr_ByNumber(TRcTemplate_CharacterClass, 0, &new_class);
		UUmError_ReturnOnErrorMsg(error, "AI2rMelee_PrepareForUse: cannot find any character classes!");
	}
	anim_collection = new_class->animations;
	ioMeleeProfile->char_class = new_class;

	/*
	 * get jump times and distances
	 */

	// work out how long we can stay in the air while jumping
	air_constants = &ioMeleeProfile->char_class->airConstants;

	ioMeleeProfile->min_jumpframes = 2 * air_constants->jumpAmount / air_constants->jumpGravity;
	ioMeleeProfile->max_jumpframes = ioMeleeProfile->min_jumpframes
		+ 2 * air_constants->jumpAccelAmount * air_constants->numJumpAccelFrames / air_constants->jumpGravity;

	// calculate how fast we are moving when we start each particular kind of running jump
	UUrMemory_Clear(ioMeleeProfile->jump_vel, AI2cMeleeJumpType_Max * sizeof(float));

	lookup_anim = (TRtAnimation *) TRrCollection_Lookup(anim_collection, ONcAnimType_Run,
											ONcAnimState_Running_Right_Down, ONcAnimVarientMask_Fight);
	if (lookup_anim != NULL) {
		TRrAnimation_GetPosition(lookup_anim, 0, &thisframe_vel);
		ioMeleeProfile->jump_vel[AI2cMeleeJumpType_RunForward] = thisframe_vel.z; 
	}

	lookup_anim = (TRtAnimation *) TRrCollection_Lookup(anim_collection, ONcAnimType_Run_Backwards,
											ONcAnimState_Running_Back_Right_Down, ONcAnimVarientMask_Fight);
	if (lookup_anim != NULL) {
		TRrAnimation_GetPosition(lookup_anim, 0, &thisframe_vel);
		ioMeleeProfile->jump_vel[AI2cMeleeJumpType_RunBack] = -thisframe_vel.z; 
	}

	lookup_anim = (TRtAnimation *) TRrCollection_Lookup(anim_collection, ONcAnimType_Run_Sidestep_Left,
											ONcAnimState_Sidestep_Left_Right_Down, ONcAnimVarientMask_Fight);
	if (lookup_anim != NULL) {
		TRrAnimation_GetPosition(lookup_anim, 0, &thisframe_vel);
		ioMeleeProfile->jump_vel[AI2cMeleeJumpType_RunLeft] = thisframe_vel.x; 
	}

	lookup_anim = (TRtAnimation *) TRrCollection_Lookup(anim_collection, ONcAnimType_Run_Sidestep_Right,
											ONcAnimState_Sidestep_Right_Right_Down, ONcAnimVarientMask_Fight);
	if (lookup_anim != NULL) {
		TRrAnimation_GetPosition(lookup_anim, 0, &thisframe_vel);
		ioMeleeProfile->jump_vel[AI2cMeleeJumpType_RunRight] = -thisframe_vel.x; 
	}

	// we must be travelling at least half as fast sideways as we are up - if this constant from Oni_GameState.c
	// changes, change it here too!
	for (itr = AI2cMeleeJumpType_Standing; itr < AI2cMeleeJumpType_Max; itr++) {
		ioMeleeProfile->jump_vel[itr] = UUmMax(ioMeleeProfile->jump_vel[itr], 0.5f * air_constants->jumpAmount);
	}

	/*
	 * fill in the profile's state transitions
	 */

	ioMeleeProfile->num_transitions = 0;
	transition = ioMeleeProfile->transitions;
	overflowed_transitions = 0;
	overflowed = UUcFalse;

	num_alternatives = alternatives_found = 0;
	for (itr = 0, transition_def = AI2gMeleeTransitionTable; ; itr++, transition_def++) {
		if (transition_def->from_state == AI2cMeleeMoveState_None) {
			// this is a sentinel in the table, telling us that the list of alternatives for a transition is over
			if (num_alternatives == 0) {
				// two sentinels in a row, we have reached the end of the table
				break;
			} else if ((alternatives_found == 0) && ((transition_def - num_alternatives)->essential)) {
				// no alternatives were valid for this transition, this could be a problem
				AI2_ERROR(AI2cError, AI2cSubsystem_Melee, AI2cError_Melee_TransitionUnavailable, NULL,
							ioMeleeProfile, num_alternatives, transition_def - num_alternatives, 0);
			}

			num_alternatives = alternatives_found = 0;
			continue;
		}

		num_alternatives++;

		// check to see if we can actually do this transition... i.e. that we have an animation for this move
		anim_state = AI2cMelee_TotoroAnimState[transition_def->from_state];
		lookup_anim = (TRtAnimation *) TRrCollection_Lookup(anim_collection, transition_def->anim_type,
															anim_state, ONcAnimVarientMask_Fight);
		if (!lookup_anim) {
			// we do not have the necessary animation and cannot use this transition.
			continue;
		}
		alternatives_found++;

		if (overflowed) {
			// this transition may have been found, but we can't store it
			overflowed_transitions++;
		} else {
			// set up the transition
			transition->direction		= (UUtUns8) transition_def->direction;
			transition->from_state		= (UUtUns8) transition_def->from_state;
			transition->num_to_states	= (UUtUns8) transition_def->num_to_states;
			transition->weight_modifier = transition_def->weight;
			transition->move_keys		= transition_def->move_keys;
			transition->anim_type		= transition_def->anim_type;
			for (itr2 = 0; itr2 < transition->num_to_states; itr2++)
				transition->to_state[itr2] = (UUtUns8) transition_def->to_state[itr2];

			// get the parameters of the actual animation that this transition entails
			transition->animation		= lookup_anim;
			transition->anim_frames		= (UUtUns8) TRrAnimation_GetDuration(lookup_anim);
			transition->anim_tostate	= TRrAnimation_GetTo(lookup_anim);

			// get the end point of this transition's animation, and a velocity estimate at this point
			end_position = TRrAnimation_GetPositionPoint(lookup_anim, transition->anim_frames - 1);
			vel_estimate_points = 0;
			MUmVector_Set(vel_estimate, 0, 0, 0);
			for (frame_itr = UUmMax(0, transition->anim_frames - 5); frame_itr < transition->anim_frames; frame_itr++) {
				TRrAnimation_GetPosition(lookup_anim, frame_itr, &thisframe_vel);
				MUmVector_Add(vel_estimate, vel_estimate, thisframe_vel);
				vel_estimate_points++;
			}
			if (vel_estimate_points > 0) {
				MUmVector_Scale(vel_estimate, 1.0f / vel_estimate_points);
			}

			// convert this to a position and velocity in the direction of motion. note that position and velocity
			// are returned by the totoro functions in XZ space with Y as the vertical.
			switch(transition->direction) {
				case TRcDirection_Forwards:
					transition->anim_distance		=  end_position->location_y * TRcPositionGranularity;
					transition->anim_endvelocity	=  vel_estimate.z;
				break;

				case TRcDirection_Left:
					transition->anim_distance		=  end_position->location_x * TRcPositionGranularity;
					transition->anim_endvelocity	=  vel_estimate.x;
				break;

				case TRcDirection_Right:
					transition->anim_distance		= -end_position->location_x * TRcPositionGranularity;
					transition->anim_endvelocity	= -vel_estimate.x;
				break;

				case TRcDirection_Back:
					transition->anim_distance		= -end_position->location_y * TRcPositionGranularity;
					transition->anim_endvelocity	= -vel_estimate.z;
				break;
				
				default:
					UUmAssert(0);
					transition->anim_distance		= 0;
					transition->anim_endvelocity	= 0;
				break;
			}

			transition++;
			ioMeleeProfile->num_transitions++;
		}
		
		if (ioMeleeProfile->num_transitions >= AI2cMeleeProfile_MaxTransitions) {
			overflowed = UUcTrue;
		}
	}

	if (overflowed_transitions > 0) {
		UUmAssert(overflowed);
		AI2_ERROR(AI2cBug, AI2cSubsystem_Melee, AI2cError_Melee_MaxTransitions, NULL,
					ioMeleeProfile, AI2cMeleeProfile_MaxTransitions + overflowed_transitions, AI2cMeleeProfile_MaxTransitions, 0);
	}

	/*
	 * check all techniques in the profile
	 */

	num_delay_timers = 0;
	overflowed_timers = 0;
	total_techniques = ioMeleeProfile->num_actions + ioMeleeProfile->num_reactions + ioMeleeProfile->num_spacing;
	for (itr = 0, technique = ioMeleeProfile->technique; itr < total_techniques; itr++, technique++) {
		technique->computed_flags = 0;
		technique->max_damage = 0;
		technique->attack_idealrange = 0;
		technique->attack_maxrange = 0;
		technique->attack_initialframes = 0;
		technique->attackmove_itr = (UUtUns32) -1;
		technique->attack_anim = NULL;
		technique->throw_distance = 0;
		technique->index_in_profile = (UUtUns16) itr;
		technique->delaytimer_index = (UUtUns16) -1;
		if (technique->index_in_profile < ioMeleeProfile->num_actions) {
			// this is an action technique
			technique->computed_flags |= AI2cTechniqueComputedFlag_IsAction;

		} else if (technique->index_in_profile < ioMeleeProfile->num_actions + ioMeleeProfile->num_reactions) {
			// this is a reaction technique
			technique->computed_flags |= AI2cTechniqueComputedFlag_IsReaction;
			technique->index_in_profile -= (UUtUns16) ioMeleeProfile->num_actions;

		} else {
			// this is a spacing technique
			technique->computed_flags |= AI2cTechniqueComputedFlag_IsSpacing;
			technique->index_in_profile -= (UUtUns16) (ioMeleeProfile->num_actions + ioMeleeProfile->num_reactions);
		}

		current_state = AI2cMeleeMoveState_None;
		move_index = technique->start_index;
		move = ioMeleeProfile->move + move_index;

		is_first_move = UUcTrue;

		if (technique->num_moves == 0) {
			// empty technique cannot be executed
			technique->computed_flags |= AI2cTechniqueComputedFlag_Broken;
			continue;
		}

		if (technique->delay_frames > 0) {
			// this technique is governed by a delay
			if (num_delay_timers >= AI2cMeleeMaxDelayTimers) {
				overflowed_timers++;
			} else {
				technique->delaytimer_index = (UUtUns16) num_delay_timers++;
			}
		}

		for (move_itr = 0; move_itr < technique->num_moves; move_itr++, move_index++, move++) {
			if (!is_first_move) {
				/*
				 * check that this move can validly follow the previous one
				 */

				move_type = (move->move & AI2cMoveType_Mask) >> AI2cMoveType_Shift;
				UUmAssert((move_type >= 0) && (move_type < AI2cMeleeNumMoveTypes));
				if (!AI2cMeleeMoveTypes[move_type].func_canfollow(move->move, move[-1].move)) {
					// it is not possible for us to run this animation at this point in the technique!
					AI2_ERROR(AI2cError, AI2cSubsystem_Melee, AI2cError_Melee_PreComputeBroken, NULL,
							ioMeleeProfile, new_class, anim_type, move);

					// the technique is broken. bail.
					move->is_broken = UUcTrue;
					technique->computed_flags |= AI2cTechniqueComputedFlag_Broken;
					break;
				}
			}

			/*
			 * determine the animation for this move
			 */

			// test to see whether this move has an animation from here. this checks current_state
			// and may set it if it's the first move
			move->animation = NULL;
			move->target_animation = NULL;
			valid_move = AI2rMelee_MoveIsValid(anim_collection, move->move, &anim_type, &move_dir,
							&current_state, &new_state, &move->animation, &move->target_animation, &throw_distance);

			move->current_state = current_state;

			if (!valid_move) {
				// it is not possible for us to run this animation at this point in the technique!
				AI2_ERROR(AI2cError, AI2cSubsystem_Melee, AI2cError_Melee_PreComputeBroken, NULL,
						ioMeleeProfile, new_class, anim_type, move);

				// the technique is broken. bail.
				move->is_broken = UUcTrue;
				technique->computed_flags |= AI2cTechniqueComputedFlag_Broken;
				break;
			} else {
				move->is_broken = UUcFalse;
			}

			if (is_first_move) {
				if (current_state == AI2cMeleeMoveState_None) {
					// this technique can possibly start from multiple states.
					technique->computed_flags |= AI2cTechniqueComputedFlag_AllowTransition;

					// the state that must be transitioned to is that at the end of the positioning move
					technique->computed_flags |= new_state;

					move_type = move->move & AI2cMoveType_Mask;
					UUmAssert((move_type >= 0) && (move_type < (AI2cMeleeNumMoveTypes << AI2cMoveType_Shift)));

					if (move_type == AI2cMoveType_Maneuver) {
						// this technique can start from multiple states.
						continue;

					} else if (move_type != AI2cMoveType_Position) {
						// only positioning and maneuver states should have their current-state as None.
						move->is_broken = UUcTrue;
						technique->computed_flags |= AI2cTechniqueComputedFlag_Broken;
						break;
					}

				} else {
					// this technique can only execute if the character is in the correct state
					technique->computed_flags |= current_state;
				}
			}

			/*
			 * get the direction of this move and set up the overall direction of the technique
			 */

			technique_dir = (technique->computed_flags & AI2cTechniqueComputedMask_MoveDirection)
				>> AI2cTechniqueComputedShift_MoveDirection;

			if (move_dir == TRcDirection_None) {
				// don't change the technique's direction

			} else if (move_dir == TRcDirection_360) {
				if (technique_dir == TRcDirection_None) {
					// only overwrite the technique's direction if it's 'none'
					technique->computed_flags |= (move_dir << AI2cTechniqueComputedShift_MoveDirection);
				}

			} else {
				if (technique_dir == move_dir) {
					// this is the same direction

				} else if ((technique_dir == TRcDirection_None) || (technique_dir == TRcDirection_360)) {
					// overwrite the technique's direction
					technique->computed_flags |= (move_dir << AI2cTechniqueComputedShift_MoveDirection);

				} else {
					// the technique is already directed towards an actual direction that isn't the same as
					// the current move's
					AI2_ERROR(AI2cError, AI2cSubsystem_Melee, AI2cError_Melee_MultiDirectionTechnique, NULL,
						ioMeleeProfile, technique->name, technique_dir, move_dir);
				}
			}

			/*
			 * get the move's distance(s) travelled along the line of the technique
			 */

			MUmVector_Set(attack_endvec, 0, 0, 0);
			MUmVector_Set(target_endvec, 0, 0, 0);
			if (move->animation != NULL) {
				end_position = TRrAnimation_GetPositionPoint(move->animation, TRrAnimation_GetDuration(move->animation) - 1);
				attack_endvec.x = end_position->location_x * TRcPositionGranularity;
				attack_endvec.y = end_position->location_y * TRcPositionGranularity;
			}
			if (move->target_animation != NULL) {
				end_position = TRrAnimation_GetPositionPoint(move->target_animation, TRrAnimation_GetDuration(move->target_animation) - 1);
				target_endvec.x = end_position->location_x * TRcPositionGranularity;
				target_endvec.y = end_position->location_y * TRcPositionGranularity;
			}

			switch(move_dir) {
				case TRcDirection_None:
				case TRcDirection_360:
				case TRcDirection_Forwards:
					move->attack_endpos = attack_endvec.y;
					move->target_endpos = target_endvec.y;
					break;

				case TRcDirection_Left:
					move->attack_endpos = attack_endvec.x;
					move->target_endpos = target_endvec.x;
					break;

				case TRcDirection_Right:
					move->attack_endpos = -attack_endvec.x;
					move->target_endpos = -target_endvec.x;
					break;

				case TRcDirection_Back:
					move->attack_endpos = -attack_endvec.y;
					move->target_endpos = -target_endvec.y;
					break;

				default:
					UUmAssert(0);
					break;
			}

			/*
			 * set the technique's attack flags
			 */

			if (move->animation != NULL) {
				if (TRrAnimation_TestFlag(move->animation, ONcAnimFlag_ThrowSource)) {
					// set up throw flags
					if (technique->computed_flags & AI2cTechniqueComputedFlag_IsThrow) {
						// multiple throws in a technique are not allowed!
						move->is_broken = UUcTrue;
						technique->computed_flags |= AI2cTechniqueComputedFlag_Broken;
						break;
					}
					technique->computed_flags |= AI2cTechniqueComputedFlag_IsThrow;
					technique->throw_distance = throw_distance;

					if (ONrAnimType_ThrowIsFromFront(anim_type)) {
						technique->computed_flags |= AI2cTechniqueComputedFlag_FromFront;
					} else {
						technique->computed_flags |= AI2cTechniqueComputedFlag_FromBehind;
					}

					if (move->target_animation != NULL) {
						// handle throw-target stuff
						target_varient = TRrAnimation_GetVarient(move->target_animation);
						if (target_varient & ONcAnimVarientMask_Any_Rifle) {
							technique->computed_flags |= AI2cTechniqueComputedFlag_IsPistolDisarm;
						} else if (target_varient & ONcAnimVarientMask_Any_Pistol) {
							technique->computed_flags |= AI2cTechniqueComputedFlag_IsRifleDisarm;
						}

						technique->max_damage += TRrAnimation_GetMaximumSelfDamage(move->target_animation);
					}
				} else if (TRrAnimation_IsAttack(move->animation)) {
					// set up attack information
					extent_info = TRrAnimation_GetExtentInfo(move->animation);

					technique->computed_flags |= AI2cTechniqueComputedFlag_IsAttack;
					
					if (TRrAnimation_TestAttackFlag(move->animation, (TRtAnimTime) -1, (UUtUns32) -1, ONcAttackFlag_AttackHigh))
						technique->computed_flags |= AI2cTechniqueComputedFlag_IsHigh;

					if (TRrAnimation_TestAttackFlag(move->animation, (TRtAnimTime) -1, (UUtUns32) -1, ONcAttackFlag_AttackLow))
						technique->computed_flags |= AI2cTechniqueComputedFlag_IsLow;

					if (TRrAnimation_TestAttackFlag(move->animation, (TRtAnimTime) -1, (UUtUns32) -1, ONcAttackFlag_Unblockable))
						technique->computed_flags |= AI2cTechniqueComputedFlag_Unblockable;

					if (TRrAnimation_HasBlockStun(move->animation, AI2cMelee_BlockStunThreshold))
						technique->computed_flags |= AI2cTechniqueComputedFlag_HasBlockStun;

					if (TRrAnimation_HasStagger(move->animation))
						technique->computed_flags |= AI2cTechniqueComputedFlag_HasStagger;

					technique->max_damage += TRrAnimation_GetMaximumDamage(move->animation);
					
					technique->attack_maxrange = extent_info->attack_ring.max_distance;
					technique->attack_idealrange = technique->attack_maxrange - AI2cMelee_IdealRangeSubtractFraction * extent_info->maxAttack.attack_dist;
					technique->attack_initialframes = extent_info->firstAttack.frame_index;
					if (((technique->computed_flags & AI2cTechniqueComputedFlag_IsThrow) == 0) && (technique->attack_anim == NULL)) {
						// this is the first attackanim or throw in our technique, we use this to trigger how long
						// we delay for
						technique->attackmove_itr = move_itr;
						technique->attack_anim = move->animation;
					}
				}
			}

			// handle special-case moves
			switch(move->move & AI2cMoveType_Mask) {
				case AI2cMoveType_Maneuver:
					switch(move->move & ~AI2cMoveType_Mask) {
						case AI2cMeleeManeuver_BarabbasWave:
							// this is a special-case move and so the technique doesn't
							// need an attack or throw in order to be executed
							technique->computed_flags |= AI2cTechniqueComputedFlag_SpecialCaseMove;
						break;
					}
				break;
			}

			/*
			FIXME: still to be implemented:

  // later work
	AI2cTechniqueComputedFlag_RequireNoRifle		= (1 << 16),
	AI2cTechniqueComputedFlag_RequireRifle			= (1 << 17),

			*/
			current_state = new_state;
			is_first_move = UUcFalse;
		}
	}

	if (overflowed_timers > 0) {
		AI2_ERROR(AI2cBug, AI2cSubsystem_Melee, AI2cError_Melee_MaxDelayTimers, NULL,
					ioMeleeProfile, AI2cMeleeMaxDelayTimers + overflowed_timers, AI2cMeleeMaxDelayTimers, 0);
	}

	return UUcError_None;
}

// find a throw in a collection's animations that matches what we desire
static UUtBool AI2iMelee_FindThrow(TRtAnimationCollection *inCollection, UUtUns16 inAnimType, UUtUns16 inAnimState,
								UUtUns16 inTargetAnimState, TRtAnimVarient inTargetAnimVarient,
								TRtAnimation **outAnimation, TRtAnimation **outTargetAnimation,
								float *outThrowDistance)
{
	TRtAnimVarient found_varient;
	UUtUns32 itr, num_animations;
	TRtAnimationCollection *current_collection;
	TRtAnimationCollectionPart *collection_entry;
	TRtAnimation *animation, *target_animation;

	for (current_collection = inCollection; current_collection != NULL;
		current_collection = (TRtAnimationCollection *) TRrCollection_GetRecursive(current_collection)) {
		// get all potential animations with this animation type
		TRrCollection_Lookup_Range(current_collection, inAnimType, inAnimState, &collection_entry, (UUtInt32*)&num_animations);

		for (itr = 0; itr < num_animations; collection_entry++, itr++) {
			animation = collection_entry->animation;
			UUmAssertReadPtr(animation->throwInfo, sizeof(*animation->throwInfo));

			target_animation = (TRtAnimation *) TRrCollection_Lookup((const TRtAnimationCollection *) inCollection, animation->throwInfo->throwType,
																			inTargetAnimState, inTargetAnimVarient | ONcAnimVarientMask_Fight);
			if (target_animation == NULL) {
				continue;
			}

			// unlike a normal lookup, we want to reject animations that don't have the varient flag set that we desire
			found_varient = TRrAnimation_GetVarient(target_animation);
			if (((found_varient & ONcAnimVarientMask_Any_Rifle) == 0) && (inTargetAnimVarient & ONcAnimVarientMask_Any_Rifle)) {
				continue;

			} else if (((found_varient & ONcAnimVarientMask_Any_Pistol) == 0) && (inTargetAnimVarient & ONcAnimVarientMask_Any_Pistol)) {
				continue;
			}

			if (outAnimation)		*outAnimation = animation;
			if (outTargetAnimation) *outTargetAnimation = target_animation;
			if (outThrowDistance)	*outThrowDistance = TRrAnimation_GetThrowDistance(animation);

			return UUcTrue;
		}
	}

	return UUcFalse;
}

// check whether a particular move would actually be valid by doing an animation lookup on it
UUtBool AI2rMelee_MoveIsValid(TRtAnimationCollection *inCollection, UUtUns32 inMove,
							   UUtUns16 *outAnimType, TRtDirection *outMoveDir,
							   AI2tMeleeMoveState *ioFromState, AI2tMeleeMoveState *outToState,
							   TRtAnimation **outAnimation, TRtAnimation **outTargetAnimation,
							   float *outThrowDistance)
{
	UUtUns32 move_type;
	TRtAnimVarient target_animvarient;
	UUtUns16 anim_type, anim_state, target_animstate;
	TRtDirection move_dir;
	UUtBool found_throw;
	float throw_dist;
	AI2tMeleeMoveState from_state, to_state;
	TRtAnimation *lookup_anim, *target_anim;

	UUmAssertReadPtr(ioFromState, sizeof(AI2tMeleeMoveState));

	// get the parameters of this move
	move_type = inMove & AI2cMoveType_Mask;
	UUmAssert((move_type >= 0) && (move_type < AI2cMoveType_Max));

	if (move_type == AI2cMoveType_Position) {
		// positioning moves are handled separately!
		UUtUns32 move_index;
		AI2tMelee_PositionMove *position_move;

		move_index = (inMove & ~AI2cMoveType_Mask);
		UUmAssert((move_index >= 0) && (move_index < AI2cMeleeMaxPositionMoves));
		position_move = &AI2gMeleePositionTable[move_index];

		// NB: we do not set ioFromState here, which signals to the calling code
		// that this is a positioning move and that the AllowTransition flag
		// should be set.

		// set up output parameters
		if (outMoveDir)		*outMoveDir = position_move->direction;
		if (outToState)		*outToState = position_move->states[position_move->num_states - 1];

		if (position_move->can_delay) {
			// work out what the animation is that we play while delaying
			UUmAssert((position_move->states[0] > AI2cMeleeMoveState_None) &&
					  (position_move->states[0] < AI2cMeleeMoveState_Max));

			anim_state = AI2cMelee_TotoroAnimState[position_move->states[0]];
			anim_type = AI2cMelee_DelayAnimType[position_move->states[0]];

			lookup_anim = (TRtAnimation *) TRrCollection_Lookup(inCollection, anim_type, anim_state, ONcAnimVarientMask_Fight);

			if (outAnimType)	*outAnimType = anim_type;
			if (outAnimation)	*outAnimation = lookup_anim;
		} else {
			// set our animation to NULL, it won't be used
			if (outAnimType)	*outAnimType = ONcAnimType_None;
			if (outAnimation)	*outAnimation = NULL;
		}
		if (outThrowDistance)	*outThrowDistance = 0;

		return UUcTrue;
	}
		

	from_state = *ioFromState;
	anim_type = AI2cMeleeMoveTypes[move_type >> AI2cMoveType_Shift].func_getanimtype(inMove,
																		&move_dir, &from_state, &to_state,
																		&target_animstate, &target_animvarient);

	if (anim_type == ONcAnimType_None) {
		// this is not a valid move
		return UUcFalse;
	}

	// the move can only execute if it matches the from-state
	if ((from_state == AI2cMeleeMoveState_None) || (from_state == *ioFromState)){
		// no change to the from-state

	} else if (*ioFromState == AI2cMeleeMoveState_None) {
		// set the from-state (as we are the first move)
		*ioFromState = from_state;

	} else {
		return UUcFalse;
	}

	if ((*ioFromState > AI2cMeleeMoveState_None) && (*ioFromState < AI2cMeleeMoveState_Max)) {
		// convert from melee state to a sample totoro state for sampling
		anim_state = AI2cMelee_TotoroAnimState[from_state];
	} else {
		// any from-state will do (this probably won't ever happen)
		anim_state = ONcAnimState_Anything;
	}
	
	if (move_type != AI2cMoveType_Throw) {
		// look up the animation in our collection
		lookup_anim = (TRtAnimation *) TRrCollection_Lookup(inCollection, anim_type, anim_state, ONcAnimVarientMask_Fight);
		if ((!lookup_anim) && ((move_type == AI2cMoveType_Evade) || (move_type == AI2cMoveType_Attack))) {
			// we can't execute attacks or evasion moves if we don't have the animation
			return UUcFalse;
		}

		target_anim = NULL;
		throw_dist = 0;
	} else {
		// throws must be looked up very differently
		found_throw = AI2iMelee_FindThrow(inCollection, anim_type, anim_state, target_animstate, target_animvarient,
											&lookup_anim, &target_anim, &throw_dist);
		if (!found_throw) {
			return UUcFalse;
		}
	}

	// set up the return codes
	if (outAnimType)		*outAnimType = anim_type;
	if (outMoveDir)			*outMoveDir = move_dir;
	if (outToState)			*outToState = to_state;
	if (outAnimation)		*outAnimation = lookup_anim;
	if (outTargetAnimation)	*outTargetAnimation = target_anim;
	if (outThrowDistance)	*outThrowDistance = throw_dist;

	return UUcTrue;
}

// ------------------------------------------------------------------------------------
// -- attack functions

static UUtUns32 AI2iMelee_Attack_Iterate(UUtUns32 inIterator)
{
	UUtUns32 move_index;

	if (inIterator == (UUtUns32) -1) {
		move_index = 0;
	} else {
		UUmAssert((inIterator & AI2cMoveType_Mask) == AI2cMoveType_Attack);

		move_index = (inIterator & ~AI2cMoveType_Mask);
		UUmAssert((move_index >= 0) && (move_index < AI2gNumMeleeAttacks));

		move_index++;
	}

	if (move_index >= AI2gNumMeleeAttacks) {
		// no more moves in table
		return (UUtUns32) -1;
	} else {
		// this move is okay!
		return (AI2cMoveType_Attack | move_index);
	}
}

static UUtBool AI2iMelee_Attack_GetName(UUtUns32 inMove, char *outString, UUtUns32 inStringLength)
{
	UUtUns32 move_index;

	if ((inMove & AI2cMoveType_Mask) != AI2cMoveType_Attack) {
		// not an attack move!
		return UUcFalse;
	}

	move_index = (inMove & ~AI2cMoveType_Mask);
	if ((move_index >= 0) && (move_index < AI2gNumMeleeAttacks)) {
		// store the name string
		UUrString_Copy(outString, AI2gMeleeAttackTable[move_index].name, inStringLength);
		return UUcTrue;
	} else {
		// invalid move index
		return UUcFalse;
	}
}

static UUtUns32 AI2iMelee_Attack_GetParams(UUtUns32 inMove, UUtBool inSpacing, char *outStringPtrs[3])
{
	// melee attacks have no parameters
	return 0;
}

static UUtUns16 AI2iMelee_Attack_GetAnimType(UUtUns32 inMove, TRtDirection *outDirection,
											AI2tMeleeMoveState *outFromState, AI2tMeleeMoveState *outToState,
											UUtUns16 *outTargetAnimState, UUtUns16 *outTargetAnimVarient)
{
	UUtUns32 move_index, flags;

	UUmAssert((inMove & AI2cMoveType_Mask) == AI2cMoveType_Attack);

	move_index = (inMove & ~AI2cMoveType_Mask);
	UUmAssert((move_index >= 0) && (move_index < AI2gNumMeleeAttacks));

	// set required data for move sequencing and animation lookup
	flags = AI2gMeleeAttackTable[move_index].attackmove_flags;

	*outFromState	= AI2mMelee_AttackFromState(flags);
	*outToState		= AI2mMelee_AttackToState(flags);
	*outDirection	= AI2mMelee_AttackDirection(flags);
	*outTargetAnimState = ONcAnimState_None;
	*outTargetAnimVarient = 0;

	return AI2gMeleeAttackTable[move_index].anim_type;
}

static UUtBool AI2iMelee_Attack_CanFollow(UUtUns32 inMove, UUtUns32 inPrevMove)
{
	UUtUns32 move_index, prevmove_index;
	UUtUns16 required_type;

	UUmAssert((inMove & AI2cMoveType_Mask) == AI2cMoveType_Attack);
	move_index = (inMove & ~AI2cMoveType_Mask);
	UUmAssert((move_index >= 0) && (move_index < AI2gNumMeleeAttacks));

	required_type = AI2gMeleeAttackTable[move_index].combo_prev_animtype;
	if (required_type == ONcAnimType_None) {
		// this attack has no combo requirements
		return UUcTrue;
	}

	// this attack must follow another attack
	if ((inPrevMove == (UUtUns32) -1) || ((inPrevMove & AI2cMoveType_Mask) != AI2cMoveType_Attack))
		return UUcFalse;

	// make sure that the previous attack was of the right animation type
	prevmove_index = inPrevMove & ~AI2cMoveType_Mask;
	UUmAssert((prevmove_index >= 0) && (prevmove_index < AI2gNumMeleeAttacks));

	return (AI2gMeleeAttackTable[prevmove_index].anim_type == required_type);
}

static UUtBool AI2iMelee_Attack_Start(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove)
{
	ioMeleeState->attack_launched = UUcFalse;

	return AI2iMelee_Attack_Update(ioCharacter, ioMeleeState, inMove);
}


static UUtBool AI2iMelee_Attack_Update(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove)
{
	const TRtAnimation *test_animation;
	TRtAnimIntersectContext intersect_context;
	TRtAnimState next_animstate;
	UUtBool can_attack = UUcFalse, should_abort = UUcFalse, delay = UUcFalse, found_next_move;
	UUtUns32 tolerance_frames, frames_until_nextanim, next_move;
	AI2tMelee_AttackMove *attack_move;
	UUtUns32 move_index;
	UUtUns16 attack_type;
	AI2tMeleeMoveState cur_state, next_state;
	float theta, sintheta, costheta, distance_fwd, distance_perp, required_turning_radius, turning_rate, movement_rate;
	AI2tMovementDirection movementstate_direction;
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);

	UUmAssertReadPtr(active_character, (sizeof(*active_character)));

	if (ioMeleeState->attack_launched) {
		// we have launched our attack
		if (AI2rExecutor_HasAttackOverride(ioCharacter, NULL)) {
			// just wait until it is realised (or discarded)
			return UUcFalse;
		}
		
		// we have launched an attack; we cannot change our facing for this technique any longer.
		ioMeleeState->lock_facing_for_technique = UUcTrue;
		ioMeleeState->technique_face_direction = ioCharacter->facing;

		// we have actually performed this technique, we can't again for a while
		AI2iMeleeState_ApplyTechniqueDelay(ioCharacter, ioMeleeState);

		if (ioMeleeState->running_active_animation) {
			// don't go to the next move yet, unless the next move is another attack (and might be part of a combo)
			found_next_move = AI2iMelee_NextMove(ioCharacter, ioMeleeState, &next_move);
			if ((!found_next_move) || ((next_move & AI2cMoveType_Mask) != AI2cMoveType_Attack)) {
				return UUcFalse;
			}
		}

		// we're done.
		return AI2iMelee_StartNextMove(ioCharacter, ioMeleeState, inMove);
	}

	UUmAssert((inMove->move & AI2cMoveType_Mask) == AI2cMoveType_Attack);

	move_index = inMove->move & ~AI2cMoveType_Mask;
	UUmAssert((move_index >= 0) && (move_index < AI2gNumMeleeAttacks));

	attack_move = &AI2gMeleeAttackTable[move_index];


	if (inMove->is_broken) {
		// this is a broken move! don't even bother
		AI2_ERROR(AI2cBug, AI2cSubsystem_Melee, AI2cError_Melee_BrokenMove, ioCharacter,
			ioMeleeState, ioMeleeState->current_technique, inMove, 0);
		return UUcTrue;
	}
	UUmAssert(inMove->animation != NULL);

	next_animstate = TRrAnimation_GetTo(active_character->animation);
	next_state = AI2iMelee_TranslateTotoroAnimState(ioCharacter, next_animstate);
	cur_state = AI2iMelee_TranslateTotoroAnimState(ioCharacter, active_character->curFromState);

	if (((ioCharacter->flags2 & ONcCharacterFlag2_UltraMode) || AI2gUltraMode || (!ioMeleeState->committed_to_technique)) &&
		(ioMeleeState->technique_face_direction != AI2cAngle_None)) {
		// we want to turn to face the target before we start our attack
		
		if (ioMeleeState->facing_delta > AI2cMeleeDirectionalAttackMax) {
			// we are facing in a very wrong direction, abort this attack
#if DEBUG_VERBOSE_ATTACK
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: facing in totally wrong direction (%f > %f), abort",
					ioCharacter->player_name, ioMeleeState->facing_delta, AI2cMeleeDirectionalAttackMax);
#endif
#if DEBUG_VERBOSE_TECHNIQUE
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: aborting due to wrongfacing (%f > %f)", ioCharacter->player_name,
				ioMeleeState->facing_delta, AI2cMeleeDirectionalAttackMax);
#endif
			AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
			return UUcTrue;

		}
		
		if (ioMeleeState->facing_delta > AI2cMeleeMinStartAttackAngle) {
			// we are facing in the wrong direction - delay
			ONrSetupAnimIntersectionContext(ioCharacter, ioMeleeState->target, UUcTrue, &intersect_context);
			intersect_context.reject_reason = TRcRejection_Angle;
#if DEBUG_VERBOSE_ATTACK
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: attack 0x%08X: delaying, too far from technique facing %f (cur facing %f -> delta %f > %f)",
							ioCharacter->player_name, inMove->move, ioMeleeState->technique_face_direction,
							ioCharacter->facing, ioMeleeState->facing_delta, AI2cMeleeMinStartAttackAngle);
#endif
		} else if (ioMeleeState->position_force_attack) {
#if DEBUG_VERBOSE_ATTACK
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: attack forced by previous positioning move", ioCharacter->player_name);
#endif
			can_attack = UUcTrue;
		} else {
			// set up an intersection context for us and our target
			ONrSetupAnimIntersectionContext(ioCharacter, ioMeleeState->target, UUcTrue, &intersect_context);

			// we pass in inStartJump of false because we should already be jumping by now if we're going to
			AI2iMelee_SetupAttackerState(ioCharacter, ioCharacter->ai2State.meleeProfile, &intersect_context.attacker,
											ioMeleeState->current_technique_ends_in_jump, ioMeleeState->current_technique_jump_vel,
											AI2mMelee_AttackDirection(attack_move->attackmove_flags), inMove->animation);

#if DEBUG_SHOWINTERSECTIONS
			TRgStoreThisIntersection = MELEE_CHECKINTERSECTIONS;
#endif
			can_attack = TRrIntersectAnimations(&intersect_context);
#if DEBUG_VERBOSE_ATTACK
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: intersect animations returned %s (%s)",
								ioCharacter->player_name, (can_attack) ? "true" : "false", (can_attack) ? "ok" :
						(intersect_context.reject_reason == TRcRejection_NoAttack ) ? "no-attack" :
						(intersect_context.reject_reason == TRcRejection_Distance ) ? "distance" :
						(intersect_context.reject_reason == TRcRejection_Angle    ) ? "angle" :
						(intersect_context.reject_reason == TRcRejection_Behind   ) ? "behind" :
						(intersect_context.reject_reason == TRcRejection_AboveHead) ? "above-head" :
						(intersect_context.reject_reason == TRcRejection_BelowFeet) ? "below-feet" : "UNKNOWN");
#endif
		}

		if (!can_attack) {
			// under some circumstances we might want to continue pressing the keys from our last
			// positioning move until the move becomes available to us.
			if (!ioMeleeState->attack_waspositioned) {
				// there is no previous positioning move
				should_abort = UUcTrue;
#if (DEBUG_VERBOSE_ATTACK || DEBUG_VERBOSE_TECHNIQUE)
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: no previous positioning move", ioCharacter->player_name);
#endif

			} else if ((intersect_context.reject_reason == TRcRejection_AboveHead) ||
						(intersect_context.reject_reason == TRcRejection_BelowFeet)) {
				// there is no way that we could delay just to get within height range
				should_abort = UUcTrue;
#if (DEBUG_VERBOSE_ATTACK || DEBUG_VERBOSE_TECHNIQUE)
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: attack won't hit due to height", ioCharacter->player_name);
#endif

			} else if (intersect_context.reject_reason == TRcRejection_Behind) {
				// the target is behind us by the time we start attacking, this attack can never succeed
				should_abort = UUcTrue;
#if (DEBUG_VERBOSE_ATTACK || DEBUG_VERBOSE_TECHNIQUE)
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: attack won't hit, target behind", ioCharacter->player_name);
#endif

			} else {
				should_abort = UUcTrue;
				delay = UUcTrue;

				if ((intersect_context.reject_reason == TRcRejection_Distance) &&
						(ioMeleeState->facing_delta < AI2cMeleeGiveUpAttackAngle) &&
						(ioMeleeState->attack_waspositioned) && (ioMeleeState->position_lastkeys & LIc_BitMask_MoveKeys)) {
					// we were rejected based on distance and we're moving (presumably in the right direction), keep trying
					should_abort = UUcFalse;
#if (DEBUG_VERBOSE_ATTACK || DEBUG_VERBOSE_TECHNIQUE)
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: facing %f < %f and rejected due to distance, keep trying",
						ioMeleeState->facing_delta, AI2cMeleeGiveUpAttackAngle, ioCharacter->player_name);
#endif

				} else if (intersect_context.reject_reason == TRcRejection_Angle) {
					if ((AI2cMeleeState_CanTurnForAttack[cur_state]) &&
						(ioMeleeState->facing_delta < AI2cMeleeMinStartAttackAngle) &&
						(ioMeleeState->facing_delta > AI2cMeleeGiveUpAttackAngle)) {
						// we can theoretically keep turning in order to get to the target.
						should_abort = UUcFalse;

						if ((ioMeleeState->position_lastkeys & LIc_BitMask_MoveKeys) > 0) {
							// make sure that we don't get stuck orbiting the target!
							switch(AI2mMelee_AttackDirection(attack_move->attackmove_flags)) {
								case TRcDirection_None:
								case TRcDirection_Forwards:
									theta = ioCharacter->facing;
									movementstate_direction = AI2cMovementDirection_Forward;
									break;

								case TRcDirection_Left:
									theta = ioCharacter->facing + M3cHalfPi;
									UUmTrig_ClipHigh(theta);
									movementstate_direction = AI2cMovementDirection_Sidestep_Left;
									break;

								case TRcDirection_Right:
									theta = ioCharacter->facing - M3cHalfPi;
									UUmTrig_ClipLow(theta);
									movementstate_direction = AI2cMovementDirection_Sidestep_Right;
									break;

								case TRcDirection_Back:
									theta = ioCharacter->facing + M3cPi;
									UUmTrig_ClipHigh(theta);
									movementstate_direction = AI2cMovementDirection_Backpedal;
									break;

								default:
									UUmAssert(!"AI2iMelee_Attack_Update: unknown attack direction");
									theta = ioCharacter->facing;
									movementstate_direction = AI2cMovementDirection_Forward;
									break;
							}

							sintheta = MUrSin(theta);		costheta = MUrCos(theta);
							distance_fwd  = (float)fabs(sintheta * ioMeleeState->vector_to_target.x + costheta * ioMeleeState->vector_to_target.z);
							distance_perp = (float)fabs(costheta * ioMeleeState->vector_to_target.x - sintheta * ioMeleeState->vector_to_target.z);
#if DEBUG_VERBOSE_ATTACK
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: checking turning radius - theta %f vec (%f %f %f) -> fwd %f perp %f",
												ioCharacter->player_name, theta,
												ioMeleeState->vector_to_target.x, ioMeleeState->vector_to_target.y, ioMeleeState->vector_to_target.z,
												distance_fwd, distance_perp);
#endif

							// what turning radius is required to reach the facing before we get to the target?
							if (distance_perp - intersect_context.target_cylinder_radius > 0.1f) {
								distance_perp -= intersect_context.target_cylinder_radius;
								
								// trigonometry tells us that r = (perp^2 + fwd^2) / 2 * perp
								required_turning_radius = (UUmSQR(distance_perp) + UUmSQR(distance_fwd)) / (2.0f * distance_perp);

								// what is our current turning radius?
								turning_rate = AI2rExecutor_GetMaxDeltaFacingPerFrame(ioCharacter->characterClass, AI2cMovementMode_NoAim_Run, UUcTrue);
								movement_rate = ONrCharacterClass_MovementSpeed(ioCharacter->characterClass, movementstate_direction,
																				AI2cMovementMode_NoAim_Run, UUcFalse, UUcFalse);
								if (movement_rate / turning_rate > AI2cMelee_TurningRadiusSafety * required_turning_radius) {
									// abort the technique, we can't turn to reach the target and will circle indefinitely
									should_abort = UUcTrue;
#if (DEBUG_VERBOSE_ATTACK || DEBUG_VERBOSE_TECHNIQUE)
									COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: turning radius %f (move rate %f / turn rate %f ) > required %f, abort attack",
														ioCharacter->player_name, movement_rate / turning_rate, movement_rate, turning_rate, required_turning_radius);
#endif
								} else {
#if DEBUG_VERBOSE_ATTACK
									COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: turning radius %f (move rate %f / turn rate %f ) less than required %f, don't abort",
														ioCharacter->player_name, movement_rate / turning_rate, movement_rate, turning_rate, required_turning_radius);
#endif
								}
							} else {
#if DEBUG_VERBOSE_ATTACK
								COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: perp-dist %f less than target radius %f, don't abort",
													ioCharacter->player_name, distance_perp, intersect_context.target_cylinder_radius);
#endif
							}
						}

#if (DEBUG_VERBOSE_ATTACK || DEBUG_VERBOSE_TECHNIQUE)
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: facing %f < %f and rejected due to angle, keep trying",
							ioCharacter->player_name, ioMeleeState->facing_delta, AI2cMeleeMinStartAttackAngle);
#endif
					} else {
						// we can't delay in order to get to the target; just launch our attack now
						should_abort = UUcFalse;
						delay = UUcFalse;
#if DEBUG_VERBOSE_ATTACK
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: rejected due to angle but can't delay to turn. launch now anyway.",
							ioCharacter->player_name, ioMeleeState->facing_delta, AI2cMeleeMinStartAttackAngle);
#endif
					}

				} else {
#if (DEBUG_VERBOSE_ATTACK || DEBUG_VERBOSE_TECHNIQUE)
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: can't compensate for non-hit reason (%s)",
						ioCharacter->player_name, TRcIntersectRejectionName[intersect_context.reject_reason]);
#endif

				}
			}

			if (should_abort) {
				// give up on this attack.
#if (DEBUG_VERBOSE_ATTACK || DEBUG_VERBOSE_TECHNIQUE)
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: can't hit with attack, abort", ioCharacter->player_name);
#endif
				AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
				return UUcTrue;

			} else if (delay) {
				// delay, and press keys as appropriate from last positioning info
#if DEBUG_VERBOSE_ATTACK
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: delaying attack until in-range, pressing keys", ioCharacter->player_name);
#endif
				if ((ioMeleeState->attack_waspositioned) && (ioMeleeState->position_lastkeys != 0)) {
					AI2rExecutor_MoveOverride(ioCharacter, ioMeleeState->position_lastkeys);
				}
				return UUcFalse;
			} else {
#if DEBUG_VERBOSE_ATTACK
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: attack may miss, but launching now anyway", ioCharacter->player_name);
#endif
			}
		}
	}

	if (AI2rExecutor_HasAttackOverride(ioCharacter, &attack_type)) {
		// we already have an attack trying to launch. don't overwrite it.
#if DEBUG_VERBOSE_ATTACK
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: attack 0x%08X: delaying, already launching attack (animtype %s)",
						ioCharacter->player_name, inMove->move, ONrAnimTypeToString(attack_type));
#endif
		// delay, and leave keys alone (because attack_override should be doing them)
		return UUcFalse;

	} else if ((active_character->lastAttack != attack_move->combo_prev_animtype) &&
				(attack_move->combo_prev_animtype != ONcAnimType_None)) {
		// we are a combo attack, and there is the wrong combo flag (or no combo flag) set. we can't
		// execute.
#if DEBUG_VERBOSE_ATTACK
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: attack 0x%08X: combo anim %s and we require %s - bail",
						ioCharacter->player_name, inMove->move, ONrAnimTypeToString(active_character->lastAttack),
						ONrAnimTypeToString(attack_move->combo_prev_animtype));
#endif
		AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
		return UUcTrue;
	}

	// if our current animation is atomic, or we have to wait until it's finished, don't interrupt it
	if ((TRrAnimation_IsAtomic(active_character->animation, active_character->animFrame)) ||
		((cur_state != inMove->current_state) && (next_state == inMove->current_state))) {

		// work out how many frames before this animation ends we can queue up our
		// desired attack
		tolerance_frames = 0;
		test_animation = TRrCollection_Lookup(ioCharacter->characterClass->animations,
											TRrAnimation_GetType(inMove->animation),
											next_animstate, active_character->animVarient | ONcAnimVarientMask_Fight);
		if (test_animation) {
			tolerance_frames = AI2cMeleeAttackQueue_HardFrames;
#if DEBUG_VERBOSE_ATTACK
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: attack 0x%08X: animation %s (type %s) direct lookup! (current tostate %s, fromstate %s)",
							ioCharacter->player_name, inMove->move, TMrInstance_GetInstanceName(inMove->animation),
							ONrAnimTypeToString(TRrAnimation_GetType(inMove->animation)), ONrAnimStateToString(next_animstate), 
							ONrAnimStateToString(TRrAnimation_GetFrom(inMove->animation)));
#endif
		} else if (TRrAnimation_IsShortcut(inMove->animation, next_animstate)) {
			tolerance_frames = AI2cMeleeAttackQueue_SoftFrames;
#if DEBUG_VERBOSE_ATTACK
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: attack 0x%08X: animation %s (type %s) shortcut lookup! (current tostate %s, fromstate %s)",
							ioCharacter->player_name, inMove->move, TMrInstance_GetInstanceName(inMove->animation),
							ONrAnimTypeToString(TRrAnimation_GetType(inMove->animation)), ONrAnimStateToString(next_animstate), 
							ONrAnimStateToString(TRrAnimation_GetFrom(inMove->animation)));
#endif
		}

		frames_until_nextanim = TRrAnimation_GetDuration(active_character->animation) - active_character->animFrame;
		if (frames_until_nextanim > tolerance_frames) {
			// we can't queue up our desired attack yet.
#if DEBUG_VERBOSE_ATTACK
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: attack 0x%08X: frames until nextanim %d > queue tolerance %d (current %s -> %s), delaying attack (%s)",
							ioCharacter->player_name, inMove->move, frames_until_nextanim, tolerance_frames,
							TMrInstance_GetInstanceName(active_character->animation), ONrAnimStateToString(next_animstate),
							TMrInstance_GetInstanceName(inMove->animation));
#endif
			// delay, and press keys as appropriate from last positioning info
			if ((ioMeleeState->attack_waspositioned) && (ioMeleeState->position_lastkeys != 0)) {
				AI2rExecutor_MoveOverride(ioCharacter, ioMeleeState->position_lastkeys);
			}
			return UUcFalse;
		} else {
#if DEBUG_VERBOSE_ATTACK
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: attack 0x%08X: queueing up attack %s (current %s -> %s)",
							ioCharacter->player_name, inMove->move, TMrInstance_GetInstanceName(inMove->animation),
							TMrInstance_GetInstanceName(active_character->animation), ONrAnimStateToString(next_animstate));
#endif
		}
	} else {
#if DEBUG_VERBOSE_ATTACK
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: attack 0x%08X: interrupting non-atomic animation (%s)",
						ioCharacter->player_name, inMove->move, TMrInstance_GetInstanceName(active_character->animation));
#endif
	}

	// we are just about to launch the attack - check to see that it wouldn't send us into a danger square
	should_abort = AI2iMelee_WeightLocalMovement(ioCharacter, ioMeleeState, ioMeleeState->current_technique, inMove->attack_endpos, NULL);
	if (should_abort) {
#if (DEBUG_VERBOSE_ATTACK || DEBUG_VERBOSE_TECHNIQUE)
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: attack distance %f sends us into danger, abort",
							ioCharacter->player_name, inMove->attack_endpos);
#endif
		AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
		return UUcTrue;
	}

	// queue up the attack.
#if DEBUG_VERBOSE_ATTACK
	COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: attack 0x%08X: executing '%s' of type %s",
					ioCharacter->player_name, inMove->move, attack_move->name, ONrAnimTypeToString(attack_move->anim_type));
#endif
	AI2rExecutor_AttackOverride(ioCharacter, attack_move->keys_isdown, attack_move->keys_wentdown,
								attack_move->anim_type, attack_move->combo_prev_animtype);
	ioMeleeState->attack_launched = UUcTrue;
	return UUcFalse;
}


static void AI2iMelee_Attack_Finish(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove)
{
	if ((ioMeleeState->current_technique->user_flags & AI2cTechniqueUserFlag_Interruptable) == 0) {
		// this technique is uninterruptable; we can't stop it now
		ioMeleeState->committed_to_technique = UUcTrue;
	}
}

static UUtBool AI2iMelee_Attack_FaceTarget(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
										  AI2tMeleeMove *inMove)
{
	return UUcTrue;
}

static UUtBool AI2iMelee_Attack_CanEvade(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
										  AI2tMeleeMove *inMove)
{
	return (!ioMeleeState->attack_launched);
}

// ------------------------------------------------------------------------------------
// -- positioning functions

static UUtUns32 AI2iMelee_Position_Iterate(UUtUns32 inIterator)
{
	UUtUns32 move_index;

	if (inIterator == (UUtUns32) -1) {
		move_index = 0;
	} else {
		UUmAssert((inIterator & AI2cMoveType_Mask) == AI2cMoveType_Position);

		move_index = (inIterator & ~AI2cMoveType_Mask);
		UUmAssert((move_index >= 0) && (move_index < AI2cMeleeMaxPositionMoves));

		move_index++;
	}

	if (move_index >= AI2cMeleeMaxPositionMoves) {
		// no more states that we could choose from
		return (UUtUns32) -1;
	} else {
		// this state is okay!
		return (AI2cMoveType_Position | move_index);
	}
}

static UUtBool AI2iMelee_Position_GetName(UUtUns32 inMove, char *outString, UUtUns32 inStringLength)
{
	UUtUns32 move_index;

	if ((inMove & AI2cMoveType_Mask) != AI2cMoveType_Position) {
		// not a positioning move!
		return UUcFalse;
	}

	move_index = (inMove & ~AI2cMoveType_Mask);
	if ((move_index >= 0) && (move_index < AI2cMeleeMaxPositionMoves)) {
		// store the name string
		UUrString_Copy(outString, AI2gMeleePositionTable[move_index].name, inStringLength);
		return UUcTrue;
	} else {
		// invalid move index
		return UUcFalse;
	}
}

static UUtUns32 AI2iMelee_Position_GetParams(UUtUns32 inMove, UUtBool inSpacing, char *outStringPtrs[3])
{
	UUtUns32 move_index;
	AI2tMelee_PositionMove *position_move;

	if ((inMove & AI2cMoveType_Mask) != AI2cMoveType_Position) {
		// not a positioning move!
		return 0;
	}

	move_index = (inMove & ~AI2cMoveType_Mask);
	if ((move_index < 0) || (move_index >= AI2cMeleeMaxPositionMoves)) {
		// invalid move index
		return 0;
	}

	// get the positioning move
	position_move = &AI2gMeleePositionTable[move_index];
	if (!position_move->can_delay)
		return 0;

	outStringPtrs[0] = "Min Run-In Dist";
	outStringPtrs[1] = "Max Run-In Dist";
	outStringPtrs[2] = "Tolerance Range";

	return 3;
}

static UUtUns16 AI2iMelee_Position_GetAnimType(UUtUns32 inMove, TRtDirection *outDirection,
											AI2tMeleeMoveState *outFromState, AI2tMeleeMoveState *outToState,
											UUtUns16 *outTargetAnimState, UUtUns16 *outTargetAnimVarient)
{
	// positioning moves don't have any animation type - they are handled separately in
	// AI2rMelee_MoveAnimationLookup.
	UUmAssert(!"AI2iMelee_Position_GetAnimType should never be called");
	return 0;
}

static UUtBool AI2iMelee_Position_CanFollow(UUtUns32 inMove, UUtUns32 inPrevMove)
{
	// positioning moves can go anywhere
	return UUcTrue;
}

// convert a totoro animation state into a melee state
static AI2tMeleeMoveState AI2iMelee_TranslateTotoroAnimState(ONtCharacter *inCharacter, UUtUns16 inAnimState)
{
	AI2tMeleeMoveState move_state;

	UUmAssert((inAnimState >= 0) && (inAnimState < ONcAnimState_Max));
	move_state = AI2gMelee_StateFromAnimState[inAnimState];
	UUmAssert((move_state >= 0) && (move_state < AI2cMeleeMoveState_Max));

	// if this is a jumping state without a direction, then we need to work out
	// which way the character is jumping!
	if (move_state == AI2cMeleeMoveState_Jumping) {
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

		// first check keys that are held down
		UUmAssertReadPtr(active_character, sizeof(*active_character));
		if (active_character->inputState.buttonIsDown & LIc_BitMask_Forward) {
			return AI2cMeleeMoveState_JumpingForwards;

		} else if (active_character->inputState.buttonIsDown & LIc_BitMask_StepLeft) {
			return AI2cMeleeMoveState_JumpingLeft;

		} else if (active_character->inputState.buttonIsDown & LIc_BitMask_StepRight) {
			return AI2cMeleeMoveState_JumpingRight;

		} else if (active_character->inputState.buttonIsDown & LIc_BitMask_Backward) {
			return AI2cMeleeMoveState_JumpingBack;
		}
	}

	return move_state;
}

// get a character's current animation state (accounting for shortcuts, etc)
static UUtUns16 AI2iMelee_Position_GetCurrentAnimState(ONtCharacter *inCharacter, AI2tMeleeMoveState *outState)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);
	const TRtAnimation *anim;
	UUtUns16 frames_remaining, current_animstate;

	UUmAssertReadPtr(active_character, sizeof(*active_character));
	anim = active_character->animation;

	// calculate what our current state is.
	frames_remaining = TRrAnimation_GetDuration(anim);
	UUmAssert((active_character->animFrame >= 0) && (active_character->animFrame <= frames_remaining));
	frames_remaining -= active_character->animFrame;

	if (frames_remaining <= AI2cMelee_PositionTransition_ToFrames) {
		current_animstate = TRrAnimation_GetTo(anim);
	} else {
		if (active_character->stitch.stitching) {
			current_animstate = active_character->stitch.fromState;
		} else {
			current_animstate = TRrAnimation_GetFrom(anim);
		}

		if (current_animstate == ONcAnimState_None) {
			current_animstate = TRrAnimation_GetTo(anim);
		}
	}

	if (current_animstate >= ONcAnimState_Max) {
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "### WARNING: character '%s' playing anim %s passes through unknown animstate %d, add to Oni_Character_Animation.h",
						inCharacter->player_name, TMrInstance_GetInstanceName(anim), current_animstate);
		current_animstate = ONcAnimState_None;
	}

	UUmAssert((current_animstate >= 0) && (current_animstate < ONcAnimState_Max));

	if (outState != NULL) {
		*outState = AI2iMelee_TranslateTotoroAnimState(inCharacter, current_animstate);
	}

	return current_animstate;
}

static UUtBool AI2iMelee_Position_Start(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove)
{
	UUtUns32 move_index, state_index;
	TRtAnimState cur_to_state;
	AI2tMeleeMoveState cur_state;
	AI2tMelee_PositionMove *position_move;

	UUmAssert((inMove->move & AI2cMoveType_Mask) == AI2cMoveType_Position);

	move_index = inMove->move & ~AI2cMoveType_Mask;
	UUmAssert((move_index >= 0) && (move_index < AI2cMeleeMaxPositionMoves));

	position_move = &AI2gMeleePositionTable[move_index];

	if (ioMeleeState->positionmove_skip == ioMeleeState->move_index) {
		// we are skipping this position move, because we're already in position
#if DEBUG_VERBOSE_POSITION
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: skip position move %d (%s)",
			ioCharacter->player_name, ioMeleeState->positionmove_skip, position_move->name);
#endif
		return AI2iMelee_StartNextMove(ioCharacter, ioMeleeState, inMove);
	} else {
#if DEBUG_VERBOSE_POSITION
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: start position move %s", ioCharacter->player_name, position_move->name);
#endif
	}

	// start at the start of the positioning move unless otherwise noted
	ioMeleeState->position_state_index = 0;
	ioMeleeState->position_skipped_delay = UUcFalse;

	if (!position_move->can_delay) {
		// work out what state we are currently in
		cur_to_state = AI2iMelee_Position_GetCurrentAnimState(ioCharacter, &cur_state);

		// work out what the next state is that we have to get to.
 		for (state_index = 0; state_index < position_move->num_states; state_index++) {
			if (cur_state == position_move->states[state_index])
				break;
		}

		if ((state_index > 0) && (state_index < position_move->num_states)) {
			// we are starting somewhere in the middle of the positioning move, because
			// we're already in the right state. if we are starting after the delay state,
			// and it turns out that we need to delay, we have to be able to reverse this.
			ioMeleeState->position_state_index = state_index;
			ioMeleeState->position_skipped_delay = UUcTrue;
		}

#if DEBUG_VERBOSE_POSITION
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  (current tostate %s [%s] -> skip = %s, try for state %d of %d [%s])",
			ONrAnimStateToString(cur_to_state), AI2cMeleeMoveStateName[cur_state], ioMeleeState->position_skipped_delay
				? "yes" : "no", ioMeleeState->position_state_index + 1, position_move->num_states,
				AI2cMeleeMoveStateName[position_move->states[state_index]]);
#endif
	}

	// set up for the positioning update code
	ioMeleeState->position_desired_state = position_move->states[ioMeleeState->position_state_index];
	ioMeleeState->position_current_transition = NULL;
	ioMeleeState->position_jumpkey_down = UUcFalse;
	ioMeleeState->position_missbyheight_timer = 0;

	return AI2iMelee_Position_Update(ioCharacter, ioMeleeState, inMove);
}


static UUtBool AI2iMelee_Position_Update(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove)
{
	UUtUns16 first_attackframe, last_attackframe;
	UUtUns32 move_index, state_index, num_transitions, max_frames_to_hit;
	UUtBool should_abort, delay_transition, localmove_success, found_transition, hit_with_attack;
	float delay_distance, target_dist, endtrans_dist, endtrans_frames, sintheta, costheta, target_closing_vel, jump_dist;
	M3tVector3D target_dir, target_vel, target_movedelta;
	TRtAnimState current_animstate, iterate_animstate;
	TRtDirection move_direction;
	AI2tMeleeMoveState current_state, next_state, iterate_state;
	AI2tMelee_PositionMove *position_move;
	AI2tMeleeProfile *profile = ioCharacter->ai2State.meleeProfile;
	TRtAnimIntersectContext anim_context;
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
#if DEBUG_VERBOSE_POSITION
	AI2tMeleeMoveState orig_state;
#endif

	UUmAssertReadPtr(active_character, sizeof(*active_character));
	UUmAssert((inMove->move & AI2cMoveType_Mask) == AI2cMoveType_Position);

	move_index = inMove->move & ~AI2cMoveType_Mask;
	UUmAssert((move_index >= 0) && (move_index < AI2cMeleeMaxPositionMoves));

	position_move = &AI2gMeleePositionTable[move_index];

	// calculate what our current state is.
	current_animstate = AI2iMelee_Position_GetCurrentAnimState(ioCharacter, &current_state);
#if DEBUG_VERBOSE_POSITION
	if (current_state == AI2cMeleeMoveState_None) {
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: warning, current animstate %s means our current state is %s",
			ioCharacter->player_name, ONrAnimStateToString(current_animstate), AI2cMeleeMoveStateName[current_state]);
	}
#endif

	if (ioMeleeState->position_desired_state == current_state) {
#if DEBUG_VERBOSE_POSITION
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: position update: current state %s == desired (%d)",
			ioCharacter->player_name, AI2cMeleeMoveStateName[current_state], ioMeleeState->position_state_index);
#endif
		delay_transition = UUcFalse;

		if (ioMeleeState->facing_delta > AI2cMeleeMinStartAttackAngle) {
			// we are facing too far away from where we should be.
#if DEBUG_VERBOSE_POSITION
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: angle is too great to attack (%f > %f).",
				ioCharacter->player_name, ioMeleeState->facing_delta, AI2cMeleeMinStartAttackAngle);
#endif

			if (ioMeleeState->position_notfacing_timer > 0) {
				ioMeleeState->position_notfacing_timer--;

				if (ioMeleeState->position_notfacing_timer == 0) {
					// abort this technique; we have been facing in the wrong direction for too long
#if (DEBUG_VERBOSE_POSITION || DEBUG_VERBOSE_TECHNIQUE)
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: exceeded maximum notfacing timer %d! abort.",
									ioCharacter->player_name, AI2cMelee_PositionNotFacingTimer);
#endif
					AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
					return UUcTrue;
				}
			} else {
				// we are facing in the wrong direction - start a timer
				ioMeleeState->position_notfacing_timer = AI2cMelee_PositionNotFacingTimer;
			}
#if DEBUG_VERBOSE_POSITION
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: notfacing timer is %d", ioCharacter->player_name, ioMeleeState->position_notfacing_timer);
#endif

			if ((position_move->can_wait_for_facing) &&
				(ioMeleeState->position_state_index == position_move->num_states - 1)) {
				// don't yet transition into our attack state.
#if DEBUG_VERBOSE_POSITION
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: last state; delaying transition to attack due to angle delta.",
					ioCharacter->player_name);
#endif
				delay_transition = UUcTrue;
			}
		}

		if ((position_move->can_delay && (ioMeleeState->position_state_index == 0)) ||
			(position_move->jump_type != AI2cMeleeJumpType_None)) {
#if DEBUG_VERBOSE_POSITION
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: we could delay from here, checking bounds", ioCharacter->player_name);
#endif

			// work out how long it will take us, after we stop delaying, to transition to the state
			// from where we can launch the attack.
			endtrans_dist = 0;
			endtrans_frames = 0;
			iterate_state = current_state;
			iterate_animstate = current_animstate;

			if (!position_move->ignore_endtrans_dist) {
				for (state_index = ioMeleeState->position_state_index; state_index < position_move->num_states; state_index++) {
#if DEBUG_VERBOSE_POSITION
					orig_state = iterate_state;
#endif
					found_transition = AI2iMelee_GetTransitionLength(profile, &iterate_state, &iterate_animstate,
																	position_move->states[state_index], NULL,
																	&endtrans_dist, &endtrans_frames, &num_transitions);
					if (!found_transition) {
#if (DEBUG_VERBOSE_POSITION || DEBUG_VERBOSE_TECHNIQUE)
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: couldn't find any transitions %s[%s] -> %s, aborting!",
							ioCharacter->player_name, AI2cMeleeMoveStateName[iterate_state], ONrAnimStateToString(iterate_animstate),
							AI2cMeleeMoveStateName[position_move->states[state_index]]);
#endif
						// no transitions are available from our current state! abort the positioning, which means
						// that we have to abort the technique as well...
						AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
						return UUcTrue;
					}	

#if DEBUG_VERBOSE_POSITION
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "  PositionEndTrans: transition chain %s -> %s, length %d dist %f frames %f\n",
						AI2cMeleeMoveStateName[orig_state], AI2cMeleeMoveStateName[iterate_state],
						num_transitions, endtrans_dist, endtrans_frames);
#endif
				}
			}

			if ((ioMeleeState->current_technique->attack_anim != NULL) &&
				(ioMeleeState->move_index < ioMeleeState->current_technique->attackmove_itr)) {
				/*
				 * there is an attack animation still to come in the technique - trigger off this to work out
				 * when we should stop the delaying position
				 */

				// build an intersection context between us and the target to see if we should keep moving
				// without attacking yet.
				ONrSetupAnimIntersectionContext(ioCharacter, ioMeleeState->target, UUcTrue, &anim_context);

				if (endtrans_dist > 0) {
					// account for this offset at the end of the delay period by moving us in our animspace
					switch(position_move->direction) {
						case TRcDirection_Forwards:
							anim_context.current_location_matrix.m[3][1] -= endtrans_dist;
#if DEBUG_SHOWINTERSECTIONS
							anim_context.attacker.position.location.x += MUrSin(anim_context.attacker.position.facing) * endtrans_dist;
							anim_context.attacker.position.location.z += MUrCos(anim_context.attacker.position.facing) * endtrans_dist;
#endif
						break;

						case TRcDirection_Left:
							anim_context.current_location_matrix.m[3][0] -= endtrans_dist;
#if DEBUG_SHOWINTERSECTIONS
							anim_context.attacker.position.location.x += MUrCos(anim_context.attacker.position.facing) * endtrans_dist;
							anim_context.attacker.position.location.z -= MUrSin(anim_context.attacker.position.facing) * endtrans_dist;
#endif
						break;

						case TRcDirection_Right:
							anim_context.current_location_matrix.m[3][0] += endtrans_dist;
#if DEBUG_SHOWINTERSECTIONS
							anim_context.attacker.position.location.x -= MUrCos(anim_context.attacker.position.facing) * endtrans_dist;
							anim_context.attacker.position.location.z += MUrSin(anim_context.attacker.position.facing) * endtrans_dist;
#endif
						break;

						case TRcDirection_Back:
							anim_context.current_location_matrix.m[3][1] += endtrans_dist;
#if DEBUG_SHOWINTERSECTIONS
							anim_context.attacker.position.location.x -= MUrSin(anim_context.attacker.position.facing) * endtrans_dist;
							anim_context.attacker.position.location.z -= MUrCos(anim_context.attacker.position.facing) * endtrans_dist;
#endif
						break;

						default:
							UUmAssert(0);
						break;
					}
				}

				if ((position_move->jump_type == AI2cMeleeJumpType_Standing) || (position_move->jump_type == AI2cMeleeJumpType_Vertical)) {
					// we don't take account of the target's velocity when performing these moves

				} else if (endtrans_frames > 0) {
					// transform the target's current velocity from target animspace to attacker animspace
					MUrMatrix3x3_MultiplyPoint(&anim_context.target_velocity_estimate,
						(M3tMatrix3x3 *) &anim_context.current_location_matrix, &target_vel);

					// FIXME: we should also account for vertical movement, both of ourselves and the target. perhaps this
					// can be done by storing it in the character?  add to target_vel.y

					// account for how far the target will move while we perform our end transition
					MUmVector_ScaleCopy(target_movedelta, endtrans_frames, target_vel);
					anim_context.current_location_matrix.m[3][0] += target_movedelta.x;
					anim_context.current_location_matrix.m[3][1] += target_movedelta.y;
					anim_context.current_location_matrix.m[3][2] += target_movedelta.z;
#if DEBUG_SHOWINTERSECTIONS
					anim_context.target.position.location.x += MUrSin(anim_context.attacker.position.facing) * target_movedelta.y + MUrCos(anim_context.attacker.position.facing) * target_movedelta.x;
					anim_context.target.position.location.y += target_movedelta.z;
					anim_context.target.position.location.z += MUrCos(anim_context.attacker.position.facing) * target_movedelta.y - MUrSin(anim_context.attacker.position.facing) * target_movedelta.x;
#endif
				}

#if DEBUG_VERBOSE_POSITION
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "    checking bounds: (inair %s) jump %s, velocity %f\n",
					(active_character->inAirControl.numFramesInAir > 0) ? "yes" : "no", ioMeleeState->current_technique_ends_in_jump
					? "yes" : "no", ioMeleeState->current_technique_jump_vel);
#endif
				AI2iMelee_SetupAttackerState(ioCharacter, ioCharacter->ai2State.meleeProfile, &anim_context.attacker,
											ioMeleeState->current_technique_ends_in_jump, ioMeleeState->current_technique_jump_vel,
											position_move->direction, ioMeleeState->current_technique->attack_anim);
				
				// check to see if we can intersect with the target.
				if (!TRrCheckAnimationBounds(&anim_context)) {
#if DEBUG_VERBOSE_POSITION
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: bounds check on attack failed", ioCharacter->player_name);
#endif
					delay_transition = UUcTrue;
				} else {
#if DEBUG_VERBOSE_POSITION
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: bounds check on attack succeeded, stop delaying", ioCharacter->player_name);
#endif
				}

				next_state = AI2iMelee_TranslateTotoroAnimState(ioCharacter, TRrAnimation_GetTo(active_character->animation));

				if (AI2rMelee_StateIsJumping(next_state) || (ONrCharacter_InAir(ioCharacter))) {
					// we are jumping... delay unless we could actually hit from here
					switch(position_move->direction) {
						case TRcDirection_None:
						case TRcDirection_360:
							target_dir = MUrMatrix_GetTranslation(&anim_context.current_location_matrix);
							target_dist = MUrSqrt(UUmSQR(target_dir.x) + UUmSQR(target_dir.y));
						case TRcDirection_Forwards:
							target_dist =  anim_context.current_location_matrix.m[3][1];
						break;

						case TRcDirection_Left:
							target_dist =  anim_context.current_location_matrix.m[3][0];
						break;

						case TRcDirection_Right:
							target_dist = -anim_context.current_location_matrix.m[3][0];
						break;

						case TRcDirection_Back:
							target_dist = -anim_context.current_location_matrix.m[3][1];
						break;

						default:
							UUmAssert(0);
							target_dist = 0;
						break;
					}

#if DEBUG_SHOWINTERSECTIONS
					TRgStoreThisIntersection = MELEE_CHECKINTERSECTIONS;
#endif
					if (target_dist < anim_context.target_cylinder_radius +
						ioMeleeState->current_technique->attack_idealrange) {
						// we are right next to the target, force an attack
#if DEBUG_VERBOSE_POSITION
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: next to target (dist %f < cyl %f + range %f), force attack",
							ioCharacter->player_name, target_dist, anim_context.target_cylinder_radius,
							ioMeleeState->current_technique->attack_idealrange);
#endif
						hit_with_attack = UUcTrue;
						ioMeleeState->position_force_attack = UUcTrue;

					} else if (TRrIntersectAnimations(&anim_context)) {
						if (ioMeleeState->position_missbyheight_timer > 0) {
							// we must wait a few frames, as we only just reached the right height
							ioMeleeState->position_missbyheight_timer--;
							hit_with_attack = UUcFalse;
#if DEBUG_VERBOSE_POSITION
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: waiting for missbyheight timer (%d)",
								ioCharacter->player_name, ioMeleeState->position_missbyheight_timer);
#endif

						} else {
							// we must hit with a significant fraction of our attack frames remaining.
							last_attackframe	= TRrAnimation_GetLastAttackFrame(anim_context.attacker.animation);
							first_attackframe	= TRrAnimation_GetFirstAttackFrame(anim_context.attacker.animation);
							first_attackframe = UUmMax(first_attackframe, anim_context.attacker.animFrame);

							max_frames_to_hit = last_attackframe - (last_attackframe - first_attackframe) / 3;

							hit_with_attack = (anim_context.frames_to_hit <= max_frames_to_hit);
#if DEBUG_VERBOSE_POSITION
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: anims intersect at %d frames, which is %s (first %d, last %d)",
								ioCharacter->player_name, anim_context.frames_to_hit, (hit_with_attack) ? "fine" : "too late",
								first_attackframe, last_attackframe);
#endif
						}
					} else {
						// our attack misses
						hit_with_attack = UUcFalse;
#if DEBUG_VERBOSE_POSITION
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: anims don't intersect", ioCharacter->player_name);
#endif
					}

					if (!hit_with_attack) {
#if DEBUG_VERBOSE_POSITION
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: jumping, wouldn't hit from here if attack launched", ioCharacter->player_name);
#endif

						if (ioMeleeState->position_jumpkey_down && (anim_context.reject_reason == TRcRejection_AboveHead)) {
							// stop jumping, we will overshoot our target's height
							ioMeleeState->position_jumpkey_down = UUcFalse;
							ioMeleeState->position_lastkeys &= ~LIc_BitMask_Jump;
							ioMeleeState->position_missbyheight_timer = AI2cMelee_PositionMissByHeightTimer;
#if DEBUG_VERBOSE_POSITION
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: releasing jump key due to height-based intersect rejection. set timer.", ioCharacter->player_name);
#endif
						}
						delay_transition = UUcTrue;
					}

				} else if (delay_transition) {
					// we are delaying and we're on the ground. maybe this move isn't going to hit anyway.

					if (ioMeleeState->position_skipped_delay) {
						// we skipped a state in order to get here quickly... this must now be reversed since we see
						// that it was necessary in the first place.
#if DEBUG_VERBOSE_POSITION
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: right state, but need to delay. reverse state skipping.", ioCharacter->player_name);
#endif
						ioMeleeState->position_state_index = 0;
						ioMeleeState->position_desired_state = position_move->states[0];
						ioMeleeState->position_current_transition = NULL;
						ioMeleeState->position_skipped_delay = UUcFalse;
						return UUcFalse;					
					}

					if ((!position_move->can_delay) || (ioMeleeState->position_state_index != 0)) {
#if (DEBUG_VERBOSE_POSITION || DEBUG_VERBOSE_TECHNIQUE)
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: trying to delay but can't. aborting!", ioCharacter->player_name);
#endif
						// we can't hit from this fixed-length positioning move, and we are on the ground. abort.
						AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
						return UUcTrue;
					}

					if (ioMeleeState->facing_delta < AI2cMeleeMinStartAttackAngle) {
						// consider how far we would have to delay in order to hit the target.
						switch(position_move->direction) {
							case TRcDirection_360:
								target_dir = MUrMatrix_GetTranslation(&anim_context.current_location_matrix);
								target_dist = MUrSqrt(UUmSQR(target_dir.x) + UUmSQR(target_dir.y));
							case TRcDirection_Forwards:
								target_dist =  anim_context.current_location_matrix.m[3][1];
							break;

							case TRcDirection_Left:
								target_dist =  anim_context.current_location_matrix.m[3][0];
							break;

							case TRcDirection_Right:
								target_dist = -anim_context.current_location_matrix.m[3][0];
							break;

							case TRcDirection_Back:
								target_dist = -anim_context.current_location_matrix.m[3][1];
							break;

							default:
								UUmAssert(0);
								target_dist = 0;
							break;
						}

						// note that at this point, the target's closing distance (target_movedelta) has already been
						// added to the current_location_matrix, as has our endtrans_dist.

						// work out how far we have to delay in order to close with the target
						delay_distance = target_dist - anim_context.target_cylinder_radius;
						if (ioMeleeState->current_technique_ends_in_jump) {
							// we will be jumping at the end of this move. we want to be at roughly the midpoint of our jump
							// range from the target when we attack.
							// FIXME: do we want to be some fraction of our jump range away? like 0.75?
							delay_distance -= ioMeleeState->current_technique_jump_vel
								* 0.5f * (profile->min_jumpframes + profile->max_jumpframes);
						} else {
							// we're a ground-based move
							delay_distance -= ioMeleeState->current_technique->attack_idealrange;
						}

						if (delay_distance < 0) {
#if DEBUG_VERBOSE_POSITION
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: no hit, delay %f (dist %f - cyl %f - %s %f) < 0. abort!",
									ioCharacter->player_name, delay_distance, target_dist, anim_context.target_cylinder_radius,
									(ioMeleeState->current_technique_ends_in_jump) ? "jumpdist" : "idealrange",
									(ioMeleeState->current_technique_ends_in_jump) ? ioMeleeState->current_technique_jump_vel
									* 0.5f * (profile->min_jumpframes + profile->max_jumpframes)
									: ioMeleeState->current_technique->attack_idealrange, inMove->param[1]);
#endif
#if DEBUG_VERBOSE_TECHNIQUE
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: trying delay but too far to delay. abort.",
									ioCharacter->player_name);
#endif
							// we should have been able to hit this frame - we are at (or closer than) an ideal range
							// for our attack. however, TRrCheckAnimationBounds failed so we're at the wrong height.
							// give up.
							AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
							return UUcTrue;

						} else if (delay_distance > inMove->param[1] + inMove->param[2] + AI2cMelee_AdditionalDelayTolerance) {
#if DEBUG_VERBOSE_POSITION
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: no hit, delay %f (dist %f - cyl %f - %s %f) >> max %f. abort!",
									ioCharacter->player_name, delay_distance, target_dist, anim_context.target_cylinder_radius,
									(ioMeleeState->current_technique_ends_in_jump) ? "jumpdist" : "idealrange",
									(ioMeleeState->current_technique_ends_in_jump) ? ioMeleeState->current_technique_jump_vel
									* 0.5f * (profile->min_jumpframes + profile->max_jumpframes)
									: ioMeleeState->current_technique->attack_idealrange, inMove->param[1]);
#endif
#if DEBUG_VERBOSE_TECHNIQUE
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: trying delay but distance too short. abort.",
									ioCharacter->player_name);
#endif
							// we can't hit this frame, and we are outside of the range bound for which this technique
							// is valid anyway. abort.
							AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
							return UUcTrue;
						} else if (active_character->collidingWithTarget) {
							// we can't delay any further (since we are next to the target), abort
#if DEBUG_VERBOSE_TECHNIQUE
							COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: trying delay but colliding with target. abort.",
									ioCharacter->player_name);
#endif
							AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
							return UUcTrue;
						}
					}
				} else if (!active_character->collidingWithTarget) {
					if ((ioMeleeState->position_lastkeys & LIc_BitMask_MoveKeys) &&
						(!TRrIntersectAnimations(&anim_context)) && (anim_context.reject_reason == TRcRejection_Distance)) {
						// we aren't in range yet, and if we delay then we will be.
#if DEBUG_VERBOSE_POSITION
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: intersect not yet (distance)... delay a bit more",
								ioCharacter->player_name);
#endif
						delay_transition = UUcTrue;
					}
				}
			} else if ((ioMeleeState->current_technique->computed_flags & AI2cTechniqueComputedFlag_IsThrow) &&
						(ioMeleeState->current_technique->throw_distance > 0)) {
				/*
				 * there is a throw still to come in the technique - trigger off this to work out when we should stop the delaying position
				 */

				delay_distance = ioMeleeState->distance_to_target - AI2cMelee_ThrowDistSafety * ioMeleeState->current_technique->throw_distance;
				if (endtrans_frames > 0) {
					if ((position_move->direction != TRcDirection_Forwards) && (position_move->direction != TRcDirection_None)) {
						// there is a problem, we should always be closing forwards when trying to throw
						COrConsole_Printf("### %s/%s: throws must only be positioned using a forwards move",
							ioCharacter->ai2State.meleeProfile->name, ioMeleeState->current_technique->name);
						target_closing_vel = 0;
					} else {
						sintheta = MUrSin(ioCharacter->facing);
						costheta = MUrCos(ioCharacter->facing);

						// work out how fast our target is closing with us.
						MUmVector_Subtract(target_movedelta, ioMeleeState->target->location, ioMeleeState->target->prev_location);
						target_closing_vel = -(sintheta * target_movedelta.x + costheta * target_movedelta.z);
					}

					delay_distance -= target_closing_vel * endtrans_frames;
				}

				if (delay_distance > 0) {
					delay_transition = UUcTrue;
				}

				if (delay_transition) {
					// we are delaying and we're on the ground.
					if (ioMeleeState->position_skipped_delay) {
						// we skipped a state in order to get here quickly... this must now be reversed since we see
						// that it was necessary in the first place.
#if DEBUG_VERBOSE_POSITION
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: right state, but need to delay. reverse state skipping.", ioCharacter->player_name);
#endif
						ioMeleeState->position_state_index = 0;
						ioMeleeState->position_desired_state = position_move->states[0];
						ioMeleeState->position_current_transition = NULL;
						ioMeleeState->position_skipped_delay = UUcFalse;
						return UUcFalse;					
					}

					if ((!position_move->can_delay) || (ioMeleeState->position_state_index != 0)) {
#if (DEBUG_VERBOSE_POSITION || DEBUG_VERBOSE_TECHNIQUE)
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: trying to delay but can't. aborting!", ioCharacter->player_name);
#endif
						// we can't hit from this fixed-length positioning move, and we are on the ground. abort.
						AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
						return UUcTrue;
					}

					if (active_character->collidingWithTarget) {
						// we are colliding with the target and can't get into range, abort
#if (DEBUG_VERBOSE_POSITION || DEBUG_VERBOSE_TECHNIQUE)
						COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: colliding with target but can't throw. aborting!", ioCharacter->player_name);
#endif	
						AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
						return UUcTrue;
					}
				}

			} else {
				// an error has occurred, get out of this move
				delay_transition = UUcFalse;
			}
		}

		if (!delay_transition) {
			// determine which state we are trying to get to next
			ioMeleeState->position_state_index++;

			if (ioMeleeState->position_state_index >= position_move->num_states) {
				// we're done with the positioning
#if DEBUG_VERBOSE_POSITION
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: position update: finishing move", ioCharacter->player_name);
#endif
				ioMeleeState->attack_waspositioned = UUcTrue;
				return AI2iMelee_StartNextMove(ioCharacter, ioMeleeState, inMove);
			}
			
			ioMeleeState->position_desired_state = position_move->states[ioMeleeState->position_state_index];
#if DEBUG_VERBOSE_POSITION
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: position update: looking for next state %s (%d)",
				ioCharacter->player_name, AI2cMeleeMoveStateName[ioMeleeState->position_desired_state],
				ioMeleeState->position_state_index);
#endif
		}
	}

	if (ioMeleeState->position_desired_state != current_state) {
		if ((ioMeleeState->position_current_transition == NULL) ||
			(ioMeleeState->position_current_transition->from_state != current_state)) {
#if DEBUG_VERBOSE_POSITION
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: current transition (to %s) doesn't come from current state %s",
				ioCharacter->player_name, (ioMeleeState->position_current_transition == NULL) ? "<null>" :
				AI2cMeleeMoveStateName[ioMeleeState->position_current_transition->from_state],
				AI2cMeleeMoveStateName[current_state]);
#endif
			// we need to find another transition that gets us to the position that we want.
			ioMeleeState->position_current_transition = AI2iMelee_CheckStateTransition(ioCharacter, profile,
										current_state, ioMeleeState->position_desired_state, current_animstate);

			if (ioMeleeState->position_current_transition == NULL) {
#if (DEBUG_VERBOSE_POSITION || DEBUG_VERBOSE_TECHNIQUE)
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: couldn't find any transitions %s[%s] -> %s, aborting!",
					ioCharacter->player_name, AI2cMeleeMoveStateName[current_state], ONrAnimStateToString(current_animstate),
					AI2cMeleeMoveStateName[ioMeleeState->position_desired_state]);
#endif
				// no transitions are available from our current state! abort the positioning, which means
				// that we have to abort the technique as well...
				AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
				return UUcTrue;
			}

			// store the keypress required, so that if we have to delay before starting the next move we won't
			// delay with no keys down
			ioMeleeState->position_lastkeys = ioMeleeState->position_current_transition->move_keys;
			ioMeleeState->position_jumpkey_down = ((ioMeleeState->position_lastkeys & LIc_BitMask_Jump) != 0);

			if (ioMeleeState->position_jumpkey_down) {
				// we are about to jump - check to see that we won't go shooting off into a danger square
				jump_dist = ioMeleeState->current_technique_jump_vel * 0.5f * (profile->min_jumpframes + profile->max_jumpframes);
				should_abort = AI2iMelee_WeightLocalMovement(ioCharacter, ioMeleeState, ioMeleeState->current_technique, jump_dist, NULL);
				if (should_abort) {
#if (DEBUG_VERBOSE_POSITION || DEBUG_VERBOSE_TECHNIQUE)
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: jumping positioning move distance %f sends us into danger, abort",
										ioCharacter->player_name, jump_dist);
#endif
					AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
					return UUcTrue;
				}
			}
		}
	}

	if (ioMeleeState->position_current_transition == NULL) {
		// no transition found, because we're already in the correct state.
		// handle next time through.
		ioMeleeState->position_lastkeys = AI2cMeleeState_MaintainStateKeys[current_state];
		ioMeleeState->position_jumpkey_down = ((ioMeleeState->position_lastkeys & LIc_BitMask_Jump) != 0);
#if DEBUG_VERBOSE_POSITION
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: no transition necessary, already in '%s'",
			ioCharacter->player_name, AI2cMeleeMoveStateName[ioMeleeState->position_desired_state]);
#endif
	}

	// apply the movement that our current transition dictates
#if DEBUG_VERBOSE_POSITION
	COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: applying transition to '%s' (move key override 0x%16LX)",
		ioCharacter->player_name, (ioMeleeState->position_current_transition == NULL) ? "NULL" :
		AI2cMeleeMoveStateName[ioMeleeState->position_current_transition->to_state[0]], ioMeleeState->position_lastkeys);
#endif

	// position moves must always be free to face how they wish
	ioMeleeState->lock_facing_for_technique = UUcFalse;

	if (ioMeleeState->facing_delta > AI2cMeleeStandingTurnAngle) {
		if (ioMeleeState->position_state_index > 0) {
			// we are in the middle of our positioning, and the target is no longer even vaguely in front of us.
			if (ioMeleeState->position_notfacing_timer > 0) {
				ioMeleeState->position_notfacing_timer--;

				if (ioMeleeState->position_notfacing_timer == 0) {
					// abort this technique; we have been facing in the wrong direction for too long
#if (DEBUG_VERBOSE_POSITION || DEBUG_VERBOSE_TECHNIQUE)
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: position notfacing timer ran out, abort.", ioCharacter->player_name);
#endif
					AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
					return UUcTrue;
				}
			} else {
				// we are facing in the wrong direction - start a timer
				ioMeleeState->position_notfacing_timer = AI2cMelee_PositionNotFacingTimer;
			}
		}

		// turn on the spot here for a little while.
		AI2rExecutor_MoveOverride(ioCharacter, 0);
	} else {
		LItButtonBits movement_keys = ioMeleeState->position_lastkeys & LIc_BitMask_MoveKeys;

		if (delay_transition && (!ONrCharacter_InAir(ioCharacter)) &&
			(movement_keys != 0) && (movement_keys == ioMeleeState->position_lastkeys)) {

			// we're trying to move, and only move, in a direction on the ground.
			if (ioMeleeState->position_lastkeys & LIc_BitMask_Forward) {
				move_direction = TRcDirection_Forwards;

			} else if (ioMeleeState->position_lastkeys & LIc_BitMask_StepLeft) {
				move_direction = TRcDirection_Left;

			} else if (ioMeleeState->position_lastkeys & LIc_BitMask_Backward) {
				move_direction = TRcDirection_Back;

			} else if (ioMeleeState->position_lastkeys & LIc_BitMask_StepRight) {
				move_direction = TRcDirection_Right;

			} else {
				UUmAssert(0);
			}

			localmove_success = AI2iMelee_AddLocalMovement(ioCharacter, ioMeleeState, move_direction, UUcFalse);
			if (!localmove_success) {
				// we can't move in the direction that we want to. give up and abort.
#if (DEBUG_VERBOSE_POSITION || DEBUG_VERBOSE_TECHNIQUE)
				COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: local movement failed, abort.", ioCharacter->player_name);
#endif
				AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
				return UUcTrue;
			}
		} else {
			// we're either jumping, or doing a not-just-purely-movement transition.
			// hold down precisely the keys that we have to in order to transition to the next state.
			AI2rExecutor_MoveOverride(ioCharacter, ioMeleeState->position_lastkeys);

			if ((ioMeleeState->position_lastkeys == LIc_BitMask_Crouch) && (current_state == AI2cMeleeMoveState_Standing)) {
				// because there is no shortcut from turning-on-the-spot to crouching, we have to stop turning
				// before we can start crouching
				AI2rExecutor_FacingOverride(ioCharacter, AI2cAngle_None, UUcTrue);
			}
		}
	}

	return UUcFalse;
}


static void AI2iMelee_Position_Finish(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove)
{
	// nothing needs to be done here
#if DEBUG_VERBOSE_POSITION
	AI2tMelee_PositionMove *position_move;
	UUtUns32 move_index;

	UUmAssert((inMove->move & AI2cMoveType_Mask) == AI2cMoveType_Position);

	move_index = inMove->move & ~AI2cMoveType_Mask;
	UUmAssert((move_index >= 0) && (move_index < AI2cMeleeMaxPositionMoves));

	position_move = &AI2gMeleePositionTable[move_index];

	COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: position move %s finished", ioCharacter->player_name, position_move->name);
#endif
}

static UUtBool AI2iMelee_Position_FaceTarget(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
										  AI2tMeleeMove *inMove)
{
	return UUcTrue;
}

static UUtBool AI2iMelee_Position_CanEvade(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
										  AI2tMeleeMove *inMove)
{
	return !ONrCharacter_InAir(ioCharacter);
}

// ------------------------------------------------------------------------------------
// -- maneuvering move types

static UUtUns32 AI2iMelee_Maneuver_Iterate(UUtUns32 inIterator)
{
	UUtUns32 move_index;

	if (inIterator == (UUtUns32) -1) {
		move_index = 0;
	} else {
		UUmAssert((inIterator & AI2cMoveType_Mask) == AI2cMoveType_Maneuver);

		move_index = (inIterator & ~AI2cMoveType_Mask);
		UUmAssert((move_index >= 0) && (move_index < AI2cMeleeManeuver_Max));

		move_index++;
	}

	if (move_index >= AI2cMeleeManeuver_Max) {
		// no more maneuvers that we could choose from
		return (UUtUns32) -1;
	} else {
		// this maneuver is okay!
		return (AI2cMoveType_Maneuver | move_index);
	}
}

static UUtBool AI2iMelee_Maneuver_GetName(UUtUns32 inMove, char *outString, UUtUns32 inStringLength)
{
	UUtUns32 move_index;

	if ((inMove & AI2cMoveType_Mask) != AI2cMoveType_Maneuver) {
		// not a maneuver!
		return UUcFalse;
	}

	move_index = (inMove & ~AI2cMoveType_Mask);
	if ((move_index >= 0) && (move_index < AI2cMeleeManeuver_Max)) {
		// store the name string
		UUrString_Copy(outString, AI2gMeleeManeuverDefinition[move_index].name, inStringLength);
		return UUcTrue;
	} else {
		// invalid move index
		return UUcFalse;
	}
}

static UUtUns32 AI2iMelee_Maneuver_GetParams(UUtUns32 inMove, UUtBool inSpacing, char *outStringPtrs[3])
{
	UUtUns32 itr, move_index, num_params;
	char **param_names;
	AI2tMeleeManeuverDefinition *maneuver_def;

	if ((inMove & AI2cMoveType_Mask) != AI2cMoveType_Maneuver) {
		// not a maneuver!
		return 0;
	}

	move_index = (inMove & ~AI2cMoveType_Mask);
	if ((move_index < 0) || (move_index >= AI2cMeleeManeuver_Max)) {
		// invalid move index
		return 0;
	}

	if (move_index == AI2cMeleeManeuver_RandomStop) {
		outStringPtrs[0] = "Chance (%age)";
		return 1;

	} else if (move_index == AI2cMeleeManeuver_BarabbasWave) {
		outStringPtrs[0] = "Max Range";
		return 1;
	}

	maneuver_def = AI2gMeleeManeuverDefinition + move_index;

	// store the number of parameters, and their names
	if (inSpacing) {
		num_params = maneuver_def->num_spacing_params;
		param_names = maneuver_def->spacing_param_names;
	} else {
		num_params = maneuver_def->num_normal_params;
		param_names = maneuver_def->normal_param_names;
	}
	UUmAssert(num_params <= 2);

	outStringPtrs[0] = "Duration (sec)";
	for (itr = 0; itr < num_params; itr++) {
		outStringPtrs[itr + 1] = param_names[itr];
	}

	return num_params + 1;
}

static UUtUns16 AI2iMelee_Maneuver_GetAnimType(UUtUns32 inMove, TRtDirection *outDirection,
											AI2tMeleeMoveState *outFromState, AI2tMeleeMoveState *outToState,
											UUtUns16 *outTargetAnimState, UUtUns16 *outTargetAnimVarient)
{
	UUtUns32 move_index;
	AI2tMeleeManeuverDefinition *maneuver_def;

	if ((inMove & AI2cMoveType_Mask) != AI2cMoveType_Maneuver) {
		// not a maneuver!
		return UUcFalse;
	}

	move_index = (inMove & ~AI2cMoveType_Mask);
	if ((move_index < 0) || (move_index >= AI2cMeleeManeuver_Max)) {
		// invalid move index
		return 0;
	}

	maneuver_def = AI2gMeleeManeuverDefinition + move_index;

	*outDirection = TRcDirection_360;
	*outFromState = maneuver_def->from_state;
	*outToState = maneuver_def->to_state;
	*outTargetAnimState = ONcAnimState_None;
	*outTargetAnimVarient = 0;
	return maneuver_def->anim_type;
}

static UUtBool AI2iMelee_Maneuver_CanFollow(UUtUns32 inMove, UUtUns32 inPrevMove)
{
	// maneuvers can follow any move
	return UUcTrue;
}

static UUtBool AI2iMelee_Maneuver_Start(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove)
{
	UUtUns32 move_index;
	float random_val;
	AI2tMeleeManeuverDefinition *maneuver_def;

	if ((inMove->move & AI2cMoveType_Mask) != AI2cMoveType_Maneuver) {
		// not a maneuver!
		return UUcFalse;
	}

	move_index = (inMove->move & ~AI2cMoveType_Mask);
	if ((move_index < 0) || (move_index >= AI2cMeleeManeuver_Max)) {
		// invalid move index
		return 0;
	}

	if ((AI2gUltraMode || (ioCharacter->flags2 & ONcCharacterFlag2_UltraMode)) && (move_index == AI2cMeleeManeuver_Pause)) {
		// don't run this move, just continue
		return AI2iMelee_StartNextMove(ioCharacter, ioMeleeState, inMove);
	}

	if (move_index == AI2cMeleeManeuver_RandomStop) {
		// calculate whether or not we should abort this technique
		random_val = ((float) UUrRandom()) / UUcMaxUns16;
		if (random_val * 100.0f < inMove->param[0]) {
			// stop this technique
			AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
			return UUcTrue;
		} else {
			// continue as normal
			return AI2iMelee_StartNextMove(ioCharacter, ioMeleeState, inMove);
		}
	}

	maneuver_def = AI2gMeleeManeuverDefinition + move_index;
	if (maneuver_def->is_maneuver) {
		// we are moving - we no longer lock down our facing
#if DEBUG_VERBOSE_MANEUVER
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "starting new maneuver, clearing lock-facing");
#endif
		ioMeleeState->lock_facing_for_technique = UUcFalse;
	}

	if (move_index == AI2cMeleeManeuver_BarabbasWave) {
		// special-case maneuver
		ioMeleeState->attack_launched = UUcFalse;
	}

	// haven't started our timer yet
	ioMeleeState->maneuver_started = UUcFalse;

	return AI2iMelee_Maneuver_Update(ioCharacter, ioMeleeState, inMove);
}


static UUtBool AI2iMelee_Maneuver_Update(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove)
{
	UUtBool localmove_success, unable_to_start;
	UUtUns32 current_time = ONrGameState_GetGameTime(), move_index;
	AI2tMeleeManeuverDefinition *maneuver_def;
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
	float dist_threshold;
	TRtAnimState next_animstate;
	UUtUns32 tolerance_frames, frames_until_nextanim;
	AI2tMeleeMoveState cur_state, next_state;
	const TRtAnimation *test_animation;

	// get the definition for this maneuver
	if ((inMove->move & AI2cMoveType_Mask) != AI2cMoveType_Maneuver) {
		// not a maneuver!
		UUmAssert(0);
#if (DEBUG_VERBOSE_MANEUVER || DEBUG_VERBOSE_TECHNIQUE)
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: maneuver is not valid (mask type), abort.", ioCharacter->player_name);
#endif
		AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
		return UUcTrue;
	}

	move_index = (inMove->move & ~AI2cMoveType_Mask);
	if ((move_index < 0) || (move_index >= AI2cMeleeManeuver_Max)) {
		// invalid move index
		UUmAssert(0);
#if (DEBUG_VERBOSE_MANEUVER || DEBUG_VERBOSE_TECHNIQUE)
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: maneuver is not valid (index), abort.", ioCharacter->player_name);
#endif
		AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
		return UUcTrue;
	}
	maneuver_def = AI2gMeleeManeuverDefinition + move_index;
	UUmAssertReadPtr(active_character, sizeof(*active_character));

	if (move_index == AI2cMeleeManeuver_BarabbasWave) {
		/*
		 * this is a special-case maneuver, it's more like an attack
		 */

		// unlike most maneuvers, this one has an actual animation stored in it
		if (inMove->animation == NULL) {
#if (DEBUG_VERBOSE_MANEUVER || DEBUG_VERBOSE_TECHNIQUE)
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: barabbas-wave has no animation, abort.", ioCharacter->player_name);
#endif
			AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
			return UUcTrue;
		}

		if (ioMeleeState->attack_launched) {
			// we have launched our attack
			if (AI2rExecutor_HasAttackOverride(ioCharacter, NULL)) {
				// just wait until it is realised (or discarded)
				return UUcFalse;
			}
			
			// we have actually performed this technique, we can't again for a while
			AI2iMeleeState_ApplyTechniqueDelay(ioCharacter, ioMeleeState);

			if (ioMeleeState->running_active_animation) {
				// don't go to the next move yet
				return UUcFalse;
			} else {
				// we're done.
				return AI2iMelee_StartNextMove(ioCharacter, ioMeleeState, inMove);
			}
		}

		// if our current animation is atomic, or we have to wait until it's finished, don't interrupt it
		next_animstate = TRrAnimation_GetTo(active_character->animation);
		next_state = AI2iMelee_TranslateTotoroAnimState(ioCharacter, next_animstate);
		cur_state = AI2iMelee_TranslateTotoroAnimState(ioCharacter, active_character->curFromState);

		if ((TRrAnimation_IsAtomic(active_character->animation, active_character->animFrame)) ||
			((cur_state != inMove->current_state) && (next_state == inMove->current_state))) {

			// work out how many frames before this animation ends we can queue up our
			// desired attack
			tolerance_frames = 0;
			test_animation = TRrCollection_Lookup(ioCharacter->characterClass->animations,
												TRrAnimation_GetType(inMove->animation),
												next_animstate, active_character->animVarient | ONcAnimVarientMask_Fight);
			if (test_animation) {
				tolerance_frames = AI2cMeleeAttackQueue_HardFrames;
			} else if (TRrAnimation_IsShortcut(inMove->animation, next_animstate)) {
				tolerance_frames = AI2cMeleeAttackQueue_SoftFrames;
			}

			frames_until_nextanim = TRrAnimation_GetDuration(active_character->animation) - active_character->animFrame;
			if (frames_until_nextanim > tolerance_frames) {
				// we can't queue up our desired attack yet.
				// delay, and press keys as appropriate from last positioning info
				if ((ioMeleeState->attack_waspositioned) && (ioMeleeState->position_lastkeys != 0)) {
					AI2rExecutor_MoveOverride(ioCharacter, ioMeleeState->position_lastkeys);
				}
				return UUcFalse;
			}
		}

		AI2rExecutor_AttackOverride(ioCharacter, maneuver_def->nonmaneuver_keys, maneuver_def->nonmaneuver_keys, maneuver_def->anim_type, ONcAnimType_None);
		ioMeleeState->attack_launched = UUcTrue;

		return UUcFalse;
	}

	if (!ioMeleeState->maneuver_started) {
		// we haven't yet started our maneuver timer!
		unable_to_start = ((TRrAnimation_IsAttack(active_character->animation)) ||
				(AI2rExecutor_HasAttackOverride(ioCharacter, NULL)) || (ONrCharacter_InAir(ioCharacter)));
		if (unable_to_start) {
			// delay!
#if DEBUG_VERBOSE_MANEUVER
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "delaying start of maneuver");
#endif
			return UUcFalse;
		}

		// we can start our maneuver timer now.
		ioMeleeState->maneuver_endtime = ONrGameState_GetGameTime() +
			MUrUnsignedSmallFloat_To_Uns_Round(inMove->param[0] * UUcFramesPerSecond);
#if DEBUG_VERBOSE_MANEUVER
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "maneuver timer set to %d (duration %f)", ioMeleeState->maneuver_endtime, inMove->param[0]);
#endif
		ioMeleeState->maneuver_started = UUcTrue;
	}
	
	if (current_time > ioMeleeState->maneuver_endtime) {
#if DEBUG_VERBOSE_MANEUVER
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "maneuver timer expired (%d > %d)", current_time, ioMeleeState->maneuver_endtime);
#endif
		// finish this move
		return AI2iMelee_StartNextMove(ioCharacter, ioMeleeState, inMove);
	}

	if (maneuver_def->is_maneuver) {
		// check to make sure that we still meet the parameters for the maneuver
		switch(move_index) {
			case AI2cMeleeManeuver_Advance:
				dist_threshold = UUmMax(inMove->param[1] - inMove->param[2], AI2cMelee_MinRange);
				if (ioMeleeState->distance_to_target < dist_threshold) {
#if DEBUG_VERBOSE_MANEUVER
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "advance maneuver too close (dist %f, min %f thresh %f)",
						ioMeleeState->distance_to_target, inMove->param[1], inMove->param[2]);
#endif
					// we're close enough to the target, stop
					return AI2iMelee_StartNextMove(ioCharacter, ioMeleeState, inMove);
				}
				break;

			case AI2cMeleeManeuver_Retreat:
				dist_threshold = UUmMax(inMove->param[1] + inMove->param[2], AI2cMelee_MaxRange - 5.0f);
				if (ioMeleeState->distance_to_target > dist_threshold) {
#if DEBUG_VERBOSE_MANEUVER
					COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "retreat maneuver too far (dist %f, max %f thresh %f)",
						ioMeleeState->distance_to_target, inMove->param[1], inMove->param[2]);
#endif
					// we're far enough away from the target, stop
					return AI2iMelee_StartNextMove(ioCharacter, ioMeleeState, inMove);
				}
				break;
		}

		UUmAssert(!ioMeleeState->lock_facing_for_technique);
		localmove_success = AI2iMelee_AddLocalMovement(ioCharacter, ioMeleeState, 
														maneuver_def->maneuver_direction, UUcTrue);
		if (!localmove_success) {
#if DEBUG_VERBOSE_MANEUVER
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "local movement failed");
#endif
			// we can't move in the direction that we want to. give up and abort.
#if (DEBUG_VERBOSE_MANEUVER || DEBUG_VERBOSE_TECHNIQUE)
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: maneuver local movement failed, abort.", ioCharacter->player_name);
#endif
			AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
			return UUcTrue;
		}
#if DEBUG_VERBOSE_MANEUVER
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "succeeded with local movement in direction %d", maneuver_def->maneuver_direction);
#endif
	} else {
		// keep holding down the keys that we should be holding down
		AI2rExecutor_MoveOverride(ioCharacter, maneuver_def->nonmaneuver_keys);
#if DEBUG_VERBOSE_MANEUVER
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "holding down nonmaneuver keys");
#endif
	}

	return UUcFalse;
}


static void AI2iMelee_Maneuver_Finish(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove)
{
	// we have actually performed this technique, we can't again for a while
	AI2iMeleeState_ApplyTechniqueDelay(ioCharacter, ioMeleeState);

#if DEBUG_VERBOSE_MANEUVER
	COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "finishing maneuver");
#endif
	// if we had a movement modifier applied, halt it... otherwise nothing else to do
	AI2rPath_Halt(ioCharacter);
}

static UUtBool AI2iMelee_Maneuver_FaceTarget(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
										  AI2tMeleeMove *inMove)
{
	return UUcFalse;
}

static UUtBool AI2iMelee_Maneuver_CanEvade(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
										  AI2tMeleeMove *inMove)
{
	return UUcTrue;
}


// ------------------------------------------------------------------------------------
// -- evasion functions

static UUtUns32 AI2iMelee_Evade_Iterate(UUtUns32 inIterator)
{
	UUtUns32 move_index;

	if (inIterator == (UUtUns32) -1) {
		move_index = 0;
	} else {
		UUmAssert((inIterator & AI2cMoveType_Mask) == AI2cMoveType_Evade);

		move_index = (inIterator & ~AI2cMoveType_Mask);
		UUmAssert((move_index >= 0) && (move_index < AI2gNumMeleeEvasions));

		move_index++;
	}

	if (move_index >= AI2gNumMeleeEvasions) {
		// no more moves in table
		return (UUtUns32) -1;
	} else {
		// this move is okay!
		return (AI2cMoveType_Evade | move_index);
	}
}

static UUtBool AI2iMelee_Evade_GetName(UUtUns32 inMove, char *outString, UUtUns32 inStringLength)
{
	UUtUns32 move_index;

	if ((inMove & AI2cMoveType_Mask) != AI2cMoveType_Evade) {
		// not an attack move!
		return UUcFalse;
	}

	move_index = (inMove & ~AI2cMoveType_Mask);
	if ((move_index >= 0) && (move_index < AI2gNumMeleeEvasions)) {
		// store the name string
		UUrString_Copy(outString, AI2gMeleeEvadeTable[move_index].name, inStringLength);
		return UUcTrue;
	} else {
		// invalid move index
		return UUcFalse;
	}
}

static UUtUns32 AI2iMelee_Evade_GetParams(UUtUns32 inMove, UUtBool inSpacing, char *outStringPtrs[3])
{
	// evasion moves have no parameters
	return 0;
}

static UUtUns16 AI2iMelee_Evade_GetAnimType(UUtUns32 inMove, TRtDirection *outDirection,
											AI2tMeleeMoveState *outFromState, AI2tMeleeMoveState *outToState,
											UUtUns16 *outTargetAnimState, UUtUns16 *outTargetAnimVarient)
{
	UUtUns32 move_index, flags;

	UUmAssert((inMove & AI2cMoveType_Mask) == AI2cMoveType_Evade);

	move_index = (inMove & ~AI2cMoveType_Mask);
	UUmAssert((move_index >= 0) && (move_index < AI2gNumMeleeEvasions));

	// set required data for move sequencing and animation lookup
	flags = AI2gMeleeEvadeTable[move_index].evademove_flags;

	*outFromState	= AI2mMelee_EvadeFromState(flags);
	*outToState		= AI2mMelee_EvadeToState(flags);
	*outDirection	= TRcDirection_None;
	*outTargetAnimState = ONcAnimState_None;
	*outTargetAnimVarient = 0;

	return (UUtUns16) (AI2mMelee_EvadeAnimType(flags));
}

static UUtBool AI2iMelee_Evade_CanFollow(UUtUns32 inMove, UUtUns32 inPrevMove)
{
	// evasion moves cannot follow anything
	return (inPrevMove == (UUtUns32) -1);
}

static UUtBool AI2iMelee_Evade_Start(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove)
{
	ioMeleeState->evasion_launched = UUcFalse;
	ioMeleeState->evasion_endtime = 0;

	return AI2iMelee_Evade_Update(ioCharacter, ioMeleeState, inMove);
}


static UUtBool AI2iMelee_Evade_Update(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove)
{
	const TRtAnimation *test_animation;
	TRtAnimState next_animstate;
	UUtUns32 tolerance_frames, frames_until_nextanim;
	AI2tMelee_EvadeMove *evade_move;
	UUtUns32 move_index, current_time;
	UUtUns16 attack_type;
	AI2tMeleeMoveState cur_state, next_state;
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter), *target_active = ONrGetActiveCharacter(ioMeleeState->target);

	UUmAssertReadPtr(active_character, sizeof(*active_character));
	UUmAssertReadPtr(target_active, sizeof(*target_active));

	current_time = ONrGameState_GetGameTime();
	if (ioMeleeState->evasion_endtime > 0) {
		if (current_time < ioMeleeState->evasion_endtime) {
			// we are playing our evasion animation, just delay
			return UUcFalse;
		} else {
			// we are done!
			return AI2iMelee_StartNextMove(ioCharacter, ioMeleeState, inMove);
		}
	}

	if (ioMeleeState->current_technique->computed_flags & AI2cTechniqueComputedFlag_IsSpacing) {
		// spacing behaviors can have evade moves without an attack that's being reacted to
	} else {
		if ((ioMeleeState->target->animCounter != ioMeleeState->current_react_anim_index) ||
			(target_active->animFrame > TRrAnimation_GetLastAttackFrame(target_active->animation))) {
			// the target is no longer executing the attack that we are evading. stop evading.
			// FIXME: add delay here to make AI easier?

			if (ioMeleeState->evasion_launched) {
				// take away the attack override that's pending
				AI2rExecutor_ClearAttackOverride(ioCharacter);
			}

			return AI2iMelee_StartNextMove(ioCharacter, ioMeleeState, inMove);
		}
	}

	if (ioMeleeState->evasion_launched) {
		// we have launched our attack
		if (AI2rExecutor_HasAttackOverride(ioCharacter, NULL)) {
			// just wait until it is realised (or discarded)
			return UUcFalse;
		} else {
			// we have launched an evasion; we cannot change our facing for this technique any longer.
			ioMeleeState->lock_facing_for_technique = UUcTrue;
			ioMeleeState->technique_face_direction = ioCharacter->facing;

			// set up the end time for our evasion, and delay until then
			UUmAssert(inMove->animation != NULL);
			ioMeleeState->evasion_endtime = current_time + TRrAnimation_GetDuration(inMove->animation);

			return UUcFalse;
		}
	}

	UUmAssert((inMove->move & AI2cMoveType_Mask) == AI2cMoveType_Evade);

	move_index = inMove->move & ~AI2cMoveType_Mask;
	UUmAssert((move_index >= 0) && (move_index < AI2gNumMeleeEvasions));

	evade_move = &AI2gMeleeEvadeTable[move_index];


	if (inMove->is_broken) {
		// this is a broken move! don't even bother
		AI2_ERROR(AI2cBug, AI2cSubsystem_Melee, AI2cError_Melee_BrokenMove, ioCharacter,
			ioMeleeState, ioMeleeState->current_technique, inMove, 0);
		return UUcTrue;
	}
	UUmAssert(inMove->animation != NULL);

	if (AI2rExecutor_HasAttackOverride(ioCharacter, &attack_type)) {
		// we already have an attack trying to launch. don't overwrite it.
#if DEBUG_VERBOSE_EVADE
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: evasion 0x%08X: delaying, already launching attack (animtype %s)",
						ioCharacter->player_name, inMove->move, ONrAnimTypeToString(attack_type));
#endif
		// delay, and leave keys alone (because attack_override should be doing them)
		return UUcFalse;

	}

	next_animstate = TRrAnimation_GetTo(active_character->animation);
	next_state = AI2iMelee_TranslateTotoroAnimState(ioCharacter, next_animstate);
	cur_state = AI2iMelee_TranslateTotoroAnimState(ioCharacter, active_character->curFromState);

	// if our current animation is atomic, or we have to wait until it's finished, don't interrupt it
	if ((TRrAnimation_IsAtomic(active_character->animation, active_character->animFrame)) ||
		((cur_state != inMove->current_state) && (next_state == inMove->current_state))) {

		// work out how many frames before this animation ends we can queue up the evasion
		tolerance_frames = 0;
		test_animation = TRrCollection_Lookup(ioCharacter->characterClass->animations,
											TRrAnimation_GetType(inMove->animation),
											next_animstate, active_character->animVarient | ONcAnimVarientMask_Fight);
		if (test_animation) {
			tolerance_frames = AI2cMeleeAttackQueue_HardFrames;
#if DEBUG_VERBOSE_EVADE
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: evasion 0x%08X: animation %s (type %s) direct lookup! (current tostate %s, fromstate %s)",
							ioCharacter->player_name, inMove->move, TMrInstance_GetInstanceName(inMove->animation),
							ONrAnimTypeToString(TRrAnimation_GetType(inMove->animation)), ONrAnimStateToString(next_animstate), 
							ONrAnimStateToString(TRrAnimation_GetFrom(inMove->animation)));
#endif
		} else if (TRrAnimation_IsShortcut(inMove->animation, next_animstate)) {
			tolerance_frames = AI2cMeleeAttackQueue_SoftFrames;
#if DEBUG_VERBOSE_EVADE
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: evasion 0x%08X: animation %s (type %s) shortcut lookup! (current tostate %s, fromstate %s)",
							ioCharacter->player_name, inMove->move, TMrInstance_GetInstanceName(inMove->animation),
							ONrAnimTypeToString(TRrAnimation_GetType(inMove->animation)), ONrAnimStateToString(next_animstate), 
							ONrAnimStateToString(TRrAnimation_GetFrom(inMove->animation)));
#endif
		}

		frames_until_nextanim = TRrAnimation_GetDuration(active_character->animation) - active_character->animFrame;
		if (frames_until_nextanim > tolerance_frames) {
#if DEBUG_VERBOSE_EVADE
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: evasion 0x%08X: frames until nextanim %d > queue tolerance %d (current %s -> %s), delaying attack (%s)",
							ioCharacter->player_name, inMove->move, frames_until_nextanim, tolerance_frames,
							TMrInstance_GetInstanceName(active_character->animation), ONrAnimStateToString(next_animstate),
							TMrInstance_GetInstanceName(inMove->animation));
#endif
			// we can't queue up our desired evasion yet. delay.
			return UUcFalse;
		} else {
#if DEBUG_VERBOSE_EVADE
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: evasion 0x%08X: queueing up move %s (current %s -> %s)",
							ioCharacter->player_name, inMove->move, TMrInstance_GetInstanceName(inMove->animation),
							TMrInstance_GetInstanceName(active_character->animation), ONrAnimStateToString(next_animstate));
#endif
		}
	} else {
#if DEBUG_VERBOSE_EVADE
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: evasion 0x%08X: interrupting non-atomic animation (%s)",
						ioCharacter->player_name, inMove->move, TMrInstance_GetInstanceName(active_character->animation));
#endif
	}

	// queue up the evasion move.
#if DEBUG_VERBOSE_EVADE
	COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: evasion 0x%08X: executing '%s' - wait-for-state %s",
					ioCharacter->player_name, inMove->move, evade_move->name, 
					ONrAnimTypeToString(AI2mMelee_EvadeAnimType(evade_move->evademove_flags)));
#endif
	AI2rExecutor_AttackOverride(ioCharacter, evade_move->keys_isdown, evade_move->keys_wentdown,
								(UUtUns16) (AI2mMelee_EvadeAnimType(evade_move->evademove_flags)), ONcAnimType_None);

	// we have actually performed this reaction, we can't again for a while
	AI2iMeleeState_ApplyTechniqueDelay(ioCharacter, ioMeleeState);
	ioMeleeState->evasion_launched = UUcTrue;
	return UUcFalse;
}


static void AI2iMelee_Evade_Finish(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove)
{
}

static UUtBool AI2iMelee_Evade_FaceTarget(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
										  AI2tMeleeMove *inMove)
{
	return UUcTrue;
}

static UUtBool AI2iMelee_Evade_CanEvade(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
										  AI2tMeleeMove *inMove)
{
	return UUcFalse;
}

// ------------------------------------------------------------------------------------
// -- throw functions

static UUtUns32 AI2iMelee_Throw_Iterate(UUtUns32 inIterator)
{
	UUtUns32 move_index;

	if (inIterator == (UUtUns32) -1) {
		move_index = 0;
	} else {
		UUmAssert((inIterator & AI2cMoveType_Mask) == AI2cMoveType_Throw);

		move_index = (inIterator & ~AI2cMoveType_Mask);
		UUmAssert((move_index >= 0) && (move_index < AI2cNumMeleeThrows));

		move_index++;
	}

	if (move_index >= AI2cNumMeleeThrows) {
		// no more moves in table
		return (UUtUns32) -1;
	} else {
		// this move is okay!
		return (AI2cMoveType_Throw | move_index);
	}
}

static UUtBool AI2iMelee_Throw_GetName(UUtUns32 inMove, char *outString, UUtUns32 inStringLength)
{
	UUtUns32 move_index;

	if ((inMove & AI2cMoveType_Mask) != AI2cMoveType_Throw) {
		// not an attack move!
		return UUcFalse;
	}

	move_index = (inMove & ~AI2cMoveType_Mask);
	if ((move_index >= 0) && (move_index < AI2cNumMeleeThrows)) {
		// store the name string
		UUrString_Copy(outString, AI2gMeleeThrowTable[move_index].name, inStringLength);
		return UUcTrue;
	} else {
		// invalid move index
		return UUcFalse;
	}
}

static UUtUns32 AI2iMelee_Throw_GetParams(UUtUns32 inMove, UUtBool inSpacing, char *outStringPtrs[3])
{
	// throw moves have no parameters
	return 0;
}

static UUtUns16 AI2iMelee_Throw_GetAnimType(UUtUns32 inMove, TRtDirection *outDirection,
											AI2tMeleeMoveState *outFromState, AI2tMeleeMoveState *outToState,
											UUtUns16 *outTargetAnimState, UUtUns16 *outTargetAnimVarient)
{
	UUtUns32 move_index, flags;

	UUmAssert((inMove & AI2cMoveType_Mask) == AI2cMoveType_Throw);

	move_index = (inMove & ~AI2cMoveType_Mask);
	UUmAssert((move_index >= 0) && (move_index < AI2cNumMeleeThrows));

	// set required data for move sequencing and animation lookup
	flags = AI2gMeleeThrowTable[move_index].throwmove_flags;

	// set our from and to states
	if (flags & AI2cMelee_Throw_FromRunning) {
		*outFromState = AI2cMeleeMoveState_Running;
	} else {
		*outFromState = AI2cMeleeMoveState_Standing;
	}
	*outToState = AI2cMeleeMoveState_Standing;
	*outDirection	= TRcDirection_Forwards;

	// set the victim's from state
	if (flags & AI2cMelee_Throw_VictimRunning) {
		*outTargetAnimState = ONcAnimState_Running_Right_Down;
	} else {
		*outTargetAnimState = ONcAnimState_Standing;
	}

	if (flags & AI2cMelee_Throw_DisarmPistol) {
		*outTargetAnimVarient = ONcAnimVarientMask_Righty_Pistol;

	} else if (flags & AI2cMelee_Throw_DisarmRifle) {
		*outTargetAnimVarient = ONcAnimVarientMask_Righty_Rifle;

	} else {
		*outTargetAnimVarient = 0;
	}

	return (UUtUns16) (AI2mMelee_ThrowAnimType(flags));
}

static UUtBool AI2iMelee_Throw_CanFollow(UUtUns32 inMove, UUtUns32 inPrevMove)
{
	// throw moves can go anywhere
	return UUcTrue;
}

static UUtBool AI2iMelee_Throw_Start(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove)
{
	ioMeleeState->attack_launched = UUcFalse;

	return AI2iMelee_Throw_Update(ioCharacter, ioMeleeState, inMove);
}


static UUtBool AI2iMelee_Throw_Update(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove)
{
	TRtAnimation *throw_animation, *throwtarget_animation;
	AI2tMelee_ThrowMove *throw_move;
	UUtUns32 move_index;
	float throw_dist, target_rel_dir;
	UUtBool can_throw, should_abort, throw_from_front, currently_behind_target, currently_in_front_of_target;
	TRtAnimVarient desired_varient;
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter), *active_target;

	UUmAssertReadPtr(active_character, sizeof(*active_character));

	if (ioMeleeState->attack_launched) {
		// we have launched our throw
		if (AI2rExecutor_HasAttackOverride(ioCharacter, NULL)) {
			// just wait until it is realised (or discarded)
			return UUcFalse;
		} else {
			// we're done.
			return AI2iMelee_StartNextMove(ioCharacter, ioMeleeState, inMove);
		}
	}

	UUmAssert((inMove->move & AI2cMoveType_Mask) == AI2cMoveType_Throw);
	move_index = inMove->move & ~AI2cMoveType_Mask;
	UUmAssert((move_index >= 0) && (move_index < AI2cNumMeleeThrows));

	throw_move = &AI2gMeleeThrowTable[move_index];

	if (inMove->is_broken) {
		// this is a broken move! don't even bother
		AI2_ERROR(AI2cBug, AI2cSubsystem_Melee, AI2cError_Melee_BrokenMove, ioCharacter,
			ioMeleeState, ioMeleeState->current_technique, inMove, 0);
		return UUcTrue;
	}
	UUmAssert(inMove->animation != NULL);


	if (TRrAnimation_TestFlag(active_character->animation, ONcAnimFlag_ThrowSource)) {
		// we are currently executing our throw, wait until it is finished
		return UUcFalse;
	}

	if (ioMeleeState->facing_delta > AI2cMelee_ThrowMaxDeltaFacing) {
		// we must be facing the target in order to throw them
#if DEBUG_VERBOSE_THROW || DEBUG_VERBOSE_TECHNIQUE
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: facing in totally wrong direction (%f > %f), abort throw",
				ioCharacter->player_name, ioMeleeState->facing_delta, AI2cMelee_ThrowMaxDeltaFacing);
#endif
		AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
		return UUcTrue;
	}

	throw_dist = TRrAnimation_GetThrowDistance(inMove->animation);
	if (ioMeleeState->distance_to_target > throw_dist) {
		// we are too far away to launch this throw
#if DEBUG_VERBOSE_THROW || DEBUG_VERBOSE_TECHNIQUE
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: too far away to throw (%f > %f), abort throw",
				ioCharacter->player_name, ioMeleeState->distance_to_target, throw_dist);
#endif
		AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
		return UUcTrue;
	}

	// check that our target still has the right animation varient
	active_target = ONrForceActiveCharacter(ioMeleeState->target);
	if (active_target == NULL) {
		UUmAssert(0);
		AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
		return UUcTrue;
	}

	// check that our target isn't invulnerable
	if (!AI2iMelee_TargetIsThrowable(ioMeleeState->target, active_target)) {
		AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
		return UUcTrue;
	}
	
	if (throw_move->throwmove_flags & AI2cMelee_Throw_DisarmPistol) {
		// the target must still have their pistol out
		desired_varient = ONcAnimVarientMask_Righty_Pistol;
		if ((active_target->animVarient & ONcAnimVarientMask_Any_Pistol) == 0) {
#if DEBUG_VERBOSE_THROW || DEBUG_VERBOSE_TECHNIQUE
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: target %s no longer has pistol, abort disarm",
					ioCharacter->player_name, ioMeleeState->target->player_name);
#endif
			AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
			return UUcTrue;
		}

	} else if (throw_move->throwmove_flags & AI2cMelee_Throw_DisarmRifle) {
		// the target must still have their rifle out
		desired_varient = ONcAnimVarientMask_Righty_Rifle;
		if ((active_target->animVarient & ONcAnimVarientMask_Any_Rifle) == 0) {
#if DEBUG_VERBOSE_THROW || DEBUG_VERBOSE_TECHNIQUE
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: target %s no longer has rifle, abort disarm",
					ioCharacter->player_name, ioMeleeState->target->player_name);
#endif
			AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
			return UUcTrue;
		}
	} else {
		desired_varient = 0;
	}

	// check that we can actually play the throw animation from here
	can_throw = AI2iMelee_FindThrow(ioCharacter->characterClass->animations, (UUtUns16) AI2mMelee_ThrowAnimType(throw_move->throwmove_flags),
					active_character->nextAnimState, active_target->nextAnimState, desired_varient, &throw_animation, &throwtarget_animation, NULL);
	if (!can_throw) {
#if DEBUG_VERBOSE_THROW || DEBUG_VERBOSE_TECHNIQUE
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: unable to launch throw type %s from state %s (target %s state %s), abort throw",
				ioCharacter->player_name, ONrAnimTypeToString((UUtUns16) AI2mMelee_ThrowAnimType(throw_move->throwmove_flags)),
				ONrAnimStateToString(active_character->nextAnimState), ioMeleeState->target->player_name, ONrAnimStateToString(active_target->nextAnimState));
#endif
		AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
		return UUcTrue;
	}

#if DEBUG_VERBOSE_THROW
	COrConsole_Printf("%s: throwing %s (%s, %s)", ioCharacter->player_name, ioMeleeState->target->player_name,
			TMrInstance_GetInstanceName(throw_animation), TMrInstance_GetInstanceName(throwtarget_animation));
#endif

	// if our current animation is atomic, don't interrupt it
	if (TRrAnimation_IsAtomic(active_character->animation, active_character->animFrame)) {
		// wait until the animation is finished
		if (active_character->animFrame < TRrAnimation_GetDuration(active_character->animation)) {
#if DEBUG_VERBOSE_THROW
			COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: throw %s: delaying, frames until nextanim %d",
							ioCharacter->player_name, throw_move->name,
							TRrAnimation_GetDuration(active_character->animation) - active_character->animFrame);
#endif
			// delay, and press keys as appropriate from last positioning info
			if ((ioMeleeState->attack_waspositioned) && (ioMeleeState->position_lastkeys != 0)) {
				AI2rExecutor_MoveOverride(ioCharacter, ioMeleeState->position_lastkeys);
			}
			return UUcFalse;
		}
	}

	// work out whereabouts we would be approaching the target from
	target_rel_dir = ioMeleeState->target->facing - ioCharacter->facing;
	UUmTrig_ClipAbsPi(target_rel_dir);
	target_rel_dir = (float)fabs(target_rel_dir);
	currently_behind_target = (target_rel_dir < AI2cMelee_ThrowMaxDeltaFacing);
	currently_in_front_of_target = (target_rel_dir > (M3cPi - AI2cMelee_ThrowMaxDeltaFacing));

	throw_from_front = ONrAnimType_ThrowIsFromFront(TRrAnimation_GetType(throw_animation));
	if (throw_from_front && !currently_in_front_of_target) {
		should_abort = UUcTrue;
#if (DEBUG_VERBOSE_THROW || DEBUG_VERBOSE_TECHNIQUE)
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: throw animtype %s is from front yet we aren't in front (delta %f > %f)",
							ioCharacter->player_name, ONrAnimTypeToString(TRrAnimation_GetType(throw_animation)),
							M3cPi - target_rel_dir, AI2cMelee_ThrowMaxDeltaFacing);
#endif

	} else if (!throw_from_front && !currently_behind_target) {
		should_abort = UUcTrue;
#if (DEBUG_VERBOSE_THROW || DEBUG_VERBOSE_TECHNIQUE)
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: throw animtype %s is from behind yet we aren't behind (delta %f > %f)",
							ioCharacter->player_name, ONrAnimTypeToString(TRrAnimation_GetType(throw_animation)),
							target_rel_dir, AI2cMelee_ThrowMaxDeltaFacing);
#endif

	} else {
		// we are just about to launch the throw - check to see that it wouldn't send us into a danger square
		should_abort = AI2iMelee_WeightLocalMovement(ioCharacter, ioMeleeState, ioMeleeState->current_technique, inMove->attack_endpos, NULL);
#if (DEBUG_VERBOSE_THROW || DEBUG_VERBOSE_TECHNIQUE)
		COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: throw source distance %f sends us into danger, abort",
							ioCharacter->player_name, inMove->attack_endpos);
#endif
	}	

	if (should_abort) {
		AI2iMeleeState_AbortTechnique(ioCharacter, ioMeleeState);
		return UUcTrue;
	}

	// we have actually performed this technique, we can't again for a while
	AI2iMeleeState_ApplyTechniqueDelay(ioCharacter, ioMeleeState);

	// queue up our throw
#if (DEBUG_VERBOSE_ATTACK || DEBUG_VERBOSE_THROW)
	COrConsole_Printf_Color(UUcTrue, 0xFFFF9090, 0xFFFF3030, "%s: executing throw %s (%s/%s)", ioCharacter->player_name, throw_move->name,
			TMrInstance_GetInstanceName(throw_animation), TMrInstance_GetInstanceName(throwtarget_animation));
#endif
	AI2rExecutor_ThrowOverride(ioCharacter, ioMeleeState->target, throw_animation, throwtarget_animation);
	ioMeleeState->attack_launched = UUcTrue;
	return UUcFalse;
}


static void AI2iMelee_Throw_Finish(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
									  AI2tMeleeMove *inMove)
{
	if ((ioMeleeState->current_technique->user_flags & AI2cTechniqueUserFlag_Interruptable) == 0) {
		// this technique is uninterruptable; we can't stop it now
		ioMeleeState->committed_to_technique = UUcTrue;
	}
}

static UUtBool AI2iMelee_Throw_FaceTarget(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
										  AI2tMeleeMove *inMove)
{
	// we must turn to face target in order for the throw to succeed
	return UUcTrue;
}

static UUtBool AI2iMelee_Throw_CanEvade(ONtCharacter *ioCharacter, AI2tMeleeState *ioMeleeState,
										  AI2tMeleeMove *inMove)
{
	// the throw is instantaneous, no opportunity to evade
	return UUcFalse;
}

