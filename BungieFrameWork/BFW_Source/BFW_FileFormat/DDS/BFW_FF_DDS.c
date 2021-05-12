/*
	FILE:	BFW_FF_DDS.c
	
	AUTHOR:	Michael Evans
	
	CREATED: Jan 12, 1999
	
	PURPOSE: 
	
	Copyright 1999

*/

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_FileFormat.h"

#include "BFW_FF_DDS_Priv.h"
#include "BFW_FF_DDS.h"
#include "BFW_Image.h"

static void FFiSwapDDSHeader(FFtDDS_FileHeader_Raw *ioHeader)
{
	//UUmAssert(sizeof(FFtDDS_FileHeader_Raw) == 0x70);

	// swap the data in the header
	UUmSwapLittle_2Byte(&ioHeader->width);
	UUmSwapLittle_2Byte(&ioHeader->height);
	UUmSwapBig_4Byte(&ioHeader->signature);
	UUmSwapBig_4Byte(&ioHeader->format);
}


UUtError
FFrWrite_DDS(
	BFtFileRef*		inFileRef,
	IMtPixelType	inPixelType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	IMtMipMap		inMipMap,
	void*			inData)
{
	return UUcError_None;
}

UUtBool
FFrType_IsDDS(
	BFtFileRef*		inFileRef)
{
	UUtBool		isDDS = UUcFalse;
	UUtError	error;
	
	const char*		suffix;
	UUtUns32	type;
	BFtFile*	file;
	
	suffix = BFrFileRef_GetSuffixName(inFileRef);
	
	if(suffix == NULL) 
	{
		goto exit;
	}
	
	if(UUrString_Compare_NoCase(suffix, "dds"))	
	{
		goto exit;
	}
	
	error = BFrFile_Open(inFileRef, "r", &file);
	if(error != UUcError_None) 
	{
		goto exit;
	}
	
	error = BFrFile_Read(file, 4, &type);
	if(error != UUcError_None) 
	{
		goto exit;
	}
	
	UUmSwapBig_4Byte(&type);
	
	BFrFile_Close(file);
	
	if(type != FFcDDS_FileSignature) 
	{
		goto exit;
	}

	// passed all the tests
	isDDS = UUcTrue;

exit:	
	return isDDS;
}
	
UUtError
FFrRead_DDS(
	BFtFileRef*		inFileRef,
	IMtPixelType	inExpectedPixelType,// Will return error if file is not this pixel type
	UUtUns16		*outWidth,
	UUtUns16		*outHeight,
	IMtMipMap		*outMipMap,
	void*			*outData)			// Caller is responsible for freeing this memory
{
	UUtError				error;
	BFtFile*				file;
	UUtUns32				fileLength;
	FFtDDS_FileHeader_Raw	fileHeader;
	UUtUns32				imageSize;
	IMtMipMap				imageMipMap;

	UUmAssertWritePtr(inFileRef, 1);

	// read the file data into memory
	error = BFrFile_Open(inFileRef, "r", &file);
	UUmError_ReturnOnError(error);
	
	error = BFrFile_GetLength(file, &fileLength);
	UUmError_ReturnOnError(error);
	
	if (fileLength < sizeof(fileHeader))
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Can not process this DDS file");
	}
	
	error = BFrFile_Read(file, sizeof(fileHeader), &fileHeader);
	UUmError_ReturnOnError(error);
	
	FFiSwapDDSHeader(&fileHeader);

	// compute the image properties
	imageMipMap = (fileHeader.mipMapLevels > 1) ? IMcHasMipMap : IMcNoMipMap;
	imageSize = IMrImage_ComputeSize(IMcPixelType_DXT1, imageMipMap, fileHeader.width, fileHeader.height);

	*outData = UUrMemory_Block_New(imageSize);
	UUmError_ReturnOnNull(*outData);

	// compute the data length
	fileLength -= sizeof(fileHeader);

	// read the image data
	UUmAssert(fileLength <= imageSize);
	error = BFrFile_Read(file, fileLength, *outData);
	UUmError_ReturnOnError(error);

	// swap
	{
		UUtUns32 itr;
		UUtUns16 *pSwap;

		for(itr = 0, pSwap = *outData; itr < (fileLength / 2); pSwap++, itr++) {
			UUmSwapLittle_2Byte(pSwap);
		}
	}

	if ((fileHeader.width != fileHeader.height) && (IMcHasMipMap == imageMipMap))
	{
		UUtUns16 src_width = fileHeader.width;
		UUtUns16 src_height = fileHeader.height;
		UUtUns16 dst_width;
		UUtUns16 dst_height;

		UUtUns16 src_555[8];	// max dimension is 8x1
		UUtUns16 dst_555[8];
		char *src_dxt1 = *outData;
		char *dst_dxt1;

		// find the last valid mipmap
		while((src_width > 1) && (src_height > 1)) { 
			src_dxt1 += IMrImage_ComputeSize(IMcPixelType_DXT1, IMcNoMipMap, src_width, src_height);
			
			src_width /= 2; 
			src_height /= 2;
		}

		// convert it to 555
		IMrImage_ConvertPixelType(
			IMcDitherMode_Off,
			src_width,
			src_height,
			IMcNoMipMap,
			IMcPixelType_DXT1,
			src_dxt1,
			IMcPixelType_RGB555,
			src_555);

		// prepare to scale and convert into the remaining levels
		dst_width = src_width;
		dst_height = src_height;
		dst_dxt1 = src_dxt1;

		do
		{
			dst_dxt1 += IMrImage_ComputeSize(IMcPixelType_DXT1, IMcNoMipMap, dst_width, dst_height);
			dst_width = UUmMax(dst_width / 2, 1);
			dst_height = UUmMax(dst_height / 2, 1);

			error = IMrImage_Scale(
				IMcScaleMode_Box,
				src_width,
				src_height,
				IMcPixelType_RGB555,
				src_555,
				dst_width,
				dst_height,
				dst_555);
			UUmAssert(UUcError_None == error);

			IMrImage_ConvertPixelType(
				IMcDitherMode_Off,
				dst_width,
				dst_height,
				IMcNoMipMap,
				IMcPixelType_RGB555,
				dst_555,
				IMcPixelType_DXT1,
				dst_dxt1);
		} while((dst_width > 1) || (dst_height > 1));
	}

	// return the results
	*outWidth = fileHeader.width;
	*outHeight = fileHeader.height;
	*outMipMap = (fileHeader.mipMapLevels > 1) ? IMcHasMipMap : IMcNoMipMap;

	BFrFile_Close(file);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
FFrPeek_DDS(
	BFtFileRef*		inFileRef,
	FFtFileInfo*	outFileInfo)
{
	UUtError		error;
	BFtFile*		file;
	UUtUns32		fileLength;
	FFtDDS_FileHeader_Raw		fileHeader;
	
retry:
	//UUmAssert(sizeof(fileHeader) == 0x70);
	UUmAssertWritePtr(inFileRef, 1);
	UUmAssertWritePtr(outFileInfo, sizeof(*outFileInfo));

	// read the file data into memory
	error = BFrFile_Open(inFileRef, "r", &file);
	UUmError_ReturnOnError(error);
	
	error = BFrFile_GetLength(file, &fileLength);
	UUmError_ReturnOnError(error);
	
	if (fileLength < sizeof(fileHeader))
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Can not process this DDS file");
	}
	
	error = BFrFile_Read(file, sizeof(fileHeader), &fileHeader);
	UUmError_ReturnOnError(error);
	
	FFiSwapDDSHeader(&fileHeader);

	BFrFile_Close(file);

	// verify various bits and pieces
	if (fileHeader.signature != FFcDDS_FileSignature)
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "DDS file signature is invalid");
	
	switch(fileHeader.format)
	{
		case FFcDDS_FormatSignature_DXT1:
		break;

		default:
			{
				AUtMB_ButtonChoice choice = AUrMessageBox(AUcMBType_RetryCancel, "DDS file format is %c%c%c%c expected %c%c%c%c", 
					(fileHeader.format >> 24) & 0xFF,
					(fileHeader.format >> 16) & 0xFF,
					(fileHeader.format >> 8) & 0xFF,
					(fileHeader.format >> 0) & 0xFF,
					(FFcDDS_FormatSignature_DXT1 >> 24) & 0xFF,
					(FFcDDS_FormatSignature_DXT1 >> 16) & 0xFF,
					(FFcDDS_FormatSignature_DXT1 >> 8) & 0xFF,
					(FFcDDS_FormatSignature_DXT1 >> 0) & 0xFF);

				if (AUcMBChoice_Retry == choice) {
					goto retry;
				}
				else {
					return UUcError_Generic;
				}
			}
	}
		
	// setup the return
	outFileInfo->format = FFcFormat_2D_DDS;
	outFileInfo->width = fileHeader.width;
	outFileInfo->height = fileHeader.height;
	outFileInfo->mipMap = (fileHeader.mipMapLevels > 1) ? IMcHasMipMap : IMcNoMipMap;
	outFileInfo->pixelType = IMcPixelType_DXT1;
	
	return UUcError_None;
}
