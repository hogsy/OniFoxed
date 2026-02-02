// ======================================================================
// BFW_SS2_MSADPCM.h
// ======================================================================
#pragma once

#ifndef BFW_SS2_MSADPCM_H
#define BFW_SS2_MSADPCM_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_SoundSystem2.h"
#include "BFW_SS2_Private.h"
#include "BFW_SS2_MSADPCM.h"

// ======================================================================
// prototypes
// ======================================================================
void
SSrMSADPCM_CompressSoundData(
	SStSoundData				*inSoundData);

UUtUns32
SSrMSADPCM_CalculateNumSamples(
	SStSoundData				*inSoundData);

// ======================================================================
#endif /* BFW_SS2_MSADPCM_H */
