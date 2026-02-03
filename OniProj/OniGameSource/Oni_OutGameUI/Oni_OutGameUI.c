// ======================================================================
// Oni_OutGameUI.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_WindowManager.h"
#include "BFW_SoundSystem2.h"
#include "BFW_Timer.h"
#include "Motoko_Manager.h"

#include "WM_CheckBox.h"
#include "WM_PopupMenu.h"

#include "Oni_GameState.h"
#include "Oni_OutGameUI.h"
#include "Oni_Motoko.h"
#include "Oni_Sound2.h"
#include "Oni_Windows.h"
#include "Oni_Persistance.h"
#include "Oni_Level.h"


// ======================================================================
// defines
// ======================================================================
#define ONcMainMusic_PlayLength		(20 * 60)	/* 20 seconds */
#define OScMainMusic_StoppingLength (2 * 60)	/* 2 seconds */

#define OScMainMusic_FadeInTime		(0.5f)		/* in seconds */
#define OScMainMusic_FadeOutTime	(0.5f)		/* in seconds */

// ======================================================================
// enums
// ======================================================================
enum
{
	ONcOGU_MainMenuID				= 150,
	ONcOGU_LoadGameID				= 151,
	ONcOGU_OptionsID_PC				= 152,
	ONcOGU_QuitYesNoID				= 153,
	ONcOGU_NewGameID				= 154,
//	ONcOGU_LevelLoadID				= 155,	in Oni_Window.h
	ONcOGU_ChangeRestart			= 156,
	ONcOGU_OptionsID_Mac			= 157
};


enum
{
	ONcOptions_Sldr_Quality			= 100,
	ONcOptions_PM_Resolution		= 101,
	ONcOptions_Sldr_OverallVol		= 102,
//	ONcOptions_CB_DialogOn			= 103,
//	ONcOptions_Sldr_DialogVol		= 104,
//	ONcOptions_CB_MusicOn			= 105,
//	ONcOptions_Sldr_MusicVol		= 106,
	ONcOptions_CB_SubtitlesOn		= 107,
	ONcOptions_PM_Difficulty		= 108,
	ONcOptions_CB_InvertMouseOn		= 109,
	ONcOptions_Sldr_Gamma			= 110
};

enum
{
	ONcMainMenu_Btn_NewGame			= 100,
	ONcMainMenu_Btn_LoadGame		= 101,
	ONcMainMenu_Btn_Options			= 102,
	ONcMainMenu_Btn_Quit			= 103,
	ONcMainMenu_Btn_Resume			= 104
};

enum
{
	ONcLoadGame_LB_Levels			= 100,
	ONcLoadGame_Btn_Load			= WMcDialogItem_OK,
	ONcLoadGame_Btn_Cancel			= WMcDialogItem_Cancel
};

enum
{
	ONcNewGame_Btn_Yes				= WMcDialogItem_OK,
	ONcNewGame_Btn_No				= WMcDialogItem_Cancel
};

enum
{
	ONcQuitYesNo_Btn_Yes			= WMcDialogItem_OK,
	ONcQuitYesNo_Btn_No				= WMcDialogItem_Cancel
};

enum
{
	ONcChangeRestart_Btn_OK			= WMcDialogItem_OK
};

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
ONiOGU_ChangeRestart_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool					handled;

	handled = UUcTrue;

	switch (inMessage)
	{
		case WMcMessage_Command:
			if (UUmHighWord(inParam1) != WMcNotify_Click) { break; }
			WMrDialog_ModalEnd(inDialog, UUmLowWord(inParam1));
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}

// ----------------------------------------------------------------------
static UUtUns32
ONiOutGameUI_ChangeRestart_Display(
	void)
{
	PStPartSpecUI			*partspec_ui;
	PStPartSpecUI			*temp_ui;
	UUtUns32				message;

	// save the current ui
	partspec_ui = PSrPartSpecUI_GetActive();

	// set the ui to the out of game ui
	temp_ui = PSrPartSpecUI_GetByName(ONcOutGameUIName);
	if (temp_ui != NULL) { PSrPartSpecUI_SetActive(temp_ui); }

	// display the dialog
	WMrDialog_ModalBegin(
		ONcOGU_ChangeRestart,
		NULL,
		ONiOGU_ChangeRestart_Callback,
		0,
		&message);

	// reset the active ui
	PSrPartSpecUI_SetActive(partspec_ui);

	return message;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
ONiOGU_QuitYesNo_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool					handled;

	handled = UUcTrue;

	switch (inMessage)
	{
		case WMcMessage_Command:
			if (UUmHighWord(inParam1) != WMcNotify_Click) { break; }
			WMrDialog_ModalEnd(inDialog, UUmLowWord(inParam1));
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}

// ----------------------------------------------------------------------
UUtUns32
ONrOutGameUI_QuitYesNo_Display(
	void)
{
	PStPartSpecUI			*partspec_ui;
	PStPartSpecUI			*temp_ui;
	UUtUns32				message;

	// save the current ui
	partspec_ui = PSrPartSpecUI_GetActive();

	// set the ui to the out of game ui
	temp_ui = PSrPartSpecUI_GetByName(ONcOutGameUIName);
	if (temp_ui != NULL) { PSrPartSpecUI_SetActive(temp_ui); }

	// display the dialog
	WMrDialog_ModalBegin(
		ONcOGU_QuitYesNoID,
		NULL,
		ONiOGU_QuitYesNo_Callback,
		0,
		&message);

	// reset the active ui
	PSrPartSpecUI_SetActive(partspec_ui);

	return message;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
ONiResolution_Switch(
	WMtDialog				*inOptionsDialog,
	WMtWindow				*inPopupMenu,
	UUtUns16				inMode)
{
	M3tDrawEngineCaps		*current_draw_engine_caps;
	UUtUns16				activeDrawEngine;
	UUtUns16				activeDevice;
	UUtUns16				activeMode;
	M3tDisplayMode			new_resolution;
	UUtBool					restart, voodoo_fullscreen, s3_crappy_card_fullscreen;
	UUtBool					osx= UUcFalse;

	extern UUtBool gl_voodoo_card_full_screen(void); // gl_utility.c
	extern UUtBool gl_s3_crappy_card_full_screen(void); // gl_utility.c

	// get the index of the active draw engine
	M3rManager_GetActiveDrawEngine(&activeDrawEngine, &activeDevice, &activeMode);
	if (inMode == activeMode) { return; }

	// get a pointer to the current draw engine's caps
	current_draw_engine_caps = M3rDrawEngine_GetCaps(activeDrawEngine);
	if (current_draw_engine_caps == NULL) { return; }

        new_resolution = current_draw_engine_caps->displayDevices[activeDevice].displayModes[inMode];

	restart = UUcFalse;

	voodoo_fullscreen= gl_voodoo_card_full_screen();
	s3_crappy_card_fullscreen= gl_s3_crappy_card_full_screen();
#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Mac)
	{
		// OSX doesn't handle res changes properly (if at all) as of OS X public beta
		// so we force a restart
		OSErr err;
		unsigned long value;

		err= Gestalt(gestaltSystemVersion, &value);
		if (err == noErr)
		{
			unsigned long major_version;

			// value will look like this: 0x00000904 (OS 9.0.4)
			major_version= (value & 0x0000FF00)>>8;
			if (major_version >= 10)
			{
				osx= UUcTrue;
			}
		}
	}
#endif

	if (voodoo_fullscreen || s3_crappy_card_fullscreen || osx)
	{
                restart = UUcTrue;
	}

	if ((ONrLevel_GetCurrentLevel() == 0) &&
		(ONrMotoko_SetResolution(&new_resolution) == UUcTrue) &&
		(ONrMotoko_SetupDrawing(&ONgPlatformData) == UUcError_None))
	{
		// update the window manager
		WMrSetResolution(new_resolution.width, new_resolution.height);
	}
	else
	{
		restart = UUcTrue;
	}
	ONrPersist_SetResolution(&new_resolution);

	// 3Dfx cards don't handle resolution switching correctly in full-screen mode
	// and after doing so the out-of-game UI will not be visible. Diito for S3
	if (voodoo_fullscreen || s3_crappy_card_fullscreen || osx)
	{
		extern void OniExit(void); // Oni.c

		// alert won't be visible afterwards
		AUrMessageBox(AUcMBType_OK, "You must restart Oni for your changes to take effect; Oni will now exit.");
		OniExit();
		exit(0);
	}

	if (restart == UUcTrue)
	{
		// tell the user they have to restart the game
		ONiOutGameUI_ChangeRestart_Display();
	}
}

/*
static void
OWiResolution_Switch(
	WMtDialog				*inOptionsDialog,
	WMtWindow				*inPopupMenu,
	UUtUns16				inMode)
{
	M3tDrawEngineCaps		*current_draw_engine_caps;
	UUtUns16				activeDrawEngine;
	UUtUns16				activeDevice;
	UUtUns16				activeMode;

	// get the index of the active draw engine
	M3rManager_GetActiveDrawEngine(&activeDrawEngine, &activeDevice, &activeMode);
	if (inMode == activeMode) { return; }

	// get a pointer to the current draw engine's caps
	current_draw_engine_caps = M3rDrawEngine_GetCaps(activeDrawEngine);
	if (current_draw_engine_caps == NULL) { return; }

	{
		UUtInt16 dialog_width, dialog_height;

		if (WMrWindow_GetSize(inOptionsDialog, &dialog_width, &dialog_height))
		{
			M3tDisplayMode new_resolution;
			UUtInt16 main_menu_width, main_menu_height;
			UUtUns16 options_x, options_y;
			UUtUns16 main_menu_x, main_menu_y;

			new_resolution.bit_depth = current_draw_engine_caps->displayDevices[activeDevice].displayModes[inMode].bitDepth;
			new_resolution.width = current_draw_engine_caps->displayDevices[activeDevice].displayModes[inMode].width;
			new_resolution.height = current_draw_engine_caps->displayDevices[activeDevice].displayModes[inMode].height;

			// center options dialog in main window
			options_x= UUmMax(((new_resolution.width - dialog_width)/2), 0);
			options_y= UUmMax(((new_resolution.height - dialog_height)/2), 0);

			// center main menu in main window
			if ((main_game_menu != NULL) &&
				WMrWindow_GetSize(main_game_menu, &main_menu_width, &main_menu_height))
			{

				main_menu_x= UUmMax(((new_resolution.width - main_menu_width)/2), 0);
				main_menu_y= UUmMax(((new_resolution.height - main_menu_height)/2), 0);
			}

			if (ONrMotoko_SetResolution(&new_resolution) &&
				OWrWindow_Resize(new_resolution.width, new_resolution.height) &&
				WMrWindow_SetPosition(inOptionsDialog, NULL,
					options_x, options_y, dialog_width, dialog_height, WMcPosChangeFlag_NoZOrder) &&
				((main_game_menu == NULL) ? UUcTrue :
					WMrWindow_SetPosition(main_game_menu, NULL,
						main_menu_x, main_menu_y,
						new_resolution.width, new_resolution.height,
						WMcPosChangeFlag_NoZOrder)))
			{
				ONrPersist_SetResolution(&new_resolution);
			}
			else
			{
				// many older cards don't support res-switching on the fly
				ONrPersist_SetResolution(&new_resolution);
				AUrMessageBox(AUcMBType_OK, "You must restart Oni for the changes to take affect; Oni will now exit");
				exit(0);
			}
		}
	}

	return;
}
*/
// ----------------------------------------------------------------------
static void
ONiResolutions_AddToPopup(
	WMtWindow				*inPopupMenu)
{
	M3tDrawEngineCaps		*current_draw_engine_caps;
	UUtUns16				activeDrawEngine;
	UUtUns16				activeDevice;
	UUtUns16				activeMode;
	UUtUns16				i;
	UUtUns16				num_modes;

	// reset the popup
	WMrPopupMenu_Reset(inPopupMenu);

	// get the index of the active draw engine
	M3rManager_GetActiveDrawEngine(&activeDrawEngine, &activeDevice, &activeMode);

	// get a pointer to the current draw engine's caps
	current_draw_engine_caps = M3rDrawEngine_GetCaps(activeDrawEngine);
	if (current_draw_engine_caps == NULL) { return; }

	// add a list of all of the available
	num_modes = current_draw_engine_caps->displayDevices[activeDevice].numDisplayModes;
	for (i = 0; i < num_modes; i++)
	{
		M3tDisplayMode			*mode;
		char					title[128];

		mode = &current_draw_engine_caps->displayDevices[activeDevice].displayModes[i];

		sprintf(title, "%d x %d x %d bit", mode->width, mode->height, mode->bitDepth);
		WMrPopupMenu_AppendItem_Light(inPopupMenu, i, title);
	}

	// select the current item
	WMrPopupMenu_SetSelection(inPopupMenu, activeMode);
}

// ----------------------------------------------------------------------
static void
ONiOBU_Options_SetControls(
	WMtDialog				*inDialog)
{
	WMtWindow				*slider;
	WMtWindow				*popup;
	WMtWindow				*checkbox;

	// set the sliders
	slider = WMrDialog_GetItemByID(inDialog, ONcOptions_Sldr_Quality);
	WMrSlider_SetPosition(slider, (UUtInt32)ONrPersist_GetGraphicsQuality());

	slider = WMrDialog_GetItemByID(inDialog, ONcOptions_Sldr_Gamma);
	if (slider != NULL) {
		WMrSlider_SetPosition(slider, MUrFloat_Round_To_Int(100 * ONrPersist_GetGamma()));
	}

	slider = WMrDialog_GetItemByID(inDialog, ONcOptions_Sldr_OverallVol);
	WMrSlider_SetPosition(slider, (UUtInt32)(ONrPersist_GetOverallVolume() * 100.0f));

//	slider = WMrDialog_GetItemByID(inDialog, ONcOptions_Sldr_DialogVol);
//	WMrSlider_SetPosition(slider, (UUtInt32)(ONrPersist_GetDialogVolume() * 100.0f));

//	slider = WMrDialog_GetItemByID(inDialog, ONcOptions_Sldr_MusicVol);
//	WMrSlider_SetPosition(slider, (UUtInt32)(ONrPersist_GetMusicVolume() * 100.0f));

	// set the checkbox
//	checkbox = WMrDialog_GetItemByID(inDialog, ONcOptions_CB_DialogOn);
//	WMrCheckBox_SetCheck(checkbox, ONrPersist_IsDialogOn());

//	checkbox = WMrDialog_GetItemByID(inDialog, ONcOptions_CB_MusicOn);
//	WMrCheckBox_SetCheck(checkbox, ONrPersist_IsMusicOn());

	checkbox = WMrDialog_GetItemByID(inDialog, ONcOptions_CB_SubtitlesOn);
	WMrCheckBox_SetCheck(checkbox, ONrPersist_AreSubtitlesOn());

	checkbox = WMrDialog_GetItemByID(inDialog, ONcOptions_CB_InvertMouseOn);
	WMrCheckBox_SetCheck(checkbox, ONrPersist_IsInvertMouseOn());

	// set the popup menu
	popup = WMrDialog_GetItemByID(inDialog, ONcOptions_PM_Difficulty);
	WMrPopupMenu_SetSelection(popup, (UUtInt16)ONrPersist_GetDifficulty());
}

// ----------------------------------------------------------------------
static void
ONiOGU_Options_InitDialog(
	WMtDialog				*inDialog)
{
	WMtWindow				*slider;
	WMtWindow				*popup;

	// set the range on the sliders
	slider = WMrDialog_GetItemByID(inDialog, ONcOptions_Sldr_Quality);
	WMrSlider_SetRange(
		slider,
		(UUtInt32)ONcGraphicsQuality_Min,
		(UUtInt32)ONcGraphicsQuality_Max);

	slider = WMrDialog_GetItemByID(inDialog, ONcOptions_Sldr_Gamma);
	if (slider != NULL) {
		WMrSlider_SetRange(slider, 0, 100);
	}

	slider = WMrDialog_GetItemByID(inDialog, ONcOptions_Sldr_OverallVol);
	WMrSlider_SetRange(slider, 0, 100);

//	slider = WMrDialog_GetItemByID(inDialog, ONcOptions_Sldr_DialogVol);
//	WMrSlider_SetRange(slider, 0, 100);

//	slider = WMrDialog_GetItemByID(inDialog, ONcOptions_Sldr_MusicVol);
//	WMrSlider_SetRange(slider, 0, 100);

	// build the popup menu
	popup = WMrDialog_GetItemByID(inDialog, ONcOptions_PM_Resolution);
	ONiResolutions_AddToPopup(popup);

	// set the fields
	ONiOBU_Options_SetControls(inDialog);
}

// ----------------------------------------------------------------------
static void
ONiOGU_Options_HandleCommand(
	WMtDialog				*inDialog,
	UUtUns32				inParam1,
	WMtWindow				*inControl)
{
	UUtUns16				control_id;
	UUtUns16				command_type;
	float					volume;

	control_id = UUmLowWord(inParam1);
	command_type = UUmHighWord(inParam1);

	switch (control_id)
	{
		case ONcOptions_Sldr_Gamma:
			if (command_type == SLcNotify_NewPosition)
			{
				float gamma_slider_position = (float) WMrSlider_GetPosition(inControl);

				ONrPersist_SetGamma(gamma_slider_position / 100.f);
			}
		break;

		case ONcOptions_Sldr_Quality:
			if (command_type == SLcNotify_NewPosition)
			{
				ONtGraphicsQuality		quality;

				quality = (ONtGraphicsQuality)WMrSlider_GetPosition(inControl);
				ONrPersist_SetGraphicsQuality(quality);
			}
		break;

		case ONcOptions_Sldr_OverallVol:
			if (command_type != SLcNotify_NewPosition) { break; }
			volume = ((float)WMrSlider_GetPosition(inControl)) / 100.0f;
			ONrPersist_SetOverallVolume(volume);
		break;

/*		case ONcOptions_CB_DialogOn:
			if (command_type != WMcNotify_Click) { break; }
			ONrPersist_SetDialogOn(WMrCheckBox_GetCheck(inControl));
		break;

		case ONcOptions_Sldr_DialogVol:
			if (command_type != SLcNotify_NewPosition) { break; }
			volume = ((float)WMrSlider_GetPosition(inControl)) / 100.0f;
			ONrPersist_SetDialogVolume(volume);
		break;

		case ONcOptions_CB_MusicOn:
			if (command_type != WMcNotify_Click) { break; }
			ONrPersist_SetMusicOn(WMrCheckBox_GetCheck(inControl));
		break;

		case ONcOptions_Sldr_MusicVol:
			if (command_type != SLcNotify_NewPosition) { break; }
			volume = ((float)WMrSlider_GetPosition(inControl)) / 100.0f;
			ONrPersist_SetMusicVolume(volume);
		break;*/

		case ONcOptions_CB_SubtitlesOn:
			if (command_type != WMcNotify_Click) { break; }
			ONrPersist_SetSubtitlesOn(WMrCheckBox_GetCheck(inControl));
		break;

		case ONcOptions_CB_InvertMouseOn:
			if (command_type != WMcNotify_Click) { break; }
			ONrPersist_SetInvertMouseOn(WMrCheckBox_GetCheck(inControl));
		break;

		case WMcDialogItem_Cancel:
			if (command_type != WMcNotify_Click) { break; }
			WMrDialog_ModalEnd(inDialog, 0);
		break;
	}
}

// ----------------------------------------------------------------------
static void
ONiOGU_Options_HandleMenuCommand(
	WMtDialog				*inDialog,
	UUtUns32				inParam1,
	WMtWindow				*inMenu)
{
	UUtUns16				item_id;

	item_id = UUmLowWord(inParam1);

	switch (WMrWindow_GetID(inMenu))
	{
		case ONcOptions_PM_Resolution:
			ONiResolution_Switch(inDialog, inMenu, item_id);
		break;

		case ONcOptions_PM_Difficulty:
			ONrPersist_SetDifficulty((ONtDifficultyLevel)item_id);
		break;
	}
}

// ----------------------------------------------------------------------
static UUtBool
ONiOGU_Options_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool					handled;

	handled = UUcTrue;

	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			ONiOGU_Options_InitDialog(inDialog);
		break;

		case WMcMessage_Command:
			ONiOGU_Options_HandleCommand(inDialog, inParam1, (WMtWindow*)inParam2);
		break;

		case WMcMessage_MenuCommand:
			ONiOGU_Options_HandleMenuCommand(inDialog, inParam1, (WMtWindow*)inParam2);
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}

// ----------------------------------------------------------------------
UUtUns32
ONrOutGameUI_Options_Display(
	void)
{
	PStPartSpecUI			*partspec_ui;
	PStPartSpecUI			*temp_ui;
	UUtUns32				message;
	WMtDialogID				dialog_id;

	// save the current ui
	partspec_ui = PSrPartSpecUI_GetActive();

	// set the ui to the out of game ui
	temp_ui = PSrPartSpecUI_GetByName(ONcOutGameUIName);
	if (temp_ui != NULL) { PSrPartSpecUI_SetActive(temp_ui); }

#if (UUmPlatform == UUmPlatform_Mac)
	// CB: macintosh options screen is different (no gamma slider)
	dialog_id = ONcOGU_OptionsID_Mac;
#else
	dialog_id = ONcOGU_OptionsID_PC;
#endif

	// display the dialog
	WMrDialog_ModalBegin(
		dialog_id,
		NULL,
		ONiOGU_Options_Callback,
		0,
		&message);

	// reset the active ui
	PSrPartSpecUI_SetActive(partspec_ui);

	return message;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
ONiOGU_LoadGame_InitDialog(
	WMtDialog				*inDialog)
{
	OWrLevelList_Initialize(inDialog, ONcLoadGame_LB_Levels);
	WMrWindow_SetFocus(WMrDialog_GetItemByID(inDialog, OWcLevelLoad_LB_Level));
}

// ----------------------------------------------------------------------
static void
ONiOGU_LoadGame_HandleCommand(
	WMtDialog				*inDialog,
	UUtUns32				inParam1,
	WMtWindow				*inControl)
{
	UUtUns16				control_id;
	UUtUns16				command_type;
	UUtUns32				level;

	control_id = UUmLowWord(inParam1);
	command_type = UUmHighWord(inParam1);

	switch (control_id)
	{
		case ONcLoadGame_LB_Levels:
			if (command_type != WMcNotify_DoubleClick) { break; }
			level = OWrLevelList_GetLevelNumber(inDialog, ONcLoadGame_LB_Levels);
			if (level != (UUtUns16)(-1))
			{
				WMrDialog_ModalEnd(inDialog, level);
			}
		break;

		case ONcLoadGame_Btn_Load:
			if (command_type != WMcNotify_Click) { break; }
			level = OWrLevelList_GetLevelNumber(inDialog, ONcLoadGame_LB_Levels);
			if (level != (UUtUns16)(-1))
			{
				WMrDialog_ModalEnd(inDialog, level);
			}
		break;

		case ONcLoadGame_Btn_Cancel:
			if (command_type != WMcNotify_Click) { break; }
			WMrDialog_ModalEnd(inDialog, 0);
		break;
	}
}

// ----------------------------------------------------------------------
static UUtBool
ONiOGU_LoadGame_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool					handled;

	handled = UUcTrue;

	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			ONiOGU_LoadGame_InitDialog(inDialog);
		break;

		case WMcMessage_Command:
			ONiOGU_LoadGame_HandleCommand(inDialog, inParam1, (WMtWindow*)inParam2);
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}

// ----------------------------------------------------------------------
UUtUns32
ONrOutGameUI_LoadGame_Display(
	void)
{
	PStPartSpecUI			*partspec_ui;
	PStPartSpecUI			*temp_ui;
	UUtUns32				message;

	// save the current ui
	partspec_ui = PSrPartSpecUI_GetActive();

	// set the ui to the out of game ui
	temp_ui = PSrPartSpecUI_GetByName(ONcOutGameUIName);
	if (temp_ui != NULL) { PSrPartSpecUI_SetActive(temp_ui); }

	// display the dialog
	WMrDialog_ModalBegin(
		ONcOGU_LoadGameID,
		NULL,
		ONiOGU_LoadGame_Callback,
		0,
		&message);

	// reset the active ui
	PSrPartSpecUI_SetActive(partspec_ui);

	return message;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
ONiOGU_NewGame_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool					handled;

	handled = UUcTrue;

	switch (inMessage)
	{
		case WMcMessage_Command:
			if (UUmHighWord(inParam1) != WMcNotify_Click) { break; }
			WMrDialog_ModalEnd(inDialog, UUmLowWord(inParam1));
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}

// ----------------------------------------------------------------------
UUtUns32
ONrOutGameUI_NewGame_Display(
	void)
{
	PStPartSpecUI			*partspec_ui;
	PStPartSpecUI			*temp_ui;
	UUtUns32				message;

	// save the current ui
	partspec_ui = PSrPartSpecUI_GetActive();

	// set the ui to the out of game ui
	temp_ui = PSrPartSpecUI_GetByName(ONcOutGameUIName);
	if (temp_ui != NULL) { PSrPartSpecUI_SetActive(temp_ui); }

	// display the dialog
	WMrDialog_ModalBegin(
		ONcOGU_NewGameID,
		NULL,
		ONiOGU_NewGame_Callback,
		0,
		&message);

	// reset the active ui
	PSrPartSpecUI_SetActive(partspec_ui);

	return message;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
ONiOGU_MainMenu_FadeMusic(
	void)
{
	// fade out the music first
	if (OSrMusic_IsPlaying())
	{
		M3tPoint3D position;
		M3tVector3D facing;
		UUtUns32 count;

		MUmVector_Set(position, 0.0f, 0.0f, 0.0f);
		MUmVector_Set(facing, 0.0f, 0.0f, 0.0f);

		OSrMusic_SetVolume(0.0f, OScMainMusic_FadeOutTime);
		OSrMusic_Stop();

		count = UUrMachineTime_Sixtieths() + (UUtUns32)(60.0f * OScMainMusic_FadeOutTime);
		do
		{
			OSrUpdate(&position, &facing);
			SS2rUpdate();
		}
		while (count > UUrMachineTime_Sixtieths());

		OSrMusic_Halt();
	}
}

// ----------------------------------------------------------------------
static void
ONiOGU_MainMenu_InitDialog(
	WMtDialog				*inDialog)
{
	WMtWindow				*options;
	WMtWindow				*resume;

	options = WMrDialog_GetItemByID(inDialog, ONcMainMenu_Btn_Options);
	resume = WMrDialog_GetItemByID(inDialog, ONcMainMenu_Btn_Resume);

	if (ONrLevel_GetCurrentLevel() > 0)
	{
		WMrWindow_SetVisible(options, UUcFalse);
		WMrWindow_SetVisible(resume, UUcTrue);
	}
	else
	{
		WMrWindow_SetVisible(options, UUcTrue);
		WMrWindow_SetVisible(resume, UUcFalse);
	}

	// stop all currently playing sounds, only allow music
	OSrSetScriptOnly(UUcTrue);
	SSrPlayingChannels_Pause();

	OSrMusic_Start(OScMusicScore_Win, 0.0f);
	OSrMusic_SetVolume(1.0f, OScMainMusic_FadeInTime);
}

// ----------------------------------------------------------------------
static void
ONiOGU_MainMenu_Destroy(
	WMtDialog				*inDialog)
{
	if (OSrMusic_IsPlaying() == UUcTrue)
	{
		OSrMusic_SetVolume(0.0f, 0.5f);
		OSrMusic_Stop();
	}

	// allow more than just music
	OSrSetScriptOnly(UUcFalse);
	SSrPlayingChannels_Resume();
}

// ----------------------------------------------------------------------
static void
ONiOGU_MainMenu_HandleNewGame(
	WMtDialog				*inDialog)
{
	UUtUns32				result;

	// display the new game dialog
	result = ONrOutGameUI_NewGame_Display();
	if (result == ONcNewGame_Btn_Yes)
	{
		// fade out the music first
		ONiOGU_MainMenu_FadeMusic();

		// if the user selects start, then close the main menu
		WMrDialog_ModalEnd(inDialog, 0);

		// update the screen
		WMrUpdate();
		WMrDisplay();

		// load the level
		OWrLevelLoad_StartLevel(1);
	}
}

// ----------------------------------------------------------------------
static void
ONiOGU_MainMenu_HandleLoadGame(
	WMtDialog				*inDialog)
{
	UUtUns16				result;

	// display the load game dialog
	result = (UUtUns16)ONrOutGameUI_LoadGame_Display();
	if (result != 0)
	{
		// fade out the music first
		ONiOGU_MainMenu_FadeMusic();

		// if the user selects load, then close the main menu
		WMrDialog_ModalEnd(inDialog, 0);

		// update the screen
		WMrUpdate();
		WMrDisplay();

		// load the level
		OWrLevelLoad_StartLevel(result);
	}
}

// ----------------------------------------------------------------------
static void
ONiOGU_MainMenu_HandleOptions(
	WMtDialog				*inDialog)
{
	ONrOutGameUI_Options_Display();
}

// ----------------------------------------------------------------------
static void
ONiOGU_MainMenu_HandleQuit(
	WMtDialog				*inDialog)
{
	UUtUns32				result;

	// display the Are you sure you want to quit dialog
	result = ONrOutGameUI_QuitYesNo_Display();
	if (result == ONcQuitYesNo_Btn_Yes)
	{
		// fade out the music first
		ONiOGU_MainMenu_FadeMusic();

		// if the user selects load, then close the main menu
		WMrDialog_ModalEnd(inDialog, 0);

		WMrMessage_Post(NULL, WMcMessage_Quit, 0, 0);

		// end the game
		ONgTerminateGame = UUcTrue;
	}
}

// ----------------------------------------------------------------------
static void
ONiOGU_MainMenu_HandleCommand(
	WMtDialog				*inDialog,
	UUtUns32				inParam1,
	WMtWindow				*inControl)
{
	UUtUns16				control_id;
	UUtUns16				command_type;

	control_id = UUmLowWord(inParam1);
	command_type = UUmHighWord(inParam1);

	if (command_type != WMcNotify_Click) { return; }

	switch (control_id)
	{
		case ONcMainMenu_Btn_NewGame:
			ONiOGU_MainMenu_HandleNewGame(inDialog);
		break;

		case ONcMainMenu_Btn_LoadGame:
			ONiOGU_MainMenu_HandleLoadGame(inDialog);
		break;

		case ONcMainMenu_Btn_Options:
			ONiOGU_MainMenu_HandleOptions(inDialog);
		break;

		case ONcMainMenu_Btn_Quit:
			ONiOGU_MainMenu_HandleQuit(inDialog);
		break;

		case ONcMainMenu_Btn_Resume:
			WMrDialog_ModalEnd(inDialog, 0);
		break;

		case WMcDialogItem_Cancel:
			if (ONrLevel_GetCurrentLevel() != 0)
			{
				WMrDialog_ModalEnd(inDialog, 0);
			}
		break;
	}
}

// ----------------------------------------------------------------------
static UUtBool
ONiOGU_MainMenu_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool					handled;

	handled = UUcTrue;

	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			ONiOGU_MainMenu_InitDialog(inDialog);
		break;

		case WMcMessage_Destroy:
			ONiOGU_MainMenu_Destroy(inDialog);
		break;

		case WMcMessage_Command:
			ONiOGU_MainMenu_HandleCommand(
				inDialog,
				inParam1,
				(WMtWindow*)inParam2);
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}

// ----------------------------------------------------------------------
UUtUns32
ONrOutGameUI_MainMenu_Display(
	void)
{
	PStPartSpecUI				*partspec_ui;
	PStPartSpecUI				*temp_ui;

	// save the current ui
	partspec_ui = PSrPartSpecUI_GetActive();

	// set the ui to the out of game ui
	temp_ui = PSrPartSpecUI_GetByName(ONcOutGameUIName);
	if (temp_ui != NULL) { PSrPartSpecUI_SetActive(temp_ui); }

	// set the background to black
	WMrSetDesktopBackground(PSrPartSpec_LoadByType(PScPartSpecType_BackgroundColor_Black));

	// display the dialog
	WMrDialog_ModalBegin(
		ONcOGU_MainMenuID,
		NULL,
		ONiOGU_MainMenu_Callback,
		0,
		NULL);

	// set the desktop background to none
	WMrSetDesktopBackground(PSrPartSpec_LoadByType(PScPartSpecType_BackgroundColor_None));

	// reset the active ui
	PSrPartSpecUI_SetActive(partspec_ui);

	return UUcError_None;
}
