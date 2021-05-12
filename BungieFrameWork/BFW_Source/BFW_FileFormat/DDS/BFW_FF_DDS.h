/*
	FILE:	BFW_FF_DDS.h
	
	AUTHOR:	Michael Evans
	
	CREATED: Jan 12, 1999
	
	PURPOSE: 
	
	Copyright 1999

*/
#ifndef BFW_FF_DDS_H
#define BFW_FF_DDS_H

UUtError
FFrWrite_DDS(
	BFtFileRef*		inFileRef,
	IMtPixelType	inPixelType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	IMtMipMap		inMipMap,
	void*			inData);

UUtBool
FFrType_IsDDS(
	BFtFileRef*		inFileRef);
	
UUtError
FFrRead_DDS(
	BFtFileRef*		inFileRef,
	IMtPixelType	inExpectedPixelType,// Will return error if file is not this pixel type
	UUtUns16		*outWidth,
	UUtUns16		*outHeight,
	IMtMipMap		*outMipMap,
	void*			*outData);			// Caller is responsible for freeing this memory

UUtError
FFrPeek_DDS(
	BFtFileRef*		inFileRef,
	FFtFileInfo*	outFileInfo);

#endif /* BFW_FF_DDS_H */

