/*
	FILE:	BFW_TextSystem.c
	
	AUTHOR:	Kevin Armstrong
	
	CREATED: ????

	PURPOSE: text
	
	Copyright 1997-1998

*/

// ======================================================================
// Note: The textureMap for the font should be in Intensity 8 format
// ======================================================================

#define SINGLE_TEXTURE_CACHE		1

// ======================================================================
// includes
// ======================================================================
#include <string.h>
#include <ctype.h>
#include "BFW_TS_Private.h"
#include "BFW_MathLib.h"
#include "BFW_LocalInput.h"
#include "BFW_WindowManager.h"
#include "BFW_Timer.h"

#include "WM_DrawContext.h"


// ======================================================================
// defines
// ======================================================================
#define TScFontSize_MaxSearch		18

#define TScTextLineDrawClumpSize	32

#define TScTextureCache_MaxCells	800
#define TScTextureCache_MaxRows		32

#define TScTextureCache_Width		M3cTextureMap_MaxWidth
#define TScTextureCache_Height		M3cTextureMap_MaxHeight

#define TScCellMask_Valid			0x80000000

// bits and shifts
#define cAlphaBits8888		0xFF000000
#define cRedBits8888		0x00FF0000
#define cGreenBits8888		0x0000FF00
#define cBlueBits8888		0x000000FF

#define cAlphaShift8888		24
#define cRedShift8888		16
#define cGreenShift8888		8
#define cBlueShift8888		0

#define cAlphaBits4444		0x0000F000
#define cRedBits4444		0x00000F00
#define cGreenBits4444		0x000000F0
#define cBlueBits4444		0x0000000F

#define cAlphaShift4444		12
#define cRedShift4444		8
#define cGreenShift4444		4
#define cBlueShift4444		0

#define cAlphaBit1555		0x00008000
#define cRedBits555			0x00007C00
#define cGreenBits555		0x000003E0
#define cBlueBits555		0x0000001F

#define cRedShift555		10
#define cGreenShift555		5
#define cBlueShift555		0

// low level mixers
#define Alpha8888(s,i)		((((s & cAlphaBits8888) >> cAlphaShift8888) * i) >> 8)
#define Red8888(s,i)		((((s & cRedBits8888) >> cRedShift8888) * i) >> 8)
#define Green8888(s,i)		((((s & cGreenBits8888) >> cGreenShift8888) * i) >> 8)
#define Blue8888(s,i)		((((s & cBlueBits8888) >> cBlueShift8888) * i) >> 8)

#define Alpha4444(s,i)		((((s & cAlphaBits4444) >> cAlphaShift4444) * i) >> 8)
#define Red4444(s,i)		((((s & cRedBits4444) >> cRedShift4444) * i) >> 8)
#define Green4444(s,i)		((((s & cGreenBits4444) >> cGreenShift4444) * i) >> 8)
#define Blue4444(s,i)		((((s & cBlueBits4444) >> cBlueShift4444) * i) >> 8)

#define Red555(s,i)			((((s & cRedBits555) >> cRedShift555) * i) >> 8)
#define Green555(s,i)		((((s & cGreenBits555) >> cGreenShift555) * i) >> 8)
#define Blue555(s,i)		((((s & cBlueBits555) >> cBlueShift555) * i) >> 8)

// mid level mixers
#define MixAlpha8888(s,d,i)	(((Alpha8888(s, i) + Alpha8888(d,(255-i))) & 0x00FF) << cAlphaShift8888)
#define MixRed8888(s,d,i)	(((Red8888(s, i) + Red8888(d,(255-i))) & 0x00FF) << cRedShift8888)
#define MixGreen8888(s,d,i)	(((Green8888(s, i) + Green8888(d,(255-i))) & 0x00FF) << cGreenShift8888)
#define MixBlue8888(s,d,i)	(((Blue8888(s, i) + Blue8888(d,(255-i))) & 0x00FF) << cBlueShift8888)

#define MixAlpha4444(s,d,i)	(((Alpha4444(s, i) + Alpha4444(d,(255-i))) & 0x00FF) << cAlphaShift4444)
#define MixRed4444(s,d,i)	(((Red4444(s, i) + Red4444(d,(255-i))) & 0x00FF) << cRedShift4444)
#define MixGreen4444(s,d,i)	(((Green4444(s, i) + Green4444(d,(255-i))) & 0x00FF) << cGreenShift4444)
#define MixBlue4444(s,d,i)	(((Blue4444(s, i) + Blue4444(d,(255-i))) & 0x00FF) << cBlueShift4444)

#define MixRed555(s,d,i)	(((Red555(s, i) + Red555(d,(255-i))) & 0x00FF) << cRedShift555)
#define MixGreen555(s,d,i)	(((Green555(s, i) + Green555(d,(255-i))) & 0x00FF) << cGreenShift555)
#define MixBlue555(s,d,i)	(((Blue555(s, i) + Blue555(d,(255-i))) & 0x00FF) << cBlueShift555)

#define MixAlpha1555(s)		(s & cAlphaBit1555)

// hi level mixers
#define Mix8888(s,d,i)		(MixAlpha8888(s,d,i) | MixRed8888(s,d,i) | MixGreen8888(s,d,i) | \
							MixBlue8888(s,d,i))
							
#define Mix4444(s,d,i)		(MixAlpha4444(s,d,i) | MixRed4444(s,d,i) | MixGreen4444(s,d,i) | \
							MixBlue4444(s,d,i))

#define Mix555(s,d,i)		(MixRed555(s,d,i) | MixGreen555(s,d,i) | MixBlue555(s,d,i))

#define Mix1555(s,d,i)		(MixAlpha1555(s) | MixRed555(s,d,i) | MixGreen555(s,d,i) | \
							MixBlue555(s,d,i))
#define MixA4I4(s, d, i)	((((((s & 0xf0) >> 4) * i) + (((d & 0xf0) >> 4) * (255 - i))) >> 8) << 4) | \
							((((s & 0x0f) * i) + ((d & 0x0f) * (255 - i))) >> 8)
#define Mix8(s, d, i)		((UUtUns8) ((((UUtUns16)(s) * (UUtUns16)(i)) + ((UUtUns16)(d) * (255 - (UUtUns16)(i)))) >> 8))

// ======================================================================
// typedefs
// ======================================================================
#if SINGLE_TEXTURE_CACHE
typedef struct TStTextureCache_Cell
{
	TStFont					*font;
	TStGlyph				*glyph;
	UUtUns32				width;
	UUtUns32				height;
	IMtPoint2D				position;
	M3tTextureCoord			uv[2];		// NW, SE

} TStTextureCache_Cell;

typedef struct TStTextureCache_Row
{
	UUtUns32				cell_start;
	UUtUns32				cell_count;
	UUtUns32				x_width;
	UUtUns32				y_start;
	UUtUns32				y_height;

} TStTextureCache_Row;

typedef struct TStTextureCache
{
	// CB: note well that the cell and row arrays are circular queues, and so
	// e.g. cell_start + cell_count may overlap the MaxCells boundary
	UUtUns32				cell_start;
	UUtUns32				cell_count;
	UUtUns32				row_start;
	UUtUns32				row_count;
	TStTextureCache_Cell	cells[TScTextureCache_MaxCells];
	TStTextureCache_Row		rows[TScTextureCache_MaxRows];

	// free vertical space in the texture
	UUtUns32				free_start;
	UUtUns32				free_height;

	M3tTextureMap *			texture;
	IMtPixel				draw_color;
	float					u_ratio;
	float					v_ratio;

} TStTextureCache;

#else	// SINGLE_TEXTURE_CACHE

typedef struct TStCell
{
	TStGlyph				*glyph;
	UUtUns32				time;
	M3tTextureCoord			uv[2];		// (left, top) and (right, bottom)
	
} TStCell;

typedef struct TStFont_PrivateData
{
	UUtUns32				cell_width;
	UUtUns32				cell_height;
	UUtUns32				num_cells;
	UUtUns32				num_cells_x;
	UUtUns32				num_cells_y;
	float					u_ratio;
	float					v_ratio;
	
	M3tTextureMap			*texture;

	TStCell					*cells;
	
} TStFont_PrivateData;

#endif	// SINGLE_TEXTURE_CACHE

// ======================================================================
// globals
// ======================================================================
TMtPrivateData		*TSgTemplate_Font_PrivateData;
TStTextureCache		TSgTextureCache;

TStFormatCharacterColor TSgColorFormattingCharacters[] =
	{{'r', 0xFFEB5050},			// deep red:      255  80  80			// CB: was 158, 2, 2 which is too hard to read
	{'y', 0xFFFCFF1C},			// pastel yellow: 252 255  28
	{'b', 0xFF93BBE9},			// pastel blue:   147 187 233
	{'u', 0xFFF67603},			// umber:         246 118   3
	{'g', 0xFFA2DAA5},			// pastel green:  162 218 165
	{'l', 0xFFBE90D9},			// lavender:      109 144 217
	{'o', 0xFFFBA500},			// light orange:  251 165   0
	{'c', 0xFF93EAEB},			// cyan:          147 234 235
	{'\0', 0}};


// ======================================================================
// prototypes
// ======================================================================
static TStGlyph*
TSiFont_GetGlyph(
	const TStFont		*inFont,
	const UUtUns16		inCharacter);
	
static void
TSiGlyph_Draw(
	TStGlyph			*inGlyph,
	TStFont				*inFont,
	M3tTextureMap		*inTexture,
	IMtPoint2D			*inTextureOrigin,
	UUtRect				*inBounds,
	IMtPoint2D			*inDestination,
	IMtPixel			inColor);

#if !SINGLE_TEXTURE_CACHE
static M3tTextureMap*
TSiFont_GetTexture(
	const TStFont			*inFont);
#endif

static void
TSiTextureCache_RemoveRow(
	void);
	
// ======================================================================
// functions
// ======================================================================

#if SINGLE_TEXTURE_CACHE
// ======================================================================
#if 0 
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
TSrTextureCache_Initialize(
	void)
{
	UUtError error;

	UUrMemory_Clear(&TSgTextureCache, sizeof(TSgTextureCache));

	// create a blank texture
	error = M3rTextureMap_New(TScTextureCache_Width, TScTextureCache_Height, IMcPixelType_A8,
								M3cTexture_AllocMemory, M3cTextureFlags_None,
								"Font Texture Cache", &TSgTextureCache.texture);
	UUmError_ReturnOnErrorMsg(error, "Unable to create font texture cache");

	M3rTextureMap_Fill_Shade(TSgTextureCache.texture, IMcShade_None, NULL);	

	// set up the texture cache
	TSgTextureCache.draw_color = IMrPixel_FromShade(TSgTextureCache.texture->texelType, IMcShade_White);
	TSgTextureCache.u_ratio = 1.0f / (float)TSgTextureCache.texture->width;
	TSgTextureCache.v_ratio = 1.0f / (float)TSgTextureCache.texture->height;
	TSgTextureCache.free_start = 0;
	TSgTextureCache.free_height = TSgTextureCache.texture->height;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
TSiTextureCache_RemoveCell(
	UUtUns32				inCellIndex)
{
	TStTextureCache_Cell *cell;

#if defined(DEBUGGING) && DEBUGGING
	// check that this is a valid cell and part of the currently used section of the cell array
	UUmAssert((inCellIndex >= 0) && (inCellIndex < TScTextureCache_MaxCells));
	if (inCellIndex >= TSgTextureCache.cell_start) {
		UUmAssert(inCellIndex < TSgTextureCache.cell_start + TSgTextureCache.cell_count);
	} else {
		UUmAssert((inCellIndex + TScTextureCache_MaxCells) < TSgTextureCache.cell_start + TSgTextureCache.cell_count);
	}
#endif

	cell = &TSgTextureCache.cells[inCellIndex];

	// This glyph and cell are no longer associated
	UUmAssertReadPtr(cell->glyph, sizeof(TStGlyph));
	UUmAssert(cell->glyph->cell == (inCellIndex | TScCellMask_Valid));
	cell->glyph->cell = 0;
        cell->glyph = NULL;
}

// ----------------------------------------------------------------------
static void
TSiTextureCache_RemoveRow(
	void)
{
	UUtUns32 itr, next_index;
	TStTextureCache_Row *row;

	if (TSgTextureCache.row_count == 0) {
		UUmAssert(!"TSiTextureCache_RemoveRow: no rows to remove!");
		return;
	}

	row = &TSgTextureCache.rows[TSgTextureCache.row_start];

	// remove the cells from the array
	UUmAssert(TSgTextureCache.cell_start == row->cell_start);
	UUmAssert(TSgTextureCache.cell_count >= row->cell_count);

	for (itr = 0; itr < row->cell_count; itr++) {
		TSiTextureCache_RemoveCell(TSgTextureCache.cell_start);

		TSgTextureCache.cell_start = (TSgTextureCache.cell_start + 1) % TScTextureCache_MaxCells;
		TSgTextureCache.cell_count--;
	}

	if (TSgTextureCache.row_count == 1) {
		// free space is now the entire texture
		TSgTextureCache.free_start = 0;
		TSgTextureCache.free_height = TSgTextureCache.texture->height;
	} else {
		// if this row started at the bottom of free space, then increase it
		UUmAssert(TSgTextureCache.free_start + TSgTextureCache.free_height <= TSgTextureCache.texture->height);
		if (TSgTextureCache.free_start + TSgTextureCache.free_height == row->y_start) {
			TSgTextureCache.free_height += row->y_height;
		}

		next_index = (TSgTextureCache.row_start + 1) % TScTextureCache_MaxRows;
		if (TSgTextureCache.rows[next_index].y_start < row->y_start) {
			// this is the last row in the texture, free space extends all the way to the end of the texture now
			TSgTextureCache.free_height = TSgTextureCache.texture->height - TSgTextureCache.free_start;
		}
	}

//	COrConsole_Printf("  removed %d (%d) -> now free %d (%d)", row->y_start, row->y_height, 
//						TSgTextureCache.free_start, TSgTextureCache.free_height);

#if defined(DEBUGGING) && DEBUGGING
	{
		UUtRect bounds;

		bounds.left = 0;
		bounds.top = (UUtInt16) row->y_start;
		bounds.right = (UUtInt16) row->x_width;
		bounds.bottom = bounds.top + (UUtInt16) row->y_height;

		UUmAssert(bounds.left >= 0);
		UUmAssert(bounds.top >= 0);
		UUmAssert(bounds.right <= TSgTextureCache.texture->width);
		UUmAssert(bounds.bottom <= TSgTextureCache.texture->height);
		UUmAssert(bounds.right >= bounds.left);
		UUmAssert(bounds.bottom >= bounds.top);
		
		M3rTextureMap_Fill_Shade(TSgTextureCache.texture, IMcShade_None, &bounds);
	}
#endif

	// remove the row
	TSgTextureCache.row_start = (TSgTextureCache.row_start + 1) % TScTextureCache_MaxRows;
	TSgTextureCache.row_count--;
}

// ----------------------------------------------------------------------
static UUtError
TSiTextureCache_GetCell(
	UUtUns32				inWidth,
	UUtUns32				inHeight,
	UUtUns32				*outIndex,
	IMtPoint2D				*outPosition)
{
	UUtUns32 cell_index;
	UUtUns32 row_index;
	TStTextureCache_Row *row;

	// check that there is room in the texture cache's cell array
	while (TSgTextureCache.cell_count >= TScTextureCache_MaxCells) {
		UUmAssert(TSgTextureCache.row_count > 0);
//		COrConsole_Printf("removerow: cell overflow");
		TSiTextureCache_RemoveRow();
	}

	// determine whether we must make a new row for this glyph
	row = NULL;
	if (TSgTextureCache.row_count > 0) {		
		// find the last row
		row_index = (TSgTextureCache.row_start + TSgTextureCache.row_count - 1) % TScTextureCache_MaxRows;
		row = &TSgTextureCache.rows[row_index];

		if (row->x_width + inWidth > TSgTextureCache.texture->width) {
			// the cell will not fit on the end of the current row, we
			// must make a new one
			row = NULL;

		} else if (row->y_start + inHeight > TSgTextureCache.texture->height) {
			// no matter how many rows we delete, we will never be able to free
			// up enough space to fit the cell into the current row, because it would
			// go off the end of the texture. we must make a new one.
			row = NULL;
		}
	}

	if (row == NULL) {
		// check that there is room in the texture cache's row array to add a new row
		while (TSgTextureCache.row_count >= TScTextureCache_MaxRows) {
//			COrConsole_Printf("removerow: row overflow");
			TSiTextureCache_RemoveRow();
		}

		while (TSgTextureCache.free_height < inHeight) {
			UUtBool free_space_at_start;

			free_space_at_start = (TSgTextureCache.free_start + TSgTextureCache.free_height == TSgTextureCache.texture->height);
			free_space_at_start = free_space_at_start || ((TSgTextureCache.rows[TSgTextureCache.row_start].y_start > 0) &&
									(TSgTextureCache.rows[TSgTextureCache.row_start].y_start < TSgTextureCache.free_start));

			if (free_space_at_start && (TSgTextureCache.free_start > 0)) {
#if defined(DEBUGGING) && DEBUGGING
				// store previous values so we can look at them in the debugger
				UUtUns32 prev_free_start = TSgTextureCache.free_start;
				UUtUns32 prev_free_height = TSgTextureCache.free_height;
				prev_free_start;
				prev_free_height;
#endif
				// we cannot add this new row to the bottom of the texture,
				// we must place it at the top of the texture instead.
				TSgTextureCache.free_start = 0;
				if (TSgTextureCache.row_count == 0) {
					TSgTextureCache.free_height = TSgTextureCache.texture->height;
				} else {
					TSgTextureCache.free_height = TSgTextureCache.rows[TSgTextureCache.row_start].y_start;

#if defined(DEBUGGING) && DEBUGGING
					{
						UUtUns32 itr, index;

						// check that the "free space" is really free
						index = TSgTextureCache.row_start;
						for (itr = 0; itr < TSgTextureCache.row_count; itr++) {
							UUmAssert((TSgTextureCache.rows[index].y_start >= TSgTextureCache.free_start + TSgTextureCache.free_height) ||
									  (TSgTextureCache.rows[index].y_start + TSgTextureCache.rows[index].y_height <= TSgTextureCache.free_start));

							index = (index + 1) % TScTextureCache_MaxRows;
						}
					}
#endif
				}

			} else {
				// we cannot add this row, there is not enough free space
				if (TSgTextureCache.row_count == 0) {
					// there are no rows in the texture, there will never be enough free space
					return UUcError_Generic;
				} else {
					// remove a row and keep trying
//					COrConsole_Printf("removerow: free %d (%d), request %d", TSgTextureCache.free_start, TSgTextureCache.free_height, inHeight);
					TSiTextureCache_RemoveRow();
				}
			}
		}

		// place the row at the start of the free space
		row_index = (TSgTextureCache.row_start + TSgTextureCache.row_count) % TScTextureCache_MaxRows;
		row = &TSgTextureCache.rows[row_index];

		row->cell_count = 0;
		row->cell_start = (TSgTextureCache.cell_count + TSgTextureCache.cell_start) % TScTextureCache_MaxCells;
		row->x_width = 0;
		row->y_height = 0;
		row->y_start = TSgTextureCache.free_start;

		TSgTextureCache.row_count++;
	}

#if defined(DEBUGGING) && DEBUGGING
	// we must now have room to place the cell here in the texture... note that
	// moving other rows out of the way has not yet been considered however.
	UUmAssert(row != NULL);
	UUmAssert(row->x_width + inWidth <= TSgTextureCache.texture->width);
	UUmAssert(row->y_start + inHeight <= TSgTextureCache.texture->height);
#endif

	// grow the row to fit the new cell if necessary
	if (inHeight > row->y_height) {
		while (row_index != TSgTextureCache.row_start) {
			TStTextureCache_Row *first_row;

			UUmAssert(TSgTextureCache.row_count >= 2);
			first_row = &TSgTextureCache.rows[TSgTextureCache.row_start];

			// check to see if there is space for us to grow this row successfully
			if (row->y_start + inHeight <= TSgTextureCache.free_start + TSgTextureCache.free_height) {
				break;
			}

			// we must remove the oldest row and keep checking
//			COrConsole_Printf("removerow: free %d (%d), request row at %d -> %d", 
//							TSgTextureCache.free_start, TSgTextureCache.free_height, row->y_start, inHeight);
			TSiTextureCache_RemoveRow();
		}

		// there must now be space to grow this row
		UUmAssert(row->y_start + inHeight <= TSgTextureCache.free_start + TSgTextureCache.free_height);
		row->y_height = inHeight;

		TSgTextureCache.free_height = (TSgTextureCache.free_height + TSgTextureCache.free_start - (row->y_start + row->y_height));
		TSgTextureCache.free_start = row->y_start + row->y_height;
	}

	// get a new cell (note: we should have made room at the start of this function)
	UUmAssert(TSgTextureCache.cell_count < TScTextureCache_MaxCells);
	cell_index = (TSgTextureCache.cell_start + TSgTextureCache.cell_count) % TScTextureCache_MaxCells;
	TSgTextureCache.cell_count++;

	if (outIndex != NULL) {
		*outIndex = cell_index;
	}

	// the current row must be the last row, so it will be adding cells to the end of the
	// cell array
	UUmAssert(((row->cell_start + row->cell_count) % TScTextureCache_MaxCells) == cell_index);
	row->cell_count++;

	// place the cell at the end of the row
	if (outPosition != NULL) {
		outPosition->x = (UUtInt16) row->x_width;
		outPosition->y = (UUtInt16) row->y_start;
	}

	// update the row according to the newly added cell
	row->x_width += inWidth;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
TSiTextureCache_RequireGlyph(
	TStFont					*inFont,
	TStGlyph				*ioGlyph,
	UUtBool					*outCacheHit,
	TStTextureCache_Cell	**outCell)
{
	UUtError error;
	UUtBool cache_hit;
	UUtUns32 cell_index;
	TStTextureCache_Cell *cell;

	if (ioGlyph->cell & TScCellMask_Valid) {
		// this glyph already has a location in the cache
		cell_index = ioGlyph->cell & ~TScCellMask_Valid;

#if defined(DEBUGGING) && DEBUGGING
		// check that this is a valid cell
		UUmAssert((cell_index >= 0) && (cell_index < TScTextureCache_MaxCells));
		if (cell_index >= TSgTextureCache.cell_start) {
			UUmAssert(cell_index < TSgTextureCache.cell_start + TSgTextureCache.cell_count);
		} else {
			UUmAssert((cell_index + TScTextureCache_MaxCells) < TSgTextureCache.cell_start + TSgTextureCache.cell_count);
		}
#endif

		cell = &TSgTextureCache.cells[cell_index];
		cache_hit = UUcTrue;

#if defined(DEBUGGING) && DEBUGGING
		UUmAssert(cell->font == inFont);
		UUmAssert(cell->glyph == ioGlyph);
#endif

	} else {
		UUtUns32 width;
		UUtUns32 height;
		IMtPoint2D position;

		// get a position in the texture cache to draw this glyph at
		width = ioGlyph->width;
		height = inFont->ascending_height + inFont->descending_height;

		error = TSiTextureCache_GetCell(width, height, &cell_index, &position);
		UUmError_ReturnOnError(error);

#if defined(DEBUGGING) && DEBUGGING
		// check that this is a valid cell
		UUmAssert((cell_index >= 0) && (cell_index < TScTextureCache_MaxCells));
		if (cell_index >= TSgTextureCache.cell_start) {
			UUmAssert(cell_index < TSgTextureCache.cell_start + TSgTextureCache.cell_count);
		} else {
			UUmAssert((cell_index + TScTextureCache_MaxCells) < TSgTextureCache.cell_start + TSgTextureCache.cell_count);
		}

		// check that this is a valid location in the texture
		UUmAssert((position.x >= 0) && (position.x + width <= TSgTextureCache.texture->width));
		UUmAssert((position.y >= 0) && (position.y + height <= TSgTextureCache.texture->height));

		// check that this position does not overlap any other cells in the texture
		{
			UUtUns32 itr, index;

			index = TSgTextureCache.cell_start;
			for (itr = 0; itr < TSgTextureCache.cell_count; itr++) {
				if (index != cell_index) {
					UUmAssert((position.x + (UUtInt16) width <= TSgTextureCache.cells[index].position.x) ||
							  (position.x >= TSgTextureCache.cells[index].position.x + (UUtInt16) TSgTextureCache.cells[index].width) ||
							  (position.y + (UUtInt16) height <= TSgTextureCache.cells[index].position.y) ||
							  (position.y >= TSgTextureCache.cells[index].position.y + (UUtInt16) TSgTextureCache.cells[index].height));
				}

				index = (index + 1) % TScTextureCache_MaxCells;
			}
		}
#endif
		
		cell = &TSgTextureCache.cells[cell_index];
		cell->font = inFont;
		cell->glyph = ioGlyph;
		cell->width = width;
		cell->height = height;
		cell->position = position;

		cell->uv[0].u = position.x;
		cell->uv[0].v = position.y;
		cell->uv[1].u = cell->uv[0].u + width;
		cell->uv[1].v = cell->uv[0].v + height;

		cell->uv[0].u *= TSgTextureCache.u_ratio;
		cell->uv[0].v *= TSgTextureCache.v_ratio;
		cell->uv[1].u *= TSgTextureCache.u_ratio;
		cell->uv[1].v *= TSgTextureCache.v_ratio;

		ioGlyph->cell = cell_index | TScCellMask_Valid;
		cache_hit = UUcFalse;
	}

	if (outCell != NULL) {
		*outCell = cell;
	}
	if (outCacheHit != NULL) {
		*outCacheHit = cache_hit;
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
TSiTextureCache_RenderGlyph(
	TStFont						*inFont,
	TStGlyph					*inGlyph,
	TStTextureCache_Cell		*inCell)
{
	IMtPoint2D					texture_origin;
	UUtRect						bounds;
	IMtPoint2D					dest;
	
	texture_origin.x = 0;
	texture_origin.y = 0;
	
	bounds.left = inCell->position.x;
	bounds.top = inCell->position.y;
	bounds.right = bounds.left + (UUtInt16) (inGlyph->width);
	bounds.bottom = bounds.top + (UUtInt16) (inFont->ascending_height + inFont->descending_height);

	dest.x = bounds.left;
	dest.y = bounds.top + inFont->ascending_height;

	UUmAssert(bounds.left >= 0);
	UUmAssert(bounds.top >= 0);
	UUmAssert(bounds.right <= TSgTextureCache.texture->width);
	UUmAssert(bounds.bottom <= TSgTextureCache.texture->height);
	UUmAssert(bounds.right >= bounds.left);
	UUmAssert(bounds.bottom >= bounds.top);
	
	M3rTextureMap_Fill_Shade(TSgTextureCache.texture, IMcShade_None, &bounds);
	
	TSiGlyph_Draw(inGlyph, inFont, TSgTextureCache.texture,
				&texture_origin, &bounds, &dest, TSgTextureCache.draw_color);
}

#endif	// SINGLE_TEXTURE_CACHE

// ======================================================================
#if 0 
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
TSiCharacter_IsDoubleByte(
	const UUtUns16		inCharacter)
{
	if ((inCharacter & 0x8000) &&
		(UUmHighByte(inCharacter) != TScCaretCharacter))
	{
		return UUcTrue;
	}
	else
	{
		return UUcFalse;
	}
}

// ----------------------------------------------------------------------
static UUtUns16
TSiString_GetNextCharacter(
	const char			*inString)
{
	UUtUns16			character;
	
	character = (UUtUns16)((UUtUns8)inString[0] << 8);
	character |= (UUtUns16)((UUtUns8)inString[1]);
	
	if (TSiCharacter_IsDoubleByte(character) == UUcFalse)
	{
		character >>= 8;
	}

	return character;
}

// ----------------------------------------------------------------------
static UUtUns16
TSiString_GetPrevCharacter(
	const char			*inString)
{
	UUtUns16			character;
	
	character = *((UUtUns16*)(inString - 2));
	
	if (TSiCharacter_IsDoubleByte(character) == UUcFalse)
	{
		character &= 0x00FF;
	}
	
	return character;
}




// ======================================================================
#if 0 
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
TSiGlyph_Draw(
	TStGlyph			*inGlyph,
	TStFont				*inFont,
	M3tTextureMap		*inTexture,
	IMtPoint2D			*inTextureOrigin,
	UUtRect				*inBounds,
	IMtPoint2D			*inDestination,
	IMtPixel			inColor)
{
	UUtUns16			i, j;
	
	UUtUns8				*src_texels, *src_texel;
	UUtUns32			src_rowtexels;
	UUtInt16			src_x, src_y;
	
	UUtUns16			*dst_texels_16, *dst_texel_16;
	UUtUns32			*dst_texels_32, *dst_texel_32;
	UUtUns8				*dst_texels_8, *dst_texel_8;
	UUtUns32			dst_rowtexels;
	UUtInt16			dst_x, dst_y;

	UUtInt16			draw_width, draw_height;
		
	UUtUns32			textColorTexelValue;
	
	UUtRect 			texture_rect;
	UUtRect 			dest_rect;
	
	if (inTexture->pixels == NULL) {
		UUmAssert(!"TSiGlyph_Draw: cannot draw into texture, not loaded");
		return;
	}

	// set the texture rect
	texture_rect.left	= inTextureOrigin->x;
	texture_rect.top	= inTextureOrigin->y;
	texture_rect.right	= texture_rect.left + inTexture->width;
	texture_rect.bottom	= texture_rect.top + inTexture->height;
	
	// intersect the texture_rect with inBounds
	IMrRect_Intersect(
		&texture_rect,
		inBounds,
		&dest_rect);
	
	// do the rectangles interect?
	if ((dest_rect.right == 0) || (dest_rect.bottom == 0)) return;
	
	// set dst_x and dst_y
	dst_x = inDestination->x - inGlyph->origin_x;
	dst_y = inDestination->y - inGlyph->origin_y;
	
	// set src_x and src_y
	src_x = 0;
	src_y = 0;
	
	// set draw_width and draw_height
	draw_width = inGlyph->width;
	draw_height = inGlyph->height;
	
	// check right edge of visible area
	if ((dst_x + draw_width) > dest_rect.right)
	{
		draw_width = dest_rect.right - dst_x;
	}
	
	// check left edge of visible area
	if (dst_x < dest_rect.left)
	{
		src_x = dest_rect.left - dst_x;
		dst_x = dest_rect.left;
		draw_width -= src_x;
	}
	
	// check bottom edge of visible area
	if ((dst_y + draw_height) > dest_rect.bottom)
	{
		draw_height = dest_rect.bottom - dst_y;
	}
	
	// check top edge of visible area
	if (dst_y < dest_rect.top)
	{
		src_y = dest_rect.top - dst_y;
		dst_y = dest_rect.top;
		draw_height -= src_y;
	}
	
	// don't draw if there is nothing to draw
	if ((draw_width <= 0) || (draw_height <= 0)) return;
	
	// offset the dst_x and dst_y by the texture_origin
	dst_x -= inTextureOrigin->x;
	dst_y -= inTextureOrigin->y;
	
	// set the color
	textColorTexelValue = inColor.value;
	
	// get a pointer to the source pixels
	src_rowtexels = inGlyph->width;
	src_texels =
		((UUtUns8*)(inFont->longs + inGlyph->longs_offset)) +
		src_x +
		(src_y * src_rowtexels);

	// draw the glyph
	switch (inTexture->texelType)
	{
		// draw into a 8bit texture
		case IMcPixelType_A8:
		case IMcPixelType_I8:
		case IMcPixelType_A4I4:
			dst_rowtexels = inTexture->width;
			dst_texels_8 =
				((UUtUns8*)inTexture->pixels) +
				dst_x +
				(dst_y * dst_rowtexels);
			
			switch(inTexture->texelType)
			{
				case IMcPixelType_A8:
					for (i = 0; i < draw_height; i++)
					{
						dst_texel_8 = dst_texels_8;
						src_texel = src_texels;
						
						for (j = 0; j < draw_width; j++)
						{
							*dst_texel_8 = 0xFF - *src_texel;

							dst_texel_8++;
							src_texel++;
						}
						dst_texels_8 += dst_rowtexels;
						src_texels += src_rowtexels;
					}
				break;

				case IMcPixelType_I8:
					for (i = 0; i < draw_height; i++)
					{
						dst_texel_8 = dst_texels_8;
						src_texel = src_texels;
						
						for (j = 0; j < draw_width; j++)
						{
							*dst_texel_8 = Mix8(*dst_texel_8, (UUtUns8) textColorTexelValue, *src_texel);

							dst_texel_8++;
							src_texel++;
						}
						dst_texels_8 += dst_rowtexels;
						src_texels += src_rowtexels;
					}
				break;

				case IMcPixelType_A4I4:
					for (i = 0; i < draw_height; i++)
					{
						dst_texel_8 = dst_texels_8;
						src_texel = src_texels;
						
						for (j = 0; j < draw_width; j++)
						{
							*dst_texel_8 = MixA4I4(*dst_texel_8, (UUtUns8) textColorTexelValue, *src_texel);

							dst_texel_8++;
							src_texel++;
						}
						dst_texels_8 += dst_rowtexels;
						src_texels += src_rowtexels;
					}
				break;

				default:
					UUmAssert(0);
			}

			break;

		// draw into a 16bit texture
		case IMcPixelType_ARGB4444:
		case IMcPixelType_RGB555:
		case IMcPixelType_ARGB1555:
			// get the rowpixels and a pointer to the dst pixels
			dst_rowtexels = inTexture->width;
			dst_texels_16 =
				((UUtUns16*)inTexture->pixels) +
				dst_x +
				(dst_y * dst_rowtexels);
			
			switch (inTexture->texelType)
			{
				case IMcPixelType_ARGB4444:
					for (i = 0; i < draw_height; i++)
					{
						dst_texel_16 = dst_texels_16;
						src_texel = src_texels;
						
						for (j = 0; j < draw_width; j++)
						{
							*dst_texel_16 = Mix4444(*dst_texel_16, (UUtUns16)textColorTexelValue, *src_texel);
							dst_texel_16++;
							src_texel++;
						}
						dst_texels_16 += dst_rowtexels;
						src_texels += src_rowtexels;
					}
					break;

				case IMcPixelType_RGB555:
					for (i = 0; i < draw_height; i++)
					{
						dst_texel_16 = dst_texels_16;
						src_texel = src_texels;
						
						for (j = 0; j < draw_width; j++)
						{
							*dst_texel_16 = Mix555(*dst_texel_16, (UUtUns16)textColorTexelValue, *src_texel);
							dst_texel_16++;
							src_texel++;
						}
						dst_texels_16 += dst_rowtexels;
						src_texels += src_rowtexels;
					}
					break;
		
				case IMcPixelType_ARGB1555:
					for (i = 0; i < draw_height; i++)
					{
						dst_texel_16 = dst_texels_16;
						src_texel = src_texels;
						
						for (j = 0; j < draw_width; j++)
						{
							*dst_texel_16 = Mix1555(*dst_texel_16, (UUtUns16)textColorTexelValue, *src_texel);
							dst_texel_16++;
							src_texel++;
						}
						dst_texels_16 += dst_rowtexels;
						src_texels += src_rowtexels;
					}
					break;

				default:
					UUmAssert(0);
			}
			break;
		
		// draw into a 32bit texture
		case IMcPixelType_ARGB8888:
			dst_rowtexels = inTexture->width;
			dst_texels_32 =
				((UUtUns32*)inTexture->pixels) +
				dst_x +
				(dst_y * dst_rowtexels);
			
			for (i = 0; i < draw_height; i++)
			{
				dst_texel_32 = dst_texels_32;
				src_texel = src_texels;
				
				for (j = 0; j < draw_width; j++)
				{
					*dst_texel_32 = Mix8888(*dst_texel_32, textColorTexelValue, *src_texel);
					dst_texel_32++;
					src_texel++;
				}
				dst_texels_32 += dst_rowtexels;
				src_texels += src_rowtexels;
			}
			break;
			
		default:
			UUmAssert(0);
			break;
	}
}

#if SINGLE_TEXTURE_CACHE
// ----------------------------------------------------------------------
static void
TSiTextureCache_GetCellUVs(
	TStTextureCache_Cell	*inCell,
	M3tTextureCoord			*outUVs)
{
	// set lt and rb
	outUVs[0] = inCell->uv[0];
	outUVs[3] = inCell->uv[1];
	
	// set rt
	outUVs[1].u = outUVs[3].u;
	outUVs[1].v = outUVs[0].v;
		
	// set lb
	outUVs[2].u = outUVs[0].u;
	outUVs[2].v = outUVs[3].v;
}

#else	// SINGLE_TEXTURE_CACHE

// ----------------------------------------------------------------------
static void
TSiGlyph_DrawToCell(
	TStGlyph					*inGlyph,
	TStFont						*inFont,
	const TStFont_PrivateData	*inPrivateData,
	UUtInt16					inX,
	UUtInt16					inY)
{
	IMtPoint2D					texture_origin;
	UUtRect						bounds;
	IMtPoint2D					dest;
	IMtPixel					color;
	
	color = IMrPixel_FromShade(inPrivateData->texture->texelType, IMcShade_White);
	
	texture_origin.x = 0;
	texture_origin.y = 0;
	
	bounds.left = inX * (UUtInt16)inPrivateData->cell_width;
	bounds.top = inY * (UUtInt16)inPrivateData->cell_height;
	bounds.right = bounds.left + (UUtInt16)inPrivateData->cell_width;
	bounds.bottom = bounds.top + (UUtInt16)inPrivateData->cell_height;

	UUmAssert(bounds.left >= 0);
	UUmAssert(bounds.top >= 0);
	UUmAssert(bounds.right <= inPrivateData->texture->width);
	UUmAssert(bounds.bottom <= inPrivateData->texture->height);
	UUmAssert(bounds.right >= bounds.left);
	UUmAssert(bounds.bottom >= bounds.top);
	
	dest.x = bounds.left;
	dest.y = bounds.top + inFont->ascending_height;

	M3rTextureMap_Fill_Shade(
		inPrivateData->texture,
		IMcShade_None,
		&bounds);
	
	TSiGlyph_Draw(
		inGlyph,
		inFont,
		inPrivateData->texture,
		&texture_origin,
		&bounds,
		&dest,
		color);
}

// ----------------------------------------------------------------------
static void
TSiGlyph_SetUVsFromCell(
	const TStGlyph			*inGlyph,
	M3tTextureCoord			*outUVs)
{
	TStCell					*cell;
	
	UUmAssert(inGlyph);
	UUmAssertWritePtr(outUVs, sizeof(M3tTextureCoord) * 4);
	
	cell = (TStCell*)inGlyph->cell;
	UUmAssert(cell);
	
	// update it's usage
	cell->time = UUrMachineTime_Sixtieths();

	// set lt and rb
	outUVs[0] = cell->uv[0];
	outUVs[3] = cell->uv[1];
	
	// set rt
	outUVs[1].u = outUVs[3].u;
	outUVs[1].v = outUVs[0].v;
		
	// set lb
	outUVs[2].u = outUVs[0].u;
	outUVs[2].v = outUVs[3].v;
}
#endif	// SINGLE_TEXTURE_CACHE

// ======================================================================
#if 0 
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
TSiFont_ByteSwapper(
	void						*inInstancePtr)
{
	return UUcError_None;
}

// ----------------------------------------------------------------------
#if SINGLE_TEXTURE_CACHE
void
TSrFont_DisplayCacheTexture(
	void)
{
	M3tTextureCoord	uv[4];
	M3tPointScreen points[2];

	M3rGeom_State_Push();

	M3rDraw_State_SetInt(M3cDrawStateIntType_Interpolation, M3cDrawState_Interpolation_None);
	M3rDraw_State_SetInt(M3cDrawStateIntType_Fill, M3cDrawState_Fill_Solid);
	M3rDraw_State_SetInt(M3cDrawStateIntType_ZCompare, M3cDrawState_ZCompare_Off);
	M3rDraw_State_SetInt(M3cDrawStateIntType_Appearence, M3cDrawState_Appearence_Texture_Unlit);
	M3rDraw_State_SetInt(M3cDrawStateIntType_Fill, M3cDrawState_Fill_Solid);				
	M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor,	IMcShade_White);
	M3rDraw_State_SetInt(M3cDrawStateIntType_Alpha, M3cMaxAlpha);
	M3rDraw_State_SetPtr(M3cDrawStatePtrType_BaseTextureMap, TSgTextureCache.texture);	

	M3rDraw_State_Commit();

	points[0].x = 10.0f;
	points[0].y = 100.0f;
	points[0].z = 0.5f;
	points[0].invW = 2.0f;

	points[1] = points[0];
	points[1].x += TScTextureCache_Width;
	points[1].y += TScTextureCache_Height;

	uv[0].u = 0.0f;
	uv[0].v = 0.0f;
	uv[1].u = 1.0f;
	uv[1].v = 0.0f;
	uv[2].u = 0.0f;
	uv[2].v = 1.0f;
	uv[3].u = 1.0f;
	uv[3].v = 1.0f;
	
	M3rDraw_Sprite(points, uv);

	M3rGeom_State_Pop();
}

#else	// SINGLE_TEXTURE_CACHE

void
TSrFont_DisplayCacheTexture(
	void)
{
	// no global debugging display
}

static void
TSiFont_DisplayCacheTexture(
	const TStFont			*inFont,
	TStCell					*inCell)
{
	M3tPointScreen			screen_dest;

	screen_dest.x = 10.0f;
	screen_dest.y = 100.0f;
	screen_dest.z = 0.5f;
	screen_dest.invW = 2.0f;

	{
		M3tTextureCoord	uv[4];
		M3tPointScreen points[2];

		points[0] = points[1] = screen_dest;
		points[1].x += 256.0f;
		points[1].y += 256.0f;

		uv[0].u = 0.0f;
		uv[0].v = 0.0f;
		uv[1].u = 1.0f;
		uv[1].v = 0.0f;
		uv[2].u = 0.0f;
		uv[2].v = 1.0f;
		uv[3].u = 1.0f;
		uv[3].v = 1.0f;
		
		M3rDraw_Sprite(points, uv);
	}
	
	if (inCell)
	{
		M3tTextureMap			*font_texture = TSiFont_GetTexture(inFont);
		UUtInt32 start_x = (UUtInt32) (inCell->uv[0].u * font_texture->width);
		UUtInt32 start_y = (UUtInt32) (inCell->uv[0].v * font_texture->height);
		UUtInt32 end_x = (UUtInt32) (inCell->uv[1].u * font_texture->width);
		UUtInt32 end_y = (UUtInt32) (inCell->uv[1].v * font_texture->height);
		
		M3rDraw_Sprite_Debug(font_texture, &screen_dest, start_x, start_y, end_x-start_x, end_y-start_y);
	}
}
#endif	// SINGLE_TEXTURE_CACHE

#if !SINGLE_TEXTURE_CACHE
// ----------------------------------------------------------------------
static UUtError
TSiFont_Initialize(
	TStFont						*inFont,
	TStFont_PrivateData			*inPrivateData)
{
	UUtError					error;
	UUtUns16					max_glyph_width;
	UUtUns32					i;
	UUtUns32					num_glyphs;
	TStGlyph					*glyph;
	UUtUns32					needed_cells_y;
	
	// clear the memory
	UUrMemory_Clear(inPrivateData, sizeof(TStFont_PrivateData));
	
	// get the number of glyphs in the font and the width of the widest glyph
	num_glyphs = 0;
	max_glyph_width = 0;
	for (i = 0; i < (256 * 256); i++)
	{
		glyph = TSiFont_GetGlyph(inFont, (UUtUns16)i);
		if (glyph)
		{
			num_glyphs++;
			
			if (glyph->width > max_glyph_width)
			{
				max_glyph_width = glyph->width;
			}
		}
	}
	
	// initialize the private data
	inPrivateData->cell_width		= max_glyph_width;
	inPrivateData->cell_height		= TSrFont_GetAscendingHeight(inFont) +
										TSrFont_GetDescendingHeight(inFont);
	inPrivateData->num_cells_x		= M3cTextureMap_MaxWidth / inPrivateData->cell_width;
	inPrivateData->num_cells_y		= M3cTextureMap_MaxWidth / inPrivateData->cell_height;
	inPrivateData->num_cells		= inPrivateData->num_cells_x * inPrivateData->num_cells_y;
	
	// try to adjust inPrivateData->num_cells_y to minimize the number of cells created
	needed_cells_y = (num_glyphs / inPrivateData->num_cells_x) + 1;
	if (needed_cells_y < inPrivateData->num_cells_y)
	{
		inPrivateData->num_cells_y	= needed_cells_y;
		inPrivateData->num_cells	= inPrivateData->num_cells_x * inPrivateData->num_cells_y;
	}
	
	// create a texture map
	error =
		M3rTextureMap_New(
			M3cTextureMap_MaxWidth,
			M3cTextureMap_MaxHeight,
			IMcPixelType_A8,
			M3cTexture_AllocMemory,
			M3cTextureFlags_None,
			"Font Texture",
			&inPrivateData->texture);
	UUmError_ReturnOnErrorMsg(error, "Unable to create texture map");
	
	M3rTextureMap_Fill_Shade(
		inPrivateData->texture, 
		IMcShade_None,
		NULL);
	
	// calculate the u and v ratios
	inPrivateData->u_ratio			= 1.0f / (float)inPrivateData->texture->width;
	inPrivateData->v_ratio			= 1.0f / (float)inPrivateData->texture->height;

	// allocate the cells
	inPrivateData->cells = (TStCell*)UUrMemory_Block_NewClear(sizeof(TStCell) * inPrivateData->num_cells);
	UUmError_ReturnOnNull(inPrivateData->cells);
	
// preload some characters
#if 1
	if (1)
	{
		UUtInt16			x;
		UUtInt16			y;
		char				*string = 
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"_=+-[]{};:'\",./<>?!@#$%^&*()"
			"abcdefghijklmnopqrstuvwxyz"
			"1234567890";
		
		for (y = 0; y < (UUtInt16)inPrivateData->num_cells_y; y++)
		{
			for (x = 0; x < (UUtInt16)inPrivateData->num_cells_x; x++)
			{
				TStGlyph	*glyph;
				UUtUns16	character;
				
				character = TSiString_GetNextCharacter(string);
				glyph = TSiFont_GetGlyph(inFont, character);
				if (glyph == NULL) { break; }
				TSiGlyph_DrawToCell(
					glyph,
					inFont,
					inPrivateData,
					x,
					y);
				if (TSiCharacter_IsDoubleByte(character))
				{
					string += 2;
				}
				else
				{
					string += 1;
				}
			}
			if (x < (UUtInt16)inPrivateData->num_cells_x) { break; }
		}
	}
#endif

	return UUcError_None;
}
#endif	// !SINGLE_TEXTURE_CACHE

// ----------------------------------------------------------------------
static TStGlyph*
TSiFont_GetGlyph(
	const TStFont			*inFont,
	const UUtUns16			inCharacter)
{
	UUtUns8					glyph_array_index;
	TStGlyph				*glyph;
	
	// no glyph yet
	glyph = NULL;
	
	// get the index of the glyph array
	glyph_array_index = HIGH_BYTE(inCharacter);
	
	// make sure the glyph array exists
	if (inFont->glyph_arrays[glyph_array_index])
	{
		// return the glyph
		glyph = &inFont->glyph_arrays[glyph_array_index]->glyphs[LOW_BYTE(inCharacter)];
	}
	
	return glyph;
}

#if !SINGLE_TEXTURE_CACHE
// ----------------------------------------------------------------------
static M3tTextureMap*
TSiFont_GetTexture(
	const TStFont			*inFont)
{
	TStFont_PrivateData		*private_data;
	
	// get the private data
	private_data = (TStFont_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(TSgTemplate_Font_PrivateData, inFont);
	UUmAssert(private_data);
	
	return private_data->texture;
}

// ----------------------------------------------------------------------
static UUtError
TSiFont_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void					*inPrivateData)
{
	UUtError				error;
	TStFont					*font;
	TStFont_PrivateData		*private_data;
	
	// get a pointer to the font
	font = (TStFont*)inInstancePtr;
	UUmAssert(font);
	
	// get a pointer to the font's private data
	private_data = (TStFont_PrivateData*)inPrivateData;
	UUmAssert(private_data);
	
	switch(inMessage)
	{
		case TMcTemplateProcMessage_NewPostProcess:
		case TMcTemplateProcMessage_LoadPostProcess:
			error = TSiFont_Initialize(font, private_data);
			UUmError_ReturnOnError(error);
		break;
		
		case TMcTemplateProcMessage_DisposePreProcess:
			if (private_data->cells)
			{
				UUrMemory_Block_Delete(private_data->cells);
				private_data->cells = NULL;
			}
		break;
		
		case TMcTemplateProcMessage_Update:
		break;
		
		default:
			UUmAssert(!"Illegal message");
		break;
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
TSiFont_PutGlyphInCell(
	TStFont					*inFont,
	TStGlyph				*ioGlyph)
{
	TStFont_PrivateData		*private_data;
	UUtUns32				i;
	UUtUns32				oldest_time;
	UUtUns32				oldest_cell;
	TStCell					*cell;
	float					x;
	float					y;
	
	// get the private data
	private_data = (TStFont_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(TSgTemplate_Font_PrivateData, inFont);
	UUmAssert(private_data);
	
	// find the oldest cell
	oldest_time = UUrMachineTime_Sixtieths();
	oldest_cell = 0;
	
	for (i = 0; i < private_data->num_cells; i++)
	{
		if (private_data->cells[i].glyph == NULL)
		{
			oldest_cell = i;
			break;
		}
		
		if (private_data->cells[i].time < oldest_time)
		{
			oldest_time = private_data->cells[i].time;
			oldest_cell = i;
		}
	}
   
	// get a pointer to the cell
	cell = &private_data->cells[oldest_cell];
	
	// make sure the old glyph no longer points to this cell
	if (cell->glyph)
	{
		cell->glyph->cell = 0;
	}
	
	// put the new glyph into the cell	
	ioGlyph->cell			= (UUtUns32)cell;
	cell->glyph				= ioGlyph;
	cell->time				= UUrMachineTime_Sixtieths();
	
	// calculate the x and y of the cell
	y = (float)(oldest_cell / private_data->num_cells_x);
	x = (float)(oldest_cell - (y * private_data->num_cells_x));
	
	// calculate (left, top)
	cell->uv[0].u = x * private_data->cell_width;
	cell->uv[0].v = y * private_data->cell_height;
	
	// calculate (right, bottom)
	cell->uv[1].u = cell->uv[0].u + (float)ioGlyph->width;
	cell->uv[1].v = cell->uv[0].v + (float)(inFont->ascending_height + inFont->descending_height);
	
	// divide (left, top) and (right, bottom) values by the texture width and height
	cell->uv[0].u *= private_data->u_ratio;
	cell->uv[0].v *= private_data->v_ratio;
	cell->uv[1].u *= private_data->u_ratio;
	cell->uv[1].v *= private_data->v_ratio;
	
	// draw the glyph into the texture
	TSiGlyph_DrawToCell(ioGlyph, inFont, private_data, (UUtInt16)x, (UUtInt16)y);
}
#endif	// !SINGLE_TEXTURE_CACHE

// ======================================================================
#if 0 
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
TSiFontFamily_IsBreakingCharacter(
	const TStFontFamily		*inFontFamily,
	const UUtUns16			inCharacter)
{
	char					*string;
	UUtBool					result;
	
	string = inFontFamily->font_language->breaking_2;
	result = UUcFalse;
	
	while (1)
	{
		UUtUns16		character;
		
		character = TSiString_GetNextCharacter(string);
		
		// stop if inCharacter is found in the list
		if (character == inCharacter)
		{
			result = UUcTrue;
			break;
		}
		
		// stop if character is '\0'
		if (character == '\0')
		{
			break;
		}
		
		if (TSiCharacter_IsDoubleByte(character))
		{
			string += 2;
		}
		else
		{
			string += 1;
		}
	}
	
	return result;
}

// ======================================================================
#if 0 
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
// this function returns true if the font style was changed it returns
// false otherwise
// ----------------------------------------------------------------------
static void
TSiContext_SetFontStyleByChar(
	TStTextContext		*inTextContext,
	UUtUns16			inCharacter)
{
// XXX - make this function change the font sytle currently in use

}

// ----------------------------------------------------------------------
static void
TSiContext_DrawLine(
	const TStTextContext	*inTextContext,
	M3tTextureMap			*inTexture,
	IMtPoint2D				*inTextureOrigin,
	char					*inString,
	UUtUns16				inNumCharacters,
	IMtShade				inShade,
	UUtRect 				*inBounds,
	IMtPoint2D				*inDestination)
{
	UUtError				i;
	IMtPoint2D				dest;
	IMtPixel				color;
	
	// copy the destination
	dest = *inDestination;
	
	// pre-clip top and bottom edge XXX - this may not work quite right
	if (((dest.y + TSrFont_GetLineHeight(inTextContext->font)) < inTextureOrigin->y) ||
		(dest.y > (inTextureOrigin->y + inTexture->height)))
	{
		// this line is not visible
		return;
	}
	
	// calculate the color
	color = IMrPixel_FromShade(inTexture->texelType, inShade);
	
	for (i = 0; i < inNumCharacters; i++)
	{
		TStGlyph			*glyph;
		UUtUns16			character;
		
		// get the next character
		character = TSiString_GetNextCharacter(inString);
		


		// get the glyph for the character
		glyph = TSiFont_GetGlyph(inTextContext->font, character);
		if (glyph)
		{
			// pre-clip left edge
			if ((dest.x + glyph->kerning) < inTextureOrigin->x)
			{
				// move to location of next character
				dest.x += glyph->kerning;
				
				// this character is not visible go on to the next one
				continue;
			}
			
			// pre-clip right edge
			if (dest.x > (inTextureOrigin->x + inTexture->width))
			{
				// the text has gone off the end of the texture map
				break;
			}

			// draw the character on the texture
			TSiGlyph_Draw(
				glyph,
				inTextContext->font,
				inTexture,
				inTextureOrigin,
				inBounds,
				&dest,
				color);
			
			// move to location of next character
			dest.x += glyph->kerning;
		}
/*		else
		{
			// change the font style
			TSiContext_SetFontStyleByChar(inTextContext, character);
		}*/
		
		// advance the string to the next character
		if (TSiCharacter_IsDoubleByte(character))
		{
			inString += 2;
		}
		else
		{
			inString += 1;
		}
	}
}

// ----------------------------------------------------------------------
static void
TSiContext_DrawSprites(
	const DCtDrawContext	*inDrawContext,
	UUtUns32				inNumSprites,
	M3tPointScreen			*inScreenPoints,
	M3tTextureCoord			*inTextureCoords,
	UUtBool					inUpdateTexture,
	M3tTextureMap			*inTextureMap)
{
	if (inNumSprites > 0) {
		UUtUns32 itr;

		if (inUpdateTexture) {
			// if we need to, update the texturemap
			M3rDraw_State_SetPtr(M3cDrawStatePtrType_BaseTextureMap, inTextureMap);
			TMrInstance_Update(inTextureMap);
			M3rDraw_State_Commit();
		}

		for (itr = 0; itr < inNumSprites; itr++) {
			M3tPointScreen *screen_dest = inScreenPoints + (2 * itr);
			M3tTextureCoord *uv = inTextureCoords + (4 * itr);

			if (inDrawContext) {
				DCrDraw_Glyph(inDrawContext, screen_dest, uv);
			} else {
				M3rDraw_Sprite(screen_dest, uv);
			}
		}
	}
}

// ----------------------------------------------------------------------
static void
TSiContext_DrawTextLine(
	const TStTextContext	*inTextContext,
	char					*inString,
	const UUtUns16			inNumCharacters,
	IMtShade				inShade,
	UUtInt32				inAlpha,
	const UUtRect			*inBounds,
	const DCtDrawContext	*inDrawContext)
{
	TStFont					*font;
	UUtBool					update_texture;
	UUtUns32				i;
	IMtPoint2D				dest;
	UUtRect					bounds;
	UUtUns32				num_sprites;
	M3tTextureCoord			sprite_uv[4 * TScTextLineDrawClumpSize], *cur_uv;
	M3tPointScreen			sprite_dest[2 * TScTextLineDrawClumpSize], *cur_dest;
	M3tTextureMap			*font_texture;
	
	UUmAssert(inTextContext);
	UUmAssert(inString);
	UUmAssert(inBounds);
   
	// get a pointer to the font
	font = inTextContext->font;
	UUmAssert(font);
	
#if SINGLE_TEXTURE_CACHE
	font_texture = TSgTextureCache.texture;
#else
	font_texture = TSiFont_GetTexture(font);
#endif

	if (font_texture == NULL) { return; }
	
	// set up the states
	M3rDraw_State_Push();

	M3rDraw_State_SetInt(M3cDrawStateIntType_Appearence, M3cDrawState_Appearence_Texture_Unlit);
	M3rDraw_State_SetInt(M3cDrawStateIntType_Interpolation, M3cDrawState_Interpolation_None);
	M3rDraw_State_SetInt(M3cDrawStateIntType_Fill, M3cDrawState_Fill_Solid);
	M3rDraw_State_SetInt(M3cDrawStateIntType_ZCompare, M3cDrawState_ZCompare_Off);
	M3rDraw_State_SetInt(M3cDrawStateIntType_Fill, M3cDrawState_Fill_Solid);				
	M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor,	inShade);
	M3rDraw_State_SetInt(M3cDrawStateIntType_Alpha, inAlpha);
	M3rDraw_State_SetPtr(M3cDrawStatePtrType_BaseTextureMap, font_texture);

	M3rDraw_State_Commit();

	// init the dest
	dest.x = inBounds->left;
	dest.y = inBounds->top;
	num_sprites = 0;
	cur_uv = sprite_uv;
	cur_dest = sprite_dest;
	update_texture = UUcFalse;

	// draw the characters of the string
	for (i = 0; i < inNumCharacters; i++)
	{
		TStGlyph			*glyph;
		UUtUns16			character;
		
		// get the next character
		character = TSiString_GetNextCharacter(inString);
		
		// get the glyph for the character
		glyph = TSiFont_GetGlyph(font, character);
		if (glyph)
		{
			UUtError			error;

#if SINGLE_TEXTURE_CACHE
			TStTextureCache_Cell *cell;
			UUtBool cache_hit;

			// make sure the glyph is in the texture cache
			error = TSiTextureCache_RequireGlyph(font, glyph, &cache_hit, &cell);
			if (error == UUcError_None) {
				if (!cache_hit) {
					// we must write in this glyph
					TSiTextureCache_RenderGlyph(font, glyph, cell);
					update_texture = UUcTrue;
				}
				TSiTextureCache_GetCellUVs(cell, cur_uv);

				// set up the bounds
				bounds.left		= dest.x;
				bounds.top		= dest.y;
				bounds.right	= bounds.left + (UUtInt16) cell->width;
				bounds.bottom	= bounds.top + (UUtInt16) cell->height;
			}
#else
			// make sure the glyph is in a cell
			if (glyph->cell == 0)
			{
				TSiFont_PutGlyphInCell(font, glyph);
				UUmAssert(glyph->cell);

				update_texture = UUcTrue;
			}
			error = UUcError_None;

			// set the UVs
			TSiGlyph_SetUVsFromCell(glyph, cur_uv);
			
			// set up the bounds
			bounds.left		= dest.x;
			bounds.top		= dest.y;
			bounds.right	= bounds.left + glyph->width;
			bounds.bottom	= bounds.top + font->ascending_height + font->descending_height;
#endif
			
			// draw the glyph if it is visible
			if ((error == UUcError_None) && (WMrClipRect(inBounds, &bounds, cur_uv)))
			{
				cur_dest[0].x = (float)(bounds.left);
				cur_dest[0].y = (float)(bounds.top);
				cur_dest[0].z = 0.5f;
				cur_dest[0].invW = 2.f;
				
				cur_dest[1].x = (float)(bounds.right);
				cur_dest[1].y = (float)(bounds.bottom);
				cur_dest[1].z = 0.5f;
				cur_dest[1].invW = 2.f;
				
				num_sprites++;
				cur_dest += 2;
				cur_uv += 4;

				if (num_sprites >= TScTextLineDrawClumpSize) {
					// we cannot fit any more sprite information in our fixed-length
					// arrays, we must draw it all out now
					TSiContext_DrawSprites(inDrawContext, num_sprites, sprite_dest, sprite_uv, update_texture, font_texture);
					num_sprites = 0;
					cur_dest = sprite_dest;
					cur_uv = sprite_uv;
					update_texture = UUcFalse;
				}
			}
			
			dest.x += glyph->kerning;
			
			// advance the string to the next character
			if (TSiCharacter_IsDoubleByte(character))
			{
				inString += 2;
			}
			else
			{
				inString += 1;
			}
		}
	}

	// draw any remaining sprites in the buffer
	TSiContext_DrawSprites(inDrawContext, num_sprites, sprite_dest, sprite_uv, update_texture, font_texture);

	M3rDraw_State_Pop();
}

// ======================================================================
#if 0 
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
TSrInitialize(
	void)
{
	UUtError	error;
	
	// register the templates of the Text System
	error = TSrRegisterTemplates();
	UUmError_ReturnOnError(error);

	error = TSrTextureCache_Initialize();
	UUmError_ReturnOnError(error);
	
#if !SINGLE_TEXTURE_CACHE
	error = 
		TMrTemplate_PrivateData_New(
			TScTemplate_Font,
			sizeof(TStFont_PrivateData),
			TSiFont_ProcHandler,
			&TSgTemplate_Font_PrivateData);
	UUmError_ReturnOnErrorMsg(error, "Could not install the proc handler");
#endif

	error = 
		TMrTemplate_InstallByteSwap(
			TScTemplate_Font,
			TSiFont_ByteSwapper);
	UUmError_ReturnOnErrorMsg(error, "Could not install byte swapper");

	return UUcError_None;
}
	
// ----------------------------------------------------------------------
void
TSrTerminate(
	void)
{
}

// ======================================================================
#if 0 
#pragma mark -
#endif
// ======================================================================
static UUtError
TSiContext_GetBestFont(
	TStTextContext			*ioTextContext)
{
	UUtUns16 i;
	UUtBool try_larger;

	// start with the desired size; try sizes all the way down to 1, and if none have a font
	// then try looking up, to a max of TScFontSize_MaxSearch.
	i = ioTextContext->font_size;
	try_larger = UUcFalse;

	while (1) {
		// try the desired style
		ioTextContext->font =
			TSrFontFamily_GetFont(
				ioTextContext->font_family, 
				i,
				ioTextContext->font_style);
		if (ioTextContext->font != NULL) {
			break;
		}
		
		// try plain style
		ioTextContext->font =
			TSrFontFamily_GetFont(
				ioTextContext->font_family,
				i,
				TScStyle_Plain);
		if (ioTextContext->font != NULL) {
			ioTextContext->font_style = TScStyle_Plain;
			break;
		}

		// try the next size
		if (!try_larger) {
			if (i <= 1) {
				// there are no smaller sizes
				i = ioTextContext->font_size;
				try_larger = UUcTrue;
			} else {
				i--;
				continue;
			}
		}

		if (i >= TScFontSize_MaxSearch) {
			// give up, there are no valid fonts
			break;
		} else {
			i++;
		}
	}
	
	if (ioTextContext->font == NULL) {
		return UUcError_Generic;
	} else {
		ioTextContext->font_size = i;
		return UUcError_None;
	}
}

// ----------------------------------------------------------------------
void
TSrContext_Delete(
	TStTextContext			*inTextContext)
{
	UUmAssert(inTextContext != NULL);
	
	// delete the memory held by the text context
	UUrMemory_Block_Delete(inTextContext);
}
	
// ----------------------------------------------------------------------
UUtError
TSrContext_DrawString(
	const TStTextContext	*inTextContext,
	void					*inTexture,
	const char				*inString,
	const UUtRect 			*inBounds,
	const IMtPoint2D		*inDestination)
{
	UUtError			error;
	UUtUns32			i;
	TStStringFormat		string_format;
	TMtTemplateTag		template_tag;
	IMtPoint2D			texture_origin;

	// make sure everything is ready to draw
	UUmAssert(inTextContext != NULL);
	UUmAssert(inTexture != NULL);
	UUmAssert(inString != NULL);
	UUmAssert(inBounds != NULL);
	UUmAssert(inDestination != NULL);
	UUmAssert(inTextContext->font_family != NULL);
	
	// format the string
	error = TSrContext_FormatString(inTextContext, inString, inBounds, inDestination, &string_format);
	UUmError_ReturnOnError(error);

   
	// get the template_tag for the texture
	template_tag = TMrInstance_GetTemplateTag(inTexture);
	switch (template_tag)
	{
		case M3cTemplate_TextureMap:
		{
			// set the texture origin
			texture_origin.x = 0;
			texture_origin.y = 0;
			
			// draw each segment of the string
			for (i = 0; i < string_format.num_segments; i++)
			{
				// draw the line
				TSiContext_DrawLine(
					inTextContext,
					(M3tTextureMap*)inTexture,
					&texture_origin,
					string_format.segments[i],
					string_format.num_characters[i],
					string_format.colors[i],
					&string_format.bounds[i],
					&string_format.destination[i]);
			}
		}
		break;
		
		case M3cTemplate_TextureMap_Big:
		{
			M3tTextureMap_Big	*texture_map_big;				
			
			// get a pointer to the texture map big
			texture_map_big = (M3tTextureMap_Big*)inTexture;
			
			// draw each line of the string
			for (i = 0; i < string_format.num_segments; i++)
			{
				UUtUns16			x;
				UUtUns16			y;
				
				for (y = 0; y < texture_map_big->num_y; y++)
				{
					for (x = 0; x < texture_map_big->num_x; x++)
					{
						UUtUns16			index;
						M3tTextureMap		*texture_map;
						
						// get a pointer to the texture to draw into
						index = x + (y * texture_map_big->num_x);
						texture_map = texture_map_big->textures[index];
						
						// set the texture origin
						texture_origin.x = x * M3cTextureMap_MaxWidth;
						texture_origin.y = y * M3cTextureMap_MaxHeight;

						// draw the line
						TSiContext_DrawLine(
							inTextContext,
							texture_map,
							&texture_origin,
							string_format.segments[i],
							string_format.num_characters[i],
							string_format.colors[i],
							&string_format.bounds[i],
							&string_format.destination[i]);
					}
				}
			}
		}
		break;
		
		default:
			UUmAssert(!"unkown texture map template tag");
		break;
	}
	
	// update the texture
	error = TMrInstance_Update(inTexture);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
TSrContext_DrawFormattedText(
	const TStTextContext	*inTextContext,
	TStStringFormat			*inStringFormat,
	UUtInt32				inAlpha,
	const IMtPoint2D		*inOffset,
	const IMtShade			*inOverrideColor)
{
	UUtUns32 i;
	IMtShade shade;
	UUtRect bounds;

	// draw each segment of the text
	for (i = 0; i < inStringFormat->num_segments; i++)
	{
		bounds = inStringFormat->bounds[i];
		if (inOffset != NULL) {
			bounds.left += inOffset->x;
			bounds.right += inOffset->x;
			bounds.top += inOffset->y;
			bounds.bottom += inOffset->y;
		}
		shade = (inOverrideColor == NULL) ? inStringFormat->colors[i] : *inOverrideColor;
		
		TSiContext_DrawTextLine(
			inTextContext,
			inStringFormat->segments[i],
			inStringFormat->num_characters[i],
			shade,
			inAlpha,
			&bounds,
			NULL);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
TSrContext_DrawText(
	const TStTextContext	*inTextContext,
	const char				*inString,
	UUtInt32				inAlpha,
	const UUtRect			*inBounds,
	const IMtPoint2D		*inDestination)
{
	UUtError				error;
	UUtUns32				i;
	TStStringFormat			string_format;
	UUtRect					bounds;

	// make sure everything is ready to draw
	UUmAssert(inTextContext != NULL);
	UUmAssert(inString != NULL);
	UUmAssert(inDestination != NULL);
	UUmAssert(inTextContext->font_family != NULL);

	if (NULL != inBounds)
	{
		bounds = *inBounds;
	}
	else
	{
		bounds.top = 0;
		bounds.left = 0;
		bounds.right = M3rDraw_GetWidth();
		bounds.bottom = M3rDraw_GetHeight();
	}
	
	// format the string
	error = TSrContext_FormatString(inTextContext, inString, &bounds, inDestination, &string_format);
	UUmError_ReturnOnError(error);
	
	// draw each line
	for (i = 0; i < string_format.num_segments; i++)
	{
		TSiContext_DrawTextLine(
			inTextContext,
			string_format.segments[i],
			string_format.num_characters[i],
			string_format.colors[i],
			inAlpha,
			&string_format.bounds[i],
			NULL);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
// only call from DCrDraw_String() in WM_DrawContext
UUtError
TSrContext_DrawText_DC(
	const TStTextContext	*inTextContext,
	const char				*inString,
	const UUtRect			*inBounds,
	const IMtPoint2D		*inDestination,
	const DCtDrawContext	*inDrawContext,
	UUtRect					*outStringRect)
{
	UUtError				error;
	UUtUns32				i;
	TStStringFormat			string_format;
	UUtRect					bounds;
	UUtInt16				width;
	UUtInt16				height;

	// make sure everything is ready to draw
	UUmAssert(inTextContext != NULL);
	UUmAssert(inString != NULL);
	UUmAssert(inDestination != NULL);
	UUmAssert(inTextContext->font_family != NULL);

	if (NULL != inBounds)
	{
		bounds = *inBounds;
	}
	else
	{
		bounds.top = 0;
		bounds.left = 0;
		bounds.right = M3rDraw_GetWidth();
		bounds.bottom = M3rDraw_GetHeight();
	}
	
	// format the string
	error = TSrContext_FormatString(inTextContext, inString, &bounds, inDestination, &string_format);
	UUmError_ReturnOnError(error);
	
	if (outStringRect)
	{
		width = 0;
		height = 0;
	}
	
	// draw each line
	for (i = 0; i < string_format.num_segments; i++)
	{
		TSiContext_DrawTextLine(
			inTextContext,
			string_format.segments[i],
			string_format.num_characters[i],
			string_format.colors[i],
			M3cMaxAlpha,
			&string_format.bounds[i],
			inDrawContext);
		
		if (outStringRect)
		{
			width = UUmMax(width, (string_format.bounds[i].right - string_format.bounds[i].left));
			height += (string_format.bounds[i].bottom - string_format.bounds[i].top);
		}
	}
	
	if (outStringRect)
	{
		outStringRect->left = 0; 
		outStringRect->top = 0;
		outStringRect->right = width;
		outStringRect->bottom = height;
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
TSrContext_FormatString(
	const TStTextContext	*inTextContext,
	const char				*inString,
	const UUtRect 			*inBounds,
	const IMtPoint2D		*inDestination,
	TStStringFormat			*outStringFormat)
{
	char					*string;

	UUtBool					done, must_break, never_wrap, start_new_line, intraline_break, begin_formatting, restore_formatting;

	UUtUns16				line_num;
	UUtUns16				segment_startindex, segment_num;
	UUtUns16				i, j;

	UUtInt16				line_width;
	UUtInt16				segment_width;
	UUtInt16				line_height;
	UUtInt16				ascending_height;
	UUtInt16				bounds_width;
	UUtInt16				segment_x, line_x, line_y;
	UUtInt16				segment_widths[TScMaxSegments];

	IMtShade				cur_shade;
	TStFormatCharacterColor	*formatcharacter;
	
	never_wrap				= ((inTextContext->format & (TSc_SingleLine | TSc_VBottom)) != 0);
	string					= (char*)inString;
	bounds_width			= inBounds->right - inBounds->left;
	line_height				= TSrFont_GetLineHeight(inTextContext->font);
	ascending_height		= TSrFont_GetAscendingHeight(inTextContext->font);
	cur_shade				= inTextContext->shade;

	if ((!inTextContext->use_formatting_commands) && (never_wrap))
	{
		outStringFormat->num_lines = 1;
		outStringFormat->num_segments = 1;
		outStringFormat->line_num_segments[0] = 1;
		UUrString_Copy(outStringFormat->segments[0], string, TScMaxCharsPerSegment);
		outStringFormat->num_characters[0] = TSrString_GetLength(outStringFormat->segments[0]);
		outStringFormat->colors[0] = inTextContext->shade;
		outStringFormat->bounds[0] = *inBounds;
		outStringFormat->destination[0] = *inDestination;
	}
	else
	{
		begin_formatting		= UUcFalse;
		start_new_line			= UUcTrue;
		line_num				= 0;
		segment_num				= 0;
		done					= UUcFalse;

		do
		{
			char					*starting_string;
			TStGlyph				*glyph;
			UUtUns16				character;
			UUtUns16				num_chars;
			UUtUns16				prev_breaking;
			
			starting_string			= string;
			prev_breaking			= 0;
			num_chars				= 0;
			intraline_break			= UUcFalse;
			restore_formatting		= UUcFalse;

			if (start_new_line) {
				line_width = 0;
				outStringFormat->line_num_segments[line_num] = 0;
				segment_startindex = segment_num;
			}
			segment_width = 0;

			// ------------------------------
			// go through as many characters as possible and fill in the segment
			// ------------------------------
			while(1)
			{
				// get the next character in the string
				character = TSiString_GetNextCharacter(string);
				
				// stop if the end of the string is reached
				if (character == 0)
				{
					done = UUcTrue;
					break;
				}
				
				if (inTextContext->use_formatting_commands) {
					if (character == TScFormatCharacter_Begin) {
						// begin a new section of the line	
						string += 1;
						begin_formatting = UUcTrue;
						intraline_break = UUcTrue;
						break;

					} else if (character == TScFormatCharacter_End) {
						// restore the standard format settings and begin a new section of the line
						string += 1;
						begin_formatting = UUcFalse;
						restore_formatting = UUcTrue;
						intraline_break = UUcTrue;
						break;

					} else if (begin_formatting) {
						if (character == TScFormatCharacter_FormatEnd) {
							// stop the formatting character search
							string += 1;
							starting_string = string;
							begin_formatting = UUcFalse;
							continue;
						} else {
							// check to see if this is a formatting character
							for (formatcharacter = TSgColorFormattingCharacters; formatcharacter->character != 0; formatcharacter++) {
								if (character == formatcharacter->character) {
									cur_shade = formatcharacter->shade;
									break;
								}
							}
							if (formatcharacter->character == 0) {
								// there are no more formatting characters, begin
								// the actual string
								starting_string = string;
								begin_formatting = UUcFalse;
							} else {
								// keep looking for formatting characters
								string += 1;
								continue;
							}
						}
					}
				}

				num_chars++;
				must_break = UUcFalse;

				// get the glyph from the font
				glyph = TSiFont_GetGlyph(inTextContext->font, character);
				if (glyph)
				{
					segment_width += glyph->kerning;
					
					// stop if the line exceeds the bounds rect
					must_break = ((!never_wrap) && (line_width + segment_width > bounds_width));
				}
/*				else
				{
					// change to new font style
					TSiContext_SetFontStyleByChar(inTextContext, character);
					
					// save the tallest line height and descending height
					line_height = UUmMax(TSrFont_GetLineHeight(inTextContext->font), line_height);
					ascending_height = UUmMax(TSrFont_GetAscendingHeight(inTextContext->font), ascending_height);
				}*/
				
				// advance to the next character in the string
				if (TSiCharacter_IsDoubleByte(character))
				{
					string += 2;
				}
				else
				{
					string += 1;
				}
				
				if (must_break || ((num_chars + 1) >= TScMaxCharsPerSegment))
				{
					// break this segment
					begin_formatting = UUcFalse;
					break;
				}
				else if ((TSiFontFamily_IsBreakingCharacter(inTextContext->font_family, character)) &&
						(num_chars > 0))
				{
					// this was a breaking character, save its position for later use
//					prev_breaking = num_chars - 1;
					prev_breaking = num_chars;
				}
			}
			
			if ((!done) && (!intraline_break) && (!never_wrap))
			{
				// we must break this line
				if (prev_breaking > 0)
				{
					// ------------------------------
					// back up to a break character
					// ------------------------------
					char			*temp_string;
					
					temp_string = string = &starting_string[prev_breaking];

					// subtract the width of the glyphs from prev_breaking to num_chars from segment_width
					for (i = 0; i < (num_chars - prev_breaking); i++)
					{
						// get the next character in the string
						character = TSiString_GetNextCharacter(temp_string);

						// get the glyph from the font
						glyph = TSiFont_GetGlyph(inTextContext->font, character);
						if (glyph)
						{
							segment_width -= glyph->kerning;
						}
	/*					else
						{
							TSiContext_SetFontStyleByChar(inTextContext, character);
						}*/

						// advance to the next character in the string
						if (TSiCharacter_IsDoubleByte(character))
						{
							temp_string += 2;
						}
						else
						{
							temp_string += 1;
						}
					}
				
					num_chars = prev_breaking;
				}
				else if (outStringFormat->line_num_segments[line_num] > 0)
				{
					// there are previous segments in this line, we can break at the start of this segment
					string = starting_string;
					num_chars = 0;
				}
			}

			if (num_chars > 0) {
				// copy the segment data into outStringFormat
				UUrString_Copy_Length(outStringFormat->segments[segment_num], starting_string, TScMaxCharsPerSegment, num_chars);
				outStringFormat->num_characters[segment_num] = num_chars;
				outStringFormat->colors[segment_num] = cur_shade;

				// store this segment as part of the line
				outStringFormat->line_num_segments[line_num]++;
				segment_widths[segment_num] = segment_width;
				line_width += segment_width;
				segment_num++;
			}

			if (!intraline_break) {
				// we have reached the end of a horizontal line, calculate the horizontal positions of all the segments

				// calculate the destination of the text line
				if (inTextContext->format & TSc_HCenter)
				{
					// center the text between the left and right sides of inBounds
					line_x =  inBounds->left + ((bounds_width - line_width) >> 1);
				}
				else if (inTextContext->format & TSc_HRight)
				{
					// set the text against the right side of inBounds
					line_x = inBounds->right - line_width;
				}
				else // TSc_HLeft
				{
					// set the text at the destination's x coordinate
					line_x = inDestination->x;
				}
				
				segment_x = line_x;
				for (i = segment_startindex; i < segment_num; i++) {
					// store the position of this segment. note that the final vertical position will be set
					// after all the lines have been processed. in the meantime, save the ascending_height of the line.
					outStringFormat->destination[i].x = segment_x;
					outStringFormat->destination[i].y = ascending_height;
					
					// set the bounds rect of the line
					outStringFormat->bounds[i].left = segment_x;
					outStringFormat->bounds[i].top = 0;
					outStringFormat->bounds[i].right = segment_x + segment_widths[i];
					outStringFormat->bounds[i].bottom = line_height;

					segment_x = outStringFormat->bounds[i].right;
				}
				
				line_num++;
				start_new_line = UUcTrue;
			} else {
				// continue the existing line
				start_new_line = UUcFalse;
			}
			
			// set up for the next segment
			if (restore_formatting) {
				cur_shade = inTextContext->shade;
			}

			// stop if the max number of lines is exceeded
			if ((segment_num + 1) >= TScMaxSegments)
			{
				done = UUcTrue;
			}
		}
		while (!done);

		outStringFormat->num_segments = segment_num;
		outStringFormat->num_lines = line_num;
	}
	
	// ------------------------------
	// calculate the vertical positions
	// ------------------------------
	// handle the vertical positioning of the text
	if (inTextContext->format & TSc_VCenter)
	{
		UUtUns16				total_height;
		
		// calculate how tall all the lines are
		total_height = 0;
		segment_startindex = 0;
		for (i = 0; i < outStringFormat->num_lines; i++)
		{
			total_height += outStringFormat->bounds[segment_startindex].bottom;
			segment_startindex += outStringFormat->line_num_segments[i];
		}
		UUmAssert(segment_startindex == outStringFormat->num_segments);

		// calculate the starting point for the list of strings
		line_y =
			(UUtInt16)inBounds->top +
			(((UUtInt16)inBounds->bottom - (UUtInt16)inBounds->top - (UUtInt16)total_height) >> 1);
	}
	else // TSc_VNormal
	{
		// the bottom left of the first character of the first line will start at the destination y coordinate
		// so line_y needs to be set to the point where the top left of the first character is going to
		// be drawn.
		line_y = inDestination->y - outStringFormat->destination[0].y;
	}
	
	// set the y coordinates for each line and the bounds top and bottom
	segment_startindex = 0;
	for (i = 0; i < outStringFormat->num_lines; i++)
	{
		for (j = 0; j < outStringFormat->line_num_segments[i]; j++) {
			// adjust the segments vertically to their final positions
			outStringFormat->bounds[j + segment_startindex].top += line_y;
			outStringFormat->bounds[j + segment_startindex].bottom += line_y;
			outStringFormat->destination[j + segment_startindex].y += line_y;
		}
		
		line_y = outStringFormat->bounds[segment_startindex].bottom;
		segment_startindex += outStringFormat->line_num_segments[i];
	}
	UUmAssert(segment_startindex == outStringFormat->num_segments);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
TStFont*
TSrContext_GetFont(
	const TStTextContext	*inTextContext,
	TStFontStyle			inStyle)
{
	TStFont					*font;
	
	UUmAssert(inTextContext);
	UUmAssert((inStyle >= 0) && (inStyle <= TScStyle_NumStyles));
	
	font = NULL;
	if (inStyle == TScStyle_InUse)
	{
		// return the font currently in use
		font = inTextContext->font;
	}
	else
	{
		font = 
			TSrFontFamily_GetFont(
				inTextContext->font_family,
				inTextContext->font_size,
				inStyle);
	}
	
	return font;
}

// ----------------------------------------------------------------------
UUtError
TSrContext_GetStringRect(
	const TStTextContext	*inTextContext,
	char					*inString,
	UUtRect 				*outRect)
{
	UUtUns16			string_length;
	UUtUns16			i;
	UUtUns16			width;
	
	UUmAssert(inTextContext);
	UUmAssert(inString);
	UUmAssert(outRect);
	
	// clear the width
	width = 0;
	
	// calculate the length of the string (aka number of characters)
	string_length = TSrString_GetLength(inString);
	
	// calculate the width of the string
	for (i = 0; i < string_length; i++)
	{
		TStGlyph			*glyph;
		UUtUns16			character;
		
		// get the next character in the string
		character = TSiString_GetNextCharacter(inString);
		
		// add the glyph width
		glyph = TSiFont_GetGlyph(inTextContext->font, character);
		if (glyph)
		{
			width += glyph->kerning;
		}
		
		if (TSiCharacter_IsDoubleByte(character))
		{
			inString += 2;
		}
		else
		{
			inString += 1;
		}
	}
	
	// set the return values
	outRect->left	= 0;
	outRect->top	= 0;
	outRect->right	= width;
	outRect->bottom	= TSrFont_GetLineHeight(inTextContext->font);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
TSrContext_New(
	TStFontFamily			*inFontFamily,
	UUtUns16				inSize,
	TStFontStyle			inStyle,
	TStFormat				inFormat,
	UUtBool					inUseFormatting,
	TStTextContext			**outTextContext)
{
	UUtError				error;

	UUmAssert(inFontFamily != NULL);
	UUmAssert(outTextContext != NULL);
	
	// make sure we have a place to store the text context's pointer
	if (outTextContext == NULL)
		return UUcError_Generic;
	
	// allocate the memory for the text context
	*outTextContext = UUrMemory_Block_New(sizeof(TStTextContext));
	if (*outTextContext == NULL)
		return UUcError_Generic;
	
	// initialize the text context
	(**outTextContext).font_family	= inFontFamily;
	(**outTextContext).format		= inFormat;
	(**outTextContext).font_size	= TScFontSize_Default;
	(**outTextContext).font_style	= TScFontStyle_Default;
	(**outTextContext).shade		= IMcShade_Black;
	(**outTextContext).use_formatting_commands = inUseFormatting;

	// set the font family
	error = TSrContext_SetFontFamily(*outTextContext, inFontFamily);
	if (error != UUcError_None)
		return UUcError_Generic;
	
	// set the font style
	error = TSrContext_SetFontStyle(*outTextContext, inStyle);
	if (error != UUcError_None)
		return UUcError_Generic;
	
	// set the font size
	error = TSrContext_SetFontSize(*outTextContext, inSize);
	if (error != UUcError_None)
		return UUcError_Generic;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
TSrContext_SetFontFamily(
	TStTextContext		*ioTextContext,
	TStFontFamily		*inFontFamily)
{
	UUtError			error;
	
	UUmAssert(ioTextContext != NULL);
	UUmAssert(inFontFamily != NULL);
	if ((ioTextContext == NULL) || (inFontFamily == NULL)) { return UUcError_Generic; }

	// save the font family
	ioTextContext->font_family = inFontFamily;
	
	// get the best font
	error = TSiContext_GetBestFont(ioTextContext);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
TSrContext_SetFontSize(
	TStTextContext		*ioTextContext,
	UUtUns16			inSize)
{
	UUtError			error;
	
	UUmAssert(ioTextContext != NULL);
	if (ioTextContext == NULL) { return UUcError_Generic; }
	
	// save the desired size
	ioTextContext->font_size = inSize;
	
	// get the best font
	error = TSiContext_GetBestFont(ioTextContext);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
TSrContext_SetFontStyle(
	TStTextContext		*ioTextContext,
	TStFontStyle		inStyle)
{
	UUtError			error;
	
	UUmAssert(ioTextContext != NULL);
	if (ioTextContext == NULL) { return UUcError_Generic; }
	
	// save the desired style
	ioTextContext->font_style = inStyle;
	
	// get the best font
	error = TSiContext_GetBestFont(ioTextContext);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}	

// ----------------------------------------------------------------------
UUtError
TSrContext_SetFormat(
	TStTextContext		*ioTextContext,
	TStFormat			inFontFormat)
{
	UUmAssert(ioTextContext != NULL);
	if (ioTextContext == NULL) { return UUcError_Generic; }
	
	ioTextContext->format = inFontFormat;
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
TSrContext_SetShade(
	TStTextContext		*ioTextContext,
	UUtUns32			inShade)
{
	UUmAssert(ioTextContext != NULL);
	if (ioTextContext == NULL) { return UUcError_Generic; }

	ioTextContext->shade = inShade;
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
TSrContext_SetFormatting(
	TStTextContext		*ioTextContext,
	UUtBool				inUseFormattingCommands)
{
	UUmAssert(ioTextContext != NULL);
	if (ioTextContext == NULL) { return UUcError_Generic; }

	ioTextContext->use_formatting_commands = inUseFormattingCommands;
	return UUcError_None;
}

// ======================================================================
#if 0 
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
TSrFontFamily_Get(
	char				*inFontFamilyName,
	TStFontFamily		**outFontFamily)
{
	*outFontFamily = TMrInstance_GetFromName(TScTemplate_FontFamily, inFontFamilyName);
	return UUcError_None;
}

// ----------------------------------------------------------------------
TStFont*
TSrFontFamily_GetFont(
	TStFontFamily		*inFontFamily,
	UUtUns16			inSize,
	TStFontStyle		inStyle)
{
	UUtUns32			i;
	TStFont				*found_font;
	
	found_font = NULL;
	for (i = 0; i < inFontFamily->num_fonts; i++)
	{
		TStFont			*font;
		
		font = inFontFamily->fonts[i];
		
		if (font == NULL) { continue; }
		if ((font->font_size != inSize) || (font->font_style != inStyle)) { continue; }
		
		found_font = font;
		break;
	}
	
	return found_font;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
TStFont*
TSrFont_Get(
	const char			*inName,
	UUtUns16			inSize,
	TStFontStyle		inStyle)
{
	TStFontFamily		*font_family;
	TStFont				*font;
	UUtUns32			i;
	
	font_family = TMrInstance_GetFromName(TScTemplate_FontFamily, inName);
	if (font_family == NULL) { return NULL; }
	
	font = NULL;
	
	for (i = 0; i < font_family->num_fonts; i++)
	{
		TStFont			*temp_font;
		
		temp_font = font_family->fonts[i];
		
		if ((temp_font->font_size == inSize) && (temp_font->font_style == inStyle))
		{
			font = temp_font;
			break;
		}
	}
	
	return font;
}

// ----------------------------------------------------------------------
UUtUns16
TSrFont_GetAscendingHeight(
	TStFont				*inFont)
{
	UUmAssert(inFont != NULL);
	
	return inFont->ascending_height;
}

// ----------------------------------------------------------------------
void
TSrFont_GetCharacterSize(
	TStFont				*inFont,
	UUtUns16			inCharacter,
	UUtUns16			*outWidth,
	UUtUns16			*outHeight)
{
	TStGlyph			*glyph;
	UUtUns16			width;
	UUtUns16			height;
	
	UUmAssert(inFont != NULL);
	
	// initialize width and height
	width = 0; 
	height = 0;
	
	// get the glyph
	glyph = TSiFont_GetGlyph(inFont, inCharacter);
	if (glyph)
	{
		width = glyph->width;
		height = glyph->height;
	}
	
	if (outWidth)
	{
		*outWidth = width;
	}
	
	if (outHeight)
	{
		*outHeight = height;
	}
}

// ----------------------------------------------------------------------
UUtUns16
TSrFont_GetLeadingHeight(
	TStFont				*inFont)
{
	UUmAssert(inFont != NULL);
	
	return inFont->leading_height;
}

// ----------------------------------------------------------------------
UUtUns16
TSrFont_GetLineHeight(
	TStFont				*inFont)
{
	UUmAssert(inFont != NULL);
	
	return inFont->ascending_height + inFont->descending_height + inFont->leading_height;
}

// ----------------------------------------------------------------------
UUtUns16
TSrFont_GetDescendingHeight(
	TStFont				*inFont)
{
	UUmAssert(inFont != NULL);
	
	return inFont->descending_height;
}

// ======================================================================
#if 0 
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtBool
TSrString_AppendChar(
	char				*ioString,
	const UUtUns16		inChar,
	const UUtUns16		inMaxCharsInString)
{
	UUtUns16			insert_before;
	
	// set insert_before to the point after the last character in the string
	insert_before = TSrString_GetLength(ioString);
	
	// insert the char at the end of the string
	TSrString_InsertChar(
		ioString,
		insert_before,
		inChar,
		inMaxCharsInString);
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtBool
TSrString_DeleteChar(
	char				*ioString,
	const UUtUns16		inDeleteIndex,
	const UUtUns16		inMaxCharsInString)
{
	UUtUns16			character;
	char				*copy_to;
	char				*copy_from;
	UUtUns32			i;
	
	// check the range of inInsertBefore
	if (inDeleteIndex > inMaxCharsInString) { return UUcFalse; }
	
	// find the copy_to point
	copy_to = ioString;
	for (i = 0; i < inDeleteIndex; i++)
	{
		character = TSiString_GetNextCharacter(copy_to);
		if (TSiCharacter_IsDoubleByte(character))
		{
			copy_to += 2;
		}
		else
		{
			copy_to += 1;
		}
	}
	
	// copy_to is now at position to copy data from
	copy_from = copy_to;

	// skip over the character at inDeleteIndex
	character = TSiString_GetNextCharacter(copy_from);
	if (TSiCharacter_IsDoubleByte(character))
	{
		copy_from += 2;
	}
	else
	{
		copy_from += 1;
	}
	
	// copy from copy_from to copy_to
	while (1)
	{
		character = TSiString_GetNextCharacter(copy_from);
		if (character == 0)
		{
			// copy the \0
			*((UUtUns8*)copy_to) = (UUtUns8)character;
			break;
		}
		
		if (TSiCharacter_IsDoubleByte(character))
		{
			copy_from += 2;
			*((UUtUns16*)copy_to) = character;
			copy_to += 2;
		}
		else
		{
			copy_from += 1;
			*((UUtUns8*)copy_to) = (UUtUns8)character;
			copy_to += 1;
		}
	}
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtUns16
TSrString_GetCharacterAtIndex(
	const char			*inString,
	const UUtUns16		inIndex)
{
	UUtUns16			i;
	const char			*string;
	UUtUns16			character;
	
	string = inString;
	for (i = 0; i < inIndex; i++)
	{
		character = TSiString_GetNextCharacter(string);
		
		if (TSiCharacter_IsDoubleByte(character))
		{
			string += 2;
		}
		else
		{
			string += 1;
		}
	}
	
	character = TSiString_GetNextCharacter(string);
	
	return character;
}

// ----------------------------------------------------------------------
UUtUns16
TSrString_GetLength(
	const char			*inString)
{
	UUtUns16			character = 0;
	UUtUns16			length = 0;
	
	while (1)
	{
		character = TSiString_GetNextCharacter(inString);
		if (character == 0)
		{
			break;
		}
		
		if (TSiCharacter_IsDoubleByte(character))
		{
			inString += 2;
		}
		else
		{
			inString += 1;
		}
		
		length++;
	}
	
	return length;
}

// ----------------------------------------------------------------------
UUtBool
TSrString_InsertChar(
	char				*ioString,
	UUtUns16			inInsertBefore,
	const UUtUns16		inChar,
	const UUtUns16		inMaxCharsInString)
{
	UUtUns16			string_length;
	char				*start;
	char				*end;
	char				*copy_from;
	UUtUns32			i;
	UUtUns16			character;
	UUtUns32			num_bytes;
	
	// check the range of inInsertBefore
	if (inInsertBefore > inMaxCharsInString) { return UUcFalse; }
	
	// make sure there is enough room to insert the character
	string_length = TSrString_GetLength(ioString);
	if ((string_length + 1) >= inMaxCharsInString) { return UUcFalse; }
	if (inInsertBefore > string_length) { inInsertBefore = string_length; }
	
	// find the start of the strings to copy from
	start = ioString;
	for (i = 0; i < inInsertBefore; i++)
	{
		character = TSiString_GetNextCharacter(start);
		if (character == 0) { break; }
		
		if (TSiCharacter_IsDoubleByte(character))
		{
			start += 2;
		}
		else
		{
			start += 1;
		}
	}
	
	// find the end of the string
	end = start;
	while (1)
	{
		character = TSiString_GetNextCharacter(end);
		if (character == 0)
		{
			break;
		}
		
		if (TSiCharacter_IsDoubleByte(character))
		{
			end += 2;
		}
		else
		{
			end += 1;
		}
	}
	
	// calculate the number of bytes to copy, including the \0 at the end of the string
	num_bytes = (end - start) + 1;
	
	// set the position to copy from to the current end position
	copy_from = end;
	
	// calculate the new end position
	if (TSiCharacter_IsDoubleByte(inChar))
	{
		end += 2;
	}
	else
	{
		end += 1;
	}
	
	// copy the data
	for (i = 0; i < num_bytes; i++)
	{
		*end-- = *copy_from--;
	}
	
	// insert the new character
	if (TSiCharacter_IsDoubleByte(inChar))
	{
		*((UUtUns16*)start) = inChar;
	}
	else
	{
		*((UUtUns8*)start) = (UUtUns8)inChar;
	}
	
	return UUcTrue;
}

