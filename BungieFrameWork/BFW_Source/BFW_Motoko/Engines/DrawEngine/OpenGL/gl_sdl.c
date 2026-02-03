/*
	gl_mswindows.c
	Friday Aug. 11, 2000 5:32 pm
	Stefan
*/

#ifdef __ONADA__

/*---------- includes */

#include "gl_engine.h"
#include "Oni_Platform.h"
#include "Oni_Persistance.h"

#include "SDL2/SDL_pixels.h"

void M3rSetGamma(
	float inGamma)
{
	Uint16 ramp[256];
	SDL_CalculateGammaRamp(inGamma + 0.5f, ramp);
	SDL_SetWindowGammaRamp(ONgPlatformData.gameWindow, ramp, ramp, ramp);

	return;
}

static boolean set_display_settings(
	int width,
	int height,
	int depth)
{
	UUtBool success= UUcFalse;
	SDL_DisplayMode desired_display_mode, current_display_mode;

	//FIXME: assumed
	UUmAssert(depth == 16 || depth == 32);
	success= SDL_GetWindowDisplayMode(ONgPlatformData.gameWindow, &current_display_mode) == 0;
	UUmAssert(success);

	// if Motoko asks for a bit depth other than what the PC is currently set to,
	// ALOT of PC video cards punt. So, we have to catch that here.
	if (success && (SDL_BITSPERPIXEL(current_display_mode.format) < depth))
	{
		depth= gl->display_mode.bitDepth= (short)SDL_BITSPERPIXEL(current_display_mode.format);
	}

	desired_display_mode.format = depth == 16 ? SDL_PIXELFORMAT_RGB565 : SDL_PIXELFORMAT_RGB888;
	desired_display_mode.w= width;
	desired_display_mode.h= height;
	desired_display_mode.refresh_rate= 0;
	desired_display_mode.driverdata= 0;

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24); //FIXME
	SDL_SetWindowSize(ONgPlatformData.gameWindow, width, height);
	success = UUcTrue;

	if(success)
	{
		Uint32 fullscreenFlags;
		if (M3gResolutionSwitch)
		{
			fullscreenFlags = SDL_WINDOW_FULLSCREEN;
		}
		else
		{
			fullscreenFlags = 0;
		}

		success = SDL_SetWindowFullscreen(ONgPlatformData.gameWindow, fullscreenFlags) == 0;
	}

	return success;
}

/*
void make_pixel_format_descriptor(
	PIXELFORMATDESCRIPTOR *pfd,
	word bit_depth)
{
	UUmAssert(pfd);
	memset(pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));

	pfd->nSize= sizeof(PIXELFORMATDESCRIPTOR);
	pfd->nVersion= 1;
	pfd->dwFlags= PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;

	pfd->iPixelType= PFD_TYPE_RGBA;

	pfd->dwLayerMask= PFD_MAIN_PLANE;

	switch (bit_depth)
	{
		case 16:
			pfd->cColorBits= 16;
			pfd->cDepthBits= 16;
			break;
		case 32:
			pfd->cColorBits= 24;
			pfd->cDepthBits= 32;
			break;
		default:
			pfd->cColorBits= 16;
			pfd->cDepthBits= 16;
			break;
	}

	return;
}

boolean gl_pixel_format_is_accelerated(
	PIXELFORMATDESCRIPTOR *pfd)
{
	boolean accelerated= TRUE;
	int generic_format, generic_accelerated;

	UUmAssert(pfd);

	generic_format= pfd->dwFlags & PFD_GENERIC_FORMAT;
	generic_accelerated= pfd->dwFlags & PFD_GENERIC_ACCELERATED;

	if (generic_format && ! generic_accelerated)
	{
		// software
		accelerated= FALSE;
	}
	else if (generic_format && generic_accelerated)
	{
		// hardware - MCD
	}
	else if (!generic_format && !generic_accelerated)
	{
		// hardware - ICD
	}
	else
	{
		accelerated= FALSE; // ?
	}

	return accelerated;
}
*/

boolean gl_create_render_context(
	void)
{
	/*
	boolean success= FALSE;

				success= gl_pixel_format_is_accelerated(&pfd);
				if (success)
				{
					UUrStartupMessage("opengl color bits = %d", pfd.cColorBits);
					UUrStartupMessage("opengl depth bits = %d", pfd.cDepthBits);
					if (!gl->render_context)
					{
						gl->render_context= WGL_FXN(wglCreateContext)(gl->device_context);
						if (!gl->render_context)
						{
							DWORD hr;
							char string[256]= "";

							hr= FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
								string, 256, NULL);
							UUrDebuggerMessage("wglCreateContext() failed : %s", string);
						}
					}
					success= ((gl->render_context != NULL) &&
						WGL_FXN(wglMakeCurrent)(gl->device_context, gl->render_context));
				}
			}
			else
			{
				DWORD hr;
				char string[256]= "";

				hr= FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					string, 256, NULL);
				UUrDebuggerMessage("SetPixelFormat() failed : %s", string);
			}
		}
	}

	return success;
*/
	UUtBool success;
	if (!gl->context)
	{
		gl->context= SDL_GL_CreateContext(ONgPlatformData.gameWindow);
		if (!gl->context)
		{
			UUrDebuggerMessage("wglCreateContext() failed : %s", SDL_GetError());
		}
	}
	success= ((gl->context != NULL) &&
		SDL_GL_MakeCurrent(ONgPlatformData.gameWindow, gl->context) == 0);
	return success;
}

//#define DESTROY_CONTEXTS_ON_DISPLAY_CHANGE we will just crash on cards that don't work with this
// WARNING! This call is also used for resolution switching!!!
UUtBool gl_platform_initialize(
	void)
{
	UUtBool success= UUcTrue;
	static int current_width= 0;
	static int current_height= 0;
	static int current_depth= 0;
	int width= gl->display_mode.width;
	int height= gl->display_mode.height;
	int depth= gl->display_mode.bitDepth;

	if ((width != current_width) || (height != current_height) || (depth != current_depth))
	{
	#ifdef DESTROY_CONTEXTS_ON_DISPLAY_CHANGE // textures get mangled, but doesn't crash the game
		// tear down current contexts & rebuild everything
		success= SDL_GL_MakeCurrent(NULL, NULL) == 0;
		if (!success)
		{
			UUrDebuggerMessage("SDL_GL_MakeCurrent() failed : %s", SDL_GetError());
		}
		UUmAssert(gl->context);
		SDL_GL_DeleteContext(gl->context) == 0;
		gl->context= NULL;
	#endif
	}

	if ((width != current_width) || (height != current_height) || (depth != current_depth))
	{
		success= set_display_settings(width, height, depth);
	}

	if (success == FALSE)
	{
		if (gl->display_mode.bitDepth != 16 || (width != 640) || (height != 480))
		{
			width= 640;
			height= 480;
			depth= 16;
			success= set_display_settings(width, height, depth);
			if (success)
			{
				// sure the higher level code thinks the bit depth is different, but who cares? not me!
				gl->display_mode.bitDepth= depth;
			}
		}
	}

	if (success)
	{
		M3rSetGamma(ONrPersist_GetGamma());

			success= gl_create_render_context();
			if (!success)
			{
				if (depth != 16)
				{
					// try again at 16-bit
					success= set_display_settings(width, height, 16);
					if (success)
					{
						gl->display_mode.bitDepth= depth= 16;
						success= gl_create_render_context();
					}
				}
			}
	}
	if (!success) // the app doesn't handle failure so we do it here
	{
		AUrMessageBox(AUcMBType_OK, "Failed to initialize OpenGL contexts; Oni will now exit.");
		exit(0);
	}
	else
	{
		current_width= gl->display_mode.width;
		current_height= gl->display_mode.height;
		current_depth= gl->display_mode.bitDepth;
	}

	return success;
}

void gl_platform_dispose(
	void)
{
	int screen_width, screen_height;
	UUtBool success;

	success= SDL_GL_MakeCurrent(NULL, NULL) == 0;
	UUmAssert(success);

	UUmAssert(gl->context);
	SDL_GL_DeleteContext(gl->context);

	UUmAssert(ONgPlatformData.gameWindow);

	return;
}


void gl_display_back_buffer(
	void)
{
	SDL_GL_SwapWindow(ONgPlatformData.gameWindow);

	return;
}

void gl_matrox_display_back_buffer(
	void)
{
	gl_display_back_buffer();
}

// fills in an array of display modes; returns number of elements in array
// array MUST BE AT LEAST M3cMaxDisplayModes elements in size!!!
int gl_enumerate_valid_display_modes(
	M3tDisplayMode display_mode_list[M3cMaxDisplayModes])
{
	M3tDisplayMode desired_display_mode_list[]= {
			{640, 480, 16, 0},
			{800, 600, 16, 0},
			{1024, 768, 16, 0},
			{1152, 864,	16, 0},
			//{1280, 1024, 16, 0},
			{1600, 1200, 16, 0},
			{1920, 1080, 16, 0},
			//{1920, 1200, 16, 0},
			{640, 480, 32, 0},
			{800, 600, 32, 0},
			{1024, 768, 32, 0},
			{1152, 864, 32, 0},
			//{1280, 1024, 32, 0},
			{1600, 1200, 32, 0},
			{1920, 1080, 32, 0}
			/*{1920, 1200, 32, 0}*/};
	int n= sizeof(desired_display_mode_list)/sizeof(desired_display_mode_list[0]);
	int i, num_valid_display_modes= 0;
	//SDL_DisplayMode mode;
	//int modes= SDL_GetNumDisplayModes();
	//SDL_GetDisplayMode(0, n, &mode);

	for (i=0; i<n && i<M3cMaxDisplayModes; i++)
	{
		SDL_DisplayMode desired_display_mode;

		//TODO: filter available modes
		display_mode_list[num_valid_display_modes]= desired_display_mode_list[i];
		++num_valid_display_modes;
	}

	return num_valid_display_modes;
}

#endif
