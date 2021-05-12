/*
	FILE:	TE_Extract.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: June 18, 1997
	
	PURPOSE: 
	
	Copyright 1997
		
*/

#include <stdio.h>
#include <string.h>

#include "BFW.h"

#include "BFW_TM_Private.h"

#include "TE_Private.h"
#include "TE_Symbol.h"
#include "TE_Extract.h"

static UUtUns32
TEiExtract_ComputeSwapCodeSize(
	TEtType*	inType)
{
	UUtInt16	value16;
	TEtType		*curType;
	UUtUns32	curSize = 0;
	
	UUmAssertReadPtr(inType, sizeof(TEtType));

	switch(inType->kind)
	{
		case TEcTypeKind_Struct:
		case TEcTypeKind_Template:
			curType = inType->baseType;
			while(curType != NULL)
			{
				curSize += TEiExtract_ComputeSwapCodeSize(curType);
				curType = curType->next;
			}
			break;
			
		case TEcTypeKind_Field:
			switch(inType->u.fieldInfo.fieldType)
			{
				case TEcFieldType_Regular:
					curSize += TEiExtract_ComputeSwapCodeSize(inType->baseType);
					break;
					
				case TEcFieldType_VarIndex:
				case TEcFieldType_VarArray:
					curSize++;
					curSize += TEiExtract_ComputeSwapCodeSize(inType->baseType);
					break;
				
				default:
					UUmAssert(!"Illegal case");
			}
			break;
		
		case TEcTypeKind_TemplatePtr:
			curSize += 5;
			break;
		
		case TEcTypeKind_Array:
			value16 = (short)inType->u.arrayInfo.arrayLength;
			
			while(value16 > 0)
			{
				curSize += 3;
				
				UUmAssert(inType->baseType != NULL);
				curSize += TEiExtract_ComputeSwapCodeSize(inType->baseType);
				value16 -= 255;
			}
			break;

		case TEcTypeKind_RawPtr:
			break;
		
		case TEcTypeKind_SeparateIndex:
		case TEcTypeKind_Enum:
		case TEcTypeKind_8Byte:
		case TEcTypeKind_4Byte:
		case TEcTypeKind_2Byte:
		case TEcTypeKind_1Byte:
			curSize++;
			break;
		
		default:
			UUmAssert(!"Illegal case");
	}
	
	return curSize;
}

static void
TEiExtract_WriteSwapCodes_Recursive(
	UUtUns8*	*ioSwapCodePtr,
	TEtType*	inType)
{
	UUtInt16	value16;
	TEtType		*curType;
	UUtUns8*	curSwapCodePtr;
	
	UUmAssertReadPtr(inType, sizeof(TEtType));

	curSwapCodePtr = *ioSwapCodePtr;
	
	switch(inType->kind)
	{
		case TEcTypeKind_Struct:
		case TEcTypeKind_Template:
			curType = inType->baseType;
			while(curType != NULL)
			{
				TEiExtract_WriteSwapCodes_Recursive(
					&curSwapCodePtr,
					curType);
				curType = curType->next;
			}
			break;
			
		case TEcTypeKind_Field:
			switch(inType->u.fieldInfo.fieldType)
			{
				case TEcFieldType_Regular:
					TEiExtract_WriteSwapCodes_Recursive(
						&curSwapCodePtr,
						inType->baseType);
					break;
					
				case TEcFieldType_VarIndex:
					*curSwapCodePtr++ = TMcSwapCode_BeginVarArray;
					TEiExtract_WriteSwapCodes_Recursive(
						&curSwapCodePtr,
						inType->baseType);
					break;
					
				case TEcFieldType_VarArray:
					TEiExtract_WriteSwapCodes_Recursive(
						&curSwapCodePtr,
						inType->baseType);
					*curSwapCodePtr++ = TMcSwapCode_EndVarArray;
					break;
				
				default:
					UUmAssert(!"Illegal case");
			}
			break;
		
		case TEcTypeKind_TemplatePtr:
			*curSwapCodePtr++ = TMcSwapCode_TemplatePtr;
			if(inType->baseType != NULL)
			{
				UUmAssert(inType->baseType->kind == TEcTypeKind_Template);
				*(UUtUns32*)curSwapCodePtr = inType->baseType->u.templateInfo.templateTag;
			}
			else
			{
				*(UUtUns32*)curSwapCodePtr = 0;
			}
			curSwapCodePtr += 4;
			break;

		case TEcTypeKind_RawPtr:
			*curSwapCodePtr++ = TMcSwapCode_RawPtr;
			break;
		
		case TEcTypeKind_Array:
			value16 = (short)inType->u.arrayInfo.arrayLength;
			
			while(value16 > 0)
			{
				*curSwapCodePtr++ = TMcSwapCode_BeginArray;
				
				if(value16 > 255)
				{
					*curSwapCodePtr++ = 255;
				}
				else
				{
					*curSwapCodePtr++ = (UUtUns8)value16;
				}
				
				UUmAssert(inType->baseType != NULL);
				TEiExtract_WriteSwapCodes_Recursive(
					&curSwapCodePtr,
					inType->baseType);
				
				*curSwapCodePtr++ = TMcSwapCode_EndArray;
				
				value16 -= 255;
			}
			break;
		
		case TEcTypeKind_8Byte:
			*curSwapCodePtr++ = TMcSwapCode_8Byte;
			break;
		
		case TEcTypeKind_Enum:
		case TEcTypeKind_4Byte:
			*curSwapCodePtr++ = TMcSwapCode_4Byte;
			break;
			
		case TEcTypeKind_2Byte:
			*curSwapCodePtr++ = TMcSwapCode_2Byte;
			break;
			
		case TEcTypeKind_1Byte:
			*curSwapCodePtr++ = TMcSwapCode_1Byte;
			break;
		
		case TEcTypeKind_SeparateIndex:
			*curSwapCodePtr++ = TMcSwapCode_SeparateIndex;
			break;

		default:
			UUmAssert(!"Illegal case");
	}
	
	*ioSwapCodePtr = curSwapCodePtr;
}

static void
TEiExtract_WriteOutSwapCodes(
	BFtFile*	inFile,
	UUtUns32	inTag,
	UUtUns8*	inSwapCode,
	UUtUns32	inLength)
{
	UUtUns32	itr;
	UUtUns8*	p = inSwapCode;
	
	BFrFile_Printf(inFile, "static UUtUns8 gSwapCodes_%c%c%c%c[%d] ="UUmNL,
		(inTag >> 24) & 0xFF,
		(inTag >> 16) & 0xFF,
		(inTag >> 8) & 0xFF,
		(inTag >> 0) & 0xFF,
		inLength);
	BFrFile_Printf(inFile, "{");
	
	for(itr = 0, p = inSwapCode; itr < inLength; itr++, p++)
	{
		if(itr % 8 == 0) BFrFile_Printf(inFile, UUmNL"\t");
		BFrFile_Printf(inFile, "0x%02x", *p);
		
		if(itr != inLength - 1) BFrFile_Printf(inFile, ", ");
	}
	BFrFile_Printf(inFile, UUmNL);
	BFrFile_Printf(inFile, "};"UUmNL);
}

static void 
TEiExtract_DumpFile(
	void)
{
	TEtType		*curSymbol;
	FILE*		infoFile;
	
	infoFile = fopen("templateInfo.txt", "wb");
	
	curSymbol = TEgSymbolList;
	while(curSymbol)
	{
		if(curSymbol->kind == TEcTypeKind_Template)
		{
			fprintf(infoFile, "template: %s"UUmNL, curSymbol->name);
			fprintf(infoFile, "\tTag: %c%c%c%c"UUmNL, 
				(curSymbol->u.templateInfo.templateTag >> 24) & 0xFF,
				(curSymbol->u.templateInfo.templateTag >> 16) & 0xFF,
				(curSymbol->u.templateInfo.templateTag >> 8) & 0xFF,
				(curSymbol->u.templateInfo.templateTag >> 0) & 0xFF);
			fprintf(infoFile, "\tname: %s"UUmNL, curSymbol->u.templateInfo.templateName);
			fprintf(infoFile, "\tsizeof: %d"UUmNL, curSymbol->sizeofSize);
			fprintf(infoFile, "\tvarElemSize: %d"UUmNL, curSymbol->u.templateInfo.varElemSize);
			fprintf(infoFile, "\tChecksum: "UUmFS_UUtUns64  UUmNL, curSymbol->u.templateInfo.checksum);
			fprintf(infoFile, "*******************************************"UUmNL);
		}
		else if(curSymbol->kind == TEcTypeKind_Struct)
		{
			fprintf(infoFile, "struct: %s"UUmNL, curSymbol->name);
			fprintf(infoFile, "\tsizeof: %d"UUmNL, curSymbol->sizeofSize);
			fprintf(infoFile, "*******************************************"UUmNL);
		}
		
		curSymbol = curSymbol->next;
	}
		
	fclose(infoFile);
}

UUtError
TErExtract(
	void)
{
	UUtUns64				totalChecksum;
	UUtUns64				totalChecksumFactor;
	TEtType*				curSymbol;
	TMtTemplateDefinition*	curTemplate;
	UUtUns8*				swapCodePtr;
	UUtUns32				swapCodeSize;
	BFtFile*				cFile;
	UUtError				error;
	BFtFileRef*				cFileRef;
	UUtUns32				count;
	BFtFileRef*				logFileRef;
	BFtFile*				logFile;
	char					dateBuffer[128];
	UUtUns32				length;
	UUtUns32				itr;
	
	// first compute the total template checksum
		totalChecksumFactor = 0;
		totalChecksum = 0;
		
		curSymbol = TEgSymbolList;
		while(curSymbol)
		{
			if(curSymbol->kind == TEcTypeKind_Template)
			{
				totalChecksumFactor += 1;
				totalChecksum += curSymbol->u.templateInfo.checksum * totalChecksumFactor;
			}
			curSymbol = curSymbol->next;
		}
	
	// if our total checksum has not changed then we are done
		if(totalChecksum == TMgTemplateChecksum) return UUcError_None;
	
	// If we are here then we need to output templatechecksum.c
	
	// Write out the dumpfile
	TEiExtract_DumpFile();
			
	// Append the log file
		error = BFrFileRef_MakeFromName("templatechangelog.txt", &logFileRef);
		UUmError_ReturnOnError(error);
		
		error = BFrFile_Open(logFileRef, "w", &logFile);
		UUmError_ReturnOnError(error);
		
		error = BFrFile_GetLength(logFile, &length);
		UUmError_ReturnOnError(error);
		
		error = BFrFile_SetPos(logFile, length);
		UUmError_ReturnOnError(error);
		
		UUrConvertSecsSince1900ToStr(UUrGetSecsSince1900(), dateBuffer);
		BFrFile_Printf(logFile, "********************************************"UUmNL);
		BFrFile_Printf(logFile, "* %s *"UUmNL, dateBuffer);
		BFrFile_Printf(logFile, "********************************************"UUmNL);
		
		BFrFile_Printf(logFile, "* Templates added"UUmNL);
		curSymbol = TEgSymbolList;
		while(curSymbol)
		{
			if(curSymbol->kind == TEcTypeKind_Template)
			{
				curTemplate = TMrUtility_Template_FindDefinition(curSymbol->u.templateInfo.templateTag);
				if(curTemplate == NULL)
				{
					BFrFile_Printf(logFile, "\t%c%c%c%c: %s"UUmNL,
						(curSymbol->u.templateInfo.templateTag >> 24) & 0xFF,
						(curSymbol->u.templateInfo.templateTag >> 16) & 0xFF,
						(curSymbol->u.templateInfo.templateTag >> 8) & 0xFF,
						(curSymbol->u.templateInfo.templateTag >> 0) & 0xFF,
						curSymbol->u.templateInfo.templateName);
				}
			}
			curSymbol = curSymbol->next;
		}
		
		BFrFile_Printf(logFile, "* Templates changed"UUmNL);
		curSymbol = TEgSymbolList;
		while(curSymbol)
		{
			if(curSymbol->kind == TEcTypeKind_Template)
			{
				curTemplate = TMrUtility_Template_FindDefinition(curSymbol->u.templateInfo.templateTag);
				if(curTemplate != NULL && curTemplate->checksum != curSymbol->u.templateInfo.checksum)
				{
					BFrFile_Printf(logFile, "\t%c%c%c%c: %s"UUmNL,
						(curSymbol->u.templateInfo.templateTag >> 24) & 0xFF,
						(curSymbol->u.templateInfo.templateTag >> 16) & 0xFF,
						(curSymbol->u.templateInfo.templateTag >> 8) & 0xFF,
						(curSymbol->u.templateInfo.templateTag >> 0) & 0xFF,
						curSymbol->u.templateInfo.templateName);
				}
			}
			curSymbol = curSymbol->next;
		}

		BFrFile_Printf(logFile, "* Templates removed"UUmNL);
		for(itr = 0, curTemplate = TMgTemplateDefinitionArray;
			itr < TMgNumTemplateDefinitions;
			itr++, curTemplate++)
		{
			curSymbol = TEgSymbolList;
			while(curSymbol)
			{
				if(curSymbol->kind == TEcTypeKind_Template)
				{
					if(curTemplate->tag == curSymbol->u.templateInfo.templateTag) break;
				}
				curSymbol = curSymbol->next;
			}
			
			if(curSymbol == NULL)
			{
				BFrFile_Printf(logFile, "\t%c%c%c%c: %s"UUmNL,
					(curTemplate->tag >> 24) & 0xFF,
					(curTemplate->tag >> 16) & 0xFF,
					(curTemplate->tag >> 8) & 0xFF,
					(curTemplate->tag >> 0) & 0xFF,
					curTemplate->name);
			}
		}
		
		BFrFile_Close(logFile);
		BFrFileRef_Dispose(logFileRef);

	TMgTemplateChecksum = totalChecksum;
	TMgNumTemplateDefinitions = (UUtUns32)totalChecksumFactor;
	
	fprintf(stderr, "*** TEMPLATE CHECKSUM CHANGED\n");
	
	error = BFrFileRef_MakeFromName("templatechecksum.c", &cFileRef);
	UUmError_ReturnOnError(error);
	error = BFrFile_Open(cFileRef, "w", &cFile);
	UUmError_ReturnOnError(error);
	
	BFrFile_Printf(cFile, "#include \"BFW.h\""UUmNL);
	BFrFile_Printf(cFile, "#include \"BFW_TemplateManager.h\""UUmNL);
	BFrFile_Printf(cFile, "#include \"BFW_TM_Private.h\""UUmNL);
	BFrFile_Printf(cFile, "UUtUns64 TMgTemplateChecksum = "UUmFS_UUtUns64 ";" UUmNL, totalChecksum);
	BFrFile_Printf(cFile, "UUtUns32 TMgNumTemplateDefinitions = %d;" UUmNL, TMgNumTemplateDefinitions);
	BFrFile_Printf(cFile, "TMtTemplateDefinition* TMgTemplateDefinitionArray = NULL;" UUmNL);

	// first build the new template data structures
		TMgTemplateDefinitionArray = TMgTemplateDefinitionsAlloced = UUrMemory_Block_New(TMgNumTemplateDefinitions * sizeof(TMtTemplateDefinition));
		UUmError_ReturnOnNull(TMgTemplateDefinitionArray);
		
		TMgTemplateNameAlloced = UUrMemory_String_New(30 * 1024);
		UUmError_ReturnOnNull(TMgTemplateNameAlloced);
		
		curTemplate = TMgTemplateDefinitionArray;
		curSymbol = TEgSymbolList;
		while(curSymbol)
		{
			if(curSymbol->kind == TEcTypeKind_Template)
			{
				curTemplate->checksum = curSymbol->u.templateInfo.checksum;
				curTemplate->tag = curSymbol->u.templateInfo.templateTag;
				curTemplate->name = UUrMemory_String_GetStr(TMgTemplateNameAlloced, curSymbol->u.templateInfo.templateName);
				UUmError_ReturnOnNull(curTemplate->name);
				
				curTemplate->flags = curSymbol->u.templateInfo.flags;
				curTemplate->size = curSymbol->sizeofSize;
				curTemplate->varArrayElemSize = curSymbol->u.templateInfo.varElemSize;
				curTemplate->magicCookie = TMcMagicCookie;
				curTemplate->timer = NULL;
				
				swapCodeSize = TEiExtract_ComputeSwapCodeSize(curSymbol) + 1;
				
				curTemplate->swapCodes = swapCodePtr = UUrMemory_Block_New(swapCodeSize);
				
				TEiExtract_WriteSwapCodes_Recursive(&swapCodePtr, curSymbol);
				
				*swapCodePtr = TMcSwapCode_EndArray;
				
				TEiExtract_WriteOutSwapCodes(
					cFile,
					curTemplate->tag,
					curTemplate->swapCodes,
					swapCodeSize);
				
				curTemplate++;
			}
			curSymbol = curSymbol->next;
		}
		
		BFrFile_Printf(cFile, "static TMtTemplateDefinition TMgTemplateDefinitionArray_Mem[%d] ="UUmNL, TMgNumTemplateDefinitions);
		BFrFile_Printf(cFile, "{"UUmNL);
		
		curSymbol = TEgSymbolList;
		curTemplate = TMgTemplateDefinitionArray;
		count = 0;
		while(curSymbol)
		{
			if(curSymbol->kind == TEcTypeKind_Template)
			{
				BFrFile_Printf(cFile, "\t// #%d"UUmNL, count);
				BFrFile_Printf(cFile, "\t{"UUmNL);
				
				BFrFile_Printf(cFile, "\t\t" UUmFS_UUtUns64 "," UUmNL, curTemplate->checksum);
				BFrFile_Printf(cFile, "\t\t0x%x," UUmNL, curTemplate->tag);
				BFrFile_Printf(cFile, "\t\t\"%s\","UUmNL, curTemplate->name);
				BFrFile_Printf(cFile, "\t\tNULL," UUmNL);
				BFrFile_Printf(cFile, "\t\t(TMtTemplateFlags)0x%x," UUmNL, curTemplate->flags);
				BFrFile_Printf(cFile, "\t\t0x%x," UUmNL, curTemplate->size);
				BFrFile_Printf(cFile, "\t\t0x%x," UUmNL, curTemplate->varArrayElemSize);
				BFrFile_Printf(cFile, "\t\tNULL," UUmNL);
				BFrFile_Printf(cFile, "\t\t0x%x," UUmNL, curTemplate->magicCookie);
				BFrFile_Printf(cFile, "\t\tNULL," UUmNL);
				
				if(count == TMgNumTemplateDefinitions - 1)
				{
					BFrFile_Printf(cFile, "\t}"UUmNL);
				}
				else
				{
					BFrFile_Printf(cFile, "\t},"UUmNL);
				}
				
				curTemplate++;
				count++;
			}
			curSymbol = curSymbol->next;
		}

		BFrFile_Printf(cFile, "};"UUmNL);

		BFrFile_Printf(cFile, "void" UUmNL "TMrTemplate_BuildList(" UUmNL "\tvoid)"UUmNL);
		BFrFile_Printf(cFile, "{"UUmNL);
	
		curSymbol = TEgSymbolList;
		curTemplate = TMgTemplateDefinitionArray;
		count = 0;
		while(curSymbol)
		{
			if(curSymbol->kind == TEcTypeKind_Template)
			{
				BFrFile_Printf(cFile, "\tTMgTemplateDefinitionArray_Mem[%d].swapCodes = gSwapCodes_%c%c%c%c;"UUmNL,
					count,
					(curTemplate->tag >> 24) & 0xFF,
					(curTemplate->tag >> 16) & 0xFF,
					(curTemplate->tag >> 8) & 0xFF,
					(curTemplate->tag >> 0) & 0xFF);
								
				curTemplate++;
				count++;
			}
			curSymbol = curSymbol->next;
		}
	
		BFrFile_Printf(cFile, "\tTMgTemplateDefinitionArray = TMgTemplateDefinitionArray_Mem;"UUmNL);
	
		BFrFile_Printf(cFile, "}"UUmNL);
	
	BFrFile_SetEOF(cFile);
	BFrFile_Close(cFile);
	BFrFileRef_Dispose(cFileRef);

	return UUcError_None;
}
