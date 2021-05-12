/*
	FILE:	MS_Geom_Clip.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: May 21, 1997
	
	PURPOSE: Interface to the Motoko 3D engine
	
	Copyright 1997

*/
#include <stdlib.h>

#include "BFW.h"
#include "BFW_Motoko.h"

#include "Motoko_Manager.h"
#include "MS_GC_Private.h"
#include "MS_Geom_Clip.h"
#include "MS_Geom_Transform.h"

#if UUmCompiler	== UUmCompiler_VisC
#pragma optimize( "g", off )
#endif

#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING

	extern UUtInt32	gClipNesting;

#endif

#define UUmAssert_Clip(x)

#undef	MSmGeomClipCompPoint_3DFunctionName
#undef	MSmGeomClipCompPoint_4DFunctionName
#undef	MSmGeomClipCompPoint_InterpShading
#undef	MSmGeomClipCompPoint_IsVisible
#define MSmGeomClipCompPoint_3DFunctionName		MSiClip3D_ComputeClipPoint_NoShade
#define MSmGeomClipCompPoint_4DFunctionName		MSiClip4D_ComputeClipPoint_NoShade
#define MSmGeomClipCompPoint_InterpShading		0
#define MSmGeomClipCompPoint_IsVisible			0

#include "BFW_Shared_ClipCompPoint_c.h"

void
MSrClip_ComputeVertex_TextureInterpolate(
	UUtUns32				inClipPlane,
	
	UUtUns32				inVIndexIn0,
	UUtUns32				inVIndexOut0,
	UUtUns32				inVIndexNew0,
	
	UUtUns32				inVIndexIn1,
	UUtUns32				inVIndexOut1,
	UUtUns32				inVIndexNew1,
	
	UUtUns8					*outClipCodeNew0,
	UUtUns8					*outClipCodeNew1)
{
	M3tPoint4D*			frustumPoints;
	M3tPointScreen*		screenPoints;
	M3tTextureCoord*	textureCoords;
	
	M3tPoint4D*			frustumPointIn;
	M3tPoint4D*			frustumPointOut;
	M3tPoint4D*			frustumPointNew;
	
	M3tTextureCoord*	textureCoordIn;
	M3tTextureCoord*	textureCoordOut;
	M3tTextureCoord*	textureCoordNew;
	
	UUtUns32*			shades;
	UUtUns32			shadeIn;
	UUtUns32			shadeOut;
	UUtUns32*			shadeNew;
	
	M3tPointScreen*		screenPointNew;
	
	float				scaleX, scaleY;
	
	UUtUns8				newClipCode;
	float				oneMinust, pIn, pOut, negW, invW;
	float				hW, hX, hY, hZ;
	//float				wNew;
	float				wIn, wOut;
	float				t;
	UUtUns32			i;
	
	float				rIn, gIn, bIn;
	float				rOut, gOut, bOut;
	float				rNew, gNew, bNew;
	
	frustumPoints	= MSgGeomContextPrivate->objectVertexData.frustumPoints;
	UUmAssertReadPtr(frustumPoints, sizeof(M3tPoint4D));

	screenPoints	= MSgGeomContextPrivate->objectVertexData.screenPoints;
	UUmAssertReadPtr(screenPoints, sizeof(M3tPointScreen));

	textureCoords	= MSgGeomContextPrivate->objectVertexData.textureCoords;
	UUmAssertReadPtr(textureCoords, sizeof(M3tTextureCoord));

	shades			= MSgGeomContextPrivate->shades_vertex;
	UUmAssertReadPtr(shades, sizeof(UUtUns32));
	
	scaleX = MSgGeomContextPrivate->scaleX;
	scaleY = MSgGeomContextPrivate->scaleY;
	
	for(i = 0; i < 2; i++)
	{
		if(i == 0)
		{
			frustumPointIn = frustumPoints + inVIndexIn0;
			frustumPointOut = frustumPoints + inVIndexOut0;
			frustumPointNew = frustumPoints + inVIndexNew0;
		}
		else
		{
			frustumPointIn = frustumPoints + inVIndexIn1;
			frustumPointOut = frustumPoints + inVIndexOut1;
			frustumPointNew = frustumPoints + inVIndexNew1;
		}
	
		wIn = frustumPointIn->w;
		wOut = frustumPointOut->w;
		
		switch(inClipPlane)										
		{															
			case MScClipCode_PosX:									
				UUmAssert_Clip(frustumPointIn->x <= frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->x > frustumPointOut->w);			
				
				pIn = frustumPointIn->x;					
				pOut = frustumPointOut->x;				
				t = (wIn - pIn) / ((pOut - pIn) - (wOut - wIn));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hX = hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hY = t * frustumPointOut->y + oneMinust * frustumPointIn->y;
				hZ = t * frustumPointOut->z + oneMinust * frustumPointIn->z;
				break;				
												
			case MScClipCode_NegX:									
				UUmAssert_Clip(frustumPointIn->x >= -frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->x < -frustumPointOut->w);		
				
				pIn = frustumPointIn->x;					
				pOut = frustumPointOut->x;				
				t = (pIn + wIn) / ((pIn - pOut) + (wIn - wOut));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hY = t * frustumPointOut->y + oneMinust * frustumPointIn->y;
				hX = -hW;											
				hZ = t * frustumPointOut->z + oneMinust * frustumPointIn->z;
				break;					
											
			case MScClipCode_PosY:									
				UUmAssert_Clip(frustumPointIn->y <= frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->y > frustumPointOut->w);			
				
				pIn = frustumPointIn->y;					
				pOut = frustumPointOut->y;				
				t = (wIn - pIn) / ((pOut - pIn) - (wOut - wIn));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hY = hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hX = t * frustumPointOut->x + oneMinust * frustumPointIn->x;
				hZ = t * frustumPointOut->z + oneMinust * frustumPointIn->z;
				break;
															
			case MScClipCode_NegY:									
				UUmAssert_Clip(frustumPointIn->y >= -frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->y < -frustumPointOut->w);		
				
				pIn = frustumPointIn->y;					
				pOut = frustumPointOut->y;				
				t = (pIn + wIn) / ((pIn - pOut) + (wIn - wOut));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hY = -hW;											
				hX = t * frustumPointOut->x + oneMinust * frustumPointIn->x;
				hZ = t * frustumPointOut->z + oneMinust * frustumPointIn->z;
				break;		
														
			case MScClipCode_PosZ:									
				UUmAssert_Clip(frustumPointIn->z <= frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->z > frustumPointOut->w);			
				
				pIn = frustumPointIn->z;					
				pOut = frustumPointOut->z;				
				t = (wIn - pIn) / ((pOut - pIn) - (wOut - wIn));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hX = t * frustumPointOut->x + oneMinust * frustumPointIn->x;
				hY = t * frustumPointOut->y + oneMinust * frustumPointIn->y;
				hZ = hW;											
				break;	
															
			case MScClipCode_NegZ:									
				UUmAssert_Clip(frustumPointIn->z >= 0.0f);	
				UUmAssert_Clip(frustumPointOut->z < 0.0f);	
				
				pIn = frustumPointIn->z;					
				pOut = frustumPointOut->z;				
				t = pIn / (pIn - pOut);								
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hX = t * frustumPointOut->x + oneMinust * frustumPointIn->x;
				hY = t * frustumPointOut->y + oneMinust * frustumPointIn->y;
				hZ = 0.0f;											
				break;												

		}
		
		frustumPointNew->x = hX;
		frustumPointNew->y = hY;
		frustumPointNew->z = hZ;
		frustumPointNew->w = hW;
		
		if(i == 0)
		{
			textureCoordIn = textureCoords + inVIndexIn0;
			textureCoordOut = textureCoords + inVIndexOut0;
			textureCoordNew = textureCoords + inVIndexNew0;
		}
		else
		{
			textureCoordIn = textureCoords + inVIndexIn1;
			textureCoordOut = textureCoords + inVIndexOut1;
			textureCoordNew = textureCoords + inVIndexNew1;
		}
		textureCoordNew->u = t * textureCoordOut->u + oneMinust * textureCoordIn->u;
		textureCoordNew->v = t * textureCoordOut->v + oneMinust * textureCoordIn->v;
		
		// XXX - optimize this someday
		if(i == 0)
		{
			shadeIn = shades[inVIndexIn0];
			shadeOut = shades[inVIndexOut0];
			shadeNew = shades + inVIndexNew0;
		}
		else
		{
			shadeIn = shades[inVIndexIn1];
			shadeOut = shades[inVIndexOut1];
			shadeNew = shades + inVIndexNew1;
		}
		
		rOut = (float)((shadeOut >> 16) & 0xFF);
		gOut = (float)((shadeOut >> 8) & 0xFF);
		bOut = (float)((shadeOut >> 0) & 0xFF);
		
		rIn = (float)((shadeIn >> 16) & 0xFF);
		gIn = (float)((shadeIn >> 8) & 0xFF);
		bIn = (float)((shadeIn >> 0) & 0xFF);
		
		rNew = t * rOut + oneMinust * rIn;
		gNew = t * gOut + oneMinust * gIn;
		bNew = t * bOut + oneMinust * bIn;
		
		*shadeNew = 
			(UUtUns32) (MUrUnsignedSmallFloat_To_Uns_Round(rNew) << 16) |
			(UUtUns32) (MUrUnsignedSmallFloat_To_Uns_Round(gNew) << 8) | 
			(UUtUns32) (MUrUnsignedSmallFloat_To_Uns_Round(bNew) << 0);

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
		
		if(i == 0)
		{
			screenPointNew = screenPoints + inVIndexNew0;
		}
		else
		{
			screenPointNew = screenPoints + inVIndexNew1;
		}
		
		MSrClipCode_ValidateFrustum(frustumPointNew, newClipCode);

		if(newClipCode == 0)
		{
			invW = 1.0f / hW;
			if(hX == negW)
			{
				screenPointNew->x = 0.0f;
			}
			else
			{
				screenPointNew->x = (hX * invW + 1.0f) * scaleX;
			}
			
			if(hY == hW)
			{
				screenPointNew->y = 0.0f;
			}
			else
			{
				screenPointNew->y = (hY * invW - 1.0f) * scaleY;
			}
			
			screenPointNew->z = hZ * invW;
			screenPointNew->invW = invW;
			
			MSiVerifyPointScreen(screenPointNew);
		}
		
		if(i == 0)
		{
			*outClipCodeNew0 = newClipCode;
		}
		else
		{
			*outClipCodeNew1 = newClipCode;
		}
	}
}

void
MSrClip_ComputeVertex_TextureEnvInterpolate(
	UUtUns32				inClipPlane,
	
	UUtUns32				inVIndexIn0,
	UUtUns32				inVIndexOut0,
	UUtUns32				inVIndexNew0,
	
	UUtUns32				inVIndexIn1,
	UUtUns32				inVIndexOut1,
	UUtUns32				inVIndexNew1,
	
	UUtUns8					*outClipCodeNew0,
	UUtUns8					*outClipCodeNew1)
{
	M3tPoint4D*			frustumPoints;
	M3tPointScreen*		screenPoints;
	
	M3tPoint4D*			frustumPointIn;
	M3tPoint4D*			frustumPointOut;
	M3tPoint4D*			frustumPointNew;
	
	M3tTextureCoord*	textureCoords;
	M3tTextureCoord*	textureCoordIn;
	M3tTextureCoord*	textureCoordOut;
	M3tTextureCoord*	textureCoordNew;
	
	M3tTextureCoord*	envTextureCoords;
	M3tTextureCoord*	envTextureCoordIn;
	M3tTextureCoord*	envTextureCoordOut;
	M3tTextureCoord*	envTextureCoordNew;
	
	UUtUns32*			shades;
	UUtUns32			shadeIn;
	UUtUns32			shadeOut;
	UUtUns32*			shadeNew;
	
	M3tPointScreen*		screenPointNew;
	
	float				scaleX, scaleY;
	
	UUtUns8				newClipCode;
	float				oneMinust, pIn, pOut, negW, invW;
	float				hW, hX, hY, hZ;
	//float				wNew;
	float				wIn, wOut;
	float				t;
	UUtUns32			i;
	
	float				rIn, gIn, bIn;
	float				rOut, gOut, bOut;
	float				rNew, gNew, bNew;
	
	frustumPoints	= MSgGeomContextPrivate->objectVertexData.frustumPoints;
	UUmAssertReadPtr(frustumPoints, sizeof(M3tPoint4D));

	screenPoints	= MSgGeomContextPrivate->objectVertexData.screenPoints;
	UUmAssertReadPtr(screenPoints, sizeof(M3tPointScreen));

	textureCoords	= MSgGeomContextPrivate->objectVertexData.textureCoords;
	UUmAssertReadPtr(textureCoords, sizeof(M3tTextureCoord));

	envTextureCoords	= MSgGeomContextPrivate->envMapCoords;
	UUmAssertReadPtr(envTextureCoords, sizeof(M3tTextureCoord));

	shades			= MSgGeomContextPrivate->shades_vertex;
	UUmAssertReadPtr(shades, sizeof(UUtUns32));
	
	scaleX = MSgGeomContextPrivate->scaleX;
	scaleY = MSgGeomContextPrivate->scaleY;
	
	for(i = 0; i < 2; i++)
	{
		if(i == 0)
		{
			frustumPointIn = frustumPoints + inVIndexIn0;
			frustumPointOut = frustumPoints + inVIndexOut0;
			frustumPointNew = frustumPoints + inVIndexNew0;
		}
		else
		{
			frustumPointIn = frustumPoints + inVIndexIn1;
			frustumPointOut = frustumPoints + inVIndexOut1;
			frustumPointNew = frustumPoints + inVIndexNew1;
		}
	
		wIn = frustumPointIn->w;
		wOut = frustumPointOut->w;
		
		switch(inClipPlane)										
		{															
			case MScClipCode_PosX:									
				UUmAssert_Clip(frustumPointIn->x <= frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->x > frustumPointOut->w);			
				
				pIn = frustumPointIn->x;					
				pOut = frustumPointOut->x;				
				t = (wIn - pIn) / ((pOut - pIn) - (wOut - wIn));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hX = hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hY = t * frustumPointOut->y + oneMinust * frustumPointIn->y;
				hZ = t * frustumPointOut->z + oneMinust * frustumPointIn->z;
				break;				
												
			case MScClipCode_NegX:									
				UUmAssert_Clip(frustumPointIn->x >= -frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->x < -frustumPointOut->w);		
				
				pIn = frustumPointIn->x;					
				pOut = frustumPointOut->x;				
				t = (pIn + wIn) / ((pIn - pOut) + (wIn - wOut));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hY = t * frustumPointOut->y + oneMinust * frustumPointIn->y;
				hX = -hW;											
				hZ = t * frustumPointOut->z + oneMinust * frustumPointIn->z;
				break;					
											
			case MScClipCode_PosY:									
				UUmAssert_Clip(frustumPointIn->y <= frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->y > frustumPointOut->w);			
				
				pIn = frustumPointIn->y;					
				pOut = frustumPointOut->y;				
				t = (wIn - pIn) / ((pOut - pIn) - (wOut - wIn));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hY = hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hX = t * frustumPointOut->x + oneMinust * frustumPointIn->x;
				hZ = t * frustumPointOut->z + oneMinust * frustumPointIn->z;
				break;
															
			case MScClipCode_NegY:									
				UUmAssert_Clip(frustumPointIn->y >= -frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->y < -frustumPointOut->w);		
				
				pIn = frustumPointIn->y;					
				pOut = frustumPointOut->y;				
				t = (pIn + wIn) / ((pIn - pOut) + (wIn - wOut));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hY = -hW;											
				hX = t * frustumPointOut->x + oneMinust * frustumPointIn->x;
				hZ = t * frustumPointOut->z + oneMinust * frustumPointIn->z;
				break;		
														
			case MScClipCode_PosZ:									
				UUmAssert_Clip(frustumPointIn->z <= frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->z > frustumPointOut->w);			
				
				pIn = frustumPointIn->z;					
				pOut = frustumPointOut->z;				
				t = (wIn - pIn) / ((pOut - pIn) - (wOut - wIn));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hX = t * frustumPointOut->x + oneMinust * frustumPointIn->x;
				hY = t * frustumPointOut->y + oneMinust * frustumPointIn->y;
				hZ = hW;											
				break;	
															
			case MScClipCode_NegZ:									
				UUmAssert_Clip(frustumPointIn->z >= 0.0f);	
				UUmAssert_Clip(frustumPointOut->z < 0.0f);	
				
				pIn = frustumPointIn->z;					
				pOut = frustumPointOut->z;				
				t = pIn / (pIn - pOut);								
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hX = t * frustumPointOut->x + oneMinust * frustumPointIn->x;
				hY = t * frustumPointOut->y + oneMinust * frustumPointIn->y;
				hZ = 0.0f;											
				break;												

		}
		
		frustumPointNew->x = hX;
		frustumPointNew->y = hY;
		frustumPointNew->z = hZ;
		frustumPointNew->w = hW;
		
		if(i == 0)
		{
			textureCoordIn = textureCoords + inVIndexIn0;
			textureCoordOut = textureCoords + inVIndexOut0;
			textureCoordNew = textureCoords + inVIndexNew0;
			
			envTextureCoordIn = envTextureCoords + inVIndexIn0;
			envTextureCoordOut = envTextureCoords + inVIndexOut0;
			envTextureCoordNew = envTextureCoords + inVIndexNew0;
		}
		else
		{
			textureCoordIn = textureCoords + inVIndexIn1;
			textureCoordOut = textureCoords + inVIndexOut1;
			textureCoordNew = textureCoords + inVIndexNew1;
			
			envTextureCoordIn = envTextureCoords + inVIndexIn1;
			envTextureCoordOut = envTextureCoords + inVIndexOut1;
			envTextureCoordNew = envTextureCoords + inVIndexNew1;
		}
		textureCoordNew->u = t * textureCoordOut->u + oneMinust * textureCoordIn->u;
		textureCoordNew->v = t * textureCoordOut->v + oneMinust * textureCoordIn->v;
		envTextureCoordNew->u = t * envTextureCoordOut->u + oneMinust * envTextureCoordIn->u;
		envTextureCoordNew->v = t * envTextureCoordOut->v + oneMinust * envTextureCoordIn->v;
		
		// XXX - optimize this someday
		if(i == 0)
		{
			shadeIn = shades[inVIndexIn0];
			shadeOut = shades[inVIndexOut0];
			shadeNew = shades + inVIndexNew0;
		}
		else
		{
			shadeIn = shades[inVIndexIn1];
			shadeOut = shades[inVIndexOut1];
			shadeNew = shades + inVIndexNew1;
		}
		
		rOut = (float)((shadeOut >> 16) & 0xFF);
		gOut = (float)((shadeOut >> 8) & 0xFF);
		bOut = (float)((shadeOut >> 0) & 0xFF);
		
		rIn = (float)((shadeIn >> 16) & 0xFF);
		gIn = (float)((shadeIn >> 8) & 0xFF);
		bIn = (float)((shadeIn >> 0) & 0xFF);
		
		rNew = t * rOut + oneMinust * rIn;
		gNew = t * gOut + oneMinust * gIn;
		bNew = t * bOut + oneMinust * bIn;
		
		*shadeNew = 
			(UUtUns32) (MUrUnsignedSmallFloat_To_Uns_Round(rNew) << 16) |
			(UUtUns32) (MUrUnsignedSmallFloat_To_Uns_Round(gNew) << 8) | 
			(UUtUns32) (MUrUnsignedSmallFloat_To_Uns_Round(bNew) << 0);

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
		
		if(i == 0)
		{
			screenPointNew = screenPoints + inVIndexNew0;
		}
		else
		{
			screenPointNew = screenPoints + inVIndexNew1;
		}
		
		MSrClipCode_ValidateFrustum(frustumPointNew, newClipCode);

		if(newClipCode == 0)
		{
			invW = 1.0f / hW;
			if(hX == negW)
			{
				screenPointNew->x = 0.0f;
			}
			else
			{
				screenPointNew->x = (hX * invW + 1.0f) * scaleX;
			}
			
			if(hY == hW)
			{
				screenPointNew->y = 0.0f;
			}
			else
			{
				screenPointNew->y = (hY * invW - 1.0f) * scaleY;
			}
			
			screenPointNew->z = hZ * invW;
			screenPointNew->invW = invW;
			
			MSiVerifyPointScreen(screenPointNew);
		}
		
		if(i == 0)
		{
			*outClipCodeNew0 = newClipCode;
		}
		else
		{
			*outClipCodeNew1 = newClipCode;
		}
	}
}

void
MSrClip_ComputeVertex_GouraudInterpolate(
	UUtUns32				inClipPlane,
	
	UUtUns32				inVIndexIn0,
	UUtUns32				inVIndexOut0,
	UUtUns32				inVIndexNew0,
	
	UUtUns32				inVIndexIn1,
	UUtUns32				inVIndexOut1,
	UUtUns32				inVIndexNew1,
	
	UUtUns8					*outClipCodeNew0,
	UUtUns8					*outClipCodeNew1)
{
	M3tPoint4D*			frustumPoints;
	M3tPointScreen*		screenPoints;
	
	M3tPoint4D*			frustumPointIn;
	M3tPoint4D*			frustumPointOut;
	M3tPoint4D*			frustumPointNew;
	
	UUtUns32*			shades;
	UUtUns32			shadeIn;
	UUtUns32			shadeOut;
	UUtUns32*			shadeNew;
	
	M3tPointScreen*		screenPointNew;
	
	float				scaleX, scaleY;
	
	UUtUns8				newClipCode;
	float				oneMinust, pIn, pOut, negW, invW;
	float				hW, hX, hY, hZ;
	//float				wNew;
	float				wIn, wOut;
	float				t;
	UUtUns32			i;
	
	float				rIn, gIn, bIn;
	float				rOut, gOut, bOut;
	float				rNew, gNew, bNew;
	
	frustumPoints	= MSgGeomContextPrivate->objectVertexData.frustumPoints;
	UUmAssertReadPtr(frustumPoints, sizeof(M3tPoint4D));

	screenPoints	= MSgGeomContextPrivate->objectVertexData.screenPoints;
	UUmAssertReadPtr(screenPoints, sizeof(M3tPointScreen));

	shades			= MSgGeomContextPrivate->shades_vertex;
	UUmAssertReadPtr(shades, sizeof(UUtUns32));
	
	scaleX = MSgGeomContextPrivate->scaleX;
	scaleY = MSgGeomContextPrivate->scaleY;
	
	for(i = 0; i < 2; i++)
	{
		if(i == 0)
		{
			frustumPointIn = frustumPoints + inVIndexIn0;
			frustumPointOut = frustumPoints + inVIndexOut0;
			frustumPointNew = frustumPoints + inVIndexNew0;
		}
		else
		{
			frustumPointIn = frustumPoints + inVIndexIn1;
			frustumPointOut = frustumPoints + inVIndexOut1;
			frustumPointNew = frustumPoints + inVIndexNew1;
		}
	
		wIn = frustumPointIn->w;
		wOut = frustumPointOut->w;
		
		switch(inClipPlane)										
		{															
			case MScClipCode_PosX:									
				UUmAssert_Clip(frustumPointIn->x <= frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->x > frustumPointOut->w);			
				
				pIn = frustumPointIn->x;					
				pOut = frustumPointOut->x;				
				t = (wIn - pIn) / ((pOut - pIn) - (wOut - wIn));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hX = hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hY = t * frustumPointOut->y + oneMinust * frustumPointIn->y;
				hZ = t * frustumPointOut->z + oneMinust * frustumPointIn->z;
				break;				
												
			case MScClipCode_NegX:									
				UUmAssert_Clip(frustumPointIn->x >= -frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->x < -frustumPointOut->w);		
				
				pIn = frustumPointIn->x;					
				pOut = frustumPointOut->x;				
				t = (pIn + wIn) / ((pIn - pOut) + (wIn - wOut));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hY = t * frustumPointOut->y + oneMinust * frustumPointIn->y;
				hX = -hW;											
				hZ = t * frustumPointOut->z + oneMinust * frustumPointIn->z;
				break;					
											
			case MScClipCode_PosY:									
				UUmAssert_Clip(frustumPointIn->y <= frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->y > frustumPointOut->w);			
				
				pIn = frustumPointIn->y;					
				pOut = frustumPointOut->y;				
				t = (wIn - pIn) / ((pOut - pIn) - (wOut - wIn));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hY = hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hX = t * frustumPointOut->x + oneMinust * frustumPointIn->x;
				hZ = t * frustumPointOut->z + oneMinust * frustumPointIn->z;
				break;
															
			case MScClipCode_NegY:									
				UUmAssert_Clip(frustumPointIn->y >= -frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->y < -frustumPointOut->w);		
				
				pIn = frustumPointIn->y;					
				pOut = frustumPointOut->y;				
				t = (pIn + wIn) / ((pIn - pOut) + (wIn - wOut));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hY = -hW;											
				hX = t * frustumPointOut->x + oneMinust * frustumPointIn->x;
				hZ = t * frustumPointOut->z + oneMinust * frustumPointIn->z;
				break;		
														
			case MScClipCode_PosZ:									
				UUmAssert_Clip(frustumPointIn->z <= frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->z > frustumPointOut->w);			
				
				pIn = frustumPointIn->z;					
				pOut = frustumPointOut->z;				
				t = (wIn - pIn) / ((pOut - pIn) - (wOut - wIn));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hX = t * frustumPointOut->x + oneMinust * frustumPointIn->x;
				hY = t * frustumPointOut->y + oneMinust * frustumPointIn->y;
				hZ = hW;											
				break;	
															
			case MScClipCode_NegZ:									
				UUmAssert_Clip(frustumPointIn->z >= 0.0f);	
				UUmAssert_Clip(frustumPointOut->z < 0.0f);	
				
				pIn = frustumPointIn->z;					
				pOut = frustumPointOut->z;				
				t = pIn / (pIn - pOut);								
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hX = t * frustumPointOut->x + oneMinust * frustumPointIn->x;
				hY = t * frustumPointOut->y + oneMinust * frustumPointIn->y;
				hZ = 0.0f;											
				break;												

		}
		
		frustumPointNew->x = hX;
		frustumPointNew->y = hY;
		frustumPointNew->z = hZ;
		frustumPointNew->w = hW;
		
		// XXX - optimize this someday
		if(i == 0)
		{
			shadeIn = shades[inVIndexIn0];
			shadeOut = shades[inVIndexOut0];
			shadeNew = shades + inVIndexNew0;
		}
		else
		{
			shadeIn = shades[inVIndexIn1];
			shadeOut = shades[inVIndexOut1];
			shadeNew = shades + inVIndexNew1;
		}
		
		rOut = (float)((shadeOut >> 16) & 0xFF);
		gOut = (float)((shadeOut >> 8) & 0xFF);
		bOut = (float)((shadeOut >> 0) & 0xFF);
		
		rIn = (float)((shadeIn >> 16) & 0xFF);
		gIn = (float)((shadeIn >> 8) & 0xFF);
		bIn = (float)((shadeIn >> 0) & 0xFF);
		
		rNew = t * rOut + oneMinust * rIn;
		gNew = t * gOut + oneMinust * gIn;
		bNew = t * bOut + oneMinust * bIn;
		
		*shadeNew = 
			((UUtUns32)rNew << 16) |
			((UUtUns32)gNew << 8) |
			((UUtUns32)bNew << 0);

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
		
		if(i == 0)
		{
			screenPointNew = screenPoints + inVIndexNew0;
		}
		else
		{
			screenPointNew = screenPoints + inVIndexNew1;
		}
		
		MSrClipCode_ValidateFrustum(frustumPointNew, newClipCode);

		if(newClipCode == 0)
		{
			invW = 1.0f / hW;
			if(hX == negW)
			{
				screenPointNew->x = 0.0f;
			}
			else
			{
				screenPointNew->x = (hX * invW + 1.0f) * scaleX;
			}
			
			if(hY == hW)
			{
				screenPointNew->y = 0.0f;
			}
			else
			{
				screenPointNew->y = (hY * invW - 1.0f) * scaleY;
			}
			
			screenPointNew->z = hZ * invW;
			screenPointNew->invW = invW;
			
			MSiVerifyPointScreen(screenPointNew);
		}
		
		if(i == 0)
		{
			*outClipCodeNew0 = newClipCode;
		}
		else
		{
			*outClipCodeNew1 = newClipCode;
		}
	}
}

void
MSrClip_ComputeVertex_TextureFlat(
	UUtUns32				inClipPlane,
	
	UUtUns32				inVIndexIn0,
	UUtUns32				inVIndexOut0,
	UUtUns32				inVIndexNew0,
	
	UUtUns32				inVIndexIn1,
	UUtUns32				inVIndexOut1,
	UUtUns32				inVIndexNew1,
	
	UUtUns8					*outClipCodeNew0,
	UUtUns8					*outClipCodeNew1)
{
	M3tPoint4D*			frustumPoints;
	M3tPointScreen*		screenPoints;
	M3tTextureCoord*	textureCoords;
	
	M3tPoint4D*			frustumPointIn;
	M3tPoint4D*			frustumPointOut;
	M3tPoint4D*			frustumPointNew;
	
	M3tTextureCoord*	textureCoordIn;
	M3tTextureCoord*	textureCoordOut;
	M3tTextureCoord*	textureCoordNew;
	
	M3tPointScreen*		screenPointNew;
	
	float				scaleX, scaleY;
	
	UUtUns8				newClipCode;
	float				oneMinust, pIn, pOut, negW, invW;
	float				hW, hX, hY, hZ;
	//float				wNew;
	float				wIn, wOut;
	float				t;
	UUtUns32			i;
	
	frustumPoints	= MSgGeomContextPrivate->objectVertexData.frustumPoints;
	UUmAssertReadPtr(frustumPoints, sizeof(M3tPoint4D));

	screenPoints	= MSgGeomContextPrivate->objectVertexData.screenPoints;
	UUmAssertReadPtr(screenPoints, sizeof(M3tPointScreen));

	textureCoords	= MSgGeomContextPrivate->objectVertexData.textureCoords;
	UUmAssertReadPtr(textureCoords, sizeof(M3tTextureCoord));
	
	scaleX = MSgGeomContextPrivate->scaleX;
	scaleY = MSgGeomContextPrivate->scaleY;
	
	for(i = 0; i < 2; i++)
	{
		if(i == 0)
		{
			frustumPointIn = frustumPoints + inVIndexIn0;
			frustumPointOut = frustumPoints + inVIndexOut0;
			frustumPointNew = frustumPoints + inVIndexNew0;
		}
		else
		{
			frustumPointIn = frustumPoints + inVIndexIn1;
			frustumPointOut = frustumPoints + inVIndexOut1;
			frustumPointNew = frustumPoints + inVIndexNew1;
		}
	
		wIn = frustumPointIn->w;
		wOut = frustumPointOut->w;
		
		switch(inClipPlane)										
		{															
			case MScClipCode_PosX:									
				UUmAssert_Clip(frustumPointIn->x <= frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->x > frustumPointOut->w);			
				
				pIn = frustumPointIn->x;					
				pOut = frustumPointOut->x;				
				t = (wIn - pIn) / ((pOut - pIn) - (wOut - wIn));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hX = hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hY = t * frustumPointOut->y + oneMinust * frustumPointIn->y;
				hZ = t * frustumPointOut->z + oneMinust * frustumPointIn->z;
				break;				
												
			case MScClipCode_NegX:									
				UUmAssert_Clip(frustumPointIn->x >= -frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->x < -frustumPointOut->w);		
				
				pIn = frustumPointIn->x;					
				pOut = frustumPointOut->x;				
				t = (pIn + wIn) / ((pIn - pOut) + (wIn - wOut));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hY = t * frustumPointOut->y + oneMinust * frustumPointIn->y;
				hX = -hW;											
				hZ = t * frustumPointOut->z + oneMinust * frustumPointIn->z;
				break;					
											
			case MScClipCode_PosY:									
				UUmAssert_Clip(frustumPointIn->y <= frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->y > frustumPointOut->w);			
				
				pIn = frustumPointIn->y;					
				pOut = frustumPointOut->y;				
				t = (wIn - pIn) / ((pOut - pIn) - (wOut - wIn));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hY = hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hX = t * frustumPointOut->x + oneMinust * frustumPointIn->x;
				hZ = t * frustumPointOut->z + oneMinust * frustumPointIn->z;
				break;
															
			case MScClipCode_NegY:									
				UUmAssert_Clip(frustumPointIn->y >= -frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->y < -frustumPointOut->w);		
				
				pIn = frustumPointIn->y;					
				pOut = frustumPointOut->y;				
				t = (pIn + wIn) / ((pIn - pOut) + (wIn - wOut));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hY = -hW;											
				hX = t * frustumPointOut->x + oneMinust * frustumPointIn->x;
				hZ = t * frustumPointOut->z + oneMinust * frustumPointIn->z;
				break;		
														
			case MScClipCode_PosZ:									
				UUmAssert_Clip(frustumPointIn->z <= frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->z > frustumPointOut->w);			
				
				pIn = frustumPointIn->z;					
				pOut = frustumPointOut->z;				
				t = (wIn - pIn) / ((pOut - pIn) - (wOut - wIn));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hX = t * frustumPointOut->x + oneMinust * frustumPointIn->x;
				hY = t * frustumPointOut->y + oneMinust * frustumPointIn->y;
				hZ = hW;											
				break;	
															
			case MScClipCode_NegZ:									
				UUmAssert_Clip(frustumPointIn->z >= 0.0f);	
				UUmAssert_Clip(frustumPointOut->z < 0.0f);	
				
				pIn = frustumPointIn->z;					
				pOut = frustumPointOut->z;				
				t = pIn / (pIn - pOut);								
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hX = t * frustumPointOut->x + oneMinust * frustumPointIn->x;
				hY = t * frustumPointOut->y + oneMinust * frustumPointIn->y;
				hZ = 0.0f;											
				break;												

		}
		
		frustumPointNew->x = hX;
		frustumPointNew->y = hY;
		frustumPointNew->z = hZ;
		frustumPointNew->w = hW;
		
		if(i == 0)
		{
			textureCoordIn = textureCoords + inVIndexIn0;
			textureCoordOut = textureCoords + inVIndexOut0;
			textureCoordNew = textureCoords + inVIndexNew0;
		}
		else
		{
			textureCoordIn = textureCoords + inVIndexIn1;
			textureCoordOut = textureCoords + inVIndexOut1;
			textureCoordNew = textureCoords + inVIndexNew1;
		}
		textureCoordNew->u = t * textureCoordOut->u + oneMinust * textureCoordIn->u;
		textureCoordNew->v = t * textureCoordOut->v + oneMinust * textureCoordIn->v;
		
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
		
		if(i == 0)
		{
			screenPointNew = screenPoints + inVIndexNew0;
		}
		else
		{
			screenPointNew = screenPoints + inVIndexNew1;
		}
		
		MSrClipCode_ValidateFrustum(frustumPointNew, newClipCode);

		if(newClipCode == 0)
		{
			invW = 1.0f / hW;
			if(hX == negW)
			{
				screenPointNew->x = 0.0f;
			}
			else
			{
				screenPointNew->x = (hX * invW + 1.0f) * scaleX;
			}
			
			if(hY == hW)
			{
				screenPointNew->y = 0.0f;
			}
			else
			{
				screenPointNew->y = (hY * invW - 1.0f) * scaleY;
			}
			
			screenPointNew->z = hZ * invW;
			screenPointNew->invW = invW;
			
			MSiVerifyPointScreen(screenPointNew);
		}
		
		if(i == 0)
		{
			*outClipCodeNew0 = newClipCode;
		}
		else
		{
			*outClipCodeNew1 = newClipCode;
		}
	}
}

void
MSrClip_ComputeVertex_GouraudFlat(
	UUtUns32				inClipPlane,
	
	UUtUns32				inVIndexIn0,
	UUtUns32				inVIndexOut0,
	UUtUns32				inVIndexNew0,
	
	UUtUns32				inVIndexIn1,
	UUtUns32				inVIndexOut1,
	UUtUns32				inVIndexNew1,
	
	UUtUns8					*outClipCodeNew0,
	UUtUns8					*outClipCodeNew1)
{
	M3tPoint4D*			frustumPoints;
	M3tPointScreen*		screenPoints;
	
	M3tPoint4D*			frustumPointIn;
	M3tPoint4D*			frustumPointOut;
	M3tPoint4D*			frustumPointNew;
	
	M3tPointScreen*		screenPointNew;
	
	float				scaleX, scaleY;
	
	UUtUns8				newClipCode;
	float				oneMinust, pIn, pOut, negW, invW;
	float				hW, hX, hY, hZ;
	//float				wNew;
	float				wIn, wOut;
	float				t;
	UUtUns32			i;
	
	frustumPoints	= MSgGeomContextPrivate->objectVertexData.frustumPoints;
	UUmAssertReadPtr(frustumPoints, sizeof(M3tPoint4D));

	screenPoints	= MSgGeomContextPrivate->objectVertexData.screenPoints;
	UUmAssertReadPtr(screenPoints, sizeof(M3tPointScreen));
	
	scaleX = MSgGeomContextPrivate->scaleX;
	scaleY = MSgGeomContextPrivate->scaleY;
	
	for(i = 0; i < 2; i++)
	{
		if(i == 0)
		{
			frustumPointIn = frustumPoints + inVIndexIn0;
			frustumPointOut = frustumPoints + inVIndexOut0;
			frustumPointNew = frustumPoints + inVIndexNew0;
		}
		else
		{
			frustumPointIn = frustumPoints + inVIndexIn1;
			frustumPointOut = frustumPoints + inVIndexOut1;
			frustumPointNew = frustumPoints + inVIndexNew1;
		}
	
		wIn = frustumPointIn->w;
		wOut = frustumPointOut->w;
		
		switch(inClipPlane)										
		{															
			case MScClipCode_PosX:									
				UUmAssert_Clip(frustumPointIn->x <= frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->x > frustumPointOut->w);			
				
				pIn = frustumPointIn->x;					
				pOut = frustumPointOut->x;				
				t = (wIn - pIn) / ((pOut - pIn) - (wOut - wIn));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hX = hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hY = t * frustumPointOut->y + oneMinust * frustumPointIn->y;
				hZ = t * frustumPointOut->z + oneMinust * frustumPointIn->z;
				break;				
												
			case MScClipCode_NegX:									
				UUmAssert_Clip(frustumPointIn->x >= -frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->x < -frustumPointOut->w);		
				
				pIn = frustumPointIn->x;					
				pOut = frustumPointOut->x;				
				t = (pIn + wIn) / ((pIn - pOut) + (wIn - wOut));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hY = t * frustumPointOut->y + oneMinust * frustumPointIn->y;
				hX = -hW;											
				hZ = t * frustumPointOut->z + oneMinust * frustumPointIn->z;
				break;					
											
			case MScClipCode_PosY:									
				UUmAssert_Clip(frustumPointIn->y <= frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->y > frustumPointOut->w);			
				
				pIn = frustumPointIn->y;					
				pOut = frustumPointOut->y;				
				t = (wIn - pIn) / ((pOut - pIn) - (wOut - wIn));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hY = hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hX = t * frustumPointOut->x + oneMinust * frustumPointIn->x;
				hZ = t * frustumPointOut->z + oneMinust * frustumPointIn->z;
				break;
															
			case MScClipCode_NegY:									
				UUmAssert_Clip(frustumPointIn->y >= -frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->y < -frustumPointOut->w);		
				
				pIn = frustumPointIn->y;					
				pOut = frustumPointOut->y;				
				t = (pIn + wIn) / ((pIn - pOut) + (wIn - wOut));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hY = -hW;											
				hX = t * frustumPointOut->x + oneMinust * frustumPointIn->x;
				hZ = t * frustumPointOut->z + oneMinust * frustumPointIn->z;
				break;		
														
			case MScClipCode_PosZ:									
				UUmAssert_Clip(frustumPointIn->z <= frustumPointIn->w);			
				UUmAssert_Clip(frustumPointOut->z > frustumPointOut->w);			
				
				pIn = frustumPointIn->z;					
				pOut = frustumPointOut->z;				
				t = (wIn - pIn) / ((pOut - pIn) - (wOut - wIn));	
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hX = t * frustumPointOut->x + oneMinust * frustumPointIn->x;
				hY = t * frustumPointOut->y + oneMinust * frustumPointIn->y;
				hZ = hW;											
				break;	
															
			case MScClipCode_NegZ:									
				UUmAssert_Clip(frustumPointIn->z >= 0.0f);	
				UUmAssert_Clip(frustumPointOut->z < 0.0f);	
				
				pIn = frustumPointIn->z;					
				pOut = frustumPointOut->z;				
				t = pIn / (pIn - pOut);								
				oneMinust = (1.0f - t);								
				UUmAssert_Clip(t <= 1.0f);
				UUmAssert_Clip(t >= -1.0f);		
				hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
				hX = t * frustumPointOut->x + oneMinust * frustumPointIn->x;
				hY = t * frustumPointOut->y + oneMinust * frustumPointIn->y;
				hZ = 0.0f;											
				break;												

		}
		
		frustumPointNew->x = hX;
		frustumPointNew->y = hY;
		frustumPointNew->z = hZ;
		frustumPointNew->w = hW;
		
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
		
		if(i == 0)
		{
			screenPointNew = screenPoints + inVIndexNew0;
		}
		else
		{
			screenPointNew = screenPoints + inVIndexNew1;
		}
		
		MSrClipCode_ValidateFrustum(frustumPointNew, newClipCode);

		if(newClipCode == 0)
		{
			invW = 1.0f / hW;
			if(hX == negW)
			{
				screenPointNew->x = 0.0f;
			}
			else
			{
				screenPointNew->x = (hX * invW + 1.0f) * scaleX;
			}
			
			if(hY == hW)
			{
				screenPointNew->y = 0.0f;
			}
			else
			{
				screenPointNew->y = (hY * invW - 1.0f) * scaleY;
			}
			
			screenPointNew->z = hZ * invW;
			screenPointNew->invW = invW;
			
			MSiVerifyPointScreen(screenPointNew);
		}
		
		if(i == 0)
		{
			*outClipCodeNew0 = newClipCode;
		}
		else
		{
			*outClipCodeNew1 = newClipCode;
		}
	}
}

void
MSrClip_ComputeVertex_LineFlat(
	UUtUns32				inClipPlane,
	UUtUns32				inVIndexIn,
	UUtUns32				inVIndexOut,
	UUtUns32				inVIndexNew,
	UUtUns8					*outClipCodeNew)
{
	M3tPoint4D*			frustumPoints;
	M3tPointScreen*		screenPoints;
	
	M3tPoint4D*			frustumPointIn;
	M3tPoint4D*			frustumPointOut;
	M3tPoint4D*			frustumPointNew;
	
	M3tPointScreen*		screenPointNew;
	
	float				scaleX, scaleY;
	
	UUtUns8				newClipCode;
	float				oneMinust, pIn, pOut, negW, invW;
	float				hW, hX, hY, hZ;
	float				wIn, wOut;
	float				t;
	
	frustumPoints	= MSgGeomContextPrivate->objectVertexData.frustumPoints;
	UUmAssertReadPtr(frustumPoints, sizeof(M3tPoint4D));

	screenPoints	= MSgGeomContextPrivate->objectVertexData.screenPoints;
	UUmAssertReadPtr(screenPoints, sizeof(M3tPointScreen));
	
	scaleX = MSgGeomContextPrivate->scaleX;
	scaleY = MSgGeomContextPrivate->scaleY;

	frustumPointIn = frustumPoints + inVIndexIn;
	frustumPointOut = frustumPoints + inVIndexOut;
	frustumPointNew = frustumPoints + inVIndexNew;

	wIn = frustumPointIn->w;
	wOut = frustumPointOut->w;
	
	switch(inClipPlane)										
	{															
		case MScClipCode_PosX:									
			UUmAssert_Clip(frustumPointIn->x <= frustumPointIn->w);			
			UUmAssert_Clip(frustumPointOut->x > frustumPointOut->w);			
			
			pIn = frustumPointIn->x;					
			pOut = frustumPointOut->x;				
			t = (wIn - pIn) / ((pOut - pIn) - (wOut - wIn));	
			oneMinust = (1.0f - t);								
			UUmAssert_Clip(t <= 1.0f);
			UUmAssert_Clip(t >= -1.0f);		
			hX = hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
			hY = t * frustumPointOut->y + oneMinust * frustumPointIn->y;
			hZ = t * frustumPointOut->z + oneMinust * frustumPointIn->z;
			break;				
											
		case MScClipCode_NegX:									
			UUmAssert_Clip(frustumPointIn->x >= -frustumPointIn->w);			
			UUmAssert_Clip(frustumPointOut->x < -frustumPointOut->w);		
			
			pIn = frustumPointIn->x;					
			pOut = frustumPointOut->x;				
			t = (pIn + wIn) / ((pIn - pOut) + (wIn - wOut));	
			oneMinust = (1.0f - t);								
			UUmAssert_Clip(t <= 1.0f);
			UUmAssert_Clip(t >= -1.0f);		
			hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
			hY = t * frustumPointOut->y + oneMinust * frustumPointIn->y;
			hX = -hW;											
			hZ = t * frustumPointOut->z + oneMinust * frustumPointIn->z;
			break;					
										
		case MScClipCode_PosY:									
			UUmAssert_Clip(frustumPointIn->y <= frustumPointIn->w);			
			UUmAssert_Clip(frustumPointOut->y > frustumPointOut->w);			
			
			pIn = frustumPointIn->y;					
			pOut = frustumPointOut->y;				
			t = (wIn - pIn) / ((pOut - pIn) - (wOut - wIn));	
			oneMinust = (1.0f - t);								
			UUmAssert_Clip(t <= 1.0f);
			UUmAssert_Clip(t >= -1.0f);		
			hY = hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
			hX = t * frustumPointOut->x + oneMinust * frustumPointIn->x;
			hZ = t * frustumPointOut->z + oneMinust * frustumPointIn->z;
			break;
														
		case MScClipCode_NegY:									
			UUmAssert_Clip(frustumPointIn->y >= -frustumPointIn->w);			
			UUmAssert_Clip(frustumPointOut->y < -frustumPointOut->w);		
			
			pIn = frustumPointIn->y;					
			pOut = frustumPointOut->y;				
			t = (pIn + wIn) / ((pIn - pOut) + (wIn - wOut));	
			oneMinust = (1.0f - t);								
			UUmAssert_Clip(t <= 1.0f);
			UUmAssert_Clip(t >= -1.0f);		
			hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
			hY = -hW;											
			hX = t * frustumPointOut->x + oneMinust * frustumPointIn->x;
			hZ = t * frustumPointOut->z + oneMinust * frustumPointIn->z;
			break;		
													
		case MScClipCode_PosZ:									
			UUmAssert_Clip(frustumPointIn->z <= frustumPointIn->w);			
			UUmAssert_Clip(frustumPointOut->z > frustumPointOut->w);			
			
			pIn = frustumPointIn->z;					
			pOut = frustumPointOut->z;				
			t = (wIn - pIn) / ((pOut - pIn) - (wOut - wIn));	
			oneMinust = (1.0f - t);								
			UUmAssert_Clip(t <= 1.0f);
			UUmAssert_Clip(t >= -1.0f);		
			hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
			hX = t * frustumPointOut->x + oneMinust * frustumPointIn->x;
			hY = t * frustumPointOut->y + oneMinust * frustumPointIn->y;
			hZ = hW;											
			break;	
														
		case MScClipCode_NegZ:									
			UUmAssert_Clip(frustumPointIn->z >= 0.0f);	
			UUmAssert_Clip(frustumPointOut->z < 0.0f);	
			
			pIn = frustumPointIn->z;					
			pOut = frustumPointOut->z;				
			t = pIn / (pIn - pOut);								
			oneMinust = (1.0f - t);								
			UUmAssert_Clip(t <= 1.0f);
			UUmAssert_Clip(t >= -1.0f);		
			hW = t * frustumPointOut->w + oneMinust * frustumPointIn->w;
			hX = t * frustumPointOut->x + oneMinust * frustumPointIn->x;
			hY = t * frustumPointOut->y + oneMinust * frustumPointIn->y;
			hZ = 0.0f;											
			break;												

	}
	
	frustumPointNew->x = hX;
	frustumPointNew->y = hY;
	frustumPointNew->z = hZ;
	frustumPointNew->w = hW;
	
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
	
	screenPointNew = screenPoints + inVIndexNew;
	
	MSrClipCode_ValidateFrustum(frustumPointNew, newClipCode);

	if(newClipCode == 0)
	{
		invW = 1.0f / hW;
		if(hX == negW)
		{
			screenPointNew->x = 0.0f;
		}
		else
		{
			screenPointNew->x = (hX * invW + 1.0f) * scaleX;
		}
		
		if(hY == hW)
		{
			screenPointNew->y = 0.0f;
		}
		else
		{
			screenPointNew->y = (hY * invW - 1.0f) * scaleY;
		}
		
		screenPointNew->z = hZ * invW;
		screenPointNew->invW = invW;
		
		MSiVerifyPointScreen(screenPointNew);
	}
	
	*outClipCodeNew = newClipCode;
}

void
MSrClip_Line(
	UUtUns32		inVIndex0,
	UUtUns32		inVIndex1,
	UUtUns8			inClipCode0,
	UUtUns8			inClipCode1,
	UUtUns8			inClippedPlanes)
{
	UUtInt8					i;
	UUtUns32				curClipPlane;
	
	UUtUns8				newClipCode;

	UUtUns32			newVIndex;	
	
	MSrClip_LineComputeVertexProc	lineComputeVertexProc;
	
	if(inClipCode0 & inClipCode1)
	{
		return;
	}
	
	lineComputeVertexProc = MSgGeomContextPrivate->lineComputeVertexProc;
	
	newVIndex = MSgGeomContextPrivate->objectVertexData.newClipVertexIndex++;
	
	UUmAssert(newVIndex < M3cMaxObjVertices);

	#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING
	
	gClipNesting++;
	
	UUmAssert(gClipNesting < 10);
	UUmAssert(gClipNesting >= 0);
	
	MSiVerifyPoint4D(
		frustumPoint0);
	MSiVerifyPoint4D(
		frustumPoint1);
		
	MSrClipCode_ValidateFrustum(
		frustumPoint0,
		inClipCode0);
	MSrClipCode_ValidateFrustum(
		frustumPoint1,
		inClipCode1);

	#endif
	
	for(i = 6; i-- > 0;)
	{
		curClipPlane = 1 << i;
		
		if(inClippedPlanes & curClipPlane)
		{
			continue;
		}
		
		if(inClipCode0 & curClipPlane)
		{
			// clipCode0 is out of bounds
			// clipCode1 is in bounds
			
			lineComputeVertexProc(
				curClipPlane,
				inVIndex1,
				inVIndex0,
				newVIndex,
				&newClipCode);
			
			/* Check for trivial accept */
			if(!(inClipCode1 | newClipCode))
			{
				
				// Draw the line
				M3rDraw_Line(inVIndex1,	newVIndex);
			}
			else
			{
				// Recurse
				MSrClip_Line(
							inVIndex1,
					newVIndex,
					inClipCode1,
					newClipCode,
					(UUtUns8)(inClippedPlanes | curClipPlane));
			}
			
			/* Check for trivial reject */
			if(inClipCode0 & (newClipCode | curClipPlane))
			{
				// Draw nothing
			}
			else
			{
				// Recurse
				MSrClip_Line(
							inVIndex0,
					newVIndex,
					inClipCode0,
					newClipCode,
					(UUtUns8)(inClippedPlanes | curClipPlane));
			}
		}
		
		if(inClipCode1 & curClipPlane)
		{
			// clipCode1 is out of bounds
			// clipCode0 is in bounds
			lineComputeVertexProc(
				curClipPlane,
				inVIndex0,
				inVIndex1,
				newVIndex,
				&newClipCode);
				
			/* Check for trivial accept */
			if(!(inClipCode0 | newClipCode))
			{
				// Draw the line
				M3rDraw_Line(
					inVIndex0,
					newVIndex);
			}
			else
			{
				// Recurse
				MSrClip_Line(
							inVIndex0,
					newVIndex,
					inClipCode0,
					newClipCode,
					(UUtUns8)(inClippedPlanes | curClipPlane));
			}
			
			/* Check for trivial reject */
			if(inClipCode1 & (newClipCode | curClipPlane))
			{
				// Draw nothing
			}
			else
			{
				// Recurse
				MSrClip_Line(
							inVIndex1,
					newVIndex,
					inClipCode1,
					newClipCode,
					(UUtUns8)(inClippedPlanes | curClipPlane));
			}
		}
	}

	#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING
		
		gClipNesting--;
		
	#endif
}

void
MSrClip_Triangle(
	UUtUns32		inVIndex0,
	UUtUns32		inVIndex1,
	UUtUns32		inVIndex2,
	UUtUns32		inClipCode0,
	UUtUns32		inClipCode1,
	UUtUns32		inClipCode2,
	UUtUns32		inClippedPlanes)	/* Set to 0 */
{

	MSrClip_PolyComputeVertexProc	computeVertexProc;
	
	UUtUns8					newClipCode0;
	UUtUns8					newClipCode1;
	UUtUns32				newVIndex0;	
	UUtUns32				newVIndex1;	

	UUtInt32					i;
	UUtUns8					curClipPlane;
	
	M3tTri					newTri;
	M3tQuad					newQuad;
	
	if(inClipCode0 & inClipCode1 & inClipCode2)
	{
		return;
	}
	
	newVIndex0 = MSgGeomContextPrivate->objectVertexData.newClipVertexIndex++;
	newVIndex1 = MSgGeomContextPrivate->objectVertexData.newClipVertexIndex++;
	
	computeVertexProc = MSgGeomContextPrivate->polyComputeVertexProc;
	
	UUmAssert(MSgGeomContextPrivate->objectVertexData.newClipVertexIndex <= MSgGeomContextPrivate->objectVertexData.maxClipVertices);
	
	for(i = 6; i-- > 0;)
	{
		curClipPlane = (UUtUns8)(1 << i);
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
				computeVertexProc(
					curClipPlane,
					inVIndex2,			inVIndex0,			newVIndex0, 
					inVIndex2,			inVIndex1,			newVIndex1, 
					&newClipCode0,		&newClipCode1);
																
				/* Check for trivial accept on 2, new0, new1 */	
				if(inClipCode2 | newClipCode0 | newClipCode1)
				{
					MSrClip_Triangle(
						inVIndex2,			newVIndex0,			newVIndex1,
						inClipCode2,		newClipCode0,		newClipCode1,
						inClippedPlanes | curClipPlane);
				}
				else
				{
					newTri.indices[0] = inVIndex2;
					newTri.indices[1] = newVIndex0;
					newTri.indices[2] = newVIndex1;
					
					M3rDraw_Triangle(
						&newTri);
				}
																
				return;							
			}													
			else												
			{													
				/* 0: Out, 1: in, 2: ? */						
				if(curClipPlane & inClipCode2)						
				{												
					/* 0: Out, 1: In, 2: Out */					
																
					/* Compute 1 -> 0 Intersection */			
					/* Compute 1 -> 2 Intersection */			
					computeVertexProc(
						curClipPlane,
						inVIndex1,			inVIndex0,			newVIndex0, 
						inVIndex1,			inVIndex2,			newVIndex1, 
						&newClipCode0,		&newClipCode1);
				
					/* Check for trivial accept on 1, new1, new0 */
					if(inClipCode1 | newClipCode1 | newClipCode0)
					{
						MSrClip_Triangle(
							inVIndex1,			newVIndex1,			newVIndex0,
							inClipCode1,		newClipCode1,		newClipCode0,
							inClippedPlanes | curClipPlane);
					}
					else
					{
						newTri.indices[0] = inVIndex1;
						newTri.indices[1] = newVIndex1;
						newTri.indices[2] = newVIndex0;
						
						M3rDraw_Triangle(
							&newTri);
					}
																
					return;						
				}												
				else											
				{												
					/* 0: Out, 1: In, 2: In */					
																
					/* Compute 1 -> 0 intersection */			
					/* Compute 2 -> 0 intersection */			
					computeVertexProc(
						curClipPlane,
						inVIndex1,			inVIndex0,			newVIndex0, 
						inVIndex2,			inVIndex0,			newVIndex1, 
						&newClipCode0,		&newClipCode1);
					
					/* Check for trivial accept on 1, 2, new1, new0 */		
					if(inClipCode1 | inClipCode2 | newClipCode1 | newClipCode0)
					{
						MSrClip_Quad(
							inVIndex1,			inVIndex2,			newVIndex1,			newVIndex0,
							inClipCode1,		inClipCode2,		newClipCode1,		newClipCode0,
							inClippedPlanes | curClipPlane);
					}
					else
					{
						newQuad.indices[0] = inVIndex1;
						newQuad.indices[1] = inVIndex2;
						newQuad.indices[2] = newVIndex1;
						newQuad.indices[3] = newVIndex0;
						
						M3rDraw_Quad(
							&newQuad);
					}
																
					return;						
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
					computeVertexProc(
						curClipPlane,
						inVIndex0,			inVIndex1,			newVIndex0, 
						inVIndex0,			inVIndex2,			newVIndex1, 
						&newClipCode0,		&newClipCode1);
																
					/* Check for trivial accept on 0, new0, new1 */
					if(inClipCode0 | newClipCode0 | newClipCode1)
					{
						MSrClip_Triangle(
											inVIndex0,			newVIndex0,			newVIndex1,
							inClipCode0,		newClipCode0,		newClipCode1,
							inClippedPlanes | curClipPlane);
					}
					else
					{
						newTri.indices[0] = inVIndex0;
						newTri.indices[1] = newVIndex0;
						newTri.indices[2] = newVIndex1;
						
						M3rDraw_Triangle(
							&newTri);
					}
																
					return;						
				}												
				else											
				{												
					/* 0: In, 1: Out, 2: In */					
																
					/* Compute 0 -> 1 intersection */			
					/* Compute 2 -> 1 intersection */			
					computeVertexProc(
						curClipPlane,
						inVIndex0,			inVIndex1,			newVIndex0, 
						inVIndex2,			inVIndex1,			newVIndex1, 
						&newClipCode0,		&newClipCode1);
																
					/* Check for trivial accept on 2, 0, new0, new1 */		
					if(inClipCode2 | inClipCode0 | newClipCode0 | newClipCode1)
					{
						MSrClip_Quad(
							inVIndex2,			inVIndex0,			newVIndex0,			newVIndex1,
							inClipCode2,		inClipCode0,		newClipCode0,		newClipCode1,
							inClippedPlanes | curClipPlane);
					}
					else
					{
						newQuad.indices[0] = inVIndex2;
						newQuad.indices[1] = inVIndex0;
						newQuad.indices[2] = newVIndex0;
						newQuad.indices[3] = newVIndex1;
						
						M3rDraw_Quad(
							&newQuad);
					}
																
					return;						
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
					computeVertexProc(
						curClipPlane,
						inVIndex0,			inVIndex2,			newVIndex0, 
						inVIndex1,			inVIndex2,			newVIndex1, 
						&newClipCode0,		&newClipCode1);
																
					/* Check for trivial accept on 0, 1, new1, new0 */		
					if(inClipCode0 | inClipCode1 | newClipCode1 | newClipCode0)
					{
						MSrClip_Quad(
											inVIndex0,			inVIndex1,			newVIndex1,			newVIndex0,
							inClipCode0,		inClipCode1,		newClipCode1,		newClipCode0,
							inClippedPlanes | curClipPlane);
					}
					else
					{
						newQuad.indices[0] = inVIndex0;
						newQuad.indices[1] = inVIndex1;
						newQuad.indices[2] = newVIndex1;
						newQuad.indices[3] = newVIndex0;
						
						M3rDraw_Quad(
							&newQuad);
					}
																
					return;						
				}												
				/* else all are in */							
			}													
		}
	}
}

void
MSrClip_Quad(
	UUtUns32		inVIndex0,
	UUtUns32		inVIndex1,
	UUtUns32		inVIndex2,
	UUtUns32		inVIndex3,
	UUtUns32		inClipCode0,
	UUtUns32		inClipCode1,
	UUtUns32		inClipCode2,
	UUtUns32		inClipCode3,
	UUtUns32		inClippedPlanes)	/* Set to 0 */
{


	UUtUns8					newClipCode0;
	UUtUns8					newClipCode1;
	UUtUns32				newVIndex0;	
	UUtUns32				newVIndex1;	

	UUtInt32				i;
	UUtUns8					curClipPlane;
	MSrClip_PolyComputeVertexProc	computeVertexProc;
	
	M3tTri					newTri;
	M3tQuad					newQuad;
	M3tPent					newPent;
	
	if(inClipCode0 & inClipCode1 & inClipCode2 & inClipCode3)
	{
		return;
	}
	
	newVIndex0 = MSgGeomContextPrivate->objectVertexData.newClipVertexIndex++;
	newVIndex1 = MSgGeomContextPrivate->objectVertexData.newClipVertexIndex++;
	
	UUmAssert(MSgGeomContextPrivate->objectVertexData.newClipVertexIndex <= MSgGeomContextPrivate->objectVertexData.maxClipVertices);
	
	computeVertexProc = MSgGeomContextPrivate->polyComputeVertexProc;
	
	for(i = 6; i-- > 0;)
	{
		curClipPlane = (UUtUns8)(1 << i);
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
					computeVertexProc(
						curClipPlane,
						inVIndex3,			inVIndex0,			newVIndex0, 
						inVIndex3,			inVIndex2,			newVIndex1, 
						&newClipCode0,		&newClipCode1);
												
					/* Check for trivial accept on 3, new0, new1 */
					if(inClipCode3 | newClipCode0 | newClipCode1)
					{
						MSrClip_Triangle(
							inVIndex3,			newVIndex0,			newVIndex1,
							inClipCode3,		newClipCode0,		newClipCode1,
							inClippedPlanes | curClipPlane);
					}
					else
					{
						newTri.indices[0] = inVIndex3;
						newTri.indices[1] = newVIndex0;
						newTri.indices[2] = newVIndex1;
						
						M3rDraw_Triangle(
							&newTri);
					}
					return;
				}														
				else													
				{														
					/* 0: Out, 1: Out, 2: In, 3: ? */					
					if(curClipPlane & inClipCode3)							
					{													
						/* 0: Out, 1: Out, 2: In, 3: Out */				
						/* Compute 2 -> 1 intersection */				
						/* Compute 2 -> 3 intersection */				
						computeVertexProc(
							curClipPlane,
							inVIndex2,			inVIndex1,			newVIndex0, 
							inVIndex2,			inVIndex3,			newVIndex1, 
							&newClipCode0,		&newClipCode1);

						/* Check for trivial accept on 2, new1, new0 */	
						if(inClipCode2 | newClipCode1 | newClipCode0)
						{
							MSrClip_Triangle(
								inVIndex2,			newVIndex1,			newVIndex0,
								inClipCode2,		newClipCode1,		newClipCode0,
								inClippedPlanes | curClipPlane);
						}
						else
						{
							newTri.indices[0] = inVIndex2;
							newTri.indices[1] = newVIndex1;
							newTri.indices[2] = newVIndex0;
							
							M3rDraw_Triangle(
								&newTri);
						}
						return;
					}													
					else												
					{													
						/* 0: Out, 1: Out, 2: In, 3: In */				
						/* Compute 2 -> 1 intersection */				
						/* Compute 3 -> 0 intersection */				
						computeVertexProc(
							curClipPlane,
							inVIndex2,			inVIndex1,			newVIndex0, 
							inVIndex3,			inVIndex0,			newVIndex1, 
							&newClipCode0,		&newClipCode1);

						/* Check for trivial accept on 2, 3, new1, new0 */
						if(inClipCode2 | inClipCode3 | newClipCode1 | newClipCode0)
						{
							MSrClip_Quad(
								inVIndex2,			inVIndex3,			newVIndex1,			newVIndex0,
								inClipCode2,		inClipCode3,		newClipCode1,		newClipCode0,
								inClippedPlanes | curClipPlane);
						}
						else
						{
							newQuad.indices[0] = inVIndex2;
							newQuad.indices[1] = inVIndex3;
							newQuad.indices[2] = newVIndex1;
							newQuad.indices[3] = newVIndex0;
							
							M3rDraw_Quad(
								&newQuad);
						}
						return;
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
					computeVertexProc(
						curClipPlane,
						inVIndex1,			inVIndex0,			newVIndex0, 
						inVIndex1,			inVIndex2,			newVIndex1, 
						&newClipCode0,		&newClipCode1);

					/* Check for trivial accept on 1, new1, new0 */		
					if(inClipCode1 | newClipCode1 | newClipCode0)
					{
						MSrClip_Triangle(
							inVIndex1,			newVIndex1,			newVIndex0,
							inClipCode1,		newClipCode1,		newClipCode0,
							inClippedPlanes | curClipPlane);
					}
					else
					{
						newTri.indices[0] = inVIndex1;
						newTri.indices[1] = newVIndex1;
						newTri.indices[2] = newVIndex0;
						
						M3rDraw_Triangle(
							&newTri);
					}
					return;
				}														
				else													
				{														
					/* 0: Out, 1: In, 2: In, 3: ? */					
					if(curClipPlane & inClipCode3)							
					{													
						/* 0: Out, 1: In, 2: In, 3: Out */				
						/* Compute 1 -> 0 intersection */				
						/* Compute 2 -> 3 intersection */				
						computeVertexProc(
							curClipPlane,
							inVIndex1,			inVIndex0,			newVIndex0, 
							inVIndex2,			inVIndex3,			newVIndex1, 
							&newClipCode0,		&newClipCode1);

						/* Check for trivial accept on 1, 2, new1, new0 */
						if(inClipCode1 | inClipCode2 | newClipCode1 | newClipCode0)
						{
							MSrClip_Quad(
								inVIndex1,			inVIndex2,			newVIndex1,			newVIndex0,
								inClipCode1,		inClipCode2,		newClipCode1,		newClipCode0,
								inClippedPlanes | curClipPlane);
						}
						else
						{
							newQuad.indices[0] = inVIndex1;
							newQuad.indices[1] = inVIndex2;
							newQuad.indices[2] = newVIndex1;
							newQuad.indices[3] = newVIndex0;
							
							M3rDraw_Quad(
								&newQuad);
						}

						return;
					}													
					else												
					{													
						/* 0: Out, 1: In, 2: In, 3: In */				
						/* Compute 1 -> 0 intersection */				
						/* Compute 3 -> 0 intersection */				
						computeVertexProc(
							curClipPlane,
							inVIndex1,			inVIndex0,			newVIndex0, 
							inVIndex3,			inVIndex0,			newVIndex1, 
							&newClipCode0,		&newClipCode1);

						/* Check for trivial accept on 1, 2, 3, new1, new0 */
						if(inClipCode1 | inClipCode2 | inClipCode3 | newClipCode1 | newClipCode0)
						{
							if(inClipCode1 | inClipCode2 | inClipCode3 | newClipCode1)
							{
								MSrClip_Quad(
									inVIndex1,			inVIndex2,			inVIndex3,			newVIndex1,
									inClipCode1,		inClipCode2,		inClipCode3,		newClipCode1,
									inClippedPlanes | curClipPlane);
							}
							else
							{
								newQuad.indices[0] = inVIndex1;
								newQuad.indices[1] = inVIndex2;
								newQuad.indices[2] = inVIndex3;
								newQuad.indices[3] = newVIndex1;
								
								M3rDraw_Quad(
									&newQuad);
							}
							
							if(inClipCode1 | newClipCode1 | newClipCode0)
							{
								MSrClip_Triangle(
									inVIndex1,			newVIndex1,			newVIndex0,
									inClipCode1,		newClipCode1,		newClipCode0,
									inClippedPlanes | curClipPlane);
							}
							else
							{
								newTri.indices[0] = inVIndex1;
								newTri.indices[1] = newVIndex1;
								newTri.indices[2] = newVIndex0;
								
								M3rDraw_Triangle(
									&newTri);
							}
						}
						else
						{
							newPent.indices[0] = inVIndex1;
							newPent.indices[1] = inVIndex2;
							newPent.indices[2] = inVIndex3;
							newPent.indices[3] = newVIndex1;
							newPent.indices[4] = newVIndex0;
							
							M3rDraw_Pent(
								&newPent);
						}
						return;
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
						computeVertexProc(
							curClipPlane,
							inVIndex0,			inVIndex3,			newVIndex0, 
							inVIndex0,			inVIndex1,			newVIndex1, 
						
							&newClipCode0,		&newClipCode1);

						/* Check for trivial accept on 0, new1, new0 */	
						if(inClipCode0 | newClipCode1 | newClipCode0)
						{
							MSrClip_Triangle(
													inVIndex0,			newVIndex1,			newVIndex0,
								inClipCode0,		newClipCode1,		newClipCode0,
								inClippedPlanes | curClipPlane);
						}
						else
						{
							newTri.indices[0] = inVIndex0;
							newTri.indices[1] = newVIndex1;
							newTri.indices[2] = newVIndex0;
							
							M3rDraw_Triangle(
								&newTri);
						}
						return;
					}													
					else												
					{													
						/* 0: In, 1: Out, 2: Out, 3: In */				
						/* Compute 3 -> 2 intersection */				
						/* Compute 0 -> 1 intersection */				
						computeVertexProc(
							curClipPlane,
							inVIndex3,			inVIndex2,			newVIndex0, 
							inVIndex0,			inVIndex1,			newVIndex1, 
							&newClipCode0,		&newClipCode1);

						/* Check for trivial accept on 3, 0, new1, new0 */
						if(inClipCode3 | inClipCode0 | newClipCode1 | newClipCode0)
						{
							MSrClip_Quad(
								inVIndex3,			inVIndex0,			newVIndex1,			newVIndex0,
								inClipCode3,		inClipCode0,		newClipCode1,		newClipCode0,
								inClippedPlanes | curClipPlane);
						}
						else
						{
							newQuad.indices[0] = inVIndex3;
							newQuad.indices[1] = inVIndex0;
							newQuad.indices[2] = newVIndex1;
							newQuad.indices[3] = newVIndex0;
							
							M3rDraw_Quad(
								&newQuad);
						}

						return;
					}													
				}														
				else													
				{														
					/* 0: In, 1: Out, 2: In, 3: In */					
					//UUmAssert(!(curClipPlane & inClipCode3));		
					/* Compute 2 -> 1 intersection */					
					/* Compute 0 -> 1 intersection */					
					computeVertexProc(
						curClipPlane,
						inVIndex2,			inVIndex1,			newVIndex0, 
						inVIndex0,			inVIndex1,			newVIndex1, 
						&newClipCode0,		&newClipCode1);

					/* Check for trivial accept on 2, 3, 0, new1, new0 */
					if(inClipCode2 | inClipCode3 | inClipCode0 | newClipCode1 | newClipCode0)
					{
						if(inClipCode2 | inClipCode3 | inClipCode0 | newClipCode1)
						{
							MSrClip_Quad(
								inVIndex2,			inVIndex3,			inVIndex0,			newVIndex1,
								inClipCode2,		inClipCode3,		inClipCode0,		newClipCode1,
								inClippedPlanes | curClipPlane);
						}
						else
						{
							newQuad.indices[0] = inVIndex2;
							newQuad.indices[1] = inVIndex3;
							newQuad.indices[2] = inVIndex0;
							newQuad.indices[3] = newVIndex1;
							
							M3rDraw_Quad(
								&newQuad);
						}
						
						if(inClipCode2 | newClipCode1 | newClipCode0)
						{
							MSrClip_Triangle(
								inVIndex2,			newVIndex1,			newVIndex0,
								inClipCode2,		newClipCode1,		newClipCode0,
								inClippedPlanes | curClipPlane);
						}
						else
						{
							newTri.indices[0] = inVIndex2;
							newTri.indices[1] = newVIndex1;
							newTri.indices[2] = newVIndex0;
							
							M3rDraw_Triangle(
								&newTri);
						}
					}
					else
					{
						newPent.indices[0] = inVIndex2;
						newPent.indices[1] = inVIndex3;
						newPent.indices[2] = inVIndex0;
						newPent.indices[3] = newVIndex1;
						newPent.indices[4] = newVIndex0;
						
						M3rDraw_Pent(
							&newPent);
					}
					return;
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
						computeVertexProc(
							curClipPlane,
							inVIndex0,			inVIndex3,			newVIndex0, 
							inVIndex1,			inVIndex2,			newVIndex1, 
							&newClipCode0,		&newClipCode1);

						/* Check for trivial accept on 0, 1, new1, new0 */
						if(inClipCode0 | inClipCode1 | newClipCode1 | newClipCode0)
						{
							MSrClip_Quad(
													inVIndex0,			inVIndex1,			newVIndex1,			newVIndex0,
								inClipCode0,		inClipCode1,		newClipCode1,		newClipCode0,
								inClippedPlanes | curClipPlane);
						}
						else
						{
							newQuad.indices[0] = inVIndex0;
							newQuad.indices[1] = inVIndex1;
							newQuad.indices[2] = newVIndex1;
							newQuad.indices[3] = newVIndex0;
							
							M3rDraw_Quad(
								&newQuad);
						}

						return;
					}													
					else												
					{													
						/* 0: In, 1: In, 2: Out, 3: In */				
						/* Compute 3 -> 2 intersection */				
						/* Compute 1 -> 2 intersection */				
						computeVertexProc(
							curClipPlane,
							inVIndex3,			inVIndex2,			newVIndex0, 
							inVIndex1,			inVIndex2,			newVIndex1, 
							&newClipCode0,		&newClipCode1);

						/* Check for trivial accept on 3, 0, 1, new1, new0 */
						if(inClipCode3 | inClipCode0 | inClipCode1 | newClipCode1 | newClipCode0)
						{
							if(inClipCode3 | inClipCode0 | inClipCode1 | newClipCode1)
							{
								MSrClip_Quad(
															inVIndex3,			inVIndex0,			inVIndex1,			newVIndex1,
									inClipCode3,		inClipCode0,		inClipCode1,		newClipCode1,
									inClippedPlanes | curClipPlane);
							}
							else
							{
								newQuad.indices[0] = inVIndex3;
								newQuad.indices[1] = inVIndex0;
								newQuad.indices[2] = inVIndex1;
								newQuad.indices[3] = newVIndex1;
								
								M3rDraw_Quad(
									&newQuad);
							}
							
							if(inClipCode3 | newClipCode1 | newClipCode0)
							{
								MSrClip_Triangle(
									inVIndex3,			newVIndex1,			newVIndex0,
									inClipCode3,		newClipCode1,		newClipCode0,
									inClippedPlanes | curClipPlane);
							}
							else
							{
								newTri.indices[0] = inVIndex3;
								newTri.indices[1] = newVIndex1;
								newTri.indices[2] = newVIndex0;
								
								M3rDraw_Triangle(
									&newTri);
							}
						}
						else
						{
							newPent.indices[0] = inVIndex3;
							newPent.indices[1] = inVIndex0;
							newPent.indices[2] = inVIndex1;
							newPent.indices[3] = newVIndex1;
							newPent.indices[4] = newVIndex0;
							
							M3rDraw_Pent(
								&newPent);
						}
						return;
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
						computeVertexProc(
							curClipPlane,
							inVIndex0,			inVIndex3,			newVIndex0, 
							inVIndex2,			inVIndex3,			newVIndex1, 
							&newClipCode0,		&newClipCode1);

						/* Check for trivial accept on 0, 1, 2, new1, new0 */
						if(inClipCode0 | inClipCode1 | inClipCode2 | newClipCode1 | newClipCode0)
						{
							if(inClipCode0 | inClipCode1 | inClipCode2 | newClipCode1)
							{
								MSrClip_Quad(
									inVIndex0,			inVIndex1,			inVIndex2,			newVIndex1,
									inClipCode0,		inClipCode1,		inClipCode2,		newClipCode1,
									inClippedPlanes | curClipPlane);
							}
							else
							{
								newQuad.indices[0] = inVIndex0;
								newQuad.indices[1] = inVIndex1;
								newQuad.indices[2] = inVIndex2;
								newQuad.indices[3] = newVIndex1;
								
								M3rDraw_Quad(
									&newQuad);
							}
							
							if(inClipCode0 | newClipCode1 | newClipCode0)
							{
								MSrClip_Triangle(
									inVIndex0,			newVIndex1,			newVIndex0,
									inClipCode0,		newClipCode1,		newClipCode0,
									inClippedPlanes | curClipPlane);
							}
							else
							{
								newTri.indices[0] = inVIndex0;
								newTri.indices[1] = newVIndex1;
								newTri.indices[2] = newVIndex0;
								
								M3rDraw_Triangle(
									&newTri);
							}
						}
						else
						{
							newPent.indices[0] = inVIndex0;
							newPent.indices[1] = inVIndex1;
							newPent.indices[2] = inVIndex2;
							newPent.indices[3] = newVIndex1;
							newPent.indices[4] = newVIndex0;
							
							M3rDraw_Pent(
								&newPent);
						}
						return;
					}													
					/* else all are in */								
				}														
			}															
		}
	}
}
