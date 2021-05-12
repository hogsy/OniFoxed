/*
	FILE:	EM_GC_Method_Frame.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: June 10, 1999
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef EM_GC_METHOD_FRAME_H
#define EM_GC_METHOD_FRAME_H

UUtError
EGrGeomContext_Method_Frame_Start(
	UUtUns32			inGameTime);

UUtError
EGrGeomContext_Method_Frame_End(
	void);

UUtError
EGrGeomContext_Method_Frame_Sync(
	void);

#endif /* EM_GC_METHOD_FRAME_H */
