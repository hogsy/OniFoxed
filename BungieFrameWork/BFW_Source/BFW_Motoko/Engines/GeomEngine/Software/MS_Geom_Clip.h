/*
	FILE:	MS_Geom_Clip.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: May 21, 1997
	
	PURPOSE: Interface to the Motoko 3D engine
	
	Copyright 1997


*/
#ifndef MS_CLIP_H
#define MS_CLIP_H

void
MSrClip_Line(
	UUtUns32		inVIndex0,
	UUtUns32		inVIndex1,
	UUtUns8			inClipCode0,
	UUtUns8			inClipCode1,
	UUtUns8			inClippedPlanes);	/* Set to 0 */

/*
 * clipping
 */
void
MSrClip_Triangle(
	UUtUns32		inVIndex0,
	UUtUns32		inVIndex1,
	UUtUns32		inVIndex2,
	UUtUns32		inClipCode0,
	UUtUns32		inClipCode1,
	UUtUns32		inClipCode2,
	UUtUns32		inClippedPlanes);	/* Set to 0 */

void
MSrClip_Quad(
	UUtUns32		inVIndex0,
	UUtUns32		inVIndex1,
	UUtUns32		inVIndex2,
	UUtUns32		inVIndex3,
	UUtUns32		inClipCode0,
	UUtUns32		inClipCode1,
	UUtUns32		inClipCode2,
	UUtUns32		inClipCode3,
	UUtUns32		inClippedPlanes);	/* Set to 0 */

void
MSrClip_ComputeVertex_TextureInterpolate(
	UUtUns32				inClipPlane,
	
	UUtUns32				inVIndexIn0,
	UUtUns32				inVIndexOut0,
	UUtUns32				inVIndexNew0,
	
	UUtUns32				inVIndexIn1,
	UUtUns32				inVIndexOut1,
	UUtUns32				inVIndexNew1,
	
	UUtUns8					*outClipCodeNew0,
	UUtUns8					*outClipCodeNew1);

void
MSrClip_ComputeVertex_TextureEnvInterpolate(
	UUtUns32				inClipPlane,
	
	UUtUns32				inVIndexIn0,
	UUtUns32				inVIndexOut0,
	UUtUns32				inVIndexNew0,
	
	UUtUns32				inVIndexIn1,
	UUtUns32				inVIndexOut1,
	UUtUns32				inVIndexNew1,
	
	UUtUns8					*outClipCodeNew0,
	UUtUns8					*outClipCodeNew1);

void
MSrClip_ComputeVertex_GouraudInterpolate(
	UUtUns32				inClipPlane,
	
	UUtUns32				inVIndexIn0,
	UUtUns32				inVIndexOut0,
	UUtUns32				inVIndexNew0,
	
	UUtUns32				inVIndexIn1,
	UUtUns32				inVIndexOut1,
	UUtUns32				inVIndexNew1,
	
	UUtUns8					*outClipCodeNew0,
	UUtUns8					*outClipCodeNew1);

void
MSrClip_ComputeVertex_TextureFlat(
	UUtUns32				inClipPlane,
	
	UUtUns32				inVIndexIn0,
	UUtUns32				inVIndexOut0,
	UUtUns32				inVIndexNew0,
	
	UUtUns32				inVIndexIn1,
	UUtUns32				inVIndexOut1,
	UUtUns32				inVIndexNew1,
	
	UUtUns8					*outClipCodeNew0,
	UUtUns8					*outClipCodeNew1);

void
MSrClip_ComputeVertex_GouraudFlat(
	UUtUns32				inClipPlane,
	
	UUtUns32				inVIndexIn0,
	UUtUns32				inVIndexOut0,
	UUtUns32				inVIndexNew0,
	
	UUtUns32				inVIndexIn1,
	UUtUns32				inVIndexOut1,
	UUtUns32				inVIndexNew1,
	
	UUtUns8					*outClipCodeNew0,
	UUtUns8					*outClipCodeNew1);

void
MSrClip_ComputeVertex_LineFlat(
	UUtUns32				inClipPlane,
	UUtUns32				inVIndexIn,
	UUtUns32				inVIndexOut,
	UUtUns32				inVIndexNew,
	UUtUns8					*outClipCodeNew);
	


#endif
