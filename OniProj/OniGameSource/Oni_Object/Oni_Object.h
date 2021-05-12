// ======================================================================
// Oni_Object.h
// ======================================================================
#pragma once

#ifndef ONI_OBJECT_H
#define ONI_OBJECT_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_ScriptLang.h"
#include "BFW_Particle3.h"
#include "BFW_SoundSystem2.h"
#include "BFW_EnvParticle.h"

#include "Oni_Weapon.h"
#include "Oni_Level.h"
#include "Oni_Camera.h"
#include "Oni_Windows.h"
#include "Oni_AI2_Targeting.h"
#include "Oni_AI2_Alert.h"
#include "Oni_AI2_Patrol.h"
#include "Oni_AI2_MeleeProfile.h"
#include "Oni_AI2_Pursuit.h"

#include "Oni_Event.h"

// ======================================================================
// forward refs
// ======================================================================

typedef struct ONtMechanicsMethods ONtMechanicsMethods;
typedef struct ONtMechanicsClass ONtMechanicsClass;

// ======================================================================
// defines
// ======================================================================

#define OBJcBinaryDataClass			UUm4CharToUns32('O', 'B', 'J', 'C')

#define OBJcMaxNameLength			63

#define ONcMaxPlayerNameLength		(32)

#define OBJcMaxNoteChars			127

#define	OBJcMaxTurretParticles		16

// ======================================================================
// globals
// ======================================================================
extern const char		*OBJgCharacter_TeamName[];
extern const IMtShade	OBJgCharacter_TeamColor[];

// ======================================================================
// enums
// ======================================================================
enum
{
	OBJcVersion_1 = 1,
	OBJcVersion_2,
	OBJcVersion_3,
	OBJcVersion_4,
	OBJcVersion_5,
	OBJcVersion_6,
	OBJcVersion_7,
	OBJcVersion_8,
	OBJcVersion_9,
	OBJcVersion_10,
	OBJcVersion_11,
	OBJcVersion_12,
	OBJcVersion_13,
	OBJcVersion_14,
	OBJcVersion_15,
	OBJcVersion_16,
	OBJcVersion_17,
	OBJcVersion_18,
	OBJcVersion_19,
	OBJcVersion_20,
	OBJcVersion_21,
	OBJcVersion_22,
	OBJcVersion_23,
	OBJcVersion_24,
	OBJcVersion_25,
	OBJcVersion_26,
	OBJcVersion_27,
	OBJcVersion_28,
	OBJcVersion_29,
	OBJcVersion_30,
	OBJcVersion_31,
	OBJcVersion_32,
	OBJcVersion_33,
	OBJcVersion_34,
	OBJcVersion_35,
	OBJcVersion_36,
	OBJcVersion_37,
	OBJcVersion_38,
	OBJcVersion_39,

	OBJcCurrentVersion				= OBJcVersion_39
};


// ----------------------------------------------------------------------
// CB: these IDs are used when writing out gunk, in order to identify the object group of an object
// they must be unique and range from 0-255; do not change them once created.
enum
{
	OBJcTypeIndex_None				= 0,
	OBJcTypeIndex_Character			= 1,
	OBJcTypeIndex_PatrolPath		= 2,
	OBJcTypeIndex_Door				= 3,
	OBJcTypeIndex_Flag				= 4,
	OBJcTypeIndex_Furniture			= 5,
	OBJcTypeIndex_Machine			= 6,
	OBJcTypeIndex_LevelSpecificItem	= 7,
	OBJcTypeIndex_Particle			= 8,
	OBJcTypeIndex_PowerUp			= 9,
	OBJcTypeIndex_Sound				= 10,
	OBJcTypeIndex_TriggerVolume		= 11,
	OBJcTypeIndex_Weapon			= 12,
	OBJcTypeIndex_Trigger			= 13,
	OBJcTypeIndex_Turret			= 14,
	OBJcTypeIndex_Console			= 15,
	OBJcTypeIndex_Combat			= 16,
	OBJcTypeIndex_Melee				= 17,
	OBJcTypeIndex_Neutral			= 18
};

enum
{
	OBJcType_None					= UUm4CharToUns32('N', 'O', 'N', 'E'),
	OBJcType_Character				= UUm4CharToUns32('C', 'H', 'A', 'R'),
	OBJcType_PatrolPath				= UUm4CharToUns32('P', 'A', 'T', 'R'),
	OBJcType_Door					= UUm4CharToUns32('D', 'O', 'O', 'R'),
	OBJcType_Flag					= UUm4CharToUns32('F', 'L', 'A', 'G'),
	OBJcType_Furniture				= UUm4CharToUns32('F', 'U', 'R', 'N'),
	OBJcType_Machine				= UUm4CharToUns32('M', 'A', 'C', 'H'),
	OBJcType_LevelSpecificItem		= UUm4CharToUns32('L', 'S', 'I', 'T'),
	OBJcType_Particle				= UUm4CharToUns32('P', 'A', 'R', 'T'),
	OBJcType_PowerUp				= UUm4CharToUns32('P', 'W', 'R', 'U'),
	OBJcType_Sound					= UUm4CharToUns32('S', 'N', 'D', 'G'),
	OBJcType_TriggerVolume			= UUm4CharToUns32('T', 'R', 'G', 'V'),
	OBJcType_Weapon					= UUm4CharToUns32('W', 'E', 'A', 'P'),
	OBJcType_Trigger				= UUm4CharToUns32('T', 'R', 'I', 'G'),
	OBJcType_Turret					= UUm4CharToUns32('T', 'U', 'R', 'R'),
	OBJcType_Console				= UUm4CharToUns32('C', 'O', 'N', 'S'),
	OBJcType_Combat					= UUm4CharToUns32('C', 'M', 'B', 'T'),
	OBJcType_Melee					= UUm4CharToUns32('M', 'E', 'L', 'E'),
	OBJcType_Neutral				= UUm4CharToUns32('N', 'E', 'U', 'T'),
	OBJcType_Generic				= 0
};


// ----------------------------------------------------------------------
enum
{
	OBJcObjectFlag_None			= 0x0000,
	OBJcObjectFlag_Locked		= 0x0001,
	OBJcObjectFlag_PlacedInGame	= 0x0002,		// was placed via object tool, not loaded
	OBJcObjectFlag_Temporary	= 0x0004,		// do not save
	OBJcObjectFlag_Gunk			= 0x0008		// object has gunk geometry in the environment
};

// ----------------------------------------------------------------------
enum
{
	OBJcMoveMode_LR			= 0x0001,	/* left/right */
	OBJcMoveMode_UD			= 0x0002,	/* up/down */
	OBJcMoveMode_FB			= 0x0004,	/* forward/backward */
	OBJcMoveMode_Wall		= 0x0008,	/* stick to walls (overrides forward/backward) */
	
	OBJcMoveMode_All		= (OBJcMoveMode_LR | OBJcMoveMode_UD | OBJcMoveMode_FB)
};

enum
{
	OBJcRotateMode_XY		= 0x0001,
	OBJcRotateMode_XZ		= 0x0002,
	OBJcRotateMode_YZ		= 0x0004,
	
	OBJcRotateMode_XYZ		= (OBJcRotateMode_XY | OBJcRotateMode_XZ | OBJcRotateMode_XY)
};


// ----------------------------------------------------------------------
enum
{
	OBJcSearch_None					= 0x0000,
	OBJcSearch_FlagID				= 0x0001
};

// ----------------------------------------------------------------------
enum
{
	OBJcSearch_CharacterName		= 0x0001,
	OBJcSearch_CharacterFlags		= 0x0002
};

// ----------------------------------------------------------------------
enum
{
	OBJcSearch_TriggerID			= 0x0001
};

// ----------------------------------------------------------------------
enum
{
	OBJcSearch_PatrolPathName		= 0x0001,
	OBJcSearch_PatrolPathID			= 0x0002
};

enum 
{
	OBJcSearch_CombatName			= 0x0001,
	OBJcSearch_CombatID				= 0x0002
};

enum 
{
	OBJcSearch_MeleeName			= 0x0001,
	OBJcSearch_MeleeID				= 0x0002
};

enum
{
	OBJcSearch_TriggerVolumeName	= 0x0001,
	OBJcSearch_TriggerVolumeID		= 0x0002
};

enum 
{
	OBJcSearch_NeutralName			= 0x0001,
	OBJcSearch_NeutralID			= 0x0002
};

enum
{
	OBJcSearch_DoorID				= 0x0001
};

// ----------------------------------------------------------------------
enum
{
	OBJcCharacter_TeamKonoko				= 0,
	OBJcCharacter_TeamTCTF					= 1,
	OBJcCharacter_TeamSyndicate				= 2,
	OBJcCharacter_TeamNeutral				= 3,
	OBJcCharacter_TeamSecurityGuard			= 4,
	OBJcCharacter_TeamRogueKonoko			= 5,
	OBJcCharacter_TeamSwitzerland			= 6,
	OBJcCharacter_TeamSyndicateAccessory	= 7,
	OBJcCharacter_TeamMaxNamed				= 8
};

// ----------------------------------------------------------------------
enum
{
	OBJcLightFlag_None			= 0x0000,
	OBJcLightFlag_HasLight		= 0x0001,
	
	OBJcLightFlag_Type_Point	= 0x0010,
	OBJcLightFlag_Type_Linear	= 0x0020,
	OBJcLightFlag_Type_Area		= 0x0040,
	
	OBJcLightFlag_Dist_Diffuse	= 0x0100,
	OBJcLightFlag_Dist_Spot		= 0x0200
	
};

// ======================================================================
// typedefs
// ======================================================================
typedef UUtUns32			OBJtObjectType;

typedef struct OBJtObject OBJtObject;
typedef struct OBJtOSD_All OBJtOSD_All;

typedef UUtBool
(*OBJtEnumCallback_ObjectType)(
	OBJtObjectType			inObjectType,
	const char				*inName,
	UUtUns32				inUserData);
	
typedef UUtBool
(*OBJtEnumCallback_Object)(
	OBJtObject				*inObject,
	UUtUns32				inUserData);
	
typedef UUtBool
(*OBJtEnumCallback_ObjectName)(
	const char				*inName,
	UUtUns32				inUserData);

typedef void
(*OBJtMethod_Delete)(
	OBJtObject				*inObject);

typedef void
(*OBJtMethod_Draw)(
	OBJtObject				*inObject,
	UUtUns32				inDrawFlags);

typedef UUtError
(*OBJtMethod_Enumerate)(
	OBJtObject						*inObject,
	OBJtEnumCallback_ObjectName		inEnumCallback,
	UUtUns32						inUserData);

typedef void
(*OBJtMethod_GetBoundingSphere)(
	const OBJtObject		*inObject,
	M3tBoundingSphere		*outBoundingSphere);

typedef void
(*OBJtMethod_OSD_GetName)(
	const OBJtOSD_All		*inObject,
	char					*outName,
	UUtUns32				inNameLength);

typedef void
(*OBJtMethod_OSD_SetName)(
	OBJtOSD_All				*inObject,
	const char				*inName);
	
typedef void
(*OBJtMethod_GetOSD)(
	const OBJtObject		*inObject,
	OBJtOSD_All				*outOSD);

typedef UUtBool
(*OBJtMethod_IntersectsLine)(
	const OBJtObject		*inObject,
	const M3tPoint3D		*inStartPoint,
	const M3tPoint3D		*inEndPoint);

typedef UUtUns32
(*OBJtMethod_GetOSDWriteSize)(
	const OBJtObject		*inObject);

typedef UUtError
(*OBJtMethod_New)(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD);
	
typedef UUtError
(*OBJtMethod_SetDefaults)(
	OBJtOSD_All				*outOSD);
	
typedef UUtUns32
(*OBJtMethod_Read)(
	OBJtObject				*inObject,
	UUtUns16 				inVersion,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer);

typedef void
(*OBJtMethod_UpdatePosition)(
	OBJtObject				*inObject);
	
typedef UUtError
(*OBJtMethod_SetOSD)(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD);

typedef UUtError
(*OBJtMethod_Write)(
	OBJtObject				*inObject,
	UUtUns8					*ioBuffer,
	UUtUns32				*ioBufferSize);
	
typedef const char *
(*OBJtMethod_IsInvalid)(
	OBJtObject				*inObject);

typedef UUtError
(*OBJtMethod_LevelBegin)(
	void);
	
typedef UUtError
(*OBJtMethod_LevelEnd)(
	void);

typedef UUtBool
(*OBJtMethod_GetClassVisible)(
	void);

typedef UUtBool
(*OBJtMethod_Search)(
	const OBJtObject		*inObject,
	const UUtUns32			inSearchType,
	const OBJtOSD_All		*inSearchParams);

typedef void
(*OBJtMethod_SetClassVisible)(
	UUtBool					inIsVisible);

typedef void
(*OBJtMethod_GetUniqueOSD)(
	OBJtOSD_All *outOSD);


typedef struct OBJtMethods
{
	OBJtMethod_New					rNew;
	OBJtMethod_SetDefaults			rSetDefaults;
	OBJtMethod_Delete				rDelete;
	OBJtMethod_IsInvalid			rIsInvalid;
	OBJtMethod_LevelBegin			rLevelBegin;
	OBJtMethod_LevelEnd				rLevelEnd;

	OBJtMethod_Draw					rDraw;
	OBJtMethod_Enumerate			rEnumerate;
	OBJtMethod_GetBoundingSphere	rGetBoundingSphere;
	OBJtMethod_OSD_GetName			rOSDGetName;
	OBJtMethod_OSD_SetName			rOSDSetName;
	OBJtMethod_IntersectsLine		rIntersectsLine;
	OBJtMethod_UpdatePosition		rUpdatePosition;
	
	OBJtMethod_GetOSD				rGetOSD;
	OBJtMethod_GetOSDWriteSize		rGetOSDWriteSize;
	OBJtMethod_SetOSD				rSetOSD;
	
	OBJtMethod_Write				rWrite;
	OBJtMethod_Read					rRead;

	OBJtMethod_Search				rSearch;

	OBJtMethod_GetClassVisible		rGetClassVisible;
	OBJtMethod_SetClassVisible		rSetClassVisible;

	OBJtMethod_GetUniqueOSD			rGetUniqueOSD;
	
} OBJtMethods;

struct OBJtObject
{
	OBJtObjectType					object_type;
	UUtUns32						object_id;
	UUtUns32						flags;
	M3tPoint3D						position;
	M3tPoint3D						rotation;
	OBJtMethods						*methods;
	ONtMechanicsClass				*mechanics_class;
	UUtUns32						object_data[0];	
};
// ----------------------------------------------------------------------
#define OBJcTemplate_LSData				UUm4CharToUns32('O', 'B', 'L', 'S')
typedef tm_template	('O', 'B', 'L', 'S', "Object LS Data")
OBJtLSData
{
	UUtUns32						index;
	UUtUns32						light_flags;
	float							filter_color[3];
	UUtUns32						light_intensity;
	float							beam_angle;
	float							field_angle;
	
} OBJtLSData;

typedef tm_struct OBJtFurnGeom
{
	UUtUns32						gq_flags;
	M3tGeometry						*geometry;
	OBJtLSData						*ls_data;
	
} OBJtFurnGeom;

#define OBJcTemplate_FurnGeomArray		UUm4CharToUns32('O', 'F', 'G', 'A')
typedef tm_template	('O', 'F', 'G', 'A', "Object Furn Geom Array")
OBJtFurnGeomArray
{
	tm_pad							pad0[16];

	EPtEnvParticleArray				*particle_array;

	tm_varindex UUtUns32			num_furn_geoms;
	tm_vararray OBJtFurnGeom		furn_geom[1];

} OBJtFurnGeomArray;

#define OBJcTemplate_TriggerEmitterClass		UUm4CharToUns32('T', 'R', 'G', 'E')
typedef tm_template	('T', 'R', 'G', 'E', "Trigger Emitter")
OBJtTriggerEmitterClass
{
	M3tPoint3D						emit_position;
	M3tVector3D						emit_vector;
	M3tGeometry						*geometry;
	UUtUns32						gq_flags;
} OBJtTriggerEmitterClass;

#define OBJcTemplate_TriggerClass		UUm4CharToUns32('T', 'R', 'I', 'G')
typedef tm_template	('T', 'R', 'I', 'G', "Trigger")
OBJtTriggerClass
{
	UUtUns32						color;			
	UUtUns16						time_on;
	UUtUns16						time_off;
		
	float							start_offset;
	float							anim_scale;

	M3tGeometry						*base_geometry;
	OBJtLSData						*base_ls_data;
	UUtUns32						base_gq_flags;
	
	OBJtTriggerEmitterClass			*emitter;

	OBtAnimation					*animation;

	char							active_soundname[32];		// SScMaxNameLength
	char							trigger_soundname[32];
	tm_raw(SStAmbient *)			active_sound;				// only valid at runtime
	tm_raw(SStImpulse *)			trigger_sound;
} OBJtTriggerClass;

typedef tm_struct OBJtTurretParticleAttachment
{
	char							particle_class_name[16];	// must match P3cStringVarSize + 1 in BFW_Particle3.h
	tm_raw(P3tParticleClass*)		particle_class;				// set up at runtime
	UUtUns16						chamber_time;				// frames before the weapon can fire again (only used for shooters)
	UUtUns16						shooter_index;				// if a shooter: index from 0 to shooter_count if not: attachment is pulsed only when the shooter with this index fires. -1 = pulse every shot.
	M3tMatrix4x3					matrix;						// location and orientation

	tm_pad							pad0[4];
} OBJtTurretParticleAttachment;

#define OBJcTurretNodeCount 32


#define OBJcTemplate_TurretClass		UUm4CharToUns32('T', 'U', 'R', 'R')
typedef tm_template	('T', 'U', 'R', 'R', "Turret")
OBJtTurretClass
{
	char							name[32];
	char							base_name[32];
	
	UUtUns16						flags;
	UUtUns16						freeTime;

	UUtUns16						reloadTime;	
	UUtUns16						barrelCount;

	UUtUns16						recoilAnimType;
	UUtUns16						reloadAnimType;

	UUtUns16						max_ammo;
	UUtUns16						attachment_count;

	UUtUns16						shooter_count;

	tm_pad							pad0[2];

	float							aimingSpeed;

	M3tGeometry						*base_geometry;
	OBJtLSData						*base_ls_data;
	UUtUns32						base_gq_flags;

	M3tGeometry						*turret_geometry;
	UUtUns32						turret_gq_flags;

	M3tGeometry						*barrel_geometry;
	UUtUns32						barrel_gq_flags;

	M3tVector3D						turret_position;
	M3tVector3D						barrel_position;

	OBJtTurretParticleAttachment	attachment[16];

	AI2tWeaponParameters			ai_params;
	AI2tTargetingParameters			targeting_params;
	AI2tShootingSkill				shooting_skill;

	UUtUns32						timeout;

	float							min_vert_angle;	
	float							max_vert_angle;	

	float							min_horiz_angle;
	float							max_horiz_angle;

	float							max_vert_speed;	
	float							max_horiz_speed;

	char							active_soundname[32];		// SScMaxNameLength
	tm_raw(SStAmbient *)			active_sound;				// only valid at runtime
} OBJtTurretClass;

#define OBJcTemplate_ConsoleClass		UUm4CharToUns32('C', 'O', 'N', 'S')
typedef tm_template	('C', 'O', 'N', 'S', "Console")
OBJtConsoleClass
{
	UUtUns32						flags;
	M3tPoint3D						action_position;
	M3tVector3D						action_vector;
	OBJtFurnGeomArray				*geometry_array;
	M3tGeometry						*screen_geometry;
	UUtUns32						screen_gq_flags;

	char							screen_inactive[32];
	char							screen_active[32];
	char							screen_triggered[32];

} OBJtConsoleClass;

#define OBJcTemplate_DoorClass		UUm4CharToUns32('D', 'O', 'O', 'R')
typedef tm_template	('D', 'O', 'O', 'R', "Door")
OBJtDoorClass
{
	OBJtFurnGeomArray				*geometry_array[2];
	OBtAnimation					*animation;

	float							sound_muffle_fraction;	// 0 = no effect, 1 = completely muffled
	UUtUns32						sound_allow;

	UUtUns32						ai_soundtype;		// (UUtUns32) -1 is no AI sound
	float							ai_soundradius;

	char							open_soundname[32];
	char							close_soundname[32];
	tm_raw(SStImpulse *)			open_sound;			// only valid at runtime
	tm_raw(SStImpulse *)			close_sound;		// only valid at runtime
} OBJtDoorClass;



// ----------------------------------------------------------------------
// Object Specific Flags
// ----------------------------------------------------------------------

typedef enum OBJcTurretFlags
{
	OBJcTurretFlag_None				= 0x0000,
	OBJcTurretFlag_Initialized		= 0x0001,
	OBJcTurretFlag_InitialActive	= 0x0002,
	OBJcTurretFlag_IsFiring			= 0x0004,
	OBJcTurretFlag_PlayingSound		= 0x0008,
	OBJcTurretFlag_Visible			= 0x0010,
	OBJcTurretFlag_BackgroundFX		= 0x0020,
	OBJcTurretFlag_Persist			= OBJcTurretFlag_InitialActive
} OBJcTurretFlags;

typedef enum OBJcTurretStates
{
	OBJcTurretState_Inactive		= 0x0000,
	OBJcTurretState_Active			= 0x0001
} OBJcTurretStates;

typedef enum OBJcTriggerFlags
{
	OBJcTriggerFlag_None			= 0x0000,
	OBJcTriggerFlag_Initialized		= 0x0001,
	OBJcTriggerFlag_Triggered		= 0x0002,
	OBJcTriggerFlag_Active			= 0x0004,
	OBJcTriggerFlag_InitialActive	= 0x0008,
	OBJcTriggerFlag_ReverseAnim		= 0x0010,
	OBJcTriggerFlag_PingPong		= 0x0020,
	OBJcTriggerFlag_Hidden			= 0x0040,
	OBJcTriggerFlag_PlayingSound	= 0x0080,
	OBJcTriggerFlag_Persist			= OBJcTriggerFlag_InitialActive | OBJcTriggerFlag_ReverseAnim | OBJcTriggerFlag_PingPong
} OBJcTriggerFlags;

enum
{
	OBJcTriggerState_Inactive		= 0x0000,
	OBJcTriggerState_Active			= 0x0001
};

typedef enum OBJcConsoleClassFlags
{
	OBJcConsoleClassFlag_AlarmPrompt= 0x0001

} OBJcConsoleClassFlags;

typedef enum OBJcConsoleFlags
{
	OBJcConsoleFlag_None			= 0x0000,
	OBJcConsoleFlag_Initialized		= 0x0001,
	OBJcConsoleFlag_Triggered		= 0x0002,
	OBJcConsoleFlag_Active			= 0x0004,
	OBJcConsoleFlag_InitialActive	= 0x0008,
	OBJcConsoleFlag_New				= 0x0010,
	OBJcConsoleFlag_Punch			= 0x0020,
	OBJcConsoleFlag_IsAlarm			= 0x0040,
	OBJcConsoleFlag_Persist			= OBJcConsoleFlag_InitialActive | OBJcConsoleFlag_Punch | OBJcConsoleFlag_IsAlarm
} OBJcConsoleFlags;

enum
{
	OBJcConsoleState_Inactive		= 0x0000,
	OBJcConsoleState_Active			= 0x0001
};

typedef enum OBJcDoorFlags
{
	OBJcDoorFlag_None				= 0x0000,
	OBJcDoorFlag_InitialLocked		= 0x0001,
	OBJcDoorFlag_Locked				= 0x0002,
	OBJcDoorFlag_InDoorFrame		= 0x0004,
	OBJcDoorFlag_Manual				= 0x0010,
	OBJcDoorFlag_Busy				= 0x0020,
	OBJcDoorFlag_TestMode			= 0x0040,
	OBJcDoorFlag_DoubleDoors		= 0x0080,
	OBJcDoorFlag_Mirror				= 0x0100,
	OBJcDoorFlag_OneWay				= 0x0200,
	OBJcDoorFlag_Reverse			= 0x0400,
	OBJcDoorFlag_Jammed				= 0x0800,
	OBJcDoorFlag_InitialOpen		= 0x1000,			// CB: unimplemented but probably a good idea for future
	OBJcDoorFlag_GiveUpOnCollision	= 0x2000

} OBJcDoorFlags;

#define OBJcDoorFlag_Persist		(OBJcDoorFlag_Manual | OBJcDoorFlag_InDoorFrame | OBJcDoorFlag_InitialLocked | \
									 OBJcDoorFlag_DoubleDoors | OBJcDoorFlag_Mirror | OBJcDoorFlag_OneWay | OBJcDoorFlag_Reverse | \
									 OBJcDoorFlag_InitialOpen)

enum
{
	OBJcDoorState_Closed			= 0x0000,
	OBJcDoorState_Open				= 0x0001
};
// ----------------------------------------------------------------------

enum {
	OBJcCharInv_AmmoBallistic	= 0,
	OBJcCharInv_AmmoEnergy		= 1,
	OBJcCharInv_Hypo			= 2,
	OBJcCharInv_ShieldBelt		= 3,
	OBJcCharInv_Invisibility	= 4,
	OBJcCharInv_Max				= 6		// NB: do not change without changing serialisation of character objects
};

enum {
	OBJcCharSlot_Use			= 0,
	OBJcCharSlot_Drop			= 1,
	OBJcCharSlot_Max			= 2		// NB: do not change without changing serialisation of character objects
};

enum {
	OBJcCharFlags_Player				= (1 << 0),
	OBJcCharFlags_RandomCostume			= (1 << 1),
	OBJcCharFlags_NotInitiallyPresent	= (1 << 2),
	OBJcCharFlags_NonCombatant			= (1 << 3),
	OBJcCharFlags_SpawnMultiple			= (1 << 4),
	OBJcCharFlags_Spawned				= (1 << 5),			// runtime only
	OBJcCharFlags_Unkillable			= (1 << 6),
	OBJcCharFlags_InfiniteAmmo			= (1 << 7),
	OBJcCharFlags_Omniscient			= (1 << 8),
	OBJcCharFlags_HasLSI				= (1 << 9),
	OBJcCharFlags_Boss					= (1 << 10),
	OBJcCharFlags_UpgradeDifficulty		= (1 << 11),		// this character gets upgraded one level green -> blue -> red
															// on hard difficulty
	OBJcCharFlags_NoAutoDrop			= (1 << 12)
};

// ----------------------------------------------------------------------

enum {
	OBJcCombatFlags_Alarm_AttackIfKnockedDown	= (1 << 0)
};

// ----------------------------------------------------------------------
// Object Specific Data structures
// ----------------------------------------------------------------------

typedef struct OBJtOSD_Character
{
	UUtUns32				flags;
	char					character_class[64];
	char					character_name[ONcMaxPlayerNameLength];
	char					weapon_class_name[64];

	char					spawn_script[SLcScript_MaxNameLength];
	char					die_script[SLcScript_MaxNameLength];
	char					combat_script[SLcScript_MaxNameLength];
	char					alarm_script[SLcScript_MaxNameLength];
	char					hurt_script[SLcScript_MaxNameLength];
	char					defeated_script[SLcScript_MaxNameLength];
	char					outofammo_script[SLcScript_MaxNameLength];
	char					nopath_script[SLcScript_MaxNameLength];

	UUtInt32				hit_points;			// NB: this is an offset (+15, -20) from class's default HP

	UUtUns16				inventory[OBJcCharInv_Max][OBJcCharSlot_Max];

	UUtUns32				team_number;
	UUtUns32				ammo_percentage;	// 0-100: controls how much currently loaded into weapon

	UUtUns32				job;
	UUtUns16				job_specific_id;
	UUtUns16				combat_id;
	UUtUns16				melee_id;
	UUtUns16				neutral_id;			// 0 = no neutral-character interaction

	UUtUns32				alarm_groups;

	enum AI2tAlertStatus	initial_alert;
	enum AI2tAlertStatus	minimum_alert;
	enum AI2tAlertStatus	jobstart_alert;
	enum AI2tAlertStatus	investigate_alert;

	enum AI2tPursuitMode	pursue_strong_unseen;
	enum AI2tPursuitMode	pursue_weak_unseen;
	enum AI2tPursuitMode	pursue_strong_seen;
	enum AI2tPursuitMode	pursue_weak_seen;

	enum AI2tPursuitLostBehavior	pursue_lost;

	float					facing;					/* internal use only */
} OBJtOSD_Character;

typedef struct OBJtOSD_Combat
{
	char					name[64];

	UUtUns16				id;
	UUtUns16				flags;
	float					medium_range;
	UUtUns32				behavior[5];
	UUtUns32				melee_when;
	UUtUns32				no_gun;
	float					short_range;
	float					pursuit_distance;

	// non-combatant characters only
	UUtUns32				panic_melee;
	UUtUns32				panic_gunfire;
	UUtUns32				panic_sight;
	UUtUns32				panic_hurt;

	// alarm settings
	float					alarm_find_distance;
	float					alarm_ignore_enemy_dist;
	float					alarm_chase_enemy_dist;
	UUtUns32				alarm_damage_threshold;
	UUtUns32				alarm_fight_timer;

} OBJtOSD_Combat;

#define OBJcNeutral_MaxLines			10

typedef struct OBJtOSD_NeutralLine
{
	UUtUns16				flags;
	UUtUns16				pad;
	UUtUns16				anim_type;
	UUtUns16				other_anim_type;
	char					sound[BFcMaxFileNameLength];
} OBJtOSD_NeutralLine;

typedef struct OBJtOSD_Neutral
{
	char					name[32];

	UUtUns16				id;
	UUtUns16				num_lines;
	UUtUns32				flags;

	float					trigger_range;
	float					conversation_range;
	float					follow_range;
	float					abort_enemy_range;

	char					trigger_sound[BFcMaxFileNameLength];
	char					abort_sound[BFcMaxFileNameLength];
	char					enemy_sound[BFcMaxFileNameLength];
	char					end_script[SLcScript_MaxNameLength];

	char					give_weaponclass[WPcMaxWeaponName];
	UUtUns8					give_ballistic_ammo;
	UUtUns8					give_energy_ammo;
	UUtUns8					give_hypos;
	UUtUns8					give_flags;

	OBJtOSD_NeutralLine		lines[OBJcNeutral_MaxLines];

} OBJtOSD_Neutral;

// AI typedefs of object structures
typedef struct AI2tPatrolPath		OBJtOSD_PatrolPath;
typedef struct AI2tMeleeProfile		OBJtOSD_Melee;
typedef struct OBJtOSD_Neutral		AI2tNeutralBehavior;
typedef struct OBJtOSD_NeutralLine	AI2tNeutralLine;

typedef struct OBJtOSD_Flag
{
	IMtShade				shade;
	UUtUns16				id_prefix;
	UUtUns16				id_number;
#if TOOL_VERSION
	char					note[OBJcMaxNoteChars + 1];
#endif
	M3tMatrix4x3			rotation_matrix;		/* internal use only */
	
} OBJtOSD_Flag;

typedef struct OBJtOSD_Furniture
{
	char					furn_geom_name[BFcMaxFileNameLength];
	char					furn_tag[EPcParticleTagNameLength + 1];
	OBJtLSData				*ls_data_array;
	UUtUns32				num_ls_datas;			/* external read-only, internal read/write */
	OBJtFurnGeomArray		*furn_geom_array;		/* internal use only */
	M3tMatrix4x3			matrix;					/* internal use only */	// CB: changed this to combine rot + position
	M3tBoundingSphere		bounding_sphere;		/* internal use only */
	UUtUns32				num_particles;			/* internal use only */
	EPtEnvParticle			*particle;				/* internal use only */
	
} OBJtOSD_Furniture;

// these are kept only for compatibility with object versions before 13
#define OBJcParticleTagNameLength		31

enum {
	OBJcParticleFlag_NotInitiallyCreated	= (1 << 0)
};

typedef struct OBJtOSD_Particle
{
	EPtEnvParticle			particle;

	UUtBool					is_decal;				/* internal use only */
	struct OBJtObject		*owner;					/* internal use only */
	
} OBJtOSD_Particle;

typedef struct OBJtOSD_PowerUp
{
	WPtPowerupType			powerup_type;
	WPtPowerup				*powerup;				/* internal use only */
	
} OBJtOSD_PowerUp;

// ------------------------------
typedef enum OBJtSoundType
{
	OBJcSoundType_Spheres	= 'SPHR',
	OBJcSoundType_BVolume	= 'VLME'
	
} OBJtSoundType;

typedef struct OBJtOSD_Sound_Spheres
{
	float					max_volume_distance;
	float					min_volume_distance;
	
} OBJtOSD_Sound_Spheres;

typedef struct OBJtOSD_Sound_BVolume
{
	M3tBoundingBox_MinMax	bbox;
	M3tBoundingVolume		bvolume;
	
} OBJtOSD_Sound_BVolume;

typedef struct OBJtOSD_Sound
{
	OBJtSoundType			type;
	
	char					ambient_name[SScMaxNameLength];
	SStAmbient				*ambient;				/* internal use only */
	
	float					pitch;
	float					volume;
	
	union
	{
		OBJtOSD_Sound_Spheres		spheres;
		OBJtOSD_Sound_BVolume		bvolume;
		
	} u;
	
} OBJtOSD_Sound;
// ------------------------------

// ==================================================================================================================
// game mechanics
// ==================================================================================================================

typedef struct OBJtTriggerEmitterInstance
{
	float						anim_frame;
	OBtAnimationContext			anim_context;
	M3tMatrix4x3				matrix;
	float						start_offset;

} OBJtTriggerEmitterInstance;

typedef struct OBJtOSD_Trigger
{
	UUtUns16					id;										// persist
	UUtUns16					flags;									// persist
	UUtUns16					state;									// internal

	char						trigger_class_name[OBJcMaxNameLength];	// persist
	OBJtTriggerClass			*trigger_class;							// internal

	UUtUns32					color;									// persist
	float						start_offset;							// persist
	float						persistant_anim_scale;					// persist
	float						current_anim_scale;						// internal
	UUtUns16					emitter_count;							// persist
	UUtUns16					time_off;								// persist
	UUtUns16					time_on;								// persist
	ONtEventList				event_list;								// persist
	OBJtLSData					*base_ls_data;							// internal

	OBJtTriggerEmitterInstance	*emitters;								// internal	

	UUtUns16					change_time;							// internal
	UUtBool						laser_on;								// internal
	M3tPoint3D					*anim_points;							// internal

	M3tBoundingBox_MinMax		bounding_box;							// internal
	M3tMatrix4x3				rotation_matrix;						// internal
	M3tBoundingSphere			bounding_sphere;						// internal

	SStPlayID					playing_sound;							// internal

	M3tPoint3D					*cached_points;							// internal
	
} OBJtOSD_Trigger;

typedef struct OBJtOSD_Turret
{
	UUtUns16					id;										// persist		
	UUtUns16					flags;									// persist
	UUtUns16					state;									// internal

	char						turret_class_name[OBJcMaxNameLength];	// persist
	OBJtTurretClass				*turret_class;							// internal
	
	UUtUns32					target_teams;							// persist		- targeting

	OBJtLSData					*base_ls_data;							// internal
	
	float						vert_angle;								// internal		- actual positioning
	float						horiz_angle;							// internal
	float						desired_horiz_angle;					// internal		- desired positioning
	float						desired_vert_angle;						// internal
	UUtBool						fire_weapon;							// internal

	M3tMatrix4x3				matrix;									// internal
	M3tMatrix4x3				turret_matrix;							// internal
	M3tMatrix4x3				barrel_matrix;							// internal
	M3tMatrix4x3				projectile_matrix;						// internal
	UUtBool						target_los;								// internal
	
	UUtUns16					chamber_time;							// internal		- timing
	UUtUns16					check_target_time;						// internal
	UUtUns32					active_time;							// internal
	UUtUns32					notvisible_time;						// internal

	UUtUns32					particle_id;							// internal
	P3tParticle					*particle[OBJcMaxTurretParticles];		// internal
	P3tParticleReference		particle_ref[OBJcMaxTurretParticles];	// internal

	AI2tWeaponParameters		ai_parameters;							// internal
	AI2tTargetingState			targeting;								// internal
	AI2tTargetingParameters		*targeting_params;						// internal

	M3tMatrix4x3				rotation_matrix;						// internal
	M3tBoundingSphere			bounding_sphere;						// internal

	UUtBool						new_object;								// internal

	SStPlayID					playing_sound;							// internal

	UUtUns32					visible_node_list[OBJcTurretNodeCount];
} OBJtOSD_Turret;

typedef struct OBJtOSD_Console
{
	UUtUns16					id;										// persist
	UUtUns16					flags;									// persist
	UUtUns16					state;									// internal

	char						console_class_name[OBJcMaxNameLength];	// persist
	OBJtConsoleClass			*console_class;							// internal

	char						screen_inactive[OBJcMaxNameLength];		// persist
	char						screen_active[OBJcMaxNameLength];		// persist
	char						screen_triggered[OBJcMaxNameLength];	// persist
	
	ONtEventList				event_list;								// persist

	OBJtLSData					*ls_data_array;							// internal
	UUtUns32					num_ls_datas;							// internal

	ONtActionMarker*			action_marker;							// internal

	M3tTextureMap				*active_screen_texture;					// internal
	M3tTextureMap				*inactive_screen_texture;				// internal
	M3tTextureMap				*triggered_screen_texture;				// internal

	M3tMatrix4x3				matrix;									// internal
	M3tMatrix4x3				rotation_matrix;						// internal
	M3tBoundingSphere			bounding_sphere;						// internal
} OBJtOSD_Console;

typedef struct OBJtOSD_Door
{
	UUtUns16					id;									// persist
	UUtUns16					flags;								// persist
	UUtUns16					key_id;								// persist
	UUtUns16					open_time;							// persist

	float						activation_radius_squared;			// internal

	UUtUns16					state;								// internal
	UUtUns16					blocked_frames;						// internal
	UUtUns16					open_time_left;						// internal
	UUtUns16					proximity_disable_timer;			// internal

	M3tPoint3D					center_offset;						// internal

	char						door_class_name[OBJcMaxNameLength];	// persist
	OBJtDoorClass				*door_class;						// internal
	AKtDoorFrame				*door_frame;						// internal

	OBtObject					*internal_door_object[2];			// internal	
	OBtObjectSetup				*internal_door_object_setup[2];		// setup for those objects

	char						door_texture[2][32];				// persist
	M3tTextureMap				*door_texture_ptr[2];				// internal
	
	M3tPoint3D					door_frame_position;				// persist

	ONtEventList				event_list;							// persist

	OBJtLSData					*ls_data_array;						// internal
	UUtUns32					num_ls_datas;						// internal

	AKtGQ_General				*general_quad;						// internal
	AKtGQ_Render				*render_quad;						// internal

	ONtObjectGunk				*object_gunk;						// internal
	IMtShade					shade;								// internal

	M3tBoundingBox_MinMax		bBox;								// internal
	
	M3tMatrix4x3				door_matrix[2];						// internal

	OBtAnimationContext			animation_context;					// internal			- for testing non-object-gunked doors
	M3tMatrix4x3				animation_matrix;					// internal

	M3tMatrix4x3				matrix;								// internal
	M3tMatrix4x3				rotation_matrix;					// internal
	M3tBoundingSphere			bounding_sphere;					// internal

	PHtConnection				*pathfinding_connection[2];			// internal
	float						pathfinding_conndistsq[2];			// internal
	float						pathfinding_connwidth[2];			// internal
} OBJtOSD_Door;

// ==================================================================================================================
// 
// ==================================================================================================================

enum
{
	OBJcOSD_TriggerVolume_Enter_Once =		0x00000001,
	OBJcOSD_TriggerVolume_Inside_Once =		0x00000002,
	OBJcOSD_TriggerVolume_Exit_Once	=		0x00000004,

	OBJcOSD_TriggerVolume_Enter_Disabled	= 0x00000008,
	OBJcOSD_TriggerVolume_Inside_Disabled	= 0x00000010,
	OBJcOSD_TriggerVolume_Exit_Disabled		= 0x00000020,

	OBJcOSD_TriggerVolume_Disabled			= 0x00000040,
	OBJcOSD_TriggerVolume_PlayerOnly		= 0x00000080
};

typedef struct OBJtOSD_TriggerVolume
{
	char				name[OBJcMaxNameLength];
	char				entry_script[SLcScript_MaxNameLength];
	char				inside_script[SLcScript_MaxNameLength];
	char				exit_script[SLcScript_MaxNameLength];

	char				note[OBJcMaxNoteChars + 1];

	M3tPoint3D			scale;
	UUtInt32			id;
	UUtInt32			parent_id;

	M3tBoundingVolume	volume;
	M3tBoundingSphere	sphere;								// not written to disk
	UUtUns32			team_mask;

	UUtUns32			authored_flags;
	UUtUns32			in_game_flags;						// not written to disk
	char				cur_entry_script[SLcScript_MaxNameLength];// not written to disk
	char				cur_inside_script[SLcScript_MaxNameLength];// not written to disk
	char				cur_exit_script[SLcScript_MaxNameLength];// not written to disk
} OBJtOSD_TriggerVolume;

typedef struct OBJtOSD_Weapon
{
	char						weapon_class_name[WPcMaxWeaponName];
	WPtWeaponClass				*weapon_class;						// internal
	
} OBJtOSD_Weapon;

struct OBJtOSD_All
{
	union
	{
		OBJtOSD_Combat			combat_osd;
		OBJtOSD_Character		character_osd;
		OBJtOSD_PatrolPath		patrolpath_osd;
		OBJtOSD_Flag			flag_osd;
		OBJtOSD_Furniture		furniture_osd;
		OBJtOSD_Particle		particle_osd;
		OBJtOSD_PowerUp			powerup_osd;
		OBJtOSD_Sound			sound_osd;
		OBJtOSD_TriggerVolume	trigger_volume_osd;
		OBJtOSD_Weapon			weapon_osd;
		OBJtOSD_Trigger			trigger_osd;
		OBJtOSD_Turret			turret_osd;
		OBJtOSD_Console			console_osd;
		OBJtOSD_Door			door_osd;
		OBJtOSD_Melee			melee_osd;
		OBJtOSD_Neutral			neutral_osd;
	} osd;
	
};

// ======================================================================
// prototypes
// ======================================================================

UUtUns32 OBJrObjectType_GetOSDSize(
	OBJtObjectType				inObjectType);

static UUcInline void OBJrObject_GetObjectSpecificData(const OBJtObject *inObject, OBJtOSD_All *outOSD)
{
	UUrMemory_Clear(outOSD, OBJrObjectType_GetOSDSize(inObject->object_type));
	inObject->methods->rGetOSD(inObject, outOSD);

	return;
}

UUtError
OBJrObject_SetObjectSpecificData(
	OBJtObject				*ioObject,
	const OBJtOSD_All		*inObjectSpecificData);

UUtError OBJrObject_OSD_SetDefaults(OBJtObjectType inType, OBJtOSD_All *outOSD);
void OBJrObject_OSD_GetName(OBJtObjectType inType, const OBJtOSD_All *inOSD, char *outName, UUtUns32 inNameLength);
void OBJrObject_OSD_SetName(OBJtObjectType inType, OBJtOSD_All *inOSD, const char *inName);

static UUcInline OBJtObjectType OBJrObject_GetType(const OBJtObject *inObject)
{
	return inObject->object_type;
}

static UUcInline UUtError OBJrObject_Enumerate(
	OBJtObject *inObject, 
	OBJtEnumCallback_ObjectName inEnumCallback, 
	UUtUns32 inUserData)
{
	return inObject->methods->rEnumerate(inObject, inEnumCallback, inUserData);
}

static UUcInline void OBJrObject_GetBoundingSphere(const OBJtObject *inObject, M3tBoundingSphere *outBoundingSphere)
{
	inObject->methods->rGetBoundingSphere(inObject, outBoundingSphere);

	return;
}

static UUcInline void OBJrObject_GetName(OBJtObject *inObject, char *outName, UUtUns32 inNameLength)
{
	OBJrObject_OSD_GetName(OBJrObject_GetType(inObject), (OBJtOSD_All *) inObject->object_data, outName, inNameLength);

	return;
}

static UUcInline void OBJrObject_SetName(OBJtObject *inObject, const char *inString)
{
	OBJtOSD_All osd;

	OBJrObject_GetObjectSpecificData(inObject, &osd);
	OBJrObject_OSD_SetName(OBJrObject_GetType(inObject), &osd, inString);
	OBJrObject_SetObjectSpecificData(inObject, &osd);

	return;
}

static UUcInline void OBJrObject_UpdatePosition(OBJtObject *inObject)
{
	inObject->methods->rUpdatePosition(inObject);

	return;
}

static UUcInline void OBJrObject_GetOSD(const OBJtObject *inObject, OBJtOSD_All *outOSD)
{
	inObject->methods->rGetOSD(inObject, outOSD);

	return;
}

static UUcInline UUtBool OBJrObject_IntersectsLine(
	const OBJtObject		*inObject,
	const M3tPoint3D		*inStartPoint,
	const M3tPoint3D		*inEndPoint)
{
	return inObject->methods->rIntersectsLine(inObject, inStartPoint, inEndPoint);
}	

static UUcInline UUtUns32 OBJrObject_GetOSDWriteSize(const OBJtObject *inObject)
{
	return inObject->methods->rGetOSDWriteSize(inObject);
}
	
static UUcInline UUtUns32 OBJrObject_Read(	
	OBJtObject				*inObject,
	UUtUns16 				inVersion,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer)
{
	return inObject->methods->rRead(inObject, inVersion, inSwapIt, inBuffer);
}

UUtError
OBJrObject_CreateFromBuffer(
	UUtUns16				inVersion,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer,
	UUtUns32				*outNumBytesRead);

void
OBJrObject_Delete(
	OBJtObject				*inObject);
	
void
OBJrObject_Draw(
	OBJtObject				*inObject);
	
void
OBJrObject_GetPosition(
	const OBJtObject		*inObject,
	M3tPoint3D				*outPosition,
	M3tPoint3D				*outRotation);
		
void
OBJrObject_GetRotationMatrix(
	const OBJtObject		*inObject,
	M3tMatrix4x3			*outMatrix);
	
UUtBool
OBJrObject_IsLocked(
	const OBJtObject			*inObject);
	
UUtError
OBJrObject_New(
	const OBJtObjectType	inObjectType,
	const M3tPoint3D		*inPosition,
	const M3tPoint3D		*inRotation,
	const OBJtOSD_All		*inObjectSpecificData);

void
OBJrObject_MouseMove(
	OBJtObject				*inObject,
	IMtPoint2D				*inStartPoint,
	IMtPoint2D				*inEndPoint);

void
OBJrObject_MouseRotate_Begin(
	OBJtObject				*inObject,
	IMtPoint2D				*inMousePosition);
	
void
OBJrObject_MouseRotate_Update(
	OBJtObject				*inObject,
	IMtPoint2D				*inPositionDelta);
	
void
OBJrObject_MouseRotate_End(
	void);
	
void
OBJrObject_SetLocked(
	OBJtObject				*ioObject,
	UUtBool					inIsLocked);
		
void
OBJrObject_SetPosition(
	OBJtObject				*ioObject,
	const M3tPoint3D		*inPosition,
	const M3tPoint3D		*inRotation);
	
void
OBJrObject_SetRotationMatrix(
	OBJtObject				*ioObject,
	M3tMatrix4x3			*inMatrix);
	
void
OBJrObject_Write(
	OBJtObject				*inObject,
	UUtUns8					*ioBuffer,
	UUtUns32				*ioNumBytes);

OBJtObject* 
OBJrObject_FindByID( 
	UUtUns32				inID );

// ----------------------------------------------------------------------
UUtUns32
OBJrObjectType_EnumerateObjects(
	OBJtObjectType					inObjectType,
	OBJtEnumCallback_Object			inEnumCallback,
	UUtUns32						inUserData);

OBJtObject *OBJrObjectType_GetObject_ByNumber(
	OBJtObjectType inObjectType,
	UUtUns32 inIndex);
	
void
OBJrObjectType_GetUniqueOSD(
	OBJtObjectType					inObjectType,
	OBJtOSD_All						*outOSD);

UUtBool
OBJrObjectType_GetVisible(
	OBJtObjectType					inObjectType);
	
UUtBool
OBJrObjectType_IsLocked(
	OBJtObjectType					inObjectType);
	
OBJtObject*
OBJrObjectType_Search(
	OBJtObjectType					inObjectType,
	UUtUns32						inSearchType,
	const OBJtOSD_All				*inSearchParams);
	
void
OBJrObjectType_SetVisible(
	OBJtObjectType					inObjectType,
	UUtBool							inIsVisible);
	
void
OBJrObjectTypes_Enumerate(
	OBJtEnumCallback_ObjectType		inEnumCallback,
	UUtUns32						inUserData);
	
const char *
OBJrObjectType_GetName(
	OBJtObjectType					inObjectType);

UUtUns32 OBJrObjectType_GetNumObjects( OBJtObjectType inObjectType );
UUtError OBJrObjectType_GetObjectList( OBJtObjectType inObjectType, OBJtObject ***ioList, UUtUns32 *ioObjectCount );
UUtError OBJrObjectType_RegisterMechanics( OBJtObjectType inObjectType, ONtMechanicsClass *inMechanicsClass );
ONtMechanicsClass* OBJrObjectType_GetMechanicsByType( OBJtObjectType inObjectType );


// ----------------------------------------------------------------------
UUtError
OBJrENVFile_Write(
	UUtBool inDebug);

// ----------------------------------------------------------------------
UUtError
OBJrFlagTXTFile_Write(
	void);

UUtUns16
OBJrFlag_GetUniqueID(
	void);

// ----------------------------------------------------------------------
UUtError
OBJrPatrolPath_ReadPathData(
	OBJtObject *inObject,
	OBJtOSD_PatrolPath *ioPath);

UUtError
OBJrPatrolPath_WritePathData(
	OBJtObject *ioObject,
	OBJtOSD_PatrolPath *inPath);

// ----------------------------------------------------------------------
UUtError
OBJrLevel_Load(
	UUtUns16				inLevelNumber);
	
void
OBJrLevel_Unload(
	void);
	
// ----------------------------------------------------------------------
void
OBJrSelectedObjects_Delete(
	void);
	
UUtUns32
OBJrSelectedObjects_GetNumSelected(
	void);
		
OBJtObject*
OBJrSelectedObjects_GetSelectedObject(
	UUtUns32				inIndex);
	
UUtBool
OBJrSelectedObjects_IsObjectSelected(
	const OBJtObject		*inObject);

UUtBool
OBJrSelectedObjects_Lock_Get(
	void);
	
void
OBJrSelectedObjects_Lock_Set(
	UUtBool					inLock);
	
void
OBJrSelectedObjects_Select(
	OBJtObject				*inObject,
	UUtBool					inAddToList);
	
void
OBJrSelectedObjects_Unselect(
	OBJtObject				*inObject);
	
void
OBJrSelectedObjects_UnselectAll(
	void);
	
void
OBJrSelectedObjects_MoveCameraToSelection(
	void);

// ----------------------------------------------------------------------
UUtUns32
OBJrMoveMode_Get(
	void);

void
OBJrMoveMode_Set(
	UUtUns32				inMoveMode);
	
UUtUns32
OBJrRotateMode_Get(
	void);
	
void
OBJrRotateMode_Set(
	UUtUns32				inRotateMode);
	
// ----------------------------------------------------------------------
void
OBJrDrawObjects(
	void);
	
OBJtObject*
OBJrGetObjectUnderPoint(
	IMtPoint2D				*inPoint);
	
UUtError
OBJrInitialize(
	void);

UUtError
OBJrRegisterTemplates(
	void);
	
void
OBJrSaveObjects(
	UUtUns32				inObjectType);
	
void
OBJrTerminate(
	void);

// ======================================================================
// furniture
// ======================================================================

void OBJrFurniture_DrawArray( OBJtFurnGeomArray* inGeometryArray, UUtUns32 inDrawFlags );
UUtError OBJrFurniture_CreateParticles(void);

// ======================================================================
// sound
// ======================================================================

SStAmbient*
OBJrSound_GetAmbient(
	const OBJtObject		*inObject);

UUtBool
OBJrSound_GetMinMaxDistances(
	const OBJtObject		*inObject,
	float					*outMaxVolumeDistance,
	float					*outMinVolumeDistance);
	
float
OBJrSound_GetPitch(
	const OBJtObject		*inObject);
	
OBJtSoundType
OBJrSound_GetType(
	const OBJtObject		*inObject);
	
float
OBJrSound_GetVolume(
	const OBJtObject		*inObject);
	
UUtBool
OBJrSound_PointIn(
	const OBJtObject		*inObject,
	const M3tPoint3D		*inPoint);

// ======================================================================
// turrets syndrome
// ======================================================================

void OBJrTurret_UpdateSoundPointers(void);

UUtError OBJrTurret_Activate_ID( UUtUns16 inID );
UUtError OBJrTurret_Deactivate_ID( UUtUns16 inID );
UUtError OBJrTurret_Reset_ID( UUtUns16 inID );

void OBJrTurret_Activate( OBJtObject *inObject );
void OBJrTurret_Deactivate( OBJtObject *inObject );

void OBJrTurret_RecreateParticles(void);

// ======================================================================
// triggers
// ======================================================================

void OBJrTrigger_UpdateSoundPointers(void);

void OBJrTrigger_Activate_ID( UUtUns16 inID );
void OBJrTrigger_Deactivate_ID( UUtUns16 inID );
void OBJrTrigger_Reset_ID( UUtUns16 inID );
void OBJrTrigger_Hide_ID(UUtUns16 inID);
void OBJrTrigger_Show_ID(UUtUns16 inID);
void OBJrTrigger_SetSpeed_ID(UUtUns16 inID, float inSpeed);

void OBJrTrigger_AddEvent( OBJtOSD_Trigger *inTrigger_osd, ONtEvent *inEvent );
void OBJrTrigger_DeleteEvent( OBJtOSD_Trigger *inTrigger_osd, UUtUns32 inIndex );

// ======================================================================
// consoles
// ======================================================================

UUtError OBJrConsole_OnActivate( OBJtObject *inObject, ONtCharacter *inCharacter );

UUtError OBJrConsole_DeleteEvent( OBJtOSD_Console *inConsole_osd, UUtUns32 inIndex );
UUtError OBJrConsole_AddEvent( OBJtOSD_Console *inConsole_osd, ONtEvent *inEvent );

void OBJrConsole_Activate_ID(UUtUns16 inID);
void OBJrConsole_Deactivate_ID(UUtUns16 inID);
void OBJrConsole_Reset_ID(UUtUns16 inID);

OBJtObject *OBJrConsole_GetByID(UUtUns16 inID);

// ======================================================================
// The Doors
// ======================================================================

extern UUtBool OBJgDoor_DrawFrames;
#define OBJcDoor_EitherSide				((UUtUns8) 0xff)

void OBJrDoor_UpdateSoundPointers(void);

OBJtObject* OBJrDoor_FindDoorWithID( UUtUns16 inID );
void OBJrDoor_MakeConnectionLink(struct PHtConnection *ioConnection, M3tPoint3D *inSidePoint);
UUtUns32 OBJrDoor_ComputeObstruction(OBJtObject *inObject, UUtUns32 *outDoorID, M3tPoint3D *outDoorPoints);

UUtUns8 OBJrDoor_PointOnSide(M3tPoint3D *inPoint, OBJtObject *inObject);
UUtBool OBJrDoor_CharacterCanOpen(ONtCharacter* inCharacter, OBJtObject *inObject, UUtUns8 inSide);
UUtBool OBJrDoor_CharacterProximity(OBJtObject *inObject, ONtCharacter *inCharacter);
UUtBool OBJrDoor_CharacterOpen(ONtCharacter* inCharacter, OBJtObject *inObject);
UUtBool OBJrDoor_TryDoorAction(ONtCharacter *inCharacter, OBJtObject **outClosestDoor);
UUtBool OBJrDoor_IsOpen(OBJtObject *inObject);
UUtBool OBJrDoor_OpensManually(OBJtObject *inObject);

UUtError OBJrDoor_Lock_ID( UUtUns16 inID );
UUtError OBJrDoor_Unlock_ID( UUtUns16 inID );

OBJtObject *OBJrDoor_GetByID(UUtUns16 inID);

// jammed doors can't be opened by characters, only by scripting control
void OBJrDoor_Jam(UUtUns16 inID, UUtBool inJam);

void OBJrDoor_ForceOpen(UUtUns16 inID);
void OBJrDoor_ForceClose(UUtUns16 inID);

void OBJrDoor_JumpToNextDoor();
void OBJrDoor_JumpToPrevDoor();

#if TOOL_VERSION
// debugging routines
UUtError OBJrDoor_DumpAll(void);
UUtError OBJrDoor_DumpNearby(M3tPoint3D *inPoint, float inRadius);
#endif

// ======================================================================
// The Trigger Volumes
// ======================================================================

UUtBool OBJrTriggerVolume_IntersectsBBox_Approximate(const OBJtObject *inObject, const M3tBoundingBox_MinMax *inBBox);
UUtBool OBJrTriggerVolume_IntersectsCharacter(const OBJtObject *inObject, UUtUns32 inMask, const ONtCharacter *inCharacter);
char *OBJrTriggerVolume_IsInvalid(OBJtObject *inObject);
UUtInt32 OBJrTriggerVolumeID_CountInside(UUtInt32 id);
void OBJrTriggerVolumeID_DeleteCorpsesInside(UUtInt32 id);
void OBJrTriggerVolumeID_DeleteCharactersInside(UUtInt32 id);

UUtError
OBJrTriggerVolume_Reset(
	OBJtObject				*inObject);

// ======================================================================
WPtPowerupType
OBJrPowerUp_NameToType(
	const char				*inPowerUpName);

// ======================================================================

UUtError OBJrObject_LevelBegin(void);
UUtError OBJrObject_LevelEnd(void);

// resets and add all objects of inType 
// into the listbox sorted with the item
// data equal tobject pointers

void OBJrObjectType_BuildListBox(
	OBJtObjectType inObjectType, 
	WMtWindow *ioListBox,
	UUtBool inAllowNone);

// brings up a dialog which allows you to create, delete, duplicate and select objects
OBJtObject *OWrSelectObject(OBJtObjectType inObjectType, OBJtObject *inSelectedObject, UUtBool inAllowNone, UUtBool inAllowGoto);

// **** ID to object functions

UUtBool ONrLevel_Path_ID_To_Path(UUtInt16 inID, OBJtOSD_PatrolPath *outPath);
UUtBool ONrLevel_Combat_ID_To_Combat(UUtUns16 inID, OBJtOSD_Combat *outCombat);
UUtBool ONrLevel_Neutral_ID_To_Neutral(UUtUns16 inID, OBJtOSD_Neutral *outNeutral);
UUtBool ONrLevel_Melee_ID_To_Melee_Ptr(UUtUns16 inID, OBJtOSD_Melee **outMelee);

// conversion helpers

UUtError StringToBitMask( UUtUns32 *ioMask, char* inString );
UUtError BitMaskToString( UUtUns32 inMask, char* ioString, UUtUns32 inStringSize );

// id managment
UUtUns32 OBJrObject_GetNextAvailableID();



#endif /* ONI_OBJECT_H */
