#pragma once

#ifndef __BFW_PARTICLE3_H__
#define __BFW_PARTICLE3_H__

/*
	FILE:	BFW_Particle3.h
	
	AUTHOR:	Chris Butcher
	
	CREATED: Feb 02, 2000
	
	PURPOSE: definitions for new particle system (iteration 3)
	
	Copyright 2000

 */

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_BinaryData.h"
#include "BFW_Physics.h"
#include "BFW_SoundSystem2.h"

#if TOOL_VERSION
#define PARTICLE_PERF_ENABLE				1
#else
#define PARTICLE_PERF_ENABLE				0
#endif

// =============================================
// structure definitions
// =============================================


/*
 * Definitions for particle data
 */


// P3tDataType is a bitvector that describes a number of permissible
// data types for an action's parameter - or it may be a single value
// that describes the data type of a variable.
typedef enum P3tDataType
{
	P3cDataType_None			= 0,
	P3cDataType_Integer			= (1 << 0),
	P3cDataType_Float			= (1 << 1),
	P3cDataType_String			= (1 << 2),
	P3cDataType_Shade			= (1 << 3),
	P3cDataType_Enum			= (256 << 4),
	P3cDataType_Sound_Ambient	= (1 << 13),
	P3cDataType_Sound_Impulse	= (1 << 14)
	
} P3tDataType;

// P3tEnumType is used in combination with P3cDataType_Enum to store
// the fact that a parameter is an enumerated type, of a specific kind
// e.g. P3cDataType_Enum | P3cEnumType_BlendMode means that a parameter
// is a blend mode.
#define P3cEnumShift	4
typedef enum P3tEnumType
{
	P3cEnumType_Pingpong			= (0 << P3cEnumShift),
	P3cEnumType_ActionIndex			= (1 << P3cEnumShift),
	P3cEnumType_Emitter				= (2 << P3cEnumShift),
	P3cEnumType_Falloff				= (3 << P3cEnumShift),
	P3cEnumType_CoordFrame			= (4 << P3cEnumShift),
	P3cEnumType_CollisionOrientation= (5 << P3cEnumShift),
	P3cEnumType_Bool				= (6 << P3cEnumShift),
	P3cEnumType_AmbientSoundID		= (7 << P3cEnumShift),
	P3cEnumType_ImpulseSoundID		= (8 << P3cEnumShift),
	P3cEnumType_ImpactModifier		= (9 << P3cEnumShift),
	P3cEnumType_DamageType			= (10 << P3cEnumShift),
	P3cEnumType_Axis				= (11 << P3cEnumShift),

	P3cEnumType_Max					= (12 << P3cEnumShift)
} P3tEnumType;

#define P3cStringVarSize      15
typedef char P3tString[P3cStringVarSize + 1];

// info stored about each enumeration type
typedef struct P3tEnumTypeInfo {
	P3tEnumType		type;
	P3tString		name;
	UUtBool			not_handled;		// if this is set, the windowing system has to handle the valid-range & default
	UUtUns32		min, max;
	UUtUns32		defaultval;
} P3tEnumTypeInfo;

/*
 * enumerated types
 */

// pingpong animation: either up or down
typedef enum P3tEnumPingpong {
	P3cEnumPingpong_Up				= 0,
	P3cEnumPingpong_Down			= 1,

	P3cEnumPingpong_Max				= 2
} P3tEnumPingpong;

// blast falloff
typedef enum P3tEnumFalloff {
	P3cEnumFalloff_None				= 0,
	P3cEnumFalloff_Linear			= 1,
	P3cEnumFalloff_InvSquare		= 2,

	P3cEnumFalloff_Max				= 3
} P3tEnumFalloff;

typedef enum P3tEnumDamageType {
	P3cEnumDamageType_Normal			= 0,
	P3cEnumDamageType_MinorStun			= 1,
	P3cEnumDamageType_MajorStun			= 2,
	P3cEnumDamageType_MinorKnockdown	= 3,
	P3cEnumDamageType_MajorKnockdown	= 4,
	P3cEnumDamageType_Blownup			= 5,
	P3cEnumDamageType_PickUp			= 6,

	P3cEnumDamageType_Max				= 7,		// must match size of ONtCharacterClass->damageResistance

	P3cEnumDamageType_First				= P3cEnumDamageType_Normal,
	P3cEnumDamageType_Last				= P3cEnumDamageType_PickUp
} P3tEnumDamageType;

// coordinate frame
typedef enum P3tEnumCoordFrame {
	P3cEnumCoordFrame_Local			= 0,
	P3cEnumCoordFrame_World			= 1,

	P3cEnumCoordFrame_Max			= 2
} P3tEnumCoordFrame;

// collision effect orientation
typedef enum P3tEnumCollisionOrientation {
	P3cEnumCollisionOrientation_Unchanged	= 0,
	P3cEnumCollisionOrientation_Reversed	= 1,
	P3cEnumCollisionOrientation_Reflected	= 2,
	P3cEnumCollisionOrientation_Normal		= 3,

	P3cEnumCollisionOrientation_Max			= 4
} P3tEnumCollisionOrientation;

// boolean
typedef enum P3tEnumBool {
	P3cEnumBool_False						= 0,
	P3cEnumBool_True						= 1,

	P3cEnumBool_Max							= 2
} P3tEnumBool;

// axis
typedef enum P3tEnumAxis {
	P3cEnumAxis_PosX						= 0,
	P3cEnumAxis_NegX						= 1,
	P3cEnumAxis_PosY						= 2,
	P3cEnumAxis_NegY						= 3,
	P3cEnumAxis_PosZ						= 4,
	P3cEnumAxis_NegZ						= 5,

	P3cEnumAxis_Max							= 6
} P3tEnumAxis;

/*
 * definitions for value generation
 */

// A reference to a variable that is modified by an action.
typedef struct P3tVariableReference {
	P3tString			name;		// used by P3rGetVarOffset
	P3tDataType			type;		// used by actions to make sure they have the right var type
	UUtUns16			offset;		// calculated on var packing
} P3tVariableReference;

// Types of values
typedef enum P3tValueType {
	P3cValueType_Variable			= 0,
	P3cValueType_Integer			= 1,
	P3cValueType_IntegerRange		= 2,
	P3cValueType_Float				= 3,
	P3cValueType_FloatRange			= 4,
	P3cValueType_FloatBellcurve		= 5,
	P3cValueType_String				= 6,
	P3cValueType_Shade				= 7,
	P3cValueType_ShadeRange			= 8,
	P3cValueType_ShadeBellcurve		= 9,
	P3cValueType_Enum				= 10,
	P3cValueType_FloatTimeBased		= 11,
	// if necessary, add a random enum type here that picks randomly from a number
	// of settings
	P3cValueType_NumTypes			= 12
} P3tValueType;


// Data structures that store the parameters for each kind of value
typedef struct P3tValue_Integer {
	UUtUns16	val;
} P3tValue_Integer;

typedef struct P3tValue_IntegerRange {
	UUtUns16	val_low;
	UUtUns16	val_hi;
} P3tValue_IntegerRange;

typedef struct P3tValue_Float {
	float		val;
} P3tValue_Float;

typedef struct P3tValue_FloatRange {
	float		val_low;
	float		val_hi;
} P3tValue_FloatRange;

typedef struct P3tValue_FloatBellcurve {
	float		val_mean;
	float		val_stddev;
} P3tValue_FloatBellcurve;

typedef struct P3tValue_FloatTimeBased {
	float		cycle_length;
	float		cycle_max;
} P3tValue_FloatTimeBased;

typedef struct P3tValue_String {
	P3tString	val;
} P3tValue_String;

typedef struct P3tValue_Shade {
	IMtShade	val;
} P3tValue_Shade;

typedef struct P3tValue_ShadeRange {
	IMtShade	val_low;
	IMtShade	val_hi;
} P3tValue_ShadeRange;

typedef struct P3tValue_ShadeBellcurve {
	IMtShade	val_mean;
	IMtShade	val_stddev;
} P3tValue_ShadeBellcurve;

typedef struct P3tValue_Enum {
	UUtUns32	val;
} P3tValue_Enum;

// Information stored about each value in a particle class.
//
// A value is something that a variable can be set to, or that
// can be passed into an action as a parameter. It may be a constant,
// a simple rule for generating a number, or a reference to a variable.
typedef struct P3tValue {
	P3tValueType	type;
	union {
		P3tVariableReference	variable;
		P3tValue_Integer		integer_const;
		P3tValue_IntegerRange	integer_range;
		P3tValue_Float			float_const;
		P3tValue_FloatRange		float_range;
		P3tValue_FloatBellcurve	float_bellcurve;
		P3tValue_String			string_const;
		P3tValue_Shade			shade_const;
		P3tValue_ShadeRange		shade_range;
		P3tValue_ShadeBellcurve	shade_bellcurve;
		P3tValue_Enum			enum_const;
		P3tValue_FloatTimeBased	float_timebased;
	} u;
} P3tValue;

// Assert that a value is of a particular type... necessary because P3iCalculateValue
// writes its data into a void * and we need some kind of sanity check.
#define P3mAssertFloatValue(val) {								\
	UUmAssert((val.type == P3cValueType_Float) ||				\
			  (val.type == P3cValueType_FloatRange) ||			\
			  (val.type == P3cValueType_FloatBellcurve) ||		\
			  (val.type == P3cValueType_FloatTimeBased) ||		\
			  ((val.type == P3cValueType_Variable) &&			\
			   (val.u.variable.type == P3cDataType_Float)));	\
}

#define P3mAssertStringValue(val) {								\
	UUmAssert((val.type == P3cValueType_String) ||				\
			  ((val.type == P3cValueType_Variable) &&			\
			   (val.u.variable.type == P3cDataType_String)));	\
}

#define P3mAssertShadeValue(val) {								\
	UUmAssert((val.type == P3cValueType_Shade) ||				\
			  (val.type == P3cValueType_ShadeRange) ||			\
			  (val.type == P3cValueType_ShadeBellcurve) ||		\
			  ((val.type == P3cValueType_Variable) &&			\
			   (val.u.variable.type == P3cDataType_Shade)));	\
}

#define P3mAssertEnumValue(val, enumtype) {								\
	UUmAssert((val.type == P3cValueType_Enum) ||						\
			  ((val.type == P3cValueType_Variable) &&					\
			   (val.u.variable.type == (P3cDataType_Enum | (enumtype)))));	\
}


/*
 * definitions for variable bookkeeping
 */

// Information stored about each variable in a particle class.
typedef struct P3tVariableInfo {
	P3tString			name;
	P3tDataType			type;
	UUtUns16			offset;		// calculated on var packing
	P3tValue			initialiser;// cannot be P3cValueType_Variable
} P3tVariableInfo;

// a routine that can iterate over all variable references
typedef void (*P3tVarTraverseProc)(P3tVariableReference *ioVarRef, char *inLocation);


/*
 * Action procedures - describe actions that a particle can perform.
 *
 * each action procedure has a certain number of parameters associated with it.
 * these are stored in a P3tActionStruct_SomeKindOfAction structure
 */

// An action list defines a set of actions (that must be stored
// sequentially in the P3tParticleDefinition).
typedef struct P3tActionList {
	UUtUns16		start_index;
	UUtUns16		end_index;
} P3tActionList;

// The kinds of actions.
typedef enum P3tActionType {

	// *** animation
	P3cActionType_AnimateLinear			= 0,
	P3cActionType_AnimateAccel			= 1,
	P3cActionType_AnimateRandom			= 2,
	P3cActionType_AnimatePingpong		= 3,
	P3cActionType_AnimateLoop			= 4,
	P3cActionType_AnimateToValue		= 5,
	P3cActionType_ColorInterpolate		= 6,
	P3cActionType_Divider0				= 7,

	// *** action control
	P3cActionType_FadeOut				= 8,
	P3cActionType_EnableAtTime			= 9,
	P3cActionType_DisableAtTime			= 10,
	P3cActionType_Die					= 11,
	P3cActionType_SetLifetime			= 12,

	// *** particle emission
	P3cActionType_EmitActivate			= 13,
	P3cActionType_EmitDeactivate		= 14,
	P3cActionType_EmitParticles			= 15,
	P3cActionType_ChangeClass			= 16,
	P3cActionType_KillLastEmitted		= 17,
	P3cActionType_ExplodeLastEmitted	= 18,
	P3cActionType_Divider2				= 19,

	// *** sound control
	P3cActionType_SoundStart			= 20,
	P3cActionType_SoundEnd				= 21,
	P3cActionType_SoundVolume			= 22,
	P3cActionType_ImpulseSound			= 23,
	// 1 blank space for future usage
	P3cActionType_Divider3				= 25,

	// *** damage
	P3cActionType_DamageHitChar			= 26,
	P3cActionType_DamageBlast			= 27,
	P3cActionType_Explode				= 28,
	P3cActionType_DamageEnvironment		= 29,
	P3cActionType_GlassCharge			= 30,
	P3cActionType_Stop					= 31,
	P3cActionType_Divider4				= 32,

	// *** rotation about an axis
	P3cActionType_RotateX				= 33,
	P3cActionType_RotateY				= 34,
	P3cActionType_RotateZ				= 35,
	P3cActionType_Divider5				= 36,

	// *** attractor functions
	P3cActionType_FindAttractor			= 37,
	P3cActionType_AttractGravity		= 38,
	P3cActionType_AttractHoming			= 39,
	P3cActionType_AttractSpring			= 40,
	// 5 blank spaces for future usage
	P3cActionType_Divider6				= 46,

	// *** motion control
	P3cActionType_MoveLine				= 47,
	P3cActionType_MoveGravity			= 48,
	P3cActionType_MoveSpiral			= 49,
	P3cActionType_MoveResistance		= 50,
	P3cActionType_MoveDrift				= 51,
	P3cActionType_SetVelocity			= 52,
	P3cActionType_SpiralTangent			= 53,
	P3cActionType_KillBeyondPoint		= 54,

	// *** interaction with environment
	P3cActionType_CollisionEffect		= 55,
	P3cActionType_StickToWall			= 56,
	P3cActionType_Bounce				= 57,
	P3cActionType_CreateDecal			= 58,
	P3cActionType_Chop					= 59,
	P3cActionType_ImpactEffect			= 60,

	P3cActionType_Divider8				= 61,

	// *** visibility control
	P3cActionType_Show					= 62,
	P3cActionType_Hide					= 63,
	P3cActionType_SetTextureTick		= 64,
	P3cActionType_RandomTextureFrame	= 65,
	// 3 blank spaces for future usage
	P3cActionType_Divider9				= 69,

	// *** action and variable control - triggers
	P3cActionType_SetVariable			= 70,
	P3cActionType_RecalculateAll		= 71,
	P3cActionType_EnableAbove			= 72,
	P3cActionType_EnableBelow			= 73,
	P3cActionType_EnableNow				= 74,
	P3cActionType_DisableNow			= 75,
	P3cActionType_Divider10				= 76,

	// *** special-case and hacks
	P3cActionType_SuperballTrigger		= 77,
	P3cActionType_StopIfBreakable		= 78,
	P3cActionType_AvoidWalls			= 79,
	P3cActionType_RandomSwirl			= 80,
	P3cActionType_FloatAbovePlayer		= 81,
	P3cActionType_StopIfSlow			= 82,
	P3cActionType_SuperParticle			= 83,

	// *** link control
	P3cActionType_StopLink				= 84,
	P3cActionType_CheckLink				= 85,
	P3cActionType_BreakLink				= 86,
	// 3 blank spaces for future usage

	P3cNumActionTypes					= 90,

	P3cActionType_Disabled				= (1 << 8),
	P3cActionType_Invalid				= ((UUtUns32)-1)

} P3tActionType;

// Information about the variables and parameters that an action accepts. A
// global P3tActionTemplate structure stores a number of these for each type of action.
typedef struct P3tActionVariable {
	P3tString					name;					// for display in editor
	P3tDataType					datatype;				// variable:  bitvector of allowed types
														// parameter: a single type
} P3tActionVariable;

#define P3cActionMaxParameters			8

typedef struct P3tParticleClass P3tParticleClass;

// An action's function pointer for the code to run
struct P3tActionInstance;
typedef UUtBool (*P3tActionProc)(struct P3tParticleClass *inClass, struct P3tParticle *inParticle,
								struct P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);

// Static information stored about each kind of action.
typedef enum P3tActionTemplateFlags {
	P3cActionTemplate_NotIfCollision	= (1 << 0),
	P3cActionTemplate_NotIfStopped		= (1 << 1),
	P3cActionTemplate_NotIfChopped		= (1 << 2)
} P3tActionTemplateFlags;


typedef struct P3tActionTemplate {
	P3tActionType				type;
	UUtUns32					flags;
	P3tString					name;					// for display in editor
	P3tActionProc				functionproc;			// the code to call
	UUtUns16					num_variables;
	UUtUns16					num_parameters;
	P3tActionVariable			info[P3cActionMaxParameters];	// stores variables, then parameters
} P3tActionTemplate;


// The data structure that stores the parameters for an instance of an action, as
// defined by the P3tParticleDefinition.
typedef struct P3tActionInstance_Version0 {
	P3tActionType			action_type;
	P3tVariableReference	action_var	[P3cActionMaxParameters];
	P3tValue				action_value[P3cActionMaxParameters];
} P3tActionInstance_Version0;

typedef struct P3tActionInstance {
	P3tActionType			action_type;
	UUtUns32				action_data;		// computed at load time. depends on action_type;
												// may be particle class, action pointer etc
	P3tVariableReference	action_var	[P3cActionMaxParameters];
	P3tValue				action_value[P3cActionMaxParameters];
} P3tActionInstance;



/*
 * particle appearance
 */

// this structure stores values that control how the particle is drawn
// (there are also some flags stored in definition->flags)
//
// only used if this is a sprite rather than a polygonal model: y_scale, rotation, x_offset, x_shorten, edgefade_*
//
// instance is a M3tTexture if a sprite, and a M3tGeometry if a polygonal model

// used in definition versions 0-3
typedef struct P3tAppearanceVersion_Initial {
	P3tValue				scale;		// float
	P3tValue				y_scale;	// float - only used if flag UseSeparateYScale is set
	P3tValue				rotation;	// float, degrees
	P3tValue				alpha;		// float, 0 to 1
	char					instance[32];
} P3tAppearanceVersion_Initial;

// used in definition version 4 - added x_offset, x_shorten
typedef struct P3tAppearanceVersion_XOffset {
	P3tValue				scale;		// float
	P3tValue				y_scale;	// float - only used if flag UseSeparateYScale is set
	P3tValue				rotation;	// float, degrees
	P3tValue				alpha;		// float, 0 to 1
	char					instance[32];
	P3tValue				x_offset;	// float, -1 to +1
	P3tValue				x_shorten;	// float, 0 to 1
} P3tAppearanceVersion_XOffset;

// used in definition versions 5-7 - added tint, edgefade_min, edgefade_max
typedef struct P3tAppearanceVersion_Tint {
	P3tValue				scale;			// float
	P3tValue				y_scale;		// float - only used if flag UseSeparateYScale is set
	P3tValue				rotation;		// float, degrees
	P3tValue				alpha;			// float, 0 to 1
	char					instance[32];
	P3tValue				x_offset;		// float, -1 to +1
	P3tValue				x_shorten;		// float, 0 to 1
	P3tValue				tint;			// float, 0 to 1
	P3tValue				edgefade_min;	// float, 0 to 1
	P3tValue				edgefade_max;	// float, 0 to 1
} P3tAppearanceVersion_Tint;

// used in definition versions 8-9 - added max_contrail
typedef struct P3tAppearanceVersion_MaxContrail {
	P3tValue				scale;			// float
	P3tValue				y_scale;		// float - only used if flag UseSeparateYScale is set
	P3tValue				rotation;		// float, degrees
	P3tValue				alpha;			// float, 0 to 1
	char					instance[32];
	P3tValue				x_offset;		// float, -1 to +1
	P3tValue				x_shorten;		// float, 0 to 1
	P3tValue				tint;			// float, 0 to 1
	P3tValue				edgefade_min;	// float, 0 to 1
	P3tValue				edgefade_max;	// float, 0 to 1
	P3tValue				max_contrail;	// float, max length of contrail
} P3tAppearanceVersion_MaxContrail;

// used in definition versions 10-11 - added lensflare_dist
typedef struct P3tAppearanceVersion_LensflareDist {
	P3tValue				scale;			// float
	P3tValue				y_scale;		// float - only used if flag UseSeparateYScale is set
	P3tValue				rotation;		// float, degrees
	P3tValue				alpha;			// float, 0 to 1
	char					instance[32];
	P3tValue				x_offset;		// float, -1 to +1
	P3tValue				x_shorten;		// float, 0 to 1
	P3tValue				tint;			// float, 0 to 1
	P3tValue				edgefade_min;	// float, 0 to 1
	P3tValue				edgefade_max;	// float, 0 to 1
	P3tValue				max_contrail;	// float, max length of contrail
	P3tValue				lensflare_dist;	// float, distance that lensflares can be seen through objects
	UUtUns16				lens_inframes;
	UUtUns16				lens_outframes;	// frames to fade in + fade out lensflares
} P3tAppearanceVersion_LensflareDist;

// used in definition versions 12+ - added decal parameters
typedef struct P3tAppearanceVersion_Decal {
	P3tValue				scale;			// float
	P3tValue				y_scale;		// float - only used if flag UseSeparateYScale is set
	P3tValue				rotation;		// float, degrees
	P3tValue				alpha;			// float, 0 to 1
	char					instance[32];
	P3tValue				x_offset;		// float, -1 to +1
	P3tValue				x_shorten;		// float, 0 to 1
	P3tValue				tint;			// float, 0 to 1
	P3tValue				edgefade_min;	// float, 0 to 1
	P3tValue				edgefade_max;	// float, 0 to 1
	P3tValue				max_contrail;	// float, max length of contrail
	P3tValue				lensflare_dist;	// float, distance that lensflares can be seen through objects
	UUtUns16				lens_inframes;
	UUtUns16				lens_outframes;	// frames to fade in + fade out lensflares
	UUtUns16				max_decals;		// max decals of this class
	UUtUns16				decal_fadeframes;
} P3tAppearanceVersion_Decal;

// used in definition versions 16+ - added decal parameters
typedef struct P3tAppearance {
	P3tValue				scale;			// float
	P3tValue				y_scale;		// float - only used if flag UseSeparateYScale is set
	P3tValue				rotation;		// float, degrees
	P3tValue				alpha;			// float, 0 to 1
	char					instance[32];
	P3tValue				x_offset;		// float, -1 to +1
	P3tValue				x_shorten;		// float, 0 to 1
	P3tValue				tint;			// float, 0 to 1
	P3tValue				edgefade_min;	// float, 0 to 1
	P3tValue				edgefade_max;	// float, 0 to 1
	P3tValue				max_contrail;	// float, max length of contrail
	P3tValue				lensflare_dist;	// float, distance that lensflares can be seen through objects
	UUtUns16				lens_inframes;
	UUtUns16				lens_outframes;	// frames to fade in + fade out lensflares
	UUtUns16				max_decals;		// max decals of this class
	UUtUns16				decal_fadeframes;
	P3tValue				decal_wrap;		// float, angle to wrap decals around edges
} P3tAppearance;

#define P3tAppearanceVersion_DecalWrap	P3tAppearance



/*
 * emitter parameters
 */

#define P3cParticleClassNameLength		63

typedef enum P3tEmitterFlags {
	P3cEmitterFlag_InitiallyActive					= (1 << 0),
	P3cEmitterFlag_IncreaseEmitCount				= (1 << 1),
	P3cEmitterFlag_DeactivateWhenEmitCountThreshold	= (1 << 2),
	P3cEmitterFlag_ActivateWhenEmitCountThreshold	= (1 << 3),
	P3cEmitterFlag_AddParentVelocity				= (1 << 4),
	P3cEmitterFlag_SameDynamicMatrix				= (1 << 5),
	P3cEmitterFlag_OrientToVelocity					= (1 << 6),
	P3cEmitterFlag_InheritTint						= (1 << 7),
	P3cEmitterFlag_OnePerAttractor					= (1 << 8),
	P3cEmitterFlag_AtLeastOne						= (1 << 9),
	P3cEmitterFlag_RotateAttractors					= (1 << 10)

} P3tEmitterFlags;

typedef enum P3tEmitterRateType {
	P3cEmitterRate_Continuous				= 0,
	P3cEmitterRate_Random					= 1,
	P3cEmitterRate_Instant					= 2,
	P3cEmitterRate_Distance					= 3,
	P3cEmitterRate_Attractor				= 4
} P3tEmitterRateType;

typedef enum P3tEmitterPositionType {
	P3cEmitterPosition_Point				= 0,
	P3cEmitterPosition_Line					= 1,
	P3cEmitterPosition_Circle				= 2,
	P3cEmitterPosition_Sphere				= 3,
	P3cEmitterPosition_Offset				= 4,
	P3cEmitterPosition_Cylinder				= 5,
	P3cEmitterPosition_BodySurface			= 6,
	P3cEmitterPosition_BodyBones			= 7
} P3tEmitterPositionType;

typedef enum P3tEmitterDirectionType {
	P3cEmitterDirection_Linear				= 0,
	P3cEmitterDirection_Random				= 1,
	P3cEmitterDirection_Conical				= 2,
	P3cEmitterDirection_Ring				= 3,
	P3cEmitterDirection_OffsetDir			= 4,
	P3cEmitterDirection_Inaccurate			= 5,
	P3cEmitterDirection_Attractor			= 6
} P3tEmitterDirectionType;

typedef enum P3tEmitterSpeedType {
	P3cEmitterSpeed_Uniform					= 0,
	P3cEmitterSpeed_Stratified				= 1
} P3tEmitterSpeedType;

typedef enum P3tEmitterOrientationType {
	P3cEmitterOrientation_ParentPosX		= 0,
	P3cEmitterOrientation_ParentNegX		= 1,
	P3cEmitterOrientation_ParentPosY		= 2,
	P3cEmitterOrientation_ParentNegY		= 3,
	P3cEmitterOrientation_ParentPosZ		= 4,
	P3cEmitterOrientation_ParentNegZ		= 5,
	P3cEmitterOrientation_WorldPosX			= 6,
	P3cEmitterOrientation_WorldNegX			= 7,
	P3cEmitterOrientation_WorldPosY			= 8,
	P3cEmitterOrientation_WorldNegY			= 9,
	P3cEmitterOrientation_WorldPosZ			= 10,
	P3cEmitterOrientation_WorldNegZ			= 11,
	P3cEmitterOrientation_Velocity			= 12,
	P3cEmitterOrientation_ReverseVelocity	= 13,
	P3cEmitterOrientation_TowardsEmitter	= 14,
	P3cEmitterOrientation_AwayFromEmitter	= 15
} P3tEmitterOrientationType;

// constants for packing in the emitter_value array
#define P3cEmitterValue_NumRateVals				2
#define P3cEmitterValue_NumPositionVals			3
#define P3cEmitterValue_NumDirectionVals		3
#define P3cEmitterValue_NumSpeedVals			3
#define P3cEmitterValue_NumOrientationVals		0
#define P3cEmitterValue_NumPadVals				1

#define P3cEmitterValue_RateValOffset			0
#define P3cEmitterValue_PositionValOffset		(P3cEmitterValue_RateValOffset		+ P3cEmitterValue_NumRateVals)
#define P3cEmitterValue_DirectionValOffset		(P3cEmitterValue_PositionValOffset	+ P3cEmitterValue_NumPositionVals)
#define P3cEmitterValue_SpeedValOffset			(P3cEmitterValue_DirectionValOffset	+ P3cEmitterValue_NumDirectionVals)
#define P3cEmitterValue_OrientationValOffset	(P3cEmitterValue_SpeedValOffset		+ P3cEmitterValue_NumOrientationVals)

#define P3cEmitterValue_NumVals					(P3cEmitterValue_NumRateVals + P3cEmitterValue_NumPositionVals + \
												 P3cEmitterValue_NumDirectionVals + P3cEmitterValue_NumSpeedVals + \
												 P3cEmitterValue_NumOrientationVals + P3cEmitterValue_NumPadVals)

#define P3cMaxEmitters							8

typedef enum P3tEmitterLinkType {
	P3cEmitterLinkType_None				= 0,
	P3cEmitterLinkType_Emitter			= 1,
	P3cEmitterLinkType_LastEmission		= 2,	// P3cMaxEmitters values here, each for one emitter
	P3cEmitterLinkType_OurLink			= 10,	// in the future we may end up storing more than one link in each particle,
											// so leave some room here
	P3cEmitterLinkType_Max				= 14
} P3tEmitterLinkType;

// emitter for versions 0-6
typedef struct P3tEmitter_Version0 {
	char						classname[P3cParticleClassNameLength + 1];
	P3tParticleClass			*emittedclass;

	UUtUns32					flags;
	UUtUns16					particle_limit;
	UUtUns16					burst_size;				// amount to emit is multiplied by this

	P3tEmitterRateType			rate_type;
	P3tEmitterPositionType		position_type;
	P3tEmitterDirectionType		direction_type;
	P3tEmitterSpeedType			speed_type;

	P3tValue					emitter_value[P3cEmitterValue_NumVals];

} P3tEmitter_Version0;

// emitter for versions 7+ - added link_type
typedef struct P3tEmitter_Version7 {
	char						classname[P3cParticleClassNameLength + 1];
	P3tParticleClass			*emittedclass;

	UUtUns32					flags;
	UUtUns16					particle_limit;
	UUtUns16					burst_size;				// amount to emit is multiplied by this
	P3tEmitterLinkType			link_type;

	P3tEmitterRateType			rate_type;
	P3tEmitterPositionType		position_type;
	P3tEmitterDirectionType		direction_type;
	P3tEmitterSpeedType			speed_type;

	P3tValue					emitter_value[P3cEmitterValue_NumVals];

} P3tEmitter_Version7;

// emitter for versions 9+ - changed burst_size, added emit_chance, orientdir_type and orientup_type
typedef struct P3tEmitter {
	char						classname[P3cParticleClassNameLength + 1];
	P3tParticleClass			*emittedclass;

	UUtUns32					flags;
	UUtUns16					particle_limit;
	UUtUns16					emit_chance;			// 0 - UUcMaxUns16
	float						burst_size;				// fractional values indicate random variation in burst
	P3tEmitterLinkType			link_type;

	P3tEmitterRateType			rate_type;
	P3tEmitterPositionType		position_type;
	P3tEmitterDirectionType		direction_type;
	P3tEmitterSpeedType			speed_type;
	P3tEmitterOrientationType	orientdir_type;
	P3tEmitterOrientationType	orientup_type;

	P3tValue					emitter_value[P3cEmitterValue_NumVals];

} P3tEmitter;

// this structure is used to store the kinds of settings possible for
// an emitter
#define P3cEmitterMaxParameters	4
typedef struct P3tEmitterParamDescription {
	P3tString						param_name;
	P3tDataType						data_type;
} P3tEmitterParamDescription;

typedef struct P3tEmitterSettingDescription {
	UUtUns32						setting_enum;
	P3tString						setting_name;
	UUtUns32						num_params;
	P3tEmitterParamDescription		param_desc[P3cEmitterMaxParameters];
} P3tEmitterSettingDescription;

/*
 * particle class
 */

// weapon / turret system assigns each particle a P3tOwner when they fire... this is
// used for damage ownership tracking
typedef UUtUns32 P3tOwner;

// previous space/time position point
typedef struct P3tPrevPosition {
	float		time;
	M3tPoint3D	point;
} P3tPrevPosition;

typedef enum P3tParticleOptionalVar {
	P3cParticleOptionalVar_Velocity			= 0,
	P3cParticleOptionalVar_Orientation		= 1,
	P3cParticleOptionalVar_OffsetPos		= 2,
	P3cParticleOptionalVar_DynamicMatrix	= 3,
	P3cParticleOptionalVar_PrevPosition		= 4,
	P3cParticleOptionalVar_DecalData		= 5,
	P3cParticleOptionalVar_TextureIndex		= 6,
	P3cParticleOptionalVar_TextureTime		= 7,
	P3cParticleOptionalVar_Owner			= 8,
	P3cParticleOptionalVar_ContrailData		= 9,
	P3cParticleOptionalVar_LensFrames		= 10,
	P3cParticleOptionalVar_Attractor		= 11,
	P3cParticleOptionalVar_EnvironmentCache	= 12,
	P3cNumParticleOptionalVars				= 13
} P3tParticleOptionalVar;

extern size_t P3gOptionalVarSize[P3cNumParticleOptionalVars];

// flags is a 32-bit vector
#define P3cParticleClassFlags_SpriteTypeShift	5
#define P3cParticleClassFlags_OptionShift		12

#define P3cParticleClassFlag_Appearance_SpriteTypeMask (7 << P3cParticleClassFlags_SpriteTypeShift)

enum P3tParticleClassFlags {
	P3cParticleClassFlag_Decorative							= (1 << 0),
	P3cParticleClassFlag_VisibilityAlways					= (0 << 1),
	P3cParticleClassFlag_VisibilitySlave					= (1 << 1),
	P3cParticleClassFlag_VisibilityMaster					= (2 << 1),
	P3cParticleClassFlag_VisibilityMask						= (3 << 1),

	P3cParticleClassFlag_Appearance_UseSeparateYScale		= (1 << 3),
	P3cParticleClassFlag_Appearance_NeedsTextureInstance	= (1 << 4),

	P3cParticleClassFlag_Appearance_FaceCameraScreenXY		= (0 << P3cParticleClassFlags_SpriteTypeShift),
	P3cParticleClassFlag_Appearance_FaceCameraRotatedDir	= (1 << P3cParticleClassFlags_SpriteTypeShift),
	P3cParticleClassFlag_Appearance_ParallelDirFaceCamera	= (2 << P3cParticleClassFlags_SpriteTypeShift),
	P3cParticleClassFlag_Appearance_ParallelDirUpVector		= (3 << P3cParticleClassFlags_SpriteTypeShift),
	P3cParticleClassFlag_Appearance_PerpendicularDir		= (4 << P3cParticleClassFlags_SpriteTypeShift),
	P3cParticleClassFlag_Appearance_Contrail				= (5 << P3cParticleClassFlags_SpriteTypeShift),
	P3cParticleClassFlag_Appearance_ContrailFaceCamera		= (6 << P3cParticleClassFlags_SpriteTypeShift),
	P3cParticleClassFlag_Appearance_ParallelDirRightVector	= (7 << P3cParticleClassFlags_SpriteTypeShift),	// bits 5-7

	P3cParticleClassFlag_Appearance_Geometry				= (1 << 8),
	
	P3cParticleClassFlag_Physics_CollideEnv					= (1 << 9),
	P3cParticleClassFlag_Physics_CollideChar				= (1 << 10),

	P3cParticleClassFlag_Appearance_ScaleToVelocity			= (1 << 11),

	// optional variables
	P3cParticleClassFlag_HasVelocity		= (1 << (P3cParticleClassFlags_OptionShift + P3cParticleOptionalVar_Velocity)),
	P3cParticleClassFlag_HasOrientation		= (1 << (P3cParticleClassFlags_OptionShift + P3cParticleOptionalVar_Orientation)),
	P3cParticleClassFlag_HasOffsetPos		= (1 << (P3cParticleClassFlags_OptionShift + P3cParticleOptionalVar_OffsetPos)),
	P3cParticleClassFlag_HasDynamicMatrix	= (1 << (P3cParticleClassFlags_OptionShift + P3cParticleOptionalVar_DynamicMatrix)),
	P3cParticleClassFlag_HasPrevPosition	= (1 << (P3cParticleClassFlags_OptionShift + P3cParticleOptionalVar_PrevPosition)),
	P3cParticleClassFlag_HasDecalData		= (1 << (P3cParticleClassFlags_OptionShift + P3cParticleOptionalVar_DecalData)),
	P3cParticleClassFlag_HasTextureIndex	= (1 << (P3cParticleClassFlags_OptionShift + P3cParticleOptionalVar_TextureIndex)),
	P3cParticleClassFlag_HasTextureTime		= (1 << (P3cParticleClassFlags_OptionShift + P3cParticleOptionalVar_TextureTime)),
	P3cParticleClassFlag_HasOwner			= (1 << (P3cParticleClassFlags_OptionShift + P3cParticleOptionalVar_Owner)),
	P3cParticleClassFlag_HasContrailData	= (1 << (P3cParticleClassFlags_OptionShift + P3cParticleOptionalVar_ContrailData)),
	P3cParticleClassFlag_HasLensFrames		= (1 << (P3cParticleClassFlags_OptionShift + P3cParticleOptionalVar_LensFrames)),
	P3cParticleClassFlag_HasAttractor		= (1 << (P3cParticleClassFlags_OptionShift + P3cParticleOptionalVar_Attractor)),
	P3cParticleClassFlag_HasEnvironmentCache= (1 << (P3cParticleClassFlags_OptionShift + P3cParticleOptionalVar_EnvironmentCache))
};

enum P3tParticleClassFlags2 {
	P3cParticleClassFlag2_UseGlobalTint						= (1 << 0),

	P3cParticleClassFlag2_Attractor_CheckEnvironment		= (1 << 1),
	P3cParticleClassFlag2_Attractor_AvoidFriendly			= (1 << 2),

	P3cParticleClassFlag2_ExpireOnCutscene					= (1 << 3),
	P3cParticleClassFlag2_DieOnCutscene						= (1 << 4),

	P3cParticleClassFlag2_LowDetailDisable					= (1 << 5),
	P3cParticleClassFlag2_MediumDetailDisable				= (1 << 6),

	// these flags moved across wholesale from the first flags variable
	P3cParticleClassFlag2_Appearance_Sky					= (1 << 20),
	P3cParticleClassFlag2_Appearance_DecalFullBright		= (1 << 21),
	P3cParticleClassFlag2_Appearance_DecalCullBackfacing	= (1 << 22),
	P3cParticleClassFlag2_Appearance_Decal					= (1 << 23),
	P3cParticleClassFlag2_Appearance_InitiallyHidden		= (1 << 24),
	P3cParticleClassFlag2_Appearance_Invisible				= (1 << 25),
	P3cParticleClassFlag2_Appearance_EdgeFade				= (1 << 26),
	P3cParticleClassFlag2_Appearance_Vector					= (1 << 27),
	P3cParticleClassFlag2_LockToLinkedParticle				= (1 << 28),
	P3cParticleClassFlag2_ContrailEmitter					= (1 << 29),
	P3cParticleClassFlag2_Appearance_LensFlare				= (1 << 30),
	P3cParticleClassFlag2_Appearance_EdgeFade_OneSided		= (1 << 31)
};

// the maximum number of events that a particle can handle
#define P3cMaxNumEvents_Version0	12
#define P3cMaxNumEvents				16

enum P3tEventIndex {
	P3cEvent_Update				= 0,
	P3cEvent_Pulse				= 1,
	P3cEvent_Start				= 2,
	P3cEvent_Stop				= 3,
	P3cEvent_BackgroundStart	= 4,
	P3cEvent_BackgroundStop		= 5,
	P3cEvent_CollideEnv			= 6,
	P3cEvent_CollideChar		= 7,
	P3cEvent_Lifetime			= 8,
	P3cEvent_Explode			= 9,
	P3cEvent_BrokenLink			= 10,
	P3cEvent_Create				= 11,
	P3cEvent_Die				= 12,
	P3cEvent_NewAttractor		= 13,
	P3cEvent_DelayStart			= 14,
	P3cEvent_DelayStop			= 15,
	P3cEvent_NumEvents			= 16
};

extern const P3tString P3gEventName[P3cMaxNumEvents];


/*
 * bitflags for lensflare fading
 */

#define P3cLensFrames_TypeOffset	16
#define P3cLensFrames_TypeBits		16
#define P3cLensFrames_TypeMask		(((1 << P3cLensFrames_TypeBits) - 1) << P3cLensFrames_TypeOffset)
#define P3cLensFrames_FrameBits		16
#define P3cLensFrames_FrameMask		((1 << P3cLensFrames_FrameBits) - 1)

enum {
	P3cLensFrames_Unset			= (0 << P3cLensFrames_TypeOffset),
	P3cLensFrames_Visible		= (1 << P3cLensFrames_TypeOffset),
	P3cLensFrames_Invisible		= (2 << P3cLensFrames_TypeOffset),
	P3cLensFrames_FadeIn		= (3 << P3cLensFrames_TypeOffset),
	P3cLensFrames_FadeOut		= (4 << P3cLensFrames_TypeOffset)
};

/*
 * particle attractors
 */

// iterator module determines what are potential targets
enum {
	P3cAttractorIterator_None				= 0,
	P3cAttractorIterator_Link				= 1,
	P3cAttractorIterator_ParticleClass		= 2,
	P3cAttractorIterator_ParticleTag		= 3,
	P3cAttractorIterator_Character			= 4,
	P3cAttractorIterator_HostileCharacter	= 5,
	P3cAttractorIterator_EmittedTowards		= 6,
	P3cAttractorIterator_Parent				= 7,
	P3cAttractorIterator_AllCharacters		= 8,

	P3cAttractorIterator_Max				= 9
};

// selector module determines which target to pick
enum {
	P3cAttractorSelector_Distance		= 0,
	P3cAttractorSelector_Conical		= 1,

	P3cAttractorSelector_Max			= 2
};

// data stored in a class definition for attractor functions
typedef struct P3tAttractor_Version14 {
	UUtUns32			iterator_type;
	UUtUns32			selector_type;

	char				attractor_name[P3cParticleClassNameLength + 1];
	P3tValue			max_distance;
	P3tValue			max_angle;
} P3tAttractor_Version14;

typedef struct P3tAttractor_Version15 {
	UUtUns32			iterator_type;
	UUtUns32			selector_type;

	char				attractor_name[P3cParticleClassNameLength + 1];
	P3tValue			max_distance;
	P3tValue			max_angle;
	P3tValue			select_thetamin;
	P3tValue			select_thetamax;
	P3tValue			select_thetaweight;
} P3tAttractor_Version15;

typedef struct P3tAttractor {
	UUtUns32			iterator_type;
	UUtUns32			selector_type;

	void *				attractor_ptr;		// either P3tParticleClass or EPtEnvParticle, for class or tag
	char				attractor_name[P3cParticleClassNameLength + 1];
	P3tValue			max_distance;
	P3tValue			max_angle;
	P3tValue			select_thetamin;
	P3tValue			select_thetamax;
	P3tValue			select_thetaweight;
} P3tAttractor;

#define P3cAttractor_ParticleReference		P3cParticleReference_Valid
#define P3cAttractor_CharacterIndex			(1 << 30)

// data stored in a particle that is being attracted
typedef struct P3tAttractorData {
	UUtUns32				index;
	UUtUns32				delay_tick;
} P3tAttractorData;

// functions that specify the behavior of iterator and selector modules
typedef UUtBool (*P3tAttractorIteratorFunction)(struct P3tParticleClass *inClass, struct P3tParticle *inParticle, P3tAttractor *inAttractor,
												M3tPoint3D *outFoundPoint, float *ioDistanceSquared, UUtUns32 *ioIterator);
typedef UUtBool (*P3tAttractorSelectorFunction)(struct P3tParticleClass *inClass, struct P3tParticle *inParticle, P3tAttractor *inAttractor,
												M3tPoint3D *inFoundPoint, float inDistanceSquared, UUtUns32 inFoundID, UUtUns32 *ioChoice);

typedef struct P3tAttractorIteratorSpec {
	const char *					name;
	P3tAttractorIteratorFunction	function;
} P3tAttractorIteratorSpec;

typedef struct P3tAttractorSelectorSpec {
	const char *					name;
	P3tAttractorSelectorFunction	function;
} P3tAttractorSelectorSpec;

extern P3tAttractorIteratorSpec P3gAttractorIteratorTable[P3cAttractorIterator_Max];
extern P3tAttractorSelectorSpec P3gAttractorSelectorTable[P3cAttractorSelector_Max];

/*
 * versioning for particle definitions
 */

// P3tParticleDefinition is loaded as binary data, not parsed from an _ins
// file. The binary data consists of:
// 1				P3tParticleDefinition
// num_variables	P3tVariableInfo
// num_actions		P3tActionInstance
// num_emitters		P3tEmitter
// 
// a P3tParticleDefinition defines the characteristics and behaviour of a
// single class of particles. at load time, a P3tParticleClass is created
// that stores this definition and all of the runtime structures as required.

// version 0
typedef struct P3tParticleVersion_Initial {
	// total size of the particle definition
	UUtUns16			definition_size;

	// version of the definition
	UUtUns16			version;

	// particle flags - visibility, optional variables
	UUtUns32			flags;

	// information about the number of structures in this particle class
	UUtUns16			num_variables;
	UUtUns16			num_actions;
	UUtUns16			num_emitters;

	// particle classes that are 'decorative' are not necessary for gameplay
	// and they reduce the number created so that they never exceed max_particles
	UUtUns16			max_particles;

	// the set of actions to be done every frame
	P3tActionList		actionlist_everyframe;

	// particle lifetime
	P3tValue			lifetime;

	// how to draw the particle
	P3tAppearanceVersion_Initial appearance;

	// pointers to the variable-length arrays in this particle class
	// (initialised on structure loading)
	P3tVariableInfo *	variable;
	P3tActionInstance_Version0 *action;
	P3tEmitter_Version0 *emitter;

} P3tParticleVersion_Initial;

// version 1 - changed actionlist_everyframe to an array of eventlists
typedef struct P3tParticleVersion_EventList {
	// total size of the particle definition
	UUtUns16			definition_size;

	// version of the definition
	UUtUns16			version;

	// particle flags - visibility, optional variables
	UUtUns32			flags;

	// information about the number of structures in this particle class
	UUtUns16			num_variables;
	UUtUns16			num_actions;
	UUtUns16			num_emitters;

	// particle classes that are 'decorative' are not necessary for gameplay
	// and they reduce the number created so that they never exceed max_particles
	UUtUns16			max_particles;

	// the set of actions to be done when an event happens (update, explode, etc)
	P3tActionList		eventlist[P3cMaxNumEvents_Version0];

	// particle lifetime
	P3tValue			lifetime;

	// how to draw the particle
	P3tAppearanceVersion_Initial appearance;

	// pointers to the variable-length arrays in this particle class
	// (initialised on structure loading)
	P3tVariableInfo *	variable;
	P3tActionInstance_Version0 *	action;
	P3tEmitter_Version0 *emitter;

} P3tParticleVersion_EventList;

// version 2 - added collision_radius
typedef struct P3tParticleVersion_Collision {
	// total size of the particle definition
	UUtUns16			definition_size;

	// version of the definition
	UUtUns16			version;

	// particle flags - visibility, optional variables
	UUtUns32			flags;

	// information about the number of structures in this particle class
	UUtUns16			num_variables;
	UUtUns16			num_actions;
	UUtUns16			num_emitters;

	// particle classes that are 'decorative' are not necessary for gameplay
	// and they reduce the number created so that they never exceed max_particles
	UUtUns16			max_particles;

	// the set of actions to be done when an event happens (update, explode, etc)
	P3tActionList		eventlist[P3cMaxNumEvents_Version0];

	// particle lifetime
	P3tValue			lifetime;

	// collision radius
	P3tValue			collision_radius;

	// how to draw the particle
	P3tAppearanceVersion_Initial appearance;

	// pointers to the variable-length arrays in this particle class
	// (initialised on structure loading)
	P3tVariableInfo *	variable;
	P3tActionInstance_Version0 *	action;
	P3tEmitter_Version0 *emitter;

} P3tParticleVersion_Collision;

// version 3 - actioninstances have a user-data ptr
typedef struct P3tParticleVersion_ActionPtr {
	// total size of the particle definition
	UUtUns16			definition_size;

	// version of the definition
	UUtUns16			version;

	// particle flags - visibility, optional variables
	UUtUns32			flags;

	// information about the number of structures in this particle class
	UUtUns16			num_variables;
	UUtUns16			num_actions;
	UUtUns16			num_emitters;

	// particle classes that are 'decorative' are not necessary for gameplay
	// and they reduce the number created so that they never exceed max_particles
	UUtUns16			max_particles;

	// the set of actions to be done when an event happens (update, explode, etc)
	P3tActionList		eventlist[P3cMaxNumEvents_Version0];

	// particle lifetime
	P3tValue			lifetime;

	// collision radius
	P3tValue			collision_radius;

	// how to draw the particle
	P3tAppearanceVersion_Initial appearance;

	// pointers to the variable-length arrays in this particle class
	// (initialised on structure loading)
	P3tVariableInfo *	variable;
	P3tActionInstance *	action;
	P3tEmitter_Version0 *emitter;

} P3tParticleVersion_ActionPtr;

// version 4 - added x_offset and x_shorten to the appearance
typedef struct P3tParticleVersion_XOffset {
	// total size of the particle definition
	UUtUns16			definition_size;

	// version of the definition
	UUtUns16			version;

	// particle flags - visibility, optional variables
	UUtUns32			flags;

	// information about the number of structures in this particle class
	UUtUns16			num_variables;
	UUtUns16			num_actions;
	UUtUns16			num_emitters;

	// particle classes that are 'decorative' are not necessary for gameplay
	// and they reduce the number created so that they never exceed max_particles
	UUtUns16			max_particles;

	// the set of actions to be done when an event happens (update, explode, etc)
	P3tActionList		eventlist[P3cMaxNumEvents_Version0];

	// particle lifetime
	P3tValue			lifetime;

	// collision radius
	P3tValue			collision_radius;

	// how to draw the particle
	P3tAppearanceVersion_XOffset	appearance;

	// pointers to the variable-length arrays in this particle class
	// (initialised on structure loading)
	P3tVariableInfo *	variable;
	P3tActionInstance *	action;
	P3tEmitter_Version0 *emitter;

} P3tParticleVersion_XOffset;

// version 5 - added tint, edgefade_min and edgefade_max to the appearance
typedef struct P3tParticleVersion_Tint {
	// total size of the particle definition
	UUtUns16			definition_size;

	// version of the definition
	UUtUns16			version;

	// particle flags - visibility, optional variables
	UUtUns32			flags;

	// information about the number of structures in this particle class
	UUtUns16			num_variables;
	UUtUns16			num_actions;
	UUtUns16			num_emitters;

	// particle classes that are 'decorative' are not necessary for gameplay
	// and they reduce the number created so that they never exceed max_particles
	UUtUns16			max_particles;

	// the set of actions to be done when an event happens (update, explode, etc)
	P3tActionList		eventlist[P3cMaxNumEvents_Version0];

	// particle lifetime
	P3tValue			lifetime;

	// collision radius
	P3tValue			collision_radius;

	// how to draw the particle
	P3tAppearanceVersion_Tint		appearance;

	// pointers to the variable-length arrays in this particle class
	// (initialised on structure loading)
	P3tVariableInfo *	variable;
	P3tActionInstance *	action;
	P3tEmitter_Version0 *emitter;

} P3tParticleVersion_Tint;

// version 6 - added initial_actionmask
typedef struct P3tParticleVersion_ActionMask {
	// total size of the particle definition
	UUtUns16			definition_size;

	// version of the definition
	UUtUns16			version;

	// particle flags - visibility, optional variables
	UUtUns32			flags;

	// actionmask for newly created particles
	UUtUns32			actionmask;

	// information about the number of structures in this particle class
	UUtUns16			num_variables;
	UUtUns16			num_actions;
	UUtUns16			num_emitters;

	// particle classes that are 'decorative' are not necessary for gameplay
	// and they reduce the number created so that they never exceed max_particles
	UUtUns16			max_particles;

	// the set of actions to be done when an event happens (update, explode, etc)
	P3tActionList		eventlist[P3cMaxNumEvents_Version0];

	// particle lifetime
	P3tValue			lifetime;

	// collision radius
	P3tValue			collision_radius;

	// how to draw the particle
	P3tAppearanceVersion_Tint		appearance;

	// pointers to the variable-length arrays in this particle class
	// (initialised on structure loading)
	P3tVariableInfo *	variable;
	P3tActionInstance *	action;
	P3tEmitter_Version0 *emitter;

} P3tParticleVersion_ActionMask;

// version 7 - added link_type to emitters
typedef struct P3tParticleVersion_EmitterLink {
	// total size of the particle definition
	UUtUns16			definition_size;

	// version of the definition
	UUtUns16			version;

	// particle flags - visibility, optional variables
	UUtUns32			flags;

	// actionmask for newly created particles
	UUtUns32			actionmask;

	// information about the number of structures in this particle class
	UUtUns16			num_variables;
	UUtUns16			num_actions;
	UUtUns16			num_emitters;

	// particle classes that are 'decorative' are not necessary for gameplay
	// and they reduce the number created so that they never exceed max_particles
	UUtUns16			max_particles;

	// the set of actions to be done when an event happens (update, explode, etc)
	P3tActionList		eventlist[P3cMaxNumEvents_Version0];

	// particle lifetime
	P3tValue			lifetime;

	// collision radius
	P3tValue			collision_radius;

	// how to draw the particle
	P3tAppearanceVersion_Tint		appearance;

	// pointers to the variable-length arrays in this particle class
	// (initialised on structure loading)
	P3tVariableInfo *	variable;
	P3tActionInstance *	action;
	P3tEmitter_Version7 *emitter;

} P3tParticleVersion_EmitterLink;

// version 8 - added max_contrail to appearance
typedef struct P3tParticleVersion_MaxContrail {
	// total size of the particle definition
	UUtUns16			definition_size;

	// version of the definition
	UUtUns16			version;

	// particle flags - visibility, optional variables
	UUtUns32			flags;

	// actionmask for newly created particles
	UUtUns32			actionmask;

	// information about the number of structures in this particle class
	UUtUns16			num_variables;
	UUtUns16			num_actions;
	UUtUns16			num_emitters;

	// particle classes that are 'decorative' are not necessary for gameplay
	// and they reduce the number created so that they never exceed max_particles
	UUtUns16			max_particles;

	// the set of actions to be done when an event happens (update, explode, etc)
	P3tActionList		eventlist[P3cMaxNumEvents_Version0];

	// particle lifetime
	P3tValue			lifetime;

	// collision radius
	P3tValue			collision_radius;

	// how to draw the particle
	P3tAppearanceVersion_MaxContrail appearance;

	// pointers to the variable-length arrays in this particle class
	// (initialised on structure loading)
	P3tVariableInfo *	variable;
	P3tActionInstance *	action;
	P3tEmitter_Version7 *emitter;

} P3tParticleVersion_MaxContrail;

// version 9 - added some fields to P3tEmitter
typedef struct P3tParticleVersion_EmitterOrient {
	// total size of the particle definition
	UUtUns16			definition_size;

	// version of the definition
	UUtUns16			version;

	// particle flags - visibility, optional variables
	UUtUns32			flags;

	// actionmask for newly created particles
	UUtUns32			actionmask;

	// information about the number of structures in this particle class
	UUtUns16			num_variables;
	UUtUns16			num_actions;
	UUtUns16			num_emitters;

	// particle classes that are 'decorative' are not necessary for gameplay
	// and they reduce the number created so that they never exceed max_particles
	UUtUns16			max_particles;

	// the set of actions to be done when an event happens (update, explode, etc)
	P3tActionList		eventlist[P3cMaxNumEvents_Version0];

	// particle lifetime
	P3tValue			lifetime;

	// collision radius
	P3tValue			collision_radius;

	// how to draw the particle
	P3tAppearanceVersion_MaxContrail appearance;

	// pointers to the variable-length arrays in this particle class
	// (initialised on structure loading)
	P3tVariableInfo *	variable;
	P3tActionInstance *	action;
	P3tEmitter *		emitter;

} P3tParticleVersion_EmitterOrient;

// version 10 - added lensflare_dist to P3tAppearance
typedef struct P3tParticleVersion_LensflareDist {
	// total size of the particle definition
	UUtUns16			definition_size;

	// version of the definition
	UUtUns16			version;

	// particle flags - visibility, optional variables
	UUtUns32			flags;

	// actionmask for newly created particles
	UUtUns32			actionmask;

	// information about the number of structures in this particle class
	UUtUns16			num_variables;
	UUtUns16			num_actions;
	UUtUns16			num_emitters;

	// particle classes that are 'decorative' are not necessary for gameplay
	// and they reduce the number created so that they never exceed max_particles
	UUtUns16			max_particles;

	// the set of actions to be done when an event happens (update, explode, etc)
	P3tActionList		eventlist[P3cMaxNumEvents_Version0];

	// particle lifetime
	P3tValue			lifetime;

	// collision radius
	P3tValue			collision_radius;

	// how to draw the particle
	P3tAppearanceVersion_LensflareDist		appearance;

	// pointers to the variable-length arrays in this particle class
	// (initialised on structure loading)
	P3tVariableInfo *	variable;
	P3tActionInstance *	action;
	P3tEmitter *		emitter;

} P3tParticleVersion_LensflareDist;

// version 11 - added flags2
typedef struct P3tParticleVersion_Flags2 {
	// total size of the particle definition
	UUtUns16			definition_size;

	// version of the definition
	UUtUns16			version;

	// particle flags - lots of settings
	UUtUns32			flags;
	UUtUns32			flags2;

	// actionmask for newly created particles
	UUtUns32			actionmask;

	// information about the number of structures in this particle class
	UUtUns16			num_variables;
	UUtUns16			num_actions;
	UUtUns16			num_emitters;

	// particle classes that are 'decorative' are not necessary for gameplay
	// and they reduce the number created so that they never exceed max_particles
	UUtUns16			max_particles;

	// the set of actions to be done when an event happens (update, explode, etc)
	P3tActionList		eventlist[P3cMaxNumEvents_Version0];

	// particle lifetime
	P3tValue			lifetime;

	// collision radius
	P3tValue			collision_radius;

	// how to draw the particle
	P3tAppearanceVersion_LensflareDist		appearance;

	// pointers to the variable-length arrays in this particle class
	// (initialised on structure loading)
	P3tVariableInfo *	variable;
	P3tActionInstance *	action;
	P3tEmitter *		emitter;

} P3tParticleVersion_Flags2;

// version 12 - added some decal fields to appearance
typedef struct P3tParticleVersion_Decal {
	// total size of the particle definition
	UUtUns16			definition_size;

	// version of the definition
	UUtUns16			version;

	// particle flags - lots of settings
	UUtUns32			flags;
	UUtUns32			flags2;

	// actionmask for newly created particles
	UUtUns32			actionmask;

	// information about the number of structures in this particle class
	UUtUns16			num_variables;
	UUtUns16			num_actions;
	UUtUns16			num_emitters;

	// particle classes that are 'decorative' are not necessary for gameplay
	// and they reduce the number created so that they never exceed max_particles
	UUtUns16			max_particles;

	// the set of actions to be done when an event happens (update, explode, etc)
	P3tActionList		eventlist[P3cMaxNumEvents_Version0];

	// particle lifetime
	P3tValue			lifetime;

	// collision radius
	P3tValue			collision_radius;

	// how to draw the particle
	P3tAppearanceVersion_Decal appearance;

	// pointers to the variable-length arrays in this particle class
	// (initialised on structure loading)
	P3tVariableInfo *	variable;
	P3tActionInstance *	action;
	P3tEmitter *		emitter;

} P3tParticleVersion_Decal;

// version 13 - added more event types
typedef struct P3tParticleVersion_MoreEvents {
	// total size of the particle definition
	UUtUns16			definition_size;

	// version of the definition
	UUtUns16			version;

	// particle flags - lots of settings
	UUtUns32			flags;
	UUtUns32			flags2;

	// actionmask for newly created particles
	UUtUns32			actionmask;

	// information about the number of structures in this particle class
	UUtUns16			num_variables;
	UUtUns16			num_actions;
	UUtUns16			num_emitters;

	// particle classes that are 'decorative' are not necessary for gameplay
	// and they reduce the number created so that they never exceed max_particles
	UUtUns16			max_particles;

	// the set of actions to be done when an event happens (update, explode, etc)
	P3tActionList		eventlist[P3cMaxNumEvents];

	// particle lifetime
	P3tValue			lifetime;

	// collision radius
	P3tValue			collision_radius;

	// how to draw the particle
	P3tAppearanceVersion_Decal appearance;

	// pointers to the variable-length arrays in this particle class
	// (initialised on structure loading)
	P3tVariableInfo *	variable;
	P3tActionInstance *	action;
	P3tEmitter *		emitter;

} P3tParticleVersion_MoreEvents;

// version 14 - added attractors
typedef struct P3tParticleVersion_Attractor {
	// total size of the particle definition
	UUtUns16			definition_size;

	// version of the definition
	UUtUns16			version;

	// particle flags - lots of settings
	UUtUns32			flags;
	UUtUns32			flags2;

	// actionmask for newly created particles
	UUtUns32			actionmask;

	// information about the number of structures in this particle class
	UUtUns16			num_variables;
	UUtUns16			num_actions;
	UUtUns16			num_emitters;

	// particle classes that are 'decorative' are not necessary for gameplay
	// and they reduce the number created so that they never exceed max_particles
	UUtUns16			max_particles;

	// the set of actions to be done when an event happens (update, explode, etc)
	P3tActionList		eventlist[P3cMaxNumEvents];

	// particle lifetime
	P3tValue			lifetime;

	// collision radius
	P3tValue			collision_radius;

	// how to draw the particle
	P3tAppearanceVersion_Decal appearance;

	// particle attractor control settings
	P3tAttractor_Version14	attractor;

	// pointers to the variable-length arrays in this particle class
	// (initialised on structure loading)
	P3tVariableInfo *	variable;
	P3tActionInstance *	action;
	P3tEmitter *		emitter;

} P3tParticleVersion_Attractor;

// version 15 - added angles to attractors
typedef struct P3tParticleVersion_AttractorAngle {
	// total size of the particle definition
	UUtUns16			definition_size;

	// version of the definition
	UUtUns16			version;

	// particle flags - lots of settings
	UUtUns32			flags;
	UUtUns32			flags2;

	// actionmask for newly created particles
	UUtUns32			actionmask;

	// information about the number of structures in this particle class
	UUtUns16			num_variables;
	UUtUns16			num_actions;
	UUtUns16			num_emitters;

	// particle classes that are 'decorative' are not necessary for gameplay
	// and they reduce the number created so that they never exceed max_particles
	UUtUns16			max_particles;

	// the set of actions to be done when an event happens (update, explode, etc)
	P3tActionList		eventlist[P3cMaxNumEvents];

	// particle lifetime
	P3tValue			lifetime;

	// collision radius
	P3tValue			collision_radius;

	// how to draw the particle
	P3tAppearanceVersion_Decal appearance;

	// particle attractor control settings
	P3tAttractor_Version15 attractor;

	// pointers to the variable-length arrays in this particle class
	// (initialised on structure loading)
	P3tVariableInfo *	variable;
	P3tActionInstance *	action;
	P3tEmitter *		emitter;

} P3tParticleVersion_AttractorAngle;

// version 16 - added a pointer cache to attractors
typedef struct P3tParticleVersion_AttractorPtr {
	// total size of the particle definition
	UUtUns16			definition_size;

	// version of the definition
	UUtUns16			version;

	// particle flags - lots of settings
	UUtUns32			flags;
	UUtUns32			flags2;

	// actionmask for newly created particles
	UUtUns32			actionmask;

	// information about the number of structures in this particle class
	UUtUns16			num_variables;
	UUtUns16			num_actions;
	UUtUns16			num_emitters;

	// particle classes that are 'decorative' are not necessary for gameplay
	// and they reduce the number created so that they never exceed max_particles
	UUtUns16			max_particles;

	// the set of actions to be done when an event happens (update, explode, etc)
	P3tActionList		eventlist[P3cMaxNumEvents];

	// particle lifetime
	P3tValue			lifetime;

	// collision radius
	P3tValue			collision_radius;

	// how to draw the particle
	P3tAppearanceVersion_Decal appearance;

	// particle attractor control settings
	P3tAttractor		attractor;

	// pointers to the variable-length arrays in this particle class
	// (initialised on structure loading)
	P3tVariableInfo *	variable;
	P3tActionInstance *	action;
	P3tEmitter *		emitter;

} P3tParticleVersion_AttractorPtr;

// version 17 - added the decal_wrap parameter to appearance
typedef struct P3tParticleVersion_DecalWrap {
	// total size of the particle definition
	UUtUns16			definition_size;

	// version of the definition
	UUtUns16			version;

	// particle flags - lots of settings
	UUtUns32			flags;
	UUtUns32			flags2;

	// actionmask for newly created particles
	UUtUns32			actionmask;

	// information about the number of structures in this particle class
	UUtUns16			num_variables;
	UUtUns16			num_actions;
	UUtUns16			num_emitters;

	// particle classes that are 'decorative' are not necessary for gameplay
	// and they reduce the number created so that they never exceed max_particles
	UUtUns16			max_particles;

	// the set of actions to be done when an event happens (update, explode, etc)
	P3tActionList		eventlist[P3cMaxNumEvents];

	// particle lifetime
	P3tValue			lifetime;

	// collision radius
	P3tValue			collision_radius;

	// how to draw the particle
	P3tAppearance		appearance;

	// particle attractor control settings
	P3tAttractor		attractor;

	// pointers to the variable-length arrays in this particle class
	// (initialised on structure loading)
	P3tVariableInfo *	variable;
	P3tActionInstance *	action;
	P3tEmitter *		emitter;

} P3tParticleVersion_DecalWrap;

// version 18 - added the dodge_radius, alert_radius and flyby_soundname parameters
typedef struct P3tParticleDefinition {
	// total size of the particle definition
	UUtUns16			definition_size;

	// version of the definition
	UUtUns16			version;

	// particle flags - lots of settings
	UUtUns32			flags;
	UUtUns32			flags2;

	// actionmask for newly created particles
	UUtUns32			actionmask;

	// information about the number of structures in this particle class
	UUtUns16			num_variables;
	UUtUns16			num_actions;
	UUtUns16			num_emitters;

	// particle classes that are 'decorative' are not necessary for gameplay
	// and they reduce the number created so that they never exceed max_particles
	UUtUns16			max_particles;

	// the set of actions to be done when an event happens (update, explode, etc)
	P3tActionList		eventlist[P3cMaxNumEvents];

	// particle lifetime
	P3tValue			lifetime;

	// collision radius
	P3tValue			collision_radius;

	// radius that the AI will try to get out of the way of, and AI alert sound radius
	float				dodge_radius;
	float				alert_radius;

	// impulse sound to play upon flyby
	P3tString			flyby_soundname;

	// how to draw the particle
	P3tAppearance		appearance;

	// particle attractor control settings
	P3tAttractor		attractor;

	// pointers to the variable-length arrays in this particle class
	// (initialised on structure loading)
	P3tVariableInfo *	variable;
	P3tActionInstance *	action;
	P3tEmitter *		emitter;

} P3tParticleDefinition;

enum P3tParticleDefinition_Version {
	P3cVersion_Initial		= 0,
	P3cVersion_EventList	= 1,
	P3cVersion_Collision	= 2,
	P3cVersion_ActionPtr	= 3,
	P3cVersion_XOffset		= 4,
	P3cVersion_Tint			= 5,
	P3cVersion_ActionMask	= 6,
	P3cVersion_EmitterLink	= 7,
	P3cVersion_MaxContrail	= 8,
	P3cVersion_EmitterOrient= 9,
	P3cVersion_LensflareDist= 10,
	P3cVersion_Flags2		= 11,
	P3cVersion_Decal		= 12,
	P3cVersion_MoreEvents	= 13,
	P3cVersion_Attractor	= 14,
	P3cVersion_AttractorAngle= 15,
	P3cVersion_AttractorPtr	= 16,
	P3cVersion_DecalWrap	= 17,
	P3cVersion_Dodge		= 18
};

// this is ugly - the best way of doing this would be to
// #define P3tParticleDefinition to be the latest P3tParticleVersion_Whatever
// however this breaks VC++'s auto-name completion, which is nice to have.
#define P3tParticleVersion_Dodge			P3tParticleDefinition
#define P3cVersion_Current					P3cVersion_Dodge





#if defined(DEBUGGING) && DEBUGGING
// check a particle's class_size is correct, at load time, before or after resizing
#define P3mVerifyDefinitionSize(p)  ((p)->definition_size == sizeof(P3tParticleDefinition)					\
			+ (p)->num_variables * sizeof(P3tVariableInfo)													\
			+ (p)->num_actions   * sizeof(P3tActionInstance)												\
			+ (p)->num_emitters  * sizeof(P3tEmitter))

#else
#define P3mVerifyDefinitionSize(p) (UUcTrue)
#endif


/*
 * a particle reference
 */

// particle references are useful for chaining particle effects together in various ways
typedef UUtUns32 P3tParticleReference;

// this is a packed bitfield, viz:
//              vals     bits
//    salt      0-15     0- 3
//    class     0-1023   4-13
//    index     0-1023  14-23
//    block     0-127   24-30
//    is_valid	0-1		31
//
//  this refers to a particle of class = &P3gClassTable[class], located at memory
// P3gMemory.block[block] + (index << 6). salt is there for detecting when the particle dies
// and/or is replaced by another
#define P3cParticleReference_SaltOffset     0
#define P3cParticleReference_ClassOffset    4
#define P3cParticleReference_IndexOffset    14
#define P3cParticleReference_BlockOffset    24

#define P3cParticleReference_SaltBits		4
#define P3cParticleReference_ClassBits		10
#define P3cParticleReference_IndexBits		10
#define P3cParticleReference_BlockBits		7

#define P3cParticleReference_SaltMask		((1 << P3cParticleReference_SaltBits) - 1)
#define P3cParticleReference_ClassMask		((1 << P3cParticleReference_ClassBits) - 1)
#define P3cParticleReference_IndexMask		((1 << P3cParticleReference_IndexBits) - 1)
#define P3cParticleReference_BlockMask		((1 << P3cParticleReference_BlockBits) - 1)

#define P3cParticleReference_Null			0
#define P3cParticleReference_Valid			(1 << 31)

#define P3mUnpackParticleReference(ref, classptr, particleptr) {									\
	UUtUns32 classval, indexval, blockval;															\
																									\
	classval = ((ref) & (P3cParticleReference_ClassMask << P3cParticleReference_ClassOffset))		\
		>> P3cParticleReference_ClassOffset;														\
	(classptr) = &P3gClassTable[classval];															\
																									\
	indexval = ((ref) & (P3cParticleReference_IndexMask << P3cParticleReference_IndexOffset))		\
		>> (P3cParticleReference_IndexOffset - 6);													\
	blockval = ((ref) & (P3cParticleReference_BlockMask << P3cParticleReference_BlockOffset)) 		\
		>> P3cParticleReference_BlockOffset;														\
	(particleptr) = (P3tParticle *) (P3gMemory.block[blockval] + indexval);							\
}

/*
 * a particle itself
 */

#define P3cNoVar		((UUtUns16) -1)

// a particle is made up of:
//
// sizeof(P3tParticleHeader)					  constant data for every particle
// num_emitters * sizeof(P3tParticleEmitterData)  time of last particle emission, etc - for each emitter
// varies                                         optional variables as flagged by class->definition->flags
// varies                                         num_variables vars
//
// this totals class->particle_size bytes

enum P3tParticleFlags {
	P3cParticleFlag_Hidden									= (1 << 0),
	P3cParticleFlag_FadeOut									= (1 << 1),
	P3cParticleFlag_Stopped									= (1 << 2),
	P3cParticleFlag_Chop									= (1 << 3),
	P3cParticleFlag_Deleted									= (1 << 4),
	P3cParticleFlag_OrientToVelocity						= (1 << 5),
	P3cParticleFlag_VelocityChanged							= (1 << 6),
	P3cParticleFlag_ExplodeOnContact						= (1 << 7),
	P3cParticleFlag_PlayingAmbientSound						= (1 << 8),
	P3cParticleFlag_PositionChanged							= (1 << 9),
	P3cParticleFlag_Temporary								= (1 << 10),
	P3cParticleFlag_Decal									= (1 << 11),
	P3cParticleFlag_DecalStatic								= (1 << 12),
	P3cParticleFlag_AlwaysUpdatePosition					= (1 << 13),
	P3cParticleFlag_HasAttractor							= (1 << 14),
	P3cParticleFlag_BreakLinksToMe							= (1 << 15),
	P3cParticleFlag_CutsceneKilled							= (1 << 16)
	// FIXME: visibility might eventually go here
};

#define P3cParticleMask_VelocityRequiresRecompute   (P3cParticleFlag_OrientToVelocity | P3cParticleFlag_VelocityChanged)
#define P3cParticleMask_AmbientSoundUpdate			(P3cParticleFlag_PlayingAmbientSound | P3cParticleFlag_PositionChanged)

typedef struct P3tParticleHeader {
	UUtUns16						class_index;		// into P3gClassTable
	UUtUns8							ai_projectile_index;// used by AI knowledge routines
	UUtUns8							sound_flyby_index;	// used by sound code for tracking flyby sounds
	UUtUns32						flags;				// P3tParticleFlags
	UUtUns32						actionmask;			// bitvector to determine which actions in the eventlist are off
	M3tPoint3D						position;
	SStPlayID						current_sound;
//	struct P3tParticleSystem *		systemptr;
	UUtUns32						current_tick;		// in game ticks - reflects what frame we have been updated to
	float							current_time;		// in seconds - reflects when our position has been updated to
	float							lifetime;			// in seconds remaining. 0 indicates no limit
	float							fadeout_time;		// if the fadeout flag is set, then alpha is set to be
														// lifetime / fadeout_time
														// also used if chopping. chop = 1.0 - lifetime / fadeout_time

	// particle references - self_ref points to us, user_ref
	// can be used by other routines
	P3tParticleReference			self_ref, user_ref;

	// doubly-linked list that contains all particles of this class
	// note: if the particle is deleted then nextptr instead points to the next particle
	// in the freelist
	struct P3tParticle				*nextptr, *prevptr;

	// doubly-linked list that contains all decorative particles of this size class
	struct P3tParticle				*next_decorative, *prev_decorative;
} P3tParticleHeader;

typedef struct P3tParticle {
	P3tParticleHeader		header;
	UUtUns8					vararray[1];	// really [class->particle_size - sizeof(header)]
} P3tParticle;

// data stored for each emitter in a particle
#define P3cEmitterDisabled			UUcMaxUns16
#define P3cEmitImmediately			(1e+09)				// any constant will do
typedef struct P3tParticleEmitterData {
	UUtUns16						num_emitted;		// a number, or P3cEmitterDisabled
	float							time_last_emission;
	P3tParticleReference			last_particle;
} P3tParticleEmitterData;

/*
 * information about how a particle class's memory is being handled
 */

typedef struct P3tMemoryBlock {
	UUtUns16			size;			// NB: this does **NOT** include the BDtHeader at th start of the block
										// so the memory block is actually 'size + sizeof(BDtHeader)' bytes long

	void *				block;			// if NULL, indicates the memory resides in a .dat file
										// and doesn't have to be deallocated
} P3tMemoryBlock;

#if PARTICLE_PERF_ENABLE
/*
 * particle performance tracking
 */

#define P3cPerfHistoryFrames				30
#define P3cPerfDisplayClasses				20
#define P3cPerfDisplayLines					P3cPerfDisplayClasses + 3

// particle performance is measured in tens of microseconds
#define P3cPerfTimeScale					1e+05f

enum
{
	P3cPerfEvent_Particle					= 0,
	P3cPerfEvent_Update						= 1,
	P3cPerfEvent_Action						= 2,
	P3cPerfEvent_VarEval					= 3,
	P3cPerfEvent_EnvPointCol				= 4,
	P3cPerfEvent_EnvSphereCol				= 5,
	P3cPerfEvent_PhyFaceCol					= 6,
	P3cPerfEvent_PhySphereCol				= 7,
	P3cPerfEvent_PhyCharCol					= 8,
	P3cPerfEvent_SndAmbient					= 9,
	P3cPerfEvent_SndImpulse					= 10,

	P3cPerfEvent_Max						= 11
};

#define P3mPerfTrack(classptr,event_type)     { classptr->perf_data[P3gPerfIndex][event_type]++; }

typedef struct {
	const char *name;
	UUtUns32 mask;
} P3tPerfMask;

extern const char *P3cPerfEventName[P3cPerfEvent_Max];
extern P3tPerfMask P3cPerfMasks[];

#else

#define P3mPerfTrack(classptr,event_type)

#endif

/*
 * the memory-resident structure that holds a class of particles
 * holds everything about a particle class that isn't part of its definition
 */

typedef enum P3tQualitySettings {
	P3cQuality_High				= 0,
	P3cQuality_Medium			= 1,
	P3cQuality_Low				= 2
} P3tQualitySettings;

enum {
	P3cParticleClass_NonPersistentFlag_HasSoundPointers			= (1 << 0),
	P3cParticleClass_NonPersistentFlag_HasTemporaryParticles	= (1 << 1),
	P3cParticleClass_NonPersistentFlag_DoNotSave				= (1 << 2),
	P3cParticleClass_NonPersistentFlag_WarnedAboutNotFound		= (1 << 3),
	P3cParticleClass_NonPersistentFlag_DoNotLoad				= (1 << 4),
	P3cParticleClass_NonPersistentFlag_OldVersion				= (1 << 5),
	P3cParticleClass_NonPersistentFlag_Disabled					= (1 << 6),		// determined by particle quality settings
	P3cParticleClass_NonPersistentFlag_Locked					= (1 << 7),
	P3cParticleClass_NonPersistentFlag_Dirty					= (1 << 8),
	P3cParticleClass_NonPersistentFlag_ByteSwap					= (1 << 9),
	P3cParticleClass_NonPersistentFlag_Active					= (1 << 10),
	P3cParticleClass_NonPersistentFlag_DeferredAddAtStart		= (1 << 11),
	P3cParticleClass_NonPersistentFlag_DeferredAddAtEnd			= (1 << 12),
	P3cParticleClass_NonPersistentFlag_DeferredRemoval			= (1 << 13)
};

#define P3cParticleClass_NonPersistentFlag_DeferredAddMask		(P3cParticleClass_NonPersistentFlag_DeferredAddAtStart | \
																 P3cParticleClass_NonPersistentFlag_DeferredAddAtEnd)

#define P3cClass_None							((UUtUns16) -1)

struct P3tParticleClass {
	char				classname[P3cParticleClassNameLength + 1];
	UUtUns32			non_persistent_flags;
	UUtUns32			data_size;
	P3tParticleDefinition *	definition;

	P3tMemoryBlock		memory;
	P3tMemoryBlock		originalcopy;	// created to store original status of a modified class
										// so that any changes can be undone

	UUtUns16			optionalvar_offset[P3cNumParticleOptionalVars];	// offset into vararray, -1 for not there

	UUtUns32			num_particles;
	UUtUns16			particle_size;
	UUtUns16			size_class;
	UUtUns16			prev_packed_emitters;	// used by P3rPackVariables
	UUtUns16			read_index;				// used to determine which class has precedence if several with the same name are found

	UUtUns16			prev_active_class;		// linked list of particle classes that have particles in them
	UUtUns16			next_active_class;

	UUtUns16			traverse_index;

	SStImpulse *		flyby_sound;

	// the ends of the doubly-linked particle chain
	P3tParticle			*headptr, *tailptr;

#if PARTICLE_PERF_ENABLE
	// performance counters... a buffer of P3cPerfHistoryFrames long, plus an accumulator
	UUtUns16			perf_t[P3cPerfHistoryFrames];
	UUtUns32			perf_t_accum;
	UUtUns16			perf_data[P3cPerfHistoryFrames][P3cPerfEvent_Max];
	UUtUns32			perf_accum[P3cPerfEvent_Max];
#endif
};


/*
 * the particle-system memory manager
 */

// size of arrays, and number of arrays allocated for particle storage. total particle memory
// must not exceed the product of these two numbers.
#define P3cMemory_MaxBlocks						100
#define P3cMemory_BlockSize						50000

// the number of particle size classes and their values
#define P3cMemory_ParticleSizeClasses			4
extern UUtUns16 P3gParticleSizeClass[P3cMemory_ParticleSizeClasses];

typedef struct P3tSizeClassInfo {
	UUtUns32			freelist_length;
	P3tParticle			*freelist_head;
	P3tParticle			*decorative_head, *decorative_tail;
} P3tSizeClassInfo;

typedef struct P3tParticleMemory {
	UUtUns16			num_blocks;
	UUtUns16			cur_block;
	UUtUns32			curblock_free;
	UUtUns32			total_used;

	UUtUns8 *			block[P3cMemory_MaxBlocks];

	P3tSizeClassInfo	sizeclass_info[P3cMemory_ParticleSizeClasses];
} P3tParticleMemory;

extern P3tParticleMemory P3gParticleMemory;

/*
 * a particle-system - the unit used to do visibility and drawing determination
 */

// CB: particle systems are mostly unimplemented, for now everything displays
// and updates every frame
typedef struct P3tParticleSystem {
	UUtUns16			system_id;
	UUtBool				update_system;
	UUtBool				is_visible;
} P3tParticleSystem;

/*
 * global structure for collisions
 */

typedef struct P3tCollisionGlobals {
	UUtBool				collided;
	UUtBool				env_collision;
	UUtBool				damaged_hit;
	M3tPoint3D			hit_point;
	M3tVector3D			hit_normal;
	M3tVector3D			hit_direction;
	UUtUns32			hit_gqindex;
	PHtPhysicsContext *	hit_context;
	UUtUns32			hit_partindex;			// character collisions only
	float				hit_t;
} P3tCollisionGlobals;

/*
 * data required to create a particle effect
 */

// location of an effect
typedef enum P3tEffectLocationType {
	P3cEffectLocationType_Default			= 0,
	P3cEffectLocationType_CollisionOffset	= 1,
	P3cEffectLocationType_FindWall			= 3,
	P3cEffectLocationType_Decal				= 4,
	P3cEffectLocationType_AttachedToObject	= 5,

	P3cEffectLocationType_Max				= 6
} P3tEffectLocationType;

typedef struct P3tEffectLocationData_CollisionOffset {
	float				offset;
} P3tEffectLocationData_CollisionOffset;

typedef struct P3tEffectLocationData_FindWall {
	float				radius;
	UUtBool				look_along_orientation;
} P3tEffectLocationData_FindWall;

typedef struct P3tEffectLocationData_Decal {
	UUtBool				static_decal;
	UUtBool				random_rotation;
} P3tEffectLocationData_Decal;

// P3tEffectSpecification contains information about the type of the effect
typedef struct P3tEffectSpecification {
	P3tParticleClass *			particle_class;
	P3tEnumCollisionOrientation	collision_orientation;

	P3tEffectLocationType		location_type;
	union {
		P3tEffectLocationData_CollisionOffset	collisionoffset;
		P3tEffectLocationData_FindWall			findwall;
		P3tEffectLocationData_Decal				decal;
	} location_data;
} P3tEffectSpecification;

// information about what caused an effect
typedef enum P3tEffectCauseType {
	P3cEffectCauseType_None					= 0,
	P3cEffectCauseType_ParticleHitWall		= 1,
	P3cEffectCauseType_ParticleHitPhysics	= 2,
	P3cEffectCauseType_MeleeHit				= 3,
	P3cEffectCauseType_CutsceneKilled		= 4,

	P3cEffectCauseType_Max					= 5
} P3tEffectCauseType;

typedef struct P3tEffectCauseData_ParticleHitWall {
	UUtUns32			gq_index;
} P3tEffectCauseData_ParticleHitWall;

typedef struct P3tEffectCauseData_ParticleHitPhysics {
	PHtPhysicsContext *	context;
} P3tEffectCauseData_ParticleHitPhysics;

typedef struct P3tEffectCauseData_MeleeHit {
	void *				attacker;
	void *				defender;
	UUtUns32			attacker_parts;
	UUtUns32			defender_parts;
	struct TRtAttack *	attack;
} P3tEffectCauseData_MeleeHit;

// P3tEffectData contains variables necessary to create an effect (of any specification)
typedef struct P3tEffectData {
	UUtUns32					time;
	P3tOwner					owner;
	M3tPoint3D					position;
	M3tVector3D					velocity;
	M3tVector3D					normal;					// only set for causes which have a surface associated with them
	M3tVector3D					upvector;				// only needed for decals that don't have random rotation
	float						decal_xscale;	// modifiers to class's scale... always 1.0 for dynamic decals, can be set for static decals
	float						decal_yscale;

	P3tParticleClass *			parent_class;
	P3tParticle *				parent_particle;
	M3tMatrix3x3 *				parent_orientation;		// both may be NULL if no parent
	M3tVector3D *				parent_velocity;
	M3tMatrix4x3 *				attach_matrix;

	P3tEffectCauseType			cause_type;
	union {
		P3tEffectCauseData_ParticleHitWall		particle_wall;
		P3tEffectCauseData_ParticleHitPhysics	particle_physics;
		P3tEffectCauseData_MeleeHit				melee_hit;
	} cause_data;
} P3tEffectData;


// =============================================
// decals...
// =============================================

typedef enum P3tDecalFlags 
{
	P3cDecalFlag_Static			= 0x0001,
	P3cDecalFlag_Manual			= 0x0002,
	P3cDecalFlag_CullBackFacing	= 0x0004,
	P3cDecalFlag_IgnoreAdjacency= 0x0008,
	P3cDecalFlag_FullBright		= 0x0010
} P3tDecalFlags;

typedef struct P3tDecalData 
{
	M3tDecalHeader				*decal_header;
	P3tParticleReference		particle;
	UUtUns16					block;
	UUtUns16					traverse_id;
	struct P3tDecalData			*next;
	struct P3tDecalData			*prev;
} P3tDecalData;

// =============================================
// P3rDraw non-draw flags
// =============================================
enum
{
	P3cParticleDisplay_Normal				= 0,
	P3cParticleDisplay_StaticDecals			= 1,
	P3cParticleDisplay_LensFlares			= 2
};

/*
 * interface to client system
 */

typedef void (*P3tParticleCallback)(P3tParticleClass *inClass, P3tParticle *inParticle);
typedef void (*P3tQualityCallback)(P3tQualitySettings *outSettings);
typedef void (*P3tClassCallback)(P3tParticleClass *inClass, UUtUns32 inUserData);

/*
 * globals and constants
 */

// memory manager
extern P3tParticleMemory		P3gMemory;

// information about each type of action
extern P3tActionTemplate	P3gActionInfo[];

#define P3cMaxParticleClasses	2048

extern const float P3gGravityAccel;

// macro to skip over the BDtHeader at the start of a memory block
#define P3mSkipHeader(x) ((P3tParticleDefinition *) ((UUtUns8 *) (x) + sizeof(BDtHeader)))
#define P3mBackHeader(x) ((BDtData *)				((UUtUns8 *) (x) - sizeof(BDtHeader)))

// table of all known particle classes
extern UUtUns16				P3gNumClasses;
extern P3tParticleClass		P3gClassTable[P3cMaxParticleClasses];

// an index into this table
typedef UUtUns16			P3tParticleClassID;

// set if any particle definitions are dirty
extern UUtBool				P3gDirty;

// particle binary data class constant
#define P3cBinaryDataClass	UUm4CharToUns32('P', 'A', 'R', '3')

// information stored about possible enumerated types
extern P3tEnumTypeInfo		P3gEnumTypeInfo[P3cEnumType_Max >> P3cEnumShift];

// debugging globals
extern UUtBool				P3gDrawEnvCollisions;
extern UUtBool				P3gEverythingBreakable;
extern UUtBool				P3gFurnitureBreakable;

#if PARTICLE_PERF_ENABLE
// particle performance display
extern UUtBool				P3gPerfDisplay;
extern UUtBool				P3gPerfLockClasses;
#endif

// =============================================
// prototypes
// =============================================

UUtError P3rInitialize(void);

void P3rTerminate(void);

// perform post-load operations on all particle classes
void P3rLoad_PostProcess(void);

// set up for the start of a level
UUtError P3rLevelBegin(void);

// kill all particles
void P3rKillAll(void);

// kill all particles that we don't want around during a cutscene
void P3rRemoveDangerousProjectiles(void);

// register that the level unload process is beginning
void P3rNotifyUnloadBegin(void);

// shut down at the end of a level
void P3rLevelEnd(void);

// enable or disable collision debugging
void P3rDebugCollision(UUtBool inEnable);

// change the global detail setting during a level
void P3rChangeDetail(P3tQualitySettings inSettings);

/*
 * memory management
 */

// allocate a new particle
P3tParticle *P3rNewParticle(P3tParticleClass *inClass);

// get space for a particle off the freelist for the class's size range
P3tParticle *P3rFindFreeParticle(P3tParticleClass *inClass);

// allocate a new particle out of global block storage
P3tParticle *P3rNewParticleStorage(P3tParticleMemory *inMemory, P3tParticleClass *inClass,
								   UUtUns16 inAllocateSize, UUtUns16 inActualSize);

// find an old decorative particle of the right size class that can be overwritten
P3tParticle *P3rOverwriteDecorativeParticle(P3tParticleClass *inClass);

// kill a particle
void P3rKillParticle(P3tParticleClass *inClass, P3tParticle *inParticle);

// dispose of a particle (internal use only)
void P3rDisposeParticle(P3tParticleClass *inClass, P3tParticle *inParticle);

// delete a particle (internal use only)
void P3rDeleteParticle(P3tParticleClass *inParticleClass, P3tParticle *inParticle);

// mark a particle as free (internal use only)
void P3rMarkParticleAsFree(P3tParticleClass *inClass, P3tParticle *inParticle);

// kill all temporary particles
void P3rKillTemporaryParticles(void);

/*
 * interface for particle editing
 */

typedef UUtUns16 P3tParticleClassIterateState;

#define P3cParticleClass_None			0

// iterate over particle classes
UUtBool P3rIterateClasses(P3tParticleClassIterateState *ioState,
						  P3tParticleClass **		outClass);

// build a binary-data representation of a particle class
UUtUns8 *P3rBuildBinaryData(UUtBool					inSwapBytes,
							P3tParticleDefinition *		inClassPtr,
							UUtUns16 *				outDataSize);

// mark that a particle's definition has been changed
void P3rMakeDirty(P3tParticleClass *ioClass, UUtBool inDirty);

// are there any dirty particles? does not check each individualy
UUtBool P3rAnyDirty(void);

// search all particles to determine if there are any dirty
UUtBool P3rUpdateDirty(void);

// is a particle dirty?
UUtBool P3rIsDirty(P3tParticleClass *inClass);

// revert a particle to its previous state
UUtBool P3rRevert(P3tParticleClass *inClass);

// print out a variable for the variable list
void P3rDescribeVariable(P3tParticleClass *inClass, P3tVariableInfo *inVar, char *outString);

// print out a variable type for the variable list
void P3rDescribeDataType(P3tDataType inVarType, char *outString);

// print out a value to a string
UUtBool P3rDescribeValue(P3tParticleClass *inClass, P3tDataType inType, P3tValue *inValue, char *outString);

// print out a description of an emitter's link type
UUtBool P3rDescribeEmitterLinkType(P3tParticleClass *inClass, P3tEmitter *inEmitter, P3tEmitterLinkType inLinkType,
									char *outString);

// set up the default initialiser for a newly created variable
void P3rSetDefaultInitialiser(P3tDataType inVarType, P3tValue *outValue);

// set up the defaults for a newly created action
void P3rSetActionDefaults(P3tActionInstance *ioAction);

// allocate space for a new variable in a particle class
UUtError P3rClass_NewVar(P3tParticleClass *inClass, P3tVariableInfo **outVar);

// grow an action list. requires that we resize the particle definition, and move down
// every action list below the point that's growing.
UUtError P3rClass_NewAction(P3tParticleClass *ioClass, UUtUns16 inListIndex, UUtUns16 inActionIndex);

// allocate space for a new emitter in a particle class
UUtError P3rClass_NewEmitter(P3tParticleClass *inClass, P3tEmitter **outEmitter);

// delete a variable from a particle class
UUtError P3rClass_DeleteVar(P3tParticleClass *inClass, UUtUns16 inVarNum);

// delete an action from a particle class
UUtError P3rClass_DeleteAction(P3tParticleClass *ioClass, UUtUns16 inListIndex, UUtUns16 inActionIndex);

// delete an emitter from a particle class
UUtError P3rClass_DeleteEmitter(P3tParticleClass *inClass, UUtUns16 inEmitterNum);

// get the template for an action
UUtError P3rGetActionInfo(P3tActionInstance *inAction, P3tActionTemplate **outTemplate);

// swap the actions in the particle class
void P3rSwapTwoActions(P3tParticleClass *inClass, UUtUns16 inIndex1, UUtUns16 inIndex2);

// get the enum info for an enumerated constant
void P3rGetEnumInfo(P3tParticleClass *inClass, P3tEnumType inType, P3tEnumTypeInfo *outInfo);

// describe an enumerated constant
UUtBool P3rDescribeEnum(P3tParticleClass *inClass, P3tEnumType inType, UUtUns32 inVal, char *outDescription);

// get data about a subsection of the emitter's properties (rate, position, etc)
void P3rEmitterSubsection(UUtUns16 inSection, P3tEmitter *inEmitter, UUtUns32 **outPropertyPtr,
						  P3tEmitterSettingDescription **outClass, UUtUns16 *outOffset,
						  char *outDesc);
/*
 * particle class loading
 */

// set up action runtime pointers - called at class load time
void P3rSetupActionData(P3tParticleClass *new_class);

// update the runtime pointer for an action - called by the editing code
void P3rUpdateActionData(P3tParticleClass *inClass, P3tActionInstance *inAction);

// sound data has been moved, re-calculate any cached sound pointers
void P3rRecalculateSoundPointers(void);

// write broken sound links to a file
void P3rListBrokenLinks(BFtFile *inFile);

// read a particle class's definition from a binary file,
// performing byte-swapping as necessary.
UUtError P3rLoadParticleDefinition(const char	*inIdentifier,
									BDtData		*ioBinaryData,
									UUtBool		inSwapIt,
									UUtBool		inLocked,
									UUtBool		inAllocated);

// find a particle class in the global particle class array
P3tParticleClass *P3rGetParticleClass(char *inIdentifier);

// byte-swap parts of a particle class's definition
UUtError P3iSwap_ParticleDefinition	(void *ioPtr, UUtBool inCurrentlyUsable);
UUtError P3iSwap_Variable			(void *ioPtr, UUtBool inCurrentlyUsable);
UUtError P3iSwap_ActionInstance		(void *ioPtr, UUtBool inCurrentlyUsable);
UUtError P3iSwap_Value				(void *ioPtr, UUtBool inCurrentlyUsable);
UUtError P3iSwap_VarRef				(void *ioPtr, UUtBool inCurrentlyUsable);
UUtError P3iSwap_Emitter			(void *ioPtr, UUtBool inCurrentlyUsable);
UUtError P3iSwap_EffectSpecification(void *ioPtr, UUtBool inCurrentlyUsable);

// get the size of a variable
UUtUns16 P3iGetDataSize(P3tDataType inDataType);

// Find the location of a variable in a particle, from its name
UUtError P3iFindVariable(P3tParticleClass *inParticleClass, P3tVariableReference *ioVarRef);

// re-pack the variables in the particle class.
void P3rPackVariables(P3tParticleClass *inClass, UUtBool inChangedVars);

// traverse over all variable references in the particle definition
void P3rTraverseVarRef(P3tParticleDefinition *inDefinition,
					   P3tVarTraverseProc inProc, UUtBool inGiveLocation);

// create a new (blank) particle class
P3tParticleClass *P3rNewParticleClass(char *inName, UUtBool inMakeDirty);

// duplicate an existing particle class
P3tParticleClass *P3rDuplicateParticleClass(P3tParticleClass *inClass, char *inName);

// for dumping out particle classes to a text file - used for manually merging particle changes
void P3rDumpParticles(void);

#if TOOL_VERSION
// list all particle classes that run collision
void P3rListCollision(void);
#endif

// traverse particle classes recursively
void P3rSetupTraverse(void);
UUtBool P3rTraverseParticleClass(P3tParticleClass *inClass, P3tClassCallback inCallback, UUtUns32 inUserData);

// precache all the components that a particle class will need to be created and drawn
void P3rPrecacheParticleClass(P3tParticleClass *inClass);

/*
 * particle creation
 */

// register client particlesystem callbacks
void P3rRegisterCallbacks(P3tParticleCallback inCreationCallback, P3tParticleCallback inDeletionCallback, P3tParticleCallback inUpdateCallback,
							P3tQualityCallback inQualityCallback);

// create a new particle and set up initial values
P3tParticle *P3rCreateParticle(P3tParticleClass *inParticleClass, UUtUns32 inTime);

// create a particle at a given location
P3tParticle *P3rCreateEffect(P3tEffectSpecification *inSpecification, P3tEffectData *inData);

// get a random value from 0 to 1.
static UUcInline float P3rRandom(void)
{
	// S.S.
	static const float multiplier = (1.0f / (float)UUcMaxUns16);
	return ((float)UUrLocalRandom() * multiplier);
}

// get a random value from -1 to 1.
#if UUmCompiler == UUmCompiler_MWerks

#define P3mSignedRandom() \
	(UUrLocalRandom() * (2.0f / (float)UUcMaxUns16) - 1.0f)
	
#else

static UUcInline float P3mSignedRandom(void)
{
	static const float multiplier = (2.0f / (float)UUcMaxUns16);
	return (((float)UUrLocalRandom() * multiplier) - 1.0f);	
}

#endif

// get a random direction in 3D
void P3rGetRandomUnitVector3D(M3tVector3D *outVector);

// calculates random values normally distributed about 0, std deviation 1, range ~ +/- 3
float P3rNormalDistribution(void);

// determine whether newly created particles will be temporary
void P3rSetTemporaryCreation(UUtBool inTempCreate);

// set and clear the global tint
void P3rSetGlobalTint(IMtShade inTint);
void P3rClearGlobalTint(void);
UUtBool P3rSetTint(P3tParticleClass *inClass, P3tParticle *inParticle, IMtShade inTint);

/*
 * particle update
 */

// build an orientation for a particle
void P3rConstructOrientation(P3tEmitterOrientationType inOrientDir, P3tEmitterOrientationType inUpDir,
							 M3tMatrix3x3 *inParentOrientation, M3tVector3D *inEmitOffset, M3tVector3D *inParticleDir,
							 M3tMatrix3x3 *outOrientation);

// does a value depend on which particle is evaluating it?
UUtBool P3iValueIsDynamic(P3tValue *inValue);

// calculate the value of a variable from the stored rule. inParticle may be
// NULL for values that aren't "copy from another variable".
//
// for every data type EXCEPT string, this writes the value into outStore
// for string, it writes a pointer to the string
UUtBool P3iCalculateValue(P3tParticle *inParticle, P3tValue *inValueDef,
						   void *outStore);

// update all particles. called every tick for essential, once every display update for decorative
void P3rUpdate(UUtUns32 inNewTime, UUtBool inEssential);

// send an event to a particle. returns UUcTrue if the particle dies.
UUtBool P3rSendEvent(P3tParticleClass *inClass, P3tParticle *inParticle, UUtUns16 inEventIndex, float inTime);

/*
 * particle display
 */

// display all particles
void P3rDisplay(UUtBool inLensFlares);

// display a class of geometry particles
void P3rDisplayClass_Geometry(P3tParticleClass *inClass, UUtUns32 inTime, UUtUns32 inDeltaTime);

// display a class of decal particles
void P3rDisplayClass_Decal(P3tParticleClass *inClass, UUtUns32 inTime, UUtUns32 inDeltaTime);

// display a class of vector particles
void P3rDisplayClass_Vector(P3tParticleClass *inClass, UUtUns32 inTime, UUtUns32 inDeltaTime);

// display a class of sprite particles
void P3rDisplayClass_Sprite(P3tParticleClass *inClass, UUtUns32 inTime, UUtUns32 inDeltaTime,
							M3tGeomState_SpriteMode inMode);

// display a class of contrail particles
void P3rDisplayClass_Contrail(P3tParticleClass *inClass, UUtUns32 inTime, UUtUns32 inDeltaTime,
							  M3tGeomState_SpriteMode inMode, UUtBool inEmitter);

// get the alpha multiplier due to lensflare fading
UUtUns16 P3rLensFlare_Fade(P3tAppearance *inAppearance, UUtBool inVisible, UUtUns32 inDeltaTime,
						   UUtUns16 inAlpha, UUtUns32 *ioLensFrames);

// notify particlesystem of sky visibility
void P3rNotifySkyVisible(UUtBool inVisible);

/*
 * particle interaction with environment
 */
							  
// damage a quad in the environment
UUtBool P3iDamageQuad(AKtEnvironment *inEnvironment, UUtUns32 inGQIndex, float inDamage,
					  M3tPoint3D *inBlastCenter, M3tVector3D *inBlastDir, float inBlastRadius,
					  UUtBool inForceBreak);

/*
 * decals
 */

// global routines
void P3rDecalTerminate(void);
UUtError P3rDecalInitialize(void);
void P3rDecal_LevelBegin(void);
void P3rDecalDeleteAllDynamic(void);
void P3rVerifyDecalLists(void);

#if defined(DEBUGGING) && DEBUGGING
#define P3mVerifyDecals()		P3rVerifyDecalLists();
#else
#define P3mVerifyDecals()
#endif

// creation and deletion
UUtError P3rCreateDecal(M3tTextureMap *inTexture, UUtUns32 inGQIndex, UUtUns32 inDecalFlags, float inWrapAngle, M3tVector3D *inDirection,
						M3tVector3D *inPosition, M3tVector3D *inFwdVector, M3tVector3D *inUpVector, M3tVector3D *inRightVector,
						float inXScale, float inYScale, UUtUns16 inAlpha, IMtShade inTint, P3tDecalData *inDecalData,
						UUtUns32 inManualBufferSize);
void P3rDeleteDecal(P3tDecalData *inDecal, UUtUns32 inFlags);

// rendering
void P3rDisplayDynamicDecals(void);
void P3rDisplayStaticDecals(void);

/*
 * attractors
 */

void P3rSetupAttractorPointers(P3tParticleClass *inClass);
UUtBool P3rDecodeAttractorIndex(UUtUns32 inAttractorID, UUtBool inPredictVelocity, M3tPoint3D *outPoint, M3tPoint3D *outVelocity);
void P3rIterateAttractors(P3tParticleClass *inClass, P3tParticle *inParticle, UUtUns32 inDelay);
void P3rFindAttractor(P3tParticleClass *inClass, P3tParticle *inParticle, UUtUns32 inDelay);

/*
 * performance
 */

#if PARTICLE_PERF_ENABLE
// set the display of different performance areas
void P3rPerformanceToggle(UUtUns32 inMask);

// frame-level performance routines
void P3rPerformanceBegin(UUtUns32 inTime);
void P3rPerformanceEnd(void);
#endif

/*
 * access to particles' optional values
 */

M3tPoint3D *P3rGetPositionPtr(P3tParticleClass *inClass, P3tParticle *inParticle, UUtBool *inDead);

// this gets the real position of a particle, taking everything into account. returns is_dead.
UUtBool P3rGetRealPosition(P3tParticleClass *inClass, P3tParticle *inParticle, M3tPoint3D *outPosition);

// this gets the velocity by following lock-to-link references, if there are any
M3tVector3D *P3rGetRealVelocityPtr(P3tParticleClass *inClass, P3tParticle *inParticle);

static UUcInline M3tVector3D *P3rGetVelocityPtr(P3tParticleClass *inClass, P3tParticle *inParticle)
{
	if (inClass->definition->flags & P3cParticleClassFlag_HasVelocity) {
		return (M3tVector3D *) &inParticle->vararray[inClass->optionalvar_offset[P3cParticleOptionalVar_Velocity]];
	} else {
		return NULL;
	}
}

static UUcInline M3tMatrix3x3 *P3rGetOrientationPtr(P3tParticleClass *inClass, P3tParticle *inParticle)
{
	if (inClass->definition->flags & P3cParticleClassFlag_HasOrientation) {
		return (M3tMatrix3x3 *) &inParticle->vararray[inClass->optionalvar_offset[P3cParticleOptionalVar_Orientation]];
	} else {
		return NULL;
	}
}

static UUcInline M3tVector3D *P3rGetOffsetPosPtr(P3tParticleClass *inClass, P3tParticle *inParticle)
{
	if (inClass->definition->flags & P3cParticleClassFlag_HasOffsetPos) {
		return (M3tVector3D *) &inParticle->vararray[inClass->optionalvar_offset[P3cParticleOptionalVar_OffsetPos]];
	} else {
		return NULL;
	}
}

static UUcInline M3tMatrix4x3 **P3rGetDynamicMatrixPtr(P3tParticleClass *inClass, P3tParticle *inParticle)
{
	if (inClass->definition->flags & P3cParticleClassFlag_HasDynamicMatrix) {
		return (M3tMatrix4x3 **) &inParticle->vararray[inClass->optionalvar_offset[P3cParticleOptionalVar_DynamicMatrix]];
	} else {
		return NULL;
	}
}

static UUcInline UUtUns32 *P3rGetTextureIndexPtr(P3tParticleClass *inClass, P3tParticle *inParticle)
{
	if (inClass->definition->flags & P3cParticleClassFlag_HasTextureIndex) {
		return (UUtUns32 *) &inParticle->vararray[inClass->optionalvar_offset[P3cParticleOptionalVar_TextureIndex]];
	} else {
		return NULL;
	}
}

static UUcInline UUtUns32 *P3rGetTextureTimePtr(P3tParticleClass *inClass, P3tParticle *inParticle)
{
	if (inClass->definition->flags & P3cParticleClassFlag_HasTextureTime) {
		return (UUtUns32 *) &inParticle->vararray[inClass->optionalvar_offset[P3cParticleOptionalVar_TextureTime]];
	} else {
		return NULL;
	}
}

static UUcInline P3tOwner *P3rGetOwnerPtr(P3tParticleClass *inClass, P3tParticle *inParticle)
{
	if (inClass->definition->flags & P3cParticleClassFlag_HasOwner) {
		return (P3tOwner *) &inParticle->vararray[inClass->optionalvar_offset[P3cParticleOptionalVar_Owner]];
	} else {
		return NULL;
	}
}

static UUcInline M3tContrailData *P3rGetContrailDataPtr(P3tParticleClass *inClass, P3tParticle *inParticle)
{
	if (inClass->definition->flags & P3cParticleClassFlag_HasContrailData) {
		return (M3tContrailData *) &inParticle->vararray[inClass->optionalvar_offset[P3cParticleOptionalVar_ContrailData]];
	} else {
		return NULL;
	}
}

static UUcInline P3tDecalData *P3rGetDecalDataPtr(P3tParticleClass *inClass, P3tParticle *inParticle)
{
	if (inClass->definition->flags & P3cParticleClassFlag_HasDecalData) {
		return (P3tDecalData *) &inParticle->vararray[inClass->optionalvar_offset[P3cParticleOptionalVar_DecalData]];
	} else {
		return NULL;
	}
}

static UUcInline P3tPrevPosition *P3rGetPrevPositionPtr(P3tParticleClass *inClass, P3tParticle *inParticle)
{
	if (inClass->definition->flags & P3cParticleClassFlag_HasPrevPosition) {
		return (P3tPrevPosition *) &inParticle->vararray[inClass->optionalvar_offset[P3cParticleOptionalVar_PrevPosition]];
	} else {
		return NULL;
	}
}

static UUcInline UUtUns32 *P3rGetLensFramesPtr(P3tParticleClass *inClass, P3tParticle *inParticle)
{
	if (inClass->definition->flags & P3cParticleClassFlag_HasLensFrames) {
		return (UUtUns32 *) &inParticle->vararray[inClass->optionalvar_offset[P3cParticleOptionalVar_LensFrames]];
	} else {
		return NULL;
	}
}

static UUcInline IMtShade *P3rGetTintPtr(P3tParticleClass *inClass, P3tParticle *inParticle)
{
	if (inClass->definition->appearance.tint.type == P3cValueType_Variable) {
		return (IMtShade *) &inParticle->vararray[inClass->definition->appearance.tint.u.variable.offset];
	} else {
		return NULL;
	}
}

static UUcInline P3tAttractorData *P3rGetAttractorDataPtr(P3tParticleClass *inClass, P3tParticle *inParticle)
{
	if (inClass->definition->flags & P3cParticleClassFlag_HasAttractor) {
		return (P3tAttractorData *) &inParticle->vararray[inClass->optionalvar_offset[P3cParticleOptionalVar_Attractor]];
	} else {
		return NULL;
	}
}

static UUcInline AKtOctNodeIndexCache *P3rGetEnvironmentCache(P3tParticleClass *inClass, P3tParticle *inParticle)
{
	if (inClass->definition->flags & P3cParticleClassFlag_HasEnvironmentCache) {
		return (AKtOctNodeIndexCache *) &inParticle->vararray[inClass->optionalvar_offset[P3cParticleOptionalVar_EnvironmentCache]];
	} else {
		return NULL;
	}
}

#endif /* __BFW_PARTICLE3_H__ */
