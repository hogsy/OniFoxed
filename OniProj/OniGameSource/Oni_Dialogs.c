// ======================================================================
// Oni_Dialogs.c
// ======================================================================
#if 0

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_DialogManager.h"
#include "BFW_Platform.h"
#include "VM_View_Tab.h"
#include "VM_View_Slider.h"

#include "Oni.h"
#include "Oni_Dialogs.h"
#include "Oni_GameState.h"
#include "Oni_Networking.h"
#include "Oni_Character.h"
#include "Oni_Level.h"


#if defined(ONmUnitViewer) && (ONmUnitViewer == 1)
#include "Oni_UnitViewer.h"
#endif

#include <ctype.h>

// ======================================================================
// defines
// ======================================================================
#define ONcMaxHostAddressLength		16
#define ONcMaxPasswordLength		8
#define ONcReturnToGame				1

// ======================================================================
// typedefs
// ======================================================================
typedef struct ONtMP_PlayerInfo
{
	char					player_name[ONcMaxPlayerNameLength];
	char					team_name[ONcMaxTeamNameLength];
	char					character_class[AIcMaxClassNameLen];
	char					host_address[ONcMaxHostAddressLength];
	char					password[ONcMaxPasswordLength];
	UUtBool					remember_password;

} ONtMP_PlayerInfo;

typedef struct ONtMP_HostInfo
{
	char					host_name[ONcMaxHostNameLength];
	UUtUns16				level_number;
	ONtNetGameOptions		game_options;

} ONtMP_HostInfo;

typedef struct ONtNet_ServerArrayInfo
{
	ONtNet_ServerInfo		server_info;
	NMtNetAddress			server_address;

} ONtNet_ServerArrayInfo;

// ======================================================================
// globals
// ======================================================================
UUtBool						ONgDisplayDialogs = UUcTrue;

static ONtMP_PlayerInfo		ONgMP_PlayerInfo;
static ONtMP_HostInfo		ONgMP_HostInfo;

static UUtBool				MM_first_time = UUcTrue;
static UUtBool				MPJG_first_time = UUcTrue;

static UUtBool				ONgJoining_LevelHasLoaded = UUcFalse;

static UUtMemory_Array		*ONgMPJG_Servers;

static UUtUns8				ONgMungedPassword[5] = {0x6B, 0x8B, 0x16, 0x72, 0xE1};

UUtUns32					ONgMainMenu_SwitchTime;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
ONiLevelList_Initialize(
	DMtDialog				*inDialog,
	UUtUns16				view_id)
{
	UUtError				error;
	UUtUns16				num_descriptors;

	// get the number of descriptors
	num_descriptors = (UUtUns16)TMrInstance_GetTagCount(ONcTemplate_Level_Descriptor);
	if (num_descriptors > 0)
	{
		UUtUns16			i;
		VMtView				*levels;

		// get a pointer to the level list
		levels = DMrDialog_GetViewByID(inDialog, view_id);
		if (levels == NULL) return;

		for (i = 0; i < num_descriptors; i++)
		{
			ONtLevel_Descriptor		*descriptor;

			// get a pointer to the first descriptor
			error =
				TMrInstance_GetDataPtr_ByNumber(
					ONcTemplate_Level_Descriptor,
					i,
					&descriptor);
			if (error != UUcError_None) return;

			// if the level in the descriptor doesn't exist, move on
			// to the next descriptor
			if (!TMrLevel_Exists(descriptor->level_number)) continue;

			// add the name of the level to the list
			VMrView_SendMessage(
				levels,
				LBcMessage_AddString,
				0,
				(UUtUns32)descriptor->level_name);
		}

		// set the selection to the first entry
		VMrView_SendMessage(
			levels,
			LBcMessage_SetSelection,
			0,
			0);
	}
}

// ----------------------------------------------------------------------
static UUtUns16
ONiLevelList_GetLevelNumber(
	DMtDialog		*inDialog,
	UUtUns16		inViewID)
{
	UUtUns16		num_descriptors;
	UUtUns16		level_number;

	// set the level number to the default
	level_number = (UUtUns16)(-1);

	// get the number of descriptors
	num_descriptors = (UUtUns16)TMrInstance_GetTagCount(ONcTemplate_Level_Descriptor);
	if (num_descriptors > 0)
	{
		UUtError	error;
		VMtView		*levels;
		char		level_name[ONcMaxLevelName];
		UUtUns16	i;

		// get a pointer to the levels listbox
		levels = DMrDialog_GetViewByID(inDialog, inViewID);
		if (levels == NULL) return level_number;

		// get the current selection name
		VMrView_SendMessage(
			levels,
			LBcMessage_GetText,
			(UUtUns32)(-1),
			(UUtUns32)(&level_name));

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

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
ONiMP_JoinGame(
	char					*inHostAddress)
{
	UUtError				error;

	// start a client
	error =
		ONrNet_Client_Start(
			ONcNet_DefaultClientPort);
	if ((error != UUcError_None) && (error != UUcError_Client_AlreadyStarted))
	{
		UUrError_Report(error, "Unable to start client");
		return UUcFalse;
	}

	// join the host
	error =
		ONrNet_Client_Join(
			ONcNet_DefaultServerPort,
			inHostAddress,
			ONgMP_PlayerInfo.character_class,
			ONgMP_PlayerInfo.player_name,
			ONgMP_PlayerInfo.team_name);
	if (error != UUcError_None)
	{
		UUrError_Report(error, "Unable to join host");

		error = ONrNet_Client_Stop();
		if (error != UUcError_None)
		{
			UUrError_Report(error, "Unable to stop client");
		}

		return UUcFalse;
	}

	return UUcTrue;
}

// ----------------------------------------------------------------------
static UUtBool
ONiMP_HostGame(
	void)
{
	UUtError				error;
	UUtBool					result;

	// make sure that the level number is valid
	if ((ONgMP_HostInfo.level_number == (UUtUns16)(-1)) ||
		(ONgMP_HostInfo.level_number == 0))
	{
		return UUcFalse;
	}

	// start a host
	error =
		ONrNet_Server_Start(
			ONcNet_DefaultServerPort,
			ONgMP_HostInfo.host_name,
			&ONgMP_HostInfo.game_options,
			ONgMP_HostInfo.level_number);
	if (error != UUcError_None)
	{
		UUrError_Report(error, "Unable to start host");
		return UUcFalse;
	}

	// join the host on this machine
	result =
		ONiMP_JoinGame(
			ONrNet_Server_GetAddress());
	if (result == UUcFalse)
	{
		error = ONrNet_Server_Stop();
		if (error != UUcError_None)
			UUrError_Report(error, "Unable to stop host");

		return UUcFalse;
	}

	return UUcTrue;
}

// ----------------------------------------------------------------------
static void
ONiMP_Net_Level_Callback(
	UUtBool					inIsLoading,
	UUtUns16				inLevelNumber)
{
	if (inIsLoading)
	{
		ONgJoining_LevelHasLoaded = UUcTrue;
	}
	else
	{
		ONgJoining_LevelHasLoaded = UUcFalse;
	}
}

// ----------------------------------------------------------------------
static void
ONiMP_RestoreMPInfo(
	DMtDialog				*inDialog)
{
	VMtView				*view;

	// ------------------------------
	// host tab
	// ------------------------------
	// set the host name
	view = DMrDialog_GetViewByID(inDialog, ONcMPHG_EF_HostName);
	VMrView_SetValue(view, (UUtUns32)&ONgMP_HostInfo.host_name);

	// set the number of bots
	if (ONgMP_HostInfo.game_options.game_parameters & ONcNetGameParam_HasAIs)
	{
		char			string[32];

		string[0] = '\0';

		sprintf(string, "%d", ONgMP_HostInfo.game_options.num_AIs);
		view = DMrDialog_GetViewByID(inDialog, ONcMPHG_EF_NumBots);
		VMrView_SetValue(view, (UUtUns32)string);
	}

	// ------------------------------
	// join tab
	// ------------------------------
	// set the host address
	view = DMrDialog_GetViewByID(inDialog, ONcMPJG_EF_HostAddress);
	VMrView_SetValue(view, (UUtUns32)&ONgMP_PlayerInfo.host_address);

	// set the password
	view = DMrDialog_GetViewByID(inDialog, ONcMPJG_EF_Password);
	VMrView_SetValue(view, (UUtUns32)&ONgMP_PlayerInfo.password);

	// set the remember password checkbox
	view = DMrDialog_GetViewByID(inDialog, ONcMPJG_CB_RememberPassword);
	VMrView_SetValue(view, (UUtUns32)ONgMP_PlayerInfo.remember_password);

	// ------------------------------
	// options tab
	// ------------------------------
	// set the players name
	view = DMrDialog_GetViewByID(inDialog, ONcMPO_EF_PlayerName);
	VMrView_SetValue(view, (UUtUns32)&ONgMP_PlayerInfo.player_name);

	// set the team name
	view = DMrDialog_GetViewByID(inDialog, ONcMPO_EF_TeamName);
	VMrView_SetValue(view, (UUtUns32)&ONgMP_PlayerInfo.team_name);
}

// ----------------------------------------------------------------------
static void
ONiMP_SaveMPInfo(
	DMtDialog				*inDialog)
{
	VMtView				*view;
	UUtUns32			value;
	UUtUns32			num_bots;

	// ------------------------------
	// host tab
	// ------------------------------
	// get the host name
	view = DMrDialog_GetViewByID(inDialog, ONcMPHG_EF_HostName);
	value = VMrView_GetValue(view);
	UUrString_Copy((char*)ONgMP_HostInfo.host_name, (char*)value, ONcMaxHostNameLength);

	// get the number of bots
	view = DMrDialog_GetViewByID(inDialog, ONcMPHG_EF_NumBots);
	value = VMrView_GetValue(view);
	sscanf((char*)value, "%d", &num_bots);
	if (num_bots > 0)
	{
		ONgMP_HostInfo.game_options.game_parameters |= ONcNetGameParam_HasAIs;
		ONgMP_HostInfo.game_options.num_AIs = (UUtUns16)num_bots;
	}

	// get the level number
	ONgMP_HostInfo.level_number = ONiLevelList_GetLevelNumber(inDialog, ONcMPHG_LB_Level);

	// ------------------------------
	// join tab
	// ------------------------------
	// get the host address
	view = DMrDialog_GetViewByID(inDialog, ONcMPJG_EF_HostAddress);
	value = VMrView_GetValue(view);
	UUrString_Copy((char*)ONgMP_PlayerInfo.host_address, (char*)value, ONcMaxHostAddressLength);

	// get the password
	view = DMrDialog_GetViewByID(inDialog, ONcMPJG_EF_Password);
	value = VMrView_GetValue(view);
	UUrString_Copy((char*)ONgMP_PlayerInfo.password, (char*)value, ONcMaxPasswordLength);

	// get the remember password checkbox
	view = DMrDialog_GetViewByID(inDialog, ONcMPJG_CB_RememberPassword);
	ONgMP_PlayerInfo.remember_password = (UUtBool)VMrView_GetValue(view);

	// ------------------------------
	// options tab
	// ------------------------------
	// get the players name
	view = DMrDialog_GetViewByID(inDialog, ONcMPO_EF_PlayerName);
	value = VMrView_GetValue(view);
	UUrString_Copy((char*)ONgMP_PlayerInfo.player_name, (char*)value, ONcMaxPlayerNameLength);

	// get the team name
	view = DMrDialog_GetViewByID(inDialog, ONcMPO_EF_TeamName);
	value = VMrView_GetValue(view);
	UUrString_Copy((char*)ONgMP_PlayerInfo.team_name, (char*)value, ONcMaxTeamNameLength);
}

// ----------------------------------------------------------------------
static void
ONiMPJG_FindServers_Callback(
	ONtNet_ServerInfo		*inServerInfo,
	NMtNetAddress			*inServerAddress,
	UUtUns32				inUserParam)
{
	UUtError				error;
	VMtView					*tab_view;
	VMtView					*list_view;
	UUtUns32				index;
	UUtUns16				num_servers;
	UUtBool					server_in_list;
	ONtNet_ServerArrayInfo	*server_info_array;
	UUtBool					mem_moved;

	UUmAssert(inServerInfo);
	UUmAssert(inUserParam);

	if (ONgMPJG_Servers == NULL) return;

	// ------------------------------
	// get pointers
	// ------------------------------
	// get a pointer to the views
	tab_view = (VMtView*)inUserParam;
	list_view = DMrDialog_GetViewByID(tab_view, ONcMPJG_LB_Hosts);
	UUmAssert(list_view);

	if (list_view == NULL) return;

	// ------------------------------
	// check to see if this server's info has already been received
	// ------------------------------
	// get the array pointer
	server_info_array = (ONtNet_ServerArrayInfo*)UUrMemory_Array_GetMemory(ONgMPJG_Servers);
	num_servers = (UUtUns16)UUrMemory_Array_GetUsedElems(ONgMPJG_Servers);

	server_in_list = UUcFalse;
	for (index = 0; index < num_servers; index++)
	{
		UUtBool result;

		result = strcmp(server_info_array[index].server_info.server_name, inServerInfo->server_name);
		if (result == 0)
		{
			server_in_list = UUcTrue;
			break;
		}
	}

	if (server_in_list) return;

	// ------------------------------
	// store inServerInfo in an array
	// ------------------------------
	// add the server info to the array
	error =
		UUrMemory_Array_GetNewElement(
			ONgMPJG_Servers,
			&index,
			&mem_moved);
	if (error != UUcError_None) return;

	// get a fresh pointer to the memory
	server_info_array = (ONtNet_ServerArrayInfo*)UUrMemory_Array_GetMemory(ONgMPJG_Servers);

	// copy the server info into the array
	UUrMemory_MoveFast(
		inServerInfo,
		&server_info_array[index].server_info,
		sizeof(ONtNet_ServerInfo));

	UUrMemory_MoveFast(
		inServerAddress,
		&server_info_array[index].server_address,
		sizeof(NMtNetAddress));

	// add the server info to the listbox
	VMrView_SendMessage(
		list_view,
		LBcMessage_InsertString,
		index,
		(UUtUns32)inServerInfo->server_name);
}

// ----------------------------------------------------------------------
static void
ONiMPJG_SetIPAddress(
	VMtView					*inTab)
{
	VMtView					*server_list;
	VMtView					*server_address;
	UUtUns32				current_selection;
	char					*server_address_string;

	// get pointer to the views
	server_list = DMrDialog_GetViewByID(inTab, ONcMPJG_LB_Hosts);
	server_address = DMrDialog_GetViewByID(inTab, ONcMPJG_EF_HostAddress);

	// get the index of the currently selected server
	current_selection =
		VMrView_SendMessage(
			server_list,
			LBcMessage_GetCurrentSelection,
			0,
			0);

	// set the server address
	server_address_string = "";

	if (current_selection == (UUtUns32)-1)
	{
		// no server has been selected so set the address to a blank line
		VMrView_SendMessage(
			server_address,
			VMcMessage_SetValue,
			(UUtUns32)server_address_string,
			0);
	}
	else
	{
		ONtNet_ServerArrayInfo	*server_info_array;

		// get a fresh pointer to the memory
		server_info_array = (ONtNet_ServerArrayInfo*)UUrMemory_Array_GetMemory(ONgMPJG_Servers);

		if (server_info_array)
		{
			server_address_string =
				NMrAddressToString(
					&server_info_array[current_selection].server_address);

			// set the addres to the address of the server
			VMrView_SendMessage(
				server_address,
				VMcMessage_SetValue,
				(UUtUns32)server_address_string,
				0);
		}
	}
}

// ----------------------------------------------------------------------
static UUtBool
ONiMPJG_TabCallback(
	VMtView					*inTab,
	VMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool					handled;
	UUtError				error;

	handled = UUcTrue;

	switch (inMessage)
	{
		case TbcMessage_InitTab:
			// start at timer so that the networking can be updated
			error = VMrView_Timer_Start(inTab, 1, 1);
			if (error == UUcError_None)
			{
				// find available local servers
				ONrNet_FindServers_Start(
					ONiMPJG_FindServers_Callback,
					(UUtUns32)inTab);
			}

			// allocate memory for the server info
			if (ONgMPJG_Servers == NULL)
			{
				ONgMPJG_Servers =
					UUrMemory_Array_New(
						sizeof(ONtNet_ServerArrayInfo),
						1,
						0,
						0);
			}
		break;

		case VMcMessage_Destroy:
			// stop finding servers
			ONrNet_FindServers_Stop();

			// stop the timer
			VMrView_Timer_Stop(inTab, 1);

			// release the server info array
			if (ONgMPJG_Servers)
			{
				UUrMemory_Array_Delete(ONgMPJG_Servers);
				ONgMPJG_Servers = NULL;
			}
		break;

		case VMcMessage_Timer:
			// update networking
			ONrNet_FindServers_Update();
		break;

		case VMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case ONcMPJG_LB_Hosts:
					if (UUmHighWord(inParam1) == LBcNotify_SelectionChanged)
					{
						ONiMPJG_SetIPAddress(inTab);
					}
				break;

				default:
					handled = UUcFalse;
				break;
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
ONrDialogCallback_Multiplayer(
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
		{
			VMtView			*tab_view;

			ONiMP_RestoreMPInfo(inDialog);
			ONiLevelList_Initialize(inDialog, ONcMPHG_LB_Level);
			handled = UUcFalse;

			// set the tab callbacks
			tab_view = DMrDialog_GetViewByID(inDialog, ONcMP_Tab_JoinGame);
			UUmAssert(tab_view);
			VMrView_Tab_SetTabCallback(tab_view, ONiMPJG_TabCallback);
		}
		break;

		case VMcMessage_Destroy:
			ONiMP_SaveMPInfo(inDialog);
		break;

		case VMcMessage_KeyDown:
			if (inParam1 == LIcKeyCode_Escape)
				DMrDialog_Stop(inDialog, 0);
		break;

		case VMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case ONcBtn_Back:
					DMrDialog_Stop(inDialog, 0);
				break;

				case ONcMP_Btn_HostGame:
					DMrDialog_ActivateTab(inDialog, ONcMP_Tab_HostGame);
				break;

				case ONcMP_Btn_JoinGame:
					DMrDialog_ActivateTab(inDialog, ONcMP_Tab_JoinGame);
				break;

				case ONcMP_Btn_Options:
					DMrDialog_ActivateTab(inDialog, ONcMP_Tab_Options);
				break;

				case ONcMPJG_Btn_Join:
					ONiMP_SaveMPInfo(inDialog);
					DMrDialog_Stop(inDialog, ONcMPJG_Btn_Join);
				break;

				case ONcMPHG_Btn_StartHost:
					ONiMP_SaveMPInfo(inDialog);
					if ((ONgMP_HostInfo.level_number != (UUtUns16)(-1)) &&
						(ONgMP_HostInfo.level_number != 0))
					{
						DMrDialog_Stop(inDialog, ONcMPHG_Btn_StartHost);
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

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
ONiNG_Start(
	DMtDialog		*inDialog)
{
	UUtUns16		num_descriptors;

	// get the number of descriptors
	num_descriptors = (UUtUns16)TMrInstance_GetTagCount(ONcTemplate_Level_Descriptor);
	if (num_descriptors > 0)
	{
		UUtUns16	level_number;

		// get the level number
		level_number = ONiLevelList_GetLevelNumber(inDialog, ONcNG_LB_Level);

		if (level_number != (UUtUns16)(-1))
		{
			// stop the dialog
			DMrDialog_Stop(
				inDialog,
				UUmMakeLong(level_number, ONcNG_Btn_Start));
		}
	}
}

// ----------------------------------------------------------------------
UUtBool
ONrDialogCallback_NewGame(
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
			ONiLevelList_Initialize(inDialog, ONcNG_LB_Level);
			handled = UUcFalse;
		break;

		case VMcMessage_KeyDown:
			if (inParam1 == LIcKeyCode_Escape)
				DMrDialog_Stop(inDialog, 0);
		break;

		case VMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case ONcBtn_Back:
					DMrDialog_Stop(inDialog, 0);
				break;

				case ONcNG_Btn_Start:
					ONiNG_Start(inDialog);
				break;

				case ONcNG_LB_Level:
					if (UUmHighWord(inParam1) == VMcNotify_DoubleClick)
					{
						ONiNG_Start(inDialog);
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

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
ONiOptns_Control_Set(
	UUtUns32				inBoundInput,
	UUtUns16				inActionType,
	UUtUns32				inUserParam)
{
	VMtView					*tab_view;
	VMtView					*control_view;
	char					*string;
	char					input_name[LIcMaxInputNameLength];
	UUtUns16				id;

	tab_view = (VMtView*)inUserParam;
	UUmAssert(tab_view);

	switch (inActionType)
	{
		case LIc_Bit_Forward:	id = ONcOC_EF_Forward1; 		break;
		case LIc_Bit_Backward:	id = ONcOC_EF_Backward1;		break;
		case LIc_Bit_TurnLeft:	id = ONcOC_EF_TurnLeft1;		break;
		case LIc_Bit_TurnRight:	id = ONcOC_EF_TurnRight1;		break;
		case LIc_Bit_StepLeft:	id = ONcOC_EF_SidestepLeft1;	break;
		case LIc_Bit_StepRight:	id = ONcOC_EF_SidestepRight1;	break;
		case LIc_Bit_Jump:		id = ONcOC_EF_Jump1;			break;
		case LIc_Bit_Crouch:	id = ONcOC_EF_Crouch1;			break;
		case LIc_Bit_Punch:		id = ONcOC_EF_Punch1;			break;
		case LIc_Bit_Kick:		id = ONcOC_EF_Kick1;			break;
		default: return UUcTrue;
	}

	control_view = DMrDialog_GetViewByID(tab_view, id);
	UUmAssert(control_view);
	string = (char*)VMrView_SendMessage(control_view, VMcMessage_GetValue, 0, 0);
	if (string[0] == '\0')
	{
		LIrTranslate_InputCode(inBoundInput, input_name);
		VMrView_SendMessage(control_view, VMcMessage_SetValue, (UUtUns32)input_name, 0);
		return UUcTrue;
	}

	switch (inActionType)
	{
		case LIc_Bit_Forward:	id = ONcOC_EF_Forward2; 		break;
		case LIc_Bit_Backward:	id = ONcOC_EF_Backward2;		break;
		case LIc_Bit_TurnLeft:	id = ONcOC_EF_TurnLeft2;		break;
		case LIc_Bit_TurnRight:	id = ONcOC_EF_TurnRight2;		break;
		case LIc_Bit_StepLeft:	id = ONcOC_EF_SidestepLeft2;	break;
		case LIc_Bit_StepRight:	id = ONcOC_EF_SidestepRight2;	break;
		case LIc_Bit_Jump:		id = ONcOC_EF_Jump2;			break;
		case LIc_Bit_Crouch:	id = ONcOC_EF_Crouch2;			break;
		case LIc_Bit_Punch:		id = ONcOC_EF_Punch2;			break;
		case LIc_Bit_Kick:		id = ONcOC_EF_Kick2;			break;
	}

	control_view = DMrDialog_GetViewByID(tab_view, id);
	UUmAssert(control_view);
	string = (char*)VMrView_SendMessage(control_view, VMcMessage_GetValue, 0, 0);
	if (string[0] == '\0')
	{
		LIrTranslate_InputCode(inBoundInput, input_name);
		VMrView_SendMessage(control_view, VMcMessage_SetValue, (UUtUns32)input_name, 0);
		return UUcTrue;
	}

	return UUcTrue;
}

// ----------------------------------------------------------------------
static UUtBool
ONiOptns_Tab_AVCallback(
	VMtView					*inTab,
	VMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool					handled;
	VMtView					*view;

	handled = UUcTrue;

	switch(inMessage)
	{
		case TbcMessage_InitTab:
		{
			UUtInt32		min_gamma;
			UUtInt32		max_gamma;
			UUtInt32		current_gamma;

			// set the gamma
			ONrPlatform_GetGammaParams(&min_gamma, &max_gamma, &current_gamma);
			view = DMrDialog_GetViewByID(inTab, ONcOAV_Sldr_Gamma);
			UUmAssert(view);
			VMrView_Slider_SetRange(view, min_gamma, max_gamma);
			VMrView_Slider_SetPosition(view, current_gamma);
		}
		break;

		case VMcMessage_Destroy:
		{
			UUtInt32		new_gamma;

			// set the gamma
			view = DMrDialog_GetViewByID(inTab, ONcOAV_Sldr_Gamma);
			UUmAssert(view);
			new_gamma = VMrView_Slider_GetPosition(view);
			ONrPlatform_SetGamma(new_gamma);
		}
		break;

		case VMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case SLcNotify_NewPosition:
				{
					if (UUmHighWord(inParam1) == ONcOAV_Sldr_Gamma)
					{
						ONrPlatform_SetGamma((UUtInt32)inParam2);
					}
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

// ----------------------------------------------------------------------
static UUtBool
ONiOptns_Tab_CntrlsCallback(
	VMtView					*inTab,
	VMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool					handled;
	VMtView					*view;
	UUtUns32				result;

	handled = UUcTrue;

	switch(inMessage)
	{
		case TbcMessage_InitTab:
			LIrBindings_Enumerate(ONiOptns_Control_Set, (UUtUns32)inTab);

			// set invert mouse checkbox
			view = DMrDialog_GetViewByID(inTab, ONcOC_CB_InvertMouse);
			UUmAssert(view);
			VMrView_SendMessage(view, VMcMessage_SetValue, LIrMouse_Invert_Get(), 0);
		break;

		case VMcMessage_Destroy:
			// set invert mouse
			view = DMrDialog_GetViewByID(inTab, ONcOC_CB_InvertMouse);
			UUmAssert(view);
			result = VMrView_SendMessage(view, VMcMessage_GetValue, 0, 0);
			LIrMouse_Invert_Set((UUtBool)result);
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}

// ----------------------------------------------------------------------
UUtBool
ONrDialogCallback_Options(
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
		{
			VMtView			*tab_view;

			handled = UUcFalse;

			// set the tab callbacks
			tab_view = DMrDialog_GetViewByID(inDialog, ONcOptns_Tab_Controls);
			UUmAssert(tab_view);
			VMrView_Tab_SetTabCallback(tab_view, ONiOptns_Tab_CntrlsCallback);

			// set the gamma
			tab_view = DMrDialog_GetViewByID(inDialog, ONcOptns_Tab_AV);
			UUmAssert(tab_view);
			VMrView_Tab_SetTabCallback(tab_view, ONiOptns_Tab_AVCallback);
		}
		break;

		case VMcMessage_KeyDown:
			if (inParam1 == LIcKeyCode_Escape)
			{
				DMrDialog_Stop(inDialog, 0);
			}
		break;

		case VMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case ONcBtn_Back:
					DMrDialog_Stop(inDialog, 0);
				break;

				case ONcOptns_Btn_AV:
					DMrDialog_ActivateTab(inDialog, ONcOptns_Tab_AV);
				break;

				case ONcOptns_Btn_Controls:
					DMrDialog_ActivateTab(inDialog, ONcOptns_Tab_Controls);
				break;

				case ONcOptns_Btn_Advanced:
					DMrDialog_ActivateTab(inDialog, ONcOptns_Tab_Advanced);
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
UUtBool
ONrDialogCallback_LoadSave(
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
			handled = UUcFalse;
		break;

		case VMcMessage_KeyDown:
			if (inParam1 == LIcKeyCode_Escape)
				DMrDialog_Stop(inDialog, 0);
		break;

		case VMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case ONcBtn_Back:
					DMrDialog_Stop(inDialog, 0);
				break;

				case ONcLS_Btn_Load:
					DMrDialog_ActivateTab(inDialog, ONcLS_Tab_Load);
				break;

				case ONcLS_Btn_Save:
					DMrDialog_ActivateTab(inDialog, ONcLS_Tab_Save);
				break;

				case ONcLS_Btn_Film:
					DMrDialog_ActivateTab(inDialog, ONcLS_Tab_Film);
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
UUtBool
ONrDialogCallback_Quit(
	DMtDialog				*inDialog,
	DMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool					handled;

	handled = UUcTrue;

	switch (inMessage)
	{
		case VMcMessage_KeyDown:
			switch (inParam1)
			{
				case LIcKeyCode_Escape:
					DMrDialog_Stop(inDialog, ONcQuit_Btn_Yes);
				break;

				case LIcKeyCode_Return:
					DMrDialog_Stop(inDialog, ONcQuit_Btn_No);
				break;
			}
		break;

		case VMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case ONcBtn_Back:
				case ONcQuit_Btn_No:
					DMrDialog_Stop(inDialog, ONcQuit_Btn_No);
				break;

				case ONcQuit_Btn_Yes:
					DMrDialog_Stop(inDialog, ONcQuit_Btn_Yes);
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
UUtBool
ONrDialogCallback_Joining(
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
			error = VMrView_Timer_Start(inDialog, 1, 4);
			if (error != UUcError_None)
			{
				DMrDialog_Stop(inDialog, error);
			}

			if (ONiMP_JoinGame(ONgMP_PlayerInfo.host_address) == UUcFalse)
			{
				DMrDialog_Stop(inDialog, UUcError_Generic);
			}

			ONrNet_Level_Callback_Register(ONiMP_Net_Level_Callback);
		break;

		case VMcMessage_Destroy:
			VMrView_Timer_Stop(inDialog, 1);
			ONrNet_Level_Callback_Unregister();
		break;

		case VMcMessage_Timer:
			ONrNet_Update();
			if (ONgJoining_LevelHasLoaded)
			{
				DMrDialog_Stop(inDialog, UUcError_None);
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
ONrDialogCallback_MainMenu(
	DMtDialog				*inDialog,
	DMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtUns32				message;
	UUtBool					handled;

	handled = UUcTrue;

	switch (inMessage)
	{
		case DMcMessage_InitDialog:
			ONrGameState_Pause(UUcTrue);
		break;

		case VMcMessage_Destroy:
			ONrGameState_Pause(UUcFalse);
		break;

		case VMcMessage_KeyDown:
			if (inParam1 == LIcKeyCode_Escape)
			{
				if (ONrLevel_GetCurrentLevel() != 0)
				{
					// send Btn_Back as the message that the main menu wants to go away
					DMrDialog_Stop(inDialog, ONcReturnToGame);
				}
			}
			else if ((inParam1 == 'q') || (inParam1 == 'Q'))
			{
				DMrDialog_Stop(inDialog, ONcMM_Btn_Quit);
			}
		break;

		case VMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case ONcMM_Btn_NewGame:
					DMrDialog_Run(
						ONcDialogID_NewGame,
						ONrDialogCallback_NewGame,
						inDialog,
						&message);
					if (UUmLowWord(message) == ONcNG_Btn_Start)
					{
						DMrDialog_Stop(
							inDialog,
							UUmMakeLong(UUmHighWord(message), ONcMM_Btn_NewGame));
					}
				break;

				case ONcMM_Btn_LoadSave:
					DMrDialog_Run(
						ONcDialogID_LoadSave,
						ONrDialogCallback_LoadSave,
						inDialog,
						&message);
				break;

				case ONcMM_Btn_Multiplayer:
					DMrDialog_Run(
						ONcDialogID_Multiplayer,
						ONrDialogCallback_Multiplayer,
						inDialog,
						&message);
					if ((message == ONcMPJG_Btn_Join) ||
						(message == ONcMPHG_Btn_StartHost))
					{
						DMrDialog_Stop(inDialog, message);
					}
				break;

				case ONcMM_Btn_Options:
					DMrDialog_Run(
						ONcDialogID_Options,
						ONrDialogCallback_Options,
						inDialog,
						&message);
				break;

				case ONcMM_Btn_Quit:
					DMrDialog_Run(
						ONcDialogID_Quit,
						ONrDialogCallback_Quit,
						inDialog,
						&message);
					if (message == ONcQuit_Btn_Yes)
					{
						DMrDialog_Stop(inDialog, ONcMM_Btn_Quit);
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

// ----------------------------------------------------------------------
static UUtBool
ONrDialogCallback_PasswordSplashScreen(
	DMtDialog				*inDialog,
	DMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool					handled;
	UUtUns16				len;

#define cMaxPasswordChars	10
	static char				entered_password[cMaxPasswordChars + 1];

	handled = UUcTrue;
	len = strlen(entered_password);

	switch (inMessage)
	{
		case DMcMessage_InitDialog:
			UUrMemory_Clear(entered_password, sizeof(entered_password));
		break;

		case VMcMessage_KeyDown:
			if (isalnum(inParam1))
			{
				char		append_me[2];

				if (len < cMaxPasswordChars)
				{
					append_me[0] = (char) inParam1;
					append_me[1] = 0;

					strncat(entered_password, append_me, cMaxPasswordChars);
				}
			}
			else if (iscntrl(inParam1))
			{
				if (inParam1 == LIcKeyCode_BackSpace)
				{
					if (len > 0)
					{
						entered_password[len - 1] = '\0';
					}
				}
				else if ((inParam1 == LIcKeyCode_Return) || (inParam1 == LIcKeyCode_NumPadEnter))
				{
					UUtBool success;

					success =
						UUrOneWayFunction_CompareString(
							entered_password,
							(char*)ONgMungedPassword,
							len);

					DMrDialog_Stop(inDialog, success);
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
#if defined(ONmUnitViewer) && (ONmUnitViewer == 1)

void
ONrDialog_MainMenu(
	void)
{
	UUtUns32			message;

	if (ONgTerminateGame) return;

	// load level the UnitViewer Level
	ONrLevel_Load(ONcUnitViewer_Level);

	// display the bungie logo

	// display the oni logo

	// run the unit viewer dialog
	DMrDialog_Run(
		ONcDialogID_UnitViewer,
		ONrDialogCallback_UnitViewer,
		NULL,
		&message);

	// display the coming soon picture

	ONgTerminateGame = UUcTrue;
}

#else

void
ONrDialog_MainMenu(
	void)
{
	UUtError			error;
	UUtUns32			message;

	if (ONgTerminateGame) return;

	// make sure this is set to true
	ONgRunMainMenu = UUcTrue;

	do
	{
		ONrGameState_Pause(UUcTrue);
		DMrDialog_Run(
			ONcDialogID_MainMenu,
			ONrDialogCallback_MainMenu,
			NULL,
			&message);
		ONrGameState_Pause(UUcFalse);

		switch (UUmLowWord(message))
		{
			case ONcReturnToGame:
				ONgRunMainMenu = UUcFalse;
			break;

			case ONcMM_Btn_Quit:
				// terminate an active network session before quitting
				if (ONrNet_IsActive())
				{
					ONrNet_Terminate();
				}

				ONgTerminateGame = UUcTrue;
				ONgRunMainMenu = UUcFalse;
			break;

			case ONcMM_Btn_NewGame:
			{
				UUtUns16	new_level;

				// unload any level other than 0
				if (ONrLevel_GetCurrentLevel() != 0)
				{
					ONrLevel_Unload();
				}

				// get the new level to laod
				new_level = (UUtUns16)UUmHighWord(message);

				// try to load the new level
				error = ONrLevel_Load(new_level);
				if (error != UUcError_None)
					break;

				ONgRunMainMenu = UUcFalse;
			}
			break;

			case ONcMPHG_Btn_StartHost:
				if (ONiMP_HostGame() == UUcFalse)
					break;

				ONgRunMainMenu = UUcFalse;
			break;

			case ONcMPJG_Btn_Join:
			{
				UUtUns32			stop_time;

				if (ONiMP_JoinGame(ONgMP_PlayerInfo.host_address) == UUcFalse)
					break;

				ONrNet_Level_Callback_Register(ONiMP_Net_Level_Callback);

				stop_time = UUrMachineTime_GameTicks() + (10 * 60);

				while ((ONgJoining_LevelHasLoaded == UUcFalse) &&
						(stop_time > UUrMachineTime_GameTicks()))
				{
					ONrNet_Update();
				}

				if (ONgJoining_LevelHasLoaded)
				{
					// the game has joined a server and a level has been loaded
					// put the input into game mode
					LIrMode_Set(LIcMode_Game);

					ONgRunMainMenu = UUcFalse;
				}
			}
			break;
		}
	}
	while (ONgRunMainMenu);

	ONgMainMenu_SwitchTime = UUrMachineTime_GameTicks();
}

#endif

// ----------------------------------------------------------------------
void
ONrDialog_SplashScreen(
	void)
{
	DMtDialog			*dialog;
	UUtUns16			tryCount = 0;

#ifndef UUmPasswordProtect
#define UUmPasswordProtect 0
#endif

	if (UUmPasswordProtect)
	{
		UUtUns32		message;

tryAgain:
		DMrDialog_Run(
			ONcDialogID_SplashScreen,
			ONrDialogCallback_PasswordSplashScreen,
			NULL,
			&message);
		if (message == UUcFalse)
		{
			if(tryCount == 2)
			{
				TMrMameAndDestroy();
				UUrPlatform_MameAndDestroy();

				// quit the app
				ONgTerminateGame = UUcTrue;
				ONgRunMainMenu = UUcFalse;
			}
			else
			{
				tryCount++;
				goto tryAgain;
			}
		}
	}
	else
	{
		DMrDialog_Load(
			ONcDialogID_SplashScreen,
			NULL,
			NULL,
			&dialog);

		M3rGeom_Frame_Start(0);
		DMrDialog_Display(dialog);
		M3rGeom_Frame_End();

		DMrDialog_Stop(dialog, 0);
		DMrDialog_Update(dialog);
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
ONrDialog_Initialize(
	void)
{
	ONgMainMenu_SwitchTime = 0;

	UUrString_Copy(ONgMP_PlayerInfo.player_name, "Player", ONcMaxPlayerNameLength);
	UUrString_Copy(ONgMP_PlayerInfo.team_name, "Player Team", ONcMaxTeamNameLength);
	UUrString_Copy(ONgMP_PlayerInfo.character_class, "konoko_generic", AIcMaxClassNameLen);
	ONgMP_PlayerInfo.host_address[0]			= 0;
	ONgMP_PlayerInfo.password[0]				= 0;
	ONgMP_PlayerInfo.remember_password			= UUcFalse;

	UUrString_Copy(ONgMP_HostInfo.host_name, "Oni Server", ONcMaxHostNameLength);
	ONgMP_HostInfo.game_options.max_players		= 8;

	ONgMP_HostInfo.game_options.game_parameters	=
		(ONtNetGameParams)(/*ONcNetGameParam_TimeLimit |*/ ONcNetGameParam_HasAIs);
	ONgMP_HostInfo.game_options.time_limit		= 5 * 60 * 60;		// 5 minutes in ticks
	ONgMP_HostInfo.game_options.kill_limit		= 0;
	ONgMP_HostInfo.game_options.num_AIs			= 0;

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
ONrDialog_Terminate(
	void)
{
}

#endif
