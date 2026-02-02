// ======================================================================
// Oni_Sound2.h
// ======================================================================
#pragma once

#ifndef ONI_SOUND2_H
#define ONI_SOUND2_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_SoundSystem2.h"
#include "BFW_AppUtilities.h"
#include "BFW_Particle3.h"

// ======================================================================
// defines
// ======================================================================
#define OScAmbientSuffix		"amb"
#define OScGroupSuffix			"grp"
#define OScImpulseSuffix		"imp"

#define OScBinaryDataClass		UUm4CharToUns32('O', 'S', 'B', 'D')

#define OScMusicScore_Lose		"loss_screen_music"
#define OScMusicScore_Win		"main_menu_win"

// ======================================================================
// enums
// ======================================================================
enum
{
	OS2cVersion_1			= 1,
	OS2cVersion_2			= 2,	/* added group_volume to SStGroup */
	OS2cVersion_3			= 3,	/* added group_pitch to SStGroup */
	OS2cVersion_4			= 4,	/* added alternate impulse to SStImpulse */
	OS2cVersion_5			= 5,	/* added threshold to SStAmbient */
	OS2cVersion_6			= 6,	/* added flag and flag_data to SStGroup  and min_occlusion to SStAmbient and SStImpulse */

	OS2cCurrentVersion		= OS2cVersion_6
};

// ======================================================================
// typedefs
// ======================================================================
#define OScTemplate_SoundBinaryData				UUm4CharToUns32('O', 'S', 'B', 'D')
typedef tm_template('O', 'S', 'B', 'D', "Oni Sound Binary Data")
OStBinaryData
{
	UUtUns32				data_size;
	tm_separate				data_index;

} OStBinaryData;

// ======================================================================
// global variables
// ======================================================================

extern UUtBool				OSgShowOcclusions;

// ======================================================================
// prototypes
// ======================================================================
void
OSrDebugDisplay(
	void);

// ----------------------------------------------------------------------
void
OSrAmbient_ChangeName(
	SStAmbient				*inAmbient,
	const char				*inName);

UUtError
OSrAmbient_Delete(
	const char				*inName);

SStAmbient*
OSrAmbient_GetByName(
	const char				*inName);

void
OSrAmbient_Halt(
	SStPlayID					inAmbientID);

UUtError
OSrAmbient_New(
	const char				*inName,
	SStAmbient				**outAmbient);

UUtError
OSrAmbient_Save(
	SStAmbient				*inAmbient,
	BFtFileRef				*inParentDirRef);

SStPlayID
OSrAmbient_Start(
	const SStAmbient			*inAmbient,
	const M3tPoint3D			*inPosition,		// inPosition == NULL will play non-spatial
	const M3tVector3D			*inDirection,		/* unused if inPosition == NULL */
	const M3tVector3D			*inVelocity,		/* unused if inPosition == NULL */
	const float					*inMaxVolDistance,	/* unused if inPosition == NULL */
	const float					*inMinVolDistance);	/* unused if inPosition == NULL */

void
OSrAmbient_Stop(
	SStPlayID					inAmbientID);

UUtBool
OSrAmbient_Update(
	SStPlayID					inAmbientID,
	const M3tPoint3D			*inPosition,
	const M3tVector3D			*inDirection,
	const M3tVector3D			*inVelocity);

// ----------------------------------------------------------------------
UUtError
OSrGroup_BuildHashTable(
	void);

void
OSrGroup_ChangeName(
	SStGroup				*inGroup,
	const char				*inName);

UUtError
OSrGroup_Delete(
	const char				*inName);

SStGroup*
OSrGroup_GetByName(
	const char				*inName);

UUtError
OSrGroup_New(
	const char				*inName,
	SStGroup				**outGroup);

UUtError
OSrGroup_Save(
	SStGroup				*inGroup,
	BFtFileRef				*inParentDirRef);

// ----------------------------------------------------------------------
void
OSrImpulse_ChangeName(
	SStImpulse				*inImpulse,
	const char				*inName);

UUtError
OSrImpulse_Delete(
	const char				*inName);

SStImpulse*
OSrImpulse_GetByName(
	const char				*inName);

UUtError
OSrImpulse_New(
	const char				*inName,
	SStImpulse				**outImpulse);

void
OSrImpulse_Play(
	SStImpulse				*inImpulse,
	const M3tPoint3D		*inPosition,
	const M3tVector3D		*inDirection,
	const M3tVector3D		*inVelocity,
	float					*inVolume);

void
OSrImpulse_PlayByName(
	const char				*inImpulseName,
	const M3tPoint3D		*inPosition,
	const M3tVector3D		*inDirection,
	const M3tVector3D		*inVelocity,
	float					*inVolume);

UUtError
OSrImpulse_Save(
	SStImpulse				*inImpulse,
	BFtFileRef				*inParentDirRef);

// ----------------------------------------------------------------------
UUtError
OSrGetSoundBinaryDirectory(
	BFtFileRef				**outDirRef);

UUtError
OSrGetSoundFileRef(
	const char				*inName,
	const char				*inSuffix,
	BFtFileRef				*inDirRef,
	BFtFileRef				**outFileRef);

// ----------------------------------------------------------------------
UUtError
OSrPlayList_Build(
	void);

UUtError
OSrSoundObjects_WriteAiffList(
	void);

UUtError
OSrNeutralInteractions_WriteAiffList(
	void);

void
OSrAmbient_AddToStringList(
	SStAmbient					*inAmbient,
	AUtSharedStringArray		*inStringArray);

// ----------------------------------------------------------------------
void
OSrListBrokenSounds(
	void);

UUtBool
OSrCheckBrokenAmbientSound(
	BFtFile						*inFile,
	char						*inCallerName,
	char						*inEventName,
	char						*inSoundName);

UUtBool
OSrCheckBrokenImpulseSound(
	BFtFile						*inFile,
	char						*inCallerName,
	char						*inEventName,
	char						*inSoundName);

// ----------------------------------------------------------------------
void
OSrMusic_Halt(
	void);

UUtBool
OSrMusic_IsPlaying(
	void);

void
OSrMusic_SetVolume(
	float					inVolume,
	float					inTime);

void
OSrMusic_Start(
	const char				*inMusicName,
	float					inVolume);

void
OSrMusic_Stop(
	void);

// ----------------------------------------------------------------------
void
OSrMusic_SetIsOn_Hook(UUtBool inIsOn);

void
OSrMusic_SetVolume_Hook(float oldVolume, float newVolume);

void
OSrSubtitle_Draw(
	SStAmbient				*inAmbient,
	UUtUns32				inDuration);

SStPlayID
OSrInGameDialog_Play(
	const char				*inName,
	M3tPoint3D				*inLocation,
	UUtUns32				*outDuration,
	UUtBool					*outSoundNotFound);

// ----------------------------------------------------------------------
UUtError
OSrInitialize(
	void);

UUtError
OSrLevel_Load(
	UUtUns32				inLevelNum);

void
OSrUpdateSoundPointers(
	void);

void
OSrLevel_Unload(
	void);

void
OSrMakeGoodName(
	const char				*inName,
	char					*outName);

void
OSrSetEnabled(
	UUtBool					inEnabled);

void
OSrSetScriptOnly(
	UUtBool					inIsScriptOnly);

void
OSrTerminate(
	void);

void
OSrUpdate(
	const M3tPoint3D		*inPosition,
	const M3tVector3D		*inFacing);

// ----------------------------------------------------------------------
UUtError
OSrRegisterTemplates(
	void);

// ----------------------------------------------------------------------
UUtBool
OSrFlybyProjectile_Create(
	P3tParticleClass		*inClass,
	P3tParticle				*inParticle);

UUtBool
OSrFlybyProjectile_Delete(
	P3tParticleClass		*inClass,
	P3tParticle				*inParticle);

// ======================================================================
#endif /* ONI_SOUND2_H */
