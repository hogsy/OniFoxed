#pragma once
#ifndef ONI_AI2_ERROR_H
#define ONI_AI2_ERROR_H

/*
	FILE:	 Oni_AI2_Error.h
	
	AUTHOR:	 Chris Butcher
	
	CREATED: April 12, 1999
	
	PURPOSE: Error handler for Oni's AI system
	
	Copyright (c) 2000, Bungie Software

*/

#include "BFW.h"
#include "Oni_AI2.h"

#if TOOL_VERSION
#define AI2_ERROR_REPORT        1
#else
#define AI2_ERROR_REPORT        0
#endif

// ------------------------------------------------------------------------------------
// -- constants


/* error severity */
typedef enum {
	AI2cStatus			= 0,
	AI2cWarning			= 1,
	AI2cError			= 2,
	AI2cBug				= 3,
	AI2cSeverity_Max	= 4
} AI2tErrorSeverity;

/* subsystem delineation */
typedef enum {
	AI2cSubsystem_All			= 0,
	AI2cSubsystem_Executor		= 1,
	AI2cSubsystem_Movement		= 2,
	AI2cSubsystem_Pathfinding	= 3,
	AI2cSubsystem_HighLevel		= 4,
	AI2cSubsystem_Patrol		= 5,
	AI2cSubsystem_Error			= 6,
	AI2cSubsystem_Knowledge		= 7,
	AI2cSubsystem_Combat		= 8,
	AI2cSubsystem_Melee			= 9,
	AI2cSubsystem_Fight			= 10,
	AI2cSubsystem_Max			= 11
} AI2tErrorSubsystem;

#define AI2cSubsystem_None AI2cSubsystem_All

// ------------------------------------------------------------------------------------
// -- structures

// last pathfinding error storage
typedef struct AI2tPathfindingErrorStorage {
	UUtBool					no_bnv;
	M3tPoint3D				bnv_test_location;

	UUtBool					no_path;
	M3tPoint3D				path_start, path_end;

	UUtBool					no_astar;
	M3tPoint3D				astar_start, astar_end;
	PHtRoomData *			astar_room;
	PHtNode *				astar_node;
	IMtPoint2D				astar_gridstart, astar_gridend;

	UUtBool					collision_stuck;
	M3tPoint3D				collision_point;
	UUtUns32				numCollisions;
	AI2tCollision			collision[AI2cMovementState_MaxCollisions];
	UUtUns32				numBadnessValues;
	AI2tBadnessValue		badnesslist[AI2cMovementState_MaxBadnessValues];

} AI2tPathfindingErrorStorage;

// ------------------------------------------------------------------------------------
// -- globals

extern const char AI2cErrorSeverityDesc[AI2cSeverity_Max][16];
extern const char AI2cErrorSubsystemDesc[AI2cSubsystem_Max][16];

// how to handle the current error being reported
extern UUtBool AI2gError_ReportThisError, AI2gError_LogThisError;

// global storage for pathfinding error display
extern AI2tPathfindingErrorStorage AI2gPathfindingErrorStorage;

// ------------------------------------------------------------------------------------
// -- error table definitions

typedef void (*AI2tErrorReportFunction)(ONtCharacter *inCharacter,
								  UUtUns32 inParam1, UUtUns32 inParam2,
								  UUtUns32 inParam3, UUtUns32 inParam4);

typedef UUtBool (*AI2tErrorHandleFunction)(ONtCharacter *inCharacter,
								  UUtUns32 inParam1, UUtUns32 inParam2,
								  UUtUns32 inParam3, UUtUns32 inParam4);

typedef UUtBool (*AI2tPathfindingErrorHandler)(ONtCharacter *inCharacter, UUtUns32 inErrorID,
								  UUtUns32 inParam1, UUtUns32 inParam2,
								  UUtUns32 inParam3, UUtUns32 inParam4);

typedef struct AI2tErrorTableEntry {
	UUtUns32					error_id;
	AI2tErrorReportFunction		report_function;
	AI2tErrorHandleFunction		handle_function;
} AI2tErrorTableEntry;


enum {
	AI2cError_None_Report
};

enum {
	AI2cError_Executor_TurnOverride,
	AI2cError_Executor_UnexpectedAttack
};

enum {
	AI2cError_Movement_InvalidAlertStatus,
	AI2cError_Movement_MaxModifiers,
	AI2cError_Movement_MaxCollisions,
	AI2cError_Movement_Collision,
	AI2cError_Movement_CollisionStalled,
	AI2cError_Movement_MaxBadnessValues
};

enum {
	AI2cError_Pathfinding_NoError = 0,		// used by movement code, never reported
	AI2cError_Pathfinding_NoBNVAtStart,
	AI2cError_Pathfinding_NoBNVAtDest,
	AI2cError_Pathfinding_NoConnectionsFromStart,
	AI2cError_Pathfinding_NoConnectionsToDest,
	AI2cError_Pathfinding_NoPath,
	AI2cError_Pathfinding_AStarFailed,
	AI2cError_Pathfinding_CollisionStuck,
	AI2cError_Pathfinding_FellFromRoom,
	AI2cError_Pathfinding_FellFromPath,

	AI2cError_Pathfinding_Max
};

enum {
	AI2cError_Patrol_InvalidEntry,
	AI2cError_Patrol_BeginPatrol,
	AI2cError_Patrol_AtWaypoint,
	AI2cError_Patrol_NoSuchFlag,
	AI2cError_Patrol_FailedWaypoint
};

enum {
	AI2cError_HighLevel_UnknownJob,
	AI2cError_HighLevel_ImproperJob,
	AI2cError_HighLevel_CombatSettings,
	AI2cError_HighLevel_NoPath,
	AI2cError_HighLevel_NoMeleeProfile,
	AI2cError_HighLevel_MeleeVariant,
	AI2cError_HighLevel_NoCharacter,
	AI2cError_HighLevel_TalkBufferFull,
	AI2cError_HighLevel_NoNeutralBehavior
};

enum {
	AI2cError_Error_OpenLog,
	AI2cError_Error_InvalidSubsystem,
	AI2cError_Error_InvalidSeverity,
	AI2cError_Error_UnknownError
};

enum {
	AI2cError_Knowledge_MaxEntries,
	AI2cError_Knowledge_InvalidContact,
	AI2cError_Knowledge_NoStartle,
	AI2cError_Knowledge_MaxPending,
	AI2cError_Knowledge_MaxDodgeProjectiles,
	AI2cError_Knowledge_MaxDodgeFiringSpreads
};

enum {
	AI2cError_Combat_UnknownBehavior,
	AI2cError_Combat_UnimplementedBehavior,
	AI2cError_Combat_UnhandledMessage,
	AI2cError_Combat_TargetDead,
	AI2cError_Combat_ContactDisappeared
};

enum {
	AI2cError_Melee_AttackMoveNotInCombo,
	AI2cError_Melee_NoCharacterClass,
	AI2cError_Melee_PreComputeBroken,
	AI2cError_Melee_MultiDirectionTechnique,
	AI2cError_Melee_MaxTransitions,
	AI2cError_Melee_Failed_Attack,
	AI2cError_Melee_NoTransitionToDelay,
	AI2cError_Melee_NoTransitionOutOfDelay,
	AI2cError_Melee_NoDelayAnimation,
	AI2cError_Melee_NoTransitionToStart,
	AI2cError_Melee_BlockAnimIndex,
	AI2cError_Melee_BrokenTechnique,
	AI2cError_Melee_BrokenMove,
	AI2cError_Melee_BrokenAnim,
	AI2cError_Melee_TransitionUnavailable,
	AI2cError_Melee_MaxDelayTimers
};

enum {
	AI2cError_Fight_MaxFights
};

// ------------------------------------------------------------------------------------
// -- function prototypes

/* initialization */

UUtError AI2rError_Initialize(void);
void AI2rError_Terminate(void);

/* error interface */

// handle an error - can be used to catch problems before they blow up
UUtBool AI2rHandleError(AI2tErrorSubsystem inSystem, UUtUns32 inErrorID, ONtCharacter *inCharacter,
						UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4,
						AI2tErrorTableEntry **outEntry);

// install a temporary error handler for pathfinding
AI2tPathfindingErrorHandler AI2rError_TemporaryPathfindingHandler(AI2tPathfindingErrorHandler inHandler);
void AI2rError_RestorePathfindingHandler(AI2tPathfindingErrorHandler inHandler);

#if AI2_ERROR_REPORT

// errors may be reported to the console and logged
#define AI2_ERROR(sev, system, err_id, character, p1, p2, p3, p4)					\
		AI2rReportError(system, err_id, character,									\
			(UUtUns32) (p1), (UUtUns32) (p2), (UUtUns32) (p3), (UUtUns32) (p4),		\
			__FILE__, __LINE__, sev);

#else	// AI2_ERROR_REPORT

// errors are only checked for handling and are not logged or reported
#define AI2_ERROR(sev, system, err_id, character, p1, p2, p3, p4)                   \
		AI2rHandleError(system, err_id, character,									\
			(UUtUns32) (p1), (UUtUns32) (p2), (UUtUns32) (p3), (UUtUns32) (p4), NULL);

#endif	// AI2_ERROR_REPORT

UUtBool AI2rReportError(AI2tErrorSubsystem inSystem, UUtUns32 inErrorID, ONtCharacter *inCharacter,
						UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4,
						const char *inFile, UUtUns32 inLine, AI2tErrorSeverity inSeverity);

void AI2rError_ReportLine(char *inLine);
void AI2rError_LogLine(char *inLine);
void AI2rError_Line(char *inLine);

/* control functions */

AI2tErrorSeverity AI2rError_GetSeverityFromString(char *inString);
AI2tErrorSubsystem AI2rError_GetSubsystemFromString(char *inString);

void AI2rError_SetReportLevel(AI2tErrorSubsystem inSystem, AI2tErrorSeverity inReportLevel);
void AI2rError_SetLogLevel(AI2tErrorSubsystem inSystem, AI2tErrorSeverity inLogLevel);
void AI2rError_SetSilentHandling(AI2tErrorSubsystem inSystem, UUtBool inSilentHandling);


/* user interface */

void AI2rError_ClearPathfindingErrors(void);
void AI2rError_DisplayPathfindingErrors(void);

/* default pathfinding error handling */
UUtBool AI2rError_DefaultPathfindingHandler(ONtCharacter *inCharacter, UUtUns32 inErrorID,
											UUtUns32 inParam1, UUtUns32 inParam2,
											UUtUns32 inParam3, UUtUns32 inParam4);

#endif // ONI_AI2_ERROR_H
