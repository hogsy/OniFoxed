// ======================================================================
// FontExtractor.c
// ======================================================================


// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_TextSystem.h"
#include "BFW_Group.h"
#include "BFW_TM_Construction.h"
#include "BFW_AppUtilities.h"

#include "Imp_Common.h"
#include "Imp_Font.h"

#include <string.h>

char*	gFontCompileTime = __DATE__" "__TIME__;

static int sizeof_font_character(
	struct font_character *character)
{
	int bitmap_size = character->bitmap_width * character->bitmap_height;
	
/*	vassert(character->bitmap_width>=0 && character->bitmap_height>=0 &&
		character->bitmap_width<KILO && character->bitmap_height<KILO,
			csprintf(temporary, "character @%p has bad dimensions (#%dx#%d)", character, character->bitmap_width, character->bitmap_height));
*/	
	return sizeof(struct font_character) + bitmap_size + ((bitmap_size & 1) ? 1 : 0);
}

// ======================================================================
// globals
// ======================================================================
static AUtFlagElement	IMPgStyleFlags[] =
{
	{ "plain",						TScStyle_Plain },
	{ "bold",						TScStyle_Bold },
	{ "italic",						TScStyle_Italic }
};

// ======================================================================
// functions
// ======================================================================

static UUtUns32
iComputeNumLongsForCharacters(
	font_character*	inCharacters,
	UUtUns32		inNumCharacters)
{
	UUtUns32	i;
	UUtUns32	num_longs, num_bytes;
	UUtUns16	bitmap_width;
	UUtUns16	bitmap_height;
	font_character*	character = inCharacters;
	UUtInt32 bitmap_size = character->bitmap_width * character->bitmap_height;
	
	num_longs = 0;
	
	// go through all the characters and copy the data into a glyph
	for (i = 0; i < inNumCharacters; ++i)
	{
		bitmap_width = character->bitmap_width;
		bitmap_height = character->bitmap_height;
		
		#if UUmEndian == UUmEndian_Little
			UUrSwap_2Byte(&bitmap_width);
			UUrSwap_2Byte(&bitmap_height);
			
		#endif
		
		bitmap_size = bitmap_width * bitmap_height;
		bitmap_size = sizeof(struct font_character) + bitmap_size + ((bitmap_size & 1) ? 1 : 0);
		
		// calculate the number of bytes in the character->pixels[]
		num_bytes = (bitmap_width * bitmap_height) + 3;
		
		// calculate the number of longs that will be used by those pixels
		num_longs += (num_bytes >> 2);

		// go to next character
		(u_byte *)character += bitmap_size;
	}
	
	return num_longs;
}

// ======================================================================
// ----------------------------------------------------------------------
static void
iBitmapIsDuplicate(
	font_character		*inCharacterList,
	UUtUns16			inNumberOfCharacters,
	font_character		*inCharacterToTest,
	font_character		**outDuplicateCharacter)
{
	font_character		*current_character;
	UUtUns16			character_to_test_bitmap_size;
	UUtUns16			i;
	
	// clear the duplicate character pointers
	*outDuplicateCharacter = NULL;
	
	// get a pointer to the current character
	current_character = inCharacterList;
	
	// calculate the current characters bitmap size
	character_to_test_bitmap_size =
		inCharacterToTest->bitmap_width *
		inCharacterToTest->bitmap_height;

	// go through the font looking for characters with duplicate bitmaps
	for (i = 0; i < inNumberOfCharacters; i++)
	{
		UUtUns16		current_character_bitmap_size;
		
		// calculate the current characters bitmap size
		current_character_bitmap_size =
			current_character->bitmap_width *
			current_character->bitmap_height;
			
		// do the characters have the same bitmap?
		if ((current_character->character_width == inCharacterToTest->character_width) &&
			(current_character_bitmap_size == character_to_test_bitmap_size))
		{
			UUtUns16		result;
	
			// check the pixels of the two characters
			result =
				memcmp(
					current_character->pixels,
					inCharacterToTest->pixels,
					current_character_bitmap_size);
			if (result == 0)
			{
				*outDuplicateCharacter = current_character;
				return;
			}
		}
				
		// go to next character
		(u_byte *)current_character += sizeof_font_character(current_character);
	}
}

// ----------------------------------------------------------------------
static UUtError
iProcessFont(
	BFtFileRef*			inFileRef,
	TStFont*			*outFont)
{
	UUtError			error;

	UUtUns32			file_length;

	UUtUns8				*data;
	tag_header			*header;
	font_definition		*definition;
	UUtUns8				*characters;

	UUtUns8				glyph_array_index;
	UUtUns8				glyph_index;
	TStFont				*font;
	TStGlyph			*glyph;
	
	font_character		*character;
	UUtInt32			i;
	UUtUns32			j;
	UUtUns32			num_bytes, num_longs;

	UUtUns8				*dst;
	UUtUns8				*src;
	UUtUns16			curGlyphArrayIndex;
	
	// read the font data
	error = BFrFileRef_LoadIntoMemory(
		inFileRef,
		&file_length,
		&data);
	IMPmError_ReturnOnError(error);
	
	// go to specific location in the data
	header = (tag_header*)data;
	
	#if UUmEndian == UUmEndian_Little
		UUrSwap_4Byte(&header->group);
		UUrSwap_4Byte(&header->subgroup);
		UUrSwap_4Byte(&header->version);
		UUrSwap_2Byte(&header->tag_version);
		UUrSwap_2Byte(&header->flags);
		UUrSwap_4Byte(&header->offset);
		UUrSwap_4Byte(&header->size);
		UUrSwap_4Byte(&header->user_data);
		UUrSwap_4Byte(&header->tag);
	#endif
	
	definition = (font_definition*)(data + SIZEOF_STRUCT_TAG_HEADER);	

	#if UUmEndian == UUmEndian_Little
		UUrSwap_4Byte(&definition->flags);
		UUrSwap_2Byte(&definition->ascending_height);
		UUrSwap_2Byte(&definition->descending_height);
		UUrSwap_2Byte(&definition->leading_height);
		UUrSwap_4Byte(&definition->number_of_characters);
	#endif

	characters = data + SIZEOF_STRUCT_TAG_HEADER + SIZEOF_STRUCT_FONT_DEFINITION;
	
	// create an instance of the font
	error = TMrConstruction_Instance_NewUnique(
				TScTemplate_Font,
				iComputeNumLongsForCharacters((font_character*)characters, definition->number_of_characters),
				outFont);
	IMPmError_ReturnOnError(error);
	
	font = *outFont;
	
	for(curGlyphArrayIndex = 0; curGlyphArrayIndex < 256; curGlyphArrayIndex++)
	{
		font->glyph_arrays[curGlyphArrayIndex] = NULL;
	}
	
	// set the font characteristics
	font->ascending_height = definition->ascending_height;
	font->descending_height = definition->descending_height;
	font->leading_height = definition->leading_height;
	font->leading_width =
		(definition->ascending_height + definition->descending_height) / 5;

	character = (font_character*)characters;
	if (character)
	{
		num_bytes = 0;
		num_longs = 0;
		
		// go through all the characters and copy the data into a glyph
		for (i = 0; i < definition->number_of_characters; ++i)
		{
			#if UUmEndian == UUmEndian_Little
				UUrSwap_2Byte(&character->character);
				UUrSwap_2Byte(&character->character_width);
				UUrSwap_2Byte(&character->bitmap_width);
				UUrSwap_2Byte(&character->bitmap_height);
				UUrSwap_2Byte(&character->bitmap_origin_x);
				UUrSwap_2Byte(&character->bitmap_origin_y);
			#endif
			
			glyph_array_index = HIGH_BYTE(character->character);
			
			// allocate space for glyphArrays[glyph_array_index] if it is needed
			if (font->glyph_arrays[glyph_array_index] == NULL)
			{
				error =
					TMrConstruction_Instance_NewUnique(
						TScTemplate_GlyphArray,
						0,
						&font->glyph_arrays[glyph_array_index]);
				IMPmError_ReturnOnError(error);

				UUrMemory_Clear(font->glyph_arrays[glyph_array_index], sizeof(TStGlyphArray));
			}
			
			if (font->glyph_arrays[glyph_array_index])
			{
				// get a pointer to the glyph being modified
				glyph_index = LOW_BYTE(character->character);
				glyph = &font->glyph_arrays[glyph_array_index]->glyphs[glyph_index];
				
				// if the glyph hasn't already been set, copy the character data into it
				if (glyph->code != character->character)
				{
					glyph->code			= character->character;
					glyph->kerning		= character->character_width;
					glyph->width		= character->bitmap_width;
					glyph->height		= character->bitmap_height;
					glyph->origin_x		= character->bitmap_origin_x;
					glyph->origin_y		= character->bitmap_origin_y;
					glyph->longs_offset	= 0;
					glyph->cell			= 0;
				}
			}
						
			// go to next character
			(u_byte *)character += sizeof_font_character(character);
		}

		// copy the character->pixels[] to font->longs[]
		character = (font_character*)characters;
		for (i = 0; i < definition->number_of_characters; ++i)
		{
			font_character	*duplicate_character;
			
			// get a pointer to the glyph being modified
			glyph_array_index = HIGH_BYTE(character->character);
			glyph_index = LOW_BYTE(character->character);
			glyph = &font->glyph_arrays[glyph_array_index]->glyphs[glyph_index];
			
			if (glyph->longs_offset != 0)
			{
				// go to next character
				(u_byte *)character += sizeof_font_character(character);
				continue;
			}
			
			// calculate the number of bytes to copy
			num_bytes = character->bitmap_width * character->bitmap_height;
			
			// look for duplicate character bitmaps
			iBitmapIsDuplicate(
				(font_character*)characters,
				(UUtUns16)i,
				character,
				&duplicate_character);
			if (duplicate_character)
			{
				// the character bitmap is a duplicate
				UUtUns16	similar_glyph_array_index;
				UUtUns16	similar_glyph_index;
				TStGlyph	*similar_glyph;
				
				// get a pointer to the glyph with the same bitmap
				similar_glyph_array_index = HIGH_BYTE(duplicate_character->character);
				similar_glyph_index = LOW_BYTE(duplicate_character->character);
				similar_glyph = &font->glyph_arrays[similar_glyph_array_index]->
									glyphs[similar_glyph_index];
				
				// set the longs_offset of the glyph
				glyph->longs_offset = similar_glyph->longs_offset;
			}
			else
			{
				// set the longs_offset of the glyph
				glyph->longs_offset = num_longs;
			
				// calculate the number of longs that will be used by those pixels
				num_longs += (num_bytes + 3) >> 2;
				
				// get a pointer to the font->longs[] destination
				dst = (UUtUns8*)(&font->longs[glyph->longs_offset]);
				src = character->pixels;
				
				// when copying the bitmap, switch the byte to the form we want 
				for (j = 0; j < num_bytes; j++)
				{
					*dst++ = *src++;
				}
			}
			
			// go to next character
			(u_byte *)character += sizeof_font_character(character);
		}
	}
	
	UUrMemory_Block_Delete(data);
	
	return UUcError_None;
}

// ======================================================================
static UUtError
iFindStyleInList(
	GRtElementArray*	inStyleList,
	TStFontStyle		inStyleIndex,
	GRtGroup*			*outStyleGroup)
{
	UUtError		error;
	UUtUns32		numStyles;
	UUtUns32		i;
	
	GRtElementType	elementType;
	GRtGroup*		styleGroup;
	TStFontStyle	curStyle;
	
	char*			styleStr;
	
	numStyles = GRrGroup_Array_GetLength(inStyleList);
	
	for(i = 0; i < numStyles; i++)
	{
		error = 
			GRrGroup_Array_GetElement(
				inStyleList,
				i,
				&elementType,
				&styleGroup);
		if(error != UUcError_None || elementType != GRcElementType_Group)
		{
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Could not get style group");
		}
		
		error = 
			GRrGroup_GetElement(
				styleGroup,
				"style",
				&elementType,
				&styleStr);
		if(error != UUcError_None || elementType != GRcElementType_String)
		{
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Could not get style string");
		}

		if(!strcmp(styleStr, "plain"))
		{
			curStyle = TScStyle_Plain;
		}
		else if(!strcmp(styleStr, "bold"))
		{
			curStyle = TScStyle_Bold;
		}
		else if(!strcmp(styleStr, "italic"))
		{
			curStyle = TScStyle_Italic;
		}
		else
		{
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Unknown style string");
		}
		
		if(inStyleIndex == curStyle)
		{
			*outStyleGroup = styleGroup;
			return UUcError_None;
		}
	}

	return UUcError_Generic;
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks 
#pragma mark -
#endif
// ----------------------------------------------------------------------
UUtError
Imp_AddFontFamily(
	BFtFileRef*			inSourceFileRef,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;
	UUtBool				buildInstance;
		
	buildInstance = !TMrConstruction_Instance_CheckExists(TScTemplate_FontFamily, inInstanceName);

	if(buildInstance == UUcTrue)
	{
		TStFontFamily		*font_family;
		GRtElementType		elementType;
		GRtElementArray		*font_array;
		UUtUns32			num_fonts;
		UUtUns32			i;
		BFtFileRef*			fileRef;
		char				*style_string;
		GRtGroup			*font_group;
		char				*languageStr;
		
		// get a pointer to the font array
		error = 
			GRrGroup_GetElement(
				inGroup,
				"fonts",
				&elementType,
				&font_array);
		if(error != UUcError_None || elementType != GRcElementType_Array)
		{
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Could not get font list");
		}
		
		num_fonts = GRrGroup_Array_GetLength(font_array);
		
		// create the font family
		error =
			TMrConstruction_Instance_Renew(
				TScTemplate_FontFamily,
				inInstanceName,
				num_fonts,
				&font_family);
		IMPmError_ReturnOnError(error);
		
		// process the font array
		for (i = 0; i < num_fonts; i++)
		{
			// get the group
			error =
				GRrGroup_Array_GetElement(
					font_array,
					i,
					&elementType,
					&font_group);
			if ((error != UUcError_None) || (elementType != GRcElementType_Group))
			{
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Cound not get font_group from font_array");
			}
			
			// get the file ref of the font
			error = 
				Imp_Common_GetFileRefAndModDate(
					inSourceFileRef,
					font_group,
					"file",
					&fileRef);
			IMPmError_ReturnOnError(error);
			
			// process the font
			error = iProcessFont(fileRef, &font_family->fonts[i]);
			IMPmError_ReturnOnErrorMsg(error, "Error processing the font data");
			
			BFrFileRef_Dispose(fileRef);
			fileRef = NULL;
			
			// set the size and style of the font
			error = GRrGroup_GetUns16(font_group, "size", &font_family->fonts[i]->font_size);
			IMPmError_ReturnOnErrorMsg(error, "Unable to get the font size");

			error = GRrGroup_GetString(font_group, "style", &style_string);
			IMPmError_ReturnOnErrorMsg(error, "Unable to get the font style string");
			
			error =
				AUrFlags_ParseFromGroupArray(
					IMPgStyleFlags,
					GRcElementType_String,
					style_string,
					(UUtUns32*)&font_family->fonts[i]->font_style);
			IMPmError_ReturnOnErrorMsg(error, "Unable to process flags");
		}
		
		// process the language
		error =
			GRrGroup_GetElement(
				inGroup,
				"language",
				&elementType,
				&languageStr);
		if ((error != UUcError_None) || (elementType != GRcElementType_String))
		{
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Could not get language string");
		}
		
		error = 
			TMrConstruction_Instance_GetPlaceHolder(
				TScTemplate_FontLanguage,
				languageStr,
				(TMtPlaceHolder*)&font_family->font_language);
		IMPmError_ReturnOnErrorMsg(error, "Could not find font language");
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
Imp_AddLanguage(
	BFtFileRef*			inSourceFileRef,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;
	TStFontLanguage		*font_language;
	
	GRtElementType		elementType;
	char*				breaking_1;
	char*				breaking_2;
	char*				breaking_3;
	char*				breaking_4;
	char*				breaking_5;
	
	UUtBool				buildInstance;
	
	buildInstance = !TMrConstruction_Instance_CheckExists(TScTemplate_FontLanguage,	inInstanceName);

	if(buildInstance == UUcTrue)
	{
		error =
			GRrGroup_GetElement(
				inGroup,
				"breaking1",
				&elementType,
				&breaking_1);
		if(error != UUcError_None || elementType != GRcElementType_String)
		{
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Could not get breaking1");
		}
		
		error =
			GRrGroup_GetElement(
				inGroup,
				"breaking2",
				&elementType,
				&breaking_2);
		if(error != UUcError_None || elementType != GRcElementType_String)
		{
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Could not get breaking1");
		}
		
		error =
			GRrGroup_GetElement(
				inGroup,
				"breaking3",
				&elementType,
				&breaking_3);
		if(error != UUcError_None || elementType != GRcElementType_String)
		{
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Could not get breaking1");
		}
		
		error =
			GRrGroup_GetElement(
				inGroup,
				"breaking4",
				&elementType,
				&breaking_4);
		if(error != UUcError_None || elementType != GRcElementType_String)
		{
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Could not get breaking1");
		}
		
		error =
			GRrGroup_GetElement(
				inGroup,
				"breaking5",
				&elementType,
				&breaking_5);
		if(error != UUcError_None || elementType != GRcElementType_String)
		{
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Could not get breaking1");
		}
		
		// create an instance of the font
		error = TMrConstruction_Instance_Renew(
					TScTemplate_FontLanguage,
					inInstanceName,
					0,
					&font_language);
		IMPmError_ReturnOnError(error);
		
		// don't keep the quotes at the beginning and end of the breaking lists
		{
			UUtInt16 		len_breaking_1;
			UUtInt16 		len_breaking_2;
			UUtInt16 		len_breaking_3;
			UUtInt16 		len_breaking_4;
			UUtInt16		len_breaking_5;
			
			breaking_1++;
			breaking_2++;
			breaking_3++;
			breaking_4++;
			breaking_5++;
			
			len_breaking_1 = UUmMax(strlen(breaking_1) - 1, 0);
			len_breaking_2 = UUmMax(strlen(breaking_2) - 1, 0);
			len_breaking_3 = UUmMax(strlen(breaking_3) - 1, 0);
			len_breaking_4 = UUmMax(strlen(breaking_4) - 1, 0);
			len_breaking_5 = UUmMax(strlen(breaking_5) - 1, 0);
			
			breaking_1[len_breaking_1] = '\0';
			breaking_2[len_breaking_2] = '\0';
			breaking_3[len_breaking_3] = '\0';
			breaking_4[len_breaking_4] = '\0';
			breaking_5[len_breaking_5] = '\0';
		}
		
		// set the font languages's characteristics
		UUrString_Copy(font_language->breaking_1, breaking_1, 64);
		UUrString_Copy(font_language->breaking_2, breaking_2, 64);
		UUrString_Copy(font_language->breaking_3, breaking_3, 64);
		UUrString_Copy(font_language->breaking_4, breaking_4, 64);
		UUrString_Copy(font_language->breaking_5, breaking_5, 64);
	}
	
	return UUcError_None;
}

