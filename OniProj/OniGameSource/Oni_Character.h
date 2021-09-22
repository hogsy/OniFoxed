#pragma once

/*	FILE:	Oni_Character.h
	
	AUTHOR:	Michael Evans
	
	CREATED: January 8, 1998
	
	PURPOSE: control of characters in ONI
	
	Copyright 1998, 1999

*/

#ifndef __ONI_CHARACTER_H__
#define __ONI_CHARACTER_H__

#include "BFW.h"
#include "BFW_Totoro.h"
#include "BFW_Motoko.h"
#include "BFW_TemplateManager.h"
#include "BFW_Physics.h"
#include "BFW_Object_Templates.h"

#include "Oni_Camera.h"
#include "Oni_AI.h"
#include "Oni_AI2.h"
#include "Oni_Object.h"
#include "Oni_Weapon.h"
#include "Oni_GameState.h"
#include "Oni_AI_Script.h"
#include "Oni_Film.h"
#include "Oni_Speech.h"

enum {
	ONcPelvis_Index		= 0,		// 0x00001
	ONcLLeg_Index		= 1,		// 0x00002
	ONcLLeg1_Index		= 2,		// 0x00004
	ONcLFoot_Index		= 3,		// 0x00008
	ONcRLeg_Index		= 4,		// 0x00010
	ONcRLeg1_Index		= 5,		// 0x00020
	ONcRFoot_Index		= 6,		// 0x00040
	ONcSpine_Index		= 7,		// 0x00080
	ONcSpine1_Index		= 8,		// 0x00100
	ONcNeck_Index		= 9,		// 0x00200	neck rfoot rleg1 lleg1 pelvis
	ONcHead_Index		= 10,		// 0x00400
	ONcLArm_Index		= 11,		// 0x00800
	ONcLArm1_Index		= 12,		// 0x01000
	ONcLArm2_Index		= 13,		// 0x02000
	ONcLHand_Index		= 14,		// 0x04000
	ONcRArm_Index		= 15,		// 0x08000
	ONcRArm1_Index		= 16,		// 0x10000
	ONcRArm2_Index		= 17,		// 0x20000
	ONcRHand_Index		= 18,		// 0x40000
	
	ONcWeapon_Index		= 19,		
	ONcMuzzle_Index		= 20	
	
};

#define ONcCharacter_InactiveFrames		90

#define ONcNumCharacterExtraParts ((UUtUns16) 10)
#define ONcSubSphereCount ((UUtUns8) 4)
#define ONcSubSphereCount_Mid ((UUtUns8) ONcSubSphereCount / 2)
#define ONcCharacterHeight (15.f)
#define ONcMinFloorNormalY	0.5f


#define ONcPickupRadius 16.f
#define ONcPickupMinY -6.f
#define ONcPickupMaxY 24.f

#define ONcStartPickupRadius 8.f
#define ONcStartPickupMinY -3.f
#define ONcStartPickupMaxY 18.f




typedef UUtBool
(*ONtBlockFunction)(
	ONtCharacter	*inCharacter);

typedef tm_struct ONtAirConstants
{
	float fallGravity;
	float jumpGravity;
	float jumpAmount;
	float maxVelocity;
	float jumpAccelAmount;
	UUtUns16 framesFallGravity;
	UUtUns16 numJumpAccelFrames;

	float damage_start;
	float damage_end;
} ONtAirConstants;

typedef tm_struct ONtShadowConstants
{
	M3tTextureMap			*shadow_texture;

	float	height_max;
	float	height_fade;

	float	size_max;
	float	size_fade;
	float	size_min;

	UUtUns16 alpha_max;
	UUtUns16 alpha_fade;

} ONtShadowConstants;

typedef tm_struct ONtJumpConstants
{
	float jumpDistance;
	UUtUns8 jumpHeight;
	UUtUns8 jumpDistanceSquares;
	tm_pad pad[2];
} ONtJumpConstants;



typedef tm_struct ONtAutofreezeConstants
{
	float distance_xz;
	float distance_y;
} ONtAutofreezeConstants;

typedef tm_struct ONtCoverConstants
{
	float rayIncrement;
	float rayMax;
	float rayAngle;
	float rayAngleMax;
} ONtCoverConstants;

typedef tm_struct ONtInventoryConstants
{
	UUtUns16 hypoTime;
	tm_pad	pad[2];
} ONtInventoryConstants;

typedef tm_struct ONtLODConstants
{
	float distance_squared[5];
} ONtLODConstants;


enum {
	AI2cBehaviorFlag_NeverStartle		= (1 << 0),
	AI2cBehaviorFlag_DodgeMasterEnable	= (1 << 1),
	AI2cBehaviorFlag_DodgeWhileFiring	= (1 << 2),
	AI2cBehaviorFlag_DodgeStopFiring	= (1 << 3),
	AI2cBehaviorFlag_RunningPickup		= (1 << 4)
};

// information about an AI character class's behavior.
// values in brackets are defaults
typedef tm_struct AI2tBehavior {
	UUtUns32				flags;

	/* movement */
	float					turning_nimbleness;						// (1.0)
	UUtUns16				dazed_minframes;
	UUtUns16				dazed_maxframes;						// these parameters are overridden by those in a character's melee profile if applicable
	UUtUns32				dodge_react_frames;
	float					dodge_time_scale;
	float					dodge_weight_scale;

	/* combat */
	AI2tCombatParameters	combat_parameters;
	UUtUns16				default_combat_id;
	UUtUns16				default_melee_id;

	AI2tVocalizationTable	vocalizations;

} AI2tBehavior;


#define ONcCharacterOffsetToBNV				15.f

//#define ONcShadowConstant_Size_Inner	(6.0f)
//#define ONcShadowConstant_Modulate_Inner	(0.6f)
//#define ONcShadowConstant_Size_Outer	(7.0f)
//#define ONcShadowConstant_Modulate_Outer	(0.8f)
#define ONcMaxVariantNameLength					32
#define ONcCharacter_MaxVariants				32

const ONtAirConstants *ONrCharacter_GetAirConstants(const ONtCharacter *inCharacter);


typedef tm_struct ONtVisionConstants
{
	float central_distance;
	float periph_distance;
	float vertical_range;
	float central_range;
	float central_max;
	float periph_range;
	float periph_max;
} ONtVisionConstants;

typedef tm_struct ONtMemoryConstants
{
	// decay times for contacts
	UUtUns32 definite_to_strong;
	UUtUns32 strong_to_weak;
	UUtUns32 weak_to_forgotten;

	// decay times for alert status
	UUtUns32 high_to_med;
	UUtUns32 med_to_low;
	UUtUns32 low_to_lull;
} ONtMemoryConstants;

typedef tm_struct ONtHearingConstants
{
	float acuity;
} ONtHearingConstants;

// a character variant is a common name of several classes and a pointer to a parent variant
#define ONcTemplate_CharacterVariant		UUm4CharToUns32('O', 'N', 'C', 'V')
typedef tm_template('O', 'N', 'C', 'V', "Oni Character Variant")
ONtCharacterVariant
{
	tm_templateref					parent;				// pointer to parent at runtime
	char							name[32];			// must match ONcMaxVariantNameLength above
	char							upgrade_name[32];
	
} ONtCharacterVariant;

#define ONcTemplate_VariantList		UUm4CharToUns32('O', 'N', 'V', 'L')
typedef tm_template('O', 'N', 'V', 'L', "Oni Variant List")
ONtVariantList
{
	tm_pad								pad[20];
	tm_varindex	UUtUns32				numVariants;
	tm_vararray	ONtCharacterVariant		*variant[1];
	
} ONtVariantList;

/*
 * character particle definitions
 */

#define ONcCharacterParticleNameLength		15		// remember to change TRtParticle.name[] to match
#define ONcCharacterParticle_HitFlash		0x8000

typedef tm_struct ONtCharacterParticle
{
	char			name[16];				// must match ONcCharacterParticleNameLength + 1
	char			particle_classname[64];	// must match P3cParticleClassNameLength + 1
	UUtUns16		override_partindex;		// may be [0 - ONcNumCharacterParts] for a specific part,
											// -1 for animation's specified part index,
											// ONcCharacterParticle_HitFlash means generate a 'killed' impact instead
	tm_pad			pad[2];

	// non-persistent data
	tm_raw(struct P3tParticleClass *) particle_class;
} ONtCharacterParticle;

#define ONcTemplate_CharacterParticleArray	UUm4CharToUns32('O', 'N', 'C', 'P')
typedef tm_template('O', 'N', 'C', 'P', "Oni Character Particle Array")
ONtCharacterParticleArray
{
	tm_pad								pad[16];
	UUtBool								classes_found;
	tm_pad								pad2[3];
	tm_varindex	UUtUns32				numParticles;
	tm_vararray	ONtCharacterParticle	particle[1];
	
} ONtCharacterParticleArray;

/*
 * character attack impact type definitions
 */

#define ONcCharacterAttackNameLength		15		// remember to change TRtAnimation.attackName[] to match

typedef tm_struct ONtCharacterAttackImpact
{
	char								name[16];				// must match ONcCharacterAttackNameLength + 1
	char								impact_name[128];
	char								modifier_name[16];

	// non-persistent data
	UUtUns16							impact_type;
	UUtUns16							modifier_type;

} ONtCharacterAttackImpact;

#define ONcTemplate_CharacterImpactArray	UUm4CharToUns32('O', 'N', 'I', 'A')
typedef tm_template('O', 'N', 'I', 'A', "Oni Character Impact Array")
ONtCharacterImpactArray
{
	tm_pad									pad[16];
	UUtBool									indices_found;
	tm_pad									pad2[3];
	tm_varindex	UUtUns32					numImpacts;
	tm_vararray	ONtCharacterAttackImpact	impact[1];
	
} ONtCharacterImpactArray;

#define ONcAnimationImpact_None			((UUtUns16) -1)
enum {
	ONcAnimationImpact_Walk			= 0,
	ONcAnimationImpact_Run			= 1,
	ONcAnimationImpact_Crouch		= 2,
	ONcAnimationImpact_Slide		= 3,
	ONcAnimationImpact_Land			= 4,
	ONcAnimationImpact_LandHard		= 5,
	ONcAnimationImpact_LandDead		= 6,
	ONcAnimationImpact_Knockdown	= 7,
	ONcAnimationImpact_Thrown		= 8,
	ONcAnimationImpact_StandingTurn	= 9,
	ONcAnimationImpact_RunStart		= 10,
	ONcAnimationImpact_1Step		= 11,
	ONcAnimationImpact_RunStop		= 12,
	ONcAnimationImpact_WalkStop		= 13,
	ONcAnimationImpact_Sprint		= 14,

	ONcAnimationImpact_Max			= 15		// DO NOT CHANGE without changing ONtCharacterAnimationImpacts arraysize
};

typedef tm_struct ONtCharacterAnimationImpact
{
	char								impact_name[128];

	// non-persistent data
	UUtUns16							impact_type;
} ONtCharacterAnimationImpact;

typedef tm_struct ONtCharacterAnimationImpacts
{
	UUtBool								indices_found;
	UUtUns8								pad;
	UUtUns16							modifier_type;
	char								modifier_name[16];

	ONtCharacterAnimationImpact			impact[15];				// must match ONcAnimationImpact_Max
	tm_pad								pad1[2];

} ONtCharacterAnimationImpacts;

#define ONcTemplate_BodyPartMaterials		UUm4CharToUns32('C', 'B', 'P', 'M')
typedef tm_template('C', 'B', 'P', 'M', "Character Body Part Material")
ONtBodyPartMaterials
{
	MAtMaterial				*materials[19];		// must match ONcNumCharacterParts

} ONtBodyPartMaterials;

#define ONcTemplate_BodyPartImpacts		UUm4CharToUns32('C', 'B', 'P', 'I')
typedef tm_template('C', 'B', 'P', 'I', "Character Body Part Impacts")
ONtBodyPartImpacts
{
	MAtImpact				*hit_impacts[19];		// must match ONcNumCharacterParts
	MAtImpact				*blocked_impacts[19];	// must match ONcNumCharacterParts
	MAtImpact				*killed_impacts[19];	// must match ONcNumCharacterParts
	
} ONtBodyPartImpacts;

/*
 * pain settings
 */

enum {
	ONcPain_Light,
	ONcPain_Medium,
	ONcPain_Heavy,
	ONcPain_Death,
	ONcPain_Max
};

extern const char *ONcPainTypeName[ONcPain_Max];

typedef tm_struct ONtPainConstants
{
	UUtUns16			hurt_base_percentage;
	UUtUns16			hurt_max_percentage;
	UUtUns16			hurt_percentage_threshold;
	UUtUns16			hurt_timer;
	UUtUns16			hurt_min_timer;
	UUtUns16			hurt_max_light;
	UUtUns16			hurt_max_medium;
	UUtUns16			hurt_death_chance;
	UUtUns16			hurt_volume_threshold;
	UUtUns16			hurt_medium_threshold;
	UUtUns16			hurt_heavy_threshold;
	UUtBool				sounds_found;
	tm_pad				pad[1];
	float				hurt_min_volume;

	char				hurt_light_name[32];	// must match SScMaxNameLength
	char				hurt_medium_name[32];
	char				hurt_heavy_name[32];
	char				death_sound_name[32];
	tm_raw(SStImpulse *)hurt_light;
	tm_raw(SStImpulse *)hurt_medium;
	tm_raw(SStImpulse *)hurt_heavy;
	tm_raw(SStImpulse *)death_sound;
} ONtPainConstants;

/*
 * body surface cache
 */

// bitmasks for the body surface flags
#define ONcBodySurfaceFlag_Neg_X			(1 << 0)
#define ONcBodySurfaceFlag_Pos_X			(1 << 1)
#define ONcBodySurfaceFlag_Neg_Y			(1 << 2)
#define ONcBodySurfaceFlag_Pos_Y			(1 << 3)
#define ONcBodySurfaceFlag_Neg_Z			(1 << 4)
#define ONcBodySurfaceFlag_Pos_Z			(1 << 5)

#define ONcBodySurfaceFlag_Both_X			(ONcBodySurfaceFlag_Neg_X | ONcBodySurfaceFlag_Pos_X)
#define ONcBodySurfaceFlag_Both_Y			(ONcBodySurfaceFlag_Neg_Y | ONcBodySurfaceFlag_Pos_Y)
#define ONcBodySurfaceFlag_Both_Z			(ONcBodySurfaceFlag_Neg_Z | ONcBodySurfaceFlag_Pos_Z)

#define ONcBodySurfaceFlag_MainAxisMask		(3 << 6)
#define ONcBodySurfaceFlag_MainAxis_X		(0 << 6)
#define ONcBodySurfaceFlag_MainAxis_Y		(1 << 6)
#define ONcBodySurfaceFlag_MainAxis_Z		(2 << 6)

extern const UUtUns32 ONcBodySurfacePartMasks[ONcNumCharacterParts];

typedef struct ONtBodySurfacePart {
	float				length;
	float				surface_area;
	float				volume;
	M3tVector3D			face_area;
	M3tBoundingBox_MinMax bbox;

} ONtBodySurfacePart;

typedef struct ONtBodySurfaceCache {
	UUtUns32			num_parts;
	float				total_length;
	float				total_surface_area;
	float				total_volume;
	ONtBodySurfacePart	parts[1];

} ONtBodySurfaceCache;

// a character class is a general version of a character that there might be many of
#define TRcTemplate_CharacterClass	UUm4CharToUns32('O', 'N', 'C', 'C')
tm_template('O', 'N', 'C', 'C', "Oni Character Class")
ONtCharacterClass
{
	ONtAirConstants			airConstants;			// controls how characters jump/fall
	ONtShadowConstants		shadowConstants;		// controls the shadows off characters
	ONtJumpConstants		jumpConstants;			// tells pathfinding about our jumping abilities
	ONtCoverConstants		coverConstants;			// cover finding technique
	ONtAutofreezeConstants	autofreezeConstants;	// Autofreeze parameters
	ONtInventoryConstants	inventoryConstants;		// Inventory stats
	ONtLODConstants			lodConstants;
	ONtPainConstants		painConstants;

	AI2tBehavior			ai2_behavior;

	ONtVisionConstants		vision;
	ONtMemoryConstants		memory;
	ONtHearingConstants		hearing;

	// CB: this is used for random-variant generation. if a character starting location has the
	// random variant flag, then it can pick any character class that has the same variant tag
	// as the character class that it is given. e.g. a position marked as comguy_1 with random
	// variant could pick comguy_1, comguy_2 or comguy_3 because they all have the same variant
	// tag of "comguy".
	ONtCharacterVariant		*variant;
	
	ONtCharacterParticleArray *particles;
	ONtCharacterImpactArray	*impacts;			// attack impacts
	ONtCharacterAnimationImpacts anim_impacts;	// animation impacts (footsteps, landing, etc)
	char					death_effect[64];		// must match P3cParticleClassNameLength + 1
	tm_raw(P3tParticleClass*)death_particleclass;
	tm_raw(ONtBodySurfaceCache*)body_surface;

	TRtBodySet				*body;
	TRtBodyTextures			*textures;
	ONtBodyPartMaterials	*bodyPartMaterials;
	ONtBodyPartImpacts		*bodyPartImpacts;

	UUtUns32				fightModeDuration;		// number of frames before fight mode fades
	UUtUns32				idleDelay;				// number of frames in standing before idle
	UUtUns32				idleRestartDelay;		// number of frames before second idle
	UUtUns32				class_max_hit_points;	// maximum & initial hit points
	UUtUns32				feetBits;				// these bits mark the characters feet

	float					scaleLow;
	float					scaleHigh;
	float					damageResistance[7];// must match P3cEnumDamageType_Max
	float					bossShieldProtectAmount;

	TRtAnimationCollection	*animations;
	TRtScreenCollection		*aimingScreens;
	
	UUtUns16				shottime;			// AI ROF
	UUtUns16				death_deletedelay;	// frames after death to delete character
	UUtBool					leftHanded;
	UUtBool					superHypo;			// character has a super-hypo effect
	UUtBool					superShield;		// character has a super-shield effect
	UUtBool					knockdownResistant;	// character resists being knocked down easily
};


typedef enum {
	ONcCharacterFlag_Dead_1_Animating		=	0x00000001,	// health is 0
	ONcCharacterFlag_Dead					=	ONcCharacterFlag_Dead_1_Animating,
	ONcCharacterFlag_Dead_2_Moving			=	0x00000002,	// no longer animating
	ONcCharacterFlag_Dead_3_Cosmetic		=	0x00000004,	// no longer moving
	ONcCharacterFlag_Dead_4_Gone			=	0x00000008,	// dead except but still drawn

	ONcCharacterFlag_HastyAnim				=	0x00000010,	// do this queued animation ASAP
	ONcCharacterFlag_Unkillable				=	0x00000020,	// the character cannot be killed, only defeated
	ONcCharacterFlag_InfiniteAmmo			=	0x00000040,	// the character always has infinite ammo
	ONcCharacterFlag_PleaseBlock			=	0x00000080,	// set if the character should block, cleared once block begins
	
	ONcCharacterFlag_Unstoppable			=	0x00000100,	// this character cannot be knocked down, staggered, stunned, etc
	ONcCharacterFlag_ScriptControl			=	0x00000200,	// set if the character is completely under script control
	ONcCharacterFlag_DeathLock				=	0x00000400,	// this character should never die all the way
	ONcCharacterFlag_WasUpdated				=	0x00000800,	// this character's animation was changed
		
	ONcCharacterFlag_BeingThrown			=	0x00001000,	// this character is being thrown
	ONcCharacterFlag_DontUseGunVarient		=	0x00002000,	// this character should not use a weapon varient
	ONcCharacterFlag_Draw					=	0x00004000,	// DoFrame has been executed for this character
	ONcCharacterFlag_InUse					=	0x00008000,	// this character is active and in use

	ONcCharacterFlag_DontUseFightVarient	=	0x00010000,
	ONcCharacterFlag_NoCollision			=	0x00020000,	// no collision for this character
	ONcCharacterFlag_Teleporting			=	0x00040000,	// this character is teleporting and does not accept collision
	ONcCharacterFlag_NoCorpse				=	0x00080000,	// no corpse for this character

	ONcCharacterFlag_ActiveLock				=	0x00100000,	// the character is locked active
	ONcCharacterFlag_ChrAnimate				=	0x00200000,	// the character is currently runing a chr_animate command
	ONcCharacterFlag_AIMovement				=	0x00400000,	// the character is using AI movement
	ONcCharacterFlag_NeutralUninterruptable	=	0x00800000,	// running an uninterruptable neutral interaction

	ONcCharacterFlag_NoShadow				=	0x01000000,	// 
	ONcCharacterFlag_Invincible				=	0x02000000,	// character is invincible
	ONcCharacterFlag_NoAutoDrop				=	0x04000000,	// character should not automatically drop items when killed (invisibility, shield, LSI)
	ONcCharacterFlag_RestrictedMovement		=	0x08000000,	// character cannot move fast (used for player holding barabbas' gun)

	ONcCharacterFlag_Boss					=	0x10000000,	// character is a boss (used for final muro group fight)
	ONcCharacterFlag_FinalFlash				=	0x20000000,	// 'final flash' has been played for this character's death
	ONcCharacterFlag_BossShield				=	0x40000000, // this character has the boss shield
	ONcCharacterFlag_WeaponImmune			=	0x80000000	// this character is immune to weapon damage

} ONtCharacterFlags;

typedef enum {
	ONcCharacterFlag2_WeaponEmpty			=	0x00000001, // character's weapon is empty, punch instead
	ONcCharacterFlag2_UltraMode				=	0x00000002

} ONtCharacterFlags2;

enum {
	ONcInAirFlag_Jumped		= 0x0001
};

typedef struct ONtInAirControl
{
	float				jump_height;
	UUtUns16			flags;
	UUtUns16			numFramesInAir;
	M3tVector3D			velocity;

	M3tVector3D			buildVelocity;
	UUtBool				buildValid;

	UUtBool				pickupStarted;
	UUtUns16			pickupReleaseTimer;
	M3tPoint3D			pickupStartPoint;
	ONtCharacter		*pickupOwner;
} ONtInAirControl;

enum
{
	ONcStitchFlag_NoInterpVelocity = 0x0001
};

typedef struct ONtStitchControl
{
	UUtBool				stitching;
	float				height;
	TRtAnimState		fromState;
	M3tVector3D			velocity;
	UUtUns16			itr;
	UUtUns16			count;
	M3tQuaternion		positions[ONcNumCharacterParts];
	UUtUns16			stitchFlags;
} ONtStitchControl;

typedef UUtUns16 ONtCharacterIndexType;

#define ONcCharacterIndex_None			0x0000
#define ONcCharacterIndexMask_Salt		0x7f00
#define ONcCharacterIndexMask_RawIndex	0x00ff
#define ONcCharacterIndexMask_Valid		0x8000

#define ONcCharacterSalt_Bits			7
#define ONcCharacterSalt_Shift			8

#define ONcMaxHitsPerAttack 8
#define ONcLastAttackTime_StillActive 0xffffffff

typedef struct ONtHitTable
{
	ONtCharacterIndexType hits[ONcMaxHitsPerAttack];
} ONtHitTable;


enum 
{
	ONcOverlayIndex_Recoil,
	ONcOverlayIndex_InventoryManagement,
	ONcOverlayIndex_HitReaction,
	ONcOverlayIndex_Debug,

	ONcOverlayIndex_Count
};

enum	// stored in charType
{
	ONcChar_Player	= 0,
	ONcChar_AI		= 1,
	ONcChar_AI2		= 2
};

enum
{
	ONcOverlayFlag_MagicHolster		= (1 << 0)
};

typedef struct ONtOverlay
{
	const TRtAnimation	*animation;
	TRtAnimTime			frame;
	UUtUns32			flags;
} ONtOverlay;

UUtBool ONrOverlay_DoFrame(ONtCharacter *ioCharacter, ONtOverlay *ioOverlay);
void ONrOverlay_Initialize(ONtOverlay *ioOverlay);
void ONrCharacter_ApplyOverlay(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter, const ONtOverlay *inOverlay);
void ONrOverlay_Set(ONtOverlay *ioOverlay, const TRtAnimation *inAnimation, UUtUns32 inFlags);
UUtBool ONrOverlay_IsActive(const ONtOverlay *inOverlay);

UUtBool ONrCharacter_Overlays_TestFlag(ONtCharacter *inCharacter, ONtActiveCharacter *ioActiveCharacter, TRtAnimFlag inWhichFlag);

typedef struct ONtCharacterParticleInstance
{
	ONtCharacterParticle	*particle_def;
	P3tParticleReference	particle_ref;
	P3tParticle				*particle;
	UUtUns32				bodypart_index;

} ONtCharacterParticleInstance;

typedef struct ONtPainState
{
	UUtUns16				total_damage;
	UUtUns16				decay_timer;
	UUtUns32				last_sound_type;
	UUtUns32				last_sound_time;
	UUtUns8					num_light;
	UUtUns8					num_medium;
	UUtUns16				queue_timer;
	UUtUns32				queue_type;
	float					queue_volume;

} ONtPainState;

#define ONcMaxTriggersPerCharacter 4

struct ONtCharacter
{
	ONtCharacterIndexType	index;					// index in ONgGameState->characters list 
	ONtCharacterIndexType	active_index;			// index in ONgGameState->activeCharacters or ONcCharacterIndex_None

 	// general properties
	UUtUns32				flags;	
	UUtUns32				flags2;
	ONtCharacterClass *		characterClass;
	UUtUns16				characterClassNumber;
	UUtUns16				teamNumber;
	char					player_name[ONcMaxPlayerNameLength];
	float					scale;
	UUtUns32				hitPoints;
	UUtUns32				maxHitPoints;
	UUtUns16				scriptID;
	AItScriptTable			scripts;

	// position and facing
	M3tPoint3D				location;
	M3tPoint3D				prev_location;
	M3tPoint3D				actual_position;		// CB: this is what used to be returned by ONrCharacter_GetPathfindingLocation
	float					facing;
	float					desiredFacing;
	float					facingModifier;			// signed > -pi, < pi
	M3tVector3D				facingVector;			// unit vector, cosmetic facing
	AKtBNVNode *			currentNode;			// The node the character is currently in, both BNV and pathfinding
	PHtNode *				currentPathnode;
	float					heightThisFrame;
	float					feetLocation;

	// weapon and inventory
	WPtInventory			inventory;
	M3tVector3D				weaponMovementThisFrame;// used for particle emission
	float					recoilSpeed;
	float					recoil;
	void *					pickup_target;

	// persistent animation information
	UUtUns32				animCounter;
	UUtUns32				lastActiveAnimationTime;
	UUtUns32				idleDelay;

	// AI variables
	UUtUns32				charType;
	AI2tState				ai2State;
	AI2tPathState			pathState;
	AI2tMovementOrders		movementOrders;
	AI2tMovementStub		movementStub;			// only used by inactive chararacters
	ONtBlockFunction		blockFunction;
	ONtCharacterSpeech		speech;
	ONtCharacter *			neutral_interaction_char;
	UUtUns32				neutral_keyabort_frames;
	AI2tFightState *		melee_fight;			// fight centred on this target

	// visibility
	UUtUns32				time_of_death;			// time from UUrMachineTime_Sixtieths() when the character died
	UUtUns32				num_frames_offscreen;
	UUtUns32				inactive_timer;
	UUtUns16				death_delete_timer;
	UUtUns16				pad;
	M3tBoundingBox_MinMax	boundingBox;
	float					distance_from_camera_squared;

	// super particle control
	float					external_super_amount;
	float					super_amount;

	// miscellany
	UUtUns8					hits[4];				// which of 4 quadrants the character was impacted
	UUtUns32				numKills;				// number of characters killed
	UUtUns32				damageInflicted;		// number of damage points of attacks landed
	UUtUns32				poisonLastFrame;
	UUtInt32				poisonFrameCounter;
	ONtActionMarker *		console_marker;
	UUtUns32				console_aborttime;
	ONtPainState			painState;
};

struct ONtActiveCharacter {
	ONtCharacterIndexType	index;					// parent's index in ONgGameState->characters list... may be 
	ONtCharacterIndexType	active_index;			// index in ONgGameState->activeCharacters list 

	/*
	 * physics, collision, movement
	 */

	PHtPhysicsContext *		physics; 
	PHtSphereTree			sphereTree[1];
	PHtSphereTree			leafNodes[ONcSubSphereCount];
	PHtSphereTree			midNodes[ONcSubSphereCount_Mid];

	UUtUns16				numWallCollisions;
	UUtUns16				numPhysicsCollisions;
	UUtUns16				numCharacterCollisions;
	UUtBool					collidingWithTarget;
	UUtUns8					pad0;

	M3tVector3D				knockback;	
	M3tVector3D				movementThisFrame;
	float					gravityThisFrame;
	M3tVector3D				lastImpact;				// used for weapon and powerup dropping

	UUtUns16				offGroundFrames;
	M3tPoint3D				groundPoint;
	ONtInAirControl			inAirControl;
	UUtUns32				pickup_percentage;
	UUtInt32				last_floor_gq;			// gq index or -1
	float					deferred_maximum_height;
	
	M3tBoundingSphere		boundingSphere;
	M3tPoint3D				trigger_points[2];
	M3tBoundingSphere		trigger_sphere;
	UUtInt32				current_triggers[ONcMaxTriggersPerCharacter];

	// superninja teleportation
	UUtBool					teleportEnable;
	M3tPoint3D				teleportDestination;
	float					teleportDestFacing;

	P3tParticleReference	lastParticleDamageRef;

	/*
	 * AI movement
	 */

	AI2tMovementState		movement_state;
	AI2tExecutorState		executor_state;


	/*
	 * animation system
	 */

	// animation and state control
	const TRtAnimation *	animation;

	TRtAnimState			nextAnimState;
	TRtAnimState			curFromState;

	TRtAnimType				curAnimType;
	TRtAnimType				nextAnimType;
	TRtAnimType				prevAnimType;
	TRtAnimType				pendingDamageAnimType;	// we have been hit, post this ASAP

	ONtStitchControl		stitch;
	ONtOverlay				overlay[ONcOverlayIndex_Count];
	
	// timers
	UUtUns16				animFrame;
	UUtUns16				softPauseFrames;
	UUtUns16				hardPauseFrames;
	UUtUns16				minStitchingFrames;
	UUtUns32				animationLockFrames;
	UUtUns16				hitStun;
	UUtUns16				blockStun;
	UUtUns16				dizzyStun;
	UUtUns16				staggerStun;
	UUtUns32				alertSoundAnimCounter;

	TRtAnimVarient			animVarient;
	UUtUns16				fightFramesRemaining;

	// animation particles
	UUtUns8					numParticles;
	UUtUns8					pad1;
	UUtUns16				activeParticles;
	ONtCharacterParticleInstance	particles[TRcMaxParticles];
	TRtParticle	*			animParticles;

	// super particles
	UUtBool					superParticlesCreated;
	UUtBool					superParticlesActive;
	UUtUns16				numSuperParticles;
	ONtCharacterParticleInstance	superParticles[TRcMaxParticles];

	// attack info
	TRtAnimType				lastAttack;
	UUtUns32				lastAttackTime;
	ONtHitTable				hitTables[TRcMaxAttacks];

	UUtUns16				throwing;				// character index of who I am throwing
	UUtUns16				thrownBy;				// character index of who I am being thrown by
	ONtCharacter *			throwTarget;
	const TRtAnimation *	targetThrow;
	
	// aiming screens
	const TRtAimingScreen *	aimingScreen;
	UUtUns32				oldAimingScreenTime;
	const TRtAimingScreen *	oldAimingScreen;

	// current pose
	M3tQuaternion			curAnimOrientations[ONcNumCharacterParts];
	M3tQuaternion			curOrientations[ONcNumCharacterParts];


	/*
	 * input
	 */

	UUtUns32				lastHeartbeat;
	ONtInputState			inputState;
	LItButtonBits			inputOld;
	UUtUns32				last_forward_tap;		// used for sprinting
	UUtBool					frozen;

	// aiming
	UUtBool					isAiming;
	M3tPoint3D				aimingTarget;
	M3tVector3D				aimingVector;		
	M3tVector3D				cameraVector;		
	float					aimingLR;
	float					aimingUD;
	float					aimingVectorLR;
	float					aimingVectorUD;
	float					autoAimAdjustmentLR;
	float					autoAimAdjustmentUD;
	UUtBool					oldExactAiming;
	UUtUns32				oldExactAimingTime; 

	// action flags
	UUtBool					pleaseJump;
	UUtBool					pleaseLand;
	UUtBool					pleaseLandHard;
	UUtBool					pleaseAutoFire;
	UUtBool					pleaseFire;
	UUtBool					pleaseFireAlternate;
	UUtBool					pleaseContinueFiring;
	UUtBool					pleaseReleaseTrigger;
	UUtBool					pleaseReleaseAlternateTrigger;
	UUtBool					pleaseTurnLeft;
	UUtBool					pleaseTurnRight;
	UUtBool					pad3;

	// script control
	LItButtonBits			keyHoldMask;
	UUtUns16				keyHoldFrames;	
	ONtFilmState			filmState;


	/*
	 * rendering
	 */

	// drawing information
	TRtBodySelector			baseBody;
	TRtBodySelector			curBody;
	TRtBodySelector			desiredBody;
	UUtUns16				desiredBodyFrames;		// number of frames we wish to switch
	TRtExtraBody *			extraBody;
	UUtUns8					extra_body_memory[UUcProcessor_CacheLineSize + sizeof(TRtExtraBody) + (1 - 1) * sizeof(TRtExtraBodyPart)];
	M3tMatrix4x3			matricies[ONcNumCharacterParts + ONcNumCharacterExtraParts];

	// shadow
	P3tDecalData			shadow_decal;
	UUtUns16				shadow_index;			// index into gamestate array
	UUtBool					shadow_active;
	UUtBool					pad4;

	// shield info
	UUtBool					shield_active;
	UUtBool					daodan_shield_effect;
	UUtUns8					shield_parts[ONcNumCharacterParts];

	// invis info
	UUtBool					invis_active;
	float					invis_character_alpha;

	// DEBUGGING INFO
	UUtUns16				flash_count;
	UUtUns32				flash_parts;

	// death
	M3tVector3D				death_velocity;
	UUtUns16				death_still_timer;
	UUtUns16				death_moving_timer;
};

extern ONtCharacter *ONgPlayer;
extern UUtUns16 gCharacterResolution;
extern UUtUns16 ONgNumIncrementalJumpFrames;
extern float ONgAimingWidth;				// console variable
extern UUtBool ONgDebugCharacterImpacts;
extern UUtBool ONgBoxCollisionEnable;
extern UUtBool gDebugGunBehavior;

// current trigger volume that is updating (used for script debugging)
extern struct OBJtOSD_TriggerVolume *ONgCurrentlyActiveTrigger;
extern char *ONgCurrentlyActiveTrigger_Script;
extern struct ONtCharacter *ONgCurrentlyActiveTrigger_Cause;

static UUcInline ONtCharacterIndexType ONrCharacter_GetIndex(const ONtCharacter *inCharacter) {
	return (inCharacter->index & ONcCharacterIndexMask_RawIndex);
}

static UUcInline ONtCharacterIndexType ONrCharacter_GetActiveIndex(const ONtCharacter *inCharacter) {
	return (inCharacter->active_index & ONcCharacterIndexMask_RawIndex);
}

static UUcInline ONtCharacterIndexType ONrActiveCharacter_GetIndex(const ONtActiveCharacter *inActiveCharacter) {
	return (inActiveCharacter->index & ONcCharacterIndexMask_RawIndex);
}

static UUcInline ONtCharacterIndexType ONrActiveCharacter_GetActiveIndex(const ONtActiveCharacter *inActiveCharacter) {
	return (inActiveCharacter->active_index & ONcCharacterIndexMask_RawIndex);
}

static UUcInline UUtBool ONrCharacter_IsActive(const ONtCharacter *inCharacter) {
	return (inCharacter->active_index != ONcCharacterIndex_None);
}

ONtCharacterClass *ONrGetCharacterClass(const char *inString);

TRtBody *ONrCharacter_GetBody(ONtCharacter *inCharacter, TRtBodySelector inWhichBody);
const TRtBody *ONrCharacter_GetBody_Const(const ONtCharacter *inCharacter, TRtBodySelector inWhichBody);
UUtUns16 ONrCharacter_GetNumParts(const ONtCharacter *inCharacter);

UUtError ONrCharacter_LevelBegin(void);
UUtError ONrCharacter_LevelEnd(void);

// character class functions
void ONrCharacter_SetCharacterClass(ONtCharacter *inCharacter, ONtCharacterClass *inClass);

UUtError ONrLookupString(const char *inString, const TMtStringArray *inMapping, UUtUns16 *result);

UUtError ONrCharacter_StringToAnimType(const char *inString, UUtUns16 *result);
UUtError ONrCharacter_StringToAnimState(const char *inString, UUtUns16 *result);

char *ONrCharacter_AnimTypeToString(UUtUns16 inType);
char *ONrCharacter_AnimStateToString(UUtUns16 inState);
TRtAnimType ONrStringToAnimType(const char *inString);
TRtAnimState ONrStringToAnimState(const char *inString);

UUtUns16 ONrCharacter_ForcedFollowingAnimType(ONtCharacter *ioCharacter, UUtUns16 inPrevAnimType);

// tells a character to look up an animation
void ONrCharacter_NextAnimation(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);

//
// === facing === 
//

//
// gets the current aiming screen for the character
//

const TRtAimingScreen *ONrCharacter_GetAimingScreen(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);


void ONrCharacter_SetFacing(ONtCharacter *ioCharacter, float inFacing);
void ONrCharacter_SetDesiredFacing(ONtCharacter *ioCharacter, float inFacing);
void ONrCharacter_AdjustFacing(ONtCharacter *ioCharacter, float inFacingDelta);
void ONrCharacter_AdjustDesiredFacing(ONtCharacter *ioCharacter, float inFacingDelta);

// 
// gets the difference (in signed angle format) to add to reality to get the desire
//
// range is -2 Pi to + 2Pi and the direction around the circle we pick is the direction
// that would take us across our front facing angle
//

float ONrCharacter_GetDesiredFacingOffset(const ONtCharacter *inCharacter);


typedef enum {
	ONcAnimPriority_Appropriate	= 0x0001,
	ONcAnimPriority_Queue		= 0x0002,
	ONcAnimPriority_Force		= 0x0004,
	ONcAnimPriority_BackInTime	= 0x0008
} ONtAnimPriority;

void ONrCharacter_ForceStand(ONtCharacter *inCharacter);
const TRtAnimation *ONrCharacter_DoAnimation(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter,
											ONtAnimPriority inForceOrQueue, UUtUns16 inAnimType);

#define ONrCharacter_QueueAnimation(ioCharacter, inAnimType) \
	ONrCharacter_DoAnimation(ioCharacter, ONcAnimPriority_Queue, inAnimType)

// if there is a better varient match for this animation, switch to it
void ONrCharacter_UpdateAnimationForVarient(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);

void ONrCharacter_SetAnimationInternal(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter,
									   TRtAnimState fromState, TRtAnimType nextAnimType, const TRtAnimation *inAnimation);

// tells a charcter to play a knockdown animation
void ONrCharacter_Knockdown(ONtCharacter *attacker, ONtCharacter *defender, UUtUns16 animType);
void ONrCharacter_Block(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);

UUtBool ONrCharacter_CouldBlock(ONtCharacter *inDefender, ONtCharacter *inAttacker,
								UUtBool *outBlockLow, UUtBool *outBlockHigh);

// console variables
UUtError ONrGameState_InstallConsoleVariables(void);

// Character creation and deletion
UUtError
ONrGameState_NewCharacter(
	const OBJtObject			*inStartPosition,
	const AItCharacterSetup		*inSetup,
	const ONtFlag				*inFlag,
	UUtUns16					*outCharacterIndex);

UUtError
ONrGameState_NewActiveCharacter(
	ONtCharacter				*inCharacterIndex,
	UUtUns16					*outActiveCharacterIndex,
	UUtBool						inCopyCurrent);

void
ONrGameState_DeleteCharacter(
	ONtCharacter				*inCharacter);
	
void
ONrGameState_DeleteActiveCharacter(
	ONtActiveCharacter			*ioActiveCharacter,
	UUtBool						inCopyCurrent);

// call one per heartbeat to update all the characters in the game
void ONrGameState_UpdateCharacters(void);

UUtInt16
ONrGameState_GetPlayerStart(void);

// displays all the characters
void ONrGameState_DisplayCharacters(void);
void ONrGameState_DisplayAiming(void);
void ONrGameState_NotifyCameraCut(void);


// runs collision detection
void ONrDoCharacterCollision(void);

UUtBool ONrAnimType_IsVictimType(TRtAnimType inAnimType);
UUtBool ONrAnimType_IsThrowSource(TRtAnimType inAnimType);
UUtBool ONrAnimType_ThrowIsFromFront(TRtAnimType inAnimType);
UUtBool ONrAnimType_IsKnockdown(TRtAnimType inAnimType);
UUtBool ONrAnimType_IsStartle(TRtAnimType inAnimType);
UUtBool ONrAnimState_IsFallen(TRtAnimState inAnimState);
UUtBool ONrAnimState_IsCrouch(TRtAnimState inAnimState);
TRtAnimType ONrAnimType_UpgradeToKnockdown(TRtAnimType inAnimType);

void ONrCharacter_GetPartLocation(const ONtCharacter *inCharacter, ONtActiveCharacter *inActiveCharacter,
									UUtUns16 inPartIndex, M3tPoint3D *outLocation);
float ONrCharacter_GetCosmeticFacing(const ONtCharacter *inCharacter);
void ONrCharacter_GetFacingVector(const ONtCharacter *inCharacter, M3tVector3D *outVector);
void ONrCharacter_GetAttackVector(const ONtCharacter *inCharacter, ONtActiveCharacter *inActiveCharacter, M3tVector3D *outVector);

void	ONrCharacter_NotIdle(ONtCharacter *ioCharacter);

UUtBool ONrAnim_IsPickup(TRtAnimType inAnimType);

UUtBool ONrCharacter_IsKnockdownResistant(ONtCharacter *ioCharacter);
UUtBool ONrCharacter_IsInvincible(ONtCharacter *ioCharacter);
UUtBool ONrCharacter_IsUnstoppable(ONtCharacter *ioCharacter);
UUtBool ONrCharacter_HasSuperShield(ONtCharacter *ioCharacter);

UUtBool ONrActiveCharacter_IsPlayingThisAnimationName(ONtActiveCharacter *inActiveCharacter, const char *inAnimationName);
UUtBool ONrCharacter_IsBusyInventory(const ONtCharacter *inCharacter);
UUtBool ONrCharacter_IsStanding(const ONtCharacter *inCharacter);
UUtBool ONrCharacter_CanUseConsole(const ONtCharacter *inCharacter);
UUtBool ONrCharacter_IsStill(const ONtCharacter *inCharacter);
UUtBool ONrCharacter_IsStandingStill(const ONtCharacter *inCharacter);
UUtBool ONrCharacter_IsVictimAnimation(const ONtCharacter *inCharacter);
UUtBool ONrCharacter_IsIdle(const ONtCharacter *inCharacter);
UUtBool ONrCharacter_IsDefensive(const ONtCharacter *inCharacter);
UUtBool ONrCharacter_IsStill(const ONtCharacter *inCharacter);
UUtBool ONrCharacter_IsMovingBack(const ONtCharacter *inCharacter);
UUtBool ONrCharacter_IsStandingRunning(const ONtCharacter *inCharacter);
UUtBool ONrCharacter_IsStandingRunningForward(const ONtCharacter *inCharacter);
UUtBool ONrCharacter_IsUnableToTurn(const ONtCharacter *inCharacter);
UUtBool ONrCharacter_IsPickingUp(const ONtCharacter *inCharacter);
UUtBool ONrCharacter_IsStartling(const ONtCharacter *inCharacter, UUtUns16 *outStartleFrame);
UUtBool ONrCharacter_IsStaggered(const ONtCharacter *inCharacter);
UUtBool ONrCharacter_IsFallen(const ONtCharacter *inCharacter);
UUtBool ONrCharacter_IsDizzy(const ONtCharacter *inCharacter);
UUtBool ONrCharacter_IsInactiveUpright(const ONtCharacter *inCharacter);
UUtBool ONrCharacter_IsPoweringUp(const ONtCharacter *inCharacter);
UUtBool ONrCharacter_IsPlayingFilm(const ONtCharacter *inCharacter);

// used by AI2 to determine whether we might overshoot a path point
UUtBool ONrCharacter_IsKeepingMoving(ONtCharacter *inCharacter);

float ONrCharacter_RelativeAngleToCharacter(const ONtCharacter *inFromChar, const ONtCharacter *inToChar);
void ONrCharacter_DropWeapon(ONtCharacter *ioCharacter, UUtBool inWantKnockAway, UUtUns16 inSlotNum, UUtBool inStoreOnly);
void ONrCharacter_NotifyReleaseWeapon(ONtCharacter *ioCharacter);

void ONrCharacter_BuildQuaternions(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);
void ONrCharacter_BuildMatriciesAll(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);
void ONrCharacter_BuildMatricies(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter, UUtUns32 mask);
void ONrCharacter_BuildApproximateBoundingVolumes(ONtCharacter *ioCharacter);
void ONrCharacter_BuildBoundingVolumes(ONtCharacter	*ioCharacter, ONtActiveCharacter *ioActiveCharacter);
M3tMatrix4x3 *ONrCharacter_GetMatrix(ONtCharacter *inCharacter, UUtUns32 inIndex);		// note: returns NULL if character is inactive

void ONrCharacter_MinorHitReaction(ONtCharacter *ioCharacter);
void ONrCharacter_FaceAtPoint_Raw(ONtCharacter *inCharacter, const M3tPoint3D *inPointToFace);
UUtInt32 ONrCharacter_FaceAwayOrAtVector_Raw(ONtCharacter *inCharacter, const M3tPoint3D *inPointToFace);
void ONrCharacter_FaceVector_Raw(ONtCharacter *inCharacter, const M3tVector3D *inVectorToFace);
float ONrCharacter_RelativeAngleToCharacter_Raw(const ONtCharacter *inFromChar, const ONtCharacter *inToChar);
float ONrCharacter_RelativeAngleToPoint_Raw(const ONtCharacter *inFromChar, const M3tPoint3D *inPoint);

void ONrCharacter_TakeDamage(
		ONtCharacter		*ioCharacter,
		UUtUns32			damage,
		float				inKnockback,
		const M3tPoint3D	*inLocation,
		const M3tVector3D	*inDirection,
		UUtUns32			inDamageOwner,
		TRtAnimType			deathAnimType);

void ONrCharacter_DamageCompass(ONtCharacter *ioCharacter, const M3tPoint3D *inLocation);

void ONrCharacter_Die(ONtCharacter *ioCharacter);

void ONrCharacter_AutoTrigger(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);
void ONrCharacter_Trigger(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);
void ONrCharacter_ReleaseTrigger(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);
void ONrCharacter_AlternateTrigger(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);
void ONrCharacter_ReleaseAlternateTrigger(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);

void ONrCharacters_Collision(	UUtUns16				inIgnoreCharacterIndex,
								const M3tVector3D		*inVector,
								const M3tMatrix4x3		*inGeometryMatrix,	
								const M3tGeometry		*inGeometry,
								ONtCharacter			**outCharacterPtr,
								UUtUns16				*outPartIndex);

static void UUcInline ONrCharacter_GetPathfindingLocation(const ONtCharacter *inCharacter, M3tPoint3D *outPoint) {
	*outPoint = inCharacter->actual_position;
}

void ONrCharacter_GetEarPosition(const ONtCharacter *inCharacter, M3tPoint3D *outPoint);
void ONrCharacter_GetEyePosition(const ONtCharacter *inCharacter, M3tPoint3D *outPoint);
void ONrCharacter_GetPelvisPosition(const ONtCharacter *inCharacter, M3tPoint3D *outPoint);
void ONrCharacter_GetFeetPosition(const ONtCharacter *inCharacter, M3tPoint3D *outPoint);
void ONrCharacter_Teleport(ONtCharacter *ioCharacter, M3tPoint3D *inPoint, UUtBool inRePath);
void ONrCharacter_Teleport_With_Collision(ONtCharacter *ioCharacter, M3tPoint3D *inPoint, UUtBool inRePath);
void ONrCharacter_FindOurNode(ONtCharacter *inCharacter);
UUtBool ONrCharacter_LineOfSight(ONtCharacter *inLooker, ONtCharacter *inLookee);

typedef enum 
{
	ONcVision_UseVisionCone,
	ONcVision_Look360
} ONtIsVisibleType;

float ONrCharacter_GetLeafSphereRadius(ONtCharacter *ioCharacter);
UUtBool ONrCharacter_IsAI(const ONtCharacter *inCharacter);
void ONrCharacter_GetVelocityEstimate(const ONtCharacter *inCharacter, ONtActiveCharacter *ioActiveCharacter, M3tVector3D *outVelocity);
void ONrCharacter_GetCurrentWorldVelocity(ONtCharacter *inCharacter, M3tVector3D *outVelocity);
void ONrCharacter_FightMode(ONtCharacter *ioCharacter);
void ONrCharacter_ExitFightMode(ONtCharacter *ioCharacter);
void ONrCharacter_BuildJump(ONtCharacter *inCharacter, ONtActiveCharacter *ioActiveCharacter);
UUtBool ONrCharacter_InAir(ONtCharacter *inCharacter);
UUtBool ONrCharacter_IsGunThroughWall(ONtCharacter *inCharacter, ONtActiveCharacter *ioActiveCharacter);
void ONrCharacter_SetAnimation_External(ONtCharacter *ioCharacter, TRtAnimState fromState, const TRtAnimation *inNewAnim, UUtInt32 inStitchCount);

TRtAnimType ONrCharacter_CalculateAnimType(ONtCharacter *attacker, ONtCharacter *defender,
										   ONtActiveCharacter *defender_active, UUtBool inApplyResistance, TRtAnimType inAnimType);

UUtBool ONrAnimType_IsLandType(TRtAnimType type);

void ONrDrawFacingVector(
	ONtCharacter*	inCharacter);

void ONrDrawSphere(
	M3tTextureMap	*inTexture,
	float			inScale,
	M3tPoint3D		*inLocation);
	
void ONrCharacter_DoAiming(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);
UUtBool ONiPointInSphere(const M3tPoint3D *inPoint, const M3tBoundingSphere *inSphere);
UUtBool ONrAnimation_IsFlying(const TRtAnimation *inAnimation);

void ONrCharacter_SetSprintVarient(ONtCharacter *ioCharacter, UUtBool inSprint);
void ONrCharacter_SetPanicVarient(ONtCharacter *ioCharacter, UUtBool inPanic);
void ONrCharacter_DisableWeaponVarient(ONtCharacter *ioCharacter, UUtBool inDisable);
void ONrCharacter_DisableFightVarient(ONtCharacter *ioCharacter, UUtBool inDisable);
void ONrCharacter_ResetWeaponVarient(ONtCharacter *ioCharacter);

UUtError ONrCharacter_ArmByName(ONtCharacter *inCharacter, char *weaponName);
UUtError ONrCharacter_ArmByNumber(ONtCharacter *inCharacter, UUtUns32 inWeaponNumber);
void ONrCharacter_UseWeapon(ONtCharacter *ioCharacter, WPtWeapon *inWeapon);
void ONrCharacter_UseNewWeapon(ONtCharacter *inCharacter, WPtWeaponClass *inClass);
void ONrCharacter_UseWeapon_NameAmmo(ONtCharacter *inCharacter, char *inWeaponName, UUtUns32 inAmmo);

void ONrCharacter_HandlePickupInput(
	ONtInputState *inInput,
	ONtCharacter *inCharacter,
	ONtActiveCharacter *inActiveCharacter);

void
ONrCharacter_SetHitPoints(
	ONtCharacter		*ioCharacter,
	UUtUns32			inHitPoints);
	
void
ONrCharacter_Spawn(
	ONtCharacter		*ioCharacter);

void ONrCharacter_EnablePhysics(ONtCharacter *inCharacter);
UUtBool ONrCharacter_SnapToFloor(ONtCharacter *ioCharacter);
UUtUns32 ONrCharacter_FindFloorWithRay(ONtCharacter *ioCharacter, UUtUns32 inFramesOfGravity, M3tPoint3D *inPoint, M3tVector3D *outPoint);

void ONrCharacter_Callback_PreCollision(
	struct PHtPhysicsContext *ioContext);

void
ONrCharacter_Callback_PreFilter(
	struct PHtPhysicsContext *ioContext,
	const M3tVector3D *inVector,
	struct PHtCollider *ioColliderArray,
	UUtUns16 *ioColliderCount);

void
ONrCharacter_Callback_PostCollision(
	struct PHtPhysicsContext *ioContext,
	const M3tVector3D *inVector,
	struct PHtCollider *ioColliderArray,
	UUtUns16 *ioColliderCount);

void
ONrCharacter_Callback_PostUpdate(
	struct PHtPhysicsContext *ioContext,
	struct PHtCollider *ioColliderArray,
	UUtUns16 *ioColliderCount);

void
ONrCharacter_Callback_PreDispose(
	struct PHtPhysicsContext *ioContext);

void
ONrCharacter_Callback_FindObjCollisions(
	const struct PHtPhysicsContext *inContext);

UUtBool ONrCharacter_NeedCollision(
	ONtCharacter *inCharacter);

void ONrCharacter_NewAnimationHook(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);

void ONrCharacter_BuildPhysicsFlags(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);

void ONrCharacter_UpdateInput(ONtCharacter *ioCharacter, const ONtInputState *inInput);

typedef enum
{
	ONcCharacterIsLeftHanded,
	ONcCharacterIsRightHanded
} ONtCharacterHanded;

void ONrCharacter_SetHand(TRtExtraBody *inExtraBody, ONtCharacterHanded inHanded);
void ONrCharacter_RecordGeometry(M3tGeometry *inGeometry, M3tMatrix4x3 *inMatrix);

void ONrCharacter_Heal(
		ONtCharacter		*ioCharacter,
		UUtUns32			inAmt,
		UUtBool				inAllowOverMax);

void ONrCharacter_PickupWeapon(
	ONtCharacter *inCharacter,
	WPtWeapon *inWeapon,
	UUtBool inForceHolster);

void ONrCharacter_PickupPowerup(
	ONtCharacter *inCharacter,
	WPtPowerup *inPowerup);
	
float ONrCharacter_GetTorsoHeight(const ONtCharacter *inCharacter);
float ONrCharacter_GetHeadHeight(const ONtCharacter *inCharacter);


void ONrCharacter_DumpCollection(BFtFile *inFile, TRtAnimationCollection *inCollection);
UUtBool ONrCharacter_InBadVarientState(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);

// inventory related functions
UUtBool ONrCharacter_HasAmmoForReload(ONtCharacter *ioCharacter, WPtWeapon *ioWeapon);
UUtBool ONrAnimType_IsReload(TRtAnimType inAnimType);
UUtBool ONrCharacter_IsReloading(ONtCharacter *ioCharacter);
UUtBool ONrCharacter_Reload(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);
UUtBool ONrCharacter_Holster(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter, UUtBool inForceHolster);
void ONrCharacter_Draw(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter, WPtWeapon *inWeapon);
UUtBool ONrCharacter_OutOfAmmo(ONtCharacter *ioCharacter);
UUtBool ONrCharacter_IsCaryingWeapon(ONtCharacter *ioCharacter);

// get a character class's average movement speed in units per frame
float ONrCharacterClass_MovementSpeed(ONtCharacterClass *inClass, AI2tMovementDirection inDirection,
									  AI2tMovementMode inMoveMode, UUtBool inWeapon, UUtBool inTwoHandedWeapon);

// get a character class's average stopping distance
float ONrCharacterClass_StoppingDistance(ONtCharacterClass *inClass, AI2tMovementDirection inDirection,
										AI2tMovementMode inMoveMode);

// console action functions
void ONrCharacter_ConsoleAction_Begin(ONtCharacter *inCharacter, ONtActionMarker *inActionMarker);
void ONrCharacter_ConsoleAction_Finish(ONtCharacter *inCharacter, UUtBool inSuccess);
void ONrCharacter_ConsoleAction_Update(ONtCharacter *inCharacter, ONtActiveCharacter *ioActiveCharacter);

// set up an animation intersection context for determining whether one character can strike another
void ONrSetupAnimIntersectionContext(ONtCharacter *inAttacker, ONtCharacter *inTarget,
									 UUtBool inConsiderAttack, TRtAnimIntersectContext *outContext);

// revert an animation intersection context to its original values
void ONrRevertAnimIntersectionContext(TRtAnimIntersectContext *ioContext);

// turn a character's health into a color
IMtShade ONrCharacter_GetHealthShade(UUtUns32 inHitPoints, UUtUns32 inMaxHitPoints);

void ONrCharacter_Shield_Update(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);
void ONrCharacter_Shield_HitParts(ONtCharacter *ioCharacter, UUtUns32 inMask);
UUtUns32 ONrCharacter_FindParts(const ONtCharacter *inCharacter, M3tPoint3D *inLocation, float inRadius);
UUtUns32 ONrCharacter_FindClosestPart(const ONtCharacter *inCharacter, M3tPoint3D *inLocation);

// hooks for scripting control of triggers
void ONrGameState_FireTrigger_Entry(struct OBJtOSD_TriggerVolume *inVolume, ONtCharacter *inCharacter);
void ONrGameState_FireTrigger_Inside(struct OBJtOSD_TriggerVolume *inVolume, ONtCharacter *inCharacter);
void ONrGameState_FireTrigger_Exit(struct OBJtOSD_TriggerVolume *inVolume, ONtCharacter *inCharacter);
UUtInt32 ONrGameState_CountInsideTrigger(OBJtObject *inObject);
void ONrGameState_KillCorpsesInsideTrigger(OBJtObject *inObject);
void ONrGameState_DeleteCharactersInsideTrigger(OBJtObject *inObject);

// force a character to holster their weapon - used by cinematics
void ONrCharacter_CinematicHolster(ONtCharacter *inCharacter);
void ONrCharacter_CinematicEndHolster(ONtCharacter *inCharacter, UUtBool inForceDraw);

void ONrCharacter_RecalculateIdleDelay(ONtCharacter *ioCharacter);

// play footstep, land, knockdown, etc.
UUtUns16 ONrCharacter_GetAnimationImpactType(UUtUns16 inAnimType);
void ONrCharacter_PlayAnimationImpact(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter, TRtFootstepKind inFootstep, M3tPoint3D *inOptionalLocation);
void ONrCharacter_RecalculateSoundPointers(void);

// for making debugging strings
const char *ONrAnimTypeToString(TRtAnimType inAnimType);
const char *ONrAnimStateToString(TRtAnimType inAnimType);

// activation routines
UUtError ONrCharacter_MakeActive(ONtCharacter *ioCharacter, UUtBool	inCopyCurrent);
UUtError ONrCharacter_MakeInactive(ONtCharacter *ioCharacter);

// accessors to active character info
ONtActiveCharacter *ONrGetActiveCharacter(const ONtCharacter *inCharacter);	// returns NULL if not active
ONtActiveCharacter *ONrForceActiveCharacter(ONtCharacter *inCharacter);		// makes character active (note: may still return NULL if max active chars exceeded)
																			// IMPORTANT: DO NOT call this unless you want to force a character to remain active
																			// even when they are offscreen... the CPU implications of calling this too often
																			// aren't pretty!

TRtAnimType ONrCharacter_GetAnimType(const ONtCharacter *inCharacter);

const TRtAnimation *ONrCharacter_TryImmediateAnimation(ONtCharacter *ioCharacter, TRtAnimType inAnimType);

// get the material type of a character
MAtMaterialType ONrCharacter_GetMaterialType(ONtCharacter *inCharacter, UUtUns32 inPartIndex, UUtBool inMeleeImpact);

// collide with a character
UUtBool ONrCharacter_Collide_Ray(ONtCharacter *inCharacter, M3tPoint3D *inPoint, M3tVector3D *inDeltaPos, UUtBool inStopAtOneT,
											  UUtUns32 *outIndex, float *outT, M3tPoint3D *outHitPoint, M3tVector3D *outNormal);
UUtBool ONrCharacter_Collide_SphereRay(ONtCharacter *inCharacter, M3tPoint3D *inPoint, M3tVector3D *inDeltaPos, float inRadius,
											  UUtUns32 *outIndex, float *outT, M3tPoint3D *outHitPoint, M3tVector3D *outNormal);

void ONrCharacter_Defer_Script(const char *inScript, const char *inCharacterName, const char *inCause);
void ONrCharacter_Commit_Scripts(void);
UUtBool ONrCharacter_Lock_Scripts(void);
void ONrCharacter_Unlock_Scripts(void);

void ONrCharacter_Poison(ONtCharacter *ioCharacter, UUtUns32 inPoisonAmount, UUtUns32 inDamageFrames, UUtUns32 inInitialFrames);
void ONrCharacter_Pain(ONtCharacter *ioCharacter, UUtUns32 inDamage, UUtUns32 inMinimumPain);
UUtBool ONrCharacter_PlayHurtSound(ONtCharacter *ioCharacter, UUtUns32 inHurtSoundType, float *inVolume);
float ONrCharacter_GetHealthMultiplier(ONtCharacter *inCharacter);

UUtError ONrCharacterClass_ProcHandler(TMtTemplateProc_Message inMessage, void *inInstancePtr,
										void *inPrivateData);
UUtError ONrCharacterClass_MakeBodySurfaceCache(ONtCharacterClass *ioCharacterClass);

void ONrCharacter_ListBrokenSounds(BFtFile *inFile);
void ONrCharacter_NotifyTriggerVolumeDisable(UUtInt32 inID);

#endif // __ONI_CHARACTER_H__
