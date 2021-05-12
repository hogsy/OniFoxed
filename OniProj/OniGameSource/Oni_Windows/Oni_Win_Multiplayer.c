// ======================================================================
// Oni_Win_Multiplayer.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_WindowManager.h"
#include "BFW_Timer.h"

#include "Oni_Windows.h"
#include "Oni_Networking.h"
#include "Oni_Character.h"
#include "Oni_AI_Setup.h"

// ======================================================================
// defines
// ======================================================================
#define OWcMaxHostAddressLength		16
#define OWcMaxPasswordLength		8

// ======================================================================
// typedefs
// ======================================================================
typedef struct OWtNet_ServerInfoArray
{
	ONtNet_ServerInfo		server_info;
	NMtNetAddress			server_address;
	
} OWtNet_ServerInfoArray;

typedef struct OWtMPHost_Info
{
	char					host_name[ONcMaxHostNameLength + 1];
	UUtUns16				level_number;
	ONtNetGameOptions		game_options;
	
} OWtMPHost_Info;

typedef struct OWtMPJoin_Info
{
	char					server_address[OWcMaxHostAddressLength + 1];
	char					password[OWcMaxPasswordLength + 1];
	UUtBool					remember_password;

} OWtMPJoin_Info;

typedef struct OWtMP_PlayerInfo
{
	char					team_name[ONcMaxTeamNameLength + 1];
	
} OWtMP_PlayerInfo;

// ======================================================================
// globals
// ======================================================================
static UUtMemory_Array		*OWgMPJoin_Servers;

static OWtMPHost_Info		OWgMPHost_Info;
static OWtMPJoin_Info		OWgMPJoin_Info;
static OWtMP_PlayerInfo		OWgMP_PlayerInfo;

UUtBool						OWgMPJoin_LevelHasLoaded;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
OWiMP_JoinServer(
	char					*inHostAddress)
{
	UUtError				error;
	
	// start a client
	error = ONrNet_Client_Start(ONcNet_DefaultClientPort);
	if ((error != UUcError_None) && (error != UUcError_Client_AlreadyStarted))
	{
		UUrError_Report(error, "Unable to start the client");
		return UUcFalse;
	}
	
	// join the host
	error =
		ONrNet_Client_Join(
			ONcNet_DefaultServerPort,
			inHostAddress,
			OWrSettings_GetCharacterClass(),
			OWrSettings_GetPlayerName(),
			OWgMP_PlayerInfo.team_name);
	if (error != UUcError_None)
	{
		UUrError_Report(error, "Unable to join the server");
		
		error = ONrNet_Client_Stop();
		if (error != UUcError_None)
		{
			UUrError_Report(error, "Unable to stop the client");
		}
		
		return UUcFalse;
	}
	
	return UUcTrue;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OWiMPHost_SaveInfo(
	WMtDialog				*inDialog)
{
	WMtWindow				*window;
	char					string[256];
	UUtUns32				string_length;
	UUtUns32				num_bots;
	UUtUns32				max_kills;
	UUtUns32				time_limit;
	
	OWgMPHost_Info.game_options.game_parameters = ONcNetGameParam_None;

	// save the host name
	window = WMrDialog_GetItemByID(inDialog, OWcMPHost_EF_HostName);
	if (window)
	{
		WMrMessage_Send(
			window,
			EFcMessage_GetText,
			(UUtUns32)OWgMPHost_Info.host_name,
			ONcMaxHostNameLength);
	}
	
	// get the level number
	OWgMPHost_Info.level_number = OWrLevelList_GetLevelNumber(inDialog, OWcMPHost_LB_Level);

	// get the number of bots
	window = WMrDialog_GetItemByID(inDialog, OWcMPHost_EF_NumBots);
	if (window)
	{
		string_length = WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, 255);
		sscanf((char*)string, "%d", &num_bots);
		if (num_bots > 0)
		{
			OWgMPHost_Info.game_options.game_parameters |= ONcNetGameParam_HasAIs;
			OWgMPHost_Info.game_options.num_AIs = (UUtUns16)num_bots;
		}
	}
	
	// save the max kills
	window = WMrDialog_GetItemByID(inDialog, OWcMPHost_EF_MaxKills);
	if (window)
	{
		string_length = WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, 255);
		sscanf((char*)string, "%d", &max_kills);
		if (max_kills > 0)
		{
			OWgMPHost_Info.game_options.game_parameters |= ONcNetGameParam_KillLimit;
			OWgMPHost_Info.game_options.kill_limit = (UUtUns16)max_kills;
		}
	}
	
	// save the num minutes
	window = WMrDialog_GetItemByID(inDialog, OWcMPHost_EF_NumMinutes);
	if (window)
	{
		string_length = WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, 255);
		sscanf((char*)string, "%d", &time_limit);
		if (time_limit > 0)
		{
			OWgMPHost_Info.game_options.game_parameters |= ONcNetGameParam_TimeLimit;
			OWgMPHost_Info.game_options.time_limit = (UUtUns16)time_limit;
		}
	}
}

// ----------------------------------------------------------------------
static void
OWiMPHost_SetInfo(
	WMtDialog				*inDialog)
{
	WMtWindow				*window;
	char					string[255];
	
	// set the host name
	window = WMrDialog_GetItemByID(inDialog, OWcMPHost_EF_HostName);
	if (window != NULL)
	{
		WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)OWgMPHost_Info.host_name, 0);
	}
	
	// set the number of bots
	window = WMrDialog_GetItemByID(inDialog, OWcMPHost_EF_NumBots);
	if (window != NULL)
	{
		sprintf(string, "%d", OWgMPHost_Info.game_options.num_AIs);
		WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);
	}
	
	// set the kill limit
	window = WMrDialog_GetItemByID(inDialog, OWcMPHost_EF_MaxKills);
	if (window != NULL)
	{
		sprintf(string, "%d", OWgMPHost_Info.game_options.kill_limit);
		WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);
	}
	
	// set the time limit
	window = WMrDialog_GetItemByID(inDialog, OWcMPHost_EF_NumMinutes);
	if (window != NULL)
	{
		sprintf(string, "%d", OWgMPHost_Info.game_options.time_limit);
		WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);
	}
}

// ----------------------------------------------------------------------
UUtBool
OWrMPHost_Callback(
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
			OWrLevelList_Initialize(inDialog, OWcMPHost_LB_Level);
			OWiMPHost_SetInfo(inDialog);
		break;
		
		case WMcMessage_Destroy:
			OWiMPHost_SaveInfo(inDialog);
		break;
		
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_OK:
					// only respond to clicks
					if (UUmHighWord(inParam1) != WMcNotify_Click) { break; }
					
					// save the MPHost info before deleting the dialog
					OWiMPHost_SaveInfo(inDialog);
					WMrWindow_Delete(inDialog);
					
					// post the message to start a multiplayer game
					WMrMessage_Post(NULL, OWcMessage_MPHost, 0, 0);
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
OWrMPHost_StartServer(
	void)
{
	UUtError				error;
	UUtBool					result;
	
	// make sure that hte level number is valid
	if ((OWgMPHost_Info.level_number == (UUtUns16)(-1)) ||
		(OWgMPHost_Info.level_number == 0))
	{
		return UUcFalse;
	}
	
	// start the server
	error =
		ONrNet_Server_Start(
			ONcNet_DefaultServerPort,
			OWgMPHost_Info.host_name,
			&OWgMPHost_Info.game_options,
			OWgMPHost_Info.level_number);
	if (error != UUcError_None)
	{
		UUrError_Report(error, "Unable to start host");
		return UUcFalse;
	}
	
	// join the host on this machine
	result = OWiMP_JoinServer(ONrNet_Server_GetAddress());
	if (result == UUcFalse)
	{
		error = ONrNet_Server_Stop();
		if (error != UUcError_None)
		{
			UUrError_Report(error, "Unable to stop the host");
		}
		
		return UUcFalse;
	}
	
	// run the game
	WMrMessage_Post(NULL, OWcMessage_RunGame, 0, 0);

	return UUcTrue;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OWiMPJoin_FindServers_Callback(
	ONtNet_ServerInfo		*inServerInfo,
	NMtNetAddress			*inServerAddress,
	UUtUns32				inUserParam)
{
	UUtError				error;
	WMtDialog				*mp_join_dialog;
	WMtListBox				*server_listbox;
	OWtNet_ServerInfoArray	*server_info_array;
	UUtUns32				num_servers;
	UUtBool					server_in_list;
	UUtUns32				index;
	UUtBool					mem_moved;
	
	UUmAssert(inServerInfo);
	UUmAssert(inServerAddress);
	
	UUmAssert(OWgMPJoin_Servers);
	if (OWgMPJoin_Servers == NULL) { return; }
	
	// get a pointer to the dialog
	mp_join_dialog = (WMtDialog*)inUserParam;
	UUmAssert(mp_join_dialog);
	if (mp_join_dialog == NULL) { return; }
	
	// get a pointer to the server listbox
	server_listbox = WMrDialog_GetItemByID(mp_join_dialog, OWcMPJoin_LB_Servers);
	UUmAssert(server_listbox);
	if (server_listbox == NULL) { return; }
	
	// get a pointer to the server info array
	server_info_array = (OWtNet_ServerInfoArray*)UUrMemory_Array_GetMemory(OWgMPJoin_Servers);
	num_servers = UUrMemory_Array_GetUsedElems(OWgMPJoin_Servers);
	
	// check to see if this server's info has already been received
	server_in_list = UUcFalse;
	for (index = 0; index < num_servers; index++)
	{
		UUtUns16			result;
		
		result =
			strcmp(
				server_info_array[index].server_info.server_name,
				inServerInfo->server_name);
		if (result == 0)
		{
			server_in_list = UUcTrue;
			break;
		}
	}
	
	if (server_in_list == UUcTrue) { return; }
	
	// add the server info to the array
	error =
		UUrMemory_Array_GetNewElement(
			OWgMPJoin_Servers,
			&index,
			&mem_moved);
	if (error != UUcError_None) return;
	
	// get a fresh pointer to the memory
	server_info_array = (OWtNet_ServerInfoArray*)UUrMemory_Array_GetMemory(OWgMPJoin_Servers);
	UUmAssert(server_info_array);
	if (server_info_array == NULL) { return; }
	
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
	WMrMessage_Send(
		server_listbox,
		LBcMessage_InsertString,
		(UUtUns32)inServerInfo->server_name,
		index);
}

// ----------------------------------------------------------------------
static void
OWiMPJoin_SaveInfo(
	WMtDialog				*inDialog)
{
	WMtWindow				*listbox;
	OWtNet_ServerInfoArray	*server_info_array;
	UUtUns32				current_selection;
	char					*server_address_string;
	
	// get the server address
	listbox = WMrDialog_GetItemByID(inDialog, OWcMPJoin_LB_Servers);
	if (listbox == NULL) { return; }
	
	// get the current selection
	current_selection = WMrMessage_Send(listbox, LBcMessage_GetSelection, 0, 0);
	
	// get the server address
	if (current_selection == (UUtUns32)(-1)) { return; }
	
	// get the server info array
	server_info_array = (OWtNet_ServerInfoArray*)UUrMemory_Array_GetMemory(OWgMPJoin_Servers);
	if (server_info_array == NULL) { return; }
	
	server_address_string =
		NMrAddressToString(&server_info_array[current_selection].server_address);
	
	// save the address of the server
	UUrString_Copy(OWgMPJoin_Info.server_address, server_address_string, OWcMaxHostAddressLength);	
}

// ----------------------------------------------------------------------
static void
OWiMPJoin_Initialize(
	WMtDialog				*inDialog)
{
	UUtBool					status;
	
	status = WMrTimer_Start(20, 1, inDialog);
	if (status == UUcFalse) { return; }
	
	// allocate memory for the server info
	if (OWgMPJoin_Servers == NULL)
	{
		OWgMPJoin_Servers = 
			UUrMemory_Array_New(
				sizeof(OWtNet_ServerInfoArray),
				1,
				0,
				0);
	}
	
	// find the available servers
	ONrNet_FindServers_Start(OWiMPJoin_FindServers_Callback, (UUtUns32)inDialog);
}

// ----------------------------------------------------------------------
static void
OWiMPJoin_Destroy(
	WMtDialog				*inDialog)
{
	// save the info
	OWiMPJoin_SaveInfo(inDialog);

	// stop finding servers
	ONrNet_FindServers_Stop();
	
	// stop the timer
	WMrTimer_Stop(20, inDialog);
	
	// release the server info array
	if (OWgMPJoin_Servers)
	{
		UUrMemory_Array_Delete(OWgMPJoin_Servers);
		OWgMPJoin_Servers = NULL;
	}
}

// ----------------------------------------------------------------------
static void
OWiMPJoin_LevelLoaded_Callback(
	UUtBool					inIsLoading,
	UUtUns16				inLevelNumber)
{
	if (inIsLoading)
	{
		OWgMPJoin_LevelHasLoaded = UUcTrue;
	}
	else
	{
		OWgMPJoin_LevelHasLoaded = UUcFalse;
	}
}

// ----------------------------------------------------------------------
UUtBool
OWrMPJoin_Callback(
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
			OWiMPJoin_Initialize(inDialog);
		break;
		
		case WMcMessage_Destroy:
			OWiMPJoin_Destroy(inDialog);
		break;
				
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_OK: /* join button */
					// only respond to clicks
					if (UUmHighWord(inParam1) != WMcNotify_Click) { break; }
				
					// save the join information
					OWiMPJoin_SaveInfo(inDialog);
					WMrWindow_Delete(inDialog);

					OWgMPJoin_LevelHasLoaded = UUcFalse;
					
					// post the join message
					WMrMessage_Post(NULL, OWcMessage_MPJoin, 0, 0);
				break;
			}
		break;
		
		case WMcMessage_Timer:
			ONrNet_FindServers_Update();
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ----------------------------------------------------------------------
void
OWrMPJoin_HandleMessage(
	void)
{
	UUtUns32			stop_time;

	// join the server
	if (OWiMP_JoinServer(OWgMPJoin_Info.server_address) == UUcFalse)
	{
		return;
	}
	
	// register the callback
	ONrNet_Level_Callback_Register(OWiMPJoin_LevelLoaded_Callback);
	
	// know when to stop
	stop_time = UUrMachineTime_Sixtieths() + (10 * 60);
	
	// wait for the level to load
	while ((OWgMPJoin_LevelHasLoaded == UUcFalse) &&
			(stop_time > UUrMachineTime_Sixtieths()))
	{
		ONrNet_Update_Receive();
		ONrNet_Update_Send();
	}
	
	// run the game if the level has loaded
	if (OWgMPJoin_LevelHasLoaded)
	{
		WMrMessage_Post(NULL, OWcMessage_RunGame, 0, 0);
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
OWrMP_Initialize(
	void)
{
	// Initialize Host Information
	UUrString_Copy(OWgMPHost_Info.host_name, "Oni Server", ONcMaxHostNameLength);
	OWgMPHost_Info.level_number = (UUtUns16)(-1);
	OWgMPHost_Info.game_options.max_players = 8;
	OWgMPHost_Info.game_options.game_parameters = ONcNetGameParam_None;
	OWgMPHost_Info.game_options.time_limit = 0;
	OWgMPHost_Info.game_options.kill_limit = 0;
	OWgMPHost_Info.game_options.num_AIs = 0;
	
	OWgMPJoin_Info.server_address[0] = '\0';
	OWgMPJoin_Info.password[0] = '\0';
	OWgMPJoin_Info.remember_password = UUcFalse;
	
	UUrString_Copy(OWgMP_PlayerInfo.team_name, "team_name", ONcMaxTeamNameLength);
	
	return UUcError_None;
}
