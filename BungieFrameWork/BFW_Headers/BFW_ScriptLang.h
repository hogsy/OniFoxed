#pragma once
/*
	FILE:	BFW_ScriptLang.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Oct 29, 1999
	
	PURPOSE: 
	
	This module is designed to let a game designer write simple game logic that
	can communicate with the core game engine. 
	
	Features:
	
	1) different parts of the core engine can register commands to make them
		available to the game designers. An example of this is a command to initiate a
		particle effect or to tell a character to start moving.
		
	2) different parts of the engine can register global variables to make them
		available to the game designers. An example of this is the current game time
		or the number of civilians killed.
		
	3) A simple interpreted language to describe game logic
	   - functions
	   - global/local variables
	   - expressions
	   - if then else statements
	   - iterators
	   - etc
	   
	 4) An event scheduling system. Functions can be scheduled to be called periodically.
	 	For an example a function can be scheduled every 60 game ticks to check for
	 	mission success or failure conditions.
	 	
	 5) Engine can check the value of global vars declared within a script
	 
	 6) scripts can sleep for a specified amount of time
	
	 7) Game designers can modify the scripts while still in the engine.
	 
	Uses:
	 1) Code level logic such as keeping track of number of civilians killed.
	 
	 2) Glue different parts of the system together. When particles hit something
	 	(either wall or character) then can execute a script that can change some
	 	other part of the system. When a trigger volume is hit a script can be executed
	 	to issue orders to the AI system.
	 	
	 3) Implement multiplayer game logic. Score can be kept track with the script system
	 	for an any type of multiplayer game that we come up with.
	
	Copyright 1999

*/

#ifndef BFW_SCRIPTLANG_H
#define BFW_SCRIPTLANG_H

#include "BFW.h"
#include "BFW_TemplateManager.h"

#define SLcScript_MaxNumParams		(8)
#define SLcScript_MaxNameLength		(32)
#define SLcScript_String_MaxLength	(64)

typedef struct SLtContext	SLtContext;

typedef enum SLtReadWrite
{
	SLcReadWrite_ReadOnly,
	SLcReadWrite_ReadWrite
	
} SLtReadWrite;

typedef enum SLtType
{
	SLcType_Int32		= 0,
	SLcType_String		= 1,
	SLcType_Float		= 2,
	SLcType_Bool		= 3,
	SLcType_Void		= 4,		// used to signal no return value
	SLcType_Max			= 5
	
} SLtType;

extern const char *SLcTypeName[];

typedef union SLtValue
{
	UUtInt32	i;
	float		f;
	char*		str;
	/*
	S.S. 11/20/2000
	some parts of the scripting system depend on the boolean value being in the
	low byte of this union, which doesn't implicitly happen on big-endian systems
	this change makes it so.
	*/
#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Mac)
	UUtUns32	b;
#else
	UUtBool		b;
#endif	
} SLtValue;

typedef struct SLtParameter_Actual
{
	SLtType		type;
	SLtValue	val;
	
} SLtParameter_Actual;

typedef struct SLtParameter_Formal
{
	const char*	name;
	SLtType		type;
	
} SLtParameter_Formal;

typedef struct SLtErrorContext
{
	const char*	funcName;
	const char*	fileName;
	UUtUns16	line;
	
} SLtErrorContext;

typedef UUtError
(*SLtEngineCommand)(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue);

typedef UUtBool
(*SLtScript_Iterator_GetFirst)(
	SLtValue	*ioFirstValue);

typedef UUtBool
(*SLtScript_Iterator_GetNext)(
	SLtValue	*ioNextValue);

typedef void
(*SLtNotificationProc)(
	void);

/*
 * HOW TO USE inParameterSpecification
 *
 * inParameterSpecification is used to have the engine automatically type check your
 * parameters. Directions:
 *
 * - If you don't want type checking pass in NULL
 * - If the function has no parameters pass in ""
 *
 * - a single parameter is specified by "param_name:type" where type is int, string, or float
 * - a group of parameters is specified by [] such as "[a:int b:int c:int]"
 * - if there are a limited set of values for a parameter you can use {} to specify them
 *   eg. "a:int{0 | 1 | 2}" or "b:string{\"yes\" | \"no\"}
 * - you can use | to specify either or. eg. "a:int | b:int"
 * - to specify an optional parameter use something like this "[a:int | ]"
 * - to specify a repeating parameter use + such as "a:string flags:int+"
 *
 * I suggest you look at some examples in the code to get the hang of it.
 */

/*
 * Registration commands
 *
 * These are used to register functions, variables, and iterators to the scripting engine
 */

UUtError
SLrScript_Command_Register_ReturnType(
	const char*			inCommandName,
	const char*			inDescription,
	const char*			inParameterSpecification,
	SLtType				inReturnType,
	SLtEngineCommand	inCommandFunc);

UUtError
SLrScript_Command_Register_Void(
	const char*			inCommandName,
	const char*			inDescription,
	const char*			inParameterSpecification,
	SLtEngineCommand	inCommandFunc);


typedef struct SLtRegisterVoidFunctionTable
{
	const char *name;
	const char *description;
	const char *parameters;
	SLtEngineCommand function;
} SLtRegisterVoidFunctionTable;

UUtError 
SLrScript_CommandTable_Register_Void(SLtRegisterVoidFunctionTable *inTable);

UUtError
SLrScript_Iterator_Register(
	const char*					inIteratorName,
	SLtType						inType,
	SLtScript_Iterator_GetFirst	inGetFirstFunc,
	SLtScript_Iterator_GetNext	inGetNextFunc);

UUtError
SLrGlobalVariable_Register(					// use to register a variable that is to be exposed to scripts
	const char*			inName,
	const char*			inDescription,
	SLtReadWrite		inReadWrite,
	SLtType				inType,
	SLtValue*			inVariableAddress,
	SLtNotificationProc	inNotificationProc);

typedef struct SLtRegisterInt32Table
{
	const char *name;
	const char *description;
	UUtInt32 *variableAddress;
} SLtRegisterInt32Table;

typedef struct SLtRegisterBoolTable
{
	const char *name;
	const char *description;
	UUtBool *variableAddress;
} SLtRegisterBoolTable;

typedef struct SLtRegisterFloatTable
{
	const char *name;
	const char *description;
	float *variableAddress;
} SLtRegisterFloatTable;

typedef struct SLtRegisterStringTable
{
	const char *name;
	const char *description;
	char *variableAddress;
} SLtRegisterStringTable;

UUtError
SLrGlobalVariable_Register_Int32(
	const char*	inName,
	const char*	inDescription,
	UUtInt32*	inVariableAddress);

UUtError
SLrGlobalVariable_Register_Float(
	const char*	inName,
	const char*	inDescription,
	float*		inVariableAddress);

UUtError
SLrGlobalVariable_Register_Bool(
	const char*	inName,
	const char*	inDescription,
	UUtBool*	inVariableAddress);

UUtError
SLrGlobalVariable_Register_String(
	const char*	inName,
	const char*	inDescription,
	char*		inVariableAddress);

UUtError SLrGlobalVariable_Register_Int32_Table(SLtRegisterInt32Table *inTable);
UUtError SLrGlobalVariable_Register_Float_Table(SLtRegisterFloatTable *inTable);
UUtError SLrGlobalVariable_Register_Bool_Table(SLtRegisterBoolTable *inTable);
UUtError SLrGlobalVariable_Register_String_Table(SLtRegisterStringTable *inTable);

/*
 * Execution commands
 *
 * These are used by the engine to execute commands in the scripting engine
 */

UUtError	
SLrScript_Schedule(
	const char*				inName,
	UUtUns16				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	SLtParameter_Actual		*ioReturnValue,
	UUtUns32				inTimeDelta,		// if 0 script is executed immediately
	UUtUns32				inNumberOfTimes,	// if 0 script executes every game tick(not recommended)
	SLtContext **			ioReferencePtr);	// used for getting a reference to track currently-running scripts

UUtError	
SLrScript_ExecuteOnce(
	const char*				inName,
	UUtUns16				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	SLtParameter_Actual		*ioReturnValue,
	SLtContext **			ioReferencePtr);	// used for getting a reference to track currently-running scripts);

// is a task currently executing?
UUtBool
SLrScheduler_Executing(
	void *				inContext);

// remove a currently executing task
UUtError
SLrScheduler_Remove(
	void *				inContext);

// force deletion of a context
void
SLrScheduler_ForceDelete(
	void *				inContext);


/*
 * Database commands
 *
 * These are used to add functions and variables defined in text files
 */
 
void
SLrScript_Database_Reset(
	void);

UUtError
SLrScript_Database_Add(
	const char*	inFileName,
	const char*	inText);

/*
 * 
 */

void
SLrScript_LevelBegin(
	void);

void
SLrScript_LevelEnd(
	void);

/*
 * Initialize and terminate commands
 */

UUtError
SLrScript_Initialize(
	void);

void
SLrScript_Terminate(
	void);

/*
 * Update function. Should be called once per heartbeat because it interacts with other systems
 */
UUtError
SLrScript_Update(
	UUtUns32	inGameTime);

/*
 * miscl
 */

UUtError
SLrGlobalVariable_GetValue(					// use to get a value from a global var
	const char*	inName,
	SLtType		*outType,
	SLtValue	*outValue);

void
SLrScript_ConsoleCompletionList_Get(
	UUtUns32		*outNumNames,
	char			***outList);

void UUcArglist_Call
SLrScript_ReportError(
	SLtErrorContext*	inErrorContext,
	char*				inMsg,
	...);

/*
 *
 */

#if TOOL_VERSION

typedef struct SLtScript_Debug_Counters {
	UUtUns32 scripts_running;
	UUtUns32 scripts_running_watermark;
	UUtUns32 scripts_running_max;
} SLtScript_Debug_Counters;

void SLrScript_Debug_Counters(SLtScript_Debug_Counters *outScriptDebugCounters);
void SLrScript_Debug_Console(void);

#endif

#endif /* BFW_SCRIPTLANG_H */
