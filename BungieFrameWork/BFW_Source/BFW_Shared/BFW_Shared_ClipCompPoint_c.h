/*
	FILE:	BFW_Shared_ClipCompPoint_c.h

	AUTHOR:	Brent H. Pease

	CREATED: May 22, 1997

	PURPOSE: Interface to the Motoko 3D engine

	Copyright 1997

*/

#if !defined(MSmGeomClipCompPoint_3DFunctionName) || \
	!defined(MSmGeomClipCompPoint_4DFunctionName) || \
	!defined(MSmGeomClipCompPoint_InterpShading) || \
	!defined(MSmGeomClipCompPoint_IsVisible)

	#error some macros have not been defined

#endif

static void
MSmGeomClipCompPoint_4DFunctionName(
	M3tPoint4D*		inFrustumPointIn,
	M3tPoint4D*		inFrustumPointOut,
	UUtUns16		inClipPlane,
	M3tPoint4D*		inResultFrustumPoint,
	M3tPointScreen*	inResultScreenPoint,
	UUtUns16*		inResultClipCode,
	float			inScaleX,
	float			inScaleY)
{
	register float			t, oneMinust, wIn, wOut, pIn, pOut, negW, invW;
	register float			hW, hX, hY, hZ;
	register UUtUns16		curClipCodeValue;


	#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING

		MSiVerifyPoint4D(inFrustumPointIn);
		MSiVerifyPoint4D(inFrustumPointOut);

		switch(inClipPlane)
		{
			case MScClipCode_PosX:
				UUmAssert(inFrustumPointIn->x <= inFrustumPointIn->w);
				UUmAssert(inFrustumPointOut->x > inFrustumPointOut->w);
				break;

			case MScClipCode_NegX:
				UUmAssert(inFrustumPointIn->x >= -inFrustumPointIn->w);
				UUmAssert(inFrustumPointOut->x < -inFrustumPointOut->w);
				break;

			case MScClipCode_PosY:
				UUmAssert(inFrustumPointIn->y <= inFrustumPointIn->w);
				UUmAssert(inFrustumPointOut->y > inFrustumPointOut->w);
				break;

			case MScClipCode_NegY:
				UUmAssert(inFrustumPointIn->y >= -inFrustumPointIn->w);
				UUmAssert(inFrustumPointOut->y < -inFrustumPointOut->w);
				break;

			case MScClipCode_PosZ:
				UUmAssert(inFrustumPointIn->z <= inFrustumPointIn->w);
				UUmAssert(inFrustumPointOut->z > inFrustumPointOut->w);
				break;

			case MScClipCode_NegZ:
				UUmAssert(inFrustumPointIn->z >= 0.0);
				UUmAssert(inFrustumPointOut->z < 0.0);
				break;

		}

	#endif

	wIn = inFrustumPointIn->w;
	wOut = inFrustumPointOut->w;

	switch(inClipPlane)
	{
		case MScClipCode_PosX:
			pIn = inFrustumPointIn->x;
			pOut = inFrustumPointOut->x;

			t = (wIn - pIn) / ((pOut - pIn) - (wOut - wIn));
			oneMinust = (1.0f - t);

			UUmAssert(t <= 1.0f);
			UUmAssert(t >= -1.0f);

			hX = hW = t * inFrustumPointOut->w + oneMinust * inFrustumPointIn->w;

			hY = t * inFrustumPointOut->y + oneMinust * inFrustumPointIn->y;

			hZ = t * inFrustumPointOut->z + oneMinust * inFrustumPointIn->z;
			break;

		case MScClipCode_NegX:
			pIn = inFrustumPointIn->x;
			pOut = inFrustumPointOut->x;

			t = (pIn + wIn) / ((pIn - pOut) + (wIn - wOut));
			oneMinust = (1.0f - t);

			UUmAssert(t <= 1.0f);
			UUmAssert(t >= -1.0f);

			hW = t * inFrustumPointOut->w + oneMinust * inFrustumPointIn->w;

			hY = t * inFrustumPointOut->y + oneMinust * inFrustumPointIn->y;

			hX = -hW;

			hZ = t * inFrustumPointOut->z + oneMinust * inFrustumPointIn->z;
			break;

		case MScClipCode_PosY:
			pIn = inFrustumPointIn->y;
			pOut = inFrustumPointOut->y;
			t = (wIn - pIn) / ((pOut - pIn) - (wOut - wIn));
			oneMinust = (1.0f - t);

			UUmAssert(t <= 1.0f);
			UUmAssert(t >= -1.0f);

			hY = hW = t * inFrustumPointOut->w + oneMinust * inFrustumPointIn->w;

			hX = t * inFrustumPointOut->x + oneMinust * inFrustumPointIn->x;

			hZ = t * inFrustumPointOut->z + oneMinust * inFrustumPointIn->z;
			break;

		case MScClipCode_NegY:
			pIn = inFrustumPointIn->y;
			pOut = inFrustumPointOut->y;
			t = (pIn + wIn) / ((pIn - pOut) + (wIn - wOut));
			oneMinust = (1.0f - t);

			UUmAssert(t <= 1.0f);
			UUmAssert(t >= -1.0f);

			hW = t * inFrustumPointOut->w + oneMinust * inFrustumPointIn->w;

			hY = -hW;

			hX = t * inFrustumPointOut->x + oneMinust * inFrustumPointIn->x;

			hZ = t * inFrustumPointOut->z + oneMinust * inFrustumPointIn->z;
			break;

		case MScClipCode_PosZ:
			pIn = inFrustumPointIn->z;
			pOut = inFrustumPointOut->z;
			t = (wIn - pIn) / ((pOut - pIn) - (wOut - wIn));
			oneMinust = (1.0f - t);

			UUmAssert(t <= 1.0f);
			UUmAssert(t >= -1.0f);

			hW = t * inFrustumPointOut->w + oneMinust * inFrustumPointIn->w;

			hX = t * inFrustumPointOut->x + oneMinust * inFrustumPointIn->x;

			hY = t * inFrustumPointOut->y + oneMinust * inFrustumPointIn->y;

			hZ = hW;
			break;

		case MScClipCode_NegZ:
			pIn = inFrustumPointIn->z;
			pOut = inFrustumPointOut->z;
			t = pIn / (pIn - pOut);
			oneMinust = (1.0f - t);

			UUmAssert(t <= 1.0f);
			UUmAssert(t >= -1.0f);

			hW = t * inFrustumPointOut->w + oneMinust * inFrustumPointIn->w;

			hX = t * inFrustumPointOut->x + oneMinust * inFrustumPointIn->x;

			hY = t * inFrustumPointOut->y + oneMinust * inFrustumPointIn->y;

			hZ = 0.0;
			break;

		default:
			UUmAssert(!"Illegal code");
	}

	negW = -hW;

	curClipCodeValue = MScClipCode_None;

	if(hX > hW)
	{
		curClipCodeValue |= MScClipCode_PosX;
	}
	if(hX < negW)
	{
		curClipCodeValue |= MScClipCode_NegX;
	}
	if(hY > hW)
	{
		curClipCodeValue |= MScClipCode_PosY;
	}
	if(hY < negW)
	{
		curClipCodeValue |= MScClipCode_NegY;
	}
	if(hZ > hW)
	{
		curClipCodeValue |= MScClipCode_PosZ;
	}
	if(hZ < 0.0)
	{
		curClipCodeValue |= MScClipCode_NegZ;
	}

	inResultFrustumPoint->x = hX;
	inResultFrustumPoint->y = hY;
	inResultFrustumPoint->z = hZ;
	inResultFrustumPoint->w = hW;

	#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING

		MSrClipCode_ValidateFrustum(
			inResultFrustumPoint,
			curClipCodeValue);

	#endif

	if(curClipCodeValue == 0)
	{
		invW = 1.0f / hW;

		if(hX == negW)
		{
			inResultScreenPoint->x = 0.0f;
		}
		else
		{
			inResultScreenPoint->x = (hX * invW + 1.0f) * inScaleX;
		}

		if(hY == hW)
		{
			inResultScreenPoint->y = 0.0f;
		}
		else
		{
			inResultScreenPoint->y = (hY * invW - 1.0f) * inScaleY;
		}

		UUmAssert(inResultScreenPoint->y >= 0.0f);
		UUmAssert(inResultScreenPoint->y < 480.0f);

		inResultScreenPoint->z = hZ * invW;
		inResultScreenPoint->invW = invW;

		MSiVerifyPointScreen(inResultScreenPoint);
	}

	*inResultClipCode = curClipCodeValue;
}

static void
MSmGeomClipCompPoint_3DFunctionName(
	M3tPointScreen*	inScreenPointIn,
	M3tPointScreen*	inScreenPointOut,
	UUtUns16		inClipPlane,
	M3tPointScreen*	inResultScreenPoint,
	UUtUns16*		inResultClipCode,
	float			inScreenWidth,
	float			inScreenHeight)
{
	float		xIn, xOut, xNew;
	float		yIn, yOut, yNew;
	float		zIn, zOut, zNew;
	float		wIn, wOut, wNew;
	float		t;
	UUtUns16	clipCodeNew;

	// XXX - Eventually handle W

	#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING

		MSiVerifyPointScreen(inScreenPointIn);
		MSiVerifyPointScreen(inScreenPointOut);

		switch(inClipPlane)
		{
			case MScClipCode_PosX:
				UUmAssert(inScreenPointIn->x < inScreenWidth);
				UUmAssert(inScreenPointOut->x >= inScreenWidth);
				break;

			case MScClipCode_NegX:
				UUmAssert(inScreenPointIn->x >= 0.0);
				UUmAssert(inScreenPointOut->x < 0.0);
				break;

			case MScClipCode_PosY:
				UUmAssert(inScreenPointIn->y >= 0.0);
				UUmAssert(inScreenPointOut->y < 0.0);
				break;

			case MScClipCode_NegY:
				UUmAssert(inScreenPointIn->y < inScreenHeight);
				UUmAssert(inScreenPointOut->y >= inScreenHeight);
				break;

			case MScClipCode_PosZ:
				UUmAssert(inScreenPointIn->z < 1.0);
				UUmAssert(inScreenPointOut->z >= 1.0);
				break;

			case MScClipCode_NegZ:
				UUmAssert(inScreenPointIn->z >= 0.0);
				UUmAssert(inScreenPointOut->z < 0.0);
				break;
		}
	#endif

	xIn = inScreenPointIn->x;
	yIn = inScreenPointIn->y;
	zIn = inScreenPointIn->z;
	wIn = 1.0f / inScreenPointIn->invW;
	xOut = inScreenPointOut->x;
	yOut = inScreenPointOut->y;
	zOut = inScreenPointOut->z;
	wOut = 1.0f / inScreenPointOut->invW;

	switch(inClipPlane)
	{
		case MScClipCode_PosX:
			t = (inScreenWidth - xIn) / (xOut - xIn);
			yNew = yIn + t * (yOut - yIn);
			zNew = zIn + t * (zOut - zIn);
			xNew = inScreenWidth - 0.0001f; // fudge it in a little;
			wNew = wIn + t * (wOut - wIn);
			break;

		case MScClipCode_NegX:
			t = (0.0f - xOut) / (xIn - xOut);
			yNew = yOut + t * (yIn - yOut);
			zNew = zOut + t * (zIn - zOut);
			xNew = 0.0;
			wNew = wOut + t * (wIn - wOut);
			break;

		case MScClipCode_PosY:
			t = (0.0f - yOut) / (yIn - yOut);
			xNew = xOut + t * (xIn - xOut);
			zNew = zOut + t * (zIn - zOut);
			yNew = 0.0;
			wNew = wOut + t * (wIn - wOut);
			break;

		case MScClipCode_NegY:
			t = (inScreenHeight - yIn) / (yOut - yIn);
			xNew = xIn + t * (xOut - xIn);
			zNew = zIn + t * (zOut - zIn);
			yNew = inScreenHeight - 0.0001f; // fudge it in a little;
			wNew = wIn + t * (wOut - wIn);
			break;

		case MScClipCode_PosZ:
			t = (1.0f - zIn) / (zOut - zIn);
			xNew = xIn + t * (xOut - xIn);
			yNew = yIn + t * (yOut - yIn);
			zNew = 0.9999f; // fudge it in a little;
			wNew = wIn + t * (wOut - wIn);
			break;

		case MScClipCode_NegZ:
			t = (0.0f - zOut) / (zIn - zOut);
			xNew = xOut + t * (xIn - xOut);
			yNew = yOut + t * (yIn - yOut);
			zNew = 0.0;
			wNew = wOut + t * (wIn - wOut);
			break;

		default:
			UUmAssert("Illegal clip plane");
	}

	clipCodeNew = 0;

	if(xNew < 0.0f)
	{
		clipCodeNew |= MScClipCode_NegX;
	}

	if(xNew >= inScreenWidth)
	{
		clipCodeNew |= MScClipCode_PosX;
	}

	if(yNew < 0.0f)
	{
		clipCodeNew |= MScClipCode_PosY;  // This is not a bug
	}

	if(yNew >= inScreenHeight)
	{
		clipCodeNew |= MScClipCode_NegY;  // This is not a bug
	}

	if(zNew >= 1.0f)
	{
		clipCodeNew |= MScClipCode_PosZ;
	}

	if(zNew < 0.0f)
	{
		clipCodeNew |= MScClipCode_NegZ;
	}

	*inResultClipCode = clipCodeNew;

	inResultScreenPoint->x = xNew;
	inResultScreenPoint->y = yNew;
	inResultScreenPoint->z = zNew;
	inResultScreenPoint->invW = 1.0f / wNew;

	#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING

		MSrClipCode_ValidateScreen(
			inResultScreenPoint,
			clipCodeNew,
			inScreenWidth,
			inScreenHeight);

	#endif
}
