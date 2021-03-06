// ======================================================================
// BFW_CL_Platform_Win32.h
// ======================================================================
#ifndef BFW_CL_PLATFORM_WIN32_H
#define BFW_CL_PLATFORM_WIN32_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

// ======================================================================
// prototypes
// ======================================================================
UUtInt16
CLrPlatform_GetCommandLine(
	int				inArgc,
	const char*		inArgv[],
	char			***outArgV);

// ======================================================================
#endif /* BFW_CL_PLATFORM_WIN32_H */