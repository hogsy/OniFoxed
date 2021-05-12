// ======================================================================
// BFW_Image_Draw.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Image.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
void
IMrImage_Draw_Line(
	UUtUns16			inImageWidth,
	UUtUns16			inImageHeight,
	IMtPixelType		inImagePixelType,
	void				*inImageData,
	IMtPixel			inDrawColor,
	IMtPoint2D			*inPoint1,
	IMtPoint2D			*inPoint2)
{
	UUtUns8				*dst_1;
	UUtUns16			*dst_2;
	UUtUns32			*dst_4;
	UUtUns32			offset;
	UUtInt16			x1;
	UUtInt16			y1;
	UUtInt16			x2;
	UUtInt16			y2;
	UUtInt16			width;
	UUtInt16			height;
	UUtUns16			row_bytes;
	
	UUmAssert(inImageData);
	UUmAssert(inPoint1);
	UUmAssert(inPoint2);
	
	// get the row_bytes
	row_bytes = IMrImage_ComputeRowBytes(inImagePixelType, inImageWidth);
	
	// optimize for vertical drawing
	if (inPoint1->x == inPoint2->x)
	{
		// calculate and clip the Ys
		x1 = x2 = UUmMax(0, inPoint1->x);
		x1 = x2 = UUmMin(x1, inImageWidth - 1);
		
		// calculate the Ys
		if (inPoint1->y <= inPoint2->y)
		{
			// inPoint1 is closer to the origin
			y1 = inPoint1->y;
			y2 = inPoint2->y;
		}
		else if (inPoint1->y > inPoint2->y)
		{
			// inPoint2 is closer to the origin
			y1 = inPoint2->y;
			y2 = inPoint1->y;
		}
		
		// clip the Ys
		y1 = UUmMax(0, y1);
		y2 = UUmMin(y2, inImageHeight - 1);

		// calculate the height of the line and the offset from
		// the origin of the image
		height = y2 - y1;
		offset = x1 + (y1 * row_bytes);
		
		switch (inImagePixelType)
		{
			// 1 byte
			case IMcPixelType_I8:
			case IMcPixelType_A8:
			case IMcPixelType_A4I4:
				dst_1 = (UUtUns8*)inImageData + offset;
				
				while (height--)
				{
					*dst_1 = (UUtUns8) inDrawColor.value;
					dst_1 += row_bytes;
				}
			break;
			
			// 2 byte
			case IMcPixelType_ARGB4444:
			case IMcPixelType_RGB555:
			case IMcPixelType_ARGB1555:
				dst_2 = (UUtUns16*)inImageData + offset;
				
				while (height--)
				{
					*dst_2 = (UUtUns16) inDrawColor.value;
					dst_2 += row_bytes;
				}
			break;
			
			// 4 byte
			case IMcPixelType_ARGB8888:
			case IMcPixelType_RGB888:
				dst_4 = (UUtUns32*)inImageData + offset;
				
				while (height--)
				{
					*dst_4 = inDrawColor.value;
					dst_4 += row_bytes;
				}
			break;
		}
	}
	// optimize for horizontal drawing
	else if (inPoint1->y == inPoint2->y)
	{
		// calculate and clip the Ys
		y1 = y2 = UUmMax(0, inPoint1->y);
		y1 = y2 = UUmMin(y1, inImageHeight - 1);
		
		// calculate the Xs
		if (inPoint1->x <= inPoint2->x)
		{
			// inPoint1 is closer to the origin
			x1 = inPoint1->x;
			x2 = inPoint2->x;
		}
		else if (inPoint1->x > inPoint2->x)
		{
			// inPoint2 is closer to the origin
			x1 = inPoint2->x;
			x2 = inPoint1->x;
		}
		
		// clip the Xs
		x1 = UUmMax(0, x1);
		x2 = UUmMin(x2, inImageWidth - 1);

		// calculate the widht of the line and the offset from
		// the origin of the image
		width = x2 - x1;
		offset = x1 + (y1 * row_bytes);
		
		switch (inImagePixelType)
		{
			// 1 byte
			case IMcPixelType_I8:
			case IMcPixelType_A8:
			case IMcPixelType_A4I4:
				dst_1 = (UUtUns8*)inImageData + offset;
				
				while (width--)
				{
					*dst_1++ = (UUtUns8) inDrawColor.value;
				}
			break;
			
			// 2 byte
			case IMcPixelType_ARGB4444:
			case IMcPixelType_RGB555:
			case IMcPixelType_ARGB1555:
				dst_2 = (UUtUns16*)inImageData + offset;
				
				while (width--)
				{
					*dst_2++ = (UUtUns16) inDrawColor.value;
				}
			break;
			
			// 4 byte
			case IMcPixelType_ARGB8888:
			case IMcPixelType_RGB888:
				dst_4 = (UUtUns32*)inImageData + offset;
				
				while (width--)
				{
					*dst_4++ = inDrawColor.value;
				}
			break;
		}
	}
	// general case
	else
	{
		UUmAssert(0);
	}
}

// ----------------------------------------------------------------------
void
IMrImage_Draw_Rect(
	UUtUns16			inImageWidth,
	UUtUns16			inImageHeight,
	IMtPixelType		inImagePixelType,
	void				*inImageData,
	IMtPixel			inDrawColor,
	UUtRect				*inDrawRect)
{
	IMtPoint2D			point1;
	IMtPoint2D			point2;
	IMtPoint2D			point3;
	IMtPoint2D			point4;
	
	UUmAssert(inImageData);
	UUmAssert(inDrawRect);
	
	point1.x = inDrawRect->left;
	point1.y = inDrawRect->top;
	point2.x = inDrawRect->right;
	point2.y = inDrawRect->top;
	point3.x = inDrawRect->right;
	point3.y = inDrawRect->bottom;
	point4.x = inDrawRect->left;
	point4.y = inDrawRect->bottom;
	
	// draw the top of the rect
	IMrImage_Draw_Line(
		inImageWidth,
		inImageHeight,
		inImagePixelType,
		inImageData,
		inDrawColor,
		&point1,
		&point2);
	
	// draw the right of the rect
	IMrImage_Draw_Line(
		inImageWidth,
		inImageHeight,
		inImagePixelType,
		inImageData,
		inDrawColor,
		&point2,
		&point3);

	// draw the bottom of the rect
	IMrImage_Draw_Line(
		inImageWidth,
		inImageHeight,
		inImagePixelType,
		inImageData,
		inDrawColor,
		&point3,
		&point4);
	
	// draw the left of the rect
	IMrImage_Draw_Line(
		inImageWidth,
		inImageHeight,
		inImagePixelType,
		inImageData,
		inDrawColor,
		&point4,
		&point1);
}