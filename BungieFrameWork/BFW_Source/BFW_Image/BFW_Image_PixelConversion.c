/*
	FILE:	BFW_Image_PixelConversion.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Nov 27, 1998
	
	PURPOSE: 
	
	Copyright 1998
*/

#include "BFW.h"
#include "BFW_Image.h"
#include "BFW_Image_Private.h"

#define	IMcDitherDim	4
static UUtInt16	IMgDitherMatrix[IMcDitherDim][IMcDitherDim] =
{
	{-3,  1, -2,  2},
	{ 3, -1,  4,  0},
	{-2,  2, -3,  1},
	{ 4,  0,  3, -1}
};

#define IMmShiftDownAndMask8(pixel, s) ((UUtUns8) (((pixel) >> (s)) & 0xFF))
#define IMmShiftDownAndMask6(pixel, s) ((UUtUns8) (((pixel) >> (s)) & 0x3F))
#define IMmShiftDownAndMask5(pixel, s) ((UUtUns8) (((pixel) >> (s)) & 0x1F))
#define IMmShiftDownAndMask4(pixel, s) ((UUtUns8) (((pixel) >> (s)) & 0x0F))
#define IMmShiftDownAndMask2(pixel, s) ((UUtUns8) (((pixel) >> (s)) & 0x03))
#define IMmShiftDownAndMask1(pixel, s) ((UUtUns8) (((pixel) >> (s)) & 0x01))

#define IMmMask8AndShiftUp(pixel, s) ((((pixel) & 0xFF) << (s)))
#define IMmMask5AndShiftUp(pixel, s) ((((pixel) & 0x1F) << (s)))
#define IMmMask4AndShiftUp(pixel, s) ((((pixel) & 0x0F) << (s)))
#define IMmMask1AndShiftUp(pixel, s) ((((pixel) & 0x01) << (s)))

#define IMmARGB8888_to_argb(pixel, a, r, g, b)	\
			a = IMmShiftDownAndMask8(pixel, 24);	\
			r = IMmShiftDownAndMask8(pixel, 16);	\
			g = IMmShiftDownAndMask8(pixel, 8);		\
			b = IMmShiftDownAndMask8(pixel, 0);		\

#define IMmARGB8888_to_rgb(pixel, r, g, b)		\
			r = IMmShiftDownAndMask8(pixel, 16);	\
			g = IMmShiftDownAndMask8(pixel, 8);		\
			b = IMmShiftDownAndMask8(pixel, 0);		\
			
#define IMmRGB888_to_rgb(pixel, r, g, b)		\
			r = IMmShiftDownAndMask8(pixel, 16);	\
			g = IMmShiftDownAndMask8(pixel, 8);		\
			b = IMmShiftDownAndMask8(pixel, 0);		\

#define IMmRGB555_to_rgb(pixel, r, g, b)	\
			r = IMmShiftDownAndMask5(pixel, 10);	\
			g = IMmShiftDownAndMask5(pixel, 5);		\
			b = IMmShiftDownAndMask5(pixel, 0);		\

#define IMmARGB1555_to_argb(pixel, a, r, g, b)	\
			a = IMmShiftDownAndMask1(pixel, 15);	\
			r = IMmShiftDownAndMask5(pixel, 10);	\
			g = IMmShiftDownAndMask5(pixel, 5);		\
			b = IMmShiftDownAndMask5(pixel, 0);		\

#define IMmARGB1555_to_rgb(pixel, r, g, b)		\
			r = IMmShiftDownAndMask5(pixel, 15);	\
			g = IMmShiftDownAndMask5(pixel, 5);		\
			b = IMmShiftDownAndMask5(pixel, 0);		\

#define IMmARGB4444_to_rgb(pixel, r, g, b)	\
			r = IMmShiftDownAndMask4(pixel, 8);	\
			g = IMmShiftDownAndMask4(pixel, 4);	\
			b = IMmShiftDownAndMask4(pixel, 0);	\

#define IMmARGB4444_to_argb(pixel, a, r, g, b)	\
			a = IMmShiftDownAndMask4(pixel, 12);	\
			r = IMmShiftDownAndMask4(pixel, 8);	\
			g = IMmShiftDownAndMask4(pixel, 4);	\
			b = IMmShiftDownAndMask4(pixel, 0);	\

#define IMmARGB_to_ARGB8888(pixel, a, r, g, b) \
			pixel = IMmMask8AndShiftUp(a, 24); \
			pixel |= IMmMask8AndShiftUp(r, 16); \
			pixel |= IMmMask8AndShiftUp(g, 8); \
			pixel |= IMmMask8AndShiftUp(b, 0); \

#define IMmRGB_to_RGB888(pixel, r, g, b)	\
			pixel = IMmMask8AndShiftUp(r, 24); \
			pixel |= IMmMask8AndShiftUp(g, 16); \
			pixel |= IMmMask8AndShiftUp(b, 8); \
		
#define IMmARGB_to_ARGB1555(pixel, a, r, g, b) \
			pixel = IMmMask1AndShiftUp(a, 15); \
			pixel |= IMmMask5AndShiftUp(r, 10); \
			pixel |= IMmMask5AndShiftUp(g, 5); \
			pixel |= IMmMask5AndShiftUp(b, 0); \
		
#define IMmRGB_to_RGB555(pixel, r, g, b) \
			pixel = IMmMask5AndShiftUp(r, 10); \
			pixel |= IMmMask5AndShiftUp(g, 5); \
			pixel |= IMmMask5AndShiftUp(b, 0); \
		
#define IMmARGB_to_ARGB4444(pixel, a, r, g, b) \
			pixel = IMmMask8AndShiftUp(a, 12); \
			pixel |= IMmMask8AndShiftUp(r, 8); \
			pixel |= IMmMask8AndShiftUp(g, 4); \
			pixel |= IMmMask8AndShiftUp(b, 0); \
		
#define IMmRGB888_to_I8(pixel, r, g, b) \
			pixel = (r + g + b) / 3;

#define IMmRGB555_to_I8(pixel, r, g, b) \
			pixel = (r + g + b) / 3;	\
			pixel = (pixel << 3) | (pixel & 0x7);
		
#define IMmRGB444_to_I8(pixel, r, g, b) \
			pixel = (r + g + b) / 3;	\
			pixel = (pixel << 4) | (pixel & 0xF);
		
typedef UUtError
(*IMtConvertPixelType_Proc)(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData);	 // caller is responsible for allocating this memory


// this table is built at runtime
static UUtBool IMgConvertPixelType_Table_Built = UUcFalse;
static IMtConvertPixelType_Proc IMgConvertPixelType_Table[IMcNumPixelTypes][IMcNumPixelTypes];
static void iBuild_ConvertPixelType_Table(void);

typedef struct {
	IMtPixelType				from;
	IMtPixelType				to;
	IMtConvertPixelType_Proc	function;	
} IMtConverPixelType_LookupEntry;



static UUtError
IMiConvert_RGB555_to_I8(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtUns16	x, y;
	UUtUns16	r, g, b;
	
	UUtUns16*	srcPtr;
	UUtUns8*	dstPtr;
	UUtUns16	pixel;
	
	UUtUns8*	dstRowPtr;
	UUtUns16	dstRowBytes;

	dstRowBytes = IMrImage_ComputeRowBytes(IMcPixelType_I8, inWidth);
	
	srcPtr = inSrcData;
	dstRowPtr = outDstData;
	
	for(y = 0; y < inHeight; y++)
	{
		dstPtr = dstRowPtr;
		
		for(x = 0; x < inWidth; x++)
		{
			pixel = *srcPtr++;
			
			IMmARGB1555_to_rgb(pixel, r, g, b);
			
			IMmRGB555_to_I8(pixel, r, g, b);
			
			*dstPtr++ = (UUtUns8)pixel;
		}
		
		dstRowPtr += dstRowBytes;
	}
	
	return UUcError_None;
}


static UUtError
IMiConvert_ARGB4444_to_RGB555(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtUns16	x, y;
	UUtUns16	a, r, g, b;
	
	UUtUns16*	srcPtr;
	UUtUns16*	dstPtr;
	UUtUns16	pixel;
	
	srcPtr = inSrcData;
	dstPtr = outDstData;
	
	for(y = 0; y < inHeight; y++)
	{
		for(x = 0; x < inWidth; x++)
		{
			pixel = *srcPtr++;
			
			IMmARGB4444_to_argb(pixel, a, r, g, b);
			
			if(a == 0xF)
			{
				r = (r << 1) | (r & 0x01);
				g = (g << 1) | (g & 0x01);
				b = (b << 1) | (b & 0x01);
				
				IMmARGB_to_ARGB1555(pixel, 0x1, r, g, b);
			}
			else
			{
				pixel = 0;
			}
			*dstPtr++ = pixel;
		}
	}
	
	return UUcError_None;
}

static UUtError
IMiConvert_Copy(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtUns32 numBytes;

	UUmAssert(inSrcType == inDstType);

	numBytes = IMrImage_ComputeSize(
					inSrcType,
					IMcNoMipMap,
					inWidth,
					inHeight);

	UUrMemory_MoveFast(inSrcData, outDstData, numBytes);

	return UUcError_None;
}

static void
IMiCopy_Row_ARGB8888(
	UUtUns16		inWidth,
	UUtUns32*		inSrcData,
	IMtDitherPixel*	outDstData)
{
	UUtUns16	x;
	UUtUns32	pixel;
	
	for(x = 0; x < inWidth; x++)
	{
		pixel = *inSrcData++;
		outDstData->a = (UUtInt16)IMmShiftDownAndMask8(pixel, 24);
		outDstData->r = (UUtInt16)IMmShiftDownAndMask8(pixel, 16);
		outDstData->g = (UUtInt16)IMmShiftDownAndMask8(pixel, 8);
		outDstData->b = (UUtInt16)IMmShiftDownAndMask8(pixel, 0);
		outDstData++;
	}
}

static void
IMiCopy_Row_RGB888(
	UUtUns16		inWidth,
	UUtUns32*		inSrcData,
	IMtDitherPixel*	outDstData)
{
	UUtUns16	x;
	UUtUns32	pixel;
	
	for(x = 0; x < inWidth; x++)
	{
		pixel = *inSrcData++;
		outDstData->a = 0xFF;
		outDstData->r = (UUtInt16)IMmShiftDownAndMask8(pixel, 16);
		outDstData->g = (UUtInt16)IMmShiftDownAndMask8(pixel, 8);
		outDstData->b = (UUtInt16)IMmShiftDownAndMask8(pixel, 0);
		outDstData++;
	}
}

// RGB555
static UUtError
IMiConvert_RGB555_to_ARGB4444(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtUns16	x, y;
	UUtInt16	r, g, b;
	
	UUtUns16*	srcPtr;
	UUtUns16*	dstPtr;
	UUtUns16	pixel;
	UUtInt16	bias;
	
	srcPtr = inSrcData;
	dstPtr = outDstData;
	
	for(y = 0; y < inHeight; y++)
	{
		for(x = 0; x < inWidth; x++)
		{
			pixel = *srcPtr++;
			
			IMmARGB1555_to_rgb(pixel, r, g, b);
			
			if(inDitherMode == IMcDitherMode_On)
			{
				// S.S. bias = IMgDitherMatrix[x & (IMcDitherDim-1)][y & (IMcDitherDim-1)] / 2;
				bias= IMgDitherMatrix[x & (IMcDitherDim-1)][y & (IMcDitherDim-1)] >> 1;
				r += bias;
				g += bias;
				b += bias;
				r = UUmPin(r, 0, 31);
				g = UUmPin(g, 0, 31);
				b = UUmPin(b, 0, 31);
			}
			
			r >>= 1;
			g >>= 1;
			b >>= 1;
			
			IMmARGB_to_ARGB1555(pixel, 0xF, r, g, b);
			
			*dstPtr++ = pixel;
		}
	}
	
	return UUcError_None;
}

static UUtError
IMiConvert_A8_to_RGBA4444(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtUns16	x, y;
	
	UUtUns8*	srcPtr;
	UUtUns16*	dstPtr;
	UUtUns16	pixel;
	
	srcPtr = inSrcData;
	dstPtr = outDstData;
	
	for(y = 0; y < inHeight; y++)
	{
		for(x = 0; x < inWidth; x++)
		{
			pixel = *srcPtr++;
			pixel = pixel >> 4;

			*dstPtr++ = pixel | 0xFFF0;
		}
	}
	
	return UUcError_None;
}

static UUtError
IMiConvert_A8_to_RGBA_Bytes(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtUns16	x, y;
	
	UUtUns8*	srcPtr;
	UUtUns8*	dstPtr;
	
	srcPtr = inSrcData;
	dstPtr = outDstData;
	
	for(y = 0; y < inHeight; y++)
	{
		for(x = 0; x < inWidth; x++)
		{
			UUtUns8 alpha = *srcPtr;
			
			dstPtr[0] = 0xff; // r
			dstPtr[1] = 0xff; // g
			dstPtr[2] = 0xff; // b
			dstPtr[3] = alpha;

			srcPtr += 1;
			dstPtr += 4;
		}
	}
	
	return UUcError_None;
}

static UUtError
IMiConvert_RGB555_to_ARGB1555(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtUns32		num_pixels;
	UUtUns32		*src_data;
	UUtUns32		*dst_data;
	UUtUns32		alphas;
	// calculate the number of pixels in the image
	num_pixels = inWidth * inHeight >> 1;
	
	// copy the data and add an alpha
	src_data = inSrcData;
	dst_data = outDstData;
	alphas = 0x80008000;
	
	while (num_pixels--)
		*dst_data++ = *src_data++ | alphas;
	
	return UUcError_None;
}

// ARGB8888
static UUtError
IMiConvert_ARGB8888_to_ARGB4444(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtUns16	x, y;
	UUtInt16	a, r, g, b;
	
	UUtUns32*	srcPtr;
	UUtUns16*	dstPtr;
	UUtUns32	pixel;
	UUtInt16	bias;
	
	srcPtr = inSrcData;
	dstPtr = outDstData;
	
	for(y = 0; y < inHeight; y++)
	{
		for(x = 0; x < inWidth; x++)
		{
			pixel = *srcPtr++;
			
			IMmARGB8888_to_argb(pixel, a, r, g, b);
			
			if(inDitherMode == IMcDitherMode_On)
			{
				bias = IMgDitherMatrix[x & (IMcDitherDim-1)][y & (IMcDitherDim-1)];
				a += bias;
				r += bias;
				g += bias;
				b += bias;
				a = UUmPin(a, 0, UUcMaxUns8);
				r = UUmPin(r, 0, UUcMaxUns8);
				g = UUmPin(g, 0, UUcMaxUns8);
				b = UUmPin(b, 0, UUcMaxUns8);
			}
			
			a >>= 4;
			r >>= 4;
			g >>= 4;
			b >>= 4;
			
			IMmARGB_to_ARGB4444(pixel, a, r, g, b);
			
			*dstPtr++ = (UUtUns16)pixel;
		}
	}
	
	return UUcError_None;
}

static UUtError
IMiConvert_ARGB8888_to_RGB555(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtUns16	x, y;
	UUtInt16	a, r, g, b;
	
	UUtUns32*	srcPtr;
	UUtUns16*	dstPtr;
	UUtUns32	pixel;
	UUtInt16	bias;
	
	srcPtr = inSrcData;
	dstPtr = outDstData;
	
	for(y = 0; y < inHeight; y++)
	{
		for(x = 0; x < inWidth; x++)
		{
			pixel = *srcPtr++;
			
			IMmARGB8888_to_argb(pixel, a, r, g, b);
			
			if(a == 0xFF)
			{
				if(inDitherMode == IMcDitherMode_On)
				{
					bias = IMgDitherMatrix[x & (IMcDitherDim-1)][y & (IMcDitherDim-1)];
					
					r += bias;
					g += bias;
					b += bias;
					
					r = UUmPin(r, 0, UUcMaxUns8);
					g = UUmPin(g, 0, UUcMaxUns8);
					b = UUmPin(b, 0, UUcMaxUns8);
				}
			
				r >>= 3;
				g >>= 3;
				b >>= 3;
				
				IMmARGB_to_ARGB1555(pixel, 0x1, r, g, b);
			}
			else
			{
				pixel = 0;
			}
			
			*dstPtr++ = (UUtUns16)pixel;
		}
	}
	
	return UUcError_None;
}

static UUtError
IMiConvert_ARGB8888_to_ARGB1555(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtUns16	x, y;
	UUtInt16	a, r, g, b;
	
	UUtUns32*	srcPtr;
	UUtUns16*	dstPtr;
	UUtUns32	pixel;
	UUtInt16	bias;
	
	srcPtr = inSrcData;
	dstPtr = outDstData;
	
	for(y = 0; y < inHeight; y++)
	{
		for(x = 0; x < inWidth; x++)
		{
			pixel = *srcPtr++;
			
			IMmARGB8888_to_argb(pixel, a, r, g, b);
			
			if(inDitherMode == IMcDitherMode_On)
			{
				bias = IMgDitherMatrix[x & (IMcDitherDim-1)][y & (IMcDitherDim-1)];
				r += bias;
				g += bias;
				b += bias;
				r = UUmPin(r, 0, UUcMaxUns8);
				g = UUmPin(g, 0, UUcMaxUns8);
				b = UUmPin(b, 0, UUcMaxUns8);
			}
			
			a = a == 0xFF;
			r >>= 3;
			g >>= 3;
			b >>= 3;
			
			IMmARGB_to_ARGB1555(pixel, a, r, g, b);
			
			*dstPtr++ = (UUtUns16)pixel;
		}
	}
	
	return UUcError_None;
}

// RGB888
static UUtError
IMiConvert_RGB888_to_ARGB4444(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtUns16	x, y;
	UUtInt16	a, r, g, b;
	
	UUtUns32*	srcPtr;
	UUtUns16*	dstPtr;
	UUtUns32	pixel;
	UUtInt16	bias;
	
	srcPtr = inSrcData;
	dstPtr = outDstData;
	
	for(y = 0; y < inHeight; y++)
	{
		for(x = 0; x < inWidth; x++)
		{
			pixel = *srcPtr++;
			
			IMmARGB8888_to_rgb(pixel, r, g, b);
			a = UUcMaxUns8;

			if(inDitherMode == IMcDitherMode_On)
			{
				bias = IMgDitherMatrix[x & (IMcDitherDim-1)][y & (IMcDitherDim-1)];
				
				r += bias;
				g += bias;
				b += bias;
				
				r = UUmPin(r, 0, UUcMaxUns8);
				g = UUmPin(g, 0, UUcMaxUns8);
				b = UUmPin(b, 0, UUcMaxUns8);
			}
			
			a >>= 4;
			r >>= 4;
			g >>= 4;
			b >>= 4;
			
			IMmARGB_to_ARGB4444(pixel, a, r, g, b);
			
			*dstPtr++ = (UUtUns16)pixel;
		}
	}
	
	return UUcError_None;
}

static UUtError
IMiConvert_RGB888_to_RGB555(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtUns16	x, y;
	UUtInt16	r, g, b;
	
	UUtUns32*	srcPtr;
	UUtUns16*	dstPtr;
	UUtUns32	pixel;
	UUtInt16	bias;
	
	srcPtr = inSrcData;
	dstPtr = outDstData;
	
	for(y = 0; y < inHeight; y++)
	{
		for(x = 0; x < inWidth; x++)
		{
			pixel = *srcPtr++;
			
			r = (UUtUns16)((pixel >> 16) & 0xFF);
			g = (UUtUns16)((pixel >> 8) & 0xFF);
			b = (UUtUns16)((pixel >> 0) & 0xFF);
			
			if(inDitherMode == IMcDitherMode_On)
			{
				bias = IMgDitherMatrix[x & (IMcDitherDim-1)][y & (IMcDitherDim-1)];
				r += bias;
				g += bias;
				b += bias;
				r = UUmPin(r, 0, UUcMaxUns8);
				g = UUmPin(g, 0, UUcMaxUns8);
				b = UUmPin(b, 0, UUcMaxUns8);
			}
			
			r >>= 3;
			g >>= 3;
			b >>= 3;
			
			*dstPtr++ = (r << 10) | (g << 5) | (b);
		}
	}

	
	return UUcError_None;
}

static UUtError
IMiConvert_I8_to_RGB555(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtUns16	x, y;
	
	UUtUns8*	srcPtr;
	UUtUns16*	dstPtr;
	UUtUns16	pixel;
	UUtInt16	bias;
	
	srcPtr = inSrcData;
	dstPtr = outDstData;
	
	for(y = 0; y < inHeight; y++)
	{
		for(x = 0; x < inWidth; x++)
		{
			pixel = *srcPtr++;
			
			if(inDitherMode == IMcDitherMode_On)
			{
				bias = IMgDitherMatrix[x & (IMcDitherDim-1)][y & (IMcDitherDim-1)];
				pixel += bias;
				pixel = UUmPin(pixel, 0, UUcMaxUns8);
			}
			
			pixel >>= 3;
			
			*dstPtr++ = (pixel << 10) | (pixel << 5) | (pixel);
		}
	}
	
	return UUcError_None;
}

static UUtError
IMiConvert_I8_to_RGB_Bytes(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtUns32	count = inWidth * inHeight;
	
	UUtUns8*	endPtr;
	UUtUns8*	srcPtr;
	UUtUns8*	dstPtr;
	UUtUns8		pixel;
	
	srcPtr = inSrcData;
	dstPtr = outDstData;
	endPtr = srcPtr + count;

	while(srcPtr < endPtr)
	{
		pixel = *srcPtr++;
						
		dstPtr[0] = pixel;
		dstPtr[1] = pixel;
		dstPtr[2] = pixel;

		dstPtr += 3;
	}
	
	return UUcError_None;
}

static UUtError
IMiConvert_RGB888_to_ARGB1555(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtUns16	x, y;
	UUtInt16	r, g, b;
	
	UUtUns32*	srcPtr;
	UUtUns16*	dstPtr;
	UUtUns32	pixel;
	UUtInt16	bias;
	
	srcPtr = inSrcData;
	dstPtr = outDstData;
	
	for(y = 0; y < inHeight; y++)
	{
		for(x = 0; x < inWidth; x++)
		{
			pixel = *srcPtr++;
			
			r = (UUtUns16)((pixel >> 16) & 0xFF);
			g = (UUtUns16)((pixel >> 8) & 0xFF);
			b = (UUtUns16)((pixel >> 0) & 0xFF);
			
			if(inDitherMode == IMcDitherMode_On)
			{
				bias = IMgDitherMatrix[x & (IMcDitherDim-1)][y & (IMcDitherDim-1)];
				r += bias;
				g += bias;
				b += bias;
				r = UUmPin(r, 0, UUcMaxUns8);
				g = UUmPin(g, 0, UUcMaxUns8);
				b = UUmPin(b, 0, UUcMaxUns8);
			}
			
			r >>= 3;
			g >>= 3;
			b >>= 3;
			
			*dstPtr++ = (1 << 15) | (r << 10) | (g << 5) | (b);
		}
	}
	
	return UUcError_None;
}

typedef void
(*IMtDXT1Block_Decode)(
	const UUtUns16 *inSrc,
	void *outRow0,
	void *outRow1,
	void *outRow2,
	void *outRow3);

static void DecodeDXT1Block_To_RGB565(
					const UUtUns16 *inSrc,
					UUtUns16 *outRow0,
					UUtUns16 *outRow1,
					UUtUns16 *outRow2,
					UUtUns16 *outRow3)
{
	UUtUns8 r0,g0,b0;
	UUtUns8 r1,g1,b1;
	UUtUns8 r2,g2,b2;
	UUtUns8 r3,g3,b3;
	UUtUns16 colors[4];

	colors[0] = *inSrc++;
	colors[1] = *inSrc++;

	r0 = (UUtUns8) IMmShiftDownAndMask5(colors[0], 11);
	g0 = (UUtUns8) IMmShiftDownAndMask6(colors[0],  5);
	b0 = (UUtUns8) IMmShiftDownAndMask5(colors[0],  0);

	r1 = (UUtUns8) IMmShiftDownAndMask5(colors[1], 11);
	g1 = (UUtUns8) IMmShiftDownAndMask6(colors[1],  5);
	b1 = (UUtUns8) IMmShiftDownAndMask5(colors[1],  0);

	// compute colors
	if (colors[0] > colors[1])
	{
		r2 = ((r0 * 2) + (r1 * 1) + 1) / 3;
		g2 = ((g0 * 2) + (g1 * 1)) / 3;
		b2 = ((b0 * 2) + (b1 * 1) + 1) / 3;

		r3 = ((r0 * 1) + (r1 * 2) + 1) / 3;
		g3 = ((g0 * 1) + (g1 * 2)) / 3;
		b3 = ((b0 * 1) + (b1 * 2) + 1) / 3;

		UUmAssert(r2 <= 0x1f);
		UUmAssert(g2 <= 0x3f);
		UUmAssert(b2 <= 0x1f);
		UUmAssert(r3 <= 0x1f);
		UUmAssert(g3 <= 0x3f);
		UUmAssert(b3 <= 0x1f);

		colors[2] = r2 << 11 | g2 << 5 | b2 << 0;
		colors[3] = r3 << 11 | g3 << 5 | b3 << 0;
	}
	else
	{
		/* S.S. r2 = (r0 + r1 + 1) / 2;
		g2 = (g0 + g1) / 2;
		b2 = (b0 + b1 + 1) / 2;*/
		r2 = (r0 + r1 + 1) >> 1;
		g2 = (g0 + g1) >> 1;
		b2 = (b0 + b1 + 1) >> 1;

		UUmAssert(r2 <= 0x1f);
		UUmAssert(g2 <= 0x3f);
		UUmAssert(b2 <= 0x1f);

		colors[2] = r2 << 11 | g2 << 5 | b2 << 0;
		colors[3] = 0x0000;
	}

	colors[0] = r0 << 11 | g0 << 5 | b0 << 0;
	colors[1] = r1 << 11 | g1 << 5 | b1 << 0;

	outRow0[0] = colors[IMmShiftDownAndMask2(*inSrc,0)];
	outRow0[1] = colors[IMmShiftDownAndMask2(*inSrc,2)];
	outRow0[2] = colors[IMmShiftDownAndMask2(*inSrc,4)];
	outRow0[3] = colors[IMmShiftDownAndMask2(*inSrc,6)];
	outRow0 += 4;

	outRow1[0] = colors[IMmShiftDownAndMask2(*inSrc,8)];
	outRow1[1] = colors[IMmShiftDownAndMask2(*inSrc,10)];
	outRow1[2] = colors[IMmShiftDownAndMask2(*inSrc,12)];
	outRow1[3] = colors[IMmShiftDownAndMask2(*inSrc,14)];
	outRow1 += 4;
	inSrc++;

	outRow2[0] = colors[IMmShiftDownAndMask2(*inSrc,0)];
	outRow2[1] = colors[IMmShiftDownAndMask2(*inSrc,2)];
	outRow2[2] = colors[IMmShiftDownAndMask2(*inSrc,4)];
	outRow2[3] = colors[IMmShiftDownAndMask2(*inSrc,6)];
	outRow2 += 4;

	outRow3[0] = colors[IMmShiftDownAndMask2(*inSrc,8)];
	outRow3[1] = colors[IMmShiftDownAndMask2(*inSrc,10)];
	outRow3[2] = colors[IMmShiftDownAndMask2(*inSrc,12)];
	outRow3[3] = colors[IMmShiftDownAndMask2(*inSrc,14)];
	outRow3 += 4;

	inSrc++;

	return;
}


static void DecodeDXT1Block_To_RGBA5551(
					const UUtUns16 *inSrc,
					UUtUns16 *outRow0,
					UUtUns16 *outRow1,
					UUtUns16 *outRow2,
					UUtUns16 *outRow3)
{
	UUtUns8 r0,g0,b0;
	UUtUns8 r1,g1,b1;
	UUtUns8 r2,g2,b2;
	UUtUns8 r3,g3,b3;
	UUtUns16 colors[4];

	colors[0] = *inSrc++;
	colors[1] = *inSrc++;

	r0 = (UUtUns8) IMmShiftDownAndMask5(colors[0], 11);
	g0 = (UUtUns8) IMmShiftDownAndMask6(colors[0],  5);
	b0 = (UUtUns8) IMmShiftDownAndMask5(colors[0],  0);

	r1 = (UUtUns8) IMmShiftDownAndMask5(colors[1], 11);
	g1 = (UUtUns8) IMmShiftDownAndMask6(colors[1],  5);
	b1 = (UUtUns8) IMmShiftDownAndMask5(colors[1],  0);

	// compute colors
	if (colors[0] > colors[1])
	{
		r2 = ((r0 * 2) + (r1 * 1) + 1) / 3;
		g2 = ((g0 * 2) + (g1 * 1)) / 6;
		b2 = ((b0 * 2) + (b1 * 1) + 1) / 3;

		r3 = ((r0 * 1) + (r1 * 2) + 1) / 3;
		g3 = ((g0 * 1) + (g1 * 2)) / 6;
		b3 = ((b0 * 1) + (b1 * 2) + 1) / 3;

		UUmAssert(r2 <= 0x1f);
		UUmAssert(g2 <= 0x1f);
		UUmAssert(b2 <= 0x1f);
		UUmAssert(r3 <= 0x1f);
		UUmAssert(g3 <= 0x1f);
		UUmAssert(b3 <= 0x1f);

		colors[2] = r2 << 11 | g2 << 6 | b2 << 1 | 0x0001;
		colors[3] = r3 << 11 | g3 << 6 | b3 << 1 | 0x0001;
	}
	else
	{
		/* S.S. r2 = (r0 + r1 + 1) / 2;
		g2 = (g0 + g1) / 4;
		b2 = (b0 + b1 + 1) / 2;*/
		r2 = (r0 + r1 + 1) >> 1;
		g2 = (g0 + g1) >> 2;
		b2 = (b0 + b1 + 1) >> 1;

		UUmAssert(r2 <= 0x1f);
		UUmAssert(g2 <= 0x1f);
		UUmAssert(b2 <= 0x1f);

		colors[2] = r2 << 11 | g2 << 6 | b2 << 1 | 0x0001;
		colors[3] = 0x0000;
	}

	colors[0] = r0 << 11 | (g0/2) << 6 | b0 << 1 | 0x0001;
	colors[1] = r1 << 11 | (g1/2) << 6 | b1 << 1 | 0x0001;

	outRow0[0] = colors[IMmShiftDownAndMask2(*inSrc,0)];
	outRow0[1] = colors[IMmShiftDownAndMask2(*inSrc,2)];
	outRow0[2] = colors[IMmShiftDownAndMask2(*inSrc,4)];
	outRow0[3] = colors[IMmShiftDownAndMask2(*inSrc,6)];
	outRow0 += 4;

	outRow1[0] = colors[IMmShiftDownAndMask2(*inSrc,8)];
	outRow1[1] = colors[IMmShiftDownAndMask2(*inSrc,10)];
	outRow1[2] = colors[IMmShiftDownAndMask2(*inSrc,12)];
	outRow1[3] = colors[IMmShiftDownAndMask2(*inSrc,14)];
	outRow1 += 4;
	inSrc++;

	outRow2[0] = colors[IMmShiftDownAndMask2(*inSrc,0)];
	outRow2[1] = colors[IMmShiftDownAndMask2(*inSrc,2)];
	outRow2[2] = colors[IMmShiftDownAndMask2(*inSrc,4)];
	outRow2[3] = colors[IMmShiftDownAndMask2(*inSrc,6)];
	outRow2 += 4;

	outRow3[0] = colors[IMmShiftDownAndMask2(*inSrc,8)];
	outRow3[1] = colors[IMmShiftDownAndMask2(*inSrc,10)];
	outRow3[2] = colors[IMmShiftDownAndMask2(*inSrc,12)];
	outRow3[3] = colors[IMmShiftDownAndMask2(*inSrc,14)];
	outRow3 += 4;

	inSrc++;

	return;
}

static void DecodeDXT1Block_To_ARGB1555(
					const UUtUns16 *inSrc,
					UUtUns16 *outRow0,
					UUtUns16 *outRow1,
					UUtUns16 *outRow2,
					UUtUns16 *outRow3)
{
	UUtUns8 r0,g0,b0;
	UUtUns8 r1,g1,b1;
	UUtUns8 r2,g2,b2;
	UUtUns8 r3,g3,b3;
	UUtUns16 colors[4];

	colors[0] = *inSrc++;
	colors[1] = *inSrc++;

	r0 = (UUtUns8) IMmShiftDownAndMask5(colors[0], 11);
	g0 = (UUtUns8) IMmShiftDownAndMask6(colors[0],  5);
	b0 = (UUtUns8) IMmShiftDownAndMask5(colors[0],  0);

	r1 = (UUtUns8) IMmShiftDownAndMask5(colors[1], 11);
	g1 = (UUtUns8) IMmShiftDownAndMask6(colors[1],  5);
	b1 = (UUtUns8) IMmShiftDownAndMask5(colors[1],  0);

	// compute colors
	if (colors[0] > colors[1])
	{
		r2 = ((r0 * 2) + (r1 * 1) + 1) / 3;
		g2 = ((g0 * 2) + (g1 * 1)) / 6;
		b2 = ((b0 * 2) + (b1 * 1) + 1) / 3;

		r3 = ((r0 * 1) + (r1 * 2) + 1) / 3;
		g3 = ((g0 * 1) + (g1 * 2)) / 6;
		b3 = ((b0 * 1) + (b1 * 2) + 1) / 3;

		UUmAssert(r2 <= 0x1f);
		UUmAssert(g2 <= 0x1f);
		UUmAssert(b2 <= 0x1f);
		UUmAssert(r3 <= 0x1f);
		UUmAssert(g3 <= 0x1f);
		UUmAssert(b3 <= 0x1f);

		colors[2] = 0x8000 | r2 << 10 | g2 << 5 | b2 << 0;
		colors[3] = 0x8000 | r3 << 10 | g3 << 5 | b3 << 0;
	}
	else
	{
		/* S.S. r2 = (r0 + r1 + 1) / 2;
		g2 = (g0 + g1) / 4;
		b2 = (b0 + b1 + 1) / 2;*/
		r2 = (r0 + r1 + 1) >> 1;
		g2 = (g0 + g1) >> 2;
		b2 = (b0 + b1 + 1) >> 1;

		UUmAssert(r2 <= 0x1f);
		UUmAssert(g2 <= 0x1f);
		UUmAssert(b2 <= 0x1f);

		colors[2] = 0x8000 | r2 << 10 | g2 << 5 | b2 << 0;
		colors[3] = 0x0000;
	}

	colors[0] = 0x8000 | r0 << 10 | (g0/2) << 5 | b0 << 0;
	colors[1] = 0x8000 | r1 << 10 | (g1/2) << 5 | b1 << 0;

	outRow0[0] = colors[IMmShiftDownAndMask2(*inSrc,0)];
	outRow0[1] = colors[IMmShiftDownAndMask2(*inSrc,2)];
	outRow0[2] = colors[IMmShiftDownAndMask2(*inSrc,4)];
	outRow0[3] = colors[IMmShiftDownAndMask2(*inSrc,6)];
	outRow0 += 4;

	outRow1[0] = colors[IMmShiftDownAndMask2(*inSrc,8)];
	outRow1[1] = colors[IMmShiftDownAndMask2(*inSrc,10)];
	outRow1[2] = colors[IMmShiftDownAndMask2(*inSrc,12)];
	outRow1[3] = colors[IMmShiftDownAndMask2(*inSrc,14)];
	outRow1 += 4;
	inSrc++;

	outRow2[0] = colors[IMmShiftDownAndMask2(*inSrc,0)];
	outRow2[1] = colors[IMmShiftDownAndMask2(*inSrc,2)];
	outRow2[2] = colors[IMmShiftDownAndMask2(*inSrc,4)];
	outRow2[3] = colors[IMmShiftDownAndMask2(*inSrc,6)];
	outRow2 += 4;

	outRow3[0] = colors[IMmShiftDownAndMask2(*inSrc,8)];
	outRow3[1] = colors[IMmShiftDownAndMask2(*inSrc,10)];
	outRow3[2] = colors[IMmShiftDownAndMask2(*inSrc,12)];
	outRow3[3] = colors[IMmShiftDownAndMask2(*inSrc,14)];
	outRow3 += 4;

	inSrc++;

	return;
}

static void DecodeDXT1Block_To_RGBBytes(
					const UUtUns16 *inSrc,
					void *outRow0,
					void *outRow1,
					void *outRow2,
					void *outRow3)
{
	UUtUns8 r0,g0,b0;
	UUtUns8 r1,g1,b1;
	UUtUns8 r2,g2,b2;
	UUtUns8 r3,g3,b3;
	UUtUns16 colors[2];
	UUtUns8 rgb_colors[4][3];

	char *out0 = (char *) outRow0;
	char *out1 = (char *) outRow1;
	char *out2 = (char *) outRow2;
	char *out3 = (char *) outRow3;

	colors[0] = *inSrc++;
	colors[1] = *inSrc++;

	r0 = (UUtUns8) IMmShiftDownAndMask5(colors[0], 11);
	g0 = (UUtUns8) IMmShiftDownAndMask6(colors[0],  5);
	b0 = (UUtUns8) IMmShiftDownAndMask5(colors[0],  0);

	r1 = (UUtUns8) IMmShiftDownAndMask5(colors[1], 11);
	g1 = (UUtUns8) IMmShiftDownAndMask6(colors[1],  5);
	b1 = (UUtUns8) IMmShiftDownAndMask5(colors[1],  0);

	// compute colors
	if (colors[0] > colors[1])
	{
		r2 = ((r0 * 2) + (r1 * 1) + 1) / 3;
		g2 = ((g0 * 2) + (g1 * 1)) / 3;
		b2 = ((b0 * 2) + (b1 * 1) + 1) / 3;

		r3 = ((r0 * 1) + (r1 * 2) + 1) / 3;
		g3 = ((g0 * 1) + (g1 * 2)) / 3;
		b3 = ((b0 * 1) + (b1 * 2) + 1) / 3;

		UUmAssert(r2 <= 0x1f);
		UUmAssert(g2 <= 0x3f);
		UUmAssert(b2 <= 0x1f);
		UUmAssert(r3 <= 0x1f);
		UUmAssert(g3 <= 0x3f);
		UUmAssert(b3 <= 0x1f);

		rgb_colors[2][0] = r2 << 3;
		rgb_colors[2][1] = g2 << 2;
		rgb_colors[2][2] = b2 << 3;

		rgb_colors[3][0] = r3 << 3;
		rgb_colors[3][1] = g3 << 2;
		rgb_colors[3][2] = b3 << 3;
	}
	else
	{
		/* S.S. r2 = (r0 + r1 + 1) / 2;
		g2 = (g0 + g1) / 2;
		b2 = (b0 + b1 + 1) / 2;*/
		r2 = (r0 + r1 + 1) >> 1;
		g2 = (g0 + g1) >> 1;
		b2 = (b0 + b1 + 1) >> 1;

		UUmAssert(r2 <= 0x1f);
		UUmAssert(g2 <= 0x3f);
		UUmAssert(b2 <= 0x1f);

		rgb_colors[2][0] = r2 << 3;
		rgb_colors[2][1] = g2 << 2;
		rgb_colors[2][2] = b2 << 3;

		rgb_colors[3][0] = 0;
		rgb_colors[3][1] = 0;
		rgb_colors[3][2] = 0;
	}

	rgb_colors[0][0] = r0 << 3;
	rgb_colors[0][1] = g0 << 2;
	rgb_colors[0][2] = b0 << 3;

	rgb_colors[1][0] = r1 << 3;
	rgb_colors[1][1] = g1 << 2;
	rgb_colors[1][2] = b1 << 3;

	UUrMemory_MoveFast(rgb_colors[IMmShiftDownAndMask2(*inSrc,0)], out0 + 0, 3);
	UUrMemory_MoveFast(rgb_colors[IMmShiftDownAndMask2(*inSrc,2)], out0 + 3, 3);
	UUrMemory_MoveFast(rgb_colors[IMmShiftDownAndMask2(*inSrc,4)], out0 + 6, 3);
	UUrMemory_MoveFast(rgb_colors[IMmShiftDownAndMask2(*inSrc,6)], out0 + 9, 3);
	
	UUrMemory_MoveFast(rgb_colors[IMmShiftDownAndMask2(*inSrc,8)], out1 + 0, 3);
	UUrMemory_MoveFast(rgb_colors[IMmShiftDownAndMask2(*inSrc,10)], out1 + 3, 3);
	UUrMemory_MoveFast(rgb_colors[IMmShiftDownAndMask2(*inSrc,12)], out1 + 6, 3);
	UUrMemory_MoveFast(rgb_colors[IMmShiftDownAndMask2(*inSrc,14)], out1 + 9, 3);
	inSrc++;

	UUrMemory_MoveFast(rgb_colors[IMmShiftDownAndMask2(*inSrc,0)], out2 + 0, 3);
	UUrMemory_MoveFast(rgb_colors[IMmShiftDownAndMask2(*inSrc,2)], out2 + 3, 3);
	UUrMemory_MoveFast(rgb_colors[IMmShiftDownAndMask2(*inSrc,4)], out2 + 6, 3);
	UUrMemory_MoveFast(rgb_colors[IMmShiftDownAndMask2(*inSrc,6)], out2 + 9, 3);
	
	UUrMemory_MoveFast(rgb_colors[IMmShiftDownAndMask2(*inSrc,8)], out3 + 0, 3);
	UUrMemory_MoveFast(rgb_colors[IMmShiftDownAndMask2(*inSrc,10)], out3 + 3, 3);
	UUrMemory_MoveFast(rgb_colors[IMmShiftDownAndMask2(*inSrc,12)], out3 + 6, 3);
	UUrMemory_MoveFast(rgb_colors[IMmShiftDownAndMask2(*inSrc,14)], out3 + 9, 3);
	inSrc++;

	return;
}

static void DecodeDXT1Block_To_RGBABytes(
					const UUtUns16 *inSrc,
					UUtUns16 *outRow0,
					UUtUns16 *outRow1,
					UUtUns16 *outRow2,
					UUtUns16 *outRow3)
{
	UUtUns8 r0,g0,b0;
	UUtUns8 r1,g1,b1;
	UUtUns8 r2,g2,b2;
	UUtUns8 r3,g3,b3;
	UUtUns16 colors[2];
	UUtUns8 rgba_colors[4][4];

	char *out0 = (char *) outRow0;
	char *out1 = (char *) outRow1;
	char *out2 = (char *) outRow2;
	char *out3 = (char *) outRow3;

	colors[0] = *inSrc++;
	colors[1] = *inSrc++;

	r0 = (UUtUns8) IMmShiftDownAndMask5(colors[0], 11);
	g0 = (UUtUns8) IMmShiftDownAndMask6(colors[0],  5);
	b0 = (UUtUns8) IMmShiftDownAndMask5(colors[0],  0);

	r1 = (UUtUns8) IMmShiftDownAndMask5(colors[1], 11);
	g1 = (UUtUns8) IMmShiftDownAndMask6(colors[1],  5);
	b1 = (UUtUns8) IMmShiftDownAndMask5(colors[1],  0);

	// compute colors
	if (colors[0] > colors[1])
	{
		r2 = ((r0 * 2) + (r1 * 1) + 1) / 3;
		g2 = ((g0 * 2) + (g1 * 1)) / 3;
		b2 = ((b0 * 2) + (b1 * 1) + 1) / 3;

		r3 = ((r0 * 1) + (r1 * 2) + 1) / 3;
		g3 = ((g0 * 1) + (g1 * 2)) / 3;
		b3 = ((b0 * 1) + (b1 * 2) + 1) / 3;

		UUmAssert(r2 <= 0x1f);
		UUmAssert(g2 <= 0x3f);
		UUmAssert(b2 <= 0x1f);
		UUmAssert(r3 <= 0x1f);
		UUmAssert(g3 <= 0x3f);
		UUmAssert(b3 <= 0x1f);

		rgba_colors[2][0] = r2 << 3;
		rgba_colors[2][1] = g2 << 2;
		rgba_colors[2][2] = b2 << 3;
		rgba_colors[2][3] = 0xff;

		rgba_colors[3][0] = r3 << 3;
		rgba_colors[3][1] = g3 << 2;
		rgba_colors[3][2] = b3 << 3;
		rgba_colors[3][3] = 0xff;
	}
	else
	{
		/* S.S. r2 = (r0 + r1 + 1) / 2;
		g2 = (g0 + g1) / 2;
		b2 = (b0 + b1 + 1) / 2;*/
		r2 = (r0 + r1 + 1) >> 1;
		g2 = (g0 + g1) >> 1;
		b2 = (b0 + b1 + 1) >> 1;

		UUmAssert(r2 <= 0x1f);
		UUmAssert(g2 <= 0x3f);
		UUmAssert(b2 <= 0x1f);

		rgba_colors[2][0] = r2 << 3;
		rgba_colors[2][1] = g2 << 2;
		rgba_colors[2][2] = b2 << 3;
		rgba_colors[2][3] = 0xff;

		rgba_colors[3][0] = 0;
		rgba_colors[3][1] = 0;
		rgba_colors[3][2] = 0;
		rgba_colors[3][3] = 0xff;
	}

	rgba_colors[0][0] = r0 << 3;
	rgba_colors[0][1] = g0 << 2;
	rgba_colors[0][2] = b0 << 3;
	rgba_colors[0][3] = 0xff;

	rgba_colors[1][0] = r1 << 3;
	rgba_colors[1][1] = g1 << 2;
	rgba_colors[1][2] = b1 << 3;
	rgba_colors[1][3] = 0xff;

	UUrMemory_MoveFast(rgba_colors[IMmShiftDownAndMask2(*inSrc,0)], out0 + 0, 4);
	UUrMemory_MoveFast(rgba_colors[IMmShiftDownAndMask2(*inSrc,2)], out0 + 4, 4);
	UUrMemory_MoveFast(rgba_colors[IMmShiftDownAndMask2(*inSrc,4)], out0 + 8, 4);
	UUrMemory_MoveFast(rgba_colors[IMmShiftDownAndMask2(*inSrc,6)], out0 + 12, 4);
	
	UUrMemory_MoveFast(rgba_colors[IMmShiftDownAndMask2(*inSrc,8)], out1 + 0, 4);
	UUrMemory_MoveFast(rgba_colors[IMmShiftDownAndMask2(*inSrc,10)], out1 + 4, 4);
	UUrMemory_MoveFast(rgba_colors[IMmShiftDownAndMask2(*inSrc,12)], out1 + 8, 4);
	UUrMemory_MoveFast(rgba_colors[IMmShiftDownAndMask2(*inSrc,14)], out1 + 12, 4);
	inSrc++;

	UUrMemory_MoveFast(rgba_colors[IMmShiftDownAndMask2(*inSrc,0)], out2 + 0, 4);
	UUrMemory_MoveFast(rgba_colors[IMmShiftDownAndMask2(*inSrc,2)], out2 + 4, 4);
	UUrMemory_MoveFast(rgba_colors[IMmShiftDownAndMask2(*inSrc,4)], out2 + 8, 4);
	UUrMemory_MoveFast(rgba_colors[IMmShiftDownAndMask2(*inSrc,6)], out2 + 12, 4);
	
	UUrMemory_MoveFast(rgba_colors[IMmShiftDownAndMask2(*inSrc,8)], out3 + 0, 4);
	UUrMemory_MoveFast(rgba_colors[IMmShiftDownAndMask2(*inSrc,10)], out3 + 4, 4);
	UUrMemory_MoveFast(rgba_colors[IMmShiftDownAndMask2(*inSrc,12)], out3 + 8, 4);
	UUrMemory_MoveFast(rgba_colors[IMmShiftDownAndMask2(*inSrc,14)], out3 + 12, 4);
	inSrc++;

	return;
}
static void BuildColors(UUtUns16 colors[4])
{
	UUtUns16 r0,r1,r2,r3;
	UUtUns16 g0,g1,g2,g3;
	UUtUns16 b0,b1,b2,b3;

	r0 = (UUtUns8) IMmShiftDownAndMask5(colors[0], 11);
	g0 = (UUtUns8) IMmShiftDownAndMask6(colors[0],  5);
	b0 = (UUtUns8) IMmShiftDownAndMask5(colors[0],  0);

	r1 = (UUtUns8) IMmShiftDownAndMask5(colors[1], 11);
	g1 = (UUtUns8) IMmShiftDownAndMask6(colors[1],  5);
	b1 = (UUtUns8) IMmShiftDownAndMask5(colors[1],  0);

	// compute colors
	r2 = ((r0 * 2) + (r1 * 1) + 1) / 3;
	g2 = ((g0 * 2) + (g1 * 1)) / 6;
	b2 = ((b0 * 2) + (b1 * 1) + 1) / 3;

	r3 = ((r0 * 1) + (r1 * 2) + 1) / 3;
	g3 = ((g0 * 1) + (g1 * 2)) / 6;
	b3 = ((b0 * 1) + (b1 * 2) + 1) / 3;

	UUmAssert(r2 <= 0x1f);
	UUmAssert(g2 <= 0x1f);
	UUmAssert(b2 <= 0x1f);
	UUmAssert(r3 <= 0x1f);
	UUmAssert(g3 <= 0x1f);
	UUmAssert(b3 <= 0x1f);

	colors[2] = 0x8000 | r2 << 10 | g2 << 5 | b2 << 0;
	colors[3] = 0x8000 | r3 << 10 | g3 << 5 | b3 << 0;

	return;
}

static UUtUns16 SelectColor(
				UUtUns16 colors[4],
				UUtUns16 wish)
{
	const UUtUns16 rmask = 0x1f << 11;
	const UUtUns16 gmask = 0x3f <<  5;
	const UUtUns16 bmask = 0x1f <<  0;

	UUtInt16 rwish = (wish & (UUtUns16)(0x1f << 10)) >> 10;
	UUtInt16 gwish = (wish & (UUtUns16)(0x1f <<  5)) >> 5;
	UUtInt16 bwish = (wish & (UUtUns16)(0x1f <<  0)) >> 0;
	
	UUtInt16 choice = 0;
	UUtInt16 delta = UUcMaxInt16;
	UUtUns8 itr;

	for(itr = 0; itr < 4; itr++)
	{
		UUtInt16 r,g,b;
		UUtInt16 curDelta;

		r = (colors[itr] & rmask) >> 11;
		g = (colors[itr] & gmask) >> 6;
		b = (colors[itr] & bmask) >> 0;
		
		curDelta = abs(r - rwish); 
		curDelta += abs(g - gwish);
		curDelta += abs(b - bwish);

		if (curDelta < delta)
		{
			choice = itr;
			delta = curDelta;
		}
	}

	return choice;
}

UUtInt16 static Color555_Distance(UUtUns16 c1, UUtUns16 c2)
{
	const UUtUns16 rmask = 0x1f << 11;
	const UUtUns16 gmask = 0x3f <<  5;
	const UUtUns16 bmask = 0x1f <<  0;
	UUtInt16 r1,g1,b1,r2,g2,b2;
	UUtInt16 dist;

	r1 = (c1 & rmask) >> 10;
	g1 = (c1 & gmask) >>  5;
	b1 = (c1 & bmask) >>  0;

	r2 = (c2 & rmask) >> 10;
	g2 = (c2 & gmask) >>  5;
	b2 = (c2 & bmask) >>  0;

	dist = abs(r1 - r2) + abs(g1 - g2) + abs(b1 - b2);

	return dist;
}

static UUtUns16 FindFarColor(
					UUtUns16 inColor,
					const UUtUns16 *inRow0,
					const UUtUns16 *inRow1,
					const UUtUns16 *inRow2,
					const UUtUns16 *inRow3)
{
	UUtUns16 farColor;
	UUtInt16 delta = UUcMinInt16;
	UUtInt16 curDelta;
	const UUtUns16 *row;
	UUtUns8 rowItr, itr;

	for(rowItr = 0; rowItr < 4; rowItr++)
	{
		switch(rowItr)
		{
			case 0: row = inRow0; break;
			case 1: row = inRow1; break;
			case 2: row = inRow2; break;
			case 3: row = inRow3; break;
		}

		for(itr = 0; itr < 4; itr++)
		{
			curDelta = Color555_Distance(inColor, row[itr]);

			if (curDelta > delta) {
				farColor = row[itr];
				delta = curDelta;
			}
		}
	}

	return farColor;
}

static void EncodeDXT1Block_From_ARGB1555(
					const UUtUns16 *inRow0,
					const UUtUns16 *inRow1,
					const UUtUns16 *inRow2,
					const UUtUns16 *inRow3,
					UUtUns16 *outDst)
{ 
	UUtUns16 rgb565_1, rgb565_2;
	UUtUns16 rgb555_1, rgb555_2;
	
	// find the two best colors
	rgb555_1 = FindFarColor(inRow0[0], inRow0, inRow1, inRow2, inRow3);
	rgb555_2 = FindFarColor(rgb555_1, inRow0, inRow1, inRow2, inRow3);
	rgb555_1 = FindFarColor(rgb555_2, inRow0, inRow1, inRow2, inRow3);

	// turn into 565
	{
		UUtUns16 r,g,b,r2,g2,b2;

		r = rgb555_1 & (UUtUns16) (0x1f << 10);
		g = rgb555_1 & (UUtUns16) (0x1f <<  5);
		b = rgb555_1 & (UUtUns16) (0x1f <<  0);
		r2 = rgb555_2 & (UUtUns16) (0x1f << 10);
		g2 = rgb555_2 & (UUtUns16) (0x1f <<  5);
		b2 = rgb555_2 & (UUtUns16) (0x1f <<  0);
		
		rgb565_1 = (r << 1) | (g << 1) | (b << 0);
		rgb565_2 = (r2 << 1) | (g2 << 1) | (b2 << 0);
	}

	// stub encoding
	if (rgb565_1 == rgb565_2)
	{
		*outDst++ = rgb565_1;
		*outDst++ = rgb565_2;
		*outDst++ = 0x0000;
		*outDst++ = 0x0000;
	}
	else 
	{
		UUtUns16 colors[4];

		if (rgb565_1 > rgb565_2)
		{
			colors[0] = rgb565_1;
			colors[1] = rgb565_2;
		}
		else
		{
			colors[0] = rgb565_2;
			colors[1] = rgb565_1;
		}

		BuildColors(colors);

		*outDst++ = colors[0];
		*outDst++ = colors[1];

		*outDst = SelectColor(colors, inRow0[0]) << 0;
		*outDst |= SelectColor(colors, inRow0[1]) << 2;
		*outDst |= SelectColor(colors, inRow0[2]) << 4;
		*outDst |= SelectColor(colors, inRow0[3]) << 6;

		*outDst |= SelectColor(colors, inRow1[0]) << 8;
		*outDst |= SelectColor(colors, inRow1[1]) << 10;
		*outDst |= SelectColor(colors, inRow1[2]) << 12;
		*outDst |= SelectColor(colors, inRow1[3]) << 14;
		outDst++;

		*outDst = SelectColor(colors, inRow2[0]) << 0;
		*outDst |= SelectColor(colors, inRow2[1]) << 2;
		*outDst |= SelectColor(colors, inRow2[2]) << 4;
		*outDst |= SelectColor(colors, inRow2[3]) << 6;

		*outDst |= SelectColor(colors, inRow3[0]) << 8;
		*outDst |= SelectColor(colors, inRow3[1]) << 10;
		*outDst |= SelectColor(colors, inRow3[2]) << 12;
		*outDst |= SelectColor(colors, inRow3[3]) << 14;
		outDst++;
	}

	return;
}

static UUtError
IMiConvert_ARGB1555_to_DXT1(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{

	// S.S. UUtUns16 blockWidth = (inWidth + 3) / 4;
	//UUtUns16 blockHeight = (inHeight + 3) / 4;
	UUtUns16 blockWidth = (inWidth + 3) >> 2;
	UUtUns16 blockHeight = (inHeight + 3) >> 2;
	UUtUns16 *curInPtr = inSrcData;
	UUtUns16 *curOutPtr = outDstData;
	UUtUns16 x,y;
	UUtUns16 *rows[4];

	UUmAssert((IMcPixelType_RGB555 == inSrcType) || (IMcPixelType_ARGB1555 == inSrcType));
	UUmAssert(IMcPixelType_DXT1 == inDstType);

	if ((inWidth < 4) || (inHeight < 4))
	{
		for(y = 0; y < blockHeight; y++)
		{
			rows[0] = curInPtr;
			rows[1] = rows[0] + inWidth;
			rows[2] = rows[1] + inWidth;
			rows[3] = rows[2] + inWidth;

			for(x = 0; x < blockWidth; x++) 
			{
				UUtUns16 src_x, src_y;
				UUtUns16 dst_x, dst_y;
				UUtUns16 data[4][4];

				UUrMemory_Set16(data, rows[0][0], sizeof(UUtUns16) * 4 * 4); 

				// copy, the clipped part in
				for(src_y = 0, dst_y = y * 4; dst_y < UUmMin(y * 4 + 4, inHeight); src_y++, dst_y++)
				{
					for(src_x = 0, dst_x = y * 4; dst_x < UUmMin(y * 4 + 4, inHeight); src_x++, dst_x++)
					{
						data[src_y][src_x] = *(rows[src_y] + src_x);
					}
				}

				EncodeDXT1Block_From_ARGB1555(data[0], data[1], data[2], data[3], curOutPtr);

				curOutPtr += 4;
				rows[0] += 4;
				rows[1] += 4;
				rows[2] += 4;
				rows[3] += 4;
			}

			curInPtr  += 4 * inWidth;
		}
	}
	else
	{
		UUmAssert(0 == (inWidth % 4));
		UUmAssert(0 == (inHeight % 4));

		for(y = 0; y < blockHeight; y++)
		{
			rows[0] = curInPtr;
			rows[1] = rows[0] + inWidth;
			rows[2] = rows[1] + inWidth;
			rows[3] = rows[2] + inWidth;

			for(x = 0; x < blockWidth; x++) 
			{
				EncodeDXT1Block_From_ARGB1555(rows[0], rows[1], rows[2], rows[3], curOutPtr);

				curOutPtr += 4;
				rows[0] += 4;
				rows[1] += 4;
				rows[2] += 4;
				rows[3] += 4;
			}

			curInPtr  += 4 * inWidth;
		}
	}

	return UUcError_None;
}

static UUtError
IMiConvert_DXT1_to_NByte(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{

	// S.S. UUtUns16 blockWidth = (inWidth + 3) / 4;
	//UUtUns16 blockHeight = (inHeight + 3) / 4;
	UUtUns16 blockWidth = (inWidth + 3) >> 2;
	UUtUns16 blockHeight = (inHeight + 3) >> 2;
	const UUtUns16 *curInPtr = inSrcData;
	void *curOutPtr = outDstData;
	UUtUns16 x,y;
	IMtDXT1Block_Decode block_decode = NULL;
	UUtInt32 dst_size;

	UUmAssert(IMcPixelType_DXT1 == inSrcType);

	switch(inDstType)
	{
		case IMcPixelType_RGB555:
		case IMcPixelType_ARGB1555:
			block_decode = (IMtDXT1Block_Decode)DecodeDXT1Block_To_ARGB1555;
		break;

		case IMcPixelType_RGBA5551:
			block_decode = (IMtDXT1Block_Decode)DecodeDXT1Block_To_RGBA5551;
		break;

		case IMcPixelType_RGB565:
			block_decode = (IMtDXT1Block_Decode)DecodeDXT1Block_To_RGB565;
		break;

		case IMcPixelType_RGB_Bytes:
			block_decode = (IMtDXT1Block_Decode)DecodeDXT1Block_To_RGBBytes;
		break;

		case IMcPixelType_RGBA_Bytes:
			block_decode = (IMtDXT1Block_Decode)DecodeDXT1Block_To_RGBABytes;
		break;

		default:
			return UUcError_Generic;
	}

	dst_size = IMrPixel_GetSize(inDstType);

	if ((inWidth < 4) || (inHeight < 4))
	{
		for(y = 0; y < blockHeight; y++)
		{
			void *rows[4];

			rows[0] = curOutPtr;
			rows[1] = ((char *) rows[0]) + (inWidth * dst_size);
			rows[2] = ((char *) rows[1]) + (inWidth * dst_size);
			rows[3] = ((char *) rows[2]) + (inWidth * dst_size);

			for(x = 0; x < blockWidth; x++) 
			{
				UUtUns16 src_x, src_y;
				UUtUns16 dst_x, dst_y;
				UUtUns32 data_block[16];	// largest data size possible
				void *data[4];

				data[0] = data_block + 0;
				data[1] = data_block + 4;
				data[2] = data_block + 8;
				data[3] = data_block + 12;

				UUmAssert(dst_size <= 4);	// data_block assumes that

				block_decode(curInPtr, data[0], data[1], data[2], data[3]);

				// copy, the clipped part out
				for(src_y = 0, dst_y = y * 4; dst_y < UUmMin(y * 4 + 4, inHeight); src_y++, dst_y++)
				{
					for(src_x = 0, dst_x = y * 4; dst_x < UUmMin(x * 4 + 4, inWidth); src_x++, dst_x++)
					{
						void *copy_dst = ((char *) rows[src_y]) + src_x * dst_size;
						void *copy_src = ((char *) data[src_y]) + src_x * dst_size;

						UUrMemory_MoveFast(copy_src, copy_dst, dst_size);
					}
				}

				curInPtr += 4;

				rows[0] = ((char *) rows[0]) + (4 * dst_size);
				rows[1] = ((char *) rows[1]) + (4 * dst_size);
				rows[2] = ((char *) rows[2]) + (4 * dst_size);
				rows[3] = ((char *) rows[3]) + (4 * dst_size);
			}

			curOutPtr = ((char *) curOutPtr) + (4 * inWidth * dst_size);
		}
	}
	else
	{
		UUmAssert(0 == (inWidth % 4));
		UUmAssert(0 == (inHeight % 4));

		for(y = 0; y < blockHeight; y++)
		{
			void *row0,*row1,*row2,*row3;

			row0 = curOutPtr;
			row1 = ((char *) row0) + (inWidth * dst_size);
			row2 = ((char *) row1) + (inWidth * dst_size);
			row3 = ((char *) row2) + (inWidth * dst_size);

			for(x = 0; x < blockWidth; x++) 
			{
				block_decode(curInPtr, row0, row1, row2, row3);

				curInPtr += 4;

				row0 = ((char *) row0) + (4 * dst_size);
				row1 = ((char *) row1) + (4 * dst_size);
				row2 = ((char *) row2) + (4 * dst_size);
				row3 = ((char *) row3) + (4 * dst_size);
			}

			curOutPtr = ((char *) curOutPtr) + (4 * inWidth * dst_size);
		}
	}

	return UUcError_None;
}


void
IMrPixel_Convert(
	IMtPixelType	inSrcPixelType,
	IMtPixel		inSrcPixel,
	IMtPixelType	inDstPixelType,
	IMtPixel		*outDstPixel)
{
	UUtUns8			a;
	UUtUns8			r;
	UUtUns8			g;
	UUtUns8			b;
	
	switch (inSrcPixelType)
	{
		case IMcPixelType_ARGB4444:
			IMmARGB4444_to_argb(inSrcPixel.value, a, r, g, b);
			switch (inDstPixelType)
			{
				case IMcPixelType_ARGB4444:
					*outDstPixel = inSrcPixel;
				break;
				
				case IMcPixelType_RGB555:
					r <<= 1;
					g <<= 1;
					b <<= 1;
					IMmRGB_to_RGB555(outDstPixel->value, r, g, b);
				break;
				
				case IMcPixelType_ARGB1555:
					(a > 0) ? (a = 1) : (a = 0);
					r <<= 1;
					g <<= 1;
					b <<= 1;
					IMmARGB_to_ARGB1555(outDstPixel->value, a, r, g, b);
				break;
				
				case IMcPixelType_I8:
				case IMcPixelType_I1:
				case IMcPixelType_A8:
				case IMcPixelType_A4I4:
					UUmAssert(!"Unimplemented");
				break;
				
				case IMcPixelType_ARGB8888:
					a <<= 4;
					r <<= 4;
					g <<= 4;
					b <<= 4;
					IMmARGB_to_ARGB8888(outDstPixel->value, a, r, g, b);
				break;
				
				case IMcPixelType_RGB888:
					r <<= 4;
					g <<= 4;
					b <<= 4;
					IMmRGB_to_RGB888(outDstPixel->value, r, g, b);
				break;
			}
		break;
		
		case IMcPixelType_RGB555:
			IMmRGB555_to_rgb(inSrcPixel.value, r, g, b);
			switch (inDstPixelType)
			{
				case IMcPixelType_ARGB4444:
					a = 0xF;
					r >>= 1;
					g >>= 1;
					b >>= 1;
					IMmARGB_to_ARGB4444(outDstPixel->value, a, r, g, b);
				break;
				
				case IMcPixelType_RGB555:
					*outDstPixel = inSrcPixel;
				break;
				
				case IMcPixelType_ARGB1555:
					outDstPixel->value = inSrcPixel.value | 0x8000;
				break;
				
				case IMcPixelType_I8:
				case IMcPixelType_I1:
				case IMcPixelType_A8:
				case IMcPixelType_A4I4:
					UUmAssert(!"Unimplemented");
				break;
				
				case IMcPixelType_ARGB8888:
					a = 0xFF;
					r <<= 3;
					g <<= 3;
					b <<= 3;
					IMmARGB_to_ARGB8888(outDstPixel->value, a, r, g, b);
				break;
				
				case IMcPixelType_RGB888:
					r <<= 3;
					g <<= 3;
					b <<= 3;
					IMmRGB_to_RGB888(outDstPixel->value, r, g, b);
				break;
			}
		break;
		
		case IMcPixelType_ARGB1555:
			IMmARGB1555_to_argb(inSrcPixel.value, a, r, g, b);
			switch (inDstPixelType)
			{
				case IMcPixelType_ARGB4444:
					(a == 1) ? (a = 0xF) : (a = 0);
					r >>= 1;
					g >>= 1;
					b >>= 1;
					IMmARGB_to_ARGB4444(outDstPixel->value, a, r, g, b);
				break;
				
				case IMcPixelType_RGB555:
					*outDstPixel = inSrcPixel;
				break;
				
				case IMcPixelType_ARGB1555:
					*outDstPixel = inSrcPixel;
				break;
				
				case IMcPixelType_I8:
				case IMcPixelType_I1:
				case IMcPixelType_A8:
				case IMcPixelType_A4I4:
					UUmAssert(!"Unimplemented");
				break;
				
				case IMcPixelType_ARGB8888:
					(a == 1) ? (a = 0xFF) : (a = 0);
					r <<= 3;
					g <<= 3;
					b <<= 3;
					IMmARGB_to_ARGB8888(outDstPixel->value, a, r, g, b);
				break;
				
				case IMcPixelType_RGB888:
					r <<= 3;
					g <<= 3;
					b <<= 3;
					IMmRGB_to_RGB888(outDstPixel->value, r, g, b);
				break;
			}
		break;
		
		case IMcPixelType_I8:
		case IMcPixelType_I1:
		case IMcPixelType_A8:
		case IMcPixelType_A4I4:
			UUmAssert(!"Unimplemented");
		break;
		
		case IMcPixelType_ARGB8888:
			IMmARGB8888_to_argb(inSrcPixel.value, a, r, g, b);
			switch (inDstPixelType)
			{
				case IMcPixelType_ARGB4444:
					a >>= 4;
					r >>= 4;
					g >>= 4;
					b >>= 4;
					IMmARGB_to_ARGB4444(outDstPixel->value, a, r, g, b);
				break;
				
				case IMcPixelType_RGB555:
					r >>= 3;
					g >>= 3;
					b >>= 3;
					IMmRGB_to_RGB555(outDstPixel->value, r, g, b);
				break;
				
				case IMcPixelType_ARGB1555:
					(a > 0) ? (a = 1) : (a = 0);
					r >>= 3;
					g >>= 3;
					b >>= 3;
					IMmARGB_to_ARGB1555(outDstPixel->value, a, r, g, b);
				break;
				
				case IMcPixelType_I8:
				case IMcPixelType_I1:
				case IMcPixelType_A8:
				case IMcPixelType_A4I4:
					UUmAssert(!"Unimplemented");
				break;
				
				case IMcPixelType_ARGB8888:
					*outDstPixel = inSrcPixel;
				break;
				
				case IMcPixelType_RGB888:
					*outDstPixel = inSrcPixel;
				break;
			}
		break;
		
		case IMcPixelType_RGB888:
			IMmRGB888_to_rgb(inSrcPixel.value, r, g, b);
			a = 0xFF;
			switch (inDstPixelType)
			{
				case IMcPixelType_ARGB4444:
					a = 0xF;
					r >>= 4;
					g >>= 4;
					b >>= 4;
					IMmARGB_to_ARGB4444(outDstPixel->value, a, r, g, b);
				break;
				
				case IMcPixelType_RGB555:
					a = 0xF;
					r >>= 3;
					g >>= 3;
					b >>= 3;
					IMmRGB_to_RGB555(outDstPixel->value, r, g, b);
				break;
				
				case IMcPixelType_ARGB1555:
					a = 1;
					r >>= 3;
					g >>= 3;
					b >>= 3;
					IMmARGB_to_ARGB1555(outDstPixel->value, a, r, g, b);
				break;
				
				case IMcPixelType_I8:
				case IMcPixelType_I1:
				case IMcPixelType_A8:
				case IMcPixelType_A4I4:
					UUmAssert(!"Unimplemented");
				break;
				
				case IMcPixelType_ARGB8888:
					*outDstPixel = inSrcPixel;
				break;
				
				case IMcPixelType_RGB888:
					*outDstPixel = inSrcPixel;
				break;
			}
		break;
	}
}


UUtError
IMrImage_ConvertPixelType(
	IMtDitherMode	inDitherMode,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	IMtMipMap		inMipMap,
	IMtPixelType	inSrcPixelType,
	void*			inSrcData,
	IMtPixelType	inDstPixelType,
	void*			outDstData)
{
	UUtError error;
	IMtConvertPixelType_Proc conversionProc;

	// this could go to an initialize function some day
	if (!IMgConvertPixelType_Table_Built) {
		iBuild_ConvertPixelType_Table();
	}

	conversionProc = IMgConvertPixelType_Table[inDstPixelType][inSrcPixelType];
	
	if (NULL != conversionProc) 
	{
		UUtUns16 width = inWidth;
		UUtUns16 height = inHeight;
		UUtUns8 *src = (UUtUns8 *) inSrcData;
		UUtUns8 *dst = (UUtUns8 *) outDstData;

		do
		{
			error = conversionProc(
				inDitherMode,
				inSrcPixelType,
				inDstPixelType,
				width,
				height,
				src,
				dst);

			// advance
			src += IMrImage_ComputeSize(inSrcPixelType, IMcNoMipMap, width, height);
			dst += IMrImage_ComputeSize(inDstPixelType, IMcNoMipMap, width, height);

			width /= 2;
			height /= 2;

			if ((width > 0) || (height > 0)) {
				width = UUmMax(width, 1);
				height = UUmMax(height, 1);
			}
		} while ((width > 0) && (height > 0) && (IMcHasMipMap == inMipMap));
	}
	else
	{
		UUmAssert(!"unsupplied conversion proc");
		error = UUcError_Generic;
	}

	return error;
}


static UUtError
IMiConvert_ARGB4444_to_ARGB8888(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtUns16	x, y;
	UUtUns32	a, r, g, b;
	
	UUtUns16*	srcPtr;
	UUtUns32*	dstPtr;
	UUtUns16	srcPixel;
	UUtUns32	dstPixel;
	
	UUmAssertReadPtr(inSrcData, 2 * inWidth * inHeight);
	UUmAssertWritePtr(outDstData, 4 * inWidth * inHeight);

	srcPtr = inSrcData;
	dstPtr = outDstData;
	
	for(y = 0; y < inHeight; y++)
	{
		for(x = 0; x < inWidth; x++)
		{
			srcPixel = *srcPtr++;

			a = (srcPixel & 0xf000) >> 12;
			r = (srcPixel & 0x0f00) >> 8;
			g = (srcPixel & 0x00f0) >> 4;
			b = (srcPixel & 0x000f) >> 0;

			a = (a << 4) | a;
			r = (r << 4) | r;
			g = (g << 4) | g;
			b = (b << 4) | b;
			
			dstPixel = (a << 24) | (r << 16) | (g << 8) | (b << 0);

			*dstPtr++ = dstPixel;
		}
	}
	
	return UUcError_None;
}

static UUtError
IMiConvert_RGB555_to_RGB888(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtUns16	x, y;
	UUtUns16	r, g, b;
	
	UUtUns16*	srcPtr;
	UUtUns32*	dstPtr;
	UUtUns16	pixel;
	UUtUns32	dstPixel;
	
	UUmAssertReadPtr(inSrcData, 2 * inWidth * inHeight);
	UUmAssertWritePtr(outDstData, 4 * inWidth * inHeight);

	srcPtr = inSrcData;
	dstPtr = outDstData;
	
	for(y = 0; y < inHeight; y++)
	{
		for(x = 0; x < inWidth; x++)
		{
			pixel = *srcPtr++;
			
			IMmARGB1555_to_rgb(pixel, r, g, b);
			
			r = (r << 3) | r;
			g = (g << 3) | g;
			b = (b << 3) | b;
			
			IMmARGB_to_ARGB8888(dstPixel, 0xFF, r, g, b);
			
			*dstPtr++ = dstPixel;
		}
	}
	
	return UUcError_None;
}

static UUtError
IMiConvert_RGB555_to_RGB_Bytes(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtUns16	x, y;
	UUtUns8	r, g, b;
	
	UUtUns16*	srcPtr;
	UUtUns8*	dstPtr;
	UUtUns16	pixel;
	
	UUmAssertReadPtr(inSrcData, 2 * inWidth * inHeight);
	UUmAssertWritePtr(outDstData, 4 * inWidth * inHeight);

	srcPtr = inSrcData;
	dstPtr = outDstData;
	
	for(y = 0; y < inHeight; y++)
	{
		for(x = 0; x < inWidth; x++)
		{
			pixel = srcPtr[0];
			
			r = (UUtUns8) (((pixel & (0x3f << 10)) >> 10) << 3);
			g = (UUtUns8) (((pixel & (0x3f <<  5)) >>  5) << 3);
			b = (UUtUns8) (((pixel & (0x3f <<  0)) >>  0) << 3);
						
			dstPtr[0] = r;
			dstPtr[1] = g;
			dstPtr[2] = b;

			srcPtr += 1;
			dstPtr += 3;
		}
	}
	
	return UUcError_None;
}

static UUtError
IMiConvert_RGB555_to_RGBA_Bytes(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtUns16	x, y;
	UUtUns8	r, g, b;
	
	UUtUns16*	srcPtr;
	UUtUns8*	dstPtr;
	UUtUns16	pixel;
	
	UUmAssertReadPtr(inSrcData, 2 * inWidth * inHeight);
	UUmAssertWritePtr(outDstData, 4 * inWidth * inHeight);

	srcPtr = inSrcData;
	dstPtr = outDstData;
	
	for(y = 0; y < inHeight; y++)
	{
		for(x = 0; x < inWidth; x++)
		{
			pixel = srcPtr[0];
			
			r = (UUtUns8) (((pixel & (0x3f << 10)) >> 10) << 3);
			g = (UUtUns8) (((pixel & (0x3f <<  5)) >>  5) << 3);
			b = (UUtUns8) (((pixel & (0x3f <<  0)) >>  0) << 3);
						
			dstPtr[0] = r;
			dstPtr[1] = g;
			dstPtr[2] = b;
			dstPtr[3] = 0xff;

			srcPtr += 1;
			dstPtr += 4;
		}
	}
	
	return UUcError_None;
}
static UUtError
IMiConvert_ARGB1555_to_ARGB8888(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtUns16	x, y;
	UUtUns16	r, g, b;
	
	UUtUns16*	srcPtr;
	UUtUns32*	dstPtr;
	UUtUns16	pixel;
	UUtUns32	dstPixel;
	
	UUmAssertReadPtr(inSrcData, 2 * inWidth * inHeight);
	UUmAssertWritePtr(outDstData, 4 * inWidth * inHeight);

	srcPtr = inSrcData;
	dstPtr = outDstData;
	
	for(y = 0; y < inHeight; y++)
	{
		for(x = 0; x < inWidth; x++)
		{
			pixel = *srcPtr++;
			
			IMmARGB1555_to_rgb(pixel, r, g, b);
			
			r = (r << 3) | r;
			g = (g << 3) | g;
			b = (b << 3) | b;
			
			IMmARGB_to_ARGB8888(dstPixel, 0xFF, r, g, b);
			
			*dstPtr++ = dstPixel;
		}
	}
	
	return UUcError_None;
}

static UUtError
IMiConvert_ARGB4444_to_RGBA_Bytes(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtInt32 loop;
	UUtInt32 count = inWidth * inHeight;
	UUtUns16 *pSrc = inSrcData;
	UUtUns8 *pDst = outDstData;

	for(loop = 0; loop < count; loop++) 
	{
		UUtUns8 a,r,g,b;

		a = ((pSrc[0] & 0xf000) >> 12);
		r = ((pSrc[0] & 0x0f00) >> 8);
		g = ((pSrc[0] & 0x00f0) >> 4);
		b = ((pSrc[0] & 0x000f) >> 0);

		a = (a << 4) | a;
		r = (r << 4) | r;
		g = (g << 4) | g;
		b = (b << 4) | b;

		pDst[0] = r;
		pDst[1] = g;
		pDst[2] = b;
		pDst[3] = a;

		pSrc += 1;
		pDst += 4;
	}

	return UUcError_None;
}

static UUtError
IMiConvert_ARGB1555_to_RGBA_Bytes(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtInt32 loop;
	UUtInt32 count = inWidth * inHeight;
	UUtUns16 *pSrc = inSrcData;
	UUtUns8 *pDst = outDstData;

	for(loop = 0; loop < count; loop++) 
	{
		UUtUns8 a,r,g,b;

		a = ((pSrc[0] & 0x8000) >> 15);
		r = ((pSrc[0] & 0x7c00) >> 10);
		g = ((pSrc[0] & 0x03e0) >> 5);
		b = ((pSrc[0] & 0x001f) >> 0);

		a = a ? 0xff : 0x00;
		r = (r << 3) | (r >> 2);
		g = (g << 3) | (g >> 2);
		b = (b << 3) | (b >> 2);

		pDst[0] = r;
		pDst[1] = g;
		pDst[2] = b;
		pDst[3] = a;

		pSrc += 1;
		pDst += 4;
	}

	return UUcError_None;
}



static UUtError
IMiConvert_RGB555_to_RGBA_5551(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtInt32 loop;
	UUtInt32 count = inWidth * inHeight;
	UUtUns16 *pSrc = inSrcData;
	UUtUns16 *pDst = outDstData;

	for(loop = 0; loop < count; loop++) 
	{
		UUtUns16 block;

		block = *pSrc;
		block = ((block & 0x7fff) << 1) | 0x0001;
		*pDst = block;

		pSrc += 1;
		pDst += 1;
	}

	return UUcError_None;
}

static UUtError
IMiConvert_ARGB1555_to_RGBA_5551(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtInt32 loop;
	UUtInt32 count = inWidth * inHeight;
	UUtUns16 *pSrc = inSrcData;
	UUtUns16 *pDst = outDstData;

	for(loop = 0; loop < count; loop++) 
	{
		UUtUns16 block;

		block = *pSrc;
		block = ((block & 0x7fff) << 1) | ((block & 0x8000) >> 15);
		*pDst = block;

		pSrc += 1;
		pDst += 1;
	}

	return UUcError_None;
}


static UUtError
IMiConvert_ARGB4444_to_RGBA_4444(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtInt32 loop;
	UUtInt32 count = inWidth * inHeight;
	UUtUns16 *pSrc = inSrcData;
	UUtUns16 *pDst = outDstData;

	for(loop = 0; loop < count; loop++) 
	{
		UUtUns16 block;

		block = *pSrc;
		block = ((block & 0x0fff) << 4) | ((block & 0xf000) >> 12);
		*pDst = block;

		pSrc += 1;
		pDst += 1;
	}

	return UUcError_None;
}

static UUtError IMiConvert_RGB888_to_RGB_Bytes(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)

{
	UUtInt32 loop;
	UUtInt32 count = inWidth * inHeight;
	UUtUns32 *pSrc = inSrcData;
	UUtUns8 *pDst = outDstData;

	for(loop = 0; loop < count; loop++) 
	{
		UUtUns32 chunk = *pSrc;
		UUtUns32 r = (chunk >> 16) & 0xFF;
		UUtUns32 g = (chunk >> 8) & 0xFF;
		UUtUns32 b = (chunk >> 0) & 0xFF;

		pDst[0] = (UUtUns8) r;
		pDst[1] = (UUtUns8) g;
		pDst[2] = (UUtUns8) b;

		pSrc += 1;
		pDst += 3;
	}

	return UUcError_None;
}


static UUtError  IMiConvert_ARGB8888_to_RGB888(
	IMtDitherMode	inDitherMode,
	IMtPixelType	inSrcType,
	IMtPixelType	inDstType,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	void*			inSrcData,
	void*			outDstData)
{
	UUtUns32 itr;
	UUtUns32 count = inWidth * inHeight;
	const UUtUns32 *pSrc = inSrcData;
	UUtUns32 *pDst = outDstData;

	for(itr = 0; itr < count; itr++)
	{
		*pDst++ = *pSrc++ | 0xFF000000;
	}

	return UUcError_None;
}


static IMtConverPixelType_LookupEntry IMgConvertPixelType_List[] =
{
	{	IMcPixelType_ARGB4444,	IMcPixelType_ARGB4444,	IMiConvert_Copy },
	{	IMcPixelType_RGB555,	IMcPixelType_RGB555,	IMiConvert_Copy },
	{	IMcPixelType_ARGB1555,	IMcPixelType_ARGB1555,	IMiConvert_Copy },
	{	IMcPixelType_I8,		IMcPixelType_I8,		IMiConvert_Copy },
	{	IMcPixelType_I8,		IMcPixelType_RGB555,	IMiConvert_I8_to_RGB555 },
	{	IMcPixelType_I8,		IMcPixelType_RGB_Bytes,	IMiConvert_I8_to_RGB_Bytes },
	{	IMcPixelType_I1,		IMcPixelType_I1,		IMiConvert_Copy },
	{	IMcPixelType_A8,		IMcPixelType_A8,		IMiConvert_Copy },
	{	IMcPixelType_A4I4,		IMcPixelType_A4I4,		IMiConvert_Copy },
	{	IMcPixelType_ARGB8888,	IMcPixelType_ARGB8888,	IMiConvert_Copy },
	{	IMcPixelType_RGB888,	IMcPixelType_RGB888,	IMiConvert_Copy },
	{	IMcPixelType_DXT1,		IMcPixelType_DXT1,		IMiConvert_Copy },
	{	IMcPixelType_RGB_Bytes, IMcPixelType_RGB_Bytes, IMiConvert_Copy },

	{	IMcPixelType_A8,		IMcPixelType_RGBA4444,	IMiConvert_A8_to_RGBA4444 },
	{	IMcPixelType_A8,		IMcPixelType_RGBA_Bytes,IMiConvert_A8_to_RGBA_Bytes },

	{	IMcPixelType_ARGB4444,	IMcPixelType_RGB555,	IMiConvert_ARGB4444_to_RGB555},
	{	IMcPixelType_ARGB4444,	IMcPixelType_ARGB8888,	IMiConvert_ARGB4444_to_ARGB8888},
	{	IMcPixelType_ARGB4444,	IMcPixelType_RGBA_Bytes,IMiConvert_ARGB4444_to_RGBA_Bytes},

	{	IMcPixelType_RGB555,	IMcPixelType_ARGB4444,	IMiConvert_RGB555_to_ARGB4444},
	{	IMcPixelType_RGB555,	IMcPixelType_ARGB1555,	IMiConvert_RGB555_to_ARGB1555},
	{	IMcPixelType_RGB555,	IMcPixelType_RGB888,	IMiConvert_RGB555_to_RGB888},
	{	IMcPixelType_RGB555,	IMcPixelType_RGB_Bytes,	IMiConvert_RGB555_to_RGB_Bytes},
	{	IMcPixelType_RGB555,	IMcPixelType_RGBA_Bytes,IMiConvert_RGB555_to_RGBA_Bytes},

	{	IMcPixelType_ARGB1555,	IMcPixelType_ARGB8888,	IMiConvert_ARGB1555_to_ARGB8888},
	{	IMcPixelType_ARGB1555,	IMcPixelType_RGBA_Bytes,	IMiConvert_ARGB1555_to_RGBA_Bytes},

	{	IMcPixelType_ARGB8888,	IMcPixelType_ARGB4444,	IMiConvert_ARGB8888_to_ARGB4444},
	{	IMcPixelType_ARGB8888,	IMcPixelType_RGB555,	IMiConvert_ARGB8888_to_RGB555},
	{	IMcPixelType_ARGB8888,	IMcPixelType_ARGB1555,	IMiConvert_ARGB8888_to_ARGB1555},
	{	IMcPixelType_ARGB8888,	IMcPixelType_RGB888,	IMiConvert_ARGB8888_to_RGB888},

	{	IMcPixelType_RGB888,	IMcPixelType_ARGB4444,	IMiConvert_RGB888_to_ARGB4444},
	{	IMcPixelType_RGB888,	IMcPixelType_RGB555,	IMiConvert_RGB888_to_RGB555},
	{	IMcPixelType_RGB888,	IMcPixelType_ARGB1555,	IMiConvert_RGB888_to_ARGB1555},

	{	IMcPixelType_DXT1,		IMcPixelType_RGB555,	IMiConvert_DXT1_to_NByte},
	{	IMcPixelType_DXT1,		IMcPixelType_ARGB1555,	IMiConvert_DXT1_to_NByte},
	{	IMcPixelType_DXT1,		IMcPixelType_RGB565,	IMiConvert_DXT1_to_NByte},
	{	IMcPixelType_DXT1,		IMcPixelType_RGBA5551,	IMiConvert_DXT1_to_NByte},
	{	IMcPixelType_DXT1,		IMcPixelType_RGB_Bytes,	IMiConvert_DXT1_to_NByte},
	{	IMcPixelType_DXT1,		IMcPixelType_RGBA_Bytes,IMiConvert_DXT1_to_NByte},

	//{	IMcPixelType_DXT1,		IMcPixelType_RGB_Bytes,	IMiConvert_DXT1_to_RGB_Bytes},
	
	{	IMcPixelType_ARGB1555,	IMcPixelType_DXT1,		IMiConvert_ARGB1555_to_DXT1},
	{	IMcPixelType_RGB555,	IMcPixelType_DXT1,		IMiConvert_ARGB1555_to_DXT1},

	{	IMcPixelType_ARGB1555,	IMcPixelType_I8,		IMiConvert_RGB555_to_I8},
	{	IMcPixelType_RGB555,	IMcPixelType_I8,		IMiConvert_RGB555_to_I8},

	{	IMcPixelType_ARGB1555,	IMcPixelType_RGBA5551,	IMiConvert_ARGB1555_to_RGBA_5551 },
	{	IMcPixelType_RGB555,	IMcPixelType_RGBA5551,	IMiConvert_RGB555_to_RGBA_5551 },
	{	IMcPixelType_ARGB4444,	IMcPixelType_RGBA4444,	IMiConvert_ARGB4444_to_RGBA_4444 },

	{	IMcPixelType_RGB888,	IMcPixelType_RGB_Bytes,	IMiConvert_RGB888_to_RGB_Bytes },

	{	IMcNumPixelTypes,		IMcNumPixelTypes,		NULL}
};


static void iBuild_ConvertPixelType_Table(void)
{
	IMtConverPixelType_LookupEntry *curEntry;

	// set table to NULL
	UUrMemory_Clear(IMgConvertPixelType_Table, sizeof(IMgConvertPixelType_Table));

	// build the table
	for(curEntry = IMgConvertPixelType_List; curEntry->function != NULL; curEntry++)
	{
		UUmAssert(curEntry->to < IMcNumPixelTypes);
		UUmAssert(curEntry->from < IMcNumPixelTypes);
		UUmAssert(NULL == IMgConvertPixelType_Table[curEntry->to][curEntry->from]);
		
		IMgConvertPixelType_Table[curEntry->to][curEntry->from] = curEntry->function;
	}

	// final entry should be num pixel types, num pixel types, NULL
	UUmAssert(IMcNumPixelTypes == curEntry->to);
	UUmAssert(IMcNumPixelTypes == curEntry->from);

	// mark the table as built
	IMgConvertPixelType_Table_Built = UUcTrue;

	return;
}