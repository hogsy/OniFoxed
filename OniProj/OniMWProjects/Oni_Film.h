#pragma once

/*
	Oni_Film.h

	Film recording stuff

	Author: Michael Evans, Quinn Dunki
	c1998 Bungie
*/

#ifndef __ONI_FILM_H__
#define __ONI_FILM_H__

#include "BFW.h"
#include "BFW_Totoro.h"
#include "Oni_GameState.h"

extern char ONgBoundF2[];
extern char ONgBoundF3[];

#define ONcFilmFlag_Play	0x0001
#define	ONcFilmFlag_Record	0x0002
#define	ONcFilmFlag_Interp	0x0004
#define ONcFilm_MaxKeys		(60 * 60 * 15)
#define ONcFilm_DefaultInterpFrames		30

enum {
	ONcFilmMode_Normal		= 0,
	ONcFilmMode_FromHere	= 1,
	ONcFilmMode_Interp		= 2
};

typedef tm_struct ONtFilmKey
{
	float			turnUD;
	float			turnLR;

	UUtUns64		current;
	// type must = LItButtonBits
	UUtUns32		time;
	tm_pad			pad[4];
} ONtFilmKey;

#define ONcTemplate_Film UUm4CharToUns32('F', 'I', 'L', 'M')
typedef tm_template('F', 'I', 'L', 'M', "Film")
ONtFilm
{
	// implied 8 bytes here

	M3tPoint3D	initialLocation;
	float		initialFacing;
	float		initialDesiredFacing;
	float		initialAimingLR;
	float		initialAimingUD;

	UUtUns32		length;
	TRtAnimation	*special_anim[2];

	tm_pad			pad[12];

	tm_varindex UUtUns32	numKeys;
	tm_vararray ONtFilmKey	keys[1];
} ONtFilm;

typedef struct ONtFilm_DiskFormat
{
	// implied 8 bytes here

	M3tPoint3D	initialLocation;
	float		initialFacing;
	float		initialDesiredFacing;
	float		initialAimingLR;
	float		initialAimingUD;

	UUtUns32		length;
	TRtAnimation	*special_anim[2];
	UUtUns32	pad3;
	UUtUns32	pad4;
	UUtUns32	pad5;
	UUtUns32	pad6;
	UUtUns32	pad7;

	tm_varindex UUtUns32	numKeys;
} ONtFilm_DiskFormat;

typedef struct ONtFilmState
{
	UUtUns32 flags;
	UUtUns32 curFrame;

	TRtAnimation *special1;
	TRtAnimation *special2;

	UUtUns32 interp_frames;
	M3tVector3D interp_delta;

	ONtFilm *film;
} ONtFilmState;

UUtError ONrFilm_Initialize(void);
void ONrFilm_Terminate(void);

void ONrFilm_GetFrame(const ONtFilmState *inFilm, ONtInputState *outInputState);
void ONrFilm_Create(ONtFilmState *ioFilm);
void ONrFilm_AppendFrame(ONtFilmState *ioFilm, ONtInputState *ioInputState);
UUtBool ONrFilm_Start(ONtCharacter *inCharacter, ONtFilm *inFilm, UUtUns32 inMode, UUtUns32 inInterpFrames);
UUtBool ONrFilm_Stop(ONtCharacter *ioCharacter);

typedef enum
{
	ONcFilmSwapSrc_Memory,
	ONcFilmSwapSrc_Disk
} ONtFilm_SwapSrc;
void ONrFilm_Swap(ONtFilm *ioFilm, ONtFilm_SwapSrc inSwapSrc);
void ONrFilm_Swap_DiskFormat(ONtFilm_DiskFormat *ioFilm, ONtFilmKey *inKeys);

void ONrFilm_WriteToDisk(ONtFilm *inFilm);
void ONrFilm_DisplayStats(ONtFilm *inFilm);

#endif
