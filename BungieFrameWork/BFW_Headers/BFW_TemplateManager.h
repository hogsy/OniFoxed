	#pragma once
/*
	FILE:	BFW_TemplateManager.h

	AUTHOR:	Brent H. Pease

	CREATED: June 8, 1997

	PURPOSE: Manage templates and instance data

	Copyright 1997

*/
#ifndef BFW_TEMPLATEMANAGER_H
#define BFW_TEMPLATEMANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "BFW.h"
#include "BFW_FileManager.h"

/*
 *
 */
	#define TMcMagicCookie					UUm4CharToUns32('M', 'A', 'G', 'K')

	#define TMcMaxLevelNum					(128)

	#define TMcTemplateTag_None		UUm4CharToUns32('N', 'O', 'N', 'E')

	#define TMcInstanceName_MaxLength		(128)

	#define TMcMaxStringLength				(128)

	#define TMcSeparateFileOffset_None		((TMtSeparateFileOffset) 0)

/*
 *
 */
	typedef enum TMtAllowFolding
	{
		TMcFolding_Forbid,
		TMcFolding_Allow

	} TMtAllowFolding;

/*
 *
 */
	typedef enum TMtTemplateFlags
	{
		TMcTemplateFlag_None			= 0,
		TMcTemplateFlag_Registered		= (1 << 0),
		TMcTemplateFlag_Leaf			= (1 << 1),
		TMcTemplateFlag_VarArrayIsLeaf	= (1 << 2),
		TMcTemplateFlag_AllowFolding	= (1 << 3)

	} TMtTemplateFlags;

/*
 *
 */
	typedef enum TMtDynamicPool_Type
	{
		TMcDynamicPool_Type_Temporary = 2,
		TMcDynamicPool_Type_Permanent = 3

	} TMtDynamicPool_Type;


/*
 * These are the types of messages for runtime procedures
 */
	typedef enum TMtTemplateProc_Message
	{
		TMcTemplateProcMessage_NewPostProcess,
		TMcTemplateProcMessage_LoadPostProcess,
		TMcTemplateProcMessage_DisposePreProcess,
		TMcTemplateProcMessage_Update,
		TMcTemplateProcMessage_PrepareForUse,

		TMcTemplateProcMessage_Dummy = (1 << 31)

	} TMtTemplateProc_Message;

/*
 * These are the priorities for cached templates
 */
	typedef enum TMtInstancePriorities
	{
		TMcInstancePriority_Unknown		= 0,
		TMcInstancePriority_NotNeeded	= 1,
		TMcInstancePriority_NeededSoon	= 2,
		TMcInstancePriority_NeededNow	= 3,

		TMcInstancePriority_Num			= 4

	} TMtInstancePriorities;

/*
 * Some typedefs
 */
	typedef UUtUns32				TMtTemplateTag;			// Identifies the specific template
	typedef UUtUns32				TMtPlaceHolder;			// This is used when an instance in one file references an instance in another
	typedef UUtUns32				TMtSeparateFileOffset;	// offset into a separate binary companion file

	typedef struct TMtCache_MemoryPool	TMtCache_MemoryPool;
	typedef struct TMtPrivateData		TMtPrivateData;


/*
 * The typedef for a routine that receives runtime template messages
 */
	typedef UUtError
	(*TMtTemplateProc_Handler)(
		TMtTemplateProc_Message	inMessage,
		void*					inInstancePtr,
		void*					inPrivateData);

	typedef UUtError
	(*TMtTemplateProc_ByteSwap)(
		void*					inInstancePtr);

/*
 * Template definition
 */
	typedef struct TMtTemplateDefinition
	{
		UUtUns64					checksum;
		TMtTemplateTag				tag;
		char*						name;
		UUtUns8*					swapCodes;

		TMtTemplateFlags			flags;

		UUtUns32					size;			// This is the size with a var array length of zero
		UUtUns32					varArrayElemSize;

		TMtTemplateProc_ByteSwap	byteSwapProc;

		UUtUns32					magicCookie;
		void						*timer;
	} TMtTemplateDefinition;

/*
 * Some basic templates
 */
	#define TMcTemplate_IndexArray	UUm4CharToUns32('I', 'D', 'X', 'A')
	typedef tm_template('I', 'D', 'X', 'A', "Index Array")
	TMtIndexArray
	{
		tm_pad					pad0[20];

		tm_varindex	UUtUns32	numIndices;
		tm_vararray	UUtUns32	indices[1];

	} TMtIndexArray;


	typedef tm_struct TMtTemplateRef
	{
		tm_templateref	templateRef;

	} TMtTemplateRef;

	#define TMcTemplate_TemplateRefArray UUm4CharToUns32('T', 'M', 'R', 'A')
	typedef tm_template('T', 'M', 'R', 'A', "Template Reference Array")
	TMtTemplateRefArray
	{
		tm_pad						pad0[20];

		tm_varindex	UUtUns32		numRefs;
		tm_vararray	TMtTemplateRef	templateRefs[1];

	} TMtTemplateRefArray;

	#define TMcTemplate_FloatArray	UUm4CharToUns32('T', 'M', 'F', 'A')
	typedef tm_template('T', 'M', 'F', 'A', "Float Array")
	TMtFloatArray
	{
		tm_pad						pad0[22];

		tm_varindex UUtUns16		numFloats;
		tm_vararray float			numbers[1];

	} TMtFloatArray;

	#define TMcTemplate_String UUm4CharToUns32('T', 'S', 't', 'r')
	typedef tm_template('T', 'S', 't', 'r', "String")
	TMtString
	{

		char				string[128];	// must match TMcMaxStringLength above
	} TMtString;

	#define TMcTemplate_StringArray	UUm4CharToUns32('S', 't', 'N', 'A')
	typedef tm_template('S', 't', 'N', 'A', "String Array")
	TMtStringArray
	{
		tm_pad						pad0[22];

		tm_varindex UUtUns16		numStrings;
		tm_vararray TMtString		*string[1];
	} TMtStringArray;

/*
 * Initialize the template manager
 *	IN
 *		inGame				- UUcTrue if we are running the game
 *		inGameDataFolderRef	- A folder reference for the game data
 */
	UUtError
	TMrInitialize(
		UUtBool			inGame,
		BFtFileRef*		inGameDataFolderRef);

/*
 * Terminate the template manager
 */
	void
	TMrTerminate(
		void);

/*
 * Register a template.
 *	IN
 *		inTemplateTag - The template to be registered
 *		inSize - The expected size
 *		inAllowProcs - UUcTrue if app can register callback procs
 *	NOTES
 *		This is important since the compiler can pad structures in unexpected ways...
 */
	UUtError
	TMrTemplate_Register(
		TMtTemplateTag		inTemplateTag,
		UUtUns32			inSize,
		TMtAllowFolding		inAllowFolding);

	UUtError
	TMrRegisterTemplates(
		void);

/*
 * This is used to add custom byte swapping for the var array field
 */
	UUtError
	TMrTemplate_InstallByteSwap(
		TMtTemplateTag				inTemplateTag,
		TMtTemplateProc_ByteSwap	inProc);

/*
 * Call a handler function for every instance of the given type
 */
	UUtError
	TMrTemplate_CallProc(
		TMtTemplateTag				inTemplateTag,
		TMtTemplateProc_Message		inMessage);

/*
 * Create a cache for a template type
 */
	typedef UUtError
	(*TMtCacheProc_Simple_Load)(
		void*		inInstanceData,
		void*		*outDataPtr);

	typedef void
	(*TMtCacheProc_Simple_Unload)(
		void*		inInstanceData,
		void*		inDataPtr);

	typedef void
	(*TMtCacheProc_MemoryPool_Load)(
		void*		inInstanceData,
		void*		inMemoryPtr);

	typedef void
	(*TMtCacheProc_MemoryPool_Unload)(
		void*		inInstanceData,
		void*		inDataPtr);

	typedef UUtUns32
	(*TMtCacheProc_MemoryPool_ComputeSize)(
		void*		inInstanceData);

	UUtError
	TMrTemplate_Cache_MemoryPool_New(
		TMtTemplateTag					inTemplateTag,
		UUtUns32						inMemoryPoolSize,
		TMtCacheProc_MemoryPool_Load	inLoadProc,
		TMtCacheProc_MemoryPool_Unload	inUnloadProc,
		TMtCache_MemoryPool*			*outCache);

	void
	TMrTemplate_Cache_MemoryPool_Delete(
		TMtCache_MemoryPool*	inCache);

	void*
	TMrTemplate_Cache_MemoryPool_GetDataPtr(
		TMtCache_MemoryPool*	inCache,
		void*					inInstanceDataPtr);

/*
 * Allow the allocation of private data associated with instances of common template
 */
	UUtError
	TMrTemplate_PrivateData_New(
		TMtTemplateTag				inTemplateTag,
		UUtUns32					inDataSize,
		TMtTemplateProc_Handler		inProcHandler,
		TMtPrivateData*				*outPrivateData);

	void
	TMrTemplate_PrivateData_Delete(
		TMtPrivateData*				inPrivateData);

	void*
	TMrTemplate_PrivateData_GetDataPtr(
		TMtPrivateData*				inPrivateData,
		const void*					inInstancePtr);

/*
 * This is used to load a new level
 */

	typedef enum
	{
		TMcPrivateData_Yes,
		TMcPrivateData_No

	} TMtAllowPrivateData;

	UUtBool
	TMrLevel_Exists(
		UUtUns16			inLevelNumber);

	void
	TMrLevel_Unload(
		UUtUns16			inLevelNumber);

	UUtError
	TMrLevel_Load(
		UUtUns16			inLevelNumber,
		TMtAllowPrivateData	inAllowPrivateData);

/*
 * Get a pointer to the requested instance
 *	IN
 *		inTemplateTag - The template
 *		inInstanceTag - The instance
 *	OUT
 *		outDataPtr - The pointer to the data
 */
	UUtError
	TMrInstance_GetDataPtr(
		TMtTemplateTag		inTemplateTag,
		const char*			inInstanceName,
		void*				*outDataPtr);


	void *TMrInstance_GetFromName(TMtTemplateTag, const char *inInstanceName);

/*
 * Get a list of pointers to instances of a specific type
 */
	UUtError
	TMrInstance_GetDataPtr_List(
		TMtTemplateTag		inTemplateTag,
		UUtUns32			inMaxPtrs,
		UUtUns32			*outNumPtrs,
		void*				*outPtrList);

/*
 * Get a pointer by number
 */

	UUtError
	TMrInstance_GetDataPtr_ByNumber(
		TMtTemplateTag		inTemplateTag,
		UUtUns32			inNumber,
		void*				*outDataPtr);

/*
 * Get a total count of pointer to instances of a specific type
 */

	UUtUns32
	TMrInstance_GetTagCount(
		TMtTemplateTag		inTemplateTag);


/*
 * This is used to create a runtime instance
 */

	UUtError
	TMrInstance_Dynamic_New(
		TMtDynamicPool_Type		inDynamicPoolType,
		TMtTemplateTag			inTemplateTag,
		UUtUns32				inInitialVarArrayLength,
		void*					*outDataPtr);

	void *TMrInstancePool_Allocate(
		TMtDynamicPool_Type		inDynamicPoolType,
		UUtUns32				inNumBytes);

/*
 * This is called when an instances data has been dynamically changed
 */
	UUtError
	TMrInstance_Update(
		void*				inDataPtr);

/*
 * This needs to be called before a instance is used to update the caches
 */
	UUtError
	TMrInstance_PrepareForUse(
		void*				inDataPtr);

/*
 * Get the template tag for this instance
 */
	TMtTemplateTag
	TMrInstance_GetTemplateTag(
		const void*	instanceDataPtr);

/*
 * Get the instance name for this instance
 */
	char*
	TMrInstance_GetInstanceName(
		const void*	instanceDataPtr);

/*
 * Accessors for the binary data that belongs to this instance
 */
	void*
	TMrInstance_GetRawOffset(
		const void* instanceDataPtr);

	BFtFile*
	TMrInstance_GetSeparateFile(
		const void*	instanceDataPtr);

/*
 * Destroy all the data files
 */
	UUtError
	TMrMameAndDestroy(
		void);

/*
 *
 */
	void
	TMrSwapCode_DumpDefinition(
		FILE*					inFile,
		TMtTemplateDefinition*	inTemplateDefinition);

	void
	TMrSwapCode_DumpAll(
		FILE*	inFile);

extern BFtFileRef				TMgDataFolderRef;

#ifdef __cplusplus
}
#endif

#endif /* BFW_TEMPLATEMANAGER_H */
