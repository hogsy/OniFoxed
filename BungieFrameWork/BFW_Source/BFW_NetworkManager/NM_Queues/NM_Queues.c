// ======================================================================
// NM_Queues.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "NM_Queues.h"

// ======================================================================
// prototypes
// ======================================================================
NMtQueue*
NMiGetQueue(
	NMtQueues				*inQueues,
	NMtQueueRef				inQueueRef);

// ======================================================================
// functions
// ======================================================================

// ----------------------------------------------------------------------
NMtQueue*
NMiGetQueue(
	NMtQueues				*inQueues,
	NMtQueueRef				inQueueRef)
{
	switch (inQueueRef)
	{
		case NMcIncomingQueue:			return &inQueues->incoming_queue;
		case NMcIncomingEmptyQueue:		return &inQueues->incoming_empty_queue;
		case NMcOutgoingQueue:			return &inQueues->outgoing_queue;
		case NMcOutgoingEmptyQueue:		return &inQueues->outgoing_empty_queue;
		default:						return NULL;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
NMrQueue_Initialize(
	NMtQueues				*inQueues,
	UUtUns16				inNumIncomingBuffers,
	UUtUns16				inNumOutgoingBuffers)
{
	UUtUns32				i, j;

	// initialize the queues
	inQueues->incoming_queue.head = NULL;
	inQueues->incoming_queue.tail = NULL;

	inQueues->incoming_empty_queue.head = NULL;
	inQueues->incoming_empty_queue.tail = NULL;

	inQueues->outgoing_queue.head = NULL;
	inQueues->outgoing_queue.tail = NULL;

	inQueues->outgoing_empty_queue.head = NULL;
	inQueues->outgoing_empty_queue.tail = NULL;

	// allocate buffers and put them in the incoming empty queue
	for (i = 0; i < inNumIncomingBuffers; i++)
	{
		NMtDataBuffer		*temp;

		// allocate the buffer
		temp = (NMtDataBuffer*)UUrMemory_Block_New(sizeof(NMtDataBuffer));
		if (temp == NULL)
		{
			// delete all previously allocated buffers
			for (j = 0; j < i; j++)
			{
				// remove the buffer from the incoming empty queue
				temp = NMrQueue_DequeueBuffer(inQueues, NMcIncomingEmptyQueue);
				if (temp == NULL)
				{
					// no more buffers
					break;
				}

				// delete the buffer
				UUrMemory_Block_Delete(temp);
			}

			return UUcError_Generic;
		}

		// put the buffer into the incoming empty queue
		NMrQueue_EnqueueBuffer(inQueues, NMcIncomingEmptyQueue, temp);
	}

	// allocate buffers and put them in the outgoing empty queue
	for (i = 0; i < inNumOutgoingBuffers; i++)
	{
		NMtDataBuffer		*temp;

		// allocate the buffer
		temp = (NMtDataBuffer*)UUrMemory_Block_New(sizeof(NMtDataBuffer));
		if (temp == NULL)
		{
			// delete all previously allocated buffers
			for (j = 0; j < i; j++)
			{
				// remove the buffer from the outgoing empty queue
				temp = NMrQueue_DequeueBuffer(inQueues, NMcOutgoingEmptyQueue);
				if (temp == NULL)
				{
					// no more buffers
					break;
				}

				// delete the buffer
				UUrMemory_Block_Delete(temp);
			}

			return UUcError_Generic;
		}

		// put the buffer into the outgoing empty queue
		NMrQueue_EnqueueBuffer(inQueues, NMcOutgoingEmptyQueue, temp);
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
NMrQueue_Terminate(
	NMtQueues				*inQueues)
{
	NMtDataBuffer			*temp;
	NMtQueueRef				i;

	// dequeue all of the buffers in all of the queue
	for (i = NMcIncomingQueue; i <= NMcOutgoingEmptyQueue; i++)
	{
		// dequeue the first buffer in the queue
		temp = NMrQueue_DequeueBuffer(inQueues, i);
		while (temp)
		{
			// delete the buffer
			UUrMemory_Block_Delete(temp);

			// get the next buffer
			temp = NMrQueue_DequeueBuffer(inQueues, i);
		}
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
NMrQueue_AddBuffers(
	NMtQueues				*inQueues,
	NMtQueueRef				inQueueRef,
	UUtUns16				inNumBuffersToAdd)
{
	NMtQueue				*queue;
	UUtUns16				i;

	UUmAssert(inQueues);
	UUmAssert((inQueueRef == NMcIncomingEmptyQueue) ||
				(inQueueRef == NMcOutgoingEmptyQueue));

	// get a pointer to the queue being referenced
	queue = NMiGetQueue(inQueues, inQueueRef);

	// allocate buffers and add them to the desired queue
	for (i = 0; i < inNumBuffersToAdd; i++)
	{
		NMtDataBuffer		*temp;

		// allocate the buffer
		temp = (NMtDataBuffer*)UUrMemory_Block_New(sizeof(NMtDataBuffer));
		if (temp == NULL) return UUcError_OutOfMemory;

		// put the buffer into the incoming empty queue
		NMrQueue_EnqueueBuffer(inQueues, inQueueRef, temp);
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
NMtDataBuffer*
NMrQueue_DequeueBuffer(
	NMtQueues				*inQueues,
	NMtQueueRef				inQueueRef)
{
	NMtQueue				*queue;
	NMtDataBuffer			*temp;

	// get a pointer to the queue being referenced
	queue = NMiGetQueue(inQueues, inQueueRef);

	// remove the first element in the queue
	temp = queue->head;
	if (queue->head != NULL)
	{
		// set queue head to the next element in the queue
		queue->head = queue->head->next;

		// if there are no more elements in the queue then
		// set the tail to NULL
		if (queue->head == NULL)
		{
			queue->tail = NULL;
		}
		else
		{
			// set the new heads prev to NULL
			queue->head->prev = NULL;
		}

		// set temp's pointers to NULL
		temp->next = NULL;
		temp->prev = NULL;
	}

	return temp;
}

// ----------------------------------------------------------------------
void
NMrQueue_EnqueueBuffer(
	NMtQueues				*inQueues,
	NMtQueueRef				inQueueRef,
	NMtDataBuffer			*inDataBuffer)
{
	NMtQueue				*queue;

	// get a pointer to the queue being referenced
	queue = NMiGetQueue(inQueues, inQueueRef);

	// set the pointers of the data buffer
	inDataBuffer->next = NULL;
	inDataBuffer->prev = NULL;

	// set the queue head
	if (queue->head == NULL)
	{
		// no buffers are in the queue yet
		queue->head = inDataBuffer;
	}

	// insert at the queue tail
	if (queue->tail == NULL)
	{
		// no buffers are in the queue yet
		queue->tail = inDataBuffer;
	}
	else
	{
		// set the previous pointer to the current tail
		inDataBuffer->prev = queue->tail;

		// insert after the current tail
		queue->tail->next = inDataBuffer;

		// update the queue tail to point to the new last element
		queue->tail = inDataBuffer;
	}
}

// ----------------------------------------------------------------------
UUtUns16
NMrQueue_GetBufferCount(
	NMtQueues				*inQueues,
	NMtQueueRef				inQueueRef)
{
	NMtQueue				*queue;
	UUtUns16				count = 0;
	NMtDataBuffer			*temp;

	// get a pointer to the queue being referenced
	queue = NMiGetQueue(inQueues, inQueueRef);
	if (queue)
	{
		// count the number of buffers in the queue
		temp = queue->head;

		while (temp)
		{
			count++;
			temp = temp->next;
		}
	}

	return count;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
NMtDataBuffer*
NMrQueue_GetNextBuffer(
	NMtQueues				*inQueues,
	NMtQueueRef				inQueueRef,
	NMtDataBuffer			*inDataBuffer)
{
	NMtDataBuffer			*return_buffer;

	// if inDataBuffer != NULL then return the next data buffer in the queue
	if (inDataBuffer)
	{
		return_buffer = inDataBuffer->next;
	}
	else
	{
		// inDataBuffer == NULL, return the first data buffer in the queue
		NMtQueue			*queue;

		// get a pointer to the queue being referenced
		queue = NMiGetQueue(inQueues, inQueueRef);

		return_buffer = queue->head;
	}

	return return_buffer;
}

// ----------------------------------------------------------------------
void
NMrQueue_RemoveBuffer(
	NMtQueues				*inQueues,
	NMtQueueRef				inQueueRef,
	NMtDataBuffer			*inDataBuffer)
{
	NMtQueue				*queue;

	// get a pointer to the queue being referenced
	queue = NMiGetQueue(inQueues, inQueueRef);

	// update the next buffers prev pointer
	if (inDataBuffer->next)
	{
		inDataBuffer->next->prev = inDataBuffer->prev;
	}
	else
	{
		// this buffer is at the tail of the queue, so the queue's tail
		// needs to be set to the buffer's prev pointer
		queue->tail = inDataBuffer->prev;
	}

	// update the prev buffers next pointer
	if (inDataBuffer->prev)
	{
		inDataBuffer->prev->next = inDataBuffer->next;
	}
	else
	{
		// this buffer is at the head of the queue, so the queue's head
		// needs to be set to the buffer's next pointer
		queue->head = inDataBuffer->next;
	}

	// set the buffer's pointer's to NULL
	inDataBuffer->next = NULL;
	inDataBuffer->prev = NULL;
}
