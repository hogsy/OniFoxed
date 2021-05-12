/*
	FILE:	BFW_FF_PSD.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 19, 1998
	
	PURPOSE: 
	
	Copyright 1998

*/

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_FileFormat.h"

#include "BFW_FF_PSD_Priv.h"
#include "BFW_FF_PSD.h"
#include "BFW_Image.h"

// ======================================================================
// defines
// ======================================================================
#define FFmMixAlpha(s1,s2)		((((s1) << 24) & 0xFF000000) | (s2))
#define FFmMixRed(s1,s2)		((((s1) << 16) & 0x00FF0000) | (s2))
#define FFmMixGreen(s1,s2)		((((s1) << 8)  & 0x0000FF00) | (s2))
#define FFmMixBlue(s1,s2)		((((s1) << 0)  & 0x000000FF) | (s2))

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
FFiPSD_Read_Raw(
	UUtUns16				inChannels,
	UUtUns32				inHeight,
	UUtUns32				inWidth,
	UUtUns8					*inData,
	UUtUns32				*outData)
{
	UUtUns8					*curPlane;
	UUtUns8					*curSrc;
	UUtUns32				*curDst;
	UUtUns16				x;
	UUtUns16				y;
	
	curPlane = inData;
	
	// Copy the red channel
	for (y = 0; y < inHeight; y++)
	{
		curSrc = curPlane + y * inWidth;
		curDst = outData + y * inWidth;
		
		for (x = 0; x < inWidth; x++)
		{
			*curDst++ = FFmMixRed(*curSrc++, *curDst);
		}
	}
	
	curPlane += inHeight * inWidth;
	
	// Copy the green channel
	for (y = 0; y < inHeight; y++)
	{
		curSrc = curPlane + y * inWidth;
		curDst = outData + y * inWidth;
		
		for (x = 0; x < inWidth; x++)
		{
			*curDst++ = FFmMixGreen(*curSrc++, *curDst);
		}
	}
	
	curPlane += inHeight * inWidth;
	
	// Copy the blue channel
	for (y = 0; y < inHeight; y++)
	{
		curSrc = curPlane + y * inWidth;
		curDst = outData + y * inWidth;
		
		for (x = 0; x < inWidth; x++)
		{
			*curDst++ = FFmMixBlue(*curSrc++, *curDst);
		}
	}
	
	if (inChannels > 3)
	{
		curPlane += inHeight * inWidth * (inChannels - 3);

		// copy the alpha channel
		for (y = 0; y < inHeight; y++)
		{
			curSrc = curPlane + y * inWidth;
			curDst = outData + y * inWidth;
			
			for (x = 0; x < inWidth; x++)
			{
				*curDst++ = FFmMixAlpha(*curSrc++, *curDst);
			}
		}
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
FFiPSD_Read_RLE(
	UUtUns16				inChannels,
	UUtUns32				inHeight,
	UUtUns32				inWidth,
	UUtUns8					*inData,
	UUtUns32				*outData)
{
	UUtInt8					*curSrc;
	UUtUns32				*curDst;
	UUtUns16				*curCount;
	UUtUns16				rows_processed;
	UUtInt16				num_bytes_in_row;
	UUtInt16				counter;
	UUtUns16				itr;
	
	curCount = (UUtUns16*)inData;

	// process red
	curSrc = (UUtInt8*)(inData + inHeight * inChannels * sizeof(UUtUns16));
	curDst = outData;
	
	for (rows_processed = 0; rows_processed < inHeight; rows_processed++)
	{
		// get the number of bytes in the row of data
		num_bytes_in_row = *curCount++;
		UUmSwapBig_2Byte(&num_bytes_in_row);
		
		while (num_bytes_in_row > 0)
		{
			// get the number of characters to process
			counter = *curSrc++;
			
			num_bytes_in_row--;
			UUmAssert(num_bytes_in_row >= 0);
			
			if (counter < 0)
			{
				// write the next byte counter times
				counter = -counter + 1;
				
				num_bytes_in_row--;
				UUmAssert(num_bytes_in_row >= 0);
				
				while (counter--)
					*curDst++ = FFmMixRed(*curSrc, *curDst);
				
				curSrc++;
			}
			else
			{
				// copy the next counter bytes
				counter++;
				
				num_bytes_in_row -= counter;
				UUmAssert(num_bytes_in_row >= 0);
				
				while (counter--)
					*curDst++ = FFmMixRed(*curSrc++, *curDst);
			}
			
			UUmAssert(curDst <= (outData + inHeight * inWidth));
		}
	}

	// process Green
	curDst = outData;
	for (rows_processed = 0; rows_processed < inHeight; rows_processed++)
	{
		// get the number of bytes in the row of data
		num_bytes_in_row = *curCount++;
		UUmSwapBig_2Byte(&num_bytes_in_row);
		
		while (num_bytes_in_row > 0)
		{
			// get the number of characters to process
			counter = *curSrc++;
			
			num_bytes_in_row--;
			UUmAssert(num_bytes_in_row >= 0);
			
			if (counter < 0)
			{
				// write the next byte counter times
				counter = -counter + 1;
				
				num_bytes_in_row--;
				UUmAssert(num_bytes_in_row >= 0);
				
				while (counter--)
					*curDst++ = FFmMixGreen(*curSrc, *curDst);
				
				curSrc++;
			}
			else
			{
				// copy the next counter bytes
				counter++;
				
				num_bytes_in_row -= counter;
				UUmAssert(num_bytes_in_row >= 0);
				
				while (counter--)
					*curDst++ = FFmMixGreen(*curSrc++, *curDst);
			}
			
			UUmAssert(curDst <= (outData + inHeight * inWidth));
		}
	}
	
	// process Blue
	curDst = outData;
	for (rows_processed = 0; rows_processed < inHeight; rows_processed++)
	{
		// get the number of bytes in the row of data
		num_bytes_in_row = *curCount++;
		UUmSwapBig_2Byte(&num_bytes_in_row);
		
		while (num_bytes_in_row > 0)
		{
			// get the number of characters to process
			counter = *curSrc++;
			
			num_bytes_in_row--;
			UUmAssert(num_bytes_in_row >= 0);
			
			if (counter < 0)
			{
				// write the next byte counter times
				counter = -counter + 1;
				
				num_bytes_in_row--;
				UUmAssert(num_bytes_in_row >= 0);
				
				while (counter--)
					*curDst++ = FFmMixBlue(*curSrc, *curDst);
				
				curSrc++;
			}
			else
			{
				// copy the next counter bytes
				counter++;
				
				num_bytes_in_row -= counter;
				UUmAssert(num_bytes_in_row >= 0);
				
				while (counter--)
					*curDst++ = FFmMixBlue(*curSrc++, *curDst);
			}
			
			UUmAssert(curDst <= (outData + inHeight * inWidth));
		}
	}
	
	if (inChannels > 3)
	{
		// Hack to get one of chris's files working
		for(itr = 0; itr < inChannels - 4; itr++)
		{
			curDst = outData;
			for (rows_processed = 0; rows_processed < inHeight; rows_processed++)
			{
				// get the number of bytes in the row of data
				num_bytes_in_row = *curCount++;
				UUmSwapBig_2Byte(&num_bytes_in_row);
				
				while (num_bytes_in_row > 0)
				{
					// get the number of characters to process
					counter = *curSrc++;
					
					num_bytes_in_row--;
					UUmAssert(num_bytes_in_row >= 0);
					
					if (counter < 0)
					{
						// write the next byte counter times
						counter = -counter + 1;
						
						num_bytes_in_row--;
						UUmAssert(num_bytes_in_row >= 0);
						
						curSrc++;
					}
					else
					{
						// copy the next counter bytes
						counter++;
						
						num_bytes_in_row -= counter;
						UUmAssert(num_bytes_in_row >= 0);
					}
					
					UUmAssert(curDst <= (outData + inHeight * inWidth));
				}
			}
		}
		
		// process Alpha
		curDst = outData;
		for (rows_processed = 0; rows_processed < inHeight; rows_processed++)
		{
			// get the number of bytes in the row of data
			num_bytes_in_row = *curCount++;
			UUmSwapBig_2Byte(&num_bytes_in_row);
			
			while (num_bytes_in_row > 0)
			{
				// get the number of characters to process
				counter = *curSrc++;
				
				num_bytes_in_row--;
				UUmAssert(num_bytes_in_row >= 0);
				
				if (counter < 0)
				{
					// write the next byte counter times
					counter = -counter + 1;
					
					num_bytes_in_row--;
					UUmAssert(num_bytes_in_row >= 0);
					
					while (counter--)
						*curDst++ = FFmMixAlpha(*curSrc, *curDst);
					
					curSrc++;
				}
				else
				{
					// copy the next counter bytes
					counter++;
					
					num_bytes_in_row -= counter;
					UUmAssert(num_bytes_in_row >= 0);
					
					while (counter--)
						*curDst++ = FFmMixAlpha(*curSrc++, *curDst);
				}
				
				UUmAssert(curDst <= (outData + inHeight * inWidth));
			}
		}
	}
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
FFrWrite_PSD(
	BFtFileRef*		inFileRef,
	IMtPixelType	inPixelType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	IMtMipMap		inMipMap,
	void*			inData)
{
	UUtError	error;
	UUtUns32	totalFileByteSize;
	UUtUns32	imageDataByteSize;
	UUtUns8*	fileData;
	
	UUtUns16	x, y;
	UUtUns32*	curSrc;
	UUtUns8*	curDst;
	
	BFtFile*	file;
	
	UUtUns8*	curPlane;
	
	FFtPSD_FileHeader_Raw*		fileHeader;
	FFtPSD_ColorModeData_Raw*	colorModeData;
	FFtPSD_ImageResources_Raw*	imageResources;
	FFtPSD_LayerAndMask_Raw*	layerAndMask;
	FFtPSD_ImageData_Raw*		imageData;
	
	UUtUns16					rowBytes;
	
	if(inPixelType != IMcPixelType_RGB888)
	{
		UUmError_ReturnOnErrorMsg(
			UUcError_Generic,
			"pixel type is not supported");
	}

	if(inMipMap != IMcNoMipMap)
	{
		return UUcError_Generic;
	}
	
	imageDataByteSize = inWidth * inHeight * 3;
	rowBytes = inWidth << 2;
	
	totalFileByteSize = 
		sizeof(FFtPSD_FileHeader_Raw) + 
		sizeof(FFtPSD_ColorModeData_Raw) + 
		sizeof(FFtPSD_ImageResources_Raw) +
		sizeof(FFtPSD_LayerAndMask_Raw) + 
		sizeof(FFtPSD_ImageData_Raw) + 
		imageDataByteSize;
	
	fileData = UUrMemory_Block_New(totalFileByteSize);
	UUmError_ReturnOnNull(fileData);
	
	fileHeader = (FFtPSD_FileHeader_Raw*)fileData;
	colorModeData = (FFtPSD_ColorModeData_Raw*)((char*)fileHeader + sizeof(FFtPSD_FileHeader_Raw));
	imageResources = (FFtPSD_ImageResources_Raw*)((char*)colorModeData + sizeof(FFtPSD_ColorModeData_Raw));
	layerAndMask = (FFtPSD_LayerAndMask_Raw*)((char*)imageResources + sizeof(FFtPSD_ImageResources_Raw));
	imageData = (FFtPSD_ImageData_Raw*)((char*)layerAndMask + sizeof(FFtPSD_LayerAndMask_Raw));
	
	*(UUtUns32*)fileHeader->signature = FFcPSD_FileSignature;
	*(UUtUns16*)fileHeader->version = FFcPSD_FileVersion;
	fileHeader->reserved[0] = 0;
	fileHeader->reserved[1] = 0;
	fileHeader->reserved[2] = 0;
	fileHeader->reserved[3] = 0;
	fileHeader->reserved[4] = 0;
	fileHeader->reserved[5] = 0;
	*(UUtUns16*)fileHeader->channels = 3;
	*(UUtUns32*)fileHeader->rows = (UUtUns32)inHeight;
	*(UUtUns32*)fileHeader->columns = (UUtUns32)inWidth;
	*(UUtUns16*)fileHeader->depth = FFcPSD_ChannelBitDepth_8;
	*(UUtUns16*)fileHeader->mode = FFcPSD_ColorMode_RGB;

	*(UUtUns32*)colorModeData->length = 0;
	
	*(UUtUns32*)imageResources->length = 0;
	
	*(UUtUns32*)layerAndMask->length = 0;
	
	*(UUtUns16*)imageData->compression = FFcPSD_Compression_Raw;
	
	curPlane = (UUtUns8*)imageData->data;
	
	// Copy the red channel
		for(y = 0; y < inHeight; y++)
		{
			curSrc = (UUtUns32*)((UUtUns8*)inData + y * rowBytes);
			curDst = (UUtUns8*)(curPlane + y * inWidth);
			
			for(x = 0; x < inWidth; x++)
			{
				*curDst++ = (UUtUns8)((*curSrc++ >> 16) & 0xFF);
			}
		}
	
	curPlane += inHeight * inWidth;
	
	// Copy the green channel
		for(y = 0; y < inHeight; y++)
		{
			curSrc = (UUtUns32*)((UUtUns8*)inData + y * rowBytes);
			curDst = (UUtUns8*)(curPlane + y * inWidth);
			
			for(x = 0; x < inWidth; x++)
			{
				*curDst++ = (UUtUns8)((*curSrc++ >> 8) & 0xFF);
			}
		}
	
	curPlane += inHeight * inWidth;
	
	// Copy the blue channel
		for(y = 0; y < inHeight; y++)
		{
			curSrc = (UUtUns32*)((UUtUns8*)inData + y * rowBytes);
			curDst = (UUtUns8*)(curPlane + y * inWidth);
			
			for(x = 0; x < inWidth; x++)
			{
				*curDst++ = (UUtUns8)((*curSrc++ >> 0) & 0xFF);
			}
		}
	
	// Byte swap if needed
	
	#if UUmEndian == UUmEndian_Little
	
		UUrSwap_4Byte(&fileHeader->signature);
		UUrSwap_2Byte(&fileHeader->version);
		UUrSwap_2Byte(&fileHeader->channels);
		UUrSwap_4Byte(&fileHeader->rows);
		UUrSwap_4Byte(&fileHeader->columns);
		UUrSwap_2Byte(&fileHeader->depth);
		UUrSwap_2Byte(&fileHeader->mode);
	
		UUrSwap_4Byte(&colorModeData->length);
		UUrSwap_4Byte(&imageResources->length);
		UUrSwap_4Byte(&layerAndMask->length);
		UUrSwap_2Byte(&imageData->compression);

	#endif
	
	// Delete any old inFileRef
		BFrFile_Delete(inFileRef);
	
	// Create the file ref
		error = BFrFile_Create(inFileRef);
		UUmError_ReturnOnError(error);
	
	// Open the file
		error = BFrFile_Open(inFileRef, "w", &file);
		UUmError_ReturnOnError(error);
	
	// Write the file
		error = BFrFile_Write(file, totalFileByteSize, fileData);
		UUmError_ReturnOnError(error);
	
	// Close the file
		BFrFile_Close(file);
	
	// Delete the temp memoruy
		UUrMemory_Block_Delete(fileData);
		
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtBool
FFrType_IsPSD(
	BFtFileRef*		inFileRef)
{
	UUtError	error;
	
	const char*		suffix;
	UUtUns32	type;
	BFtFile*	file;
	
	suffix = BFrFileRef_GetSuffixName(inFileRef);
	
	if(suffix == NULL) return UUcFalse;
	
	if(UUrString_Compare_NoCase(suffix, "psd"))	return UUcFalse;
	
	error = BFrFile_Open(inFileRef, "r", &file);
	if(error != UUcError_None) return UUcFalse;
	
	error = BFrFile_Read(file, 4, &type);
	if(error != UUcError_None) return UUcFalse;
	
	#if UUmEndian == UUmEndian_Little
	
		UUrSwap_4Byte(&type);
	
	#endif
	
	BFrFile_Close(file);
	
	if(type != FFcPSD_FileSignature) return UUcFalse;
	
	return UUcTrue;
}
	
// ----------------------------------------------------------------------
UUtError
FFrRead_PSD(
	BFtFileRef*		inFileRef,
	IMtPixelType	inExpectedPixelType,// Will return error if file is not this pixel type
	UUtUns16		*outWidth,
	UUtUns16		*outHeight,
	IMtMipMap		*outMipMap,
	void*			*outData)			// Caller is responsible for freeing this memory
{
	UUtError		error;
	BFtFile*		file;
	UUtUns32		fileLength;
	UUtUns8*		fileData;

	FFtPSD_FileHeader_Raw*		fileHeader;
	FFtPSD_ColorModeData_Raw*	colorModeData;
	FFtPSD_ImageResources_Raw*	imageResources;
	FFtPSD_LayerAndMask_Raw*	layerAndMask;
	FFtPSD_ImageData_Raw*		imageData;
	
	UUtUns32					signature;
	UUtUns16					version;
	UUtUns16					channels;
	UUtUns32					rows;
	UUtUns32					columns;
	FFtPSD_ChannelBitDepth		depth;
	FFtPSD_ColorMode			mode;
	FFtPSD_Compression			compression;
	UUtUns16					temp;
	UUtUns32					*data;
	UUtUns32					length;
	
	IMtPixelType				src_pixel_type;
	void						*out_data;
	
	// read the file data into memory
	error = BFrFile_Open(inFileRef, "r", &file);
	UUmError_ReturnOnError(error);
	
	error = BFrFile_GetLength(file, &fileLength);
	UUmError_ReturnOnError(error);
	
	if (fileLength == 0)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Can not process this PSD file");
	}
	
	fileData = UUrMemory_Block_New(fileLength);
	UUmError_ReturnOnNull(fileData);
	
	error = BFrFile_Read(file, fileLength, fileData);
	UUmError_ReturnOnError(error);
	
	BFrFile_Close(file);
	file = NULL;
	
	// get pointers to the various parts of the file
	fileHeader = (FFtPSD_FileHeader_Raw*)fileData;
	
	signature	= *((UUtUns32*)fileHeader->signature);
	version		= *((UUtUns16*)fileHeader->version);
	UUmSwapBig_4Byte(&signature);
	UUmSwapBig_2Byte(&version);
		
	colorModeData =
		(FFtPSD_ColorModeData_Raw*)((char*)fileHeader +
		sizeof(FFtPSD_FileHeader_Raw));
	
	length = *((UUtUns32*)colorModeData->length);
	UUmSwapBig_4Byte(&length);
	imageResources =
		(FFtPSD_ImageResources_Raw*)((char*)colorModeData +
		sizeof(FFtPSD_ColorModeData_Raw) +
		length);
	
	length = *((UUtUns32*)imageResources->length);
	UUmSwapBig_4Byte(&length);
	layerAndMask =
		(FFtPSD_LayerAndMask_Raw*)((char*)imageResources +
		sizeof(FFtPSD_ImageResources_Raw) +
		length);
	
	length = *((UUtUns32*)layerAndMask->length);
	UUmSwapBig_4Byte(&length);
	imageData =
		(FFtPSD_ImageData_Raw*)((char*)layerAndMask +
		sizeof(FFtPSD_LayerAndMask_Raw) +
		length);
	
	// check the signature
	if (signature != FFcPSD_FileSignature)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "PSD file signature is invalid");
	}

	// check the version number
	if (version != FFcPSD_FileVersion)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "PSD file version is invalid");
	}
	
	// get info about the file
	channels	= *((UUtUns16*)fileHeader->channels);
	rows		= *((UUtUns32*)fileHeader->rows);
	columns		= *((UUtUns32*)fileHeader->columns);
	temp		= *((UUtUns16*)fileHeader->depth);			depth = (FFtPSD_ChannelBitDepth)temp;
	temp		= *((UUtUns16*)fileHeader->mode);			mode = (FFtPSD_ColorMode)temp;
	temp		= *((UUtUns16*)imageData->compression);		compression = (FFtPSD_Compression)temp;
	
	// byte swap if needed
	UUmSwapBig_2Byte(&channels);
	UUmSwapBig_4Byte(&rows);
	UUmSwapBig_4Byte(&columns);
	UUmSwapBig_2Byte(&depth);
	UUmSwapBig_2Byte(&mode);
	UUmSwapBig_2Byte(&compression);
	
	// make sure the channel bit depth is supported
	if ((depth == FFcPSD_ChannelBitDepth_1) || (depth == FFcPSD_ChannelBitDepth_16))
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "this channel depth is not supported.");
	}
	
	// make sure the color mode is supported
	if (mode != FFcPSD_ColorMode_RGB)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "this color mode is not supported.");
	}
	
	// make sure the number of channels is consistent
	if (channels != 3 && channels != 4)
	{
		UUmError_ReturnOnErrorMsg(
			UUcError_Generic,
			"the number of channels in this psd file is not supported.");
	}
	
	// allocate space for the image
	data = (UUtUns32*)UUrMemory_Block_NewClear(rows * columns * sizeof(UUtUns32));
	UUmError_ReturnOnNull(data);
	
	// read the image data
	switch (compression)
	{
		case FFcPSD_Compression_Raw:
			error = FFiPSD_Read_Raw(channels, rows, columns, (UUtUns8*)imageData->data, data);
		break;
		
		case FFcPSD_Compression_RLE:
			error = FFiPSD_Read_RLE(channels, rows, columns, (UUtUns8*)imageData->data, data);
		break;
	}
	UUmError_ReturnOnErrorMsg(error, "Error reading image data from PSD file");
	
	// set the src_pixel_type
	if (channels == 3)
		src_pixel_type = IMcPixelType_RGB888;
	else
		src_pixel_type = IMcPixelType_ARGB8888;
	
	// allocate memory for the new image
	out_data =
		UUrMemory_Block_NewClear(
			rows *
			IMrImage_ComputeRowBytes(inExpectedPixelType, (UUtUns16)columns));
	UUmError_ReturnOnNull(out_data);
	
	// convert the image to the desired pixel type
	error =
		IMrImage_ConvertPixelType(
			IMcDitherMode_On,
			(UUtUns16)columns,
			(UUtUns16)rows,
			IMcNoMipMap,
			src_pixel_type,
			data,
			inExpectedPixelType,
			out_data);
	UUmError_ReturnOnError(error);
	
	// free the data block
	UUrMemory_Block_Delete(data);
		UUrMemory_Block_Delete(fileData);
	
	// set the outgoing values
	*outWidth		= (UUtUns16)columns;
	*outHeight		= (UUtUns16)rows;
	*outMipMap		= IMcNoMipMap;
	*outData		= out_data;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
FFrPeek_PSD(
	BFtFileRef*		inFileRef,
	FFtFileInfo*	outFileInfo)
{
	UUtError		error;
	BFtFile*		file;
	UUtUns32		fileLength;

	FFtPSD_FileHeader_Raw	fileHeader;

	// read the file data into memory
	error = BFrFile_Open(inFileRef, "r", &file);
	UUmError_ReturnOnError(error);
	
	error = BFrFile_GetLength(file, &fileLength);
	UUmError_ReturnOnError(error);
	
	if (fileLength < sizeof(FFtPSD_FileHeader_Raw))
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Can not process this PSD file");
	}
	
	error = BFrFile_Read(file, sizeof(FFtPSD_FileHeader_Raw), &fileHeader);
	UUmError_ReturnOnError(error);
	
	BFrFile_Close(file);
	file = NULL;

	// swap the data in the header
	UUmSwapBig_4Byte(fileHeader.signature);
	UUmSwapBig_2Byte(fileHeader.version);
	UUmSwapBig_2Byte(fileHeader.channels);
	UUmSwapBig_4Byte(fileHeader.rows);
	UUmSwapBig_4Byte(fileHeader.columns);
	UUmSwapBig_2Byte(fileHeader.depth);
	UUmSwapBig_2Byte(fileHeader.mode);
	
	// check the attributes of the file
	if (*((UUtUns32*)fileHeader.signature) != FFcPSD_FileSignature)
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "PSD file signature is invalid");
	if (*((UUtUns16*)fileHeader.version) != FFcPSD_FileVersion)
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "PSD file version is invalid");
	if (*((UUtUns16*)fileHeader.depth) != FFcPSD_ChannelBitDepth_8)
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "PSD file depth is invalid");
	if (*((UUtUns16*)fileHeader.mode) != FFcPSD_ColorMode_RGB)
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "PSD file mode is invalid");
	
	outFileInfo->format = FFcFormat_2D_PSD;
	outFileInfo->width = (UUtUns16)(*((UUtUns32*)fileHeader.columns));
	outFileInfo->height = (UUtUns16)(*((UUtUns32*)fileHeader.rows));
	outFileInfo->mipMap = IMcNoMipMap;
	outFileInfo->pixelType = IMcPixelType_ARGB8888;
	
	return UUcError_None;
}
