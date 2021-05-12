/*
	FILE:	BFW_Doors.c
	
	AUTHOR:	Quinn Dunki
	
	CREATED: 4/8/98
	
	PURPOSE: Door stuff
	
	Copyright 1998

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_MathLib.h"
#include "BFW_Akira.h"
#include "BFW_Console.h"
#include "BFW_AppUtilities.h"
#include "BFW_BitVector.h"
#include "BFW_Doors.h"
#include "Oni_GameStatePrivate.h"

#define OBcDoorFaceAngle 1.4f // 80 degrees

OBtDoor *OBgDoorChainRoot;
UUtBool OBgDoorChainFirst;
extern UUtBool OBgObjectsShowDebug;

OBtDoor *OBrDoorFromScriptID(
	UUtUns16 inID)
{
	/************
	* Finds a door with a given script ID
	*/

	UUtUns32 i;
	OBtDoor *door;

	for (i=0; i<ONgGameState->doors.numDoors; i++)
	{
		door = ONgGameState->doors.doors + i;
		if (door->doorClass->id == inID) return door;
	}

	return NULL;
}

static UUtBool OBiDoor_CharacterProximity(
	OBtDoor *inDoor,
	ONtCharacter *inCharacter)
{
	/*************
	* Returns whether this character is within radius of this door
	*/

	float distance_squared;
	UUtBool proximate;

	UUmAssert(inDoor->object);

	distance_squared = MUrPoint_Distance_Squared(&inCharacter->location,&inDoor->object->physics->position);

	proximate = distance_squared < inDoor->doorClass->activationRadiusSquared;

	return proximate;
}

static ONtCharacter *OBiDoor_FirstCharacterProximity(
	OBtDoor *inDoor)
{
	/***********
	* Returns the first character, if any, that is within our radius
	*/

	UUtUns32 itr;
	UUtUns32 count;
	ONtCharacter **character_list;

	character_list = ONrGameState_LivingCharacterList_Get();
	count = ONrGameState_LivingCharacterList_Count();

	for(itr = 0; itr < count; itr++)
	{
		ONtCharacter *character;
	
		character = character_list[itr];

		UUmAssert(character->flags & ONcCharacterFlag_InUse);
		UUmAssert(!(character->flags & ONcCharacterFlag_Dead));

		//if (UUrBitVector_TestBit(&inDoor->doorClass->invisibility,character->teamNumber)) continue;
		
		if (OBiDoor_CharacterProximity(inDoor,character)) {
			return character;
		}
	}

	return NULL;
}

void OBrDoor_Kick(
	void *inKicker)
{
	/***********
	* Checks for a character kicking open a door
	*/

	UUtUns32 i;
	OBtDoor *door;

	for (i=0; i<ONgGameState->doors.numDoors; i++)
	{
		door = ONgGameState->doors.doors + i;
		if (!(door->doorClass->flags & OBcDoorClassFlag_Kickable)) continue;

		if (OBiDoor_CharacterProximity(door,inKicker))
		{
			OBrDoor_Open(door,inKicker);
			return;
		}
	}
}

static UUtBool OBiDoor_CanOpen(
	OBtDoor *inDoor,		// Can be NULL)
	ONtCharacter *inUser)	// Can be NULL)
{
	/*********
	* Returns FALSE if this door can't open
	*/

	OBtDoor *link;
	
	UUmAssert(inDoor->object);
	if (inDoor == OBgDoorChainRoot && !OBgDoorChainFirst) return UUcTrue;	// End circular recursion
	
	// Check linked doors recursively first
	OBgDoorChainFirst = UUcFalse;
	if (inDoor->doorClass->linkID)
	{
		link = OBrDoorFromScriptID(inDoor->doorClass->linkID);
		if (!link)
		{
			COrConsole_Printf("ERROR: Link #%d is not a legal door",inDoor->doorClass->linkID);
			return UUcFalse;
		}

		if (!OBiDoor_CanOpen(link,inUser)) return UUcFalse;
	}

	if (inDoor->state != OBcDoorState_Open)
	{
		// Make sure we can open
		if (inUser)
		{
			if (inDoor->doorClass->openKey && (inDoor->flags & OBcDoorFlag_Locked))
			{
				if (!WPrInventory_HasKey(&inUser->inventory,inDoor->doorClass->openKey))
				{
					// Door is locked, requires a key, and the user doesn't have it
					return UUcFalse;
				}
			}
		}
	}

	return UUcTrue;
}

static void OBiDoor_Open(
	OBtDoor *inDoor,
	ONtCharacter *inUser)	// Can be NULL
{
	/*********
	* Open this door
	*/

	OBtDoor *link;
	
	UUmAssert(inDoor);
	UUmAssert(inDoor->object);
	if (inDoor == OBgDoorChainRoot && !OBgDoorChainFirst) return;	// End circular recursion
	
	// Activate linked doors recursively first
	OBgDoorChainFirst = UUcFalse;
	if (inDoor->doorClass->linkID)
	{
		link = OBrDoorFromScriptID(inDoor->doorClass->linkID);
		if (!link)
		{
			COrConsole_Printf("ERROR: Link #%d is not a legal door",inDoor->doorClass->linkID);
			return;
		}

		OBiDoor_Open(link,inUser);
	}

	if (inDoor->state != OBcDoorState_Open)
	{
		inDoor->state = OBcDoorState_Open;
		inDoor->flags |= OBcDoorFlag_Busy;

		if (inDoor->object->physics->animContext.animationFrame==0)	// Handles doors that have never animated yet
			inDoor->object->physics->animContext.animationFrame = inDoor->doorClass->openCloseAnim->numFrames-1;
		OBrSetAnimation(
			inDoor->object,
			inDoor->doorClass->openCloseAnim,
			inDoor->object->physics->animContext.animationFrame,
			inDoor->doorClass->openCloseAnim->doorOpenFrames);
		inDoor->object->physics->animContext.animationFrame = 
			inDoor->doorClass->openCloseAnim->numFrames - inDoor->object->physics->animContext.animationFrame - 1;
	}
}

static UUtBool OBiDoor_CanClose(
	OBtDoor *inDoor,		// Can be NULL)
	ONtCharacter *inUser)	// Can be NULL)
{
	/*********
	* Returns FALSE if this door can't close
	*/

	OBtDoor *link;
	ONtCharacter *character;
	
	UUmAssert(inDoor->object);
	if (inDoor == OBgDoorChainRoot && !OBgDoorChainFirst) return UUcTrue;	// End circular recursion
	
	// Check linked doors recursively first
	OBgDoorChainFirst = UUcFalse;
	if (inDoor->doorClass->linkID)
	{
		link = OBrDoorFromScriptID(inDoor->doorClass->linkID);
		if (!link)
		{
			COrConsole_Printf("ERROR: Link #%d is not a legal door",inDoor->doorClass->linkID);
			return UUcFalse;
		}

		if (!OBiDoor_CanClose(link,inUser)) return UUcFalse;
	}

	if (inDoor->state != OBcDoorState_Closed)
	{
		// Make sure we can close
		//if (!(inDoor->doorClass->flags & OBcDoorClassFlag_CloseManual))
		{
			character = OBiDoor_FirstCharacterProximity(inDoor);
			if (character)
			{
				// Can't close unless the character blocking us is the user and we're manual
				if (inUser && (character != inUser)) return UUcFalse;
				if (!(inDoor->doorClass->flags & OBcDoorClassFlag_CloseManual)) return UUcFalse;
			}
		}

		if (inUser)
		{
			if (inDoor->doorClass->closeKey && (inDoor->flags & OBcDoorFlag_Locked))
			{
				if (!WPrInventory_HasKey(&inUser->inventory,inDoor->doorClass->closeKey))
				{
					// Door is locked, requires a key, and the user doesn't have it
					return UUcFalse;
				}
			}
		}
	}

	return UUcTrue;
}

static void OBiDoor_Close(
	OBtDoor *inDoor,		// Can be NULL)
	ONtCharacter *inUser)	// Can be NULL)
{
	/*********
	* Close this door
	*/

	OBtDoor *link;
	
	UUmAssert(inDoor->object);
	if (inDoor == OBgDoorChainRoot && !OBgDoorChainFirst) return;	// End circular recursion
	
	// Activate linked doors recursively first
	OBgDoorChainFirst = UUcFalse;
	if (inDoor->doorClass->linkID)
	{
		link = OBrDoorFromScriptID(inDoor->doorClass->linkID);
		if (!link)
		{
			COrConsole_Printf("ERROR: Link #%d is not a legal door",inDoor->doorClass->linkID);
			return;
		}

		OBiDoor_Close(link,inUser);
	}


	if (inDoor->state != OBcDoorState_Closed)
	{
		// Close
		inDoor->state = OBcDoorState_Closed;
		inDoor->flags |= OBcDoorFlag_Busy;
		OBrSetAnimation(
			inDoor->object,
			inDoor->doorClass->openCloseAnim,
			inDoor->object->physics->animContext.animationFrame,
			inDoor->doorClass->openCloseAnim->numFrames);
		inDoor->object->physics->animContext.animationFrame = 
			inDoor->doorClass->openCloseAnim->numFrames - inDoor->object->physics->animContext.animationFrame;
	}
}

static void OBiDoor_Lock(
	OBtDoor *inDoor,		// Can be NULL)
	ONtCharacter *inUser)	// Can be NULL)
{
	/*********
	* Lock this door
	*/

	OBtDoor *link;
	
	UUmAssert(inDoor->object);
	if (inDoor == OBgDoorChainRoot && !OBgDoorChainFirst) return;	// End circular recursion
	
	// Activate linked doors recursively first
	OBgDoorChainFirst = UUcFalse;
	if (inDoor->doorClass->linkID)
	{
		link = OBrDoorFromScriptID(inDoor->doorClass->linkID);
		if (!link)
		{
			COrConsole_Printf("ERROR: Link #%d is not a legal door",inDoor->doorClass->linkID);
			return;
		}

		OBiDoor_Lock(link,inUser);
	}

	inDoor->flags |= OBcDoorFlag_Locked;
}

static void OBiDoor_Unlock(
	OBtDoor *inDoor,		// Can be NULL)
	ONtCharacter *inUser)	// Can be NULL)
{
	/*********
	* Unlock this door
	*/

	OBtDoor *link;
	
	UUmAssert(inDoor->object);
	if (inDoor == OBgDoorChainRoot && !OBgDoorChainFirst) return;	// End circular recursion
	
	// Activate linked doors recursively first
	OBgDoorChainFirst = UUcFalse;
	if (inDoor->doorClass->linkID)
	{
		link = OBrDoorFromScriptID(inDoor->doorClass->linkID);
		if (!link)
		{
			COrConsole_Printf("ERROR: Link #%d is not a legal door",inDoor->doorClass->linkID);
			return;
		}

		OBiDoor_Unlock(link,inUser);
	}

	inDoor->flags &=~OBcDoorFlag_Locked;
}


void OBrDoor_Open(
	OBtDoor *inDoor,
	void *inUser)	// Can be NULL
{
	/***********
	* Recursively opens a chain of linked doors
	*/

	OBgDoorChainRoot = inDoor;
	OBgDoorChainFirst = UUcTrue;
	if (OBiDoor_CanOpen(inDoor,inUser))
	{
		OBgDoorChainRoot = inDoor;
		OBgDoorChainFirst = UUcTrue;
		OBiDoor_Open(inDoor,inUser);
	}
}

void OBrDoor_Close(
	OBtDoor *inDoor,
	void *inUser)	// Can be NULL
{
	/***********
	* Recursively closes a chain of linked doors
	*/

	OBgDoorChainRoot = inDoor;
	OBgDoorChainFirst = UUcTrue;
	if (OBiDoor_CanClose(inDoor,inUser))
	{
		OBgDoorChainRoot = inDoor;
		OBgDoorChainFirst = UUcTrue;
		OBiDoor_Close(inDoor,inUser);
	}
}

void OBrDoor_Lock(
	OBtDoor *inDoor,
	void *inUser)	// Can be NULL
{
	/***********
	* Recursively locks a chain of linked doors
	*/

	OBgDoorChainRoot = inDoor;
	OBgDoorChainFirst = UUcTrue;
	OBiDoor_Lock(inDoor,inUser);
}

void OBrDoor_Unlock(
	OBtDoor *inDoor,
	void *inUser)	// Can be NULL
{
	/***********
	* Recursively unlocks a chain of linked doors
	*/

	OBgDoorChainRoot = inDoor;
	OBgDoorChainFirst = UUcTrue;
	OBiDoor_Unlock(inDoor,inUser);
}

static UUtBool OBiDoor_LinkClear(
	OBtDoor *inDoor)
{
	/****************
	* Checks if any characters are
	* standing in any door in our chain
	*/

	return UUcFalse;
}


void OBrDoor_Array_Update(
	OBtDoorArray *inDoors)
{
	/*************
	* Updates the state of the doors for a frame
	*/

	UUtUns16 i;
	float best_angle, door_angle;
	UUtBool check_player_activate;
	OBtDoor *door, *best_door;
	OBtDoorClass *doorClass;
	ONtCharacter *character;
	M3tVector3D charToDoor;
	M3tPoint3D charSource;
	ONtCharacter *player_character;
	ONtActiveCharacter *player_active;


	// Clear busy flags
	for (i=0; i<inDoors->numDoors; i++)
	{
		door = inDoors->doors + i;
		doorClass = door->doorClass;
		if (!door->object) continue;	// Skip destroyed or invalid doors

		door->flags &=~OBcDoorFlag_Busy;
	}

	player_character = ONrGameState_GetPlayerCharacter();
	player_active = ONrGetActiveCharacter(player_character);
	UUmAssertReadPtr(player_active, sizeof(*player_active));
	check_player_activate = ((player_active->inputState.buttonWentDown & LIc_BitMask_Action) > 0);
	best_angle = +1e09;
	best_door = NULL;
	
	// Update doors
	for (i=0; i<inDoors->numDoors; i++)
	{
		door = inDoors->doors + i;
		doorClass = door->doorClass;
		if (!door->object) continue;	// Skip destroyed or invalid doors
		if (door->flags & OBcDoorFlag_Busy) continue;

		// Check for automatic proximity detection
		if ((!(doorClass->flags & OBcDoorClassFlag_OpenManual) && door->state == OBcDoorState_Closed) ||
			(!(doorClass->flags & OBcDoorClassFlag_CloseManual) && door->state == OBcDoorState_Open))
		{
			character = OBiDoor_FirstCharacterProximity(door);
			if (character)
			{
				if (door->state == OBcDoorState_Closed) OBrDoor_Open(door,character);
			}
			else
			{
				if (door->state == OBcDoorState_Open) 
				{
					OBrDoor_Close(door,character);
				}
			}
		}

		else if ((check_player_activate) && (((doorClass->flags & OBcDoorClassFlag_OpenManual) && (door->state == OBcDoorState_Closed)) ||
											 ((doorClass->flags & OBcDoorClassFlag_CloseManual) && (door->state == OBcDoorState_Open))))
		{
			// check for manual activation by player. manually-activated doors are opened directly from AI code
			// if the AI wants to go through them.
			if (OBiDoor_CharacterProximity(door, player_character))
			{
				// See if we're facing this door
				charSource = player_character->location;
				charSource.y += ONcCharacterHeight;
				MUmVector_Subtract(charToDoor,door->object->physics->position,charSource);
				MUrNormalize(&charToDoor);
				door_angle = MUrAngleBetweenVectors3D(&charToDoor,&player_character->facingVector);
				
				if (door_angle < best_angle) {
					best_door = door;
					best_angle = door_angle;
				}
			}
		}
	}

	if ((check_player_activate) && (best_angle < OBcDoorFaceAngle)) {
		UUmAssertReadPtr(best_door, sizeof(*best_door));

		// open the door that we are facing closest towards
		if (best_door->state == OBcDoorState_Open) {
			OBrDoor_Close(best_door, player_character);
		} else {
			OBrDoor_Open(best_door, player_character);
		}
	}
}	

static UUtError OBrObjectArrayFromDoorID( UUtUns16 inID, OBtObject ***ioArray, UUtUns32 *ioCount )
{
	UUtUns16		i;
	OBtObject		*object;
	OBtObjectList	*oblist = ONrGameState_GetObjectList();
	UUtUns32		count;
	OBtObject		**new_array;
	UUtUns32		number;

	count = 0;

	for (i=0; i<oblist->numObjects; i++)
	{
		object = oblist->object_list + i;

		if (!(object->flags & OBcFlags_InUse))		continue;
		number = object->setup->doorScriptID >> 8;
		if (number == inID)	count++;
	}

	if( !count )
	{
		*ioArray = NULL;
		*ioCount = 0;
		return UUcError_None;
	}
	
	new_array = (OBtObject**) UUrMemory_Block_New(sizeof(OBtObject*) * count );
	count = 0;
	
	for (i=0; i<oblist->numObjects; i++)
	{
		object = oblist->object_list + i;

		if (!(object->flags & OBcFlags_InUse))		continue;
		number = object->setup->doorScriptID >> 8;
		if (number == inID)	new_array[count++] = object;
	}

	*ioArray	= new_array;
	*ioCount	= count;

	return UUcError_None;
}

OBtObject *OBrObjectFromDoorID( UUtUns16 inID )
{
	/************
	* Returns the object matching a door ID
	*/
	UUtUns16 i;
	OBtObject *object;
	OBtObjectList *oblist = ONrGameState_GetObjectList();

	for (i=0; i<oblist->numObjects; i++)
	{
		object = oblist->object_list + i;

		if (!(object->flags & OBcFlags_InUse))		continue;
		if (object->setup->doorScriptID == inID)	return object;
	}

	return NULL;
}

UUtError OBrDoor_Array_LevelBegin(
	OBtDoorArray *inDoors,
	OBtDoorClassArray *inClasses)
{
	/************
	* Set up doors for a level
	*/

	UUtUns16 i;
	OBtDoor *door;
	
	if (inClasses == NULL) {
		inDoors->numDoors = 0;
		inDoors->doors = NULL;
		return UUcError_None;
	}

	inDoors->numDoors = inClasses->numDoors;
	if (inDoors->numDoors > 0)
	{
		inDoors->doors = UUrMemory_Block_New(sizeof(OBtDoor) * inDoors->numDoors);
		if (!inDoors->doors) return UUcError_OutOfMemory;
	}


	for (i=0; i< inDoors->numDoors; i++)
	{
		door = inDoors->doors + i;

		UUrMemory_Clear(door,sizeof(OBtDoor));

		door->doorClass		= inClasses->doors + i;
		door->object		= OBrObjectFromDoorID(door->doorClass->id);

		if (!door->object)
		{
			COrConsole_Printf("ERROR: Unable to match door ID %d with door geometry",door->doorClass->id);
		}

		// Default to locked for keyed doors
		if (door->doorClass->openKey) door->flags |= OBcDoorFlag_Locked;
	}


	return UUcError_None;
}

void OBrDoor_Array_LevelEnd(
	OBtDoorArray *inDoors)
{
	/*************
	* Clean up doors from a level
	*/

	if (inDoors != NULL) 
	{
		if (inDoors->doors != NULL)
		{
			UUrMemory_Block_Delete(inDoors->doors);
			inDoors->doors = NULL;
		}

		inDoors->numDoors = 0;
	}

	return;
}

OBtDoor *OBrDoorFromGhost(
	AKtGQ_General *inGhost)
{
	/************
	* Returns the door sitting on this ghost quad
	*/

	UUtUns32 i;
	OBtDoor *door;
	OBtDoorArray *doorArray = &ONgGameState->doors;
	UUtUns16 gqIndex;

	if (!(inGhost->flags & AKcGQ_Flag_Door)) return NULL;

	gqIndex = (UUtUns16)(inGhost - ONgLevel->environment->gqGeneralArray->gqGeneral);

	for (i=0; i<doorArray->numDoors; i++)
	{
		door = doorArray->doors + i;

		if (door->object->setup->doorGhostIndex == gqIndex)
		{
			return door;
		}
	}
	return NULL;
}
