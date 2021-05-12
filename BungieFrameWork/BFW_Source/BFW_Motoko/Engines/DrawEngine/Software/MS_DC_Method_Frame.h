/*
	FILE:	MS_DC_Method_Frame.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 13, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef MS_DC_METHOD_FRAME_H
#define MS_DC_METHOD_FRAME_H

UUtError
MSrDrawContext_Method_Frame_Start(
	M3tDrawContext		*inDrawContext);

UUtError
MSrDrawContext_Method_Frame_End(
	M3tDrawContext		*inDrawContext);

UUtError
MSrDrawContext_Method_Frame_Sync(
	M3tDrawContext		*inDrawContext);


#endif /* MS_DC_METHOD_FRAME_H */
