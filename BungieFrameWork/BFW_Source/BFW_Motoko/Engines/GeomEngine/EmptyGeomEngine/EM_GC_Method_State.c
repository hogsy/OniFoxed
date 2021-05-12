/*
	FILE:	EM_GC_Method_State.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: June 10, 1999
	
	PURPOSE: 
	
	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "Motoko_Manager.h"

#include "EM_GC_Private.h"
#include "EM_GC_Method_State.h"

void
EGrGeomContext_Method_State_Set(
		M3tGeomStateIntType		inGeomStateIntType,
		UUtInt32				inState)
{

}

UUtInt32
EGrGeomContext_Method_State_Get(
		M3tGeomStateIntType		inGeomStateIntType)
{
	return 0xFFFFFFFF;
}

UUtError
EGrGeomContext_Method_State_Push(
		void)
{
	
	return UUcError_None;
}

UUtError
EGrGeomContext_Method_State_Pop(
		void)
{
	
	return UUcError_None;
}

UUtError
EGrGeomContext_Method_State_Commit(
		void)
{
	
	return UUcError_None;
}
