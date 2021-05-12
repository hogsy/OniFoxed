/*
	bfw_math_3dnow.c
	Wednesday Sept. 20, 2000   4:41 am
	Stefan
*/

#ifdef USE_FAST_MATH

#ifndef _WINDOWS_
#error "You must be building for MS Windows to use this code!"
#endif

/*---------- headers */

#include "bfw_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*---------- constants */

#undef FALSE
#undef TRUE
enum {
	FALSE= 0,
	TRUE
};

/*---------- code */

#define LOAD_3DNOW(x)	((fast##x= (void *)GetProcAddress(dll, #x)) != NULL)
#define LOAD_3DNOW_EXTRA(x) ((fast_##x= (void *)GetProcAddress(dll, #x)) != NULL)
int load_3dnow_math(
	void)
{
	HMODULE dll= NULL;
	int dll_loaded= FALSE;
	
	if ((dll == NULL) &&
		(((dll= GetModuleHandle("math.dll")) != NULL) ||
		((dll= LoadLibrary("math.dll"))) != NULL) &&
		LOAD_3DNOW(_atan) &&
		LOAD_3DNOW(_atan2) &&
		LOAD_3DNOW(_acos) &&
		LOAD_3DNOW(_asin) &&
		LOAD_3DNOW(_log) &&
		LOAD_3DNOW(_log10) &&
		LOAD_3DNOW(_pow) &&
		LOAD_3DNOW(_exp) &&
		LOAD_3DNOW(_sqrt) &&
		LOAD_3DNOW(_fabs) &&
		LOAD_3DNOW(_ceil) &&
		LOAD_3DNOW(_floor) &&
		LOAD_3DNOW(_frexp) &&
		LOAD_3DNOW(_ldexp) &&
		LOAD_3DNOW(_modf) &&
		LOAD_3DNOW(_fmod) &&
		LOAD_3DNOW(_sin) &&
		LOAD_3DNOW(_cos) &&
		LOAD_3DNOW(_tan) &&
		LOAD_3DNOW(_sincos) &&
		// quaternion fxns
		LOAD_3DNOW_EXTRA(add_quat) &&
		LOAD_3DNOW_EXTRA(sub_quat) &&
		LOAD_3DNOW_EXTRA(mult_quat) &&
		LOAD_3DNOW_EXTRA(norm_quat) &&
		LOAD_3DNOW_EXTRA(D3DMat2quat) &&
		LOAD_3DNOW_EXTRA(glMat2quat) &&
		LOAD_3DNOW_EXTRA(quat2D3DMat) &&
		LOAD_3DNOW_EXTRA(quat2glMat) &&
		LOAD_3DNOW_EXTRA(euler2quat) &&
		LOAD_3DNOW_EXTRA(euler2quat2) &&
		LOAD_3DNOW_EXTRA(quat2axis_angle) &&
		LOAD_3DNOW_EXTRA(axis_angle2quat) &&
		LOAD_3DNOW_EXTRA(slerp_quat) &&
		LOAD_3DNOW_EXTRA(trans_quat) &&
		// vector fxns
		LOAD_3DNOW_EXTRA(mag_vect) &&
		LOAD_3DNOW_EXTRA(dot_vect) &&
		LOAD_3DNOW_EXTRA(add_vect) &&
		LOAD_3DNOW_EXTRA(sub_vect) &&
		LOAD_3DNOW_EXTRA(mult_vect) &&
		LOAD_3DNOW_EXTRA(scale_vect) &&
		LOAD_3DNOW_EXTRA(norm_vect) &&
		LOAD_3DNOW_EXTRA(mag_vect) &&
		LOAD_3DNOW_EXTRA(dot_vect) &&
		LOAD_3DNOW_EXTRA(cross_vect) &&
		// matrix routines
		LOAD_3DNOW_EXTRA(det_3x3) &&
		LOAD_3DNOW_EXTRA(inverse_3x3) &&
		LOAD_3DNOW_EXTRA(submat_4x4) &&
		LOAD_3DNOW_EXTRA(det_4x4) &&
		LOAD_3DNOW_EXTRA(inverse_4x4) &&
		LOAD_3DNOW_EXTRA(glMul_3x3) &&
		LOAD_3DNOW_EXTRA(glMul_4x4) &&
		LOAD_3DNOW_EXTRA(D3DMul_4x4))
	{
		UUrStartupMessage("3DNow! math loaded");
		dll_loaded= TRUE;
	}

	return dll_loaded;
}

void unload_3dnow_math(
	void)
{
	HMODULE dll= GetModuleHandle("math.dll");

	if (dll)
	{
		FreeLibrary(dll);
	}

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

#ifdef __cplusplus
}
#endif

#endif // USE_FAST_MATH
