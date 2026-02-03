/*
 * BFW_BitVector.c
 *
 * from cseries: BIT_VECTOR.C
 */

#include "BFW.h"
#include "BFW_BitVector.h"


static UUtUns32 gEndMaskTable[32] =
{
	0x00000001,
	0x00000003,
	0x00000007,
	0x0000000f,

	0x0000001f,
	0x0000003f,
	0x0000007f,
	0x000000ff,

	0x000001ff,
	0x000003ff,
	0x000007ff,
	0x00000fff,

	0x00001fff,
	0x00003fff,
	0x00007fff,
	0x0000ffff,

	0x0001ffff,
	0x0003ffff,
	0x0007ffff,
	0x000fffff,

	0x001fffff,
	0x003fffff,
	0x007fffff,
	0x00ffffff,

	0x01ffffff,
	0x03ffffff,
	0x07ffffff,
	0x0fffffff,

	0x1fffffff,
	0x3fffffff,
	0x7fffffff,
	0xffffffff
};

static UUtUns32 gStartMaskTable[32] =
{
	0xffffffff,
	0xfffffffe,
	0xfffffffc,
	0xfffffff8,

	0xfffffff0,
	0xffffffe0,
	0xffffffc0,
	0xffffff80,

	0xffffff00,
	0xfffffe00,
	0xfffffc00,
	0xfffff800,

	0xfffff000,
	0xffffe000,
	0xffffc000,
	0xffff8000,

	0xffff0000,
	0xfffe0000,
	0xfffc0000,
	0xfff80000,

	0xfff00000,
	0xffe00000,
	0xffc00000,
	0xff800000,

	0xff000000,
	0xfe000000,
	0xfc000000,
	0xf8000000,

	0xf0000000,
	0xe0000000,
	0xc0000000,
	0x80000000
};


UUtUns32 *UUrBitVector_New(UUtUns32 size)
{
	UUtUns32 *result;
	UUtUns32 temp= UUmBitVector_Sizeof(size);

	result= UUrMemory_Block_NewClear(temp);

	return result;
}

void UUrBitVector_Dispose(UUtUns32 *bitVector)
{
	UUmAssert(bitVector);
	UUrMemory_Block_Delete(bitVector);
}

#if UUmCompiler	== UUmCompiler_VisC
UUtBool __fastcall UUrBitVector_TestBit(const UUtUns32 *bitVector, UUtUns32 bit)
{
	__asm
	{
		bt [ecx], edx
		setc al
	}
}

UUtBool __fastcall UUrBitVector_TestAndSetBit(UUtUns32 *bitVector, UUtUns32 bit)
{
	__asm
	{
		bts [ecx], edx
		setc al
	}
}

UUtBool __fastcall UUrBitVector_TestAndClearBit(UUtUns32 *bitVector, UUtUns32 bit)
{
	__asm
	{
		btr [ecx], edx
		setc al
	}
}

void __fastcall UUrBitVector_SetBit(UUtUns32 *bitVector, UUtUns32 bit)
{
	__asm
	{
		bts [ecx], edx
	}
}

void __fastcall UUrBitVector_ClearBit(UUtUns32 *bitVector, UUtUns32 bit)
{
	__asm
	{
		btr [ecx], edx
	}
}

void __fastcall UUrBitVector_ToggleBit(UUtUns32 *bitVector, UUtUns32 bit)
{
	__asm
	{
		btc [ecx], edx
	}
}
#endif

UUtBool UUrBitVector_TestBitRange(const UUtUns32 *bitVector, UUtUns32 start, UUtUns32 end)
{
	UUtUns32	start_mask= BV_START_MASK(start);
	UUtUns32	end_mask= BV_MASK(end);
	UUtUns32	start_index= BV_INDEX(start);
	UUtUns32	end_index= BV_INDEX(end);

	UUmAssert(bitVector);
	if (start_index==end_index)
	{
		start_mask &= end_mask;

		if (bitVector[start_index] & start_mask) return UUcTrue;
	}
	else
	{
		UUtUns32 iterator= start_index+1;

		if (bitVector[start_index] & start_mask) return UUcTrue;

		while (iterator!=end_index)
		{
			if (bitVector[iterator++])	return UUcTrue;
		}

		if (bitVector[end_index] & end_mask) return UUcTrue;
	}

	return UUcFalse;
}

void UUrBitVector_SetBitRange(UUtUns32 *bitVector, UUtUns32 start, UUtUns32 end)
{
	UUtUns32 start_mask= BV_START_MASK(start);
	UUtUns32 end_mask= BV_MASK(end);
	UUtUns32 start_index= BV_INDEX(start);
	UUtUns32 end_index= BV_INDEX(end);

	UUmAssert(bitVector);
	if (start_index!=end_index)
	{
		UUtUns32 iterator= start_index+1;

		// loop over the middle
		while (iterator!=end_index)
		{
			bitVector[iterator++]= cAllOnes;
		}

		bitVector[end_index] |= end_mask;
	}
	else
	{
		start_mask &= end_mask;
	}

	bitVector[start_index] |= start_mask;
}

void UUrBitVector_ClearBitRange(UUtUns32 *bitVector, UUtUns32 start, UUtUns32 end)
{
	UUtUns32 start_mask= BV_START_MASK(start);
	UUtUns32 end_mask= ~BV_MASK(end);
	UUtUns32 start_index= BV_INDEX(start);
	UUtUns32 end_index= BV_INDEX(end);

	UUmAssert(bitVector);
	if (start_index!=end_index)
	{
		UUtUns32 iterator= start_index+1;

		// loop over the middle
		while (iterator!=end_index)
		{
			bitVector[iterator++]= cAllZeros;
		}

		bitVector[end_index] &= end_mask;
	}
	else
	{
		start_mask &= ~end_mask;
	}

	bitVector[start_index] &= ~start_mask;
}

void UUrBitVector_ToggleBitRange(UUtUns32 *bitVector, UUtUns32 start, UUtUns32 end)
{
	UUtUns32 start_mask= BV_START_MASK(start);
	UUtUns32 end_mask= BV_MASK(end);
	UUtUns32 start_index= BV_INDEX(start);
	UUtUns32 end_index= BV_INDEX(end);

	UUmAssert(bitVector);
	if (start_index!=end_index)
	{
		UUtUns32 iterator= start_index+1;

		// loop over the middle
		while (iterator!=end_index)
		{
			bitVector[iterator++]^= cAllOnes;
		}

		bitVector[end_index] ^= end_mask;
	}
	else
	{
		start_mask &= end_mask;
	}

	bitVector[start_index] ^= start_mask;
}

#if UUmCompiler	== UUmCompiler_VisC

#pragma auto_inline( off )

UUtUns32 __fastcall UUrBitVector_FindFirstSet(UUtInt32 r3)
{
	__asm
	{
		mov eax, 32			// if ecx = 0, eax is unset so move 32 to eax
		bsf eax, ecx
	}
}

#pragma auto_inline( on )

#elif defined(i_cntlzw)

// POWERPC count trailing zeroes
// # R3 contains x
// addi  R4,R3,-1  # x - 1
// andc  R4,R4,R3  # ~x & (x - 1)
// cntlzw  R4,R4  # t = nlz(~x & (x - 1))
// subfic  R4,R4,32  # ntz(x) = 32 - t

UUtUns32 UUrBitVector_FindFirstSet(UUtInt32 r3)
{
	UUtUns32 r4;

	r4 = r3 - 1;			// addi  R4,R3,-1  # x - 1
	r4 = ~r3 & r4;			// andc  R4,R4,R3  # ~x & (x - 1)
	r4 = i_cntlzw(r4);		// cntlzw  R4,R4  # t = nlz(~x & (x - 1))
	r4 = 32 - r4;			// subfic  R4,R4,32  # ntz(x) = 32 - t

	return r4;
}

#else

UUtUns32 UUrBitVector_FindFirstSet(UUtInt32 value)
{
	// this implementation will work for other platforms
	int i;

	for (i=0; i < 32; i++)
	{
		if (value & BV_MASK(i))
		{
			break;
		}
	}

	return i;
}

#endif

UUtUns32 UUrBitVector_FindFirstClear(UUtInt32 value)
{
	return UUrBitVector_FindFirstSet(~value);
}

UUtInt32 UUrBitVector_FindFirstSetRange(const UUtUns32 *bitVector, UUtInt32 start, UUtInt32 end)
{
	UUtUns32 start_index= BV_INDEX(start);
	UUtUns32 end_index= BV_INDEX(end);
	UUtUns32 index;
	UUtUns32 num;
	UUtUns32 start_mask= BV_START_MASK(start);
	UUtInt32 result = UUcBitVector_None;

	UUmAssert(bitVector);

	if (start > end)
	{
		return UUcBitVector_None;
	}

	if (start_index==end_index)
	{
		num= UUrBitVector_FindFirstSet(bitVector[start_index] & (start_mask & BV_MASK(end)));
		if (num < UUcUns32Bits)
		{
			result = ((start_index * UUcUns32Bits) + num);
			goto exit;
		}
	}
	else
	{
		// only check just inside the start
		num= UUrBitVector_FindFirstSet(bitVector[start_index] & start_mask);
		if (num < UUcUns32Bits)
		{
			result = ((start_index * UUcUns32Bits) + num);
			goto exit;
		}

		// loop through the middle
		for (index= start_index+1; index < end_index; index++)
		{
			num= UUrBitVector_FindFirstSet(bitVector[index]);
			if (num < UUcUns32Bits)
			{
				result = ((index * UUcUns32Bits) + num);
				goto exit;
			}
		}

		// only check upto the end
		num= UUrBitVector_FindFirstSet(bitVector[end_index] & BV_MASK(end));
		if (num < UUcUns32Bits)
		{
			result = ((end_index * UUcUns32Bits) + num);
			goto exit;
		}
	}

exit:
	return result;
}


UUtInt32 UUrBitVector_FindFirstClearRange(const UUtUns32 *bitVector, UUtInt32 start, UUtInt32 end)
{
	UUtUns32 start_index= BV_INDEX(start);
	UUtUns32 end_index= BV_INDEX(end);
	UUtUns32 index;
	UUtUns32 num;
	UUtUns32 start_mask= BV_START_MASK(start);
	UUtInt32 result = UUcBitVector_None;
	UUtUns32 final_mask;
	UUtUns32 inv_mask;

	UUmAssert(bitVector);

	if (start > end)
	{
		return UUcBitVector_None;
	}

	if (start_index==end_index)
	{
		final_mask = (start_mask & BV_MASK(end));
		inv_mask = ~final_mask;

		num= UUrBitVector_FindFirstClear((bitVector[start_index] & final_mask) | inv_mask);
		if (num < UUcUns32Bits)
		{
			result = ((start_index * UUcUns32Bits) + num);
			goto exit;
		}
	}
	else
	{
		// only check just inside the start
		num= UUrBitVector_FindFirstClear((bitVector[start_index] & start_mask) | ~start_mask);
		if (num < UUcUns32Bits)
		{
			result = ((start_index * UUcUns32Bits) + num);
			goto exit;
		}

		// loop through the middle
		for (index= start_index+1; index < end_index; index++)
		{
			num= UUrBitVector_FindFirstClear(bitVector[index]);
			if (num < UUcUns32Bits)
			{
				result = ((index * UUcUns32Bits) + num);
				goto exit;
			}
		}

		// only check upto the end
		final_mask = BV_MASK(end);
		inv_mask = ~final_mask;
		num= UUrBitVector_FindFirstClear((bitVector[end_index] & final_mask) | inv_mask);
		if (num < UUcUns32Bits)
		{
			result = ((end_index * UUcUns32Bits) + num);
			goto exit;
		}
	}

exit:
	return result;
}

// clears all the bits in bitVector to 0
// relies on UUmBitVector_Sizeof so only use properly allocated BitVectors

// optimization note:
// now just uses UUrMemory_Clear, if you really want to optomize, optimize UUrMemory_Clear
// UUrMemory_Clear is inlined under windows
// and is highly optimized for the mac
void UUrBitVector_ClearBitAll(UUtUns32 *bitVector, UUtUns32 size)
{
	UUtUns32 sizeInBytes = UUmBitVector_Sizeof(size);

	UUmAssertReadPtr(bitVector, sizeInBytes);

	UUrMemory_Clear(bitVector, sizeInBytes);

	return;
}

void UUrBitVector_SetBitAll(UUtUns32 *bitVector, UUtUns32 size)
{
	UUtUns32 sizeInBytes = UUmBitVector_Sizeof(size);

	UUmAssertReadPtr(bitVector, sizeInBytes);

	// Set8 is faster then Set32
	// under windows, inlined using the repeat instructions
	UUrMemory_Set8(bitVector, 0xFF, sizeInBytes);

	return;
}

// vector1 is xored into vector2
void UUrBitVectors_Xor(const UUtUns32 *vector1, UUtUns32 *vector2, UUtUns32 size)
{
	UUtUns32 index;

	UUmAssert(vector1);
	UUmAssert(vector2);
	for (index= 0; index < BV_INDEX(size); index++)
	{
		vector2[index] ^= vector1[index];
	}

	if (BV_NUM(size))
	{
		vector2[BV_INDEX(size)] ^= (vector1[BV_INDEX(size)] & BV_MASK(size));
	}
}

// vector1 is ored into vector2
void UUrBitVectors_Or(const UUtUns32 *vector1, UUtUns32 *vector2, UUtUns32 size)
{
	UUtUns32 index;

	UUmAssert(vector1);
	UUmAssert(vector2);
	for (index= 0; index < BV_INDEX(size); index++)
	{
		vector2[index] |= vector1[index];
	}

	if (BV_NUM(size))
	{
		vector2[BV_INDEX(size)] |= (vector1[BV_INDEX(size)] & BV_MASK(size));
	}
}

// vector1 is anded into vector2
void UUrBitVectors_And(const UUtUns32 *vector1, UUtUns32 *vector2, UUtUns32 size)
{
	UUtUns32 index;

	UUmAssert(vector1);
	UUmAssert(vector2);
	for (index= 0; index < BV_INDEX(size); index++)
	{
		vector2[index] &= vector1[index];
	}

	if (BV_NUM(size))
	{
		vector2[BV_INDEX(size)] &= (vector1[BV_INDEX(size)] & BV_MASK(size));
	}
}

// vector1 is nored into vector2
void UUrBitVectors_Nor(const UUtUns32 *vector1, UUtUns32 *vector2, UUtUns32 size)
{
	UUtUns32 index;
	UUtUns32 temp;

	UUmAssert(vector1);
	UUmAssert(vector2);
	for (index= 0; index < BV_INDEX(size); index++)
	{
		temp= (vector2[index] | vector1[index]);
		vector2[index]= ~temp;
	}

	if (BV_NUM(size))
	{
		temp= vector2[BV_INDEX(size)] | (vector1[BV_INDEX(size)] & BV_MASK(size));
		vector2[BV_INDEX(size)]= ~temp;
	}
}

// vector1 is nanded into vector2
void UUrBitVectors_Nand(const UUtUns32 *vector1, UUtUns32 *vector2, UUtUns32 size)
{	UUtUns32 index;
	UUtUns32 temp;

	UUmAssert(vector1);
	UUmAssert(vector2);
	for (index= 0; index < BV_INDEX(size); index++)
	{
		temp= (vector2[index] & vector1[index]);
		vector2[index]= ~temp;
	}

	if (BV_NUM(size))
	{
		temp= vector2[BV_INDEX(size)] & (vector1[BV_INDEX(size)] & BV_MASK(size));
		vector2[BV_INDEX(size)]= ~temp & BV_MASK(size);
	}
}

// vector1 is andced with vector2 into vectorDest
void UUrBitVectors_Andc(const UUtUns32 *vector1, const UUtUns32 *vector2, UUtUns32 *vectorDest, UUtUns32 size)
{
	UUtUns32 index;

	UUmAssert(vector1);
	UUmAssert(vector2);
	for (index= 0; index < BV_INDEX(size); index++)
	{
		vectorDest[index] = vector1[index] & ~vector2[index];
	}

	if (BV_NUM(size))
	{
		vectorDest[BV_INDEX(size)] = (vector1[index] & ~vector2[BV_INDEX(size)]) & BV_MASK(size);
	}
}

// vector1 is xnored into vector2
void UUrBitVectors_Xnor(const UUtUns32 *vector1, UUtUns32 *vector2, UUtUns32 size)
{
	UUtUns32 index;
	UUtUns32 temp;

	UUmAssert(vector1);
	UUmAssert(vector2);
	for (index= 0; index < BV_INDEX(size); index++)
	{
		temp= (vector2[index] ^ vector1[index]);
		vector2[index]= ~temp;
	}

	if (BV_NUM(size))
	{
		temp= vector2[BV_INDEX(size)] ^ (vector1[BV_INDEX(size)] & BV_MASK(size));
		vector2[BV_INDEX(size)]= ~temp;
	}
}

// vector1 is copied into vector2
void UUrBitVector_Copy(const UUtUns32 *vector1, UUtUns32 *vector2, UUtUns32 size)
{
	UUtUns32 index;

	UUmAssert(vector1);
	UUmAssert(vector2);
	for (index= 0; index < BV_INDEX(size); index++)
	{
		vector2[index] = vector1[index];
	}

	if (BV_NUM(size))
	{
		vector2[BV_INDEX(size)] = (vector1[BV_INDEX(size)] & BV_MASK(size));
	}
}

#if UUmCompiler != UUmCompiler_MWerks

#if UUmCompiler == UUmCompiler_MWerks

#pragma optimize off

#endif

UUtBool UUrBitVector_Expand16Bit(
	const UUtUns32*	bitVector,
	UUtUns16*		dst,
	UUtUns16		inBitSetColor,
	UUtUns16		inBitClearColor,
	UUtUns32 		start,
	UUtUns32 		end)
{
	UUtUns32	start_mask= BV_START_MASK(start);
	UUtUns32	end_mask= BV_MASK(end);
	UUtUns32	start_index= BV_INDEX(start);
	UUtUns32	end_index= BV_INDEX(end);
	UUtUns32	i;
	UUtUns32	pattern;

	UUmAssert(bitVector);
	if (start_index==end_index)
	{
		start_mask &= end_mask;

		pattern = bitVector[start_index] & start_mask;

		for(i = 32; i-- > 0;)
		{
			if(start_mask & (1 << i))
			{
				if(pattern & (1 << i))
				{
					*dst++ = inBitSetColor;
				}
				else
				{
					*dst++ = inBitClearColor;
				}
			}
		}
	}
	else
	{
		UUtUns32 iterator= start_index+1;

		pattern = bitVector[start_index] & start_mask;

		for(i = 32; i-- > 0;)
		{
			if(start_mask & (1 << i))
			{
				if(pattern & (1 << i))
				{
					*dst++ = inBitSetColor;
				}
				else
				{
					*dst++ = inBitClearColor;
				}
			}
		}

		while (iterator!=end_index)
		{
			pattern = bitVector[iterator++];

			if (pattern == 0xFFFFFFFF) {
				UUrMemory_Set16(dst, inBitSetColor, 32);
				dst += 32;
			}
			else if (pattern == 0) {
				UUrMemory_Set16(dst, inBitClearColor, 32);
				dst += 32;
			}
			else {
				for(i = 32; i-- > 0;)
				{
					if(pattern & (1 << i))
					{
						*dst++ = inBitSetColor;
					}
					else
					{
						*dst++ = inBitClearColor;
					}
				}
			}
		}

		pattern = bitVector[end_index] & end_mask;

		for(i = 32; i-- > 0;)
		{
			if(end_mask & (1 << i))
			{
				if(pattern & (1 << i))
				{
					*dst++ = inBitSetColor;
				}
				else
				{
					*dst++ = inBitClearColor;
				}
			}
		}
	}

	return UUcFalse;
}
#endif


static UUtUns8	g2BitSubtractor[256] =
{
    0x0,    //00000000 -> 00000000
    0x0,    //00000001 -> 00000000
    0x1,    //00000010 -> 00000001
    0x2,    //00000011 -> 00000010
    0x0,    //00000100 -> 00000000
    0x0,    //00000101 -> 00000000
    0x1,    //00000110 -> 00000001
    0x2,    //00000111 -> 00000010
    0x4,    //00001000 -> 00000100
    0x4,    //00001001 -> 00000100
    0x5,    //00001010 -> 00000101
    0x6,    //00001011 -> 00000110
    0x8,    //00001100 -> 00001000
    0x8,    //00001101 -> 00001000
    0x9,    //00001110 -> 00001001
    0xa,    //00001111 -> 00001010
    0x0,    //00010000 -> 00000000
    0x0,    //00010001 -> 00000000
    0x1,    //00010010 -> 00000001
    0x2,    //00010011 -> 00000010
    0x0,    //00010100 -> 00000000
    0x0,    //00010101 -> 00000000
    0x1,    //00010110 -> 00000001
    0x2,    //00010111 -> 00000010
    0x4,    //00011000 -> 00000100
    0x4,    //00011001 -> 00000100
    0x5,    //00011010 -> 00000101
    0x6,    //00011011 -> 00000110
    0x8,    //00011100 -> 00001000
    0x8,    //00011101 -> 00001000
    0x9,    //00011110 -> 00001001
    0xa,    //00011111 -> 00001010
    0x10,   //00100000 -> 00010000
    0x10,   //00100001 -> 00010000
    0x11,   //00100010 -> 00010001
    0x12,   //00100011 -> 00010010
    0x10,   //00100100 -> 00010000
    0x10,   //00100101 -> 00010000
    0x11,   //00100110 -> 00010001
    0x12,   //00100111 -> 00010010
    0x14,   //00101000 -> 00010100
    0x14,   //00101001 -> 00010100
    0x15,   //00101010 -> 00010101
    0x16,   //00101011 -> 00010110
    0x18,   //00101100 -> 00011000
    0x18,   //00101101 -> 00011000
    0x19,   //00101110 -> 00011001
    0x1a,   //00101111 -> 00011010
    0x20,   //00110000 -> 00100000
    0x20,   //00110001 -> 00100000
    0x21,   //00110010 -> 00100001
    0x22,   //00110011 -> 00100010
    0x20,   //00110100 -> 00100000
    0x20,   //00110101 -> 00100000
    0x21,   //00110110 -> 00100001
    0x22,   //00110111 -> 00100010
    0x24,   //00111000 -> 00100100
    0x24,   //00111001 -> 00100100
    0x25,   //00111010 -> 00100101
    0x26,   //00111011 -> 00100110
    0x28,   //00111100 -> 00101000
    0x28,   //00111101 -> 00101000
    0x29,   //00111110 -> 00101001
    0x2a,   //00111111 -> 00101010
    0x0,    //01000000 -> 00000000
    0x0,    //01000001 -> 00000000
    0x1,    //01000010 -> 00000001
    0x2,    //01000011 -> 00000010
    0x0,    //01000100 -> 00000000
    0x0,    //01000101 -> 00000000
    0x1,    //01000110 -> 00000001
    0x2,    //01000111 -> 00000010
    0x4,    //01001000 -> 00000100
    0x4,    //01001001 -> 00000100
    0x5,    //01001010 -> 00000101
    0x6,    //01001011 -> 00000110
    0x8,    //01001100 -> 00001000
    0x8,    //01001101 -> 00001000
    0x9,    //01001110 -> 00001001
    0xa,    //01001111 -> 00001010
    0x0,    //01010000 -> 00000000
    0x0,    //01010001 -> 00000000
    0x1,    //01010010 -> 00000001
    0x2,    //01010011 -> 00000010
    0x0,    //01010100 -> 00000000
    0x0,    //01010101 -> 00000000
    0x1,    //01010110 -> 00000001
    0x2,    //01010111 -> 00000010
    0x4,    //01011000 -> 00000100
    0x4,    //01011001 -> 00000100
    0x5,    //01011010 -> 00000101
    0x6,    //01011011 -> 00000110
    0x8,    //01011100 -> 00001000
    0x8,    //01011101 -> 00001000
    0x9,    //01011110 -> 00001001
    0xa,    //01011111 -> 00001010
    0x10,   //01100000 -> 00010000
    0x10,   //01100001 -> 00010000
    0x11,   //01100010 -> 00010001
    0x12,   //01100011 -> 00010010
    0x10,   //01100100 -> 00010000
    0x10,   //01100101 -> 00010000
    0x11,   //01100110 -> 00010001
    0x12,   //01100111 -> 00010010
    0x14,   //01101000 -> 00010100
    0x14,   //01101001 -> 00010100
    0x15,   //01101010 -> 00010101
    0x16,   //01101011 -> 00010110
    0x18,   //01101100 -> 00011000
    0x18,   //01101101 -> 00011000
    0x19,   //01101110 -> 00011001
    0x1a,   //01101111 -> 00011010
    0x20,   //01110000 -> 00100000
    0x20,   //01110001 -> 00100000
    0x21,   //01110010 -> 00100001
    0x22,   //01110011 -> 00100010
    0x20,   //01110100 -> 00100000
    0x20,   //01110101 -> 00100000
    0x21,   //01110110 -> 00100001
    0x22,   //01110111 -> 00100010
    0x24,   //01111000 -> 00100100
    0x24,   //01111001 -> 00100100
    0x25,   //01111010 -> 00100101
    0x26,   //01111011 -> 00100110
    0x28,   //01111100 -> 00101000
    0x28,   //01111101 -> 00101000
    0x29,   //01111110 -> 00101001
    0x2a,   //01111111 -> 00101010
    0x40,   //10000000 -> 01000000
    0x40,   //10000001 -> 01000000
    0x41,   //10000010 -> 01000001
    0x42,   //10000011 -> 01000010
    0x40,   //10000100 -> 01000000
    0x40,   //10000101 -> 01000000
    0x41,   //10000110 -> 01000001
    0x42,   //10000111 -> 01000010
    0x44,   //10001000 -> 01000100
    0x44,   //10001001 -> 01000100
    0x45,   //10001010 -> 01000101
    0x46,   //10001011 -> 01000110
    0x48,   //10001100 -> 01001000
    0x48,   //10001101 -> 01001000
    0x49,   //10001110 -> 01001001
    0x4a,   //10001111 -> 01001010
    0x40,   //10010000 -> 01000000
    0x40,   //10010001 -> 01000000
    0x41,   //10010010 -> 01000001
    0x42,   //10010011 -> 01000010
    0x40,   //10010100 -> 01000000
    0x40,   //10010101 -> 01000000
    0x41,   //10010110 -> 01000001
    0x42,   //10010111 -> 01000010
    0x44,   //10011000 -> 01000100
    0x44,   //10011001 -> 01000100
    0x45,   //10011010 -> 01000101
    0x46,   //10011011 -> 01000110
    0x48,   //10011100 -> 01001000
    0x48,   //10011101 -> 01001000
    0x49,   //10011110 -> 01001001
    0x4a,   //10011111 -> 01001010
    0x50,   //10100000 -> 01010000
    0x50,   //10100001 -> 01010000
    0x51,   //10100010 -> 01010001
    0x52,   //10100011 -> 01010010
    0x50,   //10100100 -> 01010000
    0x50,   //10100101 -> 01010000
    0x51,   //10100110 -> 01010001
    0x52,   //10100111 -> 01010010
    0x54,   //10101000 -> 01010100
    0x54,   //10101001 -> 01010100
    0x55,   //10101010 -> 01010101
    0x56,   //10101011 -> 01010110
    0x58,   //10101100 -> 01011000
    0x58,   //10101101 -> 01011000
    0x59,   //10101110 -> 01011001
    0x5a,   //10101111 -> 01011010
    0x60,   //10110000 -> 01100000
    0x60,   //10110001 -> 01100000
    0x61,   //10110010 -> 01100001
    0x62,   //10110011 -> 01100010
    0x60,   //10110100 -> 01100000
    0x60,   //10110101 -> 01100000
    0x61,   //10110110 -> 01100001
    0x62,   //10110111 -> 01100010
    0x64,   //10111000 -> 01100100
    0x64,   //10111001 -> 01100100
    0x65,   //10111010 -> 01100101
    0x66,   //10111011 -> 01100110
    0x68,   //10111100 -> 01101000
    0x68,   //10111101 -> 01101000
    0x69,   //10111110 -> 01101001
    0x6a,   //10111111 -> 01101010
    0x80,   //11000000 -> 10000000
    0x80,   //11000001 -> 10000000
    0x81,   //11000010 -> 10000001
    0x82,   //11000011 -> 10000010
    0x80,   //11000100 -> 10000000
    0x80,   //11000101 -> 10000000
    0x81,   //11000110 -> 10000001
    0x82,   //11000111 -> 10000010
    0x84,   //11001000 -> 10000100
    0x84,   //11001001 -> 10000100
    0x85,   //11001010 -> 10000101
    0x86,   //11001011 -> 10000110
    0x88,   //11001100 -> 10001000
    0x88,   //11001101 -> 10001000
    0x89,   //11001110 -> 10001001
    0x8a,   //11001111 -> 10001010
    0x80,   //11010000 -> 10000000
    0x80,   //11010001 -> 10000000
    0x81,   //11010010 -> 10000001
    0x82,   //11010011 -> 10000010
    0x80,   //11010100 -> 10000000
    0x80,   //11010101 -> 10000000
    0x81,   //11010110 -> 10000001
    0x82,   //11010111 -> 10000010
    0x84,   //11011000 -> 10000100
    0x84,   //11011001 -> 10000100
    0x85,   //11011010 -> 10000101
    0x86,   //11011011 -> 10000110
    0x88,   //11011100 -> 10001000
    0x88,   //11011101 -> 10001000
    0x89,   //11011110 -> 10001001
    0x8a,   //11011111 -> 10001010
    0x90,   //11100000 -> 10010000
    0x90,   //11100001 -> 10010000
    0x91,   //11100010 -> 10010001
    0x92,   //11100011 -> 10010010
    0x90,   //11100100 -> 10010000
    0x90,   //11100101 -> 10010000
    0x91,   //11100110 -> 10010001
    0x92,   //11100111 -> 10010010
    0x94,   //11101000 -> 10010100
    0x94,   //11101001 -> 10010100
    0x95,   //11101010 -> 10010101
    0x96,   //11101011 -> 10010110
    0x98,   //11101100 -> 10011000
    0x98,   //11101101 -> 10011000
    0x99,   //11101110 -> 10011001
    0x9a,   //11101111 -> 10011010
    0xa0,   //11110000 -> 10100000
    0xa0,   //11110001 -> 10100000
    0xa1,   //11110010 -> 10100001
    0xa2,   //11110011 -> 10100010
    0xa0,   //11110100 -> 10100000
    0xa0,   //11110101 -> 10100000
    0xa1,   //11110110 -> 10100001
    0xa2,   //11110111 -> 10100010
    0xa4,   //11111000 -> 10100100
    0xa4,   //11111001 -> 10100100
    0xa5,   //11111010 -> 10100101
    0xa6,   //11111011 -> 10100110
    0xa8,   //11111100 -> 10101000
    0xa8,   //11111101 -> 10101000
    0xa9,   //11111110 -> 10101001
    0xaa,   //11111111 -> 10101010
};


UUtUns32 *UUr2BitVector_Allocate(UUtUns32 inSize)
{
	UUtUns32 num_bytes_in_vector = ((inSize + 3) >> 2);
	UUtUns32 num_4bytes_in_vector = (num_bytes_in_vector + 3) >> 2;
	UUtUns32 *new_vector;

	new_vector = UUrMemory_Block_New(num_4bytes_in_vector * 4);

	return new_vector;
}


void UUr2BitVector_Dispose(UUtUns32 *in2BitVector)
{
	UUrMemory_Block_Delete(in2BitVector);
}

void UUr2BitVector_Clear(UUtUns32 *in2BitVector, UUtUns32 inSize)
{
	UUtUns32 num_bytes_in_vector = ((inSize + 3) >> 2);
	UUtUns32 num_4bytes_in_vector = (num_bytes_in_vector + 3) >> 2;

	UUrMemory_Clear(in2BitVector, num_4bytes_in_vector * 4);

	return;
}

void UUr2BitVector_Decrement(UUtUns32 *in2BitVector, UUtUns32 inSize)
{
	UUtUns32 *curLong = (UUtUns32 *) in2BitVector;
	UUtUns32 numLongs = (((inSize + 3) >> 2) + 3) >> 2;
	UUtUns32 *endLong = curLong + numLongs;

	for(; curLong < endLong; curLong++)
	{
		UUtUns32 long_value = *curLong;
		UUtUns32 byte1;
		UUtUns32 byte2;
		UUtUns32 byte3;
		UUtUns32 byte4;

		byte1 = (long_value >> 24) & 0xff;
		byte1 = g2BitSubtractor[byte1];

		byte2 = (long_value >> 16) & 0xff;
		byte2 = g2BitSubtractor[byte2];

		byte3 = (long_value >> 8) & 0xff;
		byte3 = g2BitSubtractor[byte3];

		byte4 = (long_value >> 0) & 0xff;
		byte4 = g2BitSubtractor[byte4];

		*curLong = (byte1 << 24) | (byte2 << 16) | (byte3 << 8) | (byte4 << 0);
	}

	return;
}
