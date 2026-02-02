/*
	bfw_math.h
	Wednesday Sept. 20, 2000   2:51am
	Stefan
*/

// define USE_FAST_MATH in the project if you want to use the fast math code
#ifndef USE_FAST_MATH

#include <math.h>

#define bfw_math_initialize()	1
#define bfw_math_terminate()	1

#else

#ifndef __BFW_MATH_H__
#define __BFW_MATH_H__

// define USE_FAST_TRIG if you want to use fast trig functions (less accurate though)
//#define USE_FAST_TRIG

#ifdef _WINDOWS_

#include <windows.h>
#include "amdmath_dll.h"
#include <d3d.h>
#include <d3drm.h>

#else

typedef struct _D3DVECTOR {
    union {
	float x;
	float dvX;
    };
    union {
	float y;
	float dvY;
    };
    union {
	float z;
	float dvZ;
    };
} D3DVECTOR, *LPD3DVECTOR;

typedef struct _D3DRMQUATERNION
{   float s;
    D3DVECTOR v;
} D3DRMQUATERNION, *LPD3DRMQUATERNION;

typedef union _D3DMATRIX {
    struct {
            float        _11, _12, _13, _14;
            float        _21, _22, _23, _24;
            float        _31, _32, _33, _34;
            float        _41, _42, _43, _44;
        };
        float m[4][4];
} D3DMATRIX, *LPD3DMATRIX;

#endif

#ifdef __cplusplus
extern "C" {
#endif

/*---------- headers */

/*---------- constants */

enum {
	_processor_unknown, // PS2

	_processor_x86, // generic x86
	_processor_x86_mmx, // Pentium w/ MMX extensions
	_processor_x86_sse, // Pentium III w/ SSE
	_processor_x86_3dnow, // AMD w/ 3DNow!

	_processor_ppc, // generic powerpc
	_processor_ppc_alitvec, // powerpc G4

	NUMBER_OF_PROCESSOR_CODES
};

/*---------- macros */

#undef atan
#undef atan2
#undef acos
#undef asin
#undef log
#undef log10
#undef pow
#undef exp
#undef sqrt
#undef fabs
#undef ceil
#undef floor
#undef frexp
#undef ldexp
#undef modf
#undef fmod
#undef sin
#undef cos
#undef tan
#ifdef USE_FAST_TRIG
// replace these standard math functions (yes you will get warnings if you use doubles! this is intentional)
#define atan(x)		fast_atan(x)
#define atan2(x,y)	fast_atan2((x),(y))
#define acos(x)		fast_acos(x)
#define asin(x)		fast_asin(x)
#define log(x)		fast_log(x)
#define log10(x)	fast_log10(x)
#define pow(x,y)	fast_pow((x),(y))
#define exp(x)		fast_exp(x)
#define sqrt(x)		fast_sqrt(x)
#define fabs(x)		fast_fabs(x)
#define ceil(x)		fast_ceil(x)
#define floor(x)	fast_floor(x)
#define frexp(x,y)	fast_frexp((x),(y))
#define ldexp(x,y)	fast_ldexp((x),(y))
#define modf(x,y)	fast_modf((x),(y))
#define fmod(x,y)	fast_fmod((x),(y))
#define sin(x)		fast_sin(x)
#define cos(x)		fast_cos(x)
#define tan(x)		fast_tan(x)
#else // use standard ANSI lib functions; no speed penalty, these macros resolve to the actual library function
#define atan(x)		ansi_atan(x)
#define atan2(x,y)	ansi_atan2((x),(y))
#define acos(x)		ansi_acos(x)
#define asin(x)		ansi_asin(x)
#define log(x)		ansi_log(x)
#define log10(x)	ansi_log10(x)
#define pow(x,y)	ansi_pow((x),(y))
#define exp(x)		ansi_exp(x)
#define sqrt(x)		ansi_sqrt(x)
#define fabs(x)		ansi_fabs(x)
#define ceil(x)		ansi_ceil(x)
#define floor(x)	ansi_floor(x)
#define frexp(x,y)	ansi_frexp((x),(y))
#define ldexp(x,y)	ansi_ldexp((x),(y))
#define modf(x,y)	ansi_modf((x),(y))
#define fmod(x,y)	ansi_fmod((x),(y))
#define sin(x)		ansi_sin(x)
#define cos(x)		ansi_cos(x)
#define tan(x)		ansi_tan(x)
#endif

// non-standard
#define sincos(x,y)	fast_sincos(x, y)

/*---------- globals */

// use these if you want the ANSI C function (these are aliases to the standard C math lib functions)
extern double (*ansi_atan)(double x);
extern double (*ansi_atan2)(double x, double y);
extern double (*ansi_acos)(double x);
extern double (*ansi_asin)(double x);
extern double (*ansi_log)(double x);
extern double (*ansi_log10)(double x);
extern double (*ansi_pow)(double x, double y);
extern double (*ansi_exp)(double x);
extern double (*ansi_sqrt)(double x);
extern double (*ansi_fabs)(double x);
extern double (*ansi_ceil)(double x);
extern double (*ansi_floor)(double x);
extern double (*ansi_frexp)(double x, int *y);
extern double (*ansi_ldexp)(double x, int y);
extern double (*ansi_modf)(double x, double *y);
extern double (*ansi_fmod)(double x, double y);
extern double (*ansi_sin)(double x);
extern double (*ansi_cos)(double x);
extern double (*ansi_tan)(double x);

// replace the standard math functions w/ these
extern float (*fast_atan)(float x);
extern float (*fast_atan2)(float x, float y);
extern float (*fast_acos)(float x);
extern float (*fast_asin)(float x);
extern float (*fast_log)(float x);
extern float (*fast_log10)(float x);
extern float (*fast_pow)(float x, float y);
extern float (*fast_exp)(float x);
extern float (*fast_sqrt)(float x);
extern float (*fast_fabs)(float x);
extern float (*fast_ceil)(float x);
extern float (*fast_floor)(float x);
extern float (*fast_frexp)(float x, int *y);
extern float (*fast_ldexp)(float x, int y);
extern float (*fast_modf)(float x, float *y);
extern float (*fast_fmod)(float x, float y);
extern float (*fast_sin)(float x);
extern float (*fast_cos)(float x);
extern float (*fast_tan)(float x);

extern void (*fast_sincos)(float x, float *cs); // cs[0]= cos(x), cs[1]= sin(x)

// quaternion fxns
extern void (*fast_add_quat)(D3DRMQUATERNION *r, const D3DRMQUATERNION *a, const D3DRMQUATERNION *b);
extern void (*fast_sub_quat)(D3DRMQUATERNION *r, const D3DRMQUATERNION *a, const D3DRMQUATERNION *b);
extern void (*fast_mult_quat)(D3DRMQUATERNION *r, const D3DRMQUATERNION *a, const D3DRMQUATERNION *b);
extern void (*fast_norm_quat)(D3DRMQUATERNION *dst, const D3DRMQUATERNION *a);
extern void (*fast_D3DMat2quat)(D3DRMQUATERNION *quat, const D3DMATRIX *m);
extern void (*fast_glMat2quat)(D3DRMQUATERNION *quat, const float *m);
extern void (*fast_quat2D3DMat)(D3DMATRIX *m, const D3DRMQUATERNION *quat);
extern void (*fast_quat2glMat)(float *m, const D3DRMQUATERNION *quat);
extern void (*fast_euler2quat)(D3DRMQUATERNION *quat, const float *rot);
extern void (*fast_euler2quat2)(D3DRMQUATERNION *quat, const float *rot);
extern void (*fast_quat2axis_angle)(float *axisAngle, const D3DRMQUATERNION *quat);
extern void (*fast_axis_angle2quat)(D3DRMQUATERNION *quat, const float *axisAngle);
extern void (*fast_slerp_quat)(D3DRMQUATERNION *result, const D3DRMQUATERNION *quat1, const D3DRMQUATERNION *quat2, float slerp);
extern void (*fast_trans_quat)(D3DVECTOR *result, const D3DVECTOR *v, const D3DRMQUATERNION *q);

// vector fxns
extern void (*fast_add_vect)(D3DVECTOR *r, const D3DVECTOR *a, const D3DVECTOR *b);
extern void (*fast_sub_vect)(D3DVECTOR *r, const D3DVECTOR *a, const D3DVECTOR *b);
extern void (*fast_mult_vect)(D3DVECTOR *r, const D3DVECTOR *a, const D3DVECTOR *b);
extern void (*fast_scale_vect)(D3DVECTOR *r, const D3DVECTOR *a, float b);
extern void (*fast_norm_vect)(D3DVECTOR *r, const D3DVECTOR *a);
extern float (*fast_mag_vect)(const D3DVECTOR *a);
extern float (*fast_dot_vect)(const D3DVECTOR *a, const D3DVECTOR *b);
extern void (*fast_cross_vect)(D3DVECTOR *r, const D3DVECTOR *a, const D3DVECTOR *b);

// matrix routines
extern float (*fast_det_3x3)(const float *mat);
extern void (*fast_inverse_3x3)(float *res, const float *a);
extern void (*fast_submat_4x4)(float *mb, const D3DMATRIX *mr, int i, int j);
extern float (*fast_det_4x4)(const D3DMATRIX *mr);
extern int (*fast_inverse_4x4)(D3DMATRIX *mr, const D3DMATRIX *ma);
extern void (*fast_glMul_3x3)(float *r, const float *a, const float *b);
extern void (*fast_glMul_4x4)(float *r, const float *a, const float *b);
extern void (*fast_D3DMul_4x4)(D3DMATRIX *r, const D3DMATRIX *a, const D3DMATRIX *b);

/*---------- prototypes */

// bfw_math.c
int bfw_math_initialize(void);
int bfw_math_terminate(void);
int bfw_math_get_processor_code(void);

// bfw_math_stdc.c
int load_stdc_math(void);
void unload_stdc_math(void);
int load_ansi_math(void);

// bfw_math_3dnow.c
int load_3dnow_math(void);
void unload_3dnow_math(void);

#ifdef __cplusplus
}
#endif

#endif // __BFW_MATH_H__

#endif // USE_FAST_MATH

