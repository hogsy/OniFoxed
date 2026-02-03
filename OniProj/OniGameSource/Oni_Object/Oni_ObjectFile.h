// ======================================================================
// Oni_ObjectFile.h
// ======================================================================

#pragma once

#ifndef ONI_OBJECTFILE_H
#define ONI_OBJECTFILE_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_FileManager.h"

// ======================================================================
// defines
// ======================================================================
#define OBJcMaxFileNameLength		31
#define OBJcMaxSubNameLength		20

// ----------------------------------------------------------------------
#define OBJmGet4BytesFromBuffer(src,dst,type,swap_it)				\
			(dst) = *((type*)(src));								\
			((UUtUns8*)(src)) += sizeof(type);						\
			if ((swap_it)) { UUrSwap_4Byte(&(dst)); }

#define OBJmWrite4BytesToBuffer(buf,val,type,num_bytes)				\
			*((type*)(buf)) = (val);								\
			((UUtUns8*)(buf)) += sizeof(type);						\
			(num_bytes) -= sizeof(type);

#define OBJmGet2BytesFromBuffer(src,dst,type,swap_it)				\
			(dst) = *((type*)(src));								\
			((UUtUns8*)(src)) += sizeof(type);						\
			if ((swap_it)) { UUrSwap_2Byte(&(dst)); }

#define OBJmWrite2BytesToBuffer(buf,val,type,num_bytes)				\
			*((type*)(buf)) = (val);								\
			((UUtUns8*)(buf)) += sizeof(type);						\
			(num_bytes) -= sizeof(type);

// ======================================================================
// prototypes
// ======================================================================
void
OBJrObjectFiles_Close(
	void);

UUtError
OBJrObjectFiles_LevelLoad(
	UUtUns16				inLevelNumber);

void
OBJrObjectFiles_LevelUnload(
	void);

void
OBJrObjectFiles_Open(
	void);

UUtError
OBJrObjectFiles_RegisterObjectType(
	UUtUns32				inObjectType,
	char					*inSubName);

UUtError
OBJrObjectFiles_SaveBegin(
	void);

UUtError
OBJrObjectFiles_SaveEnd(
	void);

UUtError
OBJrObjectFiles_Write(
	UUtUns32				inObjectType,
	UUtUns8					*inBuffer,
	UUtUns32				inNumBytes);

// ----------------------------------------------------------------------
UUtError
OBJrObjectFiles_Initialize(
	void);

void
OBJrObjectFiles_Terminate(
	void);

// ======================================================================
#endif /* ONI_OBJECTFILE_H */
