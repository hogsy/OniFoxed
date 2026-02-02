/*
	FILE:	Motoko_Utility.c
	
	AUTHOR:	Brent H. Pease, Michael Evans
	
	CREATED: May 5, 1997
	
	PURPOSE: Interface to the Motoko 3D engine
	
	Copyright 1997-1999

*/

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_MathLib.h"
#include "BFW_Akira.h"

#include "BFW_TM_Construction.h"
#include "EnvFileFormat.h"

UUtBool M3rIntersect_GeomGeom(
   const M3tMatrix4x3*		inMatrixA,
   const M3tGeometry*		inGeometryA,
   const M3tMatrix4x3*		inMatrixB,
   const M3tGeometry*		inGeometryB
   )
{
	UUtBool collide = UUcFalse;
	M3tPoint3D centerA;
	M3tPoint3D centerB;
	float bothBoundingRadius;

	MUrMatrix_MultiplyPoints(1, inMatrixA, &inGeometryA->pointArray->boundingSphere.center, &centerA);
	MUrMatrix_MultiplyPoints(1, inMatrixB, &inGeometryB->pointArray->boundingSphere.center, &centerB);

	bothBoundingRadius = inGeometryA->pointArray->boundingSphere.radius + inGeometryB->pointArray->boundingSphere.radius;

	if (MUrPoint_Distance_Greater(&centerA, &centerB, bothBoundingRadius)) {
		return UUcFalse;
	}

	if (0)
	{
		M3tBoundingBox bboxA;
		M3tBoundingBox bboxB;

		M3rMinMaxBBox_To_BBox(&inGeometryA->pointArray->minmax_boundingBox, &bboxA);
		M3rMinMaxBBox_To_BBox(&inGeometryB->pointArray->minmax_boundingBox, &bboxB);

		M3rIntersect_BBox(inMatrixA, &bboxA, inMatrixB, &bboxB);
	}

	return UUcTrue;
}

UUtBool M3rIntersect_BBox(
			const M3tMatrix4x3		*inMatrixA,
			const M3tBoundingBox	*inBoxA,
			const M3tMatrix4x3		*inMatrixB,
			const M3tBoundingBox	*inBoxB)
{
	M3tPoint3D bboxA[8];
	M3tPoint3D bboxB[8];
	UUtUns16   bboxEdgeList[12][2] = 
	{
		{0, 1},
		{0, 2},
		{0, 4},
		{1, 3},
		{1, 5},
		{2, 3},
		{2, 6},
		{3, 7},
		{4, 5},
		{4, 6},
		{5, 7},
		{6, 7}
	};

	UUtUns16 bboxPlanes[6][4] =
	{
		{0, 1, 2, 3},
		{0, 1, 4, 5},
		{0, 2, 4, 6},
		{1, 3, 5, 7},
		{2, 3, 6, 7},
		{4, 5, 6, 7}
	};

	UUtUns16 edgeItr, planeItr;

	MUrMatrix_MultiplyPoints(8, inMatrixA, inBoxA->localPoints, bboxA);
	MUrMatrix_MultiplyPoints(8, inMatrixB, inBoxB->localPoints, bboxB);

	// for future (better) implementation:
	// see OBBTree: A Hierarchical Structure for Rapid interference Dectection

	for(planeItr = 0; planeItr < 6; planeItr++)
	{
		// build plane from bboxA points

		for(edgeItr = 0; edgeItr < 12; edgeItr++)
		{
			// build edge from bboxB points
		}
	}

	for(planeItr = 0; planeItr < 6; planeItr++)
	{
		// build plane from bboxB points

		for(edgeItr = 0; edgeItr < 12; edgeItr++)
		{
			// build edge from bboxA points
		}
	}


	return UUcTrue;
}


	/*
	*		 2*-------------*6
	*	     /|            /|
	*	    / |           / |
	*	   /  |          /  |
	*	  /   |         /   |
	*	3*----+--------*7   |
	*	 |    '        |    |
	*	 |   0*- - - - +----*4
	*	 |   '         |   /
	*	 |  '          |  /
	*	 | '           | /
	*	 |'            |/
	*	1*-------------*5
	*	
	*	quads		plane equ
	*	=====		=======
	*	0 1 3 2		-1, 0, 0, 0.x
	*	4 6 7 5		1, 0, 0, -4.x
	*	0 4 5 1		0, -1, 0, 0.y
	*	2 3 7 6		0, 1, 0, -2.y
	*	0 2 6 4		0, 0, -1, 0.z
	*	1 5 7 3		0, 0, 1, -1.z
	*/


const UUtUns8 M3gBBox_EdgeList[12][2] =
{
	0,1,	// 0
	1,3,	// 1
	3,2,	// 2
	2,0,	// 3
	2,6,	// 4
	6,4,	// 5
	4,0,	// 6
	4,5,	// 7
	5,7,	// 8
	7,6,	// 9
	7,3,	// 10
	5,1		// 11
};

const M3tQuad M3gBBox_QuadList[6] =	// Do NOT make this UUtUns8
{		// Look-up table for which indices to use to form each box side
	{ 0,1,3,2 }, // 0
	{ 4,6,7,5 }, // 1
	{ 0,4,5,1 }, // 2	
	{ 2,3,7,6 }, // 3
	{ 0,2,6,4 }, // 4
	{ 1,5,7,3 }	 // 5
};

const M3tQuad *M3rBBox_GetSide(
		M3tBBoxSide inSide)
{
	const M3tQuad *result;

	switch(inSide)
	{
		case M3cBBoxSide_PosX:
			result = M3gBBox_QuadList + 1;
		break;

		case M3cBBoxSide_PosY:
			result = M3gBBox_QuadList + 3;
		break;

		case M3cBBoxSide_PosZ:
			result = M3gBBox_QuadList + 5;
		break;

		case M3cBBoxSide_NegX:
			result = M3gBBox_QuadList + 0;
		break;

		case M3cBBoxSide_NegY:
			result = M3gBBox_QuadList + 2;
		break;

		case M3cBBoxSide_NegZ:
			result = M3gBBox_QuadList + 4;
		break;

		default:
		case M3cBBoxSide_None:
			result = NULL;
		break;
	}

	return result;
}
	
void M3rMinMaxBBox_To_BBox(
		const M3tBoundingBox_MinMax *inBBox, 
		M3tBoundingBox *outBoundingBox)
{
	UUmAssertReadPtr(outBoundingBox, sizeof(*outBoundingBox));
	
	outBoundingBox->localPoints[0].x = inBBox->minPoint.x;
	outBoundingBox->localPoints[0].y = inBBox->minPoint.y;
	outBoundingBox->localPoints[0].z = inBBox->minPoint.z;
	
	outBoundingBox->localPoints[1].x = inBBox->minPoint.x;
	outBoundingBox->localPoints[1].y = inBBox->minPoint.y;
	outBoundingBox->localPoints[1].z = inBBox->maxPoint.z;
	
	outBoundingBox->localPoints[2].x = inBBox->minPoint.x;
	outBoundingBox->localPoints[2].y = inBBox->maxPoint.y;
	outBoundingBox->localPoints[2].z = inBBox->minPoint.z;
	
	outBoundingBox->localPoints[3].x = inBBox->minPoint.x;
	outBoundingBox->localPoints[3].y = inBBox->maxPoint.y;
	outBoundingBox->localPoints[3].z = inBBox->maxPoint.z;
	
	outBoundingBox->localPoints[4].x = inBBox->maxPoint.x;
	outBoundingBox->localPoints[4].y = inBBox->minPoint.y;
	outBoundingBox->localPoints[4].z = inBBox->minPoint.z;
	
	outBoundingBox->localPoints[5].x = inBBox->maxPoint.x;
	outBoundingBox->localPoints[5].y = inBBox->minPoint.y;
	outBoundingBox->localPoints[5].z = inBBox->maxPoint.z;
	
	outBoundingBox->localPoints[6].x = inBBox->maxPoint.x;
	outBoundingBox->localPoints[6].y = inBBox->maxPoint.y;
	outBoundingBox->localPoints[6].z = inBBox->minPoint.z;
	
	outBoundingBox->localPoints[7].x = inBBox->maxPoint.x;
	outBoundingBox->localPoints[7].y = inBBox->maxPoint.y;
	outBoundingBox->localPoints[7].z = inBBox->maxPoint.z;
}

void M3rBBox_To_MinMaxBBox(
	M3tBoundingBox *inBox,
	M3tBoundingBox_MinMax *outBox)
{
	outBox->minPoint = inBox->localPoints[0];
	outBox->maxPoint = inBox->localPoints[7];
}

void M3rBVolume_To_WorldBBox(
	M3tBoundingVolume *inVolume,
	M3tBoundingBox *outBox)
{
	UUrMemory_MoveFast(inVolume->worldPoints,outBox->localPoints,sizeof(M3tPoint3D)*M3cNumBoundingPoints);
}

void M3rBBox_To_LocalBVolume(
	M3tBoundingBox *inBox,
	M3tBoundingVolume *outVolume)
{
	UUrMemory_MoveFast(inBox->localPoints,outVolume->worldPoints,sizeof(M3tPoint3D)*M3cNumBoundingPoints);
	UUmAssert(M3cNumBoundingFaces<=6);

	UUmAssert(sizeof(M3gBBox_QuadList) == (sizeof(M3tQuad) * 6));
	UUrMemory_MoveFast(M3gBBox_QuadList, outVolume->faces, sizeof(M3gBBox_QuadList));

	return;
}

void M3rBVolume_To_BSphere(
	M3tBoundingVolume *inVolume,
	M3tBoundingSphere *outSphere)
{
	// step 1, compute te

	MUmVector_Add(outSphere->center, inVolume->worldPoints[0], inVolume->worldPoints[7]);
	MUmVector_Scale(outSphere->center, 0.5f);

	outSphere->radius = MUmVector_GetDistance(outSphere->center, inVolume->worldPoints[7]);

	return;
}

void M3rBBox_GrowFromPlane(
	M3tBoundingBox *ioBox,
	M3tPlaneEquation *inPlane,
	float inGrowAmount)
{
	// Grows the bounding box in the plane given
	M3tVector3D onewayV,other;
	UUtUns32 i;
	M3tPoint3D *p;

	inGrowAmount/=2.0f;
	AKmPlaneEqu_GetNormal(*inPlane,onewayV);
	other = onewayV;
	MUmVector_Scale(onewayV,inGrowAmount);
	MUmVector_Scale(other,-inGrowAmount);

	for (i=0; i<8; i++)
	{
		p = ioBox->localPoints + i;
		if (MUrPoint_PointBehindPlane(p,inPlane))
		{
			MUmVector_Increment(*p,other);
		}
		else
		{
			MUmVector_Increment(*p,onewayV);
		}
	}
}

void M3rBBox_To_EdgeBBox(
		M3tBoundingBox *inBoundingBox,
		M3tBoundingBox_Edge *outBoundingBox)
{
	UUtUns16 edgeIndex;
	
	for (edgeIndex=0; edgeIndex<12; edgeIndex++)
	{
		outBoundingBox->edges[edgeIndex].a = inBoundingBox->localPoints[M3gBBox_EdgeList[edgeIndex][0]];
		outBoundingBox->edges[edgeIndex].b = inBoundingBox->localPoints[M3gBBox_EdgeList[edgeIndex][1]];
	}
}

void M3rEdgeBBox_Offset(
	M3tBoundingBox_Edge *ioBoundingBox,
	M3tPoint3D *inOffset)
{
	UUtUns16 e;
	
	for (e=0; e<12; e++)
	{
		MUmVector_Increment(ioBoundingBox->edges[e].a,*inOffset);
		MUmVector_Increment(ioBoundingBox->edges[e].b,*inOffset);
	}
}

void M3rMinMaxBBox_Draw_Line(
		M3tBoundingBox_MinMax*	inMinMaxBBox,
		UUtUns32				inShade)
{
	M3tPoint3D magicPoints[2];
	UUtUns8 itr;
	M3tBoundingBox bbox;

	M3rMinMaxBBox_To_BBox(inMinMaxBBox, &bbox);

	for(itr = 0; itr < 12; itr++) 
	{
		magicPoints[0] = bbox.localPoints[M3gBBox_EdgeList[itr][0]];
		magicPoints[1] = bbox.localPoints[M3gBBox_EdgeList[itr][1]];

		M3rGeom_Line_Light(magicPoints+0, magicPoints+1, inShade);
	}

	return;
}

void M3rBBox_Draw_Line(
	M3tBoundingBox *inBBox,
	UUtUns32 inShade)
{
	UUtUns8 itr;
	
	for(itr = 0; itr < 12; itr++) 
	{
		M3tPoint3D *from, *to;

		from = &(inBBox->localPoints[M3gBBox_EdgeList[itr][0]]);
		to = &(inBBox->localPoints[M3gBBox_EdgeList[itr][1]]);

		M3rGeom_Line_Light(from, to, inShade);
	}

	return;
}

void M3rBVolume_Draw_Line(
	M3tBoundingVolume *inBVolume,
	UUtUns32 inShade)
{
	UUtUns8 itr;
	M3tPoint3D normal1,normal2;

	// Only works if M3cNumBoundingFaces==6 && M3cNumBoundingPoints==8

	// Edges
	for(itr = 0; itr < 12; itr++)
	{
		M3tPoint3D *from, *to;

		from = &(inBVolume->worldPoints[M3gBBox_EdgeList[itr][0]]);
		to = &(inBVolume->worldPoints[M3gBBox_EdgeList[itr][1]]);

		M3rGeom_Line_Light(from, to, inShade);
	}

	// Normals
	for (itr=0; itr<M3cNumBoundingFaces; itr++)
	{
		const M3tQuad *quad = M3gBBox_QuadList + itr;

		normal1 = inBVolume->worldPoints[quad->indices[0]];
		MUmVector_Increment(normal1,inBVolume->worldPoints[quad->indices[1]]);
		MUmVector_Increment(normal1,inBVolume->worldPoints[quad->indices[2]]);
		MUmVector_Increment(normal1,inBVolume->worldPoints[quad->indices[3]]);
		MUmVector_Scale(normal1,0.25f);
		AKmPlaneEqu_GetNormal(inBVolume->curPlanes[itr],normal2);
		MUmVector_Scale(normal2,20.0f);
		MUmVector_Increment(normal2,normal1);
		M3rGeom_Line_Light(&normal1, &normal2, IMcShade_Red);
	}

	return;
}


void M3rBuildCircle(UUtUns32 inNumPoints, float inHeight, const M3tBoundingCircle *circle, M3tPoint3D *outPoints)
{
	UUtUns32 itr;

	UUmAssertWritePtr(outPoints, sizeof(M3tPoint3D) * inNumPoints);

	for(itr = 0; itr < inNumPoints; itr++)
	{
		float theta = M3c2Pi * (((float) itr) / inNumPoints);
		outPoints[itr].x = MUrCos(theta) * circle->radius + circle->center.x;
		outPoints[itr].y = inHeight;
		outPoints[itr].z = MUrSin(theta) * circle->radius + circle->center.z;
	}

	return;
}

void M3rBoundingCylinder_Draw_Line(
		const M3tBoundingCylinder *inBoundingCylinder,
		UUtUns32 inShade)
{
	M3tPoint3D magicPoints[2];
	M3tPoint3D points[16];
	UUtUns16 itr;

	M3rBuildCircle(8, inBoundingCylinder->top, &inBoundingCylinder->circle, points + 0);
	M3rBuildCircle(8, inBoundingCylinder->bottom, &inBoundingCylinder->circle, points + 8);

	for(itr = 0; itr < 8; itr++)
	{
		magicPoints[0] = points[itr];
		magicPoints[1] = points[itr + 8];

		M3rGeom_Line_Light(magicPoints+0, magicPoints+1, inShade);

		magicPoints[0] = points[itr];
		magicPoints[1] = points[((itr + 1) % 8)];

		M3rGeom_Line_Light(magicPoints+0, magicPoints+1, inShade);

		magicPoints[0] = points[8 + itr];
		magicPoints[1] = points[8 + ((itr + 1) % 8)];

		M3rGeom_Line_Light(magicPoints+0, magicPoints+1, inShade);
	}
}

void M3rGeom_Line_Light(const M3tPoint3D *point1, const M3tPoint3D *point2, UUtUns32 shade)
{
	// one times cache line size should be plenty but why risk it
	UUtUns8 block[sizeof(M3tPoint3D) * 2 + 2 * UUcProcessor_CacheLineSize];
	M3tPoint3D *aligned = UUrAlignMemory(block);

	aligned[0] = *point1;
	aligned[1] = *point2;

	M3rGeometry_LineDraw(2, aligned, shade);

	return;
}


void 
M3rDraw_Bitmap(
	M3tTextureMap			*inBitmap,
	const M3tPointScreen	*inDestPoint,
	UUtUns16				inWidth,
	UUtUns16				inHeight,
	UUtUns32				inShade,
	UUtUns16				inAlpha)			/* 0 - M3cMaxAlpha */
{
	M3tPointScreen	screenPoints[2];
	M3tTextureCoord	uv[4];

	UUmAssert(inWidth <= inBitmap->width);
	UUmAssert(inHeight <= inBitmap->height);

	screenPoints[0] = *inDestPoint;
	screenPoints[1] = *inDestPoint;
	screenPoints[1].x = screenPoints[0].x + inWidth;
	screenPoints[1].y = screenPoints[0].y + inHeight;

	// br
	uv[3].u = (float)inWidth / (float)inBitmap->width;
	uv[3].v = (float)inHeight / (float)inBitmap->height;

	// tl
	uv[0].u = 0.f;
	uv[0].v = 0.f;

	// tr
	uv[1].u = uv[3].u;
	uv[1].v = 0.f;

	// bl
	uv[2].u = 0.f;
	uv[2].v = uv[3].v;
	
	M3rDraw_State_Push();

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_BaseTextureMap,
		inBitmap);
	
	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Appearence,
		M3cDrawState_Appearence_Texture_Unlit);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Interpolation,
		M3cDrawState_Interpolation_None);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_ConstantColor,
		inShade);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Alpha,
		inAlpha);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Fill,
		M3cDrawState_Fill_Solid);

	M3rDraw_State_Commit();
	
	M3rDraw_Sprite(
		screenPoints,
		uv);

	M3rDraw_State_Pop();

	return;
}

// ----------------------------------------------------------------------
void 
M3rDraw_BitmapUV(
	M3tTextureMap			*inBitmap,
	const M3tTextureCoord	*inUV,
	const M3tPointScreen	*inDestPoint,
	UUtUns16				inWidth,
	UUtUns16				inHeight,
	UUtUns32				inShade,
	UUtUns16				inAlpha)			/* 0 - M3cMaxAlpha */
{
	M3tPointScreen	screenPoints[2];

	screenPoints[0] = *inDestPoint;
	screenPoints[1] = *inDestPoint;
	screenPoints[1].x = screenPoints[0].x + inWidth;
	screenPoints[1].y = screenPoints[0].y + inHeight;
	
	M3rDraw_State_Push();

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_BaseTextureMap,
		inBitmap);
	
	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Appearence,
		M3cDrawState_Appearence_Texture_Unlit);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Interpolation,
		M3cDrawState_Interpolation_None);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Fill,
		M3cDrawState_Fill_Solid);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_ConstantColor,
		inShade);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Alpha,
		inAlpha);

	M3rDraw_State_Commit();
	
	M3rDraw_Sprite(
		screenPoints,
		inUV);

	M3rDraw_State_Pop();

	return;
}

// ----------------------------------------------------------------------
void 
M3rDraw_BigBitmap(
	M3tTextureMap_Big		*inBigBitmap,
	const M3tPointScreen	*inDestPoint,
	UUtUns16				inWidth,
	UUtUns16				inHeight,
	UUtUns32				inShade,
	UUtUns16				inAlpha)			/* 0 - M3cMaxAlpha */
{
	UUtUns16				x;
	UUtUns16				y;
	
	// draw the subtextures that are visible
	for (y = 0; y < inBigBitmap->num_y; y++)
	{
		for (x = 0; x < inBigBitmap->num_x; x++)
		{
			UUtUns16			index;
			M3tTextureMap		*texture;
			M3tPointScreen		dest_point;
			UUtUns16			width;
			UUtUns16			height;
			
			UUtInt16			temp;			
			UUtInt32			x_times_maxwidth;
			UUtInt32			y_times_maxheight;
			
			// get a pointer to the texture to be drawn
			index = x + (y * inBigBitmap->num_x);
			texture = inBigBitmap->textures[index];
			
			x_times_maxwidth = x * M3cTextureMap_MaxWidth;
			y_times_maxheight = y * M3cTextureMap_MaxHeight;
			
			// calculate the dest_point of the texture
			dest_point.x = inDestPoint->x + (float)x_times_maxwidth;
			dest_point.y = inDestPoint->y + (float)y_times_maxheight;
			dest_point.z = inDestPoint->z;
			dest_point.invW = inDestPoint->invW;
			
			// calculate the width and height
			temp = (UUtInt16)UUmMax(0, (UUtInt32)inWidth - x_times_maxwidth);
			width = UUmMin(M3cTextureMap_MaxWidth, (UUtUns16)temp);
			temp = (UUtInt16)UUmMax(0, (UUtInt32)inHeight - y_times_maxheight);
			height = UUmMin(M3cTextureMap_MaxHeight, (UUtUns16)temp);
			
			// draw the sub_texture
			M3rDraw_Bitmap(
				texture,
				&dest_point,
				width,
				height,
				inShade,
				inAlpha);
		}
	}
}

UUtError M3rDrawEngine_FindGrayscalePixelType(
	IMtPixelType		*outTextureFormat)
{
#define numGrayscaleTextureFormats 6
	UUtUns16 itr;
	
	// array is in order of preference
	IMtPixelType table[numGrayscaleTextureFormats] = 
		{
			IMcPixelType_I8,
			IMcPixelType_A4I4,
			IMcPixelType_RGB555,
			IMcPixelType_ARGB1555,
			IMcPixelType_ARGB4444,
			IMcPixelType_ARGB8888
		};

	UUmAssertWritePtr(outTextureFormat, sizeof(*outTextureFormat));

	for(itr = 0; itr < numGrayscaleTextureFormats; itr++)
	{
		IMtPixelType texel_type = table[itr];

		if (M3rDraw_TextureFormatAvailable(texel_type))
		{
			*outTextureFormat = texel_type;
			return UUcError_None;
		}
	}

	UUmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to find a texture format.");
}

UUtError
M3rGroup_GetColor(
	GRtGroup*		inGroup,
	const char*		inVarName,
	M3tColorRGB		*outColor)
{
	UUtError			error;
	GRtElementType		elementType;
	GRtElementArray*	array;
	char*				string;
	
	error = GRrGroup_GetElement(inGroup, inVarName, &elementType, &array);
	if(error != UUcError_None) return error;
	
	if(elementType != GRcElementType_Array) return UUcError_Generic;
	
	error = GRrGroup_Array_GetElement(array, 0, &elementType, &string);
	if(error != UUcError_None) return error;
	if(elementType != GRcElementType_String) return UUcError_Generic;
	sscanf(string, "%f", &outColor->r);
	
	error = GRrGroup_Array_GetElement(array, 1, &elementType, &string);
	if(error != UUcError_None) return error;
	if(elementType != GRcElementType_String) return UUcError_Generic;
	sscanf(string, "%f", &outColor->g);
	
	error = GRrGroup_Array_GetElement(array, 2, &elementType, &string);
	if(error != UUcError_None) return error;
	if(elementType != GRcElementType_String) return UUcError_Generic;
	sscanf(string, "%f", &outColor->b);
	
	return UUcError_None;
}

void M3rGeom_FaceNormal(
		const		M3tPoint3D *inPoints,
		const		M3tQuad *inQuad,
		const		M3tVector3D *inFaceNormal,
		UUtUns32	inShade)
{
	M3tPoint3D average = { 0, 0, 0 };
	M3tPoint3D to;

	MUmVector_Add(average, average, inPoints[inQuad->indices[0]]);
	MUmVector_Add(average, average, inPoints[inQuad->indices[1]]);
	MUmVector_Add(average, average, inPoints[inQuad->indices[2]]);
	MUmVector_Add(average, average, inPoints[inQuad->indices[3]]);
	MUmVector_Scale(average, 0.25f);

	to = *inFaceNormal;
	MUmVector_Scale(to, 4.f);
	MUmVector_Add(to, to, average);

	M3rGeom_Line_Light(&average, &to, inShade);

	return;
}

void M3rDisplay_Circle(M3tPoint3D *inCenter, float inRadius, UUtUns32 inShade)
{
	/**************
	* Draws a wireframe circle in the XZ plane
	*/
	
	#define cNumPoints 16

	M3tBoundingCircle circle;
	// one times cache line size should be plenty but why risk it
	UUtUns8 block[(sizeof(M3tPoint3D) * (cNumPoints + 1)) + (2 * UUcProcessor_CacheLineSize)];
	M3tPoint3D *points = UUrAlignMemory(block);

	circle.center.x = inCenter->x;
	circle.center.z = inCenter->z;
	circle.radius = inRadius;

	M3rBuildCircle(cNumPoints, inCenter->y, &circle, points);
	points[cNumPoints] = points[0];

	M3rGeometry_LineDraw(cNumPoints + 1, points, inShade);

	return;
}

void M3rGeometry_GetBBox(
		const M3tGeometry *inGeometry,
		const M3tMatrix4x3 *inMatrix,
		M3tBoundingBox_MinMax *outBBox)
{
	M3tPoint3D center;
	float radius;

	UUmAssertReadPtr(inGeometry, sizeof(*inGeometry));
	UUmAssertReadPtr(inMatrix, sizeof(*inMatrix));
	UUmAssertWritePtr(outBBox, sizeof(*outBBox));

	radius = inGeometry->pointArray->boundingSphere.radius;

	MUrMatrix_MultiplyPoint(&inGeometry->pointArray->boundingSphere.center, inMatrix, &center);

	outBBox->minPoint.x = center.x - radius;
	outBBox->minPoint.y = center.y - radius;
	outBBox->minPoint.z = center.z - radius;

	outBBox->maxPoint.x = center.x + radius;
	outBBox->maxPoint.y = center.y + radius;
	outBBox->maxPoint.z = center.z + radius;

	return;
}

M3tTextureMap *M3rTextureMap_GetFromName(const char *inTemplateName)
{
	UUtError error;
	void *dataPtr;
	M3tTextureMap *texture = NULL;

	error = TMrInstance_GetDataPtr(M3cTemplate_TextureMap, inTemplateName, &dataPtr);

	if (UUcError_None == error) {
		texture = dataPtr;
	}	

	return texture;
}

M3tTextureMap *M3rTextureMap_GetFromName_UpperCase(const char *inTemplateName)
{
	M3tTextureMap *texture = NULL;

	if (NULL != inTemplateName) {
		char buffer[512];

		strcpy(buffer, inTemplateName);
		UUrString_Capitalize(buffer, sizeof(buffer));

		texture = M3rTextureMap_GetFromName(buffer);
	}

	return texture;
}

M3tTextureMap *M3rGroup_GetTextureMap(GRtGroup *inGroup, const char *inVarName)
{
	M3tTextureMap *result = NULL;
	char *texture_name;
	UUtError error;

	error = GRrGroup_GetString(inGroup, inVarName, &texture_name);
	if (UUcError_None == error) {
		error = TMrConstruction_Instance_GetPlaceHolder(M3cTemplate_TextureMap, texture_name, (TMtPlaceHolder*) &result);

		if (UUcError_None != error) {
			result = NULL;
		}
	}

	return result;
}

/* old MXtNode optimization

typedef struct OptimizeTriangle
{
	MXtFace		face;
	M3tVector3D normal;
	UUtUns32	edge_sort[3];
	UUtUns32	adjacencies[3];
	UUtUns32	used;
} OptimizeTriangle;

OptimizeTriangle *M3gOptimizeTriangles = NULL;
MXtFace *M3gOptimizeQuads = NULL;
UUtUns32 M3gOptimizeQuadCount;
UUtUns32 M3gOptimizeTriangleCount;

static void M3rOptimizeBuildAdjacency(MXtNode *inNode)
{
	UUtUns32 triangle_a_itr;
	UUtUns32 triangle_b_itr;
	UUtUns32 edge_a;
	UUtUns32 edge_b;

	for(triangle_a_itr = 0; triangle_a_itr < M3gOptimizeTriangleCount; triangle_a_itr++)
	{
		for(triangle_b_itr = triangle_a_itr + 1; triangle_b_itr < M3gOptimizeTriangleCount; triangle_b_itr++)
		{
			OptimizeTriangle *triangle_a = M3gOptimizeTriangles + triangle_a_itr;
			OptimizeTriangle *triangle_b = M3gOptimizeTriangles + triangle_b_itr;

			if (triangle_a->face.material != triangle_b->face.material) {
				continue;
			}

			if (!MUrPoint_IsEqual(&triangle_a->normal, &triangle_b->normal)) {
				continue;
			}

			for(edge_a = 0; edge_a < 3; edge_a++)
			{
				for(edge_b = 0; edge_b < 3; edge_b++)
				{
					if (triangle_a->edge_sort[edge_a] == triangle_b->edge_sort[edge_b]) {
						triangle_a->adjacencies[edge_a] = triangle_b_itr;
						triangle_b->adjacencies[edge_b] = triangle_a_itr;
					}
				}
			}
		}
	}

	return;
}

static void M3rOptimize_Setup(MXtNode *inNode)
{
	UUtUns32 itr;

	M3gOptimizeTriangles = UUrMemory_Block_New(sizeof(OptimizeTriangle) * inNode->numTriangles);

	// pass 1: setup the list 
	for(itr = 0; itr < inNode->numTriangles; itr++)
	{
		UUtUns32 edge_itr;
		MXtFace *src_face = inNode->triangles + itr;
		OptimizeTriangle *dst_face = M3gOptimizeTriangles + itr;

		dst_face->face = *src_face;
		dst_face->adjacencies[0] = -1;
		dst_face->adjacencies[1] = -1;
		dst_face->adjacencies[2] = -1;
		dst_face->adjacencies[3] = -1;
		dst_face->used = 0;

		MUrVector_NormalFromPoints(
			&inNode->points[src_face->indices[0]].point, 
			&inNode->points[src_face->indices[1]].point, 
			&inNode->points[src_face->indices[2]].point, 
			&dst_face->normal);

		if (MUrVector_DotProduct(&src_face->dont_use_this_normal, &dst_face->normal) < 0.f) {
			MUmVector_Negate(dst_face->normal);
		}

		for(edge_itr = 0; edge_itr < 3; edge_itr++)
		{
			UUtUns32 point0 = src_face->indices[edge_itr];
			UUtUns32 point1 = src_face->indices[(edge_itr + 1) % 3];
			UUtUns32 point_high = UUmMax(point0, point1);
			UUtUns32 point_low = UUmMin(point0, point1);

			dst_face->edge_sort[edge_itr] = (point_high << 16) | (point_low << 0);
		}
	}

	M3gOptimizeTriangleCount = inNode->numTriangles;

	return;
}

void M3rOptimize_BuildQuads_SinglePass(MXtNode *inNode, UUtBool inLeafOnly)
{
	UUtUns32 itr;

	// ok scan through the list using a simple greedy algorithm to turn quads into triangles
	for(itr = 0; itr < M3gOptimizeTriangleCount; itr++)
	{
		UUtUns32 adjacency;
		UUtUns32 adjacency_count = 0;
		OptimizeTriangle *adjacent_triangle;
		OptimizeTriangle *triangle = M3gOptimizeTriangles + itr;
		UUtUns32 adj_itr;

		if (triangle->used) {
			continue;
		}

		for(adj_itr = 0; adj_itr < 3; adj_itr++)
		{
			adjacency = triangle->adjacencies[adj_itr];
			adjacent_triangle = M3gOptimizeTriangles + adjacency;

			if (-1 == adjacency) {
				continue;
			}

			if (adjacent_triangle->used) {
				triangle->adjacencies[adj_itr] = -1;
				continue;
			}

			adjacency_count++;
		}

		if (0 == adjacency_count) {
			continue;
		}

		if ((1 != adjacency_count) && inLeafOnly) {
			continue;
		}

		for(adj_itr = 0; adj_itr < 3; adj_itr++)
		{
			adjacency = triangle->adjacencies[adj_itr];
			adjacent_triangle = M3gOptimizeTriangles + adjacency;

			// if any of these adjacencies are valid and the triangle had not been used, then
			// use it and add it to the quad list

			if (adjacency == -1) {
				continue;
			}

			if (adjacent_triangle->used) {
				continue;
			}

			// ok this triangle has not been used, merge the two
			// start w/ the 2nd vertex of the edge, add all through of triangle
			// a, then add the non-edge vertex of triangle b

			triangle->used = UUcTrue;
			adjacent_triangle->used = UUcTrue;

			{
				MXtFace new_face = triangle->face;
				UUtUns32 new_point_itr;

				new_face.indices[0] = triangle->face.indices[(adj_itr + 1) % 3];
				new_face.indices[1] = triangle->face.indices[(adj_itr + 2) % 3];
				new_face.indices[2] = triangle->face.indices[(adj_itr + 3) % 3];
				new_face.indices[3] = new_face.indices[0];

				for(new_point_itr = 0; new_point_itr < 3; new_point_itr++)
				{
					UUtUns32 new_point_index = adjacent_triangle->face.indices[new_point_itr];

					if (new_point_index == new_face.indices[0]) {
						continue;
					}

					if (new_point_index == new_face.indices[1]) {
						continue;
					}

					if (new_point_index == new_face.indices[2]) {
						continue;
					}

					new_face.indices[3] = (UUtUns16) new_point_index;
				}

				M3gOptimizeQuads[M3gOptimizeQuadCount++] = new_face;
				M3gOptimizeTriangleCount -= 2;
			}

			break;
		}
	}


	return;
}

void M3rOptimize_BuildQuads(MXtNode *inNode)
{

	M3gOptimizeQuads = UUrMemory_Block_New(sizeof(MXtFace) * (inNode->numQuads + inNode->numTriangles));
	UUrMemory_MoveFast(inNode->quads, M3gOptimizeQuads, sizeof(MXtFace) * inNode->numQuads);
	M3gOptimizeQuadCount = inNode->numQuads;

	M3rOptimize_BuildQuads_SinglePass(inNode, UUcTrue);
	M3rOptimize_BuildQuads_SinglePass(inNode, UUcTrue);
	M3rOptimize_BuildQuads_SinglePass(inNode, UUcFalse);

	return;
}

MXtNode *M3rOptimize_BuildNode(MXtNode *inNode)
{
	UUtUns32 sizeof_header = sizeof(MXtNode);
	UUtUns32 sizeof_points = inNode->numPoints * sizeof(MXtPoint);
	UUtUns32 sizeof_triangles = M3gOptimizeTriangleCount * sizeof(MXtFace);
	UUtUns32 sizeof_quads = M3gOptimizeQuadCount * sizeof(MXtFace);

	char *buffer = UUrMemory_Block_New(sizeof_header + sizeof_points + sizeof_triangles + sizeof_quads);
	MXtNode *node = (MXtNode *) buffer;

	if (NULL == buffer) {
		goto exit;
	}

	UUrMemory_Clear(node, sizeof(MXtNode));
	buffer += sizeof_header;

	node->points = (MXtPoint *) buffer;
	buffer += sizeof_points;

	node->triangles = (MXtFace *) buffer;
	buffer += sizeof_triangles;

	node->quads = (MXtFace *) buffer;
	buffer += sizeof_quads;

	node->markers = NULL;
	node->materials = NULL;
	node->userData = NULL;

	node->numPoints = inNode->numPoints;
	UUrMemory_MoveFast(inNode->points, node->points, sizeof_points);

	{
		UUtUns32 itr;
		UUtUns32 triangle_count = 0;

		node->numTriangles = (UUtUns16) M3gOptimizeTriangleCount;

		for(itr = 0; itr < inNode->numTriangles; itr++)
		{
			if (M3gOptimizeTriangles[itr].used) {
				continue;
			}

			node->triangles[triangle_count++] = M3gOptimizeTriangles[itr].face;
		}
	}

	node->numQuads = (UUtUns16) M3gOptimizeQuadCount;
	UUrMemory_MoveFast(M3gOptimizeQuads, node->quads, sizeof_quads);

exit:
	return node;
}

void M3rOptimize_Cleanup(MXtNode *inNode)
{
	UUrMemory_Block_Delete(M3gOptimizeTriangles);
	UUrMemory_Block_Delete(M3gOptimizeQuads);

	M3gOptimizeTriangles = NULL;
	M3gOptimizeQuads = NULL;
	M3gOptimizeQuadCount = 0;
	M3gOptimizeTriangleCount = 0;

	return;
}

void *M3rOptimizeNode(void *inNode)
{
	MXtNode *old_node = (MXtNode *) inNode;
	MXtNode *new_node;

	M3rOptimize_Setup(old_node);
	M3rOptimizeBuildAdjacency(old_node);
	M3rOptimize_BuildQuads(old_node);
	new_node = M3rOptimize_BuildNode(old_node);
	M3rOptimize_Cleanup(old_node);

	return new_node;
}

*/

// new MXtNode optimization code - S.S.

typedef struct triangle_grouping_data {
	int tri_index; // index into node->triangles array
	int quad_index; // index of the quad this triangle has been used in, or NONE
	int edge_shared_index[3]; // indices of tris which share each of the 3 edges, or NONE
	M3tVector3D normal; // used to check for parallel planes
} triangle_grouping_data;

enum {
	AB= 0,
	BC= 1,
	CA= 2
};

enum {
	A= 0,
	B= 1,
	C= 2,
	D= 3,

	NONE= -1L
};

static UUtBool edge_shared(
	int e0[2],
	int e1[2])
{
	return (((e0[0] == e1[0]) && (e0[1] == e1[1])) || ((e0[0] == e1[1]) && (e0[1] == e1[0])));
}

static UUtBool triangles_parallel(
	triangle_grouping_data *t0,
	triangle_grouping_data *t1,
	MXtNode *node)
{
	M3tVector3D diff, *n0, *n1;
	UUtBool parallel;
	const float tolerance= 0.01f; // a percentage
	const float tiny= 0.001f; // values < tiny == 0

	n0= &t0->normal;
	n1= &t1->normal;
	diff.x= (float)fabs(((fabs(n1->x) - fabs(n0->x))/UUmMax(tiny, fabs(n0->x))));
	diff.y= (float)fabs(((fabs(n1->y) - fabs(n0->y))/UUmMax(tiny, fabs(n0->y))));
	diff.z= (float)fabs(((fabs(n1->z) - fabs(n1->z))/UUmMax(tiny, fabs(n0->z))));

	if ((diff.x < tolerance) && (diff.y < tolerance) && (diff.z < tolerance))
	{
		parallel= UUcTrue;
	}
	else
	{
		parallel= UUcFalse;
	}

	return parallel;
}

static UUtBool build_quad(
	MXtNode *node,
	triangle_grouping_data *triangle_datum0, // abc
	triangle_grouping_data *triangle_datum1, // a'b'c'
	int edge_index0,
	int edge_index1,
	MXtFace *quad,
	BFtFile *debug_file)
{
	UUtBool success= UUcTrue;

	UUmAssert(node->triangles[triangle_datum0->tri_index].material ==
		node->triangles[triangle_datum1->tri_index].material);

	UUrMemory_Clear(quad, sizeof(MXtFace));
	*quad= node->triangles[triangle_datum0->tri_index];

	// oops... have to reverse the winding
	// ab / a'b'
	if ((edge_index0 == AB) && (edge_index1 == AB))
	{
		quad->indices[D]= node->triangles[triangle_datum0->tri_index].indices[A]; // a
		quad->indices[C]= node->triangles[triangle_datum1->tri_index].indices[C]; // c'
		quad->indices[B]= node->triangles[triangle_datum0->tri_index].indices[B]; // b
		quad->indices[A]= node->triangles[triangle_datum0->tri_index].indices[C]; // c
	}
	// ab / b'c'
	else if ((edge_index0 == AB) && (edge_index1 == BC))
	{
		quad->indices[D]= node->triangles[triangle_datum0->tri_index].indices[A]; // a
		quad->indices[C]= node->triangles[triangle_datum1->tri_index].indices[A]; // a'
		quad->indices[B]= node->triangles[triangle_datum0->tri_index].indices[B]; // b
		quad->indices[A]= node->triangles[triangle_datum0->tri_index].indices[C]; // c
	}
	// ab / c'a'
	else if ((edge_index0 == AB) && (edge_index1 == CA))
	{
		quad->indices[D]= node->triangles[triangle_datum0->tri_index].indices[A]; // a
		quad->indices[C]= node->triangles[triangle_datum1->tri_index].indices[B]; // b'
		quad->indices[B]= node->triangles[triangle_datum0->tri_index].indices[B]; // b
		quad->indices[A]= node->triangles[triangle_datum0->tri_index].indices[C]; // c
	}
	// bc / a'b'
	else if ((edge_index0 == BC) && (edge_index1 == AB))
	{
		quad->indices[D]= node->triangles[triangle_datum0->tri_index].indices[A]; // a
		quad->indices[C]= node->triangles[triangle_datum0->tri_index].indices[B]; // b
		quad->indices[B]= node->triangles[triangle_datum1->tri_index].indices[C]; // c'
		quad->indices[A]= node->triangles[triangle_datum0->tri_index].indices[C]; // c
	}
	// bc / b'c'
	else if ((edge_index0 == BC) && (edge_index1 == BC))
	{
		quad->indices[D]= node->triangles[triangle_datum0->tri_index].indices[A]; // a
		quad->indices[C]= node->triangles[triangle_datum0->tri_index].indices[B]; // b
		quad->indices[B]= node->triangles[triangle_datum1->tri_index].indices[A]; // a'
		quad->indices[A]= node->triangles[triangle_datum0->tri_index].indices[C]; // c
	}
	// bc / c'a'
	else if ((edge_index0 == BC) && (edge_index1 == CA))
	{
		quad->indices[D]= node->triangles[triangle_datum0->tri_index].indices[A]; // a
		quad->indices[C]= node->triangles[triangle_datum0->tri_index].indices[B]; // b
		quad->indices[B]= node->triangles[triangle_datum1->tri_index].indices[B]; // b'
		quad->indices[A]= node->triangles[triangle_datum0->tri_index].indices[C]; // c
	}
	// ca / a'b'
	else if ((edge_index0 == CA) && (edge_index1 == AB))
	{
		quad->indices[D]= node->triangles[triangle_datum0->tri_index].indices[A]; // a
		quad->indices[C]= node->triangles[triangle_datum0->tri_index].indices[B]; // b
		quad->indices[B]= node->triangles[triangle_datum0->tri_index].indices[C]; // c
		quad->indices[A]= node->triangles[triangle_datum1->tri_index].indices[C]; // c'
	}
	// ca / b'c'
	else if ((edge_index0 == CA) && (edge_index1 == BC))
	{
		quad->indices[D]= node->triangles[triangle_datum0->tri_index].indices[A]; // a
		quad->indices[C]= node->triangles[triangle_datum0->tri_index].indices[B]; // b
		quad->indices[B]= node->triangles[triangle_datum0->tri_index].indices[C]; // c
		quad->indices[A]= node->triangles[triangle_datum1->tri_index].indices[A]; // a'
	}
	// ca / c'a'
	else if ((edge_index0 == CA) && (edge_index1 == CA))
	{
		quad->indices[D]= node->triangles[triangle_datum0->tri_index].indices[A]; // a
		quad->indices[C]= node->triangles[triangle_datum0->tri_index].indices[B]; // b
		quad->indices[B]= node->triangles[triangle_datum0->tri_index].indices[C]; // c
		quad->indices[A]= node->triangles[triangle_datum1->tri_index].indices[B]; // b'
	}
	else
	{
		UUmAssert(0);
	}

	// discard bad quads
	{
		M3tVector3D normal0, normal1, delta;
		M3tVector3D edge0, edge1;
		UUtUns32 i;
		float dot;

		// CB: this pretzel detection code lifted from Imp_Env2_Parse.c
		MUrVector_NormalFromPoints(
			&node->points[quad->indices[A]].point,
			&node->points[quad->indices[B]].point,
			&node->points[quad->indices[C]].point,
			&normal0);
		MUrVector_NormalFromPoints(
			&node->points[quad->indices[A]].point,
			&node->points[quad->indices[C]].point,
			&node->points[quad->indices[D]].point,
			&normal1);
		
		if (success) {
			dot = normal0.x * normal1.x + normal0.y * normal1.y + normal0.z * normal1.z;
			if (dot < 0.8f) {
				success = UUcFalse;
				if (debug_file) {
					BFrFile_Printf(debug_file, "    %d/%d normal (%f %f %f) / (%f %f %f) dot %f, reject"UUmNL,
									triangle_datum0->tri_index, triangle_datum1->tri_index, normal0.x, normal0.y, normal0.z, normal1.x, normal1.y, normal1.z);
				}
			}
		}

		if (success) {
			// check that the quad is actually planar... note that 0.01f is the tolerance value
			// used when importing also in IMPiEnv_Parse_GQ_AddQuadProperSize
			MUmVector_Subtract(delta, node->points[quad->indices[D]].point, node->points[quad->indices[A]].point);
			dot = MUrVector_DotProduct(&delta, &normal0);
			if (fabs(dot) > 0.01f) {
				success = UUcFalse;
				if (debug_file) {
					BFrFile_Printf(debug_file, "    %d/%d nonplanar pt 0 [%f %f %f] pt3 [%f %f %f] delta (%f %f %f) normal (%f %f %f) dot %f, reject"UUmNL,
									triangle_datum0->tri_index, triangle_datum1->tri_index,
									node->points[quad->indices[A]].point.x, node->points[quad->indices[A]].point.y, node->points[quad->indices[A]].point.z,
									node->points[quad->indices[D]].point.x, node->points[quad->indices[D]].point.y, node->points[quad->indices[D]].point.z,
									delta.x, delta.y, delta.z, normal0.x, normal0.y, normal0.z, dot);
				}
			}
			success = success && (fabs(dot) < 0.01f);
		}

		if (success) {
			// check that the quad is convex
			MUmVector_Subtract(edge0, node->points[quad->indices[0]].point, node->points[quad->indices[3]].point);
			for (i = 0; i < 4; i++) {
				// calculate the next edge around the quad
				MUmVector_Subtract(edge1, node->points[quad->indices[(i + 1) % 4]].point, node->points[quad->indices[i]].point);

				// test for convex condition
				MUrVector_CrossProduct(&edge0, &edge1, &normal1);
				dot = MUrVector_DotProduct(&normal0, &normal1);
				if (dot < -0.1f) {
					success = UUcFalse;
					if (debug_file) {
						BFrFile_Printf(debug_file, "    %d/%d concave edge %d (%f %f %f) %d (%f %f %f) cross [%f %f %f] norm [%f %f %f] dot %f, reject"UUmNL,
										triangle_datum0->tri_index, triangle_datum1->tri_index,
										(i + 3) % 4, edge0.x, edge0.y, edge0.z, i, edge1.x, edge1.y, edge1.z, normal1.x, normal1.y, normal1.z,
										normal0.x, normal0.y, normal0.z, dot);
					}
					break;
				}

				edge0 = edge1;
			}
		}
	}

	return success;
}

void *M3rOptimizeNode(
	void *inNode,
	BFtFile *debug_file)
{
	MXtNode *node= (MXtNode *)inNode;
	MXtNode *new_node;
	MXtFace *quads;
	triangle_grouping_data *triangle_datum;
	int i, num_quads, num_tris;

	num_tris= node->numTriangles;
	num_quads= 0;

	UUmAssert(num_tris>0);
	triangle_datum= (triangle_grouping_data *)UUrMemory_Block_New(num_tris * sizeof(triangle_grouping_data));
	UUmAssert(triangle_datum);

	quads= (MXtFace *)UUrMemory_Block_NewClear((1 + num_tris/2) * sizeof(MXtFace));
	UUmAssert(quads);

	if (debug_file) {
		BFrFile_Printf(debug_file, "node %s, %d triangles..."UUmNL, node->name, num_tris);
	}

	for (i=0; i<num_tris; i++)
	{
		float length;
		const float tiny= 0.001f; // values less that this == 0
		int a, b, c;

		triangle_datum[i].tri_index= i;
		triangle_datum[i].quad_index= NONE;
		triangle_datum[i].edge_shared_index[0]= NONE;
		triangle_datum[i].edge_shared_index[1]= NONE;
		triangle_datum[i].edge_shared_index[2]= NONE;

		// calculate a temporary normalized normal vector
		a= node->triangles[i].indices[A];
		b= node->triangles[i].indices[B];
		c= node->triangles[i].indices[C];
		
		MUrVector_NormalFromPoints(&node->points[a].point,
			&node->points[b].point, &node->points[c].point,
			&triangle_datum[i].normal);
		
		length= triangle_datum[i].normal.x * triangle_datum[i].normal.x +
			triangle_datum[i].normal.y * triangle_datum[i].normal.y +
			triangle_datum[i].normal.z + triangle_datum[i].normal.z;
		length= 1.f/length;
		triangle_datum[i].normal.x*= length;
		triangle_datum[i].normal.y*= length;
		triangle_datum[i].normal.z*= length;
		if (fabs(triangle_datum[i].normal.x) < tiny)
			triangle_datum[i].normal.x= 0.f;
		if (fabs(triangle_datum[i].normal.y) < tiny)
			triangle_datum[i].normal.y= 0.f;
		if (fabs(triangle_datum[i].normal.z) < tiny)
			triangle_datum[i].normal.z= 0.f;
	}

	// determine shared edges
	for (i=0; i<num_tris; i++)
	{
		int j;
		int t0_ab[2];
		int t0_bc[2];
		int t0_ca[2];

		t0_ab[0]= node->triangles[i].indices[A];
		t0_ab[1]= node->triangles[i].indices[B];
		t0_bc[0]= node->triangles[i].indices[B];
		t0_bc[1]= node->triangles[i].indices[C];
		t0_ca[0]= node->triangles[i].indices[C];
		t0_ca[1]= node->triangles[i].indices[A];

		for (j=0; j<num_tris; j++)
		{
			if (i != j)
			{
				int t1_ab[2];
				int t1_bc[2];
				int t1_ca[2];

				t1_ab[0]= node->triangles[j].indices[A];
				t1_ab[1]= node->triangles[j].indices[B];
				t1_bc[0]= node->triangles[j].indices[B];
				t1_bc[1]= node->triangles[j].indices[C];
				t1_ca[0]= node->triangles[j].indices[C];
				t1_ca[1]= node->triangles[j].indices[A];

				/*
					only count shared edges on triangles that are coplanar;
					if they share an edge and are parallel, they are coplanar &
					therefor can be combined into a quad
				*/
			
				// ab == a'b'
				if (edge_shared(t0_ab, t1_ab))
				{
					if (triangles_parallel(&triangle_datum[i], &triangle_datum[j], node))
					{
						triangle_datum[i].edge_shared_index[AB]= j;
						triangle_datum[j].edge_shared_index[AB]= i;
					}
				}
				// ab == b'c'
				else if (edge_shared(t0_ab, t1_bc))
				{
					if (triangles_parallel(&triangle_datum[i], &triangle_datum[j], node))
					{
						triangle_datum[i].edge_shared_index[AB]= j;
						triangle_datum[j].edge_shared_index[BC]= i;
					}
				}
				// ab == c'a'
				else if (edge_shared(t0_ab, t1_ca))
				{
					if (triangles_parallel(&triangle_datum[i], &triangle_datum[j], node))
					{
						triangle_datum[i].edge_shared_index[AB]= j;
						triangle_datum[j].edge_shared_index[CA]= i;
					}
				}
				// bc == a'b'
				else if (edge_shared(t0_bc, t1_ab))
				{
					if (triangles_parallel(&triangle_datum[i], &triangle_datum[j], node))
					{
						triangle_datum[i].edge_shared_index[BC]= j;
						triangle_datum[j].edge_shared_index[AB]= i;
					}
				}
				// bc == b'c'
				else if (edge_shared(t0_bc, t1_bc))
				{
					if (triangles_parallel(&triangle_datum[i], &triangle_datum[j], node))
					{
						triangle_datum[i].edge_shared_index[BC]= j;
						triangle_datum[j].edge_shared_index[BC]= i;
					}
				}
				// bc == c'a'
				else if (edge_shared(t0_bc, t1_ca))
				{
					if (triangles_parallel(&triangle_datum[i], &triangle_datum[j], node))
					{
						triangle_datum[i].edge_shared_index[BC]= j;
						triangle_datum[j].edge_shared_index[CA]= i;
					}
				}
				// ca == a'b'
				else if (edge_shared(t0_ca, t1_ab))
				{
					if (triangles_parallel(&triangle_datum[i], &triangle_datum[j], node))
					{
						triangle_datum[i].edge_shared_index[CA]= j;
						triangle_datum[j].edge_shared_index[AB]= i;
					}
				}
				// ca == b'c'
				else if (edge_shared(t0_ca, t1_bc))
				{
					if (triangles_parallel(&triangle_datum[i], &triangle_datum[j], node))
					{
						triangle_datum[i].edge_shared_index[CA]= j;
						triangle_datum[j].edge_shared_index[BC]= i;
					}
				}
				// ca == c'a'
				else if (edge_shared(t0_ca, t1_ca))
				{
					if (triangles_parallel(&triangle_datum[i], &triangle_datum[j], node))
					{
						triangle_datum[i].edge_shared_index[CA]= j;
						triangle_datum[j].edge_shared_index[CA]= i;
					}
				}
			}
		}
	}

	// group triangle pairs with a shared edge as a quad
	for (i=0; i<node->numTriangles; i++)
	{
		int j;

		for (j=0; j<node->numTriangles; j++)
		{
			if ((i != j) &&
				(triangle_datum[i].quad_index == NONE) &&
				(triangle_datum[j].quad_index == NONE))
			{
				int shared_edge0= NONE;
				int shared_edge1= NONE;

				// ti_ab is shared with ...
				if (triangle_datum[i].edge_shared_index[AB] == j)
				{
					// tj_ab
					if (triangle_datum[j].edge_shared_index[AB] == i)
					{
						shared_edge0= AB; shared_edge1= AB;
					}
					// tj_bc
					else if (triangle_datum[j].edge_shared_index[BC] == i)
					{
						shared_edge0= AB; shared_edge1= BC;
					}
					// tj_ca
					else if (triangle_datum[j].edge_shared_index[CA] == i)
					{
						shared_edge0= AB; shared_edge1= CA;
					}
					else
					{
						UUmAssert(0);
					}
				}
				// ti_bc is shared with ...
				else if (triangle_datum[i].edge_shared_index[BC] == j)
				{
					// tj_ab
					if (triangle_datum[j].edge_shared_index[AB] == i)
					{
						shared_edge0= BC; shared_edge1= AB;
					}
					// tj_bc
					else if (triangle_datum[j].edge_shared_index[BC] == i)
					{
						shared_edge0= BC; shared_edge1= BC;
					}
					// tj_ca
					else if (triangle_datum[j].edge_shared_index[CA] == i)
					{
						shared_edge0= BC; shared_edge1= CA;
					}
					else
					{
						UUmAssert(0);
					}
				}
				// ti_ca is shared with ...
				else if (triangle_datum[i].edge_shared_index[CA] == j)
				{
					// tj_ab
					if (triangle_datum[j].edge_shared_index[AB] == i)
					{
						shared_edge0= CA; shared_edge1= AB;
					}
					// tj_bc
					else if (triangle_datum[j].edge_shared_index[BC] == i)
					{
						shared_edge0= CA; shared_edge1= BC;
					}
					// tj_ca
					else if (triangle_datum[j].edge_shared_index[CA] == i)
					{
						shared_edge0= CA; shared_edge1= CA;
					}
					else
					{
						UUmAssert(0);
					}
				}

				if ((shared_edge0 != NONE) && (shared_edge1 != NONE))
				{
					if (build_quad(node, &triangle_datum[i], &triangle_datum[j],
						shared_edge0, shared_edge1, &quads[num_quads], debug_file))
					{
						triangle_datum[i].quad_index= num_quads;
						triangle_datum[j].quad_index= num_quads;
						++num_quads;
						num_tris-= 2;

						if (debug_file) {
							BFrFile_Printf(debug_file, "  merge tris %d %d -> quad %d"UUmNL, i, j, num_quads - 1);
						}
					}
				}
			}
		}
	}

	// create optimized node
	{
		int sizeof_header= sizeof(MXtNode);
		int sizeof_points= node->numPoints * sizeof(MXtPoint);
		int sizeof_triangles= num_tris * sizeof(MXtFace);
		int sizeof_quads= num_quads * sizeof(MXtFace);
		char *buffer= UUrMemory_Block_New(sizeof_header + sizeof_points + sizeof_triangles + sizeof_quads);
		
		new_node= (MXtNode *) buffer;
		UUmAssert(new_node);

		UUrMemory_Clear(new_node, sizeof(MXtNode));
		buffer+= sizeof_header;

		new_node->points= (MXtPoint *)buffer;
		buffer+= sizeof_points;

		new_node->triangles= (MXtFace *)buffer;
		buffer+= sizeof_triangles;

		new_node->quads= (MXtFace *)buffer;
		buffer+= sizeof_quads;

		new_node->markers= NULL;
		new_node->materials= NULL;
		new_node->userData= NULL;

		new_node->numPoints= node->numPoints;
		UUrMemory_MoveFast(node->points, new_node->points, sizeof_points);

		new_node->numTriangles= num_tris;
		if (num_tris > 0)
		{
			int n= 0;

			for (i=0; i < node->numTriangles; i++)
			{
				if (triangle_datum[i].quad_index == NONE) {
					if (debug_file) {
						BFrFile_Printf(debug_file, "  write unmerged tri %d out as #%d"UUmNL, i, n);
					}
					new_node->triangles[n++]= node->triangles[triangle_datum[i].tri_index];
				}
			}
			UUmAssert(n == num_tris);
		}

		new_node->numQuads= num_quads;
		if (num_quads > 0) UUrMemory_MoveFast(quads, new_node->quads, sizeof_quads);

		if (debug_file) {
			BFrFile_Printf(debug_file, "...done, %d tris, %d quads\n\n", num_tris, num_quads);
		}
	}

	// cleanup
	UUrMemory_Block_Delete(triangle_datum);
	UUrMemory_Block_Delete(quads);

	return new_node;
}


