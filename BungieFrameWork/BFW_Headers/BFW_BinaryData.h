// ======================================================================
// BFW_BinaryData.h
// ======================================================================
#pragma once

#ifndef BFW_BINARYDATA_H
#define BFW_BINARYDATA_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_FileManager.h"

// ======================================================================
// typedefs
// ======================================================================
typedef UUtUns32			BDtClassType;

// ----------------------------------------------------------------------
#define BDcTemplate_BinaryData				UUm4CharToUns32('B', 'I', 'N', 'A')
typedef tm_template('B', 'I', 'N', 'A', "Binary Data")
BDtBinaryData
{
	UUtUns32				data_size;
	tm_separate				data_index;
	
} BDtBinaryData;

typedef struct BDtHeader
{
	BDtClassType			class_type;
	UUtUns32				data_size;
	
} BDtHeader;

typedef struct BDtData
{
	BDtHeader				header;
	UUtUns8					data[1];
	
} BDtData;

// ----------------------------------------------------------------------
typedef UUtError
(*BDtMethod_Load)(
	const char				*inIdentifier,
	BDtData					*ioBinaryData,
	UUtBool					inSwapIt,
	UUtBool					inLocked,
	UUtBool					inAllocated);


typedef struct BDtMethods
{
	BDtMethod_Load			rLoad;
	
} BDtMethods;

// ======================================================================
// macros
// ======================================================================
#define BDcWrite_Little				0
#define BDcWrite_Big				1

#define BDmGetByteFromBuffer(src,dst,type,swap_it)					\
			1;														\
			do {													\
			(dst) = *((type *)(src));								\
			*(UUtUns8**)&(src) += 1;									\
			} while (0)

#define BDmWriteByteToBuffer(buf,val,type,num_bytes,write_big)		\
			1;														\
			do {													\
			*((type*)(buf)) = (val);								\
			*(UUtUns8**)&(buf) += 1;									\
			UUmAssert(sizeof(type) == 1);							\
			UUmAssert((num_bytes) >= 1);							\
			(num_bytes) -= 1;										\
			} while (0)

#define BDmSkip4BytesFromBuffer(src)								\
			4; do {													\
			*(UUtUns8**)&(src) += 4;									\
			} while(0)	

#define BDmGet4BytesFromBuffer(src,dst,type,swap_it)				\
			4;														\
			do {													\
			(dst) = *((type*)(src));								\
			*(UUtUns8**)&(src) += sizeof(type);						\
			if ((swap_it)) { UUrSwap_4Byte(&(dst)); }				\
			} while (0)

#define BDmWrite4BytesToBuffer(buf,val,type,num_bytes,write_big)	\
			4;														\
			do {													\
			*((type*)(buf)) = (val);								\
			if (write_big) { UUmSwapBig_4Byte(buf); } else { UUmSwapLittle_4Byte(buf); } \
			*(UUtUns8**)&(buf) += sizeof(type);						\
			UUmAssert((num_bytes) >= sizeof(type));					\
			(num_bytes) -= sizeof(type);							\
			} while (0)

#define	BDmSkip2BytesFromBuffer(src)								\
			2; do {													\
			*(UUtUns8**)&(src) += 2;								\
			} while(0)	

#define BDmGet2BytesFromBuffer(src,dst,type,swap_it)				\
			2;														\
			do {													\
			(dst) = *((type*)(src));								\
			*(UUtUns8**)&(src) += sizeof(type);						\
			if ((swap_it)) { UUrSwap_2Byte(&(dst)); }				\
			} while (0)

#define BDmWrite2BytesToBuffer(buf,val,type,num_bytes,write_big)	\
			2;														\
			do {													\
			*((type*)(buf)) = (val);								\
			if (write_big) { UUmSwapBig_2Byte(buf); } else { UUmSwapLittle_2Byte(buf); } \
			*(UUtUns8**)&(buf) += sizeof(type);						\
			UUmAssert((num_bytes) >= sizeof(type));					\
			(num_bytes) -= sizeof(type);							\
			} while (0)

#define BDmGet4CharsFromBuffer(buf,c0,c1,c2,c3)						\
			4;														\
			do {													\
			(c0) = ((UUtUns8*)buf)[0];								\
			(c1) = ((UUtUns8*)buf)[1];								\
			(c2) = ((UUtUns8*)buf)[2];								\
			(c3) = ((UUtUns8*)buf)[3];								\
			*(UUtUns8**)&(buf) += (sizeof(UUtUns8) * 4);			\
			} while (0)

#define BDmWrite4CharsToBuffer(buf,c0,c1,c2,c3,num_bytes)			\
			4;														\
			do {													\
			*((UUtUns8*)(buf)) = (c0);								\
			*((UUtUns8*)(buf + 1)) = (c1);							\
			*((UUtUns8*)(buf + 2)) = (c2);							\
			*((UUtUns8*)(buf + 3)) = (c3);							\
			*(UUtUns8**)&(buf) += (sizeof(UUtUns8) * 4);			\
			UUmAssert((num_bytes) >= sizeof(UUtUns8));				\
			(num_bytes) -= (sizeof(UUtUns8) * 4);					\
			} while (0)

#define BDmGetStringFromBuffer(src,dst,len,swap_it)					\
			(len); do {												\
			UUrString_Copy((dst), (char *) (src), (len));			\
			*(UUtUns8**)&(src) += (len);	} while(0)			

#define BDmWriteStringToBuffer(buf,val,len,num_bytes, write_big)	\
			(len); do {												\
			UUrMemory_Clear((void *) (buf), (len));					\
			UUrString_Copy((char *) (buf), (char *) (val), (len));	\
			*(UUtUns8**)&(buf) += (len);								\
			(num_bytes) -= (len); } while(0)

// ======================================================================
// prototypes
// ======================================================================
UUtError
BDrBinaryData_Write(
	BDtClassType			inClassType,
	UUtUns8					*inData,
	UUtUns32				inDataSize,
	BFtFile					*inFile);

// ----------------------------------------------------------------------
UUtError
BDrBinaryLoader(
	const char				*inIdentifier,
	UUtUns8					*inBuffer,
	UUtUns32				inBufferSize,
	UUtBool					inLocked,
	UUtBool					inAllocated);

UUtError
BDrRegisterClass(
	BDtClassType			inClassType,
	BDtMethods				*inMethods);

// ----------------------------------------------------------------------
UUtError
BDrInitialize(
	void);

UUtError
BDrRegisterTemplates(
	void);

void
BDrTerminate(
	void);

// ======================================================================
#endif /* BFW_BINARYDATA_H */
