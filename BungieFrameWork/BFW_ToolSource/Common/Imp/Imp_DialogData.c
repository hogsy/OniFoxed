// ======================================================================
// Imp_DialogData.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_TM_Construction.h"
#include "BFW_Group.h"
#include "BFW_AppUtilities.h"

#include "BFW_WindowManager.h"
#include "WM_Box.h"
#include "WM_Button.h"
#include "WM_CheckBox.h"
#include "WM_Dialog.h"
#include "WM_EditField.h"
#include "WM_ListBox.h"
#include "WM_Picture.h"
#include "WM_PopupMenu.h"
#include "WM_RadioButton.h"
#include "WM_Text.h"

#include "Imp_Common.h"
#include "Imp_DialogData.h"

// ======================================================================
// enums
// ======================================================================
enum
{
	cItemType,
	cItemName,
	cItemID,
	cItemFlags,
	cItemStyles,
	cItemX,
	cItemY,
	cItemWidth,
	cItemHeight,
	
	cNumItems,
	
	cItemFormat = cNumItems,
	
	cMaxNumItems
};

// ======================================================================
// globals
// ======================================================================
static char*	gDialogDataCompileTime = __DATE__" "__TIME__;

static AUtFlagElement	IMPgWindowFlags[] =
{
	{ "none",						WMcWindowFlag_None },
	{ "visible",					WMcWindowFlag_Visible },
	{ "disabled",					WMcWindowFlag_Disabled },
	{ "mouse_transparent",			WMcWindowFlag_MouseTransparent },

	{ NULL,							0 }
};

static AUtFlagElement	IMPgWindowStyles[] =
{
	{ "has_background",				WMcWindowStyle_HasBackground },
	{ "has_border",					WMcWindowStyle_HasBorder },
	{ "has_drag",					WMcWindowStyle_HasDrag },
	{ "has_title",					WMcWindowStyle_HasTitle },
	{ "has_close",					WMcWindowStyle_HasClose },
	{ "has_zoom",					WMcWindowStyle_HasZoom },
	{ "has_flatten",				WMcWindowStyle_HasFlatten },
	
	// window styles
	{ "window_basic",				WMcWindowStyle_Basic },
	{ "window_standard",			WMcWindowStyle_Standard },
	
	// dialog styles
	{ "dialog_centered",			WMcDialogStyle_Centered },
	{ "dialog_modal",				WMcDialogStyle_Modal },
	{ "dialog_standard",			WMcDialogStyle_Standard },
	
	// box styles
	{ "box_has_outline",			WMcBoxStyle_HasOutline },
	{ "box_has_background",			WMcBoxStyle_HasBackground },
	{ "box_has_title",				WMcBoxStyle_HasTitle },
	{ "box_plain",					WMcBoxStyle_PlainBox },
	{ "box_group",					WMcBoxStyle_GroupBox },
	
	// button styles
	{ "btn_has_background",			WMcButtonStyle_HasBackground },
	{ "btn_has_title",				WMcButtonStyle_HasTitle },
	{ "btn_has_icon",				WMcButtonStyle_HasIcon },
	{ "btn_toggle",					WMcButtonStyle_Toggle },
	{ "btn_default",				WMcButtonStyle_Default },
	{ "btn_push_button",			WMcButtonStyle_PushButton },
	{ "btn_default_push_button",	WMcButtonStyle_DefaultPushButton },
	{ "btn_icon_button",			WMcButtonStyle_IconButton },
	{ "btn_toggle_button",			WMcButtonStyle_ToggleButton },
	{ "btn_toggle_icon_button",		WMcButtonStyle_ToggleIconButton	},
	
	// checkbox styles
	{ "cb_has_title",				WMcCheckBoxStyle_HasTitle },
	{ "cb_has_icon",				WMcCheckBoxStyle_HasIcon },
	{ "cb_text_checkbox",			WMcCheckBoxStyle_TextCheckBox },
	{ "cb_icon_checkbox",			WMcCheckBoxStyle_IconCheckBox },
	
	// editfield styles
	{ "ef_number_only",				WMcEditFieldStyle_NumbersOnly },
	
	// listbox styles
	{ "lb_has_scrollbar",			WMcListBoxStyle_HasScrollbar },
	{ "lb_sort",					WMcListBoxStyle_Sort },
	{ "lb_multiple_selection",		WMcListBoxStyle_MultipleSelection },
	{ "lb_has_strings",				WMcListBoxStyle_HasStrings },
	{ "lb_owner_draw",				WMcListBoxStyle_OwnerDraw },
	{ "lb_directory",				WMcListBoxStyle_Directory },
	{ "lb_simple_listbox",			WMcListBoxStyle_SimpleListBox },
	{ "lb_sorted_listbox",			WMcListBoxStyle_SortedListBox },
	
	// menu styles
	// menubar styles
	
	// picture
	{ "pt_scale",					WMcPictureStyle_Scale },
	{ "pt_set_at_runtime",			WMcPictureStyle_SetAtRuntime },
	
	// popup menu
	{ "pm_no_resize",		 		WMcPopupMenuStyle_NoResize },
	{ "pm_build_at_runtime", 		WMcPopupMenuStyle_BuildAtRuntime },
	
	// progressbar styles
	
	// radiobutton styles
	{ "rb_has_title",				WMcRadioButtonStyle_HasTitle },
	{ "rb_has_icon",				WMcRadioButtonStyle_HasIcon },
	{ "rb_text_radiobutton",		WMcRadioButtonStyle_TextRadioButton },
	{ "rb_icon_radiobutton",		WMcRadioButtonStyle_IconRadioButton },
	
	// scrollbar styles
	// slider styles
	
	// text styles
	{ "text_hleft", 				WMcTextStyle_HLeft },
	{ "text_hcenter", 				WMcTextStyle_HCenter },
	{ "text_hright", 				WMcTextStyle_HRight },
	{ "text_vtop",					WMcTextStyle_VTop },
	{ "text_vcenter", 				WMcTextStyle_VCenter },
	{ "text_vbottom",				WMcTextStyle_VBottom },
	{ "text_singleline",			WMcTextStyle_SingleLine },
	{ "text_owner_draw",			WMcTextStyle_OwnerDraw },
	{ "text_basic",					WMcTextStyle_Basic },
	{ "text_standard",				WMcTextStyle_Standard },
	
	{ NULL,							0 }
};

static AUtFlagElement	IMPgWindowTypes[] =
{
	{ "none",						WMcWindowType_None },
	{ "box",						WMcWindowType_Box },
	{ "button",						WMcWindowType_Button },
	{ "checkbox",					WMcWindowType_CheckBox },
	{ "editfield",					WMcWindowType_EditField },
	{ "listbox",					WMcWindowType_ListBox },
	{ "menubar",					WMcWindowType_MenuBar },
	{ "menu",						WMcWindowType_Menu },
	{ "picture",					WMcWindowType_Picture },
	{ "popup_menu",					WMcWindowType_PopupMenu },
	{ "progressbar",				WMcWindowType_ProgressBar },
	{ "radiobutton",				WMcWindowType_RadioButton },
	{ "radiogroup",					WMcWindowType_RadioGroup },
	{ "scrollbar",					WMcWindowType_Scrollbar },
	{ "slider",						WMcWindowType_Slider },
	{ "tab",						WMcWindowType_Tab },
	{ "tabgroup",					WMcWindowType_TabGroup },
	{ "text",						WMcWindowType_Text },
	{ NULL,							0 }
};

AUtFlagElement			IMPgFontStyleTypes[] =
{
	{ "plain",						TScStyle_Plain },
	{ "bold",						TScStyle_Bold },
	{ "italic",						TScStyle_Italic },
	{ NULL,							0 }
};


AUtFlagElement			IMPgFontShadeTypes[] =
{
	{ "black",						IMcShade_Black },
	{ "white",						IMcShade_White },
	{ "red",						IMcShade_Red },
	{ "green",						IMcShade_Green },
	{ "blue",						IMcShade_Blue },
	{ "yellow",						IMcShade_Yellow },
	{ "pink",						IMcShade_Pink },
	{ "lightblue",					IMcShade_LightBlue },
	{ "orange",						IMcShade_Orange },
	{ "purple",						IMcShade_Purple },
	{ "gray75",						IMcShade_Gray75 },
	{ "gray50",						IMcShade_Gray50 },
	{ "gray40",						IMcShade_Gray40 },
	{ "gray35",						IMcShade_Gray35 },
	{ "gray25",						IMcShade_Gray25 },
	{ NULL,							0 }
};

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
UUtError
IMPrProcess_FontInfo(
	void					*inData,
	GRtElementType			inElementType,
	TStFontInfo				*outFontInfo)
{
	UUtError				error;
	GRtElementType			element_type;
	
	char					*font_name;
	TMtPlaceHolder			font_family;
	void					*font_size_data;
	UUtUns32				font_size;
	void					*font_style_data;
	UUtUns32				font_style;
	void					*font_shade_data;
	UUtUns32				font_shade;
	
	if (inData == NULL)
	{
		font_name = TScFontFamily_Default;
		font_size = TScFontSize_Default;
		font_style = TScFontStyle_Default;
		font_shade = TScFontShade_Default;
	}
	else if (inElementType == GRcElementType_Group)
	{
		error = GRrGroup_GetString(inData, "font", &font_name);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get font name");
		
		error = GRrGroup_GetUns32(inData, "font_size", &font_size);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get font size");

		error = GRrGroup_GetElement(inData, "font_style", &element_type, &font_style_data);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get font style data");
		
		error =
			AUrFlags_ParseFromGroupArray(
				IMPgFontStyleTypes,
				element_type,
				font_style_data,
				&font_style);
		IMPmError_ReturnOnErrorMsg(error, "Unable to process font style data");
		
		error = GRrGroup_GetElement(inData, "font_shade", &element_type, &font_shade_data);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get font shade");
		
		error =
			AUrFlags_ParseFromGroupArray(
				IMPgFontShadeTypes,
				element_type,
				font_shade_data,
				&font_shade);
		if (error != UUcError_None)
		{
			// try processing the data as a number
			error = GRrGroup_GetUns32(inData, "font_shade", &font_shade);
			IMPmError_ReturnOnErrorMsg(error, "Unable to process font style data");
		}
	}
	else if (inElementType == GRcElementType_Array)
	{
		error = GRrGroup_Array_GetElement(inData, cInfoFont, &element_type, &font_name);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get font name");
		
		error = GRrGroup_Array_GetElement(inData, cInfoSize, &element_type, &font_size_data);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get font size");
		
		sscanf((char*)font_size_data, "%d", &font_size);
		
		error = GRrGroup_Array_GetElement(inData, cInfoStyle, &element_type, &font_style_data);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get font size");
		
		error =
			AUrFlags_ParseFromGroupArray(
				IMPgFontStyleTypes,
				element_type,
				font_style_data,
				&font_style);
		IMPmError_ReturnOnErrorMsg(error, "Unable to process font style data");
		
		error = GRrGroup_Array_GetElement(inData, cInfoShade, &element_type, &font_shade_data);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get font shade");
		
		error =
			AUrFlags_ParseFromGroupArray(
				IMPgFontShadeTypes,
				element_type,
				font_shade_data,
				&font_shade);
		if (error != UUcError_None)
		{
			if (('0' == ((char*)font_shade_data)[0]) && ('x' == ((char*)font_shade_data)[1])) {
				sscanf(((char*)font_shade_data) + 2, "%x", &font_shade);
			}
			else {
				sscanf(((char*)font_shade_data), "%d", &font_shade);
			}
		}
	}
	else
	{
		IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to process formatting data");
	}
	
	// get a placeholder for the font
	error = 
		TMrConstruction_Instance_GetPlaceHolder(
			TScTemplate_FontFamily,
			font_name,
			&font_family);
	IMPmError_ReturnOnErrorMsg(error, "Could not get partspec placeholder");
	
	// save the font info	
	outFontInfo->font_family = (TStFontFamily*)font_family;
	outFontInfo->font_size = (UUtUns16)font_size;
	outFontInfo->font_style = (TStFontStyle)font_style;
	outFontInfo->font_shade = font_shade;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiProcess_Group(
	GRtGroup				*inItemGroup,
	WMtDialogItemData		*ioItemData)
{
	UUtError				error;
	char					*type_name;
	UUtUns32				type;
	char					*name;
	UUtUns16				id;
	GRtElementArray			*flags_array;
	GRtElementArray			*styles_array;
	UUtUns32				flags;
	UUtUns32				style;
	UUtInt16				x;
	UUtInt16				y;
	UUtInt16				width;
	UUtInt16				height;
	GRtElementType			element_type;
	void					*format_data;
	TStFontInfo				font_info;
	
	// get the elements from the item group
	
	// get the item type
	error = GRrGroup_GetElement(inItemGroup, "type", &element_type, &type_name);
	IMPmError_ReturnOnErrorMsg(error, "Unable to get type name");
	
	error = AUrFlags_ParseFromGroupArray(IMPgWindowTypes, element_type, type_name, &type);
	IMPmError_ReturnOnErrorMsg(error, "Unable to process item type");
	
	// get the item name
	error = GRrGroup_GetString(inItemGroup, "name", &name);
	IMPmError_ReturnOnErrorMsg(error, "Unable to get dialog name");
	
	// get the item id
	error = GRrGroup_GetUns16(inItemGroup, "id", &id);
	IMPmError_ReturnOnErrorMsg(error, "Unable to get dialog id");
	
	// get the item flags
	error =
		GRrGroup_GetElement(
			inItemGroup,
			"flags",
			&element_type,
			&flags_array);
	if (error != UUcError_None)
	{
		IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to get dialog item flags");
	}
	
	error =
		AUrFlags_ParseFromGroupArray(
			IMPgWindowFlags,
			element_type,
			flags_array,
			&flags);
	IMPmError_ReturnOnErrorMsg(error, "Unable to process flags");
	
	// get the item styles
	error =
		GRrGroup_GetElement(
			inItemGroup,
			"styles",
			&element_type,
			&styles_array);
	if (error != UUcError_None)
	{
		IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to get dialog item styles");
	}
	
	error =
		AUrFlags_ParseFromGroupArray(
			IMPgWindowStyles,
			element_type,
			styles_array,
			&style);
	IMPmError_ReturnOnErrorMsg(error, "Unable to process styles");
	
	// get the item X
	error = GRrGroup_GetInt16(inItemGroup, "x", &x);
	IMPmError_ReturnOnErrorMsg(error, "Unable to get dialog x coordinate");
	
	// get the item Y
	error = GRrGroup_GetInt16(inItemGroup, "y", &y);
	IMPmError_ReturnOnErrorMsg(error, "Unable to get dialog y coordinate");
	
	// get the item Width
	error = GRrGroup_GetInt16(inItemGroup, "width", &width);
	IMPmError_ReturnOnErrorMsg(error, "Unable to get dialog width");
	
	// get the item Height
	error = GRrGroup_GetInt16(inItemGroup, "height", &height);
	IMPmError_ReturnOnErrorMsg(error, "Unable to get dialog height");
	
	// get the optional formatting info
	error = GRrGroup_GetElement(inItemGroup, "formatting", &element_type, &format_data);
	if (error == GRcError_ElementNotFound)
	{
		error = IMPrProcess_FontInfo(NULL, element_type, &font_info);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get formatting info");
	}
	else if (error != UUcError_None)
	{
		IMPmError_ReturnOnErrorMsg(error, "Unable to process the formatting");
	}
	else
	{
		error = IMPrProcess_FontInfo(format_data, element_type, &font_info);
		IMPmError_ReturnOnErrorMsg(error, "Unable to process the formatting");
	}
	
	// initialize the item
	UUrString_Copy(ioItemData->title, name, WMcMaxTitleLength);
	ioItemData->windowtype		= (UUtUns16)type;
	ioItemData->id				= id;
	ioItemData->flags			= flags;
	ioItemData->style			= style;
	ioItemData->x				= x;
	ioItemData->y				= y;
	ioItemData->width			= width;
	ioItemData->height			= height;
	ioItemData->font_info		= font_info;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiProcess_Array(
	GRtElementArray			*inItemArray,
	WMtDialogItemData		*ioItemData)
{
	UUtError				error;
	void					*item_data;
	GRtElementType			element_type;
	char					*type_name;
	UUtUns32				type;
	char					*name;
	UUtUns32				id;
	GRtElementArray			*flags_array;
	GRtElementArray			*styles_array;
	UUtUns32				flags;
	UUtUns32				style;
	UUtUns32				x;
	UUtUns32				y;
	UUtUns32				width;
	UUtUns32				height;
	TStFontInfo				font_info;
	
	// get the number of items in the array
	if ((GRrGroup_Array_GetLength(inItemArray) != cNumItems) &&
		(GRrGroup_Array_GetLength(inItemArray) != cMaxNumItems))
	{
		IMPmError_ReturnOnErrorMsg(UUcError_Generic, "The items doesn't have the proper number of fields");
	}
	
	// get the item type
	error =	GRrGroup_Array_GetElement(inItemArray, cItemType, &element_type, &type_name);
	IMPmError_ReturnOnError(error);
		
	error = AUrFlags_ParseFromGroupArray(IMPgWindowTypes, element_type, type_name, &type);
	IMPmError_ReturnOnErrorMsg(error, "Unable to process item type");
			
	// get the item name
	error =	GRrGroup_Array_GetElement(inItemArray, cItemName, &element_type, &name);
	IMPmError_ReturnOnError(error);
		
	// get the item id
	error =	GRrGroup_Array_GetElement(inItemArray, cItemID, &element_type, &item_data);
	IMPmError_ReturnOnError(error);
	
	sscanf((char*)item_data, "%d", &id);
	
	// get the item flags
	error =	GRrGroup_Array_GetElement(inItemArray, cItemFlags, &element_type, &flags_array);
	IMPmError_ReturnOnError(error);
	
	error = AUrFlags_ParseFromGroupArray(IMPgWindowFlags, element_type, flags_array, &flags);
	IMPmError_ReturnOnErrorMsg(error, "Unable to get dialog item flags");
	
	// get the item styles
	error =	GRrGroup_Array_GetElement(inItemArray, cItemStyles, &element_type, &styles_array);
	IMPmError_ReturnOnError(error);
	
	error = AUrFlags_ParseFromGroupArray(IMPgWindowStyles, element_type, styles_array, &style);
	IMPmError_ReturnOnErrorMsg(error, "Unable to get dialog item styles");
	
	// get the item X
	error =	GRrGroup_Array_GetElement(inItemArray, cItemX, &element_type, &item_data);
	IMPmError_ReturnOnError(error);
	
	sscanf((char*)item_data, "%d", &x);
	
	// get the item Y
	error =	GRrGroup_Array_GetElement(inItemArray, cItemY, &element_type, &item_data);
	IMPmError_ReturnOnError(error);
	
	sscanf((char*)item_data, "%d", &y);
	
	// get the item Y
	error =	GRrGroup_Array_GetElement(inItemArray, cItemY, &element_type, &item_data);
	IMPmError_ReturnOnError(error);
	
	sscanf((char*)item_data, "%d", &y);
	
	// get the item width
	error =	GRrGroup_Array_GetElement(inItemArray, cItemWidth, &element_type, &item_data);
	IMPmError_ReturnOnError(error);
	
	sscanf((char*)item_data, "%d", &width);
	
	// get the item height
	error =	GRrGroup_Array_GetElement(inItemArray, cItemHeight, &element_type, &item_data);
	IMPmError_ReturnOnError(error);
	
	sscanf((char*)item_data, "%d", &height);
	
	// get the optional formatting info
	if (GRrGroup_Array_GetLength(inItemArray) == cMaxNumItems)
	{
		error = GRrGroup_Array_GetElement(inItemArray, cItemFormat, &element_type, &item_data);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get formatting info");

		error = IMPrProcess_FontInfo(item_data, element_type, &font_info);
		IMPmError_ReturnOnErrorMsg(error, "Unable to process the formatting");
	}
	else
	{
		error = IMPrProcess_FontInfo(NULL, element_type, &font_info);
		IMPmError_ReturnOnErrorMsg(error, "Unable to process the formatting");
	}
	
	// initialize the item
	UUrString_Copy(ioItemData->title, name, WMcMaxTitleLength);
	ioItemData->windowtype		= (UUtUns16)type;
	ioItemData->id				= (UUtUns16)id;
	ioItemData->flags			= flags;
	ioItemData->style			= style;
	ioItemData->x				= (UUtInt16)x;
	ioItemData->y				= (UUtInt16)y;
	ioItemData->width			= (UUtInt16)width;
	ioItemData->height			= (UUtInt16)height;
	ioItemData->font_info		= font_info;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
Imp_AddDialogData(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;

	UUtBool				build_instance;
	
	
	// check to see if the dialogs need to be built
	build_instance =
		!TMrConstruction_Instance_CheckExists(
			WMcTemplate_DialogData,
			inInstanceName);
	
	if (build_instance)
	{
		char					*name;
		UUtUns16				id;
		GRtElementArray			*flags_array;
		GRtElementArray			*styles_array;
		UUtUns32				flags;
		UUtUns32				style;
		UUtInt16				x;
		UUtInt16				y;
		UUtInt16				width;
		UUtInt16				height;
		GRtElementType			element_type;
		GRtElementArray			*items_array;
		UUtUns32				num_items;
		WMtDialogData			*dialog_data;
		UUtUns32				i;
		
		// get all the elements that will make up a dialog data instance
		error = GRrGroup_GetString(inGroup, "name", &name);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get dialog name");
		
		error = GRrGroup_GetUns16(inGroup, "id", &id);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get dialog id");
		
		error =
			GRrGroup_GetElement(
				inGroup,
				"flags",
				&element_type,
				&flags_array);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get dialog data flags");
		
		error =
			AUrFlags_ParseFromGroupArray(
				IMPgWindowFlags,
				element_type,
				flags_array,
				&flags);
		IMPmError_ReturnOnErrorMsg(error, "Unable to process button flags");
		
		error =
			GRrGroup_GetElement(
				inGroup,
				"styles",
				&element_type,
				&styles_array);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get dialog data flags");
		
		error =
			AUrFlags_ParseFromGroupArray(
				IMPgWindowStyles,
				element_type,
				styles_array,
				&style);
		IMPmError_ReturnOnErrorMsg(error, "Unable to process dialog styles");
		
		error = GRrGroup_GetInt16(inGroup, "x", &x);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get dialog x coordinate");

		error = GRrGroup_GetInt16(inGroup, "y", &y);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get dialog y coordinate");
		
		error = GRrGroup_GetInt16(inGroup, "width", &width);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get dialog width");

		error = GRrGroup_GetInt16(inGroup, "height", &height);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get dialog height");
		
		error =
			GRrGroup_GetElement(
				inGroup,
				"items",
				&element_type,
				&items_array);
		if ((error != UUcError_None) || (element_type != GRcElementType_Array))
		{
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to get dialog items array");
		}
		
		num_items = GRrGroup_Array_GetLength(items_array);
		if (num_items == 0)
		{
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "The dialog items array is empty");
		}
		
		// create the dialog data instance
		error =
			TMrConstruction_Instance_Renew(
				WMcTemplate_DialogData,
				inInstanceName,
				num_items,
				&dialog_data);
		IMPmError_ReturnOnErrorMsg(error, "Unable to create dialog data instance");
		
		// initialize the dialog_data
		UUrString_Copy(dialog_data->title, name, WMcMaxTitleLength);
		dialog_data->id		= id;
		dialog_data->unused	= 0;
		dialog_data->flags	= flags;
		dialog_data->style	= style;
		dialog_data->x		= x;
		dialog_data->y		= y;
		dialog_data->width	= width;
		dialog_data->height	= height;
		
		// process the items
		for (i = 0; i < num_items; i++)
		{
			void				*item_stuff;
			
			// get the ith item from the items_array
			error =
				GRrGroup_Array_GetElement(
					items_array,
					i,
					&element_type,
					&item_stuff);
			IMPmError_ReturnOnErrorMsg(error, "unable to get item stuff");
			
			if (element_type == GRcElementType_Group)
			{
				error = IMPiProcess_Group(item_stuff, &dialog_data->items[i]);
				IMPmError_ReturnOnErrorMsg(error, "Unable to process item group");
			}
			else if (element_type == GRcElementType_Array)
			{
				error = IMPiProcess_Array(item_stuff, &dialog_data->items[i]);
				IMPmError_ReturnOnErrorMsg(error, "Unable to process item group");
			}
			else
			{
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to process element type");
			}
		}
	}
	
	return UUcError_None;
}