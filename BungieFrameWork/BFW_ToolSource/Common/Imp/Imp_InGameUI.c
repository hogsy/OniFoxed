// ======================================================================
// Imp_InGameUI.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_TM_Construction.h"
#include "BFW_Group.h"
#include "BFW_AppUtilities.h"
#include "WM_PartSpecification.h"

#include "Oni_InGameUI.h"

#include "Imp_Common.h"
#include "Imp_InGameUI.h"
#include "Imp_Texture.h"
#include "Imp_DialogData.h"

#include <ctype.h>

// ======================================================================
// globals
// ======================================================================
static char*	gInGameUICompileTime = __DATE__" "__TIME__;

static AUtFlagElement	IMPgMoveKeys[] =
{
	{ "p",							ONcKey_Punch },
	{ "punch",						ONcKey_Punch },
	{ "k",							ONcKey_Kick },
	{ "kick",						ONcKey_Kick },
	{ "f",							ONcKey_Forward },
	{ "forward",					ONcKey_Forward },
	{ "b",							ONcKey_Backward },
	{ "backward",					ONcKey_Backward },
	{ "l",							ONcKey_Left },
	{ "left",						ONcKey_Left },
	{ "r",							ONcKey_Right },
	{ "right",						ONcKey_Right },
	{ "c",							ONcKey_Crouch },
	{ "crouch",						ONcKey_Crouch },
	{ "j",							ONcKey_Jump },
	{ "jump",						ONcKey_Jump },
	{ "+",							ONcKey_Plus },
	{ "plus",						ONcKey_Plus },
	{ "h",							ONcKey_Hold },
	{ "hold",						ONcKey_Hold },
	{ "q",							ONcKey_Quick },
	{ "quick",						ONcKey_Quick },
	{ "w",							ONcKey_Wait },
	{ "wait",						ONcKey_Wait },
	
	{ NULL,							0 }
};

static AUtFlagElement	IMPgPowerupTypes[] =
{
	{ "ammo_ballistic",				WPcPowerup_AmmoBallistic },
	{ "ammo_energy",				WPcPowerup_AmmoEnergy },
	{ "hypo",						WPcPowerup_Hypo },
	{ "shield",						WPcPowerup_ShieldBelt },
	{ "invis",						WPcPowerup_Invisibility },
	{ "lsi",						WPcPowerup_LSI },
	
	{ NULL,							WPcPowerup_None }
};

#define ONcIGUI_ElemName_MaxChars		32
#define ONcIGUI_HUDHelpText_MaxItems	32

// ======================================================================
// typedefs
// ======================================================================
typedef struct ONtIGUI_StringListElem
{
	char							id[ONcIGUI_ElemName_MaxChars];
	char							*text;
	
} ONtIGUI_StringListElem;

typedef struct ONtIGUI_StringList
{
	UUtUns32						num_lines;
	ONtIGUI_StringListElem			lines[1024];
	
} ONtIGUI_StringList;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static ONtIGUI_StringList*
IGUIiStringList_Get(
	BFtFileRef			*inSourceFile)
{
	UUtError			error;
	BFtTextFile			*text_file;
	ONtIGUI_StringList	*string_list;
	
	text_file = NULL;
	string_list = NULL;
	
	error = BFrTextFile_MapForRead(inSourceFile, &text_file);
	if (error != UUcError_None) { return NULL; }
	
	// find the "#strings" line
	do
	{
		char				*start;
		
		start = BFrTextFile_GetNextStr(text_file);
		if (start == NULL) { goto exit; }
		if (UUrString_CompareLen_NoCase(start, "#strings", strlen("#strings")) == 0)
		{
			break;
		}
	}
	while (1);
	
	// the file has a string list, so allocate an ONtIGUI_StringList
	string_list = (ONtIGUI_StringList*)UUrMemory_Block_NewClear(sizeof(ONtIGUI_StringList));
	string_list->num_lines = 0;
	
	// the following are the actual text
	do
	{
		char					*string;
		char					elem_name[ONcIGUI_ElemName_MaxChars];
		UUtUns32				i;
		UUtUns32				index;
		
		string = BFrTextFile_GetNextStr(text_file);
		if (string == NULL) { break; }
		
		// skip lines that don't begin with #
		if (string[0] != '#') { continue; }
		
		// read the element name
		UUrMemory_Clear(elem_name, ONcIGUI_ElemName_MaxChars);
		for (i = 0; i < ONcIGUI_ElemName_MaxChars; i++)
		{
			if (UUrIsSpace(string[i + 1])) { break; }
			elem_name[i] = string[i + 1];
		}
		
		// find the start of the string
		do
		{
			char					c;
			c = *string;
			if ((c == 0) || (c == '"')) { break; }
			string++;
		}
		while (1);
		
		// create the ONtIGUI_StringListElem
		index = string_list->num_lines;
		UUrString_Copy(
			string_list->lines[index].id,
			elem_name,
			ONcIGUI_ElemName_MaxChars);
		
		string_list->lines[index].text = (char*)UUrMemory_Block_New((strlen(string) + 1));
		if (string_list->lines[index].text != NULL)
		{
			UUrString_Copy(string_list->lines[index].text, string, (strlen(string) + 1));
		}
		
		string_list->num_lines++;
	}
	while (1);
	
exit:
	if (text_file)
	{
		BFrTextFile_Close(text_file);
	}
	
	return string_list;
}

// ----------------------------------------------------------------------
static char*
IGUIiStringList_GetText(
	ONtIGUI_StringList	*inStringList,
	const char			*inElemName)
{
	UUtUns32			i;
	char				*text;
	
	if ((inStringList == NULL) || (inElemName == NULL)) { return NULL; }
	
	text = NULL;
	
	for (i = 0; i < inStringList->num_lines; i++)
	{
		UUtInt32			result;
		
		result =
			UUrString_Compare_NoCase(
				inStringList->lines[i].id,
				inElemName);
				
		if (result != 0) { continue; }
		
		text = inStringList->lines[i].text;
		break;
	}
	
	return text;
}

// ----------------------------------------------------------------------
static void
IGUIiStringList_Delete(
	ONtIGUI_StringList	*inStringList)
{
	UUtUns32			i;
	
	if (inStringList == NULL) { return; }
	
	for (i = 0; i < inStringList->num_lines; i++)
	{
		if (inStringList->lines[i].text != NULL)
		{
			UUrMemory_Block_Delete(inStringList->lines[i].text);
			inStringList->lines[i].text = NULL;
		}
	}
	
	UUrMemory_Block_Delete(inStringList);
	inStringList = NULL;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
iGetTextureByPath(
	const char			*inPath,
	BFtFileRef			*inParentDir,
	TMtPlaceHolder		*outTextureMap)
{
	UUtError			error;
	BFtFileRef			*texture_file_ref;
	const char			*texture_name;
	TMtPlaceHolder		texture_map;
	
	UUmAssert(inPath);
	UUmAssert(inParentDir);
	UUmAssert(outTextureMap);
	
	// create a new file ref using the file name
	error =
		BFrFileRef_DuplicateAndReplaceName(
			inParentDir,
			inPath,
			&texture_file_ref);
	IMPmError_ReturnOnErrorMsg(error, "texture file was not found");
	
	// set the textures name
	texture_name = BFrFileRef_GetLeafName(texture_file_ref);
	
	// process the texture map file
	error =
		Imp_ProcessTexture_File(
			texture_file_ref,
			texture_name,
			&texture_map);
	IMPmError_ReturnOnErrorMsg(error, "unable to process the texture");
	
	BFrFileRef_Dispose(texture_file_ref);
	
	*outTextureMap = texture_map;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
iGetTextureBigByPath(
	const char			*inPath,
	BFtFileRef			*inParentDir,
	TMtPlaceHolder		*outTextureMap)
{
	UUtError			error;
	BFtFileRef			*texture_file_ref;
	const char			*texture_name;
	TMtPlaceHolder		texture_map;
	
	UUmAssert(inPath);
	UUmAssert(inParentDir);
	UUmAssert(outTextureMap);
	
	// create a new file ref using the file name
	error =
		BFrFileRef_DuplicateAndReplaceName(
			inParentDir,
			inPath,
			&texture_file_ref);
	IMPmError_ReturnOnErrorMsg(error, "texture file was not found");
	
	// set the textures name
	texture_name = BFrFileRef_GetLeafName(texture_file_ref);
	
	// process the texture map file
	error =
		Imp_ProcessTexture_Big_File(
			texture_file_ref,
			texture_name,
			&texture_map);
	IMPmError_ReturnOnErrorMsg(error, "unable to process the texture");
	
	BFrFileRef_Dispose(texture_file_ref);
	
	*outTextureMap = texture_map;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
Imp_IGUI_FontInfo(
	GRtGroup				*inGroup,
	ONtIGUI_FontInfo		*outFontInfo)
{
	UUtError				error;
	GRtElementType			element_type;
	void					*data;

	char					*font_name;
	void					*font_size_data;
	UUtUns32				font_size;
	void					*font_style_data;
	UUtUns32				font_style;
	void					*font_shade_data;
	UUtUns32				font_shade;

	// initialize the font info
	UUrMemory_Clear(outFontInfo, sizeof(ONtIGUI_FontInfo));
	outFontInfo->flags = IGUIcFontInfoFlag_None;
	
	if (inGroup == NULL) { return; }
	
	// get the font_info from the group
	error =
		GRrGroup_GetElement(
			inGroup,
			"font_info",
			&element_type,
			&data);
	if (error != UUcError_None)
	{
		return;
	}
	else if (element_type == GRcElementType_Group)
	{
		// get the font family name
		error = GRrGroup_GetString(data, "font", &font_name);
		if (error == UUcError_None)
		{
			outFontInfo->flags |= IGUIcFontInfoFlag_FontFamily;
		}
		
		// get teh font size
		error = GRrGroup_GetUns32(data, "font_size", &font_size);
		if (error == UUcError_None)
		{
			outFontInfo->flags |= IGUIcFontInfoFlag_FontSize;
		}
		
		// get the font style
		error = GRrGroup_GetElement(data, "font_style", &element_type, &font_style_data);
		if (error == UUcError_None)
		{
			error =
				AUrFlags_ParseFromGroupArray(
					IMPgFontStyleTypes,
					element_type,
					font_style_data,
					&font_style);
			if (error == UUcError_None)
			{
				outFontInfo->flags |= IGUIcFontInfoFlag_FontStyle;
			}
		}
		
		// get the font shade
		error = GRrGroup_GetElement(data, "font_shade", &element_type, &font_shade_data);
		if (error == UUcError_None)
		{
			error =
				AUrFlags_ParseFromGroupArray(
					IMPgFontShadeTypes,
					element_type,
					font_shade_data,
					&font_shade);
			if (error == UUcError_None)
			{
				outFontInfo->flags |= IGUIcFontInfoFlag_FontShade;
			}
			else
			{
				// try processing the data as a number
				error = GRrGroup_GetUns32(data, "font_shade", &font_shade);
				if (error == UUcError_None)
				{
					outFontInfo->flags |= IGUIcFontInfoFlag_FontShade;
				}
			}
		}
	}
	else if (element_type == GRcElementType_Array)
	{
		// get the font family name
		error = GRrGroup_Array_GetElement(data, cInfoFont, &element_type, &font_name);
		if (error == UUcError_None)
		{
			outFontInfo->flags |= IGUIcFontInfoFlag_FontFamily;
		}
		
		// get teh font size
		error = GRrGroup_Array_GetElement(data, cInfoSize, &element_type, &font_size_data);
		if (error == UUcError_None)
		{
			outFontInfo->flags |= IGUIcFontInfoFlag_FontSize;
		}
		
		sscanf((char*)font_size_data, "%d", &font_size);
		
		// get the font style
		error = GRrGroup_Array_GetElement(data, cInfoStyle, &element_type, &font_style_data);
		if (error == UUcError_None)
		{
			error =
				AUrFlags_ParseFromGroupArray(
					IMPgFontStyleTypes,
					element_type,
					font_style_data,
					&font_style);
			if (error == UUcError_None)
			{
				outFontInfo->flags |= IGUIcFontInfoFlag_FontStyle;
			}
		}
		
		// get the font shade
		error = GRrGroup_Array_GetElement(data, cInfoShade, &element_type, &font_shade_data);
		if (error == UUcError_None)
		{
			error =
				AUrFlags_ParseFromGroupArray(
					IMPgFontShadeTypes,
					element_type,
					font_shade_data,
					&font_shade);
			if (error == UUcError_None)
			{
				outFontInfo->flags |= IGUIcFontInfoFlag_FontShade;
			}
			else
			{
				// try processing the data as a number
				error = GRrGroup_GetUns32(data, "font_shade", &font_shade);
				if (error == UUcError_None)
				{
					outFontInfo->flags |= IGUIcFontInfoFlag_FontShade;
				}
			}
		}
	}
	
	// setup the outFontInfo data
	if ((outFontInfo->flags & IGUIcFontInfoFlag_FontFamily) != 0)
	{
		// get a placeholder for the font
		error = 
			TMrConstruction_Instance_GetPlaceHolder(
				TScTemplate_FontFamily,
				font_name,
				(TMtPlaceHolder*)&outFontInfo->font_family);
		if (error != UUcError_None)
		{
			outFontInfo->flags &= ~IGUIcFontInfoFlag_FontFamily;
		}
	}

	if ((outFontInfo->flags & IGUIcFontInfoFlag_FontSize) != 0)
	{
		outFontInfo->font_size = (UUtUns16)font_size;
	}
	
	if ((outFontInfo->flags & IGUIcFontInfoFlag_FontStyle) != 0)
	{
		outFontInfo->font_style = (TStFontStyle)font_style;
	}

	if ((outFontInfo->flags & IGUIcFontInfoFlag_FontShade) != 0)
	{
		outFontInfo->font_shade = font_shade;
	}
}

// ----------------------------------------------------------------------
static ONtIGUI_String*
Imp_IGUI_String(
	void				*inData,
	GRtElementType		inElementType,
	ONtIGUI_StringList	*inStringList)
{
	UUtError			error;
	char				*text;
	char				*elem_name;
	ONtIGUI_String		*string;
	UUtUns32			length;
	UUtBool				no_quotes;
	char				*last_quote;
	
	// create the string
	error =
		TMrConstruction_Instance_NewUnique(
			ONcTemplate_IGUI_String,
			0,
			&string);
	if (error != UUcError_None)
	{
		Imp_PrintWarning("Unable to create string instance.");
		return NULL;
	}
	
	switch (inElementType)
	{
		case GRcElementType_String:
			// get the text
			elem_name = (char*)inData;
			
			// get the font info
			Imp_IGUI_FontInfo(NULL, &string->font_info);
		break;
		
		case GRcElementType_Group:
		{
			GRtGroup				*group;
			
			group = (GRtGroup*)inData;
			
			// get the text
			error = GRrGroup_GetString(group, "text", &elem_name);
			if (error != UUcError_None)
			{
				Imp_PrintWarning("Unable to get text from text_array group.");
				return NULL;
			}
			
			// get the font info
			Imp_IGUI_FontInfo(group, &string->font_info);
		}
		break;
		
		default:
			Imp_PrintWarning("can't process that element type");
			return NULL;
		break;
	}
	
	// get the string from the inStringList
	text = IGUIiStringList_GetText(inStringList, elem_name);
	if (text == NULL)
	{
		Imp_PrintWarning("Unable to get text %s from the string list", elem_name);
		return NULL;
	}
	
	// make sure the strings will have at least one character in it and that it begins
	// with a quotation mark
	no_quotes = UUcFalse;
	length = strlen(text);
	if ((length < 3) || (text[0] != '"'))
	{
		no_quotes = UUcTrue;
	}
	
	if (no_quotes == UUcFalse)
	{
		// find the last quotation mark and make sure there is only white space
		// between it and the end of the test.  There is guaranteed to be at least
		// one quote on the line if this code is reached
		last_quote = strrchr(text, '"');
		last_quote++;
		
		while (1)
		{
			char			c;
			
			c = *last_quote++;
			if (c == 0) { break; }
			if (UUrIsSpace(c) == UUcFalse)
			{
				no_quotes = UUcTrue;
				break;
			}
		}
	}
		
	if (no_quotes == UUcTrue)
	{
		if (length < 3)
		{
			Imp_PrintWarning("A line with only quotes isn't valid");
		}
		else
		{
			Imp_PrintWarning("String must have quotes around it.");
		}
	}
	else
	{
		// terminate the string after the last quote
		last_quote = strrchr(text, '"');
		last_quote[1] = '\0';

		// make sure the text will fit into the string
		if (length > ONcIGUI_MaxStringLength)
		{
			text[ONcIGUI_MaxStringLength - 1] = '\0';
		}
		
		// copy the text into the string
		UUrString_Copy(string->string, (text + 1), ONcIGUI_MaxStringLength);
		
		length = strlen(string->string);
		string->string[length - 1] = '\0';
	}
	
	// return the string
	return string;
}

// ----------------------------------------------------------------------
static ONtIGUI_StringArray*
Imp_IGUI_StringArray(
	GRtGroup			*inGroup,
	const char			*inName,
	ONtIGUI_StringList	*inStringList)
{
	UUtError			error;
	GRtElementArray		*text_array;
	GRtElementType		element_type;
	UUtUns32			num_elements;
	ONtIGUI_StringArray	*string_array;
	UUtUns32			i;
	
	error =
		GRrGroup_GetElement(
			inGroup,
			inName,
			&element_type,
			&text_array);
	if ((error != UUcError_None) || (element_type != GRcElementType_Array))
	{
//		Imp_PrintWarning("Unable to get string array from group");
		return NULL;
	}
	
	num_elements = GRrGroup_Array_GetLength(text_array);
	if (num_elements == 0)
	{
//		Imp_PrintWarning("The string array has no items in it");
		return NULL;
	}
	
	// create the text array
	error =
		TMrConstruction_Instance_NewUnique(
			ONcTemplate_IGUI_StringArray,
			num_elements,
			&string_array);
	if (error != UUcError_None)
	{
		Imp_PrintWarning("Unable to create string array instance.");
		return NULL;
	}
	
	for (i = 0; i < num_elements; i++)
	{
		void				*data;
		
		string_array->string[i] = NULL;
		
		// get the text from the array
		error =
			GRrGroup_Array_GetElement(
				text_array,
				i,
				&element_type,
				&data);
		if (error != UUcError_None)
		{
			Imp_PrintWarning("Unable to get element from text array");
			continue;
		}
		
		// process the string
		string_array->string[i] = Imp_IGUI_String(data, element_type, inStringList);
	}
	
	// return the string array
	return string_array;
}

// ----------------------------------------------------------------------
static ONtIGUI_Page*
Imp_IGUI_Page(
	BFtFileRef			*inSourceFile,
	GRtGroup			*inGroup,
	ONtIGUI_StringList	*inStringList)
{
	UUtError			error;
	TMtPlaceHolder		pict;
	char				*path;
	ONtIGUI_Page		*page;
	char				*part_name;
	TMtPlaceHolder		part;
	
	pict = 0;
	path = NULL;
	page = NULL;
	part = 0;
	
	// create the template instance
	error =
		TMrConstruction_Instance_NewUnique(
			ONcTemplate_IGUI_Page,
			0,
			&page);
	if (error != UUcError_None)
	{
		Imp_PrintWarning("Unable to create the page instance");
		return NULL;
	}
	
	// import the pict
	error = GRrGroup_GetString(inGroup, "pict", &path);
	if (error == UUcError_None)
	{
		error = iGetTextureByPath(path, inSourceFile, &pict);
	}
	
	// import the part
	if (pict == 0)
	{
		error = GRrGroup_GetString(inGroup, "part", &part_name);
		if (error == UUcError_None)
		{
			TMrConstruction_Instance_GetPlaceHolder(
				PScTemplate_PartSpecification,
				part_name,
				&part);
		}
	}
	
	// set the data
	if (pict != 0) { page->pict = (void*)pict; }
	else { page->pict = (void*)part; }
	page->text = Imp_IGUI_StringArray(inGroup, "text", inStringList);
	page->hint = Imp_IGUI_StringArray(inGroup, "hint", inStringList);
	
	// get the font info 
	Imp_IGUI_FontInfo(inGroup, &page->font_info);
	
	return page;
}

// ----------------------------------------------------------------------
static ONtIGUI_PageArray*
Imp_IGUI_PageArray(
	BFtFileRef			*inSourceFile,
	GRtGroup			*inGroup,
	const char			*inName,
	ONtIGUI_StringList	*inStringList)
{
	UUtError			error;
	UUtUns32			num_elements;
	GRtElementType		element_type;
	GRtElementArray		*element_array;
	ONtIGUI_PageArray	*page_array;
	UUtUns32			i;
	
	// get the page_array element array
	error =
		GRrGroup_GetElement(
			inGroup,
			inName,
			&element_type,
			&element_array);
	if ((error != UUcError_None) || (element_type != GRcElementType_Array))
	{
		Imp_PrintWarning("Unable to get page_array from group");
		return NULL;
	}
	
	// get the number of elements
	num_elements = GRrGroup_Array_GetLength(element_array);
	if (num_elements == 0)
	{
		Imp_PrintWarning("The page_array doesn't have any elements in it.");
		return NULL;
	}
	
	// create the template instance
	error = 
		TMrConstruction_Instance_NewUnique(
			ONcTemplate_IGUI_PageArray,
			num_elements,
			&page_array);
	if (error != UUcError_None)
	{
		Imp_PrintWarning("Unable to create the page array instance");
		return NULL;
	}
	
	// process the array elements
	for (i = 0; i < num_elements; i++)
	{
		GRtGroup			*page_group;
		
		error =
			GRrGroup_Array_GetElement(
				element_array,
				i,
				&element_type,
				&page_group);
		if ((error != UUcError_None) || (element_type != GRcElementType_Group))
		{
			Imp_PrintWarning("Unable to process page group in page_array");
			continue;
		}
		
		page_array->pages[i] = Imp_IGUI_Page(inSourceFile, page_group, inStringList);
	}
	
	return page_array;	
}

// ----------------------------------------------------------------------
UUtError
Imp_AddDiaryPage(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;

	UUtBool				build_instance;
	
	
	// check to see if the dialogs need to be built
	build_instance = !TMrConstruction_Instance_CheckExists(ONcTemplate_DiaryPage, inInstanceName);
	
	if (build_instance)
	{
		GRtElementType		element_type;
		UUtInt16			level_number;
		UUtInt16			page_number;
		UUtBool				has_new_move;
		ONtDiaryPage		*diary_page;
		GRtElementArray		*key_array;
		ONtKey				keys[ONcMaxNumKeys];
		UUtUns32			i;
		ONtIGUI_Page		*page;
		ONtIGUI_StringList	*string_list;
		
		// get the level number
		error = GRrGroup_GetInt16(inGroup, "level_number", &level_number);
		IMPmError_ReturnOnErrorMsg(error, "unable to get level number");
		
		// get the page number
		error = GRrGroup_GetInt16(inGroup, "page_number", &page_number);
		IMPmError_ReturnOnErrorMsg(error, "unable to get page number");
		
		// get the page number
		error = GRrGroup_GetBool(inGroup, "has_new_move", &has_new_move);
		if (error != UUcError_None) {
			has_new_move = UUcFalse;
		}
		
		// get the move keys
		error =
			GRrGroup_GetElement(
				inGroup,
				"keys",
				&element_type,
				&key_array);
		if ((error != UUcError_None) || (element_type != GRcElementType_Array))
		{
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "unable to get key array");
		}
		
		UUrMemory_Clear(keys, sizeof(ONtKey) * ONcMaxNumKeys);
		for (i = 0; i < GRrGroup_Array_GetLength(key_array); i++)
		{
			UUtUns32			flag;
			void				*element;
			
			error = GRrGroup_Array_GetElement(key_array, i, &element_type, &element);
			IMPmError_ReturnOnErrorMsg(error, "unable to get element from key array"); 
			
			error =
				AUrFlags_ParseFromGroupArray(
					IMPgMoveKeys,
					element_type,
					element,
					&flag);
			IMPmError_ReturnOnErrorMsg(error, "Unable to process keys");
			
			keys[i] = (ONtKey)flag;
		}
		
		// get the string list
		string_list = IGUIiStringList_Get(inSourceFile);
		if (string_list == NULL)
		{
			Imp_PrintWarning("Unable to get the string list for the Diary Page");
			return UUcError_None;
		}
		
		// get the IGUI page
		page = Imp_IGUI_Page(inSourceFile, inGroup, string_list);
		
		// create the DiaryPage instance
		error =
			TMrConstruction_Instance_Renew(
				ONcTemplate_DiaryPage,
				inInstanceName,
				0,
				&diary_page);
		if (error != UUcError_None)
		{
			IGUIiStringList_Delete(string_list);
			string_list = NULL;
			
			IMPmError_ReturnOnErrorMsg(error, "Unable to create diary page instance");
		}
		
		// fill in the elements of the diary_page
		diary_page->level_number = level_number;
		diary_page->page_number = page_number;
		diary_page->has_new_move = has_new_move;
		diary_page->page = page;
				
		for (i = 0; i < ONcMaxNumKeys; i++)
		{
			diary_page->keys[i] = keys[i];
		}

		IGUIiStringList_Delete(string_list);
		string_list = NULL;
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
Imp_AddHelpPage(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;

	UUtBool				build_instance;
	
	
	// check to see if the dialogs need to be built
	build_instance = !TMrConstruction_Instance_CheckExists(ONcTemplate_HelpPage, inInstanceName);
	
	if (build_instance)
	{
		ONtHelpPage			*help_page;
		ONtIGUI_StringList	*string_list;
		
		// get the strings
		string_list = IGUIiStringList_Get(inSourceFile);
		if (string_list == NULL)
		{
			Imp_PrintWarning("Unable to get the string list for the help page");
			return UUcError_None;
		}

		// create the help page template instance
		error =
			TMrConstruction_Instance_Renew(
				ONcTemplate_HelpPage,
				inInstanceName,
				0,
				&help_page);
		IMPmError_ReturnOnErrorMsg(error, "unable to create the help page");
		
		// create the text page
		help_page->page = Imp_IGUI_Page(inSourceFile, inGroup, string_list);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
Imp_AddItemPage(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;

	UUtBool				build_instance;
	
	// check to see if the dialogs need to be built
	build_instance = !TMrConstruction_Instance_CheckExists(ONcTemplate_ItemPage, inInstanceName);

	if (build_instance)
	{
		char				*type;
		UUtUns32			powerup_type;
		ONtItemPage			*item_page;
		ONtIGUI_StringList	*string_list;
		
		// get the strings
		string_list = IGUIiStringList_Get(inSourceFile);
		if (string_list == NULL)
		{
			Imp_PrintWarning("Unable to get the string list for the item page");
			return UUcError_None;
		}

		// get the type
		error = GRrGroup_GetString(inGroup, "type", &type);
		IMPmError_ReturnOnErrorMsg(error, "unable to get the item page type");
		
		error =
			AUrFlags_ParseFromGroupArray(
				IMPgPowerupTypes,
				GRcElementType_String,
				type,
				&powerup_type);
		IMPmError_ReturnOnErrorMsg(error, "Unable to process keys");
		
		// create the template instance
		error =
			TMrConstruction_Instance_Renew(
				ONcTemplate_ItemPage,
				inInstanceName,
				0,
				&item_page);
		IMPmError_ReturnOnErrorMsg(error, "Unable to create item page instance");
				
		// set the data
		item_page->powerup_type = (WPtPowerupType)powerup_type;
		item_page->page = Imp_IGUI_Page(inSourceFile, inGroup, string_list);
	}
		
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
Imp_AddObjectivePage(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;

	UUtBool				build_instance;
	
	
	// check to see if the dialogs need to be built
	build_instance = !TMrConstruction_Instance_CheckExists(ONcTemplate_ObjectivePage, inInstanceName);
	
	if (build_instance)
	{
		UUtInt16				level_number;
		ONtObjectivePage		*objective_page;
		ONtIGUI_PageArray		*objective_page_array;
		ONtIGUI_StringList		*string_list;
		
		// get the strings
		string_list = IGUIiStringList_Get(inSourceFile);
		if (string_list == NULL)
		{
			Imp_PrintWarning("Unable to get the string list for the objective page");
			return UUcError_None;
		}

		// get the level number
		error = GRrGroup_GetInt16(inGroup, "level_number", &level_number);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get the level number");
		
		objective_page_array = Imp_IGUI_PageArray(inSourceFile, inGroup, "objective_array", string_list);
		if (objective_page_array == NULL)
		{
			Imp_PrintWarning("Unable to process the objective page array.");
		}

		// create the template instance
		error =
			TMrConstruction_Instance_Renew(
				ONcTemplate_ObjectivePage,
				inInstanceName,
				0,
				&objective_page);
		IMPmError_ReturnOnErrorMsg(error, "Unable to create objective page instance");
		
		objective_page->level_number = level_number;
		objective_page->objectives = objective_page_array;
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
Imp_AddWeaponPage(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;

	UUtBool				build_instance;
	
	
	// check to see if the dialogs need to be built
	build_instance = !TMrConstruction_Instance_CheckExists(ONcTemplate_WeaponPage, inInstanceName);
	
	if (build_instance)
	{
		char				*weapon_class;
		ONtWeaponPage		*weapon_page;
		ONtIGUI_StringList	*string_list;
		
		// get the strings
		string_list = IGUIiStringList_Get(inSourceFile);
		if (string_list == NULL)
		{
			Imp_PrintWarning("Unable to get the string list for the weapon page");
			return UUcError_None;
		}

		// get the weapon class
		error = GRrGroup_GetString(inGroup, "weaponClass", &weapon_class);
		IMPmError_ReturnOnErrorMsg(error, "unable to get weapon class");
		
		// create the weapon page template instance
		error =
			TMrConstruction_Instance_Renew(
				ONcTemplate_WeaponPage,
				inInstanceName,
				0,
				&weapon_page);
		IMPmError_ReturnOnErrorMsg(error, "unable to create the weapon page");
		
		// save a placeholder to the weapon class
		error =
			TMrConstruction_Instance_GetPlaceHolder(
				WPcTemplate_WeaponClass,
				weapon_class,
				(TMtPlaceHolder*)&(weapon_page->weaponClass));
		UUmError_ReturnOnErrorMsg(error, "could not set weapon page's weapon class");
		
		// create the text page
		weapon_page->page = Imp_IGUI_Page(inSourceFile, inGroup, string_list);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
Imp_AddEnemyScannerUI(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
#if 0
	UUtError			error;

	UUtBool				bool_result;
	UUtBool				build_instance;
	
	UUtUns32			create_date;
	UUtUns32			compile_date;
	
	// check to see if the dialogs need to be built
	bool_result =
		TMrConstruction_Instance_CheckExists(
			ONcTemplate_WeaponPage,
			inInstanceName,
			&create_date);
	if (bool_result)
	{
		compile_date = UUrConvertStrToSecsSince1900(gInGameUICompileTime);
		
		build_instance = (UUtBool)(create_date < inSourceFileModDate ||
									create_date < compile_date);
	}
	else
	{
		build_instance = UUcTrue;
	}
	
	if (build_instance)
	{
		char					*focus_texture_path;
		char					*background_texture_path;
		TMtPlaceHolder			focus_texture;
		TMtPlaceHolder			background_texture;
		ONtEnemyScannerUI		*enemy_scanner_ui;
				
		// get a pointer to all of the textures
		error = GRrGroup_GetString(inGroup, "focus_texture", &focus_texture_path);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get path for focus texture");
		
		error = GRrGroup_GetString(inGroup, "background_texture", &background_texture_path);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get path for backgrouind texture");
		
		// create placeholders for the textures
		error = iGetTextureByPath(focus_texture_path, inSourceFile, &focus_texture);
		IMPmError_ReturnOnErrorMsg(error, "unable to process the focus texture");
		
		error = iGetTextureBigByPath(background_texture_path, inSourceFile, &background_texture);
		IMPmError_ReturnOnErrorMsg(error, "unable to process the background texture");
		
		// create an instance of the enemy scanner ui
		error =
			TMrConstruction_Instance_Renew(
				ONcTemplate_EnemyScannerUI,
				inInstanceName,
				0,
				&enemy_scanner_ui);
		IMPmError_ReturnOnErrorMsg(error, "Unable to create enemy scanner ui");
		
		// save the pointers
		enemy_scanner_ui->focus = (M3tTextureMap*)focus_texture;
		enemy_scanner_ui->background = (M3tTextureMap_Big*)background_texture;
	}
#endif
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
Imp_AddKeyIcons(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;

	UUtBool				build_instance;
	
	
	// check to see if the dialogs need to be built
	build_instance = !TMrConstruction_Instance_CheckExists(ONcTemplate_KeyIcons, inInstanceName);
	
	if (build_instance)
	{
		char				*path;
		TMtPlaceHolder		place_holder;
		ONtKeyIcons			*key_icons;
		
		// build the key icons
		error = 
			TMrConstruction_Instance_Renew(
				ONcTemplate_KeyIcons,
				inInstanceName,
				0,
				&key_icons);
		IMPmError_ReturnOnErrorMsg(error,"Unable to create the key icons instance");
		
		// load the icons
		// ------------------------------
		error = GRrGroup_GetString(inGroup, "punch", &path);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get path");
		
		error = iGetTextureByPath(path, inSourceFile, &place_holder);
		IMPmError_ReturnOnErrorMsg(error, "unable to texture place holder");
		
		key_icons->punch = (M3tTextureMap*)place_holder;

		// ------------------------------
		error = GRrGroup_GetString(inGroup, "kick", &path);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get path");
		
		error = iGetTextureByPath(path, inSourceFile, &place_holder);
		IMPmError_ReturnOnErrorMsg(error, "unable to texture place holder");
		
		key_icons->kick = (M3tTextureMap*)place_holder;

		// ------------------------------
		error = GRrGroup_GetString(inGroup, "forward", &path);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get path");
		
		error = iGetTextureByPath(path, inSourceFile, &place_holder);
		IMPmError_ReturnOnErrorMsg(error, "unable to texture place holder");
		
		key_icons->forward = (M3tTextureMap*)place_holder;

		// ------------------------------
		error = GRrGroup_GetString(inGroup, "backward", &path);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get path");
		
		error = iGetTextureByPath(path, inSourceFile, &place_holder);
		IMPmError_ReturnOnErrorMsg(error, "unable to texture place holder");
		
		key_icons->backward = (M3tTextureMap*)place_holder;

		// ------------------------------
		error = GRrGroup_GetString(inGroup, "left", &path);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get path");
		
		error = iGetTextureByPath(path, inSourceFile, &place_holder);
		IMPmError_ReturnOnErrorMsg(error, "unable to texture place holder");
		
		key_icons->left = (M3tTextureMap*)place_holder;

		// ------------------------------
		error = GRrGroup_GetString(inGroup, "right", &path);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get path");
		
		error = iGetTextureByPath(path, inSourceFile, &place_holder);
		IMPmError_ReturnOnErrorMsg(error, "unable to texture place holder");
		
		key_icons->right = (M3tTextureMap*)place_holder;

		// ------------------------------
		error = GRrGroup_GetString(inGroup, "crouch", &path);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get path");
		
		error = iGetTextureByPath(path, inSourceFile, &place_holder);
		IMPmError_ReturnOnErrorMsg(error, "unable to texture place holder");
		
		key_icons->crouch = (M3tTextureMap*)place_holder;

		// ------------------------------
		error = GRrGroup_GetString(inGroup, "jump", &path);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get path");
		
		error = iGetTextureByPath(path, inSourceFile, &place_holder);
		IMPmError_ReturnOnErrorMsg(error, "unable to texture place holder");
		
		key_icons->jump = (M3tTextureMap*)place_holder;

		// ------------------------------
		error = GRrGroup_GetString(inGroup, "hold", &path);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get path");
		
		error = iGetTextureByPath(path, inSourceFile, &place_holder);
		IMPmError_ReturnOnErrorMsg(error, "unable to texture place holder");
		
		key_icons->hold = (M3tTextureMap*)place_holder;

		// ------------------------------
		error = GRrGroup_GetString(inGroup, "plus", &path);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get path");
		
		error = iGetTextureByPath(path, inSourceFile, &place_holder);
		IMPmError_ReturnOnErrorMsg(error, "unable to texture place holder");
		
		key_icons->plus = (M3tTextureMap*)place_holder;
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
Imp_AddTextConsole(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;
	
	UUtBool				build_instance;
	
	
	// check to see if the dialogs need to be built
	build_instance = !TMrConstruction_Instance_CheckExists(ONcTemplate_TextConsole, inInstanceName);

	if (build_instance)
	{
		ONtIGUI_StringList			*string_list;
		ONtIGUI_PageArray			*console_data;
		ONtTextConsole				*text_console;
		
		// get the strings
		string_list = IGUIiStringList_Get(inSourceFile);
		if (string_list == NULL)
		{
			Imp_PrintWarning("Unable to get the string list for the text console");
			return UUcError_None;
		}
		
		// get the console_data
		console_data = Imp_IGUI_PageArray(inSourceFile, inGroup, "text", string_list);
		if (console_data == NULL)
		{
			IGUIiStringList_Delete(string_list);
			string_list = NULL;
			
			Imp_PrintWarning("Unable to get the text for the text console");
			return UUcError_None;
		}
		
		IGUIiStringList_Delete(string_list);
		string_list = NULL;
		
		// create the text console instance
		error =
			TMrConstruction_Instance_Renew(
				ONcTemplate_TextConsole,
				inInstanceName,
				0,
				&text_console);
		IMPmError_ReturnOnErrorMsg(error, "Unable to create the text console instance");
		
		// set the console_data of the text_console
		text_console->console_data = console_data;
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
Imp_AddHUDHelp(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;

	UUtBool				build_instance;
	
	// check to see if the instance needs to be built
	build_instance = !TMrConstruction_Instance_CheckExists(ONcTemplate_IGUI_HUDHelp, inInstanceName);
	
	if (build_instance)
	{
		char				*leftTexture;
		char				*rightTexture;
		char				*file_key;
		IMtPoint2D			leftOffset, rightOffset;
		BFtFileRef			*file_ref;
		BFtTextFile			*text_file;
		UUtUns32			i, j, len;
		UUtUns32			left_items, right_items, *num_items;
		ONtIGUI_HUDTextItem	left_itemarray[ONcIGUI_HUDHelpText_MaxItems];
		ONtIGUI_HUDTextItem	right_itemarray[ONcIGUI_HUDHelpText_MaxItems];
		ONtIGUI_HUDTextItem	*item_array;
		char				*text_line, *stringptr, *string_end;
		ONtIGUI_HUDHelp		*help_instance;
		
		// get the textures
		error = GRrGroup_GetString(inGroup, "left_texture", &leftTexture);
		IMPmError_ReturnOnErrorMsg(error, "unable to get left texture");

		error = GRrGroup_GetString(inGroup, "right_texture", &rightTexture);
		IMPmError_ReturnOnErrorMsg(error, "unable to get right texture");

		// get the offsets
		error = GRrGroup_GetInt16(inGroup, "left_offset_x", &leftOffset.x);
		IMPmError_ReturnOnErrorMsg(error, "unable to get left x offset");
		
		error = GRrGroup_GetInt16(inGroup, "left_offset_y", &leftOffset.y);
		IMPmError_ReturnOnErrorMsg(error, "unable to get left y offset");

		error = GRrGroup_GetInt16(inGroup, "right_offset_x", &rightOffset.x);
		IMPmError_ReturnOnErrorMsg(error, "unable to get right x offset");

		error = GRrGroup_GetInt16(inGroup, "right_offset_y", &rightOffset.y);
		IMPmError_ReturnOnErrorMsg(error, "unable to get right y offset");

		// read the text strings
		for (i = 0; i < 2; i++) {
			if (i == 0) {
				file_key = "left_text";
				num_items = &left_items;
				item_array = left_itemarray;
			} else {
				file_key = "right_text";
				num_items = &right_items;
				item_array = right_itemarray;
			}

			error = Imp_Common_GetFileRefAndModDate(inSourceFile, inGroup, file_key, &file_ref);
			IMPmError_ReturnOnErrorMsg(error, "unable to open HUD help text file");

			error = BFrTextFile_MapForRead(file_ref, &text_file);
			IMPmError_ReturnOnErrorMsg(error, "unable to parse HUD help text file for reading as text");

			*num_items = 0;
			while((text_line = BFrTextFile_GetNextStr(text_file)) != NULL)
			{
				stringptr = strchr(text_line, '#');
				if (stringptr != NULL) {
					*stringptr = '\0';
				}

				j = sscanf(text_line, "%hd, %hd", &item_array[*num_items].offset.x, &item_array[*num_items].offset.y);
				if (j < 2) {
					continue;
				}

				// get the start of the string (the first non-whitespace character after the second comma)
				stringptr = strchr(text_line, ',');
				if (stringptr == NULL) {
					continue;
				}
				stringptr = strchr(stringptr + 1, ',');
				if (stringptr == NULL) {
					continue;
				}

				do {
					stringptr++;
				} while ((*stringptr != '\0') && (isspace(*stringptr)));

				if (*stringptr == '\0') {
					// no string found
					continue;
				}

				// strip whitespace from the end of the string
				len = strlen(stringptr);
				string_end = stringptr + len - 1;
				while ((string_end > stringptr) && (isspace(*string_end))) {
					string_end--;
					len--;
				}

				// copy the string
				len = UUmMin(len, ONcIGUI_HUDMaxStringLength - 1);
				UUrString_Copy_Length(item_array[*num_items].text, stringptr, ONcIGUI_HUDMaxStringLength, len);

				(*num_items)++;
				if (*num_items >= ONcIGUI_HUDHelpText_MaxItems) {
					Imp_PrintWarning("in-game HUD help text file %s has too many items (%d) - increase ONcIGUI_HUDMaxStringLength",
									BFrFileRef_GetLeafName(file_ref), *num_items);
					break;
				}
			}
		}

		// create the HUDHelp instance
		error =
			TMrConstruction_Instance_Renew(
				ONcTemplate_IGUI_HUDHelp,
				inInstanceName,
				left_items + right_items,
				&help_instance);
		IMPmError_ReturnOnErrorMsg(error, "Unable to create HUD help info instance");
		
		// fill in the elements of the help_instance
		error = TMrConstruction_Instance_GetPlaceHolder(M3cTemplate_TextureMap, leftTexture, (TMtPlaceHolder *) &help_instance->left_texture);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get left-texture placeholder");

		error = TMrConstruction_Instance_GetPlaceHolder(M3cTemplate_TextureMap, rightTexture, (TMtPlaceHolder *) &help_instance->right_texture);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get right-texture placeholder");

		help_instance->left_offset = leftOffset;
		help_instance->right_offset = rightOffset;
		help_instance->num_left_items = left_items;
		help_instance->num_right_items = right_items;

		UUrMemory_MoveFast(left_itemarray, &help_instance->text_items[0], left_items * sizeof(ONtIGUI_HUDTextItem));
		UUrMemory_MoveFast(right_itemarray, &help_instance->text_items[left_items], right_items * sizeof(ONtIGUI_HUDTextItem));
	}
	
	return UUcError_None;
}

