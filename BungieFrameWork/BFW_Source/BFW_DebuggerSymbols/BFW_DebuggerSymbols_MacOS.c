/*
	FILE:	BFW_DebuggerSymbols_MacOS.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: July 12, 1999
	
	PURPOSE: 
	
	Copyright 1999

*/

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_DebuggerSymbols.h"
#include "BFW_Math.h"

#include "DebugDataTypes.h"

DISK_SYMBOL_HEADER_BLOCK_v32*			DSgHeaderBlock = NULL;
FILE_REFERENCE_TABLE_ENTRY_v32*			DSgFRTE;
NAME_TABLE_ENTRY*						DSgNTE;
RESOURCE_TABLE_ENTRY_v32*				DSgRTE;
MODULES_TABLE_ENTRY_v33*				DSgMTE;
FRTE_INDEX_TABLE_ENTRY_v32*				DSgFITE;
CONTAINED_STATEMENTS_TABLE_ENTRY_v32*	DSgCSNTE;

UUtUns32						DSgPageSize;

UUtUns32						DSgMTE_EntriesPerPage;
UUtUns32						DSgMTE_Offset;
UUtUns32						DSgCSNTE_EntriesPerPage;
UUtUns32						DSgCSNTE_Offset;
UUtUns32						DSgFRTE_EntriesPerPage;
UUtUns32						DSgFRTE_Offset;

#define DSmMTE_PointerFromIndex(x) (MODULES_TABLE_ENTRY_v33*)((char*)DSgMTE + x * sizeof(MODULES_TABLE_ENTRY_v33) + (x / DSgMTE_EntriesPerPage) * DSgMTE_Offset)
#define DSmCSNTE_PointerFromIndex(x) (CONTAINED_STATEMENTS_TABLE_ENTRY_v32*)((char*)DSgCSNTE + x * sizeof(CONTAINED_STATEMENTS_TABLE_ENTRY_v32) + (x / DSgCSNTE_EntriesPerPage) * DSgCSNTE_Offset)
#define DSmFRTE_PointerFromIndex(x) (FILE_REFERENCE_TABLE_ENTRY_v32*)((char*)DSgFRTE + x * sizeof(FILE_REFERENCE_TABLE_ENTRY_v32) + (x / DSgFRTE_EntriesPerPage) * DSgFRTE_Offset)

static UUtError
DSiSymFile_Create(
	void)
{
	UUtError	error;
	BFtFileRef*	xsymFileRef;
	BFtFile*	xsymFile;
	UUtUns32	xsymFileSize;
	char*		xsymMemory;

	if(DSgHeaderBlock != NULL) return UUcError_None;
	
	error = BFrFileRef_MakeFromName(UUmDebuggerSymFileName, &xsymFileRef);
	UUmError_ReturnOnError(error);
	
	error = BFrFile_Open(xsymFileRef, "r", &xsymFile);
	UUmError_ReturnOnErrorMsg(error, UUmDebuggerSymFileName);

	error = BFrFile_GetLength(xsymFile, &xsymFileSize);
	UUmError_ReturnOnError(error);

	xsymMemory = malloc(xsymFileSize);
	UUmError_ReturnOnNull(xsymMemory);
	
	error = BFrFile_Read(xsymFile, xsymFileSize, xsymMemory);
	UUmError_ReturnOnError(error);
	
	DSgHeaderBlock = (DISK_SYMBOL_HEADER_BLOCK_v32*)xsymMemory;
	
	DSgPageSize = DSgHeaderBlock->dshb_page_size;
	
	DSgFRTE = (FILE_REFERENCE_TABLE_ENTRY_v32*)(xsymMemory + DSgHeaderBlock->dshb_frte.dti_first_page * DSgPageSize);
	DSgNTE = (NAME_TABLE_ENTRY*)(xsymMemory + DSgHeaderBlock->dshb_nte.dti_first_page * DSgPageSize);
	DSgRTE = (RESOURCE_TABLE_ENTRY_v32*)(xsymMemory + DSgHeaderBlock->dshb_rte.dti_first_page * DSgPageSize);
	DSgMTE = (MODULES_TABLE_ENTRY_v33*)(xsymMemory + DSgHeaderBlock->dshb_mte.dti_first_page * DSgPageSize);
	DSgFITE = (FRTE_INDEX_TABLE_ENTRY_v32*)(xsymMemory + DSgHeaderBlock->dshb_fite.dti_first_page * DSgPageSize);
	DSgCSNTE = (CONTAINED_STATEMENTS_TABLE_ENTRY_v32*)(xsymMemory + DSgHeaderBlock->dshb_csnte.dti_first_page * DSgPageSize);
	
	DSgMTE_EntriesPerPage = DSgPageSize / sizeof(MODULES_TABLE_ENTRY_v33);
	DSgMTE_Offset = DSgPageSize % sizeof(MODULES_TABLE_ENTRY_v33);
	DSgCSNTE_EntriesPerPage = DSgPageSize / sizeof(CONTAINED_STATEMENTS_TABLE_ENTRY_v32);
	DSgCSNTE_Offset = DSgPageSize % sizeof(CONTAINED_STATEMENTS_TABLE_ENTRY_v32);
	DSgFRTE_EntriesPerPage = DSgPageSize / sizeof(FILE_REFERENCE_TABLE_ENTRY_v32);
	DSgFRTE_Offset = DSgPageSize % sizeof(FILE_REFERENCE_TABLE_ENTRY_v32);
	
	BFrFile_Close(xsymFile);
	
	BFrFileRef_Dispose(xsymFileRef);
	
	return UUcError_None;
}

static void
DSiSymFile_Dispose(
	void)
{
	if(DSgHeaderBlock != NULL)
	{
		free(DSgHeaderBlock);
	}
}

static RESOURCE_TABLE_ENTRY_v32*
DSiSymTable_RTE_Get(
	UUtUns32	inIndex)
{
	return DSgRTE + inIndex;
}

static UUtUns32
DSiSymTable_MTE_FindFromRTEAndResourceOffset(
	UUtUns32	inRTEIndex,
	UUtUns32	inResourceOffset)
{
	RESOURCE_TABLE_ENTRY_v32*	targetRTE;
	MODULES_TABLE_ENTRY_v33*	curMTE;
	UUtUns32					curIndex;
	
	targetRTE = DSgRTE + inRTEIndex;
	
	for(curIndex = targetRTE->rte_mte_first;
		curIndex < targetRTE->rte_mte_last;
		curIndex++)
	{	
		curMTE = DSmMTE_PointerFromIndex(curIndex);
		
		UUmAssert(curMTE->mte_rte_index == 1);
		
		if(inResourceOffset >= curMTE->mte_res_offset && 
			inResourceOffset < curMTE->mte_res_offset + curMTE->mte_size)
		{
			return curIndex;
		}
	}
	
	return NULL;
}

static UUtUns32
DSiSymTable_CSNTE_FindFromMTEAndResourceOffset(
	UUtUns32	inMTEIndex,
	UUtUns32	inResourceOffset,
	UUtUns32	*outMTERelativeSourcePos)
{
	MODULES_TABLE_ENTRY_v33*				targetMTE;
	CONTAINED_STATEMENTS_TABLE_ENTRY_v32*	curCSNTE;
	UUtUns32								curIndex;
	UUtUns32								csnteIndex;
	UUtInt32								mteRelativeOffset;
	UUtUns32								offsetDistance;
	UUtUns32								minOffsetDistance = UUcMaxInt32;
	UUtBool									foundChange = UUcFalse;
	UUtUns32								curFilePos = 0;
	UUtUns32								returnFilePos = 0;
	
	targetMTE = DSmMTE_PointerFromIndex(inMTEIndex);
	
	mteRelativeOffset = inResourceOffset - targetMTE->mte_res_offset;
	
	for(curIndex = targetMTE->mte_csnte_idx_1;
		curIndex < targetMTE->mte_csnte_idx_2;
		curIndex++)
	{
		curCSNTE = DSmCSNTE_PointerFromIndex(curIndex);
		
		if(curCSNTE->csnte_file_.change == SOURCE_FILE_CHANGE_v32)
		{
			UUmAssert(foundChange == UUcFalse);
			foundChange = UUcTrue;
			continue;
		}
		
		UUmAssert(curCSNTE->csnte_.mte_index == inMTEIndex);
		
		curFilePos += curCSNTE->csnte_.file_delta;
		
		offsetDistance = fabs((UUtInt32)curCSNTE->csnte_.mte_offset - mteRelativeOffset);
		if(offsetDistance < minOffsetDistance)
		{
			returnFilePos = curFilePos;
			minOffsetDistance = offsetDistance;
			csnteIndex = curIndex;
		}
	}
	
	*outMTERelativeSourcePos = returnFilePos;
	
	return csnteIndex;
}

static void
DSiMTE_GetFunctionName(
	UUtUns32	inMTEIndex,
	char		*outFunctionName)
{
	MODULES_TABLE_ENTRY_v33*	mte;
	unsigned char*				pString;
	
	mte = DSmMTE_PointerFromIndex(inMTEIndex);
	
	pString = (unsigned char*)(DSgNTE + mte->mte_nte_index);
	
	UUrString_PStr2CStr(pString, outFunctionName, DScNameBufferSize);
}

static void
DSiMTE_GetFileName(
	UUtUns32	inMTEIndex,
	char		*outFileName)
{
	MODULES_TABLE_ENTRY_v33*		mte;
	FILE_REFERENCE_TABLE_ENTRY_v32*	frte;
	unsigned char*					pString;
	
	mte = DSmMTE_PointerFromIndex(inMTEIndex);
	
	frte = DSmFRTE_PointerFromIndex(mte->mte_imp_fref.fref_frte_index);
	
	UUmAssert(frte->frte_file_.name_entry == FILE_NAME_INDEX_v32);

	pString = (unsigned char*)(DSgNTE + frte->frte_file_.nte_index);
	
	UUrString_PStr2CStr(pString, outFileName, DScNameBufferSize);
}

static UUtUns32
DSiSourceFile_GetLineFromOffset(
	char*		inFileName,
	UUtUns32	inFileOffset)
{
	UUtError	error;
	BFtFileRef*	fileRef;
	char*		fileMemory;
	UUtUns32	fileSize;
	UUtUns32	curFilePos;
	UUtUns32	curLineNum;
	char*		cp;
	char		c;
	
	error = BFrFileRef_MakeFromName(inFileName, &fileRef);
	if(error != UUcError_None) return UUcMaxUns32;
	
	error = BFrFileRef_LoadIntoMemory(fileRef, &fileSize, &fileMemory);
	if(error != UUcError_None) return UUcMaxUns32;

	BFrFileRef_Dispose(fileRef);
	
	cp = fileMemory;
	curLineNum = 0;
	curFilePos = 0;
	
	for(;;)
	{
		if(curFilePos >= fileSize || curFilePos >= inFileOffset) break;

		c = *cp;
		
		if(c == '\n')
		{
			curLineNum++;
			if(cp[1] == '\r') cp++;
		}
		else if(c == '\r')
		{
			curLineNum++;
			if(cp[1] == '\n') cp++;
		}
		
		cp++;
		curFilePos++;
	}
	
	UUrMemory_Block_Delete(fileMemory);
	
	return curLineNum;
}

UUtError
DSrInitialize(
	void)
{
#ifdef DEBUGGING
	DSiSymFile_Create();
#endif

	return UUcError_None;
}

void
DSrTerminate(
	void)
{
#ifdef DEBUGGING
	DSiSymFile_Dispose();
#endif

	return;
}

UUtError
DSrProgramCounter_GetFileAndLine(
	UUtUns32	inProgramCounter,
	char		*outFile,
	char		*outFunctionName,
	UUtUns32	*outLine)
{
#if defined(DEBUGGING) && (DEBUGGING)
	MODULES_TABLE_ENTRY_v33*				mte;
	CONTAINED_STATEMENTS_TABLE_ENTRY_v32*	csnte;
	
	UUtUns32					resourceOffset;
	extern UUtUns32				__code_start__[];		// generated by the linker
	UUtUns32					code_start;
	
	UUtUns32					mteIndex;
	UUtUns32					csnteIndex;
	UUtUns32					mteRelativeSourcePos;
	
	UUtUns32					line;
	
	if(DSgHeaderBlock == NULL) return UUcError_Generic;
	
	code_start = (UUtUns32)__code_start__;
	
	resourceOffset = inProgramCounter - code_start;
	
	mteIndex =
		DSiSymTable_MTE_FindFromRTEAndResourceOffset(
			1,
			resourceOffset);
	
	csnteIndex =
		DSiSymTable_CSNTE_FindFromMTEAndResourceOffset(
			mteIndex,
			resourceOffset,
			&mteRelativeSourcePos);
	
	mte = DSmMTE_PointerFromIndex(mteIndex);
	csnte = DSmCSNTE_PointerFromIndex(csnteIndex);
	
	DSiMTE_GetFunctionName(mteIndex, outFunctionName);
	DSiMTE_GetFileName(mteIndex, outFile);
	line = DSiSourceFile_GetLineFromOffset(outFile, mte->mte_imp_fref.fref_offset + mteRelativeSourcePos);
	
	*outLine = line;
#else
	strcpy(outFile, "<unknown");
	strcpy(outFunctionName, "<unknown>");
	*outLine = 0;
#endif
	
	return UUcError_None;
}
