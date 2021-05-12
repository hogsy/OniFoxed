// ======================================================================
// BFW_SS2_IMA.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_SoundSystem2.h"
#include "BFW_SS2_IMA.h"

// ======================================================================
// defines
// ======================================================================

// ======================================================================
// typedefs
// ======================================================================
typedef UUtInt32	PCM_32;		/* This typedef is platform dependent!!! */

typedef struct SStIMA_DecompressorState
{
	UUtUns16		*destination;
	const UUtUns8	*source_samples;
	UUtInt16		predsample;
	UUtInt8			index;
	
} SStIMA_DecompressorState;

typedef struct SStIMA_Intermediate
{
	long		magic_cookie;
	UUtBool		initialized;
	short		compression_type;
	short		uncompressed_sample_size_in_bits;
	long		frequency;
	short		number_of_channels;
	long		sample_frame_count;
	void		*samples;
	
} SStIMA_Intermediate;

// ======================================================================
// globals
// ======================================================================
const UUtInt8 SSgIndextab[]= {
	-1,-1,-1,-1, 2, 4, 6, 8,
	-1,-1,-1,-1, 2, 4, 6, 8};
	
const UUtInt16 SSgSteptab[89]= { 	
	7, 8, 9, 10, 11, 12, 13, 14,
	16,  17,  19,  21,  23,  25,  28,
	31,  34,  37,  41,  45,  50,  55,
	60,  66,  73,  80,  88,  97, 107,
	118, 130, 143, 157, 173, 190, 209,
	230, 253, 279, 307, 337, 371, 408,
	449, 494, 544, 598, 658, 724, 796,
	876, 963,1060,1166,1282,1411,1552,
	1707,1878,
	2066,2272,2499,2749,3024,3327,3660,4026,
	4428,4871,5358,5894,6484,7132,7845,8630,
	9493,10442,11487,12635,13899,15289,16818,
	18500,20350,22385,24623,27086,29794,32767
};

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
SSiIMA_CompressPacket(
	short						*source,
	unsigned int				number_of_samples,
	UUtUns8						*destination, 
	short						*compression_index, 
	short						*predicted)
{
	PCM_32						predsample;
	UUtInt16					index;
	int							step;
	int							codebuf;
	UUtUns32					sample;
	
	predsample = *predicted;
	index = *compression_index;
	step = SSgSteptab[index];
	codebuf = 0;
	
	for (sample = 0; sample < number_of_samples; sample++)
	{
		int						i;
		int						mask;
		int						code;
		int						tempstep;
		PCM_32					diff;
		
		diff = (PCM_32)source[sample] - predsample; /* difference may require 17 bits! */
		
		if (diff >= 0)  			/* set sign bit */
		{
			code = 0;
		}
		else
		{
			code = 8;
			diff = -diff;
		}
		
		mask = 4;
		tempstep = step;
		for(i = 0; i < 3; i++)		/* quantize diff sample */
		{
			if(diff >= tempstep)
			{
				code |= mask;
				diff -= tempstep;
			}
			tempstep >>= 1;
			mask >>= 1;
		}
		
		// cache the writes
		if (sample & 1)	
		{
			destination[sample >> 1] = (code << 4) | codebuf;
		}
		else	
		{
			codebuf = code & 0xf;		/* buffer for write with next nibble */
		}
		
		/* compute new sample estimate predsample */
		diff = step >> 3;
		if (code & 4) { diff += step; }
		if (code & 2) { diff += step >> 1; }
		if (code & 1) { diff += step >> 2; }
		
		if (code & 8) { diff = -diff; }
		predsample= UUmPin(predsample + diff, -32768, 32767);
		
		/* compute new stepsize step */
		index = UUmPin(index + SSgIndextab[code], 0, 88);		
		step = SSgSteptab[index];
	}
	
	*predicted = (short) predsample;
	*compression_index = index;
}

// ----------------------------------------------------------------------
static void
SSiIMA_CompressMono(
	UUtUns32				inNumChannels,
	UUtUns32				inNumBits,
	UUtUns32				inNumFrames,
	UUtUns8					*inBuffer,
	UUtUns8					**outCompressedBuffer,
	UUtUns32				*outCompressedBufferNumFrames)
{
	int						number_of_samples;
	int						packet_count;
	int						padded_samples_per_channel;
	int						padded_length;
	short					*samples;
	int						silence;
	char					*new_samples;
	
	UUmAssert(inNumChannels == 1);
	UUmAssert(inBuffer);
	UUmAssert(outCompressedBuffer);
	UUmAssert(outCompressedBufferNumFrames);
	
	/*
		Simply take the original samples, grab them in chunks of packets,
		and compress them.
	*/
	number_of_samples = inNumFrames;
	packet_count =
		(number_of_samples / SScIMA_SamplesPerPacket) +
		((number_of_samples % SScIMA_SamplesPerPacket) ? 1 : 0);
	padded_samples_per_channel = packet_count * SScIMA_SamplesPerPacket;
	padded_length = sizeof(short) * padded_samples_per_channel;
	samples = (short*)UUrMemory_Block_New(padded_length);
	silence = (inNumBits == 8) ? 0x80 : 0;
	
	UUmAssert(samples);
	
	// fill with silence and copy the original data into the padded buffer.
	UUrMemory_Set8(samples, silence, padded_length);
	UUrMemory_MoveFast(inBuffer, samples, sizeof(short) * number_of_samples);
	
	// compress the padded buffer
	new_samples = (char*) UUrMemory_Block_New(packet_count * sizeof(SStIMA_SampleData));

	if (new_samples) {
		SStIMA_SampleData		*sample_data;
		short					*source;
		short					ima_index;
		short					ima_predicted;
		long					index;
		
		sample_data = (SStIMA_SampleData*)new_samples;
		source = samples;
		ima_index = 0;
		ima_predicted = 0;
		
		for (index = 0; index < packet_count; ++index)
		{
			// These two bytes of "state" contain the step index, a 7 bit value
			// in the low byte, and the 9 most significant bits of the
			// predictor value.
			sample_data->state =
				(short)(ima_predicted & 0xFF80) |
				(ima_index & 0x007F);
			UUmSwapBig_2Byte(&sample_data->state);
				
			SSiIMA_CompressPacket(
				source,
				SScIMA_SamplesPerPacket,
				sample_data->samples,
				&ima_index,
				&ima_predicted);
			
			sample_data++;
			source += SScIMA_SamplesPerPacket;
		}
		
		*outCompressedBuffer = (UUtUns8*)new_samples;
		*outCompressedBufferNumFrames = packet_count;
	}
	
	if (samples)
	{
		UUrMemory_Block_Delete(samples);
		samples = NULL;
	}
}

// ----------------------------------------------------------------------
static void
SSiIMA_CompressStereo(
	UUtUns32				inNumChannels,
	UUtUns32				inNumBits,
	UUtUns32				inNumFrames,
	UUtUns8					*inBuffer,
	UUtUns8					**outCompressedBuffer,
	UUtUns32				*outCompressedBufferNumFrames)
{
	int						number_of_samples_per_channel;
	int						packets_per_channel;
	int						padded_samples_per_channel;
	int						padded_length;
	int						total_packet_count;
	short					*left_samples;
	short					*right_samples;
	int						silence;
	char					*new_samples;
	short					*source;
	short					*left;
	short					*right;
	int						index;
	
	/*
	The original stereo sound samples (from the AIFF chunk) come in the format
	L0 R0 L1 R1 L2 R2 L3 R3 .... Ln Rn
	Unfortunately, the Mac's IMA IMA packet format wants to get stereo data as a
	packet of left,	followed by a packet of right.  This makes sense, but is a total
	pain in the ass for us.  Thus, if there are, say, 3 samples per packet, the Mac
	wants this:	L0 L1 L2 R0 R1 R2 .... Ln Ln+1 Ln+2 Rn Rn+1 Rn+2
	*/
	
	UUmAssert(inNumChannels == 2);
	UUmAssert(inBuffer);
	UUmAssert(outCompressedBuffer);
	UUmAssert(outCompressedBufferNumFrames);
	
	number_of_samples_per_channel = inNumFrames;
	
	// Pad, if necessary, to an even multiple of SScIMA_SamplesPerPacket
	packets_per_channel =
		(number_of_samples_per_channel / SScIMA_SamplesPerPacket) +
		((number_of_samples_per_channel % SScIMA_SamplesPerPacket) ? 1 : 0);
	padded_samples_per_channel = packets_per_channel * SScIMA_SamplesPerPacket;
	padded_length = sizeof(short) * padded_samples_per_channel;
	total_packet_count = inNumChannels * packets_per_channel;
	
	left_samples = (short*)UUrMemory_Block_New(padded_length);
	right_samples = (short*)UUrMemory_Block_New(padded_length);
	silence = (inNumBits == 8) ? 0x80 : 0;
	
	UUmAssert(left_samples);
	UUmAssert(right_samples);
	
	// Initially fill with silence
	UUrMemory_Set8(left_samples, silence, padded_length);
	UUrMemory_Set8(right_samples, silence, padded_length);	
	
	// Split the old sound samples into left and right sample buffers
	source = (short*)inBuffer;
	left = left_samples;
	right = right_samples;
	
	for (index = 0; index < number_of_samples_per_channel; ++index)
	{
		*left++ = *source++;
		*right++ = *source++;
	}
	
	// IMA compress the left and right buffers, alternating left packet, right packet...
	new_samples =
		(char*)UUrMemory_Block_New(
			total_packet_count * sizeof(SStIMA_SampleData));
	
	if(new_samples)
	{
		SStIMA_SampleData			*sample_data;
		short						left_ima_index;
		short						left_ima_predicted;
		
		short						right_ima_index;
		short						right_ima_predicted;
		
		sample_data = (SStIMA_SampleData*)new_samples;
		left = left_samples;
		left_ima_index = 0;
		left_ima_predicted = 0;
		
		right = right_samples;
		right_ima_index = 0;
		right_ima_predicted = 0;
		
		for(index = 0; index < packets_per_channel; ++index)
		{
		 	// These two bytes of "state" contains the step index, a 7 bit value in the
		 	// low byte, and the 9 most significant bits of the predictor value.
			
			sample_data->state =
				(short)(left_ima_predicted & 0xFF80) |
				(left_ima_index & 0x007F);
			UUmSwapBig_2Byte(&sample_data->state);
			
			SSiIMA_CompressPacket(
				left,
				SScIMA_SamplesPerPacket,
				sample_data->samples,
				&left_ima_index,
				&left_ima_predicted);
			
			sample_data++;
			left += SScIMA_SamplesPerPacket;
			
			sample_data->state =
				(short)(right_ima_predicted & 0xFF80) |
				(right_ima_index & 0x007F);
			UUmSwapBig_2Byte(&sample_data->state);
			
			SSiIMA_CompressPacket(
				right,
				SScIMA_SamplesPerPacket,
				sample_data->samples,
				&right_ima_index,
				&right_ima_predicted);
				
			sample_data++;
			right += SScIMA_SamplesPerPacket;
		}
		
		*outCompressedBuffer = (UUtUns8*)new_samples;
		*outCompressedBufferNumFrames = packets_per_channel;
	}
	
	// Release the left and right sample buffers
	if (left_samples) { UUrMemory_Block_Delete(left_samples); }
	if (right_samples) { UUrMemory_Block_Delete(right_samples); }
}

// ----------------------------------------------------------------------
static void
SSrIMA_CompressSoundData_Internal(
	UUtUns32					inNumChannels,
	UUtUns32					inNumBits,
	UUtUns32					inNumFrames,
	UUtUns8						*inBuffer,
	UUtUns8						**outCompressedBuffer,
	UUtUns32					*outCompressedBufferNumFrames)
{
	switch (inNumChannels)
	{
		case 2:
			SSiIMA_CompressStereo(
				inNumChannels,
				inNumBits,
				inNumFrames,
				inBuffer,
				outCompressedBuffer,
				outCompressedBufferNumFrames);
		break;
			
		case 1:
			SSiIMA_CompressMono(
				inNumChannels,
				inNumBits,
				inNumFrames,
				inBuffer,
				outCompressedBuffer,
				outCompressedBufferNumFrames);
		break;
	}
}

void
SSrIMA_CompressSoundData(
	SStSoundData				*inSoundData)
{
	UUtUns32					num_channels = SSrSound_GetNumChannels(inSoundData);
	UUtUns32					num_frames = inSoundData->num_bytes / (num_channels * SScBytesPerSample);
	UUtUns8						*compressed_buffer;
	UUtUns32					compressed_buffer_num_packets;

	if (inSoundData->data == NULL) { return; }

	SSrIMA_CompressSoundData_Internal(num_channels, SScBitsPerSample, num_frames, inSoundData->data, &compressed_buffer, &compressed_buffer_num_packets);

	if (NULL != compressed_buffer) {
		// inSoundData has been compressed
		UUrMemory_Block_Delete(inSoundData->data);
		inSoundData->data = compressed_buffer;
		
		inSoundData->flags |= SScSoundDataFlag_Compressed;
		inSoundData->num_bytes = (compressed_buffer_num_packets * sizeof(SStIMA_SampleData) * num_channels);
	}

	return;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================

#if 1

// ----------------------------------------------------------------------
static void 
SSiIMA_DecompressState(
	SStIMA_DecompressorState	*ioState)
{
	UUtUns32					sample;
	PCM_32						predsample;
	UUtInt16					step;
	UUtUns8						codebuf;
	UUtInt8						index;
	UUtUns16					*destination;
	
	UUmAssert(ioState);
	UUmAssert(ioState->source_samples);
	UUmAssert(ioState->destination);
	
	destination = ioState->destination;
	predsample = ioState->predsample;
	index = ioState->index;
	codebuf = 0; // just a reminder. (read first time through)
	
	// reset the step..
	step = SSgSteptab[index];
	
	for (sample = 0; sample < SScIMA_SamplesPerPacket; sample++)
	{
		PCM_32					diff;
		UUtUns8					code;
		
		// cache every other sample.
		if (sample & 1)										/* two samples per inbuf char */
		{
			code = (codebuf >> 4) & 0xF;
		}
		else
		{
			codebuf = ioState->source_samples[sample>>1];	/* buffer two IMA nibbles */
			code = codebuf & 0xF;
		}
		
		/* compute new sample estimate predsample */
		diff = step >> 3;
		if (code & 4) { diff += step; }
		if (code & 2) {	diff += step >> 1; }
		if (code & 1) {	diff += step >> 2; }
		
		if (code & 8) { diff = -diff; }
		predsample = UUmPin(predsample + diff, -32768, 32767);
		
	    *destination = (UUtUns16)predsample;		/* store estimate to output buffer */
	    destination++;
		
	    /* compute new stepsize step */
		index = UUmPin(index + SSgIndextab[code], 0, 88);
	    step = SSgSteptab[index];
	}
}

// ----------------------------------------------------------------------
static void
SSiIMA_DecompressPacket(
	UUtUns16					inState,
	UUtUns8						*inSamples,
	UUtUns16					*inDestination)
{
	SStIMA_DecompressorState	ima_state;

	UUmAssert(inDestination);
	
	// apple has 64 samples per packet..
	ima_state.destination				= inDestination;
	ima_state.source_samples			= inSamples;
	ima_state.predsample				= (inState & 0xFF80);	// 9 bit predictor
	ima_state.index						= (inState & 0x007F);	// 7 bit index
	
	// override the defaults...
	SSiIMA_DecompressState(&ima_state);
}

// ----------------------------------------------------------------------
static UUtUns32
SSiIMA_DecompressSoundData_Mono(
	SStSoundData				*inSoundData,
	UUtUns8						*ioDecompressedData,
	UUtUns32					inDecompressDataLength, /* in packets */
	UUtUns32					inStartPacket)
{
	UUtUns32					number_of_packets;
	UUtUns32					packet_index;
	UUtUns16					*destination;
	SStIMA_SampleData			*sample_data;
	UUtUns32					num_channels = 1;
	UUtUns32					packet_count = inSoundData->num_bytes / (num_channels * sizeof(SStIMA_SampleData));

	number_of_packets =	UUmMin(inDecompressDataLength, (packet_count - inStartPacket));
	
	destination = (UUtUns16*)ioDecompressedData;
	sample_data = ((SStIMA_SampleData*)inSoundData->data) + inStartPacket;
	
	for (packet_index = 0; packet_index < number_of_packets; packet_index++)
	{
		UUtUns16			state;
		
		state = sample_data->state;
		UUmSwapBig_2Byte(&state);
		
		SSiIMA_DecompressPacket(state, sample_data->samples, destination);
		
		destination += SScIMA_SamplesPerPacket;
		sample_data++;
	}
	
	return number_of_packets;
}

// ----------------------------------------------------------------------
static UUtUns32
SSiIMA_DecompressSoundData_Stereo(
	SStSoundData				*inSoundData,
	UUtUns8						*ioDecompressedData,
	UUtUns32					inDecompressDataLength, /* in packets */
	UUtUns32					inStartPacket)
{
	UUtUns32					number_of_packets;
	UUtUns32					packet_index;
	UUtUns16					*destination;
	SStIMA_SampleData			*sample_data;
	UUtUns32					num_channels = 2;
	UUtUns32					packet_count = inSoundData->num_bytes / (num_channels * sizeof(SStIMA_SampleData));

	number_of_packets =	UUmMin(inDecompressDataLength, (packet_count - inStartPacket));
	
	destination = (UUtUns16*)ioDecompressedData;
	sample_data = ((SStIMA_SampleData*)inSoundData->data) + (inStartPacket * 2);
	
	for (packet_index = 0; packet_index < number_of_packets; packet_index++)
	{
		UUtUns16				state;
		UUtUns16				left_samples[SScIMA_SamplesPerPacket];
		UUtUns16				right_samples[SScIMA_SamplesPerPacket];
		
		state = sample_data->state; UUmSwapBig_2Byte(&state);
		SSiIMA_DecompressPacket(state, sample_data->samples, left_samples);
		sample_data++;

		state = sample_data->state; UUmSwapBig_2Byte(&state);
		SSiIMA_DecompressPacket(state, sample_data->samples, right_samples);
		sample_data++;
		
		// Interleave the samples from the left and right channels
		{
			UUtUns16			*left = left_samples;
			UUtUns16			*right = right_samples;
			UUtUns32			index;

			for (index = 0; index < SScIMA_SamplesPerPacket; ++index)
			{
				*destination++ = *left++;
				*destination++ = *right++;
			}
		}
	}
	
	return number_of_packets;
}

// ----------------------------------------------------------------------
UUtUns32
SSrIMA_DecompressSoundData(
	SStSoundData				*inSoundData,
	UUtUns8						*ioDecompressedData,
	UUtUns32					inDecompressDataLength, /* in packets */
	UUtUns32					inStartPacket)
{
	UUtUns32					num_packets_decompressed;
	UUtUns32					num_channels = SSrSound_GetNumChannels(inSoundData);
	
	UUmAssert(inSoundData);
	UUmAssert(ioDecompressedData);
	UUmAssert(inDecompressDataLength != 0);
	
	num_packets_decompressed = 0;
	if (num_channels == 1)
	{
		num_packets_decompressed =
			SSiIMA_DecompressSoundData_Mono(
				inSoundData,
				ioDecompressedData,
				inDecompressDataLength,
				inStartPacket); 
	}
	else if (num_channels == 2)
	{
		num_packets_decompressed =
			SSiIMA_DecompressSoundData_Stereo(
				inSoundData,
				ioDecompressedData,
				inDecompressDataLength,
				inStartPacket);
	}
	
	return num_packets_decompressed;
}

#endif