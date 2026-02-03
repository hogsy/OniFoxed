// ======================================================================
// Oni_Net_Support.h
// ======================================================================
#ifndef ONI_NET_SUPPORT_H
#define ONI_NET_SUPPORT_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "Oni_Net_Private.h"

// ======================================================================
// prototypes
// ======================================================================
void
ONrNet_Packet_Init(
	ONtNet_PacketData		*ioPacketData);

// ----------------------------------------------------------------------
void
ONrNet_Client_Packet_Send(
	ONtNet_Client			*ioClient);

// ----------------------------------------------------------------------
void
ONrNet_CS_Heartbeat(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup);

void
ONrNet_CS_Request_CharacterCreate(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup,
	UUtUns32				inRequestID);

void
ONrNet_CS_Request_Join(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup,
	UUtUns32				inRequestID);

void
ONrNet_CS_Request_Spawn(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup);

void
ONrNet_CS_Request_Quit(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup,
	UUtUns32				inRequestID);

void
ONrNet_CS_Update_Character(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup);

void
ONrNet_CS_Update_Critical(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup);

void
ONrNet_Client_ProcessPacket(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup);

// ----------------------------------------------------------------------
void
ONrNet_Server_PlayerPacket_Send(
	ONtNet_Server			*ioServer,
	UUtUns16				inPlayerIndex);

void
ONrNet_Server_MassPacket_Clear(
	ONtNet_Server			*ioServer);

void
ONrNet_Server_MassPacket_PrepareForSend(
	ONtNet_Server			*ioServer);

void
ONrNet_Server_MassPacket_SendToAddress(
	ONtNet_Server			*ioServer,
	NMtNetAddress			*inToAddress);

// ----------------------------------------------------------------------
void
ONrNet_SS_Update_Character(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup,
	ONtCharacter			*inCharacter);

void
ONrNet_SS_Update_Critical(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup,
	ONtCharacter			*inCharacter,
	UUtUns32				*outUpdateBits);

void
ONrNet_SS_Update_Critical_Force(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup,
	ONtCharacter			*inCharacter,
	UUtUns32				inUpdateFlags,
	UUtUns16				inPlayerIndex);

void
ONrNet_SS_Update_Weapons(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup);

void
ONrNet_Server_ProcessPacket(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup);

void
ONrNet_Server_FS_ProcessPacket(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup);

// ----------------------------------------------------------------------
void
ONrNet_FindServer_Broadcast(
	ONtNet_FindServer		*ioFindServer);

void
ONrNet_FindServer_ProcessPacket(
	ONtNet_FindServer		*ioFindServer,
	ONtNet_ProcessGroup		*ioProcessGroup);

// ======================================================================
#endif /* ONI_NET_SUPPORT_H */
