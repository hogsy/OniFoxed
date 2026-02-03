/*
	FILE:	Oni_AI2_Targeting.h

	AUTHOR:	Chris Butcher

	CREATED: April 20, 1999

	PURPOSE: Targeting infrastructure for Oni AI system

	Copyright (c) 2000

*/

#include "Oni_AI2.h"
#include "Oni_Character.h"

// ------------------------------------------------------------------------------------
// -- global constants and macros

#define AI_DEBUG_TARGETING									0

#define AI2cCombat_WeaponTargetingOffsetInterpolate_Min		10.0f
#define AI2cCombat_WeaponTargetingOffsetInterpolate_Max		25.0f
#define AI2cCombat_WeaponTargetingOffsetShooterReverse		8.0f

// interpolates from min_ratio to close_ratio between weapontargetingoffsetinterpolate_max and min
#define AI2cCombat_WeaponTargeting_MinAccuracyRatio			2.0f
#define AI2cCombat_WeaponTargeting_CloseAccuracyRatio		0.5f

// closer than this distance, we always fire our weapon
#define AI2cCombat_WeaponTargeting_AbsoluteMinimumRange		10.0f
#define AI2cCombat_WeaponTargeting_MinimumDistFireFrames	10

#define AI2cCombat_Targeting_CloseToleranceIncrease			3.0f
#define AI2cCombat_Targeting_AimingErrorScale				1.5f

#define AI2cCombat_Prediction_MinimumLeadSpeed				1.0f

#define AI2cCombat_Prediction_MinimumAccuracy				0.5f
#define AI2cCombat_Prediction_TotalAccuracy					0.9f

#define AI2cCombat_MissAngle_StopMissing					M3cDegToRad * 1.0f
#define AI2cCombat_Miss_MaximumVerticalAngle				M3cDegToRad * 8.0f
#define AI2cCombat_MinimumMissTolerance						10.0f

#define AI2cCombat_Targeting_MinThwartFrames				30
#define AI2cCombat_Targeting_DistPerThwartedFrame			0.1f

#define AI2cCombat_ShootingAimSpeed							0.5f
#define AI2cCombat_ShootingAimSpeed_MaxError				5.0f

const char AI2cCombatAimingName[AI2cCombatAiming_Max][16] =
	{"by-movement-mode", "force-weapon", "force-look"};

// ------------------------------------------------------------------------------------
// -- internal function prototypes

// prediction
static void AI2iTargeting_CalculateErrorVector(AI2tTargetingState *ioTargetingState);
#if TARGETING_MISS_DECAY
static void AI2iTargeting_NextMissDirection(AI2tTargetingState *ioTargetingState);
static void AI2iTargeting_CalculateMissDirection(AI2tTargetingState *ioTargetingState);
#endif
static void AI2iTargeting_UpdatePrediction(AI2tTargetingState *ioTargetingState, UUtUns32 inTime);

// targeting
static void AI2iTargeting_ApplyTargetingError(AI2tTargetingState *ioTargetingState, UUtBool inShootWeapon);
static UUtBool AI2iTargeting_Fire(AI2tTargetingState *ioTargetingState);
static void AI2iTargeting_UpdateTargeting(AI2tTargetingState *ioTargetingState, UUtBool inPointWeapon);
static void AI2iTargeting_DoDelay(AI2tTargetingState *ioTargetingState);


// ------------------------------------------------------------------------------------
// -- high-level combat manager control

// set up targeting
void AI2rTargeting_Initialize(AI2tTargetingOwner inOwner, AI2tTargetingState *ioTargetingState,
								AI2tTargetingCallbacks *inCallbacks,
								AI2tWeaponParameters *inWeaponParams,
								M3tMatrix4x3 *inWeaponMatrix,
								AI2tTargetingParameters *inTargetingParams,
								AI2tShootingSkill *inShootingSkillArray)
{
	ioTargetingState->owner = inOwner;
	ioTargetingState->targeting_params = inTargetingParams;
	ioTargetingState->shooting_skillarray = inShootingSkillArray;
	ioTargetingState->callback = inCallbacks;

	ioTargetingState->current_aim_state = AI2cCombatAiming_ByMovementMode;
	ioTargetingState->predictionbuf = NULL;
	ioTargetingState->valid_target_pt = UUcFalse;
	ioTargetingState->aiming_distance = 0.0f;
	ioTargetingState->target_distance = 0.0f;
	AI2rTargeting_ChangeWeapon(ioTargetingState, inWeaponParams, inWeaponMatrix);
}

// called every tick while in combat
void AI2rTargeting_Update(AI2tTargetingState *ioTargetingState, UUtBool inPredict, UUtBool inPointWeapon,
						  UUtBool inShootWeapon, float *outTooCloseWeight)
{
	ioTargetingState->last_computation_success = UUcFalse;
	ioTargetingState->predict_position = inPredict &&
				(ioTargetingState->weapon_parameters != NULL);

	if (ioTargetingState->target == NULL) {
		// our target is dead - remove any error.
		if (ioTargetingState->valid_target_pt) {
			MUmVector_Copy(ioTargetingState->current_aim_pt, ioTargetingState->target_pt);
		} else {
			// we have no target yet
			inPointWeapon = UUcFalse;
		}
	} else {
		// work out where to aim at (target_pt)
		AI2iTargeting_UpdatePrediction(ioTargetingState, ONrGameState_GetGameTime());

		// apply error as necessary to get current_aim_pt
		AI2iTargeting_ApplyTargetingError(ioTargetingState, inShootWeapon);
	}

	// update our aiming vector to stay on current_aim_pt
	AI2iTargeting_UpdateTargeting(ioTargetingState, inPointWeapon);

	if (outTooCloseWeight != NULL) {
		AI2tWeaponParameters *params = ioTargetingState->weapon_parameters;

		// work out if we are too close to the target to fire
		*outTooCloseWeight = 0;
		if ((params != NULL) && inPointWeapon) {
			if (ioTargetingState->target_distance < params->min_safe_distance) {
				*outTooCloseWeight = 1.0f - (ioTargetingState->target_distance / params->min_safe_distance);
			}
		}
	}
}

// called at the end of the combat update
void AI2rTargeting_Execute(AI2tTargetingState *ioTargetingState,
						   UUtBool inPointWeapon, UUtBool inShoot, UUtBool inHaveLOS)
{
	if ((ioTargetingState->weapon_parameters != NULL) && inPointWeapon && inShoot && inHaveLOS) {
		if (ioTargetingState->last_disallowed_firing) {
			// this is the first frame we have been trying to point / shoot / have LOS, etc. reset
			// our immediate-fire timer and burst-fire settings, so we don't just shoot in a random direction.
			ioTargetingState->returning_to_center = UUcTrue;
			ioTargetingState->minimumdist_frames = 0;
		}
		ioTargetingState->last_disallowed_firing = UUcFalse;
		ioTargetingState->last_computed_ontarget = AI2iTargeting_Fire(ioTargetingState);
	} else {
		ioTargetingState->last_disallowed_firing = UUcTrue;
		ioTargetingState->last_computed_ontarget = UUcFalse;
	}

	// tell our owner to fire if necessary
	if (ioTargetingState->callback->rFire) {
		ioTargetingState->callback->rFire(ioTargetingState, ioTargetingState->last_computed_ontarget);
	}
}

// terminate targeting
void AI2rTargeting_Terminate(AI2tTargetingState *ioTargetingState)
{
	// return to normal aiming speed
	if (ioTargetingState->callback->rSetAimSpeed) {
		ioTargetingState->callback->rSetAimSpeed(ioTargetingState, 1.0f);
	}

	// if we had forced aiming of a particular type, revert back to normal
	if ((ioTargetingState->current_aim_state != AI2cCombatAiming_ByMovementMode) &&
		(ioTargetingState->callback->rEndForceAim)) {
		ioTargetingState->callback->rEndForceAim(ioTargetingState);
	}

	// deallocate our prediction buffer if necessary
	if (ioTargetingState->predictionbuf != NULL) {
		UUrMemory_Block_Delete(ioTargetingState->predictionbuf);
		ioTargetingState->predictionbuf = NULL;
	}
}


void AI2rTargeting_SetupNewTarget(AI2tTargetingState *ioTargetingState, ONtCharacter *inTarget,
									UUtBool inInitialTarget)
{
	ioTargetingState->target = inTarget;

	// set up parameters here for the new target
	ioTargetingState->aiming_distance = 0.0f;
	ioTargetingState->target_distance = 0.0f;
	ioTargetingState->returning_to_center = UUcTrue;
	ioTargetingState->thwarted_frames = 0;
	ioTargetingState->delay_frames = 0;
	ioTargetingState->minimumdist_frames = 0;
	ioTargetingState->error_distance = 0;
	ioTargetingState->last_disallowed_firing = UUcTrue;
	MUmVector_Set(ioTargetingState->error_vector, 0, 0, 0);

#if TARGETING_MISS_DECAY
	// set up missing - if there is any, it will be modified down
	// by weapon max in calculatemissdirection
	ioTargetingState->miss_count = (inInitialTarget) ? 100 : 0;
#else
	if (ioTargetingState->target != NULL) {
		// work out where the target is, and our distance to them
		MUmVector_Copy(ioTargetingState->target_pt, ioTargetingState->target->location);
		ioTargetingState->target_pt.y += ioTargetingState->target->heightThisFrame;

		ioTargetingState->callback->rGetPosition(ioTargetingState, &ioTargetingState->targeting_frompt);
		ioTargetingState->target_distance = MUrPoint_Distance(&ioTargetingState->targeting_frompt, &ioTargetingState->target_pt);
		ioTargetingState->aiming_distance = ioTargetingState->target_distance;
		ioTargetingState->valid_target_pt = UUcTrue;

		MUmAssertVector(ioTargetingState->target_pt, 10000.0f);
		UUmAssert((ioTargetingState->target_distance >= 0) && (ioTargetingState->target_distance < 10000.0f));
		UUmAssert((ioTargetingState->aiming_distance >= 0) && (ioTargetingState->aiming_distance < 10000.0f));

		// by default don't initially miss
//		COrConsole_Printf_Color(UUcTrue, 0xFF8080FF, 0xFF303060, "### setup: no miss");
		ioTargetingState->miss_enable = UUcFalse;
		ioTargetingState->miss_tolerance = 0;
		if ((inInitialTarget) && (ioTargetingState->weapon_parameters != NULL) &&
			((ioTargetingState->weapon_parameters->targeting_flags & WPcWeaponTargetingFlag_NoWildShots) == 0)) {
			// we have a weapon and this is our first target, we initially miss
			ioTargetingState->miss_enable = UUcTrue;

			ioTargetingState->miss_tolerance = ioTargetingState->target_distance * ioTargetingState->targeting_params->startle_miss_angle;
	//		COrConsole_Printf_Color(UUcTrue, 0xFF8080FF, 0xFF303060, "### setup: dist %f ang %f -> tolerance %f",
	//				target_dist, ioTargetingState->targeting_params->startle_miss_angle, ioTargetingState->miss_tolerance);

			UUmAssert(ioTargetingState->miss_tolerance >= 0);
			ioTargetingState->miss_tolerance = UUmMin(ioTargetingState->miss_tolerance,
													ioTargetingState->targeting_params->startle_miss_distance);
	//		COrConsole_Printf_Color(UUcTrue, 0xFF8080FF, 0xFF303060, "### setup: final miss tolerance %f",
	//				ioTargetingState->miss_tolerance);
		}
	} else {
		// we have no target, don't miss
		ioTargetingState->miss_enable = UUcFalse;
		ioTargetingState->miss_tolerance = 0;
	}

#endif

#if TARGETING_MISS_DECAY
	if (ioTargetingState->miss_count) {
		// calculate our initial miss direction
		AI2iTargeting_CalculateMissDirection(ioTargetingState);

		// we aim at full speed because we're missing
		if (ioTargetingState->callback->rSetAimSpeed) {
			ioTargetingState->callback->rSetAimSpeed(ioTargetingState, 1.0f);
		}
	} else {
		// aim at reduced speed because we're not missing
		if (ioTargetingState->callback->rSetAimSpeed) {
			ioTargetingState->callback->rSetAimSpeed(ioTargetingState, AI2cCombat_ShootingAimSpeed);
		}
	}
#else
	if (ioTargetingState->callback->rSetAimSpeed) {
		ioTargetingState->callback->rSetAimSpeed(ioTargetingState, AI2cCombat_ShootingAimSpeed);
	}
#endif
}

// ------------------------------------------------------------------------------------
// -- prediction

static void AI2iTargeting_CalculateErrorVector(AI2tTargetingState *ioTargetingState)
{
	float inaccuracy, shot_decay;

	UUmAssert((ioTargetingState->target_distance >= 0) && (ioTargetingState->target_distance < 10000.0f));

	// what is our inaccuracy per shot? (shot_error is stored as a %age)
	inaccuracy = ioTargetingState->shooting_skill->shot_error * ioTargetingState->target_distance / 100.0f;

	// reduce the current error vector by a scale factor back towards the target
	shot_decay = UUmPin(ioTargetingState->shooting_skill->shot_decay, 0.0f, 1.0f);
	MUmVector_Scale(ioTargetingState->error_vector, shot_decay);

	// compute the maximum distance that the error vector could possibly get away from zero
	ioTargetingState->error_distance = inaccuracy * (1.0f - shot_decay);

	// add a random offset to the error vector
	ioTargetingState->error_vector.x += UUmRandomRangeFloat(-inaccuracy, +inaccuracy);
	ioTargetingState->error_vector.y += UUmRandomRangeFloat(-inaccuracy, +inaccuracy);
	ioTargetingState->error_vector.z += UUmRandomRangeFloat(-inaccuracy, +inaccuracy);
}

#if TARGETING_MISS_DECAY
static void AI2iTargeting_NextMissDirection(AI2tTargetingState *ioTargetingState)
{
	M3tQuaternion miss_quaternion;

	if (ioTargetingState->miss_count == 0)
		return;

	if (--ioTargetingState->miss_count) {
		// reduce our miss threshold for subsequent shots.
		ioTargetingState->miss_angle *= ioTargetingState->targeting_params->startle_miss_decay;

#if AI_DEBUG_TARGETING
		COrConsole_Printf("reducing miss-rotation to %f deg [%d shots remain]",
					ioTargetingState->miss_angle * M3cRadToDeg, ioTargetingState->miss_count);
#endif
		if (ioTargetingState->miss_angle < AI2cCombat_MissAngle_StopMissing) {
#if AI_DEBUG_TARGETING
			COrConsole_Printf("miss-rotation below threshold of %f deg, stop",
								ioTargetingState->miss_angle * M3cRadToDeg);
#endif
			ioTargetingState->miss_count = 0;
		} else {
			// calculate the new miss rotation matrix
			MUrQuat_SetValue(&miss_quaternion,
				ioTargetingState->miss_axis.x,
				ioTargetingState->miss_axis.y,
				ioTargetingState->miss_axis.z,
				ioTargetingState->miss_angle);
			MUrQuatToMatrix(&miss_quaternion, &ioTargetingState->miss_matrix);
		}
	} else {
#if AI_DEBUG_TARGETING
		COrConsole_Printf("stop missing");
#endif
	}
}

static void AI2iTargeting_CalculateMissDirection(AI2tTargetingState *ioTargetingState)
{
	M3tVector3D target_vector, our_point, target_point, weapon_dir;
	M3tQuaternion miss_quaternion;
	float phi, dir_horiz, targetdir_horiz, target_phi, miss_angle, qx, qy, qz, delta_phi;

	if ((ioTargetingState->target == NULL) || (ioTargetingState->weapon_parameters == NULL)) {
		ioTargetingState->miss_count = 0;
	} else {
		ioTargetingState->miss_count = UUmMin(ioTargetingState->miss_count,
													ioTargetingState->weapon_parameters->max_startle_misses);
	}

	if (ioTargetingState->miss_count == 0) {
		// don't set up any deliberate misses
		ioTargetingState->miss_angle = 0;
		MUmVector_Set(ioTargetingState->miss_axis, 0, 0, 0);
		MUrMatrix_Identity(&ioTargetingState->miss_matrix);
		return;
	}

	// work out the distance to our target
	UUmAssert(ioTargetingState->callback->rGetPosition);
	ioTargetingState->callback->rGetPosition(ioTargetingState, &our_point);

	MUmVector_Copy(target_point, ioTargetingState->target->location);
	target_point.y += ioTargetingState->target->heightThisFrame;

	MUmVector_Subtract(target_vector, target_point, our_point);

	// work out where our weapon is currently pointing and get its angle from
	// the horizontal (phi). this is its local X axis if a character and Z axis if a turret (don't ask)
	if (ioTargetingState->owner.type == AI2cTargetingOwnerType_Character) {
		MUrMatrix_GetCol(ioTargetingState->weapon_matrix, 0, &weapon_dir);
	} else {
		UUmAssert(ioTargetingState->owner.type == AI2cTargetingOwnerType_Turret);
		MUrMatrix_GetCol(ioTargetingState->weapon_matrix, 2, &weapon_dir);
	}

	dir_horiz = MUrSqrt(UUmSQR(weapon_dir.x) + UUmSQR(weapon_dir.z));
	phi = MUrATan2(weapon_dir.y, dir_horiz);

	// we don't want to start shooting too far above or below the target... calculate their angle
	// from the horizontal too.
	targetdir_horiz = MUrSqrt(UUmSQR(target_vector.x) + UUmSQR(target_vector.z));
	target_phi = MUrATan2(target_vector.y, targetdir_horiz);

	// note that both angles are from -pi to +pi now
	delta_phi = phi - target_phi;

	// if the miss direction is outside the acceptable vertical range, then work out what an
	// acceptable value of delta_phi would be and construct a miss direction with the correct phi.
	// note that the magnitude of weapon_dir will change but this doesn't matter since
	// MUrQuat_SetFromTwoVectors doesn't use magnitude in its calculations.
	if (delta_phi < -AI2cCombat_Miss_MaximumVerticalAngle) {
		phi = target_phi - AI2cCombat_Miss_MaximumVerticalAngle;
		UUmTrig_ClipLow(phi);

		weapon_dir.y = dir_horiz * MUrTan(phi);

	} else if (delta_phi > AI2cCombat_Miss_MaximumVerticalAngle) {
		phi = target_phi + AI2cCombat_Miss_MaximumVerticalAngle;
		UUmTrig_ClipLow(phi);

		// reconstruct the miss direction from the new phi
		weapon_dir.y = dir_horiz * MUrTan(phi);
	}

	// what is the rotation from the where we *should* be aiming to the miss direction?
	MUrQuat_SetFromTwoVectors(&target_vector, &weapon_dir, &miss_quaternion);
	MUrQuat_GetValue(&miss_quaternion, &qx, &qy, &qz, &miss_angle);

	// miss angle must start between the two values specified in the character class
	miss_angle = UUmPin(miss_angle, ioTargetingState->targeting_params->startle_miss_angle_min,
						ioTargetingState->targeting_params->startle_miss_angle_max);

	// store the current miss quaternion and matrix
	ioTargetingState->miss_angle = miss_angle;
	MUmVector_Set(ioTargetingState->miss_axis, qx, qy, qz);
	MUrQuat_SetValue(&miss_quaternion, qx, qy, qz, miss_angle);
	MUrQuatToMatrix(&miss_quaternion, &ioTargetingState->miss_matrix);
}
#endif

static void AI2iTargeting_UpdatePrediction(AI2tTargetingState *ioTargetingState, UUtUns32 inTime)
{
	M3tVector3D *next_sample, enemy_velocity, trend_motion, sample_pt;
	UUtUns32 sample_base, sample_now, sample_trend, sample_vel;
	UUtUns32 bufsize, trend_frames, velocity_frames, delay_frames;
	float current_enemy_speed, current_trend_speed, dir_accuracy, speed_accuracy, prediction_accuracy, lead_magnitude;
	UUtBool can_predict;

	// if our target is dead, this will be handled by AI2iCombat_CheckTarget
	UUmAssert(ioTargetingState->target != NULL);

	// the size of the prediction buffer is determined by the maximum number of frames we need
	// to ever look ahead (which is the amount to predict trends)
	bufsize = ioTargetingState->targeting_params->predict_trendframes;

	MUmVector_Copy(sample_pt, ioTargetingState->target->location);
	sample_pt.y += ioTargetingState->target->heightThisFrame;

	if (ioTargetingState->predict_position && (ioTargetingState->predictionbuf != NULL)) {
		UUmAssert(ioTargetingState->weapon_parameters != NULL);
		can_predict = UUcTrue;

		trend_frames = ioTargetingState->targeting_params->predict_trendframes;
		velocity_frames = ioTargetingState->targeting_params->predict_velocityframes;
		delay_frames = ioTargetingState->targeting_params->predict_delayframes;

		// work out which indices we're using to build the current motion estimate
		sample_trend = ioTargetingState->next_sample + UUmMin(ioTargetingState->num_samples_taken, trend_frames);
		sample_vel   = ioTargetingState->next_sample + UUmMin(ioTargetingState->num_samples_taken, velocity_frames);
		sample_now   = ioTargetingState->next_sample + UUmMin(ioTargetingState->num_samples_taken, delay_frames);

		// the buffer is trend_frames long
		sample_trend %= bufsize;
		sample_vel   %= bufsize;
		sample_now   %= bufsize;

		if ((ioTargetingState->targeting_params->predict_positiondelayframes > 0) && (ioTargetingState->num_samples_taken > 0)) {
			// we predict velocity starting from where they were some number of frames in the past
			sample_base = ioTargetingState->next_sample + UUmMin(ioTargetingState->num_samples_taken, ioTargetingState->targeting_params->predict_positiondelayframes);
			sample_base %= bufsize;
			ioTargetingState->target_pt = ioTargetingState->predictionbuf[sample_base];
		} else {
			// we predict velocity starting from the target's current location
			ioTargetingState->target_pt = sample_pt;
		}
		ioTargetingState->target_distance = MUrPoint_Distance(&ioTargetingState->target_pt, &ioTargetingState->targeting_frompt);

		if (sample_vel == sample_now) {
			// no samples yet!
			can_predict = UUcFalse;
			MUmVector_Set(ioTargetingState->predicted_velocity, 0, 0, 0);
		} else {
			// estimate the enemy's velocity in units per second by averaging their motion over this time period
			MUmVector_Subtract(enemy_velocity, ioTargetingState->predictionbuf[sample_now],
												ioTargetingState->predictionbuf[sample_vel]);
			MUmVector_Scale(enemy_velocity, ((float) UUcFramesPerSecond) / (velocity_frames - delay_frames));

			current_enemy_speed = MUmVector_GetLength(enemy_velocity);

			MUmVector_Copy(ioTargetingState->predicted_velocity, enemy_velocity);
			ioTargetingState->current_enemy_speed = current_enemy_speed;

			if (current_enemy_speed < AI2cCombat_Prediction_MinimumLeadSpeed) {
				can_predict = UUcFalse;
			}
		}

		if (sample_trend == sample_now) {
			// no samples yet!
			can_predict = UUcFalse;
			MUmVector_Set(ioTargetingState->predicted_trend, 0, 0, 0);
		} else {
			// estimate the enemy's velocity in units per second by averaging their motion over this time period
			MUmVector_Subtract(trend_motion, ioTargetingState->predictionbuf[sample_now],
											ioTargetingState->predictionbuf[sample_trend]);
			MUmVector_Scale(trend_motion, ((float) UUcFramesPerSecond) / (trend_frames - delay_frames));

			MUmVector_Copy(ioTargetingState->predicted_trend, trend_motion);
			current_trend_speed = MUmVector_GetLength(trend_motion);

			if (current_enemy_speed < AI2cCombat_Prediction_MinimumLeadSpeed) {
				can_predict = UUcFalse;
			}
		}

		// sample the new enemy location
		next_sample = ioTargetingState->predictionbuf + ioTargetingState->next_sample;
		*next_sample = sample_pt;

		ioTargetingState->num_samples_taken++;
		if (ioTargetingState->next_sample == 0) {
			ioTargetingState->next_sample = trend_frames - 1;
		} else {
			ioTargetingState->next_sample--;
		}

		if (can_predict) {
			// determine how well our current leading velocity matches the character's general trend of movement
			// recently.

			// both vectors should be pointing in the same direction
			dir_accuracy = MUrVector_DotProduct(&enemy_velocity, &trend_motion) / (current_enemy_speed * current_trend_speed);

			// if the character's trend velocity is only a fraction of their current speed, they are likely
			// juking about in place - don't lead them as much.
			speed_accuracy = current_trend_speed / current_enemy_speed;

			prediction_accuracy = UUmMin(dir_accuracy, speed_accuracy);

			if (prediction_accuracy > AI2cCombat_Prediction_MinimumAccuracy) {
				// don't predict by the full velocity unless we're sure the character is basically running in a
				// straight line
				if (prediction_accuracy > AI2cCombat_Prediction_TotalAccuracy) {
					prediction_accuracy = 1.0f;
				} else {
					prediction_accuracy = (prediction_accuracy - AI2cCombat_Prediction_MinimumAccuracy)
										/ (AI2cCombat_Prediction_TotalAccuracy - AI2cCombat_Prediction_MinimumAccuracy);
				}
				UUmAssert((prediction_accuracy >= 0.0f) && (prediction_accuracy <= 1.0f));
				ioTargetingState->current_prediction_accuracy = prediction_accuracy;

				// work out how far this predicted velocity means that we have to lead the enemy
				lead_magnitude = ioTargetingState->targeting_params->predict_amount *
					prediction_accuracy * ioTargetingState->target_distance /
					ioTargetingState->weapon_parameters->prediction_speed;

				lead_magnitude += ((float) ioTargetingState->targeting_params->predict_positiondelayframes) / UUcFramesPerSecond;

				MUmVector_ScaleIncrement(ioTargetingState->target_pt, lead_magnitude, enemy_velocity);
			} else {
				// don't predict at all, leave target_pt unchanged
				ioTargetingState->current_prediction_accuracy = 0.f;
			}
		}
	} else {
		MUmVector_Copy(ioTargetingState->target_pt, sample_pt);
		ioTargetingState->target_distance = MUrPoint_Distance(&ioTargetingState->target_pt, &ioTargetingState->targeting_frompt);
	}

	MUmAssertVector(ioTargetingState->target_pt, 10000.0f);
	ioTargetingState->valid_target_pt = UUcTrue;
}

// ------------------------------------------------------------------------------------
// -- targeting

// called by Oni_Character.c if a character actually fires its weapon
void AI2rNotifyFireSuccess(AI2tTargetingState *ioTargetingState)
{
	UUmAssertReadPtr(ioTargetingState, sizeof(AI2tTargetingState));

#if TARGETING_MISS_DECAY
	AI2iTargeting_NextMissDirection(ioTargetingState);
#endif
	AI2iTargeting_CalculateErrorVector(ioTargetingState);

	if (ioTargetingState->weapon_parameters->burst_radius == 0) {
		// this weapon is not a bursting weapon; start the delay timer now
		AI2iTargeting_DoDelay(ioTargetingState);
	}
}

static void AI2iTargeting_ApplyTargetingError(AI2tTargetingState *ioTargetingState, UUtBool inShootWeapon)
{
#if TARGETING_MISS_DECAY
	M3tVector3D aim_vector;
#endif

	// work out where we are targeting from (shoulder height)
	UUmAssert(ioTargetingState->callback->rGetPosition);
	ioTargetingState->callback->rGetPosition(ioTargetingState, &ioTargetingState->targeting_frompt);

	if (!inShootWeapon) {
		// no inaccuracy of the kind applied here
		MUmVector_Copy(ioTargetingState->current_aim_pt, ioTargetingState->target_pt);
		return;
	}

#if TARGETING_MISS_DECAY
	if (ioTargetingState->miss_count == 0){
#endif
		// aim at the target and add the random offset computed when each shot is fired
		MUmVector_Add(ioTargetingState->current_aim_pt, ioTargetingState->target_pt,
						ioTargetingState->error_vector);

#if TARGETING_MISS_DECAY
	} else {
		// rotate the aim point about us by the initial-miss rotation
#if AI_DEBUG_TARGETING
		COrConsole_Printf("applying miss-rotation of %f deg", ioTargetingState->miss_angle * M3cRadToDeg);
#endif
		MUmVector_Subtract(aim_vector, ioTargetingState->target_pt, ioTargetingState->targeting_frompt);
		MUrMatrix3x3_MultiplyPoint(&aim_vector, (M3tMatrix3x3 *) &ioTargetingState->miss_matrix, &aim_vector);
		MUmVector_Add(ioTargetingState->current_aim_pt, aim_vector, ioTargetingState->targeting_frompt);
	}
#endif
}

void ONrCreateTargetingVector(
	const M3tVector3D *current_aim_pt,						// point to aim the gun at, include prediction if required
	const M3tVector3D *targeting_frompt,					// characters location + height this frame
	const AI2tWeaponParameters *weapon_parameters,			//
	const M3tMatrix4x3 *weapon_matrix,						// character's weapon_matrix
	M3tVector3D *targeting_vector)							// the final vector we should be aiming along
{
	M3tPoint3D weapon_pt;
	M3tVector3D aiming_vector;
	float desired_aim_length, reverse_offset;

	// work out how far away the target is
	MUmVector_Subtract(aiming_vector, *current_aim_pt, *targeting_frompt);
	desired_aim_length = MUmVector_GetLength(aiming_vector);

	// when we get close to an enemy, we must check from a point that's behind our actual weapon...
	// this lets us account for problems that would occur if they were right on top of us and we
	// had pointed our gun through their chest
	reverse_offset = (desired_aim_length - AI2cCombat_WeaponTargetingOffsetInterpolate_Max) /
					(AI2cCombat_WeaponTargetingOffsetInterpolate_Min - AI2cCombat_WeaponTargetingOffsetInterpolate_Max);
	reverse_offset = UUmPin(reverse_offset, 0, 1);

	// work out the actual position of the shooter on our weapon rather than
	// the character's location... adjusts for the fact that we hold our weapon
	// out to one side of our body, and our shooter may not be centered on the weapon.
	//
	// shooter_perpoffset_gunspace which does not include the axial part of the shooter's position
	// in gunspace. instead we use an axial term (reverse_offset) only when the enemy gets close,
	// and it's away from the enemy. this ensures that we don't get wacky stuff happening when people
	// stand so close that the barrel of our gun goes through their chest (damn collision)
	MUmVector_Copy(weapon_pt, weapon_parameters->shooter_perpoffset_gunspace);
	weapon_pt.x -= AI2cCombat_WeaponTargetingOffsetShooterReverse * reverse_offset;
	MUrMatrix_MultiplyPoint(&weapon_pt, weapon_matrix, &weapon_pt);

	// adjust the target's location by weapon_pt rather than targeting_frompt so that we hit the target
	MUmVector_Subtract(aiming_vector, *current_aim_pt, weapon_pt);

	// we want the shooter on the gun to line up with our target. shooter_inversematrix
	// will do the trick, if we get into gunspace from worldspace first before applying it.
	//
	// note that we apply only the rotational component of inversematrix.
	MUrMatrix3x3_TransposeMultiplyPoint(&aiming_vector, (M3tMatrix3x3 *) weapon_matrix,
										&aiming_vector);
	MUrMatrix3x3_MultiplyPoint(&aiming_vector,
		(M3tMatrix3x3 *) &weapon_parameters->shooter_inversematrix, &aiming_vector);
	MUrMatrix3x3_MultiplyPoint(&aiming_vector, (M3tMatrix3x3 *) weapon_matrix, &aiming_vector);

	*targeting_vector = aiming_vector;
}


static void AI2iTargeting_UpdateTargeting(AI2tTargetingState *ioTargetingState, UUtBool inAimWeapon)
{
	UUtBool aim_with_weapon;
	float desired_aim_length, reverse_offset;
	AI2tCombatAimingState desired_aim_state;
	M3tVector3D orig_aiming_vector, offset_aiming_vector, wpn_offset;

	ioTargetingState->firing_solution = UUcTrue;

	if (inAimWeapon && (ioTargetingState->weapon_parameters != NULL)) {
		desired_aim_state = AI2cCombatAiming_ForceWeapon;
		aim_with_weapon = UUcTrue;
	} else {
		desired_aim_state = AI2cCombatAiming_ForceLook;
		aim_with_weapon = UUcFalse;
	}

	if (ioTargetingState->current_aim_state != desired_aim_state) {
		if (ioTargetingState->callback->rForceAimWithWeapon) {
			ioTargetingState->callback->rForceAimWithWeapon(ioTargetingState, aim_with_weapon);
		}
		ioTargetingState->current_aim_state = desired_aim_state;
	}

	if (aim_with_weapon) {
		// work out where we are aiming at
		MUmVector_Subtract(orig_aiming_vector, ioTargetingState->current_aim_pt,
							ioTargetingState->targeting_frompt);
		desired_aim_length = MUmVector_GetLength(orig_aiming_vector);

		if (ioTargetingState->weapon_parameters->ballistic_speed > 0) {
			float vel, vel_sq, height, height_sq, dist, dist_sq, gravity, gravity_sq;
			float t_sq, t, a, b, c, disc, disc_sqrt;

			// determine how we should aim in order to hit with this ballistic weapon
			ioTargetingState->firing_solution = UUcFalse;

			height = orig_aiming_vector.y;
			height_sq = UUmSQR(height);
			dist_sq = UUmSQR(orig_aiming_vector.x) + UUmSQR(orig_aiming_vector.z);
			dist = MUrSqrt(dist_sq);

			vel = ioTargetingState->weapon_parameters->ballistic_speed;
			vel_sq = UUmSQR(vel);

			gravity = ioTargetingState->weapon_parameters->ballistic_gravity * P3gGravityAccel;
			gravity_sq = UUmSQR(gravity);

			// in order to hit the point at (dist, height) at time t we must have:
			// x = dist / t
			// y = height / t + 1/2 * gravity * t
			//
			// our weapon constrains us to have v^2 = x^2 + y^2
			//
			// we substitute these and build a quadratic equation that solves for t^2
			a = gravity_sq / 4;
			b = gravity*height - vel_sq;
			c = dist_sq + height_sq;

			disc = UUmSQR(b) - 4*a*c;
			if (disc >= 0) {
				disc_sqrt = MUrSqrt(disc);

				t_sq = (-b - disc_sqrt) / (2 * a);
				if (t_sq > 0) {
					t = MUrSqrt(t_sq);
					ioTargetingState->solution_xvel = dist / t;
					ioTargetingState->solution_yvel = height / t + 0.5f * gravity * t;
					ioTargetingState->solution_theta = MUrATan2(orig_aiming_vector.x, orig_aiming_vector.z);
					UUmTrig_ClipLow(ioTargetingState->solution_theta);

					// we have actually found a firing solution!
					ioTargetingState->firing_solution = UUcTrue;
				}
			}

			if (ioTargetingState->firing_solution) {
				// adjust the desired aiming point up or down to compensate for the ballistic solution
				// that we found
				orig_aiming_vector.x = (orig_aiming_vector.x / dist) * (ioTargetingState->solution_xvel / vel) * desired_aim_length;
				orig_aiming_vector.z = (orig_aiming_vector.z / dist) * (ioTargetingState->solution_xvel / vel) * desired_aim_length;
				orig_aiming_vector.y =								   (ioTargetingState->solution_yvel / vel) * desired_aim_length;
			}
		}

		// when we get close to an enemy, we must check from a point that's behind our actual weapon...
		// this lets us account for problems that would occur if they were right on top of us and we
		// had pointed our gun through their chest
		reverse_offset = (desired_aim_length - AI2cCombat_WeaponTargetingOffsetInterpolate_Max) /
						(AI2cCombat_WeaponTargetingOffsetInterpolate_Min - AI2cCombat_WeaponTargetingOffsetInterpolate_Max);
		reverse_offset = UUmPin(reverse_offset, 0, 1);
		ioTargetingState->targeting_interp_val = reverse_offset;

		// work out the actual position of the shooter on our weapon rather than
		// the character's location... adjusts for the fact that we hold our weapon
		// out to one side of our body, and our shooter may not be centered on the weapon.
		//
		// shooter_perpoffset_gunspace which does not include the axial part of the shooter's position
		// in gunspace. instead we use an axial term (reverse_offset) only when the enemy gets close,
		// and it's away from the enemy. this ensures that we don't get wacky stuff happening when people
		// stand so close that the barrel of our gun goes through their chest (damn collision)
		MUmVector_Copy(ioTargetingState->weapon_pt, ioTargetingState->weapon_parameters->shooter_perpoffset_gunspace);
		ioTargetingState->weapon_pt.x -= AI2cCombat_WeaponTargetingOffsetShooterReverse * reverse_offset;
		MUrMatrix_MultiplyPoint(&ioTargetingState->weapon_pt, ioTargetingState->weapon_matrix, &ioTargetingState->weapon_pt);

		// calculate the aiming vector required from weapon_pt rather than from targeting_frompt
		MUmVector_Subtract(wpn_offset, ioTargetingState->targeting_frompt, ioTargetingState->weapon_pt);
		MUmVector_Add(ioTargetingState->desired_shooting_vector, wpn_offset, orig_aiming_vector);
		ioTargetingState->aiming_distance = MUmVector_GetLength(ioTargetingState->desired_shooting_vector);

		// we want the shooter on the gun to line up with our target. shooter_inversematrix
		// will do the trick, if we get into gunspace from worldspace first before applying it.
		//
		// note that we apply only the rotational component of inversematrix.
		MUrMatrix3x3_TransposeMultiplyPoint(&ioTargetingState->desired_shooting_vector,
									(M3tMatrix3x3 *) ioTargetingState->weapon_matrix, &offset_aiming_vector);
		MUrMatrix3x3_MultiplyPoint(&offset_aiming_vector,
			(M3tMatrix3x3 *) &ioTargetingState->weapon_parameters->shooter_inversematrix, &offset_aiming_vector);
		MUrMatrix3x3_MultiplyPoint(&offset_aiming_vector, (M3tMatrix3x3 *) ioTargetingState->weapon_matrix,
									&ioTargetingState->targeting_vector);

		if (ioTargetingState->callback->rAimVector) {
			ioTargetingState->callback->rAimVector(ioTargetingState, &ioTargetingState->targeting_vector);
		}

	} else {
		// just looking... we can look directly at people. this becomes necessary when
		// in melee and we need to block!
		ioTargetingState->aiming_distance = ioTargetingState->target_distance;

		if (ioTargetingState->target != NULL) {
			if (ioTargetingState->callback->rAimCharacter) {
				ioTargetingState->callback->rAimCharacter(ioTargetingState, ioTargetingState->target);
			}
		} else {
			if (ioTargetingState->callback->rAimPoint) {
				ioTargetingState->callback->rAimPoint(ioTargetingState, &ioTargetingState->target_pt);
			}
		}
	}

	MUmAssertVector(ioTargetingState->target_pt, 10000.0f);
	UUmAssert((ioTargetingState->aiming_distance >= 0) && (ioTargetingState->aiming_distance < 10000.0f));

	// update our aiming information for this tick
	if (ioTargetingState->callback->rUpdateAiming) {
		ioTargetingState->callback->rUpdateAiming(ioTargetingState);
	}
}

static void AI2iTargeting_DoDelay(AI2tTargetingState *ioTargetingState)
{
	ioTargetingState->delay_frames = UUmRandomRange(ioTargetingState->shooting_skill->delay_min,
			ioTargetingState->shooting_skill->delay_max - ioTargetingState->shooting_skill->delay_min);
}

static UUtBool AI2iTargeting_Fire(AI2tTargetingState *ioTargetingState)
{
	M3tVector3D shooter_dir;
	float aiming_accuracy, aiming_tolerance, min_aiming_tolerance, required_ratio, increase_tolerance;
	float aim_axial;
	UUtBool using_burst;

	// obey delay timer if we have one
	if (ioTargetingState->delay_frames > 0) {
		ioTargetingState->delay_frames--;

		if (ioTargetingState->delay_frames > 0) {
			return UUcFalse;
		}
	}

	if (ioTargetingState->aiming_distance < AI2cCombat_WeaponTargeting_AbsoluteMinimumRange) {
		// our enemy is stupidly close... pull the trigger after some number of frames
#if AI_DEBUG_TARGETING
		COrConsole_Printf("target range %f < absolute minimum %f",
							ioTargetingState->aiming_distance, AI2cCombat_WeaponTargeting_AbsoluteMinimumRange);
#endif
		ioTargetingState->minimumdist_frames++;
		if (ioTargetingState->minimumdist_frames >= AI2cCombat_WeaponTargeting_MinimumDistFireFrames) {
			// pull the trigger
#if AI_DEBUG_TARGETING
			COrConsole_Printf("minimum-dist frames %d > threshold %d, firing",
								ioTargetingState->minimumdist_frames, AI2cCombat_WeaponTargeting_MinimumDistFireFrames);
#endif
			ioTargetingState->returning_to_center = UUcFalse;
			return UUcTrue;
		} else {
#if AI_DEBUG_TARGETING
			COrConsole_Printf("minimum-dist frames %d < threshold %d, not yet firing",
								ioTargetingState->minimumdist_frames, AI2cCombat_WeaponTargeting_MinimumDistFireFrames);
#endif
		}
	} else {
		ioTargetingState->minimumdist_frames = 0;
	}

	// work out how close the gun's primary shooter particle's emission direction is, to the direction that we wanted to
	// point it in. we do this by transposint desired_shooting_vector into weapon-space and comparing it to the +Y direction.
	// note that desired_shooting_vector's length approximates distance to target.
	MUrMatrix3x3_TransposeMultiplyPoint(&ioTargetingState->desired_shooting_vector, (M3tMatrix3x3 *) ioTargetingState->weapon_matrix, &shooter_dir);
	MUrMatrix3x3_MultiplyPoint(&shooter_dir, (M3tMatrix3x3 *) &ioTargetingState->weapon_parameters->shooter_inversematrix, &shooter_dir);

	if (ioTargetingState->owner.type == AI2cTargetingOwnerType_Character) {
		// perfectly aligned is (1, 0, 0), as the gun is oriented along +X in its coordinate frame
		aiming_accuracy = MUrSqrt(UUmSQR(shooter_dir.y) + UUmSQR(shooter_dir.z));
		aim_axial = shooter_dir.x;
	} else {
		UUmAssert(ioTargetingState->owner.type == AI2cTargetingOwnerType_Turret);
		aiming_accuracy = MUrSqrt(UUmSQR(shooter_dir.x) + UUmSQR(shooter_dir.y));
		aim_axial = shooter_dir.z;
	}

	// there is a minimum alignment below which we will never fire... prevents
	// stupid firing behavior when the enemy is in close. it's a ratio of axial
	// distance to perpendicular distance... 30 degrees when far, 60 degrees when
	// close.
	//
	// outside interpolation distance, use min_ratio... inside minimum radius of interpolation distance,
	// use close_ratio. interpolate between the two as necessary.
	required_ratio = AI2cCombat_WeaponTargeting_CloseAccuracyRatio + ioTargetingState->targeting_interp_val *
				(AI2cCombat_WeaponTargeting_MinAccuracyRatio - AI2cCombat_WeaponTargeting_CloseAccuracyRatio);

	if (aim_axial / aiming_accuracy < required_ratio) {
#if AI_DEBUG_TARGETING
		COrConsole_Printf("rejecting vector %f / %f at dist %f due to ratio %f < %f",
							aim_axial, aiming_accuracy, ioTargetingState->aiming_distance,
							aim_axial / aiming_accuracy, required_ratio);
#endif
		// we are a *long* way from our target. don't use the reduced shooting aim speed, as we aren't
		// actually firing right now
		if (ioTargetingState->callback->rSetAimSpeed) {
			ioTargetingState->callback->rSetAimSpeed(ioTargetingState, 1.0f);
		}
		return UUcFalse;
	}

	aiming_tolerance = ioTargetingState->weapon_parameters->aim_radius;

	if ((ioTargetingState->weapon_parameters->burst_radius > aiming_tolerance) &&
		(!ioTargetingState->returning_to_center)) {
		// we are in the middle of a burst and our weapon has a greater tolerance
		// when burst firing
#if AI_DEBUG_TARGETING
		COrConsole_Printf("burstfiring: tolerance %f -> %f", aiming_tolerance,
							ioTargetingState->weapon_parameters->burst_radius);
#endif
		aiming_tolerance = ioTargetingState->weapon_parameters->burst_radius;
		using_burst = UUcTrue;
	} else {
		using_burst = UUcFalse;
	}

	// if we are randomly jittering our target point, add this to the aiming tolerance
	aiming_tolerance += AI2cCombat_Targeting_AimingErrorScale * ioTargetingState->error_distance;

#if TARGETING_MISS_DECAY
	// if we are still in the initial-missing phase, increase our aiming tolerance
	if (ioTargetingState->miss_count > 0) {
		float miss_tolerance;

		miss_tolerance = MUrSin(ioTargetingState->miss_angle) * ioTargetingState->aiming_distance;
		miss_tolerance = UUmMax(miss_tolerance, AI2cCombat_MinimumMissTolerance);
#if AI_DEBUG_TARGETING
		COrConsole_Printf("still missing: tolerance %f -> %f", aiming_tolerance, miss_tolerance);
#endif
		aiming_tolerance = UUmMax(aiming_tolerance, miss_tolerance);
	}
#else
	// if we're in our initial missing phase, increase our tolerance.
	if (ioTargetingState->miss_enable) {
//		COrConsole_Printf_Color(UUcTrue, 0xFF8080FF, 0xFF303060, "### target: miss tol %f, standard aiming tolerance %f",
//				ioTargetingState->miss_tolerance, aiming_tolerance);
		aiming_tolerance = UUmMax(aiming_tolerance, ioTargetingState->miss_tolerance);
	}
#endif

	// if we have been thwarted for a while, start increasing our aiming tolerance. prevents
	// infinite error conditions.
	if (ioTargetingState->thwarted_frames > AI2cCombat_Targeting_MinThwartFrames) {
		aiming_tolerance += (ioTargetingState->thwarted_frames - AI2cCombat_Targeting_MinThwartFrames)
				* AI2cCombat_Targeting_DistPerThwartedFrame;
#if AI_DEBUG_TARGETING
		COrConsole_Printf("thwarted... frames %d, increase tolerance %f",
						ioTargetingState->thwarted_frames,
			(ioTargetingState->thwarted_frames - AI2cCombat_Targeting_MinThwartFrames)
							* AI2cCombat_Targeting_DistPerThwartedFrame);
#endif
	}

	if (ioTargetingState->targeting_interp_val < 1.0f) {
		// our aiming tolerance gets larger as the enemy gets really close... this is so that we don't
		// become unable to fire when they are right next to us
		increase_tolerance = AI2cCombat_Targeting_CloseToleranceIncrease *
					(1.0f - ioTargetingState->targeting_interp_val);
#if AI_DEBUG_TARGETING
		COrConsole_Printf("target is close (%f)... tolerance increase by %f: %f -> %f",
						ioTargetingState->targeting_interp_val, increase_tolerance, aiming_tolerance,
						aiming_tolerance * (1.0f + increase_tolerance));
#endif
		aiming_tolerance *= 1.0f + increase_tolerance;
	} else {
		// the enemy is outside our max interpolation range. ensure that our
		// aiming tolerance never sinks below some class-specific angle... so we are actively
		// incapable of aiming precisely at a distance
		min_aiming_tolerance = ioTargetingState->shooting_skill->best_aiming_angle * aim_axial;
		if (min_aiming_tolerance > aiming_tolerance) {
#if AI_DEBUG_TARGETING
			COrConsole_Printf("min class aiming tolerance %f -> %f (was %f)",
				0.5f, min_aiming_tolerance, aiming_tolerance);
#endif
			aiming_tolerance = min_aiming_tolerance;
		}
	}

	ioTargetingState->last_computed_accuracy = aiming_accuracy;
	ioTargetingState->last_computed_tolerance = aiming_tolerance;
	ioTargetingState->last_computation_success = UUcTrue;

	if (aiming_accuracy > aiming_tolerance) {
		// we do not have a valid shot on the player, we are not aiming closely enough at them
#if AI_DEBUG_TARGETING
		COrConsole_Printf("OFF target. accuracy %f, tolerance %f, returning-to-center %s",
			aiming_accuracy, aiming_tolerance, ioTargetingState->returning_to_center ? "YES" : "NO");
#endif

		if (aiming_accuracy >= AI2cCombat_ShootingAimSpeed_MaxError * aiming_tolerance) {
			// we are a long way away from the target... don't use the reduced
			// aim-smoothing speed
			if (ioTargetingState->callback->rSetAimSpeed) {
				ioTargetingState->callback->rSetAimSpeed(ioTargetingState, 1.0f);
			}
		}

#if TARGETING_MISS_DECAY
		if (ioTargetingState->miss_count == 0) {
#else
		if (!ioTargetingState->miss_enable) {
#endif
			// must now return to aim at the player before firing again
			ioTargetingState->returning_to_center = UUcTrue;
		} else {
			// we are in our initial-missing phase; don't do the accurate aiming
			ioTargetingState->returning_to_center = UUcFalse;
		}

		if (using_burst) {
			// our burst is over, do delay before we can fire again
			AI2iTargeting_DoDelay(ioTargetingState);
		}

		ioTargetingState->thwarted_frames++;

		return UUcFalse;
	} else {
#if TARGETING_MISS_DECAY
		if (ioTargetingState->miss_count == 0) {
			// we are on target, and no longer missing. start interpolating our aiming direction in the executor
			// a bit more slowly.
			if (ioTargetingState->callback->rSetAimSpeed) {
				ioTargetingState->callback->rSetAimSpeed(ioTargetingState, AI2cCombat_ShootingAimSpeed);
			}
		}
#else
		if (ioTargetingState->miss_enable) {
			// now that we have become this accurate, we won't accept any less for subsequent shots
			ioTargetingState->miss_tolerance = UUmMin(aiming_accuracy, ioTargetingState->miss_tolerance);
//			COrConsole_Printf_Color(UUcTrue, 0xFF8080FF, 0xFF303060, "### target: on target, reducing to %f",
//					aiming_accuracy);
			if (ioTargetingState->miss_tolerance < ioTargetingState->weapon_parameters->aim_radius) {
				// we have reached the weapon's limit of accuracy, don't bother checking the miss-tolerance term
				// any longer
				ioTargetingState->miss_enable = UUcFalse;
//				COrConsole_Printf_Color(UUcTrue, 0xFF8080FF, 0xFF303060, "### target: stop missing (below radius %f)",
//						ioTargetingState->weapon_parameters->aim_radius);
			}
		}
#endif

		// we are on target!
		ioTargetingState->returning_to_center = UUcFalse;
		ioTargetingState->thwarted_frames = 0;

#if AI_DEBUG_TARGETING
		COrConsole_Printf("on target. accuracy %f, tolerance %f", aiming_accuracy, aiming_tolerance);
#endif

		// we are on target. fire.
		return UUcTrue;
	}
}

// ------------------------------------------------------------------------------------
// -- misc routines

void AI2rTargeting_ChangeWeapon(AI2tTargetingState *ioTargetingState, AI2tWeaponParameters *inWeaponParams,
								M3tMatrix4x3 *inWeaponMatrix)
{
	UUtUns32 index;

	ioTargetingState->weapon_parameters = inWeaponParams;
	ioTargetingState->weapon_matrix = inWeaponMatrix;

	if (ioTargetingState->weapon_parameters == NULL) {
		ioTargetingState->predict_position = UUcFalse;
		ioTargetingState->shooting_skill = NULL;

		// return the our full aiming speed
		if (ioTargetingState->callback->rSetAimSpeed) {
			ioTargetingState->callback->rSetAimSpeed(ioTargetingState, 1.0f);
		}
		return;
	}

	// get our shooting skill
	index = ioTargetingState->weapon_parameters->shootskill_index;
	UUmAssert((index >= 0) && (index < AI2cCombat_MaxWeapons));
	ioTargetingState->shooting_skill = &ioTargetingState->shooting_skillarray[index];

	// set up the prediction buffer
	ioTargetingState->predict_position = UUcTrue;
	ioTargetingState->next_sample = ioTargetingState->targeting_params->predict_trendframes - 1;
	ioTargetingState->num_samples_taken = 0;
	MUmVector_Set(ioTargetingState->predicted_velocity, 0, 0, 0);
	MUmVector_Set(ioTargetingState->predicted_trend, 0, 0, 0);

	UUmAssert(ioTargetingState->targeting_params->predict_velocityframes > ioTargetingState->targeting_params->predict_delayframes);
	UUmAssert(ioTargetingState->targeting_params->predict_trendframes >= ioTargetingState->targeting_params->predict_velocityframes);

	ioTargetingState->predictionbuf =
		(M3tVector3D *) UUrMemory_Block_Realloc((void *) ioTargetingState->predictionbuf,
					ioTargetingState->targeting_params->predict_trendframes * sizeof(M3tVector3D));
}

