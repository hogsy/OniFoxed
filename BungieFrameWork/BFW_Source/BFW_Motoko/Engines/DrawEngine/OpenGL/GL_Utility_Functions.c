/*
	FILE:	GL_DC_Method_Frame.h
	
	AUTHOR:	Michael Evans
	
	CREATED: August 10, 1999
	
	PURPOSE: 
	
	Copyright 1999

*/

#ifndef __ONADA__

#include "BFW_Motoko.h"
#include "BFW_Image.h"
#include "GL_Utility_Functions.h"
#include "GL_DC_Private.h"
#include "GL_Platform.h"
#include "OGL_DrawGeom_Common.h"

UUtError
GLrDrawContext_Method_Frame_Start(
	UUtUns32			inGameTime)
{
	if (GLgDoubleBuffer) {
		glDrawBuffer(GL_BACK);
	}
	else {
		glDrawBuffer(GL_FRONT);
	}

	if (GLgBufferClear)
	{
		/*static float one_over_255= 1.f/255.f;

		glClearColor(((GLgClearColor>>16)&0xFF)*one_over_255,
			((GLgClearColor>>8)&0xFF)*one_over_255,
			(GLgClearColor&0xFF)*one_over_255,
			0.f);*/
		glClearColor(OGLgCommon.fog_color[0], OGLgCommon.fog_color[1],
			OGLgCommon.fog_color[2], OGLgCommon.fog_color[3]);

		#if 1
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		#else
		if (GLgDrawContextPrivate->depth_write) {
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
		else {
			glDepthMask(GL_TRUE);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDepthMask(GL_FALSE);
		}
		#endif
	}

	// update fog params
	OGLrCommon_glFogEnable();
	
	return UUcError_None;
}
	
// ----------------------------------------------------------------------
UUtError
GLrDrawContext_Method_Frame_End(
	UUtUns32	*outTextureBytesDownloaded)
{
	OGLrCommon_glFlush();

	*outTextureBytesDownloaded = 0;

	if (GLgDoubleBuffer) {
		GLrPlatform_DisplayBackBuffer();
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
GLrDrawContext_Method_Frame_Sync(
	void)
{
	return UUcError_None;
}



#endif // __ONADA__
