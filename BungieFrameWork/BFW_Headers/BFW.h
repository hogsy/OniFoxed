#pragma once
/*
	FILE:	BFW.h
	
	AUTHOR:	Brent H. Pease, Michael Evans
	
	CREATED: May 17, 1997
	
	PURPOSE: Utilities for platform, processor, memory, error, and assertions
	
	Copyright 1997, 2000

*/
#ifndef BFW_H
#define BFW_H

#include <stdlib.h>
#include <stdio.h> /* XXX - Get rid of me */
#include <string.h>
// S.S. #include <math.h>
#include <float.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

	#ifndef SHIPPING_VERSION
	#define SHIPPING_VERSION 0
	#endif

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

	#if SHIPPING_VERSION
	#define PERFORMANCE_TIMER 0
	#else
	#define PERFORMANCE_TIMER 1
	#endif

	#ifndef THE_DAY_IS_MINE

	#if TOOL_VERSION
	#define THE_DAY_IS_MINE 1
	#else
	#define THE_DAY_IS_MINE 0
	#endif

	#endif

	#ifndef CONSOLE_DEBUGGING_COMMANDS

	#if THE_DAY_IS_MINE
	#define CONSOLE_DEBUGGING_COMMANDS 1
	#else
	#define CONSOLE_DEBUGGING_COMMANDS 0
	#endif

	#endif



/*
 * The possible platforms, processors, compilers, and endiannesses.
 */
	#define UUmPlatform_Win32		1
	#define UUmPlatform_Mac			2
	#define UUmPlatform_Linux		3
	
	#define UUmProcessor_Pentium	1
	#define UUmProcessor_PPC		2
	
	#define UUmCompiler_VisC		1
	#define UUmCompiler_MWerks		2
	#define UUmCompiler_Moto		3
	#define UUmCompiler_Watcom		4
	#define UUmCompiler_GCC			5
	
	#define UUmEndian_Little		0
	#define UUmEndian_Big			1
	
	#define UUmSIMD_None			0
	#define UUmSIMD_AltiVec			1
	#define UUmSIMD_Intel			2
	
/*
 * Try to determine the correct platform, processor, and compiler settings if
 * they are not given.
 */
	#if !defined(UUmPlatform)
		#if (defined(__MWERKS__) && defined(__POWERPC__)) || defined(__MRC__) || defined(__MOTO__)
			#define UUmPlatform	UUmPlatform_Mac
		#elif (defined(__MWERKS__) && defined(__INTEL__)) || defined(_MSC_VER) || defined(__WATCOMC__)
			#define UUmPlatform	UUmPlatform_Win32
		#elif defined(__linux__)
			#define UUmPlatform	UUmPlatform_Linux
		#else
			#error Unknown platform - please specify and then do a search on UUmPlatform to add the needed cases
		#endif
	#endif
	
	#if !defined(UUmProcessor)
		#if (defined(__MWERKS__) && defined(__POWERPC__)) || defined(__MRC__) || defined(__MOTO__) || defined(ppc) || defined(__ppc__) || defined(__ppc)
			#define UUmProcessor	UUmProcessor_PPC
			
			#if defined(__ALTIVEC__) && defined(__VEC__)
				#define UUmSIMD		UUmSIMD_AltiVec
			#else
				#define UUmSIMD		UUmSIMD_None
			#endif
			
		#elif defined(i386) || defined(__i386__)
			#define UUmProcessor	UUmProcessor_Pentium
			#define UUmSIMD			UUmSIMD_None

		#elif (defined(__MWERKS__) && defined(__INTEL__)) || defined(_MSC_VER) || defined(__WATCOMC__)
			#define UUmProcessor	UUmProcessor_Pentium
			#define UUmSIMD			UUmSIMD_None
		#else 
			#error Unknown processor - please specify and then do a search on UUmProcessor to add the needed cases
		#endif
	#endif
	
	#if !defined(UUmCompiler)
		#if defined(__MWERKS__)
			#define UUmCompiler	UUmCompiler_MWerks
		#elif defined(_MSC_VER)
			#define UUmCompiler	UUmCompiler_VisC
		#elif defined(__MOTO__)
			#define UUmCompiler	UUmCompiler_Moto
		#elif defined(__WATCOMC__)
			#define UUmCompiler	UUmCompiler_Watcom
		#elif defined(__MRC__)
			#define UUmCompiler	UUmCompiler_MrC
		#elif defined(__GNUC__)
			#define UUmCompiler	UUmCompiler_GCC
		#else
			#error Unknown compiler - please specify and then do a search on UUmCompiler to add the needed cases
		#endif
	#endif
	
	#if !defined(UUmEndian)
		#if UUmProcessor == UUmProcessor_PPC
			#define UUmEndian	UUmEndian_Big
		#elif UUmProcessor == UUmProcessor_Pentium
			#define UUmEndian	UUmEndian_Little
		#else
			#error Could not automatically determine endianness, please specify manually
		#endif
	#endif

/*
 * Define the processor specific macros
 *
 * Things defined in this file
 *	1. UUcProcessor_CacheLineSize - A hint at the cache line size. While this is required to be defined it does not 
 *		necessarily need to be exactly correct. A correct value would lead to higher performance.
 *	2. UUrProcessor_ZeroCacheLine - An instruction to zero a cache line. This is a hint to help memory bandwidth
 */
	#if UUmProcessor == UUmProcessor_Pentium
		#define UUcProcessor_CacheLineBits	(5)
		#define UUrProcessor_ZeroCacheLine(x, y)
	#elif UUmProcessor == UUmProcessor_PPC
		#define UUcProcessor_CacheLineBits	(5)
                #ifdef __GNUC__
                    #define UUrProcessor_ZeroCacheLine(x, y)  asm volatile ("dcbz %0,%1" : : "b" (x), "r" (y) : "memory")
                #else
                    #define UUrProcessor_ZeroCacheLine(x, y)	__dcbz(x, y)
                #endif
	#else
		#error unknown processor
	#endif

	#define UUcProcessor_CacheLineSize		(1 << UUcProcessor_CacheLineBits)
	#define UUcProcessor_CacheLineSize_Mask	((UUcProcessor_CacheLineSize) - 1)


// change compiler behavior
#if UUmCompiler	== UUmCompiler_VisC

#pragma warning( 1 : 4003)			// not enough actual parameters for macro 
#pragma warning( 1 : 4018)			// signed/unsigned mismatch
#pragma warning( error	 : 4013)	// undefined; assuming extern returning int
#pragma warning( error   : 4020)	// too many actual parameters
#pragma warning( error   : 4022)	// pointer mismatch for actual parameter
#pragma warning( error	 : 4024)	// different types for formal and actual parameter
#pragma warning( 1       : 4035)	// no return value
#pragma warning( error   : 4087)	// passing parameters to a function of type void
#pragma warning( disable : 4100)	// unreferenced formal parameter
#pragma warning( 1 : 4101)			// unused local variable
#pragma warning( 1 : 4113)			// differing parameter lists
#pragma warning( disable : 4115)	// named type definition in parentheses
#pragma warning( disable : 4127)	// conditional expression is constant
#pragma warning( error   : 4133)	// 'initializing' : incompatible types
#pragma warning( 1 : 4146)			// unary minus operator applied to unsigned type, result still unsigned
//#pragma warning( 1 : 4189)			// local variable is initialized but not referenced
#pragma warning( disable : 4201)	// nonstandard extension: nameless struct/union
#pragma warning( 1 : 4211)			// nonstandard extension used : redefined extern to static

#pragma warning( disable : 4214)	// nonstandard extension: bit field types other than int
// #pragma warning( 1 : 4244)			// conversion (possible loss of data)
#pragma warning( 1 : 4305)			// '=' : truncation from 'const int ' to 'char '
//#pragma warning( 1 : 4310)			// cast truncates constant value
#pragma warning( disable : 4505)	// unrefenced local function
#pragma warning( disable : 4514)	// unreferenced inline function
#pragma warning( error : 4700)		// local variable used without having been initialized
//#pragma warning(1 : 4701)			// local variable may be used without having been initialized
#pragma warning( disable : 4761)	// integral size mismatch




#endif

/*
 * Include platform and system header files
 */
	#if UUmSDL
		#include <SDL2/SDL_assert.h>
		#include <SDL2/SDL_video.h>
		#include <SDL2/SDL_platform.h>
	#elif UUmPlatform == UUmPlatform_Win32
		#if UUmCompiler	!= UUmCompiler_MWerks	
			#include <windows.h>
		#endif

		#include <WTYPES.h>	
		#include <ddraw.h>

		#define DDPIXELFORMAT_dwRGBBitCount(x) (*(((DWORD *)&x) + 3))
	#endif

#if UUmCompiler	== UUmCompiler_VisC
	#define UUcInline __inline
#elif UUmCompiler == UUmCompiler_MWerks 
	#define UUcInline inline
#elif UUmCompiler == UUmCompiler_GCC
        #define UUcInline inline
#else
	#define UUcInline
#endif


#if UUmPlatform == UUmPlatform_Win32
	#define UUcArglist_Call		__cdecl
	#define UUcExternal_Call	__cdecl
#else
	#define UUcArglist_Call		
	#define UUcExternal_Call	
#endif

/*
 * ============================= Macros used for template definitions =============================
 */

	/*
	 * Used to replace the "struct" keyword when defining a new template
	 */
		#define tm_template(c1, c2, c3, c4, string)	struct
	
	/*
	 * Used to replace the struct keyword when defining a struct to be used within a template
	 */
		#define tm_struct	struct
	
	/*
	 * Used to replace the enum keyword when defining a enum to be used within a template
	 */
		#define tm_enum		enum
		
	/*
	 * Used to declare a template field that will be the index to a variable length array.
	 * *** This must immediately preceed a tm_vararray declaration
	 */
		#define tm_varindex
	
	/*
	 * Used to declare a variable length array. *** This must be the last field
	 */
		#define tm_vararray
	
	/*
	 * This is used to pad a template declaration so the data is cache aligned.
	 * "x" is typically the size of the var array index type so that the variable
	 * array data is cache aligned
	 */
		#define tm_pad			char

	/*
	 * This is used to indicate that this field can point to any type of template
	 */
		#define tm_templateref	void*

	/*
	 * This is a pointer to raw data that is not byteswapped or touched in anyway
	 */

		#define tm_raw(type)	type

	/*
	 * This is an offset to binary data stored in a separate companion file
	 */

		#define tm_separate		UUtUns32

/*
 * ============================= Useful typedefs =============================
 */
 
#if UUmCompiler_GCC
	#include <stdint.h>

	typedef int8_t				UUtInt8;
	typedef uint8_t				UUtUns8;
	typedef int16_t				UUtInt16;
	typedef uint16_t			UUtUns16;
	typedef int32_t				UUtInt32;
	typedef uint32_t			UUtUns32;
	typedef int64_t				UUtInt64;
	typedef uint64_t			UUtUns64;

	#define UUmFS_UUtUns64		"%qu"
	#define UUmFS_UUtInt64		"%qd"
#else
	typedef unsigned long		UUtUns32;
	typedef long				UUtInt32;
	typedef unsigned short		UUtUns16;
	typedef short				UUtInt16;
	typedef unsigned char  		UUtUns8;
	typedef signed char			UUtInt8;

#if UUmCompiler == UUmCompiler_MWerks || UUmCompiler == UUmCompiler_MrC

	typedef long long			UUtInt64;
	typedef unsigned long long	UUtUns64;
	
	#define UUmFS_UUtUns64		"%llu"
	#define UUmFS_UUtInt64		"%lld"

#elif UUmCompiler == UUmCompiler_VisC

	typedef LONGLONG			UUtInt64;
	typedef ULONGLONG			UUtUns64;

	#define UUmFS_UUtUns64		"%I64u"
	#define UUmFS_UUtInt64		"%I64"

#endif
#endif

	typedef UUtUns8				UUtBool;
	typedef UUtUns16			UUtError;

	typedef tm_struct UUtRect
	{
		UUtInt16		left;
		UUtInt16		top;
		UUtInt16		right;
		UUtInt16		bottom;		
	} UUtRect;

/*
 * ============================= Useful constants =============================
 */

	#define UUcUns8Bits		 8 // yes very useful :o)
	#define UUcUns12Bits	12
	#define UUcUns16Bits	16
	#define UUcUns32Bits	32

	#define UUcMaxInt8		((UUtInt8)  0x7f)
	#define UUcMaxInt12		((UUtInt16) 0x7ff)
	#define UUcMaxInt16		((UUtInt16) 0x7fff)
	#define UUcMaxInt32		((UUtInt32) 0x7fffffff)
	#define UUcMaxUns8		((UUtUns8)  0xff)
	#define UUcMaxUns12		((UUtUns12) 0xfff)
	#define UUcMaxUns16		((UUtUns16) 0xffff)
	#define	UUcMaxUns32		((UUtUns32) 0xffffffff)

	#define UUcMinInt8		((UUtInt8)  0x80)
	#define UUcMinInt12		((UUtInt16) -2048)
	#define UUcMinInt16		((UUtInt16) 0x8000)
	#define UUcMinInt32		((UUtInt32) 0x80000000)
	#define UUcMinUns8		(0)
	#define UUcMinUns12		(0)
	#define UUcMinUns16		(0)
	#define	UUcMinUns32		(0)


 	#define UUcTrue		((UUtBool)(1))
	#define UUcFalse	((UUtBool)(0))

/*
 * ============================= Useful macros =============================
 */

	#define UUmBlankFunction do { } while (0)
	#define UUmSign(x) ((x)?((x)<0?-1:1):0)
	#define UUmFabs(x) ((float) fabs(x))
	#define UUmABS(x) (abs(x))
	#define UUmSQR(x) ((x) * (x))
	#define UUmMin(a,b) ((a)>(b)?(b):(a))
	#define UUmMin3(a,b,c) UUmMin(UUmMin((a),(b)),(c))
	#define UUmMin4(a,b,c,d) UUmMin(UUmMin((a),(b)),UUmMin((c),(d)))
	#define UUmMin5(a,b,c,d,e) UUmMin(UUmMin4((a),(b),(c),(d)),(e))
	#define UUmMax(a,b) ((a)>(b)?(a):(b))
	#define UUmMax3(a,b,c) UUmMax(UUmMax((a),(b)),(c))
	#define UUmMax4(a,b,c,d) UUmMax(UUmMax((a),(b)),UUmMax((c),(d)))
	#define UUmMax5(a,b,c,d,e) UUmMax(UUmMax4((a),(b),(c),(d)),(e))
	#define UUmMinMax(a,min,max) do { if ((a) > (max)) { (max) = (a); } else if ((a) < (min)) { (min) = (a); } } while(0)
//	S.S this is wrong #define UUmFloor(n,floor) ((n)<(floor)?(floor):(n))
//	S.S. this is wrong #define UUmCeiling(n,ceiling) ((n)>(ceiling)?(ceiling):(n))
	#define UUmPin(n,floor,ceiling) ((n)<(floor) ? (floor) : UUmMin(n,ceiling))
	#define UUmMakeMultiple(value, multipleof) (((value) + ((multipleof) - 1)) & ~((multipleof) - 1))
	#define UUmMakeMultipleCacheLine(value) UUmMakeMultiple(value, UUcProcessor_CacheLineSize)
//	S.S. this is bad; you can actually spend more cycles this way than doing a real swap! #define UUmSwap(a,b) { (a)^=(b); (b)^=(a); (a)^=(b); }
	#define UUmOffsetPtr(ptr, offset) ( (void *) (((char *) (ptr)) + ((UUtUns32) (offset))) )

	#define UUmMakeLong(hi,lo)	((UUtUns32)(((((UUtUns16)(hi)) << 16)) | ((UUtUns16)(lo))))
	#define UUmHighWord(l)		((UUtUns16)(((UUtUns32)(l)) >> 16))
	#define UUmLowWord(l)		((UUtUns16)(l))
	
	#define UUmMakeShort(hi,lo)	((UUtUns16)((((UUtUns8)(hi)) << 8) | ((UUtUns8)(lo))))
	#define UUmHighByte(s)		((UUtUns8)(((UUtUns16)(s)) >> 8))
	#define UUmLowByte(s)		((UUtUns8)(s))
	
	
	#define UUm2CharToUns16(c1, c2) (UUtUns16)(((UUtUns16)(c1) << 8) | (UUtUns16)(c2))
	#define UUm4CharToUns32(c1, c2, c3, c4) (UUtUns32)(((UUtUns32)(c1) << 24) | ((UUtUns32)(c2) << 16) | ((UUtUns32)(c3) << 8) | (UUtUns32)(c4))

	// some trig helper macros 
	
	// for theta >= 0 && theta <= 4 pi, changes theta to theta >= 0 && theta <= 2 pi
	#define UUmTrig_ClipHigh(theta) do { if ((theta) >= M3c2Pi) { (theta) -= M3c2Pi; } } while(0)

	// for theta >= -2 pi && theta <= 2 pi, changes theta to theta >= 0 && theta <= 2 pi
	#define UUmTrig_ClipLow(theta) do { if ((theta) < 0) { (theta) += M3c2Pi; } } while(0)

	// for theta >= -2 pi && theta <= 4 pi, changes theta to theta >= 0 && theta <= 2 pi
	#define UUmTrig_Clip(theta) do { \
					if ((theta) < 0) { (theta) += M3c2Pi; } \
					else if ((theta) >= M3c2Pi) { (theta) -= M3c2Pi; } } while(0)

	#define UUmTrig_ClipAbsPi(theta) do {		\
		if (theta < -M3cPi)						\
			theta += M3c2Pi;					\
		else if (theta > M3cPi)					\
			theta -= M3c2Pi;					\
	} while (0)

	// some floating point helper macros
	#define UUcEpsilon			0.0009765625f // S.S. (1.0f / 1024.0f)
	#define UUcNewfieEpsilon	(0.1f)
	#define UUcLooseEpsilon		(0.01f)
	#define UUcTightEpsilon		0.00000095367431640625f // S.S. (1.0f / UUmSQR(1024.0f))
	#define UUmFloat_2Int16Trunc(x) ((UUtUns16)(x))
	//#define UUmFloat_Ceiling(x) ((float)((UUtInt32)(x + 0.99999f)))
	//#define UUmFloat_Floor(x) ((float)(((x) < 0.0f) ? (UUtInt32)(x - 0.99999f) : (UUtInt32)(x)))
	#define UUmFloat_Ceiling(x) ((float)(ceil((double)x)))
	#define UUmFloat_Floor(x) ((float)(floor((double)x)))
	#define UUmFloat_LooseCompareZero(x) ((float)fabs(x) < UUcLooseEpsilon)
	#define UUmFloat_TightCompareZero(x) ((float)fabs(x) < UUcTightEpsilon)
	#define UUmFloat_CompareEqu(a, b) ((float)fabs((a) - (b)) < UUcEpsilon)
	#define UUmFloat_CompareLE(a, b) ((float)((a) - (b)) < UUcEpsilon)
	#define UUmFloat_CompareLT(a, b) ((float)((a) - (b)) < -UUcEpsilon)
	#define UUmFloat_CompareGE(a, b) ((float)((b) - (a)) < UUcEpsilon)
	#define UUmFloat_CompareGT(a, b) ((float)((b) - (a)) < -UUcEpsilon)
	#define UUmFloat_CompareEquSloppy(a, b) ((float)fabs((a) - (b)) < UUcNewfieEpsilon)
	#define UUmFloat_CompareLESloppy(a, b) ((float)((a) - (b)) < UUcNewfieEpsilon)
	#define UUmFloat_CompareLTSloppy(a, b) ((float)((a) - (b)) < -UUcNewfieEpsilon)
	#define UUmFloat_CompareGESloppy(a, b) ((float)((b) - (a)) < UUcNewfieEpsilon)
	#define UUmFloat_CompareGTSloppy(a, b) ((float)((b) - (a)) < -UUcNewfieEpsilon)
	
	#define UUmFloat_Mod(n, mod) ( ((n) < 0) ? ((n) + (mod)) : (((n) > mod) ? ((n) - mod) : (n) ) )

	#define UUmChar_Upper(c)	(((c) >= 'a' && (c) <= 'z') ? ((c) + ('A' - 'a')) : (c))
	
	#define UUmCPPCanSuck(x) #x
	#define UUmStringize(y) UUmCPPCanSuck(y)

	#define UUcFramesPerSecond				60
	#define UUcRadPerFrameToDegPerSec		3437.7467707849392526078892888463f // S.S. ( M3cRadToDeg * UUcFramesPerSecond )
	#define UUcDegPerSecToRadPerFrame		0.0002908882086657215961539484614f // S.S. ( M3cDegToRad / UUcFramesPerSecond )

	#define UUmMeters(x)					((x) * 10)
	#define UUmFeet(x)						((x) * 3)
	#define UUcGravity						0.294f // S.S. (.18f * (UUmMeters(9.8f)) / ((float) UUcFramesPerSecond))

	#define UUmFeetToUnits(x)				(UUmFeet(x))
	#define UUmUnitsToFeet(x)				(0.333333333333333f * (x)) // S.S. ((x) / UUmFeet(1))

	#define UUcFloat_Max (FLT_MAX)
	#define UUcFloat_Min (-FLT_MAX)

/*
 * message box code
 */

	typedef enum {
		AUcMBType_AbortRetryIgnore,
		AUcMBType_OK,
		AUcMBType_OKCancel,
		AUcMBType_RetryCancel,
		AUcMBType_YesNo,
		AUcMBType_YesNoCancel
	} AUtMB_ButtonType;

	typedef enum {
		AUcMBChoice_Abort,
		AUcMBChoice_Cancel,
		AUcMBChoice_Ignore,
		AUcMBChoice_No,
		AUcMBChoice_OK,
		AUcMBChoice_Retry,
		AUcMBChoice_Yes
	} AUtMB_ButtonChoice;

	AUtMB_ButtonChoice UUcArglist_Call AUrMessageBox(AUtMB_ButtonType inButtonType, const char *format, ...);

/*
 * ============================ assertions and debugger support ============================
 */

	// UUrDebuggerMessage prints a messsage to the debugger
	// UUrEnterDebugger prints a message to the debugger and halts
	void UUcArglist_Call UUrStartupMessage(const char *format, ...);
	void UUcArglist_Call UUrDebuggerMessage(const char *format, ...);
	void UUcArglist_Call UUrEnterDebugger(const char *format, ...);

	int UUrEnterDebuggerInline(char *msg);

	#if defined(DEBUGGING) && DEBUGGING
		#if UUmSDL
			#define UUmEnterDebugger SDL_TriggerBreakpoint()
		#elif (UUmPlatform == UUmPlatform_Win32)
			#define UUmEnterDebugger __asm { int 3 } UUmBlankFunction
		#elif (UUmPlatform == UUmPlatform_Mac)
			#define UUmEnterDebugger DebugStr("\p")
		#endif
	#else
		#define UUmEnterDebugger
	#endif

	#if defined(DEBUGGING) && DEBUGGING
		
		#if defined(DEBUG_EXTERNAL) && DEBUG_EXTERNAL

			#define UUmAssertTrigRange(number) UUmAssert(((number) >= 0)  && ((number) <= M3c2Pi))

			#if UUmPlatform == UUmPlatform_Mac

				#define UUmAssertReadPtr(ptr, size) UUmAssert((0 == (size)) || ((NULL != (ptr)) && (0x55554742 != ((UUtUns32)(ptr)))))
				#define UUmAssertWritePtr(ptr, size) UUmAssert((0 == (size)) || ((NULL != (ptr)) && (0x55554742 != ((UUtUns32)(ptr)))))
			
			#elif UUmPlatform == UUmPlatform_Win32
			
				#define UUmAssertReadPtr(ptr, size) UUmAssert(!IsBadReadPtr(ptr, size));
				#define UUmAssertWritePtr(ptr, size) UUmAssert(!IsBadWritePtr(ptr, size));
			
			#elif UUmPlatform == UUmPlatform_Linux
			
				//FIXME
				#define UUmAssertReadPtr(ptr, size)
				#define UUmAssertWritePtr(ptr, size)
			
			#endif

			#define UUmAssertAlignedPtr(ptr) UUmAssert((((UUtUns32)(ptr)) & UUcProcessor_CacheLineSize_Mask) == 0)
			
			#if UUmPlatform == UUmPlatform_Win32 && defined(SHIPPING_VERSION) && (SHIPPING_VERSION != 1)
			#define UUmAssert(x) if (!(x)) { \
				extern void stack_walk(int); \
				UUrDebuggerMessage("file %s line %d msg %s"UUmNL,__FILE__,__LINE__,#x); \
				stack_walk(0); }
			#else
			#define UUmAssert(x) if (!(x)) {UUrDebuggerMessage("file %s line %d msg %s"UUmNL,__FILE__,__LINE__,#x);}
			#endif

			#define UUmAssertOnce(x)	UUmAssert(x)

			#define UUmDebugStr(x) if (1) { UUrDebuggerMessage(x);}

			#define UUmAssert_Untested()

		#else

			#define UUmAssertTrigRange(number) UUmAssert(((number) >= 0)  && ((number) <= M3c2Pi))
			
			#if UUmPlatform == UUmPlatform_Mac
			
				#define UUmAssertReadPtr(ptr, size) UUmAssert((0 == (size)) || ((NULL != (ptr)) && (0x55554742 != ((UUtUns32) (ptr))) && (0xDDDDDDDD != ((UUtUns32)(ptr)))))
				#define UUmAssertWritePtr(ptr, size) UUmAssert((0 == (size)) || ((NULL != (ptr)) && (0x55554742 != ((UUtUns32)(ptr)))))
			
			#elif UUmPlatform == UUmPlatform_Win32
			
				#define UUmAssertReadPtr(ptr, size) UUmAssert(!IsBadReadPtr(ptr, size) && (0xDDDDDDDD != ((UUtUns32) ptr)));
				#define UUmAssertWritePtr(ptr, size) UUmAssert(!IsBadWritePtr(ptr, size) && (0xDDDDDDDD != ((UUtUns32) ptr)));
			
			#elif UUmPlatform == UUmPlatform_Linux
			
				//FIXME
				#define UUmAssertReadPtr(ptr, size)
				#define UUmAssertWritePtr(ptr, size)
			
			#endif

			#define UUmAssertAlignedPtr(ptr) UUmAssert((((UUtUns32)(ptr)) & UUcProcessor_CacheLineSize_Mask) == 0)
			
			#if UUmPlatform == UUmPlatform_Mac
			
				#define UUmAssert(x) if (!(x)) {UUrDebuggerMessage("file %s line %d msg %s"UUmNL,__FILE__,__LINE__,#x); UUmEnterDebugger;}
			
			#else
			
				#define UUmAssert(x) if (!(x)) do { AUrMessageBox(AUcMBType_OK, "file %s line %d msg %s"UUmNL,__FILE__,__LINE__,#x);  UUrDebuggerMessage("file %s line %d msg %s"UUmNL,__FILE__,__LINE__,#x); UUmEnterDebugger; } while(0)
			
			#endif
			
			#define UUmAssertOnce(x) do { static int assert_once = 0; if (!assert_once) { if (!(x)) { int assert_result;  assert_once = 1; UUrDebuggerMessage("file %s line %d msg %s"UUmNL,__FILE__,__LINE__,#x); assert_result = UUrEnterDebuggerInline(#x);  if (assert_result) { UUmEnterDebugger; } } } } while(0)
			#define UUmDebugStr(x) if (1) { int debug_result = UUrEnterDebuggerInline(x); if (debug_result) UUmEnterDebugger; }

			#define UUmAssert_Untested()	UUmAssert(!"I am not tested, just a warning - remove me if I work")
		
		#endif

	#else	

		#define UUmAssertTrigRange(number)
		#define UUmAssertReadPtr(ptr, size)
		#define UUmAssertWritePtr(ptr, size)
		#define UUmAssertAlignedPtr(ptr)
		#define UUmAssert(x)
		#define UUmAssertOnce(x)
		#define UUmDebugStr(x)
		#define UUmAssert_Untested()
		
	#endif	 

/*
 * ============================= error handling =============================
 */

	#define UUcError_None				((UUtError) 0x0000)
	#define UUcError_Generic			((UUtError) 0x0001)
	#define UUcError_OutOfMemory		((UUtError) 0x0002)
	#define UUcError_DirectDraw			((UUtError) 0x0003)
			
		
	void UUrError_Report_Internal(
		const char			*inFile,
		unsigned long		inLine,
		UUtError			inError,
		const char			*inMessage);

	void UUrError_ReportP_Internal(
		const char			*inFile,
		unsigned long		inLine,
		UUtError			inError,
		const char			*inMessage,
		UUtUns32			value0,
		UUtUns32			value1,
		UUtUns32			value2);
			
	void UUcArglist_Call UUrPrintWarning(
		const char 			*format,
		...);

/*
 * Error handling macros
 */
	extern UUtBool UUgError_HaltOnError;
	extern UUtBool UUgError_PrintWarning;

	#define UUmError_ReturnOnErrorMsgP(error, msg, v1, v2, v3)									\
		if((error) != UUcError_None)															\
		{																						\
			if (UUgError_HaltOnError)															\
			{																					\
				UUmEnterDebugger;																\
			}																					\
			if (UUgError_PrintWarning)															\
			{																					\
				UUrPrintWarning(msg, v1, v2, v3);												\
			}																					\
			else																				\
			{																					\
				UUrError_ReportP_Internal(__FILE__, __LINE__, (error), msg, v1, v2 ,v3);		\
			}																					\
			return (error);																		\
		}																						\
		do { } while(0)

	#define UUmError_ReturnOnErrorMsg(error, msg)								\
		if((error) != UUcError_None)											\
		{																		\
			if (UUgError_HaltOnError)											\
			{																	\
				UUmEnterDebugger;												\
			}																	\
			if (UUgError_PrintWarning)											\
			{																	\
				UUrPrintWarning(												\
					"File %s Line %d Error %d, %s",								\
					__FILE__, __LINE__, (error), msg); 							\
			}																	\
			else																\
			{																	\
				UUrError_Report_Internal(__FILE__, __LINE__, (error), msg);		\
			}																	\
			return (error);														\
		}																		\
		do { } while(0)

	#define UUmError_ReturnOnError(error)										\
		if ((error) != UUcError_None)											\
		{																		\
			if (UUgError_HaltOnError)											\
			{																	\
				UUmEnterDebugger;												\
			}																	\
			if (UUgError_PrintWarning)											\
			{																	\
				UUrPrintWarning(												\
					"File %s Line %d Error %d",									\
					__FILE__, __LINE__, (error)); 								\
			}																	\
			else																\
			{																	\
				UUrError_Report_Internal(__FILE__, __LINE__, (error), "error"); \
			}																	\
			return(error);														\
		}																		\
		do { } while(0)

	#define UUmError_ReturnOnNull(ptr) \
		if (NULL == (ptr)) { UUmError_ReturnOnError(UUcError_OutOfMemory); } \
		do { } while(0)

	#define UUmError_ReturnOnNullMsg(ptr, msg) \
		if (NULL == (ptr)) { UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, msg); } \
		do { } while(0)

	// not really sure what this macro does
	#define UUmWarning_ReportOnError(error, msg) if ((error) != UUcError_None) { UUrError_Report(__FILE__, __LINE__, (error), msg); }


/*
 * currently UUrError_Report generates a halt however this will
 * not be true in the future.  The goal is to return all the
 * way out building a call chain of errors.
 *
 * UUrError_Report will write to the error log in the final
 * version of the program so keep that in mind if this is a
 * non-fatal error, chances are that you didn't want to do
 * that.
 *
 */

	#define UUrError_Report(error,msg) \
		UUrError_Report_Internal(__FILE__, __LINE__, (error), msg); \
		UUmAssert(UUcError_None == (error)); \
		do {} while (0)

	#define UUrError_ReportP(error,msg,p0,p1,p2) \
		UUrError_ReportP_Internal(__FILE__, __LINE__, error, msg, p0, p1, p2); \
		UUmAssert(UUcError_None == (error)); \
		do {} while (0)
		
#if UUmPlatform == UUmPlatform_Win32

	char *UUrError_DirectDraw2Str(HRESULT	ddReturn);

#endif
			
/*
 * Memory utilities
 *
 * Important notes
 *	1. All memory is UUcProcessor_CacheLineSize aligned
 *	2. All memory is allocated in multiples of UUcProcessor_CacheLineSize blocks
 *  3. Do not use the functions with the "_Real" or "_Debug" directly. Use the macros 
 */

/*
 * IMPORTANT:
 * Do not use the functions with the "_Real" suffix . These are for documentation.
 * Use the macros defined below
 */

	/*
	 * alignment function, increments pointer until aligned
	 */ 

	void *UUrAlignMemory(void *inMemory);

	/*
	 * Regular block allocation functions
	 */
		void*
		UUrMemory_Block_New_Real(		// This is used to allocate a new block
			UUtUns32	inSize);

		void*
		UUrMemory_Block_NewClear_Real(		// This is used to allocate a new block
			UUtUns32	inSize);

		void 
		UUrMemory_Block_Delete_Real(	// This is used to delete a block
			void*		inMemory);

		void*
		UUrMemory_Block_Realloc_Real(		// This is used to reallocate the size of a block
			void*		inMemory,
			UUtUns32	inSize);

	/*
	 * Memory pool functions - This allows fast allocs and a single delete for entire pool
	 */
		typedef struct UUtMemory_Pool	UUtMemory_Pool;

#define UUcPool_Fixed		UUcTrue
#define UUcPool_Growable	UUcFalse
	
		UUtMemory_Pool* 
		UUrMemory_Pool_New_Real(			// This is used to create a new memory pool
			UUtUns32		inChunkSize,	// This is the chunk size of the pool
			UUtBool			inFixed);		// This is true if only one pool is allocated

		void 
		UUrMemory_Pool_Delete_Real(		// This is used to delete a pool - Use reset below to empty out the pool
			UUtMemory_Pool	*inMemoryPool);
		
		void *
		UUrMemory_Pool_Block_New(	// This is used to allocate a block within a pool
			UUtMemory_Pool	*inMemoryPool,
			UUtUns32		inSize);
		
		void 
		UUrMemory_Pool_Reset(		// This is used to reset the pool to be empty
			UUtMemory_Pool	*inMemoryPool);

		UUtUns32
		UUrMemory_Pool_Get_ChunkSize(
			UUtMemory_Pool	*inMemoryPool);
	
	/*
	 * Heap allocation functions
	 */
		typedef struct UUtMemory_Heap	UUtMemory_Heap;
		
		UUtMemory_Heap* 
		UUrMemory_Heap_New_Real(			// This is used to create a new memory heap
			UUtUns32		inChunkSize,	// This is the chunk size of the pool
			UUtBool			inFixed);		// This is true if only one pool is allocated
		void 
		UUrMemory_Heap_Delete_Real(		// This is used to delete a heap - Use reset below to empty out the pool
			UUtMemory_Heap	*inMemoryHeap);
		
		void *
		UUrMemory_Heap_Block_New_Real(	// This is used to allocate a block within a heap
			UUtMemory_Heap	*inMemoryHeap,
			UUtUns32		inSize);
		
		void
		UUrMemory_Heap_Block_Delete_Real(	// This is used to allocate a block within a heap
			UUtMemory_Heap	*inMemoryHeap,
			void*			inBlock);
		
		void
		UUrMemory_Heap_Reset(
			UUtMemory_Heap	*inMemoryHeap);

	/*
	 * Resizable array functions - The elements are guaranteed to be linear
	 */
		typedef struct UUtMemory_Array	UUtMemory_Array;

		UUtMemory_Array* 
		UUrMemory_Array_New_Real(			// This is used to create a new array
			UUtUns32			inElementSize,			// The size of each element
			UUtUns32			inAllocChunkSize,		// The chunk size of incremental allocations
			UUtUns32			inNumInitialElemsUsed,	// The number of elements initially used
			UUtUns32			inNumElemsToAlloc);		// The number of elements to allocate

		void 
		UUrMemory_Array_Delete_Real(		// This is used to delete a array
			UUtMemory_Array*	inMemoryArray);
		
		UUtError 
		UUrMemory_Array_MakeRoom(			// This is used to make sure that enough elems have been allocated
			UUtMemory_Array*	inMemoryArray,
			UUtUns32			inNumElems,
			UUtBool				*outMemoryMoved);		// Returns wether a reallocation happened this moving memory

		void*
		UUrMemory_Array_GetMemory(			// This is used to get the pointer to the linear array memory
			UUtMemory_Array*	inMemoryArray);
		
		UUtError
		UUrMemory_Array_GetNewElement(		// This is used to get an additional element from the array
			UUtMemory_Array*	inMemoryArray,
			UUtUns32			*outNewIndex,
			UUtBool				*outMemoryMoved);
		
		void
		UUrMemory_Array_DeleteElement(		// This always moves memory
			UUtMemory_Array*	inMemoryArray,
			UUtUns32			inIndex);
		
		UUtError
		UUrMemory_Array_InsertElement(		// This always moves memory
			UUtMemory_Array*	inMemoryArray,
			UUtUns32			inIndex,
			UUtBool				*outMemoryMoved);
		
		UUtUns32
		UUrMemory_Array_GetUsedElems(	// This is used to return the number of elements in use
			UUtMemory_Array*	inMemoryArray);
			
		UUtError
		UUrMemory_Array_SetUsedElems(	// This is used to set the number of elements in use
			UUtMemory_Array*	inMemoryArray,
			UUtUns32			inElemsUsed,
			UUtBool				*outMemoryMoved);
			
	/*
	 * Parallel array functions - operations effect all member arrays
	 */
		enum
		{
			UUcMemory_ParallelArray_MaxMembers	= 5
		};
		
		typedef struct UUtMemory_ParallelArray			UUtMemory_ParallelArray;
		typedef struct UUtMemory_ParallelArray_Member	UUtMemory_ParallelArray_Member;

		UUtMemory_ParallelArray* 
		UUrMemory_ParallelArray_New_Real(		// This is used to create a new parallel array
			UUtUns32					inAllocChunkSize,		// The chunk size of incremental allocations
			UUtUns32					inNumInitialElemsUsed,	// The number of elements initially used
			UUtUns32					inNumElemsToAlloc);		// The number of elements to allocate
		
		void 
		UUrMemory_ParallelArray_Delete_Real(	// This is used to delete a parallel array
			UUtMemory_ParallelArray*	inParallelArray);
		
		UUtMemory_ParallelArray_Member* 
		UUrMemory_ParallelArray_Member_New(	// This is used to create a new member array
			UUtMemory_ParallelArray*	inParallelArray,
			UUtUns32					inElementSize);
		
		UUtError 
		UUrMemory_ParallelArray_MakeRoom(		// This is used to make sure that enough elems have been allocated
			UUtMemory_ParallelArray*	inParallelArray,
			UUtUns32					inNumElems,
			UUtBool						*outMemoryMoved);

		void
		UUrMemory_ParallelArray_DeleteElement(	// This is used to delete a element in every member array
			UUtMemory_ParallelArray*	inParallelArray,
			UUtUns32					inIndex);
		
		UUtUns32
		UUrMemory_ParallelArray_GetNumUsedElems(	// This is used to return the number of used elements
			UUtMemory_ParallelArray*	inParallelArray);
		
		UUtError
		UUrMemory_ParallelArray_SetUsedElems(	// This is used to set the number of elements in use
			UUtMemory_ParallelArray*	inParallelArray,
			UUtUns32					inElemsUsed,
			UUtBool						*outMemoryMoved);
			
		UUtError
		UUrMemory_ParallelArray_AddNewElem(			// This adds a new element to every member array - use UUrMemory_ParallelArray_Member_GetNewElement_Real to get the memory
			UUtMemory_ParallelArray*	inParallelArraym,
			UUtUns32					*outNewIndex,
			UUtBool						*outMemoryMoved);
		
		void*
		UUrMemory_ParallelArray_Member_GetMemory(	// This is used to get the pointer to the linear array memory
			UUtMemory_ParallelArray*		inParallelArray,
			UUtMemory_ParallelArray_Member*	inMemberArray);

		void*
		UUrMemory_ParallelArray_Member_GetNewElement(	// This is used to get the new element from a member array
			UUtMemory_ParallelArray*		inParallelArray,
			UUtMemory_ParallelArray_Member*	inMemberArray);
	
	/*
	 * String memory utilities
	 */
		typedef struct UUtMemory_String	UUtMemory_String;
		
		UUtMemory_String*
		UUrMemory_String_New(
			UUtUns32	inTableSize);
		
		void
		UUrMemory_String_Delete(
			UUtMemory_String*	inMemoryString);
		
		char*
		UUrMemory_String_GetStr(
			UUtMemory_String*	inMemoryString,
			const char*			inStr);
		
		UUtUns32
		UUrMemory_String_GetSize(
			UUtMemory_String*	inMemoryString);

		void
		UUrMemory_String_Reset(
			UUtMemory_String*	inMemoryString);
		
	#if 0 && defined(DEBUGGING) && DEBUGGING
           #define DEBUGGING_MEMORY 1
        #endif
        
	#if DEBUGGING_MEMORY
		
		#define UUmMemoryBlock_Garbage		((unsigned long)'UUGB')
		#define UUmMemoryBlock_GarbageTest		((void *)'UUGB')

		/*
		 * Block allocation debug functions
		 */
			void 
			UUrMemory_Block_VerifyList(
				void);

			void 
			UUrMemory_Block_Verify(
				void			*inMemory);
			
			void*
			UUrMemory_Block_New_Debug(
				char*			inFile,
				long			inLine,
				unsigned long	inSize);

			void*
			UUrMemory_Block_NewClear_Debug(
				char*			inFile,
				long			inLine,
				unsigned long	inSize);

			void 
			UUrMemory_Block_Delete_Debug(
				char*			inFile,
				long			inLine,
				void*			inMemory);
		
			void*
			UUrMemory_Block_Realloc_Debug(
				char		*inFile,
				UUtInt32	inLine,
				void*		inMemory,
				UUtUns32	inSize);

		/*
		 * Memory pool debug functions
		 */
			
			UUtMemory_Pool* 
			UUrMemory_Pool_New_Debug(
				char*			inFile,
				long			inLine,
				UUtUns32		inChunkSize,
				UUtBool			inFixed);
				
			void 
			UUrMemory_Pool_Delete_Debug(
				char*			inFile,
				long			inLine,
				UUtMemory_Pool*	inMemoryPool);
		
		/*
		 * Memory heap debug functions
		 */
			
			UUtMemory_Heap* 
			UUrMemory_Heap_New_Debug(
				char*			inFile,
				long			inLine,
				UUtUns32		inChunkSize,
				UUtBool			inFixed);
				
			void 
			UUrMemory_Heap_Delete_Debug(
				char*			inFile,
				long			inLine,
				UUtMemory_Heap*	inMemoryHeap);
		
			void *
			UUrMemory_Heap_Block_New_Debug(		// This is used to allocate a block within a heap
				char*			inFile,
				long			inLine,
				UUtMemory_Heap	*inMemoryHeap,
				UUtUns32		inSize);
			
			void
			UUrMemory_Heap_Block_Delete_Debug(	// This is used to allocate a block within a heap
				char*			inFile,
				long			inLine,
				UUtMemory_Heap	*inMemoryHeap,
				void*			inBlock);

		/*
		 * Resizeable array debug functions
		 */
			UUtMemory_Array* 
			UUrMemory_Array_New_Debug(
				char*			inFile,
				long			inLine,
				UUtUns32		inElementSize,
				UUtUns32		inAllocChunkSize,
				UUtUns32		inNumInitialElemsUsed,
				UUtUns32		inNumElemsToAlloc);

			void 
			UUrMemory_Array_Delete_Debug(
				char*				inFile,
				long				inLine,
				UUtMemory_Array*	inMemoryArray);
		
		/*
		 * Parallel array debug functions
		 */
			UUtMemory_ParallelArray* 
			UUrMemory_ParallelArray_New_Debug(
				char*			inFile,
				long			inLine,
				UUtUns32		inAllocChunkSize,
				UUtUns32		inNumInitialElemsUsed,
				UUtUns32		inNumElemsToAlloc);
			
			void 
			UUrMemory_ParallelArray_Delete_Debug(
				char*						inFile,
				long						inLine,
				UUtMemory_ParallelArray*	inParallelArray);
			
		/*
		 * Functions for tracing mem leaks
		 */
			void
			UUrMemory_Leak_Start_Real(
				char*		inFileName,
				UUtInt32	inLineNum,
				const char*	inComment);
			
			void
			UUrMemory_Leak_StopAndReport_Real(
				void);
		
		/*
		 * 
		 */
			void
			UUrMemory_Heap_Block_Verify(
				UUtMemory_Heap*	inMemoryHeap,
				void*			inPtr,
				UUtUns32		inLength);
				
			UUtBool
			UUrMemory_SetVerifyEnable(
				UUtBool			inEnable);

			void
			UUrMemory_SetVerifyForce(
								UUtBool inForce);

		/*
		 * Block allocation macros
		 */
			#define	UUrMemory_Block_New(size)			\
				UUrMemory_Block_New_Debug(__FILE__, __LINE__, size)
				
			#define	UUrMemory_Block_NewClear(size)			\
				UUrMemory_Block_NewClear_Debug(__FILE__, __LINE__, size)
				
			#define	UUrMemory_Block_Delete(ptr)			\
				UUrMemory_Block_Delete_Debug(__FILE__, __LINE__, ptr)
			
			#define UUrMemory_Block_Realloc(ptr, size) \
				UUrMemory_Block_Realloc_Debug(__FILE__, __LINE__, ptr, size)
				
		/*
		 * Memory pool macros
		 */
			#define UUrMemory_Pool_New(chunkSize, fixed) \
				UUrMemory_Pool_New_Debug(__FILE__, __LINE__, chunkSize, fixed)
			
			#define UUrMemory_Pool_Delete(pool) \
				UUrMemory_Pool_Delete_Debug(__FILE__, __LINE__, pool)

		/*
		 * Memory heap macros
		 */
			#define UUrMemory_Heap_New(chunkSize, fixed) \
				UUrMemory_Heap_New_Debug(__FILE__, __LINE__, chunkSize, fixed)
			
			#define UUrMemory_Heap_Delete(heap) \
				UUrMemory_Heap_Delete_Debug(__FILE__, __LINE__, heap)
		
			#define	UUrMemory_Heap_Block_New(heap, size)			\
				UUrMemory_Heap_Block_New_Debug(__FILE__, __LINE__, heap, size)
				
			#define	UUrMemory_Heap_Block_Delete(heap, ptr)			\
				UUrMemory_Heap_Block_Delete_Debug(__FILE__, __LINE__, heap, ptr)
		
		/*
		 * Resizeable array macros
		 */
			#define UUrMemory_Array_New(inElementSize, inAllocChunkSize, inNumInitialElemsUsed, inNumElemsToAlloc) \
				UUrMemory_Array_New_Debug(__FILE__, __LINE__, inElementSize, inAllocChunkSize, inNumInitialElemsUsed, inNumElemsToAlloc)
			
			#define UUrMemory_Array_Delete(inMemoryArray) \
				UUrMemory_Array_Delete_Debug(__FILE__, __LINE__, inMemoryArray)
		
		/*
		 * Parallel array macros
		 */
			#define UUrMemory_ParallelArray_New(inAllocChunkSize, inNumInitialElemsUsed, inNumElemsToAlloc) \
				UUrMemory_ParallelArray_New_Debug(__FILE__, __LINE__, inAllocChunkSize, inNumInitialElemsUsed, inNumElemsToAlloc)
			
			#define UUrMemory_ParallelArray_Delete(inParallelArray) \
				UUrMemory_ParallelArray_Delete_Debug(__FILE__, __LINE__, inParallelArray)
				
		/*
		 * Leak macros
		 */
			#define UUrMemory_Leak_Start(comm) UUrMemory_Leak_Start_Real(__FILE__, __LINE__, comm)
			#define UUrMemory_Leak_StopAndReport()	UUrMemory_Leak_StopAndReport_Real()
			
			#define UUrMemory_Leak_ForceGlobal_Begin()	UUrMemory_Leak_ForceGlobal_Begin_Real()
			#define UUrMemory_Leak_ForceGlobal_End()	UUrMemory_Leak_ForceGlobal_End_Real()
			
			void
			UUrMemory_Leak_ForceGlobal_Begin_Real(
				void);
			
			void
			UUrMemory_Leak_ForceGlobal_End_Real(
				void);
				
	#else		
		
		#define UUrMemory_Heap_Block_Verify(x, y, z)
		#define UUrMemory_SetVerifyEnable(x)	UUcFalse
		#define UUrMemory_SetVerifyForce(x)
		
		/* Non debugging macors */
				
		/*
		 * Block allocation macros
		 */
			#define UUrMemory_Block_VerifyList()
			
			#define UUrMemory_Block_Verify(inMemory)
		
			#define	UUrMemory_Block_New(size) \
				UUrMemory_Block_New_Real(size)
				
			#define	UUrMemory_Block_NewClear(size) \
				UUrMemory_Block_NewClear_Real(size)
				
			#define	UUrMemory_Block_Delete(ptr) \
				UUrMemory_Block_Delete_Real(ptr)
			
			#define UUrMemory_Block_Realloc(ptr, size) \
				UUrMemory_Block_Realloc_Real(ptr, size)
		
		
		/*
		 * Memory pool macros
		 */
			#define UUrMemory_Pool_New(chunkSize, fixed) \
				UUrMemory_Pool_New_Real(chunkSize, fixed)
			
			#define UUrMemory_Pool_Delete(pool) \
				UUrMemory_Pool_Delete_Real(pool)
		
		
		/*
		 * Memory heap macros
		 */
			#define UUrMemory_Heap_New(chunkSize, fixed) \
				UUrMemory_Heap_New_Real(chunkSize, fixed)
			
			#define UUrMemory_Heap_Delete(heap) \
				UUrMemory_Heap_Delete_Real(heap)
				
			#define	UUrMemory_Heap_Block_New(heap, size)			\
				UUrMemory_Heap_Block_New_Real(heap, size)
				
			#define	UUrMemory_Heap_Block_Delete(heap, ptr)			\
				UUrMemory_Heap_Block_Delete_Real(heap, ptr)

		/*
		 * Resizeable array macros
		 */
			#define UUrMemory_Array_New(inElementSize, inAllocChunkSize, inNumInitialElemsUsed, inNumElemsToAlloc) \
				UUrMemory_Array_New_Real(inElementSize, inAllocChunkSize, inNumInitialElemsUsed, inNumElemsToAlloc)
			
			#define UUrMemory_Array_Delete(inMemoryArray) \
				UUrMemory_Array_Delete_Real(inMemoryArray)
		
		/*
		 * Parallel array macros
		 */
			#define UUrMemory_ParallelArray_New(inAllocChunkSize, inNumInitialElemsUsed, inNumElemsToAlloc) \
				UUrMemory_ParallelArray_New_Real(inAllocChunkSize, inNumInitialElemsUsed, inNumElemsToAlloc)
			
			#define UUrMemory_ParallelArray_Delete(inParallelArray) \
				UUrMemory_ParallelArray_Delete_Real(inParallelArray)

		/*
		 * Leak macros
		 */
			#define UUrMemory_Leak_Start(comm)
			#define UUrMemory_Leak_StopAndReport()
			#define UUrMemory_Leak_ForceGlobal_Begin()
			#define UUrMemory_Leak_ForceGlobal_End()
	#endif


/*
 * ============================ Memory Moving and Setting functions ============================
 */

	void UUrMemory_ArrayElement_Delete(const void *inBasePtr, UUtUns32 inDeleteElemIndex, UUtUns32 inArrayLength, UUtUns32 inElemSize);

	void UUrMemory_Set16(void *inDst, UUtUns16 inFillValue, UUtUns32 inNumBytes);
	void UUrMemory_Set32(void *inDst, UUtUns32 inFillValue, UUtUns32 inNumBytes);


#if DEBUGGING_MEMORY
	void UUrMemory_MoveFast_Debug(const void *inSrc, void *inDst, UUtUns32 inSize);	
	void UUrMemory_MoveOverlap_Debug(const void *inSrc, void *inDst, UUtUns32 inSize);
	void UUrMemory_Clear_Debug(void *inDst, UUtUns32 inNumBytes);
	void UUrMemory_Set8_Debug(void *inDst, UUtUns8 inFillValue, UUtUns32 inNumBytes);
#endif

	static UUcInline UUtBool UUrMemory_IsEqual(const void *inPtrA, const void *inPtrB, UUtUns32 inLength)
	{
		UUtBool result;
				
		result = 0 == memcmp(inPtrA, inPtrB, inLength);

		return result;
	}

	static UUcInline void UUrMemory_Set8(void *inDst, UUtUns8 inFillValue, UUtUns32 inNumBytes)
	{
#if DEBUGGING_MEMORY
		UUrMemory_Set8_Debug(inDst, inFillValue, inNumBytes);
#else
		memset(inDst, inFillValue, inNumBytes);
#endif
	}

	static UUcInline void UUrMemory_Clear(void *inDst, UUtUns32 inNumBytes)
	{
#if DEBUGGING_MEMORY
		UUrMemory_Clear_Debug(inDst, inNumBytes);
#else
		memset(inDst, 0, inNumBytes);
#endif
	}

	static UUcInline void UUrMemory_MoveFast(const void *inSrc, void *inDst, UUtUns32 inSize)
	{
#if DEBUGGING_MEMORY
		UUrMemory_MoveFast_Debug(inSrc, inDst, inSize);
#elif UUmPlatform == UUmPlatform_Mac
		BlockMoveData(inSrc, inDst, inSize);
#else
		memcpy(inDst, inSrc, inSize);
#endif
		return;
	}

	static UUcInline void UUrMemory_MoveOverlap(const void *inSrc, void *inDst, UUtUns32 inSize)
	{
#if DEBUGGING_MEMORY
		UUrMemory_MoveOverlap_Debug(inSrc, inDst, inSize);
#elif UUmPlatform == UUmPlatform_Mac
		BlockMoveData(inSrc, inDst, inSize);
#else
		memmove(inDst, inSrc, inSize);
#endif
		return;
	}

/*
 * ============================ String utilities ============================
 */
	UUtInt32 UUrString_Compare_NoCase_NoSpace(const char *inStrA, const char *inStrB);	// ignores all whitespace
	UUtInt32 UUrString_Compare_NoCase(const char*	inStrA, const char* inStrB);
	UUtInt32 UUrString_CompareLen_NoCase(const char*	inStrA, const char* inStrB, UUtUns32	inLength);

	#define UUmString_IsEqual(inStrA, inStrB) ( ( 0 == strcmp((inStrA),(inStrB)) ) )
	#define UUmString_IsEqual_NoCase(inStrA, inStrB) ( ( 0 == UUrString_Compare_NoCase((inStrA),(inStrB)) ) )
	#define UUmString_IsEqual_NoCase_NoSpace(inStrA, inStrB) ( ( 0 == UUrString_Compare_NoCase_NoSpace((inStrA),(inStrB)) ) )

	UUtBool	 UUrIsSpace(char c);

	UUtError UUrString_To_Uns8(const char *inString, UUtUns8 *outNum);
	UUtError UUrString_To_Int8(const char *inString, UUtInt8 *outNum);
	UUtError UUrString_To_Uns16(const char *inString, UUtUns16 *outNum);
	UUtError UUrString_To_Int16(const char *inString, UUtInt16 *outNum);
	UUtError UUrString_To_Uns32(const char *inString, UUtUns32 *outNum);
	UUtError UUrString_To_Int32(const char *inString, UUtInt32 *outNum);
	UUtError UUrString_To_Float(const char *inSTring, float *outNum);
	UUtError UUrString_To_FloatRange(const char *inString, float *outLow, float *outHigh);

	UUtInt32 UUrString_CountDigits(UUtInt32 inNumber);
	void UUrString_PadNumber(UUtInt32 inNumber, UUtInt32 inDigits, char *outString);

	char* UUrTag2Str(UUtUns32	inTag);
	
	#if UUmPlatform == UUmPlatform_Mac
	
		#define UUmNL	"\n"
	
	#elif UUmPlatform == UUmPlatform_Win32
	
			#define UUmNL	"\r\n"
	
	#elif UUmPlatform == UUmPlatform_Linux
	
			#define UUmNL	"\n"
	
	#else
	
		#error define me
	
	#endif
/*
 * ============================ Random Numbers ============================
 */

	#define UUcRandomBits 16
	#define UUcDefaultRandomSeed 0xfded

	extern UUtUns32 UUgLocalRandomSeed;

	// set and get a seed (for example on game start)
	// this effects UUrRandom only, not UUrLocalRandom
	void UUrRandom_SetSeed(UUtUns32 seed);
	UUtUns32 UUrRandom_GetSeed(void);

	// gets a random number in sync
	UUtUns16 UUrRandom(void);

	// gets a local random number (not in sync with the world)
	UUtUns16 UUrLocalRandom(void);

	// delta < 16 bits
	#define UUmRandomRange(lower_bound, delta) ((UUtUns16) ((lower_bound) + (((delta)>0) ? (((delta)*UUrRandom())>>UUcRandomBits) : 0)))
	#define UUmLocalRandomRange(lower_bound, delta) ((UUtUns16) ((lower_bound) + (((delta)>0) ? (((delta)*UUrLocalRandom())>>UUcRandomBits) : 0)))

	// Random angle
	#define UUmRandomAngle(lower,delta) (	\
		(float)(UUmRandomRange(					\
			(UUtUns16)(57.295779513f/* S.S.(180.0f / M3cPi)*/ * (lower)),	\
			(UUtUns16)(57.295779513f/* S.S.(180.0f / M3cPi)*/ * (delta))))	\
		* 0.01745329252f/* S.S.(M3cPi / 180.0f)*/)

	// Random float, 0->1
	// S.S. #define UUmRandomFloat ((float)UUrRandom() / (float)UUcMaxUns16)
	#define UUmRandomFloat ((float)UUrRandom() * 1.52590219e-5f)
	
	// Random float, lo->hi
	#define UUmRandomRangeFloat(lo,hi) (UUmRandomFloat * ((hi)-(lo)) + (lo))
				

/*
 * Interrupt handling utilities
 */
	typedef struct UUtInterruptProcRef UUtInterruptProcRef;

	typedef UUtBool (*UUtInterruptProc)(	// Return UUcTrue if reinstall
		void	*refcon);

	UUtError UUrInterruptProc_Install(
		UUtInterruptProc	inInterruptProc,
		UUtInt32			inTiggersPerSec,
		void				*inRefCon,
		UUtInterruptProcRef	**outInterruptProcRef);

	void UUrInterruptProc_Deinstall(
		UUtInterruptProcRef	*inInterruptProcRef);

/*
 * Byte swapping utilities
 */
	static UUcInline void
	UUrSwap_4Byte(
		void*	inAddr)
	{
		UUtUns32 i = *(UUtUns32*)inAddr;
		
		UUmAssertReadPtr(inAddr, 4);
		
		*(UUtUns32*)inAddr = ((i & 0xFF000000) >> 24) | ((i & 0x00FF0000) >> 8)
			| ((i & 0x0000FF00) << 8) | ((i & 0x000000FF) << 24);
	}

	static UUcInline void 
	UUrSwap_2Byte(
		void	*inAddr)
	{
		UUtUns16 i = *(UUtUns16*)inAddr;
		
		UUmAssertReadPtr(inAddr, 2);
		
		*(UUtUns16*)inAddr = ((i & 0xFF00) >> 8) | ((i & 0x00FF) << 8);
	}

	static UUcInline void 
	UUrSwap_8Byte(
		void*	inAddr)
	{	
		UUtUns32*	p = (UUtUns32*)inAddr;
		UUtUns32	v;

		UUmAssertReadPtr(inAddr, 8);
		
		v = p[0];
		p[0] = p[1];
		p[1] = v;
		
		UUrSwap_4Byte(p);
		UUrSwap_4Byte(p + 1);
	}

	void UUrSwap_2Byte_Array(void *inAddr, UUtUns32 count);
	void UUrSwap_4Byte_Array(void *inAddr, UUtUns32 count);
	void UUrSwap_8Byte_Array(void *inAddr, UUtUns32 count);
		
#if UUmEndian == UUmEndian_Big
	#define UUmSwapLittle_2Byte(addr) UUrSwap_2Byte(addr)
	#define UUmSwapLittle_4Byte(addr) UUrSwap_4Byte(addr)
	#define UUmSwapLittle_8Byte(addr) UUrSawp_8Byte(addr)
	#define UUmSwapLittle_2Byte_Array(addr, count) UUrSwap_2Byte_Array(addr, count)
	#define UUmSwapLittle_4Byte_Array(addr, count) UUrSwap_4Byte_Array(addr, count)
	#define UUmSwapLittle_8Byte_Array(addr, count) UUrSawp_8Byte_Array(addr, count)

	#define UUmSwapBig_2Byte(addr) UUmAssertReadPtr(addr, 2)
	#define UUmSwapBig_4Byte(addr) UUmAssertReadPtr(addr, 4)
	#define UUmSwapBig_8Byte(addr) UUmAssertReadPtr(addr, 8)
	#define UUmSwapBig_2Byte_Array(addr, count) UUmAssertReadPtr((addr), (count) * 2)
	#define UUmSwapBig_4Byte_Array(addr, count) UUmAssertReadPtr((addr), (count) * 4)
	#define UUmSwapBig_8Byte_Array(addr, count) UUmAssertReadPtr((addr), (count) * 8)
#elif UUmEndian == UUmEndian_Little
	#define UUmSwapLittle_2Byte(addr) UUmAssertReadPtr((addr), 2)
	#define UUmSwapLittle_4Byte(addr) UUmAssertReadPtr((addr), 4)
	#define UUmSwapLittle_8Byte(addr) UUmAssertReadPtr((addr), 8)
	#define UUmSwapLittle_2Byte_Array(addr, count) UUmAssertReadPtr((addr), (count) * 2)
	#define UUmSwapLittle_4Byte_Array(addr, count) UUmAssertReadPtr((addr), (count) * 4)
	#define UUmSwapLittle_8Byte_Array(addr, count) UUmAssertReadPtr((addr), (count) * 8)

	#define UUmSwapBig_2Byte(addr) UUrSwap_2Byte(addr)
	#define UUmSwapBig_4Byte(addr) UUrSwap_4Byte(addr)
	#define UUmSwapBig_8Byte(addr) UUrSwap_8Byte(addr)
	#define UUmSwapBig_2Byte_Array(addr, count) UUrSwap_2Byte_Array(addr, count)
	#define UUmSwapBig_4Byte_Array(addr, count) UUrSwap_4Byte_Array(addr, count)
	#define UUmSwapBig_8Byte_Array(addr, count) UUrSawp_8Byte_Array(addr, count)
#else
	#pragma error 
#endif

	#define UUmSwapNet_2Byte(addr) UUmSwapLittle_2Byte(addr)
	#define UUmSwapNet_4Byte(addr) UUmSwapLittle_4Byte(addr)


/*
 * Initialization and termination functions
 */
	UUtError UUrInitialize(
		UUtBool	inInitializeBasePlatform);
		
	void UUrTerminate(
		void);
	
/*
 * Our atexit functions
 */

	typedef void (UUcExternal_Call *UUtAtExitProc)(void);
	
	UUtError
	UUrAtExit_Register(	
		UUtAtExitProc	inAtExitProc);
		
/*
 * Performance monitoring utilities
 */

#if UUmCompiler	== UUmCompiler_VisC
#define PROFILE 1
#endif
	
	typedef enum UUtProfileState
	{
		UUcProfile_State_Off,
		UUcProfile_State_On
		
	} UUtProfileState;
	
	#if defined(PROFILE) && PROFILE
		
		UUtError
		UUrProfile_Initialize(
			void);
		
		void
		UUrProfile_Terminate(
			void);
		
		void
		UUrProfile_State_Set(
			UUtProfileState	inProfileState);
						
		UUtProfileState
		UUrProfile_State_Get(
			void);
						
		void
		UUrProfile_Dump(
			char*	inFileName);
	
	#else
		
		#define UUrProfile_Initialize() (UUcError_None)
		#define UUrProfile_Terminate()
		#define UUrProfile_State_Set(x)
		#define UUrProfile_State_Get() UUcProfile_State_Off
		#define UUrProfile_Dump(x)
		
	#endif

/*
 * 
 */
	UUtUns32
	UUrStack_GetPCChain(
		UUtUns32	*ioChainList,
		UUtUns32	inMaxNumChains);



/* XXX - get rid of me */
FILE *UUrFOpen(
	char	*name,
	char	*mode);

/*
 * Application instance type
 */
#if UUmSDL

	typedef struct {}			UUtAppInstance;
	typedef SDL_Window*			UUtWindow;

#elif UUmPlatform == UUmPlatform_Win32

	typedef HINSTANCE			UUtAppInstance;
	typedef HWND				UUtWindow;

#elif UUmPlatform == UUmPlatform_Mac

	typedef void*				UUtAppInstance;
	typedef void*				UUtWindow;

#else

#error implement me

#endif

	void
	UUrWindow_GetSize(
		UUtWindow					inWindow,
		UUtUns16					*outWidth,
		UUtUns16					*outHeight);

/*
 * Calendar related time functions
 */
	UUtUns32
	UUrGetSecsSince1900(
		void);
	
	UUtUns32
	UUrConvertStrToSecsSince1900(
		char*	inStr);				// MMM DD YYYY HH:MM:SS

	void
	UUrConvertSecsSince1900ToStr(
		UUtUns32	inSecs,
		char*		inBuffer);		// MMM DD YYYY HH:MM:SS
		
/*
 * ============================ Queue Functions ============================ 
 *
 * note: not suitable for threads, if you must have threads add a 
 * thread get, which memcopies or peak(..., 0) copy and get instead
 * of getting
 *
 */

typedef struct UUtQueue UUtQueue;

UUtQueue *UUrQueue_Create(UUtUns16 inNumElements, UUtUns16 inSize);
void UUrQueue_Dispose(UUtQueue *inQueue);

void *UUrQueue_Get(UUtQueue *inQueue);
void *UUrQueue_Peek(UUtQueue *inQueue, UUtUns16 peekOffset);

UUtError UUrQueue_Add(UUtQueue *inQueue, void *inElement);

UUtUns16 UUrQueue_CountMembers(UUtQueue *inQueue);
UUtUns16 UUrQueue_CountVacant(UUtQueue *inQueue);
	
/*
 * ============================ Misc Junk ============================ 
 */

	UUtUns32 UUrStringToOSType(char *inString);

	#define UUmCompileTimeString __DATE__" "__TIME__
	#define UUmCompileTimeSince1900 UUrConvertStrToSecsSince1900(UUmCompileTimeString)
	
#if UUmPlatform == UUmPlatform_Mac

	#define UUc1904_To_1900_era_offset  ((365UL * 4UL) * 24UL * 60UL * 60UL)

#endif

/*
 * =========================== include all mandatory inlude files ===========================
 */

#include "BFW_ErrorCodes.h"

/*
 * ============================ String stuff ============================ 
 */

	UUtBool
	UUrString_IsSpace(
		const char *inString);

	void
	UUrString_StripDoubleQuotes(
		char*		ioString,
		UUtUns32	inLength);

	void
	UUrString_Capitalize(
		char*		ioString,
		UUtUns32	inLength);

	void
	UUrString_MakeLowerCase(
		char*		ioString,
		UUtUns32	inLength);
		
	UUtBool
	UUrString_Substring(
		const char*	inString,
		const char*	inSub,
		UUtUns32	inStringLength);

	UUtBool
	UUrString_HasPrefix(
		const char*	inString,
		const char*	inPrefix);

	void
	UUrString_NukeNumSuffix(
		char*		ioString,
		UUtUns32	inLength);
	
	void
	UUrString_Copy(					// This copies the terminator as well
		char*		inDst,
		const char*	inSrc,
		UUtUns32	inDstLength);	// length includes the terminator
	
	// CB: copy at most inCopyLength characters to inDst
	// this does not include the terminator, which will be copied
	// only if there is room for it in inDst
	//
	// i.e. if inDstLength == inCopyLength and inSrc has at least
	// inCopyLength non-zero characters, no terminator will be written!
	void
	UUrString_Copy_Length(
		char*		inDst,
		const char*	inSrc,
		UUtUns32	inDstLength,
		UUtUns32	inCopyLength);

	void
	UUrString_Cat(
		char*		inDst,
		const char*	inSrc,
		UUtUns32	inLength);

	char *
	UUrString_Tokenize1(				// like strtok, w/o static & does only 1 token at a time
		char *ioString,
		const char *inStrDelimit,
		char **internal);				// private char *, pass in

	char *
	UUrString_Tokenize(					// like strtok, w/o static
		char *ioString,
		const char *inStrDelimit,
		char **internal);				// private char *, pass in
	
	void
	UUrString_PStr2CStr(
		const unsigned char*const	inPStr,
		char						*outCStr,
		UUtUns32					inBufferSize);
		
	void
	UUrString_StripExtension(
		char*	inStr);
		
	
/*
 * Various one way functions
 */
	UUtUns32
	UUrOneWayFunction_Uns32(
		UUtUns32	inUns32);
	UUtUns16
	UUrOneWayFunction_Uns16(
		UUtUns16	inUns16);
	UUtUns8
	UUrOneWayFunction_Uns8(
		UUtUns8	inUns8);
	
	void
	UUrOneWayFunction_ConvertString(
		char*		ioString,
		UUtUns16	inLength);
	
	UUtBool
	UUrOneWayFunction_CompareString(
		char*		inNormalString,
		char*		inMungedString,
		UUtUns16	inLength);

#ifdef __cplusplus
}
#endif

#endif /* BFW_H */
