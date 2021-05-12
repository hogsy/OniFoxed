/*
 * This is special for MacOS
 */
	#include <MacHeadersCarbon.h>

	#define CALL_IN_SPOCKETS_BUT_NOT_IN_CARBON 1 // this lets us use Sprockets for OS8/9 and still build a Carbonized app

/*
 * Turn on for debugging checks and messages
 */
	#define DEBUGGING			0

	#define MSmPixelTouch		0

	#define UUmDebuggerSymFileName	"Oni.xSYM"

	#define UUmPasswordProtect	0
	
	#if !defined(UUmPlatform) && !defined(UUmPlatform_Mac)
		#define UUmPlatform_Mac			2
		#define UUmPlatform				UUmPlatform_Mac
	
		#include "gl_code_version.h"
	
		#undef UUmPlatform
		#undef UUmPlatform_Mac
	
	#else
		#include "gl_code_version.h"

	#endif

	#define SHIPPING_VERSION	1

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

	