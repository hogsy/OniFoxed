/*
	FILE:	BFW_FF_BMP.h

	AUTHOR:	Brent H. Pease

	CREATED: Sept 19, 1998

	PURPOSE:

	Copyright 1998

*/
#ifndef BFW_FF_BMP_H
#define BFW_FF_BMP_H

UUtError
FFrWrite_BMP(
	BFtFileRef*		inFileRef,
	IMtPixelType	inPixelType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	IMtMipMap		inMipMap,
	void*			inData);

UUtBool
FFrType_IsBMP(
	BFtFileRef*		inFileRef);

UUtError
FFrRead_BMP(
	BFtFileRef*		inFileRef,
	IMtPixelType	inExpectedPixelType,// Will return error if file is not this pixel type
	UUtUns16		*outWidth,
	UUtUns16		*outHeight,
	IMtMipMap		*outMipMap,
	void*			*outData);			// Caller is responsible for freeing this memory

UUtError
FFrPeek_BMP(
	BFtFileRef*		inFileRef,
	FFtFileInfo*	outFileInfo);

#endif /* BFW_FF_BMP_H */
