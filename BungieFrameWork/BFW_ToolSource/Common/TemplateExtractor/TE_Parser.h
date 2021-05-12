/*
	FILE:	TE_Parser.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: June 22, 1997
	
	PURPOSE: 
	
	Copyright 1997
		
*/
#ifndef TE_PARSER_H
#define TE_PARSER_H

#include "BFW.h"
#include "BFW_FileManager.h"

void TErParser_Initialize(
	void);

void TErParser_Terminate(
	void);
	
void TErParser_ProcessFile(
	UUtUns32	inFileLength,
	char		*inFileData);

#endif /* TE_PARSER_H */
