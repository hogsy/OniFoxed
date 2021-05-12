/*
	FILE:	MG_DC_Method_Frame.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 13, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef MG_DC_METHOD_FRAME_H
#define MG_DC_METHOD_FRAME_H

UUtError
MGrDrawContext_Method_Frame_Start(
	UUtUns32			inGameTime);

UUtError
MGrDrawContext_Method_Frame_End(
	UUtUns32	*outTextureBytesDownloaded);

UUtError
MGrDrawContext_Method_Frame_Sync(
	void);


#endif /* MG_DC_METHOD_FRAME_H */
