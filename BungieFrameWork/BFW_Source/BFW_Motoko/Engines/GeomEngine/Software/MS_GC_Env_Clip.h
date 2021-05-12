/*
	FILE:	MS_GC_Env_Clip.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Jan 14, 1998
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef MS_GC_ENV_CLIP_H
#define MS_GC_ENV_CLIP_H

void MSrEnv_Clip_Tri(
	M3tTriSplit*	inTriSplit,
	UUtUns8			inClipCode0,
	UUtUns8			inClipCode1,
	UUtUns8			inClipCode2,
	UUtUns8			inClippedPlanes);

void
MSrEnv_Clip_Quad(
	M3tQuadSplit*	inQuadSplit,
	UUtUns8			inClipCode0,
	UUtUns8			inClipCode1,
	UUtUns8			inClipCode2,
	UUtUns8			inClipCode3,
	UUtUns8			inClippedPlanes);

#endif /* MS_GC_ENV_CLIP_H */
