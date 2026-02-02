/*
	FILE:	OG_GC_Method_State.h

	AUTHOR:	Brent H. Pease

	CREATED: June 10, 1999

	PURPOSE:

	Copyright 1997

*/
#ifndef OG_GC_METHOD_STATE_H
#define OG_GC_METHOD_STATE_H

void
OGrGeomContext_Method_State_Set(
			M3tGeomStateIntType		inGeomStateIntType,
		UUtInt32				inState);

UUtInt32
OGrGeomContext_Method_State_Get(
			M3tGeomStateIntType		inGeomStateIntType);

UUtError
OGrGeomContext_Method_State_Push(
		void);

UUtError
OGrGeomContext_Method_State_Pop(
		void);

UUtError
OGrGeomContext_Method_State_Commit(
		void);

#endif /* OG_GC_METHOD_STATE_H */
