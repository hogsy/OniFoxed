// ======================================================================
// Oni_AStar.h
// ======================================================================
#ifndef ONI_ASTAR_H
#define ONI_ASTAR_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Path.h"
#include "BFW_Akira.h"
#include "Oni_GameState.h"
#include "Oni_Path.h"

// ======================================================================
// typedef
// ======================================================================
typedef struct AStPath		AStPath;
typedef struct AStPathNode	AStPathNode;

typedef enum {
	AScPathPoint_Clear = 0,		// not near danger, in a clear area
	AScPathPoint_Tight = 1,		// not near danger, in a tight area
	AScPathPoint_Danger = 2,	// danger nearby
	AScPathPoint_Impassable = 3	// this point is nominally impassable, we might not be able
								// to get there
} AStPathPointType;

typedef struct AStPathPoint {
	M3tPoint3D			point;
	AStPathPointType		type;
} AStPathPoint;

// ======================================================================
// globals
// ======================================================================
extern AStPath				*ASgAstar_Path;

// ======================================================================
// prototypes
// ======================================================================
UUtError
ASrInitialize(
	void);
	
void
ASrTerminate(
	void);
	
// ----------------------------------------------------------------------
// allocates a data for a new path.  You really only need one of these
// per game.  Use ASrPath_SetParams when a room has changed
// ----------------------------------------------------------------------
AStPath*
ASrPath_New(
	void);

// ----------------------------------------------------------------------
// deletes the path and any data it is using
// ----------------------------------------------------------------------
void
ASrPath_Delete(
	AStPath			**ioAstar);

// ----------------------------------------------------------------------
// finds a path and generates a series of waypoints at the turn points
// of the path.
// asssumes outWaypoints points to an array with AIcMax_Waypoints
// ----------------------------------------------------------------------
UUtBool
ASrPath_Generate(
	AStPath					*ioAstar,
	UUtUns16				inMaxWaypoints,
	UUtUns16				*outNumWaypoints,
	AStPathPoint			*outWaypoints,
	ONtCharacter			*inWalker,
	UUtBool					inAllowDiagonals,
	UUtBool					inCarefulPath);

// ----------------------------------------------------------------------
// sets the parameters that ASrPath_Generate will use
// ----------------------------------------------------------------------
UUtBool
ASrPath_SetParams(
	AStPath					*ioAstar,
	PHtNode					*inNode,
	const PHtRoomData		*inRoom,
	const M3tPoint3D		*inStart,
	const M3tPoint3D		*inEnd,
	ONtCharacter			*inCharacter,
	UUtBool					inAllowStairs,
	UUtBool					inShowDebug,
	UUtBool					inShowEvaluation);
	
// Misc

UUtError ASrGetWorldGridValue(
	M3tPoint3D *inPoint,
	ONtCharacter *inViewer,
	PHtSquare *outValue);

// ======================================================================

const PHtRoomData *ASrGetCurrentRoom(void);
void ASrGetLocalStart(IMtPoint2D *outPoint);
void ASrGetLocalEnd(IMtPoint2D *outPoint);
void ASrDisplayWorkingGrid(void);

void ASrDisplayDebuggingInfo(void);

// CB: just for debugging access to variables
void ASrDebugGetGridPoints(AStPath *inPath, IMtPoint2D *outStart, IMtPoint2D *outEnd);

#endif /* ONI_ASTAR_H */