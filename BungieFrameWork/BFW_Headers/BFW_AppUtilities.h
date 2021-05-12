#pragma once
/*
	FILE:	BFW_AppUtilities.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Feb 27, 1998
	
	PURPOSE: Interface to the Motoko 3D engine
	
	Copyright 1998

*/
#ifndef BFW_APPUTILITIES_H
#define BFW_APPUTILITIES_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "BFW.h"
#include "BFW_Group.h"
#include "BFW_Motoko.h"
#include "BFW_MathLib.h"
#include "BFW_TemplateManager.h"
#include "BFW_BitVector.h"
/*
 * Basic shared element functions
 */
	typedef struct AUtSharedElemArray	AUtSharedElemArray;
	typedef UUtInt16
	(*AUtSharedElemArray_Equal)(
		void*	inElemA,
		void*	inElemB);
		
	AUtSharedElemArray*
	AUrSharedElemArray_New(
		UUtUns32					inElemSize,
		AUtSharedElemArray_Equal	inCompareFunc);

	void
	AUrSharedElemArray_Delete(
		AUtSharedElemArray*	inSharedElemArray);

	void
	AUrSharedElemArray_Reset(
		AUtSharedElemArray*	inSharedElemArray);

	UUtBool
	AUrSharedElemArray_FindElem(
		AUtSharedElemArray*	inSharedElemArray,
		void*				inElem,
		UUtUns32			*outElemIndex);

	UUtError
	AUrSharedElemArray_AddElem(
		AUtSharedElemArray*	inSharedElemArray,
		void*				inElem,
		UUtUns32			*outElemIndex);

	UUtError
	AUrSharedElemArray_Resort(
		AUtSharedElemArray*	inSharedElemArray);

	UUtError
	AUrSharedElemArray_Reorder(
		AUtSharedElemArray*	inSharedElemArray,
		UUtUns32*			inRemapArray);

	UUtUns32
	AUrSharedElemArray_GetNum(
		AUtSharedElemArray*	inSharedElemArray);

	void*
	AUrSharedElemArray_GetList(
		AUtSharedElemArray*	inSharedElemArray);
	
	UUtUns32*
	AUrSharedElemArray_GetSortedIndexList(
		AUtSharedElemArray*	inSharedElemArray);
		
/*
 * Shared point functions
 */
	typedef struct AUtSharedPointArray	AUtSharedPointArray;
	
	AUtSharedPointArray*
	AUrSharedPointArray_New(
		void);

	void
	AUrSharedPointArray_Delete(
		AUtSharedPointArray*	inSharedPointArray);

	void
	AUrSharedPointArray_Reset(
		AUtSharedPointArray*	inSharedPointArray);

	UUtError
	AUrSharedPointArray_AddPoint(
		AUtSharedPointArray*	inSharedPointArray,
		float					inX,
		float					inY,
		float					inZ,
		UUtUns32				*outPointIndex);

	void AUrSharedPointArray_RoundPoint(M3tPoint3D *ioPoint);

	UUtError
	AUrSharedPointArray_Reorder(
		AUtSharedPointArray*	inSharedPointArray,
		UUtUns32*				inRemapArray);

	M3tPoint3D*
	AUrSharedPointArray_GetList(
		AUtSharedPointArray*	inSharedPointArray);

	UUtUns32
	AUrSharedPointArray_GetNum(
		AUtSharedPointArray*	inSharedPointArray);
	
	void
	AUrSharedPointArray_Dump(
		AUtSharedPointArray*	inSharedPointArray,
		FILE*					inFile);
	
	UUtError
	AUrSharedPointArray_CreateTemplate(
		AUtSharedPointArray*	inSharedPointArray,
		char*					inName,
		M3tPoint3DArray*		*outReference);
			
	UUtError
	AUrSharedPointArray_InterpolatePoint(
		AUtSharedPointArray*	inPointArray,
		float					inA, 
		float					inB, 
		float					inC, 
		float					inD, 
		M3tPoint3D*				inInVertex,
		M3tPoint3D*				inOutVertex,
		UUtUns32				*outNewVertexIndex);
/*
 * Shared PlaneEqu functions
 *
 * NOTES:
 *	if d is negative the negative plane equ is stored and the high bit of the index field is set
 *  This means that d is always positive in the actual list of plane equations
 */
	typedef struct AUtSharedPlaneEquArray	AUtSharedPlaneEquArray;
	
	AUtSharedPlaneEquArray*
	AUrSharedPlaneEquArray_New(
		void);

	void
	AUrSharedPlaneEquArray_Delete(
		AUtSharedPlaneEquArray*	inSharedPlaneEquArray);

	void
	AUrSharedPlaneEquArray_Reset(
		AUtSharedPlaneEquArray*	inSharedPlaneEquArray);

	UUtError
	AUrSharedPlaneEquArray_AddPlaneEqu(
		AUtSharedPlaneEquArray*	inSharedPlaneEquArray,
		float					inA,
		float					inB,
		float					inC,
		float					inD,
		UUtUns32				*outPlaneEquIndex);

	M3tPlaneEquation*
	AUrSharedPlaneEquArray_GetList(
		AUtSharedPlaneEquArray*	inSharedPlaneEquArray);

	UUtUns32
	AUrSharedPlaneEquArray_GetNum(
		AUtSharedPlaneEquArray*	inSharedPlaneEquArray);
	
	void
	AUrSharedPlaneEquArray_Dump(
		AUtSharedPlaneEquArray*	inSharedPlaneEquArray,
		FILE*					inFile);

	UUtError
	AUrSharedPlaneEquArray_CreateTemplate(
		AUtSharedPlaneEquArray*	inSharedPlaneEquArray,
		char*					inName,
		M3tPlaneEquationArray*	*outReference);
	
	UUtBool
	AUrSharedPlaneEquArray_FindPlaneEqu(
		AUtSharedPlaneEquArray*	inSharedPlaneEquArray,
		float					inA,
		float					inB,
		float					inC,
		float					inD,
		UUtUns32				*outPlaneEquIndex);
		
/*
 * Shared Quad functions
 */
	typedef struct AUtSharedQuadArray	AUtSharedQuadArray;
	
	AUtSharedQuadArray*
	AUrSharedQuadArray_New(
		void);

	void
	AUrSharedQuadArray_Delete(
		AUtSharedQuadArray*	inSharedQuadArray);

	void
	AUrSharedQuadArray_Reset(
		AUtSharedQuadArray*	inSharedQuadArray);

	UUtError
	AUrSharedQuadArray_AddQuad(
		AUtSharedQuadArray*	inSharedQuadArray,
		UUtUns32			inIndex0,
		UUtUns32			inIndex1,
		UUtUns32			inIndex2,
		UUtUns32			inIndex3,
		UUtUns32			*outQuadIndex);

	UUtError
	AUrSharedQuadArray_Resort(
		AUtSharedQuadArray*		inSharedQuadArray);

	M3tQuad*
	AUrSharedQuadArray_GetList(
		AUtSharedQuadArray*		inSharedQuadArray);

	UUtUns32
	AUrSharedQuadArray_GetNum(
		AUtSharedQuadArray*		inSharedQuadArray);
		
	void
	AUrSharedQuadArray_Dump(
		AUtSharedQuadArray*		inSharedQuadArray,
		FILE*					inFile);
	
	UUtError
	AUrSharedQuadArray_CreateTemplate(
		AUtSharedQuadArray*		inSharedQuadArray,
		char*					inName,
		M3tQuadArray*			*outReference);
		
/*
 * Shared Vector functions
 */
	typedef struct AUtSharedVectorArray	AUtSharedVectorArray;
	
	AUtSharedVectorArray*
	AUrSharedVectorArray_New(
		void);

	void
	AUrSharedVectorArray_Delete(
		AUtSharedVectorArray*	inSharedVectorArray);

	void
	AUrSharedVectorArray_Reset(
		AUtSharedVectorArray*	inSharedVectorArray);

	UUtError
	AUrSharedVectorArray_AddVector(
		AUtSharedVectorArray*	inSharedVectorArray,
		float					inX,
		float					inY,
		float					inZ,
		UUtUns32				*outVectorIndex);

	M3tVector3D*
	AUrSharedVectorArray_GetList(
		AUtSharedVectorArray*	inSharedVectorArray);

	UUtUns32
	AUrSharedVectorArray_GetNum(
		AUtSharedVectorArray*	inSharedVectorArray);

	void
	AUrSharedVectorArray_Dump(
		AUtSharedVectorArray*	inSharedVectorArray,
		FILE*					inFile);
	
	UUtError
	AUrSharedVectorArray_CreateTemplate(
		AUtSharedVectorArray*	inSharedVectorArray,
		char*					inName,
		M3tVector3DArray*		*outReference);

/*
 * Shared TexCoord functions
 */
	typedef struct AUtSharedTexCoordArray	AUtSharedTexCoordArray;
	
	AUtSharedTexCoordArray*
	AUrSharedTexCoordArray_New(
		void);

	void
	AUrSharedTexCoordArray_Delete(
		AUtSharedTexCoordArray*	inSharedTexCoordArray);

	void
	AUrSharedTexCoordArray_Reset(
		AUtSharedTexCoordArray*	inSharedTexCoordArray);

	UUtError
	AUrSharedTexCoordArray_AddTexCoord(
		AUtSharedTexCoordArray*	inSharedTexCoordArray,
		float					inU,
		float					inV,
		UUtUns32				*outTexCoordIndex);

	M3tTextureCoord*
	AUrSharedTexCoordArray_GetList(
		AUtSharedTexCoordArray*	inSharedTexCoordArray);

	UUtUns32
	AUrSharedTexCoordArray_GetNum(
		AUtSharedTexCoordArray*	inSharedTexCoordArray);
	
	UUtError
	AUrSharedTexCoordArray_CreateTemplate(
		AUtSharedTexCoordArray*	inSharedTexCoordArray,
		char*					inName,
		M3tTextureCoordArray*	*outReference);

/*
 * Shared edge functions
 */
	#define AUcMaxQuadsPerEdge	12
	
	typedef struct AUtSharedEdgeArray	AUtSharedEdgeArray;
	typedef struct AUtEdge
	{
		UUtUns32	vIndex0;
		UUtUns32	vIndex1;
		
		UUtUns32	quadIndices[AUcMaxQuadsPerEdge];
		
	} AUtEdge;

	AUtSharedEdgeArray*
	AUrSharedEdgeArray_New(
		void);

	void
	AUrSharedEdgeArray_Delete(
		AUtSharedEdgeArray*	inSharedEdgeArray);

	void
	AUrSharedEdgeArray_Reset(
		AUtSharedEdgeArray*	inSharedEdgeArray);

	UUtError
	AUrSharedEdgeArray_AddEdge(
		AUtSharedEdgeArray*	inSharedEdgeArray,
		UUtUns32			inEdge0,
		UUtUns32			inEdge1,
		UUtUns32			inQuadIndex,
		UUtUns32			*outEdgeIndex);
	
	UUtError
	AUrSharedEdgeArray_AddPoly(
		AUtSharedEdgeArray*	inSharedEdgeArray,
		UUtUns32			inNumVertices,
		UUtUns32*			inVertices,
		UUtUns32			inQuadIndex,
		UUtUns32			*outEdgeIndices);
		
	AUtEdge*
	AUrSharedEdgeArray_GetList(
		AUtSharedEdgeArray*	inSharedEdgeArray);

	UUtUns32
	AUrSharedEdgeArray_GetNum(
		AUtSharedEdgeArray*	inSharedEdgeArray);
	
	UUtUns32
	AUrSharedEdge_FindCommonVertex(
		AUtEdge*	inEdgeA,
		AUtEdge*	inEdgeB);
		
	UUtUns32
	AUrSharedEdge_FindOtherVertex(
		AUtEdge*	inEdge,
		UUtUns32	inVertexIndex);

	UUtUns32
	AUrSharedEdge_GetOtherFaceIndex(
		AUtEdge*	inEdge,
		UUtUns32	inFaceIndex);

	AUtEdge*
	AUrSharedEdge_Triangle_FindEdge(
		AUtSharedEdgeArray*	inSharedEdgeArray,
		M3tTri*		inTriEdges,
		UUtUns32			inIndex0,
		UUtUns32			inIndex1);

/*
 * Shared String functions
 */
	#define AUcMaxStringLength	128
	
	typedef struct AUtSharedStringArray	AUtSharedStringArray;
	
	typedef struct AUtSharedString
	{
		char		string[AUcMaxStringLength];
		UUtUns32	data;
		
	} AUtSharedString;
	
	AUtSharedStringArray*
	AUrSharedStringArray_New(
		void);

	void
	AUrSharedStringArray_Delete(
		AUtSharedStringArray*	inSharedStringArray);

	void
	AUrSharedStringArray_Reset(
		AUtSharedStringArray*	inSharedStringArray);

	UUtError
	AUrSharedStringArray_AddString(
		AUtSharedStringArray*	inSharedStringArray,
		const char*				inString,
		UUtUns32				inData,
		UUtUns32				*outStringIndex);

	AUtSharedString*
	AUrSharedStringArray_GetList(
		AUtSharedStringArray*	inSharedStringArray);

	UUtUns32
	AUrSharedStringArray_GetNum(
		AUtSharedStringArray*	inSharedStringArray);

	UUtBool
	AUrSharedStringArray_GetIndex(
		AUtSharedStringArray*	inSharedStringArray,
		const char*				inString,
		UUtUns32				*outIndex);

	UUtBool
	AUrSharedStringArray_GetData(
		AUtSharedStringArray*	inSharedStringArray,
		const char*				inString,
		UUtUns32				*outData);

	UUtUns32*
	AUrSharedStringArray_GetSortedIndexList(
		AUtSharedStringArray*	inSharedStringArray);

/*
 * Hash table utilities
 */
	typedef struct AUtHashTable	AUtHashTable;
	
	typedef UUtUns32
	(*AUtHashTable_Hash)(
		void*	inElement);
	
	typedef UUtBool
	(*AUtHashTable_IsEqual)(
		void*	inElement1,
		void*	inElement2);

	UUtUns32 AUrString_GetHashValue(const char *inString);
	UUtUns32 AUrHash_String_Hash(void *inElement);
	UUtBool AUrHash_String_IsEqual(void *inElement1, void *inElement2);

	UUtUns32 AUrHash_4Byte_Hash(void *inElement);
	UUtBool AUrHash_4Byte_IsEqual(void *inElement1, void *inElement2);


	AUtHashTable*
	AUrHashTable_New(
		UUtUns32						inElemSize,
		UUtUns32						inNumBuckets,
		AUtHashTable_Hash				inHashFunction,
		AUtHashTable_IsEqual			inCompareFunction);
	
	void
	AUrHashTable_Delete(
		AUtHashTable*	inHashTable);
	
	void
	AUrHashTable_Reset(
		AUtHashTable*	inHashTable);
		
	void *
	AUrHashTable_Add(
		AUtHashTable*	inHashTable,
		void			*inNewElement);
		
	void*
	AUrHashTable_Find(
		AUtHashTable*	inHashTable,
		void			*inElement);
	
/*
 * Some quad utilities
 */
	void
	AUrQuad_FindMinMax(
		const M3tPoint3D*			inPointList,
		const M3tQuad*			inQuad,
		float						*outMinX,
		float						*outMaxX,
		float						*outMinY,
		float						*outMaxY,
		float						*outMinZ,
		float						*outMaxZ);
	
	// This assumes they already lie on the same plane
	UUtBool
	AUrQuad_QuadOverlapsQuad(
		const M3tPoint3D*			inPointList,
		const M3tQuad*			inQuad,
		const M3tQuad*			inQuadOverlapping);
		
	UUtBool
	AUrQuad_QuadWithinQuad(
		const M3tPoint3D*			inPointList,
		const M3tQuad*			inQuad,
		const M3tQuad*			inQuadWithin);
	
	void
	AUrQuad_ComputeCenter(
		const M3tPoint3D*	inPointList,
		const M3tQuad*	inQuad,
		float				*outCenterX,
		float				*outCenterY,
		float				*outCenterZ);
	
	UUtBool
	AUrQuad_SharesAnEdge(
		const M3tQuad*	inQuadA,
		const M3tQuad*	inQuadB);
		
	void AUrQuad_LowestPoints(
		M3tQuad *inQuad,
		M3tPoint3D *inPointArray,
		M3tPoint3D **outPointLowest,
		M3tPoint3D **outPointNextLowest);
		
	void AUrQuad_HighestPoints(
		M3tQuad *inQuad,
		M3tPoint3D *inPointArray,
		M3tPoint3D **outPointHighest,
		M3tPoint3D **outPointNextHighest);
		
/*
 * some text parsing funcs
 */
	typedef UUtError
	(*AUtParseGeometry_AddPointFunc)(
		void*		inRefCon,
		UUtUns32	i,
		float		x,
		float		y,
		float		z,
		float		nx,
		float		ny,
		float		nz,
		float		u,
		float		v);

	typedef UUtError
	(*AUtParseGeometry_AddTriangleFunc)(
		void*		inRefCon,
		UUtUns32	i0,
		UUtUns32	i1,
		UUtUns32	i2,
		float		nx,
		float		ny,
		float		nz);
		
	typedef UUtError
	(*AUtParseGeometry_AddQuadFunc)(
		void*		inRefCon,
		UUtUns32	i0,
		UUtUns32	i1,
		UUtUns32	i2,
		UUtUns32	i3,
		float		nx,
		float		ny,
		float		nz);
	
	typedef UUtError
	(*AUtParseGeometry_AddEnvTriangleFunc)(
		void*		inRefCon,
		char*		inTextureName,
		UUtUns32	i0,
		UUtUns32	i1,
		UUtUns32	i2,
		float		nx,
		float		ny,
		float		nz);
		
	typedef UUtError
	(*AUtParseGeometry_AddEnvQuadFunc)(
		void*		inRefCon,
		char*		inTextureName,
		UUtUns32	i0,
		UUtUns32	i1,
		UUtUns32	i2,
		UUtUns32	i3,
		float		nx,
		float		ny,
		float		nz);
	
	UUtError
	AUrParse_CoreGeometry(
		BFtTextFile*						inTextFile,
		AUtParseGeometry_AddPointFunc		inAddPointFunc,
		AUtParseGeometry_AddTriangleFunc	inAddTriFunc,
		AUtParseGeometry_AddQuadFunc		inAddQuadFunc,
		void*								inRefCon);

	UUtError
	AUrParse_EnvGeometry(
		BFtTextFile*						inTextFile,
		M3tMatrix4x3*							inMatrix,
		AUtParseGeometry_AddPointFunc		inAddPointFunc,
		AUtParseGeometry_AddEnvTriangleFunc	inAddTriFunc,
		AUtParseGeometry_AddEnvQuadFunc		inAddQuadFunc,
		void*								inRefCon);

	void AUrPlane_ClosestPoint(
		const M3tPlaneEquation*		inPlaneEqu,
		const M3tPoint3D*			inPoint,
		M3tPoint3D*					outPoint);

	float AUrPlane_Distance(
		const M3tPlaneEquation*		inPlaneEqu,
		const M3tPoint3D*			inPoint,
		M3tPoint3D*					outPoint);

	float AUrPlane_Distance_Squared(
		const M3tPlaneEquation*		inPlaneEqu,
		const M3tPoint3D*			inPoint,
		M3tPoint3D*					outPoint);

/*
 * 
 */
	typedef struct AUtFlagElement
	{
		char*		textName;
		UUtUns32	flagValue;
		
	} AUtFlagElement;
	
	const char*
	AUrFlags_GetTextName(
		AUtFlagElement		*inFlagList,
		UUtUns32			inFlagValue);
	
	UUtError
	AUrFlags_ParseFromGroupArray(
		AUtFlagElement*		inFlagList,		// Terminated by a NULL textName
		GRtElementType		inElementType,
		void*				inElement,
		UUtUns32			*outResult);
	
	UUtError
	AUrFlags_ParseFromString(
		char*				inString,		// multiple entries are delimited by "|"
		UUtBool				inIsBitField,
		AUtFlagElement*		inFlagList,		// Terminated by a NULL textName
		UUtUns32			*outResult);
	
	void
	AUrFlags_PrintFromValue(
		FILE*			inFile,
		UUtBool			inIsBitField,
		AUtFlagElement*	inFlagList,
		UUtUns32		inValue);
/*
 * 
 */
	UUtWindow
	AUrWindow_New(
		UUtRect*	inRect);
	
	void
	AUrWindow_CopyImageInto(
		UUtWindow		inDstWindow,
		UUtUns16		inSrcWidth,
		UUtUns16		inSrcHeight,
		IMtPixelType	inSrcPixelType,
		void*			inSrcData);
		
	void
	AUrWindow_EORImage(
		UUtWindow	inWindow,
		UUtUns16	inMinX,
		UUtUns16	inMaxX,
		UUtUns16	inMinY,
		UUtUns16	inMaxY);

	void
	AUrWindow_Clear(
		UUtWindow	inWindow);

	void
	AUrWindow_Delete(
		UUtWindow	inWindow);

	void AUrBuildArgumentList(
		char *inString, 
		UUtUns32 inMaxCount, 
		UUtUns32 *outCount, 
		char **outArguments);
	
	// dictionary funtions
	typedef struct AUtDict
	{
		UUtUns32 *vector;
		UUtUns32 maxPages;
		UUtUns32 numPages;
		UUtUns32 pages[1];
	} AUtDict;

#if UUmCompiler	== UUmCompiler_VisC
	AUtDict *AUrDict_New(UUtUns32 inMaxEntries, UUtUns32 inRange);
	void AUrDict_Clear(AUtDict *inDict);
	void AUrDict_Dispose(AUtDict *inDict);
	UUtBool __fastcall AUrDict_TestAndAdd(AUtDict *inDict, UUtUns32 key);
#else
	AUtDict *AUrDict_New(UUtUns32 inMaxEntries, UUtUns32 inRange);
	void AUrDict_Clear(AUtDict *inDict);
	void AUrDict_Dispose(AUtDict *inDict);
	UUtBool AUrDict_TestAndAdd(AUtDict *inDict, UUtUns32 key);
#endif

	static UUcInline UUtBool AUrDict_Test(AUtDict *inDict, UUtUns32 key)
	{
		UUmAssertWritePtr(inDict, sizeof(*inDict));
		return UUrBitVector_TestBit(inDict->vector, key);
	}

/*
 * There functions reverse the order of the bits.
 * i.e. the bit at 1 << n becomes 1 << (_number_of_bits - n - 1)
 */

	UUtUns16 AUrBits16_Reverse(UUtUns16 inBits);
	UUtUns32 AUrBits32_Reverse(UUtUns32 inBits);

	UUtUns8 AUrBits16_Count(UUtUns16 inBits);
	UUtUns8 AUrBits32_Count(UUtUns32 inBits);
	UUtUns8 AUrBits64_Count(UUtUns64 inBits);

	// takes a a bunch of strings and turns them into a bitfield
	/*
	 * AUrParamListToBitfield
	 *
	 * Takes a string and a mapping array.  It creates a bitfield based
	 * on the two.  Where element n in the string array is the mask 1 << n.
	 *
	 * Present a dialog and returns an error if there is an invalid string
	 * in the array
	 *
	 */
	UUtError AUrParamListToBitfield(const char *inFlagStr, const TMtStringArray *inMapping, UUtUns32 *outBitfield);

	/*
	 * AUrStringToSingleBit
	 *
	 * Takes a string, mapping and ptr to a uns32.
	 *
	 * If it finds inString in inMapping sets the result to 1 << n where n is 
	 * the index of the string in inMapping.
	 *
	 * Otherwise returns an error
	 *
	 */

	UUtError AUrStringToSingleBit(const char *inString, const TMtStringArray *inMapping, UUtUns32 *result);


	/*
	 *
	 * AUrLookupString
	 *
	 * Takes a string and a mapping array.  It looks the string up and returns an index or AUcNoSuchString on failure.
	 *
	 */

	enum {
		AUcNoSuchString = -1
	};

	UUtInt16 AUrLookupString(const char *inString, const TMtStringArray *inMapping);

	void AUrNumToCommaString(UUtInt32 inNumber, char *outString);

	/*
	 *
	 * sorting
	 *
	 */

	// sort an array of 16 bit numbers
	typedef UUtBool
	(*AUtCompare_Function_16)(
		UUtUns16 inA,
		UUtUns16 inB);

	void AUrQSort_16(
	    void *inBase,
		unsigned inNumElements,
		AUtCompare_Function_16 inCompare);

	// sort an array of 32 bit numbers
	typedef UUtBool
	(*AUtCompare_Function_32)(
		UUtUns32 inA,
		UUtUns32 inB);

	void AUrQSort_32(
	    void *inBase,
		unsigned inNumElements,
		AUtCompare_Function_32 inCompare);


	// squezes duplicates out of a sorted array
	UUtUns16 AUrSqueeze_16(
		UUtUns16 *inBase,
		UUtUns32 inNumElements);




	UUtError AUrParseNumericalRangeString(
		char*		inArg,
		UUtUns32*	ioBitVector,
		UUtUns32	inBitVectorLen);


	void AUrDeviateXZ(
		M3tPoint3D *ioPoint,
		float inAmt);

#ifdef __cplusplus
}
#endif

#endif /* BFW_APPUTILITIES_H */
