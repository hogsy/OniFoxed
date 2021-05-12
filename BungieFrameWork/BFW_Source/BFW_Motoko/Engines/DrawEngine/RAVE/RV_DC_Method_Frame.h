/*
	FILE:	RV_DC_Method_Frame.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 13, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef RV_DC_METHOD_FRAME_H
#define RV_DC_METHOD_FRAME_H

UUtError
RVrDrawContext_Method_Frame_Start(
	UUtUns32			inTime);

UUtError
RVrDrawContext_Method_Frame_End(
	UUtUns32	*outTextureBytesDownloaded);

UUtError
RVrDrawContext_Method_Frame_Sync(
	void);

#endif /* MG_DC_METHOD_FRAME_H */
