// ======================================================================
// Oni_UnitViewer.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_DialogManager.h"
#include "VM_View_Box.h"

#include "Oni_UnitViewer.h"

#include "Oni_Weapon.h"
#include "Oni_Character.h"
#include "Oni_GameStatePrivate.h"

#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Mac)

#include "ICAPI.h"
#include "ICTypes.h"

#elif defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Win32)
#endif

// ======================================================================
// defines
// ======================================================================
#define ONcUV_Forward				LIcKeyCode_W
#define ONcUV_Backward				LIcKeyCode_S
#define ONcUV_StepLeft				LIcKeyCode_A
#define ONcUV_StepRight				LIcKeyCode_D
#define ONcUV_TurnLeft				LIcKeyCode_Q
#define ONcUV_TurnRight				LIcKeyCode_E
#define ONcUV_FirePunch				LIcKeyCode_F
#define ONcUV_Kick					LIcKeyCode_C
#define ONcUV_Crouch				LIcKeyCode_LeftShift
#define ONcUV_Jump					LIcKeyCode_Space

#define ONcUV_UpdateTimerID			1
#define ONcUV_UpdateFrequency		4 /* num ticks between updates */

#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Mac)
#define ONcUV_Creator				'????'
#endif
 	
// ======================================================================
// enums
// ======================================================================
// ----------------------------------------------------------------------
enum
{
	ONcUV_Btn_Quit					= 100,
	ONcUV_Btn_Characters			= 101,
	ONcUV_Btn_Weapons				= 102,
	ONcUV_Btn_Controls				= 103,
	ONcUV_Btn_OniWebsite			= 104,
	ONcUV_Btn_Select				= 105,
	ONcUV_Btn_Stats					= 106,
	ONcUV_Btn_Info					= 107,
	
	ONcUV_Box_CharView				= 140,
	
	ONcUV_Pict_CharLine				= 150,
	ONcUV_Pict_CharSelectLine		= 151,
	ONcUV_Pict_WeapLine				= 160,
	ONcUV_Pict_WeapSelectLine		= 161,
	
	ONcUV_LB_Characters				= 400,
	ONcUV_LB_Weapons				= 401
};

// ======================================================================
// globals
// ======================================================================
static UVtDataList			*ONgUV_CharacterList;
static UVtDataList			*ONgUV_WeaponList;
static UVtDataList			*ONgUV_URLList;

static UUtUns16				ONgUV_CurrentListID;

static LItAction			ONgUV_ActionBuffer;


// ======================================================================
// prototypes
// ======================================================================
UUtBool
ONrDialogCallbac_UnitViewerControls(
	DMtDialog				*inDialog,
	DMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);
	
// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
ONiCharViewBoxCallback(
	VMtView					*inView,
	VMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	// let the box callback handle all of the received messages
	VMrView_Box_Callback(inView, inMessage, inParam1, inParam2);
	
	// draw the character on top of the box
	if (inMessage == VMcMessage_Paint)
	{
		ONrUnitViewer_Character_Update(1, &ONgUV_ActionBuffer);
	}
	
	return 0;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
ONiUnitViewer_Characters(
	DMtDialog				*inDialog,
	UUtBool					inSetSelection)
{
	VMtView					*characters_list;
	VMtView					*weapons_list;
	VMtView					*char_line;
	VMtView					*char_select_line;
	VMtView					*weap_line;
	VMtView					*weap_select_line;
	
	// get character related views
	characters_list = DMrDialog_GetViewByID(inDialog, ONcUV_LB_Characters);
	UUmAssert(characters_list);
	
	char_line = DMrDialog_GetViewByID(inDialog, ONcUV_Pict_CharLine);
	UUmAssert(char_line);
	
	char_select_line = DMrDialog_GetViewByID(inDialog, ONcUV_Pict_CharSelectLine);
	UUmAssert(char_select_line);
	
	// get weapon related views
	weapons_list = DMrDialog_GetViewByID(inDialog, ONcUV_LB_Weapons);
	UUmAssert(weapons_list);
	
	weap_line = DMrDialog_GetViewByID(inDialog, ONcUV_Pict_WeapLine);
	UUmAssert(weap_line);
	
	weap_select_line = DMrDialog_GetViewByID(inDialog, ONcUV_Pict_WeapSelectLine);
	UUmAssert(weap_select_line);
	
	// set the visibility of the views
	VMrView_SetVisible(characters_list, UUcTrue);
	VMrView_SetVisible(char_line, UUcTrue);
	VMrView_SetVisible(char_select_line, UUcTrue);

	VMrView_SetVisible(weapons_list, UUcFalse);
	VMrView_SetVisible(weap_line, UUcFalse);
	VMrView_SetVisible(weap_select_line, UUcFalse);
	
	// set the current list ID
	ONgUV_CurrentListID = ONcUV_LB_Characters;
	
	// select the first character in the list
	if (inSetSelection)
	{
		VMrView_SendMessage(
			characters_list,
			LBcMessage_SetSelection,
			0,
			0);
	}
	
	// set the focus
	DMrDialog_SetFocus(inDialog, characters_list);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiUnitViewer_Info(
	DMtDialog				*inDialog)
{
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiUnitViewer_OniWebsite(
	DMtDialog				*inDialog)
{
	char					oni_website_url[255];

	UUrString_Copy(oni_website_url, ONgUV_URLList->data[0].name, sizeof(oni_website_url));

#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Mac)
	{
		ICInstance				UV_ICInstance;
		ICError					err;
		UUtInt32				oni_website_url_len;
		UUtInt32				oni_website_url_start;
		UUtInt32				oni_website_url_end;	
		
		err = ICStart(&UV_ICInstance, ONcUV_Creator);
		if (err != noErr) return UUcError_Generic;
		
		err = ICFindConfigFile(UV_ICInstance, 0, nil);
		if (err != noErr) return UUcError_Generic;
		
		oni_website_url_len = strlen(oni_website_url);
		c2pstr(oni_website_url);
		oni_website_url_start = 0;
		oni_website_url_end = oni_website_url[0];
		
		err =
			ICLaunchURL(
				UV_ICInstance,
				"\p",
				(char*)&oni_website_url[1],
				oni_website_url[0],
				&oni_website_url_start,
				&oni_website_url_end);
		if (err != noErr) return UUcError_Generic;
		
		ICStop(UV_ICInstance);
	}
#elif defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Win32)
	{
		HINSTANCE				hApp;

		hApp = ShellExecute (GetDesktopWindow(), "open", oni_website_url, NULL, NULL, SW_SHOW);
	}
#endif

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiUnitViewer_Controls(
	DMtDialog				*inDialog)
{
	UUtError				error;
	UUtUns32				message;
	
	error =
		DMrDialog_Run(
			ONcDialogID_UnitViewerControls,
			ONrDialogCallbac_UnitViewerControls,
			inDialog,
			&message);
	if (error != UUcError_None) return error;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiUnitViewer_Select(
	DMtDialog				*inDialog)
{
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiUnitViewer_Stats(
	DMtDialog				*inDialog)
{
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiUnitViewer_Weapons(
	DMtDialog				*inDialog,
	UUtBool					inSetSelection)
{
	VMtView					*characters_list;
	VMtView					*weapons_list;
	VMtView					*char_line;
	VMtView					*char_select_line;
	VMtView					*weap_line;
	VMtView					*weap_select_line;
	
	// get character related views
	characters_list = DMrDialog_GetViewByID(inDialog, ONcUV_LB_Characters);
	UUmAssert(characters_list);
	
	char_line = DMrDialog_GetViewByID(inDialog, ONcUV_Pict_CharLine);
	UUmAssert(char_line);
	
	char_select_line = DMrDialog_GetViewByID(inDialog, ONcUV_Pict_CharSelectLine);
	UUmAssert(char_select_line);
	
	// get weapon related views
	weapons_list = DMrDialog_GetViewByID(inDialog, ONcUV_LB_Weapons);
	UUmAssert(weapons_list);
	
	weap_line = DMrDialog_GetViewByID(inDialog, ONcUV_Pict_WeapLine);
	UUmAssert(weap_line);
	
	weap_select_line = DMrDialog_GetViewByID(inDialog, ONcUV_Pict_WeapSelectLine);
	UUmAssert(weap_select_line);
	
	// set the visibility of the views
	VMrView_SetVisible(characters_list, UUcFalse);
	VMrView_SetVisible(char_line, UUcFalse);
	VMrView_SetVisible(char_select_line, UUcFalse);

	VMrView_SetVisible(weapons_list, UUcTrue);
	VMrView_SetVisible(weap_line, UUcTrue);
	VMrView_SetVisible(weap_select_line, UUcTrue);
	
	// set the current list ID
	ONgUV_CurrentListID = ONcUV_LB_Characters;
	
	// select the first weapon in the list
	if (inSetSelection)
	{
		VMrView_SendMessage(
			weapons_list,
			LBcMessage_SetSelection,
			0,
			0);
	}
	
	// set the focus
	DMrDialog_SetFocus(inDialog, weapons_list);

	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
ONiUnitViewer_Character_SetClass(
	ONtCharacterClass			*inCharacterClass)
{
	ONrCharacter_SetCharacterClass(&ONgGameState->characters[0], inCharacterClass);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiUnitViewer_Weapon_SetClass(
	WPtWeaponClass			*inWeaponClass)
{
	ONrCharacter_UseNewWeapon(&ONgGameState->characters[0], inWeaponClass);
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
ONiUnitViewer_ListBox_Initialize(
	DMtDialog				*inDialog,
	UUtUns16				inListBoxID,
	UVtDataList				*inDataList)
{
	UUtError				error;
	VMtView					*listbox;
	UUtUns32				i;
	IMtPixel				gray;
	IMtPixel				white;
	
	// get the list box view
	listbox = DMrDialog_GetViewByID(inDialog, inListBoxID);
	if (listbox == NULL) return UUcError_Generic;	
	
	// get default gray and white
	gray = IMrPixel_FromShade(VMcControl_PixelType, IMcShade_Gray50);
	white = IMrPixel_FromShade(VMcControl_PixelType, IMcShade_White);
	
	// fill in the lines of the text boxes
	for (i = 0; i < inDataList->num_data_entries; i++)
	{
		UUtInt32			index;
		void				*instance;
		UUtBool				instance_available;
		
		// add the name to the list
		index =
			VMrView_SendMessage(
				listbox,
				LBcMessage_AddString,
				0,
				(UUtUns32)inDataList->data[i].name);
		
		// set the color
		switch (inDataList->data_type)
		{
			case UVcDataType_Character:
				error =
					TMrInstance_GetDataPtr(
						TRcTemplate_CharacterClass,
						inDataList->data[i].instance_name,
						&instance);
			break;
			
			case UVcDataType_Weapon:
				error =
					TMrInstance_GetDataPtr(
						WPcTemplate_WeaponClass,
						inDataList->data[i].instance_name,
						&instance);
			break;
		}
		
		if ((error == UUcError_None) && (instance != NULL))
		{
			instance_available = UUcTrue;
		}
		else
		{
			instance_available = UUcFalse;
		}
		
		if (instance_available)
		{
			VMrView_SendMessage(
				listbox,
				LBcMessage_SetLineColor,
				index,
				(UUtUns32)(&white));
		}
		else
		{
			VMrView_SendMessage(
				listbox,
				LBcMessage_SetLineColor,
				index,
				(UUtUns32)(&gray));
		}
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
ONiUnitViewer_ListBox_Scroll(
	DMtDialog				*inDialog,
	UUtUns16				inListBoxID,
	UUtBool					inDirection)
{
	VMtView					*listbox;
	UUtUns32				current_selection;
	UUtUns32				num_lines;
	
	// get the list box view
	listbox = DMrDialog_GetViewByID(inDialog, inListBoxID);
	if (listbox == NULL) return;
	
	// get the current_selection from the list box
	current_selection = 
		VMrView_SendMessage(
			listbox,
			LBcMessage_GetCurrentSelection,
			0,
			0);
	
	// get the number of lines from the list box
	num_lines = 
		VMrView_SendMessage(
			listbox,
			LBcMessage_GetNumLines,
			0,
			0);
	
	if (inDirection)
	{
		if (current_selection > 0)
		{
			current_selection--;
		}
	}
	else
	{
		if (current_selection < num_lines)
		{
			current_selection++;
		}
	}
	
	// tell the list to scroll
	VMrView_SendMessage(
		listbox,
		LBcMessage_SetSelection,
		current_selection,
		0);
}

// ----------------------------------------------------------------------
static UUtError
ONiUnitViewer_ListBox_SelectionChange(
	DMtDialog				*inDialog,
	VMtView					*inListBox,
	UVtDataList				*inDataList)
{
	UUtError				error;
	UUtInt32				selected_item;
	
	// get the currently selected item of the listbox
	selected_item =
		(UUtInt32)VMrView_SendMessage(
			inListBox,
			LBcMessage_GetCurrentSelection,
			0,
			0);
	
	if ((selected_item < 0) || (selected_item >= (UUtInt32)inDataList->num_data_entries))
	{
		// avoid recursion
		if (inDataList->num_data_entries == 0) return UUcError_None;
		
		// the selected item is out of the range, select the first item
		// in the list
		selected_item = 0;
		
		VMrView_SendMessage(
			inListBox,
			LBcMessage_SetSelection,
			selected_item,
			0);
			
		return UUcError_None;
	}
	
	// a new item was selected, make use of it
	switch (inDataList->data_type)
	{
		case UVcDataType_Character:
		{
			ONtCharacterClass			*character_class;
			
			error =
				TMrInstance_GetDataPtr(
					TRcTemplate_CharacterClass,
					ONgUV_CharacterList->data[selected_item].instance_name,
					&character_class);
			if (error != UUcError_None) return error;
			
			error = ONiUnitViewer_Character_SetClass(character_class);
		}
		break;
		
		case UVcDataType_Weapon:
		{
			WPtWeaponClass				*weapon_class;
			
			error =
				TMrInstance_GetDataPtr(
					WPcTemplate_WeaponClass,
					ONgUV_WeaponList->data[selected_item].instance_name,
					&weapon_class);
			if (error != UUcError_None) return error;

			error = ONiUnitViewer_Weapon_SetClass(weapon_class);
		}
		break;
	}
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
ONiUnitViewer_Destroy(
	DMtDialog				*inDialog)
{
	// terminate the character display
	ONrUnitViewer_Character_Terminate();
	
	// terminate the timer
	VMrView_Timer_Stop(inDialog, ONcUV_UpdateTimerID);
}

// ----------------------------------------------------------------------
static UUtError
ONiUnitViewer_Initialize(
	DMtDialog				*inDialog)
{
	UUtError				error;
	UVtDataList				*datalists[10];
	UUtUns32				num_datalists;
	UUtUns32				i;
	VMtView					*character_view;
	
	// get the uv data lists
	error =
		TMrInstance_GetDataPtr_List(
			UVcTemplate_DataList,
			10,
			&num_datalists,
			datalists);
	UUmError_ReturnOnError(error);
	
	for (i = 0; i < num_datalists; i++)
	{
		switch (datalists[i]->data_type)
		{
			case UVcDataType_Character:
				ONgUV_CharacterList = datalists[i];
			break;

			case UVcDataType_Weapon:
				ONgUV_WeaponList = datalists[i];
			break;
			
			case UVcDataType_URL:
				ONgUV_URLList = datalists[i];
			break;
			
			default:
				return UUcError_Generic;
			break;
		}
	}
	
	if ((ONgUV_CharacterList == NULL) ||
		(ONgUV_WeaponList == NULL) ||
		(ONgUV_URLList == NULL))
	{
		return UUcError_Generic;
	}
	
	// initialize the character display
	error =	ONrUnitViewer_Character_Initialize();
	
	// initialize the list boxes
	error =
		ONiUnitViewer_ListBox_Initialize(
			inDialog,
			ONcUV_LB_Characters,
			ONgUV_CharacterList);
	UUmError_ReturnOnError(error);
	
	error =
		ONiUnitViewer_ListBox_Initialize(
			inDialog,
			ONcUV_LB_Weapons,
			ONgUV_WeaponList);
	UUmError_ReturnOnError(error);
	
	ONiUnitViewer_Weapons(inDialog, UUcTrue);
	ONiUnitViewer_Characters(inDialog, UUcTrue);
	
	// replace the view proc of the character view box
	character_view = DMrDialog_GetViewByID(inDialog, ONcUV_Box_CharView);
	UUmAssert(character_view);
	VMrView_SetCallback(character_view, ONiCharViewBoxCallback);
	
	// clear the action buffer
	UUrMemory_Clear(&ONgUV_ActionBuffer, sizeof(LItAction));
	
	// set the timer
	VMrView_Timer_Start(inDialog, ONcUV_UpdateTimerID, ONcUV_UpdateFrequency);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiUnitViewer_HandleCommand(
	DMtDialog				*inDialog,
	UUtUns16				inCommandType,
	UUtUns16				inViewID,
	VMtView					*inView)
{
	UUtError				error;
	
	if (inCommandType == VMcNotify_Click)
	{
		switch (inViewID)
		{
			case ONcUV_Btn_Quit:
				DMrDialog_Stop(inDialog, 0);
			break;
			
			case ONcUV_Btn_Characters:
				error = ONiUnitViewer_Characters(inDialog, UUcFalse);
			break;
			
			case ONcUV_Btn_Weapons:
				error = ONiUnitViewer_Weapons(inDialog, UUcFalse);
			break;
			
			case ONcUV_Btn_Controls:
				error = ONiUnitViewer_Controls(inDialog);
			break;

			case ONcUV_Btn_OniWebsite:
				error = ONiUnitViewer_OniWebsite(inDialog);
				if (error == UUcError_None)
				{
					DMrDialog_Stop(inDialog, 0);
				}
			break;
			
			case ONcUV_Btn_Select:
				error = ONiUnitViewer_Select(inDialog);
			break;
			
			case ONcUV_Btn_Stats:
				error = ONiUnitViewer_Stats(inDialog);
			break;
			
			case ONcUV_Btn_Info:
				error = ONiUnitViewer_Info(inDialog);
			break;
		}
	}
	else if (inCommandType == LBcNotify_SelectionChanged)
	{
		if (inViewID == ONcUV_LB_Characters)
		{
			error =
				ONiUnitViewer_ListBox_SelectionChange(
					inDialog,
					inView,
					ONgUV_CharacterList);
		}
		else
		{
			error =
				ONiUnitViewer_ListBox_SelectionChange(
					inDialog,
					inView,
					ONgUV_WeaponList);
		}
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiUnitViewer_HandleKeyDown(
	DMtDialog				*inDialog,
	UUtUns32				inKey)
{	
	switch (inKey)
	{
		case LIcKeyCode_Escape:
			DMrDialog_Stop(inDialog, 0);
		break;
				
/*		case LIcKeyCode_UpArrow:
			ONiUnitViewer_ListBox_Scroll(
				inDialog,
				ONgUV_CurrentListID,
				UUcTrue);
		break;
		
		case LIcKeyCode_DownArrow:
			ONiUnitViewer_ListBox_Scroll(
				inDialog,
				ONgUV_CurrentListID,
				UUcFalse);
		break;*/
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
ONrUnitViewer_RegisterTemplates(
	void)
{
	UUtError				error;
	
	error =
		TMrTemplate_Register(
			UVcTemplate_DataList,
			sizeof(UVtDataList),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiUnitViewer_Update(
	DMtDialog				*inDialog)
{
	ONgUV_ActionBuffer.buttonBits = 0;
	
	// test the keys
	if (LIrTestKey(ONcUV_Forward))
		ONgUV_ActionBuffer.buttonBits |= LIc_BitMask_Forward;
	
	if (LIrTestKey(ONcUV_Backward))
		ONgUV_ActionBuffer.buttonBits |= LIc_BitMask_Backward;
	
	if (LIrTestKey(ONcUV_StepLeft))
		ONgUV_ActionBuffer.buttonBits |= LIc_BitMask_StepLeft;
	
	if (LIrTestKey(ONcUV_StepRight))
		ONgUV_ActionBuffer.buttonBits |= LIc_BitMask_StepRight;
	
	if (LIrTestKey(ONcUV_TurnLeft))
		ONgUV_ActionBuffer.buttonBits |= LIc_BitMask_TurnLeft;
	
	if (LIrTestKey(ONcUV_TurnRight))
		ONgUV_ActionBuffer.buttonBits |= LIc_BitMask_TurnRight;
	
	if (LIrTestKey(ONcUV_FirePunch))
		ONgUV_ActionBuffer.buttonBits |= LIc_BitMask_Fire1;
	
	if (LIrTestKey(ONcUV_Kick))
		ONgUV_ActionBuffer.buttonBits |= LIc_BitMask_Kick;
	
	if (LIrTestKey(ONcUV_Crouch))
		ONgUV_ActionBuffer.buttonBits |= LIc_BitMask_Crouch;
	
	if (LIrTestKey(ONcUV_Jump))
		ONgUV_ActionBuffer.buttonBits |= LIc_BitMask_Jump;
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtBool
ONrDialogCallback_UnitViewer(
	DMtDialog				*inDialog,
	DMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool					handled;
	UUtError				error;
	
	handled = UUcTrue;
	
	switch (inMessage)
	{
		case DMcMessage_InitDialog:
			error = ONiUnitViewer_Initialize(inDialog);
			if (error != UUcError_None)
			{
				DMrDialog_Stop(inDialog, 0);
			}
		break;
		
		case VMcMessage_Destroy:
			ONiUnitViewer_Destroy(inDialog);
		break;
		
		case VMcMessage_Command:
			error =
				ONiUnitViewer_HandleCommand(
					inDialog,
					(UUtUns16)UUmHighWord(inParam1),
					(UUtUns16)UUmLowWord(inParam1),
					(VMtView*)inParam2);
			if (error != UUcError_None)
			{
				DMrDialog_Stop(inDialog, 0);
			}
		break;
		
		case VMcMessage_KeyDown:
			error = ONiUnitViewer_HandleKeyDown(inDialog, inParam1);
			if (error != UUcError_None)
			{
				DMrDialog_Stop(inDialog, 0);
			}
		break;
		
		case VMcMessage_Timer:
			error = ONiUnitViewer_Update(inDialog);
			if (error != UUcError_None)
			{
				DMrDialog_Stop(inDialog, 0);
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ----------------------------------------------------------------------
UUtBool
ONrDialogCallbac_UnitViewerControls(
	DMtDialog				*inDialog,
	DMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool					handled;
	
	handled = UUcTrue;
	
	switch (inMessage)
	{
		case DMcMessage_InitDialog:
			DMrDialog_SetMouseFocusView(inDialog, inDialog);
		break;
		
		case VMcMessage_Destroy:
			DMrDialog_ReleaseMouseFocusView(inDialog, inDialog);
		break;
		
		case VMcMessage_LMouseUp:
		case VMcMessage_KeyDown:
			DMrDialog_Stop(inDialog, 0);
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

#if defined(ONmUnitViewer) && ONmUnitViewer
static uv_camera(void)
{
	CAtCamera *camera = ONgGameState->local.camera;
	OBtAnimation *animation;
	UUtError error;
	char *object_animation_name = "camera1";

	// Load the named animation
	error = TMrInstance_GetDataPtr(
		OBcTemplate_Animation,
		object_animation_name,
		&animation);
	UUmAssert(UUcError_None == error);

	// Set up the camera animation
	camera->posAnim.animation = animation;
	OBrAnim_Reset(&camera->posAnim);
	OBrAnim_Start(&camera->posAnim);
	camera->modeFlags |= CAcModeBit_AnimateBoth;

	//COrCommand_Execute("gs_do_environment = 0");

	return UUcError_None;
}
#endif

// ----------------------------------------------------------------------
UUtError
ONrUnitViewer_Character_Initialize(
	void)
{
	COrCommand_Execute("gs_do_environment = 0");
	COrCommand_Execute("co_display = 0");

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
ONrUnitViewer_Character_Terminate(
	void)
{
	return;
}

// ----------------------------------------------------------------------
void
ONrUnitViewer_Character_Update(
	UUtUns16 numActionsInBuffer,
	LItAction *actionBuffer)
{
	uv_camera();
	ONrGameState_UpdateServerTime(ONgGameState);
	ONrGameState_Update(numActionsInBuffer, actionBuffer);
	ONrGameState_Display();

	return;
}


