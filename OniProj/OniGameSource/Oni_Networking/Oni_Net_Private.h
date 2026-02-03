// ======================================================================
// Oni_Networking.h
// ======================================================================
#ifndef ONI_NET_PRIVATE_H
#define ONI_NET_PRIVATE_H


// ======================================================================
// include
// ======================================================================
#include "BFW.h"
#include "BFW_NetworkManager.h"
#include "BFW_Object.h"
#include "Oni_Character.h"
#include "Oni_AI_Setup.h"

// ======================================================================
// defines
// ======================================================================
#define ONcMaxInstNameLength					32

#define ONcNet_UnknownPlayer					0xFFFF

#define ONcNet_ServerIndex						0

#define ONcNet_UpdateTime						15	/* ticks */
#define ONcNet_HeartbeatTime					720	/* ticks */

#define ONcNet_MaxClientRequests				16

#define ONcNet_MaxDataTypesPerPacket			128

// ======================================================================
// enums
// ======================================================================
typedef enum ONtDebugLevel
{
	ONcDebugLevel_None,
	ONcDebugLevel_Major,
	ONcDebugLevel_Medium,
	ONcDebugLevel_Minor,

	ONcNumDebugLevels

} ONtDebugLevel;

typedef enum ONtNet_JoinStage
{
	ONcNet_JoinStage_None,
	ONcNet_JoinStage_GameInfo,
	ONcNet_JoinStage_CreateCharacter

} ONtNet_JoinStage;

// ----------------------------------------------------------------------
enum
{
	ONcNet_Update_None,				// this one is never used, but it starts the count
	ONcNet_Update_isAiming,
	ONcNet_Update_validTarget,
	ONcNet_Update_fightFramesRemaining,
	ONcNet_Update_fire,
	ONcNet_Update_throw,
	ONcNet_Update_inAir,
	ONcNet_Update_facing,
	ONcNet_Update_desiredFacing,
	ONcNet_Update_aimingLR,
	ONcNet_Update_aimingUD,
	ONcNet_Update_buttonIsDown,
	ONcNet_Update_aimingTarget,
	ONcNet_Update_aimingVector,
	ONcNet_Update_animName,

	ONcNet_NumUpdates

};

// ======================================================================
// typedefs
// ======================================================================
typedef UUtUns8				ONtNet_DataType;		// 1

// ----------------------------------------------------------------------
typedef struct ONtNet_PacketHeader
{
	UUtUns32			packet_number;
	UUtUns16			packet_swap;
	UUtUns16			packet_version;
	UUtUns16			player_index;
	UUtUns16			packet_length;

} ONtNet_PacketHeader;

#define ONcNet_PacketDataSize	(NMcPacketDataSize - sizeof(ONtNet_PacketHeader))

typedef struct ONtNet_Packet
{
	ONtNet_PacketHeader		packet_header;
	UUtUns8					packet_data[ONcNet_PacketDataSize];

} ONtNet_Packet;

// ======================================================================
typedef struct ONtNet_DataTypeInfo
{
	UUtUns16			num_datatypes;
	UUtUns16			datatypes_offset;

} ONtNet_DataTypeInfo;

typedef struct ONtNet_PacketData
{
	ONtNet_Packet		packet;
	UUtUns8				*data_ptr;
	ONtNet_DataTypeInfo	*datatype_info;
	ONtNet_DataType		datatypes[ONcNet_MaxDataTypesPerPacket];

} ONtNet_PacketData;

// ======================================================================

typedef struct ONtNet_UC_Data
{
	UUtBool				isAiming;								// 1
	UUtBool				validTarget;							// 1
	UUtBool				fire;									// 1

	UUtUns16			fightFramesRemaining;					// 2
	UUtUns16			throwing;								// 2
	UUtUns16			throwFrame;								// 2
	UUtUns16			inAirFlags;								// 2
	UUtUns16			numFramesInAir;							// 2

	float				facing;									// 4
	float				desiredFacing;							// 4
	float				aimingLR;								// 4
	float				aimingUD;								// 4

	LItButtonBits		buttonIsDown;							// 8

	M3tPoint3D			aimingTarget;							// 12
	M3tVector3D			aimingVector;							// 12
	M3tVector3D			inAirVelocity;							// 12

	char				anim_name[ONcMaxInstNameLength];		// 32
	char				throw_name[ONcMaxInstNameLength];		// 32

	// make sure there is room for the datatypes
	char				datatypes[ONcNet_NumUpdates];

} ONtNet_UC_Data;

// ======================================================================
typedef enum ONtNet_RequestType
{
	ONcNet_Request_None,
	ONcNet_Request_Character_Create,
	ONcNet_Request_Info_Server,
	ONcNet_Request_Join,
	ONcNet_Request_Quit

} ONtNet_RequestType;

typedef struct ONtNet_Request
{
	ONtNet_RequestType		request_type;
	UUtUns8					request_counter;
	UUtUns32				request_id;
	UUtUns32				request_time;

} ONtNet_Request;

// ======================================================================

typedef struct ONtNet_Players
{
	UUtBool				inUse;
	UUtBool				send_updates;
	NMtNetAddress		player_address;
	ONtCharacter		*character;
	UUtUns32			previous_packet_number;
	UUtUns32			previous_packet_time;
	ONtNet_PacketData	player_packet_data;

	UUtBool				updateACKNeeded[ONcMaxCharacters];
	UUtBool				weaponACKNeeded[WPcMaxWeapons];

} ONtNet_Players;

// ----------------------------------------------------------------------
typedef struct ONtNet_Common
{
	ONtCharacter			*character_list;
	UUtUns32				game_time;
	UUtUns16				level_number;

} ONtNet_Common;

typedef struct ONtNet_Client
{
	NMtNetContext			*client_context;

	ONtCharacter			*character;
	UUtUns16				player_index;

	NMtNetAddress			server_address;
	ONtNet_JoinStage		join_stage;

	UUtUns16				request_id;
	ONtNet_Request			requests[ONcNet_MaxClientRequests];

	UUtUns32				packet_number;

	ONtNet_PacketData		packet_data;

	UUtUns32				cupdate_bits;

	UUtUns32				previous_packet_time;
	UUtUns32				previous_packet_number;
	ONtNet_UC_Data			prev_update;

	UUtUns32				heartbeat_time;

	UUtUns16				teamNumber;
	char					characterClassName[AIcMaxClassNameLen];
	char					playerName[ONcMaxPlayerNameLength];

	UUtUns32				client_send_time;
	UUtInt32				client_send_delta;
	UUtBool					client_adjust_this_frame;

	UUtUns32				packets_sent;
	UUtUns32				bytes_sent;
	UUtUns32				performance_time;

} ONtNet_Client;

typedef struct ONtNet_Server
{
	NMtNetContext			*server_context;
	NMtNetContext			*findserver_context;
	ONtNet_Players			*players;
	ONtNetGameOptions		game_options;
	UUtUns16				num_players;

	UUtUns32				packet_number;

	ONtNet_PacketData		mass_packet_data;

	ONtNet_UC_Data			prev_update[ONcMaxCharacters];
	UUtUns32				updateFlags[ONcMaxCharacters];
	UUtUns16				weaponFlags[WPcMaxWeapons];

	char					server_name[ONcMaxHostNameLength];

	UUtUns32				server_send_time;
	UUtInt32				server_send_delta;
	UUtBool					server_adjust_this_frame;

	UUtUns32				raw_packets_sent;
	UUtUns32				raw_bytes_sent;

	UUtUns32				player_packets_sent;
	UUtUns32				player_bytes_sent;

	UUtUns32				mass_packets_sent;
	UUtUns32				mass_bytes_sent;

	UUtUns32				performance_time;

} ONtNet_Server;

typedef struct ONtNet_FindServer
{
	NMtNetContext			*findserver_context;
	UUtUns32				packet_number;
	UUtUns32				broadcast_time;
	ONtNet_FindServers_Callback	callback;
	UUtUns32				user_param;

} ONtNet_FindServer;

// ----------------------------------------------------------------------
typedef struct ONtNet_ProcessGroup
{
	ONtNet_Common			*common;		// common information
	NMtNetAddress			from;			// address packet is from
	ONtNet_Packet			packet;			// original packet received
	UUtUns16				packet_length;	// length of packet received
	UUtUns8					*data;			// current point of processing in packet_data

	ONtNet_DataType			*datatypes;		// pointer to the datatypes
	ONtNet_DataTypeInfo		*datatype_info; // pointer to the datatype info

	UUtBool					swap_it;		// this packet needs to be byte-swapped

} ONtNet_ProcessGroup;

// ======================================================================
// globals
// ======================================================================
extern UUtUns32				ONgNet_SpawnDelta;
extern UUtBool				ONgNet_Authoritative_Server;

// ======================================================================
// prototypes
// ======================================================================
UUtError
ONrNet_LevelLoad(
	ONtNet_Common			*ioCommon,
	UUtUns16				inLevelNumber);

// ----------------------------------------------------------------------
void
ONrNet_Client_JoinStage_Advance(
	ONtNet_Client			*ioClient,
	ONtNet_Common			*ioCommon);

void
ONrNet_Client_Rejected(
	ONtNet_Client			*ioClient);

// ----------------------------------------------------------------------
void
ONrNet_Client_Request_Add(
	ONtNet_Client			*ioClient,
	ONtNet_RequestType		inRequestType,
	UUtUns32				inGameTime);

void
ONrNet_Client_Request_ResponseReceived(
	ONtNet_Client			*ioClient,
	UUtUns32				inRequestID);

// ======================================================================

#endif /* ONI_NET_PRIVATE_H */
