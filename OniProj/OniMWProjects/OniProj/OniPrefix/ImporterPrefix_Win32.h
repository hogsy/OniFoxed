#include <Win32Headers.mch>

#if __MWERKS__ > 0x2310
#pragma old_argmatch on
#endif

	#if !defined(UUmPlatform) && !defined(UUmPlatform_Win32)
		#define UUmPlatform_Win32		1
		#define UUmPlatform				UUmPlatform_Win32

		#include "gl_code_version.h"

		#undef UUmPlatform
		#undef UUmPlatform_Win32

	#else
		#include "gl_code_version.h"

	#endif
/*
 * Turn on for debugging checks and messages
 */
	#define DEBUGGING			1

	#define USE_PROFILE_MEMDEBUG	0
	#define UUmDebugDelete			0

	#define SHIPPING_VERSION	0

	#if SHIPPING_VERSION
	#define STAND_ALONE_ONLY 1
	#else
	#define STAND_ALONE_ONLY 0
	#endif

	#if SHIPPING_VERSION
	#define TOOL_VERSION 0
	#else
	#define TOOL_VERSION 1
	#endif
