// ======================================================================
// NM_OT_UDP_Private.h
// ======================================================================
#ifndef NM_OT_UDP_PRIVATE_H
#define NM_OT_UDP_PRIVATE_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "NM_Queues.h"
#include <OpenTransport.h>
#include <OpenTptInternet.h>

// ======================================================================
// typedef
// ======================================================================
struct NMtNetContextPrivate
{
	EndpointRef				endpoint;
	InetSvcRef				internet_services;
	InetHost				host;
	NMtQueues				queues;

};

// ======================================================================
#endif /* NM_OT_UDP_PRIVATE_H */
