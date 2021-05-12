/*
	FILE:	Motoko_State_Geom.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: July 28, 1999
	
	PURPOSE: Interface to the Motoko 3D engine
	
	Copyright 1997-1999

*/

#include "BFW.h"
#include "BFW_Motoko.h"

#include "Motoko_Manager.h"
#include "Motoko_Private.h"

UUtUns16	M3gGeomStateTOS;
UUtInt32	M3gGeomStateIntStack[M3cStateStack_MaxDepth][M3cGeomStateIntType_NumTypes];

void*		M3gGeomStatePrivateStack;
UUtInt32*	M3gGeomStateInt;
UUtUns16	M3gGeomState_IntFlags;

UUtError
M3rGeom_State_Initialize(
	void)
{
	UUtUns16	stateItr;

	M3gGeomStateTOS = 0;
	M3gGeomState_IntFlags = 0xFFFF;
	M3gGeomStateInt = M3gGeomStateIntStack[M3gGeomStateTOS];
	
	if(M3gGeomEngineList[M3gActiveGeomEngine].methods.privateStateSize != 0)
	{
		M3gGeomStatePrivateStack =
			UUrMemory_Block_New(
				M3gGeomEngineList[M3gActiveGeomEngine].methods.privateStateSize * 
					M3cStateStack_MaxDepth);
		UUmError_ReturnOnNull(M3gGeomStatePrivateStack);

		for(stateItr = 0; stateItr < M3cStateStack_MaxDepth; stateItr++)
		{
			M3gGeomEngineList[M3gActiveGeomEngine].methods.privateStateNew(
				(void*)((char*)M3gGeomStatePrivateStack + 
					M3gGeomEngineList[M3gActiveGeomEngine].methods.privateStateSize * stateItr));
		}
	}
	
	UUrMemory_Clear(M3gGeomStateInt, sizeof(UUtUns32) * M3cGeomStateIntType_NumTypes);
	
	return UUcError_None;
	
}

void
M3rGeom_State_Terminate(
	void)
{
	UUtUns16	stateItr;
	
	if(M3gGeomStatePrivateStack != NULL)
	{
		for(stateItr = 0; stateItr < M3cStateStack_MaxDepth; stateItr++)
		{
			M3gGeomEngineList[M3gActiveGeomEngine].methods.privateStateDelete(
				(void*)((char*)M3gGeomStatePrivateStack + 
					M3gGeomEngineList[M3gActiveGeomEngine].methods.privateStateSize * stateItr));
		}
	}
}

void
M3rGeom_State_Set(
		M3tGeomStateIntType		inGeomStateIntType,
		UUtInt32				inState)
{
	if(M3gGeomStateInt[inGeomStateIntType] == inState) return;
	
	M3gGeomStateInt[inGeomStateIntType] = inState;
	M3gGeomState_IntFlags |= (1 << (UUtUns16)inGeomStateIntType);
}

UUtInt32
M3rGeom_State_Get(
		M3tGeomStateIntType		inGeomStateIntType)
{
	return M3gGeomStateInt[inGeomStateIntType];
}

UUtError
M3rGeom_State_Push(
		void)
{
	M3gGeomStateTOS++;
	
	if(M3gGeomStateTOS >= M3cStateStack_MaxDepth)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Stack too deep");
	}
	
	UUrMemory_MoveFast(
		M3gGeomStateInt,
		M3gGeomStateIntStack[M3gGeomStateTOS],
		sizeof(UUtUns32) * M3cGeomStateIntType_NumTypes);
	
	M3gGeomStateInt = M3gGeomStateIntStack[M3gGeomStateTOS];
	
	M3rDraw_State_Push();
	
	return UUcError_None;
}

UUtError
M3rGeom_State_Pop(
		void)
{
	if(M3gGeomStateTOS == 0)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Stack underflow");
	}
	
	M3rDraw_State_Pop();
	
	M3gGeomStateTOS--;
	M3gGeomState_IntFlags = 0xFFFF;
	
	M3gGeomStateInt = M3gGeomStateIntStack[M3gGeomStateTOS];
	
	return UUcError_None;
}

UUtError
M3rGeom_State_Commit(
		void)
{
	UUtError	error;

	if(M3gGeomState_IntFlags & (1 << M3cGeomStateIntType_SubmitMode))
	{
		M3rDraw_State_SetInt(
			M3cDrawStateIntType_SubmitMode,
			M3gGeomStateInt[M3cGeomStateIntType_SubmitMode]);
	}

	if(M3gGeomState_IntFlags & (1 << M3cGeomStateIntType_Alpha))
	{
		M3rDraw_State_SetInt(
			M3cDrawStateIntType_Alpha,
			M3gGeomStateInt[M3cGeomStateIntType_Alpha]);
	}

	if(M3gGeomState_IntFlags & (1 << M3cGeomStateIntType_Fill))
	{
		switch(M3gGeomStateInt[M3cGeomStateIntType_Fill])
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
	}
	
	if(M3gGeomState_IntFlags & (1 << M3cGeomStateIntType_Appearance))
	{
		switch(M3gGeomStateInt[M3cGeomStateIntType_Appearance])
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

	if(M3gGeomState_IntFlags & (1 << M3cGeomStateIntType_Shade))
	{
		switch(M3gGeomStateInt[M3cGeomStateIntType_Shade])
		{
			case M3cGeomState_Shade_Vertex:
				M3rDraw_State_SetInt(
					M3cDrawStateIntType_Interpolation,
					M3cDrawState_Interpolation_Vertex);
				break;

			case M3cGeomState_Shade_Face:
				M3rDraw_State_SetInt(
					M3cDrawStateIntType_Interpolation,
					M3cDrawState_Interpolation_None);
				break;
			
			default:
				UUmAssert(0);
				break;
		}
	}
	error = 
		M3gGeomEngineList[M3gActiveGeomEngine].methods.privateStateUpdate(
			(char*)M3gGeomStatePrivateStack + M3gGeomStateTOS * M3gGeomEngineList[M3gActiveGeomEngine].methods.privateStateSize,
			M3gGeomState_IntFlags,
			M3gGeomStateInt);
	UUmError_ReturnOnError(error);

	M3gGeomState_IntFlags = 0;
	
	M3rDraw_State_Commit();
	
	return UUcError_None;
}
