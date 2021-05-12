/*
	FILE:	MG_DC_CreateVertex.c
	
	AUTHOR:	Brent Pease
	
	CREATED: July 31, 1999
	
	PURPOSE: 
	
	Copyright 1997 - 1999

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_BitVector.h"


#include "Motoko_Manager.h"

#include "MG_DC_Private.h"
#include "MG_DC_CreateVertex.h"

void
MGrVertexCreate_XYZ(
	void)
{
	UUtUns16		numVertices;
	GrVertex*		curGlideVertex;
	M3tPointScreen*	curMotokoVertex;
	UUtUns16		vertexItr;
	UUtUns16		block8;
	const UUtUns32*	curBVPtr;
	UUtUns32		curBV;
	
	UUmAssert(sizeof(GrVertex) == 15 * sizeof(float));
	UUmAssert(sizeof(GrVertex) * 8 == UUcProcessor_CacheLineSize * 15);
	
	numVertices = (UUtUns16)MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_NumRealVertices];
	curBVPtr = MGgDrawContextPrivate->statePtr[M3cDrawStatePtrType_VertexBitVector];
	
	curMotokoVertex = MGmGetScreenPoints(MGgDrawContextPrivate);
	
	curGlideVertex = MGgDrawContextPrivate->vertexList;
	
	block8 = numVertices >> 3;
	
	curBV = 0xFFFFFFFF;
	
	for(vertexItr = 0;
		vertexItr < block8;
		vertexItr++, curGlideVertex += 8, curMotokoVertex += 8)
	{
		if(curBVPtr != NULL)
		{
			curBV = curBVPtr[vertexItr >> 2] >> ((vertexItr & 0x3) << 3);
			
			if(!curBV) continue;
		}

		UUrProcessor_ZeroCacheLine((char*)curGlideVertex, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 1, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 2, 0);
		
		if(curBV & (1 << 0))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 0, curGlideVertex + 0);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 3, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 4, 0);

		if(curBV & (1 << 1))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 1, curGlideVertex + 1);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 5, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 6, 0);

		if(curBV & (1 << 2))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 2, curGlideVertex + 2);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 7, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 8, 0);

		if(curBV & (1 << 3))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 3, curGlideVertex + 3);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 9, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 10, 0);

		if(curBV & (1 << 4))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 4, curGlideVertex + 4);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 11, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 12, 0);

		if(curBV & (1 << 5))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 5, curGlideVertex + 5);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 13, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 14, 0);

		if(curBV & (1 << 6))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 6, curGlideVertex + 6);
		}
		
		if(curBV & (1 << 7))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 7, curGlideVertex + 7);
		}
	}
	
	// don't forget trailing vertices
	
	for(vertexItr = block8 * 8;
		vertexItr < numVertices;
		vertexItr++, curGlideVertex++, curMotokoVertex++)
	{
		if(curBVPtr == NULL || UUrBitVector_TestBit(curBVPtr, vertexItr))
		{
			MGmConvertVertex_XYZ(curMotokoVertex, curGlideVertex);
		}	
	}
	
}

void
MGrVertexCreate_XYZ_RGB(
	void)
{
	UUtUns16		numVertices;
	GrVertex*		curGlideVertex;
	M3tPointScreen*	curMotokoVertex;
	UUtUns16		vertexItr;
	UUtUns16		block8;
	const UUtUns32*	curBVPtr;
	UUtUns32		curBV;
	UUtUns32*		curVertexShade = MGmGetVertexShades(MGgDrawContextPrivate);
	
	UUmAssert(sizeof(GrVertex) == 15 * sizeof(float));
	UUmAssert(sizeof(GrVertex) * 8 == UUcProcessor_CacheLineSize * 15);
	
	numVertices = (UUtUns16)MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_NumRealVertices];
	curBVPtr = MGgDrawContextPrivate->statePtr[M3cDrawStatePtrType_VertexBitVector];
	
	curMotokoVertex = MGmGetScreenPoints(MGgDrawContextPrivate);
	
	curGlideVertex = MGgDrawContextPrivate->vertexList;
	
	block8 = numVertices >> 3;
	
	curBV = 0xFF;
	
	for(vertexItr = 0;
		vertexItr < block8;
		vertexItr++, curGlideVertex += 8, curMotokoVertex += 8, curVertexShade += 8)
	{
		if(curBVPtr != NULL)
		{
			curBV = curBVPtr[vertexItr >> 2] >> ((vertexItr & 0x3) << 3);
			
			if(!curBV) continue;
		}

		UUrProcessor_ZeroCacheLine((char*)curGlideVertex, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 1, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 2, 0);
		
		if(curBV & (1 << 0))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 0, curGlideVertex + 0);
			MGmConvertVertex_RGB(curVertexShade[0], curGlideVertex + 0);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 3, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 4, 0);

		if(curBV & (1 << 1))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 1, curGlideVertex + 1);
			MGmConvertVertex_RGB(curVertexShade[1], curGlideVertex + 1);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 5, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 6, 0);

		if(curBV & (1 << 2))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 2, curGlideVertex + 2);
			MGmConvertVertex_RGB(curVertexShade[2], curGlideVertex + 2);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 7, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 8, 0);

		if(curBV & (1 << 3))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 3, curGlideVertex + 3);
			MGmConvertVertex_RGB(curVertexShade[3], curGlideVertex + 3);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 9, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 10, 0);

		if(curBV & (1 << 4))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 4, curGlideVertex + 4);
			MGmConvertVertex_RGB(curVertexShade[4], curGlideVertex + 4);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 11, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 12, 0);

		if(curBV & (1 << 5))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 5, curGlideVertex + 5);
			MGmConvertVertex_RGB(curVertexShade[5], curGlideVertex + 5);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 13, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 14, 0);

		if(curBV & (1 << 6))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 6, curGlideVertex + 6);
			MGmConvertVertex_RGB(curVertexShade[6], curGlideVertex + 6);
		}
		
		if(curBV & (1 << 7))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 7, curGlideVertex + 7);
			MGmConvertVertex_RGB(curVertexShade[7], curGlideVertex + 7);
		}
	}
	
	// don't forget trailing vertices
	
	for(vertexItr = block8 * 8;
		vertexItr < numVertices;
		vertexItr++, curGlideVertex++, curMotokoVertex++, curVertexShade++)
	{
		if(curBVPtr == NULL || UUrBitVector_TestBit(curBVPtr, vertexItr))
		{
			MGmConvertVertex_XYZ(curMotokoVertex, curGlideVertex);
			MGmConvertVertex_RGB(curVertexShade[0], curGlideVertex);
		}	
	}
	
}

void
MGrVertexCreate_XYZ_BaseUV(
	void)
{
	UUtUns16				numVertices;
	GrVertex*				curGlideVertex;
	M3tPointScreen*			curMotokoVertex;
	UUtUns16				vertexItr;
	UUtUns16				block8;
	const UUtUns32*			curBVPtr;
	UUtUns32				curBV;
	MGtTextureMapPrivate*	baseMapPrivate = MGmGetBaseMapPrivate(MGgDrawContextPrivate);
	M3tTextureCoord*		curTextureCoord = MGmGetTextureCoords(MGgDrawContextPrivate);
	float					uScale, vScale;
	
	UUmAssert(sizeof(GrVertex) == 15 * sizeof(float));
	UUmAssert(sizeof(GrVertex) * 8 == UUcProcessor_CacheLineSize * 15);
	
	numVertices = (UUtUns16)MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_NumRealVertices];
	curBVPtr = MGgDrawContextPrivate->statePtr[M3cDrawStatePtrType_VertexBitVector];
	
	curMotokoVertex = MGmGetScreenPoints(MGgDrawContextPrivate);
	
	curGlideVertex = MGgDrawContextPrivate->vertexList;
	
	block8 = numVertices >> 3;
	
	curBV = 0xFFFFFFFF;
	
	uScale = baseMapPrivate->u_scale;
	vScale = baseMapPrivate->v_scale;
	
	for(vertexItr = 0;
		vertexItr < block8;
		vertexItr++, curGlideVertex += 8, curMotokoVertex += 8, curTextureCoord += 8)
	{
		if(curBVPtr != NULL)
		{
			curBV = curBVPtr[vertexItr >> 2] >> ((vertexItr & 0x3) << 3);
			
			if(!curBV) continue;
		}

		UUrProcessor_ZeroCacheLine((char*)curGlideVertex, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 1, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 2, 0);
		
		if(curBV & (1 << 0))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 0, curGlideVertex + 0);
			MGmConvertVertex_UV(uScale, vScale, 0, curTextureCoord + 0, curGlideVertex + 0);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 3, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 4, 0);

		if(curBV & (1 << 1))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 1, curGlideVertex + 1);
			MGmConvertVertex_UV(uScale, vScale, 0, curTextureCoord + 1, curGlideVertex + 1);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 5, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 6, 0);

		if(curBV & (1 << 2))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 2, curGlideVertex + 2);
			MGmConvertVertex_UV(uScale, vScale, 0, curTextureCoord + 2, curGlideVertex + 2);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 7, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 8, 0);

		if(curBV & (1 << 3))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 3, curGlideVertex + 3);
			MGmConvertVertex_UV(uScale, vScale, 0, curTextureCoord + 3, curGlideVertex + 3);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 9, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 10, 0);

		if(curBV & (1 << 4))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 4, curGlideVertex + 4);
			MGmConvertVertex_UV(uScale, vScale, 0, curTextureCoord + 4, curGlideVertex + 4);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 11, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 12, 0);

		if(curBV & (1 << 5))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 5, curGlideVertex + 5);
			MGmConvertVertex_UV(uScale, vScale, 0, curTextureCoord + 5, curGlideVertex + 5);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 13, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 14, 0);

		if(curBV & (1 << 6))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 6, curGlideVertex + 6);
			MGmConvertVertex_UV(uScale, vScale, 0, curTextureCoord + 6, curGlideVertex + 6);
		}
		
		if(curBV & (1 << 7))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 7, curGlideVertex + 7);
			MGmConvertVertex_UV(uScale, vScale, 0, curTextureCoord + 7, curGlideVertex + 7);
		}
	}
	
	// don't forget trailing vertices
	for(vertexItr = block8 * 8;
		vertexItr < numVertices;
		vertexItr++, curGlideVertex++, curMotokoVertex++, curBV <<= 1, curTextureCoord++)
	{
		if(curBVPtr == NULL || UUrBitVector_TestBit(curBVPtr, vertexItr))
		{
			MGmConvertVertex_XYZ(curMotokoVertex, curGlideVertex);
			MGmConvertVertex_UV(uScale, vScale, 0, curTextureCoord, curGlideVertex);
		}	
	}
}

void
MGrVertexCreate_XYZ_RGB_BaseUV(
	void)
#if UUmPlatform == UUmPlatform_Win32
{
	UUtUns16				numVertices;
	GrVertex*				curGlideVertex;
	M3tPointScreen*			curMotokoVertex;
	UUtUns16				vertexItr;
	const UUtUns32*			curBVPtr;
	UUtUns32				curBV;
	MGtTextureMapPrivate*	baseMapPrivate = MGmGetBaseMapPrivate(MGgDrawContextPrivate);
	M3tTextureCoord*		curTextureCoord = MGmGetTextureCoords(MGgDrawContextPrivate);
	UUtUns32*				curVertexShade = MGmGetVertexShades(MGgDrawContextPrivate);
	float					uScale, vScale;
	
	UUmAssert(sizeof(GrVertex) == 15 * sizeof(float));
	UUmAssert(sizeof(GrVertex) * 8 == UUcProcessor_CacheLineSize * 15);
	
	numVertices = (UUtUns16)MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_NumRealVertices];
	curBVPtr = MGgDrawContextPrivate->statePtr[M3cDrawStatePtrType_VertexBitVector];
	
	curMotokoVertex = MGmGetScreenPoints(MGgDrawContextPrivate);
	
	curGlideVertex = MGgDrawContextPrivate->vertexList;
		
	curBV = 0xFFFFFFFF;
	
	uScale = baseMapPrivate->u_scale;
	vScale = baseMapPrivate->v_scale;
	
	for(vertexItr = 0;
		vertexItr < numVertices;
		vertexItr++, curGlideVertex += 1, curMotokoVertex += 1, curTextureCoord += 1, curVertexShade += 1)
	{
		if ((curBVPtr != NULL) && (0 == (vertexItr % 8))) {
			curBV = curBVPtr[vertexItr >> 5] >> (((vertexItr >> 3) & 0x3) << 3);
				
			if(!curBV) continue;
		}
		
		if(curBV & (1 << (vertexItr % 8)))
		{
			MGmConvertVertex_XYZ(curMotokoVertex, curGlideVertex);
			MGmConvertVertex_UV(uScale, vScale, 0, curTextureCoord, curGlideVertex );
			MGmConvertVertex_RGB(*curVertexShade, curGlideVertex);
		}
	}	
}
#else
{
	UUtUns16				numVertices;
	GrVertex*				curGlideVertex;
	M3tPointScreen*			curMotokoVertex;
	UUtUns16				vertexItr;
	UUtUns16				block8;
	const UUtUns32*			curBVPtr;
	UUtUns32				curBV;
	MGtTextureMapPrivate*	baseMapPrivate = MGmGetBaseMapPrivate(MGgDrawContextPrivate);
	M3tTextureCoord*		curTextureCoord = MGmGetTextureCoords(MGgDrawContextPrivate);
	UUtUns32*				curVertexShade = MGmGetVertexShades(MGgDrawContextPrivate);
	float					uScale, vScale;
	
	UUmAssert(sizeof(GrVertex) == 15 * sizeof(float));
	UUmAssert(sizeof(GrVertex) * 8 == UUcProcessor_CacheLineSize * 15);
	
	numVertices = (UUtUns16)MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_NumRealVertices];
	curBVPtr = MGgDrawContextPrivate->statePtr[M3cDrawStatePtrType_VertexBitVector];
	
	curMotokoVertex = MGmGetScreenPoints(MGgDrawContextPrivate);
	
	curGlideVertex = MGgDrawContextPrivate->vertexList;
	
	block8 = numVertices >> 3;
	
	curBV = 0xFFFFFFFF;
	
	uScale = baseMapPrivate->u_scale;
	vScale = baseMapPrivate->v_scale;
	
	for(vertexItr = 0;
		vertexItr < block8;
		vertexItr++, curGlideVertex += 8, curMotokoVertex += 8, curTextureCoord += 8, curVertexShade += 8)
	{
		if(curBVPtr != NULL)
		{
			curBV = curBVPtr[vertexItr >> 2] >> ((vertexItr & 0x3) << 3);
			
			if(!curBV) continue;
		}

		UUrProcessor_ZeroCacheLine((char*)curGlideVertex, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 1, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 2, 0);
		
		if(curBV & (1 << 0))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 0, curGlideVertex + 0);
			MGmConvertVertex_UV(uScale, vScale, 0, curTextureCoord + 0, curGlideVertex + 0);
			MGmConvertVertex_RGB(curVertexShade[0], curGlideVertex + 0);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 3, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 4, 0);

		if(curBV & (1 << 1))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 1, curGlideVertex + 1);
			MGmConvertVertex_UV(uScale, vScale, 0, curTextureCoord + 1, curGlideVertex + 1);
			MGmConvertVertex_RGB(curVertexShade[1], curGlideVertex + 1);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 5, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 6, 0);

		if(curBV & (1 << 2))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 2, curGlideVertex + 2);
			MGmConvertVertex_UV(uScale, vScale, 0, curTextureCoord + 2, curGlideVertex + 2);
			MGmConvertVertex_RGB(curVertexShade[2], curGlideVertex + 2);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 7, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 8, 0);

		if(curBV & (1 << 3))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 3, curGlideVertex + 3);
			MGmConvertVertex_UV(uScale, vScale, 0, curTextureCoord + 3, curGlideVertex + 3);
			MGmConvertVertex_RGB(curVertexShade[3], curGlideVertex + 3);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 9, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 10, 0);

		if(curBV & (1 << 4))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 4, curGlideVertex + 4);
			MGmConvertVertex_UV(uScale, vScale, 0, curTextureCoord + 4, curGlideVertex + 4);
			MGmConvertVertex_RGB(curVertexShade[4], curGlideVertex + 4);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 11, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 12, 0);

		if(curBV & (1 << 5))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 5, curGlideVertex + 5);
			MGmConvertVertex_UV(uScale, vScale, 0, curTextureCoord + 5, curGlideVertex + 5);
			MGmConvertVertex_RGB(curVertexShade[5], curGlideVertex + 5);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 13, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 14, 0);

		if(curBV & (1 << 6))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 6, curGlideVertex + 6);
			MGmConvertVertex_UV(uScale, vScale, 0, curTextureCoord + 6, curGlideVertex + 6);
			MGmConvertVertex_RGB(curVertexShade[6], curGlideVertex + 6);
		}
		
		if(curBV & (1 << 7))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 7, curGlideVertex + 7);
			MGmConvertVertex_UV(uScale, vScale, 0, curTextureCoord + 7, curGlideVertex + 7);
			MGmConvertVertex_RGB(curVertexShade[7], curGlideVertex + 7);
		}
	}
	
	// don't forget trailing vertices
	for(vertexItr = block8 * 8;
		vertexItr < numVertices;
		vertexItr++, curGlideVertex++, curMotokoVertex++, curBV <<= 1, curTextureCoord++, curVertexShade++)
	{
		if(curBVPtr == NULL || UUrBitVector_TestBit(curBVPtr, vertexItr))
		{
			MGmConvertVertex_XYZ(curMotokoVertex, curGlideVertex);
			MGmConvertVertex_UV(uScale, vScale, 0, curTextureCoord, curGlideVertex);
			MGmConvertVertex_RGB(curVertexShade[0], curGlideVertex);
		}	
	}
}
#endif

void
MGrVertexCreate_XYZ_RGB_BaseUV_EnvUV(
	void)
{
	UUtUns16				numVertices;
	GrVertex*				curGlideVertex;
	M3tPointScreen*			curMotokoVertex;
	UUtUns16				vertexItr;
	UUtUns16				block8;
	const UUtUns32*			curBVPtr;
	UUtUns32				curBV;
	MGtTextureMapPrivate*	baseMapPrivate = MGmGetBaseMapPrivate(MGgDrawContextPrivate);
	MGtTextureMapPrivate*	envMapPrivate = MGmGetEnvMapPrivate(MGgDrawContextPrivate);
	M3tTextureCoord*		curTextureCoord = MGmGetTextureCoords(MGgDrawContextPrivate);
	M3tTextureCoord*		curEnvTextureCoord = MGmGetEnvTextureCoords(MGgDrawContextPrivate);
	UUtUns32*				curVertexShade = MGmGetVertexShades(MGgDrawContextPrivate);
	float					uScaleBase, vScaleBase;
	float					uScaleEnv, vScaleEnv;
	
	UUmAssert(sizeof(GrVertex) == 15 * sizeof(float));
	UUmAssert(sizeof(GrVertex) * 8 == UUcProcessor_CacheLineSize * 15);
	
	numVertices = (UUtUns16)MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_NumRealVertices];
	curBVPtr = MGgDrawContextPrivate->statePtr[M3cDrawStatePtrType_VertexBitVector];
	
	curMotokoVertex = MGmGetScreenPoints(MGgDrawContextPrivate);
	
	curGlideVertex = MGgDrawContextPrivate->vertexList;
	
	block8 = numVertices >> 3;
	
	curBV = 0xFFFFFFFF;
	
	uScaleBase = baseMapPrivate->u_scale;
	vScaleBase = baseMapPrivate->v_scale;
	uScaleEnv = envMapPrivate->u_scale;
	vScaleEnv = envMapPrivate->v_scale;
	
	for(vertexItr = 0;
		vertexItr < block8;
		vertexItr++, curGlideVertex += 8, curMotokoVertex += 8, curTextureCoord += 8, curEnvTextureCoord += 8, curVertexShade += 8)
	{
		if(curBVPtr != NULL)
		{
			curBV = curBVPtr[vertexItr >> 2] >> ((vertexItr & 0x3) << 3);
			
			if(!curBV) continue;
		}

		UUrProcessor_ZeroCacheLine((char*)curGlideVertex, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 1, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 2, 0);
		
		if(curBV & (1 << 0))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 0, curGlideVertex + 0);
			MGmConvertVertex_UV(uScaleBase, vScaleBase, 0, curTextureCoord + 0, curGlideVertex + 0);
			MGmConvertVertex_UV(uScaleEnv, vScaleEnv, 1, curEnvTextureCoord + 0, curGlideVertex + 0);
			MGmConvertVertex_RGB(curVertexShade[0], curGlideVertex + 0);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 3, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 4, 0);

		if(curBV & (1 << 1))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 1, curGlideVertex + 1);
			MGmConvertVertex_UV(uScaleBase, vScaleBase, 0, curTextureCoord + 1, curGlideVertex + 1);
			MGmConvertVertex_UV(uScaleEnv, vScaleEnv, 1, curEnvTextureCoord + 1, curGlideVertex + 1);
			MGmConvertVertex_RGB(curVertexShade[1], curGlideVertex + 1);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 5, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 6, 0);

		if(curBV & (1 << 2))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 2, curGlideVertex + 2);
			MGmConvertVertex_UV(uScaleBase, vScaleBase, 0, curTextureCoord + 2, curGlideVertex + 2);
			MGmConvertVertex_UV(uScaleEnv, vScaleEnv, 1, curEnvTextureCoord + 2, curGlideVertex + 2);
			MGmConvertVertex_RGB(curVertexShade[2], curGlideVertex + 2);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 7, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 8, 0);

		if(curBV & (1 << 3))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 3, curGlideVertex + 3);
			MGmConvertVertex_UV(uScaleBase, vScaleBase, 0, curTextureCoord + 3, curGlideVertex + 3);
			MGmConvertVertex_UV(uScaleEnv, vScaleEnv, 1, curEnvTextureCoord + 3, curGlideVertex + 3);
			MGmConvertVertex_RGB(curVertexShade[3], curGlideVertex + 3);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 9, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 10, 0);

		if(curBV & (1 << 4))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 4, curGlideVertex + 4);
			MGmConvertVertex_UV(uScaleBase, vScaleBase, 0, curTextureCoord + 4, curGlideVertex + 4);
			MGmConvertVertex_UV(uScaleEnv, vScaleEnv, 1, curEnvTextureCoord + 4, curGlideVertex + 4);
			MGmConvertVertex_RGB(curVertexShade[4], curGlideVertex + 4);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 11, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 12, 0);

		if(curBV & (1 << 5))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 5, curGlideVertex + 5);
			MGmConvertVertex_UV(uScaleBase, vScaleBase, 0, curTextureCoord + 5, curGlideVertex + 5);
			MGmConvertVertex_UV(uScaleEnv, vScaleEnv, 1, curEnvTextureCoord + 5, curGlideVertex + 5);
			MGmConvertVertex_RGB(curVertexShade[5], curGlideVertex + 5);
		}
		
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 13, 0);
		UUrProcessor_ZeroCacheLine((char*)curGlideVertex + UUcProcessor_CacheLineSize * 14, 0);

		if(curBV & (1 << 6))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 6, curGlideVertex + 6);
			MGmConvertVertex_UV(uScaleBase, vScaleBase, 0, curTextureCoord + 6, curGlideVertex + 6);
			MGmConvertVertex_UV(uScaleEnv, vScaleEnv, 1, curEnvTextureCoord + 6, curGlideVertex + 6);
			MGmConvertVertex_RGB(curVertexShade[6], curGlideVertex + 6);
		}
		
		if(curBV & (1 << 7))
		{
			MGmConvertVertex_XYZ(curMotokoVertex + 7, curGlideVertex + 7);
			MGmConvertVertex_UV(uScaleBase, vScaleBase, 0, curTextureCoord + 7, curGlideVertex + 7);
			MGmConvertVertex_UV(uScaleEnv, vScaleEnv, 1, curEnvTextureCoord + 7, curGlideVertex + 7);
			MGmConvertVertex_RGB(curVertexShade[7], curGlideVertex + 7);
		}
	}
	
	// don't forget trailing vertices
	for(vertexItr = block8 * 8;
		vertexItr < numVertices;
		vertexItr++, curGlideVertex++, curMotokoVertex++, curBV <<= 1, curEnvTextureCoord++, curTextureCoord++, curVertexShade++)
	{
		if(curBVPtr == NULL || UUrBitVector_TestBit(curBVPtr, vertexItr))
		{
			MGmConvertVertex_XYZ(curMotokoVertex, curGlideVertex);
			MGmConvertVertex_UV(uScaleBase, vScaleBase, 0, curTextureCoord, curGlideVertex);
			MGmConvertVertex_UV(uScaleEnv, vScaleEnv, 1, curEnvTextureCoord, curGlideVertex);
			MGmConvertVertex_RGB(curVertexShade[0], curGlideVertex);
		}	
	}
}
