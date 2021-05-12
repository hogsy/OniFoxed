// ======================================================================
// BFW_SS_Platform_MacOS.h
// ======================================================================
#ifndef BFW_SS_PLATFORM_MACOS_H
#define BFW_SS_PLATFORM_MACOS_H

// ======================================================================
// includes
// ======================================================================
#include <Sound.h>

// ======================================================================
// typedefs
// ======================================================================
typedef struct SStPlatformSpecific
{
	SndChannelPtr					sound_channel;
	ExtSoundHeaderPtr				sound;
	UUtInt32						pan;
	UUtInt32						volume;
	UUtInt16						status;
	
} SStPlatformSpecific;

// ======================================================================
#endif /* BFW_SS_PLATFORM_MACOS_H */
