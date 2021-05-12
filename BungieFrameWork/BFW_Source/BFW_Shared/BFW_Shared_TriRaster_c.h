/*
	FILE:	MS_TriRaster_c.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: April 3, 1997
	
	PURPOSE:  Assert interface files
	
	Copyright 1997

*/

#if !defined(MSmTriRaster_FunctionName) || \
	!defined(MSmTriRaster_Texture) || \
	!defined(MSmTriRaster_TextureNoShade) || \
	!defined(MSmTriRaster_InterpShading) || \
	!defined(MSmTriRaster_Alpha) || \
	!defined(MSmTriRaster_Dither) || \
	!defined(MSmTriRaster_RGB565) || \
	!defined(MSmTriRaster_IsVisible) || \
	!defined(MSmTriRaster_PixelTouch) || \
	!defined(MSmTriRaster_RasterZOnly) || \
	!defined(MSmTriRaster_Split) || \
	!defined(MSmTriRaster_Lightmap)
	
	#error some macros have not been defined
	
#endif

#undef MSmTriRaster_ReturnType
#undef MSmTriRaster_ContextType
#undef MSmTriRaster_BufferWidth
#undef MSmTriRaster_BufferHeight

#if MSmTriRaster_IsVisible

	#define MSmTriRaster_ReturnType		UUtBool
	#define MSmTriRaster_ContextType	AKtEnvironment
	#define MSmTriRaster_BufferWidth	AKcPreviewBufferWidth
	#define MSmTriRaster_BufferHeight	AKcPreviewBufferHeight

#elif MSmTriRaster_RasterZOnly
	
	#define MSmTriRaster_ReturnType		void
	#define MSmTriRaster_ContextType	AKtEnvironment
	#define MSmTriRaster_BufferWidth	AKcPreviewBufferWidth
	#define MSmTriRaster_BufferHeight	AKcPreviewBufferHeight

#elif MSmTriRaster_Lightmap

	#define MSmTriRaster_ReturnType		void
	#define MSmTriRaster_ContextType	tLightmap
	#define MSmTriRaster_BufferWidth	contextPrivate->width
	#define MSmTriRaster_BufferHeight	contextPrivate->height

#else

	#define MSmTriRaster_ReturnType		void
	#define MSmTriRaster_ContextType	M3tDrawContext
	#define MSmTriRaster_BufferWidth	contextPrivate->width
	#define MSmTriRaster_BufferHeight	contextPrivate->height
	
#endif

MSmTriRaster_ReturnType
MSmTriRaster_FunctionName
(
	register MSmTriRaster_ContextType*	inContext,	/* Draw context */

	register UUtUns16		inVIndex0Min,
	register UUtUns16		inVIndex1Mid,
	register UUtUns16		inVIndex2Max
	
	#if MSmTriRaster_Split
	
		,
		register UUtUns16		inETextIndex0Min,
		register UUtUns16		inETextIndex1Mid,
		register UUtUns16		inETextIndex2Max
			
	#endif
	
	#if !MSmTriRaster_IsVisible && !MSmTriRaster_RasterZOnly && !MSmTriRaster_Split
	
		#if !MSmTriRaster_InterpShading && !MSmTriRaster_TextureNoShade
			,
			register UUtUns16	faceShade
		
		#endif
		
	#endif
)
{
#if MSmTriRaster_IsVisible || MSmTriRaster_RasterZOnly

	AKtEnvironment_Private*	contextPrivate = (AKtEnvironment_Private*)(TMmInstance_GetDynamicData(inContext));

#elif MSmTriRaster_Lightmap

	#define contextPrivate	inContext

#else

	MStDrawContextPrivate*	contextPrivate = (MStDrawContextPrivate *)inContext->privateContext;

#endif
	
	#if MSmTriRaster_Texture
	
		MStTextureMapPrivate*	textureMapPrivate = (MStTextureMapPrivate*)(M3rManager_Texture_GetEnginePrivate(contextPrivate->statePtr[M3cDrawStatePtrType_BaseTextureMap]));
		
		#if 0//MSmTriRaster_Split
		
			MStTextureMapPrivate*	ltextureMapPrivate;
		
		#endif
		
	#endif
	
	/*
	 * 
	 */
		M3tPointScreen*	screenPoints;
		
		#if MSmTriRaster_InterpShading
			
			UUtUns16*	vShades;
			UUtUns16	vShadeMin;
			UUtUns16	vShadeMid;
			UUtUns16	vShadeMax;
		
		#endif
		
		#if MSmTriRaster_Texture
			
			#if MSmTriRaster_Split
			
				M3tTextureCoord*	textureCoords;
			
			#else
			
				M3tTextureCoord*	textureCoords;
			
			#endif
			
		#endif
		
	/*
	 * Key to variable names
	 * 
	 * ddx = d/dx, horizontal incremental change
	 * ddy = d/dy, vertical incremental change
	 *
	 * The type is appended to ease the scheduling of integer and floating point variables
	 */
	
	#if !MSmTriRaster_Lightmap
	
		/*
		 * Z parameters
		 */
			UUtUns32	z_ddx_uns;
			UUtUns32	z_ddy_uns;
			UUtUns32	z_longEdge_uns;
			float		z_min_float, z_mid_float, z_max_float;
	
	#endif
	
	/*
	 * x left and right variables
	 */
		UUtInt32	x_longEdge_int,			x_longEdge_ddy_int;
		UUtInt32	x_leftEdge_int,			x_leftEdge_ddy_int;
		UUtInt32	x_rightEdge_int,		x_rightEdge_ddy_int;
		UUtInt32	x_shortEdgeBottom_int,	x_shortEdgeBottom_ddy_int;
		UUtInt32	x_shortEdgeTop_int,		x_shortEdgeTop_ddy_int;
		
	#if MSmTriRaster_InterpShading
	
		UUtInt32	m10_int, m11_int, m12_int;

		/*
		 * These variables have double duty. They are used to store the int version of the 
		 * min, mid, max rgb values and the variable interpolation values for rgb
		 */
			UUtInt32	r_max_ddy_int, g_max_ddy_int, b_max_ddy_int;
			UUtInt32	r_mid_ddx_int, g_mid_ddx_int, b_mid_ddx_int;
			UUtInt32	r_min_longEdge_int, g_min_longEdge_int, b_min_longEdge_int;
	#endif
	
	#if MSmTriRaster_Texture
	
		/*
		 * Texture UV 
		 */
		float		uOverW_ddx_float, vOverW_ddx_float, invW_ddx_float;
		float		uOverWMin_float, uOverWMid_float, uOverWMax_float;
		float		vOverWMin_float, vOverWMid_float, vOverWMax_float;
		float		invWMin_float, invWMid_float, invWMax_float;
		
		float		uOverW_longEdge_float, vOverW_longEdge_float, invW_longEdge_float;
		float		uOverW_longEdge_ddy_float, vOverW_longEdge_ddy_float, invW_longEdge_ddy_float;
		
		float		uOverW_float, vOverW_float, invW_float, w_float;
		float		u_float, v_float;
		UUtInt32	u_int, v_int;
		
		char		*textureBase;
		UUtInt32	nUResolutionBits_int, nVResolutionBits_int;
		UUtInt32	pageResolutionMaskU_int;
		UUtInt32	pageResolutionMaskV_int;
		
		UUtInt32	texelOffset;
		UUtUns16	texel;
		
		#if MSmTriRaster_Split
		
			UUtBool		useLightMap;
			
			//float		luOverW_ddx_float, lvOverW_ddx_float;
			//float		luOverWMin_float, luOverWMid_float, luOverWMax_float;
			//float		lvOverWMin_float, lvOverWMid_float, lvOverWMax_float;
			
			//float		luOverW_longEdge_float, lvOverW_longEdge_float;
			//float		luOverW_longEdge_ddy_float, lvOverW_longEdge_ddy_float;
			
			//float		luOverW_float, lvOverW_float;
			//float		lu_float, lv_float;
			//UUtInt32	lu_int, lv_int;
			
			//char		*ltextureBase;
			//UUtInt32	lnUResolutionBits_int, lnVResolutionBits_int;
			//UUtInt32	lpageResolutionMaskU_int;
			//UUtInt32	lpageResolutionMaskV_int;
			
			//UUtInt32	ltexelOffset;
			//UUtUns16	ltexel;
			
			
		#endif
		
	#endif
	
	/*
	 * top, middle, and bottom scanlines
	 */
		UUtInt32	scanline_min_int, scanline_mid_int, scanline_max_int;
		
	/*
	 * Pointers into the buffers and the rowBytes
	 */
	 	#if !MSmTriRaster_IsVisible && !MSmTriRaster_RasterZOnly
	 	
			UUtUns16*	imageScanlineLeft_ptr;
			UUtUns16	imageRowBytes_uns;
		
		#endif
		
		#if !MSmTriRaster_Lightmap
		
			UUtUns16*	z_scanlineLeft_ptr;
			UUtUns16	zRowBytes_uns;
		
		#endif
		
		#if MSmTriRaster_PixelTouch
		
			UUtUns32*	pixelTouch_scanLeft_ptr;
			UUtUns16	pixelTouchRowBytes_uns;
			
			UUtUns32	pixelTouchBitMask;
			UUtUns32	pixelTouchBitMaskOrig;
			
		#endif
		
	/*
	 * Used to indicate whether scanline traversal is left->right or right->left
	 */
		UUtInt16	spanDirection_int;
	
	/*
	 * min, mid, and max variables for x and y
	 */
		float	y_min_float, y_mid_float, y_max_float;
		float	x_min_float, x_mid_float, x_max_float;
		float	y0_float, y1_float, y2_float;
	
	/*
	 * Plane equation variables
	 */
		float		m10_float, m11_float, m12_float;
		float		invDet_float;
		float		y_maxMinusMin_float, y_midMinusMin_float;
		float		x_maxMinusMin_float, x_midMinusMin_float;
	
	/*
	 * Error variables
	 */
		float	y_minError_float, y_midError_float;
	
	/*
	 * Temporary variables
	 */
		float			temp_float;
		UUtUns16		temp_uns16;
		
		#if !MSmTriRaster_TextureNoShade && !MSmTriRaster_IsVisible && MSmTriRaster_InterpShading && !MSmTriRaster_RasterZOnly
		
			UUtInt32			r_temp_int, g_temp_int, b_temp_int;
		
		#endif
				
	/*
	 * floating point constants
	 */
	 	#if !MSmTriRaster_Lightmap
	 	
			float	z_scale_float = (float)MSmULongScale;
			float	z_offset_float = (float)MSmULongOffset;
		
		#endif
		
		float	x_scale_float = MSmFractOne * MSmSubPixelUnits;
		float	oneHalf_float = 0.5F;
		float	one_float = 1.0F;
	
	/*
	 * Scanline variables
	 */
		UUtInt32	scanline_int;
		
		UUtInt32	ptr_ddx_int;
		
		#if !MSmTriRaster_Lightmap
		
			UUtUns32	z_uns, z_target_uns;
			UUtUns16	*z_ptr;
		
		#endif
		
		#if !MSmTriRaster_IsVisible && !MSmTriRaster_RasterZOnly
		
			UUtUns16	*image_ptr;
		
		#endif
		
		
		#if MSmTriRaster_PixelTouch
		
			UUtUns32	pixelTouch_uns;
			UUtUns32*	pixelTouch_ptr;
		
		#endif
		
	/*
	 * Pixel variables
	 */		
		UUtInt32	x_pixelLeft_int, x_pixelRight_int;
		UUtInt32	i;
	
	/*
	 * DDA variables
	 */
		float		invHeight_float;
		UUtInt32	invHeight_int;
		

	#if 0//MmCOMPILER_MW
	
		__dcbt(vCoord0Min,0);
		__dcbt(vCoord1Mid,0);
		__dcbt(vCoord2Max,0);
		__dcbt(vShade0Min,0);
		__dcbt(vShade1Mid,0);
		__dcbt(vShade2Max,0);
	
	#endif
	
	/*
	 * Step 1. Sort the vertices according to y. Sort in place to save registers
	 */
		/* prefetch y mid, min, max, these should be cache misses */
		#if MSmTriRaster_IsVisible || MSmTriRaster_RasterZOnly || MSmTriRaster_Lightmap
		
			screenPoints = (M3tPointScreen*)contextPrivate->screenPoints;

		#else
		
			screenPoints = (M3tPointScreen*)contextPrivate->statePtr[M3cDrawStatePtrType_ScreenPointArray];
		
		#endif
		
		y0_float = (screenPoints + inVIndex0Min)->y;
		y1_float = (screenPoints + inVIndex1Mid)->y;
		y2_float = (screenPoints + inVIndex2Max)->y;
		
		UUmAssert(y0_float >= 0.0 && y0_float < (float)MSmTriRaster_BufferHeight);
		UUmAssert(y1_float >= 0.0 && y1_float < (float)MSmTriRaster_BufferHeight);
		UUmAssert(y2_float >= 0.0 && y2_float < (float)MSmTriRaster_BufferHeight);

		if (y0_float < y1_float)
		{
			y_min_float = y0_float;
			y_mid_float = y1_float;
		}
		else
		{
			y_min_float = y1_float;
			y_mid_float = y0_float;
			
			temp_uns16 = inVIndex0Min;
			inVIndex0Min = inVIndex1Mid;
			inVIndex1Mid = temp_uns16;
			
			#if MSmTriRaster_Split
			
				temp_uns16 = inETextIndex0Min;
				inETextIndex0Min = inETextIndex1Mid;
				inETextIndex1Mid = temp_uns16;
			
			#endif
		}
		
		if (y2_float < y_min_float)
		{
			y_max_float = y_mid_float;
			y_mid_float = y_min_float;
			y_min_float = y2_float;
			
			temp_uns16 = inVIndex2Max;
			inVIndex2Max = inVIndex1Mid;
			inVIndex1Mid = inVIndex0Min;
			inVIndex0Min = temp_uns16;
			
			#if MSmTriRaster_Split
			
				temp_uns16 = inETextIndex2Max;
				inETextIndex2Max = inETextIndex1Mid;
				inETextIndex1Mid = inETextIndex0Min;
				inETextIndex0Min = temp_uns16;
			
			#endif
			
		}
		else if (y2_float < y_mid_float)
		{
			y_max_float = y_mid_float;
			y_mid_float = y2_float;
			
			
			temp_uns16 = inVIndex2Max;
			inVIndex2Max = inVIndex1Mid;
			inVIndex1Mid = temp_uns16;
			
			#if MSmTriRaster_Split
			
				temp_uns16 = inETextIndex2Max;
				inETextIndex2Max = inETextIndex1Mid;
				inETextIndex1Mid = temp_uns16;
			
			#endif
			
		}
		else
		{
			y_max_float = y2_float;
		}
	
	/*
	 * Step 2. Compute plane equations and interwind load/store operations
	 */
		
		#if MSmTriRaster_Lightmap
		
			vShades = (UUtUns16*)contextPrivate->vShades;
			
		#elif MSmTriRaster_InterpShading
			
			vShades = (UUtUns16*)contextPrivate->statePtr[M3cDrawStatePtrType_ScreenShadeArray_DC];
		
		#endif
		
		/* prefetch x mid, min, max */
		x_min_float = (screenPoints + inVIndex0Min)->x;
		x_mid_float = (screenPoints + inVIndex1Mid)->x;
		x_max_float = (screenPoints + inVIndex2Max)->x;
		
		UUmAssert(x_min_float >= 0.0 && x_min_float < (float)MSmTriRaster_BufferWidth);
		UUmAssert(x_mid_float >= 0.0 && x_mid_float < (float)MSmTriRaster_BufferWidth);
		UUmAssert(x_max_float >= 0.0 && x_max_float < (float)MSmTriRaster_BufferWidth);

		#if MSmTriRaster_InterpShading
			
			vShadeMin = vShades[inVIndex0Min];
			vShadeMid = vShades[inVIndex1Mid];
			vShadeMax = vShades[inVIndex2Max];
			
			/* prefetch min rgb */					/* Calc plane equation stuff */
			r_min_longEdge_int = ((vShadeMin >> 10) & 0x1F) << 3;	y_maxMinusMin_float = y_max_float - y_min_float;
			g_min_longEdge_int = ((vShadeMin >> 5) & 0x1F) << 3;	x_midMinusMin_float = x_mid_float - x_min_float;
			b_min_longEdge_int = ((vShadeMin & 0x1F)) << 3;			y_midMinusMin_float = y_mid_float - y_min_float;
																	x_maxMinusMin_float = x_max_float - x_min_float;
																		
			/* prefetch mid rgb */					/* Continue plane equation stuff */
			r_mid_ddx_int = ((vShadeMid >> 10) & 0x1F) << 3;		invDet_float = y_maxMinusMin_float * x_midMinusMin_float - y_midMinusMin_float * x_maxMinusMin_float;
			g_mid_ddx_int = ((vShadeMid >> 5) & 0x1F) << 3;
			b_mid_ddx_int = ((vShadeMid & 0x1F)) << 3;				invDet_float = one_float / invDet_float;
			
			#if !MSmTriRaster_Lightmap
			
				/* prefetch z */									
				z_min_float = (screenPoints + inVIndex0Min)->z;	
				z_mid_float = (screenPoints + inVIndex1Mid)->z;
				z_max_float = (screenPoints + inVIndex2Max)->z;
			
				UUmAssert(z_min_float >= 0.0 && z_min_float < 1.0);
				UUmAssert(z_mid_float >= 0.0 && z_mid_float < 1.0);
				UUmAssert(z_max_float >= 0.0 && z_max_float < 1.0);
				
				z_min_float = z_min_float * z_scale_float + z_offset_float;
				z_mid_float = z_mid_float * z_scale_float + z_offset_float;		
				z_max_float = z_max_float * z_scale_float + z_offset_float;	
					
			#endif
												
			/* Prefetch max rgb */					/* scale z */								
			r_max_ddy_int = ((vShadeMax >> 10) & 0x1F) << 3;			
			g_max_ddy_int = ((vShadeMax >> 5) & 0x1F) << 3;			
			b_max_ddy_int = ((vShadeMax & 0x1F)) << 3;											
		
		#else
		
			#if !MSmTriRaster_TextureNoShade && !MSmTriRaster_IsVisible && MSmTriRaster_RGB565 && !MSmTriRaster_RasterZOnly && !MSmTriRaster_Split
				
				faceShade = ((faceShade >> 10) & 0x1F) << 11 |
							((faceShade >> 5) & 0x1F) << 6 |
							(faceShade & 0x1F);
				
 			#endif
		
			/* Calc plane equation stuff */
			y_maxMinusMin_float = y_max_float - y_min_float;	
			x_midMinusMin_float = x_mid_float - x_min_float;	
			y_midMinusMin_float = y_mid_float - y_min_float;	
			x_maxMinusMin_float = x_max_float - x_min_float;
																		
			/* Continue plane equation stuff */
			invDet_float = y_maxMinusMin_float * x_midMinusMin_float - y_midMinusMin_float * x_maxMinusMin_float;
			invDet_float = one_float / invDet_float;
			
			#if !MSmTriRaster_Lightmap
			
				/* prefetch z */									
				z_min_float = (screenPoints + inVIndex0Min)->z;	
				z_mid_float = (screenPoints + inVIndex1Mid)->z;
				z_max_float = (screenPoints + inVIndex2Max)->z;
																
				UUmAssert(z_min_float >= 0.0 && z_min_float <= 1.0);
				UUmAssert(z_mid_float >= 0.0 && z_mid_float <= 1.0);
				UUmAssert(z_max_float >= 0.0 && z_max_float <= 1.0);

				/* scale z */								
				z_min_float = z_min_float * z_scale_float + z_offset_float;			
				z_mid_float = z_mid_float * z_scale_float + z_offset_float;		
				z_max_float = z_max_float * z_scale_float + z_offset_float;								
 			
 			#endif
 			
		#endif
		
		if (invDet_float > 0.0F)
		{
			spanDirection_int = 1;
		}
		else
		{
			invDet_float = -invDet_float;
			spanDirection_int = -1;
		}
		
		/* Compute plane equation */
		m10_float = (y_mid_float - y_max_float) * invDet_float;
		m11_float = y_maxMinusMin_float * invDet_float;
		m12_float = -y_midMinusMin_float * invDet_float;
		
		#if !MSmTriRaster_Lightmap
		
			/* compute ddx for Z */											
			temp_float = m10_float * z_min_float;
			temp_float += m11_float * z_mid_float;
			temp_float += m12_float * z_max_float;
		
		#endif
		
		#if MSmTriRaster_Texture
			
			// XXX - Schedule this better
			
			#if MSmTriRaster_Split

				textureCoords = (M3tTextureCoord*)contextPrivate->statePtr[M3cDrawStatePtrType_TextureCoordArray];

				uOverWMin_float = (textureCoords + inETextIndex0Min)->u;
				uOverWMid_float = (textureCoords + inETextIndex1Mid)->u;
				uOverWMax_float = (textureCoords + inETextIndex2Max)->u;
				
				vOverWMin_float = (textureCoords + inETextIndex0Min)->v;
				vOverWMid_float = (textureCoords + inETextIndex1Mid)->v;
				vOverWMax_float = (textureCoords + inETextIndex2Max)->v;
				
				#if 0
				//luOverWMin_float = (textureCoords + inETextIndex0Min)->light.u;
				//luOverWMid_float = (textureCoords + inETextIndex1Mid)->light.u;
				//luOverWMax_float = (textureCoords + inETextIndex2Max)->light.u;
				
				lvOverWMin_float = (textureCoords + inETextIndex0Min)->light.v;
				lvOverWMid_float = (textureCoords + inETextIndex1Mid)->light.v;
				lvOverWMax_float = (textureCoords + inETextIndex2Max)->light.v;
				#endif
				
				invWMin_float = (screenPoints + inVIndex0Min)->invW;
				invWMid_float = (screenPoints + inVIndex1Mid)->invW;
				invWMax_float = (screenPoints + inVIndex2Max)->invW;
				
				uOverWMin_float *= invWMin_float;
				uOverWMid_float *= invWMid_float;
				uOverWMax_float *= invWMax_float;
				
				vOverWMin_float *= invWMin_float;
				vOverWMid_float *= invWMid_float;
				vOverWMax_float *= invWMax_float;

				#if 0
				luOverWMin_float *= invWMin_float;
				luOverWMid_float *= invWMid_float;
				luOverWMax_float *= invWMax_float;
				
				lvOverWMin_float *= invWMin_float;
				lvOverWMid_float *= invWMid_float;
				lvOverWMax_float *= invWMax_float;
				#endif
				
			#else
			
				textureCoords = (M3tTextureCoord*)contextPrivate->statePtr[M3cDrawStatePtrType_TextureCoordArray];

				uOverWMin_float = (textureCoords + inVIndex0Min)->u;
				uOverWMid_float = (textureCoords + inVIndex1Mid)->u;
				uOverWMax_float = (textureCoords + inVIndex2Max)->u;
				
				vOverWMin_float = (textureCoords + inVIndex0Min)->v;
				vOverWMid_float = (textureCoords + inVIndex1Mid)->v;
				vOverWMax_float = (textureCoords + inVIndex2Max)->v;
				
				invWMin_float = (screenPoints + inVIndex0Min)->invW;
				invWMid_float = (screenPoints + inVIndex1Mid)->invW;
				invWMax_float = (screenPoints + inVIndex2Max)->invW;
				
				uOverWMin_float *= invWMin_float;
				uOverWMid_float *= invWMid_float;
				uOverWMax_float *= invWMax_float;
				
				vOverWMin_float *= invWMin_float;
				vOverWMid_float *= invWMid_float;
				vOverWMax_float *= invWMax_float;
			
			#endif
			
		#endif
		
		
	/*
	 * Step 4. compute ddx parameters and the first, mid and bottom scanlines
	 * Step 5. Set up the vertical DDA
	 */
	 	#if !MSmTriRaster_Lightmap
			z_ddx_uns = (long)temp_float;
		#endif
		
		scanline_min_int = (UUtInt32)(y_min_float + oneHalf_float);
		scanline_mid_int = (UUtInt32)(y_mid_float + oneHalf_float);
		y_minError_float = ((float)scanline_min_int + oneHalf_float) - y_min_float;
		y_midError_float = ((float)scanline_mid_int + oneHalf_float) - y_mid_float;
		
		#if MSmTriRaster_IsVisible
			
			z_ddx_uns <<= 1;
			
		#endif
		
		#if MSmTriRaster_InterpShading
			
			m10_int = (UUtInt32)(m10_float * (float)MSmFractOne);
			m11_int = (UUtInt32)(m11_float * (float)MSmFractOne);
			m12_int = (UUtInt32)(m12_float * (float)MSmFractOne);
		
			/* start using r_mid_ddx_int as the ddx instead of the mid */
			r_mid_ddx_int = m11_int * r_mid_ddx_int;					temp_float = y_max_float - oneHalf_float;
			g_mid_ddx_int = m11_int * g_mid_ddx_int;					invHeight_float = x_scale_float / y_midMinusMin_float;
			b_mid_ddx_int = m11_int * b_mid_ddx_int;					x_min_float = x_min_float * x_scale_float;
			
			/* If the triangle does not hit the first scanline on the screen bail */
			if(temp_float < 0.5f)
			{
				return;
			}

			scanline_max_int = (UUtInt32)temp_float;

			/*
			 * This code section sets up the vertical DDA for all three edges.
			 * Begin by computing the inverse of the height of the short top edge.
			 */
			
			r_mid_ddx_int += m10_int * r_min_longEdge_int;
			g_mid_ddx_int += m10_int * g_min_longEdge_int;
			b_mid_ddx_int += m10_int * b_min_longEdge_int;					temp_float = x_midMinusMin_float * invHeight_float;

			r_mid_ddx_int += m12_int * r_max_ddy_int;		
			g_mid_ddx_int += m12_int * g_max_ddy_int;			
			b_mid_ddx_int += m12_int * b_max_ddy_int;			
		
		#else
			
			temp_float = y_max_float - oneHalf_float;
			invHeight_float = x_scale_float / y_midMinusMin_float;
			x_min_float = x_min_float * x_scale_float;
			/* If the triangle does not hit the first scanline on the screen bail */
			if(temp_float < 0.5f)
			{
				#if MSmTriRaster_IsVisible
				
					return UUcFalse;
				
				#else
				
					return;
					
				#endif
				
			}
			scanline_max_int = (UUtInt32)temp_float;
			temp_float = x_midMinusMin_float * invHeight_float;
			
		#endif
		
		#if MSmTriRaster_Texture
		
			uOverW_ddx_float = m10_float * uOverWMin_float + m11_float * uOverWMid_float + m12_float * uOverWMax_float;
			vOverW_ddx_float = m10_float * vOverWMin_float + m11_float * vOverWMid_float + m12_float * vOverWMax_float;
			invW_ddx_float = m10_float * invWMin_float + m11_float * invWMid_float + m12_float * invWMax_float;		
			
			#if 0//MSmTriRaster_Split
			
				luOverW_ddx_float = m10_float * luOverWMin_float + m11_float * luOverWMid_float + m12_float * luOverWMax_float;
				lvOverW_ddx_float = m10_float * lvOverWMin_float + m11_float * lvOverWMid_float + m12_float * lvOverWMax_float;

			#endif
			
		#endif
		
		x_shortEdgeTop_int = (UUtInt32)(x_min_float + y_minError_float * temp_float);
		x_shortEdgeTop_ddy_int = (UUtInt32)temp_float;
		
	/*
	 * Compute the inverse of the height of the long edge (and do a bit more
	 * pointer setup).
	 */
		invHeight_float = one_float / y_maxMinusMin_float;
		
		#if !MSmTriRaster_IsVisible && !MSmTriRaster_RasterZOnly
		
			imageRowBytes_uns	= contextPrivate->imageBufferRowBytes;
			imageScanlineLeft_ptr = (UUtUns16 *)((char *)contextPrivate->imageBufferBaseAddr +
									(scanline_min_int * imageRowBytes_uns));
		
		#endif
		
		#if !MSmTriRaster_Lightmap
		
			zRowBytes_uns		= (UUtUns16)(MSmTriRaster_BufferWidth * M3cDrawZBytesPerPixel);
			
			z_scanlineLeft_ptr	= (UUtUns16 *)((char *)contextPrivate->zBufferBaseAddr +
									(scanline_min_int * zRowBytes_uns));
		#endif
		
		#if MSmTriRaster_PixelTouch
			
			pixelTouchRowBytes_uns = contextPrivate->pixelTouchRowBytes;
			pixelTouch_scanLeft_ptr = (UUtUns32 *)((char *)contextPrivate->pixelTouchBaseAddr + 
											scanline_min_int * pixelTouchRowBytes_uns);
			
		#endif
		
		invHeight_int = (UUtInt32)(invHeight_float * (float)MSmFractOne);
	
	#if MSmTriRaster_Texture
	
		/*
		 * Grab some texture info
		 */
			textureBase = (char *)((M3tTextureMap*)contextPrivate->statePtr[M3cDrawStatePtrType_BaseTextureMap])->data;
			nUResolutionBits_int = textureMapPrivate->nWidthBits;
			nVResolutionBits_int = textureMapPrivate->nHeightBits;
			
		/*
		 * Handle some texture stuff
		 */
		{
			float	uResolution_float, vResolution_float;
			
			uResolution_float = (float)(1 << nUResolutionBits_int);
			vResolution_float = (float)(1 << nVResolutionBits_int);
			
			uOverW_longEdge_ddy_float = (uOverWMax_float - uOverWMin_float) * invHeight_float;
			vOverW_longEdge_ddy_float = (vOverWMax_float - vOverWMin_float) * invHeight_float;
			invW_longEdge_ddy_float = (invWMax_float - invWMin_float) * invHeight_float;
			
			uOverW_longEdge_float = uOverWMin_float + y_minError_float * uOverW_longEdge_ddy_float;
			vOverW_longEdge_float = vOverWMin_float + y_minError_float * vOverW_longEdge_ddy_float;
			invW_longEdge_float = invWMin_float + y_minError_float * invW_longEdge_ddy_float;
			
			uOverW_longEdge_ddy_float *= uResolution_float;
			vOverW_longEdge_ddy_float *= vResolution_float;
			
			uOverW_longEdge_float *= uResolution_float;
			vOverW_longEdge_float *= vResolution_float;
			
			uOverW_ddx_float *= uResolution_float;
			vOverW_ddx_float *= vResolution_float;
			
			pageResolutionMaskU_int = (1 << nUResolutionBits_int) - 1;
			pageResolutionMaskV_int = (1 << nUResolutionBits_int) - 1;
		}
		
		#if MSmTriRaster_Split
		
			/*
			 * Grab some texture info
			 */
			 	useLightMap = (UUtBool)(contextPrivate->statePtr[M3cDrawStatePtrType_LightTextureMap] != NULL);
			 
			 #if 0
			 if(useLightMap)
			 {
				ltextureBase = (char *)((M3tTextureMap*)contextPrivate->statePtr[M3cDrawStatePtrType_LightTextureMap])->data;
				ltextureMapPrivate = (MStTextureMapPrivate*)(M3rManager_Texture_GetEnginePrivate(contextPrivate->statePtr[M3cDrawStatePtrType_LightTextureMap]));
				lnUResolutionBits_int = ltextureMapPrivate->nWidthBits;
				lnVResolutionBits_int = ltextureMapPrivate->nHeightBits;
				
				/*
				 * Handle some texture stuff
				 */
				{
					float	luResolution_float, lvResolution_float;
					
					luResolution_float = (float)(1 << lnUResolutionBits_int);
					lvResolution_float = (float)(1 << lnVResolutionBits_int);
					
					luOverW_longEdge_ddy_float = (luOverWMax_float - luOverWMin_float) * invHeight_float;
					lvOverW_longEdge_ddy_float = (lvOverWMax_float - lvOverWMin_float) * invHeight_float;
					
					luOverW_longEdge_float = luOverWMin_float + y_minError_float * luOverW_longEdge_ddy_float;
					lvOverW_longEdge_float = lvOverWMin_float + y_minError_float * lvOverW_longEdge_ddy_float;
					
					luOverW_longEdge_ddy_float *= luResolution_float;
					lvOverW_longEdge_ddy_float *= lvResolution_float;
					
					luOverW_longEdge_float *= luResolution_float;
					lvOverW_longEdge_float *= lvResolution_float;
					
					luOverW_ddx_float *= luResolution_float;
					lvOverW_ddx_float *= lvResolution_float;
					
					lpageResolutionMaskU_int = (1 << lnUResolutionBits_int) - 1;
					lpageResolutionMaskV_int = (1 << lnUResolutionBits_int) - 1;
				}
			}
			#endif
			
		#endif
		
	#endif
	
	/*
	 * Compute the initial value and forward difference for all interpolated parameters
	 * along the long edge.
	 */
	
	temp_float = x_maxMinusMin_float * x_scale_float * invHeight_float;
	x_longEdge_int = (UUtInt32)(x_min_float + y_minError_float * temp_float);
	x_longEdge_ddy_int = (UUtInt32)temp_float;

	#if !MSmTriRaster_Lightmap
	
		temp_float = (z_max_float - z_min_float) * invHeight_float;
		z_longEdge_uns = (UUtUns32)(z_min_float + y_minError_float * temp_float);
		z_ddy_uns = (UUtInt32) temp_float;									/* Cast to long first to retain negative values */
	
	#endif
	
	#if MSmTriRaster_InterpShading
	
		r_max_ddy_int = (r_max_ddy_int - r_min_longEdge_int) * invHeight_int;	invHeight_float = x_scale_float / (y_maxMinusMin_float - y_midMinusMin_float);
		g_max_ddy_int = (g_max_ddy_int - g_min_longEdge_int) * invHeight_int;
		b_max_ddy_int = (b_max_ddy_int - b_min_longEdge_int) * invHeight_int;
		
		r_min_longEdge_int = r_min_longEdge_int << MSmFractBits;				temp_float = (x_max_float - x_mid_float) * invHeight_float;
		g_min_longEdge_int = g_min_longEdge_int << MSmFractBits;
		b_min_longEdge_int = b_min_longEdge_int << MSmFractBits;
		
		{
			UUtUns32	y_minError_int;
			
			y_minError_int = (UUtInt32)(y_minError_float * 255.9f);
			
			UUmAssert(y_minError_float > 0.0);
			
			/* Cheesy hack to handle subpixel error */
			if(r_max_ddy_int < 0)
			{
				r_min_longEdge_int -= ((-r_max_ddy_int) * y_minError_int) >> 8;
			}
			else
			{
				r_min_longEdge_int += (r_max_ddy_int * y_minError_int) >> 8;
			}
			if(g_max_ddy_int < 0)
			{
				g_min_longEdge_int -= ((-g_max_ddy_int) * y_minError_int) >> 8;
			}
			else
			{
				g_min_longEdge_int += (g_max_ddy_int * y_minError_int) >> 8;
			}
			if(b_max_ddy_int < 0)
			{
				b_min_longEdge_int -= ((-b_max_ddy_int) * y_minError_int) >> 8;
			}
			else
			{
				b_min_longEdge_int += (b_max_ddy_int * y_minError_int) >> 8;
			}
		}
	
		UUmAssert((scanline_max_int < scanline_min_int) || (unsigned long)(r_min_longEdge_int >> MSmFractBits) <= 0xFF);
		UUmAssert((scanline_max_int < scanline_min_int) || (unsigned long)(g_min_longEdge_int >> MSmFractBits) <= 0xFF);
		UUmAssert((scanline_max_int < scanline_min_int) || (unsigned long)(b_min_longEdge_int >> MSmFractBits) <= 0xFF);

	#else
	
		invHeight_float = x_scale_float / (y_maxMinusMin_float - y_midMinusMin_float);
		temp_float = (x_max_float - x_mid_float) * invHeight_float;
		
	#endif
	
	/*
	 * Compute the inital value and forward difference of X for the bottom short edge.
	 */
	
	// XXX - VisC++ bug
	{
		float t2;
		t2 = (x_mid_float * x_scale_float + y_midError_float * temp_float);

		x_shortEdgeBottom_int = (UUtInt32)t2;
	}
	x_shortEdgeBottom_ddy_int = (UUtInt32)temp_float;

	/*
	 * Assign the xLong and xShortTop DDA information to xLeft and xRight, as
	 * indicated by spanDirection. The spanDirection test also indicates whether
	 * the bottom short edge is left or right, so add the appropriate sub-pixel bias
	 * to xShortBottom as well.
	 */
	
	if (spanDirection_int > 0)
	{
		/*
		 * vMin-vMax (long) is left edge. vMin-vMid (shortTop) is the top right edge,
		 * and vMid-vMax is the bottom right edge.
		 */
		
		x_leftEdge_int = x_longEdge_int;
		x_leftEdge_ddy_int = x_longEdge_ddy_int;
		
		x_rightEdge_int = x_shortEdgeTop_int;
		x_rightEdge_ddy_int = x_shortEdgeTop_ddy_int;
		
		x_shortEdgeBottom_int -= (MSmHalfSubPixel + 1) << MSmFractBits;
	}
	else
	{
		/*
		 * vMin-vMax (short) is right edge, vMin-vMid (shortTop) is the top left edge,
		 * and vMid-vMax is the bottom left edge.
		 */
		
		x_rightEdge_int = x_longEdge_int;
		x_rightEdge_ddy_int = x_longEdge_ddy_int;
		
		x_leftEdge_int = x_shortEdgeTop_int;
		x_leftEdge_ddy_int = x_shortEdgeTop_ddy_int;
		
		x_shortEdgeBottom_int += (MSmHalfSubPixel - 1) << MSmFractBits;
	}
	
	/*
	 * Add in the sub-pixel coverage bias to xLeft and xRight.
	 */
	
	x_leftEdge_int += (MSmHalfSubPixel - 1) << MSmFractBits;
	x_rightEdge_int -= (MSmHalfSubPixel + 1) << MSmFractBits;
	
	for (scanline_int = scanline_min_int;
		scanline_int <= scanline_max_int;
		++scanline_int)
	{
		#if MSmTriRaster_Dither
		
			unsigned char		*ditherLUT;
			
		#endif
		
		if (scanline_int == scanline_mid_int)
		{
			if (spanDirection_int > 0)
			{
				/*
				 * Right vMin-vMid leg has ended, replace with vMid-vMax.
				 * Set xRight to the exact subpixel-accurate value that was computed
				 * earlier.
				 */
				
				x_rightEdge_ddy_int = x_shortEdgeBottom_ddy_int;
				x_rightEdge_int = x_shortEdgeBottom_int;
			}
			else
			{
				/*
				 * Left vMin-vMid leg has ended, replace with vMid-vMax.
				 * Set xLeft to the exact subpixel-accurate value that was computed
				 * earlier.
				 */
				
				x_leftEdge_ddy_int = x_shortEdgeBottom_ddy_int;
				x_leftEdge_int = x_shortEdgeBottom_int;
			}
		}
		
		/*
		 * Render a horizontal span. Note that the sub-pixel coverage bias has
		 * already been added into xLeft and xRight.
		 */
		
		x_pixelLeft_int = x_leftEdge_int >> (MSmSubPixelBits + MSmFractBits);
		x_pixelRight_int = x_rightEdge_int >> (MSmSubPixelBits + MSmFractBits);
		
		#if MSmTriRaster_Dither
		
			ditherLUT = MgDitherLUT;

			if (scanline_int & 0x1)
			{
				ditherLUT = (unsigned char *) (((unsigned long) ditherLUT) ^ dither4);
			}

		#endif
		
		if (spanDirection_int > 0)
		{
			/*
			 * We are rasterizing from left to right. Compute the pointers to the first
			 * pixel in the span.
			 */
			 	#if !MSmTriRaster_Lightmap
			 	
					z_ptr = z_scanlineLeft_ptr + x_pixelLeft_int;
				
				#endif
				
				#if !MSmTriRaster_IsVisible && !MSmTriRaster_RasterZOnly
				
					image_ptr = imageScanlineLeft_ptr + x_pixelLeft_int;
					
				#endif
				
				#if MSmTriRaster_PixelTouch
				
					pixelTouch_ptr = pixelTouch_scanLeft_ptr + (x_pixelLeft_int >> 5);
					
					pixelTouchBitMask = 1 << (31 - (x_pixelLeft_int & 0x1f));
					
					pixelTouchBitMaskOrig = 1 << 31;
					
				#endif
				
				ptr_ddx_int = 1;
				
			#if MSmTriRaster_Dither
				/*
				 * See comments above. If xPixelLeft (the initial pixel) is odd, XOR dither offset
				 * with 6.
				 */		
				if (x_pixelLeft_int & 0x1)
				{
					saturationLUT = (unsigned char *) (((unsigned long) saturationLUT) ^ dither6);
				}
				
			#endif
		}
		else
		{
			/*
			 * We are rasterizing from right to left.
			 */
			 	#if !MSmTriRaster_Lightmap
			 	
					z_ptr = z_scanlineLeft_ptr + x_pixelRight_int;
				
				#endif
				
				#if !MSmTriRaster_IsVisible && !MSmTriRaster_RasterZOnly
				
					image_ptr = imageScanlineLeft_ptr + x_pixelRight_int;
					
				#endif
				
				ptr_ddx_int = -1;

				#if MSmTriRaster_PixelTouch
				
					pixelTouch_ptr = pixelTouch_scanLeft_ptr + (x_pixelRight_int >> 5);
				
					pixelTouchBitMask = 1 << (31 - (x_pixelRight_int & 0x1f));
					
					pixelTouchBitMaskOrig = 1;
					
				#endif
				
			#if MSmTriRaster_Dither

				/*
				 * See comments above. If xPixelRight (the initial pixel) is odd, XOR dither offset
				 * with 6.
				 */
				if (x_pixelRight_int & 0x1)
				{
					saturationLUT = (unsigned char *) (((unsigned long) saturationLUT) ^ dither6);
				}
			
			#endif
		}
		
		#if MSmTriRaster_IsVisible
			
			ptr_ddx_int <<= 1;
			
		#endif
		
		#if MSmTriRaster_InterpShading && !MSmTriRaster_TextureNoShade
			
			r_temp_int = r_min_longEdge_int;
			g_temp_int = g_min_longEdge_int;
			b_temp_int = b_min_longEdge_int;
		
		#endif
		
		#if MSmTriRaster_Texture
		
			uOverW_float = uOverW_longEdge_float;
			vOverW_float = vOverW_longEdge_float;
			invW_float = invW_longEdge_float;
			
			#if 0//MSmTriRaster_Split
			
				luOverW_float = luOverW_longEdge_float;
				lvOverW_float = lvOverW_longEdge_float;
			
			#endif
			
		#endif
		
		
		/*
		 * Note that we cast z_a to a long, so that the shift is signed, and that we
		 * shift _before_ the multiple to overflow when the magnitude of z_ddx is large.
		 */
			#if !MSmTriRaster_Lightmap
			
				z_uns = z_longEdge_uns;
			
				/*
				 * Prefetch the first Z target value. Note that we test against the
				 * extended Z address range (<= zPtr < dbgZAddrMax), to allow for the
				 * 1 pixel overshoot that can occur on Z prefetches. This test is
				 * repeated with < dbgZAddrMax in the inner loop as well.
				 */
				
				z_target_uns = *z_ptr << 16;
			
			#endif
			
			#if MSmTriRaster_PixelTouch
			
				pixelTouch_uns = *pixelTouch_ptr;
			
			#endif
			
		/*
		 * Horizontal interpolation loop.
		 */
		{
			#if MSmTriRaster_Texture
			
				w_float = 1.0F / invW_float;
				
			#endif
			
			#if MSmTriRaster_IsVisible
			
				for (i = 1 + x_pixelRight_int - x_pixelLeft_int; (i -= 2) > 0; /* Nothing */)
			
			#else
			
				for (i = 1 + x_pixelRight_int - x_pixelLeft_int; i-- > 0; /* Nothing */)
			
			#endif
			{
				
				#if !MSmTriRaster_TextureNoShade && !MSmTriRaster_IsVisible && MSmTriRaster_InterpShading && !MSmTriRaster_RasterZOnly
				
					UUmAssert((unsigned long)(r_temp_int >> MSmFractBits) <= 0xFF);
					UUmAssert((unsigned long)(g_temp_int >> MSmFractBits) <= 0xFF);
					UUmAssert((unsigned long)(b_temp_int >> MSmFractBits) <= 0xFF);
				
				#endif
				
				#if MSmTriRaster_PixelTouch
				
					if(!(pixelTouch_uns & pixelTouchBitMask) ||
						(z_uns < z_target_uns)) 
					{
						
						pixelTouch_uns |= pixelTouchBitMask;
				
				#elif MSmTriRaster_Lightmap
				
					if(1)
					{
					
				#else
				
				
					if (z_uns < z_target_uns)
					{
				
				#endif

					#if MSmTriRaster_IsVisible
					
						return UUcTrue;
					
					#else
						
						#if !MSmTriRaster_Lightmap
						
							*z_ptr = (UUtUns16)(z_uns >> 16);
						
						#endif
						
						#if !MSmTriRaster_RasterZOnly
						
							#if MSmTriRaster_Texture
							
								u_float = uOverW_float * w_float;
								v_float = vOverW_float * w_float;
								
								u_int = (UUtInt32)u_float;
								v_int = (UUtInt32)v_float;
								
							/*
							 * Compute texel offset 
							 */
								texelOffset = ((pageResolutionMaskV_int & v_int) << nUResolutionBits_int)
												+ (pageResolutionMaskU_int & u_int);

								#if MSmTriRaster_RGB565

									texel = *(UUtUns16 *)(textureBase + (texelOffset << 1));
									
									texel = ((texel << 1) & 0xFFE) | (texel & 0x1F);
									
								#else
								
									texel = *(UUtUns16 *)(textureBase + (texelOffset << 1));

								#endif
								
								#if MSmTriRaster_Split
								#if 0
								if(useLightMap)
								{
									UUtUns32 texelR, texelG, texelB;
									
									// THIS IS HIGHLY UNOPTIMIZED FOR EXPERIMENTATION
									
									lu_float = luOverW_float * w_float;
									lv_float = lvOverW_float * w_float;
									
									lu_int = (UUtInt32)lu_float;
									lv_int = (UUtInt32)lv_float;
										
									/*
									 * Compute texel offset 
									 */
										ltexelOffset = ((lpageResolutionMaskV_int & lv_int) << lnUResolutionBits_int)
														+ (lpageResolutionMaskU_int & lu_int);
									
									texelR = (texel >> 10) & 0x1F;
									texelR = (texelR << 3) | (texelR >> 2);
									texelG = (texel >> 5) & 0x1F;
									texelG = (texelG << 3) | (texelG >> 2);
									texelB = (texel >> 0) & 0x1F;
									texelB = (texelB << 3) | (texelB >> 2);
									
									ltexel = *(UUtUns8*)(ltextureBase + ltexelOffset);
									
									texelR = (texelR * ltexel) >> 8;
									texelG = (texelG * ltexel) >> 8;
									texelB = (texelB * ltexel) >> 8;
									
									texelR = texelR >> 3;
									texelG = texelG >> 3;
									texelB = texelB >> 3;
									
									texel = (UUtUns16)((texelR << 10) | (texelG << 5) | (texelB));
								}
								else
								#endif
								{
									texel = *(UUtUns16 *)(textureBase + (texelOffset << 1));
								}
								#endif


								*image_ptr = texel;
								
								
							#else
							
								/*
								 * Render Gouraud color.
								 */
									#if MSmTriRaster_Alpha
									
									
									#else
										
										#if MSmTriRaster_InterpShading
										
											#if MSmTriRaster_RGB565
											
												*image_ptr = (UUtUns16)(
													((r_temp_int >> (MSmFractBits - (10 - 3))) & (0x1F << 11)) |
													((g_temp_int >> (MSmFractBits - (5 - 2))) & (0x1F << 5)) |
													((b_temp_int >> (MSmFractBits + 3))));

											#else
											
												*image_ptr = (UUtUns16)(
													((r_temp_int >> (MSmFractBits - (10 - 3))) & (0x1F << 10)) |
													((g_temp_int >> (MSmFractBits - (5 - 3))) & (0x1F << 5)) |
													((b_temp_int >> (MSmFractBits + 3))));
											
											#endif
										
										#else
										
											*image_ptr = (UUtUns16)(faceShade);
												
										#endif
										
									#endif
							
							#endif
						
						#endif
						
					#endif
					
				} /* if z test */
			
				#if MSmTriRaster_Texture
				
					invW_float += invW_ddx_float;
					w_float = 1.0F / invW_float;
					uOverW_float += uOverW_ddx_float;
					vOverW_float += vOverW_ddx_float;
					
					#if 0//MSmTriRaster_Split

						luOverW_float += luOverW_ddx_float;
						lvOverW_float += lvOverW_ddx_float;					
					
					#endif
					
				#endif
				
				#if !MSmTriRaster_Lightmap
				
					z_ptr += ptr_ddx_int;
					z_uns += z_ddx_uns;
					z_target_uns = *z_ptr << 16;
				
				#endif
				
				#if !MSmTriRaster_IsVisible && !MSmTriRaster_RasterZOnly
				
					image_ptr += ptr_ddx_int;
				
				#endif
				
				#if MSmTriRaster_PixelTouch
					
					if(ptr_ddx_int < 0)
					{
						pixelTouchBitMask <<= 1;
					}
					else
					{
						pixelTouchBitMask >>= 1;
					}
					
					if(pixelTouchBitMask == 0)
					{
						*pixelTouch_ptr = pixelTouch_uns;
						
						pixelTouch_ptr += ptr_ddx_int;
						
						pixelTouch_uns = *pixelTouch_ptr;
						
						pixelTouchBitMask = pixelTouchBitMaskOrig;
					}
				
				#endif
				
				#if MSmTriRaster_InterpShading && !MSmTriRaster_TextureNoShade

					r_temp_int += r_mid_ddx_int;
					g_temp_int += g_mid_ddx_int;
					b_temp_int += b_mid_ddx_int;
				
				#endif
				
				#if MSmTriRaster_Dither

					saturationLUT = (unsigned char *) (((unsigned long) saturationLUT) ^ dither6);
				
				#endif
			}
		}

		#if MSmTriRaster_PixelTouch
		
			if(pixelTouchBitMask != 0)
			{
				*pixelTouch_ptr = pixelTouch_uns;
			}
			
		#endif
		
		/*
		 * Advance to next scanline
		 */
		
		x_leftEdge_int += x_leftEdge_ddy_int;
		x_rightEdge_int += x_rightEdge_ddy_int;
		
		#if MSmTriRaster_Texture
		
			uOverW_longEdge_float += uOverW_longEdge_ddy_float;
			vOverW_longEdge_float += vOverW_longEdge_ddy_float;
			invW_longEdge_float += invW_longEdge_ddy_float;
			
			#if 0//MSmTriRaster_Split
			
				luOverW_longEdge_float += luOverW_longEdge_ddy_float;
				lvOverW_longEdge_float += lvOverW_longEdge_ddy_float;
			
			#endif
			
		#endif
		
		#if !MSmTriRaster_Lightmap
		
			z_longEdge_uns += z_ddy_uns;
			z_scanlineLeft_ptr = (UUtUns16 *)((char *)z_scanlineLeft_ptr + zRowBytes_uns);
		
		#endif
		
		#if !MSmTriRaster_IsVisible && !MSmTriRaster_RasterZOnly
		
			imageScanlineLeft_ptr = (UUtUns16 *)((char *)imageScanlineLeft_ptr + imageRowBytes_uns);
		
		#endif
		
		#if MSmTriRaster_PixelTouch
		
			pixelTouch_scanLeft_ptr = (UUtUns32 *)((char *)pixelTouch_scanLeft_ptr + pixelTouchRowBytes_uns);
			
		#endif
		
		#if MSmTriRaster_InterpShading
		
			r_min_longEdge_int += r_max_ddy_int;
			g_min_longEdge_int += g_max_ddy_int;
			b_min_longEdge_int += b_max_ddy_int;
			
		#endif
		
	}
	
	#if MSmTriRaster_IsVisible
	
		return UUcFalse;
	
	#endif
}

