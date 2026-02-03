#pragma once
#ifndef ONI_AI2_LOCALPATH_H
#define ONI_AI2_LOCALPATH_H

/*
	FILE:	Oni_AI2_LocalPath.h

	AUTHOR:	Chris Butcher

	CREATED: May 08, 1999

	PURPOSE: Local-grid pathfinding for Oni AI

	Copyright (c) 2000

*/

// ------------------------------------------------------------------------------------
// -- global variables

#if TOOL_VERSION
// DEBUGGING
#define AI2_LOCALPATH_LINEMAX		60

extern UUtBool		AI2gDebugLocalPath_Stored;
extern UUtUns32		AI2gDebugLocalPath_LineCount;
extern M3tPoint3D	AI2gDebugLocalPath_Point;
extern UUtBool		AI2gDebugLocalPath_Success[AI2_LOCALPATH_LINEMAX];
extern M3tPoint3D	AI2gDebugLocalPath_EndPoint[AI2_LOCALPATH_LINEMAX];
extern UUtUns8		AI2gDebugLocalPath_Weight[AI2_LOCALPATH_LINEMAX];
#endif

// ------------------------------------------------------------------------------------
// -- function prototypes

// set up for local path finding
UUtError AI2rLocalPath_Initialize(void);

// dispose local path finding data structures
void AI2rLocalPath_Terminate(void);

// find a path through the local environment, in a specified direction
UUtError AI2rFindLocalPath(ONtCharacter *ioCharacter, PHtPathMode inPathMode, M3tPoint3D *inStartPoint,
						   AKtBNVNode *inStartNode, PHtNode *inStartPathNode,
						   float inDistance, float inDirection, float inSideVary,
						   UUtUns32 inSideAttempts, UUtUns8 inMaxWeight, UUtUns8 inEscapeDistance,
						   UUtBool *outSuccess, M3tPoint3D *outPoint, UUtUns8 *outWeight, UUtBool *outEscape);

// find a point that is close to another point and can reach it through the local environment, in a specified direction
UUtError AI2rFindNearbyPoint(ONtCharacter *ioCharacter, PHtPathMode inPathMode, M3tPoint3D *inStartPoint, float inDistance, float inMinDistance,
							 float inDirection, float inSideVary, UUtUns32 inSideAttempts, UUtUns8 inMaxWeight, UUtUns8 inEscapeDistance,
							 UUtBool *outSuccess, M3tPoint3D *outPoint, UUtUns8 *outWeight, UUtBool *outEscape);

// check to see whether a specified path through the local environment is viable
UUtError AI2rTryLocalMovement(ONtCharacter *ioCharacter, PHtPathMode inPathMode, M3tPoint3D *inStart, M3tPoint3D *inEnd, UUtUns8 inEscapeDistance,
							   UUtBool *outMoveOK, M3tPoint3D *outStopPoint, UUtUns8 *outWeight, UUtBool *outEscape);

// debugging - cast a halo of lines into the local environment
void AI2rLocalPath_DebugLines(ONtCharacter *inCharacter);

#endif // ONI_AI2_LOCALPATH_H
