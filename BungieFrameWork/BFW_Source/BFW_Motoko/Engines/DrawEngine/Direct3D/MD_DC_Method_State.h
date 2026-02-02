/*
	FILE:	MD_DC_Method_State.h

	AUTHOR:	Brent H. Pease

	CREATED: Sept 13, 1997

	PURPOSE:

	Copyright 1997

*/
#ifndef MD_DC_METHOD_STATE_H
#define MD_DC_METHOD_STATE_H

void
MDrDrawContext_Method_State_SetInt(
	M3tDrawContext*			inDrawContext,
	M3tDrawStateIntType		inDrawStateType,
	UUtInt32				inDrawState);

UUtInt32
MDrDrawContext_Method_State_GetInt(
	M3tDrawContext*			inDrawContext,
	M3tDrawStateIntType		inDrawStateType);

void
MDrDrawContext_Method_State_SetPtr(
	M3tDrawContext*			inDrawContext,
	M3tDrawStatePtrType		inDrawStateType,
	void*					inDrawState);

void*
MDrDrawContext_Method_State_GetPtr(
	M3tDrawContext*			inDrawContext,
	M3tDrawStatePtrType		inDrawStateType);

#endif /* MD_DC_METHOD_STATE_H */
