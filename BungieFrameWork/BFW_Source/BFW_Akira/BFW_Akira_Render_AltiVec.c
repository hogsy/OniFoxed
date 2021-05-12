/*
	FILE:	BFW_Akira_Render_AltiVec.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: October 12, 1998

	PURPOSE: environment engine
	
	Copyright 1997

*/

#if 0 // S.S.

#if UUmSIMD == UUmSIMD_AltiVec

#include "BFW.h"
#include "BFW_TemplateManager.h"
#include "BFW_Motoko.h"
#include "BFW_Akira.h"
#include "BFW_Console.h"
#include "BFW_BitVector.h"
#include "BFW_AppUtilities.h"
#include "BFW_Collision.h"
#include "BFW_Shared_Math.h"
#include "BFW_BitVector.h"
#include "BFW_Object.h"
#include "BFW_Platform_AltiVec.h"

#include "Akira_Private.h"

#define UUmAltiVec_Trace 0

#if defined(UUmAltiVec_Trace) && UUmAltiVec_Trace

#include "pitsTT6lib.h"

#endif



vector float*	AKgSIMDRays[AKcNumTemperalFrames] = {NULL, NULL, NULL};

#define DEBUG_COLLISION2_TRAVERSAL	0
#define UUmRayCast_AltiVec_Refine	0

#if 1

#if UUmCompiler == UUmCompiler_MrC
#pragma inline_func AKiQuad_PointInQuad
#endif

static UUtBool
AKiQuad_PointInQuad(
	CLtQuadProjection	inProjection,
	const M3tPoint3D*	inPointArray,
	const M3tQuad*		inQuad,
	const vector float	inVecPoint)
{
	
	vector float	splat_wt, splat_ut, splat_vt;
	
	vector float	u0123;
	vector float	v0123;
	vector float	w0123;
	vector float	last;
	vector float	vI0, vI1, vI2, vI3;
	
	vector float	u1230, v1230;
	
	const vector float splat_epsilon = (const vector float)(UUcEpsilon, UUcEpsilon, UUcEpsilon, UUcEpsilon);
	const vector float splat_neg_epsilon = (const vector float)(-UUcEpsilon, -UUcEpsilon, -UUcEpsilon, -UUcEpsilon);
	const vector float splat_zero = (vector float)((vector unsigned long)(0,0,0,0)); //AVcSplatZero;
	
	const vector unsigned char perm_XZ = (const vector unsigned char)
		AVmPerm_Build4Byte(
			0, 0,
			0, 2,
			0, 1,
			0, 0);
	const vector unsigned char perm_YZ = (const vector unsigned char)
		AVmPerm_Build4Byte(
			0, 1,
			0, 2,
			0, 0,
			0, 0);

	const M3tPoint3D*		curPoint;
	
	vector float	diff;
	vector float	vt0, vt1, vt2, vt3;
	
	if(inProjection == CLcProjection_Unknown)
	{
		inProjection = CLrQuad_FindProjection(inPointArray, inQuad);
		UUmAssert(inProjection != CLcProjection_Unknown);		// should be discarded at import time
	}
	
	curPoint = inPointArray + inQuad->indices[0];
	AVmLoadMisalignedVectorPoint(curPoint, u0123);
	
	curPoint = inPointArray + inQuad->indices[1];
	AVmLoadMisalignedVectorPoint(curPoint, v0123);
	
	curPoint = inPointArray + inQuad->indices[2];
	AVmLoadMisalignedVectorPoint(curPoint, w0123);
	
	curPoint = inPointArray + inQuad->indices[3];
	AVmLoadMisalignedVectorPoint(curPoint, last);

	if(inProjection == CLcProjection_XY)
	{
		splat_ut = vec_splat(inVecPoint, 0);
		splat_vt = vec_splat(inVecPoint, 1);
		splat_wt = vec_splat(inVecPoint, 2);
		
	}
	else if(inProjection == CLcProjection_XZ)
	{
		splat_ut = vec_splat(inVecPoint, 0);
		splat_vt = vec_splat(inVecPoint, 2);
		splat_wt = vec_splat(inVecPoint, 1);
		
		u0123 = vec_perm(u0123, u0123, perm_XZ);
		v0123 = vec_perm(v0123, v0123, perm_XZ);
		w0123 = vec_perm(w0123, w0123, perm_XZ);
		last = vec_perm(last, last, perm_XZ);
		
	}
	else
	{
		splat_ut = vec_splat(inVecPoint, 1);
		splat_vt = vec_splat(inVecPoint, 2);
		splat_wt = vec_splat(inVecPoint, 0);

		u0123 = vec_perm(u0123, u0123, perm_YZ);
		v0123 = vec_perm(v0123, v0123, perm_YZ);
		w0123 = vec_perm(w0123, w0123, perm_YZ);
		last = vec_perm(last, last, perm_YZ);
	}
	
	vI0 = vec_mergeh ( u0123, w0123 );
	vI1 = vec_mergeh ( v0123, last );
	vI2 = vec_mergel ( u0123, w0123 );
	vI3 = vec_mergel ( v0123, last );

	u0123 = vec_mergeh ( vI0, vI1 );
	v0123 = vec_mergel ( vI0, vI1 );
	w0123 = vec_mergeh ( vI2, vI3 );

	diff = vec_sub(splat_wt, w0123);
	
	//if(vec_all_gt(diff, splat_epsilon)) return UUcFalse;
	//if(vec_all_lt(diff, splat_neg_epsilon)) return UUcFalse;
	
	/*
		If the point is in the quad we have something like this
		
	                     *0
	                    /|\
	                   / | \
	                  /  |  \
	                 /   |   \
	                /    |    \
	              3*-----*t----*1
	                \    |    /
	                 \   |   /
	                  \  |  /
	                   \ | /
	                    \|/
	                     *2
	  either:
	  
	  |t0 x t1| <= 0 &&
	  |t1 x t2| <= 0 &&
	  |t2 x t3| <= 0 &&
	  |t3 x t0| <= 0
	  
	  or
	  
	  |t0 x t1| >= 0 &&
	  |t1 x t2| >= 0 &&
	  |t2 x t3| >= 0 &&
	  |t3 x t0| >= 0
	  
	  (It depends on the orientation of the vertices)
	 
	  if the above conditions are not met the point is outside the quad
	  
	*/
	
	u1230 = (vector float)vec_sld((vector unsigned char)u0123, (vector unsigned char)u0123, 4);
	v1230 = (vector float)vec_sld((vector unsigned char)v0123, (vector unsigned char)v0123, 4);
	
	vt0 = vec_sub(u0123, splat_ut);
	vt1 = vec_sub(v1230, splat_vt);
	vt2 = vec_sub(v0123, splat_vt);
	vt3 = vec_sub(u1230, splat_ut);
	
	vt2 = vec_madd(vt2, vt3, splat_zero);
	vt0 = vec_nmsub(vt0, vt1, vt2);
	
	if(vec_all_gt(vt0, splat_neg_epsilon)) return UUcTrue;
	if(vec_all_lt(vt0, splat_epsilon)) return UUcTrue;
	
	return UUcFalse;
}
#endif

#define BRENTS_CHEESY_PROFILE 0

#if BRENTS_CHEESY_PROFILE

	extern UUtUns32	funcTime;
	extern UUtUns32	funcNum;

	extern UUtUns32	funcRayTime;
	extern UUtUns32	funcNumRays;
	
	extern UUtUns32	funcOctantTime;
	extern UUtUns32	funcNumMissOctants;
	extern UUtUns32	funcNumHitOctants;
	extern UUtUns32	funcNumSkippedOctants;
	
	extern UUtUns32	funcMissOctant_QuadTraversalTime;
	extern UUtUns32	funcMissOctant_NumQuadTraversals;
	extern UUtUns32	funcMissOctant_NumQuadChecks;
	
	extern UUtUns32	funcHitOctant_QuadTraversalTime;
	extern UUtUns32	funcHitOctant_NumQuadTraversals;
	extern UUtUns32	funcHitOctant_NumQuadChecks;
	
	extern UUtUns32	funcHitOctant_NumQuadsTillHit;

#endif

void
AKrEnvironment_RayCastOctTree_SIMD(
	AKtEnvironment*		inEnvironment,
	M3tGeomCamera*		inCamera)
{
#if 1
#if BRENTS_CHEESY_PROFILE
	UUtUns64	funcStart;
	
	#if !BRENTS_CHEESY_PROFILE_ONLY_FUNC
	
	UUtUns64	funcRayStart;
	UUtUns64	funcOctantStart;
	UUtUns64	funcQuadTraversalStart;
	
	UUtUns32	numQuadChecks;
	
	#endif
	
	UUtUns64	funcEnd;
	
#endif

	UUtUns32*					octTreeGQIndices;
	M3tPoint3D*					pointArray;
	vector float*				planeEquArray;
	AKtOctTree_InteriorNode*	interiorNodeArray;
	AKtOctTree_LeafNode*		leafNodeArray;
	AKtQuadTree_Node*			qtInteriorNodeArray;
	AKtQuadTree_Node*			curQTIntNode;
	
	UUtUns32					curGQIndIndex;

	UUtUns32					curGQIndex;
	
	AKtGQ_General*				gqGeneralArray;
	AKtGQ_Collision*			gqCollisionArray;
	AKtGQ_General*				curGQGeneral;
	AKtGQ_Collision*			curGQCollision;

	UUtUns16					sideCrossing;
	
	//UUtUns16					endNodeIndex;
	UUtUns16					curNodeIndex;
	AKtOctTree_LeafNode*		curOTLeafNode;
	UUtUns16					curOctant;
	
	#if 0
	float						curCenterPointU;
	float						curCenterPointV;
	

	float						curHalfDim;
	float						uc, vc;
	#endif
	
	M3tPoint3D					frustumWorldPoints[8];
	M3tPlaneEquation			frustumWorldPlanes[6];
	M3tPoint3D					cameraLocation;
	
	vector float				uv_far_initial_xyz;
	vector float				u_far_delta_xyz;
	vector float				v_far_delta_xyz;
	vector float				uv_near_initial_xyz;
	vector float				u_near_delta_xyz;
	vector float				v_near_delta_xyz;
	
	vector float				invVXYZ;
	vector float				maxXYZ;
	vector float				minXYZ;
	vector float				planeEqu;
	vector float				viewVectorXYZ;

	vector float				denom;
	vector float				num1;
	
	vector float				vector_t0, vector_t1, vector_t2;
	vector unsigned int			int_t0;

	vector float				testPointXYZ;
	
	UUtUns32					itr, numRays;
	AKtRay*						curRay;
		
	UUtUns16					startNodeIndex;
	
	UUtBool						stopChecking;
	
	M3tPoint3D*					curPoint;

	UUtUns32					startIndirectIndex;
	UUtUns8						length;
	
	UUtBool						gotPlaneEqu;
				

	UUtUns32*					gqComputedBackFaceBV;
	UUtUns32*					gqBackFaceBV;
		
	float						curFullDim;
	
	const vector float			splat_onehalf = (const vector float)(0.5f, 0.5f, 0.5f, 0.5f);
	vector float				planeMinusStartXYZ;

	AKtEnvironment_Private*	environmentPrivate = (AKtEnvironment_Private*)(TMrTemplate_PrivateData_GetDataPtr(AKgTemplate_PrivateData, inEnvironment));
	
			
	const vector float splat_one = vec_ctf((vector unsigned long)(1,1,1,1), 0); //AVcSplatOne;
	const vector float splat_zero = (vector float)((vector unsigned long)(0,0,0,0)); //AVcSplatZero;
	const vector unsigned int splat_int_one = (const vector unsigned int)(1, 1, 1, 1);
	const vector unsigned int splat_neg_xyz = (const vector unsigned int)(AKcOctTree_Side_NegX, AKcOctTree_Side_NegY, AKcOctTree_Side_NegZ, 0);
	const vector unsigned int shift_XYZ = (const vector unsigned int)(AKcOctTree_Shift_X, AKcOctTree_Shift_Y, AKcOctTree_Shift_Z, 0);
	const vector unsigned int mask_XYZ = (const vector unsigned int)(AKcOctTree_Mask_X, AKcOctTree_Mask_Y, AKcOctTree_Mask_Z, 0);
	const vector float splat_255 = (const vector float)(255.0f, 255.0f, 255.0f, 255.0f);
	const vector float splat_minOTDim = (const vector float)(AKcMinOTDim, AKcMinOTDim, AKcMinOTDim, AKcMinOTDim);
	const vector unsigned char vPerm = (const vector unsigned char)
		AVmPerm_Build4Byte(
			0, 2,			// vZ
			0, 1,			// vY
			0, 2,			// vZ
			0, 0);			// vX
	const vector unsigned char pmsPerm = (const vector unsigned char)
		AVmPerm_Build4Byte(
			0, 1,			// pmsY
			0, 2,			// pmsZ
			0, 0,			// pmsX
			0, 2);			// pmsZ
	const vector unsigned char primPerm = (const vector unsigned char)
		AVmPerm_Build4Byte(
			0, 1,			// vY
			0, 0,			// vX
			0, 0,			// vX
			0, 0);			// vX

	#if defined(DEBUG_COLLISION2_TRAVERSAL) && DEBUG_COLLISION2_TRAVERSAL
		M3tBoundingBox_MinMax	bBox;
		UUtBool					in;
	#endif
	
	#if BRENTS_CHEESY_PROFILE
		funcNum++;
		funcStart = UUrMachineTime_High();
	#endif
	
	M3rCamera_GetWorldFrustum(
		inCamera,
		frustumWorldPoints,
		frustumWorldPlanes);
	
	M3rCamera_GetViewData(
		inCamera,
		&cameraLocation,
		NULL,
		NULL);
	
	pointArray = inEnvironment->pointArray->points;
	planeEquArray = (vector float*)inEnvironment->planeArray->planes;
	interiorNodeArray = inEnvironment->octTree->interiorNodeArray->nodes;
	leafNodeArray = inEnvironment->octTree->leafNodeArray->nodes;
	qtInteriorNodeArray = inEnvironment->octTree->qtNodeArray->nodes;
	gqGeneralArray = inEnvironment->gqGeneralArray->gqGeneral;
	gqCollisionArray = inEnvironment->gqCollisionArray->gqCollision;
	octTreeGQIndices = inEnvironment->octTree->gqIndices->indices;
	
	gqComputedBackFaceBV = environmentPrivate->gqComputedBackFaceBV;
	gqBackFaceBV = environmentPrivate->gqBackFaceBV;
	
	//UUrMemory_Clear(environmentPrivate->gqVisibilityBV, (inEnvironment->gqArray->numGQs + 3) >> 2);
	
	UUrBitVector_ClearBitAll(
		environmentPrivate->gqComputedBackFaceBV,
		inEnvironment->gqGeneralArray->numGQs);

	UUrBitVector_ClearBitAll(
		environmentPrivate->gqBackFaceBV,
		inEnvironment->gqGeneralArray->numGQs);

	UUrBitVector_ClearBitAll(
		environmentPrivate->otLeafNodeVisibility,
		inEnvironment->octTree->leafNodeArray->numNodes);

	// go into the start node
		startNodeIndex = AKrFindOctTreeNodeIndex(interiorNodeArray, cameraLocation.x, cameraLocation.y, cameraLocation.z);		
	
	// cache the interpolation bases
		*((float*)&uv_far_initial_xyz + 0) = frustumWorldPoints[4].x;
		*((float*)&uv_far_initial_xyz + 1) = frustumWorldPoints[4].y;
		*((float*)&uv_far_initial_xyz + 2) = frustumWorldPoints[4].z;
		*((float*)&uv_far_initial_xyz + 3) = 0.0f;
		
		*((float*)&u_far_delta_xyz + 0) = frustumWorldPoints[5].x;
		*((float*)&u_far_delta_xyz + 1) = frustumWorldPoints[5].y;
		*((float*)&u_far_delta_xyz + 2) = frustumWorldPoints[5].z;
		*((float*)&u_far_delta_xyz + 3) = 0.0f;
		
		u_far_delta_xyz = vec_sub(u_far_delta_xyz, uv_far_initial_xyz);
		
		*((float*)&v_far_delta_xyz + 0) = frustumWorldPoints[6].x;
		*((float*)&v_far_delta_xyz + 1) = frustumWorldPoints[6].y;
		*((float*)&v_far_delta_xyz + 2) = frustumWorldPoints[6].z;
		*((float*)&v_far_delta_xyz + 3) = 0.0f;
		v_far_delta_xyz = vec_sub(v_far_delta_xyz, uv_far_initial_xyz);
		
		*((float*)&uv_near_initial_xyz + 0) = frustumWorldPoints[0].x;
		*((float*)&uv_near_initial_xyz + 1) = frustumWorldPoints[0].y;
		*((float*)&uv_near_initial_xyz + 2) = frustumWorldPoints[0].z;
		*((float*)&uv_near_initial_xyz + 3) = 0.0f;
		
		*((float*)&u_near_delta_xyz + 0) = frustumWorldPoints[1].x;
		*((float*)&u_near_delta_xyz + 1) = frustumWorldPoints[1].y;
		*((float*)&u_near_delta_xyz + 2) = frustumWorldPoints[1].z;
		*((float*)&u_near_delta_xyz + 3) = 0.0f;
		u_near_delta_xyz = vec_sub(u_near_delta_xyz, uv_near_initial_xyz);
		
		*((float*)&v_near_delta_xyz + 0) = frustumWorldPoints[2].x;
		*((float*)&v_near_delta_xyz + 1) = frustumWorldPoints[2].y;
		*((float*)&v_near_delta_xyz + 2) = frustumWorldPoints[2].z;
		*((float*)&v_near_delta_xyz + 3) = 0.0f;
		v_near_delta_xyz = vec_sub(v_near_delta_xyz, uv_near_initial_xyz);
		
	// Begin traversing all the rays
		numRays = AKgUniqueFrames[gFrameNum % AKcNumTemperalFrames].numRays;
		curRay = AKgUniqueFrames[gFrameNum % AKcNumTemperalFrames].rays;
		
	#if BRENTS_CHEESY_PROFILE && !BRENTS_CHEESY_PROFILE_ONLY_FUNC
		funcNumRays += numRays;
		funcRayStart = UUrMachineTime_High();
	#endif
	
		for(itr = 0; itr < numRays; itr++, curRay++)
		{
			vector float	splat_u, splat_v;
			vector float	endPointXYZ;
			vector float	startPointXYZ;
			vector float	vXYZ;
			vector bool int	signVXYZ;
			vector float	splat_curFullDim;
			vector unsigned int splat_dim_Encode;
			vector unsigned int	sideCrossingXYZ;
			
			#if defined(UUmAltiVec_Trace) && UUmAltiVec_Trace
			
				if(itr == 4)
				{
					startTrace_alt("rayCastTrace", &pointArray);
				}
				else if(itr == 7)
				{
					stopTrace_alt(&pointArray);
					exit(0);
				}
			
			#endif

			*((float*)&splat_u) = curRay->u;
			*((float*)&splat_v) = curRay->v;
			
			splat_u = vec_splat(splat_u, 0);
			splat_v = vec_splat(splat_v, 0);
			
			endPointXYZ = vec_madd(splat_u, u_far_delta_xyz, uv_far_initial_xyz);
			endPointXYZ = vec_madd(splat_v, v_far_delta_xyz, endPointXYZ);
			
			startPointXYZ = vec_madd(splat_u, u_near_delta_xyz, uv_near_initial_xyz);
			startPointXYZ = vec_madd(splat_v, v_near_delta_xyz, startPointXYZ);
			
			*((float*)&startPointXYZ + 3) = 1.0f;
						
			vXYZ = vec_sub(endPointXYZ, startPointXYZ);
			
			vector_t0 = vec_re(vXYZ);
			vector_t1 = vec_nmsub(vector_t0, vXYZ, splat_one);
			invVXYZ = vec_madd(vector_t0, vector_t1, vector_t0);
			
			signVXYZ = vec_cmpgt(vXYZ, splat_zero);
			
			// set up the side crossing for x, y, and z
				sideCrossingXYZ = vec_and(signVXYZ, splat_int_one);
				sideCrossingXYZ = vec_add(sideCrossingXYZ, splat_neg_xyz);
					
			curNodeIndex = startNodeIndex;
			
			#if BRENTS_CHEESY_PROFILE && !BRENTS_CHEESY_PROFILE_ONLY_FUNC
				funcOctantStart = UUrMachineTime_High();
			#endif
			
			while(1)
			{
				UUmAssert(AKmOctTree_IsLeafNode(curNodeIndex));

				if(0xFFFFFFFF == curNodeIndex) break;
				
				UUmAssert(AKmOctTree_GetLeafIndex(curNodeIndex) < inEnvironment->octTree->leafNodeArray->numNodes);
				
				curOTLeafNode = leafNodeArray + AKmOctTree_GetLeafIndex(curNodeIndex);
				
				UUrBitVector_SetBit(
					environmentPrivate->otLeafNodeVisibility,
					AKmOctTree_GetLeafIndex(curNodeIndex));
				
				//
				*(UUtUns32*)&splat_dim_Encode = curOTLeafNode->dim_Encode;
				splat_dim_Encode = vec_splat(splat_dim_Encode, 0);
								
				curFullDim = (float)(1 << ((curOTLeafNode->dim_Encode >> AKcOctTree_Shift_Dim) & AKcOctTree_Mask_Dim));
				curFullDim *= AKcMinOTDim;
				*(float*)&splat_curFullDim = curFullDim;
				splat_curFullDim = vec_splat(splat_curFullDim, 0);
				
				int_t0 = vec_sr(splat_dim_Encode, shift_XYZ);
				int_t0 = vec_and(int_t0, mask_XYZ);
				
				maxXYZ = vec_ctf(int_t0, 0);
				
				maxXYZ = vec_sub(maxXYZ, splat_255);
				maxXYZ = vec_madd(maxXYZ, splat_minOTDim, splat_zero);
				minXYZ = vec_sub(maxXYZ, splat_curFullDim);
				
				*((float*)&minXYZ + 3) = -1e9;
				*((float*)&maxXYZ + 3) = 1e9;
				
				#if defined(DEBUG_COLLISION2_TRAVERSAL) && DEBUG_COLLISION2_TRAVERSAL
					
					bBox.minPoint.x = *((float*)&minXYZ + 0);
					bBox.minPoint.y = *((float*)&minXYZ + 1);
					bBox.minPoint.z = *((float*)&minXYZ + 2);
					bBox.maxPoint.x = *((float*)&maxXYZ + 0);
					bBox.maxPoint.y = *((float*)&maxXYZ + 1);
					bBox.maxPoint.z = *((float*)&maxXYZ + 2);
					
					in = CLrBox_Line(&bBox, (M3tPoint3D*)&startPointXYZ, (M3tPoint3D*)&endPointXYZ);
					
					UUmAssert(in);

				#endif
				
				stopChecking = UUcFalse;
				
				startIndirectIndex = curOTLeafNode->gqIndirectIndex_Encode;
				
				if(startIndirectIndex == 0)
				{
					#if BRENTS_CHEESY_PROFILE && !BRENTS_CHEESY_PROFILE_ONLY_FUNC
					
						funcNumSkippedOctants++;
						
					#endif
					
					goto skip;
				}
				
				length = (UUtUns8)(startIndirectIndex & 0xFF);
				startIndirectIndex >>= 8;
				
				#if BRENTS_CHEESY_PROFILE && !BRENTS_CHEESY_PROFILE_ONLY_FUNC
					numQuadChecks = 0;
					funcQuadTraversalStart = UUrMachineTime_High();
				#endif
				
				for(curGQIndIndex = 0;
					curGQIndIndex < length;
					curGQIndIndex++)
				{
					curGQIndex = octTreeGQIndices[curGQIndIndex + startIndirectIndex];
					
					UUmAssert(curGQIndex < inEnvironment->gqGeneralArray->numGQs);
					
					curGQGeneral = gqGeneralArray + curGQIndex;
					curGQCollision = gqCollisionArray + curGQIndex;
					
					gotPlaneEqu = UUcFalse;
					
					//Mark this gq as visible
					if(UUrBitVector_TestAndSetBit(gqComputedBackFaceBV, curGQIndex))
					{
						if(UUrBitVector_TestBit(gqBackFaceBV, curGQIndex))
						{
							continue;
						}
					}
					else
					{
						// test for backfacing
	 					if(!(curGQGeneral->flags & AKcGQ_Flag_2Sided))
	 					{
	 						vector float curPointXYZ;
	 						
	 						gotPlaneEqu = UUcTrue;
	 						
	 						#if 0
	 						// Remove backfacing stuff
	 						AKmPlaneEqu_GetComponents(
	 							curGQCollision->planeEquIndex,
								planeEquArray,
								a, b, c, d);
							#endif
							
							planeEqu = planeEquArray[AKmPlaneEqu_GetIndex(curGQCollision->planeEquIndex)];
							if(AKmPlaneEqu_IsNegative(curGQCollision->planeEquIndex))
							{
								vector unsigned long signBit = vec_sl((vector unsigned long)(0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF), (vector unsigned long)(0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF));
								planeEqu = (vector float) vec_xor((vector unsigned long) planeEqu, signBit);
							}
							
							curPoint = pointArray + curGQGeneral->m3Quad.vertexIndices.indices[0];
							
							AVmLoadMisalignedVectorPoint(curPoint, curPointXYZ);

							viewVectorXYZ = vec_sub(startPointXYZ, curPointXYZ);
	 						vector_t0 = vec_madd(viewVectorXYZ, viewVectorXYZ, splat_zero);
	 						vector_t1 = vec_splat(vector_t0, 1);
	 						vector_t2 = vec_splat(vector_t0, 2);
	 						
	 						vector_t0 = vec_add(vector_t0, vector_t1);
	 						vector_t0 = vec_add(vector_t0, vector_t2);
	 						
	 						// compute one over sqrt
							#if defined(UUmRayCast_AltiVec_Refine) && UUmRayCast_AltiVec_Refine
	 						{
	 							vector float	y0, t0, t1;
	 							
		 						y0 = vec_rsqrte(vector_t0);
		 						t0 = vec_madd(y0, y0, splat_zero);
		 						t1 = vec_madd(y0, splat_onehalf, splat_zero);
		 						t0 = vec_nmsub(vector_t0, t0, splat_one);
		 						vector_t0 = vec_madd(t0, t1, y0);
							}
							#else
		 						vector_t0 = vec_rsqrte(vector_t0);
							#endif
													
	 						vector_t0 = vec_splat(vector_t0, 0);
	 						
	 						viewVectorXYZ = vec_madd(viewVectorXYZ, vector_t0, splat_zero);
	 						
	 						viewVectorXYZ = vec_madd(viewVectorXYZ, planeEqu, splat_zero);
	 						vector_t0 = vec_splat(viewVectorXYZ, 1);
	 						vector_t1 = vec_splat(viewVectorXYZ, 2);
	 						viewVectorXYZ = vec_add(viewVectorXYZ, vector_t0);
	 						viewVectorXYZ = vec_add(viewVectorXYZ, vector_t1);
	 						viewVectorXYZ = vec_splat(viewVectorXYZ, 0);
	 						
	 						if(vec_any_lt(viewVectorXYZ, splat_zero))
	 						{
	 							UUrBitVector_SetBit(gqBackFaceBV, curGQIndex);
	 							continue;
							}
	 					}
					}
					
					{
						UUtUns8*	p = (UUtUns8 *)environmentPrivate->gqVisibilityVector + (curGQIndex >> 2);
						*p |= 0x3 << ((curGQIndex & 0x3) << 1);
					}
					
 					if(curGQGeneral->flags & (AKcGQ_Flag_VisIgnore | AKcGQ_Flag_Jello | AKcGQ_Flag_Transparent)) continue;
					
					if(!stopChecking)
					{
						
						#if BRENTS_CHEESY_PROFILE && !BRENTS_CHEESY_PROFILE_ONLY_FUNC
							
							numQuadChecks++;
							
						#endif
						
						if(!gotPlaneEqu)
						{
							planeEqu = planeEquArray[AKmPlaneEqu_GetIndex(curGQCollision->planeEquIndex)];
							if(AKmPlaneEqu_IsNegative(curGQCollision->planeEquIndex))
							{
								vector unsigned long signBit = vec_sl((vector unsigned long)(0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF), (vector unsigned long)(0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF));
								planeEqu = (vector float) vec_xor((vector unsigned long) planeEqu, signBit);
							}
						}

						denom = vec_madd(vXYZ, planeEqu, splat_zero);
						vector_t0 = vec_splat(denom, 1);
						vector_t1 = vec_splat(denom, 2);
						denom = vec_add(denom, vector_t0);
						denom = vec_add(denom, vector_t1);
						denom = vec_splat(denom, 0);

						
						// compute one over denom
						vector_t0 = vec_re(denom);
						vector_t1 = vec_nmsub(vector_t0, denom, splat_one);
						denom = vec_madd(vector_t0, vector_t1, vector_t0);
						
						num1 = vec_madd(startPointXYZ, planeEqu, splat_zero);
						vector_t0 = (vector float)vec_sld((vector unsigned char)num1, (vector unsigned char)num1, 4);
						num1 = vec_add(vector_t0, num1);
						vector_t0 = (vector float)vec_sld((vector unsigned char)num1, (vector unsigned char)num1, 8);
						num1 = vec_add(vector_t0, num1);

						num1 = vec_nmsub(num1, denom, splat_zero);
						
						testPointXYZ = vec_madd(vXYZ, num1, startPointXYZ);

						if(vec_any_lt(num1, splat_zero)) continue;
						if(vec_any_gt(num1, splat_one)) continue;
						
						if(vec_any_lt(testPointXYZ, minXYZ)) continue;
						if(vec_any_gt(testPointXYZ, maxXYZ)) continue;
						
						if (AKiQuad_PointInQuad(
							(CLtQuadProjection)curGQCollision->projection,
							pointArray,
							&curGQGeneral->m3Quad.vertexIndices,
							testPointXYZ))
						{
							stopChecking = UUcTrue;
						
							// swap
							if(curGQIndIndex != 0)
							{
								octTreeGQIndices[curGQIndIndex + startIndirectIndex] = octTreeGQIndices[startIndirectIndex];
								octTreeGQIndices[startIndirectIndex] = curGQIndex;
							}

							#if BRENTS_CHEESY_PROFILE && !BRENTS_CHEESY_PROFILE_ONLY_FUNC
								funcHitOctant_NumQuadsTillHit += curGQIndIndex + 1;
							#endif
						}
					}
				}
				
				#if BRENTS_CHEESY_PROFILE && !BRENTS_CHEESY_PROFILE_ONLY_FUNC
				
					if(stopChecking)
					{
						funcHitOctant_QuadTraversalTime += UUrMachineTime_High() - funcQuadTraversalStart;
						funcHitOctant_NumQuadTraversals += length;
						funcHitOctant_NumQuadChecks += numQuadChecks;
						funcNumHitOctants++;
					}
					else
					{
						funcMissOctant_QuadTraversalTime += UUrMachineTime_High() - funcQuadTraversalStart;
						funcMissOctant_NumQuadTraversals += length;
						funcMissOctant_NumQuadChecks += numQuadChecks;
						funcNumMissOctants++;
					}
				#endif
				
skip:
				
				#if 0
				{
					M3tPoint3D	minPoint;
					M3tPoint3D	maxPoint;
					
					minPoint.x = minX + 0.1f;
					minPoint.y = minY + 0.1f;
					minPoint.z = minZ + 0.1f;
					maxPoint.x = maxX - 0.1f;
					maxPoint.y = maxY - 0.1f;
					maxPoint.z = maxZ - 0.1f;
					
					if(/*endNodeIndex == curNodeIndex ||*/ stopChecking)
					{
						AKiBoundingBox_Draw(
											&minPoint,
							&maxPoint,
							0x1F << 10);
					}
					else
					{
						AKiBoundingBox_Draw(
											&minPoint,
							&maxPoint,
							0xFFFF);
					}
				}
				#endif
				
				// If we are finished then bail
				if(stopChecking) break;

				// Find out which plane I cross
				{
					vector float	planeXYZ;
					vector float	secondaryCompare;
					vector float	primaryCompare;
					vector float	splat_d;
					
					planeXYZ = vec_sel(minXYZ, maxXYZ, signVXYZ);
					
					planeMinusStartXYZ = vec_sub(planeXYZ, startPointXYZ);
					
					// next set up the secondary compares, YZ and XZ
					{
						
						vector float	vTemp;
						vector float	pmsTemp;
						
						vTemp = vec_perm(vXYZ, vXYZ, vPerm);
						pmsTemp = vec_perm(planeMinusStartXYZ, planeMinusStartXYZ, pmsPerm);
						
						secondaryCompare = vec_madd(vTemp, pmsTemp, splat_zero);
						secondaryCompare = vec_abs(secondaryCompare);
						
					}
					
					// Now set up the primary compare
					{
						vector float primTemp;
						
						primTemp = vec_perm(vXYZ, vXYZ, primPerm);
						
						primaryCompare = vec_madd(primTemp, planeMinusStartXYZ, splat_zero);
						primaryCompare = vec_abs(primaryCompare);
						
					}
					
					splat_d = vec_madd(invVXYZ, planeMinusStartXYZ, splat_zero);
					
					vector_t0 = vec_splat(primaryCompare, 0);
					vector_t1 = vec_splat(primaryCompare, 1);
					
					// make the decisions
						if(vec_any_gt(vector_t0, vector_t1))
						{
							vector_t0 = vec_splat(secondaryCompare, 0);
							vector_t1 = vec_splat(secondaryCompare, 1);
							
							if(vec_any_gt(vector_t0, vector_t1))
							{
								sideCrossing = *((UUtUns32*)&sideCrossingXYZ + 2);
								splat_d = vec_splat(splat_d, 2);
							}
							else
							{
								sideCrossing = *((UUtUns32*)&sideCrossingXYZ + 1);
								splat_d = vec_splat(splat_d, 1);
							}
						}
						else
						{
							vector_t0 = vec_splat(secondaryCompare, 2);
							vector_t1 = vec_splat(secondaryCompare, 3);
							if(vec_any_gt(vector_t0, vector_t1))
							{
								sideCrossing = *((UUtUns32*)&sideCrossingXYZ + 2);
								splat_d = vec_splat(splat_d, 2);
							}
							else
							{
								sideCrossing = *((UUtUns32*)&sideCrossingXYZ + 0);
								splat_d = vec_splat(splat_d, 0);
							}
						}
					
					// Lets move to the next node
						curNodeIndex = curOTLeafNode->adjInfo[sideCrossing];
						
						if(!AKmOctTree_IsLeafNode(curNodeIndex))
						{
							vector float	splat_curHalfDim;
							vector float	curCenterPointXYZ;
							vector float	projCenterXYZ;
							vector float	curCenterPointU, curCenterPointV;
							vector float	uc, vc;
							
							// need to traverse quad tree
							
							splat_curHalfDim = vec_madd(splat_curFullDim, splat_onehalf, splat_zero);
							
							curCenterPointXYZ = vec_add(minXYZ, splat_curHalfDim);
							
							projCenterXYZ = vec_madd(vXYZ, splat_d, startPointXYZ);
							
							// First compute the intersection point
							switch(sideCrossing)
							{
								case AKcOctTree_Side_NegX:
								case AKcOctTree_Side_PosX:
									curCenterPointU = vec_splat(curCenterPointXYZ, 1);
									curCenterPointV = vec_splat(curCenterPointXYZ, 2);
									uc = vec_splat(projCenterXYZ, 1);
									vc = vec_splat(projCenterXYZ, 2);
									break;
									
								case AKcOctTree_Side_NegY:
								case AKcOctTree_Side_PosY:
									curCenterPointU = vec_splat(curCenterPointXYZ, 0);
									curCenterPointV = vec_splat(curCenterPointXYZ, 2);
									uc = vec_splat(projCenterXYZ, 0);
									vc = vec_splat(projCenterXYZ, 2);
									break;
									
								case AKcOctTree_Side_NegZ:
								case AKcOctTree_Side_PosZ:
									curCenterPointU = vec_splat(curCenterPointXYZ, 0);
									curCenterPointV = vec_splat(curCenterPointXYZ, 1);
									uc = vec_splat(projCenterXYZ, 0);
									vc = vec_splat(projCenterXYZ, 1);
									break;
								
								default:
									UUmAssert(0);
							}
							
							// next traverse the tree
							while(1)
							{
								curQTIntNode = qtInteriorNodeArray + curNodeIndex;
								
								curOctant = 0;
								if(vec_any_gt(uc, curCenterPointU)) curOctant |= AKcQuadTree_SideBV_U;
								if(vec_any_gt(vc, curCenterPointV)) curOctant |= AKcQuadTree_SideBV_V;
								
								curNodeIndex = curQTIntNode->children[curOctant];
								
								if(AKmOctTree_IsLeafNode(curNodeIndex)) break;
								
								splat_curHalfDim = vec_madd(splat_curFullDim, splat_onehalf, splat_zero);
								
								if(curOctant & AKcQuadTree_SideBV_U) curCenterPointU = vec_add(curCenterPointU, splat_curHalfDim);
								else curCenterPointU = vec_sub(curCenterPointU, splat_curHalfDim);
							
								if(curOctant & AKcQuadTree_SideBV_V) curCenterPointV = vec_add(curCenterPointV, splat_curHalfDim);
								else curCenterPointV = vec_sub(curCenterPointV, splat_curHalfDim);
							}
						}
					}
			}
			
			#if BRENTS_CHEESY_PROFILE && !BRENTS_CHEESY_PROFILE_ONLY_FUNC
				funcOctantTime += UUrMachineTime_High() - funcOctantStart;
			#endif
			
			if(AKgDebug_ShowRays)
			{
				M3tPoint3D	points[2];
				UUtUns16	colorLine;
				
				points[0] = *(M3tPoint3D*)&startPointXYZ;
				
				points[0].x += 0.01f;
				points[0].y += 0.01f;
				points[0].z += 0.01f;
				
				points[1] = *(M3tPoint3D*)&testPointXYZ;
				
				switch(gFrameNum % AKcNumTemperalFrames)
				{
					case 0:
						colorLine = 0x1f << 10;
						break;
					case 1:
						colorLine = 0x1f << 5;
						break;
					case 2:
						colorLine = 0x1f << 0;
						break;
						
				}
				
				M3rGeom_Line_Light(
					&points[0],
					&points[1],
					0xFFFF);
			}
		}
	
	#if BRENTS_CHEESY_PROFILE
		funcEnd = UUrMachineTime_High();
		
		#if !BRENTS_CHEESY_PROFILE_ONLY_FUNC
			funcRayTime += funcEnd - funcRayStart;
		#endif
		
		funcTime += funcEnd - funcStart;
		
	#endif
#endif
}

#endif /* AltiVec*/

#endif // S.S.
