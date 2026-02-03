// ======================================================================
// BFW_NetworkManager.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_NetworkManager.h"
#if (UUmPlatform == UUmPlatform_Mac)
	#include "NM_OpenTransport.h"
#elif (UUmPlatform == UUmPlatform_Win32)
	#include "NM_WinSock.h"
#else
	#error unknown platform
#endif

// ======================================================================
// globals
// ======================================================================
static NMtNetServices			NMgNetServicesList[NMcNumServiceTypes];

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
char*
NMrAddressToString(
	const NMtNetAddress		*inNetAddress)
{
	char					*address_string;

#if (UUmPlatform == UUmPlatform_Mac)
	address_string = NMrOT_AddressToString(inNetAddress);
#elif (UUmPlatform == UUmPlatform_Win32)
	address_string = NMrWS_AddressToString(inNetAddress);
#endif

	return address_string;
}

// ----------------------------------------------------------------------
UUtError
NMrRegisterService(
	NMtNetServiceCaps		*inNetServiceCaps,
	NMtNetContextMethods	*inNetServiceMethods)
{
	UUmAssert(inNetServiceCaps);
	UUmAssert(inNetServiceMethods);
	UUmAssert((inNetServiceCaps->type >= NMcUDP) &&
		(inNetServiceCaps->type < NMcNumServiceTypes));

	// add the service
	NMgNetServicesList[inNetServiceCaps->type].service = *inNetServiceCaps;
	NMgNetServicesList[inNetServiceCaps->type].methods = *inNetServiceMethods;

	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
NMrNetContext_Delete(
	NMtNetContext			**inNetContext)
{
	UUmAssert(*inNetContext);

	// release the memory used by inNetContext
	UUrMemory_Block_Delete(*inNetContext);
	*inNetContext = NULL;

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
NMrNetContext_New(
	NMtServiceType			inServiceType,
	UUtUns16				inPortNumber,
	NMtFlags				inFlags,
	NMtNetContext			**outNetContext)
{
	UUtError				error;
	NMtNetContext			*net_context;

	UUmAssert((inServiceType >= NMcUDP) && (inServiceType < NMcNumServiceTypes));

	// make sure the requested service is registered
	if (NMgNetServicesList[inServiceType].methods.start_protocol == NULL)
	{
		UUmError_ReturnOnErrorMsg(
			UUcError_Generic,
			"That network service is unavailable.");
	}

	// allocate memory for a new net context
	net_context = (NMtNetContext*)UUrMemory_Block_NewClear(sizeof(NMtNetContext));
	UUmError_ReturnOnNull(net_context);

	// clear the flags
	net_context->flags = 0;

	// set the methods of the network context
	net_context->methods = NMgNetServicesList[inServiceType].methods;

	// set the port number
	net_context->port_number = inPortNumber;

	// start the protocol
	error = NMrNetContext_StartProtocol(net_context, inFlags);
	UUmError_ReturnOnError(error);

	// set the initialized flag
	net_context->flags |= NMcInitialized;

	// set the return value
	*outNetContext = net_context;

	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
NMrInitialize(
	void)
{
	UUtError				error;

	// initialize the net services list
	UUrMemory_Clear(NMgNetServicesList, sizeof(NMgNetServicesList));

	// initialize the plugins
#if (UUmPlatform == UUmPlatform_Mac)
	error = NMrOT_UDP_Initialize();
#elif (UUmPlatform == UUmPlatform_Win32)
	error = NMrWS_UDP_Initialize();
#endif

	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
NMrTerminate(
	void)
{
	// terminate the plugins
#if (UUmPlatform == UUmPlatform_Mac)
	NMrOT_UDP_Terminate();
#elif (UUmPlatform == UUmPlatform_Win32)
	NMrWS_UDP_Terminate();
#endif
}

