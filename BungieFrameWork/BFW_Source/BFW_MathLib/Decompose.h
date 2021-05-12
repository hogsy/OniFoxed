/**** Decompose.h - Basic declarations ****/
#ifndef _H_Decompose
#define _H_Decompose

#include "BFW.h"
#include "BFW_MathLib.h"
#include "BFW_Motoko.h"

enum QuatPart {eX, eY, eZ, eW};
typedef M3tQuaternion HVect; /* Homogeneous 3D vector */
typedef float HMatrix[4][4]; /* Right-handed, for column vectors */
typedef struct {
    HVect t;	/* Translation components */
    M3tQuaternion  q;	/* Essential rotation	  */
    M3tQuaternion  u;	/* Stretch rotation	  */
    HVect k;	/* Stretch factors	  */
    float f;	/* Sign of determinant	  */
} AffineParts;
float polar_decomp(HMatrix M, HMatrix Q, HMatrix S);
HVect spect_decomp(HMatrix S, HMatrix U);
M3tQuaternion snuggle(M3tQuaternion q, HVect *k);
void decomp_affine(HMatrix A, AffineParts *parts);
void invert_affine(AffineParts *parts, AffineParts *inverse);
#endif
