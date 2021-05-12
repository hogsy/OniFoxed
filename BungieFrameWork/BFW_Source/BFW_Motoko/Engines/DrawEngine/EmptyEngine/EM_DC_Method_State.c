/*
	FILE:	EM_DC_Method_State.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 13, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_TemplateManager.h"
#include "BFW_BitVector.h"

#include "Motoko_Manager.h"

#include "EM_DC_Private.h"
#include "EM_DC_Method_State.h"
#include "EM_DC_Method_Triangle.h"
#include "EM_DC_Method_Quad.h"
#include "EM_DC_Method_SmallQuad.h"
#include "EM_DC_Method_Pent.h"


void
EDrDrawContext_Method_State_SetInt(
	M3tDrawStateIntType		inDrawStateType,
	UUtInt32				inDrawState)
{

}

UUtInt32
EDrDrawContext_Method_State_GetInt(
	M3tDrawStateIntType		inDrawStateType)
{
	
	return 0;
}

void
EDrDrawContext_Method_State_SetPtr(
	M3tDrawStatePtrType		inDrawStatePrType,
	void*					inDrawState)
{

}

void*
EDrDrawContext_Method_State_GetPtr(
	M3tDrawStatePtrType		inDrawStateType)
{
	
	return NULL;
}

UUtError
EDrDrawContext_Method_State_Push(
	void)
{
	
	return UUcError_None;
}

UUtError
EDrDrawContext_Method_State_Pop(
	void)
{

	return UUcError_None;
}


UUtError
EDrDrawContext_Method_State_Commit(
	void)
{

	return UUcError_None;
}


