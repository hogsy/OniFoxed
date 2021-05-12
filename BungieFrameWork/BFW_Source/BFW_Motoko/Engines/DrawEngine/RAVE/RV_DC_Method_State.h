/*
	FILE:	RV_DC_Method_State.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 13, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef RV_DC_METHOD_STATE_H
#define RV_DC_METHOD_STATE_H

void
RVrDrawContext_Method_State_SetInt(
	M3tDrawStateIntType		inDrawStateType,
	UUtInt32				inDrawState);

UUtInt32
RVrDrawContext_Method_State_GetInt(
	M3tDrawStateIntType		inDrawStateType);

void
RVrDrawContext_Method_State_SetPtr(
	M3tDrawStatePtrType		inDrawStateType,
	void*					inDrawState);

void*
RVrDrawContext_Method_State_GetPtr(
	M3tDrawStatePtrType		inDrawStateType);

UUtError
RVrDrawContext_Method_State_Push(
	void);

UUtError
RVrDrawContext_Method_State_Pop(
	void);

UUtError
RVrDrawContext_Method_State_Commit(
	void);

#endif /* RV_DC_METHOD_STATE_H */
