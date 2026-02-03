/*
	FILE:	MS_TriRasterize.h

	AUTHOR:	Brent H. Pease

	CREATED: April 3, 1997

	PURPOSE:  Assert interface files

	Copyright 1997

*/
#ifndef MS_TRIRASTERIZE_H
#define MS_TRIRASTERIZE_H


#define MSmUCharOffset	0.1F
#define MSmUCharScale	(256.0F - 2.0F * MSmUCharOffset)

#define MSmULongOffset	0x400UL
#define MSmULongScale	(0xffffffffUL-MSmULongOffset)

#define MSmFloatOffset	0.0001F

#define MSmFractBits		16
#define MSmFractOneHalf	(1L<<(MSmFractBits-1))
#define MSmFractOne		(1L<<MSmFractBits)

#define MSmSubPixelBits		2
#define MSmSubPixelUnits		(1<<MSmSubPixelBits)
#define MSmHalfSubPixel		(MSmSubPixelUnits/2)

#define MSmNumPerspectiveGuardBits

#endif
