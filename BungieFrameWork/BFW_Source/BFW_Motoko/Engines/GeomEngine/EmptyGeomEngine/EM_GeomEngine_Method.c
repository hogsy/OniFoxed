/*
	FILE:	EM_GeomEngine_Method.c

	AUTHOR:	Brent H. Pease

	CREATED: June 10, 1999

	PURPOSE: Interface to the Motoko 3D engine

	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Akira.h"
#include "BFW_BitVector.h"
#include "BFW_Console.h"

#include "EM_GeomEngine_Method.h"

#include "Motoko_Manager.h"

#include "EM_GC_Private.h"
#include "EM_GC_Method_Frame.h"
#include "EM_GC_Method_Matrix.h"
#include "EM_GC_Method_Camera.h"
#include "EM_GC_Method_Light.h"
#include "EM_GC_Method_State.h"
#include "EM_GC_Method_Geometry.h"
#include "EM_GC_Method_Env.h"
#include "EM_GC_Method_Pick.h"

static UUtError
EGrGeomEngine_Method_ContextPrivateNew(
	M3tDrawContextDescriptor*	inDrawContextDescriptor)
{

	return UUcError_None;
}

static void
EGrGeomEngine_Method_ContextPrivateDelete(
	void)
{

}

static UUtError
EGrGeomEngine_Method_ContextSetEnvironment(
	struct AKtEnvironment*		inEnvironment)
{

	return UUcError_None;
}

void
EGrGeomEngine_Initialize(
	void)
{

}

void
EGrGeomEngine_Terminate(
	void)
{

}
