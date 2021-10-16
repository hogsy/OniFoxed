// ======================================================================
// BFW_SoundSystem2.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================

#include <ctype.h>

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_SoundSystem2.h"
#include "BFW_SS2_Private.h"
#include "BFW_SS2_Platform.h"
#include "BFW_SS2_IMA.h"
#include "BFW_SS2_MSADPCM.h"
#include "BFW_Timer.h"
#include "BFW_MathLib.h"
#include "BFW_ScriptLang.h"
#include "BFW_WindowManager.h"
#include "BFW_FileManager.h"

#include "BFW_Console.h"
#include "BFW_Particle3.h"

#if IMPORTER
#include "Imp_Common.h"
#endif

// ======================================================================
// defines
// ======================================================================
#define SScSoundDataCacheFile_Version_Internal 10

#define SScDistanceThreshold		300.0f

#define SScVolumeAdjust				(0.05f)
#define SScVolumeAdjustEpsilon		(0.001f)

#define SScSoundDirectory			"Sound2"

#define SScPercentStereo			(0.40f)
#define SScNoPanDistanceSquared		(49.0f)
#define SScImpulseThresholdDistance	(3600.0f)


#define UUmGet4BytesFromBuffer(src,dst,type,swap_it)				\
			(dst) = *((type*)(src));								\
			*(UUtInt8**)&(src) += sizeof(type);					\
			if ((swap_it)) { UUrSwap_4Byte(&(dst)); }							

#define UUmWrite4BytesToBuffer(buf,val,type,num_bytes,write_big)	\
			*((type*)(buf)) = (val);								\
			if (write_big) { UUmSwapBig_4Byte(buf); } else { UUmSwapLittle_4Byte(buf); } \
			*(UUtInt8**)&(buf) += sizeof(type);						\
			(num_bytes) -= sizeof(type);

#define UUmGet2BytesFromBuffer(src,dst,type,swap_it)				\
			(dst) = *((type*)(src));								\
			*(UUtInt8**)&(src) += sizeof(type);						\
			if ((swap_it)) { UUrSwap_2Byte(&(dst)); }							

#define UUmWrite2BytesToBuffer(buf,val,type,num_bytes,write_big)	\
			*((type*)(buf)) = (val);								\
			if (write_big) { UUmSwapBig_2Byte(buf); } else { UUmSwapLittle_2Byte(buf); } \
			*(UUtInt8**)&(buf) += sizeof(type);						\
			(num_bytes) -= sizeof(type);

#define UUmGet4CharsFromBuffer(buf,c0,c1,c2,c3)					\
			(c0) = *((char*)(buf[0]));								\
			(c1) = *((char*)(buf[1]));								\
			(c2) = *((char*)(buf[2]));								\
			(c3) = *((char*)(buf[3]));								\
			*(UUtUns8**)&(buf) += (sizeof(UUtUns8) * 4);

#define UUmWrite4CharsToBuffer(buf,c0,c1,c2,c3,num_bytes)			\
			*((UUtUns8*)(buf)) = (c0);								\
			*((UUtUns8*)(buf + 1)) = (c1);							\
			*((UUtUns8*)(buf + 2)) = (c2);							\
			*((UUtUns8*)(buf + 3)) = (c3);							\
			*(UUtUns8**)&(buf) += (sizeof(UUtUns8) * 4);			\
			(num_bytes) -= (sizeof(UUtUns8) * 4);

// ======================================================================
// enums
// ======================================================================
// must match the constants in <MMReg.h>
enum
{
	cWAVE_FORMAT_PCM							= 0x0001,
	cWAVE_FORMAT_ADPCM							= 0x0002,
	cWAVE_FORMAT_IMA_ADPCM						= 0x0011
};

enum
{
	SScPAStage_None,
	
	SScPAStage_Done,
	SScPAStage_Start,
	SScPAStage_InSoundStart,
	SScPAStage_InSoundPlaying,
	SScPAStage_BodyStart,
	SScPAStage_BodyPlaying,
	SScPAStage_BodyStopping,
	SScPAStage_OutSoundStart,
	SScPAStage_OutSoundPlaying,
	SScPAStage_NonAudible,
	SScPAStage_DetailOnly
};

// ======================================================================
// typedefs
// ======================================================================
typedef struct SStContainerChunk
{
	UUtUns32	ckID;				/* chunk ID (4 character code) */
	UUtInt32	ckSize;				/* chunk size */
	UUtUns32	formType;			/* form type (4 character code) */

} SStContainerChunk;

typedef struct SStExtCommonChunk
{
	UUtUns32	ckID;				/* chunk ID (4 character code) */
	UUtInt32	ckSize;				/* chunk size */
	UUtInt16	numChannels;		/* 1 - mono, 2 - stereo */
	UUtUns32	numSampleFrames;	/* number of sample frames */
	UUtInt16	sampleSize;			/* number of bits per sample */
	UUtUns32	sampleRate;			/* number of frames per second */
	UUtUns32	compressionType;	/* compression type (4 character code) (undefined for non compressed AIFF files) */
	
} SStExtCommonChunk;

typedef struct SStSoundDataChunk
{
	UUtUns32	ckID;				/* chunk ID (4 character code) */
	UUtInt32	ckSize;				/* chunk size */
	UUtInt32	offset;				/* offset to sound data */
	UUtInt32	blockSize;			/* size of alignment blocks */
	
} SStSoundDataChunk;

// ----------------------------------------------------------------------
typedef struct SStPlayingAmbient
{
	SStPlayID					id;
	UUtUns32					stage;
	const SStAmbient			*ambient;
	SStSoundChannel				*channel1;
	SStSoundChannel				*channel2;
	UUtUns32					detail_time;
	
	UUtBool						has_position;
	M3tPoint3D					position;
	M3tVector3D					direction;
	M3tVector3D					velocity;
	float						max_volume_distance;
	float						min_volume_distance;
	float						channel_volume;
	
	UUtUns32					volume_adjust_start_time;
	float						volume_adjust_delta;
	float						volume_adjust_final_volume;
	
} SStPlayingAmbient;

// ======================================================================
// globals
// ======================================================================

#if UUmPlatform == UUmPlatform_Win32

WAVEFORMAT_EX_ADPCM				SSgWaveFormat_Mono = 
{ 
	WAVE_FORMAT_ADPCM,
	1,
	22050,
	11155,
	512,
	4,
	32,
	1012,
	7,

	(UUtInt16) 0x0100,
	(UUtInt16) 0x0000,

	(UUtInt16) 0x0200,
	(UUtInt16) 0xff00,

	(UUtInt16) 0x0000,
	(UUtInt16) 0x0000,

	(UUtInt16) 0x00C0,
	(UUtInt16) 0x0040,

	(UUtInt16) 0x00F0,
	(UUtInt16) 0x0000,

	(UUtInt16) 0x01CC,
	(UUtInt16) 0xFF30,

	(UUtInt16) 0x0188,
	(UUtInt16) 0xFF18
};

WAVEFORMAT_EX_ADPCM				SSgWaveFormat_Stereo = 
{ 
	WAVE_FORMAT_ADPCM,
	2,
	22050,
	22311,
	1024,
	4,
	32,
	1012,
	7,

	(UUtInt16) 0x0100,
	(UUtInt16) 0x0000,

	(UUtInt16) 0x0200,
	(UUtInt16) 0xff00,

	(UUtInt16) 0x0000,
	(UUtInt16) 0x0000,

	(UUtInt16) 0x00C0,
	(UUtInt16) 0x0040,

	(UUtInt16) 0x00F0,
	(UUtInt16) 0x0000,

	(UUtInt16) 0x01CC,
	(UUtInt16) 0xFF30,

	(UUtInt16) 0x0188,
	(UUtInt16) 0xFF18
};

#endif

#if UUmPlatform	== UUmPlatform_Mac
SStCompressionMode				SSgCompressionMode = SScCompressionMode_Mac;
#else
SStCompressionMode				SSgCompressionMode = SScCompressionMode_PC;
#endif

static UUtBool					SSgChannelsInitialized = UUcFalse;
static UUtBool					SSgPlaybackInitialized = UUcFalse;
static UUtBool					SSgEnabled = UUcFalse;
static UUtBool					SSgUsable = UUcFalse;
static UUtBool					SSgDetails_CanPlay = UUcTrue;

static UUtMemory_Array			*SSgAmbientSounds;
static UUtMemory_Array			*SSgImpulseSounds;
static UUtMemory_Array			*SSgSoundGroups;
static UUtMemory_Array			*SSgDeallocatedSoundData;

static UUtMemory_Array			*SSgDynamicSoundData;
static SStPlayingAmbient		*SSgPlayingAmbient;
static UUtUns32					SSgNumPlayingAmbients;

UUtUns32						SSgSoundChannels_NumTotal;
UUtUns32						SSgSoundChannels_NumMono;
UUtUns32						SSgSoundChannels_NumStereo;
static SStSoundChannel			*SSgSoundChannels_Stereo;
static SStSoundChannel			*SSgSoundChannels_Mono;

static TMtPrivateData			*SSgTemplate_PrivateData;

static SStPlayID				SSgPlayID;

static M3tPoint3D				SSgListener_Position;
static M3tVector3D				SSgListener_Facing;

static float					SSgOverallVolume;
static float					SSgDesiredVolume;

UUtBool							SSgShowDebugInfo;

UUtBool							SSgSearchOnDisk = UUcFalse;
UUtBool							SSgSearchBinariesOnDisk = UUcFalse;

static SStImpulsePointerHandler SSgImpulseHandler;
static SStAmbientPointerHandler SSgAmbientHandler;

static char						*SSgPriority_Name[] =
{
	"Low",
	"Normal",
	"High",
	"Highest",
	NULL
};

SStGuard						*SSgGuardAll;

// ======================================================================
// prototypes
// ======================================================================
static void
SSiPlayingAmbient_Halt(
	SStPlayingAmbient			*inPlayingAmbient);
	
static void
SS2iVolume_Update(
	void);
	
static void
SSiPlayingAmbient_Stall(
	SStSoundChannel				*inSoundChannel);
	
// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------

static UUtUns32 SSiGetCacheFileVersion(void)
{
	UUtUns32 version;
	
	version = SScSoundDataCacheFile_Version_Internal;
	
	if (SScCompressionMode_Mac == SSgCompressionMode) {
		version |= 0x80000000;
	}

	return version;
}

void
SS2rInstallPointerHandlers(
	SStImpulsePointerHandler	inImpulseHandler,
	SStAmbientPointerHandler	inAmbientHandler)
{
	SSgImpulseHandler = inImpulseHandler;
	SSgAmbientHandler = inAmbientHandler;
}

// ----------------------------------------------------------------------
static float
x80tof(
	UUtUns16					*inData)
{
	UUtInt16					sign;
	UUtInt16					exponent;
	UUtUns32					mantissa;	
	float						outf;
	
	/* 80-bit = sign 1, exp 15, mantissa 64 */
	/* 32-bit = sign 1, exp 8, mantissa 23 */
	UUmSwapBig_2Byte(&inData[0]);
	UUmSwapBig_2Byte(&inData[1]);
	UUmSwapBig_2Byte(&inData[2]);
	
	sign = inData[0] >> 15;
	exponent = (inData[0] & 0x7FFF) - 16382;
	mantissa = (inData[1] << 16) | inData[2];
	
	outf =
		((float)mantissa/(float)UUcMaxUns32) *
		(float)(1 << exponent) *
		(float)pow(-1, sign);

	return outf;
}

// ----------------------------------------------------------------------
UUtBool
UUrFindTag(
	UUtUns8						*inSource,
	UUtUns32					inSourceLength,
	UUtUns32					inTag,
	UUtUns8						**outData)
{
	char						*src;
	UUtBool						found;
	UUtUns32					src_tag;
	
	// find inTag in inSource
	src = (char*)inSource;
	src_tag = UUm4CharToUns32(src[0], src[1], src[2], src[3]);
	found = UUcFalse;
	while (inSourceLength--)
	{
		if (src_tag == inTag)
		{
			*outData = (UUtUns8*)(src);
			found = UUcTrue;
			break;
		}
		
		src++;
		
		src_tag <<= 8;
		src_tag |= src[3];
	}

	return found;
}

// ----------------------------------------------------------------------
UUtError
UUrFindTagData(
	UUtUns8						*inSource,
	UUtUns32					inSourceLength,
	UUtFileEndian				inFileEndian,
	UUtUns32					inTag,
	UUtUns8						**outData,
	UUtUns32					*outLength)
{
	// clear the outgoing data
	*outData = NULL;
	*outLength = 0;
	
	if (UUrFindTag(inSource, inSourceLength, inTag, outData) == UUcTrue)
	{
		UUtUns32				length;
		UUtUns8					*src;
		
		// step over the tag
		src = *outData + 4;
		
		// get the length of the data
		length = *(UUtUns32*)src;
		if (inFileEndian == UUcFile_BigEndian)
		{
			UUmSwapBig_4Byte(&length);
		}
		else
		{
			UUmSwapLittle_4Byte(&length);
		}
		
		if (length > inSourceLength)
		{
			return UUcError_Generic;
		}
		
		// step over the length
		src += 4;
		
		// set the outgoing data
		*outData = (UUtUns8*)src;
		*outLength = length;
		
		return UUcError_None;
	}
	
	return UUcError_Generic;
}

// ----------------------------------------------------------------------
UUtUns32
UUrWriteTagDataToBuffer(
	UUtUns8						*inDestinationBuffer,
	UUtUns32					inDestinationSize,
	UUtUns32					inTag,
	UUtUns8						*inSourceData,
	UUtUns32					inSourceDataLength,
	UUtFileEndian				inWriteEndian)
{
	UUtTag						tag;
	
	UUmAssert(sizeof(UUtTag) == ((sizeof(UUtUns8) * 4) + sizeof(UUtUns32)));
	
	if (inDestinationSize < (inSourceDataLength + sizeof(UUtTag)))
	{
		UUmAssert(!"inDestinationSize is too small");
		return 0;
	}
	
	tag.tag[0] = (UUtUns8)(inTag >> 24);
	tag.tag[1] = (UUtUns8)(inTag >> 16);
	tag.tag[2] = (UUtUns8)(inTag >> 8);
	tag.tag[3] = (UUtUns8)inTag;
	tag.size = inSourceDataLength;
	
	if (inWriteEndian == UUcFile_BigEndian)
	{
		UUmSwapBig_4Byte(&tag.size);
	}
	else
	{
		UUmSwapLittle_4Byte(&tag.size);
	}
	
	// write the tag to the destination buffer
	UUrMemory_MoveFast(&tag, inDestinationBuffer, sizeof(UUtTag));
	inDestinationBuffer += sizeof(UUtTag);
	
	// write teh source data to the destination buffer
	UUrMemory_MoveFast(inSourceData, inDestinationBuffer, inSourceDataLength);
	
	return (inSourceDataLength + sizeof(UUtTag));
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
SSrShowDebugInfo(
	void)
{
	IMtPoint2D					dest;
	char						string[128];
	UUtUns32					i;
	UUtUns32					num_channels;
	UUtInt16					width;
	UUtInt16					height;
	PStPartSpec					*partspec;
	M3tPointScreen				screen_dest;
	UUtError					error;
	TStFontFamily				*font_family;
	UUtUns32					platform_numlines;
	
	if (SSgShowDebugInfo == UUcFalse) { return; }
	
	error = TSrFontFamily_Get(TScFontFamily_Default, &font_family);
	if (error == UUcError_None)
	{
		DCrText_SetFontFamily(font_family);
	}
	DCrText_SetSize(TScFontSize_Default);
	DCrText_SetStyle(TScFontStyle_Default);
	
	num_channels = SSiSoundChannels_GetNumChannels();
	
	SS2rPlatform_GetDebugNeeds(&platform_numlines);
	width = 275;
	height = (10 + (UUtInt16)DCrText_GetLineHeight() * ((UUtInt16)num_channels + (UUtInt16)platform_numlines));
	
	screen_dest.x = (float)((UUtInt16)M3rDraw_GetWidth() - width);
	screen_dest.y = 10.0f;
	screen_dest.z = 0.5f;
	screen_dest.invW = 2.0f;
	
	partspec = PSrPartSpec_LoadByType(PScPartSpecType_BackgroundColor_Blue);
	PSrPartSpec_Draw(partspec, PScPart_All, &screen_dest, (UUtInt16) width, (UUtInt16) height, (M3cMaxAlpha >> 2));
	
	DCrText_SetFormat(TSc_HLeft);
	
	dest.y = (UUtInt16)(screen_dest.y) + DCrText_GetLineHeight();
	
	for (i = 0; i < num_channels; i++)
	{
		SStSoundChannel			*channel;
		IMtShade				shade;
		
		channel = SSiSoundChannels_GetChannelByID(i);
		if (channel == NULL) { break; }
		
		shade = (channel->flags & SScSoundChannelFlag_Mono) ? IMcShade_Green : IMcShade_Red;
		DCrText_SetShade(shade);
		
		dest.x = (UUtInt16)screen_dest.x + 5;
		sprintf(string, "chnl %d", i);
		DCrText_DrawText(string, NULL, &dest);
		dest.x += 35;
		
		sprintf(string, "%1.3f", channel->volume);
		DCrText_DrawText(string, NULL, &dest);	
		dest.x += 25;
		
//		sprintf(string, "%1.3f", channel->distance_to_listener);
//		DCrText_DrawText(string, NULL, &dest);	
//		dest.x += 25;
		
		if (SSiSoundChannel_IsPlaying(channel))
		{
			sprintf(string, "%s", "P");
			DCrText_DrawText(string, NULL, &dest);
		}
		dest.x += 10;
		
		if (SSiSoundChannel_IsLooping(channel))
		{
			sprintf(string, "%s", "l");
			DCrText_DrawText(string, NULL, &dest);
		}
		dest.x += 10;
		
		if (SSiSoundChannel_IsLocked(channel))
		{
			sprintf(string, "%s", "L");
			DCrText_DrawText(string, NULL, &dest);
		}
		dest.x += 10;
		
		if (SSiSoundChannel_CanPan(channel))
		{
			sprintf(string, "%1.1f %1.1f", channel->pan_left, channel->pan_right);
			DCrText_DrawText(string, NULL, &dest);
		}
		else
		{
			DCrText_SetShade(IMcShade_White);
			sprintf(string, "%1.1f %1.1f", channel->pan_left, channel->pan_right);
			DCrText_DrawText(string, NULL, &dest);
		}
		dest.x += 40;
		
		DCrText_SetShade(shade);
		if (SSiSoundChannel_IsPlaying(channel))
		{
			if ((channel->group->group_name) && (channel->sound_data))
			{
				sprintf(
					string,
					"%s, %s",
					channel->group->group_name,
					SSrSoundData_GetName(channel->sound_data));
				DCrText_DrawText(string, NULL, &dest);
			}
		}
		dest.x += 150;
		
		dest.y += DCrText_GetLineHeight();
	}
	
	dest.x = (UUtInt16)screen_dest.x + 5;
	SS2rPlatform_ShowDebugInfo_Overall(&dest);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtBool
SSrNameIsValidDialogName(
	const char					*inName)
{
	UUtBool is_valid_dialog_name = UUcTrue;

	// name must be c##_##_##.... // length of 10+ with [0]=c, [3]=_ [6]=_

	if (NULL == inName) {
		is_valid_dialog_name = UUcFalse;
	}
/*	else if (strlen(inName) < 10) {
		is_valid_dialog_name = UUcFalse;
	}
	else if ((inName[0] != 'c') || (inName[3] != '_') || (inName[6] != '_')) {
		is_valid_dialog_name = UUcFalse;
	}*/

	return is_valid_dialog_name;
}

// ----------------------------------------------------------------------
static const char*
SSiSubtitleArray_FindByName(
	SStSubtitle					*inArray,
	const char					*inName) 
{
	const char *result = NULL;

	// search the subtitle table for an exact match to inName
	if (NULL != inArray)
	{
		UUtInt32 itr;
		UUtInt32 count = inArray->numSubtitles;
		UUtUns32 offset = (UUtUns32) TMrInstance_GetRawOffset(inArray);

		for(itr = 0; itr < count; itr++)
		{
			const char *compare_string = inArray->data + offset + inArray->subtitles[itr];

			if (UUmString_IsEqual_NoCase(inName, compare_string))
			{
				result = compare_string + strlen(compare_string) + 1;
				break;
			}
		}
	}

	return result;
}

// ----------------------------------------------------------------------
static const char*
SSiSubtitleArray_FindByNumber(
	SStSubtitle					*inArray,
	const char					*inName)
{
	// search the subtitle table for a match to the number encoded in inName
	// name of c01_01_01Griffin should match with subtitles named "01_01_01"

	const char *result = NULL;

	if (NULL == inArray)
	{
		result = NULL;
	}
	else
	{
		UUtInt32 itr;
		UUtInt32 count = inArray->numSubtitles;
		UUtUns32 offset = (UUtUns32) TMrInstance_GetRawOffset(inArray);

		for(itr = 0; itr < count; itr++)
		{
			const char *compare_string = inArray->data + offset + inArray->subtitles[itr];
			UUtBool is_equal;
			const char *name;
			UUtUns32 compare_len;
			
			if (isdigit(compare_string[0])) { name = inName + 1; }
			else { name = inName; }
			
			compare_len = strlen(compare_string);
			is_equal = (UUrString_CompareLen_NoCase(compare_string, name, compare_len) == 0);

/*			is_equal = UUcTrue;
			is_equal &= compare_string[0] == inName[0];
			is_equal &= compare_string[1] == inName[1];
			is_equal &= compare_string[3] == inName[3];
			is_equal &= compare_string[4] == inName[4];
			is_equal &= compare_string[6] == inName[6];
			is_equal &= compare_string[7] == inName[7];
			
			if (isalpha(compare_string[8]) == UUcTrue) {
				is_equal &= compare_string[8] == inName[8];
			}*/

			if (is_equal)
			{
				result = compare_string + strlen(compare_string) + 1;
				break;
			}
		}
	}

	return result;
}

// ----------------------------------------------------------------------
const char*
SSrSubtitle_Find(
	const char					*inName) 
{
	return SSiSubtitleArray_FindByNumber(TMrInstance_GetFromName(SScTemplate_Subtitle, "subtitles"), inName);
}

// ----------------------------------------------------------------------
const char*
SSrMessage_Find(
	const char					*inName) 
{
	return SSiSubtitleArray_FindByName(TMrInstance_GetFromName(SScTemplate_Subtitle, "messages"), inName);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static int UUcExternal_Call
SSiSoundData_Compare(
	const void					*inItem1,
	const void					*inItem2)
{
	const SStSoundData			*item1;
	const SStSoundData			*item2;
	const char					*item1_name;
	const char					*item2_name;
	
	item1 = *((SStSoundData**)inItem1);
	item2 = *((SStSoundData**)inItem2);
	
	item1_name = SSrSoundData_GetName(item1);
	item2_name = SSrSoundData_GetName(item2);
	
	return strcmp(item1_name, item2_name);
}

// ----------------------------------------------------------------------
static UUtError
SSiSoundData_AddToArray(
	SStSoundData				*inSoundData)
{
	UUtError					error;
	SStSoundData				**sound_data_array;
	UUtUns32					num_elements;
	UUtUns32					found_location;
	
	// find the point in the array to insert the element
	sound_data_array = (SStSoundData**)UUrMemory_Array_GetMemory(SSgDynamicSoundData);
	num_elements = UUrMemory_Array_GetUsedElems(SSgDynamicSoundData);
#if 1
	{
		// binary search on the array, to find the right place to insert
		UUtInt32 min, max, midpoint, compare;

		min = 0;
		max = num_elements - 1;

		while(min <= max) {
			midpoint = (min + max) >> 1; // S.S. / 2;

			compare = SSiSoundData_Compare(&inSoundData, &sound_data_array[midpoint]);

			if (compare < 0) {
				// must lie below here
				max = midpoint - 1;

			} else {
				// must lie above here
				min = midpoint + 1;
			}
		}

		found_location = (UUtUns32) min;
	}
#else
	for (found_location = 0; found_location < num_elements; found_location++)
	{
		if (SSiSoundData_Compare(&inSoundData, &sound_data_array[found_location]) < 0)
		{
			break;
		}
	}
#endif

	// create space in the array at the desired location
	UUmAssert((found_location >= 0) && (found_location <= num_elements));
	error = UUrMemory_Array_InsertElement(SSgDynamicSoundData, found_location, NULL);
	UUmError_ReturnOnError(error);
	
	// set the array element
	sound_data_array = (SStSoundData**)UUrMemory_Array_GetMemory(SSgDynamicSoundData);
	sound_data_array[found_location] = inSoundData;
	
	return UUcError_None;
}
			
// ----------------------------------------------------------------------
static void SSiSoundData_StoreDeallocatedPointer(SStSoundData *inSoundData)
{
	SStSoundData **array;
	UUtUns32 index;

	if (SSgDeallocatedSoundData != NULL) {
		// store this sound data pointer so that we know to NULL it out in any
		// groups where it appears
		UUrMemory_Array_GetNewElement(SSgDeallocatedSoundData, &index, NULL);
		array = UUrMemory_Array_GetMemory(SSgDeallocatedSoundData);
		array[index] = inSoundData;
	}
}

// ----------------------------------------------------------------------
static void SSiSoundData_Delete(SStSoundData *inSoundData)
{
	UUmAssert(inSoundData);
	
	SSiSoundData_StoreDeallocatedPointer(inSoundData);

	// dispose of the data
	if (inSoundData->data)
	{
		UUrMemory_Block_Delete(inSoundData->data);
		inSoundData->data = NULL;
	}
	
	// dispose of the sound_data
	UUrMemory_Block_Delete(inSoundData);
}

// ----------------------------------------------------------------------
static UUtError
SSiSoundData_ProcHandler(
	TMtTemplateProc_Message		inMessage,
	void						*inInstancePtr,
	void						*inPrivateData)
{
	SStSoundData				*sound_data;
	
	sound_data = (SStSoundData*)inInstancePtr;
	
	// handle the message
	switch (inMessage)
	{
		case TMcTemplateProcMessage_LoadPostProcess:
			sound_data->data = (void*)(((UUtUns32)sound_data->data) + ((UUtUns32)TMrInstance_GetRawOffset(sound_data)));
		break;

		case TMcTemplateProcMessage_DisposePreProcess:
			SSiSoundData_StoreDeallocatedPointer(sound_data);
		break;
	}
	
	return UUcError_None;
}

static UUtUns32 SSgSoundCacheDepth = 0;
static BFtFileCache *SSgSoundCache = NULL;

void SSrSoundData_GetByName_StartCache(void)
{
	if (0 == SSgSoundCacheDepth) {
		BFtFileRef *sound_directory = NULL;
		UUtError error;

		error = SSrGetSoundDirectory(&sound_directory);

		if (UUcError_None == error) {
			SSgSoundCache = BFrFileCache_New(sound_directory, BFcFileCache_Recursive);
			if (SSgSoundCache != NULL) {
				BFrFileCache_PrepareForUse(SSgSoundCache);
			}

			if (sound_directory) {
				UUrMemory_Block_Delete(sound_directory);
			}
		}		
	}

	SSgSoundCacheDepth++;

	return;
}

void SSrSoundData_GetByName_StopCache(void)
{
	UUmAssert(SSgSoundCacheDepth > 0);
	SSgSoundCacheDepth--;

	if (0 == SSgSoundCacheDepth) {
		if (SSgSoundCache != NULL) {
			BFrFileCache_Dispose(SSgSoundCache);
			SSgSoundCache = NULL;
		}
	}

	return;
}

// ----------------------------------------------------------------------
static SStSoundData *SSiSoundData_FindOnDisk(const char *inFileName)
{
	SStSoundData *loaded_sound_data = NULL;

	SSrSoundData_GetByName_StartCache();

	if (NULL != SSgSoundCache) {
		BFtFileRef *file_to_load = BFrFileCache_Search(SSgSoundCache, inFileName, NULL);

		if (NULL != file_to_load) {
			UUtError error;

			error = SSrSoundData_New(file_to_load, &loaded_sound_data);

			if (error != UUcError_None) {
				loaded_sound_data = NULL;
			}

			goto exit;
		}
	}

exit:
	SSrSoundData_GetByName_StopCache();
	return loaded_sound_data;
}

// ----------------------------------------------------------------------
static UUtBool
SS2iSoundData_DeallocatedPointerCompareFunc(
	UUtUns32 inA,
	UUtUns32 inB)
{
	return (inA > inB);
}

// ----------------------------------------------------------------------
void
SS2rSoundData_FlushDeallocatedPointers(
	void)
{
	UUtUns32					num_pointers;
	UUtUns32					num_groups;
	UUtUns32					itr;
	SStSoundData				**pointer_array;
	SStGroup					**group_array;

	// sort the deallocated pointer array
	num_pointers = UUrMemory_Array_GetUsedElems(SSgDeallocatedSoundData);
	if (num_pointers == 0)
		return;

	pointer_array = (SStSoundData **) UUrMemory_Array_GetMemory(SSgDeallocatedSoundData);
	AUrQSort_32(pointer_array, num_pointers, SS2iSoundData_DeallocatedPointerCompareFunc);

	// go through all of the groups and NULL out the sound_data pointers if they've been deleted
	group_array = (SStGroup**)UUrMemory_Array_GetMemory(SSgSoundGroups);
	num_groups = UUrMemory_Array_GetUsedElems(SSgSoundGroups);	
	for (itr = 0; itr < num_groups; itr++)
	{
		SStGroup					*group;
		SStPermutation				*perm_array;
		UUtUns32					num_perms;
		UUtUns32					itr2;
		
		// get a pointer to the sound group
		group = group_array[itr];
		
		// update the sound data pointers for the permutations of the group
		perm_array = (SStPermutation*)UUrMemory_Array_GetMemory(group->permutations);
		num_perms = UUrMemory_Array_GetUsedElems(group->permutations);
		for (itr2 = 0; itr2 < num_perms; itr2++)
		{
			SStSoundData				*candidate = perm_array[itr2].sound_data;
			UUtInt32					min, max, midpoint;

			// this check quickly catches all dynamically allocated sound data and sound data
			// from another templated file to the one currently being deallocated
			if ((candidate < pointer_array[0]) || (candidate > pointer_array[num_pointers - 1]))
				continue;

			// perform a binary search on the deallocated pointer array to see
			// if this sound data has been deleted
			min = 0;
			max = num_pointers - 1;

			while(min <= max) {
				midpoint = (min + max) >> 1; // S.S. / 2;

				if (pointer_array[midpoint] == candidate) {
					// found the candidate pointer in the array, it has in fact
					// been deleted
					perm_array[itr2].sound_data = NULL;
					break;

				} else if (pointer_array[midpoint] > candidate) {
					// must lie below here
					max = midpoint - 1;

				} else {
					// must lie above here
					min = midpoint + 1;
				}
			}
		}
	}

	// flush the deallocated pointer array
	UUrMemory_Array_SetUsedElems(SSgDeallocatedSoundData, 0, NULL);
}

// ----------------------------------------------------------------------
// get the approximate duration of a sound in frames (60ths of a second)...
// this is used for subtitle fading and dialogue cueing.
UUtUns32
SSrSoundData_GetDuration(
	SStSoundData				*inSoundData)
{
	UUtUns32 duration = inSoundData->duration_ticks;

	return duration;
}

// ----------------------------------------------------------------------
SStSoundData*
SSrSoundData_GetByName(
	const char					*inSoundDataName,
	UUtBool						inLookOnDisk)
{
	UUtError					error;
	SStSoundData				*sound_data;
	
	
	UUmAssert(inSoundDataName);
	
	sound_data = NULL;
	
	// look for the a template instance of the sound data
	error =
		TMrInstance_GetDataPtr(
			SScTemplate_SoundData,
			inSoundDataName,
			&sound_data);
	if (error == UUcError_None)
	{
		return sound_data;
	}
	
#if SHIPPING_VERSION == 0
{
	SStSoundData				**sound_data_h;
	SStSoundData				**sound_data_array;
	UUtUns32					num_sound_data;
	SStSoundData				*search_sound_data_ptr;
	SStSoundData				search_sound_data;
	char						search_sound_data_name[SScMaxNameLength];

	// look for the sound in the dynamically allocated sound list
	sound_data_array = (SStSoundData**)UUrMemory_Array_GetMemory(SSgDynamicSoundData);
	num_sound_data = UUrMemory_Array_GetUsedElems(SSgDynamicSoundData);
	
	UUrString_Copy(search_sound_data_name, inSoundDataName, SScMaxNameLength);
	search_sound_data.flags = SScSoundDataFlag_DynamicAlloc; // so SSrSoundData_GetName() looks in the data pointer
	search_sound_data.data = search_sound_data_name;
	search_sound_data.num_bytes = 0;
	search_sound_data_ptr = &search_sound_data;
	
	sound_data_h =
		(SStSoundData**)bsearch(
			&search_sound_data_ptr,
			sound_data_array,
			num_sound_data,
			sizeof(SStSoundData*),
			SSiSoundData_Compare);
	if (sound_data_h != NULL) { sound_data = *sound_data_h; }

	// look for the sound data on disk
	if ((NULL == sound_data) && inLookOnDisk && SSgSearchOnDisk) {
		sound_data = SSiSoundData_FindOnDisk(inSoundDataName);
	}
}

/*#if 0															// CB: this debugging information is no longer useful as 
																// we are now storing all binary sound data in level 0,
																// but many sounds in specific levels -> this is not
																// necessarily a problem, so I'm removing this.
																// -- 12 october, 2000

#if defined(UUmPlatform) && (UUmPlatform != UUmPlatform_Mac)	// debug build pukes on this output -S.S.
	// TEMPORARY DEBUGGING CODE
	if (sound_data == NULL) {
		UUtUns32 itr, num_ptrs;
		void *ptr_list[1024];
		static UUtBool can_dump = UUcTrue;

		UUrDebuggerMessage("SSrSoundData_GetByName: failed to find '%s'\n", inSoundDataName);

		if (can_dump) {
			error = TMrInstance_GetDataPtr_List(SScTemplate_SoundData, 1024, &num_ptrs, ptr_list);
			if (error != UUcError_None) {
				UUrDebuggerMessage("SSrSoundData_GetByName: list dump failed\n");
			} else {
				UUrDebuggerMessage("SSrSoundData_GetByName: dumping list of all sound data instances\n");
				for (itr = 0; itr < num_ptrs; itr++) {
					UUrDebuggerMessage("  %s\n", TMrInstance_GetInstanceName(ptr_list[itr]));
				}
			}
			can_dump = UUcFalse;		// spew only once
		}
	}
#endif
#endif
*/
#endif

	return sound_data;
}

// ----------------------------------------------------------------------
const char*
SSrSoundData_GetName(
	const SStSoundData			*inSoundData)
{
	const char					*name;
	
	UUmAssert(inSoundData);
	
	// if the sound data was dynamically allocated then the sound name is
	// stored at the end of the sound data
	if ((inSoundData->flags & SScSoundDataFlag_DynamicAlloc) != 0)
	{
		name = (char*)inSoundData->data + inSoundData->num_bytes;
	}
	else
	{
		name = TMrInstance_GetInstanceName(inSoundData);
	}
	
	return name;
}

// ----------------------------------------------------------------------
static UUtError
SSrSoundData_WriteCacheFile(
	SStSoundData				*inSoundData,
	UUtUns32					inTime,
	BFtFileRef					*inFileRef)
{
	BFtFile *file;
	UUtUns32 version = SSiGetCacheFileVersion();
	UUtError error;

	error = BFrFile_Open(inFileRef, "w", &file);
	UUmError_ReturnOnError(error);

	BFrFile_Write(file, sizeof(UUtUns32), &version);
	BFrFile_Write(file, sizeof(UUtUns32), &inTime);
	BFrFile_Write(file, sizeof(SStSoundData), inSoundData);
	BFrFile_Write(file, inSoundData->num_bytes, inSoundData->data);

	BFrFile_Close(file);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static SStSoundData *
SSrSoundData_ReadCacheFile(
	BFtFileRef					*inOrigFileRef,
	BFtFileRef					*inCacheFileRef,
	UUtUns32					inModificationTime)
{
	BFtFile						*file;
	UUtUns32					time;
	UUtUns32					version = 0;
	UUtUns32					length;
	SStSoundData				*new_sound_data = NULL;
	UUtError					error;

	error = BFrFile_Open(inCacheFileRef, "r", &file);
	if (error != UUcError_None) {
		goto exit;
	}
	
	BFrFile_Read(file, sizeof(UUtUns32), &version);
	BFrFile_Read(file, sizeof(UUtUns32), &time);

	if ((version != SSiGetCacheFileVersion()) || (time != inModificationTime)) {
		// we can't use this cache file, bail
		BFrFile_Close(file);
		goto exit;
	}
	
	// allocate space for the new sound
	new_sound_data = (SStSoundData*)UUrMemory_Block_New(sizeof(SStSoundData));
	if (new_sound_data == NULL) {
		BFrFile_Close(file);
		goto exit;
	}
	
	// read the SStSoundData into new_sound_data
	BFrFile_Read(file, sizeof(SStSoundData), new_sound_data);
	
	// allocate memory for the data
	length = new_sound_data->num_bytes;
	new_sound_data->data = (UUtUns8*)UUrMemory_Block_NewClear(length);
	if (new_sound_data->data == NULL) {
		// we can't allocate memory, bail
		BFrFile_Close(file);
		UUrMemory_Block_Delete(new_sound_data);
		new_sound_data = NULL;
		goto exit;
	}
	
	// read the file data into the data
	BFrFile_Read(file, length, new_sound_data->data);
	
	// close the file
	BFrFile_Close(file);
	
exit:
	return new_sound_data;
}

// ----------------------------------------------------------------------
static SStSoundData *
SSrSoundData_New_Uncached(
	BFtFileRef					*inFileRef)
{	
	UUtError					error;	
	SStSoundData				temp_sound_data;
	SStSoundData				*new_sound_data = NULL;
	UUtUns8						*uncompressed_data = NULL;
	UUtUns8						*compressed_data = NULL;
	UUtUns8						*file_data = NULL;
	UUtUns8						*raw_data;
	UUtUns32					file_length, raw_data_length;
	const char					*leaf_name;
	
	// load the file
	error =	BFrFileRef_LoadIntoMemory(inFileRef, &file_length, &file_data);
	if (error != UUcError_None) { goto exit; }
	if (file_length == 0) { goto exit; }

	leaf_name = BFrFileRef_GetLeafName(inFileRef);

	// process the file and extract a pointer to the raw sound data
	error =
		SSrSoundData_Process(
			leaf_name,
			file_data,
			file_length,
			&temp_sound_data,
			&raw_data,
			&raw_data_length);
	if (error != UUcError_None) { goto exit; }
	
	// allocate space for the sound data
	new_sound_data = (SStSoundData*)UUrMemory_Block_New(sizeof(SStSoundData));
	if (new_sound_data == NULL) { goto exit; }
	
	// copy temp_sound_data into new_sound_data
	UUrMemory_MoveFast(&temp_sound_data, new_sound_data, sizeof(SStSoundData));
	
	// allocate space for the sound data data
	new_sound_data->data = (UUtUns8*)UUrMemory_Block_NewClear(raw_data_length);
	if (new_sound_data->data == NULL) {
		UUrMemory_Block_Delete(new_sound_data);
		new_sound_data = NULL;
		goto exit;
	}
	
	// copy the raw_data into new_sound_data->data
	UUrMemory_MoveFast(raw_data, new_sound_data->data, raw_data_length);
	
exit:
	if (file_data != NULL) {
		UUrMemory_Block_Delete(file_data);
	}
	
	return new_sound_data;
}
	
// ----------------------------------------------------------------------
UUtError
SSrSoundData_Load(
	BFtFileRef					*inFileRef,
	SStLoadCallback_StoreSoundBlock inStoreSoundCallback,
	SStSoundData				**outSoundData)
{
	UUtError					error, returncode = UUcError_None;
	SStSoundData				*sound_data = NULL;
	BFtFileRef					ssc_file_ref;
	UUtUns32					uncompressed_time_stamp;
		
	UUmAssert(inFileRef);
	UUmAssert(outSoundData);

	ssc_file_ref = *inFileRef;

	BFrFileRef_SetLeafNameSuffex(&ssc_file_ref, "ssc");
	
	// if the ssc file exists load it
	if (BFrFileRef_FileExists(&ssc_file_ref)) {
		// get the modification time
		error = BFrFileRef_GetModTime(inFileRef, &uncompressed_time_stamp);

		if (error != UUcError_None) {
			returncode = error;
			goto cleanup;
		}
		
		// load the ssc file
		sound_data = SSrSoundData_ReadCacheFile(inFileRef, &ssc_file_ref, uncompressed_time_stamp);
	}
	
	// if the ssc file does not exist, then read the sound file
	if (NULL == sound_data) {
		// load the sound file
		sound_data = SSrSoundData_New_Uncached(inFileRef);

		if (NULL == sound_data) {
			returncode = UUcError_Generic;
			goto cleanup;
		}
		
		// compress the sound data
		if (SScCompressionMode_Mac == SSgCompressionMode) {
			SSrIMA_CompressSoundData(sound_data);
		}
		else if (SSgCompressionMode == SScCompressionMode_PC) {
			SSrMSADPCM_CompressSoundData(sound_data);
		}
		else {
			UUmAssert(!"invalid compression mode");
		}
		
		// write a cache file
		error =	SSrSoundData_WriteCacheFile(sound_data,	uncompressed_time_stamp, &ssc_file_ref);

		if (error != UUcError_None) {
			returncode = error;
			goto cleanup;
		}
	}
	
	// store the sound data
	inStoreSoundCallback(sound_data, BFrFileRef_GetLeafName(inFileRef));
	
cleanup:	
	// return the sound data
	*outSoundData = sound_data;

	return returncode;
}

// ----------------------------------------------------------------------
static UUtError
SSiLoadCallback_StoreSoundData(
	SStSoundData				*inSoundData,
	const char					*inSoundName)
{
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
SSrSoundData_New(
	BFtFileRef					*inFileRef,
	SStSoundData				**outSoundData)
{
	UUtError					error;
	
	SStSoundData				*sound_data = NULL;
	
	UUmAssert(inFileRef);
	UUmAssert(outSoundData);
	
	error =
		SSrSoundData_Load(
			inFileRef,
			SSiLoadCallback_StoreSoundData,
			&sound_data);
	UUmError_ReturnOnError(error);

	if (sound_data == NULL) {
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "couldn't load sound data from file, no error flagged");
	}
	
	// allocate memory for the sound data and the sound name
	sound_data->data =
		UUrMemory_Block_Realloc(
			sound_data->data,
			sound_data->num_bytes + SScMaxNameLength);
	if (sound_data->data == NULL) { goto cleanup; }
	
	// this sound_data was dynamically allocated
	sound_data->flags |= SScSoundDataFlag_DynamicAlloc;
	
	// put the file name at the end of the sound data
	{
		char *dest = ((char*)sound_data->data + sound_data->num_bytes);

		UUrString_Copy(dest, BFrFileRef_GetLeafName(inFileRef), SScMaxNameLength);
		UUrString_MakeLowerCase(dest, SScMaxNameLength);
	}
	
	// add sound to SSgDynamicSoundData
	error = SSiSoundData_AddToArray(sound_data);
	if (error != UUcError_None) { goto cleanup; }

	// set the outgoing sound data
	*outSoundData = sound_data;
	
	return UUcError_None;
	
cleanup:
	// delete the sound data
	if (sound_data)
	{
		if (sound_data->data)
		{
			UUrMemory_Block_Delete(sound_data->data);
		}
		UUrMemory_Block_Delete(sound_data);
	}
	
	*outSoundData = NULL;

	return error;
}


static void SSiDownSample(void *data, UUtUns32 num_channels, UUtUns32 count)
{
	UUtUns32 itr;
	UUtUns16 *shorts = (UUtUns16 *) data;

	for(itr = 0; itr < count; itr++)
	{
		UUtUns32 src_sample = itr * 2 * num_channels;
		UUtUns32 dst_sample = itr * 1 * num_channels;

		shorts[dst_sample] = shorts[src_sample];

		if (2 == num_channels) {
			shorts[dst_sample + 1] = shorts[src_sample + 1];
		}
	}

	return;
}

// ----------------------------------------------------------------------
static UUtError
SSiSoundData_ProcessAIFF(
	const char					*inDebugName,
	UUtUns8						*inFileData,
	UUtUns32					inFileDataLength,
	SStSoundData				*outSoundData,
	UUtUns8						**outRawData,
	UUtUns32					*outRawDataLength)
{
	UUtError					error;
	SStContainerChunk			*form;
	UUtUns32					form_size;
	UUtUns8						*common_data;
	SStExtCommonChunk			common;
	UUtUns32					common_size;
	SStSoundDataChunk			*sound;
	UUtUns32					sound_size;
	UUtBool						swap_it;
	
	UUmAssert(inFileData);
	UUmAssertWritePtr(outSoundData, sizeof(SStSoundData));
	UUmAssertWritePtr(outRawData, sizeof(UUtUns8*));
	UUmAssertWritePtr(outRawDataLength, sizeof(UUtUns32));
	
	// set swap it
	#if UUmEndian == UUmEndian_Big
		swap_it = UUcFalse;
	#else
		swap_it = UUcTrue;
	#endif
	
	// clear the outgoing data
	UUrMemory_Clear(outSoundData, sizeof(SStSoundData));
	*outRawData = NULL;
	*outRawDataLength = 0;
	
	// find the FORM data
	error =
		UUrFindTagData(
			inFileData,
			inFileDataLength,
			UUcFile_BigEndian,
			UUm4CharToUns32('F', 'O', 'R', 'M'),
			(UUtUns8**)&form,
			&form_size);
	UUmError_ReturnOnErrorMsg(error, "Unable to find FORM tag");
	
	// back up 8 bytes to get to beginning of SStContainerChunk
	*(UUtInt8**)&form -= (sizeof(UUtUns32) + sizeof(UUtInt32));
	
	// determine if the AIFF is compressed or not
	UUmSwapBig_4Byte(&form->formType);
	if (form->formType == UUm4CharToUns32('A', 'I', 'F', 'C'))
	{
		UUmAssert(!"compressed aiff files are not supported");
		return UUcError_Generic;
	}
	
	// find the COMM data
	error =
		UUrFindTagData(
			inFileData,
			inFileDataLength,
			UUcFile_BigEndian,
			UUm4CharToUns32('C', 'O', 'M', 'M'),
			&common_data,
			&common_size);
	UUmError_ReturnOnErrorMsg(error, "Unable to find COMM tag");
	
	// read the common data
	UUmGet2BytesFromBuffer(common_data, common.numChannels, UUtInt16, swap_it);
	UUmGet4BytesFromBuffer(common_data, common.numSampleFrames, UUtInt32, swap_it);
	UUmGet2BytesFromBuffer(common_data, common.sampleSize, UUtInt16, swap_it);
	common.sampleRate = (UUtUns32)x80tof((UUtUns16*)common_data);
	common_data += 10;	/* skip over the 80 bytes */
		
	// find the SSND data
	error =
		UUrFindTagData(
			inFileData,
			inFileDataLength,
			UUcFile_BigEndian,
			UUm4CharToUns32('S', 'S', 'N', 'D'),
			(UUtUns8**)&sound,
			&sound_size);
	UUmError_ReturnOnErrorMsg(error, "Unable to find SSND tag");
	
	// back up 8 bytes to get to beginning of SStContainerChunk
	*(UUtInt8**)&sound -= (sizeof(UUtUns32) + sizeof(UUtInt32));
	
	// get the offset to the sound data
	UUmSwapBig_4Byte(&sound->ckSize);
	UUmSwapBig_4Byte(&sound->offset);

	*outRawData = ((UUtUns8*)sound + sizeof(SStSoundDataChunk) + sound->offset);


	{
		UUtUns32 length_in_bytes = sound->ckSize - sizeof(UUtUns32) - sizeof(UUtUns32);
		UUtUns32 double_sample_rate = SScSamplesPerSecond * 2;

		UUrSwap_2Byte_Array(*outRawData, length_in_bytes >> 1);

		// ok if our sample rate is double, we need to down sample
		if (double_sample_rate == common.sampleRate) {
			length_in_bytes /= 2;

#if IMPORTER
			Imp_PrintMessage(IMPcMsg_Important, "downsampling %s", inDebugName);
#else
			COrConsole_Printf("downsampling %s", inDebugName);
#endif

			SSiDownSample(*outRawData, common.numChannels, length_in_bytes / (2 * common.numChannels));
		}

		outSoundData->num_bytes = length_in_bytes;
		outSoundData->data = NULL;
	
		*outRawDataLength = length_in_bytes;
	}


	
	// set the return data
	{
		UUtBool valid_sample_rate;
		UUtBool valid_bits_per_sample;

		outSoundData->flags = 0;

		if (2 == common.numChannels) {
			outSoundData->flags |= SScSoundDataFlag_Stereo;
		}

		valid_sample_rate = (SScSamplesPerSecond == common.sampleRate) || ((2 * SScSamplesPerSecond) == common.sampleRate);
		valid_bits_per_sample = (SScBitsPerSample == common.sampleSize);

		if (!valid_sample_rate || !valid_bits_per_sample) {
			// there is a sample / rate mismatch with our assumptions, warn about this
#if IMPORTER
			Imp_PrintWarning("improper sound format: '%s' is %d / %d, expecting %d / %d",
				inDebugName, common.sampleRate, common.sampleSize, SScSamplesPerSecond, SScBitsPerSample);
#else
			UUrPrintWarning("improper sound format: '%s' is %d / %d, expecting %d / %d",
				inDebugName, common.sampleRate, common.sampleSize, SScSamplesPerSecond, SScBitsPerSample);
#endif
		}


		// determine the exact duration of the sound
		outSoundData->duration_ticks = (UUtUns16) ((*outRawDataLength * UUcFramesPerSecond) / 
			(common.numChannels * SScBytesPerSample * SScSamplesPerSecond));
	}
	
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
#if 0
static UUtError
SSiSoundData_ProcessWAVE(
	const char					*inDebugName,
	UUtUns8						*inFileData,
	UUtUns32					inFileDataLength,
	SStSoundData				*outSoundData,
	UUtUns8						**outRawData,
	UUtUns32					*outRawDataLength)
{
	UUtError					error;
	UUtUns8						*riff;
	UUtUns32					riff_size;
	UUtUns8						*wav;
	SStFormat					*format;
	UUtUns32					format_size;
	UUtUns8						*data;
	UUtUns32					data_size;
	UUtBool						found;
	
	UUmAssert(inFileData);
	UUmAssertWritePtr(outSoundData, sizeof(SStSoundData));
	UUmAssertWritePtr(outRawData, sizeof(UUtUns8*));
	UUmAssertWritePtr(outRawDataLength, sizeof(UUtUns32));
	
	// clear the outgoing data
	UUrMemory_Clear(outSoundData, sizeof(SStSoundData));
	*outRawData = NULL;
	*outRawDataLength = 0;
	
	// find the RIFF data
	error =
		UUrFindTagData(
			inFileData,
			inFileDataLength,
			UUcFile_LittleEndian,
			UUm4CharToUns32('R', 'I', 'F', 'F'),
			&riff,
			&riff_size);
	UUmError_ReturnOnErrorMsg(error, "Unable to find WAVE tag");
	
	// find the WAVE
	found =
		UUrFindTag(
			riff,
			riff_size,
			UUm4CharToUns32('W', 'A', 'V', 'E'),
			&wav);
	if (found == UUcFalse)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to find WAVE tag");
	}
	
	// find the format data
	error =
		UUrFindTagData(
			wav,
			riff_size,
			UUcFile_LittleEndian,
			UUm4CharToUns32('f', 'm', 't', ' '),
			&((UUtUns8*)format),
			&format_size);
	UUmError_ReturnOnErrorMsg(error, "Unable to find format data");
	
	// find the data
	error =
		UUrFindTagData(
			wav,
			riff_size,
			UUcFile_LittleEndian,
			UUm4CharToUns32('d', 'a', 't', 'a'),
			&data,
			&data_size);
	UUmError_ReturnOnErrorMsg(error, "Unable to find data data");
	
	// process the format
	UUmSwapLittle_2Byte(&format->wFormatTag);
	UUmSwapLittle_2Byte(&format->nChannels);
	UUmSwapLittle_4Byte(&format->nSamplesPerSec);
	UUmSwapLittle_4Byte(&format->nAvgBytesPerSec);
	UUmSwapLittle_2Byte(&format->nBlockAlign);
	UUmSwapLittle_2Byte(&format->wBitsPerSample);
	UUmSwapLittle_2Byte(&format->cbSize);
	
	// set the return data
	outSoundData->flags = (format->wFormatTag == 2) ? SScSoundDataFlag_MSADPCM : SScSoundDataFlag_PCM_Little;
	outSoundData->f = *format;
	
	*outRawData = data;
	*outRawDataLength = data_size;

	if (format->wFormatTag == 2) {
		// this data is compressed, we do not know what the duration of this sound is
		outSoundData->duration_ticks = 0;
	} else {
		// this data is uncompressed, calculate the sound's duration in ticks
		outSoundData->duration_ticks = (UUtUns16) ((*outRawDataLength * UUcFramesPerSecond) / 
			(format->nChannels * (format->wBitsPerSample >> 3) * format->nSamplesPerSec));
	}


	// byte swap the data on Big Endian machines
#if defined(UUmEndian) && (UUmEndian == UUmEndian_Big)
	if (outSoundData->f.wBitsPerSample == 16)
	{
		UUtUns32			i;
		UUtUns32			num_shorts;
		UUtUns16			*shorts;
		
		num_shorts = (*outRawDataLength) >> 1;
		shorts = (UUtUns16*)(*outRawData);
		
		for (i = 0; i < num_shorts; i++)
		{
			UUmSwapLittle_2Byte((void*)shorts);
			shorts++;
		}
	}
#endif
	
	return UUcError_None;
}
#endif

// ----------------------------------------------------------------------
UUtError
SSrSoundData_Process(
	const char					*inDebugName,
	UUtUns8						*inFileData,
	UUtUns32					inFileDataLength,
	SStSoundData				*outSoundData,
	UUtUns8						**outRawData,
	UUtUns32					*outRawDataLength)
{
	UUtError					error;
	UUtUns8						*data;
	UUtUns32					data_size;
	
	// if RIFF data is found, the file is a WAVE file so process it as such
	error =
		UUrFindTagData(
			inFileData,
			inFileDataLength,
			UUcFile_LittleEndian,
			UUm4CharToUns32('R', 'I', 'F', 'F'),
			&data,
			&data_size);
	if (error == UUcError_None)
	{
#if 0
		error =
			SSiSoundData_ProcessWAVE(
				inDebugName,
				inFileData,
				inFileDataLength,
				outSoundData,
				outRawData,
				outRawDataLength);
		UUmError_ReturnOnError(error);
#endif

		UUmAssert(!"wave file format not supported");
		
		return UUcError_None;
	}
	
	// if FORM data is found, the file is an AIFF file so process it as such
	error =
		UUrFindTagData(
			inFileData,
			inFileDataLength,
			UUcFile_BigEndian,
			UUm4CharToUns32('F', 'O', 'R', 'M'),
			&data,
			&data_size);
	if (error == UUcError_None)
	{
		error =
			SSiSoundData_ProcessAIFF(
				inDebugName,
				inFileData,
				inFileDataLength,
				outSoundData,
				outRawData,
				outRawDataLength);
		UUmError_ReturnOnError(error);
		
		return UUcError_None;
	}
	
	return UUcError_Generic;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtBool
SSiSoundChannel_CanPan(
	SStSoundChannel				*inSoundChannel)
{
	if (inSoundChannel == NULL) { return UUcFalse; }
	return ((inSoundChannel->status & SScSCStatus_CanPan) != 0);
}

// ----------------------------------------------------------------------
static float
SSiSoundChannel_GetVolume(
	SStSoundChannel				*inSoundChannel)
{
	if (inSoundChannel == NULL) { return 0.0f; }
	return	inSoundChannel->volume;
}

// ----------------------------------------------------------------------
UUtBool
SSiSoundChannel_IsAmbient(
	SStSoundChannel				*inSoundChannel)
{
	if (inSoundChannel == NULL) { return UUcFalse; }
	return ((inSoundChannel->status & SScSCStatus_Ambient) != 0);
}

// ----------------------------------------------------------------------
UUtBool
SSiSoundChannel_IsLocked(
	SStSoundChannel				*inSoundChannel)
{
	if (inSoundChannel == NULL) { return UUcFalse; }
	return ((inSoundChannel->status & SScSCStatus_Locked) != 0);
}

// ----------------------------------------------------------------------
UUtBool
SSiSoundChannel_IsLooping(
	SStSoundChannel				*inSoundChannel)
{
	if (inSoundChannel == NULL) { return UUcFalse; }
	return ((inSoundChannel->status & SScSCStatus_Looping) != 0);
}

// ----------------------------------------------------------------------
UUtBool
SSiSoundChannel_IsPaused(
	SStSoundChannel				*inSoundChannel)
{
	if (inSoundChannel == NULL) { return UUcFalse; }
	return ((inSoundChannel->status & SScSCStatus_Paused) != 0);
}

// ----------------------------------------------------------------------
UUtBool
SSiSoundChannel_IsPlaying(
	SStSoundChannel				*inSoundChannel)
{
	if (inSoundChannel == NULL) { return UUcFalse; }
#if UUmOpenAL
	return SS2rPlatform_SoundChannel_IsPlaying(inSoundChannel);
#else
	return ((inSoundChannel->status & SScSCStatus_Playing) != 0);
#endif
}

// ----------------------------------------------------------------------
UUtBool
SSiSoundChannel_IsUpdating(
	SStSoundChannel				*inSoundChannel)
{
	if (inSoundChannel == NULL) { return UUcFalse; }
	return ((inSoundChannel->status & SScSCStatus_Updating) != 0);
}

// ----------------------------------------------------------------------
static void
SSiSoundChannel_Pause(
	SStSoundChannel				*inSoundChannel)
{
	// pause the sound channel
	SS2rPlatform_SoundChannel_Pause(inSoundChannel);
}

// ----------------------------------------------------------------------
static UUtBool
SSiSoundChannel_Play(
	SStSoundChannel				*inSoundChannel,
	SStSoundData				*inSoundData)
{
	UUtBool						result;
	
	UUmAssert(inSoundChannel);
	UUmAssert(inSoundData);

	// play inSoundData on the inSoundChannel
	result = SS2rPlatform_SoundChannel_SetSoundData(inSoundChannel, inSoundData);
	if (result == UUcFalse) { return UUcFalse; }
	
	SS2rPlatform_SoundChannel_Play(inSoundChannel);
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
static void
SSiSoundChannel_Resume(
	SStSoundChannel				*inSoundChannel)
{
	// pause the sound channel
	SS2rPlatform_SoundChannel_Resume(inSoundChannel);
}

// ----------------------------------------------------------------------
static void
SSiSoundChannel_Stop(
	SStSoundChannel				*inSoundChannel)
{
	UUmAssert(inSoundChannel);

	SS2rPlatform_SoundChannel_Stop(inSoundChannel);
}

// ----------------------------------------------------------------------
void
SSiSoundChannel_SetAmbient(
	SStSoundChannel				*inSoundChannel,
	UUtBool						inAmbient)
{
	UUmAssert(inSoundChannel);
	
	if (inAmbient)
	{
		inSoundChannel->status |= SScSCStatus_Ambient;
	}
	else
	{
		inSoundChannel->status &= ~SScSCStatus_Ambient;
	}
}

// ----------------------------------------------------------------------
void
SSiSoundChannel_SetCanPan(
	SStSoundChannel				*inSoundChannel,
	UUtBool						inCanPan)
{
	UUmAssert(inSoundChannel);
	
	if (inCanPan)
	{
		inSoundChannel->status |= SScSCStatus_CanPan;
	}
	else
	{
		inSoundChannel->status &= ~SScSCStatus_CanPan;
	}
}

// ----------------------------------------------------------------------
void
SSiSoundChannel_SetLock(
	SStSoundChannel				*inSoundChannel,
	UUtBool						inLocked)
{
	UUmAssert(inSoundChannel);

	if (inLocked)
	{
		inSoundChannel->status |= SScSCStatus_Locked;
	}
	else
	{
		inSoundChannel->status &= ~SScSCStatus_Locked;
	}
}

// ----------------------------------------------------------------------
void
SSiSoundChannel_SetLooping(
	SStSoundChannel				*inSoundChannel,
	UUtBool						inLooping)
{
	UUmAssert(inSoundChannel);

	if (inLooping)
	{
		inSoundChannel->status |= SScSCStatus_Looping;
	}
	else
	{
		inSoundChannel->status &= ~SScSCStatus_Looping;
	}
}

// ----------------------------------------------------------------------
static void
SSiSoundChannel_SetPan(
	SStSoundChannel				*inSoundChannel,
	UUtUns32					inPanFlag,
	float						inPan)
{
	UUmAssert(inSoundChannel);
	
	switch (inPanFlag)
	{
		case SScPanFlag_Left:
			inSoundChannel->pan_left = 1.0f;
			inSoundChannel->pan_right = inPan;
		break;
		
		case SScPanFlag_Right:
			inSoundChannel->pan_left = inPan;
			inSoundChannel->pan_right = 1.0f;
		break;
		
		case SScPanFlag_None:
		default:
			inSoundChannel->pan_left = 1.0f;
			inSoundChannel->pan_right = 1.0f;
		break;
	}
	
	SS2rPlatform_SoundChannel_SetPan(inSoundChannel, inPanFlag, inPan);
}

// ----------------------------------------------------------------------
void
SSiSoundChannel_SetPaused(
	SStSoundChannel				*inSoundChannel,
	UUtBool						inPaused)
{
	UUmAssert(inSoundChannel);

	if (inPaused)
	{
		inSoundChannel->status |= SScSCStatus_Paused;
	}
	else
	{
		inSoundChannel->status &= ~SScSCStatus_Paused;
	}
}

// ----------------------------------------------------------------------
static void
SSiSoundChannel_SetPitch(
	SStSoundChannel				*inSoundChannel,
	float						inPitch)
{
	float						prev_pitch;
	
	UUmAssert(inSoundChannel);
	
	prev_pitch = inSoundChannel->pitch;
	inSoundChannel->pitch = inPitch;
	
	if (inSoundChannel->group != NULL)
	{
		inSoundChannel->pitch *= inSoundChannel->group->group_pitch;
	}
	
	if (prev_pitch != inSoundChannel->pitch)
	{
		SS2rPlatform_SoundChannel_SetPitch(inSoundChannel, inSoundChannel->pitch);
	}
}

// ----------------------------------------------------------------------
void
SSiSoundChannel_SetPlaying(
	SStSoundChannel				*inSoundChannel,
	UUtBool						inPlaying)
{
	UUmAssert(inSoundChannel);

	if (inPlaying)
	{
		inSoundChannel->status |= SScSCStatus_Playing;
	}
	else
	{
		inSoundChannel->status &= ~SScSCStatus_Playing;
	}
}

// ----------------------------------------------------------------------
void
SSiSoundChannel_SetUpdating(
	SStSoundChannel				*inSoundChannel,
	UUtBool						inUpdating)
{
	UUmAssert(inSoundChannel);

	if (inUpdating)
	{
		inSoundChannel->status |= SScSCStatus_Updating;
	}
	else
	{
		inSoundChannel->status &= ~SScSCStatus_Updating;
	}
}

// ----------------------------------------------------------------------
static void
SSiSoundChannel_CalcVolume(
	SStSoundChannel				*inSoundChannel)
{
	inSoundChannel->volume = 
		SSgOverallVolume *
		inSoundChannel->channel_volume *
		inSoundChannel->distance_volume *
		inSoundChannel->permutation_volume;
	
	if (inSoundChannel->group != NULL)
	{
		inSoundChannel->volume *= inSoundChannel->group->group_volume;
	}
}

// ----------------------------------------------------------------------
static void
SSiSoundChannel_SetChannelVolume(
	SStSoundChannel				*inSoundChannel,
	float						inChannelVolume)
{
	UUmAssert(inSoundChannel);
	
	inSoundChannel->channel_volume = UUmPin(inChannelVolume, 0.0f, 2.0f);
	SSiSoundChannel_CalcVolume(inSoundChannel);
	SS2rPlatform_SoundChannel_SetVolume(inSoundChannel, inSoundChannel->volume);
}

// ----------------------------------------------------------------------
static void
SSiSoundChannel_SetDistanceVolume(
	SStSoundChannel				*inSoundChannel,
	float						inDistanceVolume)
{
	UUmAssert(inSoundChannel);

	inSoundChannel->distance_volume = UUmPin(inDistanceVolume, 0.0f, 2.0f);
	SSiSoundChannel_CalcVolume(inSoundChannel);
	SS2rPlatform_SoundChannel_SetVolume(inSoundChannel, inSoundChannel->volume);
}

// ----------------------------------------------------------------------
static void
SSiSoundChannel_SetPermutationVolume(
	SStSoundChannel				*inSoundChannel,
	float						inPermutationVolume)
{
	UUmAssert(inSoundChannel);

	inSoundChannel->permutation_volume = UUmPin(inPermutationVolume, 0.0f, 2.0f);
	SSiSoundChannel_CalcVolume(inSoundChannel);
	SS2rPlatform_SoundChannel_SetVolume(inSoundChannel, inSoundChannel->volume);
}

// ----------------------------------------------------------------------
static UUtBool
SSiSoundChannel_Spatialize(
	SStSoundChannel				*inSoundChannel,
	float						inMaxVolDistance,
	float						inMinVolDistance,
	const M3tPoint3D			*inPosition,
	const M3tPoint3D			*inDirection,
	const M3tPoint3D			*inVelocity)
{
	float						distance_volume;
	M3tVector3D					ortho_vector;
	M3tVector3D					to_sound;
	float						dot;
	float						theta;
	float						pan;
	UUtUns32					pan_flag;
	
	M3tPoint3D					position;
	M3tPoint3D					listener;
	float						dist_squared;
	
	UUmAssert(inSoundChannel);
	
	// calculate the distance to the listener
	if (inPosition == NULL) {
		inSoundChannel->position = SSgListener_Position;
		inSoundChannel->distance_to_listener = 0.0f;
		inSoundChannel->previous_distance_to_listener = 0.0f;
	} else {
		inSoundChannel->position = *inPosition;
		inSoundChannel->previous_distance_to_listener =
			inSoundChannel->distance_to_listener;
		inSoundChannel->distance_to_listener =
			MUrPoint_Distance(inPosition, &SSgListener_Position) * SScFootToDist;
	}
	
	if (inSoundChannel->distance_to_listener < inMaxVolDistance)
	{
		// the volume is at it's max
		distance_volume = 1.0f;
	}
	else
	{
		float				diff;
		
		diff = (inMinVolDistance - inMaxVolDistance);
	
		// calculate the distance_volume
		if (diff <= 0.0f)
		{
			distance_volume = 0.0f;
		}
		else
		{
			distance_volume = (inMinVolDistance - inSoundChannel->distance_to_listener) / diff;
			if (distance_volume < 0.0f) { distance_volume = 0.0f; }
		}
	}
	
	// set the distance volume
	SSiSoundChannel_SetDistanceVolume(inSoundChannel, distance_volume);
	if (distance_volume <= 0.0f) { return UUcFalse; }
	
	// exit if the channel doesn't calculate the pan
	inSoundChannel->pan_left = 1.0f;
	inSoundChannel->pan_right = 1.0f;
	if (SSiSoundChannel_CanPan(inSoundChannel) == UUcFalse)
	{
		pan_flag = SScPanFlag_None;
		pan = 1.0f;
		return UUcTrue;
	}
	else
	{
		if (inPosition == NULL) {
			dot = 0.0f;
		} else {
			// calculate the pan
			MUmVector_Set(position, inPosition->x, 0.0f, inPosition->z);
			MUmVector_Set(listener, SSgListener_Position.x, 0.0f, SSgListener_Position.z);
			dist_squared = MUrPoint_Distance_Squared(&position, &listener);
			if ((MUrPoint_IsEqual(&position, &listener)) || (dist_squared <= SScNoPanDistanceSquared))
			{
				dot = 0.0f;
			}
			else
			{
				MUmVector_Set(ortho_vector, -SSgListener_Facing.z, SSgListener_Facing.y, SSgListener_Facing.x);
				MUmVector_Subtract(to_sound, *inPosition, SSgListener_Position);
				dot = MUrVector_DotProduct(&ortho_vector, &to_sound);
				theta = MUrAngleBetweenVectors(SSgListener_Facing.x, SSgListener_Facing.z, to_sound.x, to_sound.z);
				theta = UUmPin(theta, 0.0f, M3cPi);
			}
		}

		if (dot < 0.0f)
		{
			// pan left
			pan_flag = SScPanFlag_Left;
			pan = 0.5f + (float)fabs((theta - M3cHalfPi) * 0.3183099f /*S.S./ M3cPi*/);
		}
		else if (dot > 0.0f)
		{
			// pan right
			pan_flag = SScPanFlag_Right;
			pan = 0.5f + (float)fabs((theta - M3cHalfPi) * 0.3183099f /*S.S./ M3cPi*/);
		}
		else
		{
			pan_flag = SScPanFlag_None;
			pan = 1.0f;
		}
	}
	
	// set the pan volume
	SSiSoundChannel_SetPan(inSoundChannel, pan_flag, pan);
	
	return UUcTrue;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static SStSoundChannel*
SSiSoundChannels_GetChannelForPlay(
	SStSoundChannel				*inSoundChannels,
	UUtUns32					inNumChannels,
	SStPriority2				inPriority)
{
	SStSoundChannel				*sound_channel_array;
	SStSoundChannel				*found_channel;
	UUtUns32					i;
	UUtUns32					num_channels;
	float						distance;
	
	UUmAssert(inSoundChannels);
	
	found_channel = NULL;
	distance = 0.0f;
	
	sound_channel_array = inSoundChannels;
	num_channels = inNumChannels;
	
	// look for a channel which isn't locked and isn't playing anything
	for (i = 0; i < num_channels; i++)
	{
		if ((SSiSoundChannel_IsLocked(&sound_channel_array[i]) == UUcFalse) &&
			(SSiSoundChannel_IsPlaying(&sound_channel_array[i]) == UUcFalse))
		{
			found_channel = &sound_channel_array[i];
			break;
		}
	}
	
	// get the first available channel which isn't locked and is playing a sound that
	// has a lower priority
	if (found_channel == NULL)
	{
		for (i = 0; i < num_channels; i++)
		{
			if ((SSiSoundChannel_IsLocked(&sound_channel_array[i]) == UUcFalse) &&
				(sound_channel_array[i].priority < inPriority))
			{
				found_channel = &sound_channel_array[i];
				if (SSiSoundChannel_IsPlaying(found_channel) == UUcTrue)
				{
					SSiSoundChannel_Stop(found_channel);
				}
				break;
			}
		}
	}
	
	// look for a channel which is playing any sound of the same or lower or equal priority
	// that isn't locked and is near zero volume
	if (found_channel == NULL)
	{
		for (i = 0; i < num_channels; i++)
		{
			if ((SSiSoundChannel_IsLocked(&sound_channel_array[i]) == UUcFalse) &&
				(sound_channel_array[i].priority <= inPriority) &&
				(sound_channel_array[i].volume <= 0.1f))
			{
				found_channel = &sound_channel_array[i];
				if (SSiSoundChannel_IsPlaying(found_channel) == UUcTrue)
				{
					SSiSoundChannel_Stop(found_channel);
				}
				break;
			}
		}
	}
	
	// look for a channel which is playing with a lower or equal priority but
	// with a lower volume
	if (found_channel == NULL)
	{
		float					min_volume = 1.0f;
		
		for (i = 0; i < num_channels; i++)
		{
			if ((sound_channel_array[i].priority <= inPriority) &&
				(sound_channel_array[i].volume < min_volume))
			{
				found_channel = &sound_channel_array[i];
				min_volume = found_channel->volume;
			}
		}
		
		if (found_channel)
		{
			if (SSiSoundChannel_IsAmbient(found_channel) == UUcTrue)
			{
				SSiPlayingAmbient_Stall(found_channel);
			}
			else if (SSiSoundChannel_IsPlaying(found_channel) == UUcTrue)
			{
				SSiSoundChannel_Stop(found_channel);
			}
		}
	}
	
	// init found_channel
	if (found_channel != NULL)
	{
		found_channel->distance_to_listener = 0.0f;
		found_channel->previous_distance_to_listener = 0.0f;
		found_channel->pan_left = 1.0f;
		found_channel->pan_right = 1.0f;
	}
	
	return found_channel;
}

// ----------------------------------------------------------------------
SStSoundChannel*
SSiSoundChannels_GetChannelByID(
	UUtUns32					inSoundChannelID)
{
	SStSoundChannel				*sound_channel_array;
	UUtUns32					i;
	UUtUns32					num_channels;
	
	// search for the channel in the mono channel array
	sound_channel_array = SSgSoundChannels_Mono;
	num_channels = SSgSoundChannels_NumMono;
	for (i = 0; i < num_channels; i++)
	{
		if (sound_channel_array[i].id == inSoundChannelID)
		{
			return &sound_channel_array[i];
		}
	}
	
	// search for the channel in the stereo channel array
	sound_channel_array = SSgSoundChannels_Stereo;
	num_channels = SSgSoundChannels_NumStereo;
	for (i = 0; i < num_channels; i++)
	{
		if (sound_channel_array[i].id == inSoundChannelID)
		{
			return &sound_channel_array[i];
		}
	}
	
	return NULL;
}

// ----------------------------------------------------------------------
UUtUns32
SSiSoundChannels_GetNumChannels(
	void)
{
	UUtUns32					num_channels;
	
	num_channels = SSgSoundChannels_NumMono + SSgSoundChannels_NumStereo;
				
	return num_channels;
}

// ----------------------------------------------------------------------
static UUtError
SSiSoundChannels_Initialize(
	UUtUns32					inNumChannels,
	UUtUns32					*outNumStereoChannels,
	UUtUns32					*outNumMonoChannels)
{
	UUtUns32					i;
	SStSoundChannel				*sound_channels;
	UUtUns32					num_stereo_channels;
	UUtUns32					num_mono_channels;
	UUtUns16					sound_channel_id;
	
	UUmAssert(SSgSoundChannels_Stereo == NULL);
	UUmAssert(SSgSoundChannels_Mono == NULL);
	
	// calculate the number of stereo channels
	num_stereo_channels = (UUtUns32)(((float)inNumChannels * SScPercentStereo) + 0.5f);
	num_mono_channels = inNumChannels - num_stereo_channels;
	
	*outNumStereoChannels = num_stereo_channels;
	*outNumMonoChannels = num_mono_channels;
	
	// allocate memory for the stereo sound channels
	if (num_stereo_channels > 0)
	{
		SSgSoundChannels_Stereo =
			(SStSoundChannel*)UUrMemory_Block_New(sizeof(SStSoundChannel) * num_stereo_channels);
		UUmError_ReturnOnNull(SSgSoundChannels_Stereo);
	}
	// allocate memory for the mono sound channels
	if (num_mono_channels > 0)
	{
		SSgSoundChannels_Mono =
			(SStSoundChannel*)UUrMemory_Block_New(sizeof(SStSoundChannel) * num_mono_channels);
		UUmError_ReturnOnNull(SSgSoundChannels_Mono);
	}

	sound_channel_id = 0;
	
	// initialize each stereo sound channel
	sound_channels = SSgSoundChannels_Stereo;
	for (i = 0; i < num_stereo_channels; i++)
	{
//		SSrCreateGuard(&sound_channels[i].guard);
		sound_channels[i].id = sound_channel_id++;
		sound_channels[i].flags = SScSoundChannelFlag_Stereo;
		sound_channels[i].status = SScSCStatus_None;
		sound_channels[i].priority = SScPriority2_Normal;
		sound_channels[i].group = NULL;
		sound_channels[i].sound_data = NULL;
		MUmVector_Set(sound_channels[i].position, 0.0f, 0.0f, 0.0f);
		sound_channels[i].distance_to_listener = 0.0f;
		sound_channels[i].previous_distance_to_listener = 0.0f;
		sound_channels[i].volume = 1.0f;
		sound_channels[i].pitch = 1.0f;
		sound_channels[i].channel_volume = 1.0f;
		sound_channels[i].distance_volume = 1.0f;
		sound_channels[i].permutation_volume = 1.0f;
		sound_channels[i].pan_left = 1.0f;
		sound_channels[i].pan_right = 1.0f;
		SS2rPlatform_SoundChannel_Initialize(&sound_channels[i]);
	}
	
	// initialize each mono sound channel
	sound_channels = SSgSoundChannels_Mono;
	for (i = 0; i < num_mono_channels; i++)
	{
//		SSrCreateGuard(&sound_channels[i].guard);
		sound_channels[i].id = sound_channel_id++;
		sound_channels[i].flags = SScSoundChannelFlag_Mono;
		sound_channels[i].status = SScSCStatus_None;
		sound_channels[i].priority = SScPriority2_Normal;
		sound_channels[i].group = NULL;
		sound_channels[i].sound_data = NULL;
		MUmVector_Set(sound_channels[i].position, 0.0f, 0.0f, 0.0f);
		sound_channels[i].distance_to_listener = 0.0f;
		sound_channels[i].previous_distance_to_listener = 0.0f;
		sound_channels[i].volume = 1.0f;
		sound_channels[i].pitch = 1.0f;
		sound_channels[i].channel_volume = 1.0f;
		sound_channels[i].distance_volume = 1.0f;
		sound_channels[i].permutation_volume = 1.0f;
		sound_channels[i].pan_left = 1.0f;
		sound_channels[i].pan_right = 1.0f;
		SS2rPlatform_SoundChannel_Initialize(&sound_channels[i]);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
SSiSoundChannels_Silence(
	void)
{
	UUtUns32					i;
	SStSoundChannel				*sound_channel_array;
	UUtUns32					num_elements;
	
	SSrWaitGuard(SSgGuardAll);
	
	if (SSgSoundChannels_Mono)
	{
		// go through the sound channels and do any platform specific termination
		sound_channel_array = SSgSoundChannels_Mono;
		num_elements = SSgSoundChannels_NumMono;
		for (i = 0; i < num_elements; i++)
		{
			if (SSiSoundChannel_IsPlaying(&sound_channel_array[i]) == UUcTrue) { continue; }
			SS2rPlatform_SoundChannel_Silence(&sound_channel_array[i]);
			SS2rPlatform_SoundChannel_SetVolume(&sound_channel_array[i], 0.0f);
		}
	}

	if (SSgSoundChannels_Stereo)
	{
		// go through the sound channels and do any platform specific termination
		sound_channel_array = SSgSoundChannels_Stereo;
		num_elements = SSgSoundChannels_NumStereo;
		for (i = 0; i < num_elements; i++)
		{
			if (SSiSoundChannel_IsPlaying(&sound_channel_array[i]) == UUcTrue) { continue; }
			SS2rPlatform_SoundChannel_Silence(&sound_channel_array[i]);
			SS2rPlatform_SoundChannel_SetVolume(&sound_channel_array[i], 0.0f);
		}
	}
	
	SSrReleaseGuard(SSgGuardAll);
}

// ----------------------------------------------------------------------
static void
SSiSoundChannels_Terminate(
	void)
{
	UUtUns32					i;
	SStSoundChannel				*sound_channel_array;
	UUtUns32					num_elements;
	
	if (SSgSoundChannels_Mono)
	{
		// go through the sound channels and do any platform specific termination
		sound_channel_array = SSgSoundChannels_Mono;
		num_elements = SSgSoundChannels_NumMono;
		for (i = 0; i < num_elements; i++)
		{
			SS2rPlatform_SoundChannel_Terminate(&sound_channel_array[i]);
		}
		
		// delete the sound channel array
		UUrMemory_Block_Delete(SSgSoundChannels_Mono);
		SSgSoundChannels_Mono = NULL;
		SSgSoundChannels_NumMono = 0;
	}

	if (SSgSoundChannels_Stereo)
	{
		// go through the sound channels and do any platform specific termination
		sound_channel_array = SSgSoundChannels_Stereo;
		num_elements = SSgSoundChannels_NumStereo;
		for (i = 0; i < num_elements; i++)
		{
			SS2rPlatform_SoundChannel_Terminate(&sound_channel_array[i]);
		}
		
		// delete the sound channel array
		UUrMemory_Block_Delete(SSgSoundChannels_Stereo);
		SSgSoundChannels_Stereo = NULL;
		SSgSoundChannels_NumStereo = 0;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
const char*
SSrPermutation_GetName(
	SStPermutation				*inPermutation)
{
	UUmAssert(inPermutation);
	return inPermutation->sound_data_name;
}

// ----------------------------------------------------------------------
void
SSrPermutation_Play(
	SStPermutation				*inPermutation,
	SStSoundChannel				*inSoundChannel,	/* optional */
	const char					*inDebugSoundType,
	const char					*inDebugSoundName)
{
	float						play_volume;
	float						play_pitch;
	SStSoundChannel				*sound_channel;
	
	UUmAssert(inPermutation);
	 
	if (SSgUsable == UUcFalse) { return; }

	if (inPermutation->sound_data == NULL)
	{
		inPermutation->sound_data =
			SSrSoundData_GetByName(
				inPermutation->sound_data_name,
				UUcFalse);

		if (inPermutation->sound_data == NULL) {
			COrConsole_Printf("SOUND NOT FOUND: %s %s - sound %s", 
					(inDebugSoundType == NULL) ? "" : inDebugSoundType,
					(inDebugSoundName == NULL) ? "" : inDebugSoundName, inPermutation->sound_data_name);
			return;
		}
	}
	
	// search for a channel to use
	if (inSoundChannel)
	{
		sound_channel = inSoundChannel;
	}
	else
	{
		UUtUns32 num_channels = SSrSound_GetNumChannels(inPermutation->sound_data);

		if (1 == num_channels)
		{
			sound_channel =
				SSiSoundChannels_GetChannelForPlay(
					SSgSoundChannels_Mono,
					SSgSoundChannels_NumMono,
					SScPriority2_Normal);
		}
		else if (2 == num_channels)
		{
			sound_channel =
				SSiSoundChannels_GetChannelForPlay(
					SSgSoundChannels_Stereo,
					SSgSoundChannels_NumStereo,
					SScPriority2_Normal);
		}
	}
	if (sound_channel == NULL) { return; }
	
	// pick a volume
	play_volume =
		UUmRandomRangeFloat(
			inPermutation->min_volume_percent,
			inPermutation->max_volume_percent);
	
	// pick a pitch
	play_pitch =
		UUmRandomRangeFloat(
			inPermutation->min_pitch_percent,
			inPermutation->max_pitch_percent);
	
	// set the volume
	SSiSoundChannel_SetPermutationVolume(sound_channel, play_volume);
	
	// play the permutation
	SSiSoundChannel_Play(sound_channel, inPermutation->sound_data);
	
	// set the pitch
	SSiSoundChannel_SetPitch(sound_channel, play_pitch);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static int UUcExternal_Call
SSiGroup_Compare(
	const void					*inItem1,
	const void					*inItem2)
{
	const SStGroup				*group1;
	const SStGroup				*group2;
	
	group1 = *((SStGroup**)inItem1);
	group2 = *((SStGroup**)inItem2);
		
	return strcmp(group1->group_name, group2->group_name);
}

// ----------------------------------------------------------------------
static SStPermutation*
SSiGroup_SelectPermutation(
	SStGroup					*inGroup)
{
	UUtUns32					total_weight, cumulative_weight;
	UUtUns32					num_permutations;
	SStPermutation				*permutation_array;
	SStPermutation				*permutation;
	UUtUns32					i;
	UUtUns32					random_num;
	UUtBool						done;
	UUtUns32					count;
	UUtUns16					selected;
	
	// get the number of permutations and a pointer to the permutation_array
	num_permutations = UUrMemory_Array_GetUsedElems(inGroup->permutations);
	permutation_array = (SStPermutation*)UUrMemory_Array_GetMemory(inGroup->permutations);
	
	if (num_permutations == 0) { return NULL; }
	if (num_permutations == 1) { return &permutation_array[0]; }
	
	// calculate the sum of all the weights of the permutations
	total_weight = 0;
	for (i = 0; i < num_permutations; i++)
	{
		total_weight += permutation_array[i].weight;
	}
	
	done = UUcTrue;
	count = 0;
	selected = 0;
	do
	{
		// pick a permutation
		random_num = UUmLocalRandomRange(0, total_weight);
		
		// find the permutation which corresponds to this random number
		permutation = NULL;
		cumulative_weight = 0;
		for (i = 0; i < num_permutations; i++)
		{
			cumulative_weight += permutation_array[i].weight;
			if (random_num <= cumulative_weight)
			{
				permutation = &permutation_array[i];
				selected = (UUtUns16)i;
				break;
			}
		}
		UUmAssert(i < num_permutations);
		UUmAssert(permutation != NULL);
		
		// when the prevent repeats flag is set, then pick a new permutation
		// if this is the same permutation that was picked the previous time
		// this sound group was played
		if (((inGroup->flags & SScGroupFlag_PreventRepeats) != 0) &&
			(num_permutations > 1) &&
			(i == inGroup->flag_data))
		{
			done = UUcFalse;
		}
		
		count++;
	}
	while ((done == UUcFalse) && (count < 10));
	
	inGroup->flag_data = selected;
	
	return permutation;
}

// ----------------------------------------------------------------------
UUtBool
SSrGroup_CheckSoundData(
	const SStGroup				*inGroup)
{
	UUtUns32					num_permutations;
	SStPermutation				*permutation_array;
	UUtUns32					itr;
	
	// get the number of permutations and a pointer to the permutation_array
	num_permutations = UUrMemory_Array_GetUsedElems(inGroup->permutations);
	permutation_array = (SStPermutation*)UUrMemory_Array_GetMemory(inGroup->permutations);
	for (itr = 0; itr < num_permutations; itr++)
	{
		if (permutation_array[itr].sound_data == NULL) { return UUcFalse; }
	}
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtError
SSrGroup_Copy(
	const SStGroup				*inSource,
	SStGroup					*ioDest)
{
	UUtError					error;
	UUtUns32					num_permutations;
	
	UUmAssert(inSource);
	UUmAssert(inSource->permutations);
	UUmAssert(ioDest);
	UUmAssert(ioDest->permutations);
	
	// clear ioDest's permutations array
	num_permutations = UUrMemory_Array_GetUsedElems(ioDest->permutations);
	if (num_permutations > 0)
	{		
		while (num_permutations--)
		{
			UUrMemory_Array_DeleteElement(ioDest->permutations, 0);
		}
	}
	
	// copy the permutations from inSource to ioDest
	ioDest->num_channels = inSource->num_channels;
	
	num_permutations = UUrMemory_Array_GetUsedElems(inSource->permutations);
	if (num_permutations > 0)
	{
		SStPermutation				*src_permutations;
		SStPermutation				*dst_permutations;
		UUtBool						mem_moved;
		UUtUns32					i;
		
		// set the number of permutations used in ioDest
		error = UUrMemory_Array_SetUsedElems(ioDest->permutations, num_permutations, &mem_moved);
		UUmError_ReturnOnError(error);
		
		// get a pointer to the inSource permutations
		src_permutations = (SStPermutation*)UUrMemory_Array_GetMemory(inSource->permutations);
		UUmAssert(src_permutations);
		
		// get a pointer to the ioDest permutations
		dst_permutations = (SStPermutation*)UUrMemory_Array_GetMemory(ioDest->permutations);
		UUmAssert(dst_permutations);
		
		// copy the permutation data from inSource to ioDest
		for (i = 0; i < num_permutations; i++)
		{
			dst_permutations[i] = src_permutations[i];
		}
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
SSrGroup_Delete(
	const char					*inGroupName,
	UUtUns32					inUpdatePointers)
{
	UUtUns32					num_sound_groups;
	SStGroup					**group_array;
	UUtUns32					i;
	
	num_sound_groups = UUrMemory_Array_GetUsedElems(SSgSoundGroups);
	group_array = (SStGroup**)UUrMemory_Array_GetMemory(SSgSoundGroups);
	
	for (i = 0; i < num_sound_groups; i++)
	{
		if (UUrString_Compare_NoCase(group_array[i]->group_name, inGroupName) != 0) { continue; }

		// delete the permutations
		if (group_array[i]->permutations)
		{
			UUrMemory_Array_Delete(group_array[i]->permutations);
			group_array[i]->permutations = NULL;
		}
		
		// delete the sound group
		UUrMemory_Block_Delete(group_array[i]);
		
		// delete the sound group pointer
		UUrMemory_Array_DeleteElement(SSgSoundGroups, i);
		
		if (inUpdatePointers == UUcTrue)
		{
			// update the pointers
			SSrAmbient_UpdateGroupName(inGroupName, NULL);
			SSrImpulse_UpdateGroupName(inGroupName, NULL);
		}
		
		break;
	}
}

// ----------------------------------------------------------------------
SStGroup*
SSrGroup_GetByName(
	const char					*inGroupName)
{
	SStGroup					**group_array;
	SStGroup					find_me;
	SStGroup					*find_me_ptr = &find_me;
	SStGroup					**found_group;
	UUtUns32					num_elements;
	
	if (inGroupName == NULL) { return NULL; }
	
	UUrString_Copy(find_me.group_name, inGroupName, SScMaxNameLength);
	
	// get a pointer to the array
	group_array = (SStGroup**)UUrMemory_Array_GetMemory(SSgSoundGroups);
	if (group_array == NULL) { return NULL; }
	
	num_elements = UUrMemory_Array_GetUsedElems(SSgSoundGroups);
	
	found_group =
		(SStGroup**)bsearch(
			&find_me_ptr,
			group_array,
			num_elements,
			sizeof(SStGroup*),
			SSiGroup_Compare);
	if (found_group == NULL) { return NULL; }

	return *found_group;
}

// ----------------------------------------------------------------------
SStGroup*
SSrGroup_GetByIndex(
	UUtUns32					inIndex)
{
	UUtUns32					num_sound_groups;
	SStGroup					**group_array;
	
	num_sound_groups = UUrMemory_Array_GetUsedElems(SSgSoundGroups);
	group_array = (SStGroup**)UUrMemory_Array_GetMemory(SSgSoundGroups);
	
	UUmAssert(num_sound_groups > inIndex);
	
	return group_array[inIndex];
}

// ----------------------------------------------------------------------
UUtUns32
SSrGroup_GetNumChannels(
	const SStGroup				*inGroup)
{
	UUmAssert(inGroup);
	return inGroup->num_channels;
}

// ----------------------------------------------------------------------
UUtUns32
SSrGroup_GetNumPermutations(
	const SStGroup				*inGroup)
{
	UUmAssert(inGroup);
	return UUrMemory_Array_GetUsedElems(inGroup->permutations);
}

// ----------------------------------------------------------------------
UUtUns32
SSrGroup_GetNumSoundGroups(
	void)
{
	UUmAssert(SSgSoundGroups);
	return UUrMemory_Array_GetUsedElems(SSgSoundGroups);
}

// ----------------------------------------------------------------------
UUtError
SSrGroup_New(
	const char					*inGroupName,
	SStGroup					**outGroup)
{
	UUtError					error;
	SStGroup					*group;
	
	UUmAssert(inGroupName);
	UUmAssert(outGroup);
	
	// allocate memory for the group sound
	group = (SStGroup*)UUrMemory_Block_NewClear(sizeof(SStGroup));
	UUmError_ReturnOnNull(group);
	
	// initialize the group
	group->group_volume = 1.0f;
	group->group_pitch = 1.0f;
	group->flags = SScGroupFlag_None;
	group->flag_data = 0;
	group->num_channels = 0;
	UUrString_Copy(group->group_name, inGroupName, SScMaxNameLength);
	group->permutations =
		UUrMemory_Array_New(
			sizeof(SStPermutation),
			1,
			0,
			0);
	if (group->permutations == NULL)
	{
		UUrMemory_Block_Delete(group);
		return UUcError_OutOfMemory;
	}
	
	// insert the group into the sound groups list
	{
		SStGroup				**array;
		UUtUns32				num_elements;
		UUtInt32				min, max, midpoint, compare;
		UUtUns32				found_location;
		
		array = (SStGroup**)UUrMemory_Array_GetMemory(SSgSoundGroups);
		num_elements = UUrMemory_Array_GetUsedElems(SSgSoundGroups);
		
		min = 0; 
		max = num_elements - 1;
		
		while(min <= max)
		{
			midpoint = (min + max) >> 1; // S.S. / 2;
		
			compare = SSiGroup_Compare(&group, &array[midpoint]);
		
			if (compare < 0) {
				// must lie below here
				max = midpoint - 1;
		
			} else {
				// must lie above here
				min = midpoint + 1;
			}
		}
		
		found_location = (UUtUns32)min;
		
		// create space in the array at the desired location
		UUmAssert((found_location >= 0) && (found_location <= num_elements));
		error = UUrMemory_Array_InsertElement(SSgSoundGroups, found_location, NULL);
		UUmError_ReturnOnError(error);
		
		// set the array element
		array = (SStGroup**)UUrMemory_Array_GetMemory(SSgSoundGroups);
		array[found_location] = group;
	}
	
	// set the outgoing data
	if (outGroup) { *outGroup = group; }
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
SSrGroup_Permutation_Delete(
	SStGroup					*ioGroup,
	UUtUns32					inPermIndex)
{
	UUmAssert(ioGroup);
	
	// check the index's range
	if (inPermIndex >= UUrMemory_Array_GetUsedElems(ioGroup->permutations))
	{
		return;
	}
	
	// delete the permutation
	UUrMemory_Array_DeleteElement(ioGroup->permutations, inPermIndex);
}

// ----------------------------------------------------------------------
SStPermutation*
SSrGroup_Permutation_Get(
	SStGroup					*inGroup,
	UUtUns32					inPermIndex)
{
	SStPermutation				*perms;
	
	UUmAssert(inGroup);
	
	// check the index's range
	if (inPermIndex >= UUrMemory_Array_GetUsedElems(inGroup->permutations))
	{
		return NULL;
	}
	
	perms = (SStPermutation*)UUrMemory_Array_GetMemory(inGroup->permutations);
	if (perms == NULL) { return NULL; }
	
	return &perms[inPermIndex];
}

// ----------------------------------------------------------------------
UUtError
SSrGroup_Permutation_New(
	SStGroup					*ioGroup,
	SStSoundData				*inSoundData,
	UUtUns32					*outPermutationIndex)
{
	UUtError					error;
	UUtUns32					index;
	UUtBool						mem_moved;
	SStPermutation				*perms;
	UUtUns32					in_sound_data_num_channels = SSrSound_GetNumChannels(inSoundData);
				
	UUmAssert(ioGroup);
	UUmAssert(inSoundData);
	UUmAssert(outPermutationIndex);
	
	*outPermutationIndex = UUcMaxUns32;
	
	// make sure the number of sound channels of the permutations match
	if (UUrMemory_Array_GetUsedElems(ioGroup->permutations) > 0)
	{
		if (ioGroup->num_channels != in_sound_data_num_channels)
		{
			return UUcError_Generic;
		}
	}
	else
	{
		ioGroup->num_channels = in_sound_data_num_channels;
	}
	
	// allocate a new permutation in the permutations array
	error = UUrMemory_Array_GetNewElement(ioGroup->permutations, &index, &mem_moved);
	UUmError_ReturnOnError(error);
	
	// get a pointer to the permutataions array
	perms = (SStPermutation*)UUrMemory_Array_GetMemory(ioGroup->permutations);
	UUmAssert(perms);
	
	// initialize the permutation
	perms[index].weight					= 10;
	perms[index].min_volume_percent		= 1.0;
	perms[index].max_volume_percent		= 1.0;
	perms[index].min_pitch_percent		= 1.0;
	perms[index].max_pitch_percent		= 1.0;
	perms[index].sound_data				= inSoundData;
	UUrString_Copy(
		perms[index].sound_data_name,
		SSrSoundData_GetName(inSoundData),
		SScMaxNameLength);
	
	*outPermutationIndex = index;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
SSrGroup_Play(
	SStGroup					*inGroup,
	SStSoundChannel				*inSoundChannel,	/* optional */
	const char					*inDebugSoundType,
	const char					*inDebugSoundName)
{
	UUtUns32					num_permutations;
	SStPermutation				*permutation;
	SStSoundChannel				*sound_channel;
	
	UUmAssert(inGroup);
	UUmAssert(inGroup->permutations);
	
	if (SSgUsable == UUcFalse) { return; }

	// get the number of permutations
	num_permutations = UUrMemory_Array_GetUsedElems(inGroup->permutations);
	if (num_permutations == 0) { return; }
	
	// pick the permutation to play
	permutation = SSiGroup_SelectPermutation(inGroup);
	if (permutation == NULL) {
		// an error has occurred finding the permutation
		return;

	} else if (permutation->sound_data == NULL) {
		COrConsole_Printf("SOUND NOT FOUND: %s %s - sound %s", 
				(inDebugSoundType == NULL) ? "" : inDebugSoundType,
				(inDebugSoundName == NULL) ? "" : inDebugSoundName, permutation->sound_data_name);
		return;
	}
	
	SSrWaitGuard(SSgGuardAll);

	// get a sound channel
	if (inSoundChannel)
	{
		sound_channel = inSoundChannel;
	}
	else
	{
		UUtUns32 permutation_num_channels = SSrSound_GetNumChannels(permutation->sound_data);

		if (permutation_num_channels == 1)
		{
			sound_channel =
				SSiSoundChannels_GetChannelForPlay(
					SSgSoundChannels_Mono,
					SSgSoundChannels_NumMono,
					SScPriority2_Normal);
		}
		else if (permutation_num_channels == 2)
		{
			sound_channel =
				SSiSoundChannels_GetChannelForPlay(
					SSgSoundChannels_Stereo,
					SSgSoundChannels_NumStereo,
					SScPriority2_Normal);
		}
		else
		{
			sound_channel = NULL;
		}
	}
	
	if (sound_channel == NULL) {
		SSrReleaseGuard(SSgGuardAll);
		return;
	}

//	SSrWaitGuard(&sound_channel->guard);

	sound_channel->group = inGroup;
	
	// play the permutation
	SSrPermutation_Play(permutation, sound_channel, inDebugSoundType, inDebugSoundName);

//	SSrReleaseGuard(&sound_channel->guard);

	SSrReleaseGuard(SSgGuardAll);
}

// ----------------------------------------------------------------------
static UUtBool
SSiGroup_PlaySpatialized(
	SStGroup					*inGroup,
	const float					inMaxVolDistance,
	const float					inMinVolDistance,
	const M3tPoint3D			*inPosition,
	const M3tVector3D			*inDirection,
	const M3tVector3D			*inVelocity,
	float						inVolume,
	SStPriority2				inPriority,
	const char					*inDebugSoundType,
	const char					*inDebugSoundName)
{
	UUtUns32					num_permutations;
	SStPermutation				*permutation;
	SStSoundChannel				*sound_channel;
	UUtBool						result;
	
	UUmAssert(inGroup);
	UUmAssert(inGroup->permutations);
	
	if (SSgUsable == UUcFalse) { return UUcFalse; }

	// get the number of permutations
	num_permutations = UUrMemory_Array_GetUsedElems(inGroup->permutations);
	if (num_permutations == 0) { return UUcFalse; }
	
	// pick the permutation to play
	permutation = SSiGroup_SelectPermutation(inGroup);
	if (permutation == NULL) {
		// an error has occurred finding the permutation
		return UUcFalse;

	} else if (permutation->sound_data == NULL) {
		COrConsole_Printf("SOUND NOT FOUND: %s %s - group %s - sound %s", 
				(inDebugSoundType == NULL) ? "" : inDebugSoundType,
				(inDebugSoundName == NULL) ? "" : inDebugSoundName,
				inGroup->group_name, permutation->sound_data_name);
		return UUcFalse;
	}

	SSrWaitGuard(SSgGuardAll);

	{
		UUtUns32 permutation_num_channels = SSrSound_GetNumChannels(permutation->sound_data);

		// get a sound channel
		if (permutation_num_channels == 1)
		{
			sound_channel =
				SSiSoundChannels_GetChannelForPlay(
					SSgSoundChannels_Mono,
					SSgSoundChannels_NumMono,
					inPriority);
		}
		else if (permutation_num_channels == 2)
		{
			sound_channel =
				SSiSoundChannels_GetChannelForPlay(
					SSgSoundChannels_Stereo,
					SSgSoundChannels_NumStereo,
					inPriority);
		}
		else
		{
			sound_channel = NULL;
		}
	}
	
	if (sound_channel == NULL)
	{
/*		if (SSgShowDebugInfo)
		{
			COrConsole_Printf("SSrGroup_PlaySpatialized: didn't play %s, no channels available", inGroup->group_name);
		}*/
		
		SSrReleaseGuard(SSgGuardAll);
		return UUcFalse;
	}
	
	// setup the channel
	SSiSoundChannel_SetLooping(sound_channel, UUcFalse);
	SSiSoundChannel_SetChannelVolume(sound_channel, inVolume);
	SSiSoundChannel_SetCanPan(sound_channel, UUcTrue);
	
	// spacialize the sound channel
	result =
		SSiSoundChannel_Spatialize(
			sound_channel,
			inMaxVolDistance,
			inMinVolDistance,
			inPosition,
			inDirection,
			inVelocity);
	
	// play the permutation
	if (result == UUcTrue)
	{
		sound_channel->group = inGroup;
		SSrPermutation_Play(permutation, sound_channel, inDebugSoundType, inDebugSoundName);
	}
	
	SSrReleaseGuard(SSgGuardAll);
	
	return result;
}

// ----------------------------------------------------------------------
void
SSrGroup_UpdateSoundDataPointers(
	void)
{
	SStGroup					**group_array;
	UUtUns32					num_groups;
	UUtUns32					itr;
	UUtBool						search_on_disk;
	
#if SHIPPING_VERSION == 0
	search_on_disk = SSgSearchOnDisk;
#else
	search_on_disk = UUcFalse;
#endif
	
	// go through all of the groups and updates the sound_data pointers of their
	// permutations
	group_array = (SStGroup**)UUrMemory_Array_GetMemory(SSgSoundGroups);
	num_groups = UUrMemory_Array_GetUsedElems(SSgSoundGroups);	
	for (itr = 0; itr < num_groups; itr++)
	{
		SStGroup					*group;
		SStPermutation				*perm_array;
		UUtUns32					num_perms;
		UUtUns32					itr2;
		
		// get a pointer to the sound group
		group = group_array[itr];
		
		// update the sound data pointers for the permutations of the group
		perm_array = (SStPermutation*)UUrMemory_Array_GetMemory(group->permutations);
		num_perms = UUrMemory_Array_GetUsedElems(group->permutations);
		for (itr2 = 0; itr2 < num_perms; itr2++)
		{
			perm_array[itr2].sound_data =
				SSrSoundData_GetByName(
					perm_array[itr2].sound_data_name,
					search_on_disk);
		}
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static int UUcExternal_Call
SSiImpulse_Compare(
	const void					*inItem1,
	const void					*inItem2)
{
	const SStImpulse			*impulse1;
	const SStImpulse			*impulse2;
	
	impulse1 = *((SStImpulse**)inItem1);
	impulse2 = *((SStImpulse**)inItem2);
		
	return strcmp(impulse1->impulse_name, impulse2->impulse_name);
}

// ----------------------------------------------------------------------
UUtError
SSrImpulse_Copy(
	const SStImpulse			*inSource,
	SStImpulse					*ioDest)
{
	UUmAssert(inSource);
	UUmAssert(ioDest);
	
	// restore the id after copying the data
	ioDest->impulse_group = inSource->impulse_group;
	UUrString_Copy(ioDest->impulse_group_name, inSource->impulse_group_name, SScMaxNameLength);
	ioDest->priority = inSource->priority;
	ioDest->max_volume_distance = inSource->max_volume_distance;
	ioDest->min_volume_distance = inSource->min_volume_distance;
	ioDest->max_volume_angle = inSource->max_volume_angle;
	ioDest->min_volume_angle = inSource->min_volume_angle;
	ioDest->min_angle_attenuation = inSource->min_angle_attenuation;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
SSrImpulse_Delete(
	const char					*inImpulseName,
	UUtBool						inUpdatePointers)
{
	UUtUns32					num_impulse_sounds;
	SStImpulse					**impulse_array, *deleted_element = NULL;
	UUtUns32					i;
	
	num_impulse_sounds = UUrMemory_Array_GetUsedElems(SSgImpulseSounds);
	impulse_array = (SStImpulse**)UUrMemory_Array_GetMemory(SSgImpulseSounds);
	
	for (i = 0; i < num_impulse_sounds; i++)
	{
		if (UUrString_Compare_NoCase(impulse_array[i]->impulse_name, inImpulseName) == 0)
		{
			deleted_element = impulse_array[i];
			UUrMemory_Block_Delete(deleted_element);
			UUrMemory_Array_DeleteElement(SSgImpulseSounds, i);
			break;
		}
	}

	if (inUpdatePointers && (deleted_element != NULL))
	{
		SSgImpulseHandler(deleted_element, UUcTrue);
	}
}

// ----------------------------------------------------------------------
SStImpulse*
SSrImpulse_GetByIndex(
	UUtUns32					inIndex)
{
	UUtUns32					num_impulse_sounds;
	SStImpulse					**impulse_array;
	
	num_impulse_sounds = UUrMemory_Array_GetUsedElems(SSgImpulseSounds);
	impulse_array = (SStImpulse**)UUrMemory_Array_GetMemory(SSgImpulseSounds);
	
	UUmAssert(num_impulse_sounds > inIndex);
	
	return impulse_array[inIndex];
}

// ----------------------------------------------------------------------
SStImpulse*
SSrImpulse_GetByName(
	const char					*inImpulseName)
{
	SStImpulse					**impulse_array;
	SStImpulse					find_me;
	SStImpulse					*find_me_ptr = &find_me;
	SStImpulse					**found_impulse;
	UUtUns32					num_elements;
	
	if (inImpulseName == NULL) { return NULL; }
	
	UUrString_Copy(find_me.impulse_name, inImpulseName, SScMaxNameLength);
	
	// get a pointer to the array
	impulse_array = (SStImpulse**)UUrMemory_Array_GetMemory(SSgImpulseSounds);
	if (impulse_array == NULL) { return NULL; }
	
	num_elements = UUrMemory_Array_GetUsedElems(SSgImpulseSounds);
	
	found_impulse =
		(SStImpulse**)bsearch(
			&find_me_ptr,
			impulse_array,
			num_elements,
			sizeof(SStImpulse*),
			SSiImpulse_Compare);
	if (found_impulse == NULL) { return NULL; }

	return *found_impulse;
}

// ----------------------------------------------------------------------
UUtUns32
SSrImpulse_GetNumImpulseSounds(
	void)
{
	UUmAssert(SSgImpulseSounds);
	return UUrMemory_Array_GetUsedElems(SSgImpulseSounds);
}

// ----------------------------------------------------------------------
UUtError
SSrImpulse_New(
	const char					*inImpulseName,
	SStImpulse					**outImpulse)
{
	UUtError					error;
	SStImpulse					*impulse;
	
	UUmAssert(inImpulseName);
	UUmAssert(outImpulse);
	
	// allocate memory for the impulse sound
	impulse = (SStImpulse*)UUrMemory_Block_New(sizeof(SStImpulse));
	UUmError_ReturnOnNull(impulse);
	
	// initialize the impulse sound
	UUrString_Copy(impulse->impulse_name, inImpulseName, SScMaxNameLength);
	impulse->impulse_group = NULL;
	impulse->impulse_group_name[0] = '\0';
	impulse->priority = SScPriority2_Normal;
	impulse->max_volume_distance = 1.0f;
	impulse->min_volume_distance = 10.0f;
	impulse->max_volume_angle = 360.0;
	impulse->min_volume_angle = 360.0;
	impulse->min_angle_attenuation = 1.0;
	impulse->alt_threshold = 0;
	impulse->alt_impulse = NULL;
	UUrMemory_Clear(impulse->alt_impulse_name, SScMaxNameLength);
	impulse->impact_velocity = 0.0f;
	impulse->min_occlusion = 0.0f;
		
	// insert the impulse sound into the impulse sound list
	{
		SStImpulse				**array;
		UUtUns32				num_elements;
		UUtInt32				min, max, midpoint, compare;
		UUtUns32				found_location;
		
		array = (SStImpulse**)UUrMemory_Array_GetMemory(SSgImpulseSounds);
		num_elements = UUrMemory_Array_GetUsedElems(SSgImpulseSounds);
		
		min = 0; 
		max = num_elements - 1;
		
		while(min <= max)
		{
			midpoint = (min + max) >> 1; // S.S. / 2;
		
			compare = SSiImpulse_Compare(&impulse, &array[midpoint]);
		
			if (compare < 0) {
				// must lie below here
				max = midpoint - 1;
		
			} else {
				// must lie above here
				min = midpoint + 1;
			}
		}
		
		found_location = (UUtUns32)min;
		
		// create space in the array at the desired location
		UUmAssert((found_location >= 0) && (found_location <= num_elements));
		error = UUrMemory_Array_InsertElement(SSgImpulseSounds, found_location, NULL);
		UUmError_ReturnOnError(error);
		
		// set the array element
		array = (SStImpulse**)UUrMemory_Array_GetMemory(SSgImpulseSounds);
		array[found_location] = impulse;
	}

	// set the outgoing data
	if (outImpulse) { *outImpulse = impulse; }
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
SSrImpulse_Play(
	SStImpulse					*inImpulse,
	const M3tPoint3D			*inPosition,
	const M3tVector3D			*inDirection,
	const M3tVector3D			*inVelocity,
	const float					*inVolume)
{
	float						volume;
	UUtBool						result;
	
	UUmAssert(inImpulse);
	
	if (SSgUsable == UUcFalse) { return; }

	// find the impulse sound group if it isn't already set
	if (inImpulse->impulse_group == NULL) { return; }
	
	if (inVolume == NULL)
	{
		volume = 1.0f;
	}
	else
	{
		volume = *inVolume;
	}
	
	if (inImpulse->alt_threshold > 0)
	{
		SStSoundChannel				*channel_array;
		UUtUns32					num_channels;
		UUtUns32					i;
		UUtUns32					num_playing;
		
		// scan the appropriate sound channels to determine if
		// this group has being played the maximum number of times it
		// is allowed to be played
		if (inImpulse->impulse_group->num_channels == 2)
		{
			channel_array = SSgSoundChannels_Stereo;
			num_channels = SSgSoundChannels_NumStereo;
		}
		else
		{
			channel_array = SSgSoundChannels_Mono;
			num_channels = SSgSoundChannels_NumMono;
		}
		
		SSrWaitGuard(SSgGuardAll);

		num_playing = 0;
		for (i = 0; i < num_channels; i++)
		{
			if (SSiSoundChannel_IsPlaying(&channel_array[i]) == UUcFalse) { continue; }
			if (channel_array[i].group != inImpulse->impulse_group) { continue; }
			
			num_playing++;
			if (num_playing >= inImpulse->alt_threshold) { break; }
		}
		
		SSrReleaseGuard(SSgGuardAll);

		if (num_playing >= inImpulse->alt_threshold)
		{
			if (inImpulse->alt_impulse != NULL)
			{
				SSrImpulse_Play(
					inImpulse->alt_impulse,
					inPosition,
					inDirection,
					inVelocity,
					inVolume);
			}
			return;
		}
	}
	
	result =
		SSiGroup_PlaySpatialized(
			inImpulse->impulse_group,
			inImpulse->max_volume_distance,
			inImpulse->min_volume_distance,
			inPosition,
			inDirection,
			inVelocity,
			volume,
			inImpulse->priority,
			"impulse",
			inImpulse->impulse_name);
/*	if (SSgShowDebugInfo == UUcTrue)
	{
		if ((result == UUcFalse) && (inImpulse->priority >= SScPriority2_High))
		{
			COrConsole_Printf("SSrImpulse_Play: unable to play %s", inImpulse->impulse_name);
		}
	}*/
}

// ----------------------------------------------------------------------
void
SSrImpulse_Play_Simple(
	const char					*inImpulseName,
	const M3tPoint3D			*inPosition)
{
	SStImpulse *impulse_sound = SSrImpulse_GetByName(inImpulseName);

	if (NULL == impulse_sound) {
		COrConsole_Printf("failed to find impulse sound %s", inImpulseName);
	}
	else {
		SSrImpulse_Play(impulse_sound, inPosition, NULL, NULL, NULL);
	}

	return;
}

// ----------------------------------------------------------------------
void
SSrImpulse_UpdateGroupName(
	const char					*inOldGroupName,
	const char					*inNewGroupName)
{
	UUtUns32					num_impulse_sounds;
	SStImpulse					**impulse_array;
	UUtUns32					i;
	
	num_impulse_sounds = UUrMemory_Array_GetUsedElems(SSgImpulseSounds);
	impulse_array = (SStImpulse**)UUrMemory_Array_GetMemory(SSgImpulseSounds);
	
	// get a new pointer to each SStGroup
	for (i = 0; i < num_impulse_sounds; i++)
	{
		SStImpulse				*impulse;
		
		impulse = impulse_array[i];
		
		// update the impulse sound group
		if (UUrString_Compare_NoCase(impulse->impulse_group_name, inOldGroupName) == 0)
		{
			if (inNewGroupName == NULL)
			{
				UUrMemory_Clear(impulse->impulse_group_name, SScMaxNameLength);
				impulse->impulse_group = NULL;
			}
			else
			{
				UUrString_Copy(impulse->impulse_group_name, inNewGroupName, SScMaxNameLength);
			}
		}
	}
}

// ----------------------------------------------------------------------
void
SSrImpulse_UpdateGroupPointers(
	void)
{
	UUtUns32					num_impulse_sounds;
	SStImpulse					**impulse_array;
	UUtUns32					i;
	
	num_impulse_sounds = UUrMemory_Array_GetUsedElems(SSgImpulseSounds);
	impulse_array = (SStImpulse**)UUrMemory_Array_GetMemory(SSgImpulseSounds);
	
	// get a new pointer to each SStGroup
	for (i = 0; i < num_impulse_sounds; i++)
	{
		SStImpulse				*impulse;
		
		impulse = impulse_array[i];
		
		// get a pointer to the impulse group
		impulse->impulse_group = SSrGroup_GetByName(impulse->impulse_group_name);
		
		// update the alt_impulse
		if (impulse->alt_threshold > 0)
		{
			impulse->alt_impulse = SSrImpulse_GetByName(impulse->alt_impulse_name);
		}
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
SSiPAUpdate_CalcNextDetailTime(
	SStPlayingAmbient		*inPlayingAmbient)
{
	float					detail_time;
	UUtUns32				time;
	
	if (inPlayingAmbient->ambient->detail == NULL) { return; }
	
	// get the amount of time to wait
	detail_time = 
		UUmRandomRangeFloat(
			inPlayingAmbient->ambient->min_detail_time,
			inPlayingAmbient->ambient->max_detail_time);
	
	// calculate the next detail time
	time = UUrMachineTime_Sixtieths();
	inPlayingAmbient->detail_time = time + (UUtUns32)(detail_time * 60.0f);
}

// ----------------------------------------------------------------------
static SStSoundChannel*
SSiPAUpdate_ReserveChannel(
	UUtUns32				inNumChannels,
	SStPriority2			inPriority,
	float					inVolume,
	UUtBool					inCanPan)
{
	SStSoundChannel			*channel;
	
	// get a channel
	channel =
		SSiSoundChannels_GetChannelForPlay(
			((inNumChannels == 2) ? SSgSoundChannels_Stereo : SSgSoundChannels_Mono),
			((inNumChannels == 2) ? SSgSoundChannels_NumStereo : SSgSoundChannels_NumMono),
			inPriority);
	
	if (channel != NULL)
	{
		// set the status flags
		SSiSoundChannel_SetAmbient(channel, UUcTrue);
		SSiSoundChannel_SetLock(channel, UUcTrue);
		SSiSoundChannel_SetLooping(channel, UUcFalse);
		SSiSoundChannel_SetUpdating(channel, UUcFalse);
		SSiSoundChannel_SetCanPan(channel, inCanPan);
		
		// initialize the volumes, pan, and pitch.
		// the channel volume must be set first in order to keep the distance volume
		// from making the channel audible before it should be
		SSiSoundChannel_SetChannelVolume(channel, inVolume);
		SSiSoundChannel_SetDistanceVolume(channel, 1.0f);
		SSiSoundChannel_SetPitch(channel, 1.0f);
		SSiSoundChannel_SetPan(channel, SScPanFlag_None, 1.0f);
	}
	
	return channel;
}

// ----------------------------------------------------------------------
static UUtBool
SSiPAUpdate_Start(
	SStPlayingAmbient			*inPlayingAmbient)
{
	SStGroup					*group;
	UUtUns32					num_channels;
	UUtBool						pan;
	
	// ------------------------------
	// reserve channel1
	// ------------------------------
	group = inPlayingAmbient->ambient->base_track1;
	if (group == NULL) { group = inPlayingAmbient->ambient->in_sound; }
	if (group == NULL) { group = inPlayingAmbient->ambient->out_sound; }
	
	pan = ((inPlayingAmbient->ambient->flags & SScAmbientFlag_Pan) != 0); 
	
	if (group != NULL)
	{
		num_channels = SSrGroup_GetNumChannels(group);
		inPlayingAmbient->channel1 =
			SSiPAUpdate_ReserveChannel(
				num_channels,
				inPlayingAmbient->ambient->priority,
				inPlayingAmbient->channel_volume,
				pan);
		
		// get a channel
/*		inPlayingAmbient->channel1 =
			SSiSoundChannels_GetChannelForPlay(
				((num_channels == 2) ? SSgSoundChannels_Stereo : SSgSoundChannels_Mono),
				((num_channels == 2) ? SSgSoundChannels_NumStereo : SSgSoundChannels_NumMono),
				inPlayingAmbient->ambient->priority);
		
		if (inPlayingAmbient->channel1 != NULL)
		{
			// set the status flags
			SSiSoundChannel_SetAmbient(inPlayingAmbient->channel1, UUcTrue);
			SSiSoundChannel_SetLock(inPlayingAmbient->channel1, UUcTrue);
			SSiSoundChannel_SetLooping(inPlayingAmbient->channel1, UUcFalse);
			SSiSoundChannel_SetUpdating(inPlayingAmbient->channel1, UUcFalse);
			SSiSoundChannel_SetCanPan(inPlayingAmbient->channel1, pan);
			
			// initialize the volumes, pan, and pitch.
			// the channel volume must be set first in order to keep the distance volume
			// from making the channel audible before it should be
			SSiSoundChannel_SetChannelVolume(
				inPlayingAmbient->channel1,
				inPlayingAmbient->channel_volume);
			SSiSoundChannel_SetDistanceVolume(inPlayingAmbient->channel1, 1.0f);
			SSiSoundChannel_SetPitch(inPlayingAmbient->channel1, 1.0f);
			SSiSoundChannel_SetPan(inPlayingAmbient->channel1, SScPanFlag_None, 1.0f);
		}*/
	}
	
	// ------------------------------
	// reserve channel2
	// ------------------------------
	group = inPlayingAmbient->ambient->base_track2;
	
	if (group != NULL)
	{
		num_channels = SSrGroup_GetNumChannels(group);
		inPlayingAmbient->channel2 =
			SSiPAUpdate_ReserveChannel(
				num_channels,
				inPlayingAmbient->ambient->priority,
				inPlayingAmbient->channel_volume,
				pan);
		
		// get a channel
/*		inPlayingAmbient->channel2 =
			SSiSoundChannels_GetChannelForPlay(
				((num_channels == 2) ? SSgSoundChannels_Stereo : SSgSoundChannels_Mono),
				((num_channels == 2) ? SSgSoundChannels_NumStereo : SSgSoundChannels_NumMono),
				inPlayingAmbient->ambient->priority);
		
		if (inPlayingAmbient->channel2 != NULL)
		{
			// set the status flags
			SSiSoundChannel_SetLock(inPlayingAmbient->channel2, UUcTrue);
			SSiSoundChannel_SetLooping(inPlayingAmbient->channel2, UUcFalse);
			SSiSoundChannel_SetAmbient(inPlayingAmbient->channel2, UUcTrue);
			SSiSoundChannel_SetUpdating(inPlayingAmbient->channel2, UUcFalse);
			SSiSoundChannel_SetCanPan(inPlayingAmbient->channel2, pan);
			
			// initialize the volumes, pan, and pitch
			// the channel volume must be set first in order to keep the distance volume
			// from making the channel audible before it should be
			SSiSoundChannel_SetChannelVolume(
				inPlayingAmbient->channel2,
				inPlayingAmbient->channel_volume);
			SSiSoundChannel_SetDistanceVolume(inPlayingAmbient->channel2, 1.0f);
			SSiSoundChannel_SetPitch(inPlayingAmbient->channel2, 1.0f);
			SSiSoundChannel_SetPan(inPlayingAmbient->channel2, SScPanFlag_None, 1.0f);
		}*/
	}
	
	// ------------------------------
	// move on to the next stage
	// ------------------------------
	if ((inPlayingAmbient->channel1 == NULL) && (inPlayingAmbient->channel2 == NULL))
	{
		if (inPlayingAmbient->ambient->detail != NULL)
		{
			inPlayingAmbient->stage = SScPAStage_DetailOnly;
		}
		else
		{
			inPlayingAmbient->stage = SScPAStage_Done;
		}
	}
	else
	{
		inPlayingAmbient->stage = SScPAStage_InSoundStart;
	}
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
static UUtBool
SSiPAUpdate_InSoundStart(
	SStPlayingAmbient			*inPlayingAmbient)
{
	// go to stage in_sound_playing
	inPlayingAmbient->stage = SScPAStage_InSoundPlaying;

	// play the in sound if there is one
	if ((inPlayingAmbient->ambient->in_sound) && (inPlayingAmbient->channel1))
	{
		// play the in_sound group
		SSrGroup_Play(inPlayingAmbient->ambient->in_sound, inPlayingAmbient->channel1,
					"ambient in", inPlayingAmbient->ambient->ambient_name);
	}
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
static UUtBool
SSiPAUpdate_InSoundPlaying(
	SStPlayingAmbient			*inPlayingAmbient)
{
	UUtBool						next_stage;
	
	next_stage = UUcFalse;
	
	// if channel1 is NULL then the in_sound is not playing so
	// move on to the Body_Start stage.  Otherwise, wait for the in_sound
	// to stop playing
	if (inPlayingAmbient->channel1 == NULL)
	{
		inPlayingAmbient->stage = SScPAStage_BodyStart;
		next_stage = UUcTrue;
	}
	else if ((SSiSoundChannel_IsPlaying(inPlayingAmbient->channel1) == UUcFalse) ||
			(SSiSoundChannel_IsUpdating(inPlayingAmbient->channel1) == UUcTrue))
	{
		inPlayingAmbient->stage = SScPAStage_BodyStart;
		next_stage = UUcTrue;
	}
	
	return next_stage;
}

// ----------------------------------------------------------------------
static UUtBool
SSiPAUpdate_DetailOnly(
	SStPlayingAmbient			*inPlayingAmbient)
{
	// update the detail_track playing
	if ((inPlayingAmbient->ambient->detail != NULL) &&
		(inPlayingAmbient->detail_time < UUrMachineTime_Sixtieths()))
	{
		M3tPoint3D			position;
		float				x;
		float				z;
		
		// get the position of the sound
		if (inPlayingAmbient->has_position)
		{
			position = inPlayingAmbient->position;
		}
		else
		{
			position = SSgListener_Position;
		}
		
		// calculate a random offset from the current position
		x =	UUmRandomRangeFloat(
				-inPlayingAmbient->ambient->sphere_radius,
				inPlayingAmbient->ambient->sphere_radius);
		z = UUmRandomRangeFloat(
				-inPlayingAmbient->ambient->sphere_radius,
				inPlayingAmbient->ambient->sphere_radius);
		
		position.x += x;
		position.z += z;
		
		// play the detail
		SSiGroup_PlaySpatialized(
			inPlayingAmbient->ambient->detail,
			inPlayingAmbient->ambient->max_volume_distance,
			inPlayingAmbient->ambient->min_volume_distance,
			&position,
			NULL,
			NULL,
			inPlayingAmbient->channel_volume,
			SScPriority2_Low,
			"ambient detail",
			inPlayingAmbient->ambient->ambient_name);
		
		// calculate the next time to play the detail
		SSiPAUpdate_CalcNextDetailTime(inPlayingAmbient);
	}

	return UUcFalse;
}

// ----------------------------------------------------------------------
static UUtBool
SSiPAUpdate_BodyStart(
	SStPlayingAmbient			*inPlayingAmbient)
{
	UUtBool						next_stage;
	
	next_stage = UUcFalse;
	
	if ((SSiSoundChannel_IsUpdating(inPlayingAmbient->channel1) == UUcFalse) &&
		(SSiSoundChannel_IsUpdating(inPlayingAmbient->channel2) == UUcFalse))
	{
		// if the ambient sound has a base_track1, then play it
		if ((inPlayingAmbient->ambient->base_track1 != NULL) &&
			(inPlayingAmbient->channel1))
		{
			// set the looping state of the channel
			SSiSoundChannel_SetLooping(
				inPlayingAmbient->channel1,
				((inPlayingAmbient->ambient->flags & SScAmbientFlag_PlayOnce) == 0));
			
			// play the base_track1
			SSrGroup_Play(inPlayingAmbient->ambient->base_track1, inPlayingAmbient->channel1,
						"ambient base1", inPlayingAmbient->ambient->ambient_name);
		}
	
		// if the ambient sound has a base_track2, then play it
		if ((inPlayingAmbient->ambient->base_track2 != NULL) &&
			(inPlayingAmbient->channel2))
		{
			// set the looping state of the channel
			SSiSoundChannel_SetLooping(
				inPlayingAmbient->channel2,
				((inPlayingAmbient->ambient->flags & SScAmbientFlag_PlayOnce) == 0));
			
			// play the base_track2
			SSrGroup_Play(inPlayingAmbient->ambient->base_track2, inPlayingAmbient->channel2,
						"ambient base2", inPlayingAmbient->ambient->ambient_name);
		}
		
		// set the time for the detail track if there is one
		SSiPAUpdate_CalcNextDetailTime(inPlayingAmbient);
		
		// move on to the next appropriate stage
		if ((inPlayingAmbient->channel1 != NULL) || (inPlayingAmbient->channel2 != NULL))
		{
			inPlayingAmbient->stage = SScPAStage_BodyPlaying;
			next_stage = UUcTrue;
		}
		else
		{
			inPlayingAmbient->stage = SScPAStage_BodyStopping;
			next_stage = UUcTrue;
		}
	}
	
	return next_stage;
}

// ----------------------------------------------------------------------
static UUtBool
SSiPAUpdate_BodyPlaying(
	SStPlayingAmbient			*inPlayingAmbient)
{
	UUtBool						channel1_playing;
	UUtBool						channel2_playing;
	UUtUns32					num_channels;
	UUtBool						can_pan;
	
	// determine if the sound channels are playing
	channel1_playing = SSiSoundChannel_IsPlaying(inPlayingAmbient->channel1);
	channel2_playing = SSiSoundChannel_IsPlaying(inPlayingAmbient->channel2);
	
	// recover from losing channels
	if ((inPlayingAmbient->ambient->base_track1 != NULL) &&
		(inPlayingAmbient->channel1 == NULL))
	{
		can_pan = ((inPlayingAmbient->ambient->flags & SScAmbientFlag_Pan) != 0); 
		num_channels = SSrGroup_GetNumChannels(inPlayingAmbient->ambient->base_track1);
		inPlayingAmbient->channel1 =
			SSiPAUpdate_ReserveChannel(
				num_channels,
				inPlayingAmbient->ambient->priority,
				0.0f,
				can_pan);
		SSrAmbient_SetVolume(inPlayingAmbient->id, inPlayingAmbient->channel_volume, 0.5f);
	}
	
	if ((inPlayingAmbient->ambient->base_track2 != NULL) &&
		(inPlayingAmbient->channel2 == NULL))
	{
		can_pan = ((inPlayingAmbient->ambient->flags & SScAmbientFlag_Pan) != 0); 
		num_channels = SSrGroup_GetNumChannels(inPlayingAmbient->ambient->base_track2);
		inPlayingAmbient->channel2 =
			SSiPAUpdate_ReserveChannel(
				num_channels,
				inPlayingAmbient->ambient->priority,
				0.0f,
				can_pan);
		SSrAmbient_SetVolume(inPlayingAmbient->id, inPlayingAmbient->channel_volume, 0.5f);
	}
	
	// update base_track1 playing
	if ((inPlayingAmbient->ambient->base_track1 != NULL) &&
		(inPlayingAmbient->channel1 != NULL) &&
		(channel1_playing == UUcFalse) &&
		((inPlayingAmbient->ambient->flags & SScAmbientFlag_PlayOnce) == 0))
	{
		// play the base_track1
		SSrGroup_Play(inPlayingAmbient->ambient->base_track1, inPlayingAmbient->channel1,
						"ambient base1", inPlayingAmbient->ambient->ambient_name);
	}
	
	// update base_track2 playing
	if ((inPlayingAmbient->ambient->base_track2 != NULL) &&
		(inPlayingAmbient->channel2 != NULL) &&
		(channel2_playing == UUcFalse) &&
		((inPlayingAmbient->ambient->flags & SScAmbientFlag_PlayOnce) == 0))
	{
		if (SSiSoundChannel_IsLooping(inPlayingAmbient->channel2) == UUcTrue)
		{
			// play the base_track2
			SSrGroup_Play(inPlayingAmbient->ambient->base_track2, inPlayingAmbient->channel2,
						"ambient base2", inPlayingAmbient->ambient->ambient_name);
		}
		else
		{
			if (SSiSoundChannel_IsLooping(inPlayingAmbient->channel1) == UUcTrue)
			{
				SSiSoundChannel_SetLooping(inPlayingAmbient->channel1, UUcFalse);
				
				// move on to body stopping
				inPlayingAmbient->stage = SScPAStage_BodyStopping;
				return UUcTrue;
			}
		}
	}
	
	// if both channels have stopped playing move on to the body stopping stage
	if ((channel1_playing == UUcFalse) && (channel2_playing == UUcFalse))
	{
		// move on to body stopping
		inPlayingAmbient->stage = SScPAStage_BodyStopping;
		return UUcTrue;
	}
	else
	if (((inPlayingAmbient->ambient->flags & SScAmbientFlag_PlayOnce) != 0) &&
		(SSiSoundChannel_IsUpdating(inPlayingAmbient->channel1) == UUcTrue))
	{
		// move on to body stopping
		inPlayingAmbient->stage = SScPAStage_BodyStopping;
		return UUcTrue;
	}
	
	// update the detail_track playing
	if ((inPlayingAmbient->ambient->detail != NULL) &&
		(inPlayingAmbient->detail_time < UUrMachineTime_Sixtieths()) &&
		(SSgDetails_CanPlay == UUcTrue))
	{
		M3tPoint3D			position;
		float				x;
		float				z;
		UUtBool				result;
		
		// get the position of the sound
		if (inPlayingAmbient->has_position)
		{
			position = inPlayingAmbient->position;
		}
		else
		{
			position = SSgListener_Position;
		}
		
		// calculate a random offset from the current position
		x =	UUmRandomRangeFloat(
				-inPlayingAmbient->ambient->sphere_radius,
				inPlayingAmbient->ambient->sphere_radius);
		z = UUmRandomRangeFloat(
				-inPlayingAmbient->ambient->sphere_radius,
				inPlayingAmbient->ambient->sphere_radius);
		
		position.x += x;
		position.z += z;
		
		// play the detail
		result = 
			SSiGroup_PlaySpatialized(
				inPlayingAmbient->ambient->detail,
				inPlayingAmbient->ambient->max_volume_distance,
				inPlayingAmbient->ambient->min_volume_distance,
				&position,
				NULL,
				NULL,
				inPlayingAmbient->channel_volume,
				SScPriority2_Low,
				"ambient detail",
				inPlayingAmbient->ambient->ambient_name);
/*		if ((SSgShowDebugInfo == UUcTrue) && (result == UUcFalse))
		{
			COrConsole_Printf(
				"SSiPAUpdate_BodyPlaying: unable to play %s",
				inPlayingAmbient->ambient->ambient_name);
		}*/
		
		// calculate the next time to play the detail
		SSiPAUpdate_CalcNextDetailTime(inPlayingAmbient);
	}
	
	return UUcFalse;
}

// ----------------------------------------------------------------------
static UUtBool
SSiPAUpdate_BodyStopping(
	SStPlayingAmbient			*inPlayingAmbient)
{
	UUtBool						next_stage;
	UUtBool						channel1_playing;
	UUtBool						channel2_playing;
	
	next_stage = UUcFalse;
	
	// make sure the channels aren't looping
	if (SSiSoundChannel_IsLooping(inPlayingAmbient->channel1))
	{
		SSiSoundChannel_SetLooping(inPlayingAmbient->channel1, UUcFalse);
	}
	
	if (SSiSoundChannel_IsLooping(inPlayingAmbient->channel2))
	{
		SSiSoundChannel_SetLooping(inPlayingAmbient->channel2, UUcFalse);
	}
	
	// determine if the sound channels are playing
	channel1_playing = SSiSoundChannel_IsPlaying(inPlayingAmbient->channel1);
	channel2_playing = SSiSoundChannel_IsPlaying(inPlayingAmbient->channel2);
	
	// when both channels have stopped playing move on to playing the out sound
	if ((channel1_playing == UUcFalse) && (channel2_playing == UUcFalse))
	{
		inPlayingAmbient->stage = SScPAStage_OutSoundStart;
		next_stage = UUcTrue;
	}
	else
	if ((channel2_playing == UUcFalse) &&
		(SSiSoundChannel_IsUpdating(inPlayingAmbient->channel1) == UUcTrue))
	{
		inPlayingAmbient->stage = SScPAStage_OutSoundStart;
		next_stage = UUcTrue;
	}
	else
	if ((channel1_playing == UUcFalse) &&
		(SSiSoundChannel_IsUpdating(inPlayingAmbient->channel2) == UUcTrue))
	{
		inPlayingAmbient->stage = SScPAStage_OutSoundStart;
		next_stage = UUcTrue;
	}
	
	return next_stage;
}

// ----------------------------------------------------------------------
static UUtBool
SSiPAUpdate_OutSoundStart(
	SStPlayingAmbient			*inPlayingAmbient)
{
	// wait for the out sound to finish in the out sound playing stage
	inPlayingAmbient->stage = SScPAStage_OutSoundPlaying;
	
	// start the out sound
	if ((inPlayingAmbient->ambient->out_sound != NULL) &&
		(inPlayingAmbient->channel1 != NULL))
	{
		// make sure the channel isn't going to loop
		SSiSoundChannel_SetLooping(inPlayingAmbient->channel1, UUcFalse);
		
		// play the out sound
		SSrGroup_Play(inPlayingAmbient->ambient->out_sound, inPlayingAmbient->channel1,
						"ambient out", inPlayingAmbient->ambient->ambient_name);
	}
	
	// release channel2
	if (inPlayingAmbient->channel2)
	{
		SSiSoundChannel_SetAmbient(inPlayingAmbient->channel2, UUcFalse);
		SSiSoundChannel_Stop(inPlayingAmbient->channel2);
		SSiSoundChannel_SetLock(inPlayingAmbient->channel2, UUcFalse);
		inPlayingAmbient->channel2 = NULL;
	}
	
	return UUcFalse;
}

// ----------------------------------------------------------------------
static UUtBool
SSiPAUpdate_OutSoundPlaying(
	SStPlayingAmbient			*inPlayingAmbient)
{
	// move on to the final stage if the out sound has finished playing
	if ((SSiSoundChannel_IsPlaying(inPlayingAmbient->channel1) == UUcFalse) &&
		(SSiSoundChannel_IsUpdating(inPlayingAmbient->channel1) == UUcFalse))
	{
		inPlayingAmbient->stage = SScPAStage_Done;
		return UUcTrue;
	}
	
	return UUcFalse;
}

// ----------------------------------------------------------------------
static UUtBool
SSiPAUpdate_NonAudible(
	SStPlayingAmbient			*inPlayingAmbient)
{
	SSiPlayingAmbient_Halt(inPlayingAmbient);
	inPlayingAmbient->stage = SScPAStage_Done;
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
static UUtBool
SSiPAUpdate_Done(
	SStPlayingAmbient			*inPlayingAmbient)
{
	// release channel1
	if (inPlayingAmbient->channel1 != NULL)
	{
		SSiSoundChannel_SetAmbient(inPlayingAmbient->channel1, UUcFalse);
		SSiSoundChannel_Stop(inPlayingAmbient->channel1);
		SSiSoundChannel_SetLock(inPlayingAmbient->channel1, UUcFalse);
		inPlayingAmbient->channel1 = NULL;
	}

	// release channel2
	if (inPlayingAmbient->channel2 != NULL)
	{
		SSiSoundChannel_SetAmbient(inPlayingAmbient->channel2, UUcFalse);
		SSiSoundChannel_Stop(inPlayingAmbient->channel2);
		SSiSoundChannel_SetLock(inPlayingAmbient->channel2, UUcFalse);
		inPlayingAmbient->channel2 = NULL;
	}
	
	inPlayingAmbient->stage = SScPAStage_None;
	return UUcTrue;
}

// ----------------------------------------------------------------------
static void
SSiPAUpdate_Position(
	SStPlayingAmbient			*inPlayingAmbient)
{
	if (inPlayingAmbient->has_position == UUcFalse) { return; }
	
	// update the position of the ambient sound
	if (inPlayingAmbient->channel1 != NULL)
	{
		SSiSoundChannel_Spatialize(
			inPlayingAmbient->channel1,
			inPlayingAmbient->max_volume_distance,
			inPlayingAmbient->min_volume_distance,
			&inPlayingAmbient->position,
			&inPlayingAmbient->direction,
			&inPlayingAmbient->velocity);
	}
	
	if (inPlayingAmbient->channel2 != NULL)
	{
		SSiSoundChannel_Spatialize(
			inPlayingAmbient->channel2,
			inPlayingAmbient->max_volume_distance,
			inPlayingAmbient->min_volume_distance,
			&inPlayingAmbient->position,
			&inPlayingAmbient->direction,
			&inPlayingAmbient->velocity);
	}
}

// ----------------------------------------------------------------------
static void
SSiPAUpdate_Volume(
	SStPlayingAmbient			*inPlayingAmbient)
{
	UUtUns32					time;
	UUtInt32					delta;
	
	if ((inPlayingAmbient->volume_adjust_final_volume == inPlayingAmbient->channel_volume) ||
		(UUmFabs(inPlayingAmbient->volume_adjust_delta) < SScVolumeAdjustEpsilon))
	{
		return;
	}
	
	time = UUrMachineTime_Sixtieths();
	delta = time - inPlayingAmbient->volume_adjust_start_time;
	inPlayingAmbient->volume_adjust_start_time = time;
		
	inPlayingAmbient->channel_volume += (inPlayingAmbient->volume_adjust_delta * (float)delta);

	if (inPlayingAmbient->volume_adjust_delta > 0.0f)
	{
		inPlayingAmbient->channel_volume =
			UUmPin(inPlayingAmbient->channel_volume, 0.0f, inPlayingAmbient->volume_adjust_final_volume);
	}
	else
	{
		inPlayingAmbient->channel_volume =
			UUmPin(inPlayingAmbient->channel_volume, inPlayingAmbient->volume_adjust_final_volume, 1.0f);
	}
	
	if (inPlayingAmbient->channel1 != NULL)
	{
		SSiSoundChannel_SetChannelVolume(
			inPlayingAmbient->channel1,
			inPlayingAmbient->channel_volume);
	}
	
	if (inPlayingAmbient->channel2 != NULL)
	{
		SSiSoundChannel_SetChannelVolume(
			inPlayingAmbient->channel2,
			inPlayingAmbient->channel_volume);
	}
}

// ----------------------------------------------------------------------
static void
SSiPAUpdate_StopNonAudible(
	SStPlayingAmbient			*inPlayingAmbient)
{
	float						volume;
	float						distance_to_listener;
	float						previous_distance_to_listener;
	
	// if the ambient sound isn't spatialized, don't stop the channel
	if (inPlayingAmbient->has_position == UUcFalse) { return; }
	if (inPlayingAmbient->stage <= SScPAStage_InSoundStart) { return; }
	
	// get the distance to listener
	if (inPlayingAmbient->channel1 != NULL)
	{
		volume = SSiSoundChannel_GetVolume(inPlayingAmbient->channel1);
		distance_to_listener = inPlayingAmbient->channel1->distance_to_listener;
		previous_distance_to_listener =
			inPlayingAmbient->channel1->previous_distance_to_listener;
	}
	else if (inPlayingAmbient->channel2 != NULL)
	{
		volume = SSiSoundChannel_GetVolume(inPlayingAmbient->channel2);
		distance_to_listener = inPlayingAmbient->channel2->distance_to_listener;
		previous_distance_to_listener =
			inPlayingAmbient->channel2->previous_distance_to_listener;
	}
	else
	{
		return;
	}
	
	// or if the volume is greater than zero,
	// or if the distance to the listener is less than the distance threshold
	// or if the distance to the listener is decreasing,
	// don't stop the channel
	if (volume > 0.0f) { return; }
	if (distance_to_listener < SScDistanceThreshold) { return; }
	if (distance_to_listener <= previous_distance_to_listener) { return; }
	
	inPlayingAmbient->stage = SScPAStage_NonAudible;
}

// ----------------------------------------------------------------------
static void
SSiPlayingAmbient_Halt(
	SStPlayingAmbient			*inPlayingAmbient)
{
	UUmAssert(inPlayingAmbient);
	
	// stop channel1 completely
	if (inPlayingAmbient->channel1 != NULL)
	{
		SSiSoundChannel_Stop(inPlayingAmbient->channel1);
	}
	
	// stop channel2 completely
	if (inPlayingAmbient->channel2 != NULL)
	{
		SSiSoundChannel_Stop(inPlayingAmbient->channel2);
	}
	
	// go to next stage
	inPlayingAmbient->stage = SScPAStage_Done;
}

// ----------------------------------------------------------------------
static void
SSiPlayingAmbient_Update(
	SStPlayingAmbient			*inPlayingAmbient,
	UUtUns32					inIndex)
{
	UUtBool						next_stage;
	
	do
	{
		switch (inPlayingAmbient->stage)
		{
			case SScPAStage_None:
				return;
			break;
			
			case SScPAStage_Done:
				next_stage = SSiPAUpdate_Done(inPlayingAmbient);
			break;
			
			case SScPAStage_Start:
				next_stage = SSiPAUpdate_Start(inPlayingAmbient);
			break;
			
			case SScPAStage_InSoundStart:
				next_stage = SSiPAUpdate_InSoundStart(inPlayingAmbient);
			break;
			
			case SScPAStage_InSoundPlaying:
				next_stage = SSiPAUpdate_InSoundPlaying(inPlayingAmbient);
			break;
			
			case SScPAStage_BodyStart:
				next_stage = SSiPAUpdate_BodyStart(inPlayingAmbient);
			break;
			
			case SScPAStage_BodyPlaying:
				next_stage = SSiPAUpdate_BodyPlaying(inPlayingAmbient);
			break;
			
			case SScPAStage_BodyStopping:
				next_stage = SSiPAUpdate_BodyStopping(inPlayingAmbient);
			break;
			
			case SScPAStage_OutSoundStart:
				next_stage = SSiPAUpdate_OutSoundStart(inPlayingAmbient);
			break;
			
			case SScPAStage_OutSoundPlaying:
				next_stage = SSiPAUpdate_OutSoundPlaying(inPlayingAmbient);
			break;
			
			case SScPAStage_NonAudible:
				next_stage = SSiPAUpdate_NonAudible(inPlayingAmbient);
			break;
			
			case SScPAStage_DetailOnly:
				next_stage = SSiPAUpdate_DetailOnly(inPlayingAmbient);
			break;
		}
	}
	while (next_stage == UUcTrue);
	
	// update the volume
	SSiPAUpdate_Volume(inPlayingAmbient);
	
	// update the position
	SSiPAUpdate_Position(inPlayingAmbient);
	
	// stop playing ambient sounds that are no longer audible 
	// and likely won't be
	SSiPAUpdate_StopNonAudible(inPlayingAmbient);
}

// ----------------------------------------------------------------------
static void
SSiPlayingAmbient_UpdateList(
	void)
{
	SStPlayingAmbient			*playing_ambient_array;
	UUtUns32					i;
	
	SSrWaitGuard(SSgGuardAll);

	// update all of the playing ambient sounds
	playing_ambient_array = SSgPlayingAmbient;
	for (i = 0; i < SSgNumPlayingAmbients; i++)
	{
		if (playing_ambient_array[i].stage != SScPAStage_None)
		{
			SSiPlayingAmbient_Update(&playing_ambient_array[i], i);
		}
	}

	SSrReleaseGuard(SSgGuardAll);
}

// ----------------------------------------------------------------------
UUtBool
SSiPlayingAmbient_UpdateSoundChannel(
	SStSoundChannel				*inSoundChannel)
{
	SStPlayingAmbient			*playing_ambient_array;
	UUtUns32					i;
	UUtBool						updated;
	
	updated = UUcFalse;
	
	SSrWaitGuard(SSgGuardAll);

	// update all of the playing ambient sounds
	playing_ambient_array = SSgPlayingAmbient;
	for (i = 0; i < SSgNumPlayingAmbients; i++)
	{
		SStPlayingAmbient		*playing_ambient;
		
		playing_ambient = &playing_ambient_array[i];

		if (playing_ambient->stage == SScPAStage_None) { continue; }
		
		if ((playing_ambient->channel1 != inSoundChannel) &&
			(playing_ambient->channel2 != inSoundChannel))
		{
			continue;
		}
		
		// update the sound channel
		SSiPlayingAmbient_Update(playing_ambient, i);
		updated = UUcTrue;
		
		break;
	}

	SSrReleaseGuard(SSgGuardAll);

	return updated;
}

// ----------------------------------------------------------------------
static SStPlayID
SSiPlayingAmbient_GetNextID(
	void)
{
	SSgPlayID++;
	
	if (SSgPlayID == SScInvalidID)
	{
		SSgPlayID = 0;
	}

	return SSgPlayID;
}

// ----------------------------------------------------------------------
static void
SSiPlayingAmbient_Stall(
	SStSoundChannel				*inSoundChannel)
{
	SStPlayingAmbient			*playing_ambient_array;
	UUtUns32					i;
	
	SSrWaitGuard(SSgGuardAll);

/*	if (SSgShowDebugInfo)
	{
		COrConsole_Printf("SSiPlayingAmbient_Stall %d %s", inSoundChannel->id, inSoundChannel->group->group_name);
	}*/
	
	// update all of the playing ambient sounds
	playing_ambient_array = SSgPlayingAmbient;
	for (i = 0; i < SSgNumPlayingAmbients; i++)
	{
		SStPlayingAmbient		*playing_ambient;
		
		playing_ambient = &playing_ambient_array[i];
		if (playing_ambient->stage == SScPAStage_None) { continue; }
		
		// stall the channel
		if (playing_ambient->channel1 == inSoundChannel)
		{
			SSiSoundChannel_SetAmbient(playing_ambient->channel1, UUcFalse);
			SSiSoundChannel_Stop(playing_ambient->channel1);
			SSiSoundChannel_SetLock(playing_ambient->channel1, UUcFalse);
			playing_ambient->channel1 = NULL;
		}
		else if (playing_ambient->channel2 == inSoundChannel)
		{
			SSiSoundChannel_SetAmbient(playing_ambient->channel2, UUcFalse);
			SSiSoundChannel_Stop(playing_ambient->channel2);
			SSiSoundChannel_SetLock(playing_ambient->channel2, UUcFalse);
			playing_ambient->channel2 = NULL;
		}
		else
		{
			continue;
		}
		
		break;
	}

	SSrReleaseGuard(SSgGuardAll);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static int UUcExternal_Call
SSiAmbient_Compare(
	const void					*inItem1,
	const void					*inItem2)
{
	const SStAmbient			*ambient1;
	const SStAmbient			*ambient2;
	
	ambient1 = *((SStAmbient**)inItem1);
	ambient2 = *((SStAmbient**)inItem2);
		
	return strcmp(ambient1->ambient_name, ambient2->ambient_name);
}

// ----------------------------------------------------------------------
static void
SSiAmbient_CheckPointers(
	SStAmbient					*ioAmbient)
{
	// check detail_id
	if ((ioAmbient->detail == NULL) && (ioAmbient->detail_name[0] != '\0'))
	{
		ioAmbient->detail = SSrGroup_GetByName(ioAmbient->detail_name);
	}

	// check base_track1_id
	if ((ioAmbient->base_track1 == NULL) && (ioAmbient->base_track1_name[0] != '\0'))
	{
		ioAmbient->base_track1 = SSrGroup_GetByName(ioAmbient->base_track1_name);
	}
	
	// check base_track2_id
	if ((ioAmbient->base_track2 == NULL) && (ioAmbient->base_track2_name[0] != '\0'))
	{
		ioAmbient->base_track2 = SSrGroup_GetByName(ioAmbient->base_track2_name);
	}
	
	// check in_sound
	if ((ioAmbient->in_sound == NULL) && (ioAmbient->in_sound_name[0] != '\0'))
	{
		ioAmbient->in_sound = SSrGroup_GetByName(ioAmbient->in_sound_name);
	}
	
	// check out_sound
	if ((ioAmbient->out_sound == NULL) && (ioAmbient->out_sound_name[0] != '\0'))
	{
		ioAmbient->out_sound = SSrGroup_GetByName(ioAmbient->out_sound_name);
	}
}

// ----------------------------------------------------------------------
UUtBool
SSrAmbient_CheckSoundData(
	const SStAmbient			*inAmbient)
{
	UUtBool						all_sounds_found;
	
	UUmAssert(inAmbient);
	
	if (inAmbient->detail)
	{
		all_sounds_found = SSrGroup_CheckSoundData(inAmbient->detail);
		if (all_sounds_found == UUcFalse) { return UUcFalse; }
	}
	
	if (inAmbient->base_track1)
	{
		all_sounds_found = SSrGroup_CheckSoundData(inAmbient->base_track1);
		if (all_sounds_found == UUcFalse) { return UUcFalse; }
	}
	
	if (inAmbient->base_track2)
	{	
		all_sounds_found = SSrGroup_CheckSoundData(inAmbient->base_track2);
		if (all_sounds_found == UUcFalse) { return UUcFalse; }
	}
	
	if (inAmbient->in_sound)
	{	
		all_sounds_found = SSrGroup_CheckSoundData(inAmbient->in_sound);
		if (all_sounds_found == UUcFalse) { return UUcFalse; }
	}
	
	if (inAmbient->out_sound)
	{	
		all_sounds_found = SSrGroup_CheckSoundData(inAmbient->out_sound);
		if (all_sounds_found == UUcFalse) { return UUcFalse; }
	}
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtError
SSrAmbient_Copy(
	const SStAmbient			*inSource,
	SStAmbient					*ioDest)
{
	UUmAssert(inSource);
	UUmAssert(ioDest);
	
	// restore the id after copying the data
	ioDest->priority = inSource->priority;
	ioDest->flags = inSource->flags;
	ioDest->sphere_radius = inSource->sphere_radius;
	ioDest->min_detail_time = inSource->min_detail_time;
	ioDest->max_detail_time = inSource->max_detail_time;
	ioDest->max_volume_distance = inSource->max_volume_distance;
	ioDest->min_volume_distance = inSource->min_volume_distance;
	ioDest->detail = inSource->detail;
	ioDest->base_track1 = inSource->base_track1;
	ioDest->base_track2 = inSource->base_track2;
	ioDest->in_sound = inSource->in_sound;
	ioDest->out_sound = inSource->out_sound;
	UUrString_Copy(ioDest->detail_name, inSource->detail_name, SScMaxNameLength);
	UUrString_Copy(ioDest->base_track1_name, inSource->base_track1_name, SScMaxNameLength);
	UUrString_Copy(ioDest->base_track2_name, inSource->base_track2_name, SScMaxNameLength);
	UUrString_Copy(ioDest->in_sound_name, inSource->in_sound_name, SScMaxNameLength);
	UUrString_Copy(ioDest->out_sound_name, inSource->out_sound_name, SScMaxNameLength);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
SSrAmbient_Delete(
	const char					*inAmbientName,
	UUtBool						inUpdatePointers)
{
	UUtUns32					num_ambient_sounds;
	SStAmbient					**ambient_array, *deleted_element = NULL;
	UUtUns32					i;
	
	num_ambient_sounds = UUrMemory_Array_GetUsedElems(SSgAmbientSounds);
	ambient_array = (SStAmbient**)UUrMemory_Array_GetMemory(SSgAmbientSounds);
	
	// delete the ambient sound which has an id equal to inAmbientID
	for (i = 0; i < num_ambient_sounds; i++)
	{
		if (UUrString_Compare_NoCase(ambient_array[i]->ambient_name, inAmbientName) == 0)
		{
			deleted_element = ambient_array[i];
			UUrMemory_Block_Delete(deleted_element);
			UUrMemory_Array_DeleteElement(SSgAmbientSounds, i);
			break;
		}
	}

	if ((inUpdatePointers == UUcTrue) && (deleted_element != NULL))
	{
		SSgAmbientHandler(deleted_element, UUcTrue);
	}
}

// ----------------------------------------------------------------------
SStAmbient*
SSrAmbient_GetByIndex(
	UUtUns32					inIndex)
{
	UUtUns32					num_ambient_sounds;
	SStAmbient					**ambient_array;
	
	num_ambient_sounds = UUrMemory_Array_GetUsedElems(SSgAmbientSounds);
	ambient_array = (SStAmbient**)UUrMemory_Array_GetMemory(SSgAmbientSounds);
	
	UUmAssert(inIndex < num_ambient_sounds);
	
	return ambient_array[inIndex];
}

// ----------------------------------------------------------------------
SStAmbient*
SSrAmbient_GetByName(
	const char					*inAmbientName)
{
	SStAmbient					**ambient_array;
	SStAmbient					find_me;
	SStAmbient					*find_me_ptr = &find_me;
	SStAmbient					**found_ambient;
	UUtUns32					num_elements;
	
	if (inAmbientName == NULL) { return NULL; }
	
	UUrString_Copy(find_me.ambient_name, inAmbientName, SScMaxNameLength);
	
	// get a pointer to the array
	ambient_array = (SStAmbient**)UUrMemory_Array_GetMemory(SSgAmbientSounds);
	if (ambient_array == NULL) { return NULL; }
	
	num_elements = UUrMemory_Array_GetUsedElems(SSgAmbientSounds);
	
	found_ambient =
		(SStAmbient**)bsearch(
			&find_me_ptr,
			ambient_array,
			num_elements,
			sizeof(SStAmbient*),
			SSiAmbient_Compare);
	if (found_ambient == NULL) { return NULL; }

	return *found_ambient;
}

// ----------------------------------------------------------------------
UUtUns32
SSrAmbient_GetNumAmbientSounds(
	void)
{
	UUmAssert(SSgAmbientSounds);
	return UUrMemory_Array_GetUsedElems(SSgAmbientSounds);
}

// ----------------------------------------------------------------------
const char*
SSrAmbient_GetSubtitle(
	SStAmbient					*inAmbient)
{
	if (SScInvalidSubtitle == inAmbient->subtitle)
	{
		const char				*ambient_name;
		
		inAmbient->subtitle = NULL;
		
		ambient_name = inAmbient->ambient_name;
		if (SSrNameIsValidDialogName(ambient_name))
		{
			// first look in the cutscene subtitle file
			inAmbient->subtitle = SSrSubtitle_Find(ambient_name);
		}
	}
	
	return inAmbient->subtitle;
}

// ----------------------------------------------------------------------
void
SSrAmbient_Halt(
	SStPlayID					inPlayID)
{
	SStPlayingAmbient			*playing_ambient_array;
	UUtUns32					i;
	UUtUns32					num_playing_ambients;
	
	if (inPlayID == SScInvalidID) { return; }
	
	SSrWaitGuard(SSgGuardAll);

	// get the playing ambient with id == inPlayID
	playing_ambient_array = SSgPlayingAmbient;
	num_playing_ambients = SSgNumPlayingAmbients;
	for(i = 0; i < num_playing_ambients; i++)
	{
		SStPlayingAmbient		*playing_ambient;
		
		playing_ambient = &playing_ambient_array[i];

		if (playing_ambient->stage == SScPAStage_None) { continue; }
		if (playing_ambient->id != inPlayID) { continue; }
		
		SSiPlayingAmbient_Halt(playing_ambient);
		SSiPlayingAmbient_Update(playing_ambient, i);
		
		break;
	}

	SSrReleaseGuard(SSgGuardAll);
}

// ----------------------------------------------------------------------
UUtError
SSrAmbient_New(
	const char					*inAmbientName,
	SStAmbient					**outAmbient)
{
	UUtError					error;
	SStAmbient					*ambient;
	
	UUmAssert(inAmbientName);
	UUmAssert(outAmbient);
	
	if (outAmbient) { *outAmbient = NULL; }
	
	// allocate memory for the ambient sound
	ambient = (SStAmbient*)UUrMemory_Block_New(sizeof(SStAmbient));
	UUmError_ReturnOnNull(ambient);
	
	// initialize the ambient sound
	UUrString_Copy(ambient->ambient_name, inAmbientName, SScMaxNameLength);
	ambient->priority = SScPriority2_Normal;
	ambient->flags = SScAmbientFlag_None;
	ambient->sphere_radius = 10.0f;
	ambient->min_detail_time = 0.0f;
	ambient->max_detail_time = 0.0f;
	ambient->max_volume_distance = 10.0f;
	ambient->min_volume_distance = 50.0f;
	ambient->threshold = SScAmbientThreshold;
	ambient->min_occlusion = 0.0f;
	ambient->detail = NULL;
	ambient->base_track1 = NULL;
	ambient->base_track2 = NULL;
	ambient->in_sound = NULL;
	ambient->out_sound = NULL;
	ambient->detail_name[0] = '\0';
	ambient->base_track1_name[0] = '\0';
	ambient->base_track2_name[0] = '\0';
	ambient->in_sound_name[0] = '\0';
	ambient->out_sound_name[0] = '\0';
	ambient->subtitle = SScInvalidSubtitle;
	
	// insert the ambient sound into the ambient sound list
	{
		SStAmbient				**array;
		UUtUns32				num_elements;
		UUtInt32				min, max, midpoint, compare;
		UUtUns32				found_location;
		
		array = (SStAmbient**)UUrMemory_Array_GetMemory(SSgAmbientSounds);
		num_elements = UUrMemory_Array_GetUsedElems(SSgAmbientSounds);
		
		min = 0; 
		max = num_elements - 1;
		
		while(min <= max)
		{
			midpoint = (min + max) >> 1; // S.S. / 2;
		
			compare = SSiAmbient_Compare(&ambient, &array[midpoint]);
		
			if (compare < 0) {
				// must lie below here
				max = midpoint - 1;
		
			} else {
				// must lie above here
				min = midpoint + 1;
			}
		}
		
		found_location = (UUtUns32)min;
		
		// create space in the array at the desired location
		UUmAssert((found_location >= 0) && (found_location <= num_elements));
		error = UUrMemory_Array_InsertElement(SSgAmbientSounds, found_location, NULL);
		UUmError_ReturnOnError(error);
		
		// set the array element
		array = (SStAmbient**)UUrMemory_Array_GetMemory(SSgAmbientSounds);
		array[found_location] = ambient;
	}
		
	// set the outgoing data
	if (outAmbient) { *outAmbient = ambient; }
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
SSrAmbient_SetPitch(
	SStPlayID					inPlayID,
	float						inPitch)
{
	SStPlayingAmbient			*playing_ambient_array;
	UUtUns32					i;
	UUtUns32					num_playing_ambients;

	if (SSgUsable == UUcFalse) { return; }

	SSrWaitGuard(SSgGuardAll);

	// get the playing ambient with id == inPlayID
	playing_ambient_array = SSgPlayingAmbient;
	num_playing_ambients = SSgNumPlayingAmbients;
	for(i = 0; i < num_playing_ambients; i++)
	{
		if (playing_ambient_array[i].stage == SScPAStage_None) { continue; }
		if (playing_ambient_array[i].id != inPlayID) { continue; }
		
		if (playing_ambient_array[i].channel1 != NULL)
		{
			SSiSoundChannel_SetPitch(playing_ambient_array[i].channel1, inPitch);
		}
		
		if (playing_ambient_array[i].channel2 != NULL)
		{
			SSiSoundChannel_SetPitch(playing_ambient_array[i].channel2, inPitch);
		}
	}

	SSrReleaseGuard(SSgGuardAll);
}

// ----------------------------------------------------------------------
void
SSrAmbient_SetVolume(
	SStPlayID					inPlayID,
	float						inVolume,
	float						inTime)				// seconds
{
	SStPlayingAmbient			*playing_ambient_array;
	UUtUns32					i;
	UUtUns32					num_playing_ambients;

	if (SSgUsable == UUcFalse) { return; }

	SSrWaitGuard(SSgGuardAll);
	
	// make sure it is in range
	inVolume = UUmPin(inVolume, 0.0f, 1.0f);
	
	// get the playing ambient with id == inPlayID
	playing_ambient_array = SSgPlayingAmbient;
	num_playing_ambients = SSgNumPlayingAmbients;
	for(i = 0; i < num_playing_ambients; i++)
	{
		float					volume_adjust_ticks;
		
		if (playing_ambient_array[i].stage == SScPAStage_None) { continue; }
		if (playing_ambient_array[i].id != inPlayID) { continue; }
		
		// break if the final volume is already set
		if (playing_ambient_array[i].volume_adjust_final_volume == inVolume) { break; }
		
		volume_adjust_ticks = inTime * 60.0f;
		if (volume_adjust_ticks > 0.0f)
		{
			playing_ambient_array[i].volume_adjust_delta =
				(inVolume - playing_ambient_array[i].channel_volume) / volume_adjust_ticks;
			playing_ambient_array[i].volume_adjust_final_volume = inVolume;
			playing_ambient_array[i].volume_adjust_start_time = UUrMachineTime_Sixtieths();
		}
		else
		{
			// record the channel volume so that the detail sound will have a volume
			// to use
			playing_ambient_array[i].channel_volume = inVolume;
			playing_ambient_array[i].volume_adjust_final_volume = playing_ambient_array[i].channel_volume;
			
			// set the volume of channel1 and channel2
			if (playing_ambient_array[i].channel1 != NULL)
			{
				SSiSoundChannel_SetChannelVolume(playing_ambient_array[i].channel1, inVolume);
			}
			
			if (playing_ambient_array[i].channel2 != NULL)
			{
				SSiSoundChannel_SetChannelVolume(playing_ambient_array[i].channel2, inVolume);
			}
		}
		
		break;
	}

	SSrReleaseGuard(SSgGuardAll);
}

// ----------------------------------------------------------------------
SStPlayID
SSrAmbient_Start(
	const SStAmbient			*inAmbient,
	const M3tPoint3D			*inPosition,		// inPosition == NULL will play non-spatial
	const M3tVector3D			*inDirection,		/* unused if inPosition == NULL */
	const M3tVector3D			*inVelocity,		/* unused if inPosition == NULL */
	const float					inMaxVolDistance,	/* unused if inPosition == NULL */
	const float					inMinVolDistance,	/* unused if inPosition == NULL */
	const float					*inVolume)
{
	UUtUns32					index;
	SStPlayingAmbient			*playing_ambient_array;
	UUtUns32					num_elements;
	UUtUns32					itr;
	UUtUns32					num_playing;
	
	UUmAssert(inAmbient);
	
	if (SSgUsable == UUcFalse) { return SScInvalidID; }
	
	SSrWaitGuard(SSgGuardAll);
	
	// count the number of instances of this ambient sound that are already playing
	// and keep track of the first available index of the SSgPlayingAmbient list
	index = SScInvalidID;
	num_playing = 0;
	playing_ambient_array = SSgPlayingAmbient;
	num_elements = SSgNumPlayingAmbients;	
	for (itr = 0; itr < num_elements; itr++)
	{
		if (playing_ambient_array[itr].stage == SScPAStage_None)
		{
			if (index == SScInvalidID) { index = itr; }
			continue;
		}
		if (inAmbient != playing_ambient_array[itr].ambient) { continue; }
		if (playing_ambient_array[itr].stage >= SScPAStage_OutSoundStart) { continue; }
		num_playing++;
	}
	
	// stop playing if the ambient threshold has been reached
	if (num_playing >= inAmbient->threshold)
	{
		SSrReleaseGuard(SSgGuardAll);
		return SScInvalidID;
	}
	
	// add the ambient sound to the SSgPlayingAmbient list
	if (index == SScInvalidID)
	{
		SSrReleaseGuard(SSgGuardAll);
		return SScInvalidID;
	}
	
	playing_ambient_array[index].id = SSiPlayingAmbient_GetNextID();
	playing_ambient_array[index].stage = SScPAStage_Start;
	playing_ambient_array[index].ambient = inAmbient;
	playing_ambient_array[index].channel1 = NULL;
	playing_ambient_array[index].channel2 = NULL;
	playing_ambient_array[index].has_position = UUcFalse;
	playing_ambient_array[index].detail_time = 0;
	if (inVolume)
	{
		playing_ambient_array[index].channel_volume = *inVolume;
	}
	else
	{
		playing_ambient_array[index].channel_volume = 1.0f;
	}
	playing_ambient_array[index].volume_adjust_start_time = UUrMachineTime_Sixtieths();
	playing_ambient_array[index].volume_adjust_delta = 0.0f;
	playing_ambient_array[index].volume_adjust_final_volume = playing_ambient_array[index].channel_volume;
	
	if (inPosition)
	{
		playing_ambient_array[index].position = *inPosition;
		playing_ambient_array[index].has_position = UUcTrue;
	
		if (inDirection) { playing_ambient_array[index].direction = *inDirection; }
		if (inVelocity) { playing_ambient_array[index].velocity = *inVelocity; }
		playing_ambient_array[index].max_volume_distance = inMaxVolDistance;
		playing_ambient_array[index].min_volume_distance = inMinVolDistance;
	}
	else
	{
		playing_ambient_array[index].max_volume_distance = inAmbient->max_volume_distance;
		playing_ambient_array[index].min_volume_distance = inAmbient->min_volume_distance;
	}
	
	SSiPlayingAmbient_Update(&playing_ambient_array[index], index);
	
/*	if (SSgShowDebugInfo)
	{
		COrConsole_Printf(
			"SSrAmbient_Start %d %s",
			playing_ambient_array[index].id,
			playing_ambient_array[index].ambient->ambient_name);
	}*/
	
	SSrReleaseGuard(SSgGuardAll);
	
	return playing_ambient_array[index].id;
}

// ----------------------------------------------------------------------
SStPlayID
SSrAmbient_Start_Simple(
	const SStAmbient			*inAmbient,
	const float					*inVolume)
{
	return SSrAmbient_Start(inAmbient, NULL, NULL, NULL, 0.0f, 0.0f, inVolume);
}

// ----------------------------------------------------------------------
void
SSrAmbient_Stop(
	SStPlayID					inPlayID)
{
	SStPlayingAmbient			*playing_ambient_array;
	UUtUns32					i;
	UUtUns32					num_playing_ambients;
	
	if (SSgUsable == UUcFalse) { return; }

	SSrWaitGuard(SSgGuardAll);

	// get the playing ambient with id == inPlayID
	playing_ambient_array = SSgPlayingAmbient;
	num_playing_ambients = SSgNumPlayingAmbients;
	for(i = 0; i < num_playing_ambients; i++)
	{
		if (playing_ambient_array[i].id != inPlayID) { continue; }

		if ((playing_ambient_array[i].stage == SScPAStage_None) ||
			((playing_ambient_array[i].stage >= SScPAStage_BodyStopping) &&
			(playing_ambient_array[i].stage <= SScPAStage_OutSoundPlaying)))
		{
			break;
		}
		
		
		if ((playing_ambient_array[i].ambient->flags & SScAmbientFlag_InterruptOnStop) != 0)
		{
			// stop channel 1
			if (playing_ambient_array[i].channel1 != NULL)
			{
				SSiSoundChannel_Stop(playing_ambient_array[i].channel1);
			}
			
			// stop channel 2
			if (playing_ambient_array[i].channel2 != NULL)
			{
				SSiSoundChannel_Stop(playing_ambient_array[i].channel2);
			}
		}
		else
		{
			// make sure channel 2 isn't looping
			if ((playing_ambient_array[i].channel2 != NULL) &&
				(SSiSoundChannel_IsPlaying(playing_ambient_array[i].channel2) == UUcTrue))
			{
				SSiSoundChannel_SetLooping(playing_ambient_array[i].channel2, UUcFalse);
				break;
			}
			else
			{
				// make sure channel 1 isn't looping
				if (playing_ambient_array[i].channel1 != NULL)
				{
					SSiSoundChannel_SetLooping(playing_ambient_array[i].channel1, UUcFalse);
				}
			}
		}
		
		// go to next stage
		playing_ambient_array[i].stage = SScPAStage_BodyStopping;
		
/*		if (SSgShowDebugInfo)
		{
			COrConsole_Printf(
				"SSrAmbient_Stop %d %s",
				playing_ambient_array[i].id,
				playing_ambient_array[i].ambient->ambient_name);
		}*/
		
		break;
	}
	
	SSrReleaseGuard(SSgGuardAll);
}

// ----------------------------------------------------------------------
UUtBool
SSrAmbient_Update(
	SStPlayID					inPlayID,
	const M3tPoint3D			*inPosition,
	const M3tVector3D			*inDirection,
	const M3tVector3D			*inVelocity,
	const float					*inVolume)
{
	SStPlayingAmbient			*playing_ambient_array;
	UUtUns32					num_playing_ambients;
	UUtUns32					i;
	UUtBool						active;
	
	if (SSgUsable == UUcFalse) { return UUcFalse; }

	SSrWaitGuard(SSgGuardAll);

	// get the playing ambient with id == inPlayID
	active = UUcFalse;
	playing_ambient_array = SSgPlayingAmbient;
	num_playing_ambients = SSgNumPlayingAmbients;
	for(i = 0; i < num_playing_ambients; i++)
	{
		if (playing_ambient_array[i].stage == SScPAStage_None) { continue; }
		if (playing_ambient_array[i].id != inPlayID) { continue; }
		
		// update the position, direction, and velocity of the playing ambient
		if (inPosition)
		{
			playing_ambient_array[i].position = *inPosition;
			playing_ambient_array[i].has_position = UUcTrue;
		}
		if (inDirection) { playing_ambient_array[i].direction = *inDirection; }
		if (inVelocity) { playing_ambient_array[i].velocity = *inVelocity; }
		
		if (inVolume)
		{
			playing_ambient_array[i].channel_volume = *inVolume;
			
			if (playing_ambient_array[i].channel1 != NULL)
			{
				SSiSoundChannel_SetChannelVolume(
					playing_ambient_array[i].channel1,
					playing_ambient_array[i].channel_volume);
			}
			
			if (playing_ambient_array[i].channel2 != NULL)
			{
				SSiSoundChannel_SetChannelVolume(
					playing_ambient_array[i].channel2,
					playing_ambient_array[i].channel_volume);
			}
		}
		
		active = UUcTrue;
		
		break;
	}
	
	SSrReleaseGuard(SSgGuardAll);

	return active;
}

// ----------------------------------------------------------------------
void
SSrAmbient_UpdateGroupName(
	const char					*inOldGroupName,
	const char					*inNewGroupName)
{
	UUtUns32					num_ambient_sounds;
	SStAmbient					**ambient_array;
	UUtUns32					i;
	
	num_ambient_sounds = UUrMemory_Array_GetUsedElems(SSgAmbientSounds);
	ambient_array = (SStAmbient**)UUrMemory_Array_GetMemory(SSgAmbientSounds);
	
	// get a new pointer to each SStGroup
	for (i = 0; i < num_ambient_sounds; i++)
	{
		SStAmbient				*ambient;
		
		ambient = ambient_array[i];
		
		// update the detail group
		if (UUrString_Compare_NoCase(ambient->detail_name, inOldGroupName) == 0)
		{
			if (inNewGroupName == NULL)
			{
				UUrMemory_Clear(ambient->detail_name, SScMaxNameLength);
				ambient->detail = NULL;
			}
			else
			{
				UUrString_Copy(ambient->detail_name, inNewGroupName, SScMaxNameLength);
			}
		}

		// update the base track 1
		if (UUrString_Compare_NoCase(ambient->base_track1_name, inOldGroupName) == 0)
		{
			if (inNewGroupName == NULL)
			{
				UUrMemory_Clear(ambient->base_track1_name, SScMaxNameLength);
				ambient->base_track1 = NULL;
			}
			else
			{
				UUrString_Copy(ambient->base_track1_name, inNewGroupName, SScMaxNameLength);
			}
		}

		// update the base track 2
		if (UUrString_Compare_NoCase(ambient->base_track2_name, inOldGroupName) == 0)
		{
			if (inNewGroupName == NULL)
			{
				UUrMemory_Clear(ambient->base_track2_name, SScMaxNameLength);
				ambient->base_track2 = NULL;
			}
			else
			{
				UUrString_Copy(ambient->base_track2_name, inNewGroupName, SScMaxNameLength);
			}
		}

		// update the in sound
		if (UUrString_Compare_NoCase(ambient->in_sound_name, inOldGroupName) == 0)
		{
			if (inNewGroupName == NULL)
			{
				UUrMemory_Clear(ambient->in_sound_name, SScMaxNameLength);
				ambient->in_sound = NULL;
			}
			else
			{
				UUrString_Copy(ambient->in_sound_name, inNewGroupName, SScMaxNameLength);
			}
		}

		// update the out sound
		if (UUrString_Compare_NoCase(ambient->out_sound_name, inOldGroupName) == 0)
		{
			if (inNewGroupName == NULL)
			{
				UUrMemory_Clear(ambient->out_sound_name, SScMaxNameLength);
				ambient->out_sound = NULL;
			}
			else
			{
				UUrString_Copy(ambient->out_sound_name, inNewGroupName, SScMaxNameLength);
			}
		}
	}
}

// ----------------------------------------------------------------------
void
SSrAmbient_UpdateGroupPointers(
	void)
{
	UUtUns32					num_ambient_sounds;
	SStAmbient					**ambient_array;
	UUtUns32					i;
	
	num_ambient_sounds = UUrMemory_Array_GetUsedElems(SSgAmbientSounds);
	ambient_array = (SStAmbient**)UUrMemory_Array_GetMemory(SSgAmbientSounds);
	
	// get a new pointer to each SStGroup
	for (i = 0; i < num_ambient_sounds; i++)
	{
		SStAmbient				*ambient;
		
		ambient = ambient_array[i];
		
		// get pointers to all of the detail groups
		ambient->detail = SSrGroup_GetByName(ambient->detail_name);
		ambient->base_track1 = SSrGroup_GetByName(ambient->base_track1_name);
		ambient->base_track2 = SSrGroup_GetByName(ambient->base_track2_name);
		ambient->in_sound = SSrGroup_GetByName(ambient->in_sound_name);
		ambient->out_sound = SSrGroup_GetByName(ambient->out_sound_name);
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
SSrListener_SetPosition(
	const M3tPoint3D			*inPosition,
	const M3tVector3D			*inFacing)
{
	SSgListener_Position = *inPosition;
	SSgListener_Facing = *inFacing;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
SSrGetSoundDirectory(
	BFtFileRef					**outDirectoryRef)
{
	UUtError					error;
	BFtFileRef					prefFileRef;
	GRtGroup_Context			*prefGroupContext;
	GRtGroup					*prefGroup;
	char						*soundDirName;
	char						soundDirNameStr[BFcMaxPathLength];
	BFtFileRef					*soundDirRef;
	
	// find the preferences file
#if defined(SHIPPING_VERSION) && (!SHIPPING_VERSION)
	error = BFrFileRef_Search("Preferences.txt", &prefFileRef);
#else
	error= UUcError_Generic;
#endif
	if (error != UUcError_None) {
		*outDirectoryRef = NULL;
		return UUcError_Generic;
	}
	
	// create a group from the file
	error =
		GRrGroup_Context_NewFromFileRef(
			&prefFileRef,
			NULL,
			NULL,
			&prefGroupContext,
			&prefGroup);
	if (error != UUcError_None) { return error; }
	
	// get the data file directory
	error = GRrGroup_GetString(prefGroup, "SoundFileDir", &soundDirName);
	if (error != UUcError_None)
	{
		char						*dataFileDir;
		
		// get the data file directory
		error = GRrGroup_GetString(prefGroup, "DataFileDir", &dataFileDir);
		if (error != UUcError_None) { return error; }
		
		// set the sound Dir name
		sprintf(soundDirNameStr, "%s\\%s", dataFileDir, SScSoundDirectory);
		soundDirName = soundDirNameStr;
	}
	
	// make a file ref for the sound data folder
	error = BFrFileRef_MakeFromName(soundDirName, &soundDirRef);
	if (error != UUcError_None) { return error; }
	
	// delete the group context
	GRrGroup_Context_Delete(prefGroupContext);
	prefGroupContext = NULL;
	
	// see if the directory exists
	if (BFrFileRef_FileExists(soundDirRef) == UUcFalse)
	{
		// the directory was not found delete the folder ref
		UUrMemory_Block_Delete(soundDirRef);
		soundDirRef = NULL;
		
		*outDirectoryRef = NULL;
		
		return UUcError_Generic;
	}
	
	*outDirectoryRef = soundDirRef;
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
SSiTextFile_WriteAmbient(
	BFtFile					*inFile)
{
	UUtUns32				num_ambients;
	UUtUns32				i;
	char					string[2048];
	
	// write the header
	sprintf(string, "Name\tPriority\tIn Sound\tOut Sound\tBase Track 1\tBase Track 2\tDetail Track\tMin Detail Time\tMax Detail Time\tSphere Radius\tMax Volume Distance\tMin Volume Distance\tInterrupt On Stop\tPlay Once\n");
	BFrFile_Write(inFile, strlen(string), string);
	
	// write the ambient sounds
	num_ambients = SSrAmbient_GetNumAmbientSounds();
	for (i = 0; i < num_ambients; i++)
	{
		SStAmbient			*ambient;
		char				number[128];
		const char			*priority_name;
		
		ambient = SSrAmbient_GetByIndex(i);
		if (ambient == NULL) { continue; }
		
		string[0] = '\0';
		
		strcat(string, ambient->ambient_name); strcat(string, "\t");

		switch(ambient->priority)
		{
			case SScPriority2_Low:
			case SScPriority2_Normal:
			case SScPriority2_High:
			case SScPriority2_Highest:
				priority_name = SSgPriority_Name[ambient->priority];
			break;
			
			default:
				priority_name = "";
			break;
		}

		strcat(string, priority_name); strcat(string, "\t");
		
		if (ambient->in_sound) { strcat(string, ambient->in_sound->group_name); }
		strcat(string, "\t");

		if (ambient->out_sound) { strcat(string, ambient->out_sound->group_name); }
		strcat(string, "\t");

		if (ambient->base_track1) { strcat(string, ambient->base_track1->group_name); }
		strcat(string, "\t");

		if (ambient->base_track2) { strcat(string, ambient->base_track2->group_name); }
		strcat(string, "\t");

		if (ambient->detail) { strcat(string, ambient->detail->group_name); }
		strcat(string, "\t");
		
		sprintf(number, "%5.3f", ambient->min_detail_time); strcat(string, number); strcat(string, "\t");
		sprintf(number, "%5.3f", ambient->max_detail_time); strcat(string, number); strcat(string, "\t");
		sprintf(number, "%5.3f", ambient->sphere_radius); strcat(string, number); strcat(string, "\t");
		sprintf(number, "%5.3f", ambient->max_volume_distance); strcat(string, number); strcat(string, "\t");
		sprintf(number, "%5.3f", ambient->min_volume_distance); strcat(string, number); strcat(string, "\t");

		if ((ambient->flags & SScAmbientFlag_InterruptOnStop) != 0) { strcat(string, "Y"); }
		strcat(string, "\t");

		if ((ambient->flags & SScAmbientFlag_PlayOnce) != 0) { strcat(string, "Y"); }
		
		strcat(string, "\n");

		BFrFile_Write(inFile, strlen(string), string);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
SSiTextFile_WriteImpulse(
	BFtFile					*inFile)
{
	UUtUns32				num_impulses;
	UUtUns32				i;
	char					string[2048];
	
	// write the header
	sprintf(string, "Name\tPriority\tSound\tMax Volume Distance\tMin Volume Distance\n");
	BFrFile_Write(inFile, strlen(string), string);
	
	// write the impulse sounds
	num_impulses = SSrImpulse_GetNumImpulseSounds();
	for (i = 0; i < num_impulses; i++)
	{
		SStImpulse			*impulse;
		char				number[128];
		const char			*priority_name;
		
		impulse = SSrImpulse_GetByIndex(i);
		if (impulse == NULL) { continue; }
		
		string[0] = '\0';
		
		strcat(string, impulse->impulse_name); strcat(string, "\t");
		
		switch(impulse->priority)
		{
			case SScPriority2_Low:
			case SScPriority2_Normal:
			case SScPriority2_High:
			case SScPriority2_Highest:
				priority_name = SSgPriority_Name[impulse->priority];
			break;
			
			default:
				priority_name = "";
			break;
		}
		
		strcat(string, priority_name); strcat(string, "\t");

		if (impulse->impulse_group) { strcat(string, impulse->impulse_group->group_name); }
		strcat(string, "\t");
		
		sprintf(number, "%5.3f", impulse->min_volume_distance); strcat(string, number); strcat(string, "\t");
		sprintf(number, "%5.3f", impulse->max_volume_distance); strcat(string, number);

		strcat(string, "\n");

		BFrFile_Write(inFile, strlen(string), string);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
SSiTextFile_WriteGroup(
	BFtFile					*inFile)
{
	UUtUns32				num_groups;
	UUtUns32				i;
	char					string[2048];
	
	// write the header
	sprintf(string, "Name\tVolume\n");
	BFrFile_Write(inFile, strlen(string), string);
	sprintf(string, "Perm\tFile Name\tWeight\tMin Volume Percent\tMax Volume Percent\tMin Pitch Percent\tMax Pitch Percent\n");
	BFrFile_Write(inFile, strlen(string), string);
	
	// write the groups
	num_groups = SSrGroup_GetNumSoundGroups();
	for (i = 0; i < num_groups; i++)
	{
		SStGroup			*group;
		SStPermutation		*perm_array;
		UUtUns32			num_permutations;
		UUtUns32			j;
		
		group = SSrGroup_GetByIndex(i);
		if (group == NULL) { continue; }
		
		sprintf(string, "%s \t %5.3f \n", group->group_name, group->group_volume);
		BFrFile_Write(inFile, strlen(string), string);
		
		perm_array = (SStPermutation*)UUrMemory_Array_GetMemory(group->permutations);
		num_permutations = UUrMemory_Array_GetUsedElems(group->permutations);
		for (j = 0; j < num_permutations; j++)
		{
			SStPermutation		*perm;
			
			perm = &perm_array[j];
			sprintf(
				string,
				"%d\t %s\t %d\t %5.3f \t %5.3f \t %5.3f \t %5.3f \n",
				j,
				perm->sound_data_name,
				perm->weight,
				perm->min_volume_percent,
				perm->max_volume_percent,
				perm->min_pitch_percent,
				perm->max_pitch_percent);
			BFrFile_Write(inFile, strlen(string), string);
		}
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
SSiTextFile_Write(
	SStType					inType)
{
	UUtError				error;
	char					filename[128];
	BFtFileRef				*file_ref;
	BFtFile					*file;
	
	// create the env file ref
	switch (inType)
	{
		case SScType_Ambient:
			sprintf(filename, "Sound_Ambient.txt");
		break;
		
		case SScType_Impulse:
			sprintf(filename, "Sound_Impulse.txt");
		break;
		
		case SScType_Group:
			sprintf(filename, "Sound_Group.txt");
		break;
	}
	error = BFrFileRef_MakeFromName(filename, &file_ref);
	UUmError_ReturnOnError(error);
	
	// create the .TXT file if it doesn't already exist
	if (BFrFileRef_FileExists(file_ref) == UUcFalse)
	{
		// create the object.env file
		error = BFrFile_Create(file_ref);
		UUmError_ReturnOnError(error);
	}
	
	// open the file
	error = BFrFile_Open(file_ref, "rw", &file);
	UUmError_ReturnOnError(error);
	
	// set the position to 0
	error = BFrFile_SetPos(file, 0);
	UUmError_ReturnOnError(error);
	
	// write the items
	switch (inType)
	{
		case SScType_Ambient:
			SSiTextFile_WriteAmbient(file);
		break;
		
		case SScType_Impulse:
			SSiTextFile_WriteImpulse(file);
		break;
		
		case SScType_Group:
			SSiTextFile_WriteGroup(file);
		break;
	}

	// set the end of the file
	BFrFile_SetEOF(file);
	
	// close the file
	BFrFile_Close(file);
	file = NULL;
	
	// delete the file ref
	BFrFileRef_Dispose(file_ref);
	file_ref = NULL;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
SSrTextFile_Write(
	void)
{
	UUtError					error;
	
	error = SSiTextFile_Write(SScType_Ambient);
	UUmError_ReturnOnError(error);

	error = SSiTextFile_Write(SScType_Impulse);
	UUmError_ReturnOnError(error);

	error = SSiTextFile_Write(SScType_Group);
	UUmError_ReturnOnError(error);
	
	return error;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
SSrListBrokenSounds(
	BFtFile					*inFile)
{
	SStAmbient				**ambient_array;
	UUtUns32				num_ambients;
	SStImpulse				**impulse_array;
	UUtUns32				num_impulses;
	SStGroup				**group_array;
	UUtUns32				num_groups;
	UUtUns32				i;
	char					text[1024];
	
	// make sure all of the pointers are up to date
	SSrAmbient_UpdateGroupPointers();
	SSrImpulse_UpdateGroupPointers();
	
	// printf a header
	BFrFile_Printf(inFile, "********** Ambient Sound Links **********"UUmNL);
	BFrFile_Printf(inFile, UUmNL);

	// go through all of the ambient sounds and list ones which can't find their groups
	num_ambients = UUrMemory_Array_GetUsedElems(SSgAmbientSounds);
	if (num_ambients > 0)
	{
		BFrFile_Printf(inFile, "Ambient\tGroup\tGroup Name"UUmNL);
	}
	
	ambient_array = (SStAmbient**)UUrMemory_Array_GetMemory(SSgAmbientSounds);
	for (i = 0; i < num_ambients; i++)
	{
		SStAmbient				*ambient;
		
		ambient = ambient_array[i];
		
		if ((ambient->in_sound_name[0] != '\0') && (ambient->in_sound == NULL))
		{
			sprintf(text, "%sin_sound\t%s", ambient->ambient_name, ambient->in_sound_name);
			COrConsole_Printf(text);
			BFrFile_Printf(inFile, "%s"UUmNL, text);
		}

		if ((ambient->out_sound_name[0] != '\0') && (ambient->out_sound == NULL))
		{
			sprintf(text, "%s\tout_sound\t%s", ambient->ambient_name, ambient->out_sound_name);
			COrConsole_Printf(text);
			BFrFile_Printf(inFile, "%s"UUmNL, text);
		}

		if ((ambient->base_track1_name[0] != '\0') && (ambient->base_track1 == NULL))
		{
			sprintf(text, "%s\tbase_track1\t%s", ambient->ambient_name, ambient->base_track1_name);
			COrConsole_Printf(text);
			BFrFile_Printf(inFile, "%s"UUmNL, text);
		}

		if ((ambient->base_track2_name[0] != '\0') && (ambient->base_track2 == NULL))
		{
			sprintf(text, "%s\tbase_track2\t%s", ambient->ambient_name, ambient->base_track2_name);
			COrConsole_Printf(text);
			BFrFile_Printf(inFile, "%s"UUmNL, text);
		}

		if ((ambient->detail_name[0] != '\0') && (ambient->detail == NULL))
		{
			sprintf(text, "%s\tdetail\t%s", ambient->ambient_name, ambient->detail_name);
			COrConsole_Printf(text);
			BFrFile_Printf(inFile, "%s"UUmNL, text);
		}
	}
	
	if (num_ambients > 0)
	{
		BFrFile_Printf(inFile, UUmNL);
		BFrFile_Printf(inFile, UUmNL);
	}
	
	// go through all of the impulse sounds and list ones which can't find their groups
	num_impulses = UUrMemory_Array_GetUsedElems(SSgImpulseSounds);
	if (num_impulses > 0)
	{
		BFrFile_Printf(inFile, "Impulse\tGroup Name"UUmNL);
	}
	impulse_array = (SStImpulse**)UUrMemory_Array_GetMemory(SSgImpulseSounds);
	for (i = 0; i < num_impulses; i++)
	{
		SStImpulse				*impulse;
		
		impulse = impulse_array[i];
		
		if ((impulse->impulse_group_name[0] != '\0') && (impulse->impulse_group == NULL))
		{
			sprintf(text, "%s\t%s", impulse->impulse_name, impulse->impulse_group_name);
			COrConsole_Printf(text);
			BFrFile_Printf(inFile, "%s"UUmNL, text);
		}
	}
	
	if (num_impulses > 0)
	{
		BFrFile_Printf(inFile, UUmNL);
		BFrFile_Printf(inFile, UUmNL);
	}

	if (SSgSearchOnDisk) {
		// go through the groups and list ones which can't find sound data
		num_groups = UUrMemory_Array_GetUsedElems(SSgSoundGroups);
		if (num_groups > 0)
		{
			BFrFile_Printf(inFile, "Group\tPermutation\tSound Name"UUmNL);
		}
		group_array = (SStGroup**)UUrMemory_Array_GetMemory(SSgSoundGroups);
		for (i = 0; i < num_groups; i++)
		{
			SStGroup				*group;
			UUtUns32				j;
			SStPermutation			*perm_array;
			UUtUns32				num_permutations;
			
			group = group_array[i];
			
			perm_array = (SStPermutation*)UUrMemory_Array_GetMemory(group->permutations);
			num_permutations = UUrMemory_Array_GetUsedElems(group->permutations);
			
			for (j = 0; j < num_permutations; j++)
			{
				SStPermutation		*perm;
				
				perm = &perm_array[j];
				
				if (perm->sound_data == NULL)
				{
					if (perm->sound_data_name[0] == '\0')
					{
						sprintf(
							text,
							"%s\t%d",
							group->group_name,
							j);
					}
					else
					{
						sprintf(
							text,
							"%s\t%d\t%s",
							group->group_name,
							j,
							perm->sound_data_name);
					}
					COrConsole_Printf(text);
					BFrFile_Printf(inFile, "%s"UUmNL, text);
				}
			}
		}
		
		BFrFile_Printf(inFile, UUmNL);
		BFrFile_Printf(inFile, UUmNL);
		BFrFile_Printf(inFile, UUmNL);
	}
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
SSrDetails_SetCanPlay(
	UUtBool						inCanPlay)
{
	SSgDetails_CanPlay = inCanPlay;
}

// ----------------------------------------------------------------------
void
SS2rFrame_Start(
	void)
{
	SS2rPlatform_PerformanceStartFrame();
}

// ----------------------------------------------------------------------
void
SS2rFrame_End(
	void)
{
	SS2rPlatform_PerformanceEndFrame();
}

// ----------------------------------------------------------------------
UUtError
SS2rInitializeBasic(
	UUtWindow					inWindow,
	UUtBool						inUseSound)
{
	UUtError					error;
	
	UUrStartupMessage("initializing basic sound system 2 layer...");
	
	SSgSoundChannels_Mono = NULL;
	SSgSoundChannels_Stereo = NULL;
	SSgSoundChannels_NumStereo = 0;
	SSgSoundChannels_NumMono = 0;

	// this must be done before SS2rPlatform_InitializeThread()
	SSrCreateGuard(&SSgGuardAll);

	// initialize the platform specific sound stuff
	error = SS2rPlatform_Initialize(inWindow, &SSgSoundChannels_NumTotal, inUseSound);
	if (error != UUcError_None) { goto exit; }
	
	// initialize the sound channels
	error =
		SSiSoundChannels_Initialize(
			SSgSoundChannels_NumTotal,
			&SSgSoundChannels_NumStereo,
			&SSgSoundChannels_NumMono);
	if (error != UUcError_None) { goto exit; }
	
	SSgChannelsInitialized = UUcTrue;
	SSgEnabled = (SSgSoundChannels_NumTotal > 0);

	// start the thread
	error = SS2rPlatform_InitializeThread();
	if (error != UUcError_None) { goto exit; }
	
exit:
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
SS2rInitializeFull(
	void)
{
	UUtError					error;
	UUtUns32					itr;
	
	UUrStartupMessage("initializing full sound system 2...");

/*	if (!SSgChannelsInitialized) {
		// this requires the base level to be initialized first
		goto exit;
	}*/
	
	SSrWaitGuard(SSgGuardAll);
	
	// no pointer handlers yet
	SSgImpulseHandler = NULL;
	SSgAmbientHandler = NULL;

	// register the templates
	error = SS2rRegisterTemplates();
	if (error != UUcError_None) { goto exit; }
	
	// register the at exit callback
	UUrAtExit_Register(SS2rTerminate);
	
	// set the proc handler
	error = 
		TMrTemplate_PrivateData_New(
			SScTemplate_SoundData,
			0,
			SSiSoundData_ProcHandler,
			&SSgTemplate_PrivateData);
	if (error != UUcError_None) { goto exit; }

	// allocate memory for the sound groups
	SSgSoundGroups =
		UUrMemory_Array_New(
			sizeof(SStGroup*),
			64,
			0,
			0);
	if (SSgSoundGroups == NULL) { goto exit; }
	
	// allocate memory for the ambient sounds
	SSgAmbientSounds =
		UUrMemory_Array_New(
			sizeof(SStAmbient*),
			64,
			0,
			0);
	if (SSgAmbientSounds == NULL) { goto exit; }
	
	// allocate memory for the impulse sounds
	SSgImpulseSounds =
		UUrMemory_Array_New(
			sizeof(SStImpulse*),
			64,
			0,
			0);
	if (SSgImpulseSounds == NULL) { goto exit; }
	
	// allocate memory for the dynamically allocated SStSoundData array
	SSgDynamicSoundData =
		UUrMemory_Array_New(
			sizeof(SStSoundData*),
			64,
			0,
			0);
	if (SSgDynamicSoundData == NULL) { goto exit; }
	
	// allocate memory for the dynamic deallocation of SStSoundData
	SSgDeallocatedSoundData =
		UUrMemory_Array_New(
			sizeof(SStSoundData *),
			64,
			0,
			0);
	if (SSgDeallocatedSoundData == NULL) { goto exit; }

	// allocate memory for the playing ambient array
	SSgNumPlayingAmbients = (SSgSoundChannels_NumStereo + SSgSoundChannels_NumMono);
	SSgPlayingAmbient =
		(SStPlayingAmbient*)UUrMemory_Block_NewClear(
			sizeof(SStPlayingAmbient) *
			SSgNumPlayingAmbients);
	if (SSgPlayingAmbient == NULL)
	{
		SSgNumPlayingAmbients = 0;
		goto exit;
	}
	
	for (itr = 0; itr < SSgNumPlayingAmbients; itr++)
	{
		SSgPlayingAmbient[itr].stage = SScPAStage_None;
	}

	// init globals
	SSgPlayID = 0;
	SSgOverallVolume = 1.0f;
	SSgDesiredVolume = SSgOverallVolume;
	MUmVector_Set(SSgListener_Facing, 1.0f, 0.0f, 0.0f);
	
	error =
		SLrGlobalVariable_Register_Bool(
			"sound_show_debug",
			"Displays sound debugging info",
			&SSgShowDebugInfo);
	if (error != UUcError_None) { goto exit; }
	
	SSgPlaybackInitialized = UUcTrue;
	SSgUsable = SSgEnabled;

exit:
	SSrReleaseGuard(SSgGuardAll);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtBool
SS2rEnabled(
	void)
{
	return SSgEnabled;
}

// ----------------------------------------------------------------------
void
SSrLevel_Begin(
	void)
{
	SSiSoundChannels_Silence();
}

// ----------------------------------------------------------------------
void
SSrPlayingChannels_Pause(
	void)
{
	UUtUns32					i;
	SStSoundChannel				*sound_channels;
	UUtUns32					num_channels;
	
	SSrWaitGuard(SSgGuardAll);
	
	if (SSgSoundChannels_Stereo != NULL)
	{
		// pause all playing stereo channels
		sound_channels = SSgSoundChannels_Stereo;
		num_channels = SSgSoundChannels_NumStereo;
		for (i = 0; i < num_channels; i++)
		{
			if (SSiSoundChannel_IsPlaying(&sound_channels[i]) == UUcFalse) { continue; }
			SSiSoundChannel_Pause(&sound_channels[i]);
		}
	}
	
	if (SSgSoundChannels_Mono != NULL)
	{
		// initialize each mono sound channel
		sound_channels = SSgSoundChannels_Mono;
		num_channels = SSgSoundChannels_NumMono;
		for (i = 0; i < num_channels; i++)
		{
			if (SSiSoundChannel_IsPlaying(&sound_channels[i]) == UUcFalse) { continue; }
			SSiSoundChannel_Pause(&sound_channels[i]);
		}
	}
	
	SSrReleaseGuard(SSgGuardAll);
}

// ----------------------------------------------------------------------
void
SSrPlayingChannels_Resume(
	void)
{
	UUtUns32					i;
	SStSoundChannel				*sound_channels;
	UUtUns32					num_channels;
	
	SSrWaitGuard(SSgGuardAll);
	
	if (SSgSoundChannels_Stereo != NULL)
	{
		// pause all playing stereo channels
		sound_channels = SSgSoundChannels_Stereo;
		num_channels = SSgSoundChannels_NumStereo;
		for (i = 0; i < num_channels; i++)
		{
			if ((SSiSoundChannel_IsPlaying(&sound_channels[i]) == UUcFalse) ||
				(SSiSoundChannel_IsPaused(&sound_channels[i]) == UUcFalse))
			{
				continue;
			}
			SSiSoundChannel_Resume(&sound_channels[i]);
		}
	}
	
	if (SSgSoundChannels_Mono != NULL)
	{
		// initialize each mono sound channel
		sound_channels = SSgSoundChannels_Mono;
		num_channels = SSgSoundChannels_NumMono;
		for (i = 0; i < num_channels; i++)
		{
			if ((SSiSoundChannel_IsPlaying(&sound_channels[i]) == UUcFalse) ||
				(SSiSoundChannel_IsPaused(&sound_channels[i]) == UUcFalse))
			{
				continue;
			}
			SSiSoundChannel_Resume(&sound_channels[i]);
		}
	}
	
	SSrReleaseGuard(SSgGuardAll);
}

// ----------------------------------------------------------------------
void
SSrStopAll(
	void)
{
	UUtUns32					i;
	SStPlayingAmbient			*playing_ambient_array;
	UUtUns32					num_playing_ambients;
	SStSoundChannel				*sound_channels;
	UUtUns32					num_channels;
	
	SSrWaitGuard(SSgGuardAll);

	if (SSgPlayingAmbient != NULL)
	{
		// stop playing ambient sounds immediately
		playing_ambient_array = SSgPlayingAmbient;
		num_playing_ambients = SSgNumPlayingAmbients;
		for(i = 0; i < num_playing_ambients; i++)
		{
			SSrAmbient_Halt(playing_ambient_array[i].id);
		}
	}
	
	if (SSgSoundChannels_Stereo != NULL)
	{
		// stop all remaining playing sounds
		sound_channels = SSgSoundChannels_Stereo;
		num_channels = SSgSoundChannels_NumStereo;
		for (i = 0; i < num_channels; i++)
		{
			SSiSoundChannel_Stop(&sound_channels[i]);
			sound_channels[i].sound_data = NULL;
		}
	}
	
	if (SSgSoundChannels_Mono != NULL)
	{
		// initialize each mono sound channel
		sound_channels = SSgSoundChannels_Mono;
		num_channels = SSgSoundChannels_NumMono;
		for (i = 0; i < num_channels; i++)
		{
			SSiSoundChannel_Stop(&sound_channels[i]);
			sound_channels[i].sound_data = NULL;
		}
	}

	SSrReleaseGuard(SSgGuardAll);
}

// ----------------------------------------------------------------------
void UUcExternal_Call
SS2rTerminate(
	void)
{
	SStSoundData				**sound_data_array;
	SStGroup					**sound_group_array;
	SStImpulse					**impulse_sound_array;
	SStAmbient					**ambient_sound_array;
	UUtUns32					i;
	
	SSgUsable = UUcFalse;
	
	if (SSgGuardAll)
	{
		SSrWaitGuard(SSgGuardAll);
	}
	
	// stop the platform-specific thread so it doesn't try to update while we
	// are in the midst of terminating
	SS2rPlatform_TerminateThread();
	
	// terminate the sound channels
	SSiSoundChannels_Terminate();
	
	// delete the group, array
	if (SSgSoundGroups)
	{
		sound_group_array = (SStGroup**)UUrMemory_Array_GetMemory(SSgSoundGroups);
		while (UUrMemory_Array_GetUsedElems(SSgSoundGroups))
		{
			SSrGroup_Delete(sound_group_array[0]->group_name, UUcFalse);
		}
		UUrMemory_Array_Delete(SSgSoundGroups);
		SSgSoundGroups = NULL;
	}
	
	// delete the impulse array
	if (SSgImpulseSounds)
	{
		impulse_sound_array = (SStImpulse**)UUrMemory_Array_GetMemory(SSgImpulseSounds);
		while (UUrMemory_Array_GetUsedElems(SSgImpulseSounds))
		{
			SSrImpulse_Delete(impulse_sound_array[0]->impulse_name, UUcFalse);
		}
		UUrMemory_Array_Delete(SSgImpulseSounds);
		SSgImpulseSounds = NULL;
	}
	
	// delete the ambient array
	if (SSgAmbientSounds)
	{
		ambient_sound_array = (SStAmbient**)UUrMemory_Array_GetMemory(SSgAmbientSounds);
		while (UUrMemory_Array_GetUsedElems(SSgAmbientSounds))
		{
			SSrAmbient_Delete(ambient_sound_array[0]->ambient_name, UUcFalse);
		}
		UUrMemory_Array_Delete(SSgAmbientSounds);
		SSgAmbientSounds = NULL;
	}
	
	// delete the deallocated-pointer array
	// CB: this must come before we delete the sound data array, otherwise
	// we will store ALL of the sound data pointers (which isn't useful).
	if (SSgDeallocatedSoundData)
	{
		UUrMemory_Array_Delete(SSgDeallocatedSoundData);
		SSgDeallocatedSoundData = NULL;
	}

	// delete the dynamic sound data
	if (SSgDynamicSoundData)
	{
		sound_data_array = (SStSoundData**)UUrMemory_Array_GetMemory(SSgDynamicSoundData);
		for (i = 0; i < UUrMemory_Array_GetUsedElems(SSgDynamicSoundData); i++)
		{
			SSiSoundData_Delete(sound_data_array[i]);
			sound_data_array[i] = NULL;
		}
		UUrMemory_Array_Delete(SSgDynamicSoundData);
		SSgDynamicSoundData = NULL;
	}
	
	// delete the playing ambient
	if (SSgPlayingAmbient)
	{
		UUrMemory_Block_Delete(SSgPlayingAmbient);
		SSgPlayingAmbient = NULL;
	}
	
	SSgTemplate_PrivateData = NULL;
	
	SS2rPlatform_Terminate();

	SSgPlaybackInitialized = UUcFalse;
	SSgChannelsInitialized = UUcFalse;
	
	if (SSgGuardAll)
	{
		SSrReleaseGuard(SSgGuardAll);
		SSrDeleteGuard(SSgGuardAll);
		SSgGuardAll = NULL;
	}
}

// ----------------------------------------------------------------------
void
SS2rUpdate(
	void)
{
	if (SSgUsable == UUcFalse) { return; }
	
	SS2iVolume_Update();
	
	SS2rPlatform_PerformanceStartFrame();
	
	SSiPlayingAmbient_UpdateList();
	
	SS2rPlatform_PerformanceEndFrame();
}

// ----------------------------------------------------------------------
float
SS2rVolume_Get(
	void)
{
	// the desired volume is the one that needs to be reported because
	// the overall volume can be in transition between where it was
	// and where it is going which is the desired volume.
	return SSgDesiredVolume;
}

// ----------------------------------------------------------------------
float
SS2rVolume_Set(
	float						inVolume)
{
	float						old_volume;
	
	if (SSgUsable == UUcFalse) { return 0.0f; }
	
	old_volume = SSgOverallVolume;
	inVolume = UUmPin(inVolume, 0.0f, 1.0f);
	SSgDesiredVolume = inVolume;
	
	return old_volume;
}

// ----------------------------------------------------------------------
static void
SS2iVolume_Update(
	void)
{
	float						volume_delta;
	UUtUns32					num_channels;
	SStSoundChannel				*sound_channel_array;
	UUtUns32					i;
	
	if (SSgUsable == UUcFalse) { return; }
	if (SSgOverallVolume == SSgDesiredVolume) { return; }
	
	// calculate the adjustment
	volume_delta = SSgDesiredVolume - SSgOverallVolume;
	if (volume_delta > 0.0f)
	{
		volume_delta = UUmMin(volume_delta, SScVolumeAdjust);
	}
	else if (volume_delta < 0.0f)
	{
		volume_delta = UUmMax(volume_delta, -SScVolumeAdjust);
	}
	
	// adjust the volume
	SSgOverallVolume += volume_delta;
	SSgOverallVolume = UUmPin(SSgOverallVolume, 0.0f, 1.0f);
	
	// adjust the volume of the sound channels
	if (SSgSoundChannels_Stereo)
	{
		num_channels = SSgSoundChannels_NumStereo;
		sound_channel_array = SSgSoundChannels_Stereo;
		for (i = 0; i < num_channels; i++)
		{
			SSiSoundChannel_SetChannelVolume(
				&sound_channel_array[i],
				sound_channel_array[i].channel_volume);
		}
	}

	if (SSgSoundChannels_Mono)
	{
		num_channels = SSgSoundChannels_NumMono;
		sound_channel_array = SSgSoundChannels_Mono;
		for (i = 0; i < num_channels; i++)
		{
			SSiSoundChannel_SetChannelVolume(
				&sound_channel_array[i],
				sound_channel_array[i].channel_volume);
		}
	}
	
}
