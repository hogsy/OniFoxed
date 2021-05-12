/*
	FILE:	MG_DrawEngine_Method_Ptrs.h
	
	AUTHOR:	Brent Pease
	
	CREATED: July 31, 1999
	
	PURPOSE: 
	
	Copyright 1997 - 1999

*/

#ifndef MG_DRAWENGINE_METHOD_PTRS_H
#define MG_DRAWENGINE_METHOD_PTRS_H


extern M3tDrawContextMethod_Line		MGgLineFuncs[M3cDrawState_Interpolation_Num];

extern MGtDrawTri				MGgTriangleFuncs_VertexUnified
											[M3cDrawState_Fill_Num]
											[M3cDrawState_Appearence_Num]
											[M3cDrawState_Interpolation_Num];
extern MGtDrawTriSplit			MGgTriangleFuncs_VertexSplit
											[M3cDrawState_Fill_Num]
											[M3cDrawState_Appearence_Num]
											[M3cDrawState_Interpolation_Num]
											[MGcDrawState_TMU_Num];

extern MGtDrawQuad				MGgQuadFuncs_VertexUnified
											[M3cDrawState_Fill_Num]
											[M3cDrawState_Appearence_Num]
											[M3cDrawState_Interpolation_Num];
extern MGtDrawQuadSplit			MGgQuadFuncs_VertexSplit
											[M3cDrawState_Fill_Num]
											[M3cDrawState_Appearence_Num]
											[M3cDrawState_Interpolation_Num]
											[MGcDrawState_TMU_Num];

extern MGtDrawPent				MGgPentFuncs_VertexUnified
											[M3cDrawState_Fill_Num]
											[M3cDrawState_Appearence_Num]
											[M3cDrawState_Interpolation_Num];
extern MGtDrawPentSplit			MGgPentFuncs_VertexSplit
											[M3cDrawState_Fill_Num]
											[M3cDrawState_Appearence_Num]
											[M3cDrawState_Interpolation_Num]
											[MGcDrawState_TMU_Num];

extern MGtModeFunction			MGgModeFuncs_VertexUnified
											[M3cDrawState_Fill_Num]
											[M3cDrawState_Appearence_Num]
											[M3cDrawState_Interpolation_Num];
extern MGtModeFunction			MGgModeFuncs_VertexSplit
											[M3cDrawState_Fill_Num]
											[M3cDrawState_Appearence_Num]
											[M3cDrawState_Interpolation_Num]
											[MGcDrawState_TMU_Num];

extern MGtVertexCreateFunction	MGgVertexCreateFuncs_Unified
											[M3cDrawState_Appearence_Num]
											[M3cDrawState_Interpolation_Num];

extern MGtVertexCreateFunction	MGgVertexCreateFuncs_Split
											[M3cDrawState_Appearence_Num]
											[M3cDrawState_Interpolation_Num]
											[MGcDrawState_TMU_Num];

#endif /* MG_DRAWENGINE_METHOD_PTRS_H */
