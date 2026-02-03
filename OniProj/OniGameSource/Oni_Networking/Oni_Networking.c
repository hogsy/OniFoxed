// ======================================================================
// Oni_Networking.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_NetworkManager.h"
#include "BFW_Console.h"
#include "BFW_TextSystem.h"
#include "BFW_ScriptLang.h"
#include "BFW_Timer.h"


#include "Oni.h"
#include "Oni_Networking.h"
#include "Oni_Net_Private.h"
#include "Oni_Net_Support.h"
#include "Oni_Level.h"

// ======================================================================
// defines
// ======================================================================
#define ONcNet_DefaultMaxNumPlayers		8
#define ONcNet_MaxNumPlayers			32

#define ONcNet_MaxRequests				3
#define ONcNet_Request_Delta			(2 * 60) /* in ticks */

#define ONcNet_DelinquentDelta			(7 * 60) /* in ticks */
#define ONcNet_HeartbeatDelta			(3 * 60) /* in ticks */

#define ONcNet_FindServer_Broadcast_Interval	(5 * 60) /* in ticks */

// ======================================================================
// typedef
// ======================================================================
typedef struct ONtNet_Performance
{
	// display
	UUtBool						display;
	M3tPointScreen				dest[2];
	M3tTextureCoord				uv[4];
	TStTextContext				*text_context;
	M3tTextureMap				*texture_map;
	UUtRect						bounds;

	// client
	UUtUns32					client_packets_sent;
	UUtUns32					client_bytes_sent;

	// server
	UUtUns32					raw_packets_sent;
	UUtUns32					raw_bytes_sent;

	UUtUns32					player_packets_sent;
	UUtUns32					player_bytes_sent;

	UUtUns32					mass_packets_sent;
	UUtUns32					mass_bytes_sent;

} ONtNet_Performance;

// ======================================================================
// globals
// ======================================================================
static ONtNet_Common			*ONgNet_Common;
static ONtNet_Client			*ONgNet_Client;
static ONtNet_Server			*ONgNet_Server;
static ONtNet_FindServer		*ONgNet_FindServer;

static UUtBool 					ONgNet_No_Disconnect;
UUtBool							ONgNet_Authoritative_Server;

ONtDebugLevel					ONgNet_Verbose = ONcDebugLevel_None;
UUtUns32						ONgNet_SpawnDelta;

static ONtNet_Level_Callback	ONgNet_Level_Callback;

static UUtBool					ONgNet_Client_Stop = UUcFalse;

static ONtNet_Performance		ONgNet_Performance;

static UUtInt32					ONgNet_PPS_Target;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
ONiDebugLevel_Callback(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	char				*levels[ONcNumDebugLevels] = { "none", "major", "medium", "minor" };
	char				*command_format = "%s [none|major|medium|minor]";

	if (inParameterListLength == 0)
	{
		// display the current value
		COrConsole_Printf("%s = %s", "debug_level", levels[(UUtUns32)ONgNet_Verbose]);
	}
	else
	{
		UUtUns16		i;

		// set a new value
		for (i = 0; i < ONcNumDebugLevels; i++)
		{
			if (strcmp(levels[i], inParameterList[0].val.str) == 0)
			{
				ONgNet_Verbose = (ONtDebugLevel)i;
				break;
			}
		}
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiNet_Client_Start_Callback(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	UUtError			error;

	// start the client
	error = ONrNet_Client_Start(ONcNet_DefaultClientPort);
	if (error == UUcError_None)
	{
		COrConsole_Printf("client started.");
	}
	else
	{
		COrConsole_Printf("Unable to start client.");
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiNet_Client_Stop_Callback(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	UUtError			error;

	// stop the client
	error = ONrNet_Client_Stop();
	if (error == UUcError_None)
	{
		COrConsole_Printf("client stopping.");
	}
	else
	{
		COrConsole_Printf("Unable to stop client.");
	}
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiNet_Server_Start_Callback(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	UUtError			error;
	UUtInt32			max_num_players;
	ONtNetGameOptions	options;

	// set the maximum number of players
	max_num_players = ONcNet_DefaultMaxNumPlayers;

	// check to see if the user wants to set a different maximum number of players
	if (inParameterListLength == 1)
	{
		max_num_players = inParameterList[0].val.i;

		if (max_num_players > ONcNet_MaxNumPlayers)
		{
			COrConsole_Printf(
				"The maxinum number players must be less than %d",
				ONcNet_MaxNumPlayers);
			return UUcError_None;
		}
		else if (max_num_players < 2)
		{
			COrConsole_Printf("The maximun number of players must be at least 2");
			return UUcError_None;
		}
	}

	// start the server
	options.max_players			= (UUtUns16)max_num_players;
	options.game_parameters		= ONcNetGameParam_None;
	options.time_limit			= 0;
	options.kill_limit			= 0;

	error =
		ONrNet_Server_Start(
			ONcNet_DefaultServerPort,
			"Host 1",
			&options,
			1);
	if (error == UUcError_None)
	{
		COrConsole_Printf("Server started.");
	}
	else
	{
		COrConsole_Printf("Unable to start the server.");
	}
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiNet_Server_Stop_Callback(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	UUtError			error;

	// stop the server
	error = ONrNet_Server_Stop();
	if (error == UUcError_None)
	{
		COrConsole_Printf("Server stopped.");
	}
	else
	{
		COrConsole_Printf("Unable to stop the server.");
	}
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiNet_Join_Callback(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	UUtError			error;
	char				*character_class_name;

	// process the character class if it was supplied
	if (inParameterListLength == 2)
	{
		ONtCharacterClass		*temp;

		// set the class name
		character_class_name = inParameterList[1].val.str;

		// get a pointer to the character class
		error =
			TMrInstance_GetDataPtr(
				TRcTemplate_CharacterClass,
				(char*)character_class_name,
				&temp);
		if (error != UUcError_None)
		{
			COrConsole_Printf(
				"%s is not a valid character class name",
				character_class_name);
			return UUcError_None;
		}
	}
	else
	{
		// set the character class name
		character_class_name = "konoko_generic";
	}

	// start the client if it isn't already running
	if (ONgNet_Client == NULL)
	{
		ONiNet_Client_Start_Callback(inErrorContext, 0, NULL, NULL, NULL, NULL);
	}

	// join the specified host
	error =
		ONrNet_Client_Join(
			ONcNet_DefaultServerPort,
			inParameterList[0].val.str,
			character_class_name,
			"Player 1",
			"Player 1 Team");
	if (error == UUcError_None)
	{
		COrConsole_Printf("Connecting to desired address.");
	}
	else
	{
		COrConsole_Printf("Unable to connect to desired address.");
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
ONrNet_LevelLoad(
	ONtNet_Common			*ioCommon,
	UUtUns16				inLevelNumber)
{
	UUtError				error;

	// unload the current level
	if (ONrLevel_GetCurrentLevel() != 0)
	{
		ONrLevel_Unload();
	}

	// load the new level
	error = ONrLevel_Load(inLevelNumber, UUcTrue);
	UUmError_ReturnOnError(error);

	// reset the charaters
	ONrGameState_ResetCharacters();

	// save the level number
	ioCommon->level_number = inLevelNumber;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void UUcExternal_Call
ONiNet_Terminate_AtExit(
	void)
{
	// terminate the networking
	ONrNet_Terminate();
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
ONiNet_Common_Initialize(
	ONtNet_Common			*ioNetCommon)
{
	UUmAssert(ioNetCommon);

	// clear the common structure
	UUrMemory_Clear(ioNetCommon, sizeof(ONtNet_Common));

	// get a pointer to the character list
	ioNetCommon->character_list = ONrGameState_GetCharacterList();

	// set the game time
	ioNetCommon->game_time = UUrMachineTime_Sixtieths();

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
ONiNet_Common_Terminate(
	ONtNet_Common			*ioNetCommon)
{
	UUmAssert(ioNetCommon);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
ONrNet_Client_Request_Add(
	ONtNet_Client			*ioClient,
	ONtNet_RequestType		inRequestType,
	UUtUns32				inGameTime)
{
	UUtUns16				i;

	for (i = 0; i < ONcNet_MaxClientRequests; i++)
	{
		ONtNet_Request		*request;

		// get a local pointer to the request
		request = &ioClient->requests[i];

		if (request->request_type == ONcNet_Request_None)
		{
			request->request_type		= inRequestType;
			request->request_counter	= ONcNet_MaxRequests + 1;
			request->request_id			= ioClient->request_id++;
			request->request_time		= inGameTime;
			break;
		}
	}

	if (i == ONcNet_MaxClientRequests) UUmAssert(!"No requests available");
}

// ----------------------------------------------------------------------
static void
ONiNet_Client_Request_Failed(
	ONtNet_Client			*ioClient)
{
	// inform the user
	COrConsole_Printf("General networking failure: disconnected from host");
}

// ----------------------------------------------------------------------
void
ONrNet_Client_Request_ResponseReceived(
	ONtNet_Client			*ioClient,
	UUtUns32				inRequestID)
{
	UUtUns16				i;

	for (i = 0; i < ONcNet_MaxClientRequests; i++)
	{
		ONtNet_Request		*request;

		// get a local pointer to the request
		request = &ioClient->requests[i];

		if (request->request_id == inRequestID)
		{
			// this request is no longer being used
			request->request_type = ONcNet_Request_None;
			break;
		}
	}
}

// ----------------------------------------------------------------------
static UUtUns32
ONiNet_Client_Request_Update(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	UUtUns16				i;
	UUtBool					failed_request;

	failed_request = 0;

	for (i = 0; i < ONcNet_MaxClientRequests; i++)
	{
		ONtNet_Request		*request;

		// get a local pointer to the request
		request = &ioClient->requests[i];

		// if this is an active request and it needs to be updated
		if ((request->request_type != ONcNet_Request_None) &&
			(request->request_time < ioProcessGroup->common->game_time))
		{
			// update the request time and count
			request->request_time = ioProcessGroup->common->game_time + ONcNet_Request_Delta;
			request->request_counter--;

			// when the counter reaches 0, stop processing this packet
			if (request->request_counter == 0)
			{
				// call the callback
				ONiNet_Client_Request_Failed(ioClient);

				// note the failed request
				failed_request = request->request_type;

				// this request is no longer being used
				request->request_type = ONcNet_Request_None;

				// exit the for loop
				break;
			}

			// make a request on behalf of this counter
			switch (request->request_type)
			{
				case ONcNet_Request_Character_Create:
					ONrNet_CS_Request_CharacterCreate(ioClient, ioProcessGroup, request->request_id);
				break;

				case ONcNet_Request_Info_Server:
				break;

				case ONcNet_Request_Join:
					ONrNet_CS_Request_Join(ioClient, ioProcessGroup, request->request_id);
				break;

				case ONcNet_Request_Quit:
					ONrNet_CS_Request_Quit(ioClient, ioProcessGroup, request->request_id);
				break;
			}
		}
	}

	return failed_request;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
ONiNet_Client_Heartbeat(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	// don't update if there is no character
	if (ioClient->character == NULL) { return UUcTrue; }

	// make sure the server is still talking to the client
	if (ONgNet_No_Disconnect == UUcFalse)
	{
		if ((ioClient->previous_packet_time + ONcNet_DelinquentDelta) <
			ioProcessGroup->common->game_time)
		{
			COrConsole_Printf("Client: host not responding, disconnecting from host");

			// server hasn't sent a message in the required amount of time,
			// disconnect from the server
			ONrNet_Client_Stop();
			return UUcFalse;
		}
	}

	// send a heartbeat to the server
	if ((ioClient->heartbeat_time + ONcNet_HeartbeatDelta) <
		ioProcessGroup->common->game_time)
	{
		ioClient->heartbeat_time = ioProcessGroup->common->game_time + ONcNet_HeartbeatDelta;

		ONrNet_CS_Heartbeat(ioClient, ioProcessGroup);
	}

	return UUcTrue;
}

// ----------------------------------------------------------------------
static UUtError
ONiNet_Client_Initialize(
	ONtNet_Client			*ioClient,
	UUtUns16				inClientPortNumber)
{
	UUtError				error;

	UUmAssert(ioClient);

	// clear the client structure
	UUrMemory_Clear(ioClient, sizeof(ONtNet_Client));

	// initialize some variables
	ioClient->player_index = ONcNet_UnknownPlayer;
	ioClient->packet_number = 1;
	ONrNet_Packet_Init(&ioClient->packet_data);

	// create a client context
	error =
		NMrNetContext_New(
			NMcUDP,
			inClientPortNumber,
			NMcFlag_None,
			&ioClient->client_context);
	UUmError_ReturnOnErrorMsg(error, "Unable to create client context.");

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
ONrNet_Client_JoinStage_Advance(
	ONtNet_Client			*ioClient,
	ONtNet_Common			*ioCommon)
{
	switch (ioClient->join_stage)
	{
		case ONcNet_JoinStage_GameInfo:
			// advance to the next join stage
			ioClient->join_stage = ONcNet_JoinStage_CreateCharacter;

			// request that the character be created
			ONrNet_Client_Request_Add(
				ioClient,
				ONcNet_Request_Character_Create,
				ioCommon->game_time);
		break;

		case ONcNet_JoinStage_CreateCharacter:
			// the character has been created, joining has completed
			ioClient->join_stage = ONcNet_JoinStage_None;
		break;
	}
}

// ----------------------------------------------------------------------
void
ONrNet_Client_Rejected(
	ONtNet_Client			*ioClient)
{
	// stop the client
	ONrNet_Client_Stop();

	// inform the user
	COrConsole_Printf("Unable to connect with host");

	// unload the level
	ONrLevel_Unload();

	// run the main menu
	ONgRunMainMenu = UUcTrue;
}

// ----------------------------------------------------------------------
static void
ONiNet_Client_Spawn(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	// if the client's character exists and it is dead and the
	// minimum amount of time it can be dead has passed and the character
	// has pushed the fire/kick key, then send a respawn request
	if ((ioClient->character != NULL) &&
		(ioClient->character->flags & ONcCharacterFlag_Dead) &&
		((ioClient->character->time_of_death + ONgNet_SpawnDelta) < ioProcessGroup->common->game_time) &&
		((ioClient->character->inputState.buttonIsDown & LIc_BitMask_Kick) ||
		 (ioClient->character->inputState.buttonIsDown & LIc_BitMask_Fire1)))
	{
		ONrNet_CS_Request_Spawn(ioClient, ioProcessGroup);
	}
}

// ----------------------------------------------------------------------
static UUtError
ONiNet_Client_Terminate(
	ONtNet_Client			*ioClient)
{
	UUtError				error;

	UUmAssert(ioClient);

	// stop the client
	error = NMrNetContext_CloseProtocol(ioClient->client_context);
	UUmError_ReturnOnError(error);

	// delete the client context
	NMrNetContext_Delete(&ioClient->client_context);
	ioClient->client_context = NULL;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
ONiNet_Client_Update_Receive(
	ONtNet_Client			*ioClient,
	ONtNet_Common			*ioCommon)
{
	ONtNet_ProcessGroup		process_group;

	if (ioClient == NULL) return;
	UUmAssert(ioCommon);

	// ------------------------------
	// update the network context
	// ------------------------------
	NMrNetContext_Update(ioClient->client_context);

	// ------------------------------
	// process the incoming packets
	// ------------------------------
	// set up the process group
	process_group.common		= ioCommon;

	// process all available packets
	while (
		NMrNetContext_ReadData(
			ioClient->client_context,
			&process_group.from,
			(UUtUns8*)&process_group.packet,
			&process_group.packet_length))
	{
		// process the packet
		ONrNet_Client_ProcessPacket(ioClient, &process_group);
	}
}

// ----------------------------------------------------------------------
static void
ONiNet_Client_Update_Send(
	ONtNet_Client			*ioClient,
	ONtNet_Common			*ioCommon)
{
	ONtNet_ProcessGroup		process_group;
	UUtUns32				failed_request;

	if (ioClient == NULL) return;
	UUmAssert(ioCommon);

	// ------------------------------
	// only send when it's time
	// ------------------------------
	if ((ioClient->client_send_time + ioClient->client_send_delta) > ioCommon->game_time)
	{
		return;
	}
	ioClient->client_send_time = ioCommon->game_time;

	// ------------------------------
	// update the requests
	// ------------------------------
	// set up the process group
	process_group.common		= ioCommon;

	failed_request = ONiNet_Client_Request_Update(ioClient, &process_group);
	if (failed_request != ONcNet_Request_None)
	{
		COrConsole_Printf("Client: request failed, disconnecting from host");

		// the request failed to get an answer
		ONrNet_Client_Stop();
		return;
	}

	// ------------------------------
	// Build the update packet
	// ------------------------------
	if (ONrNet_IsServer() == UUcFalse)
	{
		ONrNet_CS_Update_Character(ioClient, &process_group);

		ONrNet_CS_Update_Critical(ioClient, &process_group);
	}

	ONiNet_Client_Spawn(ioClient, &process_group);
	if (ONiNet_Client_Heartbeat(ioClient, &process_group) == UUcFalse) { return; }

	// ------------------------------
	// send the update packet
	// ------------------------------
	ONrNet_Client_Packet_Send(ioClient);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
ONrNet_Client_Join(
	UUtUns16				inServerPortNumber,
	char					*inServerAddressString,
	char					*inCharacterClass,
	char					*inPlayerName,
	char					*inTeamName)
{
	UUtError				error;

	UUmAssert(ONgNet_Client);
	UUmAssert(inServerAddressString);

	// change the address string into a real address
	error =
		NMrNetContext_StringToAddress(
			ONgNet_Client->client_context,
			inServerAddressString,
			inServerPortNumber,
			&ONgNet_Client->server_address);
	UUmError_ReturnOnErrorMsg(error, "Invalid ip address");

	// put the client in join mode
	ONgNet_Client->join_stage = ONcNet_JoinStage_GameInfo;

	// copy the information into the client
	ONgNet_Client->teamNumber = 0xFFFF;
	UUrString_Copy(
		ONgNet_Client->characterClassName,
		inCharacterClass,
		AIcMaxClassNameLen);
	UUrString_Copy(
		ONgNet_Client->playerName,
		inPlayerName,
		ONcMaxPlayerNameLength);

	// request join
	ONrNet_Client_Request_Add(
		ONgNet_Client,
		ONcNet_Request_Join,
		ONgNet_Common->game_time);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
ONrNet_Client_Start(
	UUtUns16				inClientPortNumber)
{
	UUmAssert(ONgNet_Client == NULL);

	// allocate memory for the common data
	if (ONrNet_IsServer() == UUcFalse)
	{
		ONgNet_Common = UUrMemory_Block_New(sizeof(ONtNet_Common));
		UUmError_ReturnOnNull(ONgNet_Common);
	}

	// allocate memory for the client data
	ONgNet_Client = UUrMemory_Block_New(sizeof(ONtNet_Client));
	UUmError_ReturnOnNull(ONgNet_Client);

	// initialize the data
	if (ONrNet_IsServer() == UUcFalse)
	{
		ONiNet_Common_Initialize(ONgNet_Common);
	}
	ONiNet_Client_Initialize(ONgNet_Client, inClientPortNumber);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
ONrNet_Client_Stop(
	void)
{
	UUtError				error;
	UUtUns16				i;

	if (ONgNet_Client == NULL) return UUcError_None;

	// prevent multiple entries into this function
	if (ONgNet_Client_Stop) return UUcError_None;
	ONgNet_Client_Stop = UUcTrue;

	// tell the server the client is leaving
	ONrNet_Client_Request_Add(
		ONgNet_Client,
		ONcNet_Request_Quit,
		ONgNet_Common->game_time);
	for (i = 0; i < 10; i++)
	{
		ONiNet_Client_Update_Receive(ONgNet_Client, ONgNet_Common);
		ONiNet_Client_Update_Send(ONgNet_Client, ONgNet_Common);
	}

	// terminate the client data
	error = ONiNet_Client_Terminate(ONgNet_Client);
	if (error != UUcError_None)
	{
		ONgNet_Client_Stop = UUcFalse;
		UUmError_ReturnOnError(error);
	}

	// delete the client data
	UUrMemory_Block_Delete(ONgNet_Client);
	ONgNet_Client = NULL;

	// terminate the common data
	if ((ONrNet_IsServer() == UUcFalse) && (ONgNet_Common))
	{
		ONiNet_Common_Terminate(ONgNet_Common);
		UUrMemory_Block_Delete(ONgNet_Common);
		ONgNet_Common = NULL;
	}

	ONgNet_Client_Stop = UUcFalse;
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
ONiNet_Server_CreateAIs(
	ONtNet_Server			*ioServer,
	ONtNet_Common			*ioCommon,
	ONtNetGameOptions		*inGameOptions)
{
	UUtUns16				i;
	UUtError				error;
	const ONtFlag			*prev_flag;

	// don't make more AIs than spawn points available
	if (inGameOptions->num_AIs > ONgLevel->spawnArray->numSpawnFlagIDs)
	{
		inGameOptions->num_AIs = ONgLevel->spawnArray->numSpawnFlagIDs;
	}

	// generate the AIs
	prev_flag = NULL;
	for (i = 0; i < inGameOptions->num_AIs; i++)
	{
		const AItCharacterSetup		*setup;
		ONtFlag						flag;
		ONtCharacter				*character;
		UUtUns16					random_index;
		UUtUns16					character_id;

		// get a random character setup from the level's characterSetupArray
		random_index = UUmRandomRange(1, ONgLevel->characterSetupArray->numCharacterSetups - 1);
		setup = &ONgLevel->characterSetupArray->characterSetups[random_index];

		// get a random flag that the character can start at
		ONrLevel_Flag_ID_To_Flag(ONrGameState_GetPlayerStart(), &flag);

		// create the AI
		error = ONrGameState_NewCharacter(NULL, setup, &flag, &character_id);
		if (error != UUcError_None) return error;

		character = &ioCommon->character_list[character_id];
		character->teamNumber = i;
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiNet_Server_Initialize(
	ONtNet_Server			*ioServer,
	UUtUns16				inServerPortNumber,
	ONtNetGameOptions		*inGameOptions)
{
	UUtError				error;

	UUmAssert(ioServer);

	// clear the ioServer structure
	UUrMemory_Clear(ioServer, sizeof(ONtNet_Server));

	// create the server context
	error =
		NMrNetContext_New(
			NMcUDP,
			inServerPortNumber,
			NMcFlag_None,
			&ioServer->server_context);
	UUmError_ReturnOnErrorMsg(error, "Unable to create server context");

	// allocate memory for the players
	ioServer->players =
		 (ONtNet_Players*)UUrMemory_Block_NewClear(
		 	sizeof(ONtNet_Players) * inGameOptions->max_players);
	UUmError_ReturnOnNull(ioServer->players);

	// init some of the vars
	ioServer->game_options = *inGameOptions;
	ioServer->packet_number = 1;
	ONrNet_Packet_Init(&ioServer->mass_packet_data);

	// create the server findserver context
	error =
		NMrNetContext_New(
			NMcUDP,
			ONcNet_FindServerPort,
			NMcFlag_Broadcast,
			&ioServer->findserver_context);
	UUmError_ReturnOnErrorMsg(error, "Unable to create findserver context");

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiNet_Server_Terminate(
	ONtNet_Server			*ioServer)
{
	UUtError				error;

	UUmAssert(ioServer);

	// close the server context
	error = NMrNetContext_CloseProtocol(ioServer->server_context);
	UUmError_ReturnOnError(error);

	// delete the players
	UUrMemory_Block_Delete(ioServer->players);

	// delete the server context
	NMrNetContext_Delete(&ioServer->server_context);
	ioServer->server_context = NULL;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
ONiNet_Server_UpdateAI(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup,
	ONtCharacter			*ioCharacter)
{
	// if the AI is dead and has been dead long enough, then respawn the AI
	if ((ioCharacter->flags & ONcCharacterFlag_Dead) &&
		((ioCharacter->time_of_death + ONgNet_SpawnDelta) < ioProcessGroup->common->game_time))
	{
		const AItCharacterSetup		*setup;
		UUtUns16					random_index;

		// ------------------------------
		// spawn the AI
		// ------------------------------
		ONrCharacter_Spawn(ioCharacter);

		// ------------------------------
		// change to a random character class
		// ------------------------------

		// get a random character setup from the level's characterSetupArray
		random_index = UUmRandomRange(1, ONgLevel->characterSetupArray->numCharacterSetups - 1);
		setup = &ONgLevel->characterSetupArray->characterSetups[random_index];

		ONrCharacter_SetCharacterClass(ioCharacter, setup->characterClass);
	}
}

// ----------------------------------------------------------------------
static void
ONiNet_Server_Update_Receive(
	ONtNet_Server			*ioServer,
	ONtNet_Common			*ioCommon)
{
	ONtNet_ProcessGroup		process_group;

	if (ioServer == NULL) return;
	UUmAssert(ioCommon);

	// ------------------------------
	// update the network context
	// ------------------------------
	NMrNetContext_Update(ioServer->server_context);
	NMrNetContext_Update(ioServer->findserver_context);

	// ------------------------------
	// process the incoming packets
	// ------------------------------
	// set up the process group
	process_group.common		= ioCommon;

	// process all available server packets
	while (
		NMrNetContext_ReadData(
			ioServer->server_context,
			&process_group.from,
			(UUtUns8*)&process_group.packet,
			&process_group.packet_length))
	{
		// process the packet
		ONrNet_Server_ProcessPacket(ioServer, &process_group);
	}

	// process all available findserver packets
	while (
		NMrNetContext_ReadData(
			ioServer->findserver_context,
			&process_group.from,
			(UUtUns8*)&process_group.packet,
			&process_group.packet_length))
	{
		// process the packet
		ONrNet_Server_FS_ProcessPacket(ioServer, &process_group);
	}
}

// ----------------------------------------------------------------------
static void
ONiNet_Server_Update_Send(
	ONtNet_Server			*ioServer,
	ONtNet_Common			*ioCommon)
{
	ONtNet_ProcessGroup		process_group;
	ONtCharacter			*character;
	UUtUns16				i;
	UUtUns16				j;

	if (ioServer == NULL) return;
	UUmAssert(ioCommon);

	// ------------------------------
	// only send when it's time
	// ------------------------------
	if ((ioServer->server_send_time + ioServer->server_send_delta) > ioCommon->game_time)
	{
		return;
	}
	ioServer->server_send_time = ioCommon->game_time;

	// ------------------------------
	// check for non-responsive clients
	// ------------------------------
	// set up the process group
	process_group.common		= ioCommon;

	if (ONgNet_No_Disconnect == UUcFalse)
	{
		for (i = 0; i < ioServer->game_options.max_players; i++)
		{
			if ((ioServer->players[i].inUse == UUcFalse) ||
				(ioServer->players[i].character == NULL))
			{
				continue;
			}

			// if this player needs to ack an update and it hasn't been heard from
			// in the designated amount of time, then it is gone.
			if ((ioServer->players[i].previous_packet_time + ONcNet_DelinquentDelta) < ioCommon->game_time)
			{
				ioServer->players[i].character->updateFlags |= ONcCharacterUpdateFlag_Gone;
			}
		}
	}

	// ------------------------------
	// Build the update packet
	// ------------------------------

	// update the weapons
	ONrNet_SS_Update_Weapons(ioServer, &process_group);

	// go through the characters and build the update packet
	for (i = 0; i < ONrGameState_GetNumCharacters(); i++)
	{
		UUtUns32			updateFlags;
		UUtBool				update_forced;
		UUtUns16			owning_player_index;

		// get a pointer to the character
		character = &ioCommon->character_list[i];

		update_forced = UUcFalse;
		owning_player_index = ONcNet_UnknownPlayer;

		// if the character is active, add to update list
		if (!(character->flags & ONcCharacterFlag_InUse)) continue;

		ONrNet_SS_Update_Character(
			ioServer,
			&process_group,
			character);

		ONrNet_SS_Update_Critical(
			ioServer,
			&process_group,
			character,
			&updateFlags);

		// if updateFlags == ONcCharacterUpdateFlag_None then no critical
		// update for the player was added by ONrNet_SS_Update_Critical.
		// If it did change then keep the old critical update flags for the
		// character so that a client that doesn't acknowledge receiving
		// the critical update can be sent another copy.
		if (updateFlags != ONcCharacterUpdateFlag_None)
		{
			// save the update flags that changed
			ioServer->updateFlags[i] |= updateFlags;
			update_forced = UUcTrue;
		}

		// set the need ACK field for the players
		for (j = 0; j < ioServer->game_options.max_players; j++)
		{
			if (ioServer->players[j].inUse == UUcFalse) continue;
			if (ioServer->players[j].character == character) owning_player_index = j;

			if (updateFlags != ONcCharacterUpdateFlag_None)
			{
				ioServer->players[j].updateACKNeeded[i] = UUcTrue;
			}
			else if (ioServer->players[j].updateACKNeeded[i])
			{
				ONrNet_SS_Update_Critical_Force(
					ioServer,
					&process_group,
					character,
					ioServer->updateFlags[i],
					j);

				update_forced = UUcTrue;
			}
		}

		// the Gone update flag is set when the player isn't responding.  Delete the
		// character and make the player is no longer in use
		if ((ioServer->updateFlags[i] & ONcCharacterUpdateFlag_Gone) &&
			(owning_player_index != ONcNet_UnknownPlayer))
		{
			ioServer->players[owning_player_index].inUse = UUcFalse;
			ONrGameState_DeleteCharacter(character);
			ioServer->players[owning_player_index].character = NULL;
		}

		// if the update wasn't forced then the character's updateFlags can
		// be cleared
		if (update_forced == UUcFalse)
		{
			ioServer->updateFlags[i] = ONcCharacterUpdateFlag_None;
		}

		// do any special processing for AIs
		if (character->charType != ONcChar_AI)
		{
			ONiNet_Server_UpdateAI(ioServer, &process_group, character);
		}
	}

	// ------------------------------
	// send the outgoing packet
	// ------------------------------
	// initialize the mass packet
	ONrNet_Server_MassPacket_PrepareForSend(ioServer);

	// go through all the players
	for (i = 0; i < ioServer->game_options.max_players; i++)
	{
		if (ioServer->players[i].inUse == UUcFalse) continue;

		// send the info packet
		ONrNet_Server_PlayerPacket_Send(ioServer, i);

		// send the update packet
		if (ioServer->players[i].send_updates)
		{
			ONrNet_Server_MassPacket_SendToAddress(
				ioServer,
				&ioServer->players[i].player_address);
		}
	}

	// clear the mass packet
	ONrNet_Server_MassPacket_Clear(ioServer);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
char*
ONrNet_Server_GetAddress(
	void)
{
	// make sure the host is initialized
	if (ONgNet_Server == NULL) return NULL;

	return NMrNetContext_GetAddress(ONgNet_Server->server_context);
}

// ----------------------------------------------------------------------
UUtError
ONrNet_Server_Start(
	UUtUns16				inServerPortNumber,
	char					*inHostName,
	ONtNetGameOptions		*inGameOptions,
	UUtUns16				inLevelNumber)
{
	UUtError				error;

	UUmAssert(ONgNet_Common == NULL);
	UUmAssert(ONgNet_Server == NULL);

	// allocate the common data
	ONgNet_Common = UUrMemory_Block_New(sizeof(ONtNet_Common));
	UUmError_ReturnOnNull(ONgNet_Common);

	// allocate the server data
	ONgNet_Server = UUrMemory_Block_New(sizeof(ONtNet_Server));
	UUmError_ReturnOnNull(ONgNet_Server);

	// initialize the common data
	error = ONiNet_Common_Initialize(ONgNet_Common);
	UUmError_ReturnOnError(error);

	// initialize the server
	error = ONiNet_Server_Initialize(ONgNet_Server, inServerPortNumber, inGameOptions);
	UUmError_ReturnOnError(error);

	UUrMemory_MoveFast(inHostName, ONgNet_Server->server_name, ONcMaxHostNameLength);

	// load the level
	error = ONrNet_LevelLoad(ONgNet_Common, inLevelNumber);
	UUmError_ReturnOnError(error);

	// create the AIs
	error = ONiNet_Server_CreateAIs(ONgNet_Server, ONgNet_Common, inGameOptions);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
ONrNet_Server_Stop(
	void)
{
	UUtError				error;

	if (ONgNet_Server == NULL) return UUcError_None;

	// terminate the server data
	error = ONiNet_Server_Terminate(ONgNet_Server);
	UUmError_ReturnOnError(error);

	// delete the server memory
	UUrMemory_Block_Delete(ONgNet_Server);
	ONgNet_Server = NULL;

	// terminate the common data
	ONiNet_Common_Terminate(ONgNet_Common);
	UUrMemory_Block_Delete(ONgNet_Common);
	ONgNet_Common = NULL;

	return error;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
ONrNet_FindServers_Start(
	ONtNet_FindServers_Callback	inFS_Callback,
	UUtUns32					inUserParam)
{
	UUtError					error;

	UUmAssert(ONgNet_FindServer == NULL);

	// allocate the findserver data
	ONgNet_FindServer = UUrMemory_Block_New(sizeof(ONtNet_FindServer));
	UUmError_ReturnOnNull(ONgNet_FindServer);

	// initialize
	UUrMemory_Clear(ONgNet_FindServer, sizeof(ONtNet_FindServer));
	ONgNet_FindServer->callback = inFS_Callback;
	ONgNet_FindServer->user_param = inUserParam;

	// create the context
	error =
		NMrNetContext_New(
			NMcUDP,
			ONcNet_FindServerPort,
			NMcFlag_Broadcast,
			&ONgNet_FindServer->findserver_context);
	UUmError_ReturnOnErrorMsg(error, "Unable to create findserver context.");

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
ONrNet_FindServers_Stop(
	void)
{
	UUtError			error;

	if (ONgNet_FindServer)
	{
		// close the findserver context
		error = NMrNetContext_CloseProtocol(ONgNet_FindServer->findserver_context);
		UUmError_ReturnOnError(error);

		// delete the findserver network context
		NMrNetContext_Delete(&ONgNet_FindServer->findserver_context);
		ONgNet_FindServer->findserver_context = NULL;

		// delete ONgNet_FindServer
		UUrMemory_Block_Delete(ONgNet_FindServer);
		ONgNet_FindServer = NULL;
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
ONrNet_FindServers_Update(
	void)
{
	UUtUns32				game_time;
	ONtNet_ProcessGroup		process_group;

	UUmAssert(ONgNet_FindServer);

	game_time = UUrMachineTime_Sixtieths();

	// ------------------------------
	// broadcast the server request
	// ------------------------------
	if (ONgNet_FindServer->broadcast_time < game_time)
	{
		ONgNet_FindServer->broadcast_time = game_time + ONcNet_FindServer_Broadcast_Interval;

		ONrNet_FindServer_Broadcast(ONgNet_FindServer);
	}

	// ------------------------------
	// update the networking
	// ------------------------------
	NMrNetContext_Update(ONgNet_FindServer->findserver_context);

	// ------------------------------
	// process the incoming packets
	// ------------------------------
	// set up the process group
	process_group.common		= NULL;

	// process all available packets
	while (
		NMrNetContext_ReadData(
			ONgNet_FindServer->findserver_context,
			&process_group.from,
			(UUtUns8*)&process_group.packet,
			&process_group.packet_length))
	{
		// process the packet
		ONrNet_FindServer_ProcessPacket(ONgNet_FindServer, &process_group);
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
ONiNet_Performance_Initialize(
	void)
{
	UUtError			error;
	TStFontFamily		*fontFamily;
	IMtPixelType		pixel_type;

	UUtUns16			width = 256;
	UUtUns16			height = 256;

	// set defaults
	ONgNet_Performance.text_context = NULL;
	ONgNet_Performance.texture_map = NULL;

	ONgNet_Performance.dest[0].x	= 640.0f - (float)width;
	ONgNet_Performance.dest[0].y	= 480.0f - (float)height;
	ONgNet_Performance.dest[0].z	= -1e5f;
	ONgNet_Performance.dest[0].invW	= 10.0f;

	ONgNet_Performance.dest[1].x	= 639.0f;
	ONgNet_Performance.dest[1].y	= 479.0f;
	ONgNet_Performance.dest[1].z	= -1e5f;
	ONgNet_Performance.dest[1].invW	= 10.0f;

	ONgNet_Performance.uv[0].u = 0.0f;
	ONgNet_Performance.uv[0].v = 0.0f;
	ONgNet_Performance.uv[1].u = 1.0f;
	ONgNet_Performance.uv[1].v = 0.0f;
	ONgNet_Performance.uv[2].u = 0.0f;
	ONgNet_Performance.uv[2].v = 1.0f;
	ONgNet_Performance.uv[3].u = 1.0f;
	ONgNet_Performance.uv[3].v = 1.0f;

	ONgNet_Performance.bounds.left		= 0;
	ONgNet_Performance.bounds.top		= 0;
	ONgNet_Performance.bounds.right		= width;
	ONgNet_Performance.bounds.bottom	= height;

	error = M3rDrawEngine_FindGrayscalePixelType(&pixel_type);
	UUmError_ReturnOnError(error);

	// create the text context
	if (ONgNet_Performance.text_context == NULL)
	{
		error =
			TMrInstance_GetDataPtr(
				TScTemplate_FontFamily,
				TScFontFamily_RomanSmall,
				&fontFamily);
		UUmError_ReturnOnError(error);

		error =
			TSrContext_New(
				fontFamily,
				TScStyle_Plain,
				TSc_HLeft,
				&ONgNet_Performance.text_context);
		UUmError_ReturnOnError(error);

		error = TSrContext_SetShade(ONgNet_Performance.text_context, IMcShade_White);
		UUmError_ReturnOnError(error);
	}

	// create the texture_map
	if (ONgNet_Performance.texture_map == NULL)
	{
		error =
			M3rTextureMap_New(
				width,
				height,
				pixel_type,
				M3cTexture_AllocMemory | M3cTexture_UseTempMem,
				M3cTextureFlags_None,
				"networking performance texture",
				&ONgNet_Performance.texture_map);
		UUmError_ReturnOnError(error);
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
ONiNet_Performance_Terminate(
	void)
{
	ONgNet_Performance.display = UUcFalse;

	// delete the text context
	if (ONgNet_Performance.text_context)
	{
		TSrContext_Delete(ONgNet_Performance.text_context);
		ONgNet_Performance.text_context = NULL;
	}
}

// ----------------------------------------------------------------------
static UUtError
ONiNet_Performance_Update(
	ONtNet_Server			*ioServer,
	ONtNet_Client			*ioClient,
	UUtUns32				inGameTime)
{
	UUtError				error;
	UUtUns16				font_height;
	IMtPoint2D				dest;
	char					client[128];
	char					raw[128];
	char					player[128];
	char					mass[128];
	static UUtBool			done_once = UUcFalse;

	// update the stats
	if (ioClient && ((ioClient->performance_time + 60) < inGameTime))
	{
		// adjust the send delta
		if (ioClient->client_adjust_this_frame)
		{
			if (ONgNet_Performance.client_packets_sent > (UUtUns32)(ONgNet_PPS_Target + 2))
			{
				ioClient->client_send_delta++;
			}
			else if (ONgNet_Performance.client_packets_sent < (UUtUns32)(ONgNet_PPS_Target - 2))
			{
				ioClient->client_send_delta--;
				if (ioClient->client_send_delta < 0)
				{
					ioClient->client_send_delta = 0;
				}
			}
		}

		// update the stats
		ONgNet_Performance.client_packets_sent = ioClient->packets_sent;
		ONgNet_Performance.client_bytes_sent = ioClient->bytes_sent;
		ioClient->packets_sent = 0;
		ioClient->bytes_sent = 0;

		ioClient->performance_time = inGameTime;
		ioClient->client_adjust_this_frame = !ioClient->client_adjust_this_frame;
	}

	if (ioServer && ((ioServer->performance_time + 60) < inGameTime))
	{
		// adjust the send delta
		if (ioServer->server_adjust_this_frame)
		{
			if ((ONgNet_Performance.mass_packets_sent / ioServer->num_players) > (UUtUns32)(ONgNet_PPS_Target + 2))
			{
				ioServer->server_send_delta++;
			}
			else if (ONgNet_Performance.mass_packets_sent < (UUtUns32)(ONgNet_PPS_Target - 2))
			{
				ioServer->server_send_delta--;
				if (ioServer->server_send_delta < 0)
				{
					ioServer->server_send_delta = 0;
				}
			}
		}

		// update the stats
		ONgNet_Performance.raw_packets_sent = ioServer->raw_packets_sent;
		ONgNet_Performance.raw_bytes_sent = ioServer->raw_bytes_sent;
		ioServer->raw_packets_sent = 0;
		ioServer->raw_bytes_sent = 0;

		ONgNet_Performance.player_packets_sent = ioServer->player_packets_sent;
		ONgNet_Performance.player_bytes_sent = ioServer->player_bytes_sent;
		ioServer->player_packets_sent = 0;
		ioServer->player_bytes_sent = 0;

		ONgNet_Performance.mass_packets_sent = ioServer->mass_packets_sent;
		ONgNet_Performance.mass_bytes_sent = ioServer->mass_bytes_sent;
		ioServer->mass_packets_sent = 0;
		ioServer->mass_bytes_sent = 0;

		ioServer->performance_time = inGameTime;
		ioServer->server_adjust_this_frame = !ioServer->server_adjust_this_frame;
	}

	// only draw when the player wants to
	if (ONgNet_Performance.display == UUcFalse) return UUcError_None;

	// initialize the stats
	if ((ONgNet_Performance.text_context == NULL) ||
		(ONgNet_Performance.texture_map == NULL))
	{
		error = ONiNet_Performance_Initialize();
		if (error != UUcError_None)
		{
			ONgNet_Performance.display = UUcFalse;
		}
	}

	// draw the stats
	font_height = TSrFont_GetLineHeight(TSrContext_GetFont(ONgNet_Performance.text_context, TScStyle_InUse));
	M3rTextureMap_Fill_Shade(
		ONgNet_Performance.texture_map,
		IMcShade_Black,
		&ONgNet_Performance.bounds);

	dest.x = 4;
	dest.y = font_height;

	if (ioClient)
	{
		// draw the client packet stats
		sprintf(
			client,
			"Client:  PPS %d,  BPS %d",
			ONgNet_Performance.client_packets_sent,
			ONgNet_Performance.client_bytes_sent);

		error =
			TSrContext_DrawString(
				ONgNet_Performance.text_context,
				ONgNet_Performance.texture_map,
				client,
				&ONgNet_Performance.bounds,
				&dest);
		UUmError_ReturnOnError(error);

		dest.y += font_height;

		// draw the client packet stats
		sprintf(client, "Client:  Send Delta %d", ioClient->client_send_delta);

		error =
			TSrContext_DrawString(
				ONgNet_Performance.text_context,
				ONgNet_Performance.texture_map,
				client,
				&ONgNet_Performance.bounds,
				&dest);
		UUmError_ReturnOnError(error);

		if (ioServer)
		{
			dest.y += font_height;
		}
	}

	if (ioServer)
	{
		// draw server raw packet stats
		sprintf(
			raw,
			"Server: Raw PPS %d,  BPS %d",
			ONgNet_Performance.raw_packets_sent,
			ONgNet_Performance.raw_bytes_sent);

		error =
			TSrContext_DrawString(
				ONgNet_Performance.text_context,
				ONgNet_Performance.texture_map,
				raw,
				&ONgNet_Performance.bounds,
				&dest);
		UUmError_ReturnOnError(error);

		dest.y += font_height;

		// draw server player packet stats
		sprintf(
			player,
			"Server: Player PPS %d,  BPS %d",
			ONgNet_Performance.player_packets_sent,
			ONgNet_Performance.player_bytes_sent);

		error =
			TSrContext_DrawString(
				ONgNet_Performance.text_context,
				ONgNet_Performance.texture_map,
				player,
				&ONgNet_Performance.bounds,
				&dest);
		UUmError_ReturnOnError(error);

		dest.y += font_height;

		// draw server mass packet stats
		sprintf(
			mass,
			"Server: Mass PPS %d,  BPS %d",
			ONgNet_Performance.mass_packets_sent,
			ONgNet_Performance.mass_bytes_sent);

		error =
			TSrContext_DrawString(
				ONgNet_Performance.text_context,
				ONgNet_Performance.texture_map,
				mass,
				&ONgNet_Performance.bounds,
				&dest);
		UUmError_ReturnOnError(error);

		dest.y += font_height;

		// draw the client packet stats
		sprintf(client, "Server:  Send Delta %d", ioServer->server_send_delta);

		error =
			TSrContext_DrawString(
				ONgNet_Performance.text_context,
				ONgNet_Performance.texture_map,
				client,
				&ONgNet_Performance.bounds,
				&dest);
		UUmError_ReturnOnError(error);
	}

	// make the texture_map draw nice
	if ((!done_once) &&
		(dest.y < M3cTextureMap_MaxHeight))
	{
		float				dest_y;

		done_once = UUcTrue;

		dest.y += TSrFont_GetDescendingHeight(TSrContext_GetFont(ONgNet_Performance.text_context, TScStyle_InUse));
		dest_y = (float)dest.y;

		ONgNet_Performance.dest[0].y = 480.0f - dest_y;

		ONgNet_Performance.uv[0].u = 0.0f;
		ONgNet_Performance.uv[0].v = 0.0f;
		ONgNet_Performance.uv[1].u = 1.0f;
		ONgNet_Performance.uv[1].v = 0.0f;
		ONgNet_Performance.uv[2].u = 0.0f;
		ONgNet_Performance.uv[2].v = dest_y / ONgNet_Performance.texture_map->height;
		ONgNet_Performance.uv[3].u = 1.0f;
		ONgNet_Performance.uv[3].v = dest_y / ONgNet_Performance.texture_map->height;
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
ONrNet_Performance_Display(
	void)
{
	if (ONgNet_Performance.display == UUcFalse) return;

	M3rDraw_State_Push();

	M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor, IMcShade_White);
	M3rDraw_State_SetInt(M3cDrawStateIntType_Alpha, M3cMaxAlpha);

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_BaseTextureMap,
		ONgNet_Performance.texture_map);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Interpolation,
		M3cDrawState_Interpolation_None);

	M3rDraw_State_Commit();

	M3rDraw_Sprite(
		ONgNet_Performance.dest,
		ONgNet_Performance.uv);

	M3rDraw_State_Pop();
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
ONrNet_Initialize(
	void)
{
	UUtError				error;

	UUmAssert(ONgNet_Common == NULL);
	UUmAssert(ONgNet_Client == NULL);
	UUmAssert(ONgNet_Server == NULL);

	// install exit routine
	UUrAtExit_Register(ONiNet_Terminate_AtExit);

	// initialize the network manager
	error = NMrInitialize();
	UUmError_ReturnOnError(error);

	// initialize the data
	ONgNet_Common			= NULL;
	ONgNet_Client			= NULL;
	ONgNet_Server			= NULL;
	ONgNet_FindServer		= NULL;
	ONgNet_Level_Callback	= NULL;
	ONgNet_SpawnDelta		= 3 * 60;  // 3 seconds in ticks

	// setup the console variables
	error =
		COrVariable_New(
			"net_no_disconnect",
			"0",
			"Do not disconnect when machine is not responding.",
			COrVariable_BoolCallback,
			&ONgNet_No_Disconnect,
			NULL);
	UUmError_ReturnOnErrorMsg(error, "Unable to create a new variable.");

	error =
		COrVariable_New(
			"net_authoritative",
			"0",
			"Server is more authoritative when true.",
			COrVariable_BoolCallback,
			&ONgNet_Authoritative_Server,
			NULL);
	UUmError_ReturnOnErrorMsg(error, "Unable to create a new variable.");

	error =
		COrVariable_New(
			"net_performance",
			"0",
			"display net performance.",
			COrVariable_BoolCallback,
			&ONgNet_Performance.display,
			NULL);
	UUmError_ReturnOnErrorMsg(error, "Unable to create a new variable.");

	error =
		COrVariable_New(
			"net_pps_target",
			"15",
			"packet per second target",
			COrVariable_Uns32Callback,
			&ONgNet_PPS_Target,
			NULL);
	UUmError_ReturnOnErrorMsg(error, "Unable to create a new variable.");

	// set up the console commands
	error =
		SLrScript_Command_Register_Void(
			"net_verbose",
			"set the debug level",
			"[level:string{\"none\" | \"none\" | \"major\" | \"medium\" | \"minor\"} | ]",
			ONiDebugLevel_Callback);
	UUmError_ReturnOnErrorMsg(error, "Unable to create a new command.");

	error =
		SLrScript_Command_Register_Void(
			"net_server_stop",
			"stop the server",
			"",
			ONiNet_Server_Stop_Callback);
	UUmError_ReturnOnErrorMsg(error, "Unable to create a new command.");

	error =
		SLrScript_Command_Register_Void(
			"net_server_start",
			"start the server",
			"[max_players:int | ]",
			ONiNet_Server_Start_Callback);
	UUmError_ReturnOnErrorMsg(error, "Unable to create a new command.");

	error =
		SLrScript_Command_Register_Void(
			"net_client_stop",
			"stop the client",
			"",
			ONiNet_Client_Stop_Callback);
	UUmError_ReturnOnErrorMsg(error, "Unable to create a new command.");

	error =
		SLrScript_Command_Register_Void(
			"net_client_start",
			"start the client",
			"",
			ONiNet_Client_Start_Callback);
	UUmError_ReturnOnErrorMsg(error, "Unable to create a new command.");

	error =
		SLrScript_Command_Register_Void(
			"net_client_join",
			"join the host's game",
			"ip_address:string [class_name:string]",
			ONiNet_Join_Callback);
	UUmError_ReturnOnErrorMsg(error, "Unable to create a new command.");

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtBool
ONrNet_IsActive(
	void)
{
	if (ONgNet_Client || ONgNet_Server) return UUcTrue;

	return UUcFalse;
}

// ----------------------------------------------------------------------
UUtBool
ONrNet_IsClient(
	void)
{
	if (ONgNet_Client) return UUcTrue;

	return UUcFalse;
}

// ----------------------------------------------------------------------
UUtBool
ONrNet_IsServer(
	void)
{
	if (ONgNet_Server) return UUcTrue;

	return UUcFalse;
}

// ----------------------------------------------------------------------
UUtError
ONrNet_Level_Begin(
	UUtUns16				inLevelNumber)
{
	if (ONgNet_Level_Callback && ONgNet_Common)
	{
		ONgNet_Level_Callback(UUcTrue, ONgNet_Common->level_number);
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
ONrNet_Level_Callback_Register(
	ONtNet_Level_Callback	inLevelCallback)
{
	if (inLevelCallback != NULL)
	{
		ONgNet_Level_Callback = inLevelCallback;
	}
}

// ----------------------------------------------------------------------
void
ONrNet_Level_Callback_Unregister(
	void)
{
	ONgNet_Level_Callback = NULL;
}

// ----------------------------------------------------------------------
void
ONrNet_Level_End(
	void)
{
	if (ONgNet_Level_Callback && ONgNet_Common)
	{
		ONgNet_Level_Callback(UUcFalse, ONgNet_Common->level_number);
	}
}

// ----------------------------------------------------------------------
UUtBool
ONrNet_Override(
	void)
{
	if ((ONgNet_Client != NULL) &&
		(ONgNet_Server == NULL))
	{
		return UUcTrue;
	}

	return UUcFalse;
}

// ----------------------------------------------------------------------
void
ONrNet_Terminate(
	void)
{
	// stop performance monitor
	ONiNet_Performance_Terminate();

	// close down
	ONrNet_Client_Stop();
	ONrNet_Server_Stop();

	// terminate the network manager
	NMrTerminate();

	// make sure things got shut down properly
	UUmAssert(ONgNet_Client == NULL);
	UUmAssert(ONgNet_Common == NULL);
	UUmAssert(ONgNet_Server == NULL);
}

// ----------------------------------------------------------------------
void
ONrNet_Update_Receive(
	void)
{
	UUtUns32				game_time;

	if ((ONgNet_Server == NULL) && (ONgNet_Client == NULL)) return;

	// update the game time
	game_time = UUrMachineTime_Sixtieths();
	ONgNet_Common->game_time = game_time;

	// update the client and the server with the data received
	ONiNet_Client_Update_Receive(ONgNet_Client, ONgNet_Common);
	ONiNet_Server_Update_Receive(ONgNet_Server, ONgNet_Common);
}

// ----------------------------------------------------------------------
void
ONrNet_Update_Send(
	void)
{
	if ((ONgNet_Server == NULL) && (ONgNet_Client == NULL)) return;

	// send data
	ONiNet_Client_Update_Send(ONgNet_Client, ONgNet_Common);
	ONiNet_Server_Update_Send(ONgNet_Server, ONgNet_Common);

	// display performance data
	ONiNet_Performance_Update(ONgNet_Server, ONgNet_Client, UUrMachineTime_Sixtieths());
}
