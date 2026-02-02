/*
	bfw_math.c
	Wednesday Sept. 20, 2000   2:50am
	Stefan
*/

#ifdef USE_FAST_MATH

/*---------- headers */

#include "bfw_math.h"

#ifdef _WINDOWS_
#include <windows.h>
// already included #include <xmmintrin.h> // SIMD
#include <amath.h> // 3DNow!
#endif // _WINDOWS_

/*---------- constants */

#undef FALSE
#undef TRUE
enum {
	FALSE= 0,
	TRUE
};

/*---------- prototypes */

#ifdef __cplusplus
extern "C" {
#endif

static int determine_processor(void);

/*---------- globals */

double (*ansi_atan)(double x)= NULL;
double (*ansi_atan2)(double x, double y)= NULL;
double (*ansi_acos)(double x)= NULL;
double (*ansi_asin)(double x)= NULL;
double (*ansi_log)(double x)= NULL;
double (*ansi_log10)(double x)= NULL;
double (*ansi_pow)(double x, double y)= NULL;
double (*ansi_exp)(double x)= NULL;
double (*ansi_sqrt)(double x)= NULL;
double (*ansi_fabs)(double x)= NULL;
double (*ansi_ceil)(double x)= NULL;
double (*ansi_floor)(double x)= NULL;
double (*ansi_frexp)(double x, int *y)= NULL;
double (*ansi_ldexp)(double x, int y)= NULL;
double (*ansi_modf)(double x, double *y)= NULL;
double (*ansi_fmod)(double x, double y)= NULL;
double (*ansi_sin)(double x)= NULL;
double (*ansi_cos)(double x)= NULL;
double (*ansi_tan)(double x)= NULL;

float (*fast_atan)(float)= NULL;
float (*fast_atan2)(float,float)= NULL;
float (*fast_acos)(float)= NULL;
float (*fast_asin)(float)= NULL;
float (*fast_log)(float)= NULL;
float (*fast_log10)(float)= NULL;
float (*fast_pow)(float, float)= NULL;
float (*fast_exp)(float)= NULL;
float (*fast_sqrt)(float)= NULL;
float (*fast_fabs)(float)= NULL;
float (*fast_ceil)(float)= NULL;
float (*fast_floor)(float)= NULL;
float (*fast_frexp)(float, int *)= NULL;
float (*fast_ldexp)(float, int)= NULL;
float (*fast_modf)(float, float *)= NULL;
float (*fast_fmod)(float, float)= NULL;
float (*fast_sin)(float)= NULL;
float (*fast_cos)(float)= NULL;
float (*fast_tan)(float)= NULL;

void (*fast_sincos)(float x, float *cs)= NULL;

// quaternion fxns
void (*fast_add_quat)(D3DRMQUATERNION *r, const D3DRMQUATERNION *a, const D3DRMQUATERNION *b)= NULL;
void (*fast_sub_quat)(D3DRMQUATERNION *r, const D3DRMQUATERNION *a, const D3DRMQUATERNION *b)= NULL;
void (*fast_mult_quat)(D3DRMQUATERNION *r, const D3DRMQUATERNION *a, const D3DRMQUATERNION *b)= NULL;
void (*fast_norm_quat)(D3DRMQUATERNION *dst, const D3DRMQUATERNION *a)= NULL;
void (*fast_D3DMat2quat)(D3DRMQUATERNION *quat, const D3DMATRIX *m)= NULL;
void (*fast_glMat2quat)(D3DRMQUATERNION *quat, const float *m)= NULL;
void (*fast_quat2D3DMat)(D3DMATRIX *m, const D3DRMQUATERNION *quat)= NULL;
void (*fast_quat2glMat)(float *m, const D3DRMQUATERNION *quat)= NULL;
void (*fast_euler2quat)(D3DRMQUATERNION *quat, const float *rot)= NULL;
void (*fast_euler2quat2)(D3DRMQUATERNION *quat, const float *rot)= NULL;
void (*fast_quat2axis_angle)(float *axisAngle, const D3DRMQUATERNION *quat)= NULL;
void (*fast_axis_angle2quat)(D3DRMQUATERNION *quat, const float *axisAngle)= NULL;
void (*fast_slerp_quat)(D3DRMQUATERNION *result, const D3DRMQUATERNION *quat1, const D3DRMQUATERNION *quat2, float slerp)= NULL;
void (*fast_trans_quat)(D3DVECTOR *result, const D3DVECTOR *v, const D3DRMQUATERNION *q)= NULL;

// vector fxns
void (*fast_add_vect)(D3DVECTOR *r, const D3DVECTOR *a, const D3DVECTOR *b)= NULL;
void (*fast_sub_vect)(D3DVECTOR *r, const D3DVECTOR *a, const D3DVECTOR *b)= NULL;
void (*fast_mult_vect)(D3DVECTOR *r, const D3DVECTOR *a, const D3DVECTOR *b)= NULL;
void (*fast_scale_vect)(D3DVECTOR *r, const D3DVECTOR *a, float b)= NULL;
void (*fast_norm_vect)(D3DVECTOR *r, const D3DVECTOR *a)= NULL;
float (*fast_mag_vect)(const D3DVECTOR *a)= NULL;
float (*fast_dot_vect)(const D3DVECTOR *a, const D3DVECTOR *b)= NULL;
void (*fast_cross_vect)(D3DVECTOR *r, const D3DVECTOR *a, const D3DVECTOR *b)= NULL;

// matrix routines
float (*fast_det_3x3)(const float *mat)= NULL;
void (*fast_inverse_3x3)(float *res, const float *a)= NULL;
void (*fast_submat_4x4)(float *mb, const D3DMATRIX *mr, int i, int j)= NULL;
float (*fast_det_4x4)(const D3DMATRIX *mr)= NULL;
int (*fast_inverse_4x4)(D3DMATRIX *mr, const D3DMATRIX *ma)= NULL;
void (*fast_glMul_3x3)(float *r, const float *a, const float *b)= NULL;
void (*fast_glMul_4x4)(float *r, const float *a, const float *b)= NULL;
void (*fast_D3DMul_4x4)(D3DMATRIX *r, const D3DMATRIX *a, const D3DMATRIX *b)= NULL;

static int processor_code= _processor_unknown;

/*---------- code */

int bfw_math_initialize(
	void)
{
	static int initialized= FALSE;
	int success;

	if (!initialized)
	{
		switch (determine_processor())
		{
			case _processor_unknown:
				success= load_stdc_math();
				break;
			case _processor_x86:
				success= load_stdc_math();
				break;
			case _processor_x86_mmx:
				success= load_stdc_math();
				break;
			case _processor_x86_sse:
				success= load_stdc_math();
				break;
			case _processor_x86_3dnow:
				success= load_3dnow_math();
				if (!success)
				{
					success= load_stdc_math();
				}
				break;
			case _processor_ppc:
				success= load_stdc_math();
				break;
			case _processor_ppc_alitvec:
				success= load_stdc_math();
				break;
			default:
				success= load_stdc_math();
				break;
		}

		if (success)
		{
			success= load_ansi_math();
		}

		initialized= success;
	}
	else
	{
		success= FALSE;
	}

	return success;
}

int bfw_math_terminate(
	void)
{
	int success= TRUE;

	return success;
}

int bfw_math_get_processor_code(
	void)
{
	return processor_code;
}

/*---------- private code */

#ifdef _WINDOWS_

/* Symbolic constants for feature flags in CPUID standard feature flags */

#define CPUID_STD_FPU          0x00000001
#define CPUID_STD_VME          0x00000002
#define CPUID_STD_DEBUGEXT     0x00000004
#define CPUID_STD_4MPAGE       0x00000008
#define CPUID_STD_TSC          0x00000010
#define CPUID_STD_MSR          0x00000020
#define CPUID_STD_PAE          0x00000040
#define CPUID_STD_MCHKXCP      0x00000080
#define CPUID_STD_CMPXCHG8B    0x00000100
#define CPUID_STD_APIC         0x00000200
#define CPUID_STD_SYSENTER     0x00000800
#define CPUID_STD_MTRR         0x00001000
#define CPUID_STD_GPE          0x00002000
#define CPUID_STD_MCHKARCH     0x00004000
#define CPUID_STD_CMOV         0x00008000
#define CPUID_STD_PAT          0x00010000
#define CPUID_STD_PSE36        0x00020000
#define CPUID_STD_MMX          0x00800000
#define CPUID_STD_FXSAVE       0x01000000
#define CPUID_STD_SSE          0x02000000

/* Symbolic constants for feature flags in CPUID extended feature flags */

#define CPUID_EXT_3DNOW        0x80000000
#define CPUID_EXT_AMD_3DNOWEXT 0x40000000
#define CPUID_EXT_AMD_MMXEXT   0x00400000

/* Symbolic constants for application specific feature flags */

#define FEATURE_CPUID          0x00000001
#define FEATURE_STD_FEATURES   0x00000002
#define FEATURE_EXT_FEATURES   0x00000004
#define FEATURE_TSC            0x00000010
#define FEATURE_MMX            0x00000020
#define FEATURE_CMOV           0x00000040
#define FEATURE_3DNOW          0x00000080
#define FEATURE_3DNOWEXT       0x00000100
#define FEATURE_MMXEXT         0x00000200
#define FEATURE_SSEFP          0x00000400
#define FEATURE_K6_MTRR        0x00000800
#define FEATURE_P6_MTRR        0x00001000

// older compilers do not support the CPUID instruction in inline assembly
#define cpuid _asm _emit 0x0f _asm _emit 0xa2

static unsigned int get_feature_flags(
	void)
{
   unsigned int result= 0;
   unsigned int signature= 0;
   char vendor[13]= "UnknownVendr";  /* Needs to be exactly 12 chars */

   /* Define known vendor strings here */

   char vendorAMD[13]= "AuthenticAMD";  /* Needs to be exactly 12 chars */
	/*;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;; Step 1: Check if processor has CPUID support. Processor faults
         ;; with an illegal instruction exception if the instruction is not
         ;; supported. This step catches the exception and immediately returns
         ;; with feature string bits with all 0s, if the exception occurs.
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;*/
	__try {
		__asm xor    eax, eax
		__asm xor    ebx, ebx
		__asm xor    ecx, ecx
		__asm xor    edx, edx
		__asm cpuid
	}

	__except (EXCEPTION_EXECUTE_HANDLER) {
		return (0);
	}

	result |= FEATURE_CPUID;

	_asm {

         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;; Step 2: Check if CPUID supports function 1 (signature/std features)
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         xor     eax, eax                      ; CPUID function #0
         cpuid                                 ; largest std func/vendor string
         mov     dword ptr [vendor], ebx       ; save
         mov     dword ptr [vendor+4], edx     ;  vendor
         mov     dword ptr [vendor+8], ecx     ;   string
         test    eax, eax                      ; largest standard function==0?
         jz      $no_standard_features         ; yes, no standard features func
         or      [result], FEATURE_STD_FEATURES; does have standard features

         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;; Step 3: Get standard feature flags and signature
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         mov     eax, 1                        ; CPUID function #1
         cpuid                                 ; get signature/std feature flgs
         mov     [signature], eax              ; save processor signature

         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;; Step 4: Extract desired features from standard feature flags
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         ;; Check for time stamp counter support

         mov     ecx, CPUID_STD_TSC            ; bit 4 indicates TSC support
         and     ecx, edx                      ; supports TSC ? CPUID_STD_TSC:0
         neg     ecx                           ; supports TSC ? CY : NC
         sbb     ecx, ecx                      ; supports TSC ? 0xffffffff:0
         and     ecx, FEATURE_TSC              ; supports TSC ? FEATURE_TSC:0
         or      [result], ecx                 ; merge into feature flags

         ;; Check for MMX support

         mov     ecx, CPUID_STD_MMX            ; bit 23 indicates MMX support
         and     ecx, edx                      ; supports MMX ? CPUID_STD_MMX:0
         neg     ecx                           ; supports MMX ? CY : NC
         sbb     ecx, ecx                      ; supports MMX ? 0xffffffff:0
         and     ecx, FEATURE_MMX              ; supports MMX ? FEATURE_MMX:0
         or      [result], ecx                 ; merge into feature flags

         ;; Check for CMOV support

         mov     ecx, CPUID_STD_CMOV           ; bit 15 indicates CMOV support
         and     ecx, edx                      ; supports CMOV?CPUID_STD_CMOV:0
         neg     ecx                           ; supports CMOV ? CY : NC
         sbb     ecx, ecx                      ; supports CMOV ? 0xffffffff:0
         and     ecx, FEATURE_CMOV             ; supports CMOV ? FEATURE_CMOV:0
         or      [result], ecx                 ; merge into feature flags

         ;; Check support for P6-style MTRRs

         mov     ecx, CPUID_STD_MTRR           ; bit 12 indicates MTRR support
         and     ecx, edx                      ; supports MTRR?CPUID_STD_MTRR:0
         neg     ecx                           ; supports MTRR ? CY : NC
         sbb     ecx, ecx                      ; supports MTRR ? 0xffffffff:0
         and     ecx, FEATURE_P6_MTRR          ; supports MTRR ? FEATURE_MTRR:0
         or      [result], ecx                 ; merge into feature flags

         ;; Check for initial SSE support. There can still be partial SSE
         ;; support. Step 9 will check for partial support.

         mov     ecx, CPUID_STD_SSE            ; bit 25 indicates SSE support
         and     ecx, edx                      ; supports SSE ? CPUID_STD_SSE:0
         neg     ecx                           ; supports SSE ? CY : NC
         sbb     ecx, ecx                      ; supports SSE ? 0xffffffff:0
         and     ecx, (FEATURE_MMXEXT+FEATURE_SSEFP) ; supports SSE ?
                                               ; FEATURE_MMXEXT+FEATURE_SSEFP:0
         or      [result], ecx                 ; merge into feature flags

         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;; Step 5: Check for CPUID extended functions
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         mov     eax, 0x80000000               ; extended function 0x80000000
         cpuid                                 ; largest extended function
         cmp     eax, 0x80000000               ; no function > 0x80000000 ?
         jbe     $no_extended_features         ; yes, no extended feature flags
         or      [result], FEATURE_EXT_FEATURES; does have ext. feature flags

         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;; Step 6: Get extended feature flags
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         mov     eax, 0x80000001               ; CPUID ext. function 0x80000001
         cpuid                                 ; EDX = extended feature flags

         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;; Step 7: Extract vendor independent features from extended flags
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         ;; Check for 3DNow! support (vendor independent)

         mov     ecx, CPUID_EXT_3DNOW          ; bit 31 indicates 3DNow! supprt
         and     ecx, edx                      ; supp 3DNow! ?CPUID_EXT_3DNOW:0
         neg     ecx                           ; supports 3DNow! ? CY : NC
         sbb     ecx, ecx                      ; supports 3DNow! ? 0xffffffff:0
         and     ecx, FEATURE_3DNOW            ; support 3DNow!?FEATURE_3DNOW:0
         or      [result], ecx                 ; merge into feature flags

         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;; Step 8: Determine CPU vendor
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         lea     esi, vendorAMD                ; AMDs vendor string
         lea     edi, vendor                   ; this CPUs vendor string
         mov     ecx, 12                       ; strings are 12 characters
         cld                                   ; compare lowest to highest
         repe    cmpsb                         ; current vendor str == AMDs ?
         jnz     $not_AMD                      ; no, CPU vendor is not AMD

         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;; Step 9: Check AMD specific extended features
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         mov     ecx, CPUID_EXT_AMD_3DNOWEXT   ; bit 30 indicates 3DNow! ext.
         and     ecx, edx                      ; 3DNow! ext?
         neg     ecx                           ; 3DNow! ext ? CY : NC
         sbb     ecx, ecx                      ; 3DNow! ext ? 0xffffffff : 0
         and     ecx, FEATURE_3DNOWEXT         ; 3DNow! ext?FEATURE_3DNOWEXT:0
         or      [result], ecx                 ; merge into feature flags

         test    [result], FEATURE_MMXEXT      ; determined SSE MMX support?
         jnz     $has_mmxext                   ; yes, do not need to check again

         ;; Check support for AMDs multimedia instruction set additions

         mov     ecx, CPUID_EXT_AMD_MMXEXT     ; bit 22 indicates MMX extension
         and     ecx, edx                      ; MMX ext?CPUID_EXT_AMD_MMXEXT:0
         neg     ecx                           ; MMX ext? CY : NC
         sbb     ecx, ecx                      ; MMX ext? 0xffffffff : 0
         and     ecx, FEATURE_MMXEXT           ; MMX ext ? FEATURE_MMXEXT:0
         or      [result], ecx                 ; merge into feature flags

      $has_mmxext:

         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;; Step 10: Check AMD-specific features not reported by CPUID
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         ;; Check support for AMD-K6 processor-style MTRRs

         mov     eax, [signature] 	; get processor signature
         and     eax, 0xFFF 		; extract family/model/stepping
         cmp     eax, 0x588 		; CPU < AMD-K6-2/CXT ? CY : NC
         sbb     edx, edx 		; CPU < AMD-K6-2/CXT ? 0xffffffff:0
         not     edx 			; CPU < AMD-K6-2/CXT ? 0:0xffffffff
         cmp     eax, 0x600 		; CPU < AMD Athlon ? CY : NC
         sbb     ecx, ecx 		; CPU < AMD-K6 ? 0xffffffff:0
         and     ecx, edx 		; (CPU>=AMD-K6-2/CXT)&&
					; (CPU<AMD Athlon) ? 0xffffffff:0
         and     ecx, FEATURE_K6_MTRR 	; (CPU>=AMD-K6-2/CXT)&&
					; (CPU<AMD Athlon) ? FEATURE_K6_MTRR:0
         or      [result], ecx 		; merge into feature flags

         jmp     $all_done 		; desired features determined

      $not_AMD:

         /* Extract features specific to non AMD CPUs */

      $no_extended_features:
      $no_standard_features:
      $all_done:
   }

   /* The FP part of SSE introduces a new architectural state and therefore
      requires support from the operating system. So even if CPUID indicates
      support for SSE FP, the application might not be able to use it. If
      CPUID indicates support for SSE FP, check here whether it is also
      supported by the OS, and turn off the SSE FP feature bit if there
      is no OS support for SSE FP.

      Operating systems that do not support SSE FP return an illegal
      instruction exception if execution of an SSE FP instruction is performed.
      Here, a sample SSE FP instruction is executed, and is checked for an
      exception using the (non-standard) __try/__except mechanism
      of Microsoft Visual C.
   */

   if (result & FEATURE_SSEFP) {
       __try {
          __asm _emit 0x0f
          __asm _emit 0x56
          __asm _emit 0xC0    ;; orps xmm0, xmm0
          return (result);
       }
       __except (EXCEPTION_EXECUTE_HANDLER) {
          return (result & (~FEATURE_SSEFP));
       }
   }
   else {
      return (result);
   }
}

static int supports_mmx(
	void)
{
	unsigned long features= get_feature_flags();

	return (features & FEATURE_MMX);
}

static int supports_sse(
	void)
{
	unsigned long features= get_feature_flags();

	return (features & FEATURE_SSEFP);
}

static int supports_3dnow(
	void)
{
	unsigned long features= get_feature_flags();

	return (features & FEATURE_3DNOW);
}

#endif // windows

static int determine_processor(
	void)
{
	processor_code= _processor_unknown;

#ifdef macintosh
	processor_code= _processor_ppc;
#endif

#ifdef _WINDOWS_
	if (supports_sse())
	{
		processor_code= _processor_x86_sse;
	}
	else if (supports_3dnow())
	{
		processor_code= _processor_x86_3dnow;
	}
	else if (supports_mmx())
	{
		processor_code= _processor_x86_mmx;
	}
	else
	{
		processor_code= _processor_x86;
	}
#endif

	return processor_code;
}


#ifdef __cplusplus
}
#endif

#endif // USE_FAST_MATH
