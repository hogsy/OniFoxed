/*
	FILE:	MS_Mac_CustomProc.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: May 19, 1997
	
	PURPOSE: Interface to the Motoko 3D engine
	
	Copyright 1997

*/

#include "BFW_Motoko.h"

#include "MS_DC_Private.h"

#include "MS_Mac_CustomProc.h"
#include "MS_Mac_PPCAsm.h"

UUtError  MSrMacCustomProc_BuildZBufferClear(
	MStDrawContextPrivate	*drawContextPrivate)
{
	UUtUns32	generatingCode;
	UUtUns32	customProcSize;
	UUtUns32	*customProcMemory;
	UUtUns32	*customProcBaseAddr;
	UUtUns32	*curInstrPtr;
	UUtUns32	*loopPC;
	
	UUtUns32	destAddrReg;
	UUtUns32	clearValueFReg;
	UUtUns32	tempReg0;
	MStPlatformSpecific	*platformSpecific;
	
	platformSpecific = &drawContextPrivate->platformSpecific;

	UUmAssert(platformSpecific->zBufferClearProcMemory == NULL);
	
	destAddrReg = 4;
	clearValueFReg = 1;
	tempReg0 = 6;
	
	
	/* Do an initial pass to determine the size of the custom proc, generatingCode = 0 */
	/* Do a second pass to generate the code, generatingCode = 1 */
	for(generatingCode = 0; generatingCode < 2; generatingCode++)
	{
		if(generatingCode)
		{
			/* Allocate memory for the custom proc */
			customProcMemory = UUrMemory_Block_New(customProcSize + UUcProcessor_CacheLineSize);
			
			if(customProcMemory == NULL)
			{
				UUrError_Report(UUcError_OutOfMemory, "Could not allocate z buffer clear memory");
				
				return UUcError_OutOfMemory;
			}
			
			/* Make sure the function ptr is cache aligned */
			customProcBaseAddr =
				(UUtUns32 *)(((UUtUns32)customProcMemory + UUcProcessor_CacheLineSize - 1) &
					~UUcProcessor_CacheLineSize_Mask);
			
			curInstrPtr = customProcBaseAddr;
		}
		else
		{
			customProcSize = 0;
		}
		
		/* r5 has a pointer to the zClearValue */
		MSrPPCAsm_BuildOp_LFD(
			generatingCode,
			&curInstrPtr,
			&customProcSize,
			clearValueFReg,
			5,
			0);

		/* Load the base address into destAddrReg */
		MSrPPCAsm_BuildOp_L32(
			generatingCode,
			&curInstrPtr,
			&customProcSize,
			destAddrReg,
			(UUtUns32)drawContextPrivate->zBufferBaseAddr);
		
		/*
		 * Load number of iterations into CTR
		 */
			MSrPPCAsm_BuildOp_L32(
				generatingCode,
				&curInstrPtr,
				&customProcSize,
				tempReg0,
				drawContextPrivate->height * drawContextPrivate->width * M3cDrawZBytesPerPixel / UUcProcessor_CacheLineSize);
			MSrPPCAsm_BuildOp_MTCTR(
				generatingCode,
				&curInstrPtr,
				&customProcSize,
				tempReg0);
		
		loopPC = curInstrPtr;
		
		MSrPPCAsm_BuildOp_DCBZ(
			generatingCode,
			&curInstrPtr,
			&customProcSize,
			0,
			destAddrReg);
		
		#if 1
		MSrPPCAsm_BuildOp_STFD(
			generatingCode,
			&curInstrPtr,
			&customProcSize,
			clearValueFReg,		/* Clear Value */
			destAddrReg,		/* Base Addr */
			0);					/* Offset */

		MSrPPCAsm_BuildOp_STFD(
			generatingCode,
			&curInstrPtr,
			&customProcSize,
			clearValueFReg,		/* Clear Value */
			destAddrReg,		/* Base Addr */
			8);					/* Offset */

		MSrPPCAsm_BuildOp_STFD(
			generatingCode,
			&curInstrPtr,
			&customProcSize,
			clearValueFReg,		/* Clear Value */
			destAddrReg,		/* Base Addr */
			16);				/* Offset */

		MSrPPCAsm_BuildOp_STFD(
			generatingCode,
			&curInstrPtr,
			&customProcSize,
			clearValueFReg,		/* Clear Value */
			destAddrReg,		/* Base Addr */
			24);				/* Offset */
		#endif

		MSrPPCAsm_BuildOp_ADDI(
			generatingCode,
			&curInstrPtr,
			&customProcSize,
			destAddrReg,
			destAddrReg,
			UUcProcessor_CacheLineSize);

		/* BDNZ back to top */
		MSrPPCAsm_BuildOp_BDNZ(
			generatingCode,
			&curInstrPtr,
			&customProcSize,
			loopPC);
		
		/* Return from the subroutine */
		MSrPPCAsm_BuildOp_BLR(
			generatingCode,
			&curInstrPtr,
			&customProcSize);
	}

	platformSpecific->zBufferClearProcMemory = customProcMemory;
	
	platformSpecific->zBufferClearProc = (MStBufferProcPtr)customProcBaseAddr;
	
	/* Flush the data cache so that we can execute our custom proc */
	MakeDataExecutable(customProcBaseAddr, customProcSize);

	return UUcError_None;
}

UUtError  MSrMacCustomProc_BuildImageBufferClear(
	MStDrawContextPrivate	*drawContextPrivate)
{
	UUtUns32	generatingCode;
	UUtUns32	customProcSize;
	UUtUns32	*customProcMemory;
	UUtUns32	*customProcBaseAddr;
	UUtUns32	*curInstrPtr;
	UUtUns32	*loopPC;
	
	UUtInt32	destAddrReg;
	UUtInt32	tempReg0;
	UUtInt32	clearValueReg;
	UUtInt32	clearValueFReg;
	MStPlatformSpecific	*platformSpecific;
	
	platformSpecific = &drawContextPrivate->platformSpecific;
	
	UUmAssert(platformSpecific->imageBufferClearProcMemory == NULL);
	
	destAddrReg = 4;
	tempReg0 = 6;
	clearValueReg = 7;
	clearValueFReg = 5;
	
	
	/* Do an initial pass to determine the size of the custom proc, generatingCode = 0 */
	/* Do a second pass to generate the code, generatingCode = 1 */
	for(generatingCode = 0; generatingCode < 2; generatingCode++)
	{
		if(generatingCode)
		{
			/* Allocate memory for the custom proc */
			customProcMemory = UUrMemory_Block_New(customProcSize + UUcProcessor_CacheLineSize);
			
			if(customProcMemory == NULL)
			{
				UUrError_Report(UUcError_OutOfMemory, "Could not allocate image buffer clear memory");
				
				return UUcError_OutOfMemory;
			}
			
			/* Make sure the function ptr is cache aligned */
			customProcBaseAddr =
				(UUtUns32 *)(((UUtUns32)customProcMemory + UUcProcessor_CacheLineSize - 1) &
					~UUcProcessor_CacheLineSize_Mask);
			
			curInstrPtr = customProcBaseAddr;
		}
		else
		{
			customProcSize = 0;
		}
		
		/* Load the base address into destAddrReg */
		MSrPPCAsm_BuildOp_L32(
			generatingCode,
			&curInstrPtr,
			&customProcSize,
			destAddrReg,
			(UUtUns32)drawContextPrivate->imageBufferBaseAddr);
		
		if(drawContextPrivate->screenFlags & M3cDrawContextScreenFlags_DoubleBuffer)
		{
			/*
			 * Load number of iterations into CTR
			 */
				MSrPPCAsm_BuildOp_L32(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempReg0,
					drawContextPrivate->height * drawContextPrivate->width * M3cDrawRGBBytesPerPixel / UUcProcessor_CacheLineSize);
				MSrPPCAsm_BuildOp_MTCTR(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempReg0);
			
			loopPC = curInstrPtr;
			
			MSrPPCAsm_BuildOp_DCBZ(
				generatingCode,
				&curInstrPtr,
				&customProcSize,
				0,
				destAddrReg);
			
			MSrPPCAsm_BuildOp_ADDI(
				generatingCode,
				&curInstrPtr,
				&customProcSize,
				destAddrReg,
				destAddrReg,
				UUcProcessor_CacheLineSize);
		}
		else
		{
			UUtInt32	addrAlignment;
			UUtInt32	offset;
			UUtInt32	imageRowBytes;

			addrAlignment = (UUtUns32)platformSpecific->frontImageBaseAddr;
			offset = 0;
			imageRowBytes = drawContextPrivate->width * M3cDrawRGBBytesPerPixel;
			//imageRowBytes = platformSpecific->imageBufferBaseAddr;
			
			/* r5 has a pointer to the clearValue */
			MSrPPCAsm_BuildOp_LFD(
				generatingCode,
				&curInstrPtr,
				&customProcSize,
				clearValueFReg,
				5,
				0);
			
			MSrPPCAsm_BuildOp_LI(
				generatingCode,
				&curInstrPtr,
				&customProcSize,
				clearValueReg,
				0);

			/*
			 * Load number of iterations into CTR
			 */
				MSrPPCAsm_BuildOp_L32(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempReg0,
					drawContextPrivate->height);
				MSrPPCAsm_BuildOp_MTCTR(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempReg0);
			
			loopPC = curInstrPtr;
			
			if(addrAlignment & 0x01)
			{
				UUmAssert("Illegal condition");
			}
			
			if(addrAlignment & 0x02)
			{
				MSrPPCAsm_BuildOp_STH(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueReg,
					destAddrReg,
					offset);
				offset += 2;
				addrAlignment += 2;
			}
			
			if(addrAlignment & 0x04)
			{
				MSrPPCAsm_BuildOp_STW(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueReg,
					destAddrReg,
					offset);
				offset += 4;
				addrAlignment += 4;
			}
			
			if(addrAlignment & 0x8)
			{
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueFReg,
					destAddrReg,
					offset);
				offset += 8;
				addrAlignment += 8;
			}
			
			if(addrAlignment & 0x10)
			{
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueFReg,
					destAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueFReg,
					destAddrReg,
					offset+8);
				offset += 16;
				addrAlignment += 16;
			}
			
			while((imageRowBytes - offset) >= UUcProcessor_CacheLineSize)
			{
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueFReg,
					destAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueFReg,
					destAddrReg,
					offset+8);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueFReg,
					destAddrReg,
					offset+16);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueFReg,
					destAddrReg,
					offset+24);
				
				offset += 32;
				addrAlignment += 32;
			}
			
			if((imageRowBytes - offset) >= 16)
			{
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueFReg,
					destAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueFReg,
					destAddrReg,
					offset + 8);
				offset += 16;
				addrAlignment += 16;
			}
			
			if((imageRowBytes - offset) >= 8)
			{
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueFReg,
					destAddrReg,
					offset);
				offset += 8;
				addrAlignment += 8;
			}
			
			if((imageRowBytes - offset) >= 4)
			{
				MSrPPCAsm_BuildOp_STW(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueReg,
					destAddrReg,
					offset);
				offset += 4;
				addrAlignment += 4;
			}
			if((imageRowBytes - offset) >= 2)
			{
				MSrPPCAsm_BuildOp_STH(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueReg,
					destAddrReg,
					offset);
				offset += 2;
				addrAlignment += 2;
			}
			
			MSrPPCAsm_BuildOp_ADDI(
				generatingCode,
				&curInstrPtr,
				&customProcSize,
				destAddrReg,
				destAddrReg,
				drawContextPrivate->imageBufferRowBytes);
			
		}
		
		/* BDNZ back to top */
		MSrPPCAsm_BuildOp_BDNZ(
			generatingCode,
			&curInstrPtr,
			&customProcSize,
			loopPC);
		
		/* Return from the subroutine */
		MSrPPCAsm_BuildOp_BLR(
			generatingCode,
			&curInstrPtr,
			&customProcSize);
	}

	platformSpecific->imageBufferClearProcMemory = customProcMemory;
	
	platformSpecific->imageBufferClearProc = (MStBufferProcPtr)customProcBaseAddr;
	
	/* Flush the data cache so that we can execute our custom proc */
	MakeDataExecutable(customProcBaseAddr, customProcSize);

	return UUcError_None;
}

UUtError  MSrMacCustomProc_BuildImageBufferCopy(
	MStDrawContextPrivate	*drawContextPrivate)
{
	UUtUns32	generatingCode;
	UUtUns32	customProcSize;
	UUtUns32	*customProcMemory;
	UUtUns32	*customProcBaseAddr;
	UUtUns32	*curInstrPtr;
	UUtUns32	*loopPC;
	
	UUtInt32	destAddrReg;
	UUtInt32	srcAddrReg;
	UUtInt32	tempReg0;
	UUtInt32	tempFReg0;
	UUtInt32	tempFReg1;
	UUtInt32	tempFReg2;
	UUtInt32	tempFReg3;
	UUtInt32	addrAlignment;
	UUtInt32	offset;
	UUtInt32	imageRowBytes;
	MStPlatformSpecific	*platformSpecific;
	
	platformSpecific = &drawContextPrivate->platformSpecific;
	
	UUmAssert(platformSpecific->imageBufferCopyProcMemory == NULL);
	
	destAddrReg = 4;
	srcAddrReg = 5;
	tempReg0 = 6;
	
	tempFReg0 = 1;
	tempFReg1 = 2;
	tempFReg2 = 3;
	tempFReg3 = 4;
	
	
	/* Do an initial pass to determine the size of the custom proc, generatingCode = 0 */
	/* Do a second pass to generate the code, generatingCode = 1 */
	for(generatingCode = 0; generatingCode < 2; generatingCode++)
	{
		if(generatingCode)
		{
			/* Allocate memory for the custom proc */
			customProcMemory = UUrMemory_Block_New(customProcSize + UUcProcessor_CacheLineSize);
			
			if(customProcMemory == NULL)
			{
				UUrError_Report(UUcError_OutOfMemory, "Could not allocate image buffer copy memory");
				
				return UUcError_OutOfMemory;
			}
			
			/* Make sure the function ptr is cache aligned */
			customProcBaseAddr =
				(UUtUns32 *)(((UUtUns32)customProcMemory + UUcProcessor_CacheLineSize - 1) &
					~UUcProcessor_CacheLineSize_Mask);
			
			curInstrPtr = customProcBaseAddr;
		}
		else
		{
			customProcSize = 0;
		}
		
		/* Load the dest base address into destAddrReg */
		MSrPPCAsm_BuildOp_L32(
			generatingCode,
			&curInstrPtr,
			&customProcSize,
			destAddrReg,
			(UUtUns32)platformSpecific->frontImageBaseAddr);
		
		/* Load the src base address into srcAddrReg */
		MSrPPCAsm_BuildOp_L32(
			generatingCode,
			&curInstrPtr,
			&customProcSize,
			srcAddrReg,
			(UUtUns32)drawContextPrivate->imageBufferBaseAddr);
		
		addrAlignment = (UUtUns32)platformSpecific->frontImageBaseAddr;
		offset = 0;
		imageRowBytes = drawContextPrivate->width * M3cDrawRGBBytesPerPixel;
		
		/*
		 * Load number of iterations into CTR
		 */
			MSrPPCAsm_BuildOp_L32(
				generatingCode,
				&curInstrPtr,
				&customProcSize,
				tempReg0,
				drawContextPrivate->height);
			MSrPPCAsm_BuildOp_MTCTR(
				generatingCode,
				&curInstrPtr,
				&customProcSize,
				tempReg0);
		
		loopPC = curInstrPtr;
		
		/*
		 * Build scanline copy
		 */
			if(addrAlignment & 0x01)
			{
				UUmAssert("Illegal condition");
			}
			
			if(addrAlignment & 0x02)
			{
				MSrPPCAsm_BuildOp_LHZ(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempReg0,
					srcAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STH(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempReg0,
					destAddrReg,
					offset);
				offset += 2;
				addrAlignment += 2;
			}
			
			if(addrAlignment & 0x04)
			{
				MSrPPCAsm_BuildOp_LWZ(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempReg0,
					srcAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STW(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempReg0,
					destAddrReg,
					offset);
				offset += 4;
				addrAlignment += 4;
			}
			
			if(addrAlignment & 0x8)
			{
				MSrPPCAsm_BuildOp_LFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg0,
					srcAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg0,
					destAddrReg,
					offset);
				offset += 8;
				addrAlignment += 8;
			}
			
			if(addrAlignment & 0x10)
			{
				MSrPPCAsm_BuildOp_LFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg0,
					srcAddrReg,
					offset);
				MSrPPCAsm_BuildOp_LFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg1,
					srcAddrReg,
					offset+8);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg0,
					destAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg1,
					destAddrReg,
					offset+8);
				offset += 16;
				addrAlignment += 16;
			}
			
			while((imageRowBytes - offset) >= UUcProcessor_CacheLineSize)
			{
				MSrPPCAsm_BuildOp_LFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg0,
					srcAddrReg,
					offset);
				MSrPPCAsm_BuildOp_LFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg1,
					srcAddrReg,
					offset+8);
				MSrPPCAsm_BuildOp_LFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg2,
					srcAddrReg,
					offset+16);
				MSrPPCAsm_BuildOp_LFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg3,
					srcAddrReg,
					offset+24);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg0,
					destAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg1,
					destAddrReg,
					offset+8);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg2,
					destAddrReg,
					offset+16);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg3,
					destAddrReg,
					offset+24);
				offset += 32;
				addrAlignment += 32;
			}
			
			if((imageRowBytes - offset) >= 16)
			{
				MSrPPCAsm_BuildOp_LFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg0,
					srcAddrReg,
					offset);
				MSrPPCAsm_BuildOp_LFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg1,
					srcAddrReg,
					offset+8);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg0,
					destAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg1,
					destAddrReg,
					offset + 8);
				offset += 16;
				addrAlignment += 16;
			}
			
			if((imageRowBytes - offset) >= 8)
			{
				MSrPPCAsm_BuildOp_LFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg0,
					srcAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg0,
					destAddrReg,
					offset);
				offset += 8;
				addrAlignment += 8;
			}
			
			if((imageRowBytes - offset) >= 4)
			{
				MSrPPCAsm_BuildOp_LWZ(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempReg0,
					srcAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STW(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempReg0,
					destAddrReg,
					offset);
				offset += 4;
				addrAlignment += 4;
			}
			if((imageRowBytes - offset) >= 2)
			{
				MSrPPCAsm_BuildOp_LHZ(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempReg0,
					srcAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STH(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempReg0,
					destAddrReg,
					offset);
				offset += 2;
				addrAlignment += 2;
			}
		
		
		
		MSrPPCAsm_BuildOp_ADDI(
			generatingCode,
			&curInstrPtr,
			&customProcSize,
			destAddrReg,
			destAddrReg,
			platformSpecific->frontImageRowBytes);

		MSrPPCAsm_BuildOp_ADDI(
			generatingCode,
			&curInstrPtr,
			&customProcSize,
			srcAddrReg,
			srcAddrReg,
			drawContextPrivate->imageBufferRowBytes);

		/* BDNZ back to top */
		MSrPPCAsm_BuildOp_BDNZ(
			generatingCode,
			&curInstrPtr,
			&customProcSize,
			loopPC);
		
		/* Return from the subroutine */
		MSrPPCAsm_BuildOp_BLR(
			generatingCode,
			&curInstrPtr,
			&customProcSize);
	}

	platformSpecific->imageBufferCopyProcMemory = customProcMemory;
	
	platformSpecific->imageBufferCopyProc = (MStBufferProcPtr)customProcBaseAddr;
	
	/* Flush the data cache so that we can execute our custom proc */
	MakeDataExecutable(customProcBaseAddr, customProcSize);

	return UUcError_None;
}

UUtError  MSrMacCustomProc_BuildImageBufferCopyAndClear(
	MStDrawContextPrivate	*drawContextPrivate)
{
	UUtUns32	generatingCode;
	UUtUns32	customProcSize;
	UUtUns32	*customProcMemory;
	UUtUns32	*customProcBaseAddr;
	UUtUns32	*curInstrPtr;
	UUtUns32	*loopPC;
	
	UUtInt32	destAddrReg;
	UUtInt32	srcAddrReg;
	UUtInt32	clearValueReg;
	UUtInt32	clearValueFReg;
	UUtInt32	tempReg0;
	UUtInt32	tempFReg0;
	UUtInt32	tempFReg1;
	UUtInt32	tempFReg2;
	UUtInt32	tempFReg3;
	UUtInt32	addrAlignment;
	UUtInt32	offset;
	UUtInt32	imageRowBytes;
	MStPlatformSpecific	*platformSpecific;
	
	platformSpecific = &drawContextPrivate->platformSpecific;
	
	UUmAssert(platformSpecific->imageBufferCopyAndClearProcMemory == NULL);
	
	destAddrReg = 4;
	srcAddrReg = 5;
	tempReg0 = 6;
	clearValueReg = 7;
	
	tempFReg0 = 1;
	tempFReg1 = 2;
	tempFReg2 = 3;
	tempFReg3 = 4;
	clearValueFReg = 5;
	
	/* Do an initial pass to determine the size of the custom proc, generatingCode = 0 */
	/* Do a second pass to generate the code, generatingCode = 1 */
	for(generatingCode = 0; generatingCode < 2; generatingCode++)
	{
		if(generatingCode)
		{
			/* Allocate memory for the custom proc */
			customProcMemory = UUrMemory_Block_New(customProcSize + UUcProcessor_CacheLineSize);
			
			if(customProcMemory == NULL)
			{
				UUrError_Report(UUcError_OutOfMemory, "Could not allocate image buffer copy and clear memory");
				
				return UUcError_OutOfMemory;
			}
			
			/* Make sure the function ptr is cache aligned */
			customProcBaseAddr =
				(UUtUns32 *)(((UUtUns32)customProcMemory + UUcProcessor_CacheLineSize - 1) &
					~UUcProcessor_CacheLineSize_Mask);
			
			curInstrPtr = customProcBaseAddr;
		}
		else
		{
			customProcSize = 0;
		}
		
		/* r5 has a pointer to the clearValue */
		MSrPPCAsm_BuildOp_LFD(
			generatingCode,
			&curInstrPtr,
			&customProcSize,
			clearValueFReg,
			5,
			0);
		
		MSrPPCAsm_BuildOp_LI(
			generatingCode,
			&curInstrPtr,
			&customProcSize,
			clearValueReg,
			0);
					
		/* Load the dest base address into destAddrReg */
		MSrPPCAsm_BuildOp_L32(
			generatingCode,
			&curInstrPtr,
			&customProcSize,
			destAddrReg,
			(UUtUns32)platformSpecific->frontImageBaseAddr);
		
		/* Load the src base address into srcAddrReg */
		MSrPPCAsm_BuildOp_L32(
			generatingCode,
			&curInstrPtr,
			&customProcSize,
			srcAddrReg,
			(UUtUns32)drawContextPrivate->imageBufferBaseAddr);
		
		addrAlignment = (UUtUns32)platformSpecific->frontImageBaseAddr;
		offset = 0;
		imageRowBytes = drawContextPrivate->width * M3cDrawRGBBytesPerPixel;
		
		/*
		 * Load number of iterations into CTR
		 */
			MSrPPCAsm_BuildOp_L32(
				generatingCode,
				&curInstrPtr,
				&customProcSize,
				tempReg0,
				drawContextPrivate->height);
			MSrPPCAsm_BuildOp_MTCTR(
				generatingCode,
				&curInstrPtr,
				&customProcSize,
				tempReg0);
		
		loopPC = curInstrPtr;
		
		/*
		 * Build scanline copy
		 */
			if(addrAlignment & 0x01)
			{
				UUmAssert("Illegal condition");
			}
			
			if(addrAlignment & 0x02)
			{
				
				MSrPPCAsm_BuildOp_LHZ(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempReg0,
					srcAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STH(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempReg0,
					destAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STH(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueReg,
					srcAddrReg,
					offset);
				offset += 2;
				addrAlignment += 2;
			}
			
			if(addrAlignment & 0x04)
			{
				MSrPPCAsm_BuildOp_LWZ(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempReg0,
					srcAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STW(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempReg0,
					destAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STW(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueReg,
					srcAddrReg,
					offset);
				offset += 4;
				addrAlignment += 4;
			}
			
			if(addrAlignment & 0x8)
			{
				MSrPPCAsm_BuildOp_LFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg0,
					srcAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg0,
					destAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueFReg,
					srcAddrReg,
					offset);
				offset += 8;
				addrAlignment += 8;
			}
			
			if(addrAlignment & 0x10)
			{
				MSrPPCAsm_BuildOp_LFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg0,
					srcAddrReg,
					offset);
				MSrPPCAsm_BuildOp_LFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg1,
					srcAddrReg,
					offset+8);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg0,
					destAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg1,
					destAddrReg,
					offset+8);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueFReg,
					srcAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueFReg,
					srcAddrReg,
					offset+8);
				offset += 16;
				addrAlignment += 16;
			}
			
			while((imageRowBytes - offset) >= UUcProcessor_CacheLineSize)
			{
				MSrPPCAsm_BuildOp_LFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg0,
					srcAddrReg,
					offset);
				MSrPPCAsm_BuildOp_LFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg1,
					srcAddrReg,
					offset + 8);
				MSrPPCAsm_BuildOp_LFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg2,
					srcAddrReg,
					offset + 16);
				MSrPPCAsm_BuildOp_LFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg3,
					srcAddrReg,
					offset + 24);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg0,
					destAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg1,
					destAddrReg,
					offset+8);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg2,
					destAddrReg,
					offset+16);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg3,
					destAddrReg,
					offset+24);

				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueFReg,
					srcAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueFReg,
					srcAddrReg,
					offset+8);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueFReg,
					srcAddrReg,
					offset+16);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueFReg,
					srcAddrReg,
					offset+24);
				
				offset += 32;
				addrAlignment += 32;
			}
			
			if((imageRowBytes - offset) >= 16)
			{
				MSrPPCAsm_BuildOp_LFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg0,
					srcAddrReg,
					offset);
				MSrPPCAsm_BuildOp_LFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg1,
					srcAddrReg,
					offset + 8);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg0,
					destAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg1,
					destAddrReg,
					offset + 8);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueFReg,
					srcAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueFReg,
					srcAddrReg,
					offset + 8);
				offset += 16;
				addrAlignment += 16;
			}
			
			if((imageRowBytes - offset) >= 8)
			{
				MSrPPCAsm_BuildOp_LFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg0,
					srcAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempFReg0,
					destAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STFD(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueFReg,
					srcAddrReg,
					offset);
				offset += 8;
				addrAlignment += 8;
			}
			
			if((imageRowBytes - offset) >= 4)
			{
				MSrPPCAsm_BuildOp_LWZ(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempReg0,
					srcAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STW(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempReg0,
					destAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STW(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueReg,
					srcAddrReg,
					offset);
				offset += 4;
				addrAlignment += 4;
			}
			if((imageRowBytes - offset) >= 2)
			{
				MSrPPCAsm_BuildOp_LHZ(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempReg0,
					srcAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STH(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					tempReg0,
					destAddrReg,
					offset);
				MSrPPCAsm_BuildOp_STH(
					generatingCode,
					&curInstrPtr,
					&customProcSize,
					clearValueReg,
					srcAddrReg,
					offset);
				offset += 2;
				addrAlignment += 2;
			}
				
		MSrPPCAsm_BuildOp_ADDI(
			generatingCode,
			&curInstrPtr,
			&customProcSize,
			destAddrReg,
			destAddrReg,
			platformSpecific->frontImageRowBytes);
		
		MSrPPCAsm_BuildOp_ADDI(
			generatingCode,
			&curInstrPtr,
			&customProcSize,
			srcAddrReg,
			srcAddrReg,
			drawContextPrivate->imageBufferRowBytes);
		
		/* BDNZ back to top */
		MSrPPCAsm_BuildOp_BDNZ(
			generatingCode,
			&curInstrPtr,
			&customProcSize,
			loopPC);
		
		/* Return from the subroutine */
		MSrPPCAsm_BuildOp_BLR(
			generatingCode,
			&curInstrPtr,
			&customProcSize);
	}

	platformSpecific->imageBufferCopyAndClearProcMemory = customProcMemory;
	
	platformSpecific->imageBufferCopyAndClearProc = (MStBufferProcPtr)customProcBaseAddr;
	
	/* Flush the data cache so that we can execute our custom proc */
	MakeDataExecutable(customProcBaseAddr, customProcSize);

	return UUcError_None;
}
