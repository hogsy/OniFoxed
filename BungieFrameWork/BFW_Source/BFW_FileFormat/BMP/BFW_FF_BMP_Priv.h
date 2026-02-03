/*
	FILE:	BFW_FF_BMP_Priv.h

	AUTHOR:	Brent H. Pease

	CREATED: Sept 19, 1998

	PURPOSE:

	Copyright 1998

*/

#ifndef BFW_FF_PRIV_BMP_H
#define BFW_FF_PRIV_BMP_H

#include "BFW.h"

/*
 * Constants used in the "type" field.
 */
	#define FFcBMP_Type_ICO			UUm2CharToUns16('I', 'C')
	#define FFcBMP_Type_BMP			UUm2CharToUns16('B', 'M')
	#define FFcBMP_Type_PTR			UUm2CharToUns16('P', 'T')
	#define FFcBMP_Type_ICO_COLOR	UUm2CharToUns16('C', 'I')
	#define FFcBMP_Type_PTR_COLOR	UUm2CharToUns16('C', 'P')
	#define FFcBMP_Type_ARRAY		UUm2CharToUns16('B', 'A')

/*
 * Compression schemes
 */
	#define FFcBMP_Compression_None			(0)
	#define FFcBMP_Compression_RLE_8		(1)
	#define FFcBMP_Compression_RLE_4		(2)
	#define FFcBMP_Compression_HUFFMAN1D	(3)
	#define FFcBMP_Compression_BITFIELDS	(3)
	#define FFcBMP_Compression_RLE_24		(4)
	#define FFcBMP_Compression_Last			(4)

/*
 * units of resolution
 */
	#define FFcBMP_Units_PELS_PER_METER		(0)
	#define FFcBMP_Units_LAST				(0)

/*
 * origin of coordinate space
 */
	#define FFcBMP_Origin_LOWER_LEFT		(0)
	#define FFcBMP_Origin_LAST				(0)

/*
 * halftoning algorithms
 */
	#define FFcBMP_Halftoning_NONE				(0)
	#define FFcBMP_Halftoning_ERROR_DIFFUSION	(1)
	#define FFcBMP_Halftoning_PANDA				(2)
	#define FFcBMP_Halftoning_SUPER_CIRCLE		(3)
	#define FFcBMP_Halftoning_LAST				(3)

/*
 * color table encoding
 */
	#define FFcBMP_Color_Encoding_RGB	(0)
	#define FFcBMP_Color_Encoding_LAST	(0)

/*
 * BMPtHeader defines the properties of a bitmap.
 *
 * A color table is concatenated to this structure.  The number of elements in
 * the color table determined by the bit-depth of the image.
 *
 * Note, that if the field "size" is 12 or less, then the width and height
 * fields should be read as UINT16's instead of UINT32's.
 *
 * Also note that if the field "size" is greater than 12, then the color table
 * will have an extra byte of padding between each structures (to longword
 * align it)
 *
 * The different sizes for the width, height, and color table are the only
 * differences between the "old" and "new" bitmap file formats.
 */
	typedef struct BMPtHeaderOld_Raw
	{
		UUtUns8		size[4];
		UUtUns8		width[2];
		UUtUns8		height[2];
		UUtUns8		numBitPlanes[2];
		UUtUns8		numBitsPerPlane[2];

	} BMPtHeaderOld_Raw;

	typedef struct BMPtHeaderNew_Raw
	{
		UUtUns8		size[4];
		UUtUns8		width[4];
		UUtUns8		height[4];
		UUtUns8		numBitPlanes[2];
		UUtUns8		numBitsPerPlane[2];
		UUtUns8		compressionScheme[4];
		UUtUns8		sizeOfImageData[4];
		UUtUns8		xResolution[4];
		UUtUns8		yResolution[4];
		UUtUns8		numColorsUsed[4];
		UUtUns8		numImportantColors[4];
		UUtUns8		resolutionUnits[2];
		UUtUns8		padding[2];
		UUtUns8		origin[2];
		UUtUns8		halftoning[2];
		UUtUns8		halftoningParam1[4];
		UUtUns8		halftoningParam2[4];
		UUtUns8		colorEncoding[4];
		UUtUns8		identifier[4];

	} BMPtHeaderNew_Raw;

	typedef struct BMPtHeader
	{
		UUtUns32		size;
		UUtUns32		width;
		UUtUns32		height;
		UUtUns16		numBitPlanes;
		UUtUns16		numBitsPerPlane;
		UUtUns32		compressionScheme;
		UUtUns32		sizeOfImageData;
		UUtUns32		xResolution;
		UUtUns32		yResolution;
		UUtUns32		numColorsUsed;
		UUtUns32		numImportantColors;
		UUtUns16		resolutionUnits;
		UUtUns16		padding;
		UUtUns16		origin;
		UUtUns16		halftoning;
		UUtUns32		halftoningParam1;
		UUtUns32		halftoningParam2;
		UUtUns32		colorEncoding;
		UUtUns32		identifier;

	} BMPtHeader;

/*
 * BMPtFileHeader defines a single bitmap image.
 */
	typedef struct BMPtFileHeader_Raw
	{
		UUtUns8		type[2];
		UUtUns8		size[4];
		UUtUns8		xHotSpot[2];
		UUtUns8		yHotSpot[2];
		UUtUns8		offsetToBits[4];

	} BMPtFileHeader_Raw;

/*
 * BMPtArrayHeader is used to establish a linked list of BMPtFileHeader
 * structures for a bitmap file with multiple images in it.
 */
	typedef struct BMPtArrayHeader_Raw
	{
		UUtUns8			type[2];
		UUtUns8			size[4];
		UUtUns8			next[2];
		UUtUns8			screenWidth[2];
		UUtUns8			screenHeight[2];

	} BMPtArrayHeader_Raw;

/*
 * BMPtRGB defines a single color palette entry.
 */
	typedef struct BMPtRGB
	{
		UUtUns8	blue;
		UUtUns8	green;
		UUtUns8	red;

	} BMPtRGB;

#endif /* BFW_FF_PRIV_BMP_H */
