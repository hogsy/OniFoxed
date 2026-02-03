#pragma once

#ifndef BFW_TEXTSYSTEM_H
#define BFW_TEXTSYSTEM_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"

// ======================================================================
// defines
// ======================================================================
#define TScMaxBreakChar							64		// maximum number of breaking characters
#define TScMaxLines								24
#define TScMaxSegments							24
#define TScMaxCharsPerSegment					256

#define TScFontFamily_Tahoma					"Tahoma"

#define TScStyleCharacterCode					'\\'
#define TScCaretCharacter						0x00FF

#define TScFontLanguage_Default					TScFontLanguage_Roman
#define TScFontFamily_Default					TScFontFamily_Tahoma
#define TScFontSize_Default						(7)
#define TScFontStyle_Default					TScStyle_Plain
#define TScFontShade_Default					IMcShade_Black

// ======================================================================
// enums
// ======================================================================
enum
{
	TScFontLanguage_Roman						= 'ROMN',
	TScFontLanguage_Japanese					= 'NPPN',
	TScFontLanguage_Simple_Chinese				= 'SCHN',
	TScFontLanguage_Traditional_Chinese			= 'TCHN',
	TScFontLanguage_Korean_Wansung				= 'KRNW',
	TScFontLanguage_Korean_Johab				= 'KRNJ'

};

// ----------------------------------------------------------------------
enum
{
	TSc_HLeft				= 0x0001,				// Horizontal left justified text
	TSc_HCenter				= 0x0002,				// Horizontal centered text
	TSc_HRight				= 0x0004,				// Horizontal right justified text

	TSc_VTop				= 0x0001,				// Vertical top justified text
	TSc_VCenter				= 0x0008,				// Vertical centered text
	TSc_VBottom				= 0x0010,				// Vertical bottom justified text

	TSc_SingleLine			= 0x0020				// Text is drawn on a single line

};

// ----------------------------------------------------------------------
typedef tm_enum TStFontStyle
{
	TScStyle_Plain			= 0,					// Plain text
	TScStyle_Bold			= 1,					// Bold text
	TScStyle_Italic			= 2,					// Italic text

	TScStyle_NumStyles		= 3,
	TScStyle_InUse			= TScStyle_NumStyles	// used in TSrGetFont() to get the font
													// in use

} TStFontStyle;

// ======================================================================
// typedefs
// ======================================================================
typedef struct TStTextContext TStTextContext;
typedef UUtUns16	TStFormat;

typedef struct TStStringFormat
{
	UUtUns16				num_lines;
	UUtUns16				num_segments;
	UUtUns16				line_num_segments[TScMaxLines];
	UUtUns16				num_characters[TScMaxSegments];
	char					segments[TScMaxSegments][TScMaxCharsPerSegment];
	IMtShade				colors[TScMaxSegments];
	UUtRect 				bounds[TScMaxSegments];
	IMtPoint2D				destination[TScMaxSegments];

} TStStringFormat;

// ----------------------------------------------------------------------
typedef tm_struct TStGlyph
{
	UUtUns16				code;					// glyph code (code used in a char *)
	UUtUns16				kerning;				// the positive horizontal penoffset in
													//      pixels after drawing this character
	UUtInt16				width;					// the width in pixels of the glyph's bitmap
	UUtInt16				height;					// the height in pixels of the glyph's bitmap
	UUtInt16				origin_x;				// the x coordinate of the bitmap's origin
	UUtInt16				origin_y;				// the y coordinate of the bitmap's origin
													// NOTE: origin_x and origin_y can be bigger
													//		 than the width and height

	UUtUns32				longs_offset;			// offset into longs[] array

	UUtUns32				cell;					// pointer to cell  -- only used at runtime

} TStGlyph;

// ----------------------------------------------------------------------
#define TScTemplate_GlyphArray		UUm4CharToUns32('T', 'S', 'G', 'A')

typedef tm_template('T', 'S', 'G', 'A', "Glyph Array")
TStGlyphArray
{

	TStGlyph				glyphs[256];			// array of glyphs

} TStGlyphArray;

// ----------------------------------------------------------------------
#define TScTemplate_Font		UUm4CharToUns32('T', 'S', 'F', 'T')

typedef tm_template	('T', 'S', 'F', 'T', "Font")
TStFont
{
	tm_pad					pad[6];

	UUtUns16				font_size;				// size of this font
	TStFontStyle			font_style;				// style of this font

	UUtInt16				ascending_height;		// character_height == ascending_height +
	UUtInt16				descending_height;		//						descending_height

	UUtInt16				leading_height;			// line_height == character_height + leading_height
	UUtInt16				leading_width;			// amount to inset from left edge of rectangle

	TStGlyphArray			*glyph_arrays[256];		// array of glyph arrays

	tm_varindex UUtUns32	num_longs;				// number of UUtUns32s in the longs[1]
	tm_vararray UUtUns32	longs[1];				// glyph intensity 8 data

} TStFont;

extern TMtPrivateData		*TSgTemplate_Font_PrivateData;

// ----------------------------------------------------------------------
#define TScTemplate_FontLanguage		UUm4CharToUns32('T', 'S', 'F', 'L')

typedef tm_template('T', 'S', 'F', 'L', "Font Language")
TStFontLanguage
{

	char					breaking_1[64];			// Truncation string
	char					breaking_2[64];			// Characters which cannot be allowed to preceed
													//      a truncation
	char					breaking_3[64];			// Characters which can end words (plus all
													//      double-byte characters, minus breaking_2)
	char					breaking_4[64];			// Characters which cannot end words (plus all
													//      single-byte characters, minus breaking_3)
	char					breaking_5[64];			// Characters which cannot begin words

} TStFontLanguage;

// ----------------------------------------------------------------------
#define TScTemplate_FontFamily		UUm4CharToUns32('T', 'S', 'F', 'F')

typedef tm_template ('T', 'S', 'F', 'F', "Font Family")
TStFontFamily
{
	tm_pad					pad[16];

	TStFontLanguage			*font_language;			// language of the family

	tm_varindex UUtUns32	num_fonts;				// the number of fonts in the language
	tm_vararray TStFont		*fonts[1];				// the fonts

} TStFontFamily;

// ----------------------------------------------------------------------
typedef tm_struct TStFontInfo
{
	TStFontFamily			*font_family;
	TStFontStyle			font_style;
	UUtUns32				font_shade;
	tm_pad					pad[2];
	UUtUns16				font_size;

} TStFontInfo;

// ======================================================================
// prototypes
// ======================================================================
// ----------------------------------------------------------------------
UUtError
TSrInitialize(
	void);

UUtError
TSrRegisterTemplates(
	void);

void
TSrTerminate(
	void);

UUtError
TSrTextureCache_Initialize(
	void);

// CB: for debugging only
void
TSrFont_DisplayCacheTexture(
	void);

// ----------------------------------------------------------------------
UUtError
TSrContext_New(
	TStFontFamily			*inFontFamily,
	UUtUns16				inSize,
	TStFontStyle			inStyle,
	TStFormat				inFormat,
	UUtBool					inUseFormatting,
	TStTextContext			**outTextContext);

void
TSrContext_Delete(
	TStTextContext			*inTextContext);

UUtError
TSrContext_DrawFormattedText(
	const TStTextContext	*inTextContext,
	TStStringFormat			*inStringFormat,
	UUtInt32				inAlpha,
	const IMtPoint2D		*inOffset,
	const IMtShade			*inOverrideColor);

UUtError
TSrContext_DrawString(
	const TStTextContext	*inTextContext,
	void					*inTexture,
	const char				*inString,
	const UUtRect			*inBounds,
	const IMtPoint2D		*inDestination);

UUtError
TSrContext_DrawText(
	const TStTextContext	*inTextContext,
	const char				*inString,
	UUtInt32				inAlpha,
	const UUtRect			*inBounds,
	const IMtPoint2D		*inDestination);


struct DCtDrawContext;
UUtError
TSrContext_DrawText_DC(
	const TStTextContext	*inTextContext,
	const char				*inString,
	const UUtRect			*inBounds,
	const IMtPoint2D		*inDestination,
	const struct DCtDrawContext	*inDrawContext,
	UUtRect					*outStringRect);

UUtError
TSrContext_FormatString(
	const TStTextContext	*inTextContext,
	const char				*inString,
	const UUtRect 			*inBounds,
	const IMtPoint2D		*inDestination,
	TStStringFormat			*outStringFormat);

TStFont*
TSrContext_GetFont(
	const TStTextContext	*inTextContext,
	TStFontStyle			inStyle);

UUtError
TSrContext_GetStringRect(
	const TStTextContext	*inTextContext,
	char					*inString,
	UUtRect 				*outRect);

UUtError
TSrContext_SetFontFamily(
	TStTextContext		*inTextContext,
	TStFontFamily		*inFontFamily);

UUtError
TSrContext_SetShade(
	TStTextContext		*inTextContext,
	UUtUns32			inShade);

UUtError
TSrContext_SetFormat(
	TStTextContext		*inTextContext,
	TStFormat			inFontFormat);

UUtError
TSrContext_SetFontSize(
	TStTextContext		*ioTextContext,
	UUtUns16			inSize);

UUtError
TSrContext_SetFontStyle(
	TStTextContext		*inTextContext,
	TStFontStyle		inStyle);

UUtError
TSrContext_SetFormatting(
	TStTextContext		*inTextContext,
	UUtBool				inUseFormattingCommand);

// ----------------------------------------------------------------------
UUtBool
TSrString_AppendChar(
	char				*ioString,
	const UUtUns16		inKey,
	const UUtUns16		inMaxCharsInString);

UUtBool
TSrString_DeleteChar(
	char				*ioString,
	const UUtUns16		inDeleteIndex,
	const UUtUns16		inMaxCharsInString);

UUtUns16
TSrString_GetCharacterAtIndex(
	const char			*inString,
	const UUtUns16		inIndex);

UUtUns16
TSrString_GetLength(
	const char			*inString);

UUtBool
TSrString_InsertChar(
	char				*ioString,
	UUtUns16			inInsertBefore,
	const UUtUns16		inChar,
	const UUtUns16		inMaxCharsInString);

// ----------------------------------------------------------------------
UUtError
TSrFontFamily_Get(
	char				*inFontFamilyName,
	TStFontFamily		**outFontFamily);

TStFont*
TSrFontFamily_GetFont(
	TStFontFamily		*inFontFamily,
	UUtUns16			inSize,
	TStFontStyle		inStyle);

// ----------------------------------------------------------------------
TStFont*
TSrFont_Get(
	const char			*inName,
	UUtUns16			inSize,
	TStFontStyle		inStyle);

UUtUns16
TSrFont_GetAscendingHeight(
	TStFont				*inFont);

void
TSrFont_GetCharacterSize(
	TStFont				*inFont,
	UUtUns16			inCharacter,
	UUtUns16			*outWidth,
	UUtUns16			*outHeight);

UUtUns16
TSrFont_GetLeadingHeight(
	TStFont				*inFont);

UUtUns16
TSrFont_GetLineHeight(
	TStFont				*inFont);

UUtUns16
TSrFont_GetDescendingHeight(
	TStFont				*inFont);

// ======================================================================
#endif /* BFW_TEXTSYSTEM_H */
