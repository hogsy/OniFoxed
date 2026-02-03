// ======================================================================
// Imp_Sound2.h
// ======================================================================
#pragma once

#ifndef IMP_SOUND2_H
#define IMP_SOUND2_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

// ======================================================================
// prototypes
// ======================================================================
UUtError
IMPrImport_AddSound2(
	BFtFileRef*				inSourceFileRef,
	UUtUns32				inSourceFileModDate,
	GRtGroup*				inGroup,
	char*					inInstanceName);

UUtError
IMPrImport_AddSoundBin(
	BFtFileRef*				inSourceFileRef,
	UUtUns32				inSourceFileModDate,
	GRtGroup*				inGroup,
	char*					inInstanceName);

UUtError
IMPrImport_AddSubtitles(
	BFtFileRef*				inSourceFileRef,
	UUtUns32				inSourceFileModDate,
	GRtGroup*				inGroup,
	char*					inInstanceName);


// ======================================================================
#endif /* IMP_SOUND2_H */
