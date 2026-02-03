#pragma once

// ======================================================================
// BFW_NetworkManager.h
// ======================================================================
#ifndef BFW_NETWORKMANAGER_H
#define BFW_NETWORKMANAGER_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

#if (UUmPlatform == UUmPlatform_Mac)
	#include <OpenTransport.h>
	#include <OpenTptInternet.h>
#elif (UUmPlatform == UUmPlatform_Win32)
	#include <WinSock2.h>
#else
	#error unknown platform
#endif

// ======================================================================
// define
// ======================================================================
#define NMcInitialized			1

#define NMcMaxNetPacketSize		1024
#define NMcMaxAddressLength		16

#define NMcRetryInterval		60	/* ticks */

#define NMcMaxResends			3

// ----------------------------------------------------------------------
#define NMrNetContext_Broadcast(netContext, port, buffer, buffer_length) \
	((netContext)->methods.broadcast(netContext, port, buffer, buffer_length))

#define NMrNetContext_CloseProtocol(netContext) \
	(netContext)->methods.close_protocol(netContext)

#define NMrNetContext_CompareAddresses(netContext, address1, address2) \
	((netContext)->methods.compare_addresses(netContext, address1, address2))

#define NMrNetContext_GetAddress(netContext) \
	(netContext)->methods.get_address(netContext)

#define NMrNetContext_ReadData(netContext, from_address, buffer, buffer_length) \
	((netContext)->methods.read_data(netContext, from_address, buffer, buffer_length))

#define NMrNetContext_StartProtocol(netContext, flags) \
	(netContext)->methods.start_protocol(netContext, flags)

#define NMrNetContext_StringToAddress(netContext, string, port, netAddress) \
	((netContext)->methods.string_to_address(netContext, string, port, netAddress))

#define NMrNetContext_Update(netContext) \
	((netContext)->methods.update(netContext))

#define NMrNetContext_WriteData(netContext, dest_address, buffer, buffer_length, flag) \
	((netContext)->methods.write_data(netContext, dest_address, buffer, buffer_length, flag))

// ======================================================================
// enums
// ======================================================================
// ----------------------------------------------------------------------
typedef enum NMtServiceType
{
	NMcUDP,
	NMcTCP,
	NMcNetSprocket,
	NMcDirectPlay,

	NMcNumServiceTypes

} NMtServiceType;

// ----------------------------------------------------------------------
typedef enum NMtFlags
{
	NMcFlag_None,
	NMcFlag_Broadcast

} NMtFlags;

// ----------------------------------------------------------------------
enum
{
	NMcPacketFlags_None				= 0x0000,
	NMcPacketFlags_Compressed		= 0x0001
};

// ======================================================================
// typedefs
// ======================================================================
typedef struct NMtNetContext		NMtNetContext;
typedef struct NMtNetContextPrivate	NMtNetContextPrivate;
typedef UUtUns8						NMtNetAddress[NMcMaxAddressLength];

// ======================================================================
typedef UUtError
(*NMtNetContextMethod_Broadcast)(
	NMtNetContext					*inNetContext,
	UUtUns16						inPortNumber,
	UUtUns8							*inDataBuffer,
	UUtUns16						inNumBytes);

typedef UUtError
(*NMtNetContextMethod_CloseProtocol)(
	NMtNetContext					*inNetContext);

typedef UUtBool
(*NMtNetContextMethod_CompareAddresses)(
	NMtNetContext					*inNetContext,
	NMtNetAddress					*inAddress1,
	NMtNetAddress					*inAddress2);

typedef char*
(*NMtNetContextMethod_GetAddress)(
	NMtNetContext					*inNetContext);

typedef UUtBool
(*NMtNetContextMethod_ReadData)(
	NMtNetContext					*inNetContext,
	NMtNetAddress					*outFromAddress,
	UUtUns8							*outDataBuffer,
	UUtUns16						*outNumBytes);

typedef UUtError
(*NMtNetContextMethod_StartProtocol)(
	NMtNetContext					*inNetContext,
	NMtFlags						inFlags);

typedef UUtError
(*NMtNetContextMethod_StringToAddress)(
	NMtNetContext					*inNetContext,
	char							*inString,
	UUtUns16						inPortNumber,
	NMtNetAddress					*outNetAddress);

typedef UUtError
(*NMtNetContextMethod_Update)(
	NMtNetContext					*inNetContext);

typedef UUtError
(*NMtNetContextMethod_WriteData)(
	NMtNetContext					*inNetContext,
	NMtNetAddress					*inDestAddress,
	UUtUns8							*inDataBuffer,
	UUtUns16						inNumBytes,
	UUtUns16						inFlags);

// ======================================================================
typedef struct NMtNetServiceCaps
{
	NMtServiceType					type;

} NMtNetServiceCaps;

// ----------------------------------------------------------------------
typedef struct NMtNetContextMethods
{
	NMtNetContextMethod_Broadcast			broadcast;
	NMtNetContextMethod_CloseProtocol		close_protocol;
	NMtNetContextMethod_CompareAddresses	compare_addresses;
	NMtNetContextMethod_GetAddress			get_address;
	NMtNetContextMethod_ReadData			read_data;
	NMtNetContextMethod_StartProtocol		start_protocol;
	NMtNetContextMethod_StringToAddress		string_to_address;
	NMtNetContextMethod_Update				update;
	NMtNetContextMethod_WriteData			write_data;

} NMtNetContextMethods;

// ----------------------------------------------------------------------
struct NMtNetContext
{
	UUtUns32						flags;
	UUtUns32						game_time;
	NMtServiceType					type;
	UUtUns16						port_number;
	NMtNetContextPrivate			*private_data;
	NMtNetContextMethods			methods;

	UUtUns16						num_incoming_buffers;
	UUtUns16						num_outgoing_buffers;
	UUtBool							out_of_incoming_buffers;
	UUtBool							out_of_outgoing_buffers;

};

// ----------------------------------------------------------------------
typedef struct NMtNetServices
{
	NMtNetServiceCaps				service;
	NMtNetContextMethods			methods;

} NMtNetServices;

// ----------------------------------------------------------------------
typedef struct NMtPacketHeader
{
	UUtUns32						packet_flags;
	UUtUns32						packet_data_size;

} NMtPacketHeader;

#define NMcPacketDataSize			(NMcMaxNetPacketSize - sizeof(NMtPacketHeader))

typedef struct NMtPacket
{
	NMtPacketHeader					packet_header;
	UUtUns8							packet_data[NMcPacketDataSize];

} NMtPacket;

#define NMcPacketSize				sizeof(NMtPacket)

typedef struct NMtUDPData
{
	NMtNetAddress					address;
	UUtUns32						address_length;
	UUtUns16						buffer_length;
	UUtUns8							buffer[NMcPacketSize];

} NMtUDPData;

// ======================================================================
// prototypes
// ======================================================================
char*
NMrAddressToString(
	const NMtNetAddress		*inNetAddress);

UUtError
NMrInitialize(
	void);

void
NMrTerminate(
	void);

UUtError
NMrRegisterService(
	NMtNetServiceCaps		*inNetServiceCaps,
	NMtNetContextMethods	*inNetServiceMethods);

// ----------------------------------------------------------------------
UUtError
NMrNetContext_New(
	NMtServiceType			inServiceType,
	UUtUns16				inPortNumber,
	NMtFlags				inFlags,
	NMtNetContext			**outNetContext);

UUtError
NMrNetContext_Delete(
	NMtNetContext			**inNetContext);

// ======================================================================
#endif /* BFW_NETWORKMANAGER_H */
