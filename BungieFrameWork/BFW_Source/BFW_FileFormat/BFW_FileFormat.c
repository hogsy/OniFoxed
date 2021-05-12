/*
	FILE:	BFW_FileFormat.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 19, 1998
	
	PURPOSE: Interface to various graphical file formats
	
	Copyright 1998

*/

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_FileFormat.h"

#include "BFW_FF_PSD.h"
#include "BFW_FF_BMP.h"
#include "BFW_FF_DDS.h"

typedef UUtBool
(*FFtFunc_IsType)(
	BFtFileRef*		inFileRef);
	
typedef UUtError
(*FFtFunc_Read)(
	BFtFileRef*		inFileRef,
	IMtPixelType	inPixelType,
	UUtUns16		*outWidth,
	UUtUns16		*outHeight,
	IMtMipMap		*outMipMap,
	void*			*outData);
	
typedef UUtError
(*FFtFunc_Write)(
	BFtFileRef*		inFileRef,
	IMtPixelType	inPixelType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	IMtMipMap		inMipMap,
	void*			inData);

typedef UUtError
(*FFtFunc_Peek)(
	BFtFileRef*		inFileRef,
	FFtFileInfo*	outFileInfo);
	
typedef struct FFtParser
{
	FFtFormat_2D	format;
	FFtFunc_IsType	isType;
	FFtFunc_Read	read;
	FFtFunc_Write	write;
	FFtFunc_Peek	peek;
	
} FFtParser;

FFtParser	FFgParserTable[] =
	{
		{FFcFormat_2D_BMP, FFrType_IsBMP, FFrRead_BMP, FFrWrite_BMP, FFrPeek_BMP},
		{FFcFormat_2D_PSD, FFrType_IsPSD, FFrRead_PSD, FFrWrite_PSD, FFrPeek_PSD},
		{FFcFormat_2D_DDS, FFrType_IsDDS, FFrRead_DDS, FFrWrite_DDS, FFrPeek_DDS},
		{FFcFormat_2D_None, NULL, NULL, NULL}
	};
	
UUtError
FFrWrite_2D(
	BFtFileRef*		inFileRef,
	FFtFormat_2D	inFormat,
	IMtPixelType	inPixelType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	IMtMipMap		inMipMap,
	void*			inData)
{
	FFtParser*	curParser;
	
	for(curParser = FFgParserTable;
		curParser->format != FFcFormat_2D_None;
		curParser++)
	{
		if(curParser->format == inFormat)
		{
			return curParser->write(
					inFileRef,
					inPixelType,
					inWidth,
					inHeight,
					inMipMap,
					inData);
		}
	}
	
	// Could not find parser
	return UUcError_Generic;
}


UUtError
FFrRead_2D_Reduce(
	BFtFileRef*		inFileRef,
	IMtPixelType	inPixelType,
	UUtUns32		inReduce,
	IMtScaleMode	inScaleMode,
	UUtUns16		*outWidth,
	UUtUns16		*outHeight,
	IMtMipMap		*outMipMap,
	void			**outData)
{
	UUtError error = UUcError_None;
	FFtFileInfo info;
	UUtUns32 buffer_size;
	UUtUns32 reduced_buffer_size;
	char *buffer = NULL;
	char *out_buffer = NULL;
	UUtUns16 width, height;
	UUtUns32 itr;
	IMtMipMap mipMap;

	if (0 == inReduce) {
		error = FFrRead_2D(inFileRef, inPixelType, outWidth, outHeight, outMipMap, outData);
		return error;
	}

	FFrPeek_2D(inFileRef, &info);

	buffer_size = IMrImage_ComputeSize(inPixelType, info.mipMap, info.width, info.height);
	reduced_buffer_size = IMrImage_ComputeSize(inPixelType, info.mipMap, info.width >> inReduce, info.height >> inReduce);
	out_buffer = UUrMemory_Block_New(reduced_buffer_size); 
	if (error != UUcError_None) { 
		goto error_exit;
	}

	error = FFrRead_2D(inFileRef, inPixelType, &width, &height, &mipMap, &buffer);
	if (error != UUcError_None) { 
		goto error_exit;
	}

	if (IMcHasMipMap == mipMap) {
		const char *reduced_buffer = buffer;

		for(itr = 0; itr < inReduce; itr++)
		{
			reduced_buffer += IMrImage_ComputeSize(inPixelType, IMcNoMipMap, width, height);

			width = width >> 1;
			height = height >> 1;
		}

		UUrMemory_MoveFast(reduced_buffer, out_buffer, reduced_buffer_size);
	}
	else {
		error = IMrImage_Scale(inScaleMode, width, height, inPixelType, buffer, width >> inReduce, height >> inReduce, out_buffer);

		if (error != UUcError_None) { 
			goto error_exit;
		}
	}

	*outWidth = info.width >> inReduce;
	*outHeight = info.height >> inReduce;
	*outMipMap = mipMap;
	*outData = out_buffer;

	UUrMemory_Block_Delete(buffer);

	return UUcError_None;

error_exit:
	if (out_buffer != NULL) {
		UUrMemory_Block_Delete(out_buffer);
	}

	if (buffer != NULL) {
		UUrMemory_Block_Delete(buffer);
	}

	error = (UUcError_None == error) ?  UUcError_Generic : error;

	return error;

}

UUtError
FFrRead_2D(
	BFtFileRef*		inFileRef,
	IMtPixelType	inPixelType,
	UUtUns16		*outWidth,
	UUtUns16		*outHeight,
	IMtMipMap		*outMipMap,
	void*			*outData)			// Caller is responsible for freeing this memory
{
	FFtParser*	curParser;
	
	for(curParser = FFgParserTable;
		curParser->format != FFcFormat_2D_None;
		curParser++)
	{
		if(curParser->isType(inFileRef) == UUcTrue)
		{
			return curParser->read(
					inFileRef,
					inPixelType,
					outWidth,
					outHeight,
					outMipMap,
					outData);
		}
	}

	// Could not find parser
	return UUcError_Generic;
}

UUtError
FFrPeek_2D(
	BFtFileRef*		inFileRef,
	FFtFileInfo*	outFileInfo)
{
	FFtParser*	curParser;
	UUtError error;
	
	for(curParser = FFgParserTable;
		curParser->format != FFcFormat_2D_None;
		curParser++)
	{
		UUtBool isType = curParser->isType(inFileRef);

		if(isType)
		{
			error = curParser->peek(inFileRef, outFileInfo);
		}
	}

	// Could not find parser
	return error;
}
