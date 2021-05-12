/*
	FILE:	 Oni_AI2_Error.c
	
	AUTHOR:	 Chris Butcher
	
	CREATED: April 12, 2000
	
	PURPOSE: Error handler for Oni's AI system
	
	Copyright (c) Bungie Software, 2000

*/

#include "Oni_AI2.h"
#include "Oni_Character.h"
#include "Oni_Character_Animation.h"


// ------------------------------------------------------------------------------------
// -- external globals

const char AI2cErrorSeverityDesc[AI2cSeverity_Max][16] = {"status", "warning", "error", "bug"};

const char AI2cErrorSubsystemDesc[AI2cSubsystem_Max][16] =
	 {"all", "executor", "movement", "pathfinding", "high-level", "patrol", "error", "knowledge", "combat", "melee", "fight"};

// how to handle the current error being reported
UUtBool				AI2gError_ReportThisError, AI2gError_LogThisError;

// ------------------------------------------------------------------------------------
// -- internal globals

// settings for error reporting
AI2tErrorSeverity	AI2gError_ReportLevel		[AI2cSubsystem_Max];
AI2tErrorSeverity	AI2gError_LogLevel			[AI2cSubsystem_Max];
UUtBool				AI2gError_SilentHandling	[AI2cSubsystem_Max];

// AI2 error log file
UUtBool				AI2gError_LogFileOpened;
FILE *				AI2gError_LogFile;

// global line buffer
char				AI2gErrorBuf				[256];

// temporary error handler for pathfinding
AI2tPathfindingErrorHandler	AI2gPathfindingHandler;

// global storage for pathfinding error display
AI2tPathfindingErrorStorage AI2gPathfindingErrorStorage;


// ------------------------------------------------------------------------------------
// -- internal function prototypes

/* error reporting */
static void AI2iReport_All_Report(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Error_OpenLog(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Error_InvalidSubsystem(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Error_InvalidSeverity(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Error_UnknownError(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Executor_TurnOverride(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Executor_UnexpectedAttack(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Movement_InvalidAlertStatus(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Movement_MaxModifiers(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Movement_MaxCollisions(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Movement_Collision(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Movement_CollisionStalled(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Movement_MaxBadnessValues(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_HighLevel_UnknownJob(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_HighLevel_ImproperJob(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_HighLevel_CombatSettings(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_HighLevel_NoPath(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_HighLevel_NoMeleeProfile(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_HighLevel_MeleeVariant(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_HighLevel_NoCharacter(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_HighLevel_TalkBufferFull(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_HighLevel_NoNeutralBehavior(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Pathfinding_NoBNVAtStart(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Pathfinding_NoBNVAtDest(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Pathfinding_NoConnectionsFromStart(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Pathfinding_NoConnectionsToDest(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Pathfinding_NoPath(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Pathfinding_AStarFailed(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Pathfinding_CollisionStuck(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Pathfinding_FellFromRoom(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Pathfinding_FellFromPath(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Patrol_InvalidEntry(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Patrol_BeginPatrol(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Patrol_AtWaypoint(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Patrol_NoSuchFlag(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Patrol_FailedWaypoint(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Knowledge_MaxEntries(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Knowledge_InvalidContact(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Knowledge_NoStartle(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Knowledge_MaxPending(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Knowledge_MaxDodgeProjectiles(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Knowledge_MaxDodgeFiringSpreads(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Combat_UnknownBehavior(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Combat_UnimplementedBehavior(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Combat_UnhandledMessage(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Combat_TargetDead(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Combat_ContactDisappeared(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Melee_AttackMoveNotInCombo(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Melee_NoCharacterClass(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Melee_PreComputeBroken(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Melee_MultiDirectionTechnique(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Melee_MaxTransitions(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Melee_Failed_Attack(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Melee_NoTransitionToDelay(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Melee_NoTransitionOutOfDelay(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Melee_NoDelayAnimation(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Melee_NoTransitionToStart(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Melee_BlockAnimIndex(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Melee_BrokenMove(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Melee_BrokenTechnique(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Melee_BrokenAnim(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Melee_TransitionUnavailable(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Melee_MaxDelayTimers(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);
static void AI2iReport_Fight_MaxFights(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4);

// ------------------------------------------------------------------------------------
// -- error tables


AI2tErrorTableEntry AI2cErrorTable_None[] = {
	{AI2cError_None_Report,							AI2iReport_All_Report,							NULL				},
	{-1,											NULL,											NULL				}
};

AI2tErrorTableEntry AI2cErrorTable_Executor[] = {
	{AI2cError_Executor_TurnOverride,				AI2iReport_Executor_TurnOverride,				NULL				},
	{AI2cError_Executor_UnexpectedAttack,			AI2iReport_Executor_UnexpectedAttack,			NULL				},
	{-1,											NULL,											NULL				}
};

AI2tErrorTableEntry AI2cErrorTable_Movement[] = {
	{AI2cError_Movement_InvalidAlertStatus,			AI2iReport_Movement_InvalidAlertStatus,			NULL				},
	{AI2cError_Movement_MaxModifiers,				AI2iReport_Movement_MaxModifiers,				NULL				},
	{AI2cError_Movement_MaxCollisions,				AI2iReport_Movement_MaxCollisions,				NULL				},
	{AI2cError_Movement_Collision,					AI2iReport_Movement_Collision,					NULL				},
	{AI2cError_Movement_CollisionStalled,			AI2iReport_Movement_CollisionStalled,			NULL				},
	{AI2cError_Movement_MaxBadnessValues,			AI2iReport_Movement_MaxBadnessValues,			NULL				},
	{-1,											NULL,											NULL				}
};

AI2tErrorTableEntry AI2cErrorTable_Pathfinding[] = {
	{AI2cError_Pathfinding_NoBNVAtStart,			AI2iReport_Pathfinding_NoBNVAtStart,			NULL				},
	{AI2cError_Pathfinding_NoBNVAtDest,				AI2iReport_Pathfinding_NoBNVAtDest,				NULL				},
	{AI2cError_Pathfinding_NoConnectionsFromStart,	AI2iReport_Pathfinding_NoConnectionsFromStart,	NULL				},
	{AI2cError_Pathfinding_NoConnectionsToDest,		AI2iReport_Pathfinding_NoConnectionsToDest,		NULL				},
	{AI2cError_Pathfinding_NoPath,					AI2iReport_Pathfinding_NoPath,					NULL				},
	{AI2cError_Pathfinding_AStarFailed,				AI2iReport_Pathfinding_AStarFailed,				NULL				},
	{AI2cError_Pathfinding_CollisionStuck,			AI2iReport_Pathfinding_CollisionStuck,			NULL				},
	{AI2cError_Pathfinding_FellFromRoom,			AI2iReport_Pathfinding_FellFromRoom,			NULL				},
	{AI2cError_Pathfinding_FellFromPath,			AI2iReport_Pathfinding_FellFromPath,			NULL				},
	{-1,											NULL,											NULL				}
};

AI2tErrorTableEntry AI2cErrorTable_HighLevel[] = {
	{AI2cError_HighLevel_UnknownJob,				AI2iReport_HighLevel_UnknownJob,				NULL				},
	{AI2cError_HighLevel_ImproperJob,				AI2iReport_HighLevel_ImproperJob,				NULL				},
	{AI2cError_HighLevel_CombatSettings,			AI2iReport_HighLevel_CombatSettings,			NULL				},
	{AI2cError_HighLevel_NoPath,					AI2iReport_HighLevel_NoPath,					NULL				},
	{AI2cError_HighLevel_NoMeleeProfile,			AI2iReport_HighLevel_NoMeleeProfile,			NULL				},
	{AI2cError_HighLevel_MeleeVariant,				AI2iReport_HighLevel_MeleeVariant,				NULL				},
	{AI2cError_HighLevel_NoCharacter,				AI2iReport_HighLevel_NoCharacter,				NULL				},
	{AI2cError_HighLevel_TalkBufferFull,			AI2iReport_HighLevel_TalkBufferFull,			NULL				},
	{AI2cError_HighLevel_NoNeutralBehavior,			AI2iReport_HighLevel_NoNeutralBehavior,			NULL				},
	{-1,											NULL,											NULL				}
};

AI2tErrorTableEntry AI2cErrorTable_Patrol[] = {
	{AI2cError_Patrol_InvalidEntry,					AI2iReport_Patrol_InvalidEntry,					NULL				},
	{AI2cError_Patrol_BeginPatrol,					AI2iReport_Patrol_BeginPatrol,					NULL				},
	{AI2cError_Patrol_AtWaypoint,					AI2iReport_Patrol_AtWaypoint,					NULL				},
	{AI2cError_Patrol_NoSuchFlag,					AI2iReport_Patrol_NoSuchFlag,					NULL				},
	{AI2cError_Patrol_FailedWaypoint,				AI2iReport_Patrol_FailedWaypoint,				NULL				},
	{-1,											NULL,											NULL				}
};

AI2tErrorTableEntry AI2cErrorTable_Error[] = {
	{AI2cError_Error_OpenLog,						AI2iReport_Error_OpenLog,						NULL				},
	{AI2cError_Error_InvalidSubsystem,				AI2iReport_Error_InvalidSubsystem,				NULL				},
	{AI2cError_Error_InvalidSeverity,				AI2iReport_Error_InvalidSeverity,				NULL				},
	{AI2cError_Error_UnknownError,					AI2iReport_Error_UnknownError,					NULL				},
	{-1,											NULL,											NULL				}
};

AI2tErrorTableEntry AI2cErrorTable_Knowledge[] = {
	{AI2cError_Knowledge_MaxEntries,				AI2iReport_Knowledge_MaxEntries,				NULL				},
	{AI2cError_Knowledge_InvalidContact,			AI2iReport_Knowledge_InvalidContact,			NULL				},
	{AI2cError_Knowledge_NoStartle,					AI2iReport_Knowledge_NoStartle,					NULL				},
	{AI2cError_Knowledge_MaxPending,				AI2iReport_Knowledge_MaxPending,				NULL				},
	{AI2cError_Knowledge_MaxDodgeProjectiles,		AI2iReport_Knowledge_MaxDodgeProjectiles,		NULL				},
	{AI2cError_Knowledge_MaxDodgeFiringSpreads,		AI2iReport_Knowledge_MaxDodgeFiringSpreads,		NULL				},
	{-1,											NULL,											NULL				}
};

AI2tErrorTableEntry AI2cErrorTable_Combat[] = {
	{AI2cError_Combat_UnknownBehavior,				AI2iReport_Combat_UnknownBehavior,				NULL				},
	{AI2cError_Combat_UnimplementedBehavior,		AI2iReport_Combat_UnimplementedBehavior,		NULL				},
	{AI2cError_Combat_UnhandledMessage,				AI2iReport_Combat_UnhandledMessage,				NULL				},
	{AI2cError_Combat_TargetDead,					AI2iReport_Combat_TargetDead,					NULL				},
	{AI2cError_Combat_ContactDisappeared,			AI2iReport_Combat_ContactDisappeared,			NULL				},
	{-1,											NULL,											NULL				}
};

AI2tErrorTableEntry AI2cErrorTable_Melee[] = {
	{AI2cError_Melee_AttackMoveNotInCombo,			AI2iReport_Melee_AttackMoveNotInCombo,			NULL				},
	{AI2cError_Melee_NoCharacterClass,				AI2iReport_Melee_NoCharacterClass,				NULL				},
	{AI2cError_Melee_PreComputeBroken,				AI2iReport_Melee_PreComputeBroken,				NULL				},
	{AI2cError_Melee_MultiDirectionTechnique,		AI2iReport_Melee_MultiDirectionTechnique,		NULL				},
	{AI2cError_Melee_MaxTransitions,				AI2iReport_Melee_MaxTransitions,				NULL				},
	{AI2cError_Melee_Failed_Attack,					AI2iReport_Melee_Failed_Attack,					NULL				},
	{AI2cError_Melee_NoTransitionToDelay,			AI2iReport_Melee_NoTransitionToDelay,			NULL				},
	{AI2cError_Melee_NoTransitionOutOfDelay,		AI2iReport_Melee_NoTransitionOutOfDelay,		NULL				},
	{AI2cError_Melee_NoDelayAnimation,				AI2iReport_Melee_NoDelayAnimation,				NULL				},
	{AI2cError_Melee_NoTransitionToStart,			AI2iReport_Melee_NoTransitionToStart,			NULL				},
	{AI2cError_Melee_BlockAnimIndex,				AI2iReport_Melee_BlockAnimIndex,				NULL				},
	{AI2cError_Melee_BrokenTechnique,				AI2iReport_Melee_BrokenTechnique,				NULL				},
	{AI2cError_Melee_BrokenMove,					AI2iReport_Melee_BrokenMove,					NULL				},
	{AI2cError_Melee_BrokenAnim,					AI2iReport_Melee_BrokenAnim,					NULL				},
	{AI2cError_Melee_TransitionUnavailable,			AI2iReport_Melee_TransitionUnavailable,			NULL				},
	{AI2cError_Melee_MaxDelayTimers,				AI2iReport_Melee_MaxDelayTimers,				NULL				},
	{-1,											NULL,											NULL				}
};

AI2tErrorTableEntry AI2cErrorTable_Fight[] = {
	{AI2cError_Fight_MaxFights,						AI2iReport_Fight_MaxFights,						NULL				},
	{-1,											NULL,											NULL				}
};

AI2tErrorTableEntry *AI2cErrorTable_List[AI2cSubsystem_Max]
	= {AI2cErrorTable_None, AI2cErrorTable_Executor, AI2cErrorTable_Movement, AI2cErrorTable_Pathfinding,
		AI2cErrorTable_HighLevel, AI2cErrorTable_Patrol, AI2cErrorTable_Error, AI2cErrorTable_Knowledge,
		AI2cErrorTable_Combat, AI2cErrorTable_Melee, AI2cErrorTable_Fight};

// ------------------------------------------------------------------------------------
// -- initialization

UUtError AI2rError_Initialize(void)
{
#if defined(DEBUGGING) && DEBUGGING
	// report any AI problems that are encountered
	AI2rError_SetReportLevel(AI2cSubsystem_All, AI2cError);
#else
	// only report actual errors to the user
	// note: in the shipping version this is turned off by #define AI2_ERROR_REPORT 0 in Oni_AI2_Error.h
	AI2rError_SetReportLevel(AI2cSubsystem_All, AI2cBug);
#endif
	AI2rError_SetLogLevel(AI2cSubsystem_All, AI2cWarning);
	AI2rError_SetSilentHandling(AI2cSubsystem_All, UUcTrue);

	AI2gError_LogFileOpened = UUcFalse;
	AI2gError_LogFile = NULL;

	AI2gPathfindingHandler = NULL;

	// temporary debugging settings for individual levels go here

	return UUcError_None;
}

void AI2rError_Terminate(void)
{
}

// ------------------------------------------------------------------------------------
// -- top-level error control

// install a temporary error handler for pathfinding
AI2tPathfindingErrorHandler AI2rError_TemporaryPathfindingHandler(AI2tPathfindingErrorHandler inHandler)
{
	AI2tPathfindingErrorHandler previous_handler = AI2gPathfindingHandler;

	AI2gPathfindingHandler = inHandler;

	return previous_handler;
}

// remove the temporary error handler for pathfinding
void AI2rError_RestorePathfindingHandler(AI2tPathfindingErrorHandler inHandler)
{
	AI2gPathfindingHandler = inHandler;
}

// handle an error - can be used to catch problems before they blow up
UUtBool AI2rHandleError(AI2tErrorSubsystem inSystem, UUtUns32 inErrorID, ONtCharacter *inCharacter,
						UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4,
						AI2tErrorTableEntry **outEntry)
{
	AI2tErrorTableEntry *errorptr;
	UUtBool handled;

	if ((inSystem == AI2cSubsystem_Pathfinding) && AI2gIgnorePathError) {
		// this is not a real error! it's just being called as part of a report function
		return UUcFalse;
	}

	if ((inSystem < 0) || (inSystem >= AI2cSubsystem_Max) ||
		((errorptr = AI2cErrorTable_List[inSystem]) == NULL)) {
		// we can't find the error table for this subsystem!
		if (outEntry != NULL) {
			*outEntry = NULL;
		}
		return UUcFalse;
	}

	// look for the proper entry in the error table
	for ( ; errorptr->error_id != -1; errorptr++) {
		if (errorptr->error_id == inErrorID)
			break;
	}

	// store the entry for the error reporting function (if that is what called us)
	// note that this may not be the correct entry if we ran off the end of the table, but it will
	// detect this.
	if (outEntry != NULL) {
		*outEntry = errorptr;
	}

	if (inSystem == AI2cSubsystem_Pathfinding) {
		handled = UUcFalse;

		if (AI2gPathfindingHandler != NULL) {
			// use temporary pathfinding error handler
			handled = AI2gPathfindingHandler(inCharacter, inErrorID, inParam1, inParam2, inParam3, inParam4);
			if (handled)
				return UUcTrue;
		}

		// try to handle using goal-specific pathfinding error function
		switch (inCharacter->ai2State.currentGoal) {
			case AI2cGoal_Neutral:
				handled = AI2rNeutral_PathfindingHandler(inCharacter, inErrorID, inParam1, inParam2, inParam3, inParam4);
				break;

			case AI2cGoal_Patrol:
				handled = AI2rPatrol_PathfindingHandler(inCharacter, inErrorID, inParam1, inParam2, inParam3, inParam4);
				break;
		}
		if (handled)
			return UUcTrue;

		// try to handle using default error handler
		handled = AI2rError_DefaultPathfindingHandler(inCharacter, inErrorID, inParam1, inParam2, inParam3, inParam4);
		if (handled)
			return UUcTrue;	
	}

	if (errorptr->handle_function) {
		return (errorptr->handle_function)(inCharacter, inParam1, inParam2, inParam3, inParam4);
	} else {
		return UUcFalse;
	}
}

// report and handle an error - only called when running debug build of app
UUtBool AI2rReportError(AI2tErrorSubsystem inSystem, UUtUns32 inErrorID, ONtCharacter *inCharacter,
						UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4,
						const char *inFile, UUtUns32 inLine, AI2tErrorSeverity inSeverity)
{
	UUtBool handled;
	AI2tErrorTableEntry *error_entry;

	handled = AI2rHandleError(inSystem, inErrorID, inCharacter, inParam1, inParam2, inParam3, inParam4, &error_entry);

	if (error_entry == NULL) {
		// we could not even find the error table for this subsystem!
		AI2_ERROR(AI2cBug, AI2cSubsystem_Error, AI2cError_Error_InvalidSubsystem, NULL, inSystem, inFile, inLine, 0);
		return UUcFalse;

	} else if (error_entry->report_function == NULL) {
		// we could not find the error table entry that corresponds to this error id!
		AI2_ERROR(AI2cBug, AI2cSubsystem_Error, AI2cError_Error_UnknownError, NULL, inSystem, inErrorID, inFile, inLine);
		return UUcFalse;

	} else if ((inSeverity < 0) || (inSeverity >= AI2cSeverity_Max)) {
		// invalid severity value
		AI2_ERROR(AI2cBug, AI2cSubsystem_Error, AI2cError_Error_InvalidSeverity, NULL, inSeverity, inFile, inLine, 0);
		return UUcFalse;
	}


	if (handled && AI2gError_SilentHandling[inSystem])
		return handled;

	AI2gError_ReportThisError	= (inSeverity >= AI2gError_ReportLevel[inSystem]);
	AI2gError_LogThisError		= (inSeverity >= AI2gError_LogLevel[inSystem]);

	if (!(AI2gError_ReportThisError || AI2gError_LogThisError))
		return handled;

	// log the location and type of error to file (not to console)
	if (AI2gError_LogThisError) {
		sprintf(AI2gErrorBuf, "%s/%s: %s line %d from %s",
				AI2cErrorSeverityDesc[inSeverity], AI2cErrorSubsystemDesc[inSystem], inFile, inLine,
				(inCharacter == NULL) ? "(nobody)" : inCharacter->player_name);

		AI2rError_LogLine(AI2gErrorBuf);
	}

	// report the error
	(error_entry->report_function)(inCharacter, inParam1, inParam2, inParam3, inParam4);
	return handled;
}

void AI2rError_Line(char *inLine)
{
	AI2rError_ReportLine(inLine);
	AI2rError_LogLine(inLine);
}

void AI2rError_ReportLine(char *inLine)
{
	if (AI2gError_ReportThisError) {
		COrConsole_Printf_Color(COcPriority_Console, 0xFF30FF30, 0xFF307030, "%s", inLine);
	}

	return;
}

void AI2rError_LogLine(char *inLine)
{
	if (AI2gError_LogThisError) {
		if (!AI2gError_LogFileOpened) {
			AI2gError_LogFile = fopen("ai2_log.txt", "wt");

			if (AI2gError_LogFile == NULL) {
				AI2_ERROR(AI2cBug, AI2cSubsystem_Error, AI2cError_Error_OpenLog, NULL, 0, 0, 0, 0);
				return;
			}

			fprintf(AI2gError_LogFile, "AI2 Error Reporting Log\n");
			fprintf(AI2gError_LogFile, "=======================\n");
			fprintf(AI2gError_LogFile, "\n");

			AI2gError_LogFileOpened = UUcTrue;
		}

		fputs(inLine, AI2gError_LogFile);
		fputc('\n', AI2gError_LogFile);
	}
}

// ------------------------------------------------------------------------------------
// -- top-level error control

AI2tErrorSeverity AI2rError_GetSeverityFromString(char *inString)
{
	UUmAssertReadPtr(inString, sizeof(char));

	if (strcmp(inString, "bug") == 0) {
		return AI2cBug;

	} else if (strcmp(inString, "error") == 0) {
		return AI2cError;

	} else if (strcmp(inString, "warning") == 0) {
		return AI2cWarning;

	} else if (strcmp(inString, "status") == 0) {
		return AI2cStatus;

	} else {
		// default severity to report and log
		return AI2cError;
	}
}

AI2tErrorSubsystem AI2rError_GetSubsystemFromString(char *inString)
{
	UUmAssertReadPtr(inString, sizeof(char));

	if (strcmp(inString, "executor") == 0) {
		return AI2cSubsystem_Executor;

	} else if (strncmp(inString, "move", 4) == 0) {
		return AI2cSubsystem_Movement;

	} else if (strncmp(inString, "path", 4) == 0) {
		return AI2cSubsystem_Pathfinding;

	} else if (strcmp(inString, "highlevel") == 0) {
		return AI2cSubsystem_HighLevel;

	} else if (strcmp(inString, "patrol") == 0) {
		return AI2cSubsystem_Patrol;

	} else if (strcmp(inString, "error") == 0) {
		return AI2cSubsystem_Error;

	} else if (strncmp(inString, "know", 4) == 0) {
		return AI2cSubsystem_Knowledge;

	} else if (strcmp(inString, "combat") == 0) {
		return AI2cSubsystem_Combat;

	} else if (strcmp(inString, "melee") == 0) {
		return AI2cSubsystem_Melee;

	} else if (strcmp(inString, "fight") == 0) {
		return AI2cSubsystem_Fight;

	} else {
		return AI2cSubsystem_All;
	}
}

void AI2rError_SetReportLevel(AI2tErrorSubsystem inSystem, AI2tErrorSeverity inReportLevel)
{
	UUtUns32 itr;

	if ((inSystem < 0) || (inSystem >= AI2cSubsystem_Max)) {
		AI2_ERROR(AI2cBug, AI2cSubsystem_Error, AI2cError_Error_InvalidSubsystem, NULL, 0, 0, 0, 0);
		return;
	}
	
	if (inSystem == AI2cSubsystem_All) {
		for (itr = 0; itr < AI2cSubsystem_Max; itr++) {
			AI2gError_ReportLevel[itr] = inReportLevel;
		}
	} else {
		AI2gError_ReportLevel[inSystem] = inReportLevel;
	}
}

void AI2rError_SetLogLevel(AI2tErrorSubsystem inSystem, AI2tErrorSeverity inLogLevel)
{
	UUtUns32 itr;

	if ((inSystem < 0) || (inSystem >= AI2cSubsystem_Max)) {
		AI2_ERROR(AI2cBug, AI2cSubsystem_Error, AI2cError_Error_InvalidSubsystem, NULL, 0, 0, 0, 0);
		return;
	}
	
	if (inSystem == AI2cSubsystem_All) {
		for (itr = 0; itr < AI2cSubsystem_Max; itr++) {
			AI2gError_LogLevel[itr] = inLogLevel;
		}
	} else {
		AI2gError_LogLevel[inSystem] = inLogLevel;
	}
}

void AI2rError_SetSilentHandling(AI2tErrorSubsystem inSystem, UUtBool inSilentHandling)
{
	UUtUns32 itr;

	if ((inSystem < 0) || (inSystem >= AI2cSubsystem_Max)) {
		AI2_ERROR(AI2cBug, AI2cSubsystem_Error, AI2cError_Error_InvalidSubsystem, NULL, 0, 0, 0, 0);
		return;
	}
	
	if (inSystem == AI2cSubsystem_All) {
		for (itr = 0; itr < AI2cSubsystem_Max; itr++) {
			AI2gError_SilentHandling[itr] = inSilentHandling;
		}
	} else {
		AI2gError_SilentHandling[inSystem] = inSilentHandling;
	}
}

// ------------------------------------------------------------------------------------
// -- error reporting

static void AI2iReport_All_Report(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	AI2rError_Line((char *) inParam1);
}

static void AI2iReport_Error_OpenLog(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	AI2rError_Line("cannot open AI2 error reporting log file!");
}

static void AI2iReport_Error_InvalidSubsystem(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	if (inParam2 == 0) {
		sprintf(AI2gErrorBuf, "AI2 subsystem index %d is not handled!", inParam1);
	} else {
		sprintf(AI2gErrorBuf, "error reported from %s line %d with unknown AI2 subsystem index %d [range 0-%d]",
				(char *) inParam2, inParam3, inParam1, AI2cSubsystem_Max);
	}
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Error_InvalidSeverity(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "error reported from %s line %d with unknown AI2 severity level %d [range 0-%d]",
			(char *) inParam2, inParam3, inParam1, AI2cSeverity_Max);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Error_UnknownError(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "%s error reported from %s line %d with unknown error ID %d",
			AI2cErrorSubsystemDesc[inParam1], (char *) inParam3, inParam4, inParam2);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Executor_TurnOverride(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "%s executing turn override from %f to %f",
			inCharacter->player_name, *((float *) inParam1), *((float *) inParam2));
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Executor_UnexpectedAttack(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	TRtAnimation *anim = (TRtAnimation *) inParam3;

	sprintf(AI2gErrorBuf, "%s: executed attack of type %d (%s) but was instead trying for type %d",
			inCharacter->player_name, inParam2, TMrInstance_GetInstanceName(anim), inParam1);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_HighLevel_UnknownJob(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "%s: unknown job mode %d (range 0-%d)", inCharacter->player_name, inParam1, AI2cGoal_Max);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_HighLevel_ImproperJob(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	if ((inParam1 < 0) || (inParam1 >= AI2cGoal_Max)) {
		AI2_ERROR(AI2cBug, AI2cSubsystem_HighLevel, AI2cError_HighLevel_UnknownJob, inCharacter, inParam1, 0, 0, 0);
		return;
	}

	sprintf(AI2gErrorBuf, "%s: goal %d '%s' should never be assigned as a job",
			inCharacter->player_name, inParam1, AI2cGoalName[inParam1]);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_HighLevel_CombatSettings(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "%s: can't find combat settings (ID %d)", inCharacter->player_name, inParam1);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_HighLevel_NoPath(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "%s: can't find patrol path (ID %d)", inCharacter->player_name, inParam1);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_HighLevel_NoMeleeProfile(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "%s: can't find melee profile (ID %d)", inCharacter->player_name, inParam1);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_HighLevel_MeleeVariant(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	AI2tMeleeProfile *profile = (AI2tMeleeProfile *) inParam2;
	ONtCharacterClass *char_class = (ONtCharacterClass *) inParam3;
	char *melee_classname, *char_classname;

	melee_classname = TMrInstance_GetInstanceName(profile->char_class);
	char_classname = TMrInstance_GetInstanceName(char_class);

	sprintf(AI2gErrorBuf, "%s: melee profile %d (%s) is for class %s variant %s, doesn't match character class %s variant %s",
			inCharacter->player_name, inParam1, profile->name,
			melee_classname, profile->char_class->variant->name, char_classname, char_class->variant->name);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_HighLevel_NoCharacter(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	char *command_name = (char *) inParam1;
	SLtParameter_Actual *parameter = (SLtParameter_Actual *) inParam2;

	switch(parameter->type) {
		case SLcType_Int32:
			sprintf(AI2gErrorBuf, "%s: no living character with script ID %d", command_name, parameter->val.i);
			break;

		case SLcType_String:
			sprintf(AI2gErrorBuf, "%s: no living character named '%s'", command_name, parameter->val.str);
			break;

		case SLcType_Float:
			sprintf(AI2gErrorBuf, "%s: floating-point parameter %f cannot be used as a character!", command_name, parameter->val.f);
			break;

		case SLcType_Bool:
			sprintf(AI2gErrorBuf, "%s: boolean parameter %s cannot be used as a character!", command_name, parameter->val.b ? "true" : "false");
			break;

		default:
			UUmAssert(!"AI2iReport_HighLevel_NoCharacter: unknown SLtParameter type!");
			sprintf(AI2gErrorBuf, "%s: internal scripting language error!", command_name);
			break;
	}

	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_HighLevel_TalkBufferFull(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	char *sound_name = (char *) inParam2;

	sprintf(AI2gErrorBuf, "%s: overflowed talk buffer size AI2cScript_TalkBufferSize = %d trying to play speech '%s'",
			inCharacter->player_name, inParam1, sound_name);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_HighLevel_NoNeutralBehavior(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "%s: can't find neutral behavior (ID %d)", inCharacter->player_name, inParam1);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Pathfinding_NoBNVAtStart(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	M3tPoint3D *point = (M3tPoint3D *) inParam1;

	sprintf(AI2gErrorBuf, "%s: no BNV at current location(%f %f %f)",
			inCharacter->player_name, point->x, point->y, point->z);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Pathfinding_NoBNVAtDest(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	M3tPoint3D *point = (M3tPoint3D *) inParam2;

	sprintf(AI2gErrorBuf, "%s: no BNV at destination(%f %f %f)",
			inCharacter->player_name, point->x, point->y, point->z);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Pathfinding_NoConnectionsFromStart(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	M3tPoint3D *point = (M3tPoint3D *) inParam1;

	sprintf(AI2gErrorBuf, "%s: no connections out of current PHtNode (%f %f %f)",
			inCharacter->player_name, point->x, point->y, point->z);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Pathfinding_NoConnectionsToDest(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	M3tPoint3D *point = (M3tPoint3D *) inParam2;

	sprintf(AI2gErrorBuf, "%s: no connections into destination PHtNode (%f %f %f)",
			inCharacter->player_name, point->x, point->y, point->z);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Pathfinding_NoPath(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "%s: failed to generate path through large-scale graph",
			inCharacter->player_name);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Pathfinding_AStarFailed(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	M3tPoint3D *start = (M3tPoint3D *) inParam1, *end = (M3tPoint3D *) inParam2;
	IMtPoint2D start_grid, end_grid;
	PHtRoomData *room = (PHtRoomData *) inParam3;
	AStPath *path = (AStPath *) inParam4;

	ASrDebugGetGridPoints(path, &start_grid, &end_grid);
	sprintf(AI2gErrorBuf, "%s: A* failed to generate grid path from (%f %f %f)/(%d,%d) to (%f %f %f)/(%d,%d) [grid %dx%d]",
			inCharacter->player_name,
			start->x, start->y, start->z, start_grid.x, start_grid.y,
			end->x, end->y, end->y, end_grid.x, end_grid.y, room->gridX, room->gridY);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Pathfinding_CollisionStuck(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "%s: colliding against wall, AI may be stuck", inCharacter->player_name);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Pathfinding_FellFromRoom(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "%s: AI fell from path (found itself in wrong room)", inCharacter->player_name);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Pathfinding_FellFromPath(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "%s: AI fell from path (somehow not actually following path points)", inCharacter->player_name);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Patrol_InvalidEntry(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	AI2tPatrolState *patrolstate = (AI2tPatrolState *) inParam3;

	sprintf(AI2gErrorBuf, "%s: attempted to enter patrol %s at waypoint %d [length %d]",
			inCharacter->player_name, patrolstate->path.name, inParam1, inParam2);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Patrol_BeginPatrol(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	AI2tPatrolState *patrolstate = (AI2tPatrolState *) inParam1;

	sprintf(AI2gErrorBuf, "%s: begin patrol %s", inCharacter->player_name, patrolstate->path.name);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Patrol_AtWaypoint(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	AI2tPatrolState *patrolstate = (AI2tPatrolState *) inParam3;
	AI2tWaypoint *waypoint = (AI2tWaypoint *) inParam2;

	sprintf(AI2gErrorBuf, "%s: patrolling %s: at waypoint %d which is:", inCharacter->player_name,
			patrolstate->path.name, inParam1);
	AI2rError_Line(AI2gErrorBuf);

	switch(waypoint->type)
	{
		case AI2cWaypointType_LookInDirection:
			sprintf(AI2gErrorBuf, "  lock-facing %s", AI2cMovementDirectionName[waypoint->data.lookInDirection.facing]);
		break;

		case AI2cWaypointType_MoveToFlag:
			sprintf(AI2gErrorBuf, "  move to flag %d", waypoint->data.moveToFlag.flag);
		break;

		case AI2cWaypointType_Stop:
			sprintf(AI2gErrorBuf, "  stop");
		break;

		case AI2cWaypointType_Pause:
			sprintf(AI2gErrorBuf, "  pause for %d frames", waypoint->data.pause.count);
		break;

		case AI2cWaypointType_LookAtFlag:
			sprintf(AI2gErrorBuf, "  look at flag %d", waypoint->data.lookAtFlag.lookFlag);
		break;

		case AI2cWaypointType_LookAtPoint:
			sprintf(AI2gErrorBuf, "  look at point (%f %f %f)",
				waypoint->data.lookAtPoint.lookPoint.x,
				waypoint->data.lookAtPoint.lookPoint.y,
				waypoint->data.lookAtPoint.lookPoint.z);
		break;

		case AI2cWaypointType_MoveAndFaceFlag:
			sprintf(AI2gErrorBuf, "  move and face flag %d", waypoint->data.moveAndFaceFlag.faceFlag);
		break;

		case AI2cWaypointType_Loop:
			sprintf(AI2gErrorBuf, "  loop");
		break;

		case AI2cWaypointType_MovementMode:
			sprintf(AI2gErrorBuf, "  movement mode %s", AI2cMovementModeName[waypoint->data.movementMode.newMode]);
		break;

		case AI2cWaypointType_MoveToPoint:
			sprintf(AI2gErrorBuf, "  move to point (%f %f %f)", 
				waypoint->data.moveToPoint.point.x,
				waypoint->data.moveToPoint.point.y,
				waypoint->data.moveToPoint.point.z);
		break;

		case AI2cWaypointType_MoveThroughFlag:
			sprintf(AI2gErrorBuf, "  move through flag %d (dist %f)", 
				waypoint->data.moveThroughFlag.flag,
				waypoint->data.moveThroughFlag.required_distance);
		break;

		case AI2cWaypointType_MoveThroughPoint:
			sprintf(AI2gErrorBuf, "  move through point (%f %f %f) (dist %f)",
				waypoint->data.moveThroughPoint.point.x,
				waypoint->data.moveThroughPoint.point.y,
				waypoint->data.moveThroughPoint.point.z,
				waypoint->data.moveThroughPoint.required_distance);
		break;

		case AI2cWaypointType_StopLooking:
			sprintf(AI2gErrorBuf, "  stop looking");
		break;

		case AI2cWaypointType_FreeFacing:
			sprintf(AI2gErrorBuf, "  free-facing");
		break;

		case AI2cWaypointType_GlanceAtFlag:
			sprintf(AI2gErrorBuf, "  glance(%d frames) at flag %d", waypoint->data.glanceAtFlag.glanceFrames,
						waypoint->data.glanceAtFlag.glanceFlag);
		break;

		default:
			UUmAssert(!"AI2iReport_Patrol_AtWaypoint: unknown waypoint type");
		break;
	}
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Patrol_NoSuchFlag(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	AI2tPatrolState *patrolstate = (AI2tPatrolState *) inParam3;

	sprintf(AI2gErrorBuf, "%s: patrolling %s: no such flag %d (at waypoint %d)", inCharacter->player_name,
			patrolstate->path.name, inParam1, inParam2);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Patrol_FailedWaypoint(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	AI2tPatrolPath *path = (AI2tPatrolPath *) inParam1;

	sprintf(AI2gErrorBuf, "%s: patrolling %s (ID %d): pathfinding failure, skipping waypoint #%d of %d (tried %d times to reach it)", inCharacter->player_name,
			path->name, path->id_number, inParam2 + 1, inParam3, inParam4);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Knowledge_MaxEntries(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "%s: unable to add knowledge, exceeded AI2cKnowledge_MaxEntries %d", inCharacter->player_name,
			inParam1);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Knowledge_NoStartle(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	char *startle_direction;

	switch(inParam1) {
	case ONcAnimType_Startle_Forward:
		startle_direction = "forward";
		break;

	case ONcAnimType_Startle_Left:
		startle_direction = "left";
		break;

	case ONcAnimType_Startle_Right:
		startle_direction = "right";
		break;

	case ONcAnimType_Startle_Back:
		startle_direction = "back";
		break;

	default:
		startle_direction = "<error>";
		break;
	}

	sprintf(AI2gErrorBuf, "%s (variant %s): no %s startle found", inCharacter->player_name,
			inCharacter->characterClass->variant->name, startle_direction);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Knowledge_MaxPending(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "%s: unable to post knowledge, exceeded AI2cKnowledge_MaxPending %d for this frame", inCharacter->player_name,
			inParam1);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Knowledge_MaxDodgeProjectiles(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "AI unable to create dodge projectile entry, exceeded AI2cKnowledge_MaxDodgeProjectiles %d", inParam1);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Knowledge_MaxDodgeFiringSpreads(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "AI unable to create dodge firing spread entry, exceeded AI2cKnowledge_MaxDodgeFiringSpreads %d", inParam1);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Movement_InvalidAlertStatus(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "%s: unable to find movement mode for unknown alert status %d",
			inCharacter->player_name, inParam1);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Movement_MaxModifiers(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "%s: exceeded max movement modifiers trying to add modifier %d: dir %f weight %f",
			inCharacter->player_name, inParam1, *((float *) inParam2), *((float *) inParam3));
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Movement_MaxCollisions(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	AI2tCollisionIdentifier *collision_id = (AI2tCollisionIdentifier *) inParam2;
	M3tPoint3D *point = (M3tPoint3D *) inParam3;
	M3tPlaneEquation *plane = (M3tPlaneEquation *) inParam4;

	sprintf(AI2gErrorBuf, "%s: exceeded max movement modifiers trying to add collision %d: point (%f %f %f), plane (%f %f %f / %f)",
					inCharacter->player_name, inParam1, point->x, point->y, point->z, plane->a, plane->b, plane->c, plane->d);
	AI2rError_Line(AI2gErrorBuf);

	switch(collision_id->type) {
		case AI2cCollisionType_Wall:
			sprintf(AI2gErrorBuf, "    was a wall collision with gq #%d, plane #%d", collision_id->data.wall.gq_index, collision_id->data.wall.plane_index);
		break;

		case AI2cCollisionType_Physics:
			sprintf(AI2gErrorBuf, "    was a physics collision with type %d", collision_id->data.physics_context->callback->type);
		break;

		case AI2cCollisionType_Character:
			sprintf(AI2gErrorBuf, "    was a character collision with %s", collision_id->data.character->player_name);
		break;

		default:
			sprintf(AI2gErrorBuf, "    was a collision of unknown type %d", collision_id->type);
		break;
	}
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Movement_Collision(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	AI2tCollisionIdentifier *collision_id = (AI2tCollisionIdentifier *) inParam1;
	M3tPoint3D *point = (M3tPoint3D *) inParam2;
	M3tPlaneEquation *plane = (M3tPlaneEquation *) inParam3;
	float *dir = (float *) inParam4;

	sprintf(AI2gErrorBuf, "%s: %u collision at point (%f %f %f), plane (%f %f %f / %f), dir %f",
					inCharacter->player_name, inParam1, point->x, point->y, point->z, plane->a, plane->b, plane->c, plane->d, *dir);
	AI2rError_Line(AI2gErrorBuf);

	switch(collision_id->type) {
		case AI2cCollisionType_Wall:
			sprintf(AI2gErrorBuf, "    is a wall collision with gq #%d, plane #%d", collision_id->data.wall.gq_index, collision_id->data.wall.plane_index);
		break;

		case AI2cCollisionType_Physics:
			sprintf(AI2gErrorBuf, "    is a physics collision with type %d", collision_id->data.physics_context->callback->type);
		break;

		case AI2cCollisionType_Character:
			sprintf(AI2gErrorBuf, "    is a character collision with %s", collision_id->data.character->player_name);
		break;

		default:
			sprintf(AI2gErrorBuf, "    is a collision of unknown type %d", collision_id->type);
		break;
	}
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Movement_CollisionStalled(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	AI2tCollision *collision = (AI2tCollision *) inParam2;

	sprintf(AI2gErrorBuf, "%s: collision stallled - %d frames", inCharacter->player_name, inParam1);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Movement_MaxBadnessValues(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "%s: can't add new badness value (max %d)", inCharacter->player_name, inParam1);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Knowledge_InvalidContact(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "%s: contact of unknown type (%d)", inCharacter->player_name, inParam1);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Combat_UnknownBehavior(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "%s: behavior at range %d is an unknown type (%d)", inCharacter->player_name, inParam1, inParam2);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Combat_UnimplementedBehavior(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "%s: behavior %s (type %d) at range %d is not yet implemented, sorry",
				inCharacter->player_name, (char *) inParam3, inParam1, inParam2);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Combat_UnhandledMessage(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	UUtUns32 behavior_type = (UUtUns32) inCharacter->ai2State.currentState->state.combat.current_behavior.type;

	sprintf(AI2gErrorBuf, "%s: behavior %d (%s) didn't handle message %d (params %d %d %d)",
			inCharacter->player_name, behavior_type, ((behavior_type >= 0) && (behavior_type < AI2cBehavior_Max))
			? AI2cBehaviorName[behavior_type] : "<error>", inParam1, inParam2, inParam3, inParam4);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Combat_TargetDead(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "%s: target %s is dead but we weren't notified",
			inCharacter->player_name, ((ONtCharacter *) inParam1)->player_name);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Combat_ContactDisappeared(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	ONtCharacter *target = (ONtCharacter *) inParam1;

	sprintf(AI2gErrorBuf, "%s: knowledge entry for target %s disappeared unexpectedly",
		inCharacter->player_name, (target == NULL) ? "NULL" : target->player_name);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Melee_AttackMoveNotInCombo(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "melee attack %d [anim type %d] is not in combo table", inParam1, inParam2);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Melee_NoCharacterClass(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "character class '%s' not found when loading melee profile ID %d (%s)",
			(char *) inParam1, inParam2, (char *) inParam3);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Melee_PreComputeBroken(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	AI2tMeleeProfile *profile = (AI2tMeleeProfile *) inParam1;
	ONtCharacterClass *new_class = (ONtCharacterClass *) inParam2;
	AI2tMeleeMove *move = (AI2tMeleeMove *) inParam4;
	AI2tMeleeTechnique *technique;
	char *anim_name, *newclassname, *oldclassname, techniquebuf[96];
	UUtUns32 itr, move_index;

	move_index = move - profile->move;
	if ((move_index < 0) || (move_index >= profile->num_moves)) {
		sprintf(techniquebuf, "<error moveindex=%d [validrange 0,%d)>", move_index, profile->num_moves);
	} else {
		for (itr = 0, technique = profile->technique; itr < profile->num_actions + profile->num_reactions;
			itr++, technique++) {
			if ((move_index >= technique->start_index) && (move_index < technique->start_index + technique->num_moves)) {
				break;
			}
		}

		if (itr >= profile->num_actions + profile->num_reactions) {
			sprintf(techniquebuf, "<error moveindex=%d outside all techniques>", move_index);
		} else if (itr >= profile->num_actions) {
			sprintf(techniquebuf, "technique reaction %d(%s) move %d", 
					itr - profile->num_actions, technique->name, move_index - technique->start_index);
		} else {
			sprintf(techniquebuf, "technique action %d(%s) move %d", 
					itr, technique->name, move_index - technique->start_index);
		}
	}

	anim_name = (move->animation == NULL) ? "<null>" : TMrInstance_GetInstanceName(move->animation);
	newclassname = TMrInstance_GetInstanceName(new_class);

	if ((profile->char_class == NULL) || (new_class == profile->char_class)) {
		sprintf(AI2gErrorBuf, "melee profile %s: class '%s' %s can't do anim_type %d from state %d [prev anim %s]",
				profile->name, newclassname, techniquebuf, inParam3, move->current_state, anim_name);
	} else {
		oldclassname = TMrInstance_GetInstanceName(profile->char_class);

		sprintf(AI2gErrorBuf, "melee profile %s: changing from class '%s' to class '%s': %s can't do anim_type %d from state %d [prev anim %s]",
				profile->name, oldclassname, newclassname, techniquebuf, inParam3, move->current_state, anim_name);
	}
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Melee_MultiDirectionTechnique(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	AI2tMeleeProfile *profile = (AI2tMeleeProfile *) inParam1;
	char *technique_name = (char *) inParam2;

	sprintf(AI2gErrorBuf, "melee profile %s: technique '%s' direction flags 0x%08X don't match supplied dir 0x%08X",
			profile->name, technique_name, inParam3, inParam4);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Melee_MaxTransitions(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	AI2tMeleeProfile *profile = (AI2tMeleeProfile *) inParam1;

	sprintf(AI2gErrorBuf, "melee profile %s: there are %d transitions, exceeded AI2cMeleeProfile_MaxTransitions (%d)",
			profile->name, inParam2, inParam3);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Melee_Failed_Attack(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "%s: failing attack %s (current anims %s, %s, current state %s)",
			inCharacter->player_name, ONrAnimTypeToString((UUtUns16) inParam1), ONrAnimTypeToString((UUtUns16) inParam2),
			ONrAnimTypeToString((UUtUns16) inParam3), ONrAnimStateToString((UUtUns16) inParam4));
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Melee_NoTransitionToDelay(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	AI2tMeleeProfile *profile = (AI2tMeleeProfile *) inParam1;

	sprintf(AI2gErrorBuf, "%s: can't find transition from %s to delaystate %s (animstate %s)",
			profile->name, AI2cMeleeMoveStateName[inParam2], AI2cMeleeMoveStateName[inParam3], ONrAnimStateToString((UUtUns16) inParam4));
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Melee_NoTransitionOutOfDelay(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	AI2tMeleeProfile *profile = (AI2tMeleeProfile *) inParam1;

	sprintf(AI2gErrorBuf, "%s: can't find transition out of delaystate %s (animstate %s) to %s",
			profile->name, AI2cMeleeMoveStateName[inParam2], ONrAnimStateToString((UUtUns16) inParam4), AI2cMeleeMoveStateName[inParam3]);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Melee_NoDelayAnimation(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	AI2tMeleeProfile *profile = (AI2tMeleeProfile *) inParam1;

	sprintf(AI2gErrorBuf, "%s: trying to find animation for delaystate %s (animstate %s): animtype %s - fail",
			profile->name, AI2cMeleeMoveStateName[inParam2], ONrAnimStateToString((UUtUns16) inParam4), ONrAnimTypeToString((UUtUns16) inParam3));
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Melee_NoTransitionToStart(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	AI2tMeleeTechnique *technique = (AI2tMeleeTechnique *) inParam1;

	sprintf(AI2gErrorBuf, "%s: technique %s: can't find transition from current state %s (animstate %s) to start state %s",
			inCharacter->player_name, technique->name, AI2cMeleeMoveStateName[inParam2], ONrAnimStateToString((UUtUns16) inParam4),
			AI2cMeleeMoveStateName[inParam3]);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Melee_BlockAnimIndex(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	ONtCharacter *attacker = (ONtCharacter *) inParam1;

	sprintf(AI2gErrorBuf, "%s: blocking character %s: hit by anim %d != expected anim %d",
			inCharacter->player_name, attacker->player_name, inParam2, inParam3);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Melee_BrokenTechnique(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	AI2tMeleeState *melee_state = (AI2tMeleeState *) inParam1;
	AI2tMeleeTechnique *technique = (AI2tMeleeTechnique *) inParam2;

	sprintf(AI2gErrorBuf, "%s: melee technique %s is broken", inCharacter->player_name, technique->name);
}

static void AI2iReport_Melee_BrokenMove(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	AI2tMeleeState *melee_state = (AI2tMeleeState *) inParam1;
	AI2tMeleeTechnique *technique = (AI2tMeleeTechnique *) inParam2;
	AI2tMeleeMove *move = (AI2tMeleeMove *) inParam3;

	sprintf(AI2gErrorBuf, "%s: tried to execute broken move %d of technique %s",
		inCharacter->player_name, melee_state->move_index + 1, technique->name);
}

static void AI2iReport_Melee_BrokenAnim(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	AI2tMeleeState *melee_state = (AI2tMeleeState *) inParam1;
	AI2tMeleeTechnique *technique = (AI2tMeleeTechnique *) inParam2;
	AI2tMeleeProfile *profile = (AI2tMeleeProfile *) inParam4;
	ONtCharacterVariant *our_variant = inCharacter->characterClass->variant, *profile_variant = profile->char_class->variant;

	sprintf(AI2gErrorBuf, "%s: considering technique %s - no animation found for move %d! (our class %s, profile %s for %s)",
		inCharacter->player_name, technique->name, inParam3 + 1, our_variant ? our_variant->name : "NULL",
		profile->name, profile_variant ? profile_variant->name : "NULL");
}

static void AI2iReport_Melee_TransitionUnavailable(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	AI2tMeleeProfile *profile = (AI2tMeleeProfile *) inParam1;
	UUtUns32 itr, num_alternatives = inParam2;
	AI2tMeleeTransitionDefinition *transition_def = (AI2tMeleeTransitionDefinition *) inParam3;

	sprintf(AI2gErrorBuf, "%s: couldn't get any transition from %s[%s] to %s, tried animtypes (",
		profile->name, AI2cMeleeMoveStateName[transition_def->from_state],
		ONrAnimStateToString(AI2cMelee_TotoroAnimState[transition_def->from_state]),
		AI2cMeleeMoveStateName[transition_def->to_state[0]]);

	for (itr = 0; itr < num_alternatives; itr++) {
		if (itr > 0) {
			strcat(AI2gErrorBuf, ", ");
		}
		strcat(AI2gErrorBuf, ONrAnimTypeToString(transition_def[itr].anim_type));
	}
	strcat(AI2gErrorBuf, ")");
}

static void AI2iReport_Melee_MaxDelayTimers(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	AI2tMeleeProfile *profile = (AI2tMeleeProfile *) inParam1;

	sprintf(AI2gErrorBuf, "melee profile %s: there are %d techniques with delay timers, exceeded AI2cMeleeMaxDelayTimers (%d)",
			profile->name, inParam2, inParam3);
	AI2rError_Line(AI2gErrorBuf);
}

static void AI2iReport_Fight_MaxFights(ONtCharacter *inCharacter,
									 UUtUns32 inParam1, UUtUns32 inParam2, UUtUns32 inParam3, UUtUns32 inParam4)
{
	sprintf(AI2gErrorBuf, "%s: exceeded AI2cFight_MaxFights %d, cannot run spacing behavior",
			inCharacter->player_name, inParam1);
	AI2rError_Line(AI2gErrorBuf);
}

// ------------------------------------------------------------------------------------
// -- pathfinding error handling

UUtBool AI2rError_DefaultPathfindingHandler(ONtCharacter *inCharacter, UUtUns32 inErrorID,
											UUtUns32 inParam1, UUtUns32 inParam2,
											UUtUns32 inParam3, UUtUns32 inParam4)
{
#if TOOL_VERSION
	switch(inErrorID) {
		case AI2cError_Pathfinding_NoBNVAtStart:
			AI2gPathfindingErrorStorage.no_bnv = UUcTrue;
			AI2gPathfindingErrorStorage.bnv_test_location = *((M3tPoint3D *) inParam1);
			break;

		case AI2cError_Pathfinding_NoBNVAtDest:
			AI2gPathfindingErrorStorage.no_bnv = UUcTrue;
			AI2gPathfindingErrorStorage.bnv_test_location = *((M3tPoint3D *) inParam2);
			break;

		case AI2cError_Pathfinding_NoConnectionsFromStart:
		case AI2cError_Pathfinding_NoConnectionsToDest:
		case AI2cError_Pathfinding_NoPath:
			{
				PHtNode *node_start = (PHtNode *) inParam3;
				PHtNode *node_end = (PHtNode *) inParam4;

				AI2gPathfindingErrorStorage.no_path = UUcTrue;
				if (node_start == NULL) {
					MUmVector_Copy(AI2gPathfindingErrorStorage.path_start, *((M3tPoint3D *) inParam1));
				} else {
					MUmVector_Add(AI2gPathfindingErrorStorage.path_start, node_start->location->roomData.origin, node_start->location->roomData.antiOrigin);
					MUmVector_Scale(AI2gPathfindingErrorStorage.path_start, 0.5);
				}
				if (node_end == NULL) {
					MUmVector_Copy(AI2gPathfindingErrorStorage.path_end, *((M3tPoint3D *) inParam2));
				} else {
					MUmVector_Add(AI2gPathfindingErrorStorage.path_end,   node_end  ->location->roomData.origin, node_end  ->location->roomData.antiOrigin);
					MUmVector_Scale(AI2gPathfindingErrorStorage.path_end, 0.5);
				}

				if ((node_start != NULL) && (node_end != NULL)) {
					PHrDebugGraph_TraverseConnections(ONrGameState_GetGraph(), node_start, node_end, 10);
				}
			}
			break;
			
		case AI2cError_Pathfinding_AStarFailed:
			AI2gPathfindingErrorStorage.no_astar = UUcTrue;
			AI2gPathfindingErrorStorage.astar_start = *((M3tPoint3D *) inParam1);
			AI2gPathfindingErrorStorage.astar_end = *((M3tPoint3D *) inParam2);
			AI2gPathfindingErrorStorage.astar_room = (PHtRoomData *) inParam3;
			AI2gPathfindingErrorStorage.astar_node = (PHtNode *) inParam4;
			
			ASrGetLocalStart(&AI2gPathfindingErrorStorage.astar_gridstart);
			ASrGetLocalEnd(&AI2gPathfindingErrorStorage.astar_gridend);
			break;

		case AI2cError_Pathfinding_CollisionStuck:
			{
				ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);
				UUtUns32 itr;

				UUmAssert(active_character);
				if (active_character) {
					AI2gPathfindingErrorStorage.collision_stuck = UUcTrue;
					ONrCharacter_GetPathfindingLocation(inCharacter, &AI2gPathfindingErrorStorage.collision_point);

					AI2gPathfindingErrorStorage.numCollisions = active_character->movement_state.numCollisions;
					for (itr = 0; itr < active_character->movement_state.numCollisions; itr++) {
						AI2gPathfindingErrorStorage.collision[itr] = active_character->movement_state.collision[itr];
					}

					AI2gPathfindingErrorStorage.numBadnessValues = active_character->movement_state.numBadnessValues;
					for (itr = 0; itr < active_character->movement_state.numBadnessValues; itr++) {
						AI2gPathfindingErrorStorage.badnesslist[itr] = active_character->movement_state.badnesslist[itr];
					}
				}
			}
			break;

		case AI2cError_Pathfinding_FellFromRoom:
			// FIXME: no debugging display
			break;

		case AI2cError_Pathfinding_FellFromPath:
			// FIXME: no debugging_display
			break;

		default:
			UUmAssert(0);
			break;
	}
#endif

	return UUcFalse;
}

void AI2rError_ClearPathfindingErrors(void)
{
	UUrMemory_Clear(&AI2gPathfindingErrorStorage, sizeof(AI2tPathfindingErrorStorage));
}

void AI2rError_DisplayPathfindingErrors(void)
{
	if (AI2gPathfindingErrorStorage.no_bnv) {
		M3tPoint3D p0, p1, p2;

		p0 = AI2gPathfindingErrorStorage.bnv_test_location;

		p1 = p2 = p0;
		p1.x += 4.0f;
		p2.x -= 4.0f;
		M3rGeom_Line_Light(&p1, &p2, IMcShade_Pink);

		p1 = p2 = p0;
		p1.y += 4.0f;
		p2.y -= 4.0f;
		M3rGeom_Line_Light(&p1, &p2, IMcShade_Pink);

		p1 = p2 = p0;
		p1.z += 4.0f;
		p2.z -= 4.0f;
		M3rGeom_Line_Light(&p1, &p2, IMcShade_Pink);
	}

	if (AI2gPathfindingErrorStorage.no_path) {
		M3rGeom_Line_Light(&AI2gPathfindingErrorStorage.path_start, &AI2gPathfindingErrorStorage.path_end, IMcShade_Pink);
	}

	if (AI2gPathfindingErrorStorage.no_astar) {
		M3rGeom_Line_Light(&AI2gPathfindingErrorStorage.astar_start, &AI2gPathfindingErrorStorage.astar_end, IMcShade_Pink);

		PHrRenderGrid(AI2gPathfindingErrorStorage.astar_node, AI2gPathfindingErrorStorage.astar_room,
					  &AI2gPathfindingErrorStorage.astar_gridstart, &AI2gPathfindingErrorStorage.astar_gridend);
	}

	if (AI2gPathfindingErrorStorage.collision_stuck) {
		AI2rMovementState_RenderCollision(&AI2gPathfindingErrorStorage.collision_point, AI2gPathfindingErrorStorage.numCollisions,
											AI2gPathfindingErrorStorage.collision, AI2gPathfindingErrorStorage.numBadnessValues,
											AI2gPathfindingErrorStorage.badnesslist);
	}
}
