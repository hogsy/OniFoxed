// ======================================================================
// BFW_SS2_MSADPCM.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_SoundSystem2.h"
#include "BFW_SS2_Private.h"
#include "BFW_SS2_MSADPCM.h"

#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Win32)

WAVEFORMATEX wave_format_ex_mono =
{
	WAVE_FORMAT_PCM,
	1,
	SScSamplesPerSecond,
	SScSamplesPerSecond * SScBytesPerSample * 1,
	SScBytesPerSample * 1,
	SScBitsPerSample,
	0
};

WAVEFORMATEX wave_format_ex_stereo =
{
	WAVE_FORMAT_PCM,
	2,
	SScSamplesPerSecond,
	SScSamplesPerSecond * SScBytesPerSample * 2,
	SScBytesPerSample * 2,
	SScBitsPerSample,
	0
};

// ======================================================================
// functions
// ======================================================================

// ----------------------------------------------------------------------
void
SSrMSADPCM_CompressSoundData(
	SStSoundData				*inSoundData)
{
	MMRESULT					result;
	HACMSTREAM					acm;
	ACMSTREAMHEADER				stream;
	UUtUns8						*buffer;
	UUtUns8						*delete_me;
	UUtUns32					buffer_size;
	WAVEFORMATEX				*src_wave_format_ex = SSrSound_IsStereo(inSoundData) ? &wave_format_ex_stereo : &wave_format_ex_mono;
	WAVEFORMATEX				*dst_wave_format_ex = (WAVEFORMATEX *) (SSrSound_IsStereo(inSoundData) ? &SSgWaveFormat_Stereo : &SSgWaveFormat_Mono);

	buffer = NULL;
	acm = NULL;
	delete_me = NULL;

	if (inSoundData->data == NULL) { return; }

	// get the format for the PCM header
	result =
		acmFormatSuggest(
			NULL,
			src_wave_format_ex,
			dst_wave_format_ex,
			sizeof(SStFormat),
			ACM_FORMATSUGGESTF_WFORMATTAG);
	if (result != 0) { goto cleanup; }

	// open the stream
	result =
		acmStreamOpen(
			&acm,
			NULL,
			src_wave_format_ex,
			dst_wave_format_ex,
			NULL,
			0,
			0,
			ACM_STREAMOPENF_NONREALTIME);
	if (result != 0) { goto cleanup; }

	// calculate the buffer size
	result =
		acmStreamSize(
			acm,
			inSoundData->num_bytes,
			&buffer_size,
			ACM_STREAMSIZEF_SOURCE);
	if (result != 0) { goto cleanup; }

	// allocate memory for the decompression buffer
	buffer = (UUtUns8*)UUrMemory_Block_NewClear(buffer_size);
	if (buffer == NULL) { goto cleanup; }

	// prepare the stream header
	UUrMemory_Clear(&stream, sizeof(ACMSTREAMHEADER));
	stream.cbStruct = sizeof(ACMSTREAMHEADER);
	stream.pbSrc = inSoundData->data;
	stream.cbSrcLength = inSoundData->num_bytes;
	stream.pbDst = buffer;
	stream.cbDstLength = buffer_size;

	result = acmStreamPrepareHeader(acm, &stream, 0);
	if (result != 0) { goto cleanup; }

	// decompress the sound data into the buffer
	result = acmStreamConvert(acm, &stream, 0);
	if (result == 0)
	{
		delete_me = inSoundData->data;

		// inSoundData has been compressed
		inSoundData->flags |= SScSoundDataFlag_Compressed;
		inSoundData->num_bytes = stream.cbDstLengthUsed;
		inSoundData->data = buffer;
	}

	// unprepare the stream header
	result = acmStreamUnprepareHeader(acm, &stream, 0);
	if (result != 0) { goto cleanup; }

cleanup:
	if (delete_me != NULL)
	{
		UUrMemory_Block_Delete(delete_me);
		delete_me = NULL;
	}

	if ((inSoundData->data != buffer) && (buffer != NULL))
	{
		UUrMemory_Block_Delete(buffer);
		buffer = NULL;
	}

	if (acm != NULL)
	{
		result = acmStreamClose(acm, 0);
		acm = NULL;
	}
}

// ----------------------------------------------------------------------
UUtUns32
SSrMSADPCM_CalculateNumSamples(
	SStSoundData				*inSoundData)
{
	UUtUns32					num_samples;
	MMRESULT					result;
	WAVEFORMATEX				DstFormat;
	HACMSTREAM					acm;
	UUtUns32					buffer_size;
	WAVEFORMATEX				*wave_format_ex = (WAVEFORMATEX *) (SSrSound_IsStereo(inSoundData) ? &SSgWaveFormat_Stereo : &SSgWaveFormat_Mono);

	UUmAssert(inSoundData);
	if ((inSoundData->flags & SScSoundDataFlag_Compressed) == 0) {
		UUmAssert(0);
		return 0;
	}

	num_samples = 0;
	acm = NULL;

	// get the format for the PCM header
	DstFormat.wFormatTag = WAVE_FORMAT_PCM;
	result =
		acmFormatSuggest(
			NULL,
			wave_format_ex,
			&DstFormat,
			sizeof(WAVEFORMATEX),
			ACM_FORMATSUGGESTF_WFORMATTAG);
	if (result != 0) { goto cleanup; }

	// open the stream
	result =
		acmStreamOpen(
			&acm,
			NULL,
			wave_format_ex,
			&DstFormat,
			NULL,
			0,
			0,
			ACM_STREAMOPENF_NONREALTIME);
	if (result != 0) { goto cleanup; }

	// calculate the buffer size
	result =
		acmStreamSize(
			acm,
			inSoundData->num_bytes,
			&buffer_size,
			ACM_STREAMSIZEF_SOURCE);
	if (result != 0) { goto cleanup; }

	// calculate the approximate number of samples that fit into this buffer size (in bytes)
	num_samples = buffer_size / (DstFormat.wBitsPerSample >> 3);

cleanup:
	if (acm != NULL)
	{
		result = acmStreamClose(acm, 0);
		acm = NULL;
	}

	return num_samples;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
#elif defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Mac)

// ----------------------------------------------------------------------
void
SSrMSADPCM_CompressSoundData(
	SStSoundData				*inSoundData)
{
}

// ----------------------------------------------------------------------
UUtUns32
SSrMSADPCM_CalculateDuration(
	SStSoundData				*inSoundData)
{
	return 0L;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
#else

// ----------------------------------------------------------------------
void
SSrMSADPCM_CompressSoundData(
	SStSoundData				*inSoundData)
{
}


// ----------------------------------------------------------------------
UUtUns32
SSrMSADPCM_CalculateDuration(
	SStSoundData				*inSoundData)
{
}

#endif
