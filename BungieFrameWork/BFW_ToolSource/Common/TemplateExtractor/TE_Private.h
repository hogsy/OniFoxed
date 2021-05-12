/*
	FILE:	TE_Private.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: June 20, 1997
	
	PURPOSE: 
	
	Copyright 1997
		
*/
#ifndef TE_PRIVATE_H
#define TE_PRIVATE_H

#include "BFW_TemplateManager.h"

#define TEcMsgBufferSize	(3 * 1024)

typedef enum TEtType_Kind
{
	TEcTypeKind_Template,
	TEcTypeKind_Struct,
	TEcTypeKind_Enum,
	TEcTypeKind_Field,
	TEcTypeKind_TemplatePtr,
	TEcTypeKind_RawPtr,
	TEcTypeKind_Array,
	TEcTypeKind_8Byte,
	TEcTypeKind_4Byte,
	TEcTypeKind_2Byte,
	TEcTypeKind_1Byte,
	TEcTypeKind_SeparateIndex
	
} TEtType_Kind;

typedef enum TEtField_Type
{
	TEcFieldType_Regular,
	TEcFieldType_VarIndex,
	TEcFieldType_VarArray
	//TEcFieldType_TemplateReference
	
} TEtField_Type;

typedef struct TEtType	TEtType;

typedef struct TEtType_Template
{
	UUtUns32			templateTag;
	char*				templateName;
	UUtUns32			persistentSize;	// Size of the part that is saved to disk
	UUtUns32			varElemSize;	// Size of the variable length array element
	TEtType*			varArrayType;
	UUtUns32			varArrayStartOffset;
	TMtTemplateFlags	flags;
	UUtUns64			checksum;
	
} TEtType_Template;

typedef struct TEtType_Array
{
	UUtUns32		arrayLength;

} TEtType_Array;

typedef struct TEtType_Field
{
	TEtField_Type	fieldType;
	UUtUns32		fieldOffset;
	UUtBool			extraVarField;
	
} TEtType_Field;

struct TEtType
{
	TEtType_Kind	kind;
	char*			name;
	UUtBool			isLeaf;
	UUtBool			error;
	UUtBool			usedInArray;	// true if this type is array'd - it affects how we pad it
	UUtBool			hasBeenPadded;	// true if run through padding correction
	UUtBool			isPad;
	
	union
	{
		TEtType_Template	templateInfo;
		TEtType_Array		arrayInfo;
		TEtType_Field		fieldInfo;
	} u;	
	
	UUtUns32		sizeofSize;		// Total size to match C sizeof 
	UUtUns32		alignmentRequirement;	// 1 -> byte alignment, 2 -> short alignment, 4 -> long alignment, 8 -> 8byte alignment
	
	TEtType*		baseType;
	TEtType*		next;
	
	char*			fileName;
	UUtUns32		line;
	
};

extern char		*TEgCurInputFileName;
extern UUtInt32	TEgCurInputFileLine;

extern char		TEgLexem[512];

extern UUtBool		TEgError;

extern FILE*		TEgErrorFile;

extern UUtUns64		gTemplateChecksum;

extern TEtType*		TEgSymbolList;

#endif /* TE_PRIVATE_H */
