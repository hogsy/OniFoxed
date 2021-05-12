/*
	FILE:	MG_DC_Method_State.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 13, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef MG_DC_METHOD_STATE_H
#define MG_DC_METHOD_STATE_H

#if 0
void
MGrDrawContext_Method_State_SetInt(
	M3tDrawStateIntType		inDrawStateType,
	UUtInt32				inDrawState);

UUtInt32
MGrDrawContext_Method_State_GetInt(
	M3tDrawStateIntType		inDrawStateType);

void
MGrDrawContext_Method_State_SetPtr(
	M3tDrawStatePtrType		inDrawStateType,
	void*					inDrawState);

void*
MGrDrawContext_Method_State_GetPtr(
	M3tDrawStatePtrType		inDrawStateType);

UUtError
MGrDrawContext_Method_State_Push(
	void);

UUtError
MGrDrawContext_Method_State_Pop(
	void);

UUtError
MGrDrawContext_Method_State_Commit(
	void);
#endif

#endif /* MG_DC_METHOD_STATE_H */
