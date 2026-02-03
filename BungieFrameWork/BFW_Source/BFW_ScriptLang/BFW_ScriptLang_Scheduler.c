/*
	FILE:	BFW_ScriptLang_Scheduler.c

	AUTHOR:	Brent H. Pease

	CREATED: Oct 29, 1999

	PURPOSE:

	Copyright 1999

*/

#include "BFW.h"
#include "BFW_TemplateManager.h"
#include "BFW_AppUtilities.h"
#include "BFW_Console.h"

#include "BFW_ScriptLang.h"
#include "BFW_ScriptLang_Private.h"
#include "BFW_ScriptLang_Token.h"
#include "BFW_ScriptLang_Context.h"
#include "BFW_ScriptLang_Database.h"
#include "BFW_ScriptLang_Scheduler.h"
#include "BFW_ScriptLang_Parse.h"
#include "BFW_ScriptLang_Eval.h"
#include "BFW_ScriptLang_Error.h"

#define SLcScheduler_MaxTasks	(32)

typedef enum SLtScheduledTask_Type
{
	SLcScheduledTask_Type_Unused,
	SLcScheduledTask_Type_Script,
	SLcScheduledTask_Type_Command

} SLtScheduledTask_Type;

typedef struct SLtScheduledTask_Script
{
	SLtContext*	context;
	UUtUns32	wakeTime;
	UUtBool		sleeping;

} SLtScheduledTask_Script;

typedef struct SLtScheduledTask_Command
{
	SLtEngineCommand	command;

} SLtScheduledTask_Command;

typedef struct SLtScheduledTask
{
	SLtScheduledTask_Type	type;

	UUtUns32				count;
	UUtUns32				delta;
	UUtUns32				nextGameTime;

	SLtErrorContext			errorContext;

	const char*				name;
	UUtUns16				numParams;
	SLtParameter_Actual			params[SLcScript_MaxNumParams];

	union
	{
		SLtScheduledTask_Script		script;
		SLtScheduledTask_Command	command;

	} u;

} SLtScheduledTask;

SLtScheduledTask	SLgSchedule_Tasks[SLcScheduler_MaxTasks];
UUtUns32			SLgSchedule_GameTime;

static SLtScheduledTask*
SLiSchedule_Task_Find(
	SLtContext*	inContext)
{
	UUtUns16	itr;

	if(inContext == NULL)
	{
		for(itr = 0; itr < SLcScheduler_MaxTasks; itr++)
		{
			if(SLgSchedule_Tasks[itr].type == SLcScheduledTask_Type_Unused) return SLgSchedule_Tasks + itr;
		}
	}
	else
	{
		for(itr = 0; itr < SLcScheduler_MaxTasks; itr++)
		{
			if(SLgSchedule_Tasks[itr].type == SLcScheduledTask_Type_Script &&
				SLgSchedule_Tasks[itr].u.script.context == inContext)
			{
				return SLgSchedule_Tasks + itr;
			}
		}
	}

	return NULL;
}

static void
SLiSchedule_Task_Add(
	SLtScheduledTask*		inTask,
	SLtErrorContext*		inErrorContext,
	SLtScheduledTask_Type	inType,
	const char*				inName,
	UUtUns32				inTimeDelta,
	UUtUns32				inNumberOfTimes)
{
	inTask->type = inType;
	inTask->count = inNumberOfTimes;
	inTask->delta = inTimeDelta;
	inTask->nextGameTime = SLgSchedule_GameTime + inTask->delta;
	inTask->errorContext = *inErrorContext;
	inTask->name = inName;
}

static void
SLiSchedule_Task_Delete(
	SLtScheduledTask*		inTask)
{
	if(inTask->type == SLcScheduledTask_Type_Script)
	{
		SLrContext_Delete(inTask->u.script.context);
	}

	inTask->type = SLcScheduledTask_Type_Unused;
}

static UUtError
SLiSchedule_Add_Script(
	const char*			inName,
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext,
	UUtUns16			inNumParams,
	SLtParameter_Actual*		inParams,
	UUtUns32			inTimeDelta,
	UUtUns32			inNumberOfTimes)
{
	SLtScheduledTask*	newTask;

	newTask = SLiSchedule_Task_Find(NULL);

	if (NULL == newTask) {
		COrConsole_Printf("SLiSchedule_Task_Find FAILED [IMPORTANT]");
	}

	UUmError_ReturnOnNull(newTask);

	newTask->u.script.context = inContext;
	newTask->u.script.sleeping = UUcFalse;

	newTask->numParams = inNumParams;
	if(inNumParams > 0)
	{
		UUrMemory_MoveFast(inParams, newTask->params, inNumParams * sizeof(*inParams));
	}

	SLiSchedule_Task_Add(
		newTask,
		inErrorContext,
		SLcScheduledTask_Type_Script,
		inName,
		inTimeDelta,
		inNumberOfTimes);

	return UUcError_None;
}

static UUtError
SLiSchedule_Add_Command(
	const char*			inName,
	SLtErrorContext*	inErrorContext,
	UUtUns16			inNumParams,
	SLtParameter_Actual*		inParams,
	SLtEngineCommand	inCommand,
	UUtUns32			inTimeDelta,
	UUtUns32			inNumberOfTimes)
{
	SLtScheduledTask*	newTask;

	newTask = SLiSchedule_Task_Find(NULL);
	UUmError_ReturnOnNull(newTask);

	newTask->numParams = inNumParams;
	UUrMemory_MoveFast(inParams, newTask->params, inNumParams * sizeof(*inParams));

	newTask->u.command.command = inCommand;

	SLiSchedule_Task_Add(
		newTask,
		inErrorContext,
		SLcScheduledTask_Type_Command,
		inName,
		inTimeDelta,
		inNumberOfTimes);

	return UUcError_None;
}

static UUtError
SLrSchedule_Function_Script(
	const char*				inName,
	SLtErrorContext*		inErrorContext,
	SLtSymbol_Func_Script*	inScript,
	UUtUns16				inParameterListLength,
	SLtParameter_Actual*			inPassedInParams,
	SLtParameter_Actual			*ioReturnValue,
	UUtUns32				inTimeDelta,
	UUtUns32				inNumberOfTimes,
	SLtContext **			ioReferencePtr)
{
	UUtError			error;
	SLtContext*			context;

	// create the context
	context = SLrContext_New(ioReferencePtr);
	UUmError_ReturnOnNull(context);




	if(inTimeDelta == 0 && inNumberOfTimes == 1)
	{
		// run immediately

		// push the initial state on the function stack
			error =
				SLrParse_FuncStack_Push(
					context,
					inErrorContext,
					inParameterListLength,
					inPassedInParams,
					inName);
			if(error != UUcError_None) return error;

		error =
			SLrScript_Parse(
				context,
				inErrorContext,
				SLcParseMode_InitialExecution);
		if(error != UUcError_None) return error;

		if(ioReturnValue != NULL)
		{
			// process return value here
			SLrExpr_GetResult(context, inErrorContext, &ioReturnValue->type, &ioReturnValue->val);
		}

		if ((context->ticksTillCompletion == 0) && (!context->stalled)) {
			// CB: the script has finished executing immediately, delete the context
//			SLrContext_Delete(context);
		}
	}
	else
	{
		if(inScript->returnType != SLcType_Void)
		{
			SLrScript_Error_Semantic(inErrorContext, "function \"%s\" can be scheduled and return a value", inName);
			return UUcError_Generic;
		}

		// schedule this context
		SLiSchedule_Add_Script(
			inName,
			context,
			inErrorContext,
			inParameterListLength,
			inPassedInParams,
			inTimeDelta,
			inNumberOfTimes);
	}

	return UUcError_None;
}

static UUtError
SLrSchedule_Function_Engine(
	const char*				inName,
	SLtErrorContext*		inErrorContext,
	SLtSymbol_Func_Command*	inCommand,
	UUtUns16				inParameterListLength,
	SLtParameter_Actual*			inPassedInParams,
	SLtParameter_Actual			*ioReturnValue,
	UUtUns32				inTimeDelta,
	UUtUns32				inNumberOfTimes)
{
	UUtError	error;

	if(inTimeDelta == 0 && inNumberOfTimes == 1)
	{
		// run immediately
		error =
			inCommand->command(
				inErrorContext,
				inParameterListLength,
				inPassedInParams,
				NULL,
				NULL,
				ioReturnValue);
		if(error != UUcError_None) return error;

	}
	else
	{
		// schedule
		error =
			SLiSchedule_Add_Command(
				inName,
				inErrorContext,
				inParameterListLength,
				inPassedInParams,
				inCommand->command,
				inTimeDelta,
				inNumberOfTimes);
		if(error != UUcError_None) return error;
	}

	return UUcError_None;
}

UUtError
SLrScheduler_Execute(
	const char*			inName,
	SLtErrorContext*	inErrorContext,
	UUtUns16			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	SLtParameter_Actual		*ioReturnValue,
	UUtUns32			inTimeDelta,		// if 0 script is executed immediately
	UUtUns32			inNumberOfTimes,	// if 0 script executes every game tick(not recommended)
	SLtContext **		ioReferencePtr)		// used for getting a reference to track currently-running scripts
{
	UUtError					error;
	SLtSymbol*					symbol;

	SLtSymbol_Func_Script*		script;
	SLtSymbol_Func_Command*		command;

	error = SLrScript_Database_Symbol_Get(NULL, inName, &symbol);
	if(error != UUcError_None)
	{
		SLrScript_Error_Semantic(inErrorContext, "function \"%s\" does not exist", inName);
		return UUcError_Generic;
	}

	switch(symbol->kind)
	{
		case SLcSymbolKind_Func_Script:
			script = &symbol->u.funcScript;

			inErrorContext->fileName = symbol->fileName;

			error =
				SLrSchedule_Function_Script(
					inName,
					inErrorContext,
					script,
					inParameterListLength,
					inParameterList,
					ioReturnValue,
					inTimeDelta,
					inNumberOfTimes,
					ioReferencePtr);
			if(error != UUcError_None) return error;	// someday report something meaningful here
			break;

		case SLcSymbolKind_Func_Command:
			command = &symbol->u.funcCommand;

			if(command->numParamListOptions > 0)
			{
				error =
					SLrCommand_ParameterCheckAndPromote(
						inErrorContext,
						symbol->name,
						command,
						&inParameterListLength,
						inParameterList);
				if(error != UUcError_None) return error;
			}

			error =
				SLrSchedule_Function_Engine(
					inName,
					inErrorContext,
					command,
					inParameterListLength,
					inParameterList,
					ioReturnValue,
					inTimeDelta,
					inNumberOfTimes);
			break;

		default:
			SLrScript_Error_Semantic(inErrorContext, "identifier is not a function");
			return UUcError_Generic;
	}

	return UUcError_None;
}

UUtError
SLrScheduler_Schedule(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext,
	UUtUns32			inTicksTillWake)
{
	UUtError			error;
	SLtScheduledTask*	targetTask;

	targetTask = SLiSchedule_Task_Find(inContext);
	if(targetTask == NULL)
	{
		error =
			SLiSchedule_Add_Script(
				inContext->funcStack[0].funcName,
				inContext,
				inErrorContext,
				0,
				NULL,
				0,
				1);
		if(error != UUcError_None) return error;
		targetTask = SLiSchedule_Task_Find(inContext);
		UUmAssertReadPtr(targetTask, sizeof(*targetTask));
	}
	targetTask->errorContext = *inErrorContext;

	targetTask->u.script.sleeping = UUcTrue;
	targetTask->u.script.wakeTime = SLgSchedule_GameTime + inTicksTillWake;

	return UUcError_None;
}

// is a task currently executing?
UUtBool
SLrScheduler_Executing(
	void *				inContext)
{
	return (SLiSchedule_Task_Find((SLtContext *) inContext) != NULL);
}

UUtError
SLrScheduler_Remove(
	void *		inContext)
{
	SLtScheduledTask*	task;

	if (inContext == NULL) {
		return UUcError_Generic;
	}

	// find the task that is running this context.
	UUmAssertReadPtr(inContext, sizeof(SLtContext));
	task = SLiSchedule_Task_Find((SLtContext *) inContext);
	if (task == NULL) {
		return UUcError_Generic;
	}

	// remove the task from our list of currently running tasks.
	SLiSchedule_Task_Delete(task);
	return UUcError_None;
}

// force deletion of a context
void
SLrScheduler_ForceDelete(
	void *				inContext)
{
	SLtScheduledTask*	task;

	task = SLiSchedule_Task_Find((SLtContext *) inContext);
	if (task != NULL) {
		SLrContext_Delete((SLtContext *) inContext);
	}
}

UUtError
SLrScheduler_Update(
	UUtUns32	inGameTime)
{
	UUtError			error;
	UUtUns16			itr;
	SLtScheduledTask*	curTask;
	UUtBool				sleeping;

	sleeping = UUcFalse;

	SLgSchedule_GameTime = inGameTime;

	for(itr = 0, curTask = SLgSchedule_Tasks;
		itr < SLcScheduler_MaxTasks;
		itr++, curTask++)
	{
		switch(curTask->type)
		{
			case SLcScheduledTask_Type_Script:
				if(curTask->u.script.sleeping)
				{
					if(curTask->u.script.wakeTime <= inGameTime)
					{
						curTask->u.script.sleeping = UUcFalse;
						// continue execution
						error =
							SLrScript_Parse(
								curTask->u.script.context,
								&curTask->errorContext,
								SLcParseMode_ContinueExecution);
						if(error != UUcError_None)
						{
							SLiSchedule_Task_Delete(curTask);
							return error;
						}

						if(!curTask->u.script.sleeping && curTask->count <= 1)
						{
							SLiSchedule_Task_Delete(curTask);
						}
					}
				}
				else if(curTask->nextGameTime <= inGameTime)
				{
					// start execution
						curTask->u.script.context->funcTOS = 0;

						error =
							SLrParse_FuncStack_Push(
								curTask->u.script.context,
								&curTask->errorContext,
								curTask->numParams,
								curTask->params,
								curTask->name);
						if(error != UUcError_None) return error;

					error =
						SLrScript_Parse(
							curTask->u.script.context,
							&curTask->errorContext,
							SLcParseMode_InitialExecution);
					if(error != UUcError_None)
					{
						// stop this task
						SLiSchedule_Task_Delete(curTask);
						return error;
					}

					if(!curTask->u.script.sleeping)
					{
						curTask->count--;
						if(curTask->count == 0)
						{
							SLiSchedule_Task_Delete(curTask);
						}
						else
						{
							// reschedule
							curTask->nextGameTime += curTask->delta;
						}
					}
				}
				break;

			case SLcScheduledTask_Type_Command:
				if(curTask->nextGameTime <= inGameTime)
				{
					UUtBool	stalled = UUcFalse;

					// start execution
					error =
						curTask->u.command.command(
							&curTask->errorContext,
							curTask->numParams,
							curTask->params,
							NULL,
							&stalled,
							NULL);
					if(error != UUcError_None)
					{
						SLiSchedule_Task_Delete(curTask);
						return error;
					}

					if(!stalled)
					{
						curTask->count--;
						if(curTask->count == 0)
						{
							SLiSchedule_Task_Delete(curTask);
						}
						else
						{
							// reschedule
							curTask->nextGameTime += curTask->delta;
						}
					}
				}
				break;
		}
	}

	return UUcError_None;
}

UUtError
SLrScheduler_Initialize(
	void)
{
	UUtUns16	itr;

	for(itr = 0; itr < SLcScheduler_MaxTasks; itr++)
	{
		SLgSchedule_Tasks[itr].type = SLcScheduledTask_Type_Unused;
	}

	SLgSchedule_GameTime = 0;

	return UUcError_None;
}

void
SLrScheduler_Terminate(
	void)
{

}

#if TOOL_VERSION
void SLrScript_Debug_Counters(SLtScript_Debug_Counters *outScriptDebugCounters)
{
	UUtUns32 itr;
	UUtUns32 scripts_running = 0;
	UUtUns32 scripts_running_watermark = 0;
	UUtUns32 scripts_running_max = SLcScheduler_MaxTasks;


	for(itr = 0; itr < SLcScheduler_MaxTasks; itr++)
	{
		if(SLgSchedule_Tasks[itr].type != SLcScheduledTask_Type_Unused) {
			scripts_running++;
		}
	}

	outScriptDebugCounters->scripts_running = scripts_running;
	outScriptDebugCounters->scripts_running_watermark = scripts_running_watermark;
	outScriptDebugCounters->scripts_running_max = scripts_running_max;

	return;
}

void SLrScript_Debug_Console(void)
{
	SLtScript_Debug_Counters debug_counters;

	SLrScript_Debug_Counters(&debug_counters);

	COrConsole_Printf("scripts running %d", debug_counters.scripts_running);
	COrConsole_Printf("scripts running max %d", debug_counters.scripts_running_max);
	COrConsole_Printf("scripts running watermark %d", debug_counters.scripts_running_watermark);

	{
		UUtUns32 itr;

		for(itr = 0; itr < SLcScheduler_MaxTasks; itr++)
		{
			if(SLgSchedule_Tasks[itr].type != SLcScheduledTask_Type_Unused) {
				COrConsole_Printf("task #%d %s %s %d",
					itr,
					SLgSchedule_Tasks[itr].errorContext.fileName,
					SLgSchedule_Tasks[itr].errorContext.funcName,
					SLgSchedule_Tasks[itr].errorContext.line);
			}
		}
	}

	return;

}
#endif
