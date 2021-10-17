/*
	FILE:	Oni_Platform_Win32.c
	
	AUTHOR:	Brent H. Pease, Michael Evans, Kevin Armstrong
	
	CREATED: May 26, 1997
	
	PURPOSE: Win32 specific code
	
	Copyright 1997, 2000

*/
// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

#include "BFW_Motoko.h"
#include "BFW_LocalInput.h"
#include "BFW_AppUtilities.h"
#include "BFW_LI_Platform_Win32.h"

#include "Oni.h"

#include "Oni_Platform.h"

#include "BFW_Console.h"

// ======================================================================
// defines
// ======================================================================
#define ONcMainWindowClass	TEXT("ONI ")
#define ONcMainWindowTitle	TEXT("ONI ")

#define	ONcSurface_Width	MScScreenWidth
#define ONcSurface_Height	MScScreenHeight

#define ONcSurface_Left	0
#define ONcSurface_Top	0

#if defined(DEBUGGING) && DEBUGGING

	#define DEBUG_AKIRA 1

#endif

// ======================================================================
// globals
// ======================================================================

HINSTANCE	ONgAppHInstance = NULL;
int			ONgICmdShow;
FILE*		ONgErrorFile = NULL;
UUtBool		ONgShiftDown = UUcFalse;

// ======================================================================
// prototypes
// ======================================================================
void UUcExternal_Call main(int argc, char *argv[]);

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
ONiHandleMouseEvent(
	UINT	iMsg,
	WPARAM	wParam,
	LPARAM	lParam)
{
	LItInputEventType		event_type;
	IMtPoint2D				mouse_pos;
	
	switch (iMsg)
	{
		case WM_MOUSEMOVE:		event_type = LIcInputEvent_MouseMove;		break;
		case WM_LBUTTONDOWN:	event_type = LIcInputEvent_LMouseDown;		break;
		case WM_LBUTTONDBLCLK:	event_type = LIcInputEvent_LMouseDown;		break;
		case WM_LBUTTONUP:		event_type = LIcInputEvent_LMouseUp;		break;
		case WM_RBUTTONDOWN:	event_type = LIcInputEvent_RMouseDown;		break;
		case WM_RBUTTONDBLCLK:	event_type = LIcInputEvent_RMouseDown;		break;
		case WM_RBUTTONUP:		event_type = LIcInputEvent_RMouseUp;		break;
		case WM_MBUTTONDOWN:	event_type = LIcInputEvent_MMouseDown;		break;
		case WM_MBUTTONDBLCLK:	event_type = LIcInputEvent_MMouseDown;		break;
		case WM_MBUTTONUP:		event_type = LIcInputEvent_MMouseUp;		break;
	}
	
	mouse_pos.x = LOWORD(lParam);
	mouse_pos.y = HIWORD(lParam);
	LIrInputEvent_Add(event_type, &mouse_pos, 0, wParam);
}	

// ----------------------------------------------------------------------
static LRESULT CALLBACK ONiPlatform_WindowProc(
	HWND	hWind,
	UINT	iMsg,
	WPARAM	wParam,
	LPARAM	lParam)
{
	LRESULT				result;
	static UUtUns32		modifiers = 0;
	static UUtUns32		previous_key = 0;
	static UUtBool		add_char;
	
	switch(iMsg)
	{
		case WM_CHAR:
			if (add_char)
			{
				if ((modifiers & MK_CONTROL) && (previous_key != 0))
				{
					LIrInputEvent_Add(LIcInputEvent_KeyDown, NULL, previous_key, modifiers);
				}
				else
				{
					LIrInputEvent_Add(LIcInputEvent_KeyDown, NULL, (UUtUns32)wParam, modifiers);
				}
			}
		break;
		
		case WM_KEYDOWN:
			add_char = UUcFalse;
			
			switch(wParam)
			{
				case VK_SHIFT:
					modifiers |= MK_SHIFT;
				break;
				
				case VK_CONTROL:
					modifiers |= MK_CONTROL;
				break;
				
				case VK_TAB:
				case VK_UP:
				case VK_DOWN:
				case VK_LEFT:
				case VK_RIGHT:
				case VK_PRIOR:
				case VK_NEXT:
				case VK_END:
				case VK_HOME:
				case VK_INSERT:
				case VK_DELETE:
				case VK_F1:
				case VK_F2:
				case VK_F3:
				case VK_F4:
				case VK_F5:
				case VK_F6:
				case VK_F7:
				case VK_F8:
				case VK_F9:
				case VK_F10:
				case VK_F11:
				case VK_F12:
					LIrInputEvent_Add(
						LIcInputEvent_KeyDown,
						NULL,
						LIrPlatform_Win32_TranslateVirtualKey(wParam),
						modifiers);
				break;
				
				default:
					add_char = UUcTrue;
					previous_key = LIrPlatform_Win32_TranslateVirtualKey(wParam);
				break;
			}
		break;
		
		case WM_KEYUP:
			switch(wParam)
			{
				case VK_SHIFT:
					modifiers &= ~MK_SHIFT;
				break;
				
				case VK_CONTROL:
					modifiers &= ~MK_CONTROL;
				break;
			}
		break;
		
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
		case WM_MBUTTONUP:
			ONiHandleMouseEvent(iMsg, wParam, lParam);
		return 0;
				
		case WM_ACTIVATE:
		{
			if (LOWORD(wParam) == WA_INACTIVE)
			{		
				// set the current mode to normal input
				LIrGameIsActive(UUcFalse);
			}
			else
			{
				// restore the previous mode
				LIrGameIsActive(UUcTrue);
				modifiers = 0;
			}
		}
		return 0;
		
		case WM_PAINT:
		{
			PAINTSTRUCT			ps;
			HDC					hdc;
			
			hdc = BeginPaint(hWind, &ps);
			PatBlt(hdc, 0, 0, 4096, 4096, BLACKNESS);
			EndPaint(hWind, &ps);
      	}
      	return 0;
	}

	result = DefWindowProc(hWind, iMsg, wParam, lParam);

	return result;
}

static UUtBool fullscreenCandidate(HWND hwnd) {
    RECT  winRect;
    RECT  dtRect;
    UUtBool status = UUcFalse;
    DWORD style = GetWindowLong (hwnd, GWL_STYLE);

    // Quake II sets   (WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS)
    // Heretic II sets (WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_SYSMENU)
    // TrueSpace4 sets (WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER | WS_SYSMENU | WS_THICKFRAME)
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


// ----------------------------------------------------------------------
static void
ONiPlatform_CreateWindow(
	ONtPlatformData		*ioPlatformData)
{
	WNDCLASSEX	windClass;
	ATOM		atom;
	UUtUns16	screen_width;
	UUtUns16	screen_height;

	if (FindWindow(ONcMainWindowClass, ONcMainWindowTitle) != NULL)
	{
		AUrMessageBox(AUcMBType_OK, "There is already an instance of the game running.");
		exit(0);
	}
	
	windClass.cbSize = sizeof(windClass);
	windClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	windClass.lpfnWndProc = ONiPlatform_WindowProc;
	windClass.cbClsExtra = 0;
	windClass.cbWndExtra = 0;
	windClass.hInstance = ioPlatformData->appInstance;
	windClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	windClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windClass.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH/*WHITE_BRUSH*/);
	windClass.lpszMenuName = NULL;
	windClass.lpszClassName = ONcMainWindowClass;
	windClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	atom = RegisterClassEx(&windClass);

	// get the width and height of the screen
	screen_width = GetSystemMetrics(SM_CXSCREEN);
	screen_height = GetSystemMetrics(SM_CYSCREEN);

	if (0 != atom)
	{
		ioPlatformData->gameWindow =
			CreateWindowEx(
				WS_EX_LEFT,
				ONcMainWindowClass,
				ONcMainWindowTitle,
				WS_POPUP,
				ONcSurface_Left,
				ONcSurface_Top,
				screen_width,
				screen_height,
				NULL,
				NULL,
				ioPlatformData->appInstance,
				NULL);

		ShowWindow(ioPlatformData->gameWindow, 1);
		UpdateWindow(ioPlatformData->gameWindow);

		UUmAssert(fullscreenCandidate(ioPlatformData->gameWindow));

		//	UpdateWindow(ioPlatformData->gameWindow);
		
		#if 0 //defined(DEBUG_AKIRA) && DEBUG_AKIRA

			ioPlatformData->akiraWindow =
				CreateWindowEx(
					WS_EX_LEFT,
					ONcMainWindowClass,
					ONcMainWindowTitle,
					WS_POPUP,
					ONcSurface_Width,
					0,
					512,
					512,
					NULL,
					NULL,
					ioPlatformData->appInstance,
					NULL);
		
			ShowWindow(ioPlatformData->akiraWindow, 1);
		
		#endif

	}
	else
	{
		// error here
	}
}


// ----------------------------------------------------------------------
UUtError ONrPlatform_Initialize(
	ONtPlatformData			*outPlatformData)
{
	HRESULT			ddReturn = DD_OK;
	
	outPlatformData->appInstance = ONgAppHInstance;
	
	ONiPlatform_CreateWindow(outPlatformData);

	ShowCursor(FALSE);	// FALSE = hide the cursor
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtBool
ONrPlatform_IsForegroundApp(
	void)
{
	return (ONgPlatformData.gameWindow == GetForegroundWindow());
}

// ----------------------------------------------------------------------
void ONrPlatform_Terminate(
	void)
{
	if(ONgErrorFile != NULL)
	{
		fclose(ONgErrorFile);
	}
}

// ----------------------------------------------------------------------
void ONrPlatform_Update(
	void)
{
}

// ----------------------------------------------------------------------
void ONrPlatform_ErrorHandler(
	UUtError			theError,
	char				*debugDescription,
	UUtInt32			userDescriptionRef,
	char				*message)
{

	if(ONgErrorFile == NULL)
	{
		ONgErrorFile = UUrFOpen("oniErr.txt", "wb");

		if(ONgErrorFile == NULL)
		{
			/* XXX - Someday bitch really loudly */
		}
	}
	
	fprintf(ONgErrorFile, "InternalError: %s, %s\n\r", debugDescription, message);
}

void
ONrPlatform_CopyAkiraToScreen(
	UUtUns16	inBufferWidth,
	UUtUns16	inBufferHeight,
	UUtUns16	inRowBytes,
	UUtUns16*	inBaseAdddr);
	
void
ONrPlatform_CopyAkiraToScreen(
	UUtUns16	inBufferWidth,
	UUtUns16	inBufferHeight,
	UUtUns16	inRowBytes,
	UUtUns16*	inBaseAdddr)
{

}

#ifdef ONI_MAP_FILE
extern int handle_exception(LPEXCEPTION_POINTERS);
extern void stack_walk_initialize(void);
#endif

// ----------------------------------------------------------------------
int WINAPI WinMain(
	HINSTANCE	hInstance,
	HINSTANCE	hPrevInstance,
	PSTR		ssCmdLine,
	int			iCmdShow)
{
//	MSG	msg;
#define iMaxArguments 20
	int argc;
	char *argv[iMaxArguments + 1];

#ifdef ONI_MAP_FILE
	stack_walk_initialize();
#endif

	ONgAppHInstance = hInstance;
	ONgICmdShow = iCmdShow;

	argv[0] = "";
	AUrBuildArgumentList(ssCmdLine, iMaxArguments, (UUtUns32*)&argc, argv + 1);

#if (UUmPlatform != UUmPlatform_Win32) || (defined(DEBUGGING) && DEBUGGING)

	#ifdef ONI_MAP_FILE
	__try {
	#endif

	main(argc + 1, argv);

	#ifdef ONI_MAP_FILE
	} __except (handle_exception(GetExceptionInformation())) {}
	#endif

#else
	{
		DEVMODE original_display_mode;
		BOOL success;

		original_display_mode.dmSize = sizeof(DEVMODE);
		original_display_mode.dmDriverExtra = 0;

		success = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &original_display_mode);
		UUmAssert(success);

	// FIXME: add SEH for mingw
	#ifdef _MSC_VER
		__try
	#endif
		{
			main(argc + 1, argv);
		}
	#ifdef _MSC_VER
	#ifdef ONI_MAP_FILE
		__except (handle_exception(GetExceptionInformation())) {
	#else
		__except (1) {
	#endif
			ChangeDisplaySettings(&original_display_mode, 0);
			MessageBox(NULL, "Blam, Oni crashed", "damn!", MB_OK);
		}
	#endif
	}
#endif

	return 0;
}
