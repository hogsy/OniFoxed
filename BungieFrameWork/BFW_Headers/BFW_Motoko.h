#pragma once

/*
	FILE:	BFW_Motoko.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: April 2, 1997
	
	PURPOSE: Interface to the Motoko 3D engine
	
	Copyright 1997

*/

/*
	Motoko is an abtract plug-in 3D engine. Its actually an API specification
	for a 3D engine. The real implementation is contained within a shared
	library. When Motoko is initialized at start-up time all shared libraries
	that implement the Motoko API are loaded and available for use.
	
	Coordinate Systems:
	
	  Frustum/View Space Coordinate System: Right Handed
	  	x increases to the right
	  	y increases upwards
	  	z increases out of the screen
	  
	  Screen Coordinate System: Right Handed
	    x increases to the right
	    y increases downwards
	    z increases into the screen
	  
	  
	
	
	SCREEN
	======
	
           x ->
	*-------------------------------->
	|
	|
 y  |
 |  |
 |  |
 \/ |
	|
	|
	|
	|
	|
	|
	|
	|
	|
	\/

	Motoko uses a right handed coordinate system
*/


#ifndef MOTOKO_H
#define MOTOKO_H

// S.S. bfw_math.h cometh #include <math.h>

#include "BFW.h"
#include "BFW_TemplateManager.h"
#include "BFW_Image.h"
#include "BFW_Group.h"
#include "BFW_Materials.h"
//#include "EnvFileFormat.h"

#ifdef __cplusplus
extern "C" {
#endif


/*
 * A few useful constants
 */

	#define M3cPi						3.1415926535897932384626433832795f
	#define M3c2Pi						6.283185307179586476925286766559f // S.S. ((float) (2.0f * M3cPi))
	#define M3cHalfPi					1.5707963267948966192313216916398f // S.S. ((float) (0.5f * M3cPi))
	#define M3cQuarterPi				0.78539816339744830961566084581988f // S.S. ((float) (0.25f * M3cPi))
	#define M3cEpsilon					UUcEpsilon
	#define M3cModelMatrix_MaxDepth		32
	#define M3cMaxObjVertices			2048 // S.S. (2 * 1024)	
	#define M3cMaxObjTris				4096 // S.S. (4 * 1024)	
	#define M3cMaxObjQuads				4096 // S.S. (4 * 1024)	
	#define M3cDegToRad					0.017453292519943295769236907684886f // S.S. (M3c2Pi / 360.f)
	#define M3cRadToDeg					57.295779513082320876798154814105f // S.S. (360.f / M3c2Pi)

	
	#define M3cExtraCoords				(60)
	
	#define M3cMaxDisplayModes			(16)
	#define M3cMaxDisplayDevices		(8)
	#define M3cMaxNumEngines			(2)
	#define M3cMaxNameLen				(64)
	
	#define M3cMaxAlpha					(255)
	
	#define M3cBBoxSideBV_PosX			1 // S.S. (1 << 0)
	#define M3cBBoxSideBV_NegX			2 // S.S. (1 << 1)
	#define M3cBBoxSideBV_PosY			4 // S.S. (1 << 2)
	#define M3cBBoxSideBV_NegY			8 // S.S. (1 << 3)
	#define M3cBBoxSideBV_PosZ			16 // S.S. (1 << 4)
	#define M3cBBoxSideBV_NegZ			32 // S.S. (1 << 5)
	
/*
 * Motoko is a 16bit RGB(1555 or 565) and 16bit zbuffer engine
 */
	#define M3cDrawRGBBytesPerPixel		2
	#define M3cDrawZBytesPerPixel		2


/*
 * Here is a handy little macro to turn a 16bit color into a 32bit color
 * all it does is shifts so it isn't perfect
 */
	#define M3mARGB16_to_32(x)		(((x & 0x0000F000) << 16) | \
									 ((x & 0x00000F00) << 12) | \
									 ((x & 0x000000F0) <<  8) | \
									 ((x & 0x0000000F) <<  4))

	#define M3mARGB32_to_16(x)		(((x & 0xF0000000) >> 16) | \
									 ((x & 0x00F00000) >> 12) | \
									 ((x & 0x0000F000) >>  8) | \
									 ((x & 0x000000F0) >>  4))

/*
 * Predefine a few opaque data structures
 */
 
	typedef struct M3tDrawContext			M3tDrawContext;
	typedef struct M3tDrawContextPrivate	M3tDrawContextPrivate;

	typedef struct M3tGeomContext			M3tGeomContext;
	typedef struct M3tGeomContextPrivate	M3tGeomContextPrivate;
	
	typedef struct M3tGeomCamera			M3tGeomCamera;
	
	#if UUmPlatform == UUmPlatform_Mac
	
		typedef GDHandle					M3tPlatformDevice;
		
	#elif UUmPlatform == UUmPlatform_Win32
		
		typedef LPVOID						M3tPlatformDevice;
		
	#elif UUmPlatform == UUmPlatform_Linux
		
		// software renderer not yet available on Linux
		typedef struct {} M3tPlatformDevice;
		
	#else
	
		#error implement me
		
	#endif

/*
 * A 3d point
 */
	typedef tm_struct M3tPoint3D	
	{
		float	x; 
		float	y;  
		float	z; 
		
	} M3tPoint3D;

#if 0 /*
 * Your standard issue 3d vector
 */
	typedef tm_struct M3tVector3D
	{
		float	x;
		float	y;
		float	z;
		
	} M3tVector3D;				
#endif

	typedef M3tPoint3D M3tVector3D;

	typedef tm_struct M3tPoint2D
	{
		float	x;
		float	z;
		
	} M3tPoint2D;

/*
 * 
 */
	typedef tm_struct M3tTextureCoord	
	{
		float	u;
		float	v;
		
	} M3tTextureCoord;

/*
 * 
 */
	typedef tm_struct M3tPoint4D
	{
		float	x;
		float	y;
		float	z;
		float	w;
	
	} M3tPoint4D;

/*
 * A Quaternion
 */


	typedef tm_struct M3tQuaternion
	{
		float		x;
		float		y;
		float		z;
		float		w;
	} M3tQuaternion;

/*
 * An Angle + Axis rotation
 */

	typedef tm_struct M3tRotation
	{
		M3tVector3D axis;
		float		angle;
	} M3tRotation;

/*
 * 
 */
	typedef tm_struct M3tTri
	{
		UUtUns32	indices[3];		
	} M3tTri;
		
	typedef tm_struct M3tQuad
	{
		UUtUns32	indices[4];		
	} M3tQuad;
		
	typedef tm_struct M3tPent
	{
		UUtUns32	indices[5];		
	} M3tPent;

/*
 * A Plane equation
 */
	typedef tm_struct M3tPlaneEquation
	{
		float	a;
		float	b;
		float	c;
		float	d;
		
	} M3tPlaneEquation;
	
/*
 * 
 */
	typedef tm_struct M3tPointScreen
	{
		float	x;
		float	y;
		float	z;
		float	invW;
		
	} M3tPointScreen;


/*
    Conceptually a M3tMatrix4x3 is viewed as the following:
    
	--         --
	|  a b c d  |
	|  e f g h  |
	|  i j k l  |
	--         --
	
	use the following example to access the elements of the matrix
	
	M3tMatrix4x3*	matrix;
	
	matrix->m[0][0] is a;
	matrix->m[1][0] is b;
	matrix->m[2][0] is c;
	matrix->m[3][0] is d;
	
	...
	
	matrix->m[0][2] is i;
	matrix->m[1][2] is j;
	matrix->m[2][2] is k;
	matrix->m[3][2] is l;
*/
	typedef tm_struct M3tMatrix4x3
	{
		float m[4][3];

	} M3tMatrix4x3;

	typedef tm_struct M3tMatrix3x3
	{
		float m[3][3];

	} M3tMatrix3x3;

	typedef tm_struct M3tMatrix4x4
	{
		float m[4][4];

	} M3tMatrix4x4;

	typedef tm_struct M3tEdge
	{
		M3tPoint3D a;
		M3tPoint3D b;
	} M3tEdge;

	typedef enum M3tBBoxSide
	{
		M3cBBoxSide_PosX,
		M3cBBoxSide_PosY,
		M3cBBoxSide_PosZ,
		M3cBBoxSide_NegX,
		M3cBBoxSide_NegY,
		M3cBBoxSide_NegZ,
		M3cBBoxSide_None
	} M3tBBoxSide;
	
	// cylinder
	// Etymology: Middle French or Latin; Middle French cylindre, from Latin cylindrus, 
	// from Greek kylindros, from kylindein to roll; perhaps akin to Greek kyklos wheel 
	
	typedef tm_struct M3tBoundingCircle
	{
		M3tPoint2D	center;
		float		radius;
	} M3tBoundingCircle;
	
	typedef tm_struct M3tBoundingCylinder
	{
		M3tBoundingCircle	circle;
		float		top;
		float		bottom;
	} M3tBoundingCylinder;
	
	typedef tm_struct M3tBoundingBox_MinMax
	{
		M3tPoint3D	minPoint;
		M3tPoint3D	maxPoint;
		
	} M3tBoundingBox_MinMax;
	
	#define M3cBoundingBox_Edge_Count 12
	typedef tm_struct M3tBoundingBox_Edge
	{
		M3tEdge edges[12];
	} M3tBoundingBox_Edge;
	
	typedef tm_struct M3tBoundingBox
	{
		M3tPoint3D	localPoints[8];
		
	} M3tBoundingBox;
	
	
	#define M3cNumBoundingPoints 8		// Do not change without changing references below
	#define M3cNumBoundingFaces 6		// Do not change without changing references below
	typedef tm_struct M3tBoundingVolume	
	{
		M3tPoint3D			worldPoints[8];	// Must match M3cNumBoundingPoints above
		M3tQuad				faces[6];		// Must match M3cNumBoundingFaces above
		M3tVector3D			normals[6];		// Must match M3cNumBoundingFaces above		- starting normals
		
		M3tPlaneEquation	curPlanes[6];	// Must match M3cNumBoundingFaces above	- current plane equs
		UUtUns16			curProjections[6]; // Must match M3cNumBoundingFaces above
	
	} M3tBoundingVolume;
	
	
	
	typedef tm_struct M3tBoundingSphere
	{
		M3tPoint3D	center;
		float		radius;
	} M3tBoundingSphere;
	
	
	// hotdog (path carved by moving sphere)
	// Etymology: American, from Latin sweatus caninus, 
	// from Greek baseballos snackos, from kholesterol to grow wide; perhaps akin to Greek rapidos foodos. 
	typedef tm_struct M3tBoundingHotdog
	{
		M3tPoint3D	start;
		M3tPoint3D	end;
		float		radius;
		
	} M3tBoundingHotdog;


/*
 * 
 */
	typedef tm_struct M3tColorRGB
	{
		float	r;
		float	g;
		float	b;
		
	} M3tColorRGB;

	typedef struct M3tLight_Ambient
	{
		M3tColorRGB	color;
		
	} M3tLight_Ambient;

	typedef struct M3tLight_Directional
	{
		M3tVector3D	direction;
		M3tColorRGB	color;
		
	} M3tLight_Directional;

	typedef struct M3tLight_Point
	{
		M3tPoint3D	location;
		float		a, b, c;	// attenuation = a/(d*d) + b/d + c
		M3tColorRGB	color;
		
	} M3tLight_Point;

	typedef struct M3tLight_Cone
	{
		M3tPoint3D	location;
		M3tVector3D	direction;
		float		hotAngle; 		// max intensity within this angle
		float		falloffAngle; 	// no intensity outside this angle
		float		a, b, c;		// attenuation = a/(d*d) + b/d + c
		M3tColorRGB	color;
		
	} M3tLight_Cone;

/*
 * Define some types for object storage
 */

	typedef tm_struct M3tEdgeIndex
	{
		UUtUns16	indices[2];
		UUtUns16	triA;
		UUtUns16	triB;
		
	} M3tEdgeIndex;
	
/*
 * 
 */
	typedef enum M3tGeomStateIntType
	{
		M3cGeomStateIntType_Fill,
		M3cGeomStateIntType_Shade,
		M3cGeomStateIntType_Appearance,
		M3cGeomStateIntType_Hint,
		M3cGeomStateIntType_SubmitMode,
		M3cGeomStateIntType_Alpha,			// range is 0 - 255
		M3cGeomStateIntType_SpriteMode,		
		M3cGeomStateIntType_DebugMode,
		M3cGeomStateIntType_FastMode,
		M3cGeomStateIntType_NumTypes
	} M3tGeomStateIntType;
	
/*
 * 
 */
	typedef enum M3tGeomState_Fill
	{
		M3cGeomState_Fill_Point,
		M3cGeomState_Fill_Line,
		M3cGeomState_Fill_Solid
		
	} M3tGeomState_Fill;

/*
 * 
 */
	typedef enum M3tGeomState_Shade
	{
		M3cGeomState_Shade_Vertex,
		M3cGeomState_Shade_Face
		
	} M3tGeomState_Shade;

/*
 * 
 */
	typedef enum M3tGeomState_Appearance
	{
		M3cGeomState_Appearance_Gouraud,
		M3cGeomState_Appearance_Texture
		
	} M3tGeomState_Appearance;

/*
 * 
 */
	typedef enum M3tGeomState_Hint
	{
		M3cGeomState_Hint_None,
		M3cGeomState_Hint_NoClip
		
	} M3tGeomState_Hint;

/*
 * 
 */
	typedef enum M3tGeomState_SubmitMode
	{
		M3cGeomState_SubmitMode_Normal,
		M3cGeomState_SubmitMode_SortAlphaTris
		
	} M3tGeomState_SubmitMode;

/*
 *
 */

	// CB: there are now multiple modes to draw sprites which possess an orientation
	typedef enum M3tGeomState_SpriteMode
	{
		M3cGeomState_SpriteMode_Normal,		// faces camera, oriented to screen X + Y
		M3cGeomState_SpriteMode_Rotated,	// faces camera, rotated in screenspace to match direction vector
		M3cGeomState_SpriteMode_Billboard,	// parallel to direction, rotates about direction to face camera
		M3cGeomState_SpriteMode_Arrow,		// oriented horizontally along direction, vertically along upvector
		M3cGeomState_SpriteMode_Flat,		// oriented perpendicular to direction
		M3cGeomState_SpriteMode_Contrail,
		M3cGeomState_SpriteMode_ContrailRotated,		// these two are not sprites and are handled by M3rDraw_Contrail
		M3cGeomState_SpriteMode_Discus
	} M3tGeomState_SpriteMode;

	// CB: contrails need to store width, tint and alpha at each point along
	// their length
	typedef struct M3tContrailData {
		M3tPoint3D		position;
		M3tVector3D		width;
		IMtShade		tint;
		UUtUns16		alpha;
	} M3tContrailData;

/*
 * 
 */
	typedef enum M3tGeomState_DebugMode
	{
		M3cGeomState_DebugMode_None				= 0,
		M3cGeomState_DebugMode_UseEnvDbgTexture	= (1 << 0),
		M3cGeomState_DebugMode_DrawGhostGQs		= (1 << 1)
		
	} M3tGeomState_DebugMode;
	
/*
 * 
 */
	typedef enum M3tDrawStatePtrType
	{
		M3cDrawStatePtrType_ScreenPointArray,
		M3cDrawStatePtrType_ScreenShadeArray_DC,
		M3cDrawStatePtrType_TextureCoordArray,		// This list must have an extra MScExtraTextureCoords coords for clipping
		M3cDrawStatePtrType_EnvTextureCoordArray,		// This list must have an extra MScExtraTextureCoords coords for clipping

		M3cDrawStatePtrType_BaseTextureMap,
		M3cDrawStatePtrType_LightTextureMap,
		M3cDrawStatePtrType_EnvTextureMap,
		
		M3cDrawStatePtrType_VertexBitVector,		// This list indicates which vertices will actually be used
		
		M3cDrawStatePtrType_NumTypes
		
	} M3tDrawStatePtrType;

/*
 * 
 */
	typedef enum M3tDrawStateIntType
	{
		M3cDrawStateIntType_Appearence,
		M3cDrawStateIntType_Interpolation,
		M3cDrawStateIntType_Fill,
		M3cDrawStateIntType_ZCompare,
		M3cDrawStateIntType_ZWrite,
		M3cDrawStateIntType_ZBias,
		M3cDrawStateIntType_NumRealVertices,		// This is the number of vertices not including extra for clipping
		M3cDrawStateIntType_ConstantColor,
		
		M3cDrawStateIntType_SubmitMode,
		M3cDrawStateIntType_Alpha,					// range is 0 to 255
		M3cDrawStateIntType_Time,					// time in 60ths, used for animated textures
		M3cDrawStateIntType_TextureInstance,		// used to identify a particular instance of a texture (random start)
		M3cDrawStateIntType_BufferClear,			// 1 = buffer clear, 0 = dont buffer clear
		M3cDrawStateIntType_DoubleBuffer,			// 1 = double buffer, 0 = single buffer	
		M3cDrawStateIntType_Clipping,				// 1 = clipping on, 0 = no clipping

		M3cDrawStateIntType_Fog, // S.S. 1 = enable fog, 0 = disable fog; initial state is on
		M3cDrawStateIntType_FrameBufferBlendWithConstantAlpha, // S.S. the name says it all; boolean value
		
		M3cDrawStateIntType_VertexFormat,
		M3cDrawStateIntType_ClearColor,
				
		M3cDrawStateIntType_NumTypes
		
	} M3tDrawStateIntType;

/*
 * 
 */
typedef struct M3tDrawContext_Counters
{
	UUtUns32	textureDownload;
	UUtUns32	numTris;
	UUtUns32	numTriSplits;
	UUtUns32	numQuads;
	UUtUns32	numQuadSplits;
	UUtUns32	numPents;
	UUtUns32	numPentSplits;
	UUtUns32	numSprites;
	UUtUns32	numAlphaSortedObjs;
	
} M3tDrawContext_Counters;

/*
 * 
 */
	typedef enum M3tDrawStateAppearence
	{
		M3cDrawState_Appearence_Gouraud,
		M3cDrawState_Appearence_Texture_Lit,
		M3cDrawState_Appearence_Texture_Lit_EnvMap,
		M3cDrawState_Appearence_Texture_Unlit,

		M3cDrawState_Appearence_Num
		
	} M3tDrawStateAppearence;
	
/*
 * 
 */
	typedef enum M3tDrawState_Fill
	{
		M3cDrawState_Fill_Point,
		M3cDrawState_Fill_Line,
		M3cDrawState_Fill_Solid,
		
		M3cDrawState_Fill_Num
		
	} M3tDrawState_Fill;
/*
 * 
 */
	typedef enum M3tDrawStateInterpolation
	{
		M3cDrawState_Interpolation_None,
		M3cDrawState_Interpolation_Vertex,
		
		M3cDrawState_Interpolation_Num
		
	} M3tDrawStateInterpolation;
	
/*
 * 
 */
	typedef enum M3tDrawStateZWrite
	{
		M3cDrawState_ZWrite_Off,
		M3cDrawState_ZWrite_On
		
	} M3tDrawStateZWrite;

/*
 * 
 */
	typedef enum M3tDrawStateZCompare
	{
		M3cDrawState_ZCompare_Off,
		M3cDrawState_ZCompare_On
		
	} M3tDrawStateZCompare;

/*
 * 
 */
	typedef enum M3tDrawStateBufferClear
	{
		M3cDrawState_BufferClear_Off,
		M3cDrawState_BufferClear_On
		
	} M3tDrawStateBufferClear;
	
/*
 * 
 */
	typedef enum M3tDrawStateDoubleBuffer
	{
		M3cDrawState_DoubleBuffer_Off,
		M3cDrawState_DoubleBuffer_On
		
	} M3tDrawStateDoubleBuffer;

/*
 * 
 */
	typedef enum M3tDrawStateSubmitMode
	{
		M3cDrawState_SubmitMode_Normal,
		M3cDrawState_SubmitMode_SortAlphaTris
		
	} M3tDrawStateSubmitMode;
	
/*
 * 
 */
	typedef enum M3tDrawStateVertexFormat
	{
		M3cDrawStateVertex_Unified,			// a single index specifies both xyz, rgb, and uv
		M3cDrawStateVertex_Split,
		
		M3cDrawStateVertex_Num
		
	} M3tDrawStateVertexFormat;

/*
 * 
 */

	typedef enum M3tDrawStateFogDisable // S.S.
	{
		M3cDrawStateFogDisable,
		M3cDrawStateFogEnable,

		M3cDrawStateFog_Num

	} M3tDrawStateFogDisable;
	
/*
 * 
 */
	#define M3cTemplate_PlaneEquationArray	UUm4CharToUns32('P', 'L', 'E', 'A')
	typedef tm_template('P', 'L', 'E', 'A', "Plane Equation Array")
	M3tPlaneEquationArray
	{
		tm_pad							pad0[20];
		
		tm_varindex	UUtUns32			numPlanes;
		tm_vararray M3tPlaneEquation	planes[1];
		
	} M3tPlaneEquationArray;

/*
 * 
 */
	#define M3cTextureMap_MaxWidth		256
	#define M3cTextureMap_MaxHeight		256

	enum {
		M3cTextureFlags_None				= 0,
		M3cTextureFlags_HasMipMap			= (1 << 0),
		M3cTextureFlags_Trilinear			= (1 << 1),		// just good trilinear candidate (dither would be ok)
		M3cTextureFlags_ClampHoriz			= (1 << 2),
		M3cTextureFlags_ClampVert			= (1 << 3),
		M3cTextureFlags_PixelType			= (1 << 4),		// only used when importing
		M3cTextureFlags_Offscreen			= (1 << 5),		// this texture will never be drawn or loaded on a card
		M3cTextureFlags_Anim_PingPong		= (1 << 6),		// cycle through the anim back and forth
		M3cTextureFlags_Anim_RandomFrame	= (1 << 7),		// pick random frames
		M3cTextureFlags_Anim_RandomStart	= (1 << 8),
		M3cTextureFlags_ReceivesEnvMap		= (1 << 9),
		M3cTextureFlags_Blend_Additive		= (1 << 10),
		M3cTextureFlags_SelfIlluminent		= (1 << 11),
		M3cTextureFlags_LittleEndian		= (1 << 12),
		M3cTextureFlags_Prepared			= (1 << 13),
		M3cTextureFlags_Anim_DontAnimate	= (1 << 14),
		M3cTextureFlags_MagicShield_Blue	= (1 << 15),
		M3cTextureFlags_MagicShield_Invis	= (1 << 16),
		M3cTextureFlags_MagicShield_Daodan	= (1 << 17)
	};

	typedef struct M3tTextureMap M3tTextureMap;

	#define M3cTemplate_TextureMap	UUm4CharToUns32('T', 'X', 'M', 'P')
	tm_template('T', 'X', 'M', 'P', "Texture Map")
	M3tTextureMap
	{
		// implied 8 bytes here
		
		//char					pad[28];
		
		char					debugName[128];
		
		UUtUns32				flags;
		UUtUns16				width;
		UUtUns16				height;
		IMtPixelType			texelType;
		tm_templateref			animation;
		M3tTextureMap*			envMap;			// The environment map that is tied to this texture map
		
		tm_raw(void *)			pixels;			// CB: if this is non-NULL then it is either 1) a special texture that is saved as raw data and
												// not separate data during the importer (there are none of these textures at present)
												// or 2) a texture that was created at runtime.

		tm_separate				pixelStorage;	// CB: this is used as an index into the separate data file only if pixels is NULL which indicates
												// that we do not have any permanently-resident data for this texture

		UUtUns32				materialType;

		UUtUns32				opengl_texture_name;
		UUtBool					opengl_dirty;
		tm_pad					pad1[3]; // S.S. I am using these pad bytes internally so don't mess with them without making sure they aren't used
		
	};

	#define M3cTemplate_TextureMapAnimation	UUm4CharToUns32('T', 'X', 'A', 'N')
	typedef tm_template('T', 'X', 'A', 'N', "Texture Map Animation")
	M3tTextureMapAnimation
	{
		tm_pad						pad0[12];

		UUtUns16					timePerFrame;	// in 60th sec
		
		UUtUns16					randTPF_low;
		UUtUns16					randTPF_range;
		
		tm_pad						pad1[2];

		tm_varindex	UUtUns32		numFrames;
		tm_vararray M3tTextureMap	*maps[1];
	} M3tTextureMapAnimation;


	// ----------------------------------------------------------------------
	// M3tTextureMap_Big holds a list of textures in which each one holds
	// a piece of a larger texture map
	// ----------------------------------------------------------------------
	#define M3cTemplate_TextureMap_Big	UUm4CharToUns32('T', 'X', 'M', 'B')
	typedef tm_template('T', 'X', 'M', 'B', "Texture Map Big")
	M3tTextureMap_Big
	{
		tm_pad						pad0[8];
		
		UUtUns16					width;
		UUtUns16					height;
		IMtPixelType				texelType;
		
		UUtUns16					num_x;
		UUtUns16					num_y;

		tm_varindex UUtUns32		num_textures;
		tm_vararray M3tTextureMap	*textures[1];
		
	} M3tTextureMap_Big;
	
	#define M3cTemplate_TextureMap_Proc	UUm4CharToUns32('T', 'X', 'P', 'C')
	typedef tm_template('T','X','P','C', "Texture Procedure Data")
	M3tTextureMap_Proc
	{
		tm_pad					pad0[8];
		
		UUtUns32				nextFrameTime;
		UUtUns32				speed;
		UUtUns32				curTexture;
		
		tm_varindex UUtUns32	numTextures;
		tm_vararray UUtUns32	textureIndices[1];
	} M3tTextureMap_Proc;
	
/*
 * 
 */
	#define M3cTemplate_Point3DArray	UUm4CharToUns32('P', 'N', 'T', 'A')
	
	typedef tm_template('P', 'N', 'T', 'A', "3D Point Array")
	M3tPoint3DArray
	{
		tm_pad					pad0[12];

		M3tBoundingBox_MinMax	minmax_boundingBox;
		M3tBoundingSphere		boundingSphere;
		
		tm_varindex	UUtUns32	numPoints;
		tm_vararray M3tPoint3D	points[1];
		
	} M3tPoint3DArray;
	
/*
 * 
 */
	#define M3cTemplate_Vector3DArray	UUm4CharToUns32('V', 'C', 'R', 'A')
	
	typedef tm_template('V', 'C', 'R', 'A', "3D Vector Array")
	M3tVector3DArray
	{
		tm_pad					pad0[20];
		
		tm_varindex	UUtUns32	numVectors;
		tm_vararray M3tVector3D	vectors[1];
		
	} M3tVector3DArray;
	
/*
 * 
 */
	#define M3cTemplate_ColorRGBArray	UUm4CharToUns32('3', 'C', 'L', 'A')
	
	typedef tm_template('3', 'C', 'L', 'A', "RGB Color Array")
	M3tColorRGBArray
	{
		tm_pad					pad0[20];
		
		tm_varindex	UUtUns32	numColors;
		tm_vararray	M3tColorRGB	colors[1];
		
	} M3tColorRGBArray;

/*
 * 
 */
	#define M3cTemplate_EdgeIndexArray	UUm4CharToUns32('E', 'D', 'I', 'A')
	
	typedef tm_template('E', 'D', 'I', 'A', "Edge Index Array")
	M3tEdgeIndexArray
	{
		tm_pad						pad0[20];
		
		tm_varindex	UUtUns32		numEdges;
		tm_vararray	M3tEdgeIndex	edgeIndices[1];
		
	} M3tEdgeIndexArray;

/*
 * 
 */
	#define M3cTemplate_TextureCoordArray	UUm4CharToUns32('T', 'X', 'C', 'A')
	
	typedef tm_template('T', 'X', 'C', 'A', "Texture Coordinate Array")
	M3tTextureCoordArray
	{
		tm_pad						pad0[20];
		
		tm_varindex	UUtUns32		numTextureCoords;
		tm_vararray	M3tTextureCoord	textureCoords[1];	// This list must have an extra MScExtraTextureCoords coords for clipping
		
	} M3tTextureCoordArray;

/*
 * 
 */
	typedef tm_enum M3tGeometryFlags
	{
		M3cGeometryFlag_None			= 0x00,
		M3cGeometryFlag_RemoveBackface	= 0x01,	// Remove backfacing faces
		M3cGeometryFlag_ComputeSpecular	= 0x02,	// Compute specular highlights
		M3cGeometryFlag_ComputeDiffuse	= 0x04,	// Compute diffuse component
		M3cGeometryFlag_HasAmbient		= 0x08,	// Object has an ambient color
		M3cGeometryFlag_SelfIlluminent	= 0x10,	// Object is self-illuminent (do not shade) 
		M3cGeometryFlag_NoQuads			= 0x20	// Object has no quads defined (only points)
	} M3tGeometryFlags;
		
/*
 * 
 */
	#define M3cTemplate_TriArray	UUm4CharToUns32('M', '3', 'T', 'A')
	
	typedef tm_template('M', '3', 'T', 'A', "Triangle array")
	M3tTriArray
	{
		tm_pad					pad0[20];
		
		tm_varindex	UUtUns32	numTris;
		tm_vararray	M3tTri		tris[1];
		
	} M3tTriArray;

/*
 * 
 */
	#define M3cTemplate_QuadArray	UUm4CharToUns32('Q', 'U', 'D', 'A')
	
	typedef tm_template('Q', 'U', 'D', 'A', "Quad array")
	M3tQuadArray
	{
		tm_pad					pad0[20];
		
		tm_varindex	UUtUns32	numQuads;
		tm_vararray	M3tQuad		quads[1];
		
	} M3tQuadArray;

/*
 * 
 */
	#define M3cTemplate_TextureMapArray	UUm4CharToUns32('T', 'X', 'M', 'A')
	
	typedef tm_template('T', 'X', 'M', 'A', "Texture map array")
	M3tTextureMapArray
	{
		tm_pad							pad0[20];

		tm_varindex	UUtUns32			numMaps;
		tm_vararray	M3tTextureMap*		maps[1];	
	} M3tTextureMapArray;
	
/*
 *
 */
	#define M3cTemplate_Geometry	UUm4CharToUns32('M', '3', 'G', 'M')
	
	typedef tm_template('M', '3', 'G', 'M', "Geometry")
	M3tGeometry
	{
		M3tGeometryFlags		geometryFlags;
		
		M3tPoint3DArray*		pointArray;			
		M3tVector3DArray*		vertexNormalArray;	
		M3tVector3DArray*		triNormalArray;
		M3tTextureCoordArray*	texCoordArray;		// This list must have an extra MScExtraTextureCoords coords for clipping
													
		TMtIndexArray*			triStripArray;
		TMtIndexArray*			triNormalIndexArray;	// This also indicates the number of tris in the object
		
		M3tTextureMap*			baseMap;			// This is also the source of the environment map
		
		tm_templateref			animation;
		
	} M3tGeometry;

	#define M3cTemplate_GeometryAnimation	UUm4CharToUns32('G', 'M', 'A', 'N')
	typedef tm_template('G', 'M', 'A', 'N', "Geometry Animation")
	M3tGeometryAnimation
	{
		tm_pad						pad0[12];

		UUtUns16					timePerFrame;	// in 60th sec
		UUtUns16					randTPF_low;
		UUtUns16					randTPF_range;
		tm_pad						pad1[2];
		
		tm_varindex	UUtUns32		numFrames;
		tm_vararray M3tGeometry*	frames[1];
		
	} M3tGeometryAnimation;

	#define M3cTemplate_GeometryArray	UUm4CharToUns32('M', '3', 'G', 'A')
	typedef tm_template('M', '3', 'G', 'A', "GeometryArray")
	M3tGeometryArray
	{
		tm_pad						pad0[20];

		tm_varindex UUtUns32		numGeometries;
		tm_vararray M3tGeometry		*geometries[1];
	} M3tGeometryArray;

	typedef tm_struct M3tSpriteInstance
	{
		M3tPoint3D					position;
		float						scale;
		UUtUns8						alpha;
		UUtUns8						max_alpha;
	} M3tSpriteInstance;

	typedef tm_struct M3tSpriteArray
	{
		UUtUns32					sprite_count;
		M3tTextureMap				*texture_map;
		M3tSpriteInstance			sprites[1];
	} M3tSpriteArray;

/*
 * 
 */
	typedef struct M3tDiffuseRGB
	{
		UUtUns32		diffuseRGB;

	} M3tDiffuseRGB;
 
/*
 * Draw Engine Names
 */
	#define M3cDrawEngine_Software		"Oni Software"
	#define M3cDrawEngine_3DFX_Glide2x	"3DFX Glide 2x"
	#define M3cDrawEngine_D3D			"Direct3D"
	#define M3cDrawEngine_RAVE			"RAVE"
	#define M3cDrawEngine_OpenGL		"OpenGL"
	
/*
 * Geom Engine IDs
 */
	#define M3cGeomEngine_Software		"Oni Software"
	
/*
 * Draw Engine cap list structure
 */

	typedef enum M3tDrawEngineFlags
	{
		M3cDrawEngineFlag_None					= 0,
		M3cDrawEngineFlag_CanHandleOffScreen	= (1 << 0),
		M3cDrawEngineFlag_3DOnly				= (1 << 1),
		M3cDrawEngineFlag_CanSort				= (1 << 2)
		
	} M3tDrawEngineFlags;
	
	typedef struct M3tDisplayMode
	{
		UUtUns16	width;
		UUtUns16	height;
		UUtUns16	bitDepth;
		UUtUns16	platformData;
	} M3tDisplayMode;
	
	typedef struct M3tDisplayDevice
	{
		M3tPlatformDevice	platformDevice;
		
		UUtUns16			numDisplayModes;
		M3tDisplayMode		displayModes[M3cMaxDisplayModes];
		
	} M3tDisplayDevice;
	
	typedef struct M3tDrawEngineCaps
	{
		M3tDrawEngineFlags	engineFlags;
		
		char				engineName[M3cMaxNameLen];
		char				engineDriver[M3cMaxNameLen];
		
		UUtUns32			engineVersion;
		
		UUtUns32			numDisplayDevices;
		M3tDisplayDevice	displayDevices[M3cMaxDisplayDevices];
		
		void*				enginePrivate;
		
	} M3tDrawEngineCaps;
	
/*
 * Geom engine cap list structure
 */
	typedef enum M3tGeomEngineFlags
	{
		M3tGeomEngineFlag_None	= 0
		
	} M3tGeomEngineFlags;
	
	typedef struct M3tGeomEngineCaps
	{
		M3tGeomEngineFlags	engineFlags;
		
		char				engineName[M3cMaxNameLen];
		char				engineDriver[M3cMaxNameLen];
		
		UUtUns32			engineVersion;
		UUtUns32			compatibleDrawEngineBV;
		
	} M3tGeomEngineCaps;
	
/*
 * Data structures for creating contexts
 */
	typedef enum M3tDrawContextType
	{
		M3cDrawContextType_OffScreen,
		M3cDrawContextType_OnScreen
		
	} M3tDrawContextType;
	
	
	typedef struct M3tDrawContextDescriptorOffScreen
	{
		UUtUns16	inWidth;
		UUtUns16	inHeight;
		
		void*		outBaseAddr;		/* This is returned by the engine */
		UUtUns32	outRowBytes;		/* This is returned by the engine */
		
	} M3tDrawContextDescriptorOffScreen;
	
	typedef struct M3tDrawContextDescriptorOnScreen
	{
		UUtAppInstance				appInstance;
		UUtWindow					window;	// Must live on the active device
		UUtRect						rect;	// Must live on the active device
		
	} M3tDrawContextDescriptorOnScreen;
	
	typedef struct M3tDrawContextDescriptor
	{
		M3tDrawContextType	type;
		
		union
		{
			M3tDrawContextDescriptorOffScreen	offScreen;
			M3tDrawContextDescriptorOnScreen	onScreen;
			
		} drawContext;
		
	} M3tDrawContextDescriptor;

/*
 * Parameter block structures
 */
	typedef struct M3tTriSplit
	{
		M3tTri				vertexIndices;
		M3tTri				baseUVIndices;
		UUtUns32			shades[3];	
	} M3tTriSplit;
	
	typedef tm_struct M3tQuadSplit
	{
		M3tQuad				vertexIndices;
		M3tQuad				baseUVIndices;
		UUtUns32			shades[4];		
	} M3tQuadSplit;
	
	typedef struct M3tPentSplit
	{
		M3tPent				vertexIndices;
		M3tPent				baseUVIndices;
		UUtUns32			shades[5];				
	} M3tPentSplit;
	
	
/*
 * Real function prototypes for the draw engine interface
 */
	
	/*
	 * Engine caps and display mode stuff
	 */

		UUtError M3rDrawEngine_FindGrayscalePixelType(
			IMtPixelType		*outTextureFormat);
		
		UUtUns16
		M3rDrawEngine_GetNumber(
			void);
		
		M3tDrawEngineCaps*
		M3rDrawEngine_GetCaps(
			UUtUns16	inDrawEngineIndex);
	
		UUtUns16
		M3rGeomEngine_GetNumber(
			void);
		
		M3tGeomEngineCaps*
		M3rGeomEngine_GetCaps(
			UUtUns16	inGeomEngineIndex);
	
	/*
	 * Specify the active engine, display device, and mode
	 */
	UUtError
	M3rDrawEngine_MakeActive(
		UUtUns16				inDrawEngineIndex,		// Index into the engine list
		UUtUns16				inDisplayDeviceIndex,	// The display to use
		UUtBool					inFullScreen,
		UUtUns16				inDisplayModeIndex);	// Only relevent for full screen mode
	
	UUtError
	M3rDrawEngine_GetActiveDeviceRect(
		UUtRect		*outDeviceRect);
		
	/*
	 * Draw Context functions, use these ONLY if you are using the rasterizer directly
	 */
	
		UUtError 
		M3rDrawContext_New(
			M3tDrawContextDescriptor*	inDrawContextDescriptor);
		
		void
		M3rDrawContext_Delete(
			void);
	
	/*
	 * Draw Context texture stuff
	 */
		void
		M3rDrawContext_ResetTextures(
			void);
	
/*
 * Real function prototypes for the geom engine interface
 */
	
	/*
	 * Service query functions
	 */
		UUtUns16
		M3rGeomEngine_GetNumber(
			void);
		
		M3tGeomEngineCaps*
		M3rGeomEngine_GetCaps(
			UUtUns16	inGeomEngineIndex);

		UUtError
		M3rGeomEngine_MakeActive(
			UUtUns16	inGeomEngineIndex);
		
	/*
	 * Geom Context functions
	 */
	
	UUtError 
	M3rGeomContext_New(
		M3tDrawContextDescriptor*	inDrawContextDescriptor);
	
        struct AKtEnvironment;
	UUtError
	M3rGeomContext_SetEnvironment(
		struct AKtEnvironment*		inEnvironment);
		
	void
	M3rGeomContext_Delete(
		void);

/*
 * Collision functiions
 */

   UUtBool M3rIntersect_GeomGeom(
	   const M3tMatrix4x3*		inMatrixA,
	   const M3tGeometry*		inGeometryA,
	   const M3tMatrix4x3*		inMatrixB,
	   const M3tGeometry*		inGeometryB);

   UUtBool M3rIntersect_BBox(
			const M3tMatrix4x3		*inMatrixA,
			const M3tBoundingBox	*inBoxA,
			const M3tMatrix4x3		*inMatrixB,
			const M3tBoundingBox	*inBoxB);

/*
 * typedefs for draw engine functions
 */
	
	/*
	 * Frame functions
	 */	
		UUtError 
		M3rDraw_Frame_Start(
			UUtUns32			inGameTime);
			
		UUtError 
		M3rDraw_Frame_End(
			void);

		UUtError 
		M3rDraw_Frame_Sync(
			void);

		void M3rDraw_Reset_Fog(void); // S.S.

	/*
	 * Texture loading
	 */
		void M3rDraw_Texture_EnsureLoaded(M3tTextureMap *texture_map);  // GME
		UUtBool M3rDraw_Texture_Load(M3tTextureMap *texture_map); // S.S.
		UUtBool M3rDraw_Texture_Unload(M3tTextureMap *texture_map); // S.S.
	
	/*
	 * Bitmap drawing
	 */
		extern M3tTextureCoord sprite_uv[4];			

		void
		M3rDraw_TriSprite(
			const M3tPointScreen	*inPoints,			// points[3]
			const M3tTextureCoord	*inTextureCoords);	// UVs[3]
	
		void
		M3rDraw_Sprite(
			const M3tPointScreen	*inPoints,			// topleft, botright
			const M3tTextureCoord	*inTextureCoords);	// topleft, topright, botleft, botright

		void
		M3rDraw_SpriteArray(
			const M3tPointScreen	*inPoints,			// topleft, botright
			const M3tTextureCoord	*inTextureCoords,	// topleft, topright, botleft, botright
			const UUtUns32			*inColors,
			const UUtUns32			inCount );

		void
		M3rDraw_Sprite_Debug(
			const M3tTextureMap*		inTextureMap,
			const M3tPointScreen*		inTopLeft,
			UUtInt32					inStartX,
			UUtInt32					inStartY,
			UUtInt32					inWidth,
			UUtInt32					inHeight);

		UUtError 
		M3rDraw_ScreenCapture(
			const UUtRect*	inRect, 
			void*			outBuffer);
	

		UUtBool 
		M3rDraw_PointVisible(
			const M3tPointScreen	*inPoint,
			float					inTolerance);

		float
		M3rDraw_PointVisibleScale(
			const M3tPointScreen	*inPoint,
			M3tPoint2D				*inTestOffsets,
			UUtUns32				inTestOffsetCount );

		UUtBool 
		M3rDraw_SupportPointVisible(
			void);

	/*
	 * Query functions
	 */
		UUtBool
		M3rDraw_TextureFormatAvailable(
			IMtPixelType	inTexelType);


	/*
	 * Resolution functions
	 */
			
		UUtError
		M3rDraw_SetResolution(M3tDisplayMode mode);
		
	/*
	 * Draw engine state
	 */
			
		UUtUns16
		M3rDraw_GetWidth(
			void);
			
		UUtUns16
		M3rDraw_GetHeight(
			void);
		
		void
		M3rDraw_State_SetInt(
			M3tDrawStateIntType	inDrawStateType,
			UUtInt32			inDrawState);
		
		UUtInt32
		M3rDraw_State_GetInt(
			M3tDrawStateIntType	inDrawStateType);
			
		void
		M3rDraw_State_SetPtr(
			M3tDrawStatePtrType	inDrawStateType,
			void*				inDrawState);
		
		void*
		M3rDraw_State_GetPtr(
			M3tDrawStatePtrType	inDrawStateType);
			
		UUtError
		M3rDraw_State_Push(
			void);
			
		UUtError
		M3rDraw_State_Pop(
			void);
			
		UUtError
		M3rDraw_State_Commit(
			void);
			
		const M3tDrawContext_Counters*
		M3rDraw_Counters_Get(
			void);

/*
 * typedefs for geom engine functions
 */
	
	/*
	 * Frame functions
	 */	
		typedef UUtError 
		(*M3tGeomContextMethod_Frame_Start)(
			UUtUns32			inGameTicksElapsed);
			
		typedef UUtError 
		(*M3tGeomContextMethod_Frame_End)(
			void);
			
		UUtError 
		M3rGeom_Frame_Start(
			UUtUns32			inGameTicksElapsed);
			
		UUtError
		M3rGeom_Draw_Environment_Alpha(
			void);

		UUtError 
		M3rGeom_Frame_End(
			void);
			
		void
		M3rGeom_Clear_Jello(
			void);

	/*
	 * Pick functions
	 */
		typedef void
		(*M3tGeomContextMethod_Pick_ScreenToWorld)(
			M3tGeomCamera*		inCamera,
			UUtUns16			inScreenX,
			UUtUns16			inScreenY,
			M3tPoint3D			*outWorldZNearPoint);
				
	/*
	 * Light functions
	 */

		typedef void
		(*M3tGeomContextMethod_LightList_Ambient)(
			M3tLight_Ambient*		inLightList);

		typedef void
		(*M3tGeomContextMethod_LightList_Directional)(
			UUtUns32				inNumLights,
			M3tLight_Directional*	inLightList);

		typedef void
		(*M3tGeomContextMethod_LightList_Point)(
			UUtUns32				inNumLights,
			M3tLight_Point*			inLightList);

		typedef void
		(*M3tGeomContextMethod_LightList_Cone)(
			UUtUns32				inNumLights,
			M3tLight_Cone*			inLightList);


	/*
	 * Sprites
	 */

		// CB: added parameters inRotation, inDirection and inOrientation for drawing oriented
		// sprites. if M3cGeomState_SpriteMode_Normal then both direction and orientation may be NULL.
		// the other sprite modes require one or the other to be valid.
		//
		// inDirection is assumed not to be normalised, however inOrientation should be
		// an orthonormal 3x3 matrix
		//
		// CB: added parameters inXOffset, inXShorten and inXChop for drawing sprites where 'inPoint' isn't
		// in the middle of the displayed polygon
		//
		// to draw unoriented sprites just as before, set the new parameters to 0, NULL, NULL, 0, 0, 0
		typedef UUtError
		(*M3tGeomContextMethod_Sprite_Draw)(
			M3tTextureMap*		inTextureMap,
			M3tPoint3D*			inPoint,
			float				horizSize,
			float				vertSize,
			UUtUns32			inShade,
			UUtUns16			inAlpha,
			float				inRotation,
			M3tVector3D*		inDirection,
			M3tMatrix3x3*		inOrientation,
			float				inXOffset,
			float				inXShorten,
			float				inXChop);	

		UUtError
		M3rSprite_Draw(
			M3tTextureMap*		inTextureMap,
			M3tPoint3D*			inPoint,
			float				horizSize,
			float				vertSize,
			UUtUns32			inShade,
			UUtUns16			inAlpha,
			float				inRotation,
			M3tVector3D*		inDirection,
			M3tMatrix3x3*		inOrientation,
			float				inXOffset,
			float				inXShorten,
			float				inXChop);	

		UUtError
		M3rSimpleSprite_Draw(
			M3tTextureMap*		inTextureMap,
			M3tPoint3D*			inPoint,
			float				horizSize,
			float				vertSize,
			UUtUns32			inShade,
			UUtUns16			inAlpha);	

		typedef UUtError
		(*M3tGeomContextMethod_SpriteArray_Draw)(
			M3tSpriteArray		*inSpriteArray );

		UUtError
		M3rSpriteArray_Draw(
			M3tSpriteArray		*inSpriteArray );


	/*
	 * Contrails
	 */

		// CB: this basically draws a sequence of quads and links them up into a strip
		typedef UUtError
		(*M3tGeomContextMethod_Contrail_Draw)(
			M3tTextureMap*		inTextureMap,
			float				inV0,
			float				inV1,
			M3tContrailData*	inPoint0,
			M3tContrailData*	inPoint1);	

		UUtError
		M3rContrail_Draw(
			M3tTextureMap*		inTextureMap,
			float				inV0,
			float				inV1,
			M3tContrailData*	inPoint0,
			M3tContrailData*	inPoint1);	


	/*
	 * Geometry
	 */
		typedef UUtError
		(*M3tGeomContextMethod_Geometry_BoundingBox_Draw)(
			UUtUns32			inShade,
			M3tGeometry*		inGeometryObject);

		typedef UUtError
		(*M3tGeomContextMethod_Geometry_Draw)(
			M3tGeometry*		inGeometryObject);	
	
		UUtError
		M3rGeometry_Draw(
			M3tGeometry*		inGeometryObject);	

		void 
		M3rGeometry_MultiplyAndDraw(
			M3tGeometry *inGeometryObject,
			const M3tMatrix4x3 *inMatrix);

	
	/*
	 * Environment rendering
	 */
		typedef UUtError
		(*M3tGeomContextMethod_Env_SetCamera)(
			M3tGeomCamera*			inCamera);		// If null use active camera in geom context
			
		typedef UUtError
		(*M3tGeomContextMethod_Env_DrawGQList)(
			UUtUns32	inNumGQs,
			UUtUns32*	inGQIndices,
			UUtBool		inTransparentList);
	
		UUtError
		M3rEnv_DrawGQList(
			UUtUns32	inNumGQs,
			UUtUns32*	inGQIndices);
	

	/*
	 * polys, points and lines
	 */
		typedef UUtError
		(*M3tGeomContextMethod_Geometry_PolyDraw)(	// Currently flat only
			UUtUns32			inNumPoints,
			M3tPoint3D*			inPoints,
			UUtUns32			inShade);
		
		typedef UUtError
		(*M3tGeomContextMethod_Geometry_LineDraw)(
			UUtUns32			inNumPoints,
			M3tPoint3D*			inPoints,
			UUtUns32			inShade);
		
		typedef UUtError
		(*M3tGeomContextMethod_Geometry_PointDraw)(
			UUtUns32			inNumPoints,
			M3tPoint3D*			inPoints,
			UUtUns32			inShade);

		typedef UUtBool
		(*M3tGeomContextMethod_PointVisible)(
			M3tPoint3D*			inPoint,
			float				inTolerance);
	
		void
		M3rGeom_Draw_DebugSphere(
			M3tPoint3D *			inPoint,
			float					inRadius,
			IMtShade				inShade);

		UUtBool
		M3rPointVisible(
			M3tPoint3D*			inPoint,
			float				inTolerance);

		typedef float
		(*M3tGeomContextMethod_PointVisibleScale)(
			M3tPoint3D*			inPoint,
			M3tPoint2D*			inTestOffsets,
			UUtUns32			inTestOffsetCount );
	
		float
		M3rPointVisibleScale(
			M3tPoint3D*			inPoint,
			M3tPoint2D*			inTestOffsets,
			UUtUns32			inTestOffsetCount );

/*
 * decals
 */

	typedef struct M3tDecalHeader
	{
		M3tTextureMap			*texture;
		UUtUns32				gq_index;
		UUtUns16				vertex_count;
		UUtUns16				triangle_count;
		UUtUns16				alpha;
		UUtUns16				pad;
		IMtShade				tint;
	} M3tDecalHeader;

	typedef UUtError
	(*M3tGeomContextMethod_Decal_Draw)(
			M3tDecalHeader*		inDecal,
			UUtUns16			inAlpha);

	UUtError
	M3rDecal_Draw( 
			M3tDecalHeader*		inDecal,
			UUtUns16			inAlpha);

/*
 * Camera Functions
 */
	UUtError
	M3rCamera_New(
			M3tGeomCamera*		*outNewCamera);
			
	void
	M3rCamera_Delete(
			M3tGeomCamera*		inCamera);
			
	void
	M3rCamera_SetActive(
			M3tGeomCamera*		inCamera);
			
	UUtError
	M3rCamera_GetActive(
			M3tGeomCamera*		*outCamera);
			
	void 
	M3rCamera_SetStaticData(
			M3tGeomCamera*		inCamera,
			float				inFOVy,
			float				inAspect,
			float				inZNear,
			float				inZFar);
		
	void 
	M3rCamera_SetViewData(
			M3tGeomCamera*		inCamera,
			M3tPoint3D*			inCameraLocation,
			M3tVector3D*		inViewDirection,
			M3tVector3D*		inUpDirection);

	void 
	M3rCamera_GetStaticData(
			M3tGeomCamera*		inCamera,
			float				*outFOVy,
			float				*outAspect,
			float				*outZNear,
			float				*outZFar);
		
	void 
	M3rCamera_GetViewData(
			M3tGeomCamera*		inCamera,
			M3tPoint3D			*outCameraLocation,
			M3tVector3D			*outViewDirection,
			M3tVector3D			*outUpDirection);
	
	void
	M3rCamera_GetViewData_VxU(
			M3tGeomCamera*		inCamera,
			M3tVector3D			*outViewXUp);
		
	/*
	 
	 outPointList mapping
	 
		 index	x		y		z
		 0		left	up		near
		 1		right	up		near
		 2		left	down	near
		 3		right	down	near
		 4		left	up		far
		 5		right	up		far
		 6		left	down	far
		 7		right	down	far
		 
	 outPlaneEquList mapping
	 
	 	index	facing
	 	0		perp to zNear plane(toward camera)
	 	1		right side
	 	2		left side
	 	3		bottom side
	 	4		top side
	 	5		perp to zFar plane(away from camera)
	 	
	 */
	void 
	M3rCamera_GetWorldFrustum(
			M3tGeomCamera*		inCamera,
			M3tPoint3D			*outPointList,
			M3tPlaneEquation	*outPlaneEquList);

	void 
	M3rCamera_DrawWorldFrustum(
			M3tGeomCamera*		inCamera);

	// Sky stuff
		enum
		{
			M3cSkyTop		= 0,
			M3cSkyLeft		= 1,
			M3cSkyRight		= 2,
			M3cSkyFront		= 3,
			M3cSkyBack		= 4,

			M3cMaxSkyboxVerts	=	32
		};

		typedef struct M3tSkyboxData
		{
			float					height;
			M3tTextureMap			*textures[6];
			UUtUns32				indices[6][4];
			M3tPoint3D				cube_points[8];
			M3tTextureCoord			texture_coords[M3cMaxSkyboxVerts];
		} M3tSkyboxData;

		UUtError 
		M3rDraw_Skybox( 
			M3tSkyboxData		*inSkybox );

		UUtError
		M3rDraw_CreateSkybox(
			M3tSkyboxData		*inSkybox,
			M3tTextureMap**		inTextures );

		UUtError
		M3rDraw_DestroySkybox(
			M3tSkyboxData		*inSkybox );

		typedef UUtError 
		(*M3tGeomContextMethod_Skybox_Draw)(
			M3tSkyboxData		*inSkybox );

		typedef UUtError
		(*M3tGeomContextMethod_Skybox_Create)(
			M3tSkyboxData		*inSkybox,
			M3tTextureMap**		inTextures );

		typedef UUtError
		(*M3tGeomContextMethod_Skybox_Destroy)(
			M3tSkyboxData		*inSkybox );


/*
 * Matrix Functions
 */
	UUtError
	M3rMatrixStack_Push(
		void);

	UUtError
	M3rMatrixStack_Get(
		M3tMatrix4x3*		*outMatrix);
	
	UUtError
	M3rMatrixStack_Pop(
		void);
			
	void
	M3rMatrixStack_Identity(
		void);
	
	void 
	M3rMatrixStack_Clear(
		void);

	void 
	M3rMatrixStack_Rotate(
		float				inRadians,
		float				inX,
		float				inY,
		float				inZ);
			
	void
	M3rMatrixStack_UniformScale(
		float				inScale);
			
	void 
	M3rMatrixStack_Translate(
		float				inX,
		float				inY,
		float				inZ);

	void
	M3rMatrixStack_Quaternion(
		const M3tQuaternion*	inQuaternion);
	
	void
	M3rMatrixStack_Multiply(
		const M3tMatrix4x3*		inMatrix);

/*
 * macros for accessing the function pointers
 */
	
	/* macros for geom engines */
	#define M3rMatrixStack_RotateXAxis(radians) \
		M3rMatrixStack_Rotate(radians, 1.0f, 0.0f, 0.0f)

	#define M3rMatrixStack_RotateYAxis(radians) \
		M3rMatrixStack_Rotate(radians, 0.0f, 1.0f, 0.0f)

	#define M3rMatrixStack_RotateZAxis(radians) \
		M3rMatrixStack_Rotate(radians, 0.0f, 0.0f, 1.0f)

	#define M3rMatrixStack_ApplyRotate(rotation) \
		M3rMatrixStack_Rotate(rotation.angle, rotation.axis.x, rotation.axis.y, rotation.axis.z)

	#define M3rMatrixStack_ApplyTranslate(vector) \
		M3rMatrixStack_Translate((vector).x, (vector).y, (vector).z)

	#define M3rPick_ScreenToWorld(camera, screenX, screenY, worldZNearPoint) \
		(M3gGeomContext)->pickScreenToWorld(camera, screenX, screenY, worldZNearPoint)

	#define M3rLightList_Ambient(inLightList) \
		(M3gGeomContext)->lightListAmbient(inLightList)

	#define M3rLightList_Directional(numLights, inLightList) \
		(M3gGeomContext)->lightListDirectional(numLights, inLightList)

	#define M3rLightList_Point(numLights, inLightList) \
		(M3gGeomContext)->lightListPoint(numLights, inLightList)
		
	#define M3rLightList_Cone(numLights, inLightList) \
		(M3gGeomContext)->lightListCone(numLights, inLightList)
		
	#define M3rGeometry_Draw_BoundingBox(rgb55, geometryObject) \
		(M3gGeomContext)->geometryBoundingBoxDraw(rgb55, geometryObject)
		
	#define M3rEnv_SetCamera(inCamera) \
		(M3gGeomContext)->envSetCamera(inCamera)
		
	#define M3rGeometry_PolyDraw(inNumPoints, inPoints, inShade) \
		(M3gGeomContext)->geometryPolyDraw(inNumPoints, inPoints, inShade)
		
	#define M3rGeometry_LineDraw(inNumPoints, inPoints, inShade) \
		(M3gGeomContext)->geometryLineDraw(inNumPoints, inPoints, inShade)
		
	#define M3rGeometry_PointDraw(inNumPoints, inPoints, inShade) \
		(M3gGeomContext)->geometryPointDraw(inNumPoints, inPoints, inShade)


/*
 * The geom context data structure
 */
	typedef struct M3tGeomContextMethods
	{
		M3tGeomContextMethod_Frame_Start					frameStart;
		M3tGeomContextMethod_Frame_End						frameEnd;
		
		M3tGeomContextMethod_Pick_ScreenToWorld				pickScreenToWorld;

		M3tGeomContextMethod_LightList_Ambient				lightListAmbient;
		M3tGeomContextMethod_LightList_Directional			lightListDirectional;
		M3tGeomContextMethod_LightList_Point				lightListPoint;		
		M3tGeomContextMethod_LightList_Cone					lightListCone;
		
		M3tGeomContextMethod_Geometry_BoundingBox_Draw		geometryBoundingBoxDraw;
		M3tGeomContextMethod_Geometry_PolyDraw				geometryPolyDraw;
		M3tGeomContextMethod_Geometry_LineDraw				geometryLineDraw;
		M3tGeomContextMethod_Geometry_PointDraw				geometryPointDraw;
		
		M3tGeomContextMethod_Env_SetCamera					envSetCamera;
		M3tGeomContextMethod_Env_DrawGQList					envDrawGQList;
	
		M3tGeomContextMethod_Sprite_Draw					spriteDraw;
		M3tGeomContextMethod_Contrail_Draw					contrailDraw;
		M3tGeomContextMethod_Geometry_Draw					geometryDraw;
		M3tGeomContextMethod_Decal_Draw						decalDraw;

		M3tGeomContextMethod_SpriteArray_Draw				spriteArrayDraw;

		M3tGeomContextMethod_Skybox_Draw					skyboxDraw;
		M3tGeomContextMethod_Skybox_Create					skyboxCreate;
		M3tGeomContextMethod_Skybox_Destroy					skyboxDestroy;

		M3tGeomContextMethod_PointVisible					pointVisible;
		M3tGeomContextMethod_PointVisibleScale				pointVisibleScale;
		

	} M3tGeomContextMethods;

/*
 * globals
 */

const extern UUtUns8 M3gBBox_EdgeList[12][2];
const extern M3tQuad M3gBBox_QuadList[6];
extern UUtBool M3gResolutionSwitch;		// control resolution switching of draw contexts



extern M3tGeomContextMethods*	M3gGeomContext;

/*
 * Motoko prototypes
 */
UUtError
M3rInitialize(
	void);

UUtError
M3rRegisterTemplates(
	void);

void
M3rTerminate(
	void);


/*
 * Geometry engine state
 */
	void
	M3rGeom_State_Set(
			M3tGeomStateIntType		inGeomStateIntType,
			UUtInt32				inState);

	UUtInt32
	M3rGeom_State_Get(
			M3tGeomStateIntType		inGeomStateIntType);

	UUtError
	M3rGeom_State_Push(
			void);

	UUtError
	M3rGeom_State_Pop(
			void);

	UUtError
	M3rGeom_State_Commit(
			void);


/*
 * Debugging Routines
 */

#if defined(DEBUGGING) && DEBUGGING

	UUtError M3rVerify_Geometry(const M3tGeometry *inGeometry);
	UUtError M3rVerify_Point3D(const M3tPoint3D *inPoint);
	UUtError M3rVerify_PointScreen(const M3tPointScreen *inPoint);

#else

#define M3rVerify_Geometry(x)
#define M3rVerify_Point3D(x)
#define M3rVerify_PointScreen(x)

#endif

/*
 * M3rGeom_Line_Light
 */

void M3rGeom_Line_Light(
		const M3tPoint3D *point1, 
		const M3tPoint3D *point2, 
		UUtUns32 shade);

void M3rGeom_FaceNormal(
		const M3tPoint3D *inPoints,
		const M3tQuad *inQuads,
		const M3tVector3D *inFaceNormals,
		UUtUns32 inShade);

/* 
 * BBox routines
 */ 

void M3rGeometry_GetBBox(
		const M3tGeometry *inGeometry,
		const M3tMatrix4x3 *inMatrix,
		M3tBoundingBox_MinMax *outBBox);

const M3tQuad *M3rBBox_GetSide(
		M3tBBoxSide inSide);

void M3rMinMaxBBox_To_BBox(
		const M3tBoundingBox_MinMax *inBoundingBox, 
		M3tBoundingBox *outBoundingBox);

void M3rMinMaxBBox_Draw_Line(
		M3tBoundingBox_MinMax *inBBox,
		UUtUns32 inShade);

void M3rBVolume_Draw_Line(
	M3tBoundingVolume *inBVolume,
	UUtUns32 inShade);

void M3rBBox_Draw_Line(
	M3tBoundingBox *inBBox,
	UUtUns32 inShade);
	
void M3rBBox_To_EdgeBBox(
		M3tBoundingBox *inBoundingBox,
		M3tBoundingBox_Edge *outBoundingBox);

void M3rEdgeBBox_Offset(
	M3tBoundingBox_Edge *ioBoundingBox,
	M3tPoint3D *inOffset);

void M3rBBox_To_MinMaxBBox(
	M3tBoundingBox *inBox,
	M3tBoundingBox_MinMax *outBox);

void M3rBVolume_To_WorldBBox(
	M3tBoundingVolume *inVolume,
	M3tBoundingBox *outBox);

void M3rBVolume_To_BSphere(
	M3tBoundingVolume *inVolume,
	M3tBoundingSphere *outSphere);

void M3rBBox_To_LocalBVolume(
	M3tBoundingBox *inBox,
	M3tBoundingVolume *outVolume);

void M3rBBox_GrowFromPlane(
	M3tBoundingBox *ioBox,
	M3tPlaneEquation *inPlane,
	float inGrowAmount);

/*
 * Cylinder & Circle routines
 */

// draws a wireframe circle in the XZ plane
void M3rDisplay_Circle(M3tPoint3D *inCenter, float inRadius, UUtUns32 inShade);

void M3rBuildCircle(
	UUtUns32 inNumPoints,
	float inHeight,
	const M3tBoundingCircle *circle, 
	M3tPoint3D *outPoints);
	
void M3rBoundingCylinder_Draw_Line(
	const M3tBoundingCylinder *inBoundingCylinder,
	UUtUns32 inShade);
			
void
M3rDraw_Bitmap(
	M3tTextureMap			*inBitmap,
	const M3tPointScreen	*inDestPoint,
	UUtUns16				inWidth,
	UUtUns16				inHeight,
	UUtUns32				inShade,
	UUtUns16				inAlpha);			

void 
M3rDraw_BitmapUV(
	M3tTextureMap			*inBitmap,
	const M3tTextureCoord	*inUV,
	const M3tPointScreen	*inDestPoint,
	UUtUns16				inWidth,
	UUtUns16				inHeight,
	UUtUns32				inShade,
	UUtUns16				inAlpha);

void 
M3rDraw_BigBitmap(
	M3tTextureMap_Big		*inBigBitmap,
	const M3tPointScreen	*inDestPoint,
	UUtUns16				inWidth,
	UUtUns16				inHeight,
	UUtUns32				inShade,
	UUtUns16				inAlpha);			

void M3rDraw_Commit_Alpha(
	void);

/*
 * Motoko Texture Map Functions
 */

typedef enum
{
	M3cTexture_AllocMemory		= (1 << 0),
	M3cTexture_UseTempMem		= (1 << 1)
	
} M3tTextureAllocMemory;

UUtError
M3rTextureMap_New(
	UUtUns16				inWidth,
	UUtUns16				inHeight,
	IMtPixelType			inTexelType,
	UUtUns32				inAllocMemory,
	UUtUns32				inFlags,
	const char				*inDebugName,
	M3tTextureMap*			*outTextureMap);

void
M3rTextureMap_Prepare_Internal(
	M3tTextureMap			*ioTextureMap);

void
M3rTextureMap_TemporarilyLoad_Internal(
	M3tTextureMap			*ioTextureMap,
	UUtUns32				inSkipLargeLODs);

void
M3rTextureMap_UnloadTemporary(
	M3tTextureMap			*ioTextureMap);

M3tTextureMap *M3rTextureMap_GetFromName(const char *inTemplateName);
M3tTextureMap *M3rTextureMap_GetFromName_UpperCase(const char *inTemplateName);
M3tTextureMap *M3rTextureMap_GetFromName_LowerCase(const char *inTextureName);

M3tTextureMap *M3rTextureMap_GetPlaceholder(const char *inTextureName);
M3tTextureMap *M3rTextureMap_GetPlaceholder_UpperCase(const char *inTextureName);
M3tTextureMap *M3rTextureMap_GetPlaceholder_LowerCase(const char *inTextureName);

M3tTextureMap *M3rTextureMap_GetPlaceholder_StripExtension(const char *inTextureName);
M3tTextureMap *M3rTextureMap_GetPlaceholder_StripExtension_UpperCase(const char *inTextureName);
M3tTextureMap *M3rTextureMap_GetPlaceholder_StripExtension_LowerCase(const char *inTextureName);


static UUcInline void M3rTextureMap_Prepare(
	M3tTextureMap			*ioTextureMap)
{
	if (0 == (ioTextureMap->flags & M3cTextureFlags_Prepared)) {
		M3rTextureMap_Prepare_Internal(ioTextureMap);
	}

	return;
}

static UUcInline void M3rTextureMap_TemporarilyLoad(
	M3tTextureMap			*ioTextureMap,
	UUtUns32				inSkipLargeLODs)
{
	if (ioTextureMap->pixels == NULL) {
		M3rTextureMap_TemporarilyLoad_Internal(ioTextureMap, inSkipLargeLODs);
	}
}

UUtError
M3rTextureMap_Big_New(
	UUtUns16				inWidth,
	UUtUns16				inHeight,
	IMtPixelType			inTexelType,
	UUtUns32				inAllocMemory,
	UUtUns32				inFlags,
	const char				*inDebugName,
	M3tTextureMap_Big		**outTextureMap);

void M3rTextureMap_Big_Unload(M3tTextureMap_Big *inTextureMap);  // GME

#define M3cFillEntireMap (NULL)
	
UUtError
M3rTextureMap_BuildMipMap(
	IMtScaleMode	inScaleMode,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	IMtPixelType	inPixelType,
	void*			inBaseMap);

void
M3rTextureMap_Copy(
	void				*inSrcTexture,
	UUtRect 			*inSrcRect,
	void				*inDstTexture,
	UUtRect 			*inDstRect);

void
M3rTextureMap_GetTextureSize(
	UUtUns16				inWidth,
	UUtUns16 				inHeight,
	UUtUns16 				*outWidth,
	UUtUns16 				*outHeight);

void
M3rTextureMap_Fill(
	M3tTextureMap*		inTextureMap,
	IMtPixel			inColor,
	const UUtRect*		inBounds);

void
M3rTextureMap_Fill_Shade(
	M3tTextureMap*		inTextureMap,
	IMtShade			inShade,
	const UUtRect*		inBounds);

void
M3rTextureMap_Big_Fill(
	M3tTextureMap_Big*	inTextureMap,
	IMtPixel			inColor,
	const UUtRect*		inBounds);

MAtMaterialType
M3rTextureMap_GetMaterialType(
	const M3tTextureMap	*inTextureMap);
	
void
M3rTextureMap_SetMaterialType(
	M3tTextureMap		*ioTextureMap,
	MAtMaterialType		inMaterialType);

void
M3rTextureRef_GetSize(
	void				*inTextureRef,
	UUtUns16			*outWidth,
	UUtUns16			*outHeight);

/*
 * 
 */
	void
	M3rTextureCoord_FindMinMax(
		UUtUns32			inNumCoords,
		M3tTextureCoord*	inTextureCoords,
		float				*outMinU,
		float				*outMinV,
		float				*outMaxU,
		float				*outMaxV);
			
UUtError
M3rGroup_GetColor(
	GRtGroup*		inGroup,
	const char*		inVarName,
	M3tColorRGB		*outColor);

M3tTextureMap *M3rGroup_GetTextureMap(GRtGroup *inGroup, const char *inVarName);
void *M3rOptimizeNode(void *inNode, BFtFile *inDebugFile);
void M3rSetGamma(float inGamma);

#if defined(DEBUGGING) && DEBUGGING
void MSrMatrixVerify(const M3tMatrix4x3 *m);
void MSrMatrix3x3Verify(const M3tMatrix3x3 *m);
void MSrStackVerify_Debug(void);

#define MSmMatrixVerify(m) MSrMatrixVerify(m);
#define MSmMatrix3x3Verify(m) MSrMatrix3x3Verify(m);
#define	MSmStackVerify()	MSrStackVerify_Debug()
#else
#define MSmMatrixVerify(m) UUmBlankFunction 
#define MSmMatrix3x3Verify(m) UUmBlankFunction
#define	MSmStackVerify() UUmBlankFunction 
#endif


// S.S. I put this into Motoko_Manager.c
UUtBool M3rSinglePassMultitexturingAvailable(void);


#ifdef __cplusplus
}
#endif

#endif
