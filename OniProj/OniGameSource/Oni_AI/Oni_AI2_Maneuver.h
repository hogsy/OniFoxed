#pragma once
#ifndef ONI_AI2_MANEUVER_H
#define ONI_AI2_MANEUVER_H

/*
	FILE:	Oni_AI2_Maneuver.h
	
	AUTHOR:	Chris Butcher
	
	CREATED: May 02, 2000
	
	PURPOSE: Maneuvering code for Oni AI system
	
	Copyright (c) 2000

*/

// ------------------------------------------------------------------------------------
// -- constants

#define AI2cManeuver_MaxThwartedGuns	4

typedef enum AI2tPrimaryMovementType {
	AI2cPrimaryMovement_Hold		= 0,
	AI2cPrimaryMovement_Advance		= 1,
	AI2cPrimaryMovement_Retreat		= 2,
	AI2cPrimaryMovement_Gun			= 3,
	AI2cPrimaryMovement_FindAlarm	= 4,
	AI2cPrimaryMovement_Melee		= 5,
	AI2cPrimaryMovement_GetUp		= 6,

	AI2cPrimaryMovement_Max			= 7,
	AI2cPrimaryMovement_None		= (UUtUns32) -1
} AI2tPrimaryMovementType;

#define AI2cDodgeEntries_Max			8

extern const char *AI2cPrimaryMovementName[];

// ------------------------------------------------------------------------------------
// -- structure definitions

/*
 * dodge entries hold dangers that we wish to move away from
 */

enum {
	AI2cDodgeCauseType_None			= 0,
	AI2cDodgeCauseType_Projectile	= 1,
	AI2cDodgeCauseType_FiringSpread	= 2
};

typedef struct AI2tDodgeCause_Projectile {
	AI2tDodgeProjectile *	projectile;
} AI2tDodgeCause_Projectile;

typedef struct AI2tDodgeCause_FiringSpread {
	AI2tDodgeFiringSpread *	spread;
} AI2tDodgeCause_FiringSpread;

typedef struct AI2tDodgeEntry {
	UUtUns32				total_frames;
	UUtUns32				active_timer;
	UUtUns32				max_active_timer;
	float					weight;
	float					current_weight;
	float					avoid_angle;
	UUtBool					already_dodging;
	M3tVector3D				incoming_dir;
	M3tVector3D				danger_dir;
	M3tVector3D				dodge_dir;

	UUtUns32				cause_type;
	union {
		AI2tDodgeCause_Projectile	projectile;
		AI2tDodgeCause_FiringSpread	spread;
	} cause;
} AI2tDodgeEntry;

/*
 * current state storage
 */

typedef struct AI2tManeuverState {
	/*
	 * primary movement
	 */

	AI2tPrimaryMovementType			primary_movement;
	UUtUns32						viability_checked_bitmask;
	float							primary_movement_weights[AI2cPrimaryMovement_Max];
	float							primary_movement_modifier_angle;
	float							primary_movement_modifier_weight;

	// AI2cPrimaryMovement_Advance
	float							advance_mindist;	// dist at which we won't get any closer
	float							advance_maxdist;	// dist to start interpolating weights at
	UUtBool							advance_failed;

	// AI2cPrimaryMovement_Retreat
	float							retreat_mindist;	// dist to start interpolating weights at
	float							retreat_maxdist;	// dist beyond which we won't retreat

	// AI2cPrimaryMovement_Gun
	float							gun_search_minradius;
	float							gun_search_maxradius;
	UUtBool							gun_running_pickup;
	WPtWeapon *						gun_target;
	UUtUns32						gun_thwart_timer;
	UUtUns32						gun_possible_thwart_timer;
	UUtUns32						guns_num_thwarted;
	WPtWeapon *						guns_thwarted[AI2cManeuver_MaxThwartedGuns];

	// AI2cPrimaryMovement_GetUp
	AI2tMovementDirection			getup_direction;

	// AI2cPrimaryMovement_FindAlarm
	struct OBJtOSD_Console *		alarm_console;		// this may be NULL for 'find alarm', or a specific console
	struct ONtActionMarker *		found_alarm;		// output variable

	/*
	 * secondary movement
	 */

	UUtUns32						num_dodge_entries;
	AI2tDodgeEntry					dodge_entries[AI2cDodgeEntries_Max];

	float							dodge_max_weight;
	float							dodge_avoid_angle;
	float							dodge_accum_angle;

} AI2tManeuverState;


// ------------------------------------------------------------------------------------
// -- function prototypes

// clear our maneuver state
struct AI2tCombatState;
void AI2rManeuverState_Clear(ONtCharacter *ioCharacter, struct AI2tCombatState *ioCombatState);

// clear our primary movement weights
void AI2rManeuver_ClearPrimaryMovement(ONtCharacter *ioCharacter, struct AI2tCombatState *ioCombatState);

// decide our primary movement based on the weight table
void AI2rManeuver_DecidePrimaryMovement(ONtCharacter *ioCharacter, struct AI2tCombatState *ioCombatState);

// determine dodge-type secondary movement
void AI2rManeuver_FindDodge(ONtCharacter *ioCharacter, struct AI2tCombatState *ioCombatState);

// apply dodge-type secondary movement
void AI2rManeuver_ApplyDodge(ONtCharacter *ioCharacter, struct AI2tCombatState *ioCombatState);

// update a dodge-type maneuver table of projectiles
void AI2rManeuver_NotifyNearbyProjectile(ONtCharacter *ioCharacter, AI2tManeuverState *ioManeuverState,
										 AI2tDodgeProjectile *inProjectile, M3tVector3D *inIncomingDirection,
										 float inClosestDistance, M3tVector3D *inDirectionToContact);

// update a dodge-type maneuver table of firing spreads
void AI2rManeuver_NotifyFiringSpread(ONtCharacter *ioCharacter, AI2tManeuverState *ioManeuverState,
									 AI2tDodgeFiringSpread *inSpread, M3tVector3D *inIncomingDirection,
									 float inSpreadSizeAtThisRange, float inDistanceToCenter,
									 M3tVector3D *inDirectionToCenter);

// determine whether there is a blockage between us and some point that prevents us from moving straight there.
// intended for usage in situations where the pathfinding grid says there is a blockage... does not take into
// account any characters.
UUtBool AI2rManeuver_CheckBlockage(ONtCharacter *ioCharacter, M3tPoint3D *inTargetPoint);

// determine whether there are any characters between us and some point
UUtBool AI2rManeuver_CheckInterveningCharacters(ONtCharacter *ioCharacter, M3tPoint3D *inTargetPoint);

// try colliding with a spheretree against the environment		
UUtBool AI2rManeuver_TrySphereTreeCollision(PHtSphereTree *inSphereTree, M3tVector3D *inMovementVector);

// check to see if we can perform an escape move
const TRtAnimation *AI2rManeuver_TryEscapeMove(ONtCharacter *ioCharacter, AI2tMovementDirection inDirection, UUtBool inRequirePickup, M3tPoint3D *outPoint);

// check to see if we can pick up a gun with an escape move
UUtBool AI2rManeuver_TryRunningGunPickup(ONtCharacter *ioCharacter, struct AI2tCombatState *ioCombatState);

// ------------------------------------------------------------------------------------
// -- debugging commands



#endif  // ONI_AI2_MANEUVER_H
