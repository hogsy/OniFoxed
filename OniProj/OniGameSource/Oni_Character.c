/*
	FILE:	Oni_Character.c

	AUTHOR:	Michael Evans

	CREATED: April 2, 1997

	PURPOSE: control of characters in ONI

	Copyright 1997-2000

*/

#include "bfw_math.h"

#include "Oni.h"
#include "Oni_Character.h"
#include "Oni_GameStatePrivate.h"
#include "Oni_AI.h"
#include "Oni_Path.h"
#include "Oni_Aiming.h"
#include "Oni_AI_Setup.h"
#include "Oni_Character_Animation.h"
#include "Oni_KeyBindings.h"
#include "Oni_Weapon.h"
#include "Oni_Particle3.h"
#include "Oni_Sound_Animation.h"
#include "Oni_ImpactEffect.h"
#include "Oni_InGameUI.h"
#include "Oni_FX.h"
#include "Oni_Speech.h"
#include "Oni_Motoko.h"
#include "Oni_GameSettings.h"
#include "Oni_Persistance.h"
#include "Oni_Script.h"

#include "BFW_Akira.h"
#include "BFW_Motoko.h"
#include "BFW_Totoro.h"
#include "BFW_MathLib.h"
#include "BFW_TemplateManager.h"
#include "BFW_Console.h"
#include "BFW_AppUtilities.h"
#include "BFW_Collision.h"
#include "BFW_BitVector.h"
#include "BFW_TextSystem.h"
#include "BFW_Totoro_Private.h"
#include "BFW_ScriptLang.h"
#include "BFW_Timer.h"

#include "EulerAngles.h"

#define ONcInitialFallingVelocity				0.5f
#define ONcMaxRampVelocity						4.0f
#define ONcMaxFramesInAir						(60 * 5)
#define ONcCharacterSnapHeight					12.0f
#define ONcConsoleAction_AbortTimer				180
#define ONcCharacter_TauntEnable_MeleeDamage	(5 * UUcFramesPerSecond)
#define ONcCharacter_TauntEnable_Close			(3 * UUcFramesPerSecond)
#define ONcCharacter_TauntEnable_CloseDist		200.0f
#define ONcDamage_HurtHypoScale					2.5f
#define ONcPoisonForgetFrames					30
#define ONcPain_DelayFrames						7
#define ONcSuperAmount_IncreasePerFrame			0.03f
#define ONcSuperAmount_DecreasePerFrame			0.06f

#define DEBUG_ATTACKEXTENT			0
#define DEBUG_CHARACTER_CREATION	0

// local enum used for ONiCharacter_GetImpactModifier
enum {
	ONcAttack_Hit,
	ONcAttack_Blocked,
	ONcAttack_Invulnerable,
	ONcAttack_Killed
};

const char *ONcPainTypeName[ONcPain_Max] = {"light", "medium", "heavy", "death"};

extern UUtInt32 PHgGridVisible;
extern UUtUns16 AIgShowDebug;
extern UUtBool ONgDisplayBNV;

static void ONrCorpse_Create(ONtCharacter *inCharacter);
static void ONrCorpse_Display(M3tPoint3D *inCameraLocation);
static void  ONrCorpse_LevelBegin(void);

static void inverse_kinematics_adjust_matrices(
	M3tPoint3D *hand_position,
	M3tVector3D *hand_forward,
	M3tVector3D *hand_up,
	M3tMatrix4x3 *shoulder_matrix,
	M3tMatrix4x3 *elbow_matrix,
	M3tMatrix4x3 *hand_matrix);
static void ONrCharacter_Die_1_Animating_Hook(ONtCharacter *ioCharacter, ONtCharacter *inKiller);
static void ONrCharacter_Die_2_Moving_Hook(ONtCharacter *ioCharacter);
static void ONrCharacter_Die_3_Cosmetic_Hook(ONtCharacter *ioCharacter);
static void ONrCharacter_Die_4_Gone_Hook(ONtCharacter *ioCharacter);
static void ONrCharacter_TotallyDead_Hook(ONtCharacter *ioCharacter);
static void ONrCharacter_Live_Hook(ONtCharacter *ioCharacter);
static void ONiCharacter_DoAiming_Vector(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);
static void ONiCharacter_DoAiming_Location(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);
static void ONrCharacter_SetMovementThisFrame(ONtCharacter *inCharacter, ONtActiveCharacter *inActiveCharacter, M3tVector3D *inVector);

static float ONiCharacter_GetAttackPotency(ONtCharacter *ioCharacter, UUtBool *outOmnipotent);
static IMtShade ONiCharacter_GetAnimParticleTint(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);
static P3tParticle *ONiCharacter_AttachParticle(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter,
												P3tParticleClass *inParticleClass, UUtUns32 inPartIndex);

static void ONiCharacter_BuildTriggerSphere(ONtActiveCharacter *ioCharacter);

typedef enum ONtFireMode
{
	ONcFireMode_Single,
	ONcFireMode_Autofire,
	ONcFireMode_Alternate,
	ONcFireMode_Continuous
} ONtFireMode;

const ONtCharacterAnimationImpactType ONcAnimImpactTypes[] = {
	{"walk",			ONcAnimationImpact_Walk},
	{"run",				ONcAnimationImpact_Run},
	{"crouch",			ONcAnimationImpact_Crouch},
	{"slide",			ONcAnimationImpact_Slide},
	{"land",			ONcAnimationImpact_Land},
	{"landHard",		ONcAnimationImpact_LandHard},
	{"landDead",		ONcAnimationImpact_LandDead},
	{"knockdown",		ONcAnimationImpact_Knockdown},
	{"thrown",			ONcAnimationImpact_Thrown},
	{"standingTurn",	ONcAnimationImpact_StandingTurn},
	{"runStart",		ONcAnimationImpact_RunStart},
	{"singleStep",		ONcAnimationImpact_1Step},
	{"runStop",			ONcAnimationImpact_RunStop},
	{"walkStop",		ONcAnimationImpact_WalkStop},
	{"sprint",			ONcAnimationImpact_Sprint},
	{NULL,				ONcAnimationImpact_None}
};

const UUtUns32 ONcBodySurfacePartMasks[ONcNumCharacterParts] = {
	0							| ONcBodySurfaceFlag_Both_Y | ONcBodySurfaceFlag_Both_Z | ONcBodySurfaceFlag_MainAxis_X,	// pelvis
	0							| ONcBodySurfaceFlag_Both_Y | ONcBodySurfaceFlag_Both_Z | ONcBodySurfaceFlag_MainAxis_X,	// lleg
	0							| ONcBodySurfaceFlag_Both_Y | ONcBodySurfaceFlag_Both_Z | ONcBodySurfaceFlag_MainAxis_X,	// lleg1
	ONcBodySurfaceFlag_Pos_X	| ONcBodySurfaceFlag_Both_Y | ONcBodySurfaceFlag_Both_Z | ONcBodySurfaceFlag_MainAxis_X,	// lfoot
	0							| ONcBodySurfaceFlag_Both_Y | ONcBodySurfaceFlag_Both_Z | ONcBodySurfaceFlag_MainAxis_X,	// rleg
	0							| ONcBodySurfaceFlag_Both_Y | ONcBodySurfaceFlag_Both_Z | ONcBodySurfaceFlag_MainAxis_X,	// rleg1
	ONcBodySurfaceFlag_Pos_X	| ONcBodySurfaceFlag_Both_Y | ONcBodySurfaceFlag_Both_Z | ONcBodySurfaceFlag_MainAxis_X,	// rfoot
	0							| ONcBodySurfaceFlag_Both_Y | ONcBodySurfaceFlag_Both_Z | ONcBodySurfaceFlag_MainAxis_X,	// spine
	0							| ONcBodySurfaceFlag_Both_Y | ONcBodySurfaceFlag_Both_Z | ONcBodySurfaceFlag_MainAxis_X,	// spine1
	0																					| ONcBodySurfaceFlag_MainAxis_X,	// neck
	ONcBodySurfaceFlag_Pos_X	| ONcBodySurfaceFlag_Both_Y | ONcBodySurfaceFlag_Both_Z | ONcBodySurfaceFlag_MainAxis_X,	// head
	0																					| ONcBodySurfaceFlag_MainAxis_X,	// larm
	0							| ONcBodySurfaceFlag_Both_Y | ONcBodySurfaceFlag_Both_Z | ONcBodySurfaceFlag_MainAxis_X,	// larm1
	0							| ONcBodySurfaceFlag_Both_Y | ONcBodySurfaceFlag_Both_Z | ONcBodySurfaceFlag_MainAxis_X,	// larm2
	ONcBodySurfaceFlag_Pos_X	| ONcBodySurfaceFlag_Both_Y | ONcBodySurfaceFlag_Both_Z | ONcBodySurfaceFlag_MainAxis_X,	// lhand
	0																					| ONcBodySurfaceFlag_MainAxis_X,	// rarm
	0							| ONcBodySurfaceFlag_Both_Y | ONcBodySurfaceFlag_Both_Z | ONcBodySurfaceFlag_MainAxis_X,	// rarm1
	0							| ONcBodySurfaceFlag_Both_Y | ONcBodySurfaceFlag_Both_Z | ONcBodySurfaceFlag_MainAxis_X,	// rarm2
	ONcBodySurfaceFlag_Pos_X	| ONcBodySurfaceFlag_Both_Y | ONcBodySurfaceFlag_Both_Z | ONcBodySurfaceFlag_MainAxis_X		// rhand
};

static void ONiCharacter_Fire(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter, ONtFireMode inFireMode);
static void ONiCharacter_StopFiring(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);
static UUtUns16 ONiGameState_GetFreeCharacter(void);
static UUtUns16 ONiGameState_GetFreeActiveCharacter(void);
static void iCollideTwoCharacters(ONtCharacter *inCharA, ONtCharacter *inCharB);
static const TRtAnimation *RemapAnimationHook(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter, const TRtAnimation *animation);
typedef enum {
	ONcIgnoreFinalRotation,
	ONcApplyFinalRotation
} ONtFinalRotationMode;

static void ONiCharacter_OldAnimationHook(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter, ONtFinalRotationMode inFinalRotationMode, const TRtAnimation *nextAnimation);


static float iSignAngle(float angle);
static void ONrCharacter_Verify(const ONtCharacter *inCharacter, const ONtActiveCharacter *ioActiveCharacter);
static void ONiOrientations_Verify(ONtCharacter *inCharacter, ONtActiveCharacter *ioActiveCharacter);
static void ONrCharacter_Display_Local(const ONtCharacter *inCharacter);
static UUtBool FindGroundCollision(ONtCharacter *inCharacter, ONtActiveCharacter *ioActiveCharacter);
static void ONiCharacter_ThrowPhysics(ONtCharacter *inSource, ONtActiveCharacter *inActiveSource,
									  ONtCharacter *inTarget, ONtActiveCharacter *inActiveTarget);
static const TRtAnimation *ONrCharacter_DoAnimationSlide(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter,
														 UUtUns16 inAnimType, UUtUns16 inFrames);

void ONrCharacter_Invis_Update(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);
UUtUns32 ONrCharacter_FindParts(const ONtCharacter *inCharacter, M3tPoint3D *inLocation, float inRadius);

static void ONiCharacter_InitializeShadow(ONtActiveCharacter *ioActiveCharacter);
static void ONiCharacter_AllocateShadow(ONtActiveCharacter *ioActiveCharacter);
static void ONiCharacter_DeallocateShadow(ONtActiveCharacter *ioActiveCharacter);
static void ONiCharacter_DeleteShadow(ONtActiveCharacter *ioActiveCharacter);

#if DEBUG_ATTACKEXTENT
// CB: temporary debugging
static UUtBool			ONgTempDebug_AttackExtent_ShowGlobalExtents = UUcFalse;
static UUtError
ONiTempDebug_AttackExtent(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue);
#endif

static void ONrCharacter_Stitch(
	ONtCharacter *ioCharacter,
	ONtActiveCharacter *ioActiveCharacter,
	TRtAnimState fromState,
	const TRtAnimation *inNewAnim,
	UUtInt32 inStitchCount);
const ONtAirConstants *ONrCharacter_GetAirConstants(const ONtCharacter *inCharacter);
static const TRtAnimation *ONiAnimation_FindBlock(const ONtCharacter *inCharacter, ONtActiveCharacter *ioActiveCharacter);

static UUtBool ONrCharacter_Collide_With_GQ(
	UUtUns16			inGQIndex,
	const M3tPoint3D *inIntersection,
	const ONtCharacter *inCharacter,
	const M3tVector3D *inVector);

// hit control stuff
static void ONiCharacter_ClearHits(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);
static UUtBool ONiCharacter_TestAndSetHit(ONtCharacter *ioAttacker, ONtActiveCharacter *ioActiveCharacter,
										  const ONtCharacter *inTarget, UUtUns32 inAttack);
static void ONrCharacter_UpdateSphereTree(
	ONtCharacter *ioCharacter,
	ONtActiveCharacter *activeCharacter);

static void
ONrCharacter_Callback_FindEnvCollisions(
	const PHtPhysicsContext *inContext,
	const AKtEnvironment *inEnvironment,
	const M3tVector3D *inVelocity,
	PHtCollider *outColliders,
	UUtUns16 *ioNumColliders);

static void
ONrCharacter_Callback_FindPhyCollisions(
	const PHtPhysicsContext *inContext,
	const M3tVector3D *inVelocity,
	PHtCollider *outColliders,
	UUtUns16 *ioNumColliders);

static void
ONrCharacter_Callback_PrivateMovement(
	PHtPhysicsContext *inContext);

static void
ONrCharacter_Callback_PostReflection(
	PHtPhysicsContext *ioContext,
	M3tVector3D *inVelocityThisFrame);

static void ONrCharacter_Callback_ReceiveForce(
	PHtPhysicsContext *ioContext,
	const PHtPhysicsContext *inPushingContext,
	const M3tVector3D *inForce);

static ONtCharacter *ONiCharacter_FindNearestOpponent(
			ONtCharacter *inCharacter,
			float inFacingOffset,
			float inMaxDistance,
			float inMaxAngle,
			float *outFacingOffset,
			UUtBool inSkipInvulnerable);

static void ONrGameState_DoCharacterDebugFrame(
					ONtCharacter *ioCharacter);

static float iMoveTowardsZero(float inNumber, float inAmtTowardsZero);
static void ONrCharacter_SetWeaponVarient(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);
static void ONrCharacter_VerifyExtraBody(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);

void ONrCharacter_DumpAllCollections(void);

static void ONiCharacter_RunDeathAnimation(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter, TRtAnimType inDeathType);
static void ONiCharacter_PainUpdate(ONtCharacter *ioCharacter);

// super particle functions
static void ONiCharacter_SuperParticle_Create(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);
static void ONiCharacter_SuperParticle_Delete(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);
static void ONiCharacter_SuperParticle_SendEvent(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter, UUtUns16 inEvent);

#include <string.h>

typedef struct ONtAnimTypeToStringTable
{
	TRtAnimType type;
	const char *string;
} ONtAnimTypeToStringTable;

typedef struct AnimStateToStringTable
{
	TRtAnimState state;
	const char *string;
} ONtAnimStateToStringTable;

static UUtBool ONgAnimTypeToStringTableSorted = UUcFalse;
static UUtBool ONgAnimStateToStringTableSorted = UUcFalse;
static UUtUns32 ONgAnimTypeToStringTableLength;
static UUtUns32 ONgAnimStateToStringTableLength;

static ONtAnimTypeToStringTable ONcAnimTypeToStringTable[] =
{
	{ ONcAnimType_None,								"None" },						// 0
	{ ONcAnimType_Anything,							"Anything" },
	{ ONcAnimType_Walk,								"Walk" },
	{ ONcAnimType_Run,								"Run" },
	{ ONcAnimType_Slide,							"Slide" },

	{ ONcAnimType_Jump,								"Jump" },						// 5
	{ ONcAnimType_Stand,							"Stand" },
	{ ONcAnimType_Standing_Turn_Left,				"Standing_Turn_Left" },
	{ ONcAnimType_Standing_Turn_Right,				"Standing_Turn_Right" },
	{ ONcAnimType_Run_Backwards,					"Run_Backwards" },

	{ ONcAnimType_Run_Sidestep_Left,				"Run_Sidestep_Left" },			// 10
	{ ONcAnimType_Run_Sidestep_Right,				"Run_Sidestep_Right" },
	{ ONcAnimType_Kick,								"Kick" },
	{ ONcAnimType_Walk_Sidestep_Left,				"Walk_Sidestep_Left" },
	{ ONcAnimType_Walk_Sidestep_Right,				"Walk_Sidestep_Right" },

	{ ONcAnimType_Walk_Backwards,					"Walk_Backwards" },				// 15
	{ ONcAnimType_Stance,							"Stance" },
	{ ONcAnimType_Crouch,							"Crouch" },
	{ ONcAnimType_Jump_Forward,						"Jump_Forward" },
	{ ONcAnimType_Jump_Backward,					"Jump_Backward" },

	{ ONcAnimType_Jump_Left,						"Jump_Left" },					// 20
	{ ONcAnimType_Jump_Right,						"Jump_Right" },
	{ ONcAnimType_Punch,							"Punch" },
	{ ONcAnimType_Block,							"Block" },
	{ ONcAnimType_Land,								"Land" },

	{ ONcAnimType_Fly,								"Fly" },						// 25
	{ ONcAnimType_Kick_Forward,						"Kick_Forward" },
	{ ONcAnimType_Kick_Left,						"Kick_Left" },
	{ ONcAnimType_Kick_Right,						"Kick_Right" },
	{ ONcAnimType_Kick_Back,						"Kick_Back" },
	{ ONcAnimType_Kick_Low,							"Kick_Low" },					// 30

	{ ONcAnimType_Punch_Forward,					"Punch_Forward" },
	{ ONcAnimType_Punch_Left,						"Punch_Left" },
	{ ONcAnimType_Punch_Right,						"Punch_Right" },
	{ ONcAnimType_Punch_Back,						"Punch_Back" },
	{ ONcAnimType_Punch_Low,						"Punch_Low" },					// 35

	{ ONcAnimType_Kick2,							"Kick2" },
	{ ONcAnimType_Kick3,							"Kick3" },

	{ ONcAnimType_Punch2,							"Punch2" },
	{ ONcAnimType_Punch3,							"Punch3" },

	{ ONcAnimType_Land_Forward,						"Land_Forward" },				// 40
	{ ONcAnimType_Land_Right,						"Land_Right" },
	{ ONcAnimType_Land_Left,						"Land_Left" },
	{ ONcAnimType_Land_Back,						"Land_Back" },

	{ ONcAnimType_PPK,								"PPK" },
	{ ONcAnimType_PKK,								"PKK" },						// 45
	{ ONcAnimType_PKP,								"PKP" },

	{ ONcAnimType_KPK,								"KPK" },
	{ ONcAnimType_KPP,								"KPP" },
	{ ONcAnimType_KKP,								"KKP" },

	{ ONcAnimType_PK,								"PK" },							// 50
	{ ONcAnimType_KP,								"KP" },

	{ ONcAnimType_Punch_Heavy,						"Punch_Heavy" },
	{ ONcAnimType_Kick_Heavy,						"Kick_Heavy" },
	{ ONcAnimType_Punch_Forward_Heavy,				"Punch_Forward_Heavy" },
	{ ONcAnimType_Kick_Forward_Heavy,				"Kick_Forward_Heavy" },			// 55

	{ ONcAnimType_Aiming_Overlay,					"Aiming_Overlay" },
	{ ONcAnimType_Hit_Overlay,						"Hit_Overlay" },

	{ ONcAnimType_Crouch_Run,						"Crouch_Run" },
	{ ONcAnimType_Crouch_Walk,						"Crouch_Walk" },

	{ ONcAnimType_Crouch_Run_Backwards,				"Crouch_Run_Backwards" },		// 60
	{ ONcAnimType_Crouch_Walk_Backwards,			"Crouch_Walk_Backwards" },

	{ ONcAnimType_Crouch_Run_Sidestep_Left,			"Crouch_Run_Sidestep_Left" },
	{ ONcAnimType_Crouch_Run_Sidestep_Right,		"Crouch_Run_Sidestep_Right" },

	{ ONcAnimType_Crouch_Walk_Sidestep_Left,		"Crouch_Walk_Sidestep_Left" },
	{ ONcAnimType_Crouch_Walk_Sidestep_Right,		"Crouch_Walk_Sidestep_Right" },	// 65

	{ ONcAnimType_Run_Kick,							"Run_Kick" },
	{ ONcAnimType_Run_Punch,						"Run_Punch" },

	{ ONcAnimType_Run_Back_Punch,					"Run_Back_Punch" },
	{ ONcAnimType_Run_Back_Kick,					"Run_Back_Kick" },

	{ ONcAnimType_Sidestep_Left_Kick,				"Sidestep_Left_Kick" },			// 70
	{ ONcAnimType_Sidestep_Left_Punch,				"Sidestep_Left_Punch" },

	{ ONcAnimType_Sidestep_Right_Kick,				"Sidestep_Right_Kick" },
	{ ONcAnimType_Sidestep_Right_Punch,				"Sidestep_Right_Punch" },

	{ ONcAnimType_Prone,							"Prone" },
	{ ONcAnimType_Flip,								"Flip" },						// 75

	{ ONcAnimType_Hit_Head,							"Hit_Head" },
	{ ONcAnimType_Hit_Body,							"Hit_Body" },
	{ ONcAnimType_Hit_Foot,							"Hit_Foot" },

	{ ONcAnimType_Knockdown_Head,					"Knockdown_Head" },
	{ ONcAnimType_Knockdown_Body,					"Knockdown_Body" },				// 80
	{ ONcAnimType_Knockdown_Foot,					"Knockdown_Foot" },

	{ ONcAnimType_Hit_Crouch,						"Hit_Crouch" },
	{ ONcAnimType_Knockdown_Crouch,					"Knockdown_Crouch" },

	{ ONcAnimType_Hit_Fallen,						"Hit_Fallen" },

	{ ONcAnimType_Hit_Head_Behind,					"Hit_Head_Behind" },			// 85
	{ ONcAnimType_Hit_Body_Behind,					"Hit_Body_Behind" },
	{ ONcAnimType_Hit_Foot_Behind,					"Hit_Foot_Behind" },

	{ ONcAnimType_Knockdown_Head_Behind,			"Knockdown_Head_Behind" },
	{ ONcAnimType_Knockdown_Body_Behind,			"Knockdown_Body_Behind" },
	{ ONcAnimType_Knockdown_Foot_Behind,			"Knockdown_Foot_Behind" },		// 90

	{ ONcAnimType_Hit_Crouch_Behind,				"Hit_Crouch_Behind" },
	{ ONcAnimType_Knockdown_Crouch_Behind,			"Knockdown_Crouch_Behind" },

	{ ONcAnimType_Idle,								"Idle" },
	{ ONcAnimType_Taunt,							"Taunt" },

	{ ONcAnimType_Throw,							"Throw" },

	{ ONcAnimType_Thrown1,							"Thrown1" },
	{ ONcAnimType_Thrown2,							"Thrown2" },
	{ ONcAnimType_Thrown3,							"Thrown3" },
	{ ONcAnimType_Thrown4,							"Thrown4" },
	{ ONcAnimType_Thrown5,							"Thrown5" },
	{ ONcAnimType_Thrown6,							"Thrown6" },

	{ ONcAnimType_Special1,							"Special1" },
	{ ONcAnimType_Special2,							"Special2" },
	{ ONcAnimType_Special3,							"Special3" },
	{ ONcAnimType_Special4,							"Special4" },

	{ ONcAnimType_Throw_Forward_Punch,				"Throw_Forward_Punch" },
	{ ONcAnimType_Throw_Forward_Kick,				"Throw_Forward_Kick" },
	{ ONcAnimType_Throw_Behind_Punch,				"Throw_Behind_Punch" },
	{ ONcAnimType_Throw_Behind_Kick,				"Throw_Behind_Kick" },

	{ ONcAnimType_Run_Throw_Forward_Punch,			"Run_Throw_Forward_Punch" },
	{ ONcAnimType_Run_Throw_Behind_Punch,			"Run_Throw_Behind_Punch" },
	{ ONcAnimType_Run_Throw_Forward_Kick,			"Run_Throw_Forward_Kick" },
	{ ONcAnimType_Run_Throw_Behind_Kick,			"Run_Throw_Behind_Kick" },

	{ ONcAnimType_Thrown7,							"Thrown7" },
	{ ONcAnimType_Thrown8,							"Thrown8" },
	{ ONcAnimType_Thrown9,							"Thrown9" },
	{ ONcAnimType_Thrown10,							"Thrown10" },
	{ ONcAnimType_Thrown11,							"Thrown11" },
	{ ONcAnimType_Thrown12,							"Thrown12" },

	{ ONcAnimType_Startle_Left_Unused,				"Startle_Left_Unused" },
	{ ONcAnimType_Startle_Right_Unused,				"Startle_Right_Unused" },

	{ ONcAnimType_Sit,								"Sit" },
	{ ONcAnimType_Stand_Special,					"Stand_Special" },
	{ ONcAnimType_Act,								"Act" },
	{ ONcAnimType_Kick3_Forward,					"Kick3_Forward" },

	{ ONcAnimType_Hit_Foot_Ouch,					"Hit_Foot_Ouch" },
	{ ONcAnimType_Hit_Jewels,						"Hit_Jewels" },
	{ ONcAnimType_Thrown13,							"Thrown13" },
	{ ONcAnimType_Thrown14,							"Thrown14" },
	{ ONcAnimType_Thrown15,							"Thrown15" },
	{ ONcAnimType_Thrown16,							"Thrown16" },
	{ ONcAnimType_Thrown17,							"Thrown17" },

	{ ONcAnimType_PPKK,								"PPKK" },
	{ ONcAnimType_PPKKK,							"PPKKK" },
	{ ONcAnimType_PPKKKK,							"PPKKKK" },

	{ ONcAnimType_Land_Hard,						"Land_Hard" },
	{ ONcAnimType_Land_Forward_Hard,				"Land_Forward_Hard" },
	{ ONcAnimType_Land_Right_Hard,					"Land_Right_Hard" },
	{ ONcAnimType_Land_Left_Hard,					"Land_Left_Hard" },
	{ ONcAnimType_Land_Back_Hard,					"Land_Back_Hard" },

	{ ONcAnimType_Land_Dead,						"Land_Dead" },
	{ ONcAnimType_Crouch_Turn_Left,					"Crouch_Turn_Left" },
	{ ONcAnimType_Crouch_Turn_Right,				"Crouch_Turn_Right" },

	{ ONcAnimType_Crouch_Forward,					"Crouch_Forward" },
	{ ONcAnimType_Crouch_Back,						"Crouch_Back" },
	{ ONcAnimType_Crouch_Left,						"Crouch_Left" },
	{ ONcAnimType_Crouch_Right,						"Crouch_Right" },

	{ ONcAnimType_Getup_Kick_Back,					"Getup_Kick_Back" },

	{ ONcAnimType_Autopistol_Recoil,				"Autopistol_Recoil" },
	{ ONcAnimType_Phase_Rifle_Recoil,				"Phase_Rifle_Recoil" },
	{ ONcAnimType_Phase_Stream_Recoil,				"Phase_Stream_Recoil" },
	{ ONcAnimType_Superball_Recoil,					"Superball_Recoil" },
	{ ONcAnimType_Vandegraf_Recoil,					"Vandegraf_Recoil" },
	{ ONcAnimType_Scram_Cannon_Recoil,				"Scram_Cannon_Recoil" },
	{ ONcAnimType_Mercury_Bow_Recoil,				"Mercury_Bow_Recoil" },
	{ ONcAnimType_Screamer_Recoil,					"Screamer_Recoil" },

	{ ONcAnimType_Pickup_Object,					"Pickup_Object" },
	{ ONcAnimType_Pickup_Pistol,					"Pickup_Pistol" },
	{ ONcAnimType_Pickup_Rifle,						"Pickup_Rifle" },

	{ ONcAnimType_Holster,							"Holster" },
	{ ONcAnimType_Draw_Pistol,						"Draw_Pistol" },
	{ ONcAnimType_Draw_Rifle,						"Draw_Rifle" },

	{ ONcAnimType_Punch4,							"Punch4" },

	{ ONcAnimType_Reload_Pistol,					"Reload_Pistol" },
	{ ONcAnimType_Reload_Rifle,						"Reload_Rifle" },
	{ ONcAnimType_Reload_Stream,					"Reload_Stream" },
	{ ONcAnimType_Reload_Superball,					"Reload_Superball" },
	{ ONcAnimType_Reload_Vandegraf,					"Reload_Vandegraf" },
	{ ONcAnimType_Reload_Scram_Cannon,				"Reload_Scram_Cannon" },
	{ ONcAnimType_Reload_MercuryBow,				"Reload_MercuryBow" },
	{ ONcAnimType_Reload_Screamer,					"Reload_Screamer" },

	{ ONcAnimType_PF_PF,							"PF_PF" },
	{ ONcAnimType_PF_PF_PF,							"PF_PF_PF" },

	{ ONcAnimType_PL_PL,							"PL_PL" },
	{ ONcAnimType_PL_PL_PL,							"PL_PL_PL" },

	{ ONcAnimType_PR_PR,							"PR_PR" },
	{ ONcAnimType_PR_PR_PR,							"PR_PR_PR" },

	{ ONcAnimType_PB_PB,							"PB_PB" },
	{ ONcAnimType_PB_PB_PB,							"PB_PB_PB" },

	{ ONcAnimType_PD_PD,							"PD_PD" },
	{ ONcAnimType_PD_PD_PD,							"PD_PD_PD" },

	{ ONcAnimType_KF_KF,							"KF_KF" },
	{ ONcAnimType_KF_KF_KF,							"KF_KF_KF" },

	{ ONcAnimType_KL_KL,							"KL_KL" },
	{ ONcAnimType_KL_KL_KL,							"KL_KL_KL" },

	{ ONcAnimType_KR_KR,							"KR_KR" },
	{ ONcAnimType_KR_KR_KR,							"KR_KR_KR" },

	{ ONcAnimType_KB_KB,							"KB_KB" },
	{ ONcAnimType_KB_KB_KB,							"KB_KB_KB" },

	{ ONcAnimType_KD_KD,							"KD_KD" },
	{ ONcAnimType_KD_KD_KD,							"KD_KD_KD" },

	{ ONcAnimType_Startle_Left,						"Startle_Left" },
	{ ONcAnimType_Startle_Right,					"Startle_Right" },
	{ ONcAnimType_Startle_Back,						"Startle_Back" },
	{ ONcAnimType_Startle_Forward,					"Startle_Forward" },

	{ ONcAnimType_Console,							"Console" },
	{ ONcAnimType_Console_Walk,						"Console_Walk" },

	{ ONcAnimType_Stagger,							"Stagger" },

	{ ONcAnimType_Watch,							"Watch" },

	{ ONcAnimType_Act_No,							"Act_No" },
	{ ONcAnimType_Act_Yes,							"Act_Yes" },
	{ ONcAnimType_Act_Talk,							"Act_Talk" },
	{ ONcAnimType_Act_Shrug,						"Act_Shrug" },
	{ ONcAnimType_Act_Shout,						"Act_Shout" },
	{ ONcAnimType_Act_Give,							"Act_Give" },

	{ ONcAnimType_Run_Stop,							"Run_Stop" },
	{ ONcAnimType_Walk_Stop,						"Walk_Stop" },

	{ ONcAnimType_Run_Start,						"Run_Start" },
	{ ONcAnimType_Walk_Start,						"Walk_Start" },

	{ ONcAnimType_Run_Backwards_Start,				"Run_Backwards_Start" },
	{ ONcAnimType_Walk_Backwards_Start,				"Walk_Backwards_Start" },

	{ ONcAnimType_Stun,								"Stun" },
	{ ONcAnimType_Stagger_Behind,					"Stagger_Behind" },
	{ ONcAnimType_Blownup,							"Blownup" },
	{ ONcAnimType_Blownup_Behind,					"Blownup_Behind" },

	{ ONcAnimType_1Step_Stop,						"1Step_Stop" },
	{ ONcAnimType_Run_Sidestep_Left_Start,			"Run_Sidestep_Left_Start" },
	{ ONcAnimType_Run_Sidestep_Right_Start,			"Run_Sidestep_Right_Start" },

	{ ONcAnimType_Powerup,							"Powerup" },
	{ ONcAnimType_Falling_Flail,					"Falling_Flail" },

	{ ONcAnimType_Console_Punch,					"Console_Punch" },

	{ ONcAnimType_Teleport_In,						"Teleport_In" },
	{ ONcAnimType_Teleport_Out,						"Teleport_Out" },
	{ ONcAnimType_Ninja_Fireball,					"Ninja_Fireball" },
	{ ONcAnimType_Ninja_Invisible,					"Ninja_Invisible" },
	{ ONcAnimType_Punch_Rifle,						"Punch_Rifle" },

	{ ONcAnimType_Pickup_Object_Mid,				"Pickup_Object_Mid" },
	{ ONcAnimType_Pickup_Pistol_Mid,				"Pickup_Pistol_Mid" },
	{ ONcAnimType_Pickup_Rifle_Mid,					"Pickup_Rifle_Mid" },

	{ ONcAnimType_Hail,								"Hail" },

	{ ONcAnimType_Muro_Thunderbolt,					"Muro_Thunderbolt" },
	{ ONcAnimType_Hit_Overlay_AI,					"Hit_Overlay_AI" },

	{ 0, NULL }
};

static ONtAnimStateToStringTable ONcAnimStateToStringTable[] =
{
	{ONcAnimState_None,								"None"},							// 0
	{ONcAnimState_Anything,							"Anything"},
	{ONcAnimState_Running_Left_Down,				"Running_Left_Down"},
	{ONcAnimState_Running_Right_Down,				"Running_Right_Down"},
	{ONcAnimState_Sliding,							"Sliding"},

	{ONcAnimState_Walking_Left_Down,				"Walking_Left_Down"},				// 5
	{ONcAnimState_Walking_Right_Down,				"Walking_Right_Down"},
	{ONcAnimState_Standing,							"Standing"},
	{ONcAnimState_Run_Start,						"Run_Start"},
	{ONcAnimState_Run_Accel,						"Run_Accel"},

	{ONcAnimState_Run_Sidestep_Left,				"Run_Sidestep_Left"},				// 10
	{ONcAnimState_Run_Sidestep_Right,				"Run_Sidestep_Right"},
	{ONcAnimState_Run_Slide,						"Run_Slide"},
	{ONcAnimState_Run_Jump,							"Run_Jump"},
	{ONcAnimState_Run_Jump_Land,					"Run_Jump_Land"},

	{ONcAnimState_Run_Back_Start,					"Run_Back_Start"},					// 15
	{ONcAnimState_Running_Back_Right_Down,			"Running_Back_Right_Down"},
	{ONcAnimState_Running_Back_Left_Down,			"Running_Back_Left_Down"},
	{ONcAnimState_Fallen_Back,						"Fallen_Back"},
	{ONcAnimState_Crouch,							"Crouch"},

	{ONcAnimState_Running_Upstairs_Right_Down,		"Running_Upstairs_Right_Down"},		// 20
	{ONcAnimState_Running_Upstairs_Left_Down,		"Running_Upstairs_Left_Down"},
	{ONcAnimState_Sidestep_Left_Left_Down,			"Sidestep_Left_Left_Down"},
	{ONcAnimState_Sidestep_Left_Right_Down,			"Sidestep_Left_Right_Down"},
	{ONcAnimState_Sidestep_Right_Left_Down,			"Sidestep_Right_Left_Down"},

	{ONcAnimState_Sidestep_Right_Right_Down,		"Sidestep_Right_Right_Down"},		// 25
	{ONcAnimState_Sidestep_Right_Jump,				"Sidestep_Right_Jump"},
	{ONcAnimState_Sidestep_Left_Jump,				"Sidestep_Left_Jump"},
	{ONcAnimState_Jump_Forward,						"Jump_Forward"},
	{ONcAnimState_Jump_Up,							"Jump_Up"},

	{ONcAnimState_Run_Back_Slide,					"Run_Back_Slide"},					// 30
	{ONcAnimState_Lie_Back,							"Lie_Back"},
	{ONcAnimState_Sidestep_Left_Start,				"Sidestep_Left_Start"},
	{ONcAnimState_Sidestep_Right_Start,				"Sidestep_Right_Start"},
	{ONcAnimState_Walking_Sidestep_Left,			"Walking_Sidestep_Left"},

	{ONcAnimState_Crouch_Walk,						"Crouch_Walk"},						// 35
	{ONcAnimState_Walking_Sidestep_Right,			"Walking_Sidestep_Right"},
	{ONcAnimState_Flying,							"Flying"},
	{ONcAnimState_Falling,							"Falling"},
	{ONcAnimState_Flying_Forward,					"Flying_Forward"},

	{ONcAnimState_Falling_Forward,					"Falling_Forward"},					// 40
	{ONcAnimState_Flying_Back,						"Flying_Back"},
	{ONcAnimState_Falling_Back,						"Falling_Back"},
	{ONcAnimState_Flying_Left,						"Flying_Left"},
	{ONcAnimState_Falling_Left,						"Falling_Left"},

	{ONcAnimState_Flying_Right,						"Flying_Right"},					// 45
	{ONcAnimState_Falling_Right,					"Falling_Right"},
	{ONcAnimState_Crouch_Start,						"Crouch_Start"},
	{ONcAnimState_Walking_Back_Left_Down,			"Walking_Back_Left_Down"},
	{ONcAnimState_Walking_Back_Right_Down,			"Walking_Back_Right_Down"},

	{ONcAnimState_Fallen_Front,						"Fallen_Front"},					// 50
	{ONcAnimState_Sidestep_Left_Start_Long_Name,	"Sidestep_Left_Start_Long_Name"},
	{ONcAnimState_Sidestep_Right_Start_Long_Name,	"Sidestep_Right_Start_Long_Name"},
	{ONcAnimState_Sit,								"Sit"},
	{ONcAnimState_Punch_Low,						"Punch_Low"},

	{ONcAnimState_Stand_Special,					"Stand_Special"},					// 55
	{ONcAnimState_Acting,							"Acting"},

	{ONcAnimState_Crouch_Run_Left,					"Crouch_Run_Left"},
	{ONcAnimState_Crouch_Run_Right,					"Crouch_Run_Right"},
	{ONcAnimState_Crouch_Run_Back_Left,				"Crouch_Run_Back_Left"},
	{ONcAnimState_Crouch_Run_Back_Right,			"Crouch_Run_Back_Right"},			// 60

	{ONcAnimState_Blocking1,						"Blocking1"},
	{ONcAnimState_Blocking2,						"Blocking2"},
	{ONcAnimState_Blocking3,						"Blocking3"},
	{ONcAnimState_Crouch_Blocking1,					"Crouch_Blocking1"},

	{ ONcAnimState_Gliding,							"Gliding"},							// 65
	{ ONcAnimState_Watch_Idle,						"Watch_Idle" },
	{ ONcAnimState_Stunned,							"Stunned" },
	{ ONcAnimState_Powerup,							"Powerup" },
	{ ONcAnimState_Thunderbolt,						"Thunderbolt" },

	{ 0,											NULL }
};

// variables
static UUtBool ONgShow_Corpses = UUcTrue;
static UUtBool ONgWpForceScale = UUcFalse;
static UUtBool ONgWpForceHalfScale = UUcFalse;
static UUtBool ONgWpForceNoScale = UUcFalse;
static float ONgWpScaleAdjustment = 1.f;
static float ONgWpPowAdjustment = 0.3f;


static UUtBool ONgSpatialFootsteps = UUcTrue;
UUtUns16	ONgNumCharactersDrawn = 0;
static float ONgLookspringFightModePercent = 0.5f;
static UUtBool gChrDebugPathfinding = UUcFalse;
static UUtBool gChrDebugFall = UUcFalse;
static float ONgAutoAim_Arc = 90.f;
static float ONgAutoAim_Distance = 40.f;
static UUtInt32 gDebugCharacterTarget = 0;
static UUtBool gDebugCharacters = UUcFalse;
static UUtBool gDebugOverlay = UUcFalse;
static UUtBool gDebugCharactersSphereTree = UUcFalse;
static UUtBool gPrintSound = UUcFalse;
static UUtBool gDebugCollision = UUcFalse;
static UUtBool gShowTriggerQuad = UUcFalse;
static UUtBool gDoCollision = UUcTrue;
static UUtBool gDrawFacingVector = UUcFalse;
static UUtBool gDrawAimingVector = UUcFalse;
static UUtBool gPauseCharacters = UUcTrue;
static float ONgBlockAngle = 20.f;
UUtBool ONgBigHead = UUcFalse;
static float ONgBigHeadAmount = 4.f;
static UUtBool ONgMiniMe = UUcFalse;
static float	ONgMiniMe_Amount = 0.5f;
static UUtInt32	ONgForceLOD = -1;
static UUtBool ONgMarketing_Line_Off = UUcFalse;
static UUtBool ONgDrawDot;
static M3tPoint3D ONgDot;
static UUtBool ONgShowAimingScreen = UUcFalse;
float ONgAimingWidth = 70.f;	// also used in game state
static UUtInt32 ONgDeathFadeStart = 60 * 60 * 2;
static UUtInt32 ONgDeathFadeFrames = 180;
static UUtBool gConsole_show_chr_env_collision = UUcFalse;
static UUtBool gConsole_show_laser_env_collision = UUcFalse;
COtStatusLine ONgChrStatus[9];
COtStatusLine ONgChrLODStatus[2];
COtStatusLine ONgChrOverlayStatus[ONcOverlayIndex_Count + 1];
COtStatusLine ONgBNVStatus[2];
UUtBool gShowBNV = UUcFalse;
static UUtBool ONgNoMeleeDamage = UUcFalse;
static UUtBool gShowLOD = UUcFalse;
static UUtBool gDrawWeapon = UUcTrue;
static UUtBool gPinCharacters = UUcFalse;
static UUtBool gDrawAllCharacters = UUcFalse;
UUtBool ONgAllCharactersSameHeight = UUcFalse;
static UUtBool gCorpseFadeOffscreen = UUcTrue;
UUtInt32 saved_film_character_offset = 1;
UUtBool saved_film_loop = UUcFalse;
static UUtBool gWeaponAutoAim = UUcTrue;
static UUtBool gDebugTriggerVolume = UUcFalse;
static UUtBool gDebugScripts = UUcFalse;
UUtBool gDebugGunBehavior = UUcFalse;
static UUtBool ONgCharacterShadow	= UUcTrue;
UUtBool ONgDebugCharacterImpacts = UUcFalse;
UUtBool ONgDebugFootsteps = UUcFalse;
UUtBool ONgDebugFootstepFlash = UUcFalse;
UUtBool ONgDebugVerboseFootsteps = UUcFalse;
UUtBool ONgCharactersAllActive = UUcFalse;
UUtBool ONgDisableVisActive = UUcFalse;
UUtBool ONgCharacterCollision_BoxEnable = UUcTrue;
float ONgCharacterCollision_Grow = 0.0f;

// current trigger volume that is updating (used for script debugging)
OBJtOSD_TriggerVolume *ONgCurrentlyActiveTrigger = NULL;
char *ONgCurrentlyActiveTrigger_Script = NULL;
ONtCharacter *ONgCurrentlyActiveTrigger_Cause = NULL;

#define ONcNumStitchingFrames 8


#if PERFORMANCE_TIMER
	UUtPerformanceTimer *ONg_TriggerVolume_Timer = NULL;
#endif


typedef struct StringNumLookup
{
	UUtUns16 num;
	char *string;
} StringNumLookup;

// more debugging code
UUtInt32			gDebugCharacterMovement = 0;
UUtInt32			gDebugCharacterMovement_Old = 0;
M3tPoint3D			*gSmoothedMovement = NULL;

UUtBool ONrOverlay_DoFrame(ONtCharacter *ioCharacter, ONtOverlay *ioOverlay)
{
	UUtBool last_frame = UUcFalse;
	const TRtAnimation *animation = ioOverlay->animation;

	if (NULL == animation) {
		goto exit;
	}

	// play the appropriate sound for this animation
	if (TRrAnimation_IsSoundFrame(animation, ioOverlay->frame)) {
		OSrSoundAnimation_Play(
			ioCharacter->characterClass->variant,
			animation,
			&ioCharacter->actual_position,		/* location won't be NULL like physics might */
			NULL,
			NULL);
	}

	ioOverlay->frame += 1;

	if (TRrAnimation_GetDuration(animation) == ioOverlay->frame) {
		ioOverlay->animation = NULL;
		ioOverlay->frame = 0;
		last_frame = UUcTrue;
	}

exit:
	return last_frame;
}

void ONrOverlay_Initialize(ONtOverlay *ioOverlay)
{
	ioOverlay->animation = NULL;
	ioOverlay->frame = 0;
}

void ONrCharacter_ApplyOverlay(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter, const ONtOverlay *inOverlay)
{
	if (inOverlay->animation != NULL) {
		TRrOverlay_Apply(
			inOverlay->animation,
			inOverlay->frame,
			ioActiveCharacter->curOrientations,
			ioActiveCharacter->curOrientations,
			ONrCharacter_GetNumParts(ioCharacter));
	}

	return;
}

void ONrOverlay_Set(ONtOverlay *ioOverlay, const TRtAnimation *inAnimation, UUtUns32 inFlags)
{
	ioOverlay->animation = inAnimation;
	ioOverlay->frame = 0;
	ioOverlay->flags = inFlags;
}

UUtBool ONrOverlay_IsActive(const ONtOverlay *inOverlay)
{
	UUtBool is_active = inOverlay->animation != NULL;

	return is_active;
}

UUtBool ONrCharacter_Overlays_TestFlag(ONtCharacter *inCharacter, ONtActiveCharacter *ioActiveCharacter, TRtAnimFlag inWhichFlag)
{
	UUtUns32 itr;
	UUtBool has_flag = UUcFalse;

	for(itr = 0; itr < ONcOverlayIndex_Count; itr++)
	{
		ONtOverlay *overlay = ioActiveCharacter->overlay + itr;

		if (overlay->animation != NULL) {
			has_flag = TRrAnimation_TestFlag(overlay->animation, inWhichFlag);
			if (has_flag) {
				break;
			}
		}
	}

	return has_flag;
}


static void ONrCharacter_ResetAnimation(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter, UUtBool inCopyInactive)
{
	TRtAnimState newState;
	TRtAnimType newType;
	const TRtAnimation *newAnimation;
	AI2tMovementDirection direction;
	AI2tMovementMode mode;

	if (inCopyInactive && (ioCharacter->flags & ONcCharacterFlag_AIMovement)) {
		// get our movement mode
		mode = ioCharacter->movementOrders.movementMode;
		if (mode == AI2cMovementMode_Default) {
			mode = AI2rMovement_Default(ioCharacter);
		}

		if (ioCharacter->movementStub.path_active) {
			// we're moving. which direction are we moving in?
			direction = AI2cMovementDirection_Forward;
			if (ioCharacter->movementOrders.facingMode == AI2cFacingMode_PathRelative) {
				direction = AI2cMovementDirection_Forward;
			}
		} else {
			direction = AI2cMovementDirection_Stopped;
		}

		switch(direction) {
			case AI2cMovementDirection_Stopped:
				newType = ONcAnimType_Stand;
				newState = (mode == AI2cMovementMode_Creep) ? ONcAnimState_Crouch : ONcAnimState_Standing;
			break;

			case AI2cMovementDirection_Forward:
				switch(mode) {
					case AI2cMovementMode_Creep:
						newType = ONcAnimType_Crouch_Run;
						newState = ONcAnimState_Crouch_Run_Left;
					break;

					case AI2cMovementMode_NoAim_Walk:
					case AI2cMovementMode_Walk:
						newType = ONcAnimType_Walk;
						newState = ONcAnimState_Walking_Left_Down;
					break;

					case AI2cMovementMode_NoAim_Run:
					case AI2cMovementMode_Run:
						newType = ONcAnimType_Run;
						newState = ONcAnimState_Running_Left_Down;
					break;
				}
			break;

			case AI2cMovementDirection_Sidestep_Left:
				switch(mode) {
					case AI2cMovementMode_Creep:
						newType = ONcAnimType_Crouch_Run_Sidestep_Left;
						newState = ONcAnimState_Crouch_Run_Left;
					break;

					case AI2cMovementMode_NoAim_Walk:
					case AI2cMovementMode_Walk:
						newType = ONcAnimType_Walk_Sidestep_Left;
						newState = ONcAnimState_Walking_Left_Down;
					break;

					case AI2cMovementMode_NoAim_Run:
					case AI2cMovementMode_Run:
						newType = ONcAnimType_Run_Sidestep_Left;
						newState = ONcAnimState_Sidestep_Left_Left_Down;
					break;
				}
			break;

			case AI2cMovementDirection_Sidestep_Right:
				switch(mode) {
					case AI2cMovementMode_Creep:
						newType = ONcAnimType_Crouch_Run_Sidestep_Right;
						newState = ONcAnimState_Crouch_Run_Left;
					break;

					case AI2cMovementMode_NoAim_Walk:
					case AI2cMovementMode_Walk:
						newType = ONcAnimType_Walk_Sidestep_Right;
						newState = ONcAnimState_Walking_Left_Down;
					break;

					case AI2cMovementMode_NoAim_Run:
					case AI2cMovementMode_Run:
						newType = ONcAnimType_Run_Sidestep_Right;
						newState = ONcAnimState_Sidestep_Left_Left_Down;
					break;
				}
			break;

			case AI2cMovementDirection_Backpedal:
				switch(mode) {
					case AI2cMovementMode_Creep:
						newType = ONcAnimType_Crouch_Run_Backwards;
						newState = ONcAnimState_Crouch_Run_Back_Left;
					break;

					case AI2cMovementMode_NoAim_Walk:
					case AI2cMovementMode_Walk:
						newType = ONcAnimType_Walk_Backwards;
						newState = ONcAnimState_Walking_Left_Down;
					break;

					case AI2cMovementMode_NoAim_Run:
					case AI2cMovementMode_Run:
						newType = ONcAnimType_Run_Backwards;
						newState = ONcAnimState_Running_Back_Left_Down;
					break;
				}
			break;
		}

		switch(mode) {
			case AI2cMovementMode_NoAim_Walk:
			case AI2cMovementMode_NoAim_Run:
				ONrCharacter_DisableWeaponVarient(ioCharacter, UUcTrue);
			break;

			case AI2cMovementMode_Creep:
			case AI2cMovementMode_Walk:
			case AI2cMovementMode_Run:
				ONrCharacter_DisableWeaponVarient(ioCharacter, UUcFalse);
			break;
		}

		if ((ioCharacter->charType == ONcChar_AI2) && (ioCharacter->ai2State.currentGoal == AI2cGoal_Panic)) {
			ONrCharacter_SetPanicVarient(ioCharacter, UUcTrue);
		} else {
			ONrCharacter_SetPanicVarient(ioCharacter, UUcFalse);
		}
	} else {
		newState = ONcAnimState_Standing;
		newType = ONcAnimType_Stand;
	}
	ONrCharacter_SetWeaponVarient(ioCharacter, ioActiveCharacter);

	ioActiveCharacter->curAnimType = newType;
	ioActiveCharacter->nextAnimType = newType;
	ioActiveCharacter->nextAnimState = newState;

	UUmAssert(ioCharacter->characterClass->animations != NULL);
	newAnimation = TRrCollection_Lookup(ioCharacter->characterClass->animations, newType, ioActiveCharacter->nextAnimState, ioActiveCharacter->animVarient);
	UUmAssertReadPtr(newAnimation, sizeof(*newAnimation));

	if (NULL != newAnimation) {
		ONrCharacter_SetAnimationInternal(ioCharacter, ioActiveCharacter, ioActiveCharacter->nextAnimState, ONcAnimType_None, newAnimation);
	}

	return;
}

static void InverseMaxMatrix(M3tMatrix4x3 *ioMatrix)
{
	M3tMatrix4x3 matrix;

	UUrMemory_Clear(&matrix, sizeof(matrix));

	MUrMatrix_Identity(&matrix);

	MUrMatrix_RotateX(&matrix, 0.5f * M3cPi);
	MUrMatrix_RotateY(&matrix, 0.f * M3cPi);
	MUrMatrix_RotateZ(&matrix, 0.f * M3cPi);

	MUrMatrixStack_Matrix(ioMatrix, &matrix);

	return;
}

void ONrCharacter_SetHand(TRtExtraBody *inExtraBody, ONtCharacterHanded inHanded)
{
	switch(inHanded)
	{
		case ONcCharacterIsLeftHanded:
			// gun
			inExtraBody->parts[0].translation;
			MUrQuat_Clear(&(inExtraBody->parts[0].quaternion));
			MUrZYXEulerToQuat(M3cPi, 0.f, 0.f, &(inExtraBody->parts[0].quaternion));
			inExtraBody->parts[0].index = ONcLHand_Index;
		break;

		case ONcCharacterIsRightHanded:
			// gun
			inExtraBody->parts[0].translation;
			MUrQuat_Clear(&(inExtraBody->parts[0].quaternion));
			MUrZYXEulerToQuat(0.f, 0.f, 0.f, &(inExtraBody->parts[0].quaternion));
			inExtraBody->parts[0].index = ONcRHand_Index;
		break;

		default:
			UUmAssert(0);
	}

	return;
}

static UUtError iStringToNumLookup(const StringNumLookup *inTable, const char *inString, UUtUns16 *result)
{
	int index;

	for(index = 0; inTable[index].string != NULL; index++)
	{
		if (0 == strcmp(inString, inTable[index].string))
			break;
	}

	*result = inTable[index].num;

	if (NULL == inTable[index].string)
	{
		return UUcError_Generic;
	}

	return UUcError_None;
}

static UUtError
ONrWho(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns32 count = ONrGameState_LivingCharacterList_Count();
	ONtCharacter ** characterList = ONrGameState_LivingCharacterList_Get();
	UUtUns32 itr;

	for(itr = 0; itr < count; itr++)
	{
		ONtCharacter *character = characterList[itr];
		ONtCharacterClass *character_class = character->characterClass;
		char *class_name = TMrInstance_GetInstanceName(character->characterClass);

		if (NULL == class_name) { class_name = ""; }

		COrConsole_Printf("character %s #%d id %d, class %s",
				character->player_name, ONrCharacter_GetIndex(character), character->scriptID, class_name);
	}

	return UUcError_None;
}


static UUtError
iSetCharacterWeapon(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	ONtCharacter *character;
	UUtError error;
	WPtWeapon *weapon = NULL;
	UUtBool changed = UUcFalse;
	UUtUns32 index;

	index = inParameterList[0].val.i;

	if (index >= ONgGameState->numCharacters) {
		return UUcError_None;
	}

	character = ONgGameState->characters + index;

	if (0 == (character->flags & ONcCharacterFlag_InUse)) {
		return UUcError_None;
	}

	if(inParameterList[1].type == SLcType_String)
	{
		error = ONrCharacter_ArmByName(character, inParameterList[1].val.str);
		if (UUcError_None == error)
		{
			COrConsole_Printf("armed with %s", TMrInstance_GetInstanceName(WPrGetClass(character->inventory.weapons[0])));
		}
	}
	else
	{
		UUtUns32 weaponNum;

		weaponNum = inParameterList[1].val.i;

		error = ONrCharacter_ArmByNumber(character, weaponNum);
		COrConsole_Printf("armed with %s (%d/%d)", TMrInstance_GetInstanceName(WPrGetClass(character->inventory.weapons[0])), weaponNum, TMrInstance_GetTagCount(WPcTemplate_WeaponClass));
	}

	if (UUcError_None == error)
	{
		UUmAssert(character->inventory.weapons[0]);
		changed = UUcTrue;
	}

	if (!changed) {
		COrConsole_Printf("failed to change");
	}

	return UUcError_None;
}

void ONrCharacter_UseNewWeapon(
	ONtCharacter *inCharacter,
	WPtWeaponClass *inClass)
{
	// Discards current weapon and creates a new one. For debugging only
	WPtWeapon *weapon = NULL;

	if (inCharacter->inventory.weapons[0]) {
		WPrDelete(inCharacter->inventory.weapons[0]);
	}

	if (inClass) {
		weapon = WPrNew(inClass, WPcNormalWeapon);
	}

	ONrCharacter_UseWeapon(inCharacter,weapon);

	return;
}

void ONrCharacter_UseWeapon_NameAmmo(ONtCharacter *inCharacter, char *inWeaponName, UUtUns32 inAmmo)
{
	WPtWeapon *weapon = NULL;
	WPtWeaponClass *weapon_class = WPrWeaponClass_GetFromName(inWeaponName);

	if (weapon_class != NULL) {
		weapon = WPrNew(weapon_class, WPcNormalWeapon);

		if (weapon != NULL) {
			WPrSetAmmo(weapon, inAmmo);

			ONrCharacter_UseWeapon(inCharacter, weapon);
		}
	}

	return;
}

UUtError
ONrCharacter_ArmByNumber(
	ONtCharacter *inCharacter,
	UUtUns32 inWeaponNumber)
{
	UUtError error;
	WPtWeaponClass *weapon;

	if (inCharacter->flags & ONcCharacterFlag_Dead) { return UUcError_None; }
	if (0 == (inCharacter->flags & ONcCharacterFlag_InUse)) { return UUcError_None; }

	inWeaponNumber = inWeaponNumber % TMrInstance_GetTagCount(WPcTemplate_WeaponClass);
	error = TMrInstance_GetDataPtr_ByNumber(WPcTemplate_WeaponClass, inWeaponNumber, &weapon);

	if (UUcError_None == error) {
		ONrCharacter_UseNewWeapon(inCharacter, weapon);
	}

	return error;
}


// CB: this code causes a crash or a hang, to test the error handling routines. 02 September 2000
static UUtError
DebugCrash(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns32 *crash_pointer = NULL;

	COrConsole_Printf("memory at 0x00000000 is: %d", *crash_pointer);

	return UUcError_None;
}

static UUtError
DebugHang(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns32 hang_counter = 0;

	while (1) {
		hang_counter++;
	}

	return UUcError_None;
}



static UUtError
PrintType(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	COrConsole_Printf("%s", ONrAnimTypeToString((TRtAnimType) inParameterList[0].val.i));

	return UUcError_None;
}


static UUtError
PrintState(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	COrConsole_Printf("%s", ONrAnimStateToString((TRtAnimState) inParameterList[0].val.i));

	return UUcError_None;
}

static UUtError
iDiplayCharacterCombatStats(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	ONtCharacter		*character_list;
	ONtCharacter		*character;
	UUtUns32			character_index;

	// get the character index
	character_index = inParameterList[0].val.i;

	// verify the range
	if (character_index > ONgGameState->numCharacters) {
		COrConsole_Printf("Character Number out of range");
		return UUcError_Generic;
	}

	// get a pointer to the character list
	character_list = ONrGameState_GetCharacterList();

	// get a pointer to the character
	character = &character_list[character_index];

	// make sure the character is in use
	if (!(character->flags & ONcCharacterFlag_InUse))
	{
		COrConsole_Printf("Invalid Character Number");
		return UUcError_Generic;
	}

	// display the character stats
	COrConsole_Printf(
		"Character %d: Damage Inflicted %d, Number of Kills %d",
		character_index,
		character->damageInflicted,
		character->numKills);

	return UUcError_None;
}

static UUtError
iSetMainCharacterClass(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	ONtCharacter *character = (ONtCharacter *) &ONgGameState->characters[0];
	UUtError error;
	ONtCharacterClass *characterClass = NULL;
	UUtBool changed = UUcFalse;

	if(inParameterList[0].type == SLcType_String)
	{
		error = TMrInstance_GetDataPtr(TRcTemplate_CharacterClass,	inParameterList[0].val.str,	(void **) &characterClass);
		if ((characterClass != NULL) && (UUcError_None == error))
		{
			character->characterClass = characterClass;
			COrConsole_Printf("changed");
			changed = UUcTrue;
		}
	}
	else
	{
		UUtUns32	characterNum;

		characterNum = inParameterList[0].val.i;

		characterNum = characterNum % TMrInstance_GetTagCount(TRcTemplate_CharacterClass);
		error = TMrInstance_GetDataPtr_ByNumber(TRcTemplate_CharacterClass, characterNum, &characterClass);

		if (UUcError_None == error)
		{
			ONrCharacter_SetCharacterClass(character, characterClass);

			COrConsole_Printf("changed to %s (%d / %d)", TMrInstance_GetInstanceName(character->characterClass), characterNum, TMrInstance_GetTagCount(TRcTemplate_CharacterClass));
			changed = UUcTrue;
		}
	}

	if (!changed) {
		COrConsole_Printf("failed to change");
	}

	return UUcError_None;
}

static UUtError
iSetCharacterClass(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	UUtError			error;
	ONtCharacter		*character_list;
	UUtUns32			character_index;

	// get a pointer to the character_list
	character_list = ONrGameState_GetCharacterList();

	// get the character index
	character_index = inParameterList[0].val.i;

	if (character_index > ONgGameState->numCharacters) {
		COrConsole_Printf("character index out of range");
		return UUcError_Generic;
	}

	if (inParameterListLength == 2)
	{
		ONtCharacterClass	*character_class;

		if (SLcType_String == inParameterList[1].type)
		{
		// get the character class the user wants to set all the character to
			error =
				TMrInstance_GetDataPtr(
					TRcTemplate_CharacterClass,
					inParameterList[1].val.str,
					(void **)&character_class);

			// user entered a class name
			ONrCharacter_SetCharacterClass(&character_list[character_index], character_class);
			COrConsole_Printf(
				"character %d class changed to %s",
				character_index,
				TMrInstance_GetInstanceName(character_list[character_index].characterClass));
		}
		else
		{
			// user entered a class number
			UUtUns32		character_class_number;

			// get the character class number
			character_class_number = inParameterList[1].val.i;

			// get the instance of the character class
			error =
				TMrInstance_GetDataPtr_ByNumber(
					TRcTemplate_CharacterClass,
					character_class_number,
					&character_class);
			if (error != UUcError_None)
			{
				COrConsole_Printf("character class was not changed");
				return UUcError_None;
			}

			// set the character class
			ONrCharacter_SetCharacterClass(&character_list[character_index], character_class);
			COrConsole_Printf(
				"character %d class changed to %s (%d / %d)",
				character_index,
				TMrInstance_GetInstanceName(character_list[character_index].characterClass),
				character_class_number,
				TMrInstance_GetTagCount(TRcTemplate_CharacterClass));
		}
	}
	else if (inParameterListLength == 1)
	{
		// print the current character class name of the character
		COrConsole_Printf(
			"character %d class name is %s",
			character_index,
			TMrInstance_GetInstanceName(character_list[character_index].characterClass));
	}

	return UUcError_None;
}

static void ONrFall(ONtCharacter *inCharacter, TRtAnimState inState)
{
	ONtActiveCharacter *active_character;

	UUmAssertReadPtr(inCharacter, sizeof(*inCharacter));
	active_character = ONrForceActiveCharacter(inCharacter);

	if (active_character != NULL)
	{
		ONtCharacterClass		*characterClass = inCharacter->characterClass;
		TRtAnimationCollection	*collection = characterClass->animations;
		const TRtAnimation *animation;
		TRtAnimType newAnimType;

		ONrCharacter_FightMode(inCharacter);
		active_character->lastAttack = ONcAnimType_None;
		active_character->lastAttackTime = 0;
		active_character->animationLockFrames = 0;	// stop looping an anim

		newAnimType = ONcAnimType_Stand;
		animation = TRrCollection_Lookup(collection, ONcAnimType_Stand, inState, active_character->animVarient);

		if (NULL != animation) {
			ONiCharacter_OldAnimationHook(inCharacter, active_character, ONcIgnoreFinalRotation, animation);
			ONrCharacter_Stitch(inCharacter, active_character, active_character->nextAnimState, animation, ONcNumStitchingFrames);
			ONrCharacter_NewAnimationHook(inCharacter, active_character);
			active_character->nextAnimType = ONcAnimType_Stand;
			active_character->curFromState = inState;
		}
	}

	return;
}

static UUtError
ONrFallFront(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	ONtCharacter			*character = ONrGameState_GetPlayerCharacter();

	ONrFall(character, ONcAnimState_Fallen_Front);

	return UUcError_None;
}

static UUtError
ONrFallBack(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	ONtCharacter			*character = ONrGameState_GetPlayerCharacter();

	ONrFall(character, ONcAnimState_Fallen_Back);

	return UUcError_None;
}

// delete all dynamically-created corpses
static UUtError
ONrCorpse_Reset(
	SLtErrorContext		*inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual	*inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual	*ioReturnValue)
{
	ONtCorpseArray *corpse_array = ONgLevel->corpseArray;

	if (corpse_array == NULL)
		return UUcError_None;

	if (corpse_array->static_corpses < corpse_array->max_corpses) {
		UUrMemory_Clear(&corpse_array->corpses[corpse_array->static_corpses],
						(corpse_array->max_corpses - corpse_array->static_corpses) * sizeof(ONtCorpse));
	}
	corpse_array->next = corpse_array->static_corpses;

	return UUcError_None;
}

static UUtError
ONrMakeCorpse(
	SLtErrorContext		*inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual	*inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual	*ioReturnValue)
{
	ONtCharacter	*character = ONrGameState_GetPlayerCharacter();
	ONtActiveCharacter *active_character = ONrForceActiveCharacter(character);
	char file_name[128];

	UUmAssertReadPtr(active_character, sizeof(*active_character));

	sprintf(file_name, "lvl_%d_%s_corpse.dat", ONgGameState->levelNumber, inParameterList[0].val.str);

#if UUmPlatform == UUmPlatform_Win32
	{
		BFtFile *stream = BFrFile_FOpen(file_name, "w");

		if (NULL != stream) {
			ONtCorpse_Data corpse_data;
			const char *character_class_name = TMrInstance_GetInstanceName(character->characterClass);
			char character_class_name_buffer[128];

			strncpy(character_class_name_buffer, character_class_name, sizeof(character_class_name_buffer));
			character_class_name_buffer[127] = '\0';

			BFrFile_Write(stream, sizeof(character_class_name_buffer), character_class_name_buffer);

			corpse_data.characterClass = NULL;
			corpse_data.corpse_bbox = character->boundingBox;
			UUrMemory_MoveFast(active_character->matricies, corpse_data.matricies, sizeof(corpse_data.matricies));

			BFrFile_Write(stream, sizeof(corpse_data), &corpse_data);

			BFrFile_Close(stream);

			COrConsole_Printf("%s written", file_name);
		}
	}
#endif


	return UUcError_None;
}


static UUtError
ONrKillAllAI(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns16 itr;

	for(itr = 0; itr < ONgGameState->numCharacters; itr++) {
		ONtCharacter *character = ONgGameState->characters + itr;

		if (0 == (character->flags & ONcCharacterFlag_InUse)) continue;
		if (0 == (character->flags & ONcCharacterFlag_Dead)) continue;

		if (ONrCharacter_IsAI(character)) {
			ONrCharacter_TakeDamage(character, character->hitPoints, 0.f, NULL, NULL, WPcDamageOwner_ActOfGod, ONcAnimType_None);
		}
	}

	return UUcError_None;
}

static void ONiCharacterClass_SetupParticleArray(ONtCharacterClass *inClass)
{
	ONtCharacterParticleArray	*particle_array = inClass->particles;
	UUtUns32 itr;

	if ((particle_array != NULL) && (!(particle_array->classes_found))) {
		// the character class hasn't found its particle classes yet. find them.
		for (itr = 0; itr < particle_array->numParticles; itr++) {
			particle_array->particle[itr].particle_class = P3rGetParticleClass(particle_array->particle[itr].particle_classname);
		}
		particle_array->classes_found = UUcTrue;
	}
}

static void ONiCharacterClass_SetupImpactArray(ONtCharacterClass *inClass)
{
	ONtCharacterImpactArray	*impact_array = inClass->impacts;
	UUtUns32 itr;

	if ((impact_array != NULL) && (!(impact_array->indices_found))) {
		// the character class hasn't found its impact type or modifier indices yet. find them.
		for (itr = 0; itr < impact_array->numImpacts; itr++) {
			impact_array->impact[itr].impact_type = MArImpactType_GetByName(impact_array->impact[itr].impact_name);
			impact_array->impact[itr].modifier_type = ONrImpactEffect_GetModifierByName(impact_array->impact[itr].modifier_name);
		}
		impact_array->indices_found = UUcTrue;
	}
}

static void ONiCharacterClass_SetupAnimationImpacts(ONtCharacterClass *inClass)
{
	ONtCharacterAnimationImpact *impact;
	UUtUns32 itr;

	if (!(inClass->anim_impacts.indices_found)) {
		// the character class hasn't found its impact type or modifier indices yet. find them.
		inClass->anim_impacts.modifier_type = ONrImpactEffect_GetModifierByName(inClass->anim_impacts.modifier_name);

		impact = &inClass->anim_impacts.impact[0];
		for (itr = 0; itr < ONcAnimationImpact_Max; itr++, impact++) {
			impact->impact_type = MArImpactType_GetByName(impact->impact_name);
		}
		inClass->anim_impacts.indices_found = UUcTrue;
	}
}

static void ONiCharacterClass_SetupHurtSounds(ONtCharacterClass *inClass)
{
	ONtPainConstants *pain_constants = &inClass->painConstants;

	if (!pain_constants->sounds_found) {
		// the character class hasn't found its hurt sounds yet.
		pain_constants->hurt_light	= OSrImpulse_GetByName(pain_constants->hurt_light_name);
		pain_constants->hurt_medium	= OSrImpulse_GetByName(pain_constants->hurt_medium_name);
		pain_constants->hurt_heavy	= OSrImpulse_GetByName(pain_constants->hurt_heavy_name);
		pain_constants->death_sound	= OSrImpulse_GetByName(pain_constants->death_sound_name);

		pain_constants->sounds_found = UUcTrue;
	}
}

static void ONiCharacterClass_PrepareForUse(ONtCharacterClass *inClass)
{
	// class's particle and impact arrays must be set up before we can use it
	inClass->death_particleclass = P3rGetParticleClass(inClass->death_effect);

	ONiCharacterClass_SetupParticleArray(inClass);
	ONiCharacterClass_SetupImpactArray(inClass);
	ONiCharacterClass_SetupAnimationImpacts(inClass);
	ONiCharacterClass_SetupHurtSounds(inClass);
}

void ONrCharacter_SetCharacterClass(
	ONtCharacter *ioCharacter,
	ONtCharacterClass *inClass)
{
	ONtActiveCharacter *active_character;
	ONtCharacterClass *prev_class;
	UUmAssert(NULL != ioCharacter);
	UUmAssert(NULL != inClass);

	active_character = ONrGetActiveCharacter(ioCharacter);

	if (active_character != NULL) {
		// we must shut down anything specifically related to our previous character class
		if (active_character->superParticlesCreated) {
			ONiCharacter_SuperParticle_Delete(ioCharacter, active_character);
		}
	}

	{
		const char *instance_name = TMrInstance_GetInstanceName(inClass);

		if (NULL == inClass->variant) {
			COrConsole_Printf("%s had a NULL variant", instance_name);
			return;
		}

		if (NULL == OSrVariantList_Variant_GetByVariant(inClass->variant)) {
			COrConsole_Printf("%s failed on OSrVariantList_Variant_GetByVariant", instance_name);
			return;
		}

		if (NULL == inClass->animations) {
			COrConsole_Printf("%s had a NULL animation collection", instance_name);
			return;
		}

		if (NULL == inClass->body) {
			COrConsole_Printf("%s had a NULL body set", instance_name);
			return;
		}
	}

	ONiCharacterClass_PrepareForUse(inClass);

	prev_class = ioCharacter->characterClass;
	ioCharacter->characterClass = inClass;

	if (active_character != NULL) {
		if (ioCharacter->characterClass->leftHanded) {
			ONrCharacter_SetHand(active_character->extraBody, ONcCharacterIsLeftHanded);
		}
		else {
			ONrCharacter_SetHand(active_character->extraBody, ONcCharacterIsRightHanded);
		}

		if (prev_class->animations != inClass->animations) {
			ONrCharacter_ResetAnimation(ioCharacter, active_character, UUcFalse);
		}

		ONrCharacter_SetWeaponVarient(ioCharacter, active_character);

		ioCharacter->scale = UUmRandomRangeFloat(inClass->scaleLow, inClass->scaleHigh);
	}

	return;
}

static float iMoveTowardsZero(float inNumber, float inAmtTowardsZero)
{
	float result;

	if (inNumber > inAmtTowardsZero) {
		result = inNumber - inAmtTowardsZero;
	}
	else if (inNumber < -inAmtTowardsZero) {
		result = inNumber + inAmtTowardsZero;
	}
	else {
		result = 0;
	}

	return result;
}


static void ONiCharacter_DoAiming_Location(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	const float moveAmt = 0.1f;
	float height,len;
	M3tVector3D toTarget;
	M3tPoint3D shoulder;

	shoulder = ioCharacter->location;
	shoulder.y += ONcCharacterHeight;
	MUmVector_Subtract(toTarget, shoulder, ioActiveCharacter->aimingTarget);

	if (MUmVector_IsZero(toTarget)) {
		ioActiveCharacter->isAiming = UUcFalse;
	}
	else {
		len = MUmVector_GetLength(toTarget);	// Need this later
		height = toTarget.y;
		MUmVector_Scale(toTarget,1.0f/len);
	}

	if (ioActiveCharacter->isAiming) {
		float lr_angle_to_target;
		float ud_angle_to_target;

		if (UUmSQR(toTarget.x) + UUmSQR(toTarget.z) < 1e-03f) {
			lr_angle_to_target = ioCharacter->facing;
		} else {
			lr_angle_to_target = -(M3cHalfPi + MUrATan2(toTarget.z, toTarget.x));
		}

		ud_angle_to_target = MUrASin((float)fabs(height) / len);

		if (toTarget.y > 0) {
			ud_angle_to_target = -ud_angle_to_target;
		}

		ioActiveCharacter->aimingUD = ud_angle_to_target;
		ioActiveCharacter->aimingLR = lr_angle_to_target - ioCharacter->facing;

		if (ioActiveCharacter->aimingLR > M3cPi) {
			ioActiveCharacter->aimingLR -= M3c2Pi;
		}
		else if (ioActiveCharacter->aimingLR < -M3cPi) {
			ioActiveCharacter->aimingLR += M3c2Pi;
		}

		ioActiveCharacter->aimingVector = toTarget;
		MUmVector_Negate(ioActiveCharacter->aimingVector);

		ioActiveCharacter->cameraVector = ioActiveCharacter->aimingVector;

		ioActiveCharacter->aimingVectorLR = ioActiveCharacter->aimingLR + ioCharacter->facing;
		ioActiveCharacter->aimingVectorUD = ioActiveCharacter->aimingUD;

		UUmTrig_Clip(ioActiveCharacter->aimingVectorLR);
		UUmTrig_ClipLow(ioActiveCharacter->aimingVectorUD);
	}
	else {
		ioActiveCharacter->aimingLR = 0;
		ioActiveCharacter->aimingUD = 0;

		ioActiveCharacter->aimingVectorLR = ioCharacter->facing;
		ioActiveCharacter->aimingVectorUD = 0;

		ioActiveCharacter->aimingVector.x = MUrSin(ioActiveCharacter->aimingVectorLR);
		ioActiveCharacter->aimingVector.y = 0.f;
		ioActiveCharacter->aimingVector.z = MUrCos(ioActiveCharacter->aimingVectorLR);

		ioActiveCharacter->cameraVector = ioActiveCharacter->aimingVector;
	}

	return;
}


static void ONiCharacter_DoAiming_Vector(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	const float moveAmt = 0.1f;
	float angleLR, angleUD, costheta, sintheta, cosphi, sinphi;
	float finalRotation;
	// ok in this case aimingLR and aimingUD are the given parts
	// from these we build aimingVector

	// pin aiminLR and aimingUD
	ioActiveCharacter->aimingLR = UUmPin(ioActiveCharacter->aimingLR, -(M3c2Pi / 4.f), (M3c2Pi / 4.f));
	ioActiveCharacter->aimingUD = UUmPin(ioActiveCharacter->aimingUD, -(M3c2Pi / 4.f), (M3c2Pi / 4.f));

	angleLR = ioActiveCharacter->aimingLR;
	UUmTrig_ClipLow(angleLR);

	angleUD = ioActiveCharacter->aimingUD;
	UUmTrig_ClipLow(angleUD);

	angleLR += ONrCharacter_GetCosmeticFacing(ioCharacter);
	UUmTrig_ClipHigh(angleLR);

	finalRotation = (ioActiveCharacter->animation == NULL) ? 0 : TRrAnimation_GetFinalRotation(ioActiveCharacter->animation);
	if (finalRotation != 0.f)
	{
		float multiplier;

		if (finalRotation > M3cPi) {
			finalRotation -= M3c2Pi;
		}

		multiplier = ((float)ioActiveCharacter->animFrame) / ((float) TRrAnimation_GetDuration(ioActiveCharacter->animation));

		finalRotation *= multiplier;

		angleLR += finalRotation;
		UUmTrig_Clip(angleLR);
	}

	// step 3 build the vector from the angles
	costheta = MUrCos(angleLR); sintheta = MUrSin(angleLR);
	cosphi = MUrCos(angleUD); sinphi = MUrSin(angleUD);

	// CB: this is the original code that was here which computes an incorrect aiming vector.
	// when I fixed the calculation of aimingVector to let the AI aim correctly, it was necessary to
	// leave this incorrect computation here, because using the correct equation would change
	// the behavior of the camera and we don't want to do that. all hail the glory of the aiming code.
	//
	// -- chris, 15 september 2000
	ioActiveCharacter->cameraVector.x = sintheta;
	ioActiveCharacter->cameraVector.y = sinphi;
	ioActiveCharacter->cameraVector.z = costheta;
	MUmVector_Normalize(ioActiveCharacter->cameraVector);

	// this is the correct aiming vector computation
	ioActiveCharacter->aimingVector.x = sintheta * cosphi;
	ioActiveCharacter->aimingVector.y = sinphi;
	ioActiveCharacter->aimingVector.z = costheta * cosphi;
	UUmAssert(MUmVector_IsNormalized(ioActiveCharacter->aimingVector));

	ioActiveCharacter->aimingVectorLR = angleLR;
	ioActiveCharacter->aimingVectorUD = angleUD;
}

void ONrCharacter_DoAiming(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	if (ONrCharacter_IsAI(ioCharacter) && !ONrCharacter_IsPlayingFilm(ioCharacter)) {
		ONiCharacter_DoAiming_Location(ioCharacter, ioActiveCharacter);
	} else {
		ONiCharacter_DoAiming_Vector(ioCharacter, ioActiveCharacter);
	}
}

static UUtError
iDrawDot(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{

	if (0 == inParameterListLength) {
		ONgDrawDot = UUcFalse;
	}
	else if (3 == inParameterListLength) {
		float buffer[3];
		UUtInt32 itr;

		ONgDrawDot = UUcTrue;

		for(itr = 0; itr < 3; itr++) {
			if (SLcType_Int32 == inParameterList[itr].type) {
				buffer[itr] = (float) inParameterList[itr].val.i;
			}
			else if (SLcType_Float == inParameterList[itr].type) {
				buffer[itr] = inParameterList[itr].val.f;
			}
			else {
				ONgDrawDot = UUcFalse;
			}
		}

		ONgDot.x = buffer[0];
		ONgDot.y = buffer[1];
		ONgDot.z = buffer[2];
	}

	return UUcError_None;
}

static UUtError
iCharacterLocation(
	ONtCharacter*		ioCharacter,
	M3tPoint3D*			inDestination)
{
	COrConsole_Printf("%s %s %3.1f %3.1f %3.1f", ioCharacter->player_name, (inDestination == NULL) ? "at" : "old",
						ioCharacter->location.x, ioCharacter->location.y, ioCharacter->location.z);

	if (inDestination != NULL) {
		ONrCharacter_Teleport(ioCharacter, inDestination, UUcTrue);

		COrConsole_Printf("%s new %3.1f %3.1f %3.1f", ioCharacter->player_name,
							ioCharacter->location.x, ioCharacter->location.y, ioCharacter->location.z);
	}

	return UUcError_None;
}

static UUtError
iSetAnyCharacterLocation(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	int whichCharacter;
	M3tPoint3D newLocation;
	UUtBool settingLocation = 4 == inParameterListLength;

	whichCharacter = inParameterList[0].val.i;
	if ((whichCharacter < 0) || (whichCharacter > ONgGameState->numCharacters)) {
		goto errorExit;
	}

	if ((ONgGameState->characters[whichCharacter].flags & ONcCharacterFlag_InUse) == 0) {
		goto errorExit;
	}

	if (settingLocation)
	{
		newLocation.x = inParameterList[1].val.f;
		newLocation.y = inParameterList[2].val.f;
		newLocation.z = inParameterList[3].val.f;
	}

	iCharacterLocation(&ONgGameState->characters[whichCharacter], (settingLocation) ? &newLocation : NULL);

	return UUcError_None;

errorExit:
	COrConsole_Printf("bad parameters on chr_location");

	return UUcError_None;
}

static UUtError
iSetPlayerLocation(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	ONtCharacter *player;
	M3tPoint3D newLocation, *destination = NULL;

	player = ONrGameState_GetPlayerCharacter();

	if (inParameterListLength >= 3) {
		newLocation.x = inParameterList[0].val.f;
		newLocation.y = inParameterList[1].val.f;
		newLocation.z = inParameterList[2].val.f;
		destination = &newLocation;
	}

	iCharacterLocation(player, destination);
	return UUcError_None;
}

static UUtError
iSetAnyCharacterHealth(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	int whichCharacter;
	UUtUns32 newHP;
	UUtBool settingHealth = 2 == inParameterListLength;
	UUtBool readingHealth = 1 == inParameterListLength;
	ONtCharacter *targetCharacter = NULL;

	if (!settingHealth && !readingHealth) {
		goto errorExit;
	}

	whichCharacter = inParameterList[0].val.i;

	if (settingHealth) {
		newHP = inParameterList[1].val.i;

		if (inParameterList[1].type != SLcType_Int32) {
			goto errorExit;
		}
	}

	if ((whichCharacter < 0) || (whichCharacter > ONgGameState->numCharacters)) {
		goto errorExit;
	}

	if (whichCharacter >= ONgGameState->numCharacters) {
		COrConsole_Printf("invalid character");
		return UUcError_Generic;
	}

	targetCharacter = ONgGameState->characters + whichCharacter;
	if (0 == (targetCharacter->flags & ONcCharacterFlag_InUse)) {
		COrConsole_Printf("invalid character (already deleted)");
	}

	COrConsole_Printf("char %d old health %u", whichCharacter, targetCharacter->hitPoints);

	if (settingHealth) {
		ONrCharacter_SetHitPoints(targetCharacter, newHP);

		COrConsole_Printf("char %d new health %u", whichCharacter, targetCharacter->hitPoints);
	}

	return UUcError_None;

errorExit:
	COrConsole_Printf("bad parameters on chr_health");

	return UUcError_Generic;
}

static UUtError
SetMainCharacter(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	int which_character_index;

	which_character_index = inParameterList[0].val.i;

	if (which_character_index < ONgGameState->numCharacters) {
		ONrGameState_SetPlayerNum(which_character_index);
		CAgCamera.star = ONgGameState->characters + which_character_index;
	}

	return UUcError_None;
}


static UUtError
iSetAnyCharacterWeapon(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	int whichCharacter = -1;

	ONtCharacter **present_character_list = ONrGameState_PresentCharacterList_Get();
	UUtUns32 present_character_count = ONrGameState_PresentCharacterList_Count();
	UUtUns32 itr;

	whichCharacter = inParameterList[0].val.i;

	if ((whichCharacter < -1) || (whichCharacter > ONgGameState->numCharacters)) {
		return UUcError_Generic;
	}

	for(itr = 0; itr < present_character_count; itr++)
	{
		ONtCharacter *character = present_character_list[itr];

		if ((character->index == whichCharacter) || (-1 == whichCharacter)) {
			ONrCharacter_DropWeapon(character,UUcFalse,WPcPrimarySlot,UUcFalse);
		}
	}

	return UUcError_None;
}

static UUtError
iCharacterLocation_SetFromCamera(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	int 			whichCharacter;
	M3tPoint3D		cameraLocation;

	M3rCamera_GetViewData(ONgActiveCamera, &cameraLocation, NULL, NULL);

	whichCharacter = inParameterList[0].val.i;

	if ((whichCharacter < 0) || (whichCharacter > ONgGameState->numCharacters))
	{
		return UUcError_None;
	}

	if ((ONgGameState->characters[whichCharacter].flags & ONcCharacterFlag_InUse) == 0)
	{
		return UUcError_None;
	}

	iCharacterLocation(&ONgGameState->characters[whichCharacter], &cameraLocation);
	return UUcError_None;
}

static UUtError ONrBuildParticleClasses(void)
{
	return UUcError_None;
}

UUtError ONrCharacter_LevelBegin(void)
{
	UUtError error;

	ONgPlayerFistsOfFury = UUcFalse;
	ONgPlayerKnockdownResistant = UUcFalse;
	ONgPlayerInvincible = UUcFalse;
	ONgPlayerOmnipotent = UUcFalse;
	ONgPlayerUnstoppable = UUcFalse;

	ONrCorpse_LevelBegin();

	ONgBigHead = UUcFalse;
	ONgMarketing_Line_Off = UUcFalse;

	error = ONrBuildParticleClasses();
	UUmError_ReturnOnError(error);

	// clear the local character
	ONrGameState_SetPlayerNum(UUcMaxUns16);

	// mark us having no characters
	ONgGameState->numCharacters = 0;
	ONgGameState->usedActiveCharacters = 0;

	// for safety's sake clear the characters
	UUrMemory_Clear(
		ONgGameState->characters,
		sizeof(ONtCharacter) * ONcMaxCharacters);

	UUrMemory_Clear(
		ONgGameState->activeCharacterStorage,
		sizeof(ONtActiveCharacter) * ONcMaxActiveCharacters);

	// reset character salt
	ONgGameState->nextCharacterSalt = 1;
	ONgGameState->nextActiveCharacterSalt = 1;
	// ONrCharacter_DumpAllCollections();

	// make sure all character classes have variants
	{
		ONtCharacterClass			*character_class_list[2048];
		UUtUns32					num_classes;
		UUtUns32					i;

		error =
			TMrInstance_GetDataPtr_List(
				TRcTemplate_CharacterClass,
				2048,
				&num_classes,
				character_class_list);
		UUmAssert(error == UUcError_None);

		for (i = 0; i < num_classes; i++)
		{
			const char				*name;

			name = TMrInstance_GetInstanceName(character_class_list[i]);
			if (character_class_list[i]->variant == NULL) {
				UUrDebuggerMessage("ERROR: character class %s has no variant!\n", name);
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "character class without variant, see debugger.txt");
			}
		}
	}

	return UUcError_None;
}

UUtError ONrCharacter_LevelEnd(void)
{
	UUtUns16 itr;

	for(itr = 0; itr < ONgGameState->numCharacters; itr++) {
		ONrGameState_DeleteCharacter(ONgGameState->characters + itr);
	}

	return UUcError_None;
}


UUtError
ONrGameState_InstallConsoleVariables(void)
{
	SLtRegisterBoolTable bool_table[] =
	{
#if CONSOLE_DEBUGGING_COMMANDS
		{ "spatial_footsteps", "spatial footsteps", &ONgSpatialFootsteps },
		{ "chr_disable_visactive", "disable visibility activation", &ONgDisableVisActive },
		{ "chr_all_active", "forces all characters to be active", &ONgCharactersAllActive },
		{ "chr_debug_footsteps", "footstep debugging mode", &ONgDebugFootsteps },
		{ "chr_debug_footsteps_verbose", "verbose footstep debugging mode", &ONgDebugVerboseFootsteps },
		{ "footstep_flash", "footstep flash mode", &ONgDebugFootstepFlash },
		{ "door_drawframes", "draws doorframes for all unplaced doors", &OBJgDoor_DrawFrames },
		{ "debug_triggers", "enable trigger volume debugging", &gDebugTriggerVolume },
		{ "debug_scripts", "enable script debugging", &gDebugScripts },
		{ "debug_gun_behavior", "enable AI gun behavior debugging", &gDebugGunBehavior },
		{ "chr_weapon_auto_aim", "enables auto aiming", &gWeaponAutoAim },
		{ "saved_film_loop", "saved_film_loop", &saved_film_loop },
		{ "chr_debug_pathfinding", "control pathfinding debugging", &gChrDebugPathfinding },
		{ "chr_debug_fall", "controls debugging falling", &gChrDebugFall },
		{ "debug_impacts", "prints impacts as they occur", &ONgDebugImpactEffects },
		{ "chr_collision_box", "toggles character bounding box / bounding sphere collision", &ONgCharacterCollision_BoxEnable },
		{ "chr_debug_trigger_quad", "controls trigger quad debugging", &gShowTriggerQuad },
		{ "chr_draw_weapon", "controls drawing of weapons", &gDrawWeapon },
		{ "chr_pin_character", "pins a character to the ground", &gPinCharacters },
		{ "chr_draw_all_characters", "forces the drawing of all characters", &gDrawAllCharacters },
		{ "chr_mini_me", "Draws the main character small", &ONgMiniMe },
		{ "show_chr_env_collision", "Draws character / environment collisions.", &gConsole_show_chr_env_collision },
		{ "show_laser_env_collision", "Draws laser / environment collisions.", &gConsole_show_laser_env_collision },
		{ "chr_corpse_fade_offscreen", "Makes corpses fade when they are offscreen.", &gCorpseFadeOffscreen },
		{ "gs_show_shadows", "Enables character shadows", &ONgCharacterShadow },
		{ "chr_debug_impacts", "Prints debugging info about character melee impacts", &ONgDebugCharacterImpacts },
		{ "chr_debug_aiming_screen", "turns on aiming screen debugging", &ONgShowAimingScreen },
		{ "chr_enable_collision", "Enables character collision", &gDoCollision },
		{ "chr_active", "Enables character activity", &gPauseCharacters },
		{ "chr_draw_facing_vector", "Enables drawing of the facing vector", &gDrawFacingVector },
		{ "chr_draw_aiming_vector", "Enables drawing of the aiming vector", &gDrawAimingVector },
		{ "chr_debug_characters", "Enables debugging information for characters", &gDebugCharacters },
		{ "chr_debug_overlay", "Enables debugging information for overlays", &gDebugOverlay },
		{ "chr_debug_sphere", "Enables sphere tree debugging for characters", &gDebugCharactersSphereTree },
		{ "chr_print_sound", "Turns on printing of the sound effect", &gPrintSound },
		{ "chr_debug_collision", "Turns on/off debugging of collision", &gDebugCollision },
		{ "chr_disable_melee", "Turns off all melee damage", &ONgNoMeleeDamage },
		{ "wp_force_scale", "", &ONgWpForceScale },
		{ "wp_force_half_scale", "", &ONgWpForceHalfScale },
		{ "wp_force_no_scale", "", &ONgWpForceNoScale },
		{ "gs_show_corpses", "", &ONgShow_Corpses },
#endif
		{ "marketing_line_off", "turns the laser line off", &ONgMarketing_Line_Off },
		{ "chr_big_head", "Draws everyone with a big head", &ONgBigHead },
		{ NULL, NULL, NULL }
	};

	SLtRegisterInt32Table int32_table[] =
	{
#if CONSOLE_DEBUGGING_COMMANDS
		{ "saved_film_character_offset", "saved_film_character_offset", &saved_film_character_offset },
		{ "chr_death_fade_start", "the number of game ticks until charaters begin to fade", &ONgDeathFadeStart },
		{ "chr_death_fade_frames", "the number of game ticks until charaters fade over", &ONgDeathFadeFrames },
		{ "chr_lod", "forces a given lod if it is between 0 and 4", &ONgForceLOD },
		{ "chr_show_movement", "Enables movement debugging information for characters", &gDebugCharacterMovement },
		{ "chr_debug_target", "Selects the character debugging target", &gDebugCharacterTarget },
#endif
		{ NULL, NULL, NULL }
	};

	SLtRegisterFloatTable float_table[] =
	{
#if CONSOLE_DEBUGGING_COMMANDS
		{ "cm_lookspring_percent", "at what percent of lookspring fight mode turns off", &ONgLookspringFightModePercent },
		{ "chr_collision_grow", "sets character bounding box collision grow amount", &ONgCharacterCollision_Grow },
		{ "chr_block_angle", "controls the angle at which the characters can block", &ONgBlockAngle },
		{ "chr_aim_width", "number of degress of aiming width", &ONgAimingWidth },
		{ "chr_auto_aim_arc", "number of degress of auto aiming arc", &ONgAutoAim_Arc },
		{ "chr_auto_aim_dist", "distance for auto aiming", &ONgAutoAim_Distance },
		{ "chr_big_head_amount", "Controls the size of the big head", &ONgBigHeadAmount },
		{ "chr_mini_me_amount", "Controls the size of the main character", &ONgMiniMe_Amount },
		{ "wp_scale_adjustment", "", &ONgWpScaleAdjustment },
		{ "wp_pow_adjustment", "", &ONgWpPowAdjustment },
#endif
		{ NULL, NULL, NULL }
	};

	SLtRegisterVoidFunctionTable scripting_function_table[] =
	{
#if CONSOLE_DEBUGGING_COMMANDS
		{ "chr_focus", "Selects what character to control", "chr_index:int", SetMainCharacter },
		{ "chr_health", "Sets character's health", "chr_index:int [hit_points:int | ]", iSetAnyCharacterHealth },
		{ "chr_disarm", "Disarms a character or everyone", "chr_index:int", iSetAnyCharacterWeapon },
		{ "goto", "Sets the location of the player character", "[[loc_x:float loc_y:float loc_z:float] | ]", iSetPlayerLocation },
		{ "chr_location", "Sets the location of any character", "chr_index:int [[loc_x:float loc_y:float loc_z:float] | ]", iSetAnyCharacterLocation },
		{ "chr_location_settocamera", "Sets the location of any character to the camera location", "chr_index:int", iCharacterLocation_SetFromCamera },
		{ "chr_main_class", "Sets the main characters class", "[class_name:string | class_index:int]", iSetMainCharacterClass },
		{ "chr_display_combat_stats", "Displays the characters combat stats", "chr_index:int", iDiplayCharacterCombatStats },
		{ "chr_set_class", "Sets the character class of a specific character", "chr_index:int [class_name:string | class_index:int]", iSetCharacterClass },
		{ "chr_weapon", "Sets the weapon for a give character", "chr_index:int [weapon_name:string | weapon_num:int]", iSetCharacterWeapon },
		{ "chr_draw_dot", "draws a dot at a specified location", NULL, iDrawDot },
		{ "chr_kill_all_ai", "kills all the AI", NULL, ONrKillAllAI },
		{ "fall_front", "makes the player character fall front", NULL, ONrFallFront },
		{ "fall_back", "makes the player character fall front", NULL, ONrFallBack },
		{ "make_corpse", "makes a corpse", "corpse_name:string", ONrMakeCorpse },
		{ "corpse_reset", "resets corpses to their initial state", "", ONrCorpse_Reset },
		{ "chr_who", "lists all the players", NULL, ONrWho },
		{ "print_type", "prints an anim type ", "type:int", PrintType },
		{ "print_state", "prints an anim state", "state:int", PrintState },
		{ "crash", "crashes the game (used for testing error handling)", "when:string{\"now\"}", DebugCrash },
		{ "hang", "hangs the game (used for testing error handling)", "when:string{\"now\"}", DebugHang },
#endif
		{ NULL, NULL, NULL, NULL }
	};

	TRrInstallConsoleVariables();

	SLrGlobalVariable_Register_Bool_Table(bool_table);
	SLrGlobalVariable_Register_Int32_Table(int32_table);
	SLrGlobalVariable_Register_Float_Table(float_table);
	SLrScript_CommandTable_Register_Void(scripting_function_table);

#if CONSOLE_DEBUGGING_COMMANDS
	SLrGlobalVariable_Register_Bool("chr_show_bnv", "shows the bnv of the main character", &gShowBNV);
	SLrGlobalVariable_Register_Bool("chr_show_lod", "shows the current lod of the main character", &gShowLOD);
#endif

	COrConsole_StatusLines_Begin(ONgBNVStatus, &gShowBNV, 2);
	strcpy(ONgBNVStatus[0].text, "*** character bnv index ***");

	COrConsole_StatusLines_Begin(ONgChrLODStatus, &gShowLOD, 2);
	strcpy(ONgChrLODStatus[0].text, "*** character lod ***");

	COrConsole_StatusLines_Begin(ONgChrStatus, &gDebugCharacters, 9);
	strcpy(ONgChrStatus[0].text, "*** character status ***");

	COrConsole_StatusLines_Begin(ONgChrOverlayStatus, &gDebugOverlay, ONcOverlayIndex_Count + 1);
	strcpy(ONgChrStatus[0].text, "*** character status ***");

#if DEBUG_ATTACKEXTENT
	// CB: temporary debugging
	error =
	SLrScript_Command_Register_Void(
		"chr_showextent",
		"shows the attack extent of an animation",
		"anim:string",
		ONiTempDebug_AttackExtent);
	UUmError_ReturnOnError(error);

	error =	SLrGlobalVariable_Register_Bool("chr_showextent_globals", "shows the global parts of an attack animation's extent",
						&ONgTempDebug_AttackExtent_ShowGlobalExtents);
	UUmError_ReturnOnError(error);
#endif

#if PERFORMANCE_TIMER
	ONg_TriggerVolume_Timer= UUrPerformanceTimer_New("CH_TriggerVolume");
#endif

	return UUcError_None;
}

UUtUns32 ONrCharacter_FindFloorWithRay(ONtCharacter *ioCharacter, UUtUns32 inFramesOfgravity, M3tPoint3D *inPoint, M3tVector3D *outPoint)
{
	UUtUns32				gq_index;
	float					gravity;
	M3tPoint3D				ray_point;
	M3tVector3D				vector;
	const ONtAirConstants *	airConstants;

	// cast a ray down to try and find ground below the character, up to their height
	ray_point = *inPoint;
	ray_point.y += ONcCharacterSnapHeight;

	airConstants = ONrCharacter_GetAirConstants(ioCharacter);
	gravity = -airConstants->fallGravity;
	gravity *= UUmMax(1, inFramesOfgravity);
	MUmVector_Set(vector, 0.0f, -ONcCharacterSnapHeight + gravity, 0.0f);

	gq_index = (UUtUns32) -1;
	if (AKrCollision_Point(ONrGameState_GetEnvironment(), &ray_point, &vector, AKcGQ_Flag_Chr_Col_Skip | AKcGQ_Flag_Wall, AKcMaxNumCollisions)) {
		*outPoint = AKgCollisionList[0].collisionPoint;
		gq_index = AKgCollisionList[0].gqIndex;
	}

	return gq_index;
}

UUtBool ONrCharacter_SnapToFloor(ONtCharacter *ioCharacter)
{
	UUtUns32				floor_gq;
	ONtActiveCharacter *	active_character = ONrGetActiveCharacter(ioCharacter);

	// first, try to find a ground collision and snap to that
	if ((active_character != NULL) && (FindGroundCollision(ioCharacter, active_character)))
		return UUcTrue;

	floor_gq = ONrCharacter_FindFloorWithRay(ioCharacter, 0, &ioCharacter->location, &ioCharacter->location);
	if (floor_gq != (UUtUns32) -1) {
		// move the character down to the location that we found
		ioCharacter->actual_position = ioCharacter->location;
		ioCharacter->actual_position.y += PHcWorldCoord_YOffset;

		if (active_character != NULL) {
			if (active_character->physics != NULL) {
				active_character->physics->position = ioCharacter->location;
			}
			ONrCharacter_BuildMatriciesAll(ioCharacter, active_character);
		}
		return UUcTrue;
	}

	return UUcFalse;
}

static UUtUns16
ONiGameState_GetFreeCharacter(
	void)
{
	UUtUns16				i;

	for (i = 0; i < ONcMaxCharacters; i++)
	{
		if ((ONgGameState->characters[i].flags & ONcCharacterFlag_InUse) == 0)
		{
			return i;
		}
	}

	return ONcMaxCharacters;
}

static UUtUns16
ONiGameState_GetFreeActiveCharacter(
	void)
{
	UUtUns16				i;

	for (i = 0; i < ONcMaxActiveCharacters; i++) {
		if (ONgGameState->activeCharacterStorage[i].index == ONcCharacterIndex_None) {
			return i;
		}
	}

	return ONcMaxActiveCharacters;
}

UUtError
ONrGameState_NewCharacter(
	const OBJtObject			*inStartPosition,
	const AItCharacterSetup		*inSetup,
	const ONtFlag				*inFlag,
	UUtUns16					*outCharacterIndex)
{
	ONtCharacterClass		*variants[ONcCharacter_MaxVariants];
	ONtCharacterClass		*test_class;
	ONtCharacter			*thisCharacter;
	OBJtOSD_Character		*char_osd;
	ONtCharacterIndexType	index;
	UUtBool					default_melee_profile = UUcTrue;
	UUtUns16				salt;
	UUtInt32				hp;
	UUtError				error;
	UUtUns32				num_variants;
	UUtUns16				itr;
	ONtFlag					flag;
	float					health_multiplier;
	ONtDifficultyLevel		difficulty;
	const char				*desired_variant;

	UUmAssert(inStartPosition || inSetup);

	/****** memory allocation */

	// set the character's index
	index = ONiGameState_GetFreeCharacter();
	if (index >= ONcMaxCharacters)
	{
		return UUcError_Generic;
	}

	// set the number of characters in the gamestate
	if (index >= ONgGameState->numCharacters)
	{
		ONgGameState->numCharacters = index + 1;
	}

#if DEBUG_CHARACTER_CREATION
	UUrDebuggerMessage("create character %d\n", index);
#endif

	// get a pointer to the character
	thisCharacter = ONgGameState->characters + index;
	UUrMemory_Clear(thisCharacter, sizeof(*thisCharacter));

	/****** initialize constant variables */

	// calculate the pseudo-unique character index
	salt = (UUtUns16) ((ONgGameState->nextCharacterSalt++) & ((1 << ONcCharacterSalt_Bits) - 1));

	thisCharacter->index = (index & ONcCharacterIndexMask_RawIndex);
	thisCharacter->index |= (salt << ONcCharacterSalt_Shift);
	thisCharacter->index |= ONcCharacterIndexMask_Valid;

	thisCharacter->active_index = ONcCharacterIndex_None;
	thisCharacter->flags = ONcCharacterFlag_InUse;

	thisCharacter->blockFunction = NULL;
	thisCharacter->facingModifier = 0;
	thisCharacter->animCounter = 0;
	thisCharacter->lastActiveAnimationTime = 0;
	thisCharacter->heightThisFrame = ONcCharacterHeight / 2.0f;		// initial approximation, will be set if we run any animation
	thisCharacter->damageInflicted = 0;
	thisCharacter->numKills = 0;
	thisCharacter->time_of_death = 0;
	thisCharacter->num_frames_offscreen = 1000;						// so that characters don't immediately get activated
	thisCharacter->poisonLastFrame = 0;
	thisCharacter->poisonFrameCounter = 0;

	ONrSpeech_Initialize(thisCharacter);
	thisCharacter->neutral_interaction_char = NULL;
	thisCharacter->neutral_keyabort_frames = 0;
	thisCharacter->melee_fight = NULL;

	/****** character starting position */

	// Is there a valid AI2 character start position?
	if (inStartPosition == NULL) {
		char_osd = NULL;
	} else if (inStartPosition->object_type != OBJcType_Character) {
		UUrDebuggerMessage("ONrGameState_NewCharacter: object type is not character");
		inStartPosition = NULL;
		char_osd = NULL;
	} else {
		char_osd = (OBJtOSD_Character *) inStartPosition->object_data;
	}

	if (inStartPosition != NULL) {
		/*
		 * get initial state from the character's start-position object
		 */

		// get the character class
		error = TMrInstance_GetDataPtr(TRcTemplate_CharacterClass, char_osd->character_class,
										&thisCharacter->characterClass);
		if (error == UUcError_None) {
			// check for random costuming
			num_variants = 0;
			default_melee_profile = (char_osd->melee_id == thisCharacter->characterClass->ai2_behavior.default_melee_id);

			if (char_osd->flags & (OBJcCharFlags_RandomCostume | OBJcCharFlags_UpgradeDifficulty)) {
				// either look for random people of our variant, or the next harder variant
				desired_variant = thisCharacter->characterClass->variant->name;

				if ((char_osd->flags & OBJcCharFlags_UpgradeDifficulty) &&
					(ONrPersist_GetDifficulty() >= ONcDifficultyLevel_Hard) &&
					(thisCharacter->characterClass->variant->upgrade_name[0] != '\0')) {
					desired_variant = thisCharacter->characterClass->variant->upgrade_name;
				}

				for (itr = 0; itr < UUcMaxUns16; itr++) {
					error = TMrInstance_GetDataPtr_ByNumber(TRcTemplate_CharacterClass, itr, &test_class);
					if (error != UUcError_None)
						break;

					if (UUmString_IsEqual(test_class->variant->name, desired_variant)) {
						// this is a variant of the desired character class
						variants[num_variants++] = test_class;
					}
				}

				if (num_variants == 0) {
					COrConsole_Printf("ERROR: character '%s' variant '%s' couldn't find any classes of %svariant '%s'",
						char_osd->character_name, thisCharacter->characterClass->variant->name,
						(desired_variant == thisCharacter->characterClass->variant->upgrade_name) ? "upgrade " : "", desired_variant);
				} else {
					// pick a random class from the list that matched the desired variant
					itr = (UUtUns16) ((num_variants * UUrLocalRandom()) >> 16);
					UUmAssert((itr >= 0) && (itr < num_variants));

					thisCharacter->characterClass = variants[itr];
				}
			}
		} else {
			// get a default character class
			error = TMrInstance_GetDataPtr_ByNumber(TRcTemplate_CharacterClass, 0, &thisCharacter->characterClass);
			UUmError_ReturnOnError(error);
		}

		// set the character's name
		UUrString_Copy(thisCharacter->player_name, char_osd->character_name, ONcMaxPlayerNameLength);

		// set the character's weapon
		WPrInventory_Reset(thisCharacter->characterClass, &thisCharacter->inventory);
		error = ONrCharacter_ArmByName(thisCharacter, char_osd->weapon_class_name);
		if (error != UUcError_None) {
			if ((char_osd->weapon_class_name[0] != '\0') &&
				(strcmp(char_osd->weapon_class_name, "none") != 0)) {
				UUrPrintWarning("can't find weapon class '%s'", char_osd->weapon_class_name);
			}
		} else {
			UUmAssert(thisCharacter->inventory.weapons[0] != NULL);
			WPrSetAmmoPercentage(thisCharacter->inventory.weapons[0], (UUtUns16) char_osd->ammo_percentage);
		}

		// set the character's position
		thisCharacter->location = inStartPosition->position;
		thisCharacter->actual_position = thisCharacter->location;
		thisCharacter->actual_position.y += PHcWorldCoord_YOffset;
		thisCharacter->facing = char_osd->facing;

		// set the character's hit points
		hp = thisCharacter->characterClass->class_max_hit_points + char_osd->hit_points;
		thisCharacter->hitPoints = thisCharacter->maxHitPoints = (UUtUns32) UUmMax(hp, 1);

		// set the character's team
		thisCharacter->teamNumber = (UUtUns16) char_osd->team_number;

		// set the character's inventory fields
		thisCharacter->inventory.ammo = char_osd->inventory[OBJcCharInv_AmmoBallistic][OBJcCharSlot_Use];
		thisCharacter->inventory.cell = char_osd->inventory[OBJcCharInv_AmmoEnergy][OBJcCharSlot_Use];
		thisCharacter->inventory.hypo = char_osd->inventory[OBJcCharInv_Hypo][OBJcCharSlot_Use];

		thisCharacter->inventory.drop_ammo = char_osd->inventory[OBJcCharInv_AmmoBallistic][OBJcCharSlot_Drop];
		thisCharacter->inventory.drop_cell = char_osd->inventory[OBJcCharInv_AmmoEnergy][OBJcCharSlot_Drop];
		thisCharacter->inventory.drop_hypo = char_osd->inventory[OBJcCharInv_Hypo][OBJcCharSlot_Drop];

		thisCharacter->inventory.shieldDisplayAmount = 0;
		thisCharacter->inventory.shieldRemaining = char_osd->inventory[OBJcCharInv_ShieldBelt][OBJcCharSlot_Use];
		if (thisCharacter->inventory.shieldRemaining == 1) {
			// CB: the designers probably intended this to mean "1 whole shield" not "1% of max", so we alter it to
			// a full shield
			thisCharacter->inventory.shieldRemaining = WPcMaxShields;
		}
		thisCharacter->inventory.drop_shield = (char_osd->inventory[OBJcCharInv_ShieldBelt][OBJcCharSlot_Drop] > 0);

		thisCharacter->inventory.invisibilityRemaining = char_osd->inventory[OBJcCharInv_Invisibility][OBJcCharSlot_Use];
		thisCharacter->inventory.drop_invisibility = (char_osd->inventory[OBJcCharInv_Invisibility][OBJcCharSlot_Drop] > 0);

		// if the character starts invisible, assume that they've been invisible for some time; this means that
		// the AI won't be able to see them for a brief period upon spawning even though they're completely invisible
		thisCharacter->inventory.numInvisibleFrames = (thisCharacter->inventory.invisibilityRemaining > 0) ? 600 : 0;

		thisCharacter->inventory.has_lsi = (char_osd->flags & OBJcCharFlags_HasLSI) > 0;

		// FIXME: handle LSIs
	} else if (inSetup != NULL) {
		/*
		 * get initial state from an AItCharacterSetup and a flag
		 */

		if (NULL == inFlag)
		{
			ONrLevel_Flag_ID_To_Flag(inSetup->defaultFlagID, &flag);
		}
		else
		{
			flag = *inFlag;
		}

		// Character class
		thisCharacter->characterClass = inSetup->characterClass;
		if (NULL == thisCharacter->characterClass) {
			TMrInstance_GetDataPtr_ByNumber(TRcTemplate_CharacterClass, 0, &thisCharacter->characterClass);
		}

		UUmAssert( thisCharacter->characterClass );

		// set the characters name
		if (TSrString_GetLength(inSetup->playerName) > 0)
		{
			// copy the name from the player setup
			UUrString_Copy(
				thisCharacter->player_name,
				inSetup->playerName,
				ONcMaxPlayerNameLength);
		}
		else
		{
			// Create default name
			sprintf(thisCharacter->player_name, "ai_%d", index);
		}

		// Misc
		thisCharacter->teamNumber = inSetup->teamNumber;

		thisCharacter->maxHitPoints = thisCharacter->characterClass->class_max_hit_points;
		thisCharacter->hitPoints = thisCharacter->maxHitPoints;

		// Spatial
		thisCharacter->location = flag.location;
		thisCharacter->actual_position = thisCharacter->location;
		thisCharacter->actual_position.y += PHcWorldCoord_YOffset;
		thisCharacter->facing = flag.rotation;

		// Weapons
		WPrInventory_Reset(thisCharacter->characterClass, &thisCharacter->inventory);
		ONrCharacter_UseNewWeapon(thisCharacter, inSetup->weapon);
		if (thisCharacter->inventory.weapons[0])
		{
			WPrSetAmmo(thisCharacter->inventory.weapons[0], inSetup->ammo);
		}

	} else {
		UUrDebuggerMessage("ONrGameState_NewCharacter: neither starting position or character setup specified!");
		return UUcError_Generic;
	}

	/****** AI setup */
	// set up this character to be controlled by the AI2 system
	AI2rInitializeCharacter(thisCharacter, char_osd, inSetup, &flag, default_melee_profile);

	/****** difficulty-level health modification */

	health_multiplier = 1.0f;
	difficulty = ONrPersist_GetDifficulty();
	if (thisCharacter->charType == ONcChar_Player) {
		health_multiplier = ONgGameSettings->player_hp_multipliers[difficulty];

	} else {
		// check to see if the character would be hostile to the player character
		ONtCharacter *player_character = ONrGameState_GetPlayerCharacter();
		if ((player_character != NULL) && (AI2rTeam_FriendOrFoe(thisCharacter->teamNumber, player_character->teamNumber) == AI2cTeam_Hostile)) {
			health_multiplier = ONgGameSettings->enemy_hp_multipliers[difficulty];
		}
	}

	if (health_multiplier != 1.0f) {
		thisCharacter->hitPoints = MUrUnsignedSmallFloat_To_Uns_Round(thisCharacter->hitPoints * health_multiplier);
		thisCharacter->maxHitPoints = MUrUnsignedSmallFloat_To_Uns_Round(thisCharacter->maxHitPoints * health_multiplier);
	}

	/****** character-class dependent setup */
	ONiCharacterClass_PrepareForUse(thisCharacter->characterClass);

	thisCharacter->scale = UUmRandomRangeFloat(thisCharacter->characterClass->scaleLow, thisCharacter->characterClass->scaleHigh);

	ONrCharacter_RecalculateIdleDelay(thisCharacter);

	// character control
	thisCharacter->prev_location = thisCharacter->location;
	thisCharacter->desiredFacing = thisCharacter->facing;

	// Camera
	ONrCharacter_GetFacingVector(thisCharacter,&thisCharacter->facingVector);
	ONrCharacter_FindOurNode(thisCharacter);

	/****** pain constants */

	UUmAssert(thisCharacter->painState.total_damage == 0);
	UUmAssert(thisCharacter->painState.decay_timer == 0);
	UUmAssert(thisCharacter->painState.last_sound_time == 0);
	UUmAssert(thisCharacter->painState.last_sound_type == 0);
	UUmAssert(thisCharacter->painState.num_light == 0);
	UUmAssert(thisCharacter->painState.num_medium == 0);
	UUmAssert(thisCharacter->painState.queue_timer == 0);
	UUmAssert(thisCharacter->painState.queue_type == 0);
	UUmAssert(thisCharacter->painState.queue_volume == 0);

	/****** post-character setup */

	ONrGameState_PresentCharacterList_Add(thisCharacter);
	ONrGameState_LivingCharacterList_Add(thisCharacter);

	/**** done */

	if (outCharacterIndex != NULL) {
		*outCharacterIndex = index;
	}

	if (thisCharacter->charType == ONcChar_Player) {
		// the character is initially active
		error = ONrCharacter_MakeActive(thisCharacter, UUcFalse);
		if (error != UUcError_None) {
			return error;
		}
		ONrGameState_SetPlayerNum(index);

	} else {
		// the character is initially inactive
		AI2rMovement_Initialize(thisCharacter);
		AI2rMovementStub_Initialize(thisCharacter, NULL);

		// set the AI into its job
		AI2rReturnToJob(thisCharacter, UUcTrue, UUcFalse);
	}

	// call spawn script if one exists
	if (thisCharacter->scripts.spawn_name[0] != '\0') {
		if (ONgGameState->gameTime == 0) {
			// delay so that scripts aren't called on game tick 0
			ONrScript_Schedule(thisCharacter->scripts.spawn_name, thisCharacter->player_name, 1, 1);
		} else {
			// safe to call immediately
			ONrScript_ExecuteSimple(thisCharacter->scripts.spawn_name, thisCharacter->player_name);
		}
	}

	if (ONcChar_AI2 == thisCharacter->charType) {
		UUmAssert(AI2cFlag_InUse & thisCharacter->ai2State.flags);
	}

	return UUcError_None;
}

UUtError
ONrGameState_NewActiveCharacter(
	ONtCharacter				*ioCharacter,
	UUtUns16					*outActiveCharacterIndex,
	UUtBool						inCopyCurrent)
{
	ONtActiveCharacter *activeCharacter;
	UUtUns16			index, salt;
	UUtUns32			overlay_index, itr;
	UUtUns16			numExtraParts;

	// get the parent character
	UUmAssertReadPtr(ioCharacter, sizeof(ONtCharacter));
	UUmAssert(ioCharacter->active_index == ONcCharacterIndex_None);

	if ((ioCharacter->flags & ONcCharacterFlag_Dead) || ((ioCharacter->flags & ONcCharacterFlag_InUse) == 0)) {
		// this character cannot be made active
		return UUcError_Generic;
	}

	index = ONiGameState_GetFreeActiveCharacter();
	if (index >= ONcMaxActiveCharacters)
	{
		return UUcError_Generic;
	}

#if DEBUG_CHARACTER_CREATION
	UUrDebuggerMessage("activate character %d -> activecharacter %d\n", ONrCharacter_GetIndex(ioCharacter), index);
#endif

	// set the number of characters in the gamestate
	if (index >= ONgGameState->usedActiveCharacters)
	{
		ONgGameState->usedActiveCharacters = index + 1;
	}

	// get a pointer to the active-character data
	activeCharacter = ONgGameState->activeCharacterStorage + index;
	UUrMemory_Clear(activeCharacter, sizeof(*activeCharacter));

	// calculate the pseudo-unique activecharacter index
	salt = (UUtUns16) ((ONgGameState->nextActiveCharacterSalt++) & ((1 << ONcCharacterSalt_Bits) - 1));
	activeCharacter->active_index = (index & ONcCharacterIndexMask_RawIndex);
	activeCharacter->active_index |= (salt << ONcCharacterSalt_Shift);
	activeCharacter->active_index |= ONcCharacterIndexMask_Valid;

	// set up the links between the character and its activecharacter
	activeCharacter->index = ioCharacter->index;
	ioCharacter->active_index = activeCharacter->active_index;
	ioCharacter->inactive_timer = 0;

	/****** physics, collision, movement */

	// clear the knockback
	UUmAssert(0.f == activeCharacter->knockback.x);
	UUmAssert(0.f == activeCharacter->knockback.y);
	UUmAssert(0.f == activeCharacter->knockback.z);

	UUmAssert(0 == activeCharacter->numWallCollisions);
	UUmAssert(0 == activeCharacter->numPhysicsCollisions);
	UUmAssert(0 == activeCharacter->numCharacterCollisions);
	UUmAssert(UUcFalse == activeCharacter->collidingWithTarget);

	UUmAssert(0.f == activeCharacter->gravityThisFrame);
	UUmAssert(0 == activeCharacter->offGroundFrames);
	MUmVector_Copy(activeCharacter->groundPoint, ioCharacter->location);

	// zero out the 'air control'
	UUmAssert(0 == activeCharacter->inAirControl.flags);
	UUmAssert(0 == activeCharacter->inAirControl.numFramesInAir);
	UUmAssert(0.f == activeCharacter->inAirControl.velocity.x);
	UUmAssert(0.f == activeCharacter->inAirControl.velocity.y);
	UUmAssert(0.f == activeCharacter->inAirControl.velocity.z);
	UUmAssert(UUcFalse == activeCharacter->inAirControl.buildValid);

	UUmAssert(P3cParticleReference_Null == activeCharacter->lastParticleDamageRef);

	/****** animation system */

	// clear the animation
	UUmAssert(NULL == activeCharacter->animation);
	activeCharacter->pendingDamageAnimType = ONcAnimType_None;
	UUmAssert(0 == activeCharacter->animVarient);
	UUmAssert(0 == activeCharacter->animFrame);
	UUmAssert(UUcFalse == activeCharacter->stitch.stitching);

	activeCharacter->throwing = (UUtUns16) -1;
	activeCharacter->thrownBy = (UUtUns16) -1;
	UUmAssert(NULL == activeCharacter->throwTarget);
	UUmAssert(NULL == activeCharacter->targetThrow);

	// clear the overlays
	for(overlay_index = 0; overlay_index < ONcOverlayIndex_Count; overlay_index++) {
		ONrOverlay_Initialize(activeCharacter->overlay + overlay_index);
	}

	// clear pose
	for(itr = 0; itr < ONcNumCharacterParts; itr++) {
		activeCharacter->curOrientations[itr] = MUgZeroQuat;
		activeCharacter->curAnimOrientations[itr] = MUgZeroQuat;
	}

	// no aiming screen
	UUmAssert(NULL == activeCharacter->aimingScreen);
	UUmAssert(NULL == activeCharacter->oldAimingScreen);
	UUmAssert(0 == activeCharacter->oldAimingScreenTime);

	// clear animation timers (relies on the memory clear to tdo this
	UUmAssert(0 == activeCharacter->softPauseFrames);
	UUmAssert(0 == activeCharacter->hardPauseFrames);
	UUmAssert(0 == activeCharacter->animationLockFrames);
	UUmAssert(0 == activeCharacter->minStitchingFrames);
	UUmAssert(0 == activeCharacter->hitStun);
	UUmAssert(0 == activeCharacter->blockStun);
	UUmAssert(0 == activeCharacter->dizzyStun);
	UUmAssert(0 == activeCharacter->staggerStun);

	// clear attacks
	activeCharacter->lastAttack = ONcAnimType_None;
	UUmAssert(0 == activeCharacter->lastAttackTime);

	// clear animation particles (relies on the memory clear to do this)
	UUmAssert(0 == activeCharacter->numParticles);
	UUmAssert(0 == activeCharacter->activeParticles);
	UUmAssert(NULL == activeCharacter->animParticles);
	//UUrMemory_Clear(activeCharacter->particles, TRcMaxParticles * sizeof(ONtCharacterParticleInstance));

	/****** input */

	activeCharacter->lastHeartbeat = UUcMaxUns32;

	// zero the please fields
	UUmAssert(UUcFalse == activeCharacter->pleaseFire);
	UUmAssert(UUcFalse == activeCharacter->pleaseAutoFire);
	UUmAssert(UUcFalse == activeCharacter->pleaseContinueFiring);
	UUmAssert(UUcFalse == activeCharacter->pleaseFireAlternate);
	UUmAssert(UUcFalse == activeCharacter->pleaseReleaseTrigger);
	UUmAssert(UUcFalse == activeCharacter->pleaseReleaseAlternateTrigger);
	UUmAssert(UUcFalse == activeCharacter->pleaseJump);
	UUmAssert(UUcFalse == activeCharacter->pleaseLand);
	UUmAssert(UUcFalse == activeCharacter->pleaseLandHard);

	// no keys held down
	UUmAssert(0 == activeCharacter->inputState.buttonIsDown);
	UUmAssert(0 == activeCharacter->inputState.buttonIsUp);
	UUmAssert(0 == activeCharacter->inputState.buttonLast);
	UUmAssert(0 == activeCharacter->inputState.buttonWentDown);
	UUmAssert(0 == activeCharacter->inputState.buttonWentUp);
	UUmAssert(0 == activeCharacter->inputState.turnLR);
	UUmAssert(0 == activeCharacter->inputState.turnUD);

	// aiming straight ahead
	activeCharacter->isAiming = UUcTrue;
	UUmAssert(0.f == activeCharacter->aimingLR);
	UUmAssert(0.f == activeCharacter->aimingUD);

	/****** rendering */

	// setup persistent rendering effects
	if (ioCharacter->inventory.invisibilityRemaining > 0) {
		activeCharacter->invis_character_alpha = 1.f;
		activeCharacter->invis_active = UUcTrue;
	}

	// character LOD
	activeCharacter->curBody = TRcBody_NotDrawn;
	activeCharacter->desiredBody = TRcBody_NotDrawn;

	// extra body part (weapon)
	numExtraParts = 1;
	UUmAssert(sizeof(activeCharacter->extra_body_memory) == (UUcProcessor_CacheLineSize + sizeof(TRtExtraBody) + (numExtraParts - 1) * sizeof(TRtExtraBodyPart)));
	activeCharacter->extraBody = (TRtExtraBody *) UUrAlignMemory(activeCharacter->extra_body_memory);
	activeCharacter->extraBody->numParts = numExtraParts;
	activeCharacter->extraBody->parts[0].geometry = (ioCharacter->inventory.weapons[0] == NULL) ? NULL : WPrGetClass(ioCharacter->inventory.weapons[0])->geometry;
	activeCharacter->extraBody->parts[0].matrix = NULL;
	ONrCharacter_SetHand(activeCharacter->extraBody, (ioCharacter->characterClass->leftHanded) ? ONcCharacterIsLeftHanded : ONcCharacterIsRightHanded);

	ONiCharacter_InitializeShadow(activeCharacter);

	activeCharacter->deferred_maximum_height = ioCharacter->actual_position.y;

	/****** final setup */

	activeCharacter->animVarient = 0;
	ONrCharacter_ResetAnimation(ioCharacter, activeCharacter, inCopyCurrent);
	ONrCharacter_DoAiming(ioCharacter, activeCharacter);
	ONrCharacter_BuildQuaternions(ioCharacter, activeCharacter);
	ONrCharacter_BuildMatriciesAll(ioCharacter, activeCharacter);
	ONrCharacter_EnablePhysics(ioCharacter);
	ONrCharacter_SnapToFloor(ioCharacter);

	/****** AI movement */

	if (ioCharacter->flags & ONcCharacterFlag_AIMovement) {
		// note: this must come after physics, as the A* stuff requires the spheretree to be working
		AI2rMovementState_Initialize(ioCharacter, &activeCharacter->movement_state, (inCopyCurrent) ? &ioCharacter->movementStub : NULL);
		AI2rExecutor_Initialize(ioCharacter);

		// the movement state may have moved us, we should recalculate our matricies
		ONrCharacter_BuildQuaternions(ioCharacter, activeCharacter);
		ONrCharacter_BuildMatriciesAll(ioCharacter, activeCharacter);
	}
	if (ioCharacter->currentPathnode != NULL) {
		// the dynamic grid for this room must be recalculated before it is next used, because we just appeared in there
		PHrFlushDynamicGrid(ioCharacter->currentPathnode);
	}

	/******* trigger volume */
	activeCharacter->trigger_points[0] = ioCharacter->location;
	activeCharacter->trigger_points[1] = ioCharacter->location;
	activeCharacter->trigger_points[1].y += 12.f;
	ONiCharacter_BuildTriggerSphere(activeCharacter);

	/****** done */

	ONrGameState_ActiveCharacterList_Add(ioCharacter);
	ONrCharacter_BuildPhysicsFlags(ioCharacter, activeCharacter);


	return UUcError_None;
}

UUtError
ONrCharacter_ArmByName(
	ONtCharacter *inCharacter,
	char *weaponName)
{
	WPtWeaponClass *weapon;
	UUtError error;

	if (NULL == weaponName) {
		weaponName = "StrMP10HC";
	}

	// get a reference to the weapon
	error = TMrInstance_GetDataPtr(WPcTemplate_WeaponClass, weaponName, (void **) &weapon);

	if (error) {
		return error;
	}

	ONrCharacter_UseNewWeapon(inCharacter, weapon);

	return UUcError_None;
}

static void ONrCharacter_BuildSphereTree(
	ONtCharacter *ioCharacter,
	ONtActiveCharacter *activeCharacter)
{
	UUtUns16 itr;
	const TRtBody *body = ONrCharacter_GetBody(ioCharacter, TRcBody_SuperHigh);

	// root node radius changes per frame
	PHrSphereTree_New(activeCharacter->sphereTree, NULL);

	for(itr = 0; itr < ONcSubSphereCount_Mid; itr++)
	{
		PHrSphereTree_New(activeCharacter->midNodes + itr, activeCharacter->sphereTree);
	}

	for(itr = 0; itr < ONcSubSphereCount; itr++)
	{
		PHrSphereTree_New(activeCharacter->leafNodes + itr, activeCharacter->midNodes + (itr / 2));
	}

	ONrCharacter_UpdateSphereTree(ioCharacter, activeCharacter);

	return;
}

float ONrCharacter_GetLeafSphereRadius(ONtCharacter *ioCharacter)
{
	float leaf_sphere_radius;

	if (ONrCharacter_IsAI(ioCharacter)) {
		leaf_sphere_radius = 3.0f;
	}
	else {
		leaf_sphere_radius = 4.0f;
	}

	return leaf_sphere_radius;
}

void ONrCharacter_RecalculateIdleDelay(ONtCharacter *ioCharacter)
{
	// each idle delay varies from 1 to 1.5 times character class's base idle delay
	ioCharacter->idleDelay = ioCharacter->characterClass->idleDelay +
		((ioCharacter->characterClass->idleDelay * UUrRandom()) >> (UUcRandomBits + 1));
}

static void ONrCharacter_UpdateSphereTree(
	ONtCharacter *ioCharacter,
	ONtActiveCharacter *activeCharacter)
{
	float sub_radius = ONrCharacter_GetLeafSphereRadius(ioCharacter);
	float increment;
	float height;
	UUtUns16 itr;
	float bottom;

	if (activeCharacter->inAirControl.numFramesInAir > 0) {
		bottom = ioCharacter->feetLocation;
	}
	else {
		bottom = ioCharacter->location.y;
	}

	height = ioCharacter->boundingBox.maxPoint.y - bottom; // boundingCylinder.top - bottom;

	UUmAssertOnce(height > 0.f);
	UUmAssertOnce(height <= 72.f);
	height = UUmPin(height, 0.f, 72.f);

	activeCharacter->sphereTree[0].sphere.radius = height / 2.f;
	activeCharacter->sphereTree[0].sphere.center = ioCharacter->location;
	activeCharacter->sphereTree[0].sphere.center.y = bottom;
	activeCharacter->sphereTree[0].sphere.center.y += height / 2.f;

	height -= sub_radius * 2.f;
	increment = height / ((float)ONcSubSphereCount - 1);

	for(itr = 0; itr < ONcSubSphereCount; itr++)
	{
		activeCharacter->leafNodes[itr].sphere.radius = sub_radius;
		activeCharacter->leafNodes[itr].sphere.center = ioCharacter->location;
		activeCharacter->leafNodes[itr].sphere.center.y = bottom;
		activeCharacter->leafNodes[itr].sphere.center.y += sub_radius + (itr * increment);
	}

	for(itr = 0; itr < ONcSubSphereCount_Mid; itr++)
	{
		PHtSphereTree *one = activeCharacter->leafNodes + ((itr * 2) + 0);
		PHtSphereTree *two = activeCharacter->leafNodes + ((itr * 2) + 1);
		float diff = two->sphere.center.y - one->sphere.center.y;

		activeCharacter->midNodes[itr].sphere.center = ioCharacter->location;
		activeCharacter->midNodes[itr].sphere.center.y = one->sphere.center.y + (diff / 2.f);
		activeCharacter->midNodes[itr].sphere.radius = one->sphere.radius + (diff / 2.f);
	}

	return;
}

void ONrCharacter_EnablePhysics(ONtCharacter *inCharacter)
{
	ONtActiveCharacter *active_character;
	static PHtPhysicsCallbacks callbacks;

	UUmAssertWritePtr(inCharacter, sizeof(*inCharacter));

	active_character = ONrForceActiveCharacter(inCharacter);

	ONrCharacter_BuildSphereTree(inCharacter, active_character);
	ONrCharacter_UpdateSphereTree(inCharacter, active_character);

	active_character->physics = PHrPhysicsContext_Add();
	if (active_character->physics == NULL) {
		return;
	}

	PHrPhysics_Init(active_character->physics);
	active_character->physics->sphereTree = active_character->sphereTree;
	active_character->physics->orientation = MUgZeroQuat;
	active_character->physics->position = inCharacter->location;
	active_character->physics->level = PHcPhysics_Linear;
	active_character->physics->animContext.animation = NULL;
	active_character->physics->callbackData = inCharacter;
	active_character->physics->gravity = 0.f;

	UUrMemory_Clear(&callbacks, sizeof(callbacks));
	callbacks.type = PHcCallback_Character;
	callbacks.findEnvCollisions = ONrCharacter_Callback_FindEnvCollisions;
	callbacks.findPhyCollisions = ONrCharacter_Callback_FindPhyCollisions;
	callbacks.skipPhyCollisions = NULL;
	callbacks.privateMovement = ONrCharacter_Callback_PrivateMovement;
	callbacks.receiveForce = ONrCharacter_Callback_ReceiveForce;
	callbacks.postReflection = ONrCharacter_Callback_PostReflection;
	callbacks.preDispose = ONrCharacter_Callback_PreDispose;

	active_character->physics->callback = &callbacks;
}

void ONrCharacter_DisablePhysics(ONtCharacter *inCharacter)
{
	ONtActiveCharacter *active_character;

	UUmAssertWritePtr(inCharacter, sizeof(*inCharacter));
	active_character = ONrGetActiveCharacter(inCharacter);
	if ((active_character == NULL) || (active_character->physics == NULL)) {
		return;
	}

	PHrPhysicsContext_Remove(active_character->physics);
	active_character->physics = NULL;
}

void
ONrGameState_DeleteCharacter(
	ONtCharacter				*inCharacter)
{
	ONtCharacterIndexType active_index;
#if defined(DEBUGGING) && DEBUGGING
	ONtCharacter debugging_character = *inCharacter;
	debugging_character;
#endif

	UUmAssertReadPtr(inCharacter, sizeof(*inCharacter));

	ONrGameState_PresentCharacterList_Remove(inCharacter);
	ONrGameState_LivingCharacterList_Remove(inCharacter);

	if (0 == (inCharacter->flags & ONcCharacterFlag_InUse)) {
		return;
	}

	if (ONrCharacter_IsActive(inCharacter)) {
		active_index = ONrCharacter_GetActiveIndex(inCharacter);

		if ((active_index >= 0) && (active_index < ONgGameState->usedActiveCharacters)) {
			ONtActiveCharacter *active_character = ONgGameState->activeCharacterStorage + active_index;

			if (active_character->index == inCharacter->index) {
				ONrGameState_DeleteActiveCharacter(active_character, UUcFalse);
			} else {
				UUmAssert(!"ONrGameState_DeleteCharacter: supposed active character's index does not match ours");
			}
		}
	}

#if DEBUG_CHARACTER_CREATION
	UUrDebuggerMessage("delete character %d\n", ONrCharacter_GetIndex(inCharacter));
#endif

	if (inCharacter->charType == ONcChar_AI2) {
		AI2rDeleteState(inCharacter);
	}

	AI2rFight_StopFight(inCharacter);

	WPrSlots_DeleteAll(&inCharacter->inventory);

	if ((ONrCharacter_GetIndex(inCharacter) + 1) == ONgGameState->numCharacters) {
		// find the index of the highest character still in use
		while ((ONgGameState->numCharacters > 0) &&
				((ONgGameState->characters[ONgGameState->numCharacters - 1].flags & ONcCharacterFlag_InUse) == 0)) {
			ONgGameState->numCharacters -= 1;
		}
	}

	UUrMemory_Clear(inCharacter, sizeof(*inCharacter));

	// tell the AI knowledgebase that this character was deleted
	AI2rKnowledge_ThisCharacterWasDeleted(inCharacter);

	// check to make sure that deleting this character hasn't broken the
	// AI knowledgebase
	AI2rKnowledge_Verify();

	return;
}

void
ONrGameState_DeleteActiveCharacter(
	ONtActiveCharacter			*ioActiveCharacter,
	UUtBool						inCopyCurrent)
{
	ONtCharacter *			thisCharacter;
	ONtCharacterIndexType	parent_index;

	UUmAssertReadPtr(ioActiveCharacter, sizeof(*ioActiveCharacter));
	if (ioActiveCharacter->index == ONcCharacterIndex_None) {
		UUmAssert(!"ONrGameState_DeleteActiveCharacter: tried to delete invalid active character");
		return;
	}

	// get the parent character
	parent_index = ONrActiveCharacter_GetIndex(ioActiveCharacter);
	if (parent_index >= ONgGameState->numCharacters) {
		UUmAssert(!"ONrGameState_DeleteActiveCharacter: tried to delete invalid active character");
		return;
	}

	thisCharacter = ONgGameState->characters + parent_index;
	if ((ioActiveCharacter->active_index != thisCharacter->active_index) || (thisCharacter->index != ioActiveCharacter->index)) {
		// there is an error - this active-character isn't ours
		UUmAssert(!"ONrGameState_DeleteActiveCharacter: active-character does not match character");
		return;
	}

#if DEBUG_CHARACTER_CREATION
	UUrDebuggerMessage("delete active character %d -> character %d inactive\n", ONrActiveCharacter_GetActiveIndex(ioActiveCharacter), parent_index);
#endif

	if (inCopyCurrent) {
		// set up the movement stub for our present position
		AI2rMovementStub_Initialize(thisCharacter, &ioActiveCharacter->movement_state);
	}

	if (ioActiveCharacter->extraBody != NULL) {
		ioActiveCharacter->extraBody = NULL;
	}

	// we must check ONgParticle3_Valid because we could be in the middle of the
	// unload process when we try to delete this character, in which case the particle
	// system has already been torn down
	if (ONgParticle3_Valid) {
		ONtCharacterParticleInstance *particle;
		UUtUns32 itr;

		if (ioActiveCharacter->superParticlesCreated) {
			ONiCharacter_SuperParticle_Delete(thisCharacter, ioActiveCharacter);
		}

		if (ioActiveCharacter->numParticles > 0) {
			// kill all the animation particles
			for (itr = 0, particle = ioActiveCharacter->particles; itr < ioActiveCharacter->numParticles; itr++, particle++) {
				if (particle->particle == NULL)
					continue;

				if (particle->particle->header.self_ref != particle->particle_ref)
					continue;

				UUmAssert(particle->particle_def != NULL);
				P3rKillParticle(particle->particle_def->particle_class, particle->particle);
			}
			ioActiveCharacter->numParticles = 0;
		}
	}

	ONiCharacter_DeleteShadow(ioActiveCharacter);
	ONiCharacter_DeallocateShadow(ioActiveCharacter);

	// delete the active character's information
	ONrCharacter_DisablePhysics(thisCharacter);

	ONrGameState_ActiveCharacterList_Remove(thisCharacter);

	thisCharacter->active_index = ONcCharacterIndex_None;

	if ((ONrActiveCharacter_GetIndex(ioActiveCharacter) + 1) == ONgGameState->usedActiveCharacters) {
		// work out what the index of the highest active-character in use is
		while ((ONgGameState->usedActiveCharacters > 0) &&
				(ONgGameState->activeCharacterStorage[ONgGameState->usedActiveCharacters - 1].active_index == ONcCharacterIndex_None)) {
			ONgGameState->usedActiveCharacters -= 1;
		}
	}

	UUrMemory_Clear(ioActiveCharacter, sizeof(*ioActiveCharacter));

	return;
}

float MUrPointsXYAngle(float inFromX, float inFromY, float inToX, float inToY);

void ONrCharacter_BuildJump(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	if (ioActiveCharacter == NULL)
		return;

	ONrCharacter_GetVelocityEstimate(ioCharacter, ioActiveCharacter, &(ioActiveCharacter->inAirControl.buildVelocity));

	ioActiveCharacter->inAirControl.flags = 0;
	ioActiveCharacter->inAirControl.flags |= ONcInAirFlag_Jumped;
	ioActiveCharacter->inAirControl.buildValid = UUcTrue;

	return;
}

UUtBool ONrCharacter_InAir(ONtCharacter *inCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character == NULL) {
		return UUcFalse;
	} else {
		return (active_character->inAirControl.numFramesInAir > 0);
	}
}

static void ONrCharacter_Verify(const ONtCharacter *inCharacter, const ONtActiveCharacter *ioActiveCharacter)
{
	UUtUns16 itr;
	UUtUns16 numParts = ONrCharacter_GetNumParts(inCharacter);

	for(itr = 0; itr < numParts; itr++) {
		MUmQuat_VerifyUnit(ioActiveCharacter->curOrientations + itr);
		MUmQuat_VerifyUnit(ioActiveCharacter->curAnimOrientations + itr);
	}
}

static void ONiCharacter_StartJumping(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	const ONtAirConstants *airConstants;
	M3tVector3D speed;
	float boost;

	if (ioActiveCharacter == NULL)
		return;

	airConstants = ONrCharacter_GetAirConstants(ioCharacter);
	boost = 0.5f * airConstants->jumpAmount;

	speed = ioActiveCharacter->inAirControl.buildVelocity;
	speed.y += airConstants->jumpAmount;

	if (ioActiveCharacter->inputState.buttonIsDown & LIc_BitMask_Forward)
	{
		speed.z = UUmMax(speed.z, boost);
	}
	else if (ioActiveCharacter->inputState.buttonIsDown & LIc_BitMask_Backward)
	{
		speed.z = UUmMin(speed.z, -boost);
	}
	else if (ioActiveCharacter->inputState.buttonIsDown & LIc_BitMask_StepLeft)
	{
		speed.x = UUmMax(speed.x, boost);
	}
	else if (ioActiveCharacter->inputState.buttonIsDown & LIc_BitMask_StepRight)
	{
		speed.x = UUmMin(speed.x, -boost);
	}

	ioActiveCharacter->inAirControl.velocity = speed;
	ioActiveCharacter->inAirControl.buildValid = UUcFalse;

	return;
}

UUtBool ONrAnim_IsPickup(TRtAnimType inAnimType)
{
	return ((inAnimType == ONcAnimType_Pickup_Object) ||
			(inAnimType == ONcAnimType_Pickup_Pistol) || (inAnimType == ONcAnimType_Pickup_Rifle));
}

static void	ONrCharacter_Pickup(ONtCharacter *ioCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
	WPtWeapon *weapon;
	WPtPowerup *powerup;
	UUtUns16 action_frame;
	float min_y = ioCharacter->location.y + ONcPickupMinY;
	float max_y = ioCharacter->location.y + ONcPickupMaxY;

	if (active_character == NULL) {
		return;
	}

	if (ioCharacter->pickup_target != NULL) {
		if (!ONrAnim_IsPickup(active_character->curAnimType) && !ONrAnim_IsPickup(active_character->nextAnimType)) {
			ioCharacter->pickup_target = NULL;
		}
	}

	action_frame = UUmMin(active_character->animation->actionFrame, active_character->animation->numFrames - 1);

	if (active_character->animFrame != action_frame)
	{
		return;
	}

	switch(active_character->curAnimType)
	{
		case ONcAnimType_Pickup_Object_Mid:
		case ONcAnimType_Pickup_Object:
			powerup = WPrFind_Powerup_XZ(ioCharacter, &ioCharacter->actual_position, min_y, max_y, ioCharacter->pickup_target, ONcPickupRadius, NULL);
			if (NULL != powerup) {
				ONrCharacter_PickupPowerup(ioCharacter, powerup);
			}

			// the action frame has passed, this pickup is over - successful or not
			ioCharacter->pickup_target = NULL;
		break;

		case ONcAnimType_Pickup_Pistol_Mid:
		case ONcAnimType_Pickup_Rifle_Mid:
		case ONcAnimType_Pickup_Pistol:
		case ONcAnimType_Pickup_Rifle:
			weapon = WPrFind_Weapon_XZ(&ioCharacter->actual_position, min_y, max_y, ioCharacter->pickup_target, ONcPickupRadius);

			if (NULL != weapon) {
				ONrCharacter_PickupWeapon(ioCharacter, weapon, UUcFalse);
			}

			// the action frame has passed, this pickup is over - successful or not
			ioCharacter->pickup_target = NULL;
		break;
	}

	return;
}

// super particle functions
static void ONiCharacter_SuperParticle_Create(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	// create the super particles
	ONtCharacterParticleArray *particle_array = ioCharacter->characterClass->particles;
	ONtCharacterParticle *particle_def;
	ONtCharacterParticleInstance *particle;
	UUtUns32 itr;

	// no super particles yet
	ioActiveCharacter->numSuperParticles = 0;
	particle = ioActiveCharacter->superParticles;

	if (particle_array != NULL) {
		// loop over all particles in the array
		for (itr = 0, particle_def = particle_array->particle; itr < particle_array->numParticles; itr++, particle_def++) {
			if (UUrString_CompareLen_NoCase(particle_def->name, "super", 5) == 0) {
				if (particle_def->particle_class == NULL) {
					// this particle class was not found
					continue;
				}

				// this is a super particle and must be created
				particle->particle_def = particle_def;
				particle->particle = NULL;
				particle->particle_ref = P3cParticleReference_Null;
				if ((particle_def->override_partindex < 0) || (particle_def->override_partindex >= ONcNumCharacterParts)) {
					// we don't have an animation to tell us the part index, we can't attach this particle
					continue;
				}

				particle->bodypart_index = particle_def->override_partindex;
				particle->particle = ONiCharacter_AttachParticle(ioCharacter, ioActiveCharacter,
																particle_def->particle_class, particle->bodypart_index);
				if (particle->particle == NULL) {
					// could not create and attach the particle
					continue;
				}

				particle->particle_ref = particle->particle->header.self_ref;
				particle++;
				ioActiveCharacter->numSuperParticles++;
				if (ioActiveCharacter->numSuperParticles >= TRcMaxParticles) {
					// can't add any more particles
					break;
				}
			}
		}
	}

	ioActiveCharacter->superParticlesCreated = UUcTrue;
	ioActiveCharacter->superParticlesActive = UUcFalse;
}

static void ONiCharacter_SuperParticle_Delete(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	ONtCharacterParticleInstance *particle;
	UUtUns32 itr;

	// kill all the super particles
	for (itr = 0, particle = ioActiveCharacter->superParticles; itr < ioActiveCharacter->numSuperParticles; itr++, particle++) {
		if ((particle->particle == NULL) || (particle->particle->header.self_ref != particle->particle_ref))
			continue;

		UUmAssert(particle->particle_def != NULL);
		P3rKillParticle(particle->particle_def->particle_class, particle->particle);
	}

	ioActiveCharacter->numSuperParticles = 0;
	ioActiveCharacter->superParticlesCreated = UUcFalse;
	ioActiveCharacter->superParticlesActive = UUcFalse;
}

static void ONiCharacter_SuperParticle_SendEvent(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter, UUtUns16 inEvent)
{
	float current_time = ONrGameState_GetGameTime() / ((float) UUcFramesPerSecond);
	ONtCharacterParticleInstance *particle;
	UUtUns32 itr;

	// send an event to all the super particles
	for (itr = 0, particle = ioActiveCharacter->superParticles; itr < ioActiveCharacter->numSuperParticles; itr++, particle++) {
		if ((particle->particle == NULL) || (particle->particle->header.self_ref != particle->particle_ref))
			continue;

		UUmAssert(particle->particle_def != NULL);
		P3rSendEvent(particle->particle_def->particle_class, particle->particle, inEvent, current_time);
	}

	if (inEvent == P3cEvent_Start) {
		ioActiveCharacter->superParticlesActive = UUcTrue;
	} else if (inEvent == P3cEvent_Stop) {
		ioActiveCharacter->superParticlesActive = UUcFalse;
	}
}

UUtBool ONrCharacter_AutoAim(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter, ONtCharacter *target);

UUtBool LIrKeyWentDown(LItKeyCode inKeyCode, UUtBool *old_key_down);

float ONrGameState_CalculateGravity(ONtCharacter *inCharacter, ONtActiveCharacter *inActiveCharacter)
{
	float min_gravity = ONcInitialFallingVelocity;
	float max_gravity = ONcMaxRampVelocity;
	float gravity = min_gravity;

	if (ONrAnimType_IsVictimType(inActiveCharacter->curAnimType)) {
		// if you are being hit or thrown you get the default gravity
		goto exit;
	}

	if (TRrAnimation_IsAttack(inActiveCharacter->animation)) {
		goto exit;
	}

	{
		M3tPoint3D vector_down = MUgZeroPoint;
		UUtBool did_collide;

		vector_down.y = -max_gravity;

		did_collide = AKrCollision_Point(ONgLevel->environment, &inCharacter->location, &vector_down, AKcGQ_Flag_Chr_Col_Skip, 1);

		if (did_collide) {
			float distance_to_ground = inCharacter->location.y - AKgCollisionList[0].collisionPoint.y;

			if (distance_to_ground < max_gravity) {
				gravity = UUmMax(gravity, distance_to_ground);
			}
		}
	}

exit:
	return gravity;
}

static void ONrCharacter_DebugKeys(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
#if TOOL_VERSION
	/*
	 * debugging keys
	 */

	if (ONrDebugKey_WentDown(ONcDebugKey_Test_One)) {
		P3rRemoveDangerousProjectiles();
	}

	if (ONrDebugKey_WentDown(ONcDebugKey_Test_Two)) {
		ONtCharacter *saved_film_src_character = ONrGameState_GetPlayerCharacter();
		ONtActiveCharacter *saved_film_active_character = ONrGetActiveCharacter(saved_film_src_character);
		ONtCharacter *saved_film_dst_character = ONgGameState->characters + saved_film_character_offset;

		UUmAssertReadPtr(saved_film_active_character, sizeof(*saved_film_active_character));
		if (saved_film_active_character->filmState.film != NULL) {
			ONrFilm_Start(saved_film_dst_character, saved_film_active_character->filmState.film, ONcFilmMode_Normal, 0);
		}
	}

	if (ONrDebugKey_WentDown(ONcDebugKey_Test_Three) && (ioActiveCharacter != NULL)) {
	}

	if (ONrDebugKey_WentDown(ONcDebugKey_Test_Four) && (ioActiveCharacter != NULL)) {
		const TRtAnimation *startle = NULL;

		ONrCharacter_FightMode(ioCharacter);

		startle = TRrCollection_Lookup(
				ioCharacter->characterClass->animations,
				ONcAnimType_Startle_Back,
				ioActiveCharacter->nextAnimState,
				ioActiveCharacter->animVarient);


		if (NULL != startle) {
			ONrCharacter_SetAnimation_External(ioCharacter, ioActiveCharacter->nextAnimState, startle, 6);
		}
		else {
			COrConsole_Printf("no startle to be found");
		}
	}

	if (ONrDebugKey_WentDown(ONcDebugKey_KillParticles)) {
		// kill all particles
		ONrParticle3_KillAll(UUcFalse);
	}

	if (ONrDebugKey_WentDown(ONcDebugKey_ResetParticles)) {
		// kill all particles and then recreate from definitions
		ONrParticle3_KillAll(UUcTrue);
	}
#endif
}

static void ONrGameState_DoCharacterFrame(
					ONtCharacter *ioCharacter)
{
	const ONtAirConstants *airConstants = ONrCharacter_GetAirConstants(ioCharacter);
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
	float height;
	TRtBody *body;
	M3tMatrix4x3 *weapon_matrixptr = NULL;
	UUtUns32 itr;
	UUtBool is_sound_frame = UUcFalse;
	TRtAnimTime duration;
	const char *new_sound_name = NULL;
	M3tVector3D moved, previous_weapon_location, new_weapon_location;
	static M3tMatrix4x3 test_matrix;
	UUmAssertWritePtr(ONgGameState, sizeof(*ONgGameState));
	UUmAssertWritePtr(ioCharacter, sizeof(*ioCharacter));
	UUmAssertTrigRange(ioCharacter->facing);

	if ((active_character != NULL) && (active_character->frozen)) {
		// don't perform this update at all
		return;
	}

	ONrSpeech_Update(ioCharacter);

	ONrCharacter_Pickup(ioCharacter);
	WPrInventory_Update(ioCharacter);

	if (active_character == NULL) {
		// build an approximate bounding box since we aren't actually running animations
		ONrCharacter_BuildApproximateBoundingVolumes(ioCharacter);

	} else {
		/***************** PRELIMINARY INPUT */

		if (ONcChar_Player == ioCharacter->charType) {
			ONrCharacter_VerifyExtraBody(ioCharacter, active_character);
		}

		if (ONcChar_Player == ioCharacter->charType) {
			ONrCharacter_DebugKeys(ioCharacter, active_character);
		}

		/*
		 * keyboard input
		 */

		{
			UUtBool weapon_full, no_ammo;

			if ((active_character->inputState.buttonWentDown & LIc_BitMask_Reload) && (ioCharacter->inventory.weapons[0])) {
				WPrTryReload(ioCharacter, ioCharacter->inventory.weapons[0], &weapon_full, &no_ammo);

				if ((ioCharacter == ONrGameState_GetPlayerCharacter()) && (weapon_full || no_ammo)) {
					// play inventory-fail sound
					ONrGameState_EventSound_Play(ONcEventSound_InventoryFail, NULL);
				}
			}
		}

		if (active_character->keyHoldFrames > 0) {
			active_character->keyHoldFrames -= 1;

			if (0 == active_character->keyHoldFrames) {
				ioCharacter->flags &= ~ONcCharacterFlag_ScriptControl;
			}
		}

		if (ioCharacter->charType == ONcChar_Player) {
			if (0xFFFFFFFF == active_character->last_forward_tap) {
				ONrCharacter_SetSprintVarient(ioCharacter, UUcTrue);

				if (active_character->inputState.buttonIsUp & LIc_BitMask_Forward) {
					active_character->last_forward_tap = 0;
				}
			}
			else {
				ONrCharacter_SetSprintVarient(ioCharacter, UUcFalse);

				if (active_character->inputState.buttonWentDown & LIc_BitMask_Forward) {
					UUtUns32 how_long_ago_did_we_forward_tap = ONgGameState->gameTime - active_character->last_forward_tap;

					if (how_long_ago_did_we_forward_tap < 15) {
						active_character->last_forward_tap = 0xFFFFFFFF;
					}
					else {
						active_character->last_forward_tap = ONgGameState->gameTime;
					}
				}
			}
		}

		/*
		 * player aiming control
		 */

		active_character->autoAimAdjustmentLR = 0.f;
		active_character->autoAimAdjustmentUD = 0.f;

		if (ioCharacter->charType == ONcChar_Player)
		{
			const float move_amount = 0.03f;
			const float return_angle = M3cHalfPi * 0.2f;
			UUtBool do_lookspring;
			UUtUns32 auto_aim_itr;
			UUtUns32 auto_aim_count = ONrGameState_ActiveCharacterList_Count();
			ONtCharacter **auto_aim_character_list = ONrGameState_ActiveCharacterList_Get();
			UUtBool found_target = UUcFalse;
			float one_minus_fight_mode_percent = 1.f - ((float) active_character->fightFramesRemaining) / ((float) ioCharacter->characterClass->fightModeDuration);
			float auto_aiming_distance = 0.f;

			// lookspring
			do_lookspring = (active_character != NULL) && (ONgLookspringFightModePercent >= one_minus_fight_mode_percent);
			do_lookspring &= !ONrCharacter_IsStill(ioCharacter);

			if (do_lookspring) {
				if (!ioCharacter->inventory.weapons[0] && (active_character->inputState.buttonIsUp & LIc_BitMask_LookMode)) {
					if (active_character->aimingUD > return_angle) {
						active_character->aimingUD -= move_amount;
						active_character->aimingUD = UUmMax(active_character->aimingUD, return_angle);
					}
					else if (active_character->aimingUD < -return_angle) {
						active_character->aimingUD += move_amount;
						active_character->aimingUD = UUmMin(active_character->aimingUD, -return_angle);
					}
				}
			}

			if (active_character->inputState.buttonIsUp & LIc_BitMask_LookMode) {
				active_character->aimingLR = iMoveTowardsZero(active_character->aimingLR, move_amount);
			}

			// auto-aiming
			for (auto_aim_itr = 0; auto_aim_itr < auto_aim_count; auto_aim_itr++)
			{
				ONtCharacter *target = auto_aim_character_list[auto_aim_itr];
				ONtActiveCharacter *active_target = ONrGetActiveCharacter(target);

				UUmAssertReadPtr(active_target, sizeof(*active_target));

				if (ioCharacter->flags & ONcCharacterFlag_Dead) { continue; }
				if (TRcBody_NotDrawn == active_target->curBody) { continue; }
				if (ioCharacter == target) { continue; }

				if (!found_target) {
					found_target = ONrCharacter_AutoAim(ioCharacter, active_character, target);

					if (found_target) {
						auto_aiming_distance = MUrPoint_Distance(&ioCharacter->actual_position, &target->actual_position);
					}
				}
				else {
					UUtBool found_second_target = ONrCharacter_AutoAim(ioCharacter, active_character, target);

					if (found_second_target) {
						active_character->autoAimAdjustmentLR = 0.f;
						active_character->autoAimAdjustmentUD = 0.f;
						found_target = UUcFalse;
						break;
					}
				}
			}

			if (found_target) {
				float max_auto_aiming = 135.f;
				float zero_auto_aiming = 195.f;

				if (auto_aiming_distance > max_auto_aiming) {
					float auto_aiming_distance_percent;

					auto_aiming_distance_percent = auto_aiming_distance - max_auto_aiming;
					auto_aiming_distance_percent /= zero_auto_aiming - max_auto_aiming;
					auto_aiming_distance_percent = UUmPin(auto_aiming_distance_percent, 0.f, 1.f);
					auto_aiming_distance_percent = 1.f - auto_aiming_distance_percent;

					active_character->autoAimAdjustmentLR *= auto_aiming_distance_percent;
					active_character->autoAimAdjustmentUD *= auto_aiming_distance_percent;
				}
			}
		}


		/***************** PRE-MOVEMENT ANIMATION */

		/*
		 * general animation update
		 */

		if (active_character->animationLockFrames > 0) {
			active_character->animationLockFrames -= 1;

			if (active_character->animationLockFrames == 0) {
				ONrCharacter_DoAnimation(ioCharacter, active_character, ONcAnimPriority_Appropriate, ONcAnimType_Stand);
			}
		}

		if (active_character->softPauseFrames > 0) {
			active_character->softPauseFrames -= 1;
		}

		if (active_character->hardPauseFrames > 0) {
			active_character->hardPauseFrames -= 1;
		}

		if (active_character->blockStun > 0) {
			active_character->blockStun -= 1;
		}

		if (active_character->hitStun > 0) {
			active_character->hitStun -= 1;
		}

		if (active_character->dizzyStun > 0) {
			active_character->dizzyStun -= 1;
		}

		if (active_character->staggerStun > 0) {
			active_character->staggerStun -= 1;
		}

		if (active_character->flash_count > 0) {
			active_character->flash_count -= 1;

			if (active_character->flash_count == 0) {
				active_character->flash_parts = 0;
			}
		}

		if (active_character->pickup_percentage > 0) {
			active_character->pickup_percentage--;
		}

		active_character->prevAnimType = active_character->curAnimType;

		/*
		 * height calculation
		 */

		height = TRrAnimation_GetHeight(active_character->animation, active_character->animFrame);
		if (active_character->stitch.stitching) {
			float toAmt = ((float) active_character->stitch.itr) / ((float) active_character->stitch.count);
			float fromAmt = 1 - toAmt;

			UUmAssert(active_character->stitch.count > 0);
			height = (active_character->stitch.height * fromAmt) + (height * toAmt);
		}
		UUmAssert((height > -128.f) && (height < 128.f));
		ioCharacter->heightThisFrame = height;


		/***************** MOVEMENT */

		/*
		 * special-case movement
		 */

		if (ioCharacter->inventory.weapons[0] != NULL) {
			UUmAssert(WPrInUse(ioCharacter->inventory.weapons[0]));

			body = ONrCharacter_GetBody(ioCharacter, TRcBody_SuperHigh);
			weapon_matrixptr = active_character->matricies + body->numParts;
			previous_weapon_location = MUrMatrix_GetTranslation(weapon_matrixptr);
		}

		if ((active_character->physics != NULL) && (active_character->physics->animContext.animation != NULL)) {
			if (active_character->physics->animContext.animationFrame >=
					(active_character->physics->animContext.animation->numFrames - 1)) {

				active_character->physics->animContext.animation = NULL;
				active_character->physics->animContext.animContextFlags = 0;
				active_character->physics->animContext.animationFrame = 0;
				active_character->physics->animContext.animationStep = 0;
				active_character->physics->animContext.frameStartTime = 0;

				ONrCharacter_BuildPhysicsFlags(ioCharacter, active_character);
			}
		}

		if (ioCharacter->flags & ONcCharacterFlag_Dead_2_Moving) {
			ONrCharacter_BuildQuaternions(ioCharacter, active_character);
			ONrCharacter_BuildMatriciesAll(ioCharacter, active_character);

			if (0 == (ioCharacter->flags & ONcCharacterFlag_Dead_3_Cosmetic)) {
				UUmAssert(active_character->physics != NULL);

				if (active_character->physics != NULL) {
					float max_xz_distance = 0.5f;
					float xz_distance = 0.f;

					xz_distance += UUmSQR(active_character->physics->velocity.x);
					xz_distance += UUmSQR(active_character->physics->velocity.z);

					if (xz_distance > UUmSQR(max_xz_distance)) {
						xz_distance = MUrSqrt(xz_distance);

						active_character->death_velocity.x *= max_xz_distance / xz_distance;
						active_character->death_velocity.z *= max_xz_distance / xz_distance;
					}

					{
						UUtBool on_the_ground = (0 == active_character->offGroundFrames) && (0 == active_character->inAirControl.numFramesInAir);

						if (on_the_ground) {
							active_character->death_velocity.x *= 0.5f;
							active_character->death_velocity.z *= 0.5f;

							active_character->death_velocity.y = -0.5f;
						}
						else {
							active_character->death_velocity.x *= 0.8f;
							active_character->death_velocity.z *= 0.8f;

							active_character->death_velocity.y -= 0.5f;
							active_character->death_velocity.y = UUmPin(active_character->death_velocity.y, -10.f, 2.f);
						}
					}

					ONrCharacter_SetMovementThisFrame(ioCharacter, active_character, &active_character->death_velocity);
				}
			}

			return;
		}

		/*
		 * air control
		 */

		if (active_character->inAirControl.pickupReleaseTimer > 0) {
			active_character->inAirControl.pickupReleaseTimer--;

			if (active_character->inAirControl.pickupReleaseTimer == 0) {
				// the character has been released from the pickup state
				active_character->inAirControl.numFramesInAir = 1;
				active_character->inAirControl.pickupOwner = NULL;
				active_character->inAirControl.jump_height = active_character->physics->position.y;
			}
		}

		if (active_character->inAirControl.pickupReleaseTimer > 0) {
			// the character is being picked up by someone
			moved = active_character->inAirControl.velocity;
			active_character->inAirControl.numFramesInAir += 1;

			MUmVector_Scale(active_character->inAirControl.velocity, 0.9f);

			if (active_character->inAirControl.pickupOwner != NULL) {
				M3tVector3D dir_to_owner, dir_to_start;
				float vel_mag, horiz_dist, horiz_dist_from_start, costheta, sintheta;
				float owner_height, target_height, max_target_height, vertical_displacement;

				static float cTargetHorizDist	= 40.0f;
				static float cHorizAccel		= 0.04f;
				static float cGlidePathAngle	= 0.3f;
				static float cTakeOffAngle		= 1.2f;
				static float cTargetHeight		= 12.0f;
				static float cVerticalAccel		= 0.12f;
				static float cMaxVerticalDist	= 40.0f;
				static float cMaxTotalVelocity	= 2.5f;
				static float cVelocityDecay		= 0.8f;

				owner_height = active_character->inAirControl.pickupOwner->actual_position.y;
				MUmVector_Subtract(dir_to_owner, active_character->inAirControl.pickupOwner->actual_position, ioCharacter->actual_position);
				horiz_dist = MUrSqrt(UUmSQR(dir_to_owner.x) + UUmSQR(dir_to_owner.z));

				if (!active_character->inAirControl.pickupStarted) {
					// calculate starting values for the pickup sequence
					active_character->inAirControl.pickupStartPoint = ioCharacter->actual_position;
					active_character->inAirControl.pickupStarted = UUcTrue;
				}

				// pull the character towards whoever has picked them up
				if (horiz_dist > cTargetHorizDist) {
					costheta = MUrCos(ioCharacter->facing);
					sintheta = MUrSin(ioCharacter->facing);

					active_character->inAirControl.velocity.x += cHorizAccel * (costheta * dir_to_owner.x - sintheta * dir_to_owner.z) / horiz_dist;
					active_character->inAirControl.velocity.z += cHorizAccel * (sintheta * dir_to_owner.x + costheta * dir_to_owner.z) / horiz_dist;
				}

				// get the target height of the "glide path"
				target_height = active_character->inAirControl.pickupOwner->actual_position.y + cTargetHeight;
				if (horiz_dist > cTargetHorizDist) {
					target_height += (horiz_dist - cTargetHorizDist) * cGlidePathAngle;
				}

				// don't accelerate too quickly
				MUmVector_Subtract(dir_to_start, active_character->inAirControl.pickupStartPoint, ioCharacter->actual_position);
				horiz_dist_from_start = MUrSqrt(UUmSQR(dir_to_start.x) + UUmSQR(dir_to_start.z));
				max_target_height = active_character->inAirControl.pickupStartPoint.y + horiz_dist_from_start * cTakeOffAngle;
				target_height = UUmMin(target_height, max_target_height);

				vertical_displacement = target_height - ioCharacter->actual_position.y;
				vertical_displacement = UUmPin(vertical_displacement, -cMaxVerticalDist, +cMaxVerticalDist);
				active_character->inAirControl.velocity.y += vertical_displacement * cVerticalAccel / cMaxVerticalDist;

				vel_mag = MUmVector_GetLength(active_character->inAirControl.velocity);
				if (vel_mag > cMaxTotalVelocity) {
					MUmVector_Scale(active_character->inAirControl.velocity,
									(cMaxTotalVelocity + cVelocityDecay * (vel_mag - cMaxTotalVelocity)) / vel_mag);
				}
			}

		} else {
			// we are not being picked up
			active_character->inAirControl.pickupStarted = UUcFalse;

			if (active_character->pleaseJump) {
				active_character->offGroundFrames = 0;

				if (0 == active_character->inAirControl.numFramesInAir) {
					if (active_character->inAirControl.buildValid) {
						ONiCharacter_StartJumping(ioCharacter, active_character);
					}
					else {
						active_character->inAirControl.flags = 0;
						ONrCharacter_GetVelocityEstimate(ioCharacter, active_character, &(active_character->inAirControl.velocity));
						active_character->inAirControl.velocity.y = -ONcInitialFallingVelocity;
					}

					active_character->inAirControl.numFramesInAir = 1;
				}

				active_character->pleaseJump = UUcFalse;	// should we zero this even if they are in the air?
			}

			if (active_character->inAirControl.numFramesInAir > 0) {
				moved = active_character->inAirControl.velocity;
				active_character->inAirControl.numFramesInAir += 1;

				if (active_character->inAirControl.numFramesInAir > ONcMaxFramesInAir) {
					ONrCharacter_TakeDamage(ioCharacter, ioCharacter->hitPoints, 0, NULL, NULL, WPcDamageOwner_Falling, ONcAnimType_None);
				}

				// dead people obey slightly different rules
				active_character->inAirControl.velocity.y -= airConstants->jumpGravity;
				active_character->inAirControl.velocity.y = UUmMax(active_character->inAirControl.velocity.y, airConstants->maxVelocity);
			}
			else if (active_character->stitch.stitching) {
				float toAmt = ((float) active_character->stitch.itr) / ((float) active_character->stitch.count);
				float fromAmt = 1 - toAmt;

				TRrAnimation_GetPosition(active_character->animation, active_character->animFrame, &moved);
				moved.x = (active_character->stitch.velocity.x * fromAmt) + (moved.x * toAmt);
				moved.y = (active_character->stitch.velocity.y * fromAmt) + (moved.y * toAmt);
				moved.z = (active_character->stitch.velocity.z * fromAmt) + (moved.z * toAmt);

				moved.y -= ONrGameState_CalculateGravity(ioCharacter, active_character);

			}
			else {
				TRrAnimation_GetPosition(active_character->animation, active_character->animFrame, &moved);
				moved.y -= ONrGameState_CalculateGravity(ioCharacter, active_character);
			}
		}

		MUmAssertVector(moved, 1000.f);

		if ((ioCharacter == ONgGameState->local.playerCharacter)) {
			if (gDebugCharacterMovement != gDebugCharacterMovement_Old) {
				gSmoothedMovement = (M3tPoint3D *) UUrMemory_Block_Realloc(gSmoothedMovement, sizeof(M3tPoint3D) * gDebugCharacterMovement);
				gDebugCharacterMovement_Old = gDebugCharacterMovement;
			}

			if (NULL != gSmoothedMovement)
			{
				M3tPoint3D smooth = MUgZeroPoint;
				UUtInt32 smooth_itr;

				gSmoothedMovement[ONgGameState->gameTime % gDebugCharacterMovement] = moved;

				if (0 == (ONgGameState->gameTime % gDebugCharacterMovement)) {
					for(smooth_itr = 0; smooth_itr < gDebugCharacterMovement; smooth_itr++)
					{
						MUmVector_Add(smooth, smooth, gSmoothedMovement[smooth_itr]);
					}

					MUmVector_Scale(smooth, 1.f/((float) gDebugCharacterMovement));

					COrConsole_Printf("movement = %4.4f,%4.4f", smooth.x, smooth.z);
				}
			}
		}

		MUmAssertVector(moved, 1000.f);

		MUrPoint_RotateYAxis(&moved, ioCharacter->facing, &moved);

		/*
		 * knockback
		 */

		if (active_character->inAirControl.numFramesInAir > 0) {
			// characters in air don't use knockback, they have stored velocity
			active_character->knockback = MUgZeroVector;
		}
		else {
			float knockback_percent_per_frame = 0.07f;
			float knockback_percent_remaining = 1.f - knockback_percent_per_frame;
			M3tPoint3D knockback_this_frame;

			knockback_this_frame = active_character->knockback;
			MUmVector_Scale(knockback_this_frame, knockback_percent_per_frame);
			MUmVector_Add(moved, moved, knockback_this_frame);

			MUmVector_Scale(active_character->knockback, knockback_percent_remaining);

			if (MUmVector_GetLengthSquared(active_character->knockback) < UUmSQR(0.1f))
			{
				active_character->knockback = MUgZeroVector;
			}
		}

		/*
		 * animation and film movement
		 */

		if (active_character->physics && active_character->physics->animContext.animation) {
			M3tMatrix4x3 animMatrix;

			OBrAnim_Matrix(&active_character->physics->animContext, &animMatrix);

			if (0 == (active_character->physics->animContext.animContextFlags & OBcAnimContextFlags_NoRotation)) {
				InverseMaxMatrix(&animMatrix);
			}

			//MUrMatrix_StripScale(&animMatrix, &animMatrix);
			active_character->physics->position = MUrMatrix_GetTranslation(&animMatrix);
			active_character->physics->position.y -= ioCharacter->heightThisFrame;
		}

		if (ONrCharacter_IsPlayingFilm(ioCharacter) && (active_character->filmState.flags & ONcFilmFlag_Interp)) {
			if (active_character->physics != NULL) {
				// interpolate the character's position so that they get onto the film start point.
				// NOTE: this bypasses all collision routines!
				MUmVector_Add(active_character->physics->position, active_character->physics->position, active_character->filmState.interp_delta);
			} else {
				MUmVector_Add(ioCharacter->location, ioCharacter->location, active_character->filmState.interp_delta);
			}
			UUmAssert(active_character->filmState.interp_frames > 0);
			active_character->filmState.interp_frames--;

			if (active_character->filmState.interp_frames == 0) {
				// interpolation complete
				active_character->filmState.flags &= ~ONcFilmFlag_Interp;
			}
		}

		/*
		 * apply movement
		 */

		// used for velocity estimation
		ioCharacter->prev_location = ioCharacter->location;
		if (active_character->physics != NULL) {
			ioCharacter->location = active_character->physics->position;
		}

		ONrCharacter_SetMovementThisFrame(ioCharacter, active_character, &moved);

		// should we pin this character
		if (gPinCharacters || active_character->frozen) {
			active_character->movementThisFrame = MUgZeroPoint;
			active_character->gravityThisFrame = 0.f;
		}

		/***************** POST-MOVEMENT ANIMATION */

		// determine if this is a sound frame
		is_sound_frame = TRrAnimation_IsSoundFrame(active_character->animation, active_character->animFrame);
		new_sound_name = TRrAnimation_GetNewSoundForFrame(active_character->animation, active_character->animFrame);

		if (active_character->animFrame == active_character->animation->actionFrame) {
			// if we are throwing then the animation action frame is swap weapon
			if (active_character->throwTarget != NULL) {
				WPtWeapon *target_weapon = active_character->throwTarget->inventory.weapons[0];
				WPtWeapon *stowed_weapon = WPrSlot_FindLastStowed(&ioCharacter->inventory, WPcPrimarySlot, NULL);
				UUtBool can_steal_gun = (NULL == ioCharacter->inventory.weapons[0]) && (NULL == stowed_weapon);

				if ((NULL != target_weapon) && can_steal_gun) {
					// move to new character
					ONrCharacter_DropWeapon(active_character->throwTarget, UUcFalse, WPcPrimarySlot, UUcTrue);
					ONrCharacter_PickupWeapon(ioCharacter, target_weapon, UUcFalse);
				}
				else if (NULL != target_weapon) {
					ONrCharacter_DropWeapon(active_character->throwTarget, UUcTrue, WPcPrimarySlot, UUcFalse);
				}
			}

			if (ioCharacter->charType == ONcChar_AI2) {
				// notify the AI that the action frame has been reached
				AI2rNotifyActionFrame(ioCharacter, active_character->curAnimType);
			}
		}

		if (active_character->numParticles > 0)
		{
			UUtUns16 new_active_particles = TRrAnimation_GetActiveParticles(active_character->animation, active_character->animFrame);
			ONtCharacterParticleInstance *particle;
			float current_time = ONrGameState_GetGameTime() / ((float) UUcFramesPerSecond);

			if (new_active_particles != active_character->activeParticles) {
				IMtShade health_tint;

				// some particles have changed state
				health_tint = ONiCharacter_GetAnimParticleTint(ioCharacter, active_character);

				for (itr = 0, particle = active_character->particles; itr < active_character->numParticles; itr++, particle++) {
					if (particle->particle_def == NULL) {
						// this is an animation particle whose definition was not found in the character class
						// however it must still be here because particles are referred to by their place in the array
						// by the animation system's active-masks
						continue;
					}

					if ((particle->particle_def->override_partindex == ONcCharacterParticle_HitFlash) &&
						(new_active_particles & (1 << itr)) && ((active_character->activeParticles & (1 << itr)) == 0)) {
						ONtCharacter *thrown_character = NULL, *other_character = NULL;

						if (active_character->throwTarget != NULL) {
							thrown_character = active_character->throwTarget;
							other_character = ioCharacter;
						} else if (active_character->thrownBy != (UUtUns16) -1) {
							thrown_character = ioCharacter;
							UUmAssert((active_character->thrownBy >= 0) && (active_character->thrownBy < ONgGameState->numCharacters));
							other_character = &ONgGameState->characters[active_character->thrownBy];
						}

						// this animation particle is a hitflash particle and we want to generate a start event
						if ((thrown_character != NULL) && (thrown_character->flags & ONcCharacterFlag_InUse) &&
							(thrown_character->flags & ONcCharacterFlag_Dead)) {
							// we generate a 'killed' hitflash instead of actually starting the particle
							M3tPoint3D part_location;
							MAtImpactType impact_type;
							MAtMaterialType material_type;
							P3tEffectData effect_data;

							if ((particle->particle != NULL) && (particle->particle_ref == particle->particle->header.self_ref)) {
								// delete the particle attached here, since our target is dead and this is a hitflash particle
								// we won't be needed again once we generate the 'killed' hit flash
								P3rKillParticle(particle->particle_def->particle_class, particle->particle);
							}
							particle->particle = NULL;
							particle->particle_ref = P3cParticleReference_Null;

							if (thrown_character->flags & ONcCharacterFlag_FinalFlash) {
								// the final flash has already been played for this character, don't play it again
								continue;
							}

							// set up information for the impact effect
							material_type = ONrCharacter_GetMaterialType(ioCharacter, particle->bodypart_index, UUcTrue);
							impact_type = MArImpactType_GetByName("Self_Damage_Killed");
							if (impact_type == MAcImpact_Base) {
								continue;
							}
							ONrCharacter_GetPartLocation(ioCharacter, active_character, (UUtUns16) particle->bodypart_index, &part_location);

							// set up effect data and play the appropriate impact effect
							UUrMemory_Clear(&effect_data, sizeof(effect_data));
							effect_data.cause_type	= P3cEffectCauseType_MeleeHit;
							effect_data.time		= ONrGameState_GetGameTime();
							effect_data.owner		= WPrOwner_MakeMeleeDamageFromCharacter(ioCharacter);
							effect_data.position	= part_location;
							effect_data.normal		= ioCharacter->facingVector;
							effect_data.parent_velocity = &effect_data.normal;

							ONrImpactEffect(impact_type, material_type, ONcIEModType_MediumDamage, &effect_data, 1.0f, thrown_character, other_character);
							thrown_character->flags |= ONcCharacterFlag_FinalFlash;
							continue;
						}
					}

					// don't check dead particles
					if ((particle->particle == NULL) || (particle->particle_ref != particle->particle->header.self_ref))
						continue;

					// update the tint on this particle to match the character's current health if necessary
					P3rSetTint(particle->particle_def->particle_class, particle->particle, health_tint);

					if (new_active_particles & (1 << itr)) {
						if ((active_character->activeParticles & (1 << itr)) == 0) {
							// activate this particle
							P3rSendEvent(particle->particle_def->particle_class, particle->particle, P3cEvent_Start,
										current_time);
						}
					} else {
						if (active_character->activeParticles & (1 << itr)) {
							// deactivate this particle
							P3rSendEvent(particle->particle_def->particle_class, particle->particle, P3cEvent_Stop,
										current_time);
						}
					}
				}

				active_character->activeParticles = new_active_particles;
			}
		}

		{
			float desired_super;

			// determine the character's super amount - can be controlled via scripting or (optionally) overhypo
			desired_super = ioCharacter->external_super_amount;
			if ((ioCharacter->characterClass->superHypo) && (ioCharacter->hitPoints > ioCharacter->maxHitPoints)) {
				float health_super;

				health_super = ((float) (ioCharacter->hitPoints - ioCharacter->maxHitPoints)) / ioCharacter->maxHitPoints;
				desired_super = UUmMax(desired_super, health_super);
			}
			desired_super = UUmMax(desired_super, 0.0f);

			if (ioCharacter->super_amount < desired_super) {
				ioCharacter->super_amount = UUmMin(ioCharacter->super_amount + ONcSuperAmount_IncreasePerFrame, desired_super);
			} else {
				ioCharacter->super_amount = UUmMax(ioCharacter->super_amount - ONcSuperAmount_DecreasePerFrame, desired_super);
			}

			// update super particles
			if (ioCharacter->super_amount > 0) {
				if (!active_character->superParticlesCreated) {
					ONiCharacter_SuperParticle_Create(ioCharacter, active_character);
				}

				if (!active_character->superParticlesActive) {
					ONiCharacter_SuperParticle_SendEvent(ioCharacter, active_character, P3cEvent_Start);
				}
			} else {
				if ((active_character->superParticlesCreated) && (active_character->superParticlesActive)) {
					ONiCharacter_SuperParticle_SendEvent(ioCharacter, active_character, P3cEvent_Stop);
				}
			}
		}

		{
			UUtUns32 overlay_index;

			for(overlay_index = 0; overlay_index < ONcOverlayIndex_Count; overlay_index++) {
				UUtBool overlay_finished;

				if (!ONrOverlay_IsActive(active_character->overlay + overlay_index)) {
					continue;
				}

				if (active_character->overlay[overlay_index].frame == active_character->overlay[overlay_index].animation->actionFrame) {
					switch(overlay_index)
					{
						case ONcOverlayIndex_Recoil:
						break;

						case ONcOverlayIndex_InventoryManagement:
							switch(active_character->overlay[overlay_index].animation->type)
							{
								case ONcAnimType_Reload_Pistol:
								case ONcAnimType_Reload_Rifle:
								case ONcAnimType_Reload_Stream:
								case ONcAnimType_Reload_Superball:
								case ONcAnimType_Reload_Vandegraf:
								case ONcAnimType_Reload_Scram_Cannon:
								case ONcAnimType_Reload_MercuryBow:
								case ONcAnimType_Reload_Screamer:
									if (ioCharacter->inventory.weapons[0] != NULL) {
										WPrNotifyReload(ioCharacter->inventory.weapons[0]);
									}
								break;

								case ONcAnimType_Holster:
									if (ioCharacter->inventory.weapons[0] != NULL) {
										UUtBool magic_holster = UUcFalse;

										if (active_character->overlay[overlay_index].flags & ONcOverlayFlag_MagicHolster) {
											magic_holster = UUcTrue;
										}

										ONrCharacter_PickupWeapon(ioCharacter, NULL, magic_holster);
									}

									if (ioCharacter->inventory.weapons[0] != NULL) {
										ONrCharacter_SetWeaponVarient(ioCharacter, active_character);

										ONrCharacter_Draw(ioCharacter, active_character, ioCharacter->inventory.weapons[0]);

										if (active_character->overlay[overlay_index].animation->actionFrame > 0) {
											active_character->overlay[overlay_index].frame = active_character->overlay[overlay_index].animation->actionFrame;
										}
									}
								break;

								case ONcAnimType_Draw_Pistol:
								case ONcAnimType_Draw_Rifle:
									if (NULL == ioCharacter->inventory.weapons[0]) {
										ONrCharacter_PickupWeapon(ioCharacter, NULL, UUcFalse);
									}
								break;

								default:
									COrConsole_Printf("unknown inventory management animation type");
								break;
							}
						break;
					}
				}

				overlay_finished = ONrOverlay_DoFrame(ioCharacter, active_character->overlay + overlay_index);

				if (overlay_finished) {
					switch(overlay_index)
					{
						case ONcOverlayIndex_Recoil:
						break;

						case ONcOverlayIndex_InventoryManagement:
						break;
					}
				}
			}
		}

		// play the appropriate sound for this animation
		if (is_sound_frame) {
			OSrSoundAnimation_Play(
				ioCharacter->characterClass->variant,
				active_character->animation,
				&ioCharacter->actual_position,		/* location won't be NULL like physics might */
				NULL,
				NULL);
		}

		if (new_sound_name != NULL)
		{
			OSrImpulse_PlayByName(
				new_sound_name,
				&ioCharacter->actual_position,
				NULL,
				NULL,
				NULL);
		}

		// play footsteps
		{
			TRtFootstepKind footstep = TRrGetFootstep(active_character->animation, active_character->animFrame);

			if (footstep != TRcFootstep_None) {
				if (active_character->inAirControl.numFramesInAir == 0) {

					if (ONgSpatialFootsteps) {
						M3tPoint3D footstep_location;
						switch(footstep)
						{
							case TRcFootstep_Left:
								footstep_location = MUrMatrix_GetTranslation(active_character->matricies + ONcLFoot_Index);
								footstep_location.y = ioCharacter->actual_position.y + 0.5f;
							break;

							case TRcFootstep_Right:
								footstep_location = MUrMatrix_GetTranslation(active_character->matricies + ONcRFoot_Index);
								footstep_location.y = ioCharacter->actual_position.y + 0.5f;
							break;

							default:
								footstep_location = ioCharacter->actual_position;
						}

						ONrCharacter_PlayAnimationImpact(ioCharacter, active_character, footstep, &footstep_location);
					}
					else {
						ONrCharacter_PlayAnimationImpact(ioCharacter, active_character, footstep, NULL);
					}
				} else {
					COrConsole_Printf("footstep not played, not standing on ground");
				}

				if (ONgDebugFootstepFlash) {
					active_character->flash_count = 5;
					switch(footstep)
					{
						case TRcFootstep_Left:
							active_character->flash_parts = (1 << ONcLFoot_Index);
						break;

						case TRcFootstep_Right:
							active_character->flash_parts = (1 << ONcRFoot_Index);
						break;

						case TRcFootstep_Single:
							active_character->flash_parts = (1 << ONcRFoot_Index) | (1 << ONcLFoot_Index);
						break;
					}
				}
			} else {
				if (ONgDebugVerboseFootsteps) {
					COrConsole_Printf("%s(%s) %d", TMrInstance_GetInstanceName(active_character->animation),
									ONrAnimTypeToString(active_character->curAnimType), active_character->animFrame);
				}
			}
		}

		duration = TRrAnimation_GetDuration(active_character->animation);

		// play animation alert sounds (for AI)
		{
			if ((ioCharacter->animCounter != active_character->alertSoundAnimCounter) &&
				(active_character->animFrame >= (duration / 2))) {
				UUtUns32 sound_type = (UUtUns32) -1;
				float sound_radius;

				switch(active_character->curAnimType) {
					case ONcAnimType_Crouch_Forward:
					case ONcAnimType_Crouch_Right:
					case ONcAnimType_Crouch_Left:
					case ONcAnimType_Crouch_Back:
					case ONcAnimType_Slide:
						sound_type = AI2cContactType_Sound_Ignore;
						sound_radius = 60.0f;
					break;

					case ONcAnimType_Taunt:
						sound_type = AI2cContactType_Sound_Interesting;
						sound_radius = 150.0f;
					break;
				}

				if (sound_type != (UUtUns32) -1) {
					AI2rKnowledge_Sound(sound_type, &ioCharacter->location, sound_radius, ioCharacter, NULL);
				}

				// don't play sound again for this animation
				active_character->alertSoundAnimCounter = ioCharacter->animCounter;
			}
		}

		active_character->animFrame += 1;

		if (active_character->stitch.stitching) {
			active_character->stitch.itr += 1;

			if (active_character->stitch.itr == active_character->stitch.count) {
				active_character->stitch.stitching = UUcFalse;
			}
		}

		if (active_character->fightFramesRemaining > 0) {
			active_character->fightFramesRemaining -= 1;
		}

		// did we reach the end of this animation ?
		if (TRrAnimation_GetDuration(active_character->animation) == active_character->animFrame) {
			ONrCharacter_NextAnimation(ioCharacter, active_character);
		}

		// is this animation hasty and we are done with the waiting period
		if ((ioCharacter->flags & ONcCharacterFlag_HastyAnim) && (!TRrAnimation_IsAtomic(active_character->animation, active_character->animFrame))) {
			ONrCharacter_NextAnimation(ioCharacter, active_character);
		}

		// pose update
		ONrCharacter_DoAiming(ioCharacter, active_character);
		ONrCharacter_BuildQuaternions(ioCharacter, active_character);
		ONrCharacter_BuildMatriciesAll(ioCharacter, active_character);

		// build the trigger points
		active_character->trigger_points[0] = ioCharacter->location;
		active_character->trigger_points[1] = ioCharacter->location;
		active_character->trigger_points[0].y = ioCharacter->feetLocation;
		active_character->trigger_points[1].y = ioCharacter->boundingBox.maxPoint.y;
		ONiCharacter_BuildTriggerSphere(active_character);

		// effects
		ONrCharacter_Shield_Update(ioCharacter, active_character);
		ONrCharacter_Invis_Update(ioCharacter, active_character);

		// motion blur (must come after matrices are built)
		if (!active_character->invis_active) {
			UUtUns32 blur_parts;
			UUtUns8 blur_quantity;
			UUtUns8 blur_amount;

			TRrAnimation_GetBlur(active_character->animation, active_character->animFrame, &blur_parts, &blur_quantity, &blur_amount);

			if (blur_parts != 0)
			{
				TRtBody *body = ioCharacter->characterClass->body->body[TRcBody_SuperLow];
				TRtBodyTextures *textures = ioCharacter->characterClass->textures;
				UUtUns32 blur_itr;

				for(blur_itr = 0; blur_itr < ONcNumCharacterParts; blur_itr++)
				{
					ONtMotionBlur motion_blur;

					motion_blur.geometry = body->geometries[blur_itr];
					motion_blur.texture = textures->maps[blur_itr];
					motion_blur.matrix = active_character->matricies[blur_itr];
					motion_blur.date_of_birth = ONgGameState->gameTime;
					motion_blur.lifespan = blur_quantity;
					motion_blur.blur_base = ((float) (blur_amount)) / ((float) 255.f);

					ONrGameState_MotionBlur_Add(&motion_blur);
				}
			}
		}

		/***************** FINAL INPUT */

		if (active_character->pleaseFire) {
			ONiCharacter_Fire(ioCharacter, active_character, ONcFireMode_Single);
		}
		else if (active_character->pleaseAutoFire) {
			ONiCharacter_Fire(ioCharacter, active_character, ONcFireMode_Autofire);
		}
		else if (active_character->pleaseFireAlternate) {
			ONiCharacter_Fire(ioCharacter, active_character, ONcFireMode_Alternate);
		}
		else if (active_character->pleaseContinueFiring) {
			ONiCharacter_Fire(ioCharacter, active_character, ONcFireMode_Continuous);
		}

		if (active_character->pleaseReleaseTrigger) {
			ONiCharacter_StopFiring(ioCharacter, active_character);
			active_character->pleaseReleaseTrigger = UUcFalse;
		}
		if (active_character->pleaseReleaseAlternateTrigger) {
			ONiCharacter_StopFiring(ioCharacter, active_character);
			active_character->pleaseReleaseAlternateTrigger = UUcFalse;
		}

		/***************** NEXT FRAME PREPARATION */

		// zero collision counters for the next frame
		active_character->numWallCollisions = 0;
		active_character->numPhysicsCollisions = 0;
		active_character->numCharacterCollisions = 0;
		active_character->collidingWithTarget = UUcFalse;
	}

	/***************** FINAL UPDATE */

	// get the character's actual position (can be used to test which BNV a character is in, or for other pathfinding
	// purposes). guaranteed not to be below the floor and hopefully not so far above the floor that BNV issues occur.
	if (active_character == NULL) {
		ioCharacter->actual_position = ioCharacter->location;
	} else {
		ioCharacter->actual_position = MUrMatrix_GetTranslation(&active_character->matricies[ONcPelvis_Index]);
		if ((active_character->offGroundFrames == 0) && (active_character->inAirControl.numFramesInAir == 0) &&
			((ioCharacter->flags & ONcCharacterFlag_NoCollision) == 0)) {
			// the character is touching the ground... make sure that the point we use isn't
			// below the ground
			ioCharacter->actual_position.y = UUmMax(ioCharacter->feetLocation, active_character->groundPoint.y);
		} else {
			// use character's feet location
			ioCharacter->actual_position.y = ioCharacter->feetLocation;
		}
	}
	ioCharacter->actual_position.y += PHcWorldCoord_YOffset;

	// CB: this is used for particle emission
	if (weapon_matrixptr == NULL) {
		MUmVector_Set(ioCharacter->weaponMovementThisFrame, 0, 0, 0);
	} else {
		new_weapon_location = MUrMatrix_GetTranslation(weapon_matrixptr);
		MUmVector_Subtract(ioCharacter->weaponMovementThisFrame, new_weapon_location, previous_weapon_location);
	}

	// check to see if we have to play any hurt sounds
	ONiCharacter_PainUpdate(ioCharacter);

	return;
}

extern UUtUns32 TRgCollectionLookupCounter;

static void ONrGameState_DoCharacterDebugFrame(
					ONtCharacter *ioCharacter)
{

	const TRtAnimation *blockAnim;
	UUtBool canBlock;
	UUtUns32 idleTime = ONgGameState->gameTime - ioCharacter->lastActiveAnimationTime;
	UUtUns32 idleWait = ioCharacter->idleDelay;
	const char *blockString;
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
	const char *atomicString;
	UUtUns64 keys_high;
	UUtUns64 keys_low;
	UUtUns32 death_level;
	UUtInt32 itr;

	atomicString = (active_character == NULL) ? "INACTIVE" :
					(TRrAnimation_IsAtomic(active_character->animation, active_character->animFrame)) ? "ATOMIC" : "NOTATOMIC";

	blockAnim = (active_character == NULL) ? NULL : ONiAnimation_FindBlock(ioCharacter, active_character);
	canBlock = ONrCharacter_IsDefensive(ioCharacter) && (NULL != blockAnim);
	blockString = canBlock ? "BLOCK" : "NOBLOCK";

	keys_high = (active_character == NULL) ? 0 : active_character->inputState.buttonIsUp;
	keys_low = (active_character == NULL) ? 0 : active_character->inputState.buttonIsDown;

	if (NULL == ioCharacter)
	{
		death_level = 0;
	}
	else if (ioCharacter->flags & ONcCharacterFlag_Dead_4_Gone)
	{
		death_level = 4;
	}
	else if (ioCharacter->flags & ONcCharacterFlag_Dead_3_Cosmetic)
	{
		death_level = 3;
	}
	else if (ioCharacter->flags & ONcCharacterFlag_Dead_2_Moving)
	{
		death_level = 2;
	}
	else if (ioCharacter->flags & ONcCharacterFlag_Dead_1_Animating)
	{
		death_level = 1;
	}
	else
	{
		death_level = 0;
	}

	sprintf(ONgChrStatus[1].text, "face = %f fight = %d idle = %d/%d", ioCharacter->facing * (360.f / M3c2Pi), (active_character == NULL) ? -1 : active_character->fightFramesRemaining, idleTime, idleWait);
	sprintf(ONgChrStatus[2].text, "%3.3f, %3.3f, %3.3f", ioCharacter->location.x, ioCharacter->location.y, ioCharacter->location.z);
	sprintf(ONgChrStatus[3].text, "lookups %d %s %s", TRgCollectionLookupCounter, blockString, atomicString);

	TRgCollectionLookupCounter = 0;

	if (active_character == NULL) {
		sprintf(ONgChrStatus[4].text, "inactive, no anim");
		sprintf(ONgChrStatus[5].text, "inactive, no aiming screen");

	} else {
		if (active_character->stitch.stitching) {
			sprintf(ONgChrStatus[4].text, "%d/%d %s %d/%d",active_character->stitch.itr, active_character->stitch.count,
					active_character->animation->instanceName, active_character->animFrame, TRrAnimation_GetDuration(active_character->animation));

		} else {
			sprintf(ONgChrStatus[4].text, "%s %d/%d", active_character->animation->instanceName, active_character->animFrame,
					TRrAnimation_GetDuration(active_character->animation));
		}
		sprintf(ONgChrStatus[5].text, "%s", (active_character->aimingScreen != NULL) ? active_character->aimingScreen->animation->instanceName : "no aiming screen");
	}

	sprintf(ONgChrStatus[6].text, "draw = %d death = %d air = %d off = %d", ONgNumCharactersDrawn, death_level,
			(active_character == NULL) ? -1 : active_character->inAirControl.numFramesInAir, (active_character == NULL) ? -1 : active_character->offGroundFrames);
	sprintf(ONgChrStatus[7].text, "%16x%16x [%04x]", keys_high, keys_low, (active_character == NULL) ? 0 : active_character->animVarient);
	sprintf(ONgChrStatus[8].text, "alive %d active %d present %d in use %d", ONgGameState->numLivingCharacters, ONgGameState->numActiveCharacters, ONgGameState->numPresentCharacters, ONgGameState->numCharacters);

	if (active_character != NULL) {
		for(itr = 0; itr < ONcOverlayIndex_Count; itr++)
		{
			ONtOverlay *overlay = active_character->overlay + itr;

			if (ONrOverlay_IsActive(overlay)) {
				sprintf(ONgChrOverlayStatus[itr + 1].text, "%d %s %d/%d", itr, overlay->animation->instanceName , overlay->frame, overlay->animation->duration);
			}
			else {
				sprintf(ONgChrOverlayStatus[itr + 1].text, "");
			}
		}
	} else {
		for(itr = 0; itr < ONcOverlayIndex_Count; itr++)
		{
			strcpy(ONgChrOverlayStatus[itr + 1].text, "");
		}
	}

	return;
}


UUtBool ONrGameState_TriggerDisabled(OBJtOSD_TriggerVolume *inVolume, ONtCharacter *inCharacter)
{
	return ((inVolume->in_game_flags & OBJcOSD_TriggerVolume_Disabled) ||
			((inVolume->in_game_flags & OBJcOSD_TriggerVolume_PlayerOnly) && (inCharacter->charType != ONcChar_Player)));
}

void ONrGameState_FireTrigger_Entry(OBJtOSD_TriggerVolume *inVolume, ONtCharacter *inCharacter)
{
	if ((inVolume->cur_entry_script[0] != '\0') & (!(inVolume->in_game_flags & OBJcOSD_TriggerVolume_Enter_Disabled)))
	{
		UUmAssertReadPtr(inVolume, sizeof(*inVolume));
		if (gDebugTriggerVolume) {
			COrConsole_Printf("trigger %s %d entered by %s", inVolume->name, inVolume->id, inCharacter->player_name);
		}

		if (inVolume->in_game_flags & OBJcOSD_TriggerVolume_Enter_Once) {
			inVolume->in_game_flags |= OBJcOSD_TriggerVolume_Enter_Disabled;
		}

		ONrCharacter_Defer_Script(inVolume->cur_entry_script, inCharacter->player_name, inVolume->name);
	}
}

void ONrGameState_FireTrigger_Inside(OBJtOSD_TriggerVolume *inVolume, ONtCharacter *inCharacter)
{
	if ((inVolume->cur_inside_script[0] != '\0') & (!(inVolume->in_game_flags & OBJcOSD_TriggerVolume_Inside_Disabled))) {

		UUmAssertReadPtr(inVolume, sizeof(*inVolume));
		if (gDebugTriggerVolume) {
			COrConsole_Printf("trigger %s %d inside by %s", inVolume->name, inVolume->id, inCharacter->player_name);
		}

		if (inVolume->in_game_flags & OBJcOSD_TriggerVolume_Inside_Once) {
			inVolume->in_game_flags |= OBJcOSD_TriggerVolume_Inside_Disabled;
		}

		ONrCharacter_Defer_Script(inVolume->cur_inside_script, inCharacter->player_name, inVolume->name);
	}
}

void ONrGameState_FireTrigger_Exit(OBJtOSD_TriggerVolume *inVolume, ONtCharacter *inCharacter)
{
	if ((inVolume->cur_exit_script[0] != '\0') & (!(inVolume->in_game_flags & OBJcOSD_TriggerVolume_Exit_Disabled))) {
		UUmAssertReadPtr(inVolume, sizeof(*inVolume));
		if (gDebugTriggerVolume) {
			COrConsole_Printf("trigger %s %d exited by %s", inVolume->name, inVolume->id, inCharacter->player_name);
		}

		if (inVolume->in_game_flags & OBJcOSD_TriggerVolume_Exit_Once) {
			inVolume->in_game_flags |= OBJcOSD_TriggerVolume_Exit_Disabled;
		}

		ONrCharacter_Defer_Script(inVolume->cur_exit_script, inCharacter->player_name, inVolume->name);
	}

	return;
}

UUtInt32 ONrGameState_CountInsideTrigger(OBJtObject *inObject)
{
	OBJtOSD_All				*inOSD = (OBJtOSD_All *) inObject->object_data;
	OBJtOSD_TriggerVolume	*trigger_osd = &inOSD->osd.trigger_volume_osd;

	ONtCharacter **active_character_list = ONrGameState_ActiveCharacterList_Get();
	UUtUns32 active_character_count = ONrGameState_ActiveCharacterList_Count();
	UUtInt32 inside_trigger_count = 0;
	UUtUns32 team_mask = trigger_osd->team_mask;

	UUtUns32 itr;

	for(itr = 0; itr < active_character_count; itr++)
	{
		ONtCharacter *current_character = active_character_list[itr];
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(current_character);

		if (current_character->flags & ONcCharacterFlag_Dead) {
			continue;
		}

		if (OBJrTriggerVolume_IntersectsCharacter(inObject, team_mask, current_character)) {
			inside_trigger_count++;
		}
	}

	return inside_trigger_count;
}



void ONrGameState_KillCorpsesInsideTrigger(OBJtObject *inObject)
{
	{
		ONtCorpse *corpses = ONgLevel->corpseArray->corpses;
		UUtUns32 count = ONgLevel->corpseArray->max_corpses;
		UUtUns32 static_count = ONgLevel->corpseArray->static_corpses;
		UUtUns32 itr;

		for(itr = static_count; itr < count; itr++)
		{
			ONtCorpse *this_corpse = corpses + itr;

			if (OBJrTriggerVolume_IntersectsBBox_Approximate(inObject, &this_corpse->corpse_data.corpse_bbox)) {
				this_corpse->corpse_data.characterClass = NULL;
			}
		}
	}

	{
		ONtCharacter **active_character_list = ONrGameState_ActiveCharacterList_Get();
		UUtUns32 active_character_count = ONrGameState_ActiveCharacterList_Count();
		UUtUns32 team_mask = 0xFFFFFFFF;
		UUtUns32 itr;

		for(itr = 0; itr < active_character_count; itr++)
		{
			ONtCharacter *current_character = active_character_list[itr];
			ONtActiveCharacter *active_character = ONrGetActiveCharacter(current_character);

			if (current_character->flags & ONcCharacterFlag_Dead) {
				if (OBJrTriggerVolume_IntersectsCharacter(inObject, team_mask, current_character)) {
					current_character->death_delete_timer = 1;
				}
			}
		}
	}

	return;
}

void ONrGameState_DeleteCharactersInsideTrigger(OBJtObject *inObject)
{
	ONtCharacter *delete_list[ONcMaxActiveCharacters];
	UUtUns32 delete_count = 0;

	ONtCharacter **active_character_list = ONrGameState_ActiveCharacterList_Get();
	UUtUns32 active_character_count = ONrGameState_ActiveCharacterList_Count();
	UUtUns32 team_mask = 0xFFFFFFFF;
	UUtUns32 itr;

	for(itr = 0; itr < active_character_count; itr++)
	{
		ONtCharacter *current_character = active_character_list[itr];
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(current_character);

		if (OBJrTriggerVolume_IntersectsCharacter(inObject, team_mask, current_character)) {
			delete_list[delete_count++] = current_character;
		}
	}

	for(itr = 0; itr < delete_count; itr++)
	{
		if (delete_list[itr]->charType != ONcChar_Player) {
			ONrGameState_DeleteCharacter(delete_list[itr]);
		}
	}

	return;
}

static UUtBool ONrGameState_UpdateTrigger(OBJtObject *inObject, UUtUns32 inUserData)
{
	ONtCharacter **active_character_list = ONrGameState_ActiveCharacterList_Get();
	UUtUns32 active_character_count = ONrGameState_ActiveCharacterList_Count();
	OBJtOSD_All				*inOSD = (OBJtOSD_All *) inObject->object_data;
	OBJtOSD_TriggerVolume	*trigger_osd = &inOSD->osd.trigger_volume_osd;
	OBJtOSD_TriggerVolume	*parent_trigger_osd = NULL;
	UUtUns32 itr;
	UUtUns32 team_mask = trigger_osd->team_mask;

	if (trigger_osd->parent_id != 0) {
		OBJtOSD_All parent_search_osd;
		OBJtObject *parent_trigger_object;

		parent_search_osd.osd.trigger_volume_osd.id = trigger_osd->parent_id;

		parent_trigger_object = OBJrObjectType_Search(OBJcType_TriggerVolume, OBJcSearch_TriggerVolumeID, &parent_search_osd);

		if (NULL == parent_trigger_object) {
			COrConsole_Printf("trigger %d failed to locate parent trigger %d", trigger_osd->id, trigger_osd->parent_id);
		}
		else {
			OBJtOSD_All *parent_trigger_osd_all = (OBJtOSD_All *) parent_trigger_object->object_data;
			parent_trigger_osd = &parent_trigger_osd_all->osd.trigger_volume_osd;
			team_mask = parent_trigger_osd->team_mask;
		}
	}

	for(itr = 0; itr < active_character_count; itr++)
	{
		ONtCharacter *current_character = active_character_list[itr];
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(current_character);
		UUtInt32 list_itr;
		UUtInt32 list_index = -1;

		UUmAssertReadPtr(active_character, sizeof(*active_character));

		// CB: placing this check here makes update much faster
		if (ONrGameState_TriggerDisabled(trigger_osd, current_character))
			continue;

		// set global trigger debugging variables for accessing via scripts
		ONgCurrentlyActiveTrigger = trigger_osd;
		ONgCurrentlyActiveTrigger_Cause = current_character;

		for(list_itr = 0; list_itr < ONcMaxTriggersPerCharacter; list_itr++)
		{
			if (active_character->current_triggers[list_itr] == trigger_osd->id) {
				list_index = list_itr;
				break;
			}
		}

		if (OBJrTriggerVolume_IntersectsCharacter(inObject, team_mask, current_character)) {
			if (list_index < 0) {
				for(list_itr = 0; list_itr < ONcMaxTriggersPerCharacter; list_itr++)
				{
					if (0 == active_character->current_triggers[list_itr]) {
						active_character->current_triggers[list_itr] = trigger_osd->id;

						ONrGameState_FireTrigger_Entry(trigger_osd, current_character);

						if (NULL != parent_trigger_osd) {
							ONrGameState_FireTrigger_Entry(parent_trigger_osd, current_character);
						}

						break;
					}
				}
			}

			ONrGameState_FireTrigger_Inside(trigger_osd, current_character);

			if (NULL != parent_trigger_osd) {
				ONrGameState_FireTrigger_Inside(parent_trigger_osd, current_character);
			}
		}
		else {
			if (list_index >= 0) {
				UUtBool removed = UUcFalse;

				for(list_itr = 0; list_itr < ONcMaxTriggersPerCharacter; list_itr++)
				{
					if (removed) {
						active_character->current_triggers[list_itr - 1] = active_character->current_triggers[list_itr];
						active_character->current_triggers[list_itr] = 0;
					}
					else if (trigger_osd->id == active_character->current_triggers[list_itr]) {
						active_character->current_triggers[list_itr] = 0;
						removed = UUcTrue;

						ONrGameState_FireTrigger_Exit(trigger_osd, current_character);

						if (NULL != parent_trigger_osd) {
							ONrGameState_FireTrigger_Exit(parent_trigger_osd, current_character);
						}
					}
				}
			}
		}
	}

	ONrCharacter_Commit_Scripts();

	ONgCurrentlyActiveTrigger = NULL;
	ONgCurrentlyActiveTrigger_Cause = NULL;
	ONgCurrentlyActiveTrigger_Script = NULL;

	return UUcTrue;
}

static void ONrGameState_UpdateTriggers(void)
{
#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Enter(ONg_TriggerVolume_Timer);
#endif

	OBJrObjectType_EnumerateObjects(OBJcType_TriggerVolume, ONrGameState_UpdateTrigger, 0);
	// loop through the triggers
	// loop through the characters
	// call the script system

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Exit(ONg_TriggerVolume_Timer);
#endif

	return;
}

void ONrGameState_UpdateCharacters(void)
{
	ONtCharacter *character;
	ONtActiveCharacter *active_character;
	UUtUns32 itr, selfInflictedWound;
	UUtBool must_unlock;

	ONtCharacter **present_character_list;
	UUtUns32 present_character_count;

	ONtCharacter **active_character_list;
	UUtUns32 active_character_count;

	ONrGameState_UpdateTriggers();

	if (ONrDebugKey_WentDown(ONcDebugKey_Freeze)) {
		ONgGameState->local.playerActiveCharacter->frozen = !ONgGameState->local.playerActiveCharacter->frozen;
	}

	if (ONrDebugKey_WentDown(ONcDebugKey_FrameAdvance)) {
		if (ONgGameState->local.playerActiveCharacter->frozen) {
			ONgGameState->local.playerActiveCharacter->animFrame += 1;
			ONgGameState->local.playerActiveCharacter->animFrame %= ONgGameState->local.playerActiveCharacter->animation->numFrames;

			ONrCharacter_BuildQuaternions(ONgGameState->local.playerCharacter, ONgGameState->local.playerActiveCharacter);
			ONrCharacter_BuildMatriciesAll(ONgGameState->local.playerCharacter, ONgGameState->local.playerActiveCharacter);
		}
	}

	if (!gPauseCharacters) return;

	present_character_list = ONrGameState_PresentCharacterList_Get();
	present_character_count = ONrGameState_PresentCharacterList_Count();

	for(itr = 0; itr < present_character_count; itr++) {
		character = present_character_list[itr];
		active_character = ONrGetActiveCharacter(character);

		if (ONgCharactersAllActive && (active_character == NULL)) {
			// debugging - force all characters to be active
			ONrCharacter_MakeActive(character, UUcTrue);
		}

		if (character->death_delete_timer) {
			character->death_delete_timer--;

			if (character->death_delete_timer == 0) {
				// timer has run out, delete the character
				if (character->charType != ONcChar_Player) {
					ONrGameState_DeleteCharacter(character);
					continue;
				}
			}
		}

		if (character->flags & ONcCharacterFlag_Dead_3_Cosmetic) {
			if ((active_character != NULL) &&
				((active_character->shield_active) || (active_character->invis_active) || (character->flags & ONcCharacterFlag_Teleporting))) {
				ONrCharacter_Shield_Update(character, active_character);
				ONrCharacter_Invis_Update(character, active_character);

			} else if (character->charType != ONcChar_Player) {
				if ((active_character == NULL) || (TRcBody_NotDrawn == active_character->curBody) ||
					(TRcBody_SuperLow == active_character->curBody)) {
					ONrCharacter_Die_4_Gone_Hook(character);
				}
			}

		} else if (character->flags & ONcCharacterFlag_Dead_2_Moving) {
			UUtBool ready_for_ground_death = UUcFalse;

			// we can advance if we are inactive, on the ground or falling for too long
			if (NULL == active_character) {
				ready_for_ground_death = UUcTrue;
			}
			else {
				if (active_character->inAirControl.numFramesInAir >= (ONcMaxFramesInAir + 30)) {
					// if we have been falling for to longer just die, no corpse
					character->flags |= ONcCharacterFlag_NoCorpse;
					ready_for_ground_death = UUcTrue;
				}
				else {
					if ((0 == active_character->inAirControl.numFramesInAir) && (0 == active_character->offGroundFrames)) {
						ready_for_ground_death = UUcTrue;
					}

					if (ready_for_ground_death) {
						active_character->death_still_timer++;

						ready_for_ground_death = active_character->death_still_timer > 120;
					}
					else {
						active_character->death_still_timer = 0;
					}

					active_character->death_moving_timer++;

					if (active_character->death_moving_timer > (60 * 15)) {
						COrConsole_Printf("dieing for to long, aborting");

						character->flags |= ONcCharacterFlag_NoCorpse;
						ready_for_ground_death = UUcTrue;
					}
				}
			}

			if (ready_for_ground_death) {
				ONrCharacter_Die_3_Cosmetic_Hook(character);
			}
		}
	}

	must_unlock = ONrCharacter_Lock_Scripts();
	active_character_list = ONrGameState_ActiveCharacterList_Get();
	active_character_count = ONrGameState_ActiveCharacterList_Count();

	for(itr = 0; itr < active_character_count; itr++) {
		character = active_character_list[itr];
		active_character = ONrGetActiveCharacter(character);
		if (active_character == NULL) {
			UUmAssert(!"ONrGameState_UpdateCharacters: inactive character in active character list");
			continue;
		}

		UUmAssert(character->flags & ONcCharacterFlag_InUse);
		if (character->flags & ONcCharacterFlag_Dead_2_Moving) {
			continue;
		}

		ONrCharacter_HandleHeartbeatInput(character, active_character);
	}

	if (must_unlock) {
		ONrCharacter_Unlock_Scripts();
		ONrCharacter_Commit_Scripts();
	}


	must_unlock = ONrCharacter_Lock_Scripts();
	present_character_list = ONrGameState_PresentCharacterList_Get();
	present_character_count = ONrGameState_PresentCharacterList_Count();

	for(itr = 0; itr < present_character_count; itr++) {
		character = present_character_list[itr];

		if (character->flags & ONcCharacterFlag_InUse) {
			ONtCharacterIndexType index = ONrCharacter_GetIndex(character);

			if (gDebugCharacters && (index == gDebugCharacterTarget)) {
				ONrGameState_DoCharacterDebugFrame(character);
			}

			ONrCharacter_GetFacingVector(character, &character->facingVector);
			ONrGameState_DoCharacterFrame(character);
		}
	}
	if (must_unlock) {
		ONrCharacter_Unlock_Scripts();
		ONrCharacter_Commit_Scripts();
	}


	must_unlock = ONrCharacter_Lock_Scripts();
	active_character_list = ONrGameState_ActiveCharacterList_Get();
	active_character_count = ONrGameState_ActiveCharacterList_Count();

	for(itr = 0; itr < active_character_count; itr++) {
		character = active_character_list[itr];
		active_character = ONrGetActiveCharacter(character);
		UUmAssertReadPtr(active_character, sizeof(*active_character));

		if ((active_character->throwTarget) && (0 == active_character->animFrame)) {
			ONtActiveCharacter *active_target = ONrGetActiveCharacter(active_character->throwTarget);
			UUmAssertReadPtr(active_target, sizeof(*active_target));

			if ((!(active_character->throwTarget->flags & ONcCharacterFlag_Dead_1_Animating)) &&
				(active_character->throwTarget->flags & ONcCharacterFlag_InUse))
			{
				ONiCharacter_ThrowPhysics(character, active_character, active_character->throwTarget, active_target);
			}
		}

		if (!(character->flags & ONcCharacterFlag_InUse) ||
			(character->flags & ONcCharacterFlag_Dead)) continue;

		selfInflictedWound = TRrAnimation_GetSelfDamage(active_character->animation, active_character->animFrame);

		if (0 != selfInflictedWound) {
			ONtCharacter *attacker = NULL;

			if (TRrAnimation_TestFlag(active_character->animation, ONcAnimFlag_ThrowTarget)) {
				float potency;
				UUtBool omnipotent;

				attacker = ONgGameState->characters + active_character->thrownBy;
				potency = ONiCharacter_GetAttackPotency(attacker, &omnipotent);

				selfInflictedWound = MUrUnsignedSmallFloat_To_Uns_Round(potency * selfInflictedWound);
				if (omnipotent) {
					selfInflictedWound = UUmMax(selfInflictedWound, character->hitPoints);
				}
			}

			ONrCharacter_TakeDamage(character, selfInflictedWound, 0, NULL, NULL, WPrOwner_MakeMeleeDamageFromCharacter(attacker), ONcAnimType_None);
		}
	}
	if (must_unlock) {
		ONrCharacter_Unlock_Scripts();
		ONrCharacter_Commit_Scripts();
	}

	if (gDoCollision) {
		ONrDoCharacterCollision();
	}

	// post collision stuff
	for(itr = 0; itr < present_character_count; itr++) {
		character = present_character_list[itr];
		active_character = ONrGetActiveCharacter(character);

		ONrCharacter_FindOurNode(character);

		if ((active_character != NULL) && (active_character->physics != NULL)) {
			PHrPhysics_Accelerate(active_character->physics, &active_character->movementThisFrame);
		}
	}

	if (!ONgCharactersAllActive) {
		// if any characters are scheduled to go inactive, make them do so
		active_character_list = ONrGameState_ActiveCharacterList_Get();
		active_character_count = ONrGameState_ActiveCharacterList_Count();
		for (itr = 0; itr < active_character_count; itr++) {
			character = active_character_list[itr];
			active_character = ONrGetActiveCharacter(character);

			character->inactive_timer += 1;
			if (character->inactive_timer < ONcCharacter_InactiveFrames) {
				// haven't been inactive for long enough
				continue;
			}

			if (character->charType != ONcChar_AI2) {
				// never try to deactivate player
				continue;
			}

			if (ONrCharacter_IsPlayingFilm(character)) {
				// don't make inactive while film is running
				continue;
			}

			if (character->death_delete_timer > 0) {
				// don't make inactive during this period
				continue;
			}

			if ((active_character != NULL) && (active_character->inAirControl.numFramesInAir > 0)) {
				// character is in midair, do not deactivate
				continue;
			}

			if (AI2rCurrentlyActive(character)) {
				// AI is busy, do not deactivate
				continue;
			}

			if (character->flags & ONcCharacterFlag_ChrAnimate) {
				// character is doing chr_animate, do not deactivate
				continue;
			}

			if (character->flags & ONcCharacterFlag_ActiveLock) {
				// character is locked active, do not deactivate
				continue;
			}

			if (character->flags & ONcCharacterFlag_Dead) {
				// do not make dead characters inactive, they need to
				// fall, land and get made into corpses
				continue;
			}

			COrConsole_Printf("%s going inactive", character->player_name);

			ONrCharacter_MakeInactive(character);
		}
	}

	return;
}

static void ONiDrawCharacterAngle(
	ONtCharacter*	inCharacter,
	float angle,
	UUtUns16 shade)
{
	M3tPoint3D points[2];

	UUmAssertReadPtr(inCharacter, sizeof(*inCharacter));

	points[0].x = 0;
	points[0].y = 1.f;
	points[0].z = 0;

	points[1].x = 0.f;
	points[1].y = 1.f;
	points[1].z = 10.f;

	MUrPoint_RotateYAxis(points + 0, angle, points + 0);
	MUrPoint_RotateYAxis(points + 1, angle, points + 1);

	MUmVector_Add(points[0], points[0], inCharacter->location);
	MUmVector_Add(points[1], points[1], inCharacter->location);

	M3rGeom_Line_Light(points + 0, points + 1, shade);

	return;
}

void ONrDrawSphere(
	M3tTextureMap	*inTexture,
	float			inScale,
	M3tPoint3D		*inLocation)
{
	UUtError error;
	M3tGeometry *sphere = NULL;
	UUtBool sphereExists;

	UUmAssertReadPtr(inLocation, sizeof(*inLocation));

	if (NULL == inTexture) {
		error = TMrInstance_GetDataPtr(M3cTemplate_TextureMap, "NONE", (void **) &inTexture);
		if (error != UUcError_None) { return; }
	}

	error = TMrInstance_GetDataPtr(M3cTemplate_Geometry, "sphere", (void **) &sphere);
	sphereExists = (UUcError_None == error) && (NULL != sphere);
	COmAssert(sphereExists);
	if (!sphereExists) { return; }

	sphere->baseMap = inTexture;
	sphere->geometryFlags |= M3cGeometryFlag_SelfIlluminent;

	M3rGeom_State_Set(M3cGeomStateIntType_Alpha, 255);
	M3rGeom_State_Commit();

	M3rMatrixStack_Push();
		M3rMatrixStack_ApplyTranslate(*inLocation);
		M3rMatrixStack_UniformScale(inScale);
		M3rGeometry_Draw(sphere);
	M3rMatrixStack_Pop();

	return;
}

static void ONiDrawAimingVector(
	ONtCharacter*	inCharacter)
{
	M3tPoint3D points[2];
	float lrangle;
	M3tVector3D localVector;
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

	UUmAssertReadPtr(inCharacter, sizeof(*inCharacter));

	if ((active_character == NULL) || (!active_character->isAiming)) {
		return;
	}

	localVector = active_character->aimingVector;

	MUrVector_SetLength(&localVector, 300.f);

	points[0] = inCharacter->actual_position;
	points[0].y += 8.0f;
	points[1] = points[0];
	MUmVector_Increment(points[1],localVector);

	lrangle = active_character->aimingLR;
	UUmTrig_ClipLow(lrangle);

	lrangle += ONrCharacter_GetCosmeticFacing(inCharacter);
	UUmTrig_ClipHigh(lrangle);

	M3rGeom_Line_Light(points + 0, points + 1, (0x1f << 10) | (0x1f << 5));
	ONiDrawCharacterAngle(inCharacter, lrangle, 0x1f);

	return;
}

static void ONiDrawFacingVector(
	ONtCharacter*	inCharacter)
{
	float facing = ONrCharacter_GetCosmeticFacing(inCharacter);

	ONiDrawCharacterAngle(inCharacter, facing, 0x1f << 5);

	return;
}

static void ONiHolograph_Lighting(void)
{
	UUtUns32 green = (ONgGameState->gameTime % 64) >> 1;
	float float_green;

	if (green < 16) {
		green += 15;
	}
	else {
		green = 32 - green;
		green += 15;
	}

	// green is 0..31
	float_green = green * (1.f / 31.f);
	ONrGameState_ConstantColorLighting(0, float_green, 0);

	return;
}


#if DEBUG_ATTACKEXTENT
static TRtAnimation *	ONgTempDebug_AttackExtent_Animation = NULL;
static UUtUns32			ONgTempDebug_AttackExtent_CurTime;
static UUtUns32			ONgTempDebug_AttackExtent_Frame;
static float			ONgTempDebug_AttackExtent_Facing;
static float			ONgTempDebug_AttackExtent_CosTheta;
static float			ONgTempDebug_AttackExtent_SinTheta;
static M3tPoint3D		ONgTempDebug_AttackExtent_BasePt;
static float			ONgTempDebug_AttackExtent_CurDelta[2];

static UUtError
ONiTempDebug_AttackExtent(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	ONtCharacter *player_char = ONrGameState_GetPlayerCharacter();
	TRtAnimation *animation;
	UUtError error;

	if (!player_char) {
		COrConsole_Printf("### no player character");
		return UUcError_Generic;
	}

	UUmAssert(inParameterListLength >= 1);
	error = TMrInstance_GetDataPtr(TRcTemplate_Animation, inParameterList[0].val.str, (void **) &animation);
	if ((error != UUcError_None) || (animation == NULL)) {
		COrConsole_Printf("### can't find animation '%s'", inParameterList[0].val.str);
		return UUcError_Generic;
	}

	ONgTempDebug_AttackExtent_Animation = animation;
	ONgTempDebug_AttackExtent_CurTime = ONrGameState_GetGameTime();
	ONgTempDebug_AttackExtent_Frame = 0;
	ONgTempDebug_AttackExtent_Facing = player_char->facing;
	ONgTempDebug_AttackExtent_SinTheta = MUrSin(ONgTempDebug_AttackExtent_Facing);
	ONgTempDebug_AttackExtent_CosTheta = MUrCos(ONgTempDebug_AttackExtent_Facing);
	ONgTempDebug_AttackExtent_BasePt = player_char->location;
	ONgTempDebug_AttackExtent_BasePt.x += 10.0f * ONgTempDebug_AttackExtent_SinTheta;
	ONgTempDebug_AttackExtent_BasePt.z += 10.0f * ONgTempDebug_AttackExtent_CosTheta;
	ONgTempDebug_AttackExtent_CurDelta[0] = 0.0f;
	ONgTempDebug_AttackExtent_CurDelta[1] = 0.0f;

	return UUcError_None;
}


static void ONiCharacter_TempDebug_DrawAttackExtent(ONtCharacter *inCharacter)
{
	UUtUns32 itr, itr2, current_time = ONrGameState_GetGameTime();
	UUtUns8 num_attacks, attack_indices[TRcMaxAttacks];
	TRtAttack attacks[TRcMaxAttacks];
	UUtBool is_attack;
	M3tPoint3D points[5], cur_location;
	TRtAttackBound *bound;
	IMtShade shade;
	float *cur_delta;
	UUtUns16 extent_index;
	float *cur_height, attack_dist;
	TRtDirection cur_direction;
	TRtPosition *cur_position;
	TRtAttackExtent *cur_extent;
	const TRtExtentInfo *extent_info;
	float radius, theta;

	if (ONgTempDebug_AttackExtent_Animation == NULL)
		return;

	if (TRrAnimation_TestFlag(ONgTempDebug_AttackExtent_Animation, ONcAnimFlag_RealWorld)) {
		// this animation doesn't have attack extent info
		ONgTempDebug_AttackExtent_Animation = NULL;
		return;
	}

	while (ONgTempDebug_AttackExtent_CurTime < current_time) {
		cur_delta = ONgTempDebug_AttackExtent_Animation->positions + 2 * ONgTempDebug_AttackExtent_Frame;

		ONgTempDebug_AttackExtent_CurDelta[0] += cur_delta[0];
		ONgTempDebug_AttackExtent_CurDelta[1] += cur_delta[1];

		ONgTempDebug_AttackExtent_Frame++;
		ONgTempDebug_AttackExtent_CurTime++;

		if (ONgTempDebug_AttackExtent_Frame >= TRrAnimation_GetDuration(ONgTempDebug_AttackExtent_Animation)) {
			ONgTempDebug_AttackExtent_Animation = NULL;
			return;
		}
	}

	cur_location = ONgTempDebug_AttackExtent_BasePt;
	cur_location.x += ONgTempDebug_AttackExtent_CosTheta * ONgTempDebug_AttackExtent_CurDelta[0]
					+ ONgTempDebug_AttackExtent_SinTheta * ONgTempDebug_AttackExtent_CurDelta[1];
	cur_location.z += - ONgTempDebug_AttackExtent_SinTheta * ONgTempDebug_AttackExtent_CurDelta[0]
					+ ONgTempDebug_AttackExtent_CosTheta * ONgTempDebug_AttackExtent_CurDelta[1];


	cur_position = ONgTempDebug_AttackExtent_Animation->positionPoints + ONgTempDebug_AttackExtent_Frame;

	// draw a little X on the floor under the character (for depth cueing)
	points[0] = cur_location;
	points[1] = cur_location;
	points[0].x -= 2.0f * ONgTempDebug_AttackExtent_CosTheta;
	points[0].z += 2.0f * ONgTempDebug_AttackExtent_SinTheta;
	points[1].x += 2.0f * ONgTempDebug_AttackExtent_CosTheta;
	points[1].z -= 2.0f * ONgTempDebug_AttackExtent_SinTheta;
	M3rGeom_Line_Light(&points[0], &points[1], IMcShade_White);

	points[0] = cur_location;
	points[1] = cur_location;
	points[0].x -= 2.0f * ONgTempDebug_AttackExtent_SinTheta;
	points[0].z -= 2.0f * ONgTempDebug_AttackExtent_CosTheta;
	points[1].x += 2.0f * ONgTempDebug_AttackExtent_SinTheta;
	points[1].z += 2.0f * ONgTempDebug_AttackExtent_CosTheta;
	M3rGeom_Line_Light(&points[0], &points[1], IMcShade_White);

	// draw the character's height / position
	points[0] = cur_location;
	points[1] = cur_location;

	points[0].y += cur_position->floor_to_toe_offset * TRcPositionGranularity;
	points[1].y += (cur_position->floor_to_toe_offset + cur_position->toe_to_head_height) * TRcPositionGranularity;
	M3rGeom_Line_Light(&points[0], &points[1], IMcShade_Blue);

	is_attack = TRrAnimation_IsAttack(ONgTempDebug_AttackExtent_Animation);
	if (!is_attack)
		return;

	// get info about the attack
	extent_info = TRrAnimation_GetExtentInfo(ONgTempDebug_AttackExtent_Animation);
	cur_direction = TRrAnimation_GetDirection(ONgTempDebug_AttackExtent_Animation, UUcTrue);

	if (!TRrAnimation_TestFlag(ONgTempDebug_AttackExtent_Animation, ONcAnimFlag_ThrowSource)) {
		UUmAssert(cur_direction != TRcDirection_None);
	}

	if (ONgTempDebug_AttackExtent_ShowGlobalExtents) {
		// draw the animation's initial and max extents
		for (itr = 0; itr < 2; itr++) {
			if (itr == 0) {
				bound = (TRtAttackBound *) &extent_info->firstAttack;
				shade = IMcShade_Yellow;
			} else {
				bound = (TRtAttackBound *) &extent_info->maxAttack;
				shade = IMcShade_Pink;
			}

			points[0] = ONgTempDebug_AttackExtent_BasePt;
			points[0].x += ONgTempDebug_AttackExtent_CosTheta * bound->position.x
							+ ONgTempDebug_AttackExtent_SinTheta * bound->position.y;
			points[0].z += - ONgTempDebug_AttackExtent_SinTheta * bound->position.x
							+ ONgTempDebug_AttackExtent_CosTheta * bound->position.y;
			points[0].y += bound->attack_height;

			points[1] = points[0];
			switch(cur_direction) {
				case TRcDirection_None:
					COrConsole_Printf("### warning: animation is of type 'none'");
				case TRcDirection_Forwards:
					points[1].x += ONgTempDebug_AttackExtent_SinTheta * bound->attack_dist;
					points[1].z += ONgTempDebug_AttackExtent_CosTheta * bound->attack_dist;
				break;

				case TRcDirection_Left:
					points[1].x += ONgTempDebug_AttackExtent_CosTheta * bound->attack_dist;
					points[1].z -= ONgTempDebug_AttackExtent_SinTheta * bound->attack_dist;
				break;

				case TRcDirection_Right:
					points[1].x -= ONgTempDebug_AttackExtent_CosTheta * bound->attack_dist;
					points[1].z += ONgTempDebug_AttackExtent_SinTheta * bound->attack_dist;
				break;

				case TRcDirection_Back:
					points[1].x -= ONgTempDebug_AttackExtent_SinTheta * bound->attack_dist;
					points[1].z -= ONgTempDebug_AttackExtent_CosTheta * bound->attack_dist;
				break;

				default:
					UUmAssert(0);
				break;
			}

			M3rGeom_Line_Light(&points[0], &points[1], shade);
		}

		// draw the animation's attack bounding volume
		MUmVector_Set(points[0], 0, 0, 0);
		for (itr2 = 0; itr2 < TRcExtentRingSamples; itr2++) {
			theta = ONgTempDebug_AttackExtent_Facing + itr2 * M3c2Pi / TRcExtentRingSamples;
			UUmTrig_ClipHigh(theta);
			radius = extent_info->attack_ring.distance[itr2];
			points[0] = ONgTempDebug_AttackExtent_BasePt;
			points[0].x += radius * MUrSin(theta);
			points[0].z += radius * MUrCos(theta);
			points[0].y += extent_info->attack_ring.min_height;

			theta = ONgTempDebug_AttackExtent_Facing +
				((itr2 + 1) % TRcExtentRingSamples) * M3c2Pi / TRcExtentRingSamples;
			UUmTrig_ClipHigh(theta);
			radius = extent_info->attack_ring.distance[((itr2 + 1) % TRcExtentRingSamples)];
			points[1] = ONgTempDebug_AttackExtent_BasePt;
			points[1].x += radius * MUrSin(theta);
			points[1].z += radius * MUrCos(theta);
			points[1].y += extent_info->attack_ring.min_height;

			M3rGeom_Line_Light(&points[0], &points[1], IMcShade_Red);

			points[0].y += extent_info->attack_ring.max_height - extent_info->attack_ring.min_height;
			points[1].y += extent_info->attack_ring.max_height - extent_info->attack_ring.min_height;

			M3rGeom_Line_Light(&points[0], &points[1], IMcShade_Red);
		}
	}

	// get the animation's attack extent for this frame
	TRrAnimation_GetAttacks(ONgTempDebug_AttackExtent_Animation, (UUtUns16) ONgTempDebug_AttackExtent_Frame, &num_attacks,
							attacks, attack_indices);
	if (num_attacks == 0) {
		// no attacks to draw this frame.
		return;
	}

	cur_height = ONgTempDebug_AttackExtent_Animation->heights + ONgTempDebug_AttackExtent_Frame;

	for (itr = 0; itr < num_attacks; itr++) {
		extent_index = attacks[itr].attackExtentIndex;
		if (extent_index == (UUtUns16) -1) {
			// this attack has no entries in the extent array!
			COrConsole_Printf("### warning: attack %d has no extent entries", attack_indices[itr]);
			continue;
		}

		extent_index += (UUtUns16) (ONgTempDebug_AttackExtent_Frame - attacks[itr].firstDamageFrame);

		UUmAssert((extent_index >= 0) && (extent_index < extent_info->numAttackExtents));
		cur_extent = extent_info->attackExtents + extent_index;

		points[0] = cur_location;
		points[0].y += cur_extent->attack_height * TRcPositionGranularity;

		attack_dist = cur_extent->attack_distance * TRcPositionGranularity;

		if (cur_direction == TRcDirection_360) {
			points[1] = points[0];
			points[2] = points[0];
			points[3] = points[0];
			points[4] = points[0];

			points[1].x += ONgTempDebug_AttackExtent_SinTheta * attack_dist;
			points[1].z += ONgTempDebug_AttackExtent_CosTheta * attack_dist;
			points[2].x += ONgTempDebug_AttackExtent_CosTheta * attack_dist;
			points[2].z -= ONgTempDebug_AttackExtent_SinTheta * attack_dist;
			points[3].x -= ONgTempDebug_AttackExtent_CosTheta * attack_dist;
			points[3].z += ONgTempDebug_AttackExtent_SinTheta * attack_dist;
			points[4].x -= ONgTempDebug_AttackExtent_SinTheta * attack_dist;
			points[4].z -= ONgTempDebug_AttackExtent_CosTheta * attack_dist;
			M3rGeom_Line_Light(&points[1], &points[2], IMcShade_Orange);
			M3rGeom_Line_Light(&points[2], &points[3], IMcShade_Orange);
			M3rGeom_Line_Light(&points[3], &points[4], IMcShade_Orange);
			M3rGeom_Line_Light(&points[4], &points[1], IMcShade_Orange);

		} else {
			points[1] = points[0];

			switch(cur_direction) {
				case TRcDirection_None:
					COrConsole_Printf("### warning: animation is of type 'none'");
				case TRcDirection_Forwards:
					points[1].x += ONgTempDebug_AttackExtent_SinTheta * attack_dist;
					points[1].z += ONgTempDebug_AttackExtent_CosTheta * attack_dist;
				break;

				case TRcDirection_Left:
					points[1].x += ONgTempDebug_AttackExtent_CosTheta * attack_dist;
					points[1].z -= ONgTempDebug_AttackExtent_SinTheta * attack_dist;
				break;

				case TRcDirection_Right:
					points[1].x -= ONgTempDebug_AttackExtent_CosTheta * attack_dist;
					points[1].z += ONgTempDebug_AttackExtent_SinTheta * attack_dist;
				break;

				case TRcDirection_Back:
					points[1].x -= ONgTempDebug_AttackExtent_SinTheta * attack_dist;
					points[1].z -= ONgTempDebug_AttackExtent_CosTheta * attack_dist;
				break;

				default:
					UUmAssert(0);
				break;
			}
		}
		M3rGeom_Line_Light(&points[0], &points[1], (cur_direction == TRcDirection_None) ? IMcShade_Green : IMcShade_Red);
	}
}
#endif

static void ONiCharacter_Display_Body(
					ONtCharacter *inCharacter,
					ONtActiveCharacter *inActiveCharacter,
					const M3tPoint3D *inCameraLocation)
{
	const UUtUns32 deathFadeStart = ONgDeathFadeStart;
	const UUtUns32 deathFadeFrames = ONgDeathFadeFrames;
	TRtBody *body;
	float distanceToCameraSquared;
	TRtBodySelector resolution;
	float alphaAmount = 1.f;

	UUmAssertReadPtr(inCharacter, sizeof(inCharacter));
	UUmAssert(inActiveCharacter->curBody >= TRcBody_SuperLow);
	UUmAssert(inActiveCharacter->curBody <= TRcBody_SuperHigh);

	if ((inActiveCharacter->invis_active) || (inCharacter->flags & ONcCharacterFlag_Teleporting)) {
		alphaAmount = inActiveCharacter->invis_character_alpha;
	}

	UUmAssert(!(inCharacter->flags & ONcCharacterFlag_Dead_4_Gone));
	UUmAssert(inCharacter->flags & ONcCharacterFlag_InUse);

	distanceToCameraSquared = MUrPoint_Distance_Squared(inCameraLocation, &(inCharacter->location));

	if (alphaAmount < 1.f) {
		resolution = UUmPin(inActiveCharacter->curBody - 1, TRcBody_SuperLow, TRcBody_SuperHigh);
	}
	else {
		resolution = UUmPin(inActiveCharacter->curBody, TRcBody_SuperLow, TRcBody_SuperHigh);
	}

	if (ONrMotoko_GraphicsQuality_NeverUseSuperLow()) {
		resolution = UUmMax(resolution, TRcBody_Low);
	}

	body = ONrCharacter_GetBody(inCharacter, resolution);

	TRrBody_SetMaps(body, inCharacter->characterClass->textures);

	M3rGeom_State_Push();

/*	// CB: I needed a character flag and removed ONcCharacterFlag_Holograph
	if (inCharacter->flags & ONcCharacterFlag_Holograph) {
		UUtUns16 random;

		// 1 in 20 change of flickering
		random = UUmLocalRandomRange(0, 20);

		if (0 == random) {
			M3rGeom_State_Pop();
			return;
		}

		if ((ONgGameState->gameTime % 200) < 17) {
			float holograph_translate_down = ((ONgGameState->gameTime % 200) < 17) * -0.1f;

			M3rMatrixStack_Translate(0.f, holograph_translate_down, 0.f);
		}
		else if ((ONgGameState->gameTime % 200) < 26) {
			float holograph_translate_up = 0.4f;

			M3rMatrixStack_Translate(0.f, holograph_translate_up, 0.f);
		}

		// setup lighting for the holograph
		ONiHolograph_Lighting();
	}*/

	// commit the alpha to draw the body with
	if ((alphaAmount < 1.f) && (alphaAmount > 0.f)) {
		UUtUns32 alphaInt = MUrUnsignedSmallFloat_To_Uns_Round(255.f * alphaAmount);

		M3rDraw_State_SetInt(M3cDrawStateIntType_FrameBufferBlendWithConstantAlpha, UUcTrue);
		M3rDraw_State_SetInt(M3cDrawStateIntType_ZWrite, M3cDrawState_ZWrite_Off); // S.S.

		M3rGeom_State_Set(M3cGeomStateIntType_SubmitMode, M3cGeomState_SubmitMode_SortAlphaTris);
		M3rGeom_State_Set(M3cGeomStateIntType_Alpha, alphaInt);
		M3rGeom_State_Commit();
	}

	// draw the body
	if (gShowTriggerQuad) {
		UUtUns32 trigger_quad_shade = IMcShade_White;

		M3rGeom_Line_Light(inActiveCharacter->trigger_points + 0, inActiveCharacter->trigger_points + 1, trigger_quad_shade);
		M3rGeom_Line_Light(inActiveCharacter->trigger_points + 1, inActiveCharacter->trigger_points + 2, trigger_quad_shade);
	}
	else if (alphaAmount > 0.f) {
#if defined(DEBUGGING) && DEBUGGING
		if ((inActiveCharacter->extraBody != NULL) && (inActiveCharacter->extraBody->numParts > 0) &&
			(inActiveCharacter->extraBody->parts[0].geometry != NULL)) {
			// CB: this should only be true if we have a weapon
			UUmAssert(inCharacter->inventory.weapons[0] != NULL);
		}
#endif
		TRrBody_Draw(body, gDrawWeapon ? inActiveCharacter->extraBody : NULL, inActiveCharacter->matricies, TRcBody_DrawAll,
			inActiveCharacter->flash_parts, 0, 0, 0);
	}

	if (inActiveCharacter->shield_active)
	{
		M3tTextureMap *shield_texture = NULL;
		TRtBody *magic_shield_body = ONrCharacter_GetBody(inCharacter, UUmMin(inActiveCharacter->curBody, TRcBody_Low));
		const char *shield_name;

		if (inActiveCharacter->daodan_shield_effect) {
			shield_name = "DAODAN_SHIELD";
		}
		else if (inCharacter->flags & ONcCharacterFlag_BossShield) {
			shield_name = "DAODAN_SHIELD";
		}
		else {
			shield_name = "SHIELD";
		}

		TMrInstance_GetDataPtr(M3cTemplate_TextureMap, shield_name, (void **) &shield_texture);

		if (NULL != shield_texture) {
			M3rDraw_State_SetInt(M3cDrawStateIntType_Time, ONgGameState->gameTime);

			TRrBody_DrawMagic(magic_shield_body, inActiveCharacter->matricies, shield_texture, 1.4f, inActiveCharacter->shield_parts);
		}
	}

	if (inActiveCharacter->invis_active) {
		// CB: draw the invisibility magic aura
		M3tTextureMap *invis_texture = NULL;
		TRtBody *magic_invis_body = ONrCharacter_GetBody(inCharacter, UUmMin(inActiveCharacter->curBody, TRcBody_Low));

		TMrInstance_GetDataPtr(M3cTemplate_TextureMap, "INVIS", (void **) &invis_texture);

		if (NULL != invis_texture) {
			float alpha_adjustment = 0.22f;
			UUtUns32 alpha = MUrUnsignedSmallFloat_To_Uns_Round((1.f - inActiveCharacter->invis_character_alpha) * M3cMaxAlpha * alpha_adjustment);

			M3rGeom_State_Set(M3cGeomStateIntType_Alpha, alpha);
			M3rDraw_State_SetInt(M3cDrawStateIntType_FrameBufferBlendWithConstantAlpha, UUcFalse);
			M3rDraw_State_SetInt(M3cDrawStateIntType_Time, ONgGameState->gameTime);

			TRrBody_DrawMagic(magic_invis_body, inActiveCharacter->matricies, invis_texture, 1.4f, NULL);
		}
	}

	M3rGeom_State_Pop();

	// CB: I needed a character flag and removed ONcCharacterFlag_Holograph
/*	if (inCharacter->flags & ONcCharacterFlag_Holograph)
	{
		ONrGameState_SetupDefaultLighting();
	}*/

	if ((ONgGameState->local.playerCharacter == inCharacter) && gShowLOD) {
		sprintf(ONgChrLODStatus[1].text, "(%d) = %s", resolution, body->debug_name);
	}
}

static void ONrDrawLaserDot(M3tPoint3D *inPoint)
{
	UUtError error;
	M3tTextureMap *dotTexture;
	float scale = .003f;
	float cam_sphere_dist;
	M3tPoint3D cameraLocation;

	M3rCamera_GetViewData(ONgActiveCamera, &cameraLocation, NULL, NULL);

	error = TMrInstance_GetDataPtr(
		M3cTemplate_TextureMap,
		"dot",
		(void **) &dotTexture);
	UUmAssert(UUcError_None == error);

	if (error) {
		return;
	}

	cam_sphere_dist = MUmVector_GetDistance(
		cameraLocation,
		(*inPoint));

	if (cam_sphere_dist > .001f) {
		scale *= cam_sphere_dist;

		M3rSimpleSprite_Draw(dotTexture, inPoint, scale, scale, IMcShade_Red, M3cMaxAlpha);
	}

	return;
}

static void ONiDrawLocationX(const ONtCharacter *inCharacter)
{
	M3tPoint3D offset[4] =
	{
		{ 4.f, 0.f, 4.f },
		{ 4.f, 0.f, -4.f },
		{ -4.f, 0.f, 4.f },
		{ -4.f, 0.f, -4.f },
	};

	M3tPoint3D to;
	UUtUns8 itr;

	for(itr = 0; itr < 4; itr++) {
		MUmVector_Add(to, inCharacter->location, offset[itr]);
		M3rGeom_Line_Light(&inCharacter->location, &to, IMcShade_Yellow);
	}

	return;
}

void WPrDrawSprite(WPtSprite *inSprite, M3tPoint3D *inLocation, WPtScaleMode inScaleMode)
{
	float scale;
	M3tPoint3D camera_location;
	float camera_distance;
	WPtScaleMode scale_mode = inScaleMode;

	if (ONgWpForceScale) {
		scale_mode = WPcScaleMode_Scale;
	}

	if (ONgWpForceHalfScale) {
		scale_mode = WPcScaleMode_HalfScale;
	}

	if (ONgWpForceNoScale) {
		scale_mode = WPcScaleMode_NoScale;
	}

	if (NULL == inSprite->texture) {
		goto exit;
	}

	M3rCamera_GetViewData(ONgActiveCamera, &camera_location, NULL, NULL);
	camera_distance = MUmVector_GetDistance(camera_location, *inLocation);

	if (WPcScaleMode_Scale == scale_mode) {
		scale = 5.f;
	}
	else if (WPcScaleMode_HalfScale == scale_mode) {
		scale = 0.7f * (float) pow(camera_distance, ONgWpPowAdjustment);
	}
	else {
		UUmAssert(WPcScaleMode_NoScale == scale_mode);
		scale = 0.03f * camera_distance;
	}

	scale *= inSprite->scale * ONgWpScaleAdjustment;

	M3rGeom_State_Push();
		M3rDraw_State_SetInt(M3cDrawStateIntType_ZWrite, M3cDrawState_ZWrite_Off);
		M3rGeom_State_Set(M3cGeomStateIntType_SpriteMode, M3cGeomState_SpriteMode_Flat);
		M3rGeom_State_Commit();

		M3rSimpleSprite_Draw(inSprite->texture, inLocation, scale, scale, (inSprite->shade | 0xFF000000), M3cMaxAlpha);
	M3rGeom_State_Pop();

exit:
	return;
}

// ========================================================================================================
// weapon sights
// ========================================================================================================


static void ONiDrawLaserSight(WPtWeaponClass *inWeaponClass, M3tPoint3D *inFrom, M3tPoint3D *inTo )
{
	UUtUns32 color = (inWeaponClass->laser_color & 0xFFFFFF) | (M3cMaxAlpha << 24);

	FXrDrawLaser( inFrom, inTo, color );

	return;
}

static void ONiDrawTunnel(WPtWeaponClass *inWeaponClass, M3tPoint3D *inFrom, M3tPoint3D *inTo )
{
	UUtUns32			itr;
	UUtUns32			count = inWeaponClass->tunnel_count;
	float				distance = MUrPoint_Distance(inFrom, inTo);
	float				current_distance;
	M3tVector3D			vector;
	M3tPoint3D			current_location;

	MUmVector_Subtract(vector, *inTo, *inFrom);
	MUrVector_SetLength(&vector, inWeaponClass->tunnel_spacing);

	{
		float intial_offset = inWeaponClass->tunnel_spacing * 0.25f;
		M3tVector3D initial_offset_vector = vector;

		MUrVector_SetLength(&initial_offset_vector, intial_offset);

		current_distance = intial_offset;
		MUmVector_Add(current_location, *inFrom, initial_offset_vector);
	}

	for(itr = 0; itr < count; itr++)
	{
		if ((current_distance + inWeaponClass->tunnel_spacing) > distance) {
			break;
		}

		WPrDrawSprite(&inWeaponClass->tunnel, &current_location, WPcScaleMode_Scale);

		current_distance += inWeaponClass->tunnel_spacing;
		MUmVector_Increment(current_location, vector);

	}

	return;
}

static M3tPoint3D ONgLaserStart;
static M3tPoint3D ONgLaserStop;
static AMtRayResultType ONgLaserType;
static UUtBool ONgDrawLaserSight;
extern UUtBool AMgCharacterLaserSight;

static void ONiDrawWeaponSight(ONtCharacter *inCharacter)
{
	UUtBool				draw_sight;
	UUtUns16			playerNum;
	WPtWeaponClass		*weapon_class;
	M3tPoint3D			from, to;
	M3tVector3D			sight_vector;
	AMtRayResults		ray_results;
	WPtMarker			*laser_marker;
	ONtActiveCharacter	*active_character;

	UUmAssertReadPtr(inCharacter, sizeof(*inCharacter));
	active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character == NULL) {
		goto exit;
	}

	if (inCharacter->inventory.weapons[0] == NULL) {
		goto exit;
	}

	playerNum = ONrGameState_GetPlayerNum();

	draw_sight = (inCharacter->charType == ONcChar_Player);
	draw_sight = draw_sight && (inCharacter->inventory.weapons[0] != NULL);
	draw_sight = draw_sight && (AI2gDrawLaserSights || (inCharacter->charType != ONcChar_AI2));
	draw_sight = draw_sight && !ONgMarketing_Line_Off;
	draw_sight = draw_sight && !ONrCharacter_IsGunThroughWall(inCharacter, active_character);
	draw_sight = draw_sight && WPrHasAmmo(inCharacter->inventory.weapons[0]);

	if (draw_sight)
	{
		ONtCharacterIndexType index = ONrCharacter_GetIndex(inCharacter);

		ONgDrawLaserSight = UUcTrue;

		weapon_class	= WPrGetClass(inCharacter->inventory.weapons[0]);
		laser_marker	= &weapon_class->sight;

		WPrGetMarker(inCharacter->inventory.weapons[0], laser_marker, &from, &sight_vector);

		if (MUmVector_IsZero(laser_marker->vector))
			return;

		MUmAssertVector(sight_vector, weapon_class->maximum_sight_distance);


		AMgCharacterLaserSight = UUcTrue;
		AMrRayToEverything(index, &from, &sight_vector, &ray_results);

		if (ray_results.resultType != AMcRayResult_None)
		{
			float distance;

			if (gConsole_show_laser_env_collision)
			{
				if (AMcRayResult_Environment == ray_results.resultType)
				{
					AKtGQ_General *flash_gq;
					flash_gq = ONgGameState->level->environment->gqGeneralArray->gqGeneral;
					flash_gq += ray_results.resultData.rayToEnvironment.hitGQIndex;
					flash_gq->flags |= AKcGQ_Flag_Draw_Flash;
				}
			}

			to			= ray_results.intersection;
			distance	= MUmVector_GetDistance(to, from);

			if (distance > weapon_class->maximum_sight_distance) {
				// we collide but it was past the max distance
				MUmVector_Subtract(to, to, from);
				MUrVector_SetLength(&to, weapon_class->maximum_sight_distance);
				MUmVector_Add(to, to, from);

				ray_results.resultType = AMcRayResult_None;

			}
			else if (distance > UUcEpsilon)
			{
				float uAmt		= 1.f;
				float offsetAmt = 2.f;

				uAmt = offsetAmt / distance;
				uAmt = UUmPin(uAmt, 0.f, 1.f);
				uAmt = 1.f - uAmt;

				MUrLineSegment_ComputePoint(&from, &to, uAmt, &to);
			}


		}
		else
		{
			MUmVector_Scale(sight_vector, weapon_class->maximum_sight_distance);
			MUmVector_Add(to, from, sight_vector);
		}

		ONgLaserStart = from;
		ONgLaserStop = to;
		if ((ray_results.resultType == AMcRayResult_Character) &&
			(ray_results.resultData.rayToCharacter.hitCharacter->flags & ONcCharacterFlag_Dead)) {
			// don't target on dead characters
			ONgLaserType = AMcRayResult_None;
		} else {
			ONgLaserType = ray_results.resultType;
		}

		if (weapon_class->flags & WPcWeaponClassFlag_HasLaser) {
			ONiDrawLaserSight(weapon_class, &from, &to);
		}

		if (weapon_class->tunnel.texture != NULL) {
			ONiDrawTunnel(weapon_class, &from, &to);
		}
	}

exit:
	return;
}

static void ONiCharacter_InitializeShadow(ONtActiveCharacter *ioActiveCharacter)
{
	ioActiveCharacter->shadow_index = (UUtUns16) -1;
	ioActiveCharacter->shadow_active = UUcFalse;

	ioActiveCharacter->shadow_decal.decal_header = NULL;
	ioActiveCharacter->shadow_decal.block = 0xffff;
	ioActiveCharacter->shadow_decal.traverse_id = 0;
	ioActiveCharacter->shadow_decal.next = NULL;
	ioActiveCharacter->shadow_decal.prev = NULL;
}

static void ONiCharacter_AllocateShadow(ONtActiveCharacter *ioActiveCharacter)
{
	ONtCharacterShadowStorage *storage;

	if (ioActiveCharacter->shadow_index != (UUtUns16) -1) {
		// we already have a shadow buffer allocated
		UUmAssertReadPtr(ioActiveCharacter->shadow_decal.decal_header, ONcShadowDecalStaticBufferSize);
		return;
	}

	// allocate new shadow storage from the gamestate
	ioActiveCharacter->shadow_index = ONrGameState_NewShadow();
	ioActiveCharacter->shadow_active = UUcFalse;

	storage = &ONgGameState->shadowStorage[ioActiveCharacter->shadow_index];
	storage->character_index = ioActiveCharacter->index;
	storage->active_index = ioActiveCharacter->active_index;

	// set up the pointer into our decal buffer
	UUmAssert(ioActiveCharacter->shadow_decal.decal_header == NULL);
	ioActiveCharacter->shadow_decal.decal_header = (M3tDecalHeader *) storage->decalbuffer;
}

static void ONiCharacter_DeallocateShadow(ONtActiveCharacter *ioActiveCharacter)
{
	if (ioActiveCharacter->shadow_index == (UUtUns16) -1) {
		// we have no shadow
		UUmAssert(!ioActiveCharacter->shadow_active);
		UUmAssert(ioActiveCharacter->shadow_decal.decal_header == NULL);
		return;
	}

	// deallocate our shadow from the gamestate
	ONrGameState_DeleteShadow(ioActiveCharacter->shadow_index);
	ioActiveCharacter->shadow_index = (UUtUns16) -1;
	ioActiveCharacter->shadow_decal.decal_header = NULL;
}

static void ONiCharacter_DeleteShadow(ONtActiveCharacter *ioActiveCharacter)
{
	if (!ioActiveCharacter->shadow_active) {
		// we don't have a shadow
		return;
	}

	P3rDeleteDecal(&ioActiveCharacter->shadow_decal, P3cDecalFlag_Manual);

	// set the shadow decal data back to an 'unused' state
	ioActiveCharacter->shadow_active = UUcFalse;
	ioActiveCharacter->shadow_decal.prev = NULL;
	ioActiveCharacter->shadow_decal.next = NULL;
	return;
}

static void ONiCharacter_Display(
					ONtCharacter *inCharacter,
					ONtActiveCharacter *inActiveCharacter,
					const M3tPoint3D *inCameraLocation)
{
	UUtBool				drawBodyThisFrame, made_shadow;
	UUtBool				isMainCharacter;
	ONtCharacterIndexType index;
	UUtUns16			playerNum;
	float				character_alpha;
	UUtBool				support_shadow;
	UUtError			error;

	UUmAssertReadPtr(inCharacter, sizeof(*inCharacter));
	UUmAssertReadPtr(inCameraLocation, sizeof(*inCameraLocation));
	UUmAssert(inCharacter->flags & ONcCharacterFlag_InUse);
	UUmAssert(inCharacter->flags & ONcCharacterFlag_Draw);
	UUmAssert(!(inCharacter->flags & ONcCharacterFlag_Dead_4_Gone));
	UUmAssert(inCharacter->flags & ONcCharacterFlag_InUse);

	ONgNumCharactersDrawn++;

	playerNum = ONrGameState_GetPlayerNum();
	index = ONrCharacter_GetIndex(inCharacter);

	isMainCharacter = (playerNum == index);
	drawBodyThisFrame = UUcTrue;


	if (gDebugCharactersSphereTree && (inActiveCharacter != NULL)) {
		PHrSphereTree_Draw(inActiveCharacter->sphereTree);
	}

	if (drawBodyThisFrame) {
		ONiCharacter_Display_Body(inCharacter, inActiveCharacter, inCameraLocation);
	}

	if (gShowBNV && isMainCharacter) {
		sprintf(ONgBNVStatus[1].text, "%d", inCharacter->currentNode - ONgGameState->level->environment->bnvNodeArray->nodes);
	}

	if (gDrawFacingVector) {
		ONiDrawFacingVector(inCharacter);
	}

	if (gDrawAimingVector) {
		ONiDrawAimingVector(inCharacter);
	}

	if (isMainCharacter) {
		ONrCharacter_Display_Local(inCharacter);
	}

	made_shadow = UUcFalse;

	character_alpha = ((inActiveCharacter->invis_active) || (inCharacter->flags & ONcCharacterFlag_Teleporting)) ? inActiveCharacter->invis_character_alpha : 1.0f;

	support_shadow = UUcTrue;
	support_shadow = support_shadow && ONrMotoko_GraphicsQuality_SupportShadows();
	support_shadow = support_shadow && ONgCharacterShadow;
	support_shadow = support_shadow && ((inCharacter->flags & ONcCharacterFlag_Dead_3_Cosmetic) == 0);
	support_shadow = support_shadow && (character_alpha > 0);
	support_shadow = support_shadow && (!(inCharacter->flags & ONcCharacterFlag_NoShadow));


	if (support_shadow) {
		M3tTextureMap *texture;
		ONtShadowConstants *shadow_constants;
#if defined(DEBUGGING) && DEBUGGING
		P3tDecalData debug_orig_decaldata = inActiveCharacter->shadow_decal;
		debug_orig_decaldata;
#endif

		ONiCharacter_AllocateShadow(inActiveCharacter);
		if (inActiveCharacter->shadow_active) {
			// delete existing shadow
			ONiCharacter_DeleteShadow(inActiveCharacter);
		}

		shadow_constants = &inCharacter->characterClass->shadowConstants;
		texture = shadow_constants->shadow_texture;
		if (texture == NULL) {
			TMrInstance_GetDataPtr(M3cTemplate_TextureMap, "notfoundtex", (void **) &texture);
		}

		if ((inActiveCharacter->shadow_decal.decal_header != NULL) && (texture != NULL)) {
			M3tVector3D	fwd_vec, up_vec, right_vec, position;
			float size;
			UUtUns32 gq_index;
			UUtUns16 alpha;

			// form coordinate system for the shadow
			MUmVector_Set(fwd_vec, 0, 1, 0);
			MUmVector_Set(up_vec, 0, 0, 1);
			MUmVector_Set(right_vec, 1, 0, 0);

			gq_index = (UUtUns32) -1;
			position = MUrMatrix_GetTranslation(inActiveCharacter->matricies + ONcPelvis_Index);
			if (inActiveCharacter->inAirControl.numFramesInAir == 0) {
				position.y = inActiveCharacter->groundPoint.y;
				gq_index = inActiveCharacter->last_floor_gq;
			} else {
				position.y = inCharacter->feetLocation;
			}

			if (gq_index == (UUtUns32) -1) {
				AKtEnvironment *environment = ONrGameState_GetEnvironment();
				M3tVector3D delta_pos;
				M3tBoundingSphere sphere;
				float height, interp;
				UUtBool found_floor;

				// we must cast a sphere down from our current feet position to find the actual position
				// of the floor
				sphere.radius = shadow_constants->size_max;
				sphere.center = position;
				sphere.center.y -= sphere.radius;
				MUmVector_Set(delta_pos, 0, -(shadow_constants->height_max + sphere.radius), 0);
				found_floor = AKrCollision_Sphere(environment, &sphere, &delta_pos, AKcGQ_Flag_Obj_Col_Skip | AKcGQ_Flag_Invisible, 1);
				if (found_floor) {
					UUmAssert(AKgNumCollisions > 0);
					height = UUmMax(0, position.y - AKgCollisionList[0].collisionPoint.y);
					position = AKgCollisionList[0].collisionPoint;

					if (height < shadow_constants->height_fade) {
						// we are not yet high enough to start the severe fade-out
						gq_index = AKgCollisionList[0].gqIndex;
						interp = height / shadow_constants->height_fade;
						size = shadow_constants->size_max + (shadow_constants->size_fade - shadow_constants->size_max) * interp;
						alpha = (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(shadow_constants->alpha_max +
												(shadow_constants->alpha_fade - shadow_constants->alpha_max) * interp);

					} else if (height < shadow_constants->height_max) {
						// start fading out fast
						gq_index = AKgCollisionList[0].gqIndex;
						interp = (shadow_constants->height_max - height) / (shadow_constants->height_max - shadow_constants->height_fade);
						size = shadow_constants->size_min + (shadow_constants->size_fade - shadow_constants->size_min) * interp;
						alpha = (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(shadow_constants->alpha_fade * interp);

					} else {
						// don't set gq_index, which causes us not to draw a decal
					}
				}
			} else {
				// we are standing right on a quad
				alpha = shadow_constants->alpha_max;
				size = shadow_constants->size_max;
			}

			if (character_alpha < 1.0f) {
				// fade out shadow as we turn invisible
				alpha = (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(alpha * character_alpha);
			}

			if (gq_index != (UUtUns32) -1) {
				error = P3rCreateDecal(texture, gq_index, P3cDecalFlag_Manual | P3cDecalFlag_CullBackFacing |
															P3cDecalFlag_IgnoreAdjacency | P3cDecalFlag_FullBright,
									80.0f * M3cDegToRad, NULL, &position, &fwd_vec, &up_vec, &right_vec, size, size,
									alpha, IMcShade_White, &inActiveCharacter->shadow_decal, ONcShadowDecalStaticBufferSize);

				if ((error == UUcError_None) && (inActiveCharacter->shadow_decal.decal_header != NULL)) {
					// our shadow is now in use
					inActiveCharacter->shadow_active = UUcTrue;
					inActiveCharacter->shadow_decal.decal_header->gq_index = (UUtUns32) -1;
				} else {
					// the shadow buffer that we requested isn't being used after all
					ONiCharacter_DeleteShadow(inActiveCharacter);
				}
			}
		}
	}
}



static void ONrCharacter_Display_Local(const ONtCharacter *inCharacter)
{
	if (ONgDrawDot) {
		M3rGeom_Line_Light(&inCharacter->location, &ONgDot, IMcShade_Blue);
	}

	return;
}

static UUtBool ONiCharacter_NeedsRendering(ONtCharacter *inCharacter, CAtCamera *inCamera)
{
	UUtBool is_visible = UUcFalse;
	// Returns true if 'inCharacter' needs drawing relative to 'inCamera'
	float farP,fov;
	M3tVector3D toObj;
	M3tPoint3D cameraLocation;
	M3tPoint3D viewVector;

	#define fovFudge (0.25f * M3cHalfPi)

	M3rCamera_GetStaticData(ONgVisibilityCamera,&fov,NULL,NULL,&farP);
	M3rCamera_GetViewData(ONgVisibilityCamera, &cameraLocation, &viewVector, NULL);

	MUmVector_Subtract(toObj,inCharacter->location,cameraLocation);
	MUrNormalize(&toObj);

	if (gDrawAllCharacters) {
		is_visible = UUcTrue;
	}
	else if (MUrAngleBetweenVectors3D(&toObj,&viewVector) > (fov + fovFudge)) {
		is_visible = UUcFalse;
	}
	else if (inCharacter->distance_from_camera_squared > UUmSQR(farP)) {
		is_visible = UUcFalse;
	}
	else if (!ONgShow_Environment) {
		is_visible = UUcTrue;
	}
	else {
		is_visible = AKrEnvironment_IsBoundingBoxMinMaxVisible(&inCharacter->boundingBox);
	}

	if (is_visible) {
		inCharacter->num_frames_offscreen = 0;
	}
	else {
		inCharacter->num_frames_offscreen++;
	}

	// if our active character exists, we have only been offscreen for
	// a short while and our body is not !drawn then consider us visible

	if ((!is_visible) && (inCharacter->num_frames_offscreen < 4)) {
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

		if (NULL != active_character) {
			if (active_character->curBody != TRcBody_NotDrawn) {
				is_visible = UUcTrue;
			}
		}
	}

	return is_visible;
}

static UUtUns32 gColorTable[10] =
{
	IMcShade_White,
	IMcShade_Red,
	IMcShade_Green,
	IMcShade_Blue,
	IMcShade_Red | IMcShade_Green,
	IMcShade_Red | IMcShade_Blue,
	IMcShade_Green | IMcShade_Blue
};

static UUtInt32 ONrCharacter_GetPolygonCount(ONtCharacter *inCharacter, ONtActiveCharacter *inActiveCharacter, UUtInt32 inOffset)
{
	TRtBodySelector selector = UUmPin(inActiveCharacter->baseBody + inOffset, TRcBody_SuperLow, TRcBody_SuperHigh);
	TRtBody *body = ONrCharacter_GetBody(inCharacter, selector);
	UUtUns16 itr;
	UUtInt32 polygon_count = 0;

	for(itr = 0; itr < body->numParts; itr++) {
		if (body->geometries[itr] != NULL) {
			polygon_count += body->geometries[itr]->triNormalArray->numVectors;
		}
	}

	return polygon_count;
}

static UUtInt32 ONrCharacterList_GetPolygonCount(ONtCharacter **inCharacters, ONtActiveCharacter **inActiveCharacters,
												 UUtInt32 inNumCharacters, UUtInt32 inOffset)
{
	UUtInt32 loop;
	UUtInt32 polygon_count = 0;

	for (loop = 0; loop < inNumCharacters; loop++) {
		polygon_count += ONrCharacter_GetPolygonCount(inCharacters[loop], inActiveCharacters[loop], inOffset);
	}

	return polygon_count;
}

//static UUtInt32 ONrCharacters_ComputeSceneUsage()
// 0. compute visable characters
// 1. sort characters by distance
// 2. compute ideal LOD by distance & bias
// 3. count polygon
// 4. bias = 0
// 5. if polygons < polygons_for_characters goto 15

// 6. compute ideal LOD by distance & bias + 1
// 7. count polygons
// 8. if polygons > polygons_for_characters goto
// 9. bias += 1
// 10. goto 6

// 11. bias -= 1
// 12. count polygons
// 13. compute ideal LOD by distance & bias + 1
// 14. count polygons
// 13. if polygons < polygons_for_characters goto
// 14. if bias = -4 goto done:
// 15. goto 11

// 16. loop from closest to most distant character raising bias by 1, halting & resetting if we exceed count

static UUtBool distance_from_camera_compare(UUtUns32 inA, UUtUns32 inB)
{
	UUtBool result;
	ONtCharacter *characterA = (ONtCharacter *) inA;
	ONtCharacter *characterB = (ONtCharacter *) inB;
	ONtCharacter *player_character = ONgGameState->local.playerCharacter;

	if (player_character == characterA) {
		result = UUcFalse;
	}
	else if (player_character == characterB) {
		result = UUcTrue;
	}
	else {
		result = characterA->distance_from_camera_squared > characterB->distance_from_camera_squared;
	}

	return result;
}

static void ONrGameState_ComputeCharacterVisibility(UUtUns32 *outNumVisible, ONtCharacter **outVisibleChars,
													ONtActiveCharacter **outVisibleActiveChars)
{
	ONtCharacter *current_character, *player_character;
	ONtActiveCharacter *active_character;
	ONtCharacter **potential_character_list;
	UUtUns32 potential_character_count, num_visible;
	UUtUns16 itr, player_team;
	UUtBool close_enough_to_taunt = UUcFalse;
	M3tPoint3D camera_location;

	M3rCamera_GetViewData(ONgVisibilityCamera, &camera_location, NULL, NULL);

	player_character = ONrGameState_GetPlayerCharacter();
	player_team = player_character->teamNumber;

	potential_character_count = ONrGameState_PresentCharacterList_Count();
	potential_character_list = ONrGameState_PresentCharacterList_Get();

	num_visible = 0;
	for(itr = 0; itr < potential_character_count; itr++) {
		current_character = potential_character_list[itr];

		current_character->flags &= ~ONcCharacterFlag_Draw;

		current_character->distance_from_camera_squared = MUmVector_GetDistanceSquared(current_character->location, camera_location);

		if (!ONiCharacter_NeedsRendering(current_character, ONgGameState->local.camera)) {
			active_character = ONrGetActiveCharacter(current_character);
			if (active_character != NULL) {
				// this character is not drawn, flag it as such
				active_character->curBody = TRcBody_NotDrawn;
				active_character->desiredBody = TRcBody_NotDrawn;
				active_character->desiredBodyFrames = 0;
			}
			continue;
		}

		if ((current_character != player_character) &&
			(AI2rTeam_FriendOrFoe(player_team, current_character->teamNumber) == AI2cTeam_Hostile) &&
			(current_character->distance_from_camera_squared < UUmSQR(ONcCharacter_TauntEnable_CloseDist))) {
			ONrGameState_TauntEnable(ONcCharacter_TauntEnable_Close);
		}

		if (!ONgDisableVisActive) {
			// since we want to draw this character, force it to be active, and
			// make sure it stays active by zeroing its inactive timer.
			ONrForceActiveCharacter(current_character);
			current_character->inactive_timer = 0;
		}

		active_character = ONrGetActiveCharacter(current_character);
		if (active_character == NULL) {
			continue;
		}

		current_character->flags |= ONcCharacterFlag_Draw;
		outVisibleChars[num_visible] = current_character;
		outVisibleActiveChars[num_visible] = active_character;
		num_visible++;
	}

	*outNumVisible = num_visible;
}



static void ONrGameState_ComputeCharacterLOD(UUtUns32 inNumVisible, ONtCharacter **inVisibleChars, ONtActiveCharacter **inVisibleActiveChars)
{
	UUtInt32 bias;
	UUtInt32 max_polygons = ONrMotoko_GraphicsQuality_CharacterPolygonCount();
	ONtCharacter *current_character;
	ONtActiveCharacter *active_character;
	UUtUns16 itr;
	UUtInt32 polygon_count;

	if ((ONgForceLOD >= TRcBody_SuperLow) && (ONgForceLOD <= TRcBody_SuperHigh)) {
		// force all characters to a specific LOD
		for(itr = 0; itr < inNumVisible; itr++) {
			inVisibleActiveChars[itr]->curBody = ONgForceLOD;
			inVisibleActiveChars[itr]->desiredBody = ONgForceLOD;
			inVisibleActiveChars[itr]->desiredBodyFrames = 0;
		}
		return;
	}

	// step 2: sort the visible chracacters by distance from camera
	AUrQSort_32(inVisibleChars, inNumVisible, distance_from_camera_compare);
	for(itr = 0; itr < inNumVisible; itr++) {
		inVisibleActiveChars[itr] = ONrGetActiveCharacter(inVisibleChars[itr]);
		UUmAssertReadPtr(inVisibleActiveChars[itr], sizeof(*inVisibleActiveChars[itr]));
	}

	// step 3: compute the base LOD
	for(itr = 0; itr < inNumVisible; itr++) {
		TRtBodySelector lod_itr;
		ONtLODConstants *current_lod_constants;

		current_character = inVisibleChars[itr];
		active_character = ONrGetActiveCharacter(current_character);
		UUmAssertReadPtr(active_character, sizeof(*active_character));

		// note: we must re-set up active character list due to sorting
		inVisibleActiveChars[itr] = active_character;
		current_lod_constants = &(current_character->characterClass->lodConstants);

		for(lod_itr = TRcBody_SuperLow; lod_itr < TRcBody_SuperHigh; lod_itr++) {
			if (current_character->distance_from_camera_squared > current_lod_constants->distance_squared[lod_itr]) {
				break;
			}
		}
		active_character->baseBody = lod_itr;
	}

	// compute our polygon total
	polygon_count = ONrCharacterList_GetPolygonCount(inVisibleChars, inVisibleActiveChars, inNumVisible, 0);

	// based on that total, bias all the LODs up or down and recompute the total
	if (polygon_count <= max_polygons) {
		for(bias = 1; bias < 5; bias++) {
			polygon_count = ONrCharacterList_GetPolygonCount(inVisibleChars, inVisibleActiveChars, inNumVisible, bias);

			if (polygon_count > max_polygons) {
				break;
			}
		}
	}
	else {
		for(bias = -1; bias > -5; bias--) {
			polygon_count = ONrCharacterList_GetPolygonCount(inVisibleChars, inVisibleActiveChars, inNumVisible, bias);

			if (polygon_count <= max_polygons) {
				break;
			}
		}
	}

	// if after that process our polygon_count is too high, reduce the bias by 1
	if (polygon_count > max_polygons) {
		bias -= 1;
	}

	// setup desired bodies
	for(itr = 0; itr < inNumVisible; itr++) {
		TRtBodySelector desired_body;
		UUtInt32 delta_polygon_count;

		current_character = inVisibleChars[itr];
		active_character = inVisibleActiveChars[itr];
		desired_body = active_character->baseBody + bias;

		// can we raise this models polygon count ?
		delta_polygon_count = ONrCharacter_GetPolygonCount(current_character, active_character, bias + 1) -
								ONrCharacter_GetPolygonCount(current_character, active_character, bias);

		if ((delta_polygon_count + polygon_count) < max_polygons) {
			polygon_count += delta_polygon_count;
			desired_body += 1;
		}

		// clip desired_body to the limit
		desired_body = UUmPin(desired_body, TRcBody_SuperLow, TRcBody_SuperHigh);

		// help out the closest two characters a little
		if (0 == itr) {
			desired_body = UUmMax(desired_body, TRcBody_Medium);
		}
		else if (1 == itr) {
			desired_body = UUmMax(desired_body, TRcBody_Low);
		}

		// check to see if we should change the body we are using to draw this character
		if (desired_body == active_character->curBody) {
			active_character->desiredBodyFrames = 0;
		}
		else {
			active_character->desiredBodyFrames += 1;
		}

		active_character->desiredBody = desired_body;

		if (TRcBody_NotDrawn == active_character->curBody) {
			active_character->curBody = active_character->desiredBody;
		}
		else {
			UUtBool can_switch = UUcTrue;

			// try not to switch anyone close while they are standing, looks bad
			if (ONrCharacter_IsStill(current_character)) {
				can_switch = UUcFalse;
			}

			if (can_switch) {
				if (active_character->desiredBody < active_character->curBody) {
					if (active_character->desiredBodyFrames > 6) {
						active_character->curBody = desired_body;
					}
				}
				else if (active_character->desiredBody > active_character->curBody) {
					if (active_character->desiredBodyFrames > 9) {
						active_character->curBody = desired_body;
					}
				}
			}
		}

		if (ONgGameState->local.in_cutscene) {
			active_character->curBody = UUmMax(active_character->curBody, TRcBody_Low);
		}
	}

	return;
}

static void ONrCharacter_PurgeShadows(void)
{
	UUtUns32 itr;
	UUtUns32 active_character_count = ONrGameState_ActiveCharacterList_Count();
	ONtCharacter **active_character_list = ONrGameState_ActiveCharacterList_Get();

	for(itr = 0; itr < active_character_count; itr++)
	{
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(active_character_list[itr]);

		if (NULL != active_character) {
			ONiCharacter_DeleteShadow(active_character);
		}
	}
}

void ONrGameState_DisplayCharacters(void)
{
	const UUtUns16 screen_width = M3rDraw_GetWidth();
	const UUtUns16 screen_height = M3rDraw_GetHeight();
	UUtUns16 itr;
	M3tPoint3D cameraLocation;
	UUtUns32 vis_count;
	ONtCharacter *character;
	ONtCharacter *vis_list[ONcMaxCharacters];
	ONtActiveCharacter *vis_active_list[ONcMaxCharacters];

	M3rCamera_GetViewData(ONgVisibilityCamera, &cameraLocation, NULL, NULL);

	if (ONgShow_Corpses) {
		ONrCorpse_Display(&cameraLocation);
	}

	ONrGameState_ComputeCharacterVisibility(&vis_count, vis_list, vis_active_list);
	ONrGameState_ComputeCharacterLOD(vis_count, vis_list, vis_active_list);

	ONgNumCharactersDrawn = 0;

	{
		// CB: DEBUGGING display code, this can be taken out for final
		UUtUns32 present_character_count;
		ONtCharacter **present_character_list;

		AI2rDisplayGlobalDebuggingInfo();
#if DEBUG_SHOWINTERSECTIONS
		if (TRgDisplayIntersections && (gFramesSinceIntersection < 600)) {
			TRrDisplayLastIntersection();
			if (!ONrGameState_IsSingleStep()) {
				gFramesSinceIntersection++;
			}
		}
#endif

#if DEBUG_ATTACKEXTENT
		// CB: more temp debugging code
		if (character->charType == ONcChar_Player) {
			ONiCharacter_TempDebug_DrawAttackExtent(ONgGameState->local.playerCharacter);
		}
#endif

		present_character_count = ONrGameState_PresentCharacterList_Count();
		present_character_list = ONrGameState_PresentCharacterList_Get();
		for (itr = 0; itr < present_character_count; itr++) {
			character = present_character_list[itr];

			if (0 == (character->flags & ONcCharacterFlag_InUse)) continue;
			if (character->flags & ONcCharacterFlag_Dead) continue;

			AI2rDisplayDebuggingInfo(character);
		}
	}

	// if we have an active character in follow mode then draw the laser sight
	ONgDrawLaserSight = UUcFalse;

	if (ONgGameState->local.playerActiveCharacter != NULL) {
		if (ONgGameState->local.playerCharacter != NULL) {
			if (ONgGameState->local.playerCharacter->inventory.weapons[0] != NULL) {
				if (CAcMode_Follow == CAgCamera.mode) {
					ONiDrawWeaponSight(ONgGameState->local.playerCharacter);
				}
			}
		}
	}

	// get rid of all of our old shadows
	ONrCharacter_PurgeShadows();

	for(itr = 0; itr < vis_count; itr++) {
		character = vis_list[itr];

		if (0 == (character->flags & ONcCharacterFlag_InUse)) continue;
		if (0 == (character->flags & ONcCharacterFlag_Draw)) continue;

		UUmAssert(!(character->flags & ONcCharacterFlag_Dead_4_Gone));

		ONiCharacter_Display(character, vis_active_list[itr], &cameraLocation);
	}
}

void ONrCharacter_SetAnimationInternal(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter,
									   TRtAnimState inFromState, TRtAnimType inNextAnimType, const TRtAnimation *inAnimation)
{
	ONtCharacterClass		*characterClass = ioCharacter->characterClass;
	TRtAnimationCollection	*collection = characterClass->animations;
	ONtCharacterIndexType index = ONrCharacter_GetIndex(ioCharacter);
	UUtUns16 animType;

	UUmAssert(NULL != inAnimation);

	animType = TRrAnimation_GetType(inAnimation);

	ioActiveCharacter->animation = inAnimation;
	ioActiveCharacter->animFrame = 0;
	ioActiveCharacter->curFromState = inFromState;
	ioActiveCharacter->curAnimType = animType;
	ioActiveCharacter->nextAnimType = inNextAnimType;
	ioActiveCharacter->nextAnimState = TRrAnimation_GetTo(inAnimation);

	UUmAssertTrigRange(ioCharacter->facing);

	if (gDebugCharacters && (index == gDebugCharacterTarget)) {
		COrConsole_Printf("anim %s state %s type %s length %d",
			TRrAnimation_GetName(inAnimation),
			ONrAnimStateToString(TRrAnimation_GetTo(inAnimation)),
			ONrAnimTypeToString(TRrAnimation_GetType(inAnimation)),
			TRrAnimation_GetDuration(inAnimation));
	}

	return;
}

UUtUns16 ONrCharacter_ForcedFollowingAnimType(ONtCharacter *ioCharacter, UUtUns16 inPrevAnimType)
{
	switch(inPrevAnimType) {
		case ONcAnimType_Teleport_In:
			return ONcAnimType_Teleport_Out;

		default:
			return ONcAnimType_None;
	}
}

void ONrCharacter_NextAnimation(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUtUns16 softPause, hardPause;
	UUtUns16 nextType;
	const TRtAnimation *nextAnimation;
	UUtUns16 numStitchingFrames;

	UUmAssert(ioCharacter != NULL);
	UUmAssert(ioActiveCharacter != NULL);
 	UUmAssert(ioActiveCharacter->animation != NULL);

	ioCharacter->flags &= ~ONcCharacterFlag_HastyAnim;

	nextType = ONrCharacter_ForcedFollowingAnimType(ioCharacter, ioActiveCharacter->curAnimType);
	if (nextType == ONcAnimType_None) {
		nextType = ioActiveCharacter->nextAnimType;
	}

	if (ioActiveCharacter->animationLockFrames > 0) {
		nextAnimation = ioActiveCharacter->animation;
	}
	else {
		nextAnimation = TRrCollection_Lookup(ioCharacter->characterClass->animations, nextType, ioActiveCharacter->nextAnimState, ioActiveCharacter->animVarient);

		if (NULL == nextAnimation) {
			COrConsole_Printf("invalid next animation");

			nextAnimation = TRrCollection_Lookup(ioCharacter->characterClass->animations, ONcAnimType_Stand, ONcAnimState_Standing, ioActiveCharacter->animVarient);
		}
	}

	nextAnimation = RemapAnimationHook(ioCharacter, ioActiveCharacter, nextAnimation);

	softPause = TRrAnimation_GetSoftPause(ioActiveCharacter->animation, nextAnimation);
	hardPause = TRrAnimation_GetHardPause(ioActiveCharacter->animation, nextAnimation);

	ioActiveCharacter->softPauseFrames = UUmMax(ioActiveCharacter->softPauseFrames, softPause);
	ioActiveCharacter->hardPauseFrames = UUmMax(ioActiveCharacter->hardPauseFrames, hardPause);

	numStitchingFrames = TRrAnimation_GetEndInterpolation(ioActiveCharacter->animation, nextAnimation);

	ONiCharacter_OldAnimationHook(ioCharacter, ioActiveCharacter, ONcApplyFinalRotation, nextAnimation);

	if (numStitchingFrames > 0) {
		ONrCharacter_Stitch(ioCharacter, ioActiveCharacter, ioActiveCharacter->nextAnimState, nextAnimation, numStitchingFrames);
	}
	else {
		ONrCharacter_SetAnimationInternal(ioCharacter, ioActiveCharacter, ioActiveCharacter->nextAnimState, ONcAnimType_None, nextAnimation);
	}

	ONrCharacter_NewAnimationHook(ioCharacter, ioActiveCharacter);

	return;
}

void ONrCharacter_SetFacing(ONtCharacter *ioCharacter, float inFacing)
{
	UUmAssertTrigRange(inFacing);

	ioCharacter->facing = inFacing;
	ioCharacter->desiredFacing = inFacing;

	return;
}

void ONrCharacter_SetDesiredFacing(ONtCharacter *ioCharacter, float inFacing)
{
	UUmAssertTrigRange(inFacing);

	ioCharacter->desiredFacing = inFacing;

	return;
}

void ONrCharacter_AdjustFacing(ONtCharacter *ioCharacter, float inFacingDelta)
{
	UUmAssert(inFacingDelta >= -M3c2Pi);
	UUmAssert(inFacingDelta <= M3c2Pi);
	UUmAssertTrigRange(ioCharacter->facing);
	UUmAssertTrigRange(ioCharacter->desiredFacing);

	ioCharacter->facing += inFacingDelta;
	ioCharacter->desiredFacing += inFacingDelta;

	UUmTrig_Clip(ioCharacter->facing);
	UUmTrig_Clip(ioCharacter->desiredFacing);

	UUmAssertTrigRange(ioCharacter->facing);
	UUmAssertTrigRange(ioCharacter->desiredFacing);

	return;
}

void ONrCharacter_AdjustDesiredFacing(ONtCharacter *ioCharacter, float inFacingDelta)
{
	UUmAssert(inFacingDelta >= -M3c2Pi);
	UUmAssert(inFacingDelta <= M3c2Pi);
	UUmAssertTrigRange(ioCharacter->desiredFacing);

	ioCharacter->desiredFacing += inFacingDelta;

	UUmTrig_Clip(ioCharacter->desiredFacing);

	UUmAssertTrigRange(ioCharacter->desiredFacing);

	return;
}
static float angle_to_relative_angle(float angle)
{
	float new_angle = angle;

	if (new_angle > M3cPi) {
		new_angle -= M3c2Pi;
	}
	else if (new_angle < -M3cPi) {
		new_angle += M3c2Pi;
	}

	return new_angle;
}

float ONrCharacter_GetDesiredFacingOffset(const ONtCharacter *inCharacter)
{
	float forward_angle;
	float forward_to_desired_angle;
	float facing_to_forward_angle;
	float facing_to_desired_facing;

	UUmAssertTrigRange(inCharacter->facing);
	UUmAssertTrigRange(inCharacter->desiredFacing);

	forward_angle = inCharacter->desiredFacing - inCharacter->facingModifier;
	forward_to_desired_angle = inCharacter->desiredFacing - forward_angle;
	forward_to_desired_angle = angle_to_relative_angle(forward_to_desired_angle);

	facing_to_forward_angle = forward_angle - inCharacter->facing;
	facing_to_forward_angle = angle_to_relative_angle(facing_to_forward_angle);

	facing_to_desired_facing = inCharacter->desiredFacing - inCharacter->facing;

	if ((forward_to_desired_angle > 0.f) && (facing_to_forward_angle > 0.f))
	{
		// always turn positive
		if (facing_to_desired_facing < 0.f) {
			facing_to_desired_facing += M3c2Pi;
		}
	}
	else if ((forward_to_desired_angle < 0.f) && (facing_to_forward_angle < 0.f))
	{
		// always turn negative
		if (facing_to_desired_facing > 0.f) {
			facing_to_desired_facing -= M3c2Pi;
		}
	}
	else
	{
		facing_to_desired_facing = angle_to_relative_angle(facing_to_desired_facing);
	}

	return facing_to_desired_facing;
}


static float iSignAngle(float angle)
{
	float result = angle;

	UUmAssertTrigRange(angle);

	if (angle >= M3cPi) {
		result = angle - M3c2Pi;
	}

	UUmAssert(result <= M3cPi);

	return result;
}

float ONrCharacter_GetCosmeticFacing(const ONtCharacter *inCharacter)
{
	float facing = inCharacter->desiredFacing - inCharacter->facingModifier;

	UUmTrig_Clip(facing);
	UUmAssertTrigRange(facing);

	return facing;
}

void ONrCharacter_GetEarPosition(const ONtCharacter *inCharacter, M3tPoint3D *outPoint)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character == NULL) {
		// approximation to head position
		*outPoint = inCharacter->actual_position;
		outPoint->y += ONcCharacterHeight;
	} else {
		M3tPoint3D center_of_head = MUrMatrix_GetTranslation(active_character->matricies + ONcHead_Index);
		*outPoint = inCharacter->actual_position;
		outPoint->y = center_of_head.y;
	}

	return;
}

// gets a point that is used to test visibility from
void ONrCharacter_GetEyePosition(const ONtCharacter *inCharacter, M3tPoint3D *outPoint)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character == NULL) {
		// approximation to head position
		*outPoint = inCharacter->actual_position;
		outPoint->y += ONcCharacterHeight;
	} else {
		*outPoint = MUrMatrix_GetTranslation(active_character->matricies + ONcHead_Index);
	}
}

void ONrCharacter_GetPelvisPosition(const ONtCharacter *inCharacter, M3tPoint3D *outPoint)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character == NULL) {
		// approximation to pelvis position
		*outPoint = inCharacter->actual_position;
		outPoint->y += inCharacter->heightThisFrame;
	} else {
		*outPoint = MUrMatrix_GetTranslation(active_character->matricies + ONcPelvis_Index);
	}
}

void ONrCharacter_GetFeetPosition(const ONtCharacter *inCharacter, M3tPoint3D *outPoint)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character == NULL) {
		// approximation to pelvis position
		*outPoint = inCharacter->actual_position;
	} else {
		*outPoint = MUrMatrix_GetTranslation(active_character->matricies + ONcPelvis_Index);
	}
	outPoint->y = inCharacter->feetLocation + 1.0f;		// ensure point isn't below floor
}

void ONrCharacter_GetFacingVector(const ONtCharacter *inCharacter, M3tVector3D *outVector)
{
	/*******************
	* Returns a normalized vector corrosponding to the character's heading,
	* and a damped version of the same thing.
	*/

	const M3tPoint3D defFacing = {0,0,1};
	float facing = ONrCharacter_GetCosmeticFacing(inCharacter);

	UUmAssertReadPtr(inCharacter, sizeof(*inCharacter));
	UUmAssertWritePtr(outVector, sizeof(*outVector));

	MUrPoint_RotateYAxis(&defFacing, facing, outVector);
	MUrNormalize(outVector);

	return;
}

void ONrCharacter_GetAttackVector(const ONtCharacter *inCharacter, ONtActiveCharacter *inActiveCharacter, M3tVector3D *outVector)
{
	float rotate = 0.f;
	TRtDirection direction = TRrAnimation_GetDirection(inActiveCharacter->animation, UUcFalse);

	ONrCharacter_GetFacingVector(inCharacter, outVector);

	switch(direction) {
		case TRcDirection_None:
		case TRcDirection_360:
		case TRcDirection_Forwards:
			rotate = 0.0f;
		break;

		case TRcDirection_Left:
			rotate = 1.5f * M3cPi;
		break;

		case TRcDirection_Right:
			rotate = 0.5f * M3cPi;
		break;

		case TRcDirection_Back:
			rotate = M3cPi;
		break;

		default:
			UUmAssert(!"ONrCharacter_GetAttackVector: unknown moveDirection");
			rotate = 0.0f;
		break;
	}

	if (0.f != rotate) {
		MUrPoint_RotateYAxis((void *)outVector, rotate, (void *)outVector);
	}

	return;
}

UUtBool ONrAnimType_ThrowIsFromFront(TRtAnimType inAnimType)
{
	switch(inAnimType) {
		case ONcAnimType_Throw_Forward_Punch:
		case ONcAnimType_Throw_Forward_Kick:
		case ONcAnimType_Run_Throw_Forward_Punch:
		case ONcAnimType_Run_Throw_Forward_Kick:
			return UUcTrue;
	}

	return UUcFalse;
}

UUtBool ONrAnimType_IsVictimType(TRtAnimType inAnimType)
{
	/******************
	* Returns true if the character is running knockdown, throw, hit
	*/

	UUtBool busy;

	switch(inAnimType)
	{
		case ONcAnimType_Land_Dead:

		case ONcAnimType_Hit_Head:
		case ONcAnimType_Hit_Body:
		case ONcAnimType_Hit_Foot:

		case ONcAnimType_Blownup:
		case ONcAnimType_Knockdown_Head:
		case ONcAnimType_Knockdown_Body:
		case ONcAnimType_Knockdown_Foot:

		case ONcAnimType_Hit_Crouch:
		case ONcAnimType_Knockdown_Crouch:

		case ONcAnimType_Hit_Fallen:

		case ONcAnimType_Hit_Head_Behind:
		case ONcAnimType_Hit_Body_Behind:
		case ONcAnimType_Hit_Foot_Behind:

		case ONcAnimType_Blownup_Behind:
		case ONcAnimType_Knockdown_Head_Behind:
		case ONcAnimType_Knockdown_Body_Behind:
		case ONcAnimType_Knockdown_Foot_Behind:

		case ONcAnimType_Hit_Crouch_Behind:
		case ONcAnimType_Knockdown_Crouch_Behind:
		case ONcAnimType_Hit_Foot_Ouch:
		case ONcAnimType_Hit_Jewels:

		case ONcAnimType_Stun:
		case ONcAnimType_Stagger:
		case ONcAnimType_Stagger_Behind:

		case ONcAnimType_Thrown1:
		case ONcAnimType_Thrown2:
		case ONcAnimType_Thrown3:
		case ONcAnimType_Thrown4:
		case ONcAnimType_Thrown5:
		case ONcAnimType_Thrown6:
		case ONcAnimType_Thrown7:
		case ONcAnimType_Thrown8:
		case ONcAnimType_Thrown9:
		case ONcAnimType_Thrown10:
		case ONcAnimType_Thrown11:
		case ONcAnimType_Thrown12:
		case ONcAnimType_Thrown13:
		case ONcAnimType_Thrown14:
		case ONcAnimType_Thrown15:
		case ONcAnimType_Thrown16:
		case ONcAnimType_Thrown17:
			busy = UUcTrue;
		break;

		default:
			busy = UUcFalse;
		break;
	}

	return busy;
}

UUtBool ONrCharacter_IsVictimAnimation(const ONtCharacter *inCharacter)
{
	return ONrAnimType_IsVictimType(ONrCharacter_GetAnimType(inCharacter));
}

UUtBool ONrCharacter_IsDefensive(const ONtCharacter *inCharacter)
{
	/******************
	* Returns true if the character is standing still
	*/

	const TRtAnimation *animation;
	TRtAnimType type;
	TRtAnimState fromState, toState;
	UUtBool canBlock;
	ONtActiveCharacter *active_character;

	UUmAssertReadPtr(inCharacter, sizeof(*inCharacter));
	active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character == NULL) {
		return UUcTrue;
	}

	animation = active_character->animation;

	fromState = TRrAnimation_GetFrom(animation);
	toState = TRrAnimation_GetTo(animation);
	type = TRrAnimation_GetType(animation);

	canBlock = UUcFalse;

	// one method is if we are standing/crouching or going betweeen
	{
		UUtBool fromIsCrouchOrStand, toIsCrouchOrStand;
		UUtBool typeIsCrouchStandTurnOrBlock;

		fromIsCrouchOrStand =
			(ONcAnimState_Crouch == fromState) ||
			(ONcAnimState_Crouch_Start == fromState) ||
			(ONcAnimState_Standing == fromState);

		toIsCrouchOrStand =
			(ONcAnimState_Crouch == fromState) ||
			(ONcAnimState_Crouch_Start == fromState) ||
			(ONcAnimState_Standing == fromState);

		typeIsCrouchStandTurnOrBlock =
			(type == ONcAnimType_Crouch_Turn_Left) ||
			(type == ONcAnimType_Crouch_Turn_Right) ||
			(type == ONcAnimType_Standing_Turn_Left) ||
			(type == ONcAnimType_Standing_Turn_Right) ||
			(type == ONcAnimType_Block) ||
			(type == ONcAnimType_Stand) ||
			(type == ONcAnimType_Crouch);

		canBlock |= fromIsCrouchOrStand && toIsCrouchOrStand && typeIsCrouchStandTurnOrBlock;
	}

	// another is if we are moving backwards
	{
		canBlock |= ONrCharacter_IsMovingBack(inCharacter);
	}

	return canBlock;
}

UUtBool ONrCharacter_IsIdle(const ONtCharacter *inCharacter)
{
	switch (ONrCharacter_GetAnimType(inCharacter))
	{
		case ONcAnimType_Idle:
		case ONcAnimType_Stand:
			return UUcTrue;
	}

	return UUcFalse;
}


UUtBool ONrCharacter_IsMovingBack(const ONtCharacter *inCharacter)
{
	switch(ONrCharacter_GetAnimType(inCharacter))
	{
		case ONcAnimType_Run_Backwards_Start:
		case ONcAnimType_Walk_Backwards_Start:
		case ONcAnimType_Run_Backwards:
		case ONcAnimType_Walk_Backwards:
		case ONcAnimType_Crouch_Run_Backwards:
		case ONcAnimType_Crouch_Walk_Backwards:
			return UUcTrue;
	}

	return UUcFalse;
}

UUtBool ONrCharacter_IsPoweringUp(const ONtCharacter *inCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character == NULL) {
		return UUcFalse;
	}

	return ((active_character->curAnimType == ONcAnimType_Powerup) &&
			(TRrAnimation_GetFrom(active_character->animation) == ONcAnimState_Powerup));
}

UUtBool ONrCharacter_IsInactiveUpright(const ONtCharacter *inCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character == NULL) {
		return UUcTrue;
	}

	switch(active_character->curFromState) {
		case ONcAnimState_Fallen_Back:
		case ONcAnimState_Fallen_Front:
		case ONcAnimState_Sliding:
		case ONcAnimState_Run_Slide:
		case ONcAnimState_Run_Back_Slide:
		case ONcAnimState_Lie_Back:
			return UUcFalse;
	}

	switch(active_character->nextAnimState) {
		case ONcAnimState_Fallen_Back:
		case ONcAnimState_Fallen_Front:
		case ONcAnimState_Sliding:
		case ONcAnimState_Run_Slide:
		case ONcAnimState_Run_Back_Slide:
		case ONcAnimState_Lie_Back:
			return UUcFalse;
	}

	switch(active_character->curAnimType)
	{
		case ONcAnimType_Stand:
		case ONcAnimType_Idle:
		case ONcAnimType_Crouch:
		case ONcAnimType_Standing_Turn_Left:
		case ONcAnimType_Standing_Turn_Right:
		case ONcAnimType_Crouch_Turn_Left:
		case ONcAnimType_Crouch_Turn_Right:
		case ONcAnimType_Run_Start:
		case ONcAnimType_Run:
		case ONcAnimType_Run_Stop:
		case ONcAnimType_Run_Backwards_Start:
		case ONcAnimType_Run_Backwards:
		case ONcAnimType_Run_Sidestep_Left:
		case ONcAnimType_Run_Sidestep_Right:
		case ONcAnimType_Run_Sidestep_Left_Start:
		case ONcAnimType_Run_Sidestep_Right_Start:
		case ONcAnimType_Walk_Start:
		case ONcAnimType_Walk:
		case ONcAnimType_Walk_Stop:
		case ONcAnimType_1Step_Stop:
		case ONcAnimType_Walk_Backwards_Start:
		case ONcAnimType_Walk_Backwards:
		case ONcAnimType_Walk_Sidestep_Left:
		case ONcAnimType_Walk_Sidestep_Right:
		case ONcAnimType_Crouch_Run:
		case ONcAnimType_Crouch_Walk:
		case ONcAnimType_Crouch_Run_Backwards:
		case ONcAnimType_Crouch_Walk_Backwards:
		case ONcAnimType_Crouch_Run_Sidestep_Left:
		case ONcAnimType_Crouch_Run_Sidestep_Right:
		case ONcAnimType_Crouch_Walk_Sidestep_Left:
		case ONcAnimType_Crouch_Walk_Sidestep_Right:
			return UUcTrue;

		default:
			return UUcFalse;
	}
}

UUtBool ONrCharacter_IsStandingRunning(const ONtCharacter *inCharacter)
{
	switch(ONrCharacter_GetAnimType(inCharacter))
	{
		case ONcAnimType_Run_Start:
		case ONcAnimType_Run_Backwards_Start:
		case ONcAnimType_Run:
		case ONcAnimType_Run_Backwards:
			return UUcTrue;
	}

	return UUcFalse;
}

UUtBool ONrCharacter_IsUnableToTurn(const ONtCharacter *inCharacter)
{
	UUtBool unable_to_turn;
	ONtActiveCharacter *active_character;

	UUmAssertReadPtr(inCharacter, sizeof(*inCharacter));
	active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character == NULL) {
		return UUcFalse;
	}

	switch(active_character->curAnimType)
	{
		case ONcAnimType_Stand:
		case ONcAnimType_Crouch:
			// AIs can turn in these animtypes depending on their animation state
			if (inCharacter->flags & ONcCharacterFlag_AIMovement) {
				switch(active_character->nextAnimState)
				{
					case ONcAnimState_Fallen_Back:
					case ONcAnimState_Fallen_Front:
					case ONcAnimState_Sliding:
					case ONcAnimState_Run_Slide:
					case ONcAnimState_Run_Back_Slide:
					case ONcAnimState_Lie_Back:
						unable_to_turn = UUcTrue;
					break;

					default:
						unable_to_turn = UUcFalse;
					break;
				}
			}
			else {
				// player characters can never turn in these animtypes
				return UUcTrue;
			}
		break;

		case ONcAnimType_Block:
		case ONcAnimType_Prone:
			unable_to_turn = UUcTrue;
		break;

		default:
			unable_to_turn = UUcFalse;
		break;
	}

	unable_to_turn |= TRrAnimation_TestFlag(active_character->animation, ONcAnimFlag_ThrowSource);
	unable_to_turn |= TRrAnimation_TestFlag(active_character->animation, ONcAnimFlag_ThrowTarget);
	unable_to_turn |= ONrCharacter_IsVictimAnimation(inCharacter);

	return unable_to_turn;
}

UUtBool ONrCharacter_IsPickingUp(const ONtCharacter *inCharacter)
{
	switch(ONrCharacter_GetAnimType(inCharacter))
	{
		case ONcAnimType_Pickup_Object_Mid:
		case ONcAnimType_Pickup_Pistol_Mid:
		case ONcAnimType_Pickup_Rifle_Mid:

		case ONcAnimType_Pickup_Object:
		case ONcAnimType_Pickup_Pistol:
		case ONcAnimType_Pickup_Rifle:
			return UUcTrue;
	}

	return UUcFalse;
}

UUtBool ONrAnimType_IsStartle(TRtAnimType inAnimType)
{
	switch(inAnimType)
	{
		case ONcAnimType_Startle_Left:
		case ONcAnimType_Startle_Right:
		case ONcAnimType_Startle_Back:
		case ONcAnimType_Startle_Forward:
			return UUcTrue;

		default:
			return UUcFalse;
	}
}

UUtBool ONrCharacter_IsStartling(const ONtCharacter *inCharacter, UUtUns16 *outStartleFrame)
{
	ONtActiveCharacter *active_character;

	UUmAssertReadPtr(inCharacter, sizeof(*inCharacter));
	active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character == NULL) {
		return UUcFalse;
	}

	if (ONrAnimType_IsStartle(active_character->curAnimType)) {
		*outStartleFrame = active_character->animFrame;
		return UUcTrue;

	} else if (ONrAnimType_IsStartle(active_character->nextAnimType)) {
		*outStartleFrame = 0;
		return UUcTrue;

	} else {
		return UUcFalse;
	}
}

UUtBool ONrCharacter_IsStaggered(const ONtCharacter *inCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

	return ((active_character != NULL) && (active_character->staggerStun > 0));
}

UUtBool ONrCharacter_IsFallen(const ONtCharacter *inCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

	return ((active_character != NULL) && ONrAnimState_IsFallen(active_character->nextAnimState));
}

UUtBool ONrCharacter_IsDizzy(const ONtCharacter *inCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

	return ((active_character != NULL) && (active_character->dizzyStun > 0));
}

UUtBool ONrCharacter_IsPlayingFilm(const ONtCharacter *inCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character == NULL) {
		return UUcFalse;
	} else {
		return ((active_character->filmState.flags & ONcFilmFlag_Play) > 0);
	}
}

// used by AI2 to determine whether we might overshoot a path point
UUtBool ONrCharacter_IsKeepingMoving(ONtCharacter *inCharacter)
{
	ONtActiveCharacter *active_character;

	UUmAssertReadPtr(inCharacter, sizeof(*inCharacter));
	active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character == NULL) {
		return UUcFalse;
	}

	switch(TRrAnimation_GetTo(active_character->animation))
	{
		case ONcAnimState_Running_Left_Down:
		case ONcAnimState_Running_Right_Down:
		case ONcAnimState_Walking_Left_Down:
		case ONcAnimState_Walking_Right_Down:
		case ONcAnimState_Run_Start:
		case ONcAnimState_Run_Accel:
		case ONcAnimState_Run_Sidestep_Left:
		case ONcAnimState_Run_Sidestep_Right:
		case ONcAnimState_Run_Back_Start:
		case ONcAnimState_Running_Back_Right_Down:
		case ONcAnimState_Running_Back_Left_Down:
		case ONcAnimState_Running_Upstairs_Right_Down:
		case ONcAnimState_Running_Upstairs_Left_Down:
		case ONcAnimState_Sidestep_Left_Left_Down:
		case ONcAnimState_Sidestep_Left_Right_Down:
		case ONcAnimState_Sidestep_Right_Left_Down:
		case ONcAnimState_Sidestep_Right_Right_Down:
		case ONcAnimState_Sidestep_Left_Start:
		case ONcAnimState_Sidestep_Right_Start:
		case ONcAnimState_Walking_Sidestep_Left:
		case ONcAnimState_Crouch_Walk:
		case ONcAnimState_Walking_Sidestep_Right:
		case ONcAnimState_Walking_Back_Left_Down:
		case ONcAnimState_Walking_Back_Right_Down:
		case ONcAnimState_Sidestep_Left_Start_Long_Name:
		case ONcAnimState_Sidestep_Right_Start_Long_Name:
			return UUcTrue;

		default:
			return UUcFalse;
	}
}

UUtBool ONrCharacter_IsStandingRunningForward(const ONtCharacter *inCharacter)
{
	ONtActiveCharacter *active_character;
	const TRtAnimation *animation;
	TRtAnimType type;
	TRtAnimState fromState, toState;
	UUtBool isStandingRunning;

	UUmAssertReadPtr(inCharacter, sizeof(*inCharacter));
	active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character == NULL) {
		return UUcFalse;
	}

	animation = active_character->animation;

	fromState = TRrAnimation_GetFrom(animation);
	toState = TRrAnimation_GetTo(animation);
	type = TRrAnimation_GetType(animation);

	switch(type)
	{
		case ONcAnimType_Run:
		case ONcAnimType_Run_Start:
			isStandingRunning = UUcTrue;
		break;

		default:
			isStandingRunning = UUcFalse;
		break;
	}

	return isStandingRunning;
}



static void ONrCharacter_Stitch(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter,
								TRtAnimState fromState, const TRtAnimation *inNewAnim, UUtInt32 inStitchCount)
{
	TRtAnimTime maxFrame = TRrAnimation_GetDuration(ioActiveCharacter->animation) - 1;

	const TRtAnimation *animation;
	TRtAnimTime animFrame;

	UUmAssert(inStitchCount > 0);
	UUmAssert(ioCharacter->heightThisFrame < 128.f);

	animation = ioActiveCharacter->animation;
	animFrame = UUmMin(ioActiveCharacter->animFrame, maxFrame);

	// NOTE: some day the movefasts could be removed
	UUrMemory_MoveFast(ioActiveCharacter->curAnimOrientations, ioActiveCharacter->stitch.positions, sizeof(M3tQuaternion) * ONcNumCharacterParts);

	ioActiveCharacter->stitch.stitching = UUcTrue;
	ONrCharacter_GetVelocityEstimate(ioCharacter, ioActiveCharacter, &(ioActiveCharacter->stitch.velocity));
	ioActiveCharacter->stitch.height = ioCharacter->heightThisFrame;
	ioActiveCharacter->stitch.count = UUmMax(inStitchCount, ioActiveCharacter->stitch.count - ioActiveCharacter->stitch.itr);
	ioActiveCharacter->stitch.itr = 0;
	ioActiveCharacter->stitch.fromState = fromState;

	if (ioActiveCharacter->stitch.stitching) {
		ioActiveCharacter->stitch.count = UUmMax(inStitchCount, ioActiveCharacter->stitch.count - ioActiveCharacter->stitch.itr);
	} else {
		ioActiveCharacter->stitch.count = (UUtUns16) inStitchCount;
	}

	ONrCharacter_SetAnimationInternal(ioCharacter, ioActiveCharacter, fromState, ONcAnimType_None, inNewAnim);

	if (TRrAnimation_TestFlag(ioActiveCharacter->animation, ONcAnimFlag_NoInterpVelocity))
	{
		ioActiveCharacter->stitch.velocity.x = 0.f;
		ioActiveCharacter->stitch.velocity.y = 0.f;
		ioActiveCharacter->stitch.velocity.z = 0.f;
	}

	UUmAssert((ioActiveCharacter->stitch.velocity.x < 1e9f) && (ioActiveCharacter->stitch.velocity.x > -1e9f));
	UUmAssert((ioActiveCharacter->stitch.velocity.y < 1e9f) && (ioActiveCharacter->stitch.velocity.y > -1e9f));
	UUmAssert((ioActiveCharacter->stitch.velocity.z < 1e9f) && (ioActiveCharacter->stitch.velocity.z > -1e9f));

	return;
}


static ONtAnimPriority iCharacter_BuildPriority(ONtCharacter *inCharacter, ONtActiveCharacter *ioActiveCharacter,
												const TRtAnimation *inAnimation, ONtAnimPriority inPriority)
{
	UUtBool atomic;
	const TRtAnimation *oldAnimation = ioActiveCharacter->animation;
	TRtAnimState oldFromState = TRrAnimation_GetFrom(oldAnimation);
	TRtAnimState oldToState = TRrAnimation_GetTo(oldAnimation);

	TRtAnimType  newType = TRrAnimation_GetType(inAnimation);
	TRtAnimState newFromState = TRrAnimation_GetFrom(inAnimation);
	TRtAnimState newToState = TRrAnimation_GetTo(inAnimation);

	ONtAnimPriority result = inPriority;
	UUtBool isShortcut = TRrAnimation_IsShortcut(inAnimation, oldToState);
	UUtBool type_is_equal = TRrAnimation_GetType(inAnimation) == ioActiveCharacter->curAnimType;
	UUtBool varient_is_equal = TRrAnimation_GetVarient(inAnimation) == TRrAnimation_GetVarient(ioActiveCharacter->animation);
	UUtBool to_state_is_equal = TRrAnimation_GetTo(inAnimation) == TRrAnimation_GetTo(ioActiveCharacter->animation);

	atomic = TRrAnimation_IsAtomic(oldAnimation, ioActiveCharacter->animFrame);

	if (type_is_equal && varient_is_equal && to_state_is_equal) {
		result |= ONcAnimPriority_Queue;
	}
	else if (ONrCharacter_IsVictimAnimation(inCharacter)) {
		result |= ONcAnimPriority_Queue;
	}
	else if (TRrAnimation_TestFlag(oldAnimation, ONcAnimFlag_ThrowTarget)) {
		result |= ONcAnimPriority_Queue;
	}
	else if (TRrAnimation_TestFlag(oldAnimation, ONcAnimFlag_ThrowSource)) {
		result |= ONcAnimPriority_Queue;
	}
	else if (ONrAnimType_IsKnockdown(oldAnimation->type)) {
		result |= ONcAnimPriority_Queue;
	}
	else if (!atomic) {
		result |= ONcAnimPriority_Force;
	}
	else if (isShortcut && TRrAnimation_TestShortcutFlag(inAnimation, oldToState, ONcShortcutFlag_Nuke)) {
		result |= ONcAnimPriority_Force;
	}
	else {
		result |= ONcAnimPriority_Queue;
	}

	return result;
}

static UUtBool ONiAnimation_ValidSoftPause(const TRtAnimation *animation)
{
	TRtAnimType type;
	UUtBool valid;

	valid = !TRrAnimation_IsAttack(animation);
	if (!valid) goto exit;

	type = TRrAnimation_GetType(animation);

	switch(type)
	{
		case ONcAnimType_Stand:
		case ONcAnimType_Crouch:
		case ONcAnimType_Block:
		case ONcAnimType_Land:
		case ONcAnimType_Land_Forward:
		case ONcAnimType_Land_Right:
		case ONcAnimType_Land_Left:
		case ONcAnimType_Land_Back:
			valid = UUcTrue;
		break;

		default:
			valid = UUcFalse;
	}

exit:
	return valid;
}

static UUtBool ONiAnimation_ValidHardPause(const TRtAnimation *animation)
{
	TRtAnimType type;
	UUtBool valid;

	valid = ONiAnimation_ValidSoftPause(animation);
	if (!valid) goto exit;

	type = TRrAnimation_GetType(animation);

	switch(type)
	{
		case ONcAnimType_Block:
			valid = UUcFalse;
		break;
	}

exit:
	return valid;
}

UUtBool ONrCharacter_InBadVarientState(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUtBool excessive_varient = ((~ioActiveCharacter->animVarient) & TRrAnimation_GetVarient(ioActiveCharacter->animation)) > 0;

	return excessive_varient;
}

UUtBool ONrCharacter_DisallowStitch(UUtUns16 inCurAnimType, UUtUns16 inNextAnimType)
{
	if (inCurAnimType == inNextAnimType)
		return UUcTrue;

	switch(inCurAnimType) {
		case ONcAnimType_Run_Start:
			return (inNextAnimType == ONcAnimType_Run);

		case ONcAnimType_Walk_Start:
			return (inNextAnimType == ONcAnimType_Walk);

		case ONcAnimType_Run_Backwards_Start:
			return (inNextAnimType == ONcAnimType_Run_Backwards);

		case ONcAnimType_Walk_Backwards_Start:
			return (inNextAnimType == ONcAnimType_Walk_Backwards);

		case ONcAnimType_Run_Sidestep_Left_Start:
			return (inNextAnimType == ONcAnimType_Run_Sidestep_Left);

		case ONcAnimType_Run_Sidestep_Right_Start:
			return (inNextAnimType == ONcAnimType_Run_Sidestep_Right);
	}

	return UUcFalse;
}

const TRtAnimation *ONrCharacter_DoAnimation(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter,
											 ONtAnimPriority inPriority, UUtUns16 inAnimType)
{
	const UUtUns16 cMaxBacklook = 8;
	ONtCharacterClass		*characterClass = ioCharacter->characterClass;
	TRtAnimationCollection	*collection = characterClass->animations;
	const TRtAnimation *animation;
	UUtBool doStandingStillAnimation;

	UUmAssertReadPtr(ioCharacter, sizeof(*ioCharacter));
	UUmAssertReadPtr(ioActiveCharacter, sizeof(*ioActiveCharacter));
	UUmAssertReadPtr(characterClass, sizeof(*characterClass));
	UUmAssertReadPtr(collection, 4);
	UUmAssert(ONcAnimType_None != inAnimType);

	if (ioCharacter->flags & ONcCharacterFlag_Dead_1_Animating) {
		return NULL;
	}

	// lookup animation
	doStandingStillAnimation = (ONcAnimType_Stand == inAnimType) && (ONcAnimState_Standing == ioActiveCharacter->nextAnimState);
	animation = TRrCollection_Lookup(collection, inAnimType, ioActiveCharacter->nextAnimState, ioActiveCharacter->animVarient);

	if ((NULL == animation) &&  (ioActiveCharacter->animFrame < cMaxBacklook ) && (inPriority & ONcAnimPriority_BackInTime)) {
		animation = TRrCollection_Lookup(collection, inAnimType, ioActiveCharacter->curFromState, ioActiveCharacter->animVarient);

		inPriority &= (ONtAnimPriority)~ONcAnimPriority_Appropriate;
		inPriority &= (ONtAnimPriority)~ONcAnimPriority_Queue;
		inPriority |= ONcAnimPriority_Force;
	}

	if ((ioCharacter->flags & ONcCharacterFlag_BeingThrown) &&
		(ioActiveCharacter->animation != animation))
	{
		ioCharacter->flags &= ~ONcCharacterFlag_BeingThrown;
//COrConsole_Printf("Character no longer being thrown");
	}

	if (NULL == animation) { return NULL; }

	// check for pausing
	{
		UUtBool softPause;
		UUtBool hardPause;
		UUtBool valid;

		softPause =
			ioActiveCharacter->hitStun ||
			ioActiveCharacter->blockStun ||
			ioActiveCharacter->softPauseFrames ||
			(TRrAnimation_GetSoftPause(ioActiveCharacter->animation, animation) > 0);
		hardPause = ioActiveCharacter->hardPauseFrames || (TRrAnimation_GetHardPause(ioActiveCharacter->animation, animation) > 0);

		if (hardPause) {
			valid = ONiAnimation_ValidHardPause(animation);

			if (!valid) return NULL;
		}
		else if (softPause) {
			valid = ONiAnimation_ValidSoftPause(animation);

			if (!valid) return NULL;
		}
	}

	if (inPriority & ONcAnimPriority_Appropriate) {
		inPriority = iCharacter_BuildPriority(ioCharacter, ioActiveCharacter, animation, inPriority);
	}

	// if we aren't about to run a stand still animation consider switching out of fight stance
	if ((!doStandingStillAnimation) && (0 == ioActiveCharacter->fightFramesRemaining)) {
		ioActiveCharacter->animVarient &= ~ONcAnimVarientMask_Fight;
	}

	ioCharacter->flags &= ~ONcCharacterFlag_HastyAnim;

	if ((!ONrCharacter_InBadVarientState(ioCharacter, ioActiveCharacter) && ONrCharacter_DisallowStitch(ioActiveCharacter->curAnimType, inAnimType)) ||
		(ONcAnimPriority_Queue & inPriority)) {
		// queue up the animation
		ioActiveCharacter->nextAnimType = inAnimType;

		return animation;
	}

	UUmAssert(ONcAnimPriority_Force & inPriority);

	if (NULL != animation) {
		UUtUns8 stitchAmt = UUmMax(ONcNumStitchingFrames, ioActiveCharacter->minStitchingFrames);
		TRtAnimState fromState = ioActiveCharacter->nextAnimState;

		animation = RemapAnimationHook(ioCharacter, ioActiveCharacter, animation);

		if (NULL != ioActiveCharacter->animation) {
			ONiCharacter_OldAnimationHook(ioCharacter, ioActiveCharacter, ONcApplyFinalRotation, animation);
		}

		if (TRrAnimation_IsShortcut(animation, fromState)) {
			stitchAmt = TRrAnimation_GetShortcutLength(animation, fromState);
		}

		stitchAmt = UUmMin(stitchAmt, TRrAnimation_GetMaxInterpolation(animation));

		if (0 == stitchAmt) {
			ONrCharacter_SetAnimationInternal(ioCharacter, ioActiveCharacter, ioActiveCharacter->nextAnimState, ONcAnimType_None, animation);
		}
		else {
			ONrCharacter_Stitch(ioCharacter, ioActiveCharacter, ioActiveCharacter->nextAnimState, animation, stitchAmt);
		}

		ONrCharacter_NewAnimationHook(ioCharacter, ioActiveCharacter);
	}

	ioActiveCharacter->minStitchingFrames = 0;

	return animation;
}

static void ONiCharacter_DropInventoryItems(ONtCharacter *ioCharacter)
{
	UUtUns16				i;

	/*
	 * drop specifically placed "death candy"
	 */

	for (i = 0; i < ioCharacter->inventory.drop_ammo; i++)
	{
		WPrPowerup_Drop(WPcPowerup_AmmoBallistic, 1, ioCharacter, UUcFalse);
	}

	for (i = 0; i < ioCharacter->inventory.drop_cell; i++)
	{
		WPrPowerup_Drop(WPcPowerup_AmmoEnergy, 1, ioCharacter, UUcFalse);
	}

	for (i = 0; i < ioCharacter->inventory.drop_hypo; i++)
	{
		WPrPowerup_Drop(WPcPowerup_Hypo, 1, ioCharacter, UUcFalse);
	}

	if (ioCharacter->inventory.drop_shield) {
		// drop a full-strength shield
		WPrPowerup_Drop(WPcPowerup_ShieldBelt, WPcMaxShields, ioCharacter, UUcFalse);
	}

	if (ioCharacter->inventory.drop_invisibility) {
		// drop a full-strength invisibility
		WPrPowerup_Drop(WPcPowerup_Invisibility, WPcMaxInvisibility, ioCharacter, UUcFalse);
	}

	if (((ioCharacter->flags & ONcCharacterFlag_NoAutoDrop) == 0) && (ioCharacter->charType != ONcChar_Player)) {
		/*
		 * automatically drop certain items from character's inventory
		 */

		if (ioCharacter->inventory.shieldRemaining > 0) {
			// drop their existing partial-strength shield
			WPrPowerup_Drop(WPcPowerup_ShieldBelt, ioCharacter->inventory.shieldRemaining, ioCharacter, UUcFalse);
		}

		if (ioCharacter->inventory.invisibilityRemaining > 0) {
			// drop their existing partial-strength invisibility
			WPrPowerup_Drop(WPcPowerup_Invisibility, ioCharacter->inventory.invisibilityRemaining, ioCharacter, UUcFalse);
		}

		if (ioCharacter->inventory.has_lsi) {
			// drop their LSI
			WPrPowerup_Drop(WPcPowerup_LSI, 1, ioCharacter, UUcFalse);
		}
	}
}



void ONrCharacter_DropWeapon(ONtCharacter *ioCharacter, UUtBool inWantKnockAway, UUtUns16 inSlotNum, UUtBool inStoreOnly)
{
	ONtActiveCharacter *active_character;

	UUmAssert(ioCharacter->flags & ONcCharacterFlag_InUse);

	if (inSlotNum == WPcAllSlots)
	{
		WPrSlots_DropAll(&ioCharacter->inventory, inWantKnockAway, inStoreOnly);
	}
	else
	{
		WPrSlot_Drop(&ioCharacter->inventory, inSlotNum, inWantKnockAway, inStoreOnly);
	}

	ioCharacter->inventory.weapons[0] = NULL;		// Backward compatibility

	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character != NULL) {
		if (active_character->extraBody != NULL) {
			active_character->extraBody->parts[0].geometry = NULL;
		}

		active_character->pleaseFire = UUcFalse;
		active_character->pleaseFireAlternate = UUcFalse;
		active_character->pleaseAutoFire = UUcFalse;
		active_character->pleaseContinueFiring = UUcFalse;

		ONrCharacter_SetWeaponVarient(ioCharacter, active_character);
	}

	return;
}

void ONrCharacter_NotifyReleaseWeapon(ONtCharacter *ioCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);

	if (active_character != NULL) {
		if (active_character->extraBody != NULL) {
			active_character->extraBody->parts[0].geometry = NULL;
		}
	}
}

void
ONrCharacter_UseWeapon(
	ONtCharacter			*ioCharacter,
	WPtWeapon				*inWeapon)
{
	ONtActiveCharacter *active_character;

	UUmAssertWritePtr(ioCharacter, sizeof(*ioCharacter));

	if (NULL == inWeapon)
	{
		// no weapon was specified so store the current weapon
		ONrCharacter_DropWeapon(ioCharacter, UUcFalse, WPcPrimarySlot, UUcTrue);
	}
	else
	{
		M3tGeometry *marker;
		WPtWeaponClass *weapon_class;

		UUmAssert(inWeapon);
		weapon_class = WPrGetClass(inWeapon);

		// don't arm dead characters
		if (ioCharacter->flags & ONcCharacterFlag_Dead_1_Animating) return;

		marker = NULL;

		active_character = ONrGetActiveCharacter(ioCharacter);

		// set the character's weapon
		ioCharacter->inventory.weapons[WPcPrimarySlot] = inWeapon;

		if (active_character != NULL) {
			active_character->extraBody->parts[0].geometry = WPrGetClass(inWeapon)->geometry;

			// set the characters weapon varients for the new state
			ONrCharacter_SetWeaponVarient(ioCharacter, active_character);

			active_character->matricies[ONcWeapon_Index] = MUgIdentityMatrix;
		}

		// tell the weapon it's been picked up
		WPrAssign(inWeapon,ioCharacter);

		if (ioCharacter == ONrGameState_GetPlayerCharacter()) {
			if (!ONrPersist_WeaponUnlocked(weapon_class)) {
				// tell the UI that this is a new weapon and that it needs to be displayed
				ONrInGameUI_NewWeapon(weapon_class);
				ONrPersist_UnlockWeapon(weapon_class);
			}
		}
	}

	return;
}

static TRtAnimType AdjustForCrouch(TRtAnimType type)
{
	TRtAnimType result;

	switch(type)
	{
		case ONcAnimType_Hit_Head:
		case ONcAnimType_Hit_Body:
		case ONcAnimType_Hit_Foot:
			result = ONcAnimType_Hit_Crouch;
		break;

		case ONcAnimType_Knockdown_Head:
		case ONcAnimType_Knockdown_Body:
		case ONcAnimType_Knockdown_Foot:
			result = ONcAnimType_Knockdown_Crouch;
		break;

		default:
			result = type;
	}

	return result;
}

static TRtAnimType AdjustForFallen(TRtAnimType type)
{
	return ONcAnimType_Hit_Fallen;
}

static TRtAnimType AdjustForResistance(TRtAnimType inType)
{
	TRtAnimType result;

	switch(inType)
	{
		case ONcAnimType_Hit_Head:
		case ONcAnimType_Hit_Body:
		case ONcAnimType_Hit_Foot:
		case ONcAnimType_Hit_Jewels:
		case ONcAnimType_Hit_Foot_Ouch:
		case ONcAnimType_Hit_Crouch:
		case ONcAnimType_Stun:
			// note: this is not a real animation to play but will be
			// converted into an overlay instead
			result = ONcAnimType_Hit_Overlay;
		break;

		case ONcAnimType_Stagger:
			result = ONcAnimType_Hit_Body;
		break;

		case ONcAnimType_Knockdown_Head:
		case ONcAnimType_Knockdown_Body:
		case ONcAnimType_Knockdown_Foot:
		case ONcAnimType_Knockdown_Crouch:
		case ONcAnimType_Blownup:
			result = ONcAnimType_Stagger;
		break;

		case ONcAnimType_Hit_Fallen:
		default:
			result = inType;
		break;
	}

	return result;
}

static TRtAnimType AdjustForBehind(TRtAnimType inType)
{
	TRtAnimType result;

	switch(inType)
	{
		case ONcAnimType_Hit_Head:
			result = ONcAnimType_Hit_Head_Behind;
		break;

		case ONcAnimType_Hit_Body:
			result = ONcAnimType_Hit_Body_Behind;
		break;

		case ONcAnimType_Hit_Jewels:
		case ONcAnimType_Hit_Foot_Ouch:
		case ONcAnimType_Hit_Foot:
			result = ONcAnimType_Hit_Body_Behind;
		break;

		case ONcAnimType_Knockdown_Head:
			result = ONcAnimType_Knockdown_Head_Behind;
		break;

		case ONcAnimType_Knockdown_Body:
			result = ONcAnimType_Knockdown_Body_Behind;
		break;

		case ONcAnimType_Knockdown_Foot:
			result = ONcAnimType_Knockdown_Foot_Behind;
		break;

		case ONcAnimType_Hit_Crouch:
			result = ONcAnimType_Hit_Crouch_Behind;
		break;

		case ONcAnimType_Knockdown_Crouch:
			result = ONcAnimType_Knockdown_Crouch_Behind;
		break;

		case ONcAnimType_Blownup:
			result = ONcAnimType_Blownup_Behind;
		break;

		case ONcAnimType_Stagger:
			result = ONcAnimType_Stagger_Behind;
		break;

		case ONcAnimType_Stun:
		case ONcAnimType_Hit_Fallen:
			result = inType;
		break;

		default:
			COrConsole_Printf("failed to adjust for behind on %s", ONrAnimTypeToString(inType));
			result = inType;
		break;
	}

	return result;
}

static UUtBool DamageAnimTypeHasFacing(TRtAnimType inType)
{
	UUtBool has_facing;

	switch(inType)
	{
		case ONcAnimType_Stun:
		case ONcAnimType_Hit_Fallen:
			has_facing = UUcFalse;
		break;

		default:
			has_facing = UUcTrue;
		break;
	}

	return has_facing;
}

static UUtBool AnimIsBehind(TRtAnimType inType)
{
	UUtBool is_behind;

	switch(inType)
	{
		case ONcAnimType_Hit_Head_Behind:
		case ONcAnimType_Hit_Body_Behind:
		case ONcAnimType_Knockdown_Head_Behind:
		case ONcAnimType_Knockdown_Body_Behind:
		case ONcAnimType_Knockdown_Foot_Behind:
		case ONcAnimType_Hit_Crouch_Behind:
		case ONcAnimType_Knockdown_Crouch_Behind:
		case ONcAnimType_Stagger_Behind:
		case ONcAnimType_Blownup_Behind:
			is_behind = UUcTrue;
		break;

		default:
			is_behind = UUcFalse;
		break;
	}

	return is_behind;
}

UUtBool ONrCharacter_IsKnockdownResistant(ONtCharacter *ioCharacter)
{
	if (ioCharacter->characterClass->knockdownResistant)
		return UUcTrue;

	return ((ioCharacter->charType == ONcChar_Player) && ONgPlayerKnockdownResistant);
}

TRtAnimType ONrCharacter_CalculateAnimType(ONtCharacter *attacker, ONtCharacter *defender,
										   ONtActiveCharacter *defender_active, UUtBool inApplyResistance,
										   TRtAnimType inAnimType)
{
	TRtAnimType result = inAnimType;
	const TRtAnimation *curAnimation = defender_active->animation;
	TRtAnimState toState = TRrAnimation_GetTo(curAnimation);
	UUtBool knockdown_resistant = (inApplyResistance && ONrCharacter_IsKnockdownResistant(defender));

	if (ONrAnimType_IsVictimType(inAnimType)) {
		// victim animations are upgraded to knockdowns in the air
		if (defender_active->inAirControl.numFramesInAir > 0) {
			result = ONrAnimType_UpgradeToKnockdown(result);
			goto exit;
		}
	}

	switch(toState)
	{
		case ONcAnimState_Crouch:
			result = AdjustForCrouch(result);
		break;

		case ONcAnimState_Fallen_Front:
		case ONcAnimState_Fallen_Back:
			result = AdjustForFallen(result);
		break;
	}

	if (knockdown_resistant) {
		result = AdjustForResistance(result);
	}

exit:
	return result;
}

static TRtAnimType ONrCharacter_GetSimpleAnimType(TRtAnimType inAnimType)
{
	TRtAnimType result_type;

	switch(inAnimType)
	{
		case ONcAnimType_Hit_Head:
		case ONcAnimType_Hit_Body:
		case ONcAnimType_Hit_Foot:
		case ONcAnimType_Hit_Crouch:
		case ONcAnimType_Hit_Fallen:
		case ONcAnimType_Hit_Foot_Ouch:
		case ONcAnimType_Hit_Jewels:
			result_type = ONcAnimType_Hit_Body;
		break;

		case ONcAnimType_Hit_Head_Behind:
		case ONcAnimType_Hit_Body_Behind:
		case ONcAnimType_Hit_Foot_Behind:
		case ONcAnimType_Hit_Crouch_Behind:
			result_type = ONcAnimType_Hit_Body_Behind;
		break;

		case ONcAnimType_Knockdown_Head:
		case ONcAnimType_Knockdown_Body:
		case ONcAnimType_Knockdown_Foot:
		case ONcAnimType_Knockdown_Crouch:
			result_type = ONcAnimType_Knockdown_Body;
		break;

		case ONcAnimType_Knockdown_Crouch_Behind:
		case ONcAnimType_Knockdown_Head_Behind:
		case ONcAnimType_Knockdown_Body_Behind:
		case ONcAnimType_Knockdown_Foot_Behind:
			result_type = ONcAnimType_Knockdown_Body_Behind;
		break;

		default:
			result_type = inAnimType;
	}

	return result_type;
}

void ONrCharacter_Knockdown(ONtCharacter *attacker, ONtCharacter *defender, UUtUns16 inAnimType)
{
	ONtCharacterClass		*characterClass = defender->characterClass;
	TRtAnimationCollection	*collection = characterClass->animations;
	const TRtAnimation *animation;
	TRtAnimType newAnimType;
	ONtActiveCharacter *defender_activechar;


	if (defender->flags & ONcCharacterFlag_Dead) {
		ONrAnimType_UpgradeToKnockdown(inAnimType);
	}

	if (NULL != attacker) {
		UUmAssertReadPtr(attacker, sizeof(*attacker));
	}

	UUmAssertReadPtr(defender, sizeof(*defender));
	defender_activechar = ONrForceActiveCharacter(defender);
	if (defender_activechar == NULL) {
		return;
	}

	ONrCharacter_FightMode(defender);

	defender_activechar->lastAttack = ONcAnimType_None;
	defender_activechar->lastAttackTime = 0;
	defender_activechar->animationLockFrames = 0;	// stop looping an anim

	newAnimType = ONrCharacter_CalculateAnimType(attacker, defender, defender_activechar, UUcFalse, inAnimType);

	animation = TRrCollection_Lookup(collection, newAnimType, defender_activechar->nextAnimState, defender_activechar->animVarient);

	if (NULL == animation) {
		animation = TRrCollection_Lookup(collection, newAnimType, ONcAnimState_Standing, defender_activechar->animVarient);
	}
	COmAssert(NULL != animation);

	if (NULL == animation) {
		animation = TRrCollection_Lookup(collection, inAnimType, defender_activechar->nextAnimState, defender_activechar->animVarient);
	}

	if (NULL == animation) {
		TRtAnimType simple_anim_type = ONrCharacter_GetSimpleAnimType(inAnimType);

		animation = TRrCollection_Lookup(collection, simple_anim_type, defender_activechar->nextAnimState, defender_activechar->animVarient);

		if (NULL == animation) {
			animation = TRrCollection_Lookup(collection, inAnimType, ONcAnimState_Standing, defender_activechar->animVarient);
		}

		if (NULL == animation) {
			animation = TRrCollection_Lookup(collection, simple_anim_type, ONcAnimState_Standing, defender_activechar->animVarient);
		}
	}

	if (NULL == animation) {
		TRtAnimType falling_test = ONcAnimState_None;
		UUtBool try_falling;

		switch(defender_activechar->nextAnimState)
		{
			case ONcAnimState_Flying:			falling_test = ONcAnimState_Falling; break;
			case ONcAnimState_Flying_Forward:	falling_test = ONcAnimState_Falling_Forward; break;
			case ONcAnimState_Flying_Back:		falling_test = ONcAnimState_Falling_Back; break;
			case ONcAnimState_Flying_Left:		falling_test = ONcAnimState_Falling_Left; break;
			case ONcAnimState_Flying_Right:		falling_test = ONcAnimState_Falling_Right; break;

			case ONcAnimState_Falling:
			case ONcAnimState_Falling_Forward:
			case ONcAnimState_Falling_Back:
			case ONcAnimState_Falling_Left:
			case ONcAnimState_Falling_Right:
			break;


			default:
				try_falling = UUcFalse;
		}

		if (falling_test != ONcAnimState_None) {
			animation = TRrCollection_Lookup(collection, inAnimType, falling_test, defender_activechar->animVarient);
		}

		if ((NULL == animation) && (try_falling)) {
			animation = TRrCollection_Lookup(collection, inAnimType, ONcAnimState_Falling, defender_activechar->animVarient);
		}
	}

	if (NULL == animation) {
		COrConsole_Printf("### failed to find knockdown animation for %s", ONrAnimTypeToString(inAnimType));
	}
	else {
		if ((ONrAnimType_IsKnockdown(inAnimType)) && (!ONrAnimState_IsFallen(defender_activechar->nextAnimState)) &&
			(defender->charType == ONcChar_AI2)) {
			// the AI has just been knocked down
			AI2rNotifyKnockdown(defender);
		}

		ONrOverlay_Initialize(defender_activechar->overlay + ONcOverlayIndex_InventoryManagement);

		ONiCharacter_OldAnimationHook(defender, defender_activechar, ONcIgnoreFinalRotation, animation);
		ONrCharacter_Stitch(defender, defender_activechar, defender_activechar->nextAnimState, animation, ONcNumStitchingFrames);
		ONrCharacter_NewAnimationHook(defender, defender_activechar);
		defender_activechar->nextAnimType = ONcAnimType_Stand;
	}

	return;
}

void ONrCharacter_Block(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	const TRtAnimation *blockAnim;

	UUmAssertReadPtr(ioCharacter, sizeof(*ioCharacter));

	ONrCharacter_FightMode(ioCharacter);

	ioActiveCharacter->animationLockFrames = 0;	// stop looping an anim

	if (ioActiveCharacter->curAnimType != ONcAnimType_Block) {
		blockAnim = ONrCharacter_DoAnimation(ioCharacter, ioActiveCharacter, ONcAnimPriority_Force, ONcAnimType_Block);

		if (NULL == blockAnim) {
			blockAnim = ONrCharacter_DoAnimation(ioCharacter, ioActiveCharacter, ONcAnimPriority_Force, ONcAnimType_Block);
		}
	}

	return;
}

UUtError ONrLookupString(const char *inString, const TMtStringArray *inMapping, UUtUns16 *result)
{
	UUtError error = UUcError_None;
	UUtUns16 itr;

	UUmAssert(NULL != inMapping);

	for(itr = 0; itr < inMapping->numStrings; itr++) {
		if (0 == strcmp(inString, inMapping->string[itr]->string)) {
			break;
		}
	}

	if (itr >= inMapping->numStrings) {
		error = UUcError_Generic;
	}

	*result = itr;

	return error;
}

static TRtAnimType ONrCharacter_NextScreenType(TRtAnimType inType)
{
	TRtAnimType newType;

	switch(inType)
	{
		case ONcAnimType_Run_Sidestep_Left_Start:
			newType = ONcAnimType_Run_Sidestep_Left;
		break;

		case ONcAnimType_Run_Sidestep_Right_Start:
			newType = ONcAnimType_Run_Sidestep_Right;
		break;

		case ONcAnimType_Run_Backwards_Start:
			newType = ONcAnimType_Run_Backwards;
		break;

		case ONcAnimType_Walk_Backwards:
		case ONcAnimType_Walk_Sidestep_Left:
		case ONcAnimType_Walk_Sidestep_Right:
			newType = ONcAnimType_Walk;
		break;

		case ONcAnimType_Run_Start:
		case ONcAnimType_Run_Backwards:
		case ONcAnimType_Run_Sidestep_Left:
		case ONcAnimType_Run_Sidestep_Right:
			newType = ONcAnimType_Run;
		break;

		case ONcAnimType_Standing_Turn_Left:
		case ONcAnimType_Standing_Turn_Right:
		case ONcAnimType_Walk:
		case ONcAnimType_Run:
			newType = ONcAnimType_Stand;
		break;

		case ONcAnimType_Crouch_Walk_Backwards:
		case ONcAnimType_Crouch_Walk_Sidestep_Left:
		case ONcAnimType_Crouch_Walk_Sidestep_Right:
			newType = ONcAnimType_Crouch_Walk;
		break;

		case ONcAnimType_Crouch_Run_Backwards:
		case ONcAnimType_Crouch_Run_Sidestep_Left:
		case ONcAnimType_Crouch_Run_Sidestep_Right:
			newType = ONcAnimType_Crouch_Run;
		break;

		case ONcAnimType_Crouch_Turn_Left:
		case ONcAnimType_Crouch_Turn_Right:
		case ONcAnimType_Crouch_Run:
		case ONcAnimType_Crouch_Walk:
			newType = ONcAnimType_Crouch;
		break;

		case ONcAnimType_Anything:
			newType = ONcAnimType_None;
		break;

		default:
			newType = ONcAnimType_Anything;
		break;
	}

	return newType;
}

// if this function ever shows up on the charts at all, it would be easy
// to make it cache things

const TRtAimingScreen *ONrCharacter_GetAimingScreen(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	const TRtScreenCollection *aiming_screen_collection = ioCharacter->characterClass->aimingScreens;
	const TRtAimingScreen *result_aiming_screen = NULL;
	TRtAnimType type = ioActiveCharacter->animation->aimingType;
	TRtAnimVarient varient = TRrAnimation_GetVarient(ioActiveCharacter->animation);

	if (ioCharacter->flags & ONcCharacterFlag_Dead) {
		UUmAssert(NULL == result_aiming_screen);
		goto exit;
	}

	while (TRcAnimType_None != type)
	{
		const TRtAimingScreen *new_aiming_screen;

		new_aiming_screen = TRrAimingScreen_Lookup(
			aiming_screen_collection,
			type,
			varient);

		if (NULL != new_aiming_screen) {
			if (NULL == result_aiming_screen) {
				result_aiming_screen = new_aiming_screen;
			}
			else {
				TRtAnimVarient old_varient = TRrAnimation_GetVarient(TRrAimingScreen_GetAnimation(result_aiming_screen));
				TRtAnimVarient new_varient = TRrAnimation_GetVarient(TRrAimingScreen_GetAnimation(new_aiming_screen));

				if (new_varient > old_varient) {
					result_aiming_screen = new_aiming_screen;
				}
			}
		}

		type = ONrCharacter_NextScreenType(type);
	}

exit:
	return result_aiming_screen;
}

void ONrCharacter_BuildQuaternions(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	ONtCharacterClass		*characterClass	= ioCharacter->characterClass;
	TRtExtraBody			*extraBody		= ioActiveCharacter->extraBody;
	const TRtAimingScreen	*aimingScreen;
	float aimUD, aimLR;
	UUtUns16 numParts = ONrCharacter_GetNumParts(ioCharacter);
	UUtBool overlay_says_dont_aim = UUcFalse;

	// build the animation part of the quaternions
	if (!ioActiveCharacter->stitch.stitching) {
		TRrQuatArray_SetAnimation(ioActiveCharacter->animation, ioActiveCharacter->animFrame, ioActiveCharacter->curAnimOrientations);
	}
	else {
		float interpolateAmt;

		interpolateAmt = 1.f - ((float)ioActiveCharacter->stitch.itr) / ((float) ioActiveCharacter->stitch.count);

		TRrQuatArray_SetAnimation(ioActiveCharacter->animation, ioActiveCharacter->animFrame, TRgQuatArrayA);
		UUrMemory_MoveFast(ioActiveCharacter->stitch.positions, TRgQuatArrayB, sizeof(M3tQuaternion) * ONcNumCharacterParts);

		TRrQuatArray_Lerp(numParts, interpolateAmt, TRgQuatArrayA, TRgQuatArrayB, ioActiveCharacter->curAnimOrientations);
	}

	UUrMemory_MoveFast(
		ioActiveCharacter->curAnimOrientations,
		ioActiveCharacter->curOrientations,
		sizeof(M3tQuaternion) * numParts);

	{
		UUtUns32 overlay_index;

		for(overlay_index = 0; overlay_index < ONcOverlayIndex_Count; overlay_index++) {
			const ONtOverlay *overlay = ioActiveCharacter->overlay + overlay_index;
			UUtUns32 number_of_frames_to_ignore_flags = 0;

			if (overlay->animation != NULL) {
				ONrCharacter_ApplyOverlay(ioCharacter, ioActiveCharacter, overlay);

				if ((overlay->frame + number_of_frames_to_ignore_flags) < (overlay->animation->duration)) {
					overlay_says_dont_aim = overlay_says_dont_aim || TRrAnimation_TestFlag(overlay->animation, ONcAnimFlag_DontAim);
				}
			}
		}
	}

	// do aiming if we have a screen to aim with
	aimingScreen = NULL;
	if (TRrAnimation_TestFlag(ioActiveCharacter->animation, ONcAnimFlag_DoAim) && !overlay_says_dont_aim) {
		if (ONcAnimType_Block != ioActiveCharacter->animation->type) {
			if (!ONrAnimType_IsVictimType(ioActiveCharacter->animation->type)) {
				aimingScreen = ONrCharacter_GetAimingScreen(ioCharacter, ioActiveCharacter);
			}
		}
	}

	if (aimingScreen != ioActiveCharacter->aimingScreen) {
		ioActiveCharacter->oldAimingScreen = ioActiveCharacter->aimingScreen;
		ioActiveCharacter->oldAimingScreenTime = ONgGameState->gameTime;
		ioActiveCharacter->aimingScreen = aimingScreen;

		if (ONgShowAimingScreen) {
			if (NULL != aimingScreen) {
				COrConsole_Printf("%s", aimingScreen->animation->instanceName);
			}
			else {
				COrConsole_Printf("no aiming screen");
			}
		}
	}

	aimLR = ioActiveCharacter->aimingLR;
	aimUD = 0.f;

	aimLR += ONrCharacter_GetDesiredFacingOffset(ioCharacter);
	aimLR -= ioCharacter->facingModifier;
	aimUD += ioActiveCharacter->aimingUD;
	aimUD += ioCharacter->recoil;
	aimUD = UUmPin(aimUD, -(M3c2Pi / 4.f), (M3c2Pi / 4.f));

	aimLR += ioActiveCharacter->autoAimAdjustmentLR;
	aimUD += ioActiveCharacter->autoAimAdjustmentUD;

	if ((aimLR != 0.f) || (aimUD != 0.f)) {
		UUtUns32 num_interp_frames = 8;

		if ((ONgGameState->gameTime - ioActiveCharacter->oldAimingScreenTime) > num_interp_frames) {
			ioActiveCharacter->oldAimingScreen = aimingScreen;
		}

		if ((NULL == aimingScreen) && (NULL == ioActiveCharacter->oldAimingScreen)) {
			// do nothing
		}
		else if (aimingScreen == ioActiveCharacter->oldAimingScreen) {
			TRrAimingScreen_Apply(
				aimingScreen,
				aimLR,
				aimUD,
				ioActiveCharacter->curOrientations,
				ioActiveCharacter->curOrientations,
				numParts);
		}
		else {
			float distance = ((float) (ONgGameState->gameTime - ioActiveCharacter->oldAimingScreenTime)) / ((float) num_interp_frames);

			TRrAimingScreen_Apply_Lerp(
				ioActiveCharacter->oldAimingScreen,
				aimingScreen,
				distance,
				aimLR,
				aimUD,
				ioActiveCharacter->curOrientations,
				ioActiveCharacter->curOrientations,
				numParts);
		}
	}

	ioActiveCharacter->aimingScreen = aimingScreen;

	if (0)
	{
		const char *anim_name = TRrAnimation_GetName(ioActiveCharacter->animation);
		const char *screen_name = ioActiveCharacter->aimingScreen != NULL ? TRrAnimation_GetName(ioActiveCharacter->aimingScreen->animation) : "";

		COrConsole_Printf("%s %s", anim_name, screen_name);
	}

	return;
}

void ONrCharacter_BuildMatriciesAll(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	ONrCharacter_BuildMatricies(ioCharacter, ioActiveCharacter, 0xffffffff);
	ONrCharacter_BuildBoundingVolumes(ioCharacter, ioActiveCharacter);
	ONrCharacter_UpdateSphereTree(ioCharacter, ioActiveCharacter);

	return;
}

static void ONrCharacter_DoExactAiming(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUtUns32 num_exact_aiming_interpolation = 8;
	float amount_exact_aiming = 1.f;
	float pinned_lr;
	float pinned_ud;

	UUtInt32 index;
	M3tVector3D translation;
	MUtEuler aimEuler;
	M3tMatrix4x3 aimMatrix;
	M3tMatrix4x3 goal_matrix;

	if (!ioActiveCharacter->oldExactAiming)
	{
		UUtUns32 num_frames_exact_aiming = ONgGameState->gameTime - ioActiveCharacter->oldExactAimingTime;

		if (num_frames_exact_aiming > num_exact_aiming_interpolation) {
			ioActiveCharacter->oldExactAiming = UUcTrue;
		}
		else {
			amount_exact_aiming = ((float) (num_frames_exact_aiming)) / ((float) num_exact_aiming_interpolation);
		}
	}

	if (ioCharacter->characterClass->leftHanded) {
		index = ONcLHand_Index;
	}
	else {
		index = ONcRHand_Index;
	}

	pinned_lr = ioActiveCharacter->aimingLR + ONrCharacter_GetDesiredFacingOffset(ioCharacter) - ioCharacter->facingModifier;
	pinned_ud = ioActiveCharacter->aimingUD + ioCharacter->recoil;

	pinned_lr += ioActiveCharacter->autoAimAdjustmentLR;
	pinned_ud += ioActiveCharacter->autoAimAdjustmentUD;

	TRrAimingScreen_Clip(ioActiveCharacter->aimingScreen, &pinned_lr, &pinned_ud);

	pinned_lr += ioCharacter->facing;

	UUmTrig_Clip(pinned_lr);
	UUmTrig_Clip(pinned_ud);

	index = ONcWeapon_Index;

	translation = MUrMatrix_GetTranslation(ioActiveCharacter->matricies + index);

	aimEuler = Euler_(
		0,
		pinned_lr,
		pinned_ud,
		MUcEulerOrderZYXr);

	MUrEulerToMatrix(&aimEuler, &aimMatrix);


	goal_matrix = MUgIdentityMatrix;
	MUrMatrixStack_Translate(&goal_matrix, &translation);
	MUrMatrixStack_RotateX(&goal_matrix, M3c2Pi - M3cHalfPi);

	{
		float lr;
		float ud;


		lr = pinned_lr - M3cHalfPi;
		UUmTrig_Clip(lr);

		ud = M3c2Pi - pinned_ud;

		MUrMatrixStack_RotateZ(&goal_matrix, lr);
		MUrMatrixStack_RotateY(&goal_matrix, ud);
	}

	if (1.f == amount_exact_aiming) {
		ioActiveCharacter->matricies[index] = goal_matrix;
	}
	else {
		MUrMatrix_Lerp(ioActiveCharacter->matricies + index, &goal_matrix, amount_exact_aiming, ioActiveCharacter->matricies + index);
	}
}

void ONrCharacter_BuildMatricies(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter, UUtUns32 mask)
{
	ONtCharacterClass	*characterClass	= ioCharacter->characterClass;
	TRtBody				*body			= ONrCharacter_GetBody(ioCharacter, TRcBody_SuperHigh);
	TRtExtraBody		*extraBody		= ioActiveCharacter->extraBody;
	M3tPoint3D			location;
	M3tMatrix4x3		chrBaseMatrix;
	UUtBool				running_environment_animation = ((ioActiveCharacter->physics != NULL) &&
														(ioActiveCharacter->physics->animContext.animation != NULL));

	UUmAssertWritePtr(ioCharacter, sizeof(*ioCharacter));
	UUmAssertWritePtr(ioActiveCharacter, sizeof(*ioActiveCharacter));
	UUmAssert((ioCharacter->heightThisFrame > -128.f) && (ioCharacter->heightThisFrame < 128.f));

	location = ioCharacter->location;

	if (running_environment_animation) {
		OBrAnim_Matrix(&ioActiveCharacter->physics->animContext, &chrBaseMatrix);

		if (0 == (ioActiveCharacter->physics->animContext.animContextFlags & OBcAnimContextFlags_NoRotation)) {
			InverseMaxMatrix(&chrBaseMatrix);
		}
	}
	else {
		// speed up later
		chrBaseMatrix = MUgIdentityMatrix;
		MUrMatrixStack_Translate(&chrBaseMatrix, &location);
		MUrMatrixStack_RotateY(&chrBaseMatrix, ioCharacter->facing);
	}

	if (ONgAllCharactersSameHeight) {
	}
	else if ((ioCharacter->charType == ONcChar_Player) && (ONgMiniMe)) {
		MUrMatrixStack_Scale(&chrBaseMatrix, ONgMiniMe_Amount);
	}
	else if (ioCharacter->scale != 1.f) {
		MUrMatrixStack_Scale(&chrBaseMatrix, ioCharacter->scale);
	}

	if (!running_environment_animation)
	{
		M3tPoint3D height;

		height.x = 0.f;
		height.y = ioCharacter->heightThisFrame;
		height.z = 0.f;

		MUrMatrixStack_Translate(&chrBaseMatrix, &height);
	}

	TRrBody_BuildMatricies(body, extraBody, ioActiveCharacter->curOrientations, &chrBaseMatrix, mask, ioActiveCharacter->matricies);

	{
		UUtBool did_exact_aiming = UUcFalse;

		if ((ioActiveCharacter->isAiming) && (ioCharacter->inventory.weapons[0] != NULL) && (ioActiveCharacter->aimingScreen != NULL))
		{
			const TRtAnimation *aiming_screen_animation = TRrAimingScreen_GetAnimation(ioActiveCharacter->aimingScreen);
			TRtAnimVarient aiming_screen_varient = TRrAnimation_GetVarient(aiming_screen_animation);
			UUtBool varient_ok = (aiming_screen_varient & ONcAnimVarientMask_Any_Weapon) > 0;

			if (varient_ok) {
				ONrCharacter_DoExactAiming(ioCharacter, ioActiveCharacter);

				did_exact_aiming = UUcTrue;
			}
		}

		if (!did_exact_aiming) {
			ioActiveCharacter->oldExactAimingTime = ONgGameState->gameTime;
			ioActiveCharacter->oldExactAiming = UUcFalse;
		}
	}

#if 0
	// CB: I am commenting out all of this IK code (rather than just the actual call to
	// inverse_kinematics_adjust_matrices) because it's all wasted cycles
	if (ioCharacter->inventory.weapons[0] != NULL) {
		if (ioActiveCharacter->aimingScreen != NULL) {
			const TRtAnimation *aiming_screen_animation = TRrAimingScreen_GetAnimation(ioActiveCharacter->aimingScreen);
			TRtAnimVarient aiming_screen_varient = TRrAnimation_GetVarient(aiming_screen_animation);

			if (aiming_screen_varient & ONcAnimVarientMask_Any_Rifle) {
				WPtWeaponClass *weaponClass = WPrGetClass(ioCharacter->inventory.weapons[0]);
				M3tPoint3D hand_forward;
				M3tPoint3D hand_up;
				M3tPoint3D hand_location;
				const M3tMatrix4x3 *weapon_matrix = ioActiveCharacter->matricies + ONcWeapon_Index;

				MUrMatrix_MultiplyPoint(&weaponClass->secondHandVector, weapon_matrix, &hand_forward);
				MUrMatrix_MultiplyPoint(&weaponClass->secondHandUpVector, weapon_matrix, &hand_up);
				MUrMatrix_MultiplyPoint(&weaponClass->secondHandPosition, weapon_matrix, &hand_location);

				MUmVector_Subtract(hand_forward, hand_forward, hand_location);
				MUmVector_Normalize(hand_forward);

				MUmVector_Subtract(hand_up, hand_up, hand_location);
				MUmVector_Normalize(hand_up);

				hand_forward = ioActiveCharacter->aimingVector;

				hand_up.x = 0.f;
				hand_up.y = 1.f;
				hand_up.z = 0.f;

				inverse_kinematics_adjust_matrices(
					&hand_location,
					&hand_forward,
					&hand_up,
					ioActiveCharacter->matricies + ONcLArm1_Index,
					ioActiveCharacter->matricies + ONcLArm2_Index,
					ioActiveCharacter->matricies + ONcLHand_Index);
			}
		}
	}
#endif

	if (ONgBigHead) {
		MUrMatrixStack_Scale(ioActiveCharacter->matricies + ONcHead_Index, ONgBigHeadAmount);
	}

	if (0)
	{
		UUtInt32 big_part;

		for(big_part = 0; big_part < 18; big_part++)
		{
			UUtUns32 damage;

			damage = TRrAnimation_GetDamage(ioActiveCharacter->animation, ioActiveCharacter->animFrame, big_part);

			if (damage > 0.f) {
				float big_scale = 1.f + (damage * 0.03f);

				MUrMatrixStack_Scale(ioActiveCharacter->matricies + big_part, big_scale);
			}
		}
	}


	return;
}

void ONrCharacter_BuildApproximateBoundingVolumes(ONtCharacter *ioCharacter)
{
	M3tBoundingSphere sphere;

	MUmVector_Copy(sphere.center, ioCharacter->actual_position);
	sphere.radius = ONrCharacter_GetLeafSphereRadius(ioCharacter);

	MUrMinMaxBBox_CreateFromSphere(NULL, &sphere, &ioCharacter->boundingBox);

	sphere.center.y += ONcCharacterHeight;
	MUrMinMaxBBox_ExpandBySphere(NULL, &sphere, &ioCharacter->boundingBox);
}

void ONrCharacter_BuildBoundingVolumes(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	ONtCharacterClass	*characterClass	= ioCharacter->characterClass;
	TRtBody				*body			= ONrCharacter_GetBody(ioCharacter, TRcBody_SuperHigh);
	UUtUns16			itr;
	float				part_radius;
	M3tBoundingSphere	curSphere;

	// bounding volume initially encompasses the pelvis
	ioActiveCharacter->boundingSphere = body->geometries[0]->pointArray->boundingSphere;
	MUrMatrix_MultiplyPoints(1, &(ioActiveCharacter->matricies[0]), &ioActiveCharacter->boundingSphere.center, &ioActiveCharacter->boundingSphere.center);
	MUrMinMaxBBox_CreateFromSphere(NULL, &ioActiveCharacter->boundingSphere, &ioCharacter->boundingBox);

	for(itr = 1; itr < ONcNumCharacterParts; itr++)
	{
		UUtBool skip;

		// get the body part's location in worldspace
		curSphere = body->geometries[itr]->pointArray->boundingSphere;
		MUrMatrix_MultiplyPoints(1, &ioActiveCharacter->matricies[itr], &curSphere.center, &curSphere.center);

		// expand the collision bounding sphere by this part
		part_radius = MUmVector_GetDistance(ioActiveCharacter->boundingSphere.center, curSphere.center) + curSphere.radius;
		ioActiveCharacter->boundingSphere.radius = UUmMax(ioActiveCharacter->boundingSphere.radius, part_radius);

		// does this part contribute to the trigger / visibility bounding box?
		skip = UUcFalse;
		switch(itr)
		{
			case ONcLArm_Index:
			case ONcLArm1_Index:
			case ONcLArm2_Index:
			case ONcLHand_Index:
			case ONcRArm_Index:
			case ONcRArm1_Index:
			case ONcRArm2_Index:
			case ONcRHand_Index:
				skip = UUcTrue;
			break;
		}

		if (!skip) {
			MUrMinMaxBBox_ExpandBySphere(NULL, &curSphere, &ioCharacter->boundingBox);
		}
	}

	ioCharacter->feetLocation = UUmMax(ioCharacter->boundingBox.minPoint.y, ioCharacter->location.y);

	ioCharacter->boundingBox.maxPoint.y = UUmMax(ioCharacter->boundingBox.maxPoint.y, ioCharacter->boundingBox.minPoint.y + 16.f);

	if (1)
	{
		M3tBoundingSphere bottomSphere;

		bottomSphere.center = ioCharacter->location;
		bottomSphere.radius = 2.f;

		MUrMinMaxBBox_ExpandBySphere(NULL, &bottomSphere, &ioCharacter->boundingBox);
	}



	// COrConsole_Printf("bot %2.2f/%2.2f top %2.2f", ioCharacter->boundingBox.minPoint.y, ioCharacter->location.y, ioCharacter->boundingBox.maxPoint.y);

	return;
}

static const TRtAnimation *ONiAnimation_FindBlock(const ONtCharacter *inCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	TRtAnimationCollection	*collection = inCharacter->characterClass->animations;
	TRtAnimState state = ioActiveCharacter->nextAnimState;
	const TRtAnimation *blockAnim;
	TRtAnimVarient varient = ioActiveCharacter->animVarient | ONcAnimVarientMask_Fight;

	blockAnim = TRrCollection_Lookup(collection, ONcAnimType_Block, ioActiveCharacter->nextAnimState, varient);

	return blockAnim;
}

static TRtAnimType ONiRemapAnimType_For_Direction(TRtAnimType anim_type, ONtCharacter *inAttacker, ONtCharacter *inDefender)
{
	TRtAnimType result = anim_type;
	M3tVector3D toVector;
	float angle;
	float facing = inDefender->facing;
	float backwards_threshold = 90 * M3c2Pi * (1.f/360.f);

	MUmVector_Subtract(toVector, inAttacker->location, inDefender->location);
	angle = MUrATan2(toVector.x, toVector.z);
	UUmTrig_ClipLow(angle);

	// ok angle is now the angle from the defender to the attacker

	angle = angle - facing;
	UUmTrig_ClipLow(angle);

	// ok now the angle is the difference between the defenders facing
	// and the angle from the defender to the attacker

	if (angle > M3cPi) {
		angle -= M3c2Pi;
	}

	// ok, now the angle is -pi to +pi

	if ((float)fabs(angle) > backwards_threshold)
	{
		result = AdjustForBehind(anim_type);
	}

	return result;
}

static UUtUns16
ONiCharacter_GetPrimaryPartFromBits(
	ONtCharacter *inCharacter,
	UUtUns32 inBodyPartBits)
{
	UUtUns16 i;
	TRtBody *body;
	UUtUns16 part;

	if (inBodyPartBits == 0) { return ONcPelvis_Index; }

	part = ONcPelvis_Index;
	body = ONrCharacter_GetBody(inCharacter, TRcBody_SuperHigh);
	UUmAssert(body);

	for (i = body->numParts; i <= body->numParts; i--)
	{
		if (inBodyPartBits & (1 << i))
		{
			part = i;
			break;
		}
	}

	return part;
}

MAtMaterialType
ONrCharacter_GetMaterialType(
	ONtCharacter *inCharacter,
	UUtUns32 inPartIndex,
	UUtBool inMeleeImpact)
{
	MAtMaterialType type;

	if (!inMeleeImpact) {
		if (inCharacter->flags & ONcCharacterFlag_BossShield) {
			return MArMaterialType_GetByName("Super_Shield");
		}
		else if (ONrCharacter_HasSuperShield(inCharacter)) {
			return MArMaterialType_GetByName("Super_Shield");

		} else if (inCharacter->inventory.shieldRemaining > 0) {
			return MArMaterialType_GetByName("Shield");
		}
	}

	if ((inPartIndex >= 0) && (inPartIndex < ONrCharacter_GetNumParts(inCharacter)) &&
		(inCharacter->characterClass->bodyPartMaterials->materials[inPartIndex] != NULL)) {
		type = inCharacter->characterClass->bodyPartMaterials->materials[inPartIndex]->id;
	} else {
		COrConsole_Printf("character %s class %s bodypart %d: failed to find material type",
			inCharacter->player_name, TMrInstance_GetInstanceName(inCharacter->characterClass), inPartIndex);
		type = MArMaterialType_GetByName("Character");
	}

	return type;
}

static MAtMaterialType
ONiCharacter_GetMaterialTypeFromMask(
	ONtCharacter *inCharacter,
	UUtUns32 inBodyPartBits)
{
	UUtUns32 part;

	part = ONiCharacter_GetPrimaryPartFromBits(inCharacter, inBodyPartBits);

	return ONrCharacter_GetMaterialType(inCharacter, part, UUcTrue);
}

static MAtImpactType
ONiCharacter_GetSpecificImpactType(
	ONtCharacter *inCharacter,
	ONtActiveCharacter *inActiveCharacter,
	UUtUns16 *ioModifierType)
{
	UUtUns32 itr;
	ONtCharacterImpactArray *impacts = inCharacter->characterClass->impacts;
	const char *attack_name;

	attack_name = TRrAnimation_GetAttackName(inActiveCharacter->animation);
	if (attack_name[0] == '\0') {
		if (ONgDebugCharacterImpacts) {
			COrConsole_Printf("attack animation %s has no attack-name, no special impact type",
								TMrInstance_GetInstanceName(inActiveCharacter->animation));
		}
		return MAcInvalidID;
	}

	if (ONgDebugCharacterImpacts) {
		COrConsole_Printf("attack animation %s has attack-name %s, looking up in character class %s",
							TMrInstance_GetInstanceName(inActiveCharacter->animation), attack_name,
							TMrInstance_GetInstanceName(inCharacter->characterClass));
	}
	if ((impacts != NULL) && (strcmp(attack_name, "") != 0)) {
		// look to see if our attack animation has a specific impact type
		for (itr = 0; itr < impacts->numImpacts; itr++) {
			if (strcmp(impacts->impact[itr].name, attack_name) == 0) {
				if (ONgDebugCharacterImpacts) {
					COrConsole_Printf("found impact entry: attack-name %s, impact '%s', modifier '%s' (raw: %d, %d)",
										impacts->impact[itr].name, impacts->impact[itr].impact_name,
										impacts->impact[itr].modifier_name, impacts->impact[itr].impact_type,
										impacts->impact[itr].modifier_type);
				}

				if (impacts->impact[itr].impact_type == MAcInvalidID) {
					// the impact name matches, but it's incorrect. give up.
					COrConsole_Printf("### %s attack '%s' has impact type '%s' which was not found",
									TMrInstance_GetInstanceName(inCharacter->characterClass),
									impacts->impact[itr].name, impacts->impact[itr].impact_name);
					return MAcInvalidID;
				} else {
					// we have found an impact type!
					*ioModifierType = impacts->impact[itr].modifier_type;
					return impacts->impact[itr].impact_type;
				}
			}
		}
	}

	COrConsole_Printf("### %s: no attack names matching '%s' found in character class %s!",
						TMrInstance_GetInstanceName(inActiveCharacter->animation), attack_name,
						TMrInstance_GetInstanceName(inCharacter->characterClass));
	return MAcInvalidID;
}

static MAtImpactType
ONiCharacter_GetGenericImpactType(
	ONtCharacter *inCharacter,
	UUtUns32 inBodyPartBits,
	UUtUns32 inHitResult)
{
	MAtImpactType default_type;
	MAtImpact **impacts;
	UUtUns16 part;

	// if there's an error, use impact type "Melee"
	default_type = MArImpactType_GetByName("Melee");
	if (inBodyPartBits == 0) {
		return default_type;
	}

	// look for a generic bodypart impact type
	if (inCharacter->characterClass->bodyPartImpacts == NULL) {
		COrConsole_Printf("### character class has no bodyPartImpacts");
		return default_type;
	}
	switch (inHitResult) {
		case ONcAttack_Hit:
			impacts = inCharacter->characterClass->bodyPartImpacts->hit_impacts;
		break;

		case ONcAttack_Killed:
			impacts = inCharacter->characterClass->bodyPartImpacts->killed_impacts;
		break;

		default:
			impacts = inCharacter->characterClass->bodyPartImpacts->blocked_impacts;
		break;
	}

	part = ONiCharacter_GetPrimaryPartFromBits(inCharacter, inBodyPartBits);
	UUmAssert((part >= 0) && (part < ONcNumCharacterParts));

	if (impacts[part] == NULL) {
		COrConsole_Printf("failed to find impact type from primary part index");
		return default_type;
	}

	return impacts[part]->id;
}

static UUtUns16
ONiCharacter_GetImpactModifier(
	UUtUns32 inDamage)
{
	if (inDamage >= ONcAnimDamage_Heavy)
	{
		return ONcIEModType_HeavyDamage;
	}
	else if (inDamage >= ONcAnimDamage_Medium)
	{
		return ONcIEModType_MediumDamage;
	}
	else if (inDamage > ONcAnimDamage_None)
	{
		return ONcIEModType_LightDamage;
	}
	else
	{
		return ONcIEModType_Any;
	}
}

static float
ONiCharacter_HitAISoundVolumeModifier(
	UUtUns16 inModifier)
{
	switch(inModifier) {
		case ONcIEModType_HeavyDamage:
			return 2.0f;

		case ONcIEModType_MediumDamage:
			return 1.5f;

		case ONcIEModType_LightDamage:
		default:
			return 1.0f;
	}
}

// checks to see whether a character could block theoretical attacks (even if low or high)
UUtBool ONrCharacter_CouldBlock(ONtCharacter *inDefender, ONtCharacter *inAttacker,
								UUtBool *outBlockLow, UUtBool *outBlockHigh)
{
	const TRtAnimation *blockAnim;
	const float blockWidth = ONgBlockAngle * M3cDegToRad;
	float relativeFacing;
	ONtActiveCharacter *defender_activechar;

	if (ONcChar_AI2 == inDefender->charType) {
		if (inDefender->inventory.weapons[0] != NULL) {
			return UUcFalse;
		}
	}
	else {
		if (inDefender->inventory.weapons[0] != NULL) {
			WPtWeaponClass *weapon_class = WPrGetClass(inDefender->inventory.weapons[0]);

			if (weapon_class->flags & WPcWeaponClassFlag_TwoHanded) {
				return UUcFalse;
			}
		}
	}

	if (!ONrCharacter_IsDefensive(inDefender)) {
		return UUcFalse;
	}

	defender_activechar = ONrGetActiveCharacter(inDefender);
	if (defender_activechar == NULL)
		return UUcFalse;

	if (defender_activechar->hitStun > 0)
		return UUcFalse;

	blockAnim = ONiAnimation_FindBlock(inDefender, defender_activechar);
	if (blockAnim == NULL)
		return UUcFalse;

	relativeFacing = ONrCharacter_RelativeAngleToCharacter(inDefender, inAttacker);
	if ((float)fabs(relativeFacing) > blockWidth) {
		// CB: TEMPORARY DEBUGGING
/*		if ((inDefender->charType == ONcChar_AI2) && (inDefender->ai2State.currentGoal == AI2cGoal_Combat) &&
			(inDefender->ai2State.currentState->state.combat.melee.currently_blocking)) {
			COrConsole_Printf("%s: can't block, not facing attacker (%f > %f)",
				inDefender->player_name, UUmFabs(relativeFacing), blockWidth);
		}*/

		return UUcFalse;
	}

	*outBlockLow	= TRrAnimation_TestFlag(blockAnim, ONcAnimFlag_BlockLow);
	*outBlockHigh	= TRrAnimation_TestFlag(blockAnim, ONcAnimFlag_BlockHigh);

	return UUcTrue;
}

static float ONiCharacter_GetAttackPotency(ONtCharacter *ioCharacter, UUtBool *outOmnipotent)
{
	float overhealth, potency;

	if (outOmnipotent != NULL) {
		*outOmnipotent = (ONgPlayerOmnipotent && (ioCharacter->charType == ONcChar_Player));
	}

	if ((!ioCharacter->characterClass->superHypo) || (ioCharacter->hitPoints <= ioCharacter->maxHitPoints)) {
		// character has normal potency
		potency = 1.0f;
	} else {
		overhealth = (float) (ioCharacter->hitPoints - ioCharacter->maxHitPoints) / ioCharacter->maxHitPoints;
		overhealth = UUmMin(overhealth, 1.0f);

		potency = ONgGameSettings->overhypo_base_damage + (ONgGameSettings->overhypo_max_damage - ONgGameSettings->overhypo_base_damage) * overhealth;
	}

	return potency;
}

static void ONiCharacter_BecomeTemporarilyVisible(ONtCharacter *inAttacker, ONtActiveCharacter *inActiveAttacker,
												  ONtCharacter *inDefender, ONtActiveCharacter *inActiveDefender)
{
	// any invisible characters become visible for a short period
	inAttacker->inventory.numInvisibleFrames = 0;
	if ((inActiveAttacker != NULL) && (inActiveAttacker->invis_active)) {
		inActiveAttacker->invis_character_alpha = UUmMax(inActiveAttacker->invis_character_alpha, 0.6f);
	}

	inDefender->inventory.numInvisibleFrames = 0;
	if ((inActiveDefender != NULL) && (inActiveDefender->invis_active)) {
		inActiveDefender->invis_character_alpha = UUmMax(inActiveDefender->invis_character_alpha, 0.6f);
	}
}

static void	HandleAttackMask(
	ONtCharacter *inAttacker,
	ONtCharacter *inDefender,
	ONtActiveCharacter *inActiveAttacker,
	ONtActiveCharacter *inActiveDefender,
	const TRtAttack *inAttack,
	UUtUns32 inAttackIndex,
	UUtUns32 inAttackerParts,
	UUtUns32 inDefenderParts)
{
	M3tVector3D attackerVector;
	M3tVector3D defenderVector;
	UUtUns32 attack_owner, itr, attack_result, actual_damage;
	UUtBool canBlock = UUcFalse;
	UUtBool isInvulnerable;
	UUtBool attackLandedAlready;
	UUtBool attacker_is_being_thrown;
	MAtMaterialType material_type;
	MAtImpactType impact_type;
	UUtUns16 defender_part, impact_modifier;
	M3tPoint3D defender_part_location;
	float current_time, ai_sound_volumemodifier, actual_knockback;
	TRtBody *body;
	ONtCharacterParticleInstance *particle;
	P3tEffectData effect_data;
	UUtBool isUnstoppable, isOmnipotent, hasSuperShield;
	float damage_multiplier;


	UUmAssertReadPtr(inAttacker, sizeof(*inAttacker));
	UUmAssertReadPtr(inDefender, sizeof(*inDefender));
	UUmAssertReadPtr(inActiveAttacker, sizeof(*inActiveAttacker));
	UUmAssertReadPtr(inActiveDefender, sizeof(*inActiveDefender));
	UUmAssertReadPtr(inAttack, sizeof(*inAttack));

	attacker_is_being_thrown = TRrAnimation_TestFlag(inActiveAttacker->animation, ONcAnimFlag_ThrowTarget);
	if ((AI2rTeam_FriendOrFoe(inAttacker->teamNumber, inDefender->teamNumber) == AI2cTeam_Friend) && !attacker_is_being_thrown) {
		return;
	}

	attackLandedAlready = ONiCharacter_TestAndSetHit(inAttacker, inActiveAttacker, inDefender, inAttackIndex);

	if (attackLandedAlready) {
		return;
	}

	isInvulnerable = TRrAnimation_TestFlag(inActiveDefender->animation, ONcAnimFlag_Invulnerable);
	isInvulnerable = isInvulnerable || TRrAnimation_IsInvulnerable(inActiveDefender->animation, inActiveDefender->animFrame);

	isUnstoppable = ONrCharacter_IsUnstoppable(inDefender);

	// characters with a super shield effect (mutant muro) are invulnerable and unstoppable when it is active
	hasSuperShield = ONrCharacter_HasSuperShield(inDefender);
	if (hasSuperShield) {
		isInvulnerable = UUcTrue;
		isUnstoppable = UUcTrue;
	}

	ONiCharacter_BecomeTemporarilyVisible(inAttacker, inActiveAttacker, inDefender, inActiveDefender);

	ONrCharacter_GetAttackVector(inAttacker, inActiveAttacker, &attackerVector);
	ONrCharacter_GetFacingVector(inDefender, &defenderVector);

	// can we block this attack ?
	if (0 == (inAttack->flags & (1 << ONcAttackFlag_Unblockable)))
	{
		UUtBool attackHigh = (inAttack->flags & (1 << ONcAttackFlag_AttackHigh)) > 0;
		UUtBool attackLow = (inAttack->flags & (1 << ONcAttackFlag_AttackLow)) > 0;
		UUtBool blockLow, blockHigh;

		// CB: TEMPORARY DEBUGGING
/*		if ((inDefender->charType == ONcChar_AI2) && (inDefender->ai2State.currentGoal == AI2cGoal_Combat) &&
			(inDefender->ai2State.currentState->state.combat.melee.currently_blocking)) {
			COrConsole_Printf("%s: hit when trying to block", inDefender->player_name);
		}*/

		canBlock = ONrCharacter_CouldBlock(inDefender, inAttacker, &blockLow, &blockHigh);

		if (canBlock) {
			if (attackHigh && !blockHigh) {
				canBlock = UUcFalse;
			}
			if (attackLow && !blockLow) {
				canBlock = UUcFalse;
			}
		}

		if (canBlock && inDefender->blockFunction) {
			canBlock = inDefender->blockFunction(inDefender);
		}
	}

	// tell the AI that this attack has landed - one way or the other they must
	// stop blocking
	AI2rNotifyAttackLanded(inAttacker, inDefender);

	// if the defender was knocked down and in a daze, they should get up now in order to avoid
	// the attacker getting multiple cheap shots in on them
	AI2rStopDaze(inDefender);

	// get the material type of the defender
	if (hasSuperShield) {
		material_type = MArMaterialType_GetByName("Super_Shield");
	} else {
		material_type = ONiCharacter_GetMaterialTypeFromMask(inDefender, inDefenderParts);
	}

	// no attack impact type found yet
	impact_type = MAcInvalidID;
	impact_modifier = ONcIEModType_Any;

	// get the location of the part of the defender that was hit
	defender_part = ONiCharacter_GetPrimaryPartFromBits(inDefender, inDefenderParts);
	ONrCharacter_GetPartLocation(inDefender, inActiveDefender, defender_part, &defender_part_location);
	if (ONgDebugCharacterImpacts) {
		COrConsole_Printf("hit defender part %d, material type %d (%s)",
						defender_part, material_type, MArMaterialType_GetName(material_type));
	}

	// get the attack's damage and knockback
	attack_owner = WPrOwner_MakeMeleeDamageFromCharacter(inAttacker);
	damage_multiplier = ONiCharacter_GetAttackPotency(inAttacker, &isOmnipotent);
	actual_knockback = damage_multiplier * inAttack->knockback;
	actual_damage = MUrUnsignedSmallFloat_To_Uns_Round(damage_multiplier * inAttack->damage);

	if (isOmnipotent) {
		// omnipotence beats all forms of defense
		isInvulnerable = UUcFalse;
		canBlock = UUcFalse;
		actual_damage = UUmMax(actual_damage, inDefender->hitPoints);
	}

	if (isInvulnerable) {
		attack_result = ONcAttack_Invulnerable;
		actual_damage = 0;
		actual_knockback = 1.0f;

		if (!ONgNoMeleeDamage) {
			ONrCharacter_TakeDamage(inDefender, 0, 1.f, &defender_part_location, &attackerVector,
									attack_owner, ONcAnimType_None);
		}
	}
	else if (canBlock) {
		attack_result = ONcAttack_Blocked;

		if (!ONgNoMeleeDamage) {
			{
				float adjustment_angle = ONrCharacter_RelativeAngleToCharacter_Raw(inDefender, inAttacker);

				inDefender->facing += adjustment_angle;
				UUmTrig_Clip(inDefender->facing);
			}

			if (inAttack->flags & (1 << ONcAttackFlag_SpecialMove)) {
				attack_result = ONcAttack_Hit;

				// the super move penetrates through the block but loses half its effect
				// NOTE: this is half of the BASE damage, not half of the powered-up damage... thus a
				// super move powered up to 150% will inflict 100% of its base damage on a blocking opponent
				actual_damage = UUmMax(1, actual_damage / 2);
				actual_knockback *= 0.5f;

				ONrCharacter_TakeDamage(inDefender, actual_damage, actual_knockback, &defender_part_location, &attackerVector, attack_owner, ONcAnimType_None);
			}
			else {
				// the move is blocked
				actual_damage = 0;
				actual_knockback = 1.0f;

				ONrCharacter_TakeDamage(inDefender, 0, 1.f, &defender_part_location, &attackerVector, attack_owner, ONcAnimType_None);
			}

			if (isUnstoppable) {
				// CB: unstoppable characters can block without being blockstunned
				inDefender->flags |= ONcCharacterFlag_PleaseBlock;
			}
			else if (inAttack->stagger > 0) {
				inActiveDefender->pendingDamageAnimType = ONcAnimType_Stagger;
				inActiveDefender->staggerStun = inAttack->stagger;
			}
			else {
				inDefender->flags |= ONcCharacterFlag_PleaseBlock;
				inActiveDefender->blockStun = inAttack->blockStun;
			}
		}
	}
	else {
		// this is a valid hit!
		attack_result = ONcAttack_Hit;

		if (!ONgNoMeleeDamage) {
			UUtBool defender_is_dizzy = (inActiveDefender->dizzyStun > 0) || (inActiveDefender->staggerStun > 0);
			UUtBool is_dizzy_stun = (inAttack->hitStun > 20);
			TRtAnimType damage_animation_type = inAttack->damageAnimation;
			UUtUns16 hitstun_amount = inAttack->hitStun;

			if (ONgPlayerFistsOfFury && (inAttacker->charType == ONcChar_Player)) {
				damage_animation_type = ONcAnimType_Blownup;

			} else if (defender_is_dizzy && is_dizzy_stun) {
				is_dizzy_stun = UUcFalse;
				damage_animation_type = ONrAnimType_UpgradeToKnockdown(damage_animation_type);
			}
			else if (is_dizzy_stun) {
				damage_animation_type = ONcAnimType_Stagger;
			}

			if ((!isUnstoppable) && (damage_animation_type != ONcAnimType_None)) {
				TRtAnimType new_anim_type = damage_animation_type;
				float damage_animation_adjustment_angle;

				new_anim_type = ONrCharacter_CalculateAnimType(inAttacker, inDefender, inActiveDefender, UUcTrue, new_anim_type);
				if (new_anim_type == ONcAnimType_None) {
					// computed no damage animation, don't do anything

				} else if (new_anim_type == ONcAnimType_Hit_Overlay) {
					// apply a hit overlay instead
					ONrCharacter_MinorHitReaction(inDefender);

				} else {
					if (ONrCharacter_IsKnockdownResistant(inDefender)) {
						// ensure that the stun values are set correctly for any changed type
						// of damage animation
						if (new_anim_type == ONcAnimType_Stagger) {
							if (damage_animation_type != ONcAnimType_Stagger) {
								// this stagger animation has been created due to a type change,
								// ensure it has at least some stagger stun attached to it
								is_dizzy_stun = UUcTrue;
								hitstun_amount = UUmMax(hitstun_amount, 30);
							}
						} else {
							// this is no longer a staggering attack
							is_dizzy_stun = UUcFalse;
						}
					}

					new_anim_type = ONiRemapAnimType_For_Direction(new_anim_type, inAttacker, inDefender);
					inActiveDefender->pendingDamageAnimType = new_anim_type;

					// COrConsole_Printf("knockdown %d %s", new_anim_type, ONrAnimTypeToString(new_anim_type));

					if (DamageAnimTypeHasFacing(new_anim_type)) {
						damage_animation_adjustment_angle = ONrCharacter_RelativeAngleToCharacter_Raw(inDefender, inAttacker);
						inDefender->facing += damage_animation_adjustment_angle;
						UUmTrig_Clip(inDefender->facing);

						if (AnimIsBehind(new_anim_type)) {
							inDefender->facing += M3cPi;
							UUmTrig_ClipHigh(inDefender->facing);
						}
					}
				}
			}

			if (actual_damage >= inDefender->hitPoints) {
				// we killed the defender, show the final flash
				attack_result = ONcAttack_Killed;
				inDefender->flags |= ONcCharacterFlag_FinalFlash;
			}

			ONrCharacter_TakeDamage(inDefender, actual_damage, actual_knockback, &defender_part_location,
									&attackerVector, attack_owner, ONcAnimType_None);

			if (is_dizzy_stun) {
				if (NULL != inActiveDefender) {
					inActiveDefender->staggerStun = hitstun_amount;
				}
			}
			else {
				if (NULL != inActiveDefender) {
					inActiveDefender->hitStun = hitstun_amount;
				}
			}

		}
	}

	if ((ONcAttack_Hit == attack_result) || (ONcAttack_Killed == attack_result)) {
		// get the impact type of the attacker's animation, and specific impact modifier if any
		impact_type = ONiCharacter_GetSpecificImpactType(inAttacker, inActiveAttacker, &impact_modifier);

		// trigger any particles that were attached to the attacker's body part that impacted
		body = ONrCharacter_GetBody(inAttacker, TRcBody_SuperHigh);
		UUmAssert(body);

		if (inActiveAttacker->activeParticles != 0) {
			particle = inActiveAttacker->particles;
			current_time = ((float) ONrGameState_GetGameTime()) / UUcFramesPerSecond;
			if (ONgDebugCharacterImpacts) {
				COrConsole_Printf("attack has %d particles, at current frame %d: active mask 0x%08X, bodypart index 0x%08X",
					inActiveAttacker->numParticles, inActiveAttacker->animFrame, inActiveAttacker->activeParticles, inAttackerParts);
			}
			for (itr = 0; itr < inActiveAttacker->numParticles; itr++, particle++) {
				// don't check inactive particles
				if ((inActiveAttacker->activeParticles & (1 << itr)) == 0)
					continue;

				// don't check dead particles
				if ((particle->particle == NULL) || (particle->particle_ref != particle->particle->header.self_ref))
					continue;

				// what body part is this attached to?
				if ((inAttackerParts & (1 << particle->bodypart_index)) == 0)
					continue;

				if (ONgDebugCharacterImpacts) {
					COrConsole_Printf("  - particle %d on bodypart %d is active, alive and its body part hit, pulse it",
								itr, particle->bodypart_index);
				}

				// tell this particle about the impact; disable the hitflash, because we are pulsing a particle
				P3rSendEvent(particle->particle_def->particle_class, particle->particle,
								P3cEvent_Pulse, current_time);
			}
		}
		else {
			if (ONgDebugCharacterImpacts) {
				COrConsole_Printf("attack has %d particles, but at current frame %d none are active",
					inActiveAttacker->numParticles, inActiveAttacker->animFrame);
			}
		}
	}

	// make sure that we at least have a generic impact type and modifier for this attack
	if (impact_type == MAcInvalidID) {
		impact_type = ONiCharacter_GetGenericImpactType(inAttacker, inAttackerParts, attack_result);
		if (ONgDebugCharacterImpacts) {
			COrConsole_Printf("unable to find impact type from animation, using default (%s)",
								MArImpactType_GetName(impact_type));
		}
	}
	if (impact_modifier == ONcIEModType_Any) {
		impact_modifier = ONiCharacter_GetImpactModifier(inAttack->damage);
		if (ONgDebugCharacterImpacts) {
			COrConsole_Printf("unable to find modifier type from animation, using default based on damage %d (%s)",
								inAttack->damage, ONgIEModTypeName[impact_modifier]);
		}
	}

	// set up effect data and play the appropriate impact effect
	UUrMemory_Clear(&effect_data, sizeof(effect_data));
	effect_data.cause_type	= P3cEffectCauseType_MeleeHit;
	effect_data.time		= ONrGameState_GetGameTime();
	effect_data.owner		= attack_owner;
	effect_data.position	= defender_part_location;
	effect_data.normal		= attackerVector;
	MUmVector_Negate(effect_data.normal);
	effect_data.parent_velocity = &effect_data.normal;

	ai_sound_volumemodifier = ONiCharacter_HitAISoundVolumeModifier(impact_modifier);

	if (ONgDebugCharacterImpacts) {
		COrConsole_Printf("* playing melee impact effect: impact %s, material %s, modifier %s",
							MArImpactType_GetName(impact_type), MArMaterialType_GetName(material_type), ONgIEModTypeName[impact_modifier]);
	}

	// set the tint of any global-tintable particles created to depend on the defender's health, and play
	// the impact effect (this alerts the AI)
	P3rSetGlobalTint(ONrCharacter_GetHealthShade(inDefender->hitPoints, inDefender->maxHitPoints));
	ONrImpactEffect(impact_type, material_type, impact_modifier, &effect_data, ai_sound_volumemodifier, inAttacker, inDefender);
	P3rClearGlobalTint();

	return;
}

static void HandleGenericMask(
				ONtCharacter *inAttacker,
				ONtCharacter *inDefender)
{
	ONtActiveCharacter *activeAttacker;
	M3tVector3D pushBackVector;
	M3tVector3D toMeVector;
	M3tPoint3D collision_point;
	M3tPlaneEquation collision_plane;

	activeAttacker = ONrGetActiveCharacter(inAttacker);
	if (activeAttacker == NULL)
		return;

	MUmVector_Subtract(toMeVector, inAttacker->location,  inDefender->location);
	toMeVector.y = 0.f;

	if (MUmVector_GetLengthSquared(toMeVector) < 1.f)
	{
		return;
	}

	MUmVector_Normalize(toMeVector);
	MUrVector_PushBack(&pushBackVector, &activeAttacker->movementThisFrame, &toMeVector);
	MUmVector_Subtract(activeAttacker->movementThisFrame, activeAttacker->movementThisFrame, pushBackVector);

	activeAttacker->numCharacterCollisions++;
	if (inAttacker->flags & ONcCharacterFlag_AIMovement) {
		if (AI2rTryToPathfindThrough(inAttacker, inDefender)) {
			// we are colliding with a character that we are currently ignoring with pathfinding
			activeAttacker->collidingWithTarget = UUcTrue;
		}

		// tell the AI about the collision encountered
		ONrCharacter_GetPelvisPosition(inAttacker, &collision_point);
		MUmVector_ScaleIncrement(collision_point, -0.5f, toMeVector);
		collision_plane.a = toMeVector.x;
		collision_plane.b = toMeVector.y;
		collision_plane.c = toMeVector.z;
		collision_plane.d = -MUrVector_DotProduct(&collision_point, &toMeVector);
		AI2rMovementState_NotifyCharacterCollision(inAttacker, &activeAttacker->movement_state, inDefender, &collision_point, &collision_plane);
	}

	return;
}

// step 1: build a collision list for both characters

static UUtUns32 BuildAttackCollisionList(
		ONtCharacter *attacker,
		ONtCharacter *defender,
		ONtActiveCharacter *active_attacker,
		ONtActiveCharacter *active_defender,
		const UUtUns32 *attackerPartList,
		const UUtUns32 *defenderPartList,
		TRtAttack *outAttacks,
		UUtUns32 *outAttackerParts,
		UUtUns32 *outDefenderParts)
{
	UUtUns32 result = 0;
	UUtUns32 max_precedence = 0;
	TRtBody *attackerBody = ONrCharacter_GetBody(attacker, TRcBody_SuperHigh);
	TRtBody *defenderBody = ONrCharacter_GetBody(defender, TRcBody_SuperHigh);

	UUtUns8 bodyPartItr;
	UUtUns8 numAttacks;
	UUtUns8	attackIndices[TRcMaxAttacks];
	UUtUns32 attackMask[TRcMaxAttacks] = { 0, 0 };
	UUtUns8 attackItr;

	*outAttackerParts = 0;
	*outDefenderParts  = 0;

	UUmAssertReadPtr(attacker, sizeof(*attacker));
	UUmAssertReadPtr(defender, sizeof(*defender));

	TRrAnimation_GetAttacks(active_attacker->animation, active_attacker->animFrame, &numAttacks, outAttacks, attackIndices);

	for(attackItr = 0; attackItr < numAttacks; attackItr++)
	{
		for(bodyPartItr = 0; bodyPartItr < defenderBody->numParts; bodyPartItr++)
		{
			UUtUns32 hit_mask = defenderPartList[bodyPartItr] & outAttacks[attackItr].damageBits;

			if (hit_mask) {
				result |= 1 << attackItr;
				max_precedence = UUmMax(max_precedence, outAttacks[attackItr].precedence);
				*outAttackerParts |= outAttacks[attackItr].damageBits;
				*outDefenderParts |= (1 << bodyPartItr);
				break;
			}
		}
	}

	UUmAssert(max_precedence < UUcMaxUns16);
	result = result | (max_precedence << 16);

	return result;
}

static void iCollideTwoLikelyCharacters(
	ONtCharacter *inCharA,
	ONtCharacter *inCharB)
{
	const TRtBody *bodyA = ONrCharacter_GetBody(inCharA, TRcBody_SuperHigh);
	const TRtBody *bodyB = ONrCharacter_GetBody(inCharB, TRcBody_SuperHigh);
	UUtUns32 collideListA[ONcNumCharacterParts];
	UUtUns32 collideListB[ONcNumCharacterParts];
	UUtBool collision = UUcFalse;
	UUtUns8 numPartsA = (UUtUns8) bodyA->numParts;
	UUtUns8 numPartsB = (UUtUns8) bodyB->numParts;
	UUtUns8 itrA, itrB;
	ONtActiveCharacter *a_active, *b_active;
	ONtCharacterIndexType a_index, b_index;

	UUrMemory_Clear(collideListA, sizeof(collideListA));
	UUrMemory_Clear(collideListB, sizeof(collideListB));

	UUmAssertReadPtr(inCharA, sizeof(*inCharA));
	UUmAssertReadPtr(inCharB, sizeof(*inCharB));
	UUmAssert(bodyA->numParts < UUcMaxUns8);	// justify above cast
	UUmAssert(bodyB->numParts < UUcMaxUns8);	// justify above cast
	a_active = ONrGetActiveCharacter(inCharA);
	b_active = ONrGetActiveCharacter(inCharB);
	a_index = ONrCharacter_GetIndex(inCharA);
	b_index = ONrCharacter_GetIndex(inCharB);

	if (a_active->throwing == b_index) return;
	if (b_active->throwing == a_index) return;

	for(itrA = 0; itrA < numPartsA; itrA++) {
	   const M3tMatrix4x3*	inMatrixA = a_active->matricies + itrA;
	   const M3tGeometry*	inGeometryA = bodyA->geometries[itrA];

		for(itrB = 0; itrB < numPartsB; itrB++) {
		   const M3tMatrix4x3*	inMatrixB = b_active->matricies + itrB;
		   const M3tGeometry*	inGeometryB = bodyB->geometries[itrB];
		   UUtBool collide;

		     collide = M3rIntersect_GeomGeom(
					inMatrixA,
					inGeometryA,
					inMatrixB,
					inGeometryB);

			 if (collide) {
				 collideListA[itrA] |= 1 << itrB;
				 collideListB[itrB] |= 1 << itrA;
				 collision = UUcTrue;
			 }
		}
	}



	if (collision)
	{
		UUtUns32 attack_mask_a;
		UUtUns32 attack_mask_b;
		UUtUns32 precedence_a;
		UUtUns32 precedence_b;
		UUtUns32 attack_itr;
		TRtAttack char_a_attacks[2];
		TRtAttack char_b_attacks[2];
		UUtUns32 a_damagers;			// parts of char a that caused damage to char b
		UUtUns32 b_damagers;			// parts of char b that caused damage to char a
		UUtUns32 a_damagees;			// parts of char a that were damaged
		UUtUns32 b_damagees;			// parts of char b that were damaged

		attack_mask_a =
			BuildAttackCollisionList(
				inCharA,
				inCharB,
				a_active,
				b_active,
				collideListA,
				collideListB,
				char_a_attacks,
				&a_damagers,
				&b_damagees);
		attack_mask_b =
			BuildAttackCollisionList(
				inCharB,
				inCharA,
				b_active,
				a_active,
				collideListB,
				collideListA,
				char_b_attacks,
				&b_damagers,
				&a_damagees);

		HandleGenericMask(inCharA, inCharB);
		HandleGenericMask(inCharB, inCharA);

		precedence_a = attack_mask_a >> 16;
		precedence_b = attack_mask_b >> 16;

		if (precedence_a >= precedence_b) {
			for(attack_itr = 0; attack_itr < a_active->animation->numAttacks; attack_itr++) {
				if (attack_mask_a & (1 << attack_itr)) {
					HandleAttackMask(inCharA, inCharB, a_active, b_active, char_a_attacks + attack_itr, attack_itr, a_damagers, b_damagees);
				}
			}
		}

		if (precedence_b >= precedence_a) {
			for(attack_itr = 0; attack_itr < b_active->animation->numAttacks; attack_itr++) {
				if (attack_mask_b & (1 << attack_itr)) {
					HandleAttackMask(inCharB, inCharA, b_active, a_active, char_b_attacks + attack_itr, attack_itr, b_damagers, a_damagees);
				}
			}
		}
	}

	return;
}

static void iCollideTwoCharacters(
	ONtCharacter *inCharA,
	ONtCharacter *inCharB)
{
	float distance_squared = 0;
	const float distance_to_great_to_collide_squared = UUmSQR(25.f);

//	if (inCharA->noCollisionTimer || inCharB->noCollisionTimer) {
//		if (inCharA->teamNumber == inCharB->teamNumber) {
			// Don't collide with our teammates during no-collision-time.
//			return;
//		}
//	}

	if (inCharA->flags & ONcCharacterFlag_Dead_2_Moving) {
		goto exit;
	}

	if (inCharB->flags & ONcCharacterFlag_Dead_2_Moving) {
		goto exit;
	}

	distance_squared = MUrPoint_Distance_Squared(&inCharA->location, &inCharB->location);

	if (distance_squared < distance_to_great_to_collide_squared) {
		iCollideTwoLikelyCharacters(inCharA, inCharB);
	}

exit:
	return;
}

void ONrCharacter_Heal(
		ONtCharacter		*ioCharacter,
		UUtUns32			inAmt,
		UUtBool				inAllowOverMax)
{
	if (NULL == ioCharacter) {
		goto exit;
	}

	UUmAssertReadPtr(ioCharacter, sizeof(*ioCharacter));

	if (ioCharacter->flags & ONcCharacterFlag_Dead) {
		goto exit;
	}

	ioCharacter->hitPoints += inAmt;

	if (!inAllowOverMax && ioCharacter->hitPoints > ioCharacter->maxHitPoints)
	{
		ioCharacter->hitPoints = ioCharacter->maxHitPoints;
	}

exit:
	return;
}

UUtBool ONrCharacter_HasSuperShield(ONtCharacter *ioCharacter)
{
	return (ioCharacter->characterClass->superShield && (ioCharacter->super_amount > 0.5f));
}

UUtBool ONrCharacter_IsUnstoppable(ONtCharacter *ioCharacter)
{
	if (ONgPlayerUnstoppable && (ioCharacter->charType == ONcChar_Player)) {
		return UUcTrue;
	}

	return ((ioCharacter->flags & ONcCharacterFlag_Unstoppable) > 0);
}

UUtBool ONrCharacter_IsInvincible(ONtCharacter *ioCharacter)
{
	if (ONgPlayerInvincible && (ioCharacter->charType == ONcChar_Player)) {
		return UUcTrue;
	}

	return ((ioCharacter->flags & ONcCharacterFlag_Invincible) > 0);
}

void ONrCharacter_DamageCompass(ONtCharacter *ioCharacter, const M3tPoint3D *inDirection)
{
	// set which quadrant the hit came from
	if (inDirection != NULL)
	{
		float					costheta, sintheta, facing;
		float					fwd_dot, right_dot;
		UUtUns32				quadrant;

		facing = ONrCharacter_GetCosmeticFacing(ioCharacter);
		costheta = MUrCos(facing);		sintheta = MUrSin(facing);
		fwd_dot   = sintheta * inDirection->x + costheta * inDirection->z;
		right_dot = costheta * inDirection->x - sintheta * inDirection->z;

		if (fwd_dot < right_dot) {
			if (fwd_dot < -right_dot) {
				// back quadrant
				quadrant = 3;
			} else {
				// right quadrant
				quadrant = 0;
			}
		} else {
			if (fwd_dot < -right_dot) {
				// left quadrant
				quadrant = 2;
			} else {
				// front quadrant
				quadrant = 1;
			}
		}

		ioCharacter->hits[quadrant] = M3cMaxAlpha;
	}

}

static void ONiCharacter_Defeated(ONtCharacter *ioCharacter)
{
	// the character has been defeated
	if (ioCharacter->scripts.defeated_name[0] != '\0') {
		if ((ioCharacter->scripts.flags & AIcScriptTableFlag_DefeatedCalled) == 0) {
			ONrCharacter_Defer_Script(ioCharacter->scripts.defeated_name, ioCharacter->player_name, "defeated");

			ioCharacter->scripts.flags |= AIcScriptTableFlag_DefeatedCalled;
		}
	}
}

void ONrCharacter_TakeDamage(
		ONtCharacter		*ioCharacter,
		UUtUns32			inDamage,
		float				inKnockback,
		const M3tPoint3D	*inLocation,
		const M3tVector3D	*inDirection,
		UUtUns32			inDamageOwner,
		TRtAnimType			inAnimType)
{
	WPtDamageOwnerType damage_owner_type = WPrOwner_GetType(inDamageOwner);
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
	ONtCharacter *attacker, *player;
	UUtBool unstoppable = ONrCharacter_IsUnstoppable(ioCharacter), defeated = UUcFalse;
	UUtUns32 damage_scale, hypo_damage, pain_type, death_chance;

	if (active_character != NULL) {
		active_character->hitStun = 0;
		active_character->blockStun = 0;
		active_character->hitStun = 0;
		active_character->staggerStun = 0;
		active_character->dizzyStun = 0;
	}

	attacker = WPrOwner_GetCharacter(inDamageOwner);

	UUmAssertReadPtr(ioCharacter, sizeof(*ioCharacter));
	if (NULL != inDirection) { UUmAssertReadPtr(inDirection, sizeof(*inDirection)); }
	if (NULL != inLocation) { UUmAssertReadPtr(inLocation, sizeof(*inLocation)); }
	if (NULL != attacker) { UUmAssertReadPtr(attacker, sizeof(*attacker)); }

	player = ONrGameState_GetPlayerCharacter();
	if ((damage_owner_type == WPcDamageOwner_CharacterMelee) &&
		((player == attacker) || (player == ioCharacter))) {
		ONrGameState_TauntEnable(ONcCharacter_TauntEnable_MeleeDamage);
	}

	if (ioCharacter->flags & ONcCharacterFlag_Dead)
	{
		ONiCharacter_Defeated(ioCharacter);
		return;
	}

	if (active_character != NULL) {
		// store damage direction and amount for use by weapon and powerup dropping
		if (inDirection == NULL) {
			MUmVector_Set(active_character->lastImpact, 0, 0, 0);
		}
		else {
			// don't generate tremendously huge or unnoticably small impacts
			damage_scale = UUmPin(inDamage, 5, 15);
			MUmVector_ScaleCopy(active_character->lastImpact, damage_scale, *inDirection);
		}

		active_character->hitStun = 0;
	}

	// break out of any neutral interactions that we are engaged in
	if (ioCharacter->neutral_interaction_char != NULL) {
		AI2rNeutral_Stop(ioCharacter, UUcFalse, UUcTrue, UUcFalse, UUcTrue);
	}

	if ((damage_owner_type == WPcDamageOwner_CharacterMelee) && (attacker != NULL)) {
		// someone has hit someone else in melee
		AI2rFight_AdjustForDamage(attacker, ioCharacter, inDamage);
	}


	if (!ONrCharacter_IsInvincible(ioCharacter)) {
		// call the hurt script if there is one
		if (ioCharacter->scripts.hurt_name[0] != '\0') {
			if ((ioCharacter->scripts.flags & AIcScriptTableFlag_HurtCalled) == 0) {
				ONrCharacter_Defer_Script(ioCharacter->scripts.hurt_name, ioCharacter->player_name, "hurt");

				ioCharacter->scripts.flags |= AIcScriptTableFlag_HurtCalled;
			}
		}

		if ((ioCharacter->flags & ONcCharacterFlag_Unkillable) && (ioCharacter->hitPoints <= inDamage))
		{
			if (WPcDamageOwner_ActOfGod == (inDamageOwner & WPcDamageOwner_TypeMask)) {
				COrConsole_Printf("killing %s (act of god)", ioCharacter->player_name);
			}
			else {
				// reduce the amount of damage so that the character can survive it
				inDamage = ioCharacter->hitPoints - 1;
				ONiCharacter_Defeated(ioCharacter);
			}

			defeated = UUcTrue;
		}

		if (ioCharacter->hitPoints > inDamage)
		{
			ioCharacter->hitPoints -= inDamage;

			// make the character cry out in pain
			pain_type = ONcPain_Light;
			if (active_character != NULL) {
				switch(active_character->curAnimType)
				{
					case ONcAnimType_Blownup:
					case ONcAnimType_Blownup_Behind:
						pain_type = ONcPain_Heavy;
					break;

					case ONcAnimType_Land_Dead:
					case ONcAnimType_Knockdown_Head:
					case ONcAnimType_Knockdown_Body:
					case ONcAnimType_Knockdown_Foot:
					case ONcAnimType_Knockdown_Head_Behind:
					case ONcAnimType_Knockdown_Body_Behind:
					case ONcAnimType_Knockdown_Foot_Behind:
						pain_type = ONcPain_Medium;
					break;
				}
			}
			ONrCharacter_Pain(ioCharacter, inDamage, pain_type);

			// reduce the amount of remaining-hypo by a factor more than the actual amount of damage that we took
			hypo_damage = MUrUnsignedSmallFloat_To_Uns_Round(ONcDamage_HurtHypoScale * inDamage);
			if (ioCharacter->inventory.hypoRemaining < hypo_damage)
			{
				ioCharacter->inventory.hypoRemaining = 0;
			}
			else
			{
				ioCharacter->inventory.hypoRemaining -= (UUtUns16) hypo_damage;
			}

			if (ioCharacter->charType == ONcChar_AI2) {
				// tell the AI that it's been hurt
				AI2rNotify_Damage(ioCharacter, inDamage, inDamageOwner);
			}
		}
		else
		{
			ioCharacter->hitPoints = 0;
			ioCharacter->inventory.hypoRemaining = 0;

			if (damage_owner_type != WPcDamageOwner_ActOfGod) {
				if (ioCharacter->painState.num_medium > 0) {
					// we are in the middle of screaming anyway
					ONrCharacter_PlayHurtSound(ioCharacter, ONcPain_Heavy, NULL);
				} else {
					// maybe play a death scream
					death_chance = (ioCharacter->characterClass->painConstants.hurt_death_chance * UUcMaxUns16) / 100;
					if (UUrRandom() < death_chance) {
						ONrCharacter_PlayHurtSound(ioCharacter, ONcPain_Death, NULL);
					}
				}
			}

			if (ONcAnimType_None != inAnimType) {
				ONiCharacter_RunDeathAnimation(ioCharacter, active_character, inAnimType);
			}
			else if (NULL != active_character) {
				if (ONrAnimType_IsKnockdown(active_character->pendingDamageAnimType)) {
					ONiCharacter_RunDeathAnimation(ioCharacter, active_character, active_character->pendingDamageAnimType);
				}
			}

			ONrCharacter_Die_1_Animating_Hook(ioCharacter, attacker);
		}

		if (defeated) {
			ONiCharacter_RunDeathAnimation(ioCharacter, active_character, TRcAnimType_None);
		}
	}

	// record the damage done by the attacker
	if (attacker)
	{
		attacker->damageInflicted += inDamage;

		if (ioCharacter->hitPoints <= 0)
		{
			attacker->numKills++;
		}
	}

	if (!unstoppable) {
		// handle knockback of charcters from hit, blocked hit knockback ?
		if ((active_character != NULL) && (inKnockback != 0.f) && (NULL != inDirection))
		{
			// CB: note there can be negative knockback now - 06 october, 2000
			M3tVector3D newKnockbackVector = *inDirection;
			UUmAssert(MUmVector_IsNormalized(newKnockbackVector));

			MUmVector_Scale(newKnockbackVector, inKnockback);
			MUmVector_Add(active_character->knockback, active_character->knockback, newKnockbackVector);
		}
	}

	ONrCharacter_DamageCompass(ioCharacter, inDirection);

	ONrCharacter_Commit_Scripts();

	return;
}

void ONrCharacter_Die(ONtCharacter *ioCharacter)
{
	UUmAssertReadPtr(ioCharacter, sizeof(*ioCharacter));

	if (0 == (ioCharacter->flags & ONcCharacterFlag_Dead))
	{
		ioCharacter->hitPoints = 0;
		ONrCharacter_Die_1_Animating_Hook(ioCharacter, NULL);
	}

	return;
}

UUtBool ONiPointInSphere(const M3tPoint3D *inPoint, const M3tBoundingSphere *inSphere)
{
	float distance_squared;
	UUtBool pointInSphere;

	distance_squared = MUrPoint_Distance_Squared(inPoint, &(inSphere->center));

	pointInSphere = distance_squared <= UUmSQR(inSphere->radius);

	return pointInSphere;
}

static void ONiGameState_DoCharacterCollision(void)
{
	UUtUns32 i,j;
	UUtBool must_unlock;
	ONtCharacter **chrList = ONrGameState_ActiveCharacterList_Get();
	UUtUns32 chrCount = ONrGameState_ActiveCharacterList_Count();
	ONtActiveCharacter *active_char;

	ONrGameState_ActiveCharacterList_Lock();
	must_unlock = ONrCharacter_Lock_Scripts();

	for(i = 0; i < chrCount; i++) {
		ONtCharacter *charA	= chrList[i];

		if (charA->flags & (ONcCharacterFlag_Dead_3_Cosmetic | ONcCharacterFlag_Teleporting))
			continue;

		for(j = i + 1; j < chrCount; j++) {
			ONtCharacter *charB = chrList[j];

			if (charB->flags & (ONcCharacterFlag_Dead_3_Cosmetic | ONcCharacterFlag_Teleporting))
				continue;

			// collision detect these two characters
			iCollideTwoCharacters(charA, charB);
		}
	}

	for(i = 0; i < chrCount; i++) {
		ONtCharacter *curChar = chrList[i];
		active_char = ONrGetActiveCharacter(curChar);
		UUmAssertReadPtr(active_char, sizeof(*active_char));

		if (active_char->pendingDamageAnimType != ONcAnimType_None) {
			if (curChar->flags & ONcCharacterFlag_Dead) {
				UUmAssert(ONrAnimState_IsFallen(active_char->nextAnimState));
			}
			else {
				ONrCharacter_Knockdown(NULL, curChar, active_char->pendingDamageAnimType);
			}

			active_char->pendingDamageAnimType = ONcAnimType_None;
		}
		else if (curChar->flags & ONcCharacterFlag_PleaseBlock) {
			ONrCharacter_Block(curChar, active_char);
		}

		curChar->flags &= ~ONcCharacterFlag_PleaseBlock;
	}

	ONrGameState_ActiveCharacterList_Unlock();
	if (must_unlock) {
		ONrCharacter_Unlock_Scripts();
		ONrCharacter_Commit_Scripts();
	}

	return;
}


void ONrDoCharacterCollision(void)
{
	ONiGameState_DoCharacterCollision();

	return;
}


void ONrCharacter_Trigger(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUmAssert(NULL != ONgGameState);
	UUmAssert(NULL != ioCharacter);

	if (NULL == ioCharacter->inventory.weapons[0]) {
		return;
	}

	ioActiveCharacter->pleaseFire = UUcTrue;

	return;
}

void ONrCharacter_AutoTrigger(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUmAssertWritePtr(ONgGameState, sizeof(*ONgGameState));
	UUmAssertWritePtr(ioCharacter, sizeof(ONtCharacter));

	if (NULL == ioCharacter->inventory.weapons[0]) {
		return;
	}

	if (WPrIsAutoFire(ioCharacter->inventory.weapons[0])) {
		ioActiveCharacter->pleaseAutoFire = UUcTrue;
	} else {
		ioActiveCharacter->pleaseContinueFiring = UUcTrue;
	}

	return;
}

void ONrCharacter_AlternateTrigger(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUmAssert(NULL != ONgGameState);
	UUmAssert(NULL != ioCharacter);

	if ((NULL == ioCharacter->inventory.weapons[0]) || ((WPrGetClass(ioCharacter->inventory.weapons[0])->flags & WPcWeaponClassFlag_HasAlternate) == 0)) {
		return;
	}

	ioActiveCharacter->pleaseFireAlternate = UUcTrue;

	return;
}

void ONrCharacter_ReleaseTrigger(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUmAssert(NULL != ONgGameState);
	UUmAssert(NULL != ioCharacter);

	if (NULL == ioCharacter->inventory.weapons[0]) {
		return;
	}

	ioActiveCharacter->pleaseReleaseTrigger = UUcTrue;

	return;
}

void ONrCharacter_ReleaseAlternateTrigger(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUmAssert(NULL != ONgGameState);
	UUmAssert(NULL != ioCharacter);

	if (NULL == ioCharacter->inventory.weapons[0]) {
		return;
	}

	ioActiveCharacter->pleaseReleaseAlternateTrigger = UUcTrue;

	return;
}

static void ONiCharacter_Fire(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter, ONtFireMode inFireMode)
{
	UUtBool fired, out_of_ammo, reloaded, is_ai, is_autofire, play_click;

	UUmAssertReadPtr(ONgGameState, sizeof(*ONgGameState));
	UUmAssertReadPtr(ioCharacter, sizeof(*ioCharacter));

	is_ai = ((ioCharacter->charType == ONcChar_AI2) && !ONrCharacter_IsPlayingFilm(ioCharacter));

	if (inFireMode == ONcFireMode_Alternate) {
		ioActiveCharacter->pleaseFireAlternate = UUcFalse;
	} else {
		ioActiveCharacter->pleaseFire = UUcFalse;
		ioActiveCharacter->pleaseAutoFire = UUcFalse;
		ioActiveCharacter->pleaseContinueFiring = UUcFalse;
	}

	if (NULL == ioCharacter->inventory.weapons[0]) {
		return;
	}

	if (ONrCharacter_IsGunThroughWall(ioCharacter, ioActiveCharacter)) {
		ONiCharacter_StopFiring(ioCharacter, ioActiveCharacter);
		return;
	}

	MUmVector_Verify(ioActiveCharacter->aimingVector);

	if (ioCharacter->flags & ONcCharacterFlag_Dead) {
		return;
	}

	// mark you as not idle
	ONrCharacter_NotIdle(ioCharacter);

	// fire the gun
	fired = WPrDepressTrigger(ioCharacter->inventory.weapons[0], UUcFalse, (inFireMode == ONcFireMode_Alternate), &out_of_ammo);
	if (out_of_ammo) {
		// we have to stop firing now, out of ammo
		WPrReleaseTrigger(ioCharacter->inventory.weapons[0], UUcFalse);

		// try to reload
		is_autofire = ((inFireMode == ONcFireMode_Autofire) || (inFireMode == ONcFireMode_Continuous));

		if (!is_autofire || is_ai) {
			reloaded = WPrTryReload(ioCharacter, ioCharacter->inventory.weapons[0], NULL, &out_of_ammo);
			play_click = out_of_ammo;
		}
		else {
			// we can't reload our weapon by just holding the trigger down
			play_click = UUcTrue;
		}

		if (play_click) {
			WPrOutOfAmmoSound(ioCharacter->inventory.weapons[0]);
		}

		if (out_of_ammo && is_ai) {
			// the AI is out of ammo
			ioCharacter->ai2State.flags |= AI2cFlag_RunOutOfAmmoScript;
		}
	}

	if (fired && (ioCharacter->charType == ONcChar_AI2)) {
		AI2rNotifyAIFired(ioCharacter);
	}

	return;
}

static void ONiCharacter_StopFiring(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUmAssertReadPtr(ONgGameState, sizeof(*ONgGameState));
	UUmAssertReadPtr(ioCharacter, sizeof(*ioCharacter));

	ioActiveCharacter->pleaseReleaseTrigger = UUcFalse;
	ioActiveCharacter->pleaseReleaseAlternateTrigger = UUcFalse;

	if (ioCharacter->inventory.weapons[0] != NULL) {
		WPrReleaseTrigger(ioCharacter->inventory.weapons[0], UUcFalse);
	}

	return;
}

void ONrCharacter_FindOurNode(ONtCharacter *inCharacter)
{
	M3tPoint3D p;
	PHtGraph *graph = ONrGameState_GetGraph();

	UUmAssertReadPtr(inCharacter, sizeof(*inCharacter));
	UUmAssertReadPtr(ONgLevel, sizeof(*ONgLevel));
	UUmAssertReadPtr(ONgLevel->environment, sizeof(*ONgLevel->environment));
	UUmAssertReadPtr(graph, sizeof(*graph));

	ONrCharacter_GetPathfindingLocation(inCharacter, &p);
	inCharacter->currentNode = AKrNodeFromPoint(&p);
	inCharacter->currentPathnode = PHrAkiraNodeToGraphNode(inCharacter->currentNode, graph);

	return;
}

void ONrCharacter_Teleport(ONtCharacter *ioCharacter, M3tPoint3D *inPoint, UUtBool inRePath)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);

	ioCharacter->prev_location = *inPoint;
	ioCharacter->location = *inPoint;
	ioCharacter->actual_position = *inPoint;
	ioCharacter->actual_position.y += PHcWorldCoord_YOffset;

	if ((active_character != NULL) && (active_character->physics != NULL)) {
		active_character->physics->position = *inPoint;
		active_character->deferred_maximum_height = inPoint->y;
	}

	ONrCharacter_FindOurNode(ioCharacter);

	if (inRePath && (ioCharacter->flags & ONcCharacterFlag_AIMovement)) {
		AI2rPath_Repath(ioCharacter, UUcTrue);
	}

	return;
}

void ONrCharacter_Teleport_With_Collision(ONtCharacter *ioCharacter, M3tPoint3D *inPoint, UUtBool inRePath)
{
	// ok we are teleporting a character to inPoint but we want to find collision
	// just like we were doing that movement

	ONtActiveCharacter *active_character = ONrForceActiveCharacter(ioCharacter);

	if (NULL != active_character) {
		M3tPoint3D movement_vector;

		MUmVector_Subtract(movement_vector, *inPoint, active_character->physics->position);

		if (OBrPhysics_Update_Reflect(active_character->physics, &movement_vector)) {
			OBrPhysics_Update_ReflectHalt(active_character->physics, &movement_vector);
		}

		MUmVector_Increment(active_character->physics->position, movement_vector);

		ONrCharacter_Teleport(ioCharacter, &active_character->physics->position, inRePath);
	}

	return;
}

UUtBool ONrCharacter_LineOfSight(ONtCharacter *inLooker, ONtCharacter *inLookee)
{
	/**********************
	* Returns whether 'inLooker' can see 'inLookee'. Does not respect
	* fields of view.
	*/

	M3tPoint3D src,dest;

	src = inLooker->location;
	dest = inLookee->location;
	src.y += ONcCharacterHeight;
	dest.y+= ONcCharacterHeight;

	return AKrLineOfSight(&src,&dest);
}

float ONrCharacter_RelativeAngleToPoint_Raw(const ONtCharacter *inFromChar, const M3tPoint3D *inPoint)
{
	M3tVector3D toVector;
	float angle;
	float facing = inFromChar->facing;

	MUmVector_Subtract(toVector, *inPoint, inFromChar->location);
	angle = (float) MUrATan2(toVector.x, toVector.z);
	UUmTrig_ClipLow(angle);

	if (angle < 0.f) {
		angle += M3c2Pi;
	}

	angle = angle - facing;
	if (angle < 0) { angle += M3c2Pi; }
	angle = iSignAngle(angle);
	UUmAssert((float)fabs(angle) <= M3cPi);

	return angle;
}

float ONrCharacter_RelativeAngleToCharacter_Raw(const ONtCharacter *inFromChar, const ONtCharacter *inToChar)
{
	float angle;

	angle = ONrCharacter_RelativeAngleToPoint_Raw(inFromChar, &inToChar->location);

	return angle;
}

void ONrCharacter_FaceAtPoint_Raw(ONtCharacter *inCharacter, const M3tPoint3D *inPointToFace)
{
	float adjustment_angle = ONrCharacter_RelativeAngleToPoint_Raw(inCharacter, inPointToFace);

	inCharacter->facing += adjustment_angle;
	UUmTrig_Clip(inCharacter->facing);

	return;
}

void ONrCharacter_FaceVector_Raw(ONtCharacter *inCharacter, const M3tVector3D *inVectorToFace)
{
	float angle = MUrATan2(inVectorToFace->x, inVectorToFace->z);
	UUmTrig_ClipLow(angle);

	inCharacter->facing = angle;

	return;
}

UUtInt32 ONrCharacter_FaceAwayOrAtVector_Raw(ONtCharacter *inCharacter, const M3tPoint3D *inVectorToFace)
{
	float angle = MUrATan2(inVectorToFace->x, inVectorToFace->z);
	float delta;
	UUtInt32 result = 1;

	UUmTrig_ClipLow(angle);

	delta = angle - inCharacter->facing;
	UUmTrig_ClipAbsPi(delta);

	if ((float)fabs(delta) > M3cHalfPi) {
		angle += M3cPi;					// facing at the reverse is closer
		UUmTrig_ClipHigh(angle);

		result = -1;
	}

	inCharacter->facing = angle;

	return result;
}


float ONrCharacter_RelativeAngleToCharacter(const ONtCharacter *inFromChar, const ONtCharacter *inToChar)
{
	M3tVector3D toVector;
	float angle;
	float facing = ONrCharacter_GetCosmeticFacing(inFromChar);

	MUmVector_Subtract(toVector, inToChar->location, inFromChar->location);
	angle = (float) MUrATan2(toVector.x, toVector.z);
	UUmTrig_ClipLow(angle);

	if (angle < 0.f) {
		angle += M3c2Pi;
	}

	angle = angle - facing;
	if (angle < 0) { angle += M3c2Pi; }
	angle = iSignAngle(angle);
	UUmAssert((float)fabs(angle) <= M3cPi);

	return angle;
}

static ONtCharacter *ONiCharacter_FindNearestOpponent(
			ONtCharacter *inCharacter,
			float inFacingOffset,
			float inMaxDistance,
			float inMaxAngle,
			float *outFacingOffset,
			UUtBool inSkipInvulnerable)
{
	ONtCharacter *best = NULL;
	float bestDistanceSoFar = inMaxDistance + 1.f;
	float bestAngleSoFar = inMaxAngle + 1.f;
	float facingOffset;
	UUtUns16 itr;
	ONtCharacter **active_list;
	UUtUns32 active_count;

	UUmAssertReadPtr(inCharacter, sizeof(*inCharacter));
	UUmAssertReadPtr(outFacingOffset, sizeof(*outFacingOffset));
	UUmAssertTrigRange(inFacingOffset);

	*outFacingOffset = 0.f;

	active_list = ONrGameState_ActiveCharacterList_Get();
	active_count = ONrGameState_ActiveCharacterList_Count();

	for(itr = 0; itr < active_count; itr++) {
		ONtCharacter *characterItr = active_list[itr];
		float distance;
		float angle;

		if (characterItr->flags & ONcCharacterFlag_Dead) {
			continue;
		}

		if (characterItr == inCharacter) {
			continue;
		}

		if (AI2rTeam_FriendOrFoe(characterItr->teamNumber, inCharacter->teamNumber) == AI2cTeam_Friend) {
			continue;
		}

		if (inSkipInvulnerable) {
			ONtActiveCharacter *active_character = ONrGetActiveCharacter(characterItr);

			if (ONrCharacter_IsUnstoppable(characterItr) || (characterItr->flags & ONcCharacterFlag_Teleporting) ||
				ONrCharacter_IsKnockdownResistant(characterItr) ||
				((active_character != NULL) && TRrAnimation_TestFlag(active_character->animation, ONcAnimFlag_Invulnerable))) {
				continue;
			}
		}

		distance = MUrPoint_Distance(&inCharacter->location, &characterItr->location);
		angle = ONrCharacter_RelativeAngleToCharacter(inCharacter, characterItr);
		UUmTrig_ClipLow(angle);	// clip so it is not a relative angle

		// subtract & clip
		angle = angle - inFacingOffset;
		UUmTrig_ClipLow(angle);

		// turn into a relative angle
		if (angle > M3cPi) {
			angle -= M3c2Pi;
		}

		if (distance > inMaxDistance) {
			continue;
		}

		if ((float)fabs(angle) > inMaxAngle) {
			continue;
		}

		if ((float)fabs(angle) < bestAngleSoFar) {
			best = characterItr;
			facingOffset = angle;

			bestAngleSoFar = (float)fabs(angle);
			bestDistanceSoFar = distance;
		}
	}

	if (NULL != best) {
		*outFacingOffset = facingOffset;
	}

	return best;
}

static void ONiCharacter_FaceOpponent(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	ONtCharacter *target = NULL;
	float angleDeltaToApply;
	float attackFacing;
	TRtDirection direction = TRrAnimation_GetDirection(ioActiveCharacter->animation, UUcFalse);

	switch(direction) {
		case TRcDirection_None:
		case TRcDirection_360:
		case TRcDirection_Forwards:
			attackFacing = 0;
		break;

		case TRcDirection_Left:
			attackFacing = M3cHalfPi;
		break;

		case TRcDirection_Right:
			attackFacing = M3c2Pi - M3cHalfPi;
		break;

		case TRcDirection_Back:
			attackFacing = M3cPi;
		break;

		default:
			UUmAssert(!"ONiCharacter_FaceOpponent: unknown moveDirection");
			attackFacing = 0;
		break;
	}

	target = ONiCharacter_FindNearestOpponent(
		ioCharacter,
		attackFacing,
		ONgAutoAim_Distance,
		ONgAutoAim_Arc * M3cDegToRad,
		&angleDeltaToApply,
		UUcFalse);

	if (NULL == target) {
		goto exit;
	}

	if (AI2cTeam_Hostile != AI2rTeam_FriendOrFoe(ioCharacter->teamNumber, target->teamNumber)) {
		goto exit;
	}

	// search through all the characters and find the best angle fit and min distance
	// auto aim to that one

	ONrCharacter_AdjustFacing(ioCharacter, angleDeltaToApply);

	if (gDebugCollision) {
		switch(direction) {
			case TRcDirection_None:
				COrConsole_Printf("a: none");
			break;

			case TRcDirection_360:
				COrConsole_Printf("a: 360");
			break;

			case TRcDirection_Forwards:
				COrConsole_Printf("a: fwd");
			break;

			case TRcDirection_Left:
				COrConsole_Printf("a: lft");
			break;

			case TRcDirection_Right:
				COrConsole_Printf("a: rgt");
			break;

			case TRcDirection_Back:
				COrConsole_Printf("a: bck");
			break;

			default:
				COrConsole_Printf("a: ???");
			break;
		}
	}

exit:
	return;
}

UUtBool ONrCharacter_IsAI(const ONtCharacter *ioCharacter)
{
	ONtCharacterIndexType index = ONrCharacter_GetIndex((ONtCharacter *) ioCharacter);

	if ((ioCharacter->charType == ONcChar_Player) || (index == ONrGameState_GetPlayerNum())) {
		return UUcFalse;
	} else {
		return UUcTrue;
	}
}

UUtBool ONrAnimation_IsFlying(const TRtAnimation *inAnimation)
{
	TRtAnimState fromState;
	UUtBool flying;

	fromState = TRrAnimation_GetFrom(inAnimation);

	switch(fromState)
	{
		case ONcAnimState_Flying:
		case ONcAnimState_Falling:
		case ONcAnimState_Flying_Forward:
		case ONcAnimState_Falling_Forward:
		case ONcAnimState_Flying_Back:
		case ONcAnimState_Falling_Back:
		case ONcAnimState_Flying_Left:
		case ONcAnimState_Falling_Left:
		case ONcAnimState_Flying_Right:
		case ONcAnimState_Falling_Right:
			flying = UUcTrue;
		break;

		default:
			flying = UUcFalse;
	}

	return flying;
}

static void ONiCharacter_ThrowPhysics(ONtCharacter *inSource, ONtActiveCharacter *inActiveSource,
									  ONtCharacter *inTarget, ONtActiveCharacter *inActiveTarget)
{
	M3tVector3D position, offset;

	if ((NULL == inSource) || (NULL == inTarget)) {
		return;
	}

	// if neither is active, cancel
	if ((NULL == inActiveSource) || (NULL == inActiveTarget)) {
		return;
	}

	// if neither has physics, cancel
	if ((NULL == inActiveSource->physics) || (NULL == inActiveTarget->physics)) {
		return;
	}

	if (NULL == inActiveSource->animation->throwInfo) {
		return;
	}

	UUmAssertReadPtr(inSource, sizeof(*inSource));
	position = inSource->location;

	UUmAssertReadPtr(inActiveSource, sizeof(*inActiveSource));
	UUmAssertReadPtr(inActiveSource->animation, sizeof(*inActiveSource->animation));
	UUmAssertReadPtr(inActiveSource->animation->throwInfo, sizeof(*inActiveSource->animation->throwInfo));

	offset = inActiveSource->animation->throwInfo->throwOffset;
	MUrPoint_RotateYAxis(&offset, inSource->facing, &offset);

	MUmVector_Increment(position, offset);

	ONrCharacter_Teleport_With_Collision(inTarget, &position, UUcFalse);

	return;
}

static P3tParticle *ONiCharacter_AttachParticle(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter,
												P3tParticleClass *inParticleClass, UUtUns32 inPartIndex)
{
	P3tEffectSpecification effect_spec;
	P3tEffectData effect_data;
	const TRtBody *character_body;
	M3tGeometry *attach_geometry;
	M3tBoundingBox_MinMax *bbox;
	P3tParticle *particle;

	UUrMemory_Clear(&effect_data, sizeof(effect_data));
	effect_spec.collision_orientation = P3cEnumCollisionOrientation_Unchanged;
	effect_spec.location_type = P3cEffectLocationType_Default;
	effect_spec.particle_class = inParticleClass;

	effect_data.cause_type = P3cEffectCauseType_None;
	effect_data.owner = WPrOwner_MakeMeleeDamageFromCharacter(ioCharacter);
	effect_data.time = ONrGameState_GetGameTime();
	effect_data.parent_orientation = (M3tMatrix3x3 *) &MUgIdentityMatrix;
	effect_data.attach_matrix = ioActiveCharacter->matricies + inPartIndex;
	MUmVector_Set(effect_data.velocity, 0, 0, 0);

	// place the particle at the center of the body part's bounding box
	character_body = ONrCharacter_GetBody(ioCharacter, TRcBody_SuperHigh);
	attach_geometry = character_body->geometries[inPartIndex];
	bbox = &attach_geometry->pointArray->minmax_boundingBox;
	MUmVector_Add(effect_data.position, bbox->minPoint, bbox->maxPoint);
	MUmVector_Scale(effect_data.position, 0.5);

	// create the particle
	particle = P3rCreateEffect(&effect_spec, &effect_data);
	if (particle == NULL) {
		return NULL;
	}

	// because the particle is attached to our body (which may move independently), it
	// must always update its position.
	particle->header.flags |= P3cParticleFlag_AlwaysUpdatePosition;

	return particle;
}

void ONrCharacter_NewAnimationHook(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	const TRtAnimation *animation = ioActiveCharacter->animation;
	TRtAnimType new_animation_type = animation->type;
	TRtAnimVarient	animVarient = TRrAnimation_GetVarient(animation);
	UUtBool hasFightVarient = (ONcAnimVarientMask_Fight & animVarient) > 0;
	UUtUns16 vocalization;

	// increment our animation counter... used by AI2 to determine which
	// animations it has to respond to
	ioCharacter->animCounter++;

#if DEBUG_ATTACKEXTENT
	{
		// CB: temporary debugging code
		if (ioCharacter == ONrGameState_GetPlayerCharacter()) {
			ONgTempDebug_AttackExtent_Animation = (TRtAnimation *) animation;
			ONgTempDebug_AttackExtent_CurTime = ONrGameState_GetGameTime();
			ONgTempDebug_AttackExtent_Frame = 0;
			ONgTempDebug_AttackExtent_Facing = ioCharacter->facing;
			ONgTempDebug_AttackExtent_CosTheta = MUrCos(ONgTempDebug_AttackExtent_Facing);
			ONgTempDebug_AttackExtent_SinTheta = MUrSin(ONgTempDebug_AttackExtent_Facing);
			ONgTempDebug_AttackExtent_BasePt = ioCharacter->location;
			ONgTempDebug_AttackExtent_CurDelta[0] = 0.0f;
			ONgTempDebug_AttackExtent_CurDelta[1] = 0.0f;
		}
	}
#endif

	vocalization = TRrAnimation_GetVocalization(animation);
	if ((vocalization >= 0) && (vocalization < AI2cVocalization_Max)) {
		AI2rVocalize(ioCharacter, (UUtUns32) vocalization, UUcFalse);
	}

	if (ioActiveCharacter->teleportEnable && (new_animation_type == ONcAnimType_Teleport_Out)) {
		// do the previously setup teleport
		ONrCharacter_SetFacing(ioCharacter, ioActiveCharacter->teleportDestFacing);
		ONrCharacter_Teleport(ioCharacter, &ioActiveCharacter->teleportDestination, UUcTrue);
	}

	if (ioCharacter->charType == ONcChar_AI2) {
		AI2rMelee_NotifyAnimation(ioCharacter, (void *)animation, new_animation_type, ioActiveCharacter->nextAnimType,	ioActiveCharacter->curFromState, ioActiveCharacter->nextAnimState);
	}

	if (!(TRrAnimation_TestFlag(animation, ONcAnimFlag_ThrowSource))) {
		ioActiveCharacter->throwTarget = NULL;
		ioActiveCharacter->targetThrow = NULL;
	}

	if ((ONrAnimType_IsVictimType(new_animation_type)) || (new_animation_type == ONcAnimType_Block)) {
		if (ioCharacter->console_marker != NULL) {
			ONrCharacter_ConsoleAction_Finish(ioCharacter, UUcFalse);
		}
	}

	if (TRrAnimation_TestFlag(animation, ONcAnimFlag_DropWeapon)) {
		ONrCharacter_DropWeapon(ioCharacter,UUcTrue,WPcPrimarySlot,UUcFalse);
	}

	ioActiveCharacter->throwing = 0xffff;

	if (TRrAnimation_IsAttack(ioActiveCharacter->animation)) {
		if (!TRrAnimation_TestFlag(animation, ONcAnimFlag_ThrowSource)) {
			if (!ONrCharacter_IsAI(ioCharacter)) {
				if (0 == ioActiveCharacter->inAirControl.numFramesInAir) {
					ONiCharacter_FaceOpponent(ioCharacter, ioActiveCharacter);
				}
			}

			ioActiveCharacter->lastAttack = TRrAnimation_GetType(ioActiveCharacter->animation);
			ioActiveCharacter->lastAttackTime = ONcLastAttackTime_StillActive;
		}
	}

	if (TRrAnimation_TestFlag(animation, ONcAnimFlag_ThrowSource)) {
		ONtCharacter *target = ioActiveCharacter->throwTarget;

		if (NULL == target) {
			COrConsole_Printf("throwtarget was NULL");
		}
		else {
			ONtActiveCharacter *active_target;

			UUmAssertReadPtr(target, sizeof(*target));

			active_target = ONrForceActiveCharacter(target);

			if (NULL == active_target) {
				COrConsole_Printf("failed to make throw target active");
			}
			else {
				UUmAssertReadPtr(active_target, sizeof(*active_target));

				if ((!(target->flags & ONcCharacterFlag_Dead_1_Animating)) && (target->flags & ONcCharacterFlag_InUse))
				{
					if (TRrAnimation_GetVarient(animation) & ONcAnimVarientMask_Fight) {
						ONrCharacter_FightMode(target);
					}

					if (target->charType == ONcChar_AI2) {
						// an AI has just been knocked down by a throw
						AI2rNotifyKnockdown(target);
					}

					ONiCharacter_BecomeTemporarilyVisible(ioCharacter, ioActiveCharacter, target, active_target);
					AI2rKnowledge_Sound(AI2cContactType_Sound_Melee, &ioCharacter->actual_position, 150.0f, ioCharacter, target);

					ONrCharacter_SetAnimation_External(target, active_target->nextAnimState, ioActiveCharacter->targetThrow, 6);

					ONiCharacter_ThrowPhysics(ioCharacter, ioActiveCharacter, target, active_target);

					ioActiveCharacter->throwing = ONrCharacter_GetIndex(target);
					target->facing = ioCharacter->facing + animation->throwInfo->relativeThrowFacing;
					UUmTrig_ClipHigh(target->facing);

					target->flags |= ONcCharacterFlag_BeingThrown;
				}
			}
		}
	}

	if (!ONrCharacter_IsIdle(ioCharacter)) {
		ONrCharacter_NotIdle(ioCharacter);

	} else if (ONcAnimType_Idle == ioActiveCharacter->curAnimType) {
		ONrCharacter_RecalculateIdleDelay(ioCharacter);

		// set up so that our idle anim will play again in idleRestartDelay ticks
		ioCharacter->lastActiveAnimationTime = ONgGameState->gameTime - ioCharacter->idleDelay;		// this would mean play now
		ioCharacter->lastActiveAnimationTime += ioCharacter->characterClass->idleRestartDelay +		// add random time
			((ioCharacter->characterClass->idleRestartDelay * UUrRandom()) >> (UUcRandomBits + 1));
	}

	if ((ioCharacter->flags & ONcCharacterFlag_Dead_1_Animating) && (!ONrCharacter_IsVictimAnimation(ioCharacter))) {
		ONrCharacter_Die_2_Moving_Hook(ioCharacter);
	}

	if (ONrAnimation_IsFlying(ioActiveCharacter->animation)) {
		ioActiveCharacter->pleaseJump = UUcTrue;

		if (ioActiveCharacter->inAirControl.numFramesInAir == 0) {
			// COrConsole_Printf("jump_start mark flying %f", ioCharacter->physics->position.y);
			ioActiveCharacter->inAirControl.jump_height = ioActiveCharacter->physics->position.y;
		}
	}

	if ((0 == ioActiveCharacter->fightFramesRemaining) && !hasFightVarient) {
		ioActiveCharacter->animVarient &= ~ONcAnimVarientMask_Fight;
	}

	ONiCharacter_ClearHits(ioCharacter, ioActiveCharacter);

	// set up particles for this animation
	ioActiveCharacter->activeParticles = 0;
	TRrAnimation_GetParticles(ioActiveCharacter->animation, &ioActiveCharacter->numParticles, &ioActiveCharacter->animParticles);
	if (ioActiveCharacter->numParticles > 0) {
		UUtUns32 itr, itr2, health, max_health;
		ONtCharacterParticleInstance *particle;
		TRtParticle *anim_particle;
		ONtCharacterParticleArray *particle_array = ioCharacter->characterClass->particles;
		IMtShade tint;

		if (particle_array == NULL) {
			ioActiveCharacter->numParticles = 0;
		} else {
			if (ioActiveCharacter->throwTarget != NULL) {
				// tint by the throw target's health
				health = ioActiveCharacter->throwTarget->hitPoints;
				max_health = ioActiveCharacter->throwTarget->maxHitPoints;
			} else {
				// tint by our health
				health = ioCharacter->hitPoints;
				max_health = ioCharacter->maxHitPoints;
			}

			tint = ONiCharacter_GetAnimParticleTint(ioCharacter, ioActiveCharacter);
			P3rSetGlobalTint(tint);

			for (itr = 0, particle = ioActiveCharacter->particles, anim_particle = ioActiveCharacter->animParticles;
					itr < ioActiveCharacter->numParticles; itr++, particle++, anim_particle++) {

				// find this particle in our list of particle definitions
				for (itr2 = 0; itr2 < particle_array->numParticles; itr2++) {
					if (strcmp(particle_array->particle[itr2].name, anim_particle->particleName) == 0) {
						break;
					}
				}

				if (itr2 >= particle_array->numParticles) {
					// can't find this particle
					COrConsole_Printf("%s: no particle found for anim %s particle '%s'", ioCharacter->player_name,
						TMrInstance_GetInstanceName(ioActiveCharacter->animation), anim_particle->particleName);
					UUrMemory_Clear(particle, sizeof(ONtCharacterParticleInstance));
					continue;
				}

				// set up a link to the particle definition in our animation
				particle->particle_def = &particle_array->particle[itr2];
				particle->particle = NULL;
				particle->particle_ref = P3cParticleReference_Null;
				particle->bodypart_index = anim_particle->partIndex;

				if (particle->particle_def->particle_class != NULL) {
					// create the particle
					particle->particle = ONiCharacter_AttachParticle(ioCharacter, ioActiveCharacter,
															particle->particle_def->particle_class, particle->bodypart_index);
				}

				if (particle->particle != NULL) {
					particle->particle_ref = particle->particle->header.self_ref;
				}
			}

			P3rClearGlobalTint();
		}
	}

	return;
}

typedef struct ONtSpecificThrow
{
	const TRtAnimation *srcThrow;
	const TRtAnimation *dstThrow;
	UUtUns32 score;
	float facingOffset;
} ONtSpecificThrow;

static void AttemptSpecificThrow(
		ONtCharacter		*inSrcCharcter,
		ONtActiveCharacter	*inSrcActiveCharacter,
		ONtCharacter		*inDstCharacter,
		ONtActiveCharacter	*inDstActiveCharacter,
		float				inDistanceSquared,
		TRtAnimation		*inThrow,
		ONtSpecificThrow	*ioSpecificThrow)
{
	const TRtAnimation *targetThrow;
	UUtUns32 score;

	if (UUmSQR(TRrAnimation_GetThrowDistance(inThrow)) >= inDistanceSquared) {
		targetThrow =
			TRrCollection_Lookup(
			inSrcCharcter->characterClass->animations,
			inThrow->throwInfo->throwType,
			inDstActiveCharacter->nextAnimState,
			inDstActiveCharacter->animVarient | ONcAnimVarientMask_Fight);

		if (targetThrow != NULL) {
			UUtUns16 target_varient = TRrAnimation_GetVarient(targetThrow);

			if (0 == (target_varient & ONcAnimVarientMask_Fight)) {
				COrConsole_Printf("animation %s requires the fight varient", targetThrow->instanceName);
			}

			score = (TRrAnimation_GetVarient(targetThrow) << 16) | (TRrAnimation_GetVarient(inThrow) << 0);

			if (score > ioSpecificThrow->score) {
				ioSpecificThrow->srcThrow = inThrow;
				ioSpecificThrow->dstThrow = targetThrow;
				ioSpecificThrow->score = score;
			}
		}
	}

	return;
}

static void ONrPerformSpecificThrow(
	ONtCharacter *ioCharacter,
	ONtActiveCharacter *ioActiveCharacter,
	ONtCharacter *ioTarget,
	ONtActiveCharacter *ioActiveTarget,
	ONtSpecificThrow *inThrow)
{
	ioActiveTarget->thrownBy = ONrCharacter_GetIndex(ioCharacter);

	ioCharacter->facing += inThrow->facingOffset;
	UUmTrig_Clip(ioCharacter->facing);

	ioActiveCharacter->throwTarget = ioTarget;
	ioActiveCharacter->targetThrow = inThrow->dstThrow;

	ONrCharacter_FightMode(ioCharacter);

	return;
}

static const TRtAnimation *AttemptThrow(
	ONtCharacter *ioCharacter,
	ONtActiveCharacter *ioActiveCharacter,
	TRtAnimType inFrontAnimType,
	TRtAnimType inBehindAnimType)
{
	const TRtAnimation *result = NULL;
	ONtCharacter *target;
	ONtActiveCharacter *target_active;
	TRtAnimType animType;
	float throw_target_diff_facing;
	float distance_squared;
	ONtSpecificThrow specificThrow;

	// search through the characters, finding the best target
	// test him
	// throw him (if required)

	UUrMemory_Clear(&specificThrow, sizeof(specificThrow));
	target = ONiCharacter_FindNearestOpponent(ioCharacter, 0.f, 24.f, 45.f * (M3c2Pi / 360.f), &specificThrow.facingOffset, UUcTrue);

	if (target == NULL) {
		goto exit;
	}

	if (ONrCharacter_IsKnockdownResistant(target)) {
		// the target is unthrowable
		goto exit;
	}

	target_active = ONrForceActiveCharacter(target);
	if (target_active == NULL) {
		goto exit;
	}

	{
		UUtBool target_is_invulnerable;

		target_is_invulnerable = TRrAnimation_TestFlag(target_active->animation, ONcAnimFlag_Invulnerable);
		target_is_invulnerable = target_is_invulnerable || TRrAnimation_IsInvulnerable(target_active->animation, target_active->animFrame);

		if (target_is_invulnerable) {
			goto exit;
		}
	}

	throw_target_diff_facing = target->facing - ioCharacter->facing;
	UUmTrig_ClipLow(throw_target_diff_facing);

	if (throw_target_diff_facing > M3cPi) {
		throw_target_diff_facing -= M3c2Pi;
	}

	if ((float)fabs(throw_target_diff_facing) > M3cHalfPi) {
		animType = inFrontAnimType;
	}
	else {
		animType = inBehindAnimType;
	}

	distance_squared = MUrPoint_Distance_Squared(&ioCharacter->location, &target->location);

	{
		TRtAnimVarient not_my_varients = ~(ioActiveCharacter->animVarient | ONcAnimVarientMask_Fight);
		UUtInt32 itr;
		UUtInt32 count;
		TRtAnimationCollectionPart *collection_entry;
		TRtAnimationCollection *collection = ioCharacter->characterClass->animations;

		for(collection = ioCharacter->characterClass->animations; collection != NULL; collection = TRrCollection_GetRecursive(collection))
		{
			TRrCollection_Lookup_Range(
				collection,
				animType,
				ioActiveCharacter->nextAnimState,
				&collection_entry,
				&count);

			for(itr = 0; itr < count; collection_entry++, itr++) {
				TRtAnimVarient varient_mask = TRrAnimation_GetVarient(collection_entry->animation);

				if ((varient_mask & not_my_varients) != 0) {
					// animation required varients that I do not posses
					continue;
				}

				AttemptSpecificThrow(ioCharacter, ioActiveCharacter, target, target_active, distance_squared, collection_entry->animation, &specificThrow);
			}
		}
	}

	if ((NULL != specificThrow.srcThrow) && (NULL != specificThrow.dstThrow)) {
		ONrPerformSpecificThrow(ioCharacter, ioActiveCharacter, target, target_active, &specificThrow);
	}

	if (NULL != specificThrow.srcThrow) {
		UUmAssertReadPtr(specificThrow.srcThrow, sizeof(*specificThrow.srcThrow));
	}

	result = specificThrow.srcThrow;

exit:
	return result;
}

// based on the current situation, remaps the animation to a new animation
static const TRtAnimation *RemapAnimationHook(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter, const TRtAnimation *animation)
{
	if (NULL == animation) { return NULL; }

	// AI2 shouldn't throw characters unless it is trying to
	if ((ioCharacter->flags & ONcCharacterFlag_AIMovement) && (!ONrCharacter_IsPlayingFilm(ioCharacter))) {
		ONtCharacter *target;
		TRtAnimation *throwanim, *desiredtargetanim;

		if (AI2rExecutor_TryingToThrow(ioCharacter, &target, (const TRtAnimation **)(&throwanim), (const TRtAnimation **)(&desiredtargetanim))) {
			ONtActiveCharacter *target_active = ONrForceActiveCharacter(target);
			ONtSpecificThrow specific_throw;

			// get relative angle to target
			UUrMemory_Clear(&specific_throw, sizeof(specific_throw));
			specific_throw.facingOffset = ONrCharacter_RelativeAngleToCharacter(ioCharacter, target);
			if ((float)fabs(specific_throw.facingOffset) > M3cQuarterPi) {
				COrConsole_Printf("### RemapAnimationHook: AI %s tried to throw %s but facing delta %f > 45 degrees",
								ioCharacter->player_name, target->player_name, specific_throw.facingOffset * M3cRadToDeg);

			} else {
				// attempt the throw
				AttemptSpecificThrow(ioCharacter, ioActiveCharacter, target, target_active,
									MUmVector_GetDistanceSquared(ioCharacter->location, target->location),
									throwanim, &specific_throw);

				if (specific_throw.srcThrow != NULL) {
					// success, perform the throw
					if (specific_throw.dstThrow != desiredtargetanim) {
						COrConsole_Printf("### RemapAnimationHook: AI tried throw-target %s but got %s instead",
										TMrInstance_GetInstanceName(desiredtargetanim), TMrInstance_GetInstanceName(specific_throw.dstThrow));
					}
					ONrPerformSpecificThrow(ioCharacter, ioActiveCharacter, target, target_active, &specific_throw);
					return specific_throw.srcThrow;
				} else {
					COrConsole_Printf("### RemapAnimationHook: AI %s tried to throw %s (%s/%s) but was unable to",
									ioCharacter->player_name, target->player_name,
									TMrInstance_GetInstanceName(throwanim), TMrInstance_GetInstanceName(specific_throw.dstThrow));
				}
			}
		}

		// no throw was found
		return animation;
	}

	// if this is a punch, could be a throw
	if (ONcAnimType_Punch_Forward == animation->type)
	{
		const TRtAnimation *punchThrow;

		punchThrow = AttemptThrow(
			ioCharacter, ioActiveCharacter,
			ONcAnimType_Throw_Forward_Punch,
			ONcAnimType_Throw_Behind_Punch);

		if (NULL != punchThrow) {
			animation = punchThrow;
		}
	}

	// if this is a kick, could be a kick throw
	else if (ONcAnimType_Kick_Forward == animation->type)
	{
		const TRtAnimation *kickThrow;

		kickThrow = AttemptThrow(
			ioCharacter, ioActiveCharacter,
			ONcAnimType_Throw_Forward_Kick,
			ONcAnimType_Throw_Behind_Kick);

		if (NULL != kickThrow) {
			animation = kickThrow;
		}
	}

	// if this is a running punch, could be a running punch throw
	else if (ONcAnimType_Run_Punch == animation->type)
	{
		const TRtAnimation *runPunchThrow;

		runPunchThrow = AttemptThrow(
			ioCharacter, ioActiveCharacter,
			ONcAnimType_Run_Throw_Forward_Punch,
			ONcAnimType_Run_Throw_Behind_Punch);

		if (NULL != runPunchThrow) {
			animation = runPunchThrow;
		}
	}

	// if this is a running kick, could be a running kick throw
	else if (ONcAnimType_Run_Kick == animation->type)
	{
		const TRtAnimation *runKickThrow;

 		runKickThrow = AttemptThrow(
			ioCharacter, ioActiveCharacter,
			ONcAnimType_Run_Throw_Forward_Kick,
			ONcAnimType_Run_Throw_Behind_Kick);

		if (NULL != runKickThrow) {
			animation = runKickThrow;
		}
	}

	return animation;
}

static void ONiCharacter_OldAnimationHook(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter, ONtFinalRotationMode inFinalRotationMode,
                                          const TRtAnimation *nextAnimation)
{
	float finalRotation = 0.f;
	float current_time = ONrGameState_GetGameTime() / ((float) UUcFramesPerSecond);
	UUtUns32 itr;
	ONtCharacterParticleInstance *particle;

	if (ioCharacter->flags & ONcCharacterFlag_ChrAnimate) {
		if (0 == ioActiveCharacter->animationLockFrames) {
			ioCharacter->flags &= ~ONcCharacterFlag_ChrAnimate;
		}
	}

//	UUmAssert((TRrAnimation_GetDuration(ioCharacter->animation) == ioCharacter->animFrame) || (!TRrAnimation_TestFlag(ioCharacter->animation, ONcAnimFlag_ThrowTarget)));

	if ((ioActiveCharacter->curAnimType == ioActiveCharacter->lastAttack) &&
		(ioActiveCharacter->lastAttackTime == ONcLastAttackTime_StillActive))
	{
		ioActiveCharacter->lastAttackTime = ONgGameState->gameTime;
	}

	if (inFinalRotationMode != ONcIgnoreFinalRotation) {
		finalRotation = TRrAnimation_GetFinalRotation(ioActiveCharacter->animation);
		UUmTrig_Clip(finalRotation);
	}

	if (0 != finalRotation) {
		// ok we need to patch up the quaternion if we are doing stitching...
		M3tQuaternion *rootQuat1 = ioActiveCharacter->curOrientations + 0;
		M3tQuaternion *rootQuat2 = ioActiveCharacter->curAnimOrientations + 0;
		M3tQuaternion patchQuat;

		// CB: this is much faster than using MUrXYZEulerToQuat
		patchQuat.x = 0.0f;
		patchQuat.y = MUrSin(finalRotation * 0.5f);
		patchQuat.z = 0.0f;
		patchQuat.w = MUrCos(finalRotation * 0.5f);
		MUmQuat_VerifyUnit(&patchQuat);

		MUrQuat_Mul(rootQuat1, &patchQuat, rootQuat1);
		MUrQuat_Mul(rootQuat2, &patchQuat, rootQuat2);

		// now adjust the facing
		ONrCharacter_AdjustFacing(ioCharacter, finalRotation);
	}

	// kill any particles created by this animation
	for (itr = 0, particle = ioActiveCharacter->particles; itr < ioActiveCharacter->numParticles; itr++, particle++) {

		// don't delete particles that are already dead
		if ((particle->particle == NULL) || (particle->particle->header.self_ref != particle->particle_ref))
			continue;

		UUmAssertReadPtr(particle->particle_def, sizeof(TRtParticle));
		UUmAssertReadPtr(particle->particle_def->particle_class, sizeof(P3tParticleClass));

		if (ioActiveCharacter->activeParticles & (1 << itr)) {
			// deactivate this particle
			P3rSendEvent(particle->particle_def->particle_class, particle->particle, P3cEvent_Stop,
						current_time);
		}

		P3rKillParticle(particle->particle_def->particle_class, particle->particle);
		particle->particle = NULL;
	}
	ioActiveCharacter->numParticles = 0;
	ioActiveCharacter->activeParticles = 0;

	return;
}

void ONrCharacter_GetPartLocation(const ONtCharacter *inCharacter, ONtActiveCharacter *inActiveCharacter,
								  UUtUns16 inPartIndex, M3tPoint3D *outLocation)
{
	UUmAssertReadPtr(inCharacter, sizeof(*inCharacter));
	UUmAssertReadPtr(inActiveCharacter, sizeof(*inActiveCharacter));
	UUmAssertWritePtr(outLocation, sizeof(*outLocation));

	*outLocation = MUrMatrix_GetTranslation(&inActiveCharacter->matricies[inPartIndex]);

	return;
}

// at some point this function could become more sophisticated and sample every frames velocity,
// keep an array of the last four and so on
void ONrCharacter_GetVelocityEstimate(const ONtCharacter *inCharacter, ONtActiveCharacter *ioActiveCharacter, M3tVector3D *outVelocity)
{
#define cMaxSampleFrames ((UUtUns8) 4)
	TRtAnimTime maxTime = TRrAnimation_GetDuration(ioActiveCharacter->animation) - 1;
	TRtAnimTime animTime;
	UUtUns8 sampleSize;
	UUtUns8 itr;
	M3tVector3D curSample;
	M3tVector3D runningTotal;

	UUmAssertReadPtr(inCharacter, sizeof(*inCharacter));
	UUmAssertWritePtr(outVelocity, sizeof(*outVelocity));

	runningTotal.x = 0;
	runningTotal.y = 0;
	runningTotal.z = 0;
	sampleSize = 0;
	animTime = UUmMin(ioActiveCharacter->animFrame, maxTime);
	for(itr = 0; itr < cMaxSampleFrames; itr++) {
		TRrAnimation_GetPosition(ioActiveCharacter->animation, animTime, &curSample);
		MUmVector_Add(runningTotal, runningTotal, curSample);
		sampleSize++;

		if (0 == animTime) {
			break;
		}

		animTime--;
	}

	MUmVector_Scale(runningTotal, 1.f / sampleSize);

	*outVelocity = runningTotal;

	UUmAssert((outVelocity->x < 1e9f) && (outVelocity->x > -1e9f));
	UUmAssert((outVelocity->y < 1e9f) && (outVelocity->y > -1e9f));
	UUmAssert((outVelocity->z < 1e9f) && (outVelocity->z > -1e9f));

	return;
}

void ONrCharacter_FightMode(ONtCharacter *ioCharacter)
{
	ONtActiveCharacter *active_character = ONrForceActiveCharacter(ioCharacter);

	if (active_character == NULL) {
		return;
	}

	UUmAssert(0 == (ioCharacter->characterClass->fightModeDuration & 0xffff0000));

	active_character->fightFramesRemaining = (UUtUns16) ioCharacter->characterClass->fightModeDuration;
	active_character->animVarient |= ONcAnimVarientMask_Fight;

	return;
}

void ONrCharacter_ExitFightMode(ONtCharacter *ioCharacter)
{
	ONtActiveCharacter *active_character = ONrForceActiveCharacter(ioCharacter);

	if (active_character == NULL) {
		return;
	}

	active_character->fightFramesRemaining = 0;
	active_character->animVarient &= ~ONcAnimVarientMask_Fight;

	return;
}


// if there is a better varient match for this animation, switch to it
void ONrCharacter_UpdateAnimationForVarient(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	const TRtAnimation *newAnimation;
	TRtAnimVarient newVarient;
	TRtAnimVarient oldVarient;
	UUtBool ok_to_update_for_varient = UUcTrue;

	UUmAssertWritePtr(ioCharacter, sizeof(*ioCharacter));

	if (ONrAnimType_IsThrowSource(ioActiveCharacter->curAnimType)) {
		// ok, this is lame but I am just going to work around
		// you cannot look up throws with TRrCollection_Lookup
		// you have to use TRrCollection_Lookup_Range

		goto exit;
	}

	newAnimation = TRrCollection_Lookup(ioCharacter->characterClass->animations, ioActiveCharacter->curAnimType,
										ioActiveCharacter->curFromState, ioActiveCharacter->animVarient);

	if (newAnimation) {
		oldVarient = TRrAnimation_GetVarient(ioActiveCharacter->animation);
		newVarient = TRrAnimation_GetVarient(newAnimation);

		if (newVarient > oldVarient) {
			ONrCharacter_Stitch(ioCharacter, ioActiveCharacter, ioActiveCharacter->curFromState, newAnimation, 8);
		}
	}

exit:
	return;
}

UUtBool ONrAnimType_IsLandType(TRtAnimType type)
{
	UUtBool land;

	switch(type)
	{
		case ONcAnimType_Land:
		case ONcAnimType_Land_Forward:
		case ONcAnimType_Land_Right:
		case ONcAnimType_Land_Left:
		case ONcAnimType_Land_Back:
			land = UUcTrue;
		break;

		default:
			land = UUcFalse;
		break;
	}

	return land;
}

void ONiOrientations_Verify(ONtCharacter *inCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUtUns16 itr;

	for(itr = 0; itr < ONcNumCharacterParts; itr++) {
		MUmQuat_VerifyUnit(ioActiveCharacter->curOrientations + itr);
		MUmQuat_VerifyUnit(ioActiveCharacter->curAnimOrientations + itr);
	}

	return;
}

void
ONrCharacter_SetHitPoints(
	ONtCharacter		*ioCharacter,
	UUtUns32			inHitPoints)
{
	UUmAssert(ioCharacter);

	ioCharacter->hitPoints = inHitPoints;

	if ((inHitPoints > 0) && (ioCharacter->flags & ONcCharacterFlag_Dead_1_Animating))
	{
		ONrCharacter_Live_Hook(ioCharacter);
	}
	else if ((inHitPoints <= 0) && !(ioCharacter->flags & ONcCharacterFlag_Dead))
	{
		ONrCharacter_Die(ioCharacter);
	}

	return;
}

TRtBody *ONrCharacter_GetBody(ONtCharacter *inCharacter, TRtBodySelector inWhichBody)
{
	ONtCharacterClass *characterClass = inCharacter->characterClass;
	TRtBodySet *set = characterClass->body;

	return set->body[inWhichBody];
}

const TRtBody *ONrCharacter_GetBody_Const(const ONtCharacter *inCharacter, TRtBodySelector inWhichBody)
{
	const ONtCharacterClass *characterClass = inCharacter->characterClass;
	const TRtBodySet *set = characterClass->body;

	return set->body[inWhichBody];
}

UUtUns16 ONrCharacter_GetNumParts(const ONtCharacter *inCharacter)
{
	return ONcNumCharacterParts;
}


// hit control stuff
static void ONiCharacter_ClearHits(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUrMemory_Clear(ioActiveCharacter->hitTables, sizeof(ONtHitTable) * TRcMaxAttacks);

	return;
}

static UUtBool ONiCharacter_TestAndSetHit(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter,
										  const ONtCharacter *inTarget, UUtUns32 inAttack)
{
	UUtUns8 itr;
	ONtHitTable *table = ioActiveCharacter->hitTables + inAttack;

	for(itr = 0; itr < ONcMaxHitsPerAttack; itr++)
	{
		if (ONcCharacterIndex_None == table->hits[itr])
		{
			table->hits[itr] = inTarget->index;
			return UUcFalse;
		}
		else if (inTarget->index == table->hits[itr])
		{
			return UUcTrue;
		}
	}

	return UUcTrue;
}

void ONrCharacter_SetAnimation_External(
	ONtCharacter *ioCharacter,
	TRtAnimState fromState,
	const TRtAnimation *inNewAnim,
	UUtInt32 inStitchCount)
{
	ONtActiveCharacter *active_character = ONrForceActiveCharacter(ioCharacter);

	if (active_character == NULL)
		return;

	inNewAnim = RemapAnimationHook(ioCharacter, active_character, inNewAnim);

	ONiCharacter_OldAnimationHook(ioCharacter, active_character, ONcApplyFinalRotation, inNewAnim);

	if (inStitchCount > 0) {
		ONrCharacter_Stitch(ioCharacter, active_character, fromState, inNewAnim, inStitchCount);
	}
	else {
		ONrCharacter_SetAnimationInternal(ioCharacter, active_character, fromState, ONcAnimType_None, inNewAnim);
	}

	ONrCharacter_NewAnimationHook(ioCharacter, active_character);

	return;
}


const ONtAirConstants *ONrCharacter_GetAirConstants(const ONtCharacter *inCharacter)
{
	const ONtAirConstants *constants;

	constants = &inCharacter->characterClass->airConstants;

	return constants;
}


void
ONrCharacter_Callback_PreDispose(
	PHtPhysicsContext *ioContext)
{
	ioContext->sphereTree = NULL;

	return;
}

static void ONiCharacter_RunDeathAnimation(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter, TRtAnimType inAnimType)
{
	if ((NULL != ioCharacter) && (NULL != ioActiveCharacter)) {
		UUtBool character_to_is_ground = ONrAnimState_IsFallen(ioActiveCharacter->nextAnimState);
		UUtBool character_is_on_ground = (character_to_is_ground && (ioActiveCharacter->curFromState == ioActiveCharacter->nextAnimState));
		UUtBool already_running_victim = ONrAnimType_IsVictimType(ioActiveCharacter->curAnimType);

		// if we are on the ground, not running a victim animation or not heading to ground do a knockdown
		if (character_is_on_ground || (!already_running_victim) || (!character_to_is_ground)) {
			if (ONrAnimType_IsVictimType(inAnimType)) {
				ONrCharacter_Knockdown(NULL, ioCharacter, inAnimType);
			}
			else {
				ONrCharacter_Knockdown(NULL, ioCharacter, ONcAnimType_Knockdown_Head);
			}
		}
	}

	return;
}

static void ONrCharacter_Die_1_Animating_Hook(ONtCharacter *ioCharacter, ONtCharacter *inKiller)
{
	ONtActiveCharacter *active_character;

	UUmAssertReadPtr(ioCharacter, sizeof(*ioCharacter));
	UUmAssert(0 == ioCharacter->hitPoints);
	UUmAssert(0 == (ioCharacter->flags & ONcCharacterFlag_Dead_1_Animating));
	UUmAssert(0 == (ioCharacter->flags & ONcCharacterFlag_Dead_2_Moving));
	UUmAssert(0 == (ioCharacter->flags & ONcCharacterFlag_Dead_3_Cosmetic));
	UUmAssert(0 == (ioCharacter->flags & ONcCharacterFlag_Dead_4_Gone));

	// make absolutely sure that this character has been defeated
	ONiCharacter_Defeated(ioCharacter);

	// tell the AI2 system that a character is dying
	AI2rNotify_Death(ioCharacter, inKiller);

	if (ioCharacter->charType == ONcChar_AI2) {
		// delete this AI
		AI2rDeleteState(ioCharacter);

		// check to make sure that deleting this character hasn't broken the
		// AI knowledgebase
		AI2rKnowledge_Verify();
	}

	// we must be active in order to die!
	active_character = ONrForceActiveCharacter(ioCharacter);

	if (NULL == active_character) {
		ioCharacter->flags |= ONcCharacterFlag_NoCorpse;
	}

	ONrGameState_LivingCharacterList_Remove(ioCharacter);

	ONrCharacter_DropWeapon(ioCharacter,UUcTrue,WPcPrimarySlot,UUcFalse);

	ONrSpeech_Stop(ioCharacter, UUcTrue);

	if (ONcChar_Player != ioCharacter->charType) {
		ONiCharacter_DropInventoryItems(ioCharacter);
	}

	if (ioCharacter->characterClass->death_particleclass != NULL) {
		// create our death particle effect
		ONiCharacter_AttachParticle(ioCharacter, active_character, ioCharacter->characterClass->death_particleclass, ONcPelvis_Index);
	}

	ioCharacter->flags |= ONcCharacterFlag_Dead_1_Animating;
	ioCharacter->time_of_death = ONgGameState->gameTime;
	ioCharacter->death_delete_timer = ioCharacter->characterClass->death_deletedelay;

	// stop any super particle effects
	ioCharacter->super_amount = 0.0f;

	if (ioCharacter->scripts.die_name[0] != '\0') {
		// try to run our death script right now. this might result in the script being deferred if
		// we are inside a region where scripts are locked, e.g. the character collision loop for melee
		ioCharacter->scripts.flags |= AIcScriptTableFlag_DieCalled;
		ONrCharacter_Defer_Script(ioCharacter->scripts.die_name, ioCharacter->player_name, "death");
		ONrCharacter_Commit_Scripts();
	}

	if (NULL == active_character) {
		COrConsole_Printf("killing %s but failed to make active", ioCharacter->player_name);
	}
	else {
		active_character->pleaseFire = UUcFalse;
		active_character->pleaseFireAlternate = UUcFalse;
		active_character->pleaseAutoFire = UUcFalse;
		active_character->pleaseContinueFiring = UUcFalse;

		ONiCharacter_RunDeathAnimation(ioCharacter, active_character, TRcAnimType_None);
	}

	return;
}

static void ONrCharacter_Die_2_Moving_Hook(ONtCharacter *ioCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);

	UUmAssert(0 == (ioCharacter->flags & ONcCharacterFlag_Dead_2_Moving));
	UUmAssert(0 == (ioCharacter->flags & ONcCharacterFlag_Dead_3_Cosmetic));
	UUmAssert(0 == (ioCharacter->flags & ONcCharacterFlag_Dead_4_Gone));

	//COrConsole_Printf("character %d death stage 2", ioCharacter->index);

	if (0 == (ioCharacter->flags & ONcCharacterFlag_Dead_1_Animating))
	{
		ONrCharacter_Die_1_Animating_Hook(ioCharacter, NULL);
	}

	if (NULL != active_character) {
		if (active_character->inAirControl.numFramesInAir > 0) {
			active_character->death_velocity = active_character->inAirControl.velocity;
		}
		else {
			active_character->death_velocity = active_character->knockback;
		}

		active_character->death_still_timer = 0;
		active_character->death_moving_timer = 0;
	}

	ioCharacter->flags |= ONcCharacterFlag_Dead_2_Moving;

	ONrCharacter_BuildPhysicsFlags(ioCharacter, active_character);

	return;
}

static void ONrCharacter_Die_3_Cosmetic_Hook(ONtCharacter *ioCharacter)
{
	UUmAssert(0 == (ioCharacter->flags & ONcCharacterFlag_Dead_3_Cosmetic));
	UUmAssert(0 == (ioCharacter->flags & ONcCharacterFlag_Dead_4_Gone));

	// don't make the player drop their holstered weapon, because it looks bad
	if (ioCharacter->charType != ONcChar_Player) {
		// drop a weapon if we have one
		ONrCharacter_DropWeapon(ioCharacter, UUcFalse, WPcAllSlots, UUcFalse);
	}

//	ONrGameState_PresentCharacterList_Remove(ioCharacter);

	ioCharacter->flags |= ONcCharacterFlag_Dead_3_Cosmetic;

	ONrCharacter_DisablePhysics(ioCharacter);

	if (!ONrCharacter_IsActive(ioCharacter)) {
		// delete this character immediately
		ONrCharacter_Die_4_Gone_Hook(ioCharacter);
	}

	return;
}

static void ONrCharacter_Die_4_Gone_Hook(ONtCharacter *ioCharacter)
{
	if (ioCharacter->flags & ONcCharacterFlag_DeathLock) {
		return;
	}

	UUmAssert(0 == (ioCharacter->flags & ONcCharacterFlag_Dead_4_Gone));

	if (0 == (ioCharacter->flags & ONcCharacterFlag_Dead_3_Cosmetic)) {
		ONrCharacter_Die_3_Cosmetic_Hook(ioCharacter);
	}

	if (ioCharacter->death_delete_timer > 0) {
		// we are waiting to be deleted... don't turn us into a corpse, which couldn't
		// be deleted
		return;
	}

	{
		UUtBool make_a_corpse = !(ioCharacter->flags & ONcCharacterFlag_NoCorpse);

		if (make_a_corpse) {
			ONrCorpse_Create(ioCharacter);
		}

		ONrGameState_DeleteCharacter(ioCharacter);
	}

	return;
}


static void ONrCharacter_Live_Hook(ONtCharacter *ioCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);

	ONrGameState_PresentCharacterList_Add(ioCharacter);
	ONrGameState_LivingCharacterList_Add(ioCharacter);

	if ((active_character != NULL) && (active_character->physics == NULL))
	{
		ONrCharacter_EnablePhysics(ioCharacter);
		UUmAssert(NULL != active_character->physics);
	}

	ioCharacter->flags &= ~(
		ONcCharacterFlag_Dead_1_Animating |
		ONcCharacterFlag_Dead_2_Moving |
		ONcCharacterFlag_Dead_3_Cosmetic |
		ONcCharacterFlag_Dead_4_Gone);

	ONrCharacter_BuildPhysicsFlags(ioCharacter, active_character);

	return;
}

static UUtBool ONrCharacter_Collide_With_Part(UUtUns16 inPart)
{
	UUtBool collide;

	return UUcTrue;

	switch(inPart)
	{
		case ONcLLeg_Index:
		case ONcLLeg1_Index:
		case ONcLFoot_Index:
		case ONcRLeg_Index:
		case ONcRLeg1_Index:
		case ONcRFoot_Index:

		case ONcLArm_Index:
		case ONcLArm1_Index:
		case ONcLArm2_Index:
		case ONcLHand_Index:
		case ONcRArm_Index:
		case ONcRArm1_Index:
		case ONcRArm2_Index:
		case ONcRHand_Index:
			collide = UUcFalse;
		break;

		default:
			collide = UUcTrue;
	}

	return collide;
}

static float iHighest_Quad_Point(
	UUtUns16				inGQIndex,
	const AKtEnvironment	*inEnvironment)
{
	float y0,y1,y2,y3;
	float yMax;
	const M3tQuad *quad = &(inEnvironment->gqGeneralArray->gqGeneral[inGQIndex].m3Quad.vertexIndices);

	y0 = (inEnvironment->pointArray->points + quad->indices[0])->y;
	y1 = (inEnvironment->pointArray->points + quad->indices[1])->y;
	y2 = (inEnvironment->pointArray->points + quad->indices[2])->y;
	y3 = (inEnvironment->pointArray->points + quad->indices[3])->y;

	yMax = UUmMax4(y0,y1,y2,y3);

	return yMax;
}

static void
ONrCharacter_Callback_FindEnvCollisions(
	const PHtPhysicsContext *inContext,
	const AKtEnvironment *inEnvironment,
	const M3tVector3D *inVelocity,
	PHtCollider *outColliders,
	UUtUns16 *ioNumColliders)
{
	M3tVector3D localVelocity = *inVelocity;
	ONtCharacter *character = (ONtCharacter *) inContext->callbackData;
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(character);
	UUtUns16 itr;
	UUtBool is_floor;
	char *level_name = ONgGameState->level->name;

	UUmAssertReadPtr(active_character, sizeof(ONtActiveCharacter));

	AKrCollision_Sphere(inEnvironment, &inContext->sphereTree->sphere, &localVelocity, AKcGQ_Flag_Chr_Col_Skip,	AKcMaxNumCollisions);

	for(itr = 0; itr < AKgNumCollisions; itr++)
	{
		AKtCollision *curCollision = AKgCollisionList + itr;
		AKtGQ_General *gqGeneral = inEnvironment->gqGeneralArray->gqGeneral + curCollision->gqIndex;
		AKtGQ_Collision *gqCollision = inEnvironment->gqCollisionArray->gqCollision + curCollision->gqIndex;
		M3tPlaneEquation plane;
		PHtCollider *newCollider;
		UUtBool collide;
		M3tPoint3D intersection;

		collide = PHrCollision_Quad_SphereTreeVector(
			inEnvironment,	curCollision->gqIndex, active_character->sphereTree, &localVelocity, &intersection);

		if (!collide) { continue; }

		AKmPlaneEqu_GetComponents(gqCollision->planeEquIndex, inEnvironment->planeArray->planes, plane.a, plane.b, plane.c, plane.d);

		newCollider = outColliders + *ioNumColliders;
		UUrMemory_Clear(newCollider, sizeof(*newCollider));

		newCollider->type = PHcCollider_Env;
		newCollider->data = gqGeneral;
		newCollider->plane = plane;
		newCollider->planePoint = curCollision->collisionPoint;

		if (gConsole_show_chr_env_collision) {
			gqGeneral->flags |= AKcGQ_Flag_Draw_Flash;
		}

		(*ioNumColliders)++;

		is_floor = (plane.b > ONcMinFloorNormalY);

		if (is_floor)
		{
//			character->floorCount++;
		}
		else
		{
			active_character->numWallCollisions++;
			if (character->flags & ONcCharacterFlag_AIMovement)
			{
				AI2rMovementState_NotifyWallCollision(character, &active_character->movement_state,
														curCollision->gqIndex, gqCollision->planeEquIndex,
														&newCollider->planePoint, &newCollider->plane);
			}
		}

//		character->collisionCount++;


		if (gqGeneral->flags & AKcGQ_Flag_2Sided)
		{
			PHtCollider *secondNewCollider = outColliders + *ioNumColliders;

			*secondNewCollider = *newCollider;

			secondNewCollider->plane.a *= -1.f;
			secondNewCollider->plane.b *= -1.f;
			secondNewCollider->plane.c *= -1.f;
			secondNewCollider->plane.d *= -1.f;

			if ((!is_floor) && (character->flags & ONcCharacterFlag_AIMovement))
			{
				// add a wall collision with the reverse of this plane (0x80000000 denotes a reversed plane index)
				AI2rMovementState_NotifyWallCollision(character, &active_character->movement_state,
														curCollision->gqIndex, (gqCollision->planeEquIndex ^ 0x80000000),
														&secondNewCollider->planePoint, &secondNewCollider->plane);
			}

			(*ioNumColliders)++;
		}
	}

	return;
}

static int iSortEnvColVert(const void *elem1, const void *elem2)
{
	int result;
	const AKtCollision *col1 = elem1;
	const AKtCollision *col2 = elem2;

	if (col1->collisionPoint.y < col2->collisionPoint.y)
	{
		result = -1;
	}
	else if (col1->collisionPoint.y > col2->collisionPoint.y)
	{
		result = 1;
	}
	else
	{
		result = 0;
	}

	return result;
}

static UUtBool GetQuadRangeClipped(UUtInt32 inNumPoints, const M3tPoint3D **inPoints, float inMinX, float inMaxX, float inMinZ, float inMaxZ, float inMaxY, float *outHeight);

static void ONrCharacter_Land(ONtCharacter *inCharacter, ONtActiveCharacter *ioActiveCharacter, float inLocationOfImpact)
{
	ONtCharacterClass *character_class = inCharacter->characterClass;
	UUtBool is_landing = ONrAnimType_IsLandType(TRrAnimation_GetType(ioActiveCharacter->animation));
	float falling_height;
	float falling_damage;
	UUtUns32 int_falling_damage;

	falling_height = ioActiveCharacter->inAirControl.jump_height - inLocationOfImpact;
	int_falling_damage = 0;

	if (falling_height > character_class->airConstants.damage_end) {
		falling_damage = UUmMax(1000.f, inCharacter->hitPoints);
	}
	else if (falling_height < character_class->airConstants.damage_start) {
		falling_damage = 0;
	}
	else {
		float falling_delta;
		float fall_percent;

		falling_delta = character_class->airConstants.damage_end - character_class->airConstants.damage_start;
		fall_percent = (falling_height - character_class->airConstants.damage_start) / falling_delta;
		falling_damage = fall_percent * inCharacter->maxHitPoints;
	}

	if (falling_damage > 0.f) {
		int_falling_damage = MUrUnsignedSmallFloat_To_Uns_Round(falling_damage);
	}

	if (int_falling_damage > 0) {
		float falling_knockback = 0.f;

		ONrCharacter_TakeDamage(
			inCharacter,
			int_falling_damage,
			falling_knockback,
			NULL,
			NULL,
			WPcDamageOwner_Falling,
			ONcAnimType_Land_Dead);
	}

	ioActiveCharacter->inAirControl.velocity.x = 0;
	ioActiveCharacter->inAirControl.velocity.y = 0;
	ioActiveCharacter->inAirControl.velocity.z = 0;
	ioActiveCharacter->inAirControl.numFramesInAir = 0;
	ioActiveCharacter->pleaseLand = UUcTrue;

	if (int_falling_damage > 0) {
		ioActiveCharacter->pleaseLandHard = UUcTrue;
	}

	return;
}


static UUtBool FindGroundCollision(ONtCharacter *inCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	float width = 1.5f;
	float height;
	UUtUns32 itr;
	M3tPoint3D velocity;
	AKtEnvironment *environment = ONgGameState->level->environment;
	const M3tPoint3D *environment_points = environment->pointArray->points;
	UUtBool hit_ground = UUcFalse;
	float touched_ground_location;
	M3tBoundingSphere sphere;
	UUtBool is_landing = ONrAnimType_IsLandType(TRrAnimation_GetType(ioActiveCharacter->animation));

	float foot_offset = 0;
	float solid_offset = 10.f;
	float feet_location = UUmMax(inCharacter->feetLocation, inCharacter->location.y);
	float maximum_height;
	UUtBool is_in_air;
	UUtInt32 gq_index_of_max_height_gq = -1;

	if ((ioActiveCharacter->inAirControl.numFramesInAir > 0) || is_landing) {
		feet_location = UUmMax(inCharacter->feetLocation, inCharacter->location.y);
		foot_offset = inCharacter->feetLocation - inCharacter->location.y;
		is_in_air = UUcTrue;

		// do clear the last floor gq for players
		ioActiveCharacter->last_floor_gq = -1;
	}
	else {
		feet_location = inCharacter->location.y;
		is_in_air = UUcFalse;
	}

	height = inCharacter->location.y + foot_offset + solid_offset;
	maximum_height = inCharacter->location.y + foot_offset + (inCharacter->heightThisFrame - foot_offset) / 2.f;
	maximum_height = UUmMax(maximum_height, inCharacter->location.y + 6.f);		// we can always bump up 6 units from our max location

	sphere.center = inCharacter->location;
	sphere.center.y = height;
	sphere.radius = 5.f * 1.42f;

	velocity.x = 0.f;
	velocity.z = 0.f;
	velocity.y = ioActiveCharacter->gravityThisFrame;
	velocity.y -= foot_offset + solid_offset;

	if (gChrDebugFall) {
		COrConsole_Printf("ground collision %s frame %d {fh %3.3f max %3.3f feet %3.3f} %3.3f -> %3.3f",
			inCharacter->player_name,
			ONgGameState->gameTime,
			foot_offset,
			height,
			feet_location,
			sphere.center.y,
			sphere.center.y + velocity.y);
	}

	AKrCollision_Sphere(
		ONgGameState->level->environment,
		&sphere,
		&velocity,
		AKcGQ_Flag_Chr_Col_Skip | AKcGQ_Flag_Wall,
		AKcMaxNumCollisions);

	ioActiveCharacter->physics->position.y += ioActiveCharacter->gravityThisFrame;
	feet_location += ioActiveCharacter->gravityThisFrame;
	inCharacter->location = ioActiveCharacter->physics->position;

	if (gChrDebugFall) {
		COrConsole_Printf("looking for collision from %2.2f .. %2.2f (%2.2f)", feet_location, maximum_height, ioActiveCharacter->deferred_maximum_height);
	}

	// bump the maximum height if it changed
	if (maximum_height < (ioActiveCharacter->deferred_maximum_height + 6.f)) {
		maximum_height = ioActiveCharacter->deferred_maximum_height + 6.f;
	}

	ioActiveCharacter->deferred_maximum_height = feet_location;

	for(itr = 0; itr < AKgNumCollisions; itr++)
	{
		AKtCollision *curCollision = AKgCollisionList + itr;
		AKtGQ_General *gqGeneral = environment->gqGeneralArray->gqGeneral + curCollision->gqIndex;
		AKtGQ_Collision *gqCollision = environment->gqCollisionArray->gqCollision + curCollision->gqIndex;
		M3tVector3D gq_plane_normal;
		float gq_plane_d, sphere_d;
		UUtBool gq_is_floor;
		UUtBool gq_is_ceiling;

		AKmPlaneEqu_GetComponents(
			gqCollision->planeEquIndex,
			environment->planeArray->planes,
			gq_plane_normal.x,
			gq_plane_normal.y,
			gq_plane_normal.z,
			gq_plane_d);

		// CB: this is so that characters don't fall through glass floors, etc
		if ((gqGeneral->flags & AKcGQ_Flag_2Sided) && !(gqGeneral->flags & AKcGQ_Flag_Jello)) {
			sphere_d = MUrVector_DotProduct(&gq_plane_normal, &sphere.center);
			if ((sphere_d + gq_plane_d) < 0) {
				// we are on the other side of this quad
				MUmVector_Negate(gq_plane_normal);
				gq_plane_d *= -1;
			}
		}

		gq_is_floor = gq_plane_normal.y > ONcMinFloorNormalY;
		gq_is_ceiling = gq_plane_normal.y < -ONcMinFloorNormalY;

		UUmAssert(gq_is_floor || gq_is_ceiling);

		if (gq_is_ceiling) {
			continue;
		}
		else if (gq_is_floor) {
			const M3tPoint3D *points[4];
			UUtUns32 num_points = (gqGeneral->flags & AKcGQ_Flag_Triangle) ? 3 : 4;
			UUtUns32 point_itr;
			UUtBool clip_success;
			float new_max;
			float debug_min_y = UUcFloat_Max;
			float debug_max_y = UUcFloat_Min;


			for(point_itr = 0; point_itr < num_points; point_itr++)
			{
				UUtUns32 environment_point_index = gqGeneral->m3Quad.vertexIndices.indices[point_itr];

				points[point_itr] = environment_points + environment_point_index;

				debug_min_y = UUmMin(debug_min_y, environment_points[environment_point_index].y);
				debug_max_y = UUmMax(debug_max_y, environment_points[environment_point_index].y);
			}

			if (gChrDebugFall) {
				COrConsole_Printf("searching for floor collision %3.3f .. %3.3f", debug_min_y, debug_max_y);
			}

			{
				float minx = inCharacter->location.x - width;
				float maxx = inCharacter->location.x + width;
				float minz = inCharacter->location.z - width;
				float maxz = inCharacter->location.z + width;
				float maxy = maximum_height;

				clip_success = GetQuadRangeClipped(num_points, points, minx, maxx, minz, maxz, maxy, &new_max);

				if (clip_success) {
					if (!hit_ground) {
						touched_ground_location = new_max;
						hit_ground = UUcTrue;
						gq_index_of_max_height_gq = curCollision->gqIndex;
					}
					else {
						if (new_max > touched_ground_location) {
							touched_ground_location = new_max;
							gq_index_of_max_height_gq = curCollision->gqIndex;
						}
					}

					if (gChrDebugFall) {
						COrConsole_Printf("clip success %3.3f", new_max);
					}
				}
				else {
					if (gChrDebugFall) {
						COrConsole_Printf("clip fail");
					}
				}
			}
		}
	}

	if ((hit_ground) && (feet_location > touched_ground_location)) {
		hit_ground = UUcFalse;
	}

	if (hit_ground) {
		float amount_to_move_character = touched_ground_location - feet_location;

		if (gChrDebugFall) {
			COrConsole_Printf("hit ground %3.3f", touched_ground_location);
		}

		ioActiveCharacter->deferred_maximum_height = touched_ground_location;

		ioActiveCharacter->physics->position.y += amount_to_move_character;
		inCharacter->location = ioActiveCharacter->physics->position;
		ioActiveCharacter->last_floor_gq = gq_index_of_max_height_gq;

		// store the point where we touch the ground, for use by pathfinding BNV-node determination code
		MUmVector_Set(ioActiveCharacter->groundPoint, ioActiveCharacter->physics->position.x, touched_ground_location,
						ioActiveCharacter->physics->position.z);

		if (is_in_air  && !is_landing) {
			ONrCharacter_Land(inCharacter, ioActiveCharacter, touched_ground_location);
		}
		else if (is_in_air && is_landing) {
			if (ioActiveCharacter->inAirControl.numFramesInAir != 0) {
				COrConsole_Printf("characters %s was landing, hit ground, but was also in air, fixing", inCharacter->player_name);
				ioActiveCharacter->inAirControl.numFramesInAir = 0;
			}
		}
	}
	else {
		if (gChrDebugFall) {
			COrConsole_Printf("missed");
		}
	}

	return hit_ground;
}

static void
ONrCharacter_Callback_PrivateMovement(
	PHtPhysicsContext *inContext)
{
	ONtCharacter *character = (ONtCharacter *) inContext->callbackData;
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(character);
	UUtBool hit_ground;

	UUmAssertReadPtr(active_character, sizeof(ONtActiveCharacter));

	if ((active_character->gravityThisFrame == 0) || (inContext->flags & PHcFlags_Physics_NoGravity)) {
		return;
	}

 	hit_ground = FindGroundCollision(character, active_character);

	if ((0 == active_character->inAirControl.numFramesInAir) && !hit_ground) {
		if (0 == active_character->offGroundFrames) {
			active_character->inAirControl.jump_height = active_character->physics->position.y;
		}

		active_character->offGroundFrames++;
	}
	else
	{
		active_character->offGroundFrames = 0;
	}

	if (active_character->offGroundFrames > 11) {
		active_character->pleaseJump = UUcTrue;
	}


	return;
}

static void ONrCharacter_Callback_ReceiveForce(
	PHtPhysicsContext *ioContext,
	const PHtPhysicsContext *inPushingContext,
	const M3tVector3D *inForce)
{
	ONtCharacter *character;
	ONtActiveCharacter *active_character;

	UUmAssertReadPtr(ioContext, sizeof(*ioContext));
	UUmAssertReadPtr(inForce, sizeof(*inForce));

	character = (ONtCharacter *) ioContext->callbackData;
	UUmAssertWritePtr(character, sizeof(*character));

	active_character = ONrGetActiveCharacter(character);
	UUmAssertReadPtr(active_character, sizeof(*active_character));

	if (inPushingContext->callback->type == PHcCallback_Weapon) {
		// characters don't get pushed around by weapons
		return;
	}

	MUmVector_Increment(active_character->knockback, *inForce);

	return;
}

static void
ONrCharacter_Callback_PostReflection(
	PHtPhysicsContext *ioContext,
	M3tVector3D *inVelocityThisFrame)
{
	inVelocityThisFrame->y = UUmMax(0.f, inVelocityThisFrame->y);

	return;
}

static void
ONrCharacter_Callback_FindPhyCollisions(
	const PHtPhysicsContext *inContext,
	const M3tVector3D *inVelocity,
	PHtCollider *outColliders,
	UUtUns16 *ioNumColliders)
{
	UUtUns16 prev_colliders;
	UUtUns32 i;
	PHtPhysicsContext *targetContext;
	PHtCollider *collider;
	UUtBool skip_weapons;
	UUtBool has_any_physics_collisions;
	ONtCharacter *character;
	ONtActiveCharacter *active_character;

	prev_colliders = *ioNumColliders;

	UUmAssert(inContext->callback->type == PHcCallback_Character);
	character = (ONtCharacter *) inContext->callbackData;
	active_character = ONrGetActiveCharacter(character);
	UUmAssertReadPtr(active_character, sizeof(*active_character));

	// player does not push weapons
	skip_weapons = (character->charType == ONcChar_Player);
	has_any_physics_collisions = !(character->flags & ONcCharacterFlag_Dead_2_Moving);

	if (has_any_physics_collisions) {
		for (i=0; i < PHgPhysicsContextArray->num_contexts; i++)
		{
			targetContext = PHgPhysicsContextArray->contexts + i;
			if (!(targetContext->flags & PHcFlags_Physics_InUse)) continue;
			if (targetContext == inContext) continue;

			// eventually handle characters, for now skip
			if (targetContext->callback->type == PHcCallback_Character) continue;
			if (skip_weapons && (targetContext->callback->type == PHcCallback_Weapon)) continue;
			if (targetContext->callback->type == PHcCallback_Powerup) continue;
			if (targetContext->flags & PHcFlags_Physics_DontBlock) continue;

			PHrPhysics_Single_PhyCollision(inContext, targetContext, inVelocity, outColliders, ioNumColliders);
		}
	}

	active_character->numPhysicsCollisions = *ioNumColliders - prev_colliders;

	if (character->flags & ONcCharacterFlag_AIMovement) {
		// tell the AI about any collisions encountered
		for (i = 0, collider = &outColliders[prev_colliders]; i < active_character->numPhysicsCollisions; i++, collider++) {
			UUmAssert(collider->type == PHcCollider_Phy);
			AI2rMovementState_NotifyPhysicsCollision(character, &active_character->movement_state, (PHtPhysicsContext *) collider->data,
														&collider->planePoint, &collider->plane);
		}
	}

	return;
}

UUtBool ONrAnimType_IsThrowSource(TRtAnimType inAnimType)
{
	UUtBool result = UUcFalse;

	switch(inAnimType)
	{
		case ONcAnimType_Throw_Forward_Punch:
		case ONcAnimType_Throw_Behind_Punch:
		case ONcAnimType_Throw_Forward_Kick:
		case ONcAnimType_Throw_Behind_Kick:
		case ONcAnimType_Run_Throw_Forward_Punch:
		case ONcAnimType_Run_Throw_Behind_Punch:
		case ONcAnimType_Run_Throw_Forward_Kick:
		case ONcAnimType_Run_Throw_Behind_Kick:
			result = UUcTrue;
		break;
	}

	return result;
}

UUtBool ONrAnimType_IsKnockdown(TRtAnimType inAnimType)
{
	UUtBool result = UUcFalse;

	switch(inAnimType)
	{
		case ONcAnimType_Land_Dead:
		case ONcAnimType_Blownup:
		case ONcAnimType_Blownup_Behind:
		case ONcAnimType_Knockdown_Head:
		case ONcAnimType_Knockdown_Body:
		case ONcAnimType_Knockdown_Foot:
		case ONcAnimType_Knockdown_Head_Behind:
		case ONcAnimType_Knockdown_Body_Behind:
		case ONcAnimType_Knockdown_Foot_Behind:
		case ONcAnimType_Knockdown_Crouch:
		case ONcAnimType_Knockdown_Crouch_Behind:
			result = UUcTrue;
		break;
	}

	return result;
}

TRtAnimType ONrAnimType_UpgradeToKnockdown(TRtAnimType inAnimType)
{
	TRtAnimType upgraded_type = inAnimType;

	switch (inAnimType)
	{
		default:
		case ONcAnimType_Stagger:
		case ONcAnimType_Stun:
		case ONcAnimType_Hit_Head:
			upgraded_type = ONcAnimType_Knockdown_Head;
		break;

		case ONcAnimType_Hit_Body:
			upgraded_type = ONcAnimType_Knockdown_Body;
		break;

		case ONcAnimType_Hit_Foot:
			upgraded_type = ONcAnimType_Knockdown_Foot;
		break;

		case ONcAnimType_Hit_Crouch:
			upgraded_type = ONcAnimType_Knockdown_Crouch;
		break;

		case ONcAnimType_Hit_Head_Behind:
			upgraded_type = ONcAnimType_Knockdown_Head_Behind;
		break;

		case ONcAnimType_Hit_Body_Behind:
			upgraded_type = ONcAnimType_Knockdown_Body_Behind;
		break;

		case ONcAnimType_Hit_Foot_Behind:
			upgraded_type = ONcAnimType_Knockdown_Foot_Behind;
		break;

		case ONcAnimType_Hit_Crouch_Behind:
			upgraded_type = ONcAnimType_Knockdown_Crouch_Behind;
		break;
	}

	return upgraded_type;
}

UUtBool ONrAnimState_IsFallen(TRtAnimState inAnimState)
{
	UUtBool result = UUcFalse;

	switch(inAnimState)
	{
		case ONcAnimState_Fallen_Back:
		case ONcAnimState_Fallen_Front:
			result = UUcTrue;
		break;
	}

	return result;
}

UUtBool ONrAnimState_IsCrouch(TRtAnimState inAnimState)
{
	UUtBool result = UUcFalse;

	switch(inAnimState)
	{
		case ONcAnimState_Crouch:
		case ONcAnimState_Crouch_Walk:
		case ONcAnimState_Crouch_Start:
		case ONcAnimState_Crouch_Run_Left:
		case ONcAnimState_Crouch_Run_Right:
		case ONcAnimState_Crouch_Run_Back_Left:
		case ONcAnimState_Crouch_Run_Back_Right:
		case ONcAnimState_Crouch_Blocking1:
			result = UUcTrue;
		break;
	}

	return result;
}

ONtCharacterClass *ONrGetCharacterClass(const char *inString)
{
	UUtError error;
	ONtCharacterClass *result = NULL;

	error = TMrInstance_GetDataPtr(
		TRcTemplate_CharacterClass,
		inString,
		&result);

	if (UUcError_None != error) {
		result = NULL;
	}

	return result;
}

void ONrCharacter_SetSprintVarient(ONtCharacter *ioCharacter, UUtBool inSprint)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);

	if (active_character == NULL) {
		return;
	}

	if (inSprint) {
		active_character->animVarient |= ONcAnimVarientMask_Sprint;
	} else {
		active_character->animVarient &= ~ONcAnimVarientMask_Sprint;
	}
}

static void ONrCharacter_SetWeaponVarient(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	ioActiveCharacter->animVarient &= ~ONcAnimVarientMask_Any_Weapon;

	if ((NULL != ioCharacter->inventory.weapons[0]) && (0 == (ioCharacter->flags & ONcCharacterFlag_DontUseGunVarient)))
	{
		WPtWeaponClass		*weaponClass;
		UUtBool two_handed_weapon;
		UUtBool lefty;

		weaponClass = WPrGetClass(ioCharacter->inventory.weapons[0]);
		UUmAssertReadPtr(weaponClass, sizeof(WPtWeaponClass));

		two_handed_weapon = (weaponClass->flags & WPcWeaponClassFlag_TwoHanded) ? UUcTrue : UUcFalse;
		lefty = ioCharacter->characterClass->leftHanded;

		if (lefty & two_handed_weapon) {
			ioActiveCharacter->animVarient |= ONcAnimVarientMask_Lefty_Rifle;
		}
		else if (two_handed_weapon) {
			ioActiveCharacter->animVarient |= ONcAnimVarientMask_Righty_Rifle;
		}
		else if (lefty) {
			ioActiveCharacter->animVarient |= ONcAnimVarientMask_Lefty_Pistol;
		}
		else {
			ioActiveCharacter->animVarient |= ONcAnimVarientMask_Righty_Pistol;
		}
	}

	return;
}

void ONrCharacter_SetPanicVarient(ONtCharacter *ioCharacter, UUtBool inPanic)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);

	if (active_character != NULL) {
		if (inPanic) {
			active_character->animVarient |= ONcAnimVarientMask_Panic;
		} else {
			active_character->animVarient &= ~ONcAnimVarientMask_Panic;
		}
	}
}

void ONrCharacter_DisableWeaponVarient(ONtCharacter *ioCharacter, UUtBool inDisable)
{
	ONtActiveCharacter *active_character;

	if (inDisable) {
		ioCharacter->flags |= ONcCharacterFlag_DontUseGunVarient;
	}
	else {
		ioCharacter->flags &= ~ONcCharacterFlag_DontUseGunVarient;
	}

	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character != NULL) {
		ONrCharacter_SetWeaponVarient(ioCharacter, active_character);
	}

	return;
}

void ONrCharacter_DisableFightVarient(ONtCharacter *ioCharacter, UUtBool inDisable)
{
	ONtActiveCharacter *active_character;

	if (inDisable) {
		ioCharacter->flags |= ONcCharacterFlag_DontUseFightVarient;

		active_character = ONrGetActiveCharacter(ioCharacter);
		if (active_character != NULL) {
			active_character->animVarient &= ~ONcAnimVarientMask_Fight;
		}
	} else {
		ioCharacter->flags &= ~ONcCharacterFlag_DontUseFightVarient;
	}

	return;
}

void ONrCharacter_BuildPhysicsFlags(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUtBool noCollision;
	PHtPhysicsContext *myPhysics;

	UUmAssertReadPtr(ioCharacter, sizeof(*ioCharacter));
	if (ioActiveCharacter == NULL) {
		goto exit;
	}

	UUmAssertReadPtr(ioActiveCharacter, sizeof(*ioActiveCharacter));
	UUmAssertReadPtr(ioActiveCharacter->physics, sizeof(*ioActiveCharacter->physics));

	myPhysics = ioActiveCharacter->physics;
	if (NULL == myPhysics) {
		goto exit;
	}

	// three situations where we don't want collision
	noCollision = UUcFalse;
	noCollision = noCollision || (myPhysics->animContext.animation != NULL);
	noCollision = noCollision || (ioCharacter->flags & ONcCharacterFlag_NoCollision);
//	noCollision = noCollision || (ioCharacter->flags & ONcCharacterFlag_Frozen);

	// set the collision & gravity flags accordingly
	if (noCollision) {
		myPhysics->flags |= PHcFlags_Physics_NoCollision;
		myPhysics->flags |= PHcFlags_Physics_NoGravity;
	}
	else {
		myPhysics->flags &= ~PHcFlags_Physics_NoCollision;
		myPhysics->flags &= ~PHcFlags_Physics_NoGravity;
	}

	// NOTE: we could set the AI to have two reflection passes if it was getting
	// stuck on something for a couple of frames
	if (ONcChar_Player == ioCharacter->charType) {
		myPhysics->flags |= PHcFlags_Physics_TwoReflectionPasses;
	}
	else {
		myPhysics->flags &= ~PHcFlags_Physics_TwoReflectionPasses;
	}

	if (ioCharacter->flags & ONcCharacterFlag_Dead_2_Moving) {
		myPhysics->flags |= PHcFlags_Physics_DontBlock;
	}
	else {
		myPhysics->flags &= ~PHcFlags_Physics_DontBlock;
	}

exit:
	return;
}

UUtBool ONrCharacter_IsBusyInventory(const ONtCharacter *inCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character == NULL) {
		return UUcFalse;
	} else {
		return ONrOverlay_IsActive(active_character->overlay + ONcOverlayIndex_InventoryManagement);
	}
}

UUtBool ONrCharacter_CanUseConsole(const ONtCharacter *inCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character == NULL) {
		return UUcTrue;
	}

	// characters must be upright and not actively running in order to use a console
	if (active_character->animVarient & ONcAnimVarientMask_Sprint)
		return UUcFalse;

	switch(active_character->curAnimType) {
		case ONcAnimType_Stand:
		case ONcAnimType_Standing_Turn_Left:
		case ONcAnimType_Standing_Turn_Right:
		case ONcAnimType_Run_Stop:
		case ONcAnimType_Walk_Stop:
		case ONcAnimType_1Step_Stop:
		case ONcAnimType_Walk:
		case ONcAnimType_Walk_Sidestep_Left:
		case ONcAnimType_Walk_Sidestep_Right:
		case ONcAnimType_Run:
		case ONcAnimType_Run_Backwards:
		case ONcAnimType_Run_Sidestep_Left:
		case ONcAnimType_Run_Sidestep_Right:
		case ONcAnimType_Walk_Backwards:
		case ONcAnimType_Land:
		case ONcAnimType_Run_Start:
		case ONcAnimType_Walk_Start:
		case ONcAnimType_Run_Backwards_Start:
		case ONcAnimType_Walk_Backwards_Start:
		case ONcAnimType_Run_Sidestep_Left_Start:
		case ONcAnimType_Run_Sidestep_Right_Start:
			return UUcTrue;

		default:
			return UUcFalse;
	}
}

UUtBool ONrCharacter_IsStanding(const ONtCharacter *inCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character == NULL) {
		return UUcTrue;
	}

	if (active_character->nextAnimState != ONcAnimState_Standing) {
		return UUcFalse;
	}

	switch(active_character->curAnimType) {
		case ONcAnimType_Stand:
		case ONcAnimType_Standing_Turn_Left:
		case ONcAnimType_Standing_Turn_Right:
		case ONcAnimType_Run_Stop:
		case ONcAnimType_Walk_Stop:
		case ONcAnimType_1Step_Stop:
			return UUcTrue;

		default:
			return UUcFalse;
	}
}

UUtBool ONrCharacter_IsStill(const ONtCharacter *inCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character == NULL) {
		return UUcTrue;

	} else if ((active_character->curAnimType != ONcAnimType_Stand) &&
				(active_character->curAnimType != ONcAnimType_Crouch)) {
		return UUcFalse;

	} else {
		switch(active_character->nextAnimState) {
			case ONcAnimState_Standing:
			case ONcAnimState_Crouch:
			case ONcAnimState_Lie_Back:
			case ONcAnimState_Fallen_Back:
			case ONcAnimState_Fallen_Front:
				return UUcTrue;

			default:
				return UUcFalse;
		}
	}
}

UUtBool ONrCharacter_IsStandingStill(const ONtCharacter *inCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character == NULL) {
		return UUcTrue;

	} else if (active_character->curAnimType != ONcAnimType_Stand) {
		return UUcFalse;

	} else {
		return ((active_character->nextAnimState == ONcAnimState_Standing) && (active_character->curFromState == ONcAnimState_Standing));
	}
}

static void DoFilmUpdate(ONtCharacter *inCharacter, ONtActiveCharacter *ioActiveCharacter, ONtInputState *ioInput)
{
	if (ioInput->buttonWentDown & LIc_BitMask_StartRecord) {
		if (ioActiveCharacter->filmState.flags & ONcFilmFlag_Record) {
			COrConsole_Printf("already recording");
		}
		else {
			COrConsole_Printf("starting to record");
			ONrFilm_Create(&ioActiveCharacter->filmState);

			ioActiveCharacter->filmState.flags |= ONcFilmFlag_Record;

			ioActiveCharacter->filmState.film->initialLocation = inCharacter->location;
			ioActiveCharacter->filmState.film->initialFacing = inCharacter->facing;
			ioActiveCharacter->filmState.film->initialDesiredFacing = inCharacter->desiredFacing;
			ioActiveCharacter->filmState.film->initialAimingLR = ioActiveCharacter->aimingLR;
			ioActiveCharacter->filmState.film->initialAimingUD = ioActiveCharacter->aimingUD;
		}
	}

	if (ioInput->buttonWentDown & LIc_BitMask_PlayRecord) {
		if (ONrCharacter_IsPlayingFilm(inCharacter)) {
			COrConsole_Printf("already playing");
		}
		else if (NULL == ioActiveCharacter->filmState.film) {
			COrConsole_Printf("nothing recorded");
		}
		else {
			COrConsole_Printf("starting to play");

			ONrFilm_Start(inCharacter, ioActiveCharacter->filmState.film, ONcFilmMode_Normal, 0);
		}
	}

	if (ioInput->buttonWentDown & LIc_BitMask_StopRecord) {
		COrConsole_Printf("stop");

		if (!(ioActiveCharacter->filmState.flags & ONcFilmFlag_Record)) {
			COrConsole_Printf("not recording");
		}
		else {
			ONrFilm_WriteToDisk(ioActiveCharacter->filmState.film);

			ioActiveCharacter->filmState.flags &= ~ONcFilmFlag_Record;
		}
	}

	ioInput->buttonWentDown &= ~LIc_BitMask_StartRecord;
	ioInput->buttonIsDown &= ~LIc_BitMask_StartRecord;
	ioInput->buttonIsUp |= LIc_BitMask_StartRecord;
	ioInput->buttonWentUp &= ~LIc_BitMask_StartRecord;

	ioInput->buttonWentDown &= ~LIc_BitMask_StopRecord;
	ioInput->buttonIsDown &= ~LIc_BitMask_StopRecord;
	ioInput->buttonIsUp |= LIc_BitMask_StopRecord;
	ioInput->buttonWentUp &= ~LIc_BitMask_StopRecord;

	ioInput->buttonWentDown &= ~LIc_BitMask_PlayRecord;
	ioInput->buttonIsDown &= ~LIc_BitMask_PlayRecord;
	ioInput->buttonIsUp |= LIc_BitMask_PlayRecord;
	ioInput->buttonWentUp &= ~LIc_BitMask_PlayRecord;

	if (ioActiveCharacter->filmState.flags & ONcFilmFlag_Record)
	{
		ONrFilm_AppendFrame(&ioActiveCharacter->filmState, ioInput);
	}
	else if (ONrCharacter_IsPlayingFilm(inCharacter))
	{
		if (ioActiveCharacter->filmState.curFrame >= ioActiveCharacter->filmState.film->length) {
			ONrFilm_Stop(inCharacter);
		} else {
			ONrFilm_GetFrame(&ioActiveCharacter->filmState, ioInput);

			ioActiveCharacter->filmState.curFrame += 1;
		}

		if ((saved_film_loop) && (ioActiveCharacter->filmState.curFrame >= ioActiveCharacter->filmState.film->length)) {
			ONrFilm_Start(inCharacter, ioActiveCharacter->filmState.film, ONcFilmMode_Normal, 0);
		}
	}

	return;
}

void ONrCharacter_UpdateInput(ONtCharacter *ioCharacter, const ONtInputState *inInput)
{
	ONtActiveCharacter *active_character;
	ONtInputState input;

	UUmAssertReadPtr(ioCharacter, sizeof(*ioCharacter));

	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character == NULL) {
		return;
	}

	UUmAssertReadPtr(inInput, sizeof(*inInput));
	input = *inInput;

	active_character->inputOld = active_character->inputState.buttonIsDown;

	DoFilmUpdate(ioCharacter, active_character, &input);

	active_character->inputState = input;

	return;
}

void ONrCharacter_PickupWeapon(
	ONtCharacter *inCharacter,
	WPtWeapon *inWeapon,
	UUtBool inForceHolster)
{
	WPtWeapon *stowed;
	UUtUns16 stowedSlot;

	// Does logic for getting and storing a particular weapon.
	// NOTE: Do NOT call this function directly unless you know what you're doing

	if (inWeapon)
	{
		// There's a weapon on the ground
		if (inCharacter->inventory.weapons[0])
		{
			// Storage full, so swap with ground weapon
			ONrCharacter_DropWeapon(inCharacter,UUcFalse,WPcPrimarySlot,UUcFalse);
		}

		ONrCharacter_UseWeapon(inCharacter,inWeapon);
	}
	else if (inForceHolster)
	{
		// CB: we are starting a cinematic and forcing a holster... find an empty slot (we can
		// use the "magic" slot here)
		if (WPrSlot_FindEmpty(&inCharacter->inventory, &stowedSlot, UUcTrue)) {
			WPrSlot_Swap(&inCharacter->inventory, stowedSlot, WPcPrimarySlot);
			ONrCharacter_UseWeapon(inCharacter, NULL);
		}
		else {
			// no slots found. we must drop the weapon.
			ONrCharacter_DropWeapon(inCharacter, UUcFalse, WPcPrimarySlot, UUcFalse);
		}
	}
	else
	{
		// No weapon on ground- if we have a weapon stowed, swap it out
		// If none stowed, we'll stow weapon in hand
		stowed = WPrSlot_FindLastStowed(&inCharacter->inventory, WPcPrimarySlot, &stowedSlot);
		WPrSlot_Swap(&inCharacter->inventory, stowedSlot, WPcPrimarySlot);
		ONrCharacter_UseWeapon(inCharacter, stowed);
	}

	return;
}

void ONrCharacter_PickupPowerup(
	ONtCharacter *inCharacter,
	WPtPowerup *inPowerup)
{
	if (inPowerup == NULL) { return; }

	if (WPrPowerup_Give(inCharacter, inPowerup->type, inPowerup->amount, UUcFalse, UUcTrue))
	{
		WPrPowerup_Delete(inPowerup);
	}

	return;
}


void ONrCharacter_HandlePickupInput(
	ONtInputState *inInput,
	ONtCharacter *inCharacter,
	ONtActiveCharacter *inActiveCharacter)
{
	WPtWeapon *weapon = NULL;
	WPtPowerup *powerup = NULL;

	// can't do a pickup while running a reload or holster animation
	if (ONrOverlay_IsActive(inActiveCharacter->overlay + ONcOverlayIndex_InventoryManagement)) {
		return;
	}

	if (inInput->buttonWentDown & LIc_BitMask_Drop)
	{
		if (inCharacter->inventory.weapons[0])
		{
			ONrCharacter_DropWeapon(inCharacter,UUcFalse,WPcPrimarySlot,UUcFalse);
			return;	// Don't allow other pickup input this frame
		}
	}

	if (inInput->buttonWentDown & LIc_BitMask_Hypo)
	{
//		if (!inCharacter->inventory.hypoRemaining) // don't allow ammo stacking
		{
			if (WPrPowerup_Use(inCharacter,WPcPowerup_Hypo)) {
				ONrGameState_EventSound_Play(ONcEventSound_UseHypo, NULL);
			}
		}
	}

	return;
}



void inverse_kinematics_adjust_matrices(
	M3tPoint3D *hand_position,
	M3tVector3D *hand_forward,
	M3tVector3D *hand_up,
	M3tMatrix4x3 *shoulder_matrix,
	M3tMatrix4x3 *elbow_matrix,
	M3tMatrix4x3 *hand_matrix)
{
	const M3tPoint3D shoulder_position= MUrMatrix_GetTranslation(shoulder_matrix);
	const M3tPoint3D elbow_position_hint= MUrMatrix_GetTranslation(elbow_matrix);
	const M3tPoint3D hand_position_hint= MUrMatrix_GetTranslation(hand_matrix);

	float shoulder_to_elbow_distance= MUmVector_GetDistance(shoulder_position, elbow_position_hint);
	float elbow_to_hand_distance= MUmVector_GetDistance(elbow_position_hint, hand_position_hint);

	float shoulder_to_hand_distance= MUmVector_GetDistance(*hand_position, shoulder_position);

	M3tVector3D shoulder_to_elbow_hint;
	M3tVector3D plane_i, plane_j, plane_k;

	float shoulder_to_elbow_triangle_base, elbow_to_hand_triangle_base;
	float triangle_height;

	M3tPoint3D elbow_position;

	shoulder_to_elbow_hint.x= elbow_position_hint.x - shoulder_position.x;
	shoulder_to_elbow_hint.y= elbow_position_hint.y - shoulder_position.y;
	shoulder_to_elbow_hint.z= elbow_position_hint.z - shoulder_position.z;

	// generate coordinate system for plane of rotation
	{
		float one_over_shoulder_to_hand_distance= 1.f/shoulder_to_hand_distance;

 		// x-axis
		plane_i.x= (hand_position->x-shoulder_position.x)*one_over_shoulder_to_hand_distance;
		plane_i.y= (hand_position->y-shoulder_position.y)*one_over_shoulder_to_hand_distance;
		plane_i.z= (hand_position->z-shoulder_position.z)*one_over_shoulder_to_hand_distance;

		// z-axis
		MUrVector_CrossProduct(&plane_i, &shoulder_to_elbow_hint, &plane_k);
		MUmVector_Normalize(plane_k);

		// y-axis
		MUrVector_CrossProduct(&plane_k, &plane_i, &plane_j);
	}

	// adjust hand position if it's too far away
	{
		const float longest_reach_factor= 0.98f; // only allow 98% extension

		float longest_reach= longest_reach_factor*(shoulder_to_elbow_distance+elbow_to_hand_distance);

		if (longest_reach<shoulder_to_hand_distance)
		{
			hand_position->x= shoulder_position.x + plane_i.x*longest_reach;
			hand_position->y= shoulder_position.y + plane_i.y*longest_reach;
			hand_position->z= shoulder_position.z + plane_i.z*longest_reach;

			shoulder_to_hand_distance= longest_reach;
		}
	}

	// generate lengths of triangles in plane
	shoulder_to_elbow_triangle_base= (shoulder_to_elbow_distance*shoulder_to_elbow_distance + shoulder_to_hand_distance*shoulder_to_hand_distance -
		elbow_to_hand_distance*elbow_to_hand_distance)/(2.f*shoulder_to_hand_distance);
	elbow_to_hand_triangle_base= shoulder_to_hand_distance-shoulder_to_elbow_triangle_base;
	triangle_height= MUrSqrt(shoulder_to_elbow_distance*shoulder_to_elbow_distance - shoulder_to_elbow_triangle_base*shoulder_to_elbow_triangle_base);

	// generate shoulder_matrix
	{
		M3tVector3D x_axis= MUrMatrix_GetXAxis(shoulder_matrix);
		M3tVector3D y_axis= MUrMatrix_GetYAxis(shoulder_matrix);
		M3tVector3D z_axis= MUrMatrix_GetZAxis(shoulder_matrix);

		// x-axis
		x_axis.x= shoulder_to_elbow_triangle_base*plane_i.x + triangle_height*plane_j.x;
		x_axis.y= shoulder_to_elbow_triangle_base*plane_i.y + triangle_height*plane_j.y;
		x_axis.z= shoulder_to_elbow_triangle_base*plane_i.z + triangle_height*plane_j.z;
		MUmVector_Normalize(x_axis);

		// y-axis and z-axis
		MUrVector_CrossProduct(&x_axis, &y_axis, &z_axis);
		MUmVector_Normalize(z_axis);
		MUrVector_CrossProduct(&z_axis, &x_axis, &y_axis);

		// compute elbow position in world space
		elbow_position.x= shoulder_position.x + shoulder_to_elbow_distance*x_axis.x;
		elbow_position.y= shoulder_position.y + shoulder_to_elbow_distance*x_axis.y;
		elbow_position.z= shoulder_position.z + shoulder_to_elbow_distance*x_axis.z;

		MUrMatrix_SetXAxis(shoulder_matrix, &x_axis);
		MUrMatrix_SetYAxis(shoulder_matrix, &y_axis);
		MUrMatrix_SetZAxis(shoulder_matrix, &z_axis);
	}

	// generate elbow_matrix
	{
		M3tVector3D x_axis= MUrMatrix_GetXAxis(elbow_matrix);
		M3tVector3D y_axis= MUrMatrix_GetYAxis(elbow_matrix);
		M3tVector3D z_axis= MUrMatrix_GetZAxis(elbow_matrix);
		M3tPoint3D translation= MUrMatrix_GetTranslation(elbow_matrix);

		// x-axis
		x_axis.x= elbow_to_hand_triangle_base*plane_i.x - triangle_height*plane_j.x;
		x_axis.y= elbow_to_hand_triangle_base*plane_i.y - triangle_height*plane_j.y;
		x_axis.z= elbow_to_hand_triangle_base*plane_i.z - triangle_height*plane_j.z;
		MUmVector_Normalize(x_axis);

		// y-axis and z-axis
		MUrVector_CrossProduct(&x_axis, &y_axis, &z_axis);
		MUmVector_Normalize(z_axis);
		MUrVector_CrossProduct(&z_axis, &x_axis, &y_axis);

		// translation to elbow position
		translation= elbow_position;

		MUrMatrix_SetXAxis(elbow_matrix, &x_axis);
		MUrMatrix_SetYAxis(elbow_matrix, &y_axis);
		MUrMatrix_SetZAxis(elbow_matrix, &z_axis);
		MUrMatrix_SetTranslation(elbow_matrix, &translation);
	}

	// generate hand_matrix
	MUrMatrix_From_Point_And_Vectors(hand_matrix, hand_position, hand_forward, hand_up);

	return;
}

float ONrCharacter_GetTorsoHeight(const ONtCharacter *inCharacter)
{
	float torso_height = inCharacter->heightThisFrame;

	return torso_height;
}

float ONrCharacter_GetHeadHeight(const ONtCharacter *inCharacter)
{
	float head_height = inCharacter->heightThisFrame + 5.f;

	return head_height;
}

void ONrCharacter_NotIdle(ONtCharacter *ioCharacter)
{
	ioCharacter->lastActiveAnimationTime = ONgGameState->gameTime;

	return;
}

static char *mask_to_string(TMtStringArray *inTable, UUtUns32 inNumber)
{
	static char buffer[512];
	buffer[0] = '\0';
	buffer[1] = '\0';

	if ((NULL != inTable)) {
		UUtUns32 itr;

		for(itr = 0; itr < 32; itr++) {
			if (inNumber & (1 << itr)) {
				char *string = "";

				TMtString *entry = inTable->string[itr];

				string = entry->string;

				strcat(buffer, " ");
				strcat(buffer, string);
			}
		}
	}

	return buffer + 1;
}

static char *num_to_string(TMtStringArray *inTable, UUtUns32 inNumber)
{
	char *string = "";

	if ((NULL != inTable) && (inNumber >= 0) && (inNumber < inTable->numStrings))
	{
		TMtString *entry = inTable->string[inNumber];

		string = entry->string;
	}

	return string;
}

void ONrCharacter_DumpAllCollections(void)
{
	UUtUns32 collection_index;
	UUtUns32 num_collections = TMrInstance_GetTagCount(TRcTemplate_AnimationCollection);
	BFtFile *file = BFrFile_FOpen("anim_table.xls", "w");

	if (NULL == file) {
		return;
	}

	for(collection_index = 0; collection_index < num_collections; collection_index++)
	{
		TRtAnimationCollection *collection;

		TMrInstance_GetDataPtr_ByNumber(TRcTemplate_AnimationCollection, collection_index, &collection);
		ONrCharacter_DumpCollection(file, collection);
	}

	BFrFile_Close(file);

	return;
}

void ONrCharacter_DumpCollection(BFtFile *inFile, TRtAnimationCollection *inCollection)
{
	UUtUns16 anim_index;
	UUtUns16 anim_count = inCollection->numAnimations;
	char *collection_name;
	TMtStringArray *anim_states = NULL;
	TMtStringArray *anim_types = NULL;
	TMtStringArray *anim_flags = NULL;

	TMrInstance_GetDataPtr(TMcTemplate_StringArray, "anim_types", &anim_types);
	TMrInstance_GetDataPtr(TMcTemplate_StringArray, "anim_states", &anim_states);
	TMrInstance_GetDataPtr(TMcTemplate_StringArray, "anim_flags", &anim_flags);

	if (inCollection->recursiveOnly) {
		return;
	}

	collection_name = TMrInstance_GetInstanceName(inCollection);

	BFrFile_Printf(inFile, UUmNL);

	for(anim_index = 0; anim_index < anim_count; anim_index++)
	{
		TRtAnimation *animation = inCollection->entry[anim_index].animation;
		char *animation_name = TMrInstance_GetInstanceName(animation);
		UUtUns32 attack_index;
		UUtUns32 anim_flag_mask = ~0x1;

		BFrFile_Printf(inFile, "%s\t", animation_name);
		BFrFile_Printf(inFile, "%s\t", collection_name);
		BFrFile_Printf(inFile, "%s\t", TRrAnimation_IsAttack(animation) ? "YES" : "NO");
		BFrFile_Printf(inFile, "%d\t", animation->numAttacks);
		BFrFile_Printf(inFile, "%s\t", num_to_string(anim_states, animation->fromState));
		BFrFile_Printf(inFile, "%s\t", num_to_string(anim_states, animation->toState));
		BFrFile_Printf(inFile, "%s\t", num_to_string(anim_types, animation->type));
		BFrFile_Printf(inFile, "%s\t", mask_to_string(anim_flags, anim_flag_mask & animation->flags));

		for(attack_index = 0; attack_index < TRcMaxAttacks; attack_index++)
		{
			if (attack_index < animation->numAttacks) {
				TRtAttack *cur_attack= animation->attacks + attack_index;

				BFrFile_Printf(inFile, "%x\t", cur_attack->damageBits);
				BFrFile_Printf(inFile, "%f\t", cur_attack->knockback);
				BFrFile_Printf(inFile, "%d\t", cur_attack->damage);
				BFrFile_Printf(inFile, "%d\t", cur_attack->firstDamageFrame);
				BFrFile_Printf(inFile, "%d\t", cur_attack->lastDamageFrame);
				BFrFile_Printf(inFile, "%s\t", num_to_string(anim_types, cur_attack->damageAnimation));
			}
			else {
				BFrFile_Printf(inFile, "\t\t\t\t\t\t\t");
			}
		}

		BFrFile_Printf(inFile, UUmNL);
	}

	return;
}

UUtBool ONrCharacter_IsCaryingWeapon(ONtCharacter *ioCharacter)
{
	WPtWeapon *stowed_weapon;
	UUtUns16 slot_number;

	if (ioCharacter->inventory.weapons[0] != NULL)
		return UUcTrue;

	stowed_weapon = WPrSlot_FindLastStowed(&ioCharacter->inventory, WPcPrimarySlot, &slot_number);
	return (stowed_weapon != NULL);
}

// is a character out of ammo for a weapon?
UUtBool ONrCharacter_OutOfAmmo(ONtCharacter *ioCharacter)
{
	UUtUns32 reloads;
	ONtActiveCharacter *active_character;

	if (ioCharacter->inventory.weapons[0] == NULL) {
		return UUcFalse;
	}

	if (WPrHasAmmo(ioCharacter->inventory.weapons[0])) {
		return UUcFalse;
	}

	// CB: a character may have no ammo in their weapon, and no ammo in their inventory,
	// yet not actually be out of ammo because they're running the reload animation. 02 September 2000
	active_character = ONrGetActiveCharacter(ioCharacter);

	if (active_character != NULL) {
		ONtOverlay *inventory_overlay = active_character->overlay + ONcOverlayIndex_InventoryManagement;

		if (ONrOverlay_IsActive(active_character->overlay + ONcOverlayIndex_InventoryManagement)) {
			switch(TRrAnimation_GetType(inventory_overlay->animation))
			{
				case ONcAnimType_Reload_Pistol:
				case ONcAnimType_Reload_Rifle:
				case ONcAnimType_Reload_Stream:
				case ONcAnimType_Reload_Superball:
				case ONcAnimType_Reload_Vandegraf:
				case ONcAnimType_Reload_Scram_Cannon:
				case ONcAnimType_Reload_MercuryBow:
				case ONcAnimType_Reload_Screamer:
					return UUcFalse;
				break;
			}
		}
	}

	reloads = WPrPowerup_Count(ioCharacter, WPrAmmoType(ioCharacter->inventory.weapons[0]));
	return (reloads == 0);
}

// does a character have ammo to reload a weapon?
UUtBool ONrCharacter_HasAmmoForReload(ONtCharacter *ioCharacter, WPtWeapon *ioWeapon)
{
	if (ioCharacter->flags & ONcCharacterFlag_InfiniteAmmo) {
		return UUcTrue;
	}

	return (WPrPowerup_Count(ioCharacter, WPrAmmoType(ioWeapon)) > 0);
}

UUtBool ONrAnimType_IsReload(TRtAnimType inAnimType)
{
	switch(inAnimType) {
		case ONcAnimType_Reload_Pistol:
		case ONcAnimType_Reload_Rifle:
		case ONcAnimType_Reload_Stream:
		case ONcAnimType_Reload_Superball:
		case ONcAnimType_Reload_Vandegraf:
		case ONcAnimType_Reload_Scram_Cannon:
		case ONcAnimType_Reload_MercuryBow:
		case ONcAnimType_Reload_Screamer:
			return UUcTrue;

		default:
			return UUcFalse;
	}
}

UUtBool ONrCharacter_IsReloading(ONtCharacter *ioCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
	TRtAnimType anim_type;

	if (active_character == NULL) {
		return UUcFalse;
	}

	if (!ONrOverlay_IsActive(active_character->overlay + ONcOverlayIndex_InventoryManagement)) {
		return UUcFalse;
	}

	anim_type = TRrAnimation_GetType(active_character->overlay[ONcOverlayIndex_InventoryManagement].animation);
	return ONrAnimType_IsReload(anim_type);
}

// start reloading - if UUcFalse is returned, call again next frame
UUtBool ONrCharacter_Reload(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	const TRtAnimation *reload_animation;

	// can we even stop to think about reloading right now?
	if ((TRrAnimation_IsAttack(ioActiveCharacter->animation)) ||
		(ONrAnimType_IsVictimType(ioActiveCharacter->curAnimType))) {
		return UUcFalse;
	}

	// can't reload if we're already reloading
	if (ONrOverlay_IsActive(ioActiveCharacter->overlay + ONcOverlayIndex_InventoryManagement)) {
		return UUcFalse;
	}

	if (ioCharacter->inventory.weapons[0] == NULL) {
		return UUcTrue;
	}

	// must have ammo to reload
	if (!ONrCharacter_HasAmmoForReload(ioCharacter, ioCharacter->inventory.weapons[0])) {
		return UUcTrue;
	}

	reload_animation = TRrCollection_Lookup(ioCharacter->characterClass->animations,
											WPrGetClass(ioCharacter->inventory.weapons[0])->reloadAnimType,
											ONcAnimState_Standing,
											ioActiveCharacter->animVarient);
	if (reload_animation != NULL) {
		// use the ammo and start the reload
		WPrPowerup_Use(ioCharacter, WPrAmmoType(ioCharacter->inventory.weapons[0]));
		ONrCharacter_NotIdle(ioCharacter);
		ONrOverlay_Set(ioActiveCharacter->overlay + ONcOverlayIndex_InventoryManagement, reload_animation, 0);
	}

	return UUcTrue;
}


UUtBool ONrCharacter_Holster(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter, UUtBool inForceHolster)
{
	UUtBool holstered = UUcFalse;
	UUtBool is_attack = TRrAnimation_IsAttack(ioActiveCharacter->animation);
	UUtBool is_victim_animation = ONrAnimType_IsVictimType(ioActiveCharacter->curAnimType);

	if ((ioCharacter->inventory.weapons[0] == NULL) || (WPrGetClass(ioCharacter->inventory.weapons[0])->flags & WPcWeaponClassFlag_Unstorable)) {
		return UUcFalse;
	}

	ONrCharacter_NotIdle(ioCharacter);

	if ((ioCharacter->inventory.weapons[0]) && (!is_attack) && (!is_victim_animation)) {
		const WPtWeaponClass *weapon_class = WPrGetClass(ioCharacter->inventory.weapons[0]);
		TRtAnimType holster_anim_type = ONcAnimType_Holster;
		const TRtAnimation *holster_animation;
		UUtUns32 flags = 0;

		if (inForceHolster) {
			flags |= ONcOverlayFlag_MagicHolster;
		}

		holster_animation =
			TRrCollection_Lookup(
				ioCharacter->characterClass->animations,
				holster_anim_type,
				ONcAnimState_Standing,
				ioActiveCharacter->animVarient);

		if (NULL != holster_animation) {
			ONrOverlay_Set(ioActiveCharacter->overlay + ONcOverlayIndex_InventoryManagement, holster_animation, flags);
			holstered = UUcTrue;
		}
	}

	return holstered;
}

void ONrCharacter_Draw(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter, WPtWeapon *inWeapon)
{
	UUtBool is_attack = TRrAnimation_IsAttack(ioActiveCharacter->animation);
	UUtBool is_victim_animation = ONrAnimType_IsVictimType(ioActiveCharacter->curAnimType);
	UUtUns16 slot_number;
	WPtWeapon *weapon_to_draw = inWeapon;

	if (NULL == weapon_to_draw) {
		WPtWeapon *stowed_weapon = WPrSlot_FindLastStowed(&ioCharacter->inventory, WPcPrimarySlot, &slot_number);

		weapon_to_draw = stowed_weapon;
	}

	ONrCharacter_NotIdle(ioCharacter);

	if ((weapon_to_draw != NULL) && (!is_attack) && (!is_victim_animation)) {
		const WPtWeaponClass *weapon_class = WPrGetClass(weapon_to_draw);
		TRtAnimType draw_anim_type = (weapon_class->flags & WPcWeaponClassFlag_TwoHanded) ? ONcAnimType_Draw_Rifle : ONcAnimType_Draw_Pistol;
		const TRtAnimation *draw_animation;

		draw_animation =
			TRrCollection_Lookup(
				ioCharacter->characterClass->animations,
				draw_anim_type,
				ONcAnimState_Standing,
				ioActiveCharacter->animVarient);

		if (NULL != draw_animation) {
			ONrOverlay_Set(ioActiveCharacter->overlay + ONcOverlayIndex_InventoryManagement, draw_animation, 0);
		}
	}

	return;
}

// get a character class's average movement speed in units per frame
float ONrCharacterClass_MovementSpeed(ONtCharacterClass *inClass, AI2tMovementDirection inDirection,
									  AI2tMovementMode inMoveMode, UUtBool inWeapon, UUtBool inTwoHandedWeapon)
{
	if (inDirection == AI2cMovementDirection_Stopped)
		return 0.0f;

	// CB: this will eventually read a table of values from the character class - however
	// for now it has hardcoded numbers that represent konoko.

	switch(inMoveMode) {
	case AI2cMovementMode_NoAim_Walk:
		inWeapon = UUcFalse;
		// fall through to walking

	case AI2cMovementMode_Walk:
		switch(inDirection) {
		case AI2cMovementDirection_Forward:
			// walk
			return 0.22f;

		case AI2cMovementDirection_Backpedal:
			// walk backwards
			return 0.22f;

		case AI2cMovementDirection_Sidestep_Left:
		case AI2cMovementDirection_Sidestep_Right:
			// walk sideways
			return 0.20f;

		default:
			UUmAssert(!"ONrCharacterClass_MovementSpeed: unknown movement direction!");
			return 0.0f;
		}

	case AI2cMovementMode_NoAim_Run:
		inWeapon = UUcFalse;
		// fall through to running

	case AI2cMovementMode_Run:
		switch(inDirection) {
		case AI2cMovementDirection_Forward:
			if (inWeapon) {
				if (inTwoHandedWeapon) {
					// rifle run
					return 0.80f;
				} else {
					// pistol run
					return 0.88f;
				}
			} else {
				// run
				return 0.88f;
			}

		case AI2cMovementDirection_Backpedal:
			if (inWeapon) {
				if (inTwoHandedWeapon) {
					// rifle run backwards
					return 0.59f;
				} else {
					// pistol run backwards
					return 0.70f;
				}
			} else {
				// run backwards
				return 0.80f;
			}

		case AI2cMovementDirection_Sidestep_Left:
		case AI2cMovementDirection_Sidestep_Right:
			if (inWeapon) {
				if (inTwoHandedWeapon) {
					// rifle run sideways
					return 0.75f;
				} else {
					// pistol run sideways
					return 0.84f;
				}
			} else {
				// run sideways
				return 0.84f;
			}

		default:
			UUmAssert(!"ONrCharacterClass_MovementSpeed: unknown movement direction!");
			return 0.0f;
		}

	case AI2cMovementMode_Creep:
		switch(inDirection) {
		case AI2cMovementDirection_Forward:
			// creep
			return 0.37f;

		case AI2cMovementDirection_Backpedal:
			// creep backwards
			return 0.35f;

		case AI2cMovementDirection_Sidestep_Left:
		case AI2cMovementDirection_Sidestep_Right:
			// creep sideways
			return 0.37f;

		default:
			UUmAssert(!"ONrCharacterClass_MovementSpeed: unknown movement direction!");
			return 0.0f;
		}

	default:
		UUmAssert(!"ONrCharacterClass_MovementSpeed: unknown movement mode!");
		return 0.0f;
	}
}

// get a character class's stopping distance in units
float ONrCharacterClass_StoppingDistance(ONtCharacterClass *inClass, AI2tMovementDirection inDirection,
										AI2tMovementMode inMoveMode)
{
	if (inDirection == AI2cMovementDirection_Stopped)
		return 0.0f;

	// CB: the values that I got from steve's animations are far too small. in fact characters take
	// much longer to stop than their animations would indicate... probably dependent on their initial
	// velocity too. these values are largely guesswork.

	switch(inMoveMode) {
	case AI2cMovementMode_NoAim_Walk:
	case AI2cMovementMode_Walk:
		// walk - all directions are the same
		return 6.0f;		// CB 5/29/00: changed from 8 to 6

	case AI2cMovementMode_NoAim_Run:
	case AI2cMovementMode_Run:
		switch(inDirection) {
		case AI2cMovementDirection_Forward:
		case AI2cMovementDirection_Backpedal:
			// run
			return 15.0f;

		case AI2cMovementDirection_Sidestep_Left:
		case AI2cMovementDirection_Sidestep_Right:
			// sidestep
			return 8.0f;

		default:
			UUmAssert(!"ONrCharacterClass_StoppingDistance: unknown movement direction!");
			return 0.0f;
		}

	case AI2cMovementMode_Creep:
		// creep - all directions are the same
		return 6.0f;

	default:
		UUmAssert(!"ONrCharacterClass_MovementSpeed: unknown movement mode!");
		return 0.0f;
	}
}

void ONrCharacter_ConsoleAction_Begin(ONtCharacter *inCharacter, ONtActionMarker *inActionMarker)
{
	UUmAssert( inCharacter );
	UUmAssert( inActionMarker );
	UUmAssert( !inCharacter->console_marker );

	// mark character as moving towards marker
	inCharacter->console_marker = inActionMarker;

	// force a holster of our weapon
	ONrCharacter_CinematicHolster(inCharacter);

	if (inCharacter == ONgGameState->local.playerCharacter) {
		// block user input until console action is complete
		ONgGameState->inside_console_action = UUcTrue;
	}

	// flag that we haven't started the movement part of the console action yet
	inCharacter->console_aborttime = 0;
}

void ONrCharacter_ConsoleAction_Finish(ONtCharacter *inCharacter, UUtBool inSuccess)
{
	UUmAssert( inCharacter );
	UUmAssert( inCharacter->console_marker );

	// release action...
	inCharacter->console_marker = NULL;

	if (inSuccess) {
		// if we holstered our weapon previously and we have no room to store it there,
		// unholster it now
		ONrCharacter_CinematicEndHolster(inCharacter, UUcTrue);

		if (inCharacter->charType == ONcChar_AI2) {
			// tell the AI that the console action completed, successfully or unsuccessfully
			AI2rNotifyConsoleAction(inCharacter, inSuccess);
		}
	}
	else {
		// we have been knocked out of the console action, don't try to draw our weapon ever
		WPrInventory_Weapon_Purge_Magic(&inCharacter->inventory);
	}

	if (inCharacter == ONgGameState->local.playerCharacter) {
		// unblock user input
		ONgGameState->inside_console_action = UUcFalse;
	}

	return;
}

void ONrCharacter_ConsoleAction_Update(ONtCharacter *inCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	ONtActionMarker*	action_marker;
	OBJtObject			*object;
	OBJtOSD_Console		*console_osd;

	M3tPoint3D			new_position;
	M3tVector3D			new_vector;
	UUtInt16			action_frame;
	float				new_facing;
	float				min_distance = UUmFeet(4.0);
	float				distance;
	UUtBool				is_running_a_console_animation =
		(ioActiveCharacter->curAnimType == ONcAnimType_Console) ||
		(ioActiveCharacter->curAnimType == ONcAnimType_Console_Punch);

	UUmAssert( inCharacter );
	UUmAssert( inCharacter->console_marker );

	if (inCharacter->console_aborttime == 0) {
		// we are still waiting for the character to come to a stop before we can run the
		// console walk animation
		if (!ONrCharacter_CanUseConsole(inCharacter)) {
			// the character has somehow been knocked out of a console-able state. abort.
			if (ONgDebugConsoleAction) {
				COrConsole_Printf("%s: aborting console wait-for-stop, invalid animtype %s",
									inCharacter, ONrAnimTypeToString(ioActiveCharacter->curAnimType));
			}
			ONrCharacter_ConsoleAction_Finish(inCharacter, UUcFalse);
			return;
		}

		if (ONrCharacter_IsStanding(inCharacter)) {
			// the character is standing still. proceed to the movement phase of the console action;
			// calculate what time we must finish this movement phase by.
			inCharacter->console_aborttime = ONrGameState_GetGameTime() + ONcConsoleAction_AbortTimer;
		}
	}

	// grab the action marker and console object...
	action_marker = inCharacter->console_marker;
	object = (OBJtObject*) action_marker->object;
	console_osd = &((OBJtOSD_All *)object->object_data)->osd.console_osd;

	// if we are already showing the console animation...
	if (is_running_a_console_animation)
	{
		action_frame = UUmMin(ioActiveCharacter->animation->actionFrame, ioActiveCharacter->animation->numFrames - 1);

		UUmAssert( action_frame != -1 );

		if (ioActiveCharacter->animFrame == action_frame)
		{
			// do console action here.....
			OBJrConsole_OnActivate( object, inCharacter );
			ONrCharacter_ConsoleAction_Finish( inCharacter, UUcTrue );
		}
		return;
	}

	new_position.x = action_marker->position.x;
	new_position.y = object->position.y;
	new_position.z = action_marker->position.z;

	distance = MUmVector_GetDistance( inCharacter->location, new_position );

	MUmVector_Subtract( new_vector, new_position, inCharacter->location );

	new_facing = MUrATan2( new_vector.x, new_vector.z );

	UUmTrig_Clip( new_facing );

	// if we are close enough to the console...
	if (distance < 2.f) {
		// face the console...
		ONrCharacter_SetFacing( inCharacter, action_marker->direction );

		ONrCharacter_Teleport(inCharacter, &new_position, UUcFalse);

		// stop
		ONrCharacter_DoAnimation( inCharacter, ioActiveCharacter, ONcAnimPriority_Force, ONcAnimType_Stand );

		// run the console animation...
		if (!ONrCharacter_IsReloading(inCharacter)) {
			const TRtAnimation *new_animation;

			new_animation = ONrCharacter_DoAnimation( inCharacter, ioActiveCharacter, ONcAnimPriority_Force, (console_osd->flags & OBJcConsoleFlag_Punch) ? ONcAnimType_Console_Punch : ONcAnimType_Console );

			if (NULL == new_animation) {
				COrConsole_Printf("failed to find proper console animation");

				new_animation = ONrCharacter_DoAnimation( inCharacter, ioActiveCharacter, ONcAnimPriority_Force, ONcAnimType_Console );
			}
		}
	}
	else
	{
		if (ONrGameState_GetGameTime() > inCharacter->console_aborttime) {
			// we have been trying for too long to reach the console; it is unreachable, give up
			ONrCharacter_ConsoleAction_Finish(inCharacter, UUcFalse);
			return;
		}

		if (ioActiveCharacter->curAnimType != ONcAnimType_Console_Walk) {
			const TRtAnimation *animation = TRrCollection_Lookup(inCharacter->characterClass->animations, ONcAnimType_Console_Walk, ioActiveCharacter->nextAnimState, ioActiveCharacter->animVarient);

			if (NULL != animation) {
				ONrCharacter_SetAnimation_External(inCharacter, ioActiveCharacter->nextAnimState, animation, 12);
			}
		}

		inCharacter->desiredFacing = action_marker->direction;

		MUmVector_Normalize(new_vector);
		MUmVector_Scale( new_vector, (float)UUmMin( 0.25, distance ) );
		new_position.x = inCharacter->location.x + new_vector.x;
		new_position.y = inCharacter->location.y + new_vector.y;
		new_position.z = inCharacter->location.z + new_vector.z;

		ONrCharacter_Teleport(inCharacter, &new_position, UUcFalse);
	}

	return;
}

// set up an animation intersection state for totoro to determine where a character is going
static void ONiSetupCharacterIntersectState(ONtCharacter *inCharacter, TRtCharacterIntersectState *outState)
{
	ONtActiveCharacter *active_character = ONrForceActiveCharacter(inCharacter);

	outState->character = (void *) inCharacter;
	outState->animation = (active_character == NULL) ? NULL : active_character->animation;
	outState->animFrame = (active_character == NULL) ? 0 : active_character->animFrame;
	outState->position.location = inCharacter->location;
	outState->position.facing = inCharacter->facing;
	outState->collision_leafsphere_radius = ONrCharacter_GetLeafSphereRadius(inCharacter);

	outState->position.inAir = UUcFalse;
	if ((active_character != NULL) && (active_character->inAirControl.numFramesInAir > 0)) {
		outState->position.inAir			= UUcTrue;
		outState->position.jumped			= ((active_character->inAirControl.flags & ONcInAirFlag_Jumped) != 0);
		outState->position.jumpKeyDown		= ((active_character->inputState.buttonIsDown & LIc_BitMask_Jump) != 0);
		outState->position.numFramesInAir	= active_character->inAirControl.numFramesInAir;
		outState->position.airVelocity		= active_character->inAirControl.velocity;
	}

	outState->fallGravity		= inCharacter->characterClass->airConstants.fallGravity;
	outState->jumpGravity		= inCharacter->characterClass->airConstants.jumpGravity;
	outState->jumpAmount		= inCharacter->characterClass->airConstants.jumpAmount;
	outState->maxVelocity		= inCharacter->characterClass->airConstants.maxVelocity;
	outState->jumpAccelAmount	= inCharacter->characterClass->airConstants.jumpAccelAmount;
	outState->framesFallGravity	= inCharacter->characterClass->airConstants.framesFallGravity;
	outState->numJumpAccelFrames= inCharacter->characterClass->airConstants.numJumpAccelFrames;

	// store the current values of the context's matrix and velocity estimates - used so that
	// the melee code can play with different hypothetical situations and revert to the original
	// ones if necessary.
	outState->original_position = outState->position;
	outState->dirty_position = UUcFalse;
}

// set up an animation intersection context for determining whether one character can strike another
void ONrSetupAnimIntersectionContext(ONtCharacter *inAttacker, ONtCharacter *inTarget,
									 UUtBool inConsiderAttack, TRtAnimIntersectContext *outContext)
{
	typedef struct tCylinderPart {
		UUtUns32	index;
		UUtBool		is_core;
		UUtBool		only_for_height;
	} tCylinderPart;
	static tCylinderPart cylinder_parts[] = {
		{ONcPelvis_Index,	UUcTrue,	UUcFalse},
		{ONcSpine_Index,	UUcTrue,	UUcFalse},
		{ONcSpine1_Index,	UUcTrue,	UUcFalse},
		{ONcHead_Index,		UUcTrue,	UUcFalse},
		{ONcLFoot_Index,	UUcTrue,	UUcTrue},
		{ONcRFoot_Index,	UUcTrue,	UUcTrue},
		{ONcLHand_Index,	UUcFalse,	UUcFalse},
		{ONcRHand_Index,	UUcFalse,	UUcFalse},
		{-1,				UUcFalse,	UUcFalse}
	};
	static const M3tMatrix4x3 max2oni = {{{1, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0, 0, 0}}};

	UUtUns32 itr;
	UUtInt32 part_index;
	float dist, max_dist, min_height, max_height, part_radius, costheta, sintheta;
	M3tMatrix4x3 matrix;
	M3tVector3D delta_position, part_position;
	TRtBody *body;
	M3tGeometry *part_geom;
	ONtActiveCharacter *target_active;
	UUtBool height_only, ignore_height_only;

	outContext->consider_attack = inConsiderAttack;
	ONiSetupCharacterIntersectState(inAttacker, &outContext->attacker);
	ONiSetupCharacterIntersectState(inTarget,	&outContext->target);

	// calculate a transformation from the target's (negated-Y-3DMax) animation space to oni worldspace
	matrix = max2oni;
	MUrMatrix_RotateY(&matrix, outContext->target.position.facing);

	// transform this into attacker-centered worldspace by adding a vector to the target's origin from
	// the attacker's origin
	MUmVector_Subtract(delta_position, outContext->target.position.location, outContext->attacker.position.location);
	MUrMatrix_SetTranslation(&matrix, &delta_position);

	// transform from worldspace to the attacker's (negated-Y-3DMax) animation space
	// note that because max2oni is a reflection, it is its own inverse.
	MUrMatrix_RotateY(&matrix, M3c2Pi - outContext->attacker.position.facing);
	MUrMatrix_Multiply(&max2oni, &matrix, &matrix);

	// store the target matrix
	outContext->dirty_matrix = UUcFalse;
	outContext->original_location_matrix = matrix;
	outContext->current_location_matrix = matrix;

	target_active = ONrGetActiveCharacter(inTarget);
	if (target_active == NULL) {
		// no velocity, and just a simple bounding cylinder
		MUmVector_Set(outContext->target_velocity_estimate, 0, 0, 0);
		outContext->target_height_min = 0;
		outContext->target_height_max = ONcCharacterHeight;
		outContext->target_cylinder_radius = ONrCharacter_GetLeafSphereRadius(inTarget);
		return;
	}

	MUmVector_Subtract(delta_position, inTarget->location, inTarget->prev_location);
	if ((target_active->numWallCollisions > 0) || (target_active->numPhysicsCollisions > 0) || (target_active->numCharacterCollisions > 0)) {
		// because the target is colliding, we can't rely on their animation for a velocity estimate.
		// use their actual change in movement since the last frame.

		// transform this into XY animspace for the target
		costheta = MUrCos(inTarget->facing);	sintheta = MUrSin(inTarget->facing);
		outContext->target_velocity_estimate.y = sintheta * delta_position.x + costheta * delta_position.z;
		outContext->target_velocity_estimate.x = costheta * delta_position.x - sintheta * delta_position.x;
	} else {
		// get a velocity estimate for the target - in XZ space, then transform to XY animspace.
		ONrCharacter_GetVelocityEstimate(inTarget, target_active, &outContext->target_velocity_estimate);
		outContext->target_velocity_estimate.y = outContext->target_velocity_estimate.z;
	}

	// use the delta-position to get the target's vertical motion
	outContext->target_velocity_estimate.z = delta_position.y;

	// if the target has fallen down, use their legs for cylinder radius checking; don't otherwise (this ensures that
	// we don't construct a ridiculously small bounding cylinder for fallen opponents)
	ignore_height_only = ONrAnimState_IsFallen(target_active->nextAnimState);

	// get simple bounds on the target
	itr = 0;
	max_dist = 0; min_height = 1e09; max_height = -1e09;
	body = ONrCharacter_GetBody(inTarget, TRcBody_SuperLow);

	while (1) {
		part_index = cylinder_parts[itr].index;
		if (part_index == -1)
			break;

		// we use all the listed parts when we are considering a defense, so that we make a
		// wide bounding cylinder for the AI to try and move out of the way of the
		// incoming attack.
		//
		// otherwise we only consider 'core' parts, and the legs are marked as height-only.
		// this results in tight bounding cylinders around people when we consider whether we
		// can hit them with a given attack.
		if (inConsiderAttack && !cylinder_parts[itr].is_core)
			break;
		height_only = (!ignore_height_only && inConsiderAttack && cylinder_parts[itr].only_for_height);
		itr++;

		// get the geometry for this part
		UUmAssert((part_index >= 0) && (part_index < body->numParts));
		part_geom = body->geometries[part_index];
		part_radius = part_geom->pointArray->boundingSphere.radius;

		// get the location of this part's bounding sphere's center relative to the Max base location
		part_position = MUrMatrix_GetTranslation(&target_active->matricies[part_index]);
		MUmVector_Add(part_position, part_geom->pointArray->boundingSphere.center, part_position);
		MUmVector_Subtract(part_position, part_position, inTarget->location);

		if (!height_only) {
			// compute the 2D distance that this part gets from the Max base location
			dist = MUrSqrt(UUmSQR(part_position.x) + UUmSQR(part_position.z)) + part_radius;
			max_dist = UUmMax(dist, max_dist);
		}

		// update the character's height bounds
		max_height = UUmMax(max_height, part_position.y + part_radius);
		min_height = UUmMin(min_height, part_position.y - part_radius);
	}

	// if we are considering an attack against a target, make the bounding cylinder tighter so that
	// we don't run the risk of missing them barely.
	if (inConsiderAttack) {
		max_dist *= AI2cMelee_TargetCylinderSafetyScale;
	}

	// store this crude bounding cylinder for the target
	outContext->target_height_min = min_height;
	outContext->target_height_max = max_height;
	outContext->target_cylinder_radius = max_dist;
}

// revert an animation intersection context to its original values
void ONrRevertAnimIntersectionContext(TRtAnimIntersectContext *ioContext)
{
	// restore the original target matrix
	if (ioContext->dirty_matrix) {
		ioContext->current_location_matrix = ioContext->original_location_matrix;
	}

	// restore the characters' original positions
	if (ioContext->attacker.dirty_position) {
		ioContext->attacker.position = ioContext->attacker.original_position;
	}

	if (ioContext->target.dirty_position) {
		ioContext->target.position = ioContext->target.original_position;
	}
}

//
// clip codes
//

static UUtUns32 ONrClipLinePosX(const M3tPoint3D *inPoint1, const M3tPoint3D *inPoint2, float x, M3tPoint3D *outNewPoints)
{
	UUtUns32 clip_index;
	UUtUns32 num_points;
	const M3tPoint3D *x_larger;
	const M3tPoint3D *x_smaller;

	if (inPoint1->x >= inPoint2->x) {
		x_larger = inPoint1;
		x_smaller = inPoint2;
		clip_index = 1;
	}
	else {
		x_larger = inPoint2;
		x_smaller = inPoint1;
		clip_index = 2;
	}

	if (x_smaller->x >= x) {
		num_points = 0;
	}
	else if (x_larger->x <= x) {
		num_points = 1;
		outNewPoints[0] = *inPoint2;
	}
	else {
		// clip x_larger to x;
		float d = (x - x_smaller->x) / (x_larger->x - x_smaller->x);

		UUmAssert((d >= 0) && (d <= 1));

		outNewPoints[0].x = x;
		outNewPoints[0].y = ((x_larger->y - x_smaller->y) * d) + x_smaller->y;
		outNewPoints[0].z = ((x_larger->z - x_smaller->z) * d) + x_smaller->z;

		if (1 == clip_index) {
			outNewPoints[1] = *inPoint2;
			num_points = 2;
		}
		else {
			num_points = 1;
		}
	}

	return num_points;
}

static UUtUns32 ONrClipLineNegX(const M3tPoint3D *inPoint1, const M3tPoint3D *inPoint2, float x, M3tPoint3D *outNewPoints)
{
	UUtUns32 clip_index;
	UUtUns32 num_points;
	const M3tPoint3D *x_larger;
	const M3tPoint3D *x_smaller;

	if (inPoint1->x >= inPoint2->x) {
		x_larger = inPoint1;
		x_smaller = inPoint2;
		clip_index = 2;
	}
	else {
		x_larger = inPoint2;
		x_smaller = inPoint1;
		clip_index = 1;
	}

	if (x_larger->x <= x) {
		num_points = 0;
	}
	else if (x_smaller->x >= x) {
		num_points = 1;
		outNewPoints[0] = *inPoint2;
	}
	else {
		// clip x_larger to x;
		float d = (x - x_smaller->x) / (x_larger->x - x_smaller->x);

		UUmAssert((d >= 0) && (d <= 1));

		outNewPoints[0].x = x;
		outNewPoints[0].y = ((x_larger->y - x_smaller->y) * d) + x_smaller->y;
		outNewPoints[0].z = ((x_larger->z - x_smaller->z) * d) + x_smaller->z;

		if (1 == clip_index) {
			outNewPoints[1] = *inPoint2;
			num_points = 2;
		}
		else {
			num_points = 1;
		}
	}

	return num_points;
}

static UUtUns32 ONrClipLinePosZ(const M3tPoint3D *inPoint1, const M3tPoint3D *inPoint2, float z, M3tPoint3D *outNewPoints)
{
	UUtUns32 clip_index;
	UUtUns32 num_points;
	const M3tPoint3D *z_larger;
	const M3tPoint3D *z_smaller;

	if (inPoint1->z >= inPoint2->z) {
		z_larger = inPoint1;
		z_smaller = inPoint2;
		clip_index = 1;
	}
	else {
		z_larger = inPoint2;
		z_smaller = inPoint1;
		clip_index = 2;
	}

	if (z_smaller->z >= z) {
		num_points = 0;
	}
	else if (z_larger->z <= z) {
		num_points = 1;
		outNewPoints[0] = *inPoint2;
	}
	else {
		// clip z_larger to z;
		float d = (z - z_smaller->z) / (z_larger->z - z_smaller->z);

		UUmAssert((d >= 0) && (d <= 1));

		outNewPoints[0].x = ((z_larger->x - z_smaller->x) * d) + z_smaller->x;
		outNewPoints[0].y = ((z_larger->y - z_smaller->y) * d) + z_smaller->y;
		outNewPoints[0].z = z;

		if (1 == clip_index) {
			outNewPoints[1] = *inPoint2;
			num_points = 2;
		}
		else {
			num_points = 1;
		}
	}

	return num_points;
}

static UUtUns32 ONrClipLineNegZ(const M3tPoint3D *inPoint1, const M3tPoint3D *inPoint2, float z, M3tPoint3D *outNewPoints)
{
	UUtUns32 clip_index;
	UUtUns32 num_points;
	const M3tPoint3D *z_larger;
	const M3tPoint3D *z_smaller;

	if (inPoint1->z >= inPoint2->z) {
		z_larger = inPoint1;
		z_smaller = inPoint2;
		clip_index = 2;
	}
	else {
		z_larger = inPoint2;
		z_smaller = inPoint1;
		clip_index = 1;
	}

	if (z_larger->z <= z) {
		num_points = 0;
	}
	else if (z_smaller->z >= z) {
		num_points = 1;
		outNewPoints[0] = *inPoint2;
	}
	else {
		// clip z_larger to z;
		float d = (z - z_smaller->z) / (z_larger->z - z_smaller->z);

		UUmAssert((d >= 0) && (d <= 1));

		outNewPoints[0].x = ((z_larger->x - z_smaller->x) * d) + z_smaller->x;
		outNewPoints[0].y = ((z_larger->y - z_smaller->y) * d) + z_smaller->y;
		outNewPoints[0].z = z;

		if (1 == clip_index) {
			outNewPoints[1] = *inPoint2;
			num_points = 2;
		}
		else {
			num_points = 1;
		}
	}

	return num_points;
}

static UUtBool GetQuadRangeClipped(UUtInt32 inNumPoints, const M3tPoint3D **inPoints, float inMinX, float inMaxX, float inMinZ, float inMaxZ, float inMaxY, float *outHeight)
{
	UUtInt32 current_total_points = inNumPoints;
	UUtInt32 new_total_points;
	M3tPoint3D clipped_points0[16];
	M3tPoint3D clipped_points1[16];
	UUtBool accept = UUcFalse;
	float new_height = UUcFloat_Min;
	UUtInt32 itr;

	// clip to pos x
	new_total_points = 0;
	for(itr = 0; itr < (current_total_points - 1); itr++)
	{
		new_total_points += ONrClipLinePosX(inPoints[itr], inPoints[(itr + 1)], inMaxX, clipped_points0 + new_total_points);
	}
	new_total_points += ONrClipLinePosX(inPoints[current_total_points - 1], inPoints[0], inMaxX, clipped_points0 + new_total_points);
	if (0 == new_total_points) { goto exit; }

	// clip to neg z
	current_total_points = new_total_points;
	new_total_points = 0;
	for(itr = 0; itr < (current_total_points - 1); itr++)
	{
		new_total_points += ONrClipLineNegZ(clipped_points0 + itr, clipped_points0 + itr + 1, inMinZ, clipped_points1 + new_total_points);
	}
	new_total_points += ONrClipLineNegZ(clipped_points0 + current_total_points - 1, clipped_points0 + 0, inMinZ, clipped_points1 + new_total_points);
	if (0 == new_total_points) { goto exit; }

	// clip to neg x
	current_total_points = new_total_points;
	new_total_points = 0;
	for(itr = 0; itr < (current_total_points - 1); itr++)
	{
		new_total_points += ONrClipLineNegX(clipped_points1 + itr, clipped_points1 + itr + 1, inMinX, clipped_points0 + new_total_points);
	}
	new_total_points += ONrClipLineNegX(clipped_points1 + current_total_points - 1, clipped_points1 + 0, inMinX, clipped_points0 + new_total_points);
	if (0 == new_total_points) { goto exit; }

	// clip to pos z
	current_total_points = new_total_points;
	new_total_points = 0;
	for(itr = 0; itr < (current_total_points - 1); itr++)
	{
		new_total_points += ONrClipLinePosZ(clipped_points0 + itr, clipped_points0 + itr + 1, inMaxZ, clipped_points1 + new_total_points);
	}
	new_total_points += ONrClipLinePosZ(clipped_points0 + current_total_points - 1, clipped_points0 + 0, inMaxZ, clipped_points1 + new_total_points);
	if (0 == new_total_points) { goto exit; }

	if (new_total_points >= 3) {
		for(itr = 0; itr < new_total_points; itr++)
		{
			float y0 = clipped_points1[itr].y;
			float y1 = clipped_points1[(itr + 1) % new_total_points].y;
			float ymin = UUmMin(y0, y1);
			float ymax = UUmMax(y0, y1);

			// each line segment has a y-span.  if that y-span is totally above inMax ignore
			if (ymin < inMaxY) {
				accept = UUcTrue;
				new_height = UUmMax(ymax, new_height);
			}
		}

		*outHeight = UUmMin(new_height, inMaxY);
	}

exit:
	return accept;
}

void ONrCorpse_Create_VisibleList(ONtCorpse *corpse)
{
	AKrEnvironment_NodeList_Get(&corpse->corpse_data.corpse_bbox, 32, corpse->node_list, 0);

	return;
}

static void ONrCorpse_Create(ONtCharacter *inCharacter)
{
	ONtCorpse *corpses = ONgLevel->corpseArray->corpses;
	UUtUns32 count = ONgLevel->corpseArray->max_corpses;
	UUtUns32 static_count = ONgLevel->corpseArray->static_corpses;
	UUtUns32 next = ONgLevel->corpseArray->next;
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character == NULL)
		return;

	{
		ONtCorpse *new_corpse = corpses + next;

		strcpy(new_corpse->corpse_name, "dynamic-corpse");
		new_corpse->corpse_data.characterClass = inCharacter->characterClass;

		UUrMemory_MoveFast(active_character->matricies, new_corpse->corpse_data.matricies, sizeof(new_corpse->corpse_data.matricies));
		new_corpse->corpse_data.corpse_bbox = inCharacter->boundingBox;

		ONrCorpse_Create_VisibleList(new_corpse);
	}

	next++;

	if (next >= count) {
		next = static_count;
	}

	ONgLevel->corpseArray->next = next;

	return;
}



static void  ONrCorpse_LevelBegin(void)
{
	ONtCorpse *corpses = ONgLevel->corpseArray->corpses;
	UUtUns32 count = ONgLevel->corpseArray->static_corpses;
	UUtUns32 itr;

	for(itr = 0; itr < count; itr++)
	{
		ONtCorpse *current_corpse = corpses + itr;

		ONrCorpse_Create_VisibleList(current_corpse);
	}

	return;
}


static void ONrCorpse_Print_NodeList(UUtUns32 *inList)
{
	char buffer[2084];
	UUtInt32 count = inList[0];
	UUtInt32 itr;

	sprintf(buffer, "%d", inList[0]);

	for(itr = 0; itr < count; itr++)
	{
		sprintf(buffer + strlen(buffer), " %d", inList[itr + 1]);
	}

	COrConsole_Printf("%s", buffer);

	return;
}


static void  ONrCorpse_Display(M3tPoint3D *inCameraLocation)
{
	ONtCorpse *corpses = ONgLevel->corpseArray->corpses;
	UUtUns32 count = ONgLevel->corpseArray->max_corpses;
	UUtUns32 static_count = ONgLevel->corpseArray->static_corpses;
	UUtUns32 itr;
	UUtBool skip_cosmetic_corpses = !ONrMotoko_GraphicsQuality_SupportCosmeticCorpses();

	for(itr = 0; itr < count; itr++)
	{
		ONtCorpse *this_corpse = corpses + itr;

		if (('_' == this_corpse->corpse_name[0]) && (skip_cosmetic_corpses)) {
			continue;
		}

		{
			UUtBool corpse_is_visible;

			ONtCorpse_Data *this_corpse_data = &this_corpse->corpse_data;
			ONtCharacterClass *character_class = this_corpse_data->characterClass;

			if (NULL == character_class) {
				continue;
			}

			// ONrCorpse_Print_NodeList(this_corpse->node_list);

			corpse_is_visible  = AKrEnvironment_NodeList_Visible(this_corpse->node_list);

			if (corpse_is_visible) {
				if (character_class != NULL)
				{
					TRtBodySet *set = character_class->body;
					TRtBody *corpse_body = set->body[TRcBody_SuperLow];

					if (ONrMotoko_GraphicsQuality_SupportHighQualityCorpses()) {
						corpse_body = set->body[TRcBody_Low];
					}

					TRrBody_SetMaps(corpse_body, character_class->textures);
					TRrBody_Draw(corpse_body, NULL, this_corpse_data->matricies, TRcBody_DrawAll, 0, 0, 0, 0);
				}
			}
		}
	}

	return;
}

static IMtShade ONiCharacter_GetAnimParticleTint(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	ONtCharacter *tint_character;

	if (ioActiveCharacter->throwTarget == NULL) {
		tint_character = ioCharacter;
	} else {
		tint_character = ioActiveCharacter->throwTarget;
	}

	return ONrCharacter_GetHealthShade(tint_character->hitPoints, tint_character->maxHitPoints);
}

IMtShade ONrCharacter_GetHealthShade(UUtUns32 inHitPoints, UUtUns32 inMaxHitPoints)
{
	float val, interp;
	UUtUns32 itr;

	val = ((float) inHitPoints) / inMaxHitPoints;
	if (val <= ONgGameSettings->healthcolor_values[0]) {
		// minimum shade
		return ONgGameSettings->healthcolor_colors[0];

	} else if (val >= ONgGameSettings->healthcolor_values[ONgGameSettings->healthcolor_numpoints - 1]) {
		// maximum shade
		return ONgGameSettings->healthcolor_colors[ONgGameSettings->healthcolor_numpoints - 1];
	}

	// find the two points on the ramp that bracket the current value
	for (itr = 1; itr < ONgGameSettings->healthcolor_numpoints; itr++) {
		if (val < ONgGameSettings->healthcolor_values[itr]) {
			break;
		}
	}
	UUmAssert(itr < ONgGameSettings->healthcolor_numpoints);
	interp = (val - ONgGameSettings->healthcolor_values[itr - 1]) / (ONgGameSettings->healthcolor_values[itr] - ONgGameSettings->healthcolor_values[itr - 1]);
	interp = UUmPin(interp, 0, 1);

	return IMrShade_Interpolate(ONgGameSettings->healthcolor_colors[itr - 1], ONgGameSettings->healthcolor_colors[itr], interp);
}

void ONrCharacter_Invis_Update(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUtUns16 invis_remaining = ioCharacter->inventory.invisibilityRemaining;
	UUtUns16 action_frame, duration;
	UUtBool teleporting;
	float teleport_invis;

	if ((invis_remaining > 0) && (!ioActiveCharacter->invis_active)) {
		ioActiveCharacter->invis_character_alpha = 1.f;
		ioActiveCharacter->invis_active = UUcTrue;
	}
	else if ((invis_remaining > 0) || (ioActiveCharacter->invis_active)) {

		const UUtUns32 fade_out = 120;
		float goal_invis_character_alpha = (invis_remaining > fade_out) ? 0.f : 1.f;
		float fade_in_alpha_speed = 0.02f;
		float fade_out_alpha_speed = 0.0085f;

		if (ioActiveCharacter->invis_character_alpha > goal_invis_character_alpha) {
			ioActiveCharacter->invis_character_alpha = UUmMax(ioActiveCharacter->invis_character_alpha - fade_out_alpha_speed, goal_invis_character_alpha);
		}
		else if (ioActiveCharacter->invis_character_alpha < goal_invis_character_alpha) {
			ioActiveCharacter->invis_character_alpha = UUmMin(ioActiveCharacter->invis_character_alpha + fade_in_alpha_speed, goal_invis_character_alpha);
		}

		if ((0.f == ioActiveCharacter->invis_character_alpha) && (0 == invis_remaining)) {
			ioActiveCharacter->invis_active = UUcFalse;
		}
	}

	if ((invis_remaining > 0) && (invis_remaining != (UUtUns16) -1)) {
		ioCharacter->inventory.invisibilityRemaining = invis_remaining - 1;
	}

	// CB: calculate temporary invisibility caused by a teleport animation...
	// temporary_invis goes from 0 (visible) to 1 (invisible)
	action_frame = TRrAnimation_GetActionFrame(ioActiveCharacter->animation);
	duration = TRrAnimation_GetDuration(ioActiveCharacter->animation);
	teleporting = UUcFalse;

	if (ioActiveCharacter->curAnimType == ONcAnimType_Teleport_In) {
		teleporting = (ioActiveCharacter->animFrame >= action_frame);
		teleport_invis = ((float) (ioActiveCharacter->animFrame + 1 - action_frame)) / (duration - action_frame);

	} else if (ioActiveCharacter->curAnimType == ONcAnimType_Teleport_In) {
		teleporting = (ioActiveCharacter->animFrame <= action_frame);
		teleport_invis = ((float) (action_frame + 1 - ioActiveCharacter->animFrame)) / (action_frame + 1);
	}

	if (teleporting) {
		// turn invisibility value into an alpha value
		teleport_invis = 1.0f - teleport_invis;

		// apply this alpha to the current invisibility
		ioActiveCharacter->invis_character_alpha = UUmPin(teleport_invis, 0.0f, ioActiveCharacter->invis_character_alpha);
		ioCharacter->flags |= ONcCharacterFlag_Teleporting;
	} else {
		ioCharacter->flags &= ~ONcCharacterFlag_Teleporting;
	}

	return;
}

void ONrCharacter_Shield_Update(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUtUns16 shield_amount;
	UUtInt32 shield_base_line;
	UUtInt32 shield_bonus;

	if ((ioCharacter->characterClass->superShield) && (ioCharacter->super_amount > 0)) {
		// the character has a super shield effect
		ioActiveCharacter->daodan_shield_effect = UUcTrue;
		shield_amount = (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(ioCharacter->super_amount * WPcMaxShields);
		shield_base_line = 39;
		shield_bonus = 20;
	} else {
		// regular force field
		ioActiveCharacter->daodan_shield_effect = UUcFalse;
		shield_amount = ioCharacter->inventory.shieldDisplayAmount;
		shield_base_line = 69;
		shield_bonus = 20;
	}

	if ((ioActiveCharacter->shield_active) || (shield_amount > 0)) {
		UUtInt32 shield_global = (shield_base_line * shield_amount) / WPcMaxShields;
		UUtUns32 shield_itr;
		UUtBool new_shield_active = UUcFalse;

		if (shield_amount > 0) {
			shield_global += shield_bonus;
		}
		shield_global = UUmMin(shield_global, UUcMaxUns8);

		for(shield_itr = 0; shield_itr < ONcNumCharacterParts; shield_itr++)
		{
			UUtInt32 old_value = ioActiveCharacter->shield_parts[shield_itr];
			UUtInt32 new_value = old_value;

			if (old_value > shield_global) {
				new_value = UUmMax((old_value - 7), shield_global);
			}
			else if (ioCharacter->flags & ONcCharacterFlag_Dead) {
				if (ONgGameState->gameTime % 3) {
					new_value = UUmMax(old_value - 1, 0);
				}
			}
			else if (old_value < shield_global) {
				new_value = UUmMin((old_value + 1), shield_global);
			}

			if (new_value > 0) {
				new_shield_active = UUcTrue;
			}

			UUmAssert(new_value <= UUcMaxUns8);
			ioActiveCharacter->shield_parts[shield_itr] = (UUtUns8) new_value;
		}

		ioActiveCharacter->shield_active = new_shield_active;
	}

	return;
}

void ONrCharacter_Shield_HitParts(ONtCharacter *ioCharacter, UUtUns32 inMask)
{
	UUtUns32 itr;
	UUtInt32 shield_hit_increment = 255;
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);

	if (active_character == NULL) {
		return;
	}

	if ((active_character->shield_active) || (ioCharacter->flags & ONcCharacterFlag_BossShield)) {
		active_character->shield_active = UUcTrue;

		for(itr = 0; itr < ONcNumCharacterParts; itr++)
		{
			if (inMask & (1 << itr)) {
				UUtInt32 old_value = active_character->shield_parts[itr];
				UUtInt32 new_value = UUmMin(UUcMaxUns8, old_value + shield_hit_increment);

				active_character->shield_parts[itr] = (UUtUns8) new_value;
			}
		}
	}

	return;
}

UUtUns32 ONrCharacter_FindParts(const ONtCharacter *inCharacter, M3tPoint3D *inLocation, float inRadius)
{
	UUtUns32 itr;
	UUtUns32 result = 0;
	const TRtBody *body = inCharacter->characterClass->body->body[TRcBody_SuperLow];
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character == NULL) {
		return 0;
	}

	for(itr = 0; itr < ONcNumCharacterParts; itr++)
	{
		float distance_squared;
		const M3tGeometry *geometry = body->geometries[itr];

		if (NULL != body->geometries[itr]) {
			M3tPoint3D part_location;

			MUrMatrix_MultiplyPoints(1, active_character->matricies + itr, &geometry->pointArray->boundingSphere.center, &part_location);

			distance_squared = MUmVector_GetDistanceSquared(part_location, *inLocation);

			if (distance_squared < UUmSQR(inRadius + geometry->pointArray->boundingSphere.radius)) {
				result |= 1 << itr;
			}
		}
	}

	return result;
}

UUtUns32 ONrCharacter_FindClosestPart(const ONtCharacter *inCharacter, M3tPoint3D *inLocation)
{
	UUtUns32 itr;
	const TRtBody *body = inCharacter->characterClass->body->body[TRcBody_SuperLow];
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);
	float best_distance_so_far_squared = UUcFloat_Max;
	UUtUns32 result = ONcSpine_Index;

	if (NULL == active_character) {
		goto exit;
	}

	for(itr = 0; itr < ONcNumCharacterParts; itr++)
	{
		float distance_squared;
		const M3tGeometry *geometry = body->geometries[itr];

		if (NULL != body->geometries[itr]) {
			M3tPoint3D part_location;

			MUrMatrix_MultiplyPoints(1, active_character->matricies + itr, &geometry->pointArray->boundingSphere.center, &part_location);

			distance_squared = MUmVector_GetDistanceSquared(part_location, *inLocation);

			if (distance_squared < best_distance_so_far_squared) {
				result = itr;
				best_distance_so_far_squared = distance_squared;
			}
		}
	}

exit:
	return result;
}


static void ONrCharacter_CalculateAimToTarget(const ONtCharacter *inCharacter, M3tPoint3D *inLocation, float *outLR, float *outUD)
{
	float aimingLR;
	float aimingUD;

	float lr_angle_to_target;
	float ud_angle_to_target;

	const float moveAmt = 0.1f;
	float height;
	float length_to_target;
	M3tVector3D vector_to_target;
	M3tVector3D vector_to_target_xz;
	M3tPoint3D shoulder;

	shoulder = inCharacter->location;
	shoulder.y+=ONcCharacterHeight;
	MUmVector_Subtract(vector_to_target, shoulder, *inLocation);

	// xx bale if vector_to_target is zero length

	length_to_target = MUmVector_GetLength(vector_to_target);
	height = vector_to_target.y;
	MUmVector_Scale(vector_to_target,1.0f / length_to_target);

	vector_to_target_xz.x = vector_to_target.x;
	vector_to_target_xz.y = 0.f;
	vector_to_target_xz.z = vector_to_target.z;
	MUmVector_Normalize(vector_to_target_xz);

	lr_angle_to_target = -(M3cHalfPi + MUrATan2(vector_to_target_xz.z, vector_to_target_xz.x));
	ud_angle_to_target = MUrASin((float)fabs(height) / length_to_target);

	if (vector_to_target.y > 0) {
		ud_angle_to_target = -ud_angle_to_target;
	}

	aimingUD = ud_angle_to_target;
	aimingLR = lr_angle_to_target - inCharacter->facing;

	if (aimingLR > M3cPi) {
		aimingLR -= M3c2Pi;
	}
	else if (aimingLR < -M3cPi) {
		aimingLR += M3c2Pi;
	}

	*outLR = aimingLR;
	*outUD = aimingUD;

	return;
}

UUtBool ONrCharacter_AutoAim(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter, ONtCharacter *target)
{
	M3tPoint3D calculate_aim_to_target_point;
	float lr;
	float ud;
	float lr_delta;
	float ud_delta;

	float non_auto_aiming_lr;
	float non_auto_aiming_ud;

	float snap_distance = 0.15f;
	float max_distance = 0.35f;

	const ONtCharacterClass *target_class = target->characterClass;
	const TRtBody *body = ioCharacter->characterClass->body->body[TRcBody_SuperLow];
	M3tGeometry *target_geometry = body->geometries[ONcSpine_Index];
	UUtBool did_auto_aim = UUcFalse;

	M3tPoint3D src_location;
	M3tPoint3D shoot_location;

	UUmAssert(!ONrCharacter_IsAI(ioCharacter));

	if (NULL == target) {
		goto exit;
	}

	if (ioCharacter == target) {
		goto exit;
	}

	if (NULL == ioCharacter->inventory.weapons[0]) {
		goto exit;
	}

	if (target->flags & ONcCharacterFlag_Dead) {
		goto exit;
	}

	if (target->num_frames_offscreen > 0) {
		goto exit;
	}

	if (MUrPoint_Distance_Squared(&ioCharacter->location, &target->location) > UUmSQR(500.f)) {
		goto exit;
	}

	if (!gWeaponAutoAim) {
		goto exit;
	}

	if (AI2cTeam_Hostile != AI2rTeam_FriendOrFoe(ioCharacter->teamNumber, target->teamNumber)) {
		goto exit;
	}

	shoot_location = target->location;
	shoot_location.y += target->heightThisFrame * 0.5f;

	src_location = ioCharacter->location;
	src_location.y += ioCharacter->heightThisFrame;

	// draw a line from me to the target
	{
		M3tPoint3D vector;

		MUmVector_Subtract(vector, src_location, shoot_location);

		if (AKrCollision_Point(ONgLevel->environment, &shoot_location, &vector, AKcGQ_Flag_LOS_CanSee_Skip_Player, 0)) {
			goto exit;
		}
	}

	{
		M3tVector3D aiming_vector;

		ONrCreateTargetingVector(
			&shoot_location,
			&src_location,
			&WPrGetClass(ioCharacter->inventory.weapons[0])->ai_parameters,
			ioActiveCharacter->matricies + ONcWeapon_Index,
			&aiming_vector);

		// aim in this direction... this vector is added to our shoulder height
		MUmVector_Add(calculate_aim_to_target_point, src_location, aiming_vector);
		calculate_aim_to_target_point.y += ONcCharacterHeight;
	}

	ONrCharacter_CalculateAimToTarget(ioCharacter, &calculate_aim_to_target_point, &lr, &ud);

	non_auto_aiming_lr = ioActiveCharacter->aimingLR;
	non_auto_aiming_ud = 0.f;

	non_auto_aiming_lr += ONrCharacter_GetDesiredFacingOffset(ioCharacter);
	non_auto_aiming_lr -= ioCharacter->facingModifier;
	non_auto_aiming_ud += ioActiveCharacter->aimingUD;

	lr_delta = lr - non_auto_aiming_lr;
	ud_delta = ud - non_auto_aiming_ud;

	if (((float)fabs(lr_delta) < max_distance) && ((float)fabs(ud_delta) < max_distance)) {
		float distance = (float)UUmMax(fabs(lr_delta), fabs(ud_delta));
		float percent;

		if (distance < snap_distance) {
			percent = 1.f;
		}
		else {
			percent = 1.f - ((distance - snap_distance) / (max_distance - snap_distance));
		}

		ioActiveCharacter->autoAimAdjustmentLR = lr_delta * percent;
		ioActiveCharacter->autoAimAdjustmentUD = ud_delta * percent;

		did_auto_aim = UUcTrue;
	}


exit:
	return did_auto_aim;
}

// force a character to holster their weapon
void ONrCharacter_CinematicHolster(ONtCharacter *inCharacter)
{
	ONtActiveCharacter *active_character;

	if (inCharacter->inventory.weapons[0] == NULL) {
		return;
	}

	active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character != NULL) {
		ONrCharacter_Holster(inCharacter, active_character, UUcTrue);
	}

	return;
}

// a cinematic or data-console is over. draw the holstered weapon if it is magic, or if force-draw is set
void ONrCharacter_CinematicEndHolster(ONtCharacter *inCharacter, UUtBool inForceDraw)
{
	UUtBool weapon_in_magic_slot = inCharacter->inventory.weapons[WPcMagicSlot] != NULL;

	if (!weapon_in_magic_slot) {
		return;
	}

	if (inCharacter->inventory.weapons[0] != NULL) {
		return;
	}

	{
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

		if (active_character != NULL) {
			ONrCharacter_Draw(inCharacter, active_character, NULL);
		}
	}
}

static UUtBool ONiCharacter_IsVectorThroughWall(M3tPoint3D *inPoint, M3tVector3D *inVector)
{
	UUtBool intersection;
	M3tVector3D unit_vector;
	float test_dist;

	intersection = AKrCollision_Point(ONgLevel->environment, inPoint, inVector, AKcGQ_Flag_Obj_Col_Skip, 1);
	if (intersection) {
		return UUcTrue;
	}

	test_dist = MUmVector_GetLength(*inVector);
	if (test_dist > 1e-03f) {
		MUmVector_ScaleCopy(unit_vector, 1.0f / test_dist, *inVector);

		intersection = AMrRayToObject(inPoint, &unit_vector, test_dist, NULL, NULL);
		if (intersection) {
			return UUcTrue;
		}
	}

	return UUcFalse;
}

UUtBool ONrCharacter_IsGunThroughWall(ONtCharacter *inCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUtBool gun_is_through_wall = UUcFalse;
	TRtBody *body = ONrCharacter_GetBody(inCharacter, TRcBody_SuperHigh);
	M3tPoint3D pelvis_center;
	WPtWeapon *weapon = inCharacter->inventory.weapons[0];
	WPtWeaponClass *weapon_class = WPrGetClass(weapon);
	M3tMatrix4x3 *weapon_matrix = ioActiveCharacter->matricies + ONcWeapon_Index;

	MUrMatrix_MultiplyPoint(&body->geometries[ONcPelvis_Index]->pointArray->boundingSphere.center, ioActiveCharacter->matricies + ONcPelvis_Index, &pelvis_center);


	{
		UUtUns32 attachment_index;

		for (attachment_index = 0; attachment_index < weapon_class->attachment_count; attachment_index++)
		{
			WPtParticleAttachment *attachment = weapon_class->attachment + attachment_index;

			if (attachment->ammo_used > 0) {
				// this attachment is a shooter, we must check it's not through a wall
				M3tPoint3D p_position;
				M3tPoint3D world_p_position;
				M3tPoint3D pelvis_center_to_world_p;

				MUrMatrix_GetCol(&attachment->matrix, 3, &p_position);
				MUrMatrix_MultiplyPoint(&p_position, weapon_matrix, &world_p_position);

				MUmVector_Subtract(pelvis_center_to_world_p, world_p_position, pelvis_center);

				gun_is_through_wall = ONiCharacter_IsVectorThroughWall(&pelvis_center, &pelvis_center_to_world_p);
				if (gun_is_through_wall) {
					goto exit;
				}
			}
		}
	}

	{
		M3tPoint3D pivot_point_of_gun;
		M3tPoint3D pelvis_center_to_gun;

		pivot_point_of_gun = MUrMatrix_GetTranslation(weapon_matrix);
		MUmVector_Subtract(pelvis_center_to_gun, pivot_point_of_gun, pelvis_center);

		gun_is_through_wall = ONiCharacter_IsVectorThroughWall(&pelvis_center, &pelvis_center_to_gun);
	}

	// NOTE: if the character is shooting over a ledge with a long weapon this function may fail when it should
	// pass, fix later. -Michael Evans july 1, 2000

exit:
	return gun_is_through_wall;
}

const char *ONrAnimTypeToString(TRtAnimType inAnimType)
{
	const char *anim_type_string = "unknown-anim-type";
	UUtUns32 itr;

	for(itr = 0; ONcAnimTypeToStringTable[itr].string != NULL; itr++)
	{
		if (ONcAnimTypeToStringTable[itr].type == inAnimType) {
			anim_type_string = ONcAnimTypeToStringTable[itr].string;
			break;
		}
	}

	UUmAssert(anim_type_string != NULL);
	return anim_type_string;
}

static int UUcExternal_Call ONiQSort_AnimTypeToStringTableCompare(const void *pptr1, const void *pptr2)
{
	ONtAnimTypeToStringTable *e1 = ((ONtAnimTypeToStringTable *) pptr1), *e2 = ((ONtAnimTypeToStringTable *) pptr2);

	return UUrString_Compare_NoCase(e1->string, e2->string);
}

TRtAnimType ONrStringToAnimType(const char *inString)
{
	ONtAnimTypeToStringTable *entry;

	if (!ONgAnimTypeToStringTableSorted) {
		// determine the length of the animtype table
		entry = ONcAnimTypeToStringTable;
		while (entry->string != NULL) {
			entry++;
		}

		// sort the animtype table by anim type name
		ONgAnimTypeToStringTableSorted = UUcTrue;
		ONgAnimTypeToStringTableLength = entry - ONcAnimTypeToStringTable;
		qsort(ONcAnimTypeToStringTable, ONgAnimTypeToStringTableLength,
				sizeof(ONtAnimTypeToStringTable), ONiQSort_AnimTypeToStringTableCompare);
	}

	{
		// binary search on sorted animtype list
		UUtInt32 min, max, midpoint, compare;

		min = 0;
		max = ONgAnimTypeToStringTableLength - 1;

		while(min <= max) {
			midpoint = (min + max) / 2;

			entry = ONcAnimTypeToStringTable + midpoint;
			compare = UUrString_Compare_NoCase(inString, entry->string);

			if (compare == 0) {
				// found, return
				return entry->type;

			} else if (compare < 0) {
				// must lie below here
				max = midpoint - 1;

			} else {
				// must lie above here
				min = midpoint + 1;
			}
		}
	}

	return ONcAnimType_None;
}

const char *ONrAnimStateToString(TRtAnimState inAnimState)
{
	const char *anim_state_string = "unknown-anim-state";
	UUtUns32 itr;

	for(itr = 0; ONcAnimStateToStringTable[itr].string != NULL; itr++)
	{
		if (ONcAnimStateToStringTable[itr].state == inAnimState) {
			anim_state_string = ONcAnimStateToStringTable[itr].string;
			break;
		}
	}

	UUmAssert(anim_state_string != NULL);
	return anim_state_string;
}

static int UUcExternal_Call ONiQSort_AnimStateToStringTableCompare(const void *pptr1, const void *pptr2)
{
	ONtAnimStateToStringTable *e1 = ((ONtAnimStateToStringTable *) pptr1), *e2 = ((ONtAnimStateToStringTable *) pptr2);

	return UUrString_Compare_NoCase(e1->string, e2->string);
}

TRtAnimState ONrStringToAnimState(const char *inString)
{
	ONtAnimStateToStringTable *entry;

	if (!ONgAnimStateToStringTableSorted) {
		// determine the length of the animtype table
		entry = ONcAnimStateToStringTable;
		while (entry->string != NULL) {
			entry++;
		}

		// sort the animstate table by anim state name
		ONgAnimStateToStringTableSorted = UUcTrue;
		ONgAnimStateToStringTableLength = entry - ONcAnimStateToStringTable;
		qsort(ONcAnimStateToStringTable, ONgAnimStateToStringTableLength,
				sizeof(ONtAnimStateToStringTable), ONiQSort_AnimStateToStringTableCompare);
	}

	{
		// binary search on sorted animtype list
		UUtInt32 min, max, midpoint, compare;

		min = 0;
		max = ONgAnimStateToStringTableLength - 1;

		while(min <= max) {
			midpoint = (min + max) / 2;

			entry = ONcAnimStateToStringTable + midpoint;
			compare = UUrString_Compare_NoCase(inString, entry->string);

			if (compare == 0) {
				// found, return
				return entry->state;

			} else if (compare < 0) {
				// must lie below here
				max = midpoint - 1;

			} else {
				// must lie above here
				min = midpoint + 1;
			}
		}
	}

	return ONcAnimState_None;
}

UUtUns16 ONrCharacter_GetAnimationImpactType(UUtUns16 inAnimType)
{
	typedef struct AnimTypeToAnimationImpactTypeTable
	{
		TRtAnimType anim_type;
		UUtUns16 anim_impact_type;
	} AnimTypeToAnimationImpactTypeTable;

	AnimTypeToAnimationImpactTypeTable associative_table[] = {
		// shuffling footsteps
		{ONcAnimType_Standing_Turn_Left,				ONcAnimationImpact_StandingTurn},
		{ONcAnimType_Standing_Turn_Right,				ONcAnimationImpact_StandingTurn},

		// walking footsteps
		{ONcAnimType_Walk,								ONcAnimationImpact_Walk},
		{ONcAnimType_Walk_Start,						ONcAnimationImpact_Walk},
		{ONcAnimType_Walk_Sidestep_Left,				ONcAnimationImpact_Walk},
		{ONcAnimType_Walk_Sidestep_Right,				ONcAnimationImpact_Walk},
		{ONcAnimType_Walk_Backwards,					ONcAnimationImpact_Walk},
		{ONcAnimType_Console_Walk,						ONcAnimationImpact_Walk},
		{ONcAnimType_Walk_Stop,							ONcAnimationImpact_WalkStop},

		// running footsteps
		{ONcAnimType_Run,								ONcAnimationImpact_Run},
		{ONcAnimType_Run_Backwards,						ONcAnimationImpact_Run},
		{ONcAnimType_Run_Sidestep_Left,					ONcAnimationImpact_Run},
		{ONcAnimType_Run_Sidestep_Right,				ONcAnimationImpact_Run},
		{ONcAnimType_Run_Start,							ONcAnimationImpact_RunStart},
		{ONcAnimType_Run_Backwards_Start,				ONcAnimationImpact_RunStart},
		{ONcAnimType_Run_Sidestep_Left_Start,			ONcAnimationImpact_RunStart},
		{ONcAnimType_Run_Sidestep_Right_Start,			ONcAnimationImpact_RunStart},
		{ONcAnimType_Run_Stop,							ONcAnimationImpact_RunStop},
		{ONcAnimType_1Step_Stop,						ONcAnimationImpact_1Step},

		// crouching footsteps
		{ONcAnimType_Crouch_Run,						ONcAnimationImpact_Crouch},
		{ONcAnimType_Crouch_Run_Backwards,				ONcAnimationImpact_Crouch},
		{ONcAnimType_Crouch_Run_Sidestep_Left,			ONcAnimationImpact_Crouch},
		{ONcAnimType_Crouch_Run_Sidestep_Right,			ONcAnimationImpact_Crouch},
		{ONcAnimType_Crouch_Walk,						ONcAnimationImpact_Crouch},
		{ONcAnimType_Crouch_Walk_Backwards,				ONcAnimationImpact_Crouch},
		{ONcAnimType_Crouch_Walk_Sidestep_Left,			ONcAnimationImpact_Crouch},
		{ONcAnimType_Crouch_Walk_Sidestep_Right,		ONcAnimationImpact_Crouch},

		// sliding
		{ONcAnimType_Slide,								ONcAnimationImpact_Slide},

		// knockdowns
		{ONcAnimType_Blownup,							ONcAnimationImpact_Knockdown},
		{ONcAnimType_Blownup_Behind,					ONcAnimationImpact_Knockdown},
		{ONcAnimType_Knockdown_Head,					ONcAnimationImpact_Knockdown},
		{ONcAnimType_Knockdown_Body,					ONcAnimationImpact_Knockdown},
		{ONcAnimType_Knockdown_Foot,					ONcAnimationImpact_Knockdown},
		{ONcAnimType_Knockdown_Crouch,					ONcAnimationImpact_Knockdown},
		{ONcAnimType_Knockdown_Head_Behind,				ONcAnimationImpact_Knockdown},
		{ONcAnimType_Knockdown_Body_Behind,				ONcAnimationImpact_Knockdown},
		{ONcAnimType_Knockdown_Foot_Behind,				ONcAnimationImpact_Knockdown},
		{ONcAnimType_Knockdown_Crouch_Behind,			ONcAnimationImpact_Knockdown},

		// been thrown
		{ONcAnimType_Thrown1,							ONcAnimationImpact_Thrown},
		{ONcAnimType_Thrown2,							ONcAnimationImpact_Thrown},
		{ONcAnimType_Thrown3,							ONcAnimationImpact_Thrown},
		{ONcAnimType_Thrown4,							ONcAnimationImpact_Thrown},
		{ONcAnimType_Thrown5,							ONcAnimationImpact_Thrown},
		{ONcAnimType_Thrown6,							ONcAnimationImpact_Thrown},
		{ONcAnimType_Thrown7,							ONcAnimationImpact_Thrown},
		{ONcAnimType_Thrown8,							ONcAnimationImpact_Thrown},
		{ONcAnimType_Thrown9,							ONcAnimationImpact_Thrown},
		{ONcAnimType_Thrown10,							ONcAnimationImpact_Thrown},
		{ONcAnimType_Thrown11,							ONcAnimationImpact_Thrown},
		{ONcAnimType_Thrown12,							ONcAnimationImpact_Thrown},
		{ONcAnimType_Thrown13,							ONcAnimationImpact_Thrown},
		{ONcAnimType_Thrown14,							ONcAnimationImpact_Thrown},
		{ONcAnimType_Thrown15,							ONcAnimationImpact_Thrown},
		{ONcAnimType_Thrown16,							ONcAnimationImpact_Thrown},
		{ONcAnimType_Thrown17,							ONcAnimationImpact_Thrown},

		// landing
		{ONcAnimType_Land,								ONcAnimationImpact_Land},
		{ONcAnimType_Land_Forward,						ONcAnimationImpact_Land},
		{ONcAnimType_Land_Right,						ONcAnimationImpact_Land},
		{ONcAnimType_Land_Left,							ONcAnimationImpact_Land},
		{ONcAnimType_Land_Back,							ONcAnimationImpact_Land},

		// landing hard
		{ONcAnimType_Land_Hard,							ONcAnimationImpact_LandHard},
		{ONcAnimType_Land_Forward_Hard,					ONcAnimationImpact_LandHard},
		{ONcAnimType_Land_Right_Hard,					ONcAnimationImpact_LandHard},
		{ONcAnimType_Land_Left_Hard,					ONcAnimationImpact_LandHard},
		{ONcAnimType_Land_Back_Hard,					ONcAnimationImpact_LandHard},

		// dying from a fall
		{ONcAnimType_Land_Dead,							ONcAnimationImpact_LandDead},

		// attacks may generate a landing impact because some are flying
		{ONcAnimType_Kick,								ONcAnimationImpact_Land},
		{ONcAnimType_Punch,								ONcAnimationImpact_Land},
		{ONcAnimType_Kick_Forward,						ONcAnimationImpact_Land},
		{ONcAnimType_Kick_Left,							ONcAnimationImpact_Land},
		{ONcAnimType_Kick_Right,						ONcAnimationImpact_Land},
		{ONcAnimType_Kick_Back,							ONcAnimationImpact_Land},
		{ONcAnimType_Kick_Low,							ONcAnimationImpact_Land},
		{ONcAnimType_Punch_Forward,						ONcAnimationImpact_Land},
		{ONcAnimType_Punch_Left,						ONcAnimationImpact_Land},
		{ONcAnimType_Punch_Right,						ONcAnimationImpact_Land},
		{ONcAnimType_Punch_Back,						ONcAnimationImpact_Land},
		{ONcAnimType_Punch_Low,							ONcAnimationImpact_Land},
		{ONcAnimType_Kick2,								ONcAnimationImpact_Land},
		{ONcAnimType_Kick3,								ONcAnimationImpact_Land},
		{ONcAnimType_Kick3_Forward,						ONcAnimationImpact_Land},
		{ONcAnimType_Punch2,							ONcAnimationImpact_Land},
		{ONcAnimType_Punch3,							ONcAnimationImpact_Land},
		{ONcAnimType_Punch4,							ONcAnimationImpact_Land},
		{ONcAnimType_PPK,								ONcAnimationImpact_Land},
		{ONcAnimType_PKK,								ONcAnimationImpact_Land},
		{ONcAnimType_PKP,								ONcAnimationImpact_Land},
		{ONcAnimType_KPK,								ONcAnimationImpact_Land},
		{ONcAnimType_KPP,								ONcAnimationImpact_Land},
		{ONcAnimType_KKP,								ONcAnimationImpact_Land},
		{ONcAnimType_PK,								ONcAnimationImpact_Land},
		{ONcAnimType_KP,								ONcAnimationImpact_Land},
		{ONcAnimType_PPKK,								ONcAnimationImpact_Land},
		{ONcAnimType_PPKKK,								ONcAnimationImpact_Land},
		{ONcAnimType_PPKKKK,							ONcAnimationImpact_Land},
		{ONcAnimType_Punch_Heavy,						ONcAnimationImpact_Land},
		{ONcAnimType_Kick_Heavy,						ONcAnimationImpact_Land},
		{ONcAnimType_Punch_Forward_Heavy,				ONcAnimationImpact_Land},
		{ONcAnimType_Kick_Forward_Heavy,				ONcAnimationImpact_Land},
		{ONcAnimType_Run_Kick,							ONcAnimationImpact_Land},
		{ONcAnimType_Run_Punch,							ONcAnimationImpact_Land},
		{ONcAnimType_Run_Back_Punch,					ONcAnimationImpact_Land},
		{ONcAnimType_Run_Back_Kick,						ONcAnimationImpact_Land},
		{ONcAnimType_Sidestep_Left_Kick,				ONcAnimationImpact_Land},
		{ONcAnimType_Sidestep_Left_Punch,				ONcAnimationImpact_Land},
		{ONcAnimType_Sidestep_Right_Kick,				ONcAnimationImpact_Land},
		{ONcAnimType_Sidestep_Right_Punch,				ONcAnimationImpact_Land},
		{ONcAnimType_Throw,								ONcAnimationImpact_Land},
		{ONcAnimType_Throw_Forward_Punch,				ONcAnimationImpact_Land},
		{ONcAnimType_Throw_Forward_Kick,				ONcAnimationImpact_Land},
		{ONcAnimType_Throw_Behind_Punch,				ONcAnimationImpact_Land},
		{ONcAnimType_Throw_Behind_Kick,					ONcAnimationImpact_Land},
		{ONcAnimType_Run_Throw_Forward_Punch,			ONcAnimationImpact_Land},
		{ONcAnimType_Run_Throw_Behind_Punch,			ONcAnimationImpact_Land},
		{ONcAnimType_Run_Throw_Forward_Kick,			ONcAnimationImpact_Land},
		{ONcAnimType_Run_Throw_Behind_Kick,				ONcAnimationImpact_Land},
		{ONcAnimType_PF_PF,								ONcAnimationImpact_Land},
		{ONcAnimType_PF_PF_PF,							ONcAnimationImpact_Land},
		{ONcAnimType_PL_PL,								ONcAnimationImpact_Land},
		{ONcAnimType_PL_PL_PL,							ONcAnimationImpact_Land},
		{ONcAnimType_PR_PR,								ONcAnimationImpact_Land},
		{ONcAnimType_PR_PR_PR,							ONcAnimationImpact_Land},
		{ONcAnimType_PB_PB,								ONcAnimationImpact_Land},
		{ONcAnimType_PB_PB_PB,							ONcAnimationImpact_Land},
		{ONcAnimType_PD_PD,								ONcAnimationImpact_Land},
		{ONcAnimType_PD_PD_PD,							ONcAnimationImpact_Land},
		{ONcAnimType_KF_KF,								ONcAnimationImpact_Land},
		{ONcAnimType_KF_KF_KF,							ONcAnimationImpact_Land},
		{ONcAnimType_KL_KL,								ONcAnimationImpact_Land},
		{ONcAnimType_KL_KL_KL,							ONcAnimationImpact_Land},
		{ONcAnimType_KR_KR,								ONcAnimationImpact_Land},
		{ONcAnimType_KR_KR_KR,							ONcAnimationImpact_Land},
		{ONcAnimType_KB_KB,								ONcAnimationImpact_Land},
		{ONcAnimType_KB_KB_KB,							ONcAnimationImpact_Land},
		{ONcAnimType_KD_KD,								ONcAnimationImpact_Land},
		{ONcAnimType_KD_KD_KD,							ONcAnimationImpact_Land},

		// FIXME: attach impacts to rolls?
/*		{ONcAnimType_Crouch_Forward,					ONcAnimationImpact_None},
		{ONcAnimType_Crouch_Back,						ONcAnimationImpact_None},
		{ONcAnimType_Crouch_Left,						ONcAnimationImpact_None},
		{ONcAnimType_Crouch_Right,						ONcAnimationImpact_None},*/

		{(UUtUns16) -1,									ONcAnimationImpact_None}
	};

	static UUtBool built_compiled_table = UUcFalse;
	static UUtUns16 compiled_table[ONcAnimType_Num];

	if (!built_compiled_table) {
		UUtUns32 itr;

		// loop over the associative array and build the compiled table
		UUrMemory_Set16(&compiled_table, ONcAnimationImpact_None, sizeof(compiled_table));
		for (itr = 0; itr < 1000; itr++) {
			if (associative_table[itr].anim_type == (UUtUns16) -1) {
				break;
			}

			UUmAssert((associative_table[itr].anim_type >= 0) && (associative_table[itr].anim_type < ONcAnimType_Num));
			UUmAssert((associative_table[itr].anim_impact_type >= 0) && (associative_table[itr].anim_impact_type < ONcAnimationImpact_Max));
			compiled_table[associative_table[itr].anim_type] = associative_table[itr].anim_impact_type;
		}
	}

	UUmAssert((inAnimType >= 0) && (inAnimType < ONcAnimType_Num));

	return compiled_table[inAnimType];
}

void ONrCharacter_PlayAnimationImpact(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter, TRtFootstepKind inFootstep, M3tPoint3D *inOptionalLocation)
{
	char msgbuf[256];
	MAtMaterialType material_type;
	UUtUns16 anim_impact_type;
	ONtCharacterAnimationImpact *impact;
	P3tEffectData effect_data;
	AKtGQ_Render *gq_render;
	M3tTextureMap *floor_texture;

	if (ioActiveCharacter->last_floor_gq < 0) {
		if (ONgDebugFootsteps) {
			COrConsole_Printf("footsteps: failed to find floor, not played");
		}
		return;
	}

	sprintf(msgbuf, "%s(%s) %s%d", TMrInstance_GetInstanceName(ioActiveCharacter->animation), ONrAnimTypeToString(ioActiveCharacter->curAnimType),
				(inFootstep == TRcFootstep_None) ? "" :
				(inFootstep == TRcFootstep_Left) ? "L" :
				(inFootstep == TRcFootstep_Right) ? "R" :
				(inFootstep == TRcFootstep_Single) ? "S" : "?", ioActiveCharacter->animFrame);

	anim_impact_type = ONrCharacter_GetAnimationImpactType(ioActiveCharacter->curAnimType);
	if (anim_impact_type == ONcAnimationImpact_None) {
		if (ONgDebugFootsteps) {
			COrConsole_Printf("%s: this animtype cannot play impacts", msgbuf);
		}
		return;
	}

	// promote running impacts to sprinting
	if ((anim_impact_type == ONcAnimationImpact_Run) && (TRrAnimation_GetVarient(ioActiveCharacter->animation) & ONcAnimVarientMask_Sprint)) {
		anim_impact_type = ONcAnimationImpact_Sprint;
	}

	UUmAssert(ioCharacter->characterClass->anim_impacts.indices_found);
	UUmAssert((anim_impact_type >= 0) && (anim_impact_type < ONcAnimationImpact_Max));
	impact = &ioCharacter->characterClass->anim_impacts.impact[anim_impact_type];

	gq_render = ONgGameState->level->environment->gqRenderArray->gqRender + ioActiveCharacter->last_floor_gq;
	floor_texture = ONgGameState->level->environment->textureMapArray->maps[gq_render->textureMapIndex];
	material_type = M3rTextureMap_GetMaterialType(floor_texture);

	if (ONgDebugFootsteps) {
		UUtUns32 itr;
		const ONtCharacterAnimationImpactType *impact_type;

		for (itr = 0, impact_type = ONcAnimImpactTypes; impact_type->name != NULL; itr++, impact_type++) {
			if (impact_type->type == anim_impact_type) {
				break;
			}
		}
		COrConsole_Printf("%s: anim_impact_type %s -> impact %s (floor %s)", msgbuf,
			(impact_type->name == NULL) ? "<error>" : impact_type->name, MArImpactType_GetName(impact->impact_type),
			MArMaterialType_GetName(material_type));
	}

	UUrMemory_Clear(&effect_data, sizeof(effect_data));
	effect_data.owner = WPrOwner_MakeMeleeDamageFromCharacter(ioCharacter);
	effect_data.time = ONrGameState_GetGameTime();
	effect_data.cause_type = P3cEffectCauseType_None;
	effect_data.position = (inOptionalLocation != NULL) ? *inOptionalLocation : ioCharacter->actual_position;

	ONrImpactEffect(impact->impact_type, material_type, ioCharacter->characterClass->anim_impacts.modifier_type, &effect_data, 1.0f, ioCharacter, NULL);

	return;
}

UUtError ONrCharacter_MakeActive(ONtCharacter *ioCharacter, UUtBool inCopyCurrent)
{
	ONtCharacterIndexType index;

	if ((NULL == ioCharacter) || (0 == (ONcCharacterFlag_InUse & ioCharacter->flags))) {
		UUmAssert(!"make active on a bogus character");
		return UUcError_Generic;
	}

	if (ioCharacter->active_index != ONcCharacterIndex_None) {
		// this character is already active
		ONtCharacterIndexType active_index = ONrCharacter_GetActiveIndex(ioCharacter);
		UUmAssert((active_index >= 0) && (active_index < ONgGameState->usedActiveCharacters));

		return UUcError_None;
	}

	return ONrGameState_NewActiveCharacter(ioCharacter, &index, inCopyCurrent);
}

UUtError ONrCharacter_MakeInactive(ONtCharacter *ioCharacter)
{
	ONtActiveCharacter *activeCharacter;

	if (ioCharacter->active_index == ONcCharacterIndex_None) {
		// this character is already inactive
		return UUcError_None;
	}

	// make absolutely sure we never make the player inactive
	if (ioCharacter->charType == ONcChar_Player) {
		return UUcError_Generic;
	}

	{
		ONtCharacterIndexType active_index;

		// force AI into an inactive state by returning to job
		AI2rReturnToJob(ioCharacter, UUcTrue, UUcTrue);

		active_index = ONrCharacter_GetActiveIndex(ioCharacter);
		UUmAssert((active_index >= 0) && (active_index < ONgGameState->usedActiveCharacters));
		activeCharacter = ONgGameState->activeCharacterStorage + active_index;

		UUmAssert(activeCharacter->active_index == ioCharacter->active_index);
		if (activeCharacter->index == ioCharacter->index) {
			ONrGameState_DeleteActiveCharacter(activeCharacter, UUcTrue);
		} else {
			UUmAssert(!"ONrCharacter_MakeInactive: active character does not match up with character index");
		}
	}

	return UUcError_None;
}

ONtActiveCharacter *ONrGetActiveCharacter(const ONtCharacter *inCharacter)
{
	UUmAssertReadPtr(inCharacter, sizeof(*inCharacter));

	if (inCharacter->active_index == ONcCharacterIndex_None) {
		return NULL;
	} else {
		ONtCharacterIndexType active_index = ONrCharacter_GetActiveIndex((ONtCharacter *) inCharacter);

		UUmAssert((active_index >= 0) && (active_index < ONgGameState->usedActiveCharacters));
		return ONgGameState->activeCharacterStorage + active_index;
	}
}

ONtActiveCharacter *ONrForceActiveCharacter(ONtCharacter *inCharacter)
{
	if (inCharacter->active_index == ONcCharacterIndex_None) {
		ONrCharacter_MakeActive(inCharacter, UUcTrue);
	} else {
		// this character is required to be active, reset its deactivation timer
		inCharacter->inactive_timer = 0;
	}

	return ONrGetActiveCharacter(inCharacter);
}

void ONrCharacter_MinorHitReaction(ONtCharacter *ioCharacter)
{
	ONtActiveCharacter *active_character;
	const TRtAnimation *hit_overlay;
	TRtAnimType hit_overlay_anim_type;

	active_character = ONrForceActiveCharacter(ioCharacter);
	if (active_character == NULL) {
		return;
	}

	hit_overlay_anim_type = (ONcChar_AI2 == ioCharacter->charType) ? ONcAnimType_Hit_Overlay_AI : ONcAnimType_Hit_Overlay;

	hit_overlay =
		TRrCollection_Lookup(
		ioCharacter->characterClass->animations,
		hit_overlay_anim_type,
		ONcAnimState_Anything,
		active_character->animVarient);

	if (NULL == hit_overlay) {
		hit_overlay =
			TRrCollection_Lookup(
			ioCharacter->characterClass->animations,
			ONcAnimType_Hit_Overlay,
			ONcAnimState_Anything,
			active_character->animVarient);
	}

	if (NULL != hit_overlay ) {
		ONrOverlay_Set(active_character->overlay + ONcOverlayIndex_HitReaction, hit_overlay, 0);
	}

	return;
}

TRtAnimType ONrCharacter_GetAnimType(const ONtCharacter *inCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character == NULL) {
		return ONcAnimType_Stand;
	} else {
		return active_character->curAnimType;
	}
}

void ONrCharacter_GetCurrentWorldVelocity(ONtCharacter *inCharacter, M3tVector3D *outVelocity)
{
	// if we are colliding, we can't rely on the animation for a velocity estimate.
	// use the character's actual change in movement since the last frame.
	MUmVector_Subtract(*outVelocity, inCharacter->location, inCharacter->prev_location);
}

const TRtAnimation *ONrCharacter_TryImmediateAnimation(ONtCharacter *ioCharacter, TRtAnimType inAnimType)
{
	ONtActiveCharacter *active_character = ONrForceActiveCharacter(ioCharacter);

	if (active_character == NULL) {
		return NULL;
	}

	return TRrCollection_Lookup(ioCharacter->characterClass->animations, inAnimType,
								active_character->nextAnimState, active_character->animVarient);
}

M3tMatrix4x3 *ONrCharacter_GetMatrix(ONtCharacter *inCharacter, UUtUns32 inIndex)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character == NULL) {
		return NULL;
	} else {
		return active_character->matricies + inIndex;
	}
}


void ONrCharacter_ResetWeaponVarient(ONtCharacter *ioCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);

	if (NULL != active_character) {
		ONrCharacter_SetWeaponVarient(ioCharacter, active_character);
	}
}

void ONrCharacter_ForceStand(ONtCharacter *inCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

	if (NULL != active_character) {
		const TRtAnimation *forced_stand = TRrCollection_Lookup(inCharacter->characterClass->animations, ONcAnimType_Stand, ONcAnimState_Standing, active_character->animVarient);

		ONrCharacter_SetAnimation_External(inCharacter, active_character->nextAnimState, forced_stand, 12);

		if (NULL == forced_stand) {
			COrConsole_Printf("failed to find stand animation for %s", inCharacter->player_name);
		}
	}

	return;
}

UUtBool ONrActiveCharacter_IsPlayingThisAnimationName(ONtActiveCharacter *inActiveCharacter, const char *inDesiredAnimationName)
{
	UUtBool is_playing_the_animation = UUcFalse;

	if (NULL != inActiveCharacter) {
		const char *animation_instance_name;

		if (inActiveCharacter->animation != NULL) {
			animation_instance_name = TMrInstance_GetInstanceName(inActiveCharacter->animation);

			is_playing_the_animation = 0 == UUrString_Compare_NoCase(animation_instance_name, inDesiredAnimationName);
		}

		if (!is_playing_the_animation) {
			UUtUns32 itr;

			for(itr = 0; itr < ONcOverlayIndex_Count; itr++)
			{
				ONtOverlay *overlay = inActiveCharacter->overlay + itr;

				if (overlay->animation != NULL) {
					animation_instance_name = TMrInstance_GetInstanceName(overlay->animation);

					is_playing_the_animation = 0 == UUrString_Compare_NoCase(animation_instance_name, inDesiredAnimationName);

					if (is_playing_the_animation) {
						break;
					}
				}
			}
		}
	}


	return is_playing_the_animation;
}

// collide with a character
UUtBool ONrCharacter_Collide_Ray(ONtCharacter *inCharacter, M3tPoint3D *inPoint, M3tVector3D *inDeltaPos, UUtBool inStopAtOneT,
											  UUtUns32 *outIndex, float *outT, M3tPoint3D *outHitPoint, M3tVector3D *outNormal)
{
	ONtActiveCharacter *active_character;
	M3tPoint3D local_pt, local_dir;
	M3tMatrix4x3 *matrix;
	M3tBoundingBox_MinMax bbox;
	M3tBoundingSphere sphere;
	TRtBody *body;
	UUtUns32 itr, hit_itr, hit_face, this_face;
	float hit_t, this_t;
	UUtBool collision, exit_intersection;

	active_character = ONrGetActiveCharacter(inCharacter);
	if (active_character == NULL)
		return UUcFalse;

	// check the ray against the character's big bounding sphere
	collision = CLrSphere_Ray(inPoint, inDeltaPos, &active_character->boundingSphere, &this_t, &exit_intersection);
	if (!collision) {
		return UUcFalse;

	} else if (exit_intersection) {
		this_t = 0.0f;

	} else if (inStopAtOneT && (this_t > 1.0f)) {
		return UUcFalse;
	}

	// loop over each body part, and check ray against bounding box
	body = ONrCharacter_GetBody(inCharacter, TRcBody_SuperHigh);
	matrix = active_character->matricies;

	hit_itr = (UUtUns32) -1;
	hit_t = +1e09;
	for (itr = 0; itr < ONcNumCharacterParts; itr++, matrix++) {
		if (ONgCharacterCollision_BoxEnable) {
			// get the body part's matrix and transform ray into partspace
			local_pt.x = inPoint->x - matrix->m[3][0];
			local_pt.y = inPoint->y - matrix->m[3][1];
			local_pt.z = inPoint->z - matrix->m[3][2];
			MUrMatrix3x3_TransposeMultiplyPoint(&local_pt, (M3tMatrix3x3 *) matrix, &local_pt);
			MUrMatrix3x3_TransposeMultiplyPoint(inDeltaPos, (M3tMatrix3x3 *) matrix, &local_dir);

			// intersect the ray with the body part's geometry's bounding box
			bbox = body->geometries[itr]->pointArray->minmax_boundingBox;
			MUmVector_Scale(bbox.minPoint, inCharacter->scale);
			MUmVector_Scale(bbox.maxPoint, inCharacter->scale);

			if (ONgCharacterCollision_Grow != 0) {
				bbox.minPoint.x -= ONgCharacterCollision_Grow;
				bbox.minPoint.y -= ONgCharacterCollision_Grow;
				bbox.minPoint.z -= ONgCharacterCollision_Grow;
				bbox.maxPoint.x += ONgCharacterCollision_Grow;
				bbox.maxPoint.y += ONgCharacterCollision_Grow;
				bbox.maxPoint.z += ONgCharacterCollision_Grow;

				if ((bbox.minPoint.x >= bbox.maxPoint.x) ||
					(bbox.minPoint.y >= bbox.maxPoint.y) ||
					(bbox.minPoint.z >= bbox.maxPoint.z)) {
					// the bounding box has been shrunk to nothing
					continue;
				}
			}
			collision = PHrCollision_Box_Ray(&bbox, &local_pt, &local_dir, inStopAtOneT, &this_t, &this_face);
			if (!collision) {
				continue;
			}
		} else {
			// transform ray into rotated partspace
			local_pt.x = inPoint->x - matrix->m[3][0];
			local_pt.y = inPoint->y - matrix->m[3][1];
			local_pt.z = inPoint->z - matrix->m[3][2];

			// check the ray against the body part's bounding sphere
			sphere = body->geometries[itr]->pointArray->boundingSphere;
			sphere.radius *= inCharacter->scale;

			if (ONgCharacterCollision_Grow != 0) {
				sphere.radius += ONgCharacterCollision_Grow;
				if (sphere.radius <= 0) {
					// the bounding sphere has been shrunk to nothing
					continue;
				}
			}

			collision = CLrSphere_Ray(&local_pt, inDeltaPos, &sphere, &this_t, &exit_intersection);
			if (!collision) {
				continue;
			} else if (exit_intersection) {
				this_t = 0.0f;
			} else if (inStopAtOneT && (this_t >= 1.0f)) {
				continue;
			}

			this_face = (UUtUns32) -1;
		}

		UUmAssert(this_t >= 0.0f);
		UUmAssert((!inStopAtOneT) || (this_t <= 1.0f));
		if (this_t < hit_t) {
			// this is now the closest collision
			hit_t = this_t;
			hit_face = this_face;
			hit_itr = itr;
		}
	}

	if (hit_itr == (UUtUns32) -1) {
		// no collision
		return UUcFalse;
	}

	UUmAssert((hit_itr >= 0) && (hit_itr < ONcNumCharacterParts));
	if (outIndex != NULL) {
		*outIndex = hit_itr;
	}

	UUmAssert(hit_t >= 0.0f);
	UUmAssert((!inStopAtOneT) || (hit_t <= 1.0f));
	if (outT != NULL) {
		*outT = hit_t;
	}

	if (outHitPoint != NULL) {
		// calculate the hit point
		MUmVector_Copy(*outHitPoint, *inPoint);
		MUmVector_ScaleIncrement(*outHitPoint, hit_t, *inDeltaPos);
	}

	if (outNormal != NULL) {
		if (hit_face == (UUtUns32) -1) {
			// we hit a sphere, calculate the normal from the vector out of the sphere
			local_pt = MUrMatrix_GetTranslation(&active_character->matricies[hit_itr]);
			MUmVector_Add(local_pt, local_pt, body->geometries[hit_itr]->pointArray->boundingSphere.center);
			MUmVector_Subtract(*outNormal, *outHitPoint, local_pt);
			MUmVector_Normalize(*outNormal);
		} else {
			// calculate the hit normal from the face index that we hit
			UUmAssert((hit_face >= 0) && (hit_face < 6));
			matrix = active_character->matricies + hit_itr;
			MUrMatrix_GetCol(matrix, (UUtUns8) (hit_face / 2), outNormal);
			if ((hit_face % 2) == 0) {
				MUmVector_Negate(*outNormal);
			}
		}
	}

	return UUcTrue;
}

// collide with a character
UUtBool ONrCharacter_Collide_SphereRay(ONtCharacter *inCharacter, M3tPoint3D *inPoint, M3tVector3D *inDeltaPos, float inRadius,
											  UUtUns32 *outIndex, float *outT, M3tPoint3D *outHitPoint, M3tVector3D *outNormal)
{
	ONtActiveCharacter *active_character;
	M3tPoint3D local_pt, collide_centerpt, collide_partcenter;
	M3tVector3D delta_pos;
	M3tMatrix4x3 *matrix;
	M3tBoundingSphere cur_sphere;
	TRtBody *body;
	UUtUns32 itr, hit_itr;
	float hit_t, this_radius, hit_radius, this_t, dist;
	UUtBool collision, exit_intersection;

	active_character = ONrGetActiveCharacter(inCharacter);
	if (active_character == NULL)
		return UUcFalse;

	// check the sphere-ray against the character's big bounding sphere
	cur_sphere = active_character->boundingSphere;
	cur_sphere.radius += inRadius;
	collision = CLrSphere_Ray(inPoint, inDeltaPos, &cur_sphere, &this_t, &exit_intersection);
	if (!collision) {
		return UUcFalse;

	} else if (exit_intersection) {
		this_t = 0.0f;

	} else if (this_t > 1.0f) {
		return UUcFalse;
	}

	// loop over each body part, and check ray against its bounding sphere
	body = ONrCharacter_GetBody(inCharacter, TRcBody_SuperHigh);
	matrix = active_character->matricies;

	hit_itr = (UUtUns32) -1;
	hit_t = +1e09;
	hit_radius = +1e09;
	for (itr = 0; itr < ONcNumCharacterParts; itr++, matrix++) {
		// transform ray into rotated partspace
		local_pt.x = inPoint->x - matrix->m[3][0];
		local_pt.y = inPoint->y - matrix->m[3][1];
		local_pt.z = inPoint->z - matrix->m[3][2];

		// check the ray against the body part's bounding sphere
		cur_sphere = body->geometries[itr]->pointArray->boundingSphere;
		cur_sphere.radius *= inCharacter->scale;
		if (ONgCharacterCollision_Grow != 0) {
			cur_sphere.radius += ONgCharacterCollision_Grow;
			if (cur_sphere.radius <= 0) {
				// the bounding sphere has been shrunk to nothing
				continue;
			}
		}
		this_radius = cur_sphere.radius;
		cur_sphere.radius += inRadius;

		collision = CLrSphere_Ray(&local_pt, inDeltaPos, &cur_sphere, &this_t, &exit_intersection);
		if (!collision) {
			continue;
		} else if (exit_intersection) {
			this_t = 0.0f;
		} else if (this_t > 1.0f) {
			continue;
		}

		UUmAssert((this_t >= 0.0f) && (this_t <= 1.0f));
		if (this_t < hit_t) {
			// this is now the closest collision
			hit_t = this_t;
			hit_itr = itr;
			hit_radius = this_radius;
			collide_partcenter = cur_sphere.center;
			MUrMatrix_MultiplyPoint(&collide_partcenter, &active_character->matricies[itr], &collide_partcenter);
		}
	}

	if (hit_itr == (UUtUns32) -1) {
		// no collision
		return UUcFalse;
	}

	UUmAssert((hit_itr >= 0) && (hit_itr < ONcNumCharacterParts));
	if (outIndex != NULL) {
		*outIndex = hit_itr;
	}

	UUmAssert((hit_t >= 0.0f) && (hit_t <= 1.0f));
	if (outT != NULL) {
		*outT = hit_t;
	}

	if ((outHitPoint != NULL) || (outNormal != NULL)) {
		// calculate center of the sphere at the hit time
		MUmVector_Copy(collide_centerpt, *inPoint);
		MUmVector_ScaleIncrement(collide_centerpt, hit_t, *inDeltaPos);

		// calculate the contact point at the hit time
		MUmVector_Subtract(delta_pos, collide_centerpt, collide_partcenter);
		if (outNormal != NULL) {
			MUmVector_Copy(*outNormal, delta_pos);
			if (MUrVector_Normalize_ZeroTest(outNormal)) {
				// the particle is directly at the sphere's center.
				MUmVector_ScaleCopy(*outNormal, -1, *inDeltaPos);
				if (MUrVector_Normalize_ZeroTest(outNormal)) {
					// the particle is not moving.
					MUmVector_Set(*outNormal, 0, 1, 0);
				}
			}
		}

		if (outHitPoint != NULL) {
			dist = MUmVector_GetLength(delta_pos);
			if (dist < hit_radius) {
				// the particle is inside the body part
				MUmVector_Copy(*outHitPoint, collide_centerpt);
			} else {
				// place the hit point on the outside of the body part
				MUrVector_SetLength(&delta_pos, hit_radius);
				MUmVector_Add(*outHitPoint, collide_partcenter, delta_pos);
			}
		}
	}

	return UUcTrue;
}

void ONrGameState_NotifyCameraCut(void)
{
	ONtCharacter **characters = ONrGameState_ActiveCharacterList_Get();
	UUtUns32 count = ONrGameState_ActiveCharacterList_Count();
	UUtUns32 itr;

	for(itr = 0; itr < count; itr++)
	{
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(characters[itr]);

		if (NULL != active_character) {
			active_character->curBody = TRcBody_NotDrawn;
		}
	}

	return;
}

void ONrGameState_DisplayAiming(void)
{
	ONtCharacter *player_character = ONgGameState->local.playerCharacter;
	WPtWeapon *weapon;
	WPtWeaponClass *weapon_class;
	WPtScaleMode scale_mode;
	WPtSprite *sprite;

	if (!ONgDrawLaserSight) {
		goto exit;
	}

	if (NULL == player_character) {
		goto exit;
	}

	weapon = player_character->inventory.weapons[0];

	if (NULL == weapon) {
		goto exit;
	}

	weapon_class = WPrGetClass(weapon);

	if (weapon_class->flags & WPcWeaponClassFlag_NoScale) {
		scale_mode = WPcScaleMode_NoScale;
	}
	else if (weapon_class->flags & WPcWeaponClassFlag_HalfScale) {
		scale_mode = WPcScaleMode_HalfScale;
	}
	else {
		scale_mode = WPcScaleMode_Scale;
	}

	if (AMcRayResult_Character == ONgLaserType) {
		sprite = &weapon_class->cursor_targeted;
	}
	else {
		sprite = &weapon_class->cursor;
	}

	M3rGeom_State_Push();
		M3rGeom_State_Set(M3cGeomStateIntType_SubmitMode, M3cGeomState_SubmitMode_Normal);

		if (weapon_class->flags & WPcWeaponClassFlag_DontClipSight) {
			M3rDraw_State_SetInt(M3cDrawStateIntType_ZWrite, M3cDrawState_ZWrite_Off);
			M3rDraw_State_SetInt(M3cDrawStateIntType_ZCompare, M3cDrawState_ZCompare_Off);
		}

		M3rDraw_State_SetInt(M3cDrawStateIntType_Fog, M3cDrawStateFogDisable);
		M3rGeom_State_Commit();

		WPrDrawSprite(sprite, &ONgLaserStop, scale_mode);
	M3rGeom_State_Pop();

exit:
	return;
}

typedef struct ONtDeferredScript
{
	char script[SLcScript_MaxNameLength];
	char player_name[ONcMaxPlayerNameLength];
	char cause[OBJcMaxNameLength];			// CB: max trigger volume name length is 64
} ONtDeferredScript;


#define ONcMaxDeferredScripts 128

ONtDeferredScript ONgDeferredScripts[ONcMaxDeferredScripts];
UUtUns32 ONgDeferredScriptCount = 0;
UUtBool ONgScriptsAreLocked = UUcFalse;

void ONrCharacter_Defer_Script(const char *inScript, const char *inCharacterName, const char *inCause)
{
	UUmAssert(ONgDeferredScriptCount < ONcMaxDeferredScripts);

	if (gDebugScripts) {
		COrConsole_Printf("defer script %s %s %s", inScript, inCharacterName, inCause);
	}

	if (ONgDeferredScriptCount >= ONcMaxDeferredScripts) {
		COrConsole_Printf("failed to defer script %s character %s cause %s", inScript, inCharacterName, inCause);
	}
	else {
		UUrString_Copy(ONgDeferredScripts[ONgDeferredScriptCount].script, inScript, sizeof(ONgDeferredScripts[ONgDeferredScriptCount].script));
		UUrString_Copy(ONgDeferredScripts[ONgDeferredScriptCount].player_name, inCharacterName, sizeof(ONgDeferredScripts[ONgDeferredScriptCount].player_name));
		UUrString_Copy(ONgDeferredScripts[ONgDeferredScriptCount].cause, inCause, sizeof(ONgDeferredScripts[ONgDeferredScriptCount].cause));

		ONgDeferredScriptCount++;
	}
}

void ONrCharacter_Commit_Scripts(void)
{
	UUtUns32 itr;
	UUtBool must_unlock;
	ONtDeferredScript deferred_scripts[ONcMaxDeferredScripts];
	UUtUns32 count;

	if (ONgScriptsAreLocked) {
		return;
	}

	// copy the deferred scripts, so we can recursively work with this array
	count = ONgDeferredScriptCount;
	ONgDeferredScriptCount = 0;
	UUrMemory_MoveFast(ONgDeferredScripts, deferred_scripts, sizeof(ONtDeferredScript) * count);

	must_unlock = ONrCharacter_Lock_Scripts();

	for(itr = 0; itr < count; itr++)
	{
		if (gDebugScripts) {
			COrConsole_Printf("calling script %s %s %s", deferred_scripts[itr].script, deferred_scripts[itr].player_name, deferred_scripts[itr].cause);
		}

		ONgCurrentlyActiveTrigger_Script = deferred_scripts[itr].cause;
		ONrScript_ExecuteSimple(deferred_scripts[itr].script, deferred_scripts[itr].player_name);
	}

	if (gDebugScripts && (ONgDeferredScriptCount != 0)){
		COrConsole_Printf("blam: we would have lost a script");
	}

	if (must_unlock) {
		ONrCharacter_Unlock_Scripts();
	}

	return;
}

UUtBool ONrCharacter_Lock_Scripts(void)
{
	if (ONgScriptsAreLocked) {
		return UUcFalse;
	} else {
		ONgScriptsAreLocked = UUcTrue;
		return UUcTrue;
	}
}

void ONrCharacter_Unlock_Scripts(void)
{
	ONgScriptsAreLocked = UUcFalse;

	return;
}

void ONrCharacter_Poison(ONtCharacter *ioCharacter, UUtUns32 inPoisonAmount, UUtUns32 inDamageFrames, UUtUns32 inInitialFrames)
{
	UUtUns32 current_time = ONrGameState_GetGameTime();

	if (ioCharacter->poisonLastFrame + ONcPoisonForgetFrames < current_time) {
		// we managed to get out of the poison for long enough, reset our poison frame counter
		ioCharacter->poisonFrameCounter = 0;
	}

	ioCharacter->poisonLastFrame = current_time;
	ioCharacter->poisonFrameCounter++;
	if (ioCharacter->poisonFrameCounter >= (UUtInt32) inInitialFrames) {
		// hurt the character and reset the counter so that the next damage will take place in damage-frames
		//
		// note that if initial-frames is less than damage-frames then this will cause the counter to be set
		// to a negative value, which is fine.
		ONrCharacter_TakeDamage(ioCharacter, inPoisonAmount, 0.0f, NULL, NULL, 0, ONcAnimType_None);
		ioCharacter->poisonFrameCounter = inInitialFrames - inDamageFrames;
	}
}

static void ONiCharacter_ResetPainState(ONtCharacter *ioCharacter)
{
	UUrMemory_Clear(&ioCharacter->painState, sizeof(ONtPainState));
}

static void ONiCharacter_PainUpdate(ONtCharacter *ioCharacter)
{
	ONtPainConstants *pain_constants = &ioCharacter->characterClass->painConstants;
	UUtUns32 current_time = ONrGameState_GetGameTime();

	if (ioCharacter->painState.decay_timer > 0) {
		ioCharacter->painState.decay_timer--;

		if (ioCharacter->painState.decay_timer == 0) {
			// forget about our previous pain
			ONiCharacter_ResetPainState(ioCharacter);
		}
	}

	if (ioCharacter->painState.queue_timer > 0) {
		ioCharacter->painState.queue_timer--;

		if (ioCharacter->painState.queue_timer == 0) {
			// play the queued-up hurt state
			ONrCharacter_PlayHurtSound(ioCharacter, ioCharacter->painState.queue_type, &ioCharacter->painState.queue_volume);

			if (ioCharacter->painState.queue_type == ONcPain_Light) {
				ioCharacter->painState.num_light++;
				ioCharacter->painState.last_sound_type = ONcPain_Light;
				ioCharacter->painState.last_sound_time = current_time;

			} else if (ioCharacter->painState.queue_type == ONcPain_Medium) {
				ioCharacter->painState.num_medium++;
				ioCharacter->painState.last_sound_type = ONcPain_Medium;
				ioCharacter->painState.last_sound_time = current_time + (pain_constants->hurt_min_timer / 2);

			} else if (ioCharacter->painState.queue_type == ONcPain_Heavy) {
				// heavy pain sounds are the end of a sequence
				ONiCharacter_ResetPainState(ioCharacter);
				ioCharacter->painState.last_sound_type = ONcPain_Heavy;
				ioCharacter->painState.last_sound_time = current_time + pain_constants->hurt_min_timer;

			} else {
				UUmAssert(0);
			}

			ioCharacter->painState.queue_type = 0;
			ioCharacter->painState.queue_volume = 0;
		}
	}
}

static void ONiCharacter_QueueHurtSound(ONtCharacter *ioCharacter, UUtUns32 inHurtType, float inVolume)
{
	if (ioCharacter->painState.queue_timer > 0) {
		// there is already a sound queued up
		ioCharacter->painState.queue_type = UUmMax(ioCharacter->painState.queue_type, inHurtType);
		return;
	}

	ioCharacter->painState.queue_timer = ONcPain_DelayFrames;
	ioCharacter->painState.queue_type = inHurtType;
	ioCharacter->painState.queue_volume = inVolume;
}

void ONrCharacter_Pain(ONtCharacter *ioCharacter, UUtUns32 inDamage, UUtUns32 inMinimumPain)
{
	ONtPainConstants *pain_constants = &ioCharacter->characterClass->painConstants;
	float volume;
	UUtUns16 random_val;
	UUtUns32 pain_chance, pain_type, current_time;

//	COrConsole_Printf("%s: hurt %d (%s)", ioCharacter->player_name, inDamage, ONcPainTypeName[inMinimumPain]);

	if (inDamage == 0) {
		return;
	}

	if ((ioCharacter->charType == ONcChar_AI2) && (AI2rIsCasual(ioCharacter))) {
		// we are casual, always cry out in pain
		ioCharacter->ai2State.flags |= AI2cFlag_SaidAlertSound;

	} else if (ioCharacter->painState.total_damage == 0) {
		// work out whether we will ignore this pain
		if (inDamage >= pain_constants->hurt_percentage_threshold) {
			pain_chance = pain_constants->hurt_max_percentage;
		} else {
			pain_chance = pain_constants->hurt_base_percentage + (inDamage * (pain_constants->hurt_max_percentage -
																				pain_constants->hurt_base_percentage))
																	/ pain_constants->hurt_percentage_threshold;
		}
/*		COrConsole_Printf("  dmg %d (tol %d, chance %d/%d -> %d)", inDamage, pain_constants->hurt_percentage_threshold,
						pain_constants->hurt_base_percentage, pain_constants->hurt_max_percentage, pain_chance);*/

		pain_chance = (pain_chance * UUcMaxUns16) / 100;
		random_val = UUrRandom();
		if (random_val >= pain_chance) {
			// we ignore this pain
//			COrConsole_Printf("  random %d > chance %d, ignore", random_val, pain_chance);
			return;
		} else {
//			COrConsole_Printf("  random %d < chance %d, hurt", random_val, pain_chance);
		}
	}

	// track the incoming damage
	ioCharacter->painState.total_damage += (UUtUns16) inDamage;
	ioCharacter->painState.decay_timer = pain_constants->hurt_timer;

	current_time = ONrGameState_GetGameTime();
	if (ioCharacter->painState.last_sound_time + pain_constants->hurt_min_timer > current_time) {
		// we can't make a pain sound now, it hasn't been long enough since our last one
		return;
	}

	// determine what level of pain sound to make
	pain_type = UUmMax(inMinimumPain, ioCharacter->painState.last_sound_type);

	if (pain_type == ONcPain_Light) {
		// determine whether we have to make a medium pain sound
		if ((ioCharacter->painState.num_light >= pain_constants->hurt_max_light) ||
			(ioCharacter->painState.total_damage >= pain_constants->hurt_medium_threshold)) {
			pain_type = ONcPain_Medium;
/*			COrConsole_Printf("  num %d (%d) dmg %d (%d) -> escalate to med",
					ioCharacter->painState.num_light, pain_constants->hurt_max_light,
					ioCharacter->painState.total_damage, pain_constants->hurt_medium_threshold);*/
		}
	}

	if (pain_type == ONcPain_Medium) {
		// determine whether we have to make a heavy pain sound
		if ((ioCharacter->painState.num_medium >= pain_constants->hurt_max_medium) ||
			(ioCharacter->painState.total_damage >= pain_constants->hurt_heavy_threshold)) {
			pain_type = ONcPain_Heavy;
/*			COrConsole_Printf("  num %d (%d) dmg %d (%d) -> escalate to heavy",
					ioCharacter->painState.num_medium, pain_constants->hurt_max_medium,
					ioCharacter->painState.total_damage, pain_constants->hurt_heavy_threshold);*/
		}
	}

	if (ioCharacter->painState.total_damage < pain_constants->hurt_volume_threshold) {
		// we haven't been hurt enough to be at maximum volume yet
		volume = pain_constants->hurt_min_volume + ioCharacter->painState.total_damage * (1.0f - pain_constants->hurt_min_volume)
															/ pain_constants->hurt_volume_threshold;
/*		COrConsole_Printf("  dmg %d (%d) -> volume %f (min %f)",
				ioCharacter->painState.total_damage, pain_constants->hurt_volume_threshold, volume, pain_constants->hurt_min_volume);*/
	} else {
		volume = 1.0f;
//		COrConsole_Printf("  dmg %d (%d) -> max volume", ioCharacter->painState.total_damage, pain_constants->hurt_volume_threshold);
	}

	// play the sound
	ONiCharacter_QueueHurtSound(ioCharacter, pain_type, volume);

/*	COrConsole_Printf("  done %s, now %d / %d, timer frames %d",
		ONcPainTypeName[ioCharacter->painState.last_sound_type], ioCharacter->painState.num_light, ioCharacter->painState.num_medium,
		ioCharacter->painState.last_sound_time + pain_constants->hurt_min_timer - current_time);*/
}

UUtBool ONrCharacter_PlayHurtSound(ONtCharacter *ioCharacter, UUtUns32 inHurtSoundType, float *inVolume)
{
	ONtPainConstants *pain_constants = &ioCharacter->characterClass->painConstants;
	SStImpulse *impulse_sound;
	M3tPoint3D position;

	switch(inHurtSoundType) {
		case ONcPain_Light:
			impulse_sound = pain_constants->hurt_light;
		break;

		case ONcPain_Medium:
			impulse_sound = pain_constants->hurt_medium;
		break;

		case ONcPain_Heavy:
			impulse_sound = pain_constants->hurt_heavy;
		break;

		case ONcPain_Death:
			impulse_sound = pain_constants->death_sound;
		break;

		default:
			UUmAssert(0);
		break;
	}

//	COrConsole_Printf("%s: play pain '%s'", ioCharacter->player_name, ONcPainTypeName[inHurtSoundType]);

	if (impulse_sound == NULL) {
		COrConsole_Printf("WARNING: character %s class %s has no hurt sound for '%s'",
						ioCharacter->player_name, ioCharacter->characterClass->variant->name, ONcPainTypeName[inHurtSoundType]);
		return UUcFalse;
	}

	// abort any playing speech, and play the pain sound
	ONrSpeech_Stop(ioCharacter, UUcTrue);
	ONrCharacter_GetEyePosition(ioCharacter, &position);
	OSrImpulse_Play(impulse_sound, &position, NULL, NULL, inVolume);

	return UUcTrue;
}

// template procedure handler for character class data
UUtError ONrCharacterClass_ProcHandler(TMtTemplateProc_Message inMessage, void *inInstancePtr,
										void *inPrivateData)
{
	ONtCharacterClass *character_class = (ONtCharacterClass *) inInstancePtr;

	// handle the message
	switch (inMessage)
	{
		case TMcTemplateProcMessage_DisposePreProcess:
			if (character_class->body_surface != NULL) {
				UUrMemory_Block_Delete(character_class->body_surface);
				character_class->body_surface = NULL;
			}
		break;
	}

	return UUcError_None;
}

// set up a character class's body-surface cache
UUtError ONrCharacterClass_MakeBodySurfaceCache(ONtCharacterClass *ioCharacterClass)
{
	ONtBodySurfaceCache *cache;
	TRtBody *body;
	UUtUns32 itr, flags, axis, num_parts;
	ONtBodySurfacePart *part;
	M3tVector3D part_size;

	if (ioCharacterClass->body_surface != NULL) {
		return UUcError_None;
	}

	num_parts = ONcNumCharacterParts;
	cache = UUrMemory_Block_NewClear(sizeof(ONtBodySurfaceCache) + (num_parts  - 1) * sizeof(ONtBodySurfacePart));
	if (cache == NULL) {
		return UUcError_OutOfMemory;
	}

	ioCharacterClass->body_surface = cache;
	cache->num_parts = num_parts;
	body = ioCharacterClass->body->body[TRcBody_SuperLow];
	for (itr = 0, part = cache->parts; itr < num_parts; itr++, part++) {
		flags = ONcBodySurfacePartMasks[itr];

		part->bbox = body->geometries[itr]->pointArray->minmax_boundingBox;
		MUmVector_Subtract(part_size, part->bbox.maxPoint, part->bbox.minPoint);

		// determine volume of the body part
		part->volume = part_size.x * part_size.y * part_size.z;
		cache->total_volume += part->volume;

		// determine length of the body part
		axis = (flags & ONcBodySurfaceFlag_MainAxisMask);
		if (axis == ONcBodySurfaceFlag_MainAxis_X) {
			part->length = part_size.x;

		} else if (axis == ONcBodySurfaceFlag_MainAxis_Y) {
			part->length = part_size.y;

		} else if (axis == ONcBodySurfaceFlag_MainAxis_Z) {
			part->length = part_size.z;

		} else {
			UUmAssert(0);
			part->length = 0;
		}
		cache->total_length += part->length;

		// determine surface area of the body part
		part->face_area.x = part_size.y * part_size.z;
		part->face_area.y = part_size.x * part_size.z;
		part->face_area.z = part_size.x * part_size.y;

		if (flags & ONcBodySurfaceFlag_Neg_X) {
			part->surface_area += part->face_area.x;
		}
		if (flags & ONcBodySurfaceFlag_Pos_X) {
			part->surface_area += part->face_area.x;
		}
		if (flags & ONcBodySurfaceFlag_Neg_Y) {
			part->surface_area += part->face_area.y;
		}
		if (flags & ONcBodySurfaceFlag_Pos_Y) {
			part->surface_area += part->face_area.y;
		}
		if (flags & ONcBodySurfaceFlag_Neg_Z) {
			part->surface_area += part->face_area.z;
		}
		if (flags & ONcBodySurfaceFlag_Pos_Z) {
			part->surface_area += part->face_area.z;
		}
		cache->total_surface_area += part->surface_area;
	}

	return UUcError_None;
}


static void ONrCharacter_VerifyExtraBody(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	if ((ioActiveCharacter != NULL) && (ioActiveCharacter->extraBody != NULL)) {
		UUtBool character_has_weapon_in_hand = ioCharacter->inventory.weapons[0] != NULL;
		UUtBool character_has_extra_body = ioActiveCharacter->extraBody->parts[0].geometry != NULL;

		if (character_has_weapon_in_hand && !character_has_extra_body) {
			COrConsole_Printf("character has a weapon but no extra body");

			ioActiveCharacter->extraBody->parts[0].geometry = WPrGetClass(ioCharacter->inventory.weapons[0])->geometry;
		}
		else if (!character_has_weapon_in_hand && character_has_extra_body) {
			COrConsole_Printf("character has no weapon but does have extra body");

			ioActiveCharacter->extraBody->parts[0].geometry = NULL;
		}
	}

	return;
}

void ONrCharacter_ListBrokenSounds(BFtFile *inFile)
{
	ONtCharacterClass *classptrs[256], *classptr;
	UUtUns32 itr, itr2, num_classes;
	char *classname;
	UUtError error;

	BFrFile_Printf(inFile, "********** Character Vocalizations **********"UUmNL);
	BFrFile_Printf(inFile, UUmNL);

	error = TMrInstance_GetDataPtr_List(TRcTemplate_CharacterClass, 256, &num_classes, classptrs);
	if (error != UUcError_None) {
		return;
	}

	for (itr = 0; itr < num_classes; itr++) {
		classptr = classptrs[itr];
		classname = TMrInstance_GetInstanceName(classptr);

		for (itr2 = 0; itr2 < AI2cVocalization_Max; itr2++) {
			OSrCheckBrokenAmbientSound(inFile, classname, (char *) AI2cVocalizationTypeName[itr2], classptr->ai2_behavior.vocalizations.sound_name[itr2]);
		}

		OSrCheckBrokenImpulseSound(inFile, classname, "hurt_light",		classptr->painConstants.hurt_light_name);
		OSrCheckBrokenImpulseSound(inFile, classname, "hurt_medium",	classptr->painConstants.hurt_medium_name);
		OSrCheckBrokenImpulseSound(inFile, classname, "hurt_heavy",		classptr->painConstants.hurt_heavy_name);
		OSrCheckBrokenImpulseSound(inFile, classname, "death_sound",	classptr->painConstants.death_sound_name);
	}

	BFrFile_Printf(inFile, UUmNL);
	BFrFile_Printf(inFile, UUmNL);
}


static void ONiCharacter_BuildTriggerSphere(ONtActiveCharacter *inActiveCharacter)
{
	// compute the center
	MUmVector_Add(inActiveCharacter->trigger_sphere.center, inActiveCharacter->trigger_points[0], inActiveCharacter->trigger_points[1]);
	MUmVector_Scale(inActiveCharacter->trigger_sphere.center, 0.5f);

	inActiveCharacter->trigger_sphere.radius = MUmVector_GetDistance(inActiveCharacter->trigger_sphere.center, inActiveCharacter->trigger_points[0]);

	return;
}

void ONrCharacter_NotifyTriggerVolumeDisable(UUtInt32 inID)
{
	ONtCharacter **active_character_list = ONrGameState_ActiveCharacterList_Get();
	UUtUns32 active_character_count = ONrGameState_ActiveCharacterList_Count();
	UUtUns32 character_itr;

	for(character_itr = 0; character_itr < active_character_count; character_itr++)
	{
		UUtUns32 trigger_itr;
		ONtCharacter *character = active_character_list[character_itr];
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(character);

		if (NULL != active_character) {
			for(trigger_itr = 0; trigger_itr < ONcMaxTriggersPerCharacter; trigger_itr++) {
				if (inID == active_character->current_triggers[trigger_itr]) {
					active_character->current_triggers[trigger_itr] = 0;
				}
			}
		}
	}

	return;
}

static void ONrCharacter_SetMovementThisFrame(ONtCharacter *inCharacter, ONtActiveCharacter *inActiveCharacter, M3tVector3D *inVector)
{
	MUmAssertVector(*inVector, 1000.f);

	inActiveCharacter->physics->velocity.x = 0.f;
	inActiveCharacter->physics->velocity.y = 0.f;
	inActiveCharacter->physics->velocity.z = 0.f;

	inActiveCharacter->movementThisFrame.x = inVector->x;
	inActiveCharacter->movementThisFrame.z = inVector->z;

	if (inVector->y < 0.f) {
		inActiveCharacter->movementThisFrame.y = 0.f;
		inActiveCharacter->gravityThisFrame = inVector->y;
	}
	else {
		inActiveCharacter->movementThisFrame.y = inVector->y;
		inActiveCharacter->gravityThisFrame = 0.f;
	}

	return;
}
