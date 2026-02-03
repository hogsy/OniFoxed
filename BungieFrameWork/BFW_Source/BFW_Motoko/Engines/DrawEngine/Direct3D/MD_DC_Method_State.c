/*
	FILE:	MD_DC_Method_State.c

	AUTHOR:	Brent H. Pease

	CREATED: Sept 13, 1997

	PURPOSE:

	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_TemplateManager.h"

#include "Motoko_Manager.h"

#include "MD_DC_Private.h"
#include "MD_DC_Method_State.h"
#include "MD_DC_Method_Triangle.h"
#include "MD_DC_Method_Quad.h"
#include "MD_DC_Method_SmallQuad.h"
#include "MD_DC_Method_Pent.h"

void
MDrDrawContext_Method_State_SetInt(
	M3tDrawContext*			inDrawContext,
	M3tDrawStateIntType		inDrawStateType,
	UUtInt32				inDrawState)
{
	MDtDrawContextPrivate	*drawContextPrivate = (MDtDrawContextPrivate *)inDrawContext->privateContext;
	M3tDrawContextMethod	method;

	if(drawContextPrivate->stateInt[inDrawStateType] == inDrawState)
	{
		return;
	}

	drawContextPrivate->stateInt[inDrawStateType] = inDrawState;

	if(inDrawStateType == M3cDrawStateIntType_Appearence)
	{
		switch((M3tDrawStateAppearence)inDrawState)
		{
			case M3cDrawState_Appearence_Gouraud:
				method.triInterpolate = MDrDrawContext_Method_TriGouraudInterpolate;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_TriInterpolate,
					method);

				method.triFlat = MDrDrawContext_Method_TriGouraudFlat;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_TriFlat,
					method);

				method.quadInterpolate = MDrDrawContext_Method_QuadGouraudInterpolate;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_QuadInterpolate,
					method);

				method.quadFlat = MDrDrawContext_Method_QuadGouraudFlat;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_QuadFlat,
					method);

				method.smallQuadInterpolate = MDrDrawContext_Method_SmallQuadGouraudInterpolate;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_SmallQuadInterpolate,
					method);

				method.smallQuadFlat = MDrDrawContext_Method_SmallQuadGouraudFlat;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_SmallQuadFlat,
					method);

				method.pentInterpolate = MDrDrawContext_Method_PentGouraudInterpolate;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_PentInterpolate,
					method);

				method.pentFlat = MDrDrawContext_Method_PentGouraudFlat;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_PentFlat,
					method);
				break;

			case M3cDrawState_Appearence_Texture_Lit:
			case M3cDrawState_Appearence_Texture_Unlit:
				method.triInterpolate = MDrDrawContext_Method_TriTextureInterpolate;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_TriInterpolate,
					method);

				method.triFlat = MDrDrawContext_Method_TriTextureFlat;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_TriFlat,
					method);

				method.quadInterpolate = MDrDrawContext_Method_QuadTextureInterpolate;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_QuadInterpolate,
					method);

				method.quadFlat = MDrDrawContext_Method_QuadTextureFlat;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_QuadFlat,
					method);

				method.smallQuadInterpolate = MDrDrawContext_Method_SmallQuadTextureInterpolate;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_SmallQuadInterpolate,
					method);

				method.smallQuadFlat = MDrDrawContext_Method_SmallQuadTextureFlat;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_SmallQuadFlat,
					method);

				method.pentInterpolate = MDrDrawContext_Method_PentTextureInterpolate;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_PentInterpolate,
					method);

				method.pentFlat = MDrDrawContext_Method_PentTextureFlat;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_PentFlat,
					method);
				break;

			default:
				UUmAssert(!"Unkown appearence type");
		}
	}
}

UUtInt32
MDrDrawContext_Method_State_GetInt(
	M3tDrawContext*			inDrawContext,
	M3tDrawStateIntType		inDrawStateType)
{
	MDtDrawContextPrivate	*drawContextPrivate = (MDtDrawContextPrivate *)inDrawContext->privateContext;

	return drawContextPrivate->stateInt[inDrawStateType];
}

void
MDrDrawContext_Method_State_SetPtr(
	M3tDrawContext*			inDrawContext,
	M3tDrawStatePtrType		inDrawStateType,
	void*					inDrawState)
{
	MDtDrawContextPrivate	*drawContextPrivate = (MDtDrawContextPrivate *)inDrawContext->privateContext;

	drawContextPrivate->statePtr[inDrawStateType] = inDrawState;
}

void*
MDrDrawContext_Method_State_GetPtr(
	M3tDrawContext*			inDrawContext,
	M3tDrawStatePtrType		inDrawStateType)
{
	MDtDrawContextPrivate	*drawContextPrivate = (MDtDrawContextPrivate *)inDrawContext->privateContext;

	return drawContextPrivate->statePtr[inDrawStateType];
}
