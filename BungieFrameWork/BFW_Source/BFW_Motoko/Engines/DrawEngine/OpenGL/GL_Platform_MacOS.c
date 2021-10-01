
// ======================================================================
// GL_Platform_MacOS.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "GL_Platform.h"
#include "GL_DC_Private.h"
#include "Oni_Platform.h"

#include <DrawSprocket.h>

// ======================================================================
// globals
// ======================================================================
static GLint			GLgPixelAttributes_FullScreen[] =
	{ AGL_ALL_RENDERERS, AGL_FULLSCREEN, AGL_RGBA, AGL_DOUBLEBUFFER, AGL_DEPTH_SIZE, 32, AGL_NONE };

static GLint			GLgPixelAttributes_Window_32[] =
	{ AGL_ALL_RENDERERS, AGL_RGBA, AGL_DOUBLEBUFFER, AGL_DEPTH_SIZE, 32, AGL_NONE };

static GLint			GLgPixelAttributes_Window_16[] =
	{ AGL_ALL_RENDERERS, AGL_RGBA, AGL_DOUBLEBUFFER, AGL_DEPTH_SIZE, 16, AGL_NONE };

static AGLContext		GLgContext;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
UUtError
GLrPlatform_Initialize(
	void)
{
	AGLPixelFormat			pixel_format;
	GLboolean				result;
	long pixelsize;
/*	DSpContextReference		dsp_context;
	OSStatus				status;
	DisplayIDType			displayID;
	GDHandle				device;*/
	
	UUmAssert(ONgPlatformData.gameWindow);
	
	// get the device that the draw sprocket context is displayed on
/*	dsp_context = (DSpContextReference)GetWRefCon(ONgPlatformData.gameWindow);
	if (dsp_context == NULL) return UUcError_Generic;
	
	status = DSpContext_GetDisplayID(dsp_context, &displayID);
	if (status != noErr) return UUcError_Generic;
	
	status = DMGetGDeviceByDisplayID(displayID, &device, UUcFalse);
	if (status != noErr) return UUcError_Generic;
	
	// choose a pixel format
	pixel_format = aglChoosePixelFormat(&device, 1, GLgPixelAttributes_FullScreen);
	if (pixel_format == NULL)
	{
		pixel_format = aglChoosePixelFormat(&device, 1, GLgPixelAttributes_Window);
		if (pixel_format == NULL) return UUcError_Generic;
	}*/
	
	// choose a pixel format
//	pixel_format = aglChoosePixelFormat(NULL, 0, GLgPixelAttributes_FullScreen);
//	if (pixel_format == NULL)
	{
		pixel_format = aglChoosePixelFormat(NULL, 0, GLgPixelAttributes_Window_32);
		if (pixel_format == NULL)
		{
			pixel_format = aglChoosePixelFormat(NULL, 0, GLgPixelAttributes_Window_16);
			if (pixel_format == NULL) { return UUcError_Generic; }
		}
	}
	
	// create an AGL context
	GLgContext = aglCreateContext(pixel_format, NULL);
	if (GLgContext == NULL) return UUcError_Generic;
	
	// attach the window to the context
	result = aglSetDrawable(GLgContext, ONgPlatformData.gameWindow);
	if (result == GL_FALSE) return UUcError_Generic;
	
	// set the current context
	result = aglSetCurrentContext(GLgContext);
	if (result == GL_FALSE) return UUcError_Generic;
	
	// destroy the pixel format
	aglDestroyPixelFormat(pixel_format);
	
	/* Find the depth of the main screen */
	pixelsize = (*(*GetMainDevice())->gdPMap)->pixelSize;
	if (32 != pixelsize) exit(1);

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
GLrPlatform_Dispose(
	void)
{
	if (GLgContext)
	{
		aglSetCurrentContext(NULL);
		aglSetDrawable(GLgContext, NULL);
		aglDestroyContext(GLgContext);
		GLgContext = NULL;
	}
}

// ----------------------------------------------------------------------
void
GLrPlatform_DisplayBackBuffer(
	void)
{
	UUmAssert(GLgContext);
	aglSwapBuffers(GLgContext);
}


