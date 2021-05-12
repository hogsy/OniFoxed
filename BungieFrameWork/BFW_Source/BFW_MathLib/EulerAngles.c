/**** EulerAngles.c - Convert Euler angles to/from matrix or quat ****/
/* Ken Shoemake, 1993 */

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_MathLib.h"
#include "EulerAngles.h"

//#define EULER_EPSILON 1.192092896e-07F /* smallest such that 1.0+EULER_EPSILON != 1.0 */
#define EULER_EPSILON .00001

MUtEuler Euler_(float ai, float aj, float ah, int order)
{
    MUtEuler ea;
    ea.x = ai; ea.y = aj; ea.z = ah;
    ea.order = order;

    return (ea);
}


/* Construct quaternion from Euler angles (in radians). */
M3tQuaternion MUrEulerToQuat(const MUtEuler *inEuler)
{
	MUtEuler ea = *inEuler;
    M3tQuaternion qu;
    float a[3];
	float ti, tj, th;
	float ci, cj, ch;
	float si, sj, sh;
	float cc, cs, sc, ss;
    int i,j,k,h,n,s,f;
    iEuler_GetOrder(ea.order,i,j,k,h,n,s,f);

    if (f==EulFrmR) { 
		float t = ea.x; 
		ea.x = ea.z; 
		ea.z = t; 
	}

    if (n==EulParOdd) { 
		ea.y = - ea.y; 
		UUmTrig_ClipLow(ea.y);
	}

    ti = ea.x * 0.5f; 
	tj = ea.y * 0.5f; 
	th = ea.z * 0.5f;

    ci = MUrCos(ti);  
	cj = MUrCos(tj);  
	ch = MUrCos(th);

    si = MUrSin(ti);  
	sj = MUrSin(tj);  
	sh = MUrSin(th);

    cc = ci*ch; 
	cs = ci*sh; 
	sc = si*ch; 
	ss = si*sh;

    if (s==EulRepYes) {
		a[i] = cj*(cs + sc);	/* Could speed up with */
		a[j] = sj*(cc + ss);	/* trig identities. */
		a[k] = sj*(cs - sc);
		qu.w = -(cj*(cc - ss));
    } else {
		a[i] = cj*sc - sj*cs;
		a[j] = cj*ss + sj*cc;
		a[k] = cj*cs - sj*sc;
		qu.w = -(cj*cc + sj*ss);
    }

    if (n==EulParOdd) { 
		a[j] = -a[j];
	}

    qu.x = a[0]; 
	qu.y = a[1]; 
	qu.z = a[2];

    return (qu);
}

void MUrXYZrEulerToQuat(float inX, float inY, float inZ, M3tQuaternion *outQuat)
{
	float ti, tj, th;
	float ci, cj, ch;
	float si, sj, sh;
	float cc, cs, sc, ss;

    ti = inZ * 0.5f; 
	tj = M3c2Pi - (inY * 0.5f); 
	th = inX * 0.5f;

    ci = MUrCos(ti);  
	cj = MUrCos(tj);  
	ch = MUrCos(th);

    si = MUrSin(ti);  
	sj = MUrSin(tj);  
	sh = MUrSin(th);

    cc = ci*ch; 
	cs = ci*sh; 
	sc = si*ch; 
	ss = si*sh;

    outQuat->x = cj*cs - sj*sc;
	outQuat->y = -cj*ss - sj*cc;
	outQuat->z = cj*sc - sj*cs;
	outQuat->w = -(cj*cc + sj*ss);

#if defined(DEBUGGING) && DEBUGGING
	{
		M3tQuaternion test_quat;
		MUtEuler test_euler ;

		test_euler.x = inX;
		test_euler.y = inY;
		test_euler.z = inZ;
		test_euler.order  = MUcEulerOrderXYZr;

		test_quat = MUrEulerToQuat(&test_euler);

		UUmAssert(MUrQuat_IsEqual(outQuat, &test_quat));
	}
#endif

    return;
}

void MUrXYXsEulerToQuat(float inX, float inY, float inZ, M3tQuaternion *outQuat)
{
    float ti, tj, th;
	float ci, cj, ch;
	float si, sj, sh;
	float cc, cs, sc, ss;

    ti = inX * 0.5f; 
	tj = inY * 0.5f; 
	th = inZ * 0.5f;

    ci = MUrCos(ti);  
	cj = MUrCos(tj);  
	ch = MUrCos(th);

    si = MUrSin(ti);  
	sj = MUrSin(tj);  
	sh = MUrSin(th);

    cc = ci * ch; 
	cs = ci * sh; 
	sc = si * ch; 
	ss = si * sh;

    outQuat->x = cj * (cs + sc);
	outQuat->y = sj * (cc + ss);
	outQuat->z = sj * (cs - sc);
	outQuat->w = -(cj * (cc - ss));


#if defined(DEBUGGING) && DEBUGGING
	{
		M3tQuaternion test_quat;
		MUtEuler test_euler ;

		test_euler.x = inX;
		test_euler.y = inY;
		test_euler.z = inZ;
		test_euler.order  = MUcEulerOrderXYXs;

		test_quat = MUrEulerToQuat(&test_euler);

		UUmAssert(MUrQuat_IsEqual(outQuat, &test_quat));
	}
#endif


    return;
}

/* Construct matrix from Euler angles (in radians). */
void MUrEulerToMatrix(const MUtEuler *inEuler, M3tMatrix4x3 *inMatrix)
{
	MUtEuler ea;
    float ti, tj, th, ci, cj, ch, si, sj, sh, cc, cs, sc, ss;
    int i,j,k,h,n,s,f;

	UUmAssertReadPtr(inEuler, sizeof(*inEuler));
	UUmAssertReadPtr(inMatrix, sizeof(*inMatrix));

	ea = *inEuler;

    iEuler_GetOrder(ea.order,i,j,k,h,n,s,f);

    if (f==EulFrmR) {float t = ea.x; ea.x = ea.z; ea.z = t;}

    if (n==EulParOdd) {
		ea.x = -ea.x; 
		ea.y = -ea.y;
		ea.z = -ea.z;

		UUmTrig_ClipLow(ea.x);
		UUmTrig_ClipLow(ea.y);
		UUmTrig_ClipLow(ea.z);
	}

    ti = ea.x;	  tj = ea.y;	th = ea.z;
    ci = MUrCos(ti); cj = MUrCos(tj); ch = MUrCos(th);
    si = MUrSin(ti); sj = MUrSin(tj); sh = MUrSin(th);
    cc = ci*ch; cs = ci*sh; sc = si*ch; ss = si*sh;

    if (s==EulRepYes) {
		inMatrix->m[i][i] = cj;		inMatrix->m[j][i] =  sj*si;    inMatrix->m[k][i] =  sj*ci;
		inMatrix->m[i][j] = sj*sh;	inMatrix->m[j][j] = -cj*ss+cc; inMatrix->m[k][j] = -cj*cs-sc;
		inMatrix->m[i][k] = -sj*ch;	inMatrix->m[j][k] =  cj*sc+cs; inMatrix->m[k][k] =  cj*cc-ss;
    } 
	else {
		inMatrix->m[i][i] = cj*ch;	inMatrix->m[j][i] = sj*sc-cs; inMatrix->m[k][i] = sj*cc+ss;
		inMatrix->m[i][j] = cj*sh;	inMatrix->m[j][j] = sj*ss+cc; inMatrix->m[k][j] = sj*cs-sc;
		inMatrix->m[i][k] = -sj;	inMatrix->m[j][k] = cj*si;    inMatrix->m[k][k] = cj*ci;
    }

    inMatrix->m[3][0] = 0.f;
	inMatrix->m[3][1] = 0.f;
	inMatrix->m[3][2] = 0.f;
}

/* Convert matrix to Euler angles (in radians). */
MUtEuler MUrMatrixToEuler(const M3tMatrix4x3 *inMatrix, int order)
{
    MUtEuler ea;
    int i,j,k,h,n,s,f;
    iEuler_GetOrder(order,i,j,k,h,n,s,f);

	if (s==EulRepYes) {
		float sy = MUrSqrt(UUmSQR(inMatrix->m[j][i]) + UUmSQR(inMatrix->m[k][i]));

		if (sy > 16 * EULER_EPSILON) 
		{
			ea.x = MUrATan2(inMatrix->m[j][i], inMatrix->m[k][i]);
			ea.y = MUrATan2(sy, inMatrix->m[i][i]);
			ea.z = MUrATan2(inMatrix->m[i][j], -inMatrix->m[i][k]);
		} 
		else {
			ea.x = MUrATan2(-inMatrix->m[k][j], inMatrix->m[j][j]);
			ea.y = MUrATan2(sy, inMatrix->m[i][i]);
			ea.z = 0;
		}
	}
	else {
		float cy = MUrSqrt(UUmSQR(inMatrix->m[i][i]) + UUmSQR(inMatrix->m[i][j]));

		if (cy > 16 * EULER_EPSILON) {
			ea.x = MUrATan2(inMatrix->m[j][k], inMatrix->m[k][k]);
			ea.y = MUrATan2(-inMatrix->m[i][k], cy);
			ea.z = MUrATan2(inMatrix->m[i][j], inMatrix->m[i][i]);
		} 
		else {
			ea.x = MUrATan2(-inMatrix->m[k][j], inMatrix->m[j][j]);
			ea.y = MUrATan2(-inMatrix->m[i][k], cy);
			ea.z = 0;
		}
	}

    if (n==EulParOdd) {
		ea.x = -ea.x;
		ea.y = -ea.y;
		ea.z = -ea.z;
	}

    if (f==EulFrmR) {
		float t = ea.x; 
		ea.x = ea.z; 
		ea.z = t;
	}
    
	UUmTrig_ClipLow(ea.x);
	UUmTrig_ClipLow(ea.y);
	UUmTrig_ClipLow(ea.z);
	ea.order = order;

	UUmAssert(ea.x >= 0);
	UUmAssert(ea.y >= 0);
	UUmAssert(ea.z >= 0);

    return (ea);
}

/* Convert quaternion to Euler angles (in radians). */
MUtEuler MUrQuatToEuler(const M3tQuaternion *q, int order)
{
    M3tMatrix4x3 M;
	MUtEuler euler;

	MUrQuatToMatrix(q, &M);
	euler = MUrMatrixToEuler(&M, order);

	return euler;
}
