/*
	FILE:	MS_Mac_PPCAsm.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: April 2, 1997
	
	PURPOSE: platform specific parts
	
	Copyright 1997

*/
#include <stdio.h>
#include <stdlib.h>

#include "BFW_Motoko.h"

#include "MS_DC_Private.h"
#include "MS_Mac_PPCAsm.h"

static UUtUns32 MiPPCAsm_ShiftValue(
	UUtUns32	value,
	UUtUns32	leftBit,
	UUtUns32	rightBit)
{
	UUtUns32 theShiftedValue;
	UUtUns32 theMask, theIndex;

	// convert to normal bit positions
	leftBit = 31 - leftBit;
	rightBit = 31 - rightBit;

	// build a mask to cut out unused bits
	theMask = 0xFFFFFFFF;
	for( theIndex = 0; theIndex < 32; theIndex++ )
	{
		if( (theIndex > leftBit) || (theIndex < rightBit) )
			theMask &= ~( 1 << theIndex );
	}

	// shift the value and mask it
	theShiftedValue = value << rightBit;
	theShiftedValue &= theMask;

	return theShiftedValue;
}

void MSrPPCAsm_BuildOp_LI(
	UUtUns32	generateCode,
	UUtUns32	**curInstrPtr,
	UUtUns32	*procSize,
	UUtUns32	gpr,
	UUtUns32	value)
{
	UUtUns32	instruction;
	
	if(generateCode)
	{
		instruction  = MiPPCAsm_ShiftValue( 14, 0, 5 );
		instruction |= MiPPCAsm_ShiftValue( gpr, 6, 10 );
		instruction |= MiPPCAsm_ShiftValue( value, 16, 31 );
		
		*(*curInstrPtr)++ = instruction;
	}
	else
	{
		*procSize += 4;
	}
	
}

void MSrPPCAsm_BuildOp_L32(
	UUtUns32	generateCode,
	UUtUns32	**curInstrPtr,
	UUtUns32	*procSize,
	UUtUns32	gpr,
	UUtUns32	value)
{
	UUtUns32	instruction;
	
	if(generateCode)
	{
		instruction  = MiPPCAsm_ShiftValue( 15, 0, 5 );
		instruction |= MiPPCAsm_ShiftValue( gpr, 6, 10 );
		instruction |= MiPPCAsm_ShiftValue( (value >> 16) & 0xFFFF, 16, 31 );
		
		*(*curInstrPtr)++ = instruction;
		
		instruction  = MiPPCAsm_ShiftValue( 24, 0, 5 );
		instruction |= MiPPCAsm_ShiftValue( gpr, 6, 10 );
		instruction |= MiPPCAsm_ShiftValue( gpr, 11, 15 );
		instruction |= MiPPCAsm_ShiftValue( value & 0xFFFF, 16, 31 );
		
		*(*curInstrPtr)++ = instruction;
	}
	else
	{
		*procSize += 8;
	}

}


void MSrPPCAsm_BuildOp_MTCTR(
	UUtUns32	generateCode,
	UUtUns32	**curInstrPtr,
	UUtUns32	*procSize,
	UUtUns32	gpr)
{
	UUtUns32	instruction;
	
	if(generateCode)
	{
		instruction  = MiPPCAsm_ShiftValue( 31, 0, 5 );
		instruction |= MiPPCAsm_ShiftValue( gpr, 6, 10 );
		instruction |= MiPPCAsm_ShiftValue( 288, 11, 20 );
		instruction |= MiPPCAsm_ShiftValue( 467, 21, 30 );
		
		*(*curInstrPtr)++ = instruction;
	}
	else
	{
		*procSize += 4;
	}
}

void MSrPPCAsm_BuildOp_BDNZ(
	UUtUns32	generateCode,
	UUtUns32	**curInstrPtr,
	UUtUns32	*procSize,
	UUtUns32	*pc)
{
	UUtUns32	instruction;
	
	if(generateCode)
	{
		instruction  = MiPPCAsm_ShiftValue( 16, 0, 5 );
		instruction |= MiPPCAsm_ShiftValue( 16, 6, 10 );
		instruction |= MiPPCAsm_ShiftValue( pc - *curInstrPtr, 16, 29 );
		
		*(*curInstrPtr)++ = instruction;
	}
	else
	{
		*procSize += 4;
	}
}

void MSrPPCAsm_BuildOp_BLR(
	UUtUns32	generateCode,
	UUtUns32	**curInstrPtr,
	UUtUns32	*procSize)
{
	UUtUns32	instruction;
	
	if(generateCode)
	{
		instruction  = MiPPCAsm_ShiftValue( 19, 0, 5 );
		instruction |= MiPPCAsm_ShiftValue( 20, 6, 10 );
		instruction |= MiPPCAsm_ShiftValue( 16, 21, 30 );
		
		*(*curInstrPtr)++ = instruction;
	}
	else
	{
		*procSize += 4;
	}
}

void MSrPPCAsm_BuildOp_ADDI(
	UUtUns32	generateCode,
	UUtUns32	**curInstrPtr,
	UUtUns32	*procSize,
	UUtUns32	gprDest,
	UUtUns32	gprA,
	UUtUns32	value)
{
	UUtUns32	instruction;
	
	if(generateCode)
	{
		instruction  = MiPPCAsm_ShiftValue( 14, 0, 5 );
		instruction |= MiPPCAsm_ShiftValue( gprDest, 6, 10 );
		instruction |= MiPPCAsm_ShiftValue( gprA, 11, 15 );
		instruction |= MiPPCAsm_ShiftValue( value, 16, 31 );
		
		*(*curInstrPtr)++ = instruction;
	}
	else
	{
		*procSize += 4;
	}
}

void MSrPPCAsm_BuildOp_STB(
	UUtUns32	generateCode,
	UUtUns32	**curInstrPtr,
	UUtUns32	*procSize,
	UUtUns32	gprS,
	UUtUns32	gprAddr,
	UUtUns32	disp)
{
	UUtUns32	instruction;
	
	if(generateCode)
	{
		instruction  = MiPPCAsm_ShiftValue( 38, 0, 5 );
		instruction |= MiPPCAsm_ShiftValue( gprS, 6, 10 );
		instruction |= MiPPCAsm_ShiftValue( gprAddr, 11, 15 );
		instruction |= MiPPCAsm_ShiftValue( disp, 16, 31 );
		
		*(*curInstrPtr)++ = instruction;
	}
	else
	{
		*procSize += 4;
	}
}

void MSrPPCAsm_BuildOp_STH(
	UUtUns32	generateCode,
	UUtUns32	**curInstrPtr,
	UUtUns32	*procSize,
	UUtUns32	gprS,
	UUtUns32	gprAddr,
	UUtUns32	disp)
{
	UUtUns32	instruction;
	
	if(generateCode)
	{
		instruction  = MiPPCAsm_ShiftValue( 44, 0, 5 );
		instruction |= MiPPCAsm_ShiftValue( gprS, 6, 10 );
		instruction |= MiPPCAsm_ShiftValue( gprAddr, 11, 15 );
		instruction |= MiPPCAsm_ShiftValue( disp, 16, 31 );
		
		*(*curInstrPtr)++ = instruction;
	}
	else
	{
		*procSize += 4;
	}
}

void MSrPPCAsm_BuildOp_STW(
	UUtUns32	generateCode,
	UUtUns32	**curInstrPtr,
	UUtUns32	*procSize,
	UUtUns32	gprS,
	UUtUns32	gprAddr,
	UUtUns32	disp)
{
	UUtUns32	instruction;
	
	if(generateCode)
	{
		instruction  = MiPPCAsm_ShiftValue( 36, 0, 5 );
		instruction |= MiPPCAsm_ShiftValue( gprS, 6, 10 );
		instruction |= MiPPCAsm_ShiftValue( gprAddr, 11, 15 );
		instruction |= MiPPCAsm_ShiftValue( disp, 16, 31 );
		
		*(*curInstrPtr)++ = instruction;
	}
	else
	{
		*procSize += 4;
	}
}

void MSrPPCAsm_BuildOp_STFD(
	UUtUns32	generateCode,
	UUtUns32	**curInstrPtr,
	UUtUns32	*procSize,
	UUtUns32	fS,
	UUtUns32	gprAddr,
	UUtUns32	disp)
{
	UUtUns32	instruction;
	
	if(generateCode)
	{
		instruction  = MiPPCAsm_ShiftValue( 54, 0, 5 );
		instruction |= MiPPCAsm_ShiftValue( fS, 6, 10 );
		instruction |= MiPPCAsm_ShiftValue( gprAddr, 11, 15 );
		instruction |= MiPPCAsm_ShiftValue( disp, 16, 31 );
		
		*(*curInstrPtr)++ = instruction;
	}
	else
	{
		*procSize += 4;
	}
}

void MSrPPCAsm_BuildOp_LBZ(
	UUtUns32	generateCode,
	UUtUns32	**curInstrPtr,
	UUtUns32	*procSize,
	UUtUns32	gprD,
	UUtUns32	gprAddr,
	UUtUns32	disp)
{
	UUtUns32	instruction;
	
	if(generateCode)
	{
		instruction  = MiPPCAsm_ShiftValue( 34, 0, 5 );
		instruction |= MiPPCAsm_ShiftValue( gprD, 6, 10 );
		instruction |= MiPPCAsm_ShiftValue( gprAddr, 11, 15 );
		instruction |= MiPPCAsm_ShiftValue( disp, 16, 31 );
		
		*(*curInstrPtr)++ = instruction;
	}
	else
	{
		*procSize += 4;
	}
}

void MSrPPCAsm_BuildOp_LHZ(
	UUtUns32	generateCode,
	UUtUns32	**curInstrPtr,
	UUtUns32	*procSize,
	UUtUns32	gprD,
	UUtUns32	gprAddr,
	UUtUns32	disp)
{
	UUtUns32	instruction;
	
	if(generateCode)
	{
		instruction  = MiPPCAsm_ShiftValue( 40, 0, 5 );
		instruction |= MiPPCAsm_ShiftValue( gprD, 6, 10 );
		instruction |= MiPPCAsm_ShiftValue( gprAddr, 11, 15 );
		instruction |= MiPPCAsm_ShiftValue( disp, 16, 31 );
		
		*(*curInstrPtr)++ = instruction;
	}
	else
	{
		*procSize += 4;
	}
}

void MSrPPCAsm_BuildOp_LWZ(
	UUtUns32	generateCode,
	UUtUns32	**curInstrPtr,
	UUtUns32	*procSize,
	UUtUns32	gprD,
	UUtUns32	gprAddr,
	UUtUns32	disp)
{
	UUtUns32	instruction;
	
	if(generateCode)
	{
		instruction  = MiPPCAsm_ShiftValue( 32, 0, 5 );
		instruction |= MiPPCAsm_ShiftValue( gprD, 6, 10 );
		instruction |= MiPPCAsm_ShiftValue( gprAddr, 11, 15 );
		instruction |= MiPPCAsm_ShiftValue( disp, 16, 31 );
		
		*(*curInstrPtr)++ = instruction;
	}
	else
	{
		*procSize += 4;
	}
}

void MSrPPCAsm_BuildOp_LFD(
	UUtUns32	generateCode,
	UUtUns32	**curInstrPtr,
	UUtUns32	*procSize,
	UUtUns32	fD,
	UUtUns32	gprAddr,
	UUtUns32	disp)
{
	UUtUns32	instruction;
	
	if(generateCode)
	{
		instruction  = MiPPCAsm_ShiftValue( 50, 0, 5 );
		instruction |= MiPPCAsm_ShiftValue( fD, 6, 10 );
		instruction |= MiPPCAsm_ShiftValue( gprAddr, 11, 15 );
		instruction |= MiPPCAsm_ShiftValue( disp, 16, 31 );
		
		*(*curInstrPtr)++ = instruction;
	}
	else
	{
		*procSize += 4;
	}
}

void MSrPPCAsm_BuildOp_DCBZ(
	UUtUns32	generateCode,
	UUtUns32	**curInstrPtr,
	UUtUns32	*procSize,
	UUtUns32	gprA,
	UUtUns32	gprB)
{
	UUtUns32	instruction;
	
	if(generateCode)
	{
		instruction  = MiPPCAsm_ShiftValue( 31, 0, 5 );
		instruction |= MiPPCAsm_ShiftValue( gprA, 11, 15 );
		instruction |= MiPPCAsm_ShiftValue( gprB, 16, 20 );
		instruction |= MiPPCAsm_ShiftValue( 1014, 21, 30 );
		
		*(*curInstrPtr)++ = instruction;
	}
	else
	{
		*procSize += 4;
	}
}

void MSrPPCAsm_BuildOp_DCBT(
	UUtUns32	generateCode,
	UUtUns32	**curInstrPtr,
	UUtUns32	*procSize,
	UUtUns32	gprA,
	UUtUns32	gprB)
{
	UUtUns32	instruction;
	
	if(generateCode)
	{
		instruction  = MiPPCAsm_ShiftValue( 31, 0, 5 );
		instruction |= MiPPCAsm_ShiftValue( gprA, 11, 15 );
		instruction |= MiPPCAsm_ShiftValue( gprB, 16, 20 );
		instruction |= MiPPCAsm_ShiftValue( 278, 21, 30 );
		
		*(*curInstrPtr)++ = instruction;
	}
	else
	{
		*procSize += 4;
	}
}
