/*
	FILE:	BFW_Shared_Clip.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Oct 23, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef BFW_SHARED_CLIP_H
#define BFW_SHARED_CLIP_H

typedef enum MStClipStatus
{
	MScClipStatus_TrivialAccept,
	MScClipStatus_TrivialReject,
	MScClipStatus_NeedsClipping
	
} MStClipStatus;

enum
{
	MScClipCode_None = 0,
	
	MScClipCode_PosX = 0x01,
	MScClipCode_NegX = 0x02,
	
	MScClipCode_PosY = 0x04,
	MScClipCode_NegY = 0x08,
	
	MScClipCode_PosZ = 0x10,
	MScClipCode_NegZ = 0x20
};

#if defined(DEBUGGING) && DEBUGGING

	void
	MSiVerifyPoint4D(
		M3tPoint4D	*inPoint4D);

	void
	MSiVerifyPointScreen(
		M3tPointScreen	*inPointScreen);

	void
	MSrClipCode_ValidateFrustum(
		M3tPoint4D	*inFrustumPoint,
		UUtUns16	inClipCode);

	void
	MSrClipCode_ValidateScreen(
		M3tPointScreen	*inScreenPoint,
		UUtUns16		inClipCode,
		float			inScreenWidth,
		float			inScreenHeight);

	void
	MSiDump_ClipCodes(
		FILE*		inFile,
		UUtUns32	inClipCode);

#else
	
	#define	MSiVerifyPoint4D(x)
	#define MSiVerifyPointScreen(x)
	
	#define MSrClipCode_ValidateScreen(a, b, c, d)
	#define MSrClipCode_ValidateFrustum(a, b)
	
#endif

#endif /* BFW_SHARED_CLIP_H */
