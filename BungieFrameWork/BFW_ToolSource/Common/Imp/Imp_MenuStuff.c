// ======================================================================
// Imp_MenuStuff.c
// ======================================================================

// ======================================================================
// Includes
// ======================================================================
#include "BFW.h"
#include "BFW_TM_Construction.h"
#include "BFW_Group.h"
#include "BFW_AppUtilities.h"

#include "WM_Menu.h"
#include "WM_MenuBar.h"

#include "Imp_Common.h"
#include "Imp_Texture.h"
#include "Imp_MenuStuff.h"

// ======================================================================
// globals
// ======================================================================
static char*	gMenuStuffCompileTime = __DATE__" "__TIME__;

static AUtFlagElement	IMPgMenuFlags[] =
{
	{ "none",						WMcMenuItemFlag_None },
	{ "divider",					WMcMenuItemFlag_Divider },
	{ "enabled",					WMcMenuItemFlag_Enabled },
	{ NULL,							0 }
};

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
UUtError
Imp_AddMenu(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;

	UUtBool				build_instance;
	
	
	// check to see if the dialogs need to be built
	build_instance = !TMrConstruction_Instance_CheckExists(WMcTemplate_MenuData, inInstanceName);
	
	if (build_instance)
	{
		GRtElementType				element_type;
		GRtElementArray				*items_array;
		UUtUns32					num_elements;
		UUtUns32					i;
		char						*menu_title;
		WMtMenuData					*menu;
		
		// get the items array
		error =
			GRrGroup_GetElement(
				inGroup,
				"items",
				&element_type,
				&items_array);
		if ((error != UUcError_None) || (element_type != GRcElementType_Array))
		{
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to get items array");
		}
		
		// get the number of elements in the items array
		num_elements = GRrGroup_Array_GetLength(items_array);
		if (num_elements == 0)
		{
			return UUcError_None;
		}
		
		// create an instance of the menu template
		error =
			TMrConstruction_Instance_Renew(
				WMcTemplate_MenuData,
				inInstanceName,
				num_elements,
				&menu);
		IMPmError_ReturnOnError(error);
		
		// get the menu id
		error =
			GRrGroup_GetUns16(
				inGroup,
				"id",
				&menu->id);
		IMPmError_ReturnOnError(error);
		
		// get the menu title
		error =
			GRrGroup_GetString(
				inGroup,
				"title",
				&menu_title);
		IMPmError_ReturnOnError(error);
		
		UUrString_Copy(menu->title, menu_title, WMcMaxTitleLength);
		
		// process the items of the items array
		for (i = 0; i < num_elements; i++)
		{
			GRtGroup				*item_group;
			char					*item_title;
			UUtUns32				item_flags;
			GRtElementArray			*item_flags_array;
			
			// get the item group
			error =
				GRrGroup_Array_GetElement(
					items_array,
					i,
					&element_type,
					&item_group);
			if ((error != UUcError_None) || (element_type != GRcElementType_Group))
			{
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to get item group");
			}
			
			// get the item title from the item group
			error =
				GRrGroup_GetString(
					item_group,
					"title",
					&item_title);
			IMPmError_ReturnOnError(error);
			
			UUrString_Copy(menu->items[i].title, item_title, WMcMaxTitleLength);
				
			// get the item id from the item group
			error =
				GRrGroup_GetUns16(
					item_group,
					"id",
					&menu->items[i].id);
			
			// get the item flags array from the item group
			error =
				GRrGroup_GetElement(
					item_group,
					"flags",
					&element_type,
					&item_flags_array);
			if ((error != UUcError_None) || (element_type != GRcElementType_Array))
			{
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to get flags array");
			}
			
			// process the item flags array
			error =
				AUrFlags_ParseFromGroupArray(
					IMPgMenuFlags,
					GRcElementType_Array,
					item_flags_array,
					&item_flags);
			IMPmError_ReturnOnErrorMsg(error, "Unable to process button flags");
			
			menu->items[i].flags = (UUtUns16)item_flags;
		}
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
Imp_AddMenuBar(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;

	UUtBool				build_instance;
	
	
	// check to see if the dialogs need to be built
	build_instance = !TMrConstruction_Instance_CheckExists(WMcTemplate_MenuBarData, inInstanceName);
	
	if (build_instance)
	{
		GRtElementArray			*menus_array;
		UUtUns32				num_elements;
		UUtUns32				i;
		GRtElementType			element_type;
		WMtMenuBarData			*menubar;
		
		// get the menus array
		error =
			GRrGroup_GetElement(
				inGroup,
				"menus",
				&element_type,
				&menus_array);
		if ((error != UUcError_None) || (element_type != GRcElementType_Array))
		{
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to get menus array");
		}
		
		// get the number of elements in the menus array
		num_elements = GRrGroup_Array_GetLength(menus_array);
		if (num_elements == 0)
		{
			return UUcError_None;
		}
		
		// create an instance of a menubar template
		error =
			TMrConstruction_Instance_Renew(
				WMcTemplate_MenuBarData,
				inInstanceName,
				num_elements,
				&menubar);
		IMPmError_ReturnOnError(error);
		
		// get the item id of the menubar
		error =
			GRrGroup_GetUns16(
				inGroup,
				"id",
				&menubar->id);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get the id of the menubar");
		
		// process the elements in the menus array
		for (i = 0; i < num_elements; i++)
		{
			char				*menu_name;
			TMtPlaceHolder		menu_ref;
			
			// get an item from the menus_array
			error =
				GRrGroup_Array_GetElement(
					menus_array,
					i,
					&element_type,
					&menu_name);
			if ((error != UUcError_None) || (element_type != GRcElementType_String))
			{
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to get menu name from array");
			}
			
			// get a placeholder for the item
			error = 
				TMrConstruction_Instance_GetPlaceHolder(
					WMcTemplate_MenuData,
					menu_name,
					&menu_ref);
			IMPmError_ReturnOnErrorMsg(error, "Could not get menu placeholder");
			
			menubar->menus[i] = (WMtMenuData*)menu_ref;
		}
	}

	return UUcError_None;
}

