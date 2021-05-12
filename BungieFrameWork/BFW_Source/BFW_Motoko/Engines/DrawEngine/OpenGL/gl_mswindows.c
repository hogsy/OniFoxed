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

// allow file to be included in macintosh version of project
#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Win32)

/*---------- prototypes */

static boolean fullscreen_candidate(HWND hwnd);
static void restore_display_settings(void);
void make_pixel_format_descriptor(PIXELFORMATDESCRIPTOR *pfd, word bit_depth);
static void gl_unload_opengl_dll(void);

/*---------- globals */

static DEVMODE original_display_mode= {0};
static WORD original_gamma_ramp[256 * 3]= {0};
static BOOL gamma_ramp_supported= FALSE;

/*---------- code */

void M3rSetGamma(
	float inGamma)
{
	float power= 0.4f + (1.2f * (1.f - inGamma));

	UUmAssert(sizeof(WORD) == sizeof(UUtUns16));
	
	if (gamma_ramp_supported)
	{
		UUtInt32 itr;
		WORD new_gamma_ramp[256 * 3];
		
		for (itr= 0; itr<(256*3); itr++)
		{
			// WORD src_gamma = (itr % 256) << 8 | (itr % 256);
			WORD src_gamma= original_gamma_ramp[itr];
			float value= (src_gamma) / ((float) 0xFFFF);

			value= (float)pow(value, power);
			new_gamma_ramp[itr]= (WORD)UUmPin(MUrFloat_Round_To_Int(value * ((float) 0xFFFF)), 0, 0xFFFF);
		}
		// try the 3Dfx gamma control extensions first
		if (GL_EXT(wglSetDeviceGammaRamp3DFX) != NULL)
		{
			GL_EXT(wglSetDeviceGammaRamp3DFX)(gl->device_context, new_gamma_ramp);
		}
		else
		{
			SetDeviceGammaRamp(gl->device_context, new_gamma_ramp);
		}
	}

	return;
}

static void RestoreGammaTable(
	void)
{
	if (gamma_ramp_supported)
	{
		// try the 3Dfx gamma control extensions first
		if (GL_EXT(wglSetDeviceGammaRamp3DFX) != NULL)
		{
			GL_EXT(wglSetDeviceGammaRamp3DFX)(gl->device_context, original_gamma_ramp);
		}
		else
		{
			SetDeviceGammaRamp(gl->device_context, original_gamma_ramp);
		}
	}

	return;
}

static void SetupGammaTable(
	void)
{
	if (gl->device_context)
	{
		// try the 3Dfx gamma control extensions first
		if (GL_EXT(wglGetDeviceGammaRamp3DFX) != NULL)
		{
			UUrStartupMessage("Using 3DFX gamma adjustment");
			gamma_ramp_supported= GL_EXT(wglGetDeviceGammaRamp3DFX)(gl->device_context, original_gamma_ramp);
		}
		else
		{
			UUrStartupMessage("Using standard Windows gamma adjustment");
			gamma_ramp_supported= GetDeviceGammaRamp(gl->device_context, original_gamma_ramp);
		}

		if (gamma_ramp_supported)
		{
			M3rSetGamma(ONrPersist_GetGamma());
		}
		else
		{
			UUrStartupMessage("gamma adjustment not supported");
		}
	}

	return;
}

static boolean fullscreen_candidate(
	HWND hwnd)
{
	RECT win_rect;
	RECT dt_rect;
	boolean success= FALSE;
    DWORD style= GetWindowLong (hwnd, GWL_STYLE);

	if ((style & WS_POPUP) && !(style & (WS_BORDER | WS_THICKFRAME)))
	{
		GetClientRect( hwnd, &win_rect );
		GetWindowRect(GetDesktopWindow(), &dt_rect);
		if (!memcmp(&win_rect, &dt_rect, sizeof(dt_rect)))
		{
			success= TRUE;
		}
	}

	return success;
}

static boolean set_display_settings(
	word width,
	word height,
	word depth)
{
	boolean success= FALSE;
	DEVMODE desired_display_mode= {0}, current_display_mode= {0};

	UUmAssert(depth >= 16);
	current_display_mode.dmSize= sizeof(DEVMODE);
	success= EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &current_display_mode);
	UUmAssert(success);
	// avoid overwriting the original display settings
	if (original_display_mode.dmSize == 0)
	{
		original_display_mode= current_display_mode;
	}

	// if Motoko asks for a bit depth other than what the PC is currently set to,
	// ALOT of PC video cards punt. So, we have to catch that here.
	if (success && (current_display_mode.dmBitsPerPel < depth))
	{
		depth= gl->bit_depth= (short)current_display_mode.dmBitsPerPel;
	}

	desired_display_mode.dmSize= sizeof(DEVMODE);
    desired_display_mode.dmBitsPerPel= depth;
    desired_display_mode.dmPelsWidth= width;
    desired_display_mode.dmPelsHeight= height;
    desired_display_mode.dmFields= DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
	
	if (M3gResolutionSwitch && // look before you leap!
		(ChangeDisplaySettings(&desired_display_mode, CDS_TEST) == DISP_CHANGE_SUCCESSFUL))
	{
		long result;

		result= ChangeDisplaySettings(&desired_display_mode, CDS_FULLSCREEN);
		success= (result == DISP_CHANGE_SUCCESSFUL);
	}
	else
	{
		success= TRUE; // windowed mode always succeeds
	}

	return success;
}

static void restore_display_settings(
	void)
{
	DEVMODE current_display_settings= {0};

	current_display_settings.dmSize= sizeof(DEVMODE);
	if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &current_display_settings) &&
		((current_display_settings.dmBitsPerPel != original_display_mode.dmBitsPerPel) ||
		(current_display_settings.dmPelsWidth != original_display_mode.dmPelsWidth) ||
		(current_display_settings.dmPelsHeight != original_display_mode.dmPelsHeight)))
	{
		long result= ChangeDisplaySettings(&original_display_mode, 0);
		UUmAssert(result == DISP_CHANGE_SUCCESSFUL);
	}

	return;
}

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

boolean gl_create_render_context(
	void)
{
	boolean success= FALSE;
	
	if (gl && gl->device_context)
	{
		PIXELFORMATDESCRIPTOR pfd;
		short pixel_format;
		
		make_pixel_format_descriptor(&pfd, gl->bit_depth);
		pixel_format= GDI_FXN(ChoosePixelFormat)(gl->device_context, &pfd);
		
		if (pixel_format)
		{
			success= (UUtBool)GDI_FXN(SetPixelFormat)(gl->device_context, pixel_format, &pfd);
			
			if (success)
			{
				/*	Matrox cards decided to stop running with this line uncommented.
					They used to work.. but now they don't.
					And now, a Windows Haiku for your reading pleasure...
					
					Yesterday it worked.
					Today it is not working.
					Windows is like that.

				*/
				//GDI_FXN(DescribePixelFormat)(gl->device_context, pixel_format, sizeof(pfd), &pfd);
				// make sure we have hardware acceleration
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
}

//#define DESTROY_CONTEXTS_ON_DISPLAY_CHANGE we will just crash on cards that don't work with this
// WARNING! This call is also used for resolution switching!!!
UUtBool gl_platform_initialize(
	void)
{
	UUtBool success= UUcTrue;
	static word current_width= 0;
	static word current_height= 0;
	static word current_depth= 0;
	word width= gl->width;
	word height= gl->height;
	word depth= gl->bit_depth;

	if (gl->device_context && // are we switching resolutions from within the game?
		((width != current_width) || (height != current_height) || (depth != current_depth)))
	{
	#ifdef DESTROY_CONTEXTS_ON_DISPLAY_CHANGE // textures get mangled, but doesn't crash the game
		// tear down current contexts & rebuild everything
		success= WGL_FXN(wglMakeCurrent)(NULL, NULL);
		if (!success)
		{
			UUrDebuggerMessage("wglMakeCurrent() failed!");
		}
		UUmAssert(gl->render_context);
		success= WGL_FXN(wglDeleteContext)(gl->render_context);
		UUmAssert(success);
		gl->render_context= NULL;
		if (success)
		{
			RestoreGammaTable();
			success= ReleaseDC(ONgPlatformData.gameWindow, gl->device_context);
			gl->device_context= NULL;
			if (!success)
			{
				UUrDebuggerMessage("ReleaseDC() failed!");
			}
			else
			{
				/* SetPixelFormat() fails if this is done. Windows sucks!
				// unload OpenGL completely, then reload it
				gl_unload_opengl_dll();
				success= gl_load_opengl_dll();
				if (!success)
				{
					UUrDebuggerMessage("failed to reload OpenGL DLL");
				}*/
			}
		}
		else
		{
			UUrDebuggerMessage("wglDeleteContext() failed!");
		}
	#endif
	}

	if ((width != current_width) || (height != current_height) || (depth != current_depth))
	{
		success= set_display_settings(width, height, depth);
	}

	if (success == FALSE)
	{
		if (gl->bit_depth != 16 || (width != 640) || (height != 480))
		{
			width= 640;
			height= 480;
			depth= 16;
			success= set_display_settings(width, height, depth);
			if (success)
			{
				// sure the higher level code thinks the bit depth is different, but who cares? not me!
				gl->bit_depth= depth;
			}
		}
	}

	if (success)
	{
		if ((gl->width != current_width) || (gl->height != current_height))
		{
			SetWindowPos(ONgPlatformData.gameWindow, HWND_TOP,
				0, 0, gl->width, gl->height,
				SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
		}
		if (M3gResolutionSwitch)
		{
			UUmAssert(fullscreen_candidate(ONgPlatformData.gameWindow));
		}
		UUmAssert(ONgPlatformData.gameWindow && (ONgPlatformData.gameWindow != INVALID_HANDLE_VALUE));

		if (!gl->device_context)
		{
			if (success)
			{
				gl->device_context= GetDC(ONgPlatformData.gameWindow);
			}

			SetupGammaTable();

			if (gl->device_context)
			{
				success= gl_create_render_context();
				if (!success)
				{
					if (depth != 16)
					{
						// try again at 16-bit
						success= set_display_settings(width, height, 16);
						if (success)
						{
							gl->bit_depth= depth= 16;
							success= gl_create_render_context();
						}
					}
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
		current_width= gl->width;
		current_height= gl->height;
		current_depth= gl->bit_depth;
	}

	return success;
}

void gl_platform_dispose(
	void)
{
	word screen_width, screen_height;
	boolean success;

	RestoreGammaTable();

	UUmAssert(gl->device_context);
	success= WGL_FXN(wglMakeCurrent)(NULL, NULL);
	UUmAssert(success);

	UUmAssert(gl->render_context);
	success= WGL_FXN(wglDeleteContext)(gl->render_context);
	UUmAssert(success);
	
	UUmAssert(ONgPlatformData.gameWindow && (ONgPlatformData.gameWindow != INVALID_HANDLE_VALUE));
	ReleaseDC(ONgPlatformData.gameWindow, gl->device_context);

	restore_display_settings();

	screen_width= GetSystemMetrics(SM_CXSCREEN);
	screen_height= GetSystemMetrics(SM_CYSCREEN);

	SetWindowPos(ONgPlatformData.gameWindow, HWND_TOP,
		0, 0, screen_width, screen_height,
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);

#ifdef GL_LOAD_API_DYNAMICALLY
	gl_unload_opengl_dll();
#endif

	return;	
}


void gl_display_back_buffer(
	void)
{
	boolean success;

	UUmAssert(gl->device_context);
	success= GDI_FXN(SwapBuffers)(gl->device_context);
	UUmAssert(success);

	return;
}

void gl_matrox_display_back_buffer(
	void)
{
	boolean success;

	UUmAssert(gl->device_context);
	UUmAssert(gl->render_context);
	success= (WGL_FXN(wglMakeCurrent)(gl->device_context, gl->render_context) && // Hello, I am the Matrox G400 and I am a bitch.
		GDI_FXN(SwapBuffers)(gl->device_context));
	UUmAssert(success);

	return;
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

	for (i=0; i<n && i<M3cMaxDisplayModes; i++)
	{
		DEVMODE desired_display_mode;

		memset(&desired_display_mode, 0, sizeof(desired_display_mode));
		desired_display_mode.dmSize= sizeof(DEVMODE);
		desired_display_mode.dmBitsPerPel= desired_display_mode_list[i].bitDepth;
		desired_display_mode.dmPelsWidth= desired_display_mode_list[i].width;
		desired_display_mode.dmPelsHeight= desired_display_mode_list[i].height;
		desired_display_mode.dmFields= DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		if (ChangeDisplaySettings(&desired_display_mode, CDS_TEST) == DISP_CHANGE_SUCCESSFUL)
		{
			display_mode_list[num_valid_display_modes]= desired_display_mode_list[i];
			++num_valid_display_modes;
		}
	}

	return num_valid_display_modes;
}

/*
	this function searches for some common opengl dlls in the windows registry
	it returns the first one it finds, which is not necessarily the correct one!!
*/
static char *gl_find_opengl_dll_from_registry(
	void)
{
	char *registry_subkey[]= {
		// NT/2000 subkeys
		"Software\\Microsoft\\Windows NT\\CurrentVersion\\OpenGLDrivers\\3dfx",
		"Software\\Microsoft\\Windows NT\\CurrentVersion\\OpenGLDrivers\\RIVATNT",
		"Software\\Microsoft\\Windows NT\\CurrentVersion\\OpenGLDrivers",
		// Win9x subkeys
		"Software\\Microsoft\\Windows\\CurrentVersion\\OpenGLDrivers\\3dfx",
		"Software\\Microsoft\\Windows\\CurrentVersion\\OpenGLDrivers\\RIVATNT",
		"Software\\Microsoft\\Windows\\CurrentVersion\\OpenGLDrivers",
		NULL};
	static char dll_filename[256]= "";
	boolean success= FALSE;
	int i=0;

	while ((registry_subkey[i] != NULL) && !success)
	{
		HKEY registry_key= NULL;
		long ret;
		
		ret= RegOpenKeyEx(HKEY_LOCAL_MACHINE, registry_subkey[i], 0, KEY_READ, &registry_key);
		
		if (ret == ERROR_SUCCESS)
		{
			long length= 256, type= REG_SZ;
			char *names[]= {"DLL", "Dll", "dll", NULL};
			int j=0;

			while ((names[j] != NULL) && !success)
			{
				int ret2= RegQueryValueEx(registry_key, names[j], NULL, (unsigned long*)&type, (unsigned char*)dll_filename, (unsigned long*)&length);
				
				if (ret2 == ERROR_SUCCESS)
				{
					success= TRUE; // found it
				}
				++j;
			}
		}
		if (registry_key)
		{
			RegCloseKey(registry_key);
		}
		++i;
	}

	if (!success) dll_filename[0]= '\0';

	return dll_filename;
}

static HANDLE gl_load_dll(
	char *name)
{
	HANDLE dll= NULL;

	dll= GetModuleHandle(name);
	if (dll == NULL)
	{
		dll= LoadLibrary(name);
	}

	return dll;
}

#define LOAD_GL_FUNCTION(name)		GL_FXN(name)= (void*)GetProcAddress(opengl_dll, #name)
#define LOAD_GDI_FUNCTION(name)		GDI_FXN(name)= (void*)GetProcAddress(gdi_dll, #name)

static HANDLE opengl_dll= NULL;
//static HANDLE gdi_dll= NULL;

boolean gl_load_opengl_dll(
	void)
{
	boolean success= TRUE;
	HANDLE window= ONgPlatformData.gameWindow;

	opengl_dll= gl_load_dll(DEFAULT_WIN32_GL_DLL);
	if (!opengl_dll)
	{
		char *dll;

		/*
			this is potentially dangerous; namely, if there are drivers for more
			than 1 video card listed in the registry, we have no way of knowing if
			the one we pick is the right one - picking the wrong one will crash
			the app... ahh well, Windows users should be used to that! :O)
		*/
		dll= gl_find_opengl_dll_from_registry();
		if (dll && (dll[0] != '\0'))
		{
			opengl_dll= gl_load_dll(dll);
		}
	}

	if (opengl_dll) // verify hardware support
	{
		HDC device_context;
		boolean hardware= FALSE;

		device_context= GetDC(window);
		if (device_context)
		{
			PIXELFORMATDESCRIPTOR pfd;
			short pixel_format= 0;
			extern void make_pixel_format_descriptor(PIXELFORMATDESCRIPTOR *pfd, word bit_depth); // gl_mswindows.c
			extern boolean gl_pixel_format_is_accelerated(PIXELFORMATDESCRIPTOR *pfd); // ditto
			DEVMODE current_display_mode;

			current_display_mode.dmSize= sizeof(DEVMODE);
			current_display_mode.dmDriverExtra= 0;
			if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &current_display_mode))
			{
				// save the original display mode
				if (original_display_mode.dmSize == 0)
				{
					original_display_mode= current_display_mode;
				}

				if (current_display_mode.dmBitsPerPel < 16)
				{
					// display has to be 16 bpp or better to get hardware acceleration
					long result;
					int width, height;

					width= current_display_mode.dmPelsWidth;
					height= current_display_mode.dmPelsHeight;
					UUrMemory_Clear(&current_display_mode, sizeof(current_display_mode));
					current_display_mode.dmSize= sizeof(DEVMODE);
					current_display_mode.dmBitsPerPel= 16;
					current_display_mode.dmPelsWidth= width;
					current_display_mode.dmPelsHeight= height;
					current_display_mode.dmFields= DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
					result= ChangeDisplaySettings(&current_display_mode, 0);
					// it's not terribly important that this succeed;
					// if this failed, make_pixel_format_descriptor() will return a software GL pixel format
					if (result == DISP_CHANGE_SUCCESSFUL)
					{
						// the device context will no longer be valid
						if (ReleaseDC(window, device_context))
						{
							// reload DLL... man what a bitch this is turning out to be
							FreeLibrary(opengl_dll);
							// only bother to look for default DLL;
							// they deserve to die a horrible death
							// if their POS card can't handle 16-bit
							opengl_dll= gl_load_dll(DEFAULT_WIN32_GL_DLL);
							// need to ensure we get a 16-bit window... grab the desktop
							// I HATE WINDOWS
							// I HATE WINDOWS
							// I HATE WINDOWS
							// I WORK AT MICROSOFT
							// oops
							window= GetDesktopWindow();
							device_context= GetDC(window);
						}
					}
				}
			}

			if (opengl_dll)
			{
				make_pixel_format_descriptor(&pfd, (UUtUns16)current_display_mode.dmBitsPerPel); // try for hardware GL
				pixel_format= ChoosePixelFormat(device_context, &pfd);
			}
			if (pixel_format)
			{
				hardware= gl_pixel_format_is_accelerated(&pfd);

				if (!hardware)
				{
					FreeLibrary(opengl_dll);
					opengl_dll= NULL;
				}
			}
			else
			{
				if (opengl_dll)
				{
					FreeLibrary(opengl_dll);
				}
				opengl_dll= NULL;
			}

			if (device_context)
			{
				ReleaseDC(window, device_context);
			}
		}
		else
		{
			if (opengl_dll)
			{
				FreeLibrary(opengl_dll);
			}
			opengl_dll= NULL;
		}
	}

	if (opengl_dll)
	{
		int i, n;

/*	#ifdef _WINGDI_
		LOAD_GDI_FUNCTION(ChoosePixelFormat);
		LOAD_GDI_FUNCTION(DescribePixelFormat);
		LOAD_GDI_FUNCTION(SetPixelFormat);
		LOAD_GDI_FUNCTION(GetPixelFormat);
		LOAD_GDI_FUNCTION(SwapBuffers);

		LOAD_GDI_FUNCTION(CopyContext);
		LOAD_GDI_FUNCTION(CreateContext);
		LOAD_GDI_FUNCTION(CreateLayerContext);
		LOAD_GDI_FUNCTION(DeleteContext);
		LOAD_GDI_FUNCTION(GetCurrentContext);
		LOAD_GDI_FUNCTION(GetCurrentDC);
		LOAD_GDI_FUNCTION(GetProcAddress);
		LOAD_GDI_FUNCTION(MakeCurrent);
		LOAD_GDI_FUNCTION(ShareLists);
		LOAD_GDI_FUNCTION(UseFontBitmaps);

		LOAD_GDI_FUNCTION(UseFontOutlines);

		LOAD_GDI_FUNCTION(DescribeLayerPlane);
		LOAD_GDI_FUNCTION(SetLayerPaletteEntries);
		LOAD_GDI_FUNCTION(GetLayerPaletteEntries);
		LOAD_GDI_FUNCTION(RealizeLayerPalette);
		LOAD_GDI_FUNCTION(SwapLayerBuffers);
	#endif // _WINGDI_*/

		LOAD_GL_FUNCTION(glAccum);
		LOAD_GL_FUNCTION(glAlphaFunc);
		LOAD_GL_FUNCTION(glAreTexturesResident);
		LOAD_GL_FUNCTION(glArrayElement);
		LOAD_GL_FUNCTION(glBegin);
		LOAD_GL_FUNCTION(glBindTexture);
		LOAD_GL_FUNCTION(glBitmap);
		LOAD_GL_FUNCTION(glBlendFunc);
		LOAD_GL_FUNCTION(glCallList);
		LOAD_GL_FUNCTION(glCallLists);
		LOAD_GL_FUNCTION(glClear);
		LOAD_GL_FUNCTION(glClearAccum);
		LOAD_GL_FUNCTION(glClearColor);
		LOAD_GL_FUNCTION(glClearDepth);
		LOAD_GL_FUNCTION(glClearIndex);
		LOAD_GL_FUNCTION(glClearStencil);
		LOAD_GL_FUNCTION(glClipPlane);
		LOAD_GL_FUNCTION(glColor3b);
		LOAD_GL_FUNCTION(glColor3bv);
		LOAD_GL_FUNCTION(glColor3d);
		LOAD_GL_FUNCTION(glColor3dv);
		LOAD_GL_FUNCTION(glColor3f);
		LOAD_GL_FUNCTION(glColor3fv);
		LOAD_GL_FUNCTION(glColor3i);
		LOAD_GL_FUNCTION(glColor3iv);
		LOAD_GL_FUNCTION(glColor3s);
		LOAD_GL_FUNCTION(glColor3sv);
		LOAD_GL_FUNCTION(glColor3ub);
		LOAD_GL_FUNCTION(glColor3ubv);
		LOAD_GL_FUNCTION(glColor3ui);
		LOAD_GL_FUNCTION(glColor3uiv);
		LOAD_GL_FUNCTION(glColor3us);
		LOAD_GL_FUNCTION(glColor3usv);
		LOAD_GL_FUNCTION(glColor4b);
		LOAD_GL_FUNCTION(glColor4bv);
		LOAD_GL_FUNCTION(glColor4d);
		LOAD_GL_FUNCTION(glColor4dv);
		LOAD_GL_FUNCTION(glColor4f);
		LOAD_GL_FUNCTION(glColor4fv);
		LOAD_GL_FUNCTION(glColor4i);
		LOAD_GL_FUNCTION(glColor4iv);
		LOAD_GL_FUNCTION(glColor4s);
		LOAD_GL_FUNCTION(glColor4sv);
		LOAD_GL_FUNCTION(glColor4ub);
		LOAD_GL_FUNCTION(glColor4ubv);
		LOAD_GL_FUNCTION(glColor4ui);
		LOAD_GL_FUNCTION(glColor4uiv);
		LOAD_GL_FUNCTION(glColor4us);
		LOAD_GL_FUNCTION(glColor4usv);
		LOAD_GL_FUNCTION(glColorMask);
		LOAD_GL_FUNCTION(glColorMaterial);
		LOAD_GL_FUNCTION(glColorPointer);
		LOAD_GL_FUNCTION(glCopyPixels);
		LOAD_GL_FUNCTION(glCopyTexImage1D);
		LOAD_GL_FUNCTION(glCopyTexImage2D);
		LOAD_GL_FUNCTION(glCopyTexSubImage1D);
		LOAD_GL_FUNCTION(glCopyTexSubImage2D);
		LOAD_GL_FUNCTION(glCullFace);
		LOAD_GL_FUNCTION(glDeleteLists);
		LOAD_GL_FUNCTION(glDeleteTextures);
		LOAD_GL_FUNCTION(glDepthFunc);
		LOAD_GL_FUNCTION(glDepthMask);
		LOAD_GL_FUNCTION(glDepthRange);
		LOAD_GL_FUNCTION(glDisable);
		LOAD_GL_FUNCTION(glDisableClientState);
		LOAD_GL_FUNCTION(glDrawArrays);
		LOAD_GL_FUNCTION(glDrawBuffer);
		LOAD_GL_FUNCTION(glDrawElements);
		LOAD_GL_FUNCTION(glDrawPixels);
		LOAD_GL_FUNCTION(glEdgeFlag);
		LOAD_GL_FUNCTION(glEdgeFlagPointer);
		LOAD_GL_FUNCTION(glEdgeFlagv);
		LOAD_GL_FUNCTION(glEnable);
		LOAD_GL_FUNCTION(glEnableClientState);
		LOAD_GL_FUNCTION(glEnd);
		LOAD_GL_FUNCTION(glEndList);
		LOAD_GL_FUNCTION(glEvalCoord1d);
		LOAD_GL_FUNCTION(glEvalCoord1dv);
		LOAD_GL_FUNCTION(glEvalCoord1f);
		LOAD_GL_FUNCTION(glEvalCoord1fv);
		LOAD_GL_FUNCTION(glEvalCoord2d);
		LOAD_GL_FUNCTION(glEvalCoord2dv);
		LOAD_GL_FUNCTION(glEvalCoord2f);
		LOAD_GL_FUNCTION(glEvalCoord2fv);
		LOAD_GL_FUNCTION(glEvalMesh1);
		LOAD_GL_FUNCTION(glEvalMesh2);
		LOAD_GL_FUNCTION(glEvalPoint1);
		LOAD_GL_FUNCTION(glEvalPoint2);
		LOAD_GL_FUNCTION(glFeedbackBuffer);
		LOAD_GL_FUNCTION(glFinish);
		LOAD_GL_FUNCTION(glFlush);
		LOAD_GL_FUNCTION(glFogf);
		LOAD_GL_FUNCTION(glFogfv);
		LOAD_GL_FUNCTION(glFogi);
		LOAD_GL_FUNCTION(glFogiv);
		LOAD_GL_FUNCTION(glFrontFace);
		LOAD_GL_FUNCTION(glFrustum);
		LOAD_GL_FUNCTION(glGenLists);
		LOAD_GL_FUNCTION(glGenTextures);
		LOAD_GL_FUNCTION(glGetBooleanv);
		LOAD_GL_FUNCTION(glGetClipPlane);
		LOAD_GL_FUNCTION(glGetDoublev);
		LOAD_GL_FUNCTION(glGetError);
		LOAD_GL_FUNCTION(glGetFloatv);
		LOAD_GL_FUNCTION(glGetIntegerv);
		LOAD_GL_FUNCTION(glGetLightfv);
		LOAD_GL_FUNCTION(glGetLightiv);
		LOAD_GL_FUNCTION(glGetMapdv);
		LOAD_GL_FUNCTION(glGetMapfv);
		LOAD_GL_FUNCTION(glGetMapiv);
		LOAD_GL_FUNCTION(glGetMaterialfv);
		LOAD_GL_FUNCTION(glGetMaterialiv);
		LOAD_GL_FUNCTION(glGetPixelMapfv);
		LOAD_GL_FUNCTION(glGetPixelMapuiv);
		LOAD_GL_FUNCTION(glGetPixelMapusv);
		LOAD_GL_FUNCTION(glGetPointerv);
		LOAD_GL_FUNCTION(glGetPolygonStipple);
		LOAD_GL_FUNCTION(glGetString);
		LOAD_GL_FUNCTION(glGetTexEnvfv);
		LOAD_GL_FUNCTION(glGetTexEnviv);
		LOAD_GL_FUNCTION(glGetTexGendv);
		LOAD_GL_FUNCTION(glGetTexGenfv);
		LOAD_GL_FUNCTION(glGetTexGeniv);
		LOAD_GL_FUNCTION(glGetTexImage);
		LOAD_GL_FUNCTION(glGetTexLevelParameterfv);
		LOAD_GL_FUNCTION(glGetTexLevelParameteriv);
		LOAD_GL_FUNCTION(glGetTexParameterfv);
		LOAD_GL_FUNCTION(glGetTexParameteriv);
		LOAD_GL_FUNCTION(glHint);
		LOAD_GL_FUNCTION(glIndexMask);
		LOAD_GL_FUNCTION(glIndexPointer);
		LOAD_GL_FUNCTION(glIndexd);
		LOAD_GL_FUNCTION(glIndexdv);
		LOAD_GL_FUNCTION(glIndexf);
		LOAD_GL_FUNCTION(glIndexfv);
		LOAD_GL_FUNCTION(glIndexi);
		LOAD_GL_FUNCTION(glIndexiv);
		LOAD_GL_FUNCTION(glIndexs);
		LOAD_GL_FUNCTION(glIndexsv);
		LOAD_GL_FUNCTION(glIndexub);
		LOAD_GL_FUNCTION(glIndexubv);
		LOAD_GL_FUNCTION(glInitNames);
		LOAD_GL_FUNCTION(glInterleavedArrays);
		LOAD_GL_FUNCTION(glIsEnabled);
		LOAD_GL_FUNCTION(glIsList);
		LOAD_GL_FUNCTION(glIsTexture);
		LOAD_GL_FUNCTION(glLightModelf);
		LOAD_GL_FUNCTION(glLightModelfv);
		LOAD_GL_FUNCTION(glLightModeli);
		LOAD_GL_FUNCTION(glLightModeliv);
		LOAD_GL_FUNCTION(glLightf);
		LOAD_GL_FUNCTION(glLightfv);
		LOAD_GL_FUNCTION(glLighti);
		LOAD_GL_FUNCTION(glLightiv);
		LOAD_GL_FUNCTION(glLineStipple);
		LOAD_GL_FUNCTION(glLineWidth);
		LOAD_GL_FUNCTION(glListBase);
		LOAD_GL_FUNCTION(glLoadIdentity);
		LOAD_GL_FUNCTION(glLoadMatrixd);
		LOAD_GL_FUNCTION(glLoadMatrixf);
		LOAD_GL_FUNCTION(glLoadName);
		LOAD_GL_FUNCTION(glLogicOp);
		LOAD_GL_FUNCTION(glMap1d);
		LOAD_GL_FUNCTION(glMap1f);
		LOAD_GL_FUNCTION(glMap2d);
		LOAD_GL_FUNCTION(glMap2f);
		LOAD_GL_FUNCTION(glMapGrid1d);
		LOAD_GL_FUNCTION(glMapGrid1f);
		LOAD_GL_FUNCTION(glMapGrid2d);
		LOAD_GL_FUNCTION(glMapGrid2f);
		LOAD_GL_FUNCTION(glMaterialf);
		LOAD_GL_FUNCTION(glMaterialfv);
		LOAD_GL_FUNCTION(glMateriali);
		LOAD_GL_FUNCTION(glMaterialiv);
		LOAD_GL_FUNCTION(glMatrixMode);
		LOAD_GL_FUNCTION(glMultMatrixd);
		LOAD_GL_FUNCTION(glMultMatrixf);
		LOAD_GL_FUNCTION(glNewList);
		LOAD_GL_FUNCTION(glNormal3b);
		LOAD_GL_FUNCTION(glNormal3bv);
		LOAD_GL_FUNCTION(glNormal3d);
		LOAD_GL_FUNCTION(glNormal3dv);
		LOAD_GL_FUNCTION(glNormal3f);
		LOAD_GL_FUNCTION(glNormal3fv);
		LOAD_GL_FUNCTION(glNormal3i);
		LOAD_GL_FUNCTION(glNormal3iv);
		LOAD_GL_FUNCTION(glNormal3s);
		LOAD_GL_FUNCTION(glNormal3sv);
		LOAD_GL_FUNCTION(glNormalPointer);
		LOAD_GL_FUNCTION(glOrtho);
		LOAD_GL_FUNCTION(glPassThrough);
		LOAD_GL_FUNCTION(glPixelMapfv);
		LOAD_GL_FUNCTION(glPixelMapuiv);
		LOAD_GL_FUNCTION(glPixelMapusv);
		LOAD_GL_FUNCTION(glPixelStoref);
		LOAD_GL_FUNCTION(glPixelStorei);
		LOAD_GL_FUNCTION(glPixelTransferf);
		LOAD_GL_FUNCTION(glPixelTransferi);
		LOAD_GL_FUNCTION(glPixelZoom);
		LOAD_GL_FUNCTION(glPointSize);
		LOAD_GL_FUNCTION(glPolygonMode);
		LOAD_GL_FUNCTION(glPolygonOffset);
		LOAD_GL_FUNCTION(glPolygonStipple);
		LOAD_GL_FUNCTION(glPopAttrib);
		LOAD_GL_FUNCTION(glPopClientAttrib);
		LOAD_GL_FUNCTION(glPopMatrix);
		LOAD_GL_FUNCTION(glPopName);
		LOAD_GL_FUNCTION(glPrioritizeTextures);
		LOAD_GL_FUNCTION(glPushAttrib);
		LOAD_GL_FUNCTION(glPushClientAttrib);
		LOAD_GL_FUNCTION(glPushMatrix);
		LOAD_GL_FUNCTION(glPushName);
		LOAD_GL_FUNCTION(glRasterPos2d);
		LOAD_GL_FUNCTION(glRasterPos2dv);
		LOAD_GL_FUNCTION(glRasterPos2f);
		LOAD_GL_FUNCTION(glRasterPos2fv);
		LOAD_GL_FUNCTION(glRasterPos2i);
		LOAD_GL_FUNCTION(glRasterPos2iv);
		LOAD_GL_FUNCTION(glRasterPos2s);
		LOAD_GL_FUNCTION(glRasterPos2sv);
		LOAD_GL_FUNCTION(glRasterPos3d);
		LOAD_GL_FUNCTION(glRasterPos3dv);
		LOAD_GL_FUNCTION(glRasterPos3f);
		LOAD_GL_FUNCTION(glRasterPos3fv);
		LOAD_GL_FUNCTION(glRasterPos3i);
		LOAD_GL_FUNCTION(glRasterPos3iv);
		LOAD_GL_FUNCTION(glRasterPos3s);
		LOAD_GL_FUNCTION(glRasterPos3sv);
		LOAD_GL_FUNCTION(glRasterPos4d);
		LOAD_GL_FUNCTION(glRasterPos4dv);
		LOAD_GL_FUNCTION(glRasterPos4f);
		LOAD_GL_FUNCTION(glRasterPos4fv);
		LOAD_GL_FUNCTION(glRasterPos4i);
		LOAD_GL_FUNCTION(glRasterPos4iv);
		LOAD_GL_FUNCTION(glRasterPos4s);
		LOAD_GL_FUNCTION(glRasterPos4sv);
		LOAD_GL_FUNCTION(glReadBuffer);
		LOAD_GL_FUNCTION(glReadPixels);
		LOAD_GL_FUNCTION(glRectd);
		LOAD_GL_FUNCTION(glRectdv);
		LOAD_GL_FUNCTION(glRectf);
		LOAD_GL_FUNCTION(glRectfv);
		LOAD_GL_FUNCTION(glRecti);
		LOAD_GL_FUNCTION(glRectiv);
		LOAD_GL_FUNCTION(glRects);
		LOAD_GL_FUNCTION(glRectsv);
		LOAD_GL_FUNCTION(glRenderMode);
		LOAD_GL_FUNCTION(glRotated);
		LOAD_GL_FUNCTION(glRotatef);
		LOAD_GL_FUNCTION(glScaled);
		LOAD_GL_FUNCTION(glScalef);
		LOAD_GL_FUNCTION(glScissor);
		LOAD_GL_FUNCTION(glSelectBuffer);
		LOAD_GL_FUNCTION(glShadeModel);
		LOAD_GL_FUNCTION(glStencilFunc);
		LOAD_GL_FUNCTION(glStencilMask);
		LOAD_GL_FUNCTION(glStencilOp);
		LOAD_GL_FUNCTION(glTexCoord1d);
		LOAD_GL_FUNCTION(glTexCoord1dv);
		LOAD_GL_FUNCTION(glTexCoord1f);
		LOAD_GL_FUNCTION(glTexCoord1fv);
		LOAD_GL_FUNCTION(glTexCoord1i);
		LOAD_GL_FUNCTION(glTexCoord1iv);
		LOAD_GL_FUNCTION(glTexCoord1s);
		LOAD_GL_FUNCTION(glTexCoord1sv);
		LOAD_GL_FUNCTION(glTexCoord2d);
		LOAD_GL_FUNCTION(glTexCoord2dv);
		LOAD_GL_FUNCTION(glTexCoord2f);
		LOAD_GL_FUNCTION(glTexCoord2fv);
		LOAD_GL_FUNCTION(glTexCoord2i);
		LOAD_GL_FUNCTION(glTexCoord2iv);
		LOAD_GL_FUNCTION(glTexCoord2s);
		LOAD_GL_FUNCTION(glTexCoord2sv);
		LOAD_GL_FUNCTION(glTexCoord3d);
		LOAD_GL_FUNCTION(glTexCoord3dv);
		LOAD_GL_FUNCTION(glTexCoord3f);
		LOAD_GL_FUNCTION(glTexCoord3fv);
		LOAD_GL_FUNCTION(glTexCoord3i);
		LOAD_GL_FUNCTION(glTexCoord3iv);
		LOAD_GL_FUNCTION(glTexCoord3s);
		LOAD_GL_FUNCTION(glTexCoord3sv);
		LOAD_GL_FUNCTION(glTexCoord4d);
		LOAD_GL_FUNCTION(glTexCoord4dv);
		LOAD_GL_FUNCTION(glTexCoord4f);
		LOAD_GL_FUNCTION(glTexCoord4fv);
		LOAD_GL_FUNCTION(glTexCoord4i);
		LOAD_GL_FUNCTION(glTexCoord4iv);
		LOAD_GL_FUNCTION(glTexCoord4s);
		LOAD_GL_FUNCTION(glTexCoord4sv);
		LOAD_GL_FUNCTION(glTexCoordPointer);
		LOAD_GL_FUNCTION(glTexEnvf);
		LOAD_GL_FUNCTION(glTexEnvfv);
		LOAD_GL_FUNCTION(glTexEnvi);
		LOAD_GL_FUNCTION(glTexEnviv);
		LOAD_GL_FUNCTION(glTexGend);
		LOAD_GL_FUNCTION(glTexGendv);
		LOAD_GL_FUNCTION(glTexGenf);
		LOAD_GL_FUNCTION(glTexGenfv);
		LOAD_GL_FUNCTION(glTexGeni);
		LOAD_GL_FUNCTION(glTexGeniv);
		LOAD_GL_FUNCTION(glTexImage1D);
		LOAD_GL_FUNCTION(glTexImage2D);
		LOAD_GL_FUNCTION(glTexParameterf);
		LOAD_GL_FUNCTION(glTexParameterfv);
		LOAD_GL_FUNCTION(glTexParameteri);
		LOAD_GL_FUNCTION(glTexParameteriv);
		LOAD_GL_FUNCTION(glTexSubImage1D);
		LOAD_GL_FUNCTION(glTexSubImage2D);
		LOAD_GL_FUNCTION(glTranslated);
		LOAD_GL_FUNCTION(glTranslatef);
		LOAD_GL_FUNCTION(glVertex2d);
		LOAD_GL_FUNCTION(glVertex2dv);
		LOAD_GL_FUNCTION(glVertex2f);
		LOAD_GL_FUNCTION(glVertex2fv);
		LOAD_GL_FUNCTION(glVertex2i);
		LOAD_GL_FUNCTION(glVertex2iv);
		LOAD_GL_FUNCTION(glVertex2s);
		LOAD_GL_FUNCTION(glVertex2sv);
		LOAD_GL_FUNCTION(glVertex3d);
		LOAD_GL_FUNCTION(glVertex3dv);
		LOAD_GL_FUNCTION(glVertex3f);
		LOAD_GL_FUNCTION(glVertex3fv);
		LOAD_GL_FUNCTION(glVertex3i);
		LOAD_GL_FUNCTION(glVertex3iv);
		LOAD_GL_FUNCTION(glVertex3s);
		LOAD_GL_FUNCTION(glVertex3sv);
		LOAD_GL_FUNCTION(glVertex4d);
		LOAD_GL_FUNCTION(glVertex4dv);
		LOAD_GL_FUNCTION(glVertex4f);
		LOAD_GL_FUNCTION(glVertex4fv);
		LOAD_GL_FUNCTION(glVertex4i);
		LOAD_GL_FUNCTION(glVertex4iv);
		LOAD_GL_FUNCTION(glVertex4s);
		LOAD_GL_FUNCTION(glVertex4sv);
		LOAD_GL_FUNCTION(glVertexPointer);
		LOAD_GL_FUNCTION(glViewport);

		// WGL
		LOAD_GL_FUNCTION(wglCopyContext);
		LOAD_GL_FUNCTION(wglCreateContext);
		LOAD_GL_FUNCTION(wglCreateLayerContext);
		LOAD_GL_FUNCTION(wglDeleteContext);
		LOAD_GL_FUNCTION(wglGetCurrentContext);
		LOAD_GL_FUNCTION(wglGetCurrentDC);
		LOAD_GL_FUNCTION(wglGetProcAddress);
		LOAD_GL_FUNCTION(wglMakeCurrent);
		LOAD_GL_FUNCTION(wglShareLists);

		gl_api->end_standard_api= (void *)NONE;

		// 3Dfx gamma extensions (optional)
		LOAD_GL_FUNCTION(wglSetDeviceGammaRamp3DFX);
		LOAD_GL_FUNCTION(wglGetDeviceGammaRamp3DFX);
	
		n= sizeof(struct gl_api)/sizeof(void *);
		for (i=0; i<n; i++)
		{
			void **api_ptr= (void **)gl_api;

			if (api_ptr[i] == (void *)NONE) // gl_api->end_standard_api
			{
				break;
			}
			if (api_ptr[i] == NULL)
			{
				success= FALSE;
				break;
			}
		}
	}
	else
	{
		success= FALSE;
	}

	return success;
}

#define UNLOAD_GL_FUNCTION(name)	GL_FXN(name)= NULL

static void gl_unload_opengl_dll(
	void)
{
	if (opengl_dll)
	{
		FreeLibrary(opengl_dll);
		opengl_dll= NULL;
	}

#ifdef GL_LOAD_API_DYNAMICALLY
	if (gl_api)
#endif
	{
		UNLOAD_GL_FUNCTION(glAccum);
		UNLOAD_GL_FUNCTION(glAlphaFunc);
		UNLOAD_GL_FUNCTION(glAreTexturesResident);
		UNLOAD_GL_FUNCTION(glArrayElement);
		UNLOAD_GL_FUNCTION(glBegin);
		UNLOAD_GL_FUNCTION(glBindTexture);
		UNLOAD_GL_FUNCTION(glBitmap);
		UNLOAD_GL_FUNCTION(glBlendFunc);
		UNLOAD_GL_FUNCTION(glCallList);
		UNLOAD_GL_FUNCTION(glCallLists);
		UNLOAD_GL_FUNCTION(glClear);
		UNLOAD_GL_FUNCTION(glClearAccum);
		UNLOAD_GL_FUNCTION(glClearColor);
		UNLOAD_GL_FUNCTION(glClearDepth);
		UNLOAD_GL_FUNCTION(glClearIndex);
		UNLOAD_GL_FUNCTION(glClearStencil);
		UNLOAD_GL_FUNCTION(glClipPlane);
		UNLOAD_GL_FUNCTION(glColor3b);
		UNLOAD_GL_FUNCTION(glColor3bv);
		UNLOAD_GL_FUNCTION(glColor3d);
		UNLOAD_GL_FUNCTION(glColor3dv);
		UNLOAD_GL_FUNCTION(glColor3f);
		UNLOAD_GL_FUNCTION(glColor3fv);
		UNLOAD_GL_FUNCTION(glColor3i);
		UNLOAD_GL_FUNCTION(glColor3iv);
		UNLOAD_GL_FUNCTION(glColor3s);
		UNLOAD_GL_FUNCTION(glColor3sv);
		UNLOAD_GL_FUNCTION(glColor3ub);
		UNLOAD_GL_FUNCTION(glColor3ubv);
		UNLOAD_GL_FUNCTION(glColor3ui);
		UNLOAD_GL_FUNCTION(glColor3uiv);
		UNLOAD_GL_FUNCTION(glColor3us);
		UNLOAD_GL_FUNCTION(glColor3usv);
		UNLOAD_GL_FUNCTION(glColor4b);
		UNLOAD_GL_FUNCTION(glColor4bv);
		UNLOAD_GL_FUNCTION(glColor4d);
		UNLOAD_GL_FUNCTION(glColor4dv);
		UNLOAD_GL_FUNCTION(glColor4f);
		UNLOAD_GL_FUNCTION(glColor4fv);
		UNLOAD_GL_FUNCTION(glColor4i);
		UNLOAD_GL_FUNCTION(glColor4iv);
		UNLOAD_GL_FUNCTION(glColor4s);
		UNLOAD_GL_FUNCTION(glColor4sv);
		UNLOAD_GL_FUNCTION(glColor4ub);
		UNLOAD_GL_FUNCTION(glColor4ubv);
		UNLOAD_GL_FUNCTION(glColor4ui);
		UNLOAD_GL_FUNCTION(glColor4uiv);
		UNLOAD_GL_FUNCTION(glColor4us);
		UNLOAD_GL_FUNCTION(glColor4usv);
		UNLOAD_GL_FUNCTION(glColorMask);
		UNLOAD_GL_FUNCTION(glColorMaterial);
		UNLOAD_GL_FUNCTION(glColorPointer);
		UNLOAD_GL_FUNCTION(glCopyPixels);
		UNLOAD_GL_FUNCTION(glCopyTexImage1D);
		UNLOAD_GL_FUNCTION(glCopyTexImage2D);
		UNLOAD_GL_FUNCTION(glCopyTexSubImage1D);
		UNLOAD_GL_FUNCTION(glCopyTexSubImage2D);
		UNLOAD_GL_FUNCTION(glCullFace);
		UNLOAD_GL_FUNCTION(glDeleteLists);
		UNLOAD_GL_FUNCTION(glDeleteTextures);
		UNLOAD_GL_FUNCTION(glDepthFunc);
		UNLOAD_GL_FUNCTION(glDepthMask);
		UNLOAD_GL_FUNCTION(glDepthRange);
		UNLOAD_GL_FUNCTION(glDisable);
		UNLOAD_GL_FUNCTION(glDisableClientState);
		UNLOAD_GL_FUNCTION(glDrawArrays);
		UNLOAD_GL_FUNCTION(glDrawBuffer);
		UNLOAD_GL_FUNCTION(glDrawElements);
		UNLOAD_GL_FUNCTION(glDrawPixels);
		UNLOAD_GL_FUNCTION(glEdgeFlag);
		UNLOAD_GL_FUNCTION(glEdgeFlagPointer);
		UNLOAD_GL_FUNCTION(glEdgeFlagv);
		UNLOAD_GL_FUNCTION(glEnable);
		UNLOAD_GL_FUNCTION(glEnableClientState);
		UNLOAD_GL_FUNCTION(glEnd);
		UNLOAD_GL_FUNCTION(glEndList);
		UNLOAD_GL_FUNCTION(glEvalCoord1d);
		UNLOAD_GL_FUNCTION(glEvalCoord1dv);
		UNLOAD_GL_FUNCTION(glEvalCoord1f);
		UNLOAD_GL_FUNCTION(glEvalCoord1fv);
		UNLOAD_GL_FUNCTION(glEvalCoord2d);
		UNLOAD_GL_FUNCTION(glEvalCoord2dv);
		UNLOAD_GL_FUNCTION(glEvalCoord2f);
		UNLOAD_GL_FUNCTION(glEvalCoord2fv);
		UNLOAD_GL_FUNCTION(glEvalMesh1);
		UNLOAD_GL_FUNCTION(glEvalMesh2);
		UNLOAD_GL_FUNCTION(glEvalPoint1);
		UNLOAD_GL_FUNCTION(glEvalPoint2);
		UNLOAD_GL_FUNCTION(glFeedbackBuffer);
		UNLOAD_GL_FUNCTION(glFinish);
		UNLOAD_GL_FUNCTION(glFlush);
		UNLOAD_GL_FUNCTION(glFogf);
		UNLOAD_GL_FUNCTION(glFogfv);
		UNLOAD_GL_FUNCTION(glFogi);
		UNLOAD_GL_FUNCTION(glFogiv);
		UNLOAD_GL_FUNCTION(glFrontFace);
		UNLOAD_GL_FUNCTION(glFrustum);
		UNLOAD_GL_FUNCTION(glGenLists);
		UNLOAD_GL_FUNCTION(glGenTextures);
		UNLOAD_GL_FUNCTION(glGetBooleanv);
		UNLOAD_GL_FUNCTION(glGetClipPlane);
		UNLOAD_GL_FUNCTION(glGetDoublev);
		UNLOAD_GL_FUNCTION(glGetError);
		UNLOAD_GL_FUNCTION(glGetFloatv);
		UNLOAD_GL_FUNCTION(glGetIntegerv);
		UNLOAD_GL_FUNCTION(glGetLightfv);
		UNLOAD_GL_FUNCTION(glGetLightiv);
		UNLOAD_GL_FUNCTION(glGetMapdv);
		UNLOAD_GL_FUNCTION(glGetMapfv);
		UNLOAD_GL_FUNCTION(glGetMapiv);
		UNLOAD_GL_FUNCTION(glGetMaterialfv);
		UNLOAD_GL_FUNCTION(glGetMaterialiv);
		UNLOAD_GL_FUNCTION(glGetPixelMapfv);
		UNLOAD_GL_FUNCTION(glGetPixelMapuiv);
		UNLOAD_GL_FUNCTION(glGetPixelMapusv);
		UNLOAD_GL_FUNCTION(glGetPointerv);
		UNLOAD_GL_FUNCTION(glGetPolygonStipple);
		UNLOAD_GL_FUNCTION(glGetString);
		UNLOAD_GL_FUNCTION(glGetTexEnvfv);
		UNLOAD_GL_FUNCTION(glGetTexEnviv);
		UNLOAD_GL_FUNCTION(glGetTexGendv);
		UNLOAD_GL_FUNCTION(glGetTexGenfv);
		UNLOAD_GL_FUNCTION(glGetTexGeniv);
		UNLOAD_GL_FUNCTION(glGetTexImage);
		UNLOAD_GL_FUNCTION(glGetTexLevelParameterfv);
		UNLOAD_GL_FUNCTION(glGetTexLevelParameteriv);
		UNLOAD_GL_FUNCTION(glGetTexParameterfv);
		UNLOAD_GL_FUNCTION(glGetTexParameteriv);
		UNLOAD_GL_FUNCTION(glHint);
		UNLOAD_GL_FUNCTION(glIndexMask);
		UNLOAD_GL_FUNCTION(glIndexPointer);
		UNLOAD_GL_FUNCTION(glIndexd);
		UNLOAD_GL_FUNCTION(glIndexdv);
		UNLOAD_GL_FUNCTION(glIndexf);
		UNLOAD_GL_FUNCTION(glIndexfv);
		UNLOAD_GL_FUNCTION(glIndexi);
		UNLOAD_GL_FUNCTION(glIndexiv);
		UNLOAD_GL_FUNCTION(glIndexs);
		UNLOAD_GL_FUNCTION(glIndexsv);
		UNLOAD_GL_FUNCTION(glIndexub);
		UNLOAD_GL_FUNCTION(glIndexubv);
		UNLOAD_GL_FUNCTION(glInitNames);
		UNLOAD_GL_FUNCTION(glInterleavedArrays);
		UNLOAD_GL_FUNCTION(glIsEnabled);
		UNLOAD_GL_FUNCTION(glIsList);
		UNLOAD_GL_FUNCTION(glIsTexture);
		UNLOAD_GL_FUNCTION(glLightModelf);
		UNLOAD_GL_FUNCTION(glLightModelfv);
		UNLOAD_GL_FUNCTION(glLightModeli);
		UNLOAD_GL_FUNCTION(glLightModeliv);
		UNLOAD_GL_FUNCTION(glLightf);
		UNLOAD_GL_FUNCTION(glLightfv);
		UNLOAD_GL_FUNCTION(glLighti);
		UNLOAD_GL_FUNCTION(glLightiv);
		UNLOAD_GL_FUNCTION(glLineStipple);
		UNLOAD_GL_FUNCTION(glLineWidth);
		UNLOAD_GL_FUNCTION(glListBase);
		UNLOAD_GL_FUNCTION(glLoadIdentity);
		UNLOAD_GL_FUNCTION(glLoadMatrixd);
		UNLOAD_GL_FUNCTION(glLoadMatrixf);
		UNLOAD_GL_FUNCTION(glLoadName);
		UNLOAD_GL_FUNCTION(glLogicOp);
		UNLOAD_GL_FUNCTION(glMap1d);
		UNLOAD_GL_FUNCTION(glMap1f);
		UNLOAD_GL_FUNCTION(glMap2d);
		UNLOAD_GL_FUNCTION(glMap2f);
		UNLOAD_GL_FUNCTION(glMapGrid1d);
		UNLOAD_GL_FUNCTION(glMapGrid1f);
		UNLOAD_GL_FUNCTION(glMapGrid2d);
		UNLOAD_GL_FUNCTION(glMapGrid2f);
		UNLOAD_GL_FUNCTION(glMaterialf);
		UNLOAD_GL_FUNCTION(glMaterialfv);
		UNLOAD_GL_FUNCTION(glMateriali);
		UNLOAD_GL_FUNCTION(glMaterialiv);
		UNLOAD_GL_FUNCTION(glMatrixMode);
		UNLOAD_GL_FUNCTION(glMultMatrixd);
		UNLOAD_GL_FUNCTION(glMultMatrixf);
		UNLOAD_GL_FUNCTION(glNewList);
		UNLOAD_GL_FUNCTION(glNormal3b);
		UNLOAD_GL_FUNCTION(glNormal3bv);
		UNLOAD_GL_FUNCTION(glNormal3d);
		UNLOAD_GL_FUNCTION(glNormal3dv);
		UNLOAD_GL_FUNCTION(glNormal3f);
		UNLOAD_GL_FUNCTION(glNormal3fv);
		UNLOAD_GL_FUNCTION(glNormal3i);
		UNLOAD_GL_FUNCTION(glNormal3iv);
		UNLOAD_GL_FUNCTION(glNormal3s);
		UNLOAD_GL_FUNCTION(glNormal3sv);
		UNLOAD_GL_FUNCTION(glNormalPointer);
		UNLOAD_GL_FUNCTION(glOrtho);
		UNLOAD_GL_FUNCTION(glPassThrough);
		UNLOAD_GL_FUNCTION(glPixelMapfv);
		UNLOAD_GL_FUNCTION(glPixelMapuiv);
		UNLOAD_GL_FUNCTION(glPixelMapusv);
		UNLOAD_GL_FUNCTION(glPixelStoref);
		UNLOAD_GL_FUNCTION(glPixelStorei);
		UNLOAD_GL_FUNCTION(glPixelTransferf);
		UNLOAD_GL_FUNCTION(glPixelTransferi);
		UNLOAD_GL_FUNCTION(glPixelZoom);
		UNLOAD_GL_FUNCTION(glPointSize);
		UNLOAD_GL_FUNCTION(glPolygonMode);
		UNLOAD_GL_FUNCTION(glPolygonOffset);
		UNLOAD_GL_FUNCTION(glPolygonStipple);
		UNLOAD_GL_FUNCTION(glPopAttrib);
		UNLOAD_GL_FUNCTION(glPopClientAttrib);
		UNLOAD_GL_FUNCTION(glPopMatrix);
		UNLOAD_GL_FUNCTION(glPopName);
		UNLOAD_GL_FUNCTION(glPrioritizeTextures);
		UNLOAD_GL_FUNCTION(glPushAttrib);
		UNLOAD_GL_FUNCTION(glPushClientAttrib);
		UNLOAD_GL_FUNCTION(glPushMatrix);
		UNLOAD_GL_FUNCTION(glPushName);
		UNLOAD_GL_FUNCTION(glRasterPos2d);
		UNLOAD_GL_FUNCTION(glRasterPos2dv);
		UNLOAD_GL_FUNCTION(glRasterPos2f);
		UNLOAD_GL_FUNCTION(glRasterPos2fv);
		UNLOAD_GL_FUNCTION(glRasterPos2i);
		UNLOAD_GL_FUNCTION(glRasterPos2iv);
		UNLOAD_GL_FUNCTION(glRasterPos2s);
		UNLOAD_GL_FUNCTION(glRasterPos2sv);
		UNLOAD_GL_FUNCTION(glRasterPos3d);
		UNLOAD_GL_FUNCTION(glRasterPos3dv);
		UNLOAD_GL_FUNCTION(glRasterPos3f);
		UNLOAD_GL_FUNCTION(glRasterPos3fv);
		UNLOAD_GL_FUNCTION(glRasterPos3i);
		UNLOAD_GL_FUNCTION(glRasterPos3iv);
		UNLOAD_GL_FUNCTION(glRasterPos3s);
		UNLOAD_GL_FUNCTION(glRasterPos3sv);
		UNLOAD_GL_FUNCTION(glRasterPos4d);
		UNLOAD_GL_FUNCTION(glRasterPos4dv);
		UNLOAD_GL_FUNCTION(glRasterPos4f);
		UNLOAD_GL_FUNCTION(glRasterPos4fv);
		UNLOAD_GL_FUNCTION(glRasterPos4i);
		UNLOAD_GL_FUNCTION(glRasterPos4iv);
		UNLOAD_GL_FUNCTION(glRasterPos4s);
		UNLOAD_GL_FUNCTION(glRasterPos4sv);
		UNLOAD_GL_FUNCTION(glReadBuffer);
		UNLOAD_GL_FUNCTION(glReadPixels);
		UNLOAD_GL_FUNCTION(glRectd);
		UNLOAD_GL_FUNCTION(glRectdv);
		UNLOAD_GL_FUNCTION(glRectf);
		UNLOAD_GL_FUNCTION(glRectfv);
		UNLOAD_GL_FUNCTION(glRecti);
		UNLOAD_GL_FUNCTION(glRectiv);
		UNLOAD_GL_FUNCTION(glRects);
		UNLOAD_GL_FUNCTION(glRectsv);
		UNLOAD_GL_FUNCTION(glRenderMode);
		UNLOAD_GL_FUNCTION(glRotated);
		UNLOAD_GL_FUNCTION(glRotatef);
		UNLOAD_GL_FUNCTION(glScaled);
		UNLOAD_GL_FUNCTION(glScalef);
		UNLOAD_GL_FUNCTION(glScissor);
		UNLOAD_GL_FUNCTION(glSelectBuffer);
		UNLOAD_GL_FUNCTION(glShadeModel);
		UNLOAD_GL_FUNCTION(glStencilFunc);
		UNLOAD_GL_FUNCTION(glStencilMask);
		UNLOAD_GL_FUNCTION(glStencilOp);
		UNLOAD_GL_FUNCTION(glTexCoord1d);
		UNLOAD_GL_FUNCTION(glTexCoord1dv);
		UNLOAD_GL_FUNCTION(glTexCoord1f);
		UNLOAD_GL_FUNCTION(glTexCoord1fv);
		UNLOAD_GL_FUNCTION(glTexCoord1i);
		UNLOAD_GL_FUNCTION(glTexCoord1iv);
		UNLOAD_GL_FUNCTION(glTexCoord1s);
		UNLOAD_GL_FUNCTION(glTexCoord1sv);
		UNLOAD_GL_FUNCTION(glTexCoord2d);
		UNLOAD_GL_FUNCTION(glTexCoord2dv);
		UNLOAD_GL_FUNCTION(glTexCoord2f);
		UNLOAD_GL_FUNCTION(glTexCoord2fv);
		UNLOAD_GL_FUNCTION(glTexCoord2i);
		UNLOAD_GL_FUNCTION(glTexCoord2iv);
		UNLOAD_GL_FUNCTION(glTexCoord2s);
		UNLOAD_GL_FUNCTION(glTexCoord2sv);
		UNLOAD_GL_FUNCTION(glTexCoord3d);
		UNLOAD_GL_FUNCTION(glTexCoord3dv);
		UNLOAD_GL_FUNCTION(glTexCoord3f);
		UNLOAD_GL_FUNCTION(glTexCoord3fv);
		UNLOAD_GL_FUNCTION(glTexCoord3i);
		UNLOAD_GL_FUNCTION(glTexCoord3iv);
		UNLOAD_GL_FUNCTION(glTexCoord3s);
		UNLOAD_GL_FUNCTION(glTexCoord3sv);
		UNLOAD_GL_FUNCTION(glTexCoord4d);
		UNLOAD_GL_FUNCTION(glTexCoord4dv);
		UNLOAD_GL_FUNCTION(glTexCoord4f);
		UNLOAD_GL_FUNCTION(glTexCoord4fv);
		UNLOAD_GL_FUNCTION(glTexCoord4i);
		UNLOAD_GL_FUNCTION(glTexCoord4iv);
		UNLOAD_GL_FUNCTION(glTexCoord4s);
		UNLOAD_GL_FUNCTION(glTexCoord4sv);
		UNLOAD_GL_FUNCTION(glTexCoordPointer);
		UNLOAD_GL_FUNCTION(glTexEnvf);
		UNLOAD_GL_FUNCTION(glTexEnvfv);
		UNLOAD_GL_FUNCTION(glTexEnvi);
		UNLOAD_GL_FUNCTION(glTexEnviv);
		UNLOAD_GL_FUNCTION(glTexGend);
		UNLOAD_GL_FUNCTION(glTexGendv);
		UNLOAD_GL_FUNCTION(glTexGenf);
		UNLOAD_GL_FUNCTION(glTexGenfv);
		UNLOAD_GL_FUNCTION(glTexGeni);
		UNLOAD_GL_FUNCTION(glTexGeniv);
		UNLOAD_GL_FUNCTION(glTexImage1D);
		UNLOAD_GL_FUNCTION(glTexImage2D);
		UNLOAD_GL_FUNCTION(glTexParameterf);
		UNLOAD_GL_FUNCTION(glTexParameterfv);
		UNLOAD_GL_FUNCTION(glTexParameteri);
		UNLOAD_GL_FUNCTION(glTexParameteriv);
		UNLOAD_GL_FUNCTION(glTexSubImage1D);
		UNLOAD_GL_FUNCTION(glTexSubImage2D);
		UNLOAD_GL_FUNCTION(glTranslated);
		UNLOAD_GL_FUNCTION(glTranslatef);
		UNLOAD_GL_FUNCTION(glVertex2d);
		UNLOAD_GL_FUNCTION(glVertex2dv);
		UNLOAD_GL_FUNCTION(glVertex2f);
		UNLOAD_GL_FUNCTION(glVertex2fv);
		UNLOAD_GL_FUNCTION(glVertex2i);
		UNLOAD_GL_FUNCTION(glVertex2iv);
		UNLOAD_GL_FUNCTION(glVertex2s);
		UNLOAD_GL_FUNCTION(glVertex2sv);
		UNLOAD_GL_FUNCTION(glVertex3d);
		UNLOAD_GL_FUNCTION(glVertex3dv);
		UNLOAD_GL_FUNCTION(glVertex3f);
		UNLOAD_GL_FUNCTION(glVertex3fv);
		UNLOAD_GL_FUNCTION(glVertex3i);
		UNLOAD_GL_FUNCTION(glVertex3iv);
		UNLOAD_GL_FUNCTION(glVertex3s);
		UNLOAD_GL_FUNCTION(glVertex3sv);
		UNLOAD_GL_FUNCTION(glVertex4d);
		UNLOAD_GL_FUNCTION(glVertex4dv);
		UNLOAD_GL_FUNCTION(glVertex4f);
		UNLOAD_GL_FUNCTION(glVertex4fv);
		UNLOAD_GL_FUNCTION(glVertex4i);
		UNLOAD_GL_FUNCTION(glVertex4iv);
		UNLOAD_GL_FUNCTION(glVertex4s);
		UNLOAD_GL_FUNCTION(glVertex4sv);
		UNLOAD_GL_FUNCTION(glVertexPointer);
		UNLOAD_GL_FUNCTION(glViewport);

		// WGL
		UNLOAD_GL_FUNCTION(wglCopyContext);
		UNLOAD_GL_FUNCTION(wglCreateContext);
		UNLOAD_GL_FUNCTION(wglCreateLayerContext);
		UNLOAD_GL_FUNCTION(wglDeleteContext);
		UNLOAD_GL_FUNCTION(wglGetCurrentContext);
		UNLOAD_GL_FUNCTION(wglGetCurrentDC);
		UNLOAD_GL_FUNCTION(wglGetProcAddress);
		UNLOAD_GL_FUNCTION(wglMakeCurrent);
		UNLOAD_GL_FUNCTION(wglShareLists);

		UNLOAD_GL_FUNCTION(wglGetDeviceGammaRamp3DFX);
		UNLOAD_GL_FUNCTION(wglSetDeviceGammaRamp3DFX);
	}

	return;
}

#endif // (UUmPlatform) && (UUmPlatform == UUmPlatform_Win32)

#endif // __ONADA__

