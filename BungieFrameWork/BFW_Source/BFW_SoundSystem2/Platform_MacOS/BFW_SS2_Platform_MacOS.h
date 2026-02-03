// ======================================================================
// BFW_SS2_Platform_MacOS.h
// ======================================================================
#pragma once

#ifndef BFW_SS2_PLATFORM_MACOS_H
#define BFW_SS2_PLATFORM_MACOS_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

#include <sound.h>

// ======================================================================
// typedefs
// ======================================================================
typedef UUtInt32				SStSemaphore;
typedef UUtInt32				SStGuard;

typedef struct SStSoundChannel_PlatformData
{
	SndChannelPtr				sound_channel;
	SoundHeaderUnion			*sound;
	float						volume;
	UUtUns32					pitch;
	float						left_pan;
	float						right_pan;

} SStSoundChannel_PlatformData;

// ======================================================================
#endif /* BFW_SS2_PLATFORM_MACOS_H */
