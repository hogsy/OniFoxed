#pragma once

#ifndef __BFW_DOORS_H__
#define __BFW_DOORS_H__

#include "BFW_Object.h"

typedef enum OBtDoorClassFlags
{
	OBcDoorClassFlag_None =			0x0000,
	OBcDoorClassFlag_OpenManual =	0x0001,
	OBcDoorClassFlag_CloseManual =	0x0002,
	OBcDoorClassFlag_ReverseLink =	0x0004,
	OBcDoorClassFlag_Kickable	=	0x0008
} OBtDoorClassFlags;

// ===================================================================================================

typedef tm_struct OBtDoorClass
{
	UUtUns16			id;
	UUtUns16			flags;		// OBtDoorClassFlags

	OBtAnimation		*openCloseAnim;

	UUtUns16			openKey;		// Not used
	UUtUns16			closeKey;		// "

	float				activationRadiusSquared;
	UUtUns16			objectID;		// ???
	UUtUns16			linkID;

	UUtUns32			invisibility;	// Bit vector
} OBtDoorClass;

#define OBcTemplate_DoorClassArray UUm4CharToUns32('O','B','D','C')
typedef tm_template('O','B','D','C', "Door class array")
OBtDoorClassArray
{
	tm_pad						pad0[22];

	tm_varindex UUtUns16		numDoors;
	tm_vararray OBtDoorClass	doors[1];

} OBtDoorClassArray;

typedef enum OBtDoorActivationState
{
	OBcDoorActivation_InRadius,
	OBcDoorActivation_OutRadius
} OBtDoorActivationState;

typedef enum OBtDoorState
{
	OBcDoorState_Closed,
	OBcDoorState_Open
} OBtDoorState;

typedef enum OBtDoorFlags
{
	OBcDoorFlag_None,
	OBcDoorFlag_Busy,
	OBcDoorFlag_Locked
} OBtDoorFlags;

typedef struct OBtDoor
{
	OBtDoorClass			*doorClass;
	OBtObject				*object;
	UUtUns16				flags;
	OBtDoorState			state;
	OBtDoorActivationState	activationState;
} OBtDoor;

typedef struct OBtDoorArray
{
	UUtUns16				numDoors;
	OBtDoor					*doors;
} OBtDoorArray;

// ===================================================================================================

UUtError OBrDoor_Array_LevelBegin(
	OBtDoorArray *inDoors,
	OBtDoorClassArray *inClasses);

void OBrDoor_Array_LevelEnd(
	OBtDoorArray *inDoors);

void OBrDoor_Array_Update(
	OBtDoorArray *inDoors);

OBtDoor *OBrDoorFromGhost(
	AKtGQ_General *inGhost);

void OBrDoor_Open(
	OBtDoor *inDoor,
	void *inUser);	// Can be NULL

void OBrDoor_Close(
	OBtDoor *inDoor,
	void *inUser);	// Can be NULL

OBtDoor *OBrDoorFromScriptID(
	UUtUns16 inID);

void OBrDoor_Kick(
	void *inKicker);

void OBrDoor_Array_Display(
	OBtDoorArray *inDoors);

void OBrDoor_Lock(
	OBtDoor *inDoor,
	void *inUser);	// Can be NULL

void OBrDoor_Unlock(
	OBtDoor *inDoor,
	void *inUser);	// Can be NULL

OBtObject*
OBrObjectFromDoorID(
UUtUns16 inID);

#endif
