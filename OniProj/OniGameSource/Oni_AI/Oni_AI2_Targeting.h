#pragma once
#ifndef ONI_AI2_TARGETING_H
#define ONI_AI2_TARGETING_H

/*
	FILE:	Oni_AI2_Targeting.h
	
	AUTHOR:	Chris Butcher
	
	CREATED: April 20, 1999
	
	PURPOSE: Targeting infrastructure for Oni AI system
	
	Copyright (c) 2000

*/

// ------------------------------------------------------------------------------------
// -- constants

#define TARGETING_MISS_DECAY		0

// max number of weapons in the game can never exceed this!
#define AI2cCombat_MaxWeapons		13				// DO NOT CHANGE without changing AI2tCombatParameters shooting_skill array

typedef enum AI2tCombatAimingState {
	AI2cCombatAiming_ByMovementMode	= 0,
	AI2cCombatAiming_ForceWeapon	= 1,
	AI2cCombatAiming_ForceLook		= 2,

	AI2cCombatAiming_Max			= 3
} AI2tCombatAimingState;

extern const char AI2cCombatAimingName[][16];

// ------------------------------------------------------------------------------------
// -- structure definitions

/*
 * targeting parameter definitions - stored in AI2tBehavior at the class level
 */

typedef enum AI2tTargetingOwnerType {
	AI2cTargetingOwnerType_Character	= 0,
	AI2cTargetingOwnerType_Turret		= 1
} AI2tTargetingOwnerType;

typedef struct AI2tTargetingOwner {
	AI2tTargetingOwnerType		type;
	union {
		ONtCharacter *			character;
		void *					turret;
	} owner;
} AI2tTargetingOwner;

// describes a character class's ability to shoot a particular weapon
typedef tm_struct AI2tShootingSkill 
{
	float						recoil_compensation;		// 0-1, 1 is no recoil
	float						best_aiming_angle;			// read as degrees, stored as sin(theta)
	float						shot_error;					// in % of distance... values of ~0-10 are useful
	float						shot_decay;					// 0 = shot errors go away totally for next shot,
															// 1 = shot errors are 100% cumulative (bad thing)
															// ~0.4-0.7 gives useful results
	float						gun_inaccuracy;				// multiplier for gun's inaccuracy
	UUtUns16					delay_min;
	UUtUns16					delay_max;					// timers for delay after firing
} AI2tShootingSkill;

typedef tm_struct AI2tTargetingParameters 
{
	// parameters for the initial deliberately-missed shots
//	float						startle_miss_angle_min;
//	float						startle_miss_angle_max;
//	float						startle_miss_decay;

	// parameters for how we shoot if startled
	float						startle_miss_angle;
	float						startle_miss_distance;

	// parameters for target prediction
	float						predict_amount;
	UUtUns32					predict_positiondelayframes;
	UUtUns32					predict_delayframes;
	UUtUns32					predict_velocityframes;
	UUtUns32					predict_trendframes;

} AI2tTargetingParameters;

/*
 * callbacks for targeting usage
 */

struct AI2tTargetingState;
typedef void (*AI2tTargetingCallback_Fire				)(struct AI2tTargetingState *inTargetingState, UUtBool inFire);
typedef void (*AI2tTargetingCallback_SetAimSpeed		)(struct AI2tTargetingState *inTargetingState, float inSpeed);
typedef void (*AI2tTargetingCallback_ForceAimWithWeapon	)(struct AI2tTargetingState *inTargetingState, UUtBool inWithWeapon);
typedef void (*AI2tTargetingCallback_EndForceAim		)(struct AI2tTargetingState *inTargetingState);
typedef void (*AI2tTargetingCallback_GetPosition		)(struct AI2tTargetingState *inTargetingState, M3tPoint3D *inPoint);
typedef void (*AI2tTargetingCallback_AimVector			)(struct AI2tTargetingState *inTargetingState, M3tVector3D *inDirection);
typedef void (*AI2tTargetingCallback_AimPoint			)(struct AI2tTargetingState *inTargetingState, M3tPoint3D *inPoint);
typedef void (*AI2tTargetingCallback_AimCharacter		)(struct AI2tTargetingState *inTargetingState, ONtCharacter *inTarget);
typedef void (*AI2tTargetingCallback_UpdateAiming		)(struct AI2tTargetingState *inTargetingState);

typedef struct AI2tTargetingCallbacks {
	AI2tTargetingCallback_Fire					rFire;
	AI2tTargetingCallback_SetAimSpeed			rSetAimSpeed;
	AI2tTargetingCallback_ForceAimWithWeapon	rForceAimWithWeapon;
	AI2tTargetingCallback_EndForceAim			rEndForceAim;
	AI2tTargetingCallback_GetPosition			rGetPosition;
	AI2tTargetingCallback_AimVector				rAimVector;
	AI2tTargetingCallback_AimPoint				rAimPoint;
	AI2tTargetingCallback_AimCharacter			rAimCharacter;
	AI2tTargetingCallback_UpdateAiming			rUpdateAiming;
} AI2tTargetingCallbacks;

/*
 * current state storage
 */

typedef struct AI2tTargetingState {

	/*
	 * global targeting variables
	 */

	AI2tTargetingOwner					owner;
	ONtCharacter						*target;
	AI2tTargetingParameters				*targeting_params;
	AI2tTargetingCallbacks				*callback;

	/*
	 * enemy prediction
	 */

	UUtBool								predict_position;
	float								current_enemy_speed;
	float								current_prediction_accuracy;
	UUtUns32							next_sample;
	UUtUns32							num_samples_taken;
	M3tPoint3D							*predictionbuf;			// this buffer is predict_trendframes long
	M3tVector3D							predicted_velocity;
	M3tVector3D							predicted_trend;
	M3tPoint3D							target_pt;
	UUtBool								valid_target_pt;

	/*
	 * inaccuracy
	 */

#if TARGETING_MISS_DECAY
	UUtUns32							miss_count;
	float								miss_angle;
	M3tVector3D							miss_axis;
	M3tMatrix4x3						miss_matrix;
#else
	UUtBool								miss_enable;
	float								miss_tolerance;
#endif
	float								error_distance;
	M3tVector3D							error_vector;

	/*
	 * targeting state
	 */

	UUtBool								returning_to_center;
	UUtUns32							thwarted_frames;
	UUtUns32							delay_frames;
	UUtUns32							minimumdist_frames;
	AI2tCombatAimingState				current_aim_state;

	M3tPoint3D							current_aim_pt;			// the point that we wish to shoot at
	M3tPoint3D							targeting_frompt;

	M3tPoint3D							weapon_pt;
	M3tVector3D							targeting_vector;		// where we aim
	M3tVector3D							desired_shooting_vector;// where we want our gun to point
	float								target_distance;
	float								aiming_distance;
	float								targeting_interp_val;	// used when the enemy is really close
	UUtBool								last_disallowed_firing;
	UUtBool								last_computation_success;
	UUtBool								last_computed_ontarget;
	float								last_computed_accuracy;
	float								last_computed_tolerance;

	/*
	 * weapon / shooting parameters
	 */

	AI2tWeaponParameters *				weapon_parameters;
	AI2tShootingSkill *					shooting_skill;
	AI2tShootingSkill *					shooting_skillarray;
	M3tMatrix4x3 *						weapon_matrix;

	/*
	 * ballistic firing solution
	 */

	UUtBool								firing_solution;
	float								solution_xvel;
	float								solution_yvel;
	float								solution_theta;

} AI2tTargetingState;


// ------------------------------------------------------------------------------------
// -- function prototypes

// set up targeting
void AI2rTargeting_Initialize(AI2tTargetingOwner inOwner, AI2tTargetingState *ioTargetingState,
								AI2tTargetingCallbacks *inCallbacks,
								AI2tWeaponParameters *inWeaponParams,
								M3tMatrix4x3 *inWeaponMatrix,
								AI2tTargetingParameters *inTargetingParams,
								AI2tShootingSkill *inShootingSkillArray);

// called by Oni_Character.c if a character actually fires its weapon
void AI2rNotifyFireSuccess(AI2tTargetingState *ioTargetingState);

// notify targeting to recalculate weapon parameters
void AI2rTargeting_ChangeWeapon(AI2tTargetingState *ioTargetingState, AI2tWeaponParameters *inWeaponParams,
								M3tMatrix4x3 *inWeaponMatrix);

// notify targeting that target has changed
void AI2rTargeting_SetupNewTarget(AI2tTargetingState *ioTargetingState, ONtCharacter *inTarget,
									UUtBool inInitialTarget);

// set up targeting for a game tick
void AI2rTargeting_Update(AI2tTargetingState *ioTargetingState, 
						  UUtBool inPredict, UUtBool inPointWeapon, UUtBool inShootWeapon,
						  float *outTooCloseWeight);

// act on our targeting (i.e. fire if on target)
void AI2rTargeting_Execute(AI2tTargetingState *ioTargetingState,
						   UUtBool inPointWeapon, UUtBool inShoot, UUtBool inHaveLOS);

// terminate targeting
void AI2rTargeting_Terminate(AI2tTargetingState *ioTargetingState);

// other systems can use this to aiming at a location
void ONrCreateTargetingVector(
	const M3tVector3D *current_aim_pt,						// point to aim the gun at, include prediction if required
	const M3tVector3D *targeting_frompt,					// characters location + height this frame
	const AI2tWeaponParameters *weapon_parameters,			// 
	const M3tMatrix4x3 *weapon_matrix,						// character's weapon_matrix
	M3tVector3D *targeting_vector);							// the final vector we should be aiming along


#endif  // ONI_AI2_TARGETING_H
