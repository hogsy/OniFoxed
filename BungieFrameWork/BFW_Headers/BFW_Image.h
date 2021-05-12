#pragma once

/*
	FILE:	BFW_Image.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Nov 27, 1998
	
	PURPOSE: 
	
	Copyright 1998
	
	GENERAL IMAGE RULES:
		1) the base address for any image starts at the top-left of the image
		2) tbd
		
*/
#ifndef BFW_IMAGE_H
#define BFW_IMAGE_H

#include "BFW.h"

/*
 * Image packing rules:
 	1) There is no concept of a rowbytes for images - all images must abide by these rules
 	2) 4byte per pixel images must be aligned on a 4byte boundary
 	3) 2byte per pixel images must be aligned on a 2byte boundary
 	4) 1byte per pixel images must be aligned on a 2byte boundary - each row must be
 		padded(if needed) to be a multiple of 2bytes.
 	5) IMcPixelType_I1 must be 2byte aligned and each row must be padded(if needed)
 		to be a multiple of 2bytes
 	
 */

	#define IMcShade_None				(0x00000000)
	#define IMcShade_Black				(0xFF000000)
	#define IMcShade_White				(0xFFFFFFFF)
	#define IMcShade_Red				(0xFFFF0000)
	#define IMcShade_Green				(0xFF00FF00)
	#define IMcShade_Blue				(0xFF0000FF)
	#define IMcShade_Yellow				(0xFFFFFF00)
	#define IMcShade_Pink				(0xFFFF00FF)
	#define IMcShade_LightBlue			(0xFF74D0FF)
	#define IMcShade_Orange				(0xFFFF7F00)
	#define IMcShade_Purple				(0xFF700D6F)
	#define IMcShade_Gray75				(0xFFBFBFBF)
	#define IMcShade_Gray50				(0xFF808080)
	#define IMcShade_Gray40				(0xFF666666)
	#define IMcShade_Gray35				(0xFF5A5A5A)
	#define IMcShade_Gray25				(0xFF3F3F3F)
	typedef UUtUns32 IMtShade;

	typedef tm_enum IMtPixelType
	{
		IMcPixelType_ARGB4444,		// 2bytes per pixel
		IMcPixelType_RGB555,		// 2bytes per pixel
		IMcPixelType_ARGB1555,		// 2bytes per pixel
		IMcPixelType_I8,			// 1byte per pixel
		IMcPixelType_I1,			// 8pixels per byte
		IMcPixelType_A8,			// 1byte per pixel
		IMcPixelType_A4I4,			// 1byte per pixel
		IMcPixelType_ARGB8888,		// 4bytes per pixel
		IMcPixelType_RGB888,		// 4bytes per pixel

//		DX6 texture compression formats
		IMcPixelType_DXT1,			// S3TC opaque / 1 bit alpha
//		IMcPixelType_DXT2,			// explicit alpha, pre multiplied
//		IMcPixelType_DXT3,			// explicit alpha, not pre multiplied
//		IMcPixelType_DXT4,			// interpolated alpha, pre multiplied
//		IMcPixelType_DXT5,			// interpolated alpha, not pre multiplied
		IMcPixelType_RGB_Bytes,		// byte packed rgb  (gl)
		IMcPixelType_RGBA_Bytes,	// byte packed rgba (gl)
		IMcPixelType_RGBA5551,		// (gl)
		IMcPixelType_RGBA4444,		// (gl)
		IMcPixelType_RGB565,		// 

		IMcPixelType_ABGR1555,		// 1555

		IMcNumPixelTypes
		
	} IMtPixelType;
	
	typedef enum IMtDitherMode
	{
		IMcDitherMode_Off,
		IMcDitherMode_On
		
	} IMtDitherMode;
	
	typedef enum IMtScaleMode
	{
		IMcScaleMode_None				= 0,
		IMcScaleMode_Box				= 0,
		IMcScaleMode_High				= (1 << 0),
		IMcScaleMode_IndependantAlpha	= (1 << 1)
		
	} IMtScaleMode;
	

/*
 *  rect and point types for images
 */
	
	// This is a struct so that pixel color parameters don't get misused
	typedef struct IMtPixel
	{
		UUtUns32 value;
	} IMtPixel;

	typedef tm_struct IMtPoint2D
	{
		UUtInt16		x;
		UUtInt16		y;
		
	} IMtPoint2D;

	typedef enum
	{
		IMcHasMipMap,
		IMcNoMipMap
	} IMtMipMap;

/*
 * 
 */
	UUtError
	IMrInitialize(
		void);

	void
	IMrTerminate(
		void);

	const char *IMrPixelTypeToString(IMtPixelType inPixelType);

/*
 * 
 */
	void
	IMrImage_Draw_Line(
		UUtUns16			inImageWidth,
		UUtUns16			inImageHeight,
		IMtPixelType		inImagePixelType,
		void				*inImageData,
		IMtPixel			inDrawColor,
		IMtPoint2D			*inPoint1,
		IMtPoint2D			*inPoint2);
		
	void
	IMrImage_Draw_Rect(
		UUtUns16			inImageWidth,
		UUtUns16			inImageHeight,
		IMtPixelType		inImagePixelType,
		void				*inImageData,
		IMtPixel			inDrawColor,
		UUtRect				*inDrawRect);

/*
 * 
 */
	UUtError
	IMrImage_Scale(
		IMtScaleMode	inScaleMode,
		UUtUns16		inSrcWidth,
		UUtUns16		inSrcHeight,
		IMtPixelType	inSrcPixelType,
		void*			inSrcData,
		UUtUns16		inDstWidth,
		UUtUns16		inDstHeight,
		void*			outDstData); // caller is responsible for allocating this memory
	
	UUtError
	IMrImage_ConvertPixelType(
		IMtDitherMode	inDitherMode,
		UUtUns16		inWidth,
		UUtUns16		inHeight,
		IMtMipMap		inMipMap,
		IMtPixelType	inSrcPixelType,
		void*			inSrcData,
		IMtPixelType	inDstPixelType,
		void*			outDstData);	 // caller is responsible for allocating this memory
	
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
		IMtPoint2D		*inDstLocation);

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
		void*			outDstData);	 // caller is responsible for allocating this memory
	
	void
	IMrImage_GetPixel(
		UUtUns16		inSrcWidth,
		UUtUns16		inSrcHeight,
		IMtPixelType	inSrcPixelType,
		void			*inSrcData,
		UUtUns16		inPixelX,
		UUtUns16		inPixelY,
		IMtPixelType	inDstPixelType,
		IMtPixel		*outDstPixel);

	void
	IMrImage_Fill(
		IMtPixel		inPixel,
		IMtPixelType	inPixelType,
		UUtUns16		inDstWidth,
		UUtUns16		inDstHeight,
		const UUtRect*	inDstRect,
		void*			outDstData);
		
	UUtUns16
	IMrImage_ComputeRowBytes(
		IMtPixelType	inPixelType,
		UUtUns16		inWidth);

	UUtUns32
	IMrImage_ComputeSize(
		IMtPixelType	inPixelType,
		IMtMipMap		inHasMipMap,
		UUtUns16		inWidth,
		UUtUns16		inHeight);

	UUtUns32
	IMrImage_NumPixels(
		IMtMipMap		inHasMipMap,
		UUtUns16		inWidth,
		UUtUns16		inHeight);

	UUtUns32
	IMrImage_NumRowPaddedPixels(
		IMtMipMap		inHasMipMap,
		UUtUns16		inWidth,
		UUtUns16		inHeight,
		UUtUns16		inPaddingPowerOf2);

	UUtUns32
	IMrImage_NumPixelBlocks(
		IMtMipMap		inHasMipMap,
		UUtUns16		inWidth,
		UUtUns16		inHeight,
		UUtUns16		inBlockPowerOf2);
	
/*
 * 
 */

	IMtPixel
	IMrPixel_Get(
		void *			inImageData,
		IMtPixelType	inPixelType,
		UUtUns16		inX,
		UUtUns16		inY,
		UUtUns16		inWidth,
		UUtUns16		inHeight);

	IMtPixel
	IMrPixel_FromARGB(
		IMtPixelType	inPixelType,
		float			inA,									 
		float			inR,
		float			inG,
		float			inB);

	IMtPixel
	IMrPixel_FromShade(
		IMtPixelType	inPixelType,
		IMtShade		inShade);

	IMtShade
	IMrShade_FromPixel(
		IMtPixelType	inPixelType,
		IMtPixel		inPixel);

	IMtShade
	IMrShade_Interpolate(
		IMtShade		inShade1,
		IMtShade		inShade2,
		float			inInterp);

	UUtBool
	IMrShade_IsEqual(		// shade's are tested within ~1%
		IMtShade		inShade1,
		IMtShade		inShade2);

/*
 * 
 */
	void
	IMrPixel_Convert(
		IMtPixelType	inSrcPixelType,
		IMtPixel		inSrcPixel,
		IMtPixelType	inDstPixelType,
		IMtPixel		*outDstPixel);
		
	UUtUns8
	IMrPixel_GetSize(
		IMtPixelType	inPixelType);

	void
	IMrPixel_Decompose(
		IMtPixel		inPixel, 
		IMtPixelType	inPixelType,
		float			*outA,
		float			*outR,
		float			*outG,
		float			*outB);

	UUtBool
	IMrPixelType_HasAlpha(
		IMtPixelType	inPixelType);
	
	UUtUns16
	IMrPixel_BlendARGB4444(
		UUtUns16		inSrcPixel,
		UUtUns16		inDstPixel);
	
/*
 *
 */
	void
	IMrRect_Intersect(
		const UUtRect*	inRect1,
		const UUtRect*	inRect2,
		UUtRect			*outRect);
	
	void
	IMrRect_Inset(
		UUtRect			*ioRect,
		const UUtInt16	inDeltaX,
		const UUtInt16	inDeltaY);

	void
	IMrRect_Offset(
		UUtRect			*ioRect,
		const UUtInt16	inOffsetX,
		const UUtInt16	inOffsetY);

	UUtBool
	IMrRect_PointIn(
		const UUtRect*		inRect,
		const IMtPoint2D*	inPoint);
		
	void
	IMrRect_ComputeClipInfo(
		const UUtRect*	inRect1,
		const UUtRect*	inRect2,
		UUtInt16		*outDestX,
		UUtInt16		*outDestY,
		UUtInt16		*outWidth,
		UUtInt16		*outHeight);

	void
	IMrRect_Union(
		const UUtRect		*inRect1,
		const UUtRect		*inRect2,
		UUtRect				*outRect);

/*
 *
 */
	UUtInt16
	IMrPoint2D_Distance(
		const IMtPoint2D	*inPoint1,
		const IMtPoint2D	*inPoint2);

		
#endif /* BFW_IMAGE_H */
