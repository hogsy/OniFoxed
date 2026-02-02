/*
	FILE:	OG_GC_Method_Env.c

	AUTHOR:	Brent H. Pease

	CREATED: June 10, 1999

	PURPOSE:

	Copyright 1997

*/

#ifndef __ONADA__

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Akira.h"
#include "BFW_BitVector.h"

#include "BFW_Shared_Math.h"

#include "Akira_Private.h"

#include "OGL_DrawGeom_Common.h"

#include "OG_GC_Private.h"
#include "OG_GC_Method_Env.h"
#include "OG_GC_Method_Camera.h"

static UUtUns8	g2BitSubtractor[256] =
{
    0x0,    //00000000 -> 00000000
    0x0,    //00000001 -> 00000000
    0x1,    //00000010 -> 00000001
    0x2,    //00000011 -> 00000010
    0x0,    //00000100 -> 00000000
    0x0,    //00000101 -> 00000000
    0x1,    //00000110 -> 00000001
    0x2,    //00000111 -> 00000010
    0x4,    //00001000 -> 00000100
    0x4,    //00001001 -> 00000100
    0x5,    //00001010 -> 00000101
    0x6,    //00001011 -> 00000110
    0x8,    //00001100 -> 00001000
    0x8,    //00001101 -> 00001000
    0x9,    //00001110 -> 00001001
    0xa,    //00001111 -> 00001010
    0x0,    //00010000 -> 00000000
    0x0,    //00010001 -> 00000000
    0x1,    //00010010 -> 00000001
    0x2,    //00010011 -> 00000010
    0x0,    //00010100 -> 00000000
    0x0,    //00010101 -> 00000000
    0x1,    //00010110 -> 00000001
    0x2,    //00010111 -> 00000010
    0x4,    //00011000 -> 00000100
    0x4,    //00011001 -> 00000100
    0x5,    //00011010 -> 00000101
    0x6,    //00011011 -> 00000110
    0x8,    //00011100 -> 00001000
    0x8,    //00011101 -> 00001000
    0x9,    //00011110 -> 00001001
    0xa,    //00011111 -> 00001010
    0x10,   //00100000 -> 00010000
    0x10,   //00100001 -> 00010000
    0x11,   //00100010 -> 00010001
    0x12,   //00100011 -> 00010010
    0x10,   //00100100 -> 00010000
    0x10,   //00100101 -> 00010000
    0x11,   //00100110 -> 00010001
    0x12,   //00100111 -> 00010010
    0x14,   //00101000 -> 00010100
    0x14,   //00101001 -> 00010100
    0x15,   //00101010 -> 00010101
    0x16,   //00101011 -> 00010110
    0x18,   //00101100 -> 00011000
    0x18,   //00101101 -> 00011000
    0x19,   //00101110 -> 00011001
    0x1a,   //00101111 -> 00011010
    0x20,   //00110000 -> 00100000
    0x20,   //00110001 -> 00100000
    0x21,   //00110010 -> 00100001
    0x22,   //00110011 -> 00100010
    0x20,   //00110100 -> 00100000
    0x20,   //00110101 -> 00100000
    0x21,   //00110110 -> 00100001
    0x22,   //00110111 -> 00100010
    0x24,   //00111000 -> 00100100
    0x24,   //00111001 -> 00100100
    0x25,   //00111010 -> 00100101
    0x26,   //00111011 -> 00100110
    0x28,   //00111100 -> 00101000
    0x28,   //00111101 -> 00101000
    0x29,   //00111110 -> 00101001
    0x2a,   //00111111 -> 00101010
    0x0,    //01000000 -> 00000000
    0x0,    //01000001 -> 00000000
    0x1,    //01000010 -> 00000001
    0x2,    //01000011 -> 00000010
    0x0,    //01000100 -> 00000000
    0x0,    //01000101 -> 00000000
    0x1,    //01000110 -> 00000001
    0x2,    //01000111 -> 00000010
    0x4,    //01001000 -> 00000100
    0x4,    //01001001 -> 00000100
    0x5,    //01001010 -> 00000101
    0x6,    //01001011 -> 00000110
    0x8,    //01001100 -> 00001000
    0x8,    //01001101 -> 00001000
    0x9,    //01001110 -> 00001001
    0xa,    //01001111 -> 00001010
    0x0,    //01010000 -> 00000000
    0x0,    //01010001 -> 00000000
    0x1,    //01010010 -> 00000001
    0x2,    //01010011 -> 00000010
    0x0,    //01010100 -> 00000000
    0x0,    //01010101 -> 00000000
    0x1,    //01010110 -> 00000001
    0x2,    //01010111 -> 00000010
    0x4,    //01011000 -> 00000100
    0x4,    //01011001 -> 00000100
    0x5,    //01011010 -> 00000101
    0x6,    //01011011 -> 00000110
    0x8,    //01011100 -> 00001000
    0x8,    //01011101 -> 00001000
    0x9,    //01011110 -> 00001001
    0xa,    //01011111 -> 00001010
    0x10,   //01100000 -> 00010000
    0x10,   //01100001 -> 00010000
    0x11,   //01100010 -> 00010001
    0x12,   //01100011 -> 00010010
    0x10,   //01100100 -> 00010000
    0x10,   //01100101 -> 00010000
    0x11,   //01100110 -> 00010001
    0x12,   //01100111 -> 00010010
    0x14,   //01101000 -> 00010100
    0x14,   //01101001 -> 00010100
    0x15,   //01101010 -> 00010101
    0x16,   //01101011 -> 00010110
    0x18,   //01101100 -> 00011000
    0x18,   //01101101 -> 00011000
    0x19,   //01101110 -> 00011001
    0x1a,   //01101111 -> 00011010
    0x20,   //01110000 -> 00100000
    0x20,   //01110001 -> 00100000
    0x21,   //01110010 -> 00100001
    0x22,   //01110011 -> 00100010
    0x20,   //01110100 -> 00100000
    0x20,   //01110101 -> 00100000
    0x21,   //01110110 -> 00100001
    0x22,   //01110111 -> 00100010
    0x24,   //01111000 -> 00100100
    0x24,   //01111001 -> 00100100
    0x25,   //01111010 -> 00100101
    0x26,   //01111011 -> 00100110
    0x28,   //01111100 -> 00101000
    0x28,   //01111101 -> 00101000
    0x29,   //01111110 -> 00101001
    0x2a,   //01111111 -> 00101010
    0x40,   //10000000 -> 01000000
    0x40,   //10000001 -> 01000000
    0x41,   //10000010 -> 01000001
    0x42,   //10000011 -> 01000010
    0x40,   //10000100 -> 01000000
    0x40,   //10000101 -> 01000000
    0x41,   //10000110 -> 01000001
    0x42,   //10000111 -> 01000010
    0x44,   //10001000 -> 01000100
    0x44,   //10001001 -> 01000100
    0x45,   //10001010 -> 01000101
    0x46,   //10001011 -> 01000110
    0x48,   //10001100 -> 01001000
    0x48,   //10001101 -> 01001000
    0x49,   //10001110 -> 01001001
    0x4a,   //10001111 -> 01001010
    0x40,   //10010000 -> 01000000
    0x40,   //10010001 -> 01000000
    0x41,   //10010010 -> 01000001
    0x42,   //10010011 -> 01000010
    0x40,   //10010100 -> 01000000
    0x40,   //10010101 -> 01000000
    0x41,   //10010110 -> 01000001
    0x42,   //10010111 -> 01000010
    0x44,   //10011000 -> 01000100
    0x44,   //10011001 -> 01000100
    0x45,   //10011010 -> 01000101
    0x46,   //10011011 -> 01000110
    0x48,   //10011100 -> 01001000
    0x48,   //10011101 -> 01001000
    0x49,   //10011110 -> 01001001
    0x4a,   //10011111 -> 01001010
    0x50,   //10100000 -> 01010000
    0x50,   //10100001 -> 01010000
    0x51,   //10100010 -> 01010001
    0x52,   //10100011 -> 01010010
    0x50,   //10100100 -> 01010000
    0x50,   //10100101 -> 01010000
    0x51,   //10100110 -> 01010001
    0x52,   //10100111 -> 01010010
    0x54,   //10101000 -> 01010100
    0x54,   //10101001 -> 01010100
    0x55,   //10101010 -> 01010101
    0x56,   //10101011 -> 01010110
    0x58,   //10101100 -> 01011000
    0x58,   //10101101 -> 01011000
    0x59,   //10101110 -> 01011001
    0x5a,   //10101111 -> 01011010
    0x60,   //10110000 -> 01100000
    0x60,   //10110001 -> 01100000
    0x61,   //10110010 -> 01100001
    0x62,   //10110011 -> 01100010
    0x60,   //10110100 -> 01100000
    0x60,   //10110101 -> 01100000
    0x61,   //10110110 -> 01100001
    0x62,   //10110111 -> 01100010
    0x64,   //10111000 -> 01100100
    0x64,   //10111001 -> 01100100
    0x65,   //10111010 -> 01100101
    0x66,   //10111011 -> 01100110
    0x68,   //10111100 -> 01101000
    0x68,   //10111101 -> 01101000
    0x69,   //10111110 -> 01101001
    0x6a,   //10111111 -> 01101010
    0x80,   //11000000 -> 10000000
    0x80,   //11000001 -> 10000000
    0x81,   //11000010 -> 10000001
    0x82,   //11000011 -> 10000010
    0x80,   //11000100 -> 10000000
    0x80,   //11000101 -> 10000000
    0x81,   //11000110 -> 10000001
    0x82,   //11000111 -> 10000010
    0x84,   //11001000 -> 10000100
    0x84,   //11001001 -> 10000100
    0x85,   //11001010 -> 10000101
    0x86,   //11001011 -> 10000110
    0x88,   //11001100 -> 10001000
    0x88,   //11001101 -> 10001000
    0x89,   //11001110 -> 10001001
    0x8a,   //11001111 -> 10001010
    0x80,   //11010000 -> 10000000
    0x80,   //11010001 -> 10000000
    0x81,   //11010010 -> 10000001
    0x82,   //11010011 -> 10000010
    0x80,   //11010100 -> 10000000
    0x80,   //11010101 -> 10000000
    0x81,   //11010110 -> 10000001
    0x82,   //11010111 -> 10000010
    0x84,   //11011000 -> 10000100
    0x84,   //11011001 -> 10000100
    0x85,   //11011010 -> 10000101
    0x86,   //11011011 -> 10000110
    0x88,   //11011100 -> 10001000
    0x88,   //11011101 -> 10001000
    0x89,   //11011110 -> 10001001
    0x8a,   //11011111 -> 10001010
    0x90,   //11100000 -> 10010000
    0x90,   //11100001 -> 10010000
    0x91,   //11100010 -> 10010001
    0x92,   //11100011 -> 10010010
    0x90,   //11100100 -> 10010000
    0x90,   //11100101 -> 10010000
    0x91,   //11100110 -> 10010001
    0x92,   //11100111 -> 10010010
    0x94,   //11101000 -> 10010100
    0x94,   //11101001 -> 10010100
    0x95,   //11101010 -> 10010101
    0x96,   //11101011 -> 10010110
    0x98,   //11101100 -> 10011000
    0x98,   //11101101 -> 10011000
    0x99,   //11101110 -> 10011001
    0x9a,   //11101111 -> 10011010
    0xa0,   //11110000 -> 10100000
    0xa0,   //11110001 -> 10100000
    0xa1,   //11110010 -> 10100001
    0xa2,   //11110011 -> 10100010
    0xa0,   //11110100 -> 10100000
    0xa0,   //11110101 -> 10100000
    0xa1,   //11110110 -> 10100001
    0xa2,   //11110111 -> 10100010
    0xa4,   //11111000 -> 10100100
    0xa4,   //11111001 -> 10100100
    0xa5,   //11111010 -> 10100101
    0xa6,   //11111011 -> 10100110
    0xa8,   //11111100 -> 10101000
    0xa8,   //11111101 -> 10101000
    0xa9,   //11111110 -> 10101001
    0xaa,   //11111111 -> 10101010
};

UUtError
OGrGeomContext_Method_Env_SetCamera(
	M3tGeomCamera*		inCamera)
{

	return UUcError_None;
}


UUtError
OGrGeomContext_Method_Env_DrawGQList(
	UUtUns32	inNumGQs,
	UUtUns32*	inGQIndices,
	UUtBool		inTransparent)
{
	AKtEnvironment_Private*	environmentPrivate = TMrTemplate_PrivateData_GetDataPtr(AKgTemplate_PrivateData, OGgGeomContextPrivate.environment);

	AKtGQ_General*			gqGeneralArray;
	AKtGQ_Render*			gqRenderArray;
	M3tPoint3D*				worldPoints;
	M3tTextureCoord*		baseUVs;
	M3tTextureMap**			baseMapArray;

	AKtGQ_General*			curGQGeneral;
	AKtGQ_Render*			curGQRender;

	UUtUns32				curGQIndex;

	UUtBool					useDebugMaps = UUcFalse;
	UUtBool					drawGhostGQs = UUcFalse;
	UUtUns32				debugState;

	M3tPoint3D*				tempPoints;
	M3tTextureCoord*		tempBaseUVs;
	M3tTextureCoord*		tempLightUVs;
	M3tTextureMap**			tempBaseMaps;

	//UUtUns32				itr;

	UUtUns32				numGQBlock8;
	UUtUns32				block8Itr;
	UUtUns32				numGQLeftOver;
	UUtUns32				numGQItrs;
	UUtUns32				gqItr;
	UUtUns32				vertItr;

	//return UUcError_None;

	if(OGgGeomContextPrivate.environment == NULL) return UUcError_None;

	UUmAssertReadPtr(OGgGeomContextPrivate.environment, sizeof(*OGgGeomContextPrivate.environment));

	worldPoints		= OGgGeomContextPrivate.environment->pointArray->points;
	gqGeneralArray	= OGgGeomContextPrivate.environment->gqGeneralArray->gqGeneral;
	gqRenderArray	= OGgGeomContextPrivate.environment->gqRenderArray->gqRender;
	baseUVs			= OGgGeomContextPrivate.environment->textureCoordArray->textureCoords;
	baseMapArray	= OGgGeomContextPrivate.environment->textureMapArray->maps;


	// debugState		= OGgGeomContextPrivate.stateInt[M3cGeomStateIntType_DebugMode];
	debugState = 0;

	tempPoints		= OGgGeomContextPrivate.tempPoints;
	tempBaseUVs		= OGgGeomContextPrivate.tempBaseUVs;
	tempLightUVs	= OGgGeomContextPrivate.tempLightUVs;
	tempBaseMaps	= OGgGeomContextPrivate.tempBaseMaps;

	useDebugMaps	= (UUtBool)((debugState & M3cGeomState_DebugMode_UseEnvDbgTexture) != 0);
	drawGhostGQs	= (UUtBool)((debugState & M3cGeomState_DebugMode_DrawGhostGQs) != 0);

	if(inNumGQs == 0) return UUcError_None;

	OGLrCommon_Camera_Update(OGLcCameraMode_3D);

	#if 0
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_INDEX_ARRAY);
	glDisableClientState(GL_EDGE_FLAG_ARRAY);
	#endif

	// Idea here is to build up a list of 8 quads and submit their vertices at once
	// and then submit the quads using glDrawArrays

	numGQBlock8 = inNumGQs >> 3;
	numGQLeftOver = inNumGQs - (numGQBlock8 << 3);
	numGQBlock8++;

	for(block8Itr = 0; block8Itr < numGQBlock8; block8Itr++)
	{
		if(block8Itr == numGQBlock8 - 1)
		{
			numGQItrs = numGQLeftOver;
		}
		else
		{
			numGQItrs = 8;
		}

		//OGLgCommon.pglUnlockArraysEXT();

		for(gqItr = 0, vertItr = 0; gqItr < numGQItrs; gqItr++, vertItr += 4)
		{
			curGQIndex = inGQIndices[gqItr + (block8Itr << 3)];

			curGQGeneral = gqGeneralArray + curGQIndex;
			curGQRender = gqRenderArray + curGQIndex;

			tempPoints[vertItr + 0] = worldPoints[curGQGeneral->m3Quad.vertexIndices.indices[0]];
			tempPoints[vertItr + 1] = worldPoints[curGQGeneral->m3Quad.vertexIndices.indices[1]];
			tempPoints[vertItr + 2] = worldPoints[curGQGeneral->m3Quad.vertexIndices.indices[2]];
			tempPoints[vertItr + 3] = worldPoints[curGQGeneral->m3Quad.vertexIndices.indices[3]];

			tempBaseUVs[vertItr + 0] = baseUVs[curGQGeneral->m3Quad.baseUVIndices.indices[0]];
			tempBaseUVs[vertItr + 1] = baseUVs[curGQGeneral->m3Quad.baseUVIndices.indices[1]];
			tempBaseUVs[vertItr + 2] = baseUVs[curGQGeneral->m3Quad.baseUVIndices.indices[2]];
			tempBaseUVs[vertItr + 3] = baseUVs[curGQGeneral->m3Quad.baseUVIndices.indices[3]];

			// set the proper base texture map
			if((curGQGeneral->flags & AKcGQ_Flag_Draw_Flash))
			{
				curGQGeneral->flags &= ~AKcGQ_Flag_Draw_Flash;

				if(curGQGeneral->flags & AKcGQ_Flag_Flash_State)
				{
					curGQGeneral->flags &= ~AKcGQ_Flag_Flash_State;
					tempBaseMaps[gqItr] = NULL;
				}
				else
				{
					curGQGeneral->flags |= AKcGQ_Flag_Flash_State;
					tempBaseMaps[gqItr] = baseMapArray[curGQRender->textureMapIndex];
				}
			}
			else
			{
				tempBaseMaps[gqItr] = baseMapArray[curGQRender->textureMapIndex];
			}
		}

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, tempPoints);

		glTexCoordPointer(2, GL_FLOAT, 0, tempBaseUVs);

		//OGLgCommon.pglLockArraysEXT(0, numGQItrs << 3);
		for(gqItr = 0; gqItr < numGQItrs; gqItr++)
		{
			if(1 || gqItr == 0 || tempBaseMaps[gqItr - 1] != tempBaseMaps[gqItr])
			{
				OGLrCommon_TextureMap_Select(tempBaseMaps[gqItr], GL_TEXTURE0_ARB);
			}

			glDrawArrays(GL_POLYGON, gqItr << 2, 4);
			glFlush();
		}
	}

	return UUcError_None;
}

#endif // __ONADA__

