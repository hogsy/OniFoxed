/*
	FILE:	Motoko_State_Draw.c

	AUTHOR:	Brent H. Pease

	CREATED: July 28, 1999

	PURPOSE: Interface to the Motoko 3D engine

	Copyright 1997-1999

*/

#include "BFW.h"
#include "BFW_Motoko.h"

#include "Motoko_Manager.h"
#include "Motoko_Private.h"

UUtUns16	M3gDrawStateTOS;
void**		M3gDrawStatePtrStack[M3cStateStack_MaxDepth];//[M3cDrawStatePtrType_NumTypes];
UUtInt32*	M3gDrawStateIntStack[M3cStateStack_MaxDepth];//[M3cDrawStateIntType_NumTypes];

void*		M3gDrawStatePrivateStack;

void**		M3gDrawStatePtr;
UUtInt32*	M3gDrawStateInt;

UUtUns32	M3gDrawState_IntFlags;
UUtUns32	M3gDrawState_PtrFlags;

UUtBool		M3gDraw_Sorting = UUcFalse;
UUtBool		M3gDraw_Vertex_Unified = UUcTrue;

static M3tTextureMap *
M3iResolveTexture(M3tTextureMap *inTexture, UUtUns32 inTime)
{
	UUtUns32				time;
	UUtUns32				frame;
	M3tTextureMapAnimation*	animation;
	M3tTextureMap*			outTexture;

	if (NULL == inTexture)
	{
		outTexture = NULL;
		goto exit;
	}

	animation = inTexture->animation;

	if (NULL == animation)
	{
		outTexture = inTexture;
		goto exit;
	}

	if (inTexture->flags & M3cTextureFlags_Anim_DontAnimate) {
		time = 0;
	} else {
		time = inTime;
	}

	if (inTexture->flags & M3cTextureFlags_Anim_RandomStart)
	{
		time += M3gDrawStateInt[M3cDrawStateIntType_TextureInstance];
	}

	if (inTexture->flags & M3cTextureFlags_Anim_RandomFrame)
	{
		UUtUns32 seed = (time / animation->timePerFrame);

		// this code is to try to make our seed useful for the random
		// code, should get pushed up to a higher level
		UUgLocalRandomSeed = 0xfdedfded;
		UUgLocalRandomSeed ^=
			((seed & 0x000000ff) << 24) |
			((seed & 0x0000ff00) << 8) |
			((seed & 0x00ff0000) >> 8) |
			((seed & 0xff000000) >> 24);

		frame = UUmLocalRandomRange(0, animation->numFrames);
	}
	else if (inTexture->flags & M3cTextureFlags_Anim_PingPong)
	{
		UUtUns32 pingPongNumFrames = (2 * (animation->numFrames - 1));

		frame = (time / animation->timePerFrame) % pingPongNumFrames;

		if (frame >= animation->numFrames)
		{
			frame = pingPongNumFrames - frame;
		}
	}
	else
	{
		frame = (time / animation->timePerFrame) % animation->numFrames;
	}

	UUmAssert(frame >= 0);
	UUmAssert(frame < animation->numFrames);

	// animation->texture[0] is NULL but means original texture
	// to prevent recursive touching issue
	if (0 == frame)
	{
		outTexture = inTexture;
	}
	else
	{
		outTexture = animation->maps[frame];
	}

exit:
	return outTexture;
}

UUtError
M3rDraw_State_Initialize(
	void)
{
	UUtUns16	stateItr;

	M3gDrawStateTOS = 0;
	M3gDrawState_IntFlags = UUcMaxUns32;
	M3gDrawState_PtrFlags = UUcMaxUns32;

	for(stateItr = 0; stateItr < M3cStateStack_MaxDepth; stateItr++)
	{
		M3gDrawStatePtrStack[stateItr] = UUrMemory_Block_New(sizeof(void*) * M3cDrawStatePtrType_NumTypes);
		M3gDrawStateIntStack[stateItr] = UUrMemory_Block_New(sizeof(UUtUns32) * M3cDrawStateIntType_NumTypes);
	}

	M3gDrawStatePtr = M3gDrawStatePtrStack[M3gDrawStateTOS];
	M3gDrawStateInt = M3gDrawStateIntStack[M3gDrawStateTOS];

	M3gDrawStatePrivateStack = NULL;

	if(M3gDrawEngineList[M3gActiveDrawEngine].methods.privateStateSize != 0)
	{
		M3gDrawStatePrivateStack =
			UUrMemory_Block_New(
				M3gDrawEngineList[M3gActiveDrawEngine].methods.privateStateSize *
					M3cStateStack_MaxDepth);
		UUmError_ReturnOnNull(M3gDrawStatePrivateStack);

		for(stateItr = 0; stateItr < M3cStateStack_MaxDepth; stateItr++)
		{
			M3gDrawEngineList[M3gActiveDrawEngine].methods.privateStateNew(
				(void*)((char*)M3gDrawStatePrivateStack +
					M3gDrawEngineList[M3gActiveDrawEngine].methods.privateStateSize * stateItr));
		}
	}

	UUrMemory_Clear(M3gDrawStateInt, sizeof(UUtUns32) * M3cDrawStateIntType_NumTypes);
	UUrMemory_Clear(M3gDrawStatePtr, sizeof(void*) * M3cDrawStatePtrType_NumTypes);

	M3gDrawStateInt[M3cDrawStateIntType_Alpha] = 0xFF;

	return UUcError_None;
}

void
M3rDraw_State_Terminate(
	void)
{
	UUtUns16	stateItr;

	for(stateItr = 0; stateItr < M3cStateStack_MaxDepth; stateItr++)
	{
		UUrMemory_Block_Delete(M3gDrawStatePtrStack[stateItr]);
		UUrMemory_Block_Delete(M3gDrawStateIntStack[stateItr]);
	}

	if(M3gDrawStatePrivateStack != NULL)
	{
		for(stateItr = 0; stateItr < M3cStateStack_MaxDepth; stateItr++)
		{
			M3gDrawEngineList[M3gActiveDrawEngine].methods.privateStateDelete(
				(void*)((char*)M3gDrawStatePrivateStack +
					M3gDrawEngineList[M3gActiveDrawEngine].methods.privateStateSize * stateItr));
		}

		UUrMemory_Block_Delete(M3gDrawStatePrivateStack);
		M3gDrawStatePrivateStack = NULL;
	}
}

UUtUns16
M3rDraw_GetWidth(
	void)
{
	return M3gDrawEngineList[M3gActiveDrawEngine].caps.
			displayDevices[M3gActiveDisplayDevice].
				displayModes[M3gActiveDisplayMode].width;
}

UUtUns16
M3rDraw_GetHeight(
	void)
{
	return M3gDrawEngineList[M3gActiveDrawEngine].caps.
			displayDevices[M3gActiveDisplayDevice].
				displayModes[M3gActiveDisplayMode].height;
}

void
M3rDraw_State_SetInt(
	M3tDrawStateIntType	inDrawStateType,
	UUtInt32			inDrawState)
{
	UUmAssert(inDrawStateType < M3cDrawStateIntType_NumTypes);

	if(inDrawStateType != M3cDrawStateIntType_NumRealVertices &&
		M3gDrawStateInt[inDrawStateType] == inDrawState) return;

	M3gDrawStateInt[inDrawStateType] = inDrawState;
	M3gDrawState_IntFlags |= (1 << (UUtUns32)inDrawStateType);
}

UUtInt32
M3rDraw_State_GetInt(
	M3tDrawStateIntType	inDrawStateType)
{
	UUmAssert(inDrawStateType < M3cDrawStateIntType_NumTypes);

	return M3gDrawStateInt[inDrawStateType];
}

void
M3rDraw_State_SetPtr(
	M3tDrawStatePtrType	inDrawStateType,
	void*				inDrawState)
{
	UUmAssert(inDrawStateType < M3cDrawStatePtrType_NumTypes);

	M3gDrawState_PtrFlags |= (1 << (UUtUns16)inDrawStateType);

	//if(M3gDrawStatePtr[inDrawStateType] == inDrawState) return;

	M3gDrawStatePtr[inDrawStateType] = inDrawState;
}

void*
M3rDraw_State_GetPtr(
	M3tDrawStatePtrType	inDrawStateType)
{
	UUmAssert(inDrawStateType < M3cDrawStatePtrType_NumTypes);
	return M3gDrawStatePtr[inDrawStateType];
}

UUtError
M3rDraw_State_Push(
	void)
{
	M3gDrawStateTOS++;

	if(M3gDrawStateTOS >= M3cStateStack_MaxDepth)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Stack too deep");
	}

	UUrMemory_MoveFast(
		M3gDrawStateInt,
		M3gDrawStateIntStack[M3gDrawStateTOS],
		sizeof(UUtUns32) * M3cDrawStateIntType_NumTypes);

	UUrMemory_MoveFast(
		M3gDrawStatePtr,
		M3gDrawStatePtrStack[M3gDrawStateTOS],
		sizeof(UUtUns32) * M3cDrawStatePtrType_NumTypes);

	M3gDrawStatePtr = M3gDrawStatePtrStack[M3gDrawStateTOS];
	M3gDrawStateInt = M3gDrawStateIntStack[M3gDrawStateTOS];

	return UUcError_None;
}

UUtError
M3rDraw_State_Pop(
	void)
{
	if(M3gDrawStateTOS == 0)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Stack underflow");
	}

	M3gDrawStateTOS--;
	M3gDrawState_IntFlags = UUcMaxUns32;
	M3gDrawState_PtrFlags = UUcMaxUns32;

	M3gDrawStatePtr = M3gDrawStatePtrStack[M3gDrawStateTOS];
	M3gDrawStateInt = M3gDrawStateIntStack[M3gDrawStateTOS];

	return UUcError_None;
}

UUtError
M3rDraw_State_Commit(
	void)
{
	UUtError	error;
	UUtUns32	time;
	M3tTextureMap*	texture;
	UUtInt32		alpha;

	time = M3gDrawStateInt[M3cDrawStateIntType_Time];

	if(M3gDrawState_PtrFlags & (1 << M3cDrawStatePtrType_BaseTextureMap))
	{
		M3gDrawStatePtr[M3cDrawStatePtrType_BaseTextureMap] =
			M3iResolveTexture(
				M3gDrawStatePtr[M3cDrawStatePtrType_BaseTextureMap],
				time);
	}

	if(M3gDrawState_PtrFlags & (1 << M3cDrawStatePtrType_LightTextureMap))
	{
		M3gDrawStatePtr[M3cDrawStatePtrType_LightTextureMap] =
			M3iResolveTexture(
				M3gDrawStatePtr[M3cDrawStatePtrType_LightTextureMap],
				time);
	}

	texture = M3gDrawStatePtr[M3cDrawStatePtrType_BaseTextureMap];
	alpha = M3gDrawStateInt[M3cDrawStateIntType_Alpha];

#if 0
	M3gDraw_Sorting = (UUtBool)M3gDrawStateInt[M3cDrawStateIntType_SubmitMode] == M3cDrawState_SubmitMode_SortAlphaTris &&
		((texture != NULL && IMrPixelType_HasAlpha(texture->texelType)) || alpha < 0xFF);
	if((texture != NULL) && (texture->flags & M3cTextureFlags_ReceivesEnvMap)) M3gDraw_Sorting = UUcFalse;
	if((texture != NULL) && (texture->flags & M3cTextureFlags_Blend_Additive)) M3gDraw_Sorting = UUcTrue;
#endif

	if (M3gDrawStateInt[M3cDrawStateIntType_SubmitMode] == M3cDrawState_SubmitMode_SortAlphaTris)
	{
		M3gDraw_Sorting = alpha < 0xff;

		if (M3gDrawStateInt[M3cDrawStateIntType_FrameBufferBlendWithConstantAlpha])
		{
			// CB: we always alpha-sort, even if an environment-mapped texture - september 28, 2000
			M3gDraw_Sorting = UUcTrue;
		}
		else if (NULL != texture)
		{
			if (texture->flags & M3cTextureFlags_Blend_Additive)
			{
				M3gDraw_Sorting = UUcTrue;
			}
			else if (texture->flags & M3cTextureFlags_ReceivesEnvMap)
			{
				// CB:
				M3gDraw_Sorting = UUcFalse;
			}
			else
			{
				M3gDraw_Sorting = M3gDraw_Sorting || IMrPixelType_HasAlpha(texture->texelType);
			}
		}
	}
	else {
		M3gDraw_Sorting = UUcFalse;
	}

	if(M3gDraw_Sorting)
	{
		M3gDraw_Vertex_Unified = M3gDrawStateInt[M3cDrawStateIntType_VertexFormat] == M3cDrawStateVertex_Unified;
/*
	}
	if(M3gDraw_Sorting)
	{
*/
		M3gManagerDrawContext.pointArray = (M3tPointScreen*)M3gDrawStatePtr[M3cDrawStatePtrType_ScreenPointArray];
		M3gManagerDrawContext.baseUVArray = (M3tTextureCoord*)M3gDrawStatePtr[M3cDrawStatePtrType_TextureCoordArray];
		M3gManagerDrawContext.shadeArray = (UUtUns32*)M3gDrawStatePtr[M3cDrawStatePtrType_ScreenShadeArray_DC];
	}

	error =
		M3gDrawEngineList[M3gActiveDrawEngine].methods.privateStateUpdate(
			(char*)M3gDrawStatePrivateStack + M3gDrawStateTOS * M3gDrawEngineList[M3gActiveDrawEngine].methods.privateStateSize,
			M3gDrawState_IntFlags,
			M3gDrawStateInt,
			M3gDrawState_PtrFlags,
			M3gDrawStatePtr);
	UUmError_ReturnOnError(error);

	M3gDrawState_IntFlags = 0;
	M3gDrawState_PtrFlags = 0;

	return UUcError_None;
}
