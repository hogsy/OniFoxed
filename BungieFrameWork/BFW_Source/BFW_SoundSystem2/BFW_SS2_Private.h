// ======================================================================
// BFW_SS2_Private.h
// ======================================================================
#ifndef BFW_SS2_PRIVATE_H
#define BFW_SS2_PRIVATE_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_SoundSystem2.h"

#if UUmOpenAL
	#include "BFW_SS2_Platform_OpenAL.h"
#elif UUmPlatform == UUmPlatform_Mac
	#include "BFW_SS2_Platform_MacOS.h"
#elif UUmPlatform == UUmPlatform_Win32
	#include "BFW_SS2_Platform_Win32.h"
#else
	#error Platform Undefined
#endif

// ======================================================================
// enums
// ======================================================================
enum
{
	SScSoundChannelFlag_None		= 0x00,
	SScSoundChannelFlag_Mono		= 0x01,
	SScSoundChannelFlag_Stereo		= 0x02
};

enum
{
	SScSCStatus_None				= 0x00,
	SScSCStatus_Playing				= 0x01,
	SScSCStatus_Looping				= 0x02,
	SScSCStatus_Locked				= 0x04,
	SScSCStatus_Updating			= 0x08,
	SScSCStatus_Paused				= 0x10,
	SScSCStatus_CanPan				= 0x40,
	SScSCStatus_Ambient				= 0x80
};

enum
{
	SScPanFlag_None					= 0x00,
	SScPanFlag_Left					= 0x01,
	SScPanFlag_Right				= 0x02
};

// ======================================================================
// typdefs
// ======================================================================
struct SStSoundChannel
{
	SStGuard						guard;
	UUtUns16						id;
	UUtUns8							flags;
	UUtUns8							status;
	SStPriority2					priority;
	SStGroup						*group;
	SStSoundData					*sound_data;
	M3tPoint3D						position;
	float							distance_to_listener;
	float							previous_distance_to_listener;
	float							volume;
	float							pitch;
	float							channel_volume;
	float							distance_volume;
	float							permutation_volume;
	
	float							pan_left;	// only used for debugging
	float							pan_right;	// only used for debugging
	
	SStSoundChannel_PlatformData	pd;
	
};

// ======================================================================
// globals
// ======================================================================
extern SStGuard						*SSgGuardAll;

// ======================================================================
// prototypes
// ======================================================================
UUtBool
SSiSoundChannel_CanPan(
	SStSoundChannel				*inSoundChannel);

UUtBool
SSiSoundChannel_IsAmbient(
	SStSoundChannel				*inSoundChannel);
	
UUtBool
SSiSoundChannel_IsLocked(
	SStSoundChannel				*inSoundChannel);
	
UUtBool
SSiSoundChannel_IsLooping(
	SStSoundChannel				*inSoundChannel);
	
UUtBool
SSiSoundChannel_IsPaused(
	SStSoundChannel				*inSoundChannel);
	
UUtBool
SSiSoundChannel_IsPlaying(
	SStSoundChannel				*inSoundChannel);
	
UUtBool
SSiSoundChannel_IsUpdating(
	SStSoundChannel				*inSoundChannel);
	
void
SSiSoundChannel_SetAmbient(
	SStSoundChannel				*inSoundChannel,
	UUtBool						inAmbient);

void
SSiSoundChannel_SetCanPan(
	SStSoundChannel				*inSoundChannel,
	UUtBool						inCanPan);

void
SSiSoundChannel_SetLock(
	SStSoundChannel				*inSoundChannel,
	UUtBool						inLocked);

void
SSiSoundChannel_SetLooping(
	SStSoundChannel				*inSoundChannel,
	UUtBool						inLooping);

void
SSiSoundChannel_SetPlaying(
	SStSoundChannel				*inSoundChannel,
	UUtBool						inPlaying);

void
SSiSoundChannel_SetUpdating(
	SStSoundChannel				*inSoundChannel,
	UUtBool						inUpdating);

// ----------------------------------------------------------------------
SStSoundChannel*
SSiSoundChannels_GetChannelByID(
	UUtUns32					inSoundChannelID);

UUtUns32
SSiSoundChannels_GetNumChannels(
	void);
	
// ----------------------------------------------------------------------
UUtBool
SSiPlayingAmbient_UpdateSoundChannel(
	SStSoundChannel				*inSoundChannel);
	
// ======================================================================
#endif /* BFW_SS2_PRIVATE_H */
