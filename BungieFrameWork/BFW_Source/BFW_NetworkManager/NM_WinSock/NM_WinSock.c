// ======================================================================
// NM_WS_UDP.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW_NetworkManager.h"
#include "NM_WinSock.h"
#include "NM_WinSock_Private.h"
#include <Winsock2.h>

#include "BFW_Console.h"

// ======================================================================
// defines
// ======================================================================
#define NMcDesiredWinSockVersion	MAKEWORD(2,0)

#define	NMcWS_UDP_NumIncomingBuffers		5
#define NMcWS_UDP_NumOutgoingBuffers		5
#define NMcWS_UDP_QueueIncrement			5

// ======================================================================
// globals
// ======================================================================
static UUtUns8				NMgWS_UDP_String[32];

// ======================================================================
// functions
// ======================================================================
static UUtError
NMiWS_UDP_WriteData(
	NMtNetContext			*inNetContext,
	NMtNetAddress			*inDestAddress,
	UUtUns8					*inDataBuffer,
	UUtUns16				inNumBytes,
	UUtUns16				inFlags);

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
NMiWS_UDP_HandleRead(
	NMtNetContext			*inNetContext)
{
	UUtUns32				return_val;
	WSANETWORKEVENTS		events;

	// poll for read events
	return_val =
		WSAWaitForMultipleEvents(
			1,
			&inNetContext->private_data->event,
			UUcFalse,
			0,
			UUcFalse);
	if (return_val == SOCKET_ERROR)
	{
		return;
	}

	// handle the events
	return_val =
		WSAEnumNetworkEvents(
			inNetContext->private_data->socket,
			inNetContext->private_data->event,
			&events);
	if (return_val == SOCKET_ERROR)
	{
		return;
	}

	if (events.lNetworkEvents & FD_READ)
	{
		NMtDataBuffer		*data_buffer;

		// continually read until recvfrom returns no data
		while (1)
		{
			DWORD error_val;

			data_buffer =
				NMrQueue_DequeueBuffer(
					&inNetContext->private_data->queues,
					NMcIncomingEmptyQueue);
			if (data_buffer == NULL)
			{
				inNetContext->out_of_incoming_buffers = UUcTrue;
				break;
			}

			data_buffer->data.address_length = NMcMaxAddressLength;

			// read the data
			return_val =
				recvfrom(
					inNetContext->private_data->socket,
					(char*)&data_buffer->data.buffer,
					NMcPacketSize,
					0,
					(SOCKADDR*)&data_buffer->data.address,
					(int*)&data_buffer->data.address_length);
			if ((return_val == SOCKET_ERROR) || (return_val == 0))
			{
				error_val = GetLastError();

				// put the data_buffer into the incoming empty queue
				NMrQueue_EnqueueBuffer(
					&inNetContext->private_data->queues,
					NMcIncomingEmptyQueue,
					data_buffer);

				break;
			}
			else
			{
				NMtPacket			*data_packet;

				// get a pointer to the data_packet
				data_packet = (NMtPacket*)&data_buffer->data.buffer;

				// swap the data
				UUrSwap_4Byte(&data_packet->packet_header.packet_flags);
				UUrSwap_4Byte(&data_packet->packet_header.packet_data_size);

				// put the data into the incoming empty queue
				NMrQueue_EnqueueBuffer(
					&inNetContext->private_data->queues,
					NMcIncomingQueue,
					data_buffer);
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
static UUtError
NMiWS_UDP_CloseSocket(
	NMtNetContext			*inNetContext)
{
	// delete the event
	if (inNetContext->private_data->event != WSA_INVALID_EVENT)
	{
		WSACloseEvent(inNetContext->private_data->event);
		inNetContext->private_data->event = WSA_INVALID_EVENT;
	}

	// close the socket
	if (inNetContext->private_data->socket != INVALID_SOCKET)
	{
		closesocket(inNetContext->private_data->socket);
		inNetContext->private_data->socket = INVALID_SOCKET;
	}

	// delete

	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
NMiWS_UDP_Broadcast(
	NMtNetContext			*inNetContext,
	UUtUns16				inPortNumber,
	UUtUns8					*inDataBuffer,
	UUtUns16				inNumBytes)
{
	UUtError				error;
	NMtNetAddress			raw;
	SOCKADDR_IN				*address;

	// get a pointer to the address
	address = (SOCKADDR_IN*)raw;

	// init address
	address->sin_family			= AF_INET;
	address->sin_port			= htons(inPortNumber);
	address->sin_addr.s_addr	= INADDR_BROADCAST;

	// send the data to every machine on the local network
	error =
		NMiWS_UDP_WriteData(
			inNetContext,
			&raw,
			inDataBuffer,
			inNumBytes,
			NMcPacketFlags_None);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
NMiWS_UDP_CloseProtocol(
	NMtNetContext			*inNetContext)
{
	UUtError				error;

	// close the socket
	error = NMiWS_UDP_CloseSocket(inNetContext);
	UUmError_ReturnOnError(error);

	// terminate the queues
	NMrQueue_Terminate(&inNetContext->private_data->queues);

	// free the private_data
	if (inNetContext->private_data)
	{
		UUrMemory_Block_Delete(inNetContext->private_data);
	}

	// shutdown winsock
	WSACleanup();

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtBool
NMiWS_UDP_CompareAddresses(
	NMtNetContext			*inNetContext,
	NMtNetAddress			*inAddress1,
	NMtNetAddress			*inAddress2)
{
	SOCKADDR_IN				*addr_1;
	SOCKADDR_IN				*addr_2;

	// get a pointer to the two addresses
	addr_1 = (SOCKADDR_IN*)inAddress1;
	addr_2 = (SOCKADDR_IN*)inAddress2;

	if (addr_1->sin_addr.s_addr == addr_2->sin_addr.s_addr)
	{
		return UUcTrue;
	}

	return UUcFalse;
}

// ----------------------------------------------------------------------
static UUtUns8*
NMiWS_UDP_GetAddress(
	NMtNetContext			*inNetContext)
{
	char					host_name[1024];
	UUtUns32				return_val;
	HOSTENT					*host_ent;
	IN_ADDR					address;

	return_val = gethostname(host_name, sizeof(host_name));
	if (return_val == SOCKET_ERROR) return NULL;

	host_ent = gethostbyname(host_name);
	if (host_ent == NULL) return NULL;

	if (host_ent->h_addr_list[0] != NULL)
	{
		UUrMemory_MoveFast(host_ent->h_addr_list[0], &address, sizeof(IN_ADDR));
		return (UUtUns8*)inet_ntoa(address);
	}

	return NULL;
}

// ----------------------------------------------------------------------
static UUtBool
NMiWS_UDP_ReadData(
	NMtNetContext			*inNetContext,
	NMtNetAddress			*outFromAddress,
	UUtUns8					*outDataBuffer,
	UUtUns16				*outNumBytes)
{
	NMtDataBuffer			*data_buffer;
	NMtPacket				*data_packet;

	// get a buffer from the incoming queue
	data_buffer =
		NMrQueue_DequeueBuffer(
			&inNetContext->private_data->queues,
			NMcIncomingQueue);
	if (data_buffer == NULL)
	{
		return UUcFalse;
	}

	// get a pointer to the data_packet
	data_packet = (NMtPacket*)&data_buffer->data.buffer;

	// set the outgoing data
	UUrMemory_MoveFast(
		&data_buffer->data.address,
		outFromAddress,
		sizeof(NMtNetAddress));
	*outNumBytes = (UUtUns16)data_packet->packet_header.packet_data_size;
	UUrMemory_MoveFast(&data_packet->packet_data, outDataBuffer, *outNumBytes);

	// put the buffer onto the incoming empty queue
	NMrQueue_EnqueueBuffer(
		&inNetContext->private_data->queues,
		NMcIncomingEmptyQueue,
		data_buffer);

	return UUcTrue;
}

// ----------------------------------------------------------------------
static UUtError
NMiWS_UDP_StartProtocol(
	NMtNetContext			*inNetContext,
	NMtFlags				inFlags)
{
	UUtError				error;
	UUtUns32				return_val;
	WSADATA					wsaData;
	SOCKADDR_IN				server_address;

	// initialize winsock.dll
	return_val = WSAStartup(NMcDesiredWinSockVersion, &wsaData);
	if (return_val)
	{
		WSACleanup();
		return UUcError_Generic;
	}

	// check winsock version
	if (wsaData.wVersion != NMcDesiredWinSockVersion)
	{
		WSACleanup();
		return UUcError_Generic;
	}

	// allocate memory for the private data
	inNetContext->private_data =
		(NMtNetContextPrivate*)UUrMemory_Block_New(
			sizeof(NMtNetContextPrivate));
	if (inNetContext->private_data == NULL)
	{
		WSACleanup();
		return UUcError_OutOfMemory;
	}

	// clear the private_data
	UUrMemory_Clear(
		inNetContext->private_data,
		sizeof(NMtNetContextPrivate));

	// set the socket and event to invalid values
	inNetContext->private_data->socket = INVALID_SOCKET;
	inNetContext->private_data->event = WSA_INVALID_EVENT;

	// create a socket
	inNetContext->private_data->socket =
		socket(
			AF_INET,
			SOCK_DGRAM,
			IPPROTO_UDP);
	if (inNetContext->private_data->socket == INVALID_SOCKET)
	{
		NMiWS_UDP_CloseProtocol(inNetContext);
		return UUcError_Generic;
	}

	if (inFlags & NMcFlag_Broadcast)
	{
		BOOL					option_val;

		// set the SO_BROADCAST option for the socket
		option_val = UUcTrue;
		return_val =
			setsockopt(
				inNetContext->private_data->socket,
				SOL_SOCKET,
				SO_BROADCAST,
				(char*)&option_val,
				sizeof(BOOL));
		if (return_val == SOCKET_ERROR)
		{
			NMiWS_UDP_CloseProtocol(inNetContext);
			return UUcError_Generic;
		}
	}

	// initialize the queues
	error =
		NMrQueue_Initialize(
			&inNetContext->private_data->queues,
			NMcWS_UDP_NumIncomingBuffers,
			NMcWS_UDP_NumOutgoingBuffers);
	UUmError_ReturnOnError(error);

	// set the number of buffers
	inNetContext->num_incoming_buffers = NMcWS_UDP_NumIncomingBuffers;
	inNetContext->num_outgoing_buffers = NMcWS_UDP_NumOutgoingBuffers;

	// create an event object
	inNetContext->private_data->event = WSACreateEvent();
	if (inNetContext->private_data->event == WSA_INVALID_EVENT)
	{
		NMiWS_UDP_CloseProtocol(inNetContext);
		return UUcError_Generic;
	}

	// make the socket nonblocking and tell it which events
	// to respond to
	return_val =
		WSAEventSelect(
			inNetContext->private_data->socket,
			inNetContext->private_data->event,
			FD_READ);
	if (return_val == SOCKET_ERROR)
	{
		NMiWS_UDP_CloseProtocol(inNetContext);
		return UUcError_Generic;
	}

	// set the address
	server_address.sin_family		= AF_INET;
	server_address.sin_addr.s_addr	= INADDR_ANY;
	server_address.sin_port			=
		htons((UUtUns16)inNetContext->port_number);

	// bind the name to the socket
	return_val =
		bind(
			inNetContext->private_data->socket,
			(LPSOCKADDR)&server_address,
			sizeof(SOCKADDR_IN));
	if (return_val == SOCKET_ERROR)
	{
		NMiWS_UDP_CloseProtocol(inNetContext);
		return UUcError_Generic;
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
NMiWS_UDP_StringToAddress(
	NMtNetContext			*inNetContext,
	char					*inString,
	UUtUns16				inPortNumber,
	NMtNetAddress			*outNetAddress)
{
	SOCKADDR_IN				temp_address;

	// fill in the temp_address
	temp_address.sin_family			= AF_INET;
	temp_address.sin_port			= htons(inPortNumber);
	temp_address.sin_addr.s_addr	= inet_addr((const char*)inString);

	// make sure the address is legitimate
	if (temp_address.sin_addr.s_addr == INADDR_NONE)
	{
		return UUcError_Generic;
	}

	// copy the temp address into the outNetAddress
	UUrMemory_MoveFast(
		&temp_address,
		outNetAddress,
		sizeof(NMtNetAddress));

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
NMiWS_UDP_Update(
	NMtNetContext			*inNetContext)
{
	UUtError				error;

	if (inNetContext->out_of_incoming_buffers)
	{
		error =
			NMrQueue_AddBuffers(
				&inNetContext->private_data->queues,
				NMcIncomingEmptyQueue,
				NMcWS_UDP_QueueIncrement);
		if (error == UUcError_None)
		{
			inNetContext->num_incoming_buffers += NMcWS_UDP_QueueIncrement;
			COrConsole_Printf(
				"Increased IncomingBuffers to %d",
				inNetContext->num_incoming_buffers);
		}
		else
		{
			COrConsole_Printf("Out of IncomingBuffers");
		}
		inNetContext->out_of_incoming_buffers = UUcFalse;
	}

	if (inNetContext->out_of_outgoing_buffers)
	{
		error =
			NMrQueue_AddBuffers(
				&inNetContext->private_data->queues,
				NMcOutgoingEmptyQueue,
				NMcWS_UDP_QueueIncrement);
		if (error == UUcError_None)
		{
			inNetContext->num_outgoing_buffers += NMcWS_UDP_QueueIncrement;
			COrConsole_Printf(
				"Increased OutgoingBuffers to %d",
				inNetContext->num_outgoing_buffers);
		}
		else
		{
			COrConsole_Printf("Out of OutgoingBuffers");
		}
		inNetContext->out_of_outgoing_buffers = UUcFalse;
	}

	// read incoming packets and put them into the incoming queue
	NMiWS_UDP_HandleRead(inNetContext);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
NMiWS_UDP_WriteData(
	NMtNetContext			*inNetContext,
	NMtNetAddress			*inDestAddress,
	UUtUns8					*inDataBuffer,
	UUtUns16				inNumBytes,
	UUtUns16				inFlags)
{
	UUtUns32				return_val;
	NMtPacket				raw;
	NMtPacket				*data_packet;
	UUtUns16				packet_length;

	// calculate the packet length
	packet_length = sizeof(NMtPacketHeader) + inNumBytes;

	// get a pointer to the data_packet
	data_packet = &raw;

	// initialize the packet
	data_packet->packet_header.packet_flags			= inFlags;
	data_packet->packet_header.packet_data_size		= inNumBytes;

	// swap the data
	UUrSwap_4Byte(&data_packet->packet_header.packet_flags);
	UUrSwap_4Byte(&data_packet->packet_header.packet_data_size);

	// copy the data into the packet
	UUrMemory_MoveFast(
		inDataBuffer,
		data_packet->packet_data,
		inNumBytes);

	// send the data
	return_val =
		sendto(
			inNetContext->private_data->socket,
			(const char*)data_packet,
			(int)packet_length,
			0,
			(SOCKADDR*)inDestAddress,
			sizeof(SOCKADDR_IN));
	if (return_val == SOCKET_ERROR)
	{
		return UUcError_Generic;
	}

	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
char*
NMrWS_AddressToString(
	const NMtNetAddress			*inNetAddress)
{
	SOCKADDR_IN					*address;

	address = (SOCKADDR_IN*)inNetAddress;

	return (char*)inet_ntoa(address->sin_addr);
}

// ----------------------------------------------------------------------
UUtError
NMrWS_UDP_Initialize(
	void)
{
	UUtError					error;
	UUtUns32					return_val;
	NMtNetServiceCaps			caps;
	NMtNetContextMethods		methods;
	WSADATA						wsaData;

	// initialize winsock.dll
	return_val = WSAStartup(NMcDesiredWinSockVersion, &wsaData);
	if (return_val)
	{
		WSACleanup();
		return UUcError_Generic;
	}

	// check winsock version
	if (wsaData.wVersion < NMcDesiredWinSockVersion)
	{
		WSACleanup();
		return UUcError_Generic;
	}

	// close Winsock
	WSACleanup();

	// clear the methods struct
	UUrMemory_Clear(&methods, sizeof(NMtNetContextMethods));

	// set the caps
	caps.type					= NMcUDP;

	// set the function pointers in the methods struct
	methods.broadcast			= NMiWS_UDP_Broadcast;
	methods.close_protocol		= NMiWS_UDP_CloseProtocol;
	methods.compare_addresses	= NMiWS_UDP_CompareAddresses;
	methods.get_address			= NMiWS_UDP_GetAddress;
	methods.read_data			= NMiWS_UDP_ReadData;
	methods.start_protocol		= NMiWS_UDP_StartProtocol;
	methods.string_to_address	= NMiWS_UDP_StringToAddress;
	methods.update				= NMiWS_UDP_Update;
	methods.write_data			= NMiWS_UDP_WriteData;

	// register the methods with the net manager
	error = NMrRegisterService(&caps, &methods);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
NMrWS_UDP_Terminate(
	void)
{
}

