
/*
	FILE:	BFW_Platform_AltiVec.c

	AUTHOR:	Brent H. Pease

	CREATED: Aug 6, 1999

	PURPOSE: Interface to the Motoko 3D engine

	Copyright 1997-1999

*/

#include "BFW.h"
#include "BFW_Platform.h"
#include "BFW_Platform_AltiVec.h"

#if UUmSIMD == UUmSIMD_AltiVec

/*
 * Define the permute vectors for converting xyz xyz xyz xyz to xxxx yyyy xxxx
 */
	//      t0            t1           t2
	// 0  1  2  3    0  1  2  3    0  1  2  3
	// x1 y1 z1 x2 | y2 z2 x3 y3 | z3 x4 y4 z4

	//  perm0 t0, t1  perm1 t1, t2  perm2 t0, t2
	//      t3            t4           t5
	// 0  1  2  3    0  1  2  3    0  1  2  3
	// x1 x2 x3 z2 | y2 y3 y4 x4 | z1 z3 z4 y1

	//  perm3 t3, t4  perm4 t4, t5  perm5 t3, t5
	//      t0            t1           t2
	// 0  1  2  3    0  1  2  3    0  1  2  3
	// x1 x2 x3 x4 | y1 y2 y3 y4 | z1 z2 z3 z4

								// sequential to parallel 3 element to 4 step 0 perm 0
	vector unsigned char	AVcSeq2Prl3x4_s0p0;

								// sequential to parallel 3 element to 4 step 0 perm 1
	vector unsigned char AVcSeq2Prl3x4_s0p1;

								// sequential to parallel 3 element to 4 step 0 perm 2
	vector unsigned char AVcSeq2Prl3x4_s0p2;

								// sequential to parallel 3 element to 4 step 1 perm 0
	vector unsigned char AVcSeq2Prl3x4_s1p0;

								// sequential to parallel 3 element to 4 step 1 perm 1
	vector unsigned char AVcSeq2Prl3x4_s1p1;

								// sequential to parallel 3 element to 4 step 1 perm 2
	vector unsigned char AVcSeq2Prl3x4_s1p2;

	vector unsigned char AVcPrl2Seq4x3_s0p0;
	vector unsigned char AVcPrl2Seq4x3_s0p1;
	vector unsigned char AVcPrl2Seq4x3_s0p2;
	vector unsigned char AVcPrl2Seq4x3_s1p0;
	vector unsigned char AVcPrl2Seq4x3_s1p1;
	vector unsigned char AVcPrl2Seq4x3_s1p2;

/*
 * Define the permute vectors for converting xxxx yyyy zzzz wwww to xyzw xyzw xyzw xyzw
 */
	//     sx            sy           sz            hx
	// x1 x2 x3 x4 | y1 y2 y3 y4 | z1 z2 z3 z4 | w1 w2 w3 w4
	//
	//     txy0    |     txy1    |    tzw0     |    tzw1
	// x1 x2 y1 y2 | x3 x4 y3 y4 | z1 z2 w1 w2 | z3 z4 w3 w4
	//
	//     sx      |     sy      |     sz      |    hx
	// x1 y1 z1 w1 | x2 y2 z2 w2 | x3 y3 z3 w3 | x4 y4 z4 w4

	// txy0 = perm(sx, sy)
	vector unsigned char AVcPrl2Seq4x4_s0p0;

	// txy1 = perm(sx, sy)
	vector unsigned char AVcPrl2Seq4x4_s0p1;

	// tzw0 = perm(sz, hx)
	// same as prk2Seq4x4_s0p0

	// tzw1 = perm(sz, hw)
	// same as prk2Seq4x4_s0p1

	// sx = perm(txy0, tzw0)
	vector unsigned char AVcPrl2Seq4x4_s1p0;

	// sy = perm(txy0, tzw0)
	vector unsigned char AVcPrl2Seq4x4_s1p1;

	// sz = perm(txy1, tzw1)
	// same as prk2Seq4x4_s10p0

	// hw = perm(txy1, tzw1)
	// same as prk2Seq4x4_s10p1

/*
 *
 */
	vector float AVcSplatZero;
	vector float AVcSplatOne;
	vector float AVcSplatOneMinusEpsilon;
	vector float AVcSplatNegOne;

void
UUrPlatform_SIMD_Initialize(
	void)
{

/*
 * Define the permute vectors for converting xyz xyz xyz xyz to xxxx yyyy xxxx
 */
	//      t0            t1           t2
	// 0  1  2  3    0  1  2  3    0  1  2  3
	// x1 y1 z1 x2 | y2 z2 x3 y3 | z3 x4 y4 z4

	//  perm0 t0, t1  perm1 t1, t2  perm2 t0, t2
	//      t3            t4           t5
	// 0  1  2  3    0  1  2  3    0  1  2  3
	// x1 x2 x3 z2 | y2 y3 y4 x4 | z1 z3 z4 y1

	//  perm3 t3, t4  perm4 t4, t5  perm5 t3, t5
	//      t0            t1           t2
	// 0  1  2  3    0  1  2  3    0  1  2  3
	// x1 x2 x3 x4 | y1 y2 y3 y4 | z1 z2 z3 z4

								// sequential to parallel 3 element to 4 step 0 perm 0
	AVcSeq2Prl3x4_s0p0 = // t3 = vec_perm(t0, t1, seq2Prl3x4_s0p0)
		(vector unsigned char)
		AVmPerm_Build4Byte(
			// t3[0] <- t0[0]
			0, 0,
			// t3[1] <- t0[3]
			0, 3,
			// t3[2] <- t1[2]
			1, 2,
			// t3[3] <- t1[1]
			1, 1
			);

								// sequential to parallel 3 element to 4 step 0 perm 1
	AVcSeq2Prl3x4_s0p1 = // t4 = vec_perm(t1, t2, seq2Prl3x4_s0p1)
		(vector unsigned char)
		AVmPerm_Build4Byte(
			// t4[0] <- t1[0]
			0, 0,
			// t4[1] <- t1[3]
			0, 3,
			// t4[2] <- t2[2]
			1, 2,
			// t4[3] <- t2[1]
			1, 1
			);

								// sequential to parallel 3 element to 4 step 0 perm 2
	AVcSeq2Prl3x4_s0p2 = // t5 = vec_perm(t0, t2, seq2Prl3x4_s0p2)
		(vector unsigned char)
		AVmPerm_Build4Byte(
			// t5[0] <- t0[2]
			0, 2,
			// t5[1] <- t2[0]
			1, 0,
			// t5[2] <- t2[3]
			1, 3,
			// t5[3] <- t0[1]
			0, 1
			);

								// sequential to parallel 3 element to 4 step 1 perm 0
	AVcSeq2Prl3x4_s1p0 = // t0 = vec_perm(t3, t4, seq2Prl3x4_s1p0)
		(vector unsigned char)
		AVmPerm_Build4Byte(
			// t0[0] <- t3[0]
			0, 0,
			// t0[1] <- t3[1]
			0, 1,
			// t0[2] <- t3[2]
			0, 2,
			// t0[3] <- t4[3]
			1, 3
			);

								// sequential to parallel 3 element to 4 step 1 perm 1
	AVcSeq2Prl3x4_s1p1 = // t1 = vec_perm(t4, t5, seq2Prl3x4_s1p1)
		(vector unsigned char)
		AVmPerm_Build4Byte(
			// t1[0] <- t5[3]
			1, 3,
			// t1[1] <- t4[0]
			0, 0,
			// t1[2] <- t4[1]
			0, 1,
			// t1[3] <- t4[2]
			0, 2
			);

								// sequential to parallel 3 element to 4 step 1 perm 2
	AVcSeq2Prl3x4_s1p2 =  // t2 = vec_perm(t3, t5, seq2Prl3x4_s1p2)
		(vector unsigned char)
		AVmPerm_Build4Byte(
			// t2[0] <- t5[0]
			1, 0,
			// t2[1] <- t3[3]
			0, 3,
			// t2[2] <- t5[1]
			1, 1,
			// t2[3] <- t5[2]
			1, 2
			);

/*
 * Define the permute vectors for converting xxxx yyyy zzzz to xyzx yzxy zxyz
 */
	//      t0            t1           t2
	// 0  1  2  3    0  1  2  3    0  1  2  3
	// x1 x2 x3 x4 | y1 y2 y3 y4 | z1 z2 z3 z4

	// perm t0, t2 | perm t0, t1 | perm t1, t2	<- s0
	// 00 01 02 11 | 11 12 13 03 | 10 12 13 00

	//      t3            t4           t5
	// 0  1  2  3    0  1  2  3    0  1  2  3
	// x1 x2 x3 z2 | y2 y3 y4 x4 | z1 z3 z4 y1

	// perm t3, t5 | perm t3, t4 | perm t4, t5 <- s1
	// 00 13 10 01 | 10 03 02 11 | 11 03 02 12

	//      t0            t1           t2
	// 0  1  2  3    0  1  2  3    0  1  2  3
	// x1 y1 z1 x2 | y2 z2 x3 y3 | z3 x4 y4 z4

								// parallel to sequential 4 element to 3 step 0 perm 0
	AVcPrl2Seq4x3_s0p0 = // t3 = vec_perm(t0, t2, seq2Prl3x4_s0p0)
		(vector unsigned char)
		AVmPerm_Build4Byte(
			// t3[0] <- t0[0]
			0, 0,
			// t3[1] <- t0[1]
			0, 1,
			// t3[2] <- t0[2]
			0, 2,
			// t3[3] <- t2[1]
			1, 1
			);

								// parallel to sequential 4 element to 3 step 0 perm 1
	AVcPrl2Seq4x3_s0p1 = // t4 = vec_perm(t0, t1, seq2Prl3x4_s0p1)
		(vector unsigned char)
		AVmPerm_Build4Byte(
			// t4[0] <- t1[1]
			1, 1,
			// t4[1] <- t1[2]
			1, 2,
			// t4[2] <- t1[3]
			1, 3,
			// t4[3] <- t0[3]
			0, 3
			);

								// parallel to sequential 4 element to 3 step 0 perm 2
	AVcPrl2Seq4x3_s0p2 = // t5 = vec_perm(t1, t2, seq2Prl3x4_s0p2)
		(vector unsigned char)
		AVmPerm_Build4Byte(
			// t5[0] <- t2[0]
			1, 0,
			// t5[1] <- t2[2]
			1, 2,
			// t5[2] <- t2[3]
			1, 3,
			// t5[3] <- t1[0]
			0, 0
			);

								// parallel to sequential 4 element to 3 step 1 perm 0
	AVcPrl2Seq4x3_s1p0 = // t0 = vec_perm(t3, t5, seq2Prl3x4_s1p0)
		(vector unsigned char)
		AVmPerm_Build4Byte(
			// t0[0] <- t3[0]
			0, 0,
			// t0[1] <- t5[3]
			1, 3,
			// t0[2] <- t5[0]
			1, 0,
			// t0[3] <- t3[1]
			0, 1
			);

								// parallel to sequential 4 element to 3 step 1 perm 1
	AVcPrl2Seq4x3_s1p1 = // t1 = vec_perm(t3, t4, seq2Prl3x4_s1p1)
		(vector unsigned char)
		AVmPerm_Build4Byte(
			// t1[0] <- t4[0]
			1, 0,
			// t1[1] <- t3[3]
			0, 3,
			// t1[2] <- t3[2]
			0, 2,
			// t1[3] <- t4[1]
			1, 1
			);

								// parallel to sequential 4 element to 3 step 1 perm 2
	AVcPrl2Seq4x3_s1p2 = // t2 = vec_perm(t4, t5, seq2Prl3x4_s1p1)
		(vector unsigned char)
		AVmPerm_Build4Byte(
			// t2[0] <- t5[1]
			1, 1,
			// t2[1] <- t4[3]
			0, 3,
			// t2[2] <- t4[2]
			0, 2,
			// t2[3] <- t5[2]
			1, 2
			);


/*
 * Define the permute vectors for converting xxxx yyyy zzzz wwww to xyzw xyzw xyzw xyzw
 */
	//     sx            sy           sz            hx
	// x1 x2 x3 x4 | y1 y2 y3 y4 | z1 z2 z3 z4 | w1 w2 w3 w4
	//
	//     txy0    |     txy1    |    tzw0     |    tzw1
	// x1 x2 y1 y2 | x3 x4 y3 y4 | z1 z2 w1 w2 | z3 z4 w3 w4
	//
	//     sx      |     sy      |     sz      |    hx
	// x1 y1 z1 w1 | x2 y2 z2 w2 | x3 y3 z3 w3 | x4 y4 z4 w4

	// txy0 = perm(sx, sy)
	AVcPrl2Seq4x4_s0p0 =	// txy0 = prl2Seq4x4_s0p0(sx, sy, prl2Seq4x4_s0p0)
		(vector unsigned char)
		AVmPerm_Build4Byte(
			// txy0[0] <- sx[0]
			0, 0,
			// txy0[1] <- sx[1]
			0, 1,
			// txy0[2] <- sy[0]
			1, 0,
			// txy0[3] <- sy[1]
			1, 1
			);

	// txy1 = perm(sx, sy)
	AVcPrl2Seq4x4_s0p1 =	// txy0 = prl2Seq4x4_s0p0(sx, sy, prl2Seq4x4_s0p0)
		(vector unsigned char)
		AVmPerm_Build4Byte(
			// txy1[0] <- sx[2]
			0, 2,
			// txy1[1] <- sx[3]
			0, 3,
			// txy1[2] <- sy[2]
			1, 2,
			// txy1[3] <- sy[3]
			1, 3
			);

	// tzw0 = perm(sz, hx)
	// same as prk2Seq4x4_s0p0

	// tzw1 = perm(sz, hw)
	// same as prk2Seq4x4_s0p1

	// sx = perm(txy0, tzw0)
	AVcPrl2Seq4x4_s1p0 =	// sx = prl2Seq4x4_s10p0(txy0, tzw0, prl2Seq4x4_s0p0)
		(vector unsigned char)
		AVmPerm_Build4Byte(
			// t2[0] <- txy0[0]
			0, 0,
			// t2[1] <- txy0[2]
			0, 2,
			// t2[2] <- tzw0[0]
			1, 0,
			// t2[3] <- tzw0[2]
			1, 2
			);

	// sy = perm(txy0, tzw0)
	AVcPrl2Seq4x4_s1p1 =	// sx = prl2Seq4x4_s10p0(txy0, tzw0, prl2Seq4x4_s0p0)
		(vector unsigned char)
		AVmPerm_Build4Byte(
			// t2[0] <- txy0[1]
			0, 1,
			// t2[1] <- txy0[3]
			0, 3,
			// t2[2] <- tzw0[1]
			1, 1,
			// t2[3] <- tzw0[3]
			1, 3
			);

	// sz = perm(txy1, tzw1)
	// same as prk2Seq4x4_s10p0

	// hw = perm(txy1, tzw1)
	// same as prk2Seq4x4_s10p1

/*
 *
 */
	AVcSplatZero = (vector float)(0.0f, 0.0f, 0.0f, 0.0f);
	AVcSplatOne = (vector float)(1.0f, 1.0f, 1.0f, 1.0f);
	AVcSplatOneMinusEpsilon = (vector float)(255.0f/256.0f,255.0f/256.0f, 255.0f/256.0f, 255.0f/256.0f);
	AVcSplatNegOne = (vector float)(-1.0f, -1.0f, -1.0f, -1.0f);

	if(1)
	{
		vector unsigned short	d;
		const vector unsigned short setNJ = (const vector unsigned short)(0, 0, 0, 0, 0, 0, 1, 0);
		d = vec_mfvscr();

		d = vec_or(d, setNJ);

		vec_mtvscr(d);
	}
}

void
AVrFloat_XYZ4ToXXXXYYYYZZZZ(
	UUtUns16	inNum,
	float*		inXYZ,
	float		*outXXXXYYYYZZZZ)
{

	vector unsigned char	seq2Prl3x4_s0p0 = AVcSeq2Prl3x4_s0p0;
	vector unsigned char	seq2Prl3x4_s0p1 = AVcSeq2Prl3x4_s0p1;
	vector unsigned char	seq2Prl3x4_s0p2 = AVcSeq2Prl3x4_s0p2;
	vector unsigned char	seq2Prl3x4_s1p0 = AVcSeq2Prl3x4_s1p0;
	vector unsigned char	seq2Prl3x4_s1p1 = AVcSeq2Prl3x4_s1p1;
	vector unsigned char	seq2Prl3x4_s1p2 = AVcSeq2Prl3x4_s1p2;

	vector float	p0, p1, p2;
	vector float	p3, p4, p5;

	vector float*	curPointSrc = (vector float*)inXYZ;
	vector float*	curPointDst = (vector float*)outXXXXYYYYZZZZ;

	UUtInt32	itr;

	UUmAssertReadPtr(inXYZ, sizeof(*inXYZ));
	UUmAssertReadPtr(outXXXXYYYYZZZZ, sizeof(*outXXXXYYYYZZZZ));
	UUmAssert(((UUtUns32)curPointSrc & 0xF) == 0);
	UUmAssert(((UUtUns32)curPointDst & 0xF) == 0);

	for(itr = (inNum + 3) >> 2; itr-- > 0; curPointSrc += 3, curPointDst += 3)
	{
		p0 = curPointSrc[0];
		p1 = curPointSrc[1];
		p2 = curPointSrc[2];

		// point step 0
		p3 = vec_perm(p0, p1, seq2Prl3x4_s0p0);
		p4 = vec_perm(p1, p2, seq2Prl3x4_s0p1);
		p5 = vec_perm(p0, p2, seq2Prl3x4_s0p2);

		// point step 1
		p0 = vec_perm(p3, p4, seq2Prl3x4_s1p0);
		p1 = vec_perm(p4, p5, seq2Prl3x4_s1p1);
		p2 = vec_perm(p3, p5, seq2Prl3x4_s1p2);

		curPointDst[0] = p0;
		curPointDst[1] = p1;
		curPointDst[2] = p2;
	}

}


#else
void
UUrPlatform_SIMD_Initialize(
	void)
{
}
#endif

