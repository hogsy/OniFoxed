// ======================================================================
// Oni_Obj_Tools.h
// ======================================================================
#ifndef ONI_OBJ_TOOLS_H
#define ONI_OBJ_TOOLS_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

#include "Oni_Weapon.h"
#include "Oni_Level.h"

// ======================================================================
// enums
// ======================================================================
typedef enum OBJtObjectType
{
	OBJcType_None					= UUm4CharToUns32('N', 'O', 'N', 'E'),
	OBJcType_Character				= UUm4CharToUns32('C', 'H', 'A', 'R'),
	OBJcType_Door					= UUm4CharToUns32('D', 'O', 'O', 'R'),
	OBJcType_Flag					= UUm4CharToUns32('F', 'L', 'A', 'G'),
	OBJcType_Furniture				= UUm4CharToUns32('F', 'U', 'R', 'N'),
	OBJcType_Machine				= UUm4CharToUns32('M', 'A', 'C', 'H'),
	OBJcType_LevelSpecificItem		= UUm4CharToUns32('L', 'S', 'I', 'T'),
	OBJcType_PowerUp				= UUm4CharToUns32('P', 'W', 'R', 'U'),
	OBJcType_Sound					= UUm4CharToUns32('S', 'N', 'D', 'G'),
	OBJcType_TriggerVolume			= UUm4CharToUns32('T', 'R', 'G', 'V'),
	OBJcType_Weapon					= UUm4CharToUns32('W', 'E', 'A', 'P')
	
} OBJtObjectType;

// ----------------------------------------------------------------------
typedef enum OBJtFlagType
{
	OBJcFlagType_None				= UUm4CharToUns32('N', 'O', 'N', 'E'),
	OBJcFlagType_CharacterStart		= UUm4CharToUns32('C', 'H', 'A', 'R'),
	OBJcFlagType_MultiplayerSpawn	= UUm4CharToUns32('M', 'P', 'S', 'P'),
	OBJcFlagType_Waypoint			= UUm4CharToUns32('W', 'A', 'Y', 'P')
	
} OBJtFlagType;

typedef enum OBJtPowerUpType
{
	OBJcPowerUpType_None			= UUm4CharToUns32('N', 'O', 'N', 'E'),
	OBJcPowerUpType_Hypo			= UUm4CharToUns32('H', 'Y', 'P', 'O'),
	OBJcPowerUpType_ShieldGreen		= UUm4CharToUns32('S', 'H', 'D', 'G'),
	OBJcPowerUpType_ShieldBlue		= UUm4CharToUns32('S', 'H', 'D', 'B'),
	OBJcPowerUpType_ShieldRed		= UUm4CharToUns32('S', 'H', 'D', 'R'),
	OBJcPowerUpType_AmmoEnergy	 	= UUm4CharToUns32('A', 'M', 'O', 'E'),
	OBJcPowerUpType_AmmoBallistic 	= UUm4CharToUns32('A', 'M', 'O', 'B')
	
} OBJtPowerUpType;

enum
{
	WPcInventory_Use				= 0,
	WPcInventory_Drop				= 1
	
};

// ======================================================================
// typedefs
// ======================================================================
typedef struct OBJtObject	OBJtObject;

typedef UUtBool
(*OBJtObjectEnumCallback)(
	OBJtObjectType			inObjectType,
	char					*inObjectName,
	UUtUns32				inUserData);
	
// ----------------------------------------------------------------------
// Object Specific Data structures
// ----------------------------------------------------------------------
typedef struct OBJtOSD_Character
{
	char					character_class[64];
	char					character_name[ONcMaxPlayerNameLength];
	char					weapon_class_name[64];
	UUtUns32				hit_points;
	UUtUns16				ammo_ballistic[2];
	UUtUns16				ammo_energy[2];
	UUtUns16				hypo[2];
	UUtUns16				shield_green[2];
	UUtUns16				shield_blue[2];
	UUtUns16				shield_red[2];
	UUtUns16				team_number;
	
	UUtUns16				character_index;		/* internal use only */
	
} OBJtOSD_Character;

typedef struct OBJtOSD_Flag
{
	OBJtFlagType			flag_type;
	UUtUns16				flag_index;				/* internal use only */
	
} OBJtOSD_Flag;

typedef struct OBJtOSD_Furniture
{
	char					geometry_name[BFcMaxFileNameLength];
	M3tGeometry				*geometry;				/* internal use only */
	M3tMatrix4x3			rotation_matrix;		/* internal use only */
	
} OBJtOSD_Furniture;

typedef struct OBJtOSD_PowerUp
{
	OBJtPowerUpType			powerup_type;
	WPtPowerup				*powerup;				/* internal use only */
	
} OBJtOSD_PowerUp;

typedef struct OBJtOSD_Sound
{
	SStSound				*sound;					/* internal use only */
	
} OBJtOSD_Sound;

typedef struct OBJtOSD_TrigVol
{
	ONtTrigger				*trigger;				/* internal use only */
	M3tPoint3D				scale;
//	M3tQuaternion			orientation;
	UUtInt32				id;
	
} OBJtOSD_TrigVol;

typedef struct OBJtOSD_Weapon
{
	char					weapon_class_name[BFcMaxFileNameLength];
	WPtWeapon				*weapon;				/* internal use only */
	
} OBJtOSD_Weapon;

typedef struct OBJtOSD_All
{
	union
	{
		OBJtOSD_Character		character_osd;
		OBJtOSD_Flag			flag_osd;
		OBJtOSD_Furniture		furniture_osd;
		OBJtOSD_PowerUp			powerup_osd;
		OBJtOSD_Sound			sound_osd;
		OBJtOSD_TrigVol			trigvol_osd;
		OBJtOSD_Weapon			weapon_osd;
	};
	
} OBJtOSD_All;

// ======================================================================
// prototypes
// ======================================================================
void
OBJrOSD_FlagToName(
	UUtUns32				inFlag,
	OBJtObjectType			inObjectType,
	char					*outName);
	
void
OBJrOSD_NameToFlag(
	char					*inName,
	OBJtObjectType			inObjectType,
	UUtUns32				*outFlag);
	
// ----------------------------------------------------------------------
void
OBJrObjectList_DrawObjects(
	void);
	
// ----------------------------------------------------------------------
void
OBJrObject_Draw(
	OBJtObject				*inObject);
	
UUtError
OBJrObject_Enumerate(
	OBJtObjectType			inObjectType,
	OBJtObjectEnumCallback	inObjectEnumCallback,
	UUtUns32				inUserData);
	
void
OBJrObject_GetObjectSpecificData(
	OBJtObject				*inObject,
	OBJtOSD_All				*outObjectSpecificData);

void
OBJrObject_GetPosition(
	OBJtObject				*inObject,
	M3tPoint3D				*outPosition,
	M3tPoint3D				*outRotation);

OBJtObject*
OBJrObject_GetSelectedObject(
	void);
	
OBJtObjectType
OBJrObject_GetType(
	OBJtObject				*inObject);
	
UUtBool
OBJrObject_IntersectsLine(
	OBJtObject				*inObject,
	M3tPoint3D				*inStartPoint,
	M3tPoint3D				*inEndPoint);
	
UUtError
OBJrObject_New(
	OBJtObjectType			inObjectType,
	M3tPoint3D				*inPosition,
	M3tPoint3D				*inRotation,
	OBJtOSD_All				*inObjectSpecificData);

void
OBJrObject_Delete(
	OBJtObject				*inObject);
	
void
OBJrObject_DeleteSelected(
	void);

UUtError
OBJrObject_Select(
	OBJtObject				*inObject);
	
UUtError
OBJrObject_SetObjectSpecificData(
	OBJtObject				*ioObject,
	OBJtOSD_All				*inObjectSpecificData);
	
void
OBJrObject_SetPosition(
	OBJtObject				*ioObject,
	M3tPoint3D				*inPosition,
	M3tPoint3D				*inRotation);
	
// ----------------------------------------------------------------------
UUtError
OBJrLevel_Load(
	UUtUns16				inLevelNum);

UUtError
OBJrLevel_SaveObjects(
	void);

void
OBJrLevel_Unload(
	void);
	
// ----------------------------------------------------------------------
OBJtObject*
OBJrGetObjectUnderPoint(
	IMtPoint2D				*inPoint);

void
OBJrDrawTriggers(
	UUtBool					inDrawTriggers);

// ----------------------------------------------------------------------
UUtError
OBJrENVFile_Write(
	void);

// ======================================================================
#endif /* ONI_OBJ_TOOLS_H */