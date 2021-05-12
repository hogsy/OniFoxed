// ======================================================================
// Oni_BinaryData.h
// ======================================================================
#pragma once

#ifndef ONI_BINARYDATA_H
#define ONI_BINARYDATA_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_BinaryData.h"

// ======================================================================
// defines
// ======================================================================
#define OBJcWrite_Little			0
#define OBJcWrite_Big				1

// CB: moved to BFW_BinaryData.h to unify with OBJm macros
#define OBDmSkip4BytesFromBuffer(src)								BDmSkip4BytesFromBuffer(src)
#define OBDmGet4BytesFromBuffer(src,dst,type,swap_it)				BDmGet4BytesFromBuffer(src,dst,type,swap_it)
#define OBDmWrite4BytesToBuffer(buf,val,type,num_bytes,write_big)	BDmWrite4BytesToBuffer(buf,val,type,num_bytes,write_big)
#define OBDmSkip2BytesFromBuffer(src)								BDmSkip2BytesFromBuffer(src)
#define OBDmGet2BytesFromBuffer(src,dst,type,swap_it)				BDmGet2BytesFromBuffer(src,dst,type,swap_it)
#define OBDmWrite2BytesToBuffer(buf,val,type,num_bytes,write_big)	BDmWrite2BytesToBuffer(buf,val,type,num_bytes,write_big)
#define OBDmGet4CharsFromBuffer(buf,c0,c1,c2,c3)					BDmGet4CharsFromBuffer(buf,c0,c1,c2,c3)
#define OBDmWrite4CharsToBuffer(buf,c0,c1,c2,c3,num_bytes)			BDmWrite4CharsToBuffer(buf,c0,c1,c2,c3,num_bytes)
#define OBDmGetStringFromBuffer(src,dst,len,swap_it)				BDmGetStringFromBuffer(src,dst,len,swap_it)
#define OBDmWriteStringToBuffer(buf,val,len,num_bytes, write_big)	BDmWriteStringToBuffer(buf,val,len,num_bytes, write_big)

// ======================================================================
// prototypes
// ======================================================================
UUtError
OBDrBinaryData_Load(
	BFtFileRef			*inFileRef);

UUtError
OBDrBinaryData_Save(
	BDtClassType		inClassType,
	const char			*inIdentifier,
	UUtUns8				*inBuffer,
	UUtUns32			inBufferSize,
	UUtUns16			inLevelNumber,
	UUtBool				inAutosave);

// ----------------------------------------------------------------------
UUtError
OBDrLevel_Load(
	UUtUns16				inLevelNumber);

void
OBDrLevel_Unload(
	void);
	
// ----------------------------------------------------------------------
void
OBDrMakeDirty(
	void);

void
OBDrAutosave(
	void);

// ----------------------------------------------------------------------
UUtError
OBDrInitialize(
	void);

void
OBDrTerminate(
	void);

// CB: check to see if we have to autosave
void
OBDrUpdate(
	UUtUns32 inTime);

// ======================================================================
#endif /* ONI_BINARYDATA_H */