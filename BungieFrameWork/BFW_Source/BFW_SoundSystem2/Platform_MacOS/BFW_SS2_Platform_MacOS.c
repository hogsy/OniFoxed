// ======================================================================
// BFW_SS2_Platform_MacOS.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_SoundSystem2.h"
#include "BFW_SS2_Private.h"
#include "BFW_SS2_Platform.h"
#include "BFW_Console.h"
#include "BFW_SS2_IMA.h"

#include <sound.h>

// ======================================================================
// defines
// ======================================================================
#define SScMaxSoundChannels				16
#define SSmSetSndCommand(a,b,c,d)		{a.cmd = b; a.param1 = c; a.param2 = d;}

#if defined(DEBUGGING) && DEBUGGING
	#define SSmCheckSndError(err)		if (err != noErr) \
										{UUrError_Report(UUcError_Generic, "Sound Error.");}
#else
	#define SSmCheckSndError(err)
#endif

#define SScPlatform_MaxVolume			256.0f

// ======================================================================
// globals
// ======================================================================
static SndCallBackUPP			SSgSoundCallbackUPP;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
SS2iPlatform_SoundChannel_ApplyVolume(
	SStSoundChannel				*inSoundChannel)
{
	SndCommand					cmd;
	OSErr						err;
	UUtUns32					volume;
	UUtUns16					left_volume;
	UUtUns16					right_volume;
	
	// calculate the left and right volumes
	left_volume =
			(UUtUns16)(inSoundChannel->pd.volume *
			SScPlatform_MaxVolume *
			inSoundChannel->pd.left_pan);
	right_volume =
		(UUtUns16)(inSoundChannel->pd.volume *
			SScPlatform_MaxVolume*
			inSoundChannel->pd.right_pan);
	
	// set the volume
	volume = UUmMakeLong(right_volume, left_volume);
		
	// set the channel volume
	SSmSetSndCommand(cmd, volumeCmd, 0, volume);
	err = SndDoImmediate(inSoundChannel->pd.sound_channel, &cmd);
	SSmCheckSndError(err);
}

// ----------------------------------------------------------------------
static void
SSiPlatform_SoundChannel_Callback(
	SndChannelPtr			inSoundChannel,
	SndCommand				*inSoundCommand)
{
	SStSoundChannel			*sound_channel;
	
	// get the sound channel being used
	sound_channel = (SStSoundChannel*)inSoundCommand->param2;
	
	if (SSiSoundChannel_IsLooping(sound_channel))
	{
		// play another permutation in the group
		SSrGroup_Play(sound_channel->group, sound_channel, "sound channel", NULL);
	}
	else
	{
		// the sound channel is no longer playing
		SSiSoundChannel_SetPlaying(sound_channel, UUcFalse);
		if (SSiSoundChannel_IsAmbient(sound_channel) == UUcTrue)
		{
			SSiPlayingAmbient_UpdateSoundChannel(sound_channel);
		}
	}
}

// ----------------------------------------------------------------------
static void
SSiPlatform_SoundChannel_Terminate(
	SStSoundChannel				*inSoundChannel,
	char						*inErrorString)
{
	OSErr					err;
	
	if (inErrorString)
	{
		UUrError_Report(UUcError_Generic, inErrorString);
	}
	
	if (inSoundChannel->pd.sound_channel)
	{
		// stop the sound channel
		err = 
			SndDisposeChannel(
				inSoundChannel->pd.sound_channel,
				UUcTrue);
		if (err != noErr)
		{
			UUrError_Report(UUcError_Generic, "Unable to dispose of the sound channel");
		}
		inSoundChannel->pd.sound_channel = NULL;
	}

	if (inSoundChannel->pd.sound)
	{
		// release the memory used by the sound
		UUrMemory_Block_Delete(inSoundChannel->pd.sound);
		inSoundChannel->pd.sound = NULL;
	}
}

// ----------------------------------------------------------------------
void
SS2rPlatform_SoundChannel_Initialize(
	SStSoundChannel				*inSoundChannel)
{
	OSErr						err;
	UUtInt32					init_flag;
	
	// clear the vars
	inSoundChannel->pd.sound_channel	= NULL;
	inSoundChannel->pd.sound			= NULL;
	inSoundChannel->pd.volume			= 1.0f;
	inSoundChannel->pd.left_pan			= 1.0f;
	inSoundChannel->pd.right_pan		= 1.0f;
	inSoundChannel->pd.pitch			= (UUcMaxUns16 + 1);
	
	// ------------------------------
	// Sound Channel
	// ------------------------------
	if ((inSoundChannel->flags & SScSoundChannelFlag_Mono) != 0)
	{
		init_flag = initMono;
	}
	else
	{
		init_flag = initStereo;
	}
	
	// intialize the sound channel
	err =
		SndNewChannel(
			&inSoundChannel->pd.sound_channel,
			sampledSynth,
			init_flag,
			SSgSoundCallbackUPP);
	if (err != noErr)
	{
		SSiPlatform_SoundChannel_Terminate(
			inSoundChannel,
			"Unable to initialize the sound channel.");
		return;
	}
	
	// ------------------------------
	// Sound
	// ------------------------------
	// allocate memory for the sound
	inSoundChannel->pd.sound =
		(SoundHeaderUnion*)UUrMemory_Block_NewClear(
			sizeof(SoundHeaderUnion));
	if (inSoundChannel->pd.sound == NULL)
	{
		SSiPlatform_SoundChannel_Terminate(
			inSoundChannel,
			"Unable to allocate memory for the sound");
	}
}

// ----------------------------------------------------------------------
void
SS2rPlatform_SoundChannel_Play(
	SStSoundChannel				*inSoundChannel)
{
	SndCommand					cmd;
	OSErr						err;
	
	UUmAssert(inSoundChannel->pd.sound_channel);
	UUmAssert(inSoundChannel->pd.sound);
	
	// set the status field
	SSiSoundChannel_SetPlaying(inSoundChannel, UUcTrue);

	// play the sound
	SSmSetSndCommand(cmd, bufferCmd, 0, (UUtUns32)inSoundChannel->pd.sound);
	err = SndDoCommand(inSoundChannel->pd.sound_channel, &cmd, UUcTrue);
	if (err != noErr) { goto cleanup; }
	
	// queue the callback
	SSmSetSndCommand(cmd, callBackCmd, 0, (UUtUns32)inSoundChannel);
	err = SndDoCommand(inSoundChannel->pd.sound_channel, &cmd, UUcTrue);
	if (err != noErr) { goto cleanup; }
	
	return;
	
cleanup:
	SSmSetSndCommand(cmd, quietCmd, 0, 0);
	SndDoImmediate(inSoundChannel->pd.sound_channel, &cmd);
	
	// set the status field
	SSiSoundChannel_SetPlaying(inSoundChannel, UUcTrue);
}

// ----------------------------------------------------------------------
UUtBool
SS2rPlatform_SoundChannel_SetSoundData(
	SStSoundChannel				*inSoundChannel,
	SStSoundData				*inSoundData)
{
//	UUtUns32					num_frames;
	UUtUns32					num_channels =  SSrSound_GetNumChannels(inSoundData);
	
	UUmAssert(inSoundChannel->pd.sound_channel);
	UUmAssert(inSoundChannel->pd.sound);
	
	// make sure a mono channel plays on a mono sound
	if (((inSoundChannel->flags & SScSoundChannelFlag_Mono) == SScSoundChannelFlag_Mono) &&
		(num_channels != 1))
	{
		return UUcFalse;
	}

	// make sure a stereo channel plays on a stereo sound
	if (((inSoundChannel->flags & SScSoundChannelFlag_Stereo) == SScSoundChannelFlag_Stereo) &&
		(num_channels != 2))
	{
		return UUcFalse;
	}
	
#if 0
	num_frames =
		(SScBitsPerSample == 8) ?
			(inSoundData->num_bytes / num_channels) :
			((inSoundData->num_bytes >> 1) / num_channels);
#endif

	// setup the appropriate sound header
	if (1)
	{
		CmpSoundHeader			*cmp_sound;
		UUtUns32				packet_count = inSoundData->num_bytes / (num_channels * sizeof(SStIMA_SampleData));
			
		cmp_sound = (CmpSoundHeader*)inSoundChannel->pd.sound;
		UUrMemory_Clear(cmp_sound, sizeof(CmpSoundHeader));
		
		// setup the compressed sound header
		cmp_sound->samplePtr		= (char*)inSoundData->data;
		cmp_sound->numChannels		= num_channels;
		cmp_sound->sampleRate		= SScSamplesPerSecond << 16;
//		cmp_sound->loopStart		= 0;
//		cmp_sound->loopEnd			= 0;
		cmp_sound->encode			= cmpSH;
		cmp_sound->baseFrequency	= kMiddleC;
		cmp_sound->numFrames		= packet_count;
//		cmp_sound->markerChunk		= NULL;
		cmp_sound->format			= kIMACompression;
//		cmp_sound->futureUse2		= 0;
//		cmp_sound->stateVars		= NULL;
//		cmp_sound->leftOverSamples	= NULL;
		cmp_sound->compressionID	= fixedCompression;
//		cmp_sound->packetSize		= 64;
//		cmp_sound->snthID			= 0;
		cmp_sound->sampleSize		= SScBitsPerSample;
	}
	
#if 0
	else if ((inSoundData->flags & SScSoundDataFlag_PCM_Little) != 0)
	{
		CmpSoundHeader			*cmp_sound;
		
		cmp_sound = (CmpSoundHeader*)inSoundChannel->pd.sound;
		UUrMemory_Clear(cmp_sound, sizeof(CmpSoundHeader));
		
		// setup the compressed sound header
		cmp_sound->samplePtr		= (char*)inSoundData->data;
		cmp_sound->numChannels		= inSoundData->f.nChannels;
		cmp_sound->sampleRate		= inSoundData->f.nSamplesPerSec << 16;
//		cmp_sound->loopStart		= 0;
//		cmp_sound->loopEnd			= 0;
		cmp_sound->encode			= cmpSH;
		cmp_sound->baseFrequency	= kMiddleC;
		cmp_sound->numFrames		= num_frames;
//		cmp_sound->markerChunk		= NULL;
		cmp_sound->format			= k16BitLittleEndianFormat;
//		cmp_sound->futureUse2		= 0;
//		cmp_sound->stateVars		= NULL;
//		cmp_sound->leftOverSamples	= NULL;
		cmp_sound->compressionID	= notCompressed;
//		cmp_sound->packetSize		= 0;
//		cmp_sound->snthID			= 0;
		cmp_sound->sampleSize		= inSoundData->f.wBitsPerSample;
	}
	else if ((inSoundData->flags & SScSoundDataFlag_PCM_Big) != 0)
	{
		ExtSoundHeader			*ext_sound;
		
		ext_sound = (ExtSoundHeader*)inSoundChannel->pd.sound;
		UUrMemory_Clear(ext_sound, sizeof(ExtSoundHeader));
	
		// setup the extended sound header
		ext_sound->samplePtr		= (char*)inSoundData->data;
		ext_sound->numChannels		= inSoundData->f.nChannels;
		ext_sound->sampleRate		= inSoundData->f.nSamplesPerSec << 16;
//		ext_sound->loopStart		= 0;
//		ext_sound->loopEnd			= 0;
		ext_sound->encode			= extSH;
		ext_sound->baseFrequency	= kMiddleC;
		ext_sound->numFrames		= num_frames;
//		ext_sound->markerChunk		= NULL;
//		ext_sound->instrumentChunks	= NULL;
//		ext_sound->AESRecording		= NULL;
		ext_sound->sampleSize		= inSoundData->f.wBitsPerSample;
//		ext_sound->futureUse1		= 0;
//		ext_sound->futureUse2		= 0;
//		ext_sound->futureUse3		= 0;
//		ext_sound->futureUse4		= 0;
	}
#endif
	inSoundChannel->sound_data = inSoundData;

	return UUcTrue;
}

// ----------------------------------------------------------------------
void
SS2rPlatform_SoundChannel_SetPan(
	SStSoundChannel				*inSoundChannel,
	UUtUns32					inPanFlag,
	float						inPan)
{
	// set the pan
	switch (inPanFlag)
	{
		case SScPanFlag_None:
			// no panning
			inSoundChannel->pd.left_pan = 1.0f;
			inSoundChannel->pd.right_pan = 1.0f;
		break;
		
		case SScPanFlag_Left:
			// lower right volume
			inSoundChannel->pd.left_pan = 1.0f;
			inSoundChannel->pd.right_pan = inPan;
		break;
		
		case SScPanFlag_Right:
			// lower right volume
			inSoundChannel->pd.left_pan = inPan;
			inSoundChannel->pd.right_pan = 1.0f;
		break;
	}
	
	// apply the pan
	SS2iPlatform_SoundChannel_ApplyVolume(inSoundChannel);
}
	
// ----------------------------------------------------------------------
void
SS2rPlatform_SoundChannel_SetPitch(
	SStSoundChannel				*inSoundChannel,
	float						inPitch)
{
	SndCommand					cmd;
	OSErr						err;
	UUtUns32					rate;
	
	// calculate the rate
	rate = (UUtUns32)(inPitch * (float)(UUcMaxUns16 + 1));
	inSoundChannel->pitch = rate;
	
	// set the rate
	if (!(inSoundChannel->status & SScSCStatus_Paused)) {
		SSmSetSndCommand(cmd, rateCmd, 0, rate);
		err = SndDoImmediate(inSoundChannel->pd.sound_channel, &cmd);
		SSmCheckSndError(err);
	}

	return;
}

// ----------------------------------------------------------------------
void
SS2rPlatform_SoundChannel_SetVolume(
	SStSoundChannel				*inSoundChannel,
	float						inVolume)
{
	// set the volume
	inSoundChannel->pd.volume = inVolume;
	
	// apply the volume
	SS2iPlatform_SoundChannel_ApplyVolume(inSoundChannel);
}

// ----------------------------------------------------------------------
void
SS2rPlatform_SoundChannel_Stop(
	SStSoundChannel				*inSoundChannel)
{
	SndCommand					cmd;
	OSErr						err;
	
	// flush the commands from the channel
	SSmSetSndCommand(cmd, flushCmd, 0, 0);
	err = SndDoImmediate(inSoundChannel->pd.sound_channel, &cmd);
	SSmCheckSndError(err);
	
	// stop the sound that is playing
	SSmSetSndCommand(cmd, quietCmd, 0, 0);
	err = SndDoImmediate(inSoundChannel->pd.sound_channel, &cmd);
	SSmCheckSndError(err);

	// the sound channel is no longer playing
	SSiSoundChannel_SetPlaying(inSoundChannel, UUcFalse);
}

// ----------------------------------------------------------------------
void
SS2rPlatform_SoundChannel_Terminate(
	SStSoundChannel				*inSoundChannel)
{
	SSiPlatform_SoundChannel_Terminate(inSoundChannel, NULL);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
SS2rPlatform_Initialize(
	UUtWindow					inWindow,			/* only used by Win32 */
	UUtUns32					*outNumChannels,
	UUtBool						inUseSound)
{
	NumVersion					sound_manager_version;
	OSErr						err;
	UUtInt32					response;
	
	// ------------------------------
	// make sure the needed sound capabilities exist on this hardware
	// ------------------------------
	
	// get the sound manager version
	sound_manager_version = SndSoundManagerVersion();
	if (sound_manager_version.majorRev < 3) { return UUcError_Generic; }
	if ((sound_manager_version.majorRev == 3) && (sound_manager_version.minorAndBugRev < 3))
	{
		return UUcError_Generic;
	}
	
	// gestalt the sound attributes
	err = Gestalt(gestaltSoundAttr, &response);
	if (err != noErr) { return UUcError_Generic; }
	
	// check for needed properties
	if ((response & (1 << gestaltStereoCapability)) == 0)	{ return UUcError_Generic; }
	if ((response & (1 << gestaltMultiChannels)) == 0)		{ return UUcError_Generic; }
	if ((response & (1 << gestalt16BitSoundIO)) == 0)		{ return UUcError_Generic; }
	if ((response & (1 << gestalt16BitAudioSupport)) == 0)	{ return UUcError_Generic; }
	
	// ------------------------------
	// Create the Sound Callback
	// ------------------------------
	// create the sound channel callback
	//SSgSoundCallbackUPP = (SndCallBackUPP)NewSndCallBackProc(SSiPlatform_SoundChannel_Callback);
	SSgSoundCallbackUPP = (SndCallBackUPP)(SSiPlatform_SoundChannel_Callback);
	if (SSgSoundCallbackUPP == NULL)
	{
		UUrError_Report(UUcError_Generic, "Unable to create sound callback UPP");
		return UUcError_Generic;
	}
	
	// ------------------------------
	// set the number of sound channels
	*outNumChannels = SScMaxSoundChannels;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
SS2rPlatform_InitializeThread(
	void)
{
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
SS2rPlatform_TerminateThread(
	void)
{
	// CB: not necessary under MacOS
}

// ----------------------------------------------------------------------
void
SS2rPlatform_Terminate(
	void)
{
	// free up the callback routine
	if (SSgSoundCallbackUPP)
	{
		/* Carbon
		DisposeRoutineDescriptor((UniversalProcPtr) SSgSoundCallbackUPP);*/
		DisposeSndCallBackUPP(SSgSoundCallbackUPP);
		SSgSoundCallbackUPP = NULL;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
SSrDeleteSemaphore(
	SStSemaphore				inSemaphore)
{
}

// ----------------------------------------------------------------------
void
SSrCreateSemaphore(
	SStSemaphore				*outSemaphore)
{
	*outSemaphore = 0;
}

// ----------------------------------------------------------------------
void
SSrReleaseSemaphore(
	SStSemaphore				inSemaphore)
{
}

// ----------------------------------------------------------------------
void
SSrWaitSemaphore(
	SStSemaphore				inSemaphore)
{
}

/* S.S. added these on 10/01/2000 */
void SS2rPlatform_ShowDebugInfo_Overall(
	IMtPoint2D *inDest)
{
	return;
}

void SS2rPlatform_GetDebugNeeds(
	UUtUns32 *outNumLines)
{
	*outNumLines= 0L;
	
	return;
}

void SS2rPlatform_PerformanceStartFrame(
	void)
{
	return;
}
	
void SS2rPlatform_PerformanceEndFrame(
	void)
{
	return;
}

UUtUns32 SSrMSADPCM_CalculateNumSamples(
	SStSoundData *inSoundData)
{
	return 0L;
}

/* S.S. added these on 10/17/2000 */
void SSrDeleteGuard(
	SStGuard *inGuard)
{
	return;
}

void SSrCreateGuard(
	SStGuard **inGuard)
{
	return;
}

void SSrReleaseGuard(
	SStGuard *inGuard)
{
	return;
}

void SSrWaitGuard(
	SStGuard *inGuard)
{
	return;
}

/* S.S. added these on 10/23/2000 */

void
SS2rPlatform_SoundChannel_Pause(
	SStSoundChannel *inSoundChannel)
{
	SndCommand					cmd;
	OSErr						err;
	
	UUmAssert(inSoundChannel);
	
	// make sure the channel is playing
	if (SSiSoundChannel_IsPlaying(inSoundChannel) == UUcFalse) { return; }
	
	SSmSetSndCommand(cmd, rateCmd, 0, 0);
	err = SndDoImmediate(inSoundChannel->pd.sound_channel, &cmd);
	SSmCheckSndError(err);
	
	// set the status field
	SSiSoundChannel_SetPaused(inSoundChannel, UUcTrue);

	return;
}

void
SS2rPlatform_SoundChannel_Resume(
	SStSoundChannel *inSoundChannel)
{
	SndCommand					cmd;
	OSErr						err;
	
	UUmAssert(inSoundChannel);
	
	// make sure the channel is playing and paused
	if ((SSiSoundChannel_IsPlaying(inSoundChannel) == UUcFalse) ||
		(SSiSoundChannel_IsPaused(inSoundChannel) == UUcFalse))
	{
		return;
	}
				
	SSmSetSndCommand(cmd, rateCmd, 0, inSoundChannel->pd.pitch);
	err = SndDoImmediate(inSoundChannel->pd.sound_channel, &cmd);
	SSmCheckSndError(err);

	// set the status field
	SSiSoundChannel_SetPaused(inSoundChannel, UUcFalse);

	return;
}

/* S.S. added these 10/27/2000 */

void SS2rPlatform_SoundChannel_Silence(
	SStSoundChannel *inSoundChannel)
{
	return;
}
	

/* end S.S. */

// ======================================================================
