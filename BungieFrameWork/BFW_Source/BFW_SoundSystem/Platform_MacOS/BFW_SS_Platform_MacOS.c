
// ======================================================================
// BFW_SS_Platform_MacOS.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_SoundSystem.h"
#include "BFW_SS_Platform.h"

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

// ======================================================================
// globals
// ======================================================================
static UUtBool					SSgPlatformInitialized;
static SndCallBackUPP			SSgSoundCallbackUPP;


// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
static void
SSiPlatform_SoundChannel_Callback(
	SndChannelPtr			inSoundChannel,
	SndCommand				*inSoundCommand)
{
	SStSoundChannel			*soundChannel;

	// get the sound channel being used
	soundChannel = (SStSoundChannel*)inSoundCommand->param2;

	if (soundChannel->flags & SScFlag_Loop)
	{
		SndCommand			cmd;
		OSErr				err;

		// play the sound again
		SSmSetSndCommand(cmd, bufferCmd, 0, (UUtUns32)soundChannel->ps.sound);
		err = SndDoImmediate(soundChannel->ps.sound_channel, &cmd);
		if (err != noErr)
		{
			// XXX - may need to do something here
		}
	}
	else
	{
		soundChannel->ps.status = SScFlag_None;
	}
}

// ----------------------------------------------------------------------
static void
SSiPlatform_SoundChannel_Terminate(
	SStSoundChannel			*inSoundChannel,
	char					*inErrorString)
{
	OSErr					err;

	if (inErrorString)
	{
		UUrError_Report(UUcError_Generic, inErrorString);
	}

	if (inSoundChannel->ps.sound_channel)
	{
		// stop the sound channel
		err =
			SndDisposeChannel(
				inSoundChannel->ps.sound_channel,
				UUcTrue);
		if (err != noErr)
		{
			UUrError_Report(UUcError_Generic, "Unable to dispose of the sound channel");
		}
		inSoundChannel->ps.sound_channel = NULL;
	}

	if (inSoundChannel->ps.sound)
	{
		// release the memory used by the sound
		UUrMemory_Block_Delete(inSoundChannel->ps.sound);
		inSoundChannel->ps.sound = NULL;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
SSrPlatform_Reverb_Set(
	SStReverb_Property		inReverbProperties)
{
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
SSrPlatform_SoundChannel_Initialize(
	SStSoundChannel			*inSoundChannel)
{
	OSErr					err;
	ExtSoundHeaderPtr		sound;

	UUmAssert(SSgPlatformInitialized);

	// clear the vars
	inSoundChannel->ps.sound_channel	= NULL;
	inSoundChannel->ps.sound			= NULL;
	inSoundChannel->ps.pan				= SScPan_Middle;
	inSoundChannel->ps.volume			= SScVolume_Max;
	inSoundChannel->ps.status			= SScStatus_None;

	// ------------------------------
	// Sound Channel
	// ------------------------------
	// initialize the sound channel
	err =
		SndNewChannel(
			&inSoundChannel->ps.sound_channel,
			sampledSynth,
			initStereo,
			SSgSoundCallbackUPP);
	if (err != noErr)
	{
		SSiPlatform_SoundChannel_Terminate(
			inSoundChannel,
			"Unable to initialize the sound channel");
		return;
	}

	// ------------------------------
	// Sound
	// ------------------------------
	// allocate memory for the sound
	inSoundChannel->ps.sound =
		(ExtSoundHeaderPtr)UUrMemory_Block_NewClear(sizeof(ExtSoundHeader));
	if (inSoundChannel->ps.sound == NULL)
	{
		SSiPlatform_SoundChannel_Terminate(
			inSoundChannel,
			"Unable to allocate memory for the sound");
	}

	// get a pointer to the sound
	sound = inSoundChannel->ps.sound;

	// initialize the sound
	sound->samplePtr			= NULL;
	sound->numChannels			= 0;
	sound->sampleRate			= 0;
	sound->numFrames			= 0;
	sound->sampleSize			= 0;

	sound->loopStart			= 0;
	sound->loopEnd				= 0;
	sound->encode				= extSH;
	sound->baseFrequency		= kMiddleC;		// Middle 'C'
	sound->sampleArea[0]		= NULL;
}

// ----------------------------------------------------------------------
UUtUns16
SSrPlatform_SoundChannel_GetStatus(
	SStSoundChannel			*inSoundChannel)
{
	UUmAssert(SSgPlatformInitialized);

	return inSoundChannel->ps.status;
}

// ----------------------------------------------------------------------
void
SSrPlatform_SoundChannel_Play(
	SStSoundChannel			*inSoundChannel)
{
	SndCommand				cmd;
	OSErr					err;

	UUmAssert(SSgPlatformInitialized);

	// flush the sound channels command buffer
	SSmSetSndCommand(cmd, flushCmd, 0, 0);
	err = SndDoImmediate(inSoundChannel->ps.sound_channel, &cmd);
	SSmCheckSndError(err);

	// set the volume
	SSrPlatform_SoundChannel_SetVolume(inSoundChannel, inSoundChannel->ps.volume);

	// set the pan
	SSrPlatform_SoundChannel_SetPan(inSoundChannel, inSoundChannel->ps.pan);

	// set the status field
	inSoundChannel->ps.status = SScStatus_Playing;
	if (inSoundChannel->flags & SScFlag_Loop)
	{
		inSoundChannel->ps.status = SScStatus_Looping;
	}

	// play the sound
	SSmSetSndCommand(cmd, bufferCmd, 0, (UUtUns32)inSoundChannel->ps.sound);
	err = SndDoImmediate(inSoundChannel->ps.sound_channel, &cmd);
	SSmCheckSndError(err);

	// queue a callback
	if (err == noErr)
	{
		SSmSetSndCommand(cmd, callBackCmd, 0, (UUtUns32)inSoundChannel);
		err = SndDoCommand(inSoundChannel->ps.sound_channel, &cmd, UUcTrue);
		SSmCheckSndError(err);
	}
}

// ----------------------------------------------------------------------
void
SSrPlatform_SoundChannel_Set3DParams(
	SStSoundChannel			*inSoundChannel,
	const M3tPoint3D		*inPosition,
	const M3tVector3D		*inVelocity,
	const M3tVector3D		*inOrientation)
{
	UUmAssert(SSgPlatformInitialized);
}

// ----------------------------------------------------------------------
void
SSrPlatform_SoundChannel_SetPan(
	SStSoundChannel			*inSoundChannel,
	UUtInt32				inPan)
{
	OSErr					err;
	SndCommand				cmd;
	UUtUns16				right_volume;
	UUtUns16				left_volume;
	float					scale;

	UUmAssert(SSgPlatformInitialized);

	// set the new pan
	inSoundChannel->ps.pan = inPan;

	// set up the volumes
	right_volume = left_volume = inSoundChannel->ps.volume;


	// calculate the new pan
	if (inPan > SScPan_Middle)
	{
		// pan right
		scale = (float)inPan / (float)(SScPan_Right - SScPan_Middle);
		left_volume = (UUtUns32)((float)left_volume * scale);
	}
	else if (inPan < SScPan_Middle)
	{
		// pan left
		scale = (float)inPan / (float)(SScPan_Left - SScPan_Middle);
		right_volume = (UUtUns32)((float)right_volume * scale);
	}

	// set the sound channel's pan
	SSmSetSndCommand(cmd, volumeCmd, 0, UUmMakeLong(right_volume, left_volume));
	err = SndDoImmediate(inSoundChannel->ps.sound_channel, &cmd);
	SSmCheckSndError(err);
}

// ----------------------------------------------------------------------
UUtBool
SSrPlatform_SoundChannel_SetSound(
	SStSoundChannel			*inSoundChannel,
	SStSound				*inSound)
{
	ExtSoundHeaderPtr		sound;

	UUmAssert(SSgPlatformInitialized);

	// if inSound == NULL, then exit, the buffer has been cleared
	if (inSound == NULL)
	{
		return UUcTrue;
	}

	// get a pointer to the sound
	sound = inSoundChannel->ps.sound;

	// set up the sound
	sound->samplePtr			= (char*)inSound->data;
	sound->numChannels			= inSound->numChannels;
	sound->sampleRate			= inSound->sampleRate << 16;
	sound->numFrames			=
		(inSound->numBits == 8) ?
			(inSound->numBytes) / inSound->numChannels :
			(inSound->numBytes / 2) / inSound->numChannels;
	sound->sampleSize			= inSound->numBits;

	// set the sound
	inSoundChannel->sound = inSound;

	return UUcTrue;
}

// ----------------------------------------------------------------------
void
SSrPlatform_SoundChannel_SetVolume(
	SStSoundChannel			*inSoundChannel,
	UUtInt32				inVolume)
{
	UUmAssert(SSgPlatformInitialized);

	// set the volume variable
	inSoundChannel->ps.volume = inVolume;

	// let SSrPlatform_SoundChannel_SetPan() set the volume for the
	// left and right channels
	SSrPlatform_SoundChannel_SetPan(
		inSoundChannel,
		inSoundChannel->ps.pan);
}

// ----------------------------------------------------------------------
void
SSrPlatform_SoundChannel_Stop(
	SStSoundChannel			*inSoundChannel)
{
	OSErr					err;
	SndCommand				cmd;

	UUmAssert(SSgPlatformInitialized);

	// stop the sound channel
	SSmSetSndCommand(cmd, quietCmd, 0, 0);
	err = SndDoImmediate(inSoundChannel->ps.sound_channel, &cmd);
	SSmCheckSndError(err);

	// flush the queue
	SSmSetSndCommand(cmd, flushCmd, 0, 0);
	err = SndDoImmediate(inSoundChannel->ps.sound_channel, &cmd);
	SSmCheckSndError(err);

	// set the status field
	inSoundChannel->ps.status = SScStatus_None;
}

// ----------------------------------------------------------------------
void
SSrPlatform_SoundChannel_Terminate(
	SStSoundChannel			*inSoundChannel)
{
	SSiPlatform_SoundChannel_Terminate(inSoundChannel, NULL);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
SSrPlatform_3DListener_SetParams(
	M3tPoint3D				*inPosition,
	M3tVector3D				*inVelocity,
	M3tVector3D				*inOrientationFront,
	M3tVector3D				*inOrientationTop)
{
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
SSrPlatform_Initialize(
	UUtWindow				inWindow)
{
	NumVersion				sound_manager_version;
	OSErr					err;
	UUtInt32				response;

	// ------------------------------
	// make sure the needed sound capabilities exist on this hardware
	// ------------------------------

	// get the sound manager version
	sound_manager_version = SndSoundManagerVersion();
	if (sound_manager_version.majorRev < 3)	return UUcError_Generic;

	// gestalt the sound attributes
	err = Gestalt(gestaltSoundAttr, &response);
	if (err != noErr)	return UUcError_Generic;

	// check for needed properties
	if (!(response & (1 << gestaltStereoCapability)))	return UUcError_Generic;
	if (!(response & (1 << gestaltMultiChannels)))		return UUcError_Generic;
	if (!(response & (1 << gestalt16BitSoundIO)))		return UUcError_Generic;
	if (!(response & (1 << gestalt16BitAudioSupport)))	return UUcError_Generic;

	// ------------------------------
	// Sound Callback
	// ------------------------------
	// create the sound channel callback
	SSgSoundCallbackUPP = NewSndCallBackProc(SSiPlatform_SoundChannel_Callback);
	if (SSgSoundCallbackUPP == NULL)
	{
		UUrError_Report(UUcError_Generic, "Unable to create sound callback UPP");
		return UUcError_Generic;
	}

	#if 0
	// install the byte swapper
	error =
		TMrTemplate_InstallByteSwap(
			SScTemplate_Sound,
			SSiPlatform_SoundByteSwapper);
	UUmError_ReturnOnErrorMsg(error, "Could not install sound byte swapper");
	#endif

	// init the main vars
	SSgPlatformInitialized	= UUcTrue;

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
SSrPlatform_Terminate(
	void)
{
	// free up the callback routine
	if (SSgSoundCallbackUPP)
	{
		DisposeRoutineDescriptor((UniversalProcPtr) SSgSoundCallbackUPP);
		SSgSoundCallbackUPP = NULL;
	}

	SSgPlatformInitialized = UUcFalse;
}

// ----------------------------------------------------------------------
void
SSrPlatform_Frame_Start(
	void)
{
	if (!SSgPlatformInitialized) return;
}

// ----------------------------------------------------------------------
void
SSrPlatform_Frame_End(
	void)
{
	if (!SSgPlatformInitialized) return;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
SSrPlatform_Sound_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData)
{
	SStSound				*sound;
	SStSound_PrivateData	*private_data;

	// get the data
	sound = (SStSound*)inInstancePtr;
	private_data = (SStSound_PrivateData*)inPrivateData;
	UUmAssert(private_data);

	// handle the message
	switch(inMessage)
	{
		case TMcTemplateProcMessage_LoadPostProcess:
			if ((!private_data->byte_swapped) &&
				(sound->numBits == 16))
			{
				UUtUns32		i;
				UUtUns32		num_shorts;
				UUtUns16		*data;

				// setup the vars
				num_shorts = sound->numBytes / 2;
				data = (UUtUns16*)sound->data;

				// swap all of the data
				for (i = 0; i < num_shorts; i++)
				{
					UUrSwap_2Byte(data++);
				}

				private_data->byte_swapped = UUcTrue;
			}
		break;
	}

	return UUcError_None;
}

