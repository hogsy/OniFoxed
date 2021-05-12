 /*
	FILE:	BFW_Decal.c
	
	AUTHOR:	Jason Duncan, Chris Butcher
	
	CREATED: June 03, 2000
	
	PURPOSE: routines for decals
	
	Copyright 2000 bungie/microsoft
 */

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_Particle3.h"
#include "BFW_MathLib.h"
#include "BFW_BinaryData.h"
#include "BFW_Console.h"
#include "BFW_ScriptLang.h"
#include "BFW_Image.h"

#include "bfw_cseries.h"
#include "lrar_cache.h"

// CB: argh - this is needed so that we can access the environment, collide against it and damage it.
#include "Oni.h"
#include "Oni_GameState.h"
#include "Oni_GameStatePrivate.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// constants
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define P3cDecalNumClipBuffers					3
#define	P3cDecalClipBufferSize					16
#define	P3cDecalBuildBufferSize					512
#define P3cDecalMaxTriIndices					512
#define	P3cDynamicDecalBufferSize				100 * 1024
#define P3cMaxDecalEdges						64
#define P3cDecal_GQBitVectorSize				16
#define P3cDecal_MaxAdjacencies					16
#define P3cDecal_MaxSharedPlanes				16
#define P3cDecal_VertexSize						(sizeof(M3tVector3D) + sizeof(M3tTextureCoord) + sizeof(UUtUns32))
#define P3cDecal_MinimumSize					(sizeof(M3tDecalHeader) + 4 * P3cDecal_VertexSize + 8 * sizeof(UUtUns16))

#define P3cDecalPointFudge						0.05f
#define P3cDecal_OnPlaneTolerance				1e-02f
#define P3cDecal_EdgeOverlapTolerance			1e-03f
#define P3cDecal_CoplanarEdgeTolerance			1e-04f
#define P3cDecal_CoplanarAdjEdgeLinearTolerance	1e-02f
#define P3cDecal_CollinearEdgeOverlap			1e-02f
#define P3cDecal_ParallelEdgeDelta				1e-03f
#define P3cDecal_SeamTolerance					1e-03f
#define P3cDecal_SharedPointTolerance			0.1f
#define P3cDecal_SharedUVTolerance				1e-02f
#define P3cDecal_OffsetNormalLength				1e-03f
#define P3cDecal_EdgeClipTolerance				0.1f
#define P3cDecal_Reverse2SidedTolerance			1e-03f
#define P3cDecal_PlaneDenomTolerance			1e-06f

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// structures
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct P3tDecalPointBuffer {
	UUtUns32		num_points;
	UUtUns32		max_num_points;
	M3tPoint3D		*points;
	M3tTextureCoord	*uvs;
	UUtUns8			*clipcodes;
	UUtUns32		*shades;		// may be NULL for input buffers
} P3tDecalPointBuffer;

typedef struct P3tDecalEdge {
	UUtBool			in_use, from_clip, to_clip;
	UUtUns32		traverse_depth;
	UUtUns32		from_index, from_gqindex;
	UUtUns32		to_index, to_gqindex;
	float			pos_edgeu, pos_edger;
	float			up_edgeu, up_edger;
	float			right_edgeu, right_edger;
	M3tPoint3D		edgept[2];
	float			edge_entryt, edge_exitt;
	M3tPoint3D		edge_decalmid;
	M3tVector3D		edgevec, parent_normal;
	UUtUns32		edgecode, child_edgecode;
	struct P3tDecalEdge *prev, *next;
} P3tDecalEdge;

typedef struct P3tDecalList {
	UUtUns32		num_decals;
	UUtUns16		next_traverse_id;
	UUtUns16		pad;
	P3tDecalData	*head;
	P3tDecalData	*tail;
} P3tDecalList;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// global variables
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// decal LRAR cache
lrar_cache			*P3gDecalCache= NULL;
UUtUns8				P3gDecalBuffer[P3cDynamicDecalBufferSize]= {0};

// global linked lists
P3tDecalList		P3gStaticDecals;
P3tDecalList		P3gDynamicDecals;

// global list of decal edges to be traversed
P3tDecalEdge		*P3gDecalNextEdge= NULL;
P3tDecalEdge		P3gDecalEdgeBuffer[P3cMaxDecalEdges]= {0};

// global settings and position for the current decal being constructed
UUtBool				P3gDecalDoLighting= 0;
M3tVector3D			P3gDecalDirection= {0};
M3tPoint3D			P3gDecalPosition= {0};
M3tVector3D			P3gDecalFwd= {0};
M3tVector3D			P3gDecalUp= {0};
M3tVector3D			P3gDecalRight= {0};
float				P3gDecalWrapDot= 0.f;

// temporary buffers for clipping and decal construction
P3tDecalPointBuffer	P3gDecalClipBuffer[P3cDecalNumClipBuffers]= {0};
P3tDecalPointBuffer	P3gDecalBuildBuffer= {0};
UUtUns32			*P3gDecalBuildCoincidentBV= NULL;
UUtUns32			*P3gDecalBuildDuplicateBV= NULL;
UUtUns32			*P3gDecalPlaneIndices= NULL;
UUtUns32			*P3gDecalBuildIndices= NULL;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// prototypes
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void P3iLRAR_NewProc(void *inUserData, short inBlockIndex);
static void P3iLRAR_PurgeProc(void *inUserData);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// initialization / termination
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void P3iDecalList_Initialize(P3tDecalList *ioDecalList)
{
	ioDecalList->num_decals = 0;
	ioDecalList->next_traverse_id = 1;
	ioDecalList->head = NULL;
	ioDecalList->tail = NULL;
}

UUtError P3rDecalInitialize(void)
{
	UUtUns32 i, blocks;

	// set up decal cache
	blocks = (P3cDynamicDecalBufferSize / P3cDecal_MinimumSize) + 10;
	P3gDecalCache		= lrar_new( "decal buffer", 0, P3cDynamicDecalBufferSize-1, (UUtUns16) blocks, 2, -1, P3iLRAR_NewProc, P3iLRAR_PurgeProc );

	// set up temporary decal clipping buffers
	for (i = 0; i < P3cDecalNumClipBuffers; i++) {
		P3gDecalClipBuffer[i].num_points = 0;
		P3gDecalClipBuffer[i].max_num_points = P3cDecalClipBufferSize;

		P3gDecalClipBuffer[i].points = UUrMemory_Block_New(P3cDecalClipBufferSize * sizeof(M3tPoint3D));
		UUmError_ReturnOnNull(P3gDecalClipBuffer[i].points);

		P3gDecalClipBuffer[i].uvs = UUrMemory_Block_New(P3cDecalClipBufferSize * sizeof(M3tTextureCoord));
		UUmError_ReturnOnNull(P3gDecalClipBuffer[i].uvs);

		P3gDecalClipBuffer[i].shades = UUrMemory_Block_New(P3cDecalClipBufferSize * sizeof(UUtUns32));
		UUmError_ReturnOnNull(P3gDecalClipBuffer[i].shades);

		P3gDecalClipBuffer[i].clipcodes = UUrMemory_Block_New(P3cDecalClipBufferSize * sizeof(UUtUns8));
		UUmError_ReturnOnNull(P3gDecalClipBuffer[i].clipcodes);
	}

	// set up temporary decal building buffer
	P3gDecalBuildBuffer.num_points = 0;
	P3gDecalBuildBuffer.max_num_points = P3cDecalBuildBufferSize;

	P3gDecalBuildBuffer.points = UUrMemory_Block_New(P3cDecalBuildBufferSize * sizeof(M3tPoint3D));
	UUmError_ReturnOnNull(P3gDecalBuildBuffer.points);

	P3gDecalBuildBuffer.uvs = UUrMemory_Block_New(P3cDecalBuildBufferSize * sizeof(M3tTextureCoord));
	UUmError_ReturnOnNull(P3gDecalBuildBuffer.uvs);

	P3gDecalBuildBuffer.shades = UUrMemory_Block_New(P3cDecalBuildBufferSize * sizeof(UUtUns32));
	UUmError_ReturnOnNull(P3gDecalBuildBuffer.shades);

	P3gDecalBuildBuffer.clipcodes = UUrMemory_Block_New(P3cDecalBuildBufferSize * sizeof(UUtUns8));
	UUmError_ReturnOnNull(P3gDecalBuildBuffer.clipcodes);

	P3gDecalBuildCoincidentBV = UUrBitVector_New(P3cDecalBuildBufferSize);
	UUmError_ReturnOnNull(P3gDecalBuildCoincidentBV);

	P3gDecalBuildDuplicateBV = UUrBitVector_New(P3cDecalBuildBufferSize);
	UUmError_ReturnOnNull(P3gDecalBuildDuplicateBV);

	P3gDecalPlaneIndices = UUrMemory_Block_New(P3cDecalBuildBufferSize * sizeof(UUtUns32));
	UUmError_ReturnOnNull(P3gDecalPlaneIndices);

	P3gDecalBuildIndices = UUrMemory_Block_New(P3cDecalBuildBufferSize * sizeof(UUtUns32));
	UUmError_ReturnOnNull(P3gDecalBuildIndices);

	// set up decal edge buffer
	P3gDecalNextEdge = NULL;
	UUrMemory_Clear(P3gDecalEdgeBuffer, P3cMaxDecalEdges * sizeof(P3tDecalEdge));

	return UUcError_None;
}

void P3rDecalTerminate(void)
{
	UUtUns32 i;

	lrar_dispose(P3gDecalCache);

	for (i = 0; i < P3cDecalNumClipBuffers; i++) {
		UUrMemory_Block_Delete(P3gDecalClipBuffer[i].points);
		UUrMemory_Block_Delete(P3gDecalClipBuffer[i].uvs);
		UUrMemory_Block_Delete(P3gDecalClipBuffer[i].shades);
		UUrMemory_Block_Delete(P3gDecalClipBuffer[i].clipcodes);
	}

	UUrMemory_Block_Delete(P3gDecalBuildBuffer.points);
	UUrMemory_Block_Delete(P3gDecalBuildBuffer.uvs);
	UUrMemory_Block_Delete(P3gDecalBuildBuffer.shades);
	UUrMemory_Block_Delete(P3gDecalBuildBuffer.clipcodes);

	UUrMemory_Block_Delete(P3gDecalBuildDuplicateBV);
	UUrMemory_Block_Delete(P3gDecalBuildCoincidentBV);
	UUrMemory_Block_Delete(P3gDecalPlaneIndices);
	UUrMemory_Block_Delete(P3gDecalBuildIndices);
}

void P3rDecal_LevelBegin(void)
{
	P3iDecalList_Initialize(&P3gStaticDecals);
	P3iDecalList_Initialize(&P3gDynamicDecals);
}

void P3rDecalDeleteAllDynamic(void)
{
	lrar_flush(P3gDecalCache);
	P3iDecalList_Initialize(&P3gDynamicDecals);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// decal list management
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(DEBUGGING) && DEBUGGING
static void P3iVerifyDecalList(P3tDecalList *inList)
{
	P3tDecalData *decalptr, *prev_ptr;
	UUtUns32 num_decals;

	num_decals = 0;
	if (inList->next_traverse_id == UUcMaxUns16) {
		inList->next_traverse_id = 1;
	} else {
		inList->next_traverse_id++;
	}

	prev_ptr = NULL;
	decalptr = inList->head;

	while (decalptr != NULL) {
		UUmAssertReadPtr(decalptr, sizeof(P3tDecalData));

		// ensure we have no circular links in this list
		UUmAssert(decalptr->traverse_id != inList->next_traverse_id);
		decalptr->traverse_id = inList->next_traverse_id;

		UUmAssert(decalptr->prev == prev_ptr);
		if (decalptr->prev == NULL) {
			UUmAssert(inList->head == decalptr);
		} else {
			UUmAssertReadPtr(decalptr->prev, sizeof(P3tDecalData));
			UUmAssert(decalptr->prev->next == decalptr);
		}

		if (decalptr->next == NULL) {
			UUmAssert(inList->tail == decalptr);
		} else {
			UUmAssertReadPtr(decalptr->next, sizeof(P3tDecalData));
			UUmAssert(decalptr->next->prev == decalptr);
		}

		num_decals++;
		prev_ptr = decalptr;
		decalptr = decalptr->next;
	}

	UUmAssert(inList->num_decals == num_decals);
}

void P3rVerifyDecalLists(void)
{
	P3iVerifyDecalList(&P3gDynamicDecals);
	P3iVerifyDecalList(&P3gStaticDecals);
}
#endif

static void P3iAddDecalToList(P3tDecalList *ioList, P3tDecalData *inDecal)
{
	P3mVerifyDecals();
	UUmAssertReadPtr(inDecal, sizeof(P3tDecalData));

	inDecal->prev = ioList->tail;
	if (inDecal->prev == NULL) {
		ioList->head = inDecal;
	} else {
		inDecal->prev->next = inDecal;
	}

	inDecal->next = NULL;
	ioList->tail = inDecal;

	ioList->num_decals++;
	P3mVerifyDecals();
}

static void P3iRemoveDecalFromList(P3tDecalList *ioList, P3tDecalData *inDecal)
{
	P3mVerifyDecals();
	UUmAssertReadPtr(inDecal, sizeof(P3tDecalData));

	if (inDecal->prev == NULL) {
		UUmAssert(ioList->head == inDecal);
		ioList->head = inDecal->next;
	} else {
		UUmAssert(inDecal->prev->next == inDecal);
		inDecal->prev->next = inDecal->next;
	}

	if (inDecal->next == NULL) {
		UUmAssert(ioList->tail == inDecal);
		ioList->tail = inDecal->prev;
	} else {
		UUmAssert(inDecal->next->prev == inDecal);
		inDecal->next->prev = inDecal->prev;
	}

	inDecal->prev = NULL;
	inDecal->next = NULL;

	UUmAssert(ioList->num_decals > 0);
	ioList->num_decals--;
	P3mVerifyDecals();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// polygon clipping
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void P3iDecal_LinePlaneIntersection(M3tPoint3D *inPoint0, M3tPoint3D *inPoint1, M3tTextureCoord *inUV0, M3tTextureCoord *inUV1,
										M3tPlaneEquation *inPlane, M3tPoint3D *inPlanePoint, M3tVector3D *inPlaneLine, float inPlaneLineDot,
										UUtUns32 inPlaneShade0, UUtUns32 inPlaneShade1, M3tPoint3D *outPoint, M3tTextureCoord *outUV, UUtUns32 *outShade)
{
	M3tVector3D		inLine, delta_vec;
	float			denom, t, one_minus_t, interp;
	
	MUmVector_Subtract(inLine, *inPoint1, *inPoint0);
	
	denom = inPlane->a * inLine.x + inPlane->b * inLine.y + inPlane->c * inLine.z;
	if ((float)fabs(denom) < P3cDecal_PlaneDenomTolerance) {
		// this is done purely because we _must_ return a value
		t = 0.5f;
	} else {
		t = -(inPlane->a * inPoint0->x + inPlane->b * inPoint0->y + inPlane->c * inPoint0->z + inPlane->d) / denom;
		t = UUmPin(t, 0.0f, 1.0f);
	}
	one_minus_t = 1.0f - t;

	if (outPoint != NULL) {
		MUmVector_Copy(*outPoint, *inPoint0);
		MUmVector_ScaleIncrement(*outPoint, t, inLine);
	}
	
	if (outUV != NULL) {
		outUV->u = inUV1->u * t + inUV0->u * one_minus_t;
		outUV->v = inUV1->v * t + inUV0->v * one_minus_t;
	}

	if (outShade != NULL) {
		UUmAssert(outPoint != NULL);

		// determine whereabouts this point lies along the plane's defining line
		MUmVector_Subtract(delta_vec, *outPoint, *inPlanePoint);
		interp = MUrVector_DotProduct(&delta_vec, inPlaneLine) / inPlaneLineDot;
		interp = UUmPin(interp, 0.0f, 1.0f);

		*outShade = IMrShade_Interpolate(inPlaneShade0, inPlaneShade1, interp);
	}
}

static UUtUns32 P3iDecal_ClipToPlane(UUtUns8 inClipCode, M3tPoint3D *inPoint0, M3tPoint3D *inPoint1,
									 UUtUns32 inShade0, UUtUns32 inShade1, M3tPoint3D *inSidePoint,
								 P3tDecalPointBuffer *inBuffer, P3tDecalPointBuffer *outBuffer)
{
	UUtUns32					j;
	M3tVector3D					linevec, sidevec, upvec, normal;
	M3tPoint3D					point0, point1;
	M3tPlaneEquation			clip_plane;
	M3tTextureCoord				uv0, uv1;
	UUtUns8						clipcode0, clipcode1;
	UUtBool						inside0, inside1;
	UUtUns32					orig_num_points;
	float						line_dot_product;

	orig_num_points = outBuffer->num_points;
	if (orig_num_points >= outBuffer->max_num_points) {
		// can't add any more to this buffer
		return 0;
	}

	// build the plane equation to clip against
	MUmVector_Subtract(linevec, *inPoint1, *inPoint0);
	MUmVector_Subtract(sidevec, *inSidePoint, *inPoint1);
	MUrVector_CrossProduct(&linevec, &sidevec, &upvec);
	MUrVector_CrossProduct(&linevec, &upvec, &normal);		// note: normal points in the opposite direction to sidevec
	if (MUrVector_Normalize_ZeroTest(&normal)) {
		// one of linevec and sidevec must be very small... in other words the
		// line is degenerate. don't clip against this plane. just copy the points
		// across to the output buffer unchanged.
		for (j = 0; j < inBuffer->num_points; j++) {
			outBuffer->points	[outBuffer->num_points] = inBuffer->points		[j];
			outBuffer->uvs		[outBuffer->num_points] = inBuffer->uvs			[j];
			outBuffer->clipcodes[outBuffer->num_points] = inBuffer->clipcodes	[j];
			outBuffer->num_points++;

			if (outBuffer->num_points >= outBuffer->max_num_points) {
				UUrDebuggerMessage("P3iDecal_ClipToPlane: decal has more than %d points, overflowed output buffer", outBuffer->max_num_points);
				break;
			}
		}

		return (outBuffer->num_points - orig_num_points);
	}
	clip_plane.a = normal.x;
	clip_plane.b = normal.y;
	clip_plane.c = normal.z;
	clip_plane.d = -MUrVector_DotProduct(&normal, inPoint1);
	line_dot_product = MUrVector_DotProduct(&linevec, &linevec);

	// set up for the first clipped line (n-1/0)
	point0					= inBuffer->points[inBuffer->num_points - 1];
	uv0						= inBuffer->uvs[inBuffer->num_points - 1];
	clipcode0				= inBuffer->clipcodes[inBuffer->num_points - 1];
	inside0					= MUrPoint_PointBehindPlane(&point0, &clip_plane);

	for(j = 0; j < inBuffer->num_points; j++) {
		point1				= inBuffer->points[j];
		uv1					= inBuffer->uvs[j];
		clipcode1			= inBuffer->clipcodes[j];
		inside1				= MUrPoint_PointBehindPlane(&point1, &clip_plane);

		if (inside0) {
			if (inside1) {
				// the line is inside the clip plane
				outBuffer->points	[outBuffer->num_points] = point1;
				outBuffer->uvs		[outBuffer->num_points] = uv1;
				outBuffer->clipcodes[outBuffer->num_points] = clipcode1;
				if (P3gDecalDoLighting) {
					outBuffer->shades	[outBuffer->num_points] = inBuffer->shades[j];
				}
				outBuffer->num_points++;
			} else {
				// the line exits the clip plane - point0 will have been added last loop
				P3iDecal_LinePlaneIntersection(&point0, &point1, &uv0, &uv1,
										&clip_plane, inPoint0, &linevec, line_dot_product, inShade0, inShade1,
										&outBuffer->points[outBuffer->num_points], &outBuffer->uvs[outBuffer->num_points],
										P3gDecalDoLighting ? &outBuffer->shades[outBuffer->num_points] : NULL);
				outBuffer->clipcodes[outBuffer->num_points] = (clipcode0 & clipcode1) | inClipCode;
				outBuffer->num_points++;
			}
		} else {
			if (inside1) {
				// the line enters the clip plane
				P3iDecal_LinePlaneIntersection(&point0, &point1, &uv0, &uv1,
										&clip_plane, inPoint0, &linevec, line_dot_product, inShade0, inShade1,
										&outBuffer->points[outBuffer->num_points], &outBuffer->uvs[outBuffer->num_points],
										P3gDecalDoLighting ? &outBuffer->shades[outBuffer->num_points] : NULL);
				outBuffer->clipcodes[outBuffer->num_points] = (clipcode0 & clipcode1) | inClipCode;
				outBuffer->num_points++;

				if (outBuffer->num_points >= outBuffer->max_num_points) {
					UUrDebuggerMessage("P3iDecal_ClipToPlane: decal has more than %d points, overflowed output buffer", outBuffer->max_num_points);
					break;
				}

				// add the end point to the buffer
				outBuffer->points	[outBuffer->num_points] = point1;
				outBuffer->uvs		[outBuffer->num_points] = uv1;
				outBuffer->clipcodes[outBuffer->num_points] = clipcode1;
				if (P3gDecalDoLighting) {
					outBuffer->shades	[outBuffer->num_points] = inBuffer->shades[j];
				}
				outBuffer->num_points++;
			}
		}
		if (outBuffer->num_points >= outBuffer->max_num_points) {
			UUrDebuggerMessage("P3iDecal_ClipToPlane: decal has more than %d points, overflowed output buffer", outBuffer->max_num_points);
			break;
		}
		uv0			= uv1;
		point0		= point1;
		inside0		= inside1;
		clipcode0	= clipcode1;
	}

	UUmAssert(outBuffer->num_points <= outBuffer->max_num_points);
	UUmAssert(outBuffer->num_points >= orig_num_points);
	return (outBuffer->num_points - orig_num_points);
}

static UUtUns32 P3iDecal_ClipToPolygon(UUtUns32 inPolygonNumPoints, M3tPoint3D *inPolygonPoints, UUtUns32 *inPolygonShades,
										M3tPlaneEquation *inPlaneEquation, P3tDecalPointBuffer *inSourceBuffer,
										UUtUns32 *outClipCode, P3tDecalPointBuffer *ioDestBuffer)
{
	UUtUns32					i, cur_edge, clipcode;
	UUtUns32					orig_num_points;

	// the plane to clip against
	M3tPoint3D					clip0, clip1, clip2;
	UUtUns32					shade0, shade1, shade2;

	// point buffers
	P3tDecalPointBuffer			inputbuf;
	P3tDecalPointBuffer			outputbuf;

	inputbuf = *inSourceBuffer;
	orig_num_points = ioDestBuffer->num_points;
	clipcode = 0;

	// set up for the first clip plane
	clip0 = inPolygonPoints[inPolygonNumPoints - 2];		shade0 = inPolygonShades[inPolygonNumPoints - 2];
	clip1 = inPolygonPoints[inPolygonNumPoints - 1];		shade1 = inPolygonShades[inPolygonNumPoints - 1];
	cur_edge = inPolygonNumPoints - 2;

	UUrMemory_Clear(inputbuf.clipcodes, inputbuf.num_points * sizeof(UUtUns8));

	for(i = 0; i < inPolygonNumPoints; i++) {
		// get the second point on the clip plane
		clip2 = inPolygonPoints[i];							shade2 = inPolygonShades[i];

		// determine the output buffer to write to
		if (i == inPolygonNumPoints - 1) {
			outputbuf = *ioDestBuffer;
			// NOTE: leave outputbuf's num_points unchanged as we may be adding to the end of the buffer
		} else {
			outputbuf = P3gDecalClipBuffer[i % 2];
			outputbuf.num_points = 0;
		}

		P3iDecal_ClipToPlane((1 << cur_edge), &clip0, &clip1, shade0, shade1, &clip2, &inputbuf, &outputbuf);

		clip0 = clip1;		shade0 = shade1;
		clip1 = clip2;		shade1 = shade2;
		cur_edge = (cur_edge + 1) % inPolygonNumPoints;

		if (i == inPolygonNumPoints - 1) {
			// store final number of points in the output buffer
			ioDestBuffer->num_points = outputbuf.num_points;
		} else {
			inputbuf = outputbuf;
		}
	}

	if (outClipCode != NULL) {
		// calculate the final clipcode by OR'ing together the clip codes
		// for each of the final points
		clipcode = 0;
		for (i = orig_num_points; i < ioDestBuffer->num_points; i++) {
			clipcode |= ioDestBuffer->clipcodes[i];
		}
		*outClipCode |= clipcode;
	}

	UUmAssert(ioDestBuffer->num_points <= ioDestBuffer->max_num_points);
	UUmAssert(ioDestBuffer->num_points >= orig_num_points);
	return (ioDestBuffer->num_points - orig_num_points);
}

static void P3iDecal_BarycentricShadeInterpolate(M3tPoint3D *inPoint0, M3tPoint3D *inPoint1, M3tPoint3D *inPoint2,
												 M3tVector3D *inEdge0, M3tVector3D *inEdge1, M3tVector3D *inEdge2,
												UUtUns32 inShade0, UUtUns32 inShade1, UUtUns32 inShade2, M3tVector3D *inNormal,
												M3tPoint3D *inSample, UUtUns32 *outShade)
{
	float r, g, b;
	float bary0, bary1, bary2, barysum;
	UUtInt16 rv, gv, bv;
	M3tVector3D delta_vec, area_vec;

	// calculate barycentric coordinates for the point relative to the triangle. note that
	// edge 0 lies opposite point 2, edge 1 opposite point 0, edge 2 opposite point 1
	MUmVector_Subtract(delta_vec, *inSample, *inPoint0);
	MUrVector_CrossProduct(inEdge0, &delta_vec, &area_vec);
	bary2 = MUrVector_DotProduct(inNormal, &area_vec);

	MUmVector_Subtract(delta_vec, *inSample, *inPoint1);
	MUrVector_CrossProduct(inEdge1, &delta_vec, &area_vec);
	bary0 = MUrVector_DotProduct(inNormal, &area_vec);

	MUmVector_Subtract(delta_vec, *inSample, *inPoint2);
	MUrVector_CrossProduct(inEdge2, &delta_vec, &area_vec);
	bary1 = MUrVector_DotProduct(inNormal, &area_vec);

	// interpolate the shades of the triangle
	r  = bary0 * ((inShade0 & 0x00FF0000) >> 16);
	g  = bary0 * ((inShade0 & 0x0000FF00) >>  8);
	b  = bary0 * ((inShade0 & 0x000000FF)      );
	r += bary1 * ((inShade1 & 0x00FF0000) >> 16);
	g += bary1 * ((inShade1 & 0x0000FF00) >>  8);
	b += bary1 * ((inShade1 & 0x000000FF)      );
	r += bary2 * ((inShade2 & 0x00FF0000) >> 16);
	g += bary2 * ((inShade2 & 0x0000FF00) >>  8);
	b += bary2 * ((inShade2 & 0x000000FF)      );
	barysum = bary0 + bary1 + bary2;

	if (r < 0) {
		rv = 0;
	} else {
		rv = (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(r / barysum);
		if (rv > 255) {
			rv = 255;
		}
	}

	if (g < 0) {
		gv = 0;
	} else {
		gv = (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(g / barysum);
		if (gv > 255) {
			gv = 255;
		}
	}

	if (b < 0) {
		bv = 0;
	} else {
		bv = (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(b / barysum);
		if (bv > 255) {
			bv = 255;
		}
	}

	*outShade = (((UUtUns32) rv) << 16) | (((UUtUns32) gv) << 8) | ((UUtUns32) bv);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// decal edgelist management
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static P3tDecalEdge *P3iDecal_MakeNewEdge(UUtUns32 inTraverseDepth)
{
	UUtUns32 itr;
	P3tDecalEdge *edge = NULL, *ptr, *prevptr;

	for (itr = 0; itr < P3cMaxDecalEdges; itr++) {
		if (!P3gDecalEdgeBuffer[itr].in_use) {
			edge = &P3gDecalEdgeBuffer[itr];
			break;
		}
	}

	if (edge != NULL) {
		// insert this edge into the list waiting to be processed
		prevptr = NULL;
		for (ptr = P3gDecalNextEdge; (ptr != NULL) && (ptr->traverse_depth <= inTraverseDepth); prevptr = ptr, ptr = ptr->next) {
			UUmAssert(ptr->in_use);
		}

		edge->prev = prevptr;
		if (edge->prev == NULL) {
			// insert edge at head of list
			P3gDecalNextEdge = edge;
		} else {
			// insert edge into body of list
			edge->prev->next = edge;
		}
		edge->next = ptr;
		if (edge->next != NULL) {
			edge->next->prev = edge;
			UUmAssert(edge->next->traverse_depth > inTraverseDepth);
		}

		edge->in_use = UUcTrue;
	}

	return edge;
}

static void P3iDecal_RemoveEdge(P3tDecalEdge *inEdge)
{
	UUmAssert(inEdge->in_use);

	if (inEdge->prev == NULL) {
		UUmAssert(P3gDecalNextEdge == inEdge);
		P3gDecalNextEdge = inEdge->next;
	} else {
		UUmAssert(inEdge->prev->next == inEdge);
		inEdge->prev->next = inEdge->next;
	}

	if (inEdge->next != NULL) {
		UUmAssert(inEdge->next->prev == inEdge);
		inEdge->next->prev = inEdge->prev;
	}
	inEdge->in_use = UUcFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// decal traversal and creation
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static UUtBool P3iDecal_ProcessAdjacency(AKtEnvironment *inEnvironment, UUtUns32 inDecalFlags, P3tDecalEdge *inEdge,
										float inXScale, float inYScale, UUtUns32 *ioTriCount,
										UUtUns32 *ioNumTriIndices, UUtUns32 inMaxTriIndices, UUtUns16 *ioTriIndices)
{
	UUtUns32					itr, j, k;
	UUtBool						adjacent, negate, success = UUcTrue;

	// the quad that we are clipping to
	UUtUns32					gqindex;
	AKtGQ_General				*general_quad;
	AKtGQ_Collision				*collision_quad;
	M3tPlaneEquation			quad_plane;
	UUtUns32					num_quad_points, plane_index;
	UUtUns32					quad_num_pos, quad_num_neg, quad_num_zero;
	M3tPoint3D					quad_points[4];
	M3tVector3D					quad_edgevec[4];
	UUtUns32					quad_shades[4];
	float						quad_d[4];
	UUtUns8						quad_edge;				// bitvector over 4 points
	UUtUns32					quad_clipcode;

	// the quad that we are testing for adjacency
	AKtGQ_General				*adjacent_quad;
	AKtGQ_Collision				*adjacent_collisionquad;
	M3tPlaneEquation			adj_plane;
	UUtUns32					adjacent_gqindex, num_adjacent_points;
	UUtUns32					adj_num_pos, adj_num_neg, adj_num_zero, adj_swap;
	M3tPoint3D					adj_points[4];
	float						adj_d[4];
	UUtUns8						adj_edge;				// bitvector over 4 points

	// coplanar intersection variables
	M3tVector3D					delta_vec, cross_vec, cross_vec2, quad_cross_vec, adj_edgevec;
	float						t_length;
	M3tPoint3D					in_point;

	// overlap variables
	UUtBool						prev_edge;
	UUtUns32					found_p;
	UUtUns32					this_index, prev_index;
	M3tVector3D					line_n;
	float						quad_t[2], swap;
	M3tPoint3D					quad_p[2], swap_p;
	float						adj_t[2];
	M3tPoint3D					adj_p[2];

	// current adjacency being built
	UUtUns32					current_traverseindex;
	UUtUns32					current_edgecode, current_child_edgecode;
	UUtBool						current_from_clip, current_to_clip;
	M3tPoint3D					current_edgept[2];

	// the quads that we found adjacencies to
	UUtUns32					found_adjacencies;
	P3tDecalEdge *				current_adjacency;
	P3tDecalEdge *				adjacencies[P3cDecal_MaxAdjacencies];
	
	// source point buffer
	M3tPoint3D					unprojected_points[4];
	P3tDecalPointBuffer			source_buffer;
	M3tPoint3D					source_points[4];
	M3tTextureCoord				source_uvs[4];
	UUtUns8						source_clipcodes[4];
	UUtUns32					source_shades[4];

	// wrapping variables
	UUtBool						invalid_edge;
	M3tVector3D					edgebasis_u, edgebasis_r;
	float						edgeloc[2][2], edgedelta[2], edge_entryt[2], edge_exitt[2];
	M3tPoint3D					our_position;
	M3tVector3D					plane_norm;
	M3tVector3D					our_fwd_vector;
	M3tVector3D					our_up_vector;
	M3tVector3D					our_right_vector;

	// edge clipping variables
	UUtUns32					num_clip_edges, clipped_edges, num_remaining_points;
	P3tDecalPointBuffer			*inputbuf, *outputbuf;
	float						mid_t;
	M3tPoint3D					*parentedge_midpoint;

	// decal building variables
	UUtUns32					num_tri_indices;
	float						pt_plane_dist, fwd_dot_norm;
	UUtUns32					quad_point_base, quad_point_count, new_tris;
	M3tVector3D					quad_midvector, quad_reversemidvector;


	if (inEdge->from_index == (UUtUns32) -1) {
		// we have no parent
		found_adjacencies = 0;

	} else {
		// store the adjacency from our parent, as we will have to clip against it and use it for
		// seam fixing. note: we can modify the input edge because we are the only code that will access it!
		adjacencies[0] = inEdge;
		adjacencies[0]->to_index = (UUtUns32) -1;
		adjacencies[0]->from_clip = adjacencies[0]->to_clip;
		found_adjacencies = 1;
	}

	// get the points of this quad that we are clipping against
	gqindex = inEdge->to_gqindex;
	general_quad = inEnvironment->gqGeneralArray->gqGeneral + gqindex;
	num_quad_points = (general_quad->flags & AKcGQ_Flag_Triangle) ? 3 : 4;
	for (j = 0; j < num_quad_points; j++) {
		quad_points[j] = inEnvironment->pointArray->points[general_quad->m3Quad.vertexIndices.indices[j]];
	}

	// get the edges of this quad
	for (j = 0; j < num_quad_points; j++) {
		MUmVector_Subtract(quad_edgevec[j], quad_points[(j + 1) % num_quad_points], quad_points[j]);
	}

	// get the plane of this quad
	collision_quad = inEnvironment->gqCollisionArray->gqCollision + gqindex;
	AKmPlaneEqu_GetComponents(collision_quad->planeEquIndex, inEnvironment->planeArray->planes,
								quad_plane.a, quad_plane.b, quad_plane.c, quad_plane.d);
	negate = UUcFalse;
	if (general_quad->flags & (AKcGQ_Flag_2Sided & ~AKcGQ_Flag_Jello)) {
		if (inEdge->from_gqindex == (UUtUns32) -1) {
			negate = (quad_plane.a * P3gDecalFwd.x + quad_plane.b * P3gDecalFwd.y + quad_plane.c * P3gDecalFwd.z < -P3cDecal_Reverse2SidedTolerance);
		} else {
			negate = (quad_plane.a * inEdge->parent_normal.x + quad_plane.b * inEdge->parent_normal.y + quad_plane.c * inEdge->parent_normal.z
						< -P3cDecal_Reverse2SidedTolerance);
		}

		if (negate) {
			quad_plane.a = -quad_plane.a;
			quad_plane.b = -quad_plane.b;
			quad_plane.c = -quad_plane.c;
			quad_plane.d = -quad_plane.d;
		}
	}

	// calculate the decal's location on this new quad
	if (inEdge->from_gqindex == (UUtUns32) -1) {
		// we are the first quad to check the decal
		our_position = P3gDecalPosition;
		our_fwd_vector = P3gDecalFwd;
		our_up_vector = P3gDecalUp;
		our_right_vector = P3gDecalRight;
	} else {
		// calculate a right-angle basis system on the plane of this new quad
		our_fwd_vector.x = quad_plane.a;
		our_fwd_vector.y = quad_plane.b;
		our_fwd_vector.z = quad_plane.c;

		// form a right-angle basis system that uses the edge vector
		MUmVector_Subtract(edgebasis_u, inEdge->edgept[1], inEdge->edgept[0]);
		MUmVector_Normalize(edgebasis_u);
		plane_norm.x = quad_plane.a;
		plane_norm.y = quad_plane.b;
		plane_norm.z = quad_plane.c;
		MUrVector_CrossProduct(&edgebasis_u, &our_fwd_vector, &edgebasis_r);
		MUmVector_Normalize(edgebasis_r);

		// calculate the position, up and right vectors based on the stored information and this new basis system
		MUmVector_Copy(our_position, inEdge->edgept[0]);
		MUmVector_ScaleIncrement(our_position, inEdge->pos_edgeu, edgebasis_u);
		MUmVector_ScaleIncrement(our_position, inEdge->pos_edger, edgebasis_r);

		MUmVector_ScaleCopy(our_up_vector, inEdge->up_edgeu, edgebasis_u);
		MUmVector_ScaleIncrement(our_up_vector, inEdge->up_edger, edgebasis_r);

		MUmVector_ScaleCopy(our_right_vector, inEdge->right_edgeu, edgebasis_u);
		MUmVector_ScaleIncrement(our_right_vector, inEdge->right_edger, edgebasis_r);
	}

	// determine whether this quad is ordered counter-clockwise or not
	MUrVector_CrossProduct(&quad_edgevec[0], &quad_edgevec[1], &quad_cross_vec);

	for (itr = 0; itr < AKgNumCollisions; itr++) {
		// don't check our parent or the current quad for adjacencies
		if ((itr == inEdge->to_index) || (itr == inEdge->from_index)) {
			continue;
		}

		// check each quad in the collision list
		adjacent_gqindex = AKgCollisionList[itr].gqIndex;
		adjacent_quad = inEnvironment->gqGeneralArray->gqGeneral + adjacent_gqindex;
		adjacent_collisionquad = inEnvironment->gqCollisionArray->gqCollision + adjacent_gqindex;
		num_adjacent_points = (adjacent_quad->flags & AKcGQ_Flag_Triangle) ? 3 : 4;
		adjacent = UUcFalse;
		current_edgecode = current_child_edgecode = 0;

		if ((adjacent_collisionquad->planeEquIndex & 0x7FFFFFFF) == (collision_quad->planeEquIndex & 0x7FFFFFFF)) {
			// coplanar quads - we must check to see if they touch or overlap
			for (j = 0; j < num_adjacent_points; j++) {
				adj_points[j] = inEnvironment->pointArray->points[adjacent_quad->m3Quad.vertexIndices.indices[j]];
			}

			// we never clip the parent quad against any adjacencies with coplanar child quads; we will only clip the
			// child quad if we find actual non-collinear-edge intersections.
			current_from_clip = current_to_clip = UUcFalse;

			// coplanar, so don't increment traverse-index (number of planes crossed)
			current_traverseindex = inEdge->traverse_depth;

			found_p = 0;
			for (j = 0; j < num_adjacent_points; j++) {
				// get the edge to test
				MUmVector_Subtract(adj_edgevec, adj_points[(j + 1) % num_adjacent_points], adj_points[j]);

				// for each edge in the quad
				for (k = 0; k < num_quad_points; k++) {
					MUrVector_CrossProduct(&adj_edgevec, &quad_edgevec[k], &cross_vec);
					if (MUmVector_GetLengthSquared(cross_vec) < P3cDecal_CoplanarEdgeTolerance) {
						// the edges are parallel
						MUmVector_Subtract(delta_vec, quad_points[k], adj_points[j]);
						MUrVector_CrossProduct(&adj_edgevec, &delta_vec, &cross_vec);
						if (MUmVector_GetLengthSquared(cross_vec) > P3cDecal_CoplanarEdgeTolerance) {
							// the edges are parallel but not collinear, try the next quad edge
							continue;
						}

						// the edges are collinear, determine the relative locations of the edge endpoints along this line
						t_length = MUmVector_GetLengthSquared(adj_edgevec);
						quad_p[0] = quad_points[k];
						quad_t[0] = MUrVector_DotProduct(&delta_vec, &adj_edgevec);
						quad_p[1] = quad_points[(k + 1) % num_quad_points];
						MUmVector_Subtract(delta_vec, quad_p[1], adj_points[j]);
						quad_t[1] = MUrVector_DotProduct(&delta_vec, &adj_edgevec);

						if (quad_t[0] > quad_t[1]) {
							swap = quad_t[0];
							swap_p = quad_p[0];
							quad_t[0] = quad_t[1];
							quad_p[0] = quad_p[1];
							quad_t[1] = swap;
							quad_p[1] = swap_p;
						}

						if ((quad_t[1] < P3cDecal_CollinearEdgeOverlap) || (quad_t[0] > t_length - P3cDecal_CollinearEdgeOverlap)) {
							// the edges do not overlap along their shared line, go to the next edge of
							// the quad
							continue;
						}

						// the edges overlap here, this adjacency is on edge k of the quad and edge j of the child
						current_edgecode |= (1 << k);
						current_child_edgecode |= (1 << j);

						if (found_p < 2) {
							if (quad_t[0] < 0) {
								// edge starts at adj_points[j]
								current_edgept[found_p++] = adj_points[j];
							} else {
								// edge starts at quad_p[0]
								current_edgept[found_p++] = quad_p[0];
							}
						}
						if (found_p < 2) {
							if (quad_t[1] < t_length) {
								// edge ends at adj_points[(j + 1) % num_adjacent_points]
								current_edgept[found_p++] = adj_points[(j + 1) % num_adjacent_points];
							} else {
								// edge starts at quad_p[1]
								current_edgept[found_p++] = quad_p[1];
							}
						}
					} else {
						// the edges are not parallel, check to see if they intersect
						MUmVector_Subtract(delta_vec, quad_points[k], adj_points[j]);
						MUrVector_CrossProduct(&adj_edgevec, &delta_vec, &cross_vec);
						MUmVector_Subtract(delta_vec, quad_points[(k + 1) % num_quad_points], adj_points[j]);
						MUrVector_CrossProduct(&adj_edgevec, &delta_vec, &cross_vec2);

						if (MUrVector_DotProduct(&cross_vec, &cross_vec2) >= 0) {
							// the quad points don't span the adjacent quad's edge, there is no intersection on this edge of the quad.
							// there might be one at an endpoint, but that will be picked up elsewhere by the collinear-edge check.
							continue;
						}

						quad_t[0] = MUrVector_DotProduct(&cross_vec,  &quad_cross_vec);
						quad_t[1] = MUrVector_DotProduct(&cross_vec2, &quad_cross_vec);
						UUmAssert(quad_t[0] * quad_t[1] <= 1e-03f);

						// get the point of intersection by interpolating these t-values linearly to find zero
						MUmVector_ScaleCopy(in_point, quad_t[1] / (quad_t[1] - quad_t[0]), quad_points[k]);
						MUmVector_ScaleIncrement(in_point, -quad_t[0] / (quad_t[1] - quad_t[0]), quad_points[(k + 1) % num_quad_points]);

						// whereabouts is this point along the line of the adjacent edge?
						MUmVector_Subtract(delta_vec, in_point, adj_points[j]);
						adj_t[0] = MUrVector_DotProduct(&delta_vec, &adj_edgevec) / MUmVector_GetLengthSquared(adj_edgevec);

						if ((adj_t[0] < P3cDecal_CoplanarAdjEdgeLinearTolerance) || (adj_t[0] > 1.0f - P3cDecal_CoplanarAdjEdgeLinearTolerance)) {
							// the point of intersection does not lie on the adjacent edge, or it lies at an endpoint and so will be picked
							// up by the collinear check, or discarded
							continue;
						}

						// the edges overlap here, this adjacency is on edge k of the quad and edge j of the child
						current_edgecode |= (1 << k);
						current_child_edgecode |= (1 << j);

						if (found_p < 2) {
							current_edgept[found_p++] = in_point;
						}

						// this intersection means that we have to clip the child quad against the line of intersection
						current_to_clip = UUcTrue;
					}
				}
			}

			if (found_p < 2) {
				// could not find the edge points, there is an error - bail out of checking this quad
				continue;
			}

		} else {
			// not coplanar, so increment traverse-index (number of planes crossed)
			current_traverseindex = inEdge->traverse_depth + 1;

			// calculate pt-to-plane distances for each of the points on our current quad
			AKmPlaneEqu_GetComponents(adjacent_collisionquad->planeEquIndex, inEnvironment->planeArray->planes,
										adj_plane.a, adj_plane.b, adj_plane.c, adj_plane.d);

			// flip two-sided quads if they face away from our current quad
			if (adjacent_quad->flags & (AKcGQ_Flag_2Sided & ~AKcGQ_Flag_Jello)) {
				if (quad_plane.a * adj_plane.a + quad_plane.b * adj_plane.b + quad_plane.c * adj_plane.c < P3cDecal_Reverse2SidedTolerance) {
					adj_plane.a = -adj_plane.a;
					adj_plane.b = -adj_plane.b;
					adj_plane.c = -adj_plane.c;
					adj_plane.d = -adj_plane.d;
				}
			}

			quad_num_pos = quad_num_neg = quad_num_zero = 0;
			quad_edge = 0;
			for (j = 0; j < num_quad_points; j++) {
				// where does this point lie with respect to the adjacent plane?
				quad_d[j] = quad_points[j].x * adj_plane.a + quad_points[j].y * adj_plane.b + quad_points[j].z * adj_plane.c + adj_plane.d;
				if (quad_d[j] < -P3cDecal_OnPlaneTolerance) {
					quad_num_neg++;
				} else if (quad_d[j] > +P3cDecal_OnPlaneTolerance) {
					quad_num_pos++;
				} else {
					quad_num_zero++;
					quad_edge |= (1 << j);
				}
			}

			if ((quad_num_zero < 2) && ((!quad_num_pos) || (!quad_num_neg))) {
				// this quad does not cross the plane of the adjacent quad, no adjacency
				continue;
			}


			// calculate pt-to-plane distances for each of the points on the adjacent quad
			adj_num_pos = adj_num_neg = adj_num_zero = 0;
			adj_edge = 0;
			for (j = 0; j < num_adjacent_points; j++) {
				// where does this point lie with respect to the quad's plane?
				adj_points[j] = inEnvironment->pointArray->points[adjacent_quad->m3Quad.vertexIndices.indices[j]];
				adj_d[j] = adj_points[j].x * quad_plane.a + adj_points[j].y * quad_plane.b + adj_points[j].z * quad_plane.c + quad_plane.d;
				if (adj_d[j] < -P3cDecal_OnPlaneTolerance) {
					adj_num_neg++;
				} else if (adj_d[j] > +P3cDecal_OnPlaneTolerance) {
					adj_num_pos++;
				} else {
					adj_num_zero++;
					adj_edge |= (1 << j);
				}
			}

			if ((adj_num_zero < 2) && ((!adj_num_pos) || (!adj_num_neg))) {
				// the adjacent quad does not cross the plane of our quad, no adjacency
				continue;
			}

			if (((adj_num_pos == 0) || (quad_num_pos == 0)) &&
				((adj_num_neg == 0) || (quad_num_neg == 0))) {
				// either both quads must be positive with respect to each other, or both must be negative with respect to each other,
				// in order for this to be a true surface-like adjacency
				if (adjacent_quad->flags & (AKcGQ_Flag_2Sided & ~AKcGQ_Flag_Jello)) {
					// try to flip the adjacent plane, see if this helps
					adj_swap = quad_num_pos;
					quad_num_pos = quad_num_neg;
					quad_num_neg = adj_swap;
					if (((adj_num_pos == 0) || (quad_num_pos == 0)) &&
						((adj_num_neg == 0) || (quad_num_neg == 0))) {
						// it did not help, reject this adjacency
						continue;
					} else {
						// it helps! apply this change to the already-calculated values
						for (j = 0; j < num_adjacent_points; j++) {
							quad_d[j] = -quad_d[j];
							adj_plane.a = -adj_plane.a;
							adj_plane.b = -adj_plane.b;
							adj_plane.c = -adj_plane.c;
							adj_plane.d = -adj_plane.d;
						}
					}
				} else {
					continue;
				}
			}

			// test against the original input direction supplied by the decal, and cull backfacing polygons
			fwd_dot_norm = P3gDecalDirection.x * adj_plane.a + P3gDecalDirection.y * adj_plane.b + P3gDecalDirection.z * adj_plane.c;
			if ((inDecalFlags & P3cDecalFlag_CullBackFacing) && (fwd_dot_norm < -1e-03f)) {
				continue;

			} else if (fwd_dot_norm < P3gDecalWrapDot) {
				// this polygon is facing too far away from the original in direction, don't wrap to it
				continue;
			}

			// find the points that define the overlap on the current quad
			found_p = 0;
			prev_edge = UUcTrue;		// disables crossing-checking for j == 0
			for (j = 0; j <= num_quad_points; j++) {
				if (quad_edge & (1 << j)) {
					if (quad_edge & (1 << ((j + 1) % num_quad_points))) {
						// two edge points in a row, the edges overlap here. this adjacency is on edge j of the quad
						current_edgecode |= (1 << j);
					}

					if (found_p < 2) {
						quad_p[found_p] = quad_points[j];
					}
					found_p++;
					prev_edge = UUcTrue;
				} else {
					this_index = j % num_quad_points;
					if (!prev_edge) {
						if (quad_d[prev_index] * quad_d[this_index] < 0) {
							// the d values on this edge span zero, there is a crossing here
							current_edgecode |= (1 << prev_index);
							if (found_p < 2) {
								MUmVector_ScaleCopy(quad_p[found_p], quad_d[this_index] / (quad_d[this_index] - quad_d[prev_index]), quad_points[prev_index]);
								MUmVector_ScaleIncrement(quad_p[found_p], -quad_d[prev_index] / (quad_d[this_index] - quad_d[prev_index]), quad_points[this_index]);
							}
							found_p++;
						}
					}
					prev_edge = UUcFalse;
					prev_index = this_index;
				}
			}

			if (found_p < 2) {
				// could not find the overlap points, there is an error - bail out of checking this quad
				continue;
			}

			// find the points that define the overlap on the adjacent quad
			found_p = 0;
			prev_edge = UUcTrue;		// disables crossing-checking for j == 0
			for (j = 0; j <= num_adjacent_points; j++) {
				if (adj_edge & (1 << j)) {
					if (adj_edge & (1 << ((j + 1) % num_adjacent_points))) {
						// two edge points in a row, the edges overlap here. this adjacency is on edge j of the child
						current_child_edgecode |= (1 << j);
					}

					if (found_p < 2) {
						adj_p[found_p] = adj_points[j];
					}
					found_p++;
					prev_edge = UUcTrue;
				} else {
					this_index = j % num_adjacent_points;
					if (!prev_edge) {
						if (adj_d[prev_index] * adj_d[this_index] < 0) {
							// the d values on this edge span zero, there is a crossing here
							current_child_edgecode |= (1 << prev_index);
							if (found_p < 2) {
								MUmVector_ScaleCopy(adj_p[found_p], adj_d[this_index] / (adj_d[this_index] - adj_d[prev_index]), adj_points[prev_index]);
								MUmVector_ScaleIncrement(adj_p[found_p], -adj_d[prev_index] / (adj_d[this_index] - adj_d[prev_index]), adj_points[this_index]);
							}
							found_p++;
						}
					}
					prev_edge = UUcFalse;
					prev_index = this_index;
				}
			}

			if (found_p < 2) {
				// could not find the overlap points, there is an error - bail out of checking this quad
				continue;
			}

			// if these adjacencies take place interior to a quad, then the edgecode should not be used to cull out when the
			// decal doesn't touch that particular edge.
			if ((quad_num_pos > 0) && (quad_num_neg > 0)) {
				current_edgecode = 0;
			}
			if ((adj_num_pos > 0) && (adj_num_neg > 0)) {
				current_child_edgecode = 0;
			}

			// determine the vector that defines the line that lies on both planes
			line_n.x = quad_plane.b * adj_plane.c - quad_plane.c * adj_plane.b;
			line_n.y = quad_plane.c * adj_plane.a - quad_plane.a * adj_plane.c;
			line_n.z = quad_plane.a * adj_plane.b - quad_plane.b * adj_plane.a;

			// check that the intervals of intersection actually overlap
			adj_t[0] = MUrVector_DotProduct(&adj_p[0], &line_n);
			adj_t[1] = MUrVector_DotProduct(&adj_p[1], &line_n);
			quad_t[0] = MUrVector_DotProduct(&quad_p[0], &line_n);
			quad_t[1] = MUrVector_DotProduct(&quad_p[1], &line_n);

			if (adj_t[0] > adj_t[1]) {
				swap = adj_t[0];
				swap_p = adj_p[0];
				adj_t[0] = adj_t[1];
				adj_p[0] = adj_p[1];
				adj_t[1] = swap;
				adj_p[1] = swap_p;
			}
			if (quad_t[0] > quad_t[1]) {
				swap = quad_t[0];
				swap_p = quad_p[0];
				quad_t[0] = quad_t[1];
				quad_p[0] = quad_p[1];
				quad_t[1] = swap;
				quad_p[1] = swap_p;
			}

			if ((adj_t[1] - quad_t[0] < P3cDecal_EdgeOverlapTolerance) ||
				(quad_t[1] - adj_t[0] < P3cDecal_EdgeOverlapTolerance)) {
				// the intervals do not overlap
				continue;
			}

			// store the found edge segment so that we can clip decal against it
			current_from_clip	= ((quad_num_pos > 0) && (quad_num_neg > 0));
			current_to_clip		= ((adj_num_pos > 0) && (adj_num_neg > 0));
			current_edgept[0]	= (quad_t[0] > adj_t[0]) ? quad_p[0] : adj_p[0];
			current_edgept[1]	= (quad_t[1] < adj_t[1]) ? quad_p[1] : adj_p[1];
		}

		// store the adjacency that we found - NB: previous code sets up current_from_clip, current_to_clip and current_edgept
		current_adjacency = P3iDecal_MakeNewEdge(current_traverseindex);
		if (current_adjacency == NULL) {
			// can't add any more adjacencies
			break;
		}

		current_adjacency->traverse_depth	= current_traverseindex;
		current_adjacency->from_gqindex		= inEdge->to_gqindex;
		current_adjacency->from_index		= inEdge->to_index;
		current_adjacency->to_gqindex		= adjacent_gqindex;
		current_adjacency->to_index			= itr;
		current_adjacency->from_clip		= current_from_clip;
		current_adjacency->to_clip			= current_to_clip;
		current_adjacency->edgept[0]		= current_edgept[0];
		current_adjacency->edgept[1]		= current_edgept[1];
		MUmVector_Subtract(current_adjacency->edgevec, current_edgept[1], current_edgept[0]);
		current_adjacency->edgecode			= current_edgecode;
		current_adjacency->child_edgecode	= current_child_edgecode;
		current_adjacency->parent_normal.x	= quad_plane.a;
		current_adjacency->parent_normal.y	= quad_plane.b;
		current_adjacency->parent_normal.z	= quad_plane.c;

		// calculate the edge-vector's location and position relative to the decal
		for (j = 0; j < 2; j++) {
			MUmVector_Subtract(delta_vec, current_adjacency->edgept[j], our_position);
			edgeloc[j][0] = MUrVector_DotProduct(&delta_vec, &our_right_vector);
			edgeloc[j][1] = MUrVector_DotProduct(&delta_vec, &our_up_vector);
		}
		edgedelta[0] = edgeloc[1][0] - edgeloc[0][0];
		edgedelta[1] = edgeloc[1][1] - edgeloc[0][1];

		// if the edge vector does not intersect the decal, this isn't a valid adjacency
		invalid_edge = UUcFalse;

		// first: check the edge intersects within t of [0, 1] in X
		if (edgedelta[0] < -P3cDecal_ParallelEdgeDelta) {
			edge_entryt[0] = (+inXScale - edgeloc[0][0]) / edgedelta[0];
			edge_exitt[0]  = (-inXScale - edgeloc[0][0]) / edgedelta[0];
			if ((edge_entryt[0] >= 1.0f) || (edge_exitt[0] <= 0.0f)) {
				// this is not a valid adjacency
				invalid_edge = UUcTrue;
			}
		} else if (edgedelta[0] > +P3cDecal_ParallelEdgeDelta) {
			edge_entryt[0] = (-inXScale - edgeloc[0][0]) / edgedelta[0];
			edge_exitt[0]  = (+inXScale - edgeloc[0][0]) / edgedelta[0];
			if ((edge_entryt[0] >= 1.0f) || (edge_exitt[0] <= 0.0f)) {
				// this is not a valid adjacency
				invalid_edge = UUcTrue;
			}
		} else {
			edge_entryt[0] = -1e09f;
			edge_exitt[0]  = +1e09f;
			if ((edgeloc[0][0] <= -inXScale) || (edgeloc[0][0] >= +inXScale)) {
				// this is not a valid adjacency
				invalid_edge = UUcTrue;
			}
		}
		if (!invalid_edge) {
			// next: check the edge intersects within t of [0, 1] in Y
			if (edgedelta[1] < -P3cDecal_ParallelEdgeDelta) {
				edge_entryt[1] = (+inYScale - edgeloc[0][1]) / edgedelta[1];
				edge_exitt[1]  = (-inYScale - edgeloc[0][1]) / edgedelta[1];
				if ((edge_entryt[1] >= 1.0f) || (edge_exitt[1] <= 0.0f)) {
					// this is not a valid adjacency
					invalid_edge = UUcTrue;
				}
			} else if (edgedelta[1] > +P3cDecal_ParallelEdgeDelta) {
				edge_entryt[1] = (-inYScale - edgeloc[0][1]) / edgedelta[1];
				edge_exitt[1]  = (+inYScale - edgeloc[0][1]) / edgedelta[1];
				if ((edge_entryt[1] >= 1.0f) || (edge_exitt[1] <= 0.0f)) {
					// this is not a valid adjacency
					invalid_edge = UUcTrue;
				}
			} else {
				edge_entryt[1] = -1e09f;
				edge_exitt[1]  = +1e09f;
				if ((edgeloc[0][1] <= -inYScale) || (edgeloc[0][1] >= +inYScale)) {
					// this is not a valid adjacency
					invalid_edge = UUcTrue;
				}
			}
		}
		if (!invalid_edge) {
			// next: check the edge intersections in X and Y form a valid interval
			current_adjacency->edge_entryt = UUmMax(edge_entryt[0], edge_entryt[1]);
			current_adjacency->edge_exitt  = UUmMin(edge_exitt[0],  edge_exitt[1]);
			if (current_adjacency->edge_exitt < current_adjacency->edge_entryt) {
				// this is not a valid adjacency
				invalid_edge = UUcTrue;
			}
		}
		if (invalid_edge) {
			// delete this edge and continue looking for valid adjacencies
			P3iDecal_RemoveEdge(current_adjacency);
			continue;
		}

		// calculate the middle of the interval that the decal and edge shade
		mid_t = (current_adjacency->edge_entryt + current_adjacency->edge_exitt) / 2;
		MUmVector_Copy(current_adjacency->edge_decalmid, current_adjacency->edgept[0]);
		MUmVector_ScaleIncrement(current_adjacency->edge_decalmid, mid_t, current_adjacency->edgevec);

		// form a right-angle basis system that uses the edge vector
		MUmVector_Subtract(edgebasis_u, current_adjacency->edgept[1], current_adjacency->edgept[0]);
		MUmVector_Normalize(edgebasis_u);
		plane_norm.x = quad_plane.a;
		plane_norm.y = quad_plane.b;
		plane_norm.z = quad_plane.c;
		MUrVector_CrossProduct(&edgebasis_u, &plane_norm, &edgebasis_r);
		MUmVector_Normalize(edgebasis_r);

		// calculate the position, up and right vectors in terms of this basis system
		MUmVector_Subtract(delta_vec, our_position, current_adjacency->edgept[0]);
		current_adjacency->pos_edgeu = MUrVector_DotProduct(&delta_vec, &edgebasis_u);
		current_adjacency->pos_edger = MUrVector_DotProduct(&delta_vec, &edgebasis_r);
		current_adjacency->up_edgeu = MUrVector_DotProduct(&our_up_vector, &edgebasis_u);
		current_adjacency->up_edger = MUrVector_DotProduct(&our_up_vector, &edgebasis_r);
		current_adjacency->right_edgeu = MUrVector_DotProduct(&our_right_vector, &edgebasis_u);
		current_adjacency->right_edger = MUrVector_DotProduct(&our_right_vector, &edgebasis_r);

		adjacencies[found_adjacencies++] = current_adjacency;
		if (found_adjacencies >= P3cDecal_MaxAdjacencies) {
			break;
		}
	}

	// build the initial points for the decal. note that the shades are set up as zero initially, which is
	// handled as a special case after we finish clipping. we can do this because black wouldn't be
	// 0x00000000, it would be 0xFF000000 (alpha)
	unprojected_points[0].x	= our_position.x - inXScale * our_right_vector.x + inYScale * our_up_vector.x;
	unprojected_points[0].y	= our_position.y - inXScale * our_right_vector.y + inYScale * our_up_vector.y;
	unprojected_points[0].z	= our_position.z - inXScale * our_right_vector.z + inYScale * our_up_vector.z;
	source_uvs[0].u			= 1;
	source_uvs[0].v			= 0;
	source_shades[0]		= 0;

	unprojected_points[1].x	= our_position.x + inXScale * our_right_vector.x + inYScale * our_up_vector.x;
	unprojected_points[1].y	= our_position.y + inXScale * our_right_vector.y + inYScale * our_up_vector.y;
	unprojected_points[1].z	= our_position.z + inXScale * our_right_vector.z + inYScale * our_up_vector.z;
	source_uvs[1].u			= 0;
	source_uvs[1].v			= 0;
	source_shades[1]		= 0;

	unprojected_points[2].x	= our_position.x + inXScale * our_right_vector.x + -inYScale * our_up_vector.x;
	unprojected_points[2].y	= our_position.y + inXScale * our_right_vector.y + -inYScale * our_up_vector.y;
	unprojected_points[2].z	= our_position.z + inXScale * our_right_vector.z + -inYScale * our_up_vector.z;
	source_uvs[2].u			= 0;
	source_uvs[2].v			= 1;
	source_shades[2]		= 0;

	unprojected_points[3].x	= our_position.x - inXScale * our_right_vector.x + -inYScale * our_up_vector.x;
	unprojected_points[3].y	= our_position.y - inXScale * our_right_vector.y + -inYScale * our_up_vector.y;
	unprojected_points[3].z	= our_position.z - inXScale * our_right_vector.z + -inYScale * our_up_vector.z;
	source_uvs[3].u			= 1;
	source_uvs[3].v			= 1;
	source_shades[3]		= 0;

	// set up our source buffer
	source_buffer.num_points = 4;
	source_buffer.max_num_points = 4;
	source_buffer.points = source_points;
	source_buffer.uvs = source_uvs;
	source_buffer.shades = source_shades;
	source_buffer.clipcodes = source_clipcodes;

	// project the decal onto the surface of this quad
	fwd_dot_norm = our_fwd_vector.x * quad_plane.a + our_fwd_vector.y * quad_plane.b + our_fwd_vector.z * quad_plane.c;
	if (fabs(fwd_dot_norm) < 1e-03f) {
		// the forward vector is perpendicular to our quad, something is terribly wrong
		success = UUcFalse;
		goto exit;
	}
	for(j = 0; j < 4; j++) {
		MUmVector_Copy(source_points[j], unprojected_points[j]);
		pt_plane_dist = source_points[j].x * quad_plane.a + source_points[j].y * quad_plane.b + source_points[j].z * quad_plane.c + quad_plane.d;

		// move the point back along the fwd-direction by an amount which will bring it back onto the plane
		MUmVector_ScaleIncrement(source_points[j], -pt_plane_dist / fwd_dot_norm, our_fwd_vector);
	}

	// get the shades at the corners of the quad
	for (j = 0; j < num_quad_points; j++) {
		quad_shades[j] = general_quad->m3Quad.shades[j];
	}

	// find how many planes we have to clip against
	num_clip_edges = clipped_edges = 0;
	for (itr = 0; itr < found_adjacencies; itr++) {
		if (!adjacencies[itr]->from_clip) {
			// don't clip our decal against this edge
			continue;
		}

		if ((adjacencies[itr]->edge_entryt < -P3cDecal_EdgeClipTolerance) ||
			(adjacencies[itr]->edge_exitt  > 1.0f + P3cDecal_EdgeClipTolerance)) {
			// the decal extends far enough outside the t-value of the edge (0 - 1) to make it invalid for us to clip against it
			adjacencies[itr]->from_clip = UUcFalse;
			continue;
		}

		num_clip_edges++;
	}

	// work out whereabouts the decal entered this polygon
	if (inEdge->from_index == (UUtUns32) -1) {
		parentedge_midpoint = &P3gDecalPosition;
	} else {
		parentedge_midpoint = &inEdge->edge_decalmid;
	}

	// perform the polygon clipping
	quad_point_base = P3gDecalBuildBuffer.num_points;
	quad_clipcode = 0;
	if (num_clip_edges == 0) {
		outputbuf = &P3gDecalBuildBuffer;
	} else {
		outputbuf = &P3gDecalClipBuffer[2];
		outputbuf->num_points = 0;
	}
	num_remaining_points = P3iDecal_ClipToPolygon(num_quad_points, quad_points, quad_shades, &quad_plane, &source_buffer, &quad_clipcode, outputbuf);

	if ((inEdge->from_index != (UUtUns32) -1) && (inEdge->child_edgecode != 0) && ((quad_clipcode & inEdge->child_edgecode) == 0)) {
		// we are not the source quad for this decal, and our decal was not clipped against the edges which
		// generated this adjacency. do not form a decal here.
		P3gDecalBuildBuffer.num_points = quad_point_base;
		success = UUcFalse;
		goto exit;
	}

	// clip the decal against all of the edge intersections that require it
	itr = 0;
	while ((num_remaining_points > 0) && (clipped_edges < num_clip_edges)) {
		inputbuf = outputbuf;
		if (clipped_edges + 1 >= num_clip_edges) {
			outputbuf = &P3gDecalBuildBuffer;
			// don't zero num_points, append to the end of the build buffer
		} else {
			outputbuf = &P3gDecalClipBuffer[clipped_edges % 2];
			outputbuf->num_points = 0;
		}
		UUmAssert(inputbuf != outputbuf);

		// find the next edge intersection to clip against
		while (1) {
			if (adjacencies[itr]->from_clip) {
				break;
			}

			itr++;
			if (itr >= found_adjacencies) {
				// couldn't find an adjacency to clip against! there is something horribly wrong
				UUmAssert(0);
				success = UUcFalse;
				goto exit;
			}
		}

		// clip against the edge in question
		if (adjacencies[itr]->to_index == (UUtUns32) -1) {
			// this is the edge in from our parent, use its normal to determine which side of the plane to clip on
			MUmVector_Add(delta_vec, adjacencies[itr]->parent_normal, adjacencies[itr]->edge_decalmid);
			num_remaining_points = P3iDecal_ClipToPlane(0, &adjacencies[itr]->edgept[0], &adjacencies[itr]->edgept[1], 0, 0,
														&delta_vec, inputbuf, outputbuf);
		} else {
			// this is an edge out of our quad, use the direction to the in-edge to determine which side of the plane to clip on
			num_remaining_points = P3iDecal_ClipToPlane(0, &adjacencies[itr]->edgept[0], &adjacencies[itr]->edgept[1], 0, 0,
														parentedge_midpoint, inputbuf, outputbuf);
		}
		itr++;
		clipped_edges++;
	}

	quad_point_count = P3gDecalBuildBuffer.num_points - quad_point_base;
	if (quad_point_count > 0) {
		// mark the points as belonging to our current quad
		plane_index = inEnvironment->gqCollisionArray->gqCollision[gqindex].planeEquIndex;
		if (negate) {
			// we flipped this two-sided quote, make a note of such
			plane_index ^= 0x80000000;
		}
		
		if (num_quad_points > 3) {
			MUmVector_Subtract(quad_midvector, quad_points[2], quad_points[0]);
			MUmVector_ScaleCopy(quad_reversemidvector, -1, quad_midvector);
		}

		for (j = 0; j < quad_point_count; j++) {
			P3gDecalPlaneIndices[quad_point_base + j] = plane_index;

			if (P3gDecalDoLighting && (P3gDecalBuildBuffer.shades[quad_point_base + j] == 0)) {
				// this point was one of the decal's input points and lies inside the face of the polygon.
				// interpolate between quad_shades to find the correct shade for this point.
				if (num_quad_points <= 3) {
					P3iDecal_BarycentricShadeInterpolate(&quad_points[0], &quad_points[1], &quad_points[2],
														&quad_edgevec[0], &quad_edgevec[1], &quad_edgevec[2],
														quad_shades[0], quad_shades[1], quad_shades[2], &quad_cross_vec,
														&P3gDecalBuildBuffer.points[quad_point_base + j], &P3gDecalBuildBuffer.shades[quad_point_base + j]);
				} else {
					// determine whether this point lies in the 0/1/2 or 0/2/3 triangles
					MUmVector_Subtract(delta_vec, P3gDecalBuildBuffer.points[quad_point_base + j], quad_points[0]);
					MUrVector_CrossProduct(&delta_vec, &quad_midvector, &cross_vec);
					if (MUrVector_DotProduct(&cross_vec, &quad_cross_vec) > 0) {
						P3iDecal_BarycentricShadeInterpolate(&quad_points[0], &quad_points[1], &quad_points[2],
															&quad_edgevec[0], &quad_edgevec[1], &quad_reversemidvector,
															quad_shades[0], quad_shades[1], quad_shades[2], &quad_cross_vec,
															&P3gDecalBuildBuffer.points[quad_point_base + j], &P3gDecalBuildBuffer.shades[quad_point_base + j]);
					} else {
						P3iDecal_BarycentricShadeInterpolate(&quad_points[0], &quad_points[2], &quad_points[3],
															&quad_midvector, &quad_edgevec[2], &quad_edgevec[3],
															quad_shades[0], quad_shades[2], quad_shades[3], &quad_cross_vec,
															&P3gDecalBuildBuffer.points[quad_point_base + j], &P3gDecalBuildBuffer.shades[quad_point_base + j]);
					}
				}
			}
		}

		// turn these new points into a triangle fan
		num_tri_indices = *ioNumTriIndices;
		new_tris = quad_point_count - 2;
		for (j = 0; j < new_tris; j++) {
			if (num_tri_indices + 3 > inMaxTriIndices) {
				// out of room to add triangle indices!
				UUrDebuggerMessage("P3iDecal_ProcessAdjacency: decal has more than %d triangles, overflowed P3cDecalMaxTriIndices", inMaxTriIndices / 3);
				break;
			}
			ioTriIndices[num_tri_indices++] = (UUtUns16) (quad_point_base);
			ioTriIndices[num_tri_indices++] = (UUtUns16) (quad_point_base + j+1);
			ioTriIndices[num_tri_indices++] = (UUtUns16) (quad_point_base + j+2);
		}
		*ioTriCount += j;
		*ioNumTriIndices = num_tri_indices;
	} else {
		// the decal does not in fact touch our polygon, abort
		success = UUcFalse;
		goto exit;
	}

exit:
	for (itr = 0; itr < found_adjacencies; itr++) {
		if (adjacencies[itr]->to_index == (UUtUns32) -1) {
			// this is the false parent adjacency, don't remove it
			continue;

		} else if (!success) {
			// bail out; remove all edges
			P3iDecal_RemoveEdge(adjacencies[itr]);

		} else if ((adjacencies[itr]->edgecode != 0) && ((quad_clipcode & adjacencies[itr]->edgecode) == 0)) {
			// this adjacency only touches edges that the decal did not in fact reach
			// (and thus get clipped by).
			P3iDecal_RemoveEdge(adjacencies[itr]);
		}
	}

	return success;
}

UUtError P3rCreateDecal(M3tTextureMap *inTexture, UUtUns32 inGQIndex, UUtUns32 inDecalFlags, float inWrapAngle, M3tVector3D *inDirection,
						M3tVector3D *inPosition, M3tVector3D *inFwdVector, M3tVector3D *inUpVector, M3tVector3D *inRightVector,
						float inXScale, float inYScale, UUtUns16 inAlpha, IMtShade inTint, P3tDecalData *inDecalData,
						UUtUns32 inManualBufferSize)
{
	UUtUns32					i, j;

	// collision
	UUtBool						found_collision;
	M3tVector3D					ray_dir;
	AKtCollision				*collision;
	AKtGQ_General				*general_quad;
	AKtGQ_Collision				*collide_quad;
	M3tBoundingSphere			sphere;
	M3tPlaneEquation			quad_plane;
	AKtEnvironment *			environment;

	// decal parameters
	M3tVector3D					position;
	M3tVector3D					fwd_vector;
	M3tVector3D					up_vector;
	M3tVector3D					right_vector;
	M3tPoint3D					unprojected_points[4];

	// the quad that we are clipping to
	float						pt_plane_dist, fwd_dot_norm;
	UUtUns32					num_quad_points;
	M3tPoint3D					quad_points[4];
	M3tVector3D					quad_edgevec[4];
	UUtUns32					quad_shades[4];
	M3tVector3D					delta_vec, quad_midvector, quad_reversemidvector, cross_vec, quad_cross_vec;

	// source buffer
	P3tDecalEdge *				source_adjacency;
	P3tDecalPointBuffer			source_buffer;
	M3tPoint3D					source_points[4];
	M3tTextureCoord				source_uvs[4];
	UUtUns8						source_clipcodes[4];
	UUtUns32					source_shades[4];

	// decal construction storage
	UUtUns32					quad_point_base, quad_point_count;
	UUtUns32					tri_count, new_tris;
	UUtUns32					vert_count;
	UUtUns32					index_count;
	UUtUns32					num_tri_indices;
	static UUtUns32				processed_bv[P3cDecal_GQBitVectorSize];
	static UUtUns16				tri_indices[P3cDecalMaxTriIndices];
	P3tDecalEdge *				cur_adjacency;

	// shared-point calculation variables
	UUtUns32					num_planes, num_duplicates, pt_index, new_index;
	UUtUns32					plane_indices[P3cDecal_MaxSharedPlanes];
	M3tPoint3D					compare_point, pt_offset;
	M3tTextureCoord				compare_uv;
	UUtUns32					compare_shade;
	float						offset_length;
	UUtBool						is_duplicate;

	// decal memory allocation and packing
	UUtUns32					decal_size;
	UUtUns16					decal_block;
	char						*decal_data;
	char						*decal_ptr;
	M3tPoint3D					*points;
	UUtUns16					*indices;
	M3tDecalHeader				*decal;
	M3tTextureCoord				*coords;
	UUtUns32					*shades;
	P3tDecalList 				*decal_list;

	UUmAssertReadPtr(inDecalData, sizeof(P3tDecalData));
	fwd_vector = *inFwdVector;
	up_vector = *inUpVector;
	right_vector = *inRightVector;
	if (inDirection == NULL) {
		P3gDecalDirection = fwd_vector;
	} else {
		P3gDecalDirection = *inDirection;

		// normalize the input direction
		if (MUrVector_Normalize_ZeroTest(&P3gDecalDirection)) {
			P3gDecalDirection = fwd_vector;
		} else {
			// decal direction must point out of surface, not into surface
			MUmVector_Negate(P3gDecalDirection);
		}
	}

	// calculate the minimum dot product required for us to wrap to a polygon
	P3gDecalWrapDot = MUrCos(UUmPin(inWrapAngle, 0.5f * M3cDegToRad, M3cPi));

	P3gDecalDoLighting = ((inDecalFlags & P3cDecalFlag_FullBright) == 0);

	// build a ray that goes into the wall - used for collision
	MUmVector_ScaleCopy(ray_dir, -10.0f, *inFwdVector);

	environment = ONrGameState_GetEnvironment();
	if (inGQIndex == 0xffffffff) {
		// we must cast a ray against the environment to find the first wall collision
		found_collision = AKrCollision_Point(environment, inPosition, &ray_dir, AKcGQ_Flag_Obj_Col_Skip | AKcGQ_Flag_Invisible | AKcGQ_Flag_NoDecal, 1);
		if (!found_collision) {
			// no collision, can't create decal
			return UUcError_Generic;
		}
		
		position = AKgCollisionList[0].collisionPoint;
		inGQIndex = AKgCollisionList[0].gqIndex;
	} else {
		UUmAssert((inGQIndex >= 0) && (inGQIndex < environment->gqGeneralArray->numGQs));
		general_quad = environment->gqGeneralArray->gqGeneral + inGQIndex;
		if (general_quad->flags & AKcGQ_Flag_NoDecal) {
			// don't create decals on this surface
			return UUcError_Generic;
		}
		position = *inPosition;
	}
	
	// collide a sphere with the environment to find all nearby quads
	sphere.radius = MUrSqrt(UUmSQR(inXScale) + UUmSQR(inYScale));
	sphere.center = position;
	found_collision = AKrCollision_Sphere(environment, &sphere, &ray_dir, AKcGQ_Flag_Obj_Col_Skip | AKcGQ_Flag_Invisible | AKcGQ_Flag_NoDecal, AKcMaxNumCollisions);
	if (AKgNumCollisions == 0) {
		// no nearby quads, can't create decal
		return UUcError_Generic;
	}
	if (AKgNumCollisions > 32 * P3cDecal_GQBitVectorSize) {
		UUrDebuggerMessage("P3rCreateDecal: found %d collisions nearby, overflowed P3cDecal_GQBitVectorSize (%d)",
			AKgNumCollisions, 32 * P3cDecal_GQBitVectorSize);
		AKgNumCollisions = 32 * P3cDecal_GQBitVectorSize;
	}

	// empty the decal-building buffer
	P3gDecalBuildBuffer.num_points = 0;
	tri_count = 0;
	num_tri_indices = 0;

	if (inDecalFlags & P3cDecalFlag_IgnoreAdjacency) {
		// build the initial points for the decal
		unprojected_points[0].x	= position.x - inXScale * right_vector.x + inYScale * up_vector.x;
		unprojected_points[0].y	= position.y - inXScale * right_vector.y + inYScale * up_vector.y;
		unprojected_points[0].z	= position.z - inXScale * right_vector.z + inYScale * up_vector.z;
		source_uvs[0].u			= 1;
		source_uvs[0].v			= 0;

		unprojected_points[1].x	= position.x + inXScale * right_vector.x + inYScale * up_vector.x;
		unprojected_points[1].y	= position.y + inXScale * right_vector.y + inYScale * up_vector.y;
		unprojected_points[1].z	= position.z + inXScale * right_vector.z + inYScale * up_vector.z;
		source_uvs[1].u			= 0;
		source_uvs[1].v			= 0;

		unprojected_points[2].x	= position.x + inXScale * right_vector.x + -inYScale * up_vector.x;
		unprojected_points[2].y	= position.y + inXScale * right_vector.y + -inYScale * up_vector.y;
		unprojected_points[2].z	= position.z + inXScale * right_vector.z + -inYScale * up_vector.z;
		source_uvs[2].u			= 0;
		source_uvs[2].v			= 1;

		unprojected_points[3].x	= position.x - inXScale * right_vector.x + -inYScale * up_vector.x;
		unprojected_points[3].y	= position.y - inXScale * right_vector.y + -inYScale * up_vector.y;
		unprojected_points[3].z	= position.z - inXScale * right_vector.z + -inYScale * up_vector.z;
		source_uvs[3].u			= 1;
		source_uvs[3].v			= 1;

		// set up our source buffer
		source_buffer.num_points = 4;
		source_buffer.points = source_points;
		source_buffer.uvs = source_uvs;
		source_buffer.shades = source_shades;
		source_buffer.clipcodes = source_clipcodes;

		for (i = 0, collision = AKgCollisionList; i < AKgNumCollisions; i++, collision++) {
			// cull out backfacing quads
			general_quad = environment->gqGeneralArray->gqGeneral + collision->gqIndex;
			collide_quad = environment->gqCollisionArray->gqCollision + collision->gqIndex;
			AKmPlaneEqu_GetComponents(collide_quad->planeEquIndex, environment->planeArray->planes,
										quad_plane.a, quad_plane.b, quad_plane.c, quad_plane.d);

			fwd_dot_norm = fwd_vector.x * quad_plane.a + fwd_vector.y * quad_plane.b + fwd_vector.z * quad_plane.c;
			if (general_quad->flags & (AKcGQ_Flag_2Sided & ~AKcGQ_Flag_Jello)) {
				if (fwd_dot_norm < 0) {
					// flip this two-sided quad
					quad_plane.a = -quad_plane.a;
					quad_plane.b = -quad_plane.b;
					quad_plane.c = -quad_plane.c;
					quad_plane.d = -quad_plane.d;
					fwd_dot_norm = -fwd_dot_norm;
				}
			}

			if ((inDecalFlags & P3cDecalFlag_CullBackFacing) && (fwd_dot_norm < 1e-03f)) {
				continue;
			}

			// get the points of this quad that we are clipping against
			num_quad_points = (general_quad->flags & AKcGQ_Flag_Triangle) ? 3 : 4;
			for (j = 0; j < num_quad_points; j++) {
				quad_points[j] = environment->pointArray->points[general_quad->m3Quad.vertexIndices.indices[j]];
				if (P3gDecalDoLighting) {
					quad_shades[j] = general_quad->m3Quad.shades[j];
				}
			}

			// project the decal onto the surface of this quad
			for(j = 0; j < 4; j++) {
				MUmVector_Copy(source_points[j], unprojected_points[j]);
				pt_plane_dist = source_points[j].x * quad_plane.a + source_points[j].y * quad_plane.b + source_points[j].z * quad_plane.c + quad_plane.d;

				if (fabs(fwd_dot_norm) >= 1e-03) {
					// move the point back along the fwd-direction by an amount which will bring it back onto the plane
					MUmVector_ScaleIncrement(source_points[j], -pt_plane_dist / fwd_dot_norm, fwd_vector);
				}

				// sentinel value 0x00000000 can never occur on a quad (alpha = 0)
				source_shades[j] = 0;
			}

			quad_point_base = P3gDecalBuildBuffer.num_points;
			quad_point_count = P3iDecal_ClipToPolygon(num_quad_points, quad_points, quad_shades, &quad_plane, &source_buffer, NULL, &P3gDecalBuildBuffer);
			if (quad_point_count == 0) {
				continue;
			}

			if (P3gDecalDoLighting) {
				// precalculate values for lighting interpolation
				for (j = 0; j < num_quad_points; j++) {
					MUmVector_Subtract(quad_edgevec[j], quad_points[(j + 1) % num_quad_points], quad_points[j]);
				}
				if (num_quad_points >= 4) {
					MUmVector_Subtract(quad_midvector, quad_points[2], quad_points[0]);
					MUmVector_ScaleCopy(quad_reversemidvector, -1, quad_midvector);
				}
				MUrVector_CrossProduct(&quad_edgevec[0], &quad_edgevec[1], &quad_cross_vec);
			}

			// offset the points slightly along the direction of the projection, and set up the plane indices
			for (j = 0; j < quad_point_count; j++) {
				MUmVector_ScaleIncrement(P3gDecalBuildBuffer.points[j + quad_point_base], P3cDecalPointFudge, fwd_vector);

				if (P3gDecalDoLighting && (P3gDecalBuildBuffer.shades[quad_point_base + j] == 0)) {
					// this point was one of the decal's input points and lies inside the face of the polygon.
					// interpolate between quad_shades to find the correct shade for this point.
					if (num_quad_points <= 3) {
						P3iDecal_BarycentricShadeInterpolate(&quad_points[0], &quad_points[1], &quad_points[2],
															&quad_edgevec[0], &quad_edgevec[1], &quad_edgevec[2],
															quad_shades[0], quad_shades[1], quad_shades[2], &quad_cross_vec,
															&P3gDecalBuildBuffer.points[quad_point_base + j], &P3gDecalBuildBuffer.shades[quad_point_base + j]);
					} else {
						// determine whether this point lies in the 0/1/2 or 0/2/3 triangles
						MUmVector_Subtract(delta_vec, P3gDecalBuildBuffer.points[quad_point_base + j], quad_points[0]);
						MUrVector_CrossProduct(&delta_vec, &quad_midvector, &cross_vec);
						if (cross_vec.x * quad_plane.a + cross_vec.y * quad_plane.b + cross_vec.z * quad_plane.c > 0) {
							P3iDecal_BarycentricShadeInterpolate(&quad_points[0], &quad_points[1], &quad_points[2],
																&quad_edgevec[0], &quad_edgevec[1], &quad_reversemidvector,
																quad_shades[0], quad_shades[1], quad_shades[2], &quad_cross_vec,
																&P3gDecalBuildBuffer.points[quad_point_base + j], &P3gDecalBuildBuffer.shades[quad_point_base + j]);
						} else {
							P3iDecal_BarycentricShadeInterpolate(&quad_points[0], &quad_points[2], &quad_points[3],
																&quad_midvector, &quad_edgevec[2], &quad_edgevec[3],
																quad_shades[0], quad_shades[2], quad_shades[3], &quad_cross_vec,
																&P3gDecalBuildBuffer.points[quad_point_base + j], &P3gDecalBuildBuffer.shades[quad_point_base + j]);
						}
					}
				}
			}

			// turn these new points into a triangle fan
			new_tris = quad_point_count - 2;
			for (j = 0; j < new_tris; j++) {
				if (num_tri_indices + 3 > P3cDecalMaxTriIndices) {
					// out of room to add triangle indices!
					UUrDebuggerMessage("P3rCreateDecal: decal has more than %d triangles, overflowed P3cDecalMaxTriIndices", P3cDecalMaxTriIndices / 3);
					break;
				}
				tri_indices[num_tri_indices++] = (UUtUns16) (quad_point_base);
				tri_indices[num_tri_indices++] = (UUtUns16) (quad_point_base + j+1);
				tri_indices[num_tri_indices++] = (UUtUns16) (quad_point_base + j+2);
			}
			tri_count += j;
		}
	} else {
		// set up bitvector describing which quads in the collision list have been processed
		UUrMemory_Clear(processed_bv, P3cDecal_GQBitVectorSize * sizeof(UUtUns32));

		// find which quad in the list is the quad that we hit
		for (j = 0; j < AKgNumCollisions; j++) {
			if (AKgCollisionList[j].gqIndex == inGQIndex) {
				break;
			}
		}
		if (j >= AKgNumCollisions) {
			// can't find starting quad! just start from the first
			j = 0;
		}

		// clear the adjacency buffer
		P3gDecalNextEdge = NULL;

		// process adjacencies, starting from the quad which we found
		source_adjacency = P3iDecal_MakeNewEdge(0);
		if (source_adjacency != NULL) {
			source_adjacency->traverse_depth = 0;
			source_adjacency->from_index = (UUtUns32) -1;
			source_adjacency->from_gqindex = (UUtUns32) -1;
			source_adjacency->from_clip = UUcFalse;
			source_adjacency->to_index = j;
			source_adjacency->to_gqindex = AKgCollisionList[j].gqIndex;
			source_adjacency->to_clip = UUcFalse;
			source_adjacency->parent_normal = *inFwdVector;
		}

		// set up the global decal position
		P3gDecalPosition = *inPosition;
		P3gDecalFwd = *inFwdVector;
		P3gDecalUp = *inUpVector;
		P3gDecalRight = *inRightVector;

		while (P3gDecalNextEdge != NULL) {
			cur_adjacency = P3gDecalNextEdge;

			// if the quad is unprocessed, check it to see if we need to generate points
			if (!UUrBitVector_TestAndSetBit(processed_bv, cur_adjacency->to_index)) {
				P3iDecal_ProcessAdjacency(environment, inDecalFlags, cur_adjacency, inXScale, inYScale,
											&tri_count, &num_tri_indices, P3cDecalMaxTriIndices, tri_indices);
			}
			P3iDecal_RemoveEdge(cur_adjacency);
		}
	}

	if (P3gDecalBuildBuffer.num_points == 0) {
		// can't create a decal here
		UUmAssert(tri_count == 0);
		return UUcError_Generic;
	} else {
		UUmAssert(tri_count > 0);
		UUmAssert(P3gDecalBuildBuffer.num_points <= P3cDecalBuildBufferSize);
	}

	// set up for the shared-points calculation pass
	//
	// CB: this is an O(n^2) computation and might potentially be very slow. there may be better solutions to the
	// seaming problem which would involve keeping an adjacency graph and only checking through that, or calculating shared
	// points at clip time. both of these involve a significant investment of time and I'm deferring them for now until we
	// can see just how slow this really is in practice. n < 20 for the majority of real cases.
	vert_count = 0;
	UUrBitVector_ClearBitAll(P3gDecalBuildDuplicateBV, P3gDecalBuildBuffer.num_points);
	UUrBitVector_ClearBitAll(P3gDecalBuildCoincidentBV, P3gDecalBuildBuffer.num_points);
	UUrMemory_Clear(P3gDecalBuildIndices, P3gDecalBuildBuffer.num_points * sizeof(UUtUns32));
	num_duplicates = 0;
	for (i = 0; i < P3gDecalBuildBuffer.num_points; i++) {
		if (UUrBitVector_TestBit(P3gDecalBuildDuplicateBV, i)) {
			// the index for this point should now be a packed vertex buffer index
			UUmAssert((P3gDecalBuildIndices[i] >= 0) && (P3gDecalBuildIndices[i] < vert_count));
			continue;
		}

		if ((inDecalFlags & P3cDecalFlag_IgnoreAdjacency) == 0) {
			num_planes = 1;
			plane_indices[0] = P3gDecalPlaneIndices[i];
			UUmAssert(((plane_indices[0] & 0x7FFFFFFF) >= 0) && ((plane_indices[0] & 0x7FFFFFFF) < environment->planeArray->numPlanes));
		}
		compare_point	= P3gDecalBuildBuffer.points[i];
		compare_uv		= P3gDecalBuildBuffer.uvs[i];
		compare_shade	= P3gDecalBuildBuffer.shades[i];

		// find all points which could potentially be a duplicate of this
		for (j = i + 1; j < P3gDecalBuildBuffer.num_points; j++) {
			if (UUrBitVector_TestBit(P3gDecalBuildDuplicateBV, j)) {
				continue;
			}

			if (((float)fabs(P3gDecalBuildBuffer.points[j].x - compare_point.x) > P3cDecal_SharedPointTolerance) || 
				((float)fabs(P3gDecalBuildBuffer.points[j].y - compare_point.y) > P3cDecal_SharedPointTolerance) ||
				((float)fabs(P3gDecalBuildBuffer.points[j].z - compare_point.z) > P3cDecal_SharedPointTolerance)) {
				// the points are different and cannot be snapped to each other
				continue;
			}
			is_duplicate = (((float)fabs(P3gDecalBuildBuffer.uvs[j].u - compare_uv.u) < P3cDecal_SharedUVTolerance) &&
							((float)fabs(P3gDecalBuildBuffer.uvs[j].v - compare_uv.v) < P3cDecal_SharedUVTolerance) &&
							((!P3gDecalDoLighting) || (IMrShade_IsEqual(P3gDecalBuildBuffer.shades[j], compare_shade))));

			// the points are identical
			if ((inDecalFlags & P3cDecalFlag_IgnoreAdjacency) == 0) {
				// the points are identical, store the plane that this point was generated from
				plane_indices[num_planes++] = P3gDecalPlaneIndices[j];
				UUmAssert(((plane_indices[num_planes - 1] & 0x7FFFFFFF) >= 0) &&
						  ((plane_indices[num_planes - 1] & 0x7FFFFFFF) < environment->planeArray->numPlanes));
			}

			if (is_duplicate) {
				// the point is an exact duplicate and must be collapsed into its master
				UUrBitVector_SetBit(P3gDecalBuildDuplicateBV, j);
				P3gDecalBuildIndices[j] = vert_count;
				num_duplicates++;

				// if this point was previously marked as coincident, remove this status
				UUrBitVector_ClearBit(P3gDecalBuildCoincidentBV, j);
			} else {
				// the point is merely coincident, not an exact duplicate
				if (!UUrBitVector_TestAndSetBit(P3gDecalBuildCoincidentBV, j)) {
					// this is the first master point that has found this point as being coincident, store its
					// index in the high word (NB: this is NOT its packed index!)
					P3gDecalBuildIndices[j] |= (i << 16);
				}
			}

			if (((inDecalFlags & P3cDecalFlag_IgnoreAdjacency) == 0) && (num_planes >= P3cDecal_MaxSharedPlanes)) {
				// can't store any more adjacencies
				break;
			}
		}

		// get the packed index for this (original) point and store it in the low word
		P3gDecalBuildIndices[i] |= (UUtUns16) (vert_count++);

		if ((inDecalFlags & P3cDecalFlag_IgnoreAdjacency) == 0) {
			// offset the point in the direction of the normal, averaged over all planes that contributed a coincident point
			MUmVector_Set(pt_offset, 0, 0, 0);
			for (j = 0; j < num_planes; j++) {
				AKmPlaneEqu_GetComponents(plane_indices[j], environment->planeArray->planes,
											quad_plane.a, quad_plane.b, quad_plane.c, quad_plane.d);
				pt_offset.x += quad_plane.a;
				pt_offset.y += quad_plane.b;
				pt_offset.z += quad_plane.c;
			}
			offset_length = MUmVector_GetLengthSquared(pt_offset);
			if (offset_length > UUmSQR(P3cDecal_OffsetNormalLength)) {
				MUmVector_Scale(pt_offset, P3cDecalPointFudge * MUrOneOverSqrt(offset_length));
				MUmVector_Add(P3gDecalBuildBuffer.points[i], P3gDecalBuildBuffer.points[i], pt_offset);
			}
		}
	}
	UUmAssert(vert_count + num_duplicates == P3gDecalBuildBuffer.num_points);
	UUmAssert(vert_count <= UUcMaxUns16);		// must be true in order for tri indices to work

	// go through the buffer again and snap all coincident points to their master point's location.
	// note that this is a chaining process as the distance criterion is not transitive, i.e.
	// a <-> b and b <-> c does not necessarily imply a <-> c.
	for (i = 0; i < P3gDecalBuildBuffer.num_points; i++) {
		pt_index = i;
		while (UUrBitVector_TestBit(P3gDecalBuildCoincidentBV, pt_index)) {
			// this is a coincident point
			UUmAssert(!UUrBitVector_TestBit(P3gDecalBuildDuplicateBV, pt_index));

			// snap this point to the location of its master
			new_index = P3gDecalBuildIndices[pt_index] >> 16;
			UUmAssert((new_index >= 0) && (new_index < P3gDecalBuildBuffer.num_points));
			if (new_index >= pt_index) {
				// prevent infinite loops
				UUmAssert(0);
				break;
			}

			pt_index = new_index;
		}
		if (pt_index != i) {
			P3gDecalBuildBuffer.points[i] = P3gDecalBuildBuffer.points[pt_index];
		}
	}

	// number of indices must be four-byte aligned
	index_count = num_tri_indices;
	UUmAssert(index_count == 3 * tri_count);
	if (index_count & 0x03) {
		index_count &= ~(0x03);
		index_count += 4;
	}

	// allocate our decal data buffer
	decal_size = sizeof(M3tDecalHeader);
	decal_size += vert_count * (sizeof(M3tVector3D) + sizeof(M3tTextureCoord) + sizeof(UUtUns32));
	decal_size += index_count * sizeof(UUtUns16);
	
	if (inDecalFlags & P3cDecalFlag_Manual) {
		// write into the supplied decal buffer
		decal_data = (char*) inDecalData->decal_header;

	} else if (inDecalFlags & P3cDecalFlag_Static) {
		// allocate buffer out of global dynamic memory
		decal_data = UUrMemory_Block_New(decal_size);
		if (decal_data == NULL) {
			// could not allocate memory
			return UUcError_OutOfMemory;
		}
		inDecalData->decal_header = (M3tDecalHeader *) decal_data;

	} else {
		char debug_name[100];
		static UUtUns32 global_decal_id = 0;

		// allocate buffer out of dynamic LRAR cache structure
		sprintf(debug_name, "decal %d", global_decal_id++);
		decal_block	= lrar_allocate(P3gDecalCache, decal_size, debug_name, (void *) inDecalData);
		if ((decal_block == NONE) || (inDecalData->decal_header == NULL)) {
			// could not allocate memory
			return UUcError_OutOfMemory;
		}
		decal_data					= (char *) inDecalData->decal_header;
	}

	// compute offsets into the decal buffer
	UUmAssert(decal_data != NULL);
	decal_ptr	= decal_data;
	decal		= (M3tDecalHeader *)	decal_ptr;			decal_ptr += sizeof(M3tDecalHeader);
	points		= (M3tPoint3D *)		decal_ptr;			decal_ptr += vert_count * sizeof(M3tVector3D);
	coords		= (M3tTextureCoord*)	decal_ptr;			decal_ptr += vert_count * sizeof(M3tTextureCoord);
	indices		= (UUtUns16 *)			decal_ptr;			decal_ptr += index_count * sizeof(UUtUns16);
	shades		= (UUtUns32 *)			decal_ptr;			decal_ptr += vert_count * sizeof(UUtUns32);
	UUmAssert(decal_ptr == decal_data + decal_size);

	if ((inDecalFlags & P3cDecalFlag_Manual) && (decal_size > inManualBufferSize)) {
		UUrDebuggerMessage("P3rCreateDecal: decal size %d exceeds manual buffer size %d\n", decal_size, inManualBufferSize);
		return UUcError_Generic;
	}

	// pack the information into the decal buffer
	decal->gq_index = inGQIndex;
	decal->triangle_count = (UUtUns16) tri_count;
	decal->vertex_count	= (UUtUns16)	vert_count;
	decal->texture = inTexture;
	decal->alpha = inAlpha;
	decal->tint = inTint;

	// store the vertices
	UUrBitVector_ClearBitAll(P3gDecalBuildDuplicateBV, vert_count);
	for (i = 0; i < P3gDecalBuildBuffer.num_points; i++) {
		pt_index = P3gDecalBuildIndices[i] & 0x0000FFFF;
		UUmAssert((pt_index >= 0) && (pt_index < vert_count));
		if (UUrBitVector_TestAndSetBit(P3gDecalBuildDuplicateBV, pt_index)) {
			// we have already stored this vertex
			continue;
		}

		// store the values for this vertex
		points[pt_index] = P3gDecalBuildBuffer.points[i];
		coords[pt_index] = P3gDecalBuildBuffer.uvs[i];
		shades[pt_index] = P3gDecalDoLighting ? P3gDecalBuildBuffer.shades[i] : IMcShade_White;
	}
#if defined(DEBUGGING) && DEBUGGING
	for (i = 0; i < vert_count; i++) {
		UUmAssert(UUrBitVector_TestBit(P3gDecalBuildDuplicateBV, i));
	}
#endif

	// remap and store the triangle indices
	for (i = 0; i < num_tri_indices; i++) {
		UUmAssert((tri_indices[i] >= 0) && (tri_indices[i] < P3gDecalBuildBuffer.num_points));
		pt_index = P3gDecalBuildIndices[tri_indices[i]] & 0x0000FFFF;
		UUmAssert((pt_index >= 0) && (pt_index < vert_count));
		indices[i] = (UUtUns16) pt_index;
	}

	// add this decal to the appropriate global linked list
	decal_list = (inDecalFlags & (P3cDecalFlag_Manual | P3cDecalFlag_Static)) ? &P3gStaticDecals : &P3gDynamicDecals;
	P3iAddDecalToList(decal_list, inDecalData);

	P3mVerifyDecals();
	UUrMemory_Block_VerifyList();
	return UUcError_None;
}

static void P3iLRAR_NewProc(void *inUserData, short inBlockIndex)
{
	UUtUns32				addr;
	P3tDecalData			*data_ptr = (P3tDecalData*) inUserData;

	if( data_ptr )
	{
		addr									= lrar_block_address(P3gDecalCache, inBlockIndex);
		data_ptr->block							= inBlockIndex;
		data_ptr->decal_header					= (M3tDecalHeader*) &P3gDecalBuffer[addr];
		data_ptr->decal_header->triangle_count	= 0;
		data_ptr->decal_header->vertex_count	= 0;
		data_ptr->decal_header->texture			= NULL;
	}
}

static void P3iLRAR_PurgeProc(void *inUserData)
{
	P3tParticleReference	particle_ref;
	P3tParticle				*particle;
	P3tParticleClass		*particle_class;
	P3tDecalData			*data_ptr;
	
	if( !inUserData )
		return;

	data_ptr				= (P3tDecalData*) inUserData;
	particle_ref			= data_ptr->particle;
	P3mUnpackParticleReference(particle_ref, particle_class, particle);

	if( data_ptr->block != 0xffff )
	{
		data_ptr->block			= 0xffff;
		data_ptr->decal_header	= NULL;

		if (particle->header.self_ref == particle_ref) {
			// this is a valid particle reference, and its decal data must be deleted
			UUmAssert(P3rGetDecalDataPtr(particle_class, particle) == data_ptr);
			P3rKillParticle(particle_class, particle);
		}
	}

	P3mVerifyDecals();
}

void P3rDeleteDecal( P3tDecalData *inDecal, UUtUns32 inFlags )
{
	P3mVerifyDecals();

	if (inFlags & P3cDecalFlag_Static)
	{
		P3iRemoveDecalFromList(&P3gStaticDecals, inDecal);

		if (inDecal->decal_header != NULL) {
			UUrMemory_Block_Delete(inDecal->decal_header);
			inDecal->decal_header = NULL;
		}
	}
	else if(inFlags & P3cDecalFlag_Manual)
	{
		P3iRemoveDecalFromList(&P3gStaticDecals, inDecal);

		inDecal->decal_header->triangle_count = 0;
		inDecal->decal_header->vertex_count = 0;
	}
	else
	{
		P3iRemoveDecalFromList(&P3gDynamicDecals, inDecal);

		if (inDecal->block != (UUtUns16) 0xffff)
		{
			UUtUns16			block;
			block				= inDecal->block;
			inDecal->block		= 0xffff;
			lrar_deallocate( P3gDecalCache, block );
			inDecal->decal_header = NULL;
		}
		UUmAssert(inDecal->decal_header == NULL);
	}

	P3mVerifyDecals();
}

static void P3iDisplayDecals(P3tDecalList *inDecalList)
{
	AKtEnvironment		*environment = ONrGameState_GetEnvironment();
	P3tDecalData		*decal;
	AKtGQ_General		*gq_general;
	AKtGQ_Render		*gq_render;
	UUtUns8				visible;
	UUtUns16			alpha;
	UUtUns32			*gq_visvector, gq_index;

	if (inDecalList->num_decals == 0) {
		return;
	}
	UUmAssert(inDecalList->head != NULL);


	M3rGeom_State_Push();
	M3rDraw_State_SetInt(M3cDrawStateIntType_ZWrite, M3cDrawState_ZWrite_Off);
	M3rGeom_State_Commit();
	gq_visvector = AKrEnvironment_GetVisVector(environment);

	for (decal = inDecalList->head; decal != NULL; decal = decal->next) {
		if ((decal->decal_header == NULL) || (decal->decal_header->texture == NULL)) {
			continue;
		}

		gq_index = decal->decal_header->gq_index;
		if (gq_index == (UUtUns32) -1) {
			// this decal is always drawn, and never jelloed
			alpha = M3cMaxAlpha;
		} else {
			// CB: this code taken from AKiEnvironment_RayCastOctTree
			visible = UUr2BitVector_Test(gq_visvector, gq_index);
			if (!visible) {
				// the GQ that this decal is present on is not visible
				continue;
			}

			// calculate the alpha of this quad if jelloed
			gq_general = environment->gqGeneralArray->gqGeneral + gq_index;
			if (gq_general->flags & AKcGQ_Flag_Jello) {
				gq_render = environment->gqRenderArray->gqRender + gq_index;
				alpha = gq_render->alpha;
			} else {
				alpha = M3cMaxAlpha;
			}
		}

		M3rDecal_Draw(decal->decal_header, alpha);
	}

	M3rGeom_State_Pop();
}

void P3rDisplayStaticDecals(void)
{
	P3iDisplayDecals(&P3gStaticDecals);
}

void P3rDisplayDynamicDecals(void)
{
	P3iDisplayDecals(&P3gDynamicDecals);
}


