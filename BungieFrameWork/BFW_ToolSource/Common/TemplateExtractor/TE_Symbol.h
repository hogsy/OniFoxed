/*
	FILE:	TE_Symbol.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: June 18, 1997
	
	PURPOSE: 
	
	Copyright 1997
		
*/
#ifndef TE_SYMBOL_H
#define TE_SYMBOL_H

void
TErSymbolTable_Initialize(
	void);

void
TErSymbolTable_Terminate(
	void);

void
TErSymbolTable_StartStruct(
	void);

void
TErSymbolTable_EndStruct(
	void);

void
TErSymbolTable_StartTemplate(
	void);

void
TErSymbolTable_EndTemplate(
	void);

void
TErSymbolTable_AddTag(
	UUtUns32	inTag);

void
TErSymbolTable_AddToolName(
	void);

void
TErSymbolTable_AddID(
	void);

void
TErSymbolTable_StartField(
	void);
	
void
TErSymbolTable_EndField(
	void);

void
TErSymbolTable_Field_VarIndex(
	void);
	
void
TErSymbolTable_Field_VarArray(
	void);

void
TErSymbolTable_Field_TemplateRef(
	void);

void
TErSymbolTable_Field_RawRef(
	void);
	
void
TErSymbolTable_Field_SeparateIndex(
	void);
	
void
TErSymbolTable_Field_Pad(
	void);

void
TErSymbolTable_Field_Preamble(
	void);

void
TErSymbolTable_Field_Parallel_FieldName(
	void);

void
TErSymbolTable_Field_AddID(
	void);

void
TErSymbolTable_Field_8Byte(
	void);

void
TErSymbolTable_Field_4Byte(
	void);
	
void
TErSymbolTable_Field_2Byte(
	void);

void
TErSymbolTable_Field_1Byte(
	void);

void
TErSymbolTable_Field_TypeName(
	void);

void
TErSymbolTable_Field_Star(
	void);

void
TErSymbolTable_Field_Array(
	void);

void
TErSymbolTable_StartEnum(
	void);

void
TErSymbolTable_EndEnum(
	void);

void
TErSymbolTable_FinishUp(
	void);
	
void
TErSymbolTable_Initialize(
	void);
	
void
TErSymbolTable_Terminate(
	void);
	
#endif /* TE_SYMBOL_H */
