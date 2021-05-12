/*
	FILE:	MS_GC_Env_Clip.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Jan 14, 1998
	
	PURPOSE: 
	
	Copyright 1997

*/

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Akira.h"

#include "Motoko_Manager.h"
#include "MS_GC_Env_Clip.h"
#include "MS_GC_Private.h"

#include "MS_GC_Method_Env.h"

#if UUmCompiler	== UUmCompiler_VisC
#pragma optimize( "g", off )
#endif


static void M3rDraw_Quad_Nvidia_Cheats(M3tQuadSplit *inQuad)
{
	M3tTriSplit tri;

	tri.baseUVIndices.indices[0] = inQuad->baseUVIndices.indices[0];
	tri.baseUVIndices.indices[1] = inQuad->baseUVIndices.indices[1];
	tri.baseUVIndices.indices[2] = inQuad->baseUVIndices.indices[2];

	tri.vertexIndices.indices[0] = inQuad->vertexIndices.indices[0];
	tri.vertexIndices.indices[1] = inQuad->vertexIndices.indices[1];
	tri.vertexIndices.indices[2] = inQuad->vertexIndices.indices[2];

	tri.shades[0] = inQuad->shades[0];
	tri.shades[1] = inQuad->shades[1];
	tri.shades[2] = inQuad->shades[2];

	M3rDraw_Triangle(&tri);

	tri.baseUVIndices.indices[0] = inQuad->baseUVIndices.indices[0];
	tri.baseUVIndices.indices[1] = inQuad->baseUVIndices.indices[2];
	tri.baseUVIndices.indices[2] = inQuad->baseUVIndices.indices[3];

	tri.vertexIndices.indices[0] = inQuad->vertexIndices.indices[0];
	tri.vertexIndices.indices[1] = inQuad->vertexIndices.indices[2];
	tri.vertexIndices.indices[2] = inQuad->vertexIndices.indices[3];

	tri.shades[0] = inQuad->shades[0];
	tri.shades[1] = inQuad->shades[2];
	tri.shades[2] = inQuad->shades[3];

	M3rDraw_Triangle(&tri);

	return;
}

static void M3rDraw_Pent_Nvidia_Cheats(M3tPentSplit *inPent)
{
	M3tTriSplit tri;

	tri.baseUVIndices.indices[0] = inPent->baseUVIndices.indices[0];
	tri.baseUVIndices.indices[1] = inPent->baseUVIndices.indices[1];
	tri.baseUVIndices.indices[2] = inPent->baseUVIndices.indices[2];

	tri.vertexIndices.indices[0] = inPent->vertexIndices.indices[0];
	tri.vertexIndices.indices[1] = inPent->vertexIndices.indices[1];
	tri.vertexIndices.indices[2] = inPent->vertexIndices.indices[2];

	tri.shades[0] = inPent->shades[0];
	tri.shades[1] = inPent->shades[1];
	tri.shades[2] = inPent->shades[2];

	M3rDraw_Triangle(&tri);

	tri.baseUVIndices.indices[0] = inPent->baseUVIndices.indices[0];
	tri.baseUVIndices.indices[1] = inPent->baseUVIndices.indices[2];
	tri.baseUVIndices.indices[2] = inPent->baseUVIndices.indices[3];

	tri.vertexIndices.indices[0] = inPent->vertexIndices.indices[0];
	tri.vertexIndices.indices[1] = inPent->vertexIndices.indices[2];
	tri.vertexIndices.indices[2] = inPent->vertexIndices.indices[3];

	tri.shades[0] = inPent->shades[0];
	tri.shades[1] = inPent->shades[2];
	tri.shades[2] = inPent->shades[3];

	M3rDraw_Triangle(&tri);

	tri.baseUVIndices.indices[0] = inPent->baseUVIndices.indices[0];
	tri.baseUVIndices.indices[1] = inPent->baseUVIndices.indices[3];
	tri.baseUVIndices.indices[2] = inPent->baseUVIndices.indices[4];

	tri.vertexIndices.indices[0] = inPent->vertexIndices.indices[0];
	tri.vertexIndices.indices[1] = inPent->vertexIndices.indices[3];
	tri.vertexIndices.indices[2] = inPent->vertexIndices.indices[4];

	tri.shades[0] = inPent->shades[0];
	tri.shades[1] = inPent->shades[3];
	tri.shades[2] = inPent->shades[4];

	M3rDraw_Triangle(&tri);

	return;
}


static UUtUns32 iInterpShade_NoAlpha(UUtUns32 src, UUtUns32 dst, float in_t)
{
	UUtUns32 t = MUrUnsignedSmallFloat_ShiftLeft_To_Uns_Round(in_t, 8);
	UUtUns32 one_minus_t = 0x100 - t;
	UUtUns32 r = ((((src & 0xFF0000) >> 16) * t) + (((dst & 0xFF0000) >> 16) * one_minus_t)) >> 8;
	UUtUns32 g = ((((src & 0x00FF00) >>  8) * t) + (((dst & 0x00FF00) >>  8) * one_minus_t)) >> 8;
	UUtUns32 b = ((((src & 0x0000FF) >>  0) * t) + (((dst & 0x0000FF) >>  0) * one_minus_t)) >> 8;
	UUtUns32 result = (r << 16) | (g << 8) | (b << 0);

	return result;
}

#define UUmAssert_Clip(x)

static UUtBool MUrFrustumPoint_IsZero(M3tPoint4D *inPoint)
{
	UUtBool is_zero = UUcTrue;
	
	is_zero &= UUmFloat_CompareEqu(inPoint->x, 0.f);
	is_zero &= UUmFloat_CompareEqu(inPoint->y, 0.f);
	is_zero &= UUmFloat_CompareEqu(inPoint->z, 0.f);
	is_zero &= UUmFloat_CompareEqu(inPoint->w, 0.f);

	return is_zero;
}

static void
MSrEnv_Clip_ComputeVertex_TriSplit(
	UUtUns32				inClipPlane,
	
	M3tTriSplit*			inTriSplit,
	
	UUtUns32				inQSVertexIn0,
	UUtUns32				inQSVertexOut0,
	
	UUtUns32				inVIndexNew0,
	UUtUns32				inBaseUVIndexNew0,
	UUtUns32				*inShadeNew0,
	UUtUns8					*outClipCodeNew0,
	
	UUtUns32				inQSVertexIn1,
	UUtUns32				inQSVertexOut1,

	UUtUns32				inVIndexNew1,
	UUtUns32				inBaseUVIndexNew1,
	UUtUns32				*inShadeNew1,
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
	
	frustumPoints = MSgGeomContextPrivate->gqVertexData.frustumPoints;
	screenPoints = MSgGeomContextPrivate->gqVertexData.screenPoints;
	textureCoords = MSgGeomContextPrivate->gqVertexData.textureCoords;
	
	scaleX = MSgGeomContextPrivate->scaleX;
	scaleY = MSgGeomContextPrivate->scaleY;
	
	for(i = 0; i < 2; i++)
	{
		if(i == 0)
		{
			frustumPointIn = frustumPoints + inTriSplit->vertexIndices.indices[inQSVertexIn0];
			frustumPointOut = frustumPoints + inTriSplit->vertexIndices.indices[inQSVertexOut0];
			frustumPointNew = frustumPoints + inVIndexNew0;
		}
		else
		{
			frustumPointIn = frustumPoints + inTriSplit->vertexIndices.indices[inQSVertexIn1];
			frustumPointOut = frustumPoints + inTriSplit->vertexIndices.indices[inQSVertexOut1];
			frustumPointNew = frustumPoints + inVIndexNew1;
		}

		UUmAssert(!MUrFrustumPoint_IsZero(frustumPointOut));
	
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
			textureCoordIn = textureCoords + inTriSplit->baseUVIndices.indices[inQSVertexIn0];
			textureCoordOut = textureCoords + inTriSplit->baseUVIndices.indices[inQSVertexOut0];
			textureCoordNew = textureCoords + inBaseUVIndexNew0;
		}
		else
		{
			textureCoordIn = textureCoords + inTriSplit->baseUVIndices.indices[inQSVertexIn1];
			textureCoordOut = textureCoords + inTriSplit->baseUVIndices.indices[inQSVertexOut1];
			textureCoordNew = textureCoords + inBaseUVIndexNew1;
		}
		textureCoordNew->u = t * textureCoordOut->u + oneMinust * textureCoordIn->u;
		textureCoordNew->v = t * textureCoordOut->v + oneMinust * textureCoordIn->v;
		
		if(i == 0)
		{
			*inShadeNew0 = iInterpShade_NoAlpha(inTriSplit->shades[inQSVertexOut0], inTriSplit->shades[inQSVertexIn0], t);
		}
		else
		{
			*inShadeNew1 = iInterpShade_NoAlpha(inTriSplit->shades[inQSVertexOut1], inTriSplit->shades[inQSVertexIn1], t);
		}
		
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

static void
MSrEnv_Clip_ComputeVertex_QuadSplit(
	UUtUns32				inClipPlane,
	
	M3tQuadSplit*			inQuadSplit,
	
	UUtUns32				inQSVertexIn0,
	UUtUns32				inQSVertexOut0,
	
	UUtUns32				inVIndexNew0,
	UUtUns32				inBaseUVIndexNew0,
	UUtUns32				*inShadeNew0,
	UUtUns8					*outClipCodeNew0,
	
	UUtUns32				inQSVertexIn1,
	UUtUns32				inQSVertexOut1,

	UUtUns32				inVIndexNew1,
	UUtUns32				inBaseUVIndexNew1,
	UUtUns32				*inShadeNew1,
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
	
	frustumPoints = MSgGeomContextPrivate->gqVertexData.frustumPoints;
	screenPoints = MSgGeomContextPrivate->gqVertexData.screenPoints;
	textureCoords = MSgGeomContextPrivate->gqVertexData.textureCoords;
	
	scaleX = MSgGeomContextPrivate->scaleX;
	scaleY = MSgGeomContextPrivate->scaleY;
	
	for(i = 0; i < 2; i++)
	{
		if(i == 0)
		{
			frustumPointIn = frustumPoints + inQuadSplit->vertexIndices.indices[inQSVertexIn0];
			frustumPointOut = frustumPoints + inQuadSplit->vertexIndices.indices[inQSVertexOut0];
			frustumPointNew = frustumPoints + inVIndexNew0;
		}
		else
		{
			frustumPointIn = frustumPoints + inQuadSplit->vertexIndices.indices[inQSVertexIn1];
			frustumPointOut = frustumPoints + inQuadSplit->vertexIndices.indices[inQSVertexOut1];
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
			textureCoordIn = textureCoords + inQuadSplit->baseUVIndices.indices[inQSVertexIn0];
			textureCoordOut = textureCoords + inQuadSplit->baseUVIndices.indices[inQSVertexOut0];
			textureCoordNew = textureCoords + inBaseUVIndexNew0;
		}
		else
		{
			textureCoordIn = textureCoords + inQuadSplit->baseUVIndices.indices[inQSVertexIn1];
			textureCoordOut = textureCoords + inQuadSplit->baseUVIndices.indices[inQSVertexOut1];
			textureCoordNew = textureCoords + inBaseUVIndexNew1;
		}
		textureCoordNew->u = t * textureCoordOut->u + oneMinust * textureCoordIn->u;
		textureCoordNew->v = t * textureCoordOut->v + oneMinust * textureCoordIn->v;
		
		if(i == 0)
		{
			*inShadeNew0 = iInterpShade_NoAlpha(inQuadSplit->shades[inQSVertexOut0], inQuadSplit->shades[inQSVertexIn0], t);
		}
		else
		{
			*inShadeNew1 = iInterpShade_NoAlpha(inQuadSplit->shades[inQSVertexOut1], inQuadSplit->shades[inQSVertexIn1], t);
		}
		
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

#if UUmCompiler	== UUmCompiler_VisC
#pragma optimize( "", on )
#endif

void
MSrEnv_Clip_Tri(
	M3tTriSplit*	inTriSplit,
	UUtUns8			inClipCode0,
	UUtUns8			inClipCode1,
	UUtUns8			inClipCode2,
	UUtUns8			inClippedPlanes)
{

	UUtUns8					newClipCode0;
	UUtUns8					newClipCode1;
	UUtUns32				newVIndex0;	
	UUtUns32				newVIndex1;	

	UUtUns32				newBaseUVIndex0;
	UUtUns32				newBaseUVIndex1;

	UUtInt32					i;
	UUtUns8					curClipPlane;
	
	M3tTriSplit				newTriSplit;
	M3tQuadSplit			newQuadSplit;


	UUmAssert(!MUrFrustumPoint_IsZero(MSgGeomContextPrivate->gqVertexData.frustumPoints + inTriSplit->vertexIndices.indices[0]));
	UUmAssert(!MUrFrustumPoint_IsZero(MSgGeomContextPrivate->gqVertexData.frustumPoints + inTriSplit->vertexIndices.indices[1]));
	UUmAssert(!MUrFrustumPoint_IsZero(MSgGeomContextPrivate->gqVertexData.frustumPoints + inTriSplit->vertexIndices.indices[2]));
	
	if(inClipCode0 & inClipCode1 & inClipCode2)
	{
		return;
	}
	
	newVIndex0 = MSgGeomContextPrivate->gqVertexData.newClipVertexIndex++;
	newVIndex1 = MSgGeomContextPrivate->gqVertexData.newClipVertexIndex++;
	
	UUmAssert(MSgGeomContextPrivate->gqVertexData.newClipVertexIndex <= MSgGeomContextPrivate->gqVertexData.maxClipVertices);
	
	newBaseUVIndex0 = MSgGeomContextPrivate->gqVertexData.newClipTextureIndex++;
	newBaseUVIndex1 = MSgGeomContextPrivate->gqVertexData.newClipTextureIndex++;
		
	UUmAssert(MSgGeomContextPrivate->gqVertexData.newClipTextureIndex <= MSgGeomContextPrivate->gqVertexData.maxClipTextureCords);
	
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
				MSrEnv_Clip_ComputeVertex_TriSplit(
					curClipPlane,
					inTriSplit,
					2, 0,
					newVIndex0, newBaseUVIndex0, &newTriSplit.shades[1], &newClipCode0,
					2, 1,
					newVIndex1, newBaseUVIndex1, &newTriSplit.shades[2], &newClipCode1);
				
				newTriSplit.vertexIndices.indices[0]	= inTriSplit->vertexIndices.indices[2];
				newTriSplit.baseUVIndices.indices[0]	= inTriSplit->baseUVIndices.indices[2];
				newTriSplit.shades[0]					= inTriSplit->shades[2];
				
				newTriSplit.vertexIndices.indices[1]	= newVIndex0;
				newTriSplit.baseUVIndices.indices[1]	= newBaseUVIndex0;
				
				newTriSplit.vertexIndices.indices[2]	= newVIndex1;
				newTriSplit.baseUVIndices.indices[2]	= newBaseUVIndex1;
				
				/* Check for trivial accept on 2, new0, new1 */	
				if(inClipCode2 | newClipCode0 | newClipCode1)
				{
					MSrEnv_Clip_Tri(
						&newTriSplit,
						inClipCode2,		newClipCode0,		newClipCode1,
						inClippedPlanes | curClipPlane);
				}
				else
				{
					M3rDraw_Triangle(
						&newTriSplit);
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
					MSrEnv_Clip_ComputeVertex_TriSplit(
						curClipPlane,
						inTriSplit,
						1, 0,
						newVIndex0, newBaseUVIndex0, &newTriSplit.shades[2], &newClipCode0,
						1, 2,
						newVIndex1, newBaseUVIndex1, &newTriSplit.shades[1], &newClipCode1);
				
					newTriSplit.vertexIndices.indices[0]	= inTriSplit->vertexIndices.indices[1];
					newTriSplit.baseUVIndices.indices[0]	= inTriSplit->baseUVIndices.indices[1];
					newTriSplit.shades[0]					= inTriSplit->shades[1];
					
					newTriSplit.vertexIndices.indices[1]	= newVIndex1;
					newTriSplit.baseUVIndices.indices[1]	= newBaseUVIndex1;
					
					newTriSplit.vertexIndices.indices[2]	= newVIndex0;
					newTriSplit.baseUVIndices.indices[2]	= newBaseUVIndex0;
					
					/* Check for trivial accept on 1, new1, new0 */
					if(inClipCode1 | newClipCode1 | newClipCode0)
					{
						MSrEnv_Clip_Tri(
							&newTriSplit,
							inClipCode1,		newClipCode1,		newClipCode0,
							inClippedPlanes | curClipPlane);
					}
					else
					{
						M3rDraw_Triangle(
							&newTriSplit);
					}
																
					return;						
				}												
				else											
				{												
					/* 0: Out, 1: In, 2: In */					
																
					/* Compute 1 -> 0 intersection */			
					/* Compute 2 -> 0 intersection */			
					MSrEnv_Clip_ComputeVertex_TriSplit(
						curClipPlane,
						inTriSplit,
						1, 0,
						newVIndex0, newBaseUVIndex0, &newQuadSplit.shades[3], &newClipCode0,
						2, 0,
						newVIndex1, newBaseUVIndex1, &newQuadSplit.shades[2], &newClipCode1);
					
					newQuadSplit.vertexIndices.indices[0]	= inTriSplit->vertexIndices.indices[1];
					newQuadSplit.baseUVIndices.indices[0]	= inTriSplit->baseUVIndices.indices[1];
					newQuadSplit.shades[0]					= inTriSplit->shades[1];
					
					newQuadSplit.vertexIndices.indices[1]	= inTriSplit->vertexIndices.indices[2];
					newQuadSplit.baseUVIndices.indices[1]	= inTriSplit->baseUVIndices.indices[2];
					newQuadSplit.shades[1]					= inTriSplit->shades[2];
					
					newQuadSplit.vertexIndices.indices[2]	= newVIndex1;
					newQuadSplit.baseUVIndices.indices[2]	= newBaseUVIndex1;
					
					newQuadSplit.vertexIndices.indices[3]	= newVIndex0;
					newQuadSplit.baseUVIndices.indices[3]	= newBaseUVIndex0;

					/* Check for trivial accept on 1, 2, new1, new0 */		
					if(inClipCode1 | inClipCode2 | newClipCode1 | newClipCode0)
					{
						MSrEnv_Clip_Quad(
							&newQuadSplit,
							inClipCode1,		inClipCode2,		newClipCode1,		newClipCode0,
							inClippedPlanes | curClipPlane);
					}
					else
					{
						M3rDraw_Quad(
							&newQuadSplit);
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
					MSrEnv_Clip_ComputeVertex_TriSplit(
						curClipPlane,
						inTriSplit,
						0, 1,
						newVIndex0, newBaseUVIndex0, &newTriSplit.shades[1], &newClipCode0,
						0, 2,
						newVIndex1, newBaseUVIndex1, &newTriSplit.shades[2], &newClipCode1);
					
					newTriSplit.vertexIndices.indices[0]	= inTriSplit->vertexIndices.indices[0];
					newTriSplit.baseUVIndices.indices[0]	= inTriSplit->baseUVIndices.indices[0];
					newTriSplit.shades[0]					= inTriSplit->shades[0];
					
					newTriSplit.vertexIndices.indices[1]	= newVIndex0;
					newTriSplit.baseUVIndices.indices[1]	= newBaseUVIndex0;
					
					newTriSplit.vertexIndices.indices[2]	= newVIndex1;
					newTriSplit.baseUVIndices.indices[2]	= newBaseUVIndex1;

					/* Check for trivial accept on 0, new0, new1 */
					if(inClipCode0 | newClipCode0 | newClipCode1)
					{
						MSrEnv_Clip_Tri(
							&newTriSplit,
							inClipCode0,		newClipCode0,		newClipCode1,
							inClippedPlanes | curClipPlane);
					}
					else
					{
						M3rDraw_Triangle(
							&newTriSplit);
					}
																
					return;						
				}												
				else											
				{												
					/* 0: In, 1: Out, 2: In */					
																
					/* Compute 0 -> 1 intersection */			
					/* Compute 2 -> 1 intersection */			
					MSrEnv_Clip_ComputeVertex_TriSplit(
						curClipPlane,
						inTriSplit,
						0, 1,
						newVIndex0, newBaseUVIndex0, &newQuadSplit.shades[2], &newClipCode0,
						2, 1,
						newVIndex1, newBaseUVIndex1, &newQuadSplit.shades[3], &newClipCode1);

					newQuadSplit.vertexIndices.indices[0]	= inTriSplit->vertexIndices.indices[2];
					newQuadSplit.baseUVIndices.indices[0]	= inTriSplit->baseUVIndices.indices[2];
					newQuadSplit.shades[0]					= inTriSplit->shades[2];
					
					newQuadSplit.vertexIndices.indices[1]	= inTriSplit->vertexIndices.indices[0];
					newQuadSplit.baseUVIndices.indices[1]	= inTriSplit->baseUVIndices.indices[0];
					newQuadSplit.shades[1]					= inTriSplit->shades[0];
					
					newQuadSplit.vertexIndices.indices[2]	= newVIndex0;
					newQuadSplit.baseUVIndices.indices[2]	= newBaseUVIndex0;
					
					newQuadSplit.vertexIndices.indices[3]	= newVIndex1;
					newQuadSplit.baseUVIndices.indices[3]	= newBaseUVIndex1;

					/* Check for trivial accept on 2, 0, new0, new1 */		
					if(inClipCode2 | inClipCode0 | newClipCode0 | newClipCode1)
					{
						MSrEnv_Clip_Quad(
							&newQuadSplit,
							inClipCode2,		inClipCode0,		newClipCode0,		newClipCode1,
							inClippedPlanes | curClipPlane);
					}
					else
					{
						M3rDraw_Quad(
							&newQuadSplit);
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
					MSrEnv_Clip_ComputeVertex_TriSplit(
						curClipPlane,
						inTriSplit,
						0, 2,
						newVIndex0, newBaseUVIndex0, &newQuadSplit.shades[3], &newClipCode0,
						1, 2,
						newVIndex1, newBaseUVIndex1, &newQuadSplit.shades[2], &newClipCode1);
																
					newQuadSplit.vertexIndices.indices[0]	= inTriSplit->vertexIndices.indices[0];
					newQuadSplit.baseUVIndices.indices[0]	= inTriSplit->baseUVIndices.indices[0];
					newQuadSplit.shades[0]					= inTriSplit->shades[0];
					
					newQuadSplit.vertexIndices.indices[1]	= inTriSplit->vertexIndices.indices[1];
					newQuadSplit.baseUVIndices.indices[1]	= inTriSplit->baseUVIndices.indices[1];
					newQuadSplit.shades[1]					= inTriSplit->shades[1];
					
					newQuadSplit.vertexIndices.indices[2]	= newVIndex1;
					newQuadSplit.baseUVIndices.indices[2]	= newBaseUVIndex1;
					
					newQuadSplit.vertexIndices.indices[3]	= newVIndex0;
					newQuadSplit.baseUVIndices.indices[3]	= newBaseUVIndex0;

					/* Check for trivial accept on 0, 1, new1, new0 */		
					if(inClipCode0 | inClipCode1 | newClipCode1 | newClipCode0)
					{
						MSrEnv_Clip_Quad(
							&newQuadSplit,
							inClipCode0,		inClipCode1,		newClipCode1,		newClipCode0,
							inClippedPlanes | curClipPlane);
					}
					else
					{
						M3rDraw_Quad(
							&newQuadSplit);
					}
																
					return;						
				}												
				/* else all are in */							
			}													
		}
	}

	return;
}

void
MSrEnv_Clip_Quad(
	M3tQuadSplit*	inQuadSplit,
	UUtUns8			inClipCode0,
	UUtUns8			inClipCode1,
	UUtUns8			inClipCode2,
	UUtUns8			inClipCode3,
	UUtUns8			inClippedPlanes)
{


	UUtUns8					newClipCode0;
	UUtUns8					newClipCode1;
	UUtUns32				newVIndex0;	
	UUtUns32				newVIndex1;	

	UUtUns32				newBaseUVIndex0;
	UUtUns32				newBaseUVIndex1;

	M3tTriSplit				newTriSplit;
	M3tQuadSplit			newQuadSplit;
	M3tPentSplit			newPentSplit;
	
	UUtInt32				i;
	UUtUns8					curClipPlane;

	UUmAssert(!MUrFrustumPoint_IsZero(MSgGeomContextPrivate->gqVertexData.frustumPoints + inQuadSplit->vertexIndices.indices[0]));
	UUmAssert(!MUrFrustumPoint_IsZero(MSgGeomContextPrivate->gqVertexData.frustumPoints + inQuadSplit->vertexIndices.indices[1]));
	UUmAssert(!MUrFrustumPoint_IsZero(MSgGeomContextPrivate->gqVertexData.frustumPoints + inQuadSplit->vertexIndices.indices[2]));
	UUmAssert(!MUrFrustumPoint_IsZero(MSgGeomContextPrivate->gqVertexData.frustumPoints + inQuadSplit->vertexIndices.indices[3]));

	if(inClipCode0 & inClipCode1 & inClipCode2 & inClipCode3)
	{
		return;
	}
	
	newVIndex0 = MSgGeomContextPrivate->gqVertexData.newClipVertexIndex++;
	newVIndex1 = MSgGeomContextPrivate->gqVertexData.newClipVertexIndex++;
	
	UUmAssert(MSgGeomContextPrivate->gqVertexData.newClipVertexIndex <= MSgGeomContextPrivate->gqVertexData.maxClipVertices);
	
	newBaseUVIndex0 = MSgGeomContextPrivate->gqVertexData.newClipTextureIndex++;
	newBaseUVIndex1 = MSgGeomContextPrivate->gqVertexData.newClipTextureIndex++;

	UUmAssert(MSgGeomContextPrivate->gqVertexData.newClipTextureIndex <= MSgGeomContextPrivate->gqVertexData.maxClipTextureCords);
	
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
					MSrEnv_Clip_ComputeVertex_QuadSplit(
						curClipPlane,
						inQuadSplit,
						3, 0,
						newVIndex0, newBaseUVIndex0, &newTriSplit.shades[1], &newClipCode0,
						3, 2,
						newVIndex1, newBaseUVIndex1, &newTriSplit.shades[2], &newClipCode1);
												
					newTriSplit.vertexIndices.indices[0]	= inQuadSplit->vertexIndices.indices[3];
					newTriSplit.baseUVIndices.indices[0]	= inQuadSplit->baseUVIndices.indices[3];
					newTriSplit.shades[0]					= inQuadSplit->shades[3];
					
					newTriSplit.vertexIndices.indices[1]	= newVIndex0;
					newTriSplit.baseUVIndices.indices[1]	= newBaseUVIndex0;
					
					newTriSplit.vertexIndices.indices[2]	= newVIndex1;
					newTriSplit.baseUVIndices.indices[2]	= newBaseUVIndex1;
					
					/* Check for trivial accept on 3, new0, new1 */
					if(inClipCode3 | newClipCode0 | newClipCode1)
					{
						MSrEnv_Clip_Tri(
							&newTriSplit,
							inClipCode3,		newClipCode0,		newClipCode1,
							inClippedPlanes | curClipPlane);
					}
					else
					{
						M3rDraw_Triangle(
							&newTriSplit);
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
						MSrEnv_Clip_ComputeVertex_QuadSplit(
							curClipPlane,
							inQuadSplit,
							2, 1,
							newVIndex0, newBaseUVIndex0, &newTriSplit.shades[2], &newClipCode0,
							2, 3,
							newVIndex1, newBaseUVIndex1, &newTriSplit.shades[1], &newClipCode1);

						newTriSplit.vertexIndices.indices[0]	= inQuadSplit->vertexIndices.indices[2];
						newTriSplit.baseUVIndices.indices[0]	= inQuadSplit->baseUVIndices.indices[2];
						newTriSplit.shades[0]					= inQuadSplit->shades[2];
						
						newTriSplit.vertexIndices.indices[1]	= newVIndex1;
						newTriSplit.baseUVIndices.indices[1]	= newBaseUVIndex1;
						
						newTriSplit.vertexIndices.indices[2]	= newVIndex0;
						newTriSplit.baseUVIndices.indices[2]	= newBaseUVIndex0;

						/* Check for trivial accept on 2, new1, new0 */	
						if(inClipCode2 | newClipCode1 | newClipCode0)
						{
							MSrEnv_Clip_Tri(
								&newTriSplit,
								inClipCode2,		newClipCode1,		newClipCode0,
								inClippedPlanes | curClipPlane);
						}
						else
						{
							M3rDraw_Triangle(
								&newTriSplit);
						}
						return;
					}													
					else												
					{													
						/* 0: Out, 1: Out, 2: In, 3: In */				
						/* Compute 2 -> 1 intersection */				
						/* Compute 3 -> 0 intersection */				
						MSrEnv_Clip_ComputeVertex_QuadSplit(
							curClipPlane,
							inQuadSplit,
							2, 1,
							newVIndex0, newBaseUVIndex0, &newQuadSplit.shades[3], &newClipCode0,
							3, 0,
							newVIndex1, newBaseUVIndex1, &newQuadSplit.shades[2], &newClipCode1);

						newQuadSplit.vertexIndices.indices[0]	= inQuadSplit->vertexIndices.indices[2];
						newQuadSplit.baseUVIndices.indices[0]	= inQuadSplit->baseUVIndices.indices[2];
						newQuadSplit.shades[0]					= inQuadSplit->shades[2];
						
						newQuadSplit.vertexIndices.indices[1]	= inQuadSplit->vertexIndices.indices[3];
						newQuadSplit.baseUVIndices.indices[1]	= inQuadSplit->baseUVIndices.indices[3];
						newQuadSplit.shades[1]					= inQuadSplit->shades[3];
						
						newQuadSplit.vertexIndices.indices[2]	= newVIndex1;
						newQuadSplit.baseUVIndices.indices[2]	= newBaseUVIndex1;
						
						newQuadSplit.vertexIndices.indices[3]	= newVIndex0;
						newQuadSplit.baseUVIndices.indices[3]	= newBaseUVIndex0;

						/* Check for trivial accept on 2, 3, new1, new0 */
						if(inClipCode2 | inClipCode3 | newClipCode1 | newClipCode0)
						{
							MSrEnv_Clip_Quad(
								&newQuadSplit,
								inClipCode2,		inClipCode3,		newClipCode1,		newClipCode0,
								inClippedPlanes | curClipPlane);
						}
						else
						{
							M3rDraw_Quad(
								&newQuadSplit);
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
					MSrEnv_Clip_ComputeVertex_QuadSplit(
						curClipPlane,
						inQuadSplit,
						1, 0,
						newVIndex0, newBaseUVIndex0, &newTriSplit.shades[2], &newClipCode0,
						1, 2,
						newVIndex1, newBaseUVIndex1, &newTriSplit.shades[1], &newClipCode1);

					newTriSplit.vertexIndices.indices[0]	= inQuadSplit->vertexIndices.indices[1];
					newTriSplit.baseUVIndices.indices[0]	= inQuadSplit->baseUVIndices.indices[1];
					newTriSplit.shades[0]					= inQuadSplit->shades[1];
					
					newTriSplit.vertexIndices.indices[1]	= newVIndex1;
					newTriSplit.baseUVIndices.indices[1]	= newBaseUVIndex1;
					
					newTriSplit.vertexIndices.indices[2]	= newVIndex0;
					newTriSplit.baseUVIndices.indices[2]	= newBaseUVIndex0;
					
					/* Check for trivial accept on 1, new1, new0 */		
					if(inClipCode1 | newClipCode1 | newClipCode0)
					{
						MSrEnv_Clip_Tri(
							&newTriSplit,
							inClipCode1,		newClipCode1,		newClipCode0,
							inClippedPlanes | curClipPlane);
					}
					else
					{
						M3rDraw_Triangle(
							&newTriSplit);
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
						MSrEnv_Clip_ComputeVertex_QuadSplit(
							curClipPlane,
							inQuadSplit,
							1, 0,
							newVIndex0, newBaseUVIndex0, &newQuadSplit.shades[3], &newClipCode0,
							2, 3,
							newVIndex1, newBaseUVIndex1, &newQuadSplit.shades[2], &newClipCode1);

						newQuadSplit.vertexIndices.indices[0]	= inQuadSplit->vertexIndices.indices[1];
						newQuadSplit.baseUVIndices.indices[0]	= inQuadSplit->baseUVIndices.indices[1];
						newQuadSplit.shades[0]					= inQuadSplit->shades[1];
						
						newQuadSplit.vertexIndices.indices[1]	= inQuadSplit->vertexIndices.indices[2];
						newQuadSplit.baseUVIndices.indices[1]	= inQuadSplit->baseUVIndices.indices[2];
						newQuadSplit.shades[1]					= inQuadSplit->shades[2];
						
						newQuadSplit.vertexIndices.indices[2]	= newVIndex1;
						newQuadSplit.baseUVIndices.indices[2]	= newBaseUVIndex1;
						
						newQuadSplit.vertexIndices.indices[3]	= newVIndex0;
						newQuadSplit.baseUVIndices.indices[3]	= newBaseUVIndex0;

						/* Check for trivial accept on 1, 2, new1, new0 */
						if(inClipCode1 | inClipCode2 | newClipCode1 | newClipCode0)
						{
							MSrEnv_Clip_Quad(
								&newQuadSplit,
								inClipCode1,		inClipCode2,		newClipCode1,		newClipCode0,
								inClippedPlanes | curClipPlane);
						}
						else
						{
							M3rDraw_Quad(
								&newQuadSplit);
						}

						return;
					}													
					else												
					{													
						/* 0: Out, 1: In, 2: In, 3: In */				
						/* Compute 1 -> 0 intersection */				
						/* Compute 3 -> 0 intersection */				
						MSrEnv_Clip_ComputeVertex_QuadSplit(
							curClipPlane,
							inQuadSplit,
							1, 0,
							newVIndex0, newBaseUVIndex0, &newPentSplit.shades[4], &newClipCode0,
							3, 0,
							newVIndex1, newBaseUVIndex1, &newPentSplit.shades[3], &newClipCode1);

						/* Check for trivial accept on 1, 2, 3, new1, new0 */
						if(inClipCode1 | inClipCode2 | inClipCode3 | newClipCode1 | newClipCode0)
						{
							newQuadSplit.vertexIndices.indices[0]	= inQuadSplit->vertexIndices.indices[1];
							newQuadSplit.baseUVIndices.indices[0]	= inQuadSplit->baseUVIndices.indices[1];
							newQuadSplit.shades[0]					= inQuadSplit->shades[1];
							
							newQuadSplit.vertexIndices.indices[1]	= inQuadSplit->vertexIndices.indices[2];
							newQuadSplit.baseUVIndices.indices[1]	= inQuadSplit->baseUVIndices.indices[2];
							newQuadSplit.shades[1]					= inQuadSplit->shades[2];
							
							newQuadSplit.vertexIndices.indices[2]	= inQuadSplit->vertexIndices.indices[3];
							newQuadSplit.baseUVIndices.indices[2]	= inQuadSplit->baseUVIndices.indices[3];
							newQuadSplit.shades[2]					= inQuadSplit->shades[3];						

							newQuadSplit.vertexIndices.indices[3]	= newVIndex1;
							newQuadSplit.baseUVIndices.indices[3]	= newBaseUVIndex1;
							newQuadSplit.shades[3]					= newPentSplit.shades[3];

							if(inClipCode1 | inClipCode2 | inClipCode3 | newClipCode1)
							{
								MSrEnv_Clip_Quad(
									&newQuadSplit,
									inClipCode1,		inClipCode2,		inClipCode3,		newClipCode1,
									inClippedPlanes | curClipPlane);
							}
							else
							{
								M3rDraw_Quad(
									&newQuadSplit);
							}
							
							newTriSplit.vertexIndices.indices[0]	= inQuadSplit->vertexIndices.indices[1];
							newTriSplit.baseUVIndices.indices[0]	= inQuadSplit->baseUVIndices.indices[1];
							newTriSplit.shades[0]					= inQuadSplit->shades[1];
							
							newTriSplit.vertexIndices.indices[1]	= newVIndex1;
							newTriSplit.baseUVIndices.indices[1]	= newBaseUVIndex1;
							newTriSplit.shades[1]					= newPentSplit.shades[3];
							
							newTriSplit.vertexIndices.indices[2]	= newVIndex0;
							newTriSplit.baseUVIndices.indices[2]	= newBaseUVIndex0;
							newTriSplit.shades[2]					= newPentSplit.shades[4];

							if(inClipCode1 | newClipCode1 | newClipCode0)
							{
								MSrEnv_Clip_Tri(
									&newTriSplit,
									inClipCode1, newClipCode1, newClipCode0,
									inClippedPlanes | curClipPlane);
							}
							else
							{
								M3rDraw_Triangle(
									&newTriSplit);
							}
						}
						else
						{
							newPentSplit.vertexIndices.indices[0]	= inQuadSplit->vertexIndices.indices[1];
							newPentSplit.baseUVIndices.indices[0]	= inQuadSplit->baseUVIndices.indices[1];
							newPentSplit.shades[0]					= inQuadSplit->shades[1];
							
							newPentSplit.vertexIndices.indices[1]	= inQuadSplit->vertexIndices.indices[2];
							newPentSplit.baseUVIndices.indices[1]	= inQuadSplit->baseUVIndices.indices[2];
							newPentSplit.shades[1]					= inQuadSplit->shades[2];
							
							newPentSplit.vertexIndices.indices[2]	= inQuadSplit->vertexIndices.indices[3];
							newPentSplit.baseUVIndices.indices[2]	= inQuadSplit->baseUVIndices.indices[3];
							newPentSplit.shades[2]					= inQuadSplit->shades[3];
							
							newPentSplit.vertexIndices.indices[3]	= newVIndex1;
							newPentSplit.baseUVIndices.indices[3]	= newBaseUVIndex1;
							newPentSplit.shades[3]					= newPentSplit.shades[3];
							
							newPentSplit.vertexIndices.indices[4]	= newVIndex0;
							newPentSplit.baseUVIndices.indices[4]	= newBaseUVIndex0;
							newPentSplit.shades[4]					= newPentSplit.shades[4];

							M3rDraw_Pent(
								&newPentSplit);
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
						MSrEnv_Clip_ComputeVertex_QuadSplit(
							curClipPlane,
							inQuadSplit,
							0, 3,
							newVIndex0, newBaseUVIndex0, &newTriSplit.shades[2], &newClipCode0,
							0, 1,
							newVIndex1, newBaseUVIndex1, &newTriSplit.shades[1], &newClipCode1);

						newTriSplit.vertexIndices.indices[0]	= inQuadSplit->vertexIndices.indices[0];
						newTriSplit.baseUVIndices.indices[0]	= inQuadSplit->baseUVIndices.indices[0];
						newTriSplit.shades[0]					= inQuadSplit->shades[0];
						
						newTriSplit.vertexIndices.indices[1]	= newVIndex1;
						newTriSplit.baseUVIndices.indices[1]	= newBaseUVIndex1;
						
						newTriSplit.vertexIndices.indices[2]	= newVIndex0;
						newTriSplit.baseUVIndices.indices[2]	= newBaseUVIndex0;

						/* Check for trivial accept on 0, new1, new0 */	
						if(inClipCode0 | newClipCode1 | newClipCode0)
						{
							MSrEnv_Clip_Tri(
								&newTriSplit,
								inClipCode0,		newClipCode1,		newClipCode0,
								inClippedPlanes | curClipPlane);
						}
						else
						{
							M3rDraw_Triangle(
								&newTriSplit);
						}
						return;
					}													
					else												
					{													
						/* 0: In, 1: Out, 2: Out, 3: In */				
						/* Compute 3 -> 2 intersection */				
						/* Compute 0 -> 1 intersection */				
						MSrEnv_Clip_ComputeVertex_QuadSplit(
							curClipPlane,
							inQuadSplit,
							3, 2,
							newVIndex0, newBaseUVIndex0, &newQuadSplit.shades[3], &newClipCode0,
							0, 1,
							newVIndex1, newBaseUVIndex1, &newQuadSplit.shades[2], &newClipCode1);

						newQuadSplit.vertexIndices.indices[0]	= inQuadSplit->vertexIndices.indices[3];
						newQuadSplit.baseUVIndices.indices[0]	= inQuadSplit->baseUVIndices.indices[3];
						newQuadSplit.shades[0]					= inQuadSplit->shades[3];
						
						newQuadSplit.vertexIndices.indices[1]	= inQuadSplit->vertexIndices.indices[0];
						newQuadSplit.baseUVIndices.indices[1]	= inQuadSplit->baseUVIndices.indices[0];
						newQuadSplit.shades[1]					= inQuadSplit->shades[0];
						
						newQuadSplit.vertexIndices.indices[2]	= newVIndex1;
						newQuadSplit.baseUVIndices.indices[2]	= newBaseUVIndex1;
						
						newQuadSplit.vertexIndices.indices[3]	= newVIndex0;
						newQuadSplit.baseUVIndices.indices[3]	= newBaseUVIndex0;

						/* Check for trivial accept on 3, 0, new1, new0 */
						if(inClipCode3 | inClipCode0 | newClipCode1 | newClipCode0)
						{
							MSrEnv_Clip_Quad(
								&newQuadSplit,
								inClipCode3,		inClipCode0,		newClipCode1,		newClipCode0,
								inClippedPlanes | curClipPlane);
						}
						else
						{
							M3rDraw_Quad(
								&newQuadSplit);
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
					MSrEnv_Clip_ComputeVertex_QuadSplit(
						curClipPlane,
						inQuadSplit,
						2, 1,
						newVIndex0, newBaseUVIndex0, &newPentSplit.shades[4], &newClipCode0,
						0, 1,
						newVIndex1, newBaseUVIndex1, &newPentSplit.shades[3], &newClipCode1);

					/* Check for trivial accept on 2, 3, 0, new1, new0 */
					if(inClipCode2 | inClipCode3 | inClipCode0 | newClipCode1 | newClipCode0)
					{
						newQuadSplit.vertexIndices.indices[0]	= inQuadSplit->vertexIndices.indices[2];
						newQuadSplit.baseUVIndices.indices[0]	= inQuadSplit->baseUVIndices.indices[2];
						newQuadSplit.shades[0]					= inQuadSplit->shades[2];
						
						newQuadSplit.vertexIndices.indices[1]	= inQuadSplit->vertexIndices.indices[3];
						newQuadSplit.baseUVIndices.indices[1]	= inQuadSplit->baseUVIndices.indices[3];
						newQuadSplit.shades[1]					= inQuadSplit->shades[3];
						
						newQuadSplit.vertexIndices.indices[2]	= inQuadSplit->vertexIndices.indices[0];
						newQuadSplit.baseUVIndices.indices[2]	= inQuadSplit->baseUVIndices.indices[0];
						newQuadSplit.shades[2]					= inQuadSplit->shades[0];
						
						newQuadSplit.vertexIndices.indices[3]	= newVIndex1;
						newQuadSplit.baseUVIndices.indices[3]	= newBaseUVIndex1;
						newQuadSplit.shades[3]					= newPentSplit.shades[3];

						if(inClipCode2 | inClipCode3 | inClipCode0 | newClipCode1)
						{
							MSrEnv_Clip_Quad(
								&newQuadSplit,
								inClipCode2,		inClipCode3,		inClipCode0,		newClipCode1,
								inClippedPlanes | curClipPlane);
						}
						else
						{
							M3rDraw_Quad(
								&newQuadSplit);
						}
						
						newTriSplit.vertexIndices.indices[0]	= inQuadSplit->vertexIndices.indices[2];
						newTriSplit.baseUVIndices.indices[0]	= inQuadSplit->baseUVIndices.indices[2];
						newTriSplit.shades[0]					= inQuadSplit->shades[2];
						
						newTriSplit.vertexIndices.indices[1]	= newVIndex1;
						newTriSplit.baseUVIndices.indices[1]	= newBaseUVIndex1;
						newTriSplit.shades[1]					= newPentSplit.shades[3];
						
						newTriSplit.vertexIndices.indices[2]	= newVIndex0;
						newTriSplit.baseUVIndices.indices[2]	= newBaseUVIndex0;
						newTriSplit.shades[2]					= newPentSplit.shades[4];

						if(inClipCode2 | newClipCode1 | newClipCode0)
						{
							MSrEnv_Clip_Tri(
								&newTriSplit,
								inClipCode2,		newClipCode1,		newClipCode0,
								inClippedPlanes | curClipPlane);
						}
						else
						{
							M3rDraw_Triangle(
								&newTriSplit);
						}
					}
					else
					{
						newPentSplit.vertexIndices.indices[0]	= inQuadSplit->vertexIndices.indices[2];
						newPentSplit.baseUVIndices.indices[0]	= inQuadSplit->baseUVIndices.indices[2];
						newPentSplit.shades[0]					= inQuadSplit->shades[2];
						
						newPentSplit.vertexIndices.indices[1]	= inQuadSplit->vertexIndices.indices[3];
						newPentSplit.baseUVIndices.indices[1]	= inQuadSplit->baseUVIndices.indices[3];
						newPentSplit.shades[1]					= inQuadSplit->shades[3];
						
						newPentSplit.vertexIndices.indices[2]	= inQuadSplit->vertexIndices.indices[0];
						newPentSplit.baseUVIndices.indices[2]	= inQuadSplit->baseUVIndices.indices[0];
						newPentSplit.shades[2]					= inQuadSplit->shades[0];
						
						newPentSplit.vertexIndices.indices[3]	= newVIndex1;
						newPentSplit.baseUVIndices.indices[3]	= newBaseUVIndex1;
						newPentSplit.shades[3]					= newPentSplit.shades[3];
						
						newPentSplit.vertexIndices.indices[4]	= newVIndex0;
						newPentSplit.baseUVIndices.indices[4]	= newBaseUVIndex0;
						newPentSplit.shades[4]					= newPentSplit.shades[4];

						M3rDraw_Pent(
							&newPentSplit);
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
						MSrEnv_Clip_ComputeVertex_QuadSplit(
							curClipPlane,
							inQuadSplit,
							0, 3,
							newVIndex0, newBaseUVIndex0, &newQuadSplit.shades[3], &newClipCode0,
							1, 2,
							newVIndex1, newBaseUVIndex1, &newQuadSplit.shades[2], &newClipCode1);

						newQuadSplit.vertexIndices.indices[0]	= inQuadSplit->vertexIndices.indices[0];
						newQuadSplit.baseUVIndices.indices[0]	= inQuadSplit->baseUVIndices.indices[0];
						newQuadSplit.shades[0]					= inQuadSplit->shades[0];
						
						newQuadSplit.vertexIndices.indices[1]	= inQuadSplit->vertexIndices.indices[1];
						newQuadSplit.baseUVIndices.indices[1]	= inQuadSplit->baseUVIndices.indices[1];
						newQuadSplit.shades[1]					= inQuadSplit->shades[1];
						
						newQuadSplit.vertexIndices.indices[2]	= newVIndex1;
						newQuadSplit.baseUVIndices.indices[2]	= newBaseUVIndex1;
						
						newQuadSplit.vertexIndices.indices[3]	= newVIndex0;
						newQuadSplit.baseUVIndices.indices[3]	= newBaseUVIndex0;

						/* Check for trivial accept on 0, 1, new1, new0 */
						if(inClipCode0 | inClipCode1 | newClipCode1 | newClipCode0)
						{
							MSrEnv_Clip_Quad(
								&newQuadSplit,
								inClipCode0,		inClipCode1,		newClipCode1,		newClipCode0,
								inClippedPlanes | curClipPlane);
						}
						else
						{
							M3rDraw_Quad(
								&newQuadSplit);
						}

						return;
					}													
					else												
					{													
						/* 0: In, 1: In, 2: Out, 3: In */				
						/* Compute 3 -> 2 intersection */				
						/* Compute 1 -> 2 intersection */				
						MSrEnv_Clip_ComputeVertex_QuadSplit(
							curClipPlane,
							inQuadSplit,
							3, 2,
							newVIndex0, newBaseUVIndex0, &newPentSplit.shades[4], &newClipCode0,
							1, 2,
							newVIndex1, newBaseUVIndex1, &newPentSplit.shades[3], &newClipCode1);

						/* Check for trivial accept on 3, 0, 1, new1, new0 */
						if(inClipCode3 | inClipCode0 | inClipCode1 | newClipCode1 | newClipCode0)
						{
							newQuadSplit.vertexIndices.indices[0]	= inQuadSplit->vertexIndices.indices[3];
							newQuadSplit.baseUVIndices.indices[0]	= inQuadSplit->baseUVIndices.indices[3];
							newQuadSplit.shades[0]					= inQuadSplit->shades[3];
							
							newQuadSplit.vertexIndices.indices[1]	= inQuadSplit->vertexIndices.indices[0];
							newQuadSplit.baseUVIndices.indices[1]	= inQuadSplit->baseUVIndices.indices[0];
							newQuadSplit.shades[1]					= inQuadSplit->shades[0];
							
							newQuadSplit.vertexIndices.indices[2]	= inQuadSplit->vertexIndices.indices[1];
							newQuadSplit.baseUVIndices.indices[2]	= inQuadSplit->baseUVIndices.indices[1];
							newQuadSplit.shades[2]					= inQuadSplit->shades[1];							

							newQuadSplit.vertexIndices.indices[3]	= newVIndex1;
							newQuadSplit.baseUVIndices.indices[3]	= newBaseUVIndex1;
							newQuadSplit.shades[3]					= newPentSplit.shades[3];

							if(inClipCode3 | inClipCode0 | inClipCode1 | newClipCode1)
							{
								MSrEnv_Clip_Quad(
									&newQuadSplit,
									inClipCode3,		inClipCode0,		inClipCode1,		newClipCode1,
									inClippedPlanes | curClipPlane);
							}
							else
							{
								M3rDraw_Quad(
									&newQuadSplit);
							}
							
							newTriSplit.vertexIndices.indices[0]	= inQuadSplit->vertexIndices.indices[3];
							newTriSplit.baseUVIndices.indices[0]	= inQuadSplit->baseUVIndices.indices[3];
							newTriSplit.shades[0]					= inQuadSplit->shades[3];
							
							newTriSplit.vertexIndices.indices[1]	= newVIndex1;
							newTriSplit.baseUVIndices.indices[1]	= newBaseUVIndex1;
							newTriSplit.shades[1]					= newPentSplit.shades[3];
							
							newTriSplit.vertexIndices.indices[2]	= newVIndex0;
							newTriSplit.baseUVIndices.indices[2]	= newBaseUVIndex0;
							newTriSplit.shades[2]					= newPentSplit.shades[4];

							if(inClipCode3 | newClipCode1 | newClipCode0)
							{
								MSrEnv_Clip_Tri(
									&newTriSplit,
									inClipCode3,		newClipCode1,		newClipCode0,
									inClippedPlanes | curClipPlane);
							}
							else
							{
								M3rDraw_Triangle(
									&newTriSplit);
							}
						}
						else
						{
							newPentSplit.vertexIndices.indices[0]	= inQuadSplit->vertexIndices.indices[3];
							newPentSplit.baseUVIndices.indices[0]	= inQuadSplit->baseUVIndices.indices[3];
							newPentSplit.shades[0]					= inQuadSplit->shades[3];
							
							newPentSplit.vertexIndices.indices[1]	= inQuadSplit->vertexIndices.indices[0];
							newPentSplit.baseUVIndices.indices[1]	= inQuadSplit->baseUVIndices.indices[0];
							newPentSplit.shades[1]					= inQuadSplit->shades[0];
							
							newPentSplit.vertexIndices.indices[2]	= inQuadSplit->vertexIndices.indices[1];
							newPentSplit.baseUVIndices.indices[2]	= inQuadSplit->baseUVIndices.indices[1];
							newPentSplit.shades[2]					= inQuadSplit->shades[1];
							
							newPentSplit.vertexIndices.indices[3]	= newVIndex1;
							newPentSplit.baseUVIndices.indices[3]	= newBaseUVIndex1;
							newPentSplit.shades[3]					= newPentSplit.shades[3];
							
							newPentSplit.vertexIndices.indices[4]	= newVIndex0;
							newPentSplit.baseUVIndices.indices[4]	= newBaseUVIndex0;
							newPentSplit.shades[4]					= newPentSplit.shades[4];

							M3rDraw_Pent(
								&newPentSplit);
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
						MSrEnv_Clip_ComputeVertex_QuadSplit(
							curClipPlane,
							inQuadSplit,
							0, 3,
							newVIndex0, newBaseUVIndex0, &newPentSplit.shades[4], &newClipCode0,
							2, 3,
							newVIndex1, newBaseUVIndex1, &newPentSplit.shades[3], &newClipCode1);

						/* Check for trivial accept on 0, 1, 2, new1, new0 */
						if(inClipCode0 | inClipCode1 | inClipCode2 | newClipCode1 | newClipCode0)
						{
							newQuadSplit.vertexIndices.indices[0]	= inQuadSplit->vertexIndices.indices[0];
							newQuadSplit.baseUVIndices.indices[0]	= inQuadSplit->baseUVIndices.indices[0];
							newQuadSplit.shades[0]					= inQuadSplit->shades[0];
							
							newQuadSplit.vertexIndices.indices[1]	= inQuadSplit->vertexIndices.indices[1];
							newQuadSplit.baseUVIndices.indices[1]	= inQuadSplit->baseUVIndices.indices[1];
							newQuadSplit.shades[1]					= inQuadSplit->shades[1];
							
							newQuadSplit.vertexIndices.indices[2]	= inQuadSplit->vertexIndices.indices[2];
							newQuadSplit.baseUVIndices.indices[2]	= inQuadSplit->baseUVIndices.indices[2];
							newQuadSplit.shades[2]					= inQuadSplit->shades[2];
							
							newQuadSplit.vertexIndices.indices[3]	= newVIndex1;
							newQuadSplit.baseUVIndices.indices[3]	= newBaseUVIndex1;
							newQuadSplit.shades[3]					= newPentSplit.shades[3];

							if(inClipCode0 | inClipCode1 | inClipCode2 | newClipCode1)
							{
								MSrEnv_Clip_Quad(
									&newQuadSplit,
									inClipCode0,		inClipCode1,		inClipCode2,		newClipCode1,
									inClippedPlanes | curClipPlane);
							}
							else
							{
								M3rDraw_Quad(
									&newQuadSplit);
							}
							
							newTriSplit.vertexIndices.indices[0]	= inQuadSplit->vertexIndices.indices[0];
							newTriSplit.baseUVIndices.indices[0]	= inQuadSplit->baseUVIndices.indices[0];
							newTriSplit.shades[0]					= inQuadSplit->shades[0];
							
							newTriSplit.vertexIndices.indices[1]	= newVIndex1;
							newTriSplit.baseUVIndices.indices[1]	= newBaseUVIndex1;
							newTriSplit.shades[1]					= newPentSplit.shades[3];
							
							newTriSplit.vertexIndices.indices[2]	= newVIndex0;
							newTriSplit.baseUVIndices.indices[2]	= newBaseUVIndex0;
							newTriSplit.shades[2]					= newPentSplit.shades[4];

							if(inClipCode0 | newClipCode1 | newClipCode0)
							{
								MSrEnv_Clip_Tri(
									&newTriSplit,
									inClipCode0,		newClipCode1,		newClipCode0,
									inClippedPlanes | curClipPlane);
							}
							else
							{
								M3rDraw_Triangle(
									&newTriSplit);
							}
						}
						else
						{
							newPentSplit.vertexIndices.indices[0]	= inQuadSplit->vertexIndices.indices[0];
							newPentSplit.baseUVIndices.indices[0]	= inQuadSplit->baseUVIndices.indices[0];
							newPentSplit.shades[0]					= inQuadSplit->shades[0];
							
							newPentSplit.vertexIndices.indices[1]	= inQuadSplit->vertexIndices.indices[1];
							newPentSplit.baseUVIndices.indices[1]	= inQuadSplit->baseUVIndices.indices[1];
							newPentSplit.shades[1]					= inQuadSplit->shades[1];
							
							newPentSplit.vertexIndices.indices[2]	= inQuadSplit->vertexIndices.indices[2];
							newPentSplit.baseUVIndices.indices[2]	= inQuadSplit->baseUVIndices.indices[2];
							newPentSplit.shades[2]					= inQuadSplit->shades[2];
							
							newPentSplit.vertexIndices.indices[3]	= newVIndex1;
							newPentSplit.baseUVIndices.indices[3]	= newBaseUVIndex1;
							newPentSplit.shades[3]					= newPentSplit.shades[3];
							
							newPentSplit.vertexIndices.indices[4]	= newVIndex0;
							newPentSplit.baseUVIndices.indices[4]	= newBaseUVIndex0;
							newPentSplit.shades[4]					= newPentSplit.shades[4];

							M3rDraw_Pent(
								&newPentSplit);
						}
						return;
					}													
					/* else all are in */								
				}														
			}															
		}
	}
}

