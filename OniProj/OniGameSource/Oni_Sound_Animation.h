// ======================================================================
// Oni_Sound_Animation.h
// ======================================================================
#pragma once

#ifndef ONI_SOUND_ANIMATION_H
#define ONI_SOUND_ANIMATION_H

// ======================================================================
// include
// ======================================================================
#include "BFW.h"
#include "BFW_SoundSystem2.h"

#include "Oni_Sound2.h"
#include "Oni_Character.h"
#include "Oni_Character_Animation.h"

// ======================================================================
// defines
// ======================================================================
#define OScSABinaryDataClass			UUm4CharToUns32('S', 'A', 'B', 'D')
#define OScMaxAnimNameLength			32
#define OScDefaultVariant				"Any"

// ======================================================================
// enums
// ======================================================================
// anim type modifiers
enum
{
	OScModType_Any,
	OScModType_Crouch,
	OScModType_Jump,
	OScModType_HeavyDamage,
	OScModType_MediumDamage,
	OScModType_LightDamage,
	
	OScModType_NumTypes,
	
	OScModType_None					= 0xFFFFFFFF

};

// anim types
enum
{
	OScAnimType_Any,
	OScAnimType_Animation,		/* indicates use of anim_name in OStAnimationSound */

/*	OScAnimType_Aim, */
	OScAnimType_Block,
	OScAnimType_DrawWeapon,
	OScAnimType_Fall,
	OScAnimType_Fly,
	OScAnimType_GettingHit,
	OScAnimType_Holster,
	OScAnimType_Kick,
	OScAnimType_Knockdown,
	OScAnimType_Land,
	OScAnimType_Jump,
	OScAnimType_Pickup,
	OScAnimType_Punch,
/*	OScAnimType_Recoil, */
	OScAnimType_Reload_Pistol,
	OScAnimType_Reload_Rifle,
	OScAnimType_Reload_Stream,
	OScAnimType_Reload_Superball,
	OScAnimType_Reload_Vandegraf,
	OScAnimType_Reload_Scram_Cannon,
	OScAnimType_Reload_MercuryBow,
	OScAnimType_Reload_Screamer,
	OScAnimType_Run,
	OScAnimType_Slide,
	OScAnimType_Stand,
	OScAnimType_Startle,
/*	OScAnimType_Throw, */
/*	OScAnimType_Turn, */
	OScAnimType_Walk,
	OScAnimType_Powerup,
	OScAnimType_Roll,
	OScAnimType_FallingFlail,
	
	OScAnimType_NumTypes,
	
	OScAnimType_None				= 0xFFFFFFFF

};

// ======================================================================
// typedefs
// ======================================================================
typedef UUtUns32					OStModType;
typedef UUtUns32					OStAnimType;
typedef struct OStVariant			OStVariant;

typedef struct OStSoundAnimation
{
	OStAnimType				anim_type;
	OStModType				mod_type;
	SStImpulse				*impulse;
	UUtUns32				frame;
	TRtAnimation			*animation;
	char					anim_name[OScMaxAnimNameLength];
	char					impulse_name[SScMaxNameLength];
	
} OStSoundAnimation;

// ======================================================================
// prototypes
// ======================================================================
const char*
OSrAnimType_GetName(
	OStAnimType					inAnimType);

OStAnimType
OSrAnimType_GetByName(
	const char					*inAnimTypeName);
	
const char*
OSrModType_GetName(
	OStModType					inModType);
	
OStModType
OSrModType_GetByName(
	const char					*inModTypeName);
	
// ----------------------------------------------------------------------
void
OSrSoundAnimation_Play(
	const ONtCharacterVariant	*inCharacterVariant,
	const TRtAnimation			*inAnimation,
	const M3tPoint3D			*inPosition,
	const M3tVector3D			*inDirection,
	const M3tVector3D			*inVelocity);

// ----------------------------------------------------------------------
const char*
OSrVariant_GetName(
	const OStVariant			*inVariant);

UUtUns32
OSrVariant_GetNumSounds(
	const OStVariant			*inVariant);

UUtError
OSrVariant_SoundAnimation_Add(
	OStVariant					*ioVariant,
	OStAnimType					inAnimType,
	OStModType					inModType,
	const char					*inAnimationName, /* unused if inAnimType != OScAnimType_Animation */
	UUtUns32					inFrame,
	const char					*inImpulseName);

void
OSrVariant_SoundAnimation_DeleteByIndex(
	OStVariant					*ioVariant,
	UUtUns32					inIndex);
	
const OStSoundAnimation*
OSrVariant_SoundAnimation_GetByIndex(
	OStVariant					*inVariant,
	UUtUns32					inIndex);
	
// ----------------------------------------------------------------------
UUtUns32
OSrVariantList_GetNumVariants(
	void);
	
UUtError
OSrVariantList_Save(
	UUtBool						inAutoSave);
	
OStVariant*
OSrVariantList_Variant_GetByIndex(
	UUtUns32					inIndex);
	
OStVariant*
OSrVariantList_Variant_GetByName(
	const char					*inVariantName);
	
OStVariant*
OSrVariantList_Variant_GetByVariant(
	const ONtCharacterVariant	*inCharacterVariant);

void
OSrVariantList_UpdateImpulseName(
	const char					*inOldImpulseName,
	const char					*inNewImpulseName);
	
// ----------------------------------------------------------------------
UUtError
OSrSA_Initialize(
	void);

void
OSrSA_ListBrokenSounds(
	BFtFile						*inFile);
	
void
OSrSA_UpdatePointers(
	void);
	
// ======================================================================
#endif /* ONI_SOUND_ANIMATION_H */