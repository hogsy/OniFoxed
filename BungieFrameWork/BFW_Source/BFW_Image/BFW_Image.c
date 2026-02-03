/*
	FILE:	BFW_Image.c

	AUTHOR:	Brent H. Pease

	CREATED: Nov 27, 1998

	PURPOSE:

	Copyright 1998
*/

#include "BFW.h"
#include "BFW_Image.h"
#include "BFW_Image_Private.h"
#include "BFW_MathLib.h"
#include "BFW_AppUtilities.h"
#include "BFW_Timer.h"

// ----------------------------------------------------------------------
typedef struct IMtPixelTypeInfo_Lookup
{
	IMtPixelType	type;
	UUtBool			maskValid;
	UUtUns8			sizePerPixel;

	UUtUns32		aMask;
	UUtUns32		rMask;
	UUtUns32		gMask;
	UUtUns32		bMask;

} IMtPixelTypeInfo_Lookup;

// table must be in order
const IMtPixelTypeInfo_Lookup
IMgPixelTypeInfoTable[] =
{
	{ IMcPixelType_ARGB4444,		UUcTrue,	2,     0xf000,     0x0f00,     0x00f0,     0x000f },
	{ IMcPixelType_RGB555,			UUcTrue,	2,     0x0000,     0x7c00,     0x03e0,     0x001f },
	{ IMcPixelType_ARGB1555,		UUcTrue,	2,     0x8000,     0x7c00,     0x03e0,     0x001f },
	{ IMcPixelType_I8,				UUcTrue,	1,       0x00,       0xff,       0xff,       0xff },
	{ IMcPixelType_I1,				UUcFalse,	1,       0x00,       0x01,       0x01,       0x01 },
	{ IMcPixelType_A8,				UUcTrue,	1,       0xff,       0x00,       0x00,       0x00 },
	{ IMcPixelType_A4I4,			UUcTrue,	1,       0xf0,       0x0f,       0x0f,       0x0f },
	{ IMcPixelType_ARGB8888,		UUcTrue,	4, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff },
	{ IMcPixelType_RGB888,			UUcTrue,	4, 0x00000000, 0x00ff0000, 0x0000ff00, 0x000000ff },
	{ IMcPixelType_DXT1,			UUcFalse,	0,			0,			0,			0,			0 },
	{ IMcPixelType_RGB_Bytes,		UUcFalse,	3,			3,			0,			0,			0 },
	{ IMcPixelType_RGBA_Bytes,		UUcFalse,	4,			4,			0,			0,			0 },
	{ IMcPixelType_RGBA5551,		UUcTrue,	2,     0x0001,     0xf800,     0x07c0,     0x003e },
	{ IMcPixelType_RGBA4444,		UUcTrue,	2,     0x000f,     0xf000,     0x0f00,     0x00f0 },
	{ IMcPixelType_RGB565,			UUcTrue,	2,     0x0000,     0xf800,     0x07e0,     0x001f },
	{ IMcPixelType_ABGR1555,		UUcTrue,	2,     0x8000,     0x001f,     0x03e0,     0x7c00 }
};

UUtUns16	IMgSizeofPixelInfoTable = sizeof(IMgPixelTypeInfoTable) / sizeof(IMtPixelTypeInfo_Lookup);

// ----------------------------------------------------------------------
static const IMtPixelTypeInfo_Lookup*
IMiPixel_LookupTypeInfo(
	IMtPixelType inPixelType)
{
	const IMtPixelTypeInfo_Lookup *lookup = NULL;

	UUmAssert(inPixelType < IMgSizeofPixelInfoTable);
	UUmAssert(IMgSizeofPixelInfoTable == IMcNumPixelTypes);
	UUmAssert(IMgPixelTypeInfoTable[inPixelType].type == inPixelType);

	if (IMgPixelTypeInfoTable[inPixelType].maskValid) {
		lookup = IMgPixelTypeInfoTable + inPixelType;
	}

	return lookup;
}

UUtUns8
IMrPixel_GetSize(
	IMtPixelType inPixelType)
{
	const IMtPixelTypeInfo_Lookup *lookup = NULL;

	UUmAssert(inPixelType < IMgSizeofPixelInfoTable);
	UUmAssert(IMgSizeofPixelInfoTable == IMcNumPixelTypes);
	UUmAssert(IMgPixelTypeInfoTable[inPixelType].type == inPixelType);

	return IMgPixelTypeInfoTable[inPixelType].sizePerPixel;
}

// ----------------------------------------------------------------------
static UUtUns32 IMiMaskToShift(UUtUns32 mask)
{
	UUtUns32 itr;

	if (0 == mask)
	{
		return 0;
	}

	for(itr = 0; itr < 32; itr++)
	{
		if ((1 << itr) & mask)
		{
			return itr;
		}
	}

	UUmAssert(0);

	return 0;
}

// ----------------------------------------------------------------------
static UUtUns32 IMiComponentToBits(float component, UUtUns32 mask)
{
	UUtUns32 shift = IMiMaskToShift(mask);
	UUtUns32 shiftedMask = mask >> shift;
	UUtUns32 result;

	if (0 == mask)
	{
		return 0;
	}

	result = MUrUnsignedSmallFloat_To_Int_Round(component * shiftedMask);
	UUmAssert(result == UUmPin(result, 0, shiftedMask));
	result = result << shift;

	return result;
}

// ----------------------------------------------------------------------
static float IMiBitsToComponent(UUtUns32 bits, UUtUns32 mask)
{
	UUtUns32 shift = IMiMaskToShift(mask);
	float result;

	if (0 == mask)
	{
		return 1.f;
	}

	result =  ( ((float) (bits >> shift)) / ((float) (mask >> shift)) );

	return result;
}

static UUtError
IMiImage_Scale_High(
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	IMtPixelType	inSrcPixelType,
	void*			inSrcData,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	void*			outDstData)
{
	switch(inSrcPixelType)
	{
		case IMcPixelType_ARGB4444:
			break;

		case IMcPixelType_RGB555:
			break;

		case IMcPixelType_ARGB1555:
			break;

		case IMcPixelType_I8:
			break;

		case IMcPixelType_I1:
			break;

		case IMcPixelType_A8:
			break;

		case IMcPixelType_A4I4:
			break;

		case IMcPixelType_ARGB8888:
			break;

		case IMcPixelType_RGB888:
			break;

		default:
			UUmError_ReturnOnError(UUcError_Generic);
	}

	return UUcError_None;
}

UUtBool
IMrPixelType_HasAlpha(
	IMtPixelType	inPixelType)
{
	const IMtPixelTypeInfo_Lookup *info;
	UUtBool hasAlpha;

	info = IMiPixel_LookupTypeInfo(inPixelType);

	if (NULL != info) {
		hasAlpha = (info->aMask) != 0;
	}
	else {
		switch(inPixelType)
		{
			case IMcPixelType_I1:
			case IMcPixelType_DXT1:
				hasAlpha = UUcFalse;
				break;

			default:
				UUmAssert(0);
		}
	}

	return hasAlpha;
}

UUtError
IMrImage_Scale(
	IMtScaleMode	inScaleMode,
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	IMtPixelType	inSrcPixelType,
	void*			inSrcData,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	void*			outDstData)
{

	if(inScaleMode & IMcScaleMode_High)
	{
		return
			IMiImage_Scale_High(
				inSrcWidth,
				inSrcHeight,
				inSrcPixelType,
				inSrcData,
				inDstWidth,
				inDstHeight,
				outDstData);
	}
	else
	{
		if(inScaleMode & IMcScaleMode_IndependantAlpha)
		{
			return
				IMrImage_Scale_Box_IndependantAlpha(
					inSrcWidth,
					inSrcHeight,
					inSrcPixelType,
					inSrcData,
					inDstWidth,
					inDstHeight,
					outDstData);
		}
		else
		{
			return
				IMrImage_Scale_Box(
					inSrcWidth,
					inSrcHeight,
					inSrcPixelType,
					inSrcData,
					inDstWidth,
					inDstHeight,
					outDstData);
		}
	}

	return UUcError_None;
}

void
IMrImage_Copy(
	UUtUns16		inCopyWidth,
	UUtUns16		inCopyHeight,
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	IMtPixelType	inSrcPixelType,
	void			*inSrcData,
	IMtPoint2D		*inSrcLocation,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	IMtPixelType	inDstPixelType,
	void			*inDstData,
	IMtPoint2D		*inDstLocation)
{
	UUtUns16		src_rowbytes;
	UUtUns16		src_pixel_size;
	UUtUns32		src;
	UUtRect			src_bounds;
	UUtRect			src_rect;
	UUtUns32		src_read;

	UUtUns16		dst_rowbytes;
	UUtUns16		dst_pixel_size;
	UUtUns32		dst;
	UUtRect			dst_bounds;
	UUtRect			dst_rect;
	UUtUns32		dst_write;

	UUtRect			temp;
	UUtUns16 		width;

	// clip src_rect to the image
	temp.left = inSrcLocation->x;
	temp.top = inSrcLocation->y;
	temp.right = temp.left + inCopyWidth;
	temp.bottom = temp.top + inCopyHeight;
	src_bounds.left = 0;
	src_bounds.top = 0;
	src_bounds.right = inSrcWidth;
	src_bounds.bottom = inSrcHeight;
	IMrRect_Intersect(&temp, &src_bounds, &src_rect);

	// clip inDstRect to the image
	temp.left = inDstLocation->x;
	temp.top = inDstLocation->y;
	temp.right = temp.left + inCopyWidth;
	temp.bottom = temp.top + inCopyHeight;
	dst_bounds.left = 0;
	dst_bounds.top = 0;
	dst_bounds.right = inDstWidth;
	dst_bounds.bottom = inDstHeight;
	IMrRect_Intersect(&temp, &dst_bounds, &dst_rect);

	// get the pixel sizes
	src_pixel_size = IMrPixel_GetSize(inSrcPixelType);
	dst_pixel_size = IMrPixel_GetSize(inDstPixelType);

	// calculate the src start
	src_rowbytes = IMrImage_ComputeRowBytes(inSrcPixelType, inSrcWidth);
	src =
		(UUtUns32)inSrcData +
		(src_rect.left * src_pixel_size) +
		(src_rect.top * src_rowbytes);

	// calculate the dst start
	dst_rowbytes = IMrImage_ComputeRowBytes(inDstPixelType, inDstWidth);
	dst =
		(UUtUns32)inDstData +
		(dst_rect.left * dst_pixel_size) +
		(dst_rect.top * dst_rowbytes);

	// if the pixel types are the same, the pixel doesn't have to
	// be converted between copy and past
	if (inSrcPixelType == inDstPixelType)
	{
		switch (src_pixel_size)
		{
			case 1:
				while (inCopyHeight--)
				{
					width = inCopyWidth;
					src_read = src;
					dst_write = dst;

					while (width--)
 					{
 						*((UUtUns8*)dst_write) = *((UUtUns8*)src_read);
 						dst++;
 						src_read++;
 					}
 					dst_write += src_rowbytes;
 					dst += dst_rowbytes;
				}
			break;

			case 2:
				while (inCopyHeight--)
				{
					width = inCopyWidth;
					src_read = src;
					dst_write = dst;

					while (width--)
 					{
 						*((UUtUns16*)dst_write) = *((UUtUns16*)src_read);
 						dst_write += 2;
 						src_read += 2;
 					}
 					src += src_rowbytes;
 					dst += dst_rowbytes;
				}
			break;

			case 4:
				while (inCopyHeight--)
				{
					width = inCopyWidth;
					src_read = src;
					dst_write = dst;

					while (width--)
 					{
 						*((UUtUns32*)dst_write) = *((UUtUns32*)src_read);
 						dst_write += 4;
 						src_read += 4;
 					}
 					src += src_rowbytes;
 					dst += dst_rowbytes;
				}
			break;
		}
	}
	else
	{
		IMtPixel		src_pixel;
		IMtPixel		dst_pixel;

		switch (src_pixel_size)
		{
			case 1:
				while (inCopyHeight--)
				{
					width = inCopyWidth;
					src_read = src;
					dst_write = dst;

					while (width--)
 					{
 						src_pixel.value = *((UUtUns8*)src_read);
 						IMrPixel_Convert(
 							inSrcPixelType,
 							src_pixel,
 							inDstPixelType,
 							&dst_pixel);
 						*((UUtUns8*)dst_write) = (UUtUns8)dst_pixel.value;
 						dst_write++;
 						src_read++;
 					}
 					src += src_rowbytes;
 					dst += dst_rowbytes;
				}
			break;

			case 2:
				while (inCopyHeight--)
				{
					width = inCopyWidth;
					src_read = src;
					dst_write = dst;

					while (width--)
 					{
 						src_pixel.value = *((UUtUns16*)src_read);
 						IMrPixel_Convert(
 							inSrcPixelType,
 							src_pixel,
 							inDstPixelType,
 							&dst_pixel);
 						*((UUtUns16*)dst_write) = (UUtUns16)dst_pixel.value;
 						dst_write += 2;
 						src_read += 2;
 					}
 					src += src_rowbytes;
 					dst += dst_rowbytes;
				}
			break;

			case 4:
				while (inCopyHeight--)
				{
					width = inCopyWidth;
					src_read = src;
					dst_write = dst;

					while (width--)
 					{
 						src_pixel.value = *((UUtUns32*)src_read);
 						IMrPixel_Convert(
 							inSrcPixelType,
 							src_pixel,
 							inDstPixelType,
 							&dst_pixel);
 						*((UUtUns32*)dst_write) = dst_pixel.value;
 						dst_write += 4;
 						src_read += 4;
 					}
 					src += src_rowbytes;
 					dst += dst_rowbytes;
				}
			break;
		}
	}
}

UUtError
IMrImage_GeneralManipulation(
	IMtScaleMode	inScaleMode,
	IMtDitherMode	inDitherMode,
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	UUtRect*		inSrcRect,
	IMtPixelType	inSrcPixelType,
	void*			inSrcData,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	UUtRect*		inDstRect,
	IMtPixelType	inDstPixelType,
	void*			outDstData)
{
	UUtUns16	newSrcWidth;
	UUtUns16	newSrcHeight;
	UUtUns16	newSrcRowBytes;
	void*		newSrcData;
	IMtPoint2D	srcLocation;
	IMtPoint2D	newSrcLocation;

	UUtUns16	dstWidth;
	UUtUns16	dstHeight;
	UUtUns16	unconvertedDstRowBytes;
	void*		unconvertedDstData;
	UUtUns16	convertedDstRowBytes;
	void*		convertedDstData;
	IMtPoint2D	dstLocation;

	newSrcWidth = inSrcRect->right - inSrcRect->left;
	newSrcHeight = inSrcRect->bottom - inSrcRect->top;
	newSrcRowBytes = IMrImage_ComputeRowBytes(inSrcPixelType, newSrcWidth);
	srcLocation.x = inSrcRect->left;
	srcLocation.y = inSrcRect->top;
	newSrcLocation.x = 0;
	newSrcLocation.y = 0;

	newSrcData = UUrMemory_Block_New(newSrcRowBytes * newSrcHeight);
	UUmError_ReturnOnNull(newSrcData);

	dstWidth = inDstRect->right - inDstRect->left;
	dstHeight = inDstRect->bottom - inDstRect->top;
	unconvertedDstRowBytes = IMrImage_ComputeRowBytes(inSrcPixelType, dstWidth);
	convertedDstRowBytes = IMrImage_ComputeRowBytes(inDstPixelType, dstWidth);
	dstLocation.x = inDstRect->left;
	dstLocation.y = inDstRect->top;

	unconvertedDstData = UUrMemory_Block_New(unconvertedDstRowBytes * dstHeight);
	UUmError_ReturnOnNull(unconvertedDstData);

	// Step 1. Copy srcRect out of the source image into its own image
		IMrImage_Copy(
			newSrcWidth,
			newSrcHeight,
			inSrcWidth,
			inSrcHeight,
			inSrcPixelType,
			inSrcData,
			&srcLocation,
			newSrcWidth,
			newSrcHeight,
			inSrcPixelType,
			newSrcData,
			&newSrcLocation);

	// Step 2. Scale new source image into the proper dimensions
		IMrImage_Scale(
			IMcScaleMode_Box,
			newSrcWidth,
			newSrcHeight,
			inSrcPixelType,
			newSrcData,
			dstWidth,
			dstHeight,
			unconvertedDstData);

	// Step 3. Convert to dst pixel type
		if(inSrcPixelType != inDstPixelType)
		{
			convertedDstData = UUrMemory_Block_New(convertedDstRowBytes * dstHeight);

			IMrImage_ConvertPixelType(
				inDitherMode,
				dstWidth,
				dstHeight,
				IMcNoMipMap,
				inSrcPixelType,
				unconvertedDstData,
				inDstPixelType,
				convertedDstData);
		}
		else
		{
			convertedDstData = unconvertedDstData;
		}

	// Step 3. Copy scaled image into the dst rect
		IMrImage_Copy(
			dstWidth,
			dstHeight,
			dstWidth,
			dstHeight,
			inDstPixelType,
			convertedDstData,
			&newSrcLocation,
			inDstWidth,
			inDstHeight,
			inDstPixelType,
			outDstData,
			&dstLocation);

	UUrMemory_Block_Delete(newSrcData);
	UUrMemory_Block_Delete(unconvertedDstData);

	if(convertedDstData != unconvertedDstData)
	{
		UUrMemory_Block_Delete(convertedDstData);
	}

	return UUcError_None;
}

void
IMrImage_GetPixel(
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	IMtPixelType	inSrcPixelType,
	void			*inSrcData,
	UUtUns16		inPixelX,
	UUtUns16		inPixelY,
	IMtPixelType	inDstPixelType,
	IMtPixel		*outDstPixel)
{
	IMtPixel		temp_pixel;

	UUmAssert((inPixelX >= 0) && (inPixelX < inSrcWidth));
	UUmAssert((inPixelY >= 0) && (inPixelY < inSrcHeight));
	UUmAssert(inSrcData);
	UUmAssert(outDstPixel);

	// get the pixel from the the bitmap
	switch (IMrPixel_GetSize(inSrcPixelType))
	{
		case 1:
			temp_pixel.value = *((UUtUns8*)(inSrcData) + inPixelX + inPixelY);
		break;

		case 2:
			temp_pixel.value = *((UUtUns16*)(inSrcData) + inPixelX + (inPixelY * inSrcWidth));
		break;

		case 4:
			temp_pixel.value = *((UUtUns32*)(inSrcData) + inPixelX + (inPixelY * inSrcWidth));
		break;

		default:
			UUmAssert(!"bad pixel type");
		break;
	}

	// convert the pixel to the desired pixel type
	IMrPixel_Convert(
		inSrcPixelType,
		temp_pixel,
		inDstPixelType,
		outDstPixel);
}

void
IMrImage_Fill(
	IMtPixel		inPixel,
	IMtPixelType	inPixelType,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	const UUtRect*	inDstRect,
	void*			inDstData)
{
	UUtUns32			numBytes;

	UUtUns16			i;
	UUtInt16			dest_x;
	UUtInt16			dest_y;
	UUtInt16			width;
	UUtInt16			height;
	UUtRect 			texture_rect;
	UUtUns8				texel_size;

	if (inDstRect == NULL)
	{
		numBytes = IMrImage_ComputeRowBytes(inPixelType, inDstWidth) * inDstHeight;

		switch(IMrPixel_GetSize(inPixelType))
		{
			case 1:
				UUrMemory_Set8(inDstData, (UUtUns8) inPixel.value, numBytes);
			break;

			case 2:
				UUrMemory_Set16(inDstData, (UUtUns16) inPixel.value, numBytes);
			break;

			case 4:
				UUrMemory_Set32(inDstData, (UUtUns32) inPixel.value, numBytes);
			break;

			default:
				UUmAssert(!"bad texel_size");
		}
		return;
	}

	UUmAssertReadPtr(inDstRect, sizeof(*inDstRect));

	// set the texture_rect
	texture_rect.left = 0;
	texture_rect.top = 0;
	texture_rect.right = inDstWidth;
	texture_rect.bottom = inDstHeight;

	// set the clip info
	IMrRect_ComputeClipInfo(&texture_rect, inDstRect, &dest_x, &dest_y, &width, &height);

	if ((width <= 0) || (height <= 0))
		return;

	texel_size = IMrPixel_GetSize(inPixelType);

	// fill in the bounds area of the texture map with inColor
	switch (texel_size)
	{
		case 2:
		{
			UUtUns16			*dest_16_line_start;
			UUtUns32			dest_16_row_texels;

			dest_16_row_texels =	inDstWidth;
			dest_16_line_start =	(UUtUns16*)inDstData +
									dest_x +
									(dest_16_row_texels * dest_y);

			for (i = 0; i < height; i++)
			{
				UUrMemory_Set16(
					dest_16_line_start,
					(UUtUns16) inPixel.value,
					width * texel_size);

				dest_16_line_start += dest_16_row_texels;
			}
		}
		break;

		case 1:
		{
			UUtUns8				*dest_8_line_start;
			UUtUns32			dest_8_row_texels;

			dest_8_row_texels = inDstWidth;
			dest_8_line_start =	(UUtUns8*)inDstData +
								dest_x +
								(dest_8_row_texels * dest_y);

			for (i = 0; i < height; i++)
			{
				UUrMemory_Set8(
					dest_8_line_start,
					(UUtUns8) inPixel.value,
					width * texel_size);

				dest_8_line_start += dest_8_row_texels;
			}
		}
		break;

		case 4:
		{
			UUtUns32			*dest_32_line_start;
			UUtUns32			dest_32_row_texels;

			dest_32_row_texels =	inDstWidth;
			dest_32_line_start =	(UUtUns32*)inDstData +
									dest_x +
									(dest_32_row_texels * dest_y);

			for (i = 0; i < height; i++)
			{
				UUrMemory_Set32(
					dest_32_line_start,
					inPixel.value,
					width * texel_size);

				dest_32_line_start += dest_32_row_texels;
			}
		}
		break;

		default:
			UUmAssert(!"Unknown TexelSize");
	}
}

UUtUns16
IMrImage_ComputeRowBytes(
	IMtPixelType	inPixelType,
	UUtUns16		inWidth)
{
	UUtUns8 sizePerPixel = IMrPixel_GetSize(inPixelType);

	if (sizePerPixel != 0)
	{
		switch(sizePerPixel)
		{
		case 4:
			UUmAssert(inWidth <= (UUcMaxUns16 / 4));
			return inWidth * 4;

		case 2:
			UUmAssert(inWidth <= (UUcMaxUns16 / 2));
			return inWidth * 2;

		case 1:
			return (inWidth + 1) & ~1;
		}
	}
	else
	{
		switch(inPixelType)
		{
			case IMcPixelType_I1:
				return (inWidth + 15) >> 3;

			default:
				UUmAssert(!"unknown pixel type");
		}
	}

	return 0;
}

UUtUns32
IMrImage_NumPixels(
	IMtMipMap		inHasMipMap,
	UUtUns16		inWidth,
	UUtUns16		inHeight)
{
	UUtUns32 count = 0;

	if ((0 == inWidth) || (0 == inHeight))
	{
		return 0;
	}

	count = inWidth * inHeight;

	if (IMcHasMipMap == inHasMipMap)
	{
		// S.S. UUtUns16 newWidth = inWidth / 2;
		//UUtUns16 newHeight = inHeight / 2;
		UUtUns16 newWidth= inWidth >> 1;
		UUtUns16 newHeight= inHeight >> 1;

		if ((newWidth > 0) || (newHeight > 0)) {
			newWidth = UUmMax(newWidth, 1);
			newHeight = UUmMax(newHeight, 1);
		}

		count += IMrImage_NumPixels(inHasMipMap, newWidth, newHeight);
	}

	return count;
}

UUtUns32
IMrImage_NumRowPaddedPixels(
	IMtMipMap		inHasMipMap,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	UUtUns16		inPaddingPowerOf2)
{
	UUtUns32 count = 0;
	UUtUns16 rowWidth;

	if ((0 == inWidth) || (0 == inHeight))
	{
		return 0;
	}

	rowWidth = (inWidth + (1 << inPaddingPowerOf2) - 1) & ~((1 << inPaddingPowerOf2) - 1);
	count = rowWidth * inHeight;

	if (IMcHasMipMap == inHasMipMap)
	{
		// S.S. UUtUns16 newWidth = inWidth / 2;
		//UUtUns16 newHeight = inHeight / 2;
		UUtUns16 newWidth= inWidth >> 1;
		UUtUns16 newHeight= inHeight >> 1;

		if ((newWidth > 0) || (newHeight > 0)) {
			newWidth = UUmMax(newWidth, 1);
			newHeight = UUmMax(newHeight, 1);
		}

		count += IMrImage_NumRowPaddedPixels(inHasMipMap, newWidth, newHeight, inPaddingPowerOf2);
	}

	return count;
}

UUtUns32
IMrImage_NumPixelBlocks(
	IMtMipMap		inHasMipMap,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	UUtUns16		inBlockPowerOf2)
{
	UUtUns32 count = 0;
	UUtUns16 blockWidth;
	UUtUns16 blockHeight;

	if ((0 == inWidth) || (0 == inHeight))
	{
		return 0;
	}

	blockWidth = (inWidth + (1 << inBlockPowerOf2) - 1) >> inBlockPowerOf2;
	blockHeight = (inHeight + (1 << inBlockPowerOf2) - 1) >> inBlockPowerOf2;

	count = blockWidth * blockHeight;

	if (IMcHasMipMap == inHasMipMap)
	{
		UUtUns16 newWidth= inWidth >> 1;
		UUtUns16 newHeight= inHeight >> 1;

		if ((newWidth > 0) || (newHeight > 0)) {
			newWidth = UUmMax(newWidth, 1);
			newHeight = UUmMax(newHeight, 1);
		}

		count += IMrImage_NumPixelBlocks(inHasMipMap, newWidth, newHeight, inBlockPowerOf2);
	}

	return count;
}

UUtUns32
IMrImage_ComputeSize(
	IMtPixelType	inPixelType,
	IMtMipMap		inHasMipMap,
	UUtUns16		inWidth,
	UUtUns16		inHeight)
{
	UUtUns8 sizePerPixel = IMrPixel_GetSize(inPixelType);
	UUtUns32 size = 0;

	if ((0 == inWidth) || (0 == inHeight))
	{
		return 0;
	}

	if (sizePerPixel != 0)
	{
		switch(sizePerPixel)
		{
			case 4:
				UUmAssert(inWidth <= (UUcMaxUns16 / 4));
				size =  inWidth * inHeight * 4;
				break;

			case 2:
				UUmAssert(inWidth <= (UUcMaxUns16 / 2));
				size =  inWidth * inHeight * 2;
				break;

			case 1:
				size =  ((inWidth + 1) & ~1) * inHeight * 1;
				break;
		}
	}
	else
	{
		switch(inPixelType)
		{
			case IMcPixelType_DXT1:
				// S.S. size = (((inWidth + 3) & ~3) * ((inHeight + 3) & ~3) * 2) / 4;
				size= (((inWidth + 3) & ~3) * ((inHeight + 3) & ~3) * 2) >> 2;
				break;


			case IMcPixelType_I1:
				size= IMrImage_ComputeRowBytes(inPixelType, inWidth) * inHeight;
				break;

			default:
				UUmAssert(!"unknown pixel type");
		}
	}

	if (IMcHasMipMap == inHasMipMap)
	{
		// S.S. UUtUns16 newWidth = inWidth / 2;
		//UUtUns16 newHeight = inHeight / 2;
		UUtUns16 newWidth= inWidth >> 1;
		UUtUns16 newHeight= inHeight >> 1;

		if ((newWidth > 0) || (newHeight > 0)) {
			newWidth = UUmMax(newWidth, 1);
			newHeight = UUmMax(newHeight, 1);
		}

		size += IMrImage_ComputeSize(inPixelType, inHasMipMap, newWidth, newHeight);
	}

	return size;
}


IMtPixel
IMrPixel_FromARGB(
	IMtPixelType	inPixelType,
	float			inA,
	float			inR,
	float			inG,
	float			inB)
{
	IMtPixel						result;
	const IMtPixelTypeInfo_Lookup*	pixelTypeInfo = IMiPixel_LookupTypeInfo(inPixelType);

	result.value = IMiComponentToBits(inA, pixelTypeInfo->aMask);
	result.value |= IMiComponentToBits(inR, pixelTypeInfo->rMask);
	result.value |= IMiComponentToBits(inG, pixelTypeInfo->gMask);
	result.value |= IMiComponentToBits(inB, pixelTypeInfo->bMask);

	return result;
}

IMtPixel
IMrPixel_FromShade(
	IMtPixelType	inPixelType,
	IMtShade		inShade)
{
	IMtPixel result;
	static const float one_over_255= 1.f/255.f;
	float a = (float) ((inShade >> 24) & 0xff);
	float r = (float) ((inShade >> 16) & 0xff);
	float g = (float) ((inShade >> 8) & 0xff);
	float b = (float) ((inShade >> 0) & 0xff);

	/* S.S.
	a *= 1.f / ((float) 0xff);
	r *= 1.f / ((float) 0xff);
	g *= 1.f / ((float) 0xff);
	b *= 1.f / ((float) 0xff);
	*/
	a *= one_over_255;
	r *= one_over_255;
	g *= one_over_255;
	b *= one_over_255;

	result = IMrPixel_FromARGB(inPixelType, a, r, g, b);

	return result;
}

IMtShade
IMrShade_FromPixel(
	IMtPixelType		inPixelType,
	IMtPixel		inPixel)
{
	float a,r,g,b;
	IMtShade shade_a, shade_r, shade_g, shade_b;
	IMtShade shade;

	IMrPixel_Decompose(inPixel, inPixelType, &a, &r, &g, &b);

	shade_a = MUrUnsignedSmallFloat_To_Uns_Round(a * 255.f);
	shade_r = MUrUnsignedSmallFloat_To_Uns_Round(r * 255.f);
	shade_g = MUrUnsignedSmallFloat_To_Uns_Round(g * 255.f);
	shade_b = MUrUnsignedSmallFloat_To_Uns_Round(b * 255.f);

	shade = (shade_a << 24) | (shade_r << 16) | (shade_g << 8) | (shade_b << 0);

	return shade;
}


static IMtShade
IMrShade_Interpolate_Integer(
	IMtShade		inShade1,
	IMtShade		inShade2,
	float			inInterp)
{
	IMtShade		result;
	IMtShade		dst_a;
	IMtShade		dst_r;
	IMtShade		dst_g;
	IMtShade		dst_b;
	UUtUns32		to_amount = MUrUnsignedSmallFloat_ShiftLeft_To_Uns_Round(inInterp, 8);
	UUtUns32		from_amount = 256 - to_amount;

	dst_a = ((inShade1 & 0xff000000) >> 24) * from_amount + ((inShade2 & 0xff000000) >> 24) * to_amount;
	dst_r = ((inShade1 & 0x00ff0000) >> 16) * from_amount + ((inShade2 & 0x00ff0000) >> 16) * to_amount;
	dst_g = ((inShade1 & 0x0000ff00) >>  8) * from_amount + ((inShade2 & 0x0000ff00) >>  8) * to_amount;
	dst_b = ((inShade1 & 0x000000ff) >>  0) * from_amount + ((inShade2 & 0x000000ff) >>  0) * to_amount;

	dst_a = (dst_a & 0xff00) << 16;
	dst_r = (dst_r & 0xff00) << 8;
	dst_g = (dst_g & 0xff00) << 0;
	dst_b = (dst_b & 0xff00) >> 8;

	result = dst_a | dst_r | dst_g | dst_b;

	return result;
}

static IMtShade
IMrShade_Interpolate_Float(
	IMtShade		inShade1,
	IMtShade		inShade2,
	float			inInterp)
{
	UUtUns32 returnval;
	UUtUns16 a, r, g, b;
	UUtUns8 fwd, back;

	fwd = (UUtUns8) (255 * inInterp);
	back = 255 - fwd;

	a = (UUtUns16) (back * ((inShade1 & 0xFF000000) >> 24));
	r = (UUtUns16) (back * ((inShade1 & 0x00FF0000) >> 16));
	g = (UUtUns16) (back * ((inShade1 & 0x0000FF00) >>  8));
	b = (UUtUns16) (back * ((inShade1 & 0x000000FF)      ));

	a += (UUtUns16) (fwd * ((inShade2 & 0xFF000000) >> 24));
	r += (UUtUns16) (fwd * ((inShade2 & 0x00FF0000) >> 16));
	g += (UUtUns16) (fwd * ((inShade2 & 0x0000FF00) >>  8));
	b += (UUtUns16) (fwd * ((inShade2 & 0x000000FF)      ));

	returnval  = ((UUtUns32) (a & 0xFF00)) << 16;
	returnval |= ((UUtUns32) (r & 0xFF00)) << 8;
	returnval |=  (UUtUns32) (g & 0xFF00);
	returnval |= ((UUtUns32) (b & 0xFF00)) >> 8;

	return returnval;
}

IMtShade
IMrShade_Interpolate(
	IMtShade		inShade1,
	IMtShade		inShade2,
	float			inInterp)
{
	IMtShade result_integer = IMrShade_Interpolate_Integer(inShade1, inShade2, inInterp);

#if defined(DEBUGGING) && DEBUGGING
	{
		IMtShade result_float = IMrShade_Interpolate_Float(inShade1, inShade2, inInterp);
		UUmAssert(IMrShade_IsEqual(result_integer, result_float));
	}
#endif

	return result_integer;
}

UUtBool IMrShade_IsEqual(IMtShade src, IMtShade dst)
{
	UUtInt32 max_delta = 2;
	UUtInt32 delta = 0;
	UUtBool is_equal = UUcTrue;

	delta = UUmABS(((src >> 24) & 0xff) - ((dst >> 24) & 0xff));
	is_equal = is_equal && (delta <= max_delta);

	delta = UUmABS(((src >> 16) & 0xff) - ((dst >> 16) & 0xff));
	is_equal = is_equal && (delta <= max_delta);

	delta = UUmABS(((src >>  8) & 0xff) - ((dst >>  8) & 0xff));
	is_equal = is_equal && (delta <= max_delta);

	delta = UUmABS(((src >>  0) & 0xff) - ((dst >>  0) & 0xff));
	is_equal = is_equal && (delta <= max_delta);

	return is_equal;
}

IMtPixel
IMrPixel_Get(
	void*			inImageData,
	IMtPixelType	inPixelType,
	UUtUns16		inX,
	UUtUns16		inY,
	UUtUns16		inWidth,
	UUtUns16		inHeight)
{
	IMtPixel pixel;
	UUtUns32 pixelNumber = (inWidth * inY) + inX;

	switch(IMrPixel_GetSize(inPixelType))
	{
		case 1:
			{
				UUtUns8 *ptr_1 = inImageData;

				pixel.value = ptr_1[pixelNumber];
			}
		break;

		case 2:
			{
				UUtUns16 *ptr_2 = inImageData;

				pixel.value = ptr_2[pixelNumber];
			}
		break;

		case 4:
			{
				UUtUns32 *ptr_4 = inImageData;

				pixel.value = ptr_4[pixelNumber];
			}
		break;

		default:
			UUmAssert(!"invalid case in IMrPixel_Get");
	}

	return pixel;
}


void
IMrPixel_Decompose(
	IMtPixel		inPixel,
	IMtPixelType	inPixelType,
	float			*outA,
	float			*outR,
	float			*outG,
	float			*outB)
{
	const IMtPixelTypeInfo_Lookup*	pixelTypeInfo = IMiPixel_LookupTypeInfo(inPixelType);

	UUmAssertWritePtr(outA, sizeof(*outA));
	UUmAssertWritePtr(outR, sizeof(*outR));
	UUmAssertWritePtr(outG, sizeof(*outG));
	UUmAssertWritePtr(outB, sizeof(*outB));

	*outA = IMiBitsToComponent(inPixel.value & pixelTypeInfo->aMask, pixelTypeInfo->aMask);
	*outR = IMiBitsToComponent(inPixel.value & pixelTypeInfo->rMask, pixelTypeInfo->rMask);
	*outG = IMiBitsToComponent(inPixel.value & pixelTypeInfo->gMask, pixelTypeInfo->gMask);
	*outB = IMiBitsToComponent(inPixel.value & pixelTypeInfo->bMask, pixelTypeInfo->bMask);

}

UUtUns8 IMg4BitAlphaBlendTable[0xFF];

static UUcInline UUtUns32 IMr4BitAlphaBlendTable(UUtUns32 inSrcAlpha, UUtUns32 inDstAlpha)
{
	UUtUns32 resultAlpha;

	resultAlpha = ((16*16) - ((16 - inDstAlpha) * (16 - inSrcAlpha))) >> 4;

	return resultAlpha;
}

static UUcInline UUtUns32 IMr4BitRGBBlendTable(UUtUns32 inSrcAlpha, UUtUns32 inDstAlpha, UUtUns32 inSrcColor, UUtUns32 inDstColor)
{
	UUtUns32 newC;

	UUmAssert((inSrcAlpha + inDstAlpha) != 0);

	newC = (inSrcColor * inSrcAlpha + inDstColor * inDstAlpha) / (inSrcAlpha + inDstAlpha);

	return newC;
}

UUtError
IMrInitialize(
	void)
{
	UUtUns32	itr;
	UUtUns16	srcA, dstA, newA;
	UUtUns16	srcC, dstC, newC;
	float		srcA_float, dstA_float, newA_float;
	float		srcC_float, dstC_float, newC_float;
	static const float one_over_255= 1.f/255.f;

	UUrStartupMessage("initializing image system...");

	// initialize alpha blend table
	for(itr = 0; itr < 0xFF; itr++)
	{
		UUtUns32 test_alpha;

		srcA = (UUtUns16)(itr >> 4);
		dstA = (UUtUns16)(itr & 0xF);

		test_alpha = IMr4BitAlphaBlendTable(srcA, dstA);

		srcA |= (UUtUns16)(srcA << 4);
		dstA |= (UUtUns16)(dstA << 4);

		/* S.S. srcA_float = srcA / 255.0f;
		dstA_float = dstA / 255.0f;*/
		srcA_float= srcA * one_over_255;
		dstA_float= dstA * one_over_255;

		newA_float = 1.0f - (1.0f - dstA_float) * (1.0f - srcA_float);
		newA = (UUtUns16)(newA_float * 15.0f);

		UUmAssert(UUmABS(test_alpha - newA) <= 1);
	}

	// verify the rgb blend table
	for(itr = 0; itr < 0xFFFF; itr++)
	{
		UUtUns32 function_result;

		srcA = (UUtUns16)((itr >> 12) & 0xF);
		dstA = (UUtUns16)((itr >> 8) & 0xF);
		srcC = (UUtUns16)((itr >> 4) & 0xF);
		dstC = (UUtUns16)((itr >> 0) & 0xF);

		if (0 == (srcA+dstA)) { continue; }

		function_result = IMr4BitRGBBlendTable(srcA, dstA, srcC, dstC);

		srcA |= srcA << 4;
		dstA |= dstA << 4;
		srcC |= srcC << 4;
		dstC |= dstC << 4;

		/* S.S. srcA_float = srcA / 255.0f;
		dstA_float = dstA / 255.0f;
		srcC_float = srcC / 255.0f;
		dstC_float = dstC / 255.0f;*/
		srcA_float = srcA * one_over_255;
		dstA_float = dstA * one_over_255;
		srcC_float = srcC * one_over_255;
		dstC_float = dstC * one_over_255;

		newC_float = (srcC_float * srcA_float + dstC_float * dstA_float) / (srcA_float + dstA_float);

		newC = (UUtUns16)(newC_float * 15.0f);
		UUmAssert(UUmABS(function_result == newC) <= 1);
	}

#if defined(DEBUGGING) && DEBUGGING
	for(itr = 0; itr < 0xffff; itr++)
	{
		IMtShade shade1 = (UUrLocalRandom() << 16) || UUrLocalRandom();
		IMtShade shade2 = (UUrLocalRandom() << 16) || UUrLocalRandom();
		static const float one_over_UUcMaxUns16= 1.f / UUcMaxUns16;
		// S.S. float t = UUrLocalRandom() * (1.f / UUcMaxUns16);
		float t= UUrLocalRandom() * one_over_UUcMaxUns16;

		IMrShade_Interpolate(shade1, shade2, t);
	}
#endif


	return UUcError_None;
}

void
IMrTerminate(
	void)
{
}

UUtUns16
IMrPixel_BlendARGB4444(
	UUtUns16		inSrcPixel,
	UUtUns16		inDstPixel)
{
	UUtUns32	srcA, dstA, newA;
	UUtUns32	srcR, dstR, newR;
	UUtUns32	srcG, dstG, newG;
	UUtUns32	srcB, dstB, newB;
	UUtUns32	result;

	srcA = (inSrcPixel >> 12) & 0xF;
	dstA = (inDstPixel >> 12) & 0xF;

	if (0 == srcA) {
		return inDstPixel;
	}

	newA = IMr4BitAlphaBlendTable(srcA, dstA);

	srcR = (inSrcPixel >> 8) & 0xF;
	dstR = (inDstPixel >> 8) & 0xF;
	newR = IMr4BitRGBBlendTable(srcA, dstA, srcR, dstR);

	srcG = (inSrcPixel >> 4) & 0xF;
	dstG = (inDstPixel >> 4) & 0xF;
	newG = IMr4BitRGBBlendTable(srcA, dstA, srcG, dstG);

	srcB = (inSrcPixel >> 0) & 0xF;
	dstB = (inDstPixel >> 0) & 0xF;
	newB = IMr4BitRGBBlendTable(srcA, dstA, srcB, dstB);

	result = (newA << 12) | (newR << 8) | (newG << 4) | (newB);

	return (UUtUns16) result;
}


// ----------------------------------------------------------------------
void
IMrRect_Inset(
	UUtRect			*ioRect,
	const UUtInt16	inDeltaX,
	const UUtInt16	inDeltaY)
{
	UUmAssert(ioRect);

	ioRect->left +=		inDeltaX;
	ioRect->top +=		inDeltaY;
	ioRect->right -=	inDeltaX;
	ioRect->bottom -=	inDeltaY;
}

// ----------------------------------------------------------------------
void
IMrRect_Intersect(
	const UUtRect*	inRect1,
	const UUtRect*	inRect2,
	UUtRect			*outRect)
{
	UUtInt16			left;
	UUtInt16			right;
	UUtInt16			top;
	UUtInt16			bottom;

	UUmAssert(inRect1);
	UUmAssert(inRect2);
	UUmAssert(outRect);

	// calculate the left, right, top, and bottom of the out rect
	left = UUmMin(inRect1->left, inRect2->right);
	left = UUmMax(left, inRect2->left);

	right = UUmMax(inRect1->right, inRect2->left);
	right = UUmMin(right, inRect2->right);

	top = UUmMin(inRect1->top, inRect2->bottom);
	top = UUmMax(top, inRect2->top);

	bottom = UUmMax(inRect1->bottom, inRect2->top);
	bottom = UUmMin(bottom, inRect2->bottom);

	// if they don't intersect, set the params to zero
	if (left > right) left = right = 0;
	if (top > bottom) top = bottom = 0;

	// set the out rect
	outRect->left = left;
	outRect->right = right;
	outRect->top = top;
	outRect->bottom = bottom;
}

// ----------------------------------------------------------------------
void
IMrRect_Offset(
	UUtRect			*ioRect,
	const UUtInt16	inOffsetX,
	const UUtInt16	inOffsetY)
{
	UUmAssert(ioRect);

	ioRect->left = ioRect->left + inOffsetX;
	ioRect->right = ioRect->right + inOffsetX;

	ioRect->top  = ioRect->top + inOffsetY;
	ioRect->bottom = ioRect->bottom + inOffsetY;
}

// ----------------------------------------------------------------------
UUtBool
IMrRect_PointIn(
	const UUtRect*		inRect,
	const IMtPoint2D*	inPoint)
{
	if ((inPoint->x >= inRect->left) &&
		(inPoint->x <= inRect->right) &&
		(inPoint->y >= inRect->top) &&
		(inPoint->y <= inRect->bottom))
	{
		return UUcTrue;
	}

	return UUcFalse;
}

// ----------------------------------------------------------------------
void
IMrRect_ComputeClipInfo(
	const UUtRect*	inRect1,
	const UUtRect*	inRect2,
	UUtInt16		*outDestX,
	UUtInt16		*outDestY,
	UUtInt16		*outWidth,
	UUtInt16		*outHeight)
{
	UUtInt16			left;
	UUtInt16			right;
	UUtInt16			top;
	UUtInt16			bottom;

	UUmAssertReadPtr(inRect1, sizeof(*inRect1));
	UUmAssertReadPtr(inRect2, sizeof(*inRect2));
	UUmAssertWritePtr(outDestX, sizeof(*outDestX));
	UUmAssertWritePtr(outDestY, sizeof(*outDestY));
	UUmAssertWritePtr(outWidth, sizeof(*outWidth));
	UUmAssertWritePtr(outHeight, sizeof(*outHeight));

	left = UUmMin(inRect1->left, inRect2->right);
	left = UUmMax(left, inRect2->left);

	right = UUmMax(inRect1->right, inRect2->left);
	right = UUmMin(right, inRect2->right);

	top = UUmMin(inRect1->top, inRect2->bottom);
	top = UUmMax(top, inRect2->top);

	bottom = UUmMax(inRect1->bottom, inRect2->top);
	bottom = UUmMin(bottom, inRect2->bottom);

	*outDestX = left;
	*outDestY = top;
	*outWidth = right - left;
	*outHeight = bottom - top;
}

// ----------------------------------------------------------------------
void
IMrRect_Union(
	const UUtRect		*inRect1,
	const UUtRect		*inRect2,
	UUtRect				*outRect)
{
	UUtRect				temp;

	UUmAssertReadPtr(inRect1, sizeof(*inRect1));
	UUmAssertReadPtr(inRect2, sizeof(*inRect2));
	UUmAssertWritePtr(outRect, sizeof(*outRect));

	temp.left	= UUmMin(inRect1->left, inRect2->left);
	temp.top	= UUmMin(inRect1->top, inRect2->top);
	temp.right	= UUmMax(inRect1->right, inRect2->right);
	temp.bottom	= UUmMax(inRect1->bottom, inRect2->bottom);

	*outRect = temp;
}

// ----------------------------------------------------------------------
UUtInt16
IMrPoint2D_Distance(
	const IMtPoint2D	*inPoint1,
	const IMtPoint2D	*inPoint2)
{
	M3tPoint3D delta;

	delta.x = (float)(inPoint2->x - inPoint1->x);
	delta.y = (float)(inPoint2->y - inPoint1->y);
	delta.z = 0.0f;

	return (UUtInt16)MUrPoint_Length(&delta);
}

#if 0
// ----------------------------------------------------------------------

static double
sinc(
	double x)
{
	x *= M3cPi;
	if(x != 0) return(sin(x) / x);
	return(1.0);
}

static double
Lanczos3_filter(
	double	t)
{
	if(t < 0) t = -t;
	if(t < 3.0) return(sinc(t) * sinc(t/3.0));
	return(0.0);
}

static void
AUiImage_GetRow(
	UUtUns32*		inRowMem,
	const AUtImage*	inImage,
	UUtInt32		inRowIndex)
{
	if((inRowIndex < 0) || (inRowIndex >= inImage->height))
	{
		return;
	}

	UUrMemory_MoveFast(
		(char*)inImage->baseAddr + (inRowIndex * inImage->rowBytes),
		inRowMem,
		(sizeof(UUtUns32) * inImage->width));
}

static void
AUiImage_PutPixel(
	AUtImage*	inImage,
	UUtInt32	inX,
	UUtInt32	inY,
	UUtUns32	inR,
	UUtUns32	inG,
	UUtUns32	inB)
{
	UUtUns32*	p;

	if((inX < 0) || (inX >= inImage->width) || (inY < 0) || (inY >= inImage->height))
	{
		return;
	}

	p = (UUtUns32*)((char*)inImage->baseAddr + (inY * inImage->rowBytes));

	p[inX] = (inR << 16) | (inG << 8) | (inB << 0);
}

static void
AUiImage_GetColumn(
	UUtUns32*	inColumnMem,
	AUtImage*	inImage,
	UUtInt32	inColumnIndex)
{
	UUtInt32	i, d;
	UUtUns32*	p;

	if((inColumnIndex < 0) || (inColumnIndex >= inImage->width))
	{
		return;
	}

	d = inImage->rowBytes >> 2;

	for(i = inImage->height, p = inImage->baseAddr + inColumnIndex;
		i-- > 0;
		p += d)
	{
		*inColumnMem++ = *p;
	}
}

UUtError
AUrImage_Scale(
	AUtImage*	inDst,
	AUtImage*	inSrc)
{
	AUtImage*	tmp;					/* intermediate image */
	double		xscale, yscale;			/* zoom scale factors */
	UUtInt32	i, j, k;				/* loop variables */
	UUtInt32	n;						/* pixel number */
	double		center, left, right;	/* filter calculation variables */
	double		width, fscale, weight;	/* filter calculation variables */
	UUtUns32*	raster;					/* a row or column of pixels */
	double		r, g, b;

	CLIST*		contrib;				/* array of contribution lists */

	float		fWidth = 2.0;
	/* create intermediate image to hold horizontal zoom */
		tmp = AUiImage_New(inDst->width, inSrc->height);
		if(tmp == NULL)
		{
			UUmError_ReturnOnError(UUcError_OutOfMemory);
		}

	xscale = (double) inDst->width / (double) inSrc->width;
	yscale = (double) inDst->height / (double) inSrc->height;

	/* pre-calculate filter contributions for a row */
		contrib = (CLIST *)UUrMemory_Block_NewClear(inDst->width * sizeof(CLIST));

		if(xscale < 1.0)
		{
			width = fWidth / xscale;
			fscale = 1.0 / xscale;

			for(i = 0; i < inDst->width; ++i)
			{
				contrib[i].n = 0;
				contrib[i].p = (CONTRIB *)
					UUrMemory_Block_NewClear((int) (width * 2 + 1) * sizeof(CONTRIB));

				center = (double) i / xscale;
				left = ceil(center - width);
				right = floor(center + width);
				for(j = (UUtUns32) left; j <= right; ++j)
				{
					weight = center - (double) j;
					weight = Lanczos3_filter(weight / fscale) / fscale;
					if(j < 0)
					{
						n = -j;
					}
					else if(j >= inSrc->width)
					{
						n = (inSrc->width - j) + inSrc->width - 1;
					}
					else
					{
						n = j;
					}
					k = contrib[i].n++;
					contrib[i].p[k].pixel = n;
					contrib[i].p[k].weight = weight;
				}
			}
		}
		else
		{
			for(i = 0; i < inDst->width; ++i)
			{
				contrib[i].n = 0;
				contrib[i].p = (CONTRIB *)
					UUrMemory_Block_NewClear((int) (fWidth * 2 + 1) * sizeof(CONTRIB));
				center = (double) i / xscale;
				left = ceil(center - fWidth);
				right = floor(center + fWidth);
				for(j = (UUtUns32) left; j <= right; ++j)
				{
					weight = center - (double) j;
					weight = Lanczos3_filter(weight);

					if(j < 0)
					{
						n = -j;
					}
					else if(j >= inSrc->width)
					{
						n = (inSrc->width - j) + inSrc->width - 1;
					}
					else
					{
						n = j;
					}

					k = contrib[i].n++;
					contrib[i].p[k].pixel = n;
					contrib[i].p[k].weight = weight;
				}
			}
		}

	/* apply filter to zoom horizontally from inSrc to tmp */
		raster = (UUtUns32 *)UUrMemory_Block_NewClear(inSrc->width * sizeof(UUtUns32));
		for(k = 0; k < tmp->height; ++k)
		{
			AUiImage_GetRow(raster, inSrc, k);
			for(i = 0; i < tmp->width; ++i)
			{
				r = g = b = 0.0;
				for(j = 0; j < contrib[i].n; ++j)
				{
					r += ((raster[contrib[i].p[j].pixel] >> 16) & 0xFF) * contrib[i].p[j].weight;
					g += ((raster[contrib[i].p[j].pixel] >> 8) & 0xFF) * contrib[i].p[j].weight;
					b += ((raster[contrib[i].p[j].pixel] >> 0) & 0xFF) * contrib[i].p[j].weight;
				}
				AUiImage_PutPixel(
					tmp,
					i,
					k,
					(UUtUns32)UUmPin(r, 0.0f, 255.999f),
					(UUtUns32)UUmPin(g, 0.0f, 255.999f),
					(UUtUns32)UUmPin(b, 0.0f, 255.999f));
			}
		}

		UUrMemory_Block_Delete(raster);

	/* free the memory allocated for horizontal filter weights */
		for(i = 0; i < tmp->width; ++i)
		{
			UUrMemory_Block_Delete(contrib[i].p);
		}
		UUrMemory_Block_Delete(contrib);

	/* pre-calculate filter contributions for a column */
		contrib = (CLIST *)UUrMemory_Block_NewClear(inDst->height * sizeof(CLIST));
		if(yscale < 1.0)
		{
			width = fWidth / yscale;
			fscale = 1.0 / yscale;
			for(i = 0; i < inDst->height; ++i)
			{
				contrib[i].n = 0;
				contrib[i].p = (CONTRIB *)UUrMemory_Block_NewClear((int) (width * 2 + 1) * sizeof(CONTRIB));
				center = (double) i / yscale;
				left = ceil(center - width);
				right = floor(center + width);
				for(j = (UUtUns32) left; j <= right; ++j)
				{
					weight = center - (double) j;
					weight = Lanczos3_filter(weight / fscale) / fscale;
					if(j < 0)
					{
						n = -j;
					}
					else if(j >= tmp->height)
					{
						n = (tmp->height - j) + tmp->height - 1;
					}
					else
					{
						n = j;
					}
					k = contrib[i].n++;
					contrib[i].p[k].pixel = n;
					contrib[i].p[k].weight = weight;
				}
			}
		}
		else
		{
			for(i = 0; i < inDst->height; ++i)
			{
				contrib[i].n = 0;
				contrib[i].p = (CONTRIB *)UUrMemory_Block_NewClear((int) (fWidth * 2 + 1) * sizeof(CONTRIB));
				center = (double) i / yscale;
				left = ceil(center - fWidth);
				right = floor(center + fWidth);
				for(j = (UUtUns32) left; j <= right; ++j)
				{
					weight = center - (double) j;
					weight = Lanczos3_filter(weight);
					if(j < 0)
					{
						n = -j;
					}
					else if(j >= tmp->height)
					{
						n = (tmp->height - j) + tmp->height - 1;
					}
					else
					{
						n = j;
					}
					k = contrib[i].n++;
					contrib[i].p[k].pixel = n;
					contrib[i].p[k].weight = weight;
				}
			}
		}

	/* apply filter to zoom vertically from tmp to inDst */
		raster = (UUtUns32 *)UUrMemory_Block_NewClear(tmp->height * sizeof(UUtUns32));
		for(k = 0; k < inDst->width; ++k)
		{
			AUiImage_GetColumn(raster, tmp, k);
			for(i = 0; i < inDst->height; ++i)
			{
				r = g = b = 0.0;
				for(j = 0; j < contrib[i].n; ++j)
				{
					r += ((raster[contrib[i].p[j].pixel] >> 16) & 0xFF) * contrib[i].p[j].weight;
					g += ((raster[contrib[i].p[j].pixel] >> 8) & 0xFF) * contrib[i].p[j].weight;
					b += ((raster[contrib[i].p[j].pixel] >> 0) & 0xFF) * contrib[i].p[j].weight;
				}
				AUiImage_PutPixel(
					inDst,
					k,
					i,
					(UUtUns32)UUmPin(r, 0.0f, 255.999f),
					(UUtUns32)UUmPin(g, 0.0f, 255.999f),
					(UUtUns32)UUmPin(b, 0.0f, 255.999f));
			}
		}
		UUrMemory_Block_Delete(raster);

	/* free the memory allocated for vertical filter weights */
		for(i = 0; i < inDst->height; ++i)
		{
			UUrMemory_Block_Delete(contrib[i].p);
		}

		UUrMemory_Block_Delete(contrib);

	AUiImage_Delete(tmp);

	return UUcError_None;
}

#endif

const char *IMrPixelTypeToString(IMtPixelType inPixelType)
{
	typedef struct IMtPixelTypeToStringTable
	{
		IMtPixelType type;
		const char *string;
	} IMtPixelTypeToStringTable;

	IMtPixelTypeToStringTable table[IMcNumPixelTypes] =
	{
		{ IMcPixelType_ARGB4444, "argb4444" },
		{ IMcPixelType_RGB555, "rgb555" },
		{ IMcPixelType_ARGB1555, "" },
		{ IMcPixelType_I8, "i8" },
		{ IMcPixelType_I1, "i1" },
		{ IMcPixelType_A8, "a8" },
		{ IMcPixelType_A4I4, "a4i4" },
		{ IMcPixelType_ARGB8888, "argb8888" },
		{ IMcPixelType_RGB888, "rgb888" },
		{ IMcPixelType_DXT1, "s3-compressed" },
		{ IMcPixelType_RGB_Bytes, "rgb bytes" },
		{ IMcPixelType_RGBA_Bytes, "rgba bytes" },
		{ IMcPixelType_RGBA5551, "rgba5551" },
		{ IMcPixelType_RGBA4444, "rgba4444" },
		{ IMcPixelType_RGB565, "rgb565" },
		{ IMcPixelType_ABGR1555, "abgr1555" },
	};

	const char *pixel_type_string;

	// verify the table
	{
		IMtPixelType itr;

		for(itr = 0; itr < IMcNumPixelTypes; itr++)
		{
			UUmAssert(table[itr].type == itr);
			UUmAssert(table[itr].string != NULL);
		}
	}

	if ((inPixelType < IMcNumPixelTypes) && (table[inPixelType].type == inPixelType)) {
		pixel_type_string = table[inPixelType].string;
	}
	else {
		pixel_type_string = "";
	}


	return pixel_type_string;
}
