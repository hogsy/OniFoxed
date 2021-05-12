#include "BFW.h"

#if TOOL_VERSION
// ======================================================================
// Oni_Win_Sound2.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW_FileManager.h"
#include "BFW_SoundSystem2.h"
#include "BFW_WindowManager.h"
#include "WM_CheckBox.h"
#include "WM_EditField.h"
#include "WM_PopupMenu.h"
#include "WM_Text.h"

#include "Oni_Sound2.h"
#include "Oni_Sound_Animation.h"
#include "Oni_Windows.h"
#include "Oni_Win_Sound2.h"

// ======================================================================
// enums
// ======================================================================
// ------------------------------
// Sound Manager
// ------------------------------
enum
{
	OWcSM_PM_Category				= 100,
	OWcSM_LB_Items					= 101,
	OWcSM_Btn_NewItem				= 102,
	OWcSM_Btn_EditItem				= 103,
	OWcSM_Btn_PlayItem				= 104,
	OWcSM_Btn_DeleteItem			= 105,
	OWcSM_Btn_CopyItem				= 106,
	OWcSM_Btn_PasteItem				= 107,
	OWcSM_Btn_NewCategory			= 108,
	OWcSM_Btn_RenameCategory		= 109,
	OWcSM_Btn_DeleteCategory		= 110
};

// ------------------------------
// Category Name
// ------------------------------
enum
{
	OWcCN_EF_Name					= 100,
	OWcCN_Btn_OK					= WMcDialogItem_OK,
	OWcCN_Btn_Cancel				= WMcDialogItem_Cancel
};

// ------------------------------
// Sound Group Properties
// ------------------------------
enum
{
	OWcSGP_EF_Name					= 100,
	OWcSGP_LB_Permutations			= 101,
	OWcSGP_Btn_AddPerm				= 102,
	OWcSGP_Btn_EditPerm				= 103,
	OWcSGP_Btn_PlayPerm				= 104,
	OWcSGP_Btn_DeletePerm			= 105,
	OWcSGP_Btn_Play					= 106,
	OWcSGP_EF_Volume				= 107,
	OWcSGP_EF_Pitch					= 108,
	OWcSGP_CB_PreventRepeats		= 109,
	OWcSGP_Btn_Cancel				= WMcDialogItem_Cancel,
	OWcSGP_Btn_Save					= WMcDialogItem_OK
};

// ------------------------------
// Sound Permutation Properties
// ------------------------------
enum
{
	OWcSPP_Txt_Name					= 100,
	OWcSPP_EF_Weight				= 101,
	OWcSPP_EF_MinVol				= 102,
	OWcSPP_EF_MaxVol				= 103,
	OWcSPP_EF_MinPitch				= 104,
	OWcSPP_EF_MaxPitch				= 105,
	OWcSPP_Btn_Save					= WMcDialogItem_OK,
	OWcSPP_Btn_Cancel				= WMcDialogItem_Cancel
};

// ------------------------------
// WAV Select
// ------------------------------
enum
{
	OWcWS_PM_Directory				= 100,
	OWcWS_LB_DirContents			= 101,
	OWcWS_Btn_Select				= WMcDialogItem_OK,
	OWcWS_Btn_Cancel				= WMcDialogItem_Cancel
};

// ------------------------------
// Ambient Sound Properties
// ------------------------------
enum
{
	OWcASP_EF_Name					= 100,
	OWcASP_Txt_InGroup				= 101,
	OWcASP_Btn_InGroupSet			= 102,
	OWcASP_Btn_InGroupEdit			= 103,
	OWcASP_Txt_OutGroup				= 104,
	OWcASP_Btn_OutGroupSet			= 105,
	OWcASP_Btn_OutGroupEdit			= 106,
	OWcASP_Txt_BT1					= 107,
	OWcASP_Btn_BT1Set				= 108,
	OWcASP_Btn_BT1Edit				= 109,
	OWcASP_Txt_BT2					= 110,
	OWcASP_Btn_BT2Set				= 111,
	OWcASP_Btn_BT2Edit				= 112,
	OWcASP_Txt_Detail				= 113,
	OWcASP_Btn_DetailSet			= 114,
	OWcASP_Btn_DetailEdit			= 115,
	OWcASP_CB_InterruptOnStop		= 116,
	OWcASP_EF_MinElapsedTime		= 117,
	OWcASP_EF_MaxElapsedTime		= 118,
	OWcASP_EF_SphereRadius			= 119,
	OWcASP_Btn_Inc1					= 120,
	OWcASP_Btn_Dec1					= 121,
	OWcASP_Btn_Inc10				= 122,
	OWcASP_Btn_Dec10				= 123,
	OWcASP_EF_MinVolDist			= 124,
	OWcASP_EF_MaxVolDist			= 125,
	OWcASP_Btn_Start				= 126,
	OWcASP_Btn_Stop					= 127,
	OWcASP_PM_Priority				= 128,
	OWcASP_CB_PlayOnce				= 129,
	OWcASP_CB_CanPan				= 130,
	OWcASP_EF_Threshold				= 131,
	OWcASP_EF_MinOcclusion			= 132,
	OWcASP_Btn_Cancel				= WMcDialogItem_Cancel,
	OWcASP_Btn_Save					= WMcDialogItem_OK
};

// ------------------------------
// Impulse Sound Properties
// ------------------------------
enum
{
	OWcISP_EF_Name					= 100,
	OWcISP_Txt_Group				= 101,
	OWcISP_Btn_Set					= 102,
	OWcISP_Btn_Edit					= 103,
	OWcISP_PM_Priority				= 104,
	OWcISP_EF_MaxVolDist			= 105,
	OWcISP_EF_MinVolDist			= 106,
	OWcISP_EF_MaxVolAngle			= 107,
	OWcISP_EF_MinVolAngle			= 108,
	OWcISP_EF_MinAngleAttn			= 109,
	OWcISP_Btn_Play					= 110,
	OWcISP_Txt_AltImpulse			= 111,
	OWcISP_Btn_SetImpulse			= 112,
	OWcISP_Btn_EditImpulse			= 113,
	OWcISP_EF_Threshold				= 114,
	OWcISP_EF_ImpactVelocity		= 115,	
	OWcISP_EF_MinOcclusion			= 116,	
	OWcISP_Btn_Cancel				= WMcDialogItem_Cancel,
	OWcISP_Btn_Save					= WMcDialogItem_OK
};

// ------------------------------
// Select Sound Group
// ------------------------------
enum
{
	OWcSS_PM_Category				= 100,
	OWcSS_LB_Items					= 101,
	OWcSS_Btn_None					= 102,
	OWcSS_Btn_New					= 103,
	OWcSS_Btn_Cancel				= WMcDialogItem_Cancel,
	OWcSS_Btn_Select				= WMcDialogItem_OK
};

// ------------------------------
// Sound to Animation
// ------------------------------
enum
{
	OWcStA_LB_Data					= 100,
	OWcStA_Btn_Delete				= 101,
	OWcStA_Btn_Play					= 102,
	OWcStA_Btn_Edit					= 103,
	OWcStA_Btn_New					= 104
};

// ------------------------------
// Sound to Animation Properties
// ------------------------------
enum
{
	OWcStAP_PM_Variant				= 100,
	OWcStAP_PM_AnimType				= 101,
	OWcStAP_Txt_AnimName			= 102,
	OWcStAP_Txt_SoundName			= 103,
	OWcStAP_Btn_SetAnim				= 104,
	OWcStAP_Btn_SetSound			= 105,
	OWcStAP_EF_Frame				= 106,
	OWcStAP_PM_Modifier				= 107,
	OWcStAP_Txt_Duration			= 108,
	OWcStAP_Btn_Cancel				= WMcDialogItem_Cancel,
	OWcStAP_Btn_Save				= WMcDialogItem_OK
};

// ------------------------------
// View Animation
// ------------------------------
enum
{
	OWcVA_LB_Animations				= 101,
	OWcVA_Btn_Play					= 102,
	OWcVA_Btn_Close					= WMcDialogItem_Cancel
};

// ------------------------------
// Select Animation
// ------------------------------
enum
{
	OWcSA_PB_LoadProgress			= 100,
	OWcSA_LB_Animations				= 101,
	OWcSA_Btn_None					= 102,
	OWcSA_Btn_Cancel				= WMcDialogItem_Cancel,
	OWcSA_Btn_Select				= WMcDialogItem_OK
};

// ------------------------------
// Create Dialogue
// ------------------------------
enum
{
	OWcCD_PM_Category				= 100,
	OWcCD_LB_Items					= 101,
	OWcCD_Btn_New					= 103
};

// ------------------------------
// Create Impulse
// ------------------------------
enum
{
	OWcCI_PM_Category				= 100,
	OWcCI_LB_Items					= 101,
	OWcCI_Btn_New					= 103
};

// ------------------------------
// Create Group
// ------------------------------
enum
{
	OWcCG_PM_Category				= 100,
	OWcCG_LB_Items					= 101,
	OWcCG_Btn_New					= 103
};

// ======================================================================
// typedefs
// ======================================================================
typedef struct OWtSMData
{
	OWtSMType					type;
	BFtFileRef					*dir_ref;
	const char					*extension;
	
	UUtBool						can_paste;
	union
	{
		SStAmbient				ambient;
		SStGroup				group;
		SStImpulse				impulse;
	}u;
	
} OWtSMData;

typedef struct OWtAmbientProp
{
	BFtFileRef					*parent_dir_ref;
	SStAmbient					*ambient;
	SStPlayID					play_id;
	
	SStAmbient					ta;					// temp ambient
	
	UUtBool						can_save;
		
} OWtAmbientProp;

typedef struct OWtGroupProp
{
	BFtFileRef					*parent_dir_ref;
	SStGroup					*group;

	SStGroup					tg;					// temp group

	UUtBool						can_save;
	
} OWtGroupProp;

typedef struct OWtImpulseProp
{
	BFtFileRef					*parent_dir_ref;
	SStImpulse					*impulse;
	
	SStImpulse					ti;					// temp impulse
	
	UUtBool						can_save;
	
} OWtImpulseProp;

typedef struct OWtWS
{
	UUtBool						allow_categories;
	BFtFileRef					*base_dir_ref;
	SStSoundData				*selected_sound_data;
	BFtFileRef					*selected_category;
	
} OWtWS;

typedef struct OWtSelectData
{
	OWtSMType					type;
	BFtFileRef					*dir_ref;
	const char					*extension;
	
	union
	{
		SStAmbient				*ambient;
		SStGroup				*group;
		SStImpulse				*impulse;
	} u;
	
} OWtSelectData;

typedef struct OWtCreateDialogue
{
	BFtFileRef					*dir_ref;
	
} OWtCreateDialogue;

typedef struct OWtCreateImpulse
{
	BFtFileRef					*dir_ref;
	
} OWtCreateImpulse;

typedef struct OWtCreateGroup
{
	BFtFileRef					*dir_ref;
	
} OWtCreateGroup;

typedef struct OWtStAP
{
	OStAnimType					anim_type;
	OStModType					mod_type;
	SStImpulse					*impulse;
	TRtAnimation				*animation;
	UUtUns32					frame;
	char						anim_name[ONcMaxVariantNameLength];
	char						variant_name[ONcMaxVariantNameLength];
	char						impulse_name[SScMaxNameLength];
	
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
static BFtFileRef				*OWgWS_DirRef;
static BFtFileRef				*OWgCD_DirRef = NULL;
static BFtFileRef				*OWgCI_DirRef = NULL;
static BFtFileRef				*OWgCG_DirRef = NULL;
static BFtFileRef				*OWgSelectSound_DirRef;

static WMtDialog				*OWgViewAnimation;

// ======================================================================
// prototypes
// ======================================================================
static void
OWiEditGroup(
	WMtDialog					*inParentDialog,
	SStGroup					*inGroup);
	
static void
OWiEditImpulse(
	WMtDialog					*inParentDialog,
	SStImpulse					*inImpulse);
	
// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
void
OWrSound2_Terminate(
	void)
{
	if (OWgWS_DirRef)
	{
		BFrFileRef_Dispose(OWgWS_DirRef);
		OWgWS_DirRef = NULL;
	}

	if (OWgCD_DirRef)
	{
		BFrFileRef_Dispose(OWgCD_DirRef);
		OWgCD_DirRef = NULL;
	}

	if (OWgCI_DirRef)
	{
		BFrFileRef_Dispose(OWgCI_DirRef);
		OWgCI_DirRef = NULL;
	}

	if (OWgCG_DirRef)
	{
		BFrFileRef_Dispose(OWgCG_DirRef);
		OWgCG_DirRef = NULL;
	}

	if (OWgSelectSound_DirRef)
	{
		BFrFileRef_Dispose(OWgSelectSound_DirRef);
		OWgSelectSound_DirRef = NULL;
	}
}

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
			WMrMessage_Send(editfield, EFcMessage_SetMaxChars, (BFcMaxFileNameLength - 1), 0);
			
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
						BFcMaxFileNameLength);
					
					if (name[0] == '\0')
					{
						WMrDialog_MessageBox(
							inDialog,
							"Error",
							"The category name is invalid.",
							WMcMessageBoxStyle_OK);
					}
					else
					{
						// end the dialog
						WMrDialog_ModalEnd(inDialog, OWcCN_Btn_OK);
					}
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
	BFtFileRef					*file_ref;
	UUtUns32					item_data;

	file_ref = NULL;
	
	// get a pointer to the user data
	ws = (OWtWS*)WMrDialog_GetUserData(inDialog);
	UUmAssert(ws);
	
	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcWS_LB_DirContents);
	UUmAssert(listbox);
	
	ws->selected_sound_data = NULL;
	ws->selected_category = NULL;

	// get the data of the currently selected item
	item_data = WMrListBox_GetItemData(listbox, (UUtUns32)-1);
	if (item_data == LBcDirItemType_File) {	
		// get the selected file's name
		WMrMessage_Send(listbox, LBcMessage_GetText, (UUtUns32)selected_file_name, (UUtUns32)-1);
		UUrString_MakeLowerCase(selected_file_name, BFcMaxFileNameLength);
		
		// get the SStSoundData* corresponding to this file
		ws->selected_sound_data = SSrSoundData_GetByName(selected_file_name, UUcTrue);
		if (ws->selected_sound_data != NULL) { goto cleanup; }
			
		// create the file ref
		error =	BFrFileRef_DuplicateAndAppendName(OWgWS_DirRef, selected_file_name, &file_ref);
		if (error != UUcError_None) { goto cleanup; }
		
		// create a new sound data
		error = SSrSoundData_New(file_ref, &ws->selected_sound_data);
		if (error != UUcError_None) { goto cleanup; }

	} else if (item_data == LBcDirItemType_Directory) {
		UUmAssert(ws->allow_categories);

		// get the selected directory's name
		WMrMessage_Send(listbox, LBcMessage_GetText, (UUtUns32)selected_file_name, (UUtUns32)-1);
		UUrString_MakeLowerCase(selected_file_name, BFcMaxFileNameLength);
		
		// create the directory's file ref
		error = BFrFileRef_DuplicateAndAppendName(OWgWS_DirRef, selected_file_name, &ws->selected_category);
		if (error != UUcError_None) { goto cleanup; }

	} else {
		// the select button should never be enabled in this case
		UUmAssert(0);
	}

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
	
	// add .wav files and the folders to the listbox
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
	
	// add .aiff files to listbox
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
static UUtError
OWiWS_SetPopup(
	WMtWindow					*inPopup,
	BFtFileRef					*inBaseDirRef,
	BFtFileRef					*inDirRef)
{
	UUtError					error;
	BFtFileRef					*dir_ref;
	BFtFileRef					*parent;
	UUtUns16					i;
	
	error = BFrFileRef_Duplicate(inDirRef, &dir_ref);
	UUmError_ReturnOnError(error);
	
	WMrPopupMenu_Reset(inPopup);
	i = 0;
	
	while (1)
	{
		WMtMenuItemData				item_data;
		
		// add the current directory name to the popup
		item_data.flags = WMcMenuItemFlag_Enabled;
		item_data.id = i++;
		UUrString_Copy(item_data.title, BFrFileRef_GetLeafName(dir_ref), WMcMaxTitleLength);
		WMrPopupMenu_InsertItem(inPopup, &item_data, 0);
		
		// stop if the root binary sound directory was added
		if (BFrFileRef_IsEqual(inBaseDirRef, dir_ref) == UUcTrue) { break; }
		
		// get the parent directory 
		error = BFrFileRef_GetParentDirectory(dir_ref, &parent);
		if (error != UUcError_None) { break; }
		
		BFrFileRef_Dispose(dir_ref);
		dir_ref = parent;
		parent = NULL;
	}
	
	WMrPopupMenu_SetSelection(inPopup, 0);
	
	if (dir_ref)
	{
		BFrFileRef_Dispose(dir_ref);
		dir_ref = NULL;
	}
	
	return UUcError_None;
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
	OWtWS						*ws;
	
	// get a pointer to the user data
	ws = (OWtWS*)WMrDialog_GetUserData(inDialog);
	UUmAssert(ws);
	
	can_select = UUcFalse;
	
	// get the data of the currently selected item
	item_data = WMrListBox_GetItemData(inListBox, (UUtUns32)-1);
	if (item_data == LBcDirItemType_File) { can_select = UUcTrue; }
	if ((ws->allow_categories) && (item_data == LBcDirItemType_Directory)) { can_select = UUcTrue; }
	
	// enable or disable the select button depending on the type of
	// item currently selected
	button = WMrDialog_GetItemByID(inDialog, OWcWS_Btn_Select);
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
	item_data = WMrListBox_GetItemData(inListBox, (UUtUns32)(-1));
	if (item_data == LBcDirItemType_File)
	{
		// get the selected sound data file and end the dialog
		OWiWS_SelectSoundData(inDialog);
		WMrDialog_ModalEnd(inDialog, OWcWS_Btn_Select);
	}
	else if (item_data == LBcDirItemType_Directory)
	{
		BFtFileRef					*parent_dir_ref;
		BFtFileRef					*open_dir_ref;
		char						name[BFcMaxFileNameLength];
		
		parent_dir_ref = NULL;
		open_dir_ref = NULL;
		name[0] = '\0';
		
		// get the parent dir ref
		error = BFrFileRef_Duplicate(OWgWS_DirRef, &parent_dir_ref);
		if (error != UUcError_None) { goto cleanup; }
		
		// make a dir ref for the directory being opened
		WMrListBox_GetText(inListBox, name, (UUtUns32)(-1));
		error = BFrFileRef_DuplicateAndAppendName(parent_dir_ref, name, &open_dir_ref);
		if (error != UUcError_None) { goto cleanup; }
		
		// fill in the listbox
		OWiWS_FillListbox(inDialog, open_dir_ref);
		
		// dispose of the current directory ref
		if (OWgWS_DirRef != NULL)
		{
			BFrFileRef_Dispose(OWgWS_DirRef);
			OWgWS_DirRef = NULL;
		}
		
		// set the new directory ref
		error = BFrFileRef_Duplicate(open_dir_ref, &OWgWS_DirRef);
		if (error != UUcError_None) { goto cleanup; }
		
		// fill the popup menu
		error =
			OWiWS_SetPopup(
				WMrDialog_GetItemByID(inDialog, OWcWS_PM_Directory),
				ws->base_dir_ref,
				OWgWS_DirRef);
		
cleanup:
		if (parent_dir_ref != NULL)
		{
			BFrFileRef_Dispose(parent_dir_ref);
			parent_dir_ref = NULL;
		}
		
		if (open_dir_ref != NULL)
		{
			BFrFileRef_Dispose(open_dir_ref);
			open_dir_ref = NULL;
		}
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
	
	// get the user data
	ws = (OWtWS*)WMrDialog_GetUserData(inDialog);
	if (ws == NULL) { goto cleanup; }
	
	// init ws
	ws->base_dir_ref = NULL;
	ws->selected_sound_data = NULL;
	ws->selected_category = NULL;
	
	if (ws->allow_categories) {
		WMrWindow_SetTitle(inDialog, "Select Sound File or Directory", WMcMaxTitleLength);
	} else {
		WMrWindow_SetTitle(inDialog, "Select Sound File", WMcMaxTitleLength);
	}

	// get the sound directory ref
	error = SSrGetSoundDirectory(&ws->base_dir_ref);
	if (error != UUcError_None)
	{
		WMrDialog_MessageBox(
			NULL,
			"Error",
			"Unable to find the sound data folder, you cannot select sounds.",
			WMcMessageBoxStyle_OK);

		goto cleanup;
	}
	
	if (OWgWS_DirRef == NULL)
	{
		error = BFrFileRef_Duplicate(ws->base_dir_ref, &OWgWS_DirRef);
		if (error != UUcError_None) { goto cleanup; }
	}
	
	// fill the directory
	OWiWS_FillListbox(inDialog, OWgWS_DirRef);
	
	// fill the popup menu
	error =
		OWiWS_SetPopup(
			WMrDialog_GetItemByID(inDialog, OWcWS_PM_Directory),
			ws->base_dir_ref,
			OWgWS_DirRef);
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
	
	if (ws->base_dir_ref)
	{
		BFrFileRef_Dispose(ws->base_dir_ref);
		ws->base_dir_ref = NULL;
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
	UUtUns32					i;
	BFtFileRef					*dir_ref;
	UUtError					error;
	
	// if inItemID == 0 then the popup menu is still on the same item
	// so no updating needs to be done
	if (inItemID == 0) { return; }
	
	// get the dir ref of the current item in the popup menu
	error = BFrFileRef_Duplicate(OWgWS_DirRef, &dir_ref);
	if (error != UUcError_None) { return; }
	
	// go up the hierarchy of directories until the desired directory is found
	for (i = 0; i < (UUtUns32)inItemID; i++)
	{
		BFtFileRef					*parent_ref;
		
		error = BFrFileRef_GetParentDirectory(dir_ref, &parent_ref);
		if (error != UUcError_None) { return; }
		
		BFrFileRef_Dispose(dir_ref);
		dir_ref = parent_ref;
	}
	
	// fill in the listbox
	OWiWS_FillListbox(inDialog, dir_ref);
	
	BFrFileRef_Dispose(OWgWS_DirRef);
	BFrFileRef_Duplicate(dir_ref, &OWgWS_DirRef);
	
	BFrFileRef_Dispose(dir_ref);
	dir_ref = NULL;
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
OWiASP_EnableButtons(
	WMtDialog					*inDialog,
	OWtAmbientProp				*inAP)
{
	WMtWindow					*button;
	WMtWindow					*editfield;
	char						name[SScMaxNameLength];
	SStAmbient					*found_ambient;
	UUtBool						no_name_conflict;
	
	// determine if there is a name conflict
	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_Name);
	WMrEditField_GetText(editfield, name, SScMaxNameLength);
	
	found_ambient = OSrAmbient_GetByName(name);
	if (name[0] == '\0')
	{
		no_name_conflict = UUcFalse;
	}
	else if ((found_ambient == NULL) || (found_ambient == inAP->ambient))
	{
		no_name_conflict = UUcTrue;
	}
	else
	{
		no_name_conflict = UUcFalse;
	}
	
	button = WMrDialog_GetItemByID(inDialog, OWcASP_Btn_InGroupEdit);
	WMrWindow_SetEnabled(button, (inAP->ta.in_sound != NULL));

	button = WMrDialog_GetItemByID(inDialog, OWcASP_Btn_OutGroupEdit);
	WMrWindow_SetEnabled(button, (inAP->ta.out_sound != NULL));

	button = WMrDialog_GetItemByID(inDialog, OWcASP_Btn_BT1Edit);
	WMrWindow_SetEnabled(button, (inAP->ta.base_track1 != NULL));

	button = WMrDialog_GetItemByID(inDialog, OWcASP_Btn_BT2Edit);
	WMrWindow_SetEnabled(button, (inAP->ta.base_track2 != NULL));

	button = WMrDialog_GetItemByID(inDialog, OWcASP_Btn_DetailEdit);
	WMrWindow_SetEnabled(button, (inAP->ta.detail != NULL));
	
	button = WMrDialog_GetItemByID(inDialog, OWcASP_Btn_Start);
	WMrWindow_SetEnabled(button, (inAP->play_id == SScInvalidID));

	button = WMrDialog_GetItemByID(inDialog, OWcASP_Btn_Stop);
	WMrWindow_SetEnabled(button, (inAP->play_id != SScInvalidID));
	
	button = WMrDialog_GetItemByID(inDialog, OWcASP_Btn_Save);
	WMrWindow_SetEnabled(button, inAP->can_save && no_name_conflict);
}

// ----------------------------------------------------------------------
static void
OWiASP_RecordData(
	WMtDialog					*inDialog,
	OWtAmbientProp				*inAP)
{
	WMtWindow					*popup;
	WMtWindow					*checkbox;
	WMtWindow					*editfield;
	UUtUns16					priority;
	char						name[SScMaxNameLength];
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_Name);
	WMrEditField_GetText(editfield, name, SScMaxNameLength);
	UUrString_Copy(inAP->ta.ambient_name, name, SScMaxNameLength);
	
	popup = WMrDialog_GetItemByID(inDialog, OWcASP_PM_Priority);
	WMrPopupMenu_GetItemID(popup, -1, &priority);
	inAP->ta.priority = (SStPriority2)priority;
	
	checkbox = WMrDialog_GetItemByID(inDialog, OWcASP_CB_InterruptOnStop);
	if (WMrCheckBox_GetCheck(checkbox) == UUcTrue)
	{
		inAP->ta.flags |= SScAmbientFlag_InterruptOnStop;
	}
	else
	{
		inAP->ta.flags &= ~SScAmbientFlag_InterruptOnStop;
	}
	
	checkbox = WMrDialog_GetItemByID(inDialog, OWcASP_CB_PlayOnce);
	if (WMrCheckBox_GetCheck(checkbox) == UUcTrue)
	{
		inAP->ta.flags |= SScAmbientFlag_PlayOnce;
	}
	else
	{
		inAP->ta.flags &= ~SScAmbientFlag_PlayOnce;
	}
	
	checkbox = WMrDialog_GetItemByID(inDialog, OWcASP_CB_CanPan);
	if (WMrCheckBox_GetCheck(checkbox) == UUcTrue)
	{
		inAP->ta.flags |= SScAmbientFlag_Pan;
	}
	else
	{
		inAP->ta.flags &= ~SScAmbientFlag_Pan;
	}
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_SphereRadius);
	inAP->ta.sphere_radius = WMrEditField_GetFloat(editfield);

	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_MinElapsedTime);
	inAP->ta.min_detail_time = WMrEditField_GetFloat(editfield);

	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_MaxElapsedTime);
	inAP->ta.max_detail_time = WMrEditField_GetFloat(editfield);
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_MaxVolDist);
	inAP->ta.max_volume_distance = WMrEditField_GetFloat(editfield);
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_MinVolDist);
	inAP->ta.min_volume_distance = WMrEditField_GetFloat(editfield);
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_Threshold);
	inAP->ta.threshold = (UUtUns32)WMrEditField_GetInt32(editfield);

	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_MinOcclusion);
	inAP->ta.min_occlusion = WMrEditField_GetFloat(editfield);

	if (inAP->ta.detail)
	{
		UUrString_Copy(
			inAP->ta.detail_name,
			inAP->ta.detail->group_name,
			SScMaxNameLength);
	}
	else
	{
		UUrMemory_Clear(inAP->ta.detail_name, SScMaxNameLength);
	}

	if (inAP->ta.base_track1)
	{
		UUrString_Copy(
			inAP->ta.base_track1_name,
			inAP->ta.base_track1->group_name,
			SScMaxNameLength);
	}
	else
	{
		UUrMemory_Clear(inAP->ta.base_track1_name, SScMaxNameLength);
	}

	if (inAP->ta.base_track2)
	{
		UUrString_Copy(
			inAP->ta.base_track2_name,
			inAP->ta.base_track2->group_name,
			SScMaxNameLength);
	}
	else
	{
		UUrMemory_Clear(inAP->ta.base_track2_name, SScMaxNameLength);
	}

	if (inAP->ta.in_sound)
	{
		UUrString_Copy(
			inAP->ta.in_sound_name,
			inAP->ta.in_sound->group_name,
			SScMaxNameLength);
	}
	else
	{
		UUrMemory_Clear(inAP->ta.in_sound_name, SScMaxNameLength);
	}

	if (inAP->ta.out_sound)
	{
		UUrString_Copy(
			inAP->ta.out_sound_name,
			inAP->ta.out_sound->group_name,
			SScMaxNameLength);
	}
	else
	{
		UUrMemory_Clear(inAP->ta.out_sound_name, SScMaxNameLength);
	}
}

// ----------------------------------------------------------------------
static UUtBool
OWiASP_Save(
	WMtDialog					*inDialog,
	OWtAmbientProp				*inAP)
{
	UUtError					error;
	WMtWindow					*editfield;
	char						name[BFcMaxFileNameLength];
	SStAmbient					*found_ambient;
	
	// make sure the sound has a name
	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_Name);
	WMrEditField_GetText(editfield, name, BFcMaxFileNameLength);
	
	if (strlen(name) == 0)
	{
		WMrDialog_MessageBox(
			inDialog,
			"Error",
			"The ambient sound must have a name before you can save it.",
			WMcMessageBoxStyle_OK);
		return UUcFalse;
	}
	
	OSrMakeGoodName(name, name);
	
	if (inAP->ambient == NULL)
	{
		found_ambient = OSrAmbient_GetByName(name);
		if (found_ambient != NULL)
		{
			WMrDialog_MessageBox(
				inDialog,
				"Error",
				"An ambient sound with that name already exists.  Please change the name.",
				WMcMessageBoxStyle_OK);
			return UUcFalse;
		}
		
		error = OSrAmbient_New(name, &inAP->ambient);
		if (error != UUcError_None)
		{
			WMrDialog_MessageBox(
				inDialog,
				"Error",
				"Unable to create the ambient sound.",
				WMcMessageBoxStyle_OK);
			return UUcFalse;
		}
	}
	else
	{
		// create a new ambient sound if the name was changed
		found_ambient = OSrAmbient_GetByName(name);
		if (found_ambient == NULL)
		{
			error = OSrAmbient_New(name, &inAP->ambient);
			if (error != UUcError_None)
			{
				WMrDialog_MessageBox(
					inDialog,
					"Error",
					"Unable to create the ambient sound.",
					WMcMessageBoxStyle_OK);
				return UUcFalse;
			}
		}
	}
	
	// get the data
	OWiASP_RecordData(inDialog, inAP);	
	
	if (inAP->ta.min_volume_distance < inAP->ta.max_volume_distance)
	{
		WMrDialog_MessageBox(
			inDialog,
			"Error",
			"The maximum volume distance must be smaller than the minimum volume distance.",
			WMcMessageBoxStyle_OK);
		return UUcFalse;
	}
	
	// set the data for the ambient sound
	inAP->ambient->priority = inAP->ta.priority;
	inAP->ambient->flags = inAP->ta.flags;
	inAP->ambient->sphere_radius = inAP->ta.sphere_radius;
	inAP->ambient->min_detail_time = inAP->ta.min_detail_time;
	inAP->ambient->max_detail_time = inAP->ta.max_detail_time;
	inAP->ambient->max_volume_distance = inAP->ta.max_volume_distance;
	inAP->ambient->min_volume_distance = inAP->ta.min_volume_distance;
	inAP->ambient->threshold = inAP->ta.threshold;
	inAP->ambient->min_occlusion = inAP->ta.min_occlusion;
	inAP->ambient->detail = inAP->ta.detail;
	inAP->ambient->base_track1 = inAP->ta.base_track1;
	inAP->ambient->base_track2 = inAP->ta.base_track2;
	inAP->ambient->in_sound = inAP->ta.in_sound;
	inAP->ambient->out_sound = inAP->ta.out_sound;
	UUrString_Copy(inAP->ambient->detail_name, inAP->ta.detail_name, SScMaxNameLength);
	UUrString_Copy(inAP->ambient->base_track1_name, inAP->ta.base_track1_name, SScMaxNameLength);
	UUrString_Copy(inAP->ambient->base_track2_name, inAP->ta.base_track2_name, SScMaxNameLength);
	UUrString_Copy(inAP->ambient->in_sound_name, inAP->ta.in_sound_name, SScMaxNameLength);
	UUrString_Copy(inAP->ambient->out_sound_name, inAP->ta.out_sound_name, SScMaxNameLength);
	
	// save the ambient sound to disk
	OSrAmbient_Save(inAP->ambient, inAP->parent_dir_ref);
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
static UUtBool
OWiASP_CheckGroups(
	WMtDialog					*inDialog,
	SStAmbient					*inAmbient)
{	
	if (inAmbient->base_track1 != NULL)
	{
		if ((inAmbient->in_sound != NULL) &&
			(inAmbient->base_track1->num_channels != inAmbient->in_sound->num_channels))
		{
			WMrDialog_MessageBox(
				inDialog,
				"Error",
				"The in sound and base track 1 must use the same number of sound channels.",
				WMcMessageBoxStyle_OK);
				
			return UUcFalse;
		}
		
		if ((inAmbient->out_sound != NULL) &&
			(inAmbient->base_track1->num_channels != inAmbient->out_sound->num_channels))
		{
			WMrDialog_MessageBox(
				inDialog,
				"Error",
				"The out sound and base track 1 must use the same number of sound channels.",
				WMcMessageBoxStyle_OK);
				
			return UUcFalse;
		}
		
		if ((inAmbient->in_sound != NULL) && (inAmbient->out_sound != NULL))
		{
			if (inAmbient->in_sound->num_channels != inAmbient->out_sound->num_channels)
			{
				WMrDialog_MessageBox(
					inDialog,
					"Error",
					"The in sound and the out sound must use the same number of sound channels.",
					WMcMessageBoxStyle_OK);
				
				return UUcFalse;
			}
		}
	}

	return UUcTrue;
}

// ----------------------------------------------------------------------
static void
OWiASP_SetFields(
	WMtDialog					*inDialog,
	OWtAmbientProp				*inAP)
{
	WMtWindow					*editfield;
	WMtWindow					*text;
	WMtWindow					*popup;
	WMtWindow					*checkbox;
	
	if (inAP->ambient) 
	{
		editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_Name);
		WMrEditField_SetText(editfield, inAP->ambient->ambient_name);
	}
		
	text = WMrDialog_GetItemByID(inDialog, OWcASP_Txt_InGroup);
	WMrWindow_SetTitle(text, inAP->ta.in_sound_name, SScMaxNameLength);
	if (inAP->ta.in_sound) { WMrText_SetShade(text, IMcShade_Black); }
	else { WMrText_SetShade(text, IMcShade_Red); }
	
	text = WMrDialog_GetItemByID(inDialog, OWcASP_Txt_OutGroup);
	WMrWindow_SetTitle(text, inAP->ta.out_sound_name, SScMaxNameLength);
	if (inAP->ta.out_sound) { WMrText_SetShade(text, IMcShade_Black); }
	else { WMrText_SetShade(text, IMcShade_Red); }
	
	text = WMrDialog_GetItemByID(inDialog, OWcASP_Txt_BT1);
	WMrWindow_SetTitle(text, inAP->ta.base_track1_name, SScMaxNameLength);
	if (inAP->ta.base_track1) { WMrText_SetShade(text, IMcShade_Black); }
	else { WMrText_SetShade(text, IMcShade_Red); }

	text = WMrDialog_GetItemByID(inDialog, OWcASP_Txt_BT2);
	WMrWindow_SetTitle(text, inAP->ta.base_track2_name, SScMaxNameLength);
	if (inAP->ta.base_track2) { WMrText_SetShade(text, IMcShade_Black); }
	else { WMrText_SetShade(text, IMcShade_Red); }

	text = WMrDialog_GetItemByID(inDialog, OWcASP_Txt_Detail);
	WMrWindow_SetTitle(text, inAP->ta.detail_name, SScMaxNameLength);
	if (inAP->ta.detail) { WMrText_SetShade(text, IMcShade_Black); }
	else { WMrText_SetShade(text, IMcShade_Red); }
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_MinElapsedTime);
	WMrEditField_SetFloat(editfield, inAP->ta.min_detail_time);
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_MaxElapsedTime);
	WMrEditField_SetFloat(editfield, inAP->ta.max_detail_time);
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_SphereRadius);
	WMrEditField_SetFloat(editfield, inAP->ta.sphere_radius);
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_MinVolDist);
	WMrEditField_SetFloat(editfield, inAP->ta.min_volume_distance);
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_MaxVolDist);
	WMrEditField_SetFloat(editfield, inAP->ta.max_volume_distance);
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_Threshold);
	WMrEditField_SetInt32(editfield, inAP->ta.threshold);

	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_MinOcclusion);
	WMrEditField_SetFloat(editfield, inAP->ta.min_occlusion);
	
	popup = WMrDialog_GetItemByID(inDialog, OWcASP_PM_Priority);
	WMrPopupMenu_SetSelection(popup, inAP->ta.priority);
	
	checkbox = WMrDialog_GetItemByID(inDialog, OWcASP_CB_InterruptOnStop);
	WMrCheckBox_SetCheck(checkbox, ((inAP->ta.flags & SScAmbientFlag_InterruptOnStop) != 0));

	checkbox = WMrDialog_GetItemByID(inDialog, OWcASP_CB_PlayOnce);
	WMrCheckBox_SetCheck(checkbox, ((inAP->ta.flags & SScAmbientFlag_PlayOnce) != 0));

	checkbox = WMrDialog_GetItemByID(inDialog, OWcASP_CB_CanPan);
	WMrCheckBox_SetCheck(checkbox, ((inAP->ta.flags & SScAmbientFlag_Pan) != 0));
}

// ----------------------------------------------------------------------
static void
OWiASP_InitDialog(
	WMtDialog					*inDialog)
{
	UUtError					error;
	OWtAmbientProp				*ap;
	WMtWindow					*editfield;
	
	// get the ambient properties
	ap = (OWtAmbientProp*)WMrDialog_GetUserData(inDialog);
	if (ap == NULL)
	{
		WMrDialog_ModalEnd(inDialog, OWcASP_Btn_Cancel);
		return;
	}
	
	// initialize the ambient properties
	ap->play_id = SScInvalidID;
	UUrMemory_Clear(&ap->ta, sizeof(SStAmbient));
	ap->can_save = UUcFalse;

	if (ap->ambient == NULL)
	{
		ap->ta.priority = SScPriority2_Normal;
		ap->ta.flags = SScAmbientFlag_None;
		
		ap->ta.sphere_radius = 10.0f;
		ap->ta.min_detail_time = 1.0f;
		ap->ta.max_detail_time = 1.0f;
		
		ap->ta.max_volume_distance = 10.0f;
		ap->ta.min_volume_distance = 50.0f;
		
		ap->ta.threshold = SScAmbientThreshold;
		ap->ta.min_occlusion = 0.0f;
		
		ap->ta.base_track1 = NULL;
		ap->ta.base_track2 = NULL;
		ap->ta.detail = NULL;
		ap->ta.in_sound = NULL;
		ap->ta.out_sound = NULL;
		
		if ((ap->parent_dir_ref != NULL) && 
			(BFrFileRef_FileExists(ap->parent_dir_ref) == UUcTrue))
		{
			ap->can_save = UUcTrue;
		}
	}
	else
	{
		ap->ta = *ap->ambient;
		
		// does the ambient sound have a file ref
		if (ap->parent_dir_ref != NULL)
		{
			BFtFileRef				*dir_ref;
			char					name[SScMaxNameLength];
			
			sprintf(name, "%s.%s", ap->ambient->ambient_name, OScAmbientSuffix);
			
			error =
				BFrFileRef_DuplicateAndAppendName(
					ap->parent_dir_ref,
					name,
					&dir_ref);
			if (error == UUcError_None)
			{
				if (BFrFileRef_FileExists(dir_ref) && (BFrFileRef_IsLocked(dir_ref) == UUcFalse))
				{
					ap->can_save = UUcTrue;
				}
				
				UUrMemory_Block_Delete(dir_ref);
				dir_ref = NULL;
			}
		}
	}
	
	// set the maximum number of characters for the ambient name
	editfield = WMrDialog_GetItemByID(inDialog, OWcASP_EF_Name);
	WMrMessage_Send(
		editfield,
		EFcMessage_SetMaxChars,
		BFcMaxFileNameLength - strlen(".abc"),
		0);
	
	// set the fields
	OWiASP_SetFields(inDialog, ap);
	
	// enable the buttons
	OWiASP_EnableButtons(inDialog, ap);
	
	// set the focus to the name
	WMrWindow_SetFocus(editfield);
}

// ----------------------------------------------------------------------
static void
OWiASP_HandleCommand(
	WMtDialog					*inDialog,
	WMtWindow					*inControl,
	UUtUns32					inParam)
{
	UUtUns16					control_id;
	UUtUns16					command_type;
	OWtAmbientProp				*ap;
	UUtBool						update_fields;
	SStGroup					*group;
	OWtSelectResult				result;
	
	ap = (OWtAmbientProp*)WMrDialog_GetUserData(inDialog);
	
	update_fields = UUcTrue;
	
	control_id = UUmLowWord(inParam);
	command_type = UUmHighWord(inParam);
	
	switch (control_id)
	{
		case OWcASP_EF_Name:
			if (command_type != EFcNotify_ContentChanged) { break; }
			
			// if the ambient sound was locked, then the can_save bool
			// is null.  If the name changes, then the user wants to make
			// a new ambient sound so allow this by setting the ambient to
			// null and setting can_save to UUcTrue
			if (ap->can_save == UUcFalse) 
			{
				ap->ambient = NULL;
				ap->can_save = UUcTrue;
			}
			update_fields = UUcFalse;
		break;
		
		case OWcASP_Btn_InGroupSet:
			if (command_type != WMcNotify_Click) { break; }
			result = OWrSelect_SoundGroup(&group);
			if (result == OWcSelectResult_Cancel) { break; }
			ap->ta.in_sound = group;
			if (OWiASP_CheckGroups(inDialog, &ap->ta) == UUcFalse)
			{
				ap->ta.in_sound = NULL;
			}
			
			if (ap->ta.in_sound != NULL)
			{
				UUrString_Copy(
					ap->ta.in_sound_name,
					ap->ta.in_sound->group_name,
					SScMaxNameLength);
			}
			else
			{
				UUrMemory_Clear(ap->ta.in_sound_name, SScMaxNameLength);
			}
		break;
		
		case OWcASP_Btn_InGroupEdit:
			if (command_type != WMcNotify_Click) { break; }
			OWiEditGroup(inDialog, ap->ta.in_sound);
		break;
		
		case OWcASP_Btn_OutGroupSet:
			if (command_type != WMcNotify_Click) { break; }
			result = OWrSelect_SoundGroup(&group);
			if (result == OWcSelectResult_Cancel) { break; }
			ap->ta.out_sound = group;
			if (OWiASP_CheckGroups(inDialog, &ap->ta) == UUcFalse)
			{
				ap->ta.out_sound = NULL;
			}
			
			if (ap->ta.out_sound != NULL)
			{
				UUrString_Copy(
					ap->ta.out_sound_name,
					ap->ta.out_sound->group_name,
					SScMaxNameLength);
			}
			else
			{
				UUrMemory_Clear(ap->ta.out_sound_name, SScMaxNameLength);
			}
		break;
		
		case OWcASP_Btn_OutGroupEdit:
			if (command_type != WMcNotify_Click) { break; }
			OWiEditGroup(inDialog, ap->ta.out_sound);
		break;
		
		case OWcASP_Btn_BT1Set:
			if (command_type != WMcNotify_Click) { break; }
			result = OWrSelect_SoundGroup(&group);
			if (result == OWcSelectResult_Cancel) { break; }
			ap->ta.base_track1 = group;
			if (OWiASP_CheckGroups(inDialog, &ap->ta) == UUcFalse)
			{
				ap->ta.base_track1 = NULL;
			}
			
			if (ap->ta.base_track1 != NULL)
			{
				UUrString_Copy(
					ap->ta.base_track1_name,
					ap->ta.base_track1->group_name,
					SScMaxNameLength);
			}
			else
			{
				UUrMemory_Clear(ap->ta.base_track1_name, SScMaxNameLength);
			}
		break;
		
		case OWcASP_Btn_BT1Edit:
			if (command_type != WMcNotify_Click) { break; }
			OWiEditGroup(inDialog, ap->ta.base_track1);
		break;
		
		case OWcASP_Btn_BT2Set:
			if (command_type != WMcNotify_Click) { break; }
			result = OWrSelect_SoundGroup(&group);
			if (result == OWcSelectResult_Cancel) { break; }
			ap->ta.base_track2 = group;

			if (ap->ta.base_track2 != NULL)
			{
				UUrString_Copy(
					ap->ta.base_track2_name,
					ap->ta.base_track2->group_name,
					SScMaxNameLength);
			}
			else
			{
				UUrMemory_Clear(ap->ta.base_track2_name, SScMaxNameLength);
			}
		break;
		
		case OWcASP_Btn_BT2Edit:
			if (command_type != WMcNotify_Click) { break; }
			OWiEditGroup(inDialog, ap->ta.base_track2);
		break;
		
		case OWcASP_Btn_DetailSet:
			if (command_type != WMcNotify_Click) { break; }
			result = OWrSelect_SoundGroup(&group);
			if (result == OWcSelectResult_Cancel) { break; }
			ap->ta.detail = group;

			if (ap->ta.detail != NULL)
			{
				UUrString_Copy(
					ap->ta.detail_name,
					ap->ta.detail->group_name,
					SScMaxNameLength);
			}
			else
			{
				UUrMemory_Clear(ap->ta.detail_name, SScMaxNameLength);
			}
		break;
		
		case OWcASP_Btn_DetailEdit:
			if (command_type != WMcNotify_Click) { break; }
			OWiEditGroup(inDialog, ap->ta.detail);
		break;
		
		case OWcASP_Btn_Inc1:
			if (command_type != WMcNotify_Click) { break; }
			ap->ta.sphere_radius += 1.0f;
			if (ap->ta.sphere_radius > ap->ta.min_volume_distance)
			{
				ap->ta.sphere_radius = ap->ta.min_volume_distance;
			}
		break;
		
		case OWcASP_Btn_Dec1:
			if (command_type != WMcNotify_Click) { break; }
			ap->ta.sphere_radius -= 1.0f;
			if (ap->ta.sphere_radius < 0.0f)
			{
				ap->ta.sphere_radius = 0.0f;
			}
		break;
		
		case OWcASP_Btn_Inc10:
			if (command_type != WMcNotify_Click) { break; }
			ap->ta.sphere_radius += 10.0f;
			if (ap->ta.sphere_radius > ap->ta.min_volume_distance)
			{
				ap->ta.sphere_radius = ap->ta.min_volume_distance;
			}
		break;
		
		case OWcASP_Btn_Dec10:
			if (command_type != WMcNotify_Click) { break; }
			ap->ta.sphere_radius -= 10.0f;
			if (ap->ta.sphere_radius < 0.0f)
			{
				ap->ta.sphere_radius = 0.0f;
			}
		break;
		
		case OWcASP_Btn_Start:
			if (command_type != WMcNotify_Click) { break; }
			if ((ap->play_id == SScInvalidID) ||
				(SSrAmbient_Update(ap->play_id, NULL, NULL, NULL, NULL) == UUcFalse))
			{
				OWiASP_RecordData(inDialog, ap);
				ap->play_id = SSrAmbient_Start_Simple(&ap->ta, NULL);
			}
		break;
		
		case OWcASP_Btn_Stop:
			if (command_type != WMcNotify_Click) { break; }
			if (ap->play_id != SScInvalidID)
			{
				OWiASP_RecordData(inDialog, ap);
				SSrAmbient_Stop(ap->play_id);
			}
		break;
		
		case OWcASP_Btn_Cancel:
			if (command_type != WMcNotify_Click) { break; }
			WMrDialog_ModalEnd(inDialog, OWcASP_Btn_Cancel);
		break;
		
		case OWcASP_Btn_Save:
			if (command_type != WMcNotify_Click) { break; }
			if (OWiASP_Save(inDialog, ap) == UUcTrue)
			{
				WMrDialog_ModalEnd(inDialog, OWcASP_Btn_Save);
			}
		break;
		
		default:
			update_fields = UUcFalse;
		break;
	}
	
	if (update_fields)
	{
		OWiASP_SetFields(inDialog, ap);
	}
	
	// update the buttons
	OWiASP_EnableButtons(inDialog, ap);
}
// ----------------------------------------------------------------------
static void
OWiASP_HandleDestroy(
	WMtDialog					*inDialog)
{
	OWtAmbientProp				*ap;
	
	ap = (OWtAmbientProp*)WMrDialog_GetUserData(inDialog);
	if ((ap->play_id != SScInvalidID) &&
		(SSrAmbient_Update(ap->play_id, NULL, NULL, NULL, NULL) == UUcTrue))
	{
		SSrAmbient_Halt(ap->play_id);
		ap->play_id = SScInvalidID;
	}
}

// ----------------------------------------------------------------------
static void
OWiASP_Paint(
	WMtDialog					*inDialog)
{
	OWtAmbientProp				*ap;
	
	ap = (OWtAmbientProp*)WMrDialog_GetUserData(inDialog);
	if ((ap->play_id != SScInvalidID) &&
		(SSrAmbient_Update(ap->play_id, NULL, NULL, NULL, NULL) == UUcFalse))
	{
		ap->play_id = SScInvalidID;
		OWiASP_EnableButtons(inDialog, ap);
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
			OWiASP_HandleDestroy(inDialog);
		break;
		
		case WMcMessage_Command:
			OWiASP_HandleCommand(inDialog, (WMtWindow*)inParam2, inParam1);
		break;
		
		case WMcMessage_Paint:
			OWiASP_Paint(inDialog);
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ----------------------------------------------------------------------
UUtError
OWrAmbientProperties_Display(
	WMtDialog					*inParentDialog,
	SStAmbient					*inAmbient)
{
	UUtError					error;
	OWtAmbientProp				ap;
	BFtFileRef					*file_ref;
	BFtFileRef					*dir_ref;
	
	UUmAssert(inAmbient);
	
	dir_ref = NULL;
	file_ref = NULL;
	
	// get the sound binary directory
	error = OSrGetSoundBinaryDirectory(&dir_ref);
	if (error != UUcError_None) { goto error; }
	
	// get the file ref of the sound
	error =
		OSrGetSoundFileRef(
			inAmbient->ambient_name,
			OScAmbientSuffix,
			dir_ref,
			&file_ref);
	if (error != UUcError_None) { goto error; }
	
	// dispose of the sound binary directory ref
	BFrFileRef_Dispose(dir_ref);
	dir_ref = NULL;
	
	// get the parent directory of the sound
	error = BFrFileRef_GetParentDirectory(file_ref, &dir_ref);
	if (error != UUcError_None) { goto error; }
	
	// dispose of the file ref of the sound
	BFrFileRef_Dispose(file_ref);
	file_ref = NULL;
	
	// set up the properties
	UUrMemory_Clear(&ap, sizeof(OWtAmbientProp));
	ap.parent_dir_ref = dir_ref;
	ap.ambient = inAmbient;
		
	// edit the sound
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Ambient_Sound_Prop,
			inParentDialog,
			OWiASP_Callback,
			(UUtUns32)&ap,
			NULL);
	if (error != UUcError_None) { goto error; }
	
	// dispose of the parent directory of the sound
	BFrFileRef_Dispose(dir_ref);
	dir_ref = NULL;
	
	return UUcError_None;
	
error:
	if (dir_ref)
	{
		BFrFileRef_Dispose(dir_ref);
		dir_ref = NULL;
	}
	
	if (file_ref)
	{
		BFrFileRef_Dispose(file_ref);
		file_ref = NULL;
	}
	
	return error;
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
	OWtGroupProp				*inGP)
{
	WMtWindow					*listbox;
	WMtWindow					*editfield;
	WMtWindow					*button;
	UUtBool						permutation_selected;
	UUtBool						no_name_conflict;
	SStGroup					*found_group;
	char						name[BFcMaxFileNameLength];
	
	// get a pointer to the listbox
	permutation_selected = UUcFalse;
	if ((inGP->tg.permutations != NULL) &&
		(UUrMemory_Array_GetUsedElems(inGP->tg.permutations) > 0))
	{
		listbox = WMrDialog_GetItemByID(inDialog, OWcSGP_LB_Permutations);
		permutation_selected = (WMrListBox_GetSelection(listbox) != LBcError);
	}
	
	// determine if there is a name conflict
	editfield = WMrDialog_GetItemByID(inDialog, OWcSGP_EF_Name);
	WMrEditField_GetText(editfield, name, BFcMaxFileNameLength);
	
	found_group = OSrGroup_GetByName(name);
	if (name[0] == '\0')
	{
		no_name_conflict = UUcFalse;
	}
	else if ((found_group == NULL) || (found_group == inGP->group))
	{
		no_name_conflict = UUcTrue;
	}
	else
	{
		no_name_conflict = UUcFalse;
	}
	
	// update the buttons
	button = WMrDialog_GetItemByID(inDialog, OWcSGP_Btn_AddPerm);
	WMrWindow_SetEnabled(button, UUcTrue);
	
	button = WMrDialog_GetItemByID(inDialog, OWcSGP_Btn_EditPerm);
	WMrWindow_SetEnabled(button, permutation_selected);
	
	button = WMrDialog_GetItemByID(inDialog, OWcSGP_Btn_PlayPerm);
	WMrWindow_SetEnabled(button, permutation_selected);
	
	button = WMrDialog_GetItemByID(inDialog, OWcSGP_Btn_DeletePerm);
	WMrWindow_SetEnabled(button, permutation_selected);
	
	button = WMrDialog_GetItemByID(inDialog, OWcSGP_Btn_Save);
	WMrWindow_SetEnabled(button, inGP->can_save && no_name_conflict);
}

// ----------------------------------------------------------------------
static void
OWiSGP_FillListbox(
	WMtDialog					*inDialog)
{
	OWtGroupProp				*gp;
	WMtDialog					*listbox;
	UUtUns32					i;
	UUtUns32					num_permutations;
	
	// get the group properties
	gp = (OWtGroupProp*)WMrDialog_GetUserData(inDialog);

	// get the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcSGP_LB_Permutations);
	
	// reset the listbox
	WMrListBox_Reset(listbox);
	
	// add the permutations
	num_permutations = UUrMemory_Array_GetUsedElems(gp->tg.permutations);
	for (i = 0; i < num_permutations; i++)
	{
		WMrMessage_Send(listbox, LBcMessage_AddString, i, 0);
	}
	
	WMrListBox_SetSelection(listbox, UUcFalse, 0);
}

// ----------------------------------------------------------------------
static UUtError
OWiSGP_Perm_Add(
	WMtDialog					*inDialog,
	OWtGroupProp				*inGP)
{
	UUtError					error;
	UUtUns32					message;
	OWtWS						ws;
	UUtUns32					index;
	SStPermutation				*perm_array;
	SStPermutation				*perm;

	UUrMemory_Clear(&ws, sizeof(ws));
	ws.allow_categories = UUcFalse;

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
	
	if (inGP->tg.num_channels == 0)
	{
		inGP->tg.num_channels = SSrSound_GetNumChannels(ws.selected_sound_data);
	}
	else if (SSrSound_GetNumChannels(ws.selected_sound_data) != inGP->tg.num_channels)
	{
		WMrDialog_MessageBox(
			inDialog,
			"Error",
			"The selected sound does not have the same number of channels as the other sounds.",
			WMcMessageBoxStyle_OK);
			
		return UUcError_None;
	}
		
	// add the permutation
	error = UUrMemory_Array_GetNewElement(inGP->tg.permutations, &index, NULL);
	if (error != UUcError_None) { goto error; }
	
	perm_array = (SStPermutation*)UUrMemory_Array_GetMemory(inGP->tg.permutations);
	UUmAssert(perm_array);
	
	perm = &perm_array[index];

	perm->weight					= 10;
	perm->min_volume_percent		= 1.0;
	perm->max_volume_percent		= 1.0;
	perm->min_pitch_percent			= 1.0;
	perm->max_pitch_percent			= 1.0;
	perm->sound_data				= ws.selected_sound_data;
	UUrString_Copy(
		perm->sound_data_name,
		SSrSoundData_GetName(ws.selected_sound_data),
		SScMaxNameLength);
	
	// edit the permutation
/*	
NOTE: removed per Marty's request

	error =
		WMrDialog_ModalBegin(
			OWcDialog_Sound_Perm_Prop,
			inDialog,
			OWiSPP_Callback,
			(UUtUns32)perm,
			&message);
	UUmError_ReturnOnError(error);
	
	if (message == OWcSPP_Btn_Save)*/
	{
		WMtWindow				*listbox;
		
		// update the dialog's fields
		OWiSGP_FillListbox(inDialog);
		
		// get a pointer to the listbox
		listbox = WMrDialog_GetItemByID(inDialog, OWcSGP_LB_Permutations);
		UUmAssert(listbox);
		
		// select the permutation that was just added
		WMrMessage_Send(listbox, LBcMessage_SetSelection, (UUtUns32)UUcTrue, index);
	}

	return UUcError_None;
	
error:
	// report the error
	WMrDialog_MessageBox(
		inDialog,
		"Error",
		"Unable to add the permutation.",
		WMcMessageBoxStyle_OK);

	return UUcError_Generic;
}

// ----------------------------------------------------------------------
static void
OWiSGP_Perm_Delete(
	WMtDialog					*inDialog,
	OWtGroupProp				*inGP)
{
	WMtWindow					*listbox;
	UUtUns32					perm_index;
	UUtUns32					message;

	// ask the user to confirm delete
	message =
		WMrDialog_MessageBox(
			inDialog,
			"Confirm Permutation Deletion",
			"Are you sure you want to delete the selected permutation?",
			WMcMessageBoxStyle_Yes_No);
	if (message == WMcDialogItem_No) { return; }

	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcSGP_LB_Permutations);
	UUmAssert(listbox);
	
	// get the selected permutation's index
	perm_index = WMrListBox_GetSelection(listbox);
	UUmAssert(perm_index < UUrMemory_Array_GetUsedElems(inGP->tg.permutations));
	
	// delete the selected permutation
	UUrMemory_Array_DeleteElement(inGP->tg.permutations, perm_index);
	
	// update the listbox
	OWiSGP_FillListbox(inDialog);
}

// ----------------------------------------------------------------------
static void
OWiSGP_Perm_Edit(
	WMtDialog					*inDialog,
	OWtGroupProp				*inGP)
{
	WMtWindow					*listbox;
	UUtUns32					perm_index;
	UUtUns32					message;
	UUtError					error;
	SStPermutation				*perm;
	SStPermutation				*perm_array;
	
	if (UUrMemory_Array_GetUsedElems(inGP->tg.permutations) == 0) { return; }
	
	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcSGP_LB_Permutations);
	UUmAssert(listbox);
	
	// get the selected permutation's index
	perm_index = WMrListBox_GetSelection(listbox);
	UUmAssert(perm_index < UUrMemory_Array_GetUsedElems(inGP->tg.permutations));
	
	perm_array = (SStPermutation*)UUrMemory_Array_GetMemory(inGP->tg.permutations);
	UUmAssert(perm_array);
	
	perm = &perm_array[perm_index];
	
	// edit the permutation
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Sound_Perm_Prop,
			inDialog,
			OWiSPP_Callback,
			(UUtUns32)perm,
			&message);
	if (error != UUcError_None) { return; }
}

// ----------------------------------------------------------------------
static void
OWiSGP_Perm_Play(
	WMtDialog					*inDialog,
	OWtGroupProp				*inGP)
{
	WMtWindow					*listbox;
	UUtUns32					perm_index;
	SStPermutation				*perm;
	SStPermutation				*perm_array;

	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcSGP_LB_Permutations);
	UUmAssert(listbox);
	
	// get the selected permutation's index
	perm_index = WMrListBox_GetSelection(listbox);
	UUmAssert(perm_index < UUrMemory_Array_GetUsedElems(inGP->tg.permutations));
	
	perm_array = (SStPermutation*)UUrMemory_Array_GetMemory(inGP->tg.permutations);
	UUmAssert(perm_array);
	
	perm = &perm_array[perm_index];

	// play the permutation
	SSrPermutation_Play(perm, NULL, NULL, NULL);
}

// ----------------------------------------------------------------------
static UUtBool
OWiSGP_Save(
	WMtDialog					*inDialog,
	OWtGroupProp				*inGP)
{
	UUtError					error;
	WMtWindow					*editfield;
	char						name[BFcMaxFileNameLength];
	SStGroup					*found_group;
		
	// make sure the group has a name
	editfield = WMrDialog_GetItemByID(inDialog, OWcSGP_EF_Name);
	WMrEditField_GetText(editfield, name, SScMaxNameLength);
	
	if (strlen(name) == 0)
	{
		WMrDialog_MessageBox(
			inDialog,
			"Error",
			"The group must have a name before you can save it.",
			WMcMessageBoxStyle_OK);
		return UUcFalse;
	}
	
	OSrMakeGoodName(name, name);

	if (inGP->group == NULL)
	{
		found_group = OSrGroup_GetByName(name);
		if (found_group != NULL)
		{
			WMrDialog_MessageBox(
				inDialog,
				"Error",
				"A group with that name already exists.  Please change the name.",
				WMcMessageBoxStyle_OK);
			return UUcFalse;
		}
		
		// create the group
		error = OSrGroup_New(name, &inGP->group);
		if (error != UUcError_None)
		{
			WMrDialog_MessageBox(
				inDialog,
				"Error",
				"Unable to Create a new group",
				WMcMessageBoxStyle_OK);
			return UUcFalse;
		}
	}
	else
	{
		// create a new group if the name was changed
		found_group = OSrGroup_GetByName(name);
		if (found_group == NULL)
		{
			error = OSrGroup_New(name, &inGP->group);
			if (error != UUcError_None)
			{
				WMrDialog_MessageBox(
					inDialog,
					"Error",
					"Unable to Create a new group",
					WMcMessageBoxStyle_OK);
				return UUcFalse;
			}
		}
	}
	
	// set the data for the group
	UUrMemory_Array_Delete(inGP->group->permutations);
	
	inGP->group->group_volume = inGP->tg.group_volume;
	inGP->group->group_pitch = inGP->tg.group_pitch;
	inGP->group->flags = inGP->tg.flags;
	inGP->group->flag_data = inGP->tg.flag_data;
	inGP->group->permutations = inGP->tg.permutations;
	inGP->group->num_channels = inGP->tg.num_channels;
	
	inGP->tg.permutations = NULL;
	
	// save the group to disk
	OSrGroup_Save(inGP->group, inGP->parent_dir_ref);
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
static void
OWiSGP_InitDialog(
	WMtDialog					*inDialog)
{
	OWtGroupProp				*gp;
	WMtWindow					*editfield;
	WMtWindow					*checkbox;
	UUtError					error;
	
	// get the group properties
	gp = (OWtGroupProp*)WMrDialog_GetUserData(inDialog);
	if (gp == NULL)
	{
		WMrDialog_ModalEnd(inDialog, OWcSGP_Btn_Cancel);
		return;
	}
	gp->tg.group_volume = 1.0f;
	gp->tg.group_pitch = 1.0f;
	gp->tg.permutations = NULL;
	gp->tg.flags = SScGroupFlag_None;
	gp->tg.flag_data = 0;
	gp->tg.num_channels = 0;
	gp->can_save = UUcFalse;
		
	// set the maximum number of characters for the group name
	editfield = WMrDialog_GetItemByID(inDialog, OWcSGP_EF_Name);
	WMrMessage_Send(
		editfield,
		EFcMessage_SetMaxChars,
		BFcMaxFileNameLength - strlen(".abc"),
		0);
	
	// set the fields
	if (gp->group == NULL)
	{
		gp->tg.permutations =
			UUrMemory_Array_New(
				sizeof(SStPermutation),
				1,
				0,
				5);
		if (gp->tg.permutations == NULL)
		{
			WMrDialog_ModalEnd(inDialog, OWcSGP_Btn_Cancel);
			return;
		}
		
		if ((gp->parent_dir_ref != NULL) &&
			(BFrFileRef_FileExists(gp->parent_dir_ref) == UUcTrue))
		{
			gp->can_save = UUcTrue;
		}

		// set the volume field
		editfield = WMrDialog_GetItemByID(inDialog, OWcSGP_EF_Volume);
		WMrEditField_SetFloat(editfield, gp->tg.group_volume);

		// set the pitch field
		editfield = WMrDialog_GetItemByID(inDialog, OWcSGP_EF_Pitch);
		WMrEditField_SetFloat(editfield, gp->tg.group_pitch);
		
		// set the prevent repeats checkbox
		checkbox = WMrDialog_GetItemByID(inDialog, OWcSGP_CB_PreventRepeats);
		WMrCheckBox_SetCheck(checkbox, ((gp->tg.flags & SScGroupFlag_PreventRepeats) != 0));
	}
	else
	{
		UUtUns32					i;
		UUtUns32					num_permutations;
		SStPermutation				*perm_array;
		
		// get the values of the group
		gp->tg.group_volume = gp->group->group_volume;
		gp->tg.group_pitch = gp->group->group_pitch;
		gp->tg.num_channels = (UUtUns8)gp->group->num_channels;
		
		// set the name field
		WMrEditField_SetText(editfield, gp->group->group_name);
		
		// set the volume field
		editfield = WMrDialog_GetItemByID(inDialog, OWcSGP_EF_Volume);
		WMrEditField_SetFloat(editfield, gp->group->group_volume);

		// set the pitch field
		editfield = WMrDialog_GetItemByID(inDialog, OWcSGP_EF_Pitch);
		WMrEditField_SetFloat(editfield, gp->group->group_pitch);
		
		// set the prevent repeats checkbox
		checkbox = WMrDialog_GetItemByID(inDialog, OWcSGP_CB_PreventRepeats);
		WMrCheckBox_SetCheck(checkbox, ((gp->group->flags & SScGroupFlag_PreventRepeats) != 0));
		
		// create a temp permutation array
		num_permutations = UUrMemory_Array_GetUsedElems(gp->group->permutations);
		gp->tg.permutations =
			UUrMemory_Array_New(
				sizeof(SStPermutation),
				1,
				num_permutations,
				num_permutations);
		if (gp->tg.permutations == NULL)
		{
			WMrDialog_ModalEnd(inDialog, OWcSGP_Btn_Cancel);
			return;
		}
		
		// copy the permutations
		perm_array = (SStPermutation*)UUrMemory_Array_GetMemory(gp->tg.permutations);
		for (i = 0; i < num_permutations; i++)
		{
			SStPermutation			*perm;
			
			perm = SSrGroup_Permutation_Get(gp->group, i);
			perm_array[i] = *perm;
		}
		
		OWiSGP_FillListbox(inDialog);
		
		// does the group have a file ref
		if (gp->parent_dir_ref != NULL)
		{
			BFtFileRef				*dir_ref;
			char					name[BFcMaxFileNameLength];
			
			sprintf(name, "%s.%s", gp->group->group_name, OScGroupSuffix);
			
			error =
				BFrFileRef_DuplicateAndAppendName(
					gp->parent_dir_ref,
					name,
					&dir_ref);
			if (error == UUcError_None)
			{
				if (BFrFileRef_FileExists(dir_ref) && (BFrFileRef_IsLocked(dir_ref) == UUcFalse))
				{
					gp->can_save = UUcTrue;
				}
				
				UUrMemory_Block_Delete(dir_ref);
				dir_ref = NULL;
			}
		}
	}
	
	OWiSGP_EnableButtons(inDialog, gp);
	
	// set the focus to the name
	WMrWindow_SetFocus(WMrDialog_GetItemByID(inDialog, OWcSGP_EF_Name));
}

// ----------------------------------------------------------------------
static void
OWiSGP_HandleCommand(
	WMtDialog					*inDialog,
	WMtWindow					*inControl,
	UUtUns32					inParam)
{
	UUtUns16					control_id;
	UUtUns16					command_type;
	OWtGroupProp				*gp;
	
	gp = (OWtGroupProp*)WMrDialog_GetUserData(inDialog);
	
	control_id = UUmLowWord(inParam);
	command_type = UUmHighWord(inParam);
	
	switch (control_id)
	{
		case OWcSGP_EF_Name:
			if (command_type != EFcNotify_ContentChanged) { break; }
			
			// if the sound group was locked, then the can_save bool
			// is null.  If the name changes, then the user wants to make
			// a new sound group so allow this by setting the group to
			// null and setting can_save to UUcTrue
			if (gp->can_save == UUcFalse) 
			{
				gp->group = NULL;
				gp->can_save = UUcTrue;
			}
		break;
		
		case OWcSGP_LB_Permutations:
			if (command_type == WMcNotify_DoubleClick)
			{
				OWiSGP_Perm_Edit(inDialog, gp);
			}
		break;
		
		case OWcSGP_Btn_AddPerm:
			if (command_type != WMcNotify_Click) { break; }
			OWiSGP_Perm_Add(inDialog, gp);
		break;
		
		case OWcSGP_Btn_EditPerm:
			if (command_type != WMcNotify_Click) { break; }
			OWiSGP_Perm_Edit(inDialog, gp);
		break;
		
		case OWcSGP_Btn_PlayPerm:
			if (command_type != WMcNotify_Click) { break; }
			OWiSGP_Perm_Play(inDialog, gp);
		break;
		
		case OWcSGP_Btn_DeletePerm:
			if (command_type != WMcNotify_Click) { break; }
			OWiSGP_Perm_Delete(inDialog, gp);
		break;
		
		case OWcSGP_Btn_Play:
			if (command_type != WMcNotify_Click) { break; }
			SSrGroup_Play(&gp->tg, NULL, NULL, NULL);
		break;
		
		case OWcSGP_EF_Volume:
			if (command_type != EFcNotify_ContentChanged) { break; }
			gp->tg.group_volume = WMrEditField_GetFloat(inControl);
		break;
				
		case OWcSGP_EF_Pitch:
			if (command_type != EFcNotify_ContentChanged) { break; }
			gp->tg.group_pitch = WMrEditField_GetFloat(inControl);
		break;
		
		case OWcSGP_CB_PreventRepeats:
			if (command_type != WMcNotify_Click) { break; }
			gp->tg.flags = WMrCheckBox_GetCheck(inControl);
		break;
		
		case OWcSGP_Btn_Cancel:
			if (command_type != WMcNotify_Click) { break; }
			WMrDialog_ModalEnd(inDialog, OWcSGP_Btn_Cancel);
		break;
		
		case OWcSGP_Btn_Save:
			if (command_type != WMcNotify_Click) { break; }
			if (OWiSGP_Save(inDialog, gp))
			{
				WMrDialog_ModalEnd(inDialog, OWcSGP_Btn_Save);
			}
		break;
	}
	
	OWiSGP_EnableButtons(inDialog, gp);
}

// ----------------------------------------------------------------------
static void
OWiSGP_HandleDestroy(
	WMtDialog					*inDialog)
{
	OWtGroupProp				*gp;
	
	gp = (OWtGroupProp*)WMrDialog_GetUserData(inDialog);
	if ((gp) && (gp->tg.permutations))
	{
		UUrMemory_Array_Delete(gp->tg.permutations);
		gp->tg.permutations = NULL;
	}
}

// ----------------------------------------------------------------------
static void
OWiSGP_HandleDrawItem(
	WMtDialog					*inDialog,
	WMtDrawItem					*inDrawItem)
{
	OWtGroupProp				*gp;
	IMtPoint2D					dest;
	UUtInt16					line_width;
	UUtInt16					line_height;
	SStPermutation				*perm_array;
	SStPermutation				*perm;
	char						string[128];
	
	// get a pointer to the group properties
	gp = (OWtGroupProp*)WMrDialog_GetUserData(inDialog);
	if ((gp == NULL) || (gp->tg.permutations == NULL)) { return; }
	if (inDrawItem->data >= UUrMemory_Array_GetUsedElems(gp->tg.permutations)) { return; }
	
	// get a pointer to the permutation
	perm_array = (SStPermutation*)UUrMemory_Array_GetMemory(gp->tg.permutations);
	if (perm_array == NULL) { return; }
	
	perm = &perm_array[inDrawItem->data];
	if (perm == NULL) { return; }
	
	// set the dest
	dest.x = inDrawItem->rect.left;
	dest.y = inDrawItem->rect.top;
	
	// calc the width and height of the line
	line_width = inDrawItem->rect.right - inDrawItem->rect.left;
	line_height = inDrawItem->rect.bottom - inDrawItem->rect.top;

	// setup the text drawing
	DCrText_SetFormat(TSc_HLeft | TSc_VCenter);
	DCrText_SetStyle(TScStyle_Plain);
	if (perm->sound_data == NULL) { DCrText_SetShade(IMcShade_Red); }
	else { DCrText_SetShade(IMcShade_Black); }
	
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
			OWiSGP_HandleDestroy(inDialog);
		break;
		
		case WMcMessage_Command:
			OWiSGP_HandleCommand(inDialog, (WMtWindow*)inParam2, inParam1);
		break;
		
		case WMcMessage_DrawItem:
			OWiSGP_HandleDrawItem(inDialog, (WMtDrawItem*)inParam2);			
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
OWiEditGroup(
	WMtDialog					*inParentDialog,
	SStGroup					*inGroup)
{
	UUtError					error;
	BFtFileRef					*file_ref;
	BFtFileRef					*start_ref;
	BFtFileRef					*parent_dir_ref;
	OWtGroupProp				gp;
	
	// get the parent directory of the sound
	error = OSrGetSoundBinaryDirectory(&start_ref);
	if (error != UUcError_None) { goto error; }
	
	error = OSrGetSoundFileRef(inGroup->group_name, OScGroupSuffix, start_ref, &file_ref);
	if (error != UUcError_None) { goto error; }
	
	error = BFrFileRef_GetParentDirectory(file_ref, &parent_dir_ref);
	if (error != UUcError_None) { goto error; }
	
	// set up the group properties
	UUrMemory_Clear(&gp, sizeof(OWtGroupProp));
	gp.parent_dir_ref = parent_dir_ref;
	gp.group = inGroup;
	
	if (gp.parent_dir_ref == NULL) { goto error; }
	
	// edit the group
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Sound_Group_Prop,
			inParentDialog,
			OWiSGP_Callback,
			(UUtUns32)&gp,
			NULL);
	if (error != UUcError_None) { goto error; }

	// cleanup
	if (start_ref)
	{
		BFrFileRef_Dispose(start_ref);
		start_ref = NULL;
	}
	
	if (file_ref)
	{
		BFrFileRef_Dispose(file_ref);
		file_ref = NULL;
	}
	
	if (gp.parent_dir_ref != NULL)
	{
		BFrFileRef_Dispose(gp.parent_dir_ref);
		gp.parent_dir_ref = NULL;
	}

	return;
	
error:
	if (start_ref)
	{
		BFrFileRef_Dispose(start_ref);
		start_ref = NULL;
	}
	
	if (file_ref)
	{
		BFrFileRef_Dispose(file_ref);
		file_ref = NULL;
	}
	
	if (gp.parent_dir_ref != NULL)
	{
		BFrFileRef_Dispose(gp.parent_dir_ref);
		gp.parent_dir_ref = NULL;
	}

	WMrDialog_MessageBox(
		inParentDialog,
		"Error",
		"Unable to edit the sound group.",
		WMcMessageBoxStyle_OK);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OWiISP_EnableButtons(
	WMtDialog					*inDialog,
	OWtImpulseProp				*inIP)
{
	WMtWindow					*button;
	WMtWindow					*editfield;
	char						name[SScMaxNameLength];
	SStImpulse					*found_impulse;
	UUtBool						no_name_conflict;
	
	// determine if there is a name conflict
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_Name);
	WMrEditField_GetText(editfield, name, SScMaxNameLength);
	
	found_impulse = OSrImpulse_GetByName(name);
	if (name[0] == '\0')
	{
		no_name_conflict = UUcFalse;
	}
	else if ((found_impulse == NULL) || (found_impulse == inIP->impulse))
	{
		no_name_conflict = UUcTrue;
	}
	else
	{
		no_name_conflict = UUcFalse;
	}
		
	button = WMrDialog_GetItemByID(inDialog, OWcISP_Btn_Edit);
	WMrWindow_SetEnabled(button, (inIP->ti.impulse_group != NULL));
	
	button = WMrDialog_GetItemByID(inDialog, OWcISP_Btn_Play);
	WMrWindow_SetEnabled(button, (inIP->ti.impulse_group != NULL));

	button = WMrDialog_GetItemByID(inDialog, OWcISP_Btn_Save);
	WMrWindow_SetEnabled(button, inIP->can_save && no_name_conflict);
	
	button = WMrDialog_GetItemByID(inDialog, OWcISP_Btn_EditImpulse);
	WMrWindow_SetEnabled(button, (inIP->ti.alt_impulse != NULL));
}

// ----------------------------------------------------------------------
static void
OWiISP_RecordData(
	WMtDialog					*inDialog,
	OWtImpulseProp				*inIP)
{
	WMtWindow					*editfield;
	WMtWindow					*popup;
	UUtUns16					priority;
	char						name[SScMaxNameLength];
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_Name);
	WMrEditField_GetText(editfield, name, SScMaxNameLength);
	UUrString_Copy(inIP->ti.impulse_name, name, SScMaxNameLength);
	
	popup = WMrDialog_GetItemByID(inDialog, OWcISP_PM_Priority);
	WMrPopupMenu_GetItemID(popup, -1, &priority);
	inIP->ti.priority = (SStPriority2)priority;
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MinVolDist);
	inIP->ti.min_volume_distance = WMrEditField_GetFloat(editfield);

	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MaxVolDist);
	inIP->ti.max_volume_distance = WMrEditField_GetFloat(editfield);

	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MaxVolAngle);
	inIP->ti.max_volume_angle = WMrEditField_GetFloat(editfield);
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MinVolAngle);
	inIP->ti.min_volume_angle = WMrEditField_GetFloat(editfield);
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MinAngleAttn);
	inIP->ti.min_angle_attenuation = WMrEditField_GetFloat(editfield);
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_ImpactVelocity);
	inIP->ti.impact_velocity = WMrEditField_GetFloat(editfield);
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MinOcclusion);
	inIP->ti.min_occlusion = WMrEditField_GetFloat(editfield);
	
	if (inIP->ti.impulse_group)
	{
		UUrString_Copy(
			inIP->ti.impulse_group_name,
			inIP->ti.impulse_group->group_name,
			SScMaxNameLength);
	}
	else
	{
		UUrMemory_Clear(inIP->ti.impulse_group_name, SScMaxNameLength);
	}
	
	if (inIP->ti.alt_impulse)
	{
		UUrString_Copy(
			inIP->ti.alt_impulse_name,
			inIP->ti.alt_impulse->impulse_name,
			SScMaxNameLength);
	}
	else
	{
		UUrMemory_Clear(inIP->ti.alt_impulse_name, SScMaxNameLength);
	}
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_Threshold);
	inIP->ti.alt_threshold = (UUtUns32)WMrEditField_GetInt32(editfield);
}

// ----------------------------------------------------------------------
static UUtBool
OWiISP_Save(
	WMtDialog					*inDialog,
	OWtImpulseProp				*inIP)
{
	UUtError					error;
	WMtWindow					*editfield;
	char						name[SScMaxNameLength];
	SStImpulse					*found_impulse;
	
	// make sure the sound has a name
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_Name);
	WMrEditField_GetText(editfield, name, SScMaxNameLength);
	
	if (strlen(name) == 0)
	{
		WMrDialog_MessageBox(
			inDialog,
			"Error",
			"The impulse sound must have a name before you can save it.",
			WMcMessageBoxStyle_OK);
		return UUcFalse;
	}
	
	OSrMakeGoodName(name, name);

	if (inIP->impulse == NULL)
	{
		found_impulse = OSrImpulse_GetByName(name);
		if (found_impulse != NULL)
		{
			WMrDialog_MessageBox(
				inDialog,
				"Error",
				"An impulse sound with that name already exists.  Please change the name.",
				WMcMessageBoxStyle_OK);
			return UUcFalse;
		}
		
		error = OSrImpulse_New(name, &inIP->impulse);
		if (error != UUcError_None)
		{
			WMrDialog_MessageBox(
				inDialog,
				"Error",
				"Unable to create the impulse sound.",
				WMcMessageBoxStyle_OK);
			return UUcFalse;
		}
	}
	else
	{
		// create a new impulse sound if the name was changed
		found_impulse = OSrImpulse_GetByName(name);
		if (found_impulse == NULL)
		{
			error = OSrImpulse_New(name, &inIP->impulse);
			if (error != UUcError_None)
			{
				WMrDialog_MessageBox(
					inDialog,
					"Error",
					"Unable to create the impulse sound.",
					WMcMessageBoxStyle_OK);
				return UUcFalse;
			}
		}
	}
	
	// record the data
	OWiISP_RecordData(inDialog, inIP);
	
	if (inIP->ti.min_volume_distance < inIP->ti.max_volume_distance)
	{
		WMrDialog_MessageBox(
			inDialog,
			"Error",
			"The maximum volume distance must be smaller than the minimum volume distance.",
			WMcMessageBoxStyle_OK);
		return UUcFalse;
	}
	
	// set the data for the impulse sound
	inIP->impulse->impulse_group = inIP->ti.impulse_group;
	inIP->impulse->priority = inIP->ti.priority;
	inIP->impulse->min_volume_distance = inIP->ti.min_volume_distance;
	inIP->impulse->max_volume_distance = inIP->ti.max_volume_distance;
	inIP->impulse->max_volume_angle = inIP->ti.max_volume_angle;
	inIP->impulse->min_volume_angle = inIP->ti.min_volume_angle;
	inIP->impulse->min_angle_attenuation = inIP->ti.min_angle_attenuation;
	inIP->impulse->alt_threshold = inIP->ti.alt_threshold;
	inIP->impulse->alt_impulse = inIP->ti.alt_impulse;
	inIP->impulse->impact_velocity = inIP->ti.impact_velocity;
	inIP->impulse->min_occlusion = inIP->ti.min_occlusion;
	
	if (inIP->impulse->impulse_group)
	{
		UUrString_Copy(
			inIP->impulse->impulse_group_name,
			inIP->impulse->impulse_group->group_name, 
			SScMaxNameLength);
	}
	
	if (inIP->impulse->alt_impulse)
	{
		UUrString_Copy(
			inIP->impulse->alt_impulse_name,
			inIP->impulse->alt_impulse->impulse_name,
			SScMaxNameLength);
	}
	
	// save the impulse sound to disk
	OSrImpulse_Save(inIP->impulse, inIP->parent_dir_ref);

	return UUcTrue;
}

// ----------------------------------------------------------------------
static void
OWiISP_SetFields(
	WMtDialog					*inDialog,
	OWtImpulseProp				*inIP)
{
	WMtWindow					*editfield;
	WMtWindow					*text;
	WMtWindow					*popup;
	
	if (inIP->impulse)
	{
		editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_Name);
		WMrEditField_SetText(editfield, inIP->impulse->impulse_name);
	}
	
	text = WMrDialog_GetItemByID(inDialog, OWcISP_Txt_Group);
	WMrWindow_SetTitle(text, inIP->ti.impulse_group_name, SScMaxNameLength);
	if (inIP->ti.impulse_group) { WMrText_SetShade(text, IMcShade_Black); }
	else { WMrText_SetShade(text, IMcShade_Red); }
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MaxVolDist);
	WMrEditField_SetFloat(editfield, inIP->ti.max_volume_distance);

	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MinVolDist);
	WMrEditField_SetFloat(editfield, inIP->ti.min_volume_distance);

	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MaxVolAngle);
	WMrEditField_SetFloat(editfield, inIP->ti.max_volume_angle);

	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MinVolAngle);
	WMrEditField_SetFloat(editfield, inIP->ti.min_volume_angle);

	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MinAngleAttn);
	WMrEditField_SetFloat(editfield, inIP->ti.min_angle_attenuation);
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_ImpactVelocity);
	WMrEditField_SetFloat(editfield, inIP->ti.impact_velocity);
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MinOcclusion);
	WMrEditField_SetFloat(editfield, inIP->ti.min_occlusion);
	
	popup = WMrDialog_GetItemByID(inDialog, OWcISP_PM_Priority);
	WMrPopupMenu_SetSelection(popup, inIP->ti.priority);
	
	text = WMrDialog_GetItemByID(inDialog, OWcISP_Txt_AltImpulse);
	WMrWindow_SetTitle(text, inIP->ti.alt_impulse_name, SScMaxNameLength);
	if (inIP->ti.alt_impulse_name) { WMrText_SetShade(text, IMcShade_Black); }
	else { WMrText_SetShade(text, IMcShade_Red); }
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_Threshold);
	WMrEditField_SetInt32(editfield, inIP->ti.alt_threshold);
}

// ----------------------------------------------------------------------
static void
OWiISP_InitDialog(
	WMtDialog					*inDialog)
{
	UUtError					error;
	OWtImpulseProp				*ip;
	WMtWindow					*editfield;
	
	// get the impulse properties
	ip = (OWtImpulseProp*)WMrDialog_GetUserData(inDialog);
	if (ip == NULL)
	{
		WMrDialog_ModalEnd(inDialog, OWcISP_Btn_Cancel);
		return;
	}
	
	// initialize the impulse properties
	if (ip->impulse == NULL)
	{
		UUrMemory_Clear(ip->ti.impulse_name, SScMaxNameLength);
		UUrMemory_Clear(ip->ti.impulse_group_name, SScMaxNameLength);
		ip->ti.impulse_group = NULL;

		ip->ti.priority = SScPriority2_Normal;
		
		ip->ti.max_volume_distance = 10.0f;
		ip->ti.min_volume_distance = 50.0f;
		
		ip->ti.min_volume_angle = 360.0f;
		ip->ti.max_volume_angle = 360.0f;
		ip->ti.min_angle_attenuation = 0.0f;
		
		ip->ti.alt_threshold = 0;
		ip->ti.alt_impulse = NULL;
		UUrMemory_Clear(ip->ti.alt_impulse_name, SScMaxNameLength);
		
		ip->ti.impact_velocity = 0.0f;
		ip->ti.min_occlusion = 0.0f;
		
		if ((ip->parent_dir_ref != NULL) &&
			(BFrFileRef_FileExists(ip->parent_dir_ref) == UUcTrue))
		{
			ip->can_save = UUcTrue;
		}
	}
	else
	{
		UUrString_Copy(ip->ti.impulse_name, ip->impulse->impulse_name, SScMaxNameLength);
		UUrString_Copy(ip->ti.impulse_group_name, ip->impulse->impulse_group_name, SScMaxNameLength);
		ip->ti.impulse_group = ip->impulse->impulse_group;
		
		ip->ti.priority = ip->impulse->priority;
		
		ip->ti.max_volume_distance = ip->impulse->max_volume_distance;
		ip->ti.min_volume_distance = ip->impulse->min_volume_distance;
		
		ip->ti.min_volume_angle = ip->impulse->min_volume_angle;
		ip->ti.max_volume_angle = ip->impulse->max_volume_angle;
		ip->ti.min_angle_attenuation = ip->impulse->min_angle_attenuation;
		
		ip->ti.alt_threshold = ip->impulse->alt_threshold;
		ip->ti.alt_impulse = ip->impulse->alt_impulse;
		UUrString_Copy(ip->ti.alt_impulse_name, ip->impulse->alt_impulse_name, SScMaxNameLength);
		
		ip->ti.impact_velocity = ip->impulse->impact_velocity;
		ip->ti.min_occlusion = ip->impulse->min_occlusion;
		
		// does the impulse sound have a file ref
		if (ip->parent_dir_ref != NULL)
		{
			BFtFileRef				*dir_ref;
			char					name[SScMaxNameLength];
			
			sprintf(name, "%s.%s", ip->impulse->impulse_name, OScImpulseSuffix);
			
			error =
				BFrFileRef_DuplicateAndAppendName(
					ip->parent_dir_ref,
					name,
					&dir_ref);
			if (error == UUcError_None)
			{
				if (BFrFileRef_FileExists(dir_ref) && (BFrFileRef_IsLocked(dir_ref) == UUcFalse))
				{
					ip->can_save = UUcTrue;
				}
				
				UUrMemory_Block_Delete(dir_ref);
				dir_ref = NULL;
			}
		}
	}
	
	// set the maximum number of characters for the impulse name
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_Name);
	WMrMessage_Send(
		editfield,
		EFcMessage_SetMaxChars,
		BFcMaxFileNameLength - strlen(".abc"),
		0);
	
	// set the fields
	OWiISP_SetFields(inDialog, ip);
	
	// enable the buttons
	OWiISP_EnableButtons(inDialog, ip);
	
	// disable the angle fields
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MinVolAngle);
	WMrWindow_SetEnabled(editfield, UUcFalse);
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MaxVolAngle);
	WMrWindow_SetEnabled(editfield, UUcFalse);
	
	editfield = WMrDialog_GetItemByID(inDialog, OWcISP_EF_MinAngleAttn);
	WMrWindow_SetEnabled(editfield, UUcFalse);
	
	// set the focus to the name
	WMrWindow_SetFocus(WMrDialog_GetItemByID(inDialog, OWcISP_EF_Name));
}

// ----------------------------------------------------------------------
static void
OWiISP_HandleCommand(
	WMtDialog					*inDialog,
	WMtWindow					*inControl,
	UUtUns32					inParam)
{
	UUtUns16					control_id;
	UUtUns16					command_type;
	OWtImpulseProp				*ip;
	SStGroup					*group;
	SStImpulse					*impulse;
	UUtBool						update_fields;
	OWtSelectResult				result;
	
	ip = (OWtImpulseProp*)WMrDialog_GetUserData(inDialog);
	
	update_fields = UUcTrue;

	control_id = UUmLowWord(inParam);
	command_type = UUmHighWord(inParam);
	
	switch (control_id)
	{
		case OWcISP_EF_Name:
			if (command_type != EFcNotify_ContentChanged) { break; }
			
			// if the impulse sound was locked, then the can_save bool
			// is null.  If the name changes, then the user wants to make
			// a new impulse sound so allow this by setting the impulse to
			// null and setting can_save to UUcTrue
			if (ip->can_save == UUcFalse) 
			{
				ip->impulse = NULL;
				ip->can_save = UUcTrue;
			}
			update_fields = UUcFalse;
		break;
		
		case OWcISP_Btn_Set:
			if (command_type != WMcNotify_Click) { break; }
			result = OWrSelect_SoundGroup(&group);
			if (result == OWcSelectResult_Cancel) { break; }
			ip->ti.impulse_group = group;
			
			if (ip->ti.impulse_group != NULL)
			{
				UUrString_Copy(
					ip->ti.impulse_group_name,
					ip->ti.impulse_group->group_name,
					SScMaxNameLength);
			}
			else
			{
				UUrMemory_Clear(ip->ti.impulse_group_name, SScMaxNameLength);
			}
		break;
		
		case OWcISP_Btn_Edit:
			if (command_type != WMcNotify_Click) { break; }
			OWiEditGroup(inDialog, ip->ti.impulse_group);
		break;
		
		case OWcISP_Btn_Play:
			if (command_type != WMcNotify_Click) { break; }
			if (ip->ti.impulse_group)
			{
				M3tPoint3D				position;
				float					volume;
				
				OWiISP_RecordData(inDialog, ip);
				
				MUmVector_Set(position, 1.0f, 0.0f, 0.0f);
				SSrListener_SetPosition(&position, &position);
				
				MUmVector_Set(position, 0.0f, 0.0f, 1.0f);
				volume = 1.0f;

				SSrImpulse_Play(
					&ip->ti,
					&position,
					NULL,
					NULL,
					&volume);
			}
		break;
		
		case OWcISP_Btn_SetImpulse:
		{
			OWtSelectResult				result;
			
			if (command_type != WMcNotify_Click) { break; }
			result = OWrSelect_ImpulseSound(&impulse);
			if (result == OWcSelectResult_Cancel) { break; }
			ip->ti.alt_impulse = impulse;
			
			if (ip->ti.alt_impulse != NULL)
			{
				UUrString_Copy(
					ip->ti.alt_impulse_name,
					ip->ti.alt_impulse->impulse_name,
					SScMaxNameLength);
			}
			else
			{
				UUrMemory_Clear(ip->ti.alt_impulse_name, SScMaxNameLength);
			}
		}
		break;
		
		case OWcISP_Btn_EditImpulse:
			if (command_type != WMcNotify_Click) { break; }
			OWiEditImpulse(inDialog, ip->ti.alt_impulse);
		break;
		
		case OWcISP_Btn_Cancel:
			if (command_type != WMcNotify_Click) { break; }
			WMrDialog_ModalEnd(inDialog, OWcISP_Btn_Cancel);
		break;
		
		case OWcISP_Btn_Save:
			if (command_type != WMcNotify_Click) { break; }
			if (OWiISP_Save(inDialog, ip) == UUcTrue)
			{
				WMrDialog_ModalEnd(inDialog, OWcISP_Btn_Save);
			}
		break;
		
		default:
			update_fields = UUcFalse;
		break;
	}
	
	if (update_fields)
	{
		OWiISP_SetFields(inDialog, ip);
	}
	
	// update the buttons
	OWiISP_EnableButtons(inDialog, ip);
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
		
		case WMcMessage_Command:
			OWiISP_HandleCommand(inDialog, (WMtWindow*)inParam2, inParam1);
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ----------------------------------------------------------------------
UUtError
OWrImpulseProperties_Display(
	WMtDialog					*inParentDialog,
	SStImpulse					*inImpulse)
{
	UUtError					error;
	OWtImpulseProp				ip;
	BFtFileRef					*file_ref;
	BFtFileRef					*dir_ref;
	
	UUmAssert(inImpulse);
	
	dir_ref = NULL;
	file_ref = NULL;
	
	// get the sound binary directory
	error = OSrGetSoundBinaryDirectory(&dir_ref);
	if (error != UUcError_None) { goto error; }
	
	// get the file ref of the sound
	error =
		OSrGetSoundFileRef(
			inImpulse->impulse_name,
			OScImpulseSuffix,
			dir_ref,
			&file_ref);
	if (error != UUcError_None) { goto error; }
	
	// dispose of the sound binary directory ref
	BFrFileRef_Dispose(dir_ref);
	dir_ref = NULL;
	
	// get the parent directory of the sound
	error = BFrFileRef_GetParentDirectory(file_ref, &dir_ref);
	if (error != UUcError_None) { goto error; }
	
	// dispose of the file ref of the sound
	BFrFileRef_Dispose(file_ref);
	file_ref = NULL;
	
	// set up the properties
	UUrMemory_Clear(&ip, sizeof(OWtImpulseProp));
	ip.parent_dir_ref = dir_ref;
	ip.impulse = inImpulse;
		
	// edit the sound
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Impulse_Sound_Prop,
			inParentDialog,
			OWiISP_Callback,
			(UUtUns32)&ip,
			NULL);
	if (error != UUcError_None) { goto error; }
	
	// dispose of the parent directory of the sound
	BFrFileRef_Dispose(dir_ref);
	dir_ref = NULL;
	
	return UUcError_None;
	
error:
	if (dir_ref)
	{
		BFrFileRef_Dispose(dir_ref);
		dir_ref = NULL;
	}
	
	if (file_ref)
	{
		BFrFileRef_Dispose(file_ref);
		file_ref = NULL;
	}
	
	return error;
}	

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OWiEditImpulse(
	WMtDialog					*inParentDialog,
	SStImpulse					*inImpulse)
{
	UUtError					error;
	BFtFileRef					*file_ref;
	BFtFileRef					*start_ref;
	BFtFileRef					*parent_dir_ref;
	OWtImpulseProp				ip;
	
	// get the parent directory of the sound
	error = OSrGetSoundBinaryDirectory(&start_ref);
	if (error != UUcError_None) { goto error; }
	
	error = OSrGetSoundFileRef(inImpulse->impulse_name, OScImpulseSuffix, start_ref, &file_ref);
	if (error != UUcError_None) { goto error; }
	
	error = BFrFileRef_GetParentDirectory(file_ref, &parent_dir_ref);
	if (error != UUcError_None) { goto error; }
	
	// set up the impulse properties
	UUrMemory_Clear(&ip, sizeof(OWtImpulseProp));
	ip.parent_dir_ref = parent_dir_ref;
	ip.impulse = inImpulse;
	
	if (ip.parent_dir_ref == NULL) { goto error; }
	
	// edit the impulse
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Impulse_Sound_Prop,
			inParentDialog,
			OWiISP_Callback,
			(UUtUns32)&ip,
			NULL);
	if (error != UUcError_None) { goto error; }

	// cleanup
	if (start_ref)
	{
		BFrFileRef_Dispose(start_ref);
		start_ref = NULL;
	}
	
	if (file_ref)
	{
		BFrFileRef_Dispose(file_ref);
		file_ref = NULL;
	}
	
	if (ip.parent_dir_ref != NULL)
	{
		BFrFileRef_Dispose(ip.parent_dir_ref);
		ip.parent_dir_ref = NULL;
	}

	return;
	
error:
	if (start_ref)
	{
		BFrFileRef_Dispose(start_ref);
		start_ref = NULL;
	}
	
	if (file_ref)
	{
		BFrFileRef_Dispose(file_ref);
		file_ref = NULL;
	}
	
	if (ip.parent_dir_ref != NULL)
	{
		BFrFileRef_Dispose(ip.parent_dir_ref);
		ip.parent_dir_ref = NULL;
	}

	WMrDialog_MessageBox(
		inParentDialog,
		"Error",
		"Unable to edit the impulse sound.",
		WMcMessageBoxStyle_OK);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
OWiListDirectory(
	WMtWindow					*inListBox,
	BFtFileRef					*inDirRef,
	const char					*inSuffix)
{
	UUtError					error;
	BFtFileIterator				*file_iterator;
	
	// reset the listbox
	WMrListBox_Reset(inListBox);
	
	// create a file iterator
	error =
		BFrDirectory_FileIterator_New(
			inDirRef,
			NULL,
			NULL,
			&file_iterator);
	UUmError_ReturnOnError(error);
	
	// fill in the listbox
	while (1)
	{
		BFtFileRef				ref;
		char					name[BFcMaxFileNameLength];
		UUtUns32				index;
		UUtBool					is_dir;
		
		// get the next file or dir ref
		error = BFrDirectory_FileIterator_Next(file_iterator, &ref);
		if (error != UUcError_None) { break; }
		
		// find out if this ref is a directory
		is_dir = BFrFileRef_IsDirectory(&ref);
		
		if ((!is_dir) && (UUrString_Compare_NoCase(inSuffix, BFrFileRef_GetSuffixName(&ref)) != 0)) {
			continue;
		}
		
		// add the file or dir to the listbox
		UUrString_Copy(name, BFrFileRef_GetLeafName(&ref), BFcMaxFileNameLength);
		UUrString_StripExtension(name);
		index = WMrListBox_AddString(inListBox, name);
		WMrListBox_SetItemData(inListBox, (UUtUns32)is_dir, index);
	}
	
	// delete the file iterator
	BFrDirectory_FileIterator_Delete(file_iterator);
	file_iterator = NULL;
	
	// set the selection to the first item in the list
	WMrListBox_SetSelection(inListBox, UUcTrue, 0);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OWiSetPopup(
	WMtWindow					*inPopup,
	BFtFileRef					*inDirRef)
{
	UUtError					error;
	BFtFileRef					*binary_sound_dir_ref;
	BFtFileRef					*dir_ref;
	BFtFileRef					*parent;
	UUtUns16					i;
	
	binary_sound_dir_ref = NULL;
	dir_ref = NULL;

	error = OSrGetSoundBinaryDirectory(&binary_sound_dir_ref);
	UUmError_ReturnOnError(error);
	if (inDirRef == NULL) { inDirRef = binary_sound_dir_ref; }

	error = BFrFileRef_Duplicate(inDirRef, &dir_ref);
	UUmError_ReturnOnError(error);

	WMrPopupMenu_Reset(inPopup);
	i = 0;
	
	while (1)
	{
		WMtMenuItemData				item_data;
		
		// add the current directory name to the popup
		item_data.flags = WMcMenuItemFlag_Enabled;
		item_data.id = i++;
		UUrString_Copy(item_data.title, BFrFileRef_GetLeafName(dir_ref), WMcMaxTitleLength);
		WMrPopupMenu_InsertItem(inPopup, &item_data, 0);

		// stop if the root binary sound directory was added
		if (BFrFileRef_IsEqual(binary_sound_dir_ref, dir_ref) == UUcTrue) { break; }

		// get the parent directory
		error = BFrFileRef_GetParentDirectory(dir_ref, &parent);
		if (error != UUcError_None) { break; }

		BFrFileRef_Dispose(dir_ref);
		dir_ref = parent;
		parent = NULL;
	}
	
	WMrPopupMenu_SetSelection(inPopup, 0);
	
	BFrFileRef_Dispose(binary_sound_dir_ref);
	binary_sound_dir_ref = NULL;
	
	BFrFileRef_Dispose(dir_ref);
	dir_ref = NULL;
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OWiSM_EnableButtons(
	WMtDialog					*inDialog)
{
	WMtWindow					*listbox;
	WMtWindow					*button;
	UUtUns32					selected_index;
	UUtUns32					result;
	UUtBool						is_dir;
	UUtBool						is_item;
	OWtSMData					*sm_data;
	
	sm_data = (OWtSMData*)WMrDialog_GetUserData(inDialog);
	if (sm_data->dir_ref == NULL) { return; }

	listbox = WMrDialog_GetItemByID(inDialog, OWcSM_LB_Items);
	selected_index = WMrListBox_GetSelection(listbox);
	result = WMrListBox_GetItemData(listbox, selected_index);
	if (result == 0) { is_dir = UUcFalse; } else { is_dir = UUcTrue; }
	is_item = is_dir == UUcFalse;
	
	button = WMrDialog_GetItemByID(inDialog, OWcSM_Btn_EditItem);
	WMrWindow_SetEnabled(button, is_item);
	
	button = WMrDialog_GetItemByID(inDialog, OWcSM_Btn_DeleteItem);
	WMrWindow_SetEnabled(button, is_item);
	
	button = WMrDialog_GetItemByID(inDialog, OWcSM_Btn_PlayItem);
	if (button != NULL) { WMrWindow_SetEnabled(button, is_item); }
	
	button = WMrDialog_GetItemByID(inDialog, OWcSM_Btn_CopyItem);
	WMrWindow_SetEnabled(button, is_item);
	
	button = WMrDialog_GetItemByID(inDialog, OWcSM_Btn_PasteItem);
	WMrWindow_SetEnabled(button, sm_data->can_paste);
	
	button = WMrDialog_GetItemByID(inDialog, OWcSM_Btn_NewCategory);
	WMrWindow_SetEnabled(button, UUcTrue);

	button = WMrDialog_GetItemByID(inDialog, OWcSM_Btn_RenameCategory);
	WMrWindow_SetEnabled(button, UUcFalse);
	
	button = WMrDialog_GetItemByID(inDialog, OWcSM_Btn_DeleteCategory);
	WMrWindow_SetEnabled(button, UUcFalse);
}

// ----------------------------------------------------------------------
static BFtFileRef*
OWiSM_GetDirectoryRef(
	WMtDialog					*inDialog)
{
	UUtError					error;
	BFtFileRef					*out_ref;
	OWtSMData					*sm_data;
	
	sm_data = (OWtSMData*)WMrDialog_GetUserData(inDialog);
	if (sm_data->dir_ref == NULL) { return NULL; }
	
	error = BFrFileRef_Duplicate(sm_data->dir_ref, &out_ref);
	if (error != UUcError_None) { return NULL; }
	
	return out_ref;
}

// ----------------------------------------------------------------------
static const char*
OWiSM_GetExtension(
	WMtDialog					*inDialog)
{
	OWtSMData					*sm_data;
	
	sm_data = (OWtSMData*)WMrDialog_GetUserData(inDialog);
	UUmAssert(sm_data);
	
	return sm_data->extension;
}

// ----------------------------------------------------------------------
static OWtSMType
OWiSM_GetType(
	WMtDialog					*inDialog)
{
	OWtSMData					*sm_data;
	
	sm_data = (OWtSMData*)WMrDialog_GetUserData(inDialog);
	UUmAssert(sm_data);
	
	return sm_data->type;
}

// ----------------------------------------------------------------------
static void
OWiSM_SetDirectoryRef(
	WMtDialog					*inDialog,
	BFtFileRef					*inDirRef)
{
	UUtError					error;
	BFtFileRef					*old_ref;
	BFtFileRef					*copy_ref;
	OWtSMData					*sm_data;
	
	UUrMemory_Block_VerifyList();
	
	// get the sound manipulator data
	sm_data = (OWtSMData*)WMrDialog_GetUserData(inDialog);
	UUmAssert(sm_data);
	
	// delete the old ref
	old_ref = sm_data->dir_ref;
	if (old_ref != NULL)
	{
		BFrFileRef_Dispose(old_ref);
		old_ref = NULL;
	}
	
	if (inDirRef != NULL)
	{
		error = BFrFileRef_Duplicate(inDirRef, &copy_ref);
		if (error != UUcError_None) { copy_ref = NULL; } 
	}
	else
	{
		copy_ref = NULL;
	}
	
	// set the new ref
	sm_data->dir_ref = copy_ref;
	
	error = OWiSetPopup(WMrDialog_GetItemByID(inDialog, OWcSM_PM_Category), copy_ref);

	UUrMemory_Block_VerifyList();
}

// ----------------------------------------------------------------------
static void
OWiSM_Category_Open(
	WMtDialog					*inDialog,
	WMtWindow					*inListBox)
{
	UUtError					error;
	BFtFileRef					*parent_dir_ref;
	BFtFileRef					*open_dir_ref;
	char						name[BFcMaxFileNameLength];
	
	UUrMemory_Block_VerifyList();
	
	parent_dir_ref = NULL;
	open_dir_ref = NULL;
	name[0] = '\0';
	
	// get the parent dir ref
	parent_dir_ref = OWiSM_GetDirectoryRef(inDialog);
	if (parent_dir_ref == NULL) { return; }
	
	// make a dir ref for the directory being opened
	WMrListBox_GetText(inListBox, name, (UUtUns32)(-1));
	error = BFrFileRef_DuplicateAndAppendName(parent_dir_ref, name, &open_dir_ref);
	if (error != UUcError_None) { goto cleanup; }
	
	// fill in the listbox
	error = OWiListDirectory(inListBox, open_dir_ref, OWiSM_GetExtension(inDialog));
	if (error != UUcError_None) { goto cleanup; }
	
	OWiSM_SetDirectoryRef(inDialog, open_dir_ref);
	
cleanup:
	if (parent_dir_ref != NULL)
	{
		BFrFileRef_Dispose(parent_dir_ref);
		parent_dir_ref = NULL;
	}
	
	if (open_dir_ref != NULL)
	{
		BFrFileRef_Dispose(open_dir_ref);
		open_dir_ref = NULL;
	}

	UUrMemory_Block_VerifyList();
}

// ----------------------------------------------------------------------
static void
OWiSM_Category_New(
	WMtDialog					*inDialog)
{
	UUtError					error;
	UUtUns32					message;
	BFtFileRef					*parent_dir_ref;
	BFtFileRef					*new_dir_ref;
	char						name[BFcMaxFileNameLength];
	char						*error_string;
	
	parent_dir_ref = NULL;
	new_dir_ref = NULL;
	error_string = "Unable to create the category.";
	
	// get the name of the category
	UUrMemory_Clear(name, BFcMaxFileNameLength);
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Category_Name,
			inDialog,
			OWiCN_Callback,
			(UUtUns32)name,
			&message);
	if ((error != UUcError_None) || (message == OWcCN_Btn_Cancel)) { return; }
	
	// get the directory to place the category in
	parent_dir_ref = OWiSM_GetDirectoryRef(inDialog);
	
	// get a file ref for the directory
	error = BFrFileRef_DuplicateAndAppendName(parent_dir_ref, name, &new_dir_ref);
	if (error != UUcError_None) { goto error; }
	
	// see if this directory already exists
	if (BFrFileRef_FileExists(new_dir_ref) == UUcTrue)
	{
		error_string = "A category with this name already exists.";
		goto error;
	}
	
	// create the category as a directory on disk
	error = BFrDirectory_Create(new_dir_ref, NULL);
	if (error != UUcError_None) { goto error; }
	
	// set the directory as the focus
	OWiSM_SetDirectoryRef(inDialog, new_dir_ref);
	OWiListDirectory(
		WMrDialog_GetItemByID(inDialog, OWcSM_LB_Items),
		new_dir_ref,
		OWiSM_GetExtension(inDialog));
	OWiSM_EnableButtons(inDialog);
	
	// cleanup
	BFrFileRef_Dispose(parent_dir_ref);
	parent_dir_ref = NULL;
	
	BFrFileRef_Dispose(new_dir_ref);
	new_dir_ref = NULL;
	
	return;
	
error:
	if (parent_dir_ref != NULL)
	{
		BFrFileRef_Dispose(parent_dir_ref);
		parent_dir_ref = NULL;
	}
	
	if (new_dir_ref != NULL)
	{
		BFrFileRef_Dispose(new_dir_ref);
		new_dir_ref = NULL;
	}
	
	WMrDialog_MessageBox(
		inDialog,
		"Error",
		error_string,
		WMcMessageBoxStyle_OK);
}

// ----------------------------------------------------------------------
static void
OWiSM_Category_Delete_Contents(
	BFtFileRef					*inDirRef)
{
	UUtError					error;
	BFtFileIterator				*file_iterator;
	
	error =
		BFrDirectory_FileIterator_New(
			inDirRef,
			NULL,
			NULL,
			&file_iterator);
	if (error != UUcError_None) { return; }
	
	while (1)
	{
		BFtFileRef					file_ref;
		char						name[BFcMaxFileNameLength];
		char						ext[BFcMaxFileNameLength];
		
		error = BFrDirectory_FileIterator_Next(file_iterator, &file_ref);
		if (error != UUcError_None) { break; }
		
		if (BFrFileRef_IsDirectory(&file_ref) == UUcTrue)
		{
			OWiSM_Category_Delete_Contents(&file_ref);
			continue;
		}
		
		UUrString_Copy(name, BFrFileRef_GetLeafName(&file_ref), BFcMaxFileNameLength);
		UUrString_Copy(ext, strrchr(name, '.'), BFcMaxFileNameLength);
		UUrString_StripExtension(name);
		
		if (UUrString_Compare_NoCase(ext, ".amb") == 0)
		{
			OSrAmbient_Delete(name);
		}
		else if (UUrString_Compare_NoCase(ext, ".imp") == 0)
		{
			OSrImpulse_Delete(name);
		}
		else if (UUrString_Compare_NoCase(ext, ".grp") == 0)
		{
			OSrGroup_Delete(name);
		}
	}
	
	BFrDirectory_FileIterator_Delete(file_iterator);
	file_iterator = NULL;
}

// ----------------------------------------------------------------------
static void
OWiSM_Category_Delete(
	WMtDialog					*inDialog)
{
	UUtError					error;
	UUtUns32					result;
	WMtWindow					*listbox;
	BFtFileRef					*parent_dir_ref;
	BFtFileRef					*delete_dir_ref;
	char						name[BFcMaxFileNameLength];

	parent_dir_ref = NULL;
	delete_dir_ref = NULL;
	
	// make sure the user is aware of what is about to happen
	result =
		WMrDialog_MessageBox(
			inDialog,
			"Confirm",
			"Are you sure you want to delete the category, and all its contents?",
			WMcMessageBoxStyle_Yes_No);
	if (result == WMcDialogItem_No) { return; }
	
	// get the directory to delete the category from
	parent_dir_ref = OWiSM_GetDirectoryRef(inDialog);
	
	// get the name of the category
	listbox = WMrDialog_GetItemByID(inDialog, OWcSM_LB_Items);
	WMrListBox_GetText(listbox, name, (UUtUns32)(-1));
	
	// create a ref
	error = BFrFileRef_DuplicateAndAppendName(parent_dir_ref, name, &delete_dir_ref);
	if (error != UUcError_None) { goto error; }
	
	// delete the .amb, .imp, and .grp sounds seperately in order to 
	// update other systems that may use them.
//	OWiSM_Category_Delete_Contents(delete_dir_ref);
	
	// delete the directory and any remaining contents
	error = BFrDirectory_DeleteDirectoryAndContents(delete_dir_ref);
	if (error != UUcError_None) { goto error; }
	
	// set the directory as the focus
	OWiSM_SetDirectoryRef(inDialog, parent_dir_ref);
	OWiListDirectory(
		WMrDialog_GetItemByID(inDialog, OWcSM_LB_Items),
		parent_dir_ref,
		OWiSM_GetExtension(inDialog));
	OWiSM_EnableButtons(inDialog);
	
	// cleanup
	BFrFileRef_Dispose(parent_dir_ref);
	parent_dir_ref = NULL;
	
	BFrFileRef_Dispose(delete_dir_ref);
	delete_dir_ref = NULL;
	
	return;
	
error:
	if (parent_dir_ref != NULL)
	{
		BFrFileRef_Dispose(parent_dir_ref);
		parent_dir_ref = NULL;
	}
		
	if (delete_dir_ref != NULL)
	{
		BFrFileRef_Dispose(delete_dir_ref);
		delete_dir_ref = NULL;
	}
	
	WMrDialog_MessageBox(
		inDialog,
		"Error",
		"Unable to delete the category",
		WMcMessageBoxStyle_OK);
}

// ----------------------------------------------------------------------
static void
OWiSM_Item_Copy(
	WMtDialog					*inDialog)
{
	OWtSMData					*sm_data;
	WMtWindow					*listbox;
	char						selected_name[BFcMaxFileNameLength];
	
	sm_data = (OWtSMData*)WMrDialog_GetUserData(inDialog);
	if (sm_data->dir_ref == NULL) { return; }
	
	// get the name of the selected item the user wants to delete
	listbox = WMrDialog_GetItemByID(inDialog, OWcSM_LB_Items);
	WMrListBox_GetText(listbox, selected_name, (UUtUns32)(-1));

	switch (OWiSM_GetType(inDialog))
	{
		case OWcSMType_Ambient:
		{
			SStAmbient			*ambient;
			
			ambient = SSrAmbient_GetByName(selected_name);
			
			UUrMemory_MoveFast(ambient, &sm_data->u.ambient, sizeof(SStAmbient));
			UUrMemory_Clear(&sm_data->u.ambient.ambient_name, SScMaxNameLength);
		}
		break;
		
		case OWcSMType_Group:
		{
			SStGroup			*group;
			
			group = SSrGroup_GetByName(selected_name);
			
			UUrMemory_MoveFast(group, &sm_data->u.group, sizeof(SStGroup));
			UUrMemory_Clear(&sm_data->u.group.group_name, SScMaxNameLength);
		}
		break;
		
		case OWcSMType_Impulse:
		{
			SStImpulse			*impulse;
			
			impulse = SSrImpulse_GetByName(selected_name);
			
			UUrMemory_MoveFast(impulse, &sm_data->u.impulse, sizeof(SStImpulse));
			UUrMemory_Clear(&sm_data->u.impulse.impulse_name, SScMaxNameLength);
		}
		break;
	}
	
	sm_data->can_paste = UUcTrue;
}

// ----------------------------------------------------------------------
static void
OWiSM_Item_Delete(
	WMtDialog					*inDialog)
{
	UUtError					error;
	WMtWindow					*listbox;
	char						selected_name[BFcMaxFileNameLength];
	char						file_name[BFcMaxFileNameLength];
	BFtFileRef					*parent_dir_ref;
	BFtFileRef					*file_ref;
	char						*error_string;
	UUtUns32					message;

	// ask the user to confirm delete
	message =
		WMrDialog_MessageBox(
			inDialog,
			"Confirm Deletion",
			"Are you sure you want to delete the selected item?",
			WMcMessageBoxStyle_Yes_No);
	if (message == WMcDialogItem_No) { return; }

	parent_dir_ref = NULL;
	file_ref = NULL;
	
	// get the name of the selected item the user wants to delete
	listbox = WMrDialog_GetItemByID(inDialog, OWcSM_LB_Items);
	WMrListBox_GetText(listbox, selected_name, (UUtUns32)(-1));
	
	// get the parent directory
	parent_dir_ref = OWiSM_GetDirectoryRef(inDialog);
	if (parent_dir_ref == NULL) { goto error; }
	
	// delete the file
	file_ref = NULL;
	sprintf(file_name, "%s.%s", selected_name, OWiSM_GetExtension(inDialog));
	error = BFrFileRef_DuplicateAndAppendName(parent_dir_ref, file_name, &file_ref);
	if (error != UUcError_None) { goto error; }
	
	error = BFrFile_Delete(file_ref);
	if (error != UUcError_None) { goto error; }
	
UUrMemory_Block_VerifyList();

	// delete the selected item
	switch (OWiSM_GetType(inDialog))
	{
		case OWcSMType_Ambient:
			OSrAmbient_Delete(selected_name);
		break;
		
		case OWcSMType_Group:
			OSrGroup_Delete(selected_name);
		break;
		
		case OWcSMType_Impulse:
			OSrImpulse_Delete(selected_name);
		break;
	}
	
UUrMemory_Block_VerifyList();

	// update the listbox
	OWiListDirectory(listbox, parent_dir_ref, OWiSM_GetExtension(inDialog));
	OWiSM_EnableButtons(inDialog);
	
	// cleanup
	BFrFileRef_Dispose(parent_dir_ref);
	parent_dir_ref = NULL;

	BFrFileRef_Dispose(file_ref);
	file_ref = NULL;
	
UUrMemory_Block_VerifyList();

	return;

error:
	if (parent_dir_ref != NULL)
	{
		BFrFileRef_Dispose(parent_dir_ref);
		parent_dir_ref = NULL;
	}
	
	if ((file_ref != NULL) && (BFrFileRef_IsLocked(file_ref) == UUcTrue))
	{
		error_string = "Unable to delete the item because the file is locked.";

		BFrFileRef_Dispose(file_ref);
		file_ref = NULL;
	}
	else
	{
		error_string = "Unable to delete the selected item.";
	}

	WMrDialog_MessageBox(
		inDialog,
		"Error",
		error_string,
		WMcMessageBoxStyle_OK);
}

// ----------------------------------------------------------------------
static void
OWiSM_Item_Edit_Ambient(
	WMtDialog					*inDialog)
{
	UUtError					error;
	WMtWindow					*listbox;
	SStAmbient					*ambient;
	char						name[BFcMaxFileNameLength];
	UUtUns32					message;
	OWtAmbientProp				ap;
	
	// get the name of the ambient sound the user wants to edit
	listbox = WMrDialog_GetItemByID(inDialog, OWcSM_LB_Items);
	WMrListBox_GetText(listbox, name, (UUtUns32)(-1));
	
	// get the ambient to edit
	ambient = OSrAmbient_GetByName(name);
	if (ambient == NULL) { return; }
	
	// set up the ambient properties
	UUrMemory_Clear(&ap, sizeof(OWtAmbientProp));
	ap.parent_dir_ref = OWiSM_GetDirectoryRef(inDialog);
	ap.ambient = ambient;
	
	if (ap.parent_dir_ref == NULL) { goto error; }
	
	// edit the ambient
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Ambient_Sound_Prop,
			inDialog,
			OWiASP_Callback,
			(UUtUns32)&ap,
			&message);
	if (error != UUcError_None) { goto error; }

	// update the listbox
	OWiListDirectory(listbox, ap.parent_dir_ref, OWiSM_GetExtension(inDialog));
	WMrListBox_SelectString(listbox, name);
	OWiSM_EnableButtons(inDialog);
	
	// cleanup
	if (ap.parent_dir_ref != NULL)
	{
		BFrFileRef_Dispose(ap.parent_dir_ref);
		ap.parent_dir_ref = NULL;
	}
	
	return;

error:
	WMrDialog_MessageBox(
		inDialog,
		"Error",
		"Unable to edit the ambient sound.",
		WMcMessageBoxStyle_OK);
}

// ----------------------------------------------------------------------
static void
OWiSM_Item_Edit_Group(
	WMtDialog					*inDialog)
{
	UUtError					error;
	UUtUns32					message;
	OWtGroupProp				gp;
	WMtWindow					*listbox;
	SStGroup					*group;
	char						name[BFcMaxFileNameLength];
	
	// get the name of the group the user wants to edit
	listbox = WMrDialog_GetItemByID(inDialog, OWcSM_LB_Items);
	WMrListBox_GetText(listbox, name, (UUtUns32)(-1));
	
	// get the group to edit
	group = OSrGroup_GetByName(name);
	if (group == NULL) { return; }
	
	// set up the group properties
	UUrMemory_Clear(&gp, sizeof(OWtGroupProp));
	gp.parent_dir_ref = OWiSM_GetDirectoryRef(inDialog);
	gp.group = group;
	
	if (gp.parent_dir_ref == NULL) { goto error; }
	
	// edit the group
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Sound_Group_Prop,
			inDialog,
			OWiSGP_Callback,
			(UUtUns32)&gp,
			&message);
	if (error != UUcError_None) { goto error; }

	// update the listbox
	OWiListDirectory(listbox, gp.parent_dir_ref, OScGroupSuffix);
	WMrListBox_SelectString(listbox, name);
	OWiSM_EnableButtons(inDialog);
	
	// cleanup
	if (gp.parent_dir_ref != NULL)
	{
		BFrFileRef_Dispose(gp.parent_dir_ref);
		gp.parent_dir_ref = NULL;
	}
	
	return;

error:
	WMrDialog_MessageBox(
		inDialog,
		"Error",
		"Unable to edit the group.",
		WMcMessageBoxStyle_OK);
}

// ----------------------------------------------------------------------
static void
OWiSM_Item_Edit_Impulse(
	WMtDialog					*inDialog)
{
	UUtError					error;
	WMtWindow					*listbox;
	SStImpulse					*impulse;
	char						name[BFcMaxFileNameLength];
	UUtUns32					message;
	OWtImpulseProp				ip;
	
	// get the name of the impulse sound the user wants to edit
	listbox = WMrDialog_GetItemByID(inDialog, OWcSM_LB_Items);
	WMrListBox_GetText(listbox, name, (UUtUns32)(-1));
	
	// get the impulse sound to edit
	impulse = OSrImpulse_GetByName(name);
	if (impulse == NULL) { return; }
	
	// set up the impulse properties
	UUrMemory_Clear(&ip, sizeof(OWtImpulseProp));
	ip.parent_dir_ref = OWiSM_GetDirectoryRef(inDialog);
	ip.impulse = impulse;
	
	if (ip.parent_dir_ref == NULL) { goto error; }
	
	// edit the impulse sound
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Impulse_Sound_Prop,
			inDialog,
			OWiISP_Callback,
			(UUtUns32)&ip,
			&message);
	if (error != UUcError_None) { goto error; }

	// update the listbox
	OWiListDirectory(listbox, ip.parent_dir_ref, OWiSM_GetExtension(inDialog));
	WMrListBox_SelectString(listbox, name);
	OWiSM_EnableButtons(inDialog);
	
	// cleanup
	if (ip.parent_dir_ref != NULL)
	{
		BFrFileRef_Dispose(ip.parent_dir_ref);
		ip.parent_dir_ref = NULL;
	}
	
	return;

error:
	WMrDialog_MessageBox(
		inDialog,
		"Error",
		"Unable to edit the ambient sound.",
		WMcMessageBoxStyle_OK);
}

// ----------------------------------------------------------------------
static void
OWiSM_Item_Edit(
	WMtDialog					*inDialog)
{
	switch (OWiSM_GetType(inDialog))
	{
		case OWcSMType_Ambient:
			OWiSM_Item_Edit_Ambient(inDialog);
		break;
		
		case OWcSMType_Group:
			OWiSM_Item_Edit_Group(inDialog);
		break;
		
		case OWcSMType_Impulse:
			OWiSM_Item_Edit_Impulse(inDialog);
		break;
	}
}

// ----------------------------------------------------------------------
static void
OWiSM_Item_New_Ambient(
	WMtDialog					*inDialog)
{
	UUtError					error;
	UUtUns32					message;
	OWtAmbientProp				ap;
	
	// set up the ambient sound properties
	ap.parent_dir_ref = OWiSM_GetDirectoryRef(inDialog);
	ap.ambient = NULL;
	
	if (ap.parent_dir_ref == NULL)
	{
		WMrDialog_MessageBox(
			inDialog,
			"Error",
			"Unable to create new item.  There is no Binary Directory.",
			WMcMessageBoxStyle_OK);
		return;
	}
	
	// create the ambient sound
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Ambient_Sound_Prop,
			inDialog,
			OWiASP_Callback,
			(UUtUns32)&ap,
			&message);
	if (error != UUcError_None)
	{
		WMrDialog_MessageBox(
			inDialog,
			"Error",
			"Unable to create the new ambient sound",
			WMcMessageBoxStyle_OK);
			
		return;
	}
	
	if (message == OWcASP_Btn_Save)
	{
		WMtWindow					*listbox;
		
		listbox = WMrDialog_GetItemByID(inDialog, OWcSM_LB_Items);
		
		// fill in the listbox
		error = OWiListDirectory(listbox, ap.parent_dir_ref, OWiSM_GetExtension(inDialog));
		if (error != UUcError_None) { return; }
		
		// select the new group
		WMrListBox_SelectString(listbox, ap.ambient->ambient_name);
		OWiSM_EnableButtons(inDialog);
	}
	
	if (ap.parent_dir_ref != NULL)
	{
		BFrFileRef_Dispose(ap.parent_dir_ref);
		ap.parent_dir_ref = NULL;
	}
}

// ----------------------------------------------------------------------
static void
OWiSM_Item_New_Group(
	WMtDialog					*inDialog)
{
	UUtError					error;
	UUtUns32					message;
	OWtGroupProp				gp;
	
	// set up the group properties
	gp.parent_dir_ref = OWiSM_GetDirectoryRef(inDialog);
	gp.group = NULL;
	
	if (gp.parent_dir_ref == NULL)
	{
		WMrDialog_MessageBox(
			inDialog,
			"Error",
			"Unable to create new item.  There is no Binary Directory.",
			WMcMessageBoxStyle_OK);
		return;
	}
	
	// create the group
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Sound_Group_Prop,
			inDialog,
			OWiSGP_Callback,
			(UUtUns32)&gp,
			&message);
	if (error != UUcError_None)
	{
		WMrDialog_MessageBox(
			inDialog,
			"Error",
			"Unable to create the new group",
			WMcMessageBoxStyle_OK);
			
		return;
	}
	
	if (message == OWcSGP_Btn_Save)
	{
		WMtWindow					*listbox;
		
		listbox = WMrDialog_GetItemByID(inDialog, OWcSM_LB_Items);
		
		// fill in the listbox
		error = OWiListDirectory(listbox, gp.parent_dir_ref, OWiSM_GetExtension(inDialog));
		if (error != UUcError_None) { return; }
		
		// select the new group
		WMrListBox_SelectString(listbox, gp.group->group_name);
		OWiSM_EnableButtons(inDialog);
	}
	
	if (gp.parent_dir_ref != NULL)
	{
		BFrFileRef_Dispose(gp.parent_dir_ref);
		gp.parent_dir_ref = NULL;
	}
}

// ----------------------------------------------------------------------
static void
OWiSM_Item_New_Impulse(
	WMtDialog					*inDialog)
{
	UUtError					error;
	UUtUns32					message;
	OWtImpulseProp				ip;
	
	// set up the group properties
	ip.parent_dir_ref = OWiSM_GetDirectoryRef(inDialog);
	ip.impulse = NULL;
	
	if (ip.parent_dir_ref == NULL)
	{
		WMrDialog_MessageBox(
			inDialog,
			"Error",
			"Unable to create new item.  There is no Binary Directory.",
			WMcMessageBoxStyle_OK);
		return;
	}
	
	// create the impulse sound
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Impulse_Sound_Prop,
			inDialog,
			OWiISP_Callback,
			(UUtUns32)&ip,
			&message);
	if (error != UUcError_None)
	{
		WMrDialog_MessageBox(
			inDialog,
			"Error",
			"Unable to create the new group",
			WMcMessageBoxStyle_OK);
			
		return;
	}
	
	if (message == OWcISP_Btn_Save)
	{
		WMtWindow					*listbox;
		
		listbox = WMrDialog_GetItemByID(inDialog, OWcSM_LB_Items);
		
		// fill in the listbox
		error = OWiListDirectory(listbox, ip.parent_dir_ref, OWiSM_GetExtension(inDialog));
		if (error != UUcError_None) { return; }
		
		// select the new impulse sound
		WMrListBox_SelectString(listbox, ip.impulse->impulse_name);
		OWiSM_EnableButtons(inDialog);
	}
	
	if (ip.parent_dir_ref != NULL)
	{
		BFrFileRef_Dispose(ip.parent_dir_ref);
		ip.parent_dir_ref = NULL;
	}
}

// ----------------------------------------------------------------------
static void
OWiSM_Item_New(
	WMtDialog					*inDialog)
{
	switch (OWiSM_GetType(inDialog))
	{
		case OWcSMType_Ambient:
			OWiSM_Item_New_Ambient(inDialog);
		break;
		
		case OWcSMType_Group:
			OWiSM_Item_New_Group(inDialog);
		break;
		
		case OWcSMType_Impulse:
			OWiSM_Item_New_Impulse(inDialog);
		break;
	}
}

// ----------------------------------------------------------------------
static void
OWiSM_Item_Paste(
	WMtDialog					*inDialog)
{
	OWtSMData					*sm_data;
	UUtUns32					i;
	UUtBool						done;
	UUtError					error;
	BFtFileRef					*dir_ref;
	char						name[SScMaxNameLength];
	WMtWindow					*listbox;
	
	sm_data = (OWtSMData*)WMrDialog_GetUserData(inDialog);
	if (sm_data->dir_ref == NULL) { goto error; }
	
	dir_ref = OWiSM_GetDirectoryRef(inDialog);
	if (dir_ref == NULL) { goto error; }
	
	done = UUcFalse;
	i = 1;
	do
	{
		sprintf(name, "temp_%03d", i);
		
		switch (OWiSM_GetType(inDialog))
		{
			case OWcSMType_Ambient:
			{
				SStAmbient			*ambient;
				
				ambient = OSrAmbient_GetByName(name);
				if (ambient == NULL)
				{
					// create the new ambient sound
					error = OSrAmbient_New(name, &ambient);
					if (error != UUcError_None) { goto error; }
					
					// set all of the data for the ambient sound
					SSrAmbient_Copy(&sm_data->u.ambient, ambient);
					
					// save the new ambient sound
					OSrAmbient_Save(ambient, dir_ref);

					done = UUcTrue;
				}
			}
			break;
			
			case OWcSMType_Group:
			{
				SStGroup			*group;
				
				group = OSrGroup_GetByName(name);
				if (group == NULL)
				{
					// create the new sound group
					error = OSrGroup_New(name, &group);
					if (error != UUcError_None) { goto error; }
					
					// set all of the data for the sound group
					SSrGroup_Copy(&sm_data->u.group, group);
					
					// save the new sound group
					OSrGroup_Save(group, dir_ref);

					done = UUcTrue;
				}
			}
			break;
			
			case OWcSMType_Impulse:
			{
				SStImpulse			*impulse;
				
				impulse = OSrImpulse_GetByName(name);
				if (impulse == NULL)
				{
					// create the new impulse sound
					error = OSrImpulse_New(name, &impulse);
					if (error != UUcError_None) { goto error; }
					
					// set all of the data for the impulse sound
					SSrImpulse_Copy(&sm_data->u.impulse, impulse);
					
					// save the new impulse sound
					OSrImpulse_Save(impulse, dir_ref);

					done = UUcTrue;
				}
			}
			break;
		}
		
		i++;
	}
	while (done == UUcFalse);
	
	// refresh the listbox and select the item that was just added
	listbox = WMrDialog_GetItemByID(inDialog, OWcSM_LB_Items),
	OWiListDirectory(listbox, dir_ref, sm_data->extension);
	WMrListBox_SelectString(listbox, name);
	
	if (dir_ref)
	{
		BFrFileRef_Dispose(dir_ref);
		dir_ref = NULL;
	}
	
	return;

error:
	if (dir_ref)
	{
		BFrFileRef_Dispose(dir_ref);
		dir_ref = NULL;
	}
	
	WMrDialog_MessageBox(
		inDialog,
		"Error",
		"Unable to complete the paste.",
		WMcMessageBoxStyle_OK);
}

// ----------------------------------------------------------------------
static void
OWiSM_Item_Play(
	WMtDialog					*inDialog)
{
	WMtWindow					*listbox;
	char						name[BFcMaxFileNameLength];

	// get the name of the group the user wants to edit
	listbox = WMrDialog_GetItemByID(inDialog, OWcSM_LB_Items);
	WMrListBox_GetText(listbox, name, (UUtUns32)(-1));
	
	switch (OWiSM_GetType(inDialog))
	{
		case OWcSMType_Ambient:
		break;
		
		case OWcSMType_Group:
		{
			SStGroup					*group;
		
			// get the group to play
			group = OSrGroup_GetByName(name);
			if (group == NULL) { break; }
			
			// play the group
			SSrGroup_Play(group, NULL, NULL, NULL);
		}
		break;
		
		case OWcSMType_Impulse:
		{
			SStImpulse					*impulse;
			M3tPoint3D					position;
			float						volume;
			
			// get the impulse to play
			impulse = OSrImpulse_GetByName(name);
			if ((impulse == NULL) || (impulse->impulse_group == NULL)) { break; }
			
			MUmVector_Set(position, 1.0f, 0.0f, 0.0f);
			SSrListener_SetPosition(&position, &position);
			
			MUmVector_Set(position, 0.0f, 0.0f, 1.0f);
			volume = 1.0f;
			
			SSrImpulse_Play(
				impulse,
				&position,
				NULL,
				NULL,
				&volume);
		}
		break;
	}
}

// ----------------------------------------------------------------------
static void
OWiSM_InitDialog(
	WMtDialog					*inDialog)
{
	UUtError					error;
	WMtWindow					*listbox;
	OWtSMData					*sm_data;
	OWtSMType					type;
	BFtFileRef					*dir_ref;
	
	// get the type
	type = (OWtSMType)WMrDialog_GetUserData(inDialog);
	
	// allocate memory for the 
	sm_data = UUrMemory_Block_New(sizeof(OWtSMData));
	if (sm_data == NULL)
	{
		WMrDialog_ModalEnd(inDialog, 0);
		return;
	}
	
	WMrDialog_SetUserData(inDialog, (UUtUns32)sm_data);
	
	// set up the sm_data
	sm_data->type = type;
	sm_data->dir_ref = NULL;
	sm_data->can_paste = UUcFalse;
	
	switch (sm_data->type)
	{
		case OWcSMType_Ambient:
			sm_data->extension = OScAmbientSuffix;
		break;
		
		case OWcSMType_Group:
			sm_data->extension = OScGroupSuffix;
		break;
		
		case OWcSMType_Impulse:
			sm_data->extension = OScImpulseSuffix;
		break;
	}
	
	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcSM_LB_Items);
	if (listbox == NULL) { goto cleanup; }
	
	// get the directory ref
	error = OSrGetSoundBinaryDirectory(&dir_ref);
	if (error != UUcError_None) { goto cleanup; }
	
	// fill in the listbox
	error = OWiListDirectory(listbox, dir_ref, sm_data->extension);
	if (error != UUcError_None) { goto cleanup; }
	
	// set the directory ref
	OWiSM_SetDirectoryRef(inDialog, dir_ref);
	
	OWiSM_EnableButtons(inDialog);

	BFrFileRef_Dispose(dir_ref);
	dir_ref = NULL;
	
	return;
	
cleanup:
	if (dir_ref)
	{
		BFrFileRef_Dispose(dir_ref);
		dir_ref = NULL;
	}
	WMrDialog_ModalEnd(inDialog, 0);
}

// ----------------------------------------------------------------------
static void
OWiSM_HandleCommand(
	WMtDialog					*inDialog,
	WMtWindow					*inControl,
	UUtUns32					inParam)
{
	UUtUns16					control_id;
	UUtUns16					command_type;
	
	control_id = UUmLowWord(inParam);
	command_type = UUmHighWord(inParam);
	
	switch (control_id)
	{
		case WMcDialogItem_Cancel:
			if (command_type != WMcNotify_Click) { break; }
			WMrDialog_ModalEnd(inDialog, 0);
		break;
		
		case OWcSM_LB_Items:
			if (command_type == LBcNotify_SelectionChanged)
			{
				OWiSM_EnableButtons(inDialog);
			}
			else if (command_type == WMcNotify_DoubleClick)
			{
				UUtUns32				item_data;
				
				item_data = WMrListBox_GetItemData(inControl, (UUtUns32)(-1));
				if (item_data != 0)
				{
					// open the category
					OWiSM_Category_Open(inDialog, inControl);
				}
				else
				{
					// edit the item
					OWiSM_Item_Edit(inDialog);
				}
			}
		break;
		
		case OWcSM_Btn_NewItem:
			if (command_type != WMcNotify_Click) { break; }
			OWiSM_Item_New(inDialog);
		break;
		
		case OWcSM_Btn_EditItem:
			if (command_type != WMcNotify_Click) { break; }
			OWiSM_Item_Edit(inDialog);
		break;
		
		case OWcSM_Btn_PlayItem:
			if (command_type != WMcNotify_Click) { break; }
			OWiSM_Item_Play(inDialog);
		break;
		
		case OWcSM_Btn_DeleteItem:
			if (command_type != WMcNotify_Click) { break; }
			OWiSM_Item_Delete(inDialog);
		break;
		
		case OWcSM_Btn_CopyItem:
			if (command_type != WMcNotify_Click) { break; }
			OWiSM_Item_Copy(inDialog);
		break;
		
		case OWcSM_Btn_PasteItem:
			if (command_type != WMcNotify_Click) { break; }
			OWiSM_Item_Paste(inDialog);
		break;
		
		case OWcSM_Btn_NewCategory:
			if (command_type != WMcNotify_Click) { break; }
			OWiSM_Category_New(inDialog);
		break;
		
		case OWcSM_Btn_DeleteCategory:
			if (command_type != WMcNotify_Click) { break; }
			OWiSM_Category_Delete(inDialog);
		break;
	}
	
	OWiSM_EnableButtons(inDialog);
}

// ----------------------------------------------------------------------
static void
OWiSM_HandleDestroy(
	WMtDialog					*inDialog)
{
	OWtSMData					*sm_data;
	
	UUrMemory_Block_VerifyList();
	
	sm_data = (OWtSMData*)WMrDialog_GetUserData(inDialog);
	if (sm_data)
	{
		if (sm_data->dir_ref)
		{
			BFrFileRef_Dispose(sm_data->dir_ref);
			sm_data->dir_ref = NULL;
		}
		
		UUrMemory_Block_Delete(sm_data);
		WMrDialog_SetUserData(inDialog, 0);
	}
	
	UUrMemory_Block_VerifyList();
}

// ----------------------------------------------------------------------
static void
OWiSM_HandleMenuCommand(
	WMtDialog					*inDialog,
	WMtWindow					*inPopupMenu,
	UUtUns16					inItemID)
{
	UUtUns32					i;
	BFtFileRef					*dir_ref;
	UUtError					error;
	
	// if inItemID == 0 then the popup menu is still on the same item
	// so no updating needs to be done
	if (inItemID == 0) { return; }
	
	// get the dir ref of the current item in the popup menu
	dir_ref = OWiSM_GetDirectoryRef(inDialog);
	if (dir_ref == NULL) { return; }
	
	// go up the hierarchy of directories until the desired directory is found
	for (i = 0; i < (UUtUns32)inItemID; i++)
	{
		BFtFileRef					*parent_ref;
		
		error = BFrFileRef_GetParentDirectory(dir_ref, &parent_ref);
		if (error != UUcError_None) { return; }
		
		BFrFileRef_Dispose(dir_ref);
		dir_ref = parent_ref;
	}
	
	// fill in the listbox
	OWiListDirectory(
		WMrDialog_GetItemByID(inDialog, OWcSM_LB_Items),
		dir_ref,
		OWiSM_GetExtension(inDialog));
	
	OWiSM_SetDirectoryRef(inDialog, dir_ref);
	
	BFrFileRef_Dispose(dir_ref);
	dir_ref = NULL;
}

// ----------------------------------------------------------------------
static void
OWiSM_HandleDrawItem(
	WMtDialog					*inDialog,
	WMtDrawItem					*inDrawItem)
{
	IMtPoint2D					dest;
	PStPartSpecUI				*partspec_ui;
	UUtInt16					line_width;
	UUtInt16					line_height;
	PStPartSpec					*partspec;
	
	// get a pointer to the partspec_ui
	partspec_ui = PSrPartSpecUI_GetActive();
	UUmAssert(partspec_ui);
	
	// set the dest
	dest.x = inDrawItem->rect.left;
	dest.y = inDrawItem->rect.top;
	
	// calc the width and height of the line
	line_width = inDrawItem->rect.right - inDrawItem->rect.left;
	line_height = inDrawItem->rect.bottom - inDrawItem->rect.top;
	
	// set the text destination
	dest.x += 2;
	dest.y += 1;
	
	// draw the text
	DCrText_SetFormat(TSc_HLeft | TSc_VCenter);
	DCrText_SetStyle(TScStyle_Plain);
	DCrText_SetShade(IMcShade_Black);
	if (inDrawItem->data != 0)
	{
		partspec = partspec_ui->folder;
	}
	else
	{
		partspec = partspec_ui->file;
	}
	
	// draw the icon
	DCrDraw_PartSpec(
		inDrawItem->draw_context,
		partspec,
		PScPart_MiddleMiddle,
		&dest,
		(line_height - 3),
		(line_height - 3),
		M3cMaxAlpha);
	
	// draw the name
	dest.x += line_height + 2;
	DCrDraw_String(
		inDrawItem->draw_context,
		inDrawItem->string,
		&inDrawItem->rect,
		&dest);
}

// ----------------------------------------------------------------------
static UUtBool
OWiSM_Callback(
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
			OWiSM_InitDialog(inDialog);
		break;
		
		case WMcMessage_Destroy:
			OWiSM_HandleDestroy(inDialog);
		break;
		
		case WMcMessage_Command:
			OWiSM_HandleCommand(inDialog, (WMtWindow*)inParam2, inParam1);
		break;
		
		case WMcMessage_MenuCommand:
			OWiSM_HandleMenuCommand(
				inDialog,
				(WMtWindow*)inParam2,
				UUmLowWord(inParam1));
		break;
		
		case WMcMessage_DrawItem:
			OWiSM_HandleDrawItem(inDialog, (WMtDrawItem*)inParam2);
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ----------------------------------------------------------------------
UUtError
OWrSM_Display(
	OWtSMType					inType)
{
	UUtError					error;
	WMtDialogID					dialog_id;
	
	switch (inType)
	{
		case OWcSMType_Ambient:		dialog_id = OWcDialog_Ambient_Sound_Manager; break;
		case OWcSMType_Group:		dialog_id = OWcDialog_Sound_Group_Manager; break;
		case OWcSMType_Impulse:		dialog_id = OWcDialog_Impulse_Sound_Manager; break;
	}
	
	error =
		WMrDialog_ModalBegin(
			dialog_id,
			NULL,
			OWiSM_Callback,
			inType,
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
OWiSS_EnableButtons(
	WMtDialog					*inDialog)
{
	WMtWindow					*button;
	WMtWindow					*listbox;
	UUtUns32					item_data;
	
	button = WMrDialog_GetItemByID(inDialog, OWcSS_Btn_Select);
	WMrWindow_SetEnabled(button, UUcFalse);
	
	listbox = WMrDialog_GetItemByID(inDialog, OWcSS_LB_Items);
	item_data = WMrListBox_GetItemData(listbox, (UUtUns32)(-1));
	if (item_data != 0)
	{
		WMrWindow_SetEnabled(button, UUcFalse);
	}
	else
	{
		WMrWindow_SetEnabled(button, UUcTrue);
	}
}

// ----------------------------------------------------------------------
static BFtFileRef*
OWiSS_GetDirectoryRef(
	WMtDialog					*inDialog)
{
	UUtError					error;
	BFtFileRef					*out_ref;
	OWtSelectData				*sd;
	
	sd = (OWtSelectData*)WMrDialog_GetUserData(inDialog);
	if (sd->dir_ref == NULL) { return NULL; }
	
	error = BFrFileRef_Duplicate(sd->dir_ref, &out_ref);
	if (error != UUcError_None) { return NULL; }
	
	return out_ref;
}

// ----------------------------------------------------------------------
static const char*
OWiSS_GetExtension(
	WMtDialog					*inDialog)
{
	OWtSelectData				*sd;

	sd = (OWtSelectData*)WMrDialog_GetUserData(inDialog);
	
	return sd->extension;
}

// ----------------------------------------------------------------------
static void
OWiSS_SetDirectoryRef(
	WMtDialog					*inDialog,
	BFtFileRef					*inDirRef)
{
	UUtError					error;
	BFtFileRef					*old_ref;
	BFtFileRef					*copy_ref;
	OWtSelectData				*sd;
	
	UUrMemory_Block_VerifyList();
	
	// get the select data
	sd = (OWtSelectData*)WMrDialog_GetUserData(inDialog);
	UUmAssert(sd);
	
	// delete the old ref
	old_ref = sd->dir_ref;
	if (old_ref != NULL)
	{
		BFrFileRef_Dispose(old_ref);
		old_ref = NULL;
	}
	
	if (inDirRef != NULL)
	{
		error = BFrFileRef_Duplicate(inDirRef, &copy_ref);
		if (error != UUcError_None) { copy_ref = NULL; } 
	}
	else
	{
		copy_ref = NULL;
	}
	
	// set the new ref
	sd->dir_ref = copy_ref;
	
	error = OWiSetPopup(WMrDialog_GetItemByID(inDialog, OWcSM_PM_Category), copy_ref);

	UUrMemory_Block_VerifyList();
}

// ----------------------------------------------------------------------
static void
OWiSS_SetSelection(
	WMtDialog					*inDialog)
{
	WMtWindow					*listbox;
	char						name[BFcMaxFileNameLength];
	OWtSelectData				*sd;
	
	// get the select data
	sd = (OWtSelectData*)WMrDialog_GetUserData(inDialog);
	
	// get the name of the selected item
	listbox = WMrDialog_GetItemByID(inDialog, OWcSS_LB_Items);
	WMrListBox_GetText(listbox, name, (UUtUns32)(-1));
	
	// display the selected item
	switch (sd->type)
	{
		case OWcSMType_Ambient:
			sd->u.ambient = OSrAmbient_GetByName(name);
		break;
		
		case OWcSMType_Group:
			sd->u.group = OSrGroup_GetByName(name);
		break;
		
		case OWcSMType_Impulse:
			sd->u.impulse = OSrImpulse_GetByName(name);
		break;
	}
}

// ----------------------------------------------------------------------
static void
OWiSS_Category_Open(
	WMtDialog					*inDialog,
	WMtWindow					*inListBox)
{
	UUtError					error;
	BFtFileRef					*parent_dir_ref;
	BFtFileRef					*open_dir_ref;
	char						name[BFcMaxFileNameLength];
	
	UUrMemory_Block_VerifyList();
	
	parent_dir_ref = NULL;
	open_dir_ref = NULL;
	name[0] = '\0';
	
	// get the parent dir ref
	parent_dir_ref = OWiSS_GetDirectoryRef(inDialog);
	if (parent_dir_ref == NULL) { return; }
	
	// make a dir ref for the directory being opened
	WMrListBox_GetText(inListBox, name, (UUtUns32)(-1));
	error = BFrFileRef_DuplicateAndAppendName(parent_dir_ref, name, &open_dir_ref);
	if (error != UUcError_None) { goto cleanup; }
	
	// fill in the listbox
	error = OWiListDirectory(inListBox, open_dir_ref, OWiSS_GetExtension(inDialog));
	if (error != UUcError_None) { goto cleanup; }
	
	OWiSS_SetDirectoryRef(inDialog, open_dir_ref);
	
cleanup:
	if (parent_dir_ref != NULL)
	{
		BFrFileRef_Dispose(parent_dir_ref);
		parent_dir_ref = NULL;
	}
	
	if (open_dir_ref != NULL)
	{
		BFrFileRef_Dispose(open_dir_ref);
		open_dir_ref = NULL;
	}

	UUrMemory_Block_VerifyList();
}

// ----------------------------------------------------------------------
static void
OWiSS_InitDialog(
	WMtDialog					*inDialog)
{
	UUtError					error;
	OWtSelectData				*sd;
	BFtFileRef					*dir_ref;
	WMtWindow					*listbox;
	
	// get the select data
	sd = (OWtSelectData*)WMrDialog_GetUserData(inDialog);
	
	// get the directory ref
	if (OWgSelectSound_DirRef == NULL)
	{
		error = OSrGetSoundBinaryDirectory(&dir_ref);
		if (error != UUcError_None) { goto cleanup; }
	}
	else
	{
		error = BFrFileRef_Duplicate(OWgSelectSound_DirRef, &dir_ref);
		if (error != UUcError_None) { goto cleanup; }
	}
	
	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcSS_LB_Items);
	if (listbox == NULL) { goto cleanup; }
	
	// fill in the listbox
	error = OWiListDirectory(listbox, dir_ref, sd->extension);
	if (error != UUcError_None) { goto cleanup; }

	// set the directory ref
	OWiSS_SetDirectoryRef(inDialog, dir_ref);
	
	BFrFileRef_Dispose(dir_ref);
	dir_ref = NULL;
	
	return;
	
cleanup:
	if (dir_ref)
	{
		BFrFileRef_Dispose(dir_ref);
		dir_ref = NULL;
	}
	
	WMrDialog_ModalEnd(inDialog, OWcSS_Btn_Cancel);
}

// ----------------------------------------------------------------------
static void
OWiSS_HandleCommand(
	WMtDialog					*inDialog,
	WMtWindow					*inControl,
	UUtUns32					inParam)
{
	UUtUns16					control_id;
	UUtUns16					command_type;
	OWtSelectData				*sd;
	
	sd = (OWtSelectData*)WMrDialog_GetUserData(inDialog);
	
	control_id = UUmLowWord(inParam);
	command_type = UUmHighWord(inParam);
	
	switch (control_id)
	{
		case OWcSS_LB_Items:
			if (command_type == LBcNotify_SelectionChanged)
			{
				OWiSS_EnableButtons(inDialog);
			}
			else if (command_type == WMcNotify_DoubleClick)
			{
				UUtUns32				item_data;
				
				item_data = WMrListBox_GetItemData(inControl, (UUtUns32)(-1));
				if (item_data != 0)
				{
					// open the category
					OWiSS_Category_Open(inDialog, inControl);
				}
				else
				{
					// select the item
					OWiSS_SetSelection(inDialog);
					WMrDialog_ModalEnd(inDialog, OWcSS_Btn_Select);
				}
			}
		break;
		
		case OWcSS_Btn_None:
			if (command_type != WMcNotify_Click) { break; }
			WMrDialog_ModalEnd(inDialog, OWcSS_Btn_None);
		break;
		
		case OWcSS_Btn_New:
			if (command_type != WMcNotify_Click) { break; }
			OWrSM_Display(sd->type);
			OWiListDirectory(
				WMrDialog_GetItemByID(inDialog, OWcSS_LB_Items),
				sd->dir_ref,
				sd->extension);
		break;
		
		case OWcSS_Btn_Cancel:
			if (command_type != WMcNotify_Click) { break; }
			WMrDialog_ModalEnd(inDialog, OWcSS_Btn_Cancel);
		break;
		
		case OWcSS_Btn_Select:
			if (command_type != WMcNotify_Click) { break; }
			OWiSS_SetSelection(inDialog);
			WMrDialog_ModalEnd(inDialog, OWcSS_Btn_Select);
		break;
	}
}

// ----------------------------------------------------------------------
static void
OWiSS_HandleDestroy(
	WMtDialog					*inDialog)
{
	OWtSelectData				*sd;
	
	sd = (OWtSelectData*)WMrDialog_GetUserData(inDialog);
	if (sd)
	{
		if (sd->dir_ref)
		{
			if (OWgSelectSound_DirRef)
			{
				BFrFileRef_Dispose(OWgSelectSound_DirRef);
				OWgSelectSound_DirRef = NULL;
			}
			
			OWgSelectSound_DirRef = sd->dir_ref;
			sd->dir_ref = NULL;
		}
	}
}

// ----------------------------------------------------------------------
static void
OWiSS_HandleMenuCommand(
	WMtDialog					*inDialog,
	WMtWindow					*inPopupMenu,
	UUtUns16					inItemID)
{
	UUtUns32					i;
	BFtFileRef					*dir_ref;
	UUtError					error;
	
	// if inItemID == 0 then the popup menu is still on the same item
	// so no updating needs to be done
	if (inItemID == 0) { return; }
	
	// get the dir ref of the current item in the popup menu
	dir_ref = OWiSS_GetDirectoryRef(inDialog);
	if (dir_ref == NULL) { return; }
	
	// go up the hierarchy of directories until the desired directory is found
	for (i = 0; i < (UUtUns32)inItemID; i++)
	{
		BFtFileRef					*parent_ref;
		
		error = BFrFileRef_GetParentDirectory(dir_ref, &parent_ref);
		if (error != UUcError_None) { return; }
		
		BFrFileRef_Dispose(dir_ref);
		dir_ref = parent_ref;
	}
	
	// fill in the listbox
	OWiListDirectory(
		WMrDialog_GetItemByID(inDialog, OWcSS_LB_Items),
		dir_ref,
		OWiSS_GetExtension(inDialog));
	
	OWiSS_SetDirectoryRef(inDialog, dir_ref);
	
	BFrFileRef_Dispose(dir_ref);
	dir_ref = NULL;
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
			OWiSS_HandleDestroy(inDialog);
		break;
		
		case WMcMessage_Command:
			OWiSS_HandleCommand(inDialog, (WMtWindow*)inParam2, inParam1);
		break;
		
		case WMcMessage_MenuCommand:
			OWiSS_HandleMenuCommand(inDialog, (WMtWindow*)inParam2, UUmLowWord(inParam1));
		break;
		
		case WMcMessage_DrawItem:
			OWiSM_HandleDrawItem(inDialog, (WMtDrawItem*)inParam2);
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ----------------------------------------------------------------------
OWtSelectResult
OWrSelect_AmbientSound(
	SStAmbient					**outAmbient)
{
	UUtError					error;
	OWtSelectData				sd;
	UUtUns32					message;
	OWtSelectResult				result;
	
	sd.type = OWcSMType_Ambient;
	sd.dir_ref = NULL;
	sd.extension = OScAmbientSuffix;
	sd.u.ambient = NULL;
	
	result = OWcSelectResult_Cancel;
	
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Select_Sound,
			NULL,
			OWiSS_Callback,
			(UUtUns32)&sd,
			&message);
	if (error != UUcError_None) { return result; }
	
	if (message == OWcSS_Btn_Select)
	{
		*outAmbient = sd.u.ambient;
		result = OWcSelectResult_Select;
	}
	else if (message == OWcSS_Btn_None)
	{
		*outAmbient = NULL;
		result = OWcSelectResult_None;
	}
	else if (message == OWcSS_Btn_Cancel)
	{
		*outAmbient = NULL;
		result = OWcSelectResult_Cancel;
	}
	
	return result;
}

// ----------------------------------------------------------------------
OWtSelectResult
OWrSelect_SoundGroup(
	SStGroup					**outGroup)
{
	UUtError					error;
	OWtSelectData				sd;
	UUtUns32					message;
	OWtSelectResult				result;
	
	sd.type = OWcSMType_Group;
	sd.dir_ref = NULL;
	sd.extension = OScGroupSuffix;
	sd.u.group = NULL;
	
	result = OWcSelectResult_Cancel;

	error =
		WMrDialog_ModalBegin(
			OWcDialog_Select_Sound,
			NULL,
			OWiSS_Callback,
			(UUtUns32)&sd,
			&message);
	if (error != UUcError_None) { return result; }
	
	if (message == OWcSS_Btn_Select)
	{
		*outGroup = sd.u.group;
		result = OWcSelectResult_Select;
	}
	else if (message == OWcSS_Btn_None)
	{
		*outGroup = NULL;
		result = OWcSelectResult_None;
	}
	else if (message == OWcSS_Btn_Cancel)
	{
		*outGroup = NULL;
		result = OWcSelectResult_Cancel;
	}
	
	return result;
}

// ----------------------------------------------------------------------
OWtSelectResult
OWrSelect_ImpulseSound(
	SStImpulse					**outImpulse)
{
	UUtError					error;
	OWtSelectData				sd;
	UUtUns32					message;
	OWtSelectResult				result;
	
	sd.type = OWcSMType_Impulse;
	sd.dir_ref = NULL;
	sd.extension = OScImpulseSuffix;
	sd.u.impulse = NULL;
	
	result = OWcSelectResult_Cancel;

	error =
		WMrDialog_ModalBegin(
			OWcDialog_Select_Sound,
			NULL,
			OWiSS_Callback,
			(UUtUns32)&sd,
			&message);
	if (error != UUcError_None) { return result; }
	
	if (message == OWcSS_Btn_Select)
	{
		*outImpulse = sd.u.impulse;
		result = OWcSelectResult_Select;
	}
	else if (message == OWcSS_Btn_None)
	{
		*outImpulse = NULL;
		result = OWcSelectResult_None;
	}
	else if (message == OWcSS_Btn_Cancel)
	{
		*outImpulse = NULL;
		result = OWcSelectResult_Cancel;
	}
	
	return result;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
OWiViewAnimation_InitDialog(
	WMtDialog					*inDialog)
{
	OWtSA						*sa;
	UUtUns32					num_anims;
	UUtError					error;
	WMtWindow					*listbox;
	UUtUns32					i;
	UUtRect						rect;
	
	// set the window's position to the left hand side of the screen
	WMrWindow_GetRect(inDialog, &rect);
	WMrWindow_SetLocation(inDialog, 0, rect.top);
	
	// get a pointer to the user data
	sa = (OWtSA*)UUrMemory_Block_NewClear(sizeof(OWtSA));
	UUmError_ReturnOnNull(sa);
	WMrDialog_SetUserData(inDialog, (UUtUns32)sa);
	
	sa->insert_from_index = 0;
	
	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcSA_LB_Animations);
	
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
	
	// add lines to the dialog
	for (i = 0; i < sa->num_animations; i++)
	{
		WMrMessage_Send(listbox, LBcMessage_AddString, 0, 0);
	}
	WMrMessage_Send(listbox, LBcMessage_SetSelection, (UUtUns32)UUcFalse, 0);
	
	// set the focus to the listbox
	WMrWindow_SetFocus(listbox);
	
	// disable the play button if there is no character
	if (ONrGameState_GetPlayerCharacter() == NULL)
	{
		WMtWindow					*button;

		button = WMrDialog_GetItemByID(inDialog, OWcVA_Btn_Play);
		WMrWindow_SetEnabled(button, UUcFalse);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OWiViewAnimation_HandleDestroy(
	WMtDialog					*inDialog)
{
	OWtSA						*sa;
	
	sa = (OWtSA*)WMrDialog_GetUserData(inDialog);
	if (sa != NULL)
	{
		if (sa->animation_list != NULL)
		{
			UUrMemory_Block_Delete(sa->animation_list);
			sa->animation_list = NULL;
		}
		UUrMemory_Block_Delete(sa);
		sa = NULL;
	}
	
	OWgViewAnimation = NULL;
}

// ----------------------------------------------------------------------
static void
OWiViewAnimation_HandlePlay(
	WMtDialog					*inDialog,
	OWtSA						*inSA)
{
	WMtWindow					*listbox;
	UUtUns32					result;
	TRtAnimation				*animation;
	ONtCharacter				*player;
	ONtActiveCharacter			*active_character;
	
	// get the index of tha animation from the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcSA_LB_Animations);
	result = WMrMessage_Send(listbox, LBcMessage_GetSelection, 0, 0);
	if (result == LBcError) { return; }
	
	// make sure the animation exists
	animation = inSA->animation_list[result];
	if (animation == NULL) { return; }
	
	// get the player's character
	player = ONrGameState_GetPlayerCharacter();
	if (player == NULL) { return; }
	
	active_character = ONrForceActiveCharacter(player);
	if (active_character == NULL) { return; }
	
	// play the animation
	if (1)
	{
		ONrCharacter_SetAnimation_External(
			player,
			active_character->curFromState,
			animation,
			-1);
		player->flags |= ONcCharacterFlag_ChrAnimate;
		active_character->animationLockFrames = 0;
	}
	else
	{
/*		ONtOverlay					overlay;
		
		overlay.animation = animation;
		overlay.frame = 0;
		overlay.flags = 0;
		
		ONrOverlay_Set(
			&overlay,
			player->animation,
			0);*/
	}
}

// ----------------------------------------------------------------------
static void
OWiViewAnimation_HandleCommand(
	WMtDialog					*inDialog,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	OWtSA						*sa;
	
	sa = (OWtSA*)WMrDialog_GetUserData(inDialog);
	
	switch(UUmLowWord(inParam1))
	{
		case OWcVA_LB_Animations:
			if (UUmHighWord(inParam1) == WMcNotify_DoubleClick)
			{
				OWiViewAnimation_HandlePlay(inDialog, sa);
			}
		break;

		case OWcVA_Btn_Play:
			OWiViewAnimation_HandlePlay(inDialog, sa);
		break;
		
		case OWcVA_Btn_Close:
			WMrWindow_Delete(inDialog);
		break;
	}
}

// ----------------------------------------------------------------------
static void
OWiViewAnimation_HandleDrawItem(
	WMtDialog					*inDialog,
	WMtDrawItem					*inDrawItem)
{
	OWtSA						*sa;
	IMtPoint2D					dest;
	UUtInt16					line_width;
	UUtInt16					line_height;
	TRtAnimation				*animation;

	sa = (OWtSA*)WMrDialog_GetUserData(inDialog);

	// set the dest
	dest.x = inDrawItem->rect.left;
	dest.y = inDrawItem->rect.top;
	
	// calc the width and height of the line
	line_width = inDrawItem->rect.right - inDrawItem->rect.left;
	line_height = inDrawItem->rect.bottom - inDrawItem->rect.top;
	
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
OWiViewAnimation_Callback(
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
			error = OWiViewAnimation_InitDialog(inDialog);
			if (error != UUcError_None)
			{
				WMrWindow_Delete(inDialog);
			}
		break;
		
		case WMcMessage_Destroy:
			OWiViewAnimation_HandleDestroy(inDialog);
		break;
		
		case WMcMessage_Command:
			OWiViewAnimation_HandleCommand(inDialog, inParam1, inParam2);
		break;
		
		case WMcMessage_DrawItem:
			OWiViewAnimation_HandleDrawItem(inDialog, (WMtDrawItem*)inParam2);
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ----------------------------------------------------------------------
void
OWrViewAnimation_Display(
	void)
{
	UUtError					error;
	
	if (OWgViewAnimation != NULL) { return; }
	
	error =
		WMrDialog_Create(
			OWcDialog_View_Animation,
			NULL,
			OWiViewAnimation_Callback,
			0,
			&OWgViewAnimation);
	if (error != UUcError_None) { OWgViewAnimation = NULL; }
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
	UUtUns32					i;
	UUtRect						rect;
	
	// set the window's position to the left hand side of the screen
	WMrWindow_GetRect(inDialog, &rect);
	WMrWindow_SetLocation(inDialog, 0, rect.top);
	
	// get a pointer to the user data
	sa = (OWtSA*)WMrDialog_GetUserData(inDialog);
	sa->insert_from_index = 0;
	
	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcSA_LB_Animations);
	
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
	
	// add lines to the dialog
	for (i = 0; i < sa->num_animations; i++)
	{
		WMrMessage_Send(listbox, LBcMessage_AddString, 0, 0);
	}
	WMrMessage_Send(listbox, LBcMessage_SetSelection, (UUtUns32)UUcFalse, 0);
	
	// set the focus to the listbox
	WMrWindow_SetFocus(listbox);
	
	return UUcError_None;
}

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
OWiSelectAnimation_HandleDrawItem(
	WMtDialog					*inDialog,
	WMtDrawItem					*inDrawItem)
{
	OWtSA						*sa;
	IMtPoint2D					dest;
	UUtInt16					line_width;
	UUtInt16					line_height;
	TRtAnimation				*animation;

	sa = (OWtSA*)WMrDialog_GetUserData(inDialog);

	// set the dest
	dest.x = inDrawItem->rect.left;
	dest.y = inDrawItem->rect.top;
	
	// calc the width and height of the line
	line_width = inDrawItem->rect.right - inDrawItem->rect.left;
	line_height = inDrawItem->rect.bottom - inDrawItem->rect.top;
	
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
OWiStAP_HandleSetAnimation(
	WMtDialog					*inDialog,
	OWtStAP						*inProperties)
{
	UUtError			error;
	OWtSA				sa;
	UUtUns32			message;
	
	sa.animation = inProperties->animation;
	
	// select an animation
	WMrWindow_SetVisible(inDialog, UUcFalse);
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Select_Animation,
			inDialog,
			OWiSelectAnimation_Callback,
			(UUtUns32)&sa,
			&message);
	WMrWindow_SetVisible(inDialog, UUcTrue);
	if (error != UUcError_None) { return; }
	
	if (message == OWcSA_Btn_None)
	{
		inProperties->animation = NULL;
	}
	else if (message == OWcSA_Btn_Select)
	{
		inProperties->animation = sa.animation;
		if (inProperties->animation == NULL)
		{
			UUrMemory_Clear(inProperties->anim_name, ONcMaxVariantNameLength);
		}
		else
		{
			UUrString_Copy(
				inProperties->anim_name,
				TMrInstance_GetInstanceName(inProperties->animation),
				ONcMaxVariantNameLength);
		}
	}
	
	if (inProperties->animation)
	{
		WMtWindow					*text;
		TRtAnimTime					duration;
		char						string[128];
		
		// set the animation name
		text = WMrDialog_GetItemByID(inDialog, OWcStAP_Txt_AnimName);
		WMrText_SetShade(text, IMcShade_Black);
		WMrWindow_SetTitle(
			text,
			TMrInstance_GetInstanceName(inProperties->animation),
			OScMaxAnimNameLength);

		// set the number of frames
		text = WMrDialog_GetItemByID(inDialog, OWcStAP_Txt_Duration);
		WMrText_SetShade(text, IMcShade_Black);
		duration = TRrAnimation_GetDuration(inProperties->animation);
		sprintf(string, "%d", duration);
		WMrWindow_SetTitle(
			text,
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
	WMtWindow					*text;
	UUtRect						rect;
	
	properties = (OWtStAP*)WMrDialog_GetUserData(inDialog);
	if (properties == NULL)
	{
		WMrDialog_ModalEnd(inDialog, OWcStAP_Btn_Cancel);
		return;
	}
	
	// set the window's position to the left hand side of the screen
	WMrWindow_GetRect(inDialog, &rect);
	WMrWindow_SetLocation(inDialog, 0, rect.top);

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
		const char					*animation_name;
		IMtShade					shade;
		
		if (properties->animation != NULL)
		{
			animation_name = TMrInstance_GetInstanceName(properties->animation);
			duration = TRrAnimation_GetDuration(properties->animation);
			shade = IMcShade_Black;
		}
		else
		{
			animation_name = properties->anim_name;
			duration = 0;
			shade = IMcShade_Red;
		}
		
		// set the animation name
		text = WMrDialog_GetItemByID(inDialog, OWcStAP_Txt_AnimName);
		WMrText_SetShade(text, shade);
		WMrWindow_SetTitle(
			text,
			animation_name,
			OScMaxAnimNameLength);

		// set the number of frames
		sprintf(string, "%d", duration);
		text = WMrDialog_GetItemByID(inDialog, OWcStAP_Txt_Duration);
		WMrText_SetShade(text, shade);
		WMrWindow_SetTitle(
			text,
			string,
			OScMaxAnimNameLength);
	}
	else
	{
		// set the animation name
		text = WMrDialog_GetItemByID(inDialog, OWcStAP_Txt_AnimName);
		WMrWindow_SetTitle(
			text,
			"",
			OScMaxAnimNameLength);

		// set the number of frames
		text = WMrDialog_GetItemByID(inDialog, OWcStAP_Txt_Duration);
		WMrWindow_SetTitle(
			text,
			"0",
			OScMaxAnimNameLength);
	}
	
	// set the impulse text field
	text = WMrDialog_GetItemByID(inDialog, OWcStAP_Txt_SoundName);
	WMrWindow_SetTitle(text, properties->impulse_name, SScMaxNameLength);
	if (properties->impulse == NULL) { WMrText_SetShade(text, IMcShade_Red); }
	else { WMrText_SetShade(text, IMcShade_Black); }
		
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
	
	properties = (OWtStAP*)WMrDialog_GetUserData(inDialog);
	
	switch (UUmLowWord(inParam1))
	{
		case OWcStAP_Btn_SetAnim:
			OWiStAP_HandleSetAnimation(inDialog, properties);
		break;
		
		case OWcStAP_Btn_SetSound:
		{
			SStImpulse			*impulse;
			OWtSelectResult		result;
			WMtWindow			*text;
			
			result = OWrSelect_ImpulseSound(&impulse);
			if (result == OWcSelectResult_Cancel) { break; }
			
			properties->impulse = impulse;
			if (impulse == NULL)
			{
				UUrMemory_Clear(properties->impulse_name, SScMaxNameLength);
			}
			else
			{
				UUrString_Copy(
					properties->impulse_name,
					impulse->impulse_name,
					SScMaxNameLength);
			}
			
			text = WMrDialog_GetItemByID(inDialog, OWcStAP_Txt_SoundName);
			WMrWindow_SetTitle(
				text,
				properties->impulse_name,
				SScMaxNameLength);
			if (properties->impulse == NULL) { WMrText_SetShade(text, IMcShade_Red); }
			else { WMrText_SetShade(text, IMcShade_Black); }
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
	UUtUns32					result;
	
	// ask before deleting
	result =
		WMrDialog_MessageBox(
			inDialog,
			"Confirm",
			"Are you sure you want to delete the Animation Sound?",
			WMcMessageBoxStyle_Yes_No);
	if (result == WMcDialogItem_No) { return; }
	
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
	
	// set the listener position
	MUmVector_Set(position, 0.0f, 0.0f, 0.0f);
	MUmVector_Set(facing, 1.0f, 0.0f, 1.0f);
	SSrListener_SetPosition(&position, &facing);
	
	// play the sound
	MUmVector_Set(position, 5.0f, 0.0f, 5.0f);
	MUmVector_Set(velocity, 0.0f, 0.0f, 0.0f);
	SSrImpulse_Play(sound_animation->impulse, &position, &facing, &velocity, NULL);
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
	properties.impulse = sound_animation->impulse;
	properties.frame = sound_animation->frame;
	properties.animation = sound_animation->animation;
	if (sound_animation->animation == NULL)
	{
		UUrString_Copy(
			properties.anim_name,
			sound_animation->anim_name,
			OScMaxAnimNameLength);
	}
	else
	{
		UUrString_Copy(
			properties.anim_name,
			TMrInstance_GetInstanceName(sound_animation->animation),
			OScMaxAnimNameLength);
	}
	UUrString_Copy(
		properties.variant_name,
		OSrVariant_GetName(variant),
		ONcMaxVariantNameLength);
	UUrString_Copy(
		properties.impulse_name,
		sound_animation->impulse_name,
		SScMaxNameLength);
	
	// edit the properties
	WMrWindow_SetVisible(inDialog, UUcFalse);
	error = 
		WMrDialog_ModalBegin(
			OWcDialog_Sound_to_Anim_Prop,
			inDialog,
			OWiStAP_Callback,
			(UUtUns32)&properties,
			&message);
	WMrWindow_SetVisible(inDialog, UUcTrue);
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
				properties.anim_name,
				properties.frame,
				properties.impulse_name);
		if (error != UUcError_None)
		{
			WMrDialog_MessageBox(
				inDialog,
				"Error",
				"Unable to update the sound to the variant.",
				WMcMessageBoxStyle_OK);
		}
		
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
	properties.impulse = NULL;
	properties.animation = NULL;
	properties.frame = 0;
	properties.variant_name[0] = '\0';
	properties.impulse_name[0] = '\0';
	
	WMrWindow_SetVisible(inDialog, UUcFalse);
	error = 
		WMrDialog_ModalBegin(
			OWcDialog_Sound_to_Anim_Prop,
			inDialog,
			OWiStAP_Callback,
			(UUtUns32)&properties,
			&message);
	WMrWindow_SetVisible(inDialog, UUcTrue);
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
				properties.impulse_name);
		if (error != UUcError_None)
		{
			WMrDialog_MessageBox(
				inDialog,
				"Error",
				"Unable to add the sound to the variant.",
				WMcMessageBoxStyle_OK);
		}
		
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
	UUtRect						rect;
	
	// set the window's position to the left hand side of the screen
	WMrWindow_GetRect(inDialog, &rect);
	WMrWindow_SetLocation(inDialog, 0, rect.top);
	
	// update the sound animation pointers
	OSrSA_UpdatePointers();

	// fill in the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcStA_LB_Data);
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
	OStAnimType					anim_type;
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
	
	if (inDrawItem->rect.top > 5)
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
			DCrText_SetShade(IMcShade_Red);
			DCrDraw_String(
				inDrawItem->draw_context,
				sound_animation->anim_name,
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
	DCrText_SetShade(IMcShade_Black);
	
	dest.x = 305;
	
	if (sound_animation->impulse == NULL)
	{
		DCrText_SetShade(IMcShade_Red);
	}
	
	DCrDraw_String(
		inDrawItem->draw_context,
		sound_animation->impulse_name,
		&inDrawItem->rect,
		&dest);
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

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static BFtFileRef*
OWiCD_GetDirectoryRef(
	WMtDialog					*inDialog)
{
	UUtError					error;
	BFtFileRef					*out_ref;
	OWtCreateDialogue			*cd;
	
	cd = (OWtCreateDialogue*)WMrDialog_GetUserData(inDialog);
	if (cd->dir_ref == NULL) { return NULL; }
	
	error = BFrFileRef_Duplicate(cd->dir_ref, &out_ref);
	if (error != UUcError_None) { return NULL; }
	
	return out_ref;
}

// ----------------------------------------------------------------------
static void
OWiCD_SetDirectoryRef(
	WMtDialog					*inDialog,
	BFtFileRef					*inDirRef)
{
	UUtError					error;
	BFtFileRef					*old_ref;
	BFtFileRef					*copy_ref;
	OWtCreateDialogue			*cd;
	
	// get the create data
	cd = (OWtCreateDialogue*)WMrDialog_GetUserData(inDialog);
	
	UUrMemory_Block_VerifyList();
	
	// delete the old ref
	old_ref = cd->dir_ref;
	if (old_ref != NULL)
	{
		BFrFileRef_Dispose(old_ref);
		old_ref = NULL;
	}
	
	if (inDirRef != NULL)
	{
		error = BFrFileRef_Duplicate(inDirRef, &copy_ref);
		if (error != UUcError_None) { copy_ref = NULL; }
	}
	else
	{
		copy_ref = NULL;
	}
	
	// set the new ref
	cd->dir_ref = copy_ref;
	
	error = OWiSetPopup(WMrDialog_GetItemByID(inDialog, OWcCD_PM_Category), copy_ref);
	
	UUrMemory_Block_VerifyList();
}

// ----------------------------------------------------------------------
static void
OWiCD_New(
	WMtDialog					*inDialog)
{
	UUtError					error;
	OWtCreateDialogue			*cd;
	OWtWS						ws;
	char						name[SScMaxNameLength];
	SStGroup					*group;
	SStAmbient					*ambient;
	BFtFileRef					*dir_ref;
	UUtUns32					index;
	UUtUns32					message;
	WMtWindow					*listbox;
	
	group = NULL;
	ambient = NULL;
	dir_ref = NULL;
	
	// get the create data
	cd = (OWtCreateDialogue*)WMrDialog_GetUserData(inDialog);
		
	UUrMemory_Clear(&ws, sizeof(OWtWS));
	ws.allow_categories = UUcFalse;
	
	// select the wav file to use
	error =
		WMrDialog_ModalBegin(
			OWcDialog_WAV_Select,
			inDialog,
			OWiWS_Callback,
			(UUtUns32)&ws,
			&message);
	if ((error != UUcError_None) || (message == OWcWS_Btn_Cancel)) { return; }
	
	// get the name of the sound data
	UUrString_Copy(name, SSrSoundData_GetName(ws.selected_sound_data), SScMaxNameLength);
	UUrString_StripExtension(name);
	
	// create the sound group
	error = OSrGroup_New(name, &group);
	if (error != UUcError_None) { goto cleanup; }
	
	error = SSrGroup_Permutation_New(group, ws.selected_sound_data, &index);
	if (error != UUcError_None) { goto cleanup; }
	
	// create the ambient sound
	error = OSrAmbient_New(name, &ambient);
	if (error != UUcError_None) { goto cleanup; }
	
	// set the base track 1
	ambient->base_track1 = group;
	UUrString_Copy(ambient->base_track1_name, name, SScMaxNameLength);
	
	// set the flags
	ambient->flags |= (SScAmbientFlag_PlayOnce | SScAmbientFlag_InterruptOnStop);
	
	// set the priority to highest
	ambient->priority = SScPriority2_Highest;
	
	// get the dir ref
	dir_ref = OWiCD_GetDirectoryRef(inDialog);
	if (dir_ref == NULL) { goto cleanup; }
	
	// save the sound group
	error = OSrGroup_Save(group, dir_ref);
	if (error != UUcError_None) { goto cleanup; }
	
	// save the ambient sound
	error = OSrAmbient_Save(ambient, dir_ref);
	if (error != UUcError_None) { goto cleanup; }
	
	// fill in the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcCD_LB_Items),
	OWiListDirectory(listbox, dir_ref, OScAmbientSuffix);
	
	// select the item that was just selected
	WMrListBox_SelectString(listbox, name);
	
	BFrFileRef_Dispose(dir_ref);
	dir_ref = NULL;
	
	return;
	
cleanup:
	if (group != NULL)
	{
		OSrGroup_Delete(name);
		group = NULL;
	}
	
	if (ambient != NULL)
	{
		OSrAmbient_Delete(name);
		ambient = NULL;
	}
	
	if (dir_ref != NULL)
	{
		BFrFileRef_Dispose(dir_ref);
		dir_ref = NULL;
	}
	
	WMrDialog_MessageBox(
		inDialog,
		"Error",
		"Unable to create the dialogue files",
		WMcMessageBoxStyle_OK);
}

// ----------------------------------------------------------------------
static void
OWiCD_Category_Open(
	WMtDialog					*inDialog,
	WMtWindow					*inListBox)
{
	UUtError					error;
	BFtFileRef					*parent_dir_ref;
	BFtFileRef					*open_dir_ref;
	char						name[BFcMaxFileNameLength];
	
	UUrMemory_Block_VerifyList();
	
	parent_dir_ref = NULL;
	open_dir_ref = NULL;
	name[0] = '\0';
	
	// get the parent dir ref
	parent_dir_ref = OWiCD_GetDirectoryRef(inDialog);
	if (parent_dir_ref == NULL) { return; }
	
	// make a dir ref for the directory being opened
	WMrListBox_GetText(inListBox, name, (UUtUns32)(-1));
	error = BFrFileRef_DuplicateAndAppendName(parent_dir_ref, name, &open_dir_ref);
	if (error != UUcError_None) { goto cleanup; }
	
	// fill in the listbox
	error = OWiListDirectory(inListBox, open_dir_ref, OScAmbientSuffix);
	if (error != UUcError_None) { goto cleanup; }
	
	OWiCD_SetDirectoryRef(inDialog, open_dir_ref);
	
cleanup:
	if (parent_dir_ref != NULL)
	{
		BFrFileRef_Dispose(parent_dir_ref);
		parent_dir_ref = NULL;
	}
	
	if (open_dir_ref != NULL)
	{
		BFrFileRef_Dispose(open_dir_ref);
		open_dir_ref = NULL;
	}

	UUrMemory_Block_VerifyList();
}

// ----------------------------------------------------------------------
static void
OWiCD_InitDialog(
	WMtDialog					*inDialog)
{
	UUtError					error;
	OWtCreateDialogue			*cd;
	BFtFileRef					*dir_ref;
	WMtWindow					*listbox;
	
	SSrSoundData_GetByName_StartCache();

	// get the create data
	cd = (OWtCreateDialogue*)WMrDialog_GetUserData(inDialog);
	if (cd == NULL) { goto cleanup; }
	
	// get the directory ref
	if (OWgCD_DirRef == NULL)
	{
		error = OSrGetSoundBinaryDirectory(&dir_ref);
		if (error != UUcError_None) { goto cleanup; }
	}
	else
	{
		error = BFrFileRef_Duplicate(OWgCD_DirRef, &dir_ref);
		if (error != UUcError_None) { goto cleanup; }
	}
	
	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcCD_LB_Items);
	if (listbox == NULL) { goto cleanup; }
	
	// fill in the listbox
	error = OWiListDirectory(listbox, dir_ref, OScAmbientSuffix);
	if (error != UUcError_None) { goto cleanup; }
	
	// save the directory ref
	OWiCD_SetDirectoryRef(inDialog, dir_ref);
	
	BFrFileRef_Dispose(dir_ref);
	dir_ref = NULL;
	
	return;
	
cleanup:
	if (dir_ref)
	{
		BFrFileRef_Dispose(dir_ref);
		dir_ref = NULL;
	}
	
	WMrDialog_ModalEnd(inDialog, 0);
}

// ----------------------------------------------------------------------
static void
OWiCD_HandleCommand(
	WMtDialog					*inDialog,
	WMtWindow					*inControl,
	UUtUns32					inParam)
{
	UUtUns16					control_id;
	UUtUns16					command_type;
	OWtCreateDialogue			*cd;
	
	cd = (OWtCreateDialogue*)WMrDialog_GetUserData(inDialog);
	
	control_id = UUmLowWord(inParam);
	command_type = UUmHighWord(inParam);
	
	switch (control_id)
	{
		case OWcCD_LB_Items:
			if (command_type == WMcNotify_DoubleClick)
			{
				UUtUns32					item_data;
				
				item_data = WMrListBox_GetItemData(inControl, (UUtUns32)(-1));
				if (item_data != 0)
				{
					OWiCD_Category_Open(inDialog, inControl);
				}
			}
		break;
		
		case OWcCD_Btn_New:
			if (command_type != WMcNotify_Click) { break; }
			OWiCD_New(inDialog);
		break;
		
		case WMcDialogItem_Cancel:
			if (command_type != WMcNotify_Click) { break; }
			WMrDialog_ModalEnd(inDialog, 0);
		break;
	}
}

// ----------------------------------------------------------------------
static void
OWiCD_HandleDestroy(
	WMtDialog					*inDialog)
{
	OWtCreateDialogue			*cd;
	
	cd = (OWtCreateDialogue*)WMrDialog_GetUserData(inDialog);
	if (cd)
	{
		if (cd->dir_ref)
		{
			if (OWgCD_DirRef)
			{
				BFrFileRef_Dispose(OWgCD_DirRef);
				OWgCD_DirRef = NULL;
			}
			
			OWgCD_DirRef = cd->dir_ref;
			cd->dir_ref = NULL;
		}
	}

	SSrSoundData_GetByName_StopCache();
}

// ----------------------------------------------------------------------
static void
OWiCD_HandleMenuCommand(
	WMtDialog					*inDialog,
	WMtWindow					*inPopupMenu,
	UUtUns16					inItemID)
{
	UUtUns32					i;
	BFtFileRef					*dir_ref;
	UUtError					error;
	
	// if inItemID == 0 then the popup menu is still on the same item
	// so no updating needs to be done
	if (inItemID == 0) { return; }
	
	// get the dir ref of the current item in the popup menu
	dir_ref = OWiCD_GetDirectoryRef(inDialog);
	if (dir_ref == NULL) { return; }
	
	// go up the hierarchy of directories until the desired directory is found
	for (i = 0; i < (UUtUns32)inItemID; i++)
	{
		BFtFileRef					*parent_ref;
		
		error = BFrFileRef_GetParentDirectory(dir_ref, &parent_ref);
		if (error != UUcError_None) { return; }
		
		BFrFileRef_Dispose(dir_ref);
		dir_ref = parent_ref;
	}
	
	// fill in the listbox
	OWiListDirectory(
		WMrDialog_GetItemByID(inDialog, OWcCD_LB_Items),
		dir_ref,
		OScAmbientSuffix);
	
	OWiCD_SetDirectoryRef(inDialog, dir_ref);
	
	BFrFileRef_Dispose(dir_ref);
	dir_ref = NULL;
}

// ----------------------------------------------------------------------
static UUtBool
OWiCD_Callback(
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
			OWiCD_InitDialog(inDialog);
		break;
		
		case WMcMessage_Destroy:
			OWiCD_HandleDestroy(inDialog);
		break;
		
		case WMcMessage_Command:
			OWiCD_HandleCommand(inDialog, (WMtWindow*)inParam2, inParam1);
		break;
		
		case WMcMessage_MenuCommand:
			OWiCD_HandleMenuCommand(inDialog, (WMtWindow*)inParam2, UUmLowWord(inParam1));
		break;

		case WMcMessage_DrawItem:
			OWiSM_HandleDrawItem(inDialog, (WMtDrawItem*)inParam2);
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ----------------------------------------------------------------------
UUtError
OWrCreateDialogue_Display(
	void)
{
	UUtError					error;
	OWtCreateDialogue			create_dialogue;
	
	UUrMemory_Clear(&create_dialogue, sizeof(OWtCreateDialogue));
	
	// display the create dialogue dialog
	error = 
		WMrDialog_ModalBegin(
			OWcDialog_Create_Dialogue,
			NULL,
			OWiCD_Callback,
			(UUtUns32)&create_dialogue,
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
static BFtFileRef*
OWiCI_GetDirectoryRef(
	WMtDialog					*inDialog)
{
	UUtError					error;
	BFtFileRef					*out_ref;
	OWtCreateImpulse			*ci;
	
	ci = (OWtCreateImpulse *)WMrDialog_GetUserData(inDialog);
	if (ci->dir_ref == NULL) { return NULL; }
	
	error = BFrFileRef_Duplicate(ci->dir_ref, &out_ref);
	if (error != UUcError_None) { return NULL; }
	
	return out_ref;
}

// ----------------------------------------------------------------------
static void
OWiCI_SetDirectoryRef(
	WMtDialog					*inDialog,
	BFtFileRef					*inDirRef)
{
	UUtError					error;
	BFtFileRef					*old_ref;
	BFtFileRef					*copy_ref;
	OWtCreateImpulse			*ci;
	
	// get the create data
	ci = (OWtCreateImpulse *)WMrDialog_GetUserData(inDialog);
	
	UUrMemory_Block_VerifyList();
	
	// delete the old ref
	old_ref = ci->dir_ref;
	if (old_ref != NULL)
	{
		BFrFileRef_Dispose(old_ref);
		old_ref = NULL;
	}
	
	if (inDirRef != NULL)
	{
		error = BFrFileRef_Duplicate(inDirRef, &copy_ref);
		if (error != UUcError_None) { copy_ref = NULL; }
	}
	else
	{
		copy_ref = NULL;
	}
	
	// set the new ref
	ci->dir_ref = copy_ref;
	
	error = OWiSetPopup(WMrDialog_GetItemByID(inDialog, OWcCI_PM_Category), copy_ref);
	
	UUrMemory_Block_VerifyList();
}

// ----------------------------------------------------------------------
static void
OWiCI_New(
	WMtDialog					*inDialog)
{
	UUtError					error;
	OWtCreateImpulse			*ci;
	OWtWS						ws;
	char						name[SScMaxNameLength];
	SStGroup					*group;
	SStImpulse					*impulse;
	BFtFileRef					*dir_ref;
	UUtUns32					index;
	UUtUns32					message;
	WMtWindow					*listbox;
	
	group = NULL;
	impulse = NULL;
	dir_ref = NULL;
	
	// get the create data
	ci = (OWtCreateImpulse*)WMrDialog_GetUserData(inDialog);
		
	UUrMemory_Clear(&ws, sizeof(OWtWS));
	ws.allow_categories = UUcTrue;

	// select the wav file to use
	error =
		WMrDialog_ModalBegin(
			OWcDialog_WAV_Select,
			inDialog,
			OWiWS_Callback,
			(UUtUns32)&ws,
			&message);
	if ((error != UUcError_None) || (message == OWcWS_Btn_Cancel)) { return; }

	if (ws.selected_sound_data != NULL) {
		// get the name of the sound data
		UUrString_Copy(name, SSrSoundData_GetName(ws.selected_sound_data), SScMaxNameLength);
		UUrString_StripExtension(name);

	} else if (ws.selected_category != NULL) {
		// get the name of the directory
		UUrString_Copy(name, BFrFileRef_GetLeafName(ws.selected_category), SScMaxNameLength);

	} else {
		// nothing selected
		goto cleanup;
	}
	
	// create the sound group
	error = OSrGroup_New(name, &group);
	if (error != UUcError_None) { goto cleanup; }
	
	if (ws.selected_sound_data != NULL) {
		// add a permutation for the sound data to the group
		error = SSrGroup_Permutation_New(group, ws.selected_sound_data, &index);
		if (error != UUcError_None) { goto cleanup; }

	} else if (ws.selected_category != NULL) {
		BFtFileIterator			*file_iterator;

		// search the directory for sound files and add them to the group
		error =
			BFrDirectory_FileIterator_New(
				ws.selected_category,
				NULL,
				NULL,
				&file_iterator);
		if (error != UUcError_None) { goto cleanup; }

		while (1)
		{
			BFtFileRef				ref;
			char					file_name[SScMaxNameLength];
			SStSoundData			*sound_data;
			
			// get the next ref
			error = BFrDirectory_FileIterator_Next(file_iterator, &ref);
			if (error != UUcError_None) { break; }

			// process the ref
			UUrString_Copy(file_name, BFrFileRef_GetLeafName(&ref), SScMaxNameLength);
			if (!UUrString_Substring(file_name, ".aif", SScMaxNameLength)) {
				continue;
			}
			UUrString_StripExtension(file_name);

			// get the SStSoundData* corresponding to this file
			sound_data = SSrSoundData_GetByName(file_name, UUcTrue);
			
			if (sound_data == NULL) {
				// create a new sound data
				error = SSrSoundData_New(&ref, &sound_data);
				if (error != UUcError_None) {
					continue;
				}
			}

			// add the sound data to the group
			error = SSrGroup_Permutation_New(group, sound_data, &index);
		}
	
		// delete the file iterator
		BFrDirectory_FileIterator_Delete(file_iterator);

	} else {
		UUmAssert(0);
	}

	// create the impulse sound
	error = OSrImpulse_New(name, &impulse);
	if (error != UUcError_None) { goto cleanup; }
	
	// set the sound group
	impulse->impulse_group = group;
	UUrString_Copy(impulse->impulse_group_name, name, SScMaxNameLength);
	
	// set up defaults
	impulse->min_volume_distance = 15.0f;
	impulse->max_volume_distance = 80.0f;

	// get the dir ref
	dir_ref = OWiCI_GetDirectoryRef(inDialog);
	if (dir_ref == NULL) { goto cleanup; }
	
	// save the sound group
	error = OSrGroup_Save(group, dir_ref);
	if (error != UUcError_None) { goto cleanup; }
	
	// save the impulse sound
	error = OSrImpulse_Save(impulse, dir_ref);
	if (error != UUcError_None) { goto cleanup; }
	
	// fill in the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcCI_LB_Items),
	OWiListDirectory(listbox, dir_ref, OScImpulseSuffix);
	
	// select the item that was just selected
	WMrListBox_SelectString(listbox, name);
	
	BFrFileRef_Dispose(dir_ref);
	dir_ref = NULL;
	
	return;
	
cleanup:
	if (group != NULL)
	{
		OSrGroup_Delete(name);
		group = NULL;
	}
	
	if (impulse != NULL)
	{
		OSrImpulse_Delete(name);
		impulse = NULL;
	}
	
	if (dir_ref != NULL)
	{
		BFrFileRef_Dispose(dir_ref);
		dir_ref = NULL;
	}
	
	WMrDialog_MessageBox(
		inDialog,
		"Error",
		"Unable to create the impulse sound",
		WMcMessageBoxStyle_OK);
}

// ----------------------------------------------------------------------
static void
OWiCI_Category_Open(
	WMtDialog					*inDialog,
	WMtWindow					*inListBox)
{
	UUtError					error;
	BFtFileRef					*parent_dir_ref;
	BFtFileRef					*open_dir_ref;
	char						name[BFcMaxFileNameLength];
	
	UUrMemory_Block_VerifyList();
	
	parent_dir_ref = NULL;
	open_dir_ref = NULL;
	name[0] = '\0';
	
	// get the parent dir ref
	parent_dir_ref = OWiCI_GetDirectoryRef(inDialog);
	if (parent_dir_ref == NULL) { return; }
	
	// make a dir ref for the directory being opened
	WMrListBox_GetText(inListBox, name, (UUtUns32)(-1));
	error = BFrFileRef_DuplicateAndAppendName(parent_dir_ref, name, &open_dir_ref);
	if (error != UUcError_None) { goto cleanup; }
	
	// fill in the listbox
	error = OWiListDirectory(inListBox, open_dir_ref, OScImpulseSuffix);
	if (error != UUcError_None) { goto cleanup; }
	
	OWiCI_SetDirectoryRef(inDialog, open_dir_ref);
	
cleanup:
	if (parent_dir_ref != NULL)
	{
		BFrFileRef_Dispose(parent_dir_ref);
		parent_dir_ref = NULL;
	}
	
	if (open_dir_ref != NULL)
	{
		BFrFileRef_Dispose(open_dir_ref);
		open_dir_ref = NULL;
	}

	UUrMemory_Block_VerifyList();
}

// ----------------------------------------------------------------------
static void
OWiCI_InitDialog(
	WMtDialog					*inDialog)
{
	UUtError					error;
	OWtCreateImpulse			*ci;
	BFtFileRef					*dir_ref;
	WMtWindow					*listbox;
	
	SSrSoundData_GetByName_StartCache();

	// get the create data
	ci = (OWtCreateImpulse *)WMrDialog_GetUserData(inDialog);
	if (ci == NULL) { goto cleanup; }
	
	// get the directory ref
	if (OWgCI_DirRef == NULL)
	{
		error = OSrGetSoundBinaryDirectory(&dir_ref);
		if (error != UUcError_None) { goto cleanup; }
	}
	else
	{
		error = BFrFileRef_Duplicate(OWgCI_DirRef, &dir_ref);
		if (error != UUcError_None) { goto cleanup; }
	}
	
	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcCI_LB_Items);
	if (listbox == NULL) { goto cleanup; }
	
	// fill in the listbox
	error = OWiListDirectory(listbox, dir_ref, OScImpulseSuffix);
	if (error != UUcError_None) { goto cleanup; }
	
	// save the directory ref
	OWiCI_SetDirectoryRef(inDialog, dir_ref);
	
	BFrFileRef_Dispose(dir_ref);
	dir_ref = NULL;
	
	return;
	
cleanup:

	if (dir_ref)
	{
		BFrFileRef_Dispose(dir_ref);
		dir_ref = NULL;
	}
	
	WMrDialog_ModalEnd(inDialog, 0);
}

// ----------------------------------------------------------------------
static void
OWiCI_HandleCommand(
	WMtDialog					*inDialog,
	WMtWindow					*inControl,
	UUtUns32					inParam)
{
	UUtUns16					control_id;
	UUtUns16					command_type;
	OWtCreateImpulse			*ci;
	
	ci = (OWtCreateImpulse *)WMrDialog_GetUserData(inDialog);
	
	control_id = UUmLowWord(inParam);
	command_type = UUmHighWord(inParam);
	
	switch (control_id)
	{
		case OWcCI_LB_Items:
			if (command_type == WMcNotify_DoubleClick)
			{
				UUtUns32					item_data;
				
				item_data = WMrListBox_GetItemData(inControl, (UUtUns32)(-1));
				if (item_data != 0)
				{
					OWiCI_Category_Open(inDialog, inControl);
				}
			}
		break;
		
		case OWcCI_Btn_New:
			if (command_type != WMcNotify_Click) { break; }
			OWiCI_New(inDialog);
		break;
		
		case WMcDialogItem_Cancel:
			if (command_type != WMcNotify_Click) { break; }
			WMrDialog_ModalEnd(inDialog, 0);
		break;
	}
}

// ----------------------------------------------------------------------
static void
OWiCI_HandleDestroy(
	WMtDialog					*inDialog)
{
	OWtCreateImpulse			*ci;
	
	ci = (OWtCreateImpulse *)WMrDialog_GetUserData(inDialog);
	if (ci)
	{
		if (ci->dir_ref)
		{
			if (OWgCI_DirRef)
			{
				BFrFileRef_Dispose(OWgCI_DirRef);
				OWgCI_DirRef = NULL;
			}
			
			OWgCI_DirRef = ci->dir_ref;
			ci->dir_ref = NULL;
		}
	}

	SSrSoundData_GetByName_StopCache();
}

// ----------------------------------------------------------------------
static void
OWiCI_HandleMenuCommand(
	WMtDialog					*inDialog,
	WMtWindow					*inPopupMenu,
	UUtUns16					inItemID)
{
	UUtUns32					i;
	BFtFileRef					*dir_ref;
	UUtError					error;
	
	// if inItemID == 0 then the popup menu is still on the same item
	// so no updating needs to be done
	if (inItemID == 0) { return; }
	
	// get the dir ref of the current item in the popup menu
	dir_ref = OWiCI_GetDirectoryRef(inDialog);
	if (dir_ref == NULL) { return; }
	
	// go up the hierarchy of directories until the desired directory is found
	for (i = 0; i < (UUtUns32)inItemID; i++)
	{
		BFtFileRef					*parent_ref;
		
		error = BFrFileRef_GetParentDirectory(dir_ref, &parent_ref);
		if (error != UUcError_None) { return; }
		
		BFrFileRef_Dispose(dir_ref);
		dir_ref = parent_ref;
	}
	
	// fill in the listbox
	OWiListDirectory(
		WMrDialog_GetItemByID(inDialog, OWcCI_LB_Items),
		dir_ref,
		OScImpulseSuffix);
	
	OWiCI_SetDirectoryRef(inDialog, dir_ref);
	
	BFrFileRef_Dispose(dir_ref);
	dir_ref = NULL;
}

// ----------------------------------------------------------------------
static UUtBool
OWiCI_Callback(
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
			OWiCI_InitDialog(inDialog);
		break;
		
		case WMcMessage_Destroy:
			OWiCI_HandleDestroy(inDialog);
		break;
		
		case WMcMessage_Command:
			OWiCI_HandleCommand(inDialog, (WMtWindow*)inParam2, inParam1);
		break;
		
		case WMcMessage_MenuCommand:
			OWiCI_HandleMenuCommand(inDialog, (WMtWindow*)inParam2, UUmLowWord(inParam1));
		break;

		case WMcMessage_DrawItem:
			OWiSM_HandleDrawItem(inDialog, (WMtDrawItem*)inParam2);
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ----------------------------------------------------------------------
UUtError
OWrCreateImpulse_Display(
	void)
{
	UUtError					error;
	OWtCreateImpulse			create_impulse;
	
	UUrMemory_Clear(&create_impulse, sizeof(OWtCreateImpulse));
	
	// display the create dialogue dialog
	error = 
		WMrDialog_ModalBegin(
			OWcDialog_Create_Impulse,
			NULL,
			OWiCI_Callback,
			(UUtUns32)&create_impulse,
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
static BFtFileRef*
OWiCG_GetDirectoryRef(
	WMtDialog					*inDialog)
{
	UUtError					error;
	BFtFileRef					*out_ref;
	OWtCreateGroup				*cg;
	
	cg = (OWtCreateGroup *)WMrDialog_GetUserData(inDialog);
	if (cg->dir_ref == NULL) { return NULL; }
	
	error = BFrFileRef_Duplicate(cg->dir_ref, &out_ref);
	if (error != UUcError_None) { return NULL; }
	
	return out_ref;
}

// ----------------------------------------------------------------------
static void
OWiCG_SetDirectoryRef(
	WMtDialog					*inDialog,
	BFtFileRef					*inDirRef)
{
	UUtError					error;
	BFtFileRef					*old_ref;
	BFtFileRef					*copy_ref;
	OWtCreateGroup				*cg;
	
	// get the create data
	cg = (OWtCreateGroup *)WMrDialog_GetUserData(inDialog);
	
	UUrMemory_Block_VerifyList();
	
	// delete the old ref
	old_ref = cg->dir_ref;
	if (old_ref != NULL)
	{
		BFrFileRef_Dispose(old_ref);
		old_ref = NULL;
	}
	
	if (inDirRef != NULL)
	{
		error = BFrFileRef_Duplicate(inDirRef, &copy_ref);
		if (error != UUcError_None) { copy_ref = NULL; }
	}
	else
	{
		copy_ref = NULL;
	}
	
	// set the new ref
	cg->dir_ref = copy_ref;
	
	error = OWiSetPopup(WMrDialog_GetItemByID(inDialog, OWcCG_PM_Category), copy_ref);
	
	UUrMemory_Block_VerifyList();
}

// ----------------------------------------------------------------------
static void
OWiCG_New(
	WMtDialog					*inDialog)
{
	UUtError					error;
	OWtCreateGroup				*cg;
	OWtWS						ws;
	char						name[SScMaxNameLength];
	SStGroup					*group;
	BFtFileRef					*dir_ref;
	UUtUns32					index;
	UUtUns32					message;
	WMtWindow					*listbox;
	
	group = NULL;
	dir_ref = NULL;
	
	// get the create data
	cg = (OWtCreateGroup *) WMrDialog_GetUserData(inDialog);
		
	UUrMemory_Clear(&ws, sizeof(OWtWS));
	ws.allow_categories = UUcTrue;

	// select the wav file to use
	error =
		WMrDialog_ModalBegin(
			OWcDialog_WAV_Select,
			inDialog,
			OWiWS_Callback,
			(UUtUns32)&ws,
			&message);
	if ((error != UUcError_None) || (message == OWcWS_Btn_Cancel)) { return; }

	if (ws.selected_sound_data != NULL) {
		// get the name of the sound data
		UUrString_Copy(name, SSrSoundData_GetName(ws.selected_sound_data), SScMaxNameLength);
		UUrString_StripExtension(name);

	} else if (ws.selected_category != NULL) {
		// get the name of the directory
		UUrString_Copy(name, BFrFileRef_GetLeafName(ws.selected_category), SScMaxNameLength);

	} else {
		// nothing selected
		goto cleanup;
	}
	
	// create the sound group
	error = OSrGroup_New(name, &group);
	if (error != UUcError_None) { goto cleanup; }
	
	if (ws.selected_sound_data != NULL) {
		// add a permutation for the sound data to the group
		error = SSrGroup_Permutation_New(group, ws.selected_sound_data, &index);
		if (error != UUcError_None) { goto cleanup; }

	} else if (ws.selected_category != NULL) {
		BFtFileIterator			*file_iterator;

		// search the directory for sound files and add them to the group
		error =
			BFrDirectory_FileIterator_New(
				ws.selected_category,
				NULL,
				NULL,
				&file_iterator);
		if (error != UUcError_None) { goto cleanup; }

		while (1)
		{
			BFtFileRef				ref;
			char					file_name[SScMaxNameLength];
			SStSoundData			*sound_data;
			
			// get the next ref
			error = BFrDirectory_FileIterator_Next(file_iterator, &ref);
			if (error != UUcError_None) { break; }

			// process the ref
			UUrString_Copy(file_name, BFrFileRef_GetLeafName(&ref), SScMaxNameLength);
			if (!UUrString_Substring(file_name, ".aif", SScMaxNameLength)) {
				continue;
			}
			UUrString_StripExtension(file_name);

			// get the SStSoundData* corresponding to this file
			sound_data = SSrSoundData_GetByName(file_name, UUcTrue);
			
			if (sound_data == NULL) {
				// create a new sound data
				error = SSrSoundData_New(&ref, &sound_data);
				if (error != UUcError_None) {
					continue;
				}
			}

			// add the sound data to the group
			error = SSrGroup_Permutation_New(group, sound_data, &index);
		}
	
		// delete the file iterator
		BFrDirectory_FileIterator_Delete(file_iterator);

	} else {
		UUmAssert(0);
	}

	// get the dir ref
	dir_ref = OWiCG_GetDirectoryRef(inDialog);
	if (dir_ref == NULL) { goto cleanup; }
	
	// save the sound group
	error = OSrGroup_Save(group, dir_ref);
	if (error != UUcError_None) { goto cleanup; }
	
	// fill in the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcCG_LB_Items),
	OWiListDirectory(listbox, dir_ref, OScImpulseSuffix);
	
	// select the item that was just selected
	WMrListBox_SelectString(listbox, name);
	
	BFrFileRef_Dispose(dir_ref);
	dir_ref = NULL;
	
	return;
	
cleanup:
	if (group != NULL)
	{
		OSrGroup_Delete(name);
		group = NULL;
	}
	
	if (dir_ref != NULL)
	{
		BFrFileRef_Dispose(dir_ref);
		dir_ref = NULL;
	}
	
	WMrDialog_MessageBox(
		inDialog,
		"Error",
		"Unable to create the sound group",
		WMcMessageBoxStyle_OK);
}

// ----------------------------------------------------------------------
static void
OWiCG_Category_Open(
	WMtDialog					*inDialog,
	WMtWindow					*inListBox)
{
	UUtError					error;
	BFtFileRef					*parent_dir_ref;
	BFtFileRef					*open_dir_ref;
	char						name[BFcMaxFileNameLength];
	
	UUrMemory_Block_VerifyList();
	
	parent_dir_ref = NULL;
	open_dir_ref = NULL;
	name[0] = '\0';
	
	// get the parent dir ref
	parent_dir_ref = OWiCG_GetDirectoryRef(inDialog);
	if (parent_dir_ref == NULL) { return; }
	
	// make a dir ref for the directory being opened
	WMrListBox_GetText(inListBox, name, (UUtUns32)(-1));
	error = BFrFileRef_DuplicateAndAppendName(parent_dir_ref, name, &open_dir_ref);
	if (error != UUcError_None) { goto cleanup; }
	
	// fill in the listbox
	error = OWiListDirectory(inListBox, open_dir_ref, OScImpulseSuffix);
	if (error != UUcError_None) { goto cleanup; }
	
	OWiCG_SetDirectoryRef(inDialog, open_dir_ref);
	
cleanup:
	if (parent_dir_ref != NULL)
	{
		BFrFileRef_Dispose(parent_dir_ref);
		parent_dir_ref = NULL;
	}
	
	if (open_dir_ref != NULL)
	{
		BFrFileRef_Dispose(open_dir_ref);
		open_dir_ref = NULL;
	}

	UUrMemory_Block_VerifyList();
}

// ----------------------------------------------------------------------
static void
OWiCG_InitDialog(
	WMtDialog					*inDialog)
{
	UUtError					error;
	OWtCreateGroup				*cg;
	BFtFileRef					*dir_ref;
	WMtWindow					*listbox;
	
	SSrSoundData_GetByName_StartCache();

	// get the create data
	cg = (OWtCreateGroup *)WMrDialog_GetUserData(inDialog);
	if (cg == NULL) { goto cleanup; }
	
	// get the directory ref
	if (OWgCG_DirRef == NULL)
	{
		error = OSrGetSoundBinaryDirectory(&dir_ref);
		if (error != UUcError_None) { goto cleanup; }
	}
	else
	{
		error = BFrFileRef_Duplicate(OWgCG_DirRef, &dir_ref);
		if (error != UUcError_None) { goto cleanup; }
	}
	
	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcCG_LB_Items);
	if (listbox == NULL) { goto cleanup; }
	
	// fill in the listbox
	error = OWiListDirectory(listbox, dir_ref, OScImpulseSuffix);
	if (error != UUcError_None) { goto cleanup; }
	
	// save the directory ref
	OWiCG_SetDirectoryRef(inDialog, dir_ref);
	
	BFrFileRef_Dispose(dir_ref);
	dir_ref = NULL;
	
	return;
	
cleanup:

	if (dir_ref)
	{
		BFrFileRef_Dispose(dir_ref);
		dir_ref = NULL;
	}
	
	WMrDialog_ModalEnd(inDialog, 0);
}

// ----------------------------------------------------------------------
static void
OWiCG_HandleCommand(
	WMtDialog					*inDialog,
	WMtWindow					*inControl,
	UUtUns32					inParam)
{
	UUtUns16					control_id;
	UUtUns16					command_type;
	OWtCreateGroup				*cg;
	
	cg = (OWtCreateGroup *)WMrDialog_GetUserData(inDialog);
	
	control_id = UUmLowWord(inParam);
	command_type = UUmHighWord(inParam);
	
	switch (control_id)
	{
		case OWcCG_LB_Items:
			if (command_type == WMcNotify_DoubleClick)
			{
				UUtUns32					item_data;
				
				item_data = WMrListBox_GetItemData(inControl, (UUtUns32)(-1));
				if (item_data != 0)
				{
					OWiCG_Category_Open(inDialog, inControl);
				}
			}
		break;
		
		case OWcCG_Btn_New:
			if (command_type != WMcNotify_Click) { break; }
			OWiCG_New(inDialog);
		break;
		
		case WMcDialogItem_Cancel:
			if (command_type != WMcNotify_Click) { break; }
			WMrDialog_ModalEnd(inDialog, 0);
		break;
	}
}

// ----------------------------------------------------------------------
static void
OWiCG_HandleDestroy(
	WMtDialog					*inDialog)
{
	OWtCreateGroup				*cg;
	
	cg = (OWtCreateGroup *)WMrDialog_GetUserData(inDialog);
	if (cg)
	{
		if (cg->dir_ref)
		{
			if (OWgCG_DirRef)
			{
				BFrFileRef_Dispose(OWgCG_DirRef);
				OWgCG_DirRef = NULL;
			}
			
			OWgCG_DirRef = cg->dir_ref;
			cg->dir_ref = NULL;
		}
	}

	SSrSoundData_GetByName_StopCache();
}

// ----------------------------------------------------------------------
static void
OWiCG_HandleMenuCommand(
	WMtDialog					*inDialog,
	WMtWindow					*inPopupMenu,
	UUtUns16					inItemID)
{
	UUtUns32					i;
	BFtFileRef					*dir_ref;
	UUtError					error;
	
	// if inItemID == 0 then the popup menu is still on the same item
	// so no updating needs to be done
	if (inItemID == 0) { return; }
	
	// get the dir ref of the current item in the popup menu
	dir_ref = OWiCG_GetDirectoryRef(inDialog);
	if (dir_ref == NULL) { return; }
	
	// go up the hierarchy of directories until the desired directory is found
	for (i = 0; i < (UUtUns32)inItemID; i++)
	{
		BFtFileRef					*parent_ref;
		
		error = BFrFileRef_GetParentDirectory(dir_ref, &parent_ref);
		if (error != UUcError_None) { return; }
		
		BFrFileRef_Dispose(dir_ref);
		dir_ref = parent_ref;
	}
	
	// fill in the listbox
	OWiListDirectory(
		WMrDialog_GetItemByID(inDialog, OWcCG_LB_Items),
		dir_ref,
		OScImpulseSuffix);
	
	OWiCG_SetDirectoryRef(inDialog, dir_ref);
	
	BFrFileRef_Dispose(dir_ref);
	dir_ref = NULL;
}

// ----------------------------------------------------------------------
static UUtBool
OWiCG_Callback(
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
			OWiCG_InitDialog(inDialog);
		break;
		
		case WMcMessage_Destroy:
			OWiCG_HandleDestroy(inDialog);
		break;
		
		case WMcMessage_Command:
			OWiCG_HandleCommand(inDialog, (WMtWindow*)inParam2, inParam1);
		break;
		
		case WMcMessage_MenuCommand:
			OWiCG_HandleMenuCommand(inDialog, (WMtWindow*)inParam2, UUmLowWord(inParam1));
		break;

		case WMcMessage_DrawItem:
			OWiSM_HandleDrawItem(inDialog, (WMtDrawItem*)inParam2);
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ----------------------------------------------------------------------
UUtError
OWrCreateGroup_Display(
	void)
{
	UUtError					error;
	OWtCreateGroup				create_group;
	
	UUrMemory_Clear(&create_group, sizeof(OWtCreateGroup));
	
	// display the create dialogue dialog
	error = 
		WMrDialog_ModalBegin(
			OWcDialog_Create_Group,
			NULL,
			OWiCG_Callback,
			(UUtUns32)&create_group,
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
static UUtError
OWiMinMax_Fixer(
	BFtFileRef					*inDirectoryRef,
	BFtFile						*inFile)
{
	UUtError					error;
	BFtFileIterator				*file_iterator;
	
	// go through all of the ambient sounds and fix the ones whose
	// max volume distance is less than the min volume distance
	error =
		BFrDirectory_FileIterator_New(
			inDirectoryRef,
			NULL,
			NULL,
			&file_iterator);
	UUmError_ReturnOnError(error);
	
	while (1)
	{
		BFtFileRef					ref;
		float						temp_distance;
		char						name[BFcMaxFileNameLength];
		char						out_string[1024];
		
		error = BFrDirectory_FileIterator_Next(file_iterator, &ref);
		if (error != UUcError_None) { break; }
		
		if (BFrFileRef_IsDirectory(&ref))
		{
			OWiMinMax_Fixer(&ref, inFile);
		}
		else if (UUrString_Compare_NoCase(OScAmbientSuffix, BFrFileRef_GetSuffixName(&ref)) == 0)
		{
			SStAmbient					*ambient;
			
			UUrString_Copy(name, BFrFileRef_GetLeafName(&ref), BFcMaxFileNameLength);
			UUrString_StripExtension(name);
			UUrString_MakeLowerCase(name, BFcMaxFileNameLength);
			
			ambient = OSrAmbient_GetByName(name);
			if ((ambient != NULL) &&
				(ambient->min_volume_distance < ambient->max_volume_distance))
			{
				if (BFrFileRef_IsLocked(&ref) == UUcTrue)
				{
					sprintf(
						out_string,
						"Ambient sound file %s needs to be modified, but the file is locked." UUmNL,
						BFrFileRef_GetLeafName(&ref));
					
					WMrDialog_MessageBox(
						NULL,
						"Error",
						out_string,
						WMcMessageBoxStyle_OK);
					
					sprintf(out_string, "File is locked, %s" UUmNL, BFrFileRef_GetLeafName(&ref));
					BFrFile_Write(inFile, strlen(out_string), out_string);
				}
				else
				{
					temp_distance = ambient->min_volume_distance;
					ambient->min_volume_distance = ambient->max_volume_distance;
					ambient->max_volume_distance = temp_distance;
					
					OSrAmbient_Save(ambient, inDirectoryRef);
					
					sprintf(out_string, "%s" UUmNL, BFrFileRef_GetLeafName(&ref));
					BFrFile_Write(inFile, strlen(out_string), out_string);
				}
			}
		}
		else if (UUrString_Compare_NoCase(OScImpulseSuffix, BFrFileRef_GetSuffixName(&ref)) == 0)
		{
			SStImpulse					*impulse;
			
			UUrString_Copy(name, BFrFileRef_GetLeafName(&ref), BFcMaxFileNameLength);
			UUrString_StripExtension(name);
			UUrString_MakeLowerCase(name, BFcMaxFileNameLength);
			
			impulse = OSrImpulse_GetByName(name);
			if ((impulse != NULL) &&
				(impulse->min_volume_distance < impulse->max_volume_distance))
			{
				if (BFrFileRef_IsLocked(&ref) == UUcTrue)
				{
					sprintf(
						out_string,
						"Impulse sound file %s needs to be modified, but the file is locked." UUmNL,
						BFrFileRef_GetLeafName(&ref));
					
					WMrDialog_MessageBox(
						NULL,
						"Error",
						out_string,
						WMcMessageBoxStyle_OK);
					
					sprintf(out_string, "File is locked, %s" UUmNL, BFrFileRef_GetLeafName(&ref));
					BFrFile_Write(inFile, strlen(out_string), out_string);
				}
				else
				{
					temp_distance = impulse->min_volume_distance;
					impulse->min_volume_distance = impulse->max_volume_distance;
					impulse->max_volume_distance = temp_distance;
					
					OSrImpulse_Save(impulse, inDirectoryRef);

					sprintf(out_string, "%s" UUmNL, BFrFileRef_GetLeafName(&ref));
					BFrFile_Write(inFile, strlen(out_string), out_string);
				}
			}
		}
	}
	
	if (file_iterator != NULL)
	{
		BFrDirectory_FileIterator_Delete(file_iterator);
		file_iterator = NULL;
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OWrMinMax_Fixer(
	void)
{
	UUtError					error;
	BFtFileRef					*binary_dir_ref;
	BFtFileRef					*file_ref;
	BFtFile						*file;
	
	// get the binary director reference
	error = OSrGetSoundBinaryDirectory(&binary_dir_ref);
	if (error != UUcError_None) { goto handle_error; }
	
	// make file ref
	error = BFrFileRef_MakeFromName("MinMaxDistFixes.txt", &file_ref);
	if (error != UUcError_None) { goto handle_error; }
	
	// create file if it doesn't exist
	if (BFrFileRef_FileExists(file_ref) == UUcFalse)
	{
		error = BFrFile_Create(file_ref);
		if (error != UUcError_None) { goto handle_error; }
	}
	
	// open the file
	error = BFrFile_Open(file_ref, "rw", &file);
	if (error != UUcError_None) { goto handle_error; }
	
	// set the position to 0
	error = BFrFile_SetPos(file, 0);
	if (error != UUcError_None) { goto handle_error; }

	// process the binary files
	error = OWiMinMax_Fixer(binary_dir_ref, file);
	if (error != UUcError_None) { goto handle_error; }
	
	// close the file
	BFrFile_Close(file);
	file = NULL;
	
	// delete the file ref
	BFrFileRef_Dispose(file_ref);
	file_ref = NULL;
	
	WMrDialog_MessageBox(
		NULL,
		"Success",
		"The Min/Max distance have been fixed",
		WMcMessageBoxStyle_OK);
	
	goto exit;
	
handle_error:
	if (error != UUcError_None)
	{
		WMrDialog_MessageBox(
			NULL,
			"Error",
			"Unable to process the ambient and impulse files.",
			WMcMessageBoxStyle_OK);
	}
	
exit:
	if (binary_dir_ref != NULL)
	{
		BFrFileRef_Dispose(binary_dir_ref);
		binary_dir_ref = NULL;
	}
	
	return error;
}

// ----------------------------------------------------------------------
static UUtError
OWiSpeech_Fixer(
	BFtFileRef					*inDirectoryRef,
	BFtFile						*inFile)
{
	UUtError					error;
	BFtFileIterator				*file_iterator;
	
	// go through all of the ambient sounds and fix the ones whose
	// max volume distance is less than the min volume distance
	error =
		BFrDirectory_FileIterator_New(
			inDirectoryRef,
			NULL,
			NULL,
			&file_iterator);
	UUmError_ReturnOnError(error);
	
	while (1)
	{
		BFtFileRef					ref;
		char						name[BFcMaxFileNameLength];
		char						out_string[1024];
		
		error = BFrDirectory_FileIterator_Next(file_iterator, &ref);
		if (error != UUcError_None) { break; }
		
		if (BFrFileRef_IsDirectory(&ref))
		{
			OWiSpeech_Fixer(&ref, inFile);
		}
		else if (UUrString_Compare_NoCase(OScAmbientSuffix, BFrFileRef_GetSuffixName(&ref)) == 0)
		{
			SStAmbient					*ambient;
			UUtUns32					flags;
			
			UUrString_Copy(name, BFrFileRef_GetLeafName(&ref), BFcMaxFileNameLength);
			UUrString_StripExtension(name);
			UUrString_MakeLowerCase(name, BFcMaxFileNameLength);
			
			ambient = OSrAmbient_GetByName(name);
			if (ambient == NULL) { continue; }
			
			if ((ambient->min_volume_distance == 10.0f) && (ambient->max_volume_distance == 1.0f))
			{
				if (BFrFileRef_IsLocked(&ref) == UUcTrue)
				{
					sprintf(
						out_string,
						"Ambient sound file %s needs to be modified, but the file is locked." UUmNL,
						BFrFileRef_GetLeafName(&ref));
					
					WMrDialog_MessageBox(
						NULL,
						"Error",
						out_string,
						WMcMessageBoxStyle_OK);
					
					sprintf(out_string, "File is locked, %s" UUmNL, BFrFileRef_GetLeafName(&ref));
					BFrFile_Write(inFile, strlen(out_string), out_string);
					
					continue;
				}
				else
				{
					ambient->min_volume_distance = 50.0f;
					ambient->max_volume_distance = 10.0f;
					
					OSrAmbient_Save(ambient, inDirectoryRef);
					
					sprintf(out_string, "%s" UUmNL, BFrFileRef_GetLeafName(&ref));
					BFrFile_Write(inFile, strlen(out_string), out_string);
				}
			}
			
			flags = (SScAmbientFlag_InterruptOnStop | SScAmbientFlag_PlayOnce);
			if (((ambient->flags & flags) == flags) && (ambient->priority != SScPriority2_Highest))
			{
				if (BFrFileRef_IsLocked(&ref) == UUcTrue)
				{
					sprintf(
						out_string,
						"Ambient sound file %s needs to be modified, but the file is locked." UUmNL,
						BFrFileRef_GetLeafName(&ref));
					
					WMrDialog_MessageBox(
						NULL,
						"Error",
						out_string,
						WMcMessageBoxStyle_OK);
					
					sprintf(out_string, "File is locked, %s" UUmNL, BFrFileRef_GetLeafName(&ref));
					BFrFile_Write(inFile, strlen(out_string), out_string);
					
					continue;
				}
				else
				{
					ambient->priority = SScPriority2_Highest;
					
					OSrAmbient_Save(ambient, inDirectoryRef);
					
					sprintf(out_string, "%s" UUmNL, BFrFileRef_GetLeafName(&ref));
					BFrFile_Write(inFile, strlen(out_string), out_string);
				}
			}
		}
	}
	
	if (file_iterator != NULL)
	{
		BFrDirectory_FileIterator_Delete(file_iterator);
		file_iterator = NULL;
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OWrSpeech_Fixer(
	void)
{
	UUtError					error;
	BFtFileRef					*binary_dir_ref;
	BFtFileRef					*file_ref;
	BFtFile						*file;
	
	// get the binary director reference
	error = OSrGetSoundBinaryDirectory(&binary_dir_ref);
	if (error != UUcError_None) { goto handle_error; }
	
	// make file ref
	error = BFrFileRef_MakeFromName("SpeechFixes.txt", &file_ref);
	if (error != UUcError_None) { goto handle_error; }
	
	// create file if it doesn't exist
	if (BFrFileRef_FileExists(file_ref) == UUcFalse)
	{
		error = BFrFile_Create(file_ref);
		if (error != UUcError_None) { goto handle_error; }
	}
	
	// open the file
	error = BFrFile_Open(file_ref, "rw", &file);
	if (error != UUcError_None) { goto handle_error; }
	
	// set the position to 0
	error = BFrFile_SetPos(file, 0);
	if (error != UUcError_None) { goto handle_error; }

	// process the binary files
	error = OWiSpeech_Fixer(binary_dir_ref, file);
	if (error != UUcError_None) { goto handle_error; }
	
	// close the file
	BFrFile_Close(file);
	file = NULL;
	
	// delete the file ref
	BFrFileRef_Dispose(file_ref);
	file_ref = NULL;
	
	WMrDialog_MessageBox(
		NULL,
		"Success",
		"The speech ambients have been fixed",
		WMcMessageBoxStyle_OK);
	
	goto exit;
	
handle_error:
	if (error != UUcError_None)
	{
		WMrDialog_MessageBox(
			NULL,
			"Error",
			"Unable to process the speech ambient files.",
			WMcMessageBoxStyle_OK);
	}
	
exit:
	if (binary_dir_ref != NULL)
	{
		BFrFileRef_Dispose(binary_dir_ref);
		binary_dir_ref = NULL;
	}
	
	return error;
}

#endif /* TOOL_VERSION */
