/*
	FILE:	OG_GC_Method_Frame.h

	AUTHOR:	Brent H. Pease

	CREATED: June 10, 1999

	PURPOSE:

	Copyright 1997

*/
#ifndef OG_GC_METHOD_FRAME_H
#define OG_GC_METHOD_FRAME_H

UUtError
OGrGeomContext_Method_Frame_Start(
	UUtUns32			inGameTime);

UUtError
OGrGeomContext_Method_Frame_End(
	void);

UUtError
OGrGeomContext_Method_Frame_Sync(
	void);

#endif /* OG_GC_METHOD_FRAME_H */
