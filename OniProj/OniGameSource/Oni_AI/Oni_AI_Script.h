/*
	Oni_AI_Script.h

	Script level AI stuff

	Author: Quinn Dunki, Michael Evans

	Copyright (c) 1998-2000 Bungie Software
*/

#ifndef ONI_AI_SCRIPT_H
#define ONI_AI_SCRIPT_H

#include "BFW.h"
#include "Oni_GameState.h"
#include "Oni_AI.h"

#define AIcMaxScriptName 32			// Don't change this without changing references below
#define AIcMaxParms 10				// Don't change this without changing references below
#define AIcMaxScriptVariables 16	// Don't change without changing AItScriptRender below
#define AIcNoOwner 0xFFFF
#define AIcNoTrigger 0xFFFF
#define AIcMaxContexts 32
#define AIcMaxRefName 64			// Don't change this without changing references below

#define AImScript_Scan_Index(s,i) ((s)->parms + ((i)*AIcMaxScriptName*sizeof(char)))



typedef enum AItContextFlags
{
	AIcContextFlag_IsNew = 		0x0001,
	AIcContextFlag_Joining =	0x0002,
	AIcContextFlag_Respawn =	0x0004

} AItContextFlags;


enum {
	AIcScriptTableFlag_SpawnCalled		= (1 << 0),
	AIcScriptTableFlag_CombatCalled		= (1 << 1),
	AIcScriptTableFlag_DieCalled		= (1 << 2),
	AIcScriptTableFlag_AlarmCalled		= (1 << 3),
	AIcScriptTableFlag_HurtCalled		= (1 << 4),
	AIcScriptTableFlag_DefeatedCalled	= (1 << 5),
	AIcScriptTableFlag_OutOfAmmoCalled	= (1 << 6),
	AIcScriptTableFlag_NoPathCalled		= (1 << 7)
};


typedef tm_struct AItScriptTable
{
	UUtUns32		flags;
	char			spawn_name[32];	// must all match SLcScript_MaxNameLength
	char			combat_name[32];
	char			die_name[32];
	char			alarm_name[32];
	char			hurt_name[32];
	char			defeated_name[32];
	char			outofammo_name[32];
	char			nopath_name[32];
} AItScriptTable;





typedef enum AItTriggerClassFlags
{
	AIcTriggerFlag_Toggle = 	0x0001,
	AIcTriggerFlag_Once = 		0x0002,
	AIcTriggerFlag_BNV = 		0x0004,
	AIcTriggerFlag_Sight =		0x0008,
	AIcTriggerFlag_DataConsole= 0x0010,
	AIcTriggerFlag_Action =		0x0020,
	AIcTriggerFlag_Directional= 0x0040

} AItTriggerClassFlags;

typedef enum AItScriptTriggerType
{
	AIcTriggerType_Temp,
	AIcTriggerType_Flag,
	AIcTriggerType_Character,
	AIcTriggerType_Laser
} AItScriptTriggerType;

typedef tm_struct AItScriptTriggerClass
{
	UUtInt16 id;
	UUtInt16 flagID;
	UUtUns16 type;		// AItScriptTriggerType

	UUtUns16 flags;		// AItTriggerClassFlags
	UUtUns16 team;
	UUtUns16 refRestrict;

	float radius_h;
	float radius_v;


	char refName[64];		// Must match AIcMaxRefName above
} AItScriptTriggerClass;

#define AIcTemplate_ScriptTriggerClassArray UUm4CharToUns32('A','I','T','R')
typedef tm_template('A','I','T','R',"AI script trigger array")
AItScriptTriggerClassArray
{
	tm_pad								pad0[22];

	tm_varindex UUtUns16				numTriggers;
	tm_vararray AItScriptTriggerClass	triggers[1];
} AItScriptTriggerClassArray;







typedef enum AItScriptTriggerFlags
{
	AIcScriptTriggerFlag_None =			0x0000,
	AIcScriptTriggerFlag_Disabled =		0x0001
} AItScriptTriggerFlags;

typedef enum AItTriggerState
{
	AIcTriggerState_Idle,
	AIcTriggerState_Triggered,
	AIcTriggerState_Untriggered,
	AIcTriggerState_Terminal
} AItTriggerState;

typedef struct AItScriptTrigger
{
	AItScriptTriggerClass *triggerClass;

	AItTriggerState state;
	UUtUns16 owner;
	AItScriptTriggerFlags flags;

	void *ref;
} AItScriptTrigger;

typedef struct AItScriptTriggerList
{
	UUtUns16 numTriggers;
	AItScriptTrigger *triggers;
} AItScriptTriggerList;

UUtError AIrScript_Initialize(
	void);

void AIrScript_Terminate(
	void);

void AIrScript_Render(
	void);

UUtError AIrScript_LevelBegin_Triggers(
	void);

void AIrScript_LevelEnd_Triggers(
	void);

ONtCharacter*
AIrUtility_New_GetCharacterFromID(
	UUtUns16	inID,
	UUtBool		inExhaustive);

#endif
