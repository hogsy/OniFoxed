/*
	FILE:	BFW_TM_Private.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: June 17, 1997
	
	PURPOSE: This is shared between BFW_TemplateManager.c and the template extractor
	
	Copyright 1997

*/
#ifndef TM_PRIVATE_H
#define TM_PRIVATE_H

#include "BFW.h"
#include "BFW_TemplateManager.h"

#define TMcInstanceFile_Version				UUm4CharToUns32('V', 'R', '3', '1')

#define TMmPlaceHolder_GetIndex(ph)			(UUtUns32)(ph >> 8)

#define TMmPlaceHolder_MakeFromIndex(i)		((i << 8) | (0x1))

#define TMmInstanceFile_ID_IsIndex(x)			(x & 1)

#define TMcPreDataSize	(8)

#define TMcLevels_MaxNum	(127)

#define TMcInstanceFile_PrivFiles_Max	(0x1FF)
#define TMcInstanceFile_PrivFiles_Mask	(0x1FF);

#define TMmFileIndex_LevelNumber_Get(x)	((x >> 25) & 0x7F)

typedef UUtUns32	TMtInstanceFile_ID;

typedef struct TMtInstanceFile	TMtInstanceFile;

/*
 * Swap codes that describe the structure of a template
 */
	typedef enum TMtSwapCode
	{
		TMcSwapCode_8Byte = 1,				
		TMcSwapCode_4Byte,				
		TMcSwapCode_2Byte,				
		TMcSwapCode_1Byte,				
		TMcSwapCode_BeginArray,			// Begin fixed length array
		TMcSwapCode_EndArray,			// End fixed length array
		TMcSwapCode_BeginVarArray,		// Begin variable length array
		TMcSwapCode_EndVarArray,		// End variable length array
		TMcSwapCode_TemplatePtr,		// The next four chars represent a template reference
		TMcSwapCode_RawPtr,				// a 4-byte pointer to raw data
		TMcSwapCode_SeparateIndex		// a 4-byte index into the separate-data file

	} TMtSwapCode;

/*
 * Descriptor flags
 */
	typedef enum TMtDescriptorFlags
	{
		TMcDescriptorFlags_None				= 0,
		TMcDescriptorFlags_Unique			= (1 << 0),
		TMcDescriptorFlags_PlaceHolder		= (1 << 1),
		TMcDescriptorFlags_Duplicate		= (1 << 2),			// This instance does not point to its own data - it points to its duplicates
		TMcDescriptorFlags_DuplicatedSrc	= (1 << 3),			// This instance is being used by duplicate instances as a source
		
		/* These are not saved */
		TMcDescriptorFlags_Touched			= (1 << 20),
		TMcDescriptorFlags_InBatchFile		= (1 << 21),
		TMcDescriptorFlags_DeleteMe			= (1 << 22),
		
		TMcDescriptorFlags_PersistentMask	= 0xFFFF
		
	} TMtDescriptorFlags;

/*
 * Template descriptor
 */
	typedef struct TMtTemplateDescriptor
	{
		UUtUns64		checksum;		// The checksum at the time instance file was created
		TMtTemplateTag	tag;			// The tag of the template
		UUtUns32		numUsed;		// The number of instances that use this template
		
	} TMtTemplateDescriptor;

/*
 * This is the instance descriptor structure that is saved to disk
 */
	typedef struct TMtInstanceDescriptor
	{
		TMtTemplateDefinition*	templatePtr;
		UUtUns8*				dataPtr;
		char*					namePtr;
		UUtUns32				size;			// This is the total size including entire var array that is written to disk
		TMtDescriptorFlags		flags;	
	} TMtInstanceDescriptor;
	
	typedef struct TMtNameDescriptor
	{
		UUtUns32	instanceDescIndex;
		char*		namePtr;

	} TMtNameDescriptor;

/*
 * Instance file header
 */
	typedef struct TMtInstanceFile_Header
	{
		UUtUns64		totalTemplateChecksum;

		UUtUns32		version;
		UUtUns16		sizeofHeader;
		UUtUns16		sizeofInstanceDescriptor;
		UUtUns16		sizeofTemplateDescriptor;
		UUtUns16		sizeofNameDescriptor;

		UUtUns32		numInstanceDescriptors;
		UUtUns32		numNameDescriptors;
		UUtUns32		numTemplateDescriptors;

		UUtUns32		dataBlockOffset;
		UUtUns32		dataBlockLength;
		
		UUtUns32		nameBlockOffset;
		UUtUns32		nameBlockLength;
		
		UUtUns32		pad2[4];
		
	} TMtInstanceFile_Header;

extern UUtBool					TMgInGame;
extern BFtFileRef				TMgDataFolderRef;
extern UUtUns32					TMgNumTemplateDefinitions;
extern TMtTemplateDefinition*	TMgTemplateDefinitionArray;
extern UUtUns64					TMgTemplateChecksum;
extern TMtTemplateDefinition*	TMgTemplateDefinitionsAlloced;
extern UUtMemory_String*		TMgTemplateNameAlloced;

void
TMrTemplate_BuildList(
	void);

TMtTemplateDefinition*
TMrUtility_Template_FindDefinition(
	TMtTemplateTag		inTemplateTag);

void
TMrUtility_SkipVarArray(
	UUtUns8*			*ioSwapCode);

void
TMrUtility_VarArrayReset_Recursive(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr,
	UUtUns32			inInitialVarArrayLength);

UUtError
TMrConstruction_Initialize(
	void);

void
TMrConstruction_Terminate(
	void);

UUtError
TMrGame_Initialize(
	void);

void
TMrGame_Terminate(
	void);

UUtError
TMrUtility_LevelInfo_Get(
	BFtFileRef*	inFileRef,
	UUtUns32	*outLevelNumber,
	char		*outLevelSuffix,
	UUtBool		*outFinal,
	UUtUns32	*outFileIndex);

UUtError
TMrUtility_DataRef_To_BinaryRef(
	const BFtFileRef*		inOrigFileRef,
	BFtFileRef*				*outNewFileRef,
	const char *			inExtension);

#endif /* TM_PRIVATE_H */
