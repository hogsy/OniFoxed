#if UUmPlatform == UUmPlatform_Win32 /* allows file to be included in macintosh version of project */

/*
rasterizer_opengl_windows.c
Tuesday, June 22, 1999
*/

#ifndef __ONADA__

#include "BFW.h"
#include "GL_Platform.h"
#include "GL_DC_Private.h"
#include "Oni_Platform.h"

#include <ddraw.h>

static HDC gDeviceContext;
static HGLRC gRenderContext;
static LPDIRECTDRAW gDirectDraw;
static LPDIRECTDRAW2 gDirectDraw2;

/* ---------- private prototypes */

void make_pixel_format_descriptor(PIXELFORMATDESCRIPTOR *pfd, UUtInt32 inBitDepth);


boolean available_opengl_platform(
	void)
{
	static boolean first_time= TRUE;
	static boolean has_opengl= FALSE;
		
	if (first_time)
	{
		PIXELFORMATDESCRIPTOR pfd;
		HDC device_context;
		short pixel_format;
		
		first_time= FALSE;
				
		memset(&pfd,0,sizeof(PIXELFORMATDESCRIPTOR));
		
		pfd.nSize= sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion= 1;
		pfd.dwFlags= PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		
		UUmAssert(ONgPlatformData.gameWindow && (ONgPlatformData.gameWindow != INVALID_HANDLE_VALUE));
		device_context= GetDC(ONgPlatformData.gameWindow);
		UUmAssert(device_context);
		
		pixel_format= ChoosePixelFormat(device_context,&pfd);
		
		if (
			DescribePixelFormat(device_context,pixel_format,sizeof(PIXELFORMATDESCRIPTOR),&pfd) &&
			(pfd.dwFlags & PFD_DRAW_TO_WINDOW) &&
			(pfd.dwFlags & PFD_SUPPORT_OPENGL) &&
			(pfd.dwFlags & PFD_DOUBLEBUFFER) &&
			((pfd.dwFlags & PFD_GENERIC_ACCELERATED) || !(pfd.dwFlags & PFD_GENERIC_FORMAT))
		) {
			has_opengl= TRUE;
		}
		
		ReleaseDC(ONgPlatformData.gameWindow,device_context);
	}
	
	return has_opengl;
}

static UUtBool fullscreenCandidate(HWND hwnd) {
    RECT  winRect;
    RECT  dtRect;
    UUtBool status = UUcFalse;
    DWORD style = GetWindowLong (hwnd, GWL_STYLE);

    // Quake II sets   (WS_POPUP | WS_VISIBLE | 
    //                  WS_CLIPSIBLINGS)
    // Heretic II sets (WS_POPUP | WS_VISIBLE | 
    //                  WS_CLIPSIBLINGS | 
    //                  WS_SYSMENU)
    // TrueSpace4 sets (WS_POPUP | WS_VISIBLE | 
    //                  WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
    //                  WS_BORDER |
    //                  WS_SYSMENU | WS_THICKFRAME)
    // This function needs to 
    // return 0 for TrueSpace4, which wants a window and 
    // return 1 for Quake II and Heretic II, which want fullscreen

    if ( (style & WS_POPUP) && !(style & (WS_BORDER | WS_THICKFRAME)))
	{
        GetClientRect( hwnd, &winRect );
        GetWindowRect( GetDesktopWindow(), &dtRect );
        if ( !memcmp( &winRect, &dtRect, sizeof( RECT ) ) ) {
            status = UUcTrue;
        }
    }

    return status;
}

static DEVMODE original_display_mode;
 
static void GLrSetDisplaySettings(UUtInt32 width, UUtInt32 height, UUtInt32 depth)
{
	BOOL success;
	DEVMODE desired_display_mode;

	original_display_mode.dmSize = sizeof(DEVMODE);
	original_display_mode.dmDriverExtra = 0;

	success = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &original_display_mode);
	UUmAssert(success);

    desired_display_mode.dmSize = sizeof(DEVMODE);
    desired_display_mode.dmBitsPerPel = depth;
    desired_display_mode.dmPelsWidth = width;
    desired_display_mode.dmPelsHeight = height;
    desired_display_mode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
    desired_display_mode.dmDriverExtra = 0;
	
	if (M3gResolutionSwitch) {
		LONG result;

		result = ChangeDisplaySettings(&desired_display_mode, 0);
		UUmAssert(DISP_CHANGE_SUCCESSFUL == result);
	}

	return;
}

static void GLrRestoreDisplaySettings(void)
{
	ChangeDisplaySettings(&original_display_mode, 0);
}


UUtError
GLrPlatform_Initialize(
	void)
{
	BOOL success;
	PIXELFORMATDESCRIPTOR pfd;
//	HRESULT result;
	short pixel_format;
	UUtInt32 width = GLgDrawContextPrivate->width;
	UUtInt32 height = GLgDrawContextPrivate->height;
	UUtInt32 depth = GLgDrawContextPrivate->bitDepth;

	depth = 32;
	GLrSetDisplaySettings(width, height, depth);

	SetWindowPos(
		ONgPlatformData.gameWindow,
		HWND_TOP,
		0,
		0,
		GLgDrawContextPrivate->width,
		GLgDrawContextPrivate->height,
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);

	if (M3gResolutionSwitch) {
		UUmAssert(fullscreenCandidate(ONgPlatformData.gameWindow));
	}
	
	UUmAssert(ONgPlatformData.gameWindow && (ONgPlatformData.gameWindow != INVALID_HANDLE_VALUE));
	gDeviceContext= GetDC(ONgPlatformData.gameWindow);
	UUmAssert(gDeviceContext);

	make_pixel_format_descriptor(&pfd, GLgDrawContextPrivate->bitDepth);
	pixel_format= ChoosePixelFormat(gDeviceContext, &pfd);
	UUmAssert(pixel_format);

	success = SetPixelFormat(gDeviceContext, pixel_format, &pfd);
	UUmAssert(success);

	DescribePixelFormat(gDeviceContext, pixel_format, sizeof(pfd), &pfd);

	UUrStartupMessage("opengl color bits = %d", pfd.cColorBits);
	UUrStartupMessage("opengl depth bits = %d", pfd.cDepthBits);

	if (pfd.dwFlags & PFD_DRAW_TO_WINDOW) {
		UUrStartupMessage("PFD_DRAW_TO_WINDOW");
	}

	if (pfd.dwFlags & PFD_DRAW_TO_BITMAP) {
		UUrStartupMessage("PFD_DRAW_TO_BITMAP");
	}

	if (pfd.dwFlags & PFD_SUPPORT_GDI) {
		UUrStartupMessage("PFD_SUPPORT_GDI");
	}

	if (pfd.dwFlags & PFD_SUPPORT_OPENGL) {
		UUrStartupMessage("PFD_SUPPORT_OPENGL");
	}

	if (pfd.dwFlags & PFD_GENERIC_ACCELERATED) {
		UUrStartupMessage("PFD_GENERIC_ACCELERATED");
	}

	if (pfd.dwFlags & PFD_GENERIC_FORMAT) {
		UUrStartupMessage("PFD_GENERIC_FORMAT");
	}

	if (pfd.dwFlags & PFD_NEED_PALETTE) {
		UUrStartupMessage("PFD_NEED_PALETTE");
	}

	if (pfd.dwFlags & PFD_NEED_SYSTEM_PALETTE) {
		UUrStartupMessage("PFD_NEED_SYSTEM_PALETTE");
	}

	if (pfd.dwFlags & PFD_DOUBLEBUFFER) {
		UUrStartupMessage("PFD_DOUBLEBUFFER");
	}

	if (pfd.dwFlags & PFD_STEREO) {
		UUrStartupMessage("PFD_STEREO");
	}

	if (pfd.dwFlags & PFD_SWAP_LAYER_BUFFERS) {
		UUrStartupMessage("PFD_SWAP_LAYER_BUFFERS");
	}

	if (pfd.dwFlags & PFD_SWAP_COPY) {
		UUrStartupMessage("PFD_SWAP_COPY");
	}

	if (pfd.dwFlags & PFD_SWAP_EXCHANGE) {
		UUrStartupMessage("PFD_SWAP_EXCHANGE");
	}


	gRenderContext = wglCreateContext(gDeviceContext);
	UUmAssert(gRenderContext);

	success = wglMakeCurrent(gDeviceContext, gRenderContext);
	UUmAssert(success);

	return UUcError_None;
}


void
GLrPlatform_Dispose(
	void)
{
	UUtInt16 screen_width, screen_height;
	BOOL success;

	UUmAssert(gDeviceContext);
	success = wglMakeCurrent(NULL, NULL);
	UUmAssert(success);

	UUmAssert(gRenderContext);
	success = wglDeleteContext(gRenderContext);
	UUmAssert(success);
	
	UUmAssert(ONgPlatformData.gameWindow && (ONgPlatformData.gameWindow != INVALID_HANDLE_VALUE));
	ReleaseDC(ONgPlatformData.gameWindow,gDeviceContext);

	GLrRestoreDisplaySettings();

	screen_width = GetSystemMetrics(SM_CXSCREEN);
	screen_height = GetSystemMetrics(SM_CYSCREEN);

	SetWindowPos(
		ONgPlatformData.gameWindow,
		HWND_TOP,
		0,
		0,
		screen_width,
		screen_height,
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	
	return;	
}


void
GLrPlatform_DisplayBackBuffer(
	void)
{
	UUmAssert(gDeviceContext);
	SwapBuffers(gDeviceContext);
}



/* ---------- private functions */

void make_pixel_format_descriptor(PIXELFORMATDESCRIPTOR *pfd, UUtInt32 bitDepth)
{
	UUmAssert(pfd);

	memset(pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));

	pfd->nSize= sizeof(PIXELFORMATDESCRIPTOR);
	pfd->nVersion= 1;
	pfd->dwFlags= PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_SWAP_EXCHANGE;
	pfd->cColorBits = 32;
	// pfd->cColorBits = (16 == bitDepth) ? 16 : 24;

	return;
}



#endif // __ONADA__

#endif /* #ifdef windows */
