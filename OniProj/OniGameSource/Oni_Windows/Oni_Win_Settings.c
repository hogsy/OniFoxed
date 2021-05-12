// ======================================================================
// Oni_Win_Settings.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

#include "Oni_Windows.h"
#include "Oni_Character.h"
#include "Oni_AI_Setup.h"

// ======================================================================
// typedefs
// ======================================================================
typedef struct OWtSettings_Player
{
	char					player_name[ONcMaxPlayerNameLength + 1];
	char					character_class[AIcMaxClassNameLen + 1];
	
} OWtSettings_Player;

// ======================================================================
// globals
// ======================================================================
static OWtSettings_Player	OWgSettings_Player;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
OWiSettings_Player_Initialize(
	WMtDialog				*inDialog)
{
	WMtWindow				*window;
	
	// set the player name
	window = WMrDialog_GetItemByID(inDialog, OWcSettings_Player_PlayerName);
	if (window != NULL)
	{
		WMrMessage_Send(
			window,
			EFcMessage_SetText,
			(UUtUns32)OWgSettings_Player.player_name,
			0);
	}
		
	// get the character class popup
	window = WMrDialog_GetItemByID(inDialog, OWcSettings_Player_CharacterClass);
	if (window == NULL)
	{
		// add the character classes to the popup
	
		// set the character class
	}
}

// ----------------------------------------------------------------------
static void
OWiSettings_Player_Destroy(
	WMtDialog				*inDialog)
{
	WMtWindow				*window;
	
	// get the player name
	window = WMrDialog_GetItemByID(inDialog, OWcSettings_Player_PlayerName);
	if (window != NULL)
	{
		WMrMessage_Send(
			window,
			EFcMessage_GetText,
			(UUtUns32)OWgSettings_Player.player_name,
			(UUtUns32)ONcMaxPlayerNameLength);
	}
	
	// get the character class
	window = WMrDialog_GetItemByID(inDialog, OWcSettings_Player_CharacterClass);
	if (window != NULL)
	{
	}
}

// ----------------------------------------------------------------------
UUtBool
OWrSettings_Player_Callback(
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
			OWiSettings_Player_Initialize(inDialog);
		break;
		
		case WMcMessage_Destroy:
			OWiSettings_Player_Destroy(inDialog);
		break;
		
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrDialog_ModalEnd(inDialog, WMcDialogItem_Cancel);
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
char*
OWrSettings_GetCharacterClass(
	void)
{
	return OWgSettings_Player.character_class;
}

// ----------------------------------------------------------------------
char*
OWrSettings_GetPlayerName(
	void)
{
	return OWgSettings_Player.player_name;
}

// ----------------------------------------------------------------------
UUtError
OWrSettings_Initialize(
	void)
{
	// initialize the player settings
	UUrString_Copy(OWgSettings_Player.player_name, "Player", ONcMaxPlayerNameLength);
	UUrString_Copy(OWgSettings_Player.character_class, "konoko_generic", AIcMaxClassNameLen);
	
	return UUcError_None;
}