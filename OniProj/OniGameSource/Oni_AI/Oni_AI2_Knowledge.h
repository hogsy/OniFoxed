#pragma once
#ifndef ONI_AI2_KNOWLEDGE_H
#define ONI_AI2_KNOWLEDGE_H

/*
	FILE:	 Oni_AI2_Knowledge.h

	AUTHOR:	 Chris Butcher

	CREATED: April 17, 2000

	PURPOSE: Knowledge base for Oni AI

	Copyright (c) 2000

*/

#include "Oni_AI2_Script.h"
#include "BFW_Particle3.h"

// ------------------------------------------------------------------------------------
// -- constants

#define AI2cSoundVolumeModifier_MeleeHit_Heavy	2.0f
#define AI2cSoundVolumeModifier_MeleeHit_Medium	1.5f
#define AI2cSoundVolumeModifier_MeleeHit_Light	1.0f

typedef enum AI2tSoundAllowType
{
	AI2cSoundAllow_All					= 0,
	AI2cSoundAllow_Shouting				= 1,
	AI2cSoundAllow_Gunfire				= 2,
	AI2cSoundAllow_None					= 3,

	AI2cSoundAllow_Max					= 4

} AI2tSoundAllowType;

// ------------------------------------------------------------------------------------
// -- knowledge entry definitions

typedef enum AI2tContactType
{
	AI2cContactType_Sound_Ignore		= 0,
	AI2cContactType_Sound_Interesting	= 1,
	AI2cContactType_Sound_Danger		= 2,
	AI2cContactType_Sound_Melee			= 3,
	AI2cContactType_Sound_Gunshot		= 4,
	AI2cContactType_Sight_Peripheral	= 5,
	AI2cContactType_Sight_Definite		= 6,
	AI2cContactType_Hit_Weapon			= 7,
	AI2cContactType_Hit_Melee			= 8,
	AI2cContactType_Alarm				= 9,
	// FIXME: 'obstructed' sight?
	AI2cContactType_Max					= 10
} AI2tContactType;

typedef enum AI2tContactStrength
{
	AI2cContactStrength_Dead			= 0,
	AI2cContactStrength_Forgotten		= 1,
	AI2cContactStrength_Weak			= 2,
	AI2cContactStrength_Strong			= 3,
	AI2cContactStrength_Definite		= 4,
	AI2cContactStrength_Max				= 5
} AI2tContactStrength;

typedef enum AI2tContactPriority
{
	AI2cContactPriority_Friendly		= 0,
	AI2cContactPriority_Hostile_NoThreat= 1,
	AI2cContactPriority_Hostile_Threat	= 2,
	AI2cContactPriority_ForceTarget		= 3
} AI2tContactPriority;

extern const char AI2cContactStrengthName[][16];

typedef struct AI2tKnowledgeEntry
{
	ONtCharacter *			owner;		// AI that owns this entry
	ONtCharacter *			enemy;		// character being tracked

	UUtUns32				last_notify_time;
	UUtUns32				last_time;
	M3tPoint3D				last_location;
	AI2tContactType			last_type;
	UUtUns32				last_user_data;

	AI2tContactStrength		strength;
	AI2tContactStrength		last_strength;			// equals strength at all times except when in
													// new-knowledge callback
	AI2tContactPriority		priority;

	UUtUns32				verify_traverse_id;
	UUtUns32				first_time;
	UUtUns32				degrade_time;
	UUtUns32				total_damage;
	UUtUns32				ignored_events;
	AI2tContactStrength		highest_strength;
	UUtBool					first_sighting;			// is this first contact of this strength?
	UUtBool					has_hurt_me;
	UUtBool					has_hurt_me_in_melee;
	UUtBool					has_been_hostile;		// if we're a non-combatant, just firing a gun, or waving it in our face does this
	UUtUns32				degrade_hostility_time;

	struct AI2tKnowledgeEntry *prevptr, *nextptr;	// doubly linked list, stored in order of prosecution
} AI2tKnowledgeEntry;

// ------------------------------------------------------------------------------------
// -- knowledge state definitions

typedef void (*AI2tKnowledgeNotifyCallback)(ONtCharacter *ioCharacter, AI2tKnowledgeEntry *inEntry,
											UUtBool inDegrade);

typedef struct AI2tKnowledgeState
{
	UUtUns32					vision_delay_timer;		// don't check vision until this runs down
	UUtUns32					num_contacts;
	AI2tKnowledgeEntry *		contacts;	// head of a linked list - first is current
	AI2tKnowledgeNotifyCallback	callback;

	// for DEBUGGING only - can be removed for release
	UUtUns32					verify_traverse_id;
} AI2tKnowledgeState;

// ------------------------------------------------------------------------------------
// -- dodge table definitions

enum {
	AI2cDodgeProjectileFlag_InUse			= (1 << 0),
	AI2cDodgeProjectileFlag_Dead			= (1 << 1)
};

// a damaging projectile flying through the environment
typedef struct AI2tDodgeProjectile {
	UUtUns32					flags;

	// location of the projectile
	M3tPoint3D					initial_position;
	M3tPoint3D					prev_position;
	M3tPoint3D					position;
	M3tVector3D					velocity;
	float						dodge_radius;
	float						alert_radius;

	// projectile information
	UUtUns32					particle_ref;
	P3tParticleClass *			particle_class;
	P3tParticle *				particle;
	UUtUns32					owner;

} AI2tDodgeProjectile;

enum {
	AI2cDodgeFiringSpreadFlag_InUse			= (1 << 0)
};

// a firing pattern from a weapon
typedef struct AI2tDodgeFiringSpread {
	UUtUns32					flags;

	// location of the pattern
	M3tPoint3D					position;
	M3tVector3D					unit_direction;
	float						length;
	float						radius_base;
	float						angle;

	// properties of the pattern
	struct WPtWeapon			*weapon;

} AI2tDodgeFiringSpread;


// ------------------------------------------------------------------------------------
// -- global routines

UUtError AI2rKnowledge_LevelBegin(void);

void AI2rKnowledge_LevelEnd(void);

void AI2rKnowledge_Update(void);

void AI2rKnowledge_Reset(void);

void AI2rKnowledge_ImmediatePost(UUtBool inImmediatePost);

void AI2rKnowledge_ThisCharacterWasDeleted(ONtCharacter *inCharacter);	// forget about seeing anyone being deleted

// ------------------------------------------------------------------------------------
// -- state-update routines

void AI2rKnowledgeState_Initialize(AI2tKnowledgeState *ioKnowledgeState);

// generate a sound that AIs can hear. volume = distance at which characters with acuity 1.0 will hear
void AI2rKnowledge_Sound(AI2tContactType inImportance, M3tPoint3D *inLocation, float inVolume,
						 ONtCharacter *inChar1, ONtCharacter *inChar2);						// char 2 may be NULL

void AI2rKnowledge_AlarmTripped(UUtUns8 inAlarmID, ONtCharacter *inCharacter);

void AI2rKnowledge_CharacterDeath(ONtCharacter *inDeadCharacter);

void AI2rKnowledge_HurtBy(ONtCharacter *ioCharacter, ONtCharacter *inAttacker,
							UUtUns32 inHitPoints, UUtBool inMelee);

UUtBool AI2rKnowledge_IsCombatSound(AI2tContactType inType);

UUtBool AI2rKnowledge_IsHurtEvent(AI2tContactType inType);

void AI2rKnowledge_Delete(ONtCharacter *ioCharacter);

AI2tKnowledgeEntry *AI2rKnowledge_FindEntry(ONtCharacter *inOwner, AI2tKnowledgeState *ioKnowledgeState,
													ONtCharacter *inContact);

UUtInt32 AI2rKnowledge_CompareFunc(AI2tKnowledgeEntry *inEntry1, AI2tKnowledgeEntry *inEntry2, UUtBool inSticky, ONtCharacter *inHostileCheck);

void AI2rKnowledge_CreateMagicAwareness(ONtCharacter *ioCharacter, ONtCharacter *inTarget,
										UUtBool inDisableNotify, UUtBool inForceTarget);

UUtBool AI2rKnowledge_CharacterCanBeSeen(ONtCharacter *ioCharacter);

// ------------------------------------------------------------------------------------
// -- dodge routines

// life-cycle management for a dodge projectile
AI2tDodgeProjectile *AI2rKnowledge_NewDodgeProjectile(UUtUns32 *outIndex);
AI2tDodgeProjectile *AI2rKnowledge_GetDodgeProjectile(UUtUns32 inIndex);
void AI2rKnowledge_DeleteDodgeProjectile(UUtUns32 inIndex);

// life-cycle management for a firing pattern
AI2tDodgeFiringSpread *AI2rKnowledge_NewDodgeFiringSpread(UUtUns32 *outIndex);
AI2tDodgeFiringSpread *AI2rKnowledge_GetDodgeFiringSpread(UUtUns32 inIndex);
void AI2rKnowledge_DeleteDodgeFiringSpread(UUtUns32 inIndex);

// set up a newly created dodge projectile
void AI2rKnowledge_SetupDodgeProjectile(AI2tDodgeProjectile *ioProjectile, P3tParticleClass *inClass, P3tParticle *inParticle);

// set up a newly created firing spread
void AI2rKnowledge_SetupDodgeFiringSpread(AI2tDodgeFiringSpread *ioSpread, struct WPtWeapon *ioWeapon);

// get nearby projectiles that must be dodged
struct AI2tManeuverState;
void AI2rKnowledge_FindNearbyProjectiles(ONtCharacter *ioCharacter, struct AI2tManeuverState *ioManeuverState);

// get nearby firing spreads that must be dodged
void AI2rKnowledge_FindNearbyFiringSpreads(ONtCharacter *ioCharacter, struct AI2tManeuverState *ioManeuverState);

// ------------------------------------------------------------------------------------
// -- debugging routines

// report in
void AI2rKnowledgeState_Report(ONtCharacter *ioCharacter, AI2tKnowledgeState *ioKnowledgeState,
								UUtBool inVerbose, AI2tReportFunction inFunction);

// force a character to forget about one or all of its contacts
void AI2rKnowledgeState_Forget(ONtCharacter *ioCharacter, AI2tKnowledgeState *ioKnowledgeState,
								ONtCharacter *inForget);

// display debugging information about projectile awareness
void AI2rKnowledge_DisplayProjectiles(void);

// display debugging information about firing spread awareness
void AI2rKnowledge_DisplayFiringSpreads(void);

// display debugging information about AI sounds
void AI2rKnowledge_DisplaySounds(void);

#if defined(DEBUGGING) && DEBUGGING
// check the knowledge base's validity
void AI2rKnowledge_Verify(void);
#else
#define AI2rKnowledge_Verify()
#endif

#endif  // ONI_AI2_KNOWLEDGE_H
