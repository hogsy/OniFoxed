#pragma once
/*	FILE:	Oni_Weapon.h

	AUTHOR:	Quinn Dunki

	CREATED: April 2 1999

	PURPOSE: control of weapons in ONI

	Copyright 1998

*/

#ifndef __ONI_WEAPON_H__
#define __ONI_WEAPON_H__

#include "BFW_Particle3.h"
#include "BFW_Physics.h"
#include "Oni_GameState.h"

#define WPcMaxWeapons 64
#define WPcMaxWeaponName 32	// Don't change without changing references below
#define WPcMaxKey 31		// Biggest legal key number

// the maximum number of items the character can carry
#define WPcMaxAmmoBallistic				6
#define WPcMaxAmmoEnergy				6
#define WPcMaxHypos						6

// the power of these items
#define WPcMaxShields				100
#define WPcMaxInvisibility			(60 * 30)	// duration in game ticks of the invisibility

#define WPcPowerupDefaultGlowSize	5.0f

// CB: definitions for new weapon code
#define WPcMaxParticleAttachments		16
//#define WPcMaxFireGroups				4


typedef struct WPtWeapon WPtWeapon;

typedef enum WPtWeaponClassFlags
{
	WPcWeaponClassFlag_None					= 0x00000000,
	WPcWeaponClassFlag_Big					= 0x00000001,
	WPcWeaponClassFlag_Unstorable			= 0x00000002,
	WPcWeaponClassFlag_Energy				= 0x00000004,
	WPcWeaponClassFlag_TwoHanded			= 0x00000008,
	WPcWeaponClassFlag_RecoilDirect			= 0x00000010,
	WPcWeaponClassFlag_Autofire				= 0x00000020,
	WPcWeaponClassFlag_ParticlesFound		= 0x00000040,		// zero when imported. set when particle class ptrs are found.
	WPcWeaponClassFlag_AIStun				= 0x00000080,
	WPcWeaponClassFlag_AIKnockdown			= 0x00000100,
	WPcWeaponClassFlag_AISingleShot			= 0x00000200,
	WPcWeaponClassFlag_HasAlternate			= 0x00000400,
	WPcWeaponClassFlag_Unreloadable			= 0x00000800,
	WPcWeaponClassFlag_WalkOnly				= 0x00001000,
	WPcWeaponClassFlag_RestartEachShot		= 0x00002000,
	WPcWeaponClassFlag_DontStopWhileFiring	= 0x00004000,
	WPcWeaponClassFlag_HasLaser				= 0x00008000,
	WPcWeaponClassFlag_NoScale				= 0x00010000,
	WPcWeaponClassFlag_HalfScale			= 0x00020000,
	WPcWeaponClassFlag_DontClipSight		= 0x00040000,
	WPcWeaponClassFlag_DontFadeOut			= 0x00080000,
	WPcWeaponClassFlag_UseAmmoContinuously	= 0x00100000
} WPtWeaponClassFlags;

typedef enum WPtWeaponSight
{
	WPcWeaponSight_None,
	WPcWeaponSight_Cursor,
	WPcWeaponSight_Tunnel
} WPtWeaponSight;

// CB: a P3 particle attachment point for a gun
typedef tm_struct WPtParticleAttachment
{
	// location and orientation
	M3tMatrix4x3				matrix;

	// particle that is attached
	char						classname[16];	// must match P3cStringVarSize + 1 in BFW_Particle3.h
	tm_raw(P3tParticleClass *)	classptr;		// set up at runtime

	// fire control
	UUtUns16					ammo_used;		// non-zero ammo indicates that it is a shooter
												// only one shooter will be pulsed every time the gun fires
	UUtUns16					chamber_time;	// frames before the weapon can fire again (only used for shooters)
	UUtUns16					cheat_chamber_time;// frames before the weapon can fire again (only used for shooters)
	UUtUns16					minfire_time;	// frames that the weapon must fire for
	UUtUns16					shooter_index;	// if a shooter: index from 0 to shooter_count
												// if not: attachment is pulsed only when the shooter with
												//          this index fires. -1 = pulse every shot.
	UUtUns16					start_delay;	// frames before this attachment is sent the start event
} WPtParticleAttachment;

typedef tm_struct WPtMarker
{
	// These are all relative to the gun's origin
	M3tPoint3D		position;
	M3tVector3D		vector;
} WPtMarker;

typedef tm_struct WPtRecoil
{
	float			base;
	float			max;
	float			factor;
	float			returnSpeed;
	float			firingReturnSpeed;
} WPtRecoil;

enum {
	WPcWeaponTargetingFlag_NoWildShots		= (1 << 0)
};

// AI information for aiming and shooting weapons
typedef tm_struct AI2tWeaponParameters
{
	UUtUns32				targeting_flags;

	M3tMatrix4x3			shooter_inversematrix;
	M3tVector3D				shooterdir_gunspace;
	M3tVector3D				shooter_perpoffset_gunspace;

	float					prediction_speed;
	float					burst_radius;
	float					aim_radius;
	float					detection_volume;	// volume of shots relative to running footsteps
	float					min_safe_distance;
	float					max_range;
	UUtUns16				max_startle_misses;
	UUtUns16				shootskill_index;	// index into ai2_behavior.combat_parameters.targeting_params.shooting_skill
												// only used for AI2tWeaponParameters stored in weapons, not in turrets
	UUtUns32				melee_assist_timer;	// for weapons that require combo shooting and melee
	float					ballistic_speed;
	float					ballistic_gravity;

	float					dodge_pattern_length;// firing pattern that AIs want to avoid
	float					dodge_pattern_base;
	float					dodge_pattern_angle;
} AI2tWeaponParameters;

typedef tm_struct WPtSprite
{
	M3tTextureMap	*texture;
	UUtUns32		shade;
	float			scale;
} WPtSprite;

typedef enum WPtScaleMode
{
	WPcScaleMode_Scale,
	WPcScaleMode_HalfScale,
	WPcScaleMode_NoScale
} WPtScaleMode;

void WPrDrawSprite(WPtSprite *inSprite, M3tPoint3D *inLocation, WPtScaleMode inScaleMode);

#define WPcTemplate_WeaponClass	UUm4CharToUns32('O', 'N', 'W', 'C')
typedef tm_template('O', 'N', 'W', 'C', "Oni Weapon Class")
WPtWeaponClass
{
	WPtMarker				sight;

	// maximum sight distance (effects everything)
	float					maximum_sight_distance;

	// these parameters control the laser
	UUtUns32				laser_color;

	// these parameters control the final cursor
	WPtSprite				cursor;
	WPtSprite				cursor_targeted;

	// tunnel parameters
	WPtSprite				tunnel;
	UUtUns32				tunnel_count;
	float					tunnel_spacing;

	M3tTextureMap			*icon;
	M3tTextureMap			*hud_empty;
	M3tTextureMap			*hud_fill;
	M3tGeometry *			geometry;

	char					name[32];		// Must match WPcMaxWeaponName above
	float					aimingSpeed;

	WPtRecoil				private_recoil_info;

	M3tVector3D				secondHandVector;
	M3tVector3D				secondHandUpVector;
	M3tPoint3D				secondHandPosition;

	UUtUns16				freeTime;
	UUtUns16				recoilAnimType;
	UUtUns16				reloadAnimType;
	UUtUns16				reloadTime;
	UUtUns16				max_ammo;
	UUtUns16				attachment_count;
	UUtUns16				shooter_count;
	UUtUns16				reload_delay_time;
	UUtUns16				stopChamberThreshold;
	UUtUns16				pad;
	UUtUns32				flags;

	// CB: AI targeting and detection properties (shared with turrets)
	AI2tWeaponParameters	ai_parameters;
	AI2tWeaponParameters	ai_parameters_alt;

	WPtParticleAttachment	attachment[16];		// must match WPcMaxParticleAttachments

	char					empty_soundname[32];// SScMaxNameLength
	tm_raw(SStImpulse *)	empty_sound;		// runtime only

	M3tTextureMap			*glow_texture;
	M3tTextureMap			*loaded_glow_texture;
	float					glow_scale[2];

	M3tPoint3D				pickup_offset;
	float					display_yoffset;
} WPtWeaponClass;

WPtWeaponClass *WPrWeaponClass_GetFromName(const char *inTemplateName);

typedef enum WPtWeaponFlags
{
	WPcWeaponFlag_None						= 0x0000,
	WPcWeaponFlag_InUse						= 0x0001,
	WPcWeaponFlag_HitGround					= 0x0002,		// Set if we've hit ground since last being dropped
	WPcWeaponFlag_Untouchable				= 0x0004,		// Can't be picked up by normal means
	WPcWeaponFlag_Racked					= 0x0008,		// Draw it as though it were in a rack
	WPcWeaponFlag_IsFiring					= 0x0010,
	WPcWeaponFlag_BackgroundFX				= 0x0020,
	WPcWeaponFlag_Nearby					= 0x0040,		// only set temporarily - don't depend on being persistent
	WPcWeaponFlag_Object					= 0x0080,
	WPcWeaponFlag_DelayRelease				= 0x0200,
	WPcWeaponFlag_ChamberDelay				= 0x0400,
	WPcWeaponFlag_InHand					= 0x0800,
	WPcWeaponFlag_HasParticles				= 0x1000,
	WPcWeaponFlag_NoWeaponCollision			= 0x2000,
	WPcWeaponFlag_DeferUpdate_StopFiring	= 0x4000,
	WPcWeaponFlag_DeferUpdate_GoInactive	= 0x8000

} WPtWeaponFlags;

typedef enum WPtDamageOwnerType
{
	WPcDamageOwner_None				= 0 << 24,
	WPcDamageOwner_CharacterWeapon	= 1 << 24,
	WPcDamageOwner_CharacterMelee	= 2 << 24,
	WPcDamageOwner_Turret			= 3 << 24,
	WPcDamageOwner_Environmental	= 4 << 24,
	WPcDamageOwner_Falling			= 5 << 24,
	WPcDamageOwner_ActOfGod			= 6 << 24
} WPtDamageOwnerType;

#define WPcDamageOwner_TypeMask       0xFF000000
#define WPcDamageOwner_IndexMask      0x00FFFFFF

#define WPcAllSlots 0xFFFF
#define WPcPrimarySlot 0
#define WPcMagicSlot 1
#define WPcNormalSlot 2
#define WPcMaxSlots 3

typedef struct WPtInventory
{
	WPtWeapon *weapons[WPcMaxSlots];

	UUtBool z_forced_holster;
	UUtBool z_forced_holster_is_magic;

	UUtUns16 ammo;
	UUtUns16 hypo;
	UUtUns16 cell;

	UUtUns16 drop_ammo;
	UUtUns16 drop_hypo;
	UUtUns16 drop_cell;

	UUtUns16 hypoRemaining;
	UUtUns16 hypoTimer;

	UUtBool drop_shield;
	UUtBool drop_invisibility;
	UUtBool has_lsi;
	UUtUns16 shieldDisplayAmount;
	UUtUns16 shieldRemaining;
	UUtUns16 invisibilityRemaining;
	UUtUns32 numInvisibleFrames;		// CB: this is used to determine whether the AI
										// can see a player

	UUtUns32 keys;
} WPtInventory;


#define WPcMaxPowerups 64

typedef tm_enum WPtPowerupType
{
	WPcPowerup_AmmoBallistic,
	WPcPowerup_AmmoEnergy,
	WPcPowerup_Hypo,
	WPcPowerup_ShieldBelt,
	WPcPowerup_Invisibility,
	WPcPowerup_LSI,

	WPcPowerup_NumTypes,

	WPcPowerup_None					= 0xFFFFFFFF

} WPtPowerupType;

extern const char *WPgPowerupName[];

enum {
	WPcPowerupFlag_Moving			= (1 << 0),
	WPcPowerupFlag_HitGround		= (1 << 1),
	WPcPowerupFlag_OnGround			= (1 << 2),
	WPcPowerupFlag_PlacedAsObject	= (1 << 3)
};

#define WPcPowerupNodeCount 12

typedef struct WPtPowerup
{
	UUtUns16 flags;		// None defined

	UUtUns16 amount;	// 1 for most; 0-WPcMaxShields for shield belts
	WPtPowerupType type;
	M3tPoint3D position;
	M3tTextureMap *glow_texture;
	float glow_size[2];
	M3tGeometry *geometry;
	UUtUns16 freeTime;
	UUtUns16 fadeTime;
	UUtUns16 physicsRemoveTime;
	UUtUns16 pad;

	float rotation;		// CB: randomly selected at creation,
						// or determined by powerup placement tool

	UUtUns32 birthDate;

	PHtPhysicsContext	*physics;		// NULL unless WPcPowerupFlag_Moving is set

	M3tPoint3D last_position; // S.S.
	UUtUns32 oct_tree_node_index[WPcPowerupNodeCount]; // S.S. length-prefixed list of oct-tree nodes

	PHtSphereTree sphereTree;	// CB: static storage of spheretree
} WPtPowerup;


UUtError WPrRegisterTemplates(
	void);

// CB: called if all particles get nuked by debugging code
void
WPrRecreateParticles(void);


typedef enum WPtPlacedWeapon {
	WPcPlacedWeapon,
	WPcNormalWeapon,
	WPcCheatWeapon
} WPtPlacedWeapon;

WPtWeapon *WPrNew(WPtWeaponClass *inClass, WPtPlacedWeapon inPlacedWeapon);
WPtWeapon *WPrSpawnAtFlag(WPtWeaponClass *inClass, ONtFlag *inFlag);
void WPrDelete(WPtWeapon *inWeapon);
void WPrUpdate(void);
void WPrDisplay(void);

UUtBool WPrDepressTrigger(WPtWeapon *ioWeapon, UUtBool inForced, UUtBool inAlternate, UUtBool *outNoAmmo);

void WPrReleaseTrigger(WPtWeapon *ioWeapon, UUtBool inForceImmediate);

void WPrStartBackgroundEffects(WPtWeapon *ioWeapon);
void WPrStopBackgroundEffects(WPtWeapon *ioWeapon);
void WPrStopChamberDelay(WPtWeapon *ioWeapon);

void WPrAssign(
	WPtWeapon *inWeapon,
	ONtCharacter *inOwner);
WPtWeapon *WPrFindBarabbasWeapon(void);
void WPrRelease(
	WPtWeapon *inWeapon,
	UUtBool inKnockAway);

UUtBool
WPrMustWaitToFire(
	WPtWeapon *inWeapon);

UUtBool
WPrIsFiring(
	WPtWeapon *inWeapon);

WPtWeaponClass*
WPrGetClass(
	const WPtWeapon			*inWeapon);

UUtUns16
WPrGetIndex(
	const WPtWeapon			*inWeapon);

M3tMatrix4x3*
WPrGetMatrix(
	WPtWeapon				*inWeapon);

void WPrGetMarker(
	WPtWeapon *inWeapon,
	WPtMarker *inMarker,
	M3tPoint3D *outPosition,
	M3tVector3D *outVelocity);

ONtCharacter*
WPrGetOwner(
	const WPtWeapon			*inWeapon);

void
WPrGetPosition(
	const WPtWeapon			*inWeapon,
	M3tPoint3D				*outPosition);

void
WPrGetPickupPosition(
	const WPtWeapon			*inWeapon,
	M3tPoint3D				*outPosition);

WPtWeapon*
WPrGetWeapon(
	UUtUns16				inIndex);

UUtBool
WPrInUse(
	const WPtWeapon			*inWeapon);

UUtBool WPrIsAutoFire(WPtWeapon *inWeapon);

UUtBool WPrHasAmmo(WPtWeapon *inWeapon);

void WPrNotifyReload(WPtWeapon *inWeapon);

void WPrSetMaxAmmo(WPtWeapon *inWeapon);

void WPrSetAmmo(WPtWeapon *ioWeapon, UUtUns32 inAmmo);

void
WPrSetAmmoPercentage(
	WPtWeapon			*ioWeapon,
	UUtUns16			inAmmoPercentage);

void
WPrSetFlags(
	WPtWeapon			*ioWeapon,
	UUtUns16			inFlags);

void
WPrSetPosition(
	WPtWeapon			*ioWeapon,
	const M3tPoint3D	*inPosition,
	float				inRotation);

UUtError WPrLevel_Begin(
	void);
void WPrLevel_End(
	void);

UUtUns16 WPrGetAmmo(
	WPtWeapon *inWeapon);

UUtUns16 WPrMaxAmmo(
	WPtWeapon *inWeapon);

M3tGeometry*
WPrGetGeometry(
	const WPtWeapon		*inWeapon);

WPtWeapon *WPrFind_Weapon_XZ_Squared(
	const M3tPoint3D *inLocation,
	float inMinY,
	float inMaxY,
	WPtWeapon *inWeapon,			// weapon to find or NULL to find any weapon
	float inRadiusSquared);

static UUcInline WPtWeapon *WPrFind_Weapon_XZ(
	const M3tPoint3D *inLocation,
	float inMinY,
	float inMaxY,
	WPtWeapon *inWeapon,			// weapon to find or NULL to find any weapon
	float inRadius)
{
	return WPrFind_Weapon_XZ_Squared(inLocation, inMinY, inMaxY, inWeapon, UUmSQR(inRadius));
}

// returns how much of a powerup a character could receive
UUtUns16 WPrPowerup_CouldReceive(ONtCharacter *inCharacter, WPtPowerupType inType, UUtUns16 inAmount);

WPtPowerup *WPrFind_Powerup_XZ_Squared(
	ONtCharacter *inCharacter,
	const M3tPoint3D *inLocation,
	float inMinY,
	float inMaxY,
	WPtPowerup *inPowerup,			// powerup to find or NULL to find any powerup
	float inRadiusSquared,
	UUtBool *outInventoryFull);


static UUcInline WPtPowerup *WPrFind_Powerup_XZ(
	ONtCharacter *inCharacter,
	const M3tPoint3D *inLocation,
	float inMinY,
	float inMaxY,
	WPtPowerup *inWeapon,			// powerup to find or NULL to find any powerup
	float inRadius,
	UUtBool *outInventoryFull)
{
	return WPrFind_Powerup_XZ_Squared(inCharacter, inLocation, inMinY, inMaxY, inWeapon, UUmSQR(inRadius), outInventoryFull);
}

void WPrSlots_DeleteAll(WPtInventory *inInventory);

void WPrSlots_DropAll(
	WPtInventory *inInventory,
	UUtBool inWantKnockAway,
	UUtBool inStoreOnly);

void WPrSlot_Drop(
	WPtInventory *inInventory,
	UUtUns16 inSlotNum,
	UUtBool inWantKnockAway,
	UUtBool inStoreOnly);

// make a character magically drop a weapon, even though the weapon
// might not have been theirs
void WPrMagicDrop(ONtCharacter *inCharacter, WPtWeapon *inWeapon,
				  UUtBool inWantKnockAway, UUtBool inPlaceInFrontOf);

void WPrInventory_Reset(
	struct ONtCharacterClass *inClass,
	WPtInventory *inInventory);

WPtWeapon *WPrSlot_FindLastStowed(
	WPtInventory *inInventory,
	UUtUns16 inExcludeSlot,
	UUtUns16 *outSlotNum);	// optional

void WPrSlot_Swap(
	WPtInventory *inInventory,
	UUtUns16 inSlotA,
	UUtUns16 inSlotB);

UUtBool WPrSlot_FindEmpty(
	WPtInventory *inInventory,
	UUtUns16 *outSlotNum,	// optional
	UUtBool inAllowMagic);

void WPrPowerup_Display(
	void);

WPtPowerup *WPrPowerup_Drop(
	WPtPowerupType inType,
	UUtUns16 inAmount,
	ONtCharacter *inDroppingCharacter,
	UUtBool inPlaceInFrontOf);

WPtPowerup *WPrPowerup_New(
	WPtPowerupType inType,
	UUtUns16 inAmount,
	const M3tPoint3D *inPosition,
	float inRotation);

WPtPowerup *WPrPowerup_SpawnAtFlag(
	WPtPowerupType inType,
	UUtUns16 inAmount,
	ONtFlag *inFlag);

void WPrPowerup_Delete(
	WPtPowerup *inPowerup);

// returns number of specified powerup that a character has
UUtUns32 WPrPowerup_Count(ONtCharacter *inCharacter, WPtPowerupType inType);

UUtBool WPrPowerup_Use(
	ONtCharacter *inCharacter,
	WPtPowerupType inType);

UUtUns16 WPrPowerup_Give(ONtCharacter *inCharacter, WPtPowerupType inType, UUtUns16 inAmount, UUtBool inIgnoreMaximums, UUtBool inPlaySound);
UUtUns16 WPrPowerup_DefaultAmount(WPtPowerupType inType);

void WPrInventory_Update(
	ONtCharacter *inCharacter);

UUtBool WPrTryReload(ONtCharacter *inCharacter, WPtWeapon *inWeapon, UUtBool *outWeaponFull, UUtBool *outNoAmmo);

UUtError WPrInitialize(
	void);

void
WPrTerminate(
	void);

void WPrPowerup_Update(
	void);

UUtError
WPrPowerup_SetType(
	WPtPowerup			*ioPowerup,
	WPtPowerupType		inPowerupType);

WPtPowerupType WPrAmmoType(
	WPtWeapon *inWeapon);

// damage a character. returns TRUE if a duplicate collision is detected.
UUtBool WPrHitChar(void *inHitChar, M3tPoint3D *inHitPoint, M3tVector3D *inHitDirection, M3tVector3D *inHitNormal,
				P3tEnumDamageType inDamageType, float inDamage, float inStunDmg, float inKnockback, UUtUns32 inOwner,
				P3tParticleReference inParticleRef, UUtBool inSelfImmune);

void WPrBlast(M3tPoint3D *inLocation, P3tEnumDamageType inDamageType, float inDamage, float inStunDmg, float inKnockback, float inRadius,
			  P3tEnumFalloff inFalloffType, UUtUns32 inOwner, void *inIgnoreChar, P3tParticleReference inParticleRef, UUtBool inSelfImmune);

UUtBool WPrInventory_HasKey(
	WPtInventory *inInventory,
	UUtUns16 inKeyNum);

void WPrInventory_AddKey(
	WPtInventory *inInventory,
	UUtUns16 inKeyNum);

void WPrInventory_RemoveKey(
	WPtInventory *inInventory,
	UUtUns16 inKeyNum);

void WPrInventory_GetKeyString(
	WPtInventory *inInventory,
	char *outString);

void WPrInventory_Weapon_Purge_Magic(
	WPtInventory *inInventory);

const WPtRecoil *WPrClass_GetRecoilInfo(
	const WPtWeaponClass *inClass);

void WPrRecoil_UserTo_Internal(
	const WPtRecoil *inUserRecoil,
	WPtRecoil *outInternalRecoil);

// send an event to all particles attached to a weapon
void WPrSendEvent(WPtWeapon *inWeapon, UUtUns16 inEvent, UUtUns16 inShooter);

/*
 * damage ownership routines
 */

static UUcInline WPtDamageOwnerType WPrOwner_GetType(UUtUns32 inOwner)
{
	return (WPtDamageOwnerType) (inOwner & WPcDamageOwner_TypeMask);
}

ONtCharacter *WPrOwner_GetCharacter(UUtUns32 inOwner);
struct OBJtOSD_Turret *WPrOwner_GetTurret(UUtUns32 inOwner);

UUtUns32 WPrOwner_MakeWeaponDamageFromCharacter(ONtCharacter *inCharacter);
UUtUns32 WPrOwner_MakeMeleeDamageFromCharacter(ONtCharacter *inCharacter);
UUtUns32 WPrOwner_MakeFromTurret(struct OBJtOSD_Turret *inTurret);

UUtBool WPrOwner_IsSelfDamage(UUtUns32 inDamageOwner, ONtCharacter *inCharacter);
UUtBool WPrParticleBelongsToSelf(UUtUns32 inParticleOwner, PHtPhysicsContext *inHitContext);
float WPrParticleInaccuracy(UUtUns32 inParticleOwner);

static UUcInline UUtBool WPrOwner_IsCharacter(UUtUns32 inOwner, void **outCharacter)
{
	UUtUns32 owner_type_mask = (inOwner & WPcDamageOwner_TypeMask);
	UUtBool owner_is_character_weapon = owner_type_mask == WPcDamageOwner_CharacterWeapon;
	UUtBool owner_is_character_melee = owner_type_mask == WPcDamageOwner_CharacterMelee;
	UUtBool owner_is_character = (owner_is_character_weapon || owner_is_character_melee);

	if (owner_is_character && (outCharacter != NULL)) {
		*outCharacter = WPrOwner_GetCharacter(inOwner);
	}

	return owner_is_character;
}

/*
 * look for nearby weapons - used by AI2
 */

// determine how far every weapon is from a point
UUtUns32 WPrLocateNearbyWeapons(const M3tPoint3D *inLocation, float inMaxRadius);

// traverses along the list of weapons set up by WPrLocateNearbyWeapons
WPtWeapon *WPrClosestNearbyWeapon(UUtUns32 *inIterator);

// return a distance calculated by WPrLocateNearbyWeapons
float WPrGetNearbyDistance(WPtWeapon *inWeapon);

UUtBool WPrCanBePickedUp(WPtWeapon *inWeapon);



float WPrGetChamberTimeScale( WPtWeapon *inWeapon );

void WPrUpdateSoundPointers(void);

UUtBool WPrOutOfAmmoSound(WPtWeapon *ioWeapon);

void WPrSetTimers(WPtWeapon *ioWeapon, UUtUns16 inFreeTime, UUtUns16 inFadeTime);

extern UUtInt32 WPgHypoStrength;
extern UUtInt32 WPgFadeTime;
extern UUtBool WPgDisableFade;
extern UUtBool WPgGatlingCheat;
extern UUtBool WPgRegenerationCheat;

#endif
