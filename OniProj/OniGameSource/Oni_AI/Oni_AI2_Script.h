#pragma once
#ifndef ONI_AI2_SCRIPT_H
#define ONI_AI2_SCRIPT_H

/*
	FILE:	Oni_AI2_Script.h

	AUTHOR:	Chris Butcher

	CREATED: April 04, 2000

	PURPOSE: AI interface to the scripting system for Oni

	Copyright (c) Bungie Software, 2000

*/

#include "BFW.h"
#include "BFW_ScriptLang.h"

// ------------------------------------------------------------------------------------
// -- globals

extern UUtBool AI2gShowPathfindingErrors;
extern UUtBool AI2gShowPathfindingGrids;

extern UUtBool AI2gBlind;

extern UUtBool AI2gDeaf;

extern UUtBool AI2gDrawLaserSights;

extern UUtBool AI2gDebug_ShowPrediction;

extern UUtBool AI2gDebug_ShowTargeting;

extern UUtBool AI2gShowNames;
extern UUtBool AI2gShowCombatRanges;
extern UUtBool AI2gShowVisionCones;
extern UUtBool AI2gShowHealth;
extern UUtBool AI2gShowJobLocations;
extern UUtBool AI2gShowLineToChar;
extern UUtBool AI2gShowProjectiles;
extern UUtBool AI2gShowFiringSpreads;
extern UUtBool AI2gShowSounds;
extern UUtBool AI2gShowFights;

extern UUtBool AI2gIgnorePlayer;

extern UUtBool AI2gEveryonePassive;

extern UUtBool AI2gChumpStop;

extern UUtBool AI2gDebug_LocalMovement;
extern UUtBool AI2gDebug_LocalPathfinding;
extern UUtBool AI2gDebugDisplayGraph;
extern UUtBool AI2gDebug_ShowActivationPaths;
extern UUtBool AI2gDebug_ShowAstarEvaluation;
extern UUtBool AI2gDebug_ShowLOS;

extern UUtBool AI2gDebugShowSettingIDs;
extern UUtBool AI2gDebug_ShowLocalMelee;

extern UUtBool AI2gBarabbasRun;
extern UUtBool AI2gBossBattle;

extern UUtBool AI2gMeleeWeightCorrection;
extern UUtBool AI2gSpacingEnable;
extern UUtInt32 AI2gSpacingMaxCookies;
extern UUtBool AI2gSpacingWeights;

extern UUtInt32 AI2gSound_StopIgnoring_Events;
extern UUtInt32 AI2gSound_StopIgnoring_Time;

// ------------------------------------------------------------------------------------
// -- structures and typedefs

// function to write a report to
typedef void (*AI2tReportFunction)(char *inLine);

// ------------------------------------------------------------------------------------
// -- function prototypes

UUtError AI2rInstallConsoleVariables(void);

struct ONtCharacter *AI2rScript_ParseCharacter(SLtParameter_Actual *inParameter, UUtBool inExhaustive);

void AI2rScript_NoCharacterError(SLtParameter_Actual *inParameter, char *inName, SLtErrorContext *ioContext);

void AI2rScript_ClearSkillEdit(void);

#endif // ONI_AI2_SCRIPT_H
