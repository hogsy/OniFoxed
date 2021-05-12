/*
	bfw_math_stdc.c
	Wednesday Sept. 20, 2000   4:19 am
	Stefan
*/

#ifdef USE_FAST_MATH

/*---------- headers */

// Oni's header-nightmare strikes again
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

#include <math.h>

/*---------- structures */

#ifdef _WINDOWS_

#include <d3dtypes.h>
#include <d3drmdef.h>

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

#undef TRUE
#undef FALSE
enum {
	FALSE= 0,
	TRUE
};

#ifdef __cplusplus
extern "C" {
#endif

/*---------- prototypes */

static float stdc_atan(float x);
static float stdc_atan2(float x, float y);
static float stdc_acos(float x);
static float stdc_asin(float x);
static float stdc_log(float x);
static float stdc_log10(float x);
static float stdc_pow(float x, float y);
static float stdc_exp(float x);
static float stdc_sqrt(float x);
static float stdc_fabs(float x);
static float stdc_ceil(float x);
static float stdc_floor(float x);
static float stdc_frexp(float x, int *y);
static float stdc_ldexp(float x, int y);
static float stdc_modf(float x, float *y);
static float stdc_fmod(float x, float y);
static float stdc_sin(float x);
static float stdc_cos(float x);
static float stdc_tan(float x);
static void stdc_sincos(float x, float *cs);

// quaternion fxns
static void stdc_add_quat(D3DRMQUATERNION *r, const D3DRMQUATERNION *a, const D3DRMQUATERNION *b);
static void stdc_sub_quat(D3DRMQUATERNION *r, const D3DRMQUATERNION *a, const D3DRMQUATERNION *b);
static void stdc_mult_quat(D3DRMQUATERNION *r, const D3DRMQUATERNION *a, const D3DRMQUATERNION *b);
static void stdc_norm_quat(D3DRMQUATERNION *dst, const D3DRMQUATERNION *a);
static void stdc_D3DMat2quat(D3DRMQUATERNION *quat, const D3DMATRIX *m);
static void stdc_glMat2quat(D3DRMQUATERNION *quat, const float *m);
static void stdc_quat2D3DMat(D3DMATRIX *m, const D3DRMQUATERNION *quat);
static void stdc_quat2glMat(float *m, const D3DRMQUATERNION *quat);
static void stdc_euler2quat(D3DRMQUATERNION *quat, const float *rot);
static void stdc_euler2quat2(D3DRMQUATERNION *quat, const float *rot);
static void stdc_quat2axis_angle(float *axisAngle, const D3DRMQUATERNION *quat);
static void stdc_axis_angle2quat(D3DRMQUATERNION *quat, const float *axisAngle);
static void stdc_slerp_quat(D3DRMQUATERNION *result, const D3DRMQUATERNION *quat1, const D3DRMQUATERNION *quat2, float slerp);
static void stdc_trans_quat(D3DVECTOR *result, const D3DVECTOR *v, const D3DRMQUATERNION *q);

// vector fxns
static float stdc_mag_vect(const D3DVECTOR *a);
static float stdc_dot_vect(const D3DVECTOR *a, const D3DVECTOR *b);
static void stdc_add_vect(D3DVECTOR *r, const D3DVECTOR *a, const D3DVECTOR *b);
static void stdc_sub_vect(D3DVECTOR *r, const D3DVECTOR *a, const D3DVECTOR *b);
static void stdc_mult_vect(D3DVECTOR *r, const D3DVECTOR *a, const D3DVECTOR *b);
static void stdc_scale_vect(D3DVECTOR *r, const D3DVECTOR *a, float b);
static void stdc_norm_vect(D3DVECTOR *r, const D3DVECTOR *a);
static float stdc_mag_vect(const D3DVECTOR *a);
static float stdc_dot_vect(const D3DVECTOR *a, const D3DVECTOR *b);
static void stdc_cross_vect(D3DVECTOR *r, const D3DVECTOR *a, const D3DVECTOR *b);

// matrix routines
static float stdc_det_3x3(const float *mat);
static void stdc_inverse_3x3(float *res, const float *a);
static void stdc_submat_4x4(float *mb, const D3DMATRIX *mr, int i, int j);
static float stdc_det_4x4(const D3DMATRIX *mr);
static int stdc_inverse_4x4(D3DMATRIX *mr, const D3DMATRIX *ma);
static void stdc_glMul_3x3(float *r, const float *a, const float *b);
static void stdc_glMul_4x4(float *r, const float *a, const float *b);
static void stdc_D3DMul_4x4(D3DMATRIX *r, const D3DMATRIX *a, const D3DMATRIX *b);

/*---------- globals */

// from bfw_math.h (we can't include the header file in this case)
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

extern float (*fast_atan)(float);
extern float (*fast_atan2)(float,float);
extern float (*fast_acos)(float);
extern float (*fast_asin)(float);
extern float (*fast_log)(float);
extern float (*fast_log10)(float);
extern float (*fast_pow)(float, float);
extern float (*fast_exp)(float);
extern float (*fast_sqrt)(float);
extern float (*fast_fabs)(float);
extern float (*fast_ceil)(float);
extern float (*fast_floor)(float);
extern float (*fast_frexp)(float, int *);
extern float (*fast_ldexp)(float, int);
extern float (*fast_modf)(float, float *);
extern float (*fast_fmod)(float, float);
extern float (*fast_sin)(float);
extern float (*fast_cos)(float);
extern float (*fast_tan)(float);

extern void (*fast_sincos)(float x, float *cs);

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
extern float (*fast_mag_vect)(const D3DVECTOR *a);
extern float (*fast_dot_vect)(const D3DVECTOR *a, const D3DVECTOR *b);
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

#define DEG2RAD 0.017453292519943295769236907684886f
#define RAD2DEG 57.295779513082320876798154814105f
#define HALF_PI 1.5707963267948966192313216916398f
#define DELTA   0.0001      // DIFFERENCE AT WHICH TO LERP INSTEAD OF SLERP

/*---------- code */

#define LOAD_MATH_FXN(func,name)	((func= (void *)GetProcAddress(dll, #name)) != NULL)
int load_stdc_math(
	void)
{
	fast_atan= stdc_atan;
	fast_atan2= stdc_atan2;
	fast_acos= stdc_acos;
	fast_asin= stdc_asin;
	fast_log= stdc_log;
	fast_log10= stdc_log10;
	fast_pow= stdc_pow;
	fast_exp= stdc_exp;
	fast_sqrt= stdc_sqrt;
	fast_fabs= stdc_fabs;
	fast_ceil= stdc_ceil;
	fast_floor= stdc_floor;
	fast_frexp= stdc_frexp;
	fast_ldexp= stdc_ldexp;
	fast_modf= stdc_modf;
	fast_fmod= stdc_fmod;
	fast_sin= stdc_sin;
	fast_cos= stdc_cos;
	fast_tan= stdc_tan;
	fast_sincos= stdc_sincos;

	// quaternion fxns
	fast_add_quat= stdc_add_quat;
	fast_sub_quat= stdc_sub_quat;
	fast_mult_quat= stdc_mult_quat;
	fast_norm_quat= stdc_norm_quat;
	fast_D3DMat2quat= stdc_D3DMat2quat;
	fast_glMat2quat= stdc_glMat2quat;
	fast_quat2D3DMat= stdc_quat2D3DMat;
	fast_quat2glMat= stdc_quat2glMat;
	fast_euler2quat= stdc_euler2quat;
	fast_euler2quat2= stdc_euler2quat2;
	fast_quat2axis_angle= stdc_quat2axis_angle;
	fast_axis_angle2quat= stdc_axis_angle2quat;
	fast_slerp_quat= stdc_slerp_quat;
	fast_trans_quat= stdc_trans_quat;

	// vector fxns
	fast_mag_vect= stdc_mag_vect;
	fast_dot_vect= stdc_dot_vect;
	fast_add_vect= stdc_add_vect;
	fast_sub_vect= stdc_sub_vect;
	fast_mult_vect= stdc_mult_vect;
	fast_scale_vect= stdc_scale_vect;
	fast_norm_vect= stdc_norm_vect;
	fast_mag_vect= stdc_mag_vect;
	fast_dot_vect= stdc_dot_vect;
	fast_cross_vect= stdc_cross_vect;

	// matrix routines
	fast_det_3x3= stdc_det_3x3;
	fast_inverse_3x3= stdc_inverse_3x3;
	fast_submat_4x4= stdc_submat_4x4;
	fast_det_4x4= stdc_det_4x4;
	fast_inverse_4x4= stdc_inverse_4x4;
	fast_glMul_3x3= stdc_glMul_3x3;
	fast_glMul_4x4= stdc_glMul_4x4;
	fast_D3DMul_4x4= stdc_D3DMul_4x4;

//	UUrStartupMessage("standard C math loaded");

	return TRUE;
}

void unload_stdc_math(
	void)
{
	fast_atan= NULL;
	fast_atan2= NULL;
	fast_acos= NULL;
	fast_asin= NULL;
	fast_log= NULL;
	fast_log10= NULL;
	fast_pow= NULL;
	fast_exp= NULL;
	fast_sqrt= NULL;
	fast_fabs= NULL;
	fast_ceil= NULL;
	fast_floor= NULL;
	fast_frexp= NULL;
	fast_ldexp= NULL;
	fast_modf= NULL;
	fast_fmod= NULL;
	fast_sin= NULL;
	fast_cos= NULL;
	fast_tan= NULL;
	fast_sincos= NULL;

	// quaternion fxns
	fast_add_quat= NULL;
	fast_sub_quat= NULL;
	fast_mult_quat= NULL;
	fast_norm_quat= NULL;
	fast_D3DMat2quat= NULL;
	fast_glMat2quat= NULL;
	fast_quat2D3DMat= NULL;
	fast_quat2glMat= NULL;
	fast_euler2quat= NULL;
	fast_euler2quat2= NULL;
	fast_quat2axis_angle= NULL;
	fast_axis_angle2quat= NULL;
	fast_slerp_quat= NULL;
	fast_trans_quat= NULL;

	// vector fxns
	fast_mag_vect= NULL;
	fast_dot_vect= NULL;
	fast_add_vect= NULL;
	fast_sub_vect= NULL;
	fast_mult_vect= NULL;
	fast_scale_vect= NULL;
	fast_norm_vect= NULL;
	fast_mag_vect= NULL;
	fast_dot_vect= NULL;
	fast_cross_vect= NULL;

	// matrix routines
	fast_det_3x3= NULL;
	fast_inverse_3x3= NULL;
	fast_submat_4x4= NULL;
	fast_det_4x4= NULL;
	fast_inverse_4x4= NULL;
	fast_glMul_3x3= NULL;
	fast_glMul_4x4= NULL;
	fast_D3DMul_4x4= NULL;

	return;
}

int load_ansi_math(
	void)
{
	ansi_atan= atan;
	ansi_atan2= atan2;
	ansi_acos= acos;
	ansi_asin= asin;
	ansi_log= log;
	ansi_log10= log10;
	ansi_pow= pow;
	ansi_exp= exp;
	ansi_sqrt= sqrt;
	ansi_fabs= fabs;
	ansi_ceil= ceil;
	ansi_floor= floor;
	ansi_frexp= frexp;
	ansi_ldexp= ldexp;
	ansi_modf= modf;
	ansi_fmod= fmod;
	ansi_sin= sin;
	ansi_cos= cos;
	ansi_tan= tan;

	return TRUE;
}

/*---------- private code */

static float stdc_atan(
	float x)
{
	return (float)atan(x);
}

static float stdc_atan2(
	float x,
	float y)
{
	return (float)atan2(x,y);
}

static float stdc_acos(
	float x)
{
	return (float)acos(x);
}

static float stdc_asin(
	float x)
{
	return (float)asin(x);
}

static float stdc_log(
	float x)
{
	return (float)log(x);
}

static float stdc_log10(
	float x)
{
	return (float)log10(x);
}

static float stdc_pow(
	float x,
	float y)
{
	return (float)pow(x,y);
}

static float stdc_exp(
	float x)
{
	return (float)exp(x);
}

static float stdc_sqrt(
	float x)
{
	return (float)sqrt(x);
}

static float stdc_fabs(
	float x)
{
	return (float)fabs(x);
}

static float stdc_ceil(
	float x)
{
	return (float)ceil(x);
}

static float stdc_floor(
	float x)
{
	return (float)floor(x);
}

static float stdc_frexp(
	float x,
	int *y)
{
	return (float)frexp(x,y);
}

static float stdc_ldexp(
	float x,
	int y)
{
	return (float)ldexp(x,y);
}

static float stdc_modf(
	float x,
	float *y)
{
	double n;
	float f;

	f= (float)modf(x,&n);
	*y= (float)n;

	return f;
}

static float stdc_fmod(
	float x,
	float y)
{
	return (float)fmod(x,y);
}

static float stdc_sin(
	float x)
{
	return (float)sin(x);
}

static float stdc_cos(
	float x)
{
	return (float)cos(x);
}

static float stdc_tan(
	float x)
{
	return (float)tan(x);
}

static void stdc_sincos(
	float x,
	float *cs)
{
	register double angle= x;
	cs[0]= (float)cos(angle);
	cs[1]= (float)sin(angle);

	return;
}

static void stdc_add_quat(
	D3DRMQUATERNION *r,
	const D3DRMQUATERNION *a,
	const D3DRMQUATERNION *b)
{
	r->s= a->s + b->s;
    r->v.x= a->v.x + b->v.x;
    r->v.y= a->v.y + b->v.y;
    r->v.z= a->v.z + b->v.z;

	return;
}

static void stdc_sub_quat(
	D3DRMQUATERNION *r,
	const D3DRMQUATERNION *a,
	const D3DRMQUATERNION *b)
{
	r->s= a->s - b->s;
    r->v.x= a->v.x - b->v.x;
    r->v.y= a->v.y - b->v.y;
    r->v.z = a->v.z - b->v.z;

	return;
}

static void stdc_mult_quat(
	D3DRMQUATERNION *r,
	const D3DRMQUATERNION *a,
	const D3DRMQUATERNION *b)
{
	D3DRMQUATERNION tmp;

    tmp.s= a->s * b->s - a->v.x * b->v.x - a->v.y * b->v.y - a->v.z * b->v.z;
    tmp.v.x= a->v.x * b->s + b->v.x * a->s   + a->v.y * b->v.z - a->v.z * b->v.y;
    tmp.v.y= a->v.y * b->s + b->v.y * a->s   + a->v.z * b->v.x - a->v.x * b->v.z;
    tmp.v.z= a->v.z * b->s + b->v.z * a->s   + a->v.x * b->v.y - a->v.y * b->v.x;

    *r= tmp;

	return;
}

static void stdc_norm_quat(
	D3DRMQUATERNION *dst,
	const D3DRMQUATERNION *a)
{
	const float quatx= a->v.x,
                quaty= a->v.y,
                quatz= a->v.z,
                quatw= a->s;
    const float magnitude= 1.0f / stdc_sqrt((quatx * quatx) + (quaty * quaty) + (quatz * quatz) + (quatw * quatw));

    dst->v.x= quatx * magnitude;
    dst->v.y= quaty * magnitude;
    dst->v.z= quatz * magnitude;
    dst->s= quatw * magnitude;

	return;
}

static void stdc_D3DMat2quat(
	D3DRMQUATERNION *quat,
	const D3DMATRIX *m)
{
	float *mm= (float *)&m;
    float tr, s;

    tr= m->_11 + m->_22 + m->_33;

    // check the diagonal
    if (tr > 0.0)
    {
        s= stdc_sqrt (tr + 1.0f);
        quat->s= s * 0.5f;
        s= 0.5f / s;
        quat->v.x= (m->_32 - m->_23) * s;
        quat->v.y= (m->_13 - m->_31) * s;
        quat->v.z= (m->_21 - m->_12) * s;
    }
    else
    {
        if (m->_22 > m->_11 && m->_33 <= m->_22)
        {
            s= stdc_sqrt ((m->_22 - (m->_33 + m->_11)) + 1.0f);

            quat->v.x= s * 0.5f;

            if (s != 0.0)
                s= 0.5f / s;

            quat->v.z= (m->_13 - m->_31) * s;
            quat->v.y= (m->_32 + m->_23) * s;
            quat->s= (m->_12 + m->_21) * s;
        }
        else if ((m->_22 <= m->_11  &&  m->_33 > m->_11)  ||  (m->_33 > m->_22))
        {
            s= stdc_sqrt ((m->_33 - (m->_11 + m->_22)) + 1.0f);

            quat->v.y= s * 0.5f;

            if (s != 0.0)
                s= 0.5f / s;

            quat->v.z= (m->_21 - m->_12) * s;
            quat->s= (m->_13 + m->_31) * s;
            quat->v.x= (m->_23 + m->_32) * s;
        }
        else
        {
            s= stdc_sqrt ((m->_11 - (m->_22 + m->_33)) + 1.0f);

            quat->s= s * 0.5f;

            if (s != 0.0)
                s= 0.5f / s;

            quat->v.z= (m->_32 - m->_23) * s;
            quat->v.x= (m->_21 + m->_12) * s;
            quat->v.y= (m->_31 + m->_13) * s;
        }

#if 0
        // diagonal is negative
        i= 0;
        if (m->_22 > m->_11) i = 1;
        if (m->_33 > mm[i*4+i]) i = 2;
        j= nxt[i];
        k= nxt[j];

        s= stdc_sqrt ((mm[i*4+i] - (mm[j*4+j] + mm[k*4+k])) + 1.0f);

        q[i]= s * 0.5f;

        if (s != 0.0)
            s= 0.5f / s;

        q[3]= (mm[j*4+k] - mm[k*4+j]) * s;
        q[j]= (mm[i*4+j] + mm[j*4+i]) * s;
        q[k]= (mm[i*4+k] + mm[k*4+i]) * s;

        *quat= q;
#endif
    }

	return;
}

static void stdc_glMat2quat(
	D3DRMQUATERNION *quat,
	const float *m)
{
	float *mm= (float *)&m;
    float tr, s;

    tr= m[0] + m[5] + m[10];

    // check the diagonal
    if (tr > 0.0)
    {
        s= stdc_sqrt (tr + 1.0f);
        quat->s= s * 0.5f;
        s= 0.5f / s;
        quat->v.x= (m[6] - m[9]) * s;
        quat->v.y= (m[8] - m[2]) * s;
        quat->v.z= (m[1] - m[4]) * s;
    }
    else
    {
        if (m[5] > m[0] && m[10] <= m[5])
        {
            s= stdc_sqrt ((m[5] - (m[10] + m[0])) + 1.0f);

            quat->v.x= s * 0.5f;

            if (s != 0.0)
                s= 0.5f / s;

            quat->v.z= (m[8] - m[2]) * s;
            quat->v.y= (m[6] + m[9]) * s;
            quat->s= (m[4] + m[1]) * s;
        }
        else if ((m[5] <= m[0]  &&  m[10] > m[0])  ||  (m[10] > m[5]))
        {
            s= stdc_sqrt ((m[10] - (m[0] + m[5])) + 1.0f);

            quat->v.y= s * 0.5f;

            if (s != 0.0)
                s= 0.5f / s;

            quat->v.z= (m[1] - m[4]) * s;
            quat->s= (m[8] + m[2]) * s;
            quat->v.x= (m[9] + m[6]) * s;
        }
        else
        {
            s= stdc_sqrt ((m[0] - (m[5] + m[10])) + 1.0f);

            quat->s= s * 0.5f;

            if (s != 0.0)
                s= 0.5f / s;

            quat->v.z= (m[6] - m[9]) * s;
            quat->v.x= (m[1] + m[4]) * s;
            quat->v.y= (m[2] + m[8]) * s;
        }
    }

	return;
}

static void stdc_quat2D3DMat(
	D3DMATRIX *m,
	const D3DRMQUATERNION *quat)
{
	float wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

    // calculate coefficients
    x2= quat->v.x + quat->v.x;
    y2= quat->v.y + quat->v.y; 
    z2= quat->v.z + quat->v.z;

    xx= quat->v.x * x2;   xy = quat->v.x * y2;   xz = quat->v.x * z2;
    yy= quat->v.y * y2;   yz = quat->v.y * z2;   zz = quat->v.z * z2;
    wx= quat->s   * x2;   wy = quat->s   * y2;   wz = quat->s   * z2;

    m->_11= 1.0f - yy - zz;
    m->_12= xy - wz;
    m->_13= xz + wy;
    m->_21= xy + wz;
    m->_22= 1.0f - xx - zz;
    m->_23= yz - wx;
    m->_31= xz - wy;
    m->_32= yz + wx;
    m->_33= 1.0f - xx - yy;

    m->_14 =
    m->_24 =
    m->_34 =
    m->_41 =
    m->_42 =
    m->_43 = 0.0f;
    m->_44 = 1.0f;

	return;
}

static void stdc_quat2glMat(
	float *m,
	const D3DRMQUATERNION *quat)
{
	float wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

    // calculate coefficients
    x2= quat->v.x + quat->v.x;
    y2= quat->v.y + quat->v.y; 
    z2= quat->v.z + quat->v.z;

    xx= quat->v.x * x2;   xy = quat->v.x * y2;   xz = quat->v.x * z2;
    yy= quat->v.y * y2;   yz = quat->v.y * z2;   zz = quat->v.z * z2;
    wx= quat->s   * x2;   wy = quat->s   * y2;   wz = quat->s   * z2;

    m[0]= 1.0f - yy - zz;
    m[1]= xy + wz;
    m[2]= xz - wy;

    m[4]= xy - wz;
    m[5]= 1.0f - xx - zz;
    m[6]= yz + wx;

    m[8]= xz + wy;
    m[9]= yz - wx;
    m[10]= 1.0f - xx - yy;

    m[3] =
    m[7] =
    m[11] =
    m[12] =
    m[13] =
    m[14]= 0.0f;
    m[15]= 1.0f;

	return;
}

static void stdc_euler2quat(
	D3DRMQUATERNION *quat,
	const float *rot)
{
	float csx[2],csy[2],csz[2],cc,cs,sc,ss/*, cy, sy*/;

    // Convert angles to radians/2, construct the quat axes
    if (rot[0] == 0.0f)
    {
        csx[0]= 1.0f;
        csx[1]= 0.0f;
    }
    else
    {
        const float deg= rot[0] * (DEG2RAD * 0.5f);
		stdc_sincos(deg, csx);
        //csx[0]= (float)cos (deg);
        //csx[1]= (float)sin (deg);
    }

    if (rot[2] == 0.0f)
    {
        cc= csx[0];
        ss= 0.0f;
        cs= 0.0f;
        sc= csx[1];
    }
    else
    {
        const float deg= rot[2] * (DEG2RAD * 0.5f);
        stdc_sincos(deg, csz);
		//csz[0]= (float)cos (deg);
        //csz[1]= (float)sin (deg);
        cc= csx[0] * csz[0];
        ss= csx[1] * csz[1];
        cs= csx[0] * csz[1];
        sc= csx[1] * csz[0];
    }

    if (rot[1] == 0.0f)
    {
        quat->v.x= sc;
        quat->v.y= ss;
        quat->v.z= cs;
        quat->s= cc;
    }
    else
    {
        const float deg= rot[1] * (DEG2RAD * 0.5f);
        stdc_sincos(deg, csy);
//		cy= csy[0];
//		sy= csy[1];
		//cy = csy[0]= (float)cos (deg);
        //sy = csy[1]= (float)sin (deg);
/*
        quat->v.x= (cy * sc) - (sy * cs);
        quat->v.y= (cy * ss) + (sy * cc);
        quat->v.z= (cy * cs) - (sy * sc);
        quat->s= (cy * cc) + (sy * ss);
*/
		quat->v.x= (csy[0] * sc) - (csy[1] * cs);
		quat->v.y= (csy[0] * ss) + (csy[1] * cc);
		quat->v.z= (csy[0] * cs) - (csy[1] * sc);
		quat->s= (csy[0] * cc) + (csy[1] * ss);
    }

	return;
}

static void stdc_euler2quat2(
	D3DRMQUATERNION *quat,
	const float *rot)
{
	D3DRMQUATERNION qx, qy, qz, qf;
    float   deg, cs[2];

    // Convert angles to radians (and half-angles), and compute partial quats
    deg= rot[0] * 0.5f * DEG2RAD;
    stdc_sincos(deg, cs);
	qx.v.x= cs[1]; //(float)sin (deg);
    qx.v.y= 0.0f; 
    qx.v.z= 0.0f; 
    qx.s= cs[0]; //(float)cos (deg);

    deg= rot[1] * 0.5f * DEG2RAD;
    stdc_sincos(deg, cs);
	qy.v.x= 0.0f; 
    qy.v.y= cs[1]; //(float)sin (deg);
    qy.v.z= 0.0f;
    qy.s= cs[0]; //(float)cos (deg);

    deg= rot[2] * 0.5f * DEG2RAD;
    stdc_sincos(deg, cs);
	qz.v.x= 0.0f;
    qz.v.y= 0.0f;
    qz.v.z= cs[1]; //(float)sin (deg);
    qz.s= cs[0]; //(float)cos (deg);

    stdc_mult_quat(&qf, &qy, &qx);
    stdc_mult_quat(quat, &qz, &qf);

    // should be normal, if sin/cos and quat_mult are accurate enough
	return;
}

static void stdc_quat2axis_angle(
	float *axisAngle,
	const D3DRMQUATERNION *quat)
{
	const float tw= stdc_acos(quat->s);
    const float scale= (float)(1.f / stdc_sin(tw));

    axisAngle[3]= tw * 2.f * RAD2DEG;
    axisAngle[0]= quat->v.x * scale;
    axisAngle[1]= quat->v.y * scale;
    axisAngle[2]= quat->v.z * scale;

	return;
}

static void stdc_axis_angle2quat(
	D3DRMQUATERNION *quat,
	const float *axisAngle)
{
	const float deg= axisAngle[3] / (2.0f * RAD2DEG);
	//const float cs= (float)cos (deg);
    float cs[2];

	stdc_sincos(deg, cs);
	quat->s= cs[1]; //(float)sin (deg);
    quat->v.x= cs[0] * axisAngle[0];
    quat->v.y= cs[0] * axisAngle[1];
    quat->v.z= cs[0] * axisAngle[2];

	return;
}

static void stdc_slerp_quat(
	D3DRMQUATERNION *result,
	const D3DRMQUATERNION *quat1,
	const D3DRMQUATERNION *quat2,
	float slerp)
{
	float omega, cosom, isinom;
    float scale0, scale1;
    float q2x, q2y, q2z, q2w;

    // DOT the quats to get the cosine of the angle between them
    cosom = quat1->v.x * quat2->v.x +
            quat1->v.y * quat2->v.y +
            quat1->v.z * quat2->v.z +
            quat1->s   * quat2->s;

    // Two special cases:
    // Quats are exactly opposite, within DELTA?
    if (cosom > DELTA - 1.0)
    {
        // make sure they are different enough to avoid a divide by 0
        if (cosom < 1.0 - DELTA)
        {
            // SLERP away
            omega = stdc_acos(cosom);
            isinom = 1.f / stdc_sin(omega);
            scale0 = (float)(stdc_sin ((1.f - slerp) * omega) * isinom);
            scale1 = (float)(stdc_sin (slerp * omega) * isinom);
        }
        else
        {
            // LERP is good enough at this distance
            scale0 = 1.0f - slerp;
            scale1 = slerp;
        }

        q2x = quat2->v.x * scale1;
        q2y = quat2->v.y * scale1;
        q2z = quat2->v.z * scale1;
        q2w = quat2->s   * scale1;
    }
    else
    {
        // SLERP towards a perpendicular quat
        // Set slerp parameters
        scale0 = stdc_sin ((1.0f - slerp) * HALF_PI);
        scale1 = stdc_sin (slerp * HALF_PI);

        q2x = -quat2->v.y * scale1;
        q2y =  quat2->v.x * scale1;
        q2z = -quat2->s   * scale1;
        q2w =  quat2->v.z * scale1;
    }

    // Compute the result
    result->v.x = scale0 * quat1->v.x + q2x;
    result->v.y = scale0 * quat1->v.y + q2y;
    result->v.z = scale0 * quat1->v.z + q2z;
    result->s   = scale0 * quat1->s   + q2w;

	return;
}

static void stdc_trans_quat(
	D3DVECTOR *result,
	const D3DVECTOR *v,
	const D3DRMQUATERNION *q)
{
	// result = av + bq + c(q.v CROSS v)
    // where
    //  a = q.w^2 - (q.v DOT q.v)
    //  b = 2 * (q.v DOT v)
    //  c = 2q.w
    //float   w = q->s;   // just a convenience name
    float   a = q->s * q->s - (q->v.x * q->v.x + q->v.y * q->v.y + q->v.z * q->v.z);
    float   b = 2.0f  * (q->v.x * v->x   + q->v.y * v->y   + q->v.z * v->z);
    float   c = 2.0f  * q->s;

    // Must store this, because result may alias v
    float cross[3]; // q.v CROSS v
    cross[0] = q->v.y * v->z - q->v.z * v->y;
    cross[1] = q->v.z * v->x - q->v.x * v->z;
    cross[2] = q->v.x * v->y - q->v.y * v->x;

    result->x = a * v->x + b * q->v.x + c * cross[0];
    result->y = a * v->y + b * q->v.y + c * cross[1];
    result->z = a * v->z + b * q->v.z + c * cross[2];

	return;
}

// vector fxns
static float stdc_mag_vect(
	const D3DVECTOR *a)
{
	return stdc_sqrt ((a->x * a->x + a->y * a->y + a->z * a->z));
}

static float stdc_dot_vect(
	const D3DVECTOR *a,
	const D3DVECTOR *b)
{
	return (a->x * b->x + a->y * b->y + a->z * b->z);
}

static void stdc_add_vect(
	D3DVECTOR *r,
	const D3DVECTOR *a,
	const D3DVECTOR *b)
{
	r->x = a->x + b->x;
    r->y = a->y + b->y;
    r->z = a->z + b->z;

	return;
}

static void stdc_sub_vect(
	D3DVECTOR *r,
	const D3DVECTOR *a,
	const D3DVECTOR *b)
{
	r->x = a->x - b->x;
    r->y = a->y - b->y;
    r->z = a->z - b->z;

	return;
}

static void stdc_mult_vect(
	D3DVECTOR *r,
	const D3DVECTOR *a,
	const D3DVECTOR *b)
{
	r->x = a->x * b->x;
    r->y = a->y * b->y;
    r->z = a->z * b->z;

	return;
}

static void stdc_scale_vect(
	D3DVECTOR *r,
	const D3DVECTOR *a,
	float b)
{
	r->x = a->x * b;
    r->y = a->y * b;
    r->z = a->z * b;

	return;
}

static void stdc_norm_vect(
	D3DVECTOR *r,
	const D3DVECTOR *a)
{
	float len = stdc_sqrt ((a->x * a->x + a->y * a->y + a->z * a->z));

    if (len < 0.0001)
    {
        r->x = a->x;
        r->y = a->y;
        r->z = a->z;
    }
    else
    {
        len = 1.0f/len;
        r->x = a->x*len;
        r->y = a->y*len;
        r->z = a->z*len;
    }

	return;
}

static void stdc_cross_vect(
	D3DVECTOR *r,
	const D3DVECTOR *a,
	const D3DVECTOR *b)
{
	r->x = a->y * b->z - a->z * b->y;
    r->y = a->z * b->x - a->x * b->z;
    r->z = a->x * b->y - a->y * b->x;

	return;
}

// matrix routines
static float stdc_det_3x3(
	const float *mat)
{
	float det;

    det = mat[0] * (mat[4] * mat[8] - mat[7] * mat[5])
        - mat[1] * (mat[3] * mat[8] - mat[6] * mat[5])
        + mat[2] * (mat[3] * mat[7] - mat[6] * mat[4]);

    return det;
}

static void stdc_inverse_3x3(
	float *res,
	const float *a)
{
	float det = stdc_det_3x3(res);

    if (stdc_fabs (det) < 0.0005f)
    {
        res[0] = 1.0f;
        res[1] = 0.0f;
        res[2] = 0.0f;
        res[3] = 0.0f;
        res[4] = 1.0f;
        res[5] = 0.0f;
        res[6] = 0.0f;
        res[7] = 0.0f;
        res[8] = 1.0f;
        return;
    }

    det = 1.0f / det;
    res[0] =   a[4]*a[8] - a[5]*a[7]  * det;
    res[1] = -(a[1]*a[8] - a[7]*a[2]) * det;
    res[2] =   a[1]*a[5] - a[4]*a[2]  * det;
      
    res[3] = -(a[3]*a[8] - a[5]*a[6]) * det;
    res[4] =   a[0]*a[8] - a[6]*a[2]  * det;
    res[5] = -(a[0]*a[5] - a[3]*a[2]) * det;
      
    res[6] =   a[3]*a[7] - a[6]*a[4]  * det;
    res[7] = -(a[0]*a[7] - a[6]*a[1]) * det;
    res[8] =   a[0]*a[4] - a[1]*a[3]  * det;

	return;
}

static void stdc_submat_4x4(
	float *mb,
	const D3DMATRIX *mr,
	int i,
	int j)
{
	// Unrolled - big, but very fast (one indexed jump, one unconditional jump)
    switch (i*4+j)
    {
		// i == 0
		case 0:     // 0,0
			mb[0] = mr->_22; mb[1] = mr->_23; mb[2] = mr->_24;
			mb[3] = mr->_32; mb[4] = mr->_33; mb[5] = mr->_34;
			mb[6] = mr->_42; mb[7] = mr->_43; mb[8] = mr->_44;
			break;
		case 1:     // 0,1
			mb[0] = mr->_21; mb[1] = mr->_23; mb[2] = mr->_24;
			mb[3] = mr->_31; mb[4] = mr->_33; mb[5] = mr->_34;
			mb[6] = mr->_41; mb[7] = mr->_43; mb[8] = mr->_44;
			break;
		case 2:     // 0,2
			mb[0] = mr->_21; mb[1] = mr->_22; mb[2] = mr->_24;
			mb[3] = mr->_31; mb[4] = mr->_32; mb[5] = mr->_34;
			mb[6] = mr->_41; mb[7] = mr->_42; mb[8] = mr->_44;
			break;
		case 3:     // 0,3
			mb[0] = mr->_21; mb[1] = mr->_22; mb[2] = mr->_23;
			mb[3] = mr->_31; mb[4] = mr->_32; mb[5] = mr->_33;
			mb[6] = mr->_41; mb[7] = mr->_42; mb[8] = mr->_43;
			break;

		// i == 1
		case 4:     // 1,0
			mb[0] = mr->_12; mb[1] = mr->_13; mb[2] = mr->_14;
			mb[3] = mr->_32; mb[4] = mr->_33; mb[5] = mr->_34;
			mb[6] = mr->_42; mb[7] = mr->_43; mb[8] = mr->_44;
			break;
		case 5:     // 1,1
			mb[0] = mr->_11; mb[1] = mr->_13; mb[2] = mr->_14;
			mb[3] = mr->_31; mb[4] = mr->_33; mb[5] = mr->_34;
			mb[6] = mr->_41; mb[7] = mr->_43; mb[8] = mr->_44;
			break;
		case 6:     // 1,2
			mb[0] = mr->_11; mb[1] = mr->_12; mb[2] = mr->_14;
			mb[3] = mr->_31; mb[4] = mr->_32; mb[5] = mr->_34;
			mb[6] = mr->_41; mb[7] = mr->_42; mb[8] = mr->_44;
			break;
		case 7:     // 1,3
			mb[0] = mr->_11; mb[1] = mr->_12; mb[2] = mr->_13;
			mb[3] = mr->_31; mb[4] = mr->_32; mb[5] = mr->_33;
			mb[6] = mr->_41; mb[7] = mr->_42; mb[8] = mr->_43;
			break;

		// i == 2
		case 8:     // 2,0
			mb[0] = mr->_12; mb[1] = mr->_13; mb[2] = mr->_14;
			mb[3] = mr->_22; mb[4] = mr->_23; mb[5] = mr->_24;
			mb[6] = mr->_42; mb[7] = mr->_43; mb[8] = mr->_44;
			break;
		case 9:     // 2,1
			mb[0] = mr->_11; mb[1] = mr->_13; mb[2] = mr->_14;
			mb[3] = mr->_21; mb[4] = mr->_23; mb[5] = mr->_24;
			mb[6] = mr->_41; mb[7] = mr->_43; mb[8] = mr->_44;
			break;
		case 10:    // 2,2
			mb[0] = mr->_11; mb[1] = mr->_12; mb[2] = mr->_14;
			mb[3] = mr->_21; mb[4] = mr->_22; mb[5] = mr->_24;
			mb[6] = mr->_41; mb[7] = mr->_42; mb[8] = mr->_44;
			break;
		case 11:    // 2,3
			mb[0] = mr->_11; mb[1] = mr->_12; mb[2] = mr->_13;
			mb[3] = mr->_21; mb[4] = mr->_22; mb[5] = mr->_23;
			mb[6] = mr->_31; mb[7] = mr->_32; mb[8] = mr->_33;
			break;

		// i == 3
		case 12:    // 3,0
			mb[0] = mr->_12; mb[1] = mr->_13; mb[2] = mr->_14;
			mb[3] = mr->_22; mb[4] = mr->_23; mb[5] = mr->_24;
			mb[6] = mr->_32; mb[7] = mr->_33; mb[8] = mr->_34;
			break;
		case 13:    // 3,1
			mb[0] = mr->_11; mb[1] = mr->_13; mb[2] = mr->_14;
			mb[3] = mr->_21; mb[4] = mr->_23; mb[5] = mr->_24;
			mb[6] = mr->_31; mb[7] = mr->_33; mb[8] = mr->_34;
			break;
		case 14:    // 3,2
			mb[0] = mr->_11; mb[1] = mr->_12; mb[2] = mr->_14;
			mb[3] = mr->_21; mb[4] = mr->_22; mb[5] = mr->_24;
			mb[6] = mr->_31; mb[7] = mr->_32; mb[8] = mr->_34;
			break;
		case 15:    // 3,3
			mb[0] = mr->_11; mb[1] = mr->_12; mb[2] = mr->_13;
			mb[3] = mr->_21; mb[4] = mr->_22; mb[5] = mr->_23;
			mb[6] = mr->_31; mb[7] = mr->_32; mb[8] = mr->_33;
			break;
    }

	return;
}

static float stdc_det_4x4(
	const D3DMATRIX *mr)
{
	float   res = 0.0f, det;
	// 28 muls total - totally inline-expanded and factored
    // Ugly (and nearly incomprehensible) but efficient
    const float mr_3344_4334 = mr->_33 * mr->_44 - mr->_43 * mr->_34;
    const float mr_3244_4234 = mr->_32 * mr->_44 - mr->_42 * mr->_34;
    const float mr_3243_4233 = mr->_32 * mr->_43 - mr->_42 * mr->_33;
    const float mr_3144_4134 = mr->_31 * mr->_44 - mr->_41 * mr->_34;
    const float mr_3143_4133 = mr->_31 * mr->_43 - mr->_41 * mr->_33;
    const float mr_3142_4132 = mr->_31 * mr->_42 - mr->_41 * mr->_32;

    //submat_4x4 (msub3, mr, 0, 0);
    //res += mr->_11 * det_3x3 (msub3);
    det = mr->_22 * mr_3344_4334 - mr->_23 * mr_3244_4234 + mr->_24 * mr_3243_4233;
    res += mr->_11 * det;

    //submat_4x4 (msub3, mr, 0, 1);
    //res -= mr->_12 * det_3x3 (msub3);
    det = mr->_21 * mr_3344_4334 - mr->_23 * mr_3144_4134 + mr->_24 * mr_3143_4133;
    res -= mr->_12 * det;

    //submat_4x4 (msub3, mr, 0, 2);
    //res += mr->_13 * det_3x3 (msub3);
    det = mr->_21 * mr_3244_4234 - mr->_22 * mr_3144_4134 + mr->_24 * mr_3142_4132;
    res += mr->_13 * det;

    //submat_4x4 (msub3, mr, 0, 3);
    //res -= mr->_14 * det_3x3 (msub3);
    det = mr->_21 * mr_3243_4233 - mr->_22 * mr_3143_4133 + mr->_23 * mr_3142_4132;
    res -= mr->_14 * det;

	return res;
}

static int stdc_inverse_4x4(
	D3DMATRIX *mr,
	const D3DMATRIX *ma)
{
	float   mdet = stdc_det_4x4(ma);
    float   mtemp[9];

    if (stdc_fabs (mdet) < 0.0005f)
        return 0;

    mdet = 1.0f / mdet;

    stdc_submat_4x4 (mtemp, ma, 0, 0);
    mr->_11 =  stdc_det_3x3 (mtemp) * mdet;

    stdc_submat_4x4 (mtemp, ma, 0, 1);
    mr->_12 = -stdc_det_3x3 (mtemp) * mdet;

    stdc_submat_4x4 (mtemp, ma, 0, 2);
    mr->_13 =  stdc_det_3x3 (mtemp) * mdet;

    stdc_submat_4x4 (mtemp, ma, 0, 3);
    mr->_14 = -stdc_det_3x3 (mtemp) * mdet;


    stdc_submat_4x4 (mtemp, ma, 1, 0);
    mr->_21 = -stdc_det_3x3 (mtemp) * mdet;

    stdc_submat_4x4 (mtemp, ma, 1, 1);
    mr->_22 =  stdc_det_3x3 (mtemp) * mdet;

    stdc_submat_4x4 (mtemp, ma, 1, 2);
    mr->_23 = -stdc_det_3x3 (mtemp) * mdet;

    stdc_submat_4x4 (mtemp, ma, 1, 3);
    mr->_24 =  stdc_det_3x3 (mtemp) * mdet;


    stdc_submat_4x4 (mtemp, ma, 2, 0);
    mr->_31 =  stdc_det_3x3 (mtemp) * mdet;

    stdc_submat_4x4 (mtemp, ma, 2, 1);
    mr->_32 = -stdc_det_3x3 (mtemp) * mdet;

    stdc_submat_4x4 (mtemp, ma, 2, 2);
    mr->_33 =  stdc_det_3x3 (mtemp) * mdet;

    stdc_submat_4x4 (mtemp, ma, 2, 3);
    mr->_34 = -stdc_det_3x3 (mtemp) * mdet;

    stdc_submat_4x4 (mtemp, ma, 3, 0);
    mr->_41 = -stdc_det_3x3 (mtemp) * mdet;

    stdc_submat_4x4 (mtemp, ma, 3, 1);
    mr->_42 =  stdc_det_3x3 (mtemp) * mdet;

    stdc_submat_4x4 (mtemp, ma, 3, 2);
    mr->_43 = -stdc_det_3x3 (mtemp) * mdet;

    stdc_submat_4x4 (mtemp, ma, 3, 3);
    mr->_44 =  stdc_det_3x3 (mtemp) * mdet;

    return 1;
}

static void stdc_glMul_3x3(
	float *r,
	const float *a,
	const float *b)
{
	int i, j;
 
    for (i = 0; i < 3; i++)
    {
        const int   idx = i * 3;
        const float *pa = &a[idx];
        for(j = 0; j < 3; j++)
        {
            const float *pb = &b[j];
            r[idx+j] = a[idx+0] * b[0+j]
                       + a[idx+1] * b[3+j]
                       + a[idx+2] * b[6+j];
        }
    }

	return;
}

static void stdc_glMul_4x4(
	float *r,
	const float *a,
	const float *b)
{
	r[0]  = b[0] * a[0]  + b[4] * a[1]  + b[8]  * a[2]  + b[12] * a[3];
    r[1]  = b[1] * a[0]  + b[5] * a[1]  + b[9]  * a[2]  + b[13] * a[3];
    r[2]  = b[2] * a[0]  + b[6] * a[1]  + b[10] * a[2]  + b[14] * a[3];
    r[3]  = b[3] * a[0]  + b[7] * a[1]  + b[11] * a[2]  + b[15] * a[3];
    r[4]  = b[0] * a[4]  + b[4] * a[5]  + b[8]  * a[6]  + b[12] * a[7];
    r[5]  = b[1] * a[4]  + b[5] * a[5]  + b[9]  * a[6]  + b[13] * a[7];
    r[6]  = b[2] * a[4]  + b[6] * a[5]  + b[10] * a[6]  + b[14] * a[7];
    r[7]  = b[3] * a[4]  + b[7] * a[5]  + b[11] * a[6]  + b[15] * a[7];
    r[8]  = b[0] * a[8]  + b[4] * a[9]  + b[8]  * a[10] + b[12] * a[11];
    r[9]  = b[1] * a[8]  + b[5] * a[9]  + b[9]  * a[10] + b[13] * a[11];
    r[10] = b[2] * a[8]  + b[6] * a[9]  + b[10] * a[10] + b[14] * a[11];
    r[11] = b[3] * a[8]  + b[7] * a[9]  + b[11] * a[10] + b[15] * a[11];
    r[12] = b[0] * a[12] + b[4] * a[13] + b[8]  * a[14] + b[12] * a[15];
    r[13] = b[1] * a[12] + b[5] * a[13] + b[9]  * a[14] + b[13] * a[15];
    r[14] = b[2] * a[12] + b[6] * a[13] + b[10] * a[14] + b[14] * a[15];
    r[15] = b[3] * a[12] + b[7] * a[13] + b[11] * a[14] + b[15] * a[15];

	return;
}

static void stdc_D3DMul_4x4(
	D3DMATRIX *r,
	const D3DMATRIX *a,
	const D3DMATRIX *b)
{
	r->_11 = a->_11 * b->_11 + a->_12 * b->_21 + a->_13 * b->_31 + a->_14 * b->_41;
    r->_12 = a->_11 * b->_12 + a->_12 * b->_22 + a->_13 * b->_32 + a->_14 * b->_42;
    r->_13 = a->_11 * b->_13 + a->_12 * b->_23 + a->_13 * b->_33 + a->_14 * b->_43;
    r->_14 = a->_11 * b->_14 + a->_12 * b->_24 + a->_13 * b->_34 + a->_14 * b->_44;
                                                                          
    r->_21 = a->_21 * b->_11 + a->_22 * b->_21 + a->_23 * b->_31 + a->_24 * b->_41;
    r->_22 = a->_21 * b->_12 + a->_22 * b->_22 + a->_23 * b->_32 + a->_24 * b->_42;
    r->_23 = a->_21 * b->_13 + a->_22 * b->_23 + a->_23 * b->_33 + a->_24 * b->_43;
    r->_24 = a->_21 * b->_14 + a->_22 * b->_24 + a->_23 * b->_34 + a->_24 * b->_44;
                                                         
    r->_31 = a->_31 * b->_11 + a->_32 * b->_21 + a->_33 * b->_31 + a->_34 * b->_41;
    r->_32 = a->_31 * b->_12 + a->_32 * b->_22 + a->_33 * b->_32 + a->_34 * b->_42;
    r->_33 = a->_31 * b->_13 + a->_32 * b->_23 + a->_33 * b->_33 + a->_34 * b->_43;
    r->_34 = a->_31 * b->_14 + a->_32 * b->_24 + a->_33 * b->_34 + a->_34 * b->_44;
                                                         
    r->_41 = a->_41 * b->_11 + a->_42 * b->_21 + a->_43 * b->_31 + a->_44 * b->_41;
    r->_42 = a->_41 * b->_12 + a->_42 * b->_22 + a->_43 * b->_32 + a->_44 * b->_42;
    r->_43 = a->_41 * b->_13 + a->_42 * b->_23 + a->_43 * b->_33 + a->_44 * b->_43;
    r->_44 = a->_41 * b->_14 + a->_42 * b->_24 + a->_43 * b->_34 + a->_44 * b->_44;

   return;
}


#ifdef __cplusplus
}
#endif

#endif // USE_FAST_MATH

