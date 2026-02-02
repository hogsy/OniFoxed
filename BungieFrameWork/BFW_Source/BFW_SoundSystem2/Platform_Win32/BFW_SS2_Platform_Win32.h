// ======================================================================
// BFW_SS2_Platform_Win32.h
// ======================================================================
#pragma once

#ifndef BFW_SS2_PLATFORM_WIN32_H
#define BFW_SS2_PLATFORM_WIN32_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

#define DIRECTSOUND_VERSION  0x0600
#include <dsound.h>
#include <mmreg.h>
#include <msacm.h>

// ======================================================================
// defines
// ======================================================================
#define	SScNotifiesPerChannel		3

// ======================================================================
// typedefs
// ======================================================================
typedef CRITICAL_SECTION		SStGuard;

typedef struct SStSoundChannel_PlatformData
{
	LPDIRECTSOUNDBUFFER			soundBuffer;
	LPDIRECTSOUND3DBUFFER		soundBuffer3D;

	UUtUns32					buffer_size;
	UUtUns32					buffer_pos[SScNotifiesPerChannel];

	UUtUns32					stop_pos;
	UUtUns32					stop_section;

	UUtUns32					section_size;
	UUtUns32					section;
	UUtUns32					bytes_read;
	UUtUns32					bytes_written;
	UUtBool						stop;
	UUtBool						can_stop;

	UUtUns8						*decompressed_data;
	UUtUns32					decompressed_data_length;
	UUtUns32					num_packets_decompressed;
	UUtUns32					decompressed_packets_length;

	WAVEFORMATEX				DstHeader;
	HACMSTREAM					acm;
	ACMSTREAMHEADER				stream;
	UUtUns32					DefaultReadSize;

} SStSoundChannel_PlatformData;

// ======================================================================
// prototypes
// ======================================================================
LPDIRECTSOUND
SS2rPlatform_GetDirectSound(
	void);

// ======================================================================
#endif /* BFW_SS2_PLATFORM_WIN32_H */
