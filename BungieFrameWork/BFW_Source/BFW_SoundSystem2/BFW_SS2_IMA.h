// ======================================================================
// BFW_SS2_IMA.h
// ======================================================================
#pragma once

#ifndef BFW_SS2_IMA_H
#define BFW_SS2_IMA_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

// ======================================================================
// defines
// ======================================================================
#define SScIMA_SamplesPerPacket			(64)
#define SScIMA_DecompressedSize			(SScIMA_SamplesPerPacket * sizeof(UUtUns16))

// ======================================================================
// typedefs
// ======================================================================
typedef struct SStIMA_SampleData {
	UUtUns16					state;
	UUtUns8						samples[32];

} SStIMA_SampleData;

// ======================================================================
// prototypes
// ======================================================================

UUtUns32
SSrIMA_DecompressSoundData(
	SStSoundData				*inSoundData,
	UUtUns8						*ioDecompressedData,
	UUtUns32					inDecompressDataLength, /* in packets */
	UUtUns32					inStartPacket);

void
SSrIMA_CompressSoundData(
	SStSoundData				*inSoundData);

// ======================================================================
#endif /* BFW_SS2_IMA_H */
