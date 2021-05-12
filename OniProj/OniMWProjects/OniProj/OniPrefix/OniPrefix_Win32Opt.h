#include <Win32Headers.mch>

#pragma cpp_extensions on

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
	#define DEBUGGING			0

	#define USE_PROFILE_MEMDEBUG	0
	#define UUmDebugDelete			0
	
	#define _WIN32


// set the DirectX headers to the versions we want to use

#define DIRECTINPUT_VERSION			0x0500
#define DIRECTDRAW_VERSION			0x0500
#define DIRECT3D_VERSION			0x0500

	#define SHIPPING_VERSION	0