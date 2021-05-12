// ======================================================================
// Oni_Win_Sound.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_SoundSystem2.h"
#include "BFW_WindowManager.h"
#include "BFW_FileManager.h"
#include "WM_Picture.h"
#include "WM_PopupMenu.h"
#include "WM_EditField.h"
#include "BFW_Materials.h"

#include "BFW_TM_Construction.h"

#include "Oni_Object.h"
#include "Oni_Sound.h"
#include "Oni_Sound_Animation.h"
#include "Oni_Windows.h"
#include "Oni_Win_Sound.h"
#include "Oni_ImpactEffect.h"

#include <ctype.h>

// ======================================================================
// defines
// ======================================================================
#define OWcCatToData			0x80000000
#define OWcDataToCat			0x7FFFFFFF

// ======================================================================
// typedefs
// ======================================================================
typedef struct OWtWS
{
	UUtMemory_Array				*dir_array;
	SStSoundData				*selected_sound_data;
	
} OWtWS;

typedef struct OWtSGP
{
	OStCollection				*collection;
	OStItem						*item;
	char						item_name[OScMaxNameLength];
	SStGroup					*group;
	UUtUns32					group_id;
	
} OWtSGP;

typedef struct OWtASP
{
	OStCollection				*collection;
	OStItem						*item;
	char						item_name[OScMaxNameLength];
	SStAmbient					*ambient;
	UUtUns32					ambient_id;
	SStPlayID					play_id;
	
} OWtASP;

typedef struct OWtISP
{
	OStCollection				*collection;
	OStItem						*item;
	char						item_name[OScMaxNameLength];
	SStImpulse					*impulse;
	UUtUns32					impulse_id;
	
} OWtISP;

typedef struct OWtSCM
{
	OStCollection				*collection;
	char						collection_name[OScMaxNameLength];

	UUtUns32					clip_ss_id;
	char						clip_name[OScMaxNameLength];
	
} OWtSCM;

typedef struct OWtSS
{
	OStCollectionType			type;
	UUtUns32					item_id;
	
} OWtSS;

typedef struct OWtStAP
{
	OStAnimType					anim_type;
	OStModType					mod_type;
	UUtUns32					impulse_id;
	TRtAnimation				*animation;
	UUtUns32					frame;
	char						variant_name[ONcMaxVariantNameLength];
	
} OWtStAP;

typedef struct OWtSA
{
	TRtAnimation				*animation;			/* in/out */
	
	TRtAnimation				**animation_list;	/* internal use only */
	UUtUns32					num_animations;		/* internal use only */
	UUtUns32					insert_from_index;	/* internal use only */
	
} OWtSA;

// ======================================================================
// globals
// ======================================================================
static WMtDialog				*OWgSP_Prop;

// ======================================================================
// prototypes
// ======================================================================
static UUtError
OWiSSG_Display(
	WMtDialog					*inParentDialog,
	UUtUns32					*ioSoundGroupID,
	SStGroup					**ioSoundGroup);
	
// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
OWiCN_Callback(
	WMtDialog					*inDialog,
	WMtMessage					inMessage,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	UUtBool						handled;
	WMtWindow					*editfield;
	char						*name;
	
	handled = UUcTrue;
	
	// get a pointer to the name
	name = (char*)WMrDialog_GetUserData(inDialog);
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			// get a pointer to the edit field
			editfield = WMrDialog_GetItemByID(inDialog, OWcCN_EF_Name);
			UUmAssert(editfield);
			
			// set a limit on the length of the name
			WMrMessage_Send(editfield, EFcMessage_SetMaxChars, OScMaxNameLength, 0);
			
			// set the name
			WMrMessage_Send(editfield, EFcMessage_SetText, (UUtUns32)name, 0);
			
			// set focus to the editfield
			WMrWindow_SetFocus(editfield);
		break;
		
		case WMcMessage_Command:
			if (UUmHighWord(inParam1) != WMcNotify_Click) { break; }
			
			// get a pointer to the edit field
			editfield = WMrDialog_GetItemByID(inDialog, OWcCN_EF_Name);
			UUmAssert(editfield);
			
			switch (UUmLowWord(inParam1))
			{
				case OWcCN_Btn_OK:
					// get the name in the editfield
					WMrMessage_Send(
						editfield,
						EFcMessage_GetText,
						(UUtUns32)name,
						OScMaxNameLength);
						
					// end the dialog
					WMrDialog_ModalEnd(inDialog, OWcCN_Btn_OK);
				break;
				
				case OWcCN_Btn_Cancel:
					// end the dialog
					WMrDialog_ModalEnd(inDialog, OWcCN_Btn_Cancel);
				break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
OWiWS_SelectSoundData(
	WMtDialog					*inDialog)
{
	UUtError					error;
	OWtWS						*ws;
	WMtWindow					*listbox;
	char						selected_file_name[BFcMaxFileNameLength];
	BFtFileRef					**dir_array;
	BFtFileRef					*current_directory;
	BFtFileRef					*file_ref;
	
	file_ref = NULL;
	
	// get a pointer to the user data
	ws = (OWtWS*)WMrDialog_GetUserData(inDialog);
	UUmAssert(ws);
	
	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcWS_LB_DirContents);
	UUmAssert(listbox);
	
	// get the selected file's name
	WMrMessage_Send(listbox, LBcMessage_GetText, (UUtUns32)selected_file_name, (UUtUns32)-1);
	UUrString_MakeLowerCase(selected_file_name, BFcMaxFileNameLength);
	
	// get the SStSoundData* corresponding to this file
	ws->selected_sound_data = SSrSoundData_GetByName(selected_file_name, UUcTrue);
	if (ws->selected_sound_data != NULL) { goto cleanup; }
		
	// the file doesn't have a corresponding SStSoundData* so import it on the fly
	
	// get a pointer to the directory array
	dir_array = (BFtFileRef**)UUrMemory_Array_GetMemory(ws->dir_array);
	UUmAssert(dir_array);
	
	current_directory = dir_array[(UUrMemory_Array_GetUsedElems(ws->dir_array) - 1)];
	
	// create the file ref
	error =	BFrFileRef_DuplicateAndAppendName(current_directory, selected_file_name, &file_ref);
	if (error != UUcError_None) { goto cleanup; }
	
	// create a new sound data
	error = SSrSoundData_New(file_ref, &ws->selected_sound_data);
	if (error != UUcError_None) { goto cleanup; }
	
cleanup:
	// delete the file ref
	if (file_ref)
	{
		UUrMemory_Block_Delete(file_ref);
		file_ref = NULL;
	}
	
	return error;
}

// ----------------------------------------------------------------------
static void
OWiWS_FillListbox(
	WMtDialog					*inDialog,
	BFtFileRef					*inFileRef)
{
	WMtListBox_DirectoryInfo	directory_info;
	WMtWindow					*listbox;

	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcWS_LB_DirContents);
	UUmAssert(listbox);
	
	// initialize the directory info
	directory_info.directory_ref = inFileRef;
	directory_info.flags = (LBcDirectoryInfoFlag_Files | LBcDirectoryInfoFlag_Directory);
	directory_info.prefix[0] = '\0';
	UUrString_Copy(directory_info.suffix, ".wav", BFcMaxFileNameLength);
	
	// set the directory of the listbox
	WMrMessage_Send(
		listbox,
		LBcMessage_SetDirectoryInfo,
		(UUtUns32)&directory_info,
		(UUtUns32)UUcTrue);
	
	// add .aif files to listbox
	directory_info.flags = LBcDirectoryInfoFlag_Files;
	UUrString_Copy(directory_info.suffix, ".aif", BFcMaxFileNameLength);
	WMrMessage_Send(
		listbox,
		LBcMessage_SetDirectoryInfo,
		(UUtUns32)&directory_info,
		(UUtUns32)UUcFalse);
	
	// add .aif files to listbox
	directory_info.flags = LBcDirectoryInfoFlag_Files;
	UUrString_Copy(directory_info.suffix, ".aiff", BFcMaxFileNameLength);
	WMrMessage_Send(
		listbox,
		LBcMessage_SetDirectoryInfo,
		(UUtUns32)&directory_info,
		(UUtUns32)UUcFalse);

	// select the first item in the list
	WMrMessage_Send(listbox, LBcMessage_SetSelection, (UUtUns32)UUcTrue, 0);

	// set the focus on the listbox
	WMrWindow_SetFocus(listbox);
}

// ----------------------------------------------------------------------
static void
OWiWS_EnableButtons(
	WMtDialog					*inDialog,
	WMtWindow					*inListBox)
{
	UUtUns32					item_data;
	UUtBool						can_select;	
	WMtWindow					*button;
	
	can_select = UUcFalse;
	
	// get the data of the currently selected item
	item_data = WMrMessage_Send(inListBox, LBcMessage_GetItemData, 0, (UUtUns32)-1);
	if (item_data == LBcDirItemType_File) { can_select = UUcTrue; }
	
	// enable or disable the select button depending on the type of
	// item currently selected
	button = WMrDialog_GetItemByID(inDialog, OWcWS_Btn_Select);
	UUmAssert(button);
	WMrWindow_SetEnabled(button, can_select);
}

// ----------------------------------------------------------------------
static UUtError
OWiWS_SelectItem(
	WMtDialog					*inDialog,
	WMtWindow					*inListBox)
{
	UUtError					error;
	OWtWS						*ws;
	UUtUns32					item_data;
	
	// get a pointer to the user data
	ws = (OWtWS*)WMrDialog_GetUserData(inDialog);
	UUmAssert(ws);
	
	// get the data of the currently selected item
	item_data = WMrMessage_Send(inListBox, LBcMessage_GetItemData, 0, (UUtUns32)-1);
	if (item_data == LBcDirItemType_File)
	{
		// get the selected sound data file and end the dialog
		OWiWS_SelectSoundData(inDialog);
		WMrDialog_ModalEnd(inDialog, OWcWS_Btn_Select);
	}
	else if (item_data == LBcDirItemType_Directory)
	{
		BFtFileRef				**dir_array;
		BFtFileRef				*directory_ref;
		WMtWindow				*popup;
		WMtMenuItemData			item_data;
		char					selected_dir_name[BFcMaxFileNameLength];
		UUtUns32				index;
		UUtBool					mem_moved;
		UUtUns32				num_elements;
		
		// get a pointer to the dir_array
		dir_array = UUrMemory_Array_GetMemory(ws->dir_array);
		UUmAssert(dir_array);
		
		// get the number of elements in the array
		num_elements = UUrMemory_Array_GetUsedElems(ws->dir_array);
		
		// get the selected directories name
		WMrMessage_Send(inListBox, LBcMessage_GetText, (UUtUns32)selected_dir_name, (UUtUns32)-1);
		
		// build a directory ref to the selected directory
		error = 
			BFrFileRef_DuplicateAndAppendName(
				dir_array[(num_elements - 1)],
				selected_dir_name,
				&directory_ref);
		UUmError_ReturnOnError(error);
		
		// add the directory_ref to the dir_array
		error = UUrMemory_Array_GetNewElement(ws->dir_array, &index, &mem_moved);
		UUmError_ReturnOnError(error);
		
		if (mem_moved)
		{
			// get a pointer to the dir_array
			dir_array = UUrMemory_Array_GetMemory(ws->dir_array);
			UUmAssert(dir_array);
		}
		
		dir_array[index] = directory_ref;
		
		// get a pointer to the popup menu
		popup = WMrDialog_GetItemByID(inDialog, OWcWS_PM_Directory);
		UUmAssert(popup);
		
		// set the item data
		item_data.flags = WMcMenuItemFlag_Enabled;
		item_data.id = (UUtUns16)index;
		UUrString_Copy(
			item_data.title,
			BFrFileRef_GetLeafName(directory_ref),
			BFcMaxFileNameLength);
		
		// put the directory name into the popup
		error = WMrPopupMenu_AppendItem(popup, &item_data);
		UUmError_ReturnOnError(error);
		
		error = WMrPopupMenu_SetSelection(popup, (UUtUns16)index);
		UUmError_ReturnOnError(error);
		
		// set the focus on the listbox
		WMrWindow_SetFocus(inListBox);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OWiWS_InitDialog(
	WMtDialog					*inDialog)
{
	UUtError					error;
	OWtWS						*ws;
	BFtFileRef					**dir_array;
	BFtFileRef					*directory_ref;
	WMtWindow					*popup;
	WMtMenuItemData				item_data;
	UUtUns32					index;
	
	// get the user data
	ws = (OWtWS*)WMrDialog_GetUserData(inDialog);
	if (ws == NULL) { goto cleanup; }
	
	// init ws
	ws->selected_sound_data = NULL;
	ws->dir_array =
		UUrMemory_Array_New(
			sizeof(BFtFileRef*),
			10,
			0,
			1);
	if (ws->dir_array == NULL) { goto cleanup; }
	
	// get the sound directory ref
	error = SSrGetSoundDirectory(&directory_ref);
	if (error != UUcError_None)
	{
		WMrDialog_MessageBox(
			NULL,
			"Error",
			"Unable to find the sound data folder, you cannot edit sounds.",
			WMcMessageBoxStyle_OK);

		goto cleanup;
	}
	
	// add an element to the array
	error = UUrMemory_Array_GetNewElement(ws->dir_array, &index, NULL);
	if (error != UUcError_None) { goto cleanup; }
	
	// get a pointer to the dir array
	dir_array = (BFtFileRef**)UUrMemory_Array_GetMemory(ws->dir_array);
	UUmAssert(dir_array);
	
	// add the directory_ref to the dir_array
	dir_array[index] = directory_ref;
	
	// get a pointer to the popup menu
	popup = WMrDialog_GetItemByID(inDialog, OWcWS_PM_Directory);
	UUmAssert(popup);
	
	// set the item data
	item_data.flags = WMcMenuItemFlag_Enabled;
	item_data.id = 0;
	UUrString_Copy(
		item_data.title,
		BFrFileRef_GetLeafName(directory_ref),
		BFcMaxFileNameLength);
	
	// put the directory name into the popup
	error = WMrPopupMenu_AppendItem(popup, &item_data);
	if (error != UUcError_None) { goto cleanup; }
	
	error = WMrPopupMenu_SetSelection(popup, 0);
	if (error != UUcError_None) { goto cleanup; }
	
	return;
	
cleanup:
	WMrDialog_ModalEnd(inDialog, OWcWS_Btn_Cancel);
}

// ----------------------------------------------------------------------
static void
OWiWS_Destroy(
	WMtDialog					*inDialog)
{
	OWtWS						*ws;

	// get a pointer to the user data
	ws = (OWtWS*)WMrDialog_GetUserData(inDialog);
	
	// delete the dir_array
	if (ws->dir_array)
	{
		BFtFileRef				**dir_array;
		UUtUns32				num_elements;
		UUtUns32				i;
		
		num_elements = UUrMemory_Array_GetUsedElems(ws->dir_array);
		dir_array = (BFtFileRef**)UUrMemory_Array_GetMemory(ws->dir_array);
		
		// delete the BFtFileRefs
		for (i = 0; i < num_elements; i++)
		{
			UUrMemory_Block_Delete(dir_array[i]);
			dir_array[i] = NULL;
		}
		
		// delete the memory array
		UUrMemory_Array_Delete(ws->dir_array);
		ws->dir_array = NULL;
	}
}

// ----------------------------------------------------------------------
static void
OWiWS_HandleCommand(
	WMtDialog					*inDialog,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	UUtUns16					command_type;
	UUtUns16					control_id;
	
	// interpret inParam1
	command_type = UUmHighWord(inParam1);
	control_id = UUmLowWord(inParam1);

	switch (control_id)
	{
		case OWcWS_Btn_Select:
			OWiWS_SelectSoundData(inDialog);
			WMrDialog_ModalEnd(inDialog, OWcWS_Btn_Select);
		break;
		
		case OWcWS_Btn_Cancel:
			WMrDialog_ModalEnd(inDialog, OWcWS_Btn_Cancel);
		break;
		
		case OWcWS_LB_DirContents:
			if (command_type == LBcNotify_SelectionChanged)
			{
				OWiWS_EnableButtons(inDialog, (WMtWindow*)inParam2);
			}
			else if (command_type == WMcNotify_DoubleClick)
			{
				OWiWS_SelectItem(inDialog, (WMtWindow*)inParam2);
			}
		break;
	}
}

// ----------------------------------------------------------------------
static void
OWiWS_HandleMenuCommand(
	WMtDialog					*inDialog,
	WMtWindow					*inPopupMenu,
	UUtUns32					inItemID)
{
	OWtWS						*ws;
	UUtUns32					i;
	UUtUns32					num_dirs;
	BFtFileRef					**dir_array;
	UUtBool						mem_moved;
	
	// set the current depth
	ws = (OWtWS*)WMrDialog_GetUserData(inDialog);
	
	// get a pointer to the directory array
	dir_array = (BFtFileRef**)UUrMemory_Array_GetMemory(ws->dir_array);
	
	// delete the menu items after inItemID
	num_dirs = UUrMemory_Array_GetUsedElems(ws->dir_array);
	for (i = (inItemID + 1); i < num_dirs; i++)
	{
		// remove the directory from the popup menu
		WMrPopupMenu_RemoveItem(inPopupMenu, (UUtUns16)i);
		
		// delete the BFtFileRef
		UUrMemory_Block_Delete(dir_array[i]);
		dir_array[i] = NULL;
	}
	
	// remove the entry from the directory array
	UUrMemory_Array_SetUsedElems(ws->dir_array, (inItemID + 1), &mem_moved);

	// fill the listbox with the topmost directory
	OWiWS_FillListbox(inDialog, dir_array[inItemID]);
}

// ----------------------------------------------------------------------
static UUtBool
OWiWS_Callback(
	WMtDialog					*inDialog,
	WMtMessage					inMessage,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	UUtBool						handled;
	
	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiWS_InitDialog(inDialog);
		break;
		
		case WMcMessage_Destroy:
			OWiWS_Destroy(inDialog);
		break;
		
		case WMcMessage_Command:
			OWiWS_HandleCommand(inDialog, inParam1, inParam2);
		break;
		
		case WMcMessage_MenuCommand:
			OWiWS_HandleMenuCommand(
				inDialog,
				(WMtWindow*)inParam2,
				(UUtUns32)UUmLowWord(inParam1));
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
OWiSPP_SaveFields(
	WMtWindow					*inDialog)
{
	SStPermutation				*perm;
	WMtWindow					*editfield;
	char						string[128];
	UUtUns32					weight;
	float						min_volume_percent;
	float						max_volume_percent;
	float						min_pitch_percent;
	float						max_pitch_percent;
		
	// get the fields
	editfield = WMrDialog_GetItemByID(inDialog, OWcSPP_EF_Weight);
	WMrMessage_Send(editfield, EFcMessage_GetText, (UUtUns32)string, 128);
	sscanf(string, "%d", &weight);
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcSPP_EF_MinVol);
	WMrMessage_Send(editfield, EFcMessage_GetText, (UUtUns32)string, 128);
	sscanf(string, "%f", &min_volume_percent);

	editfield = WMrDialog_GetItemByID(inDialog, OWcSPP_EF_MaxVol);
	WMrMessage_Send(editfield, EFcMessage_GetText, (UUtUns32)string, 128);
	sscanf(string, "%f", &max_volume_percent);

	editfield = WMrDialog_GetItemByID(inDialog, OWcSPP_EF_MinPitch);
	WMrMessage_Send(editfield, EFcMessage_GetText, (UUtUns32)string, 128);
	sscanf(string, "%f", &min_pitch_percent);

	editfield = WMrDialog_GetItemByID(inDialog, OWcSPP_EF_MaxPitch);
	WMrMessage_Send(editfield, EFcMessage_GetText, (UUtUns32)string, 128);
	sscanf(string, "%f", &max_pitch_percent);
	
	// check ranges
	if (min_volume_percent > max_volume_percent)
	{
		WMrDialog_MessageBox(
			inDialog,
			"Error",
			"The Minimum Volume must be less than or equal to the Maximum Volume.",
			WMcMessageBoxStyle_OK);
		WMrWindow_SetFocus(WMrDialog_GetItemByID(inDialog, OWcSPP_EF_MinVol));
		return UUcFalse;
	}
	if (min_pitch_percent > max_pitch_percent)
	{
		WMrDialog_MessageBox(
			inDialog,
			"Error",
			"The Minimum Pitch must be less than or equal to the Maximum Pitch.",
			WMcMessageBoxStyle_OK);
		WMrWindow_SetFocus(WMrDialog_GetItemByID(inDialog, OWcSPP_EF_MinPitch));
		return UUcFalse;
	}
	
	// get a pointer to the permutation
	perm = (SStPermutation*)WMrDialog_GetUserData(inDialog);
	UUmAssert(perm);
	
	// save the data
	perm->weight = weight;
	perm->min_volume_percent = min_volume_percent;
	perm->max_volume_percent = max_volume_percent;
	perm->min_pitch_percent = min_pitch_percent;
	perm->max_pitch_percent = max_pitch_percent;
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
static void
OWiSPP_InitDialog(
	WMtDialog					*inDialog)
{
	SStPermutation				*perm;
	WMtWindow					*text;
	WMtWindow					*editfield;
	const char					*name;
	char						string[128];
	
	// get a pointer to the permutation
	perm = (SStPermutation*)WMrDialog_GetUserData(inDialog);
	if (perm == NULL)
	{
		WMrDialog_ModalEnd(inDialog, OWcSPP_Btn_Cancel);
		return;
	}
	
	// set the name
	name = SSrPermutation_GetName(perm);
	text = WMrDialog_GetItemByID(inDialog, OWcSPP_Txt_Name);
	UUmAssert(text);
	WMrWindow_SetTitle(text, name, BFcMaxFileNameLength);
	
	// set the edit fields
	editfield = WMrDialog_GetItemByID(inDialog, OWcSPP_EF_Weight);
	sprintf(string, "%d", perm->weight);
	WMrMessage_Send(editfield, EFcMessage_SetText, (UUtUns32)string, 0);
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcSPP_EF_MinVol);
	sprintf(string, "%1.2f", perm->min_volume_percent);
	WMrMessage_Send(editfield, EFcMessage_SetText, (UUtUns32)string, 0);

	editfield = WMrDialog_GetItemByID(inDialog, OWcSPP_EF_MaxVol);
	sprintf(string, "%1.2f", perm->max_volume_percent);
	WMrMessage_Send(editfield, EFcMessage_SetText, (UUtUns32)string, 0);

	editfield = WMrDialog_GetItemByID(inDialog, OWcSPP_EF_MinPitch);
	sprintf(string, "%1.2f", perm->min_pitch_percent);
	WMrMessage_Send(editfield, EFcMessage_SetText, (UUtUns32)string, 0);

	editfield = WMrDialog_GetItemByID(inDialog, OWcSPP_EF_MaxPitch);
	sprintf(string, "%1.2f", perm->max_pitch_percent);
	WMrMessage_Send(editfield, EFcMessage_SetText, (UUtUns32)string, 0);
}

// ----------------------------------------------------------------------
static void
OWiSPP_HandleCommand(
	WMtDialog					*inDialog,
	UUtUns32					inParam1)
{
	switch(UUmLowWord(inParam1))
	{
		case OWcSPP_Btn_Save:
			// save the data
			if (OWiSPP_SaveFields(inDialog) == UUcTrue)
			{
				WMrDialog_ModalEnd(inDialog, OWcSPP_Btn_Save);
			}
		break;
		
		case OWcSPP_Btn_Cancel:
			WMrDialog_ModalEnd(inDialog, OWcSPP_Btn_Cancel);
		break;
	}
}

// ----------------------------------------------------------------------
static UUtBool
OWiSPP_Callback(
	WMtDialog					*inDialog,
	WMtMessage					inMessage,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	UUtBool						handled;
	
	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiSPP_InitDialog(inDialog);
		break;
		
		case WMcMessage_Command:
			OWiSPP_HandleCommand(inDialog, inParam1);
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OWiSGP_EnableButtons(
	WMtDialog					*inDialog,
	OWtSGP						*inSGP)
{
	WMtWindow					*button;
	UUtBool						permutation_selected;
	UUtBool						can_save;
	
	// get a pointer to the listbox
	permutation_selected = UUcFalse;
	if (SSrGroup_GetNumPermutations(inSGP->group) > 0)
	{
		WMtWindow				*listbox;

		listbox = WMrDialog_GetItemByID(inDialog, OWcSGP_LB_Permutations);
		permutation_selected =
			(WMrMessage_Send(listbox, LBcMessage_GetSelection, 0, 0) != LBcError);
	}
	
	// update the buttons
	can_save =
		OSrCollection_IsLocked(OSrCollection_GetByType(OScCollectionType_Group)) == UUcFalse;
	
	button = WMrDialog_GetItemByID(inDialog, OWcSGP_Btn_AddPerm);
	WMrWindow_SetEnabled(button, can_save);
	
	button = WMrDialog_GetItemByID(inDialog, OWcSGP_Btn_EditPerm);
	WMrWindow_SetEnabled(button, can_save && permutation_selected);
	
	button = WMrDialog_GetItemByID(inDialog, OWcSGP_Btn_PlayPerm);
	WMrWindow_SetEnabled(button, can_save && permutation_selected);
	
	button = WMrDialog_GetItemByID(inDialog, OWcSGP_Btn_DeletePerm);
	WMrWindow_SetEnabled(button, can_save && permutation_selected);
}

// ----------------------------------------------------------------------
static void
OWiSGP_SaveGroup(
	WMtDialog					*inDialog,
	OWtSGP						*inSGP)
{
	char						name[OScMaxNameLength];
	SStGroup					*item_group;
	
	// get the name from the editfield
	WMrMessage_Send(
		WMrDialog_GetItemByID(inDialog, OWcSGP_EF_Name),
		EFcMessage_GetText,
		(UUtUns32)name,
		OScMaxNameLength);
	
	// save the item name
	OSrCollection_Item_SetName(
		inSGP->collection,
		OSrItem_GetID(inSGP->item),
		name);
	
	// save the group data
	item_group = SSrGroup_GetByID(OSrItem_GetID(inSGP->item));
	UUmAssert(inSGP->group->id == inSGP->group_id);
	inSGP->group = SSrGroup_GetByID(inSGP->group_id);	// make sure group is an up to date pointer
	SSrGroup_Copy(inSGP->group, item_group);
}

// ----------------------------------------------------------------------
static void
OWiSGP_SetFields(
	WMtDialog					*inDialog,
	OWtSGP						*inSGP)
{
	WMtWindow					*editfield;
	WMtWindow					*listbox;
	UUtUns32					i;
	
	// set the name
	editfield = WMrDialog_GetItemByID(inDialog, OWcSGP_EF_Name);
	WMrMessage_Send(
		editfield,
		EFcMessage_SetText,
		(UUtUns32)inSGP->item_name,
		0);
	
	// reset the permutations list box
	listbox = WMrDialog_GetItemByID(inDialog, OWcSGP_LB_Permutations);
	WMrMessage_Send(listbox, LBcMessage_Reset, 0, 0);
	
	// fill in the listbox
	for (i = 0; i < SSrGroup_GetNumPermutations(inSGP->group); i++)
	{
		WMrMessage_Send(listbox, LBcMessage_AddString, i, 0);
	}
	
	// select the first item
	WMrMessage_Send(listbox, LBcMessage_SetSelection, (UUtUns32)UUcTrue, 0);
}

// ----------------------------------------------------------------------
static UUtError
OWiSGP_AddPermutation(
	WMtDialog					*inDialog,
	OWtSGP						*inSGP)
{
	UUtError					error;
	UUtUns32					message;
	UUtUns32					perm_index;
	SStPermutation				*perm;
	OWtWS						ws;
	
	UUmAssert(inDialog);
	
	// select the wav file to use
	error =
		WMrDialog_ModalBegin(
			OWcDialog_WAV_Select,
			inDialog,
			OWiWS_Callback,
			(UUtUns32)&ws,
			&message);
	UUmError_ReturnOnError(error);
	if (message == OWcWS_Btn_Cancel) { return UUcError_None; }
	
	// add the permutation to the group
	error =
		SSrGroup_Permutation_New(
			inSGP->group,
			ws.selected_sound_data,
			&perm_index);
	if (error != UUcError_None)
	{
		if (SSrGroup_GetNumChannels(inSGP->group) != ws.selected_sound_data->num_channels)
		{
			WMrDialog_MessageBox(
				inDialog,
				"Error",
				"The permutations of a sound group must have the same number of channels.",
				WMcMessageBoxStyle_OK);
		}
		return error;
	}
	
	// get a pointer to the permutation
	perm = SSrGroup_Permutation_Get(inSGP->group, perm_index);
	UUmAssert(perm);
	
	// edit the permutation
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Sound_Perm_Prop,
			inDialog,
			OWiSPP_Callback,
			(UUtUns32)perm,
			&message);
	UUmError_ReturnOnError(error);
	
	if (message == OWcSPP_Btn_Save)
	{
		WMtWindow				*listbox;
		
		// update the dialog's fields
		OWiSGP_SetFields(inDialog, inSGP);
		
		// get a pointer to the listbox
		listbox = WMrDialog_GetItemByID(inDialog, OWcSGP_LB_Permutations);
		UUmAssert(listbox);
		
		// select the permutation that was just added
		WMrMessage_Send(listbox, LBcMessage_SetSelection, (UUtUns32)UUcTrue, perm_index);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OWiSGP_EditPermutation(
	WMtDialog					*inDialog,
	OWtSGP						*inSGP)
{
	UUtError					error;
	WMtWindow					*listbox;
	SStPermutation				*perm;
	UUtUns32					perm_index;
	UUtUns32					message;
	
	UUmAssert(inDialog);
	
	if (SSrGroup_GetNumPermutations(inSGP->group) == 0) { return UUcError_None; }
	
	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcSGP_LB_Permutations);
	UUmAssert(listbox);
	
	// get the selected item id
	perm_index = WMrMessage_Send(listbox, LBcMessage_GetSelection, 0, 0);
	UUmAssert(perm_index < SSrGroup_GetNumPermutations(inSGP->group));
	
	// get a pointer to the permutation
	perm = SSrGroup_Permutation_Get(inSGP->group, perm_index);
	UUmAssert(perm);
	
	// edit the permutation
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Sound_Perm_Prop,
			inDialog,
			OWiSPP_Callback,
			(UUtUns32)perm,
			&message);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OWiSGP_PlayPermutation(
	WMtDialog					*inDialog,
	OWtSGP						*inSGP)
{
	WMtWindow					*listbox;
	UUtUns32					perm_index;
	SStPermutation				*perm;
	
	UUmAssert(inDialog);
	
	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcSGP_LB_Permutations);
	UUmAssert(listbox);
	
	// get the selected item id
	perm_index = WMrMessage_Send(listbox, LBcMessage_GetSelection, 0, 0);
	UUmAssert(perm_index < SSrGroup_GetNumPermutations(inSGP->group));
		
	// get a pointer to the permutation
	perm = SSrGroup_Permutation_Get(inSGP->group, perm_index);
	UUmAssert(perm);
	
	// play the permutation
	SSrPermutation_Play(perm, NULL);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OWiSGP_DeletePermutation(
	WMtDialog					*inDialog,
	OWtSGP						*inSGP)
{
	WMtWindow					*listbox;
	UUtUns32					perm_index;
	UUtUns32					message;
	
	UUmAssert(inDialog);
	
	// ask the user to confirm delete
	message =
		WMrDialog_MessageBox(
			inDialog,
			"Confirm Permutation Deletion",
			"Are you sure you want to delete the selected permutation?",
			WMcMessageBoxStyle_Yes_No);
	if (message == WMcDialogItem_No) { return UUcError_None; }
	
	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcSGP_LB_Permutations);
	UUmAssert(listbox);
	
	// get the selected permutation's index
	perm_index = WMrMessage_Send(listbox, LBcMessage_GetSelection, 0, 0);
	UUmAssert(perm_index < SSrGroup_GetNumPermutations(inSGP->group));
	
	// delete the permutation from the group
	SSrGroup_Permutation_Delete(inSGP->group, perm_index);
	
	// select the permutation
	WMrMessage_Send(listbox, LBcMessage_SetSelection, (UUtUns32)UUcTrue, 0);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OWiSGP_InitDialog(
	WMtDialog					*inDialog)
{
	OStItem						*item;
	OWtSGP						*sgp;
	WMtWindow					*editfield;
	UUtError					error;
	SStGroup					*item_group;
	
	// get the item
	item = (OStItem*)WMrDialog_GetUserData(inDialog);
	if (item == NULL)
	{
		WMrDialog_ModalEnd(inDialog, OWcSGP_Btn_Cancel);
		return;
	}
	
	// allocate memory for the OWtSGP
	sgp = (OWtSGP*)UUrMemory_Block_New(sizeof(OWtSGP));
	if (sgp == NULL)
	{
		WMrDialog_ModalEnd(inDialog, OWcSGP_Btn_Cancel);
		return;
	}
	
	// initialize the sgp
	sgp->collection = OSrCollection_GetByType(OScCollectionType_Group);
	sgp->item = item;
	UUrString_Copy(sgp->item_name, OSrItem_GetName(item), OScMaxNameLength);
	error = SSrGroup_New(&sgp->group_id, &sgp->group);
	if (error != UUcError_None)
	{
		UUrMemory_Block_Delete(sgp);
		WMrDialog_ModalEnd(inDialog, OWcSGP_Btn_Cancel);
		return;
	}
	
	// copy the data from the item's sound group to the sgp group
	UUmAssert(sgp->group->id == sgp->group_id);
	item_group = SSrGroup_GetByID(OSrItem_GetID(item));
	sgp->group = SSrGroup_GetByID(sgp->group_id);			// must update the group pointer
	UUmAssert(sgp->group->id == sgp->group_id);
	error = 
		SSrGroup_Copy(
			item_group,
			sgp->group);
	UUmAssert(sgp->group->id == sgp->group_id);
	if (error != UUcError_None)
	{
		UUrMemory_Block_Delete(sgp);
		WMrDialog_ModalEnd(inDialog, OWcSGP_Btn_Cancel);
		return;
	}
	
	// store the sgp in the user data
	WMrDialog_SetUserData(inDialog, (UUtUns32)sgp);
	
	// fill the editfield and the listbox 
	OWiSGP_SetFields(inDialog, sgp);
	
	// enable button because new sound group's won't have permutations
	OWiSGP_EnableButtons(inDialog, sgp);
	
	// get a pointer to the edit field
	editfield = WMrDialog_GetItemByID(inDialog, OWcSGP_EF_Name);
	UUmAssert(editfield);
	
	// set a limit on the length of the name
	WMrMessage_Send(editfield, EFcMessage_SetMaxChars, OScMaxNameLength, 0);
	
	// make the edit field the text focus
	WMrWindow_SetFocus(editfield);
}

// ----------------------------------------------------------------------
static void
OWiSGP_Destroy(
	WMtDialog					*inDialog)
{
	OWtSGP						*sgp;

	// get the sgp
	sgp = (OWtSGP*)WMrDialog_GetUserData(inDialog);
	if (sgp == NULL) { return; }
	
	// delete the allocated SStGroup
	SSrGroup_Delete(sgp->group_id);
	sgp->group_id = SScInvalidID;
	sgp->group = NULL;
	
	// delete the memory used by the sgp
	UUrMemory_Block_Delete(sgp);
	WMrDialog_SetUserData(inDialog, 0);
}

// ----------------------------------------------------------------------
static void
OWiSGP_HandleCommand(
	WMtDialog					*inDialog,
	UUtUns32					inParam1,
	WMtWindow					*inControl)
{
	UUtUns16					command_type;
	UUtUns16					control_id;
	OWtSGP						*sgp;
	
	// interpret inParam1
	command_type = UUmHighWord(inParam1);
	control_id = UUmLowWord(inParam1);
	
	// get the sgp
	sgp = (OWtSGP*)WMrDialog_GetUserData(inDialog);
	if (sgp == NULL)
	{
		WMrDialog_ModalEnd(inDialog, OWcSGP_Btn_Cancel);
		return;
	}
	
	if (control_id == OWcSGP_LB_Permutations)
	{
		if (command_type == LBcNotify_SelectionChanged)
		{
			OWiSGP_EnableButtons(inDialog, sgp);
		}
		else if (command_type == WMcNotify_DoubleClick)
		{
			OWiSGP_EditPermutation(inDialog, sgp);
		}
	}
	else if (command_type == WMcNotify_Click)
	{
		switch (control_id)
		{
			case OWcSGP_Btn_Save:
			{
				char				name[OScMaxNameLength];
				
				WMrMessage_Send(
					WMrDialog_GetItemByID(inDialog, OWcSGP_EF_Name),
					EFcMessage_GetText,
					(UUtUns32)name,
					OScMaxNameLength);
				if (TSrString_GetLength(name) == 0)
				{
					WMrDialog_MessageBox(
						inDialog,
						"Error",
						"You must supply a name for the sound group.",
						WMcMessageBoxStyle_OK);
					
					WMrWindow_SetFocus(WMrDialog_GetItemByID(inDialog, OWcSGP_EF_Name));
				}
				else
				{
					OWiSGP_SaveGroup(inDialog, sgp);
					WMrDialog_ModalEnd(inDialog, OWcSGP_Btn_Save);
				}
			}
			break;
			
			case OWcSGP_Btn_Cancel:
				WMrDialog_ModalEnd(inDialog, OWcSGP_Btn_Cancel);
			break;
			
			case OWcSGP_Btn_AddPerm:
				OWiSGP_AddPermutation(inDialog, sgp);
			break;
			
			case OWcSGP_Btn_EditPerm:
				OWiSGP_EditPermutation(inDialog, sgp);
			break;
			
			case OWcSGP_Btn_PlayPerm:
				OWiSGP_PlayPermutation(inDialog, sgp);
			break;
			
			case OWcSGP_Btn_DeletePerm:
				OWiSGP_DeletePermutation(inDialog, sgp);
			break;
			
			case OWcSGP_Btn_Play:
				SSrGroup_Play(sgp->group, NULL);
			break;
		}
	}
	else if (command_type == EFcNotify_ContentChanged)
	{
		switch (control_id)
		{
			case OWcSGP_EF_Name:
				WMrMessage_Send(
					inControl,
					EFcMessage_GetText,
					(UUtUns32)sgp->item_name,
					OScMaxNameLength);
			break;
		}
	}
}

// ----------------------------------------------------------------------
static void
OWiSGP_HandleDrawItem(
	WMtDialog					*inDialog,
	WMtWindow					*inListBox,
	WMtDrawItem					*inDrawItem)
{
	OWtSGP						*sgp;
	IMtPoint2D					dest;
//	PStPartSpecUI				*partspec_ui;
	UUtInt16					line_width;
	UUtInt16					line_height;
	SStPermutation				*perm;
	char						string[128];
	
	// get a pointer to the sgp
	sgp = (OWtSGP*)WMrDialog_GetUserData(inDialog);
	UUmAssert(sgp);
	
	// get a pointer to the permutation
	perm = SSrGroup_Permutation_Get(sgp->group, inDrawItem->data);
	if (perm == NULL) { return; }
	
	// get a pointer to the partspec_ui
//	partspec_ui = PSrPartSpecUI_GetActive();
//	UUmAssert(partspec_ui);
	
	// set the dest
	dest.x = inDrawItem->rect.left;
	dest.y = inDrawItem->rect.top;
	
	// calc the width and height of the line
	line_width = inDrawItem->rect.right - inDrawItem->rect.left;
	line_height = inDrawItem->rect.bottom - inDrawItem->rect.top;
	
/*	if (inDrawItem->state & WMcDrawItemState_Selected)
	{
		// draw the hilite
		DCrDraw_PartSpec(
			inDrawItem->draw_context,
			partspec_ui->hilite,
			PScPart_All,
			&dest,
			line_width,
			line_height,
			M3cMaxAlpha);
	}*/
	
	// setup the text drawing
	DCrText_SetFormat(TSc_HLeft | TSc_VCenter);
	DCrText_SetStyle(TScStyle_Plain);
	DCrText_SetShade(IMcShade_Black);
	
	// draw the perm's sound_data instance name
	dest.x = 4;
	DCrDraw_String(
		inDrawItem->draw_context,
		SSrPermutation_GetName(perm),
		&inDrawItem->rect,
		&dest);
	
	// draw the perm's weight
	dest.x = 160;
	sprintf(string, "%d", perm->weight);
	DCrDraw_String(inDrawItem->draw_context, string, &inDrawItem->rect, &dest);
	
	// draw the perm's min_volume_percent 
	dest.x = 200;
	sprintf(string, "%1.2f", perm->min_volume_percent);
	DCrDraw_String(inDrawItem->draw_context, string, &inDrawItem->rect, &dest);
	
	// draw the perm's max_volume_percent 
	dest.x = 230;
	sprintf(string, "%1.2f", perm->max_volume_percent);
	DCrDraw_String(inDrawItem->draw_context, string, &inDrawItem->rect, &dest);

	// draw the perm's min_pitch_percent 
	dest.x = 265;
	sprintf(string, "%1.2f", perm->min_pitch_percent);
	DCrDraw_String(inDrawItem->draw_context, string, &inDrawItem->rect, &dest);
	
	// draw the perm's max_volume_percent 
	dest.x = 295;
	sprintf(string, "%1.2f", perm->max_pitch_percent);
	DCrDraw_String(inDrawItem->draw_context, string, &inDrawItem->rect, &dest);
}

// ----------------------------------------------------------------------
static UUtBool
OWiSGP_Callback(
	WMtDialog					*inDialog,
	WMtMessage					inMessage,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	UUtBool						handled;
	
	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiSGP_InitDialog(inDialog);
		break;
		
		case WMcMessage_Destroy:
			OWiSGP_Destroy(inDialog);
		break;
		
		case WMcMessage_Command:
			OWiSGP_HandleCommand(inDialog, inParam1, (WMtWindow*)inParam2);
		break;
		
		case WMcMessage_DrawItem:
			OWiSGP_HandleDrawItem(inDialog, (WMtWindow*)inParam1, (WMtDrawItem*)inParam2);
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OWiSetGroupText(
	WMtDialog					*inDialog,
	UUtUns32					inDialogItemID,
	SStGroup					**ioSoundGroup,
	const char					*inSoundGroupName)
{
	WMtWindow					*text;
	char						string[1024];
	UUtUns32					message;
	
	text = WMrDialog_GetItemByID(inDialog, (UUtUns16)inDialogItemID);
	WMrWindow_SetTitle(text, "", OScMaxNameLength);
	
	if (*ioSoundGroup)
	{
		OStItem					*item;
		
		item =
			OSrCollection_Item_GetByID(
				OSrCollection_GetByType(OScCollectionType_Group),
				(*ioSoundGroup)->id);
		if (item)
		{
			// set the group text to the name of the item
			WMrWindow_SetTitle(text, OSrItem_GetName(item), OScMaxNameLength);
		}
		else
		{
			// no sound group exists for this sound_group
			*ioSoundGroup = NULL;

			sprintf(
				string,
				"The %s group no longer exists.  The %s group is being cleared.",
				inSoundGroupName,
				inSoundGroupName);
			
			message =
				WMrDialog_MessageBox(
					inDialog,
					"Error",
					string,
					WMcMessageBoxStyle_OK);
		}
	}
}

// ----------------------------------------------------------------------
static UUtError
OWiEditGroup(
	WMtDialog					*inParentDialog,
	SStGroup					*inSoundGroup)
{
	UUtError					error;
	OStItem						*item;
	
	// get the item
	item =
		OSrCollection_Item_GetByID(
			OSrCollection_GetByType(OScCollectionType_Group),
			inSoundGroup->id);
	if (item == NULL) { return UUcError_Generic; }
	
	// edit the properties of the group
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Sound_Group_Prop,
			inParentDialog,
			OWiSGP_Callback,
			(UUtUns32)item,
			NULL);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OWiASP_EnableButtons(
	WMtDialog					*inDialog,
	OWtASP						*inASP)
{
	WMtWindow					*button;
	
	// set the buttons
	button = WMrDialog_GetItemByID(inDialog, OWcASP_Btn_InGroupEdit);
	WMrWindow_SetEnabled(button, (inASP->ambient->in_sound != NULL));

	button = WMrDialog_GetItemByID(inDialog, OWcASP_Btn_OutGroupEdit);
	WMrWindow_SetEnabled(button, (inASP->ambient->out_sound != NULL));

	button = WMrDialog_GetItemByID(inDialog, OWcASP_Btn_BT1Edit);
	WMrWindow_SetEnabled(button, (inASP->ambient->base_track1 != NULL));

	button = WMrDialog_GetItemByID(inDialog, OWcASP_Btn_BT2Edit);
	WMrWindow_SetEnabled(button, (inASP->ambient->base_track2 != NULL));

	button = WMrDialog_GetItemByID(inDialog, OWcASP_Btn_DetailEdit);
	WMrWindow_SetEnabled(button, (inASP->ambient->detail != NULL));
	
	button = WMrDialog_GetItemByID(inDialog, OWcASP_Btn_Start);
	WMrWindow_SetEnabled(button, (inASP->play_id == SScInvalidID));

	button = WMrDialog_GetItemByID(inDialog, OWcASP_Btn_Stop);
	WMrWindow_SetEnabled(button, (inASP->play_id != SScInvalidID));
}

// ----------------------------------------------------------------------
static UUtBool
OWiASP_SaveAmbient(
	WMtDialog					*inDialog,
	OWtASP						*inASP)
{
	char						name[OScMaxNameLength];
	SStAmbient					*item_ambient;
	UUtUns16					priority;
	
	// get the name from the editfield
	WMrMessage_Send(
		WMrDialog_GetItemByID(inDialog, OWcASP_EF_Name),
		EFcMessage_GetText,
		(UUtUns32)name,
		OScMaxNameLength);
	
	// get the priority from the popup menu
	WMrPopupMenu_GetItemID(
		WMrDialog_GetItemByID(inDialog, OWcASP_PM_Priority),
		-1,
		&priority);
	inASP->ambient->priority = (SStPriority2)priority;
	
	// save the item name
	OSrCollection_Item_SetName(inASP->collection, OSrItem_GetID(inASP->item), name);
	
	// check ranges
	if (inASP->ambient->min_detail_time > inASP->ambient->max_detail_time)
	{
		WMrDialog_MessageBox(
			inDialog,
			"Error",
			"The Minimum Detail Time must be less than or equal to the Maximum Detail Time.",
			WMcMessageBoxStyle_OK);
		WMrWindow_SetFocus(WMrDialog_GetItemByID(inDialog, OWcASP_EF_MinElapsedTime));
		return UUcFalse;
	}
	if (inASP->ambient->sphere_radius > inASP->ambient->min_volume_distance)
	{
		WMrDialog_MessageBox(
			inDialog,
			"Error",
			"The Sphere Radius must be less than or equal to the Minimum Volume Distance.",
			WMcMessageBoxStyle_OK);
		WMrWindow_SetFocus(WMrDialog_GetItemByID(inDialog, OWcASP_EF_SphereRadius));
		return UUcFalse;
	}
	if (inASP->ambient->max_volume_distance > inASP->ambient->min_volume_distance)
	{
		WMrDialog_MessageBox(
			inDialog,
			"Error",
			"The Maximum Volume Distance must be less than or equal to the Minimum Volume Distance.",
			WMcMessageBoxStyle_OK);
		WMrWindow_SetFocus(WMrDialog_GetItemByID(inDialog, OWcASP_EF_MaxVolDist));
		return UUcFalse;
	}
	
	// save the ambient data
	item_ambient = SSrAmbient_GetByID(OSrItem_GetID(inASP->item));
	SSrAmbient_Copy(inASP->ambient, item_ambient);
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
static void
OWiASP_SetFields(
	WMtDialog					*inDialog,
	OWtASP						*inASP)
{
	WMtWindow					*editfield;
	WMtWindow					*popup;
	WMtWindow					*checkbox;
	char						string[128];
	
	// set the ambient sound's name
	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_Name);
	WMrMessage_Send(editfield, EFcMessage_SetText, (UUtUns32)inASP->item_name, 0);
	
	// set the priority
	popup = WMrDialog_GetItemByID(inDialog, OWcASP_PM_Priority);
	WMrPopupMenu_SetSelection(popup, (UUtUns16)inASP->ambient->priority);
	
	// set the group names
	OWiSetGroupText(inDialog, OWcASP_Txt_InGroup, &inASP->ambient->in_sound, "In Sound");
	OWiSetGroupText(inDialog, OWcASP_Txt_OutGroup, &inASP->ambient->out_sound, "Out Sound");
	OWiSetGroupText(inDialog, OWcASP_Txt_BT1, &inASP->ambient->base_track1, "Base Track 1");
	OWiSetGroupText(inDialog, OWcASP_Txt_BT2, &inASP->ambient->base_track2, "Base Track 2");
	OWiSetGroupText(inDialog, OWcASP_Txt_Detail, &inASP->ambient->detail, "Detail");
	
	// set the interrupt tracks on stop checkbox
	checkbox = WMrDialog_GetItemByID(inDialog, OWcASP_CB_InterruptOnStop);
	WMrMessage_Send(
		checkbox,
		CBcMessage_SetCheck,
		(UUtUns32)((inASP->ambient->flags & SScAmbientFlag_InterruptOnStop) != 0),
		0);
	
	// set the play once checkbox
	checkbox = WMrDialog_GetItemByID(inDialog, OWcASP_CB_PlayOnce);
	WMrMessage_Send(
		checkbox,
		CBcMessage_SetCheck,
		(UUtUns32)((inASP->ambient->flags & SScAmbientFlag_PlayOnce) != 0),
		0);
	
	// set the minimum elapsed time
	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_MinElapsedTime);
	sprintf(string, "%5.3f", inASP->ambient->min_detail_time);
	WMrMessage_Send(editfield, EFcMessage_SetText, (UUtUns32)string, 0);
	
	// set the maximum elapse time
	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_MaxElapsedTime);
	sprintf(string, "%5.3f", inASP->ambient->max_detail_time);
	WMrMessage_Send(editfield, EFcMessage_SetText, (UUtUns32)string, 0);

	// set the sphere radius
	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_SphereRadius);
	sprintf(string, "%5.3f", inASP->ambient->sphere_radius);
	WMrMessage_Send(editfield, EFcMessage_SetText, (UUtUns32)string, 0);

	// set the maximum volume distance
	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_MaxVolDist);
	sprintf(string, "%5.3f", inASP->ambient->max_volume_distance);
	WMrMessage_Send(editfield, EFcMessage_SetText, (UUtUns32)string, 0);

	// set the minumum volume distance
	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_MinVolDist);
	sprintf(string, "%5.3f", inASP->ambient->min_volume_distance);
	WMrMessage_Send(editfield, EFcMessage_SetText, (UUtUns32)string, 0);
	
	OWiASP_EnableButtons(inDialog, inASP);
}

// ----------------------------------------------------------------------
static void
OWiASP_InitDialog(
	WMtDialog					*inDialog)
{
	OStItem						*item;
	OWtASP						*asp;
	UUtError					error;
	WMtWindow					*editfield;
	
	// get the item
	item = (OStItem*)WMrDialog_GetUserData(inDialog);
	if (item == NULL)
	{
		WMrDialog_ModalEnd(inDialog, OWcSGP_Btn_Cancel);
		return;
	}
	
	// allocate memory for the asp
	asp = (OWtASP*)UUrMemory_Block_New(sizeof(OWtSGP));
	if (asp == NULL)
	{
		WMrDialog_ModalEnd(inDialog, OWcASP_Btn_Cancel);
		return;
	}
	
	// initialize the asp
	asp->collection = OSrCollection_GetByType(OScCollectionType_Ambient);
	asp->item = item;
	asp->play_id = SScInvalidID;
	UUrString_Copy(asp->item_name, OSrItem_GetName(item), OScMaxNameLength);
	error = SSrAmbient_New(&asp->ambient_id, &asp->ambient);
	if (error != UUcError_None)
	{
		UUrMemory_Block_Delete(asp);
		WMrDialog_ModalEnd(inDialog, OWcASP_Btn_Cancel);
		return;
	}
	
	// copy the data from the item's ambient sound to the asp ambient
	error = 
		SSrAmbient_Copy(
			SSrAmbient_GetByID(OSrItem_GetID(item)),
			asp->ambient);
	if (error != UUcError_None)
	{
		UUrMemory_Block_Delete(asp);
		WMrDialog_ModalEnd(inDialog, OWcSGP_Btn_Cancel);
		return;
	}
	
	// store the asp in the user data
	WMrDialog_SetUserData(inDialog, (UUtUns32)asp);
	
	// set the fields
	OWiASP_SetFields(inDialog, asp);
	
	// enable the buttons
	OWiASP_EnableButtons(inDialog, asp);
	
	// get a pointer to the edit field
	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_Name);
	UUmAssert(editfield);
	
	// set a limit on the length of the name
	WMrMessage_Send(editfield, EFcMessage_SetMaxChars, OScMaxNameLength, 0);
	
	// set the focus on the name
	WMrWindow_SetFocus(editfield);
}

// ----------------------------------------------------------------------
static void
OWiASP_Destroy(
	WMtDialog					*inDialog)
{
	OWtASP						*asp;
	
	// get a pointer to the asp
	asp = (OWtASP*)WMrDialog_GetUserData(inDialog);
	
	// stop any playing ambient sounds
	if (asp->play_id != SScInvalidID)
	{
		SSrAmbient_Stop(asp->play_id);
		asp->play_id = SScInvalidID;
	}
	
	// delete the allocated SStAmbient
	SSrAmbient_Delete(asp->ambient_id);
	asp->ambient_id = SScInvalidID;
	asp->ambient = NULL;
	
	// delete the memory used by the asp
	UUrMemory_Block_Delete(asp);
	WMrDialog_SetUserData(inDialog, 0);
}

// ----------------------------------------------------------------------
static void
OWiASP_HandleCommand(
	WMtDialog					*inDialog,
	UUtUns32					inParam1,
	WMtWindow					*inControl)
{
	OWtASP						*asp;
	UUtUns16					command_type;
	UUtUns16					control_id;
	UUtBool						set_fields;
	
	// get a pointer to the asp
	asp = (OWtASP*)WMrDialog_GetUserData(inDialog);
	
	// interpret inParam1
	command_type = UUmHighWord(inParam1);
	control_id = UUmLowWord(inParam1);
	
	set_fields = UUcFalse;
	
	if (command_type == WMcNotify_Click)
	{
		switch (control_id)
		{
			case OWcASP_Btn_Save:
			{
				char			name[OScMaxNameLength];
				
				WMrMessage_Send(
					WMrDialog_GetItemByID(inDialog, OWcASP_EF_Name),
					EFcMessage_GetText,
					(UUtUns32)name,
					OScMaxNameLength);
					
				if (TSrString_GetLength(name) == 0)
				{
					WMrDialog_MessageBox(
						inDialog,
						"Error",
						"You must supply a name for the ambient sound.",
						WMcMessageBoxStyle_OK);
					
					WMrWindow_SetFocus(WMrDialog_GetItemByID(inDialog, OWcASP_EF_Name));
					
					break;
				}
				
				if (OWiASP_SaveAmbient(inDialog, asp) == UUcTrue)
				{
					WMrDialog_ModalEnd(inDialog, OWcASP_Btn_Save);
				}
			}
			break;
			
			case OWcASP_Btn_Cancel:
				WMrDialog_ModalEnd(inDialog, OWcASP_Btn_Cancel);
			break;
			
			case OWcASP_Btn_InGroupSet:
				OWiSSG_Display(inDialog, &asp->ambient->in_sound_id, &asp->ambient->in_sound);
				set_fields = UUcTrue;
			break;
			
			case OWcASP_Btn_InGroupEdit:
				OWiEditGroup(inDialog, asp->ambient->in_sound);
			break;

			case OWcASP_Btn_OutGroupSet:
				OWiSSG_Display(inDialog, &asp->ambient->out_sound_id, &asp->ambient->out_sound);
				set_fields = UUcTrue;
			break;
			
			case OWcASP_Btn_OutGroupEdit:
				OWiEditGroup(inDialog, asp->ambient->out_sound);
			break;

			case OWcASP_Btn_DetailSet:
				OWiSSG_Display(inDialog, &asp->ambient->detail_id, &asp->ambient->detail);
				set_fields = UUcTrue;
			break;
			
			case OWcASP_Btn_DetailEdit:
				OWiEditGroup(inDialog, asp->ambient->detail);
			break;

			case OWcASP_Btn_BT1Set:
				OWiSSG_Display(inDialog, &asp->ambient->base_track1_id, &asp->ambient->base_track1);
				set_fields = UUcTrue;
			break;
			
			case OWcASP_Btn_BT1Edit:
				OWiEditGroup(inDialog, asp->ambient->base_track1);
			break;

			case OWcASP_Btn_BT2Set:
				OWiSSG_Display(inDialog, &asp->ambient->base_track2_id, &asp->ambient->base_track2);
				set_fields = UUcTrue;
			break;
			
			case OWcASP_Btn_BT2Edit:
				OWiEditGroup(inDialog, asp->ambient->base_track2);
			break;
			
			case OWcASP_CB_InterruptOnStop:
				if (WMrMessage_Send(inControl, WMcMessage_GetValue, 0, 0))
				{
					asp->ambient->flags |= SScAmbientFlag_InterruptOnStop;
				}
				else
				{
					asp->ambient->flags &= ~SScAmbientFlag_InterruptOnStop;
				}
			break;
			
			case OWcASP_Btn_Inc1:
				asp->ambient->sphere_radius += 1.0f;
				if (asp->ambient->sphere_radius > asp->ambient->min_volume_distance)
				{
					asp->ambient->sphere_radius = asp->ambient->min_volume_distance;
				}
				set_fields = UUcTrue;
			break;
			
			case OWcASP_Btn_Dec1:
				asp->ambient->sphere_radius -= 1.0f;
				if (asp->ambient->sphere_radius < 0.0f)
				{
					asp->ambient->sphere_radius = 0.0f;
				}
				set_fields = UUcTrue;
			break;
			
			case OWcASP_Btn_Inc10:
				asp->ambient->sphere_radius += 10.0f;
				if (asp->ambient->sphere_radius > asp->ambient->min_volume_distance)
				{
					asp->ambient->sphere_radius = asp->ambient->min_volume_distance;
				}
				set_fields = UUcTrue;
			break;
			
			case OWcASP_Btn_Dec10:
				asp->ambient->sphere_radius -= 10.0f;
				if (asp->ambient->sphere_radius < 0.0f)
				{
					asp->ambient->sphere_radius = 0.0f;
				}
				set_fields = UUcTrue;
			break;
			
			case OWcASP_Btn_Start:
				asp->play_id = SSrAmbient_Start(asp->ambient, NULL, NULL, NULL, 0.0f, 0.0f);
			break;
			
			case OWcASP_Btn_Stop:
				SSrAmbient_Stop(asp->play_id);
				asp->play_id = SScInvalidID;
			break;

			case OWcASP_CB_PlayOnce:
				if (WMrMessage_Send(inControl, WMcMessage_GetValue, 0, 0))
				{
					asp->ambient->flags |= SScAmbientFlag_PlayOnce;
				}
				else
				{
					asp->ambient->flags &= ~SScAmbientFlag_PlayOnce;
				}
			break;
			
		}
		
		// update the buttons
		OWiASP_EnableButtons(inDialog, asp);
	}
	else if (command_type == EFcNotify_ContentChanged)
	{
		char					string[128];
		
		WMrMessage_Send(inControl, EFcMessage_GetText, (UUtUns32)string, 128);
		
		switch (control_id)
		{
			case OWcASP_EF_Name:
				UUrString_Copy(asp->item_name, string, OScMaxNameLength);
			break;
			
			case OWcASP_EF_MinElapsedTime:
				sscanf(string, "%f", &asp->ambient->min_detail_time);
			break;
			
			case OWcASP_EF_MaxElapsedTime:
				sscanf(string, "%f", &asp->ambient->max_detail_time);
			break;
			
			case OWcASP_EF_SphereRadius:
				sscanf(string, "%f", &asp->ambient->sphere_radius);
			break;
			
			case OWcASP_EF_MinVolDist:
				sscanf(string, "%f", &asp->ambient->min_volume_distance);
			break;
			
			case OWcASP_EF_MaxVolDist:
				sscanf(string, "%f", &asp->ambient->max_volume_distance);
			break;
		}
	}

	// set the fields
	if (set_fields)
	{
		OWiASP_SetFields(inDialog, asp);
	}
}

// ----------------------------------------------------------------------
static UUtBool
OWiASP_Callback(
	WMtDialog					*inDialog,
	WMtMessage					inMessage,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	UUtBool						handled;
	
	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiASP_InitDialog(inDialog);
		break;
		
		case WMcMessage_Destroy:
			OWiASP_Destroy(inDialog);
		break;
		
		case WMcMessage_Command:
			OWiASP_HandleCommand(inDialog, inParam1, (WMtWindow*)inParam2);
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ----------------------------------------------------------------------
UUtError
OWrASP_Display(
	OStItem						*inItem)
{
	UUtError					error;
	
	// edit the properties of the group
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Ambient_Sound_Prop,
			NULL,
			OWiASP_Callback,
			(UUtUns32)inItem,
			NULL);
	UUmError_ReturnOnError(error);

	return error;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
OWiISP_SaveImpulse(
	WMtDialog					*inDialog,
	OWtISP						*inISP)
{
	SStImpulse					*item_impulse;
	WMtWindow					*editfield;
	char						string[128];
	
	// set the impulse sound's name
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_Name);
	WMrMessage_Send(editfield, EFcMessage_GetText, (UUtUns32)string, 128);
	OSrCollection_Item_SetName(inISP->collection, OSrItem_GetID(inISP->item), string);
	
	// save the floats
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MaxVolDist);
	WMrMessage_Send(editfield, EFcMessage_GetText, (UUtUns32)string, 128);
	sscanf(string, "%f", &inISP->impulse->max_volume_distance);
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MinVolDist);
	WMrMessage_Send(editfield, EFcMessage_GetText, (UUtUns32)string, 128);
	sscanf(string, "%f", &inISP->impulse->min_volume_distance);
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MinVolAngle);
	WMrMessage_Send(editfield, EFcMessage_GetText, (UUtUns32)string, 128);
	sscanf(string, "%f", &inISP->impulse->min_volume_angle);
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MaxVolAngle);
	WMrMessage_Send(editfield, EFcMessage_GetText, (UUtUns32)string, 128);
	sscanf(string, "%f", &inISP->impulse->max_volume_angle);
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MinAngleAttn);
	WMrMessage_Send(editfield, EFcMessage_GetText, (UUtUns32)string, 128);
	sscanf(string, "%f", &inISP->impulse->min_angle_attenuation);
	
	// check ranges
	if (inISP->impulse->max_volume_distance > inISP->impulse->min_volume_distance)
	{
		WMrDialog_MessageBox(
			inDialog,
			"Error",
			"The Maximum Volume Distance must be less than or equal to the Minimum Volume Distance.",
			WMcMessageBoxStyle_OK);
		WMrWindow_SetFocus(WMrDialog_GetItemByID(inDialog, OWcISP_EF_MinVolDist));
		return UUcFalse;
	}
	if (inISP->impulse->min_volume_angle > inISP->impulse->max_volume_angle)
	{
		WMrDialog_MessageBox(
			inDialog,
			"Error",
			"The Minimum Volume Angle must be less than or equal to the Maximum Volume Angle.",
			WMcMessageBoxStyle_OK);
		WMrWindow_SetFocus(WMrDialog_GetItemByID(inDialog, OWcISP_EF_MinVolAngle));
		return UUcFalse;
	}

	// save the impulse data
	item_impulse = SSrImpulse_GetByID(OSrItem_GetID(inISP->item));
	SSrImpulse_Copy(inISP->impulse, item_impulse);
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
static void
OWiISP_SetFields(
	WMtDialog					*inDialog,
	OWtISP						*inISP)
{
	WMtWindow					*editfield;
	WMtWindow					*popup;
	char						string[128];
	
	// set the impulse sound's name
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_Name);
	WMrMessage_Send(editfield, EFcMessage_SetText, (UUtUns32)inISP->item_name, 0);
	
	// set the group name
	OWiSetGroupText(inDialog, OWcISP_Txt_Group, &inISP->impulse->impulse_sound, "Impulse Sound");
		
	// set the priority
	popup = WMrDialog_GetItemByID(inDialog, OWcISP_PM_Priority);
	WMrPopupMenu_SetSelection(popup, (UUtUns16)inISP->impulse->priority);
	
	// set the rest of the edit fields
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MaxVolDist);
	sprintf(string, "%5.3f", inISP->impulse->max_volume_distance);
	WMrMessage_Send(editfield, EFcMessage_SetText, (UUtUns32)string, 0);

	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MinVolDist);
	sprintf(string, "%5.3f", inISP->impulse->min_volume_distance);
	WMrMessage_Send(editfield, EFcMessage_SetText, (UUtUns32)string, 0);

	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MinVolAngle);
	sprintf(string, "%5.3f", inISP->impulse->min_volume_angle);
	WMrMessage_Send(editfield, EFcMessage_SetText, (UUtUns32)string, 0);
	WMrWindow_SetEnabled(editfield, UUcFalse);

	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MaxVolAngle);
	sprintf(string, "%5.3f", inISP->impulse->max_volume_angle);
	WMrMessage_Send(editfield, EFcMessage_SetText, (UUtUns32)string, 0);
	WMrWindow_SetEnabled(editfield, UUcFalse);

	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MinAngleAttn);
	sprintf(string, "%5.3f", inISP->impulse->min_angle_attenuation);
	WMrMessage_Send(editfield, EFcMessage_SetText, (UUtUns32)string, 0);
	WMrWindow_SetEnabled(editfield, UUcFalse);
	
	// set the button states
	if (inISP->impulse->impulse_sound != NULL)
	{
		WMrWindow_SetEnabled(
			WMrDialog_GetItemByID(inDialog, OWcISP_Btn_Edit),
			UUcTrue);
	}
	else
	{
		WMrWindow_SetEnabled(
			WMrDialog_GetItemByID(inDialog, OWcISP_Btn_Edit),
			UUcFalse);
	}
}

// ----------------------------------------------------------------------
static void
OWiISP_InitDialog(
	WMtDialog					*inDialog)
{
	OStItem						*item;
	OWtISP						*isp;
	UUtError					error;
	WMtWindow					*editfield;
	
	// get the item
	item = (OStItem*)WMrDialog_GetUserData(inDialog);
	if (item == NULL)
	{
		WMrDialog_ModalEnd(inDialog, OWcISP_Btn_Cancel);
		return;
	}
	
	// allocate memory for the isp
	isp = (OWtISP*)UUrMemory_Block_New(sizeof(OWtISP));
	if (isp == NULL)
	{
		WMrDialog_ModalEnd(inDialog, OWcISP_Btn_Cancel);
		return;
	}
	
	// initialize the isp
	isp->collection = OSrCollection_GetByType(OScCollectionType_Impulse);
	isp->item = item;
	UUrString_Copy(isp->item_name, OSrItem_GetName(item), OScMaxNameLength);
	error = SSrImpulse_New(&isp->impulse_id, &isp->impulse);
	if (error != UUcError_None)
	{
		UUrMemory_Block_Delete(isp);
		WMrDialog_ModalEnd(inDialog, OWcISP_Btn_Cancel);
		return;
	}
	
	// copy the data from the item's impulse sound to the isp impulse
	error = 
		SSrImpulse_Copy(
			SSrImpulse_GetByID(OSrItem_GetID(item)),
			isp->impulse);
	if (error != UUcError_None)
	{
		UUrMemory_Block_Delete(isp);
		WMrDialog_ModalEnd(inDialog, OWcISP_Btn_Cancel);
		return;
	}
	
	// stop the isp in the user data
	WMrDialog_SetUserData(inDialog, (UUtUns32)isp);
	
	// set the fields
	OWiISP_SetFields(inDialog, isp);

	// get a pointer to the edit field
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_Name);
	UUmAssert(editfield);
	
	// set a limit on the length of the name
	WMrMessage_Send(editfield, EFcMessage_SetMaxChars, OScMaxNameLength, 0);
	
	// make the edit field the text focus
	WMrWindow_SetFocus(editfield);
}

// ----------------------------------------------------------------------
static void
OWiISP_Destroy(
	WMtDialog					*inDialog)
{
	OWtISP						*isp;
	
	// get a pointer to the isp
	isp = (OWtISP*)WMrDialog_GetUserData(inDialog);
	
	// delete the allocated SStImpulse
	SSrImpulse_Delete(isp->impulse_id);
	isp->impulse_id = SScInvalidID;
	isp->impulse = NULL;
	
	// delete the memory used by the isp
	UUrMemory_Block_Delete(isp);
	WMrDialog_SetUserData(inDialog, 0);
}

// ----------------------------------------------------------------------
static void
OWiISP_HandleCommand(
	WMtDialog					*inDialog,
	UUtUns32					inParam1,
	WMtWindow					*inControl)
{
	OWtISP						*isp;
	
	// get the isp
	isp = (OWtISP*)WMrDialog_GetUserData(inDialog);
	
	if (UUmHighWord(inParam1) == WMcNotify_Click)
	{
		switch (UUmLowWord(inParam1))
		{
			case OWcISP_Btn_Save:
			{
				char			name[OScMaxNameLength];
				
				WMrMessage_Send(
					WMrDialog_GetItemByID(inDialog, OWcISP_EF_Name),
					EFcMessage_GetText,
					(UUtUns32)name,
					OScMaxNameLength);
					
				if (TSrString_GetLength(name) == 0)
				{
					WMrDialog_MessageBox(
						inDialog,
						"Error",
						"You must supply a name for the impulse sound.",
						WMcMessageBoxStyle_OK);
					
					WMrWindow_SetFocus(WMrDialog_GetItemByID(inDialog, OWcISP_EF_Name));
					
					break;
				}
				
				if (OWiISP_SaveImpulse(inDialog, isp) == UUcTrue)
				{
					WMrDialog_ModalEnd(inDialog, OWcISP_Btn_Save);
				}
			}
			break;

			case OWcISP_Btn_Cancel:
				WMrDialog_ModalEnd(inDialog, OWcISP_Btn_Cancel);
			break;
			
			case OWcISP_Btn_Set:
			{
				WMtWindow					*text;
				
				// select a sound group
				OWiSSG_Display(
					inDialog,
					&isp->impulse->impulse_sound_id,
					&isp->impulse->impulse_sound);
				if (isp->impulse->impulse_sound != NULL)
				{
					WMrWindow_SetEnabled(
						WMrDialog_GetItemByID(inDialog, OWcISP_Btn_Edit),
						UUcTrue);
				}
				else
				{
					WMrWindow_SetEnabled(
						WMrDialog_GetItemByID(inDialog, OWcISP_Btn_Edit),
						UUcFalse);
				}
				
				// put the name of the sound group into the text field
				text = WMrDialog_GetItemByID(inDialog, OWcISP_Txt_Group);
				WMrWindow_SetTitle(text, "", OScMaxNameLength);
				if (isp->impulse->impulse_sound)
				{
					OStItem				*item;
					item =
						OSrCollection_Item_GetByID(
							OSrCollection_GetByType(OScCollectionType_Group),
							isp->impulse->impulse_sound->id);
					WMrWindow_SetTitle(text, OSrItem_GetName(item), OScMaxNameLength);
				}
			}
			break;
			
			case OWcISP_Btn_Edit:
				if (isp->impulse->impulse_sound != NULL)
				{
					OWiEditGroup(inDialog, isp->impulse->impulse_sound);
				}
			break;
			
			case OWcISP_Btn_Play:
				if (isp->impulse->impulse_sound == NULL)
				{
					WMrDialog_MessageBox(
						inDialog,
						"Error",
						"You must select a sound group before you can play the impulse sound.",
						WMcMessageBoxStyle_OK);
					break;
				}
				
				SSrGroup_Play(isp->impulse->impulse_sound, NULL);
			break;
		}
	}
	else if (UUmHighWord(inParam1) == EFcNotify_ContentChanged)
	{
		switch (UUmLowWord(inParam1))
		{
			case OWcISP_EF_Name:
				WMrMessage_Send(
					inControl,
					EFcMessage_GetText,
					(UUtUns32)isp->item_name,
					OScMaxNameLength);
			break;
		}
	}
}

// ----------------------------------------------------------------------
static void
OWiISP_HandleMenuCommand(
	WMtDialog					*inDialog,
	WMtWindow					*inPopupMenu,
	UUtUns32					inItemID)
{
	OWtISP						*isp;
	
	// get the isp
	isp = (OWtISP*)WMrDialog_GetUserData(inDialog);

	// save the priority
	isp->impulse->priority = (SStPriority2)inItemID;
}

// ----------------------------------------------------------------------
static UUtBool
OWiISP_Callback(
	WMtDialog					*inDialog,
	WMtMessage					inMessage,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	UUtBool						handled;
	
	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiISP_InitDialog(inDialog);
		break;
		
		case WMcMessage_Destroy:
			OWiISP_Destroy(inDialog);
		break;
		
		case WMcMessage_Command:
			OWiISP_HandleCommand(inDialog, inParam1, (WMtWindow*)inParam2);
		break;
		
		case WMcMessage_MenuCommand:
			OWiISP_HandleMenuCommand(
				inDialog,
				(WMtWindow*)inParam2,
				(UUtUns32)UUmLowWord(inParam1)); 
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ----------------------------------------------------------------------
void
OWrImpulseSoundProp_Display(
	WMtDialog					*inParentDialog,
	OStItem						*inItem)
{
	// edit the properties of the impulse sound
	WMrDialog_ModalBegin(
		OWcDialog_Impulse_Sound_Prop,
		inParentDialog,
		OWiISP_Callback,
		(UUtUns32)inItem,
		NULL);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
OWiSCM_Listbox_Fill(
	WMtDialog					*inDialog,
	OWtSCM						*inSCM)
{
	WMtWindow					*listbox;
	WMtWindow					*popup;
	UUtUns32					i;
	UUtUns16					category_id;
	
	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcSCM_LB_Items);
	UUmAssert(listbox);
	
	// clear the listbox
	WMrMessage_Send(listbox, LBcMessage_Reset, 0, 0);
	
	// get a pointer to the popup
	popup = WMrDialog_GetItemByID(inDialog, OWcSCM_PM_Category);
	UUmAssert(popup);
	
	// get the id of the active category
	WMrPopupMenu_GetItemID(popup, -1, &category_id);
	
	// add categories to the listbox
	for (i = 0; i < OSrCollection_GetNumCategories(inSCM->collection); i++)
	{
		OStCategory				*category;
		
		// get the category
		category = OSrCollection_Category_GetByIndex(inSCM->collection, i);
		if (category == NULL) { continue; }
		if (OSrCategory_GetParentCategoryID(category) != (UUtUns32)category_id) { continue; }
		
		// add the category
		WMrMessage_Send(
			listbox,
			LBcMessage_AddString,
			(OWcCatToData | OSrCategory_GetID(category)),
			0);
	}
	
	// add items to the listbox
	for (i = 0; i < OSrCollection_GetNumItems(inSCM->collection); i++)
	{
		OStItem					*item;
		
		// get the item
		item = OSrCollection_Item_GetByIndex(inSCM->collection, i);
		if (item == NULL) { continue; }
		if (OSrItem_GetCategoryID(item) != category_id) { continue; }
		
		// add the item
		WMrMessage_Send(
			listbox,
			LBcMessage_AddString,
			OSrItem_GetID(item),
			0);
	}
	
	// select the first item in the list
	WMrMessage_Send(listbox, LBcMessage_SetSelection, (UUtUns32)UUcTrue, 0);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OWiSCM_Popup_Init(
	WMtDialog					*inDialog,
	OWtSCM						*inSCM)
{
	UUtError					error;
	WMtWindow					*popup;
	WMtMenuItemData				item_data;
	
	// get a pointer to the popup menu
	popup = WMrDialog_GetItemByID(inDialog, OWcSCM_PM_Category);
	UUmAssert(popup);
	
	// initialize the menu item
	item_data.id = 0;
	item_data.flags = WMcMenuItemFlag_Enabled;
	UUrString_Copy(item_data.title, inSCM->collection_name, OScMaxNameLength); 
	
	// append the type name to the popup
	error = WMrPopupMenu_AppendItem(popup, &item_data);
	UUmError_ReturnOnError(error);
		
	// select the first item
	WMrPopupMenu_SetSelection(popup, 0);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OWiSCM_Enable_Buttons(
	WMtDialog					*inDialog,
	OWtSCM						*inSCM)
{
	WMtWindow					*listbox;
	WMtWindow					*button;
	UUtUns32					item_id;
	UUtBool						is_category;
	UUtBool						is_item;
	UUtBool						can_paste;
	UUtUns32					result;
	
	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcSCM_LB_Items);
	UUmAssert(listbox);
	
	// find out of the currently selected item is a category
	is_category = UUcFalse;
	is_item = UUcFalse;
	can_paste = inSCM->clip_ss_id != SScInvalidID;
	
	result = WMrMessage_Send(listbox, LBcMessage_GetText, (UUtUns32)&item_id, (UUtUns32)-1);
	if (result != LBcError)
	{
		if (item_id & OWcCatToData) { is_category = UUcTrue; }
		if ((item_id != SScInvalidID) && (is_category == UUcFalse)) { is_item = UUcTrue; } 
	}
	
	// if the collection is locked, then no changes can be made to the items or categories
	if (OSrCollection_IsLocked(inSCM->collection) == UUcTrue)
	{
		is_item = UUcFalse;
		can_paste = UUcFalse;
		is_category = UUcFalse;
	}
	
	// enable/disable buttons
	button = WMrDialog_GetItemByID(inDialog, OWcSCM_Btn_EditItem);
	WMrWindow_SetEnabled(button, is_item);
	
	button = WMrDialog_GetItemByID(inDialog, OWcSCM_Btn_PlayItem);
	WMrWindow_SetEnabled(button, is_item);
	
	button = WMrDialog_GetItemByID(inDialog, OWcSCM_Btn_DeleteItem);
	WMrWindow_SetEnabled(button, is_item);
	
	button = WMrDialog_GetItemByID(inDialog, OWcSCM_Btn_CopyItem);
	WMrWindow_SetEnabled(button, is_item);
	
	button = WMrDialog_GetItemByID(inDialog, OWcSCM_Btn_PasteItem);
	WMrWindow_SetEnabled(button, can_paste);
	
	button = WMrDialog_GetItemByID(inDialog, OWcSCM_Btn_RenameCategory);
	WMrWindow_SetEnabled(button, is_category);
	
	button = WMrDialog_GetItemByID(inDialog, OWcSCM_Btn_DeleteCategory);
	WMrWindow_SetEnabled(button, is_category);
}

// ----------------------------------------------------------------------
static UUtError
OWiSCM_OpenCategory(
	WMtDialog					*inDialog,
	OWtSCM						*inSCM,
	UUtUns32					inCategoryID)
{
	UUtError					error;
	OStCategory					*category;
	WMtWindow					*popup;
	WMtMenuItemData				item_data;
	
	// get a pointer to the category
	category = OSrCollection_Category_GetByID(inSCM->collection, inCategoryID);
	if (category == NULL) { return UUcError_Generic; }

	// get a pointer to the popup menu
	popup = WMrDialog_GetItemByID(inDialog, OWcSCM_PM_Category);
	UUmAssert(popup);
	
	// initialize the menu item
	item_data.id = (UUtUns16)inCategoryID;
	item_data.flags = WMcMenuItemFlag_Enabled;
	UUrString_Copy(item_data.title, OSrCategory_GetName(category), OScMaxNameLength);
	
	// append the type name to the popup
	error = WMrPopupMenu_AppendItem(popup, &item_data);
	UUmError_ReturnOnError(error);
	
	// select the item that was just added
	WMrPopupMenu_SetSelection(popup, (UUtUns16)inCategoryID);
	
	// update the buttons
	OWiSCM_Enable_Buttons(inDialog, inSCM);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OWiSCM_NewItem(
	WMtDialog					*inDialog,
	OWtSCM						*inSCM)
{
	UUtError					error;
	
	UUtUns32					new_ss_id;
	WMtDialogID					dialog_id;
	WMtDialogCallback			dialog_callback;
	
	UUtUns16					category_id;
	OStItem						*item;
	
	WMtWindow					*listbox;
	UUtUns32					list_index;
	
	UUtUns32					message;
		
	// set the dialog id and callback
	switch (OSrCollection_GetType(inSCM->collection))
	{
		case OScCollectionType_Group:
			error = SSrGroup_New(&new_ss_id, NULL);
			dialog_id = OWcDialog_Sound_Group_Prop;
			dialog_callback = OWiSGP_Callback;
		break;
		
		case OScCollectionType_Impulse:
			error = SSrImpulse_New(&new_ss_id, NULL);
			dialog_id = OWcDialog_Impulse_Sound_Prop;
			dialog_callback = OWiISP_Callback;
		break;
		
		case OScCollectionType_Ambient:
			error = SSrAmbient_New(&new_ss_id, NULL);
			dialog_id = OWcDialog_Ambient_Sound_Prop;
			dialog_callback = OWiASP_Callback;
		break;
	}
	
	// get the category_id
	WMrPopupMenu_GetItemID(
		WMrDialog_GetItemByID(inDialog, OWcSCM_PM_Category),
		-1,
		&category_id);
	
	// create a new item in the collection
	error = OSrCollection_Item_New(inSCM->collection, (UUtUns32)category_id, new_ss_id, "");
	if (error != UUcError_None) { goto cleanup; }
	
	// get the item
	item = OSrCollection_Item_GetByID(inSCM->collection, new_ss_id);
	if (item == NULL) { goto cleanup; }
	
	// set the properties of the item
	error =
		WMrDialog_ModalBegin(
			dialog_id,
			inDialog,
			dialog_callback,
			(UUtUns32)item,
			&message);
	if (error != UUcError_None) { goto cleanup; }
	
	// if the user canceled the create, then delete the new items and exit
	if (message == WMcDialogItem_Cancel)
	{
		OSrCollection_Item_Delete(inSCM->collection, new_ss_id);
		new_ss_id = SScInvalidID;
		return UUcError_None;
	}
	
	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcSCM_LB_Items);
	UUmAssert(listbox);
	
	// insert the new item's id into the listbox
	list_index = WMrMessage_Send(listbox, LBcMessage_AddString, new_ss_id, 0);
	
	// select the item that was just added
	WMrMessage_Send(listbox, LBcMessage_SetSelection, (UUtUns32)UUcTrue, list_index);
	
	return UUcError_None;
	
cleanup:
	if (new_ss_id != SScInvalidID)
	{
		switch (OSrCollection_GetType(inSCM->collection))
		{
			case OScCollectionType_Group:
				SSrGroup_Delete(new_ss_id);
			break;
			
			case OScCollectionType_Impulse:
				SSrImpulse_Delete(new_ss_id);
			break;
			
			case OScCollectionType_Ambient:
				SSrAmbient_Delete(new_ss_id);
			break;
		}
	}
	
	return error;
}

// ----------------------------------------------------------------------
static UUtError
OWiSCM_EditItem(
	WMtDialog					*inDialog,
	OWtSCM						*inSCM,
	UUtUns32					inItemID)
{
	UUtError					error;
	WMtDialogID					dialog_id;
	WMtDialogCallback			dialog_callback;
	OStItem						*item;
	
	// get a pointer to the item and it's ss_id
	item = OSrCollection_Item_GetByID(inSCM->collection, inItemID);
	if (item == NULL) { return UUcError_Generic; }
	
	// set the dialog id and callback
	switch (OSrCollection_GetType(inSCM->collection))
	{
		case OScCollectionType_Group:
			dialog_id = OWcDialog_Sound_Group_Prop;
			dialog_callback = OWiSGP_Callback;
		break;
		
		case OScCollectionType_Impulse:
			dialog_id = OWcDialog_Impulse_Sound_Prop;
			dialog_callback = OWiISP_Callback;
		break;
		
		case OScCollectionType_Ambient:
			dialog_id = OWcDialog_Ambient_Sound_Prop;
			dialog_callback = OWiASP_Callback;
		break;
	}
	
	// edit the properties of the group
	error =
		WMrDialog_ModalBegin(
			dialog_id,
			inDialog,
			dialog_callback,
			(UUtUns32)item,
			NULL);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OWiSCM_PlayItem(
	WMtDialog					*inDialog,
	OWtSCM						*inSCM,
	UUtUns32					inItemID)
{
	OStItem						*item;
	UUtUns32					item_ss_id;
	
	// get the item from the collection
	item = OSrCollection_Item_GetByID(inSCM->collection, inItemID);
	if (item == NULL) { return UUcError_Generic; }
	item_ss_id = OSrItem_GetID(item);
	
	switch (OSrCollection_GetType(inSCM->collection))
	{
		case OScCollectionType_Group:
			SSrGroup_Play(SSrGroup_GetByID(item_ss_id), NULL);
		break;
		
		case OScCollectionType_Impulse:
		{
			M3tPoint3D			position;
			M3tVector3D			direction;
			M3tVector3D			velocity;
			
			MUmVector_Set(position, 0.0f, 0.0f, 0.0f);
			MUmVector_Set(direction, 0.0f, 0.0f, 0.0f);
			MUmVector_Set(velocity, 0.0f, 0.0f, 0.0f);
			
			SSrImpulse_Play(SSrImpulse_GetByID(item_ss_id), &position, &direction, &velocity);
		}
		break;
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OWiSCM_DeleteItem(
	WMtDialog					*inDialog,
	OWtSCM						*inSCM,
	UUtUns32					inItemID)
{
	UUtUns32					message;
	
	// ask the user to confirm the deleteion of the group
	message =
		WMrDialog_MessageBox(
			inDialog,
			"Confirm Item Deletion",
			"Are you sure you want to delete the selected item?",
			WMcMessageBoxStyle_Yes_No);
	if (message == WMcDialogItem_Yes)
	{
		WMtWindow				*listbox;
		UUtUns32				item_index;
		
		// delete the item from the category
		OSrCollection_Item_Delete(inSCM->collection, inItemID);
		
		switch (OSrCollection_GetType(inSCM->collection))
		{
			case OScCollectionType_Group:
			break;
			
			case OScCollectionType_Impulse:
				OSrVariantList_UpdateImpulseIDs(inItemID);
				ONrImpactEffects_UpdateImpulseIDs(inItemID);
			break;
			
			case OScCollectionType_Ambient:
			break;
		}
		
		// get a pointer to the listbox
		listbox = WMrDialog_GetItemByID(inDialog, OWcSCM_LB_Items);
		if (listbox == NULL) { return UUcError_Generic; }
		
		// get the selected item's index
		item_index = WMrMessage_Send(listbox, LBcMessage_GetSelection, 0, 0);
		
		// remove the item's name from the list box
		WMrMessage_Send(listbox, LBcMessage_DeleteString, 0, item_index);
		
		// select the first item in the listbox
		WMrMessage_Send(listbox, LBcMessage_SetSelection, (UUtUns32)UUcTrue, 0);
		
		// enabled the buttons
		OWiSCM_Enable_Buttons(inDialog, inSCM);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OWiSCM_CopyItem(
	WMtDialog					*inDialog,
	OWtSCM						*inSCM,
	UUtUns32					inItemID)
{
	UUtError					error;
	OStItem						*item;
	UUtUns32					item_ss_id;
	
	// get the item from the collection
	item = OSrCollection_Item_GetByID(inSCM->collection, inItemID);
	if (item == NULL) { return UUcError_Generic; }
	item_ss_id = OSrItem_GetID(item);
	
	switch (OSrCollection_GetType(inSCM->collection))
	{
		case OScCollectionType_Group:
			if (inSCM->clip_ss_id == SScInvalidID)
			{
				error = SSrGroup_New(&inSCM->clip_ss_id, NULL);
				UUmError_ReturnOnError(error);
			}
			
			// copy the data from the item's data to group
			error =
				SSrGroup_Copy(
					SSrGroup_GetByID(item_ss_id),
					SSrGroup_GetByID(inSCM->clip_ss_id));
		break;
		
		case OScCollectionType_Impulse:
			if (inSCM->clip_ss_id == SScInvalidID)
			{
				error = SSrImpulse_New(&inSCM->clip_ss_id, NULL);
				UUmError_ReturnOnError(error);
			}
			
			// copy the data from the item's data to impulse
			error =	
				SSrImpulse_Copy(
					SSrImpulse_GetByID(item_ss_id),
					SSrImpulse_GetByID(inSCM->clip_ss_id));
		break;
		
		case OScCollectionType_Ambient:
			if (inSCM->clip_ss_id == SScInvalidID)
			{
				error = SSrAmbient_New(&inSCM->clip_ss_id, NULL);
				UUmError_ReturnOnError(error);
			}
			
			// copy the data from the item's data to impulse
			error =	
				SSrAmbient_Copy(
					SSrAmbient_GetByID(item_ss_id),
					SSrAmbient_GetByID(inSCM->clip_ss_id));
		break;
	}
	UUmError_ReturnOnError(error);
	
	// copy the name
	UUrString_Copy(inSCM->clip_name, OSrItem_GetName(item), OScMaxNameLength);
	
	// update the button
	OWiSCM_Enable_Buttons(inDialog, inSCM);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OWiSCM_PasteItem(
	WMtDialog					*inDialog,
	OWtSCM						*inSCM,
	UUtUns32					inItemID)
{
	UUtError					error;
	UUtUns16					category_id;
	WMtWindow					*popup;
	WMtWindow					*listbox;
	UUtUns32					list_index;
	UUtUns32					new_ss_id;
	UUtUns32					num;
	char						name[OScMaxNameLength];
	
	// get a pointer to the popup
	popup = WMrDialog_GetItemByID(inDialog, OWcSCM_PM_Category);
	UUmAssert(popup);
	
	// get the category_id
	WMrPopupMenu_GetItemID(popup, -1, &category_id);
	
	// create a new ss_id and copy the clipboard ss_id into ss_id
	switch (OSrCollection_GetType(inSCM->collection))
	{
		case OScCollectionType_Group:
			error = SSrGroup_New(&new_ss_id, NULL);
			UUmError_ReturnOnError(error);
			error =
				SSrGroup_Copy(
					SSrGroup_GetByID(inSCM->clip_ss_id),
					SSrGroup_GetByID(new_ss_id));
		break;
		
		case OScCollectionType_Impulse:
			error = SSrImpulse_New(&new_ss_id, NULL);
			UUmError_ReturnOnError(error);
			error =
				SSrImpulse_Copy(
					SSrImpulse_GetByID(inSCM->clip_ss_id),
					SSrImpulse_GetByID(new_ss_id));
		break;
		
		case OScCollectionType_Ambient:
			error = SSrAmbient_New(&new_ss_id, NULL);
			UUmError_ReturnOnError(error);
			error =
				SSrAmbient_Copy(
					SSrAmbient_GetByID(inSCM->clip_ss_id),
					SSrAmbient_GetByID(new_ss_id));
		break;
	}
	UUmError_ReturnOnError(error);
	
	UUrString_Copy(name, inSCM->clip_name, OScMaxNameLength);
	
	// paste the item
	num = 0;
	do
	{
		error =
			OSrCollection_Item_New(
				inSCM->collection,
				(UUtUns32)category_id,
				new_ss_id,
				name);
		if (error != UUcError_None)
		{
			UUtUns32	name_length;
			char		num_string[OScMaxNameLength];
			
			num++;
			if (num == 100) { goto cleanup; }
			
			sprintf(num_string, " %d", num);

			name_length = strlen(name);
			if (name_length == (OScMaxNameLength - 1))
			{
				name[name_length - strlen(num_string)] = '\0';
			}
			else
			{
				char	*term;
				
				term = strrchr(name, ' ');
				if (term != NULL)
				{
					char	check_me;
					
					check_me = term[1];
					if (isdigit(check_me))
					{
						term[0] = '\0';
					}
				}
			}
						
			UUrString_Cat(name, num_string, OScMaxNameLength);
		}
	}
	while (error != UUcError_None);
	
	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcSCM_LB_Items);
	UUmAssert(listbox);
	
	// insert the new item's id into the listbox
	list_index = WMrMessage_Send(listbox, LBcMessage_AddString, new_ss_id, 0);
	
	// select the item that was just added
	WMrMessage_Send(listbox, LBcMessage_SetSelection, (UUtUns32)UUcTrue, list_index);
	
	return UUcError_None;

cleanup:
	if (new_ss_id != SScInvalidID)
	{
		switch (OSrCollection_GetType(inSCM->collection))
		{
			case OScCollectionType_Group:
				SSrGroup_Delete(new_ss_id);
			break;
			
			case OScCollectionType_Impulse:
				SSrImpulse_Delete(new_ss_id);
			break;
			
			case OScCollectionType_Ambient:
				SSrAmbient_Delete(new_ss_id);
			break;
		}
		new_ss_id = SScInvalidID;
	}
	
	return error;
}

// ----------------------------------------------------------------------
static UUtError
OWiSCM_NewCategory(
	WMtDialog					*inDialog,
	OWtSCM						*inSCM)
{
	UUtError					error;
	UUtUns32					message;
	char						category_name[OScMaxNameLength];
	
	// null terminate category name
	category_name[0] = '\0';
		
	// get the name of the new category
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Category_Name,
			inDialog,
			OWiCN_Callback,
			(UUtUns32)category_name,
			&message);
	UUmError_ReturnOnError(error);
	
	if (message == OWcCN_Btn_OK)
	{
		UUtUns16				parent_category_id;
		UUtUns32				category_id;

		WMtWindow				*listbox;
		UUtUns32				listbox_index;
		
		// get the id of the active category
		WMrPopupMenu_GetItemID(
			WMrDialog_GetItemByID(inDialog, OWcSCM_PM_Category),
			-1,
			&parent_category_id);
				
		// create the new category
		error =
			OSrCollection_Category_New(
				inSCM->collection,
				(UUtUns32)parent_category_id,
				category_name,
				&category_id);
		if (error != UUcError_None)
		{
			WMrDialog_MessageBox(
				inDialog,
				"Error",
				"The new category could not be created.  Make sure the category name is unique and the .bin file is not locked.",
				WMcMessageBoxStyle_OK);
				
			return error;
		}
		
		// add the category to the listbox
		listbox = WMrDialog_GetItemByID(inDialog, OWcSCM_LB_Items);
		UUmAssert(listbox);
		
		// add the category to the listbox
		listbox_index =
			WMrMessage_Send(
				listbox,
				LBcMessage_AddString,
				(OWcCatToData | category_id),
				0);
		
		// select the new category
		WMrMessage_Send(listbox, LBcMessage_SetSelection, (UUtUns32)UUcTrue, listbox_index);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OWiSCM_RenameCategory(
	WMtDialog					*inDialog,
	OWtSCM						*inSCM,
	UUtUns32					inCategoryID)
{
	UUtError					error;
	UUtUns32					message;
	char						category_name[OScMaxNameLength];
	OStCategory					*category;
	
	// get a pointer to the category
	category = OSrCollection_Category_GetByID(inSCM->collection, inCategoryID);
	
	// get the current category's name
	UUrString_Copy(category_name, OSrCategory_GetName(category), OScMaxNameLength);

	// get the name of the new category
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Category_Name,
			inDialog,
			OWiCN_Callback,
			(UUtUns32)category_name,
			&message);
	UUmError_ReturnOnError(error);
	
	if (message == OWcCN_Btn_OK)
	{
		// set the name of the category
		error = OSrCollection_Category_SetName(inSCM->collection, inCategoryID, category_name);
		if (error != UUcError_None)
		{
			WMrDialog_MessageBox(
				inDialog,
				"Error",
				"Unable to change the category name.  The .bin file may be locked.",
				WMcMessageBoxStyle_OK);
		}
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OWiSCM_DeleteCategory(
	WMtDialog					*inDialog,
	OWtSCM						*inSCM,
	UUtUns32					inCategoryID)
{
	UUtUns32					message;
	
	// ask the user to confirm the deletion of the category and
	// all the items in it
	message = 
		WMrDialog_MessageBox(
			inDialog,
			"Confirm Category Deletion",
			"Are you sure you want to delete the category and all of the items in it?",
			WMcMessageBoxStyle_Yes_No);
	if (message == WMcDialogItem_Yes)
	{
		WMtListBox				*listbox;
		UUtUns32				selected_index;
		
		// delete the category
		OSrCollection_Category_Delete(inSCM->collection, inCategoryID);
		
		// get a pointer to the listbox
		listbox = WMrDialog_GetItemByID(inDialog, OWcSCM_LB_Items);
		UUmAssert(listbox);
		
		// get the selected item index
		selected_index = WMrMessage_Send(listbox, LBcMessage_GetSelection, 0, 0);
		
		// delete the selected item
		WMrMessage_Send(listbox, LBcMessage_DeleteString, 0, selected_index);
		
		// select the first item in the listbox
		WMrMessage_Send(listbox, LBcMessage_SetSelection, (UUtUns32)UUcTrue, 0);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OWiSCM_InitDialog(
	WMtDialog					*inDialog)
{
	UUtError					error;
	OWtSCM						*scm;
	OStCollectionType			collection_type;
	OStCollection				*collection;
	
	// get the collection type
	collection_type = (OStCollectionType)WMrDialog_GetUserData(inDialog);
	
	// get a pointer to the collection
	collection = OSrCollection_GetByType(collection_type);
	if (collection == NULL)
	{
		WMrDialog_ModalEnd(inDialog, 0);
		return;
	}
	
	// allocate memory for the scm
	scm = (OWtSCM*)UUrMemory_Block_New(sizeof(OWtSCM));
	if (scm == NULL)
	{
		WMrDialog_ModalEnd(inDialog, 0);
		return;
	}
	
	// set up the scm
	scm->collection = collection;
	scm->clip_ss_id = SScInvalidID;
	scm->clip_name[0] = '\0';
	
	switch (collection_type)
	{
		case OScCollectionType_Group:
			UUrString_Copy(scm->collection_name, "Group", OScMaxNameLength);
		break;
		
		case OScCollectionType_Impulse:
			UUrString_Copy(scm->collection_name, "Impulse", OScMaxNameLength);
		break;
		
		case OScCollectionType_Ambient:
			UUrString_Copy(scm->collection_name, "Ambient", OScMaxNameLength);
		break;
	}
	
	// save the scm
	WMrDialog_SetUserData(inDialog, (UUtUns32)scm);
	
	// initialize the popup menu
	error = OWiSCM_Popup_Init(inDialog, scm);
	if (error != UUcError_None)
	{
		UUrMemory_Block_Delete(scm);
		WMrDialog_ModalEnd(inDialog, 0);
		return;
	}
	
	// Enable Buttons
	OWiSCM_Enable_Buttons(inDialog, scm);
}

// ----------------------------------------------------------------------
static void
OWiSCM_Destroy(
	WMtDialog					*inDialog)
{
	OWtSCM						*scm;

	// get a pointer to the scm
	scm = (OWtSCM*)WMrDialog_GetUserData(inDialog);
	
	// delete the data associated with the OWtSCM
	switch (OSrCollection_GetType(scm->collection))
	{
		case OScCollectionType_Group:
			SSrGroup_Delete(scm->clip_ss_id);
		break;

		case OScCollectionType_Impulse:
			SSrImpulse_Delete(scm->clip_ss_id);
		break;
		
		case OScCollectionType_Ambient:
			SSrAmbient_Delete(scm->clip_ss_id);
		break;
	}
	scm->clip_ss_id = SScInvalidID;
	
	// delete the scm
	UUrMemory_Block_Delete(scm);
	WMrDialog_SetUserData(inDialog, 0);
}

// ----------------------------------------------------------------------
static void
OWiSCM_HandleCommand(
	WMtDialog					*inDialog,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	OWtSCM						*scm;
	UUtUns16					command_type;
	UUtUns16					control_id;
	WMtWindow					*listbox;
	UUtUns32					result;
	UUtUns32					item_id;

	// get a pointer to the scm
	scm = (OWtSCM*)WMrDialog_GetUserData(inDialog);
	UUmAssert(scm);

	// interpret inParam1
	command_type = UUmHighWord(inParam1);
	control_id = UUmLowWord(inParam1);
	
	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcSCM_LB_Items);
	UUmAssert(listbox);
	
	// get the selected item's item_id
	result = WMrMessage_Send(listbox, LBcMessage_GetText, (UUtUns32)&item_id, (UUtUns32)-1);
	if (result == LBcError) { item_id = UUcMaxUns32; }
		
	if (control_id == OWcSCM_LB_Items)
	{
		switch (command_type)
		{
			case LBcNotify_SelectionChanged:
				OWiSCM_Enable_Buttons(inDialog, scm);
			break;
			
			case WMcNotify_DoubleClick:
				if (item_id & OWcCatToData)
				{
					OWiSCM_OpenCategory(inDialog, scm, (item_id & OWcDataToCat));
				}
				else
				{
					OWiSCM_EditItem(inDialog, scm, item_id);
				}
			break;
		}
	}
	else if (command_type == WMcNotify_Click)
	{
		switch (control_id)
		{
			case WMcDialogItem_Cancel:
				WMrDialog_ModalEnd(inDialog, 0);
			break;

			case OWcSCM_Btn_NewItem:
				OWiSCM_NewItem(inDialog, scm);
			break;
			
			case OWcSCM_Btn_EditItem:
				OWiSCM_EditItem(inDialog, scm, item_id);
			break;
			
			case OWcSCM_Btn_PlayItem:
				OWiSCM_PlayItem(inDialog, scm, item_id);
			break;
			
			case OWcSCM_Btn_DeleteItem:
				OWiSCM_DeleteItem(inDialog, scm, item_id);
			break;
			
			case OWcSCM_Btn_CopyItem:
				OWiSCM_CopyItem(inDialog, scm, item_id);
			break;
			
			case OWcSCM_Btn_PasteItem:
				OWiSCM_PasteItem(inDialog, scm, item_id);
			break;
			
			case OWcSCM_Btn_NewCategory:
				OWiSCM_NewCategory(inDialog, scm);
			break;
			
			case OWcSCM_Btn_RenameCategory:
				OWiSCM_RenameCategory(inDialog, scm, (item_id & OWcDataToCat));
			break;
			
			case OWcSCM_Btn_DeleteCategory:
				OWiSCM_DeleteCategory(inDialog, scm, (item_id & OWcDataToCat));
			break;
		}
	}
}

// ----------------------------------------------------------------------
static void
OWiSCM_HandleMenuCommand(
	WMtDialog					*inDialog,
	WMtWindow					*inPopupMenu,
	UUtUns32					inItemID)
{
	OWtSCM						*scm;
	UUtUns32					i;
	UUtUns32					num_items;
	UUtBool						result;
	UUtUns16					item_id;
		
	// go through the popup menu items and delete the ones after inItemID
	num_items = 0;
	for (i = 0;; i++)
	{
		result = WMrPopupMenu_GetItemID(inPopupMenu, (UUtInt16)i, &item_id);
		if (result == UUcFalse) { break; }
		num_items++;
	}
	
	for (i = (num_items - 1); ; i--)
	{
		// get the item id
		result = WMrPopupMenu_GetItemID(inPopupMenu, (UUtInt16)i, &item_id);
		if (result == UUcFalse) { break; }
		if (item_id == inItemID) { break; }
		
		WMrPopupMenu_RemoveItem(inPopupMenu, item_id);
	}
	
	// get a pointer to the scm
	scm = (OWtSCM*)WMrDialog_GetUserData(inDialog);
	UUmAssert(scm);
		
	// fill the listbox
	OWiSCM_Listbox_Fill(inDialog, scm);
}

// ----------------------------------------------------------------------
static void
OWiSCM_HandleDrawItem(
	WMtDialog					*inDialog,
	WMtDrawItem					*inDrawItem)
{
	OWtSCM						*scm;
	IMtPoint2D					dest;
	PStPartSpecUI				*partspec_ui;
	UUtInt16					line_width;
	UUtInt16					line_height;
	
	// get a pointer to the scm
	scm = (OWtSCM*)WMrDialog_GetUserData(inDialog);
	UUmAssert(scm);
	
	// get a pointer to the partspec_ui
	partspec_ui = PSrPartSpecUI_GetActive();
	UUmAssert(partspec_ui);
	
	// set the dest
	dest.x = inDrawItem->rect.left;
	dest.y = inDrawItem->rect.top;
	
	// calc the width and height of the line
	line_width = inDrawItem->rect.right - inDrawItem->rect.left;
	line_height = inDrawItem->rect.bottom - inDrawItem->rect.top;
	
/*	if (inDrawItem->state & WMcDrawItemState_Selected)
	{
		// draw the hilite
		DCrDraw_PartSpec(
			inDrawItem->draw_context,
			partspec_ui->hilite,
			PScPart_All,
			&dest,
			line_width,
			line_height,
			M3cMaxAlpha);
	}*/
	
	// set the text destination
	dest.x += 2;
	dest.y += 1;
	
	// draw the text
	DCrText_SetFormat(TSc_HLeft | TSc_VCenter);
	DCrText_SetStyle(TScStyle_Plain);
	DCrText_SetShade(IMcShade_Black);
	if (inDrawItem->data & OWcCatToData)
	{
		OStCategory				*category;
		
		category = 
			OSrCollection_Category_GetByID(
				scm->collection,
				(inDrawItem->data & OWcDataToCat));
		if (category == NULL) { return; }
		
		// draw the folder
		DCrDraw_PartSpec(
			inDrawItem->draw_context,
			partspec_ui->folder,
			PScPart_MiddleMiddle,
			&dest,
			(line_height - 3),
			(line_height - 3),
			M3cMaxAlpha);
		
		// draw the name
		dest.x += line_height + 2;
		DCrDraw_String(
			inDrawItem->draw_context,
			OSrCategory_GetName(category),
			&inDrawItem->rect,
			&dest);
	}
	else
	{
		OStItem					*item;
		
		item = OSrCollection_Item_GetByID(scm->collection, inDrawItem->data);
		if (item == NULL) { return; }

		// draw the file
		DCrDraw_PartSpec(
			inDrawItem->draw_context,
			partspec_ui->file,
			PScPart_MiddleMiddle,
			&dest,
			(line_height - 3),
			(line_height - 3),
			M3cMaxAlpha);
		
		// draw the name
		dest.x += line_height + 2;
		DCrDraw_String(
			inDrawItem->draw_context,
			OSrItem_GetName(item),
			&inDrawItem->rect,
			&dest);
	}
}

// ----------------------------------------------------------------------
static UUtBool
OWiSCM_HandleCompareItems(
	WMtDialog					*inDialog,
	WMtCompareItems				*inCompareItems)
{
	OWtSCM						*scm;
	UUtInt32					result;
	
	// get a pointer to the scm
	scm = (OWtSCM*)WMrDialog_GetUserData(inDialog);
	UUmAssert(scm);
	
	// is one of the items a directory?
	if ((inCompareItems->item1_data & OWcCatToData) != 
		(inCompareItems->item2_data & OWcCatToData))
	{
		if (inCompareItems->item1_data & OWcCatToData)
		{
			// item 1 is a directory, so item1 comes before item2
			// because directories come before files
			result = -1;
		}
		else
		{
			// item 2 is a directory, so item1 comes after item2
			// because directories come before files
			result = 1;
		}
	}
	else if ((inCompareItems->item1_data & OWcCatToData) != 0)
	{
		OStCategory					*category1;
		OStCategory					*category2;
		
		// compare the categories
		category1 =
			OSrCollection_Category_GetByID(
				scm->collection,
				(inCompareItems->item1_data & OWcDataToCat));
		UUmAssert(category1);
		
		category2 =
			OSrCollection_Category_GetByID(
				scm->collection,
				(inCompareItems->item2_data & OWcDataToCat));
		UUmAssert(category2);
		
		result =
			UUrString_Compare_NoCase(
				OSrCategory_GetName(category1),
				OSrCategory_GetName(category2));
	}
	else
	{
		OStItem						*item1;
		OStItem						*item2;
		
		// compare the items
		item1 =	OSrCollection_Item_GetByID(scm->collection, inCompareItems->item1_data);
		UUmAssert(item1);
		
		item2 = OSrCollection_Item_GetByID(scm->collection, inCompareItems->item2_data);
		UUmAssert(item2);
		
		result = UUrString_Compare_NoCase(OSrItem_GetName(item1), OSrItem_GetName(item2));
	}
	
	return (UUtBool)result;
}

// ----------------------------------------------------------------------
static UUtBool
OWiSCM_Callback(
	WMtDialog					*inDialog,
	WMtMessage					inMessage,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	UUtBool						handled;
	
	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiSCM_InitDialog(inDialog);
		break;
		
		case WMcMessage_Destroy:
			OWiSCM_Destroy(inDialog);
		break;
		
		case WMcMessage_Command:
			OWiSCM_HandleCommand(inDialog, inParam1, inParam2);
		break;
		
		case WMcMessage_MenuCommand:
			OWiSCM_HandleMenuCommand(
				inDialog,
				(WMtWindow*)inParam2,
				(UUtUns32)UUmLowWord(inParam1));
		break;
		
		case WMcMessage_DrawItem:
			OWiSCM_HandleDrawItem(inDialog, (WMtDrawItem*)inParam2);
		break;
		
		case WMcMessage_CompareItems:
			handled = OWiSCM_HandleCompareItems(inDialog, (WMtCompareItems*)inParam2);
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ----------------------------------------------------------------------
UUtError
OWrSCM_Display(
	OStCollectionType			inCollectionType)
{
	UUtError					error;
	WMtDialogID					dialog_id;
	
	if (OSrCollection_IsLocked(OSrCollection_GetByType(inCollectionType)) == UUcTrue)
	{
		UUtUns32				result;
		result = 
			WMrDialog_MessageBox(
				NULL,
				"Unable to Save",
				"You may make changes but you will not be able to save them. Do you want to continue?",
				WMcMessageBoxStyle_Yes_No);
		if (result == WMcDialogItem_No) { return UUcError_None; }
	}

	switch (inCollectionType)
	{
		case OScCollectionType_Group:
			dialog_id = OWcDialog_Sound_Group_Manager;
		break;
		
		case OScCollectionType_Impulse:
			dialog_id = OWcDialog_Impulse_Sound_Manager;
		break;
		
		case OScCollectionType_Ambient:
			dialog_id = OWcDialog_Ambient_Sound_Manager;
		break;
	}
	
	// display the Sound Group Manager Dialog
	error =
		WMrDialog_ModalBegin(
			dialog_id,
			NULL,
			OWiSCM_Callback,
			(UUtUns32)inCollectionType,
			NULL);
	
	// save
	OSrSave();
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OWiSS_Enable_Buttons(
	WMtDialog					*inDialog)
{
	WMtWindow					*button;
	WMtWindow					*listbox;
	UUtUns32					item_id;
	
	button = WMrDialog_GetItemByID(inDialog, OWcSS_Btn_Select);
	WMrWindow_SetEnabled(button, UUcFalse);
	
	listbox = WMrDialog_GetItemByID(inDialog, OWcSS_LB_Items);
	item_id = WMrMessage_Send(listbox, LBcMessage_GetItemData, 0, (UUtUns32)-1);
	if ((item_id != SScInvalidID) && ((item_id & OWcCatToData) == 0))
	{
		WMrWindow_SetEnabled(button, UUcTrue);
	}
}

// ----------------------------------------------------------------------
static void
OWiSS_InitDialog(
	WMtDialog					*inDialog)
{
	UUtError					error;
	OWtSCM						*scm;
	OWtSS						*ss;
	
	// get the user data
	ss = (OWtSS*)WMrDialog_GetUserData(inDialog);
		
	// get a pointer to the scm
	scm = (OWtSCM*)UUrMemory_Block_New(sizeof(OWtSCM));
	if (scm == NULL) { goto cleanup; }
	
	// initialize the scm
	scm->collection = OSrCollection_GetByType(ss->type);
	scm->clip_ss_id = (UUtUns32)ss;
	scm->clip_name[0] = '\0';
	
	switch (ss->type)
	{
		case OScCollectionType_Group:
			UUrString_Copy(scm->collection_name, "Group", OScMaxNameLength);
		break;

		case OScCollectionType_Ambient:
			UUrString_Copy(scm->collection_name, "Ambient", OScMaxNameLength);
		break;

		case OScCollectionType_Impulse:
			UUrString_Copy(scm->collection_name, "Impulse", OScMaxNameLength);
		break;
	}
	
	// save the scm
	WMrDialog_SetUserData(inDialog, (UUtUns32)scm);
	
	// initialize the popup menu
	error = OWiSCM_Popup_Init(inDialog, scm);
	if (error != UUcError_None)
	{
		WMrDialog_ModalEnd(inDialog, OWcSS_Btn_Cancel);
		return;
	}
	
	// Enable Buttons
	OWiSS_Enable_Buttons(inDialog);
	
	return;
	
cleanup:
	if (scm != NULL)
	{
		ss->item_id = SScInvalidID;
		
		UUrMemory_Block_Delete(scm);
		WMrDialog_ModalEnd(inDialog, OWcSS_Btn_Cancel);
	}
}

// ----------------------------------------------------------------------
static void
OWiSS_Destroy(
	WMtDialog					*inDialog)
{
	OWtSCM						*scm;
	
	// get a pointer to the scm
	scm = (OWtSCM*)WMrDialog_GetUserData(inDialog);
	
	// restore the out_ss_id
	WMrDialog_SetUserData(inDialog, scm->clip_ss_id);
	
	// dispose of the scm
	UUrMemory_Block_Delete(scm);
}

// ----------------------------------------------------------------------
static void
OWiSS_HandleCommand(
	WMtDialog					*inDialog,
	UUtUns32					inParam1,
	WMtWindow					*inControl)
{
	OWtSCM						*scm;
	UUtUns16					control_id;
	UUtUns16					command_type;
	WMtWindow					*listbox;
	UUtUns32					result;
	UUtUns32					item_id;
	OWtSS						*ss;
		
	// get a pointer to the scm
	scm = (OWtSCM*)WMrDialog_GetUserData(inDialog);
	UUmAssert(scm);
	
	// interpret inParam1
	control_id = UUmLowWord(inParam1);
	command_type = UUmHighWord(inParam1);
	
	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcSCM_LB_Items);
	UUmAssert(listbox);
	
	// get the selected item's item_id
	result = WMrMessage_Send(listbox, LBcMessage_GetText, (UUtUns32)&item_id, (UUtUns32)-1);
	if (result == LBcError) { item_id = UUcMaxUns32; }
	
	// get ss
	ss = (OWtSS*)scm->clip_ss_id;
	
	// handle the command
	if (control_id == OWcSCM_LB_Items)
	{
		switch (command_type)
		{
			case LBcNotify_SelectionChanged:
				OWiSS_Enable_Buttons(inDialog);
			break;
			
			case WMcNotify_DoubleClick:
				if (item_id & OWcCatToData)
				{
					OWiSCM_OpenCategory(inDialog, scm, (item_id & OWcDataToCat));
				}
				else
				{
					ss->item_id = item_id;
					WMrDialog_ModalEnd(inDialog, OWcSS_Btn_Select);
				}
			break;
		}
	}
	else if (command_type == WMcNotify_Click)
	{
		switch (UUmLowWord(inParam1))
		{
			case OWcSS_Btn_None:
				WMrDialog_ModalEnd(inDialog, OWcSS_Btn_None);
			break;
			
			case OWcSS_Btn_New:
				OWrSCM_Display(ss->type);
				OWiSCM_Listbox_Fill(inDialog, scm);
			break;

			case OWcSS_Btn_Cancel:
				WMrDialog_ModalEnd(inDialog, OWcSS_Btn_Cancel);
			break;
			
			case OWcSS_Btn_Select:
				ss->item_id = item_id;
				WMrDialog_ModalEnd(inDialog, OWcSS_Btn_Select);
			break;
		}
	}
}

// ----------------------------------------------------------------------
static UUtBool
OWiSS_Callback(
	WMtDialog					*inDialog,
	WMtMessage					inMessage,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	UUtBool						handled;
	
	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiSS_InitDialog(inDialog);
		break;
		
		case WMcMessage_Destroy:
			OWiSS_Destroy(inDialog);
		break;
		
		case WMcMessage_Command:
			OWiSS_HandleCommand(inDialog, inParam1, (WMtWindow*)inParam2);
		break;
		
		case WMcMessage_MenuCommand:
			OWiSCM_HandleMenuCommand(
				inDialog,
				(WMtWindow*)inParam2,
				(UUtUns32)UUmLowWord(inParam1));
		break;

		case WMcMessage_DrawItem:
			OWiSCM_HandleDrawItem(inDialog, (WMtDrawItem*)inParam2);
		break;
		
		case WMcMessage_CompareItems:
			handled = OWiSCM_HandleCompareItems(inDialog, (WMtCompareItems*)inParam2);
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ----------------------------------------------------------------------
static UUtError
OWiSSG_Display(
	WMtDialog					*inParentDialog,
	UUtUns32					*ioSoundGroupID,
	SStGroup					**ioSoundGroup)
{
	UUtError					error;
	UUtUns32					message;
	OWtSS						ss;
	
	UUmAssert(ioSoundGroupID || ioSoundGroup);
	
	ss.type = OScCollectionType_Group;

	// select a group
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Select_Sound,
			inParentDialog,
			OWiSS_Callback,
			(UUtUns32)&ss,
			&message);
	UUmError_ReturnOnError(error);
	
	if (message == OWcSS_Btn_Select)
	{
		if (ioSoundGroupID) { *ioSoundGroupID = ss.item_id; }
		if (ioSoundGroup) { *ioSoundGroup = SSrGroup_GetByID(ss.item_id); }
	}
	else if (message == OWcSS_Btn_None)
	{
		if (ioSoundGroupID) { *ioSoundGroupID = SScInvalidID; }
		if (ioSoundGroup) { *ioSoundGroup = NULL; }
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OWrSAS_Display(
	WMtDialog					*inParentDialog,
	UUtUns32					*ioAmbientSoundID,
	SStAmbient					**ioAmbientSound)
{
	UUtError					error;
	UUtUns32					message;
	OWtSS						ss;
	
	UUmAssert(ioAmbientSoundID || ioAmbientSound);
	
	ss.type = OScCollectionType_Ambient;
	
	// select a group
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Select_Sound,
			inParentDialog,
			OWiSS_Callback,
			(UUtUns32)&ss,
			&message);
	UUmError_ReturnOnError(error);
	
	if (message == OWcSS_Btn_Select)
	{
		if (ioAmbientSoundID) { *ioAmbientSoundID = ss.item_id; }
		if (ioAmbientSound) { *ioAmbientSound = SSrAmbient_GetByID(ss.item_id); }
	}
	else if (message == OWcSS_Btn_None)
	{
		if (ioAmbientSoundID) { *ioAmbientSoundID = SScInvalidID; }
		if (ioAmbientSound) { *ioAmbientSound = NULL; }
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OWrSIS_Display(
	WMtDialog					*inParentDialog,
	UUtUns32					*ioImpulseSoundID,
	SStImpulse					**ioImpulseSound)
{
	UUtError					error;
	UUtUns32					message;
	OWtSS						ss;
	
	UUmAssert(ioImpulseSoundID || ioImpulseSound);
	
	ss.type = OScCollectionType_Impulse;
	
	// select a group
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Select_Sound,
			inParentDialog,
			OWiSS_Callback,
			(UUtUns32)&ss,
			&message);
	UUmError_ReturnOnError(error);
	
	if (message == OWcSS_Btn_Select)
	{
		if (ioImpulseSoundID) { *ioImpulseSoundID = ss.item_id; }
		if (ioImpulseSound) { *ioImpulseSound = SSrImpulse_GetByID(ss.item_id); }
	}
	else if (message == OWcSS_Btn_None)
	{
		if (ioImpulseSoundID) { *ioImpulseSoundID = SScInvalidID; }
		if (ioImpulseSound) { *ioImpulseSound = NULL; }
	}
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
OWiSelectAnimation_InitDialog(
	WMtDialog					*inDialog)
{
	OWtSA						*sa;
	UUtUns32					num_anims;
	UUtError					error;
	WMtWindow					*listbox;
	WMtWindow					*progressbar;
	UUtUns32					i;
	
	// get a pointer to the user data
	sa = (OWtSA*)WMrDialog_GetUserData(inDialog);
	sa->insert_from_index = 0;
	
	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcSA_LB_Animations);
//	WMrWindow_SetVisible(listbox, UUcFalse);
	
	// get the number of animations
	sa->num_animations = TMrInstance_GetTagCount(TRcTemplate_Animation);
	if (sa->num_animations == 0) { return UUcError_Generic; }
	
	sa->animation_list = (TRtAnimation**)UUrMemory_Block_New(sizeof(TRtAnimation*) * sa->num_animations);
	if (sa->animation_list == NULL) { return UUcError_Generic; }
	
	// build the list of pointers to the animations
	error =
		TMrInstance_GetDataPtr_List(
			TRcTemplate_Animation,
			sa->num_animations,
			&num_anims,
			sa->animation_list);
	UUmError_ReturnOnError(error);
	UUmAssert(sa->num_animations == num_anims);
	
	// set the number of lines the listbox is going to have
	WMrMessage_Send(listbox, LBcMessage_SetNumLines, sa->num_animations, 0);
	
	// create a timer
//	WMrTimer_Start(0, 1, inDialog);
	
	// add lines to the dialog
	for (i = 0; i < sa->num_animations; i++)
	{
		WMrMessage_Send(listbox, LBcMessage_AddString, 0, 0);
	}
	
	WMrMessage_Send(listbox, LBcMessage_SetSelection, (UUtUns32)UUcFalse, 0);

	// init the progressbar
	progressbar = WMrDialog_GetItemByID(inDialog, OWcSA_PB_LoadProgress);
//	WMrMessage_Send(progressbar, WMcMessage_SetValue, 0, 0);
	WMrWindow_SetVisible(progressbar, UUcFalse);
	
	// set the focus to the listbox
	WMrWindow_SetFocus(listbox);
	
	return UUcError_None;
}
/*
static UUtError
OWiSelectAnimation_InitDialog(
	WMtDialog					*inDialog)
{
	OWtSA						*sa;
	UUtUns32					num_anims;
	UUtError					error;
	WMtWindow					*listbox;
	WMtWindow					*progressbar;
	
	// get a pointer to the user data
	sa = (OWtSA*)WMrDialog_GetUserData(inDialog);
	sa->insert_from_index = 0;
	
	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcSA_LB_Animations);
	WMrWindow_SetVisible(listbox, UUcFalse);
	
	// get the number of animations
	sa->num_animations = TMrInstance_GetTagCount(TRcTemplate_Animation);
	if (sa->num_animations == 0) { return NULL; }
	
	sa->animation_list = (TRtAnimation**)UUrMemory_Block_New(sizeof(TRtAnimation*) * sa->num_animations);
	if (sa->animation_list == NULL) { return NULL; }
	
	// build the list of pointers to the animations
	error =
		TMrInstance_GetDataPtr_List(
			TRcTemplate_Animation,
			sa->num_animations,
			&num_anims,
			sa->animation_list);
	UUmError_ReturnOnError(error);
	UUmAssert(sa->num_animations == num_anims);
	
	// set the number of lines the listbox is going to have
	WMrMessage_Send(listbox, LBcMessage_SetNumLines, sa->num_animations, 0);

	// create a timer
	WMrTimer_Start(0, 1, inDialog);
	
	// init the progressbar
	progressbar = WMrDialog_GetItemByID(inDialog, OWcSA_PB_LoadProgress);
	WMrMessage_Send(progressbar, WMcMessage_SetValue, 0, 0);
	
	// set the focus to the listbox
	WMrWindow_SetFocus(listbox);
	
	return UUcError_None;
}
*/
// ----------------------------------------------------------------------
static void
OWiSelectAnimation_HandleDestroy(
	WMtDialog					*inDialog)
{
	OWtSA						*sa;
	
	sa = (OWtSA*)WMrDialog_GetUserData(inDialog);
	
	if (sa->animation_list != NULL)
	{
		UUrMemory_Block_Delete(sa->animation_list);
		sa->animation_list = NULL;
	}
}

// ----------------------------------------------------------------------
static void
OWiSelectAnimation_HandleSelect(
	WMtDialog					*inDialog,
	OWtSA						*inSA)
{
	WMtWindow					*listbox;
	UUtUns32					result;
	
	listbox = WMrDialog_GetItemByID(inDialog, OWcSA_LB_Animations);
	
	result = WMrMessage_Send(listbox, LBcMessage_GetSelection, 0, 0);
	if (result != LBcError)
	{
		inSA->animation = inSA->animation_list[result];
	}
	WMrDialog_ModalEnd(inDialog, OWcSA_Btn_Select);
}

// ----------------------------------------------------------------------
static void
OWiSelectAnimation_HandleCommand(
	WMtDialog					*inDialog,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	OWtSA						*sa;
	
	sa = (OWtSA*)WMrDialog_GetUserData(inDialog);
	
	switch(UUmLowWord(inParam1))
	{
		case OWcSA_LB_Animations:
			if (UUmHighWord(inParam1) == WMcNotify_DoubleClick)
			{
				OWiSelectAnimation_HandleSelect(inDialog, sa);
			}
		break;
		
		case OWcSA_Btn_None:
			sa->animation = NULL;
			WMrDialog_ModalEnd(inDialog, OWcSA_Btn_None);
		break;
		
		case OWcSA_Btn_Cancel:
			WMrDialog_ModalEnd(inDialog, OWcSA_Btn_Cancel);
		break;
		
		case OWcSA_Btn_Select:
			OWiSelectAnimation_HandleSelect(inDialog, sa);
		break;
	}
}

// ----------------------------------------------------------------------
static void
OWiSelectAnimation_HandleTimer(
	WMtDialog					*inDialog)
{
	OWtSA						*sa;
	UUtUns32					i;
	UUtUns32					stop_index;
	WMtWindow					*listbox;
	WMtWindow					*progressbar;
	UUtUns32					progress;
	
	sa = (OWtSA*)WMrDialog_GetUserData(inDialog);
	if (sa->insert_from_index == sa->num_animations) { return; }
	
	// calculate the stop_index
	stop_index = (sa->num_animations / 50) + sa->insert_from_index;
	if (stop_index > sa->num_animations) { stop_index = sa->num_animations; }
	
	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcSA_LB_Animations);
	
	// put the animations in the list
	for (i = sa->insert_from_index; i < stop_index; i++)
	{
		WMrMessage_Send(
			listbox,
			LBcMessage_AddString, 
			(UUtUns32)TMrInstance_GetInstanceName(sa->animation_list[i]),
			0);
	}
	
	// update the inser_from_index
	sa->insert_from_index = stop_index;
	
	// update the progress bar
	progressbar = WMrDialog_GetItemByID(inDialog, OWcSA_PB_LoadProgress);
	progress = (UUtUns32)(((float)sa->insert_from_index / (float)sa->num_animations) * 100.0f);
	WMrMessage_Send(progressbar, WMcMessage_SetValue, progress, 0);
	
	// if there are no more animations to add, select the current animation or the first
	// and stop the timer
	if (stop_index == sa->num_animations)
	{
		if (sa->animation)
		{
			WMrMessage_Send(
				listbox,
				LBcMessage_SelectString,
				0,
				(UUtUns32)TMrInstance_GetInstanceName(sa->animation));
		}
		else
		{
			WMrMessage_Send(listbox, LBcMessage_SetSelection, (UUtUns32)UUcFalse, 0);
		}
		
		// stop the timer
		WMrTimer_Stop(0, inDialog);
		
		// hide the progressbar
		WMrWindow_SetVisible(progressbar, UUcFalse);
		
		// show the listbox
		WMrWindow_SetVisible(listbox, UUcTrue);
		
		// dipose of the animation_list
		UUrMemory_Block_Delete(sa->animation_list);
		sa->animation_list = NULL;
	}
}

// ----------------------------------------------------------------------
static void
OWiSelectAnimation_HandleDrawItem(
	WMtDialog					*inDialog,
	WMtDrawItem					*inDrawItem)
{
	OWtSA						*sa;
	IMtPoint2D					dest;
//	PStPartSpecUI				*partspec_ui;
	UUtInt16					line_width;
	UUtInt16					line_height;
	TRtAnimation				*animation;

	sa = (OWtSA*)WMrDialog_GetUserData(inDialog);

	// get a pointer to the partspec_ui
//	partspec_ui = PSrPartSpecUI_GetActive();
//	UUmAssert(partspec_ui);
	
	// set the dest
	dest.x = inDrawItem->rect.left;
	dest.y = inDrawItem->rect.top;
	
	// calc the width and height of the line
	line_width = inDrawItem->rect.right - inDrawItem->rect.left;
	line_height = inDrawItem->rect.bottom - inDrawItem->rect.top;
	
/*	if (inDrawItem->state & WMcDrawItemState_Selected)
	{
		// draw the hilite
		DCrDraw_PartSpec(
			inDrawItem->draw_context,
			partspec_ui->hilite,
			PScPart_All,
			&dest,
			line_width,
			line_height,
			M3cMaxAlpha);
	}*/

	// draw the Text
	DCrText_SetFormat(TSc_HLeft | TSc_VCenter);
	DCrText_SetStyle(TScStyle_Plain);
	DCrText_SetShade(IMcShade_Black);
	
	dest.x = 5;
	dest.y += 1;
	
	animation = sa->animation_list[inDrawItem->item_id];
	
	DCrDraw_String(
		inDrawItem->draw_context,
		TMrInstance_GetInstanceName(animation),
		&inDrawItem->rect,
		&dest);
}

// ----------------------------------------------------------------------
static UUtBool
OWiSelectAnimation_Callback(
	WMtDialog					*inDialog,
	WMtMessage					inMessage,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	UUtBool						handled;
	UUtError					error;
	
	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			error = OWiSelectAnimation_InitDialog(inDialog);
			if (error != UUcError_None)
			{
				WMrDialog_ModalEnd(inDialog, OWcSA_Btn_Cancel);
			}
		break;
		
		case WMcMessage_Destroy:
			OWiSelectAnimation_HandleDestroy(inDialog);
		break;
		
		case WMcMessage_Command:
			OWiSelectAnimation_HandleCommand(inDialog, inParam1, inParam2);
		break;
		
		case WMcMessage_Timer:
			OWiSelectAnimation_HandleTimer(inDialog);
		break;
		
		case WMcMessage_DrawItem:
			OWiSelectAnimation_HandleDrawItem(inDialog, (WMtDrawItem*)inParam2);
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OWiStAP_EnableButtons(
	WMtDialog					*inDialog,
	OWtStAP						*inProperties)
{
	WMtWindow					*button;
	WMtWindow					*popup;
	WMtWindow					*editfield;
	WMtWindow					*text;
	
	button = WMrDialog_GetItemByID(inDialog, OWcStAP_Btn_SetAnim);
	WMrWindow_SetEnabled(button, (inProperties->anim_type == OScAnimType_Animation));
	
	popup = WMrDialog_GetItemByID(inDialog, OWcStAP_PM_Modifier);
	WMrWindow_SetEnabled(popup, (inProperties->anim_type != OScAnimType_Animation));
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcStAP_EF_Frame);
	WMrWindow_SetEnabled(editfield, (inProperties->anim_type == OScAnimType_Animation));
	
	text = WMrDialog_GetItemByID(inDialog, OWcStAP_Txt_Duration);
	WMrWindow_SetVisible(text, (inProperties->anim_type == OScAnimType_Animation));
}

// ----------------------------------------------------------------------
static void
OWiStAP_InitDialog(
	WMtDialog					*inDialog)
{
	OWtStAP						*properties;
	WMtWindow					*popup;
	WMtWindow					*editfield;
	UUtUns32					i;
	UUtError					error;
	OStAnimType					type;
	ONtVariantList				*variant_list;
	
	properties = (OWtStAP*)WMrDialog_GetUserData(inDialog);
	if (properties == NULL)
	{
		WMrDialog_ModalEnd(inDialog, OWcStAP_Btn_Cancel);
		return;
	}
	
	// build the variant popup menu
	popup = WMrDialog_GetItemByID(inDialog, OWcStAP_PM_Variant);
	error =
		TMrInstance_GetDataPtr(
			ONcTemplate_VariantList,
			"variant_list",
			&variant_list);
	if (error != UUcError_None)
	{
		WMrDialog_ModalEnd(inDialog, OWcStAP_Btn_Cancel);
		return;
	}
			
	// add the variants to the popup
	for (i = 0; i < variant_list->numVariants; i++)
	{
		WMrPopupMenu_AppendItem_Light(popup, (UUtUns16)i, variant_list->variant[i]->name);
	}
		
	// set the variant
	if (properties->variant_name[0] == '\0')
	{
		WMrPopupMenu_SetSelection(popup, 0);
	}
	else
	{
		error = WMrPopupMenu_SelectString(popup, properties->variant_name);
		if (error != UUcError_None)
		{
			WMrPopupMenu_SetSelection(popup, 0);
		}
	}
	
	// build the anim type popup menu
	popup = WMrDialog_GetItemByID(inDialog, OWcStAP_PM_AnimType);
	for (type = 0; type < OScAnimType_NumTypes; type++)
	{
		const char				*anim_name;
		
		anim_name = OSrAnimType_GetName(type);
		WMrPopupMenu_AppendItem_Light(popup, (UUtUns16)type, anim_name);
	}
	
	// set the anim type
	error = WMrPopupMenu_SetSelection(popup, (UUtUns16)properties->anim_type);
	if (error != UUcError_None)
	{
		WMrPopupMenu_SetSelection(popup, 0);
	}
	
	// set the mod type
	popup = WMrDialog_GetItemByID(inDialog, OWcStAP_PM_Modifier);
	error = WMrPopupMenu_SetSelection(popup, (UUtUns16)properties->mod_type);
	if (error != UUcError_None)
	{
		WMrPopupMenu_SetSelection(popup, 0);
	}
	
	// set the frame number
	editfield = WMrDialog_GetItemByID(inDialog, OWcStAP_EF_Frame);
	WMrEditField_SetInt32(editfield, (UUtInt32)properties->frame);
	
	// set the anim name edit field
	if (properties->anim_type == OScAnimType_Animation)
	{
		TRtAnimTime					duration;
		char						string[128];
		
		// set the animation name
		WMrWindow_SetTitle(
			WMrDialog_GetItemByID(inDialog, OWcStAP_Txt_AnimName),
			TMrInstance_GetInstanceName(properties->animation),
			OScMaxAnimNameLength);

		// set the number of frames
		duration = TRrAnimation_GetDuration(properties->animation);
		sprintf(string, "%d", duration);
		WMrWindow_SetTitle(
			WMrDialog_GetItemByID(inDialog, OWcStAP_Txt_Duration),
			string,
			OScMaxAnimNameLength);
	}
	else
	{
		// set the animation name
		WMrWindow_SetTitle(
			WMrDialog_GetItemByID(inDialog, OWcStAP_Txt_AnimName),
			"",
			OScMaxAnimNameLength);

		// set the number of frames
		WMrWindow_SetTitle(
			WMrDialog_GetItemByID(inDialog, OWcStAP_Txt_Duration),
			"0",
			OScMaxAnimNameLength);
	}
	
	// set the impulse edit field
	if ((properties->impulse_id != SScInvalidID) &&(SSrImpulse_GetByID(properties->impulse_id) != NULL))
	{
		OStItem					*item;
		
		item =
			OSrCollection_Item_GetByID(
				OSrCollection_GetByType(OScCollectionType_Impulse),
				properties->impulse_id);

		WMrWindow_SetTitle(
			WMrDialog_GetItemByID(inDialog, OWcStAP_Txt_SoundName),
			OSrItem_GetName(item),
			OScMaxNameLength);
	}
	else
	{
		WMrWindow_SetTitle(
			WMrDialog_GetItemByID(inDialog, OWcStAP_Txt_SoundName),
			"",
			OScMaxNameLength);
	}
	
	// update the controls
	OWiStAP_EnableButtons(inDialog, properties);
}

// ----------------------------------------------------------------------
static void
OWiStAP_HandleCommand(
	WMtDialog					*inDialog,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	OWtStAP						*properties;
	UUtError					error;
	
	properties = (OWtStAP*)WMrDialog_GetUserData(inDialog);
	
	switch (UUmLowWord(inParam1))
	{
		case OWcStAP_Btn_SetAnim:
		{
			OWtSA				sa;
			UUtUns32			message;
			
			sa.animation = properties->animation;
			
			// select an animation
			error =
				WMrDialog_ModalBegin(
					OWcDialog_Select_Animation,
					inDialog,
					OWiSelectAnimation_Callback,
					(UUtUns32)&sa,
					&message);
			if (error != UUcError_None) { break; }
			
			if (message == OWcSA_Btn_None)
			{
				properties->animation = NULL;
			}
			else if (message == OWcSA_Btn_Select)
			{
				properties->animation = sa.animation;
			}
			
			if (properties->animation)
			{
				TRtAnimTime					duration;
				char						string[128];
				
				// set the animation name
				WMrWindow_SetTitle(
					WMrDialog_GetItemByID(inDialog, OWcStAP_Txt_AnimName),
					TMrInstance_GetInstanceName(properties->animation),
					OScMaxAnimNameLength);

				// set the number of frames
				duration = TRrAnimation_GetDuration(properties->animation);
				sprintf(string, "%d", duration);
				WMrWindow_SetTitle(
					WMrDialog_GetItemByID(inDialog, OWcStAP_Txt_Duration),
					string,
					OScMaxAnimNameLength);
			}
			else
			{
				// set the animation name
				WMrWindow_SetTitle(
					WMrDialog_GetItemByID(inDialog, OWcStAP_Txt_AnimName),
					"",
					OScMaxAnimNameLength);

				// set the number of frames
				WMrWindow_SetTitle(
					WMrDialog_GetItemByID(inDialog, OWcStAP_Txt_Duration),
					"0",
					OScMaxAnimNameLength);
			}
		}
		break;
		
		case OWcStAP_Btn_SetSound:
		{
			OStItem				*item;
			
			error = OWrSIS_Display(inDialog, &properties->impulse_id, NULL);
			if (error != UUcError_None) { break; }
			
			if (properties->impulse_id != SScInvalidID)
			{
				item =
					OSrCollection_Item_GetByID(
						OSrCollection_GetByType(OScCollectionType_Impulse),
						properties->impulse_id);
				if (item != NULL)
				{
					WMrWindow_SetTitle(
						WMrDialog_GetItemByID(inDialog, OWcStAP_Txt_SoundName),
						OSrItem_GetName(item),
						OScMaxNameLength);
				}
			}
			else
			{
				WMrWindow_SetTitle(
					WMrDialog_GetItemByID(inDialog, OWcStAP_Txt_SoundName),
					"",
					OScMaxNameLength);
			}
		}
		break;
		
		case OWcStAP_EF_Frame:
		{
			WMtWindow			*editfield;
			
			if (UUmHighWord(inParam1) != EFcNotify_ContentChanged) { break; }
			
			editfield = WMrDialog_GetItemByID(inDialog, OWcStAP_EF_Frame);
			properties->frame = (UUtUns32)WMrEditField_GetInt32(editfield);
		}
		break;
		
		case OWcStAP_Btn_Cancel:
			WMrDialog_ModalEnd(inDialog, OWcStAP_Btn_Cancel);
		break;
		
		case OWcStAP_Btn_Save:
			if ((properties->animation != NULL) &&
				(properties->frame > TRrAnimation_GetDuration(properties->animation)))
			{
				WMrDialog_MessageBox(
					inDialog,
					"Error",
					"The Frame number must be less than the number of frames in the animation.",
					WMcMessageBoxStyle_OK);
				WMrWindow_SetFocus(WMrDialog_GetItemByID(inDialog, OWcStAP_EF_Frame));
				break;
			}
			
			WMrDialog_ModalEnd(inDialog, OWcStAP_Btn_Save);
		break;
	}
}

// ----------------------------------------------------------------------
static void
OWiStAP_HandleMenuCommand(
	WMtDialog					*inDialog,
	WMtWindow					*inPopupMenu,
	UUtUns32					inItemID)
{
	OWtStAP						*properties;
	UUtUns32					item_id;
	
	properties = (OWtStAP*)WMrDialog_GetUserData(inDialog);
	item_id = (UUtUns32)(UUmLowWord(inItemID));
	
	switch (WMrWindow_GetID(inPopupMenu))
	{
		case OWcStAP_PM_Variant:
			if (item_id == 0)
			{
				UUrString_Copy(
					properties->variant_name,
					OScDefaultVariant,
					ONcMaxVariantNameLength);
			}
			else
			{
				ONtVariantList		*variant_list;
				
				TMrInstance_GetDataPtr(
					ONcTemplate_VariantList,
					"variant_list",
					&variant_list);

				// save the name of the selected variant
				UUrString_Copy(
					properties->variant_name,
					variant_list->variant[item_id]->name,
					ONcMaxVariantNameLength);
			}
		break;

		case OWcStAP_PM_AnimType:
			properties->anim_type = item_id;
		break;
		
		case OWcStAP_PM_Modifier:
			properties->mod_type = item_id;
		break;
	}
	
	// update the controls
	OWiStAP_EnableButtons(inDialog, properties);
}

// ----------------------------------------------------------------------
static UUtBool
OWiStAP_Callback(
	WMtDialog					*inDialog,
	WMtMessage					inMessage,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	UUtBool						handled;
	
	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiStAP_InitDialog(inDialog);
		break;
		
		case WMcMessage_Command:
			OWiStAP_HandleCommand(inDialog, inParam1, inParam2);
		break;
		
		case WMcMessage_MenuCommand:
			OWiStAP_HandleMenuCommand(inDialog, (WMtWindow*)inParam2, inParam1);
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OWiStA_FillListBox(
	WMtListBox					*inListBox)
{
	UUtUns32					i;
	UUtUns32					num_variants;
	
	WMrMessage_Send(inListBox, LBcMessage_Reset, 0, 0);

	num_variants = OSrVariantList_GetNumVariants();
	for (i = 0; i < num_variants; i++)
	{
		OStVariant					*variant;
		UUtUns32					num_sounds;
		UUtUns32					j;
		
		variant = OSrVariantList_Variant_GetByIndex(i);
		num_sounds = OSrVariant_GetNumSounds(variant);
		for (j = 0; j < num_sounds; j++)
		{
			UUtUns32					item_data;
			
			item_data = UUmMakeLong(i, j);
			WMrMessage_Send(inListBox, LBcMessage_AddString, (UUtUns32)item_data, 0);
		}
	}
}

// ----------------------------------------------------------------------
static void
OWiStA_HandleDelete(
	WMtDialog					*inDialog)
{
	WMtWindow					*listbox;
	UUtUns32					item_data;
	OStVariant					*variant;
	UUtBool						is_animation;
	UUtUns32					variant_index;
	
	listbox = WMrDialog_GetItemByID(inDialog, OWcStA_LB_Data);
	item_data = WMrMessage_Send(listbox, LBcMessage_GetItemData, 0, (UUtUns32)-1);
	
	is_animation = (item_data & 0x80000000) != 0;
	variant_index = UUmHighWord(item_data) & 0x7FFF;
	
	variant = OSrVariantList_Variant_GetByIndex(variant_index);
	if (variant == NULL) { return; }
	
	// delete the sound animation from the variant
	OSrVariant_SoundAnimation_DeleteByIndex(variant, UUmLowWord(item_data));
	
	// save the data
	OSrVariantList_Save(UUcFalse);
	
	// fill the listbox
	OWiStA_FillListBox(listbox);
}

// ----------------------------------------------------------------------
static void
OWiStA_HandlePlay(
	WMtDialog					*inDialog)
{
	WMtWindow					*listbox;
	UUtUns32					item_data;
	OStVariant					*variant;
	const OStSoundAnimation		*sound_animation;
	M3tPoint3D					position;
	M3tVector3D					facing;
	M3tVector3D					velocity;
	SStImpulse					*impulse;
	UUtBool						is_animation;
	UUtUns32					variant_index;
	
	listbox = WMrDialog_GetItemByID(inDialog, OWcStA_LB_Data);
	item_data = WMrMessage_Send(listbox, LBcMessage_GetItemData, 0, (UUtUns32)-1);
	
	is_animation = (item_data & 0x80000000) != 0;
	variant_index = UUmHighWord(item_data) & 0x7FFF;
	
	variant = OSrVariantList_Variant_GetByIndex(variant_index);
	if (variant == NULL) { return; }
	
	sound_animation = OSrVariant_SoundAnimation_GetByIndex(variant, UUmLowWord(item_data));
	if (sound_animation == NULL) { return; }
	
	// get the impulse sound
	impulse = SSrImpulse_GetByID(sound_animation->impulse_id);
	if (impulse == NULL) { return; }
	
	// set the listener position
	MUmVector_Set(position, 0.0f, 0.0f, 0.0f);
	MUmVector_Set(facing, 1.0f, 0.0f, 1.0f);
	SSrListener_SetPosition(&position, &facing);
	
	// play the sound
	MUmVector_Set(position, 5.0f, 0.0f, 5.0f);
	MUmVector_Set(velocity, 0.0f, 0.0f, 0.0f);
	SSrImpulse_Play(impulse, &position, &facing, &velocity);
}

// ----------------------------------------------------------------------
static UUtError
OWiStA_HandleEdit(
	WMtDialog					*inDialog)
{
	WMtWindow					*listbox;
	UUtUns32					item_data;
	OWtStAP						properties;
	OStVariant					*variant;
	const OStSoundAnimation		*sound_animation;
	UUtError					error;
	UUtUns32					message;
	UUtBool						is_animation;
	UUtUns32					variant_index;
	
	listbox = WMrDialog_GetItemByID(inDialog, OWcStA_LB_Data);
	item_data = WMrMessage_Send(listbox, LBcMessage_GetItemData, 0, (UUtUns32)-1);
	
	is_animation = (item_data & 0x80000000) != 0;
	variant_index = UUmHighWord(item_data) & 0x7FFF;
	
	variant = OSrVariantList_Variant_GetByIndex(variant_index);
	if (variant == NULL) { return UUcError_Generic; }
	
	sound_animation = OSrVariant_SoundAnimation_GetByIndex(variant, UUmLowWord(item_data));
	if (sound_animation == NULL) { return UUcError_Generic; }
	
	// initialize the properties
	properties.anim_type = sound_animation->anim_type;
	properties.mod_type = sound_animation->mod_type;
	properties.impulse_id = sound_animation->impulse_id;
	properties.frame = sound_animation->frame;
	properties.animation = sound_animation->animation;
	UUrString_Copy(
		properties.variant_name,
		OSrVariant_GetName(variant),
		ONcMaxVariantNameLength);
	
	// edit the properties
	error = 
		WMrDialog_ModalBegin(
			OWcDialog_Sound_to_Anim_Prop,
			inDialog,
			OWiStAP_Callback,
			(UUtUns32)&properties,
			&message);
	UUmError_ReturnOnError(error);
	
	if (message == OWcStAP_Btn_Save)
	{
		OStVariant				*new_variant;
		
		// delete the old sound variant
		OSrVariant_SoundAnimation_DeleteByIndex(variant, UUmLowWord(item_data));
				
		// if the variant changed, then delete the sound animation from the old variant
		new_variant = OSrVariantList_Variant_GetByName(properties.variant_name);
		if (new_variant == NULL) { return UUcError_Generic; }
			
		// add the sound animation to the new variant
		error =
			OSrVariant_SoundAnimation_Add(
				new_variant,
				properties.anim_type,
				properties.mod_type,
				TMrInstance_GetInstanceName(properties.animation),
				properties.frame,
				properties.impulse_id);
		UUmError_ReturnOnError(error);
		
		// save the data
		OSrVariantList_Save(UUcFalse);
	
		// fill in the listbox
		OWiStA_FillListBox(WMrDialog_GetItemByID(inDialog, OWcStA_LB_Data));
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OWiStA_HandleNew(
	WMtDialog					*inDialog)
{
	UUtError					error;
	UUtUns32					message;
	OWtStAP						properties;
	
	UUrMemory_Clear(&properties, sizeof(OWtStAP));
	properties.anim_type = OScAnimType_Any;
	properties.impulse_id = SScInvalidID;
	properties.animation = NULL;
	properties.frame = 0;
	properties.variant_name[0] = '\0';
	
	error = 
		WMrDialog_ModalBegin(
			OWcDialog_Sound_to_Anim_Prop,
			inDialog,
			OWiStAP_Callback,
			(UUtUns32)&properties,
			&message);
	UUmError_ReturnOnError(error);
	
	if (message == OWcStAP_Btn_Save)
	{
		OStVariant					*variant;
		
		variant = OSrVariantList_Variant_GetByName(properties.variant_name);
		if (variant == NULL) { return UUcError_Generic; }
		
		// add the item
		error =
			OSrVariant_SoundAnimation_Add(
				variant,
				properties.anim_type,
				properties.mod_type,
				TMrInstance_GetInstanceName(properties.animation),
				properties.frame,
				properties.impulse_id);
		UUmError_ReturnOnError(error);
		
		// save the data
		OSrVariantList_Save(UUcFalse);
		
		// fill in the listbox
		OWiStA_FillListBox(WMrDialog_GetItemByID(inDialog, OWcStA_LB_Data));
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OWiStA_InitDialog(
	WMtDialog					*inDialog)
{
	WMtWindow					*listbox;
	
	listbox = WMrDialog_GetItemByID(inDialog, OWcStA_LB_Data);

	// fill in the listbox
	OWiStA_FillListBox(listbox);
}

// ----------------------------------------------------------------------
static void
OWiStA_HandleCommand(
	WMtDialog					*inDialog,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	switch (UUmLowWord(inParam1))
	{
		case WMcDialogItem_Cancel:
			WMrDialog_ModalEnd(inDialog, 0);
		break;
		
		case OWcStA_LB_Data:
			if (UUmHighWord(inParam1) == WMcNotify_DoubleClick)
			{
				OWiStA_HandleEdit(inDialog);
			}
		break;
		
		case OWcStA_Btn_Delete:
			OWiStA_HandleDelete(inDialog);
		break;
		
		case OWcStA_Btn_Play:
			OWiStA_HandlePlay(inDialog);
		break;
		
		case OWcStA_Btn_Edit:
			OWiStA_HandleEdit(inDialog);
		break;
		
		case OWcStA_Btn_New:
			OWiStA_HandleNew(inDialog);
		break;
	}
}

// ----------------------------------------------------------------------
static void
OWiStA_HandleDrawItem(
	WMtDialog					*inDialog,
	WMtDrawItem					*inDrawItem)
{
	IMtPoint2D					dest;
	PStPartSpecUI				*partspec_ui;
	UUtInt16					line_width;
	UUtInt16					line_height;
	OStVariant					*variant;
	const OStSoundAnimation		*sound_animation;
	OStItem						*item;
	OStAnimType					anim_type;
	UUtUns32					impulse_id;
	UUtBool						is_animation;
	UUtUns32					variant_index;

	// get a pointer to the partspec_ui
	partspec_ui = PSrPartSpecUI_GetActive();
	UUmAssert(partspec_ui);
	
	// set the dest
	dest.x = inDrawItem->rect.left;
	dest.y = inDrawItem->rect.top;
	
	// calc the width and height of the line
	line_width = inDrawItem->rect.right - inDrawItem->rect.left;
	line_height = inDrawItem->rect.bottom - inDrawItem->rect.top;
	
/*	if (inDrawItem->state & WMcDrawItemState_Selected)
	{
		// draw the hilite
		DCrDraw_PartSpec(
			inDrawItem->draw_context,
			partspec_ui->hilite,
			PScPart_All,
			&dest,
			line_width,
			line_height,
			M3cMaxAlpha);
	}
	else*/ if (inDrawItem->rect.top > 5)
	{
		// draw a divider
		DCrDraw_PartSpec(
			inDrawItem->draw_context,
			partspec_ui->divider,
			PScPart_All,
			&dest,
			line_width,
			2,
			M3cMaxAlpha);
	}
	
	is_animation = (inDrawItem->data & 0x80000000) != 0;
	variant_index = UUmHighWord(inDrawItem->data) & 0x7FFF;
	
	// get the variant and animation sound
	variant = OSrVariantList_Variant_GetByIndex(variant_index);
	if (variant == NULL) { return; }
	
	sound_animation = OSrVariant_SoundAnimation_GetByIndex(variant, UUmLowWord(inDrawItem->data));
	if (sound_animation == NULL) { return; }
	
	// draw the Text
	DCrText_SetFormat(TSc_HLeft | TSc_VCenter);
	DCrText_SetStyle(TScStyle_Plain);
	DCrText_SetShade(IMcShade_Black);
	
	dest.x = 5;
	dest.y += 1;

	DCrDraw_String(
		inDrawItem->draw_context,
		OSrVariant_GetName(variant),
		&inDrawItem->rect,
		&dest);
	
	dest.x = 125;
	
	anim_type = sound_animation->anim_type;
	if (anim_type == OScAnimType_Animation)
	{
		TRtAnimation				*animation;
		
		animation = sound_animation->animation;
		if (animation == NULL)
		{
			DCrDraw_String(
				inDrawItem->draw_context,
				"",
				&inDrawItem->rect,
				&dest);
		}
		else
		{
			DCrDraw_String(
				inDrawItem->draw_context,
				TMrInstance_GetInstanceName(animation),
				&inDrawItem->rect,
				&dest);
		}
	}
	else
	{
		char							string[128];
		const char						*mod_name;
		const char						*anim_name;
		
		mod_name = OSrModType_GetName(sound_animation->mod_type);
		anim_name = OSrAnimType_GetName(anim_type);
		
		sprintf(string, "%s %s", mod_name, anim_name);
		DCrDraw_String(inDrawItem->draw_context, string, &inDrawItem->rect, &dest);
	}
	
	dest.x = 305;
	
	impulse_id = sound_animation->impulse_id;
	if ((impulse_id != SScInvalidID) && (SSrImpulse_GetByID(impulse_id) != NULL))
	{
		item =
			OSrCollection_Item_GetByID(
				OSrCollection_GetByType(OScCollectionType_Impulse),
				impulse_id);
		
		DCrDraw_String(
			inDrawItem->draw_context,
			OSrItem_GetName(item),
			&inDrawItem->rect,
			&dest);
	}
	else
	{
		DCrDraw_String(
			inDrawItem->draw_context,
			"",
			&inDrawItem->rect,
			&dest);
	}
}

// ----------------------------------------------------------------------
static UUtBool
OWiStA_Callback(
	WMtDialog					*inDialog,
	WMtMessage					inMessage,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	UUtBool						handled;
	
	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiStA_InitDialog(inDialog);
		break;
		
		case WMcMessage_Command:
			OWiStA_HandleCommand(inDialog, inParam1, inParam2);
		break;
		
		case WMcMessage_DrawItem:
			OWiStA_HandleDrawItem(inDialog, (WMtDrawItem*)inParam2);
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ----------------------------------------------------------------------
UUtError
OWrSoundToAnim_Display(
	void)
{
	UUtError					error;
	
	// display the sound to animation dialog
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Sound_to_Anim,
			NULL,
			OWiStA_Callback,
			0,
			NULL);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

