/*
	FILE:	Oni_Speech.c
	
	AUTHOR:	Chris Butcher
	
	CREATED: July 15, 2000
	
	PURPOSE: Speech handling for Oni Characters
	
	Copyright (c) 2000

*/

#include "BFW_SoundSystem2.h"

#include "Oni_Character.h"
#include "Oni_Speech.h"
#include "Oni_Sound2.h"

// ------------------------------------------------------------------------------------
// -- external globals

const char *ONcSpeechTypeName[ONcSpeechType_Max] =
		{"none", "idle", "chatter", "say", "call", "pain", "yell", "override"};

// ------------------------------------------------------------------------------------
// -- internal function prototypes

static void ONiSpeech_Play(ONtCharacter *ioCharacter)
{
	UUmAssert((ioCharacter->speech.current.speech_type > ONcSpeechType_None) && 
			(ioCharacter->speech.current.speech_type < ONcSpeechType_Max));
	UUmAssert(!ioCharacter->speech.currently_speaking);

	ioCharacter->speech.currently_speaking = UUcTrue;
	ioCharacter->speech.played_sound = UUcFalse;
	ioCharacter->speech.finished_sound = UUcFalse;
}

// ------------------------------------------------------------------------------------
// -- query and command routines

// clear initial state
void ONrSpeech_Initialize(ONtCharacter *ioCharacter)
{
	ioCharacter->speech.currently_speaking = UUcFalse;
	ioCharacter->speech.played_sound = UUcFalse;
	ioCharacter->speech.finished_sound = UUcFalse;
	ioCharacter->speech.playing_sound_endtime = 0;
	ioCharacter->speech.playing_sound_id = SScInvalidID;

	ioCharacter->speech.current.speech_type = ONcSpeechType_None;
	ioCharacter->speech.next.speech_type	= ONcSpeechType_None;
}

// say something
UUtUns32 ONrSpeech_Say(ONtCharacter *ioCharacter, ONtSpeech *inSpeech, UUtBool inDontQueue)
{
	UUmAssert((inSpeech->speech_type > ONcSpeechType_None) && (inSpeech->speech_type < ONcSpeechType_Max));

	if (inSpeech->notify_string_inuse != NULL) {
		*(inSpeech->notify_string_inuse) = UUcTrue;
	}

	if ((inSpeech->speech_type == ONcSpeechType_Override) ||
		(inSpeech->speech_type > ioCharacter->speech.current.speech_type)) {
		// override the currently-playing speech (or it could be silence)
		ONrSpeech_Stop(ioCharacter, UUcFalse);
		ioCharacter->speech.current = *inSpeech;
		ONiSpeech_Play(ioCharacter);
		return ONcSpeechResult_Played;

	} else if (!inDontQueue && (inSpeech->speech_type > ioCharacter->speech.next.speech_type)) {
		// queue up this speech item
		ioCharacter->speech.next = *inSpeech;
		return ONcSpeechResult_Queued;

	} else {
		if (inSpeech->notify_string_inuse != NULL) {
			*(inSpeech->notify_string_inuse) = UUcFalse;
		}

		return ONcSpeechResult_Discarded;
	}
}

// stop any current speech
void ONrSpeech_Stop(ONtCharacter *ioCharacter, UUtBool inAbort)
{
	if (ioCharacter->speech.currently_speaking) {
		if (ioCharacter->speech.playing_sound_id != SScInvalidID) {
			if (inAbort) {
				OSrAmbient_Halt(ioCharacter->speech.playing_sound_id);
			} else {
				OSrAmbient_Stop(ioCharacter->speech.playing_sound_id);
			}
			ioCharacter->speech.playing_sound_id = SScInvalidID;
		}
		ioCharacter->speech.currently_speaking = UUcFalse;
	}

	ioCharacter->speech.current.speech_type = ONcSpeechType_None;
	ioCharacter->speech.next.speech_type = ONcSpeechType_None;
}

// currently saying something?
UUtBool ONrSpeech_Saying(struct ONtCharacter *ioCharacter)
{
	return ioCharacter->speech.currently_speaking;
}

// what are we currently saying?
ONtSpeech *ONrSpeech_Current(ONtCharacter *ioCharacter)
{
	if (ioCharacter->speech.currently_speaking) {
		UUmAssert((ioCharacter->speech.current.speech_type > ONcSpeechType_None) && 
				(ioCharacter->speech.current.speech_type < ONcSpeechType_Max));
		return &ioCharacter->speech.current;
	} else {
		return NULL;
	}
}

// ------------------------------------------------------------------------------------
// -- update routines

// update for a tick
void ONrSpeech_Update(ONtCharacter *ioCharacter)
{
	UUtUns32 current_time, duration;

	if (!ioCharacter->speech.currently_speaking)
		return;

	if (ioCharacter->speech.current.pre_pause > 0) {
		ioCharacter->speech.current.pre_pause--;
		if (ioCharacter->speech.current.pre_pause > 0) {
			// wait until pre-pause is over
			return;
		}
	}

	current_time = ONrGameState_GetGameTime();

	if (!ioCharacter->speech.finished_sound) {
		if (ioCharacter->speech.played_sound) {
			UUmAssert(ioCharacter->speech.playing_sound_id != SScInvalidID);

			// update the spatial position of our sound
			if (!OSrAmbient_Update(ioCharacter->speech.playing_sound_id, &ioCharacter->location, NULL, NULL)) {
				ioCharacter->speech.playing_sound_id = SScInvalidID;
				ioCharacter->speech.finished_sound = UUcTrue;
			}

			if (!ioCharacter->speech.finished_sound) {
				if (current_time > ioCharacter->speech.playing_sound_endtime) {
					// continue to the next line
					ioCharacter->speech.playing_sound_id = SScInvalidID;
					ioCharacter->speech.finished_sound = UUcTrue;
				}
			}
		} else {
			M3tPoint3D speech_location;
			UUtBool sound_missing;

			UUmAssert(ioCharacter->speech.playing_sound_id == SScInvalidID);

			ONrCharacter_GetEyePosition(ioCharacter, &speech_location);
			ioCharacter->speech.playing_sound_id = OSrInGameDialog_Play(ioCharacter->speech.current.sound_name, &speech_location, &duration, &sound_missing);
			ioCharacter->speech.playing_sound_endtime = current_time + duration;
			
			if (sound_missing == UUcTrue) {
				COrConsole_Printf("### %s: can't find sound '%s'", ioCharacter->player_name, ioCharacter->speech.current.sound_name);
				ioCharacter->speech.finished_sound = UUcTrue;
			}
			if (ioCharacter->speech.playing_sound_id == SScInvalidID) {
				ioCharacter->speech.finished_sound = UUcTrue;
			}
			ioCharacter->speech.played_sound = UUcTrue;

			if (ioCharacter->speech.current.notify_string_inuse != NULL) {
				*(ioCharacter->speech.current.notify_string_inuse) = UUcFalse;
			}
		}
	}

	if (!ioCharacter->speech.finished_sound) {
		// wait for our sound to finish
		return;
	}

	if (ioCharacter->speech.current.post_pause > 0) {
		ioCharacter->speech.current.post_pause--;
		if (ioCharacter->speech.current.post_pause > 0) {
			return;
		}
	}

	// done!
	ioCharacter->speech.currently_speaking = UUcFalse;
	ioCharacter->speech.played_sound = UUcFalse;
	ioCharacter->speech.finished_sound = UUcFalse;

	if (ioCharacter->speech.next.speech_type == ONcSpeechType_None) {
		// nothing else to say
		ioCharacter->speech.current.speech_type = ONcSpeechType_None;
	} else {
		// go to the next speech item and clear the queue
		ioCharacter->speech.current = ioCharacter->speech.next;
		ioCharacter->speech.next.speech_type = ONcSpeechType_None;

		ONiSpeech_Play(ioCharacter);
	}
}
