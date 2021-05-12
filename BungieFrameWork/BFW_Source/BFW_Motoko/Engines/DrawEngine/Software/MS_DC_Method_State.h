/*
	FILE:	MS_DC_Method_State.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 13, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef MS_DC_METHOD_STATE_H
#define MS_DC_METHOD_STATE_H

void
MSrDrawContext_Method_State_SetInt(
	M3tDrawContext*			inDrawContext,
	M3tDrawStateIntType		inDrawStateType,
	UUtInt32				inDrawState);

UUtInt32
MSrDrawContext_Method_State_GetInt(
	M3tDrawContext*			inDrawContext,
	M3tDrawStateIntType		inDrawStateType);

void
MSrDrawContext_Method_State_SetPtr(
	M3tDrawContext*			inDrawContext,
	M3tDrawStatePtrType		inDrawStateType,
	void*					inDrawState);

void*
MSrDrawContext_Method_State_GetPtr(
	M3tDrawContext*			inDrawContext,
	M3tDrawStatePtrType		inDrawStateType);

#endif /* MS_DC_METHOD_STATE_H */
