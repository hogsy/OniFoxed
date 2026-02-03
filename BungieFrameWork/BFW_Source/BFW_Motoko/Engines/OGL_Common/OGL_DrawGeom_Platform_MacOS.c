
/*
	FILE:	OGL_DrawGeom_Platform_MacOS.c

	AUTHOR:	Brent Pease, Kevin Armstrong, Michael Evans

	CREATED: January 5, 1998

	PURPOSE:

	Copyright 1997 - 1998

*/

#include "BFW.h"
#include "BFW_Motoko.h"

#include "OGL_DrawGeom_Platform.h"

void *
OGLrCommon_Platform_GetFunction(
	const char *inString)
{
	void					*function;

	function = NULL;

	if (strcmp(inString, "glLockArraysEXT") == 0)
	{
		function = glLockArraysEXT;
	}
	else if (strcmp(inString, "glUnlockArraysEXT") == 0)
	{
		function = glUnlockArraysEXT;
	}
	else if (strcmp(inString, "glActiveTextureARB") == 0)
	{
		function = glActiveTextureARB;
	}
	else if (strcmp(inString, "glClientActiveTextureARB") == 0)
	{
		function = glClientActiveTextureARB;
	}
	else if (strcmp(inString, "glMultiTexCoord4fARB") == 0)
	{
		function = glMultiTexCoord4fARB;
	}
	else if (strcmp(inString, "glMultiTexCoord2fvARB") == 0)
	{
		function = glMultiTexCoord2fvARB;
	}
	else
	{
		UUmAssert(!"Missing something here");
	}

	return function;
}

