#pragma once

/*
	FILE:	BFW_FileFormat.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 19, 1998
	
	PURPOSE: Interface to various graphical file formats
	
	Copyright 1998

*/
#ifndef BFW_FILEFORMAT_H
#define BFW_FILEFORMAT_H

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_Image.h"

typedef enum FFtFormat_2D
{
	FFcFormat_2D_None,
	FFcFormat_2D_BMP,
	FFcFormat_2D_PSD,
	FFcFormat_2D_DDS

} FFtFormat_2D;

typedef struct FFtFileInfo
{
	FFtFormat_2D	format;
	UUtUns16		width;
	UUtUns16		height;
	IMtPixelType	pixelType;
	IMtMipMap		mipMap;
} FFtFileInfo;

/*
 * These functions follow the image rules defined in BFW_Image.h
 */

UUtError
FFrWrite_2D(
	BFtFileRef*		inFileRef,
	FFtFormat_2D	inFormat,
	IMtPixelType	inPixelType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	IMtMipMap		inMipMap,
	void*			inData);

UUtError
FFrRead_2D(
	BFtFileRef*		inFileRef,
	IMtPixelType	inPixelType,
	UUtUns16		*outWidth,
	UUtUns16		*outHeight,
	IMtMipMap		*outMipMap,
	void*			*outData);		// Caller is responsible for freeing this memory

UUtError
FFrRead_2D_Reduce(
	BFtFileRef*		inFileRef,
	IMtPixelType	inPixelType,
	UUtUns32		inReduce,
	IMtScaleMode	inScaleMode,
	UUtUns16		*outWidth,
	UUtUns16		*outHeight,
	IMtMipMap		*outMipMap,
	void			**outData);			// Caller is responsible for freeing this memory

UUtError
FFrPeek_2D(
	BFtFileRef*		inFileRef,
	FFtFileInfo*	outFileInfo);

#endif /* BFW_FILEFORMAT_H */
