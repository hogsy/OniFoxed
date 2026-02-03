/*
	FILE:	BFW_Shared_Clip.c

	AUTHOR:	Brent H. Pease

	CREATED: Oct 23, 1997

	PURPOSE:

	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"

#include "BFW_Shared_Clip.h"

#if defined(DEBUGGING) && DEBUGGING

void
MSiVerifyPoint4D(
	M3tPoint4D	*inPoint4D)
{
	UUmAssert(inPoint4D->x > -1e13f && inPoint4D->x < 1e13f);
	UUmAssert(inPoint4D->y > -1e13f && inPoint4D->y < 1e13f);
	UUmAssert(inPoint4D->z > -1e13f && inPoint4D->z < 1e13f);
	UUmAssert(inPoint4D->w > -1e13f && inPoint4D->w < 1e13f);

}

void
MSiVerifyPointScreen(
	M3tPointScreen	*inPointScreen)
{
	UUmAssert(inPointScreen->x > -1e13f && inPointScreen->x < 1e13f);
	UUmAssert(inPointScreen->y > -1e13f && inPointScreen->y < 1e13f);
	UUmAssert(inPointScreen->z > -1e13f && inPointScreen->z < 1e13f);
	UUmAssert(inPointScreen->invW > -1e13f && inPointScreen->invW < 1e13f);
}

void
MSrClipCode_ValidateFrustum(
	M3tPoint4D	*inFrustumPoint,
	UUtUns16	inClipCode)
{
	if(inClipCode & MScClipCode_PosX)
	{
		UUmAssert(inFrustumPoint->x > inFrustumPoint->w);
	}
	else
	{
		UUmAssert(inFrustumPoint->x <= inFrustumPoint->w);
	}

	if(inClipCode & MScClipCode_NegX)
	{
		UUmAssert(inFrustumPoint->x < -inFrustumPoint->w);
	}
	else
	{
		UUmAssert(inFrustumPoint->x >= -inFrustumPoint->w);
	}

	if(inClipCode & MScClipCode_PosY)
	{
		UUmAssert(inFrustumPoint->y > inFrustumPoint->w);
	}
	else
	{
		UUmAssert(inFrustumPoint->y <= inFrustumPoint->w);
	}

	if(inClipCode & MScClipCode_NegY)
	{
		UUmAssert(inFrustumPoint->y < -inFrustumPoint->w);
	}
	else
	{
		UUmAssert(inFrustumPoint->y >= -inFrustumPoint->w);
	}

	if(inClipCode & MScClipCode_PosZ)
	{
		UUmAssert(inFrustumPoint->z > inFrustumPoint->w);
	}
	else
	{
		UUmAssert(inFrustumPoint->z <= inFrustumPoint->w);
	}

	if(inClipCode & MScClipCode_NegZ)
	{
		UUmAssert(inFrustumPoint->z < 0.0f);
	}
	else
	{
		UUmAssert(inFrustumPoint->z >= 0.0f);
	}
}

void
MSrClipCode_ValidateScreen(
	M3tPointScreen	*inScreenPoint,
	UUtUns16		inClipCode,
	float			inScreenWidth,
	float			inScreenHeight)
{
	if(inClipCode & MScClipCode_PosX)
	{
		UUmAssert(inScreenPoint->x >= inScreenWidth);
	}
	else
	{
		UUmAssert(inScreenPoint->x < inScreenWidth);
	}

	if(inClipCode & MScClipCode_NegX)
	{
		UUmAssert(inScreenPoint->x < 0.0f);
	}
	else
	{
		UUmAssert(inScreenPoint->x >= 0.0f);
	}

	if(inClipCode & MScClipCode_PosY)
	{
		UUmAssert(inScreenPoint->y < 0.0f);
	}
	else
	{
		UUmAssert(inScreenPoint->y >= 0.0f);
	}

	if(inClipCode & MScClipCode_NegY)
	{
		UUmAssert(inScreenPoint->y >= inScreenHeight);
	}
	else
	{
		UUmAssert(inScreenPoint->y < inScreenHeight);
	}

	if(inClipCode & MScClipCode_PosZ)
	{
		UUmAssert(inScreenPoint->z >= 1.0f);
	}
	else
	{
		UUmAssert(inScreenPoint->z < 1.0f);
	}

	if(inClipCode & MScClipCode_NegZ)
	{
		UUmAssert(inScreenPoint->z < 0.0f);
	}
	else
	{
		UUmAssert(inScreenPoint->z >= 0.0f);
	}
}

void
MSiDump_ClipCodes(
	FILE*		inFile,
	UUtUns32	inClipCode)
{
	if(inClipCode & MScClipCode_PosX)
	{
		fprintf(inFile, "PosX ");
	}
	if(inClipCode & MScClipCode_NegX)
	{
		fprintf(inFile, "NegX ");
	}
	if(inClipCode & MScClipCode_PosY)
	{
		fprintf(inFile, "PosY ");
	}
	if(inClipCode & MScClipCode_NegY)
	{
		fprintf(inFile, "NegY ");
	}
	if(inClipCode & MScClipCode_PosZ)
	{
		fprintf(inFile, "PosZ ");
	}
	if(inClipCode & MScClipCode_NegZ)
	{
		fprintf(inFile, "NegZ ");
	}
}

#endif

