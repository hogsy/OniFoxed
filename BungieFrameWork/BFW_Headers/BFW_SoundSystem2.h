// ======================================================================
// BFW_SoundSystem2.h
// ======================================================================
#pragma once

#ifndef BFW_SOUNDSYSTEM2_H
#define BFW_SOUNDSYSTEM2_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"

// ======================================================================
// defines
// ======================================================================
#define SScMaxNameLength			(BFcMaxFileNameLength)

#define SScMaxPermutationsPerGroup	32

#define SScInvalidID				(0xFFFFFFFF)

#define SScFootToDist				0.333333333333333f // S.S.(1.0f / UUmFeet(1))

#define SScMSADPCMTimer				0

#define SScAmbientThreshold			3

#define SScInvalidSubtitle			((const char *) 0xFFFFFFFF)

#define SScPreventRepeats_Default	4
#define SScSamplesPerSecond			22050
#define SScBytesPerSample			2
#define SScBitsPerSample			16

// ======================================================================
// enums
// ======================================================================
typedef enum SStPriority2
{
	SScPriority2_Low,
	SScPriority2_Normal,
	SScPriority2_High,
	SScPriority2_Highest

} SStPriority2;

// ----------------------------------------------------------------------
// wave formats
enum
{
	SScWaveFormat_PCM				= 0x0001,
	SScWaveFormat_ADPCM				= 0x0002

};

// ----------------------------------------------------------------------
enum
{
	SScSoundDataFlag_None			= 0x0000,
	SScSoundDataFlag_Compressed		= 0x0001,		// this sound is compressed
	SScSoundDataFlag_Stereo			= 0x0002,

	SScSoundDataFlag_DynamicAlloc	= 0x8000		/* SStSoundData was dynamically allocated */

};

// ----------------------------------------------------------------------
enum
{
	SScAmbientFlag_None				= 0x0000,
	SScAmbientFlag_InterruptOnStop	= 0x0001,		// playing tracks are interrupted when stopped
	SScAmbientFlag_PlayOnce			= 0x0002,		// the base tracks are only played one time
	SScAmbientFlag_Pan				= 0x0004		// the ambient sound can be panned

};

// ----------------------------------------------------------------------
typedef enum SStType
{
	SScType_Ambient,
	SScType_Group,
	SScType_Impulse

} SStType;

// ----------------------------------------------------------------------
enum
{
	SScGroupFlag_None				= 0x0000,
	SScGroupFlag_PreventRepeats		= 0x0001,

	SScGroupFlag_WriteMask			= 0x000F
};

// ======================================================================
// typdefs
// ======================================================================
#define SScTemplate_Subtitle		UUm4CharToUns32('S', 'U', 'B', 'T')
typedef tm_template('S', 'U', 'B', 'T', "Subtitle Array")
SStSubtitle
{
	tm_pad					pad0[16];

	tm_raw(char *)			data;

	tm_varindex UUtUns32	numSubtitles;
	tm_vararray UUtUns32	subtitles[1];

} SStSubtitle;

// ----------------------------------------------------------------------
#if defined(UUmCompiler) && ((UUmCompiler == UUmCompiler_VisC) || (UUmCompiler == UUmCompiler_MWerks))
#pragma pack(push, 1)
#endif

typedef tm_struct SStADPCMCOEFSET
{
	UUtInt16				iCoef1;
	UUtInt16				iCoef2;

} SStADPCMCOEFSET;

typedef tm_struct SStFormat
{
	UUtUns16				wFormatTag;
	UUtUns16				nChannels;
	UUtUns32				nSamplesPerSec;
	UUtUns32				nAvgBytesPerSec;
	UUtUns16				nBlockAlign;
	UUtUns16				wBitsPerSample;
	UUtUns16				cbSize;

    UUtUns16	            wSamplesPerBlock;
    UUtUns16	            wNumCoef;
    SStADPCMCOEFSET		    aCoef[7];

} SStFormat;

#if UUmPlatform == UUmPlatform_Win32

typedef struct WAVEFORMAT_EX_ADPCM
{
	UUtUns16				wFormatTag;
	UUtUns16				nChannels;
	UUtUns32				nSamplesPerSec;
	UUtUns32				nAvgBytesPerSec;
	UUtUns16				nBlockAlign;
	UUtUns16				wBitsPerSample;
	UUtUns16				cbSize;

    UUtUns16	            wSamplesPerBlock;
    UUtUns16	            wNumCoef;
    SStADPCMCOEFSET		    aCoef[7];
} WAVEFORMAT_EX_ADPCM;

extern WAVEFORMAT_EX_ADPCM SSgWaveFormat_Mono;
extern WAVEFORMAT_EX_ADPCM SSgWaveFormat_Stereo;

#endif

#if defined(UUmCompiler) && ((UUmCompiler == UUmCompiler_VisC) || (UUmCompiler == UUmCompiler_MWerks))
#pragma pack(pop)
#endif

#define SScTemplate_SoundData		UUm4CharToUns32('S', 'N', 'D', 'D')
typedef tm_template('S', 'N', 'D', 'D', "Sound Data")
SStSoundData
{
	UUtUns32				flags;

	//SStFormat				f;
	UUtUns16				duration_ticks;
	UUtUns16				pad;

	UUtUns32				num_bytes;
	tm_raw(void*)			data;

} SStSoundData;

static UUcInline UUtBool SSrSound_IsStereo(SStSoundData *inSoundData)
{
	UUtBool is_stereo = (inSoundData->flags & SScSoundDataFlag_Stereo) > 0;

	return is_stereo;
}

static UUcInline UUtUns32 SSrSound_GetNumChannels(SStSoundData *inSoundData)
{
	UUtUns32 num_channels = (inSoundData->flags & SScSoundDataFlag_Stereo) ? 2 : 1;

	return num_channels;
}

// ----------------------------------------------------------------------
typedef struct SStPermutation
{
	UUtUns32				weight;

	float					min_volume_percent;		// the minimum percent from max volume (ex: 0.0)
	float					max_volume_percent;		// the maximum percent from max volume (ex: 1.0)

	float					min_pitch_percent;		// the minimum percent from normal pitch (ex: 0.7)
	float					max_pitch_percent;		// the maximum percent from normal pitch (ex: 1.2)

	SStSoundData			*sound_data;
	char					sound_data_name[SScMaxNameLength];

} SStPermutation;

typedef struct SStGroup
{
	char					group_name[SScMaxNameLength];

	float					group_volume;
	float					group_pitch;

	UUtUns16				flags;
	UUtUns16				flag_data;

	UUtUns32				num_channels;
	UUtMemory_Array			*permutations;

} SStGroup;

// ----------------------------------------------------------------------
/*
	Ambient Sounds are used for several things: Dialog, Music, Environmental
	Ambient Sounds, and Spatial Ambient Sounds.  Game State Music and
	Environmental Ambient Sounds behave exactly the same, they are just used
	in different ways.

	Ambient Sounds have a detail track and two base tracks.

	The in sound, out sound, and base track 1 must all be mono or must all
	be stereo.

	The permutation from the detail track will be played at a time between
	the min and max detail times.

	If a position is provided for the sound, it will be played as a Spatial
	Ambient Sound.  The min and max volume distance will be used to set the
	maximum volume for all of the tracks.  The detail track	will be placed
	at the provided position.

	If no position is provided, only the detail track uses the minimum and
	maximum volume distance.  The detail track will be positioned randomly
	within the sphere radius.  The sphere radius cannot exceed the size of
	the minimum volume distance.

	Dialog uses base track 1.  The in sound, out sound, base track 2 and
	the detail sound can be used, but only base track 1 is actually used.
*/
typedef struct SStAmbient
{
	char					ambient_name[SScMaxNameLength];

	SStPriority2			priority;			// priority of the group
	UUtUns32				flags;				// flags for how the ambient functions

	float					sphere_radius;		// radius of sphere at the sound's location.
												// Sound are randomly positioned within the sphere.

	float					min_detail_time;	// minimum amount of time that must pass before a
												// detail sound is played
	float					max_detail_time;	// maximum amount of time that can pass before a
												// detail sound is played

	float					max_volume_distance;	// distance at which the maximum volume is heard
	float					min_volume_distance;	// distance at which the minimum volume is heard

	UUtUns32				threshold;			// maximimum number of this ambient which can be playing at a time
	float					min_occlusion;		// minimum occlusion percentage

	SStGroup				*detail;			// group of sounds to be played as hilites
	SStGroup				*base_track1;		// group of sounds to be played in a loop
	SStGroup				*base_track2;		// group of sounds to be played in a loop
	SStGroup				*in_sound;			// sound played at the beginning of the ambient sound
	SStGroup				*out_sound;			// sound played at the end of the ambient sound

	char					detail_name[SScMaxNameLength];
	char					base_track1_name[SScMaxNameLength];
	char					base_track2_name[SScMaxNameLength];
	char					in_sound_name[SScMaxNameLength];
	char					out_sound_name[SScMaxNameLength];

	const char				*subtitle;			// subtitle associated with this ambient sound

} SStAmbient;

// ----------------------------------------------------------------------
// Impulse sounds are meant to be used for sounds that have a distinct
// position in the world.  They do not loop.
typedef struct SStImpulse
{
	char					impulse_name[SScMaxNameLength];

	SStGroup				*impulse_group;			// group of impulse sounds
	char					impulse_group_name[SScMaxNameLength];

	SStPriority2			priority;				// priority of the group

	float					max_volume_distance;	// distance at which the maximum volume is heard
	float					min_volume_distance;	// distance at which the minimum volume is heard

	float					max_volume_angle;		// angle at which the maximum volume is heard
	float					min_volume_angle;		// angle at which the minimum volume is heard
	float					min_angle_attenuation;	// ammount of attenuation at min_volume_angle

	UUtUns32				alt_threshold;			// max number of the SStGroup of this impulse that can be playing at once
	struct SStImpulse		*alt_impulse;			// pointer to impulse sound
	char					alt_impulse_name[SScMaxNameLength]; // impulse to play if alt_threshold copies of this impulse are playing

	// this should really live in the impact effects, but it is easier to put it here this late in the project
	float					impact_velocity;		// velocity at which an impact plays at full volume
	float					min_occlusion;			// minimum occlusion percentage

} SStImpulse;

// ----------------------------------------------------------------------
typedef UUtUns32			SStPlayID;
typedef struct SStSoundChannel		SStSoundChannel;

// ----------------------------------------------------------------------
// Function callbacks for creation and deletion of impulse and ambient sound...
// allows the rest of the engine to safely use pointers to sound objects.
typedef void (*SStImpulsePointerHandler)(SStImpulse *inImpulseSound, UUtBool inDelete);
typedef void (*SStAmbientPointerHandler)(SStAmbient *inAmbientSound, UUtBool inDelete);

// ----------------------------------------------------------------------
// Function callbacks for sound data loading
typedef UUtError
(*SStLoadCallback_StoreSoundBlock)(
	SStSoundData			*inSoundData,
	const char				*inSoundName);

// ======================================================================
// globals
// ======================================================================
extern UUtBool					SSgShowDebugInfo;
extern UUtBool					SSgSearchOnDisk;
extern UUtBool					SSgSearchBinariesOnDisk;

// ======================================================================
// prototypes
// ======================================================================

const char *SSrSubtitle_Find(const char *inName);
const char *SSrMessage_Find(const char *inName);
UUtBool SSrNameIsValidDialogName(const char *inName);

// ----------------------------------------------------------------------

typedef enum SStCompressionMode{
	SScCompressionMode_PC,
	SScCompressionMode_Mac
} SStCompressionMode;

extern SStCompressionMode SSgCompressionMode;

// ----------------------------------------------------------------------

void
SS2rFrame_Start(
	void);

void
SS2rFrame_End(
	void);

// ----------------------------------------------------------------------
void
SSrDetails_SetCanPlay(
	UUtBool						inCanPlay);

UUtError
SSrGetSoundDirectory(
	BFtFileRef					**outDirectoryRef);

UUtError
SS2rInitializeBasic(
	UUtWindow					inWindow,
	UUtBool						inUseSound);

UUtError
SS2rInitializeFull(
	void);

UUtBool
SS2rEnabled(
	void);

void
SSrLevel_Begin(
	void);

void
SSrPlayingChannels_Pause(
	void);

void
SSrPlayingChannels_Resume(
	void);

UUtError
SS2rRegisterTemplates(
	void);

void
SSrStopAll(
	void);

void UUcExternal_Call
SS2rTerminate(
	void);

void
SS2rSoundData_FlushDeallocatedPointers(
	void);

void
SS2rUpdate(
	void);

float
SS2rVolume_Get(
	void);

float
SS2rVolume_Set(
	float						inVolume);

void
SS2rInstallPointerHandlers(
	SStImpulsePointerHandler	inImpulseHandler,
	SStAmbientPointerHandler	inAmbientHandler);

// ----------------------------------------------------------------------
void
SSrShowDebugInfo(
	void);

// ----------------------------------------------------------------------
void
SSrListener_SetPosition(
	const M3tPoint3D			*inPosition,
	const M3tVector3D			*inFacing);

// ----------------------------------------------------------------------
UUtBool
SSrAmbient_CheckSoundData(
	const SStAmbient			*inAmbient);

UUtError
SSrAmbient_Copy(
	const SStAmbient			*inSource,
	SStAmbient					*ioDest);

void
SSrAmbient_Delete(
	const char					*inAmbientName,
	UUtBool						inUpdatePointers);

SStAmbient*
SSrAmbient_GetByIndex(
	UUtUns32					inIndex);

SStAmbient*
SSrAmbient_GetByName(
	const char					*inAmbientName);

UUtUns32
SSrAmbient_GetNumAmbientSounds(
	void);

const char*
SSrAmbient_GetSubtitle(
	SStAmbient					*inAmbient);

void
SSrAmbient_Halt(
	SStPlayID					inPlayID);

UUtError
SSrAmbient_New(
	const char					*inAmbientName,
	SStAmbient					**outAmbient);

void
SSrAmbient_SetPitch(
	SStPlayID					inPlayID,
	float						inPitch);

void
SSrAmbient_SetVolume(
	SStPlayID					inPlayID,
	float						inVolume,
	float						inTime);			// seconds

SStPlayID
SSrAmbient_Start(
	const SStAmbient			*inAmbient,
	const M3tPoint3D			*inPosition,		// inPosition == NULL will play non-spatial
	const M3tVector3D			*inDirection,		/* unused if inPosition == NULL */
	const M3tVector3D			*inVelocity,		/* unused if inPosition == NULL */
	const float					inMaxVolDistance,	/* unused if inPosition == NULL */
	const float					inMinVolDistance,	/* unused if inPosition == NULL */
	const float					*inVolume);

SStPlayID
SSrAmbient_Start_Simple(
	const SStAmbient			*inAmbient,
	const float					*inVolume);

void
SSrAmbient_Stop(
	SStPlayID					inPlayID);

UUtBool
SSrAmbient_Update(
	SStPlayID					inPlayID,
	const M3tPoint3D			*inPosition,
	const M3tVector3D			*inDirection,
	const M3tVector3D			*inVelocity,
	const float					*inVolume);

void
SSrAmbient_UpdateGroupName(
	const char					*inOldGroupName,
	const char					*inNewGroupName);

void
SSrAmbient_UpdateGroupPointers(
	void);

// ----------------------------------------------------------------------
UUtError
SSrImpulse_Copy(
	const SStImpulse			*inSource,
	SStImpulse					*ioDest);

void
SSrImpulse_Delete(
	const char					*inImpulseName,
	UUtBool						inUpdatePointers);

SStImpulse*
SSrImpulse_GetByIndex(
	UUtUns32					inIndex);

SStImpulse*
SSrImpulse_GetByName(
	const char					*inImpulseName);

UUtUns32
SSrImpulse_GetNumImpulseSounds(
	void);

UUtError
SSrImpulse_New(
	const char					*inImpulseName,
	SStImpulse					**outImpulse);


void
SSrImpulse_Play_Simple(
	const char					*inImpulseName,
	const M3tPoint3D			*inPosition);

void
SSrImpulse_Play(
	SStImpulse					*inImpulse,
	const M3tPoint3D			*inPosition,
	const M3tVector3D			*inDirection,
	const M3tVector3D			*inVelocity,
	const float					*inVolume);

void
SSrImpulse_UpdateGroupName(
	const char					*inOldGroupName,
	const char					*inNewGroupName);

void
SSrImpulse_UpdateGroupPointers(
	void);

// ----------------------------------------------------------------------
const char*
SSrPermutation_GetName(
	SStPermutation				*inPermutation);

void
SSrPermutation_Play(
	SStPermutation				*inPermutation,
	SStSoundChannel				*inSoundChannel,	/* optional */
	const char					*inDebugSoundType,
	const char					*inDebugSoundName);

// ----------------------------------------------------------------------
UUtBool
SSrGroup_CheckSoundData(
	const SStGroup				*inGroup);

UUtError
SSrGroup_Copy(
	const SStGroup				*inSource,
	SStGroup					*ioDest);

void
SSrGroup_Delete(
	const char					*inGroupName,
	UUtUns32					inUpdatePointers);

SStGroup*
SSrGroup_GetByIndex(
	UUtUns32					inIndex);

SStGroup*
SSrGroup_GetByName(
	const char					*inGroupName);

UUtUns32
SSrGroup_GetNumChannels(
	const SStGroup				*inGroup);

UUtUns32
SSrGroup_GetNumPermutations(
	const SStGroup				*inGroup);

UUtUns32
SSrGroup_GetNumSoundGroups(
	void);

UUtError
SSrGroup_New(
	const char					*inGroupName,
	SStGroup					**outGroup);

void
SSrGroup_Permutation_Delete(
	SStGroup					*ioGroup,
	UUtUns32					inPermIndex);

SStPermutation*
SSrGroup_Permutation_Get(
	SStGroup					*inGroup,
	UUtUns32					inPermIndex);

UUtError
SSrGroup_Permutation_New(
	SStGroup					*ioGroup,
	SStSoundData				*inSoundData,
	UUtUns32					*outPermutationIndex);

void
SSrGroup_Play(
	SStGroup					*inGroup,
	SStSoundChannel				*inSoundChannel,	/* optional */
	const char					*inDebugSoundType,
	const char					*inDebugSoundName);

void
SSrGroup_UpdateSoundDataPointers(
	void);

// ----------------------------------------------------------------------
UUtUns32
SSrSoundData_GetDuration(
	SStSoundData				*inSoundData);

SStSoundData*
SSrSoundData_GetByName(
	const char					*inSoundDataName,
	UUtBool						inLookOnDisk);

void SSrSoundData_GetByName_StartCache(void);
void SSrSoundData_GetByName_StopCache(void);

const char*
SSrSoundData_GetName(
	const SStSoundData			*inSoundData);

UUtError
SSrSoundData_Load(
	BFtFileRef					*inFileRef,
	SStLoadCallback_StoreSoundBlock inStoreSoundCallback,
	SStSoundData				**outSoundData);

UUtError
SSrSoundData_New(
	BFtFileRef					*inFileRef,
	SStSoundData				**outSoundData);

UUtError
SSrSoundData_Process(
	const char					*inDebugName,
	UUtUns8						*inFileData,
	UUtUns32					inFileDataLength,
	SStSoundData				*outSoundData,
	UUtUns8						**outRawData,
	UUtUns32					*outRawDataLength);

// ----------------------------------------------------------------------
UUtError
SSrTextFile_Write(
	void);

UUtError
SSrListBrokenSounds(
	BFtFile						*inFile);

// ======================================================================
typedef enum UUtFileEndian
{
	UUcFile_LittleEndian,
	UUcFile_BigEndian
} UUtFileEndian;

typedef struct UUtTag
{
	UUtUns8						tag[4];		/* 4 chars */
	UUtUns32					size;		/* size of data following UUtTag */
} UUtTag;

UUtBool
UUrFindTag(
	UUtUns8						*inSource,
	UUtUns32					inSourceLength,
	UUtUns32					inTag,
	UUtUns8						**outData);

UUtError
UUrFindTagData(
	UUtUns8						*inSource,
	UUtUns32					inSourceLength,
	UUtFileEndian				inFileEndian,
	UUtUns32					inTag,
	UUtUns8						**outData,
	UUtUns32					*outLength);

UUtUns32
UUrWriteTagDataToBuffer(
	UUtUns8						*inDestinationBuffer,
	UUtUns32					inDestinationSize,
	UUtUns32					inTag,
	UUtUns8						*inSourceData,
	UUtUns32					inSourceDataLength,
	UUtFileEndian				inWriteEndian);

// ======================================================================
#endif /* BFW_SOUNDSYSTEM_H */
