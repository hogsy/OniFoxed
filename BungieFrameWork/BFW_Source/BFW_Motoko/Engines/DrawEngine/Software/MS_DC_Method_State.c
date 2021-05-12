/*
	FILE:	MS_DC_Method_State.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 13, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_TemplateManager.h"

#include "Motoko_Manager.h"

#include "MS_DC_Private.h"
#include "MS_DC_Method_State.h"
#include "MS_DC_Method_Triangle.h"
#include "MS_DC_Method_Quad.h"
#include "MS_DC_Method_SmallQuad.h"
#include "MS_DC_Method_Pent.h"

void
MSrDrawContext_Method_State_SetInt(
	M3tDrawContext*			inDrawContext,
	M3tDrawStateIntType		inDrawStateType,
	UUtInt32				inDrawState)
{
	MStDrawContextPrivate	*drawContextPrivate = (MStDrawContextPrivate *)inDrawContext->privateContext;
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
				method.triInterpolate = MSrDrawContext_Method_TriGouraudInterpolate;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_TriInterpolate,
					method);
					
				method.triFlat = MSrDrawContext_Method_TriGouraudFlat;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_TriFlat,
					method);
					
				method.quadInterpolate = MSrDrawContext_Method_QuadGouraudInterpolate;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_QuadInterpolate,
					method);
					
				method.quadFlat = MSrDrawContext_Method_QuadGouraudFlat;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_QuadFlat,
					method);
					
				method.smallQuadInterpolate = MSrDrawContext_Method_SmallQuadGouraudInterpolate;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_SmallQuadInterpolate,
					method);
					
				method.smallQuadFlat = MSrDrawContext_Method_SmallQuadGouraudFlat;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_SmallQuadFlat,
					method);
					
				method.pentInterpolate = MSrDrawContext_Method_PentGouraudInterpolate;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_PentInterpolate,
					method);
					
				method.pentFlat = MSrDrawContext_Method_PentGouraudFlat;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_PentFlat,
					method);
				break;
				
			case M3cDrawState_Appearence_Texture_Lit:
			case M3cDrawState_Appearence_Texture_Unlit:
				method.triInterpolate = MSrDrawContext_Method_TriTextureInterpolate;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_TriInterpolate,
					method);
					
				method.triFlat = MSrDrawContext_Method_TriTextureFlat;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_TriFlat,
					method);
					
				method.quadInterpolate = MSrDrawContext_Method_QuadTextureInterpolate;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_QuadInterpolate,
					method);
					
				method.quadFlat = MSrDrawContext_Method_QuadTextureFlat;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_QuadFlat,
					method);
					
				method.smallQuadInterpolate = MSrDrawContext_Method_SmallQuadTextureInterpolate;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_SmallQuadInterpolate,
					method);
					
				method.smallQuadFlat = MSrDrawContext_Method_SmallQuadTextureFlat;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_SmallQuadFlat,
					method);
					
				method.pentInterpolate = MSrDrawContext_Method_PentTextureInterpolate;
				M3rManager_SetDrawContextFunc(
					inDrawContext,
					M3cDrawContextMethodType_PentInterpolate,
					method);
					
				method.pentFlat = MSrDrawContext_Method_PentTextureFlat;
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
MSrDrawContext_Method_State_GetInt(
	M3tDrawContext*			inDrawContext,
	M3tDrawStateIntType		inDrawStateType)
{
	MStDrawContextPrivate	*drawContextPrivate = (MStDrawContextPrivate *)inDrawContext->privateContext;
	
	return drawContextPrivate->stateInt[inDrawStateType];
}

void
MSrDrawContext_Method_State_SetPtr(
	M3tDrawContext*			inDrawContext,
	M3tDrawStatePtrType		inDrawStateType,
	void*					inDrawState)
{
	MStDrawContextPrivate	*drawContextPrivate = (MStDrawContextPrivate *)inDrawContext->privateContext;

	drawContextPrivate->statePtr[inDrawStateType] = inDrawState;
	
	if(inDrawStateType == M3cDrawStatePtrType_BaseTextureMap)
	{
		M3rManager_Texture_EnsureAvailable((M3tTextureMap*)inDrawState);
	}
}

void*
MSrDrawContext_Method_State_GetPtr(
	M3tDrawContext*			inDrawContext,
	M3tDrawStatePtrType		inDrawStateType)
{
	MStDrawContextPrivate	*drawContextPrivate = (MStDrawContextPrivate *)inDrawContext->privateContext;
	
	return drawContextPrivate->statePtr[inDrawStateType];
}
