/*
	FILE:	GL_DC_Method_Frame.h

	AUTHOR:	Michael Evans

	CREATED: August 10, 1999

	PURPOSE:

	Copyright 1999

*/

#include "BFW.h"

#ifndef GL_UTILITY_FUNCTIONS_H
#define GL_UTILITY_FUNCTIONS_H

UUtError
GLrDrawContext_Method_Frame_Start(
	UUtUns32			inGameTime);

UUtError
GLrDrawContext_Method_Frame_End(
	UUtUns32	*outTextureBytesDownloaded);

UUtError
GLrDrawContext_Method_Frame_Sync(
	void);

#endif /* GL_UTILITY_FUNCTIONS_H */
