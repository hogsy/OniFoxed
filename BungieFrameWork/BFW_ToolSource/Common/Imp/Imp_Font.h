#ifndef FONTEXTRACTOR_H
#define FONTEXTRACTOR_H

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_Group.h"

// ======================================================================
// prototype
// ======================================================================
UUtError
Imp_AddFontFamily(
	BFtFileRef*			inSourceFileRef,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddLanguage(
	BFtFileRef*			inSourceFileRef,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

// ======================================================================
// from Myth headers.  Most are from Bitmap_Text_Private.h, but some
// come from elsewhere
// ======================================================================

typedef unsigned long file_tag;
typedef unsigned short word;
typedef unsigned char u_byte;

#define HIGH_BYTE(a)	((a & 0xFF00) >> 8)
#define LOW_BYTE(a)		(a & 0x00FF)

#define NONE			0xFFFFFFFF

#define TAG_NAME_LENGTH 31

#define SIZEOF_STRUCT_TAG_HEADER 64
typedef struct tag_header // 64 bytes
{
	char name[TAG_NAME_LENGTH+1];

	file_tag group, subgroup;
	long version;

	// unsigned long flags...
	word tag_version;
	word flags;

	long offset, size;

	unsigned long user_data;

	file_tag tag;
} tag_header;

enum // styles
{
	_font_style_italic= 0,
	_font_style_bold,
	_font_style_condensed,
	_font_style_underlined,
	NUMBER_OF_FONT_STYLES
};

/* ---------- constants */

enum
{
	FONT_DEFINITION_VERSION_NUMBER= 1,
	MAXIMUM_FONT_DEFINITIONS_PER_MAP= 16,

	NUMBER_OF_LEAD_BYTE_CHARACTER_TABLES= 256,
	NUMBER_OF_TRAILING_CHARACTERS_PER_TABLE= 256
};

/* ---------- structures */

#define SIZEOF_STRUCT_FONT_DEFINITION 64
typedef struct font_definition
{
	unsigned long flags;

	short ascending_height, descending_height; // character_height == ascending_height + descending_height
	short leading_height; // line_height == character_height + leading_height
	short leading_width; // amount to inset from left edge of rectangle

	int number_of_characters, characters_offset, characters_size;

	file_tag style_font_tags[NUMBER_OF_FONT_STYLES];

	short unused[2];

	struct bitmap_data *hardware_bitmap;

	short style_font_indexes[NUMBER_OF_FONT_STYLES];

	struct font_character *characters; // memory-based
	struct font_character ***lead_byte_character_tables; // memory-based
} font_definition;

#define SIZEOF_STRUCT_FONT_CHARACTER 16
typedef struct font_character
{
	word character; // character code

	short character_width; // the positive horizontal pen offset in pixels after drawing this character

	short bitmap_width, bitmap_height; // of bitmap
	short bitmap_origin_x, bitmap_origin_y; // in bitmap coordinates, can be outside of bitmap

	short hardware_texture_origin_x, hardware_texture_origin_y;

	// bitmap_width*bitmap_height bytes of 8-bit bitmap data follows
	// zero is transparent, 255 is opaque
	u_byte pixels[];
} font_character;

// ======================================================================
#endif /* FONTEXTRACTOR_H */
