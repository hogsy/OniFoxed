/*
	FILE:	BFW_Platform_AltiVec.h

	AUTHOR:	Brent H. Pease

	CREATED: Aug 6, 1999

	PURPOSE: Interface to the Motoko 3D engine

	Copyright 1997-1999

*/
#ifndef BFW_PLATFORM_ALTIVEC_H
#define BFW_PLATFORM_ALTIVEC_H

#if UUmSIMD == UUmSIMD_AltiVec

#define AVmPerm_Build4Byte_1Elem(x) (x) + 0, (x) + 1, (x) + 2, (x) + 3

// AVmPerm_Build4Byte builds a permute vector for a 4byte value
// sn - select the operand for elem n
// in - select the index for elem n
#define AVmPerm_Build4Byte(s0, i0, s1, i1, s2, i2, s3, i3) \
	( AVmPerm_Build4Byte_1Elem((s0 << 4) | (i0 << 2)) ,	\
		AVmPerm_Build4Byte_1Elem((s1 << 4) | (i1 << 2)) ,	\
		AVmPerm_Build4Byte_1Elem((s2 << 4) | (i2 << 2)) ,	\
		AVmPerm_Build4Byte_1Elem((s3 << 4) | (i3 << 2)) )

#define AVmBuildBlockDST(size, count, stride) (	\
	((size) << 24) |							\
	(((count) < 256 ? (count) : 0) << 16) |		\
	(stride))

#define AVmLoadMisalignedVectorPoint(p, dest)		\
{													\
	vector unsigned char temp0__, temp1__, temp2__;	\
													\
	temp0__ = vec_ld(0, (unsigned char *)&p->x);	\
	temp1__ = vec_ld(16, (unsigned char *)&p->x);	\
	temp2__ = vec_lvsl(0, (unsigned char *)&p->x);	\
	dest = (vector float)vec_perm(temp0__, temp1__, temp2__);	\
}

extern vector unsigned char AVcSeq2Prl3x4_s0p0;
extern vector unsigned char AVcSeq2Prl3x4_s0p1;
extern vector unsigned char AVcSeq2Prl3x4_s0p2;
extern vector unsigned char AVcSeq2Prl3x4_s1p0;
extern vector unsigned char AVcSeq2Prl3x4_s1p1;
extern vector unsigned char AVcSeq2Prl3x4_s1p2;

extern vector unsigned char AVcPrl2Seq4x3_s0p0;
extern vector unsigned char AVcPrl2Seq4x3_s0p1;
extern vector unsigned char AVcPrl2Seq4x3_s0p2;
extern vector unsigned char AVcPrl2Seq4x3_s1p0;
extern vector unsigned char AVcPrl2Seq4x3_s1p1;
extern vector unsigned char AVcPrl2Seq4x3_s1p2;


extern vector unsigned char AVcPrl2Seq4x4_s0p0;
extern vector unsigned char AVcPrl2Seq4x4_s0p1;
extern vector unsigned char AVcPrl2Seq4x4_s1p0;
extern vector unsigned char AVcPrl2Seq4x4_s1p1;
extern vector float AVcSplatZero;
extern vector float AVcSplatOne;
extern vector float AVcSplatNegOne;
extern vector float AVcSplatOneMinusEpsilon;

void
AVrFloat_XYZ4ToXXXXYYYYZZZZ(
	UUtUns16	inNum,
	float*		inXYZ,
	float		*outXXXXYYYYZZZZ);

#endif

#endif /* BFW_PLATFORM_ALTIVEC_H */
