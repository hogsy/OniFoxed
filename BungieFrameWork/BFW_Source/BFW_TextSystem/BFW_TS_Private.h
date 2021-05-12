#ifndef BFW_TEXTSYSTEM_PRIVATE_H
#define BFW_TEXTSYSTEM_PRIVATE_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_TextSystem.h"
#include "BFW_Image.h"

// ======================================================================
// defines
// ======================================================================
#define HIGH_BYTE(a)	((a & 0xFF00) >> 8)
#define LOW_BYTE(a)		(a & 0x00FF)

#define TScFormatCharacter_Begin		'['
#define TScFormatCharacter_End			']'
#define TScFormatCharacter_FormatEnd	'.'

// ======================================================================
// typedefs
// ======================================================================

// ----------------------------------------------------------------------
struct TStTextContext
{
	TStFontFamily	*font_family;				// font family in use in this context
	TStFont			*font;						// font currently being used
	TStFormat		format;						// format to use when drawing text
	UUtUns32		shade;						// shade to draw the glyphs in

	UUtUns16		font_size;					// desired font size
	TStFontStyle	font_style;					// desired font style
	
	UUtBool			use_formatting_commands;
};

// ----------------------------------------------------------------------
typedef struct TStFormatCharacterColor
{
	UUtUns16		character;
	IMtShade		shade;
} TStFormatCharacterColor;

// ======================================================================
// globals
// ======================================================================

// ----------------------------------------------------------------------
extern TStFormatCharacterColor TSgColorFormattingCharacters[];

// ======================================================================
#endif /* BFW_TEXTSYSTEM_PRIVATE_H */