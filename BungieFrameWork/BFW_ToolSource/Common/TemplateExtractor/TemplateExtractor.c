/*
	FILE:	TemplateExtractor.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: June 17, 1997
	
	PURPOSE: 
	
	Copyright 1997
		
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BFW.h"
#include "BFW_FileManager.h"

#include "TemplateExtractor.h"
#include "TE_Private.h"
#include "TE_Parser.h"
#include "TE_Symbol.h"
#include "TE_Extract.h"
#include "Imp_Common.h"

#define TEcInputFileName				"..\\..\\TemplateFileList.txt"
#define TEcTemplateDatFileName			"..\\..\\GameDataFolder\\template.dat"
#define TEcTemplateNameDatFileName		"..\\..\\GameDataFolder\\template.nam"

UUtBool		TEgError = UUcFalse;

FILE*		TEgErrorFile = NULL;

static void
TEiProcessMainInputFile(
	UUtUns32	inInputFileLength,
	char		*inInputFileData)
{
	BFtFileRef	*curInputFileRef;
	BFtFile		*curInputFile;
	
	char		*endPtr;
	char		*startLinePtr;
	char		*curPtr;
	char		c;
	UUtUns32	curInputFileLength;
	char		*curInputFileData;
	
	UUtError	error;
	
	curPtr = inInputFileData;
	endPtr = inInputFileData + inInputFileLength;
	
	fprintf(stderr, "Extracting template files"UUmNL);

	while(curPtr < endPtr)
	{
		startLinePtr = curPtr;
		
		while(1)
		{
			c = *curPtr++;
			if(c == '\n' || c == '\r')
			{
				break;
			}

			if(curPtr >= endPtr) break;
		}
		
		*(curPtr - 1) = 0;
		c = *curPtr;
		if(c == '\n' || c == '\r') curPtr++;

		if ('\0' == startLinePtr[0]) { continue; }	// skip lines with just a newline
		
		error = 
			BFrFileRef_MakeFromName(
				startLinePtr,
				&curInputFileRef);
		if(error != UUcError_None)
		{
			char	msg[256];
			
			sprintf(msg, "could not find file \"%s\""UUmNL, startLinePtr);
			
			UUrError_Report(UUcError_Generic, msg);
		}
		
		error = 
			BFrFile_Open(
				curInputFileRef,
				"r",
				&curInputFile);
		if(error != UUcError_None)
		{
			char	msg[256];
			
			sprintf(msg, "could not open file \"%s\""UUmNL, startLinePtr);
			
			UUrError_Report(UUcError_Generic, msg);
		}
		
		BFrFileRef_Dispose(curInputFileRef);
		
		error = 
			BFrFile_GetLength(
				curInputFile,
				&curInputFileLength);
		if(error != UUcError_None)
		{
			char	msg[256];
			
			sprintf(msg, "could not get file length \"%s\""UUmNL, startLinePtr);
			
			UUrError_Report(UUcError_Generic, msg);
		}
		
		curInputFileData = 
			UUrMemory_Block_New(curInputFileLength + 1);
		if(curInputFileData == NULL)
		{
			char	msg[256];
			
			sprintf(msg, "could not get file length \"%s\""UUmNL, startLinePtr);
			
			UUrError_Report(UUcError_Generic, msg);
		}
		
		error = 
			BFrFile_Read(
				curInputFile,
				curInputFileLength,
				curInputFileData);
		if(error != UUcError_None)
		{
			char	msg[256];
			
			sprintf(msg, "could not read file \"%s\""UUmNL, startLinePtr);
			
			UUrError_Report(UUcError_Generic, msg);
		}
		
		BFrFile_Close(curInputFile);
		
		/* process */
		//fprintf(stderr, "Processing file \"%s\""UUmNL, startLinePtr);
		fprintf(stderr, ".");
		
		TEgCurInputFileLine = 1;
		TEgCurInputFileName = startLinePtr;
		
		TErParser_ProcessFile(curInputFileLength, curInputFileData);
		
		UUrMemory_Block_Delete(curInputFileData);
	}
	fprintf(stderr, UUmNL);
}

UUtError
TErRun(
	void)
{
	UUtError	error;
	
	BFtFileRef	*masterInputFileRef;
	BFtFile		*masterInputFile;
	
	char		*masterInputFileData;
	UUtUns32	masterInputFileLength;
	
	/*
	 * Create error file
	 */
	 	TEgErrorFile = fopen("TE_Errors.txt", "wb");
	 	if(TEgErrorFile == NULL)
	 	{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "Could not create error file");
	 	}
	
	/*
	 * Initialize
	 */
		TErParser_Initialize();
		
		TErSymbolTable_Initialize();	
	
	/*
	 * Open master input file
	 */
		error = 
			BFrFileRef_MakeFromName(
				TEcInputFileName,
				&masterInputFileRef);
				
		if(error != UUcError_None)
		{
			char	msg[256];
			
			sprintf(msg, "could not find file \"%s\""UUmNL, TEcInputFileName);
			
			UUmError_ReturnOnErrorMsg(UUcError_Generic, msg);
		}
		
		error = 
			BFrFile_Open(
				masterInputFileRef,
				"r",
				&masterInputFile);
		if(error != UUcError_None)
		{
			char	msg[256];
			
			sprintf(msg, "could not open file \"%s\""UUmNL, TEcInputFileName);
			
			UUmError_ReturnOnErrorMsg(UUcError_Generic, msg);
		}
		
		BFrFileRef_Dispose(masterInputFileRef);
		
		error = 
			BFrFile_GetLength(
				masterInputFile,
				&masterInputFileLength);
		if(error != UUcError_None)
		{
			char	msg[256];
			
			sprintf(msg, "could not get file length \"%s\""UUmNL, TEcInputFileName);
			
			UUmError_ReturnOnErrorMsg(UUcError_Generic, msg);
		}
		
		masterInputFileData = 
			UUrMemory_Block_New(masterInputFileLength + 1);
		if(masterInputFileData == NULL)
		{
			char	msg[256];
			
			sprintf(msg, "could not get file length \"%s\""UUmNL, TEcInputFileName);
			
			UUmError_ReturnOnErrorMsg(UUcError_Generic, msg);
		}
		
		error = 
			BFrFile_Read(
				masterInputFile,
				masterInputFileLength,
				masterInputFileData);
		if(error != UUcError_None)
		{
			char	msg[256];
			
			sprintf(msg, "could not read file \"%s\""UUmNL, TEcInputFileName);
			
			UUmError_ReturnOnErrorMsg(UUcError_Generic, msg);
		}
		
		BFrFile_Close(masterInputFile);
		
	/*
	 * Process the file
	 */
		TEiProcessMainInputFile(masterInputFileLength, masterInputFileData);
		
		if(TEgError == UUcTrue)
		{
			goto errorExit;
		}
		
		//fprintf(stderr, "Processing attributes..."UUmNL);
		
		TErSymbolTable_FinishUp();

		if(TEgError == UUcTrue)
		{
			goto errorExit;
		}
	
	//
	
	error = TErExtract();
	if(error != UUcError_None) goto errorExit;
	
	//fprintf(stderr, "Writing to disk..."UUmNL);
	
	#if 0
	if(1)
	{
		FILE *outTemplateDatFile;
		FILE *outTemplateNameDatFile;
		
		outTemplateDatFile = UUrFOpen(TEcTemplateDatFileName, "wb");
		if(outTemplateDatFile == NULL)
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "could not open file template.dat");
		}
		
		outTemplateNameDatFile = UUrFOpen(TEcTemplateNameDatFileName, "wb");
		if(outTemplateNameDatFile == NULL)
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "could not open file template.nam");
		}
		
		TErSymbolTable_WriteFile(outTemplateDatFile, outTemplateNameDatFile);
		
		fclose(outTemplateDatFile);
		fclose(outTemplateNameDatFile);
	}
	{
		FILE	*file;
		
		file = UUrFOpen("test.out", "wb");
		TErSymbolTable_Dump(file);
		fclose(file);
	}
	#endif

	#if 0
	{
		FILE 		*outTemplateDatFile;
		char		c;
		
		outTemplateDatFile = UUrFOpen(TEcTemplateDatFileName, "b");
		while(!feof(outTemplateDatFile))
		{
			c = fgetc(outTemplateDatFile);
			printf("%3d %c"UUmNL, c, c);
		}
	}
	#endif
	
	{
		FILE*	file;
		
		file = fopen("templateSwapCodes.txt", "w");
		if(file != NULL)
		{
			TMrSwapCode_DumpAll(file);
		
			fclose(file);
		}
	}
	
	UUrMemory_Block_Delete(masterInputFileData);
	
	TErSymbolTable_Terminate();

	TErParser_Terminate();
	
	if(TEgError == UUcTrue)
	{
		goto errorExit;
	}
	
	fclose(TEgErrorFile);
	
	return UUcError_None;

errorExit:

	fclose(TEgErrorFile);
	
	return UUcError_Generic;
}

