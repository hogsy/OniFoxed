/*
	FILE:	MS_GC_Method_Frame.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 19, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef MS_GC_METHOD_FRAME_H
#define MS_GC_METHOD_FRAME_H

UUtError
MSrGeomContext_Method_Frame_Start(
	UUtUns32			inGameTicksElapsed);

UUtError
MSrGeomContext_Method_Frame_End(
	void);

UUtError
MSrGeomContext_Method_Frame_Sync(
	void);

#endif /* MS_GC_METHOD_FRAME_H */
