// ======================================================================
// NM_OT_UDP.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW_NetworkManager.h"
#include "NM_OpenTransport.h"
#include "NM_OpenTransport_Private.h"
#include "NM_Queues.h"

#include <OpenTransport.h>
#include <OpenTptInternet.h>
#include <Gestalt.h>

#include <string.h>

#include "BFW_Console.h"

// ======================================================================
// defines
// ======================================================================
#define NMcOT_UDP_NumIncomingBuffers		5
#define NMcOT_UDP_NumOutgoingBuffers		5
#define NMcOT_UDP_QueueIncrement			5

// ======================================================================
// globals
// ======================================================================
static UUtBool				NMgOT_UDP_HasOT = UUcFalse;
static UUtBool				NMgOT_UDP_HasOT_tested = UUcFalse;
static UUtInt32				NMgOT_UDP_OTGestaltResult = UUcFalse;

OTConfiguration				*NMgOT_UDP_Config = NULL;

static char					NMgOT_UDP_String[32];

// ======================================================================
// prototype
// ======================================================================
static UUtError
NMiOT_UDP_WriteData(
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
NMiOT_UDP_HandleRead(
	NMtNetContext			*inNetContext)
{
	OTResult				status;
	UUtUns32				flags;
	UUtBool					first_read;
	NMtDataBuffer			*data_buffer;
	TUnitData				unit_data;

	// set first_read to true because there could be an erroneous T_DATA event
	// which would cause OTRcvUData() to return kOTNoDataErr the first time it
	// is called.  The next call to OTRcvUData() will have data so to keep the
	// function from exiting, use this boolean to handle this case.
	first_read = UUcTrue;

	// continuously read until no queues are available or a kOTNoDataErr is encountered
	while (1)
	{
		// get a buffer from the empty queue
		data_buffer =
			NMrQueue_DequeueBuffer(
				&inNetContext->private_data->queues,
				NMcIncomingEmptyQueue);
		if (data_buffer == NULL)
		{
			inNetContext->out_of_incoming_buffers = UUcTrue;
			break;
		}

		// set up the unit_data
		unit_data.addr.buf			= (UUtUns8*)&data_buffer->data.address;
		unit_data.addr.maxlen		= sizeof(InetAddress);
		unit_data.opt.maxlen		= 0;
		unit_data.udata.buf			= data_buffer->data.buffer;
		unit_data.udata.maxlen		= NMcPacketSize;

		// receive the data
		status =
			OTRcvUData(
				inNetContext->private_data->endpoint,
				&unit_data,
				&flags);
		if (status != kOTNoError)
		{
			// put the buffer into the empty queue
			NMrQueue_EnqueueBuffer(
				&inNetContext->private_data->queues,
				NMcIncomingEmptyQueue,
				data_buffer);

			// process the error
			if ((status == kOTNoDataErr) && (first_read == UUcTrue))
			{
				first_read = UUcFalse;
				continue;
			}
			else if (status == kOTLookErr)
			{
				status = OTLook(inNetContext->private_data->endpoint);
			}

			break;
		}

		// save the buffer length
		data_buffer->data.buffer_length = unit_data.udata.len;

		// put the data into the incoming queue
		NMrQueue_EnqueueBuffer(
			&inNetContext->private_data->queues,
			NMcIncomingQueue,
			data_buffer);
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
NMiOT_UDP_OTAvailable(
	void)
{
	// only test if this hasn't been determined yet
	if (NMgOT_UDP_HasOT_tested == UUcFalse)
	{
		OTResult			status;

		// Check whether Open Transport is installed.
		status = Gestalt(gestaltOpenTpt, &NMgOT_UDP_OTGestaltResult);
		if (status == noErr)
		{
			NMgOT_UDP_HasOT = UUcTrue;
		}

		NMgOT_UDP_HasOT_tested = UUcTrue;
	}

	return NMgOT_UDP_HasOT;
}

// ----------------------------------------------------------------------
static pascal void
NMiOT_UDP_NotifyProc(
	void					*inContextPtr,
	OTEventCode				inCode,
	OTResult				inResult,
	void					*inCookie)
{
	NMtNetContext			*net_context;
	OTResult				status;

	// get a pointer to the net_context
	net_context = (NMtNetContext*)inContextPtr;

	switch (inCode)
	{
		case T_OPENCOMPLETE:
			// the endpoint has been opened, record the endpointref in the net_context
			net_context->private_data->endpoint = (EndpointRef)inCookie;
		break;

		case T_UDERR:
			// clear the error
			status = OTRcvUDErr(net_context->private_data->endpoint, NULL);
		break;

		case T_DATA:
			NMiOT_UDP_HandleRead(net_context);
		break;

		default:
		break;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
NMiOT_UDP_Broadcast(
	NMtNetContext			*inNetContext,
	UUtUns16				inPortNumber,
	UUtUns8					*inDataBuffer,
	UUtUns16				inNumBytes)
{
	UUtError				error;
	NMtNetAddress			address;
	InetAddress				*address_ptr;

	// get a pointer to the address
	address_ptr = (InetAddress*)address;

	// init address
	OTInitInetAddress(address_ptr, inPortNumber, 0xFFFFFFFF);

	// send the data to every machine on the local network
	error =
		NMiOT_UDP_WriteData(
			inNetContext,
			&address,
			inDataBuffer,
			inNumBytes,
			NMcPacketFlags_None);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
NMiOT_UDP_CloseProtocol(
	NMtNetContext			*inNetContext)
{
	OTResult				status;

	if ((inNetContext == NULL) || (inNetContext->private_data == NULL))
	{
		return UUcError_Generic;
	}

	// unbind the connection to the endpoint
	if (inNetContext->private_data->endpoint != kOTInvalidEndpointRef)
	{
		status = OTGetEndpointState(inNetContext->private_data->endpoint);
		if (status == T_IDLE)
		{
			// check the endpoint to make sure it is TRUELY idle
			do
			{
				status = OTLook(inNetContext->private_data->endpoint);
				if (status == kOTNoError)
				{
					// unbind the endpoint
					status = OTUnbind(inNetContext->private_data->endpoint);
					if (status != kOTNoError)
					{
						UUmAssert(status == kOTNoError);
						return UUcError_Generic;
					}

					// close the endpoint
					status = OTCloseProvider(inNetContext->private_data->endpoint);
					if (status != kOTNoError)
					{
						UUmAssert(status == kOTNoError);
						return UUcError_Generic;
					}
				}
				else
				{
					if (status == T_DATA)
					{
						NMtDataBuffer	*data_buffer;

						// there are reads pending that need to be cleared, so
						// handle the read
						NMiOT_UDP_HandleRead(inNetContext);

						// since we don't care about the data coming in, just put
						// the data buffers in the incoming queue back into the
						// incoming empty queue
						data_buffer =
							NMrQueue_DequeueBuffer(
								&inNetContext->private_data->queues,
								NMcIncomingQueue);
						while(data_buffer)
						{
							NMrQueue_EnqueueBuffer(
								&inNetContext->private_data->queues,
								NMcIncomingEmptyQueue,
								data_buffer);

							data_buffer =
								NMrQueue_DequeueBuffer(
									&inNetContext->private_data->queues,
									NMcIncomingQueue);
						}
					}
					else
					{
						UUmAssert(!"don't know how to handle this");
					}
				}
			}
			while (status != kOTNoError);
		}

		// the endpoint is no longer valid
		inNetContext->private_data->endpoint = kOTInvalidEndpointRef;
	}

	// terminate the queues
	NMrQueue_Terminate(&inNetContext->private_data->queues);

	// free the private_data
	if (inNetContext->private_data)
	{
		UUrMemory_Block_Delete(inNetContext->private_data);
		inNetContext->private_data = NULL;
	}

	// close OT
	CloseOpenTransport();

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtBool
NMiOT_UDP_CompareAddresses(
	NMtNetContext			*inNetContext,
	NMtNetAddress			*inAddress1,
	NMtNetAddress			*inAddress2)
{
	InetAddress				*addr_1;
	InetAddress				*addr_2;

	// get a pointer to the two addresses
	addr_1 = (InetAddress*)inAddress1;
	addr_2 = (InetAddress*)inAddress2;

	if (addr_1->fHost == addr_2->fHost)
	{
		return UUcTrue;
	}

	return UUcFalse;
}

// ----------------------------------------------------------------------
static char*
NMiOT_UDP_GetAddress(
	NMtNetContext			*inNetContext)
{
	// clear the string
	NMgOT_UDP_String[0] = '\0';

	// convert the address to a string
	OTInetHostToString(
		inNetContext->private_data->host,
		(char*)NMgOT_UDP_String);

	return NMgOT_UDP_String;
}

// ----------------------------------------------------------------------
static UUtBool
NMiOT_UDP_ReadData(
	NMtNetContext			*inNetContext,
	NMtNetAddress			*outFromAddress,
	UUtUns8					*outDataBuffer,
	UUtUns16				*outNumBytes)
{
	OTResult				result;
	NMtDataBuffer			*data_buffer;
	TUDErr					rcv_error;
	InetAddress				from_address;
	NMtPacket				*data_packet;

	// look for async events
	result = OTLook(inNetContext->private_data->endpoint);
	switch (result)
	{
		case T_UDERR:
			// clear the error
			rcv_error.addr.buf		= (UInt8*)&from_address;
			rcv_error.addr.maxlen	= sizeof(InetAddress);
			rcv_error.opt.len		= 0;
			result = OTRcvUDErr(inNetContext->private_data->endpoint, &rcv_error);
		break;

		case T_DATA:
			NMiOT_UDP_HandleRead(inNetContext);
		break;
	}

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
	*outNumBytes = data_packet->packet_header.packet_data_size;
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
NMiOT_UDP_StartProtocol(
	NMtNetContext			*inNetContext,
	NMtFlags				inFlags)
{
	UUtError				error;
	OTResult				status;
	TBind					requested_addr;
	TBind					returned_addr;
	InetAddress				in_address;
	InetAddress				out_address;
	InetInterfaceInfo		info;
	OTConfiguration			*config;

	// make sure open transport is available
	if (NMiOT_UDP_OTAvailable() == UUcFalse)
	{
		return UUcError_Generic;
	}

	// initialize OT
	status = InitOpenTransport();
	if (status != kOTNoError)
	{
		return UUcError_Generic;
	}

	// allocate memory for the private data
	inNetContext->private_data =
		(NMtNetContextPrivate*)UUrMemory_Block_New(
			sizeof(NMtNetContextPrivate));
	if (inNetContext->private_data == NULL)
	{
		return UUcError_OutOfMemory;
	}

	// clear the private_data
	UUrMemory_Clear(
		inNetContext->private_data,
		sizeof(NMtNetContextPrivate));

	// initialize the queues
	error =
		NMrQueue_Initialize(
			&inNetContext->private_data->queues,
			NMcOT_UDP_NumIncomingBuffers,
			NMcOT_UDP_NumOutgoingBuffers);
	UUmError_ReturnOnError(error);

	// get an OT configuration
	if (NMgOT_UDP_Config)
	{
		config = NMgOT_UDP_Config;
		NMgOT_UDP_Config = OTCloneConfiguration(config);
	}
	else
	{
		config = OTCreateConfiguration(kUDPName);
		NMgOT_UDP_Config = OTCloneConfiguration(config);
	}

	// create the udp endpoint that is asynchronous
	status =
		OTAsyncOpenEndpoint(
			config,
			0,
			NULL,
			NMiOT_UDP_NotifyProc,
			inNetContext);
	if (status != kOTNoError)
	{
		// free the private_data
		UUrMemory_Block_Delete(inNetContext->private_data);

		// close OT
		CloseOpenTransport();

		return UUcError_Generic;
	}

	// init in_address
	OTInitInetAddress(&in_address, inNetContext->port_number, 0);
	status = OTInetGetInterfaceInfo(&info, kDefaultInetInterface);
	if (status != kOTNoError)
	{
		return UUcError_Generic;
	}

	// under OT to receive broadcast packets, the address to bind to must
	// be kOTAnyInetAddress
	if (inFlags & NMcFlag_Broadcast)
	{
		in_address.fHost = kOTAnyInetAddress;
	}
	else
	{
		in_address.fHost = info.fAddress;
	}

	// save the host address
	inNetContext->private_data->host = info.fAddress;

	// set up the requested_addr
	requested_addr.addr.buf		= (UInt8*)&in_address;
	requested_addr.addr.len		= sizeof(InetAddress);
	requested_addr.qlen			= 0;

	// set up the returned_addr
	returned_addr.addr.buf		= (UInt8*)&out_address;
	returned_addr.addr.maxlen	= sizeof(InetAddress);
	requested_addr.qlen			= 0;

	// bind the endpoint
	status =
		OTBind(
			inNetContext->private_data->endpoint,
			&requested_addr,
			&returned_addr);
	if (status != kOTNoError)
	{
		NMiOT_UDP_CloseProtocol(inNetContext);
		return UUcError_Generic;
	}

	// get a reference to the internet services
	inNetContext->private_data->internet_services =
		OTOpenInternetServices(
			kDefaultInternetServicesPath,
			0,
			&status);
	if (status != kOTNoError)
	{
		NMiOT_UDP_CloseProtocol(inNetContext);
		return UUcError_Generic;
	}

	status = OTGetEndpointState(inNetContext->private_data->endpoint);
	if (status != T_IDLE)
	{
		UUmAssert(status == T_IDLE);
		return UUcError_Generic;
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
NMiOT_UDP_StringToAddress(
	NMtNetContext			*inNetContext,
	char					*inString,
	UUtUns16				inPortNumber,
	NMtNetAddress			*outNetAddress)
{
	InetHostInfo			host_info;
	OTResult				status;

	if (inString)
	{
		// turn the string into an address
		status =
			OTInetStringToAddress(
				inNetContext->private_data->internet_services,
				inString,
				&host_info);
		if (status != kOTNoError)
		{
			return UUcError_Generic;
		}

		// initialize the inet address
		OTInitInetAddress((InetAddress*)outNetAddress, inPortNumber, host_info.addrs[0]);
	}
	else
	{
		InetInterfaceInfo		info;

		// initialize the inet address
		OTInitInetAddress((InetAddress*)outNetAddress, inPortNumber, 0);
		status = OTInetGetInterfaceInfo(&info, kDefaultInetInterface);
		if (status != kOTNoError)
		{
			return UUcError_Generic;
		}
		((InetAddress*)outNetAddress)->fHost = info.fAddress;
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
NMiOT_UDP_Update(
	NMtNetContext			*inNetContext)
{
	UUtError				error;

	// if there were no buffers for NMiOT_UDP_HandleRead(), then a T_DATA is
	// still pending and needs to be handled.  Call NMiOT_UDP_HandleRead()
	// hopefully there will be some buffers available.
	if (inNetContext->out_of_incoming_buffers)
	{
		error =
			NMrQueue_AddBuffers(
				&inNetContext->private_data->queues,
				NMcIncomingEmptyQueue,
				NMcOT_UDP_QueueIncrement);
		if (error == UUcError_None)
		{
			inNetContext->num_incoming_buffers += NMcOT_UDP_QueueIncrement;
			COrConsole_Printf(
				"Increased IncomingBuffers to %d",
				inNetContext->num_incoming_buffers);
		}
		else
		{
			COrConsole_Printf("Out of IncomingBuffers");
		}
		inNetContext->out_of_incoming_buffers = UUcFalse;
		NMiOT_UDP_HandleRead(inNetContext);
	}

	if (inNetContext->out_of_outgoing_buffers)
	{
		error =
			NMrQueue_AddBuffers(
				&inNetContext->private_data->queues,
				NMcOutgoingEmptyQueue,
				NMcOT_UDP_QueueIncrement);
		if (error == UUcError_None)
		{
			inNetContext->num_outgoing_buffers += NMcOT_UDP_QueueIncrement;
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

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
NMiOT_UDP_WriteData(
	NMtNetContext			*inNetContext,
	NMtNetAddress			*inDestAddress,
	UUtUns8					*inDataBuffer,
	UUtUns16				inNumBytes,
	UUtUns16				inFlags)
{
	OTResult				status;
	TUnitData				unit_data;
	NMtPacket				raw;
	NMtPacket				*data_packet;
	UUtUns16				packet_length;

	// calculate the packet_length
	packet_length = sizeof(NMtPacketHeader) + inNumBytes;

	// get a pointer to the data_packet
	data_packet = &raw;

	// initialize the packet
	data_packet->packet_header.packet_flags			= inFlags;
	data_packet->packet_header.packet_data_size		= inNumBytes;

	// copy the data into the packet
	UUrMemory_MoveFast(
		inDataBuffer,
		data_packet->packet_data,
		inNumBytes);

	// set the unit_data fields
	unit_data.addr.buf		= (UUtUns8*)inDestAddress;
	unit_data.addr.len		= sizeof(InetAddress);
	unit_data.opt.len		= 0;
	unit_data.udata.buf		= (UUtUns8*)data_packet;
	unit_data.udata.len		= packet_length;

	// send the data immediately
	status = OTSndUData(inNetContext->private_data->endpoint, &unit_data);
	if (status != kOTNoError)
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
NMrOT_AddressToString(
	const NMtNetAddress			*inAddress)
{
	InetAddress					*inet_address;

	inet_address = (InetAddress*)inAddress;

	OTInetHostToString(inet_address->fHost, NMgOT_UDP_String);

	return NMgOT_UDP_String;
}

// ----------------------------------------------------------------------
UUtError
NMrOT_UDP_Initialize(
	void)
{
	UUtError					error;
	NMtNetServiceCaps			caps;
	NMtNetContextMethods		methods;

	// clear the methods struct
	UUrMemory_Clear(&methods, sizeof(NMtNetContextMethods));

	// set the caps
	caps.type					= NMcUDP;

	// set the function pointers in the methods struct
	methods.broadcast			= NMiOT_UDP_Broadcast;
	methods.close_protocol		= NMiOT_UDP_CloseProtocol;
	methods.compare_addresses	= NMiOT_UDP_CompareAddresses;
	methods.get_address			= NMiOT_UDP_GetAddress;
	methods.read_data			= NMiOT_UDP_ReadData;
	methods.start_protocol		= NMiOT_UDP_StartProtocol;
	methods.string_to_address	= NMiOT_UDP_StringToAddress;
	methods.update				= NMiOT_UDP_Update;
	methods.write_data			= NMiOT_UDP_WriteData;

	// register the methods with the net manager
	error = NMrRegisterService(&caps, &methods);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
NMrOT_UDP_Terminate(
	void)
{
}
