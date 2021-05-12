/*
 * This is special for MacOS
 */
	#include <MacHeadersPPC>

/*
 * Turn on for debugging checks and messages
 */
	#define DEBUGGING			0

	#define UUmDebugDelete			0
	
	#define PROFILE	0

	#define UUmDebuggerSymFileName	"MacOSOpt_Imp.xSYM"

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
