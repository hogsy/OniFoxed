/*
	FILE:	OG_GC_Method_State.c

	AUTHOR:	Brent H. Pease

	CREATED: June 10, 1999

	PURPOSE:

	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "Motoko_Manager.h"

#include "OG_GC_Private.h"
#include "OG_GC_Method_State.h"

void
OGrGeomContext_Method_State_Set(
		M3tGeomStateIntType		inGeomStateIntType,
		UUtInt32				inState)
{

}

UUtInt32
OGrGeomContext_Method_State_Get(
		M3tGeomStateIntType		inGeomStateIntType)
{
	return 0xFFFFFFFF;
}

UUtError
OGrGeomContext_Method_State_Push(
		void)
{

	return UUcError_None;
}

UUtError
OGrGeomContext_Method_State_Pop(
		void)
{

	return UUcError_None;
}

UUtError
OGrGeomContext_Method_State_Commit(
		void)
{

	return UUcError_None;
}
