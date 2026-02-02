// ======================================================================
// NM_Queues.h
// ======================================================================
#ifndef NM_QUEUES_H
#define NM_QUEUES_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_NetworkManager.h"

#if (UUmPlatform == UUmPlatform_Mac)
	#include <OpenTransport.h>
	#include <OpenTptInternet.h>
#elif (UUmPlatform == UUmPlatform_Win32)
	#include <WinSock.h>
#else
	#error unknown platform
#endif

// ======================================================================
// enums
// ======================================================================
typedef enum NMtQueueRef
{
	NMcIncomingQueue			= 1,
	NMcIncomingEmptyQueue		= 2,
	NMcOutgoingQueue			= 3,
	NMcOutgoingEmptyQueue		= 4

} NMtQueueRef;

// ======================================================================
// typedef
// ======================================================================
typedef struct NMtDataBuffer
{
	struct NMtDataBuffer	*next;
	struct NMtDataBuffer	*prev;
	NMtUDPData				data;

} NMtDataBuffer;

typedef struct NMtQueue
{
	NMtDataBuffer			*head;
	NMtDataBuffer			*tail;

} NMtQueue;

typedef struct NMtQueues
{
	NMtQueue				incoming_queue;
	NMtQueue				incoming_empty_queue;

	NMtQueue				outgoing_queue;
	NMtQueue				outgoing_empty_queue;

} NMtQueues;

// ======================================================================
// prototypes
// ======================================================================
UUtError
NMrQueue_Initialize(
	NMtQueues				*inQueues,
	UUtUns16				inNumIncomingBuffers,
	UUtUns16				inNumOutgoingBuffers);

void
NMrQueue_Terminate(
	NMtQueues				*inQueues);

// ----------------------------------------------------------------------
UUtError
NMrQueue_AddBuffers(
	NMtQueues				*inQueues,
	NMtQueueRef				inQueueRef,
	UUtUns16				inNumBuffersToAdd);

NMtDataBuffer*
NMrQueue_DequeueBuffer(
	NMtQueues				*inQueues,
	NMtQueueRef				inQueueRef);

void
NMrQueue_EnqueueBuffer(
	NMtQueues				*inQueues,
	NMtQueueRef				inQueueRef,
	NMtDataBuffer			*inDataBuffer);

NMtDataBuffer*
NMrQueue_GetNextBuffer(
	NMtQueues				*inQueues,
	NMtQueueRef				inQueueRef,
	NMtDataBuffer			*inDataBuffer);

void
NMrQueue_RemoveBuffer(
	NMtQueues				*inQueues,
	NMtQueueRef				inQueueRef,
	NMtDataBuffer			*inDataBuffer);

UUtUns16
NMrQueue_GetBufferCount(
	NMtQueues				*inQueues,
	NMtQueueRef				inQueueRef);

// ======================================================================
#endif /* NM_QUEUES_H */
