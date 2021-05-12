// ======================================================================
// Oni_Net_Support.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_TemplateManager.h"
#include "BFW_Console.h"
#include "BFW_MathLib.h"
#include "BFW_Totoro.h"

#include "Oni.h"
#include "Oni_Camera.h"
#include "Oni_Networking.h"
#include "Oni_Net_Support.h"
#include "Oni_GameState.h"
#include "Oni_AI_Setup.h"
#include "Oni_Level.h"
#include "Oni_Weapon.h"
#include "Oni_Character_Animation.h"

// ======================================================================
// defines
// ======================================================================
#define ONcNet_SwapIt			'SI'

#define ONcNet_DataType_Length	sizeof(ONtNet_DataType)

#define ONcNet_FindServer_Broadcast_Interval	(5 * 60) /* in ticks */

#define ONcNet_InventoryBits	(LIc_BitMask_Swap | LIc_BitMask_Drop | LIc_BitMask_Hypo | LIc_BitMask_Reload)

// ----------------------------------------------------------------------
#define ONmNet_AddDataType(data_type,dest_data)							\
	*(ONtNet_DataType*)dest_data = data_type;							\
	(ONtNet_DataType*)dest_data += ONcNet_DataType_Length;

#define ONmNet_GetDataType(src_data, data_type)							\
	data_type = *((ONtNet_DataType*)src_data);							\
	src_data += ONcNet_DataType_Length;
	
// ----------------------------------------------------------------------
#define ONmNet_GetDataPtr(group, type)									\
	(type*)group->data;													\
	ONiNet_DataAdvance(group, sizeof(type));

// ----------------------------------------------------------------------
#define ONmNet_AddData(data_type,src_data,src_data_size,dst_data)		\
	ONmNet_AddDataType(data_type,dst_data);								\
	if (src_data != NULL) {												\
		UUrMemory_MoveFast(src_data,dst_data,src_data_size);			\
		((UUtUns8*)dst_data) += src_data_size; }

#define ONmNet_AddOrdinal(data_type,src_data,src_type,dst_data)			\
	ONmNet_AddDataType(data_type,dst_data);								\
	(*((src_type*)dst_data)) = src_data;								\
	((UUtUns8*)dst_data) += sizeof(src_type);
	
#define ONmNet_AddStruct(data_type,src_data,src_type,dst_data)			\
	ONmNet_AddDataType(data_type,dst_data);								\
	UUrMemory_MoveFast(src_data,dst_data,sizeof(src_type));				\
	((UUtUns8*)dst_data) += sizeof(src_type);

#define ONmNet_SimpleAdd(var1,var2,var1_type,data)										\
	if (inCharacter->var1 != ioPrevUpdate->var2) {										\
		ioPrevUpdate->var2 = inCharacter->var1;											\
		ONmNet_AddOrdinal(ONcNet_Update_##var2, inCharacter->var1, var1_type, data);	\
		outUpdate->hdr.num_updates++; }

#define ONmNet_ComplexAdd(compare,var1,var2,var1_type,data)								\
	if (!compare(inCharacter->var1, ioPrevUpdate->var2)) {								\
		ioPrevUpdate->var2 = inCharacter->var1;											\
		ONmNet_AddOrdinal(ONcNet_Update_##var2, inCharacter->var1, var1_type, data);	\
		outUpdate->hdr.num_updates++; }

#define ONmNet_SimpleAdd2(buffer_data, var1, var2, var1_type, datatype)						\
	if (var1 != var2) {																		\
		var2 = var1;																		\
		ONiNet_Buffer_AddData(buffer_data, datatype, (UUtUns8*)&var1, sizeof(var1_type)); }

#define ONmNet_ComplexAdd2(buffer_data, compare, var1, var2, var1_type, datatype)					\
	if (!compare(var1, var2)) {																\
		var2 = var1;																		\
		ONiNet_Buffer_AddData(buffer_data, datatype, (UUtUns8*)&var1, sizeof(var1_type)); }

// ----------------------------------------------------------------------
#define ONmNet_Update(type,var_name)									\
	character->var_name = *(type*)update_data;							\
	update_data += sizeof(type);
	
#define ONmNet_SwapUpdate(swap_func,type,var_name)						\
	character->var_name = *(type*)update_data;							\
	if (ioProcessGroup->swap_it) { swap_func(&character->var_name);	}	\
	update_data += sizeof(type);

#define ONmNet_SwapComplexUpdate(swap_func,type,var_name)				\
	character->var_name = *(type*)update_data;							\
	if (ioProcessGroup->swap_it) { swap_func(character->var_name); }	\
	update_data += sizeof(type);

#define ONmNet_Update2(type, var, data)									\
	var = *(type*)data;													\
	data += sizeof(type);

#define ONmNet_SwapUpdate2Byte(type, var, data, swap_it)				\
	ONmNet_Update2(type, var, data);									\
	if (swap_it) { UUrSwap_2Byte(&var); }

#define ONmNet_SwapUpdate4Byte(type, var, data, swap_it)				\
	ONmNet_Update2(type, var, data);									\
	if (swap_it) { UUrSwap_4Byte(&var); }

#define ONmNet_SwapUpdateFunc(type, var, data, swap_it, swap_func)		\
	ONmNet_Update2(type, var, data);									\
	if (swap_it) { swap_func(var); }

// ----------------------------------------------------------------------
#if defined(DEBUGGING) && DEBUGGING

#define ONmNet_CheckByteAlignment(adr)									\
	UUmAssert((((UUtUns32)adr) & 3) == 0);

#else

#define ONmNet_CheckByteAlignment(adr)

#endif

// ======================================================================
// globals
// ======================================================================
static M3tPoint3D				ONgNet_WeaponPosition[WPcMaxWeapons];

// ======================================================================
// enums
// ======================================================================
enum
{
	ONcNone,						// this starts the count
	
	// Client to Server ACKs
	ONcC2S_ACK_Critical_Update,
	ONcC2S_ACK_Weapon_Update,
	
	// Client to Server Heartbeat
	ONcC2S_Heartbeat,
	
	// Client to Server Requests
	ONcC2S_Request_CharacterCreate,
	ONcC2S_Request_CharacterInfo,
	ONcC2S_Request_Join,
	ONcC2S_Request_Spawn,
	ONcC2S_Request_Quit,
	ONcC2S_Request_WeaponInfo,
	
	// Client to Server Updates
	ONcC2S_Update_Character,
	ONcC2S_Update_Critical,
	
	// Server to Client ACKs
	ONcS2C_ACK_Critical_Update,
	ONcS2C_ACK_Quit,
	
	// Server to Client Info
	ONcS2C_Info_Character,
	ONcS2C_Info_CharacterCreate,
	ONcS2C_Info_Game,
	
	// Server to Client Reject
	ONcS2C_Reject_Join,
	
	// Server to Client Updates
	ONcS2C_Update_Character,
	ONcS2C_Update_Critical,
	ONcS2C_Update_Weapon,
	
	// FindServer
	ONcC2S_Request_ServerInfo,
	ONcS2C_ServerInfo
};

// ----------------------------------------------------------------------
enum
{
	ONcNet_CU_None,				// this starts the count
	ONcNet_CU_Health,
	ONcNet_CU_NumKills,
	ONcNet_CU_DamageInflicted,
	ONcNet_CU_Inventory,
	ONcNet_CU_Name,
	ONcNet_CU_Class,
	ONcNet_CU_Spawn,
	ONcNet_CU_Gone,
	
	ONcNet_NumCUpdates
};

// ----------------------------------------------------------------------
enum
{
	ONcNet_WU_None,				// this starts the count
	ONcNet_WU_Create,
	ONcNet_WU_Delete,
	ONcNet_WU_Ammo,
	ONcNet_WU_Position,
	
	ONcNet_NumWUpdates
};

// ======================================================================
// typedefs
// ======================================================================
// ----------------------------------------------------------------------
// NOTE:  All of these structs must have a sizeof() that is divisible by 4 
// ----------------------------------------------------------------------
typedef struct ONtNet_TwoUns16
{
	UUtUns16				uns16_0;
	UUtUns16				uns16_1;
	
} ONtNet_TwoUns16;

typedef struct ONtNet_OneUns32
{
	UUtUns32				uns32_0;
	
} ONtNet_OneUns32;

// ------------------------------
typedef ONtNet_TwoUns16		ONtNet_Ack_CriticalUpdate;
typedef ONtNet_TwoUns16		ONtNet_Ack_WeaponUpdate;
typedef ONtNet_OneUns32		ONtNet_Ack_Quit;

// ------------------------------
typedef struct ONtNet_Info_Character
{
	UUtUns16				character_index;		// 2
	UUtUns16				team_number;			// 2
	UUtUns16				anim_frame;				// 2
	
	UUtUns16				pad;					// 2 ***
	
	float					facing;					// 4
	M3tPoint3D				position;				// 12
	
	char					character_class_name[AIcMaxClassNameLen];		// 64
	char					player_name[ONcMaxPlayerNameLength];			// 32
	char					animName[ONcMaxInstNameLength];					// 32
	
} ONtNet_Info_Character;

typedef struct ONtNet_Info_CharacterCreate
{
	UUtUns32				request_id;				// 2
	UUtUns16				character_index;		// 2
	UUtUns16				pad;					// 2 ***
	float					facing;					// 4
	M3tPoint3D				position;				// 12
	
} ONtNet_Info_CharacterCreate;

typedef struct ONtNet_Info_Game
{
	UUtUns32				request_id;				// 2
	UUtUns16				player_index;			// 2
	UUtUns16				level_number;			// 2
	
} ONtNet_Info_Game;

typedef struct ONtNet_Info_Spawn
{
	float					facing;					// 4
	M3tPoint3D				position;				// 12
	
} ONtNet_Info_Spawn;

// ------------------------------
typedef ONtNet_OneUns32		ONtNet_Reject_Join;				// uns32_0 = request_id

// ------------------------------
typedef struct ONtNet_Request_CharacterCreate
{
	UUtUns32				request_id;				// 4
	UUtUns16				teamNumber;				// 2
	UUtUns16				pad;					// 2 ***
	char					characterClassName[AIcMaxClassNameLen];			// 64
	char					playerName[ONcMaxPlayerNameLength];				// 32
	
} ONtNet_Request_CharacterCreate;

typedef ONtNet_TwoUns16		ONtNet_Request_CharacterInfo;	// uns16_0 = character_index
typedef ONtNet_OneUns32		ONtNet_Request_Join;			// uns32_0 = request_id
typedef ONtNet_OneUns32		ONtNet_Request_Quit;			// uns32_0 = request_id

// ------------------------------
typedef struct ONtNet_Update_Header
{
	UUtUns16				character_index;		// 2	the index of the character being updated

	// the following data is always updated for a character
	UUtUns16				animFrame;				// 2	the current frame of the animation
	M3tPoint3D				position;				// 12	the current position of the character

	char					animName[ONcMaxInstNameLength];		// 32
	
} ONtNet_Update_Header;

typedef struct ONtNet_Update_Character
{
	ONtNet_Update_Header	hdr;
	UUtUns8					data[sizeof(ONtNet_UC_Data)];	// the updated data
	
} ONtNet_Update_Character;

// this is the minimum size of an ONtNet_Update_Character
const UUtUns32				ONcNet_Update_Character_MinSize = sizeof(ONtNet_Update_Header); 

// ------------------------------
typedef struct ONtNet_CU_Data
{
	UUtUns32			hitPoints;								// 4
	UUtUns32			numKills;								// 4
	UUtUns32			damageInflicted;						// 4
	UUtUns32			weaponIndex;							// 4
	
	ONtNet_Info_Spawn	spawnInfo;								// 16
	
	char				playerName[ONcMaxPlayerNameLength];		// 32
	char				characterClassName[AIcMaxClassNameLen];	// 64
	
	// make sure there is room for the datatypes
	ONtNet_DataType		datatypes[ONcNet_NumCUpdates];
	
} ONtNet_CU_Data;

typedef struct ONtNet_CUpdate_Header
{
	UUtUns16				character_index;		// 2	the index of the character being updated
	UUtUns16				pad;					// 2	
	
} ONtNet_CUpdate_Header;

typedef struct ONtNet_Critical_Update
{
	ONtNet_CUpdate_Header	hdr;
	UUtUns8					data[sizeof(ONtNet_CU_Data)];	// the update data
	
} ONtNet_Critical_Update;

// this is the minimum size of an ONtNet_Critical_Update
const UUtUns32				ONcNet_CUpdate_MinSize = sizeof(ONtNet_CUpdate_Header); 

// ----------------------------------------------------------------------
typedef struct ONtNet_WU_Data
{
	char					weapon_class_name[ONcMaxInstNameLength];	// 32
	M3tPoint3D				position;									// 12
	WPtAmmoGroup			ammo_group;									// 16
	
	// make sure there is room for the datatypes
	ONtNet_DataType			datatypes[ONcNet_NumWUpdates];
	
} ONtNet_WU_Data;

typedef struct ONtNet_WUpdate_Header
{
	UUtUns16				weapon_index;			// 2	the index of the weapon being updated
	UUtUns16				pad;					// 2	
	
} ONtNet_WUpdate_Header;

typedef struct ONtNet_Weapon_Update
{
	ONtNet_WUpdate_Header	hdr;
	UUtUns8					data[sizeof(ONtNet_WU_Data)];	// the update data
	
} ONtNet_Weapon_Update;

// this is the minimum size of an ONtNet_Weapon_Update
const UUtUns32				ONcNet_WUpdate_MinSize = sizeof(ONtNet_WUpdate_Header); 

typedef struct ONtNet_Inventory
{
	UUtUns16				weapons[5];
	UUtUns16				ammo;
	UUtUns16				hypo;
	UUtUns16				cell;
	
} ONtNet_Inventory;

typedef ONtNet_TwoUns16		ONtNet_Request_WeaponInfo;		// uns16_0 = weapon_index

// ----------------------------------------------------------------------
typedef struct ONtNet_inAir
{
	UUtUns16				numFramesInAir;
	UUtUns16				flags;
	M3tVector3D				velocity;
	
} ONtNet_inAir;

typedef struct ONtNet_throw
{
	UUtUns16				throwing;
	UUtUns16				throwFrame;
	char					throwName[ONcMaxInstNameLength];
	
} ONtNet_throw;

// ----------------------------------------------------------------------
typedef struct ONtNet_BufferData
{
	UUtUns8					*data;			// start of the buffer
	UUtUns8					*data_ptr;		// current point to add data in the buffer
	ONtNet_DataTypeInfo		*datatype_info;	// datatype info
	ONtNet_DataType			datatypes[ONcNet_MaxDataTypesPerPacket];	
											// storage for the datatypes until they are
											// copied into the buffer
	
} ONtNet_BufferData;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
ONiNet_Swap_PacketHeader(
	ONtNet_Packet			*inPacket)
{
	if (inPacket->packet_header.packet_swap != ONcNet_SwapIt)
	{	
		UUrSwap_4Byte(&inPacket->packet_header.packet_number);
		UUrSwap_2Byte(&inPacket->packet_header.packet_version);
		UUrSwap_2Byte(&inPacket->packet_header.player_index);
		UUrSwap_2Byte(&inPacket->packet_header.packet_length);
		
		return UUcTrue;
	}
	
	return UUcFalse;
}

// ----------------------------------------------------------------------
static UUcInline void
ONiNet_DataAdvance(
	ONtNet_ProcessGroup		*ioProcessGroup,
	UUtUns16				inNumberOfBytes)
{
	ioProcessGroup->data =
		(UUtUns8*)((UUtUns32)(ioProcessGroup->data + inNumberOfBytes + 3) & ~3);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns16
ONiNet_DataTypes_GetSize(
	ONtNet_DataTypeInfo		*inDataTypeInfo)
{
	return ((inDataTypeInfo->num_datatypes + 3) & ~3);
}

// ----------------------------------------------------------------------
static UUtUns16
ONiNet_DataType_GetDataSize(
	ONtNet_DataType			inDataType)
{
	UUtUns16				size;
	
	switch (inDataType)
	{
		case ONcNone:								size = 0; break;
		case ONcNet_Update_isAiming:				size = sizeof(UUtBool); break;
		case ONcNet_Update_validTarget:				size = sizeof(UUtBool); break;
		case ONcNet_Update_fightFramesRemaining:	size = sizeof(UUtUns16); break;
		case ONcNet_Update_fire:					size = sizeof(WPtAmmoGroup); break;
		case ONcNet_Update_throw:					size = sizeof(ONtNet_throw); break;
		case ONcNet_Update_inAir:					size = sizeof(ONtNet_inAir); break;
		case ONcNet_Update_facing:					size = sizeof(float); break;
		case ONcNet_Update_desiredFacing:			size = sizeof(float); break;
		case ONcNet_Update_aimingLR:				size = sizeof(float); break;
		case ONcNet_Update_aimingUD:				size = sizeof(float); break;
		case ONcNet_Update_buttonIsDown:			size = sizeof(LItButtonBits); break;
		case ONcNet_Update_aimingTarget:			size = sizeof(M3tPoint3D); break;
		case ONcNet_Update_aimingVector:			size = sizeof(M3tVector3D); break;
		default:									UUmAssert(!"unknown datatype"); break;
	}
	
	return size;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
ONiNet_Buffer_AddData(
	ONtNet_BufferData		*ioBufferData,
	ONtNet_DataType			inDataType,
	UUtUns8					*inData,
	UUtUns16				inDataLength)
{
	UUtUns16				index;
	
	UUmAssert(ioBufferData->datatype_info->num_datatypes < ONcNet_MaxDataTypesPerPacket);
	
	// add the data
	if (inData)
	{
		UUrMemory_MoveFast(inData, ioBufferData->data_ptr, inDataLength);
	
		// advance the data pointer
		ioBufferData->data_ptr += inDataLength;
	}
	
	// add the datatypes
	index = ioBufferData->datatype_info->num_datatypes++;
	ioBufferData->datatypes[index] = inDataType;
}

// ----------------------------------------------------------------------
static UUcInline void
ONiNet_Buffer_Align2Byte(
	ONtNet_BufferData		*ioBufferData)
{
*((UUtUns32*)ioBufferData->data_ptr) = 'PPPP';

	// set the data_ptr at the next 2byte boundary
	ioBufferData->data_ptr = (UUtUns8*)((UUtUns32)(ioBufferData->data_ptr + 1) & ~1);
}

// ----------------------------------------------------------------------
static UUcInline void
ONiNet_Buffer_Align4Byte(
	ONtNet_BufferData		*ioBufferData)
{
*((UUtUns32*)ioBufferData->data_ptr) = 'PPPP';
	
	// set the data_ptr at the next 4byte boundary
	ioBufferData->data_ptr = (UUtUns8*)((UUtUns32)(ioBufferData->data_ptr + 3) & ~3);
}

// ----------------------------------------------------------------------
static UUtUns16
ONiNet_Buffer_Build(
	ONtNet_BufferData		*ioBufferData)
{
	UUtUns16				buffer_size;
	UUtUns16				datatypes_size;
	
	// get the number of bytes used by the datatypes
	datatypes_size = ONiNet_DataTypes_GetSize(ioBufferData->datatype_info);
	
	// calculate the final buffer size
	buffer_size =
		(UUtUns16)(ioBufferData->data_ptr - ioBufferData->data) +
		datatypes_size;
	
	// copy the datatypes to the end of the data
	if (datatypes_size > 0)
	{
		UUrMemory_MoveFast(
			ioBufferData->datatypes,
			ioBufferData->data_ptr,
			datatypes_size);
	}
	
	// set the datatypes offset
	ioBufferData->datatype_info->datatypes_offset =
		ioBufferData->data_ptr - ioBufferData->data;
	
	return buffer_size;
}

// ----------------------------------------------------------------------
static void
ONiNet_Buffer_Init(
	ONtNet_BufferData		*ioBufferData,
	UUtUns8					*inData)
{
	// set the pointer to the data
	ioBufferData->data = inData;
	
	// the datatype_info is the first piece of data in the buffer
	ioBufferData->datatype_info = (ONtNet_DataTypeInfo*)ioBufferData->data;

	// set the data ptr
	ioBufferData->data_ptr = ioBufferData->data + sizeof(ONtNet_DataTypeInfo);
	
	// initialize the datatype_info
	ioBufferData->datatype_info->num_datatypes		= 0;
	ioBufferData->datatype_info->datatypes_offset	= 0;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns16
ONiNet_Generate_Update(
	ONtCharacter			*inCharacter,
	ONtNet_UC_Data			*ioPrevUpdate,
	ONtNet_Update_Character	*outUpdate)
{
	ONtNet_BufferData		buffer_data;
	UUtBool					is_server;
	
	is_server = ONrNet_IsServer();
	
	// ---------------------------------
	// create the minimum sized update
	// ---------------------------------
	// initialize the buffer data
	ONiNet_Buffer_Init(&buffer_data, outUpdate->data);
	
	// initialize the update header
	outUpdate->hdr.character_index	= inCharacter->index;
	outUpdate->hdr.animFrame		= inCharacter->animFrame;
	outUpdate->hdr.position			= inCharacter->physics->position;
	if (inCharacter->animation)
	{
		UUrString_Copy(
			outUpdate->hdr.animName,
			TMrInstance_GetInstanceName(inCharacter->animation),
			ONcMaxInstNameLength);
	}
	else
	{
		outUpdate->hdr.animName[0] = '\0';
	}

	// ---------------------------------
	// add datatype only updates
	// ---------------------------------
	ONiNet_Buffer_Align4Byte(&buffer_data);

	if ((inCharacter->actionFlags & ONcCharacterActionFlag_Fire) &&
		(inCharacter->weapon))
	{
		// turn off the action flag
		inCharacter->actionFlags &= ~ONcCharacterActionFlag_Fire;

		ONiNet_Buffer_AddData(
			&buffer_data,
			ONcNet_Update_fire,
			NULL,
			0);
	}
	
	// ---------------------------------
	// add 1 byte updates
	// ---------------------------------
	ONiNet_Buffer_Align4Byte(&buffer_data);
	
	// check the character's isAiming
	ONmNet_SimpleAdd2(&buffer_data,
		inCharacter->isAiming, ioPrevUpdate->isAiming,
		UUtBool, ONcNet_Update_isAiming);
			
	// check the character's validTarget
	ONmNet_SimpleAdd2(&buffer_data,
		inCharacter->validTarget, ioPrevUpdate->validTarget,
		UUtBool, ONcNet_Update_validTarget);
	
	// ---------------------------------
	// add 2 byte updates
	// ---------------------------------
	ONiNet_Buffer_Align2Byte(&buffer_data);
	
	// check the character's fightFramesRemaining
	ONmNet_SimpleAdd2(&buffer_data,
		inCharacter->fightFramesRemaining, ioPrevUpdate->fightFramesRemaining,
		UUtUns16, ONcNet_Update_fightFramesRemaining);
	
	// ---------------------------------
	// add 4 byte or more updates
	// ---------------------------------
	ONiNet_Buffer_Align4Byte(&buffer_data);
	
	// check the character's facing
	ONmNet_ComplexAdd2(&buffer_data, UUmFloat_CompareEqu,
		inCharacter->facing, ioPrevUpdate->facing,
		float, ONcNet_Update_facing);
	
	// check the character's desired facing
	ONmNet_ComplexAdd2(&buffer_data, UUmFloat_CompareEqu,
		inCharacter->desiredFacing,	ioPrevUpdate->desiredFacing,
		float, ONcNet_Update_desiredFacing);
	
	// check the character's aimingLR
	ONmNet_ComplexAdd2(&buffer_data, UUmFloat_CompareEqu,
		inCharacter->aimingLR, ioPrevUpdate->aimingLR,
		float, ONcNet_Update_aimingLR);
	
	// check the character's aimingUD
	ONmNet_ComplexAdd2(&buffer_data, UUmFloat_CompareEqu,
		inCharacter->aimingUD, ioPrevUpdate->aimingUD,
		float, ONcNet_Update_aimingUD);
	
	// check the character's buttonIsDown
	if (is_server)
	{
		LItButtonBits			buttonIsDown;
		
		buttonIsDown = inCharacter->inputState.buttonIsDown & ~ONcNet_InventoryBits;
		
		ONmNet_SimpleAdd2(&buffer_data,
			buttonIsDown, ioPrevUpdate->buttonIsDown,
			LItButtonBits, ONcNet_Update_buttonIsDown);
	}
	else
	{
		ONmNet_SimpleAdd2(&buffer_data,
			inCharacter->inputState.buttonIsDown, ioPrevUpdate->buttonIsDown,
			LItButtonBits, ONcNet_Update_buttonIsDown);
	}
	
	// check the character's aimingTarget
	ONmNet_ComplexAdd2(&buffer_data, MUmPoint_Compare,
		inCharacter->aimingTarget, ioPrevUpdate->aimingTarget,
		M3tPoint3D, ONcNet_Update_aimingTarget);
	
	// check the character's aimingVector
	ONmNet_ComplexAdd2(&buffer_data, MUmPoint_Compare,
		inCharacter->aimingVector, ioPrevUpdate->aimingVector,
		M3tPoint3D, ONcNet_Update_aimingVector);
	
	// ---------------------------------
	// add group updates
	// ---------------------------------
	ONiNet_Buffer_Align4Byte(&buffer_data);
	
	// check the inAirControls
	if (inCharacter->inAirControl.numFramesInAir > 0)
	{
		ONtNet_inAir		inAir;
		
		inAir.numFramesInAir	= inCharacter->inAirControl.numFramesInAir;
		inAir.flags				= inCharacter->inAirControl.flags;
		inAir.velocity			= inCharacter->inAirControl.velocity;
		
		// add inAir
		ONiNet_Buffer_AddData(
			&buffer_data,
			ONcNet_Update_inAir,
			(UUtUns8*)&inAir,
			sizeof(ONtNet_inAir));

		ONiNet_Buffer_Align4Byte(&buffer_data);
	}
	
	// check the throws
	if (inCharacter->targetThrow)
	{
		ONtNet_throw		throw_data;
		
		UUmAssertReadPtr(inCharacter, sizeof(ONtCharacter));
		UUmAssertReadPtr(inCharacter->throwTarget, sizeof(ONtCharacter));

		throw_data.throwing		= inCharacter->throwing;
		throw_data.throwFrame	= inCharacter->throwTarget->animFrame;
		UUrString_Copy(
			throw_data.throwName,
			TMrInstance_GetInstanceName(inCharacter->targetThrow),
			ONcMaxInstNameLength);
		
		// add throws
		ONiNet_Buffer_AddData(
			&buffer_data,
			ONcNet_Update_throw,
			(UUtUns8*)&throw_data,
			sizeof(ONtNet_throw));

		ONiNet_Buffer_Align4Byte(&buffer_data);
	}
	
	// return the size
	return (UUtUns16)(ONcNet_Update_Character_MinSize + ONiNet_Buffer_Build(&buffer_data));
}

// ----------------------------------------------------------------------
static UUtUns16
ONiNet_Generate_Update_Authoritative(
	ONtCharacter			*inCharacter,
	ONtNet_UC_Data			*ioPrevUpdate,
	ONtNet_Update_Character	*outUpdate)
{
	ONtNet_BufferData		buffer_data;
	UUtBool					is_server;
	
	is_server = ONrNet_IsServer();
	
	// ---------------------------------
	// create the minimum sized update
	// ---------------------------------
	// initialize the buffer data
	ONiNet_Buffer_Init(&buffer_data, outUpdate->data);
	
	// initialize the update header
	outUpdate->hdr.character_index	= inCharacter->index;
	outUpdate->hdr.animFrame		= inCharacter->animFrame;
	outUpdate->hdr.position			= inCharacter->physics->position;
	if (inCharacter->animation)
	{
		UUrString_Copy(
			outUpdate->hdr.animName,
			TMrInstance_GetInstanceName(inCharacter->animation),
			ONcMaxInstNameLength);
	}
	else
	{
		outUpdate->hdr.animName[0] = '\0';
	}

	// ---------------------------------
	// add datatype only updates
	// ---------------------------------
	ONiNet_Buffer_Align4Byte(&buffer_data);

	if ((inCharacter->actionFlags & ONcCharacterActionFlag_Fire) &&
		(inCharacter->weapon))
	{
		// turn off the action flag
		inCharacter->actionFlags &= ~ONcCharacterActionFlag_Fire;

		ONiNet_Buffer_AddData(
			&buffer_data,
			ONcNet_Update_fire,
			NULL,
			0);
	}
	
	// ---------------------------------
	// add 1 byte updates
	// ---------------------------------
	ONiNet_Buffer_Align4Byte(&buffer_data);
	
	if (is_server)
	{
		// check the character's isAiming
		ONmNet_SimpleAdd2(&buffer_data,
			inCharacter->isAiming, ioPrevUpdate->isAiming,
			UUtBool, ONcNet_Update_isAiming);
				
		// check the character's validTarget
		ONmNet_SimpleAdd2(&buffer_data,
			inCharacter->validTarget, ioPrevUpdate->validTarget,
			UUtBool, ONcNet_Update_validTarget);
	}
	
	// ---------------------------------
	// add 2 byte updates
	// ---------------------------------
	ONiNet_Buffer_Align2Byte(&buffer_data);
	
	if (is_server)
	{
		// check the character's fightFramesRemaining
		ONmNet_SimpleAdd2(&buffer_data,
			inCharacter->fightFramesRemaining, ioPrevUpdate->fightFramesRemaining,
			UUtUns16, ONcNet_Update_fightFramesRemaining);
	}
	
	// ---------------------------------
	// add 4 byte or more updates
	// ---------------------------------
	ONiNet_Buffer_Align4Byte(&buffer_data);
	
	// check the character's facing
	ONmNet_ComplexAdd2(&buffer_data, UUmFloat_CompareEqu,
		inCharacter->facing, ioPrevUpdate->facing,
		float, ONcNet_Update_facing);
	
	// check the character's desired facing
	ONmNet_ComplexAdd2(&buffer_data, UUmFloat_CompareEqu,
		inCharacter->desiredFacing,	ioPrevUpdate->desiredFacing,
		float, ONcNet_Update_desiredFacing);
	
	// check the character's aimingLR
	ONmNet_ComplexAdd2(&buffer_data, UUmFloat_CompareEqu,
		inCharacter->aimingLR, ioPrevUpdate->aimingLR,
		float, ONcNet_Update_aimingLR);
	
	// check the character's aimingUD
	ONmNet_ComplexAdd2(&buffer_data, UUmFloat_CompareEqu,
		inCharacter->aimingUD, ioPrevUpdate->aimingUD,
		float, ONcNet_Update_aimingUD);
	
	// check the character's buttonIsDown
	if (is_server)
	{
		LItButtonBits			buttonIsDown;
		
		buttonIsDown = inCharacter->inputState.buttonIsDown & ~ONcNet_InventoryBits;
		
		ONmNet_SimpleAdd2(&buffer_data,
			buttonIsDown, ioPrevUpdate->buttonIsDown,
			LItButtonBits, ONcNet_Update_buttonIsDown);
	}
	else
	{
		ONmNet_SimpleAdd2(&buffer_data,
			inCharacter->inputState.buttonIsDown, ioPrevUpdate->buttonIsDown,
			LItButtonBits, ONcNet_Update_buttonIsDown);
	}
	
	if (is_server)
	{
		// check the character's aimingTarget
		ONmNet_ComplexAdd2(&buffer_data, MUmPoint_Compare,
			inCharacter->aimingTarget, ioPrevUpdate->aimingTarget,
			M3tPoint3D, ONcNet_Update_aimingTarget);
		
		// check the character's aimingVector
		ONmNet_ComplexAdd2(&buffer_data, MUmPoint_Compare,
			inCharacter->aimingVector, ioPrevUpdate->aimingVector,
			M3tPoint3D, ONcNet_Update_aimingVector);
	}
	
	// ---------------------------------
	// add group updates
	// ---------------------------------
	ONiNet_Buffer_Align4Byte(&buffer_data);

	if (is_server)
	{
		// check the inAirControls
		if (inCharacter->inAirControl.numFramesInAir > 0)
		{
			ONtNet_inAir		inAir;
			
			inAir.numFramesInAir	= inCharacter->inAirControl.numFramesInAir;
			inAir.flags				= inCharacter->inAirControl.flags;
			inAir.velocity			= inCharacter->inAirControl.velocity;
			
			// add inAir
			ONiNet_Buffer_AddData(
				&buffer_data,
				ONcNet_Update_inAir,
				(UUtUns8*)&inAir,
				sizeof(ONtNet_inAir));
	
			ONiNet_Buffer_Align4Byte(&buffer_data);
		}
		
		// check the throws
		if (inCharacter->targetThrow)
		{
			ONtNet_throw		throw_data;
			
			UUmAssertReadPtr(inCharacter, sizeof(ONtCharacter));
			UUmAssertReadPtr(inCharacter->throwTarget, sizeof(ONtCharacter));
	
			throw_data.throwing		= inCharacter->throwing;
			throw_data.throwFrame	= inCharacter->throwTarget->animFrame;
			UUrString_Copy(
				throw_data.throwName,
				TMrInstance_GetInstanceName(inCharacter->targetThrow),
				ONcMaxInstNameLength);
			
			// add throws
			ONiNet_Buffer_AddData(
				&buffer_data,
				ONcNet_Update_throw,
				(UUtUns8*)&throw_data,
				sizeof(ONtNet_throw));
	
			ONiNet_Buffer_Align4Byte(&buffer_data);
		}
	}	

	// return the size
	return (UUtUns16)(ONcNet_Update_Character_MinSize + ONiNet_Buffer_Build(&buffer_data));
}

// ----------------------------------------------------------------------
static void
ONiNet_Skip_Update(
	ONtNet_ProcessGroup		*ioProcessGroup,
	ONtNet_Update_Character	*ioUpdate)
{
	ONtNet_DataTypeInfo		*datatype_info;
	UUtUns16				skip_bytes;
	
	// get a pointer to the datatype info
	datatype_info = (ONtNet_DataTypeInfo*)ioUpdate->data;
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_2Byte(&datatype_info->num_datatypes);
		UUrSwap_2Byte(&datatype_info->datatypes_offset);
	}
	
	// calculate the number of bytes that are in the update
	skip_bytes = (datatype_info->datatypes_offset + datatype_info->num_datatypes + 3) & ~3;
	
	// advance the data by the number of bytes processed
	ONiNet_DataAdvance(ioProcessGroup, skip_bytes);
}

// ----------------------------------------------------------------------
static void
ONiNet_Process_Update(
	ONtNet_ProcessGroup		*ioProcessGroup,
	ONtNet_Update_Character	*ioUpdate)
{
	UUtError				error;
	
	ONtNet_DataTypeInfo		*datatype_info;
	UUtUns8					*data_ptr;
	ONtNet_DataType			*datatypes;
	UUtUns16				i;
	ONtNet_DataType			prev_datatype;
	UUtBool					swap_it;
	UUtUns16				bytes_processed;
	
	ONtCharacter			*character;
	ONtNet_inAir			*inAir;
	ONtNet_throw			*throw_data;
	
	TRtAnimation			*animation;
	const TRtAnimation		*prev_character_animation;
		
	// get a pointer to the character
	character = &ioProcessGroup->common->character_list[ioUpdate->hdr.character_index];
	if (!(character->flags & ONcCharacterFlag_InUse) ||
		(character->physics == NULL))
	{
		ONiNet_Skip_Update(ioProcessGroup, ioUpdate);
		return;
	}
	
	swap_it		= ioProcessGroup->swap_it;
	inAir		= NULL;
	throw_data	= NULL;
	prev_character_animation = character->animation;

	// get a pointer to the datatype info
	datatype_info = (ONtNet_DataTypeInfo*)ioUpdate->data;
	if (swap_it)
	{
		UUrSwap_2Byte(&datatype_info->num_datatypes);
		UUrSwap_2Byte(&datatype_info->datatypes_offset);
	}
	
	// set the other pointers
	data_ptr = ioUpdate->data + sizeof(ONtNet_DataTypeInfo);
	datatypes = ioUpdate->data + datatype_info->datatypes_offset;
	
	// set the character position
	character->physics->position = character->location = ioUpdate->hdr.position;
	
	// process the data
	prev_datatype = ONcNone;
	for (i = 0; i < datatype_info->num_datatypes; i++)
	{
		ONtNet_DataType		datatype;
		UUtUns16			prev_datatype_size;
		UUtUns16			datatype_size;
		
		datatype = datatypes[i];
		
		// align the data_ptr
		prev_datatype_size = ONiNet_DataType_GetDataSize(prev_datatype);
		if (prev_datatype_size == 1)
		{
			datatype_size = ONiNet_DataType_GetDataSize(datatype);

			if (datatype_size == 2)
			{
				data_ptr = (UUtUns8*)(((UUtUns32)data_ptr + 1) & ~1);
			}
			else if (datatype_size >= 4)
			{
				data_ptr = (UUtUns8*)(((UUtUns32)data_ptr + 3) & ~3);
			}
		}
		else if (prev_datatype_size == 2)
		{
			datatype_size = ONiNet_DataType_GetDataSize(datatype);
	
			if (datatype_size != 2)
			{
				data_ptr = (UUtUns8*)(((UUtUns32)data_ptr + 3) & ~3);
			}
		}
		
		// process the datatype
		switch (datatype)
		{
			case ONcNet_Update_isAiming:
				ONmNet_Update2(UUtBool, character->isAiming, data_ptr);
			break;
			
			case ONcNet_Update_validTarget:
				ONmNet_Update2(UUtBool, character->validTarget, data_ptr);
			break;
			
			case ONcNet_Update_fightFramesRemaining:
				ONmNet_SwapUpdate2Byte(UUtUns16, character->fightFramesRemaining, data_ptr, swap_it);
			break;
			
			case ONcNet_Update_fire:
				character->pleaseFire = UUcTrue;
			break;
			
			case ONcNet_Update_throw:
				throw_data = (ONtNet_throw*)data_ptr;
				data_ptr += sizeof(ONtNet_throw);
				
				if (swap_it)
				{
					UUrSwap_2Byte(&throw_data->throwing);
					UUrSwap_2Byte(&throw_data->throwFrame);
				}
			break;
			
			case ONcNet_Update_inAir:
				inAir = (ONtNet_inAir*)data_ptr;
				data_ptr += sizeof(ONtNet_inAir);
				
				if (swap_it)
				{
					UUrSwap_2Byte(&inAir->numFramesInAir);
					UUrSwap_2Byte(&inAir->flags);
					MUmPoint3D_Swap(inAir->velocity);
				}
			break;
			
			case ONcNet_Update_facing:
				ONmNet_SwapUpdate4Byte(float, character->facing, data_ptr, swap_it);
			break;
			
			case ONcNet_Update_desiredFacing:
				ONmNet_SwapUpdate4Byte(float, character->desiredFacing, data_ptr, swap_it);
			break;
			
			case ONcNet_Update_aimingLR:
				ONmNet_SwapUpdate4Byte(float, character->aimingLR, data_ptr, swap_it);
			break;
			
			case ONcNet_Update_aimingUD:
				ONmNet_SwapUpdate4Byte(float, character->aimingUD, data_ptr, swap_it);
			break;
			
			case ONcNet_Update_buttonIsDown:
			{
				LItButtonBits			*button_bits;
				
				button_bits = (LItButtonBits*)data_ptr;
				data_ptr += sizeof(LItButtonBits);
				
				if (swap_it)
				{
					UUrSwap_8Byte(button_bits);
				}
				
				ONrInput_Update_Keys(&character->inputState, *button_bits);
			}
			break;
			
			case ONcNet_Update_aimingTarget:
				ONmNet_SwapUpdateFunc(M3tPoint3D, character->aimingTarget, data_ptr, swap_it, MUmPoint3D_Swap);
			break;
			
			case ONcNet_Update_aimingVector:
				ONmNet_SwapUpdateFunc(M3tVector3D, character->aimingVector, data_ptr, swap_it, MUmPoint3D_Swap);
			break;
			
			default: break;
		}
		
		prev_datatype = datatype;
	}
	
	// handle inAir
	if (inAir)
	{
		character->inAirControl.numFramesInAir = inAir->numFramesInAir;
		character->inAirControl.flags = inAir->flags;
		character->inAirControl.velocity = inAir->velocity;
	}
	
	if (!(character->flags & ONcCharacterFlag_BeingThrown) &&
		(ioUpdate->hdr.animName[0] != '\0'))
	{
		// get a pointer to the animation
		error =
			TMrInstance_GetDataPtr(
				TRcTemplate_Animation,
				ioUpdate->hdr.animName,
				&animation);
		if (error == UUcError_None)
		{
			UUtUns16	num_frames;
			UUtBool		updateAnimation = UUcTrue;
			
			// if the character is dead, make sure this animation is appropriate for death
			if (character->flags & ONcCharacterFlag_Dead)
			{
				TRtAnimState curToState = TRrAnimation_GetTo(character->animation);
				TRtAnimState newToState = TRrAnimation_GetTo(animation);
				
				// if we are currently heading towards fallen and the new animation would not
				// then this is a better animation to run when we are dead
				if ((ONrAnimState_IsFallen(curToState)) &&
					(!ONrAnimState_IsFallen(newToState)))
				{
					updateAnimation = UUcFalse;
				}
			}
			
			if ((updateAnimation) && (character->animation == animation))
			{
				UUtInt32	oldFrame = character->animFrame;
				UUtInt32	newFrame = ioUpdate->hdr.animFrame;
				
				if (UUmABS(oldFrame - newFrame) < 2)
				{
					updateAnimation = UUcFalse;
				}
			}
			
			if (updateAnimation)
			{
				// set the characters animation
				ONrCharacter_SetAnimationInternal(
					character,
					TRrAnimation_GetFrom(character->animation),
					ONcAnimType_None,
					animation);
			}
			
			num_frames = TRrAnimation_GetDuration(character->animation);

			if (ioUpdate->hdr.animFrame >= num_frames)
			{
				character->animFrame = num_frames - 1;
			}
			else
			{
				character->animFrame = ioUpdate->hdr.animFrame;
			}
		}
	}
	
	// handle throwing
	if (throw_data)
	{
		if ((throw_data->throwing != 0xFFFF) &&
			(throw_data->throwing != character->throwing) &&
			(throw_data->throwFrame < 10))
		{
			TRtAnimation	*throw_animation;
			
			// get the animation
			error =
				TMrInstance_GetDataPtr(
					TRcTemplate_Animation,
					throw_data->throwName,
					&throw_animation);
			if (error != UUcError_None) return;
			
			// set the throw target
			character->throwTarget = &ioProcessGroup->common->character_list[throw_data->throwing];
			
			if ((character->throwTarget->animation != throw_animation) &&
				(prev_character_animation != animation) &&
				!(character->throwTarget->flags & ONcCharacterFlag_BeingThrown))
			{
				// set the throw variables
				character->targetThrow	= throw_animation;
				character->throwing		= throw_data->throwing;
				
				// run the throw
				ONrCharacter_NewAnimationHook(character);
				if (character->throwTarget)
				{
					character->throwTarget->animFrame += 2;
					character->throwTarget->thrownBy = character->index;
				}
			}
		}
	}

	// advance the data by the number of bytes processed
	bytes_processed = ONiNet_DataTypes_GetSize(datatype_info) + (data_ptr - ioUpdate->data);
	ONiNet_DataAdvance(ioProcessGroup, bytes_processed);
}

// ----------------------------------------------------------------------
static void
ONiNet_Process_Update_Authoritative(
	ONtNet_ProcessGroup		*ioProcessGroup,
	ONtNet_Update_Character	*ioUpdate)
{
	UUtError				error;
	
	ONtNet_DataTypeInfo		*datatype_info;
	UUtUns8					*data_ptr;
	ONtNet_DataType			*datatypes;
	UUtUns16				i;
	ONtNet_DataType			prev_datatype;
	UUtBool					swap_it;
	UUtUns16				bytes_processed;
	
	ONtCharacter			*character;
	ONtNet_inAir			*inAir;
	ONtNet_throw			*throw_data;
	
	TRtAnimation			*animation;
	const TRtAnimation		*prev_character_animation;
	
	UUtBool					is_server;
	
	is_server = ONrNet_IsServer();
	
	// get a pointer to the character
	character = &ioProcessGroup->common->character_list[ioUpdate->hdr.character_index];
	if (!(character->flags & ONcCharacterFlag_InUse) ||
		(character->physics == NULL))
	{
		ONiNet_Skip_Update(ioProcessGroup, ioUpdate);
		return;
	}
	
	swap_it		= ioProcessGroup->swap_it;
	inAir		= NULL;
	throw_data	= NULL;
	prev_character_animation = character->animation;

	// get a pointer to the datatype info
	datatype_info = (ONtNet_DataTypeInfo*)ioUpdate->data;
	if (swap_it)
	{
		UUrSwap_2Byte(&datatype_info->num_datatypes);
		UUrSwap_2Byte(&datatype_info->datatypes_offset);
	}
	
	// set the other pointers
	data_ptr = ioUpdate->data + sizeof(ONtNet_DataTypeInfo);
	datatypes = ioUpdate->data + datatype_info->datatypes_offset;
	
	// set the character position
	character->physics->position = character->location = ioUpdate->hdr.position;
	
	// process the data
	prev_datatype = ONcNone;
	for (i = 0; i < datatype_info->num_datatypes; i++)
	{
		ONtNet_DataType		datatype;
		UUtUns16			prev_datatype_size;
		UUtUns16			datatype_size;
		
		datatype = datatypes[i];
		
		// align the data_ptr
		prev_datatype_size = ONiNet_DataType_GetDataSize(prev_datatype);
		if (prev_datatype_size == 1)
		{
			datatype_size = ONiNet_DataType_GetDataSize(datatype);

			if (datatype_size == 2)
			{
				data_ptr = (UUtUns8*)(((UUtUns32)data_ptr + 1) & ~1);
			}
			else if (datatype_size >= 4)
			{
				data_ptr = (UUtUns8*)(((UUtUns32)data_ptr + 3) & ~3);
			}
		}
		else if (prev_datatype_size == 2)
		{
			datatype_size = ONiNet_DataType_GetDataSize(datatype);
	
			if (datatype_size != 2)
			{
				data_ptr = (UUtUns8*)(((UUtUns32)data_ptr + 3) & ~3);
			}
		}
		
		// process the datatype
		switch (datatype)
		{
			case ONcNet_Update_isAiming:
				ONmNet_Update2(UUtBool, character->isAiming, data_ptr);
			break;
			
			case ONcNet_Update_validTarget:
				ONmNet_Update2(UUtBool, character->validTarget, data_ptr);
			break;
			
			case ONcNet_Update_fightFramesRemaining:
				ONmNet_SwapUpdate2Byte(UUtUns16, character->fightFramesRemaining, data_ptr, swap_it);
			break;
			
			case ONcNet_Update_fire:
				character->pleaseFire = UUcTrue;
			break;
			
			case ONcNet_Update_throw:
				throw_data = (ONtNet_throw*)data_ptr;
				data_ptr += sizeof(ONtNet_throw);
				
				if (swap_it)
				{
					UUrSwap_2Byte(&throw_data->throwing);
					UUrSwap_2Byte(&throw_data->throwFrame);
				}
			break;
			
			case ONcNet_Update_inAir:
				inAir = (ONtNet_inAir*)data_ptr;
				data_ptr += sizeof(ONtNet_inAir);
				
				if (swap_it)
				{
					UUrSwap_2Byte(&inAir->numFramesInAir);
					UUrSwap_2Byte(&inAir->flags);
					MUmPoint3D_Swap(inAir->velocity);
				}
			break;
			
			case ONcNet_Update_facing:
				ONmNet_SwapUpdate4Byte(float, character->facing, data_ptr, swap_it);
			break;
			
			case ONcNet_Update_desiredFacing:
				ONmNet_SwapUpdate4Byte(float, character->desiredFacing, data_ptr, swap_it);
			break;
			
			case ONcNet_Update_aimingLR:
				ONmNet_SwapUpdate4Byte(float, character->aimingLR, data_ptr, swap_it);
			break;
			
			case ONcNet_Update_aimingUD:
				ONmNet_SwapUpdate4Byte(float, character->aimingUD, data_ptr, swap_it);
			break;
			
			case ONcNet_Update_buttonIsDown:
			{
				LItButtonBits			*button_bits;
				
				button_bits = (LItButtonBits*)data_ptr;
				data_ptr += sizeof(LItButtonBits);
				
				if (swap_it)
				{
					UUrSwap_8Byte(button_bits);
				}
				
				ONrInput_Update_Keys(&character->inputState, *button_bits);
			}
			break;
			
			case ONcNet_Update_aimingTarget:
				ONmNet_SwapUpdateFunc(M3tPoint3D, character->aimingTarget, data_ptr, swap_it, MUmPoint3D_Swap);
			break;
			
			case ONcNet_Update_aimingVector:
				ONmNet_SwapUpdateFunc(M3tVector3D, character->aimingVector, data_ptr, swap_it, MUmPoint3D_Swap);
			break;
			
			default: break;
		}
		
		prev_datatype = datatype;
	}
	
	// handle inAir
	if (inAir)
	{
		character->inAirControl.numFramesInAir = inAir->numFramesInAir;
		character->inAirControl.flags = inAir->flags;
		character->inAirControl.velocity = inAir->velocity;
	}
	
	if ((is_server == UUcFalse) &&
	 	(!(character->flags & ONcCharacterFlag_BeingThrown)) &&
		(ioUpdate->hdr.animName[0] != '\0'))
	{
		// get a pointer to the animation
		error =
			TMrInstance_GetDataPtr(
				TRcTemplate_Animation,
				ioUpdate->hdr.animName,
				&animation);
		if (error == UUcError_None)
		{
			UUtUns16	num_frames;
			UUtBool		updateAnimation = UUcTrue;
			
			// if the character is dead, make sure this animation is appropriate for death
			if (character->flags & ONcCharacterFlag_Dead)
			{
				TRtAnimState curToState = TRrAnimation_GetTo(character->animation);
				TRtAnimState newToState = TRrAnimation_GetTo(animation);
				
				// if we are currently heading towards fallen and the new animation would not
				// then this is a better animation to run when we are dead
				if ((ONrAnimState_IsFallen(curToState)) &&
					(!ONrAnimState_IsFallen(newToState)))
				{
					updateAnimation = UUcFalse;
				}
			}
			
			if ((updateAnimation) && (character->animation == animation))
			{
				UUtInt32	oldFrame = character->animFrame;
				UUtInt32	newFrame = ioUpdate->hdr.animFrame;
				
				if (UUmABS(oldFrame - newFrame) < 2)
				{
					updateAnimation = UUcFalse;
				}
			}
			
			if (updateAnimation)
			{
				// set the characters animation
				ONrCharacter_SetAnimationInternal(
					character,
					TRrAnimation_GetFrom(character->animation),
					ONcAnimType_None,
					animation);
			}
			
			num_frames = TRrAnimation_GetDuration(character->animation);

			if (ioUpdate->hdr.animFrame >= num_frames)
			{
				character->animFrame = num_frames - 1;
			}
			else
			{
				character->animFrame = ioUpdate->hdr.animFrame;
			}
		}
	}
	
	// handle throwing
	if (throw_data)
	{
		if ((throw_data->throwing != 0xFFFF) &&
			(throw_data->throwing != character->throwing) &&
			(throw_data->throwFrame < 10))
		{
			TRtAnimation	*throw_animation;
			
			// get the animation
			error =
				TMrInstance_GetDataPtr(
					TRcTemplate_Animation,
					throw_data->throwName,
					&throw_animation);
			if (error != UUcError_None) return;
			
			// set the throw target
			character->throwTarget = &ioProcessGroup->common->character_list[throw_data->throwing];
			
			if ((character->throwTarget->animation != throw_animation) &&
				(prev_character_animation != animation) &&
				!(character->throwTarget->flags & ONcCharacterFlag_BeingThrown))
			{
				// set the throw variables
				character->targetThrow	= throw_animation;
				character->throwing		= throw_data->throwing;
				
				// run the throw
				ONrCharacter_NewAnimationHook(character);
				if (character->throwTarget)
				{
					character->throwTarget->animFrame += 2;
					character->throwTarget->thrownBy = character->index;
				}
			}
		}
	}

	// advance the data by the number of bytes processed
	bytes_processed = ONiNet_DataTypes_GetSize(datatype_info) + (data_ptr - ioUpdate->data);
	ONiNet_DataAdvance(ioProcessGroup, bytes_processed);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns16
ONiNet_Generate_Critical_Update(
	const ONtCharacter		*inCharacter,
	UUtUns32				inUpdateFlags,
	ONtNet_Critical_Update	*outCUpdate)
{
	ONtNet_BufferData		buffer_data;
	UUtBool					is_server;
	
	// initialize the buffer data
	ONiNet_Buffer_Init(&buffer_data, outCUpdate->data);
	
	// initialize the critical update header
	outCUpdate->hdr.character_index		= inCharacter->index;
	
	is_server							= ONrNet_IsServer();
	
	// check the character's health only on the server
	if ((is_server) &&
		(inUpdateFlags & ONcCharacterUpdateFlag_HitPoints))
	{
		ONiNet_Buffer_AddData(
			&buffer_data,
			ONcNet_CU_Health,
			(UUtUns8*)&inCharacter->hitPoints,
			sizeof(UUtUns32)); 
	}
	
	// check the character's numKills
	if ((is_server) && (inUpdateFlags & ONcCharacterUpdateFlag_NumKills))
	{
		ONiNet_Buffer_AddData(
			&buffer_data,
			ONcNet_CU_NumKills,
			(UUtUns8*)&inCharacter->numKills,
			sizeof(UUtUns32)); 
	}
	
	// check the character's damageInflicted
	if ((is_server) && (inUpdateFlags & ONcCharacterUpdateFlag_DamageInflicted))
	{
		ONiNet_Buffer_AddData(
			&buffer_data,
			ONcNet_CU_DamageInflicted,
			(UUtUns8*)&inCharacter->damageInflicted,
			sizeof(UUtUns32)); 
	}
	
	// check the character's inventory
	if ((is_server) && (inUpdateFlags & ONcCharacterUpdateFlag_Inventory))
	{
		ONtNet_Inventory		inventory;
		UUtUns32				i;
		
		for (i = 0; i < WPcMaxSlots; i++)
		{
			if (inCharacter->inventory.weapons[i].weapon)
			{
				inventory.weapons[i] = WPrGetIndex(inCharacter->inventory.weapons[i].weapon);
			}
			else
			{
				inventory.weapons[i] = 0xFFFF;
			}
		}

		inventory.ammo = inCharacter->inventory.ammo;
		inventory.hypo = inCharacter->inventory.hypo;
		inventory.cell = inCharacter->inventory.cell;
		
		// add weapon index
		ONiNet_Buffer_AddData(
			&buffer_data,
			ONcNet_CU_Inventory,
			(UUtUns8*)&inventory,
			sizeof(ONtNet_Inventory));
	}
	
	// check the character's name
	if (inUpdateFlags & ONcCharacterUpdateFlag_Name)
	{
		ONiNet_Buffer_AddData(
			&buffer_data,
			ONcNet_CU_Name,
			(UUtUns8*)inCharacter->player_name,
			ONcMaxPlayerNameLength);
	}
	
	// check the character's class
	if (inUpdateFlags & ONcCharacterUpdateFlag_Class)
	{
		ONiNet_Buffer_AddData(
			&buffer_data,
			ONcNet_CU_Class,
			(UUtUns8*)TMrInstance_GetInstanceName(inCharacter->characterClass),
			AIcMaxClassNameLen);
	}
	
	// check for spawn
	if ((is_server) && (inUpdateFlags & ONcCharacterUpdateFlag_Spawn))
	{
		ONtNet_Info_Spawn		spawn_info;
		
		spawn_info.facing		= inCharacter->facing;
		spawn_info.position		= inCharacter->physics->position;
		
		ONiNet_Buffer_AddData(
			&buffer_data,
			ONcNet_CU_Spawn,
			(UUtUns8*)&spawn_info,
			sizeof(ONtNet_Info_Spawn));
	}
	
	// check for character gone
	if ((is_server) && (inUpdateFlags & ONcCharacterUpdateFlag_Gone))
	{
		ONiNet_Buffer_AddData(
			&buffer_data,
			ONcNet_CU_Gone,
			NULL,
			0);
	}
	
	// return the size
	return (UUtUns16)(ONcNet_CUpdate_MinSize + ONiNet_Buffer_Build(&buffer_data));
}

// ----------------------------------------------------------------------
static void
ONiNet_Skip_Critical_Update(
	ONtNet_ProcessGroup		*ioProcessGroup,
	ONtNet_Critical_Update	*ioCUpdate)
{
	ONtNet_DataTypeInfo		*datatype_info;
	UUtUns16				skip_bytes;
	
	// get a pointer to the datatype info
	datatype_info = (ONtNet_DataTypeInfo*)ioCUpdate->data;
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_2Byte(&datatype_info->num_datatypes);
		UUrSwap_2Byte(&datatype_info->datatypes_offset);
	}
	
	// calculate the number of bytes that are in the update
	skip_bytes = (datatype_info->datatypes_offset + datatype_info->num_datatypes + 3) & ~3;
	
	// advance the data by the number of bytes processed
	ONiNet_DataAdvance(ioProcessGroup, skip_bytes);
}

// ----------------------------------------------------------------------
static void
ONiNet_Process_Critical_Update(
	ONtNet_ProcessGroup		*ioProcessGroup,
	ONtNet_Critical_Update	*ioCUpdate,
	const ONtCharacter		*inClientCharacter)
{
	UUtError				error;
	
	ONtNet_DataTypeInfo		*datatype_info;
	UUtUns8					*data_ptr;
	ONtNet_DataType			*datatypes;
	UUtUns16				i;
	ONtNet_DataType			prev_datatype;
	UUtBool					swap_it;
	UUtBool					is_server;
	UUtUns16				bytes_processed;
	
	ONtCharacter			*character;
	
	// get a pointer to the character
	character = &ioProcessGroup->common->character_list[ioCUpdate->hdr.character_index];
	if (!(character->flags & ONcCharacterFlag_InUse))
	{
		ONiNet_Skip_Critical_Update(ioProcessGroup, ioCUpdate);
		return;
	}
	
	swap_it		= ioProcessGroup->swap_it;
	is_server	= ONrNet_IsServer();
	
	// get a pointer to the datatype info
	datatype_info = (ONtNet_DataTypeInfo*)ioCUpdate->data;
	if (swap_it)
	{
		UUrSwap_2Byte(&datatype_info->num_datatypes);
		UUrSwap_2Byte(&datatype_info->datatypes_offset);
	}
	
	// set the other pointers
	data_ptr = ioCUpdate->data + sizeof(ONtNet_DataTypeInfo);
	datatypes = ioCUpdate->data + datatype_info->datatypes_offset;
	
	// process the data
	prev_datatype = ONcNone;
	for (i = 0; i < datatype_info->num_datatypes; i++)
	{
		ONtNet_DataType		datatype;
		
		datatype = datatypes[i];
		
		// process the datatype
		switch (datatype)
		{
			case ONcNet_CU_Health:
				// servers do not process health updates because that
				// would allow client cheating
				if (is_server)
				{
					data_ptr += sizeof(UUtUns32);
				}
				else
				{
					ONmNet_SwapUpdate4Byte(UUtUns32, character->hitPoints, data_ptr, swap_it);
					
					// even though the hitPoints field was already updated, call
					// this function to do some extra processing
					ONrCharacter_SetHitPoints(character, character->hitPoints);
				}
			break;
			
			case ONcNet_CU_NumKills:
				if (is_server)
				{
					data_ptr += sizeof(UUtUns32);
				}
				else
				{
					ONmNet_SwapUpdate4Byte(UUtUns32, character->numKills, data_ptr, swap_it);
				}
			break;
			
			case ONcNet_CU_DamageInflicted:
				if (is_server)
				{
					data_ptr += sizeof(UUtUns32);
				}
				else
				{
					ONmNet_SwapUpdate4Byte(UUtUns32, character->damageInflicted, data_ptr, swap_it);
				}
			break;
			
			case ONcNet_CU_Inventory:
				// servers do not process inventory updates
				if (is_server)
				{
					data_ptr += sizeof(ONtNet_Inventory);
				}
				else
				{
					ONtNet_Inventory		inventory;
					UUtUns32				i;
					
//COrConsole_Printf("Critical Update: Inventory");

					inventory = *(ONtNet_Inventory*)data_ptr;
					data_ptr += sizeof(ONtNet_Inventory);
					
					if (swap_it)
					{
						for (i = 0; i < WPcMaxSlots; i++)
						{
							UUrSwap_2Byte(&inventory.weapons[i]);
						}
						
						UUrSwap_2Byte(&inventory.ammo);
						UUrSwap_2Byte(&inventory.hypo);
						UUrSwap_2Byte(&inventory.cell);
					}
					
					// get rid of the weapon currently being held by the character if any
					ONrCharacter_DropWeapon(character, UUcFalse, WPcPrimarySlot, UUcFalse);
					
					// empty the inventory slots of the character
					for (i = 0; i < WPcMaxSlots; i++)
					{
						if (character->inventory.weapons[i].weapon)
						{
							WPrRelease(character->inventory.weapons[i].weapon, UUcFalse);
							character->inventory.weapons[i].weapon = NULL;
						}
					}
					
					// fill the inventory slots of the character
					for (i = 0; i < WPcMaxSlots; i++)
					{
						if (inventory.weapons[i] != 0xFFFF)
						{
							// put the weapon in the inventory slot
							character->inventory.weapons[i].weapon = WPrGetWeapon(inventory.weapons[i]);
							
							// assign the weapon to the character
							WPrAssign(character->inventory.weapons[i].weapon, character);
						}
					}
					
					// set the character weapon matrix to the identity matrix.  This prevents hitting
					// an assert in the weapon code when the matrix is all 0.0f and their is a weapon
					// in a slot other than the primary slot.
					MUrMatrix_Identity(&character->matricies[ONcWeapon_Index]);
					
					// make the character use the weapon in WPcPrimarySlot
					if (WPrInUse(character->inventory.weapons[WPcPrimarySlot].weapon))
					{
						ONrCharacter_UseWeapon(character, character->inventory.weapons[WPcPrimarySlot].weapon);
					}
					
					character->inventory.ammo = inventory.ammo;
					character->inventory.hypo = inventory.hypo;
					character->inventory.cell = inventory.cell;
				}
			break;
			
			case ONcNet_CU_Name:
			{
				char			*player_name;
				
				player_name = (char*)data_ptr;
				data_ptr += ONcMaxPlayerNameLength;
				
				UUrString_Copy(character->player_name, player_name, ONcMaxPlayerNameLength);
			}
			break;

			case ONcNet_CU_Class:
			{
				char				*characterClassName;
				ONtCharacterClass	*characterClass;
								
				characterClassName = (char*)data_ptr;
				data_ptr += AIcMaxClassNameLen;
				
				// don't update the class of the character of the client on this machine
				if (inClientCharacter == character) break;
				
				error =
					TMrInstance_GetDataPtr(
						TRcTemplate_CharacterClass,
						characterClassName,
						&characterClass);
				if (error != UUcError_None) continue;
				
				// set the class instance of the character
				ONrCharacter_SetCharacterClass(character, characterClass);
			}
			break;
			
			case ONcNet_CU_Spawn:
				// only clients process Spawn updates
				if (is_server == UUcFalse)
				{
					ONtNet_Info_Spawn	*spawn_info;
					
					// get a pointer to the spawn info
					spawn_info = (ONtNet_Info_Spawn*)data_ptr;
					data_ptr += sizeof(ONtNet_Info_Spawn);
					
					if (swap_it)
					{
						UUrSwap_4Byte(&spawn_info->facing);
						MUmPoint3D_Swap(spawn_info->position);
					}
					
					character->facing = character->previousFacing = spawn_info->facing;
					character->physics->position = character->location = spawn_info->position;
					
					// update the character's node
					ONrCharacter_FindOurNode(character);
				}
			break;
			
			case ONcNet_CU_Gone:
				// only clients process Gone updates
				if (is_server == UUcFalse)
				{
					ONrGameState_DeleteCharacter(character);
				}
			break;
			
			default: UUmAssert(!"unknown data type"); break;
		}
		
		prev_datatype = datatype;
	}
	
	// clients clear the character update flags so that a vicious cycle isn't entered
	if (is_server == UUcFalse)
	{
		character->updateFlags = ONcCharacterUpdateFlag_None;
	}
	
	// advance the data by the number of bytes processed
	bytes_processed = ONiNet_DataTypes_GetSize(datatype_info) + (data_ptr - ioCUpdate->data);
	ONiNet_DataAdvance(ioProcessGroup, bytes_processed);
	
#if defined(DEBUGGING) && (DEBUGGING > 0)
	// try to find spawn bugs	
	if ((character->hitPoints > 0) &&
		(character->flags & ONcCharacterFlag_Dead))
	{
		COrConsole_Printf("Character %d is suppose to be dead", character->index);
	}
#endif
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns16
ONiNet_Generate_Weapon_Update(
	WPtWeapon				*ioWeapon,
	UUtUns16				inWeaponIndex,
	UUtUns16				inUpdateFlags,
	ONtNet_Weapon_Update	*outWeaponUpdate)
{
	ONtNet_BufferData		buffer_data;

	// initialize the buffer data
	ONiNet_Buffer_Init(&buffer_data, outWeaponUpdate->data);
	
	// initialize the weapon update header
	outWeaponUpdate->hdr.weapon_index = inWeaponIndex;
	outWeaponUpdate->hdr.pad = 0;

	// ---------------------------------
	// the weapon has to be created before any other datatypes are processed
	// ---------------------------------
	if (inUpdateFlags & WPcWeaponUpdateFlag_Create)
	{
		ONtCharacter		*owner;
		
		// add 32 bytes
		ONiNet_Buffer_AddData(
			&buffer_data,
			ONcNet_WU_Create,
			(UUtUns8*)TMrInstance_GetInstanceName(WPrGetClass(ioWeapon)),
			ONcMaxInstNameLength);
		
		// make sure the the owner of the weapon is set
		owner = WPrGetOwner(ioWeapon);
		if (owner)
		{
			owner->updateFlags |= ONcCharacterUpdateFlag_Inventory;
		}
//COrConsole_Printf("Generate Weapon %d", WPrGetIndex(ioWeapon));
	}

	// ---------------------------------
	// add datatype only updates
	// ---------------------------------
	if (inUpdateFlags & WPcWeaponUpdateFlag_Delete)
	{
		// add 0 bytes
		ONiNet_Buffer_AddData(
			&buffer_data,
			ONcNet_WU_Delete,
			NULL,
			0);
	}
		
	// ---------------------------------
	// add multi-byte updates
	// ---------------------------------
	if (inUpdateFlags & WPcWeaponUpdateFlag_Position)
	{
		M3tPoint3D		position;
		
		WPrGetPosition(ioWeapon, &position);
		
		// add 12 bytes
		ONiNet_Buffer_AddData(
			&buffer_data,
			ONcNet_WU_Position,
			(UUtUns8*)&position,
			sizeof(M3tPoint3D));
//COrConsole_Printf("Generate Weapon %d Position Update", WPrGetIndex(ioWeapon));
	}
	
	if (inUpdateFlags & WPcWeaponUpdateFlag_Ammo)
	{
		WPtAmmoGroup		ammo_group;
		
		WPrGetAmmoGroup(ioWeapon, &ammo_group);
		
		// add 16 bytes
		ONiNet_Buffer_AddData(
			&buffer_data,
			ONcNet_WU_Ammo,
			(UUtUns8*)&ammo_group,
			sizeof(WPtAmmoGroup));
	}

	// clear the weapon's update flags
	WPrSetUpdateFlags(ioWeapon, WPcWeaponUpdateFlag_None);
	
	// return the size
	return (UUtUns16)(ONcNet_WUpdate_MinSize + ONiNet_Buffer_Build(&buffer_data));
}

// ----------------------------------------------------------------------
static void
ONiNet_Skip_Weapon_Update(
	ONtNet_ProcessGroup		*ioProcessGroup,
	ONtNet_Weapon_Update	*ioWeaponUpdate)
{
	ONtNet_DataTypeInfo		*datatype_info;
	UUtUns16				skip_bytes;
	
	// get a pointer to the datatype info
	datatype_info = (ONtNet_DataTypeInfo*)ioWeaponUpdate->data;
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_2Byte(&datatype_info->num_datatypes);
		UUrSwap_2Byte(&datatype_info->datatypes_offset);
	}
	
	// calculate the number of bytes that are in the update
	skip_bytes = (datatype_info->datatypes_offset + datatype_info->num_datatypes + 3) & ~3;
	
	// advance the data by the number of bytes processed
	ONiNet_DataAdvance(ioProcessGroup, skip_bytes);
}

// ----------------------------------------------------------------------
static void
ONiNet_Process_Weapon_Update(
	ONtNet_ProcessGroup		*ioProcessGroup,
	ONtNet_Weapon_Update	*ioWeaponUpdate)
{
	UUtError				error;
	
	WPtWeapon				*weapon;

	ONtNet_DataTypeInfo		*datatype_info;
	UUtUns8					*data_ptr;
	ONtNet_DataType			*datatypes;
	UUtUns16				i;
	ONtNet_DataType			prev_datatype;
	UUtBool					swap_it;
	UUtUns16				bytes_processed;
	
	// get a pointer to the weapon
	weapon = WPrGetWeapon(ioWeaponUpdate->hdr.weapon_index);
	
	swap_it = ioProcessGroup->swap_it;
	
	// get a pointer to the datatype info
	datatype_info = (ONtNet_DataTypeInfo*)ioWeaponUpdate->data;
	if (swap_it)
	{
		UUrSwap_2Byte(&datatype_info->num_datatypes);
		UUrSwap_2Byte(&datatype_info->datatypes_offset);
	}
	
	// set the other pointers
	data_ptr = ioWeaponUpdate->data + sizeof(ONtNet_DataTypeInfo);
	datatypes = ioWeaponUpdate->data + datatype_info->datatypes_offset;
	
	// process the data
	prev_datatype = ONcNone;
	for (i = 0; i < datatype_info->num_datatypes; i++)
	{
		ONtNet_DataType		datatype;
		
		datatype = datatypes[i];
		
		// process the datatype
		switch (datatype)
		{
			case ONcNet_WU_Create:
			{
				WPtWeaponClass				*weapon_class;
				char						*weapon_class_name;
				
				weapon_class_name = (char*)data_ptr;
				data_ptr += ONcMaxInstNameLength;
				
				error =
					TMrInstance_GetDataPtr(
						WPcTemplate_WeaponClass,
						weapon_class_name,
						&weapon_class);
				if (error != UUcError_None) { break; }
				
				WPrInitAtIndex(weapon, ioWeaponUpdate->hdr.weapon_index, weapon_class);
//COrConsole_Printf("create weapon %d", ioWeaponUpdate->hdr.weapon_index);
			}
			break;
			
			case ONcNet_WU_Delete:
				WPrDelete(weapon);
			break;
			
			case ONcNet_WU_Ammo:
			{
				WPtAmmoGroup				ammo_group;
				
				// get the ammo group
				ammo_group = *(WPtAmmoGroup*)data_ptr;
				data_ptr += sizeof(WPtAmmoGroup);
				
				// swap and set the ammo group if there is a weapon
				if (swap_it)
				{
					UUtUns16	x;
					for (x=0; x < WPcMaxBarrels; x++)
					{
						UUrSwap_2Byte(&ammo_group.ammo[x]);
					}
				}
				
				// set the ammo amount
				if (WPrInUse(weapon))
				{
					WPrSetAmmoGroup(weapon, &ammo_group);
				}
			}
			break;
			
			case ONcNet_WU_Position:
			{
				M3tPoint3D			position;
				
				ONmNet_SwapUpdateFunc(
					M3tPoint3D,
					position,
					data_ptr,
					swap_it,
					MUmPoint3D_Swap);
				
				if (WPrInUse(weapon))
				{
					WPrSetPosition(weapon, &position);
//COrConsole_Printf("Process Weapon %d Position Update", ioWeaponUpdate->hdr.weapon_index);
				}
			}
			break;
						
			default: UUmAssert(!"unknown data type"); break;
		}
		
		prev_datatype = datatype;
	}
	
	// advance the data by the number of bytes processed
	bytes_processed = ONiNet_DataTypes_GetSize(datatype_info) + (data_ptr - ioWeaponUpdate->data);
	ONiNet_DataAdvance(ioProcessGroup, bytes_processed);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
ONiNet_Packet_AddData(
	ONtNet_PacketData		*ioPacketData,
	const ONtNet_DataType	inDataType,
	const UUtUns8			*inData,
	const UUtUns16			inDataLength)
{
	UUtUns16				new_packet_length;
	UUtUns16				index;
	
//	UUmAssert(ioPacketData->datatype_info->num_datatypes < ONcNet_MaxDataTypesPerPacket);
	
	// return if the maximum number of datatypes has been added
	if (ioPacketData->datatype_info->num_datatypes >= ONcNet_MaxDataTypesPerPacket)
	{
COrConsole_Printf("Max Data Types Per Packet Exceeded");		
		return UUcFalse;
	}
	
	// make sure there is enough room in the packet
	new_packet_length =
		(UUtUns16)(ioPacketData->data_ptr - ioPacketData->packet.packet_data) +
		ONiNet_DataTypes_GetSize(ioPacketData->datatype_info) +
		inDataLength;
	if (new_packet_length > (sizeof(ONtNet_Packet) - sizeof(ONtNet_PacketHeader)))
	{
		return UUcFalse;
	}
	
	// add the data
	if (inData)
	{
		UUrMemory_MoveFast(
			inData,
			ioPacketData->data_ptr,
			inDataLength);
		
		// set data_ptr at next 4byte boundary
		ioPacketData->data_ptr =
			(UUtUns8*)((UUtUns32)(ioPacketData->data_ptr + inDataLength + 3) & ~3);
	}
	
	// add the datatype
	index = ioPacketData->datatype_info->num_datatypes++;
	ioPacketData->datatypes[index] = inDataType;
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
static UUtUns16
ONiNet_Packet_Build(
	ONtNet_PacketData		*ioPacketData)
{
	UUtUns16				packet_size;
		
	// calculate the final packet size of the packet
	packet_size =
		sizeof(ONtNet_PacketHeader) +
		(UUtUns16)(ioPacketData->data_ptr - ioPacketData->packet.packet_data) +
		ONiNet_DataTypes_GetSize(ioPacketData->datatype_info);
	UUmAssert(packet_size < (sizeof(ONtNet_Packet) - sizeof(ONtNet_PacketHeader)));
	
	// copy the datatypes to the end of the data
	UUrMemory_MoveFast(
		ioPacketData->datatypes,
		ioPacketData->data_ptr,
		ONiNet_DataTypes_GetSize(ioPacketData->datatype_info));
	
	// set the datatypes offset
	ioPacketData->datatype_info->datatypes_offset =
		ioPacketData->data_ptr - ioPacketData->packet.packet_data;
	
	return packet_size;
}

// ----------------------------------------------------------------------
void
ONrNet_Packet_Init(
	ONtNet_PacketData		*ioPacketData)
{
	// the datatype_info is the first piece of data in the packet
	ioPacketData->datatype_info = (ONtNet_DataTypeInfo*)ioPacketData->packet.packet_data;
	
	// set the data ptr
	ioPacketData->data_ptr =
		ioPacketData->packet.packet_data +
		sizeof(ONtNet_DataTypeInfo);
	
	// initialize the datatype_info
	ioPacketData->datatype_info->num_datatypes		= 0;
	ioPacketData->datatype_info->datatypes_offset	= 0;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
ONiNet_Client_Packet_Build(
	ONtNet_Client			*ioClient,
	const ONtNet_DataType	inDataType,
	const UUtUns8			*inData,
	const UUtUns16			inDataLength)
{
	UUtBool					status;
	
	// add the data to the packet
	status =
		ONiNet_Packet_AddData(
			&ioClient->packet_data,
			inDataType,
			inData,
			inDataLength);
	if (status == UUcFalse)
	{
		// there is not enough room in the packet.  Send the current data
		// and then add the data
		ONrNet_Client_Packet_Send(ioClient);
		
		status =
			ONiNet_Packet_AddData(
				&ioClient->packet_data,
				inDataType,
				inData,
				inDataLength);
		if (status == UUcFalse)
		{
			UUmAssert(!"should never get here");
		}
	}
}

// ----------------------------------------------------------------------
void
ONrNet_Client_Packet_Send(
	ONtNet_Client			*ioClient)
{
	UUtUns16				packet_length;
	
	// build the final packet to send
	packet_length = ONiNet_Packet_Build(&ioClient->packet_data);
	
	// make sure there is data to send
	if (packet_length == 0) return;

	// initialize the packet header
	ioClient->packet_data.packet.packet_header.packet_swap		= ONcNet_SwapIt;
	ioClient->packet_data.packet.packet_header.packet_number	= ioClient->packet_number++;
	ioClient->packet_data.packet.packet_header.packet_version	= ONcNet_VersionNumber;
	ioClient->packet_data.packet.packet_header.player_index		= ioClient->player_index;
	ioClient->packet_data.packet.packet_header.packet_length	= packet_length;
	
	// send it
	NMrNetContext_WriteData(
		ioClient->client_context,
		&ioClient->server_address, 
		(UUtUns8*)&ioClient->packet_data.packet, 
		packet_length, 
		NMcPacketFlags_None);
	
	ioClient->packets_sent++;
	ioClient->bytes_sent += packet_length;
		
	// there is no more data in the update packet
	ONrNet_Packet_Init(&ioClient->packet_data);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
ONiNet_CS_ACK_Critical_Update(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup,
	UUtUns16				inCharacterIndex)
{
	ONtNet_Ack_CriticalUpdate		ack;
	
	ack.uns16_0 = inCharacterIndex;
	
	// add ack to packet
	ONiNet_Client_Packet_Build(
		ioClient,
		ONcC2S_ACK_Critical_Update,
		(UUtUns8*)&ack,
		sizeof(ONtNet_Ack_CriticalUpdate));
}

// ----------------------------------------------------------------------
static void
ONiNet_CS_ACK_Weapon_Update(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup,
	UUtUns16				inWeaponIndex)
{
	ONtNet_Ack_WeaponUpdate			ack;
	
	ack.uns16_0 = inWeaponIndex;
	
	// add ack to packet
	ONiNet_Client_Packet_Build(
		ioClient,
		ONcC2S_ACK_Weapon_Update,
		(UUtUns8*)&ack,
		sizeof(ONtNet_Ack_CriticalUpdate));
}

// ----------------------------------------------------------------------
void
ONrNet_CS_Heartbeat(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	// add heartbeat to packet
	ONiNet_Client_Packet_Build(
		ioClient,
		ONcC2S_Heartbeat,
		NULL,
		0);
}

// ----------------------------------------------------------------------
void
ONrNet_CS_Request_CharacterCreate(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup,
	UUtUns32				inRequestID)
{
	ONtNet_Request_CharacterCreate		create_request;
	
	// initialize the create request
	create_request.request_id = inRequestID;
	create_request.teamNumber = ioClient->teamNumber;
	UUrString_Copy(
		create_request.playerName,
		ioClient->playerName,
		ONcMaxPlayerNameLength);
	UUrString_Copy(
		create_request.characterClassName,
		ioClient->characterClassName,
		AIcMaxClassNameLen);
	
	// add request to packet
	ONiNet_Client_Packet_Build(
		ioClient,
		ONcC2S_Request_CharacterCreate,
		(UUtUns8*)&create_request,
		sizeof(ONtNet_Request_CharacterCreate));
}

// ----------------------------------------------------------------------
static void
ONiNet_CS_Request_CharacterInfo(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup,
	UUtUns16				inCharacterIndex)
{
	ONtNet_Request_CharacterInfo	info_request;
	ONtCharacter					*character;
	
	// get a pointer to the character
	character = &ioProcessGroup->common->character_list[inCharacterIndex];
	
	// send request only if the character info request hasn't already been made
	if (character->updateFlags & ONcCharacterUpdateFlag_NeedInfo)
	{
		return;
	}
	
	character->updateFlags |= ONcCharacterUpdateFlag_NeedInfo;
	
	// initialize the info request
	info_request.uns16_0 = inCharacterIndex;
	
	// add request to packet
	ONiNet_Client_Packet_Build(
		ioClient,
		ONcC2S_Request_CharacterInfo,
		(UUtUns8*)&info_request,
		sizeof(ONtNet_Request_CharacterInfo));
}

// ----------------------------------------------------------------------
void
ONrNet_CS_Request_Join(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup,
	UUtUns32				inRequestID)
{
	ONtNet_Request_Join		join_request;
	
	// initialize the join request
	join_request.uns32_0 = inRequestID;
	
	// add request to packet
	ONiNet_Client_Packet_Build(
		ioClient,
		ONcC2S_Request_Join,
		(UUtUns8*)&join_request,
		sizeof(ONtNet_Request_Join));
}

// ----------------------------------------------------------------------
void
ONrNet_CS_Request_Spawn(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	// add request to packet
	ONiNet_Client_Packet_Build(
		ioClient,
		ONcC2S_Request_Spawn,
		NULL,
		0);
}

// ----------------------------------------------------------------------
void
ONrNet_CS_Request_Quit(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup,
	UUtUns32				inRequestID)
{
	ONtNet_Request_Quit		quit_request;
	
	// initialize the quit request
	quit_request.uns32_0 = inRequestID;
	
	// add request to packet
	ONiNet_Client_Packet_Build(
		ioClient,
		ONcC2S_Request_Quit,
		(UUtUns8*)&quit_request,
		sizeof(ONtNet_Request_Quit));
}

// ----------------------------------------------------------------------
static void
ONiNet_CS_Request_WeaponInfo(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessgroup,
	UUtUns16				inWeaponIndex)
{
	ONtNet_Request_WeaponInfo		info_request;
	WPtWeapon						*weapon;
	UUtUns16						updateFlags;
	
	// get a pointer to the weapon
	weapon = WPrGetWeapon(inWeaponIndex);
	
	// send request only if the weapon info request hasn't already been made
	updateFlags = WPrGetUpdateFlags(weapon);
	if (updateFlags & WPcWeaponUpdateFlag_NeedInfo)
	{
		return;
	}
	
	WPrSetUpdateFlags(weapon, updateFlags | WPcWeaponUpdateFlag_NeedInfo);
	
	// initialize the info request
	info_request.uns16_0 = inWeaponIndex;
	
	// add request to packet
	ONiNet_Client_Packet_Build(
		ioClient,
		ONcC2S_Request_WeaponInfo,
		(UUtUns8*)&info_request,
		sizeof(ONtNet_Request_WeaponInfo));

//COrConsole_Printf("Client: Request Weapon %d Info", inWeaponIndex);
}

// ----------------------------------------------------------------------
void
ONrNet_CS_Update_Character(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	ONtNet_Update_Character		character_update;
	UUtUns16					update_size;
		
	// make sure the character can be updated
	if ((ioClient->character == NULL) ||
		(ioClient->character->animation == NULL) ||
		(ioClient->character->physics == NULL))
	{
		return;
	}
		
	// generate the update data
	if (ONgNet_Authoritative_Server == UUcTrue)
	{
		update_size =
			ONiNet_Generate_Update_Authoritative(
				ioClient->character,
				&ioClient->prev_update,
				&character_update);
	}
	else
	{
		update_size =
			ONiNet_Generate_Update(
				ioClient->character,
				&ioClient->prev_update,
				&character_update);
	}
		
	// add the update to packet
	ONiNet_Client_Packet_Build(
		ioClient,
		ONcC2S_Update_Character,
		(UUtUns8*)&character_update,
		update_size);
}

// ----------------------------------------------------------------------
void
ONrNet_CS_Update_Critical(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	ONtNet_Critical_Update	critical_update;
	UUtUns16				update_size;
	
	// make sure the critical update can be done
	if (ioClient->character == NULL)
	{
		return;
	}
	
	// make sure a critical update is needed
	if ((ioClient->cupdate_bits == ONcCharacterUpdateFlag_None) &&
		(ioClient->character->updateFlags == ONcCharacterUpdateFlag_None))
	{
		return;
	}
	
	// a response is needed
	ioClient->cupdate_bits |= ioClient->character->updateFlags;
	
	// clear the character's updateFlags
	ioClient->character->updateFlags = ONcCharacterUpdateFlag_None;

	// generate the critical update
	update_size =
		ONiNet_Generate_Critical_Update(
			ioClient->character,
			ioClient->cupdate_bits,
			&critical_update);
	
	if (update_size > ONcNet_CUpdate_MinSize)
	{
		// add the update to packet
		ONiNet_Client_Packet_Build(
			ioClient,
			ONcC2S_Update_Critical,
			(UUtUns8*)&critical_update,
			update_size);
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
ONiNet_CH_ACK_Critical_Update(
	ONtNet_Client			*ioClient)
{
	ioClient->cupdate_bits = ONcCharacterUpdateFlag_None;
}

// ----------------------------------------------------------------------
static void
ONiNet_CH_ACK_Quit(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	ONtNet_Ack_Quit			*ack_quit;
	UUtUns32				request_id;
	
	// get a pointer to the ack quit
	ack_quit = ONmNet_GetDataPtr(ioProcessGroup, ONtNet_Ack_Quit);
	
	// get the request id
	request_id = ack_quit->uns32_0;
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_4Byte(&request_id);
	}
	
	// the request was completed
	ONrNet_Client_Request_ResponseReceived(ioClient, request_id);
}

// ----------------------------------------------------------------------
static void
ONiNet_CH_Info_Character(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	ONtNet_Info_Character	*character_info;
	UUtUns16				character_index;
	ONtCharacter			*character;
	AItCharacterSetup		setup;
	UUtError				error;
	TRtAnimation			*animation;
		
	// get a pointer to the character create info
	character_info = ONmNet_GetDataPtr(ioProcessGroup, ONtNet_Info_Character);
	
	// get the data from the packet
	character_index = character_info->character_index;
	
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_2Byte(&character_index);
	}
	
	// get a pointer to the character
	character = &ioProcessGroup->common->character_list[character_index];
	
	// if this character is already in use, then assume that a duplicate
	// character info response was received
	if (character->flags & ONcCharacterFlag_InUse) return;
	
	// initialize the character setup
	UUrMemory_Clear(&setup, sizeof(AItCharacterSetup));
	setup.characterClass = ONrGetCharacterClass(character_info->character_class_name);
	
	// create the character
	error = ONrGameState_NewCharacter_atIndex(NULL, &setup, NULL, character_index);
	if (error != UUcError_None) return;
	
	// copy the player name
	UUrString_Copy(
		character->player_name,
		character_info->player_name,
		ONcMaxPlayerNameLength);
	
	// set the character's position, facing, and team number
	character->teamNumber			= character_info->team_number;
	character->animFrame			= character_info->anim_frame;
	character->facing				= character_info->facing;
	character->physics->position	= character_info->position;
	
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_2Byte(&character->teamNumber);
		UUrSwap_2Byte(&character->animFrame);
		UUrSwap_4Byte(&character->facing);
		MUmPoint3D_Swap(character->physics->position);
	}
	character->desiredFacing = character->facing;
	character->location = character->physics->position;
	
	// get the animation
	error =
		TMrInstance_GetDataPtr(
			TRcTemplate_Animation,
			character_info->animName,
			&animation);
	if (error == UUcError_None)
	{
		// set the characters animation
		ONrCharacter_SetAnimationInternal(
			character,
			TRrAnimation_GetFrom(character->animation),
			ONcAnimType_None,
			animation);
	}
}

// ----------------------------------------------------------------------
static void
ONiNet_CH_Info_CharacterCreate(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	ONtNet_Info_CharacterCreate		*create_info;
	UUtUns32						request_id;
	UUtUns16						character_index;
	AItCharacterSetup				setup;
	M3tPoint3D						position;
	float							facing;
	UUtError						error;
	
	// get a pointer to the character create info
	create_info = ONmNet_GetDataPtr(ioProcessGroup, ONtNet_Info_CharacterCreate);
	
	// the client has to be in the join stage character create to handle this type of data
	if (ioClient->join_stage != ONcNet_JoinStage_CreateCharacter) return;
	
	// copy the data
	request_id			= create_info->request_id;
	character_index		= create_info->character_index;
	position			= create_info->position;
	facing				= create_info->facing;
	
	// swap the data
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_2Byte(&request_id);
		UUrSwap_2Byte(&character_index);
		MUmPoint3D_Swap(position);
		UUrSwap_4Byte(&facing);
	}
	
	if (ONrNet_IsServer() == UUcFalse)
	{
		// initialize the setup
		UUrMemory_Clear(&setup, sizeof(AItCharacterSetup));
		setup.teamNumber		= ioClient->teamNumber;
		setup.characterClass	= ONrGetCharacterClass(ioClient->characterClassName);
		UUrString_Copy(
			setup.playerName,
			ioClient->playerName,
			ONcMaxPlayerNameLength);
		
		// create the character
		error = ONrGameState_NewCharacter_atIndex(NULL, &setup, NULL, character_index);
		if (error != UUcError_None)
		{
			// handle total failure
			return;
		}
	}

	// set the client's character
	ioClient->character = &ioProcessGroup->common->character_list[character_index];

	// set the player number to send input to the proper character
	ONrGameState_SetPlayerNum(character_index);

	// set up the aiming vectors (necessary for the camera)
	ONrCharacter_DoAiming(ioClient->character);

	// set the camera to track the proper character
	CArLevelBegin(/*ioClient->character*/);	// XXX- is this right??
	CAgCamera.star = ioClient->character;
	CArFollow_Enter();

	// the request was completed
	ONrNet_Client_Request_ResponseReceived(ioClient, request_id);

	// advance to the next stage of joining
	ONrNet_Client_JoinStage_Advance(ioClient, ioProcessGroup->common);
}

// ----------------------------------------------------------------------
static void
ONiNet_CH_Info_Game(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	ONtNet_Info_Game		*game_info;
	UUtUns32				request_id;
	UUtUns16				level_number;
	
	// get a pointer to the game info
	game_info = ONmNet_GetDataPtr(ioProcessGroup, ONtNet_Info_Game);
	
	// the client has to be in join stage game info to handle this type of data
	if (ioClient->join_stage != ONcNet_JoinStage_GameInfo) return;
	
	// process the game info
	request_id				= game_info->request_id;
	ioClient->player_index	= game_info->player_index;
	level_number			= game_info->level_number;
	
	// swap the data
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_2Byte(&request_id);
		UUrSwap_2Byte(&ioClient->player_index);
		UUrSwap_2Byte(&level_number);
	}
	
	// the request was completed
	ONrNet_Client_Request_ResponseReceived(ioClient, request_id);
	
	// load the level
	if (ONrNet_IsServer() == UUcFalse)
	{
		ONrNet_LevelLoad(ioProcessGroup->common, level_number);
	}
	
	// advance to the next stage of joining
	ONrNet_Client_JoinStage_Advance(ioClient, ioProcessGroup->common);
}

// ----------------------------------------------------------------------
static void
ONiNet_CH_Reject_Join(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	ONtNet_Reject_Join		*rejection;
	
	// get a pointer to the join rejection
	rejection = ONmNet_GetDataPtr(ioProcessGroup, ONtNet_Reject_Join);
	
	// the client has to be in the join stage game info to handle this type of data
	if (ioClient->join_stage != ONcNet_JoinStage_GameInfo) return;
	
	// swap the data
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_4Byte(&rejection->uns32_0);
	}

	// process the rejection
	ONrNet_Client_Request_ResponseReceived(ioClient, rejection->uns32_0);
	
	// handle the rejection
	ONrNet_Client_Rejected(ioClient);
}

// ----------------------------------------------------------------------
static void
ONiNet_CH_Update_Character(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	ONtNet_Update_Character	*character_update;
	ONtCharacter			*character;
	UUtUns16				character_index;
	
	// get a pointer to the character update
	character_update = (ONtNet_Update_Character*)ioProcessGroup->data;
	ONiNet_DataAdvance(ioProcessGroup, (UUtUns16)ONcNet_Update_Character_MinSize);
	
	// don't process character updates on the server
	if (ONrNet_IsServer() == UUcTrue)
	{
		ONiNet_Skip_Update(ioProcessGroup, character_update);
		return;
	}
	
	// swap the header data
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_2Byte(&character_update->hdr.character_index);
		UUrSwap_2Byte(&character_update->hdr.animFrame);
		MUmPoint3D_Swap(character_update->hdr.position);
	}
	
	// get a local copy of the data
	character_index = character_update->hdr.character_index;
	UUmAssert(character_index < ONcMaxCharacters);
	
	if (ONgNet_Authoritative_Server == UUcFalse)
	{
		// don't process the update for the character controlled by this machine
		if ((ioClient->character != NULL) &&
			(ioClient->character->index == character_index))
		{
			// advance past the character update data
			ONiNet_Skip_Update(ioProcessGroup, character_update);
			return;
		}
	}
		
	// get a pointer to the character
	character = &ioProcessGroup->common->character_list[character_index];
		
	// if the character isn't in use, request info on the character
	if (!(character->flags & ONcCharacterFlag_InUse))
	{
		// request info on the character
		ONiNet_CS_Request_CharacterInfo(ioClient, ioProcessGroup, character_index);

		// advance past the character update data
		ONiNet_Skip_Update(ioProcessGroup, character_update);
		return;
	}
	
	// process the character update
	if (ONgNet_Authoritative_Server == UUcTrue)
	{
		ONiNet_Process_Update_Authoritative(ioProcessGroup, character_update);
	}
	else
	{
		ONiNet_Process_Update(ioProcessGroup, character_update);
	}
	
	// cleanup
	if ((ONgNet_Authoritative_Server == UUcTrue) &&
		(ioClient->character != NULL) &&
		(ioClient->character->index == character_index))
	{
		ioClient->character->pleaseFire = UUcFalse;
	}
}

// ----------------------------------------------------------------------
static void
ONiNet_CH_Update_Critical(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	ONtNet_Critical_Update	*critical_update;
	ONtCharacter			*character;
	UUtUns16				character_index;
	
	// get a pointer to the critical update
	critical_update = (ONtNet_Critical_Update*)ioProcessGroup->data;
	ONiNet_DataAdvance(ioProcessGroup, (UUtUns16)ONcNet_CUpdate_MinSize);
	
	// swap the standard data
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_2Byte(&critical_update->hdr.character_index);
	}
	
	// get a local copy of the data
	character_index = critical_update->hdr.character_index;

	// get a pointer to the character
	character = &ioProcessGroup->common->character_list[character_index];

	// don't process critical updates on the server or
	// if the character isn't in use, skip the critical update
	if ((ONrNet_IsServer() == UUcTrue) ||
		(!(character->flags & ONcCharacterFlag_InUse)))
	{
		ONiNet_Skip_Critical_Update(ioProcessGroup, critical_update);

		// acknowledge the critical update
		ONiNet_CS_ACK_Critical_Update(ioClient, ioProcessGroup, character_index);
		
		return;
	}
			
	// process the critical update
	ONiNet_Process_Critical_Update(ioProcessGroup, critical_update, ioClient->character);
	
	// acknowledge the critical update
	ONiNet_CS_ACK_Critical_Update(ioClient, ioProcessGroup, character_index);
	
	// request info about the weapon in the primary slot if the weapon isn't in use
	if ((character->inventory.weapons[WPcPrimarySlot].weapon) &&
		(WPrInUse(character->inventory.weapons[WPcPrimarySlot].weapon) == UUcFalse))
	{
		ONiNet_CS_Request_WeaponInfo(
			ioClient,
			ioProcessGroup,
			WPrGetIndex(character->inventory.weapons[WPcPrimarySlot].weapon));
	}
}

// ----------------------------------------------------------------------
static void
ONiNet_CH_Update_Weapons(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	ONtNet_Weapon_Update	*weapon_update;
	WPtWeapon				*weapon;
	
	// get a pointer to the weapon update
	weapon_update = (ONtNet_Weapon_Update*)ioProcessGroup->data;
	ONiNet_DataAdvance(ioProcessGroup, (UUtUns16)ONcNet_WUpdate_MinSize);
	
	// swap the standard data
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_2Byte(&weapon_update->hdr.weapon_index);
	}
	
	// don't process weapon updates on the server
	if (ONrNet_IsServer() == UUcTrue)
	{
		ONiNet_Skip_Weapon_Update(ioProcessGroup, weapon_update);
		ONiNet_CS_ACK_Weapon_Update(ioClient, ioProcessGroup, weapon_update->hdr.weapon_index);
		return;
	}
	
	// if the weapon is unknown, send a request for info
	weapon = WPrGetWeapon(weapon_update->hdr.weapon_index);
	if ((WPrInUse(weapon) == UUcFalse) &&
		((WPrGetUpdateFlags(weapon) & WPcWeaponUpdateFlag_NeedInfo) == 0))
	{
		ONiNet_CS_Request_WeaponInfo(ioClient, ioProcessGroup, weapon_update->hdr.weapon_index);
		
		ONiNet_Skip_Weapon_Update(ioProcessGroup, weapon_update);
		ONiNet_CS_ACK_Weapon_Update(ioClient, ioProcessGroup, weapon_update->hdr.weapon_index);
		return;
	}
	
	// process the weapon update
	ONiNet_Process_Weapon_Update(ioProcessGroup, weapon_update);
	
	// acknowledge the weapon update
	ONiNet_CS_ACK_Weapon_Update(ioClient, ioProcessGroup, weapon_update->hdr.weapon_index);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
ONiNet_Client_IsLegalPacket(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	const ONtNet_Packet		*packet;
	UUtUns16				packet_length;
	
	// get the packet and packet_length from ioProcessGroup
	packet = &ioProcessGroup->packet;
	packet_length = ioProcessGroup->packet_length;
	
	// make sure the packet version is supported
	if (packet->packet_header.packet_version != ONcNet_VersionNumber)
	{
		return UUcFalse;
	}
	
	// check the packet's number against the previously received packet
	if (ioClient->previous_packet_number >=	packet->packet_header.packet_number)
	{
		return UUcFalse;
	}
	
	// make sure the packet sizes are okay
	if (packet->packet_header.packet_length > packet_length)
	{
		return UUcFalse;
	}
	
	// save the packet number
	ioClient->previous_packet_number = packet->packet_header.packet_number;
	
	// save the packet time
	ioClient->previous_packet_time = ioProcessGroup->common->game_time;

	return UUcTrue;
}

// ----------------------------------------------------------------------
void
ONrNet_Client_ProcessPacket(
	ONtNet_Client			*ioClient,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	UUtBool					is_legal_packet;
	ONtNet_DataType			prev_datatype;
	UUtUns16				i;
	
	// swap the packet header
	// NOTE:  byte-swapping is only done on incoming data if it needs to be swapped.
	// 		  All outgoing data is written in the processors native endian format
	ioProcessGroup->swap_it = ONiNet_Swap_PacketHeader(&ioProcessGroup->packet);
	
	// make sure the packet can be used
	is_legal_packet = ONiNet_Client_IsLegalPacket(ioClient, ioProcessGroup);
	if (is_legal_packet == UUcFalse) return;
	
	// set the pointers
	ioProcessGroup->datatype_info =
		(ONtNet_DataTypeInfo*)ioProcessGroup->packet.packet_data;
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_2Byte(&ioProcessGroup->datatype_info->num_datatypes);
		UUrSwap_2Byte(&ioProcessGroup->datatype_info->datatypes_offset);
	}
	
	ioProcessGroup->data =
		ioProcessGroup->packet.packet_data +
		sizeof(ONtNet_DataTypeInfo);
	
	ioProcessGroup->datatypes =
		ioProcessGroup->packet.packet_data +
		ioProcessGroup->datatype_info->datatypes_offset;
	
	// process the data
	prev_datatype = ONcNone;
	for (i = 0; i < ioProcessGroup->datatype_info->num_datatypes; i++)
	{
		ONtNet_DataType					datatype;
		
		// get the ONtNet_DataType
		datatype = ioProcessGroup->datatypes[i];
		
UUmAssert(((UUtUns32)ioProcessGroup->data & 3) == 0);

		switch (datatype)
		{
			case ONcS2C_ACK_Critical_Update:	ONiNet_CH_ACK_Critical_Update(ioClient); break;
			case ONcS2C_ACK_Quit:				ONiNet_CH_ACK_Quit(ioClient, ioProcessGroup); break;
			case ONcS2C_Info_Character:			ONiNet_CH_Info_Character(ioClient, ioProcessGroup); break;
			case ONcS2C_Info_CharacterCreate:	ONiNet_CH_Info_CharacterCreate(ioClient, ioProcessGroup); break;
			case ONcS2C_Info_Game:				ONiNet_CH_Info_Game(ioClient, ioProcessGroup); break;
			case ONcS2C_Reject_Join:			ONiNet_CH_Reject_Join(ioClient, ioProcessGroup); break;
			case ONcS2C_Update_Character:		ONiNet_CH_Update_Character(ioClient, ioProcessGroup); break;
			case ONcS2C_Update_Critical:		ONiNet_CH_Update_Critical(ioClient, ioProcessGroup); break;
			case ONcS2C_Update_Weapon:			ONiNet_CH_Update_Weapons(ioClient, ioProcessGroup); break;
			default: UUmAssert(!"unknown data type"); break;
		}
		
		prev_datatype = datatype;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns16
ONiNet_Server_GetFreePlayer(
	ONtNet_Server			*ioServer)
{
	UUtUns16				i;
	
	for (i = 0; i < ioServer->game_options.max_players; i++)
	{
		if (ioServer->players[i].inUse == UUcFalse) break;
	}
	
	return i;
}

// ----------------------------------------------------------------------
static void
ONiNet_Server_RawPacket_Send(
	ONtNet_Server			*ioServer,
	NMtNetAddress			*inToAddress,
	ONtNet_Packet			*inPacket,
	UUtUns16				inPacketLength)
{
	// make sure there is data to send
	if (inPacketLength == 0) return;

	// create a packet and send it
	inPacket->packet_header.packet_swap		= ONcNet_SwapIt;
	inPacket->packet_header.packet_number	= ioServer->packet_number++;
	inPacket->packet_header.packet_version	= ONcNet_VersionNumber;
	inPacket->packet_header.player_index	= ONcNet_ServerIndex;
	inPacket->packet_header.packet_length	= inPacketLength;
	
	// send it
	NMrNetContext_WriteData(
		ioServer->server_context,
		inToAddress, 
		(UUtUns8*)inPacket, 
		inPacketLength, 
		NMcPacketFlags_None);

	ioServer->raw_packets_sent++;
	ioServer->raw_bytes_sent += inPacketLength;
}

// ----------------------------------------------------------------------
void
ONrNet_Server_PlayerPacket_Send(
	ONtNet_Server			*ioServer,
	UUtUns16				inPlayerIndex)
{
	ONtNet_Players			*player;
	UUtUns16				packet_length;
	
	// get a pointer to the player
	player = &ioServer->players[inPlayerIndex];
	
	// make sure there is data to send
	if (player->player_packet_data.datatype_info->num_datatypes == 0) return;

	// build the packet
	packet_length = ONiNet_Packet_Build(&player->player_packet_data);
	
	// initialize the packet header
	player->player_packet_data.packet.packet_header.packet_swap		= ONcNet_SwapIt;
	player->player_packet_data.packet.packet_header.packet_number	= ioServer->packet_number++;
	player->player_packet_data.packet.packet_header.packet_version	= ONcNet_VersionNumber;
	player->player_packet_data.packet.packet_header.player_index	= ONcNet_ServerIndex;
	player->player_packet_data.packet.packet_header.packet_length	= packet_length;
	
	// send it
	NMrNetContext_WriteData(
		ioServer->server_context,
		&player->player_address, 
		(UUtUns8*)&player->player_packet_data.packet, 
		packet_length, 
		NMcPacketFlags_None);
	
	ioServer->player_packets_sent++;
	ioServer->player_bytes_sent += packet_length;

	// there is no more data in the packet
	ONrNet_Packet_Init(&player->player_packet_data);
}

// ----------------------------------------------------------------------
static void
ONiNet_Server_PlayerPacket_Build(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup,
	const UUtUns16			inPlayerIndex,
	const UUtUns8			inDataType,
	const UUtUns8			*inData,
	const UUtUns16			inDataLength)
{
	UUtBool					status;
	UUtUns16				packet_length;
	
	if (inPlayerIndex == ONcNet_UnknownPlayer)
	{
		// The client requesting data has not joined this server yet so it has no
		// outgoing packet to use
		ONtNet_PacketData	packet_data;
		
		// initialize the packet data
		ONrNet_Packet_Init(&packet_data);
		
		// add the data to the packet
		status =
			ONiNet_Packet_AddData(
				&packet_data,
				inDataType,
				inData,
				inDataLength);
		if (status == UUcFalse)
		{
			UUmAssert(!"should never get here");
		}
		
		// build the packet
		packet_length = ONiNet_Packet_Build(&packet_data);
		
		// send the response_packet
		ONiNet_Server_RawPacket_Send(
			ioServer,
			&ioProcessGroup->from,
			&packet_data.packet,
			packet_length);
	}
	else
	{
		// add inData to this players outgoing data if there is room, if not, send
		// the current outgoing data, and start a new packet
		ONtNet_Players		*player;
		
		// get a pointer to the player
		player = &ioServer->players[inPlayerIndex];
		
		// add the datat to the packet
		status =
			ONiNet_Packet_AddData(
				&player->player_packet_data,
				inDataType,
				inData,
				inDataLength);
		if (status == UUcFalse)
		{
			// there is not enough room in the packet.  Send the current data
			// and then add the new data
			ONrNet_Server_PlayerPacket_Send(ioServer, inPlayerIndex);
			
			status =
				ONiNet_Packet_AddData(
					&player->player_packet_data,
					inDataType,
					inData,
					inDataLength);
			if (status == UUcFalse)
			{
				UUmAssert(!"should never get here");
			}
		}
	}
}

// ----------------------------------------------------------------------
void
ONrNet_Server_MassPacket_Clear(
	ONtNet_Server			*ioServer)
{
	ONrNet_Packet_Init(&ioServer->mass_packet_data);
}

// ----------------------------------------------------------------------
void
ONrNet_Server_MassPacket_PrepareForSend(
	ONtNet_Server			*ioServer)
{
	UUtUns16				packet_length;
	
	// initialize the packet data
	packet_length = ONiNet_Packet_Build(&ioServer->mass_packet_data);

	// initialize the packet header
	ioServer->mass_packet_data.packet.packet_header.packet_swap		= ONcNet_SwapIt;
	ioServer->mass_packet_data.packet.packet_header.packet_version	= ONcNet_VersionNumber;
	ioServer->mass_packet_data.packet.packet_header.player_index	= ONcNet_ServerIndex;
	ioServer->mass_packet_data.packet.packet_header.packet_length	= packet_length;
}

// ----------------------------------------------------------------------
void
ONrNet_Server_MassPacket_SendToAddress(
	ONtNet_Server			*ioServer,
	NMtNetAddress			*inToAddress)
{
	// make sure there is data to send
	if (ioServer->mass_packet_data.datatype_info->num_datatypes == 0) return;
	
	// set the packet number
	ioServer->mass_packet_data.packet.packet_header.packet_number = ioServer->packet_number++;
	
	// send the packet
	NMrNetContext_WriteData(
		ioServer->server_context,
		inToAddress, 
		(UUtUns8*)&ioServer->mass_packet_data.packet, 
		ioServer->mass_packet_data.packet.packet_header.packet_length, 
		NMcPacketFlags_None);

	ioServer->mass_packets_sent++;
	ioServer->mass_bytes_sent += ioServer->mass_packet_data.packet.packet_header.packet_length;
}

// ----------------------------------------------------------------------
static void
ONiNet_Server_MassPacket_Send(
	ONtNet_Server			*ioServer)
{
	UUtUns16				i;
	
	// make sure there is data to send
	if (ioServer->mass_packet_data.datatype_info->num_datatypes == 0) return;
	
	// initialize the mass packet
	ONrNet_Server_MassPacket_PrepareForSend(ioServer);

	// go through all the players	
	for (i = 0; i < ioServer->game_options.max_players; i++)
	{
		if (ioServer->players[i].send_updates)
		{
			ONrNet_Server_MassPacket_SendToAddress(
				ioServer,
				&ioServer->players[i].player_address);
		}
	}

	// clear the mass packet
	ONrNet_Server_MassPacket_Clear(ioServer);
}

// ----------------------------------------------------------------------
static void
ONiNet_Server_MassPacket_Build(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup,
	const UUtUns8			inDataType,
	const UUtUns8			*inData,
	const UUtUns16			inDataLength)
{
	UUtBool					status;
	
	// add the data to the packet
	status =
		ONiNet_Packet_AddData(
			&ioServer->mass_packet_data,
			inDataType,
			inData,
			inDataLength);
	if (status == UUcFalse)
	{
		// there is not enough room in the packet.  Send the current data
		// and then add the data
		ONiNet_Server_MassPacket_Send(ioServer);
		
		status =
			ONiNet_Packet_AddData(
				&ioServer->mass_packet_data,
				inDataType,
				inData,
				inDataLength);
		if (status == UUcFalse)
		{
			UUmAssert(!"should never get here");
		}
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
ONiNet_SS_ACK_Critical_Update(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	// add ack to packet
	ONiNet_Server_PlayerPacket_Build(
		ioServer,
		ioProcessGroup,
		ioProcessGroup->packet.packet_header.player_index,
		ONcS2C_ACK_Critical_Update,
		NULL,
		0);
}

// ----------------------------------------------------------------------
static void
ONiNet_SS_ACK_Quit(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup,
	UUtUns16				inPlayerIndex,
	UUtUns32				inRequestID)
{
	ONtNet_Ack_Quit			ack_quit;
	
	ack_quit.uns32_0 = inRequestID;
	
	// add ack to packet
	ONiNet_Server_PlayerPacket_Build(
		ioServer,
		ioProcessGroup,
		ioProcessGroup->packet.packet_header.player_index,
		ONcS2C_ACK_Quit,
		(UUtUns8*)&ack_quit,
		sizeof(ONtNet_Ack_Quit));
}

// ----------------------------------------------------------------------
static void
ONiNet_SS_Info_Game(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup,
	UUtUns16				inPlayerIndex,
	UUtUns32				inRequestID)
{
	ONtNet_Info_Game		game_info;
	
	game_info.request_id		= inRequestID;
	game_info.player_index		= inPlayerIndex;
	game_info.level_number		= ioProcessGroup->common->level_number;
	
	// add info to packet
	ONiNet_Server_PlayerPacket_Build(
		ioServer,
		ioProcessGroup,
		inPlayerIndex,
		ONcS2C_Info_Game,
		(UUtUns8*)&game_info,
		sizeof(ONtNet_Info_Game));
}

// ----------------------------------------------------------------------
static void
ONiNet_SS_Reject_Join(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup,
	UUtUns32				inRequestID)
{
	ONtNet_Reject_Join		rejection;
	
	rejection.uns32_0 = inRequestID;
	
	// add ack to packet
	ONiNet_Server_PlayerPacket_Build(
		ioServer,
		ioProcessGroup,
		ONcNet_UnknownPlayer,
		ONcS2C_Reject_Join,
		(UUtUns8*)&rejection,
		sizeof(ONtNet_Reject_Join));
}

// ----------------------------------------------------------------------
void
ONrNet_SS_Update_Character(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup,
	ONtCharacter			*inCharacter)
{
	ONtNet_Update_Character			character_update;
	UUtUns16						update_size;
	
	// make sure the character can be updated
	if ((inCharacter == NULL) ||
		(inCharacter->animation == NULL) ||
		(inCharacter->physics == NULL))
	{
		return;
	}
	
	// generate the update data
	if (ONgNet_Authoritative_Server == UUcTrue)
	{
		update_size =
			ONiNet_Generate_Update_Authoritative(
				inCharacter,
				&ioServer->prev_update[inCharacter->index],
				&character_update);
	}
	else
	{
		update_size =
			ONiNet_Generate_Update(
				inCharacter,
				&ioServer->prev_update[inCharacter->index],
				&character_update);
	}
	
	// add the data to the update packet
	ONiNet_Server_MassPacket_Build(
		ioServer,
		ioProcessGroup,
		ONcS2C_Update_Character,
		(UUtUns8*)(&character_update),
		update_size);
}

// ----------------------------------------------------------------------
void
ONrNet_SS_Update_Critical(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup,
	ONtCharacter			*inCharacter,
	UUtUns32				*outUpdateFlags)
{
	ONtNet_Critical_Update		critical_update;
	UUtUns16					update_size;
	
	// set the update bits
	*outUpdateFlags = inCharacter->updateFlags;
	
	// make sure the character can be updated and an update is needed
	if ((inCharacter == NULL) ||
		(inCharacter->updateFlags == ONcCharacterUpdateFlag_None))
	{
		return;
	}
	
	// clear the character's updateFlags
	inCharacter->updateFlags = ONcCharacterUpdateFlag_None;
	
	// generate the update data
	update_size =
		ONiNet_Generate_Critical_Update(
			inCharacter,
			*outUpdateFlags,
			&critical_update);
			
	if (update_size > ONcNet_CUpdate_MinSize)
	{
		// add the data to the update packet
		ONiNet_Server_MassPacket_Build(
			ioServer,
			ioProcessGroup,
			ONcS2C_Update_Critical,
			(UUtUns8*)(&critical_update),
			update_size);
	}
}

// ----------------------------------------------------------------------
void
ONrNet_SS_Update_Critical_Force(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup,
	ONtCharacter			*inCharacter,
	UUtUns32				inUpdateFlags,
	UUtUns16				inPlayerIndex)
{
	ONtNet_Critical_Update	critical_update;
	UUtUns16				update_size;
	
	// make sure the character can be updated
	if (inCharacter == NULL) return;
	
	// generate the update data
	update_size =
		ONiNet_Generate_Critical_Update(
			inCharacter,
			inUpdateFlags,
			&critical_update);
	
	if (update_size > ONcNet_CUpdate_MinSize)
	{
		// add the data to the response packet
		ONiNet_Server_PlayerPacket_Build(
			ioServer,
			ioProcessGroup,
			inPlayerIndex,
			ONcS2C_Update_Critical,
			(UUtUns8*)&critical_update,
			update_size);
	}
	else
	{
		// there are no updates so clear the character's update flag
		ioServer->players[inPlayerIndex].updateACKNeeded[inCharacter->index] = UUcFalse;
	}
}

// ----------------------------------------------------------------------
void
ONrNet_SS_Update_Weapons(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	UUtUns16				i;
	
	// check the weapon positions
	for (i = 0; i < WPcMaxWeapons; i++)
	{
		WPtWeapon				*weapon;
		
		weapon = WPrGetWeapon(i);
		if (WPrInUse(weapon) && (WPrGetOwner(weapon) == NULL))
		{
			M3tPoint3D				position;
	
			WPrGetPosition(weapon, &position);
			
			if ((position.x != ONgNet_WeaponPosition[i].x) ||
				(position.y != ONgNet_WeaponPosition[i].y) ||
				(position.z != ONgNet_WeaponPosition[i].z))
			{
				WPrSetUpdateFlags(weapon, WPrGetUpdateFlags(weapon) | WPcWeaponUpdateFlag_Position);
			}
			
			ONgNet_WeaponPosition[i] = position;
		}
	}
	
	// update each weapon
	for (i = 0; i < WPcMaxWeapons; i++)
	{
		WPtWeapon				*weapon;
		ONtNet_Weapon_Update	weapon_update;
		UUtUns16				update_size;
		UUtUns16				j;
		UUtUns16				updateFlags;
		
		// get a pointer to the weapon
		weapon = WPrGetWeapon(i);
		if (WPrInUse(weapon) == UUcFalse) { continue; }
		
		// if the weapon's updateFlags are set, then do a mass update
		updateFlags = WPrGetUpdateFlags(weapon);
		if (updateFlags != WPcWeaponUpdateFlag_None)
		{
			// save the update flags
			ioServer->updateFlags[i] |= updateFlags; 
			
			// generate the update
			update_size =
				ONiNet_Generate_Weapon_Update(
					weapon,
					i,
					updateFlags,
					&weapon_update);
			
			// add the data to the packet for all the clients
			ONiNet_Server_MassPacket_Build(
				ioServer,
				ioProcessGroup,
				ONcS2C_Update_Weapon,
				(UUtUns8*)(&weapon_update),
				update_size);

			// mark the players weaponACKNeeded
			for (j = 0; j < ioServer->game_options.max_players; j++)
			{
				if (ioServer->players[j].inUse == UUcFalse) { continue; }
				
				// if this player failed to acknowledge a previous weapon update then
				// generate a new one and send it to the that player
				if (ioServer->players[j].weaponACKNeeded[i])
				{
					// generate the update with the previous updateFlags
					update_size =
						ONiNet_Generate_Weapon_Update(
							weapon,
							i,
							ioServer->weaponFlags[i],
							&weapon_update);
					
					// add the update to the player packet
					ONiNet_Server_PlayerPacket_Build(
						ioServer,
						ioProcessGroup,
						j,
						ONcS2C_Update_Weapon,
						(UUtUns8*)&weapon_update,
						update_size);
				}
				
				// the mass packet needs to be updated
				ioServer->players[j].weaponACKNeeded[i] = UUcTrue;
			}
		}
		else
		{
			UUtBool				forced_update;
			
			forced_update = UUcFalse;
			
			for (j = 0; j < ioServer->game_options.max_players; j++)
			{
				if (ioServer->players[j].inUse == UUcFalse) { continue; }

				// if this player failed to acknowledge the weapon update then
				// generate a new one and send it to the that player
				if (ioServer->players[j].weaponACKNeeded[i])
				{
					// generate the update with the previous updateFlags
					update_size =
						ONiNet_Generate_Weapon_Update(
							weapon,
							i,
							ioServer->weaponFlags[i],
							&weapon_update);
					
					// add the update to the player packet
					ONiNet_Server_PlayerPacket_Build(
						ioServer,
						ioProcessGroup,
						j,
						ONcS2C_Update_Weapon,
						(UUtUns8*)&weapon_update,
						update_size);

					forced_update = UUcTrue;
				}
			}
			
			if (forced_update == UUcFalse)
			{
				ioServer->weaponFlags[i] = WPcWeaponUpdateFlag_None;
			}
		}
	}
}


// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
ONiNet_SH_ACK_Critical_Update(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	UUtUns16					player_index;
	UUtUns16					character_index;
	ONtNet_Ack_CriticalUpdate	*ack;
	
	// get the critical update
	ack = ONmNet_GetDataPtr(ioProcessGroup, ONtNet_Ack_CriticalUpdate);
	
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_2Byte(&ack->uns16_0);
	}
	
	// get the character index
	character_index = ack->uns16_0;
	
	// get the player's index
	player_index = ioProcessGroup->packet.packet_header.player_index;
	
	// update ACK received
	ioServer->players[player_index].updateACKNeeded[character_index] = UUcFalse;
}

// ----------------------------------------------------------------------
static void
ONiNet_SH_ACK_Weapon_Update(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	UUtUns16					player_index;
	UUtUns16					weapon_index;
	ONtNet_Ack_CriticalUpdate	*ack;
	
	// get the weapon update
	ack = ONmNet_GetDataPtr(ioProcessGroup, ONtNet_Ack_WeaponUpdate);
	
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_2Byte(&ack->uns16_0);
	}
	
	// get the weapon index
	weapon_index = ack->uns16_0;
	
	// get the player's index
	player_index = ioProcessGroup->packet.packet_header.player_index;
	
	// update the ACK received
	ioServer->players[player_index].weaponACKNeeded[weapon_index] = UUcFalse;
}

// ----------------------------------------------------------------------
static void
ONiNet_SH_Request_CharacterCreate(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	ONtNet_Request_CharacterCreate		*create_request;
	UUtUns32							request_id;
	UUtUns16							team_number;
	UUtUns16							player_index;
	ONtNet_Players						*player;
	ONtNet_Info_CharacterCreate			create_info;
	
	// ------------------------------
	// confirm some information
	// ------------------------------
	// get a pointer to the create request
	create_request = ONmNet_GetDataPtr(ioProcessGroup, ONtNet_Request_CharacterCreate);
	
	request_id = create_request->request_id;
	team_number = create_request->teamNumber;
	
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_2Byte(&request_id);
		UUrSwap_2Byte(&team_number);
	}
	
	// ------------------------------
	// create a new character if necessary
	// ------------------------------
	player_index = ioProcessGroup->packet.packet_header.player_index;
	player = &ioServer->players[player_index];
	if (player->character == NULL)
	{
		UUtError					error;
		UUtUns16					character_index;
		AItCharacterSetup			character_setup;
		ONtFlag						start_flag;
		ONtCharacter				*character;
		UUtBool						found;
		
		// setup the character setup based on info in the character create request
		UUrMemory_Clear(&character_setup, sizeof(AItCharacterSetup));
		
		// initialize the needed data of the character setup
		character_setup.teamNumber		= team_number;
		character_setup.characterClass	= ONrGetCharacterClass(create_request->characterClassName);
		UUrString_Copy(
			character_setup.playerName,
			create_request->playerName,
			ONcMaxPlayerNameLength);
			
		// get the starting location and facing from the level
		found = ONrLevel_Flag_ID_To_Flag(ONrGameState_GetPlayerStart(), &start_flag);
		if (found == UUcFalse) { return; }
		
		// create a new character for the player
		error = ONrGameState_NewCharacter(NULL, &character_setup, &start_flag, &character_index);
		if (error != UUcError_None)
		{
			// tell the player that character creation was unsuccessful
			return;
		}

		// get a pointer to the character
		character = &ioProcessGroup->common->character_list[character_index];

		// set the players character
		player->character =	character;
		
		// change the team number to the character index if the team number is 0xFFFF
		if (team_number == 0xFFFF)	character->teamNumber = character_index;
		
		// clear the update flags so that the previous use of this character (if any)
		// doesn't cause the character to be deleted again when ONrNet_Server_Update()
		// is run
		ioServer->updateFlags[character_index] = ONcCharacterUpdateFlag_None;
	}
	
	UUmAssert(player->character);
	
	// ------------------------------
	// tell the player about its character
	// ------------------------------
	create_info.request_id			= request_id;
	create_info.character_index		= player->character->index;
	create_info.position 			= player->character->physics->position;
	create_info.facing				= player->character->facing;

	// add the data to the response packet
	ONiNet_Server_PlayerPacket_Build(
		ioServer,
		ioProcessGroup,
		player_index,
		ONcS2C_Info_CharacterCreate,
		(UUtUns8*)(&create_info),
		sizeof(ONtNet_Info_CharacterCreate));

	// the player is ready to receive updates
	player->send_updates = UUcTrue;

	// make sure the weapons are sent to the player
	WPrNetPropogate(0xFFFF);
}

// ----------------------------------------------------------------------
static void
ONiNet_SH_Request_CharacterInfo(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	ONtNet_Request_CharacterInfo	*info_request;
	UUtUns16				character_index;
	ONtCharacter			*character;
	ONtNet_Info_Character	character_info;
	
	// get a pointer to the character info request
	info_request = ONmNet_GetDataPtr(ioProcessGroup, ONtNet_Request_CharacterInfo);
	
	// get the character index
	character_index = info_request->uns16_0;
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_2Byte(&character_index);
	}
	
	// get a pointer to the character
	character = &ioProcessGroup->common->character_list[character_index];
	if (character->flags & ONcCharacterFlag_InUse)
	{
		UUtUns16			player_index;
		
		// set the character info data
		character_info.character_index	= character->index;
		character_info.team_number		= character->teamNumber;
		character_info.anim_frame		= character->animFrame;
		character_info.position			= character->physics->position;
		character_info.facing			= character->facing;
		UUrString_Copy(
			character_info.player_name,
			character->player_name,
			ONcMaxPlayerNameLength);
		UUrString_Copy(
			character_info.character_class_name,
			TMrInstance_GetInstanceName(character->characterClass),
			AIcMaxClassNameLen);
		if (character->animation)
		{
			UUrString_Copy(
				character_info.animName,
				TMrInstance_GetInstanceName(character->animation),
				ONcMaxInstNameLength);
		}
		else
		{
			character_info.animName[0] = '\0';
		}
		
		// get the player's index
		player_index = ioProcessGroup->packet.packet_header.player_index;
		
		// add the data to the response packet
		ONiNet_Server_PlayerPacket_Build(
			ioServer,
			ioProcessGroup,
			player_index,
			ONcS2C_Info_Character,
			(UUtUns8*)(&character_info),
			sizeof(ONtNet_Info_Character));
		
		// make the character generate a critical update the next time through
		ioServer->updateFlags[character_index] = 
			(ONcCharacterUpdateFlag_HitPoints |
			ONcCharacterUpdateFlag_Inventory |
			ONcCharacterUpdateFlag_Name |
			ONcCharacterUpdateFlag_Class |
			ONcCharacterUpdateFlag_DamageInflicted |
			ONcCharacterUpdateFlag_NumKills);
	}
}

// ----------------------------------------------------------------------
static void
ONiNet_SH_Request_Join(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	UUtBool					player_in_game;
	UUtUns16				player_index;
	UUtUns16				i;
	ONtNet_Request_Join		*join_request;
	UUtUns32				request_id;
	
	// see if the player is already in the game 
	player_in_game = UUcFalse;
	for (i = 0; i < ioServer->game_options.max_players; i++)
	{
		// if the player has a character, it is in use
		if ((ioServer->players[i].inUse) &&
			(NMrNetContext_CompareAddresses(
				ioServer->server_context,
				&ioServer->players[i].player_address,
				&ioProcessGroup->from)))
		{
			player_in_game = UUcTrue;
			player_index = i;
			break;
		}
	}
	
	// get a pointer to the join request
	join_request = ONmNet_GetDataPtr(ioProcessGroup, ONtNet_Request_Join);
	
	// get a local copy of the requst_id
	request_id = join_request->uns32_0;
	
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_2Byte(&request_id);
	}
	
	if (player_in_game == UUcFalse)
	{
		ONtNet_Players		*player;
		
		// get an available player index
		player_index = ONiNet_Server_GetFreePlayer(ioServer);
		if (player_index == ioServer->game_options.max_players)
		{
			ONiNet_SS_Reject_Join(ioServer, ioProcessGroup, request_id);
			return;
		}
		
		// get a pointer to the player
		player = &ioServer->players[player_index];
		UUrMemory_Clear(player, sizeof(ONtNet_Players));
		
		// initialize the player
		player->inUse						= UUcTrue;
		player->send_updates				= UUcFalse;
		player->character					= NULL;
		player->previous_packet_number		= ioProcessGroup->packet.packet_header.packet_number;
		ONrNet_Packet_Init(&player->player_packet_data);
		UUrMemory_MoveFast(
			&ioProcessGroup->from,
			&player->player_address,
			sizeof(NMtNetAddress));
		UUrMemory_Clear(
			player->updateACKNeeded,
			sizeof(UUtBool) * ONcMaxCharacters);
		UUrMemory_Clear(
			player->weaponACKNeeded,
			sizeof(UUtBool) * WPcMaxWeapons);
		
		// increment the number of players
		ioServer->num_players++;
	}
	
	// send the game info
	ONiNet_SS_Info_Game(ioServer, ioProcessGroup, player_index, request_id);
}

// ----------------------------------------------------------------------
static void
ONiNet_SH_Request_ServerInfo(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	ONtNet_ServerInfo		server_info;
	
	// set the server info
	server_info.max_players		= ioServer->game_options.max_players;
	server_info.num_players		= ioServer->num_players;
	server_info.level_number	= ioProcessGroup->common->level_number;
	
	UUrMemory_MoveFast(
		ioServer->server_name,
		server_info.server_name,
		ONcMaxHostNameLength);
		
	// add ack to packet
	ONiNet_Server_PlayerPacket_Build(
		ioServer,
		ioProcessGroup,
		ONcNet_UnknownPlayer,
		ONcS2C_ServerInfo,
		(UUtUns8*)&server_info,
		sizeof(ONtNet_ServerInfo));
}

// ----------------------------------------------------------------------
static void
ONiNet_SH_Request_Spawn(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	UUtUns16				player_index;
	ONtCharacter			*character;
	
	// get the player's index
	player_index = ioProcessGroup->packet.packet_header.player_index;
	if (player_index == ONcNet_UnknownPlayer) return;
	
	// get a pointer to the character
	character = ioServer->players[player_index].character;
	
	// spawn the character
	if (character->flags & ONcCharacterFlag_Dead)
	{
		ONrCharacter_Spawn(character);

		// set the character's hit points
		ONrCharacter_SetHitPoints(character, 100);
	}
}

// ----------------------------------------------------------------------
static void
ONiNet_SH_Request_Quit(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	UUtUns16				player_index;
	ONtCharacter			*character;
	ONtNet_Request_Quit		*quit_request;
	UUtUns32				request_id;
	
	// get the player's index
	player_index = ioProcessGroup->packet.packet_header.player_index;
	if (player_index == ONcNet_UnknownPlayer) return;
	
	// get a pointer to the quit request
	quit_request = ONmNet_GetDataPtr(ioProcessGroup, ONtNet_Request_Quit);
	
	// get the request id
	request_id = quit_request->uns32_0;
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_4Byte(&request_id);
	}
	
	// get a pointer to the character
	character = ioServer->players[player_index].character;
	
	// the character is leaving
	character->updateFlags |= ONcCharacterUpdateFlag_Gone;
	
	// send acknowledgement
	ONiNet_SS_ACK_Quit(ioServer, ioProcessGroup, player_index, request_id);
}

// ----------------------------------------------------------------------
static void
ONiNet_SH_Request_WeaponInfo(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	ONtNet_Request_WeaponInfo	*weapon_info_request;
	
	// get a pointer to the weapon info request
	weapon_info_request = ONmNet_GetDataPtr(ioProcessGroup, ONtNet_Request_WeaponInfo);
	
	// get the weapon's index
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_2Byte(&weapon_info_request->uns16_0);
	}
	
	// set the update flags for the weapon
	WPrNetPropogate(weapon_info_request->uns16_0);
}

// ----------------------------------------------------------------------
static void
ONiNet_SH_Update_Character(
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	ONtNet_Update_Character	*character_update;
	UUtUns16				player_index;
	
	// get a pointer to the character update
	character_update = (ONtNet_Update_Character*)ioProcessGroup->data;
	ONiNet_DataAdvance(ioProcessGroup, (UUtUns16)ONcNet_Update_Character_MinSize);
	
	// get the player's index
	player_index = ioProcessGroup->packet.packet_header.player_index;
	if (player_index == ONcNet_UnknownPlayer) 
	{
		ONiNet_Skip_Update(ioProcessGroup, character_update);
		return;
	}
	
	// swap the header data
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_2Byte(&character_update->hdr.character_index);
		UUrSwap_2Byte(&character_update->hdr.animFrame);
		MUmPoint3D_Swap(character_update->hdr.position);
	}
		
	// process the character update
	if (ONgNet_Authoritative_Server == UUcTrue)
	{
		ONiNet_Process_Update_Authoritative(ioProcessGroup, character_update);
	}
	else
	{
		ONiNet_Process_Update(ioProcessGroup, character_update);
	}
}

// ----------------------------------------------------------------------
static void
ONiNet_SH_Update_Critical(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	ONtNet_Critical_Update	*critical_update;
	ONtCharacter			*character;
	UUtUns16				character_index;
	
	// get a pointer to the critical update
	critical_update = (ONtNet_Critical_Update*)ioProcessGroup->data;
	ONiNet_DataAdvance(ioProcessGroup, (UUtUns16)ONcNet_CUpdate_MinSize);
	
	// swap the standard data
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_2Byte(&critical_update->hdr.character_index);
	}
	
	// get a local copy of the data
	character_index = critical_update->hdr.character_index;
	
	// get a pointer to the character
	character = &ioProcessGroup->common->character_list[character_index];
		
	// if the character isn't in use, skip the critical update
	if (!(character->flags & ONcCharacterFlag_InUse))
	{
		// advance past the critical update data
		ONiNet_Skip_Critical_Update(ioProcessGroup, critical_update);
		return;
	}

	// process the critical update
	ONiNet_Process_Critical_Update(ioProcessGroup, critical_update, NULL);
	
	// acknowledge the critical update
	ONiNet_SS_ACK_Critical_Update(ioServer, ioProcessGroup);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
ONiNet_Server_IsLegalPacket(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	UUtUns16				player_index;
	const ONtNet_Packet		*packet;
	UUtUns16				packet_length;
	
	// get the packet and packet_length from ioProcessGroup
	packet = &ioProcessGroup->packet;
	packet_length = ioProcessGroup->packet_length;
	
	// make sure the packet version is supported
	if (packet->packet_header.packet_version != ONcNet_VersionNumber)
	{
		return UUcFalse;
	}
	
	// get the player index of the sender
	player_index = packet->packet_header.player_index;
	if (player_index == ONcNet_UnknownPlayer)
	{
		return UUcTrue;
	}
	
	// check the packet's number against the previously received packet
	if (ioServer->players[player_index].previous_packet_number >=
		packet->packet_header.packet_number)
	{
		return UUcFalse;
	}
	
	// make sure the packet sizes are okay
	if (packet->packet_header.packet_length > packet_length)
	{
		return UUcFalse;
	}
	
	// save the packet number
	ioServer->players[player_index].previous_packet_number =
		packet->packet_header.packet_number;
	
	// save the packet time
	ioServer->players[player_index].previous_packet_time =
		ioProcessGroup->common->game_time;
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
void
ONrNet_Server_ProcessPacket(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	UUtBool					is_legal_packet;
	ONtNet_DataType			prev_datatype;
	UUtUns16				i;
	
	// swap the packet header
	// NOTE:  byte-swapping is only done on incoming data if it needs to be swapped.
	// 		  All outgoing data is written in the processors native endian format
	ioProcessGroup->swap_it = ONiNet_Swap_PacketHeader(&ioProcessGroup->packet);
	
	// make sure the packet can be used
	is_legal_packet =
		ONiNet_Server_IsLegalPacket(
			ioServer,
			ioProcessGroup);
	if (is_legal_packet == UUcFalse) return;
	
	// set the pointers
	ioProcessGroup->datatype_info =
		(ONtNet_DataTypeInfo*)ioProcessGroup->packet.packet_data;
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_2Byte(&ioProcessGroup->datatype_info->num_datatypes);
		UUrSwap_2Byte(&ioProcessGroup->datatype_info->datatypes_offset);
	}
	
	ioProcessGroup->data =
		ioProcessGroup->packet.packet_data +
		sizeof(ONtNet_DataTypeInfo);
	
	ioProcessGroup->datatypes =
		ioProcessGroup->packet.packet_data +
		ioProcessGroup->datatype_info->datatypes_offset;

	// process the data
	prev_datatype = ONcNone;
	for (i = 0; i < ioProcessGroup->datatype_info->num_datatypes; i++)
	{
		ONtNet_DataType					datatype;
		
		// get the ONtNet_DataType
		datatype = ioProcessGroup->datatypes[i];
		
UUmAssert(((UUtUns32)ioProcessGroup->data & 3) == 0);

		switch (datatype)
		{
			case ONcC2S_ACK_Critical_Update:		ONiNet_SH_ACK_Critical_Update(ioServer, ioProcessGroup); break;
			case ONcC2S_ACK_Weapon_Update:			ONiNet_SH_ACK_Weapon_Update(ioServer, ioProcessGroup); break;
			case ONcC2S_Heartbeat:					break; // already handled
			case ONcC2S_Request_CharacterCreate:	ONiNet_SH_Request_CharacterCreate(ioServer, ioProcessGroup); break;
			case ONcC2S_Request_CharacterInfo:		ONiNet_SH_Request_CharacterInfo(ioServer, ioProcessGroup); break;
			case ONcC2S_Request_Join:				ONiNet_SH_Request_Join(ioServer, ioProcessGroup); break;
			case ONcC2S_Request_Spawn:				ONiNet_SH_Request_Spawn(ioServer, ioProcessGroup); break;
			case ONcC2S_Request_Quit:				ONiNet_SH_Request_Quit(ioServer, ioProcessGroup); break;
			case ONcC2S_Request_WeaponInfo:			ONiNet_SH_Request_WeaponInfo(ioServer, ioProcessGroup); break;
			case ONcC2S_Update_Character:			ONiNet_SH_Update_Character(ioProcessGroup); break;
			case ONcC2S_Update_Critical:			ONiNet_SH_Update_Critical(ioServer, ioProcessGroup); break;
			case ONcC2S_Request_ServerInfo:			ONiNet_SH_Request_ServerInfo(ioServer, ioProcessGroup); break;
			default: UUmAssert(!"unknown data type");	break;
		}
		
		prev_datatype = datatype;
	}
}

// ----------------------------------------------------------------------
void
ONrNet_Server_FS_ProcessPacket(
	ONtNet_Server			*ioServer,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	UUtBool					is_legal_packet;
	ONtNet_DataType			prev_datatype;
	UUtUns16				i;
	
	// swap the packet header
	// NOTE:  byte-swapping is only done on incoming data if it needs to be swapped.
	// 		  All outgoing data is written in the processors native endian format
	ioProcessGroup->swap_it = ONiNet_Swap_PacketHeader(&ioProcessGroup->packet);
	
	// make sure the packet can be used
	is_legal_packet =
		ONiNet_Server_IsLegalPacket(
			ioServer,
			ioProcessGroup);
	if (is_legal_packet == UUcFalse) return;
	
	// set the pointers
	ioProcessGroup->datatype_info =
		(ONtNet_DataTypeInfo*)ioProcessGroup->packet.packet_data;
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_2Byte(&ioProcessGroup->datatype_info->num_datatypes);
		UUrSwap_2Byte(&ioProcessGroup->datatype_info->datatypes_offset);
	}
	
	ioProcessGroup->data =
		ioProcessGroup->packet.packet_data +
		sizeof(ONtNet_DataTypeInfo);
	
	ioProcessGroup->datatypes =
		ioProcessGroup->packet.packet_data +
		ioProcessGroup->datatype_info->datatypes_offset;

	// process the data
	prev_datatype = ONcNone;
	for (i = 0; i < ioProcessGroup->datatype_info->num_datatypes; i++)
	{
		ONtNet_DataType					datatype;
		
		// get the ONtNet_DataType
		datatype = ioProcessGroup->datatypes[i];
		
UUmAssert(((UUtUns32)ioProcessGroup->data & 3) == 0);

		switch (datatype)
		{
			case ONcC2S_Request_ServerInfo:		ONiNet_SH_Request_ServerInfo(ioServer, ioProcessGroup); break;
			default:							UUmAssert(!"unknown data type");	break;
		}
		
		prev_datatype = datatype;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
ONrNet_FindServer_Broadcast(
	ONtNet_FindServer		*ioFindServer)
{
	ONtNet_PacketData		packet_data;
	UUtUns16				packet_length;
	
	// initialize the packet data
	ONrNet_Packet_Init(&packet_data);
	
	// add the request
	ONiNet_Packet_AddData(
		&packet_data,
		ONcC2S_Request_ServerInfo,
		NULL,
		0);
	
	// build the packet
	packet_length = ONiNet_Packet_Build(&packet_data);
	
	// initialize the packet header
	packet_data.packet.packet_header.packet_swap		= ONcNet_SwapIt;
	packet_data.packet.packet_header.packet_number		= ioFindServer->packet_number++;
	packet_data.packet.packet_header.packet_version		= ONcNet_VersionNumber;
	packet_data.packet.packet_header.player_index		= ONcNet_UnknownPlayer;
	packet_data.packet.packet_header.packet_length		= packet_length;
	
	// broadcast it
	NMrNetContext_Broadcast(
		ioFindServer->findserver_context,
		ONcNet_FindServerPort,
		(UUtUns8*)&packet_data.packet, 
		packet_data.packet.packet_header.packet_length);
}

// ----------------------------------------------------------------------
static void
ONiNet_FSH_ServerInfo(
	ONtNet_FindServer		*ioFindServer,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	ONtNet_ServerInfo		*server_info;
	
	// get the server info
	server_info = ONmNet_GetDataPtr(ioProcessGroup, ONtNet_ServerInfo);
	
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_2Byte(&server_info->max_players);
		UUrSwap_2Byte(&server_info->num_players);
		UUrSwap_2Byte(&server_info->level_number);
	}
	
	// call the callback
	ioFindServer->callback(server_info, &ioProcessGroup->from, ioFindServer->user_param);
}

// ----------------------------------------------------------------------
static UUtBool
ONiNet_FindServer_IsLegalPacket(
	ONtNet_FindServer		*ioFindServer,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	const ONtNet_Packet		*packet;
	UUtUns16				packet_length;
	
	// get the packet and packet_length from ioProcessGroup
	packet = &ioProcessGroup->packet;
	packet_length = ioProcessGroup->packet_length;
	
	// make sure the packet version is supported
	if (packet->packet_header.packet_version != ONcNet_VersionNumber)
	{
		return UUcFalse;
	}
	
	// make sure the packet sizes are okay
	if (packet->packet_header.packet_length > packet_length)
	{
		return UUcFalse;
	}
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
void
ONrNet_FindServer_ProcessPacket(
	ONtNet_FindServer		*ioFindServer,
	ONtNet_ProcessGroup		*ioProcessGroup)
{
	UUtBool					is_legal_packet;
	ONtNet_DataType			prev_datatype;
	UUtUns16				i;
	
	// swap the packet header
	// NOTE:  byte-swapping is only done on incoming data if it needs to be swapped.
	// 		  All outgoing data is written in the processors native endian format
	ioProcessGroup->swap_it = ONiNet_Swap_PacketHeader(&ioProcessGroup->packet);
	
	// make sure the packet can be used
	is_legal_packet =
		ONiNet_FindServer_IsLegalPacket(
			ioFindServer,
			ioProcessGroup);
	if (is_legal_packet == UUcFalse) return;
	
	// set the pointers
	ioProcessGroup->datatype_info =
		(ONtNet_DataTypeInfo*)ioProcessGroup->packet.packet_data;
	if (ioProcessGroup->swap_it)
	{
		UUrSwap_2Byte(&ioProcessGroup->datatype_info->num_datatypes);
		UUrSwap_2Byte(&ioProcessGroup->datatype_info->datatypes_offset);
	}
	
	ioProcessGroup->data =
		ioProcessGroup->packet.packet_data +
		sizeof(ONtNet_DataTypeInfo);
	
	ioProcessGroup->datatypes =
		ioProcessGroup->packet.packet_data +
		ioProcessGroup->datatype_info->datatypes_offset;

	// process the data
	prev_datatype = ONcNone;
	for (i = 0; i < ioProcessGroup->datatype_info->num_datatypes; i++)
	{
		ONtNet_DataType					datatype;
		
		// get the ONtNet_DataType
		datatype = ioProcessGroup->datatypes[i];
		
UUmAssert(((UUtUns32)ioProcessGroup->data & 3) == 0);

		switch (datatype)
		{
			case ONcS2C_ServerInfo:			ONiNet_FSH_ServerInfo(ioFindServer, ioProcessGroup); break;
			case ONcC2S_Request_ServerInfo: ONiNet_DataAdvance(ioProcessGroup, sizeof(ONtNet_ServerInfo)); break;
			default:						UUmAssert(!"unknown data type");	break;
		}
		
		prev_datatype = datatype;
	}
}

