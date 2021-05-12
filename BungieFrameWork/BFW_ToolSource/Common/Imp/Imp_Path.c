/*
	Imp_Path.c
	
	This file contains all importer pathfinding related code
	
	Author: Quinn Dunki
	c1998 Bungie
*/
#include <stdlib.h>

#include "BFW.h"
#include "BFW_Akira.h"
#include "BFW_TM_Construction.h"
#include "BFW_Path.h"
#include "BFW_MathLib.h"
#include "BFW_Shared_TriRaster.h"
#include "BFW_Math.h"

#include "Imp_Env2_Private.h"
#include "Imp_Path.h"
#include "Imp_Common.h"

extern UUtBool gDrawGrid;

#define DEBUG_GRID				0
//#define DEBUG_DISABLE_WALLS
//#define DEBUG_DISABLE_SATS
//#define DEBUG_NODISCARD
//#define DEBUG_DISABLE_VERTICAL
//#define DEBUG_DISABLE_SLOPING

#if DEBUG_GRID
// TEMPORARY DEBUGGING CODE
static UUtBool IMPgTempDebugGrid;
#endif


#define IMPcQuadInBNVTolerance				10.0f
#define IMPcWalkUnderHeight					20.0f
#define IMPcOverhangIsStairTolerance		2.0f
#define IMPcGridBuffer						2			// number of squares around BNV to create grid for
#define IMPcGridBufferTemp					3			// number of squares to grow and then shrink
														// grid (in addition to IMPcGridBuffer)
#define IMPcSAT_InsideBNVTolerance			2.0f
#define IMPcPathDebugInfoMaxSize			(256 * 1024)
#define IMPcValidFloorMinimumCeilingSpace	6.0f
#define IMPcValidFloorMinimumBNVBase		6.0f
#define IMPcValidFloorMinimumUnderBNV		6.0f
#define IMPcStairOutletWidth				15.0f

static void
PHiRasterizeTriHeight(
	PHtGridPoint*	inVertex0,
	PHtGridPoint*	inVertex1,
	PHtGridPoint*	inVertex2,
	PHtSquare*		inMapMemory,
	UUtUns16		inRowBytes,
	UUtInt16		inWidth,
	UUtInt16		inHeight);

static void PHiFindSATs(AKtEnvironment *inEnvironment, IMPtEnv_BuildData *inBuildData, IMPtEnv_BNV *inBNV,
						AKtBNVNode *inNode);
static void PHrMarkBorderValues(UUtUns8 inBaseValue, UUtUns8 inSeedValue, UUtUns8 inMarkValue);
static UUtBool PHiQuadInBNV(IMPtEnv_BuildData *inBuildData, IMPtEnv_BNV *inBNV, UUtUns32 inGQIndex);

PHtSquare PHgTempGrid[PHcMaxGridSize * PHcMaxGridSize];
PHtSquare PHgCompressedGrid[PHcMaxGridSize * PHcMaxGridSize];
UUtUns32 PHgCompressedGridSize;
UUtInt32 PHgTempGridX, PHgTempGridOffsetX;
UUtInt32 PHgTempGridY, PHgTempGridOffsetY;

UUtUns32 PHgOriginalMemoryUsage = 0;
UUtUns32 PHgCompressedMemoryUsage = 0;

UUtUns32 PHgTempDebugInfoMaxCount, PHgTempDebugInfoCount;
UUtBool PHgTempDebugInfoOverflowed;
PHtDebugInfo *PHgTempDebugInfo = NULL;
UUtUns32 PHgCurrentDebugEvent, PHgCurrentDebugGQ;

#define IMPmGetGridPoint(gridpt, worldpt, room) {                                                                           \
	(gridpt).x = PHgTempGridOffsetX + (UUtInt16) MUrFloat_Round_To_Int(((worldpt).x - (room)->origin.x) / PHcSquareSize);   \
	(gridpt).y = PHgTempGridOffsetY + (UUtInt16) MUrFloat_Round_To_Int(((worldpt).z - (room)->origin.z) / PHcSquareSize);   \
}

#define IMPmAddDebugInfo(type, px, py, gq, added_weight) {																	\
	if (IMPgBuildPathDebugInfo) {																							\
		if (PHgTempDebugInfoCount < PHgTempDebugInfoMaxCount) {																\
			PHtDebugInfo *debug_info = &PHgTempDebugInfo[PHgTempDebugInfoCount++];											\
																															\
			debug_info->event = (UUtUns8) type;																				\
			debug_info->x = (UUtInt16) (px - IMPcGridBufferTemp);															\
			debug_info->y = (UUtInt16) (py - IMPcGridBufferTemp);															\
			debug_info->gq_index = gq;																						\
			debug_info->weight = added_weight;																				\
		} else if (!PHgTempDebugInfoOverflowed) {																			\
			Imp_PrintWarning("IMPmAddDebugInfo: overflowed max debug info size %d", IMPcPathDebugInfoMaxSize);				\
			PHgTempDebugInfoOverflowed = UUcTrue;																			\
		}																													\
	}																														\
}

static void PHrDrawGrid(void)
{
	static UUtBool dont_print = UUcFalse;
	UUtInt32 x,y;
	char buf[512];

	if (dont_print) {
		return;
	}

	for(y = 0; y < PHgTempGridY; y++)
	{
		PHtSquare *scan_line = PHgTempGrid + (y * PHgTempGridX);

		for(x = 0; x < PHgTempGridX; x++)
		{
			buf[x] = PHcPrintChar[scan_line[x].weight];
		}

		buf[x++] = '\n';
		buf[x]   = '\0';
		Imp_PrintMessage(IMPcMsg_Important, buf);
	}

	Imp_PrintMessage(IMPcMsg_Important, "\n");
}

static void PHrRasterizeTriangle_Internal(UUtInt32 y0, UUtInt32 y1, float x_left0, float x_right0, float x_left1, float x_right1, UUtUns8 inColor, UUtBool inCheckPrecedence)
{
	UUtInt32 y;
	float y_distance = (float) (y1 - y0);
	float x_left = x_left0;
	float x_right = x_right0;
	float dx_left = (x_left1 - x_left0) / y_distance;
	float dx_right = (x_right1 - x_right0) / y_distance;
	PHtSquare *scan_line = PHgTempGrid + (y0 * PHgTempGridX);

	for(y = y0; y <= y1; y++)
	{
		UUtInt32 x_integer_left = MUrFloat_Round_To_Int(x_left);
		UUtInt32 x_integer_right = MUrFloat_Round_To_Int(x_right);
		UUtInt32 x;

		if ((y >= 0) && (y < PHgTempGridY)) {
			UUtInt32 x_integer_left_clipped = UUmMax(x_integer_left, 0);
			UUtInt32 x_integer_right_clipped = UUmMin(x_integer_right, PHgTempGridX - 1);

/*			if (IMPgTempDebugGrid && (x_integer_right >= PHgTempGridX)) {
				Imp_PrintMessage(IMPcMsg_Important, "TriInt hit right edge - y0:%d y1:%d x0(%f-%f) x1(%f-%f)\n",
									y0, y1, x_left0, x_right0, x_left1, x_right1);
			}

			if (IMPgTempDebugGrid && (x_integer_left < 0)) {
				Imp_PrintMessage(IMPcMsg_Important, "TriInt hit left edge - y0:%d y1:%d x0(%f-%f) x1(%f-%f)\n",
									y0, y1, x_left0, x_right0, x_left1, x_right1);
			}*/

			for(x = x_integer_left_clipped; x <= x_integer_right_clipped; x++)
			{
				if (inCheckPrecedence && (scan_line[x].weight >= inColor)) {
					// this square is already filled with a higher weight
				} else {
					scan_line[x].weight = inColor;
					IMPmAddDebugInfo(PHgCurrentDebugEvent, x, y, PHgCurrentDebugGQ, inColor);
				}
			}
		}

		x_left += dx_left;
		x_right += dx_right;
		scan_line += PHgTempGridX;
	}

	return;
}

static void PHrRasterizeTriangle(IMtPoint2D *inPoint1, IMtPoint2D *inPoint2, IMtPoint2D *inPoint3, UUtUns8 inColor, UUtBool inCheckPrecedence)
{
	IMtPoint2D *top;
	IMtPoint2D *mid;
	IMtPoint2D *bottom;

	if ((inPoint1->y <= inPoint2->y) && (inPoint1->y <= inPoint3->y)) {
		top = inPoint1;

		if (inPoint2->y >= inPoint3->y) {
			bottom = inPoint2;
			mid = inPoint3;
		}
		else {
			bottom = inPoint3;
			mid = inPoint2;
		}
	}
	else if (inPoint2->y <= inPoint3->y) {
		top = inPoint2;

		if (inPoint1->y >= inPoint3->y) {
			bottom = inPoint1;
			mid = inPoint3;
		}
		else {
			bottom = inPoint3;
			mid = inPoint1;
		}
	}
	else {
		top = inPoint3;
		if (inPoint1->y >= inPoint2->y) {
			bottom = inPoint1;
			mid = inPoint2;
		}
		else {
			bottom = inPoint2;
			mid = inPoint1;
		}
	}

	// ok we split into two triangles
	//
	// triangle1:
	//
	//     .  <--- top
	//    . .
	//   .  .
	//  .    .
	// .     .  <----- mid (may be on the left or right)
	//  ...   .
	//    ......   <---- bottom

	// each triangle needs the following variables
	// y0
	// y1
	// x_left0, x_right0
	// x_left1, x_right1

	{
		UUtUns16 mid_total_distance = bottom->y - top->y;
		UUtUns16 mid_distance = mid->y - top->y;
		float interpolated_x;
		float left_mid_x;
		float right_mid_x;
		UUtInt32 x, x_min, x_max;
		PHtSquare *scan_line;

		if (mid_total_distance == 0) {
			// CB: just draw a single scanline (don't divide by zero)
			if ((top->y < 0) || (top->y >= PHgTempGridY))
				return;

			scan_line = PHgTempGrid + (top->y * PHgTempGridX);

			x_min = top->x;						x_max = top->x;
			x_min = UUmMin(x_min, mid->x);		x_max = UUmMax(x_max, mid->x);
			x_min = UUmMin(x_min, bottom->x);	x_max = UUmMax(x_max, bottom->x);
			x_min = UUmMax(x_min, 0);			x_max = UUmMin(x_max, PHgTempGridX - 1);

			for(x = x_min; x <= x_max; x++)
			{
				if (inCheckPrecedence && (scan_line[x].weight >= inColor)) {
					// this square is already filled with a higher weight
				} else {
					scan_line[x].weight = inColor;
					IMPmAddDebugInfo(PHgCurrentDebugEvent, x, top->y, PHgCurrentDebugGQ, inColor);
				}
			}
			return;
		}

		interpolated_x = ((top->x * (mid_total_distance - mid_distance)) + (mid_distance * bottom->x)) / ((float) mid_total_distance);

		if (interpolated_x < mid->x) {
			left_mid_x = interpolated_x;
			right_mid_x = mid->x;
		}
		else {
			left_mid_x = mid->x;
			right_mid_x = interpolated_x;
		}

		PHrRasterizeTriangle_Internal(top->y, mid->y, top->x, top->x, left_mid_x, right_mid_x, inColor, inCheckPrecedence);
		PHrRasterizeTriangle_Internal(mid->y, bottom->y, left_mid_x, right_mid_x, bottom->x, bottom->x, inColor, inCheckPrecedence);
	}


	return;
}

static UUtBool PHiQuadInBNV(IMPtEnv_BuildData *inBuildData, IMPtEnv_BNV *inBNV, UUtUns32 inGQIndex)
{
	M3tPoint3D *sharedPoints = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);
	M3tQuad	*sharedQuads = AUrSharedQuadArray_GetList(inBuildData->sharedBNVQuadArray);
	M3tPlaneEquation *sharedPlanes = AUrSharedPlaneEquArray_GetList(inBuildData->sharedPlaneEquArray);
	IMPtEnv_GQ *curGQ = inBuildData->gqList + inGQIndex;

	UUtUns32 i, j, k, outside[6];
	UUtBool inside;
	IMPtEnv_BNV_Side *side;
	M3tQuad *side_quad;
	M3tPoint3D points[4], intersect_point, side_quad_points[4];
	M3tPlaneEquation side_plane, quad_plane;
	float d[4], d0, d1, d_sum;

	for (i = 0; i < 6; i++) {
		outside[i] = 0;
	}

	// trivially accept quads which have any points inside the BNV
	for (i = 0; i < 4; i++) {
		points[i] = sharedPoints[curGQ->visibleQuad.indices[i]];
		
		if (IMPiEnv_Process_BSP_PointInBSP(inBuildData, inBNV, &points[i], IMPcQuadInBNVTolerance)) {
#if DEBUG_GRID
			if (IMPgTempDebugGrid) {
				Imp_PrintMessage(IMPcMsg_Important, "keeping quad %d, vertex %d at (%f %f %f) is inside BNV\n",
								inGQIndex, i, points[i].x, points[i].y, points[i].z);
			}
#endif
			inside = UUcTrue;
			goto exit;
		}

		if (points[i].x < inBNV->origin.x - IMPcQuadInBNVTolerance) {
			outside[0]++;
		} else if (points[i].x > inBNV->antiOrigin.x + IMPcQuadInBNVTolerance) {
			outside[1]++;
		}

		if (points[i].y < inBNV->origin.y - IMPcQuadInBNVTolerance) {
			outside[2]++;
		} else if (points[i].y > inBNV->antiOrigin.y + IMPcQuadInBNVTolerance) {
			outside[3]++;
		}

		if (points[i].z < inBNV->origin.z - IMPcQuadInBNVTolerance) {
			outside[4]++;
		} else if (points[i].z > inBNV->antiOrigin.z + IMPcQuadInBNVTolerance) {
			outside[5]++;
		}
	}

	// trivially reject quads which lie outside the BNV's bounding box
	for (i = 0; i < 6; i++) {
		if (outside[i] == 4) {
			inside = UUcFalse;
			goto exit;
		}
	}

	AKmPlaneEqu_GetComponents(curGQ->planeEquIndex, sharedPlanes, quad_plane.a, quad_plane.b, quad_plane.c, quad_plane.d);

	// loop over all sides of the BNV
	for (i = 0, side = inBNV->sides; i < inBNV->numSides; i++, side++) {
		// check to see if any of the quad's edges intersect this face
		AKmPlaneEqu_GetComponents(side->planeEquIndex, sharedPlanes, side_plane.a, side_plane.b, side_plane.c, side_plane.d);
		for (j = 0; j < 4; j++) {
			d[j] = points[j].x * side_plane.a + points[j].y * side_plane.b + points[j].z * side_plane.c + side_plane.d;
		}

		for (j = 0; j < 4; j++) {
			// check the points that make up this edge against the side's plane
			d0 = d[j]; d1 = d[(j + 1) % 4];

			if (d0 * d1 > 0) {
				continue;
			}

			// calculate the point where the edge intersects the side
			d_sum = d1 - d0;
			if ((float)fabs(d_sum) < 1e-03f) {
				MUmVector_Copy(intersect_point, points[j]);
			} else {
				MUmVector_ScaleCopy(intersect_point, d1 / (d1 - d0), points[j]);
				MUmVector_ScaleIncrement(intersect_point, -d0 / (d1 - d0), points[(j + 1) % 4]);
			}
			UUmAssert(UUmFloat_LooseCompareZero(intersect_point.x * side_plane.a + intersect_point.y * side_plane.b + 
												intersect_point.z * side_plane.c + side_plane.d));
			UUmAssert(UUmFloat_LooseCompareZero(intersect_point.x * quad_plane.a + intersect_point.y * quad_plane.b + 
												intersect_point.z * quad_plane.c + quad_plane.d));

			for (k = 0; k < side->numBNVQuads; k++) {
				if (CLrQuad_PointInQuad(CLcProjection_Unknown, sharedPoints,
										&sharedQuads[side->bnvQuadList[k]], &intersect_point)) {
					// the edge intersects the face of the BNV, we must keep this quad
					inside = UUcTrue;
					goto exit;
				}
			}
		}

		// check to see if any of the edges of the quads which make up this side intersect the quad being considered
		for (j = 0; j < side->numBNVQuads; j++) {
			side_quad = &sharedQuads[side->bnvQuadList[j]];

			// calculate the side quad's points and their location relative to the considered quad's plane
			for (k = 0; k < 4; k++) {
				side_quad_points[k] = sharedPoints[side_quad->indices[k]];
				d[k] = side_quad_points[k].x * quad_plane.a + side_quad_points[k].y * quad_plane.b + side_quad_points[k].z * quad_plane.c + quad_plane.d;

				UUmAssert(UUmFloat_LooseCompareZero(side_quad_points[k].x * side_plane.a + side_quad_points[k].y * side_plane.b + 
													side_quad_points[k].z * side_plane.c + side_plane.d));
			}
			
			for (k = 0; k < 4; k++) {
				// check the points that make up this edge against the side's plane
				d0 = d[k]; d1 = d[(k + 1) % 4];

				if (d0 * d1 > 0) {
					continue;
				}

				// calculate the point where the edge intersects the side
				d_sum = d1 - d0;
				if ((float)fabs(d_sum) < 1e-03f) {
					MUmVector_Copy(intersect_point, side_quad_points[k]);
				} else {
					MUmVector_ScaleCopy(intersect_point, d1 / (d1 - d0), side_quad_points[k]);
					MUmVector_ScaleIncrement(intersect_point, -d0 / (d1 - d0), side_quad_points[(k + 1) % 4]);
				}
				UUmAssert(UUmFloat_LooseCompareZero(intersect_point.x * quad_plane.a + intersect_point.y * quad_plane.b + 
													intersect_point.z * quad_plane.c + quad_plane.d));
				UUmAssert(UUmFloat_LooseCompareZero(intersect_point.x * side_plane.a + intersect_point.y * side_plane.b + 
													intersect_point.z * side_plane.c + side_plane.d));

				if (CLrQuad_PointInQuad(CLcProjection_Unknown, sharedPoints, &curGQ->visibleQuad, &intersect_point)) {
					// the edge of the BNV intersects the considered quad, we must keep it
					inside = UUcTrue;
					goto exit;
				}
			}
		}
	}

	// the quad does not intersect the BNV
	inside = UUcFalse;

exit:
#if DEBUG_GRID
	if (IMPgTempDebugGrid && !inside) {
		float min_y, max_y;

		min_y = UUmMin4(points[0].y, points[1].y, points[2].y, points[3].y);
		max_y = UUmMax4(points[0].y, points[1].y, points[2].y, points[3].y);

		if (max_y - min_y < 1.0f) {
			// floor or ceiling quad is being rejected
			Imp_PrintMessage(IMPcMsg_Important, "rejecting floor/ceiling quad #%d at y range (%f - %f)\n", inGQIndex, min_y, max_y);
		} else {
			Imp_PrintMessage(IMPcMsg_Important, "rejecting wall quad #%d, y range (%f - %f)\n", inGQIndex, min_y, max_y);
		}
	}
#endif

	return inside;
}

// determine whether a quad lies completely below some stairs and so
// can be safely ignored
static UUtBool PHiQuadBelowStairs(
	AKtEnvironment *inEnv,
	PHtRoomData *inRoom,
	IMPtEnv_GQ *inQuad,
	M3tPoint3D *inPoints,
	M3tPlaneEquation *inStairPlane)
{
	UUtUns32 i;
	float d;
	M3tPoint3D *p;
	
	for (i=0; i<4; i++)
	{
		p = inPoints + inQuad->visibleQuad.indices[i];

		d = p->x * inStairPlane->a + p->y * inStairPlane->b + p->z * inStairPlane->c + inStairPlane->d;

		if (UUmFloat_CompareGT(d, -4.0f))
			return UUcFalse;
	}

	return UUcTrue;
}

static void PHrRasterizeFloorQuad(IMPtEnv_BuildData *inBuildData, UUtUns32 inGQIndex, AKtEnvironment *environment, PHtRoomData *room,
								  UUtBool inForceWeight, UUtUns8 inWeight, M3tPlaneEquation *inStairPlane)
{
	AKtGQ_General *gqGeneral= environment->gqGeneralArray->gqGeneral + inGQIndex;
	IMPtEnv_GQ *gunk = inBuildData->gqList + inGQIndex;
	UUtUns8 draw_color;
	M3tPoint3D src_point;
	IMtPoint2D grid_points[4];
	UUtUns32 pointItr;
	float lowest_height, highest_height, maximum_valid_height;

	/*
	CB: this is irrelevant now, stair BNVs are not rasterized as normal

	if ((inStairPlane != NULL) && (PHiQuadBelowStairs(environment, room, gunk,
										environment->pointArray->points, inStairPlane))) {
		// don't draw quads that lie below the plane of the stairs
		return;
	}
	*/

	if (inForceWeight) {
		// force this quad to be drawn in the given weight
		draw_color = inWeight;
	} else {
		M3tPlaneEquation *planes = environment->planeArray->planes;
		M3tPlaneEquation plane;

		// if this quad is not a floor, bail out
		AKmPlaneEqu_GetComponents(gunk->planeEquIndex, planes, plane.a, plane.b, plane.c, plane.d);

		if (plane.b < PHcFlatNormal) {
#if DEBUG_GRID
			if (IMPgTempDebugGrid) {
				Imp_PrintMessage(IMPcMsg_Important, "  quad %d is not in fact floor (b component %f < floor factor %f)\n",
									inGQIndex, plane.b, PHcFlatNormal);
			}
#endif
			goto exit;
		}

		draw_color = PHcClear;
	}

	// rasterize it into the grid
	lowest_height = 1e+09f;
	highest_height = -1e+09f;
	for(pointItr = 0; pointItr < 4; pointItr++)
	{
		src_point = environment->pointArray->points[gunk->visibleQuad.indices[pointItr]];
		lowest_height = UUmMin(lowest_height, src_point.y);
		highest_height = UUmMax(highest_height, src_point.y);

		IMPmGetGridPoint(grid_points[pointItr], src_point, room)
	}

	// discard quads that lie too close to the top of the BNV - but don't do so if they are
	// close to the bottom of the BNV as well
	maximum_valid_height = room->antiOrigin.y - IMPcValidFloorMinimumCeilingSpace;
	maximum_valid_height = UUmMax(maximum_valid_height, room->origin.y + IMPcValidFloorMinimumBNVBase);

	if (lowest_height > maximum_valid_height) {
#if DEBUG_GRID
		if (IMPgTempDebugGrid) {
			Imp_PrintMessage(IMPcMsg_Important, "  floor quad %d is too close to top of BNV (lowest %f > maximumheight %f [BNV %f-%f])\n",
				inGQIndex, lowest_height, maximum_valid_height, room->origin.y, room->antiOrigin.y);
		}
#endif
		goto exit;
	}

	// discard quads that lie under the floor of the BNV
	if (highest_height < room->origin.y - IMPcValidFloorMinimumUnderBNV) {
#if DEBUG_GRID
		if (IMPgTempDebugGrid) {
			Imp_PrintMessage(IMPcMsg_Important, "  floor quad %d is under floor of BNV (highest %f < floor %f - tolerance %f)\n",
				inGQIndex, highest_height, room->origin.y, IMPcValidFloorMinimumUnderBNV);
		}
#endif
		goto exit;
	}

#if DEBUG_GRID
	if (IMPgTempDebugGrid) {
		Imp_PrintMessage(IMPcMsg_Important, "  floor quad %d maps to (%d %d), (%d %d), (%d %d), (%d %d)\n", inGQIndex,
			grid_points[0].x, grid_points[0].y, grid_points[1].x, grid_points[1].y, 
			grid_points[2].x, grid_points[2].y, grid_points[3].x, grid_points[3].y);
	}
#endif

	if (IMPgBuildPathDebugInfo) {
		if (inForceWeight) {
			PHgCurrentDebugEvent = (inWeight == PHcDanger) ? PHcDebugEvent_DangerQuad : PHcDebugEvent_ImpassableQuad;
		} else {
			PHgCurrentDebugEvent = PHcDebugEvent_Floor;
		}
		PHgCurrentDebugGQ = inGQIndex;
	}

	PHrRasterizeTriangle(grid_points + 0, grid_points + 1, grid_points + 2, draw_color, inForceWeight);
	if ((gqGeneral->flags & AKcGQ_Flag_Triangle) == 0) {
		PHrRasterizeTriangle(grid_points + 2, grid_points + 3, grid_points + 0, draw_color, inForceWeight);
	}

	if (IMPgBuildPathDebugInfo) {
		PHgCurrentDebugEvent = PHcDebugEvent_None;
		PHgCurrentDebugGQ = (UUtUns32) -1;
	}
exit:
	return;
}

/******************* Construction / Destruction *********************/

static void PHrCreatePathfindingGrid(IMPtEnv_BuildData *inBuildData, IMPtEnv_BNV *inBNV, AKtBNVNode *node, AKtEnvironment *env)
{
	#define PHcMaxRoomSize 250.0f	
	PHtSquare			*srcptr, *destptr;
	PHtRoomData			*room = &node->roomData;
	UUtUns32			size_of_pathfinding_grid;
	UUtUns32			itr;
	UUtUns32			gq_index;
	UUtInt16			stair_offset_x, stair_offset_y, dx, dy, d;
	IMPtEnv_GQ			*gq;
	AKtBNVNode_Side*	sideArray = env->bnvSideArray->sides;
	M3tPlaneEquation	*stair_plane;
	UUtBool				isStairs, skip_quad;

	UUtUns32			*gq_list, *outside_bv;
	UUtUns32			gq_list_size;

#if DEBUG_GRID
	M3tPoint3D			*points;
	M3tQuad				current_quad;

	IMPgTempDebugGrid = (node->index == 130);

	if (IMPgTempDebugGrid) {
		UUrEnterDebugger("in debug BNV");
	}
#endif

	PHgTempDebugInfoCount = 0;
	if (IMPgBuildPathDebugInfo) {
		if (PHgTempDebugInfo == NULL) {
			// allocate space for the debuginfo array
			PHgTempDebugInfoMaxCount = IMPcPathDebugInfoMaxSize;
			PHgTempDebugInfo = UUrMemory_Block_New(PHgTempDebugInfoMaxCount * sizeof(PHtDebugInfo));
		}
		PHgTempDebugInfoOverflowed = UUcFalse;
		PHgCurrentDebugEvent = PHcDebugEvent_None;
		PHgCurrentDebugGQ = (UUtUns32) -1;
	}

	// STEP 1: find the gqs that are intersecting with this BNV
	// STEP 2: calculate the size of the grid & verify the temporary grid will do ok
	// STEP 3: initialize the temporary grid as impassible
	// STEP 4: mark the grid as clear everywhere where there is floor
	// STEP 5: mark the grid as dangerous where there have been danger quads placed
	// STEP 6: mark the grid as impassable where there have been impassable quads placed
	// STEP 7: draw in any physical quads in the BNV with the appropriate grid values
	// STEP 8: draw in the stair outlets with 'stairs'
	// STEP 9: mark the grid as 'border' everywhere near danger
	// STEP 10: strip away the temporary buffer around the grid values
	// STEP 11: compress and write the grid
	// STEP 12: write debugging info

	// STEP 1: find the gqs that are intersecting with this BNV
	IMPrFixedOctTree_Test(
		inBuildData, 
		inBNV->minX - 2.f, 
		inBNV->maxX + 2.f, 
		inBNV->minY - 2.f, 
		inBNV->maxY + 2.f, 
		inBNV->minZ - 2.f, 
		inBNV->maxZ + 2.f);

	gq_list_size = inBuildData->fixed_oct_tree_temp_list->numPages;
	gq_list = inBuildData->fixed_oct_tree_temp_list->pages;
#if DEBUG_GRID
	if (IMPgTempDebugGrid) {
		Imp_PrintMessage(IMPcMsg_Important, "BNV %d has %d gqs\n", node->index, gq_list_size);
	}
#endif

	outside_bv = UUrBitVector_New(gq_list_size);

	for(itr = 0; itr < gq_list_size; itr++)
	{
		gq_index = gq_list[itr];
		gq = inBuildData->gqList + gq_index;
		skip_quad = UUcFalse;

		if (gq->flags & AKcGQ_Flag_AIGridIgnore) {
#if DEBUG_GRID
			if (IMPgTempDebugGrid) {
				Imp_PrintMessage(IMPcMsg_Important, "skipping gq %d of %d, which is index #%d - it is marked with grid-ignore\n",
									itr, gq_list_size, gq_index);
			}
#endif
			skip_quad = UUcTrue;

#ifndef DEBUG_NODISCARD
		} else if (!PHiQuadInBNV(inBuildData, inBNV, gq_index)) {
#if DEBUG_GRID
			if (IMPgTempDebugGrid) {
				Imp_PrintMessage(IMPcMsg_Important, "skipping gq %d of %d, which is index #%d - not in BNV\n", itr, gq_list_size, gq_index);
			}
#endif
			skip_quad = UUcTrue;
#endif
		}

		if (skip_quad) {
			UUrBitVector_SetBit(outside_bv, itr);
		}
	}

	// STEP 2: calculate the size of the grid & verify the temporary grid will do ok

	// NB: room->gridX and room->gridY are the size of the section of the temporary grid that we will be keeping.
	// the temporary grid is in fact IMPcGridBufferTemp bigger in every direction

	PHgTempGridOffsetX = IMPcGridBufferTemp - room->gox;
	PHgTempGridOffsetY = IMPcGridBufferTemp - room->goy;

	// no longer needs to be even because byte alignment is not an issue - we are doing RLE instead
//	room->gridX += room->gridX % 2;
//	room->gridY += room->gridY % 2;		
	
	PHgTempGridX = room->gridX + 2 * IMPcGridBufferTemp;
	PHgTempGridY = room->gridY + 2 * IMPcGridBufferTemp;

	size_of_pathfinding_grid = (PHgTempGridX * PHgTempGridY);

	if (size_of_pathfinding_grid > UUmSQR(PHcMaxGridSize)) {
		Imp_PrintWarning("file: %s, obj:%s - BNV too large, pathfinding will barf",inBNV->fileName,inBNV->objName);
		return;
	}

	if (node->flags & AKcBNV_Flag_Stairs) {
		isStairs = UUcTrue;
		stair_plane = &node->stairPlane;
	} else {
		isStairs = UUcFalse;
		stair_plane = NULL;
	}
	
	// STEP 3: initialize the temporary grid to PHcDanger - as AIs should never, ever be
	// walking anywhere that there's no floor
	UUrMemory_Set8(PHgTempGrid, PHcDanger, size_of_pathfinding_grid * sizeof(PHtSquare));

	if (isStairs) {
		IMtPoint2D stair_points[4];

		// CB: we do not rasterize gunk as normal! instead we just draw in a single floor quad
		// that lies between the endpoints of the two SATs that define the stairs.
		//
		// this is a highly suboptimal solution, but I do not have time to deal with more wacky
		// rasterization and quad culling issues.
		//
		// a better solution would be to rasterize as normal, but check every quad against the
		// volume defined by the intersection of the three planes of the down-SAT, up-SAT and
		// the stairPlane... discarding any quads that do not intersect this volume.
		PHiFindSATs(env, inBuildData, inBNV, node);

		IMPmGetGridPoint(stair_points[0], inBNV->sat_points[0][0], room);
		IMPmGetGridPoint(stair_points[1], inBNV->sat_points[0][1], room);
		IMPmGetGridPoint(stair_points[2], inBNV->sat_points[1][0], room);
		IMPmGetGridPoint(stair_points[3], inBNV->sat_points[1][1], room);

		// stretch this quad by roughly 4 squares in either direction
		stair_offset_x = stair_points[3].x - stair_points[0].x;
		stair_offset_y = stair_points[3].y - stair_points[0].y;

		dx = UUmABS(stair_offset_x); dy = UUmABS(stair_offset_y); d = UUmMax(dx, dy);
		if (d > 0) {
			stair_offset_x = (4 * stair_offset_x) / d;
			stair_offset_y = (4 * stair_offset_y) / d;
		}

		stair_points[0].x -= stair_offset_x;
		stair_points[0].y -= stair_offset_y;
		stair_points[1].x -= stair_offset_x;
		stair_points[1].y -= stair_offset_y;
		stair_points[2].x += stair_offset_x;
		stair_points[2].y += stair_offset_y;
		stair_points[3].x += stair_offset_x;
		stair_points[3].y += stair_offset_y;

		if (IMPgBuildPathDebugInfo) {
			PHgCurrentDebugEvent = PHcDebugEvent_StairQuad;
			PHgCurrentDebugGQ = (UUtUns32) -1;
		}

		PHrRasterizeTriangle(stair_points + 0, stair_points + 1, stair_points + 2, PHcClear, UUcFalse);
		PHrRasterizeTriangle(stair_points + 2, stair_points + 3, stair_points + 0, PHcClear, UUcFalse);

		if (IMPgBuildPathDebugInfo) {
			PHgCurrentDebugEvent = PHcDebugEvent_StairQuad;
			PHgCurrentDebugGQ = (UUtUns32) -1;
		}

	} else if (gq_list_size > 0) {
		inBNV->sats[0] = inBNV->sats[1] = NULL;
		UUrMemory_Clear(inBNV->sat_points, 4 * sizeof(M3tPoint3D));

		// STEP 4: draw the floor in as clear
		for(itr = 0; itr < gq_list_size; itr++)
		{
			if (UUrBitVector_TestBit(outside_bv, itr))
				continue;

			gq_index = gq_list[itr];
			gq = inBuildData->gqList + gq_index;
			if (gq->flags & (AKcGQ_Flag_AIDanger | AKcGQ_Flag_AIImpassable)) {
				continue;
			}

#if DEBUG_GRID
			if (IMPgTempDebugGrid) {
				points = env->pointArray->points;
				current_quad = gq->visibleQuad;

				Imp_PrintMessage(IMPcMsg_Important, "checking gq #%d as floor, (%f %f %f) (%f %f %f), (%f %f %f), (%f %f %f)\n", gq_index,
					points[current_quad.indices[0]].x, points[current_quad.indices[0]].y, points[current_quad.indices[0]].z, 
					points[current_quad.indices[1]].x, points[current_quad.indices[1]].y, points[current_quad.indices[1]].z, 
					points[current_quad.indices[2]].x, points[current_quad.indices[2]].y, points[current_quad.indices[2]].z, 
					points[current_quad.indices[3]].x, points[current_quad.indices[3]].y, points[current_quad.indices[3]].z);
			}
#endif

			// FIXME: only draw quads that are valid to walk on (i.e. not glass)
			PHrRasterizeFloorQuad(inBuildData, gq_index, env, room, UUcFalse, 0, stair_plane);
		}

		// STEP 5: draw any danger quads in
		for(itr = 0; itr < gq_list_size; itr++)
		{
			if (UUrBitVector_TestBit(outside_bv, itr))
				continue;

			gq_index = gq_list[itr];
			gq = inBuildData->gqList + gq_index;
			if ((gq->flags & AKcGQ_Flag_AIDanger) == 0) {
				continue;
			}

#if DEBUG_GRID
			if (IMPgTempDebugGrid) {
				points = env->pointArray->points;
				current_quad = gq->visibleQuad;

				Imp_PrintMessage(IMPcMsg_Important, "gq #%d is danger, (%f %f %f) (%f %f %f), (%f %f %f), (%f %f %f)\n", gq_index,
					points[current_quad.indices[0]].x, points[current_quad.indices[0]].y, points[current_quad.indices[0]].z, 
					points[current_quad.indices[1]].x, points[current_quad.indices[1]].y, points[current_quad.indices[1]].z, 
					points[current_quad.indices[2]].x, points[current_quad.indices[2]].y, points[current_quad.indices[2]].z, 
					points[current_quad.indices[3]].x, points[current_quad.indices[3]].y, points[current_quad.indices[3]].z);
			}
#endif

			PHrRasterizeFloorQuad(inBuildData, gq_index, env, room, UUcTrue, PHcDanger, stair_plane);
		}

		// STEP 6: draw any impassable quads in
		for(itr = 0; itr < gq_list_size; itr++)
		{
			if (UUrBitVector_TestBit(outside_bv, itr))
				continue;

			gq_index = gq_list[itr];
			gq = inBuildData->gqList + gq_index;
			if ((gq->flags & AKcGQ_Flag_AIImpassable) == 0) {
				continue;
			}

#if DEBUG_GRID
			if (IMPgTempDebugGrid) {
				points = env->pointArray->points;
				current_quad = gq->visibleQuad;

				Imp_PrintMessage(IMPcMsg_Important, "gq #%d is impassable, (%f %f %f) (%f %f %f), (%f %f %f), (%f %f %f)\n", gq_index,
					points[current_quad.indices[0]].x, points[current_quad.indices[0]].y, points[current_quad.indices[0]].z, 
					points[current_quad.indices[1]].x, points[current_quad.indices[1]].y, points[current_quad.indices[1]].z, 
					points[current_quad.indices[2]].x, points[current_quad.indices[2]].y, points[current_quad.indices[2]].z, 
					points[current_quad.indices[3]].x, points[current_quad.indices[3]].y, points[current_quad.indices[3]].z);
			}
#endif

			PHrRasterizeFloorQuad(inBuildData, gq_index, env, room, UUcTrue, PHcImpassable, stair_plane);
		}

#ifndef DEBUG_DISABLE_WALLS
		// STEP 7: Rasterize wall quads
		for (itr = 0; itr < gq_list_size; itr++)
		{
			gq_index = gq_list[itr];
			gq = inBuildData->gqList + gq_index;
			if (UUrBitVector_TestBit(outside_bv, itr))
				continue;

			if (!(gq->flags & AKcGQ_Flag_PathfindingMask)) {
				PHrChooseAndWeightQuad(inBuildData, gq_index, env, room);
			}
		}
#endif

#ifndef DEBUG_DISABLE_SATS
		// STEP 8: Now cycle through all the SATs and rasterize in the 'stairs' value
		for (itr = 0; itr < gq_list_size; itr++)
		{
			gq_index = gq_list[itr];
			gq = inBuildData->gqList + gq_index;
			if (UUrBitVector_TestBit(outside_bv, itr))
				continue;

			if (gq->flags & AKcGQ_Flag_SAT_Mask) {	
				PHrChooseAndWeightQuad(inBuildData, gq_index, env, room);
			}
		}
#endif
	}

	// STEP 9: create border values everywhere that is clear but near danger
	PHrMarkBorderValues(PHcClear, PHcDanger,  PHcBorder4);
	PHrMarkBorderValues(PHcClear, PHcBorder4, PHcBorder3);
	PHrMarkBorderValues(PHcClear, PHcBorder3, PHcBorder2);
	PHrMarkBorderValues(PHcClear, PHcBorder2, PHcBorder1);
	PHrMarkBorderValues(PHcClear, PHcSemiPassable, PHcNearWall);

#if DEBUG_GRID
	if (IMPgTempDebugGrid) {
		Imp_PrintMessage(IMPcMsg_Important, "pathfinding grid #%d:\n", node->index);
		PHrDrawGrid();
	}
#endif

	// STEP 10: strip away the temporary buffer and pack the section that we want to keep
	// into the start of the TempGrid array
	srcptr = PHgTempGrid + IMPcGridBufferTemp + IMPcGridBufferTemp * PHgTempGridX;
	destptr = PHgTempGrid;
	for (itr = 0; itr < room->gridY; itr++) {
		// these areas should never overlap because we're copying from one row
		// into the start of another
		UUmAssert(srcptr - destptr >= (UUtInt16) room->gridX);
		UUrMemory_MoveFast((void *) srcptr, (void *) destptr, room->gridX * sizeof(PHtSquare));

		srcptr += PHgTempGridX;
		destptr += room->gridX;
	}

	size_of_pathfinding_grid = room->gridX * room->gridY;

	// STEP 11: compress the pathfinding grid and write it to disk
	PHrGrid_Compress(PHgTempGrid, size_of_pathfinding_grid, PHgCompressedGrid, &PHgCompressedGridSize);
	PHgOriginalMemoryUsage += size_of_pathfinding_grid;
	PHgCompressedMemoryUsage += PHgCompressedGridSize;

	room->compressed_grid_size = PHgCompressedGridSize;
	room->compressed_grid = TMrConstruction_Raw_New(PHgCompressedGridSize, 0, AKcTemplate_BNVNodeArray);

	UUrMemory_MoveFast(PHgCompressedGrid, room->compressed_grid, PHgCompressedGridSize);
	room->compressed_grid = TMrConstruction_Raw_Write(room->compressed_grid);

	if (outside_bv != NULL) {
		UUrBitVector_Dispose(outside_bv);
	}

	// STEP 12: write debugging info
	room->debug_info_count = PHgTempDebugInfoCount;
	if (room->debug_info_count == 0) {
		room->debug_info = NULL;
	} else {
		room->debug_info = TMrConstruction_Raw_New(room->debug_info_count * sizeof(PHtDebugInfo), 0, AKcTemplate_BNVNodeArray);
		UUrMemory_MoveFast(PHgTempDebugInfo, room->debug_info, room->debug_info_count * sizeof(PHtDebugInfo));
		room->debug_info = TMrConstruction_Raw_Write(room->debug_info);
	}
}

void PHrCreateRoomData(IMPtEnv_BuildData *inBuildData, IMPtEnv_BNV *inBNV, AKtBNVNode *node, AKtEnvironment *env)
{
	PHtRoomData			*room = &node->roomData;

	if (!(node->flags & AKcBNV_Flag_Room)) {
		return;
	}

	UUrMemory_Clear(room, sizeof(*room));
	
	room->origin = inBNV->origin;
	room->antiOrigin = inBNV->antiOrigin;
	room->compressed_grid = NULL;
	room->compressed_grid_size = 0;
	room->squareSize = PHcSquareSize;

	// create the room's grid as IMPcGridBuffer more than it needs to be in every direction
	//
	// gox and goy are offsets that are added to grid-space to get worldspace coordinates relative
	// to the BNV.
	room->gox = room->goy = -IMPcGridBuffer;
	room->gridX = (UUtUns32)((room->antiOrigin.x - room->origin.x) / room->squareSize) - 2*room->gox + 1;	
	room->gridY = (UUtUns32)((room->antiOrigin.z - room->origin.z) / room->squareSize) - 2*room->gox + 1;

	if (node->flags & AKcBNV_Flag_NonAI) {
		goto exit;
	}

	room->parent = node->index;

	if (node->flags & AKcBNV_Flag_SimplePathfinding) {
		goto exit;
	}
	

	PHrCreatePathfindingGrid(inBuildData, inBNV, node, env);

exit:
	return;
}

static void PHiFindSATs(AKtEnvironment *inEnvironment, IMPtEnv_BuildData *inBuildData, IMPtEnv_BNV *inBNV,
						AKtBNVNode *inNode)
{
	UUtUns32 upsat_number = 0, downsat_number = 0;
	IMPtEnv_GQ *gq_list = inBuildData->gqList;
	UUtUns32 *gq_index_list = inBNV->gqList;
	M3tPoint3D *p0, *p1, *u0, *u1, *d0, *d1;
	M3tPoint3D downedge, upedge, stairvec;
	UUtUns32 itr;
	UUtUns32 count;

	inBNV->sats[0] = NULL;
	inBNV->sats[1] = NULL;
	u0 = u1 = d0 = d1 = NULL;

	UUrMemory_Clear(inBNV->sat_points, 4 * sizeof(M3tPoint3D));

	count = inBNV->numGQs;
	for(itr = 0; itr < count; itr++)
	{
		UUtUns32 gq_index = gq_index_list[itr];
		IMPtEnv_GQ *gq = gq_list + gq_index;

		if ((gq->flags & AKcGQ_Flag_SAT_Mask) == 0)
			continue;

		// important: only include SATs have both of their lowest points in this BNV
		// this stops us from mistakenly getting two up SATs or whatever in BNVs that touch
		// each other
		AUrQuad_LowestPoints(&gq->visibleQuad, inEnvironment->pointArray->points, &p0, &p1);
		
		if ((inNode->roomData.origin.x - p0->x		> IMPcSAT_InsideBNVTolerance) ||
			(p0->x - inNode->roomData.antiOrigin.x	> IMPcSAT_InsideBNVTolerance) ||
			(inNode->roomData.origin.x - p1->x		> IMPcSAT_InsideBNVTolerance) ||
			(p1->x - inNode->roomData.antiOrigin.x	> IMPcSAT_InsideBNVTolerance) ||
			(inNode->roomData.origin.z - p0->z		> IMPcSAT_InsideBNVTolerance) ||
			(p0->z - inNode->roomData.antiOrigin.z	> IMPcSAT_InsideBNVTolerance) ||
			(inNode->roomData.origin.z - p1->z		> IMPcSAT_InsideBNVTolerance) ||
			(p1->z - inNode->roomData.antiOrigin.z	> IMPcSAT_InsideBNVTolerance))
			continue;

		if (gq->flags & AKcGQ_Flag_SAT_Up) {
			inBNV->sats[1] = gq;
			u0 = p0; u1 = p1;
			upsat_number++;
		} else {
			inBNV->sats[0] = gq;
			d0 = p0; d1 = p1;

			downsat_number++;
		}
	}

	if ((upsat_number != 1) || (downsat_number != 1)) {
		Imp_PrintWarning("file: %s, obj:%s - stair BNV doesn't have correct SAT count (%d up, %d down)",
					inBNV->fileName, inBNV->objName, upsat_number, downsat_number);

		if ((0 == upsat_number) || (0 == downsat_number)) {
			goto exit;
		}
	}

	// calculate vectors for each SAT and the length of the stairs
	MUmVector_Subtract(downedge, *d1, *d0);
	MUmVector_Subtract(upedge,   *u1, *u0);
	MUmVector_Subtract(stairvec, *u0, *d0);

	// these points must be ordered in a counter-clockwise fashion in 2D looking down from +Y, so that
	// we can rasterize them as a quad
	if (downedge.x * stairvec.z - downedge.z * stairvec.x > 0) {
		MUmVector_Copy(inBNV->sat_points[0][0], *d0);
		MUmVector_Copy(inBNV->sat_points[0][1], *d1);
	} else {
		MUmVector_Copy(inBNV->sat_points[0][0], *d1);
		MUmVector_Copy(inBNV->sat_points[0][1], *d0);
	}

	if (upedge.x * stairvec.z - upedge.z * stairvec.x < 0) {
		MUmVector_Copy(inBNV->sat_points[1][0], *u0);
		MUmVector_Copy(inBNV->sat_points[1][1], *u1);
	} else {
		MUmVector_Copy(inBNV->sat_points[1][0], *u1);
		MUmVector_Copy(inBNV->sat_points[1][1], *u0);
	}

exit:
	return;
}


static void PHiDebugInfoRasterizationCallback(UUtInt16 inX, UUtInt16 inY, UUtUns8 inColor)
{
	IMPmAddDebugInfo(PHgCurrentDebugEvent, inX, inY, PHgCurrentDebugGQ, inColor);
}

static void PHiRasterizeSAT(
	IMPtEnv_BuildData *inBuildData,
	IMPtEnv_BNV *inBNV,
	PHtRoomData *room,
	UUtUns32 inGQIndex,
	M3tPoint3D *pointA,
	M3tPoint3D *pointB,
	UUtBool inLightOnly)
{
	/***************
	* Rasterizes the specified SAT into the specified map
	*/

	IMtPoint2D gridp1, gridp2, gridp3, gridp4;
	M3tPoint3D *o = &room->origin, bnv_center;
	PHtSquare fill = { PHcClear };
	PHtSquare blend = { PHcClear };
	UUtUns32 adjacent_bnv;
	M3tVector3D towards_vec, perp_vec, offsetp1, offsetp2;
	PHtRasterizationCallback callback = (IMPgBuildPathDebugInfo) ? PHiDebugInfoRasterizationCallback : NULL;

#if DEBUG_GRID
	if (IMPgTempDebugGrid) {
		Imp_PrintMessage(IMPcMsg_Important, "  rasterizing SAT #%d\n", inGQIndex);
	}
#endif

	// Plot the wall in the grid
	IMPmGetGridPoint(gridp1, *pointA, room);
	IMPmGetGridPoint(gridp2, *pointB, room);

#if DEBUG_GRID
	if (IMPgTempDebugGrid) {
		Imp_PrintMessage(IMPcMsg_Important, "  edge is (%d %d) - (%d %d)\n", gridp1.x, gridp1.y, gridp2.x, gridp2.y);
	}
#endif

#if 0
	// CB: this code is now superseded by the fact that we directly store stairBNVIndex in every
	// SAT quad, but I'm leaving it here in case it's needed later. 14 september 2000
	{
		UUtUns32 side_itr, adj_itr;
		AKtAdjacency *adjacency;

		// determine which of the BNV's adjacencies this SAT corresponds to
		adjacent_bnv = (UUtUns32) -1;
		for (side_itr = 0; side_itr < inBNV->numSides; side_itr++) {
			for (adj_itr = 0, adjacency = inBNV->sides[side_itr].adjacencyList;
				 adj_itr < inBNV->sides[side_itr].numAdjacencies; adj_itr++, adjacency++) {
				if (adjacency->adjacentGQIndex == inGQIndex) {
					// found the adjacency
					adjacent_bnv = adjacency->adjacentBNVIndex;
					break;
				}
			}
		}
	}
#else
	adjacent_bnv = inBuildData->gqList[inGQIndex].stairBNVIndex;
#endif

	if (adjacent_bnv != (UUtUns32) -1) {
		// get the location of the center of the stair bnv, so we can determine which side of the SAT the
		// 'stairs' grid values should be rasterized on
		MUmVector_ScaleCopy(bnv_center, 0.5f, inBuildData->bnvList[adjacent_bnv].origin);
		MUmVector_ScaleIncrement(bnv_center, 0.5f, inBuildData->bnvList[adjacent_bnv].antiOrigin);

		// build a perpendicular vector that is towards the bnv's center
		perp_vec.x = - (pointB->z - pointA->z);
		perp_vec.y = 0;
		perp_vec.z =   (pointB->x - pointA->x);

		towards_vec.x = bnv_center.x - ((pointA->x + pointB->x) / 2);
		towards_vec.y = 0;
		towards_vec.z = bnv_center.z - ((pointA->z + pointB->z) / 2);

		if (perp_vec.x * towards_vec.x + perp_vec.z * towards_vec.z < 0) {
			perp_vec.x *= -1;
			perp_vec.z *= -1;
		}

		// set this vector to a fixed length
		MUrVector_SetLength(&perp_vec, IMPcStairOutletWidth);
		MUmVector_Add(offsetp1, *pointA, perp_vec);
		MUmVector_Add(offsetp2, *pointB, perp_vec);
		IMPmGetGridPoint(gridp3, offsetp1, room);
		IMPmGetGridPoint(gridp4, offsetp2, room);

		// rasterize the outlet into the grid
		PHrRasterizeTriangle(&gridp1, &gridp2, &gridp3, PHcStairs, UUcTrue);
		PHrRasterizeTriangle(&gridp2, &gridp4, &gridp3, PHcStairs, UUcTrue);

	} else {
		// we could not find the adjacency, just rasterize in the single line of the SAT with the
		// 'stairs' weight
		blend.weight = PHcClear;
		fill.weight = PHcStairs;

		PHrBresenhamAA(gridp1.x, gridp1.y, gridp2.x, gridp2.y,
						PHgTempGrid, PHgTempGridX, PHgTempGridY,
						&fill, &blend, callback);
	}

	// rasterize the end points as impassable to stop AIs from cutting
	// corners too closely at the base of stairs
	PHrFatDot(gridp1.x,gridp1.y,PHgTempGrid,PHgTempGridX,PHgTempGridY,callback);
	PHrFatDot(gridp2.x,gridp2.y,PHgTempGrid,PHgTempGridX,PHgTempGridY,callback);
}

static void PHiRasterizeWallQuad(
	IMPtEnv_BuildData *inBuildData,
	IMPtEnv_BNV *inBNV,
	PHtRoomData *room,
	UUtUns32 inGQIndex,
	M3tPoint3D *pointA,
	M3tPoint3D *pointB,
	UUtBool inLightOnly)
{
	/***************
	* Rasterizes the specified edge of the quad into the specified map
	*/

	IMtPoint2D gridp1, gridp2;
	M3tPoint3D *o = &room->origin, *p1, *p2, endp;
	PHtSquare fill = { PHcClear };
	PHtSquare blend = { PHcClear };
	float dy, overhang_height, interp;
	PHtRasterizationCallback callback = (IMPgBuildPathDebugInfo) ? PHiDebugInfoRasterizationCallback : NULL;

	// only rasterize segment that actually overhangs
	overhang_height = o->y + IMPcWalkUnderHeight;

	if (pointA->y < pointB->y) {
		p1 = pointA;
		p2 = pointB;
	} else {
		p1 = pointB;
		p2 = pointA;
	}

	if (p2->y < overhang_height) {
		// rasterize all of this edge
		endp = *p2;
#if DEBUG_GRID
		if (IMPgTempDebugGrid) {
			Imp_PrintMessage(IMPcMsg_Important, "  all of edgebase lies below overhang height (%f, %f < %f)\n", p1->y, p2->y, overhang_height);
		}
#endif
	} else if (p1->y > overhang_height) {
		// don't bother rasterizing this edge at all, it is above the overhang height
#if DEBUG_GRID
		if (IMPgTempDebugGrid) {
			Imp_PrintMessage(IMPcMsg_Important, "  none of edgebase lies below overhang height (%f, %f > %f)\n", p1->y, p2->y, overhang_height);
		}
#endif
		return;
	} else {
		dy = p2->y - p1->y;
		UUmAssert(dy >= 0);
		if (dy <= 0.1f) {
			// edge is horizontal, and must be within the rasterization range, rasterize all of it
			endp = *p2;
#if DEBUG_GRID
			if (IMPgTempDebugGrid) {
				Imp_PrintMessage(IMPcMsg_Important, "  horizontal edgebase lies below overhang height (logic error?!) (%f, %f <> %f)\n",
								 p1->y, p2->y, overhang_height);
			}
#endif
		} else {
			interp = (overhang_height - p1->y) / dy;
			UUmAssert((interp >= 0.0f) && (interp <= 1.0f));
			endp.x = p1->x + interp * (p2->x - p1->x);
			endp.y = p1->y + interp * (p2->y - p1->y);
			endp.z = p1->z + interp * (p2->z - p1->z);
#if DEBUG_GRID
			if (IMPgTempDebugGrid) {
				Imp_PrintMessage(IMPcMsg_Important, "  edgebase (%f %f) spans overhang height %f -> (%f, %f) - (%f, %f) gets interp %f midp (%f, %f)\n",
									p1->y, p2->y, overhang_height, p1->x, p1->z, p2->x, p2->z, interp, endp.x, endp.z);
			}
#endif
		}
	}

	// Plot the wall in the grid
	IMPmGetGridPoint(gridp1, *p1, room);
	IMPmGetGridPoint(gridp2, endp, room);

#if DEBUG_GRID
	if (IMPgTempDebugGrid) {
		Imp_PrintMessage(IMPcMsg_Important, "  wall is (%d %d) - (%d %d)\n", gridp1.x, gridp1.y, gridp2.x, gridp2.y);
	}
#endif

	blend.weight = PHcSemiPassable;
	fill.weight = inLightOnly ? PHcSemiPassable : PHcImpassable;

	PHrBresenhamAA(gridp1.x, gridp1.y, gridp2.x, gridp2.y,
					PHgTempGrid, PHgTempGridX, PHgTempGridY,
					&fill, &blend, callback);
}


static UUtBool PHiQuadInRoom(
	AKtEnvironment *inEnv,
	PHtRoomData *inRoom,
	IMPtEnv_GQ *inQuad,
	M3tPoint3D *inPoints)
{
	/******************
	* Returns true if any of the points are in the room (not just the BNV)
	*/
	
	UUtUns32 i;
	
	for (i=0; i<4; i++)
	{
		if (PHrPointInRoom(inEnv,inRoom,inPoints + inQuad->visibleQuad.indices[i])) return UUcTrue;
	}

	return UUcFalse;
}

void PHrChooseAndWeightQuad(
	IMPtEnv_BuildData *inBuildData, 
	UUtUns32 gqIndex, 
	AKtEnvironment *env, 
	PHtRoomData *room)
{
	/***************
	* Decides whether to rasterize a gunk quad 'gunk' into the pathfinding grid of 'room'
	*/

	AKtGQ_General *envGQGeneral = env->gqGeneralArray->gqGeneral + gqIndex;
	IMPtEnv_GQ *gunk = inBuildData->gqList + gqIndex;
	IMPtEnv_BNV *bnv = inBuildData->bnvList + room->parent;
	M3tQuad *quad;
	float planeA,planeB,planeC,planeD;
	M3tPoint3D *o,*points;
	UUtBool isSAT = UUcFalse,lightOnly = UUcFalse;
//	M3tVector3D normal,qnormal;

	o = &room->origin;
	points = env->pointArray->points;
	if (gunk->flags & (AKcGQ_Flag_SAT_Up | AKcGQ_Flag_SAT_Down)) isSAT = UUcTrue;
	
	/*
	CB: this is irrelevant now, stair BNVs are not rasterized as normal

	if (isStairs && PHiQuadBelowStairs(env, room, gunk, points, inStairPlane))
		return;
*/

	quad = &gunk->visibleQuad;
	AKmPlaneEqu_GetComponents(
		gunk->planeEquIndex,
		env->planeArray->planes, planeA, planeB, planeC, planeD);
	
	
	// Eliminate curbs from collision
	if (UUmFloat_CompareEqu(planeB,0) &&
		(float)fabs(gunk->hi1->y - o->y) < 4.0f &&
		(float)fabs(gunk->hi1->y - gunk->lo1->y) < 4.0f)
	{
		envGQGeneral->flags |= AKcGQ_Flag_No_Character_Collision;
	}

	// If this quad is above the floor, then possibly skip it.
	if (gunk->lo1->y > o->y && gunk->lo2->y > o->y)
	{
		// If we can walk under it, skip it unless it's an SAT
		if (!((isSAT && ((bnv->sats[0] == gunk) || (bnv->sats[1] == gunk)))))
		{
			if ((float)fabs(gunk->lo1->y - o->y) > IMPcWalkUnderHeight) {
				return;
			}
		}
	}

	// If this quad can be walked on and is on the floor, skip it.
	if ((float)fabs(planeB) > PHcFlatNormal && (float)fabs(gunk->lo1->y - o->y)<4.0 && (float)fabs(gunk->hi1->y - o->y)<4.0) return;
	
	// If this quad is entirely below the floor, then skip it
	if (gunk->hi1->y <= o->y && gunk->hi2->y <= o->y) return;

	// CB: if this quad wouldn't be collided with by characters, don't rasterize it.
	if (envGQGeneral->flags & (AKcGQ_Flag_No_Collision | AKcGQ_Flag_No_Character_Collision))
		return;

	// If this quad is sloped, then handle it in a cool way
	if ((float)fabs(planeB) > 0.01f && (float)fabs(planeB) < 0.999f && !lightOnly)
	{
#ifndef DEBUG_DISABLE_SLOPING
		if (IMPgBuildPathDebugInfo) {
			PHgCurrentDebugEvent = PHcDebugEvent_SlopedQuad;
			PHgCurrentDebugGQ = gqIndex;
		}

		PHrRasterizeSlopedQuad(gunk,env,room);

		if (IMPgBuildPathDebugInfo) {
			PHgCurrentDebugEvent = PHcDebugEvent_None;
			PHgCurrentDebugGQ = (UUtUns32) -1;
		}
		return;
#endif
	}
	
	// If this quad is vertical, short and touching the floor, skip it
	// (Makes curbs work)
	if ((float)fabs(gunk->hi1->y - gunk->lo1->y) < 4.0f &&
		UUmFloat_CompareEqu(planeB,0) &&
		UUmFloat_CompareEqu(gunk->lo1->y,o->y)) return;

	// Plot the wall in the grid
#ifndef DEBUG_DISABLE_VERTICAL
#if DEBUG_GRID
	if (IMPgTempDebugGrid) {
		Imp_PrintMessage(IMPcMsg_Important, "  rasterizing edge quad %d (%sSAT) 0x%08X %s\n",
						gqIndex, (isSAT) ? "is " : "not ", envGQGeneral->flags,
						(envGQGeneral->flags & AKcGQ_Flag_No_Character_Collision) ? "nocollide" : "collision-on");
	}
#endif
	if (IMPgBuildPathDebugInfo) {
		PHgCurrentDebugEvent = (isSAT) ? PHcDebugEvent_StairQuad : PHcDebugEvent_Wall;
		PHgCurrentDebugGQ = gqIndex;
	}

	if (isSAT) {
		PHiRasterizeSAT(inBuildData, bnv, room, gqIndex, gunk->lo1, gunk->lo2, lightOnly);
	} else {
		PHiRasterizeWallQuad(inBuildData, bnv, room, gqIndex, gunk->lo1, gunk->lo2, lightOnly);
	}

	if (IMPgBuildPathDebugInfo) {
		PHgCurrentDebugEvent = PHcDebugEvent_None;
		PHgCurrentDebugGQ = (UUtUns32) -1;
	}
#endif
}




void PHrRasterizeSlopedQuad(
	IMPtEnv_GQ *gunk, 
	AKtEnvironment *env, 
	PHtRoomData *room)
{
	/***************************
	* Rasterizes a sloped quad into the grid
	*/

	IMtPoint2D p1, p2, p3, p4, p5;
	float planeA,planeB,planeC,planeD, interp1, interp2, interp3, overhang_height, dy;
	M3tPoint3D *st,*ed,*t1,*t2,*o,ex1,ex2,ex3,bottom_vector, top_vector;
	UUtBool fill_solid;
	PHtSquare fill = { PHcImpassable };
	PHtSquare blend = { PHcSemiPassable };
	PHtRasterizationCallback callback = (IMPgBuildPathDebugInfo) ? PHiDebugInfoRasterizationCallback : NULL;
#if 0
	float step,length;
	UUtBool isSAT = UUcFalse;
#endif

	o = &room->origin;

	// Find the two extremes of this quad
	st = gunk->lo1;
	ed = gunk->lo2;
	MUmVector_Subtract(bottom_vector, *ed, *st);
	MUmVector_Subtract(top_vector, *(gunk->hi2), *(gunk->hi1));

	// make sure that the bottom and top vectors point in the same direction...
	// i.e. st - t1 and ed - t2 are edges, not diagonals
	if (bottom_vector.x * top_vector.x + bottom_vector.z * top_vector.z > 0) {
		t1 = gunk->hi1;
		t2 = gunk->hi2;
	} else {
		t1 = gunk->hi2;
		t2 = gunk->hi1;
	}
	
/*	if (IMPgTempDebugGrid) {
		Imp_PrintMessage(IMPcMsg_Important, "sloping quad in room origin %f %f %f\n", o->x, o->y, o->z);
		Imp_PrintMessage(IMPcMsg_Important, "  st: %f %f %f\n", st->x, st->y, st->z);
		Imp_PrintMessage(IMPcMsg_Important, "  ed: %f %f %f\n", ed->x, ed->y, ed->z);
		Imp_PrintMessage(IMPcMsg_Important, "  t1: %f %f %f\n", t1->x, t1->y, t1->z);
		Imp_PrintMessage(IMPcMsg_Important, "  t2: %f %f %f\n", t2->x, t2->y, t2->z);
	}*/

	AKmPlaneEqu_GetComponents(
		gunk->planeEquIndex,
		env->planeArray->planes, planeA, planeB, planeC, planeD);

#if 0
// CB: this code is an abomination unto the lord. preserved for posterity.

	// Lay out the projection of the slope on the floor
	ex1 = *st;
	ex2 = *t1;
	ex1.y = ex2.y = 0.0f;
	length = MUrPoint_Distance(&ex1,&ex2);
	step =  PHcSquareSize-1.0f;
	
	// Step up the quad until the point of no return is found
	while (step < length)
	{
		ex1 = *st;
		ex2 = *ed;
		MUmVector_Subtract(edge1,*t1,*st);
		MUmVector_Subtract(edge2,*t2,*ed);
		if (!MUmPoint_Compare(*t1,*st)) MUrVector_SetLength(&edge1,step);
		if (!MUmPoint_Compare(*t2,*ed)) MUrVector_SetLength(&edge2,step);
		MUmVector_Increment(ex1,edge1);
		MUmVector_Increment(ex2,edge2);
		
		if ((float)fabs(ex1.y - o->y) > IMPcWalkUnderHeight ||
			(float)fabs(ex2.y - o->y) > IMPcWalkUnderHeight) break;
		step += PHcSquareSize;
	}
	
	// Rasterize out to that point
	IMPmGetGridPoint(p1, ex1, room);
	IMPmGetGridPoint(p2, *st, room);

	PHrBresenhamAA(p1.x, p1.y, p2.x, p2.y,
			PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);
	
	IMPmGetGridPoint(p1, ex2, room);
	IMPmGetGridPoint(p2, *ed, room);

	PHrBresenhamAA(p1.x, p1.y, p2.x, p2.y,
			PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

	// If it's too steep to walk on, rasterize low edge as well (makes slanted walls work)
	if ((float)fabs(planeB) < PHcFlatNormal)
	{
		PHiRasterizeEdgeQuad(st,ed,room,UUcFalse,UUcFalse);
	}
#else
	// CB: if the quad touches the ground at the low side, and isn't so steep that we can't walk
	// on it, it is probably a set of stairs and we shouldn't solid-fill the unwalkable area
	// or rasterize the base line
	if (((st->y - o->y < IMPcOverhangIsStairTolerance) ||
		 (ed->y - o->y < IMPcOverhangIsStairTolerance)) &&
		((float)fabs(planeB) > PHcFlatNormal)) {
		fill_solid = UUcFalse;
	} else {
		fill_solid = UUcTrue;
	}

/*#if DEBUG_GRID
	if (IMPgTempDebugGrid) {
		if (fill_solid) {
			Imp_PrintMessage(IMPcMsg_Important, "  skipping a solid-filled sloping quad\n");
			return;
		}
	}
#endif*/
	
		overhang_height = o->y + IMPcWalkUnderHeight;

/*	if (IMPgTempDebugGrid) {
		Imp_PrintMessage(IMPcMsg_Important, "  fill_solid: %s, overhang: %f\n",
							fill_solid ? "yes" : "no", overhang_height);
	}*/

	if (t1->y < overhang_height) {
		// rasterize edge 1 all the way out to the end
		interp1 = 1.0f;
		ex1.x = t1->x;
		ex1.z = t1->z;
	} else if (st->y > overhang_height) {
		// don't bother rasterizing edge 1 at all
		interp1 = 0.0f;
		ex1.x = st->x;
		ex1.z = st->z;
	} else {
		dy = t1->y - st->y;
		UUmAssert(dy > 0);
		if (dy <= 0.1f) {
			// edge is horizontal, and must be within the rasterization range
			interp1 = 1.0f;
			ex1.x = t1->x;
			ex1.z = t1->z;
		} else {
			interp1 = (overhang_height - st->y) / dy;
			UUmAssert((interp1 >= 0.0f) && (interp1 <= 1.0f));
			ex1.x = st->x + interp1 * (t1->x - st->x);
			ex1.z = st->z + interp1 * (t1->z - st->z);
		}
	}

	if (t2->y < overhang_height) {
		// rasterize edge 2 all the way out to the end
		interp2 = 1.0f;
		ex2.x = t2->x;
		ex2.z = t2->z;
	} else if (ed->y > overhang_height) {
		// don't bother rasterizing edge 2 at all
		interp2 = 0.0f;
		ex2.x = ed->x;
		ex2.z = ed->z;
	} else {
		dy = t2->y - ed->y;
		UUmAssert(dy > 0);
		if (dy <= 0.1f) {
			// edge is horizontal, and must be within the rasterization range
			interp2 = 1.0f;
			ex2.x = t2->x;
			ex2.z = t2->z;
		} else {
			interp2 = (overhang_height - ed->y) / dy;
			UUmAssert((interp2 >= 0.0f) && (interp2 <= 1.0f));
			ex2.x = ed->x + interp2 * (t2->x - ed->x);
			ex2.z = ed->z + interp2 * (t2->z - ed->z);
		}
	}

/*	if (IMPgTempDebugGrid) {
		Imp_PrintMessage(IMPcMsg_Important, "  edge 1: interp %f, point %f %f\n",
							interp1, ex1.x, ex1.z);
		Imp_PrintMessage(IMPcMsg_Important, "  edge 2: interp %f, point %f %f\n",
							interp2, ex2.x, ex2.z);
	}*/

	/* at this point, ex1 and ex2 (X and Z) are the points on the sloped edges where
	   the quad crosses the overhang height... OR they are at one of the endpoints of
	   the edges. rasterize the relevant fragment of the quad. */

	if (interp1 == 0.0f) {
		if (interp2 == 0.0f) {
/*			if (IMPgTempDebugGrid) {
				Imp_PrintMessage(IMPcMsg_Important, "  entirely above overhang\n");
			}*/

			// the quad lies entirely above the overhang height
			return;
		} else if (interp2 == 1.0f) {
/*			if (IMPgTempDebugGrid) {
				Imp_PrintMessage(IMPcMsg_Important, "  PROBLEM CASE 1\n");
			}*/

			// this case should never happen - it means that st and t1 are both above
			// the overhang height, while ed and t2 are both below the overhang height.
			//
			// this does not reconcile with the given data that st and ed are the two
			// lowest points in the quad
			return;
		} else {
/*			if (IMPgTempDebugGrid) {
				Imp_PrintMessage(IMPcMsg_Important, "  triangle at ed\n");
			}*/

			// only one corner (ed) lies below the overhang height. calculate the
			// point along the bottom vector.
			dy = ed->y - st->y;
			interp3 = (overhang_height - st->y) / dy;
			UUmAssert((interp3 >= 0.0f) && (interp3 <= 1.0f));
			ex3.x = st->x + interp3 * (ed->x - st->x);
			ex3.z = st->z + interp3 * (ed->z - st->z);

			// rasterize the triangle ex3, ex2, ed.
			IMPmGetGridPoint(p1, ex3, room);
			IMPmGetGridPoint(p2, ex2, room);
			IMPmGetGridPoint(p3, *ed, room);

			PHrBresenhamAA(p1.x, p1.y, p2.x, p2.y,
						PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

			PHrBresenhamAA(p2.x, p2.y, p3.x, p3.y,
						PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

			if (fill_solid) {
				PHrBresenhamAA(p3.x, p3.y, p1.x, p1.y,
							PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

				PHrRasterizeTriangle(&p1, &p2, &p3, fill.weight, UUcTrue);
			}
		}
	} else if (interp1 == 1.0f) {
		if (interp2 == 0.0f) {
/*			if (IMPgTempDebugGrid) {
				Imp_PrintMessage(IMPcMsg_Important, "  PROBLEM CASE 2\n");
			}*/

			// this case should never happen - it means that st and t1 are both above
			// the overhang height, while ed and t2 are both below the overhang height.
			//
			// this does not reconcile with the given data that st and ed are the two
			// lowest points in the quad
			return;
		} else if (interp2 == 1.0f) {
/*			if (IMPgTempDebugGrid) {
				Imp_PrintMessage(IMPcMsg_Important, "  rasterize whole quad\n");
			}*/

			// the quad lies entirely below the overhang height. rasterize the whole thing.
			IMPmGetGridPoint(p1, *st, room);
			IMPmGetGridPoint(p2, *t1, room);
			IMPmGetGridPoint(p3, *t2, room);
			IMPmGetGridPoint(p4, *ed, room);

			PHrBresenhamAA(p1.x, p1.y, p2.x, p2.y,
						PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

			PHrBresenhamAA(p2.x, p2.y, p3.x, p3.y,
						PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

			PHrBresenhamAA(p3.x, p3.y, p4.x, p4.y,
						PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

			if (fill_solid) {
				PHrBresenhamAA(p4.x, p4.y, p1.x, p1.y,
							PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

				PHrRasterizeTriangle(&p1, &p2, &p3, fill.weight, UUcTrue);
				PHrRasterizeTriangle(&p1, &p3, &p4, fill.weight, UUcTrue);
			}
		} else {
/*			if (IMPgTempDebugGrid) {
				Imp_PrintMessage(IMPcMsg_Important, "  cut corner on t2\n");
			}*/

			// one edge of the quad lies entirely below the overhang height, the other
			// crosses it somewhere along its edge. create a new point along the top edge
			// where it crosses the overhang height, and rasterize a pentagon.
			dy = t2->y - t1->y;
			interp3 = (overhang_height - t1->y) / dy;
			UUmAssert((interp3 >= 0.0f) && (interp3 <= 1.0f));
			ex3.x = t1->x + interp3 * (t2->x - t1->x);
			ex3.z = t1->z + interp3 * (t2->z - t1->z);

			IMPmGetGridPoint(p1, *st, room);
			IMPmGetGridPoint(p2, *t1, room);
			IMPmGetGridPoint(p3, ex3, room);
			IMPmGetGridPoint(p4, ex2, room);
			IMPmGetGridPoint(p5, *ed, room);

			PHrBresenhamAA(p1.x, p1.y, p2.x, p2.y,
						PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

			PHrBresenhamAA(p2.x, p2.y, p3.x, p3.y,
						PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

			PHrBresenhamAA(p3.x, p3.y, p4.x, p4.y,
						PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

			PHrBresenhamAA(p4.x, p4.y, p5.x, p5.y,
						PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

			if (fill_solid) {
				PHrBresenhamAA(p5.x, p5.y, p1.x, p1.y,
							PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

				PHrRasterizeTriangle(&p1, &p2, &p3, fill.weight, UUcTrue);
				PHrRasterizeTriangle(&p1, &p3, &p4, fill.weight, UUcTrue);
				PHrRasterizeTriangle(&p1, &p4, &p5, fill.weight, UUcTrue);
			}
		}
	} else {
		if (interp2 == 0.0f) {
/*			if (IMPgTempDebugGrid) {
				Imp_PrintMessage(IMPcMsg_Important, "  triangle at st\n");
			}*/

			// only one corner (st) lies below the overhang height. calculate the
			// point along the bottom vector.
			dy = ed->y - st->y;
			interp3 = (overhang_height - st->y) / dy;
			UUmAssert((interp3 >= 0.0f) && (interp3 <= 1.0f));
			ex3.x = st->x + interp3 * (ed->x - st->x);
			ex3.z = st->z + interp3 * (ed->z - st->z);

			// rasterize the triangle st, ex1, ex3.
			IMPmGetGridPoint(p1, *st, room);
			IMPmGetGridPoint(p2, ex1, room);
			IMPmGetGridPoint(p3, ex3, room);

			PHrBresenhamAA(p1.x, p1.y, p2.x, p2.y,
						PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

			PHrBresenhamAA(p2.x, p2.y, p3.x, p3.y,
						PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

			if (fill_solid) {
				PHrBresenhamAA(p3.x, p3.y, p1.x, p1.y,
							PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

				PHrRasterizeTriangle(&p1, &p2, &p3, fill.weight, UUcTrue);
			}
		} else if (interp2 == 1.0f) {
/*			if (IMPgTempDebugGrid) {
				Imp_PrintMessage(IMPcMsg_Important, "  cut corner on t1\n");
			}*/

			// one edge of the quad lies entirely below the overhang height, the other
			// crosses it somewhere along its edge. create a new point along the top edge
			// where it crosses the overhang height, and rasterize a pentagon.
			dy = t2->y - t1->y;
			interp3 = (overhang_height - t1->y) / dy;
			UUmAssert((interp3 >= 0.0f) && (interp3 <= 1.0f));
			ex3.x = t1->x + interp3 * (t2->x - t1->x);
			ex3.z = t1->z + interp3 * (t2->z - t1->z);

			IMPmGetGridPoint(p1, *st, room);
			IMPmGetGridPoint(p2, ex1, room);
			IMPmGetGridPoint(p3, ex3, room);
			IMPmGetGridPoint(p4, *t2, room);
			IMPmGetGridPoint(p5, *ed, room);

			PHrBresenhamAA(p1.x, p1.y, p2.x, p2.y,
						PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

			PHrBresenhamAA(p2.x, p2.y, p3.x, p3.y,
						PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

			PHrBresenhamAA(p3.x, p3.y, p4.x, p4.y,
						PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

			PHrBresenhamAA(p4.x, p4.y, p5.x, p5.y,
						PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

			if (fill_solid) {
				PHrBresenhamAA(p5.x, p5.y, p1.x, p1.y,
							PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

				PHrRasterizeTriangle(&p1, &p2, &p3, fill.weight, UUcTrue);
				PHrRasterizeTriangle(&p1, &p3, &p4, fill.weight, UUcTrue);
				PHrRasterizeTriangle(&p1, &p4, &p5, fill.weight, UUcTrue);
			}
		} else {
/*			if (IMPgTempDebugGrid) {
				Imp_PrintMessage(IMPcMsg_Important, "  cut both edges\n");
			}*/

			// both edges of the quad cross the overhang height somewhere along
			// their lengths. rasterize the fraction of the quad that lies below
			// this line.
			IMPmGetGridPoint(p1, *st, room);
			IMPmGetGridPoint(p2, ex1, room);
			IMPmGetGridPoint(p3, ex2, room);
			IMPmGetGridPoint(p4, *ed, room);

			PHrBresenhamAA(p1.x, p1.y, p2.x, p2.y,
						PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

			PHrBresenhamAA(p2.x, p2.y, p3.x, p3.y,
						PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

			PHrBresenhamAA(p3.x, p3.y, p4.x, p4.y,
						PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

			if (fill_solid) {
				PHrBresenhamAA(p4.x, p4.y, p1.x, p1.y,
							PHgTempGrid,PHgTempGridX,PHgTempGridY,&fill,&blend,callback);

				PHrRasterizeTriangle(&p1, &p2, &p3, fill.weight, UUcTrue);
				PHrRasterizeTriangle(&p1, &p3, &p4, fill.weight, UUcTrue);
			}
		}
	}
#endif
}

#define PHmFatDot_PutCell(x, y, value, callback) {											\
	if (callback != NULL) {																	\
		(*callback)(x, y, (value)->weight);													\
	}																						\
	PHrCheckAndPutCell(grid,(UUtUns16)height,(UUtUns16)width,x,y,value);					\
}

void PHrFatDot(
	UUtUns16 x, 
	UUtUns16 y, 
	PHtSquare *grid, 
	UUtInt32 inWidth, 
	UUtInt32 inHeight,
	PHtRasterizationCallback callback)	
{
	/**************
	* Draws a fat dot in the grid at (x,y), clipping that
	* dot to the grid edges.
	*/

	UUtInt32 width = inWidth;
	UUtInt32 height = inHeight;
	PHtSquare blocked = { PHcImpassable };
	PHtSquare tight = { PHcSemiPassable };

	PHmFatDot_PutCell(x,y,&blocked,callback);
	
	PHmFatDot_PutCell(x-1,y,&tight,callback);
	PHmFatDot_PutCell(x+1,y,&tight,callback);
	PHmFatDot_PutCell(x,y-1,&tight,callback);
	PHmFatDot_PutCell(x,y+1,&tight,callback);
	
	PHmFatDot_PutCell(x-1,y-1,&tight,callback);
	PHmFatDot_PutCell(x+1,y-1,&tight,callback);
	PHmFatDot_PutCell(x+1,y+1,&tight,callback);
	PHmFatDot_PutCell(x-1,y+1,&tight,callback);
}


void IMPiCalculateNodeOrigins(IMPtEnv_BuildData *inBuildData, IMPtEnv_BNV *inNode, M3tPoint3D *outOrigin, M3tPoint3D *outAntiOrigin)
{
	/*******************
	* Calculates the upper left and bottom right corners of the node
	*/
	
	float low1,low2,low3,ly;
	IMPtEnv_BNV_Side*	curSide;
	M3tQuad *quad;
	M3tPoint3D *points = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);
	M3tQuad *quads = AUrSharedQuadArray_GetList(inBuildData->sharedBNVQuadArray);
	UUtUns16 q,c,curSideIndex;
	
	// Find the origin of the node by looking for the bottom-upper-left corner of the BNV
	low1 = low2 = low3 = 1e9;
	
	for(curSideIndex = 0, curSide = inNode->sides;
		curSideIndex < inNode->numSides;
		curSideIndex++, curSide++)
	{
		for (q = 0; q < curSide->numBNVQuads; q++)
		{
			quad = &quads[curSide->bnvQuadList[q]];
			
			for (c=0; c<4; c++)
			{
				ly = points[quad->indices[c]].x;
				if (ly < low1)
				{
					outOrigin->x = ly;
					low1 = ly;
				}
				ly = points[quad->indices[c]].y;
				if (ly < low2)
				{
					outOrigin->y = ly;
					low2 = ly;
				}
				ly = points[quad->indices[c]].z;
				if (ly < low3)
				{
					outOrigin->z = ly;
					low3 = ly;
				}
			}
		}
	}
	
	// Find the opposite corner of the node (upper-top-right corner of the BNV)
	low1 = low2 = low3 = -1e9;
	
	for(curSideIndex = 0, curSide = inNode->sides;
		curSideIndex < inNode->numSides;
		curSideIndex++, curSide++)
	{
		for (q = 0; q < curSide->numBNVQuads; q++)
		{
			quad = &quads[curSide->bnvQuadList[q]];
			for (c=0; c<4; c++)
			{
				ly = points[quad->indices[c]].x;
				if (ly > low1)
				{
					outAntiOrigin->x = ly;
					low1 = ly;
				}
				ly = points[quad->indices[c]].y;
				if (ly > low2)
				{
					outAntiOrigin->y = ly;
					low2 = ly;
				}
				ly = points[quad->indices[c]].z;
				if (ly > low3)
				{
					outAntiOrigin->z = ly;
					low3 = ly;
				}
			}
		}
	}
}

static void PHrMarkBorderValues(UUtUns8 inBaseValue, UUtUns8 inSeedValue, UUtUns8 inMarkValue)
{
	UUtInt32 x,y;
	PHtSquare *scan_line = PHgTempGrid;

	for(y = 0; y < PHgTempGridY; y++)
	{
		for(x = 0; x < PHgTempGridX; x++) 
		{
			if (scan_line[x].weight == inSeedValue)
			{
				if (y > 0) {
					if (scan_line[x - PHgTempGridX].weight == inBaseValue) {
						scan_line[x - PHgTempGridX].weight = inMarkValue;
					}
				}

				if (y < (PHgTempGridY - 1)) {
					if (scan_line[x + PHgTempGridX].weight == inBaseValue) {
						scan_line[x + PHgTempGridX].weight = inMarkValue;
					}
				}

				if (x > 0) {
					if (scan_line[x-1].weight == inBaseValue) {
						scan_line[x-1].weight = inMarkValue;
					}
				}

				if (x < (PHgTempGridX - 1)) {
					if (scan_line[x+1].weight == inBaseValue) {
						scan_line[x+1].weight = inMarkValue;
					}
				}
			}
		}

		scan_line += PHgTempGridX;
	}
}

