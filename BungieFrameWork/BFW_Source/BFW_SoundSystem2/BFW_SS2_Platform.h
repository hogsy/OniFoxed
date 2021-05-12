// ======================================================================
// BFW_SS2_Platform.h
// ======================================================================
#pragma once

#ifndef BFW_SS2_PLATFORM_H
#define BFW_SS2_PLATFORM_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_SoundSystem2.h"
#include "BFW_SS2_Private.h"

// ======================================================================
// prototypes
// ======================================================================
void
SS2rPlatform_SoundChannel_Initialize(
	SStSoundChannel				*inSoundChannel);

void
SS2rPlatform_SoundChannel_Pause(
	SStSoundChannel				*inSoundChannel);
	
void
SS2rPlatform_SoundChannel_Play(
	SStSoundChannel				*inSoundChannel);
	
void
SS2rPlatform_SoundChannel_Resume(
	SStSoundChannel				*inSoundChannel);
	
UUtBool
SS2rPlatform_SoundChannel_SetSoundData(
	SStSoundChannel				*inSoundChannel,
	SStSoundData				*inSoundData);
	
void
SS2rPlatform_SoundChannel_SetPan(
	SStSoundChannel				*inSoundChannel,
	UUtUns32					inPanFlag,
	float						inPan);

void
SSiSoundChannel_SetPaused(
	SStSoundChannel				*inSoundChannel,
	UUtBool						inPaused);
	
void
SS2rPlatform_SoundChannel_SetPitch(
	SStSoundChannel				*inSoundChannel,
	float						inPitch);

void
SS2rPlatform_SoundChannel_SetVolume(
	SStSoundChannel				*inSoundChannel,
	float						inVolume);
	
void
SS2rPlatform_SoundChannel_Silence(
	SStSoundChannel				*inSoundChannel);
	
void
SS2rPlatform_SoundChannel_Stop(
	SStSoundChannel				*inSoundChannel);
	
void
SS2rPlatform_SoundChannel_Terminate(
	SStSoundChannel				*inSoundChannel);
	
// ----------------------------------------------------------------------
UUtError
SS2rPlatform_Initialize(
	UUtWindow					inWindow,
	UUtUns32					*outNumChannels,
	UUtBool						inUseSound);

UUtError
SS2rPlatform_InitializeThread(
	void);
	
void
SS2rPlatform_TerminateThread(
	void);

void
SS2rPlatform_Terminate(
	void);

// ----------------------------------------------------------------------
void
SSrDeleteGuard(
	SStGuard					*inGuard);

void
SSrCreateGuard(
	SStGuard					**inGuard);

void
SSrReleaseGuard(
	SStGuard					*inGuard);

void
SSrWaitGuard(
	SStGuard					*inGuard);

// ----------------------------------------------------------------------
void
SS2rPlatform_GetDebugNeeds(
	UUtUns32					*outNumLines);

void
SS2rPlatform_ShowDebugInfo_Overall(
	IMtPoint2D					*inDest);
	
void
SS2rPlatform_PerformanceStartFrame(
	void);
	
void
SS2rPlatform_PerformanceEndFrame(
	void);
	
// ======================================================================
#endif /* BFW_SS2_PLATFORM_H */
