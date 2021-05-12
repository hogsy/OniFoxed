/*
	FILE:	OG_GC_Method_Frame.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: June 10, 1999
	
	PURPOSE: 
	
	Copyright 1997

*/

#ifndef __ONADA__

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_BitVector.h"
#include "BFW_Akira.h"

#include "OG_GC_Private.h"
#include "OG_GC_Method_Frame.h"

UUtError
OGrGeomContext_Method_Frame_Start(
	UUtUns32			inGameTime)
{
	
	M3rDraw_Frame_Start(inGameTime);

	return UUcError_None;
}

UUtError
OGrGeomContext_Method_Frame_End(
	void)
{

	M3rDraw_Frame_End();

	return UUcError_None;
}

UUtError
OGrGeomContext_Method_Frame_Sync(
	void)
{

	M3rDraw_Frame_Sync();

	return UUcError_None;
}



#endif // __ONADA__

