/*
	FILE:	MS_GC_Method_State.c

	AUTHOR:	Brent H. Pease

	CREATED: Sept 19, 1997

	PURPOSE:

	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "Motoko_Manager.h"

#include "MS_GC_Private.h"
#include "MS_GC_Method_State.h"
#include "MS_Geom_Clip.h"

#if 0
void
MSrGeomContext_Method_State_Set(
		M3tGeomStateIntType		inGeomStateIntType,
		UUtInt32				inState)
{

	MSgGeomContextPrivate->stateDirty = UUcTrue;

	MSgGeomContextPrivate->stateInt[inGeomStateIntType] = inState;

	switch(inGeomStateIntType)
	{
		case M3cGeomStateIntType_Fill:
			break;

		case M3cGeomStateIntType_Shade:
			break;

		case M3cGeomStateIntType_Appearance:
			break;

		case M3cGeomStateIntType_Hint:
			break;

		case M3cGeomStateIntType_SubmitMode:
			M3rDraw_State_SetInt(
				M3cDrawStateIntType_SubmitMode,
				inState);
			break;

		case M3cGeomStateIntType_Alpha:
			M3rDraw_State_SetInt(
				M3cDrawStateIntType_Alpha,
				inState);
			break;

		case M3cGeomStateIntType_DebugMode:
			break;

		default:
			UUmAssert(!"Unknown state type");
	}
}

UUtInt32
MSrGeomContext_Method_State_Get(
		M3tGeomStateIntType		inGeomStateIntType)
{

	return MSgGeomContextPrivate->stateInt[inGeomStateIntType];
}

UUtError
MSrGeomContext_Method_State_Push(
		void)
{
	UUtUns16	i;
	UUtUns16	tos;

	MSrGeomContext_Method_State_Commit();

	M3rDraw_State_Push();

	tos = ++MSgGeomContextPrivate->stateTOS;

	UUmAssert(tos < MScStateStack_MaxDepth);

	for(i = 0; i < M3cGeomStateIntType_NumTypes; i++)
	{
		MSgGeomContextPrivate->stateIntStack[tos][i] = MSgGeomContextPrivate->stateIntStack[tos-1][i];
	}

	MSgGeomContextPrivate->stateInt = MSgGeomContextPrivate->stateIntStack[tos];

	return UUcError_None;
}

UUtError
MSrGeomContext_Method_State_Pop(
		void)
{
	UUtUns16				tos;

	M3rDraw_State_Pop();

	UUmAssert(MSgGeomContextPrivate->stateTOS > 0);

	MSgGeomContextPrivate->stateTOS -= 1;
	tos = MSgGeomContextPrivate->stateTOS;

	MSgGeomContextPrivate->stateInt = MSgGeomContextPrivate->stateIntStack[tos];

	MSgGeomContextPrivate->stateDirty = UUcTrue;

	return UUcError_None;
}

UUtError
MSrGeomContext_Method_State_Commit(
		void)
{

	if(MSgGeomContextPrivate->stateDirty == UUcTrue)
	{
		MSgGeomContextPrivate->stateDirty = UUcFalse;

		switch(MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Fill])
		{
			case M3cGeomState_Fill_Point:
				UUmAssert(0);
				break;

			case M3cGeomState_Fill_Line:
				M3rDraw_State_SetInt(
					M3cDrawStateIntType_Fill,
					M3cDrawState_Fill_Line);
				break;

			case M3cGeomState_Fill_Solid:
				M3rDraw_State_SetInt(
					M3cDrawStateIntType_Fill,
					M3cDrawState_Fill_Solid);
				break;

			default:
				UUmAssert(0);
				break;
		}

		switch(MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Shade])
		{
			case M3cGeomState_Shade_Vertex:
				M3rDraw_State_SetInt(
					M3cDrawStateIntType_Interpolation,
					M3cDrawState_Interpolation_Vertex);

				switch(MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Appearance])
				{
					case M3cGeomState_Appearance_Gouraud:
						MSgGeomContextPrivate->polyComputeVertexProc =
							MSrClip_ComputeVertex_GouraudInterpolate;
						break;

					case M3cGeomState_Appearance_Texture:
						MSgGeomContextPrivate->polyComputeVertexProc =
							MSrClip_ComputeVertex_TextureInterpolate;
						break;

					default:
						UUmAssert(0);
						break;
				}
				break;

			case M3cGeomState_Shade_Face:
				M3rDraw_State_SetInt(
					M3cDrawStateIntType_Interpolation,
					M3cDrawState_Interpolation_None);

				switch(MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Appearance])
				{
					case M3cGeomState_Appearance_Gouraud:
						MSgGeomContextPrivate->polyComputeVertexProc =
							MSrClip_ComputeVertex_GouraudFlat;
						break;

					case M3cGeomState_Appearance_Texture:
						MSgGeomContextPrivate->polyComputeVertexProc =
							MSrClip_ComputeVertex_TextureFlat;
						break;

					default:
						UUmAssert(0);
						break;
				}
				break;

			default:
				UUmAssert(0);
				break;
		}

		switch(MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Appearance])
		{
			case M3cGeomState_Appearance_Gouraud:
				M3rDraw_State_SetInt(
					M3cDrawStateIntType_Appearence,
					M3cDrawState_Appearence_Gouraud);
				break;

			case M3cGeomState_Appearance_Texture:
				M3rDraw_State_SetInt(
					M3cDrawStateIntType_Appearence,
					M3cDrawState_Appearence_Texture_Lit);
				break;

			default:
				UUmAssert(0);
				break;
		}
	}

	return UUcError_None;
}

#endif
