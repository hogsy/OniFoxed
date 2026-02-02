/*
	FILE:	EM_DC_Method_Frame.h

	AUTHOR:	Brent H. Pease

	CREATED: Sept 13, 1997

	PURPOSE:

	Copyright 1997

*/
#ifndef EM_DC_METHOD_FRAME_H
#define EM_DC_METHOD_FRAME_H

UUtError
EDrDrawContext_Method_Frame_Start(
	UUtUns32			inTime);

UUtError
EDrDrawContext_Method_Frame_End(
	void);

UUtError
EDrDrawContext_Method_Frame_Sync(
	void);

#endif /* EM_DC_METHOD_FRAME_H */
