/*
	FILE:	BFW_FF_BMP.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 19, 1998
	
	PURPOSE: 
	
	Copyright 1998

*/

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_FileFormat.h"

#include "BFW_FF_BMP_Priv.h"
#include "BFW_FF_BMP.h"

UUtError
FFrWrite_BMP(
	BFtFileRef*		inFileRef,
	IMtPixelType	inPixelType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	IMtMipMap		inMipMap,
	void*			inData)
{
	UUtError			error;
	
	BFtFile*			file;
	
	BMPtFileHeader_Raw*	fileHeaderRaw;
	BMPtHeaderOld_Raw*	headerRaw;
	
	UUtUns32			dataSize;
	UUtUns32			fileSize;
	
	UUtUns16			x, y;
	
	UUtUns8*			fileMemory;
	
	UUtUns32			dstRowBytes;
	UUtUns8*			dstBaseAddr;

	UUtUns32			bitsPerPlane;
	UUtUns32			bytesPerPlane;
	
	if ((inPixelType != IMcPixelType_RGB555) && (inPixelType != IMcPixelType_RGB888))
	{
		return UUcError_Generic;
	}

	if(inMipMap != IMcNoMipMap)
	{
		return UUcError_Generic;
	}

	if (IMcPixelType_RGB555 == inPixelType) {
		bitsPerPlane = 16;
	}
	else if (IMcPixelType_RGB888 == inPixelType) {
		bitsPerPlane = 24;
	}
	else {
		UUmAssert(0);
	}

	UUmAssert(0 == (bitsPerPlane % 8));
	bytesPerPlane = bitsPerPlane / 8;

	dstRowBytes = UUmMakeMultiple(inWidth * bytesPerPlane, 4);
	
	dataSize = inHeight * dstRowBytes;
	fileSize = dataSize + sizeof(BMPtFileHeader_Raw) + sizeof(BMPtHeaderOld_Raw);
	
	fileMemory = UUrMemory_Block_New(fileSize);
	UUmError_ReturnOnNull(fileMemory);
	
	fileHeaderRaw = (BMPtFileHeader_Raw*)fileMemory;
	headerRaw = (BMPtHeaderOld_Raw*)(fileMemory + sizeof(BMPtFileHeader_Raw));
	
	fileHeaderRaw->type[0] = ((FFcBMP_Type_BMP >> 8) & 0xFF);
	fileHeaderRaw->type[1] = ((FFcBMP_Type_BMP >> 0) & 0xFF);
	*(UUtUns32*)&fileHeaderRaw->size = fileSize;
		
	*(UUtUns16*)&fileHeaderRaw->xHotSpot = 0;
	*(UUtUns16*)&fileHeaderRaw->yHotSpot = 0;
	*(UUtUns32*)&fileHeaderRaw->offsetToBits = sizeof(BMPtFileHeader_Raw) + sizeof(BMPtHeaderOld_Raw);
	
	UUmSwapLittle_4Byte(&fileHeaderRaw->size);
	UUmSwapLittle_2Byte(&fileHeaderRaw->xHotSpot);
	UUmSwapLittle_2Byte(&fileHeaderRaw->yHotSpot);
	UUmSwapLittle_4Byte(&fileHeaderRaw->offsetToBits);
	
	*(UUtUns32*)&headerRaw->size = sizeof(BMPtHeaderOld_Raw);
	*(UUtUns16*)&headerRaw->width = inWidth;
	*(UUtUns16*)&headerRaw->height = inHeight;
	*(UUtUns16*)&headerRaw->numBitPlanes = 1;
	*(UUtUns16*)&headerRaw->numBitsPerPlane = (UUtUns16) bitsPerPlane;
	
	UUmSwapLittle_4Byte(&headerRaw->size);
	UUmSwapLittle_2Byte(&headerRaw->width);
	UUmSwapLittle_2Byte(&headerRaw->height);
	UUmSwapLittle_2Byte(&headerRaw->numBitPlanes);
	UUmSwapLittle_2Byte(&headerRaw->numBitsPerPlane);
	
	dstBaseAddr = fileMemory + sizeof(BMPtFileHeader_Raw) + sizeof(BMPtHeaderOld_Raw);

	if (IMcPixelType_RGB555 == inPixelType)
	{
		UUtUns8*			curDst;
		UUtUns16*			curSrc;

		for(y = 0; y < inHeight; y++)
		{
			curSrc = (UUtUns16*)inData + inWidth * y;
			curDst = dstBaseAddr + dstRowBytes * (inHeight - y - 1);
			
			for(x = 0; x < inWidth; x++)
			{
				*(UUtUns16*)curDst = *curSrc++;
				UUmSwapLittle_2Byte(curDst);
				curDst += 2;
			}
		}
	}
	else if (IMcPixelType_RGB888 == inPixelType)
	{
		UUtUns8*			curDst;
		UUtUns32*			curSrc;

		for(y = 0; y < inHeight; y++)
		{
			curSrc = (UUtUns32*)inData + inWidth * y;
			curDst = dstBaseAddr + dstRowBytes * (inHeight - y - 1);
			
			for(x = 0; x < inWidth; x++)
			{
				*curDst++ = (UUtUns8) ((*curSrc & 0x000000ff) >> 0);
				*curDst++ = (UUtUns8) ((*curSrc & 0x0000ff00) >> 8);
				*curDst++ = (UUtUns8) ((*curSrc & 0x00ff0000) >> 16);

				curSrc++;
			}
		}
	}
	
	BFrFile_Delete(inFileRef);
	
	error = BFrFile_Create(inFileRef);
	UUmError_ReturnOnError(error);
	
	error = BFrFile_Open(inFileRef, "w", &file);
	UUmError_ReturnOnError(error);
	
	error = BFrFile_Write(file, fileSize, fileMemory);
	UUmError_ReturnOnError(error);
	
	BFrFile_Close(file);
	
	UUrMemory_Block_Delete(fileMemory);
	
	return UUcError_None;
}

UUtBool
FFrType_IsBMP(
	BFtFileRef*		inFileRef)
{
	UUtError	error;
	
	const char*		suffix;
	char		type[2];
	BFtFile*	file;
	
	suffix = BFrFileRef_GetSuffixName(inFileRef);
	
	if(suffix == NULL) return UUcFalse;
	
	if(UUrString_Compare_NoCase(suffix, "bmp"))	return UUcFalse;
	
	error = BFrFile_Open(inFileRef, "r", &file);
	if(error != UUcError_None) return UUcFalse;
	
	error = BFrFile_Read(file, 2, &type);
	if(error != UUcError_None) return UUcFalse;
	
	BFrFile_Close(file);

	if(type[0] != ((FFcBMP_Type_BMP >> 8) & 0xFF)) return UUcFalse;
	if(type[1] != ((FFcBMP_Type_BMP >> 0) & 0xFF)) return UUcFalse;
	
	return UUcTrue;
}
	
UUtError
FFrRead_BMP(
	BFtFileRef*		inFileRef,
	IMtPixelType	inPixelType,	// Desired pixel type
	UUtUns16		*outWidth,
	UUtUns16		*outHeight,
	IMtMipMap		*outMipMap,
	void*			*outData)		// Caller is responsible for freeing this memory
{
	UUtError	error;
	BFtFile*	file;
	UUtUns32	fileLength;
	UUtUns8*	fileData;
	
	UUtUns16	width;
	UUtUns16	height;
	UUtUns16	rowBytes;
	UUtUns8*	imageData;
	UUtUns16	x, y;
	UUtUns8		r, g, b;
	
	BMPtFileHeader_Raw*		fileHeaderRaw;
	BMPtHeaderOld_Raw*		headerOldRaw;
	BMPtHeaderNew_Raw*		headerNewRaw;
	BMPtHeader				header;
	
	UUtUns8*			srcBaseAddr;
	UUtUns16			srcRowBytes;
	UUtUns8*			curSrc;
	UUtUns8*			curDst;
	UUtUns16			bitsPerPlane;

	void				*out_data;
		
	error = BFrFile_Open(inFileRef, "r", &file);
	UUmError_ReturnOnError(error);
	
	error = BFrFile_GetLength(file, &fileLength);
	UUmError_ReturnOnError(error);
	
	if(fileLength < (sizeof(BMPtFileHeader_Raw) + sizeof(BMPtHeaderOld_Raw)))
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Can not process this bmp file");
	}
	
	fileData = UUrMemory_Block_New(fileLength);
	UUmError_ReturnOnNull(fileData);
	
	error = BFrFile_Read(file, fileLength, fileData);
	UUmError_ReturnOnError(error);
	
	BFrFile_Close(file);
	file = NULL;
	
	fileHeaderRaw = (BMPtFileHeader_Raw*)fileData;
	
	#if UUmEndian == UUmEndian_Big
	
		UUrSwap_4Byte(&fileHeaderRaw->size);
		UUrSwap_2Byte(&fileHeaderRaw->xHotSpot);
		UUrSwap_2Byte(&fileHeaderRaw->yHotSpot);
		UUrSwap_4Byte(&fileHeaderRaw->offsetToBits);
		
	#endif
	
	UUrMemory_Clear(&header, sizeof(BMPtHeader));
	
	headerOldRaw = (BMPtHeaderOld_Raw*)(fileData + sizeof(BMPtFileHeader_Raw));
	
	#if UUmEndian == UUmEndian_Big
	
		UUrSwap_4Byte(&headerOldRaw->size);
	
	#endif
	
	header.size = *(UUtUns32*)headerOldRaw->size;

	if(header.size <= 12)
	{
		UUmAssert(header.size == sizeof(BMPtHeaderOld_Raw));

		#if UUmEndian == UUmEndian_Big
		
			UUrSwap_2Byte(&headerOldRaw->width);
			UUrSwap_2Byte(&headerOldRaw->height);
			UUrSwap_2Byte(&headerOldRaw->numBitPlanes);
			UUrSwap_2Byte(&headerOldRaw->numBitsPerPlane);

		#endif
		
		header.width = *(UUtUns16*)headerOldRaw->width;
		header.height = *(UUtUns16*)headerOldRaw->height;
		header.numBitPlanes = *(UUtUns16*)headerOldRaw->numBitPlanes;
		header.numBitsPerPlane = *(UUtUns16*)headerOldRaw->numBitsPerPlane;
	}
	else
	{
		headerNewRaw = (BMPtHeaderNew_Raw*)headerOldRaw;
		
		#if UUmEndian == UUmEndian_Big
		
			UUrSwap_4Byte(&headerNewRaw->width);
			UUrSwap_4Byte(&headerNewRaw->height);
			UUrSwap_2Byte(&headerNewRaw->numBitPlanes);
			UUrSwap_2Byte(&headerNewRaw->numBitsPerPlane);
			if(header.size <= 16) goto doneSwapping;
			UUrSwap_4Byte(&headerNewRaw->compressionScheme);
			if(header.size <= 20) goto doneSwapping;
			UUrSwap_4Byte(&headerNewRaw->sizeOfImageData);
			if(header.size <= 24) goto doneSwapping;
			UUrSwap_4Byte(&headerNewRaw->xResolution);
			if(header.size <= 28) goto doneSwapping;
			UUrSwap_4Byte(&headerNewRaw->yResolution);
			if(header.size <= 32) goto doneSwapping;
			UUrSwap_4Byte(&headerNewRaw->numColorsUsed);
			if(header.size <= 36) goto doneSwapping;
			UUrSwap_4Byte(&headerNewRaw->numImportantColors);
			if(header.size <= 40) goto doneSwapping;
			UUrSwap_2Byte(&headerNewRaw->resolutionUnits);
			if(header.size <= 42) goto doneSwapping;
			UUrSwap_2Byte(&headerNewRaw->padding);
			if(header.size <= 44) goto doneSwapping;
			UUrSwap_2Byte(&headerNewRaw->origin);
			if(header.size <= 46) goto doneSwapping;
			UUrSwap_2Byte(&headerNewRaw->halftoning);
			if(header.size <= 48) goto doneSwapping;
			UUrSwap_4Byte(&headerNewRaw->halftoningParam1);
			if(header.size <= 52) goto doneSwapping;
			UUrSwap_4Byte(&headerNewRaw->halftoningParam2);
			if(header.size <= 56) goto doneSwapping;
			UUrSwap_4Byte(&headerNewRaw->colorEncoding);
			if(header.size <= 60) goto doneSwapping;
			UUrSwap_4Byte(&headerNewRaw->identifier);
doneSwapping:
			
		#endif
		
		header.width = *(UUtUns32*)headerNewRaw->width;
		header.height = *(UUtUns32*)headerNewRaw->height;
		header.numBitPlanes = *(UUtUns16*)headerNewRaw->numBitPlanes;
		header.numBitsPerPlane = *(UUtUns16*)headerNewRaw->numBitsPerPlane;
		if(header.size <= 16) goto next;
		header.compressionScheme = *(UUtUns32*)headerNewRaw->compressionScheme;
		if(header.size <= 20) goto next;
		header.sizeOfImageData = *(UUtUns32*)headerNewRaw->sizeOfImageData;
		if(header.size <= 24) goto next;
		header.xResolution = *(UUtUns32*)headerNewRaw->xResolution;
		if(header.size <= 28) goto next;
		header.yResolution = *(UUtUns32*)headerNewRaw->yResolution;
		if(header.size <= 32) goto next;
		header.numColorsUsed = *(UUtUns32*)headerNewRaw->numColorsUsed;
		if(header.size <= 36) goto next;
		header.numImportantColors = *(UUtUns32*)headerNewRaw->numImportantColors;
		if(header.size <= 40) goto next;
		header.resolutionUnits = *(UUtUns16*)headerNewRaw->resolutionUnits;
		if(header.size <= 42) goto next;
		header.padding = *(UUtUns16*)headerNewRaw->padding;
		if(header.size <= 44) goto next;
		header.origin = *(UUtUns16*)headerNewRaw->compressionScheme;
		if(header.size <= 46) goto next;
		header.halftoning = *(UUtUns16*)headerNewRaw->halftoning;
		if(header.size <= 48) goto next;
		header.halftoningParam1 = *(UUtUns32*)headerNewRaw->halftoningParam1;
		if(header.size <= 52) goto next;
		header.halftoningParam2 = *(UUtUns32*)headerNewRaw->halftoningParam2;
		if(header.size <= 56) goto next;
		header.colorEncoding = *(UUtUns32*)headerNewRaw->colorEncoding;
		if(header.size <= 60) goto next;
		header.identifier = *(UUtUns32*)headerNewRaw->identifier;
	}
next:
	
	if((fileHeaderRaw->type[0] != ((FFcBMP_Type_BMP >> 8) & 0xFF)) ||
		(fileHeaderRaw->type[1] != ((FFcBMP_Type_BMP >> 0) & 0xFF)))
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Can not process this bmp file");
	}

	if((header.compressionScheme != FFcBMP_Compression_None) ||
		(header.origin != FFcBMP_Origin_LOWER_LEFT) ||
		(header.colorEncoding != FFcBMP_Color_Encoding_RGB))
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Can not process this bmp file");
	}
	
	bitsPerPlane = header.numBitsPerPlane;
	
	if(bitsPerPlane != 16 && bitsPerPlane != 24)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Can not process this bmp file");
	}
	
	UUmAssert(header.width > 0);
	UUmAssert(header.height > 0);
	
	width = (UUtUns16)header.width;
	height = (UUtUns16)header.height;
	rowBytes = IMrImage_ComputeRowBytes(IMcPixelType_RGB555, width);

	// allocate memory for the new image
	out_data =
		UUrMemory_Block_New(IMrImage_ComputeSize(inPixelType, IMcNoMipMap, width, height));
	UUmError_ReturnOnNull(out_data);
	

	if(inPixelType != IMcPixelType_RGB555)
	{
		imageData = UUrMemory_Block_New(height * rowBytes);
		UUmError_ReturnOnNull(imageData);
	}
	else
	{
		imageData = out_data;
	}
	
	srcBaseAddr = (UUtUns8*)fileHeaderRaw + *(UUtUns32*)fileHeaderRaw->offsetToBits;
	
	if(bitsPerPlane == 16)
	{
		srcRowBytes = UUmMakeMultiple(width << 1, 4);
		
		for(y = 0; y < height; y++)
		{
			curSrc = srcBaseAddr + srcRowBytes * (height - y - 1);
			curDst = imageData + rowBytes * y;
			
			for(x = 0; x < width; x++)
			{
				*(UUtUns16*)curDst = *(UUtUns16*)curSrc;
				curDst += 2;
				curSrc += 2;
			}
		}
	}
	else
	{
		srcRowBytes = UUmMakeMultiple(width * sizeof(BMPtRGB), 4);
		
		for(y = 0; y < height; y++)
		{
			curSrc = srcBaseAddr + srcRowBytes * (height - y - 1);
			curDst = imageData + rowBytes * y;
			
			for(x = 0; x < width; x++)
			{
				b = *curSrc++;
				g = *curSrc++;
				r = *curSrc++;
				
				r >>= 3;
				g >>= 3;
				b >>= 3;
				
				*(UUtUns16*)curDst = (1 << 15) | (r << 10) | (g << 5) | b;
				curDst += 2;
			}
		}
	}
	
	if(inPixelType != IMcPixelType_RGB555)
	{
		// convert the image to the desired pixel type
		error =
			IMrImage_ConvertPixelType(
				IMcDitherMode_Off,
				width,
				height,
				IMcNoMipMap,
				IMcPixelType_RGB555,
				imageData,
				inPixelType,
				out_data);
		UUmError_ReturnOnError(error);
	}
	
	// free allocated memory
	if(inPixelType != IMcPixelType_RGB555)
	{
		UUrMemory_Block_Delete(imageData);
	}
	
	UUrMemory_Block_Delete(fileData);
	
	// set the outgoing values
	*outWidth = width;
	*outHeight = height;
	*outMipMap = IMcNoMipMap;
	*outData = out_data;
	
	return UUcError_None;
}

UUtError
FFrPeek_BMP(
	BFtFileRef*		inFileRef,
	FFtFileInfo*	outFileInfo)
{
	UUtError		error;
	BFtFile*		file;
	UUtUns32		fileLength;
	UUtUns8			storage[sizeof(BMPtFileHeader_Raw) + sizeof(BMPtHeaderOld_Raw)];
	UUtUns8*		fileData = storage;
	UUtUns16		header_size;
	
	BMPtHeaderOld_Raw	*headerOldRaw;
	
	// calculate the header size
	header_size = sizeof(BMPtFileHeader_Raw) + sizeof(BMPtHeaderOld_Raw);

	// open the file read only
	error = BFrFile_Open(inFileRef, "r", &file);
	UUmError_ReturnOnError(error);
	
	// get the length of the file
	error = BFrFile_GetLength(file, &fileLength);
	UUmError_ReturnOnError(error);
	
	// make sure the file is big enough
	if (fileLength < header_size)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Can not process this bmp file");
	}
		
	// read the headers of the file
	error = BFrFile_Read(file, header_size, fileData);
	UUmError_ReturnOnError(error);
	
	// close the file
	BFrFile_Close(file);
	file = NULL;
	
	// get a pointer to the header
	headerOldRaw = (BMPtHeaderOld_Raw*)(fileData + sizeof(BMPtFileHeader_Raw));

	#if UUmEndian == UUmEndian_Big
	
		UUrSwap_4Byte(&headerOldRaw->size);
	
	#endif
	
	// process the header as old style or new style depending on it size
	if ((UUtUns16)(*(UUtUns32*)&headerOldRaw->size) <= 12)
	{
		UUmAssert((UUtUns16)(*(UUtUns32*)&headerOldRaw->size) == sizeof(BMPtHeaderOld_Raw));

		#if UUmEndian == UUmEndian_Big
		
			UUrSwap_2Byte(&headerOldRaw->width);
			UUrSwap_2Byte(&headerOldRaw->height);

		#endif
		
		outFileInfo->width = *((UUtUns16*)headerOldRaw->width);
		outFileInfo->height = *((UUtUns16*)headerOldRaw->height);
	}
	else
	{
		BMPtHeaderNew_Raw*		headerNewRaw;
	
		headerNewRaw = (BMPtHeaderNew_Raw*)headerOldRaw;
		
		#if UUmEndian == UUmEndian_Big
		
			UUrSwap_4Byte(&headerNewRaw->width);
			UUrSwap_4Byte(&headerNewRaw->height);
		
		#endif
		
		outFileInfo->width = (UUtUns16)(*((UUtUns32*)headerNewRaw->width));
		outFileInfo->height = (UUtUns16)(*((UUtUns32*)headerNewRaw->height));
	}
	
	outFileInfo->format = FFcFormat_2D_BMP;
	outFileInfo->mipMap = IMcNoMipMap;
	outFileInfo->pixelType = IMcPixelType_RGB555;

	return UUcError_None;
}

