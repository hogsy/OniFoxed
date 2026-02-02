/*
	FILE:	Oni_Platform_SDL.c

	PURPOSE: SDL specific code

*/
// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

#include "Oni_Platform.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_video.h>
// ======================================================================
// defines
// ======================================================================
#define ONcMainWindowTitle	("ONI ")

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

FILE*		ONgErrorFile = NULL;

// ======================================================================
// functions
// ======================================================================

// ----------------------------------------------------------------------
static void
ONiPlatform_CreateWindow(
	ONtPlatformData		*ioPlatformData)
{
	//FIXME: displayIndex
	SDL_DisplayMode desktopMode;
	SDL_GetDesktopDisplayMode(0, &desktopMode);
	//TODO: SDL_WINDOW_FULLSCREEN?
	ioPlatformData->gameWindow = SDL_CreateWindow(ONcMainWindowTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, desktopMode.w, desktopMode.h, SDL_WINDOW_OPENGL);

	if (!ioPlatformData->gameWindow)
	{
		// error here
	}
}


// ----------------------------------------------------------------------
UUtError ONrPlatform_Initialize(
	ONtPlatformData			*outPlatformData)
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER);

	ONiPlatform_CreateWindow(outPlatformData);

	SDL_ShowCursor(SDL_FALSE);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtBool
ONrPlatform_IsForegroundApp(
	void)
{
	//TODO: or SDL_GetMouseFocus()?
	return (ONgPlatformData.gameWindow == SDL_GetKeyboardFocus());
}

// ----------------------------------------------------------------------
void ONrPlatform_Terminate(
	void)
{
	if(ONgErrorFile != NULL)
	{
		fclose(ONgErrorFile);
	}

	SDL_Quit();
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
