#ifndef _ENVFILEFORMAT_H_
#define _ENVFILEFORMAT_H_

/*
 * inport/export magic file
 *
 *
 */

#pragma once

#ifndef MODEL_EXPORTER

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Mathlib.h"

#else

typedef unsigned long	UUtUns32;
typedef long			UUtInt32;
typedef unsigned short	UUtUns16;
typedef short			UUtInt16;
typedef unsigned char   UUtUns8;
typedef signed char		UUtInt8;

typedef struct M3tTextureCoord	
{
	float	u;
	float	v;	
} M3tTextureCoord;

typedef struct M3tPoint3D	
{
	float	x; 
	float	y;  
	float	z; 	
} M3tPoint3D;

typedef struct M3tVector3D	
{
	float	x; 
	float	y;  
	float	z; 	
} M3tVector3D;

typedef struct M3tMatrix4x3
{
	float m[4][3];
} M3tMatrix4x3;

#endif

#define MXcMaxName 128

typedef struct MXtPoint
{
	M3tPoint3D			point;
	M3tVector3D			normal;
	M3tTextureCoord		uv;
} MXtPoint;

typedef struct MXtFace
{
	UUtUns16	indices[4];

	M3tVector3D	dont_use_this_normal;

	UUtUns16	material;
	UUtUns16	pad;
} MXtFace;

typedef struct MXtMarker
{
	char		name[MXcMaxName];
	M3tMatrix4x3	matrix;
	UUtUns32	userDataCount;
	char		*userData;	// written to disk as NULL
} MXtMarker;

#define MXcMapping_AM 0   // ambient
#define MXcMapping_DI 1   // diffuse
#define MXcMapping_SP 2   // specular
#define MXcMapping_SH 3   // shininesNs
#define MXcMapping_SS 4   // shininess strength
#define MXcMapping_SI 5   // self-illumination
#define MXcMapping_OP 6   // opacity
#define MXcMapping_FI 7   // filter color
#define MXcMapping_BU 8   // bump 
#define MXcMapping_RL 9   // reflection
#define MXcMapping_RR 10  // refraction 
#define MXcMapping_Count 11  // total 

typedef struct MXtMapping
{
	char		name[MXcMaxName];
	float		amount;
} MXtMapping;

typedef struct MXtMaterial
{
	char		name[MXcMaxName];
	
	float		alpha;
	float		selfIllumination;

	float		shininess;
	float		shininessStrength;

	float		ambient[3];
	float		diffuse[3];
	float		specular[3];

	MXtMapping	maps[MXcMapping_Count];

	UUtUns32	requirements;
} MXtMaterial;

typedef struct MXtNode MXtNode;

struct MXtNode
{
	char		name[MXcMaxName];
	char		parentName[MXcMaxName];
	M3tMatrix4x3	matrix;

	MXtNode		*parentNode;			// written as 0, build on read in
	UUtUns16	sibling;				// written as 0, build on read in
	UUtUns16	child;					// written as 0, build on read in

	UUtUns16	numPoints;
	UUtUns16	pad1;
	MXtPoint	*points;				// written as NULL, read as ptr

	UUtUns16	numTriangles;
	UUtUns16	pad2;
	MXtFace		*triangles;				// written as NULL, read as ptr

	UUtUns16	numQuads;
	UUtUns16	pad3;
	MXtFace		*quads;					// written as NULL, read as ptr

	UUtUns16	numMarkers;
	UUtUns16	pad4;
	MXtMarker	*markers;				// written as NULL, read as ptr

	UUtUns16	numMaterials;
	UUtUns16	pad5;
	MXtMaterial	*materials;				// written as NULL, read as ptr

	UUtUns32	userDataCount;
	char		*userData;				// written to disk as NULL, read as ptr
};

typedef struct MXtHeader
{
	char		stringENVF[4];			// = 'ENVF'
	UUtUns32	version;				// = 0
	UUtUns16	numNodes;
	UUtUns16	time;

	MXtNode		*nodes;					// written as NULL, read as ptr
} MXtHeader;

typedef struct AXtNode AXtNode;

struct AXtNode
{
	char		name[MXcMaxName];
	char		parentName[MXcMaxName];

	AXtNode		*parentNode;			// written as 0, build on read in
	UUtUns16	sibling;				// written as 0, build on read in
	UUtUns16	child;					// written as 0, build on read in

	UUtUns32	flags;	
	
	M3tMatrix4x3	objectTM;
	M3tMatrix4x3	*matricies;
};

typedef struct AXtHeader
{
	char		stringENVA[4];				// = 'ENVA'
	UUtUns32	version;					// = 0
	UUtUns16	numNodes;
	UUtUns16	numFrames;

	UUtUns16	startFrame;
	UUtUns16	endFrame;

	UUtUns16	ticksPerFrame;

	UUtUns16	startTime;
	UUtUns16	endTime;

	UUtUns16	pad;

	AXtNode		*nodes;
} AXtHeader;

#endif // _ENVFILEFORMAT_H_