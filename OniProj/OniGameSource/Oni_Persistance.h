#pragma once
/*
	FILE:	Oni_Persistance.h

	AUTHOR:	Michael Evans

	CREATED: June 19, 2000

	PURPOSE: Persistant data storage

	Copyright (c) Microsoft

*/
#ifndef ONI_PERSISTANCE_H
#define ONI_PERSISTANCE_H

#include "Oni_GameState.h"
#include "Oni_Motoko.h"
#include "Oni_Weapon.h"

typedef struct ONtPlace
{
	UUtInt32 level;
	UUtInt32 save_point;
} ONtPlace;

typedef struct ONtWeaponSave
{
	char weapon[128];
	UUtUns32 weapon_ammo;
} ONtWeaponSave;

typedef enum ONtContinueFlags {
	ONcContinueFlag_Valid			= 0x00000001,		// should this continue display in the saved game list
	ONcContinueFlag_Ignore_Restore	= 0x00000002		// should this continue ignore a restore_game command ?
} ONtContinueFlags;

struct ONtContinue
{
	char name[64];

	// is this continue valid
	ONtContinueFlags continue_flags;

	// character info
	UUtUns32 hitPoints;
	UUtUns32 maxHitPoints;			// CB: we must store this too because max hit points changes by difficulty level
									// e.g. a game might be saved on easy and restored on medium
	M3tPoint3D actual_position;
	float facing;

	// inventory
	UUtUns32 ammo;
	UUtUns32 cell;
	UUtUns32 shieldRemaining;
	UUtUns32 invisibilityRemaining;
	UUtUns32 hypo;
	UUtUns32 keys;
	UUtUns32 has_lsi;

	ONtWeaponSave weapon_save[WPcMaxSlots];
};

void ONrUnlockLevel(UUtUns16 inLevel);
UUtBool ONrLevelIsUnlocked(UUtUns16 inLevel);

void ONrWeKilledGriffen(UUtBool inMurder);
UUtBool ONrDidWeKillGriffen(void);

void ONrPersistance_Initialize(void);

ONtGraphicsQuality ONrPersist_GetGraphicsQuality(void);
void ONrPersist_SetGraphicsQuality(ONtGraphicsQuality inQuality);

ONtDifficultyLevel ONrPersist_GetDifficulty(void);
void ONrPersist_SetDifficulty(ONtDifficultyLevel inDifficulty);

void ONrPersist_SetGamma(float inGamma);
float ONrPersist_GetGamma(void);

float ONrPersist_GetOverallVolume(void);
void ONrPersist_SetOverallVolume(float inVolume);

UUtBool ONrPersist_IsDialogOn(void);
UUtBool ONrPersist_IsInvertMouseOn(void);
UUtBool ONrPersist_IsMusicOn(void);
UUtBool ONrPersist_AreSubtitlesOn(void);

void ONrPersist_SetDialogOn(UUtBool inOn);
void ONrPersist_SetInvertMouseOn(UUtBool inOn);
void ONrPersist_SetMusicOn(UUtBool inOn);
void ONrPersist_SetSubtitlesOn(UUtBool inOn);
void ONrPersist_SetResolution(M3tDisplayMode *inResolution);
M3tDisplayMode ONrPersist_GetResolution(void);


// if you change these you will need to bum ONcPersitance_Version
#define ONcPersist_NumLevels 40
#define ONcPersist_NumContinues 10

const ONtContinue *ONrPersist_GetContinue(UUtInt32 inLevel, UUtInt32 inSavePoint);
void ONrPersist_SetContinue(UUtInt32 inLevel, UUtInt32 inSavePoint, const ONtContinue *inContinue);

const ONtPlace *ONrPersist_GetPlace(void);
void ONrPersist_SetPlace(const ONtPlace *inPlace);

UUtBool ONrPersist_WeaponUnlocked(WPtWeaponClass *inWeaponClass);
void ONrPersist_UnlockWeapon(WPtWeaponClass *inWeaponClass);

UUtBool ONrPersist_ItemUnlocked(WPtPowerupType inPowerupType);
void ONrPersist_UnlockItem(WPtPowerupType inPowerupType);

void ONrPersist_MarkDiaryPageRead(UUtUns32 inLevelNumber, UUtUns32 inPageNumber);
void ONrPersist_GetMaxDiaryPagesRead(UUtUns32 *outLevelNumber, UUtUns32 *outPageNumber);

void ONrPersist_MarkWonGame(void);
UUtBool ONrPersist_GetWonGame(void);

#endif /* ONI_PERSISTANCE_H */
