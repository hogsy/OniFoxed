/*
	FILE:	BFW_AppUtilities.c

	AUTHOR:	Brent H. Pease, Michael Evans

	CREATED: March 2, 1998

	PURPOSE: application utilities

	Copyright 1998

*/

/*---------- headers */

#include <stdarg.h>
#include <ctype.h>

#include "bfw_math.h"
#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_Group.h"
#include "BFW_Motoko.h"
#include "BFW_AppUtilities.h"
#include "BFW_LocalInput.h"
#include "BFW_TM_Construction.h"
#include "BFW_MathLib.h"
#include "BFW_BitVector.h"
#include "BFW_Image.h"

/*---------- constants */

enum {
	// Mac alerts
	_button_abort= 1,
	_button_retry1= 1,
	_button_retry2= 2,
	_button_ignore= 3,
	_button_ok= 1,
	_button_cancel2= 2,
	_button_cancel3= 3,
	_button_yes= 1,
	_button_no= 2,
	MAC_ALERT_ABORT_RETRY_IGNORE_RES_ID= 129,
	MAC_ALERT_OK_RES_ID,
	MAC_ALERT_OK_CANCEL_RES_ID,
	MAC_ALERT_RETRY_CANCEL,
	MAC_ALERT_YES_NO,
	MAC_ALERT_YES_NO_CANCEL
};

#define AUcSharedPlane_NormalTolerance		1e-05f
#define AUcSharedPlane_OffsetTolerance		1e-02f

/*---------- structures */

struct AUtSharedElemArray
{
	UUtUns32						elemSize;
	AUtSharedElemArray_Equal		compareFunc;
	UUtMemory_ParallelArray*		parallelArray;

	UUtMemory_ParallelArray_Member*	elemArray;
	UUtMemory_ParallelArray_Member*	remapIndexArray;
};

/*---------- code */

/*
* Basic shared element functions
*/
AUtSharedElemArray*
AUrSharedElemArray_New(
	UUtUns32					inElemSize,
	AUtSharedElemArray_Equal	inCompareFunc)
{
	AUtSharedElemArray*	newSharedElemArray;

	newSharedElemArray = UUrMemory_Block_New(sizeof(AUtSharedElemArray));
	if(newSharedElemArray == NULL)
	{
		return NULL;
	}

	newSharedElemArray->elemSize = inElemSize;
	newSharedElemArray->compareFunc = inCompareFunc;

	newSharedElemArray->parallelArray = UUrMemory_ParallelArray_New(1000, 0, 1000);
	if(newSharedElemArray->parallelArray == NULL)
	{
		return NULL;
	}

	newSharedElemArray->elemArray =
		UUrMemory_ParallelArray_Member_New(
			newSharedElemArray->parallelArray,
			inElemSize);
	if(newSharedElemArray->elemArray == NULL)
	{
		return NULL;
	}

	newSharedElemArray->remapIndexArray =
		UUrMemory_ParallelArray_Member_New(
			newSharedElemArray->parallelArray,
			sizeof(UUtUns32));
	if(newSharedElemArray->remapIndexArray == NULL)
	{
		return NULL;
	}

	return newSharedElemArray;
}

void
AUrSharedElemArray_Delete(
	AUtSharedElemArray*	inSharedElemArray)
{
	UUrMemory_ParallelArray_Delete(inSharedElemArray->parallelArray);
	UUrMemory_Block_Delete(inSharedElemArray);
}

void
AUrSharedElemArray_Reset(
	AUtSharedElemArray*	inSharedElemArray)
{
	UUrMemory_ParallelArray_SetUsedElems(inSharedElemArray->parallelArray, 0, NULL);
}

UUtBool
AUrSharedElemArray_FindElem(
	AUtSharedElemArray*	inSharedElemArray,
	void*				inElem,
	UUtUns32			*outElemIndex)
{
	char*		elemMemory;
	char*		targetElem;
	UUtUns32	midIndex;
	UUtUns32	lowIndex, highIndex;
	UUtUns32	elemSize;
	UUtUns32*	remapArray;
	UUtUns32	remappedIndex;
	UUtInt16	compareResult;

	UUmAssertReadPtr(outElemIndex, sizeof(UUtUns32));

	elemMemory = (char*)UUrMemory_ParallelArray_Member_GetMemory(inSharedElemArray->parallelArray, inSharedElemArray->elemArray);
	remapArray = (UUtUns32*)UUrMemory_ParallelArray_Member_GetMemory(inSharedElemArray->parallelArray, inSharedElemArray->remapIndexArray);
	elemSize = inSharedElemArray->elemSize;

	lowIndex = 0;
	highIndex = (UUtUns32)UUrMemory_ParallelArray_GetNumUsedElems(inSharedElemArray->parallelArray);

	*outElemIndex = 0xFFFFFFFF;

	while(lowIndex < highIndex)
	{
		midIndex = (lowIndex + highIndex) >> 1;

		remappedIndex = remapArray[midIndex];

		targetElem = elemMemory + elemSize * remappedIndex;
		UUmAssertReadPtr(targetElem, elemSize);

		compareResult = inSharedElemArray->compareFunc(inElem, targetElem);

		if(compareResult == 0)
		{
			if(outElemIndex != NULL) *outElemIndex = remappedIndex;
			return UUcTrue;
		}
		else if(compareResult > 0)
		{
			lowIndex = midIndex + 1;
		}
		else
		{
			highIndex = midIndex;
		}
	}

	*outElemIndex = lowIndex;

	return UUcFalse;
}

UUtError
AUrSharedElemArray_AddElem(
	AUtSharedElemArray*	inSharedElemArray,
	void*				inElem,
	UUtUns32			*outElemIndex)
{
	UUtError	error;
	char*		elemMemory;
	UUtUns32	newIndex;
	char*		newElem;
	UUtUns32	elemSize;
	UUtUns32*	remapArray;
	UUtUns32	numElems;
	UUtUns32	lowIndex;

	elemMemory = (char*)UUrMemory_ParallelArray_Member_GetMemory(inSharedElemArray->parallelArray, inSharedElemArray->elemArray);
	remapArray = (UUtUns32*)UUrMemory_ParallelArray_Member_GetMemory(inSharedElemArray->parallelArray, inSharedElemArray->remapIndexArray);
	elemSize = inSharedElemArray->elemSize;

	if(AUrSharedElemArray_FindElem(
		inSharedElemArray,
		inElem,
		outElemIndex) == UUcTrue) return UUcError_None;

	numElems = (UUtUns32)UUrMemory_ParallelArray_GetNumUsedElems(inSharedElemArray->parallelArray);
	if((UUtUns32)(numElems + 1) < numElems)
	{
		UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Can add more than MaxUns32 number elements");
	}

	error =
		UUrMemory_ParallelArray_AddNewElem(
			inSharedElemArray->parallelArray,
			&newIndex,
			NULL);
	UUmError_ReturnOnErrorMsg(error, "could not allocate new elem");

	elemMemory = (char*)UUrMemory_ParallelArray_Member_GetMemory(inSharedElemArray->parallelArray, inSharedElemArray->elemArray);
	remapArray = (UUtUns32*)UUrMemory_ParallelArray_Member_GetMemory(inSharedElemArray->parallelArray, inSharedElemArray->remapIndexArray);

	lowIndex = *outElemIndex;

	if(lowIndex != newIndex)
	{
		UUrMemory_MoveOverlap(
			remapArray + lowIndex,
			remapArray + lowIndex + 1,
			(newIndex - lowIndex) * sizeof(UUtUns32));
	}

	remapArray[lowIndex] = (UUtUns32)newIndex;

	newElem = (char*)elemMemory + newIndex * elemSize;

	UUrMemory_MoveFast(inElem, newElem, elemSize);

	if(outElemIndex != NULL)
	{
		*outElemIndex = (UUtUns32)newIndex;
	}

	return UUcError_None;
}

static AUtSharedElemArray*	AUgSharedElem_Array = NULL;

static UUtBool
AUiSharedElemArray_Compare(
	UUtUns32 inA,
	UUtUns32 inB)
{
	char*		arrayBase;
	UUtInt16	compare_function_result;
	UUtBool		result;

	UUmAssertReadPtr(AUgSharedElem_Array, sizeof(AUtSharedElemArray));

	arrayBase = AUrSharedElemArray_GetList(AUgSharedElem_Array);

	compare_function_result = AUgSharedElem_Array->compareFunc(
					arrayBase + inA * AUgSharedElem_Array->elemSize,
					arrayBase + inB * AUgSharedElem_Array->elemSize);

	result = compare_function_result > 0;

	return result;
}

UUtError
AUrSharedElemArray_Resort(
	AUtSharedElemArray*	inSharedElemArray)
{
	UUtUns32	numElems;
	UUtUns32	itr;
	UUtUns32*	sortedIndexList;

	numElems = AUrSharedElemArray_GetNum(inSharedElemArray);
	sortedIndexList = AUrSharedElemArray_GetSortedIndexList(inSharedElemArray);

	for(itr = 0; itr < numElems; itr++)
	{
		sortedIndexList[itr] = itr;
	}

	AUgSharedElem_Array = inSharedElemArray;

	AUrQSort_32(
		sortedIndexList,
		numElems,
		AUiSharedElemArray_Compare);

	AUgSharedElem_Array = NULL;

	return UUcError_None;
}


UUtError
AUrSharedElemArray_Reorder(
	AUtSharedElemArray*	inSharedElemArray,
	UUtUns32*			inRemapArray)
{
	char*		tempMemory;
	UUtUns32	numElems;
	char*		originalList;
	UUtUns32	itr;

	numElems = AUrSharedElemArray_GetNum(inSharedElemArray);

	tempMemory = UUrMemory_Block_New(inSharedElemArray->elemSize * numElems);
	UUmError_ReturnOnNull(tempMemory);

	originalList = AUrSharedElemArray_GetList(inSharedElemArray);
	UUmAssertReadPtr(originalList, sizeof(void*));

	UUrMemory_MoveFast(originalList, tempMemory, inSharedElemArray->elemSize * numElems);

	for(itr = 0; itr < numElems; itr++)
	{
		UUmAssert(inRemapArray[itr] < numElems);

		UUrMemory_MoveFast(
			tempMemory + itr * inSharedElemArray->elemSize,
			originalList + inRemapArray[itr] * inSharedElemArray->elemSize,
			inSharedElemArray->elemSize);
	}

	UUrMemory_Block_Delete(tempMemory);

	AUrSharedElemArray_Resort(inSharedElemArray);

	return UUcError_None;
}

void*
AUrSharedElemArray_GetList(
	AUtSharedElemArray*	inSharedElemArray)
{
	void*	result;

	result = UUrMemory_ParallelArray_Member_GetMemory(inSharedElemArray->parallelArray, inSharedElemArray->elemArray);

	UUmAssert(result != NULL);

	return result;
}

UUtUns32
AUrSharedElemArray_GetNum(
	AUtSharedElemArray*	inSharedElemArray)
{
	return (UUtUns32)UUrMemory_ParallelArray_GetNumUsedElems(inSharedElemArray->parallelArray);
}

UUtUns32*
AUrSharedElemArray_GetSortedIndexList(
	AUtSharedElemArray*	inSharedElemArray)
{
	return UUrMemory_ParallelArray_Member_GetMemory(inSharedElemArray->parallelArray, inSharedElemArray->remapIndexArray);
}

/*
 * Shared point functions
 */
static UUtInt16
AUiSharedPointArray_CompareFunc(
	M3tPoint3D*	inPointA,
	M3tPoint3D*	inPointB)
{
	if(UUmFloat_CompareEqu(inPointA->x, inPointB->x))
	{
		if(UUmFloat_CompareEqu(inPointA->y, inPointB->y))
		{
			if(UUmFloat_CompareEqu(inPointA->z, inPointB->z))
			{
				return 0;
			}
			else
			{
				return (inPointA->z > inPointB->z) ? 1 : -1;
			}
		}
		else
		{
			return (inPointA->y > inPointB->y) ? 1 : -1;
		}
	}
	else
	{
		return (inPointA->x > inPointB->x) ? 1 : -1;
	}

	return 0;
}

AUtSharedPointArray*
AUrSharedPointArray_New(
	void)
{
	return (AUtSharedPointArray*)
		AUrSharedElemArray_New(
			sizeof(M3tPoint3D),
			(AUtSharedElemArray_Equal)AUiSharedPointArray_CompareFunc);
}

void
AUrSharedPointArray_Delete(
	AUtSharedPointArray*	inSharedPointArray)
{
	AUrSharedElemArray_Delete((AUtSharedElemArray*)inSharedPointArray);
}

void
AUrSharedPointArray_Reset(
	AUtSharedPointArray*	inSharedPointArray)
{
	AUrSharedElemArray_Reset((AUtSharedElemArray*)inSharedPointArray);
}

void AUrSharedPointArray_RoundPoint(M3tPoint3D *ioPoint)
{
	#if 1
	/* S.S.
	ioPoint->x *= (float) (1.f / UUcEpsilon);
	ioPoint->y *= (float) (1.f / UUcEpsilon);
	ioPoint->z *= (float) (1.f / UUcEpsilon);
	*/
	static const float one_over_epsilon= 1.f / UUcEpsilon;
	ioPoint->x*= one_over_epsilon;
	ioPoint->y*= one_over_epsilon;
	ioPoint->z*= one_over_epsilon;

	ioPoint->x = (float) MUrFloat_Round_To_Int(ioPoint->x);
	ioPoint->y = (float) MUrFloat_Round_To_Int(ioPoint->y);
	ioPoint->z = (float) MUrFloat_Round_To_Int(ioPoint->z);

	ioPoint->x *= UUcEpsilon;
	ioPoint->y *= UUcEpsilon;
	ioPoint->z *= UUcEpsilon;
	#endif

	#if 0
	ioPoint->x = inX;
	ioPoint->y = inY;
	ioPoint->z = inZ;
	#endif

	return;
}

UUtError
AUrSharedPointArray_AddPoint(
	AUtSharedPointArray*	inSharedPointArray,
	float					inX,
	float					inY,
	float					inZ,
	UUtUns32				*outPointIndex)
{
	M3tPoint3D	point3D;

	point3D.x = inX;
	point3D.y = inY;
	point3D.z = inZ;

	AUrSharedPointArray_RoundPoint(&point3D);

	return
		AUrSharedElemArray_AddElem(
			(AUtSharedElemArray*)inSharedPointArray,
			&point3D,
			outPointIndex);
}

UUtError
AUrSharedPointArray_Reorder(
	AUtSharedPointArray*	inSharedPointArray,
	UUtUns32*				inRemapArray)
{
	return AUrSharedElemArray_Reorder(
			(AUtSharedElemArray*)inSharedPointArray,
			inRemapArray);
}

M3tPoint3D*
AUrSharedPointArray_GetList(
	AUtSharedPointArray*	inSharedPointArray)
{
	return AUrSharedElemArray_GetList((AUtSharedElemArray*)inSharedPointArray);
}

UUtUns32
AUrSharedPointArray_GetNum(
	AUtSharedPointArray*	inSharedPointArray)
{
	return AUrSharedElemArray_GetNum((AUtSharedElemArray*)inSharedPointArray);
}

void
AUrSharedPointArray_Dump(
	AUtSharedPointArray*	inSharedPointArray,
	FILE*					inFile)
{
	UUtUns32	i;
	M3tPoint3D*	curPoint;

	for(i = 0, curPoint = AUrSharedPointArray_GetList(inSharedPointArray);
		i < AUrSharedPointArray_GetNum(inSharedPointArray);
		i++, curPoint++)
	{
		fprintf(inFile, "\t%d: (%f, %f, %f)"UUmNL,
			i,
			curPoint->x,
			curPoint->y,
			curPoint->z);
	}

}

UUtError
AUrSharedPointArray_CreateTemplate(
	AUtSharedPointArray*	inSharedPointArray,
	char*					inName,
	M3tPoint3DArray*		*outReference)
{
	UUtError		error;
	M3tPoint3D*		sharedPoints;
	UUtUns32		numSharedPoints;
	M3tPoint3D*		pointArray;			// List of points used by the nodes of this object
	UUtUns32		i;

	sharedPoints = AUrSharedPointArray_GetList(inSharedPointArray);
	numSharedPoints = AUrSharedPointArray_GetNum(inSharedPointArray);

	if(numSharedPoints > UUcMaxUns32)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Too many points in this array");
	}

	error =
		TMrConstruction_Instance_Renew(
			M3cTemplate_Point3DArray,
			inName,
			numSharedPoints + M3cExtraCoords,  // Hack - this is needed for Akira
			outReference);

	if(error != UUcError_None)
	{
		UUmError_ReturnOnErrorMsg(error, "Could not create point array");
	}

	pointArray = (*outReference)->points;

	for(i = 0; i < numSharedPoints; i++)
	{
		pointArray[i] = sharedPoints[i];
	}

	(*outReference)->numPoints = numSharedPoints;

	return UUcError_None;
}

UUtError
AUrSharedPointArray_InterpolatePoint(
	AUtSharedPointArray*	inPointArray,
	float					inA,
	float					inB,
	float					inC,
	float					inD,
	M3tPoint3D*				inInVertex,
	M3tPoint3D*				inOutVertex,
	UUtUns32				*outNewVertexIndex)
{
	/*
		parametric equation of a line:

			X(t) = Xm * t + Xi
			Y(t) = Ym * t + Yi
			Z(t) = Zm * t + Zi

			F(t) = [X(t), Y(t), Z(t)]

			F(0) = inInVertex
			F(1) = inOutVertex

			Xi = inInVertex->x, Xm = (inOutVertex->x - inInVertex->x)
			Yi = inInVertex->y, Ym = (inOutVertex->y - inInVertex->y)
			Zi = inInVertex->z, Zm = (inOutVertex->z - inInVertex->z)

		plane equation:

			a * X + b * Y + c * Z + d = 0

		substitute:

			a * X(t) + b * Y(t) + c * Z(t) + d = 0

			a * (Xm * t + Xi) + b * (Ym * t + Yi) + c * (Zm * t + Zi) + d = 0

			...

			t = -(a * Xi + b * Yi + c * Zi + d) / (a * Xm + b * Ym + c * Zm)

		plug t back into X(t), Y(t), Z(t) for final result

	*/

	UUtError	error;
	float		Xi, Yi, Zi;
	float		Xm, Ym, Zm;
	float		Xn, Yn, Zn;
	float		denom;
	float		t;

	Xi = inInVertex->x;
	Yi = inInVertex->y;
	Zi = inInVertex->z;

	Xm = (inOutVertex->x - Xi);
	Ym = (inOutVertex->y - Yi);
	Zm = (inOutVertex->z - Zi);

	denom = inA * Xm + inB * Ym + inC * Zm;

	UUmAssert(denom != 0.0f);

	t = -(inA * Xi + inB * Yi + inC * Zi + inD) / denom;

	if(t > 0.0011f) t -= 0.0011f;
	else t -= t * 0.1f;

	Xn = Xm * t + Xi;
	Yn = Ym * t + Yi;
	Zn = Zm * t + Zi;

	error =
		AUrSharedPointArray_AddPoint(
			inPointArray,
			Xn,
			Yn,
			Zn,
			outNewVertexIndex);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

/*
 * Shared PlaneEqu functions
 *
 * NOTES:
 *	if d is negative the negative plane equ is stored and the high bit of the index field is set
 *  This means that d is always positive in the actual list of plane equations
 */
static UUtInt16
AUiSharedPlaneEquArray_CompareFunc(
	M3tPlaneEquation*	inPlaneEquA,
	M3tPlaneEquation*	inPlaneEquB)
{
	UUmAssertReadPtr(inPlaneEquA, sizeof(*inPlaneEquA));
	UUmAssertReadPtr(inPlaneEquB, sizeof(*inPlaneEquB));

	if(fabs(inPlaneEquA->a - inPlaneEquB->a) < AUcSharedPlane_NormalTolerance)
	{
		if(fabs(inPlaneEquA->b - inPlaneEquB->b) < AUcSharedPlane_NormalTolerance)
		{
			if(fabs(inPlaneEquA->c - inPlaneEquB->c) < AUcSharedPlane_NormalTolerance)
			{
				if(fabs(inPlaneEquA->d - inPlaneEquB->d) < AUcSharedPlane_OffsetTolerance)
				{
					return 0;
				}
				else
				{
					return UUmFloat_CompareGT(inPlaneEquA->d, inPlaneEquB->d) ? 1 : -1;
				}
			}
			else
			{
				return UUmFloat_CompareGT(inPlaneEquA->c, inPlaneEquB->c) ? 1 : -1;
			}
		}
		else
		{
			return UUmFloat_CompareGT(inPlaneEquA->b, inPlaneEquB->b) ? 1 : -1;
		}
	}
	else
	{
		return UUmFloat_CompareGT(inPlaneEquA->a, inPlaneEquB->a) ? 1 : -1;
	}

	return 0;
}

AUtSharedPlaneEquArray*
AUrSharedPlaneEquArray_New(
	void)
{
	return (AUtSharedPlaneEquArray*)
		AUrSharedElemArray_New(
			sizeof(M3tPlaneEquation),
			(AUtSharedElemArray_Equal)AUiSharedPlaneEquArray_CompareFunc);
}

void
AUrSharedPlaneEquArray_Delete(
	AUtSharedPlaneEquArray*	inSharedPlaneEquArray)
{
	AUrSharedElemArray_Delete((AUtSharedElemArray*)inSharedPlaneEquArray);
}

void
AUrSharedPlaneEquArray_Reset(
	AUtSharedPlaneEquArray*	inSharedPlaneEquArray)
{
	AUrSharedElemArray_Reset((AUtSharedElemArray*)inSharedPlaneEquArray);
}

UUtError
AUrSharedPlaneEquArray_AddPlaneEqu(
	AUtSharedPlaneEquArray*	inSharedPlaneEquArray,
	float					inA,
	float					inB,
	float					inC,
	float					inD,
	UUtUns32				*outPlaneEquIndex)
{
	UUtError			error;
	UUtUns32			result;
	UUtUns32			highBit;
	M3tPlaneEquation	planeEqu;
	AUtSharedElemArray*	sharedElemArray = (AUtSharedElemArray*)inSharedPlaneEquArray;

	if(inD != 0.0f)
	{
		if(inD < 0.0f)
		{
			planeEqu.a = -inA;
			planeEqu.b = -inB;
			planeEqu.c = -inC;
			planeEqu.d = -inD;

			highBit = 0x80000000;

		}
		else
		{
			planeEqu.a = inA;
			planeEqu.b = inB;
			planeEqu.c = inC;
			planeEqu.d = inD;

			highBit = 0;
		}

		error =
			AUrSharedElemArray_AddElem(
				(AUtSharedElemArray*)inSharedPlaneEquArray,
				&planeEqu,
				&result);
		UUmError_ReturnOnErrorMsg(error, "could not add new elem");

		if(outPlaneEquIndex != NULL)
		{
			*outPlaneEquIndex = result | highBit;
		}

		UUmAssert(result < 0x7FFFFFFF);
	}
	else
	{
		UUtUns32			curPlaneEquIndex;
		M3tPlaneEquation*	curPlaneEqu;

		for(curPlaneEquIndex = 0,
				curPlaneEqu = (M3tPlaneEquation*)UUrMemory_ParallelArray_Member_GetMemory(sharedElemArray->parallelArray, sharedElemArray->elemArray);
			curPlaneEquIndex < UUrMemory_ParallelArray_GetNumUsedElems(sharedElemArray->parallelArray);
			curPlaneEquIndex++, curPlaneEqu++)
		{
			if(UUmFloat_CompareEqu(curPlaneEqu->a, -inA) &&
				UUmFloat_CompareEqu(curPlaneEqu->b, -inB) &&
				UUmFloat_CompareEqu(curPlaneEqu->c, -inC) &&
				UUmFloat_CompareEqu(curPlaneEqu->d, 0.0f))
			{
				if(outPlaneEquIndex != NULL)
				{
					*outPlaneEquIndex = (curPlaneEquIndex | 0x80000000);
				}
				return UUcError_None;
			}
			if(UUmFloat_CompareEqu(curPlaneEqu->a, inA) &&
				UUmFloat_CompareEqu(curPlaneEqu->b, inB) &&
				UUmFloat_CompareEqu(curPlaneEqu->c, inC) &&
				UUmFloat_CompareEqu(curPlaneEqu->d, 0.0f))
			{
				if(outPlaneEquIndex != NULL)
				{
					*outPlaneEquIndex = curPlaneEquIndex;
				}
				return UUcError_None;
			}
		}

		planeEqu.a = inA;
		planeEqu.b = inB;
		planeEqu.c = inC;
		planeEqu.d = inD;

		error =
			AUrSharedElemArray_AddElem(
				(AUtSharedElemArray*)inSharedPlaneEquArray,
				&planeEqu,
				&result);
		UUmError_ReturnOnErrorMsg(error, "could not add new elem");

		if(outPlaneEquIndex != NULL)
		{
			*outPlaneEquIndex = result;
		}

		UUmAssert(result < 0x7FFFFFFF);
	}

	return UUcError_None;
}

M3tPlaneEquation*
AUrSharedPlaneEquArray_GetList(
	AUtSharedPlaneEquArray*	inSharedPlaneEquArray)
{
	return AUrSharedElemArray_GetList((AUtSharedElemArray*)inSharedPlaneEquArray);
}

UUtUns32
AUrSharedPlaneEquArray_GetNum(
	AUtSharedPlaneEquArray*	inSharedPlaneEquArray)
{
	UUtUns32	num;

	num = AUrSharedElemArray_GetNum((AUtSharedElemArray*)inSharedPlaneEquArray);

	UUmAssert(num < UUcMaxUns32);

	return num;
}

void
AUrSharedPlaneEquArray_Dump(
	AUtSharedPlaneEquArray*	inSharedPlaneEquArray,
	FILE*					inFile)
{
	UUtUns32			i;
	M3tPlaneEquation*	curPlaneEqu;

	for(i = 0, curPlaneEqu = AUrSharedPlaneEquArray_GetList(inSharedPlaneEquArray);
		i < AUrSharedPlaneEquArray_GetNum(inSharedPlaneEquArray);
		i++, curPlaneEqu++)
	{
		fprintf(inFile, "\t%d: (%f, %f, %f, %f)"UUmNL,
			i,
			curPlaneEqu->a,
			curPlaneEqu->b,
			curPlaneEqu->c,
			curPlaneEqu->d);
	}
}

UUtError
AUrSharedPlaneEquArray_CreateTemplate(
	AUtSharedPlaneEquArray*	inSharedPlaneEquArray,
	char*					inName,
	M3tPlaneEquationArray*	*outReference)
{
	UUtError			error;
	M3tPlaneEquation*	sharedPlaneEqus;
	UUtUns32			numSharedPlaneEqus;
	M3tPlaneEquation*	planeEquArray;			// List of points used by the nodes of this object
	UUtUns32			i;

	sharedPlaneEqus = AUrSharedPlaneEquArray_GetList(inSharedPlaneEquArray);
	numSharedPlaneEqus = AUrSharedPlaneEquArray_GetNum(inSharedPlaneEquArray);

	error =
		TMrConstruction_Instance_Renew(
			M3cTemplate_PlaneEquationArray,
			inName,
			numSharedPlaneEqus,
			outReference);

	if(error != UUcError_None)
	{
		UUmError_ReturnOnErrorMsg(error, "Could not create PlaneEqu array");
	}

	planeEquArray = (*outReference)->planes;

	for(i = 0; i < numSharedPlaneEqus; i++)
	{
		planeEquArray[i] = sharedPlaneEqus[i];
	}

	return UUcError_None;
}

UUtBool
AUrSharedPlaneEquArray_FindPlaneEqu(
	AUtSharedPlaneEquArray*	inSharedPlaneEquArray,
	float					inA,
	float					inB,
	float					inC,
	float					inD,
	UUtUns32				*outPlaneEquIndex)
{
	M3tPlaneEquation	planeEqu;
	UUtUns32*			remapArray;
	AUtSharedElemArray*	sharedElemArray = (AUtSharedElemArray*)inSharedPlaneEquArray;
	UUtUns32			highBit;
	UUtUns32			foundIndex;

	if(inD < 0.0f)
	{
		planeEqu.a = -inA;
		planeEqu.b = -inB;
		planeEqu.c = -inC;
		planeEqu.d = -inD;

		highBit = 0x80000000;
	}
	else
	{
		planeEqu.a = inA;
		planeEqu.b = inB;
		planeEqu.c = inC;
		planeEqu.d = inD;

		highBit = 0;
	}

	if(AUrSharedElemArray_FindElem(
		sharedElemArray,
		&planeEqu,
		&foundIndex) == UUcTrue)
	{
		UUmAssert(foundIndex < UUcMaxUns32);

		*outPlaneEquIndex = (foundIndex | highBit);

		return UUcTrue;
	}

	if(inD == 0.0f)
	{
		planeEqu.a = -inA;
		planeEqu.b = -inB;
		planeEqu.c = -inC;

		if(AUrSharedElemArray_FindElem(
			sharedElemArray,
			&planeEqu,
			&foundIndex) == UUcTrue)
		{
			UUmAssert(foundIndex < UUcMaxUns32);

			*outPlaneEquIndex = foundIndex;

			return UUcTrue;
		}
	}

	remapArray = (UUtUns32*)UUrMemory_ParallelArray_Member_GetMemory(
							sharedElemArray->parallelArray,
							sharedElemArray->remapIndexArray);

	*outPlaneEquIndex = remapArray[*outPlaneEquIndex];

	return UUcFalse;
}

/*
 * Shared Quad functions
 */
static UUtInt16
AUiSharedQuadArray_CompareFunc(
	M3tQuad*	inQuadA,
	M3tQuad*	inQuadB)
{
	if(
		inQuadA->indices[0] == inQuadB->indices[0] &&
		inQuadA->indices[1] == inQuadB->indices[1] &&
		inQuadA->indices[2] == inQuadB->indices[2] &&
		inQuadA->indices[3] == inQuadB->indices[3])
	{
		return 0;
	}

	if(
		inQuadA->indices[0] == inQuadB->indices[1] &&
		inQuadA->indices[1] == inQuadB->indices[2] &&
		inQuadA->indices[2] == inQuadB->indices[3] &&
		inQuadA->indices[3] == inQuadB->indices[0])
	{
		return 0;
	}

	if(
		inQuadA->indices[0] == inQuadB->indices[2] &&
		inQuadA->indices[1] == inQuadB->indices[3] &&
		inQuadA->indices[2] == inQuadB->indices[0] &&
		inQuadA->indices[3] == inQuadB->indices[1])
	{
		return 0;
	}

	if(
		inQuadA->indices[0] == inQuadB->indices[3] &&
		inQuadA->indices[1] == inQuadB->indices[0] &&
		inQuadA->indices[2] == inQuadB->indices[1] &&
		inQuadA->indices[3] == inQuadB->indices[2])
	{
		return 0;
	}

	if(
		inQuadA->indices[0] == inQuadB->indices[3] &&
		inQuadA->indices[1] == inQuadB->indices[2] &&
		inQuadA->indices[2] == inQuadB->indices[1] &&
		inQuadA->indices[3] == inQuadB->indices[0])
	{
		return 0;
	}

	if(
		inQuadA->indices[0] == inQuadB->indices[2] &&
		inQuadA->indices[1] == inQuadB->indices[1] &&
		inQuadA->indices[2] == inQuadB->indices[0] &&
		inQuadA->indices[3] == inQuadB->indices[3])
	{
		return 0;
	}

	if(
		inQuadA->indices[0] == inQuadB->indices[1] &&
		inQuadA->indices[1] == inQuadB->indices[0] &&
		inQuadA->indices[2] == inQuadB->indices[3] &&
		inQuadA->indices[3] == inQuadB->indices[2])
	{
		return 0;
	}

	if(
		inQuadA->indices[0] == inQuadB->indices[0] &&
		inQuadA->indices[1] == inQuadB->indices[3] &&
		inQuadA->indices[2] == inQuadB->indices[2] &&
		inQuadA->indices[3] == inQuadB->indices[1])
	{
		return 0;
	}

	if(inQuadA->indices[0] == inQuadB->indices[0])
	{
		if(inQuadA->indices[1] == inQuadB->indices[1])
		{
			if(inQuadA->indices[2] == inQuadB->indices[2])
			{
				return inQuadA->indices[3] > inQuadB->indices[3] ? 1 : -1;
			}
			else
			{
				return inQuadA->indices[2] > inQuadB->indices[2] ? 1 : -1;
			}
		}
		else
		{
			return inQuadA->indices[1] > inQuadB->indices[1] ? 1 : -1;
		}
	}
	else
	{
		return inQuadA->indices[0] > inQuadB->indices[0] ? 1 : -1;
	}

	return 0;
}

AUtSharedQuadArray*
AUrSharedQuadArray_New(
	void)
{
	return (AUtSharedQuadArray*)
		AUrSharedElemArray_New(
			sizeof(M3tQuad),
			(AUtSharedElemArray_Equal)AUiSharedQuadArray_CompareFunc);
}

void
AUrSharedQuadArray_Delete(
	AUtSharedQuadArray*	inSharedQuadArray)
{
	AUrSharedElemArray_Delete((AUtSharedElemArray*)inSharedQuadArray);
}

void
AUrSharedQuadArray_Reset(
	AUtSharedQuadArray*	inSharedQuadArray)
{
	AUrSharedElemArray_Reset((AUtSharedElemArray*)inSharedQuadArray);
}

UUtError
AUrSharedQuadArray_AddQuad(
	AUtSharedQuadArray*	inSharedQuadArray,
	UUtUns32			inIndex0,
	UUtUns32			inIndex1,
	UUtUns32			inIndex2,
	UUtUns32			inIndex3,
	UUtUns32			*outQuadIndex)
{
	M3tQuad	quad;

	UUmAssert(inIndex0 < 0x80000000);
	UUmAssert(inIndex1 < 0x80000000);
	UUmAssert(inIndex2 < 0x80000000);
	UUmAssert(inIndex3 == UUcMaxUns32 || inIndex3 < 0x80000000);

	quad.indices[0] = inIndex0;
	quad.indices[1] = inIndex1;
	quad.indices[2] = inIndex2;
	quad.indices[3] = inIndex3;

	return
		AUrSharedElemArray_AddElem(
			(AUtSharedElemArray*)inSharedQuadArray,
			&quad,
			outQuadIndex);
}

UUtError
AUrSharedQuadArray_Resort(
	AUtSharedQuadArray*		inSharedQuadArray)
{
	return AUrSharedElemArray_Resort((AUtSharedElemArray*)inSharedQuadArray);
}

M3tQuad*
AUrSharedQuadArray_GetList(
	AUtSharedQuadArray*		inSharedQuadArray)
{
	return AUrSharedElemArray_GetList((AUtSharedElemArray*)inSharedQuadArray);
}

UUtUns32
AUrSharedQuadArray_GetNum(
	AUtSharedQuadArray*	inSharedQuadArray)
{
	return AUrSharedElemArray_GetNum((AUtSharedElemArray*)inSharedQuadArray);
}

void
AUrSharedQuadArray_Dump(
	AUtSharedQuadArray*	inSharedQuadArray,
	FILE*				inFile)
{
	UUtUns32			i;
	M3tQuad*		curQuad;

	for(i = 0, curQuad = AUrSharedQuadArray_GetList(inSharedQuadArray);
		i < AUrSharedQuadArray_GetNum(inSharedQuadArray);
		i++, curQuad++)
	{
		fprintf(inFile, "\t%d: (%d, %d, %d, %d)"UUmNL,
			i,
			curQuad->indices[0],
			curQuad->indices[1],
			curQuad->indices[2],
			curQuad->indices[3]);
	}
}

UUtError
AUrSharedQuadArray_CreateTemplate(
	AUtSharedQuadArray*		inSharedQuadArray,
	char*					inName,
	M3tQuadArray*			*outReference)
{
	UUtError			error;
	M3tQuad*		sharedQuads;
	UUtUns32			numSharedQuads;
	M3tQuad*			quadArray;
	UUtUns32			i;

	sharedQuads = AUrSharedQuadArray_GetList(inSharedQuadArray);
	numSharedQuads = AUrSharedQuadArray_GetNum(inSharedQuadArray);

	error =
		TMrConstruction_Instance_Renew(
			M3cTemplate_QuadArray,
			inName,
			numSharedQuads,
			outReference);

	if(error != UUcError_None)
	{
		UUmError_ReturnOnErrorMsg(error, "Could not create Quad array");
	}

	quadArray = (*outReference)->quads;

	for(i = 0; i < numSharedQuads; i++)
	{
		UUmAssert(sharedQuads[i].indices[0] < UUcMaxUns32);
		UUmAssert(sharedQuads[i].indices[1] < UUcMaxUns32);
		UUmAssert(sharedQuads[i].indices[2] < UUcMaxUns32);
		UUmAssert(sharedQuads[i].indices[3] < UUcMaxUns32);

		quadArray[i].indices[0] = sharedQuads[i].indices[0];
		quadArray[i].indices[1] = sharedQuads[i].indices[1];
		quadArray[i].indices[2] = sharedQuads[i].indices[2];
		quadArray[i].indices[3] = sharedQuads[i].indices[3];
	}

	return UUcError_None;
}

/*
 * Shared Vector functions
 */
static UUtInt32
AUiSharedVectorArray_CompareFunc(
	M3tVector3D*	inVectorA,
	M3tVector3D*	inVectorB)
{
	if(UUmFloat_CompareEqu(inVectorA->x, inVectorB->x))
	{
		if(UUmFloat_CompareEqu(inVectorA->y, inVectorB->y))
		{
			if(UUmFloat_CompareEqu(inVectorA->z, inVectorB->z))
			{
				return 0;
			}
			else
			{
				return UUmFloat_CompareGT(inVectorA->z, inVectorB->z) ? 1 : -1;
			}
		}
		else
		{
			return UUmFloat_CompareGT(inVectorA->y, inVectorB->y) ? 1 : -1;
		}
	}
	else
	{
		return UUmFloat_CompareGT(inVectorA->x, inVectorB->x) ? 1 : -1;
	}
}

AUtSharedVectorArray*
AUrSharedVectorArray_New(
	void)
{
	return (AUtSharedVectorArray*)
		AUrSharedElemArray_New(
			sizeof(M3tVector3D),
			(AUtSharedElemArray_Equal)AUiSharedVectorArray_CompareFunc);
}

void
AUrSharedVectorArray_Delete(
	AUtSharedVectorArray*	inSharedVectorArray)
{
	AUrSharedElemArray_Delete((AUtSharedElemArray*)inSharedVectorArray);
}

void
AUrSharedVectorArray_Reset(
	AUtSharedVectorArray*	inSharedVectorArray)
{
	AUrSharedElemArray_Reset((AUtSharedElemArray*)inSharedVectorArray);
}

UUtError
AUrSharedVectorArray_AddVector(
	AUtSharedVectorArray*	inSharedVectorArray,
	float					inX,
	float					inY,
	float					inZ,
	UUtUns32				*outVectorIndex)
{
	M3tVector3D	vector3D;

	vector3D.x = inX;
	vector3D.y = inY;
	vector3D.z = inZ;

	return
		AUrSharedElemArray_AddElem(
			(AUtSharedElemArray*)inSharedVectorArray,
			&vector3D,
			outVectorIndex);
}

M3tVector3D*
AUrSharedVectorArray_GetList(
	AUtSharedVectorArray*	inSharedVectorArray)
{
	return AUrSharedElemArray_GetList((AUtSharedElemArray*)inSharedVectorArray);
}

UUtUns32
AUrSharedVectorArray_GetNum(
	AUtSharedVectorArray*	inSharedVectorArray)
{
	return AUrSharedElemArray_GetNum((AUtSharedElemArray*)inSharedVectorArray);
}

void
AUrSharedVectorArray_Dump(
	AUtSharedVectorArray*	inSharedVectorArray,
	FILE*					inFile)
{
	UUtUns32	i;
	M3tVector3D*	curVector;

	for(i = 0, curVector = AUrSharedVectorArray_GetList(inSharedVectorArray);
		i < AUrSharedVectorArray_GetNum(inSharedVectorArray);
		i++, curVector++)
	{
		fprintf(inFile, "\t%d: (%f, %f, %f)"UUmNL,
			i,
			curVector->x,
			curVector->y,
			curVector->z);
	}
}

UUtError
AUrSharedVectorArray_CreateTemplate(
	AUtSharedVectorArray*	inSharedVectorArray,
	char*					inName,
	M3tVector3DArray*		*outReference)
{
	UUtError		error;
	M3tVector3D*	sharedVectors;
	UUtUns32		numSharedVectors;
	M3tVector3D*	vectorArray;
	UUtUns32		i;

	sharedVectors = AUrSharedVectorArray_GetList(inSharedVectorArray);
	numSharedVectors = AUrSharedVectorArray_GetNum(inSharedVectorArray);

	error =
		TMrConstruction_Instance_Renew(
			M3cTemplate_Vector3DArray,
			inName,
			numSharedVectors,
			outReference);

	if(error != UUcError_None)
	{
		UUmError_ReturnOnErrorMsg(error, "Could not create Vector array");
	}

	vectorArray = (*outReference)->vectors;

	for(i = 0; i < numSharedVectors; i++)
	{
		vectorArray[i] = sharedVectors[i];
	}

	return UUcError_None;
}

/*
 * Shared TexCoord functions
 */
static UUtInt16
AUiSharedTexCoordArray_CompareFunc(
	M3tTextureCoord*	inTexCoordA,
	M3tTextureCoord*	inTexCoordB)
{
	if(UUmFloat_CompareEqu(inTexCoordA->u, inTexCoordB->u))
	{
		if(UUmFloat_CompareEqu(inTexCoordA->v, inTexCoordB->v))
		{
			return 0;
		}
		else
		{
			return UUmFloat_CompareGT(inTexCoordA->v, inTexCoordB->v) ? 1 : -1;
		}
	}
	else
	{
		return UUmFloat_CompareGT(inTexCoordA->u, inTexCoordB->u) ? 1 : -1;
	}

	return 0;
}

AUtSharedTexCoordArray*
AUrSharedTexCoordArray_New(
	void)
{
	return (AUtSharedTexCoordArray*)
		AUrSharedElemArray_New(
			sizeof(M3tTextureCoord),
			(AUtSharedElemArray_Equal)AUiSharedTexCoordArray_CompareFunc);
}

void
AUrSharedTexCoordArray_Delete(
	AUtSharedTexCoordArray*	inSharedTexCoordArray)
{
	AUrSharedElemArray_Delete((AUtSharedElemArray*)inSharedTexCoordArray);
}

void
AUrSharedTexCoordArray_Reset(
	AUtSharedTexCoordArray*	inSharedTexCoordArray)
{
	AUrSharedElemArray_Reset((AUtSharedElemArray*)inSharedTexCoordArray);
}

UUtError
AUrSharedTexCoordArray_AddTexCoord(
	AUtSharedTexCoordArray*	inSharedTexCoordArray,
	float					inU,
	float					inV,
	UUtUns32				*outTexCoordIndex)
{
	M3tTextureCoord	texCoord;

	#if 0
	inU *= 1.0f / UUcEpsilon;
	inV *= 1.0f / UUcEpsilon;

	inU = UUmFloat_Round(inU);
	inV = UUmFloat_Round(inV);

	inU *= UUcEpsilon;
	inV *= UUcEpsilon;
	#endif

	texCoord.u = inU;
	texCoord.v = inV;

	return
		AUrSharedElemArray_AddElem(
			(AUtSharedElemArray*)inSharedTexCoordArray,
			&texCoord,
			outTexCoordIndex);
}

M3tTextureCoord*
AUrSharedTexCoordArray_GetList(
	AUtSharedTexCoordArray*	inSharedTexCoordArray)
{
	return AUrSharedElemArray_GetList((AUtSharedElemArray*)inSharedTexCoordArray);
}

UUtUns32
AUrSharedTexCoordArray_GetNum(
	AUtSharedTexCoordArray*	inSharedTexCoordArray)
{
	return AUrSharedElemArray_GetNum((AUtSharedElemArray*)inSharedTexCoordArray);
}

UUtError
AUrSharedTexCoordArray_CreateTemplate(
	AUtSharedTexCoordArray*	inSharedTexCoordArray,
	char*					inName,
	M3tTextureCoordArray*	*outReference)
{
	UUtError			error;
	M3tTextureCoord*	sharedTexCoords;
	UUtUns32			numSharedTexCoords;
	M3tTextureCoord*	texCoordArray;
	UUtUns32			i;

	sharedTexCoords = AUrSharedTexCoordArray_GetList(inSharedTexCoordArray);
	numSharedTexCoords = AUrSharedTexCoordArray_GetNum(inSharedTexCoordArray);

	if(numSharedTexCoords > UUcMaxUns32)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Too many texture coords");
	}

	error =
		TMrConstruction_Instance_Renew(
			M3cTemplate_TextureCoordArray,
			inName,
			numSharedTexCoords + M3cExtraCoords,
			outReference);

	if(error != UUcError_None)
	{
		UUmError_ReturnOnErrorMsg(error, "Could not create TexCoord array");
	}

	texCoordArray = (*outReference)->textureCoords;

	for(i = 0; i < numSharedTexCoords; i++)
	{
		texCoordArray[i] = sharedTexCoords[i];
	}

	(*outReference)->numTextureCoords = numSharedTexCoords;

	return UUcError_None;
}

/*
 * Shared edge functions
 */
static UUtInt16
AUiSharedEdgeArray_CompareFunc(
	AUtEdge*	inEdgeA,
	AUtEdge*	inEdgeB)
{
	if(inEdgeA->vIndex0 == inEdgeB->vIndex0)
	{
		if(inEdgeA->vIndex1 == inEdgeB->vIndex1)
		{
			return 0;
		}
		else
		{
			return inEdgeA->vIndex1 > inEdgeB->vIndex1 ? 1 : -1;
		}
	}
	else
	{
		return inEdgeA->vIndex0 > inEdgeB->vIndex0 ? 1 : -1;
	}

	return 0;
}

AUtSharedEdgeArray*
AUrSharedEdgeArray_New(
	void)
{
	return (AUtSharedEdgeArray*)
		AUrSharedElemArray_New(
			sizeof(AUtEdge),
			(AUtSharedElemArray_Equal)AUiSharedEdgeArray_CompareFunc);
}

void
AUrSharedEdgeArray_Delete(
	AUtSharedEdgeArray*	inSharedEdgeArray)
{
	AUrSharedElemArray_Delete((AUtSharedElemArray*)inSharedEdgeArray);
}

void
AUrSharedEdgeArray_Reset(
	AUtSharedEdgeArray*	inSharedEdgeArray)
{
	AUrSharedElemArray_Reset((AUtSharedElemArray*)inSharedEdgeArray);
}

UUtError
AUrSharedEdgeArray_AddEdge(
	AUtSharedEdgeArray*	inSharedEdgeArray,
	UUtUns32			inEdge0,
	UUtUns32			inEdge1,
	UUtUns32			inQuadIndex,
	UUtUns32			*outEdgeIndex)
{
	UUtError	error;
	AUtEdge		edge;
	UUtUns32	resultIndex;
	AUtEdge*	resultEdge;
	UUtUns32	i;

	if(inEdge0 < inEdge1)
	{
		edge.vIndex0 = inEdge0;
		edge.vIndex1 = inEdge1;
	}
	else
	{
		edge.vIndex0 = inEdge1;
		edge.vIndex1 = inEdge0;
	}

	for(i = 0; i < AUcMaxQuadsPerEdge; i++)
	{
		edge.quadIndices[i] = UUcMaxUns32;
	}

	error =
		AUrSharedElemArray_AddElem(
			(AUtSharedElemArray*)inSharedEdgeArray,
			&edge,
			&resultIndex);
	UUmError_ReturnOnError(error);

	resultEdge =
		AUrSharedEdgeArray_GetList(inSharedEdgeArray) + resultIndex;

	for(i = 0; i < AUcMaxQuadsPerEdge; i++)
	{
		if(resultEdge->quadIndices[i] == UUcMaxUns32)
		{
			resultEdge->quadIndices[i] = inQuadIndex;
			break;
		}
	}

	if(i >= AUcMaxQuadsPerEdge)
	{
		UUmError_ReturnOnErrorMsgP(UUcError_Generic, "An edge is shared by more then %d  quads, talk to brent", AUcMaxQuadsPerEdge, 0, 0);
	}

	if(outEdgeIndex != NULL)
	{
		*outEdgeIndex = resultIndex;
	}

	return UUcError_None;
}

UUtError
AUrSharedEdgeArray_AddPoly(
	AUtSharedEdgeArray*	inSharedEdgeArray,
		UUtUns32		inNumVertices,
		UUtUns32*		inVertices,
		UUtUns32		inQuadIndex,
		UUtUns32		*outEdgeIndices)
{
	UUtError	error;
	UUtUns32	curVertexItr;
	UUtUns32	vIndex0;
	UUtUns32	vIndex1;

	UUmAssertReadPtr(inVertices, sizeof(UUtUns32) * inNumVertices);
	UUmAssertWritePtr(outEdgeIndices, sizeof(UUtUns32) * inNumVertices);

	for(curVertexItr = 0; curVertexItr < inNumVertices; curVertexItr++)
	{
		vIndex0 = curVertexItr;
		vIndex1 = (curVertexItr + 1) % inNumVertices;

		error =
			AUrSharedEdgeArray_AddEdge(
				inSharedEdgeArray,
				inVertices[vIndex0],
				inVertices[vIndex1],
				inQuadIndex,
				&outEdgeIndices[curVertexItr]);
		UUmError_ReturnOnError(error);
	}

	return UUcError_None;
}

AUtEdge*
AUrSharedEdgeArray_GetList(
	AUtSharedEdgeArray*	inSharedEdgeArray)
{
	return AUrSharedElemArray_GetList((AUtSharedElemArray*)inSharedEdgeArray);
}

UUtUns32
AUrSharedEdgeArray_GetNum(
	AUtSharedEdgeArray*	inSharedEdgeArray)
{
	return AUrSharedElemArray_GetNum((AUtSharedElemArray*)inSharedEdgeArray);
}

UUtUns32
AUrSharedEdge_FindCommonVertex(
	AUtEdge*	inEdgeA,
	AUtEdge*	inEdgeB)
{
	if(inEdgeA->vIndex0 == inEdgeB->vIndex0) return inEdgeA->vIndex0;
	else if(inEdgeA->vIndex0 == inEdgeB->vIndex1) return inEdgeA->vIndex0;
	else if(inEdgeA->vIndex1 == inEdgeB->vIndex1) return inEdgeA->vIndex1;
	else if(inEdgeA->vIndex1 == inEdgeB->vIndex0) return inEdgeA->vIndex1;

	return UUcMaxUns32;
}

UUtUns32
AUrSharedEdge_FindOtherVertex(
	AUtEdge*	inEdge,
	UUtUns32	inVertexIndex)
{
	if(inEdge->vIndex0 == inVertexIndex) return inEdge->vIndex1;
	else if(inEdge->vIndex1 == inVertexIndex) return inEdge->vIndex0;

	return UUcMaxUns32;
}

UUtUns32
AUrSharedEdge_GetOtherFaceIndex(
	AUtEdge*	inEdge,
	UUtUns32	inFaceIndex)
{
	UUtUns32	itr;

	for(itr = 0; itr < AUcMaxQuadsPerEdge; itr++)
	{
		if(inEdge->quadIndices[itr] != inFaceIndex) return inEdge->quadIndices[itr];
	}

	return UUcMaxUns32;
}

AUtEdge*
AUrSharedEdge_Triangle_FindEdge(
	AUtSharedEdgeArray*	inSharedEdgeArray,
	M3tTri*		inTriEdges,
	UUtUns32			inIndex0,
	UUtUns32			inIndex1)
{
	UUtUns32		itr;
	AUtEdge*		targetEdge;
	AUtEdge*		edgeList;

	edgeList = (AUtEdge*)AUrSharedElemArray_GetList((AUtSharedElemArray*)inSharedEdgeArray);

	for(itr = 0; itr < 3; itr++)
	{
		targetEdge = edgeList + inTriEdges->indices[itr];
		if((targetEdge->vIndex0 == inIndex0 && targetEdge->vIndex1 == inIndex1) ||
			(targetEdge->vIndex0 == inIndex1 && targetEdge->vIndex1 == inIndex0)) return targetEdge;
	}

	return NULL;
}

static UUtUns32
AUiSharedStringArray_CompareFunc(
	AUtSharedString*	inElemA,
	AUtSharedString*	inElemB)
{
	return strcmp(inElemA->string, inElemB->string);
}

AUtSharedStringArray*
AUrSharedStringArray_New(
	void)
{
	return (AUtSharedStringArray*)
		AUrSharedElemArray_New(
			sizeof(AUtSharedString),
			(AUtSharedElemArray_Equal)AUiSharedStringArray_CompareFunc);
}

void
AUrSharedStringArray_Delete(
	AUtSharedStringArray*	inSharedStringArray)
{
	AUrSharedElemArray_Delete((AUtSharedElemArray*)inSharedStringArray);
}

void
AUrSharedStringArray_Reset(
	AUtSharedStringArray*	inSharedStringArray)
{
	AUrSharedElemArray_Reset((AUtSharedElemArray*)inSharedStringArray);
}

UUtError
AUrSharedStringArray_AddString(
	AUtSharedStringArray*	inSharedStringArray,
	const char*				inString,
	UUtUns32				inData,
	UUtUns32				*outStringIndex)
{

	AUtSharedString	newString;
	UUtUns32		stringIndex;

	if(strlen(inString) > AUcMaxStringLength)
	{
		UUmError_ReturnOnErrorMsg(UUcError_None, "String too long");
	}

	if(outStringIndex == NULL) outStringIndex = &stringIndex;

	UUrString_Copy(newString.string, inString, AUcMaxStringLength);
	newString.data = inData;

	return AUrSharedElemArray_AddElem(
			(AUtSharedElemArray*)inSharedStringArray,
			&newString,
			outStringIndex);
}

AUtSharedString*
AUrSharedStringArray_GetList(
	AUtSharedStringArray*	inSharedStringArray)
{
	return AUrSharedElemArray_GetList((AUtSharedElemArray*)inSharedStringArray);
}

UUtUns32
AUrSharedStringArray_GetNum(
	AUtSharedStringArray*	inSharedStringArray)
{
	return AUrSharedElemArray_GetNum((AUtSharedElemArray*)inSharedStringArray);
}

UUtBool
AUrSharedStringArray_GetIndex(
	AUtSharedStringArray*	inSharedStringArray,
	const char*				inString,
	UUtUns32				*outIndex)
{
	AUtSharedString	newString;

	if(strlen(inString) > AUcMaxStringLength)
	{
		UUmError_ReturnOnErrorMsg(UUcError_None, "String too long");
	}

	UUrString_Copy(newString.string, inString, AUcMaxStringLength);

	return AUrSharedElemArray_FindElem(
			(AUtSharedElemArray*)inSharedStringArray,
			&newString,
			outIndex);
}

UUtBool
AUrSharedStringArray_GetData(
	AUtSharedStringArray*	inSharedStringArray,
	const char*				inString,
	UUtUns32				*outData)
{
	UUtUns32			index;
	AUtSharedString*	desiredString;


	if(AUrSharedStringArray_GetIndex(
		inSharedStringArray,
		inString,
		&index) == UUcFalse)
	{
		return UUcFalse;
	}

	desiredString = AUrSharedStringArray_GetList(inSharedStringArray) + index;

	*outData = desiredString->data;

	return UUcTrue;
}

UUtUns32*
AUrSharedStringArray_GetSortedIndexList(
	AUtSharedStringArray*	inSharedStringArray)
{
	return AUrSharedElemArray_GetSortedIndexList((AUtSharedElemArray*)inSharedStringArray);
}

UUtUns32 AUrHash_4Byte_Hash(void *inElement)
{
	UUtUns32 value = *((UUtUns32 *) inElement);

	return value;
}

UUtBool AUrHash_4Byte_IsEqual(void *inElement1, void *inElement2)
{
	UUtUns32 value1 = *((UUtUns32 *) inElement1);
	UUtUns32 value2 = *((UUtUns32 *) inElement2);
	UUtBool isEqual;

	isEqual = value1 == value2;

	return isEqual;
}

UUtUns32 AUrString_GetHashValue(const char *inString)
{
	// From the Dragon Book																																					*/
	UUtUns32 lVal = 0, i;
	char	c;

	while ((c = *inString++) != 0)
	{
		lVal = (lVal << 4) + c;
		i = lVal & 0xf0000000;
		if (i) lVal = (lVal ^ (i >> 24)) & i;
	}

	return lVal;
}

UUtUns32 AUrHash_String_Hash(void *inElement)
{
	const char *string = *((char **) inElement);
	UUtUns32 result = AUrString_GetHashValue(string);

	return result;
}

UUtBool AUrHash_String_IsEqual(void *inElement1, void *inElement2)
{
	char *string1 = *((char **) inElement1);
	char *string2 = *((char **) inElement2);
	UUtBool isEqual;

	isEqual = 0 == strcmp(string1, string2);

	return isEqual;
}


typedef struct AUtHashTableBucket AUtHashTableBucket;

struct AUtHashTableBucket
{
	AUtHashTableBucket *next;
};

struct AUtHashTable
{
	AUtHashTable_Hash		hashFunction;
	AUtHashTable_IsEqual	compareFunction;

	UUtUns32				bucketSize;
	UUtMemory_Pool			*memoryPool;
	UUtUns32				numBuckets;

	AUtHashTableBucket		**buckets;
};

static UUcInline void *AUrHashTable_BucketToPointer(AUtHashTableBucket *inBucket)
{
	char *ptr = (char *) inBucket;

	ptr += sizeof(AUtHashTableBucket);

	return ptr;
}

AUtHashTable*
AUrHashTable_New(
	UUtUns32						inElemSize,
	UUtUns32						inNumBuckets,
	AUtHashTable_Hash				inHashFunction,
	AUtHashTable_IsEqual			inCompareFunction)
{
	UUtUns32 tableSize;
	char *basePtr;
	AUtHashTable *hashTable;

	tableSize = sizeof(AUtHashTable) + inNumBuckets * (sizeof(AUtHashTableBucket *));
	basePtr = UUrMemory_Block_NewClear(tableSize);
	hashTable = (AUtHashTable *) basePtr;

	if (NULL != hashTable) {
		hashTable->hashFunction = inHashFunction;
		hashTable->compareFunction = inCompareFunction;
		hashTable->memoryPool = UUrMemory_Pool_New(UUmMax(1024, inElemSize), UUcPool_Growable);
		hashTable->numBuckets = inNumBuckets;
		hashTable->buckets = (AUtHashTableBucket **) (basePtr + sizeof(AUtHashTable));
		hashTable->bucketSize = inElemSize + sizeof(AUtHashTableBucket);

		if (NULL == hashTable->memoryPool) {
			UUrMemory_Block_Delete(hashTable);
			hashTable = NULL;
		}
	}

	return hashTable;
}

void
AUrHashTable_Delete(
	AUtHashTable*	inHashTable)
{
	if (inHashTable != NULL) {
		if (inHashTable->memoryPool != NULL) {
			UUrMemory_Pool_Delete(inHashTable->memoryPool);
			inHashTable->memoryPool = NULL;
		}

		UUrMemory_Block_Delete(inHashTable);
		inHashTable = NULL;
	}

	return;
}

void
AUrHashTable_Reset(
	AUtHashTable*	inHashTable)
{
	UUmAssert(!"not implemented");
}

void *
AUrHashTable_Add(
	AUtHashTable*	inHashTable,
	void			*inNewElement)
{
	UUtUns32 hash;
	AUtHashTableBucket *newBucket;
	void *current_data_pointer;

	hash = inHashTable->hashFunction(inNewElement);
	hash = hash % inHashTable->numBuckets;

	newBucket = UUrMemory_Pool_Block_New(inHashTable->memoryPool, inHashTable->bucketSize);
	if (NULL == newBucket) {
		return NULL;
	}

	current_data_pointer = AUrHashTable_BucketToPointer(newBucket);

	UUrMemory_MoveFast(inNewElement, current_data_pointer, inHashTable->bucketSize - sizeof(AUtHashTableBucket));
	newBucket->next = inHashTable->buckets[hash];
	inHashTable->buckets[hash] = newBucket;

	return current_data_pointer;
}

void*
AUrHashTable_Find(
	AUtHashTable*	inHashTable,
	void			*inElement)
{
	UUtUns32 hash;
	AUtHashTableBucket *curBucket;
	void *result = NULL;

	hash = inHashTable->hashFunction(inElement);
	hash = hash % inHashTable->numBuckets;

	for(curBucket = inHashTable->buckets[hash]; curBucket != NULL; curBucket = curBucket->next)
	{
		void *current_data_pointer = AUrHashTable_BucketToPointer(curBucket);

		if (inHashTable->compareFunction(inElement, current_data_pointer))
		{
			result = current_data_pointer;
			break;
		}
	}

	return result;
}

void
AUrQuad_FindMinMax(
	const M3tPoint3D*			inPointList,
	const M3tQuad*	inQuad,
	float				*outMinX,
	float				*outMaxX,
	float				*outMinY,
	float				*outMaxY,
	float				*outMinZ,
	float				*outMaxZ)
{
	float				quadMinX, quadMaxX, quadMinY, quadMaxY, quadMinZ, quadMaxZ;
	UUtUns32			i;
	float				curX, curY, curZ;

	quadMinX = quadMaxX = inPointList[inQuad->indices[0]].x;
	quadMinY = quadMaxY = inPointList[inQuad->indices[0]].y;
	quadMinZ = quadMaxZ = inPointList[inQuad->indices[0]].z;

	for(i = 1; i < 4; i++)
	{
		curX = inPointList[inQuad->indices[i]].x;
		curY = inPointList[inQuad->indices[i]].y;
		curZ = inPointList[inQuad->indices[i]].z;

		if(curX < quadMinX) quadMinX = curX;
		if(curY < quadMinY) quadMinY = curY;
		if(curZ < quadMinZ) quadMinZ = curZ;
		if(curX > quadMaxX) quadMaxX = curX;
		if(curY > quadMaxY) quadMaxY = curY;
		if(curZ > quadMaxZ) quadMaxZ = curZ;
	}

	*outMinX = quadMinX;
	*outMaxX = quadMaxX;
	*outMinY = quadMinY;
	*outMaxY = quadMaxY;
	*outMinZ = quadMinZ;
	*outMaxZ = quadMaxZ;

}

UUtBool
AUrQuad_QuadOverlapsQuad(
	const M3tPoint3D*		inPointList,
	const M3tQuad*		inQuad,
	const M3tQuad*		inQuadOverlapping)
{
	float	quadMinX, quadMaxX, quadMinY, quadMaxY, quadMinZ, quadMaxZ;
	float	overlapMinX, overlapMaxX, overlapMinY, overlapMaxY, overlapMinZ, overlapMaxZ;

	if(inQuad == inQuadOverlapping) return UUcTrue;

	// XXX - HACK ALERT!

	AUrQuad_FindMinMax(
		inPointList,
		inQuad,
		&quadMinX, &quadMaxX,
		&quadMinY, &quadMaxY,
		&quadMinZ, &quadMaxZ);

	AUrQuad_FindMinMax(
		inPointList,
		inQuadOverlapping,
		&overlapMinX, &overlapMaxX,
		&overlapMinY, &overlapMaxY,
		&overlapMinZ, &overlapMaxZ);

	if(
		overlapMinX > quadMaxX || overlapMaxX < quadMinX ||
		overlapMinY > quadMaxY || overlapMaxY < quadMinY ||
		overlapMinZ > quadMaxZ || overlapMaxZ < quadMinZ)
	{
		return UUcFalse;
	}

	return UUcTrue;
}

UUtBool
AUrQuad_QuadWithinQuad(
	const M3tPoint3D*			inPointList,
	const M3tQuad*			inQuad,
	const M3tQuad*			inQuadWithin)
{
	float	quadMinX, quadMaxX, quadMinY, quadMaxY, quadMinZ, quadMaxZ;
	float	solidMinX, solidMaxX, solidMinY, solidMaxY, solidMinZ, solidMaxZ;


	// XXX - HACK ALERT! eventually do a better quad within quad test

	AUrQuad_FindMinMax(
		inPointList,
		inQuad,
		&quadMinX, &quadMaxX,
		&quadMinY, &quadMaxY,
		&quadMinZ, &quadMaxZ);

	AUrQuad_FindMinMax(
		inPointList,
		inQuadWithin,
		&solidMinX, &solidMaxX,
		&solidMinY, &solidMaxY,
		&solidMinZ, &solidMaxZ);

	if(
		UUmFloat_CompareLE(solidMinX, quadMinX) && UUmFloat_CompareGE(solidMaxX, quadMaxX) &&
		UUmFloat_CompareLE(solidMinY, quadMinY) && UUmFloat_CompareGE(solidMaxY, quadMaxY) &&
		UUmFloat_CompareLE(solidMinZ, quadMinZ) && UUmFloat_CompareGE(solidMaxZ, quadMaxZ))
	{
		return UUcTrue;
	}

	return UUcFalse;
}

void
AUrQuad_ComputeCenter(
	const M3tPoint3D*	inPointList,
	const M3tQuad*	inQuad,
	float				*outCenterX,
	float				*outCenterY,
	float				*outCenterZ)
{
	float	quadMinX, quadMaxX, quadMinY, quadMaxY, quadMinZ, quadMaxZ;

	AUrQuad_FindMinMax(
		inPointList,
		inQuad,
		&quadMinX, &quadMaxX,
		&quadMinY, &quadMaxY,
		&quadMinZ, &quadMaxZ);

	*outCenterX = (quadMinX + quadMaxX) * 0.5f;
	*outCenterY = (quadMinY + quadMaxY) * 0.5f;
	*outCenterZ = (quadMinZ + quadMaxZ) * 0.5f;
}

UUtError
AUrParse_CoreGeometry(
	BFtTextFile*						inTextFile,
	AUtParseGeometry_AddPointFunc		inAddPointFunc,
	AUtParseGeometry_AddTriangleFunc	inAddTriFunc,
	AUtParseGeometry_AddQuadFunc		inAddQuadFunc,
	void*								inRefCon)
{
	UUtError	error;
	UUtUns32	numPoints;
	UUtUns32	numTris;
	UUtUns32	numQuads;
	UUtUns32	i;

	float		x, y, z, nx, ny, nz, u, v;
	UUtUns32	i0, i1, i2, i3;

	error = BFrTextFile_VerifyNextStr(inTextFile, "num points");
	UUmError_ReturnOnError(error);

	numPoints = BFrTextFile_GetUUtUns16(inTextFile);

	error = BFrTextFile_VerifyNextStr(inTextFile, "x,y,z,nx,ny,nz,u,v");
	UUmError_ReturnOnError(error);

	for(i = 0; i < numPoints; i++)
	{
		x = BFrTextFile_GetFloat(inTextFile);
		y = BFrTextFile_GetFloat(inTextFile);
		z = BFrTextFile_GetFloat(inTextFile);
		nx = BFrTextFile_GetFloat(inTextFile);
		ny = BFrTextFile_GetFloat(inTextFile);
		nz = BFrTextFile_GetFloat(inTextFile);
		u = BFrTextFile_GetFloat(inTextFile);
		v = BFrTextFile_GetFloat(inTextFile);

		error = BFrTextFile_VerifyNextStr(inTextFile, "");
		UUmError_ReturnOnError(error);

		inAddPointFunc(inRefCon, i, x, y, z, nx, ny, nz, u, v);
	}

	error = BFrTextFile_VerifyNextStr(inTextFile, "num triangles");
	UUmError_ReturnOnError(error);

	numTris = BFrTextFile_GetUUtUns16(inTextFile);

	error = BFrTextFile_VerifyNextStr(inTextFile, "index, index, index, nx, ny, nz");
	UUmError_ReturnOnError(error);

	for(i = 0; i < numTris; i++)
	{
		i0 = BFrTextFile_GetUUtUns16(inTextFile);
		i1 = BFrTextFile_GetUUtUns16(inTextFile);
		i2 = BFrTextFile_GetUUtUns16(inTextFile);
		nx = BFrTextFile_GetFloat(inTextFile);
		ny = BFrTextFile_GetFloat(inTextFile);
		nz = BFrTextFile_GetFloat(inTextFile);

		error = BFrTextFile_VerifyNextStr(inTextFile, "");
		UUmError_ReturnOnError(error);

		error = inAddTriFunc(inRefCon, i0, i1, i2, nx, ny, nz);
		UUmError_ReturnOnError(error);
	}

	error = BFrTextFile_VerifyNextStr(inTextFile, "num quads");
	UUmError_ReturnOnError(error);

	numQuads = BFrTextFile_GetUUtUns16(inTextFile);

	error = BFrTextFile_VerifyNextStr(inTextFile, "index, index, index, index, nx, ny, nz");
	UUmError_ReturnOnError(error);

	for(i = 0; i < numQuads; i++)
	{
		i0 = BFrTextFile_GetUUtUns16(inTextFile);
		i1 = BFrTextFile_GetUUtUns16(inTextFile);
		i2 = BFrTextFile_GetUUtUns16(inTextFile);
		i3 = BFrTextFile_GetUUtUns16(inTextFile);
		nx = BFrTextFile_GetFloat(inTextFile);
		ny = BFrTextFile_GetFloat(inTextFile);
		nz = BFrTextFile_GetFloat(inTextFile);

		error = BFrTextFile_VerifyNextStr(inTextFile, "");
		UUmError_ReturnOnError(error);

		error = inAddQuadFunc(inRefCon, i0, i1, i2, i3, nx, ny, nz);
		UUmError_ReturnOnError(error);
	}

	return UUcError_None;
}

UUtError
AUrParse_EnvGeometry(
	BFtTextFile*						inTextFile,
	M3tMatrix4x3*							inMatrix,
	AUtParseGeometry_AddPointFunc		inAddPointFunc,	// These can be NULL
	AUtParseGeometry_AddEnvTriangleFunc	inAddTriFunc,
	AUtParseGeometry_AddEnvQuadFunc		inAddQuadFunc,
	void*								inRefCon)
{
	UUtError	error;
	UUtUns32	numPoints;
	UUtUns32	numTris;
	UUtUns32	numQuads;
	UUtUns32	i;

	float		u, v;
	UUtUns32	i0, i1, i2, i3;
	char*		textureName;
	M3tPoint3D point;
	M3tVector3D normal;

	error = BFrTextFile_VerifyNextStr(inTextFile, "num points");
	UUmError_ReturnOnError(error);

	numPoints = BFrTextFile_GetUUtUns16(inTextFile);

	error = BFrTextFile_VerifyNextStr(inTextFile, "x,y,z,nx,ny,nz,u,v");
	UUmError_ReturnOnError(error);

	for(i = 0; i < numPoints; i++)
	{
		point.x = BFrTextFile_GetFloat(inTextFile);
		point.y = BFrTextFile_GetFloat(inTextFile);
		point.z = BFrTextFile_GetFloat(inTextFile);
		normal.x = BFrTextFile_GetFloat(inTextFile);
		normal.y = BFrTextFile_GetFloat(inTextFile);
		normal.z = BFrTextFile_GetFloat(inTextFile);
		u = BFrTextFile_GetFloat(inTextFile);
		v = BFrTextFile_GetFloat(inTextFile);

		MUrMatrix_MultiplyPoint(&point, inMatrix, &point);
		MUrMatrix_MultiplyNormal(&normal, inMatrix, &normal);

		error = BFrTextFile_VerifyNextStr(inTextFile, "");
		UUmError_ReturnOnError(error);

		if (inAddPointFunc) inAddPointFunc(inRefCon, i, point.x, point.y, point.z, normal.x, normal.y, normal.z, u, v);
	}

	error = BFrTextFile_VerifyNextStr(inTextFile, "num triangles");
	UUmError_ReturnOnError(error);

	numTris = BFrTextFile_GetUUtUns16(inTextFile);

	error = BFrTextFile_VerifyNextStr(inTextFile, "index, index, index, nx, ny, nz");
	UUmError_ReturnOnError(error);

	for(i = 0; i < numTris; i++)
	{
		i0 = BFrTextFile_GetUUtUns16(inTextFile);
		i1 = BFrTextFile_GetUUtUns16(inTextFile);
		i2 = BFrTextFile_GetUUtUns16(inTextFile);
		normal.x = BFrTextFile_GetFloat(inTextFile);
		normal.y = BFrTextFile_GetFloat(inTextFile);
		normal.z = BFrTextFile_GetFloat(inTextFile);
		textureName = BFrTextFile_GetNextStr(inTextFile);

		MUrMatrix_MultiplyNormal(&normal, inMatrix, &normal);

		error = BFrTextFile_VerifyNextStr(inTextFile, "");
		UUmError_ReturnOnError(error);

		if (inAddTriFunc)
		{
			error = inAddTriFunc(inRefCon, textureName, i0, i1, i2, normal.x, normal.y, normal.z);
			UUmError_ReturnOnError(error);
		}
	}

	error = BFrTextFile_VerifyNextStr(inTextFile, "num quads");
	UUmError_ReturnOnError(error);

	numQuads = BFrTextFile_GetUUtUns16(inTextFile);

	error = BFrTextFile_VerifyNextStr(inTextFile, "index, index, index, index, nx, ny, nz");
	UUmError_ReturnOnError(error);

	for(i = 0; i < numQuads; i++)
	{
		i0 = BFrTextFile_GetUUtUns16(inTextFile);
		i1 = BFrTextFile_GetUUtUns16(inTextFile);
		i2 = BFrTextFile_GetUUtUns16(inTextFile);
		i3 = BFrTextFile_GetUUtUns16(inTextFile);
		normal.x = BFrTextFile_GetFloat(inTextFile);
		normal.y = BFrTextFile_GetFloat(inTextFile);
		normal.z = BFrTextFile_GetFloat(inTextFile);
		textureName = BFrTextFile_GetNextStr(inTextFile);

		MUrMatrix_MultiplyNormal(&normal, inMatrix, &normal);

		error = BFrTextFile_VerifyNextStr(inTextFile, "");
		UUmError_ReturnOnError(error);

		if (inAddQuadFunc)
		{
			error = inAddQuadFunc(inRefCon, textureName, i0, i1, i2, i3, normal.x, normal.y, normal.z);
			UUmError_ReturnOnError(error);
		}
	}

	return UUcError_None;
}

UUtBool
AUrQuad_SharesAnEdge(
	const M3tQuad*	inQuadA,
	const M3tQuad*	inQuadB)
{
	UUtUns32	i, j;

	UUtUns32	indexA0, indexA1;
	UUtUns32	indexB0, indexB1;

	for(i = 0; i < 4; i++)
	{
		indexA0 = inQuadA->indices[i];
		indexA1 = inQuadA->indices[(i + 1) % 4];

		for(j = 0; j < 4; j++)
		{
			if(i == j) continue;

			indexB0 = inQuadB->indices[j];
			indexB1 = inQuadB->indices[(j + 1) % 4];

			if(((indexA0 == indexB0) && (indexA1 == indexB1)) ||
				((indexA0 == indexB1) && (indexA1 == indexB0)))
			{
				return UUcTrue;
			}
		}
	}

	return UUcFalse;
}

void AUrPlane_ClosestPoint(
	const M3tPlaneEquation*		inPlaneEqu,
	const M3tPoint3D*			inPoint,
	M3tPoint3D*					outPoint)
{
	/*

		Pcs = the point at the sphere center
		dist = distance from Pcs to the plane
		Pop = orthogonal projection of the center onto the plane J

		Jn = plane normal taken from a, b, c of plane equation
		Jd = d component of plane equation

		dist = Jn . Pcs + Jd

		Pop = Pcs - dist(Jn)

	*/

	float				distance;

	UUmAssertReadPtr(inPlaneEqu, sizeof(M3tPlaneEquation));
	UUmAssertReadPtr(inPoint, sizeof(M3tPoint3D));
	UUmAssertWritePtr(outPoint, sizeof(M3tPoint3D));

	distance = MUrSqrt(UUmSQR(inPlaneEqu->a) + UUmSQR(inPlaneEqu->b) + UUmSQR(inPlaneEqu->c));
	UUmAssert((distance >= 0.95f) && (distance <= 1.05f));

	distance =
		(inPlaneEqu->a * inPoint->x) +
		(inPlaneEqu->b * inPoint->y) +
		(inPlaneEqu->c * inPoint->z) +
		(inPlaneEqu->d);

	outPoint->x = inPoint->x - (distance * inPlaneEqu->a);
	outPoint->y = inPoint->y - (distance * inPlaneEqu->b);
	outPoint->z = inPoint->z - (distance * inPlaneEqu->c);
}

float AUrPlane_Distance_Squared(
	const M3tPlaneEquation*		inPlaneEqu,
	const M3tPoint3D*			inPoint,
	M3tPoint3D*					outPoint)
{
	M3tPoint3D pointOnPlane;
	float distance_squared;

	UUmAssertReadPtr(inPlaneEqu, sizeof(M3tPlaneEquation));
	UUmAssertReadPtr(inPoint, sizeof(M3tPoint3D));

	AUrPlane_ClosestPoint(inPlaneEqu, inPoint, &pointOnPlane);

	distance_squared = MUrPoint_Distance_Squared(inPoint, &pointOnPlane);

	if (NULL != outPoint) {
		*outPoint = pointOnPlane;
	}

	return distance_squared;
}

float AUrPlane_Distance(
	const M3tPlaneEquation*		inPlaneEqu,
	const M3tPoint3D*			inPoint,
	M3tPoint3D*					outPoint)
{
	M3tPoint3D pointOnPlane;
	float distance;

	UUmAssertReadPtr(inPlaneEqu, sizeof(M3tPlaneEquation));
	UUmAssertReadPtr(inPoint, sizeof(M3tPoint3D));

	AUrPlane_ClosestPoint(inPlaneEqu, inPoint, &pointOnPlane);

	distance = MUrPoint_Distance(inPoint, &pointOnPlane);

	if (NULL != outPoint) {
		*outPoint = pointOnPlane;
	}

	return distance;
}

#if UUmPlatform == UUmPlatform_Win32
AUtMB_ButtonChoice UUcArglist_Call AUrMessageBox(AUtMB_ButtonType inButtonType, const char *format, ...)
{
	char buffer[512];
	va_list arglist;
	int return_value;
	int type;
	AUtMB_ButtonChoice choice;

	va_start(arglist, format);
	return_value= vsprintf(buffer, format, arglist);
	va_end(arglist);

	switch(inButtonType)
	{
		case AUcMBType_AbortRetryIgnore:	type = MB_ABORTRETRYIGNORE; break;
		case AUcMBType_OK:					type = MB_OK; break;
		case AUcMBType_OKCancel:			type = MB_OKCANCEL; break;
		case AUcMBType_RetryCancel:			type = MB_RETRYCANCEL; break;
		case AUcMBType_YesNo:				type = MB_YESNO; break;
		case AUcMBType_YesNoCancel:			type = MB_YESNOCANCEL; break;
		default: UUmAssert(0);
	}

	ShowCursor(TRUE);
	return_value = MessageBox(NULL, buffer, "Message", type);
	ShowCursor(FALSE);

	switch(return_value)
		{
		case IDOK:		choice = AUcMBChoice_OK; break;
		case IDCANCEL:	choice = AUcMBChoice_Cancel; break;
		case IDABORT:	choice = AUcMBChoice_Abort; break;
		case IDRETRY:	choice = AUcMBChoice_Retry; break;
		case IDIGNORE:	choice = AUcMBChoice_Ignore; break;
		case IDYES:		choice = AUcMBChoice_Yes; break;
		case IDNO:		choice = AUcMBChoice_No; break;
		default: UUmAssert(0);
	}

	return choice;
}
#elif UUmPlatform == UUmPlatform_Mac
AUtMB_ButtonChoice UUcArglist_Call AUrMessageBox(
	AUtMB_ButtonType inButtonType,
	const char *format, ...)
{
	char buffer[256];
	va_list arglist;
	int return_value;
	AUtMB_ButtonChoice choice;

	extern void mac_hide_cursor(void);
	extern void mac_show_cursor(void);

	va_start(arglist, format);
	return_value= vsprintf(buffer, format, arglist);
	va_end(arglist);

	c2pstrcpy((StringPtr)buffer, buffer); // this can do in-place copies, so this is OK
	ParamText((unsigned char*)buffer, NULL, NULL, NULL);

	mac_show_cursor();

	switch(inButtonType)
	{
		case AUcMBType_AbortRetryIgnore:
			switch (Alert(MAC_ALERT_ABORT_RETRY_IGNORE_RES_ID, NULL))
			{
				case _button_abort: choice= AUcMBChoice_Abort; break;
				case _button_retry2: choice= AUcMBChoice_Retry; break;
				case _button_ignore: choice= AUcMBChoice_Ignore; break;
				default: UUmAssert(0);
			}
			break;
		case AUcMBType_OK:
			switch (Alert(MAC_ALERT_OK_RES_ID, NULL))
			{
				case _button_ok: choice= AUcMBChoice_OK; break;
				default: UUmAssert(0);
			}
			break;
		case AUcMBType_OKCancel:
			switch (Alert(MAC_ALERT_OK_CANCEL_RES_ID, NULL))
			{
				case _button_ok: choice= AUcMBChoice_OK; break;
				case _button_cancel2: choice= AUcMBChoice_Cancel; break;
				default: UUmAssert(0);
			}
			break;
		case AUcMBType_RetryCancel:
			switch (Alert(MAC_ALERT_RETRY_CANCEL, NULL))
			{
				case _button_retry1: choice= AUcMBChoice_Retry; break;
				case _button_cancel2: choice= AUcMBChoice_Cancel; break;
				default: UUmAssert(0);
			}
			break;
		case AUcMBType_YesNo:
			switch (Alert(MAC_ALERT_YES_NO, NULL))
			{
				case _button_yes: choice= AUcMBChoice_Yes; break;
				case _button_no: choice= AUcMBChoice_No; break;
				default: UUmAssert(0);
			}
			break;
		case AUcMBType_YesNoCancel:
			switch (Alert(MAC_ALERT_YES_NO_CANCEL, NULL))
			{
				case _button_yes: choice= AUcMBChoice_Yes; break;
				case _button_no: choice= AUcMBChoice_No; break;
				case _button_cancel3: choice= AUcMBChoice_Cancel; break;
				default: UUmAssert(0);
			}
			break;
		default:
			UUmAssert(0);
	}

	mac_hide_cursor();

	return choice;
}
//TODO: SDL_MessageBox
#else
AUtMB_ButtonChoice AUrMessageBox(AUtMB_ButtonType inButtonType, const char *format, ...)
{
	char buffer[512];
	va_list arglist;
	int return_value;

	va_start(arglist, format);
	return_value= vsprintf(buffer, format, arglist);
	va_end(arglist);

	fprintf(stderr, "%s"UUmNL, buffer);

	UUrEnterDebugger("%s", buffer);

	return AUcMBChoice_Yes;		// (Just to get rid of a warning)
}
#endif

typedef struct
{
	UUtUns32	pixel;
	double		weight;

} CONTRIB;

typedef struct
{
	UUtInt32	n;		/* number of contributors */
	CONTRIB*	p;		/* pointer to list of contributions */

} CLIST;


void AUrBuildArgumentList(char *inString, UUtUns32 inMaxCount, UUtUns32 *outCount, char **outArguments)
{
	char *stringPtr = inString;

	UUmAssertReadPtr(stringPtr, sizeof(char));
	UUmAssertWritePtr(outCount, sizeof(UUtUns32));
	UUmAssertWritePtr(outArguments, sizeof(char *) * inMaxCount);

	*outCount = 0;

	while (1)
	{
		while (UUrIsSpace(*stringPtr) || ('\0' == *stringPtr)) {
			if ('\0' == *stringPtr) {
				return;
			}

			stringPtr++;
		}

		outArguments[(*outCount)] = stringPtr;
		*outCount += 1;

		while (!UUrIsSpace(*stringPtr)) {
			if ('\0' == *stringPtr) {
				return;
			}

			stringPtr++;
		}

		if ('\0' == *stringPtr) {
			return;
		}

		*stringPtr = 0;
		stringPtr++;

		if (inMaxCount == (*outCount)) {
			return;
		}
	}
}

const char*
AUrFlags_GetTextName(
	AUtFlagElement		*inFlagList,		// Terminated by a NULL textName
	UUtUns32			inFlagValue)
{
	AUtFlagElement		*curFlagElem;
	const char			*textName;

	textName = NULL;

	for (curFlagElem = inFlagList;
		 curFlagElem->textName != NULL;
		 curFlagElem++)
	{
		if (curFlagElem->flagValue == inFlagValue)
		{
			textName = curFlagElem->textName;
			break;
		}
	}

	return textName;
}

UUtError
AUrFlags_ParseFromGroupArray(
	AUtFlagElement*		inFlagList,		// Terminated by a NULL textName
	GRtElementType		inElementType,
	void*				inElement,
	UUtUns32			*outResult)
{
	UUtUns16		curGroupElemIndex;
	GRtElementType	groupType;
	char*			elemFlagString;
	AUtFlagElement*	curFlagElem;
	UUtUns32		result = 0;

	UUmAssertReadPtr(inFlagList, sizeof(void*));
	UUmAssertReadPtr(inElement, sizeof(void*));

	if(inElementType == GRcElementType_String)
	{
		for(curFlagElem = inFlagList;
			curFlagElem->textName != NULL;
			curFlagElem++)
		{
			if(!strcmp((char*)inElement, curFlagElem->textName))
			{
				result |= curFlagElem->flagValue;
				break;
			}
		}

		if(curFlagElem->textName == NULL)
		{
			return AUcError_FlagNotFound;
		}
	}
	else if(inElementType == GRcElementType_Array)
	{
		for(curGroupElemIndex = 0;
			curGroupElemIndex < GRrGroup_Array_GetLength(inElement);
			curGroupElemIndex++)
		{
			GRrGroup_Array_GetElement(
				inElement,
				curGroupElemIndex,
				&groupType,
				&elemFlagString);

			for(curFlagElem = inFlagList;
				curFlagElem->textName != NULL;
				curFlagElem++)
			{
				if(!strcmp(elemFlagString, curFlagElem->textName))
				{
					result |= curFlagElem->flagValue;
					break;
				}
			}

			if(curFlagElem->textName == NULL)
			{
				return AUcError_FlagNotFound;
			}
		}
	}
	else
	{
		return UUcError_Generic;
	}

	*outResult = result;

	return UUcError_None;
}

UUtError
AUrFlags_ParseFromString(
	char*				inString,
	UUtBool				inIsBitField,
	AUtFlagElement*		inFlagList,		// Terminated by a NULL textName
	UUtUns32			*outResult)
{
	char*			parseState = inString;
	char*			curField;
	UUtUns32		result = 0;
	AUtFlagElement*	curFlagElem;

	*outResult = 0;

	for(;;)
	{
		curField = UUrString_Tokenize1(NULL, " ", &parseState);
		if(curField == NULL) break;

		for(curFlagElem = inFlagList;
			curFlagElem->textName != NULL;
			curFlagElem++)
		{
			if(!strcmp(curField, curFlagElem->textName))
			{
				result |= curFlagElem->flagValue;
				break;
			}
		}
		if(curFlagElem->textName == NULL)
		{
			return AUcError_FlagNotFound;
		}

		curField = UUrString_Tokenize1(NULL, " ", &parseState);
		if(curField == NULL) break;

		if(strcmp(curField, "|"))
		{
			return UUcError_Generic;
		}
	}

	*outResult = result;

	return UUcError_None;
}

void
AUrFlags_PrintFromValue(
	FILE*			inFile,
	UUtBool			inIsBitField,
	AUtFlagElement*	inFlagList,
	UUtUns32		inValue)
{
	UUtBool	first = UUcTrue;

	if(inValue == 0)
	{
		fprintf(inFile, "none");
		return;
	}

	if(inIsBitField)
	{
		while(inFlagList->textName != NULL)
		{
			if(inValue & inFlagList->flagValue)
			{
				if(first == UUcFalse)
				{
					fprintf(inFile, " | ");
				}
				fprintf(inFile, "%s", inFlagList->textName);
				first = UUcFalse;
			}

			inFlagList++;
		}
	}
	else
	{
		while(inFlagList->textName != NULL)
		{
			if(inValue == inFlagList->flagValue)
			{
				fprintf(inFile, "%s", inFlagList->textName);
				return;
			}
			inFlagList++;
		}
		fprintf(inFile, "???" "(%d)", inValue);
	}
}


#if UUmPlatform == UUmPlatform_Mac

UUtWindow AUrWindow_New(
	UUtRect *inRect)
{
	WindowPtr new_window;
	Rect window_rect;
	const RGBColor black_color= {0,0,0};
	short vert_offset= GetMBarHeight();

	window_rect.top= inRect->top + vert_offset;
	window_rect.left= inRect->left;
	window_rect.bottom= inRect->bottom + vert_offset;
	window_rect.right= inRect->right;

	new_window= NewCWindow(nil, &window_rect, "\p", false, plainDBox, (WindowPtr)-1, false, nil);
	if (new_window)
	{
		SetWindowContentColor(new_window, &black_color);
		ShowWindow(new_window);
		SetPort(GetWindowPort(new_window));
	}

	return (UUtWindow)new_window;
}

void AUrWindow_Delete(
	UUtWindow inWindow)
{
	if (inWindow)
	{
		DisposeWindow((WindowPtr)inWindow);
	}

	return;
}

void AUrWindow_Clear(
	UUtWindow	inWindow)
{
	return;
}

void AUrWindow_CopyImageInto(
	UUtWindow		inDstWindow,
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	IMtPixelType	inSrcPixelType,
	void*			inSrcData)
{
	return;
}

void AUrWindow_EORImage(
	UUtWindow	inWindow,
	UUtUns16	inMinX,
	UUtUns16	inMaxX,
	UUtUns16	inMinY,
	UUtUns16	inMaxY)
{
	return;
}

#elif UUmPlatform == UUmPlatform_Win32

#include "Oni_Platform.h"

UUtWindow AUrWindow_New(
	UUtRect *rect)
{
	HWND window= NULL;
	static WNDCLASSEX window_class= {0};
	static ATOM atom;
	static char *stefans_window_title= "BMF4LIFE!";
	static char *stefans_class_name= "Yo Mama!";

	if (window_class.cbSize == 0)
	{
		window_class.cbSize= sizeof(window_class);
		window_class.style= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		window_class.lpfnWndProc= DefWindowProc;
		window_class.cbClsExtra= 0;
		window_class.cbWndExtra= 0;
		window_class.hInstance= ONgPlatformData.appInstance;
		window_class.hIcon= NULL;
		window_class.hCursor= NULL;
		window_class.hbrBackground= (HBRUSH)GetStockObject(BLACK_BRUSH);
		window_class.lpszMenuName= NULL;
		window_class.lpszClassName= stefans_class_name;
		window_class.hIconSm= NULL;

		atom= RegisterClassEx(&window_class);
	}

	if (atom != 0)
	{
		window= CreateWindowEx(WS_EX_LEFT, stefans_class_name, stefans_window_title,
			WS_POPUP, rect->left, rect->top, (rect->right - rect->left), (rect->bottom - rect->top),
			NULL, NULL, ONgPlatformData.appInstance, NULL);
		if (window)
		{
			BOOL success;

			ShowWindow(window, SW_SHOW);
			success= BringWindowToTop(window);
			UUmAssert(success);
		}
	}

	return ((UUtWindow)window);
}

void AUrWindow_Delete(
	UUtWindow window)
{
	if (window != NULL)
	{
		BOOL destroyed= DestroyWindow((HWND)window);
		UUmAssert(destroyed);
	}

	return;
}

void AUrWindow_Clear(
	UUtWindow window)
{
	return;
}

void AUrWindow_CopyImageInto(
	UUtWindow dst_window,
	UUtUns16 src_width,
	UUtUns16 src_height,
	IMtPixelType src_pixel_type,
	void *src_data)
{
	return;
}


void AUrWindow_EORImage(
	UUtWindow window,
	UUtUns16 min_x,
	UUtUns16 max_x,
	UUtUns16 min_y,
	UUtUns16 max_y)
{
	return;
}

//TODO: SDL AUrWindow()?
#else

UUtWindow
AUrWindow_New(
	UUtRect*	inRect)
{
	return NULL;
}

void
AUrWindow_Delete(
	UUtWindow	inWindow)
{
	return;
}

void
AUrWindow_Clear(
	UUtWindow	inWindow)
{
	return;
}

void
AUrWindow_CopyImageInto(
	UUtWindow		inDstWindow,
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	IMtPixelType	inSrcPixelType,
	void*			inSrcData)
{
	return;
}


void
AUrWindow_EORImage(
	UUtWindow	inWindow,
	UUtUns16	inMinX,
	UUtUns16	inMaxX,
	UUtUns16	inMinY,
	UUtUns16	inMaxY)
{

}

#endif


UUtUns16 AUrBits16_Reverse(UUtUns16 inBits)
{
	UUtUns16 result = 0;
	int itr;

	for(itr = 0; itr < 16; itr++) {
		if (inBits & (1 << itr)) {
			result |= (1 << (15 - itr));
		}
	}

	return result;
}

UUtUns32 AUrBits32_Reverse(UUtUns32 inBits)
{
	UUtUns16 result = 0;
	int itr;

	for(itr = 0; itr < 32; itr++) {
		if (inBits & (1 << itr)) {
			result |= (1 << (31 - itr));
		}
	}

	return result;
}

UUtUns8 AUrBits16_Count(UUtUns16 inBits)
{
	UUtUns16 itr;
	UUtUns16 mask;
	UUtUns8 count = 0;

	for(itr = 0; itr < 16; itr++)
	{
		mask = 1 << itr;

		if (inBits & mask) {
			count += 1;
		}
	}

	return count;
}

UUtUns8 AUrBits32_Count(UUtUns32 inBits)
{
	UUtUns16 itr;
	UUtUns32 mask;
	UUtUns8 count = 0;

	for(itr = 0; itr < 32; itr++)
	{
		mask = 1 << itr;

		if (inBits & mask) {
			count += 1;
		}
	}

	return count;
}

UUtUns8 AUrBits64_Count(UUtUns64 inBits)
{
	UUtUns16 itr;
	UUtUns64 mask;
	UUtUns8 count = 0;

	for(itr = 0; itr < 64; itr++)
	{
		mask = 1 << itr;

		if (inBits & mask) {
			count += 1;
		}
	}

	return count;
}


UUtError AUrParamListToBitfield(const char *inFlagStr, const TMtStringArray *inMapping, UUtUns32 *outFlags)
{
#define cMaxFlags 32
	UUtError error;
	char *str = (inFlagStr != NULL) ? UUrMemory_Block_New((strlen(inFlagStr) + 1) * sizeof(char)) : NULL;
	UUtUns32 argc;
	char *argv[cMaxFlags];
	UUtUns32 itr;

	UUmAssertReadPtr(str, 1);
	UUmAssertReadPtr(inFlagStr, 1);
	UUmAssertReadPtr(inMapping, sizeof(*inMapping));
	UUmAssertWritePtr(outFlags, sizeof(*outFlags));

	if (NULL == str) {
		return UUcError_OutOfMemory;
	}

	strcpy(str, inFlagStr);
	AUrBuildArgumentList(str, cMaxFlags, &argc, argv);

	*outFlags = 0;

	for(itr = 0; itr < argc; itr++) {
		UUtUns32 thisFlag;

		error = AUrStringToSingleBit(argv[itr], inMapping, &thisFlag);

		if (UUcError_None != error) {
			AUrMessageBox(AUcMBType_OK, "%s is not a valid flag.", argv[itr]);

			thisFlag = 0;
			return UUcError_Generic;
		}

		(*outFlags) |= thisFlag;
	}

	UUrMemory_Block_Delete(str);

	return UUcError_None;
}

UUtError AUrStringToSingleBit(const char *inString, const TMtStringArray *inMapping, UUtUns32 *result)
{
	UUtError error = UUcError_None;
	UUtInt32 whichBit;

	UUmAssertWritePtr(result, sizeof(*result));
	UUmAssertReadPtr(inMapping, sizeof(*inMapping));

	whichBit = AUrLookupString(inString, inMapping);

	if (AUcNoSuchString == whichBit) {
		error = UUcError_Generic;
	}

	if (UUcError_None == error) {
		(*result) = 1 << whichBit;
	}

	return error;
}


UUtInt16 AUrLookupString(const char *inString, const TMtStringArray *inMapping)
{
	UUtError error = UUcError_None;
	UUtInt16 index;

	UUmAssert(NULL != inMapping);

	for(index = 0; index < inMapping->numStrings; index++) {
		if (0 == strcmp(inString, inMapping->string[index]->string)) {
			break;
		}
	}

	if (index >= inMapping->numStrings) {
		index = AUcNoSuchString;
	}

	return index;
}

void AUrQuad_LowestPoints(
	M3tQuad	*inQuad,
	M3tPoint3D		*inPointArray,
	M3tPoint3D		**outPointLowest,
	M3tPoint3D		**outPointNextLowest)
{
	M3tPoint3D *lowest = NULL;
	M3tPoint3D *next_lowest = NULL;
	UUtUns32 index;

	lowest = inPointArray + inQuad->indices[0];

	for(index = 1; index < 4; index++)
	{
		M3tPoint3D *lowest_test = inPointArray + inQuad->indices[index];

		if (lowest_test->y < lowest->y) {
			lowest = lowest_test;
		}

	}

	for(index = 0; index < 4; index++)
	{
		M3tPoint3D *next_lowest_test = inPointArray + inQuad->indices[index];

		if (next_lowest_test == lowest) { continue; }

		if ((NULL == next_lowest) || (next_lowest_test->y < next_lowest->y)) {
			next_lowest = next_lowest_test;
		}

	}

	if (outPointLowest != NULL) {
		UUmAssert(lowest != NULL);
		*outPointLowest = lowest;
	}

	if (outPointNextLowest) {
		UUmAssert(next_lowest != NULL);
		*outPointNextLowest = next_lowest;
	}

	return;
}

void AUrQuad_HighestPoints(
	M3tQuad *inQuad,
	M3tPoint3D *inPointArray,
	M3tPoint3D **outPointHighest,
	M3tPoint3D **outPointNextHighest)
{
	M3tPoint3D *highest = NULL;
	M3tPoint3D *next_highest = NULL;
	UUtUns32 index;

	highest = inPointArray + inQuad->indices[0];

	for(index = 1; index < 4; index++)
	{
		M3tPoint3D *highest_test = inPointArray + inQuad->indices[index];

		if (highest_test->y > highest->y) {
			highest = highest_test;
		}

	}

	for(index = 0; index < 4; index++)
	{
		M3tPoint3D *next_highest_test = inPointArray + inQuad->indices[index];

		if (next_highest_test == highest) { continue; }

		if ((NULL == next_highest) || (next_highest_test->y > next_highest->y)) {
			next_highest = next_highest_test;
		}

	}

	if (outPointHighest != NULL) {
		UUmAssert(highest != NULL);
		*outPointHighest = highest;
	}

	if (outPointNextHighest) {
		UUmAssert(next_highest != NULL);
		*outPointNextHighest = next_highest;
	}

	return;
}


void AUrNumToCommaString(UUtInt32 inNumber, char *outString)
{
	char *src;
	char *dst;
	char tempBuffer[32];
	int count;

	sprintf(tempBuffer, "%d", inNumber);
	count = strlen(tempBuffer);

	src = tempBuffer;
	dst = outString;

	// handle negative numbers
	if (inNumber < 0) {
		*dst++ = *src++;
		count -= 1;
	}

	switch(count)
	{
		default:
			UUmAssert(!"invalid case");
		break;

		case 12:
			*dst++ = *src++;
		case 11:
			*dst++ = *src++;
		case 10:
			*dst++ = *src++;
			*dst++ = ',';
		case 9:
			*dst++ = *src++;
		case 8:
			*dst++ = *src++;
		case 7:
			*dst++ = *src++;
			*dst++ = ',';
		case 6:
			*dst++ = *src++;
		case 5:
			*dst++ = *src++;
		case 4:
			*dst++ = *src++;
			*dst++ = ',';
		case 3:
			*dst++ = *src++;
		case 2:
			*dst++ = *src++;
		case 1:
			*dst++ = *src++;
		case 0:
			*dst++ = *src++;
	}

	return;
}

AUtDict *AUrDict_New(UUtUns32 inMaxPages, UUtUns32 inRange)
{
	UUtUns32 size = 0;
	AUtDict *newDict;

	if (0 == inMaxPages) {
		newDict = NULL;
	}
	else {
		UUtUns32 size_without_bitvector;
		UUtUns32 size;

		size_without_bitvector = 0;
		size_without_bitvector += sizeof(AUtDict);
		size_without_bitvector += (inMaxPages - 1) * sizeof(UUtUns32);

		// S.S. size = size_without_bitvector + ((inRange + 31) / 32) * sizeof(UUtUns32);
		size= size_without_bitvector + ((inRange + 31) >> 5) * sizeof(UUtUns32);

		newDict = (AUtDict *) UUrMemory_Block_NewClear(size);

		if (NULL != newDict) {
			UUtUns8 *internal_bit_vector = (UUtUns8 *) newDict;

			newDict->vector = (UUtUns32 *) (internal_bit_vector + size_without_bitvector);
			newDict->maxPages = inMaxPages;
			newDict->numPages = 0;
		}
	}

	return newDict;
}

void AUrDict_Clear(AUtDict *inDict)
{
	UUtUns32 itr;

	UUmAssertWritePtr(inDict, sizeof(*inDict));

	for(itr = 0; itr < inDict->numPages; itr++)
	{
		UUrBitVector_ClearBit(inDict->vector, inDict->pages[itr]);
	}

	inDict->numPages = 0;

	return;
}

void AUrDict_Dispose(AUtDict *inDict)
{
	UUmAssertWritePtr(inDict, sizeof(*inDict));

	UUrMemory_Block_Delete(inDict);
}

#if UUmCompiler	== UUmCompiler_VisC

#if 1
__declspec( naked ) UUtBool __fastcall AUrDict_TestAndAdd(AUtDict *inDict, UUtUns32 key)
{
	__asm
	{
		// ecx = inDict
		// edx = key

		push        esi
		mov         esi,ecx

		// ecx = inDict
		// esi = inDict
		// edx = key

		mov         ecx, [esi]inDict.vector

		// ecx = inDict.vector
		// esi = inDict
		// edx = key

		mov			eax, 1
		bts			[ecx], edx

		jc			Dict_exit

		// if not in the dictionary
		mov         ecx, [esi]inDict.numPages
		mov			eax, [esi]inDict.maxPages

		// eax = inDict.maxPages
		// ecx = inDict.numPages
		// esi = inDict
		// edx = key

		cmp			ecx, eax
		jge			Dict_not_enough_pages		// if ecx (numpages) >= eax (maxpages) goto Dict_not_enough_pages

		mov         [esi]inDict.pages[ecx*4], edx
		// inc         [esi]inDict.numPages
		inc			ecx
		mov			[esi]inDict.numPages, ecx

		Dict_not_enough_pages:

		xor			eax, eax

		Dict_exit:

		pop         esi

		ret
	}
}
#else
UUtBool __fastcall AUrDict_TestAndAdd(AUtDict *inDict, UUtUns32 key)
{
	UUtBool inDictionary;

	UUmAssertWritePtr(inDict, sizeof(*inDict));

	inDictionary = UUrBitVector_TestAndSetBit(inDict->vector, key);

	if (!inDictionary)
	{
		if (inDict->numPages >= inDict->maxPages) {
			UUmAssert(!"if we got here then we have set a bit we will not clear which is bad");
			return inDictionary;
		}

		inDict->pages[inDict->numPages] = key;
		inDict->numPages++;
	}

	return inDictionary;
}
#endif

#else
UUtBool AUrDict_TestAndAdd(AUtDict *inDict, UUtUns32 key)
{
	UUtBool inDictionary;

	UUmAssertWritePtr(inDict, sizeof(*inDict));


	inDictionary = UUrBitVector_TestAndSetBit(inDict->vector, key);

	if (!inDictionary)
	{
		if(inDict->numPages >= inDict->maxPages) return inDictionary;

		inDict->pages[inDict->numPages] = key;
		inDict->numPages++;
	}

	return inDictionary;
}
#endif

/* prototypes for local routines */


#define swap_16(a,b) { UUtUns16 temp; temp = *((UUtUns16 *)(a)); *((UUtUns16 *)(a)) = *((UUtUns16 *)(b)); *((UUtUns16 *)(b)) = temp; }

/* prototypes for local routines */
#define comp_16_macro(a,b) inComp(*((UUtUns16 *)(a)), *((UUtUns16 *)(b)))

static void shortsort_16 (
    char *lo,
    char *hi,
 	AUtCompare_Function_16 inComp);

/* this parameter defines the cutoff between using quick sort and
   insertion sort for arrays; arrays with lengths shorter or equal to the
   below value use insertion sort */

#define CUTOFF 8            /* testing shows that this is good value */


/* sort the array between lo and hi (inclusive) */

void AUrQSort_16 (
    void *base,
    unsigned num,
	AUtCompare_Function_16 inComp)
{
    char *lo, *hi;              /* ends of sub-array currently sorting */
    char *mid;                  /* points to middle of subarray */
    char *loguy, *higuy;        /* traveling pointers for partition step */
    unsigned size;              /* size of the sub-array */
    char *lostk[30], *histk[30];
    int stkptr;                 /* stack for saving sub-array to be processed */

    /* Note: the number of stack entries required is no more than
       1 + log2(size), so 30 is sufficient for any array */

    if (num < 2)
        return;                 /* nothing to do */

    stkptr = 0;                 /* initialize stack */

    lo = base;
    hi = (char *)base + 2 * (num-1);        /* initialize limits */

    /* this entry point is for pseudo-recursion calling: setting
       lo and hi and jumping to here is like recursion, but stkptr is
       prserved, locals aren't, so we preserve stuff on the stack */
recurse:

    // S.S. size = (hi - lo) / 2 + 1;        /* number of el's to sort */
	size = ((hi - lo) >> 1) + 1;

    /* below a certain size, it is faster to use a O(n^2) sorting method */
    if (size <= CUTOFF) {
         shortsort_16(lo, hi, inComp);
    }
    else {
        /* First we pick a partititioning element.  The efficiency of the
           algorithm demands that we find one that is approximately the
           median of the values, but also that we select one fast.  Using
           the first one produces bad performace if the array is already
           sorted, so we use the middle one, which would require a very
           wierdly arranged array for worst case performance.  Testing shows
           that a median-of-three algorithm does not, in general, increase
           performance. */

        // S.S. mid = lo + (size / 2) * 2;      /* find middle element */
		mid= lo + (size >> 1) * 2;
        swap_16(mid, lo);               /* swap it to beginning of array */

        /* We now wish to partition the array into three pieces, one
           consisiting of elements <= partition element, one of elements
           equal to the parition element, and one of element >= to it.  This
           is done below; comments indicate conditions established at every
           step. */

        loguy = lo;
        higuy = hi + 2;

        /* Note that higuy decreases and loguy increases on every iteration,
           so loop must terminate. */
        for (;;) {
            /* lo <= loguy < hi, lo < higuy <= hi + 1,
               A[i] <= A[lo] for lo <= i <= loguy,
               A[i] >= A[lo] for higuy <= i <= hi */

            do  {
                loguy += 2;
            } while (loguy <= hi && !comp_16_macro(loguy, lo));

            /* lo < loguy <= hi+1, A[i] <= A[lo] for lo <= i < loguy,
               either loguy > hi or A[loguy] > A[lo] */

            do  {
                higuy -= 2;
            } while (higuy > lo && comp_16_macro(higuy, lo));

            /* lo-1 <= higuy <= hi, A[i] >= A[lo] for higuy < i <= hi,
               either higuy <= lo or A[higuy] < A[lo] */

            if (higuy < loguy)
                break;

            /* if loguy > hi or higuy <= lo, then we would have exited, so
               A[loguy] > A[lo], A[higuy] < A[lo],
               loguy < hi, highy > lo */

            swap_16(loguy, higuy);

            /* A[loguy] < A[lo], A[higuy] > A[lo]; so condition at top
               of loop is re-established */
        }

        /*     A[i] >= A[lo] for higuy < i <= hi,
               A[i] <= A[lo] for lo <= i < loguy,
               higuy < loguy, lo <= higuy <= hi
           implying:
               A[i] >= A[lo] for loguy <= i <= hi,
               A[i] <= A[lo] for lo <= i <= higuy,
               A[i] = A[lo] for higuy < i < loguy */

        swap_16(lo, higuy);     /* put partition element in place */

        /* OK, now we have the following:
              A[i] >= A[higuy] for loguy <= i <= hi,
              A[i] <= A[higuy] for lo <= i < higuy
              A[i] = A[lo] for higuy <= i < loguy    */

        /* We've finished the partition, now we want to sort the subarrays
           [lo, higuy-1] and [loguy, hi].
           We do the smaller one first to minimize stack usage.
           We only sort arrays of length 2 or more.*/

        if ( higuy - 1 - lo >= hi - loguy ) {
            if (lo + 2 < higuy) {
                lostk[stkptr] = lo;
                histk[stkptr] = higuy - 2;
                ++stkptr;
            }                           /* save big recursion for later */

            if (loguy < hi) {
                lo = loguy;
                goto recurse;           /* do small recursion */
            }
        }
        else {
            if (loguy < hi) {
                lostk[stkptr] = loguy;
                histk[stkptr] = hi;
                ++stkptr;               /* save big recursion for later */
            }

            if (lo + 2 < higuy) {
                hi = higuy - 2;
                goto recurse;           /* do small recursion */
            }
        }
    }

    /* We have sorted the array, except for any pending sorts on the stack.
       Check if there are any, and do them. */

    --stkptr;
    if (stkptr >= 0) {
        lo = lostk[stkptr];
        hi = histk[stkptr];
        goto recurse;           /* pop subarray from stack */
    }
    else
        return;                 /* all subarrays done */
}


/***
*shortsort(hi, lo, width, comp) - insertion sort for sorting short arrays
*
*Purpose:
*       sorts the sub-array of elements between lo and hi (inclusive)
*       side effects:  sorts in place
*       assumes that lo < hi
*
*Entry:
*       char *lo = pointer to low element to sort
*       char *hi = pointer to high element to sort
*       unsigned width = width in bytes of each array element
*       int (*comp)() = pointer to function returning analog of strcmp for
*               strings, but supplied by user for comparing the array elements.
*               it accepts 2 pointers to elements and returns neg if 1<2, 0 if
*               1=2, pos if 1>2.
*
*Exit:
*       returns void
*
*Exceptions:
*
*******************************************************************************/

static void shortsort_16 (
    char *lo,
    char *hi,
 	AUtCompare_Function_16 inComp)
{
    char *p, *max;

    /* Note: in assertions below, i and j are alway inside original bound of
       array to sort. */

    while (hi > lo) {
        /* A[i] <= A[j] for i <= j, j > hi */
        max = lo;
        for (p = lo+2; p <= hi; p += 2) {
            /* A[i] <= A[max] for lo <= i < p */
            if (comp_16_macro(p, max)) {
                max = p;
            }
            /* A[i] <= A[max] for lo <= i <= p */
        }

        /* A[i] <= A[max] for lo <= i <= hi */

        swap_16(max, hi);

        /* A[i] <= A[hi] for i <= hi, so A[i] <= A[j] for i <= j, j >= hi */

        hi -= 2;

        /* A[i] <= A[j] for i <= j, j > hi, loop top condition established */
    }
    /* A[i] <= A[j] for i <= j, j > lo, which implies A[i] <= A[j] for i < j,
       so array is sorted */
}

#define swap_32(a,b) { UUtUns32 temp; temp = *((UUtUns32 *)(a)); *((UUtUns32 *)(a)) = *((UUtUns32 *)(b)); *((UUtUns32 *)(b)) = temp; }

/* prototypes for local routines */
#define comp_32_macro(a,b) inComp(*((UUtUns32 *)(a)), *((UUtUns32 *)(b)))

static void shortsort_32 (
    char *lo,
    char *hi,
 	AUtCompare_Function_32 inComp);

/* sort the array between lo and hi (inclusive) */

void AUrQSort_32 (
    void *base,
    unsigned num,
	AUtCompare_Function_32 inComp)
{
    char *lo, *hi;              /* ends of sub-array currently sorting */
    char *mid;                  /* points to middle of subarray */
    char *loguy, *higuy;        /* traveling pointers for partition step */
    unsigned size;              /* size of the sub-array */
    char *lostk[30], *histk[30];
    int stkptr;                 /* stack for saving sub-array to be processed */

    /* Note: the number of stack entries required is no more than
       1 + log2(size), so 30 is sufficient for any array */

    if (num < 2 || 4 == 0)
        return;                 /* nothing to do */

    stkptr = 0;                 /* initialize stack */

    lo = base;
    hi = (char *)base + 4 * (num-1);        /* initialize limits */

    /* this entry point is for pseudo-recursion calling: setting
       lo and hi and jumping to here is like recursion, but stkptr is
       prserved, locals aren't, so we preserve stuff on the stack */
recurse:

    // S.S. size = (hi - lo) / 4 + 1;        /* number of el's to sort */
	size= ((hi - lo) >> 2) + 1;

    /* below a certain size, it is faster to use a O(n^2) sorting method */
    if (size <= CUTOFF) {
         shortsort_32(lo, hi, inComp);
    }
    else {
        /* First we pick a partititioning element.  The efficiency of the
           algorithm demands that we find one that is approximately the
           median of the values, but also that we select one fast.  Using
           the first one produces bad performace if the array is already
           sorted, so we use the middle one, which would require a very
           wierdly arranged array for worst case performance.  Testing shows
           that a median-of-three algorithm does not, in general, increase
           performance. */

        // S.S. mid = lo + (size / 2) * 4;      /* find middle element */
        mid = lo + (size >> 1) * 4;
		swap_32(mid, lo);               /* swap_32 it to beginning of array */

        /* We now wish to partition the array into three pieces, one
           consisiting of elements <= partition element, one of elements
           equal to the parition element, and one of element >= to it.  This
           is done below; comments indicate conditions established at every
           step. */

        loguy = lo;
        higuy = hi + 4;

        /* Note that higuy decreases and loguy increases on every iteration,
           so loop must terminate. */
        for (;;) {
            /* lo <= loguy < hi, lo < higuy <= hi + 1,
               A[i] <= A[lo] for lo <= i <= loguy,
               A[i] >= A[lo] for higuy <= i <= hi */

            do  {
                loguy += 4;
            } while (loguy <= hi && !comp_32_macro(loguy, lo));

            /* lo < loguy <= hi+1, A[i] <= A[lo] for lo <= i < loguy,
               either loguy > hi or A[loguy] > A[lo] */

            do  {
                higuy -= 4;
            } while (higuy > lo && comp_32_macro(higuy, lo));

            /* lo-1 <= higuy <= hi, A[i] >= A[lo] for higuy < i <= hi,
               either higuy <= lo or A[higuy] < A[lo] */

            if (higuy < loguy)
                break;

            /* if loguy > hi or higuy <= lo, then we would have exited, so
               A[loguy] > A[lo], A[higuy] < A[lo],
               loguy < hi, highy > lo */

            swap_32(loguy, higuy);

            /* A[loguy] < A[lo], A[higuy] > A[lo]; so condition at top
               of loop is re-established */
        }

        /*     A[i] >= A[lo] for higuy < i <= hi,
               A[i] <= A[lo] for lo <= i < loguy,
               higuy < loguy, lo <= higuy <= hi
           implying:
               A[i] >= A[lo] for loguy <= i <= hi,
               A[i] <= A[lo] for lo <= i <= higuy,
               A[i] = A[lo] for higuy < i < loguy */

        swap_32(lo, higuy);     /* put partition element in place */

        /* OK, now we have the following:
              A[i] >= A[higuy] for loguy <= i <= hi,
              A[i] <= A[higuy] for lo <= i < higuy
              A[i] = A[lo] for higuy <= i < loguy    */

        /* We've finished the partition, now we want to sort the subarrays
           [lo, higuy-1] and [loguy, hi].
           We do the smaller one first to minimize stack usage.
           We only sort arrays of length 2 or more.*/

        if ( higuy - 1 - lo >= hi - loguy ) {
            if (lo + 4 < higuy) {
                lostk[stkptr] = lo;
                histk[stkptr] = higuy - 4;
                ++stkptr;
            }                           /* save big recursion for later */

            if (loguy < hi) {
                lo = loguy;
                goto recurse;           /* do small recursion */
            }
        }
        else {
            if (loguy < hi) {
                lostk[stkptr] = loguy;
                histk[stkptr] = hi;
                ++stkptr;               /* save big recursion for later */
            }

            if (lo + 4 < higuy) {
                hi = higuy - 4;
                goto recurse;           /* do small recursion */
            }
        }
    }

    /* We have sorted the array, except for any pending sorts on the stack.
       Check if there are any, and do them. */

    --stkptr;
    if (stkptr >= 0) {
        lo = lostk[stkptr];
        hi = histk[stkptr];
        goto recurse;           /* pop subarray from stack */
    }
    else
        return;                 /* all subarrays done */
}

static void shortsort_32 (
    char *lo,
    char *hi,
 	AUtCompare_Function_32 inComp)
{
    char *p, *max;

    /* Note: in assertions below, i and j are alway inside original bound of
       array to sort. */

    while (hi > lo) {
        /* A[i] <= A[j] for i <= j, j > hi */
        max = lo;
        for (p = lo+4; p <= hi; p += 4) {
            /* A[i] <= A[max] for lo <= i < p */
            if (comp_32_macro(p, max)) {
                max = p;
            }
            /* A[i] <= A[max] for lo <= i <= p */
        }

        /* A[i] <= A[max] for lo <= i <= hi */

        swap_32(max, hi);

        /* A[i] <= A[hi] for i <= hi, so A[i] <= A[j] for i <= j, j >= hi */

        hi -= 4;

        /* A[i] <= A[j] for i <= j, j > hi, loop top condition established */
    }
    /* A[i] <= A[j] for i <= j, j > lo, which implies A[i] <= A[j] for i < j,
       so array is sorted */
}

UUtUns16 AUrSqueeze_16(
	UUtUns16 *inBase,
	UUtUns32 inNumElements)
{
	UUtUns16 count;
	UUtUns16 *from, *to;

	for(from = to = inBase; from < (inBase + inNumElements - 1); from++,to++)
	{
		while((from < (inBase + inNumElements - 1)) && (from[0] == from[1]))
		{
			from++;
		}

		*to = *from;
	}

	*to = *from;

	count = from - to;

	return count;
}

UUtError AUrParseNumericalRangeString(
	char*		inArg,
	UUtUns32*	ioBitVector,
	UUtUns32	inBitVectorLen)
{
	// Parse strings of the format [x,y]|[w-z], and returns a bit vector with
	// those bits set.
	// Written by Brent, generalized by Quinn, so don't blame me.

	char		c;
	UUtUns16	numBufferIndex;
	char		numBuffer[10];
	UUtUns32	num, numStart = 0xFFFF;
	UUtBool		inRange = 0;

	numBufferIndex = 0;
	UUmAssert(ioBitVector);

	while(1)
	{
		c = *inArg++;

		if(isdigit(c))
		{
			numBuffer[numBufferIndex++] = c;
		}
		else if(c == ',' || c == 0)
		{
			if(numBufferIndex == 0) goto error;
			numBuffer[numBufferIndex] = 0;
			sscanf(numBuffer, "%d", &num);
			if(num >= inBitVectorLen) goto error;
			if(inRange)
			{
				UUmAssert(numStart < inBitVectorLen);
				UUrBitVector_SetBitRange(ioBitVector, numStart, num);
			}
			else
			{
				UUrBitVector_SetBit(ioBitVector, num);
			}

			if(c == 0) return UUcError_None;
			numBufferIndex = 0;
			inRange = UUcFalse;
		}
		else if(c == '-')
		{
			if(inRange) goto error;
			if(numBufferIndex == 0) goto error;
			inRange = UUcTrue;
			numBuffer[numBufferIndex] = 0;
			sscanf(numBuffer, "%d", &numStart);
			if(numStart >= inBitVectorLen) goto error;
			numBufferIndex = 0;
		}
		else
		{
			goto error;
		}
	}

	return UUcError_None;

error:
	return UUcError_Generic;
}

void AUrDeviateXZ(
	M3tPoint3D *ioPoint,
	float inAmt)
{
	/*********
	* Randomly deviate a point in the XZ plane
	*/

	float x = UUmRandomRangeFloat(0.0f,inAmt);
	float z = UUmRandomRangeFloat(0.0f,inAmt);

	UUmAssert(ioPoint);

	if (UUmRandomRange(0,100) < 50) x = -x;
	if (UUmRandomRange(0,100) < 50) z = -z;

	ioPoint->x += x;
	ioPoint->z += z;
}

