/*
	FILE:	EM_GC_Method_State.h

	AUTHOR:	Brent H. Pease

	CREATED: June 10, 1999

	PURPOSE:

	Copyright 1997

*/
#ifndef EM_GC_METHOD_STATE_H
#define EM_GC_METHOD_STATE_H

void
EGrGeomContext_Method_State_Set(
			M3tGeomStateIntType		inGeomStateIntType,
		UUtInt32				inState);

UUtInt32
EGrGeomContext_Method_State_Get(
			M3tGeomStateIntType		inGeomStateIntType);

UUtError
EGrGeomContext_Method_State_Push(
		void);

UUtError
EGrGeomContext_Method_State_Pop(
		void);

UUtError
EGrGeomContext_Method_State_Commit(
		void);

#endif /* EM_GC_METHOD_STATE_H */
