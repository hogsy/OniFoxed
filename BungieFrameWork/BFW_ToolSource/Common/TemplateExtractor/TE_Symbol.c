/*
	FILE:	TE_Symbol.c

	AUTHOR:	Brent H. Pease

	CREATED: June 18, 1997

	PURPOSE:

	Copyright 1997

*/

#include <stdio.h>
#include <string.h>

#include "BFW.h"

#include "BFW_TM_Private.h"
#include "Imp_Common.h"
#include "TE_Private.h"
#include "TE_Symbol.h"

#define TEcStringArraySize	(64 * 1024)

char*			TEgStringArray		= NULL;
UUtMemory_Pool*	TEgMemoryPool		= NULL;
TEtType*		TEgSymbolList		= NULL;

TEtType*		TEgPadType			= NULL;
TEtType*		TEg8ByteType		= NULL;
TEtType*		TEg4ByteType		= NULL;
TEtType*		TEg2ByteType		= NULL;
TEtType*		TEg1ByteType		= NULL;
TEtType*		TEgRawType			= NULL;
TEtType*		TEgSeparateType		= NULL;


TEtType*		TEgCurType			= NULL;
TEtType*		TEgCurField			= NULL;
TEtType*		TEgVarArrayField	= NULL;

static void
TEiReportSemanticErrorAndDie(
	char *msg)
{

	if(TEgErrorFile != NULL)
	{
		Imp_PrintWarning(
			"Symantic Error: File: %s, Line %d: Msg: %s"UUmNL,
			TEgCurInputFileName,
			TEgCurInputFileLine,
			msg);
	}

	TEgError = UUcTrue;
}

static char *
TEiSymbolTable_NewString(
	char	*inStr)
{
	UUtInt32	length;
	char		*p;

	p = TEgStringArray;

	while(1)
	{
		length = *p++;

		if(length == 0)
		{
			break;
		}

		if(!strcmp(p, inStr))
		{
			return p;
		}

		p += length + 1;
	}

	p--;

	length = strlen(inStr);

	if((p - TEgStringArray) + length > TEcStringArraySize)
	{
		TEiReportSemanticErrorAndDie("Ran out of space in string array");
	}

	*p++ = (char)length;
	strcpy(p, inStr);
	*(p + length) = 0;
	*(p + length + 1) = 0;

	return p;
}

static TEtType *
TEiSymbolTable_NewType(
	TEtType_Kind	inTypeKind)
{
	TEtType	*newType;

	newType = UUrMemory_Pool_Block_New(TEgMemoryPool, sizeof(TEtType));
	if(newType == NULL)
	{
		return NULL;
	}

	newType->kind			= inTypeKind;
	newType->name			= NULL;
	newType->baseType		= NULL;
	newType->next			= NULL;
	newType->sizeofSize		= 0;
	newType->error			= UUcFalse;
	newType->usedInArray	= UUcFalse;
	newType->hasBeenPadded	= UUcFalse;
	newType->isPad			= UUcFalse;

	newType->alignmentRequirement = UUcMaxUns32;

	if(TEgCurInputFileName == NULL)
	{
		newType->fileName = "implicitly defined";
	}
	else
	{
		newType->fileName = TEiSymbolTable_NewString(TEgCurInputFileName);
	}
	newType->line = TEgCurInputFileLine;

	switch(inTypeKind)
	{
		case TEcTypeKind_Template:
			newType->u.templateInfo.templateTag = 0;
			newType->u.templateInfo.templateName = NULL;
			newType->u.templateInfo.persistentSize = 0;
			newType->u.templateInfo.varElemSize = 0;
			newType->u.templateInfo.flags = TMcTemplateFlag_None;
			newType->isLeaf		= (UUtBool)0x03;
			break;

		case TEcTypeKind_Struct:
			newType->isLeaf		= (UUtBool)0x03;
			break;

		case TEcTypeKind_Enum:
			newType->sizeofSize = newType->alignmentRequirement = 4;
			newType->isLeaf		= UUcTrue;
			break;

		case TEcTypeKind_Field:
			newType->u.fieldInfo.fieldType = (TEtField_Type)-1;
			newType->u.fieldInfo.fieldOffset = 0;
			newType->isLeaf		= (UUtBool)0x03;
			break;

		case TEcTypeKind_TemplatePtr:
			newType->sizeofSize = newType->alignmentRequirement = 4;
			newType->isLeaf		= UUcFalse;
			break;

		case TEcTypeKind_RawPtr:
			newType->sizeofSize = newType->alignmentRequirement = 4;
			newType->isLeaf		= UUcTrue;
			break;

		case TEcTypeKind_SeparateIndex:
			newType->sizeofSize = newType->alignmentRequirement = 4;
			newType->isLeaf		= UUcTrue;
			break;

		case TEcTypeKind_Array:
			newType->u.arrayInfo.arrayLength = 0;
			newType->isLeaf		= (UUtBool)0x03;
			break;

		case TEcTypeKind_8Byte:
			newType->sizeofSize = newType->alignmentRequirement = 8;
			newType->isLeaf		= UUcTrue;
			break;

		case TEcTypeKind_4Byte:
			newType->sizeofSize = newType->alignmentRequirement = 4;
			newType->isLeaf		= UUcTrue;
			break;

		case TEcTypeKind_2Byte:
			newType->sizeofSize = newType->alignmentRequirement = 2;
			newType->isLeaf		= UUcTrue;
			break;

		case TEcTypeKind_1Byte:
			newType->sizeofSize = newType->alignmentRequirement = 1;
			newType->isLeaf		= UUcTrue;
			break;

		default:
			UUmAssert(!"Unknown type");
	}

	return newType;
}

static void
TEiSymbolTable_AddType(
	TEtType*	inType)
{
	TEtType	*curType;
	TEtType	*prevType;

	curType = TEgSymbolList;
	prevType = NULL;

	while(curType)
	{
		if(strcmp(curType->name, inType->name) > 0) break;

		prevType = curType;
		curType = curType->next;
	}

	if(prevType == NULL)
	{
		inType->next		= TEgSymbolList;
		TEgSymbolList		= inType;
	}
	else
	{
		inType->next = prevType->next;
		prevType->next = inType;
	}

}

static TEtType *
TEiSymbolTable_FindField(
	char 	*inName)
{
	TEtType	*curType;

	curType = TEgCurType->baseType;

	while(curType)
	{
		if(curType->name && !strcmp(curType->name, inName))
		{
			return curType;
		}

		curType = curType->next;
	}

	return NULL;
}

static TEtType *
TEiSymbolTable_Find(
	char 	*inName)
{
	TEtType	*curType;

	curType = TEgSymbolList;

	while(curType)
	{
		if(curType->name && !strcmp(curType->name, inName))
		{
			return curType;
		}

		curType = curType->next;
	}

	return NULL;
}

void
TErSymbolTable_StartStruct(
	void)
{
	UUmAssert(TEgCurType == NULL);
	TEgCurType = TEiSymbolTable_NewType(TEcTypeKind_Struct);
	UUmAssert(TEgCurType != NULL);
}

void
TErSymbolTable_EndStruct(
	void)
{
	UUmAssert(TEgCurType != NULL);

	TEgCurType = NULL;
}

void
TErSymbolTable_StartTemplate(
	void)
{
	UUmAssert(TEgCurType == NULL);
	TEgCurType = TEiSymbolTable_NewType(TEcTypeKind_Template);
	UUmAssert(TEgCurType != NULL);

	TErSymbolTable_StartField();
	TErSymbolTable_Field_Preamble();
	TErSymbolTable_EndField();
	TErSymbolTable_StartField();
	TErSymbolTable_Field_Preamble();
	TErSymbolTable_EndField();
}

void
TErSymbolTable_EndTemplate(
	void)
{
	UUmAssert(TEgVarArrayField!= NULL);
	UUmAssert(TEgVarArrayField->baseType != NULL);

	TEgCurType->u.templateInfo.varArrayType = TEgVarArrayField->baseType;

	TEgCurType = NULL;
}

void
TErSymbolTable_StartEnum(
	void)
{
	UUmAssert(TEgCurType == NULL);
	TEgCurType = TEiSymbolTable_NewType(TEcTypeKind_Enum);
	UUmAssert(TEgCurType != NULL);

	TEgCurType->baseType = TEg4ByteType;
}

void
TErSymbolTable_EndEnum(
	void)
{
	TEgCurType = NULL;
}

void
TErSymbolTable_AddTag(
	UUtUns32	inTag)
{
	TEtType*	curSymbol;

	UUmAssert(TEgCurType->kind == TEcTypeKind_Template);

	TEgCurType->u.templateInfo.templateTag = inTag;

	for(curSymbol = TEgSymbolList; curSymbol; curSymbol = curSymbol->next)
	{
		if(curSymbol->kind == TEcTypeKind_Template)
		{
			if(curSymbol->u.templateInfo.templateTag == inTag)
			{
				TEiReportSemanticErrorAndDie("Duplicate tag.");
				fprintf(
					TEgErrorFile,
					"Tag: %s, name: %s"UUmNL,
					UUrTag2Str(inTag),
					curSymbol->u.templateInfo.templateName);
			}
		}
	}
}

void
TErSymbolTable_AddToolName(
	void)
{
	UUmAssert(TEgCurType->kind == TEcTypeKind_Template);

	TEgCurType->u.templateInfo.templateName = TEiSymbolTable_NewString(TEgLexem);
}

void
TErSymbolTable_AddID(
	void)
{
	TEgCurType->name = TEiSymbolTable_NewString(TEgLexem);

	TEiSymbolTable_AddType(TEgCurType);
}

void
TErSymbolTable_StartField(
	void)
{
	TEtType	*curSymbol, *prevSymbol;

	UUmAssert(TEgCurType->kind == TEcTypeKind_Template || TEgCurType->kind == TEcTypeKind_Struct);
	UUmAssert(TEgCurField == NULL);

	TEgCurField = TEiSymbolTable_NewType(TEcTypeKind_Field);
	UUmAssert(TEgCurField != NULL);

	prevSymbol = NULL;
	curSymbol = TEgCurType->baseType;
	while(curSymbol)
	{
		prevSymbol = curSymbol;
		curSymbol = curSymbol->next;
	}

	if(prevSymbol == NULL)
	{
		TEgCurType->baseType = TEgCurField;
	}
	else
	{
		prevSymbol->next = TEgCurField;
	}

	TEgCurField->u.fieldInfo.fieldType = TEcFieldType_Regular;
}

void
TErSymbolTable_EndField(
	void)
{

	TEgCurField = NULL;
}

void
TErSymbolTable_Field_VarIndex(
	void)
{
	if(TEgCurField->u.fieldInfo.fieldType != TEcFieldType_Regular)
	{
		TEiReportSemanticErrorAndDie("Illegal field");
	}

	TEgCurField->u.fieldInfo.fieldType = TEcFieldType_VarIndex;
}

void
TErSymbolTable_Field_VarArray(
	void)
{
	if(TEgCurField->u.fieldInfo.fieldType != TEcFieldType_Regular)
	{
		TEiReportSemanticErrorAndDie("Illegal field");
	}

	TEgCurField->u.fieldInfo.fieldType = TEcFieldType_VarArray;

	TEgVarArrayField = TEgCurField;
}

void
TErSymbolTable_Field_TemplateRef(
	void)
{
	if(TEgCurField->u.fieldInfo.fieldType != TEcFieldType_Regular)
	{
		TEiReportSemanticErrorAndDie("Illegal field");
	}

	TEgCurField->baseType = TEiSymbolTable_NewType(TEcTypeKind_TemplatePtr);
}

void
TErSymbolTable_Field_RawRef(
	void)
{
	TEgCurField->baseType = TEg4ByteType;
}

void
TErSymbolTable_Field_SeparateIndex(
	void)
{
	TEgCurField->baseType = TEg4ByteType;
}

void
TErSymbolTable_Field_Pad(
	void)
{
	TEgCurField->baseType = TEgPadType;
	TEgCurField->isPad = UUcTrue;
}

void
TErSymbolTable_Field_Preamble(
	void)
{
	if(TEgCurField->u.fieldInfo.fieldType != TEcFieldType_Regular)
	{
		TEiReportSemanticErrorAndDie("Illegal field");
	}

	TEgCurField->baseType = TEiSymbolTable_NewType(TEcTypeKind_4Byte);
	UUmAssert(TEgCurField->baseType != NULL);

	if(TEgCurType->kind != TEcTypeKind_Template)
	{
		TEiReportSemanticErrorAndDie("preamble can only be within templates");
	}
}

void
TErSymbolTable_Field_AddID(
	void)
{
	TEgCurField->name = TEiSymbolTable_NewString(TEgLexem);
}

void
TErSymbolTable_Field_8Byte(
	void)
{
	TEgCurField->baseType = TEg8ByteType;
}


void
TErSymbolTable_Field_4Byte(
	void)
{
	TEgCurField->baseType = TEg4ByteType;
}

void
TErSymbolTable_Field_2Byte(
	void)
{
	TEgCurField->baseType = TEg2ByteType;
}

void
TErSymbolTable_Field_1Byte(
	void)
{
	TEgCurField->baseType = TEg1ByteType;
}

void
TErSymbolTable_Field_TypeName(
	void)
{
	UUmAssert(TEgCurField->baseType == NULL);

	TEgCurField->baseType = TEiSymbolTable_Find(TEgLexem);
	if(TEgCurField->baseType == NULL)
	{
		TEiReportSemanticErrorAndDie("Could not find specified type name");
	}
}

void
TErSymbolTable_Field_Star(
	void)
{
	TEtType*	newType;

	if(TEgCurField->baseType == NULL ||
		TEgCurField->baseType->kind != TEcTypeKind_Template)
	{
		TEiReportSemanticErrorAndDie("can only have pointers to templates");
	}

	newType = TEiSymbolTable_NewType(TEcTypeKind_TemplatePtr);
	newType->baseType = TEgCurField->baseType;
	TEgCurField->baseType = newType;
}

void
TErSymbolTable_Field_Array(
	void)
{
	TEtType*	newType;

	UUtUns32	length;

	sscanf(TEgLexem, "%d", &length);

	if(TEgCurField->u.fieldInfo.fieldType == TEcFieldType_VarArray)
	{
		if(length == 2)
		{
			TEgCurField->u.fieldInfo.extraVarField = UUcTrue;
			return;
		}
		TEgCurField->u.fieldInfo.extraVarField = UUcFalse;
		if(length == 1) return;

		TEiReportSemanticErrorAndDie("The length of a var length array must be one or 2");

		return;
	}

	if(TEgCurField->baseType == NULL || TEgCurField->baseType->kind == TEcTypeKind_Template)
	{
		TEiReportSemanticErrorAndDie("Illegal array base type");
	}

	UUmAssert(TEgCurField->baseType != NULL);

	if(TEgCurField->baseType->kind == TEcTypeKind_Array)
	{
		TEgCurField->baseType->u.arrayInfo.arrayLength *= length;
	}
	else
	{
		newType = TEiSymbolTable_NewType(TEcTypeKind_Array);
		newType->u.arrayInfo.arrayLength = length;

		newType->baseType = TEgCurField->baseType;
		TEgCurField->baseType = newType;

		TEgCurField->baseType->usedInArray = UUcTrue;
	}
}

void
TErSymbolTable_Initialize(
	void)
{

	TEgStringArray = UUrMemory_Block_New(TEcStringArraySize);

	if(TEgStringArray == NULL)
	{
		TEiReportSemanticErrorAndDie("Could not allocate string array");
	}
	TEgStringArray[0] = 0;

	TEgMemoryPool =
		UUrMemory_Pool_New(
			10 * 1024,
			UUcPool_Growable);

	UUmAssert(TEgMemoryPool != NULL);

	TEgPadType = TEiSymbolTable_NewType(TEcTypeKind_1Byte);
	TEgPadType->isPad = UUcTrue;

	TEg8ByteType	= TEiSymbolTable_NewType(TEcTypeKind_8Byte);
	TEg4ByteType	= TEiSymbolTable_NewType(TEcTypeKind_4Byte);
	TEg2ByteType	= TEiSymbolTable_NewType(TEcTypeKind_2Byte);
	TEg1ByteType	= TEiSymbolTable_NewType(TEcTypeKind_1Byte);
	TEgRawType		= TEiSymbolTable_NewType(TEcTypeKind_RawPtr);
	TEgSeparateType	= TEiSymbolTable_NewType(TEcTypeKind_SeparateIndex);

	UUmAssert(TEg8ByteType != NULL);
	UUmAssert(TEg4ByteType != NULL);
	UUmAssert(TEg2ByteType != NULL);
	UUmAssert(TEg1ByteType != NULL);
	UUmAssert(TEgRawType != NULL);

	TEgPadType->name		= TEiSymbolTable_NewString("pad");
	TEg8ByteType->name		= TEiSymbolTable_NewString("8byte");
	TEg4ByteType->name		= TEiSymbolTable_NewString("4byte");
	TEg2ByteType->name		= TEiSymbolTable_NewString("2byte");
	TEg1ByteType->name		= TEiSymbolTable_NewString("1byte");
	TEgRawType->name		= TEiSymbolTable_NewString("rawPtr");
	TEgSeparateType->name	= TEiSymbolTable_NewString("separateIndex");
}

void
TErSymbolTable_Terminate(
	void)
{
	UUrMemory_Block_Delete(TEgStringArray);

	UUrMemory_Pool_Delete(TEgMemoryPool);
}

static void
TEiSymbolTable_ChecksumName(
	char*		inName,
	UUtUns64	*outCummulativeChecksum)
{
	UUtUns64	factor, result;
	UUtUns8*	p = (UUtUns8*)inName;

	if(p == NULL)
	{
		return;
	}

	result = 0;
	factor = 1;

	while(*p != 0)
	{
		result += (UUtUns64)(*p++) * factor++;
	}

	*outCummulativeChecksum = result;
}

static void
TEiSymbolTable_ComputeChecksum(
	TEtType*	inType,
	UUtUns64	*ioChecksumFactor,
	UUtUns64	*outCummulativeChecksum)
{
	UUtInt16	value16;
	TEtType		*curType;
	UUtUns64	nameChecksum;

	UUmAssertReadPtr(inType, sizeof(TEtType));
	UUmAssertReadPtr(ioChecksumFactor, sizeof(UUtUns64));
	UUmAssertReadPtr(outCummulativeChecksum, sizeof(UUtUns64));

	if(inType->name != NULL)
	{
		TEiSymbolTable_ChecksumName(inType->name, &nameChecksum);
		*ioChecksumFactor += 1;
		*outCummulativeChecksum += nameChecksum * *ioChecksumFactor;
	}

	switch(inType->kind)
	{
		case TEcTypeKind_Struct:
		case TEcTypeKind_Template:
			curType = inType->baseType;
			while(curType != NULL)
			{
				TEiSymbolTable_ComputeChecksum(
					curType,
					ioChecksumFactor,
					outCummulativeChecksum);
				curType = curType->next;
			}
			break;

		case TEcTypeKind_Field:
			switch(inType->u.fieldInfo.fieldType)
			{
				case TEcFieldType_Regular:
					TEiSymbolTable_ComputeChecksum(
						inType->baseType,
						ioChecksumFactor,
						outCummulativeChecksum);
					break;

				case TEcFieldType_VarIndex:
					*ioChecksumFactor += 1;
					*outCummulativeChecksum += TMcSwapCode_BeginVarArray * *ioChecksumFactor;
					TEiSymbolTable_ComputeChecksum(
						inType->baseType,
						ioChecksumFactor,
						outCummulativeChecksum);
					break;

				case TEcFieldType_VarArray:

					*ioChecksumFactor += 1;
					*outCummulativeChecksum += TMcSwapCode_EndVarArray * *ioChecksumFactor;

					if(inType->u.fieldInfo.extraVarField)
					{
						*outCummulativeChecksum += TMcSwapCode_EndVarArray * *ioChecksumFactor;
					}

					TEiSymbolTable_ComputeChecksum(
						inType->baseType,
						ioChecksumFactor,
						outCummulativeChecksum);
					break;

				default:
					UUmAssert(!"Illegal case");
			}
			break;

		case TEcTypeKind_TemplatePtr:
			*ioChecksumFactor += 1;
			*outCummulativeChecksum += TMcSwapCode_TemplatePtr * *ioChecksumFactor;

			if(inType->baseType != NULL)
			{
				UUmAssert(inType->baseType->kind == TEcTypeKind_Template);
				*ioChecksumFactor += 1;
				*outCummulativeChecksum += inType->baseType->u.templateInfo.templateTag * *ioChecksumFactor;
			}
			break;

		case TEcTypeKind_RawPtr:
			*ioChecksumFactor += 1;
			*outCummulativeChecksum += TMcSwapCode_4Byte * *ioChecksumFactor;
			break;

		case TEcTypeKind_SeparateIndex:
			*ioChecksumFactor += 1;
			*outCummulativeChecksum += TMcSwapCode_4Byte * *ioChecksumFactor;
			break;

		case TEcTypeKind_Array:
			value16 = (short)inType->u.arrayInfo.arrayLength;

			while(value16 > 0)
			{
				*ioChecksumFactor += 1;
				*outCummulativeChecksum += TMcSwapCode_BeginArray * *ioChecksumFactor;

				*ioChecksumFactor += 1;
				*outCummulativeChecksum += inType->u.arrayInfo.arrayLength * *ioChecksumFactor;

				UUmAssert(inType->baseType != NULL);
				TEiSymbolTable_ComputeChecksum(
					inType->baseType,
					ioChecksumFactor,
					outCummulativeChecksum);
				*ioChecksumFactor += 1;
				*outCummulativeChecksum += TMcSwapCode_EndArray * *ioChecksumFactor;

				value16 -= 255;
			}
			break;


		case TEcTypeKind_8Byte:
			*ioChecksumFactor += 1;
			*outCummulativeChecksum += TMcSwapCode_8Byte * *ioChecksumFactor;
			break;

		case TEcTypeKind_Enum:
		case TEcTypeKind_4Byte:
			*ioChecksumFactor += 1;
			*outCummulativeChecksum += TMcSwapCode_4Byte * *ioChecksumFactor;
			break;

		case TEcTypeKind_2Byte:
			*ioChecksumFactor += 1;
			*outCummulativeChecksum += TMcSwapCode_2Byte * *ioChecksumFactor;
			break;

		case TEcTypeKind_1Byte:
			*ioChecksumFactor += 1;
			*outCummulativeChecksum += TMcSwapCode_1Byte * *ioChecksumFactor;
			break;

		default:
			UUmAssert(!"Illegal case");
	}
}

static void
TEiSymbolTable_ProcessAttributes_Recursive(
	TEtType*	inType)
{
	TEtType		*curType;
	UUtUns32	curSizeofSize = 0;
	UUtUns32	curPersistentSize = 0;
	UUtBool		isLeaf;
	UUtBool		isVarArrayLeaf;
	UUtUns64	checksumFactor;
	UUtBool		any1Byte;
	UUtBool		any2Byte;
	UUtBool		any4Byte;
	UUtBool		any8Byte;

	UUmAssert(inType != NULL);

	if(inType->sizeofSize != 0)
	{
		return;
	}

	switch(inType->kind)
	{
		case TEcTypeKind_Template:
			curType = inType->baseType;

			isLeaf = UUcTrue;
			isVarArrayLeaf = UUcTrue;

			checksumFactor = 0;
			inType->u.templateInfo.checksum = 0;

			TEiSymbolTable_ComputeChecksum(
				inType,
				&checksumFactor,
				&inType->u.templateInfo.checksum);

			while(curType != NULL)
			{
				UUmAssert(curType->kind == TEcTypeKind_Field);

				TEiSymbolTable_ProcessAttributes_Recursive(curType);

				UUmAssert(curType->isLeaf == UUcFalse || curType->isLeaf == UUcTrue);

				if(curType->isLeaf == UUcFalse) isLeaf = UUcFalse;

				UUmAssert(curType->baseType != NULL);

				curType->u.fieldInfo.fieldOffset = curSizeofSize;

				switch(curType->u.fieldInfo.fieldType)
				{
					case TEcFieldType_Regular:
						UUmAssert(curType->baseType != NULL);

					case TEcFieldType_VarIndex:
						break;

					case TEcFieldType_VarArray:
						inType->u.templateInfo.varArrayStartOffset = curSizeofSize;

						inType->u.templateInfo.varElemSize = curType->sizeofSize;
						if(curType->u.fieldInfo.extraVarField == UUcTrue)
						{
							curSizeofSize += curType->sizeofSize;
						}
						isVarArrayLeaf = curType->isLeaf;
						break;

					default:
						UUmAssert(!"Illegal state");
				}

				if(curType->u.fieldInfo.fieldType != TEcFieldType_VarArray)
				{
					curSizeofSize += curType->sizeofSize;
				}

				curPersistentSize += curType->sizeofSize;

				curType = curType->next;
			}

			inType->isLeaf = isLeaf;

			inType->sizeofSize = curSizeofSize;

			inType->u.templateInfo.persistentSize = curPersistentSize;
			inType->u.templateInfo.flags = isLeaf ? TMcTemplateFlag_Leaf : TMcTemplateFlag_None;
			if(isVarArrayLeaf) inType->u.templateInfo.flags |= TMcTemplateFlag_VarArrayIsLeaf;
			break;

		case TEcTypeKind_Struct:
			curType = inType->baseType;

			isLeaf = UUcTrue;

			any1Byte = UUcFalse;
			any2Byte = UUcFalse;
			any4Byte = UUcFalse;
			any8Byte = UUcFalse;

			while(curType != NULL)
			{
				UUmAssert(curType->kind == TEcTypeKind_Field);

				TEiSymbolTable_ProcessAttributes_Recursive(curType);

				UUmAssert(curType->isLeaf == UUcFalse || curType->isLeaf == UUcTrue);

				if(curType->isLeaf == UUcFalse) isLeaf = UUcFalse;

				curSizeofSize += curType->sizeofSize;

				if(curType->alignmentRequirement == 1) any1Byte = UUcTrue;
				else if(curType->alignmentRequirement == 2) any2Byte = UUcTrue;
				else if(curType->alignmentRequirement == 4) any4Byte = UUcTrue;
				else if(curType->alignmentRequirement == 8) any8Byte = UUcTrue;
				else UUmAssert(0);

				curType = curType->next;
			}

			if(inType->baseType->alignmentRequirement == 8)
			{
				inType->alignmentRequirement = 8;
			}
			else if(inType->baseType->alignmentRequirement == 4)
			{
				inType->alignmentRequirement = 4;
			}
			else if(inType->baseType->alignmentRequirement == 2)
			{
				if(any4Byte || any8Byte)
				{
					inType->alignmentRequirement = 4;
				}
				else
				{
					inType->alignmentRequirement = 2;
				}
			}
			else
			{
				if(any4Byte || any8Byte)
				{
					inType->alignmentRequirement = 4;
				}
				else if(any2Byte)
				{
					inType->alignmentRequirement = 2;
				}
				else
				{
					inType->alignmentRequirement = 1;
				}
			}

			inType->isLeaf = isLeaf;
			inType->sizeofSize = curSizeofSize;
			break;

		case TEcTypeKind_Field:
			UUmAssert(inType->baseType != NULL);

			TEiSymbolTable_ProcessAttributes_Recursive(inType->baseType);
			UUmAssert(inType->baseType->isLeaf == UUcFalse || inType->baseType->isLeaf == UUcTrue);

			inType->alignmentRequirement = inType->baseType->alignmentRequirement;

			switch(inType->u.fieldInfo.fieldType)
			{
				case TEcFieldType_Regular:
				case TEcFieldType_VarIndex:
				case TEcFieldType_VarArray:
					UUmAssert(inType->baseType != NULL);
					inType->sizeofSize = inType->baseType->sizeofSize;
					inType->isLeaf = inType->baseType->isLeaf;
					break;

				default:
					UUmAssert(!"Illegal state");
			}
			break;

		case TEcTypeKind_Array:
			UUmAssert(inType->baseType != NULL);
			TEiSymbolTable_ProcessAttributes_Recursive(inType->baseType);
			UUmAssert(inType->baseType->isLeaf == UUcFalse || inType->baseType->isLeaf == UUcTrue);
			inType->isLeaf = inType->baseType->isLeaf;
			inType->sizeofSize = inType->baseType->sizeofSize * inType->u.arrayInfo.arrayLength;
			inType->alignmentRequirement = inType->baseType->alignmentRequirement;
			break;

		case TEcTypeKind_SeparateIndex:
		case TEcTypeKind_RawPtr:
		case TEcTypeKind_TemplatePtr:
		case TEcTypeKind_8Byte:
		case TEcTypeKind_4Byte:
		case TEcTypeKind_Enum:
		case TEcTypeKind_2Byte:
		case TEcTypeKind_1Byte:
			break;

		default:
			UUmAssert(!"Illegal state");
	}

	UUmAssert(inType->isLeaf == UUcFalse || inType->isLeaf == UUcTrue);
}

static void
TEiSymbolTable_ProcessAttributes(
	void)
{
	TEtType	*curSymbol;

	curSymbol = TEgSymbolList;

	while(curSymbol != NULL)
	{
		TEiSymbolTable_ProcessAttributes_Recursive(curSymbol);

		curSymbol = curSymbol->next;
	}
}

static void
TEiSymbolTable_Indent(
	FILE		*inFile,
	UUtInt32	inIndentLevel)
{
	UUtInt32	i;

	for(i = inIndentLevel; i-- > 0;)
	{
		fprintf(inFile, "\t");
	}
}

static void
TEiSymbolTable_Dump_Recursive(
	FILE		*inFile,
	UUtUns32	inLevel,
	UUtBool		inPrintDetailed,
	TEtType		*inType)
{
	TEtType		*curType;
	char		*charPtr;

	switch(inType->kind)
	{
		case TEcTypeKind_Template:
			TEiSymbolTable_Indent(inFile, inLevel);
			fprintf(inFile, "Template:%s(%d)"UUmNL, inType->name ? inType->name : "no name", inType->sizeofSize);

			if(!inPrintDetailed) break;

			TEiSymbolTable_Indent(inFile, inLevel+1);
			charPtr = (char *)&inType->u.templateInfo.templateTag;
			fprintf(inFile, "Tag: %c%c%c%c"UUmNL, charPtr[0], charPtr[1], charPtr[2], charPtr[3]);
			TEiSymbolTable_Indent(inFile, inLevel+1);
			fprintf(inFile, "Desc: %s"UUmNL, inType->u.templateInfo.templateName);
			TEiSymbolTable_Indent(inFile, inLevel+1);
			fprintf(inFile, "Persistent Size: %d"UUmNL, inType->u.templateInfo.persistentSize);
			TEiSymbolTable_Indent(inFile, inLevel+1);
			fprintf(inFile, "VarElem Size: %d"UUmNL, inType->u.templateInfo.varElemSize);
			TEiSymbolTable_Indent(inFile, inLevel+1);
			fprintf(inFile, "Fields: "UUmNL);

			curType = inType->baseType;
			while(curType)
			{
				switch(curType->u.fieldInfo.fieldType)
				{
					case TEcFieldType_VarIndex:
						TEiSymbolTable_Indent(inFile, inLevel+2);
						fprintf(inFile, "varindex: %s(%d) ", curType->name, curType->u.fieldInfo.fieldOffset);
						TEiSymbolTable_Dump_Recursive(inFile, inLevel+2, UUcFalse, curType->baseType);
						break;

					case TEcFieldType_Regular:
						TEiSymbolTable_Indent(inFile, inLevel+2);
						fprintf(inFile, "regular: %s(%d) ", curType->name, curType->u.fieldInfo.fieldOffset);
						TEiSymbolTable_Dump_Recursive(inFile, inLevel+2, UUcFalse, curType->baseType);
						break;

					case TEcFieldType_VarArray:
						TEiSymbolTable_Indent(inFile, inLevel+2);
						fprintf(inFile, "vararray: %s(%d) ", curType->name, curType->u.fieldInfo.fieldOffset);
						TEiSymbolTable_Dump_Recursive(inFile, inLevel+2, UUcFalse, curType->baseType);
						break;

					default:
						UUmAssert(!"Illegal state");
				}
				curType = curType->next;
			}
			break;

		case TEcTypeKind_Struct:
			TEiSymbolTable_Indent(inFile, inLevel);
			fprintf(inFile, "Struct:%s(%d)"UUmNL, inType->name ? inType->name : "no name", inType->sizeofSize);

			if(!inPrintDetailed) break;


			curType = inType->baseType;
			while(curType)
			{
				switch(curType->u.fieldInfo.fieldType)
				{
					case TEcFieldType_Regular:
						TEiSymbolTable_Indent(inFile, inLevel+2);
						fprintf(inFile, "regular: %s(%d) ", curType->name, curType->u.fieldInfo.fieldOffset);
						TEiSymbolTable_Dump_Recursive(inFile, inLevel+2, UUcFalse, curType->baseType);
						break;
					default:
						UUmAssert(!"Illegal state");
				}
				curType = curType->next;
			}
			break;

		case TEcTypeKind_TemplatePtr:
			if(inType->baseType == NULL)
			{
				fprintf(inFile, "* void"UUmNL);
			}
			else
			{
				fprintf(inFile, "* %s"UUmNL, inType->baseType->name);
			}
			break;

		case TEcTypeKind_Array:
			fprintf(inFile, "[] ");
			TEiSymbolTable_Dump_Recursive(inFile, inLevel+1, UUcFalse, inType->baseType);
			break;

		case TEcTypeKind_Enum:
			TEiSymbolTable_Indent(inFile, inLevel);
			fprintf(inFile, "Enum:%s(%d)"UUmNL, inType->name ? inType->name : "no name", inType->sizeofSize);
			break;

		case TEcTypeKind_RawPtr:
			TEiSymbolTable_Indent(inFile, inLevel);
			fprintf(inFile, "TEcTypeKind_RawPtr(%d)"UUmNL, inType->sizeofSize);
			break;

		case TEcTypeKind_SeparateIndex:
			TEiSymbolTable_Indent(inFile, inLevel);
			fprintf(inFile, "TEcTypeKind_SeparateIndex(%d)"UUmNL, inType->sizeofSize);
			break;

		case TEcTypeKind_8Byte:
			TEiSymbolTable_Indent(inFile, inLevel);
			fprintf(inFile, "TEcTypeKind_8Byte(%d)"UUmNL, inType->sizeofSize);
			break;

		case TEcTypeKind_4Byte:
			TEiSymbolTable_Indent(inFile, inLevel);
			fprintf(inFile, "TEcTypeKind_4Byte(%d)"UUmNL, inType->sizeofSize);
			break;

		case TEcTypeKind_2Byte:
			TEiSymbolTable_Indent(inFile, inLevel);
			fprintf(inFile, "TEcTypeKind_2Byte(%d)"UUmNL, inType->sizeofSize);
			break;

		case TEcTypeKind_1Byte:
			TEiSymbolTable_Indent(inFile, inLevel);
			fprintf(inFile, "TEcTypeKind_1Byte(%d)"UUmNL, inType->sizeofSize);
			break;

		default:
			UUmAssert(!"Illegal state");
	}
}

static void
TEiSymbolTable_Dump(
	FILE	*inFile)
{
	TEtType	*curSymbol;

	curSymbol = TEgSymbolList;

	while(curSymbol != NULL)
	{
		TEiSymbolTable_Dump_Recursive(inFile, 0, UUcTrue, curSymbol);

		fprintf(inFile, "***"UUmNL);

		curSymbol = curSymbol->next;
	}
}

#if 0
static UUtBool
TEiSymbolTable_CheckAndReportErrors_Recursive(
	TEtType*	inType,
	char*		inMsg,
	UUtUns32	inCurOffset)
{
	UUtBool		errorFound = UUcFalse;
	TEtType		*curType;
	char		msgBuffer[TEcMsgBufferSize];

	UUmAssertReadPtr(inType, sizeof(TEtType));
	UUmAssertReadPtr(inMsg, 1);

	switch(inType->kind)
	{
		case TEcTypeKind_Struct:
			curType = inType->baseType;
			while(curType != NULL)
			{
				sprintf(
					msgBuffer,
					"%s(struct %s, file: %s, line: %d): ",
					inMsg,
					inType->name, inType->fileName, inType->line);
				if(TEiSymbolTable_CheckAndReportErrors_Recursive(
					curType,
					msgBuffer,
					inCurOffset) == UUcTrue)
				{
					errorFound = UUcTrue;
					inType->error = UUcTrue;
				}
				curType = curType->next;
			}
			break;

		case TEcTypeKind_Field:
			switch(inType->u.fieldInfo.fieldType)
			{
				case TEcFieldType_Regular:
				case TEcFieldType_VarIndex:
					sprintf(
						msgBuffer,
						"%s(field %s, file: %s, line: %d): ",
						inMsg,
						inType->name, inType->fileName, inType->line);
					if(TEiSymbolTable_CheckAndReportErrors_Recursive(
						inType->baseType,
						msgBuffer,
						inCurOffset + inType->u.fieldInfo.fieldOffset) == UUcTrue)
					{
						errorFound = UUcTrue;
					}
					break;

				case TEcFieldType_VarArray:
					sprintf(
						msgBuffer,
						"%s(field %s, file: %s, line: %d): ",
						inMsg,
						inType->name, inType->fileName, inType->line);

					if((inType->u.fieldInfo.fieldOffset % 32) != 0)
					{
						fprintf(TEgErrorFile, "%s: vararray field is not cache line aligned, off by %d\n",
							msgBuffer, 32 - (inType->u.fieldInfo.fieldOffset % 32));
						errorFound = UUcTrue;
					}

					if(TEiSymbolTable_CheckAndReportErrors_Recursive(
						inType->baseType,
						msgBuffer,
						inCurOffset + inType->u.fieldInfo.fieldOffset) == UUcTrue)
					{
						errorFound = UUcTrue;
					}

					// Check again for array alignment problems
					if(TEiSymbolTable_CheckAndReportErrors_Recursive(
						inType->baseType,
						msgBuffer,
						inCurOffset + inType->u.fieldInfo.fieldOffset + inType->sizeofSize) == UUcTrue)
					{
						errorFound = UUcTrue;
					}
					break;

				default:
					UUmAssert(!"Illegal case");
			}
			break;

		case TEcTypeKind_RawPtr:
			break;
		case TEcTypeKind_TemplatePtr:
			break;

		case TEcTypeKind_Array:
			if(TEiSymbolTable_CheckAndReportErrors_Recursive(
				inType->baseType,
				inMsg,
				inCurOffset) == UUcTrue)
			{
				errorFound = UUcTrue;
			}
			if(inType->u.arrayInfo.arrayLength > 1)
			{
				// Check again for array alignment problems
				if(TEiSymbolTable_CheckAndReportErrors_Recursive(
					inType->baseType,
					inMsg,
					inCurOffset + inType->baseType->sizeofSize) == UUcTrue)
				{
					errorFound = UUcTrue;
				}
			}
			break;


		case TEcTypeKind_8Byte:
			if(inCurOffset % 8 != 0)
			{
				fprintf(TEgErrorFile, "%s: field is misaligned, should be on a 8 byte boundary, offset is %d\n",
					inMsg, inCurOffset);
				errorFound = UUcTrue;
			}
			break;

		case TEcTypeKind_SeparateIndex:
		case TEcTypeKind_Enum:
		case TEcTypeKind_4Byte:
			if(inCurOffset % 4 != 0)
			{
				fprintf(TEgErrorFile, "%s: field is misaligned, should be on a 4 byte boundary, offset is %d\n",
					inMsg, inCurOffset);
				errorFound = UUcTrue;
			}
			break;

		case TEcTypeKind_2Byte:
			if(inCurOffset % 2 != 0)
			{
				fprintf(TEgErrorFile, "%s: field is misaligned, should be on a 2 byte boundary, offset is %d\n",
					inMsg, inCurOffset);
				errorFound = UUcTrue;
			}
			break;

		case TEcTypeKind_1Byte:
			break;

		case TEcTypeKind_Template:
			fprintf(
				TEgErrorFile,
				"%s: Can't directly include a template struct, must have a pointer to it.\n",
				inMsg);

			errorFound = UUcTrue;
			break;

		default:
			UUmAssert(!"Illegal case");
	}

	return errorFound;
}
#endif

static void
TEiSymbolTable_FixPad(
	TEtType*	inType);

static UUtUns32
TEiSymbolTable_FixPad_Static_Field(
	TEtType*	inField,
	UUtUns32	inCurOffset,
	UUtUns32	*outPadBytesAdded)
{
	UUtUns32	neededPad;
	TEtType*	existingField;

	*outPadBytesAdded = 0;

	if(inField->baseType->kind == TEcTypeKind_Struct && inField->baseType->error == UUcTrue)
	{
		TEiSymbolTable_FixPad(inField->baseType);
	}

	inField->u.fieldInfo.fieldOffset = inCurOffset;

	neededPad = inCurOffset % inField->alignmentRequirement;
	if(neededPad == 0)
	{
		return inField->sizeofSize;
	}

	// need to do some padding to achieve required alignment

	// allocate new memory for the existing field

	existingField = UUrMemory_Pool_Block_New(TEgMemoryPool, sizeof(TEtType));
	if(existingField == NULL)
	{
		UUmAssert(!"Out of memory");
		fprintf(stderr, "out of memory - \n");
		return 0;
	}

	// assign our inField to existingField
	*existingField = *inField;

	// now munge the next field so that inField points to existing field, now everthing should be linked correctly
	inField->next = existingField;

	inField->name = TEiSymbolTable_NewString("addedPad");

	if(neededPad == 4)
	{
		inField->baseType = TEg4ByteType;
	}
	else if(neededPad == 2)
	{
		inField->baseType = TEg2ByteType;
	}
	else if(neededPad == 1)
	{
		inField->baseType = TEg1ByteType;
	}
	else
	{
		fprintf(stderr, "Needed to insert a 3-byte pad, can't (must be 1, 2 or 4)\n");
		return 0;
	}

	*outPadBytesAdded = neededPad;

	// update

	inField->sizeofSize = inField->baseType->sizeofSize;
	inField->alignmentRequirement = inField->baseType->alignmentRequirement;
	inField->u.fieldInfo.fieldOffset = inCurOffset;
	inField->u.fieldInfo.fieldType = TEcFieldType_Regular;
	inField->isPad = UUcTrue;

	return inField->sizeofSize;
}

static void
TEiSymbolTable_FixPad_Static(
	TEtType*	inField,
	UUtUns32	*outPreVarArraySize,
	UUtBool		*outHasVarArray,
	UUtUns32	*outPadBytesAdded,
	UUtUns32	inCurOffset)
{
	UUtUns32	curOffset = inCurOffset;
	TEtType*	curField = inField;
	UUtUns32	fieldSize;
	UUtBool		hasVarArray = UUcFalse;
	UUtUns32	padBytesAdded;
	UUtUns32	totalPadBytesAdded = 0;

	while(curField)
	{
		if(curField->u.fieldInfo.fieldType == TEcFieldType_VarArray)
		{
			hasVarArray = UUcTrue;
			break;
		}

		fieldSize = TEiSymbolTable_FixPad_Static_Field(curField, curOffset, &padBytesAdded);

		totalPadBytesAdded += padBytesAdded;
		curOffset += fieldSize;

		curField = curField->next;
	}

	*outPreVarArraySize = curOffset;
	*outHasVarArray = hasVarArray;
	*outPadBytesAdded = totalPadBytesAdded;

}

static TEtType*
TEiSymbolTable_FixPad_StripOldPad(
	TEtType*	inFirstField)
{
	TEtType*	curField = inFirstField;
	TEtType*	prevField = NULL;
	TEtType*	returnField = NULL;

	if(curField->isPad == UUcTrue) while(curField != NULL && curField->isPad == UUcTrue) curField = curField->next;

	UUmAssert(curField != NULL);

	returnField = curField;

	while(curField != NULL)
	{
		while(curField != NULL && curField->isPad) curField = curField->next;

		if(prevField != NULL) prevField->next = curField;

		if(curField == NULL) break;

		prevField = curField;
		curField = curField->next;
	}

	return returnField;
}

static void
TEiSymbolTable_FixPad_Print_BaseType(
	TEtType*	inType,
	UUtUns32	*outArrayLength,
	UUtBool		inIsPad)
{
	*outArrayLength = 0;

	if(inIsPad)
	{
		fprintf(TEgErrorFile, "tm_pad");

		switch(inType->kind)
		{
			case TEcTypeKind_Array:
				*outArrayLength = inType->u.arrayInfo.arrayLength;
				break;

			case TEcTypeKind_4Byte:
				*outArrayLength = 4;
				break;

			case TEcTypeKind_2Byte:
				*outArrayLength = 2;
				break;

			case TEcTypeKind_1Byte:
				*outArrayLength = 1;
				break;

			default:
				UUmAssert(0);
		}

		return;
	}

	switch(inType->kind)
	{
		case TEcTypeKind_Struct:
			fprintf(TEgErrorFile, "struct %s", inType->name);
			break;

		case TEcTypeKind_Enum:
			fprintf(TEgErrorFile, "enum %s", inType->name);
			break;

		case TEcTypeKind_SeparateIndex:
			fprintf(TEgErrorFile, "separateIndex");
			break;

		case TEcTypeKind_RawPtr:
			fprintf(TEgErrorFile, "Raw");
			break;

		case TEcTypeKind_TemplatePtr:
			if(inType->baseType == NULL)
			{
				fprintf(TEgErrorFile, "void*");
			}
			else
			{
				fprintf(TEgErrorFile, "%s*", inType->baseType->name);
			}
			break;

		case TEcTypeKind_Array:
			TEiSymbolTable_FixPad_Print_BaseType(inType->baseType, outArrayLength, UUcFalse);
			*outArrayLength = inType->u.arrayInfo.arrayLength;
			break;

		case TEcTypeKind_8Byte:
			fprintf(TEgErrorFile, "UUtUns64");
			break;

		case TEcTypeKind_4Byte:
			fprintf(TEgErrorFile, "UUtUns32");
			break;

		case TEcTypeKind_2Byte:
			fprintf(TEgErrorFile, "UUtUns16");
			break;

		case TEcTypeKind_1Byte:
			fprintf(TEgErrorFile, "UUtUns8");
			break;

		default:
			UUmAssert(0);
	}
}

static void
TEiSymbolTable_FixPad_Print(
	TEtType*	inType)
{
	TEtType*	curField;
	UUtUns32	arrayLength;

	if(inType->kind == TEcTypeKind_Template)
	{
		fprintf(TEgErrorFile, "tm_template ");
	}
	else if(inType->kind == TEcTypeKind_Struct)
	{
		fprintf(TEgErrorFile, "tm_struct ");
	}
	else
	{
		UUmAssert(0);
	}

	fprintf(TEgErrorFile, "%s\n{\n", inType->name);


	for(curField = inType->baseType; curField != NULL; curField = curField->next)
	{
		if(curField->name == NULL) continue; // skip over implicit fields

		fprintf(TEgErrorFile, "\t");

		if(curField->u.fieldInfo.fieldType == TEcFieldType_VarIndex)
		{
			fprintf(TEgErrorFile, "tm_varindex ");
		}
		else if(curField->u.fieldInfo.fieldType == TEcFieldType_VarArray)
		{
			fprintf(TEgErrorFile, "tm_vararray ");
		}

		TEiSymbolTable_FixPad_Print_BaseType(curField->baseType, &arrayLength, curField->isPad);

		fprintf(TEgErrorFile, " %s", curField->name);

		if(arrayLength > 0)
		{
			fprintf(TEgErrorFile, "[%d]", arrayLength);
		}

		fprintf(TEgErrorFile, ";\n");
	}


	fprintf(TEgErrorFile, "}\n");
}

static void
TEiSymbolTable_FixPad(
	TEtType*	inType)
{
	UUtUns32	preVarArraySize;
	UUtBool		hasVarArray;
	UUtUns32	initialPad = 0;
	TEtType*	padField;
	TEtType*	lastImpliedField;
	UUtUns32	padBytesAdded;
	UUtUns32	leftOverPad;

	if(inType->hasBeenPadded == UUcTrue) return;

	inType->hasBeenPadded = UUcTrue;

	//UUmAssert(strcmp(inType->name, "AItCharacterSetup") != 0);

	inType->baseType = TEiSymbolTable_FixPad_StripOldPad(inType->baseType);

	// pad out everything up till the var array
	TEiSymbolTable_FixPad_Static(
		inType->baseType,
		&preVarArraySize,
		&hasVarArray,
		&padBytesAdded,
		0);

	if((preVarArraySize & 0x1F) != 0)
	{
		initialPad = 32 - (preVarArraySize & 0x1F);
	}

	if(hasVarArray == UUcTrue && initialPad != 0)
	{

		// initial pad needs to be a multiple of 4
		leftOverPad = initialPad % 4;
		initialPad -= leftOverPad;

		if(initialPad > 0)
		{
			// need to add additional pad to make var array cache line aligned
			padField = TEiSymbolTable_NewType(TEcTypeKind_Field);
			if(padField == NULL)
			{
				UUmAssert(!"Out of memory");
				return;
			}

			padField->baseType = TEiSymbolTable_NewType(TEcTypeKind_Array);
			if(padField == NULL)
			{
				UUmAssert(!"Out of memory");
				return;
			}

			padField->baseType->baseType = TEg1ByteType;
			if(padField == NULL)
			{
				UUmAssert(!"Out of memory");
				return;
			}
			padField->isLeaf = UUcTrue;
			padField->isPad = UUcTrue;
			padField->u.fieldInfo.fieldType = TEcFieldType_Regular;
			padField->name = TEiSymbolTable_NewString("pad");
			padField->alignmentRequirement = 1;

			padField->baseType->isLeaf = UUcTrue;
			padField->baseType->alignmentRequirement = 1;

			padField->sizeofSize = padField->baseType->u.arrayInfo.arrayLength = initialPad;

			lastImpliedField = inType->baseType->next;
			padField->next = lastImpliedField->next;
			lastImpliedField->next = padField;
		}

		if(leftOverPad > 0)
		{
			TEtType*	curField;
			TEtType*	prevField;

			// this pad needs to go before the var index field
			padField = TEiSymbolTable_NewType(TEcTypeKind_Field);
			if(padField == NULL)
			{
				UUmAssert(!"Out of memory");
				return;
			}

			padField->baseType = TEiSymbolTable_NewType(TEcTypeKind_Array);
			if(padField == NULL)
			{
				UUmAssert(!"Out of memory");
				return;
			}

			padField->baseType->baseType = TEg1ByteType;
			if(padField == NULL)
			{
				UUmAssert(!"Out of memory");
				return;
			}
			padField->isLeaf = UUcTrue;
			padField->isPad = UUcTrue;
			padField->u.fieldInfo.fieldType = TEcFieldType_Regular;
			padField->name = TEiSymbolTable_NewString("pad");
			padField->alignmentRequirement = 1;

			padField->baseType->isLeaf = UUcTrue;
			padField->baseType->alignmentRequirement = 1;

			padField->sizeofSize = padField->baseType->u.arrayInfo.arrayLength = leftOverPad;

			curField = inType->baseType;
			while(curField != NULL && curField->u.fieldInfo.fieldType != TEcFieldType_VarIndex)
			{
				prevField = curField;
				curField = curField->next;
			}

			UUmAssert(prevField != NULL);
			UUmAssert(curField != NULL);

			padField->next = curField;
			prevField->next = padField;
		}
	}

	TEiSymbolTable_FixPad_Static(
		inType->baseType,
		&preVarArraySize,
		&hasVarArray,
		&padBytesAdded,
		0);

	if(padBytesAdded != 0)
	{
		fprintf(TEgErrorFile, "Could not properly fix this structure, talk to brent\n");
	}

	TEiSymbolTable_FixPad_Print(inType);
}

static UUtBool
TEiSymbolTable_CheckAndReportErrors(
	void)
{
	TEtType*	curSymbol;
	TEtType*	curField;
	UUtBool		errorFound = UUcFalse;
	char		msgBuffer[TEcMsgBufferSize];

	curSymbol = TEgSymbolList;
	while(curSymbol)
	{
		if(curSymbol->kind == TEcTypeKind_Template || curSymbol->kind == TEcTypeKind_Struct)
		{
			sprintf(
				msgBuffer,
				"(template %s, file: %s, line: %d): ",
				curSymbol->name, curSymbol->fileName, curSymbol->line);

			curField = curSymbol->baseType;
			while(curField)
			{
				if(curField->u.fieldInfo.fieldOffset % curField->alignmentRequirement != 0)
				{
					curSymbol->error = UUcTrue;
					errorFound = UUcTrue;
				}

				if(curField->u.fieldInfo.fieldType == TEcFieldType_VarArray &&
					(curField->u.fieldInfo.fieldOffset % 32 != 0))
				{
					curSymbol->error = UUcTrue;
					errorFound = UUcTrue;
				}

				curField = curField->next;
			}

		}
		curSymbol = curSymbol->next;
	}

	if(errorFound)
	{
		fprintf(TEgErrorFile, "*** Corrected structures\n");
		// print out the correctly padded templates
		curSymbol = TEgSymbolList;
		while(curSymbol)
		{
			if(curSymbol->kind == TEcTypeKind_Template && curSymbol->error == UUcTrue)
			//if(curSymbol->kind == TEcTypeKind_Template || curSymbol->kind == TEcTypeKind_Struct)
			{
				TEiSymbolTable_FixPad(curSymbol);
			}

			curSymbol = curSymbol->next;
		}
	}

	return errorFound;
}

void
TErSymbolTable_FinishUp(
	void)
{
	FILE *outFile;
	UUtBool foundErrors;

	// First compute all attributes
		TEiSymbolTable_ProcessAttributes();

	// Next check for errors
		foundErrors = TEiSymbolTable_CheckAndReportErrors();

	// finally write out the dump file
		outFile = fopen("TE.out", "w");
		UUmAssert(outFile != NULL);

		TEiSymbolTable_Dump(outFile);

		fclose(outFile);

	// set TEgError if we found any errors
		if (foundErrors) {
			TEgError = UUcTrue;
		}
}
