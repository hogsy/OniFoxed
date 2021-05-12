#pragma once
/*
	FILE:	BFW_ScriptLang_Context.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Oct 29, 1999
	
	PURPOSE: 
	
	Copyright 1999

*/
#ifndef BFW_SCRIPTLANG_CONTEXT
#define BFW_SCRIPTLANG_CONTEXT

SLtContext*
SLrContext_New(
	SLtContext ** inReferencePtr);

void
SLrContext_Delete(
	SLtContext*	inContext);

#endif /* BFW_SCRIPTLANG_CONTEXT */

