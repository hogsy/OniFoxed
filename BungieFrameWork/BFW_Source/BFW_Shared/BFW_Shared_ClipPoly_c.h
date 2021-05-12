/*
	FILE:	BFW_Shared_ClipPoly_c.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: May 22, 1997
	
	PURPOSE: Interface to the Motoko 3D engine
	
	Copyright 1997
	
	NOTE
	
	THIS CODE IS ONE UGLY PIECE OF HACKED UP SHIT
*/

#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING

	extern FILE*	gClipFile;
	extern UUtBool	gClipDumping;
	extern UUtInt32	gClipNesting;

#endif

#if !defined(MSmGeomClipPoly_TriFunctionName) || \
	!defined(MSmGeomClipPoly_QuadFunctionName) || \
	!defined(MSmGeomClipPoly_ClipPointFunctionName) || \
	!defined(MSmGeomClipPoly_4D) || \
	!defined(MSmGeomClipPoly_TextureMap) || \
	!defined(MSmGeomClipPoly_InterpShading) || \
	!defined(MSmGeomClipPoly_BBox) || \
	!defined(MSmGeomClipPoly_BlockFrustum) || \
	!defined(MSmGeomClipPoly_CheckFrustum) || \
	!defined(MSmGeomClipPoly_SplitTexturePoint)
	
	#error some macros have not been defined
	
#endif

#undef MSmGeomClipPoly_ReturnValue
#undef MSmGeomClipPoly_BufferWidth
#undef MSmGeomClipPoly_BufferHeight
#undef MSmGeomClipPoly_Finished
#undef MSmGeomClipPoly_ContinueClipping
#undef MSmGeomClipPoly_ContextType
#undef MSmGeomClipPoly_Return
#undef MSmGeomClipPoly_TriFinished
#undef MSmGeomClipPoly_QuadFinished
#undef MSmGeomClipPoly_PentFinished
#undef MSmGeomClipPoly_TriContinueClipping
#undef MSmGeomClipPoly_QuadContinueClipping

	#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING
	
		#define MSmGeomClipPoly_Finished_Debug()	gClipNesting--; UUmAssert(gClipNesting >= 0)
		
	#else
	
		#define MSmGeomClipPoly_Finished_Debug()
	
	#endif

#if MSmGeomClipPoly_BlockFrustum

	
	#define MSmGeomClipPoly_ReturnValue			static void
	#define MSmGeomClipPoly_BufferWidth			(float)(contextPrivate->width)
	#define MSmGeomClipPoly_BufferHeight		(float)(contextPrivate->height)
	#define MSmGeomClipPoly_ContextType			M3tGeomContext
	
	#define MSmGeomClipPoly_TriFinished(							\
			index0, index1, index2, 								\
			texIndex0, texIndex1, texIndex2)						\
		M3rDraw_VisTriBlock(										\
			inContext->drawContext,								\
			index0,	index1, index2);
	#define MSmGeomClipPoly_QuadFinished(							\
			index0, index1, index2, index3,							\
			texIndex0, texIndex1, texIndex2, texIndex3);				\
		M3rDraw_VisQuadBlock(										\
			inContext->drawContext,								\
			index0,	index1, index2, index3);
	#define MSmGeomClipPoly_PentFinished(							\
			index0, index1, index2, index3, index4,					\
			texIndex0, texIndex1, texIndex2, texIndex3, texIndex4)	\
		M3rDraw_VisPentBlock(										\
			inContext->drawContext,								\
			index0,	index1, index2, index3, index4);
	
	#define MSmGeomClipPoly_TriContinueClipping(	\
			index0, index1, index2, 			\
			texIndex0, texIndex1, texIndex2, 			\
			clipCode0, clipCode1, clipCode2,	\
			clippedPlanes)						\
												\
		MSmGeomClipPoly_TriFunctionName(		\
			inContext,							\
			index0, index1, index2,				\
			clipCode0, clipCode1, clipCode2,	\
			clippedPlanes);
		
	#define MSmGeomClipPoly_QuadContinueClipping(		\
			index0, index1, index2, index3, 			\
			texIndex0, texIndex1, texIndex2, texIndex3, 			\
			clipCode0, clipCode1, clipCode2, clipCode3, \
			clippedPlanes)								\
														\
		MSmGeomClipPoly_QuadFunctionName(			\
			inContext,									\
			index0, index1, index2,	index3,				\
			clipCode0, clipCode1, clipCode2, clipCode3,	\
			clippedPlanes);
	
	#define MSmGeomClipPoly_Return	MSmGeomClipPoly_Finished_Debug(); return

#elif MSmGeomClipPoly_CheckFrustum

	
	#define MSmGeomClipPoly_ReturnValue			static UUtBool
	#define MSmGeomClipPoly_BufferWidth			(float)(contextPrivate->width)
	#define MSmGeomClipPoly_BufferHeight		(float)(contextPrivate->height)
	#define MSmGeomClipPoly_ContextType			M3tGeomContext
	
	#define MSmGeomClipPoly_TriFinished(							\
			index0, index1, index2, 								\
			texIndex0, texIndex1, texIndex2)						\
		if(M3rDraw_VisTriCheck(										\
			inContext->drawContext,								\
			index0,	index1, index2) == UUcTrue) return UUcTrue;
	#define MSmGeomClipPoly_QuadFinished(							\
			index0, index1, index2, index3,							\
			texIndex0, texIndex1, texIndex2, texIndex3);				\
		if(M3rDraw_VisQuadCheck(										\
			inContext->drawContext,								\
			index0,	index1, index2, index3) == UUcTrue) return UUcTrue;
	#define MSmGeomClipPoly_PentFinished(							\
			index0, index1, index2, index3, index4,					\
			texIndex0, texIndex1, texIndex2, texIndex3, texIndex4)	\
		if(M3rDraw_VisPentCheck(										\
			inContext->drawContext,								\
			index0,	index1, index2, index3, index4) == UUcTrue) return UUcTrue;
	
	#define MSmGeomClipPoly_TriContinueClipping(	\
			index0, index1, index2, 			\
			texIndex0, texIndex1, texIndex2, 			\
			clipCode0, clipCode1, clipCode2,	\
			clippedPlanes)						\
												\
		if(MSmGeomClipPoly_TriFunctionName(		\
			inContext,							\
			index0, index1, index2,				\
			clipCode0, clipCode1, clipCode2,	\
			clippedPlanes) == UUcTrue) return UUcTrue;
		
	#define MSmGeomClipPoly_QuadContinueClipping(		\
			index0, index1, index2, index3, 			\
			texIndex0, texIndex1, texIndex2, texIndex3, 			\
			clipCode0, clipCode1, clipCode2, clipCode3, \
			clippedPlanes)								\
														\
		if(MSmGeomClipPoly_QuadFunctionName(			\
			inContext,									\
			index0, index1, index2,	index3,				\
			clipCode0, clipCode1, clipCode2, clipCode3,	\
			clippedPlanes) == UUcTrue) return UUcTrue;
	
	#define MSmGeomClipPoly_Return	MSmGeomClipPoly_Finished_Debug(); return UUcFalse;

#elif MSmGeomClipPoly_BBox
	
	#define MSmGeomClipPoly_ReturnValue			static void
	#define MSmGeomClipPoly_BufferWidth			(float)(contextPrivate->width)
	#define MSmGeomClipPoly_BufferHeight		(float)(contextPrivate->height)
	#define MSmGeomClipPoly_ContextType			M3tGeomContext
	
	#define MSmGeomClipPoly_TriFinished(							\
			index0, index1, index2, 								\
			texIndex0, texIndex1, texIndex2)						\
		MSiVis_Block_Frustum_Tri(								\
			inContext,												\
			index0,	index1, index2)
	#define MSmGeomClipPoly_QuadFinished(							\
			index0, index1, index2, index3,							\
			texIndex0, texIndex1, texIndex2, texIndex3)				\
		MSiVis_Block_Frustum_Quad(								\
			inContext,												\
			index0,	index1, index2, index3)
	#define MSmGeomClipPoly_PentFinished(							\
			index0, index1, index2, index3, index4,					\
			texIndex0, texIndex1, texIndex2, texIndex3, texIndex4)	\
		MSiVis_Block_Frustum_Quad(								\
			inContext,												\
			index0,	index1, index2, index3);						\
		MSiVis_Block_ClipFrustum_Tri(								\
			inContext,												\
			index0,	index3, index4)
	
	#define MSmGeomClipPoly_TriContinueClipping(	\
			index0, index1, index2, 			\
			texIndex0, texIndex1, texIndex2, 			\
			clipCode0, clipCode1, clipCode2,	\
			clippedPlanes)						\
												\
		MSmGeomClipPoly_TriFunctionName(		\
			inContext,							\
			index0, index1, index2,				\
			clipCode0, clipCode1, clipCode2,	\
			clippedPlanes);
		
	#define MSmGeomClipPoly_QuadContinueClipping(		\
			index0, index1, index2, index3, 			\
			texIndex0, texIndex1, texIndex2, texIndex3, 			\
			clipCode0, clipCode1, clipCode2, clipCode3, \
			clippedPlanes)								\
														\
		MSmGeomClipPoly_QuadFunctionName(			\
			inContext,									\
			index0, index1, index2,	index3,				\
			clipCode0, clipCode1, clipCode2, clipCode3,	\
			clippedPlanes);
	
	#define MSmGeomClipPoly_Return	MSmGeomClipPoly_Finished_Debug(); return
	
#else

	#define MSmGeomClipPoly_ReturnValue			void
	#define MSmGeomClipPoly_BufferWidth			(float)(contextPrivate->width)
	#define MSmGeomClipPoly_BufferHeight		(float)(contextPrivate->height)
	#define MSmGeomClipPoly_ContextType			M3tGeomContext
	
	#if MSmGeomClipPoly_InterpShading
	
		#define MSmGeomClipPoly_TriFinished(			\
				index0, index1, index2, 				\
				texIndex0, texIndex1, texIndex2)	\
			M3rDraw_TriInterpolate(						\
				inContext->drawContext,				\
				index0,	index1, index2)				
		#define MSmGeomClipPoly_QuadFinished(						\
				index0, index1, index2, index3, 					\
				texIndex0, texIndex1, texIndex2, texIndex3)	\
			M3rDraw_QuadInterpolate(								\
				inContext->drawContext,							\
				index0,	index1, index2, index3)				
		#define MSmGeomClipPoly_PentFinished(						\
				index0, index1, index2, index3, index4,				\
				texIndex0, texIndex1, texIndex2, texIndex3, texIndex4)	\
			M3rDraw_PentInterpolate(												\
				inContext->drawContext,											\
				index0,	index1, index2, index3, index4)		
	
		#define MSmGeomClipPoly_TriContinueClipping(	\
				index0, index1, index2,					\
				texIndex0, texIndex1, texIndex2,	\
				clipCode0, clipCode1, clipCode2,		\
				clippedPlanes)							\
			MSmGeomClipPoly_TriFunctionName(			\
				inContext,								\
				index0, index1, index2,					\
				clipCode0, clipCode1, clipCode2,		\
				clippedPlanes)
		#define MSmGeomClipPoly_QuadContinueClipping(				\
				index0, index1, index2, index3,						\
				texIndex0, texIndex1, texIndex2, texIndex3,	\
				clipCode0, clipCode1, clipCode2, clipCode3,			\
				clippedPlanes)										\
			MSmGeomClipPoly_QuadFunctionName(						\
				inContext,											\
				index0, index1, index2, index3,						\
				clipCode0, clipCode1, clipCode2, clipCode3,			\
				clippedPlanes)

	#else
	
		#if MSmGeomClipPoly_SplitTexturePoint
		
			#define MSmGeomClipPoly_TriFinished(			\
					index0, index1, index2,					\
					texIndex0, texIndex1, texIndex2)	\
				M3rDraw_TriSplit(							\
					inContext->drawContext,				\
					index0,	index1, index2,					\
					texIndex0, texIndex1, texIndex2)				
			#define MSmGeomClipPoly_QuadFinished(						\
					index0, index1, index2, index3,						\
					texIndex0, texIndex1, texIndex2, texIndex3)	\
				M3rDraw_QuadSplit(										\
					inContext->drawContext,							\
					index0,	index1, index2, index3,						\
					texIndex0, texIndex1, texIndex2, texIndex3)				
			#define MSmGeomClipPoly_PentFinished(										\
					index0, index1, index2, index3, index4,								\
					texIndex0, texIndex1, texIndex2, texIndex3, texIndex4)	\
				M3rDraw_PentSplit(														\
					inContext->drawContext,											\
					index0,	index1, index2, index3, index4,								\
					texIndex0, texIndex1, texIndex2, texIndex3, texIndex4)				
		
			#define MSmGeomClipPoly_TriContinueClipping(	\
					index0, index1, index2,					\
					texIndex0, texIndex1, texIndex2,	\
					clipCode0, clipCode1, clipCode2,		\
					clippedPlanes)							\
				MSmGeomClipPoly_TriFunctionName(			\
					inContext,								\
					index0, index1, index2,					\
					texIndex0, texIndex1, texIndex2,	\
					clipCode0, clipCode1, clipCode2,		\
					clippedPlanes)
			#define MSmGeomClipPoly_QuadContinueClipping(				\
					index0, index1, index2, index3,						\
					texIndex0, texIndex1, texIndex2, texIndex3,	\
					clipCode0, clipCode1, clipCode2, clipCode3,			\
					clippedPlanes)										\
				MSmGeomClipPoly_QuadFunctionName(						\
					inContext,											\
					index0, index1, index2, index3,						\
					texIndex0, texIndex1, texIndex2, texIndex3,	\
					clipCode0, clipCode1, clipCode2, clipCode3,			\
					clippedPlanes)
		#else
			
			#define MSmGeomClipPoly_TriFinished(			\
					index0, index1, index2, 				\
					texIndex0, texIndex1, texIndex2)	\
				M3rDraw_TriFlat(							\
					inContext->drawContext,				\
					index0,	index1, index2, inShade)				
			#define MSmGeomClipPoly_QuadFinished(						\
					index0, index1, index2, index3,						\
					texIndex0, texIndex1, texIndex2, texIndex3)	\
				M3rDraw_QuadFlat(										\
					inContext->drawContext,							\
					index0,	index1, index2, index3, inShade)				
			#define MSmGeomClipPoly_PentFinished(										\
					index0, index1, index2, index3, index4,								\
					texIndex0, texIndex1, texIndex2, texIndex3, texIndex4)	\
				M3rDraw_PentFlat(														\
					inContext->drawContext,											\
					index0,	index1, index2, index3, index4, inShade)				
		
			#define MSmGeomClipPoly_TriContinueClipping(	\
					index0, index1, index2,					\
					texIndex0, texIndex1, texIndex2,	\
					clipCode0, clipCode1, clipCode2,		\
					clippedPlanes)							\
				MSmGeomClipPoly_TriFunctionName(			\
					inContext,								\
					index0, index1, index2,					\
					clipCode0, clipCode1, clipCode2,		\
					clippedPlanes,							\
					inShade)
			#define MSmGeomClipPoly_QuadContinueClipping(				\
					index0, index1, index2, index3,						\
					texIndex0, texIndex1, texIndex2, texIndex3,	\
					clipCode0, clipCode1, clipCode2, clipCode3,			\
					clippedPlanes)										\
				MSmGeomClipPoly_QuadFunctionName(						\
					inContext,											\
					index0, index1, index2, index3,						\
					clipCode0, clipCode1, clipCode2, clipCode3,			\
					clippedPlanes,										\
					inShade)
		#endif
	
	#endif
	
	#define MSmGeomClipPoly_Return	MSmGeomClipPoly_Finished_Debug(); return
	
#endif

#undef MSmGeomClipPoly_CheckTrivialAcceptTri
#define MSmGeomClipPoly_CheckTrivialAcceptTri(		\
		clipCode0, clipCode1, clipCode2, 			\
		index0, index1, index2,						\
		texIndex0, texIndex1, texIndex2,		\
		clippedPlanes) 								\
	if(!(clipCode0 | clipCode1 | clipCode2))		\
	{												\
		MSmGeomClipPoly_TriFinished(				\
			index0,	index1, index2,					\
			texIndex0, texIndex1, texIndex2);	\
	}												\
	else											\
	{												\
		MSmGeomClipPoly_TriContinueClipping(		\
			index0,	index1, index2,					\
			texIndex0, texIndex1, texIndex2,	\
			clipCode0, clipCode1, clipCode2,		\
			(UUtUns16)(clippedPlanes));				\
	}

#undef MSmGeomClipPoly_CheckTrivialAcceptQuad
#define MSmGeomClipPoly_CheckTrivialAcceptQuad(						\
		clipCode0, clipCode1, clipCode2, clipCode3,					\
		index0, index1, index2, index3,								\
		texIndex0, texIndex1, texIndex2, texIndex3,			\
		clippedPlanes)												\
	if(!(clipCode0 | clipCode1 | clipCode2 | clipCode3))			\
	{																\
		MSmGeomClipPoly_QuadFinished(								\
			index0,	index1, index2, index3,							\
			texIndex0, texIndex1, texIndex2, texIndex3);	\
	}																\
	else															\
	{																\
		MSmGeomClipPoly_QuadContinueClipping(						\
			index0,	index1, index2, index3,							\
			texIndex0, texIndex1, texIndex2, texIndex3,		\
			clipCode0, clipCode1, clipCode2, clipCode3,				\
			(UUtUns16)(clippedPlanes));								\
	}									

#undef MSmGeomClipPoly_CheckTrivialAcceptPent
#define MSmGeomClipPoly_CheckTrivialAcceptPent(									\
			clipCode0, clipCode1, clipCode2, clipCode3, clipCode4,				\
			index0, index1, index2, index3, index4,								\
			texIndex0, texIndex1, texIndex2, texIndex3, texIndex4,	\
			clippedPlanes)											\
	if(!(clipCode0 | clipCode1 | clipCode2 | clipCode3))			\
	{																\
		MSmGeomClipPoly_QuadFinished(								\
			index0,	index1, index2, index3,							\
			texIndex0, texIndex1, texIndex2, texIndex3);	\
	}																\
	else															\
	{																\
		MSmGeomClipPoly_QuadContinueClipping(						\
			index0,	index1, index2, index3,							\
			texIndex0, texIndex1, texIndex2, texIndex3,		\
			clipCode0, clipCode1, clipCode2, clipCode3,				\
			(UUtUns16)(clippedPlanes));								\
	}																\
	if(!(clipCode0 | clipCode3 | clipCode4))						\
	{																\
		MSmGeomClipPoly_TriFinished(								\
			index0,	index3, index4,									\
			texIndex0, texIndex3, texIndex4);					\
	}																\
	else															\
	{																\
		MSmGeomClipPoly_TriContinueClipping(						\
			index0,	index3, index4,									\
			texIndex0, texIndex3, texIndex4,					\
			clipCode0, clipCode3, clipCode4,						\
			(UUtUns16)(clippedPlanes));								\
	}

#if 0

	#undef MSmGeomClipPoly_VerifyTrivialRejectTri
	#define MSmGeomClipPoly_VerifyTrivialRejectTri(clipCode0, clipCode1, clipCode2) \
		UUmAssert((clipCode0) & (clipCode1) & (clipCode2));

	#undef MSmGeomClipPoly_VerifyTrivialRejectQuad
	#define MSmGeomClipPoly_VerifyTrivialRejectQuad(clipCode0, clipCode1, clipCode2, clipCode3) \
		UUmAssert((clipCode0) & (clipCode1) & (clipCode2) & (clipCode3));

	#undef MSmGeomClipPoly_VerifyTrivialRejectPent
	#define MSmGeomClipPoly_VerifyTrivialRejectPent(clipCode0, clipCode1, clipCode2, clipCode3, clipCode4) \
		UUmAssert((clipCode0) & (clipCode1) & (clipCode2) & (clipCode3) & (clipCode4));

#else

	#undef MSmGeomClipPoly_VerifyTrivialRejectTri
	#define MSmGeomClipPoly_VerifyTrivialRejectTri(clipCode0, clipCode1, clipCode2)
	#undef MSmGeomClipPoly_VerifyTrivialRejectQuad
	#define MSmGeomClipPoly_VerifyTrivialRejectQuad(clipCode0, clipCode1, clipCode2, clipCode3)
	#undef MSmGeomClipPoly_VerifyTrivialRejectPent
	#define MSmGeomClipPoly_VerifyTrivialRejectPent(clipCode0, clipCode1, clipCode2, clipCode3, clipCode4)
	
#endif

#undef MSmGeomClipPoly_ComputeNewClipPoints

#if MSmGeomClipPoly_4D

	#if MSmGeomClipPoly_BlockFrustum || MSmGeomClipPoly_CheckFrustum
	
		#define MSmGeomClipPoly_ComputeNewClipPoints(								\
				index0, index1, index2, index3,										\
				texIndex0, texIndex1,  texIndex2,  texIndex3)	\
			MSmGeomClipPoly_ClipPointFunctionName(		\
				screenPoints,							\
				frustumPoints,							\
				curClipPlane,							\
				index0,									\
				index1,									\
				newVIndex0,								\
				scaleX,									\
				scaleY,									\
				&newClipCode0);							\
			MSmGeomClipPoly_ClipPointFunctionName(		\
				screenPoints,							\
				frustumPoints,							\
				curClipPlane,							\
				index2,									\
				index3,									\
				newVIndex1,								\
				scaleX,									\
				scaleY,									\
				&newClipCode1);

	#elif MSmGeomClipPoly_TextureMap
	
		#if MSmGeomClipPoly_SplitTexturePoint
			
			#if MSmGeomClipPoly_InterpShading
			
				#define MSmGeomClipPoly_ComputeNewClipPoints(								\
						index0, index1, index2, index3,										\
						texIndex0, texIndex1,  texIndex2,  texIndex3)	\
					MSmGeomClipPoly_ClipPointFunctionName(		\
						screenPoints,							\
						screenShades,							\
						frustumPoints,							\
						textureCoords,							\
						curClipPlane,							\
						index0,									\
						index1,									\
						newVIndex0,								\
						texIndex0,							\
						texIndex1,							\
						newTexIndex0,							\
						scaleX,									\
						scaleY,									\
						&newClipCode0);							\
					MSmGeomClipPoly_ClipPointFunctionName(		\
						screenPoints,							\
						screenShades,							\
						frustumPoints,							\
						textureCoords,							\
						curClipPlane,							\
						index2,									\
						index3,									\
						newVIndex1,								\
						texIndex2,							\
						texIndex3,							\
						newTexIndex1,							\
						scaleX,									\
						scaleY,									\
						&newClipCode1);

			#else
			
				#define MSmGeomClipPoly_ComputeNewClipPoints(								\
						index0, index1, index2, index3,										\
						texIndex0, texIndex1,  texIndex2,  texIndex3)	\
					MSmGeomClipPoly_ClipPointFunctionName(		\
						screenPoints,							\
						frustumPoints,							\
						textureCoords,							\
						curClipPlane,							\
						index0,									\
						index1,									\
						newVIndex0,								\
						texIndex0,							\
						texIndex1,							\
						newTexIndex0,							\
						scaleX,									\
						scaleY,									\
						&newClipCode0);							\
					MSmGeomClipPoly_ClipPointFunctionName(		\
						screenPoints,							\
						frustumPoints,							\
						textureCoords,							\
						curClipPlane,							\
						index2,									\
						index3,									\
						newVIndex1,								\
						texIndex2,							\
						texIndex3,							\
						newTexIndex1,							\
						scaleX,									\
						scaleY,									\
						&newClipCode1);
			
			#endif
			
		#else
		
			#if MSmGeomClipPoly_InterpShading
			
				#define MSmGeomClipPoly_ComputeNewClipPoints(								\
						index0, index1, index2, index3,										\
						texIndex0,  texIndex1,  texIndex2,  texIndex3)	\
					MSmGeomClipPoly_ClipPointFunctionName(		\
						screenPoints,							\
						screenShades,							\
						frustumPoints,							\
						textureCoords,							\
						curClipPlane,							\
						index0,									\
						index1,									\
						newVIndex0,								\
						scaleX,									\
						scaleY,									\
						&newClipCode0);							\
					MSmGeomClipPoly_ClipPointFunctionName(		\
						screenPoints,							\
						screenShades,							\
						frustumPoints,							\
						textureCoords,							\
						curClipPlane,							\
						index2,									\
						index3,									\
						newVIndex1,								\
						scaleX,									\
						scaleY,									\
						&newClipCode1);
			
			#else
			
				#define MSmGeomClipPoly_ComputeNewClipPoints(								\
						index0, index1, index2, index3,										\
						texIndex0,  texIndex1,  texIndex2,  texIndex3)	\
					MSmGeomClipPoly_ClipPointFunctionName(		\
						screenPoints,							\
						frustumPoints,							\
						textureCoords,							\
						curClipPlane,							\
						index0,									\
						index1,									\
						newVIndex0,								\
						scaleX,									\
						scaleY,									\
						&newClipCode0);							\
					MSmGeomClipPoly_ClipPointFunctionName(		\
						screenPoints,							\
						frustumPoints,							\
						textureCoords,							\
						curClipPlane,							\
						index2,									\
						index3,									\
						newVIndex1,								\
						scaleX,									\
						scaleY,									\
						&newClipCode1);
			
			#endif
			
		#endif
		
	#else
	
		#if MSmGeomClipPoly_SplitTexturePoint
		
			#error What?
		
		#endif
		
		#if MSmGeomClipPoly_InterpShading
			
			#define MSmGeomClipPoly_ComputeNewClipPoints(								\
					index0, index1, index2, index3,										\
					texIndex0,  texIndex1,  texIndex2,  texIndex3)	\
				MSmGeomClipPoly_ClipPointFunctionName(		\
					screenPoints,							\
					screenShades,							\
					frustumPoints,							\
					curClipPlane,							\
					index0,									\
					index1,									\
					newVIndex0,								\
					scaleX,									\
					scaleY,									\
					&newClipCode0);							\
				MSmGeomClipPoly_ClipPointFunctionName(		\
					screenPoints,							\
					screenShades,							\
					frustumPoints,							\
					curClipPlane,							\
					index2,									\
					index3,									\
					newVIndex1,								\
					scaleX,									\
					scaleY,									\
					&newClipCode1);
		
		#else
		
			#define MSmGeomClipPoly_ComputeNewClipPoints(								\
					index0, index1, index2, index3,										\
					texIndex0,  texIndex1,  texIndex2,  texIndex3)	\
				MSmGeomClipPoly_ClipPointFunctionName(		\
					screenPoints,							\
					frustumPoints,							\
					curClipPlane,							\
					index0,									\
					index1,									\
					newVIndex0,								\
					scaleX,									\
					scaleY,									\
					&newClipCode0);							\
				MSmGeomClipPoly_ClipPointFunctionName(		\
					screenPoints,							\
					frustumPoints,							\
					curClipPlane,							\
					index2,									\
					index3,									\
					newVIndex1,								\
					scaleX,									\
					scaleY,									\
					&newClipCode1);
		
		#endif
		
	#endif

#elif MSmGeomClipPoly_BBox

	#define MSmGeomClipPoly_ComputeNewClipPoints(								\
			index0, index1, index2, index3,										\
			texIndex0,  texIndex1,  texIndex2,  texIndex3)	\
		MSmGeomClipPoly_ClipPointFunctionName(		\
			worldPoints,							\
			bBox,									\
			curClipPlane,							\
			index0,									\
			index1,									\
			newVIndex0,								\
			&newClipCode0);							\
		MSmGeomClipPoly_ClipPointFunctionName(		\
			worldPoints,							\
			bBox,									\
			curClipPlane,							\
			index2,									\
			index3,									\
			newVIndex1,								\
			&newClipCode1);

#else

	#if MSmGeomClipPoly_TextureMap
		
		#if MSmGeomClipPoly_SplitTexturePoint
		
			#if MSmGeomClipPoly_InterpShading
				
				#error
			
			#endif
			
			#define MSmGeomClipPoly_ComputeNewClipPoints(								\
					index0, index1, index2, index3,										\
					texIndex0,  texIndex1,  texIndex2,  texIndex3)	\
				MSmGeomClipPoly_ClipPointFunctionName(		\
					screenPoints,							\
					textureCoords,							\
					curClipPlane,							\
					index0,									\
					index1,									\
					newVIndex0,								\
					texIndex0,							\
					texIndex1,							\
					newTexIndex0,							\
					MSmGeomClipPoly_BufferWidth,			\
					MSmGeomClipPoly_BufferHeight,			\
					&newClipCode0);							\
				MSmGeomClipPoly_ClipPointFunctionName(		\
					screenPoints,							\
					textureCoords,							\
					curClipPlane,							\
					index2,									\
					index3,									\
					newVIndex1,								\
					texIndex2,							\
					texIndex3,							\
					newTexIndex1,							\
					MSmGeomClipPoly_BufferWidth,			\
					MSmGeomClipPoly_BufferHeight,			\
					&newClipCode1);
		
		#else
		
			#if MSmGeomClipPoly_InterpShading
			
				#define MSmGeomClipPoly_ComputeNewClipPoints(								\
						index0, index1, index2, index3,										\
						texIndex0,  texIndex1,  texIndex2,  texIndex3)	\
					MSmGeomClipPoly_ClipPointFunctionName(		\
						screenPoints,							\
						screenShades,							\
						textureCoords,							\
						curClipPlane,							\
						index0,									\
						index1,									\
						newVIndex0,								\
						MSmGeomClipPoly_BufferWidth,			\
						MSmGeomClipPoly_BufferHeight,			\
						&newClipCode0);							\
					MSmGeomClipPoly_ClipPointFunctionName(		\
						screenPoints,							\
						screenShades,							\
						textureCoords,							\
						curClipPlane,							\
						index2,									\
						index3,									\
						newVIndex1,								\
						MSmGeomClipPoly_BufferWidth,			\
						MSmGeomClipPoly_BufferHeight,			\
						&newClipCode1);
		
			#else
			
				#define MSmGeomClipPoly_ComputeNewClipPoints(								\
						index0, index1, index2, index3,										\
						texIndex0,  texIndex1,  texIndex2,  texIndex3)	\
					MSmGeomClipPoly_ClipPointFunctionName(		\
						screenPoints,							\
						textureCoords,							\
						curClipPlane,							\
						index0,									\
						index1,									\
						newVIndex0,								\
						MSmGeomClipPoly_BufferWidth,			\
						MSmGeomClipPoly_BufferHeight,			\
						&newClipCode0);							\
					MSmGeomClipPoly_ClipPointFunctionName(		\
						screenPoints,							\
						textureCoords,							\
						curClipPlane,							\
						index2,									\
						index3,									\
						newVIndex1,								\
						MSmGeomClipPoly_BufferWidth,			\
						MSmGeomClipPoly_BufferHeight,			\
						&newClipCode1);
		
			#endif
			
		#endif
		
	#else
	
		#if MSmGeomClipPoly_SplitTexturePoint
		
			#error what?
		
		#endif
		
	
		#if MSmGeomClipPoly_InterpShading
		
			#define MSmGeomClipPoly_ComputeNewClipPoints(								\
					index0, index1, index2, index3, 									\
					texIndex0,  texIndex1,  texIndex2,  texIndex3)	\
				MSmGeomClipPoly_ClipPointFunctionName(		\
					screenPoints,							\
					screenShades,							\
					curClipPlane,							\
					index0,									\
					index1,									\
					newVIndex0,								\
					MSmGeomClipPoly_BufferWidth,			\
					MSmGeomClipPoly_BufferHeight,			\
					&newClipCode0);							\
				MSmGeomClipPoly_ClipPointFunctionName(		\
					screenPoints,							\
					screenShades,							\
					curClipPlane,							\
					index2,									\
					index3,									\
					newVIndex1,								\
					MSmGeomClipPoly_BufferWidth,			\
					MSmGeomClipPoly_BufferHeight,			\
					&newClipCode1);
		
		#else
			
			#define MSmGeomClipPoly_ComputeNewClipPoints(								\
					index0, index1, index2, index3, 									\
					texIndex0,  texIndex1,  texIndex2,  texIndex3)	\
				MSmGeomClipPoly_ClipPointFunctionName(		\
					screenPoints,							\
					curClipPlane,							\
					index0,									\
					index1,									\
					newVIndex0,								\
					MSmGeomClipPoly_BufferWidth,			\
					MSmGeomClipPoly_BufferHeight,			\
					&newClipCode0);							\
				MSmGeomClipPoly_ClipPointFunctionName(		\
					screenPoints,							\
					curClipPlane,							\
					index2,									\
					index3,									\
					newVIndex1,								\
					MSmGeomClipPoly_BufferWidth,			\
					MSmGeomClipPoly_BufferHeight,			\
					&newClipCode1);
		
		#endif
					
	#endif
	
#endif


static void
MSmGeomClipPoly_ClipPointFunctionName(
	
	#if MSmGeomClipPoly_BBox
	
		M3tPoint3D*				inWorldPoints,
		M3tBoundingBox_MinMax*	inBBox,
		
	#else
	
		M3tPointScreen*		inScreenPoints,
		
	#endif
	
	#if MSmGeomClipPoly_InterpShading
		
		UUtUns16*			inScreenShades,
		
	#endif
	
	#if MSmGeomClipPoly_4D
	
		M3tPoint4D*			inFrustumPoints,

	#endif
	
	#if MSmGeomClipPoly_TextureMap
		
		#if 0//MSmGeomClipPoly_SplitTexturePoint
		
			M3tEnvTextureCoord*	inTextureCoords,
		
		#else
		
			M3tTextureCoord*	inTextureCoords,
		
		#endif
		
	#endif
	
	UUtUns16				inClipPlane,
	UUtUns16				inIndexIn,
	UUtUns16				inIndexOut,
	UUtUns16				inIndexNew,

#undef inTexIndexIn
#undef inTexIndexOut
#undef inTexIndexNew

#if MSmGeomClipPoly_SplitTexturePoint

	UUtUns16				inTexIndexIn,
	UUtUns16				inTexIndexOut,
	UUtUns16				inTexIndexNew,
	
#else

	#define inTexIndexIn	inIndexIn
	#define inTexIndexOut	inIndexOut
	#define inTexIndexNew	inIndexNew
	
#endif
	
	#if !MSmGeomClipPoly_BBox
	
		float					inXFactor,
		float					inYFactor,
	
	#endif
	
	UUtUns16				*outClipCodeNew)
{
	UUtUns16	newClipCode;
	
	#if MSmGeomClipPoly_4D
	
		float		oneMinust, pIn, pOut, negW, invW;
		float		hW, hX, hY, hZ;
	
	#else
	
		float		xIn, xOut, xNew;
		float		yIn, yOut, yNew;
		float		zIn, zOut, zNew;
		
		#if !MSmGeomClipPoly_BBox
		
			float		wNew;
		#endif
		
	#endif
	
	#if MSmGeomClipPoly_InterpShading
		
		UUtUns16	shadeIn, shadeOut;
		float		rOut, gOut, bOut;
		float		rIn, gIn, bIn;
		float		rNew, gNew, bNew;
		
	#endif
	
	#if !MSmGeomClipPoly_BBox
	
		float		wIn, wOut;
	
	#endif
	
		float	t;
		
#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING
	
	if(gClipDumping)
	{
		fprintf(gClipFile, "Entering %s\n", UUmStringize(MSmGeomClipPoly_ClipPointFunctionName));
		fprintf(gClipFile, "\tinVIndex: %d\n", inIndexIn);
		fprintf(gClipFile, "\toutVIndex: %d\n", inIndexOut);
		fprintf(gClipFile, "\tnewVIndex: %d\n", inIndexNew);
		fprintf(gClipFile, "\tclipPlane: ");
		MSiDump_ClipCodes(gClipFile, inClipPlane);
		fprintf(gClipFile, "\n");
	}
	
#endif
	
	#if MSmGeomClipPoly_4D
	
		MSiVerifyPoint4D(inFrustumPoints + inIndexIn);
		MSiVerifyPoint4D(inFrustumPoints + inIndexOut);
		
		#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING
			
			if(gClipDumping)
			{
				fprintf(gClipFile, "\tinFrust: (%f, %f, %f, %f)\n",
					(inFrustumPoints + inIndexIn)->x,
					(inFrustumPoints + inIndexIn)->y,
					(inFrustumPoints + inIndexIn)->z,
					(inFrustumPoints + inIndexIn)->w);
				fprintf(gClipFile, "\toutFrust: (%f, %f, %f, %f)\n",
					(inFrustumPoints + inIndexOut)->x,
					(inFrustumPoints + inIndexOut)->y,
					(inFrustumPoints + inIndexOut)->z,
					(inFrustumPoints + inIndexOut)->w);
			}
		
		#endif
		
		wIn = (inFrustumPoints + inIndexIn)->w;
		wOut = (inFrustumPoints + inIndexOut)->w;
		
		switch(inClipPlane)										
		{															
			case MScClipCode_PosX:									
				UUmAssert((inFrustumPoints + inIndexIn)->x <= (inFrustumPoints + inIndexIn)->w);			
				UUmAssert((inFrustumPoints + inIndexOut)->x > (inFrustumPoints + inIndexOut)->w);			
				
				pIn = (inFrustumPoints + inIndexIn)->x;					
				pOut = (inFrustumPoints + inIndexOut)->x;				
				t = (wIn - pIn) / ((pOut - pIn) - (wOut - wIn));	
				oneMinust = (1.0f - t);								
				UUmAssert(t <= 1.0f);
				UUmAssert(t >= -1.0f);		
				hX = hW = t * (inFrustumPoints + inIndexOut)->w + oneMinust * (inFrustumPoints + inIndexIn)->w;
				hY = t * (inFrustumPoints + inIndexOut)->y + oneMinust * (inFrustumPoints + inIndexIn)->y;
				hZ = t * (inFrustumPoints + inIndexOut)->z + oneMinust * (inFrustumPoints + inIndexIn)->z;
				break;				
												
			case MScClipCode_NegX:									
				UUmAssert((inFrustumPoints + inIndexIn)->x >= -(inFrustumPoints + inIndexIn)->w);			
				UUmAssert((inFrustumPoints + inIndexOut)->x < -(inFrustumPoints + inIndexOut)->w);		
				
				pIn = (inFrustumPoints + inIndexIn)->x;					
				pOut = (inFrustumPoints + inIndexOut)->x;				
				t = (pIn + wIn) / ((pIn - pOut) + (wIn - wOut));	
				oneMinust = (1.0f - t);								
				UUmAssert(t <= 1.0f);
				UUmAssert(t >= -1.0f);		
				hW = t * (inFrustumPoints + inIndexOut)->w + oneMinust * (inFrustumPoints + inIndexIn)->w;
				hY = t * (inFrustumPoints + inIndexOut)->y + oneMinust * (inFrustumPoints + inIndexIn)->y;
				hX = -hW;											
				hZ = t * (inFrustumPoints + inIndexOut)->z + oneMinust * (inFrustumPoints + inIndexIn)->z;
				break;					
											
			case MScClipCode_PosY:									
				UUmAssert((inFrustumPoints + inIndexIn)->y <= (inFrustumPoints + inIndexIn)->w);			
				UUmAssert((inFrustumPoints + inIndexOut)->y > (inFrustumPoints + inIndexOut)->w);			
				
				pIn = (inFrustumPoints + inIndexIn)->y;					
				pOut = (inFrustumPoints + inIndexOut)->y;				
				t = (wIn - pIn) / ((pOut - pIn) - (wOut - wIn));	
				oneMinust = (1.0f - t);								
				UUmAssert(t <= 1.0f);
				UUmAssert(t >= -1.0f);		
				hY = hW = t * (inFrustumPoints + inIndexOut)->w + oneMinust * (inFrustumPoints + inIndexIn)->w;
				hX = t * (inFrustumPoints + inIndexOut)->x + oneMinust * (inFrustumPoints + inIndexIn)->x;
				hZ = t * (inFrustumPoints + inIndexOut)->z + oneMinust * (inFrustumPoints + inIndexIn)->z;
				break;
															
			case MScClipCode_NegY:									
				UUmAssert((inFrustumPoints + inIndexIn)->y >= -(inFrustumPoints + inIndexIn)->w);			
				UUmAssert((inFrustumPoints + inIndexOut)->y < -(inFrustumPoints + inIndexOut)->w);		
				
				pIn = (inFrustumPoints + inIndexIn)->y;					
				pOut = (inFrustumPoints + inIndexOut)->y;				
				t = (pIn + wIn) / ((pIn - pOut) + (wIn - wOut));	
				oneMinust = (1.0f - t);								
				UUmAssert(t <= 1.0f);
				UUmAssert(t >= -1.0f);		
				hW = t * (inFrustumPoints + inIndexOut)->w + oneMinust * (inFrustumPoints + inIndexIn)->w;
				hY = -hW;											
				hX = t * (inFrustumPoints + inIndexOut)->x + oneMinust * (inFrustumPoints + inIndexIn)->x;
				hZ = t * (inFrustumPoints + inIndexOut)->z + oneMinust * (inFrustumPoints + inIndexIn)->z;
				break;		
														
			case MScClipCode_PosZ:									
				UUmAssert((inFrustumPoints + inIndexIn)->z <= (inFrustumPoints + inIndexIn)->w);			
				UUmAssert((inFrustumPoints + inIndexOut)->z > (inFrustumPoints + inIndexOut)->w);			
				
				pIn = (inFrustumPoints + inIndexIn)->z;					
				pOut = (inFrustumPoints + inIndexOut)->z;				
				t = (wIn - pIn) / ((pOut - pIn) - (wOut - wIn));	
				oneMinust = (1.0f - t);								
				UUmAssert(t <= 1.0f);
				UUmAssert(t >= -1.0f);		
				hW = t * (inFrustumPoints + inIndexOut)->w + oneMinust * (inFrustumPoints + inIndexIn)->w;
				hX = t * (inFrustumPoints + inIndexOut)->x + oneMinust * (inFrustumPoints + inIndexIn)->x;
				hY = t * (inFrustumPoints + inIndexOut)->y + oneMinust * (inFrustumPoints + inIndexIn)->y;
				hZ = hW;											
				break;	
															
			case MScClipCode_NegZ:									
				UUmAssert((inFrustumPoints + inIndexIn)->z >= 0.0f);	
				UUmAssert((inFrustumPoints + inIndexOut)->z < 0.0f);	
				
				pIn = (inFrustumPoints + inIndexIn)->z;					
				pOut = (inFrustumPoints + inIndexOut)->z;				
				t = pIn / (pIn - pOut);								
				oneMinust = (1.0f - t);								
				UUmAssert(t <= 1.0f);
				UUmAssert(t >= -1.0f);		
				hW = t * (inFrustumPoints + inIndexOut)->w + oneMinust * (inFrustumPoints + inIndexIn)->w;
				hX = t * (inFrustumPoints + inIndexOut)->x + oneMinust * (inFrustumPoints + inIndexIn)->x;
				hY = t * (inFrustumPoints + inIndexOut)->y + oneMinust * (inFrustumPoints + inIndexIn)->y;
				hZ = 0.0f;											
				break;												
		}
		
		#if MSmGeomClipPoly_InterpShading
			
			// XXX - optimize this someday
			shadeOut = *(inScreenShades + inIndexOut);
			shadeIn = *(inScreenShades + inIndexIn);
			
			rOut = (float)((shadeOut >> 10) & 0x1F);
			gOut = (float)((shadeOut >> 5) & 0x1F);
			bOut = (float)((shadeOut >> 0) & 0x1F);
			
			rIn = (float)((shadeIn >> 10) & 0x1F);
			gIn = (float)((shadeIn >> 5) & 0x1F);
			bIn = (float)((shadeIn >> 0) & 0x1F);
			
			rNew = t * rOut + oneMinust * rIn;
			gNew = t * gOut + oneMinust * gIn;
			bNew = t * bOut + oneMinust * bIn;
			
			*(inScreenShades + inIndexNew) = 
				((UUtUns16)rNew << 10) |
				((UUtUns16)gNew << 5) |
				((UUtUns16)bNew << 0);
			
		#endif
		
		#if MSmGeomClipPoly_TextureMap
			
			#if MSmGeomClipPoly_SplitTexturePoint
				
				inTexIndexNew <<= 1;
				inTexIndexOut <<= 1;
				inTexIndexIn <<= 1;
				(inTextureCoords + inTexIndexNew)->u = t * (inTextureCoords + inTexIndexOut)->u + oneMinust * (inTextureCoords + inTexIndexIn)->u;
				(inTextureCoords + inTexIndexNew)->v = t * (inTextureCoords + inTexIndexOut)->v + oneMinust * (inTextureCoords + inTexIndexIn)->v;
				inTexIndexNew += 1;
				inTexIndexOut += 1;
				inTexIndexIn += 1;
				(inTextureCoords + inTexIndexNew)->u = t * (inTextureCoords + inTexIndexOut)->u + oneMinust * (inTextureCoords + inTexIndexIn)->u;
				(inTextureCoords + inTexIndexNew)->v = t * (inTextureCoords + inTexIndexOut)->v + oneMinust * (inTextureCoords + inTexIndexIn)->v;
				
			#else
			
				(inTextureCoords + inTexIndexNew)->u = t * (inTextureCoords + inTexIndexOut)->u + oneMinust * (inTextureCoords + inTexIndexIn)->u;
				(inTextureCoords + inTexIndexNew)->v = t * (inTextureCoords + inTexIndexOut)->v + oneMinust * (inTextureCoords + inTexIndexIn)->v;
			
			#endif
			
		#endif
		
		negW = -hW;
		newClipCode = MScClipCode_None;
		
		if(hX > hW)
		{
			newClipCode |= MScClipCode_PosX;
		}
		
		if(hX < negW)
		{
			newClipCode |= MScClipCode_NegX;
		}
		
		if(hY > hW)
		{
			newClipCode |= MScClipCode_PosY;
		}
		
		if(hY < negW)
		{
			newClipCode |= MScClipCode_NegY;
		}
		
		if(hZ > hW)
		{
			newClipCode |= MScClipCode_PosZ;
		}
		
		if(hZ < 0.0)
		{
			newClipCode |= MScClipCode_NegZ;
		}
		
		(inFrustumPoints + inIndexNew)->x = hX;
		(inFrustumPoints + inIndexNew)->y = hY;
		(inFrustumPoints + inIndexNew)->z = hZ;
		(inFrustumPoints + inIndexNew)->w = hW;
		
		MSrClipCode_ValidateFrustum((inFrustumPoints + inIndexNew), newClipCode);
		
		if(newClipCode == 0)
		{
			invW = 1.0f / hW;
			if(hX == negW)
			{
				(inScreenPoints + inIndexNew)->x = 0.0f;
			}
			else
			{
				(inScreenPoints + inIndexNew)->x = (hX * invW + 1.0f) * inXFactor;
			}
			
			if(hY == hW)
			{
				(inScreenPoints + inIndexNew)->y = 0.0f;
			}
			else
			{
				(inScreenPoints + inIndexNew)->y = (hY * invW - 1.0f) * inYFactor;
			}
			
			(inScreenPoints + inIndexNew)->z = hZ * invW;
			(inScreenPoints + inIndexNew)->invW = invW;
			
			MSiVerifyPointScreen((inScreenPoints + inIndexNew));
		}
	
	#elif MSmGeomClipPoly_BBox
	
		xIn = (inWorldPoints + inIndexIn)->x;						
		yIn = (inWorldPoints + inIndexIn)->y;						
		zIn = (inWorldPoints + inIndexIn)->z;						
		xOut = (inWorldPoints + inIndexOut)->x;					
		yOut = (inWorldPoints + inIndexOut)->y;					
		zOut = (inWorldPoints + inIndexOut)->z;		
					
		switch(inClipPlane)										
		{															
			case MScClipCode_PosX:									
//				UUmAssert((inScreenPoints + inIndexIn)->x < inXFactor);			
//				UUmAssert((inScreenPoints + inIndexOut)->x >= inXFactor);	
				
				xNew = inBBox->maxPoint.x;
				t = (xOut - xNew) / (xOut - xIn);
				UUmAssert(t >= 0.0f && t <= 1.0f);
				yNew = yOut + t * (yIn - yOut);						
				zNew = zOut + t * (zIn - zOut);						
				break;						
										
			case MScClipCode_NegX:									
//				UUmAssert((inScreenPoints + inIndexIn)->x >= 0.0f);		
//				UUmAssert((inScreenPoints + inIndexOut)->x < 0.0f);	
					
				xNew = inBBox->minPoint.x;
				t = (xNew - xOut) / (xIn - xOut);					
				UUmAssert(t >= 0.0f && t <= 1.0f);
				yNew = yOut + t * (yIn - yOut);						
				zNew = zOut + t * (zIn - zOut);						
				break;								
								
			case MScClipCode_PosY:									
//				UUmAssert((inScreenPoints + inIndexIn)->y >= 0.0f);		
//				UUmAssert((inScreenPoints + inIndexOut)->y < 0.0f);	
					
				yNew = inBBox->maxPoint.y;
				t = (yOut - yNew) / (yOut - yIn);
				UUmAssert(t >= 0.0f && t <= 1.0f);
				xNew = xOut + t * (xIn - xOut);						
				zNew = zOut + t * (zIn - zOut);						
				break;									
							
			case MScClipCode_NegY:									
//				UUmAssert((inScreenPoints + inIndexIn)->y < inYFactor);			
//				UUmAssert((inScreenPoints + inIndexOut)->y >= inYFactor);
							
				yNew = inBBox->minPoint.y;
				t = (yNew - yOut) / (yIn - yOut);					
				UUmAssert(t >= 0.0f && t <= 1.0f);
				xNew = xOut + t * (xIn - xOut);						
				zNew = zOut + t * (zIn - zOut);						
				break;										
						
			case MScClipCode_PosZ:									
//				UUmAssert((inScreenPoints + inIndexIn)->z < 1.0);		
//				UUmAssert((inScreenPoints + inIndexOut)->z >= 1.0);	
					
				zNew = inBBox->maxPoint.z;
				t = (zOut - zNew) / (zOut - zIn);
				UUmAssert(t >= 0.0f && t <= 1.0f);
				xNew = xOut + t * (xIn - xOut);						
				yNew = yOut + t * (yIn - yOut);						
				break;										
						
			case MScClipCode_NegZ:									
//				UUmAssert((inScreenPoints + inIndexIn)->z >= 0.0f);		
//				UUmAssert((inScreenPoints + inIndexOut)->z < 0.0f);		
				
				zNew = inBBox->minPoint.z;
				t = (zNew - zOut) / (zIn - zOut);					
				UUmAssert(t >= 0.0f && t <= 1.0f);
				xNew = xOut + t * (xIn - xOut);						
				yNew = yOut + t * (yIn - yOut);						
				break;												
		}
		
		(inWorldPoints + inIndexNew)->x = xNew;					
		(inWorldPoints + inIndexNew)->y = yNew;					
		(inWorldPoints + inIndexNew)->z = zNew;					
		
		newClipCode = 0;
		
		if(xNew > inBBox->maxPoint.x)
		{
			newClipCode |= MScClipCode_PosX;
		}
		else if(xNew < inBBox->minPoint.x)
		{
			newClipCode |= MScClipCode_NegX;
		}
		
		if(yNew > inBBox->maxPoint.y)
		{
			newClipCode |= MScClipCode_PosY;
		}
		else if(yNew < inBBox->minPoint.y)
		{
			newClipCode |= MScClipCode_NegY;
		}
		
		if(zNew > inBBox->maxPoint.z)
		{
			newClipCode |= MScClipCode_PosZ;
		}
		else if(zNew < inBBox->minPoint.z)
		{
			newClipCode |= MScClipCode_NegZ;
		}
		
	#else
	
		MSiVerifyPointScreen(inScreenPoints + inIndexIn);		
		MSiVerifyPointScreen(inScreenPoints + inIndexOut);	
		
		wIn = (inScreenPoints + inIndexIn)->invW;			
		xIn = (inScreenPoints + inIndexIn)->x;						
		yIn = (inScreenPoints + inIndexIn)->y;						
		zIn = (inScreenPoints + inIndexIn)->z;						
		wOut = (inScreenPoints + inIndexOut)->invW;			
		xOut = (inScreenPoints + inIndexOut)->x;					
		yOut = (inScreenPoints + inIndexOut)->y;					
		zOut = (inScreenPoints + inIndexOut)->z;		
					
		switch(inClipPlane)										
		{															
			case MScClipCode_PosX:									
//				UUmAssert((inScreenPoints + inIndexIn)->x < inXFactor);			
//				UUmAssert((inScreenPoints + inIndexOut)->x >= inXFactor);	
						
				t = (xOut - inXFactor) / (xOut - xIn);
				yNew = yOut + t * (yIn - yOut);						
				zNew = zOut + t * (zIn - zOut);						
				xNew = inXFactor - 0.0001f;		
				wNew = wOut + t * (wIn - wOut);					
				break;						
										
			case MScClipCode_NegX:									
//				UUmAssert((inScreenPoints + inIndexIn)->x >= 0.0f);		
//				UUmAssert((inScreenPoints + inIndexOut)->x < 0.0f);	
					
				t = (0.0f - xOut) / (xIn - xOut);					
				yNew = yOut + t * (yIn - yOut);						
				zNew = zOut + t * (zIn - zOut);						
				xNew = 0.0;											
				wNew = wOut + t * (wIn - wOut);						
				break;								
								
			case MScClipCode_PosY:									
//				UUmAssert((inScreenPoints + inIndexIn)->y >= 0.0f);		
//				UUmAssert((inScreenPoints + inIndexOut)->y < 0.0f);	
					
				t = (0.0f - yOut) / (yIn - yOut);					
				xNew = xOut + t * (xIn - xOut);						
				zNew = zOut + t * (zIn - zOut);						
				yNew = 0.0;											
				wNew = wOut + t * (wIn - wOut);						
				break;									
							
			case MScClipCode_NegY:									
//				UUmAssert((inScreenPoints + inIndexIn)->y < inYFactor);			
//				UUmAssert((inScreenPoints + inIndexOut)->y >= inYFactor);
							
				t = (yOut - inYFactor) / (yOut - yIn);
				xNew = xOut + t * (xIn - xOut);						
				zNew = zOut + t * (zIn - zOut);						
				yNew = inYFactor - 0.0001f;		
				wNew = wOut + t * (wIn - wOut);						
				break;										
						
			case MScClipCode_PosZ:									
//				UUmAssert((inScreenPoints + inIndexIn)->z < 1.0);		
//				UUmAssert((inScreenPoints + inIndexOut)->z >= 1.0);	
					
				t = (zOut - 1.0f) / (zOut - zIn);					
				xNew = xOut + t * (xIn - xOut);						
				yNew = yOut + t * (yIn - yOut);						
				zNew = 0.9999f;
				wNew = wOut + t * (wIn - wOut);					
				break;										
						
			case MScClipCode_NegZ:									
//				UUmAssert((inScreenPoints + inIndexIn)->z >= 0.0f);		
//				UUmAssert((inScreenPoints + inIndexOut)->z < 0.0f);		
				
				t = (0.0f - zOut) / (zIn - zOut);					
				xNew = xOut + t * (xIn - xOut);						
				yNew = yOut + t * (yIn - yOut);						
				zNew = 0.0;											
				wNew = wOut + t * (wIn - wOut);						
				break;												
		}

		#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING
			
			if(gClipDumping)
			{
				fprintf(gClipFile, "\tt = %f\n", t);
			}
		
		#endif
		

		#if MSmGeomClipPoly_InterpShading
			
			// XXX - optimize this someday
			shadeOut = *(inScreenShades + inIndexOut);
			shadeIn = *(inScreenShades + inIndexIn);
			
			rOut = (float)((shadeOut >> 10) & 0x1F);
			gOut = (float)((shadeOut >> 5) & 0x1F);
			bOut = (float)((shadeOut >> 0) & 0x1F);
			
			rIn = (float)((shadeIn >> 10) & 0x1F);
			gIn = (float)((shadeIn >> 5) & 0x1F);
			bIn = (float)((shadeIn >> 0) & 0x1F);
			
			rNew = rOut + t * (rIn - rOut);
			gNew = gOut + t * (gIn - gOut);
			bNew = bOut + t * (bIn - bOut);
			
			*(inScreenShades + inIndexNew) = 
				((UUtUns16)rNew << 10) |
				((UUtUns16)gNew << 5) |
				((UUtUns16)bNew << 0);
			
		#endif

		#if MSmGeomClipPoly_TextureMap
			
			#if MSmGeomClipPoly_SplitTexturePoint
			{	
				float uIn, uOut, uNew;
				float vIn, vOut, vNew;
				
				inTexIndexNew <<= 1;
				inTexIndexOut <<= 1;
				inTexIndexIn <<= 1;
				uIn = (inTextureCoords + inTexIndexIn)->u;
				vIn = (inTextureCoords + inTexIndexIn)->v;
				uOut = (inTextureCoords + inTexIndexOut)->u;
				vOut = (inTextureCoords + inTexIndexOut)->v;
				uIn *= wIn;
				vIn *= wIn;
				uOut *= wOut;
				vOut *= wOut;
				
				uNew = uOut + t * (uIn - uOut);
				vNew = vOut + t * (vIn - vOut);
				uNew /= wNew;
				vNew /= wNew;
				
				(inTextureCoords + inTexIndexNew)->u = uNew;
				(inTextureCoords + inTexIndexNew)->v = vNew;

				inTexIndexNew += 1;
				inTexIndexOut += 1;
				inTexIndexIn += 1;

				uIn = (inTextureCoords + inTexIndexIn)->u;
				vIn = (inTextureCoords + inTexIndexIn)->v;
				uOut = (inTextureCoords + inTexIndexOut)->u;
				vOut = (inTextureCoords + inTexIndexOut)->v;
				uIn *= wIn;
				vIn *= wIn;
				uOut *= wOut;
				vOut *= wOut;
				
				uNew = uOut + t * (uIn - uOut);
				vNew = vOut + t * (vIn - vOut);
				uNew /= wNew;
				vNew /= wNew;
				
				(inTextureCoords + inTexIndexNew)->u = uNew;
				(inTextureCoords + inTexIndexNew)->v = vNew;

			}
			#else
			{
				float uIn, uOut, uNew;
				float vIn, vOut, vNew;
				
				uIn = (inTextureCoords + inTexIndexIn)->u;
				vIn = (inTextureCoords + inTexIndexIn)->v;
				uOut = (inTextureCoords + inTexIndexOut)->u;
				vOut = (inTextureCoords + inTexIndexOut)->v;
				uIn *= wIn;
				vIn *= wIn;
				uOut *= wOut;
				vOut *= wOut;
				
				uNew = uOut + t * (uIn - uOut);
				vNew = vOut + t * (vIn - vOut);
				uNew /= wNew;
				vNew /= wNew;
				
				(inTextureCoords + inTexIndexNew)->u = uNew;
				(inTextureCoords + inTexIndexNew)->v = vNew;
				
				//(inTextureCoords + inTexIndexNew)->u = (inTextureCoords + inTexIndexOut)->u + t * ((inTextureCoords + inTexIndexIn)->u - (inTextureCoords + inTexIndexOut)->u);
				//(inTextureCoords + inTexIndexNew)->v = (inTextureCoords + inTexIndexOut)->v + t * ((inTextureCoords + inTexIndexIn)->v - (inTextureCoords + inTexIndexOut)->v);
			
				#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING
					
					if(gClipDumping)
					{
						fprintf(gClipFile, "\tinTex: (%f, %f)\n",
							(inTextureCoords + inTexIndexIn)->u,
							(inTextureCoords + inTexIndexIn)->v);
						fprintf(gClipFile, "\toutTex: (%f, %f)\n",
							(inTextureCoords + inTexIndexOut)->u,
							(inTextureCoords + inTexIndexOut)->v);
						fprintf(gClipFile, "\tnewTex: (%f, %f)\n",
							(inTextureCoords + inTexIndexNew)->u,
							(inTextureCoords + inTexIndexNew)->v);
					}
				
				#endif
			}
			#endif
			
		#endif

		newClipCode = 0;										
		if(xNew < 0.0f)	newClipCode |= MScClipCode_NegX;		
		if(xNew >= inXFactor) newClipCode |= MScClipCode_PosX;
		if(yNew < 0.0f)	newClipCode |= MScClipCode_PosY;  /* This is not a bug */
		if(yNew >= inYFactor) newClipCode |= MScClipCode_NegY;  /* This is not a bug */
		if(zNew >= 1.0f) newClipCode |= MScClipCode_PosZ;		
		if(zNew < 0.0f)	newClipCode |= MScClipCode_NegZ;		
		(inScreenPoints + inIndexNew)->x = xNew;					
		(inScreenPoints + inIndexNew)->y = yNew;					
		(inScreenPoints + inIndexNew)->z = zNew;					
		(inScreenPoints + inIndexNew)->invW = wNew;			
																
		#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING
			
			if(gClipDumping)
			{
				fprintf(gClipFile, "\tinScreen: (%f, %f, %f, %f)\n",
					(inScreenPoints + inIndexIn)->x,
					(inScreenPoints + inIndexIn)->y,
					(inScreenPoints + inIndexIn)->z,
					(inScreenPoints + inIndexIn)->invW);
				fprintf(gClipFile, "\toutScreen: (%f, %f, %f, %f)\n",
					(inScreenPoints + inIndexOut)->x,
					(inScreenPoints + inIndexOut)->y,
					(inScreenPoints + inIndexOut)->z,
					(inScreenPoints + inIndexOut)->invW);
				fprintf(gClipFile, "\tnewScreen: (%f, %f, %f, %f)\n",
					(inScreenPoints + inIndexNew)->x,
					(inScreenPoints + inIndexNew)->y,
					(inScreenPoints + inIndexNew)->z,
					(inScreenPoints + inIndexNew)->invW);
				fprintf(gClipFile, "\tnewClipPlanes: ");
				MSiDump_ClipCodes(gClipFile, newClipCode);
				fprintf(gClipFile, "\n");
			}
		
		#endif

		MSrClipCode_ValidateScreen(								
			(inScreenPoints + inIndexNew),							
			newClipCode,										
			inXFactor,							
			inYFactor);								
		
		MSiVerifyPointScreen(inScreenPoints + inIndexNew);
		
	#endif
	
	*outClipCodeNew = newClipCode;
}

MSmGeomClipPoly_ReturnValue
MSmGeomClipPoly_TriFunctionName(
	MSmGeomClipPoly_ContextType*	inContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,

#undef inTexIndex0
#undef inTexIndex1
#undef inTexIndex2

#if MSmGeomClipPoly_SplitTexturePoint

	UUtUns16		inTexIndex0,
	UUtUns16		inTexIndex1,
	UUtUns16		inTexIndex2,

#else

	#define inTexIndex0	inVIndex0
	#define inTexIndex1	inVIndex1
	#define inTexIndex2	inVIndex2
	
#endif

	UUtUns16		inClipCode0,
	UUtUns16		inClipCode1,
	UUtUns16		inClipCode2,
	UUtUns16		inClippedPlanes
	
#if !MSmGeomClipPoly_BBox &&				\
	!MSmGeomClipPoly_SplitTexturePoint &&	\
		!MSmGeomClipPoly_InterpShading &&	\
		!MSmGeomClipPoly_BlockFrustum &&	\
		!MSmGeomClipPoly_CheckFrustum

	,
	
	UUtUns16		inShade

#endif

)
{

	MStGeomContextPrivate*	contextPrivate = (MStGeomContextPrivate *)inContext->privateContext;

	UUtUns16				newClipCode0;
	UUtUns16				newClipCode1;
	UUtUns16				newVIndex0;	
	UUtUns16				newVIndex1;	

#undef newTexIndex0
#undef newTexIndex1

#if MSmGeomClipPoly_SplitTexturePoint

	UUtUns16				newTexIndex0;
	UUtUns16				newTexIndex1;
	
#else

	#define newTexIndex0	newVIndex0
	#define newTexIndex1	newVIndex1

#endif

	#if MSmGeomClipPoly_BBox
	
		M3tPoint3D*				worldPoints;
		M3tBoundingBox_MinMax*	bBox;
	#else
	
		M3tPointScreen*			screenPoints;
	
	#endif
	
	#if MSmGeomClipPoly_InterpShading
	
		UUtUns16*				screenShades;
	
	#endif
	
	#if MSmGeomClipPoly_4D
	
		M3tPoint4D*			frustumPoints;
		float				scaleX, scaleY;
		
	#endif
	
	#if MSmGeomClipPoly_TextureMap
		
		#if 0//MSmGeomClipPoly_SplitTexturePoint
		
			M3tEnvTextureCoord*	textureCoords;
		
		#else
		
			M3tTextureCoord*	textureCoords;
		
		#endif
		
	#endif
	
	UUtInt16 i;
	UUtUns16 curClipPlane;

	#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING
	
		gClipNesting++;
		
		UUmAssert(gClipNesting < 10);
		UUmAssert(gClipNesting >= 0);
		
	#endif
	
	if(inClipCode0 & inClipCode1 & inClipCode2)
	{
		MSmGeomClipPoly_Return;
	}
	
	UUmAssert(contextPrivate->curClipData->newClipVertexIndex < contextPrivate->curClipData->maxClipVertices);
	
	newVIndex0 = contextPrivate->curClipData->newClipVertexIndex++;
	newVIndex1 = contextPrivate->curClipData->newClipVertexIndex++;
	
#if MSmGeomClipPoly_SplitTexturePoint

	UUmAssert(contextPrivate->curClipData->newClipTextureIndex < contextPrivate->curClipData->maxClipTextureCords);
	
	newTexIndex0 = contextPrivate->curClipData->newClipTextureIndex++;
	newTexIndex1 = contextPrivate->curClipData->newClipTextureIndex++;

#endif
	
	#if MSmGeomClipPoly_BBox
		
		worldPoints = contextPrivate->environment->pointArray->points;
		bBox = &contextPrivate->visBBox;
	
	#else
		
		screenPoints = contextPrivate->curClipData->screenPoints;
		
	#endif
	
	#if MSmGeomClipPoly_InterpShading
	
		screenShades = (UUtUns16*)contextPrivate->screenVertexShades;
	
	#endif
	
#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING

	if(gClipDumping)
	{
		fprintf(gClipFile, "Entering %s(%d)\n", UUmStringize(MSmGeomClipPoly_TriFunctionName), gClipNesting);
		fprintf(gClipFile, "\tIndices\n");
		fprintf(gClipFile, "\t\tv0: %d\n", inVIndex0);
		fprintf(gClipFile, "\t\tv1: %d\n", inVIndex1);
		fprintf(gClipFile, "\t\tv2: %d\n", inVIndex2);
		
		fprintf(gClipFile, "\tNew indices\n");
		fprintf(gClipFile, "\t\tnv0: %d\n", newVIndex0);
		fprintf(gClipFile, "\t\tnv1: %d\n", newVIndex1);

		fprintf(gClipFile, "\tClip Codes\n");
		fprintf(gClipFile, "\t\tv0: ");
		MSiDump_ClipCodes(gClipFile, inClipCode0);
		fprintf(gClipFile, "\n");
		fprintf(gClipFile, "\t\tv1: ");
		MSiDump_ClipCodes(gClipFile, inClipCode1);
		fprintf(gClipFile, "\n");
		fprintf(gClipFile, "\t\tv2: ");
		MSiDump_ClipCodes(gClipFile, inClipCode2);
		fprintf(gClipFile, "\n");

		fprintf(gClipFile, "\tscreen\n");
		fprintf(gClipFile, "\t\tv0: (%f, %f, %f, %f)\n",
			screenPoints[inVIndex0].x, screenPoints[inVIndex0].y, screenPoints[inVIndex0].z, screenPoints[inVIndex0].invW);
		fprintf(gClipFile, "\t\tv1: (%f, %f, %f, %f)\n",
			screenPoints[inVIndex1].x, screenPoints[inVIndex1].y, screenPoints[inVIndex1].z, screenPoints[inVIndex1].invW);
		fprintf(gClipFile, "\t\tv2: (%f, %f, %f, %f)\n",
			screenPoints[inVIndex2].x, screenPoints[inVIndex2].y, screenPoints[inVIndex2].z, screenPoints[inVIndex2].invW);
	}
	
#endif
	
	#if MSmGeomClipPoly_4D
		
		frustumPoints = contextPrivate->curClipData->frustumPoints;
		
		scaleX = contextPrivate->scaleX;
		scaleY = contextPrivate->scaleY;
		
		#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING
		
			if(gClipDumping)
			{
				fprintf(gClipFile, "\tfrustum\n");
				fprintf(gClipFile, "\t\tv0: (%f, %f, %f, %f)\n",
					frustumPoints[inVIndex0].x, frustumPoints[inVIndex0].y, frustumPoints[inVIndex0].z, frustumPoints[inVIndex0].w);
				fprintf(gClipFile, "\t\tv1: (%f, %f, %f, %f)\n",
					frustumPoints[inVIndex1].x, frustumPoints[inVIndex1].y, frustumPoints[inVIndex1].z, frustumPoints[inVIndex1].w);
				fprintf(gClipFile, "\t\tv2: (%f, %f, %f, %f)\n",
					frustumPoints[inVIndex2].x, frustumPoints[inVIndex2].y, frustumPoints[inVIndex2].z, frustumPoints[inVIndex2].w);
			}
			
		#endif

	#endif
	
	#if MSmGeomClipPoly_TextureMap
		
		#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING
		
			if(gClipDumping)
			{
				fprintf(gClipFile, "\tTexture indices\n");
				fprintf(gClipFile, "\t\tv0: %d\n", inTexIndex0);
				fprintf(gClipFile, "\t\tv1: %d\n", inTexIndex1);
				fprintf(gClipFile, "\t\tv2: %d\n", inTexIndex2);

				fprintf(gClipFile, "\tNew texture indices\n");
				fprintf(gClipFile, "\t\tnv0: %d\n", newTexIndex0);
				fprintf(gClipFile, "\t\tnv1: %d\n", newTexIndex1);
			}
		
		#endif
		
		#if MSmGeomClipPoly_SplitTexturePoint
		
			textureCoords = contextPrivate->environment->textureCoordArray->textureCoords;
		
		#else
		
			textureCoords = contextPrivate->textureCoords;
		
			#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING
			
				if(gClipDumping)
				{
					fprintf(gClipFile, "\ttexture coords\n");
					fprintf(gClipFile, "\t\tv0: (%f, %f)\n", 
						textureCoords[inTexIndex0].u, textureCoords[inTexIndex0].v);
					fprintf(gClipFile, "\t\tv1: (%f, %f)\n", 
						textureCoords[inTexIndex1].u, textureCoords[inTexIndex1].v);
					fprintf(gClipFile, "\t\tv2: (%f, %f)\n", 
						textureCoords[inTexIndex2].u, textureCoords[inTexIndex2].v);
				}
			
			#endif

		#endif
		

	#endif
	
	#if defined(DEBUGGING_CLIPPING_RANDCOLORS) && DEBUGGING_CLIPPING_RANDCOLORS
		
		#if !MSmGeomClipPoly_InterpShading
		
			inShade = rand() & 0xFFFF;
			
		#endif
		
	#endif
	
	#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING
	
		#if MSmGeomClipPoly_4D
		
			MSiVerifyPoint4D(
				frustumPoints + inVIndex0);
			MSiVerifyPoint4D(
				frustumPoints + inVIndex1);
			MSiVerifyPoint4D(
				frustumPoints + inVIndex2);

			MSrClipCode_ValidateFrustum(
				frustumPoints + inVIndex0,
				inClipCode0);
			MSrClipCode_ValidateFrustum(
				frustumPoints + inVIndex1,
				inClipCode1);
			MSrClipCode_ValidateFrustum(
				frustumPoints + inVIndex2,
				inClipCode2);
			
		#else
		
			MSiVerifyPointScreen(
				screenPoints + inVIndex0);
			MSiVerifyPointScreen(
				screenPoints + inVIndex1);
			MSiVerifyPointScreen(
				screenPoints + inVIndex2);
		
			MSrClipCode_ValidateScreen(
				screenPoints + inVIndex0,
				inClipCode0,
				(float)MSmGeomClipPoly_BufferWidth,
				(float)MSmGeomClipPoly_BufferHeight);
			MSrClipCode_ValidateScreen(
				screenPoints + inVIndex1,
				inClipCode1,
				(float)MSmGeomClipPoly_BufferWidth,
				(float)MSmGeomClipPoly_BufferHeight);
			MSrClipCode_ValidateScreen(
				screenPoints + inVIndex2,
				inClipCode2,
				(float)MSmGeomClipPoly_BufferWidth,
				(float)MSmGeomClipPoly_BufferHeight);

		#endif
	
	#endif
		
	for(i = 6; i-- > 0;)
	{
		curClipPlane = (UUtUns16)(1 << i);
		if(inClippedPlanes & curClipPlane)
		{
			continue;
		}
		
		if(curClipPlane & inClipCode0)								
		{														
			/* 0: Out, 1: ?, 2: ? */							
			if(curClipPlane & inClipCode1)							
			{													
				/* 0: Out, 1: Out, 2: In */						
				UUmAssert(!(curClipPlane & inClipCode2));			
																
				/* Compute 2 -> 0 intersection */				
				/* Compute 2 -> 1 intersection */				
				MSmGeomClipPoly_ComputeNewClipPoints(			
					inVIndex2, inVIndex0,						
					inVIndex2, inVIndex1,					
					inTexIndex2, inTexIndex0,						
					inTexIndex2, inTexIndex1);						
																
				/* Check for trivial accept on 2, new0, new1 */	
				MSmGeomClipPoly_CheckTrivialAcceptTri(			
					inClipCode2, newClipCode0, newClipCode1,	
					inVIndex2, newVIndex0, newVIndex1,			
					inTexIndex2, newTexIndex0, newTexIndex1,			
					inClippedPlanes | curClipPlane);				
																
				/* Verify trivial reject on 0, 1, new1, new0 */	
				MSmGeomClipPoly_VerifyTrivialRejectQuad(		
					inClipCode0,								
					inClipCode1,								
					(newClipCode1 | curClipPlane),					
					(newClipCode0 | curClipPlane));				
																
				MSmGeomClipPoly_Return;							
			}													
			else												
			{													
				/* 0: Out, 1: in, 2: ? */						
				if(curClipPlane & inClipCode2)						
				{												
					/* 0: Out, 1: In, 2: Out */					
																
					/* Compute 1 -> 0 Intersection */			
					/* Compute 1 -> 2 Intersection */			
					MSmGeomClipPoly_ComputeNewClipPoints(		
						inVIndex1, inVIndex0,					
						inVIndex1, inVIndex2,			
						inTexIndex1, inTexIndex0,					
						inTexIndex1, inTexIndex2);					
																
					/* Check for trivial accept on 1, new1, new0 */
					MSmGeomClipPoly_CheckTrivialAcceptTri(		
						inClipCode1, newClipCode1, newClipCode0,
						inVIndex1, newVIndex1, newVIndex0,		
						inTexIndex1, newTexIndex1, newTexIndex0,		
						inClippedPlanes | curClipPlane);			
																
					/* Verify trivial reject on 2, 0, new0, new1 */
					MSmGeomClipPoly_VerifyTrivialRejectQuad(	
						inClipCode2,							
						inClipCode0,							
						(newClipCode0 | curClipPlane),				
						(newClipCode1 | curClipPlane));			
																
					MSmGeomClipPoly_Return;						
				}												
				else											
				{												
					/* 0: Out, 1: In, 2: In */					
																
					/* Compute 1 -> 0 intersection */			
					/* Compute 2 -> 0 intersection */			
					MSmGeomClipPoly_ComputeNewClipPoints(		
						inVIndex1, inVIndex0,					
						inVIndex2, inVIndex0,				
						inTexIndex1, inTexIndex0,					
						inTexIndex2, inTexIndex0);					
																
					/* Check for trivial accept on 1, 2, new1, new0 */		
					MSmGeomClipPoly_CheckTrivialAcceptQuad(					
						inClipCode1, inClipCode2, newClipCode1, newClipCode0,
						inVIndex1, inVIndex2, newVIndex1, newVIndex0,		
						inTexIndex1, inTexIndex2, newTexIndex1, newTexIndex0,		
						inClippedPlanes | curClipPlane);			
																
					/* Verify trivial reject on 0, new0, new1 */
					MSmGeomClipPoly_VerifyTrivialRejectTri(		
						inClipCode0,							
						(newClipCode0 | curClipPlane),				
						(newClipCode1 | curClipPlane));			
																
					MSmGeomClipPoly_Return;						
				}												
			}
		}
		else													
		{														
			/* 0: In, 1: ?, 2: ? */								
			if(curClipPlane & inClipCode1)							
			{													
				/* 0: In, 1: Out, 2: ? */						
				if(curClipPlane & inClipCode2)						
				{												
					/* 0: In, 1: Out, 2: Out */					
																
					/* Compute 0 -> 1 intersection */			
					/* Compute 0 -> 2 intersection */			
					MSmGeomClipPoly_ComputeNewClipPoints(		
						inVIndex0, inVIndex1,					
						inVIndex0, inVIndex2,				
						inTexIndex0, inTexIndex1,					
						inTexIndex0, inTexIndex2);					
																
					/* Check for trivial accept on 0, new0, new1 */
					MSmGeomClipPoly_CheckTrivialAcceptTri(		
						inClipCode0, newClipCode0, newClipCode1,
						inVIndex0, newVIndex0, newVIndex1,		
						inTexIndex0, newTexIndex0, newTexIndex1,		
						inClippedPlanes | curClipPlane);			
																
					/* Verfiy trivial reject on 1, 2, new1, new0 */
					MSmGeomClipPoly_VerifyTrivialRejectQuad(	
						inClipCode1,							
						inClipCode2,							
						(newClipCode1 | curClipPlane),				
						(newClipCode0 | curClipPlane));			
																
					MSmGeomClipPoly_Return;						
				}												
				else											
				{												
					/* 0: In, 1: Out, 2: In */					
																
					/* Compute 0 -> 1 intersection */			
					/* Compute 2 -> 1 intersection */			
					MSmGeomClipPoly_ComputeNewClipPoints(		
						inVIndex0, inVIndex1,					
						inVIndex2, inVIndex1,				
						inTexIndex0, inTexIndex1,					
						inTexIndex2, inTexIndex1);					
																
					/* Check for trivial accept on 2, 0, new0, new1 */		
					MSmGeomClipPoly_CheckTrivialAcceptQuad(					
						inClipCode2, inClipCode0, newClipCode0, newClipCode1,
						inVIndex2, inVIndex0, newVIndex0, newVIndex1,		
						inTexIndex2, inTexIndex0, newTexIndex0, newTexIndex1,		
						inClippedPlanes | curClipPlane);			
																
					/* Verify trivial reject on 1, new1, new0 */
					MSmGeomClipPoly_VerifyTrivialRejectTri(		
						inClipCode1,							
						(newClipCode1 | curClipPlane),				
						(newClipCode0 | curClipPlane));			
																
					MSmGeomClipPoly_Return;						
				}												
																
			}													
			else												
			{													
				/* 0: In, 1: in, 2: ? */						
				if(curClipPlane & inClipCode2)						
				{												
					/* 0: In, 1: in, 2: Out */					
																
					/* Compute 0 -> 2 intersection */			
					/* Compute 1 -> 2 intersection */			
					MSmGeomClipPoly_ComputeNewClipPoints(		
						inVIndex0, inVIndex2,					
						inVIndex1, inVIndex2,			
						inTexIndex0, inTexIndex2,					
						inTexIndex1, inTexIndex2);					
																
					/* Check for trivial accept on 0, 1, new1, new0 */		
					MSmGeomClipPoly_CheckTrivialAcceptQuad(					
						inClipCode0, inClipCode1, newClipCode1, newClipCode0,
						inVIndex0, inVIndex1, newVIndex1, newVIndex0,		
						inTexIndex0, inTexIndex1, newTexIndex1, newTexIndex0,		
						inClippedPlanes | curClipPlane);			
																
					/* Verify trivial reject on 2, new0, new1 */
					MSmGeomClipPoly_VerifyTrivialRejectTri(		
						inClipCode2,							
						(newClipCode0 | curClipPlane),				
						(newClipCode1 | curClipPlane));			
																
					MSmGeomClipPoly_Return;						
				}												
				/* else all are in */							
			}													
		}
	}

//	UUmAssert(!"Should never get here");

	MSmGeomClipPoly_Return;
}

MSmGeomClipPoly_ReturnValue
MSmGeomClipPoly_QuadFunctionName(
	MSmGeomClipPoly_ContextType*	inContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inVIndex3,

#undef inTexIndex0
#undef inTexIndex1
#undef inTexIndex2
#undef inTexIndex3

#if MSmGeomClipPoly_SplitTexturePoint

	UUtUns16		inTexIndex0,
	UUtUns16		inTexIndex1,
	UUtUns16		inTexIndex2,
	UUtUns16		inTexIndex3,

#else

	#define inTexIndex0	inVIndex0
	#define inTexIndex1	inVIndex1
	#define inTexIndex2	inVIndex2
	#define inTexIndex3	inVIndex3
	
#endif

	UUtUns16		inClipCode0,
	UUtUns16		inClipCode1,
	UUtUns16		inClipCode2,
	UUtUns16		inClipCode3,
	UUtUns16		inClippedPlanes
	
#if !MSmGeomClipPoly_BBox &&				\
	!MSmGeomClipPoly_SplitTexturePoint &&	\
	!MSmGeomClipPoly_InterpShading &&		\
	!MSmGeomClipPoly_BlockFrustum &&		\
	!MSmGeomClipPoly_CheckFrustum

	,
	
	UUtUns16		inShade

#endif

)
{

	MStGeomContextPrivate*	contextPrivate = (MStGeomContextPrivate *)inContext->privateContext;

	UUtUns16				newClipCode0;
	UUtUns16				newClipCode1;
	UUtUns16				newVIndex0;	
	UUtUns16				newVIndex1;	

#if MSmGeomClipPoly_SplitTexturePoint

	UUtUns16				newTexIndex0;
	UUtUns16				newTexIndex1;
	
#else

	#define newTexIndex0	newVIndex0
	#define newTexIndex1	newVIndex1

#endif

	#if MSmGeomClipPoly_BBox
	
		M3tPoint3D*				worldPoints;
		M3tBoundingBox_MinMax*	bBox;
	#else
	
		M3tPointScreen*			screenPoints;
	
	#endif
	
	#if MSmGeomClipPoly_InterpShading
	
		UUtUns16*			screenShades;
		
	#endif
	
	#if MSmGeomClipPoly_4D
	
		M3tPoint4D*			frustumPoints;
		float				scaleX, scaleY;
		
	#endif

	#if MSmGeomClipPoly_TextureMap
		
		#if 0//MSmGeomClipPoly_SplitTexturePoint
		
			M3tEnvTextureCoord*	textureCoords;
		
		#else
		
			M3tTextureCoord*	textureCoords;
		
		#endif
		
	#endif

	UUtInt16	i;
	UUtUns16	curClipPlane;

	#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING
	
		gClipNesting++;
		
		UUmAssert(gClipNesting < 10);
		UUmAssert(gClipNesting >= 0);
		
	#endif
	
	if(inClipCode0 & inClipCode1 & inClipCode2 & inClipCode3)
	{
		MSmGeomClipPoly_Return;
	}
	
	UUmAssert(contextPrivate->curClipData->newClipVertexIndex < contextPrivate->curClipData->maxClipVertices);
	
	newVIndex0 = contextPrivate->curClipData->newClipVertexIndex++;
	newVIndex1 = contextPrivate->curClipData->newClipVertexIndex++;
	
#if MSmGeomClipPoly_SplitTexturePoint

	UUmAssert(contextPrivate->curClipData->newClipTextureIndex < contextPrivate->curClipData->maxClipTextureCords);
	
	newTexIndex0 = contextPrivate->curClipData->newClipTextureIndex++;
	newTexIndex1 = contextPrivate->curClipData->newClipTextureIndex++;

#endif

	#if MSmGeomClipPoly_BBox
		
		worldPoints = contextPrivate->environment->pointArray->points;
		bBox = &contextPrivate->visBBox;
		
	#else
		
		screenPoints = contextPrivate->curClipData->screenPoints;
		
	#endif

	#if MSmGeomClipPoly_InterpShading
		
		screenShades = (UUtUns16*)contextPrivate->screenVertexShades;
		
	#endif
	
	#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING

		if(gClipDumping)
		{
			fprintf(gClipFile, "Entering %s(%d)\n", UUmStringize(MSmGeomClipPoly_QuadFunctionName), gClipNesting);
			
			fprintf(gClipFile, "\tIndices\n");
			fprintf(gClipFile, "\t\tv0: %d\n", inVIndex0);
			fprintf(gClipFile, "\t\tv1: %d\n", inVIndex1);
			fprintf(gClipFile, "\t\tv2: %d\n", inVIndex2);
			fprintf(gClipFile, "\t\tv3: %d\n", inVIndex3);
			
			fprintf(gClipFile, "\tNew indices\n");
			fprintf(gClipFile, "\t\tnv0: %d\n", newVIndex0);
			fprintf(gClipFile, "\t\tnv1: %d\n", newVIndex1);
			
			fprintf(gClipFile, "\tClip Codes\n");
			fprintf(gClipFile, "\t\tv0: ");
			MSiDump_ClipCodes(gClipFile, inClipCode0);
			fprintf(gClipFile, "\n");
			fprintf(gClipFile, "\t\tv1: ");
			MSiDump_ClipCodes(gClipFile, inClipCode1);
			fprintf(gClipFile, "\n");
			fprintf(gClipFile, "\t\tv2: ");
			MSiDump_ClipCodes(gClipFile, inClipCode2);
			fprintf(gClipFile, "\n");
			fprintf(gClipFile, "\t\tv3: ");
			MSiDump_ClipCodes(gClipFile, inClipCode3);
			fprintf(gClipFile, "\n");
			
			fprintf(gClipFile, "\tscreen\n");
			fprintf(gClipFile, "\t\tv0: (%f, %f, %f, %f)\n",
				screenPoints[inVIndex0].x, screenPoints[inVIndex0].y, screenPoints[inVIndex0].z, screenPoints[inVIndex0].invW);
			fprintf(gClipFile, "\t\tv1: (%f, %f, %f, %f)\n",
				screenPoints[inVIndex1].x, screenPoints[inVIndex1].y, screenPoints[inVIndex1].z, screenPoints[inVIndex1].invW);
			fprintf(gClipFile, "\t\tv2: (%f, %f, %f, %f)\n",
				screenPoints[inVIndex2].x, screenPoints[inVIndex2].y, screenPoints[inVIndex2].z, screenPoints[inVIndex2].invW);
			fprintf(gClipFile, "\t\tv3: (%f, %f, %f, %f)\n",
				screenPoints[inVIndex3].x, screenPoints[inVIndex3].y, screenPoints[inVIndex3].z, screenPoints[inVIndex3].invW);
		}
		
	#endif

	#if MSmGeomClipPoly_4D
		
		
		frustumPoints = contextPrivate->curClipData->frustumPoints;
		
		scaleX = contextPrivate->scaleX;
		scaleY = contextPrivate->scaleY;
		
		#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING
		
			if(gClipDumping)
			{
				fprintf(gClipFile, "\tfrustum\n");
				fprintf(gClipFile, "\t\tv0: (%f, %f, %f, %f)\n",
					frustumPoints[inVIndex0].x, frustumPoints[inVIndex0].y, frustumPoints[inVIndex0].z, frustumPoints[inVIndex0].w);
				fprintf(gClipFile, "\t\tv1: (%f, %f, %f, %f)\n",
					frustumPoints[inVIndex1].x, frustumPoints[inVIndex1].y, frustumPoints[inVIndex1].z, frustumPoints[inVIndex1].w);
				fprintf(gClipFile, "\t\tv2: (%f, %f, %f, %f)\n",
					frustumPoints[inVIndex2].x, frustumPoints[inVIndex2].y, frustumPoints[inVIndex2].z, frustumPoints[inVIndex2].w);
				fprintf(gClipFile, "\t\tv3: (%f, %f, %f, %f)\n",
					frustumPoints[inVIndex3].x, frustumPoints[inVIndex3].y, frustumPoints[inVIndex3].z, frustumPoints[inVIndex3].w);
			}
			
		#endif

	#endif
	
	#if MSmGeomClipPoly_TextureMap
		
		#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING
			
			if(gClipDumping)
			{
				fprintf(gClipFile, "\tTexture indices\n");
				fprintf(gClipFile, "\t\tv0: %d\n", inTexIndex0);
				fprintf(gClipFile, "\t\tv1: %d\n", inTexIndex1);
				fprintf(gClipFile, "\t\tv2: %d\n", inTexIndex2);
				fprintf(gClipFile, "\t\tv3: %d\n", inTexIndex3);

				fprintf(gClipFile, "\tNew texture indices\n");
				fprintf(gClipFile, "\t\tnv0: %d\n", newTexIndex0);
				fprintf(gClipFile, "\t\tnv1: %d\n", newTexIndex1);
			}
		
		#endif
		
		#if MSmGeomClipPoly_SplitTexturePoint
		
			textureCoords = contextPrivate->environment->textureCoordArray->textureCoords;
		
		#else
		
			textureCoords = contextPrivate->textureCoords;
		
			#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING
			
				if(gClipDumping)
				{
					fprintf(gClipFile, "\ttexture coords\n");
					fprintf(gClipFile, "\t\tv0: (%f, %f)\n", 
						textureCoords[inTexIndex0].u, textureCoords[inTexIndex0].v);
					fprintf(gClipFile, "\t\tv1: (%f, %f)\n", 
						textureCoords[inTexIndex1].u, textureCoords[inTexIndex1].v);
					fprintf(gClipFile, "\t\tv2: (%f, %f)\n", 
						textureCoords[inTexIndex2].u, textureCoords[inTexIndex2].v);
					fprintf(gClipFile, "\t\tv3: (%f, %f)\n", 
						textureCoords[inTexIndex3].u, textureCoords[inTexIndex3].v);
				}
			
			#endif
		
		#endif
		
	#endif
	
	#if defined(DEBUGGING_CLIPPING_RANDCOLORS) && DEBUGGING_CLIPPING_RANDCOLORS
		
		#if !MSmGeomClipPoly_InterpShading
		
			inShade = rand() & 0xFFFF;
			
		#endif
		
	#endif
	
	#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING
	
		#if MSmGeomClipPoly_4D
		
			MSiVerifyPoint4D(
				frustumPoints + inVIndex0);
			MSiVerifyPoint4D(
				frustumPoints + inVIndex1);
			MSiVerifyPoint4D(
				frustumPoints + inVIndex2);
			MSiVerifyPoint4D(
				frustumPoints + inVIndex3);

			MSrClipCode_ValidateFrustum(
				frustumPoints + inVIndex0,
				inClipCode0);
			MSrClipCode_ValidateFrustum(
				frustumPoints + inVIndex1,
				inClipCode1);
			MSrClipCode_ValidateFrustum(
				frustumPoints + inVIndex2,
				inClipCode2);
			MSrClipCode_ValidateFrustum(
				frustumPoints + inVIndex3,
				inClipCode3);
			
		#else
		
			MSiVerifyPointScreen(
				screenPoints + inVIndex0);
			MSiVerifyPointScreen(
				screenPoints + inVIndex1);
			MSiVerifyPointScreen(
				screenPoints + inVIndex2);
			MSiVerifyPointScreen(
				screenPoints + inVIndex3);
		
			MSrClipCode_ValidateScreen(
				screenPoints + inVIndex0,
				inClipCode0,
				(float)MSmGeomClipPoly_BufferWidth,
				(float)MSmGeomClipPoly_BufferHeight);
			MSrClipCode_ValidateScreen(
				screenPoints + inVIndex1,
				inClipCode1,
				(float)MSmGeomClipPoly_BufferWidth,
				(float)MSmGeomClipPoly_BufferHeight);
			MSrClipCode_ValidateScreen(
				screenPoints + inVIndex2,
				inClipCode2,
				(float)MSmGeomClipPoly_BufferWidth,
				(float)MSmGeomClipPoly_BufferHeight);
			MSrClipCode_ValidateScreen(
				screenPoints + inVIndex3,
				inClipCode3,
				(float)MSmGeomClipPoly_BufferWidth,
				(float)MSmGeomClipPoly_BufferHeight);

		#endif
	
	#endif
		
	for(i = 6; i-- > 0;)
	{
		curClipPlane = (UUtUns16)(1 << i);
		if(inClippedPlanes & curClipPlane)
		{
			continue;
		}
		
		if(curClipPlane & inClipCode0)										
		{																
			/* 0: Out, 1: ?, 2: ?, 3: ? */								
			if(curClipPlane & inClipCode1)									
			{															
				/* 0: Out, 1: Out, 2: ?, 3: ? */						
				if(curClipPlane & inClipCode2)								
				{														
					/* 0: Out, 1: Out, 2: Out, 3: In */					
					//UUmAssert(!(curClipPlane & inClipCode3));		
					/* Compute 3 -> 0 intersection */					
					/* Compute 3 -> 2 intersection */					
					MSmGeomClipPoly_ComputeNewClipPoints(				
						inVIndex3, inVIndex0,							
						inVIndex3, inVIndex2,				
						inTexIndex3, inTexIndex0,							
						inTexIndex3, inTexIndex2);							
					/* Check for trivial accept on 3, new0, new1 */		
					MSmGeomClipPoly_CheckTrivialAcceptTri(				
						inClipCode3, newClipCode0, newClipCode1,		
						inVIndex3, newVIndex0, newVIndex1,				
						inTexIndex3, newTexIndex0, newTexIndex1,				
						inClippedPlanes | curClipPlane);					
					/* Verify trivial reject on 0, 1, 2, new1, new0 */	
					MSmGeomClipPoly_VerifyTrivialRejectPent(			
						inClipCode0,									
						inClipCode1,									
						inClipCode2,									
						(newClipCode1 | curClipPlane),						
						(newClipCode0 | curClipPlane));	
						
					MSmGeomClipPoly_Return;
				}														
				else													
				{														
					/* 0: Out, 1: Out, 2: In, 3: ? */					
					if(curClipPlane & inClipCode3)							
					{													
						/* 0: Out, 1: Out, 2: In, 3: Out */				
						/* Compute 2 -> 1 intersection */				
						/* Compute 2 -> 3 intersection */				
						MSmGeomClipPoly_ComputeNewClipPoints(			
							inVIndex2, inVIndex1,						
							inVIndex2, inVIndex3,				
							inTexIndex2, inTexIndex1,						
							inTexIndex2, inTexIndex3);						
						/* Check for trivial accept on 2, new1, new0 */	
						MSmGeomClipPoly_CheckTrivialAcceptTri(			
							inClipCode2, newClipCode1, newClipCode0,	
							inVIndex2, newVIndex1, newVIndex0,			
							inTexIndex2, newTexIndex1, newTexIndex0,			
							inClippedPlanes | curClipPlane);				
						/* Verify trivial reject on 3, 0, 1, new0, new1 */
						MSmGeomClipPoly_VerifyTrivialRejectPent(		
							inClipCode3,								
							inClipCode0,								
							inClipCode1,								
							(newClipCode0 | curClipPlane),					
							(newClipCode1 | curClipPlane));				

						MSmGeomClipPoly_Return;
					}													
					else												
					{													
						/* 0: Out, 1: Out, 2: In, 3: In */				
						/* Compute 2 -> 1 intersection */				
						/* Compute 3 -> 0 intersection */				
						MSmGeomClipPoly_ComputeNewClipPoints(			
							inVIndex2, inVIndex1,						
							inVIndex3, inVIndex0,						
							inTexIndex2, inTexIndex1,						
							inTexIndex3, inTexIndex0);						
						/* Check for trivial accept on 2, 3, new1, new0 */
						MSmGeomClipPoly_CheckTrivialAcceptQuad(			
							inClipCode2, inClipCode3, newClipCode1, newClipCode0,
							inVIndex2, inVIndex3, newVIndex1, newVIndex0,		
							inTexIndex2, inTexIndex3, newTexIndex1, newTexIndex0,		
							inClippedPlanes | curClipPlane);				
						/* Verify trivial reject on 0, 1, new0, new1 */	
						MSmGeomClipPoly_VerifyTrivialRejectQuad(		
							inClipCode0,								
							inClipCode1,								
							(newClipCode0 | curClipPlane),					
							(newClipCode1 | curClipPlane));				

						MSmGeomClipPoly_Return;
					}													
				}														
			}															
			else														
			{															
				/* 0: Out, 1: In, 2: ?, 3: ? */							
				if(curClipPlane & inClipCode2)								
				{														
					/* 0: Out, 1: In, 2: Out, 3: Out */					
					//UUmAssert(curClipPlane & inClipCode3);				
					/* Compute 1 -> 0 intersection */					
					/* Compute 1 -> 2 intersection */					
					MSmGeomClipPoly_ComputeNewClipPoints(				
						inVIndex1, inVIndex0,							
						inVIndex1, inVIndex2,						
						inTexIndex1, inTexIndex0,							
						inTexIndex1, inTexIndex2);							
					/* Check for trivial accept on 1, new1, new0 */		
					MSmGeomClipPoly_CheckTrivialAcceptTri(				
						inClipCode1, newClipCode1, newClipCode0,		
						inVIndex1, newVIndex1, newVIndex0,				
						inTexIndex1, newTexIndex1, newTexIndex0,				
						inClippedPlanes | curClipPlane);					
					/* Check for trivial reject on 2, 3, 0, new0, new1 */
					MSmGeomClipPoly_VerifyTrivialRejectPent(			
						inClipCode2,									
						inClipCode3,									
						inClipCode0,									
						(newClipCode0 | curClipPlane),						
						(newClipCode1 | curClipPlane));					

					MSmGeomClipPoly_Return;
				}														
				else													
				{														
					/* 0: Out, 1: In, 2: In, 3: ? */					
					if(curClipPlane & inClipCode3)							
					{													
						/* 0: Out, 1: In, 2: In, 3: Out */				
						/* Compute 1 -> 0 intersection */				
						/* Compute 2 -> 3 intersection */				
						MSmGeomClipPoly_ComputeNewClipPoints(			
							inVIndex1, inVIndex0,						
							inVIndex2, inVIndex3,						
							inTexIndex1, inTexIndex0,						
							inTexIndex2, inTexIndex3);						
						/* Check for trivial accept on 1, 2, new1, new0 */
						MSmGeomClipPoly_CheckTrivialAcceptQuad(			
							inClipCode1, inClipCode2, newClipCode1, newClipCode0,
							inVIndex1, inVIndex2, newVIndex1, newVIndex0,		
							inTexIndex1, inTexIndex2, newTexIndex1, newTexIndex0,		
							inClippedPlanes | curClipPlane);				
						/* Verify trivial reject on 3, 0, new0, new1 */	
						MSmGeomClipPoly_VerifyTrivialRejectQuad(		
							inClipCode3,								
							inClipCode0,								
							(newClipCode0 | curClipPlane),					
							(newClipCode1 | curClipPlane));				

						MSmGeomClipPoly_Return;
					}													
					else												
					{													
						/* 0: Out, 1: In, 2: In, 3: In */				
						/* Compute 1 -> 0 intersection */				
						/* Compute 3 -> 0 intersection */				
						MSmGeomClipPoly_ComputeNewClipPoints(			
							inVIndex1, inVIndex0,						
							inVIndex3, inVIndex0,					
							inTexIndex1, inTexIndex0,						
							inTexIndex3, inTexIndex0);						
						/* Check for trivial accept on 1, 2, 3, new1, new0 */
						MSmGeomClipPoly_CheckTrivialAcceptPent(			
							inClipCode1, inClipCode2, inClipCode3, newClipCode1, newClipCode0,
							inVIndex1, inVIndex2, inVIndex3, newVIndex1, newVIndex0,		
							inTexIndex1, inTexIndex2, inTexIndex3, newTexIndex1, newTexIndex0,		
							inClippedPlanes | curClipPlane);				
						/* Verify trivial reject on 0, new0, new1 */	
						MSmGeomClipPoly_VerifyTrivialRejectTri(			
							inClipCode0,								
							(newClipCode0 | curClipPlane),					
							(newClipCode1 | curClipPlane));				

						MSmGeomClipPoly_Return;
					}													
				}														
			}															
		}																
		else															
		{																
			/* 0: In, 1: ?, 2: ?, 3: ? */								
			if(curClipPlane & inClipCode1)									
			{															
				/* 0: In, 1: Out, 2: ?, 3: ? */							
				if(curClipPlane & inClipCode2)								
				{														
					/* 0: In, 1: Out, 2: Out, 3: ? */					
					if(curClipPlane & inClipCode3)							
					{													
						/* 0: In, 1: Out, 2: Out, 3: Out */				
						/* Compute 0 -> 3 intersection */				
						/* Compute 0 -> 1 intersection */				
						MSmGeomClipPoly_ComputeNewClipPoints(			
							inVIndex0, inVIndex3,						
							inVIndex0, inVIndex1,						
							inTexIndex0, inTexIndex3,						
							inTexIndex0, inTexIndex1);						
						/* Check for trivial accept on 0, new1, new0 */	
						MSmGeomClipPoly_CheckTrivialAcceptTri(			
							inClipCode0, newClipCode1, newClipCode0,	
							inVIndex0, newVIndex1, newVIndex0,			
							inTexIndex0, newTexIndex1, newTexIndex0,			
							inClippedPlanes | curClipPlane);				
						/* Verify trivial reject on 1, 2, 3, new0, new1 */
						MSmGeomClipPoly_VerifyTrivialRejectPent(		
							inClipCode1,								
							inClipCode2,								
							inClipCode3,								
							(newClipCode0 | curClipPlane),					
							(newClipCode1 | curClipPlane));				

						MSmGeomClipPoly_Return;
					}													
					else												
					{													
						/* 0: In, 1: Out, 2: Out, 3: In */				
						/* Compute 3 -> 2 intersection */				
						/* Compute 0 -> 1 intersection */				
						MSmGeomClipPoly_ComputeNewClipPoints(			
							inVIndex3, inVIndex2,						
							inVIndex0, inVIndex1,					
							inTexIndex3, inTexIndex2,						
							inTexIndex0, inTexIndex1);						
						/* Check for trivial accept on 3, 0, new1, new0 */
						MSmGeomClipPoly_CheckTrivialAcceptQuad(			
							inClipCode3, inClipCode0, newClipCode1, newClipCode0,
							inVIndex3, inVIndex0, newVIndex1, newVIndex0,		
							inTexIndex3, inTexIndex0, newTexIndex1, newTexIndex0,		
							inClippedPlanes | curClipPlane);				
						/* Verify trivial reject on 1, 2, new0, new1 */	
						MSmGeomClipPoly_VerifyTrivialRejectQuad(		
							inClipCode1,								
							inClipCode2,								
							(newClipCode0 | curClipPlane),					
							(newClipCode1 | curClipPlane));				

						MSmGeomClipPoly_Return;
					}													
				}														
				else													
				{														
					/* 0: In, 1: Out, 2: In, 3: In */					
					//UUmAssert(!(curClipPlane & inClipCode3));		
					/* Compute 2 -> 1 intersection */					
					/* Compute 0 -> 1 intersection */					
					MSmGeomClipPoly_ComputeNewClipPoints(				
						inVIndex2, inVIndex1,							
						inVIndex0, inVIndex1,							
						inTexIndex2, inTexIndex1,							
						inTexIndex0, inTexIndex1);							
					/* Check for trivial accept on 2, 3, 0, new1, new0 */
					MSmGeomClipPoly_CheckTrivialAcceptPent(				
						inClipCode2, inClipCode3, inClipCode0, newClipCode1, newClipCode0,
						inVIndex2, inVIndex3, inVIndex0, newVIndex1, newVIndex0,		
						inTexIndex2, inTexIndex3, inTexIndex0, newTexIndex1, newTexIndex0,		
						inClippedPlanes | curClipPlane);					
					/* Verify trivial reject on 1, new0, new1 */		
					MSmGeomClipPoly_VerifyTrivialRejectTri(				
						inClipCode1,									
						(newClipCode0 | curClipPlane),						
						(newClipCode1 | curClipPlane));					

					MSmGeomClipPoly_Return;
				}														
			}															
			else														
			{															
				/* 0: In, 1: In, 2: ?, 3: ? */							
				if(curClipPlane & inClipCode2)								
				{														
					/* 0: In, 1: In, 2: Out, 3: ? */					
					if(curClipPlane & inClipCode3)							
					{													
						/* 0: In, 1: In, 2: Out, 3: Out */				
						/* Compute 0 -> 3 intersection */				
						/* Compute 1 -> 2 intersection */				
						MSmGeomClipPoly_ComputeNewClipPoints(			
							inVIndex0, inVIndex3,						
							inVIndex1, inVIndex2,					
							inTexIndex0, inTexIndex3,						
							inTexIndex1, inTexIndex2);						
						/* Check for trivial accept on 0, 1, new1, new0 */
						MSmGeomClipPoly_CheckTrivialAcceptQuad(			
							inClipCode0, inClipCode1, newClipCode1, newClipCode0,
							inVIndex0, inVIndex1, newVIndex1, newVIndex0,		
							inTexIndex0, inTexIndex1, newTexIndex1, newTexIndex0,		
							inClippedPlanes | curClipPlane);						
						/* Verify trivial reject on 2, 3, new0, new1 */	
						MSmGeomClipPoly_VerifyTrivialRejectQuad(		
							inClipCode2,								
							inClipCode3,								
							(newClipCode0 | curClipPlane),					
							(newClipCode1 | curClipPlane));				

						MSmGeomClipPoly_Return;
					}													
					else												
					{													
						/* 0: In, 1: In, 2: Out, 3: In */				
						/* Compute 3 -> 2 intersection */				
						/* Compute 1 -> 2 intersection */				
						MSmGeomClipPoly_ComputeNewClipPoints(			
							inVIndex3, inVIndex2,						
							inVIndex1, inVIndex2,						
							inTexIndex3, inTexIndex2,						
							inTexIndex1, inTexIndex2);						
						/* Check for trivial accept on 3, 0, 1, new1, new0 */
						MSmGeomClipPoly_CheckTrivialAcceptPent(			
							inClipCode3, inClipCode0, inClipCode1, newClipCode1, newClipCode0,
							inVIndex3, inVIndex0, inVIndex1, newVIndex1, newVIndex0,		
							inTexIndex3, inTexIndex0, inTexIndex1, newTexIndex1, newTexIndex0,		
							inClippedPlanes | curClipPlane);				
						/* Verify trivial reject on 2, new0, new1 */	
						MSmGeomClipPoly_VerifyTrivialRejectTri(			
							inClipCode2,								
							(newClipCode0 | curClipPlane),					
							(newClipCode1 | curClipPlane));				

						MSmGeomClipPoly_Return;
					}													
				}														
				else													
				{														
					/* 0: In, 1: In, 2: In, 3: ? */						
					if(curClipPlane & inClipCode3)							
					{													
						/* 0: In, 1: In, 2: In, 3: Out */				
						/* Compute 0 -> 3 intersection */				
						/* Compute 2 -> 3 intersection */				
						MSmGeomClipPoly_ComputeNewClipPoints(			
							inVIndex0, inVIndex3,						
							inVIndex2, inVIndex3,					
							inTexIndex0, inTexIndex3,						
							inTexIndex2, inTexIndex3);						
						/* Check for trivial accept on 0, 1, 2, new1, new0 */
						MSmGeomClipPoly_CheckTrivialAcceptPent(			
							inClipCode0, inClipCode1, inClipCode2, newClipCode1, newClipCode0,
							inVIndex0, inVIndex1, inVIndex2, newVIndex1, newVIndex0,		
							inTexIndex0, inTexIndex1, inTexIndex2, newTexIndex1, newTexIndex0,		
							inClippedPlanes | curClipPlane);				
						/* Verify trivial reject on 3, new0, new1 */	
						MSmGeomClipPoly_VerifyTrivialRejectTri(			
							inClipCode3,								
							(newClipCode0 | curClipPlane),					
							(newClipCode1 | curClipPlane));				

						MSmGeomClipPoly_Return;
					}													
					/* else all are in */								
				}														
			}															
		}
	}

	//UUmAssert(!"Should never get here");
	
	MSmGeomClipPoly_Return;
}
