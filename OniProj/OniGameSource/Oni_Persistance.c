/*
	FILE:	Oni_Persistance.c

	AUTHOR:	Michael Evans

	CREATED: June 19, 2000

	PURPOSE: Persistant data storage

	Copyright (c) Microsoft

*/

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_BitVector.h"
#include "BFW_Console.h"
#include "BFW_SoundSystem2.h"

#include "Oni_Persistance.h"
#include "Oni_GameState.h"
#include "Oni_Motoko.h"
#include "Oni_Sound2.h"
#include "Oni_Particle3.h"

enum
{
	ONcOptionFlag_SubtitlesOn		= 0x0001,
	ONcOptionFlag_InvertMouseOn		= 0x0002,
	ONcOptionFlag_WonGame			= 0x0004
};

#define ONcPersistance_Version	15
#define ONcPersistance_SwapCode	0xd0d00b0e
#define ONcPersistance_FileName "persist.dat"


typedef struct ONtPersistance
{
	UUtUns32 version;
	UUtUns32 swap_code;
	UUtUns32 level_bit_field[8];
	UUtUns32 we_killed_griffen;

	// persistent UI state
	UUtUns32 weapon_bit_field;
	UUtUns32 item_bit_field;
	UUtUns32 max_diary_level;
	UUtUns32 max_diary_page;

	// game options
	ONtGraphicsQuality quality;
	float overall_volume;
	UUtUns32 option_flags;
	ONtDifficultyLevel difficulty;
	M3tDisplayMode resolution;
	float gamma;
	ONtPlace place;

	ONtContinue continues[ONcPersist_NumLevels][ONcPersist_NumContinues];			// we save up to 20 levels and 10 continues per level

} ONtPersistance;

static ONtPersistance ONgPersistance;

#if UUmPlatform == UUmPlatform_Win32
ONtGraphicsQuality ONrPersistance_GraphicsQuality_GetDefault(void)
{
	ONtGraphicsQuality default_graphics_quality;
	MEMORYSTATUS memory_status;
	UUtUns32 ram_60_megabytes = 1024 * 1024 * 60;
	UUtUns32 ram_90_megabytes = 1024 * 1024 * 90;

	GlobalMemoryStatus(&memory_status);

	if (memory_status.dwTotalPhys >= ram_90_megabytes) {
		default_graphics_quality = ONcGraphicsQuality_2;
	}
	else if (memory_status.dwTotalPhys >= ram_60_megabytes) {
		default_graphics_quality = ONcGraphicsQuality_1;
	}
	else {
		default_graphics_quality = ONcGraphicsQuality_0;
	}

	return default_graphics_quality;
}
#else
ONtGraphicsQuality ONrPersistance_GraphicsQuality_GetDefault(void)
{
	ONtGraphicsQuality default_graphics_quality = ONcGraphicsQuality_Default;

	return default_graphics_quality;
}
#endif

void ONrPersistance_Initialize(void)
{
	BFtFile *stream;
	UUtError error;
	UUtBool invalid_file;

	stream = BFrFile_FOpen(ONcPersistance_FileName, "r");
	invalid_file = (NULL == stream);

	if (!invalid_file) {
		error = BFrFile_Read(stream, sizeof(ONtPersistance), &ONgPersistance);
		BFrFile_Close(stream);
	}

	invalid_file = invalid_file || (UUcError_None != error);
	invalid_file = invalid_file || (ONcPersistance_Version != ONgPersistance.version);
	invalid_file = invalid_file || (ONcPersistance_SwapCode != ONgPersistance.swap_code);

	if (invalid_file) {
		UUrMemory_Clear(&ONgPersistance, sizeof(ONgPersistance));
		ONgPersistance.version = ONcPersistance_Version;
		ONgPersistance.swap_code = ONcPersistance_SwapCode;

		// persistant UI state
		ONgPersistance.weapon_bit_field = 0;
		ONgPersistance.item_bit_field = 0;
		ONgPersistance.max_diary_level = 0;
		ONgPersistance.max_diary_page = 0;

		// game options
		ONgPersistance.quality = ONrPersistance_GraphicsQuality_GetDefault();
		ONgPersistance.overall_volume = 1.f;
		ONgPersistance.option_flags = 0;

		if (!SS2rEnabled()) {
			// CB: enable subtitles by default only on machines without sound
			ONgPersistance.option_flags |= ONcOptionFlag_SubtitlesOn;
		}

		ONgPersistance.difficulty = ONcDifficultyLevel_Default;
		ONgPersistance.resolution.bitDepth = 32;
		ONgPersistance.resolution.width = 640;
		ONgPersistance.resolution.height = 480;
		ONgPersistance.option_flags |= ONcOptionFlag_InvertMouseOn;
		ONgPersistance.gamma = 0.5f;
		ONgPersistance.place.level = 0;
		ONgPersistance.place.save_point = 0;

		ONgPersistance.continues[1][1].continue_flags = ONcContinueFlag_Valid | ONcContinueFlag_Ignore_Restore;
		strcpy(ONgPersistance.continues[1][1].name, "Syndicate Warehouse");
	}

	return;
}

static void ONrPersist(void)
{
	BFtFile *stream = BFrFile_FOpen(ONcPersistance_FileName, "w");
	UUtError error;

	if (NULL != stream) {
		error = BFrFile_Write(stream, sizeof(ONtPersistance), &ONgPersistance);
		BFrFile_Close(stream);
	}

	return;
}

void ONrUnlockLevel(UUtUns16 inLevel)
{
	if ((inLevel > 1) && (inLevel <= ONcPersist_NumLevels)) {
		UUtBool was_avaiable;

		was_avaiable = UUrBitVector_TestAndSetBit(ONgPersistance.level_bit_field, inLevel);

		if (!was_avaiable) {
			ONrPersist();
		}
	}
}

UUtBool ONrLevelIsUnlocked(UUtUns16 inLevel)
{
	UUtBool level_available;

	if (1 == inLevel) {
		level_available = UUcTrue;
	}
	else if (inLevel > ONcPersist_NumLevels) {
		level_available = UUcFalse;
	}
	else {
		level_available = UUrBitVector_TestBit(ONgPersistance.level_bit_field, inLevel);
	}

	return level_available;
}

void ONrWeKilledGriffen(UUtBool inMurder)
{
	if (inMurder) {
		COrConsole_Printf("we killed griffen");
	}
	else {
		COrConsole_Printf("we did not kill griffen");
	}

	ONgPersistance.we_killed_griffen = inMurder;
	ONrPersist();

	return;
}

UUtBool ONrDidWeKillGriffen(void)
{
	UUtBool murder = ONgPersistance.we_killed_griffen > 0;

	return murder;
}

// game options
ONtGraphicsQuality ONrPersist_GetGraphicsQuality(void)
{
	return ONgPersistance.quality;
}

void ONrPersist_SetGraphicsQuality(ONtGraphicsQuality inQuality)
{
	ONgPersistance.quality = inQuality;
	ONrPersist();

	// apply the changes before level reload (this is useful for authoring)
	ONrParticle3_UpdateGraphicsQuality();

	return;
}

float ONrPersist_GetOverallVolume(void)
{
	return ONgPersistance.overall_volume;
}

void ONrPersist_SetOverallVolume(float inVolume)
{
	ONgPersistance.overall_volume = inVolume;
	ONrPersist();

	SS2rVolume_Set(inVolume);

	return;
}

UUtBool ONrPersist_IsInvertMouseOn(void)
{
	UUtBool invert_mouse_on = ((ONgPersistance.option_flags & ONcOptionFlag_InvertMouseOn) != 0);

	return invert_mouse_on;
}

UUtBool ONrPersist_AreSubtitlesOn(void)
{
	UUtBool subtitles_are_on = ((ONgPersistance.option_flags & ONcOptionFlag_SubtitlesOn) != 0);

	return subtitles_are_on;
}

void ONrPersist_SetInvertMouseOn(UUtBool inOn)
{
	if (inOn) {
		ONgPersistance.option_flags |= ONcOptionFlag_InvertMouseOn;
	}
	else {
		ONgPersistance.option_flags &= ~ONcOptionFlag_InvertMouseOn;
	}

	ONrPersist();

	return;
}

void ONrPersist_SetSubtitlesOn(UUtBool inOn)
{
	if (inOn) {
		ONgPersistance.option_flags |= ONcOptionFlag_SubtitlesOn;
	}
	else {
		ONgPersistance.option_flags &= ~ONcOptionFlag_SubtitlesOn;
	}

	ONrPersist();

	return;
}


ONtDifficultyLevel ONrPersist_GetDifficulty(void)
{
	ONtDifficultyLevel difficulty_level = UUmPin(ONgPersistance.difficulty, ONcDifficultyLevel_Min, ONcDifficultyLevel_Max);

	return difficulty_level;
}

void ONrPersist_SetDifficulty(ONtDifficultyLevel inDifficulty)
{
	ONtDifficultyLevel difficulty_level = UUmPin(inDifficulty, ONcDifficultyLevel_Min, ONcDifficultyLevel_Max);

	ONgPersistance.difficulty = difficulty_level;
	ONrPersist();

	return;
}

void ONrPersist_SetResolution(M3tDisplayMode *inResolution)
{
	ONgPersistance.resolution = *inResolution;
	ONrPersist();
}

M3tDisplayMode ONrPersist_GetResolution(void)
{
	M3tDisplayMode result_resolution = ONgPersistance.resolution;

	return result_resolution;
}


const ONtContinue *ONrPersist_GetContinue(UUtInt32 inLevel, UUtInt32 inSavePoint)
{
	ONtContinue *result = NULL;

#if SHIPPING_VERSION != 0
	UUmAssert((inLevel >= 0) && (inLevel < ONcPersist_NumLevels));
#endif
	UUmAssert((inSavePoint >= 0) && (inSavePoint < ONcPersist_NumContinues));

	if ((inLevel < 0) || (inLevel >= ONcPersist_NumLevels)) {
		goto exit;
	}

	if ((inSavePoint < 0) || (inSavePoint >= ONcPersist_NumContinues)) {
		goto exit;
	}

	if (ONgPersistance.continues[inLevel][inSavePoint].continue_flags & ONcContinueFlag_Valid) {
		result = &(ONgPersistance.continues[inLevel][inSavePoint]);
	}

exit:
	return result;
}

void ONrPersist_SetContinue(UUtInt32 inLevel, UUtInt32 inSavePoint, const ONtContinue *inContinue)
{
	COrConsole_Printf("ONrPersist_SetContinue %d %d", inLevel, inSavePoint);

	UUmAssert((inLevel >= 0) && (inLevel < ONcPersist_NumLevels));
	UUmAssert((inSavePoint >= 0) && (inSavePoint < ONcPersist_NumContinues));

	if ((inLevel < 0) || (inLevel >= ONcPersist_NumLevels)) {
		goto exit;
	}

	if ((inSavePoint < 0) || (inSavePoint >= ONcPersist_NumContinues)) {
		goto exit;
	}

	ONgPersistance.continues[inLevel][inSavePoint] = *inContinue;

	ONrPersist();

exit:
	return;
}

void ONrPersist_SetGamma(float inGamma)
{
	if (inGamma < 0) {
		goto exit;
	}

	if (inGamma > 1.f) {
		goto exit;
	}

	ONgPersistance.gamma = inGamma;
	M3rSetGamma(inGamma);

	ONrPersist();

exit:
	return;
}

float ONrPersist_GetGamma(void)
{
	float gamma = ONgPersistance.gamma;

	gamma = UUmPin(gamma, 0.f, 1.f);

	return gamma;
}

const ONtPlace *ONrPersist_GetPlace(void)
{
	return &ONgPersistance.place;
}

void ONrPersist_SetPlace(const ONtPlace *inPlace)
{
	ONgPersistance.place = *inPlace;

	ONrPersist();

	return;
}

UUtBool ONrPersist_WeaponUnlocked(WPtWeaponClass *inWeaponClass)
{
	UUtUns32 weapon_id = inWeaponClass->ai_parameters.shootskill_index;

	return ((ONgPersistance.weapon_bit_field & (1 << weapon_id)) > 0);
}

void ONrPersist_UnlockWeapon(WPtWeaponClass *inWeaponClass)
{
	UUtUns32 weapon_id = inWeaponClass->ai_parameters.shootskill_index;

	if ((ONgPersistance.weapon_bit_field & (1 << weapon_id)) == 0) {
		ONgPersistance.weapon_bit_field |= (1 << weapon_id);
		ONrPersist();
	}
}

UUtBool ONrPersist_ItemUnlocked(WPtPowerupType inPowerupType)
{
	return ((ONgPersistance.item_bit_field & (1 << inPowerupType)) > 0);
}

void ONrPersist_UnlockItem(WPtPowerupType inPowerupType)
{
	if ((ONgPersistance.item_bit_field & (1 << inPowerupType)) == 0) {
		ONgPersistance.item_bit_field |= (1 << inPowerupType);
		ONrPersist();
	}
}

void ONrPersist_GetMaxDiaryPagesRead(UUtUns32 *outLevelNumber, UUtUns32 *outPageNumber)
{
	*outLevelNumber = ONgPersistance.max_diary_level;
	*outPageNumber = ONgPersistance.max_diary_page;
}

void ONrPersist_MarkDiaryPageRead(UUtUns32 inLevelNumber, UUtUns32 inPageNumber)
{
	if (inLevelNumber < ONgPersistance.max_diary_level)
		return;

	if ((inLevelNumber == ONgPersistance.max_diary_level) && (inPageNumber <= ONgPersistance.max_diary_page))
		return;

	ONgPersistance.max_diary_level = inLevelNumber;
	ONgPersistance.max_diary_page = inPageNumber;
	ONrPersist();
}

void ONrPersist_MarkWonGame(void)
{
	ONgPersistance.option_flags |= ONcOptionFlag_WonGame;
	ONrPersist();
}

UUtBool ONrPersist_GetWonGame(void)
{
	UUtBool has_won_game = (ONgPersistance.option_flags & ONcOptionFlag_WonGame) > 0;

	return has_won_game;
}
