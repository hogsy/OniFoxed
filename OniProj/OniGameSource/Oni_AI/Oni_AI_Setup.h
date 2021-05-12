/*
	Oni_AI_Setup.h
	
	This file contains all top-level AI header code
	
	Author: Quinn Dunki
	Copyright 1998-1999 Bungie
*/

#ifndef ONI_AI_SETUP_H
#define ONI_AI_SETUP_H

#include "BFW.h"
#include "Oni_GameState.h"
//#include "Oni_Character.h"
//#include "Oni_Object.h"
#include "Oni_AI_Script.h"

#define AIcMaxClassNameLen 64		// Don't change this unless you also change AItCharacterSetup
#define AIcMaxClasses 4				// Don't change this unless you also change AItCharacterSetup


/*
 * Character setup stuff
 */
 
enum {
	AIcSetup_Flag_AI =			0x0001,
	AIcSetup_Flag_AutoFreeze = 	0x0002,
	AIcSetup_Flag_Neutral = 	0x0004,
	AIcSetup_Flag_TurnGuard =	0x0008
};

typedef tm_struct AItWaypoint
{
	M3tPoint3D		waypoint;
	M3tVector3D		up;
	UUtUns16		ref;		// AI scripting reference number
	tm_pad			pad[2];
	// Other data coming soon to a structure near you...
} AItWaypoint;

#define AIcTemplate_WaypointArray UUm4CharToUns32('A','I','W','A')
typedef tm_template('A','I','W','A', "AI Imported Waypoint Array")
AItWaypointArray
{
	tm_pad					pad0[22];
	
	tm_varindex UUtUns16	numWaypoints;
	tm_vararray AItWaypoint	waypoints[1];
} AItWaypointArray;

tm_struct AItCharacterSetup
{
	char				playerName[32];				// Note- should be ONcMaxPlayerNameLength

	UUtUns16			defaultScriptID;			// default scriptID (how you are refered)
	UUtInt16			defaultFlagID;				// default flagID (location and facing)

	UUtUns16			flags;					
	UUtUns16			teamNumber;					// 

	ONtCharacterClass	*characterClass;

	char				variable[32];				// variable to decrement on death or 
	
	AItScriptTable		scripts;

	WPtWeaponClass		*weapon;
	UUtUns16			ammo;
	UUtUns16			leaveAmmo;
	UUtUns16			leaveClips;
	UUtUns16			leaveHypos;
	UUtUns16			leaveCells;
	tm_pad				pad[2];
};

#define AIcTemplate_CharacterSetupArray	UUm4CharToUns32('A','I','S','A')
typedef tm_template('A','I','S','A', "AI Character Setup Array")
AItCharacterSetupArray
{
	tm_pad							pad0[22];
	
	tm_varindex UUtUns16			numCharacterSetups;
	tm_vararray AItCharacterSetup	characterSetups[1];
} AItCharacterSetupArray;

typedef tm_struct NMtSpawnPoint
{
	M3tPoint3D location;
	float facing;
} NMtSpawnPoint;

#define NMcTemplate_SpawnPointArray UUm4CharToUns32('N','M','S','A')
typedef tm_template('N','M','S','A', "Network Spawn Point Array")
NMtSpawnPointArray
{
	tm_pad						pad0[22];
	
	tm_varindex UUtUns16		numSpawnPoints;
	tm_vararray NMtSpawnPoint	spawnPoints[1];
} NMtSpawnPointArray;
	

// Initialization
UUtError AIrST_Initialize(void);
void AIrST_Terminate(void);

struct OBJtOSD_Character;
UUtError AIrST_InitializeCharacter(struct ONtCharacter *ioCharacter, const struct OBJtOSD_Character *inStartingData,
								   const struct AItCharacterSetup *inSetup, const ONtFlag *inFlag);

#endif