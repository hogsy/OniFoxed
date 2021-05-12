// ======================================================================
// Oni_Object_Private.h
// ======================================================================
#pragma once

#ifndef ONI_OBJECT_PRIVATE_H
#define ONI_OBJECT_PRIVATE_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

#include "Oni_Object.h"


// ======================================================================
// defines
// ======================================================================
#define OBJcMaxInstances			2048

// ----------------------------------------------------------------------
#define OBJcWrite_Little			0
#define OBJcWrite_Big				1

// CB: moved to BFW_BinaryData.h to unify with OBDm macros
#define OBJmGetByteFromBuffer(src,dst,type,swap_it)					BDmGetByteFromBuffer(src,dst,type,swap_it)
#define OBJmWriteByteToBuffer(buf,val,type,num_bytes,write_big)		BDmWriteByteToBuffer(buf,val,type,num_bytes,write_big)
#define OBJmSkip4BytesFromBuffer(src)								BDmSkip4BytesFromBuffer(src)
#define OBJmGet4BytesFromBuffer(src,dst,type,swap_it)				BDmGet4BytesFromBuffer(src,dst,type,swap_it)
#define OBJmWrite4BytesToBuffer(buf,val,type,num_bytes,write_big)	BDmWrite4BytesToBuffer(buf,val,type,num_bytes,write_big)
#define OBJmSkip2BytesFromBuffer(src)								BDmSkip2BytesFromBuffer(src)
#define OBJmGet2BytesFromBuffer(src,dst,type,swap_it)				BDmGet2BytesFromBuffer(src,dst,type,swap_it)
#define OBJmWrite2BytesToBuffer(buf,val,type,num_bytes,write_big)	BDmWrite2BytesToBuffer(buf,val,type,num_bytes,write_big)
#define OBJmGet4CharsFromBuffer(buf,c0,c1,c2,c3)					BDmGet4CharsFromBuffer(buf,c0,c1,c2,c3)
#define OBJmWrite4CharsToBuffer(buf,c0,c1,c2,c3,num_bytes)			BDmWrite4CharsToBuffer(buf,c0,c1,c2,c3,num_bytes)
#define OBJmGetStringFromBuffer(src,dst,len,swap_it)				BDmGetStringFromBuffer(src,dst,len,swap_it)
#define OBJmWriteStringToBuffer(buf,val,len,num_bytes, write_big)	BDmWriteStringToBuffer(buf,val,len,num_bytes, write_big)

#define OBJmMakeObjectTag(type_index, object)		(((type_index & 0xFF) << 24) | (object->object_id & 0x00FFFFFF))

// ----------------------------------------------------------------------


// ======================================================================
// enums
// ======================================================================
enum
{
	OBJcDrawFlag_None			= 0x0000,
	OBJcDrawFlag_Selected		= 0x0001,
	OBJcDrawFlag_RingX			= 0x0002,
	OBJcDrawFlag_RingY			= 0x0004,
	OBJcDrawFlag_RingZ			= 0x0008,
	
	OBJcDrawFlag_Locked			= 0x8000
	
};

// ======================================================================
// typedefs
// ======================================================================


// low 16 bits are internal flags
enum
{
	OBJcObjectGroupFlag_None = 0,
	OBJcObjectGroupFlag_CanSetName = 1 << 16,
	OBJcObjectGroupFlag_HasUniqueName = 1 << 17,
	OBJcObjectGroupFlag_CommonToAllLevels = 1 << 18
};


// ======================================================================
// prototypes
// ======================================================================
UUtError
OBJrObjectGroup_Register(
	OBJtObjectType				inObjectType,
	UUtUns32					inObjectTypeIndex,
	char						*inGroupName,
	UUtUns32					inSizeInMemory,
	OBJtMethods					*inMethods,
	UUtUns32					inFlags);

// ----------------------------------------------------------------------
void
OBJrObjectUtil_DrawAxes(
	const float				inLength);
	
void
OBJrObjectUtil_DrawRotationRings(
	const OBJtObject		*inObject,
	const M3tBoundingSphere	*inBoundingSphere,
	UUtUns32				inDrawRings);
	
UUtError
OBJrObjectUtil_EnumerateTemplate(
	char							*inPrefix,
	TMtTemplateTag					inTemplateTag,
	OBJtEnumCallback_ObjectName		inEnumCallback,
	UUtUns32						inUserData);
	
// ----------------------------------------------------------------------
UUtError
OBJrConsole_Initialize(
	void);
	
// ----------------------------------------------------------------------
UUtError
OBJrDoor_Initialize(
	void);

// ----------------------------------------------------------------------
UUtError
OBJrFlag_Initialize(
	void);

#if TOOL_VERSION
void
OBJrFlag_DrawTerminate(
	void);
#endif

// ----------------------------------------------------------------------
UUtError
OBJrFurniture_Initialize(
	void);

// ----------------------------------------------------------------------
UUtError
OBJrTurret_Initialize(
	void);

// ----------------------------------------------------------------------
UUtError
OBJrCharacter_Initialize(
	void);

#if TOOL_VERSION
void
OBJrCharacter_DrawTerminate(
	void);
#endif
	
// ----------------------------------------------------------------------
UUtError
OBJrParticle_Initialize(
	void);

// ----------------------------------------------------------------------
UUtError
OBJrPatrolPath_Initialize(
	void);

#if TOOL_VERSION
void
OBJrPatrolPath_DrawTerminate(
	void);
#endif
	
// ----------------------------------------------------------------------
UUtError
OBJrSound_Initialize(
	void);

#if TOOL_VERSION
void
OBJrSound_DrawTerminate(
	void);
#endif
	
// ----------------------------------------------------------------------
UUtError
OBJrTrigger_Initialize(
	void);

// ----------------------------------------------------------------------
UUtError
OBJrTriggerVolume_Initialize(
	void);

// ----------------------------------------------------------------------
UUtError 
OBJrCombat_Initialize(
	void);

// ----------------------------------------------------------------------
UUtError 
OBJrMelee_Initialize(
	void);

// ----------------------------------------------------------------------
UUtError
OBJrNeutral_Initialize(
	void);

// ----------------------------------------------------------------------

void
OBJrDoor_GetExportMatricies(
	OBJtObject *inObject,
	M3tMatrix4x3 *ioMatrix1A,
	M3tMatrix4x3 *ioMatrix1B,
	M3tMatrix4x3 *ioMatrix2A,
	M3tMatrix4x3 *ioMatrix2B);

#if TOOL_VERSION
UUtError OBJrDoor_DrawInitialize(void);
void OBJrDoor_DrawTerminate(void);
#endif

UUtError OBJrDoor_Initialize(void);

// ----------------------------------------------------------------------
UUtError
OBJrPowerUp_Initialize(
	void);
	
// ----------------------------------------------------------------------
UUtError
OBJrWeapon_Initialize(
	void);
	
// ======================================================================

#endif /* ONI_OBJECT_PRIVATE_H */