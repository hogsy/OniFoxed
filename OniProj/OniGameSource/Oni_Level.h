#pragma once

/*
	Oni_Level.h
	
	Level stuff
	
	Author: Quinn Dunki
	c1998 Bungie
*/
#ifndef ONI_LEVEL_H
#define ONI_LEVEL_H

#include "BFW_Doors.h"

#include "Oni_AI_Setup.h"
#include "Oni_AI_Script.h"
#include "Oni_GameState.h"
#include "Oni_Sky.h"

#define ONcMaxLevelName 64
#define ONcMaxMarkerName 64		// Don't change without changing references below
#define ONcMaxActionMarkers 32

typedef tm_struct ONtActionMarker
{
	UUtUns32				object;
	M3tPoint3D				position;
	float					direction;		// direction character needs to face to use marker
} ONtActionMarker;

//#define ONcTemplate_ActionMarkerArray UUm4CharToUns32('O','N','A','A')
//typedef tm_template('O','N','A','A', "Oni Action Marker Array")
typedef tm_struct ONtActionMarkerArray
{
	UUtUns32				num_markers;
	ONtActionMarker			markers[32];
} ONtActionMarkerArray;

typedef tm_struct ONtMarker
{
	char					name[64];		// Must match ONcMaxMarkerName above

	M3tPoint3D				position;
	M3tVector3D				direction;
} ONtMarker;

#define ONcTemplate_MarkerArray UUm4CharToUns32('O','N','M','A')
typedef tm_template('O','N','M','A', "Imported Marker Node Array")
ONtMarkerArray
{
	tm_pad					pad0[22];
	
	tm_varindex UUtUns16	numMarkers;
	tm_vararray ONtMarker	markers[1];
} ONtMarkerArray;

tm_struct ONtFlag
{
	M3tMatrix4x3	matrix;
	M3tPoint3D		location;	// equal to MUrMatrix_matrix, location)
	float			rotation;	// amount a point at 1,0,0 would be rotated to or 0
	UUtInt16		idNumber;
	UUtBool			deleted;
	UUtBool			maxFlag;	// this flag was created in max, used by the tool
};

#define ONcTemplate_FlagArray UUm4CharToUns32('O','N','F','A')
typedef tm_template('O','N','F','A', "Imported Flag Node Array")
ONtFlagArray
{
	tm_pad					pad0[20];

	UUtUns16				curNumFlags;

	tm_varindex UUtUns16	maxFlags;
	tm_vararray ONtFlag		flags[1];
} ONtFlagArray;

#define ONcTemplate_SpawnArray UUm4CharToUns32('O','N','S','A')
typedef tm_template('O','N','S','A', "Imported Spawn Array")
ONtSpawnArray
{
	tm_pad					pad0[22];

	tm_varindex UUtUns16	numSpawnFlagIDs;
	tm_vararray UUtInt16	spawnFlagIDs[1];
} ONtSpawnArray;

typedef tm_struct ONtObjectGunk
{
	UUtUns32				object_tag;
	TMtIndexArray			*gq_array;	
} ONtObjectGunk;

#define ONcTemplate_ObjectGunkArray UUm4CharToUns32('O', 'N', 'O', 'A')
typedef tm_template('O','N','O','A', "Object Gunk Array")
ONtObjectGunkArray
{
	tm_pad						pad0[20];

	tm_varindex UUtUns32		num_objects;
	tm_vararray ONtObjectGunk	objects[1];
} ONtObjectGunkArray;


typedef tm_struct ONtTrigger
{
	// actual triggering information
	M3tBoundingVolume	volume;

	// authoring information
	M3tPoint3D			location;
	M3tPoint3D			scale;
	M3tQuaternion		orientation;
	UUtInt32			idNumber;
//	UUtUns32			refCon;			// saved as zero, ptr at runtime

} ONtTrigger;

#define ONcTemplate_TriggerArray UUm4CharToUns32('O', 'N', 'T', 'A')
typedef tm_template('O','N','T','A', "Trigger Array")
ONtTriggerArray
{
	tm_pad					pad0[16];

	UUtUns32				curNumTriggers;

	tm_varindex UUtUns32	maxTriggers;
	tm_vararray ONtTrigger	triggers[1];
} ONtTriggerArray;

	
#define ONcTemplate_Level UUm4CharToUns32('O','N','L','V')
typedef tm_template('O','N','L','V', "Oni Game Level")
ONtLevel
{
	char name[64];	// Must be same as ONcMaxLevelName above (for TE)
	
	AKtEnvironment			*environment;
	OBtObjectSetupArray		*objectSetupArray;
	ONtMarkerArray			*markerArray;
	ONtFlagArray			*flagArray;
	ONtTriggerArray			*triggerArray;
	
	ONtSkyClass				*sky_class;
	float					sky_height;

	AItCharacterSetupArray	*characterSetupArray;
	AItScriptTriggerClassArray *scriptTriggerArray;
	ONtSpawnArray			*spawnArray;
	OBtDoorClassArray		*doorArray;
	
	ONtObjectGunkArray		*object_gunk_array;

	EPtEnvParticleArray		*envParticleArray;

	ONtActionMarkerArray	actionMarkerArray;
	ONtCorpseArray			*corpseArray;

} ONtLevel;


#define ONcTemplate_Level_Descriptor		UUm4CharToUns32('O', 'N', 'L', 'D')
typedef tm_template('O', 'N', 'L', 'D', "Oni Game Level Descriptor")
ONtLevel_Descriptor
{
	
	UUtUns16			level_number;
	UUtUns16			next_level_number;
	char				level_name[64];  // must be the same as ONcMaxLevelName
	
} ONtLevel_Descriptor;


// globals
extern ONtLevel *ONgLevel;
extern TMtPrivateData*	ONgTemplate_Level_PrivateData;

// functions


// **** flag functions:

// ids are how artists refer to them
UUtBool ONrLevel_Flag_ID_To_Location(UUtInt16 inID, M3tPoint3D	*outLocation);
UUtBool ONrLevel_Flag_ID_To_Flag(UUtInt16 inID, ONtFlag *outFlag);
void ONrLevel_Flag_GetVector(ONtFlag *inFlag, M3tVector3D *outVector);

// find operations
UUtBool ONrLevel_Flag_Location_To_Flag(const M3tPoint3D *inLocation, ONtFlag *outFlag);

void ONrLevel_Flag_Delete(UUtUns16 inID);

// **** character setup functions:

// ids are how artists refer to them
const AItCharacterSetup *ONrLevel_CharacterSetup_ID_To_Pointer(UUtInt16 inID);

UUtError AIrCreatePlayerFromTextFile(void);

void
ONrLevel_LevelLoadDialog_Update(
	UUtUns8				inPercentComplete);

UUtUns16
ONrLevel_GetCurrentLevel(
	void);
	
void
ONrLevel_Unload(
	void);


// returns 0 if that was the last level
UUtUns16 ONrLevel_GetNextLevel(UUtUns16 inLevelNum);

UUtError
ONrLevel_LoadZero(
	void);

UUtError
ONrLevel_Load(UUtUns16	inLevelNum, UUtBool inProgressBar);

void ONrLevel_Terminate(
	void);
	
UUtError ONrLevel_Initialize(
	void);

ONtTrigger*
ONrLevel_Trigger_New(
	void);

void
ONrLevel_Trigger_TransformPoints(
	ONtTrigger				*ioTrigger);

void
ONrLevel_Trigger_Display(
	ONtTrigger				*inTrigger,
	UUtUns32				inShade);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
// action markers

ONtActionMarker* ONrLevel_ActionMarker_New( void);

ONtActionMarker* ONrLevel_ActionMarker_FindNearest(const M3tPoint3D	*inLocation);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
// object ids

UUtBool ONrLevel_FindObjectTag( UUtUns32 inObjectTag );
ONtObjectGunk	*ONrLevel_FindObjectGunk( UUtUns32 inObjectTag );
IMtShade ONrLevel_ObjectGunk_GetShade(ONtObjectGunk	*inObjectGunk);
void	 ONrLevel_ObjectGunk_SetShade(ONtObjectGunk *inObjectGunk, IMtShade inShade);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
// preload textures

void ONrLevel_Preload_Textures(void);

#endif /* ONI_LEVEL_H */