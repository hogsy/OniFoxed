/*
	gl_code_version.h
	Friday Aug. 11, 2000 6:16 pm
	Stefan
*/

#ifndef __GL_CODE_VERSION__
#define __GL_CODE_VERSION__

// this file must be #include'ed by the project
// this allows switching between the old OpenGL rasterizer and Stefan's OpenGL rasterizer
// define __ONADA__ to use the new code

#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Win32)
#define __ONADA__
#elif defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Mac)
#define __ONADA__
#else
#error "platform not defined"
#endif


#endif // __GL_CODE_VERSION__
