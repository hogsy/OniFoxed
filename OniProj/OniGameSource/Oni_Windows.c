#include "BFW.h"

#if TOOL_VERSION
// ======================================================================
// Oni_Windows.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW_WindowManager.h"
#include "BFW_Timer.h"

#include "WM_Dialog.h"
#include "WM_DrawContext.h"
#include "WM_PartSpecification.h"
#include "WM_Scrollbar.h"
#include "WM_MenuBar.h"
#include "WM_EditField.h"
#include "WM_CheckBox.h"

#include "Oni.h"
#include "Oni_Windows.h"
#include "Oni_Level.h"
#include "Oni_Win_Tools.h"
#include "Oni_Object.h"
#include "Oni_Win_Particle.h"
#include "Oni_Particle3.h"
#include "Oni_Win_Sound2.h"
#include "Oni_Sound2.h"
#include "Oni_Camera.h"
#include "Oni_Win_AI.h"
#include "Oni_Win_ImpactEffects.h"
#include "Oni_Win_TextureMaterials.h"
#include "Oni_Persistance.h"
#include "Oni_InGameUI.h"
#include "Oni_ImpactEffect.h"
#include "Oni_OutGameUI.h"

// ======================================================================
// defines
// ======================================================================
#define OWcMoveDelta					(30)	/* in ticks */

// ======================================================================
// globals
// ======================================================================
static WMtWindow			*OWgOniWindow;
static WMtWindow			*OWgConsole;

static UUtBool				OWgRunStartup;
static UUtUns16				OWgLevelNumber;

static WMtKeyboardCommands	OWgKeyboardCommands;

static IMtPoint2D			OWgPreviousMousePos;
static UUtBool				OBJgMouseMoved;

static UUtBool				OBJgShowStatus;

static UUtUns32				OWgMoveDelta;

// ======================================================================
// prototypes
// ======================================================================
static void
OWiDuplicateSelectedObjects(
	void);

UUtBool OWrHandleObjectVisibilityMenu(WMtMenu *menu, UUtUns16 menu_item_id);
void OWrBuildVisibilityMenu(WMtMenu *menu);

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
void
OWrLevelList_Initialize(
	WMtDialog				*inDialog,
	UUtUns16				inItemID)
{
	UUtError				error;
	UUtUns32				num_descriptors;
	UUtBool					vidmaster = UUcFalse;
	
	// get the number of descriptors
	num_descriptors = (UUtUns16)TMrInstance_GetTagCount(ONcTemplate_Level_Descriptor);
	if (num_descriptors > 0)
	{
		UUtUns32			i;
		WMtListBox			*levels;
		
		// get a pointer to the level list
		levels = WMrDialog_GetItemByID(inDialog, inItemID);
		if (levels == NULL) return;
		
		for (i = 0; i < num_descriptors; i++)
		{
			ONtLevel_Descriptor		*descriptor;
			UUtUns32				index;
			
			// get a pointer to the first descriptor
			error =	TMrInstance_GetDataPtr_ByNumber(ONcTemplate_Level_Descriptor, i, &descriptor);
			if (error != UUcError_None) {
				return;
			}
			
			// if the level in the descriptor doesn't exist, move on
			// to the next descriptor
			if (!TMrLevel_Exists(descriptor->level_number)) {
				continue;
			}

#if SHIPPING_VERSION
			if (!vidmaster) {
				if (!ONrLevelIsUnlocked(descriptor->level_number)) {
					continue;
				}
			}
#endif		

			// add the name of the level to the list
			index = WMrListBox_AddString(levels, descriptor->level_name);
			WMrListBox_SetItemData(levels, descriptor->level_number, index);

			// add the save point information
			{
				const ONtPlace *place = ONrPersist_GetPlace();
				UUtInt32 save_point_index;
				const ONtContinue *save_point;

				for(save_point_index = 0; save_point_index < ONcPersist_NumContinues; save_point_index++)
				{				
					save_point = ONrPersist_GetContinue(descriptor->level_number, save_point_index);

					if (save_point != NULL) {
						char save_point_name[512];

						sprintf(save_point_name, "%s", save_point->name);

						index = WMrListBox_AddString(levels, save_point_name);
						WMrListBox_SetItemData(levels, (save_point_index << 8) | descriptor->level_number, index);

						if ((place->level == descriptor->level_number) && (place->save_point == save_point_index)) {
							WMrListBox_SetSelection(levels, LBcDontNotifyParent, index);
						}
					}
				}
			}

		}
	}
}

// ----------------------------------------------------------------------
UUtUns16
OWrLevelList_GetLevelNumber(
	WMtDialog		*inDialog,
	UUtUns16		inItemID)
{
	WMtWindow		*levels;
	UUtUns16		level_number;
	
	// get a pointer to the levels listbox
	levels = WMrDialog_GetItemByID(inDialog, inItemID);
	if (levels == NULL) { return (UUtUns16)(-1); }
	
	// get the level number of the selected item
	level_number = (UUtUns16)WMrListBox_GetItemData(levels, (UUtUns32)(-1));
	return level_number;
}

/*
UUtUns16
OWrLevelList_GetLevelNumber(
	WMtDialog		*inDialog,
	UUtUns16		inItemID)
{
	UUtUns16		num_descriptors;
	UUtUns16		level_number;
	
	// set the level number to the default
	level_number = (UUtUns16)(-1);
	
	// get the number of descriptors
	num_descriptors = (UUtUns16)TMrInstance_GetTagCount(ONcTemplate_Level_Descriptor);
	if (num_descriptors > 0)
	{
		UUtError		error;
		WMtWindow		*levels;
		char			level_name[ONcMaxLevelName];
		UUtUns16		i;
		
		// get a pointer to the levels listbox
		levels = WMrDialog_GetItemByID(inDialog, inItemID);
		if (levels == NULL) return level_number;
		
		// get the current selection name
		WMrMessage_Send(
			levels,
			LBcMessage_GetText,
			(UUtUns32)(&level_name),
			(UUtUns32)(-1));
		
		// find the level number
		for (i = 0; i < num_descriptors; i++)
		{
			ONtLevel_Descriptor		*descriptor;

			// get a pointer to the first descriptor
			error =
				TMrInstance_GetDataPtr_ByNumber(
					ONcTemplate_Level_Descriptor,
					i,
					&descriptor);
			if (error != UUcError_None) return level_number;

			if (!strcmp(descriptor->level_name, level_name))
			{
				level_number = descriptor->level_number;
				break;
			}
		}
	}
	
	return level_number;
}
*/
// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool OWiOKToQuit(void)
{

#if TOOL_VERSION
	if (P3rAnyDirty()) {			
		UUtUns32 message;

		message = WMrDialog_MessageBox(NULL, "Save particles?",
						"You have unsaved particles. Do you want to save them before quitting?",
						WMcMessageBoxStyle_Yes_No_Cancel);

		if (message == WMcDialogItem_Yes) {
			// save particles before continuing
			OWrSave_Particles(UUcFalse);
		} else if (message == WMcDialogItem_Cancel) {
			// not OK to quit
			return UUcFalse;
		}
	}
#endif

	return UUcTrue;
}

// ----------------------------------------------------------------------
static void
OWiLevelLoad_Init(
	WMtDialog				*inDialog)
{
	WMtListBox				*levels;
	
	OWrLevelList_Initialize(inDialog, OWcLevelLoad_LB_Level);
	
	levels = WMrDialog_GetItemByID(inDialog, OWcLevelLoad_LB_Level);
	if (levels)
	{
		WMrWindow_SetFocus(levels);
	}
}

// ----------------------------------------------------------------------
void
OWrLevelLoad_StartLevel(
	UUtUns16				inLevel)
{
	UUtUns16				num_descriptors;	
	UUtUns16				level_number = inLevel & 0x00ff;
	UUtUns16				save_point = (inLevel & 0xff00) >> 8;
	
	// get the number of descriptors
	num_descriptors = (UUtUns16)TMrInstance_GetTagCount(ONcTemplate_Level_Descriptor);
	if (num_descriptors > 0)
	{
		// set the level number
		OWgLevelNumber = level_number;
		if (OWgLevelNumber != (UUtUns16)(-1))
		{
			// stop running the startup stuff
			OWgRunStartup = UUcFalse;
		}

		// unload any level other than 0
		if (ONrLevel_GetCurrentLevel() != 0)
		{
			ONrLevel_Unload();
		}
		
		// try to load the new level
		ONrGameState_ClearContinue();

		if (save_point != 0) {
			const ONtContinue *save_point_data = ONrPersist_GetContinue(level_number, save_point);
			
			UUmAssert(save_point_data != NULL);

			if (NULL != save_point_data) {
				ONrGameState_Continue_SetFromSave(save_point, save_point_data);
			}
		}

		ONrLevel_Load(OWgLevelNumber, UUcTrue);	

		// run the game
		WMrMessage_Post(NULL, OWcMessage_RunGame, 0, 0);
	}
}

// ----------------------------------------------------------------------
static UUtBool
OWiLevelLoad_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool					handled;
	UUtUns32				level;
	
	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiLevelLoad_Init(inDialog);
		break;
		
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_OK:
					level = (UUtUns32)OWrLevelList_GetLevelNumber(inDialog, OWcLevelLoad_LB_Level);
					WMrDialog_ModalEnd(inDialog, level);
				break;
				
				case WMcDialogItem_Cancel:
					level = (UUtUns32)(-1);
					WMrDialog_ModalEnd(inDialog, level);
				break;
				
				case OWcLevelLoad_LB_Level:
					if (UUmHighWord(inParam1) == WMcNotify_DoubleClick)
					{
						level = OWrLevelList_GetLevelNumber(inDialog, OWcLevelLoad_LB_Level);
						WMrDialog_ModalEnd(inDialog, level);
					}
				break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

/// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
OWiTest_Callback(
	WMtWindow				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool					handled;
	
	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
		{
			WMtWindow		*slider;
			WMtWindow		*listbox;
			WMtWindow		*button;
			UUtUns32		i;
			
			WMrTimer_Start(10, 10, inDialog);
			WMrDialog_RadioButtonCheck(
				inDialog,
				OWcTest_RadioButton_1,
				OWcTest_RadioButton_2,
				OWcTest_RadioButton_1);
			
			button = WMrDialog_GetItemByID(inDialog, WMcDialogItem_OK);
			if (button)
			{
//				WMrWindow_SetLocation(button, 150, -3);
			}
			
			slider = WMrDialog_GetItemByID(inDialog, OWcTest_Slider);
			if (slider)
			{
				WMrSlider_SetRange(slider, 0, 100);
			}
			
			listbox = WMrDialog_GetItemByID(inDialog, OWcTest_ListBox);
			if (listbox)
			{
//				WMrWindow_SetLocation(listbox, -10, -10);

				for (i = 0; i < 30; i++)
				{
					char		string[255];
					
					sprintf(string, "line %d", i);
					
					WMrMessage_Send(listbox, LBcMessage_AddString, (UUtUns32)string, 0);
				}
			}
			
			listbox = WMrDialog_GetItemByID(inDialog, OWcTest_ListBox2);
			if (listbox)
			{
				UUtUns32		nothing[1];
				
				nothing[0] = 0;
				
				for (i = 0; i < 30; i++)
				{
					WMrMessage_Send(listbox, LBcMessage_AddString, (UUtUns32)nothing, 0);
				}
			}
		}
		break;
		
		case WMcMessage_Destroy:
			WMrTimer_Stop(10, inDialog);
		break;
		
		case WMcMessage_Timer:
		{
			UUtUns32		current_val;
			WMtWindow		*progressbar;
			
			progressbar = WMrDialog_GetItemByID(inDialog, OWcTest_ProgressBar);

			current_val = WMrMessage_Send(progressbar, WMcMessage_GetValue, 0, 0) + 1;
			if (current_val > 100) { current_val = 0; }
			
			WMrMessage_Send(progressbar, WMcMessage_SetValue, current_val, 0);
		}
		break;
		
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_OK:
					WMrDialog_MessageBox(
						inDialog,
						"Testing",
						"This is a test of the OK message box",
						WMcMessageBoxStyle_OK);

					WMrDialog_MessageBox(
						inDialog,
						"Testing",
						"This is a test of the OK Cancel message box",
						WMcMessageBoxStyle_OK_Cancel);

					WMrDialog_MessageBox(
						inDialog,
						"Testing",
						"This is a test of the Yes No message box",
						WMcMessageBoxStyle_Yes_No);

					WMrDialog_MessageBox(
						inDialog,
						"Testing",
						"This is a test of the Yes No Cancel message box",
						WMcMessageBoxStyle_Yes_No_Cancel);
				break;
				
				case OWcTest_RadioButton_1:
				case OWcTest_RadioButton_2:
					if (UUmHighWord(inParam1) == WMcNotify_Click)
					{
						WMrDialog_RadioButtonCheck(
							inDialog,
							OWcTest_RadioButton_1,
							OWcTest_RadioButton_2,
							(UUtUns16)UUmLowWord(inParam1));
					}
				break;
				
				case WMcDialogItem_Cancel:
					if (UUmHighWord(inParam1) == WMcNotify_Click)
					{
//						WMrDialog_ModalEnd(inDialog, WMcDialogItem_Cancel);
						WMrWindow_Delete(inDialog);
					}
				break;
				
				case OWcTest_TestChild:
				{
					WMtDialog			*dialog;
					
					WMrDialog_Create(OWcDialog_Test, inDialog, OWiTest_Callback, (UUtUns32) -1, &dialog);
				}
				break;
			}
		break;
		
		case WMcMessage_DrawItem:
		{
			WMtDrawItem			*draw_item;
			UUtUns32			i;
			IMtPoint2D			dest;
			char				string[32];
			
			draw_item = (WMtDrawItem*)inParam2;
			
/*			if (draw_item->state & WMcDrawItemState_Selected)
			{
				PStPartSpecUI	*partspec_ui;
				PStPartSpec		*draw_this;
				
				partspec_ui = PSrPartSpecUI_GetActive();
				
				if (WMrWindow_GetFocus() == draw_item->window)
				{
					draw_this = partspec_ui->hilite;
				}
				else
				{
					draw_this = partspec_ui->background;
				}
				
				dest.x = draw_item->rect.left;
				dest.y = draw_item->rect.top;
				
				DCrDraw_PartSpec(
					draw_item->draw_context,
					draw_this,
					PScPart_All,
					&dest,
					draw_item->rect.right - draw_item->rect.left,
					draw_item->rect.bottom - draw_item->rect.top,
					M3cMaxAlpha);
			}*/

			dest.x = draw_item->rect.left;
			dest.y = draw_item->rect.top + DCrText_GetAscendingHeight();
			
			for (i = 0; i < 3; i++)
			{
				sprintf(string, "[%d][%d]", draw_item->item_id, i);
				DCrText_SetFormat(TSc_HLeft);
				DCrDraw_String(draw_item->draw_context, string, &draw_item->rect, &dest);
				dest.x += 40;
			}
			
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
OWiOniWindow_Create(
	WMtWindow				*inWindow)
{
	WMtWindow				*menubar;
	
	// load the menu bar
	menubar = WMrMenuBar_Create(inWindow, "menubar_oniwindow");
	if (menubar == NULL)
	{
		return UUcError_Generic;
	}
	
	WMrWindow_SetLong(inWindow, 0, (UUtUns32)menubar);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OWiOniWindow_HandleMenuInit(
	WMtWindow				*inWindow,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	WMtMenu					*menu;
	
	menu = (WMtMenu*)inParam1;
	if (menu == NULL) { return; }
	
	switch (UUmHighWord(inParam2))
	{
		case OWcMenu_Game:
			WMrMenu_EnableItem(menu, OWcMenu_Game_Resume, (ONrLevel_GetCurrentLevel() != 0));
		break;

		case OWcMenu_Settings:
		break;
		
#if TOOL_VERSION
		
		case OWcMenu_Objects:
		{
			UUtUns32		num_objects;
			
			num_objects = OBJrSelectedObjects_GetNumSelected();

			WMrMenu_EnableItem(menu, OWcMenu_Obj_New, !OWrObjNew_IsVisible());
			WMrMenu_EnableItem(menu, OWcMenu_Obj_Duplicate, (num_objects >= 1));
			WMrMenu_EnableItem(menu, OWcMenu_Obj_Save, UUcTrue);
			
			if (num_objects == 1)
			{
				WMrMenu_EnableItem(menu, OWcMenu_Obj_Delete, UUcTrue);
				WMrMenu_EnableItem(menu, OWcMenu_Obj_Position, !OWrEOPos_IsVisible());
				WMrMenu_EnableItem(menu, OWcMenu_Obj_Properties, !OWrProp_IsVisible());
			}
			else if (num_objects > 1)
			{
				WMrMenu_EnableItem(menu, OWcMenu_Obj_Delete, UUcTrue);
				WMrMenu_EnableItem(menu, OWcMenu_Obj_Position, UUcTrue);
				WMrMenu_EnableItem(menu, OWcMenu_Obj_Properties, UUcFalse);
			}
			else
			{
				WMrMenu_EnableItem(menu, OWcMenu_Obj_Delete, UUcFalse);
				WMrMenu_EnableItem(menu, OWcMenu_Obj_Position, UUcFalse);
				WMrMenu_EnableItem(menu, OWcMenu_Obj_Properties, UUcFalse);
			}

			OWrBuildVisibilityMenu(menu);

			WMrMenu_EnableItem(
				menu,
				OWcMenu_Obj_LightProperties,
				OWrLightProp_HasLight());
		}
		break;
		
		case OWcMenu_Particle:
			WMrMenu_EnableItem(menu, OWcMenu_Particle_Save,		P3rAnyDirty());
			WMrMenu_EnableItem(menu, OWcMenu_Particle_Discard,	P3rAnyDirty());
		break;

		case OWcMenu_AI:
			OWrAI_Menu_Init(menu);
		break;
#endif
	}
}

// ----------------------------------------------------------------------
static void
OWiOniWindow_HandleMenuCommand(
	WMtWindow				*inWindow,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtError				error;
	WMtDialog				*dialog = NULL;
	WMtMenu					*menu;
	UUtUns16				menu_item_id;
	UUtUns32				message;
	
	menu = (WMtMenu*)inParam2;
	UUmAssert(menu);
	
	menu_item_id = UUmLowWord(inParam1);
	
	switch (WMrWindow_GetID(menu))
	{
		// ------------------------------
		case OWcMenu_Game:
			switch (menu_item_id)
			{
				case OWcMenu_Game_NewGame:
					error =
						WMrDialog_ModalBegin(
							OWcDialog_LevelLoad,
							NULL,
							OWiLevelLoad_Callback,
							(UUtUns32) -1, 
							&message);
					if (error == UUcError_None)
					{
						if (message != (UUtUns32)(-1))
						{
							OWrLevelLoad_StartLevel((UUtUns16)message);
						}
					}
				break;
								
				case OWcMenu_Game_Resume:
					if (OWgRunStartup == UUcFalse)
					{
						OWrOniWindow_Toggle();
					}
				break;
				
				case OWcMenu_Game_Quit:
					WMrMessage_Post(NULL, WMcMessage_Quit, 0, 0);
					WMrWindow_SetVisible(inWindow, UUcFalse);
				break;
			}
		break;
		

#if TOOL_VERSION
		// ------------------------------
		case OWcMenu_Settings:
			switch (menu_item_id)
			{
				case OWcMenu_Settings_Player:
					error =
						WMrDialog_ModalBegin(
							OWcDialog_SetupPlayer,
							NULL,
							OWrSettings_Player_Callback,
							(UUtUns32) -1,
							&message);
				break;
				
				case OWcMenu_Settings_Options:
					ONrOutGameUI_Options_Display();
				break;
				
				case OWcMenu_Settings_MainMenu:
					ONrOutGameUI_MainMenu_Display();
				break;
				
				case OWcMenu_Settings_LoadGame:
					ONrOutGameUI_LoadGame_Display();
				break;
				
				case OWcMenu_Settings_QuitGame:
					ONrOutGameUI_QuitYesNo_Display();
				break;
			}
		break;
		
		// ------------------------------
		case OWcMenu_Objects:
			switch (menu_item_id)
			{
				case OWcMenu_Obj_New:
					OWrObjNew_Display();
				break;
				
				case OWcMenu_Obj_Delete:
					OBJrSelectedObjects_Delete();
				break;
				
				case OWcMenu_Obj_Duplicate:
					OWiDuplicateSelectedObjects();
				break;
				
				case OWcMenu_Obj_Save:
					OBJrSaveObjects(OBJcType_None);
					WMrDialog_MessageBox(NULL, "Save Confirmation", "The Objects were saved.", WMcMessageBoxStyle_OK);
				break;
								
				case OWcMenu_Obj_Position:
					OWrEOPos_Display();
				break;
				
				case OWcMenu_Obj_Properties:
					OWrProp_Display(NULL);
				break;

				case OWcMenu_Obj_ExportGunk:
					error = OBJrENVFile_Write(UUcFalse);
					if (error == UUcError_None)
					{
						WMrDialog_MessageBox(NULL, "Save Confirmation", "The .ENV file was saved.", WMcMessageBoxStyle_OK);
					}
					else
					{
						WMrDialog_MessageBox(NULL, "Unable to Save", "The .ENV file was not written.", WMcMessageBoxStyle_OK);
					}
				break;
				
				case OWcMenu_Obj_LightProperties:
					OWrLightProp_Display();
				break;
				
				case OWcMenu_Obj_ExportFlag:
					error = OBJrFlagTXTFile_Write();
					if (error == UUcError_None)
					{
						WMrDialog_MessageBox(NULL, "Save Confirmation", "The LX_Flag.TXT file was saved.", WMcMessageBoxStyle_OK);
					}
					else
					{
						WMrDialog_MessageBox(NULL, "Unable to Save", "The LX_Flag.TXT file was not written.", WMcMessageBoxStyle_OK);
					}
				break;
				
				default:
					OWrHandleObjectVisibilityMenu(menu, menu_item_id);
				break;
			}
		break;
		
		// ------------------------------
		case OWcMenu_Particle:
			switch (menu_item_id)
			{
				case OWcMenu_Particle_Edit:
					WMrDialog_Create(OWcDialog_Particle_Edit, NULL, OWrParticle_Edit_Callback, (UUtUns32) -1, &dialog);
				break;

				case OWcMenu_Particle_Save:
					OWrSave_Particles(UUcFalse);
				break;

				case OWcMenu_Particle_Discard:
					message = WMrDialog_MessageBox(NULL, "Discard Changes?",
									"Erase all changes made to particle definitions?",
									WMcMessageBoxStyle_OK_Cancel);
					if (message == WMcDialogItem_OK) {
						OWrRevert_Particles();
					}
				break;
			}
		break;
		
		
		// ------------------------------
		case OWcMenu_Sound:
			switch (menu_item_id)
			{
				case OWcMenu_Sound_Groups:
					OWrSM_Display(OWcSMType_Group);
				break;
				

				case OWcMenu_Ambient_Sounds:
					OWrSM_Display(OWcSMType_Ambient);
				break;
				
				case OWcMenu_Impulse_Sounds:
					OWrSM_Display(OWcSMType_Impulse);
				break;
				
				case OWcMenu_Animations:
					OWrSoundToAnim_Display();
				break;
				
				case OWcMenu_ExportSoundInfo:
					SSrTextFile_Write();
				break;
				
				case OWcMenu_CreateDialogue:
					OWrCreateDialogue_Display();
				break;
				
				case OWcMenu_CreateImpulse:
					OWrCreateImpulse_Display();
				break;

				case OWcMenu_CreateGroup:
					OWrCreateGroup_Display();
				break;

				case OWcMenu_ViewAnimations:
					OWrViewAnimation_Display();
				break;

				case OWcMenu_WriteAmbAiffList:
					OSrSoundObjects_WriteAiffList();
				break;

				case OWcMenu_WriteNeutralAiffList:
					OSrNeutralInteractions_WriteAiffList();
				break;
				
				case OWcMenu_FixMinMaxDist:
					OWrMinMax_Fixer();
				break;

				case OWcMenu_FindBrokenSound:
					OSrListBrokenSounds();
				break;
				
				case OWcMenu_FixSpeechAmbients:
					OWrSpeech_Fixer();
				break;
			}
		break;

		// ------------------------------
		case OWcMenu_AI:
			OWrAI_Menu_HandleMenuCommand(menu, menu_item_id);
		break;
		
		// ------------------------------
		case OWcMenu_Misc:
			switch (menu_item_id)
			{
				case OWcMenu_Impact_Effect:
					OWrImpactEffect_Display();
				break;
				
				case OWcMenu_Texture_Materials:
					OWrTextureToMaterial_Display();
				break;
				
				case OWcMenu_Impact_Effect_Write:
					ONrImpactEffects_WriteTextFile();
				break;
			}
		break;

		// ------------------------------
		case OWcMenu_Test:
			switch (menu_item_id)
			{
				case OWcMenu_Test_TestDialog:
//					WMrDialog_Create(OWcDialog_Test, NULL, OWiTest_Callback, (UUtUns32) -1, &dialog);
//					WMrDialog_ModalBegin(OWcDialog_Test, NULL, OWiTest_Callback, (UUtUns32) -1, &message);
					ONrPauseScreen_Display();
				break;
			}
		break;
#endif
	}
}

// ----------------------------------------------------------------------
static void
OWiOniWindow_HandleResolutionChanged(
	WMtWindow				*inWindow,
	UUtInt16				inWidth,
	UUtInt16				inHeight)
{
	WMtWindow				*menubar;
		
	WMrWindow_SetSize(inWindow, inWidth, inHeight);
	
	// update the width of the main menu
	menubar = (WMtWindow*)WMrWindow_GetLong(inWindow, 0);
	if (menubar != NULL)
	{
		UUtInt16				menubar_height;
		
		WMrWindow_GetSize(menubar, NULL, &menubar_height);
		WMrWindow_SetSize(menubar, inWidth, menubar_height);
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns8
OWiIsLockedInSelection(
	void)
{
	UUtUns32				i;
	UUtUns32				num_selected;
	UUtUns32				num_locked;
	
	num_locked = 0;
	
	num_selected = OBJrSelectedObjects_GetNumSelected();
	for (i = 0; i < num_selected; i++)
	{
		OBJtObject			*object;
		
		object = OBJrSelectedObjects_GetSelectedObject(i);
		
		if (OBJrObject_IsLocked(object)) { num_locked++; }
	}
	
	if (num_locked == 0) { return 0; }
	if (num_locked == num_selected) { return 2; }
	
	return 1;
}

// ----------------------------------------------------------------------
static void
OWiOniWindow_Paint(
	WMtWindow				*inWindow)
{
	DCtDrawContext			*draw_context;
	PStPartSpecUI			*partspec_ui;
	IMtPoint2D				dest;
	UUtInt16				window_width;
	UUtInt16				window_height;
	UUtInt16				height;
	UUtInt16				part_left;
	UUtInt16				part_top;
	UUtInt16				part_right;
	UUtInt16				part_bottom;
	UUtRect					bounds;
	UUtUns32				mode;
	
	// don't draw the status bar unless OBJgShowStatus is true
	if (OBJgShowStatus == UUcFalse) { return; }
	
	// get a pointer to the partspec ui
	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) { return; }
	
	draw_context = DCrDraw_Begin(inWindow);
	
	// get the width
	WMrWindow_GetSize(inWindow, &window_width, &window_height);
	
	PSrPartSpec_GetSize(partspec_ui->background, PScPart_LeftTop, &part_left, &part_top);
	PSrPartSpec_GetSize(partspec_ui->background, PScPart_RightBottom, &part_right, &part_bottom);
	
	// calculate the height
	height = DCrText_GetLineHeight() + part_top + part_bottom + 2;
	
	// set the dest
	dest.x = 0;
	dest.y = window_height - height;
	
	// draw the background
	DCrDraw_PartSpec(
		draw_context,
		partspec_ui->background,
		PScPart_All,
		&dest,
		window_width,
		height,
		M3cMaxAlpha);
	
	DCrText_SetFormat(TSc_HLeft | TSc_VCenter);
	DCrText_SetStyle(TScStyle_Plain);

	// set the starting position for the status
	dest.x = 0;
	dest.y += part_top + DCrText_GetAscendingHeight();
	
	// set the bounds
	bounds.left = 0;
	bounds.top = window_height - height;
	bounds.right = window_width;
	bounds.bottom = window_height;

	// draw selection locked
	dest.x = window_width - (480 - 395);
	DCrText_SetShade(IMcShade_Black);
	DCrText_DrawText("Selection:", &bounds, &dest);
	
	dest.x = window_width - (480 - 440);
	if (OBJrSelectedObjects_Lock_Get() == UUcTrue)
	{
		DCrText_DrawText("Locked", &bounds, &dest);
	}
	else
	{
		DCrText_SetShade(IMcShade_Gray75);
		DCrText_DrawText("Locked", &bounds, &dest);
	}
	
	// draw the rotation axis
	dest.x = window_width - (480 - 335);
	DCrText_SetShade(IMcShade_Black);
	DCrText_DrawText("Rotate:", &bounds, &dest);

	dest.x = window_width - (480 - 370);
	mode = OBJrRotateMode_Get();
	switch (mode)
	{
		case OBJcRotateMode_XY:
			DCrText_DrawText("Z", &bounds, &dest);
		break;
		
		case OBJcRotateMode_XZ:
			DCrText_DrawText("Y", &bounds, &dest);
		break;
		
		case OBJcRotateMode_YZ:
			DCrText_DrawText("X", &bounds, &dest);
		break;
		
		case OBJcRotateMode_XYZ:
			DCrText_DrawText("All", &bounds, &dest);
		break;
	}
	
	// draw the move axis
	dest.x = window_width - (480 - 265);
	DCrText_SetShade(IMcShade_Black);
	DCrText_DrawText("Move:", &bounds, &dest);
	
	dest.x = window_width - (480 - 295);
	mode = OBJrMoveMode_Get();
	switch (mode)
	{
		case OBJcMoveMode_LR:
			DCrText_DrawText("LR", &bounds, &dest);
		break;
		
		case OBJcMoveMode_UD:
			DCrText_DrawText("UD", &bounds, &dest);
		break;
		
		case OBJcMoveMode_FB:
			DCrText_DrawText("FB", &bounds, &dest);
		break;
		
		case (OBJcMoveMode_LR | OBJcMoveMode_FB):
			DCrText_DrawText("LR FB", &bounds, &dest);
		break;

		case (OBJcMoveMode_Wall | OBJcMoveMode_LR):
			DCrText_DrawText("WALL LR", &bounds, &dest);
		break;

		case (OBJcMoveMode_Wall | OBJcMoveMode_UD):
			DCrText_DrawText("WALL UD", &bounds, &dest);
		break;

		case (OBJcMoveMode_Wall | OBJcMoveMode_UD | OBJcMoveMode_LR):
			DCrText_DrawText("WALL", &bounds, &dest);
		break;
	}
	
	// draw the object locked status
	if (OBJrSelectedObjects_GetNumSelected() > 0)
	{
		dest.x = 5;
		DCrText_SetShade(IMcShade_Black);
		DCrText_DrawText("Object:", &bounds, &dest);
		
		dest.x = 40;
		switch (OWiIsLockedInSelection())
		{
			case 0:
				DCrText_SetShade(IMcShade_Gray25);
				DCrText_DrawText("Unlocked", &bounds, &dest);
			break;
			
			case 1:
				DCrText_SetShade(IMcShade_Black);
				DCrText_DrawText("Some Locked", &bounds, &dest);
			break;
			
			case 2:
				DCrText_SetShade(IMcShade_Black);
				DCrText_DrawText("All Locked", &bounds, &dest);
			break;
		}
	}
	
	DCrDraw_End(draw_context, inWindow);
}

// ----------------------------------------------------------------------
static UUtUns32
OWiOniWindow_Callback(
	WMtWindow				*inWindow,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtError				error;
	
	switch (inMessage)
	{
		case WMcMessage_Create:
			error = OWiOniWindow_Create(inWindow);
			if (error != UUcError_None)
			{
				return WMcResult_Error;
			}
		return WMcResult_Handled;
		
		case WMcMessage_MenuInit:
			OWiOniWindow_HandleMenuInit(inWindow, inParam1, inParam2);
		break;
		
		case WMcMessage_MenuCommand:
			OWiOniWindow_HandleMenuCommand(inWindow, inParam1, inParam2);
		return WMcResult_Handled;
		
		case WMcMessage_KeyDown:
			if ((UUmLowWord(inParam1) == LIcKeyCode_Escape) ||
				(UUmLowWord(inParam1) == LIcKeyCode_Star))
			{
				if (OWgRunStartup == UUcFalse)
				{
					OWrOniWindow_Toggle();
					return WMcResult_Handled;
				}
			}
		break;
		
		case WMcMessage_Activate:
		{
			WMtWindow			*menubar;
//			WMtWindow			*menu;
			
			menubar = WMrDialog_GetItemByID(inWindow, OWcOniWindow_MenuBar);
			if (menubar == NULL) { break; }
			
			// CB: the object and particle menus used to be disabled while on level 0...
			// however sometimes we want to do some editing there so I'm re-enabling them; 13 July 2000
/*			menu = WMrMenuBar_GetMenu(menubar, OWcMenu_Objects);
			if (menu)
			{
				WMrWindow_SetEnabled(menu, (ONrLevel_GetCurrentLevel() != 0));
			}
			
			menu = WMrMenuBar_GetMenu(menubar, OWcMenu_Particle);
			if (menu)
			{
				WMrWindow_SetEnabled(menu, (ONrLevel_GetCurrentLevel() != 0));
			}*/
		}
		break;
		
		case WMcMessage_Paint:
			OWiOniWindow_Paint(inWindow);
		break;
		
		case WMcMessage_ResolutionChanged:
			OWiOniWindow_HandleResolutionChanged(
				inWindow,
				UUmHighWord(inParam1),
				UUmLowWord(inParam1));
		return WMcResult_Handled;
	}
	
	return WMrWindow_DefaultCallback(inWindow, inMessage, inParam1, inParam2);
}

// ----------------------------------------------------------------------
void
OWrOniWindow_Startup(
	void)
{
	UUtError				error;
	void					*splash_screen;
	
	// set the input to normal
	LIrMode_Set(LIcMode_Normal);
	
	// set the desktop background
	error =
		TMrInstance_GetDataPtr(
			M3cTemplate_TextureMap_Big,
			"pict_splash_screen",
			&splash_screen);
	if (error == UUcError_None)
	{
		WMrSetDesktopBackground(splash_screen);
	}
	else
	{
		WMrSetDesktopBackground(PSrPartSpec_LoadByType(PScPartSpecType_BackgroundColor_Blue));
	}
	
	// activate the window manager's update
	WMrActivate();
	
	// display the oni window
	LIrMode_Set(LIcMode_Normal);
	WMrWindow_SetVisible(OWgOniWindow, UUcTrue);
	
	OWgRunStartup = UUcTrue;
	while ((OWgRunStartup) && (ONgTerminateGame == UUcFalse))
	{
		M3rGeom_Frame_Start(0);
		WMrDisplay();
		M3rGeom_Frame_End();

		WMrUpdate();
		OWrUpdate();
	}
	
	// set the desktop background to none
	WMrSetDesktopBackground(PSrPartSpec_LoadByType(PScPartSpecType_BackgroundColor_None));

	if (ONgTerminateGame) { return; }
}

// ----------------------------------------------------------------------
void
OWrOniWindow_Toggle(
	void)
{
	if (OWgOniWindow == NULL) return;
	
	if (WMrWindow_GetVisible(OWgOniWindow) == UUcFalse)
	{
		WMrActivate();
		LIrMode_Set(LIcMode_Normal);
		WMrWindow_SetVisible(OWgOniWindow, UUcTrue);
		WMrWindow_Activate(OWgOniWindow);
	}
	else
	{
		WMrWindow_Activate(WMrGetDesktop());
		WMrWindow_SetVisible(OWgOniWindow, UUcFalse);
		LIrMode_Set(LIcMode_Game);
		WMrDeactivate();
	}
}

// ----------------------------------------------------------------------
UUtBool
OWrOniWindow_Visible(
	void)
{
	if (OWgOniWindow == NULL) return UUcFalse;
	
	return WMrWindow_GetVisible(OWgOniWindow);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
OWrSplashScreen_Display(
	void)
{
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OWiDuplicateSelectedObjects(
	void)
{
//	M3tVector3D			camera_facing;
//	M3tPoint3D			camera_position;
//	M3tVector3D			offset_vector;	/* offset turned off by request of Hardy and ChrisP */
	UUtUns32			num_objects;
	UUtUns32			i;
	OBJtObject			**sol_mem_array;
	OBJtObject			**nol_mem_array;
	
	// build an vector by which the position of the selected objects will
	// be offset so that they aren't exactly on top of the old ones
/*	camera_facing = CArGetFacing();
	camera_position = CArGetLocation();
	MUmVector_Subtract(offset_vector, camera_facing, camera_position);
	MUrNormalize(&offset_vector);
	MUmVector_Scale(offset_vector, 5.0f);
	offset_vector.y = 0.0f;*/
	
	// get the number of objects that are about to be duplicated
	num_objects = OBJrSelectedObjects_GetNumSelected();
	
	// create the memory arrays
	sol_mem_array = (OBJtObject**)UUrMemory_Block_New(sizeof(OBJtObject*) * num_objects);
	if (sol_mem_array == NULL) { goto cleanup; }
	
	nol_mem_array = (OBJtObject**)UUrMemory_Block_New(sizeof(OBJtObject*) * num_objects);
	if (nol_mem_array == NULL) { goto cleanup; }
	
	// put the selected object list into a new array
	for (i = 0; i < num_objects; i++)
	{
		sol_mem_array[i] = OBJrSelectedObjects_GetSelectedObject(i);
	}
	
	// duplicate all of the selected objects
	for (i = 0; i < num_objects; i++)
	{
		OBJtObject		*object;
		OBJtOSD_All		osd_all;
		M3tPoint3D		position;
		M3tPoint3D		rotation;
		OBJtObjectType	object_type;
		
		object = sol_mem_array[i];
		object_type = OBJrObject_GetType(object);
		OBJrObject_GetPosition(object, &position, &rotation);
		OBJrObject_GetObjectSpecificData(object, &osd_all);
		
		// flags' id numbers must be unique
		if (object_type == OBJcType_Flag) {
			osd_all.osd.flag_osd.id_number = OBJrFlag_GetUniqueID();
		}
		
		// offset the position in the direction the camera is facing
//		MUmVector_Increment(position, offset_vector);
		
		// create a new object with this data
		OBJrObject_New(
			OBJrObject_GetType(object),
			&position,
			&rotation,
			&osd_all);
		
		// get the newly created object
		nol_mem_array[i] = OBJrSelectedObjects_GetSelectedObject(0);
	}
	
	// select all of the newly created objects
	OBJrSelectedObjects_UnselectAll();
	for (i = 0; i < num_objects; i++)
	{
		OBJrSelectedObjects_Select(nol_mem_array[i], UUcTrue);
	}
	
cleanup:
	if (nol_mem_array)
	{
		UUrMemory_Block_Delete(nol_mem_array);
	}
	
	if (sol_mem_array)
	{
		UUrMemory_Block_Delete(sol_mem_array);
	}
}

// ----------------------------------------------------------------------
static UUtBool
OWiEnumCallback(
	WMtWindow			*inWindow,
	UUtUns32			inUserData)
{
	if ((inWindow != OWgOniWindow) &&
		(WMrWindow_GetVisible(inWindow) == UUcTrue))
	{
		UUtBool			*window_open;
		
		window_open = (UUtBool*)inUserData;
		*window_open = UUcTrue;
	
		return UUcFalse;
	}
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
static UUtBool
OWiEnumCallback_FindFirstWindow(
	WMtWindow			*inWindow,
	UUtUns32			inUserData)
{
	if ((inWindow != OWgOniWindow) &&
		(WMrWindow_GetVisible(inWindow) == UUcTrue))
	{
		WMtWindow		**windowptr;
		
		windowptr = (WMtWindow **)inUserData;
		*windowptr = inWindow;
	
		return UUcFalse;
	}
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
static void
OWiHandleKeyCommands(
	UUtUns32			inParam1)
{
	WMtWindow *window;

	switch (inParam1)
	{
		case LIcKeyCode_Q:	/* quit game */
			WMrMessage_Post(NULL, WMcMessage_Quit, 0, 0);
			break;

		case LIcKeyCode_K:	/* kill all particles - control-K */
			ONrParticle3_KillAll(UUcFalse);
			break;

		case LIcKeyCode_J:	/* recreate all particles - control-J */
			ONrParticle3_KillAll(UUcTrue);
			break;

		case LIcKeyCode_W:	/* close current window */
			window = WMrWindow_GetFocus();
			if ((window == OWgOniWindow) || (window == WMrGetDesktop())) {
				window = NULL;
			} else if (WMrWindow_GetFlags(window) & WMcWindowFlag_Child) {
				window = WMrWindow_GetOwner(window);
			}

			if (window == NULL) {
				// find a window to close
				WMrEnumWindows(OWiEnumCallback_FindFirstWindow, (UUtUns32) &window);
			}

			// really don't close the desktop
			if ((window == OWgOniWindow) || (window == WMrGetDesktop())) {
				window = NULL;
			}

			// close it
			if (window != NULL) {
				WMrMessage_Post(window, WMcMessage_Close, 0, 0);
			}
			break;
	}
}

// ----------------------------------------------------------------------
static void
OWiTranslateKeyDown(
	WMtEvent			*inEvent)
{
	UUtBool				window_open;
	
	// check for windows being open
	window_open = UUcFalse;
	WMrEnumWindows(OWiEnumCallback, (UUtUns32)&window_open);
	if (window_open) { return; }
	
	inEvent->window = NULL;
}

// ----------------------------------------------------------------------
static void
OWiHandleCameraMove(
	UUtUns32			inParam1)
{
	LItAction			action;
	
	if (ONrLevel_GetCurrentLevel() == 0) { return; }
	
	UUrMemory_Clear(&action, sizeof(LItAction));
	
	switch (UUmLowWord(inParam1))
	{
		case LIcKeyCode_8:
			action.analogValues[LIc_ManCam_Move_FB] = 1.0f;
		break;
		
		case LIcKeyCode_5:
			action.analogValues[LIc_ManCam_Move_FB] = -1.0f;
		break;
		
		case LIcKeyCode_4:
			action.analogValues[LIc_ManCam_Pan_LR] = -1.0f;
		break;
		
		case LIcKeyCode_6:
			action.analogValues[LIc_ManCam_Pan_LR] = 1.0f;
		break;
		
		case LIcKeyCode_1:
			action.analogValues[LIc_ManCam_Move_LR] = 1.0f;
		break;
		
		case LIcKeyCode_3:
			action.analogValues[LIc_ManCam_Move_LR] = -1.0f;
		break;
		
		case LIcKeyCode_Minus:
			action.analogValues[LIc_ManCam_Move_UD] = -1.0f;
		break;
		
		case LIcKeyCode_Equals:
		case LIcKeyCode_Plus:
			action.analogValues[LIc_ManCam_Move_UD] = 1.0f;
		break;
		
		case LIcKeyCode_W:
			action.analogValues[LIc_ManCam_Move_FB] = 1.0f;
		break;
		
		case LIcKeyCode_S:
			action.analogValues[LIc_ManCam_Move_FB] = -1.0f;
		break;
		
		case LIcKeyCode_A:
			action.analogValues[LIc_ManCam_Move_LR] = 1.0f;
		break;
		
		case LIcKeyCode_D:
			action.analogValues[LIc_ManCam_Move_LR] = -1.0f;
		break;
		
		case LIcKeyCode_UpArrow:
			action.analogValues[LIc_ManCam_Pan_UD] = 1.0f;
		break;
		
		case LIcKeyCode_DownArrow:
			action.analogValues[LIc_ManCam_Pan_UD] = -1.0f;
		break;
	}
	
	CArUpdate(0, 1, &action);
}

// ----------------------------------------------------------------------
static void
OWiHandleSelectedObjects_SetLock(
	UUtBool						inLock)
{
	UUtUns32					i;
	
	for (i = 0; i < OBJrSelectedObjects_GetNumSelected(); i++)
	{
		OBJtObject					*object;
		
		object = OBJrSelectedObjects_GetSelectedObject(i);
		if (object == NULL) { continue; }
		
		OBJrObject_SetLocked(object, inLock);
	}
}

// ----------------------------------------------------------------------
static void
OWiHandleObjectKeys(
	UUtUns32			inParam1)
{
	UUtError	error;

	if (ONrLevel_GetCurrentLevel() == 0) { return; }
	
	switch (UUmLowWord(inParam1))
	{
		case LIcKeyCode_F1:
		case LIcKeyCode_7:
			error = OWrTools_CreateObject(OBJcType_Furniture, NULL);
			if (error == UUcError_None) {
				OWrProp_Display(NULL);
			}
		break;
		
		case LIcKeyCode_F2:
		case LIcKeyCode_Slash:
			OBJrObjectType_SetVisible(OBJcType_Flag, UUcTrue);
			error = OWrTools_CreateObject(OBJcType_Flag, NULL);
			if (error == UUcError_None) {
				OWrProp_Display(NULL);
			}
		break;
		
		case LIcKeyCode_F3:
			OBJrObjectType_SetVisible(OBJcType_Trigger, UUcTrue);
			error = OWrTools_CreateObject(OBJcType_Trigger, NULL);
			if (error == UUcError_None) {
				OWrProp_Display(NULL);
			}
		break;
		
		case LIcKeyCode_F4:
			OBJrObjectType_SetVisible(OBJcType_Turret, UUcTrue);
			error = OWrTools_CreateObject(OBJcType_Turret, NULL);
			if (error == UUcError_None) {
				OWrProp_Display(NULL);
			}
		break;
		
		case LIcKeyCode_F5:
			OBJrDoor_JumpToNextDoor();
		break;
		
		case LIcKeyCode_F6:
			OBJrDoor_JumpToPrevDoor();
		break;
		
		case LIcKeyCode_F7:
			OWrProp_Display(NULL);
		break;
		
		case LIcKeyCode_F8:
		break;
		
		case LIcKeyCode_F9:
			OWrEOPos_Display();
		break;
		
		case LIcKeyCode_F10:
			// focus the manual camera on the selected object
			if (OBJrSelectedObjects_GetNumSelected() == 1)
			{
				OBJtObject			*object;
				M3tPoint3D			position;
				LItAction			action;
				
				object = OBJrSelectedObjects_GetSelectedObject(0);
				OBJrObject_GetPosition(object, &position, NULL);
				CArManual_LookAt(&position, 50.0f);
				
				UUrMemory_Clear(&action, sizeof(LItAction));
				CArUpdate(0, 1, &action);
			}
		break;
		
		case LIcKeyCode_F11:
			OWiDuplicateSelectedObjects();
		break;
	}
}

// ----------------------------------------------------------------------
static void
OWiHandleKeyDown(
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	UUtUns32			mode;
	
	switch (UUmLowWord(inParam1))
	{
		case LIcKeyCode_BackSpace:
		case LIcKeyCode_Delete:
			OBJrSelectedObjects_Delete();
		break;
		
		case LIcKeyCode_Escape:
		case LIcKeyCode_Star:
		case LIcKeyCode_Multiply:
			if (OWgRunStartup == UUcFalse)
			{
				OWrOniWindow_Toggle();
			}
		break;
		
		case LIcKeyCode_8:
		case LIcKeyCode_5:
		case LIcKeyCode_4:
		case LIcKeyCode_6:
		case LIcKeyCode_1:
		case LIcKeyCode_3:
		case LIcKeyCode_Minus:
		case LIcKeyCode_Equals:
		case LIcKeyCode_Plus:
		case LIcKeyCode_W:
		case LIcKeyCode_S:
		case LIcKeyCode_A:
		case LIcKeyCode_D:
		case LIcKeyCode_UpArrow:
		case LIcKeyCode_DownArrow:
			OWiHandleCameraMove(inParam1);
		break;
		
		case LIcKeyCode_Slash:
		case LIcKeyCode_7:
		case LIcKeyCode_F1:
		case LIcKeyCode_F2:
		case LIcKeyCode_F3:
		case LIcKeyCode_F4:
		case LIcKeyCode_F5:
		case LIcKeyCode_F6:
		case LIcKeyCode_F7:
		case LIcKeyCode_F8:
		case LIcKeyCode_F9:
		case LIcKeyCode_F10:
		case LIcKeyCode_F11:
			OWiHandleObjectKeys(inParam1);
		break;
		
		case LIcKeyCode_Space:
			if (OBJrSelectedObjects_GetNumSelected() != 0)
			{
				OBJrSelectedObjects_Lock_Set(!OBJrSelectedObjects_Lock_Get());
			}
		break;
		
		case LIcKeyCode_Tab:
			mode = OBJrRotateMode_Get();
			if (mode == OBJcRotateMode_XYZ)
			{
				OBJrRotateMode_Set(OBJcRotateMode_XZ);
			}
			else if (mode == OBJcRotateMode_XZ)
			{
				OBJrRotateMode_Set(OBJcRotateMode_XY);
			}
			else if (mode == OBJcRotateMode_XY)
			{
				OBJrRotateMode_Set(OBJcRotateMode_YZ);
			}
			else if (mode == OBJcRotateMode_YZ)
			{
				OBJrRotateMode_Set(OBJcRotateMode_XYZ);
			}
		break;
		
		case LIcKeyCode_Q:
			mode = OBJrMoveMode_Get();
			if (mode == OBJcMoveMode_LR)
			{
				OBJrMoveMode_Set(OBJcMoveMode_FB);
			}
			else if (mode == OBJcMoveMode_FB)
			{
				OBJrMoveMode_Set(OBJcMoveMode_UD);
			}
			else if (mode == OBJcMoveMode_UD)
			{
				OBJrMoveMode_Set(OBJcMoveMode_LR | OBJcMoveMode_FB);
			}
			else if (mode == (OBJcMoveMode_LR | OBJcMoveMode_FB))
			{
				OBJrMoveMode_Set(OBJcMoveMode_LR);
			}

			if (mode == (OBJcMoveMode_Wall | OBJcMoveMode_LR))
			{
				OBJrMoveMode_Set(OBJcMoveMode_Wall | OBJcMoveMode_UD);
			}
			else if (mode == (OBJcMoveMode_Wall | OBJcMoveMode_UD))
			{
				OBJrMoveMode_Set(OBJcMoveMode_Wall | OBJcMoveMode_UD | OBJcMoveMode_LR);
			}
			else if (mode == (OBJcMoveMode_Wall | OBJcMoveMode_UD | OBJcMoveMode_LR))
			{
				OBJrMoveMode_Set(OBJcMoveMode_Wall | OBJcMoveMode_LR);
			}
		break;
		
		case LIcKeyCode_9:
			OWiHandleSelectedObjects_SetLock(UUcTrue);
		break;
		
		case LIcKeyCode_0:
			OWiHandleSelectedObjects_SetLock(UUcFalse);
		break;
	}
}

// ----------------------------------------------------------------------
static void
OWiHandleMouseMoveEvent(
	WMtMessage			inMessage,
	IMtPoint2D			*inMousePos,
	UUtUns32			inParam2)
{
	OBJtObject			*object;
	UUtUns32			i;
	
	// don't process clicks outside of a window
	if (OWrEOPos_IsVisible() || OWrProp_IsVisible() || OWrObjNew_IsVisible()) { return; }
	
	switch (inMessage)
	{
		case WMcMessage_MouseMove:
			
			if ((OWiIsLockedInSelection() == UUcFalse) &&
				(OWgMoveDelta < UUrMachineTime_Sixtieths()))
			{
				UUtUns32	num_objects;
				UUtUns32	move_mode;
				
				move_mode = OBJrMoveMode_Get();
				
				object = OBJrSelectedObjects_GetSelectedObject(0);
				if ((object != NULL) && (OBJrObject_GetType(object) == OBJcType_Particle) &&
					((OBJtOSD_Particle *) object->object_data)->is_decal) {
					// use special decal-object movement modes
					if (inParam2 & LIcMouseState_ShiftDown) {
						// slide up and down along the wall
						OBJrMoveMode_Set(OBJcMoveMode_Wall | OBJcMoveMode_UD);

					} else if (inParam2 & LIcMouseState_ControlDown) {
						// slide left and right along the wall
						OBJrMoveMode_Set(OBJcMoveMode_Wall | OBJcMoveMode_LR);

					} else {
						// slide along the wall's surface
						OBJrMoveMode_Set(OBJcMoveMode_Wall | OBJcMoveMode_LR | OBJcMoveMode_UD);
					}

				} else {
					if (inParam2 & LIcMouseState_ShiftDown)
					{
						OBJrMoveMode_Set(OBJcMoveMode_UD);
					}
				}

				// move the objects
				num_objects = OBJrSelectedObjects_GetNumSelected();
				for (i = 0; i < num_objects; i++)
				{
					object = OBJrSelectedObjects_GetSelectedObject(i);
					
					OBJrObject_MouseMove(
						object,
						&OWgPreviousMousePos,
						inMousePos); 
				}
				
				OBJrMoveMode_Set(move_mode);
			}
		break;
	
		case WMcMessage_LMouseDown:
			OWgMoveDelta = UUrMachineTime_Sixtieths() + OWcMoveDelta;
			
			object = OBJrGetObjectUnderPoint(inMousePos);
			if (object == NULL)
			{
				// the user didn't click any objects so unselect them all
				OBJrSelectedObjects_UnselectAll();

				// CB: the user has clicked on the desktop. set the keyboard focus to the
				// top-level window so that the menu bar gets keystrokes.
				// this is useful because then we can hit esc to hide the menu bar
				// even if there are windows (e.g. particle editing windows) open
				WMrWindow_SetFocus(OWgOniWindow);
			}
			else
			{
				// the user is clicking on an object
				if ((inParam2 & LIcMouseState_ControlDown) != 0)
				{
					// the control key is down so the user is either
					// a - clicking on an object that is already in the selected objects
					//		list in order to unselect it.
					// b - clicking on an object that is not in the selected objects list
					// 		in order to select it
					if (OBJrSelectedObjects_IsObjectSelected(object) == UUcTrue)
					{
						OBJrSelectedObjects_Unselect(object);
					}
					else
					{
						OBJrSelectedObjects_Select(object, UUcTrue);
					}
				}
				else
				{
					// the control is not down so the user is either
					// a - clicking on a new object to select it exclusively
					// b - clicking on an object that is already in the selected objects
					// 		list in order to move it
					// if b, don't do anything
					if (OBJrSelectedObjects_IsObjectSelected(object) == UUcFalse)
					{
						OBJrSelectedObjects_UnselectAll();
						OBJrSelectedObjects_Select(object, UUcFalse);
					}
				}
			}
		break;
		
		case WMcMessage_LMouseDblClck:
			OWrProp_Display(NULL);
		break;
		
		case WMcMessage_LMouseUp:
		break;
	}
}

// ----------------------------------------------------------------------
static void
OWiHandleMouseRotateEvent(
	WMtMessage			inMessage,
	IMtPoint2D			*inMousePos,
	UUtUns32			inParam2)
{
	OBJtObject			*object;
	OBJtObject			*selected;
	UUtUns32			num_selected;
	
	// get the currently selected object if there is only one
	num_selected = OBJrSelectedObjects_GetNumSelected();
	if (num_selected == 1)
	{
		selected = OBJrSelectedObjects_GetSelectedObject(0);
	}
	else
	{
		selected = NULL;
	}
	
	switch (inMessage)
	{
		case WMcMessage_MouseMove:
			// rotate the selected object
			if (selected != NULL)
			{
				IMtPoint2D		delta;
				
				// calculate the mouse movement delta
				delta.x = inMousePos->x - OWgPreviousMousePos.x;
				delta.y = inMousePos->y - OWgPreviousMousePos.y;
				
				// rotate the objects
				OBJrObject_MouseRotate_Update(selected, &delta);
			}
		break;
		
		case WMcMessage_RMouseDown:
			// get the object under the point
			object = OBJrGetObjectUnderPoint(inMousePos);
			
			if (selected != NULL)
			{
				if (object == selected)
				{
					// set any additional modifiers based on object type
					switch (OBJrObject_GetType(object))
					{
						case OBJcType_Flag:
							OBJrRotateMode_Set(OBJcRotateMode_XZ);
						break;

						case OBJcType_Particle:
							if (((OBJtOSD_Particle *) object->object_data)->is_decal) {
								OBJrRotateMode_Set(OBJcRotateMode_XZ);
							}
						break;
					}

					// begin the rotation
					OBJrObject_MouseRotate_Begin(object, inMousePos);
				}
				else
				{
					OBJrSelectedObjects_UnselectAll();
				}
			}
			else if (object != NULL)
			{
				// there are no objects selected and the user has clicked on an object
				// so simply select the object being clicked on
				OBJrSelectedObjects_Select(object, UUcFalse);
			}
		break;
		
		case WMcMessage_RMouseDblClck:
			OBJrObject_MouseRotate_End();
			OWrEOPos_Display();
		break;
		
		case WMcMessage_RMouseUp:
			OBJrObject_MouseRotate_End();
		break;
	}
}

// ----------------------------------------------------------------------
static void
OWiHandleMouseEvent(
	WMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	IMtPoint2D			global_mouse;
	
	// get the global mouse location
	global_mouse.x = UUmHighWord(inParam1);
	global_mouse.y = UUmLowWord(inParam1);
	
	switch(inMessage)
	{
		case WMcMessage_MouseMove:
			// rotate or move depending on which mouse button is down
			if ((inParam2 & LIcMouseState_RButtonDown) != 0)
			{
				OWiHandleMouseRotateEvent(inMessage, &global_mouse, inParam2);
			}
			else if ((inParam2 & LIcMouseState_LButtonDown) != 0)
			{
				OWiHandleMouseMoveEvent(inMessage, &global_mouse, inParam2);
			}

			// save the previous mouse position
			OWgPreviousMousePos = global_mouse;
			OBJgMouseMoved = UUcTrue;
		break;
		
		case WMcMessage_LMouseDown:
		case WMcMessage_LMouseUp:
		case WMcMessage_LMouseDblClck:
			OWiHandleMouseMoveEvent(inMessage, &global_mouse, inParam2);
			OBJgMouseMoved = UUcFalse;
		break;
		
		case WMcMessage_RMouseDown:
		case WMcMessage_RMouseDblClck:
		case WMcMessage_RMouseUp:
			OWiHandleMouseRotateEvent(inMessage, &global_mouse, inParam2);
			OBJgMouseMoved = UUcFalse;
		break;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
OWrInitialize(
	void)
{
	UUtError				error;
	WMtWindowClass			window_class;
	WMtWindow				*desktop;
	UUtInt16				width;
	UUtInt16				height;
	
	// ------------------------------
	// startup the window manager
	// ------------------------------
	error = WMrStartup();
	UUmError_ReturnOnError(error);
	
	// ------------------------------
	// register the windows
	// ------------------------------
	// register the window class
	UUrMemory_Clear(&window_class, sizeof(WMtWindowClass));
	window_class.type = OWcWindowType_Oni;
	window_class.callback = OWiOniWindow_Callback;
	window_class.private_data_size = 4;
	
	error = WMrWindowClass_Register(&window_class);
	UUmError_ReturnOnError(error);
	
	// get the size of the desktop window
	desktop = WMrGetDesktop();
	if (desktop == NULL) { return UUcError_Generic; }
	
	WMrWindow_GetSize(desktop, &width, &height);
	
	// create the oni window
	OWgOniWindow =
		WMrWindow_New(
			OWcWindowType_Oni,
			"Oni",
			WMcWindowFlag_Visible | WMcWindowFlag_TopMost | WMcWindowFlag_MouseTransparent,
			WMcWindowStyle_None,
			0,
			0,
			0,
			width,
			height,
			NULL,
			0);
	if (OWgOniWindow == NULL)
	{
		return UUcError_Generic;
	}
	
	// ------------------------------
	OBJgShowStatus = UUcTrue;

#if CONSOLE_DEBUGGING_COMMANDS
	error =
		SLrGlobalVariable_Register_Bool(
			"obj_tool_show_status",
			"Displays the object tool status bar",
			&OBJgShowStatus);
	UUmError_ReturnOnError(error);
#endif
	
	// ------------------------------
	// initialize the tools
	// ------------------------------
	error = OWrTools_Initialize();
	UUmError_ReturnOnError(error);
	
	// ------------------------------
	// Initialize the settings dialogs
	// ------------------------------
	error = OWrSettings_Initialize();
	UUmError_ReturnOnError(error);

#if TOOL_VERSION
	// ------------------------------
	// Initialize the particle dialogs
	// ------------------------------
	error = OWrParticle_Initialize();
	UUmError_ReturnOnError(error);

	// ------------------------------
	// Initialize the AI dialogs
	// ------------------------------
	error = OWrAI_Initialize();
	UUmError_ReturnOnError(error);
#endif

	// ------------------------------
	// initialize the keyboard commands
	// ------------------------------
	OWgKeyboardCommands.num_commands = 4;
	OWgKeyboardCommands.commands[0] = LIcKeyCode_Q;
	OWgKeyboardCommands.commands[1] = LIcKeyCode_W;
	OWgKeyboardCommands.commands[2] = LIcKeyCode_K;
	OWgKeyboardCommands.commands[3] = LIcKeyCode_J;
	
	// ------------------------------
	OWgPreviousMousePos.x = 0;
	OWgPreviousMousePos.y = 0;
	
	// ------------------------------
	OBJrMoveMode_Set(OBJcMoveMode_LR | OBJcMoveMode_FB);
	OBJrRotateMode_Set(OBJcRotateMode_XYZ);
	OBJrSelectedObjects_Lock_Set(UUcFalse);
		
	return UUcError_None;
}

// S.S.
/*UUtBool OWrWindow_Resize(
	int new_width,
	int new_height)
{
	UUtBool success= UUcFalse;
	WMtWindow *desktop= WMrGetDesktop();
	
	if (OWgOniWindow) WMrWindow_SetSize(OWgOniWindow, new_width, new_height);
	if (desktop) WMrWindow_SetSize(desktop, new_width, new_height);
	success= (ONrMotoko_SetupDrawing(&ONgPlatformData) == UUcError_None) ? UUcTrue : UUcFalse;

	return success;
}*/

// ----------------------------------------------------------------------
void
OWrTerminate(
	void)
{
#if TOOL_VERSION
	OWrSound2_Terminate();
#endif
}

// ----------------------------------------------------------------------
UUtError
OWrLevelBegin(
	void)
{
#if TOOL_VERSION
	UUtError error;

	error = OWrAI_LevelBegin();
	UUmError_ReturnOnError(error);
#endif

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OWrLevelEnd(
	void)
{
}
	
// ----------------------------------------------------------------------
void
OWrUpdate(
	void)
{
	WMtEvent			event;
	
	while (WMrMessage_Get(&event))
	{
		UUtUns32		handled;
		
		WMrMessage_TranslateKeyCommand(&event, &OWgKeyboardCommands, NULL);
		if (event.message == WMcMessage_KeyDown)
		{
			OWiTranslateKeyDown(&event);
		}
		
		handled = WMrMessage_Dispatch(&event);
		if (handled == WMcResult_NoWindow)
		{
			switch (event.message)
			{
				case WMcMessage_KeyCommand:
					OWiHandleKeyCommands(event.param1);
				break;
				
				case WMcMessage_Quit:
					if (OWiOKToQuit()) {
						ONgTerminateGame = UUcTrue;
					}
				break;
				
				case WMcMessage_MouseMove:
				case WMcMessage_LMouseDown:
				case WMcMessage_LMouseUp:
				case WMcMessage_LMouseDblClck:
				case WMcMessage_RMouseDown:
				case WMcMessage_RMouseUp:
				case WMcMessage_RMouseDblClck:
					OWiHandleMouseEvent(event.message, event.param1, event.param2);
				break;
				
				case WMcMessage_KeyDown:
					OWiHandleKeyDown(event.param1, event.param2);
				break;
				
				// Oni defined messages
				case OWcMessage_RunGame:
					OWrOniWindow_Toggle();
					OWgRunStartup = UUcFalse;
				break;
			}
		}
	}
}

typedef struct OWtGetMask_Internals
{
	UUtUns32 inMask;
	const char **inFlagList;
	UUtUns32 outMask;
} OWtGetMask_Internals;

static UUtBool
OWrGetMask_Callback(
	WMtDialog			*inDialog,
	WMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	UUtBool handled = UUcFalse;
	OWtGetMask_Internals *internals = (OWtGetMask_Internals *) WMrDialog_GetUserData(inDialog);
	enum
	{
		checkbox_base = 100,
		checkbox_count = 16
	};

	switch (inMessage)
	{
		case WMcMessage_InitDialog:
		{
			const char **current_string = internals->inFlagList;
			UUtUns16 itr;
			UUtUns16 count = 0;
			UUtUns16 width;
			UUtUns16 height;
			WMtWindow *scratch;
			UUtRect client_rect;

			for(itr = 0; itr < checkbox_count; itr++) {
				UUtBool valid = (*current_string != NULL);

				scratch = WMrDialog_GetItemByID(inDialog, itr + checkbox_base);
				WMrWindow_SetVisible(scratch, valid);

				if (valid) {
					UUtBool checkbox_set = (internals->inMask & (1 << itr)) > 0;

					WMrWindow_SetTitle(scratch, *current_string, WMcMaxTitleLength);
					WMrCheckBox_SetCheck(scratch, checkbox_set);
				}				

				if (valid) {
					count++;
					current_string++;
				}
			}

			width = 300;
			height = (count * 25) + 35;
			WMrWindow_SetPosition(inDialog, NULL, 0, 0, width, height, WMcPosChangeFlag_NoMove | WMcPosChangeFlag_NoZOrder);
			
			WMrWindow_GetClientRect(inDialog, &client_rect);

			WMrWindow_SetPosition(inDialog, NULL, 0, 0, 
				width + (width - client_rect.right + client_rect.left), 
				height + (height - client_rect.bottom + client_rect.top),
				WMcPosChangeFlag_NoMove | WMcPosChangeFlag_NoZOrder);

			scratch = WMrDialog_GetItemByID(inDialog, 1);
			WMrWindow_SetPosition(scratch, NULL, width - 80, height - 30, 0, 0, WMcPosChangeFlag_NoSize | WMcPosChangeFlag_NoZOrder);
			
			scratch = WMrDialog_GetItemByID(inDialog, 2);
			WMrWindow_SetPosition(scratch, NULL, width - 160, height - 30, 0, 0, WMcPosChangeFlag_NoSize | WMcPosChangeFlag_NoZOrder);			
		}
		break;

		case WMcMessage_Command:
			switch(UUmLowWord(inParam1)) 
			{
				case WMcDialogItem_OK:
					WMrDialog_ModalEnd(inDialog, 0);
				break;

				case WMcDialogItem_Cancel:
					internals->outMask = internals->inMask;	// revert
					WMrDialog_ModalEnd(inDialog, 0);
				break;

				default:
					if ((UUmLowWord(inParam1) - checkbox_base) < checkbox_count) {
						UUtUns16 dialog_item_id = UUmLowWord(inParam1);
						UUtUns16 item_index = dialog_item_id - checkbox_base;
						UUtUns32 mask = 1 << item_index;
						WMtCheckBox *checkbox = WMrDialog_GetItemByID(inDialog, dialog_item_id);
						UUtBool checked = WMrCheckBox_GetCheck(checkbox);

						if (checked) {
							internals->outMask |= mask;
						}
						else {
							internals->outMask &= ~mask;
						}

						handled = UUcTrue;
					}
					else {
						handled = UUcFalse;
					}
				break;
			}
		break;

		default:
			handled = UUcFalse;
	}

	return handled;
}

UUtUns32 OWrGetMask(UUtUns32 inMask, const char **inFlagList)
{
	OWtGetMask_Internals internals;

	internals.inMask = inMask;
	internals.inFlagList = inFlagList;
	internals.outMask = inMask;
	
	WMrDialog_ModalBegin(OWcDialog_GetMask, NULL, OWrGetMask_Callback, (UUtUns32) &internals, NULL);

	return internals.outMask;
}

void OWrMaskToString(UUtUns32 inMask, const char **inFlagList, UUtUns32 inStringLength, char *outString)
{
	const char **current_flag;
	UUtUns32 remaining_string_length = inStringLength - 1;
	char *current_target_string = outString;
	UUtUns32 itr;

	UUmAssert(0 == strlen(""));

	for(itr = 0, current_flag = inFlagList; (*current_flag != NULL) && (remaining_string_length > 0); itr++,current_flag++)
	{
		if ((1 << itr) & (inMask)) {
			UUtUns32 current_string_length = strlen(*current_flag);
			UUtUns32 current_amount_to_copy = UUmMin(current_string_length, remaining_string_length);

			UUrMemory_MoveFast(*current_flag, current_target_string, current_amount_to_copy);
			
			remaining_string_length -= current_amount_to_copy;
			current_target_string += current_amount_to_copy;

			if (remaining_string_length > 0) {
				*current_target_string++ = ' ';
				remaining_string_length--;
			}
		}
	}

	*current_target_string = '\0';

	return;
}

typedef struct visibility_match
{
	UUtUns16 menu_item_id;
	OBJtObjectType object_type;
} visibility_match;

const visibility_match visibility_match_list[] = 
{
	{ OWcMenu_Obj_ShowCharacters, OBJcType_Character },
	{ OWcMenu_Obj_ShowPatrols, OBJcType_PatrolPath },
	{ OWcMenu_Obj_ShowTurrets, OBJcType_Turret },
	{ OWcMenu_Obj_ShowConsoles, OBJcType_Console },
	{ OWcMenu_Obj_ShowParticles, OBJcType_Particle },
	{ OWcMenu_Obj_ShowDoors, OBJcType_Door },
	{ OWcMenu_Obj_ShowPowerUps, OBJcType_PowerUp },
	{ OWcMenu_Obj_ShowWeapons, OBJcType_Weapon },
	{ OWcMenu_Obj_ShowTriggerVolumes, OBJcType_TriggerVolume },

	{ OWcMenu_Obj_ShowFlags, OBJcType_Flag },
	{ OWcMenu_Obj_ShowFurniture, OBJcType_Furniture },
	{ OWcMenu_Obj_ShowTriggers, OBJcType_Trigger },
	{ OWcMenu_Obj_ShowSounds, OBJcType_Sound },

	{ 0, 0 }
};

UUtBool OWrHandleObjectVisibilityMenu(WMtMenu *menu, UUtUns16 menu_item_id)
{
	const visibility_match *cur_match;

	UUtBool handled;

	for(cur_match = visibility_match_list; cur_match->object_type != 0; cur_match++)
	{
		if (cur_match->menu_item_id == menu_item_id)
		{
			UUtUns16 flags;

			WMrMenu_GetItemFlags(menu, menu_item_id, &flags);

			{
				UUtBool new_visibility = !(flags & WMcMenuItemFlag_Checked);
				
				OBJrObjectType_SetVisible(cur_match->object_type, new_visibility);
			}

			handled = UUcTrue;
			break;
		}
	}

	return handled;
}

void OWrBuildVisibilityMenu(WMtMenu *menu)
{
	const visibility_match *cur_menu_item;

	for(cur_menu_item = visibility_match_list; cur_menu_item->object_type != 0; cur_menu_item++)
	{
		WMrMenu_CheckItem(menu, cur_menu_item->menu_item_id, OBJrObjectType_GetVisible(cur_menu_item->object_type));
	}

	return;
}
#endif