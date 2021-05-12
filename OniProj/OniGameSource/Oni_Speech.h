#pragma once
#ifndef ONI_SPEECH_H
#define ONI_SPEECH_H

/*
	FILE:	Oni_Speech.h
	
	AUTHOR:	Chris Butcher
	
	CREATED: July 15, 2000
	
	PURPOSE: Speech handling for Oni Characters
	
	Copyright (c) 2000

*/

#include "Oni_Character.h"

// ------------------------------------------------------------------------------------
// -- constants

enum {
	ONcSpeechType_None			= 0,
	ONcSpeechType_IdleSound		= 1,	// yawns, etc
	ONcSpeechType_Chatter		= 2,	// unimportant speech
	ONcSpeechType_Say			= 3,	// speech with someone; or talking to one's self
	ONcSpeechType_Call			= 4,	// speech directed at someone, maybe at a distance
	ONcSpeechType_Pain			= 5,	// misc. grunting
	ONcSpeechType_Yell			= 6,	// cries of pain, for help, etc
	ONcSpeechType_Override		= 7,	// for overriding other types
	ONcSpeechType_Max			= 8
};

enum {
	ONcSpeechResult_Played		= 0,
	ONcSpeechResult_Queued		= 1,
	ONcSpeechResult_Discarded	= 2
};

extern const char *ONcSpeechTypeName[];

// ------------------------------------------------------------------------------------
// -- structures

typedef struct ONtSpeech {
	UUtUns32 speech_type;
	UUtUns32 pre_pause;
	UUtBool played;
	UUtBool *notify_string_inuse;
	char *sound_name;
	UUtUns32 post_pause;
} ONtSpeech;

typedef struct ONtCharacterSpeech {
	UUtBool	currently_speaking;
	UUtBool	played_sound;
	UUtBool finished_sound;

	SStPlayID playing_sound_id;
	UUtUns32 playing_sound_endtime;
	
	ONtSpeech current;
	ONtSpeech next;

} ONtCharacterSpeech;

// ------------------------------------------------------------------------------------
// -- function prototypes

// clear initial state
void ONrSpeech_Initialize(struct ONtCharacter *ioCharacter);

// say something
UUtUns32 ONrSpeech_Say(struct ONtCharacter *ioCharacter, ONtSpeech *inSpeech, UUtBool inDontQueue);

// stop any current speech
void ONrSpeech_Stop(struct ONtCharacter *ioCharacter, UUtBool inAbort);

// currently saying something?
UUtBool ONrSpeech_Saying(struct ONtCharacter *ioCharacter);

// what are you currently saying?
ONtSpeech *ONrSpeech_Current(struct ONtCharacter *ioCharacter);

// update for a tick
void ONrSpeech_Update(struct ONtCharacter *ioCharacter);

#endif // ONI_SPEECH_H
