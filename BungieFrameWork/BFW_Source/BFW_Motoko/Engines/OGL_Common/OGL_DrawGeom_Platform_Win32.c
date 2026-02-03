/*
	FILE:	OGL_DrawGeom_Platform_Win32.c

	AUTHOR:	Brent Pease, Kevin Armstrong, Michael Evans

	CREATED: January 5, 1998

	PURPOSE:

	Copyright 1997 - 1998

*/

#ifndef __ONADA__

#include "BFW.h"

void *
OGLrCommon_Platform_GetFunction(
	const char *inString)
{
	PROC function;

	function = wglGetProcAddress(inString);

	return function;
}



#endif // __ONADA__
