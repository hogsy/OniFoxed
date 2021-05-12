/*
	FILE:	BFW_ScriptLang_Context.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Oct 29, 1999
	
	PURPOSE: 
	
	Copyright 1999

*/

#include "BFW.h"
#include "BFW_TemplateManager.h"
#include "BFW_AppUtilities.h"

#include "BFW_ScriptLang.h"

#include "BFW_ScriptLang_Private.h"
#include "BFW_ScriptLang_Token.h"
#include "BFW_ScriptLang_Context.h"
#include "BFW_ScriptLang_Database.h"

SLtContext*
SLrContext_New(
	SLtContext ** inReferencePtr)
{
	SLtContext*	newContext;
	
	newContext = UUrMemory_Heap_Block_New(SLgDatabaseHeap, sizeof(*newContext));
	if(newContext == NULL) return NULL;
	
	UUrMemory_Clear(newContext, sizeof(*newContext));
	
	if ((newContext->referenceptr = inReferencePtr) != NULL) {
		// store a reference to this context
		UUmAssertWritePtr(newContext->referenceptr, sizeof(SLtContext *));
		*(newContext->referenceptr) = newContext;
	}

	return newContext;
}

void
SLrContext_Delete(
	SLtContext*	inContext)
{
	if (inContext->referenceptr != NULL) {
		// delete the reference that is kept of this context
		UUmAssertWritePtr(inContext->referenceptr, sizeof(SLtContext *));
		*(inContext->referenceptr) = NULL;
	}

	UUrMemory_Heap_Block_Delete(SLgDatabaseHeap, inContext);
}

