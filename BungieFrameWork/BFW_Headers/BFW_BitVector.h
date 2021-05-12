#pragma once
/*
 *
 * BFW_BitVector.h
 *
 * from cseries: BIT_VECTOR.H
 *
 */

#ifndef BFW_BITVECTOR_H
#define BFW_BITVECTOR_H

#include "BFW.h"

#ifdef __MOTO__
	// Motorola's PowerPC compiler has a unique name for this intrinsic
	#define i_cntlzw(value) (__builtin_cntlzw(value))
#elif defined(__POWERPC__)
	// Metrowerks and MrC both share this name
	#define i_cntlzw(value) (__cntlzw(value))
#endif

#define cTopBit				((UUtUns32) 0x80000000)
#define	cAllOnes			((UUtUns32) 0xFFFFFFFF)
#define	cAllZeros			((UUtUns32) 0x00000000)
#define	cBitShiftLong		5
#define	cRemainderMaskLong 31


// BV_INDEX: tells you for a bit which long it is
#define BV_INDEX(b) (b >> cBitShiftLong)

// BV_NUM: tells you for a bit the number of bits of remainder
#define BV_NUM(b) (b & cRemainderMaskLong)

// BV_MASK: for a bit gives you a mask of the trailing part inclusive
#define BV_MASK(b) (gEndMaskTable[BV_NUM(b)])

// BV_BIT: for a bit gives you a value to test that bit using and
#define BV_BIT(b) (((unsigned)1) << BV_NUM(b))

// BV_START_MASK: for a bit produces a mask of the starting part inclusive
#define BV_START_MASK(b) (gStartMaskTable[BV_NUM(b)])

#define UUcBitVector_None -1
#define UUmBitVector_Sizeof(size) (((size + UUcUns32Bits-1) / UUcUns32Bits) * sizeof(UUtUns32))


#define UUmBitVector_Loop_Begin(itr, count, ptr)	\
{	\
	const UUtUns32 *bit_vector_internal_cur_pointer = ptr;	\
	\
	for(itr = 0; itr < count; bit_vector_internal_cur_pointer++)	\
	{	\
		UUtUns32 bit_vector_internal_next_end = UUmMin(itr + 32, count);	\
		UUtUns32 bit_vector_internal_cur_block = *bit_vector_internal_cur_pointer;	\
		UUtUns32 bit_vector_internal_current_bit = 1;	\
	\
		for(bit_vector_internal_current_bit = 1; itr < bit_vector_internal_next_end; bit_vector_internal_current_bit = bit_vector_internal_current_bit << 1, itr++)	\
		{	\

#define UUmBitVector_Loop_End	\
		}	\
	}	\
}

#define UUmBitVector_Loop_Test	\
			if (bit_vector_internal_cur_block & bit_vector_internal_current_bit)


// iterates using i over the set bits in v from bit 0 to bit l inclusive (i must be a signed type)
#define UUmBitVector_ForEachBitSetDo(i, v, l)	for (i= UUrBitVector_FindFirstSetRange((v), 0, (l));	\
												((i <= (l)) && (i != UUcBitVector_None));			\
												i= UUrBitVector_FindFirstSetRange((v), i+1, (l)))

// end is inclusive for all range functions
typedef void (*UUtBitVector_BitRangeFunc)(UUtUns32 *vector, UUtUns32 start, UUtUns32 end);

UUtUns32 *UUrBitVector_New(UUtUns32 size);
void UUrBitVector_Dispose(UUtUns32 *vector);

#if UUmCompiler	== UUmCompiler_VisC
UUtBool __fastcall UUrBitVector_TestBit(const UUtUns32 *vector, UUtUns32 bit);
UUtBool __fastcall UUrBitVector_TestAndSetBit(UUtUns32 *vector, UUtUns32 bit);
UUtBool __fastcall UUrBitVector_TestAndClearBit(UUtUns32 *vector, UUtUns32 bit);
void __fastcall UUrBitVector_SetBit(UUtUns32 *vector, UUtUns32 bit);
void __fastcall UUrBitVector_ClearBit(UUtUns32 *vector, UUtUns32 bit);
void __fastcall UUrBitVector_ToggleBit(UUtUns32 *vector, UUtUns32 bit);
UUtUns32 __fastcall UUrBitVector_FindFirstSet(UUtInt32 value);
#else
UUtUns32 UUrBitVector_FindFirstSet(UUtInt32 r3);

#endif
UUtUns32 UUrBitVector_FindFirstClear(UUtInt32 r3);


UUtBool UUrBitVector_TestBitRange(const UUtUns32 *vector, UUtUns32 start, UUtUns32 end);	// inclusive
void UUrBitVector_SetBitRange(UUtUns32 *vector, UUtUns32 start, UUtUns32 end);	// inclusive
void UUrBitVector_ClearBitRange(UUtUns32 *vector, UUtUns32 start, UUtUns32 end);	// inclusive
void UUrBitVector_ToggleBitRange(UUtUns32 *vector, UUtUns32 start, UUtUns32 end);	// inclusive

void UUrBitVector_SetBitAll(UUtUns32 *vector, UUtUns32 size);
void UUrBitVector_ClearBitAll(UUtUns32 *vector, UUtUns32 size);

UUtInt32 UUrBitVector_FindFirstSetRange(const UUtUns32 *vector, UUtInt32 start, UUtInt32 end);	// inclusive
UUtInt32 UUrBitVector_FindFirstClearRange(const UUtUns32 *vector, UUtInt32 start, UUtInt32 end);	// inclusive

void UUrBitVectors_Xor(const UUtUns32 *vector1, UUtUns32 *vector2, UUtUns32 size);
void UUrBitVectors_Or(const UUtUns32 *vector1, UUtUns32 *vector2, UUtUns32 size);
void UUrBitVectors_And(const UUtUns32 *vector1, UUtUns32 *vector2, UUtUns32 size);
void UUrBitVectors_Andc(const UUtUns32 *vector1, const UUtUns32 *vector2, UUtUns32 *vectorDest, UUtUns32 size);
void UUrBitVectors_Xnor(const UUtUns32 *vector1, UUtUns32 *vector2, UUtUns32 size);
void UUrBitVectors_Nor(const UUtUns32 *vector1, UUtUns32 *vector2, UUtUns32 size);
void UUrBitVectors_Nand(const UUtUns32 *vector1, UUtUns32 *vector2, UUtUns32 size);
void UUrBitVector_Copy(const UUtUns32 *vector1, UUtUns32 *vector2, UUtUns32 size);

UUtBool UUrBitVector_Expand16Bit(
	const UUtUns32*	vector,
	UUtUns16*		dst,
	UUtUns16		inBitSetColor,
	UUtUns16		inBitClearColor,
	UUtUns32 		start,
	UUtUns32 		end);



#if UUmCompiler	!= UUmCompiler_VisC

static UUcInline UUtBool
UUrBitVector_TestBit(const UUtUns32 *bitVector, UUtUns32 bit)
{
	UUtBool result = (bitVector[BV_INDEX(bit)] & BV_BIT(bit)) != 0;
	UUmAssert(bitVector);

	return result;
}

static UUcInline UUtBool
UUrBitVector_TestAndSetBit(UUtUns32 *bitVector, UUtUns32 bit)
{
	UUtUns32*	elem;
	UUtUns32	bvBit;
	UUtBool		result;
	
	UUmAssert(bitVector);
	
	elem = bitVector + BV_INDEX(bit);
	bvBit = BV_BIT(bit);
	
	result = (*elem & bvBit) != 0;
	*elem |= bvBit;

	return result;
}

static UUcInline UUtBool 
UUrBitVector_TestAndClearBit(UUtUns32 *bitVector, UUtUns32 bit)
{
	UUtUns32*	elem;
	UUtUns32	bvBit;
	UUtBool		result;
	
	UUmAssert(bitVector);
	
	elem = bitVector + BV_INDEX(bit);
	bvBit = BV_BIT(bit);
	
	result = (*elem & bvBit) != 0;
	*elem &= ~bvBit;

	return result;
}

static UUcInline void
UUrBitVector_SetBit(UUtUns32 *bitVector, UUtUns32 bit)
{
	UUmAssert(bitVector);
	bitVector[BV_INDEX(bit)] |= BV_BIT(bit);
}

static UUcInline void
UUrBitVector_ClearBit(UUtUns32 *bitVector, UUtUns32 bit)
{
	UUmAssert(bitVector);
	bitVector[BV_INDEX(bit)] &= ~BV_BIT(bit);
}

static UUcInline void
UUrBitVector_ToggleBit(UUtUns32 *bitVector, UUtUns32 bit)
{
	UUmAssert(bitVector);
	bitVector[BV_INDEX(bit)] ^= BV_BIT(bit);
}
#endif // UUmCompiler	!= UUmCompiler_VisC

UUtUns32 *UUr2BitVector_Allocate(UUtUns32 inSize);
void UUr2BitVector_Dispose(UUtUns32 *in2BitVector);
void UUr2BitVector_Clear(UUtUns32 *in2BitVector, UUtUns32 inSize);
void UUr2BitVector_Decrement(UUtUns32 *in2BitVector, UUtUns32 inSize);

static UUcInline void UUr2BitVector_Set(UUtUns32 *in2BitVector, UUtUns32 inBit)
{
	UUtUns8*	p = (UUtUns8 *)in2BitVector + (inBit >> 2);

	*p |= 0x3 << ((inBit & 0x3) << 1);

	return;
}

static UUcInline UUtUns8 UUr2BitVector_Test(UUtUns32 *in2BitVector, UUtUns32 inBit)
{
	UUtUns8	*vector_p = ((UUtUns8 *) in2BitVector) + (inBit >> 2);
	UUtUns8 result;

	result = *vector_p & (0x3 << ((inBit & 0x3) << 1));

	return result;
}

#endif /* BFW_BITVECTOR_H */
