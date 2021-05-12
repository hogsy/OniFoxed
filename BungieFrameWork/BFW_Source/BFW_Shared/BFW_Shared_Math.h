/*
	FILE:	BFW_Shared_Math.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Oct 23, 1997
	
	PURPOSE: 
	
	Copyright 1997

	--                 --    --                 --    --                 --
	|  A00 A10 A20 A30  |    |  B00 B10 B20 B30  |    |  C00 C10 C20 C30  |
	|  A01 A11 A21 A31  | \/ |  B01 B11 B21 B31  | == |  C01 C11 C21 C31  |
	|  A02 A12 A22 A32  | /\ |  B02 B12 B22 B32  | == |  C02 C12 C22 C32  |
	|  A03 A13 A23 A33  |    |  B03 B13 B23 B33  |    |  C03 C13 C23 C33  |
	--                 --    --                 --    --                 --
		
		C00 = A00 * B00 + A10 * B01 + A20 * B02 + A30 * B03
		
		...
		
		C33 = A02 * B30 + A13 * B31 + A23 * B32 + A33 * B33
	
		NOTE: If the product of AB is applied to a vertex as in ABv
				the transformation defined in B affects vertex v before
				the transformation defined in A, ie OpenGL.
		
	--                 --    --   --    --    --
	|  A00 A10 A20 A30  |    |  X  |    |  X'  |
	|  A01 A11 A21 A31  | \/ |  Y  | == |  Y'  |
	|  A02 A12 A22 A32  | /\ |  Z  | == |  Z'  |
	|  A03 A13 A23 A33  |    |  W  |    |  W'  |
	--                 --	 --   --    --    --
	
		X' = A00 * X + A10 * Y + A20 * Z + A30 * W
		
		...
		
		W' = A03 * X + A13 * Y + A23 * Z + A33 * W
	
	In the camera and viewing code a 4x4 matrix is interpreted as follows
	
	--                 --    --   --    --    --
	|  A00 A10 A20 T30  |    |  X  |    |  X'  |
	|  A01 A11 A21 T31  | \/ |  Y  | == |  Y'  |
	|  A02 A12 A22 T32  | /\ |  Z  | == |  Z'  |
	|  P03 P13 P23 0.0  |    |  W  |    |  W'  |
	--                 --	 --   --    --    --
	
	T30, T31, T32 is used for translation
	
	P03, P13, P23 is used for perspective correction
	
	Aij is used for scale and rotation. Note that nonuniform scales are *not* allowed since
	this would require an inverstion for computing normals.
*/
#ifndef BFW_SHARED_MATH_H
#define BFW_SHARED_MATH_H

#include "BFW.h"
#include "BFW_TemplateManager.h"
#include "BFW_Motoko.h"
#include "BFW_MathLib.h"

#define MSmTransform_FrustumClipTest(hX, hY, hZ, hW, negW, clipCode, needs4DClipping) \
		if(hW <= 0.0f)								\
		{											\
			needs4DClipping = UUcTrue;				\
		}											\
													\
		clipCode = MScClipCode_None;				\
													\
		if(hX > hW)									\
		{											\
			clipCode |= MScClipCode_PosX;			\
		}											\
		if(hX < negW)								\
		{											\
			clipCode |= MScClipCode_NegX;			\
		}											\
		if(hY > hW)									\
		{											\
			clipCode |= MScClipCode_PosY;			\
		}											\
		if(hY < negW)								\
		{											\
			clipCode |= MScClipCode_NegY;			\
		}											\
		if(hZ > hW)									\
		{											\
			clipCode |= MScClipCode_PosZ;			\
		}											\
		if(hZ < 0.0)								\
		{											\
			clipCode |= MScClipCode_NegZ;			\
		}

#define MSmTransform_FrustumClipTest4DOnly(hX, hY, hZ, hW, negW, clipCode) \
		clipCode = MScClipCode_None;				\
													\
		if(hX > hW)									\
		{											\
			clipCode |= MScClipCode_PosX;			\
		}											\
		if(hX < negW)								\
		{											\
			clipCode |= MScClipCode_NegX;			\
		}											\
		if(hY > hW)									\
		{											\
			clipCode |= MScClipCode_PosY;			\
		}											\
		if(hY < negW)								\
		{											\
			clipCode |= MScClipCode_NegY;			\
		}											\
		if(hZ > hW)									\
		{											\
			clipCode |= MScClipCode_PosZ;			\
		}											\
		if(hZ < 0.0)								\
		{											\
			clipCode |= MScClipCode_NegZ;			\
		}

#define MSmTransform_ScreenClipTest(sX, sY, sZ, clipCode, screenWidth, screenHeight) \
		clipCode = MScClipCode_None;		\
											\
		if(sX >= screenWidth)				\
		{									\
			clipCode |= MScClipCode_PosX;	\
		}									\
		if(sX < 0.0f)						\
		{									\
			clipCode |= MScClipCode_NegX;	\
		}									\
		if(sY >= screenHeight)				\
		{									\
			clipCode |= MScClipCode_NegY;	\
		}									\
		if(sY < 0.0f)						\
		{									\
			clipCode |= MScClipCode_PosY;	\
		}									\
		if(sZ >= 1.0f)						\
		{									\
			clipCode |= MScClipCode_PosZ;	\
		}									\
		if(sZ < 0.0)						\
		{									\
			clipCode |= MScClipCode_NegZ;	\
		}

#define MSmTransform_Frustum2Screen(sX, sY, sZ, sInvW, hX, hY, hZ, invW, scaleX, scaleY) \
		sX = (hX * invW + 1.0f) * scaleX;	\
		sY = (hY * invW - 1.0f) * scaleY;	\
		sZ = hZ * invW;						\
		sInvW = invW

#define MSmTransform_Local2FrustumInvWNegW(	\
				lX, lY, lZ,					\
				hX, hY, hZ, hW, invW, negW,	\
				m00, m10, m20, m30,			\
				m01, m11, m21, m31, 		\
				m02, m12, m22, m32,			\
				m03, m13, m23, m33)			\
											\
		hW = m03 * lX + m13 * lY + m23 * lZ + m33;	\
		hX = m00 * lX + m10 * lY + m20 * lZ + m30;	\
		invW = 1.0f / hW;							\
		negW = -hW;									\
		hY = m01 * lX + m11 * lY + m21 * lZ + m31;	\
		hZ = m02 * lX + m12 * lY + m22 * lZ + m32;
		
#define MSmTransform_Local2FrustumInvW(		\
				lX, lY, lZ,					\
				hX, hY, hZ, hW, invW,		\
				m00, m10, m20, m30,			\
				m01, m11, m21, m31, 		\
				m02, m12, m22, m32,			\
				m03, m13, m23, m33)			\
											\
		hW = m03 * lX + m13 * lY + m23 * lZ + m33;	\
		hX = m00 * lX + m10 * lY + m20 * lZ + m30;	\
		invW = 1.0f / hW;							\
		hY = m01 * lX + m11 * lY + m21 * lZ + m31;	\
		hZ = m02 * lX + m12 * lY + m22 * lZ + m32;
		
#define MSmTransform_Local2FrustumNegW(		\
				lX, lY, lZ,					\
				hX, hY, hZ, hW, negW,		\
				m00, m10, m20, m30,			\
				m01, m11, m21, m31, 		\
				m02, m12, m22, m32,			\
				m03, m13, m23, m33)			\
											\
		hW = m03 * lX + m13 * lY + m23 * lZ + m33;	\
		hX = m00 * lX + m10 * lY + m20 * lZ + m30;	\
		negW = -hW;									\
		hY = m01 * lX + m11 * lY + m21 * lZ + m31;	\
		hZ = m02 * lX + m12 * lY + m22 * lZ + m32;
		
#define MSmTransform_Matrix4x4ToRegisters(	\
				matrix,							\
				m00, m10, m20, m30,			\
				m01, m11, m21, m31, 		\
				m02, m12, m22, m32,			\
				m03, m13, m23, m33)			\
										\
			m00 = (matrix).m[0][0];		\
			m01 = (matrix).m[0][1];		\
			m02 = (matrix).m[0][2];		\
			m03 = (matrix).m[0][3];		\
										\
			m10 = (matrix).m[1][0];		\
			m11 = (matrix).m[1][1];		\
			m12 = (matrix).m[1][2];		\
			m13 = (matrix).m[1][3];		\
										\
			m20 = (matrix).m[2][0];		\
			m21 = (matrix).m[2][1];		\
			m22 = (matrix).m[2][2];		\
			m23 = (matrix).m[2][3];		\
										\
			m30 = (matrix).m[3][0];		\
			m31 = (matrix).m[3][1];		\
			m32 = (matrix).m[3][2];		\
			m33 = (matrix).m[3][3];

#define MSmTransform_Matrix3x3ToRegisters(	\
				matrix,							\
				m00, m10, m20,			\
				m01, m11, m21, 		\
				m02, m12, m22)			\
										\
			m00 = (matrix).m[0][0];		\
			m01 = (matrix).m[0][1];		\
			m02 = (matrix).m[0][2];		\
										\
			m10 = (matrix).m[1][0];		\
			m11 = (matrix).m[1][1];		\
			m12 = (matrix).m[1][2];		\
										\
			m20 = (matrix).m[2][0];		\
			m21 = (matrix).m[2][1];		\
			m22 = (matrix).m[2][2];

#endif /* BFW_SHARED_MATH_H */

