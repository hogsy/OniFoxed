/*
	FILE:	BFW_FF_PSD_Priv.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 19, 1998
	
	PURPOSE: 
	
	Copyright 1998

*/

#ifndef BFW_FF_PRIV_PSD_H
#define BFW_FF_PRIV_PSD_H

#include "BFW.h"

#define FFcPSD_FileSignature	UUm4CharToUns32('8', 'B', 'P', 'S')
#define FFcPSD_FileVersion		(1)

typedef enum FFtPSD_ChannelBitDepth
{
	FFcPSD_ChannelBitDepth_1	= 1,
	FFcPSD_ChannelBitDepth_8	= 8,
	FFcPSD_ChannelBitDepth_16	= 16
	
	
} FFtPSD_ChannelBitDepth;

typedef enum FFtPSD_ColorMode
{
	FFcPSD_ColorMode_Bitmap			= 0,
	FFcPSD_ColorMode_Grayscale		= 1,
	FFcPSD_ColorMode_Indexed		= 2,
	FFcPSD_ColorMode_RGB			= 3,
	FFcPSD_ColorMode_CMYK			= 4,
	FFcPSD_ColorMode_Multichannel	= 5,
	FFcPSD_ColorMode_Duotone		= 6,
	FFcPSD_ColorMode_Lab			= 7
	
} FFtPSD_ColorMode;

typedef enum FFtPSD_Compression
{
	FFcPSD_Compression_Raw			= 0,
	FFcPSD_Compression_RLE			= 1
	
} FFtPSD_Compression;


typedef struct FFtPSD_FileHeader_Raw
{
	char	signature[4];	// Always equal to FFcPSD_FileSignature
	char	version[2];		// Always equal to FFcPSD_FileVersion
	char	reserved[6];	// Always equal to 0
	char	channels[2];	// Number of channels - 1 to 24
	char	rows[4];		// Height of the image
	char	columns[4];		// Width of the image
	char	depth[2];		// Number of bits per channel - FFtPSD_ChannelBitDepth
	char	mode[2];		// Color mode - FFtPSD_ColorMode
	
} FFtPSD_FileHeader_Raw;

typedef struct FFtPSD_ColorModeData_Raw
{
	char	length[4];
	char	colorData[0];
	
} FFtPSD_ColorModeData_Raw;

typedef struct FFtPSD_ImageResources_Raw
{
	char	length[4];
	char	resources[0];
	
} FFtPSD_ImageResources_Raw;

typedef struct FFtPSD_LayerAndMask_Raw
{
	char	length[4];
	char	layers[0];
	//char	globalLayerMask[];
	
} FFtPSD_LayerAndMask_Raw;

typedef struct FFtPSD_ImageData_Raw
{
	char	compression[2];		// FFtPSD_Compression
	char	data[0];
	
} FFtPSD_ImageData_Raw;


#endif /* BFW_FF_PRIV_PSD_H */
