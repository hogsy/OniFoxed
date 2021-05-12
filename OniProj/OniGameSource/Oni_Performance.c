/*
	FILE:	Oni_Performance.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: April 2, 1997
	
	PURPOSE: main .c file for Oni
	
	Copyright 1997

*/
#if 0
#include <stdio.h>
#include <stdlib.h>

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_TemplateManager.h"

#include "Oni_Performance.h"
#include "Oni_Platform.h"

#if UUmProcessor == UUmPlatform_PPC
	#include "604TimerLib.h"
#endif

void ONrPerformance_Measure_Gouraud(
	void)
{
	long			triWidth, triHeight;
	long			x, y;
	long			numXTris, numYTris;
	long			numTris[25][25];
	double			time[25][25];
	M3tPointScreen	*vCoords, *curVCoord;
	UUtUns16		*vShades, *curVShade;
	//unsigned long	startTimeHi, startTimeLo, stopTimeHi, stopTimeLo;
	//double			elapsedTime;
	float			offset;
	
	vCoords = (M3tPointScreen *)UUrMemory_Block_New(sizeof(M3tPointScreen) * 320 * 240);
	
	UUmAssert(vCoords != NULL);
	
	vShades = (UUtUns16 *)UUrMemory_Block_New(sizeof(UUtUns16) * 320 * 240);
	
	UUmAssert(vShades != NULL);
	
	for(offset = 0.0f; offset <= 1.0f; offset += 0.1f)
	{
		for(triWidth = 2; triWidth <= 10; triWidth += 1)
		{
			for(triHeight = 2; triHeight <= 10; triHeight += 1)
			{
				numXTris = 638 / triWidth;
				numYTris = 478 / triHeight;
				
				for(x = 0; x < numXTris; x++)
				{
					for(y = 0; y < numYTris; y++)
					{
						curVCoord = vCoords + y * numXTris + x;
						curVShade = vShades + y * numXTris + x;
						
						curVCoord->x = (float)x * (float)(triWidth) + offset;
						curVCoord->y = (float)y * (float)(triHeight) + offset;
						curVCoord->z = 0.5;
						
						*curVShade = ((x & 1 ? 0 : 0x1F) << 10) | (0x1F << 5) | (y & 1 ? 0 : 0x1F);
					}
				}
				
				if(M3rFrame_Start(drawContext) != UUcError_None)
				{
					UUrError_Report(UUcError_Generic, "Could not start frame");
					return;
				}

				M3rDraw_State_SetPtr(drawContext, M3cDrawStatePtrType_ScreenPointArray, vCoords);
				M3rDraw_State_SetPtr(drawContext, M3cDrawStatePtrType_ScreenShadeArray_DC, vShades);

				//get604Time(&startTimeHi,&startTimeLo);

				for(x = 0; x < numXTris - 1; x ++)
				{
					for(y = 0; y < numYTris - 1; y ++)
					{
						#if 0
						M3rDraw_TriInterpolate(
							drawContext,
							(UUtUns16)(y * numXTris + x),
							(UUtUns16)(y * numXTris + x + 1),
							(UUtUns16)((y + 1) * numXTris + x + 1));
						#endif
						#if 0
						M3rDraw_TriInterpolate(
							drawContext,
							(UUtUns16)((y + 1) * numXTris + x + 1),
							(UUtUns16)((y + 1) * numXTris + x),
							(UUtUns16)(y * numXTris + x));
						#endif
					}
				}

				//get604Time(&stopTimeHi,&stopTimeLo);

				//elapsedTime = (stopTimeHi - startTimeHi);
				//elapsedTime *= 0x10000;
				//elapsedTime *= 0x10000;
				//elapsedTime += (stopTimeLo - startTimeLo);
				
				numTris[triWidth / 2 - 1][triHeight / 2 - 1] = numXTris * numYTris * 2;
			//	time[triWidth / 2 - 1][triHeight / 2 - 1] = elapsedTime / getTimeBaseMhz();
				
				if(M3rFrame_End(drawContext) != UUcError_None)
				{
					UUrError_Report(UUcError_Generic, "Could not start frame");
					return;
				}
				
			}
		}
	}
	
	if(0)
	{
		FILE *file;
		long x, y;
		
		file = UUrFOpen("triRasterPerf.out", "wb");
		fprintf(file, "triWidth\ttriHeight\tnumTris\tframeTime\t|\ttrisPerSec\tpixelsPerSec"UUmNL);
		
		for(x = 0; x < 25; x++)
		{
			for(y = 0; y < 25; y++)
			{
				fprintf(file, "%8d\t%9d\t%7d\t%9f\t|\t%10f\t\t%f"UUmNL, 
					x * 2 + 2,
					y * 2 + 2,
					numTris[x][y],
					time[x][y],
					numTris[x][y] / time[x][y],
					(double)((x * 2 + 2) * (y * 2 + 2) * numTris[x][y]) / (time[x][y] * 2.0));
			}
		}
		
		fclose(file);
	}
	
	UUrMemory_Block_Delete(vCoords);
	UUrMemory_Block_Delete(vShades);
}

void ONrPerformance_Measure_GouraudFlat(
	void)
{
	long			triWidth, triHeight;
	long			x, y;
	long			numXTris, numYTris;
	long			numTris[25][25];
	double			time[25][25];
	M3tPointScreen	*vCoords, *curVCoord;
	UUtUns16		vShade;
	//unsigned long	startTimeHi, startTimeLo, stopTimeHi, stopTimeLo;
	//double			elapsedTime;
	float			offset;
	
	vCoords = (M3tPointScreen *)UUrMemory_Block_New(sizeof(M3tPointScreen) * 320 * 240);
	
	UUmAssert(vCoords != NULL);
	
	for(offset = 0.0f; offset <= 1.0f; offset += 0.1f)
	{
		for(triWidth = 2; triWidth <= 10; triWidth += 1)
		{
			for(triHeight = 2; triHeight <= 10; triHeight += 1)
			{
				numXTris = 638 / triWidth;
				numYTris = 478 / triHeight;
				
				for(x = 0; x < numXTris; x++)
				{
					for(y = 0; y < numYTris; y++)
					{
						curVCoord = vCoords + y * numXTris + x;
						
						curVCoord->x = (float)x * (float)(triWidth) + offset;
						curVCoord->y = (float)y * (float)(triHeight) + offset;
						curVCoord->z = 0.5;
						
					}
				}
				

				if(M3rFrame_Start(drawContext) != UUcError_None)
				{
					UUrError_Report(UUcError_Generic, "Could not start frame");
					return;
				}

				M3rDraw_State_SetPtr(drawContext, M3cDrawStatePtrType_ScreenPointArray, vCoords);

				//get604Time(&startTimeHi,&startTimeLo);

				for(x = 0; x < numXTris - 1; x ++)
				{
					for(y = 0; y < numYTris - 1; y ++)
					{
						vShade = ((rand() % 32) << 10) | ((rand() % 32) << 5) | ((rand() % 32));
						
						#if 0
						M3rDraw_TriFlat(
							drawContext,
							(UUtUns16)(y * numXTris + x),
							(UUtUns16)(y * numXTris + x + 1),
							(UUtUns16)((y + 1) * numXTris + x + 1),
							vShade);
						#endif
						vShade = ((rand() % 32) << 10) | ((rand() % 32) << 5) | ((rand() % 32));
						#if 0
						M3rDraw_TriFlat(
							drawContext,
							(UUtUns16)((y + 1) * numXTris + x + 1),
							(UUtUns16)((y + 1) * numXTris + x),
							(UUtUns16)(y * numXTris + x),
							vShade);
						#endif
					}
				}

				//get604Time(&stopTimeHi,&stopTimeLo);

				//elapsedTime = (stopTimeHi - startTimeHi);
				//elapsedTime *= 0x10000;
				//elapsedTime *= 0x10000;
				//elapsedTime += (stopTimeLo - startTimeLo);
				
				numTris[triWidth / 2 - 1][triHeight / 2 - 1] = numXTris * numYTris * 2;
			//	time[triWidth / 2 - 1][triHeight / 2 - 1] = elapsedTime / getTimeBaseMhz();
				
				if(M3rFrame_End(drawContext) != UUcError_None)
				{
					UUrError_Report(UUcError_Generic, "Could not start frame");
					return;
				}
				
			}
		}
	}
	
	if(0)
	{
		FILE *file;
		long x, y;
		
		file = UUrFOpen("triRasterPerf.out", "wb");
		fprintf(file, "triWidth\ttriHeight\tnumTris\tframeTime\t|\ttrisPerSec\tpixelsPerSec"UUmNL);
		
		for(x = 0; x < 25; x++)
		{
			for(y = 0; y < 25; y++)
			{
				fprintf(file, "%8d\t%9d\t%7d\t%9f\t|\t%10f\t\t%f"UUmNL, 
					x * 2 + 2,
					y * 2 + 2,
					numTris[x][y],
					time[x][y],
					numTris[x][y] / time[x][y],
					(double)((x * 2 + 2) * (y * 2 + 2) * numTris[x][y]) / (time[x][y] * 2.0));
			}
		}
		
		fclose(file);
	}
	
	UUrMemory_Block_Delete(vCoords);
}

UUtError
ONrPerformance_Measure_Texture(
	void)
{
#if 0
	long			triWidth, triHeight;
	long			x, y;
	long			numXTris, numYTris;
	long			numTris[25][25];
	double			time[25][25];
	M3tPointScreen	*vCoords, *curVCoord;
	UUtUns16		*vShades, *curVShade;
	M3tTextureCoord*	vTextureCoords, *curVTextureCoord;
	
	//unsigned long	startTimeHi, startTimeLo, stopTimeHi, stopTimeLo;
	//double			elapsedTime;
	float			offset;
	M3tTextureMap	*myTexture;
	UUtError		error;
	
	{
		char objectIndex;
		
		for(objectIndex = '0'; objectIndex <= '9'; objectIndex++)
		{
			error =
				TMrInstance_GetDataPtr(
					M3cTemplate_TextureMap,
					"MY00",
					&myTexture);

			if (UUcError_None == error) { break; }
		}
	}
	UUmError_ReturnOnError(error);
	
	#if 0
	for(x = 0; x < 256; x++)
	{
		for(y = 0; y < 256; y++)
		{
			*((char *)myTexture->data + y * 256 + x) = (UUtUns16)(
				((x >> 3) << 10) |
				((y >> 3) << 5));
		}
	}
	#endif
	
	{
		M3tPointScreen	vc[4];
		UUtUns16		vs[4];
		M3tTextureCoord	vt[4];
		
		vs[0] = vs[1] = vs[2] = vs[3] = 0xFFFF;
		
		vc[0].x = 0.0;
		vc[0].y = 0.0;
		vc[0].z = 0.5;
		vc[0].invW = 1.0f / vc[0].z;
		
		vt[0].u = 0;
		vt[0].v = 1 << 8;
		
		vc[1].x = 0.0;
		vc[1].y = 256.0;
		vc[1].z = 0.5;
		vc[1].invW = 1.0f / vc[1].z;
		
		vt[1].u = 0;
		vt[1].v = 0;

		vc[2].x = 256.0;
		vc[2].y = 0.0;
		vc[2].z = 0.5;
		vc[2].invW = 1.0f / vc[2].z;
		
		vt[2].u = 1 << 8;
		vt[2].v = 1 << 8;

		vc[3].x = 256.0;
		vc[3].y = 256.0;
		vc[3].z = 0.5;
		vc[3].invW = 1.0f / vc[3].z;
		
		vt[3].u = 1 << 8;
		vt[3].v = 0;

		if(M3rFrame_Start(drawContext) != UUcError_None)
		{
			UUrError_Report(UUcError_Generic, "Could not start frame");
			return UUcError_Generic;
		}

		M3rDraw_State_SetPtr(drawContext, M3cDrawStatePtrType_ScreenPointArray,  vc);
		M3rDraw_State_SetPtr(drawContext, M3cDrawStatePtrType_ScreenShadeArray_DC, vs);
		M3rDraw_State_SetPtr(drawContext, M3cDrawStatePtrType_TextureCoordArray, vt);
		M3rDraw_State_SetPtr(drawContext, M3cDrawStatePtrType_BaseTextureMap, myTexture);
		
		M3rDraw_TriInterpolate(
			drawContext,
			0,
			1,
			2);

		M3rDraw_TriInterpolate(
			drawContext,
			1,
			2,
			3);

		if(M3rFrame_End(drawContext) != UUcError_None)
		{
			UUrError_Report(UUcError_Generic, "Could not start frame");
			return UUcError_Generic;
		}
	}
	
	
	vCoords = (M3tPointScreen *)UUrMemory_Block_New(sizeof(M3tPointScreen) * 320 * 240);
	
	UUmAssert(vCoords != NULL);
	
	vShades = (UUtUns16 *)UUrMemory_Block_New(sizeof(UUtUns16) * 320 * 240);
	
	UUmAssert(vShades != NULL);
	
	vTextureCoords = (M3tTextureCoord*)UUrMemory_Block_New(sizeof(M3tTextureCoord) * 320 * 240);
	
	UUmAssert(vTextureCoords != NULL);
	
	for(offset = 0.0f; offset <= 1.0f; offset += 0.1f)
	{
		for(triWidth = 10; triWidth <= 25; triWidth += 2)
		{
			for(triHeight = 10; triHeight <= 25; triHeight += 2)
			{
				numXTris = 638 / triWidth;
				numYTris = 478 / triHeight;
				
				for(x = 0; x < numXTris; x++)
				{
					for(y = 0; y < numYTris; y++)
					{
						curVCoord = vCoords + y * numXTris + x;
						curVShade = vShades + y * numXTris + x;
						curVTextureCoord = vTextureCoords + y * numXTris + x;
						
						curVCoord->x = (float)x * (float)(triWidth) + offset;
						curVCoord->y = (float)y * (float)(triHeight) + offset;
						curVCoord->z = 0.5;
						curVCoord->invW = 1.0f / curVCoord->z;
						
						*curVShade = ((x & 1 ? 0 : 0x1F) << 10) | (0x1F << 5) | (y & 1 ? 0 : 0x1F);
						
						// NOTE: if something breaks around here I added this cast without
						// looking at the code         - Michael Evans
						curVTextureCoord->u = (float) (x & 1 ? 0 : 255);
						curVTextureCoord->v = (float) (y & 1 ? 0 : 255);
					}
				}
				
				if(M3rFrame_Start(drawContext) != UUcError_None)
				{
					UUrError_Report(UUcError_Generic, "Could not start frame");
					return UUcError_Generic;
				}

				M3rDraw_State_SetPtr(drawContext, M3cDrawStatePtrType_ScreenPointArray, vCoords);
				M3rDraw_State_SetPtr(drawContext, M3cDrawStatePtrType_ScreenShadeArray_DC, vShades);
				M3rDraw_State_SetPtr(drawContext, M3cDrawStatePtrType_TextureCoordArray, vTextureCoords);
				M3rDraw_State_SetPtr(drawContext, M3cDrawStatePtrType_BaseTextureMap, myTexture);
				
				//get604Time(&startTimeHi,&startTimeLo);

				for(x = 0; x < numXTris - 1; x ++)
				{
					for(y = 0; y < numYTris - 1; y ++)
					{
						#if 1
						M3rDraw_TriInterpolate(
							drawContext,
							(UUtUns16)(y * numXTris + x),
							(UUtUns16)(y * numXTris + x + 1),
							(UUtUns16)((y + 1) * numXTris + x + 1));
						#endif
						#if 1
						M3rDraw_TriInterpolate(
							drawContext,
							(UUtUns16)((y + 1) * numXTris + x + 1),
							(UUtUns16)((y + 1) * numXTris + x),
							(UUtUns16)(y * numXTris + x));
						#endif
					}
				}

			//	get604Time(&stopTimeHi,&stopTimeLo);

				//elapsedTime = (stopTimeHi - startTimeHi);
				//elapsedTime *= 0x10000;
				//elapsedTime *= 0x10000;
				//elapsedTime += (stopTimeLo - startTimeLo);
				
				numTris[triWidth / 2 - 1][triHeight / 2 - 1] = numXTris * numYTris * 2;
			//	time[triWidth / 2 - 1][triHeight / 2 - 1] = elapsedTime / getTimeBaseMhz();
				
				if(M3rFrame_End(drawContext) != UUcError_None)
				{
					UUrError_Report(UUcError_Generic, "Could not start frame");
					return UUcError_Generic;
				}
				
			}
		}
	}
	
	if(0)
	{
		FILE *file;
		
		file = UUrFOpen("triRasterPerf.out", "wb");
		fprintf(file, "triWidth\ttriHeight\tnumTris\tframeTime\t|\ttrisPerSec\tpixelsPerSec"UUmNL);
		
		for(x = 0; x < 25; x++)
		{
			for(y = 0; y < 25; y++)
			{
				fprintf(file, "%8d\t%9d\t%7d\t%9f\t|\t%10f\t\t%f"UUmNL, 
					x * 2 + 2,
					y * 2 + 2,
					numTris[x][y],
					time[x][y],
					numTris[x][y] / time[x][y],
					(double)((x * 2 + 2) * (y * 2 + 2) * numTris[x][y]) / (time[x][y] * 2.0));
			}
		}
		
		fclose(file);
	}
	
	UUrMemory_Block_Delete(vCoords);
	UUrMemory_Block_Delete(vShades);
#endif
	return UUcError_None;
}
#endif