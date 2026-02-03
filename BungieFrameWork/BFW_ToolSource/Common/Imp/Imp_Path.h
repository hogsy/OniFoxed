/*
	Imp_Path.h

	This file contains all pathfinding header stuff

	Author: Quinn Dunki
	c1998 Bungie
*/

#include "Oni_Path.h"

void PHrChooseAndWeightQuad(
	IMPtEnv_BuildData *inBuildData,
	UUtUns32 gqIndex,
	AKtEnvironment *env,
	PHtRoomData *room);

void PHrFatDot(
	UUtUns16 x,
	UUtUns16 y,
	PHtSquare *grid,
	UUtInt32 inWidth,
	UUtInt32 inHeight,
	PHtRasterizationCallback callback);

void PHrRasterizeSlopedQuad(
	IMPtEnv_GQ *gunk,
	AKtEnvironment *env,
	PHtRoomData *room);
