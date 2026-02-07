/*
	FILE:	BFW_TM3_Utility.c

	AUTHOR:	Brent H. Pease

	CREATED: July 10, 1998

	PURPOSE:

	Copyright 1998

*/

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_TemplateManager.h"
#include "BFW_AppUtilities.h"
#include "BFW_TM_Private.h"
#include "BFW_TM_Construction.h"

/*
 * =========================================================================
 * D E F I N E S
 * =========================================================================
 */

/*
 * =========================================================================
 * T Y P E S
 * =========================================================================
 */

/*
 * =========================================================================
 * G L O B A L S
 * =========================================================================
 */

UUtBool					TMgInGame;
TMtTemplateDefinition*	TMgTemplateDefinitionsAlloced = NULL;
UUtMemory_String*		TMgTemplateNameAlloced = NULL;
BFtFileRef				TMgDataFolderRef;

/*
 * =========================================================================
 * P R I V A T E   F U N C T I O N S
 * =========================================================================
 */

static void
TMiSwapCode_Dump_Indent(
	FILE*		inFile,
	UUtUns16	inIndent,
	UUtUns32	inDataOffset,
	UUtUns32	inSwapCodeOffset)
{
	UUtUns32	itr;

	fprintf(inFile, "[%04d, %04d]", inSwapCodeOffset, inDataOffset);

	for(itr = 0; itr < inIndent; itr++)
	{
		fprintf(inFile, "\t");
	}
}

static void
TMiSwapCode_Dump_Recursive(
	FILE*		inFile,
	UUtUns16	inIndent,
	UUtUns8*	*ioSwapCode,
	UUtUns32	*ioDataOffset,
	UUtUns8*	inSwapCodeBase)
{
	UUtUns8*				curSwapCode;
	UUtUns32				curDataOffset = *ioDataOffset;
	char					swapCode;
	UUtBool					stop;
	UUtUns8					count;
	UUtUns32				savedDataOffset;
	UUtUns32				value;

	curSwapCode = *ioSwapCode;

	stop = UUcFalse;

	while(!stop)
	{
		swapCode = *curSwapCode++;

		switch(swapCode)
		{
			case TMcSwapCode_8Byte:
				TMiSwapCode_Dump_Indent(inFile, inIndent, curDataOffset, curSwapCode - inSwapCodeBase - 1);
				fprintf(inFile, "8Byte(%02x)\n", TMcSwapCode_8Byte);
				curDataOffset += 8;
				break;

			case TMcSwapCode_4Byte:
				TMiSwapCode_Dump_Indent(inFile, inIndent, curDataOffset, curSwapCode - inSwapCodeBase - 1);
				fprintf(inFile, "4Byte(%02x)\n", TMcSwapCode_4Byte);
				curDataOffset += 4;
				break;

			case TMcSwapCode_2Byte:
				TMiSwapCode_Dump_Indent(inFile, inIndent, curDataOffset, curSwapCode - inSwapCodeBase - 1);
				fprintf(inFile, "2Byte(%02x)\n", TMcSwapCode_2Byte);
				curDataOffset += 2;
				break;

			case TMcSwapCode_1Byte:
				TMiSwapCode_Dump_Indent(inFile, inIndent, curDataOffset, curSwapCode - inSwapCodeBase - 1);
				fprintf(inFile, "1Byte(%02x)\n", TMcSwapCode_1Byte);
				curDataOffset += 1;
				break;

			case TMcSwapCode_BeginArray:
				TMiSwapCode_Dump_Indent(inFile, inIndent, curDataOffset, curSwapCode - inSwapCodeBase - 1);
				fprintf(inFile, "begin array(%02x)\n", TMcSwapCode_BeginArray);
				count = *curSwapCode++;
				TMiSwapCode_Dump_Indent(inFile, inIndent+1, curDataOffset, curSwapCode - inSwapCodeBase - 1);
				fprintf(inFile, "count: %d\n", count);
				savedDataOffset = curDataOffset;
				TMiSwapCode_Dump_Recursive(
					inFile,
					inIndent+1,
					&curSwapCode,
					&curDataOffset,
					inSwapCodeBase);
				curDataOffset += (curDataOffset - savedDataOffset) * (count - 1);
				break;

			case TMcSwapCode_EndArray:
				stop = UUcTrue;
				TMiSwapCode_Dump_Indent(inFile, inIndent-1, curDataOffset, curSwapCode - inSwapCodeBase - 1);
				fprintf(inFile, "end array(%02x)\n", TMcSwapCode_EndArray);
				break;

			case TMcSwapCode_BeginVarArray:
				TMiSwapCode_Dump_Indent(inFile, inIndent, curDataOffset, curSwapCode - inSwapCodeBase - 1);
				fprintf(inFile, "begin vararray(%02x)\n", TMcSwapCode_BeginVarArray);
				TMiSwapCode_Dump_Indent(inFile, inIndent+1, curDataOffset, curSwapCode - inSwapCodeBase);
				fprintf(inFile, "counttype: ");
				switch(*curSwapCode++)
				{
					case TMcSwapCode_8Byte:
						fprintf(inFile, "8byte(%02x)\n", TMcSwapCode_8Byte);
						curDataOffset += 8;
						break;
					case TMcSwapCode_4Byte:
						fprintf(inFile, "4byte(%02x)\n", TMcSwapCode_4Byte);
						curDataOffset += 4;
						break;
					case TMcSwapCode_2Byte:
						fprintf(inFile, "2byte(%02x)\n", TMcSwapCode_2Byte);
						curDataOffset += 2;
						break;
					default:
						UUmAssert(!"Swap codes damaged");
				}

				TMiSwapCode_Dump_Recursive(
					inFile,
					inIndent+1,
					&curSwapCode,
					&curDataOffset,
					inSwapCodeBase);
				break;

			case TMcSwapCode_EndVarArray:
				TMiSwapCode_Dump_Indent(inFile, inIndent-1, curDataOffset, curSwapCode - inSwapCodeBase - 1);
				fprintf(inFile, "end vararray(%02x)\n", TMcSwapCode_EndVarArray);
				stop = UUcTrue;
				break;

			case TMcSwapCode_TemplatePtr:
				TMiSwapCode_Dump_Indent(inFile, inIndent, curDataOffset, curSwapCode - inSwapCodeBase - 1);
				memcpy(&value, curSwapCode, sizeof(UUtUns32));
				fprintf(inFile, "templatePtr(%02x): %c%c%c%c\n", TMcSwapCode_TemplatePtr,
					(value >> 24) & 0xFF,
					(value >> 16) & 0xFF,
					(value >> 8) & 0xFF,
					(value >> 0) & 0xFF);

				curSwapCode += 4;
				curDataOffset += 4;
				break;

			default:
				UUmDebugStr("swap codes are messed up.");
		}
	}

	*ioSwapCode = curSwapCode;
	*ioDataOffset = curDataOffset;
}

/*
 * =========================================================================
 * P U B L I C   F U N C T I O N S
 * =========================================================================
 */
UUtError
TMrInitialize(
	UUtBool			inGame,
	BFtFileRef*		inGameDataFolderRef)
{
	UUtError	error;

	TMgInGame = inGame;

	UUmAssert(sizeof(TMtInstanceFile_Header) == UUcProcessor_CacheLineSize * 2);

	/*
	 * Get the folder ref
	 */
	TMgDataFolderRef = *inGameDataFolderRef;
	UUrStartupMessage("DataFolder = %s", BFrFileRef_GetLeafName(&TMgDataFolderRef));

	TMrTemplate_BuildList();

	if(inGame == UUcTrue)
	{
		error = TMrGame_Initialize();
		UUmError_ReturnOnError(error);
	}
	else
	{
		error = TMrConstruction_Initialize();
		UUmError_ReturnOnError(error);
	}

	return UUcError_None;
}

UUtError
TMrRegisterTemplates(
	void)
{
	TMrTemplate_Register(TMcTemplate_IndexArray, sizeof(TMtIndexArray), TMcFolding_Allow);
	TMrTemplate_Register(TMcTemplate_TemplateRefArray, sizeof(TMtTemplateRefArray), TMcFolding_Allow);
	TMrTemplate_Register(TMcTemplate_FloatArray, sizeof(TMtFloatArray), TMcFolding_Allow);

	return UUcError_None;
}

void
TMrTerminate(
	void)
{
	if(TMgInGame)
	{
		TMrGame_Terminate();
	}
	else
	{
		TMrConstruction_Terminate();
	}

	return;
}

UUtError
TMrTemplate_Register(
	TMtTemplateTag		inTemplateTag,
	UUtUns32			inSize,
	TMtAllowFolding		inAllowFolding)
{
	TMtTemplateDefinition	*curTemplateDefinition;

	curTemplateDefinition = TMrUtility_Template_FindDefinition(inTemplateTag);

	if(curTemplateDefinition == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "template does not exist");
	}

	if(curTemplateDefinition->flags & TMcTemplateFlag_Registered)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "template already registered");
	}

	// Add 4 to make up for preamble
	if(curTemplateDefinition->size + curTemplateDefinition->varArrayElemSize != inSize + TMcPreDataSize)
	{
		AUrMessageBox(AUcMBType_OK, "Template %s size %d var elem size %d inSize %d pre data size %d",
			curTemplateDefinition->name,
			curTemplateDefinition->size,
			curTemplateDefinition->varArrayElemSize,
			inSize,
			TMcPreDataSize);

		UUmAssert(0);

		UUmError_ReturnOnErrorMsg(UUcError_Generic, "size mismatch, either you need to run the extractor or structure padding is wrong");
	}

	curTemplateDefinition->flags |= TMcTemplateFlag_Registered;

	if(inAllowFolding == TMcFolding_Allow)
	{
		curTemplateDefinition->flags |= TMcTemplateFlag_AllowFolding;
	}

	return UUcError_None;
}

void
TMrSwapCode_DumpDefinition(
	FILE*					inFile,
	TMtTemplateDefinition*	inTemplateDefinition)
{
	fprintf(inFile, "Name: %s\n", inTemplateDefinition->name);
	fprintf(inFile, "Tag: %c%c%c%c\n",
		(inTemplateDefinition->tag >> 24) & 0xFF,
		(inTemplateDefinition->tag >> 16) & 0xFF,
		(inTemplateDefinition->tag >> 8) & 0xFF,
		(inTemplateDefinition->tag >> 0) & 0xFF);

	fprintf(inFile, "Swap Codes:\n");
	{
		UUtUns8*	swapCodes = inTemplateDefinition->swapCodes;
		UUtUns32	dataOffset = 0;

		TMiSwapCode_Dump_Indent(inFile, 0, 0, 0);
		fprintf(inFile, "begin array\n");
		TMiSwapCode_Dump_Recursive(inFile, 1, &swapCodes, &dataOffset, inTemplateDefinition->swapCodes);
	}
}

void
TMrSwapCode_DumpAll(
	FILE*	inFile)
{
	UUtUns32	itr;

	for(itr = 0; itr < TMgNumTemplateDefinitions; itr++)
	{
		fprintf(inFile, "******************************************\n");
		TMrSwapCode_DumpDefinition(inFile, TMgTemplateDefinitionArray + itr);
	}

}

UUtError
TMrTemplate_InstallByteSwap(
	TMtTemplateTag				inTemplateTag,
	TMtTemplateProc_ByteSwap	inProc)
{
	TMtTemplateDefinition*	templatePtr;

	templatePtr = TMrUtility_Template_FindDefinition(inTemplateTag);
	if(templatePtr == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Could not find template");
	}

	UUmAssert(templatePtr->varArrayElemSize > 0);

	templatePtr->byteSwapProc = inProc;

	return UUcError_None;
}

TMtTemplateDefinition*
TMrUtility_Template_FindDefinition(
	TMtTemplateTag		inTemplateTag)
{
	TMtTemplateDefinition*	curTemplate = TMgTemplateDefinitionArray;
	UUtInt32				i;

	if(TMgTemplateDefinitionArray == NULL) return NULL;

	for(i = TMgNumTemplateDefinitions; i-- > 0;)
	{
		if(curTemplate->tag == inTemplateTag)
		{
			return curTemplate;
		}

		curTemplate++;
	}

	if(TMgInGame)
	{
		UUmAssert(0);
		UUrDebuggerMessage(
			"Could not find template %c%c%c%c",
			(inTemplateTag >> 24) & 0xFF,
			(inTemplateTag >> 16) & 0xFF,
			(inTemplateTag >> 8) & 0xFF,
			(inTemplateTag >> 0) & 0xFF);
	}

	return NULL;
}

void
TMrUtility_SkipVarArray(
	UUtUns8*			*ioSwapCode)
{

	UUtBool		stop;
	UUtUns8*	curSwapCode;
	char		swapCode;

	curSwapCode = *ioSwapCode;

	stop = UUcFalse;

	while(!stop)
	{
		swapCode = *curSwapCode++;

		switch(swapCode)
		{
			case TMcSwapCode_8Byte:
				break;

			case TMcSwapCode_4Byte:
				break;

			case TMcSwapCode_2Byte:
				break;

			case TMcSwapCode_1Byte:
				break;

			case TMcSwapCode_BeginArray:
				curSwapCode++;

				TMrUtility_SkipVarArray(&curSwapCode);
				break;

			case TMcSwapCode_EndArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_BeginVarArray:
				curSwapCode++;

				TMrUtility_SkipVarArray(
					&curSwapCode);
				break;

			case TMcSwapCode_EndVarArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_TemplatePtr:
				curSwapCode += 4;
				break;

			default:
				UUmDebugStr("swap codes are messed up.");
		}
	}

	*ioSwapCode = curSwapCode;
}

void
TMrUtility_VarArrayReset_Recursive(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr,
	UUtUns32			inInitialVarArrayLength)
{
	UUtBool		stop;
	UUtUns8*	curSwapCode;
	UUtUns8*	curDataPtr;
	UUtUns32	i;
	UUtUns32	count;
	UUtUns8*	origSwapCode;

	curSwapCode = *ioSwapCode;
	curDataPtr = *ioDataPtr;

	stop = UUcFalse;

	while(!stop)
	{
		switch(*curSwapCode++)
		{
			case TMcSwapCode_8Byte:
				curDataPtr += 8;
				break;

			case TMcSwapCode_4Byte:
				curDataPtr += 4;
				break;

			case TMcSwapCode_2Byte:
				curDataPtr += 2;
				break;

			case TMcSwapCode_1Byte:
				curDataPtr += 1;
				break;

			case TMcSwapCode_BeginArray:
				count = *curSwapCode++;

				origSwapCode = curSwapCode;

				for(i = 0; i < count; i++)
				{
					curSwapCode = origSwapCode;

					TMrUtility_VarArrayReset_Recursive(
						&curSwapCode,
						&curDataPtr,
						inInitialVarArrayLength);
				}
				break;

			case TMcSwapCode_EndArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_BeginVarArray:
				switch(*curSwapCode++)
				{
					case TMcSwapCode_8Byte:
						*(UUtUns64 *)curDataPtr = inInitialVarArrayLength;
						curDataPtr += 8;
						break;

					case TMcSwapCode_4Byte:
						*(UUtUns32 *)curDataPtr = inInitialVarArrayLength;
						curDataPtr += 4;
						break;

					case TMcSwapCode_2Byte:
						*(UUtUns16 *)curDataPtr = (UUtUns16)inInitialVarArrayLength;
						curDataPtr += 2;
						break;

					default:
						UUmDebugStr("swap codes are messed up.");
				}

				origSwapCode = curSwapCode;

				if(inInitialVarArrayLength > 0)
				{
					for(i = 0; i < inInitialVarArrayLength; i++)
					{
						curSwapCode = origSwapCode;

						TMrUtility_VarArrayReset_Recursive(
							&curSwapCode,
							&curDataPtr,
							inInitialVarArrayLength);
					}
				}
				else
				{
					TMrUtility_SkipVarArray(&curSwapCode);
				}
				break;

			case TMcSwapCode_EndVarArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_TemplatePtr:
				*(void**)curDataPtr = NULL;
				curSwapCode += 4;
				curDataPtr += 4;
				break;

			default:
				UUmDebugStr("swap codes are messed up.");
		}
	}

	*ioSwapCode = curSwapCode;
	*ioDataPtr = curDataPtr;
}

UUtError
TMrUtility_LevelInfo_Get(
	BFtFileRef*			inInstanceFileRef,
	UUtUns32			*outLevelNumber,
	char				*outLevelSuffix,
	UUtBool				*outFinal,
	TMtInstanceFile_ID	*outInstanceFileID)
{
	char		nameBuffer[BFcMaxFileNameLength];
	char*		underscore;
	char*		dot;
	char*		cp;
	UUtUns32	levelNum;

	UUtUns32	checksum = 0;
	UUtUns32	factor = 1;
	UUtUns32	c;

	UUrString_Copy(nameBuffer, BFrFileRef_GetLeafName(inInstanceFileRef), BFcMaxFileNameLength);

	underscore = strchr(nameBuffer, '_');
	if(underscore == NULL)
	{
		return UUcError_Generic;
	}

	*underscore = 0;

	sscanf(nameBuffer + 5, "%d", &levelNum);

	cp = underscore + 1;

	dot = strchr(cp, '.');
	if(dot == NULL)
	{
		return UUcError_Generic;
	}

	*dot = 0;

	if(outLevelSuffix != NULL)
	{
		UUrString_Copy(outLevelSuffix, cp, BFcMaxFileNameLength);
	}

	if(!strcmp(cp, "Final"))
	{
		if(outFinal != NULL)
		{
			*outFinal = UUcTrue;
		}
		checksum = 0;
	}
	else
	{
		if(outFinal != NULL)
		{
			*outFinal = UUcFalse;
		}
		while(1)
		{
			c = *cp++;
			if(c == 0) break;

			checksum += (toupper(c) - 'A' + 1) * factor++;
		}
	}

	if(levelNum >= 128)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Can not have a level number larger than 127");
	}

	if(outLevelNumber != NULL)
	{
		*outLevelNumber = levelNum;
	}

	if(outInstanceFileID != NULL)
	{
		*outInstanceFileID = (levelNum << 25) | ((checksum & 0xFFFFFF) << 1) | 1;
	}

	return UUcError_None;
}


UUtError
TMrUtility_DataRef_To_BinaryRef(
	const BFtFileRef*		inOrigFileRef,
	BFtFileRef*				*outNewFileRef,
	const char *			inExtension)
{
	UUtError error;
	char newName[256];
	const char *oldName = BFrFileRef_GetLeafName(inOrigFileRef);
	UUtInt32 length = strlen(oldName);

	if (length > 255) {
		return UUcError_Generic;
	}

	UUmAssert(length > 4);
	strcpy(newName, oldName);

	UUmAssert(tolower(newName[length - 4]) == '.');
	UUmAssert(tolower(newName[length - 3]) == 'd');
	UUmAssert(tolower(newName[length - 2]) == 'a');
	UUmAssert(tolower(newName[length - 1]) == 't');

	UUmAssert(strlen(inExtension) >= 3);

	newName[length - 3] = inExtension[0];
	newName[length - 2] = inExtension[1];
	newName[length - 1] = inExtension[2];

	error = BFrFileRef_DuplicateAndReplaceName(inOrigFileRef, newName, outNewFileRef);
	UUmError_ReturnOnError(error);

	return error;
}

