/*
	FILE:	EM_DC_Method_State.h

	AUTHOR:	Brent H. Pease

	CREATED: Sept 13, 1997

	PURPOSE:

	Copyright 1997

*/
#ifndef EM_DC_METHOD_STATE_H
#define EM_DC_METHOD_STATE_H

void
EDrDrawContext_Method_State_SetInt(
	M3tDrawStateIntType		inDrawStateType,
	UUtInt32				inDrawState);

UUtInt32
EDrDrawContext_Method_State_GetInt(
	M3tDrawStateIntType		inDrawStateType);

void
EDrDrawContext_Method_State_SetPtr(
	M3tDrawStatePtrType		inDrawStateType,
	void*					inDrawState);

void*
EDrDrawContext_Method_State_GetPtr(
	M3tDrawStatePtrType		inDrawStateType);

UUtError
EDrDrawContext_Method_State_Push(
	void);

UUtError
EDrDrawContext_Method_State_Pop(
	void);

UUtError
EDrDrawContext_Method_State_Commit(
	void);

#endif /* EM_DC_METHOD_STATE_H */
