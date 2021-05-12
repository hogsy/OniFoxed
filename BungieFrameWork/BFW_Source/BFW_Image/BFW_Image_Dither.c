/*
	FILE:	BFW_Image_Dither.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Nov 27, 1998
	
	PURPOSE: 
	
	Copyright 1998
*/

#include "BFW.h"
#include "BFW_Image.h"
#include "BFW_Image_Private.h"



void
IMrImage_Dither_Row_ARGB_to_ARGB1555(
	UUtUns16		inWidth,
	IMtDitherPixel*	inSrcCurRow,
	IMtDitherPixel*	inSrcNextRow,
	UUtUns16*		outDstRow)
{
	UUtUns16			x;
	IMtDitherPixel*		curSrcPixel;
	UUtUns8				a, r, g, b;
	UUtUns8				qr, qg, qb;
	UUtUns8				dr, dg, db;
	UUtUns16*			dstPtr;
	UUtInt16			er, eg, eb;
	
	for(x = 0, curSrcPixel = inSrcCurRow, dstPtr = outDstRow;
		x < inWidth;
		x++, curSrcPixel++, dstPtr++)
	{
		// pin the current pixel to [0, 255]
		a = UUmPin(curSrcPixel->a, 0, UUcMaxUns8);
		
		if(a != 0xFF)
		{
			*dstPtr = 0;
			continue;
		}
		
		r = UUmPin(curSrcPixel->r, 0, UUcMaxUns8);
		g = UUmPin(curSrcPixel->g, 0, UUcMaxUns8);
		b = UUmPin(curSrcPixel->b, 0, UUcMaxUns8);
		
		// quantize the pixel
		//qr = (UUtUns8)(((UUtUns16)(r >> 3) * (UUtUns16)UUcMaxUns8) / (UUtUns16)31);
		//qg = (UUtUns8)(((UUtUns16)(g >> 3) * (UUtUns16)UUcMaxUns8) / (UUtUns16)31);
		//qb = (UUtUns8)(((UUtUns16)(b >> 3) * (UUtUns16)UUcMaxUns8) / (UUtUns16)31);
		qr = (UUtUns8)((r + 4) & ~0x7);
		qg = (UUtUns8)((g + 4) & ~0x7);
		qb = (UUtUns8)((b + 4) & ~0x7);
		
		dr = qr >> 3;
		dg = qg >> 3;
		db = qb >> 3;
		
		*dstPtr = (1 << 15) | (dr << 10) | (dg << 5) | db;
		
		er = (UUtInt16)r - (UUtInt16)qr;
		eg = (UUtInt16)g - (UUtInt16)qg;
		eb = (UUtInt16)b - (UUtInt16)qb;
		
		// 7/16ths distributed right
		if(x < inWidth - 1 && (curSrcPixel + 1)->a == 0xFF)
		{
			/* S.S. (curSrcPixel + 1)->r += er * 7 / 16;
			(curSrcPixel + 1)->g += eg * 7 / 16;
			(curSrcPixel + 1)->b += eb * 7 / 16;*/
			(curSrcPixel + 1)->r += ((er * 7) >> 4);
			(curSrcPixel + 1)->g += ((eg * 7) >> 4);
			(curSrcPixel + 1)->b += ((eb * 7) >> 4);
		}
		
		if(inSrcNextRow)
		{
			// 3/16ths below left
			if(x > 0 && inSrcNextRow[x - 1].a == 0xFF)
			{
				/* S.S. inSrcNextRow[x - 1].r += er * 3 / 16;
				inSrcNextRow[x - 1].g += eg * 3 / 16;
				inSrcNextRow[x - 1].b += eb * 3 / 16;*/
				inSrcNextRow[x - 1].r += ((er * 3) >> 4);
				inSrcNextRow[x - 1].g += ((eg * 3) >> 4);
				inSrcNextRow[x - 1].b += ((eb * 3) >> 4);
			}
			
			// 5/16ths below
			if(inSrcNextRow[x].a == 0xFF)
			{
				/* S.S. inSrcNextRow[x].r += er * 5 / 16;
				inSrcNextRow[x].g += eg * 5 / 16;
				inSrcNextRow[x].b += eb * 5 / 16;*/
				inSrcNextRow[x].r += ((er * 5) >> 4);
				inSrcNextRow[x].g += ((eg * 5) >> 4);
				inSrcNextRow[x].b += ((eb * 5) >> 4);
			}
			
			// 1/16th below right
			if(x < inWidth - 1 && inSrcNextRow[x + 1].a == 0xFF)
			{
				/* S.S. inSrcNextRow[x + 1].r += er / 16;
				inSrcNextRow[x + 1].g += eg / 16;
				inSrcNextRow[x + 1].b += eb / 16;*/
				inSrcNextRow[x + 1].r += er >> 4;
				inSrcNextRow[x + 1].g += eg >> 4;
				inSrcNextRow[x + 1].b += eb >> 4;
			}
		}
	}
}

void
IMrImage_Dither_Row_ARGB_to_ARGB4444(
	UUtUns16		inWidth,
	IMtDitherPixel*	inSrcCurRow,
	IMtDitherPixel*	inSrcNextRow,
	UUtUns16*		outDstRow)
{

}

void
IMrImage_Dither_Row_RGB_to_ARGB1555(
	UUtUns16		inWidth,
	IMtDitherPixel*	inSrcCurRow,
	IMtDitherPixel*	inSrcNextRow,
	UUtUns16*		outDstRow)
{
	UUtUns16			x;
	IMtDitherPixel*		curSrcPixel;
	UUtUns8				a, r, g, b;
	UUtUns8				qr, qg, qb;
	UUtUns8				dr, dg, db;
	UUtUns16*			dstPtr;
	UUtInt16			er, eg, eb;
	
	for(x = 0, curSrcPixel = inSrcCurRow, dstPtr = outDstRow;
		x < inWidth;
		x++, curSrcPixel++, dstPtr++)
	{
		// pin the current pixel to [0, 255]
		a = UUmPin(curSrcPixel->a, 0, UUcMaxUns8);
		
		if(a != 0xFF)
		{
			*dstPtr = 0;
			continue;
		}
		
		r = UUmPin(curSrcPixel->r, 0, UUcMaxUns8);
		g = UUmPin(curSrcPixel->g, 0, UUcMaxUns8);
		b = UUmPin(curSrcPixel->b, 0, UUcMaxUns8);
		
		// quantize the pixel
		qr = (UUtUns8)(((UUtUns16)(r >> 3) * (UUtUns16)UUcMaxUns8) / (UUtUns16)31);
		qg = (UUtUns8)(((UUtUns16)(g >> 3) * (UUtUns16)UUcMaxUns8) / (UUtUns16)31);
		qb = (UUtUns8)(((UUtUns16)(b >> 3) * (UUtUns16)UUcMaxUns8) / (UUtUns16)31);
		
		dr = qr >> 3;
		dg = qg >> 3;
		db = qb >> 3;
		
		*dstPtr = (1 << 15) | (dr << 10) | (dg << 5) | db;
		
		er = (UUtInt16)r - (UUtInt16)qr;
		eg = (UUtInt16)g - (UUtInt16)qg;
		eb = (UUtInt16)b - (UUtInt16)qb;
		
		// 7/16ths distributed right
		if(x < inWidth - 1 && (curSrcPixel + 1)->a == 0xFF)
		{
			/* S.S. (curSrcPixel + 1)->r += er * 7 / 16;
			(curSrcPixel + 1)->g += eg * 7 / 16;
			(curSrcPixel + 1)->b += eb * 7 / 16;*/
			(curSrcPixel + 1)->r += ((er * 7) >> 4);
			(curSrcPixel + 1)->g += ((eg * 7) >> 4);
			(curSrcPixel + 1)->b += ((eb * 7) >> 4);
		}
		
		if(inSrcNextRow)
		{
			// 3/16ths below left
			if(x > 0 && inSrcNextRow[x - 1].a == 0xFF)
			{
				/* S.S. inSrcNextRow[x - 1].r += er * 3 / 16;
				inSrcNextRow[x - 1].g += eg * 3 / 16;
				inSrcNextRow[x - 1].b += eb * 3 / 16;*/
				inSrcNextRow[x - 1].r += ((er * 3) >> 4);
				inSrcNextRow[x - 1].g += ((eg * 3) >> 4);
				inSrcNextRow[x - 1].b += ((eb * 3) >> 4);
			}
			
			// 5/16ths below
			if(inSrcNextRow[x].a == 0xFF)
			{
				/* S.S. inSrcNextRow[x].r += er * 5 / 16;
				inSrcNextRow[x].g += eg * 5 / 16;
				inSrcNextRow[x].b += eb * 5 / 16;*/
				inSrcNextRow[x].r += ((er * 5) >> 4);
				inSrcNextRow[x].g += ((eg * 5) >> 4);
				inSrcNextRow[x].b += ((eb * 5) >> 4);
			}
			
			// 1/16th below right
			if(x < inWidth - 1 && inSrcNextRow[x + 1].a == 0xFF)
			{
				/* inSrcNextRow[x + 1].r += er / 16;
				inSrcNextRow[x + 1].g += eg / 16;
				inSrcNextRow[x + 1].b += eb / 16;*/
				inSrcNextRow[x + 1].r += er >> 4;
				inSrcNextRow[x + 1].g += eg >> 4;
				inSrcNextRow[x + 1].b += eb >> 4;
			}
		}
	}
}

void
IMrImage_Dither_Row_RGB_to_ARGB4444(
	UUtUns16		inWidth,
	IMtDitherPixel*	inSrcCurRow,
	IMtDitherPixel*	inSrcNextRow,
	UUtUns16*		outDstRow)
{

}



