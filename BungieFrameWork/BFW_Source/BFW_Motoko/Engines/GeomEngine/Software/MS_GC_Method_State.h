/*
	FILE:	MS_GC_Method_State.h

	AUTHOR:	Brent H. Pease

	CREATED: Sept 19, 1997

	PURPOSE:

	Copyright 1997

*/
#ifndef MS_GC_METHOD_STATE_H
#define MS_GC_METHOD_STATE_H

#if 0

void
MSrGeomContext_Method_State_Set(
		M3tGeomStateIntType		inGeomStateIntType,
		UUtInt32				inState);

UUtInt32
MSrGeomContext_Method_State_Get(
		M3tGeomStateIntType		inGeomStateIntType);

UUtError
MSrGeomContext_Method_State_Push(
		void);

UUtError
MSrGeomContext_Method_State_Pop(
		void);

UUtError
MSrGeomContext_Method_State_Commit(
		void);

#endif

#endif /* MS_GC_METHOD_STATE_H */
