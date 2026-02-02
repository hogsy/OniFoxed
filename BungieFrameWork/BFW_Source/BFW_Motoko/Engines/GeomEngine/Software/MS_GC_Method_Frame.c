/*
	FILE:	MS_GC_Method_Frame.c

	AUTHOR:	Brent H. Pease

	CREATED: Sept 19, 1997

	PURPOSE:

	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_BitVector.h"
#include "BFW_Akira.h"

#include "MS_GC_Private.h"
#include "MS_GC_Method_Frame.h"

UUtError
MSrGeomContext_Method_Frame_Start(
	UUtUns32			inGameTicksElapsed)
{
	UUtUns32 is_fast_mode = M3rGeom_State_Get(M3cGeomStateIntType_FastMode);
	M3rMatrixStack_Clear();

	if (!is_fast_mode) {
		M3rDraw_Frame_Start(inGameTicksElapsed);

		if (MSgGeomContextPrivate->environment)
		{
			// clear env bit vectors here
			UUrBitVector_ClearBitAll(
				MSgGeomContextPrivate->gqVertexData.bitVector,
				MSgGeomContextPrivate->environment->pointArray->numPoints);

			#if 0
			UUrBitVector_ClearBitAll(
				MSgGeomContextPrivate->otVertexData.bitVector,
				MSgGeomContextPrivate->environment->renderOctTree->sharedPointArray->numPoints);
			#endif
		}
	}

	return UUcError_None;
}

UUtError
MSrGeomContext_Method_Frame_End(
	void)
{
	UUtUns32 is_fast_mode = M3rGeom_State_Get(M3cGeomStateIntType_FastMode);

	if (!is_fast_mode) {
		M3rDraw_Frame_End();
	}

	return UUcError_None;
}

UUtError
MSrGeomContext_Method_Frame_Sync(
	void)
{

	M3rDraw_Frame_Sync();

	return UUcError_None;
}
