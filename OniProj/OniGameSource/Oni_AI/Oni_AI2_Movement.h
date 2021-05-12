#pragma once
#ifndef ONI_AI2_MOVEMENT_H
#define ONI_AI2_MOVEMENT_H

/*
	FILE:	Oni_AI2_Movement.h
	
	AUTHOR:	Chris Butcher
	
	CREATED: November 15, 1999
	
	PURPOSE: Movement AI for Oni
	
	Copyright (c) Bungie Software, 1999

*/

#include "BFW.h"
#include "Oni_AStar.h"
#include "Oni_AI.h"
#include "Oni_AI2_Script.h"
#include "Oni_Path.h"

// ------------------------------------------------------------------------------------
// -- constants

#define AI2cMoveToPoint_DefaultDistance		(0.75f * PHcSquareSize)
#define AI2cMoveThroughNode_DefaultDistance 15.0f
#define AI2cAngle_None						1000.0f

// ------------------------------------------------------------------------------------
// -- movement state definitions

/* modes of movement */
typedef enum AI2tMovementMode
{
	AI2cMovementMode_Default		= 0,
//	AI2cMovementMode_Stop			= 1,	// stationary modes are not useful
//	AI2cMovementMode_Crouch			= 2,	// now handled by AI2cMovementDirection_Stopped
	AI2cMovementMode_Creep			= 3,
	AI2cMovementMode_NoAim_Walk		= 4,
	AI2cMovementMode_Walk			= 5,
	AI2cMovementMode_NoAim_Run		= 6,
	AI2cMovementMode_Run			= 7,
	AI2cMovementMode_Max			= 8
} AI2tMovementMode;

extern const char AI2cMovementModeName[][16];
extern const AI2tMovementMode AI2cDefaultMovementMode_Hurry[];
extern const AI2tMovementMode AI2cDefaultMovementMode_Unhurried[];

/* direction to move in */
typedef enum AI2tMovementDirection
{
	AI2cMovementDirection_Forward		= 0,
	AI2cMovementDirection_Backpedal		= 1,
	AI2cMovementDirection_Sidestep_Left	= 2,
	AI2cMovementDirection_Sidestep_Right= 3,
	AI2cMovementDirection_Stopped		= 4,
	AI2cMovementDirection_Max			= 5
} AI2tMovementDirection;

extern const char AI2cMovementDirectionName[][16];
extern const float AI2cMovementDirection_Offset[];
extern const AI2tMovementDirection AI2cMovementDirection_TurnLeft[];
extern const AI2tMovementDirection AI2cMovementDirection_TurnRight[];
extern const AI2tMovementDirection AI2cMovementDirection_TurnAround[];


/* where to aim / look */
typedef enum AI2tAimingMode
{
	AI2cAimingMode_None					= 0,
	AI2cAimingMode_LookAtPoint			= 1,
	AI2cAimingMode_LookInDirection		= 2,
	AI2cAimingMode_LookAtCharacter		= 3
} AI2tAimingMode;

typedef struct AI2tAimingData_LookAtPoint
{
	M3tPoint3D point;
} AI2tAimingData_LookAtPoint;

typedef struct AI2tAimingData_LookInDirection
{
	M3tPoint3D vector;
} AI2tAimingData_LookInDirection;

typedef struct AI2tAimingData_LookAtCharacter
{
	ONtCharacter *character;
} AI2tAimingData_LookAtCharacter;

typedef union AI2tAimingData
{
	AI2tAimingData_LookAtPoint		lookAtPoint;
	AI2tAimingData_LookInDirection	lookInDirection;
	AI2tAimingData_LookAtCharacter	lookAtCharacter;
} AI2tAimingData;

/* how to face */
typedef enum AI2tFacingMode
{
	AI2cFacingMode_None					= 0,
	AI2cFacingMode_PathRelative			= 1,
	AI2cFacingMode_Angle				= 2,
	AI2cFacingMode_FaceAtDestination	= 3
} AI2tFacingMode;

typedef struct AI2tFacingData_PathRelative
{
	AI2tMovementDirection direction;
} AI2tFacingData_PathRelative;

typedef struct AI2tFacingData_Angle
{
	float angle;
} AI2tFacingData_Angle;

typedef struct AI2tFacingData_FaceAtDestination
{
	float angle;
} AI2tFacingData_FaceAtDestination;

typedef union AI2tFacingData
{
	AI2tFacingData_PathRelative pathRelative;
	AI2tFacingData_Angle angle;
	AI2tFacingData_FaceAtDestination faceAtDestination;
} AI2tFacingData;

/* movement modifiers */

typedef struct AI2tMovementModifier {
	float				direction;
	M3tVector3D			unitvector;
	float				full_weight;
	float				decreased_weight;
} AI2tMovementModifier;

#define AI2cMovement_MaxModifiers		8

/*
 * movement settings
 */

typedef struct AI2tMovementOrders {
	// walk, run, creep etc
	AI2tMovementMode		movementMode;
	UUtBool					slowdownDisabled;
	
	// character to attempt to pathfind through
	ONtCharacter			*curPathIgnoreChar;

	// movement modifiers
	UUtUns32				numModifiers;
	AI2tMovementModifier	modifier[AI2cMovement_MaxModifiers];
	float					modifierAvoidAngle;

	// facing information
	AI2tFacingMode			facingMode;
	AI2tFacingData			facingData;

	// where to look / aim
	UUtBool					force_aim;
	UUtBool					force_withweapon;
	AI2tAimingMode			aimingMode;
	AI2tAimingData			aimingData;

	// temporary look / aim override [a.k.a. glancing]
	UUtUns32				glance_delay;
	UUtUns32				glance_timer;
	UUtBool					glance_danger;
	AI2tAimingMode			glanceAimingMode;
	AI2tAimingData			glanceAimingData;

} AI2tMovementOrders;

/*
 * query functions
 */

// get the default movement speed for a character based on what their alert state is
// and what they're doing
AI2tMovementMode AI2rMovement_Default(ONtCharacter *ioCharacter);

// ------------------------------------------------------------------------------------
// -- movement routines
// (note: all of these are simply passed through to Oni_AI2_MovementState.c or Oni_AI2_MovementStub.c)

/*
 * order interface with other managers
 */

void AI2rMovement_Initialize(ONtCharacter *ioCharacter);

// movement modifiers
UUtError AI2rMovement_MovementModifier(ONtCharacter *ioCharacter, float inDirection, float inWeight);
void AI2rMovement_MovementModifierAvoidAngle(ONtCharacter *ioCharacter, float inDirection);
void AI2rMovement_ClearMovementModifiers(ONtCharacter *ioCharacter);

// change movement mode
void AI2rMovement_DisableSlowdown(ONtCharacter *ioCharacter, UUtBool inDisabled);
void AI2rMovement_ChangeMovementMode(ONtCharacter *ioCharacter, AI2tMovementMode inNewMode);
void AI2rMovement_NotifyAlertChange(ONtCharacter *ioCharacter);

// aiming
void AI2rMovement_StopAiming(ONtCharacter *ioCharacter);
void AI2rMovement_LookAtPoint(ONtCharacter *ioCharacter, const M3tPoint3D *inPoint);
void AI2rMovement_LookInDirection(ONtCharacter *ioCharacter, const M3tPoint3D *inDirection);
void AI2rMovement_LookAtCharacter(ONtCharacter *ioCharacter, ONtCharacter *inTarget);
void AI2rMovement_DontForceAim(ONtCharacter *ioCharacter);
void AI2rMovement_Force_AimWithWeapon(ONtCharacter *ioCharacter, UUtBool inAimWithWeapon);

// glancing
void AI2rMovement_StopGlancing(ONtCharacter *ioCharacter);
void AI2rMovement_GlanceAtPoint(ONtCharacter *ioCharacter, const M3tPoint3D *inPoint, UUtUns32 inTimer, UUtBool inDanger, UUtUns32 inDelay);
void AI2rMovement_GlanceInDirection(ONtCharacter *ioCharacter, const M3tPoint3D *inDirection, UUtUns32 inTimer, UUtBool inDanger, UUtUns32 inDelay);
void AI2rMovement_GlanceAtCharacter(ONtCharacter *ioCharacter, ONtCharacter *inTarget, UUtUns32 inTimer, UUtBool inDanger, UUtUns32 inDelay);

// facing
void AI2rMovement_LockFacing(ONtCharacter *ioCharacter, AI2tMovementDirection inDirection);
void AI2rMovement_FreeFacing(ONtCharacter *ioCharacter);

/*
 * interface with high-level path subsystem
 */

// query functions
UUtBool AI2rMovement_HasMovementModifiers(ONtCharacter *ioCharacter);
UUtBool AI2rMovement_IsKeepingMoving(ONtCharacter *ioCharacter);
UUtBool AI2rMovement_ShouldIgnoreObstruction(ONtCharacter *ioCharacter, ONtCharacter *inObstruction);
float	AI2rMovement_GetMoveDirection(ONtCharacter *ioCharacter);
UUtBool AI2rMovement_SimplePath(ONtCharacter *ioCharacter);

// state setting
void	AI2rMovement_IgnoreCharacter(ONtCharacter *ioCharacter, ONtCharacter *inIgnoreChar);

// command routines
void	AI2rMovement_NewPathReceived(ONtCharacter *ioCharacter);
void	AI2rMovement_Halt(ONtCharacter *ioCharacter);
void	AI2rMovement_NewDestination(ONtCharacter *ioCharacter);
void	AI2rMovement_ClearPath(ONtCharacter *ioCharacter);
UUtBool AI2rMovement_AdvanceThroughGrid(ONtCharacter *ioCharacter);
void	AI2rMovement_Update(ONtCharacter *ioCharacter);
UUtBool AI2rMovement_DestinationFacing(ONtCharacter *ioCharacter);
UUtBool AI2rMovement_CheckFailure(ONtCharacter *ioCharacter);
UUtBool AI2rMovement_MakePath(ONtCharacter *ioCharacter, UUtBool inReusePath);

/*
 * debugging routines
 */

// report in
void AI2rMovement_Report(ONtCharacter *ioCharacter, UUtBool inVerbose, AI2tReportFunction inFunction);

// debugging display
void AI2rMovement_RenderPath(ONtCharacter *ioCharacter);

#endif  // ONI_AI2_MOVEMENT_H
