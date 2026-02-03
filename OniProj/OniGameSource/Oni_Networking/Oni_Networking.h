// ======================================================================
// Oni_Networking.h
// ======================================================================
#ifndef ONI_NETWORKING_H
#define ONI_NETWORKING_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_NetworkManager.h"

#include "Oni_Platform.h"

// ======================================================================
// errors
// ======================================================================
#define UUcError_Client_AlreadyStarted			((UUtError) 0x00F1)

// ======================================================================
// defines
// ======================================================================
#define ONcNet_VersionNumber			0x0106

#define ONcNet_DefaultServerPort		12222
#define ONcNet_DefaultClientPort		12223
#define ONcNet_FindServerPort			12224

#define ONcMaxTeamNameLength			32
#define ONcMaxHostNameLength			32
#define ONcMaxMapNameLength				32

// ======================================================================
// enums
// ======================================================================
typedef enum ONtNetType
{
	ONcNetType_None				= 0x0000,		// none
	ONcNetType_Server			= 0x0001,		// server only
	ONcNetType_Client			= 0x0002,		// client only
	ONcNetType_ServerClient		= 0x0003		// server and client

} ONtNetType;

typedef enum ONtNetGameParams
{
	ONcNetGameParam_None		= 0x0000,
	ONcNetGameParam_TimeLimit	= 0x0001,
	ONcNetGameParam_KillLimit	= 0x0002,
	ONcNetGameParam_HasAIs		= 0x0004

} ONtNetGameParams;

// ======================================================================
// typedefs
// ======================================================================
typedef struct ONtNetGameOptions
{
	UUtUns16				max_players;

	ONtNetGameParams		game_parameters;
	UUtUns32				time_limit;
	UUtUns16				kill_limit;
	UUtUns16				num_AIs;

} ONtNetGameOptions;

typedef struct ONtNet_ServerInfo
{
	UUtUns16				max_players;
	UUtUns16				num_players;
	UUtUns16				level_number;
	char					server_name[ONcMaxHostNameLength];

} ONtNet_ServerInfo;

typedef
void
(*ONtNet_Level_Callback)(
	UUtBool					inIsLoading,
	UUtUns16				inLevelNumber);

// ======================================================================
// prototypes
// ======================================================================
UUtError
ONrNet_Client_Join(
	UUtUns16				inServerPortNumber,
	char					*inServerAddressString,
	char					*inCharacterClass,
	char					*inPlayerName,
	char					*inTeamName);

UUtError
ONrNet_Client_Start(
	UUtUns16				inClientPortNumber);

UUtError
ONrNet_Client_Stop(
	void);

// ======================================================================
char*
ONrNet_Server_GetAddress(
	void);

UUtError
ONrNet_Server_Start(
	UUtUns16				inServerPortNumber,
	char					*inHostName,
	ONtNetGameOptions		*inGameOptions,
	UUtUns16				inLevelNumber);

UUtError
ONrNet_Server_Stop(
	void);

// ======================================================================
typedef
void
(*ONtNet_FindServers_Callback)(
	ONtNet_ServerInfo		*inServerInfo,
	NMtNetAddress			*inServerAddress,
	UUtUns32				inUserParam);

UUtError
ONrNet_FindServers_Start(
	ONtNet_FindServers_Callback	inFS_Callback,
	UUtUns32					inUserParam);

UUtError
ONrNet_FindServers_Stop(
	void);

void
ONrNet_FindServers_Update(
	void);

// ======================================================================
void
ONrNet_Performance_Display(
	void);

// ======================================================================
UUtError
ONrNet_Initialize(
	void);

UUtBool
ONrNet_IsActive(
	void);

UUtBool
ONrNet_IsClient(
	void);

UUtBool
ONrNet_IsServer(
	void);

UUtError
ONrNet_Level_Begin(
	UUtUns16				inLevelNumber);

void
ONrNet_Level_Callback_Register(
	ONtNet_Level_Callback	inLevelCallback);

void
ONrNet_Level_Callback_Unregister(
	void);

void
ONrNet_Level_End(
	void);

UUtBool
ONrNet_Override(
	void);

void
ONrNet_Terminate(
	void);

void
ONrNet_Update_Receive(
	void);

void
ONrNet_Update_Send(
	void);

// ======================================================================
#endif /* ONI_NETWORKING_H */
