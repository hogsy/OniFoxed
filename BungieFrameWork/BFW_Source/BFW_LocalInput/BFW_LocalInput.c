// ======================================================================
// BFW_LocalInput.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include <string.h>
#include <ctype.h>

#include "BFW.h"
#include "BFW_LocalInput.h"
#include "BFW_LI_Platform.h"
#include "BFW_LI_Private.h"
#include "BFW_Console.h"
#include "BFW_ScriptLang.h"
#include "Oni_KeyBindings.h"
#include "Oni_Persistance.h"

// ======================================================================
// defines
// ======================================================================
#define LIcMaxQueueElements			20
#define LIcMaxActionsInBuffer		120
#define LIcActionSamplesPerSec		60
#define LIcMaxBindingsPerActionDescription	5
#define LIcMaxArgs							32
#define LIcMaxVarValueLength				64
#define LIcMaxBindings				100

#if UUmCompiler == UUmCompiler_MWerks
#pragma profile off
#endif

// ======================================================================
// typedefs
// ======================================================================

typedef struct LItEventQueueElement LItEventQueueElement;
struct LItEventQueueElement
{
	LItEventQueueElement	*next;
	LItInputEvent			event;	
};

typedef struct LItEventQueue
{
	LItEventQueueElement	*head;
	LItEventQueueElement	*tail;
	
} LItEventQueue;

// ======================================================================
// globals
// ======================================================================
UUtBool							LIgMouse_Invert = UUcTrue;				// also set every time we change mode

LItMode							LIgMode_Internal = LIcMode_Normal;
static LItMode					LIgMode_External = LIcMode_Normal;
static UUtBool					LIgGameIsActive = UUcTrue;

static LItEventQueue			LIgEventQueue;
static LItEventQueue			LIgEmptyQueue;

static LItAction				LIgBuffer[2][LIcMaxActionsInBuffer];	// Input buffer
static UUtUns16					LIgBufferIndex[2];						// Index of next slot in active input buffer
static UUtUns16					LIgActiveBuffer;						// The active buffer

static LItBinding				LIgBindingArray[LIcMaxBindings];

static UUtInterruptProcRef		*LIgInterruptProcRef;			// Reference for my timer proc
static LItCheatHook				LIgCheatHook = NULL;

// False is the default value of LIgActionBufferUnavail so that the programmer
// doesn't have to remember to initialize it.  It is good to initialize it, but
// this is safer than having a variable that has to be initialize to True.
static volatile UUtBool			LIgActionBufferUnavail;	


#if UUmPlatform == UUmPlatform_Win32
HANDLE gInputMutex= NULL;
#endif		

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
LIrActionBuffer_Enter(
	void)
{
#if UUmPlatform == UUmPlatform_Win32
	WaitForSingleObject(gInputMutex, INFINITE);
#else
	
	while(LIgActionBufferUnavail)
	{
	};
	
	LIgActionBufferUnavail=UUcTrue;
#endif
}

	
// ----------------------------------------------------------------------
static void
LIrActionBuffer_Leave(
	void)
{
#if UUmPlatform == UUmPlatform_Win32
	ReleaseMutex(gInputMutex);
#else
	LIgActionBufferUnavail=UUcFalse;
#endif
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
LIiBind(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	LIrBinding_Add(LIrTranslate_InputName(inParameterList[0].val.str), inParameterList[2].val.str);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
LIiUnbind(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	LIrBinding_Remove(LIrTranslate_InputName(inParameterList[0].val.str));

	return UUcError_None;
}
	
// ----------------------------------------------------------------------
static UUtError
LIiUnbindAll(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	LIrBindings_RemoveAll();
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static LItEventQueueElement*
LIiEventQueue_Dequeue(
	LItEventQueue			*ioQueue)
{
	LItEventQueueElement	*temp;
	
	// get the head element
	temp = ioQueue->head;
	
	// if temp != NULL then there was at least one element in the queue
	// so set the queue's head to the next element in the queue.  If
	// there is only one element in the queue, next will be NULL, and
	// the head will be set properly.
	if (temp == NULL) return NULL;
	
	ioQueue->head = ioQueue->head->next;
	
	// if there was only one element in the queue, then set the tail to NULL
	if (temp == ioQueue->tail)
	{
		ioQueue->tail = NULL;
	}
	
	// temp no longer points to anything
	temp->next = NULL;
	
	// return the element
	return temp;
}

// ----------------------------------------------------------------------
static void
LIiEventQueue_Enqueue(
	LItEventQueue			*ioQueue,
	LItEventQueueElement	*ioElement)
{
	UUmAssert(ioElement);
	
	// there is nothing after the element
	ioElement->next = NULL;

	// if the queue isn't null then the current tail
	// needs to point to the new tail
	if (ioQueue->tail)
	{
		ioQueue->tail->next = ioElement;
	}

	// insert the element at the tail of the queue
	ioQueue->tail = ioElement;
	
	// mark the head if there are no elements in the queue
	if (ioQueue->head == NULL)
	{
		ioQueue->head = ioElement;
	}
}

// ----------------------------------------------------------------------
static UUtError
LIiEventQueue_Initialize(
	void)
{
	UUtUns16			i;
	
	// initialize the queue
	LIgEmptyQueue.head	= NULL;
	LIgEmptyQueue.tail	= NULL;
	LIgEventQueue.head	= NULL;
	LIgEventQueue.tail	= NULL;
	
	// add elements to the empty queue
	for (i = 0; i < LIcMaxQueueElements; i++)
	{
		LItEventQueueElement		*temp;
		
		// allocate a new element
		temp =
			(LItEventQueueElement*)UUrMemory_Block_New(
				sizeof(LItEventQueueElement));
		UUmError_ReturnOnNull(temp);
		
		// insert the element into the empty queue
		LIiEventQueue_Enqueue(&LIgEmptyQueue, temp);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
LIiEventQueue_Terminate(
	void)
{
	LItEventQueueElement		*temp;
	
	// clear the empty queue
	temp = LIiEventQueue_Dequeue(&LIgEmptyQueue);
	while (temp)
	{
		UUrMemory_Block_Delete(temp);
		temp = LIiEventQueue_Dequeue(&LIgEmptyQueue);
	}
	LIgEmptyQueue.head = NULL;
	LIgEmptyQueue.tail = NULL;
	
	// clear the event queue
	temp = LIiEventQueue_Dequeue(&LIgEventQueue);
	while (temp)
	{
		UUrMemory_Block_Delete(temp);
		temp = LIiEventQueue_Dequeue(&LIgEventQueue);
	}
	LIgEventQueue.head = NULL;
	LIgEventQueue.tail = NULL;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
#if UUmCompiler == UUmCompiler_MWerks
//#pragma profile off
#endif

void
LIrActionBuffer_Add(
	LItAction			*outAction,
	LItDeviceInput		*inDeviceInput)
{
	UUtUns16			i;
	LItBinding			*binding;
	
	#if UUmPlatform != UUmPlatform_Mac
		
		// The mac does not like hitting asserts at interrupt time when debugging
		
		UUmAssertReadPtr(inDeviceInput, sizeof(*inDeviceInput));
		UUmAssertReadPtr(outAction, sizeof(*outAction));
		UUmAssert(inDeviceInput->input != LIcKeyCode_None);
	
	#endif
	
	// find the binding assigned to this input
	for (i = 0; i < LIcMaxBindings; i++)
	{
		binding = &LIgBindingArray[i];
		if (binding->boundInput == inDeviceInput->input) break;
	}
	
	// if no binding was found, exit
	if (i == LIcMaxBindings) return;
	
	#if UUmPlatform != UUmPlatform_Mac
		// set the data in the action
		UUmAssertReadPtr(binding->action, sizeof(binding->action));
	#endif
	
	switch (binding->action->inputType)
	{
		case LIcIT_Button:
			outAction->buttonBits |=
				LImMakeBitMask(binding->action->actionData);
		break;
		
		case LIcIT_Axis_Asymmetric:
			outAction->analogValues[binding->action->actionData] +=
				inDeviceInput->analogValue;
		break;
		
		case LIcIT_Axis_Symmetric_Pos:
			outAction->analogValues[binding->action->actionData] +=
				inDeviceInput->analogValue;
		break;
		
		case LIcIT_Axis_Symmetric_Neg:
			outAction->analogValues[binding->action->actionData] -=
				inDeviceInput->analogValue;
		break;
		
		case LIcIT_Axis_Delta:
			outAction->analogValues[binding->action->actionData] +=
				inDeviceInput->analogValue;
		break;
	}
}

#if UUmCompiler == UUmCompiler_MWerks
//#pragma profile reset
#endif

static UUtBool
LIiInterruptHandleProc(
	void	*refcon);
// ----------------------------------------------------------------------
void
LIrActionBuffer_Get(
	UUtUns16			*outNumActionsInBuffer,
	LItAction			**outActionBuffer)
{
	if (LIgMode_Internal != LIcMode_Game) {
		*outNumActionsInBuffer = 0;
		*outActionBuffer = NULL;
	}
	else {
		UUtUns16	returnBuffer;

		// take control of the action buffer
		LIrActionBuffer_Enter();
			
		// Get the buffer which will be returned
		returnBuffer = LIgActiveBuffer;
		
		// Flip the buffer
		LIgActiveBuffer = !LIgActiveBuffer;
		
		*outNumActionsInBuffer = LIgBufferIndex[returnBuffer];
		LIgBufferIndex[returnBuffer] = 0;
		*outActionBuffer = LIgBuffer[returnBuffer];
		
		// release the action buffer
		LIrActionBuffer_Leave();
	}

	return;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
static LItBinding*
LIiBindings_GetFreeBinding(
	void)
{
	UUtUns16				i;
	LItBinding				*binding;
	
	for (i = 0; i < LIcMaxBindings; i++)
	{
		binding = &LIgBindingArray[i];
		
		if (binding->boundInput == LIcKeyCode_None) break;
	}
	
	if (i == LIcMaxBindings)
	{
		binding = NULL;
	}
	
	return binding;
}

// ----------------------------------------------------------------------
UUtError
LIrBinding_Add(
	UUtUns32			inBoundInput,
	const char			*inActionName)
{
	UUtError				error;
	LItBinding				*binding;
	LItActionDescription	*ad;
	UUtUns16				i;
	UUtBool					found_binding;
	
	// don't bind to LIcKeyCode_None
	if (inBoundInput == LIcKeyCode_None) return UUcError_None;
	
	// init vars that need initing
	error = UUcError_None;
		
	// take control of the action buffer
	LIrActionBuffer_Enter();
	
	// ------------------------------
	// search for a binding with a matching boundInput
	// ------------------------------
	found_binding = UUcFalse;
	for (i = 0; i < LIcMaxBindings; i++)
	{
		binding = &LIgBindingArray[i];
		if (binding->boundInput == inBoundInput)
		{
			found_binding = UUcTrue;
			break;
		}
	}
	
	// ------------------------------
	// find all the action descriptions and calculate the number of actions
	// ------------------------------
	// convert inActionName to lowercase
	
	// search for a matching action description
	for (ad = LIgActionDescriptions;
		 ad->actionName[0] != '\0';
		 ad++)
	{
		if (UUrString_Compare_NoCase(ad->actionName, inActionName) == 0) break;
	}
	
	// if no actions were found, then exit
	if (ad->actionName[0] == '\0')
	{
		// XXX - perhaps this should set error to LIcError_ActionNotFound or something similar
		goto exit;
	}

	// ------------------------------
	// set the binding
	// ------------------------------
	// if the binding already exists, then change it's action
	// don't allow the escape key's binding to change.  This
	// test for LIcKeyCode_Escape has to happen here because
	// the binding for escape goes through this function and if
	// inBoundInput is checked against LIcKeyCode_Escape, the 
	// escape key will never get bound.
	if ((found_binding) && (binding->boundInput != LIcKeyCode_Escape))
	{
		binding->action = ad;
	}
	else
	{
		// get a free binding
		binding = LIiBindings_GetFreeBinding();
		if (binding == NULL)
		{
			error = UUcError_OutOfMemory;
			goto exit;
		}
		
		// set the bound input
		binding->boundInput = inBoundInput;
		
		// add the action descriptions to the binding
		binding->action = ad;
	}

exit:
	// release the action buffer
	LIrActionBuffer_Leave();
	
	//UUrMemory_Block_VerifyList();
	
	return error;
}

// ----------------------------------------------------------------------
void
LIrBindings_Enumerate(
	LItEnumBindings		inBindingEnumerator,
	UUtUns32			inUserParam)
{
	UUtUns16			i;

	UUmAssert(inBindingEnumerator);
	
	for (i = 0; i < LIcMaxBindings; i++)
	{
		UUtBool			result;
		LItBinding		*binding;
		
		binding = &LIgBindingArray[i];
		if (binding->boundInput == LIcKeyCode_None)
		{
			continue;
		}
		
		result =
			inBindingEnumerator(
				binding->boundInput,
				binding->action->actionData,
				inUserParam);
		if (result == UUcFalse) break;
	}
}

// ----------------------------------------------------------------------
static UUtError
LIiBindings_Initialize(
	void)
{
	UUtUns16			i;
	
	// initialize LIgBindingArray
	for (i = 0; i < LIcMaxBindings; i++)
	{
		LIgBindingArray[i].boundInput = LIcKeyCode_None;
		LIgBindingArray[i].action = NULL;
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
LIrBinding_Remove(
	UUtUns32			inBoundInput)
{
	LItBinding			*binding;
	UUtUns16			i;
	
	// don't unbind escape
	if ((inBoundInput == LIcKeyCode_Escape) ||
		(inBoundInput == LIcKeyCode_None))
	{
		return;
	}
	
	// take control of the action buffer
	LIrActionBuffer_Enter();
	
	// search for a binding with a matching boundInput
	for (i = 0; i < LIcMaxBindings; i++)
	{
		binding = &LIgBindingArray[i];
		if (binding->boundInput == inBoundInput) break;
	}
	
	// turn off the binding
	if (i != LIcMaxBindings)
	{
		binding->boundInput = LIcKeyCode_None;
		binding->action = NULL;
	}

	// release the action buffer
	LIrActionBuffer_Leave();
}

// ----------------------------------------------------------------------
static void
LIiBindings_RemoveAll(
	void)
{
	UUtUns16			i;
	
	// take control of the action buffer
	LIrActionBuffer_Enter();
	
	// turn off all of the bindings
	for (i = 0; i < LIcMaxBindings; i++)
	{
		LItBinding		*binding;
		
		binding = &LIgBindingArray[i];
		
		binding->boundInput = LIcKeyCode_None;
		binding->action = NULL;
	}
	
	// release the action buffer
	LIrActionBuffer_Leave();
}

// ----------------------------------------------------------------------
void
LIrBindings_RemoveAll(
	void)
{
	// remove all of the bindings
	LIiBindings_RemoveAll();
	
	// put escape back
	LIrBinding_Add(LIcKeyCode_Escape, "escape");
}

// ----------------------------------------------------------------------
static void
LIiBindings_Terminate(
	void)
{
	// remove all of the bindings
	LIiBindings_RemoveAll();
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
LIrInputEvent_Add(
	LItInputEventType	inEventType,
	IMtPoint2D			*inPoint,
	UUtUns32			inKey,
	UUtUns32			inModifiers)
{
	LItEventQueueElement	*temp;
	
	// don't add events when in game mode
	if (LIgMode_Internal == LIcMode_Game) return;
	
	// get a queue element from the empty queue
	temp = LIiEventQueue_Dequeue(&LIgEmptyQueue);
	if (temp == NULL) return;
	
	// set up outInputEvent
	temp->event.type		= inEventType;
	if (inPoint)
	{
		temp->event.where.x		= inPoint->x;
		temp->event.where.y		= inPoint->y;
	}
	else
	{
		temp->event.where.x		= 0;
		temp->event.where.y		= 0;
	}
	temp->event.key			= (UUtUns16)inKey;
	temp->event.modifiers	=
		LIrPlatform_InputEvent_InterpretModifiers(
			inEventType,
			inModifiers);
	
	// add the event to the queue
	LIiEventQueue_Enqueue(&LIgEventQueue, temp);
}

// ----------------------------------------------------------------------
UUtBool
LIrInputEvent_Get(
	LItInputEvent		*outInputEvent)
{
	LItEventQueueElement	*temp;
	
	// update the input system
	LIrUpdate();
	
	// set up outInputEvent
	outInputEvent->type			= LIcInputEvent_None;
	outInputEvent->where.x		= 0;
	outInputEvent->where.y		= 0;
	outInputEvent->key			= 0;
	outInputEvent->modifiers	= 0;
	
	// try to get an event from the event queue and return the event
	// if one was available
	temp = LIiEventQueue_Dequeue(&LIgEventQueue);
	if (temp)
	{
		// copy the event data from temp to outInputEvent
		*outInputEvent = temp->event;

		if (NULL != LIgCheatHook) {
			LIgCheatHook(outInputEvent);
		}
		
		// put temp into the empty queue
		LIiEventQueue_Enqueue(&LIgEmptyQueue, temp);
		
		return UUcTrue;
	}
	
	return UUcFalse;
}

// ----------------------------------------------------------------------
void
LIrInputEvent_GetMouse(
	LItInputEvent		*outInputEvent)
{
	LIrPlatform_InputEvent_GetMouse(LIgMode_Internal, outInputEvent);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
LItMode
LIrMode_Get(
	void)
{
	return LIgMode_External;
}

// ----------------------------------------------------------------------
static void LIrMode_Set_Internal(void)
{
	LItMode new_mode;

	LIgMouse_Invert = ONrPersist_IsInvertMouseOn();
	
	if (!LIgGameIsActive) {
		new_mode = LIcMode_Normal;
	}
	else {
		new_mode = LIgMode_External;
	}

	// don't switch modes if it isn't necessary
	if (LIgMode_Internal == new_mode) return;
	
	// update the platform level
	LIrPlatform_Mode_Set(new_mode);
	
	// clear out any old events
	while (LIrPlatform_Update(new_mode)){};
	
	// record the mode change
	LIgMode_Internal = new_mode;

	return;
}

void LIrMode_Set(LItMode inMode)
{
	LIgMode_External = inMode;

	LIrMode_Set_Internal();

	return;
}

void LIrGameIsActive(UUtBool inGameIsActive)
{
	LIgGameIsActive = inGameIsActive;

	LIrMode_Set_Internal();

	return;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
#if UUmCompiler == UUmCompiler_MWerks
//#pragma profile off
#endif

static UUtBool
LIiInterruptHandleProc(
	void				*refcon)
{
	UUtBool call_the_thread;

#if UUmPlatform == UUmPlatform_Win32
	call_the_thread = (LIgInterruptProcRef != NULL);
#else
	call_the_thread = (LIgInterruptProcRef != NULL) && (LIgMode_Internal == LIcMode_Game) && (!LIgActionBufferUnavail);
#endif

	if (call_the_thread)
	{
		// take control of the action buffers
		LIrActionBuffer_Enter();
		
		// wrap the action buffer if it is full
		if(LIgBufferIndex[LIgActiveBuffer] >= LIcMaxActionsInBuffer)
		{
			LIgBufferIndex[LIgActiveBuffer] = 0;
		}
		
		// get an action from the platform
		LIrPlatform_PollInputForAction(
			&LIgBuffer[LIgActiveBuffer][LIgBufferIndex[LIgActiveBuffer]]);

		// advance to the next buffer in the buffer
		LIgBufferIndex[LIgActiveBuffer]++;
		
		// release the action buffers
		LIrActionBuffer_Leave();
	}

	return UUcTrue;	// Re-install interrupt proc
}

#if UUmPlatform == UUmPlatform_Mac
// use this function to get input in-game; do not use the interrupt proc mechanism, as that will crash your system.
static void mac_get_input_in_game(
	void)
{
	if ((LIgMode_Internal == LIcMode_Game) && (!LIgActionBufferUnavail))
	{
		// take control of the action buffers
		LIrActionBuffer_Enter();
		
		// wrap the action buffer if it is full
		if(LIgBufferIndex[LIgActiveBuffer] >= LIcMaxActionsInBuffer)
		{
			LIgBufferIndex[LIgActiveBuffer] = 0;
		}
		
		// get an action from the platform
		LIrPlatform_PollInputForAction(
			&LIgBuffer[LIgActiveBuffer][LIgBufferIndex[LIgActiveBuffer]]);

		// advance to the next buffer in the buffer
		LIgBufferIndex[LIgActiveBuffer]++;
		
		// release the action buffers
		LIrActionBuffer_Leave();
	}
	
	return;
}
#endif

#if UUmCompiler == UUmCompiler_MWerks
//#pragma profile reset
#endif

// ----------------------------------------------------------------------
static void UUcExternal_Call
LIiTerminate_AtExit(
	void)
{
	// this can be called reentrantly under Win32 so make sure that doesn't happen
	static UUtBool inside = UUcFalse;		
		
	// take control of the action buffer
	LIrActionBuffer_Enter();
	
	if (!inside)
	{
		inside = UUcTrue;
		
		if (LIgInterruptProcRef != NULL)
		{
			UUrInterruptProc_Deinstall(LIgInterruptProcRef);
			LIgInterruptProcRef = NULL;
		}
	}
	
	inside = UUcFalse;

	// release the action buffer
	LIrActionBuffer_Leave();
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
LIrInitialize(
	UUtAppInstance		inInstance,
	UUtWindow			inWindow)
{
	UUtError			error;

	UUrStartupMessage("initializing input system...");

#if UUmPlatform == UUmPlatform_Win32
	gInputMutex = CreateMutex(NULL, FALSE, NULL);
	error = (NULL == gInputMutex) ? UUcError_Generic : UUcError_None;
	UUmError_ReturnOnErrorMsg(error, "Could not create mutex");
#endif

	// initialize the globals
	LIgActiveBuffer			= 0;
	LIgBufferIndex[0]		= 0;
	LIgBufferIndex[1]		= 0;
	LIgInterruptProcRef 	= NULL;
	LIgActionBufferUnavail	= UUcFalse;
	
	// register an atexit proc
	UUrAtExit_Register(LIiTerminate_AtExit);
	
	// initialize the bindings
	error = LIiBindings_Initialize();
	UUmError_ReturnOnError(error);
	
	// initialize the queues
	error = LIiEventQueue_Initialize();
	UUmError_ReturnOnError(error);
	
	// initialize the platform specific input stuff
	error = LIrPlatform_Initialize(inInstance, inWindow);
	UUmError_ReturnOnError(error);
	
	// install the interrupt handler
#if UUmPlatform != UUmPlatform_Mac
	// the whole interrupt proc thing does not work on Mac. don't use it.
	error =
		UUrInterruptProc_Install(
			LIiInterruptHandleProc,
			LIcActionSamplesPerSec,
			NULL,
			&LIgInterruptProcRef);
	if(error != UUcError_None)
	{
		UUmError_ReturnOnErrorMsg(error, "Could not install timer proc.");
	}
#endif

	// ------------------------------
	// set up the commands
	// ------------------------------
	error =
		SLrScript_Command_Register_Void(
			"bind",
			"binds an input to a function",
			"input_name:string to:string{\"to\"} input_function:string",
			LIiBind);
	UUmError_ReturnOnError(error);

	error =
		SLrScript_Command_Register_Void(
			"unbind",
			"removes a binding from a input function",
			"input_name:string",
			LIiUnbind);
	UUmError_ReturnOnError(error);

	error =
		SLrScript_Command_Register_Void(
			"unbindall",
			"removes all bindings",
			"",
			LIiUnbindAll);
	UUmError_ReturnOnError(error);

	// ------------------------------
	// bind the keys and set up the mouse
	// ------------------------------
	LIrBinding_Add(LIcKeyCode_Escape, "escape");
	LIrBinding_Add(LIcKeyCode_Multiply, "escape");
	LIrBinding_Add(LIcKeyCode_A, "stepleft");
	LIrBinding_Add(LIcKeyCode_S, "backward");
	LIrBinding_Add(LIcKeyCode_D, "stepright");
	LIrBinding_Add(LIcKeyCode_W, "forward");
	LIrBinding_Add(LIcKeyCode_Space, "jump");
	LIrBinding_Add(LIcKeyCode_Q, "swap");
	LIrBinding_Add(LIcKeyCode_E, "drop");
	LIrBinding_Add(LIcKeyCode_F, "punch");
	LIrBinding_Add(LIcKeyCode_C, "kick");
	LIrBinding_Add(LIcMouseCode_Button1, "fire1");
	LIrBinding_Add(LIcMouseCode_Button2, "fire2");
	LIrBinding_Add(LIcMouseCode_Button3, "fire3");
	LIrBinding_Add(LIcKeyCode_X, "secretx");
	LIrBinding_Add(LIcKeyCode_Y, "secrety");
	LIrBinding_Add(LIcKeyCode_Z, "secretz");
	LIrBinding_Add(LIcMouseCode_XAxis, "aim_LR");
	LIrBinding_Add(LIcMouseCode_YAxis, "aim_UD");
	LIrBinding_Add(LIcKeyCode_F1, "pausescreen");

	LIrBinding_Add(LIcKeyCode_V,"lookmode");
	LIrBinding_Add(LIcKeyCode_LeftShift, "crouch");
	LIrBinding_Add(LIcKeyCode_CapsLock, "walk");
	LIrBinding_Add(LIcKeyCode_LeftControl, "action");
	LIrBinding_Add(LIcKeyCode_Tab, "hypo");
	LIrBinding_Add(LIcKeyCode_R, "reload");

	LIrBinding_Add(LIcKeyCode_P, "screenshot");

#if TOOL_VERSION
	LIrInitializeDeveloperKeys();
#endif

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
LIrInitializeDeveloperKeys(
	void)
{
	LIrBinding_Add(LIcKeyCode_BackSlash, "profile_toggle");
	LIrBinding_Add(LIcKeyCode_Grave, "console");

	LIrBinding_Add(LIcKeyCode_U, "unstick");

	LIrBinding_Add(LIcKeyCode_F2, "cutscene1");
	LIrBinding_Add(LIcKeyCode_F3, "cutscene2");
	LIrBinding_Add(LIcKeyCode_F4, "f4");
	LIrBinding_Add(LIcKeyCode_F5, "f5");
	LIrBinding_Add(LIcKeyCode_F6, "f6");
	LIrBinding_Add(LIcKeyCode_F7, "f7");
	LIrBinding_Add(LIcKeyCode_F8, "f8");

	LIrBinding_Add(LIcKeyCode_F9, "start_record");
	LIrBinding_Add(LIcKeyCode_F10, "stop_record");
	LIrBinding_Add(LIcKeyCode_F11, "play_record");

	LIrBinding_Add(LIcKeyCode_N, "camera_record");
	LIrBinding_Add(LIcKeyCode_M, "camera_stop");
	LIrBinding_Add(LIcKeyCode_Comma, "camera_play");

	LIrBinding_Add(LIcKeyCode_L, "record_screen");
	LIrBinding_Add(LIcKeyCode_Insert, "addflag");
	LIrBinding_Add(LIcKeyCode_Delete, "deleteflag");
	
	LIrBinding_Add(LIcKeyCode_Subtract, "man_cam_move_up");
	LIrBinding_Add(LIcKeyCode_Add, "man_cam_move_down");
	LIrBinding_Add(LIcKeyCode_NumPad1, "man_cam_move_left");
	LIrBinding_Add(LIcKeyCode_NumPad3, "man_cam_move_right");
	LIrBinding_Add(LIcKeyCode_NumPad8, "man_cam_move_forward");
	LIrBinding_Add(LIcKeyCode_NumPad5, "man_cam_move_backward");
	
	LIrBinding_Add(LIcKeyCode_NumPad6, "man_cam_pan_left");
	LIrBinding_Add(LIcKeyCode_NumPad4, "man_cam_pan_right");
	LIrBinding_Add(LIcKeyCode_UpArrow, "man_cam_pan_up");
	LIrBinding_Add(LIcKeyCode_DownArrow, "man_cam_pan_down");
	
	LIrBinding_Add(LIcKeyCode_Comma, "place_quad");
	LIrBinding_Add(LIcKeyCode_Period, "place_quad_mode");
}

// ----------------------------------------------------------------------
void
LIrUnbindDeveloperKeys(
	void)
{
	LIrBinding_Remove(LIcKeyCode_U);

	LIrBinding_Remove(LIcKeyCode_F2);
	LIrBinding_Remove(LIcKeyCode_F3);
	LIrBinding_Remove(LIcKeyCode_F4);
	LIrBinding_Remove(LIcKeyCode_F5);
	LIrBinding_Remove(LIcKeyCode_F6);
	LIrBinding_Remove(LIcKeyCode_F7);
	LIrBinding_Remove(LIcKeyCode_F8);

	LIrBinding_Remove(LIcKeyCode_F9);
	LIrBinding_Remove(LIcKeyCode_F10);
	LIrBinding_Remove(LIcKeyCode_F11);

	LIrBinding_Remove(LIcKeyCode_N);
	LIrBinding_Remove(LIcKeyCode_M);
	LIrBinding_Remove(LIcKeyCode_Comma);

	LIrBinding_Remove(LIcKeyCode_L);
	LIrBinding_Remove(LIcKeyCode_Insert);
	LIrBinding_Remove(LIcKeyCode_Delete);
	
	LIrBinding_Remove(LIcKeyCode_Subtract);
	LIrBinding_Remove(LIcKeyCode_Add);
	LIrBinding_Remove(LIcKeyCode_NumPad1);
	LIrBinding_Remove(LIcKeyCode_NumPad3);
	LIrBinding_Remove(LIcKeyCode_NumPad8);
	LIrBinding_Remove(LIcKeyCode_NumPad5);
	
	LIrBinding_Remove(LIcKeyCode_NumPad6);
	LIrBinding_Remove(LIcKeyCode_NumPad4);
	LIrBinding_Remove(LIcKeyCode_UpArrow);
	LIrBinding_Remove(LIcKeyCode_DownArrow);
	
	LIrBinding_Remove(LIcKeyCode_Comma);
	LIrBinding_Remove(LIcKeyCode_Period);
}

// ----------------------------------------------------------------------
void
LIrTerminate(
	void)
{
	// remove the interrupt proc
	if(LIgInterruptProcRef != NULL)
	{
		UUrInterruptProc_Deinstall(LIgInterruptProcRef);
		LIgInterruptProcRef = NULL;
	}
	
	// terminate the platform specific input stuff
	LIrPlatform_Terminate();
	
	// terminate the queue
	LIiEventQueue_Terminate();
	
	// terminate the bindings
	LIiBindings_Terminate();

#if UUmPlatform == UUmPlatform_Win32
	if (gInputMutex) CloseHandle(gInputMutex);
#endif
}

// ----------------------------------------------------------------------
void
LIrUpdate(
	void)
{
#if UUmPlatform == UUmPlatform_Mac
	// this is used instead of the interrupt proc mechanism, which does not work on Mac.
	mac_get_input_in_game();
#endif

	// read the input data
	LIrPlatform_Update(LIgMode_Internal);
}

// ----------------------------------------------------------------------
UUtBool
LIrTestKey(LItKeyCode inKeyCode)
{
	UUtBool result;

	result = LIrPlatform_TestKey(inKeyCode, LIgMode_Internal);

	return result;
}

UUtBool 
LIrKeyWentDown(LItKeyCode inKeyCode, UUtBool *ioOldKeyDown)
{
	UUtBool key_down = LIrTestKey(inKeyCode);
	UUtBool key_went_down;

	key_went_down = key_down & !(*ioOldKeyDown);
	*ioOldKeyDown = key_down;

	return key_went_down;
}

void LIrInputEvent_CheatHook(LItCheatHook inHook)
{
	LIgCheatHook = inHook;

	return;
}
