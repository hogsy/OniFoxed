// ======================================================================
// BFW_CommandLine.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW_CommandLine.h"

#if UUmPlatform == UUmPlatform_Mac

	#include "BFW_CL_Platform_MacOS.h"

#elif UUmPlatform == UUmPlatform_Win32

	#include "BFW_CL_Platform_Win32.h"

#endif

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
UUtInt16
CLrGetCommandLine(
	int				inArgc,
	char*			inArgv[],
	char			***outArgV)
{
#if UUmSDL
	//TODO: SDL startup dialog window?
	//      Mac can set up bindings I guess, dunno what the Win32 one offers
	*outArgV = inArgv;
	return inArgc;
#elif UUmPlatform == UUmPlatform_Mac
	return CLrPlatform_GetCommandLine(outArgV);
#elif UUmPlatform == UUmPlatform_Win32
	return CLrPlatform_GetCommandLine(inArgc, inArgv, outArgV);
#else
	#error Unknown platform
#endif
}
