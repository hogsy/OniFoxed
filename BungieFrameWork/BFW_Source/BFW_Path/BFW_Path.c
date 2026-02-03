/*
	BFW_Path.c

	This file contains all BFW pathfinding related code

	Author: Quinn Dunki
	c1998 Bungie
*/

#include "BFW.h"
#include "BFW_Akira.h"
#include "BFW_TM_Construction.h"
#include "BFW_Path.h"
#include "BFW_MathLib.h"
#include "BFW_AppUtilities.h"
#include <stdlib.h>

#define PHmSwap(a,b) { t=a; a=b; b=t; }

int symwuline(int a1, int b1, int a2, int b2);
UUtUns32 gWidth,gHeight;
PHtSquare *gGrid,gFill;
UUtBool gOverwrite;

const char *PHcWeightName[PHcMax] = {"clear", "nearwall", "border1", "border2", "semipassable", "border3", "border4", "stairs", "danger", "impassable"};
const char PHcPrintChar[PHcMax]  = {' ',  '.', '1',  '2',  'x', '3',  '4', 'S', '*',  'X'};
const float PHcPathWeight[PHcMax] = {1.0f, 2.0f, 2.5f, 3.0f, 3.5f, 5.0f, 10.0f, 20.0f, 400.0f, 1000.0f};
const char *PHcDebugEventName[PHcDebugEvent_Max] = {"none", "floor", "wall", "sloped", "danger", "impassable", "stair"};

UUtError PHrInitialize(void)
{

	return UUcError_None;
}

void PHrTerminate(void)
{
}

void PHrBresenham2(
	UUtInt16 x1,
	UUtInt16 y1,
	UUtInt16 x2,
	UUtInt16 y2,
	PHtSquare *grid,
	UUtUns32 inWidth,
	UUtUns32 inHeight,
	PHtSquare *fill,
	PHtRasterizationCallback callback);


void PHrPutObstruction(
	PHtDynamicSquare *grid,
	UUtUns16 height,
	UUtUns16 width,
	UUtInt16 s,
	UUtInt16 t,
	UUtUns8 obstruction)
{
	if (s>=0 && s<width && t>=0 && t<height)
	{
		PutObstruction(s,t,obstruction);
	}
}

void PHrCheckAndPutCell(
	PHtSquare *grid,
	UUtUns16 height,
	UUtUns16 width,
	UUtInt16 s,
	UUtInt16 t,
	PHtSquare *c)
{
	PHtSquare cell_macro_value;
	UUtUns8 weight;

	if (s>=0 && s<width && t>=0 && t<height)
	{
		cell_macro_value = GetCell(s,t);

		weight = c->weight;

		if (cell_macro_value.weight < weight)
		{
			PutCell(s,t,weight);
			DEBUG_PH_PLOT;
		}
	}
}


void PHrCheckAndBlastCell(
	PHtSquare *grid,
	UUtUns16 height,
	UUtUns16 width,
	UUtInt16 s,
	UUtInt16 t,
	PHtSquare *c)	// or heightmap
{
	PHtSquare cell_macro_value;
	UUtUns8 weight;

	if (s>=0 && s<width && t>=0 && t<height)
	{
		cell_macro_value = GetCell(s,t);

		weight = c->weight;

		PutCell(s,t,weight);
		DEBUG_PH_PLOT;
	}
}

void PHrGridToWorldSpace(UUtUns16 inGridX, UUtUns16 inGridY, const float *inAltitude, M3tPoint3D *outWorld, const PHtRoomData *inRoom)
{
	/**********************
	* Converts the point (inGridX,inGridY) into world space coordinates with respect
	* to the grid defined for 'inRoom'. Pass the altitude of the room at (inGridX,inGridY)
	* if known, otherwise NULL
	*/

	outWorld->x = (float)(inGridX+inRoom->gox) * inRoom->squareSize + inRoom->squareSize/2.0f + inRoom->origin.x;
	if (inAltitude)
		outWorld->y = *inAltitude;
	else
		outWorld->y = inRoom->origin.y + PHcWorldCoord_YOffset;
	outWorld->z = (float)(inGridY+inRoom->goy) * inRoom->squareSize + inRoom->squareSize/2.0f + inRoom->origin.z;
}

void PHrWorldToGridSpace(UUtUns16 *outGridX, UUtUns16 *outGridY, const M3tPoint3D *inWorld, const PHtRoomData *inRoom)
{
	/************************
	* Converts the world space point 'inWorld' into the coordinate system of the grid
	* defined by 'inRoom'.
	*/
	UUtInt32 gridX, gridY;

	UUmAssertWritePtr(outGridX, sizeof(UUtUns16));
	UUmAssertWritePtr(outGridY, sizeof(UUtUns16));
	UUmAssertReadPtr(inWorld, sizeof(M3tPoint3D));
	UUmAssertReadPtr(inRoom, sizeof(PHtRoomData));

	gridX = MUrFloat_Round_To_Int(((inWorld->x - inRoom->squareSize * 0.5f /* S.S./2.0f*/ - inRoom->origin.x) / inRoom->squareSize - (float)inRoom->gox));
	gridY = MUrFloat_Round_To_Int(((inWorld->z - inRoom->squareSize * 0.5f /* S.S./2.0f*/ - inRoom->origin.z) / inRoom->squareSize - (float)inRoom->goy));

	UUmAssert((gridX >= 0) && ((UUtUns32) gridX < inRoom->gridX));
	UUmAssert((gridY >= 0) && ((UUtUns32) gridY < inRoom->gridY));

	*outGridX = (UUtUns16) gridX;
	*outGridY = (UUtUns16) gridY;
}

void PHrWorldPinToGridSpace(M3tPoint3D *inWorld, const PHtRoomData *inRoom)
{
	float x_min, x_max, z_min, z_max;

	x_min = inRoom->origin.x + ((                  inRoom->gox) * inRoom->squareSize);
	x_max = inRoom->origin.x + ((inRoom->gridX + 2*inRoom->gox) * inRoom->squareSize);
	z_min = inRoom->origin.z + ((                  inRoom->goy) * inRoom->squareSize);
	z_max = inRoom->origin.z + ((inRoom->gridY + 2*inRoom->goy) * inRoom->squareSize);

	inWorld->x = UUmPin(inWorld->x, x_min, x_max);
	inWorld->z = UUmPin(inWorld->z, z_min, z_max);
}

UUtBool PHrWorldInGridSpace(const M3tPoint3D *inWorld, const PHtRoomData *inRoom)
{
	UUtInt32 gridX, gridY;
	UUtBool success;

	UUmAssertReadPtr(inWorld, sizeof(M3tPoint3D));
	UUmAssertReadPtr(inRoom, sizeof(PHtRoomData));

	gridX = MUrFloat_Round_To_Int(((inWorld->x - inRoom->squareSize * 0.5f/* S.S. /2.0f*/ - inRoom->origin.x) / inRoom->squareSize - (float)inRoom->gox));
	gridY = MUrFloat_Round_To_Int(((inWorld->z - inRoom->squareSize * 0.5f/*/2.0f*/ - inRoom->origin.z) / inRoom->squareSize - (float)inRoom->goy));

	success =	(gridX >= 0) && ((UUtUns32) gridX < inRoom->gridX) &&
				(gridY >= 0) && ((UUtUns32) gridY < inRoom->gridY);

	return success;
}

void PHrWorldToGridSpaceDangerous(UUtInt16 *outGridX, UUtInt16 *outGridY, const M3tPoint3D *inWorld, const PHtRoomData *inRoom)
{
	/************************
	* Converts the world space point 'inWorld' into the coordinate system of the grid
	* defined by 'inRoom'. Important- this routine CAN return values that are not valid
	* for the coordinate system of the grid in question. Don't use this routine unless you
	* know what you're doing.
	*/

	UUmAssertWritePtr(outGridX, sizeof(UUtUns16));
	UUmAssertWritePtr(outGridY, sizeof(UUtUns16));
	UUmAssertReadPtr(inWorld, sizeof(M3tPoint3D));
	UUmAssertReadPtr(inRoom, sizeof(PHtRoomData));

	*outGridX = (UUtInt16) MUrFloat_Round_To_Int(((inWorld->x - inRoom->squareSize * 0.5f/* S.S./2.0f*/ - inRoom->origin.x) / inRoom->squareSize - (float)inRoom->gox));
	*outGridY = (UUtInt16) MUrFloat_Round_To_Int(((inWorld->z - inRoom->squareSize * 0.5f/* S.S./2.0f*/ - inRoom->origin.z) / inRoom->squareSize - (float)inRoom->goy));

	return;
}

void PHrWaypointFromGunk(AKtEnvironment *inEnv, AKtGQ_General *inGQGeneral, M3tPoint3D *outPoint, M3tPoint3D *inPoint)
{
	/*************************
	* Given a quad, this returns a waypoint suitable for
	* getting to that quad from 'inPoint'. Pass NULL in 'inPoint'
	* to return the center of the quad on the floor
	*/

	M3tPoint3D *lowest_point_1;
	M3tPoint3D *lowest_point_2;

	// Find lowest two points of quad
	{
		M3tQuad *quad = &inGQGeneral->m3Quad.vertexIndices;
		M3tPoint3D *points = inEnv->pointArray->points;

		AUrQuad_LowestPoints(quad,points,&lowest_point_1,&lowest_point_2);
	}

	// If the doorway is narrow or an SAT, or we want the center, head for the middle of it
	if (
			(NULL == inPoint) ||
			(MUrPoint_Distance_Squared(lowest_point_1,lowest_point_2) <= UUmSQR(PHcComfortDistance * 2.0f))
		)
	{
		outPoint->x = (lowest_point_1->x + lowest_point_2->x) * 0.5f;
		outPoint->y = (lowest_point_1->y + lowest_point_2->y) * 0.5f;
		outPoint->z = (lowest_point_1->z + lowest_point_2->z) * 0.5f;
	}
	else
	{
		MUrLineSegment_ClosestPointToPoint(lowest_point_1,lowest_point_2,inPoint,outPoint);

		if (MUrPoint_Distance_Squared(outPoint,lowest_point_1) < UUmSQR(PHcComfortDistance)) {
			// make a point that is comfort distance away from the lowest_point_1
			// outPoint = lowest_point_1 + scale((lowest_point_2 - lowest_point_1), comfort distance);

			MUmVector_Subtract(*outPoint, *lowest_point_2, *lowest_point_1);
			MUrVector_SetLength(outPoint, PHcComfortDistance);
			MUmVector_Increment(*outPoint, *lowest_point_1);

		}
		else if (MUrPoint_Distance_Squared(outPoint,lowest_point_2) < UUmSQR(PHcComfortDistance)) {
			// make a point that is comfort distance away from the lowest_point_2
			// outPoint = lowest_point_2 + scale((lowest_point_1 - lowest_point_2), comfort distance);

			MUmVector_Subtract(*outPoint, *lowest_point_1, *lowest_point_2);
			MUrVector_SetLength(outPoint, PHcComfortDistance);
			MUmVector_Increment(*outPoint, *lowest_point_2);
		}
	}

	outPoint->y += 1.0f;
}

void PHrBresenhamAA(
	UUtInt16 x1,
	UUtInt16 y1,
	UUtInt16 x2,
	UUtInt16 y2,
	PHtSquare *grid,
	UUtUns32 width,
	UUtUns32 height,
	PHtSquare *fill,
	PHtSquare *blend,
	PHtRasterizationCallback callback)
{
	/****************
	* Draws an antialiased bresenham line with 'fill' as the line,
	* and using 'blend' to smooth the edges.
	*/

	UUtInt16 dx,dy;

	PHrBresenham2(x1,y1,x2,y2,grid,width,height,fill,callback);

	dx = UUmABS(x2-x1);
	dy = UUmABS(y2-y1);

	if (dx > dy)
	{
		PHrBresenham2(x1,y1-1,x2,y2-1,grid,width,height,blend,callback);
		PHrBresenham2(x1,y1+1,x2,y2+1,grid,width,height,blend,callback);
	}
	else
	{
		PHrBresenham2(x1-1,y1,x2-1,y2,grid,width,height,blend,callback);
		PHrBresenham2(x1+1,y1,x2+1,y2,grid,width,height,blend,callback);
	}

	return;
}

void PHrBresenham2(
	UUtInt16 x1,
	UUtInt16 y1,
	UUtInt16 x2,
	UUtInt16 y2,
	PHtSquare *grid,
	UUtUns32 inWidth,
	UUtUns32 inHeight,
	PHtSquare *fill,
	PHtRasterizationCallback callback)
{
	/**************
	* Draws a bresenham line from 1 to 2 in 'grid', assuming 'grid' is 'width'
	* cells wide. Note that this draws slightly thicker lines then a straight
	* Bresenham to prevent diagonal "leaks". The 'height' is only used for
	* input validation. Will not overwrite squares of heavier weight unless
	* the fill colour is PHcClear (which indicates we want to erase)
	*/

	UUtInt32 width = inWidth;
	UUtInt32 height = inHeight;
	UUtInt16 ax,ay,dx,dy,x,y,e,xdir,ydir;
	UUtBool overwrite;

	overwrite = (PHcClear == fill->weight) ? UUcTrue : UUcFalse;

	gOverwrite = overwrite;
	gHeight = inHeight;
	gWidth = inWidth;
	gFill = *fill;
	//symwuline((int)x1,(int)y1,(int)x2,(int)y2);
	//return;

	UUmAssert(inWidth <= UUcMaxInt32);
	UUmAssert(inHeight <= UUcMaxInt32);

//	UUmAssert(x1 >= 0 && x1 < width && y1 >= 0 && y1 < height);
//	UUmAssert(x2 >= 0 && x2 < width && y2 >= 0 && y2 < height);

	dx = x2-x1;
	dy = y2-y1;
	ax = UUmABS(x2-x1);
	ay = UUmABS(y2-y1);

	if (dx < 0) {
		xdir = -1;
	}
	else if (dx==0) {
		xdir = 0;
	}
	else {
		xdir = 1;
	}

	if (dy<0) {
		ydir = -1;
	}
	else if (dy==0) {
		ydir=0;
	}
	else {
		ydir = 1;
	}

	x = x1;
	y = y1;

	if (ax > ay)
	{
		e = (ay-ax)/2;
		while (1)
		{
			if (callback != NULL) {
				(*callback)(x, y, fill->weight);
			}

			if (!overwrite) PHrCheckAndPutCell(grid,(UUtUns16)height,(UUtUns16)width,x,y,fill);
			else PHrCheckAndBlastCell(grid,(UUtUns16)height,(UUtUns16)width,x,y,fill);

			if (x==x2) return;
			if (e>=0)
			{
				y+=ydir;
				e-=ax;
			}

			x+=xdir;
			e+=ay;
		}
	}
	else
	{
		e = (ax-ay)/2;
		while (1)
		{
			if (callback != NULL) {
				(*callback)(x, y, fill->weight);
			}

			if (!overwrite) PHrCheckAndPutCell(grid,(UUtUns16)height,(UUtUns16)width,x,y,fill);
			else PHrCheckAndBlastCell(grid,(UUtUns16)height,(UUtUns16)width,x,y,fill);

			if (y==y2) return;
			if (e>=0)
			{
				x+=xdir;
				e-=ay;
			}

			y+=ydir;
			e+=ax;
		}
	}
}


void PHrDynamicBresenham2(
	UUtInt16 x1,
	UUtInt16 y1,
	UUtInt16 x2,
	UUtInt16 y2,
	PHtDynamicSquare *grid,
	UUtUns32 inWidth,
	UUtUns32 inHeight,
	UUtUns8 inObstruction,
	UUtBool inOverwrite)
{
	/**************
	* Draws a bresenham line from 1 to 2 in the dynamic grid, assuming 'grid' is 'width'
	* cells wide. Note that this draws slightly thicker lines then a straight
	* Bresenham to prevent diagonal "leaks". The 'height' is only used for
	* input validation. Will not overwrite existing obstructions unless
	* inOverwrite is set.
	*/

	UUtInt32 width = inWidth;
	UUtInt32 height = inHeight;
	UUtInt16 ax,ay,dx,dy,x,y,e,xdir,ydir;

	UUmAssert(inWidth <= UUcMaxInt32);
	UUmAssert(inHeight <= UUcMaxInt32);

	dx = x2-x1;
	dy = y2-y1;
	ax = UUmABS(x2-x1);
	ay = UUmABS(y2-y1);

	if (dx < 0) {
		xdir = -1;
	}
	else if (dx==0) {
		xdir = 0;
	}
	else {
		xdir = 1;
	}

	if (dy<0) {
		ydir = -1;
	}
	else if (dy==0) {
		ydir=0;
	}
	else {
		ydir = 1;
	}

	x = x1;
	y = y1;

	if (ax > ay)
	{
		e = (ay-ax)/2;
		while (1)
		{
			if ((x >= 0) && (x < width) && (y >= 0) && (y < height)) {
				if (inOverwrite || (grid[x + y*width].obstruction == 0)) {
					grid[x + y*width].obstruction = inObstruction;
				}
			}

			if (x==x2) return;
			if (e>=0)
			{
				y+=ydir;
				e-=ax;
			}

			x+=xdir;
			e+=ay;
		}
	}
	else
	{
		e = (ax-ay)/2;
		while (1)
		{
			if ((x >= 0) && (x < width) && (y >= 0) && (y < height)) {
				if (inOverwrite || (grid[x + y*width].obstruction == 0)) {
					grid[x + y*width].obstruction = inObstruction;
				}
			}

			if (y==y2) return;
			if (e>=0)
			{
				x+=xdir;
				e-=ay;
			}

			y+=ydir;
			e+=ax;
		}
	}
}


// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================


void PHrGrid_Compress(PHtSquare *inSrc, UUtUns32 inLength, PHtSquare *outDst, UUtUns32 *outLength)
{
	UUtUns8 type_mask = 0x0f;
	PHtSquare *current = inSrc;
	PHtSquare *end = inSrc + inLength;
	PHtSquare *current_out = outDst;

	for(; current < end; current++)
	{
		UUtUns8 strip_type = current[0].weight & type_mask;
		UUtUns32 strip_length = 1;

		// scan for strip
		while(((current + 1) < end) && (current[1].weight == strip_type))
		{
			strip_length++;
			current++;

			if (UUcMaxUns8 == strip_length) {
				break;
			}
		}

		if (strip_length > 0x0f) {
			current_out[0].weight = (UUtUns8) (strip_type);
			current_out[1].weight = (UUtUns8) (strip_length);

			current_out += 2;
		}
		else {
			current_out[0].weight = (UUtUns8) ((strip_length << 4) | strip_type);
			current_out += 1;
		}
	}

	*outLength = current_out - outDst;

	return;
}

void PHrGrid_Decompress(PHtSquare *inSrc, UUtUns32 inLength, PHtSquare *outDst)
{
	PHtSquare *current = inSrc;
	PHtSquare *end = inSrc + inLength;
	PHtSquare *current_out = outDst;
	UUtUns8 rle_mask = 0xf0;
	UUtUns8 type_mask = 0x0f;

	for(; current < end; current++)
	{
		UUtUns8 type;
		UUtUns32 count;

		type = current[0].weight & type_mask;
		count = (current[0].weight & rle_mask) >> 4;

		if (0 == count) {
			count = current[1].weight;
			current += 1;
		}

		UUrMemory_Set8(current_out, type, count);
		current_out += count;
	}

	return;
}

