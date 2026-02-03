/*
	FILE:	BFW_FF_PSD.h

	AUTHOR:	Brent H. Pease

	CREATED: Sept 19, 1998

	PURPOSE:

	Copyright 1998

*/
#ifndef BFW_FF_PSD_H
#define BFW_FF_PSD_H

UUtError
FFrWrite_PSD(
	BFtFileRef*		inFileRef,
	IMtPixelType	inPixelType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	IMtMipMap		inMipMap,
	void*			inData);

UUtBool
FFrType_IsPSD(
	BFtFileRef*		inFileRef);

UUtError
FFrRead_PSD(
	BFtFileRef*		inFileRef,
	IMtPixelType	inExpectedPixelType,// Will return error if file is not this pixel type
	UUtUns16		*outWidth,
	UUtUns16		*outHeight,
	IMtMipMap		*outMipMap,
	void*			*outData);			// Caller is responsible for freeing this memory

UUtError
FFrPeek_PSD(
	BFtFileRef*		inFileRef,
	FFtFileInfo*	outFileInfo);

#endif /* BFW_FF_PSD_H */
