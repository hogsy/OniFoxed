/*
	Oni_Film.c
	
	Film recording stuff
	
	Author: Quinn Dunki
	c1998 Bungie
*/

#include <ctype.h>

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Mathlib.h"
#include "BFW_Console.h"
#include "BFW_BitVector.h"
#include "BFW_AppUtilities.h"
#include "BFW_ScriptLang.h"
#include "Oni.h"
#include "Oni_AI.h"
#include "Oni_AI_Script.h"
#include "Oni_Astar.h"
#include "Oni_Character.h"
#include "Oni_GameState.h"
#include "Oni_GameStatePrivate.h"
#include "Oni_Film.h"

#define ONcFilm_MaxInterpDist		80.0f

char ONgBoundF2[SLcScript_String_MaxLength];
char ONgBoundF3[SLcScript_String_MaxLength];

static UUtError
ONiSetFocus(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	ONtCharacter *ch;
	UUtUns16 playerNum;
	
	playerNum = (UUtUns16)inParameterList[0].val.i;

	ONrGameState_SetPlayerNum(playerNum);

	ch = ONrGameState_GetPlayerCharacter();
	UUmAssert(ch);

	CAgCamera.star = ch;
	CArFollow_Enter();

	return UUcError_None;
}

UUtError ONrFilm_Initialize(
	void)
{
	/*******************
	* Initializes film recording
	*/
	
	UUtError error;	

	error;

	strcpy(ONgBoundF2, "");
	strcpy(ONgBoundF3, "");

#if CONSOLE_DEBUGGING_COMMANDS
	// Console	
	error =	SLrScript_Command_Register_Void(
		"sc_focus",
		"sets which character we're authoring for a film",
		"chr_index:int",
		ONiSetFocus);
	UUmError_ReturnOnError(error);

	SLrGlobalVariable_Register_String("sc_bind_f2", "binds f2 to a specific animation", ONgBoundF2);
	SLrGlobalVariable_Register_String("sc_bind_f3", "binds f3 to a specific animation", ONgBoundF3);
#endif

	return UUcError_None;
}

void ONrFilm_Terminate(
	void)
{
	/****************
	* Game shutdown of film recording
	*/

	return;
}

void ONrFilm_GetFrame(const ONtFilmState *inFilmState, ONtInputState *outInputState)
{
	ONtFilmKey *key;
	ONtFilm *film;
	UUtUns32 itr;

	film = inFilmState->film;

	for(itr = 0; itr < film->numKeys; itr++) {
		key = film->keys + itr;

		if (key->time > inFilmState->curFrame) {
			key -= 1;
			break;
		}
	}

	

	if (0 == itr) {
		outInputState->turnLR = key->turnLR;
		outInputState->turnUD = key->turnUD;
		outInputState->buttonIsDown = key->current;
		outInputState->buttonIsUp = ~key->current;
		outInputState->buttonWentDown = 0;
		outInputState->buttonWentUp = 0;
	}
	else if (key->time == inFilmState->curFrame) {
		ONtFilmKey *lastKey = key - 1;

		outInputState->turnLR = key->turnLR;
		outInputState->turnUD = key->turnUD;
		outInputState->buttonIsDown = key->current;
		outInputState->buttonIsUp = ~key->current;
		outInputState->buttonWentDown = key->current & ~lastKey->current;
		outInputState->buttonWentUp = ~key->current & lastKey->current;
	}
	else {
		outInputState->turnLR = 0.f;
		outInputState->turnUD = 0.f;
		outInputState->buttonIsDown = key->current;
		outInputState->buttonIsUp = ~key->current;
		outInputState->buttonWentDown = 0;
		outInputState->buttonWentUp = 0;
	}

}

void ONrFilm_AppendFrame(ONtFilmState *inFilmState, ONtInputState *inInputState)
{
	UUtBool addKey;
	ONtFilm *curFilm;

	UUmAssertWritePtr(inFilmState, sizeof(*inFilmState));
	UUmAssertWritePtr(inInputState, sizeof(*inInputState));

	curFilm = inFilmState->film;

	UUmAssert(NULL != curFilm);

	curFilm = inFilmState->film;

	if (NULL == curFilm) {
		COrConsole_Printf("failed to allocate memory for new film");
	}

	if (0 == curFilm->numKeys) {
		addKey = UUcTrue;
	}
	else {
		const ONtFilmKey *oldKey  = curFilm->keys + curFilm->numKeys - 1;

		addKey = 
			(inInputState->turnLR != 0.f) ||
			(inInputState->turnUD != 0.f) ||
			(oldKey->current != inInputState->buttonIsDown);
	}

	if (addKey) {
		ONtFilmKey *newKey  = curFilm->keys + curFilm->numKeys;

		newKey->turnLR = inInputState->turnLR;
		newKey->turnUD = inInputState->turnUD;
		newKey->current = inInputState->buttonIsDown;
		newKey->time = curFilm->length;
		curFilm->numKeys += 1;
	}

	curFilm->length += 1;

	return;
}

UUtBool ONrFilm_Start(ONtCharacter *ioCharacter, ONtFilm *inFilm, UUtUns32 inMode, UUtUns32 inInterpFrames)
{
	ONtActiveCharacter *active_character = ONrForceActiveCharacter(ioCharacter);

	if (active_character == NULL)
		return UUcFalse;

	// clear the sprint varient so we don't mess up the film
	ONrCharacter_SetSprintVarient(ioCharacter, UUcFalse);
	active_character->last_forward_tap = 0;

	active_character->filmState.film = inFilm;
	active_character->filmState.curFrame =0;

	// snap the character's facing and aiming
	ioCharacter->facing			= active_character->filmState.film->initialFacing;
	ioCharacter->desiredFacing	= active_character->filmState.film->initialDesiredFacing;
	active_character->aimingLR	= active_character->filmState.film->initialAimingLR;
	active_character->aimingUD	= active_character->filmState.film->initialAimingUD;

	if (inMode == ONcFilmMode_Normal) {
		// warp the character to the film's initial location
		ONrCharacter_Teleport(ioCharacter, &active_character->filmState.film->initialLocation, UUcFalse);

		if (active_character->inAirControl.numFramesInAir > 0) {
			active_character->inAirControl.numFramesInAir = 0;
			ONrCharacter_ForceStand(ioCharacter);
		}

	} else if (inMode == ONcFilmMode_Interp) {
		// store the character's delta-position in order to get where they should be for the film
		active_character->filmState.interp_frames = UUmMax(inInterpFrames, 1);
		MUmVector_Subtract(active_character->filmState.interp_delta, active_character->filmState.film->initialLocation, ioCharacter->location);

		if (MUmVector_GetLengthSquared(active_character->filmState.interp_delta) > UUmSQR(ONcFilm_MaxInterpDist)) {
			// we cannot play this film, we are too far away! abort!
			return UUcFalse;
		}

		MUmVector_Scale(active_character->filmState.interp_delta, 1.0f / active_character->filmState.interp_frames);

		active_character->filmState.flags |= ONcFilmFlag_Interp;
	}

	active_character->filmState.flags |= ONcFilmFlag_Play;
	return UUcTrue;
}

UUtBool ONrFilm_Stop(ONtCharacter *ioCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);

	if (active_character == NULL) {
		// we can't possibly be playing a film
		return UUcFalse;
	}

	if ((active_character->filmState.flags & ONcFilmFlag_Play) == 0) {
		return UUcFalse;
	}

	active_character->filmState.flags = 0;
	return UUcTrue;
}

void ONrFilm_Swap(ONtFilm *ioFilm, ONtFilm_SwapSrc inSwapSrc)
{
	UUtUns32 itr;
	UUtUns32 numKeys;

	numKeys = ioFilm->numKeys;

	if (ONcFilmSwapSrc_Disk == inSwapSrc) {
		UUmSwapBig_4Byte(&numKeys);
	}

	UUmSwapBig_4Byte(&ioFilm->initialLocation.x);
	UUmSwapBig_4Byte(&ioFilm->initialLocation.y);
	UUmSwapBig_4Byte(&ioFilm->initialLocation.z);
	UUmSwapBig_4Byte(&ioFilm->initialFacing);
	UUmSwapBig_4Byte(&ioFilm->initialDesiredFacing);
	UUmSwapBig_4Byte(&ioFilm->initialAimingLR);
	UUmSwapBig_4Byte(&ioFilm->initialAimingUD);
	UUmSwapBig_4Byte(&ioFilm->length);
	UUmSwapBig_4Byte(&ioFilm->numKeys);

	for(itr = 0; itr < numKeys; itr++) {
		ONtFilmKey *key = ioFilm->keys + itr;
	
		UUmSwapBig_4Byte(&key->turnUD);
		UUmSwapBig_4Byte(&key->turnLR);
		UUmSwapBig_8Byte(&key->current);
		UUmSwapBig_4Byte(&key->time);
	}
}

void ONrFilm_Swap_DiskFormat(ONtFilm_DiskFormat *ioFilm, ONtFilmKey *ioKeys)
{
	UUtUns32 itr;

	UUmSwapBig_4Byte(&ioFilm->initialLocation.x);
	UUmSwapBig_4Byte(&ioFilm->initialLocation.y);
	UUmSwapBig_4Byte(&ioFilm->initialLocation.z);
	UUmSwapBig_4Byte(&ioFilm->initialFacing);
	UUmSwapBig_4Byte(&ioFilm->initialDesiredFacing);
	UUmSwapBig_4Byte(&ioFilm->initialAimingLR);
	UUmSwapBig_4Byte(&ioFilm->initialAimingUD);
	UUmSwapBig_4Byte(&ioFilm->length);
	UUmSwapBig_4Byte(&ioFilm->numKeys);

	for(itr = 0; itr < ioFilm->numKeys; itr++) {
		ONtFilmKey *key = ioKeys + itr;
	
		UUmSwapBig_4Byte(&key->turnUD);
		UUmSwapBig_4Byte(&key->turnLR);
		UUmSwapBig_8Byte(&key->current);
		UUmSwapBig_4Byte(&key->time);
	}
}


void ONrFilm_WriteToDisk(ONtFilm *inFilm)
{
	static UUtInt32 saved_film_number = 0;
	BFtFileRef *fileRef = NULL;

	fileRef = BFrFileRef_MakeUnique("saved_film", ".dat", &saved_film_number, 999);

	if (NULL != fileRef) {
		UUtError error;
		BFtFile *file;

		COrConsole_Printf("writing film %s", BFrFileRef_GetLeafName(fileRef));

		error = BFrFile_Open(fileRef, "w", &file);

		if (UUcError_None != error) {
			COrConsole_Printf("could not open file");
		}
		else {
			UUtUns32 fileLength = inFilm->numKeys * sizeof(ONtFilmKey);
			UUtUns32 numKeys = inFilm->numKeys;
			ONtFilm_DiskFormat diskFormat;

			BFrFile_Write(file, 128, ONgBoundF2);
			BFrFile_Write(file, 128, ONgBoundF3);

			ONrFilm_Swap(inFilm, ONcFilmSwapSrc_Memory);

			UUrMemory_Clear(&diskFormat, sizeof(diskFormat));

			diskFormat.initialAimingLR = inFilm->initialAimingLR;
			diskFormat.initialAimingUD = inFilm->initialAimingUD;
			diskFormat.initialDesiredFacing = inFilm->initialDesiredFacing;
			diskFormat.initialFacing = inFilm->initialFacing;
			diskFormat.initialLocation = inFilm->initialLocation;
			diskFormat.length = inFilm->length;
			diskFormat.numKeys = inFilm->numKeys;

			error = BFrFile_Write(file, sizeof(diskFormat), &diskFormat);

			if (UUcError_None == error) {
				error = BFrFile_Write(file, fileLength, inFilm->keys);
			}

			if (UUcError_None != error) { 
				COrConsole_Printf("failed to write file");
			}

			ONrFilm_Swap(inFilm, ONcFilmSwapSrc_Disk);	// after previous swap, we were in disk format

			BFrFile_Close(file);
		}

		BFrFileRef_Dispose(fileRef);
	}

	return;
}


void ONrFilm_Create(ONtFilmState *ioFilmState)
{
	ioFilmState->film = UUrMemory_Block_New(sizeof(ONtFilm) + ONcFilm_MaxKeys * sizeof(ONtFilmKey));

	if (ioFilmState->film != NULL) {
		ioFilmState->film->length = 0;
		ioFilmState->film->numKeys = 0;
	}

	return;
}

void ONrFilm_DisplayStats(ONtFilm *inFilm)
{
	UUmAssert(NULL != inFilm);

	if (NULL != inFilm) {
		const char *instance_name;

		instance_name = TMrInstance_GetInstanceName(inFilm);
		instance_name = (NULL == instance_name) ? "no-name" : instance_name;

		COrConsole_Printf("%s length %d keys %d loc %f,%f,%f",
			instance_name, 
			inFilm->length, 
			inFilm->numKeys, 
			inFilm->initialLocation.x, 
			inFilm->initialLocation.y, 
			inFilm->initialLocation.z);
	}
}