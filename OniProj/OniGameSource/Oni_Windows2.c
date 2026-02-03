#if SHIPPING_VERSION
// ======================================================================
// Oni_Windows2.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_LocalInput.h"
#include "BFW_WindowManager.h"

#include "Oni_Level.h"
#include "Oni_Windows.h"
#include "Oni_OutGameUI.h"
#include "Oni_Persistance.h"

#include "Oni_Win_AI.h"
#include "Oni_Win_Particle.h"

// ======================================================================
// globals
// ======================================================================
static UUtBool				OWgRunStartup;
static UUtUns16				OWgLevelNumber;
static WMtWindow			*OWgOniWindow;

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

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
OWrOniWindow_Startup(
	void)
{
	// set the desktop background to none
	WMrSetDesktopBackground(PSrPartSpec_LoadByType(PScPartSpecType_BackgroundColor_None));

	// run the main menu
	OWrOniWindow_Toggle();
}

// ----------------------------------------------------------------------
void
OWrOniWindow_Toggle(
	void)
{
		WMrActivate();
		LIrMode_Set(LIcMode_Normal);

		ONrOutGameUI_MainMenu_Display();

		LIrMode_Set(LIcMode_Game);
		WMrDeactivate();
}

// ----------------------------------------------------------------------
UUtBool
OWrOniWindow_Visible(
	void)
{
	return UUcFalse;
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

	// ------------------------------
	// startup the window manager
	// ------------------------------
	error = WMrStartup();
	UUmError_ReturnOnError(error);

	OWgLevelNumber = (UUtUns16)0xFFFF;

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
}

// ----------------------------------------------------------------------
UUtError
OWrLevelBegin(
	void)
{
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
		UUtUns32			handled;

		handled = WMrMessage_Dispatch(&event);
		if (handled == WMcResult_NoWindow)
		{
			switch (event.message)
			{
				case WMcMessage_Quit:
					ONgTerminateGame = UUcTrue;
				break;

				// Oni defined messages
				case OWcMessage_RunGame:
					OWgRunStartup = UUcFalse;
				break;
			}
		}
	}
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
AI2tPatrolPath *OWgCurrentPatrolPath = NULL;
UUtBool OWgCurrentPatrolPathValid = UUcFalse;

// ----------------------------------------------------------------------
void
OWrAI_Display(
	void)
{
}

// ----------------------------------------------------------------------
UUtBool
OWrAI_DropAndAddFlag(
	void)
{
	return UUcFalse;
}

// ----------------------------------------------------------------------
UUtError OWrSave_Particles(UUtBool inAutosave)
{
	return UUcError_None;
}

#endif
