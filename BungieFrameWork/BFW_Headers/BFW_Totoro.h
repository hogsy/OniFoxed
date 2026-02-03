#pragma once

/*
	FILE:	BFW_Totoro.h

	AUTHOR:	Michael Evans

	CREATED: Sept 25, 1997

	PURPOSE:

	Copyright 1997, 1998

*/
#ifndef BFW_TOTORO_H
#define BFW_TOTORO_H

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_SoundSystem2.h"
#include "BFW_MathLib.h"

typedef UUtUns16 TRtAnimTime;
typedef UUtUns16 TRtAnimType;
typedef UUtUns16 TRtAnimState;
typedef UUtUns16 TRtAnimVarient;
typedef UUtUns8	 TRtAnimFlag;
typedef UUtUns8  TRtShorcutFlag;
typedef UUtUns8	 TRtAttackFlag;

typedef struct TRtAnimation	TRtAnimation;
typedef struct TRtAimingScreen TRtAimingScreen;
typedef struct TRtBodyTextures TRtBodyTextures;
typedef struct TRtAnimationCollection TRtAnimationCollection;
typedef struct TRtAnimationIdentifier TRtAnimationIdentifier;
typedef struct TRtScreenCollection TRtScreenCollection;
typedef struct TRtAnimationCollectionPart TRtAnimationCollectionPart;

#define TRcAnimState_None		((TRtAnimState) 0)
#define TRcAnimState_Anything	((TRtAnimState) 1)

#define TRcAnimType_None		((TRtAnimType) 0)
#define TRcAnimType_Anything	((TRtAnimType) 1)

enum
{
	TRcAnimFlag_Prepared,
	TRcAnimFlag_FirstPublic
};

#define TRcTemplate_Animation			UUm4CharToUns32('T', 'R', 'A', 'M')
#define TRcTemplate_BodySet				UUm4CharToUns32('T', 'R', 'B', 'S')
#define TRcTemplate_Body				UUm4CharToUns32('T', 'R', 'C', 'M')
#define TRcTemplate_BodyTextures		UUm4CharToUns32('T', 'R', 'M', 'A')
#define TRcTemplate_AnimationCollection	UUm4CharToUns32('T', 'R', 'A', 'C')
#define TRcTemplate_FacingTable			UUm4CharToUns32('T', 'R', 'F', 'T')
#define TRcTemplate_AimingScreen		UUm4CharToUns32('T', 'R', 'A', 'S')
#define TRcTemplate_ScreenCollection	UUm4CharToUns32('T', 'R', 'S', 'C')
#define TRcTemplate_GeometryArray		UUm4CharToUns32('T', 'R', 'G', 'A')
#define TRcTemplate_TranslationArray	UUm4CharToUns32('T', 'R', 'T', 'A')
#define TRcTemplate_IndexArray			UUm4CharToUns32('T', 'R', 'I', 'A')

UUtError
TRrInitialize(
	void);

void
TRrTerminate(
	void);

UUtError
TRrRegisterTemplates(
	void);

// directions that a move can be in
typedef enum TRtDirection {
	TRcDirection_None		= 0,
	TRcDirection_Forwards	= 1,
	TRcDirection_Back		= 2,
	TRcDirection_Left		= 3,
	TRcDirection_Right		= 4,
	TRcDirection_360		= 5,

	TRcDirection_Max		= 6
} TRtDirection;

extern const char *TRcDirectionName[];

//
// ================================ Body ================================
//

tm_template('T', 'R', 'M', 'A', "Texture Map Array")
TRtBodyTextures
{
	tm_pad						pad0[22];

	tm_varindex UUtUns16		numMaps;
	tm_vararray M3tTextureMap	*maps[1];
};

typedef tm_template('T', 'R', 'G', 'A', "Totoro Quaternion Body Geometry Array")
TRtGeometryArray
{
	tm_pad		pad0[22];

	tm_varindex UUtUns16		numGeometries;
	tm_vararray M3tGeometry		*geometries[1];
} TRtGeometryArray;

typedef tm_template('T', 'R', 'T', 'A', "Totoro Quaternion Body Translation Array")
TRtTranslationArray
{
	tm_pad		pad0[22];

	tm_varindex UUtUns16		numTranslations;
	tm_vararray M3tPoint3D		translations[1];
} TRtTranslationArray;

typedef tm_struct TRtIndexBlock
{
	UUtUns8 parent;
	UUtUns8 child;
	UUtUns8 sibling;
	UUtUns8	zero;
} TRtIndexBlock;


typedef tm_template('T', 'R', 'I', 'A', "Totoro Quaternion Body Index Array")
TRtIndexBlockArray
{
	tm_pad		pad0[22];

	tm_varindex UUtUns16		numIndexBlocks;
	tm_vararray TRtIndexBlock	indexBlocks[1];
} TRtIndexBlockArray;

// an entire translation, rotation Body
typedef tm_template('T', 'R', 'C', 'M', "Totoro Quaternion Body")
TRtBody
{
	UUtInt32	polygonCount;
	UUtUns16	numParts;
	tm_pad		pad0[2];

	char		debug_name[64];

	tm_raw(M3tGeometry **) geometries;
	tm_raw(M3tPoint3D *) translations;
	tm_raw(TRtIndexBlock *) indexBlocks;

	TRtGeometryArray *geometryStorage;
	TRtTranslationArray *translationStorage;
	TRtIndexBlockArray *indexBlockStorage;

} TRtBody;


#define	TRcBody_SuperLow	((TRtBodySelector) (0))
#define	TRcBody_Low			((TRtBodySelector) (TRcBody_SuperLow + 1))
#define	TRcBody_Medium		((TRtBodySelector) (TRcBody_Low + 1))
#define	TRcBody_High		((TRtBodySelector) (TRcBody_Medium + 1))
#define	TRcBody_SuperHigh	((TRtBodySelector) (TRcBody_High + 1))
#define	TRcBody_NumBodies	((TRtBodySelector) (TRcBody_SuperHigh + 1))
#define	TRcBody_NotDrawn	((TRtBodySelector) (TRcBody_NumBodies + 1))

typedef UUtInt32 TRtBodySelector;

typedef tm_template('T', 'R', 'B', 'S', "Totoro Body Set")
TRtBodySet
{

	TRtBody *body[5];
} TRtBodySet;

//
// ================================ Facing Table ================================
//

// an entire translation, rotation Body
typedef tm_template('T', 'R', 'F', 'T', "Totoro Facing Table")
TRtFacingTable
{
	tm_pad					pad0[22];

	tm_varindex	UUtUns16	tableSize;
	tm_vararray float		facing[1];
} TRtFacingTable;

//
// ================================ Extra Body ================================
//

typedef struct TRtExtraBodyPart
{
	M3tGeometry*	geometry;
	M3tQuaternion	quaternion;
	M3tPoint3D		translation;
	UUtUns16		index;
	M3tMatrix4x3		*matrix;
} TRtExtraBodyPart;

// an entire translation, rotation Body
typedef struct TRtExtraBody
{
	tm_varindex	UUtUns16			numParts;
	tm_vararray TRtExtraBodyPart	parts[1];
} TRtExtraBody;


//
// ================================ Quaternion Arrays ================================
//


extern M3tQuaternion TRgQuatArrayA[];
extern M3tQuaternion TRgQuatArrayB[];
extern M3tQuaternion TRgQuatArrayC[];
extern M3tQuaternion TRgQuatArrayD[];
extern M3tQuaternion TRgQuatArrayE[];

void TRrQuatArray_Zero(
		UUtUns16				inNumParts,
		M3tQuaternion			*quaternions);

void TRrQuatArray_SetAnimation(
		const TRtAnimation*		inAnimaton,
		TRtAnimTime				inTime,
		M3tQuaternion			*inDest);

void TRrQuatArray_Multiply(
		UUtUns16				inNumParts,
		const M3tQuaternion		*inSrc1,
		const M3tQuaternion		*inSrc2,
		M3tQuaternion			*inDest);

void TRrQuatArray_Lerp(
		UUtUns16				inNumParts,
		float					inAmt1,
		const M3tQuaternion		*inSrc1,
		const M3tQuaternion		*inSrc2,
		M3tQuaternion			*inDest);

void TRrQuatArray_Lerp4(
		UUtUns16				inNumParts,
		float					*inAmts,
		const M3tQuaternion		*inSrc1,
		const M3tQuaternion		*inSrc2,
		const M3tQuaternion		*inSrc3,
		const M3tQuaternion		*inSrc4,
		M3tQuaternion			*inDest);

void TRrQuatArray_Slerp(
		UUtUns16				inNumParts,
		float					inAmt1,
		const M3tQuaternion		*inSrc1,
		const M3tQuaternion		*inSrc2,
		M3tQuaternion			*inDest);

void TRrQuatArray_Move(
		UUtUns16				inNumParts,
		const M3tQuaternion		*inSrc,
		M3tQuaternion			*inDest);



//
// ================================ Animation ================================
//

#define TRcMaxAttacks			((UUtUns8) 2)

typedef tm_struct TRtAttack
{
	UUtUns32				damageBits;
	float					knockback;
	UUtUns32				flags;

	UUtUns16				damage;
	UUtUns16				firstDamageFrame;

	UUtUns16				lastDamageFrame;
	UUtUns16				damageAnimation;

	UUtUns16				hitStun;
	UUtUns16				blockStun;

	UUtUns16				stagger;		// locked in the stagger animation even if you block
	UUtUns16				precedence;

	UUtUns16				attackExtentIndex;	// index into attackExtents buffer
	UUtUns16				pad;
} TRtAttack;

#define TRcMaxParticles			((UUtUns8) 16)

typedef tm_struct TRtParticle
{
	UUtUns16				startFrame;
	UUtUns16				stopFrame;

	UUtUns16				partIndex;
	UUtUns16				pad;

	char					particleName[16];	// must match ONcCharacterParticleNameLength

} TRtParticle;

//
// ================================ Attack Extents ================================
//

#define TRcAngleGranularity					9.5875262183254543e-5f // S.S. (M3c2Pi / UUcMaxUns16)
#define TRcPositionGranularity				0.01f
#define TRcExtentRingSamples				36		// don't change this without changing TRtExtentRing->distance
#define TRcExtentSampleBracket				(30.0f * M3cDegToRad)

// location of an attack relative to the start of the animation
// height and distance in multiples of TRcPositionGranularity
// angle in multiples of TRcAngleGranularity
typedef tm_struct TRtAttackExtent
{
	UUtUns16	frame_index;
	UUtUns16	attack_angle;
	UUtUns16	attack_distance;
	UUtUns16	pad;
	UUtInt16	attack_minheight;
	UUtInt16	attack_maxheight;
} TRtAttackExtent;

// position of the character at a given point in an animation, relative
// to the start of the animation. all in multiples of TRcPositionGranularity.
typedef tm_struct TRtPosition
{
	UUtInt16	location_x;
	UUtInt16	location_y;
	UUtUns16	toe_to_head_height;
	UUtInt16	floor_to_toe_offset;
} TRtPosition;

// information about the position of an attack which is stored globally in the animation
// rather than at a specific frame.
typedef tm_struct TRtAttackBound
{
	UUtUns16	frame_index;
	UUtUns8		attack_index;
	UUtUns8		extent_index;

	M3tVector3D	position;		// X, Y and Z are deltas in Max from animation start. X and Y correspond to position
								// array, Z to height array
	float		attack_dist;
	float		attack_minheight;	// relative to Max location
	float		attack_maxheight;	// relative to Max location
	float		attack_angle;
} TRtAttackBound;

// crude info about where an animation extends to in space, measured from its starting location
typedef tm_struct TRtExtentRing
{
	float		max_distance;
	float		min_height;
	float		max_height;
	float		distance[36];	// must match TRcExtentRingSamples
} TRtExtentRing;

// structure that encapsulates info about an attack's extents (stored in a single structure for
// caching in the importer). completely invalid for non-attack animations.
typedef tm_struct TRtExtentInfo
{
	TRtExtentRing				attack_ring;

	TRtAttackBound				firstAttack;
	TRtAttackBound				maxAttack;

	UUtUns32					computed_attack_direction;	// only used if animation->moveDirection is TRcDirection_None

	UUtUns32					numAttackExtents;
	tm_raw(TRtAttackExtent *)	attackExtents;
} TRtExtentInfo;

//
// ================================ Animation Intersection ================================
//

typedef struct TRtCharacterIntersectPosition {
	// this structure contains info about a character's position
	// that can be saved and restored to the intersectstate.
	M3tPoint3D			location;
	float				facing;

	UUtBool				inAir;
	UUtBool				jumped;
	UUtBool				jumpKeyDown;
	UUtUns16			numFramesInAir;
	UUtUns16			expectedJumpFrames;
	M3tVector3D			airVelocity;

} TRtCharacterIntersectPosition;

typedef struct TRtCharacterIntersectState {
	const TRtAnimation				*animation;
	UUtUns16						animFrame;
	void							*character;

	UUtBool							dirty_position;
	TRtCharacterIntersectPosition	position;
	TRtCharacterIntersectPosition	original_position;

	float							collision_leafsphere_radius;

	// parameters that control the fall/jump
	float 							fallGravity;
	float 							jumpGravity;
	float							jumpAmount;
	float							maxVelocity;
	float							jumpAccelAmount;
	UUtUns16						framesFallGravity;
	UUtUns16						numJumpAccelFrames;

	// temporary values about the current state of this character
	M3tVector3D						offsetLocation;
	UUtUns16						currentAnimFrame;
	UUtUns16						currentNumFramesInAir;
	M3tVector3D						currentAirVelocity;
	UUtBool							currentJumped;

} TRtCharacterIntersectState;

typedef enum TRtIntersectRejection {
	TRcRejection_NoAttack		= 0,
	TRcRejection_Distance		= 1,
	TRcRejection_Angle			= 2,
	TRcRejection_AboveHead		= 3,
	TRcRejection_BelowFeet		= 4,
	TRcRejection_Behind			= 5,

	TRcRejection_Max			= 6
} TRtIntersectRejection;

extern const char *TRcIntersectRejectionName[];

// TEMPORARY DEBUGGING SETTINGS
#define DEBUG_SHOWINTERSECTIONS		1

#if DEBUG_SHOWINTERSECTIONS
extern UUtBool TRgDisplayIntersections;
extern UUtBool TRgStoreThisIntersection;
extern UUtUns32 gFramesSinceIntersection;

void TRrDisplayLastIntersection(void);
#endif

typedef struct TRtAnimIntersectContext {
	UUtBool							consider_attack;	// true indicates that this is an AI considering an attack
														// false indicates that this is an AI wanting to know if it
														// will get hit
	TRtCharacterIntersectState		attacker;
	TRtCharacterIntersectState		target;

	UUtBool				dirty_matrix;
	M3tMatrix4x3		original_location_matrix;	// target locationspace (x, z) to attacker animspace (x, z) relative
													// to attacker's current location
	M3tMatrix4x3		current_location_matrix;	// may be modified for attacker's current location in their animspace
													// (if !consider_attack), or to account for attacker autofacing, or
													// to account for attacker's required transitioning before attack may be
													// launched
	M3tVector3D			target_velocity_estimate;
	float				target_height_min;			// relative to target base location
	float				target_height_max;
	float				target_cylinder_radius;

	// output parameters
	TRtIntersectRejection			reject_reason;
	UUtUns32						frames_to_hit;
} TRtAnimIntersectContext;

//
// ================================ Function Prototypes ================================
//

const TRtAnimation *TRrAnimation_GetFromName(const char *inTemplateName);

void TRrAnimation_GetAttacks(const TRtAnimation *inAnimation, TRtAnimTime inTime, UUtUns8 *outNumAttacks, TRtAttack *outAttacks, UUtUns8 *outindices);

TRtAnimType TRrAnimation_GetDamageAnimation(const TRtAnimation *inAnimation, TRtAnimTime inTime, UUtUns32 inPartIndex);
UUtUns32 TRrAnimation_GetDamage(const TRtAnimation *inAnimation, TRtAnimTime inTime, UUtUns32 inPartIndex);
UUtUns32 TRrAnimation_GetMaximumDamage(const TRtAnimation *inAnimation);
UUtUns32 TRrAnimation_GetMaximumSelfDamage(const TRtAnimation *inAnimation);
float TRrAnimation_GetKnockback(const TRtAnimation *inAnimation, TRtAnimTime inTime, UUtUns32 inPartIndex);
UUtBool TRrAnimation_TestAttackFlag(const TRtAnimation *inAnimation, TRtAnimTime inTime, UUtUns32 inPartIndex, TRtAttackFlag inWhichFlag);

UUtBool TRrAnimation_GetBlur(const TRtAnimation *inAnimation, TRtAnimTime inTime, UUtUns32 *outParts, UUtUns8 *outQuantity, UUtUns8 *outAmount);

TRtAnimVarient	TRrAnimation_GetVarient(const TRtAnimation *inAnimation);
TRtAnimType		TRrAnimation_GetType(const TRtAnimation *inAnimation);
TRtAnimState	TRrAnimation_GetFrom(const TRtAnimation *inAnimation);
TRtAnimState	TRrAnimation_GetTo(const TRtAnimation *inAnimation);

// shortcut functions
UUtUns8	TRrAnimation_GetShortcutLength(const TRtAnimation *inAnimation, TRtAnimState fromState);
UUtBool	TRrAnimation_TestShortcutFlag(const TRtAnimation *inAnimation, TRtAnimState fromState, TRtShorcutFlag inWhichFlag);
UUtBool	TRrAnimation_IsShortcut(const TRtAnimation *inAnimation, TRtAnimState fromState);

TRtAnimTime	TRrAnimation_GetDuration(const TRtAnimation *inAnimation);
void		TRrAnimation_GetPosition(const TRtAnimation *inAnimation, TRtAnimTime inTime, M3tVector3D *outPosition);
UUtBool		TRrAnimation_IsSoundFrame(const TRtAnimation *inAnimation, TRtAnimTime inTime);
const char*	TRrAnimation_GetNewSoundForFrame(const TRtAnimation *inAnimation, TRtAnimTime inTime);
const char* TRrAnimation_GetSoundName(const TRtAnimation *inAnimation);
void		TRrAnimation_SetSoundName(TRtAnimation *inAnimation, const char *inSoundName, TRtAnimTime inFrame);
float		TRrAnimation_GetHeight(const TRtAnimation *inAnimation, TRtAnimTime inTime);
const char *TRrAnimation_GetName(const TRtAnimation *inAnimation);
UUtBool		TRrAnimation_TestFlag(const TRtAnimation *inAnimation, TRtAnimFlag inWhichFlag);
UUtUns16	TRrAnimation_GetHardPause(const TRtAnimation *inFromAnim, const TRtAnimation *inToAnim);
UUtUns16	TRrAnimation_GetSoftPause(const TRtAnimation *inFromAnim, const TRtAnimation *inToAnim);
UUtBool		TRrAnimation_IsAttack(const TRtAnimation *inAnimation);
float		TRrAnimation_GetFinalRotation(const TRtAnimation *inAnimation);
UUtBool		TRrAnimation_IsAtomic(const TRtAnimation *inAnimation, TRtAnimTime inTime);
UUtUns16	TRrAnimation_GetEndInterpolation(const TRtAnimation *inFromAnim, const TRtAnimation *inToAnim);
UUtUns16	TRrAnimation_GetMaxInterpolation(const TRtAnimation	*inNewAnim);
UUtBool		TRrAnimation_IsDirect(const TRtAnimation *inFromAnim, const TRtAnimation *inToAnim);
UUtUns16	TRrAnimation_GetSelfDamage(const TRtAnimation *inAnim, TRtAnimTime inTime);
float		TRrAnimation_GetThrowDistance(const TRtAnimation *inAnim);
TRtDirection TRrAnimation_GetDirection(const TRtAnimation *inAnim, UUtBool inCheckAttackDirection);
const TRtExtentInfo *TRrAnimation_GetExtentInfo(const TRtAnimation *inAnim);
TRtAnimTime TRrAnimation_GetFirstAttackFrame(const TRtAnimation *inAnim);
TRtAnimTime TRrAnimation_GetLastAttackFrame(const TRtAnimation *inAnim);
TRtPosition *TRrAnimation_GetPositionPoint(const TRtAnimation *inAnimation, TRtAnimTime inTime);
const char *TRrAnimation_GetAttackName(const TRtAnimation *inAnimation);
UUtBool		TRrAnimation_HasBlockStun(const TRtAnimation *inAnimation, UUtUns16 inAmount);
UUtBool		TRrAnimation_HasStagger(const TRtAnimation *inAnimation);
UUtUns16	TRrAnimation_GetActionFrame(const TRtAnimation *inAnimation);
UUtUns16	TRrAnimation_GetThrowType(const TRtAnimation *inAnimation);
UUtBool		TRrAnimation_IsInvulnerable(const TRtAnimation *inAnimation, TRtAnimTime inTime);
UUtUns16	TRrAnimation_GetVocalization(const TRtAnimation *inAnimation);

// particles
void TRrAnimation_GetParticles(const TRtAnimation *inAnimation, UUtUns8 *outNumParticles, TRtParticle **outParticles);
UUtUns16 TRrAnimation_GetActiveParticles(const TRtAnimation *inAnimation, TRtAnimTime inTime);

//
// ================================ Body Drawing ================================
//

#define TRcNoExtraBody ((TRtExtraBody*) 0)

void TRrBody_BuildMatricies(
	const TRtBody*		inBody,
	const TRtExtraBody*	inExtraBody,
	M3tQuaternion		*inQuaternions,
	const M3tMatrix4x3		*inMatrix,
	UUtUns32			inMask,
	M3tMatrix4x3*			outMatricies);

void TRrBody_SetMaps(
	TRtBody			*inBody,
	TRtBodyTextures	*inMaps);

#define TRcBody_DrawExtra ((UUtUns32) 0x80000000)
#define TRcBody_DrawAll ((UUtUns32) 0xffffffff)

void TRrBody_Draw(
	const TRtBody*			inBody,
	const TRtExtraBody*		inExtraBody,
	const M3tMatrix4x3*		inMatricies,
	UUtUns32				drawBits,
	UUtUns32				redBits,
	UUtUns32				greenBits,
	UUtUns32				whiteBits,
	UUtUns32				blueBits);

void TRrBody_DrawMagic(
	const TRtBody*			inBody,
	const M3tMatrix4x3*		inMatricies,
	M3tTextureMap*			inTexture,
	float					inScale,
	const UUtUns8 *			inPartAlpha);


//
// ================================ Animation Collections ================================
//

/*

  type rules

  1. must match inType

  state rules

  1. must match inState or have a shortcut from inState

  varient rules

  1. never include a varient bit that we don't want
  2. find the highest varient match i.e. largest (inVarient & animVarient)

 */

const TRtAnimation *TRrCollection_Lookup(
	const TRtAnimationCollection *inCollection,
	TRtAnimType inType,
	TRtAnimState inState,
	TRtAnimVarient inFlags);

void
TRrCollection_Lookup_Range(
		TRtAnimationCollection *inCollection,
		TRtAnimType inType,
		TRtAnimState inState,
		TRtAnimationCollectionPart **outFirst,
		UUtInt32 *outCount);

TRtAnimationCollection *TRrCollection_GetRecursive(
		TRtAnimationCollection *inCollection);

//
// === overlays
//

void TRrOverlay_Apply(
		const TRtAnimation *inAnimation,
		TRtAnimTime inTime,
		const M3tQuaternion *inQuaternions,
		M3tQuaternion *outQuaternion,
		UUtUns16 inNumParts);


//
// === aiming screens
//

void TRrAimingScreen_Apply(
	const TRtAimingScreen *inAimingScreen,
	float inDirection,
	float inElevation,
	const M3tQuaternion *inQuaternions,
	M3tQuaternion *outQuaternions,
	UUtInt16 inNumParts);

void TRrAimingScreen_Apply_Lerp(
	const TRtAimingScreen *inAimingScreen1,
	const TRtAimingScreen *inAimingScreen2,
	float inAmount2,
	float inDirection,
	float inElevation,
	const M3tQuaternion *inQuaternions,
	M3tQuaternion *outQuaternions,
	UUtInt16 inNumParts);

const TRtAimingScreen *TRrAimingScreen_Lookup(
	const TRtScreenCollection *inScreenCollection,
	TRtAnimType inType,
	TRtAnimVarient inVarient);

void TRrAimingScreen_Clip(
	const TRtAimingScreen *inAimingScreen,
	float *ioDirection,
	float *ioElevation);

const TRtAnimation *TRrAimingScreen_GetAnimation(
	const TRtAimingScreen *inAimingScreen);


//
// === animation intersection
//

// quickly check the bounds of an attack animation
UUtBool TRrCheckAnimationBounds(TRtAnimIntersectContext *inContext);

// precisely determine whether an attack animation will hit
UUtBool TRrIntersectAnimations(TRtAnimIntersectContext *inContext);

// check for footstep sounds
typedef enum TRtFootstepKind {
	TRcFootstep_None,
	TRcFootstep_Left,
	TRcFootstep_Right,
	TRcFootstep_Single

} TRtFootstepKind;

TRtFootstepKind TRrGetFootstep(const TRtAnimation *inAnimation, TRtAnimTime inFrame);

void TRrInstallConsoleVariables(void);


#endif /* BFW_TOTORO_H */
