/*
	BFW_Path.h

	This file contains all pathfinding header stuff

	Author: Quinn Dunki, Michael Evans
	Copyright (c) 1998 Bungie
*/

#ifndef __BFW_PATH_H__
#define __BFW_PATH_H__

#include "BFW.h"
#include "BFW_Types.h"
#include "BFW_Motoko.h"

//#define DEBUG_PH
#ifdef DEBUG_PH
	extern UUtBool IMPgPlotted;
#endif

// Macros
#define PutCell(s,t,c) ((*((grid) + ((width)*(t))+(s))).weight = (c))
#define PutCellHeight(s,t,c) ((*((grid) + ((width)*(t))+(s))).height = (c))
#define GetCell(s,t) (*((grid) + ((width)*(t))+(s)))

#define PutObstruction(s,t,o) ((*((grid) + ((width)*(t))+(s))).obstruction = (o))
#define GetObstruction(s,t) (*((grid) + ((width)*(t))+(s))).obstruction

#ifdef DEBUG_PH
	#define DEBUG_PH_PLOT IMPgPlotted = UUcTrue
#else
	#define DEBUG_PH_PLOT
#endif



// constants
#define PHcWorldCoord_YOffset	0.5f	// added to worldspace coordinates so that if they are tested they are guaranteed
	                                    // not to lie below the room's origin
#define PHcFlatNormal			0.5f	// Y component of normal of "flatest" surface we can walk on
#define PHcSquareSize			4.0f	// Default size (in units) of 1 grid square
#define PHcMaxGridSize			200		// Maximum size of working grid (each dimension)
#define PHcComfortDistance		10.0f
#define PHcMaxHeight			127.0f	// Max value of a height map cell

#define PHcOverhangBit 0x80

// lower 4-bits are used for type
// higher 4-bits are used to encode runs up to 15 long
// if higher 4-bits are 0, then the next 2-bytes encodes a run

enum {
	PHcClear			= 0x00,
	PHcNearWall			= 0x01,		// used for anti-aliasing walls
	PHcBorder1			= 0x02,		// 1 = least dangerous, 4 = most dangerous
	PHcBorder2			= 0x03,
	PHcSemiPassable		= 0x04,		// used for anti-aliasing walls
	PHcBorder3			= 0x05,
	PHcBorder4			= 0x06,
	PHcStairs			= 0x07,
	PHcDanger   		= 0x08,
	PHcImpassable		= 0x09,
	PHcMax				= 0x0A
};

enum {
	PHcDebugEvent_None			= 0,
	PHcDebugEvent_Floor			= 1,
	PHcDebugEvent_Wall			= 2,
	PHcDebugEvent_SlopedQuad	= 3,
	PHcDebugEvent_DangerQuad	= 4,
	PHcDebugEvent_ImpassableQuad= 5,
	PHcDebugEvent_StairQuad		= 6,
	PHcDebugEvent_Max			= 7
};

extern const char PHcPrintChar[];
extern const float PHcPathWeight[];
extern const char *PHcWeightName[];
extern const char *PHcDebugEventName[];

typedef tm_struct PHtSquare
{
	UUtUns8 weight;	// in units
} PHtSquare;

typedef tm_struct PHtDynamicSquare
{
	UUtUns8 obstruction;	// index into the node's obstruction table, or 0 for none
} PHtDynamicSquare;

typedef tm_struct PHtDebugInfo
{
	UUtInt16 x;
	UUtInt16 y;
	UUtUns8 weight;
	UUtUns8 event;
	UUtUns32 gq_index;
} PHtDebugInfo;

typedef struct PHtGridPoint
{
	PHtSquare value;
	UUtInt16 x;
	UUtInt16 y;
} PHtGridPoint;

void PHrGrid_Compress(PHtSquare *inSrc, UUtUns32 inLength, PHtSquare *outDst, UUtUns32 *outLength);
void PHrGrid_Decompress(PHtSquare *inSrc, UUtUns32 inLength, PHtSquare *outDst);

typedef tm_struct PHtRoomData
{
	UUtUns32				gridX;			// Size of pathfinding grid
	UUtUns32				gridY;

	tm_raw(PHtSquare *)		compressed_grid;
	UUtUns32				compressed_grid_size;

	float					squareSize;		// Size (in AutoCad units) of one grid square

	M3tPoint3D				origin;			// Worldspace origin of the node
	M3tPoint3D				antiOrigin;		// Opposite corner of the worldspace origin (forms an axis bounding box)
	UUtInt16				gox;			// Offset origin for the grid. Will always be -IMPcGridBuffer
	UUtInt16				goy;
	UUtUns32 				parent;

	UUtUns32				debug_render_time;		// debugging only

	UUtUns32				debug_info_count;			// zero unless imported with -pathdebug
	tm_raw(PHtDebugInfo *)	debug_info;
} PHtRoomData;

void PHrGridToWorldSpace(UUtUns16 inGridX, UUtUns16 inGridY, const float *inAltitude, M3tPoint3D *outWorld, const PHtRoomData *inRoom);
void PHrWorldToGridSpace(UUtUns16 *outGridX, UUtUns16 *outGridY, const M3tPoint3D *inWorld, const PHtRoomData *inRoom);
void PHrWorldToGridSpaceDangerous(UUtInt16 *outGridX, UUtInt16 *outGridY, const M3tPoint3D *inWorld, const PHtRoomData *inRoom);

void PHrWorldPinToGridSpace(M3tPoint3D *inWorld, const PHtRoomData *inRoom);
UUtBool PHrWorldInGridSpace(const M3tPoint3D *inWorld, const PHtRoomData *inRoom);
void PHrWaypointFromGunk(AKtEnvironment *inEnv, AKtGQ_General *inQQGeneral, M3tPoint3D *outPoint, M3tPoint3D *inPoint);

typedef void (*PHtRasterizationCallback)(UUtInt16 inX, UUtInt16 inY, UUtUns8 inWeight);

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
	PHtRasterizationCallback callback);

void PHrDynamicBresenham2(
	UUtInt16 x1,
	UUtInt16 y1,
	UUtInt16 x2,
	UUtInt16 y2,
	PHtDynamicSquare *grid,
	UUtUns32 inWidth,
	UUtUns32 inHeight,
	UUtUns8 inObstruction,
	UUtBool inOverwrite);

void PHrDisplayGrid(
	PHtRoomData *room,
	UUtUns32 gq,
	UUtUns32 flags);

void PHrPutObstruction(
	PHtDynamicSquare *grid,
	UUtUns16 height,
	UUtUns16 width,
	UUtInt16 s,
	UUtInt16 t,
	UUtUns8 obstruction);

void PHrCheckAndPutCell(
	PHtSquare *grid,
	UUtUns16 height,
	UUtUns16 width,
	UUtInt16 s,
	UUtInt16 t,
	PHtSquare *c);

void PHrCheckAndBlastCell(
	PHtSquare *grid,
	UUtUns16 height,
	UUtUns16 width,
	UUtInt16 s,
	UUtInt16 t,
	PHtSquare *c);

UUtError PHrInitialize(void);
void PHrTerminate(void);

#endif
