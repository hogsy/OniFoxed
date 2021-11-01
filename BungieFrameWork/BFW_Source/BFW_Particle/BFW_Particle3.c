/*
	FILE:	BFW_Particle3.c
	
	AUTHOR:	Chris Butcher
	
	CREATED: Feb 03, 2000
	
	PURPOSE: routines for new particle system (iteration 3)
	
	Copyright 2000

 */

#include "bfw_math.h"
//#include <math.h>

#include "BFW.h"
#include "BFW_Particle3.h"
#include "BFW_MathLib.h"
#include "BFW_BinaryData.h"
#include "BFW_Console.h"
#include "BFW_ScriptLang.h"
#include "BFW_Image.h"
#include "BFW_Timer.h"

#include "lrar_cache.h"

// CB: argh - this is needed so that we can access the environment, collide against it
// and damage it.
#include "Oni.h"
#include "Oni_GameState.h"
#include "Oni_GameStatePrivate.h"
#include "Oni_Weapon.h"
#include "Oni_Particle3.h"
#include "Oni_ImpactEffect.h"
#include "Oni_Sound2.h"

PHtPhysicsContext*P3gPhysicsList[256];
UUtUns32 P3gPhysicsCount;

#define ENABLE_PREVPOSITION_COLLISION		0
#define PARTICLE_DEBUG_BLAST				0
#define PARTICLE_DEBUG_SOUND				0

#define PARTICLE_PROFILE_UPDATE				0
#define PARTICLE_PROFILE_DISPLAY			0

 /*
 * static constants
 */

#define P3cParallelBlastTolerance					4.0f
#define P3cBounceStopVelocity						1.0f

#define P3cMaxLensFlares							32
#define P3cMaxContrailEmitters						16
#define P3cAttractorBuf_Max							32

#define P3cAttractor_RandomDelay					10

#define P3cSelector_PreviousScoreWeight				0.9f

#define P3cBlastCenter_NormalOffset					0.5f

#define P3cAvoidWalls_CheckCollisionMinInterval		0.2f

// particles are allocated in a number of different 'size classes'.
UUtUns16 P3gParticleSizeClass[P3cMemory_ParticleSizeClasses] = {64, 128, 192, 256};


size_t P3gOptionalVarSize[P3cNumParticleOptionalVars] = {sizeof(M3tVector3D),			// velocity
														sizeof(M3tMatrix3x3),			// orientation
														sizeof(M3tPoint3D),				// offsetpos
														sizeof(M3tMatrix4x3*),			// dynamic matrix
														sizeof(P3tPrevPosition),		// previous position
														sizeof(P3tDecalData),			// decal data
														sizeof(UUtUns32),				// texture instance
														sizeof(UUtUns32),				// texture time
														sizeof(P3tOwner),				// owner pointer
														sizeof(M3tContrailData),		// contrail data
														sizeof(UUtUns32),				// lensflare frames
														sizeof(P3tAttractorData),		// attractor data
														sizeof(AKtOctNodeIndexCache)};	// environment collision cache data

const float P3gGravityAccel = 98;				// one unit is 10cm. this is 9.8 ms-2
const float P3gGravityDistSq = UUmSQR(10.0f);	// the distance at which attractors exert 1 G

#define P3gInverseNormalTableSize 10
const float P3gInverseNormalTable[P3gInverseNormalTableSize + 1] =
				{0.0000f,	// standard deviations that give cumulative frequency
				 0.1257f,	// sampled at 50, 55, 60, ..., 90, 95, 99
				 0.2533f,
				 0.3853f,
				 0.5244f,
				 0.6745f,
				 0.8416f,
				 1.0364f,
				 1.2816f,
				 1.6449f,
				 3.0902f};

const P3tString P3gEventName[P3cMaxNumEvents] = {"update",
													"pulse",
													"start",
													"stop",
													"bgfx_start",
													"bgfx_stop",
													"hit_wall",
													"hit_char",
													"lifetime",
													"explode",
													"brokenlink",
													"create",
													"die",
													"newattractor",
													"delay_start",
													"delay_stop"};

/*
 * prototypes for action procedures
 */

UUtBool P3iAction_AnimateLinear(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_AnimateAccel(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_AnimateRandom(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_AnimatePingpong(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_AnimateLoop(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_AnimateToValue(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_ColorInterpolate(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_Die(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_FadeOut(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_SetLifetime(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_MoveLine(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_MoveGravity(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_MoveResistance(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_MoveSpiral(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_MoveDrift(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_SetVelocity(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_SpiralTangent(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_KillBeyondPoint(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_EmitActivate(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_EmitDeactivate(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_EmitParticles(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_ChangeClass(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_KillLastEmitted(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_ExplodeLastEmitted(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_StickToWall(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_Bounce(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_CollisionEffect(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_ImpactEffect(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_DamageChar(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_DamageBlast(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_DamageEnvironment(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_GlassCharge(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_Explode(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_Stop(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_Show(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_Hide(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_SetTextureTick(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_RandomTextureFrame(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_SetVariable(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_RecalculateAll(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_EnableAtTime(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_DisableAtTime(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_EnableAbove(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_EnableBelow(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_EnableNow(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_DisableNow(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_RotateX(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_RotateY(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_RotateZ(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_FindAttractor(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_AttractGravity(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_AttractHoming(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_AttractSpring(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_StopLink(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_CheckLink(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_BreakLink(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_Chop(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_SuperballTrigger(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_StopIfBreakable(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_AvoidWalls(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_RandomSwirl(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_FloatAbovePlayer(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_StopIfSlow(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_AmbientSound(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_EndAmbientSound(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_ImpulseSound(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);
UUtBool P3iAction_SuperParticle(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop);

#define VAR_BLANK  {"", P3cDataType_None}

// NB: these macros rely on P3cActionMaxParameters being 8 and
// pad out some number of variables out to this value
#define VARS_8				/**/
#define VARS_7				VAR_BLANK
#define VARS_6				VAR_BLANK, VAR_BLANK
#define VARS_5				VAR_BLANK, VAR_BLANK, VAR_BLANK
#define VARS_4				VAR_BLANK, VAR_BLANK, VAR_BLANK, VAR_BLANK
#define VARS_3				VAR_BLANK, VAR_BLANK, VAR_BLANK, VAR_BLANK, VAR_BLANK
#define VARS_2				VAR_BLANK, VAR_BLANK, VAR_BLANK, VAR_BLANK, VAR_BLANK, VAR_BLANK
#define VARS_1				VAR_BLANK, VAR_BLANK, VAR_BLANK, VAR_BLANK, VAR_BLANK, VAR_BLANK, VAR_BLANK
#define VARS_0				VAR_BLANK, VAR_BLANK, VAR_BLANK, VAR_BLANK, VAR_BLANK, VAR_BLANK, VAR_BLANK, VAR_BLANK

#define DIVIDER				{P3cActionType_Invalid, 0, "-", NULL, 0, 0, {VARS_0}}
#define BLANK				{P3cActionType_Invalid, 0, "",  NULL, 0, 0, {VARS_0}}

P3tActionTemplate P3gActionInfo[P3cNumActionTypes] = {

	/*
	 *   variable animation actions
	 */

	{P3cActionType_AnimateLinear,			0,
	 "Linear Change",			P3iAction_AnimateLinear,		1,	1, {
		{"var",			P3cDataType_Float},
		{"rate",		P3cDataType_Float},
		VARS_2}},

	{P3cActionType_AnimateAccel,			0,
	 "Acceler. Change",			P3iAction_AnimateAccel,			2,	1, {
		{"var",			P3cDataType_Float},
		{"anim_vel",	P3cDataType_Float},
		{"accel",		P3cDataType_Float},
		VARS_3}},

	{P3cActionType_AnimateRandom,			0,
	 "Random Change",			P3iAction_AnimateRandom,		1,	3, {
		{"var",			P3cDataType_Float},
		{"min",			P3cDataType_Float},
		{"max",			P3cDataType_Float},
		{"rate",		P3cDataType_Float},
		VARS_4}},

	{P3cActionType_AnimatePingpong,			0,
	 "Pingpong Change",			P3iAction_AnimatePingpong,		2,	3, {
		{"var",			P3cDataType_Float},
		{"pingpong",	(P3tDataType)(P3cDataType_Enum | P3cEnumType_Pingpong)},
		{"min",			P3cDataType_Float},
		{"max",			P3cDataType_Float},
		{"rate",		P3cDataType_Float},
		VARS_5}},

	{P3cActionType_AnimateLoop,				0,
	 "Looping Change",			P3iAction_AnimateLoop,			1,	3, {
		{"var",			P3cDataType_Float},
		{"min",			P3cDataType_Float},
		{"max",			P3cDataType_Float},
		{"rate",		P3cDataType_Float},
		VARS_4}},

	{P3cActionType_AnimateToValue,			0,
	 "Change To Value",			P3iAction_AnimateToValue,		1,	2, {
		{"var",			P3cDataType_Float},
		{"rate",		P3cDataType_Float},
		{"endpoint",	P3cDataType_Float},
		VARS_3}},

	{P3cActionType_ColorInterpolate,		0,
	 "Color Blend",		P3iAction_ColorInterpolate,		1,	3, {
		{"var",			P3cDataType_Shade},
		{"color_0",		P3cDataType_Shade},
		{"color_1",		P3cDataType_Shade},
		{"blendval",	P3cDataType_Float},
		VARS_4}},

	DIVIDER,




	/*
	 *   action control
	 */

	{P3cActionType_FadeOut,					0,
	 "Fade Out",				P3iAction_FadeOut,				0,	1, {
		{"time_to_die",	P3cDataType_Float},
		VARS_1}},

	{P3cActionType_EnableAtTime,			0,
	 "Enable Action",			P3iAction_EnableAtTime,			0,	2, {
		{"action",		(P3tDataType)(P3cDataType_Enum | P3cEnumType_ActionIndex)},
		{"lifetime",	P3cDataType_Float},
		VARS_2}},

	{P3cActionType_DisableAtTime,			0,
	 "Disable Action",			P3iAction_DisableAtTime,			0,	2, {
		{"action",		(P3tDataType)(P3cDataType_Enum | P3cEnumType_ActionIndex)},
		{"lifetime",	P3cDataType_Float},
		VARS_2}},

	{P3cActionType_Die,						0,
	 "Die",					P3iAction_Die,					0,	0, {
		VARS_0}},

	{P3cActionType_SetLifetime,				P3cActionTemplate_NotIfChopped,
	 "Set Lifetime",			P3iAction_SetLifetime,			0,	1, {
		{"time",		P3cDataType_Float},
		VARS_1}},





	/*
	 *   particle emitting
	 */

	{P3cActionType_EmitActivate,			0,
	 "Enable Emitter",		P3iAction_EmitActivate,			0,	1, {
		{"emitter_num",	(P3tDataType)(P3cDataType_Enum | P3cEnumType_Emitter)},
		VARS_1}},

	{P3cActionType_EmitDeactivate,			0,
	 "Disable Emitter",		P3iAction_EmitDeactivate,		0,	1, {
		{"emitter_num",	(P3tDataType)(P3cDataType_Enum | P3cEnumType_Emitter)},
		VARS_1}},

	{P3cActionType_EmitParticles,			P3cActionTemplate_NotIfChopped,
	 "Emit Particles",		P3iAction_EmitParticles,		0,	2, {
		{"emitter_num",	(P3tDataType)(P3cDataType_Enum | P3cEnumType_Emitter)},
		{"particles",	P3cDataType_Float},
		VARS_2}},

	{P3cActionType_ChangeClass,				P3cActionTemplate_NotIfChopped,
	 "Change Class",		P3iAction_ChangeClass,			0,	1, {
		{"emitter_num",	(P3tDataType)(P3cDataType_Enum | P3cEnumType_Emitter)},
		VARS_1}},

	{P3cActionType_KillLastEmitted,			0,
	 "KillLastEmitted",	P3iAction_KillLastEmitted,		0,	1, {
		{"emitter_num",	(P3tDataType)(P3cDataType_Enum | P3cEnumType_Emitter)},
		VARS_1}},

	{P3cActionType_ExplodeLastEmitted,			0,
	 "ExplodeLastEmit",	P3iAction_ExplodeLastEmitted,0,	1, {
		{"emitter_num",	(P3tDataType)(P3cDataType_Enum | P3cEnumType_Emitter)},
		VARS_1}},

	DIVIDER,



	/*
	 *   sound control - not yet known, but reserve actions anyway
	 */

	{P3cActionType_SoundStart,				0,
	 "Start Amb Sound",			P3iAction_AmbientSound,			0,	1, {
		{"sound",		P3cDataType_Sound_Ambient},
		VARS_1}},

	{P3cActionType_SoundEnd,				0,
	 "End Amb Sound",			P3iAction_EndAmbientSound,		0,	0, {
		VARS_0}},

	{P3cActionType_SoundVolume,				0,
	 "XX Sound Volume",			NULL,							0,	0, {
		VARS_0}},

	{P3cActionType_ImpulseSound,			0,
	 "Impulse Sound",			P3iAction_ImpulseSound,			0,	1, {
		{"sound",		P3cDataType_Sound_Impulse},
		VARS_1}},

	BLANK,
	DIVIDER,




	/*
	 *   damage
	 */

	 // damages the character most recently hit by collidecharacter
	{P3cActionType_DamageHitChar,			P3cActionTemplate_NotIfChopped,
	 "Damage Char",			P3iAction_DamageChar,			0,	6, {
		{"damage",		P3cDataType_Float},
		{"stun_damage",	P3cDataType_Float},
		{"knockback",	P3cDataType_Float},
		{"damage type",	(P3tDataType)(P3cDataType_Enum | P3cEnumType_DamageType)},
		{"self_immune",	(P3tDataType)(P3cDataType_Enum | P3cEnumType_Bool)},
		{"can_hit_mult",(P3tDataType)(P3cDataType_Enum | P3cEnumType_Bool)},
		VARS_6}},

	{P3cActionType_DamageBlast,				P3cActionTemplate_NotIfChopped,
	 "Blast Damage",			P3iAction_DamageBlast,			0,	8, {
		{"damage",		P3cDataType_Float},
		{"stun_damage",	P3cDataType_Float},
		{"knockback",	P3cDataType_Float},
		{"radius",		P3cDataType_Float},
		{"falloff",		(P3tDataType)(P3cDataType_Enum | P3cEnumType_Falloff)},
		{"damage type",	(P3tDataType)(P3cDataType_Enum | P3cEnumType_DamageType)},
		{"self_immune",	(P3tDataType)(P3cDataType_Enum | P3cEnumType_Bool)},
		{"damage_environ",(P3tDataType)(P3cDataType_Enum | P3cEnumType_Bool)},
		VARS_8}},

	{P3cActionType_Explode,					P3cActionTemplate_NotIfChopped,
	 "Explode",				P3iAction_Explode,				0,	0, {
		VARS_0}},

	{P3cActionType_DamageEnvironment,		P3cActionTemplate_NotIfChopped,
	 "Damage Environ.",		P3iAction_DamageEnvironment,	0,	1, {
		{"damage",		P3cDataType_Float},
		VARS_1}},

	{P3cActionType_GlassCharge,				P3cActionTemplate_NotIfChopped,
	 "Glass Charge",		P3iAction_GlassCharge,			0,	2, {
		{"blast-vel",	P3cDataType_Float},
		{"radius",		P3cDataType_Float},
		VARS_2}},

	{P3cActionType_Stop,					P3cActionTemplate_NotIfChopped,
	 "Stop",			P3iAction_Stop,				0,	0, {
		VARS_0}},

	DIVIDER,



	/*
	 *   rotation relative to world frame - use only on particles without
	 *     orientation
	 */

	{P3cActionType_RotateX,					0,
	 "Rotate X",			P3iAction_RotateX,				0,	3, {
		{"space",		(P3tDataType)(P3cDataType_Enum | P3cEnumType_CoordFrame)},
		{"rate",		P3cDataType_Float},
		{"rotate velocity",(P3tDataType)(P3cDataType_Enum | P3cEnumType_Bool)},
		VARS_3}},

	{P3cActionType_RotateY,					0,
	 "Rotate Y",			P3iAction_RotateY,				0,	3, {
		{"space",		(P3tDataType)(P3cDataType_Enum | P3cEnumType_CoordFrame)},
		{"rate",		P3cDataType_Float},
		{"rotate velocity",(P3tDataType)(P3cDataType_Enum | P3cEnumType_Bool)},
		VARS_3}},

	{P3cActionType_RotateZ,					0,
	 "Rotate Z",			P3iAction_RotateZ,				0,	3, {
		{"space",		(P3tDataType)(P3cDataType_Enum | P3cEnumType_CoordFrame)},
		{"rate",		P3cDataType_Float},
		{"rotate velocity",(P3tDataType)(P3cDataType_Enum | P3cEnumType_Bool)},
		VARS_3}},

	DIVIDER,




	/*
	 *   attractor finding and homing functions
	 */

	{P3cActionType_FindAttractor,			0,
	 "Find Attractor",	P3iAction_FindAttractor,			0,	1, {
		{"delaytime",	P3cDataType_Float},
		VARS_1}},

	{P3cActionType_AttractGravity,			0,
	 "Attract Gravity",	P3iAction_AttractGravity,			0,	3, {
		{"gravity",		P3cDataType_Float},
		{"max g",		P3cDataType_Float},
		{"horiz only",	(P3tDataType) (P3cDataType_Enum | P3cEnumType_Bool)},
		VARS_3}},

	{P3cActionType_AttractHoming,			0,
	 "Attract Homing",	P3iAction_AttractHoming,			0,	3, {
		{"turn speed",	P3cDataType_Float},
		{"predict pos",(P3tDataType)(P3cDataType_Enum | P3cEnumType_Bool)},
		{"horiz only",	(P3tDataType) (P3cDataType_Enum | P3cEnumType_Bool)},
		VARS_3}},

	{P3cActionType_AttractSpring,			0,
	 "Attract Spring",	P3iAction_AttractSpring,			0,	3, {
		{"accel rate",	P3cDataType_Float},
		{"max accel",	P3cDataType_Float},
		{"desired dist",P3cDataType_Float},
		VARS_3}},

	// blank spaces for future attraction functions
	BLANK,
	BLANK,
	BLANK,
	BLANK,
	BLANK,

	DIVIDER,

	/*
	 *   movement routines. all use position, velocity.
	 */

	{P3cActionType_MoveLine,				P3cActionTemplate_NotIfStopped,
	 "Apply Velocity",		P3iAction_MoveLine,				0,	0, {
		VARS_0}},

	{P3cActionType_MoveGravity,				P3cActionTemplate_NotIfStopped | P3cActionTemplate_NotIfCollision,
	 "Apply Gravity",			P3iAction_MoveGravity,			0,	1, {
		{"fraction",		P3cDataType_Float},
		VARS_1}},

	{P3cActionType_MoveSpiral,				P3cActionTemplate_NotIfStopped | P3cActionTemplate_NotIfCollision,
	 "Spiral",			P3iAction_MoveSpiral,			1,	2, {
		{"theta",			P3cDataType_Float},
		{"radius",			P3cDataType_Float},
		{"rotate_speed",	P3cDataType_Float},
		VARS_3}},

	{P3cActionType_MoveResistance,			P3cActionTemplate_NotIfStopped | P3cActionTemplate_NotIfCollision,
	 "Air Resistance",		P3iAction_MoveResistance,		0,	2, {
		{"resistance",		P3cDataType_Float},
		{"minimum_vel",		P3cDataType_Float},
		VARS_2}},

	{P3cActionType_MoveDrift,				P3cActionTemplate_NotIfStopped | P3cActionTemplate_NotIfCollision,
	 "Drift",			P3iAction_MoveDrift,				0,	7, {
		{"acceleration",	P3cDataType_Float},
		{"max_speed",		P3cDataType_Float},
		{"sideways_decay",	P3cDataType_Float},
		{"dir_x",			P3cDataType_Float},
		{"dir_y",			P3cDataType_Float},
		{"dir_z",			P3cDataType_Float},
		{"space",			(P3tDataType)(P3cDataType_Enum | P3cEnumType_CoordFrame)},
		VARS_7}},

	{P3cActionType_SetVelocity,				P3cActionTemplate_NotIfStopped | P3cActionTemplate_NotIfCollision,
	 "Set Speed",		P3iAction_SetVelocity,				0,	3, {
		{"speed",			P3cDataType_Float},
		{"space",			(P3tDataType)(P3cDataType_Enum | P3cEnumType_CoordFrame)},
		{"no_sideways",		(P3tDataType)(P3cDataType_Enum | P3cEnumType_Bool)},
		VARS_3}},

	{P3cActionType_SpiralTangent,			P3cActionTemplate_NotIfStopped | P3cActionTemplate_NotIfCollision,
	 "Spiral Tangent",	P3iAction_SpiralTangent,			0,	3, {
		{"theta",			P3cDataType_Float},
		{"radius",			P3cDataType_Float},
		{"rotate_speed",	P3cDataType_Float},
		VARS_3}},

	{P3cActionType_KillBeyondPoint,			P3cActionTemplate_NotIfStopped,
	 "KillBeyondPoint",P3iAction_KillBeyondPoint,			0,	2, {
		{"direction",		(P3tDataType)(P3cDataType_Enum | P3cEnumType_Axis)},
		{"value",			P3cDataType_Float},
		VARS_2}},

	/*
	 *   collision actions. must only be used after a collision is detected.
	 */

	 // actually, collisioneffect can be used at any time - if no collision was detected this frame
	 // it just uses current position
	{P3cActionType_CollisionEffect,			P3cActionTemplate_NotIfChopped,
	 "Create Effect",		P3iAction_CollisionEffect,		0,	4, {
		{"classname",		P3cDataType_String},
		{"wall_offset",		P3cDataType_Float},
		{"orientation",		(P3tDataType)(P3cDataType_Enum | P3cEnumType_CollisionOrientation)},
		{"attach",			(P3tDataType)(P3cDataType_Enum | P3cEnumType_Bool)},
		VARS_4}},

	{P3cActionType_StickToWall,				P3cActionTemplate_NotIfChopped,
	 "Stick To Wall",		P3iAction_StickToWall,			0,	0, {
		VARS_0}},

	// uses the result of the last successful collide_wall or collide_character
	{P3cActionType_Bounce,					P3cActionTemplate_NotIfChopped,
	 "Bounce",				P3iAction_Bounce,				0,	2, {
		{"elastic_direct",	P3cDataType_Float},
		{"elastic_glancing",P3cDataType_Float},
		VARS_2}},

	// uses the result of the last successful collide_wall
	{P3cActionType_CreateDecal,				P3cActionTemplate_NotIfChopped,
	 "XXX Create Decal",		NULL,			0,	0, {
		VARS_0}},

	// chops a particle so that it doesn't poke through walls
	{P3cActionType_Chop,					P3cActionTemplate_NotIfChopped,
	 "Chop Particle",		P3iAction_Chop,					0,	0, {
		VARS_0}},

	 // impacteffect can be used at any time - if no collision was detected this frame
	 // it just uses current position
	{P3cActionType_ImpactEffect,			P3cActionTemplate_NotIfChopped,
	 "Impact Effect",		P3iAction_ImpactEffect,			0,	2, {
		{"impact_type",		P3cDataType_String},
		{"impact modifier",	P3cDataType_Enum | P3cEnumType_ImpactModifier},
		VARS_2}},

	DIVIDER,

	/*
	 *   visibility control
	 */

	{P3cActionType_Show,					0,
	 "Unhide Particle",		P3iAction_Show,					0,	0, {
		VARS_0}},

	{P3cActionType_Hide,					0,
	 "Hide Particle",		P3iAction_Hide,					0,	0, {
		VARS_0}},

	{P3cActionType_SetTextureTick,			0,
	 "Set TextureTick",		P3iAction_SetTextureTick,		0,	1, {
		{"tick",	P3cDataType_Float},
		VARS_1}},

	{P3cActionType_RandomTextureFrame,		0,
	 "Random TexFrame",		P3iAction_RandomTextureFrame,	0,	0, {
		VARS_0}},
	BLANK,
	BLANK,
	BLANK,
	DIVIDER,


	/*
	 *   action and variable control - triggers
	 */

	{P3cActionType_SetVariable,				0,
	 "Set Variable",		P3iAction_SetVariable,			1,	1, {
		{"var",		P3cDataType_Float},
		{"value",	P3cDataType_Float},
		VARS_2}},
	{P3cActionType_RecalculateAll,			0,
	 "Recalculate All",		P3iAction_RecalculateAll,		0,	0, {
		VARS_0}},
	{P3cActionType_EnableAbove,				0,
	 "Enable Above",	P3iAction_EnableAbove,		0,	3, {
		{"action",		(P3tDataType)(P3cDataType_Enum | P3cEnumType_ActionIndex)},
		{"var",			P3cDataType_Float},
		{"threshold",	P3cDataType_Float},
		VARS_3}},
	{P3cActionType_EnableBelow,				0,
	 "Enable Below",	P3iAction_EnableBelow,		0,	3, {
		{"action",		(P3tDataType)(P3cDataType_Enum | P3cEnumType_ActionIndex)},
		{"var",			P3cDataType_Float},
		{"threshold",	P3cDataType_Float},
		VARS_3}},

	{P3cActionType_EnableNow,				0,
	 "Enable Now",			P3iAction_EnableNow,			0,	1, {
		{"action",		(P3tDataType)(P3cDataType_Enum | P3cEnumType_ActionIndex)},
		VARS_1}},

	{P3cActionType_DisableNow,				0,
	 "Disable Now",			P3iAction_DisableNow,			0,	1, {
		{"action",		(P3tDataType)(P3cDataType_Enum | P3cEnumType_ActionIndex)},
		VARS_1}},

	DIVIDER,

	/*
	 *   special-case actions and hacks
	 */

	{P3cActionType_SuperballTrigger,			0,
	 "SuperB Trigger",	P3iAction_SuperballTrigger,			0,	2, {
		{"emitter_num",	(P3tDataType)(P3cDataType_Enum | P3cEnumType_Emitter)},
		{"fuse_time",	P3cDataType_Float},
		VARS_2}},

	{P3cActionType_StopIfBreakable,				0,
	 "BreakableStop",	P3iAction_StopIfBreakable,			0,	0, {
		VARS_0}},

	{P3cActionType_AvoidWalls,					0,
	 "Avoid Walls",		P3iAction_AvoidWalls,				5,	3, {
		{"axis_x",		P3cDataType_Float},
		{"axis_y",		P3cDataType_Float},
		{"axis_z",		P3cDataType_Float},
		{"cur_angle",	P3cDataType_Float},
		{"t_until_check",P3cDataType_Float},
		{"sense dist",	P3cDataType_Float},
		{"turning speed",P3cDataType_Float},
		{"turning decay",P3cDataType_Float},
		VARS_8}},

	{P3cActionType_RandomSwirl,					0,
	 "Random Swirl",	P3iAction_RandomSwirl,				1,	3, {
		{"swirl angle",	P3cDataType_Float},
		{"swirl baserate",P3cDataType_Float},
		{"swirl deltarate",P3cDataType_Float},
		{"swirl speed",	P3cDataType_Float},
		VARS_4}},
		
	{P3cActionType_FloatAbovePlayer,			0,
	 "Follow Player",P3iAction_FloatAbovePlayer,		0,	1, {
		{"height",	P3cDataType_Float},
		VARS_1}},
		
	{P3cActionType_StopIfSlow,					0,
	 "Stop BelowSpeed",P3iAction_StopIfSlow,			0,	1, {
		{"speed",	P3cDataType_Float},
		VARS_1}},
		
	{P3cActionType_SuperParticle,				0,
	 "Super Particle",	P3iAction_SuperParticle,		1,	4, {
		{"variable",	P3cDataType_Float},
		{"base value",P3cDataType_Float},
		{"delta value",P3cDataType_Float},
		{"min value",P3cDataType_Float},
		{"max value",P3cDataType_Float},
		VARS_5}},
		
	/*
	 *   link control
	 */

	{P3cActionType_StopLink,				0,
	 "Stop Our Link",	P3iAction_StopLink,				0,	0, {
		VARS_0}},

	{P3cActionType_CheckLink,				0,
	 "Check Link",		P3iAction_CheckLink,				0,	0, {
		VARS_0}},

	{P3cActionType_BreakLink,				0,
	 "Break Links To",	P3iAction_BreakLink,				0,	0, {
		VARS_0}},

	BLANK,
	BLANK,
	BLANK
};

#undef DIVIDER
#undef VARS_0
#undef VARS_1
#undef VARS_2
#undef VARS_3
#undef VARS_4
#undef VARS_5
#undef VARS_6
#undef VARS_7
#undef VARS_8
#undef VAR_BLANK

// information stored about possible enumerated types
P3tEnumTypeInfo P3gEnumTypeInfo[P3cEnumType_Max >> P3cEnumShift] = {
	{P3cEnumType_Pingpong,		"pingpong state",		UUcFalse,	P3cEnumPingpong_Up,		P3cEnumPingpong_Max,	P3cEnumPingpong_Up},
	{P3cEnumType_ActionIndex,	"action index",			UUcFalse,	0,						256,					0},
	{P3cEnumType_Emitter,		"emitter",				UUcFalse,	0,						P3cMaxEmitters,			(UUtUns32) -1},
	{P3cEnumType_Falloff,		"blast falloff",		UUcFalse,	P3cEnumFalloff_None,	P3cEnumFalloff_Max,		P3cEnumFalloff_Linear},
	{P3cEnumType_CoordFrame,	"coord frame",			UUcFalse,	P3cEnumCoordFrame_Local,P3cEnumCoordFrame_Max,	P3cEnumCoordFrame_World},
	{P3cEnumType_CollisionOrientation,"collision orient",UUcFalse,	P3cEnumCollisionOrientation_Unchanged,P3cEnumCollisionOrientation_Max,	P3cEnumCollisionOrientation_Normal},
	{P3cEnumType_Bool,			"boolean",				UUcFalse,	P3cEnumBool_False,		P3cEnumBool_Max,		P3cEnumBool_True},
	{P3cEnumType_AmbientSoundID,"ambient sound",		UUcTrue,	0,						0,						SScInvalidID},
	{P3cEnumType_ImpulseSoundID,"impulse sound",		UUcTrue,	0,						0,						SScInvalidID},
	{P3cEnumType_ImpactModifier,"impact modifier",		UUcFalse,	ONcIEModType_Any,		ONcIEModType_NumTypes,	ONcIEModType_Any},
	{P3cEnumType_DamageType,	"damage type",			UUcFalse,	P3cEnumDamageType_First,P3cEnumDamageType_Last,	P3cEnumDamageType_Normal},
	{P3cEnumType_Axis,			"axis",					UUcFalse,	P3cEnumAxis_PosX,		P3cEnumAxis_NegZ,		P3cEnumAxis_PosX}
};


// table of possible emitter properties and their required values
#define NO_PARAMS {{"", P3cDataType_None},		\
					{"", P3cDataType_None},		\
					{"", P3cDataType_None},		\
					{"", P3cDataType_None}}

#define END		{(UUtUns32) -1, "", 0, NO_PARAMS}


P3tEmitterSettingDescription P3gEmitterRateDesc[] = {{P3cEmitterRate_Continuous, "continuous",	1, 
														{{"emit interval", P3cDataType_Float},		
														{"", P3cDataType_None},
														{"", P3cDataType_None},
														{"", P3cDataType_None}}},
													{P3cEmitterRate_Random,		 "random",		2,
														{{"min emitinterval", P3cDataType_Float},
														{"max emitinterval", P3cDataType_Float},
														{"", P3cDataType_None},
														{"", P3cDataType_None}}},
													{P3cEmitterRate_Instant,	 "instant",		0, NO_PARAMS},
													{P3cEmitterRate_Distance,	"distance",		1,
														{{"distance", P3cDataType_Float},
														{"", P3cDataType_None},
														{"", P3cDataType_None},
														{"", P3cDataType_None}}},
													{P3cEmitterRate_Attractor,	"attractor",	2,
														{{"recharge time", P3cDataType_Float},
														{"check interval", P3cDataType_Float},
														{"", P3cDataType_None},
														{"", P3cDataType_None}}},
													END};

P3tEmitterSettingDescription P3gEmitterPositionDesc[] = {{P3cEmitterPosition_Point,	"point",		0,  NO_PARAMS},
														{P3cEmitterPosition_Line,	"line",			1,
															{{"radius", P3cDataType_Float},
															{"", P3cDataType_None},
															{"", P3cDataType_None},
															{"", P3cDataType_None}}},
														{P3cEmitterPosition_Circle, "circle",		2, 
															{{"inner radius", P3cDataType_Float},		
															{"outer radius", P3cDataType_Float},
															{"", P3cDataType_None},
															{"", P3cDataType_None}}},
														{P3cEmitterPosition_Sphere,	"sphere",		2,
															{{"inner radius", P3cDataType_Float},		
															{"outer radius", P3cDataType_Float},
															{"", P3cDataType_None},
															{"", P3cDataType_None}}},
														{P3cEmitterPosition_Offset,	"offset",		3,
															{{"x", P3cDataType_Float},		
															{"y", P3cDataType_Float},
															{"z", P3cDataType_Float},
															{"", P3cDataType_None}}},
														{P3cEmitterPosition_Cylinder,"cylinder",	3,
															{{"height", P3cDataType_Float},		
															{"inner radius", P3cDataType_Float},
															{"outer radius", P3cDataType_Float},
															{"", P3cDataType_None}}},
														{P3cEmitterPosition_BodySurface,"body-surface",	1,
															{{"offsetradius", P3cDataType_Float},		
															{"", P3cDataType_None},
															{"", P3cDataType_None},
															{"", P3cDataType_None}}},
														{P3cEmitterPosition_BodyBones,"body-bones",	1,
															{{"offsetradius", P3cDataType_Float},		
															{"", P3cDataType_None},
															{"", P3cDataType_None},
															{"", P3cDataType_None}}},
														END};

P3tEmitterSettingDescription P3gEmitterDirectionDesc[] = {{P3cEmitterDirection_Linear,	"straight",	0,  NO_PARAMS},
														{P3cEmitterDirection_Random,	"random",	0, NO_PARAMS},
														{P3cEmitterDirection_Conical,	"cone",		2, 
															{{"angle", P3cDataType_Float},		
															{"center bias", P3cDataType_Float},
															{"", P3cDataType_None},
															{"", P3cDataType_None}}},
														{P3cEmitterDirection_Ring,		"ring",		2, 
															{{"angle", P3cDataType_Float},		
															{"offset", P3cDataType_Float},
															{"", P3cDataType_None},
															{"", P3cDataType_None}}},
														{P3cEmitterDirection_OffsetDir,	"offset",		3,
															{{"x", P3cDataType_Float},		
															{"y", P3cDataType_Float},
															{"z", P3cDataType_Float},
															{"", P3cDataType_None}}},
														{P3cEmitterDirection_Inaccurate,"inaccurate",	3,
															{{"base angle", P3cDataType_Float},		
															{"inaccuracy", P3cDataType_Float},
															{"center bias", P3cDataType_Float},
															{"", P3cDataType_None}}},
														{P3cEmitterDirection_Attractor,	"attractor", 0,	NO_PARAMS},
														END};

P3tEmitterSettingDescription P3gEmitterSpeedDesc[] = {{P3cEmitterSpeed_Uniform,		"uniform",	1, 
														{{"speed", P3cDataType_Float},		
														{"", P3cDataType_None},
														{"", P3cDataType_None},
														{"", P3cDataType_None}}},
													{P3cEmitterSpeed_Stratified,	"stratified",	2,
														{{"speed 1", P3cDataType_Float},
														{"speed 2", P3cDataType_Float},
														{"", P3cDataType_None},
														{"", P3cDataType_None}}},
													END};

P3tEmitterSettingDescription P3gEmitterOrientationDesc[]
	 = {{P3cEmitterOrientation_ParentPosX,		"parent +X",		0,	NO_PARAMS},
		{P3cEmitterOrientation_ParentNegX,		"parent -X",		0,	NO_PARAMS},
		{P3cEmitterOrientation_ParentPosY,		"parent +Y",		0,	NO_PARAMS},
		{P3cEmitterOrientation_ParentNegY,		"parent -Y",		0,	NO_PARAMS},
		{P3cEmitterOrientation_ParentPosZ,		"parent +Z",		0,	NO_PARAMS},
		{P3cEmitterOrientation_ParentNegZ,		"parent -Z",		0,	NO_PARAMS},
		{P3cEmitterOrientation_WorldPosX,		"world +X",			0,	NO_PARAMS},
		{P3cEmitterOrientation_WorldNegX,		"world -X",			0,	NO_PARAMS},
		{P3cEmitterOrientation_WorldPosY,		"world +Y",			0,	NO_PARAMS},
		{P3cEmitterOrientation_WorldNegY,		"world -Y",			0,	NO_PARAMS},
		{P3cEmitterOrientation_WorldPosZ,		"world +Z",			0,	NO_PARAMS},
		{P3cEmitterOrientation_WorldNegZ,		"world -Z",			0,	NO_PARAMS},
		{P3cEmitterOrientation_Velocity,		"velocity",			0,	NO_PARAMS},
		{P3cEmitterOrientation_ReverseVelocity,	"reverse-velocity",	0,	NO_PARAMS},
		{P3cEmitterOrientation_TowardsEmitter,	"towards-emitter",	0,	NO_PARAMS},
		{P3cEmitterOrientation_AwayFromEmitter,	"awayfrom-emitter",	0,	NO_PARAMS},
		END};

#undef NO_PARAMS
#undef END

// binary data methods
BDtMethods		P3gBinaryMethods = {P3rLoadParticleDefinition};

/*
 * attractor functions
 */

// iterator functions
UUtBool P3iAttractorIterator_None(P3tParticleClass *inClass, P3tParticle *inParticle, P3tAttractor *inAttractor,
												M3tPoint3D *outFoundPoint, float *ioDistance, UUtUns32 *ioIterator);
UUtBool P3iAttractorIterator_Link(P3tParticleClass *inClass, P3tParticle *inParticle, P3tAttractor *inAttractor,
												M3tPoint3D *outFoundPoint, float *ioDistance, UUtUns32 *ioIterator);
UUtBool P3iAttractorIterator_Class(P3tParticleClass *inClass, P3tParticle *inParticle, P3tAttractor *inAttractor,
												M3tPoint3D *outFoundPoint, float *ioDistance, UUtUns32 *ioIterator);
UUtBool P3iAttractorIterator_Tag(P3tParticleClass *inClass, P3tParticle *inParticle, P3tAttractor *inAttractor,
												M3tPoint3D *outFoundPoint, float *ioDistance, UUtUns32 *ioIterator);
UUtBool P3iAttractorIterator_Character(P3tParticleClass *inClass, P3tParticle *inParticle, P3tAttractor *inAttractor,
												M3tPoint3D *outFoundPoint, float *ioDistance, UUtUns32 *ioIterator);

// selector functions
UUtBool P3iAttractorSelector_Distance(P3tParticleClass *inClass, P3tParticle *inParticle, P3tAttractor *inAttractor,
												M3tPoint3D *inFoundPoint, float inDistance, UUtUns32 inFoundID, UUtUns32 *ioChoice);
UUtBool P3iAttractorSelector_Conical(P3tParticleClass *inClass, P3tParticle *inParticle, P3tAttractor *inAttractor,
												M3tPoint3D *inFoundPoint, float inDistance, UUtUns32 inFoundID, UUtUns32 *ioChoice);

// constant tables
P3tAttractorIteratorSpec P3gAttractorIteratorTable[P3cAttractorIterator_Max]
	= {	{"none",				NULL},
		{"link",				P3iAttractorIterator_Link},
		{"class",				P3iAttractorIterator_Class},
		{"tag",					P3iAttractorIterator_Tag},
		{"characters",			P3iAttractorIterator_Character},
		{"hostiles",			P3iAttractorIterator_Character},
		{"emitted towards",		NULL},
		{"parent's attractor",	NULL},
		{"all characters",		P3iAttractorIterator_Character}};

P3tAttractorSelectorSpec P3gAttractorSelectorTable[P3cAttractorSelector_Max]
	= {	{"distance",	P3iAttractorSelector_Distance},
		{"angle",		P3iAttractorSelector_Conical}};

#if PARTICLE_PERF_ENABLE
/*
 * performance masks
 */

const char *P3cPerfEventName[P3cPerfEvent_Max] =
	{"num", "upd", "act", "var", "ept", "esp", "fac", "sph", "chr", "amb", "imp"};

P3tPerfMask P3cPerfMasks[] = {
	{"update",		((1 << P3cPerfEvent_Update) | (1 << P3cPerfEvent_Action) | (1 << P3cPerfEvent_VarEval))},
	{"collision",	((1 << P3cPerfEvent_EnvPointCol) | (1 << P3cPerfEvent_EnvSphereCol))},
	{"physics",		((1 << P3cPerfEvent_PhyFaceCol) | (1 << P3cPerfEvent_PhySphereCol) | (1 << P3cPerfEvent_PhyCharCol))},
	{"sound",		((1 << P3cPerfEvent_SndAmbient) | (1 << P3cPerfEvent_SndImpulse))},
	{NULL, 0}};
#endif

/*
 * internal prototypes
 */

static void P3iCheckActionListIndices(P3tParticleClass *inClass);
static UUtError P3iUpdateVersion(const char *inIdentifier, P3tParticleDefinition **ioDefinition,
						 P3tMemoryBlock *ioMemory, UUtBool inSwapBytes, UUtUns32 inSize, UUtUns16 *outOldVersion);
static UUtBool P3iAmbientSound_Update(P3tParticleClass *inClass, P3tParticle *inParticle, const SStAmbient *inAmbientSound);
UUtBool P3iCalculateValue(P3tParticle *inParticle, P3tValue *inValueDef,
						   void *outStore);

static void P3iActiveClass_Add(P3tParticleClass *inClass, UUtBool inAddAtEnd, UUtBool inWasDeferred);
static void P3iActiveClass_Remove(P3tParticleClass *inClass);
static void P3iActiveList_Lock(void);
static void P3iActiveList_Unlock(void);

/*
 * globals
 */

// table of all known particle classes
UUtUns32				P3gTotalParticles, P3gTotalMemoryUsage;
UUtUns16				P3gNumClasses, P3gOverflowedClasses, P3gAllocatedClassesIndex;
P3tParticleClass		P3gClassTable[P3cMaxParticleClasses];				// sorted by class type (essential / decorative)
P3tParticleClass		**P3gClassLookupTable;								// sorted alphabetically
UUtBool					P3gDirty;
UUtBool					P3gUnloading;

// memory manager
P3tParticleMemory		P3gMemory;

// flag newly created particles as temporary
UUtBool					P3gTemporaryCreation, P3gTemporaryParticles;

// quality settings
P3tQualitySettings		P3gQuality;

// global tint for newly created particles
UUtBool					P3gGlobalTintEnable;
IMtShade				P3gGlobalTint;

// collision globals
P3tCollisionGlobals		P3gCurrentCollision;
P3tCollisionGlobals		P3gPreviousCollision;
UUtBool					P3gCurrentStep_EndPointValid;
M3tPoint3D				P3gCurrentStep_EndPoint;

UUtUns16				P3gNumColliders;
PHtCollider				P3gColliders[AKcMaxNumCollisions];

// active list of particle classes
UUtUns16				P3gFirstActiveClass, P3gLastActiveClass;
UUtUns16				P3gFirstDeferredActiveClass;
UUtBool					P3gActiveList_Locked;
UUtBool					P3gActiveList_MustRemoveUponUnlock;

UUtBool					P3gRemoveDangerousProjectiles;

// debugging flags
UUtBool P3gDrawEnvCollisions = UUcFalse;
UUtBool P3gEverythingBreakable = UUcFalse;
UUtBool	P3gFurnitureBreakable = UUcFalse;

// temporary storage used for display
UUtBool P3gSkyVisible;
UUtUns32 P3gNumLensFlares;
P3tParticleClass *P3gLensFlares[P3cMaxLensFlares];
P3tParticleClass *P3gContrailEmitters[P3cMaxContrailEmitters];

// last update time - this is basically a hack to supply an approximation of
// the current time to places that only need an approximation. specificially,
// the FloatTimeBased value type.
float P3gApproxCurrentTime = 0.0f;

// last draw tick and decorative update tick
UUtUns32 P3gLastDrawTick, P3gLastDecorativeTick;

// global buffer that stores nearby attractors
UUtBool P3gAttractorBuf_Valid;
UUtUns32 P3gAttractorBuf_NumAttractors;
UUtUns32 P3gAttractorBuf_Storage[P3cAttractorBuf_Max];

// callbacks to client code
P3tParticleCallback P3gParticleCreationCallback;
P3tParticleCallback P3gParticleDeletionCallback;
P3tParticleCallback P3gParticleUpdateCallback;
P3tQualityCallback P3gQualityCallback;

// global index for traversing particle classes
UUtUns16 P3gNextTraverseIndex;

// TEMPORARY DEBUGGING - collision debugging display variables
#define P3cDebugCollisionMaxPoints		(16 * 1024)
UUtBool P3gDebugCollisionEnable;
M3tPoint3D *P3gDebugCollisionPoints;
UUtUns32 P3gDebugCollision_UsedPoints, P3gDebugCollision_NextPoint;

#if PARTICLE_PERF_ENABLE
// performance variables
UUtUns32 P3gPerfIndex, P3gPerfTime, P3gPerfMask;
const UUtUns32 P3cDefaultPerfMask = ((1 << P3cPerfEvent_Particle) | (1 << P3cPerfEvent_EnvPointCol) | (1 << P3cPerfEvent_EnvSphereCol) |
									(1 << P3cPerfEvent_PhyCharCol) | (1 << P3cPerfEvent_SndAmbient) | (1 << P3cPerfEvent_SndImpulse));

UUtBool P3gPerfDisplay;
COtStatusLine P3gPerfDisplayArea[P3cPerfDisplayLines];

UUtBool P3gPerfLockClasses;
UUtUns32 P3gPerfDisplayNumClasses;
P3tParticleClass *P3gPerfDisplayClasses[P3cPerfDisplayClasses];
#endif

#if PERFORMANCE_TIMER
UUtPerformanceTimer *P3g_ParticleUpdate = NULL;
UUtPerformanceTimer *P3g_ParticleSound = NULL;
UUtPerformanceTimer *P3g_ParticleCollision = NULL;
#endif


/*
 * routines
 */

static UUtBool P3rPointVisible(M3tPoint3D *inPoint, float inTolerance)
{
	UUtBool lensflare_visible = AKrEnvironment_PointIsVisible(inPoint);

	if (lensflare_visible) {
		lensflare_visible = M3rPointVisible(inPoint, inTolerance);
	}

	return lensflare_visible;
}


// get a random unit vector in 3D (used for random directions)
// this is slow-ish, but it's important that we get an even
// distribution of directions across a sphere, unless we want our particle
// effects to look obviously biased.
void P3rGetRandomUnitVector3D(M3tVector3D *outVector)
{
	float d_sq;

	do {
		outVector->x = P3mSignedRandom();
		outVector->y = P3mSignedRandom();
		outVector->z = P3mSignedRandom();

		d_sq = UUmSQR(outVector->x) + UUmSQR(outVector->y) + UUmSQR(outVector->z);
	} while (d_sq > 1.0f);

	MUmVector_Scale(*outVector, 1.0f / MUrSqrt(d_sq));
}

// set up the array pointers for a particle class
static void P3iCalculateArrayPointers(P3tParticleDefinition *inClassPtr)
{
	inClassPtr->variable	= (P3tVariableInfo *)	(((UUtUns8 *) inClassPtr) + sizeof(P3tParticleDefinition));
	inClassPtr->action		= (P3tActionInstance *)	(((UUtUns8 *) inClassPtr->variable)
		                              + sizeof(P3tVariableInfo) * inClassPtr->num_variables);
	inClassPtr->emitter		= (P3tEmitter *)		(((UUtUns8 *) inClassPtr->action)
		                              + sizeof(P3tActionInstance) * inClassPtr->num_actions);
}

static void P3iLRAR_NewProc(void *inUserData, short inBlockIndex);
static void P3iLRAR_PurgeProc(void *inUserData);

UUtError P3rInitialize(void)
{
	UUtUns16			itr;

#if PERFORMANCE_TIMER
	if (NULL == P3g_ParticleUpdate) {
		P3g_ParticleUpdate = UUrPerformanceTimer_New("P3_Update");
	}
	if (NULL == P3g_ParticleSound) {
		P3g_ParticleSound = UUrPerformanceTimer_New("P3_Sound");
	}
	if (NULL == P3g_ParticleCollision) {
		P3g_ParticleCollision = UUrPerformanceTimer_New("P3_Collision");
	}
#endif

	// register our type of binary data
	BDrRegisterClass(P3cBinaryDataClass, &P3gBinaryMethods);

	// no particle classes loaded
	P3gNumClasses = 0;
	P3gOverflowedClasses = 0;
	P3gDirty = UUcFalse;
	P3gUnloading = UUcFalse;
	P3gClassLookupTable = NULL;

	// set up the memory manager
	P3gMemory.num_blocks = 0;
	P3gMemory.cur_block = 0;
	P3gMemory.total_used = 0;
	P3gMemory.curblock_free = 0;

	for (itr = 0; itr < P3cMemory_ParticleSizeClasses; itr++) {
		P3gMemory.sizeclass_info[itr].freelist_length = 0;
		P3gMemory.sizeclass_info[itr].freelist_head = NULL;
		P3gMemory.sizeclass_info[itr].decorative_head = NULL;
		P3gMemory.sizeclass_info[itr].decorative_tail = NULL;
	}

	P3gAttractorBuf_Valid	= UUcFalse;
	P3gTemporaryCreation	= UUcFalse;
	P3gTemporaryParticles	= UUcFalse;
	P3gGlobalTintEnable		= UUcFalse;
	P3gGlobalTint			= IMcShade_White;

	P3gParticleCreationCallback = NULL;
	P3gParticleDeletionCallback = NULL;
	P3gParticleUpdateCallback = NULL;

	P3gQuality = P3cQuality_High;

	P3gDebugCollisionEnable = UUcFalse;
	P3gDebugCollisionPoints = NULL;
	
	P3gNextTraverseIndex = 0;

	P3gFirstActiveClass = P3cClass_None;
	P3gLastActiveClass = P3cClass_None;

#if PARTICLE_PERF_ENABLE
	P3gPerfIndex = 0;
	P3gPerfTime = 0;

	P3gPerfMask = P3cDefaultPerfMask;

	P3gPerfDisplay = UUcFalse;
	P3gPerfLockClasses = UUcFalse;
	COrConsole_StatusLines_Begin(P3gPerfDisplayArea, &P3gPerfDisplay, P3cPerfDisplayLines);
#endif

	P3rDecalInitialize();

	return UUcError_None;
}

void P3rTerminate(void)
{
	UUtUns16	itr;
	void *		mem_block;

	P3rDecalTerminate();

	// dispose of any particle classes' memory that we have to
	for (itr = 0; itr < P3gNumClasses; itr++) {

		// dispose of memory allocated for the definition
		mem_block = P3gClassTable[itr].memory.block;
		if (mem_block != NULL)
			UUrMemory_Block_Delete(mem_block);	

		mem_block = P3gClassTable[itr].originalcopy.block;
		if (mem_block != NULL)
			UUrMemory_Block_Delete(mem_block);	
	}
	P3gNumClasses = 0;

	// dispose of the lookup table
	if (P3gClassLookupTable != NULL) {
		UUrMemory_Block_Delete(P3gClassLookupTable);
	}

	// dispose of debugging storage
	if (P3gDebugCollisionPoints != NULL) {
		UUrMemory_Block_Delete(P3gDebugCollisionPoints);
	}
}

/*
 * memory management
 */

// allocate space for a new particle
P3tParticle *P3rNewParticle(P3tParticleClass* inClass)
{
	P3tParticle *		new_particle;
	P3tSizeClassInfo *	sci;

	UUmAssert(inClass->particle_size <= P3gParticleSizeClass[inClass->size_class]);

	// first, look for free space to write over
	new_particle = P3rFindFreeParticle(inClass);

	if (new_particle == NULL) {
		// try to allocate new space
		new_particle = P3rNewParticleStorage(&P3gMemory, inClass, P3gParticleSizeClass[inClass->size_class], inClass->particle_size);

		if ((new_particle == NULL) && ((inClass->definition->flags & P3cParticleClassFlag_Decorative) == 0)) {
			// try to find a decorative particle that we can overwrite
			new_particle = P3rOverwriteDecorativeParticle(inClass);
		}
	}

	if (new_particle == NULL) {
		// we have been completely unable to find any memory for this particle, give up
		return NULL;
	} else {
#if defined(DEBUGGING) && DEBUGGING
		{
			P3tParticle *particleptr;
			UUtUns32 ref;

			// we *MUST* have set up the particle's reference correctly in either
			// FindFreeParticle, NewParticleStorage or OverwriteDecorativeParticle.
			// check that it is correct.
			ref = new_particle->header.self_ref;
			UUmAssert(ref & P3cParticleReference_Valid);
			P3mUnpackParticleReference(ref, inClass, particleptr);
			UUmAssert(particleptr == new_particle);
		}
#endif

		// found a place to store the particle. if it is a decorative particle, add it to
		// the list of all decorative particles in its size class.
		if (inClass->definition->flags & P3cParticleClassFlag_Decorative) {
			sci = &P3gMemory.sizeclass_info[inClass->size_class];

			new_particle->header.prev_decorative = sci->decorative_tail;
			new_particle->header.next_decorative = NULL;
			if (sci->decorative_tail == NULL) {
				sci->decorative_head = new_particle;
			} else {
				sci->decorative_tail->header.next_decorative = new_particle;
			}
			sci->decorative_tail = new_particle;
		} else {
			new_particle->header.prev_decorative = NULL;
			new_particle->header.next_decorative = NULL;
		}

		return new_particle;
	}
}

// allocate a new particle out of global block storage
P3tParticle *P3rNewParticleStorage(P3tParticleMemory *inMemory, P3tParticleClass *inClass,
								   UUtUns16 inAllocateSize, UUtUns16 inActualSize)
{
	UUtUns8 *new_mem;
	UUtUns32 ref, index;
	P3tParticle *new_particle;

	if (inMemory->curblock_free < inAllocateSize) {
		// we need to find a new block of memory
		if (inMemory->cur_block + 1 < inMemory->num_blocks) {
			// move to the next block
			inMemory->cur_block++;
			UUmAssertReadPtr(inMemory->block[inMemory->cur_block], P3cMemory_BlockSize);
		} else {
			// we have to expand the memory manager by allocating another block
			if (inMemory->num_blocks >= P3cMemory_MaxBlocks) {
				// we have reached the maximum limit of the particle memory.
				COrConsole_Printf("### particle memory reached global limit of %d blocks @ %d trying to allocate for '%s'",
							P3cMemory_MaxBlocks, P3cMemory_BlockSize, inClass->classname);
				return NULL;
			}

			new_mem = (UUtUns8 *) UUrMemory_Block_New(P3cMemory_BlockSize);
			if (new_mem == NULL) {
				// can't allocate any new memory.
				COrConsole_Printf("### particle memory failed on block allocation trying to allocate for '%s'", inClass->classname);
				return NULL;
			}

			inMemory->cur_block = inMemory->num_blocks++;
			inMemory->block[inMemory->cur_block] = new_mem;
		}

		// our current block is totally empty
		inMemory->curblock_free = P3cMemory_BlockSize;
	}

	// index must be an even multiple of 64 - because size classes all are. this is relied
	// upon by particle references
	index = P3cMemory_BlockSize - inMemory->curblock_free;
	UUmAssert(index % 64 == 0);

	new_mem = &inMemory->block[inMemory->cur_block][index];
	inMemory->curblock_free		-= inAllocateSize;
	inMemory->total_used		+= inActualSize;

	// we must set up the particle's reference now - based on blocks, etc
	new_particle = (P3tParticle *) new_mem;

	// check that the components of the reference won't overflow
	UUmAssert((inClass >= P3gClassTable) && (inClass - P3gClassTable < (1 << P3cParticleReference_ClassBits)));
	UUmAssert((index >= 0) && (index < (1 << (P3cParticleReference_IndexBits + 6))));
	UUmAssert((inMemory->cur_block >= 0) && (inMemory->cur_block < (1 << P3cParticleReference_BlockBits)));

	ref = P3cParticleReference_Valid;
	ref |= 1							<< P3cParticleReference_SaltOffset;
	ref |= (inClass - P3gClassTable)	<< P3cParticleReference_ClassOffset;
	ref |= index						<< (P3cParticleReference_IndexOffset - 6);
	ref |= inMemory->cur_block			<< P3cParticleReference_BlockOffset;

#if defined(DEBUGGING) && DEBUGGING
	// write garbage over the particle
	UUrMemory_Set8((void *) new_particle, 0xdd, inClass->particle_size);
#endif

	// all other components of the particle will be set up by createparticle
	new_particle->header.self_ref = ref;

	return new_particle;
}

// get space for a particle off the freelist for the class's size range
P3tParticle *P3rFindFreeParticle(P3tParticleClass *inClass)
{
	P3tParticle *new_particle;
	UUtUns32 ref, classindex;

	if (P3gMemory.sizeclass_info[inClass->size_class].freelist_length == 0) {
		// there are no free memory blocks of the right size class
		return NULL;
	}

	new_particle = P3gMemory.sizeclass_info[inClass->size_class].freelist_head;
	UUmAssertReadPtr(new_particle, sizeof(P3tParticle));

	P3gMemory.sizeclass_info[inClass->size_class].freelist_length--;
	P3gMemory.sizeclass_info[inClass->size_class].freelist_head = new_particle->header.nextptr;

#if defined(DEBUGGING) && DEBUGGING
	if (P3gMemory.sizeclass_info[inClass->size_class].freelist_head != NULL) {
		UUmAssertReadPtr(P3gMemory.sizeclass_info[inClass->size_class].freelist_head, sizeof(P3tParticle));
	}
#endif

	// we must alter up the particle's reference now - give it a new class
	// the salt has *ALREADY BEEN DONE* by MarkParticleAsFree
	ref = new_particle->header.self_ref;

#if defined(DEBUGGING) && DEBUGGING
	{
		P3tParticleClass *classptr;
		P3tParticle *particleptr;

		// check that the reference is correct
		UUmAssert(ref & P3cParticleReference_Valid);
		P3mUnpackParticleReference(ref, classptr, particleptr);
		UUmAssert(particleptr == new_particle);
	}
#endif

	// set up the particle's class properly
	classindex = inClass - P3gClassTable;
	UUmAssert((classindex >= 0) && (classindex < (1 << P3cParticleReference_ClassBits)));
	ref &= ~(P3cParticleReference_ClassMask << P3cParticleReference_ClassOffset);
	ref |= classindex << P3cParticleReference_ClassOffset;

	// all other components of the particle will be set up by createparticle
	new_particle->header.self_ref = ref;

	return new_particle;
}

// find an old decorative particle of the right size class that can be overwritten
P3tParticle *P3rOverwriteDecorativeParticle(P3tParticleClass *inClass)
{
	P3tParticleClass *other_class;
	P3tSizeClassInfo *	sci;
	P3tParticle *new_particle;
	UUtUns32 ref, salt, classindex;

	UUmAssertReadPtr(inClass, sizeof(P3tParticleClass));
	sci = &P3gMemory.sizeclass_info[inClass->size_class];

	// look for a decorative particle in this size class
	new_particle = sci->decorative_head;
	if (new_particle == NULL)
		return NULL;

	// kill this particle by removing it from all of its class's lists,
	// but don't mark it as free.
	other_class = &P3gClassTable[new_particle->header.class_index];
	P3rDeleteParticle(other_class, new_particle);

	// get the particle's reference
	ref = new_particle->header.self_ref;
#if defined(DEBUGGING) && DEBUGGING
	{
		P3tParticleClass *classptr;
		P3tParticle *particleptr;

		// check that the particle's reference is correct
		UUmAssert(ref & P3cParticleReference_Valid);
		P3mUnpackParticleReference(ref, classptr, particleptr);
		UUmAssert(classptr == &P3gClassTable[new_particle->header.class_index]);
		UUmAssert(particleptr == new_particle);
	}

	// write garbage over the particle
	UUrMemory_Set8((void *) new_particle, 0xdd, inClass->particle_size);
#endif

	new_particle->header.flags |= P3cParticleFlag_Deleted;

	// increment the reference's salt so that we register existing links
	// to it are no longer valid
	salt = (ref & (P3cParticleReference_SaltMask << P3cParticleReference_SaltOffset)) >> P3cParticleReference_SaltOffset;
	ref &= ~(P3cParticleReference_SaltMask << P3cParticleReference_SaltOffset);
	salt = (salt + 1) & P3cParticleReference_SaltMask;
	ref |= salt << P3cParticleReference_SaltOffset;

	// set up the particle's class properly
	classindex = inClass - P3gClassTable;
	UUmAssert((classindex >= 0) && (classindex < (1 << P3cParticleReference_ClassBits)));
	ref &= ~(P3cParticleReference_ClassMask << P3cParticleReference_ClassOffset);
	ref |= classindex << P3cParticleReference_ClassOffset;

	new_particle->header.self_ref = ref;

	return new_particle;
}

// register a particle creation callback
void P3rRegisterCallbacks(P3tParticleCallback inCreationCallback, P3tParticleCallback inDeletionCallback, P3tParticleCallback inUpdateCallback,
							P3tQualityCallback inQualityCallback)
{
	P3gParticleCreationCallback = inCreationCallback;
	P3gParticleDeletionCallback = inDeletionCallback;
	P3gParticleUpdateCallback = inUpdateCallback;
	P3gQualityCallback = inQualityCallback;
}

// create a new particle and set up initial values
P3tParticle *P3rCreateParticle(P3tParticleClass *inClass, UUtUns32 inTime)
{
	P3tParticle *				new_particle;
	UUtUns16					itr;
	P3tVariableInfo *			var;
//	float *						timer;
	P3tEmitter *				emitter;
	P3tParticleEmitterData *	em_data;

	if (inClass->non_persistent_flags & P3cParticleClass_NonPersistentFlag_Disabled) {
		// this particle class is disabled at this detail setting
		return NULL;
	}

	new_particle = P3rNewParticle(inClass);
	if (new_particle == NULL) {
		// we could not create the particle. bail.
		return NULL;
	}

	if (inClass->num_particles == 0) {
		// this class is currently not a member of the active list, make it one
		P3iActiveClass_Add(inClass, ((inClass->definition->flags & P3cParticleClassFlag_Decorative) > 0), UUcFalse);
	}

	new_particle->header.class_index = inClass - P3gClassTable;
	UUmAssert((new_particle->header.class_index >= 0) && (new_particle->header.class_index < P3gNumClasses));

	// default flags
	new_particle->header.flags = 0;
	new_particle->header.actionmask = inClass->definition->actionmask;
	new_particle->header.current_sound = SScInvalidID;
	new_particle->header.ai_projectile_index = (UUtUns8) -1;
	new_particle->header.sound_flyby_index = (UUtUns8) -1;

	if (inClass->definition->flags2 & P3cParticleClassFlag2_Appearance_InitiallyHidden)
		new_particle->header.flags |= P3cParticleFlag_Hidden;

	if (P3gTemporaryCreation) {
		new_particle->header.flags |= P3cParticleFlag_Temporary;
		inClass->non_persistent_flags |= P3cParticleClass_NonPersistentFlag_HasTemporaryParticles;
	}

	// set up the linked-list pointers
	new_particle->header.prevptr = inClass->tailptr;
	if (new_particle->header.prevptr != NULL) {
		new_particle->header.prevptr->header.nextptr = new_particle;
	} else {
		inClass->headptr = new_particle;
	}
	new_particle->header.nextptr = NULL;
	inClass->tailptr = new_particle;

	inClass->num_particles++;
	P3gTotalParticles++;
	P3gTotalMemoryUsage += P3gParticleSizeClass[inClass->size_class];

	// FIXME: particle system pointers are not yet done
//	new_particle->header.systemptr = NULL;

	// this particle is not linked to anything
	new_particle->header.user_ref = P3cParticleReference_Null;

	// set up the particle's lifetime
	new_particle->header.current_tick = inTime;
	new_particle->header.current_time = ((float) inTime) / UUcFramesPerSecond;
	P3iCalculateValue(NULL, &inClass->definition->lifetime, (void *) &new_particle->header.lifetime);

	// initialise the emitters for this particle
	emitter = inClass->definition->emitter;
	em_data = (P3tParticleEmitterData *) new_particle->vararray;
	for (itr = 0; itr < inClass->definition->num_emitters; itr++, emitter++, em_data++) {
		if (emitter->flags & P3cEmitterFlag_InitiallyActive) {
			em_data->num_emitted = 0;
		} else {
			em_data->num_emitted = P3cEmitterDisabled;
		}

		em_data->time_last_emission = P3cEmitImmediately;
		em_data->last_particle = P3cParticleReference_Null;
	}

	// initialise all of the variables attached to this particle
	var = inClass->definition->variable;
	for (itr = 0; itr < inClass->definition->num_variables; itr++, var++) {
		P3iCalculateValue(NULL, &var->initialiser, (void *) &new_particle->vararray[var->offset]);
	}

	// tint the particle by the global tint if it wants to be
	if (P3gGlobalTintEnable && (inClass->definition->flags2 & P3cParticleClassFlag2_UseGlobalTint)) {
		IMtShade *tintptr = P3rGetTintPtr(inClass, new_particle);

		if (tintptr != NULL) {
			*tintptr = P3gGlobalTint;
		}
	}

	// optional variables. all those relating to a particle's position, velocity etc need
	// to be set up by whatever's calling CreateParticle. others are set up here.

	return new_particle;
}

// create a particle at a given location
P3tParticle *P3rCreateEffect(P3tEffectSpecification *inSpecification, P3tEffectData *inData)
{
	P3tParticleClass *new_class;
	P3tParticle *particle;
	M3tMatrix3x3 *p_orientation;
	M3tPoint3D *p_position;
	M3tVector3D new_velocity, *p_velocity, *p_offset, translation;
	M3tMatrix4x3 **p_matrix, *dynmatrix;
	P3tPrevPosition *p_prev_position;
	P3tOwner *p_owner;
	AKtOctNodeIndexCache *p_envcache;
	UUtUns32 *p_texture, *p_lensframes;
	UUtUns16 int_alpha;
	float v_dot_n, n_dot_n, float_alpha;
	IMtShade tint;
	UUtBool is_dead, caused_by_cutscene, inverse_transform_dynmatrix;


	UUmAssertReadPtr(inSpecification, sizeof(P3tEffectSpecification));
	UUmAssertReadPtr(inData, sizeof(P3tEffectData));

	caused_by_cutscene = ((inData->cause_type == P3cEffectCauseType_CutsceneKilled) ||
							((inData->parent_particle != NULL) && (inData->parent_particle->header.flags & P3cParticleFlag_CutsceneKilled)));

	// if we're being triggered by the fact that we are entering a cutscene, then don't emit any
	// dangerous particles
	new_class = inSpecification->particle_class;
	if (caused_by_cutscene && (new_class->definition->flags2 & (P3cParticleClassFlag2_ExpireOnCutscene | P3cParticleClassFlag2_DieOnCutscene))) {
		return NULL;
	}

	particle = P3rCreateParticle(new_class, inData->time);
	if (particle == NULL)
		return NULL;

	UUmAssert( &P3gClassTable[particle->header.class_index] == new_class );

	if (inData->parent_particle != NULL) {
		if (inData->parent_particle->header.flags & P3cParticleFlag_Temporary) { 
			// temporary particles emit temporary particles
			particle->header.flags |= P3cParticleFlag_Temporary;
			new_class->non_persistent_flags |=
				P3cParticleClass_NonPersistentFlag_HasTemporaryParticles;
		}
	}
	if (caused_by_cutscene) {
		// this effect is being generated as a result of a cutscene killing some particles
		particle->header.flags |= P3cParticleFlag_CutsceneKilled;
	}
	
	p_position = P3rGetPositionPtr(new_class, particle, &is_dead);
	if (is_dead) {
		return NULL;
	}

	/*
	 * calculate particle's position
	 */

	dynmatrix = inData->attach_matrix;
	inverse_transform_dynmatrix = UUcFalse;

	switch(inSpecification->location_type) {
	case P3cEffectLocationType_Default:
	case P3cEffectLocationType_Decal:
		MUmVector_Copy(*p_position, inData->position);
		break;

	case P3cEffectLocationType_FindWall:
		// FIXME: find-wall location not presently implemented.
		UUmAssert(0);
		break;

	case P3cEffectLocationType_CollisionOffset:
		// offset the particle a small amount along the normal
		MUmVector_Copy(*p_position, inData->position);
		MUmVector_ScaleIncrement(*p_position, inSpecification->location_data.collisionoffset.offset,
								inData->normal);
		break;

	case P3cEffectLocationType_AttachedToObject:
		MUmVector_Copy(*p_position, inData->position);
		if ((inData->cause_type == P3cEffectCauseType_ParticleHitPhysics) && (P3gCurrentCollision.collided) &&
			(!P3gCurrentCollision.env_collision) && (P3gCurrentCollision.hit_context != NULL)) {
			// get a dynamic matrix pointer from the physics context that we hit
			dynmatrix = &P3gCurrentCollision.hit_context->matrix;
			inverse_transform_dynmatrix = UUcTrue;
			if (P3gCurrentCollision.hit_context->callback->type == PHcCallback_Character) {
				UUmAssert((P3gCurrentCollision.hit_partindex >= 0) && (P3gCurrentCollision.hit_partindex < ONcNumCharacterParts));
				dynmatrix = ONrCharacter_GetMatrix((ONtCharacter *) P3gCurrentCollision.hit_context->callbackData, P3gCurrentCollision.hit_partindex);
			}
		}
		break;

	default:
		UUmAssert(!"P3rCreateEffect: unknown location type");
		break;
	}

	/*
	 * calculate particle's orientation
	 */

	p_orientation = P3rGetOrientationPtr(new_class, particle);
	if (p_orientation != NULL) {

		switch(inSpecification->collision_orientation) {
		case P3cEnumCollisionOrientation_Reflected:

			if ((inData->cause_type != P3cEffectCauseType_None) && (inData->cause_type != P3cEffectCauseType_CutsceneKilled) &&
				(inData->parent_velocity != NULL)) {
				// we can apply this kind of orientation
				v_dot_n = MUrVector_DotProduct(inData->parent_velocity, &inData->normal);
				n_dot_n = MUrVector_DotProduct(&inData->normal, &inData->normal);

				if (fabs(n_dot_n) > 0.001f) {
					// reflect the velocity in the normal vector
					MUmVector_Copy(new_velocity, inData->normal);
					MUmVector_Scale(new_velocity, -2 * v_dot_n / n_dot_n);
					MUmVector_Add(new_velocity, new_velocity, *inData->parent_velocity);

					if (inData->parent_orientation == NULL) {
						// try upvector -> world Y
						P3rConstructOrientation(P3cEmitterOrientation_Velocity, P3cEmitterOrientation_WorldPosY,
												NULL, NULL, &new_velocity, p_orientation);
					} else {
						// try using parent's upvector
						P3rConstructOrientation(P3cEmitterOrientation_Velocity, P3cEmitterOrientation_ParentPosZ,
												inData->parent_orientation, NULL, &new_velocity, p_orientation);
					}
					break;
				}
			}
			// if we didn't succeed, fall through to the next type (P3cEnumCollisionOrientation_Normal)

		case P3cEnumCollisionOrientation_Normal:
			if ((inData->cause_type != P3cEffectCauseType_None) && (inData->cause_type != P3cEffectCauseType_CutsceneKilled)) {
				// the particle is oriented directly outwards from the impact
				if (inData->parent_orientation != NULL) {
					// try to orient "similarly" to the parent
					P3rConstructOrientation(P3cEmitterOrientation_Velocity, P3cEmitterOrientation_ParentPosZ,
											inData->parent_orientation, NULL, &inData->normal, p_orientation);
				} else {
					// try upvector -> world Y
					P3rConstructOrientation(P3cEmitterOrientation_Velocity, P3cEmitterOrientation_WorldPosY,
											NULL, NULL, &inData->normal, p_orientation);
				}
				break;
			}
			// if we didn't succeed, fall through to the next type (P3cEnumCollisionOrientation_Unchanged)
			
		case P3cEnumCollisionOrientation_Unchanged:
			if (inData->parent_orientation != NULL) {
				// copy the parent's orientation
				*p_orientation = *inData->parent_orientation;

			} else if (inData->parent_velocity != NULL) {
				// orient along the parent's velocity
				P3rConstructOrientation(P3cEmitterOrientation_Velocity, P3cEmitterOrientation_WorldPosY,
										NULL, NULL, inData->parent_velocity, p_orientation);

			} else {
				// give up and use identity matrix
				MUrMatrix3x3_Identity(p_orientation);
			}
			break;

		case P3cEnumCollisionOrientation_Reversed:
			if (inData->parent_orientation != NULL) {
				// copy and reverse the parent's orientation.
				// reverse Y and Z axes - X is unchanged
				p_orientation->m[0][0] =  inData->parent_orientation->m[0][0];
				p_orientation->m[0][1] =  inData->parent_orientation->m[0][1];
				p_orientation->m[0][2] =  inData->parent_orientation->m[0][2];
				p_orientation->m[1][0] = -inData->parent_orientation->m[1][0];
				p_orientation->m[1][1] = -inData->parent_orientation->m[1][1];
				p_orientation->m[1][2] = -inData->parent_orientation->m[1][2];
				p_orientation->m[2][0] = -inData->parent_orientation->m[2][0];
				p_orientation->m[2][1] = -inData->parent_orientation->m[2][1];
				p_orientation->m[2][2] = -inData->parent_orientation->m[2][2];

			} else if (inData->parent_velocity != NULL) {
				// orient along the parent's velocity reversed
				new_velocity.x = -inData->parent_velocity->x;
				new_velocity.y = -inData->parent_velocity->y;
				new_velocity.z = -inData->parent_velocity->z;
				P3rConstructOrientation(P3cEmitterOrientation_Velocity, P3cEmitterOrientation_WorldPosY,
										NULL, NULL, &new_velocity, p_orientation);

			} else {
				// give up and use identity matrix
				MUrMatrix3x3_Identity(p_orientation);
			}
			break;

		default:
			UUmAssert(!"P3rCreateEffect: unknown collision orientation type");
		}
	}

	/*
	 * set up default parameters
	 */

	p_velocity = P3rGetVelocityPtr(new_class, particle);
	if (p_velocity != NULL) {
		MUmVector_Copy(*p_velocity, inData->velocity);
	}

	p_offset = P3rGetOffsetPosPtr(new_class, particle);
	if (p_offset != NULL) {
		MUmVector_Set(*p_offset, 0, 0, 0);
	}

	p_matrix = P3rGetDynamicMatrixPtr(new_class, particle);
	if (p_matrix != NULL) {
		*p_matrix = dynmatrix;

		if (inverse_transform_dynmatrix && (dynmatrix != NULL)) {
			// transform the particle's position by the inverse of this matrix, so that it
			// ends up in the right place
			translation = MUrMatrix_GetTranslation(dynmatrix);
			MUmVector_Subtract(*p_position, *p_position, translation);
			MUrMatrix3x3_TransposeMultiplyPoint(p_position, (M3tMatrix3x3 *) dynmatrix, p_position);
		}
	}

	p_owner = P3rGetOwnerPtr(new_class, particle);
	if (p_owner != NULL) {
		*p_owner = inData->owner;
	}

	// randomise texture start index
	p_texture = P3rGetTextureIndexPtr(new_class, particle);
	if (p_texture != NULL) {
		*p_texture = (UUtUns32) UUrLocalRandom();
	}

	// set texture time index to be now
	p_texture = P3rGetTextureTimePtr(new_class, particle);
	if (p_texture != NULL) {
		*p_texture = inData->time;
	}

	// no previous position
	p_prev_position = P3rGetPrevPositionPtr(new_class, particle);
	if (p_prev_position != NULL) {
		p_prev_position->time = 0;
	}

	// lensflares are not initially visible
	p_lensframes = P3rGetLensFramesPtr(new_class, particle);
	if (p_lensframes != NULL) {
		*p_lensframes = 0;
	}

	p_envcache = P3rGetEnvironmentCache(new_class, particle);
	if (p_envcache != NULL) {
		AKrCollision_InitializeSpatialCache(p_envcache);
	}

	if (inSpecification->particle_class->definition->flags2 & P3cParticleClassFlag2_Appearance_Decal) {
		M3tVector3D			up_dir, right_dir, input_dir, *inputdir_ptr, tempvec;
		P3tDecalData		*decal_data;
		float				xscale, yscale, wrap_angle, theta, costheta, sintheta;
		M3tTextureMap		*texture;
		UUtError			error;
		UUtUns32			decal_flags;
		UUtBool				returnval;

		/*
		 * build a decal to accompany this particle
		 */

		// decals must be created with the right kind of input parameters
		if ((inSpecification->location_type != P3cEffectLocationType_Decal) ||
			(inData->cause_type != P3cEffectCauseType_ParticleHitWall)) {
			COrConsole_Printf("P3rCreateEffect: tried to create decal that wasn't location = decal, cause = wall");
			P3rKillParticle(new_class, particle);
			return NULL;
		}

		// check that this particle can store a decal
		decal_data = P3rGetDecalDataPtr(new_class, particle);
		if (decal_data == NULL) {
			// decal particle classes should ALWAYS automatically be assigned the decal-data optional variable
			UUmAssert(!"P3rCreateEffect: decal class with no decal data");
			P3rKillParticle(new_class, particle);
			return NULL;
		}
		UUrMemory_Clear(decal_data, sizeof(P3tDecalData));

		// get the decal's texture
		error = TMrInstance_GetDataPtr(M3cTemplate_TextureMap, new_class->definition->appearance.instance, &texture);
		if (error != UUcError_None) 
		{
			if ((new_class->non_persistent_flags & P3cParticleClass_NonPersistentFlag_WarnedAboutNotFound) == 0) {
				COrConsole_Printf("DECAL TEXTURE NOT FOUND: decal '%s' texture '%s'", new_class->classname, new_class->definition->appearance.instance);
				new_class->non_persistent_flags |= P3cParticleClass_NonPersistentFlag_WarnedAboutNotFound;
			}
			error = TMrInstance_GetDataPtr(M3cTemplate_TextureMap, "notfoundtex", &texture);
			if (error != UUcError_None) {
				P3rKillParticle(new_class, particle);
				return NULL;
			}
		}

		// get the size of the decal class
		returnval = P3iCalculateValue(particle, &new_class->definition->appearance.scale, (void *) &xscale);
		if (!returnval) {
			xscale = 1.0f;
		}
		if (new_class->definition->flags & P3cParticleClassFlag_Appearance_UseSeparateYScale) {
			returnval = P3iCalculateValue(particle, &new_class->definition->appearance.y_scale, (void *) &yscale);
			if (!returnval) {
				xscale = 1.0f;
			}
		} else {
			yscale = xscale;
		}

		// modify this scale by the individual decal's scale
		xscale *= inData->decal_xscale;
		yscale *= inData->decal_yscale;

		// get the decal's wrap angle
		returnval = P3iCalculateValue(particle, &new_class->definition->appearance.decal_wrap, (void *) &wrap_angle);
		if (!returnval) {
			wrap_angle = 60.0f;
		}
		wrap_angle *= M3cDegToRad;

		// get the input direction that created this decal
		if (inData->parent_velocity != NULL) {
			inputdir_ptr = inData->parent_velocity;

		} else if (inData->parent_orientation != NULL) {
			MUrMatrix_GetCol((M3tMatrix4x3 *) inData->parent_orientation, 1, &input_dir);
			inputdir_ptr = &input_dir;

		} else {
			inputdir_ptr = NULL;
		}

		// calculate the coordinate system for this decal
		UUmAssert(MUmVector_IsNormalized(inData->normal));
		MUrVector_CrossProduct(&inData->normal, &inData->upvector, &right_dir);
		
		if (MUrVector_Normalize_ZeroTest(&right_dir)) {
			// the upvector and the normal are aligned. try an upvector of world Y...
			// cross product of a vector (vx, vy, vz) with (0, 1, 0) is (-vz, 0, vx)
			right_dir.x = -inData->normal.z;
			right_dir.y = 0;
			right_dir.z = inData->normal.x;

			if (MUrVector_Normalize_ZeroTest(&right_dir)) {
				// the normal is along +Y or -Y!
				if (UUmFloat_TightCompareZero(inData->normal.y)) {
					// the normal is zero, cannot create the decal
					P3rKillParticle(new_class, particle);
					return NULL;
				} else {
					// use an upvector of +Z
					MUmVector_Set(right_dir, 0, 0, 1);
				}
			}
		}

		// calculate the perpendicular upvector from the calculated rightvector
		MUrVector_CrossProduct(&right_dir, &inData->normal, &up_dir);
		MUmVector_Normalize(right_dir);

		if (inSpecification->location_data.decal.random_rotation) {
			// rotate the right and up directions by a random angle
			theta = P3rRandom() * M3c2Pi;
			costheta = MUrCos(theta);		sintheta = MUrSin(theta);

			MUmVector_ScaleCopy(tempvec, costheta, up_dir);
			MUmVector_ScaleIncrement(tempvec, sintheta, right_dir);

			MUmVector_Scale(right_dir, costheta);
			MUmVector_ScaleIncrement(right_dir, -sintheta, up_dir);

			MUmVector_Copy(up_dir, tempvec);
		}

		// calculate initial alpha
		P3mAssertFloatValue(new_class->definition->appearance.alpha);
		P3iCalculateValue(particle, &new_class->definition->appearance.alpha, (void *) &float_alpha);
		float_alpha = UUmPin(float_alpha, 0, 1);
		int_alpha = (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(float_alpha * M3cMaxAlpha);

		// calculate initial tint
		P3mAssertShadeValue(new_class->definition->appearance.tint);
		P3iCalculateValue(particle, &new_class->definition->appearance.tint, (void *) &tint);

		decal_flags = 0;
		if (inSpecification->location_data.decal.static_decal) {
			decal_flags |= P3cDecalFlag_Static;
		}
		if (new_class->definition->flags2 & P3cParticleClassFlag2_Appearance_DecalCullBackfacing) {
			decal_flags |= P3cDecalFlag_CullBackFacing;
		}
		if (new_class->definition->flags2 & P3cParticleClassFlag2_Appearance_DecalFullBright) {
			decal_flags |= P3cDecalFlag_FullBright;
		}

		// actually create the decal
		error = P3rCreateDecal(texture, inData->cause_data.particle_wall.gq_index, decal_flags, wrap_angle, inputdir_ptr,
								&inData->position, &inData->normal, &up_dir, &right_dir,
								xscale, yscale, int_alpha, tint, decal_data, 0);
		if (error != UUcError_None) {
			P3rKillParticle(new_class, particle);
			return NULL;
		}

		// set up links between the particle and the newly created decal
		if (inSpecification->location_data.decal.static_decal) {
			particle->header.flags |= P3cParticleFlag_DecalStatic;
		} else {
			particle->header.flags |= P3cParticleFlag_Decal;
		}
		decal_data->particle = particle->header.self_ref;

		UUrMemory_Block_VerifyList();
	}

	P3rSendEvent(new_class, particle, P3cEvent_Create, ((float) inData->time) / UUcFramesPerSecond);
	return particle;
}


// kill a particle
void P3rKillParticle(P3tParticleClass *inClass, P3tParticle *inParticle)
{
	if (inParticle->header.flags & P3cParticleFlag_Deleted) {
		// don't multiply delete particles (although this should never
		// happen anyway)
		return;
	}

#if defined(DEBUGGING) && DEBUGGING
	{
		UUtUns32 ref = inParticle->header.self_ref;
		P3tParticleClass *self_class;
		P3tParticle *self_particle;

		// sanity checking!
		P3mUnpackParticleReference(ref, self_class, self_particle);
		UUmAssert(inClass == self_class);
		UUmAssert(inParticle == self_particle);
		UUmAssert((inParticle->header.flags & P3cParticleFlag_Deleted) == 0);
}
#endif

	// remove the particle from the class's lists
	P3rDeleteParticle(inClass, inParticle);

	// mark this particle as free
	P3rMarkParticleAsFree(inClass, inParticle);
}

// dispose of a particle - cleanup that is REQUIRED when a particle is deleted
// i.e. everything that isn't particlesystem internal
void P3rDisposeParticle(P3tParticleClass *inClass, P3tParticle *inParticle)
{
	if (P3gParticleDeletionCallback != NULL) {
		P3gParticleDeletionCallback(inClass, inParticle);
	}

	if (inClass->definition->flags2 & P3cParticleClassFlag2_Appearance_Decal) {
		P3tDecalData *decal_data = P3rGetDecalDataPtr(inClass, inParticle);

		if (decal_data != NULL) {
			if (inParticle->header.flags & P3cParticleFlag_DecalStatic) {
				P3rDeleteDecal(decal_data, P3cDecalFlag_Static);

			} else if (inParticle->header.flags & P3cParticleFlag_Decal) {
				P3rDeleteDecal(decal_data, 0);

			} else {
				// the particle has no decal flag, which almost certainly indicates that
				// there was an error during the decal creation process nad so no decal
				// exists for us to delete
			}
		}
	}
	
	// if the particle is playing a sound, stop it
	if (inParticle->header.flags & P3cParticleFlag_PlayingAmbientSound) {
#if PERFORMANCE_TIMER
		UUrPerformanceTimer_Enter(P3g_ParticleSound);
#endif
		UUmAssert(inParticle->header.current_sound != SScInvalidID);
		OSrAmbient_Stop(inParticle->header.current_sound);
		inParticle->header.current_sound = SScInvalidID;
		inParticle->header.flags &= ~P3cParticleFlag_PlayingAmbientSound;

#if PERFORMANCE_TIMER
		UUrPerformanceTimer_Exit(P3g_ParticleSound);
#endif
	}
}

// delete a particle - remove it from its class's lists
void P3rDeleteParticle(P3tParticleClass *inClass, P3tParticle *inParticle)
{
	UUmAssert(inClass != NULL);
	UUmAssert(inParticle->header.class_index == inClass - P3gClassTable);

	// run the death event
	P3rSendEvent(inClass, inParticle, P3cEvent_Die, inParticle->header.current_time);

	P3rDisposeParticle(inClass, inParticle);

	// remove this particle from the doubly-linked list
	if (inParticle->header.nextptr == NULL) {
		UUmAssert(inClass->tailptr == inParticle);
		inClass->tailptr = inParticle->header.prevptr;
	} else {
		inParticle->header.nextptr->header.prevptr = inParticle->header.prevptr;
	}

	if (inParticle->header.prevptr == NULL) {
		UUmAssert(inClass->headptr == inParticle);
		inClass->headptr = inParticle->header.nextptr;
	} else {
		inParticle->header.prevptr->header.nextptr = inParticle->header.nextptr;
	}
	
	UUmAssert(inClass->num_particles > 0);
	UUmAssert(P3gTotalParticles > 0);
	inClass->num_particles--;
	P3gTotalParticles--;
	P3gTotalMemoryUsage -= P3gParticleSizeClass[inClass->size_class];


	// FIXME: remove the particle from the particlesystem

	if (inClass->definition->flags & P3cParticleClassFlag_Decorative) {
		// this is a decorative particle and must be removed from the list of
		// decorative particles
		P3tSizeClassInfo *sci = &P3gMemory.sizeclass_info[inClass->size_class];

		if (inParticle->header.prev_decorative == NULL)
			sci->decorative_head = inParticle->header.next_decorative;
		else
			inParticle->header.prev_decorative->header.next_decorative = inParticle->header.next_decorative;

		if (inParticle->header.next_decorative == NULL)
			sci->decorative_tail = inParticle->header.prev_decorative;
		else
			inParticle->header.next_decorative->header.prev_decorative = inParticle->header.prev_decorative;
	}

	if (inClass->num_particles == 0) {
		// this class is no longer a member of the active list
		P3iActiveClass_Remove(inClass);
	}
}

// mark a particle as free
void P3rMarkParticleAsFree(P3tParticleClass *inClass, P3tParticle *inParticle)
{
	UUtUns32 ref, salt;

	ref = inParticle->header.self_ref;

#if defined(DEBUGGING) && DEBUGGING
	{
		P3tParticleClass *classptr;
		P3tParticle *particleptr;

		// check that the particle's reference is correct
		UUmAssert(ref & P3cParticleReference_Valid);
		P3mUnpackParticleReference(ref, classptr, particleptr);
		UUmAssert(classptr == inClass);
		UUmAssert(particleptr == inParticle);
	}

	// write garbage over the particle
	UUrMemory_Set8((void *) inParticle, 0xdd, inClass->particle_size);
#endif

	inParticle->header.flags |= P3cParticleFlag_Deleted;

	// increment the reference's salt so that we register existing links
	// to it are no longer valid
	salt = (ref & (P3cParticleReference_SaltMask << P3cParticleReference_SaltOffset)) >> P3cParticleReference_SaltOffset;
	ref &= ~(P3cParticleReference_SaltMask << P3cParticleReference_SaltOffset);
	salt = (salt + 1) & P3cParticleReference_SaltMask;
	ref |= salt << P3cParticleReference_SaltOffset;

	inParticle->header.self_ref = ref;

	// place this particle in the freelist for its size class
	// so that its memory will be reused when the next particle is
	// allocated
	inParticle->header.nextptr = P3gMemory.sizeclass_info[inClass->size_class].freelist_head;
	P3gMemory.sizeclass_info[inClass->size_class].freelist_head = inParticle;
	P3gMemory.sizeclass_info[inClass->size_class].freelist_length++;
}

// kill all temporary particles
void P3rKillTemporaryParticles(void)
{
	UUtUns16 index;
	P3tParticleClass *classptr;
	P3tParticle *particle, *next_particle;

	P3iActiveList_Lock();

	index = P3gFirstActiveClass;
	while (index != P3cClass_None) {
		classptr = P3gClassTable + index;
		index = classptr->next_active_class;

		if (classptr->non_persistent_flags & P3cParticleClass_NonPersistentFlag_HasTemporaryParticles) {
			// kill all temporary particles in this class
			for (particle = classptr->headptr; particle != NULL; particle = next_particle) {
				next_particle = particle->header.nextptr;
				if (particle->header.flags & P3cParticleFlag_Temporary) {
					P3rKillParticle(classptr, particle);
				}
			}

			// this class no longer has any temporary particles
			classptr->non_persistent_flags &= ~P3cParticleClass_NonPersistentFlag_HasTemporaryParticles;
		}
	}

	P3iActiveList_Unlock();
}

// determine whether newly created particles will be temporary
void P3rSetTemporaryCreation(UUtBool inTempCreate)
{
	P3gTemporaryCreation = inTempCreate;
}

// set and clear the global tint
void P3rSetGlobalTint(IMtShade inTint)
{
	P3gGlobalTintEnable = UUcTrue;
	P3gGlobalTint = inTint;
}

void P3rClearGlobalTint(void)
{
	P3gGlobalTintEnable = UUcFalse;
}

UUtBool P3rSetTint(P3tParticleClass *inClass, P3tParticle *inParticle, IMtShade inTint)
{
	// tint the particle by the global tint if it wants to be
	if (inClass->definition->flags2 & P3cParticleClassFlag2_UseGlobalTint) {
		IMtShade *tintptr = P3rGetTintPtr(inClass, inParticle);

		if (tintptr != NULL) {
			*tintptr = inTint;
		}
		return UUcTrue;
	} else {
		return UUcFalse;
	}
}

// sound data has been moved, re-calculate any cached sound pointers
void P3rRecalculateSoundPointers(void)
{
	UUtUns32 itr;
	P3tParticleClass *classptr;

	for (itr = 0, classptr = P3gClassTable; itr < P3gNumClasses; itr++, classptr++) {
		if ((classptr->non_persistent_flags & P3cParticleClass_NonPersistentFlag_HasSoundPointers) == 0) {
			continue;
		}

		if (classptr->definition->flyby_soundname[0] == '\0') {
			classptr->flyby_sound = NULL;
		} else {
			classptr->flyby_sound = OSrImpulse_GetByName(classptr->definition->flyby_soundname);
		}

		if (classptr->non_persistent_flags & P3cParticleClass_NonPersistentFlag_HasSoundPointers) {
			P3rSetupActionData(classptr);
		}
	}
}

// write to the file the particle classes for which the ambient or impulse
// sound no longer exists
void P3rListBrokenLinks(BFtFile *inFile)
{
	char text[2048];
	UUtUns32 itr;
	P3tParticleClass *classptr;
	
	// printf a header
	BFrFile_Printf(inFile, "********** Particle Sound Links **********"UUmNL);
	BFrFile_Printf(inFile, "Particle Class\tType\tSound Name"UUmNL);
	BFrFile_Printf(inFile, UUmNL);
	
	for (itr = 0, classptr = P3gClassTable; itr < P3gNumClasses; itr++, classptr++) {
		UUtUns16 itr2;
		P3tActionInstance *action;

		if ((classptr->non_persistent_flags & P3cParticleClass_NonPersistentFlag_HasSoundPointers) == 0) {
			continue;
		}
		
		for (itr2 = 0, action = classptr->definition->action; itr2 < classptr->definition->num_actions; itr2++, action++) {
			switch (action->action_type) {
			case P3cActionType_SoundStart:
				if ((action->action_value[0].type == P3cValueType_String) &&
					(action->action_value[0].u.string_const.val[0] != '\0')) {
					SStAmbient *ambient;
					ambient = OSrAmbient_GetByName(action->action_value[0].u.string_const.val);
					if (ambient == NULL) {
						sprintf(text, "%s\tAmbient\t%s", classptr->classname, action->action_value[0].u.string_const.val);
						COrConsole_Printf(text);
						BFrFile_Printf(inFile, "%s"UUmNL, text);
					}
				}
				break;
			case P3cActionType_ImpulseSound:
				if ((action->action_value[0].type == P3cValueType_String) &&
					(action->action_value[0].u.string_const.val[0] != '\0')) {
					SStImpulse *impulse;
					impulse = OSrImpulse_GetByName(action->action_value[0].u.string_const.val);
					if (impulse == NULL) {
						sprintf(text, "%s\tImpulse\t%s", classptr->classname, action->action_value[0].u.string_const.val);
						COrConsole_Printf(text);
						BFrFile_Printf(inFile, "%s"UUmNL, text);
					}
				}
				break;
			}
		}
	}

	BFrFile_Printf(inFile, UUmNL);
	BFrFile_Printf(inFile, UUmNL);
}

#if TOOL_VERSION
// write to the file all particle classes that collide
void P3rListCollision(void)
{
	UUtUns32 itr;
	UUtBool collide_env, collide_char;
	P3tParticleClass *classptr;
	FILE *out_file;
	
	out_file = fopen("particle_collision.txt", "wt");
	if (out_file == NULL) {
		COrConsole_Printf("### could not open particle_collision.txt");
		return;
	}

	// printf a header
	fprintf(out_file, "********** Particle Collision Info **********\n");
	fprintf(out_file, "\n");
	
	for (itr = 0, classptr = P3gClassTable; itr < P3gNumClasses; itr++, classptr++) {
		collide_env  = ((classptr->definition->flags & P3cParticleClassFlag_Physics_CollideEnv) > 0);
		collide_char = ((classptr->definition->flags & P3cParticleClassFlag_Physics_CollideChar) > 0);
		if ((!collide_env) && (!collide_char)) {
			continue;
		}

		fprintf(out_file, "%s:%s%s", classptr->classname, collide_env ? " env" : "", collide_char ? " char" : "");
		if (classptr->definition->collision_radius.u.float_const.val > 0) {
			fprintf(out_file, " (sphere: %.1f)", classptr->definition->collision_radius.u.float_const.val);
		}
		fprintf(out_file, "\n");
	}

	fclose(out_file);
}
#endif

// set up action runtime pointers - called at class load time or if
// any cached pointers need to be recalculated
void P3rSetupActionData(P3tParticleClass *new_class)
{
	UUtUns16 itr;
	P3tActionInstance *action;

	for (itr = 0, action = new_class->definition->action; itr < new_class->definition->num_actions; itr++, action++) {
		action->action_data = 0;
		P3rUpdateActionData(new_class, action);
	}
}

// update the runtime pointer for an action - called by the editing code
void P3rUpdateActionData(P3tParticleClass *inClass, P3tActionInstance *inAction)
{
	char *classname, *impactname;

	UUmAssert(inAction >= inClass->definition->action);
	UUmAssert(inAction < &inClass->definition->action[inClass->definition->num_actions]);

	inAction->action_data = 0;

	switch(inAction->action_type) {
	case P3cActionType_CreateDecal:
		if (inAction->action_value[0].type == P3cValueType_String) 
		{
			UUtError		error;
			M3tTextureMap	*texturemap;
			P3iCalculateValue(NULL, &inAction->action_value[0], (void *) &classname);
			error = TMrInstance_GetDataPtr(M3cTemplate_TextureMap, classname, &texturemap);
			if (error != UUcError_None) 
			{
				COrConsole_Printf("DECAL TEXTURE NOT FOUND: particle class '%s': Create Decal action texture '%s'",
									inClass->classname, classname);

				// default texture if none can be found
				error = TMrInstance_GetDataPtr(M3cTemplate_TextureMap, "notfoundtex", &texturemap);
				if (error != UUcError_None) 
				{
					texturemap = NULL;
				}
			}
			inAction->action_data = (UUtUns32) texturemap;
		}
		break;
	case P3cActionType_CollisionEffect:
		if (inAction->action_value[0].type == P3cValueType_String) {
			P3iCalculateValue(NULL, &inAction->action_value[0], (void *) &classname);
			inAction->action_data = (UUtUns32) P3rGetParticleClass(classname);
		}
		break;

	case P3cActionType_ImpactEffect:
		if (inAction->action_value[0].type == P3cValueType_String) {
			P3iCalculateValue(NULL, &inAction->action_value[0], (void *) &impactname);
			inAction->action_data = (UUtUns32) MArImpactType_GetByName(impactname);
		}
		break;

	case P3cActionType_SoundStart:		
		if (inAction->action_value[0].type == P3cValueType_String) {
			// look up the ambient sound with this name
			inAction->action_data = (UUtUns32) OSrAmbient_GetByName(inAction->action_value[0].u.string_const.val);

			// if sound data moves, this class's action data must be re-setup
			inClass->non_persistent_flags |= P3cParticleClass_NonPersistentFlag_HasSoundPointers;
		}
		break;

	case P3cActionType_ImpulseSound:		
		if (inAction->action_value[0].type == P3cValueType_String) {
			// look up the impulse sound with this name
			inAction->action_data = (UUtUns32) OSrImpulse_GetByName(inAction->action_value[0].u.string_const.val);

			// if sound data moves, this class's action data must be re-setup
			inClass->non_persistent_flags |= P3cParticleClass_NonPersistentFlag_HasSoundPointers;
		}
		break;

	default:
		break;
	}
}

#if TOOL_VERSION
static UUtBool P3iQSort_FindDuplicateClassCompare(UUtUns32 inA, UUtUns32 inB)
{
	P3tParticleClass *class_a = (P3tParticleClass *) inA;
	P3tParticleClass *class_b = (P3tParticleClass *) inB;
	UUtInt32 compare;

	compare = strcmp(class_a->classname, class_b->classname);
	if (compare == 0) {
		// CB: we want to use the particle class with the HIGHER read-index... so we sort
		// in descending order and pick the first one. 11 september 2000
		return (class_a->read_index < class_b->read_index);
	} else {
		return (compare > 0);
	}
}
#endif

static int UUcExternal_Call P3iQSort_AlphabeticalClassCompare(const void *pptr1, const void *pptr2)
{
	P3tParticleClass *pc1 = *((P3tParticleClass **) pptr1), *pc2 = *((P3tParticleClass **) pptr2);

	return strcmp(pc1->classname, pc2->classname);
}

static int UUcExternal_Call P3iQSort_ClassTableOrdering(const void *pptr1, const void *pptr2)
{
	P3tParticleClass *pc1 = (P3tParticleClass *) pptr1, *pc2 = (P3tParticleClass *) pptr2;

	if (pc1->non_persistent_flags & P3cParticleClass_NonPersistentFlag_DoNotLoad) {
		if (pc2->non_persistent_flags & P3cParticleClass_NonPersistentFlag_DoNotLoad) {
			return 0;
		} else {
			return +1;
		}
	} else {
		if (pc2->non_persistent_flags & P3cParticleClass_NonPersistentFlag_DoNotLoad) {
			return -1;
		} else {
			// sort particle classes into essential and decorative
			if (pc1->definition->flags & P3cParticleClassFlag_Decorative) {
				if (pc2->definition->flags & P3cParticleClassFlag_Decorative) {
					return 0;
				} else {
					return +1;
				}
			} else {
				if (pc2->definition->flags & P3cParticleClassFlag_Decorative) {
					return -1;
				} else {
					return 0;
				}
			}
		}
	}
}

// prepare a particle class for use
static UUtBool P3iProcessParticleClass(P3tParticleClass *inClass)
{
	UUtUns32 itr, itr2, data_size;
	UUtUns16 original_version;
	UUtError error;
	UUtBool reset_value, byte_swap;
	P3tActionInstance *action;
	P3tActionTemplate *action_template;
	P3tVariableReference *var_info;
	P3tValue *val_info;

	if (inClass->non_persistent_flags & P3cParticleClass_NonPersistentFlag_DoNotLoad) {
		return UUcFalse;
	}

	byte_swap = ((inClass->non_persistent_flags & P3cParticleClass_NonPersistentFlag_ByteSwap) > 0);

	// update the particle class to the most recent version
	error = P3iUpdateVersion(inClass->classname, &inClass->definition, &inClass->memory,
							byte_swap, inClass->data_size, &original_version);
	if (error != UUcError_None) {
		UUrPrintWarning("Could not update particle class '%s' to current version", inClass->classname);
		return UUcFalse;
	}

	// byte-swap if necessary
	if (byte_swap) {
		P3iSwap_ParticleDefinition((void *) inClass->definition, UUcFalse);
	}

	// verify that the definition was read and updated successfully
	if (!P3mVerifyDefinitionSize(inClass->definition)) {
		UUrPrintWarning("Particle class '%s' definition has size mismatch, cannot load", inClass->classname);
		return UUcFalse;
	}

	// set up pointers to the variable-length arrays
	P3iCalculateArrayPointers(inClass->definition);

	// initialize the particle class
	inClass->originalcopy.size		= 0;
	inClass->originalcopy.block		= NULL;
	inClass->num_particles			= 0;
	inClass->headptr				= NULL;
	inClass->tailptr				= NULL;
	inClass->flyby_sound			= NULL;
	inClass->prev_active_class		= P3cClass_None;
	inClass->next_active_class		= P3cClass_None;

	// CB: all particles that collide against the environment must now store an environment cache
	if (inClass->definition->flags & P3cParticleClassFlag_Physics_CollideEnv) {
		inClass->definition->flags |= P3cParticleClassFlag_HasEnvironmentCache;
	}

	// set up the variable offsets and copy them into each variable reference in the class
	inClass->prev_packed_emitters = (UUtUns16) -1;
	P3rPackVariables(inClass, UUcFalse);

	// determine the size class
	for (inClass->size_class = 0; inClass->size_class < P3cMemory_ParticleSizeClasses; inClass->size_class++) {
		if (inClass->particle_size <= P3gParticleSizeClass[inClass->size_class])
			break;
	}

	if (inClass->size_class >= P3cMemory_ParticleSizeClasses) {
		UUrPrintWarning("Particle class '%s' is too large (%d) for largest size class (%d)!",
						inClass->classname, inClass->particle_size, P3gParticleSizeClass[P3cMemory_ParticleSizeClasses - 1]);
		return UUcFalse;
	}

#if TOOL_VERSION
	// if we had to update the version of this class, mark that we should write it out
	if (inClass->definition->version != original_version) {
		inClass->non_persistent_flags |= P3cParticleClass_NonPersistentFlag_OldVersion;
	}
#endif

	// check that the actionlist indices are correct
	P3iCheckActionListIndices(inClass);

	// CB: decals must now be decorative, and some have been authored that aren't... so set them to be
	if (inClass->definition->flags2 & P3cParticleClassFlag2_Appearance_Decal) {
		inClass->definition->flags |= P3cParticleClassFlag_Decorative;
	}

	// check that all action variables and values are valid
	for (itr = 0, action = inClass->definition->action; itr < inClass->definition->num_actions; itr++, action++) {
		action_template = &P3gActionInfo[action->action_type];
		UUmAssert(action_template->type == action->action_type);

		for (itr2 = 0, var_info = action->action_var; itr2 < action_template->num_variables; itr2++, var_info++) {
			if (var_info->offset == P3cNoVar) {
				continue;
			}
			
			data_size = P3iGetDataSize(var_info->type);
			if ((data_size == 0) || (var_info->offset < 0) || (sizeof(P3tParticleHeader) + var_info->offset + data_size > inClass->particle_size)) {
				// this variable is corrupt, reset it
				var_info->offset = P3cNoVar;
				var_info->type = action_template->info[itr2].datatype;
			}
		}

		for (itr2 = 0, val_info = action->action_value; itr2 < action_template->num_parameters; itr2++, val_info++) {
			reset_value = UUcFalse;
			if ((val_info->type < 0) || (val_info->type >= P3cValueType_NumTypes)) {
				reset_value = UUcTrue;
			} else if (val_info->type == P3cValueType_Variable) {
				data_size = P3iGetDataSize(val_info->u.variable.type);
				if ((data_size == 0) || (val_info->u.variable.offset < 0) ||
					(sizeof(P3tParticleHeader) + val_info->u.variable.offset + data_size > inClass->particle_size)) {
					// this value points to a nonexistent variable, make it a default value
					reset_value = UUcTrue;
				}
			}

			if (reset_value) {
				// this value is corrupt
				P3rSetDefaultInitialiser(action_template->info[action_template->num_variables + itr2].datatype, val_info);
			}
		}
	}

	return UUcTrue;
}

// perform post-load operations on all particle classes
void P3rLoad_PostProcess(void)
{
	UUtUns32 itr, itr2, lookup_index, successfully_processed, num_unique_classes;
	UUtBool verify_enable, success;
	P3tParticleClass *class_itr;
	P3tEmitter *emitter;
#if TOOL_VERSION
	P3tParticleClass **pointer_array;
	char *prev_classname, *this_classname;
#endif

	if (P3gOverflowedClasses > 0) {
		UUrPrintWarning("Too many particle classes; total %d exceeds maximum %d, had to discard %d",
						P3gOverflowedClasses + P3gNumClasses, P3cMaxParticleClasses, P3gOverflowedClasses);
	}

	if (P3gNumClasses == 0) {
		return;
	}

	// disable memory verify-list checking here to speed up the load process
	verify_enable = UUrMemory_SetVerifyEnable(UUcFalse);

	num_unique_classes = P3gNumClasses;
#if TOOL_VERSION
	// because we load both binary data and templated data in the tool, we must check to make sure
	// that we didn't load the same particle class twice. if we did, then keep only the first copy
	// that was read (this should be the binary data version)
	pointer_array = (P3tParticleClass **) UUrMemory_Block_New(P3gNumClasses * sizeof(P3tParticleClass *));
	if (pointer_array != NULL) {
		// sort particle classes by name and read_index
		for (itr = 0; itr < P3gNumClasses; itr++) {
			pointer_array[itr] = P3gClassTable + itr;
		}

		AUrQSort_32(pointer_array, P3gNumClasses, P3iQSort_FindDuplicateClassCompare);

		num_unique_classes = 0;
		prev_classname = "";
		for (itr = 0; itr < P3gNumClasses; itr++) {
			this_classname = pointer_array[itr]->classname;
			if (strcmp(prev_classname, this_classname) == 0) {
				// this particle class should be deleted
				pointer_array[itr]->non_persistent_flags |= P3cParticleClass_NonPersistentFlag_DoNotLoad;
			} else {
				// this is a unique particle class
				prev_classname = this_classname;
				num_unique_classes++;
			}
		}
		UUrMemory_Block_Delete(pointer_array);
	}
#endif

	// load all unique particle classes
	successfully_processed = 0;
	for (itr = 0, class_itr = P3gClassTable; itr < P3gNumClasses; itr++, class_itr++) {
		success = P3iProcessParticleClass(class_itr);
		if (success) {
			successfully_processed++;
		} else {
			// we did not load this class successfully
			if (class_itr->memory.block != NULL) {
				UUrMemory_Block_Delete(class_itr->memory.block);
			}
			class_itr->non_persistent_flags |= P3cParticleClass_NonPersistentFlag_DoNotLoad;
		}
	}

	// restore memory heap verification
	UUrMemory_SetVerifyEnable(verify_enable);

	// sort particle classes into essential and decorative, also placing all unwanted classes at the end.
	qsort((void *) P3gClassTable, P3gNumClasses, sizeof(P3tParticleClass), P3iQSort_ClassTableOrdering);

	// build class lookup table
	P3gClassLookupTable = (P3tParticleClass **) UUrMemory_Block_New(successfully_processed * sizeof(P3tParticleClass *));
	lookup_index = 0;
	for (itr = 0, class_itr = P3gClassTable; itr < P3gNumClasses; itr++, class_itr++) {
		if (class_itr->non_persistent_flags & P3cParticleClass_NonPersistentFlag_DoNotLoad) {
			break;
		}

		P3gClassLookupTable[lookup_index++] = class_itr;
	}
	qsort(P3gClassLookupTable, lookup_index, sizeof(P3tParticleClass *), P3iQSort_AlphabeticalClassCompare);

	// set up global tables
	UUmAssert(lookup_index == successfully_processed);
	P3gNumClasses = (UUtUns16) lookup_index;
	// FIXME: we could resize P3gClassTable here if it was dynamic
	P3gAllocatedClassesIndex = P3gNumClasses;

#if defined(DEBUGGING) && DEBUGGING
	// check that the class lookup table is actually sorted alphabetically
	for (itr = 0; itr + 1 < P3gNumClasses; itr++) {
		UUmAssert(strcmp(P3gClassLookupTable[itr]->classname, P3gClassLookupTable[itr + 1]->classname) < 0);
	}
#endif

	// now that particle classes are at their final places and we have built the lookup table, set up all internal
	// particle-system pointers
	for (itr = 0, class_itr = P3gClassTable; itr < P3gNumClasses; itr++, class_itr++) {
		for (itr2 = 0, emitter = class_itr->definition->emitter; itr2 < class_itr->definition->num_emitters; itr2++, emitter++) {
			emitter->emittedclass = P3rGetParticleClass(emitter->classname);
			if (emitter->emittedclass == NULL) {
				UUrDebuggerMessage("### warning: particle class '%s' emitter %d can't find emitted particle '%s'\n",
						class_itr->classname, itr2 + 1, emitter->classname);
			}
		}

		P3rSetupActionData(class_itr);
	}
}

static UUtBool P3iDumpParticles_SortCompareFunc(UUtUns32 inA, UUtUns32 inB)
{
	P3tParticleClass *class_a = (P3tParticleClass *) inA;
	P3tParticleClass *class_b = (P3tParticleClass *) inB;

	return (strcmp(class_a->classname, class_b->classname) > 0);
}

// for dumping out particle classes to a text file - used for manually merging particle changes
void P3rDumpParticles(void)
{
	P3tParticleClass **classptrs;
	FILE *out_file;
	UUtBool write, printed_action, printed_class;
	UUtUns32 itr, itr2, itr3;
	P3tActionInstance *action;
	char msg[256];

	out_file = fopen("particle_actions.txt", "wt");
	if (out_file == NULL) {
		COrConsole_Printf("### could not open particle_actions.txt");
		return;
	}

	// build an alphabetical list of all particle classes
	classptrs = (P3tParticleClass **) UUrMemory_Block_New(P3gNumClasses * sizeof(P3tParticleClass *));
	for (itr = 0; itr < P3gNumClasses; itr++) {
		classptrs[itr] = &P3gClassTable[itr];
	}
	AUrQSort_32(classptrs, P3gNumClasses, P3iDumpParticles_SortCompareFunc);

	// write out the particle classes
	for (itr = 0; itr < P3gNumClasses; itr++) {
		printed_class = UUcFalse;
		for (itr2 = 0; itr2 < P3cEvent_NumEvents; itr2++) {
			printed_action = UUcFalse;
			for (itr3 = classptrs[itr]->definition->eventlist[itr2].start_index; itr3 < classptrs[itr]->definition->eventlist[itr2].end_index; itr3++) {
				// only write out sound-related actions at present
				action = &classptrs[itr]->definition->action[itr3];

				write = UUcTrue;
				switch(action->action_type) {
					case P3cActionType_SoundStart:
						sprintf(msg, "start ambient %s", action->action_value[0].u.string_const.val);
					break;

					case P3cActionType_SoundEnd:
						sprintf(msg, "end ambient");
					break;

					case P3cActionType_ImpulseSound:
						sprintf(msg, "impulse sound %s", action->action_value[0].u.string_const.val);
					break;

					default:
						write = UUcFalse;
					break;
				}

				if (write) {
					if (!printed_class) {
						fprintf(out_file, "*** %s\n", classptrs[itr]->classname);
						printed_class = UUcTrue;
					}

					if (!printed_action) {
						fprintf(out_file, "  %s:\n", P3gEventName[itr2]);
						printed_action = UUcTrue;
					}

					fprintf(out_file, "    %s\n", msg);
				}
			}
		}
		if (printed_class) {
			fprintf(out_file, "\n");
		}
	}

	UUrMemory_Block_Delete(classptrs);
	fclose(out_file);
}

// set up for a new particle class traversal
void P3rSetupTraverse(void)
{
	P3gNextTraverseIndex++;
}

// traverse particle classes recursively
UUtBool P3rTraverseParticleClass(P3tParticleClass *inClass, P3tClassCallback inCallback, UUtUns32 inUserData)
{
	UUtUns32 itr;
	P3tEmitter *emitter;
	P3tActionInstance *action;

	if ((inClass == NULL) || (inClass->traverse_index == P3gNextTraverseIndex)) {
		// we do not need to enumerate this class
		return UUcFalse;
	}

	// enumerate this class
	inClass->traverse_index = P3gNextTraverseIndex;
	inCallback(inClass, inUserData);

	// enumerate its emitters
	for (itr = 0, emitter = inClass->definition->emitter; itr < inClass->definition->num_emitters; itr++, emitter++) {
		P3rTraverseParticleClass(emitter->emittedclass, inCallback, inUserData);
	}

	// enumerate its actions, looking for any which might emit particles
	for (itr = 0, action = inClass->definition->action; itr < inClass->definition->num_actions; itr++, action++) {
		if (action->action_type == P3cActionType_CollisionEffect) {
			P3rTraverseParticleClass((P3tParticleClass *) action->action_data, inCallback, inUserData);
		}
	}

	return UUcTrue;
}

// precache all the components that a particle class will need to be created and drawn
void P3rPrecacheParticleClass(P3tParticleClass *inClass)
{
	UUtError error;

	/*
	 * precache textures
	 */

	if (((inClass->definition->flags2 & (P3cParticleClassFlag2_Appearance_Invisible | P3cParticleClassFlag2_Appearance_Vector)) == 0) &&
		((inClass->definition->flags & P3cParticleClassFlag_Appearance_Geometry) == 0)) {
		M3tTextureMap *texture;

		error = TMrInstance_GetDataPtr(M3cTemplate_TextureMap, inClass->definition->appearance.instance, &texture);
		if (error == UUcError_None) {
			M3rDraw_Texture_EnsureLoaded(texture);
		}
	}
}

static UUtUns32 P3iGetDisabledParticleMask(P3tQualitySettings inSettings)
{
	switch(P3gQuality) {
		case P3cQuality_High:
			return 0;
		break;

		case P3cQuality_Medium:
			return P3cParticleClassFlag2_MediumDetailDisable;
		break;

		case P3cQuality_Low:
			return (P3cParticleClassFlag2_MediumDetailDisable | P3cParticleClassFlag2_LowDetailDisable);
		break;

		default:
			UUmAssert(!"P3rLevelBegin: unknown quality settings");
			return 0;
		break;
	}
}

#if PARTICLE_PERF_ENABLE
static void P3iZeroPerformanceData(P3tParticleClass *inClass)
{
	inClass->perf_t_accum = 0;
	UUrMemory_Clear(&inClass->perf_t, sizeof(inClass->perf_t));
	UUrMemory_Clear(&inClass->perf_data, sizeof(inClass->perf_data));
	UUrMemory_Clear(&inClass->perf_accum, sizeof(inClass->perf_accum));
}
#endif

// set up for the start of a level
UUtError P3rLevelBegin(void)
{
	P3tParticleClass *class_itr;
	UUtUns32 itr, disabled_mask;

	// FIXME: as a memory saving, dispose of any classes that are unused, resize P3gClassTable?
	// would require some way of recursively determining which classes can be created, including
	// those from objects, those from weapons, and those directly from code (hitflashes etc).

	P3gFirstActiveClass = P3cClass_None;
	P3gLastActiveClass = P3cClass_None;

	P3gFirstDeferredActiveClass = P3cClass_None;
	P3gActiveList_Locked = UUcFalse;

	P3gRemoveDangerousProjectiles = UUcFalse;

#if PARTICLE_PERF_ENABLE
	P3gPerfIndex = 0;
	P3gPerfTime = 0;

	P3gPerfDisplayNumClasses = 0;
	P3gPerfLockClasses = UUcFalse;
	UUrMemory_Clear(P3gPerfDisplayClasses, P3cPerfDisplayClasses * sizeof(P3tParticleClass *));
#endif

	P3gLastDrawTick = 0;
	P3gLastDecorativeTick = 0;

	P3gSkyVisible = UUcTrue;
	P3gDebugCollision_NextPoint = 0;
	P3gDebugCollision_UsedPoints = 0;

	// get particle quality settings
	if (P3gQualityCallback == NULL) {
		P3gQuality = P3cQuality_High;
	} else {
		P3gQualityCallback(&P3gQuality);
	}

	// determine which particle classes are disabled at this detail setting
	disabled_mask = P3iGetDisabledParticleMask(P3gQuality);
	for (itr = 0, class_itr = P3gClassTable; itr < P3gNumClasses; itr++, class_itr++) {
		if (class_itr->definition->flags2 & disabled_mask) {
			class_itr->non_persistent_flags |=  P3cParticleClass_NonPersistentFlag_Disabled;
		} else {
			class_itr->non_persistent_flags &= ~P3cParticleClass_NonPersistentFlag_Disabled;
		}

		class_itr->non_persistent_flags &= ~(P3cParticleClass_NonPersistentFlag_Active | P3cParticleClass_NonPersistentFlag_DeferredAddMask |
											P3cParticleClass_NonPersistentFlag_DeferredRemoval);
		class_itr->num_particles = 0;
		class_itr->prev_active_class = P3cClass_None;
		class_itr->next_active_class = P3cClass_None;

#if PARTICLE_PERF_ENABLE
		P3iZeroPerformanceData(class_itr);
#endif
	}
	P3gTotalParticles = 0;
	P3gTotalMemoryUsage = 0;

	P3rDecal_LevelBegin();

	// set up all attractor class and tag pointers (must be done before particles are created)
	P3rSetupAttractorPointers(NULL);

	return UUcError_None;
}

// recalculate disabled flags based on graphics detail settings
void P3rChangeDetail(P3tQualitySettings inSettings)
{
	UUtUns32 itr, disabled_mask;
	P3tParticleClass *class_itr;
	P3tParticle *particle, *next_particle;

	P3gQuality = inSettings;
	disabled_mask = P3iGetDisabledParticleMask(P3gQuality);
	for (itr = 0, class_itr = P3gClassTable; itr < P3gNumClasses; itr++, class_itr++) {
		if (class_itr->definition->flags2 & disabled_mask) {
			// this particle class is disabled
			class_itr->non_persistent_flags |=  P3cParticleClass_NonPersistentFlag_Disabled;

			if (class_itr->num_particles > 0) {
				// delete all particles from this class
				for (particle = class_itr->headptr; particle != NULL; particle = next_particle) {
					next_particle = particle->header.nextptr;

					P3rKillParticle(class_itr, particle);
				}
			}
			UUmAssert(class_itr->num_particles == 0);
		} else {
			// this particle class is not disabled
			class_itr->non_persistent_flags &= ~P3cParticleClass_NonPersistentFlag_Disabled;
		}
	}
}

// register that the level unload process is beginning
void P3rNotifyUnloadBegin(void)
{
	P3gUnloading = UUcTrue;
}

// kill all particles
void P3rKillAll(void)
{
	P3tParticleClass *classptr;
	P3tParticle *particle;
	UUtUns32 itr;

	for (itr = 0, classptr = P3gClassTable; itr < P3gNumClasses; itr++, classptr++) {
		for (particle = classptr->headptr; particle != NULL; particle = particle->header.nextptr) {
			// this kills the particle without the overhead of maintaining the linked lists, but remains
			// tidy with respect to external systems
			P3rDisposeParticle(classptr, particle);

			// anything that tries to access the particle will see that its self-ref doesn't match
			// and determine that it has died
			particle->header.self_ref = P3cParticleReference_Null;
		}

		classptr->non_persistent_flags &= ~(P3cParticleClass_NonPersistentFlag_Active | P3cParticleClass_NonPersistentFlag_DeferredAddMask |
											P3cParticleClass_NonPersistentFlag_DeferredRemoval);
		classptr->num_particles = 0;
		classptr->headptr = classptr->tailptr = NULL;
		classptr->prev_active_class = P3cClass_None;
		classptr->next_active_class = P3cClass_None;
	}
	P3gTotalParticles = 0;
	P3gTotalMemoryUsage = 0;

	// delete all decal data that we missed in the previous pass (shouldn't be any, but be sure)
	P3rDecalDeleteAllDynamic();
	
	// restore the memory manager to an empty state (but don't deallocate its memory)
	P3gMemory.cur_block = 0;
	P3gMemory.total_used = 0;
	P3gMemory.curblock_free = (P3gMemory.cur_block < P3gMemory.num_blocks) ? P3cMemory_BlockSize : 0;

	for (itr = 0; itr < P3cMemory_ParticleSizeClasses; itr++) {
		P3gMemory.sizeclass_info[itr].freelist_length = 0;
		P3gMemory.sizeclass_info[itr].freelist_head = NULL;
		P3gMemory.sizeclass_info[itr].decorative_head = NULL;
		P3gMemory.sizeclass_info[itr].decorative_tail = NULL;
	}

	P3gFirstActiveClass = P3cClass_None;
	P3gLastActiveClass = P3cClass_None;
}

// remove all dangerous projectiles
static void P3iRemoveDangerousProjectiles(void)
{
	P3tParticleClass *classptr;
	P3tParticle *particle, *next_particle;
	UUtUns16 index;
	UUtBool kill_particles;
	
	UUmAssert(P3gRemoveDangerousProjectiles == UUcTrue);
	
	P3iActiveList_Lock();

	// make all particles with the 'dangerous projectile' flag set expire
	index = P3gFirstActiveClass;
	while (index != P3cClass_None) {
		classptr = P3gClassTable + index;
		index = classptr->next_active_class;
		UUmAssert(classptr->num_particles > 0);
		UUmAssert(classptr->non_persistent_flags & P3cParticleClass_NonPersistentFlag_Active);

		if (classptr->definition->flags2 & P3cParticleClassFlag2_DieOnCutscene) {
			kill_particles = UUcTrue;

		} else if (classptr->definition->flags2 & P3cParticleClassFlag2_ExpireOnCutscene) {
			kill_particles = UUcFalse;

		} else {
			// this particle class is unaffected by the fact that a cutscene is removing
			// all of the dangerous particles
			continue;
		}

		for (particle = classptr->headptr; particle != NULL; particle = next_particle) {
			next_particle = particle->header.nextptr;

			if (particle->header.flags & (P3cParticleFlag_FadeOut | P3cParticleFlag_Chop)) {
				// we don't have to do anything about this particle
				continue;
			}

			particle->header.flags |= P3cParticleFlag_CutsceneKilled;
			if (kill_particles) {
				// kill the particle
				P3rKillParticle(classptr, particle);
			} else {
				// set the particle's lifetime so that it expires next frame
				particle->header.lifetime = 0.01f;
			}
		}
	}

	P3iActiveList_Unlock();

	P3gRemoveDangerousProjectiles = UUcFalse;
}

void P3rRemoveDangerousProjectiles(void)
{
	P3gRemoveDangerousProjectiles = UUcTrue;
}

// shut down at the end of a level
void P3rLevelEnd(void)
{
	UUtUns32	itr;

	UUmAssert(P3gUnloading);

	P3rKillAll();

	// delete all of the memory in the global memory manager
	for (itr = 0; itr < P3gMemory.num_blocks; itr++) {
		UUmAssertReadPtr(P3gMemory.block[itr], P3cMemory_BlockSize);
		UUrMemory_Block_Delete(P3gMemory.block[itr]);
		P3gMemory.block[itr] = NULL;
	}
	P3gMemory.total_used = 0;
	P3gMemory.num_blocks = 0;
	P3gMemory.cur_block = 0;
	P3gMemory.curblock_free = 0;

	for (itr = 0; itr < P3cMemory_ParticleSizeClasses; itr++) {
		P3gMemory.sizeclass_info[itr].freelist_length = 0;
		P3gMemory.sizeclass_info[itr].freelist_head = NULL;
		P3gMemory.sizeclass_info[itr].decorative_head = NULL;
		P3gMemory.sizeclass_info[itr].decorative_tail = NULL;
	}

	// FIXME: remove all particle systems here
	P3gUnloading = UUcFalse;

	P3gFirstActiveClass = P3cClass_None;
	P3gLastActiveClass = P3cClass_None;
}

static void P3iActiveList_Lock(void)
{
	UUmAssert(!P3gActiveList_Locked);
	P3gActiveList_Locked = UUcTrue;
	P3gFirstDeferredActiveClass = P3cClass_None;
	P3gActiveList_MustRemoveUponUnlock = UUcFalse;
}

static void P3iActiveList_Unlock(void)
{
	UUtUns16 index, next_index;
	P3tParticleClass *particle_class;

	UUmAssert(P3gActiveList_Locked);
	P3gActiveList_Locked = UUcFalse;

	if (P3gActiveList_MustRemoveUponUnlock) {
		// we must traverse the whole active list and remove any
		// particle classes that have no particles
		index = P3gFirstActiveClass;
		while (index != P3cClass_None) {
			particle_class = &P3gClassTable[index];
			index = particle_class->next_active_class;

			if (particle_class->non_persistent_flags & P3cParticleClass_NonPersistentFlag_DeferredRemoval) {
				P3iActiveClass_Remove(particle_class);
			}
		}
	}

	// we must now traverse the waiting-to-be-added list and add
	// any particle classes stored there
	index = P3gFirstDeferredActiveClass;
	while (index != P3cClass_None) {
		particle_class = &P3gClassTable[index];
		next_index = particle_class->next_active_class;

		UUmAssert(particle_class->num_particles > 0);
		UUmAssert(particle_class->non_persistent_flags & P3cParticleClass_NonPersistentFlag_DeferredAddMask);

		if (particle_class->prev_active_class != P3cClass_None) {
			// our prev_active_class index must point to another class
			// that is not yet active but has particles
			UUmAssert((P3gClassTable[particle_class->prev_active_class].non_persistent_flags &
						P3cParticleClass_NonPersistentFlag_Active) == 0);
			UUmAssert(P3gClassTable[particle_class->prev_active_class].num_particles > 0);

			P3gClassTable[particle_class->prev_active_class].next_active_class = particle_class->next_active_class;			
		} else {
			P3gFirstDeferredActiveClass = particle_class->next_active_class;
		}

		if (particle_class->next_active_class != P3cClass_None) {
			// our next_active_class index must point to another class
			// that is not yet active but has particles
			UUmAssert((P3gClassTable[particle_class->next_active_class].non_persistent_flags &
						P3cParticleClass_NonPersistentFlag_Active) == 0);
			UUmAssert(P3gClassTable[particle_class->next_active_class].num_particles > 0);

			P3gClassTable[particle_class->next_active_class].prev_active_class = P3cClass_None;			
		}

		// remove this particle class from the deferred add list
		particle_class->next_active_class = P3cClass_None;
		particle_class->prev_active_class = P3cClass_None;

		// add this particle class to the active list
		if (particle_class->non_persistent_flags & P3cParticleClass_NonPersistentFlag_DeferredAddAtStart) {
			P3iActiveClass_Add(particle_class, UUcFalse, UUcTrue);

		} else if (particle_class->non_persistent_flags & P3cParticleClass_NonPersistentFlag_DeferredAddAtEnd) {
			P3iActiveClass_Add(particle_class, UUcTrue, UUcTrue);

		} else {
			UUmAssert(0);		// should never get here
		}

		particle_class->non_persistent_flags &= ~P3cParticleClass_NonPersistentFlag_DeferredAddMask;

		index = next_index;
	}

	UUmAssert(P3gFirstDeferredActiveClass == P3cClass_None);
}

static void P3iActiveClass_Add(P3tParticleClass *inClass, UUtBool inAddAtEnd, UUtBool inWasDeferred)
{
	UUtUns16 this_class_index;

	UUmAssert(inWasDeferred || (inClass->num_particles == 0));
	UUmAssert(inWasDeferred || ((inClass->non_persistent_flags & P3cParticleClass_NonPersistentFlag_DeferredAddMask) == 0));

	if (inClass->non_persistent_flags & P3cParticleClass_NonPersistentFlag_DeferredRemoval) {
		// this class is set up to be removed at the end of this frame, but it's still a member of the
		// active list. don't do anything to it except take it off the deferred-removal list.
		UUmAssert(inClass->non_persistent_flags & P3cParticleClass_NonPersistentFlag_Active);
		inClass->non_persistent_flags &= ~P3cParticleClass_NonPersistentFlag_DeferredRemoval;
		return;
	}

	UUmAssert((inClass->prev_active_class == P3cClass_None) && (inClass->next_active_class == P3cClass_None));
	UUmAssert((inClass->non_persistent_flags & P3cParticleClass_NonPersistentFlag_Active) == 0);

	this_class_index = (UUtUns16) (inClass - P3gClassTable);

	if (P3gActiveList_Locked) {
		UUmAssert(!inWasDeferred);

		// add this class to the list that will become active when we unlock
		inClass->next_active_class = P3gFirstDeferredActiveClass;
		inClass->prev_active_class = P3cClass_None;

		if (P3gFirstDeferredActiveClass != P3cClass_None) {
			// the P3gFirstDeferredActiveClass index must point to another class
			// that is not yet active but has particles
			UUmAssert((P3gClassTable[P3gFirstDeferredActiveClass].non_persistent_flags &
						P3cParticleClass_NonPersistentFlag_Active) == 0);
			UUmAssert(P3gClassTable[P3gFirstDeferredActiveClass].num_particles > 0);
			UUmAssert(P3gClassTable[P3gFirstDeferredActiveClass].non_persistent_flags &
						P3cParticleClass_NonPersistentFlag_DeferredAddMask);

			P3gClassTable[P3gFirstDeferredActiveClass].prev_active_class = this_class_index;
		}
		P3gFirstDeferredActiveClass = this_class_index;

		if (inAddAtEnd) {
			inClass->non_persistent_flags |= P3cParticleClass_NonPersistentFlag_DeferredAddAtEnd;
		} else {
			inClass->non_persistent_flags |= P3cParticleClass_NonPersistentFlag_DeferredAddAtStart;
		}
	} else {
		// add this class to the active list
		if (inAddAtEnd) {
			inClass->prev_active_class = P3gLastActiveClass;
			inClass->next_active_class = P3cClass_None;

			if (inClass->prev_active_class == P3cClass_None) {
				P3gFirstActiveClass = this_class_index;
			} else {
				P3gClassTable[inClass->prev_active_class].next_active_class = this_class_index;
			}
			P3gLastActiveClass = this_class_index;
		} else {
			inClass->prev_active_class = P3cClass_None;
			inClass->next_active_class = P3gFirstActiveClass;

			if (inClass->next_active_class == P3cClass_None) {
				P3gLastActiveClass = this_class_index;
			} else {
				P3gClassTable[inClass->next_active_class].prev_active_class = this_class_index;
			}
			P3gFirstActiveClass = this_class_index;
		}

#if PARTICLE_PERF_ENABLE
		P3iZeroPerformanceData(inClass);
#endif
		inClass->non_persistent_flags |= P3cParticleClass_NonPersistentFlag_Active;
	}
}

static void P3iActiveClass_Remove(P3tParticleClass *inClass)
{
	UUtUns16 this_class_index;

	UUmAssert(inClass->num_particles == 0);

	this_class_index = (UUtUns16) (inClass - P3gClassTable);

	if (inClass->non_persistent_flags & P3cParticleClass_NonPersistentFlag_Active) {
		if (P3gActiveList_Locked) {
			// don't do anything, just leave this class alone. it will be removed when
			// we unlock the active list
			P3gActiveList_MustRemoveUponUnlock = UUcTrue;
			inClass->non_persistent_flags |= P3cParticleClass_NonPersistentFlag_DeferredRemoval;
		} else {
			// remove this class from the active list and patch up the pointers in both directions
			if (inClass->prev_active_class == P3cClass_None) {
				P3gFirstActiveClass = inClass->next_active_class;
			} else {
				P3gClassTable[inClass->prev_active_class].next_active_class = inClass->next_active_class;
			}
			if (inClass->next_active_class == P3cClass_None) {
				P3gLastActiveClass = inClass->prev_active_class;
			} else {
				P3gClassTable[inClass->next_active_class].prev_active_class = inClass->prev_active_class;
			}

			inClass->prev_active_class = P3cClass_None;
			inClass->next_active_class = P3cClass_None;
			inClass->non_persistent_flags &= ~(P3cParticleClass_NonPersistentFlag_Active | P3cParticleClass_NonPersistentFlag_DeferredRemoval);
		}
	} else {
		// we are removing a non-active class, the list must be locked otherwise the
		// class would already be active
		UUmAssert(P3gActiveList_Locked);
		UUmAssert(inClass->non_persistent_flags & P3cParticleClass_NonPersistentFlag_DeferredAddMask);

		// patch up the deferred chain next pointer
		if (inClass->prev_active_class != P3cClass_None) {
			// our prev_active_class index must point to another class
			// that is not yet active but has particles
			UUmAssert((P3gClassTable[inClass->prev_active_class].non_persistent_flags &
						P3cParticleClass_NonPersistentFlag_Active) == 0);
			UUmAssert(P3gClassTable[inClass->prev_active_class].num_particles > 0);
			UUmAssert(P3gClassTable[inClass->prev_active_class].non_persistent_flags &
						P3cParticleClass_NonPersistentFlag_DeferredAddMask);

			P3gClassTable[inClass->prev_active_class].next_active_class = inClass->next_active_class;			
		} else {
			P3gFirstDeferredActiveClass = inClass->next_active_class;
		}

		// patch up the deferred chain prev pointer
		if (inClass->next_active_class != P3cClass_None) {
			// our next_active_class index must point to another class
			// that is not yet active but has particles
			UUmAssert((P3gClassTable[inClass->next_active_class].non_persistent_flags &
						P3cParticleClass_NonPersistentFlag_Active) == 0);
			UUmAssert(P3gClassTable[inClass->next_active_class].num_particles > 0);
			UUmAssert(P3gClassTable[inClass->next_active_class].non_persistent_flags &
						P3cParticleClass_NonPersistentFlag_DeferredAddMask);

			P3gClassTable[inClass->next_active_class].prev_active_class = inClass->prev_active_class;			
		}

		inClass->prev_active_class = P3cClass_None;
		inClass->next_active_class = P3cClass_None;
		inClass->non_persistent_flags &= ~(P3cParticleClass_NonPersistentFlag_Active | P3cParticleClass_NonPersistentFlag_DeferredAddMask);
	}
}

/*
 * particle class loading
 */

// resize the memory allocated for a particle definition
static UUtError P3iResizeParticleDefinition(P3tParticleDefinition **ioDefinition, P3tMemoryBlock *ioMemory,
											UUtUns16 inSize, UUtBool inValidPointers)
{
	void *new_mem;

	UUmAssertReadPtr(ioMemory, sizeof(P3tMemoryBlock));
	UUmAssertReadPtr(ioDefinition, sizeof(P3tParticleDefinition *));

	// this memory block must be the right one for the definition. it may be
	// NULL to indicate that there's no dynamic memory to grow from, but any
	// other value should point exactly sizeof(BDtHeader) before the definition.
	UUmAssert((ioMemory->block == NULL) ||
			  (ioMemory->block == P3mBackHeader(*ioDefinition)));

	if (ioMemory->size < inSize) {
		// we need a larger memory block.
		if (ioMemory->block != NULL) {
			// grow from existing allocated block  that was loaded from binary file
			new_mem = UUrMemory_Block_Realloc(ioMemory->block, inSize + sizeof(BDtHeader));
			if (new_mem == NULL)
				return UUcError_OutOfMemory;
		} else {
			// allocate new memory block (original was loaded from .dat file)
			new_mem = UUrMemory_Block_New(inSize + sizeof(BDtHeader));
			if (new_mem == NULL)
				return UUcError_OutOfMemory;

			// copy the original definition
			UUrMemory_MoveFast(*ioDefinition, P3mSkipHeader(new_mem), ioMemory->size - sizeof(BDtHeader));
		}

		// store this new memory block
		ioMemory->size = inSize;
		ioMemory->block = new_mem;
		*ioDefinition = P3mSkipHeader(new_mem);

		if (inValidPointers) {
			// set up the array pointers again
			P3iCalculateArrayPointers(*ioDefinition);
		}
	}

	(*ioDefinition)->definition_size = inSize;

	return UUcError_None;
}

// update a particle class to the current version
static UUtError P3iUpdateVersion(const char *inIdentifier, P3tParticleDefinition **ioDefinition,
						 P3tMemoryBlock *ioMemory, UUtBool inSwapBytes, UUtUns32 inSize, UUtUns16 *outOldVersion)
{
	UUtUns16 itr, new_size, num_variables, num_actions, num_emitters;
	UUtUns8 *copyfrom, *copyto;
	UUtUns16 copysize, burst_size;
	UUtUns32 mask;
	P3tParticleVersion_Initial		*def_initial;
	P3tParticleVersion_EventList	*def_eventlist;
	P3tParticleVersion_Collision	*def_collision;
	P3tParticleVersion_ActionPtr	*def_actionptr;
	P3tParticleVersion_XOffset		*def_xoffset;
	P3tParticleVersion_Tint			*def_tint;
	P3tParticleVersion_ActionMask	*def_actionmask;
	P3tParticleVersion_EmitterLink	*def_emitterlink;
	P3tParticleVersion_MaxContrail	*def_maxcontrail;
	P3tParticleVersion_EmitterOrient*def_emitterorient;
	P3tParticleVersion_LensflareDist*def_lensflaredist;
	P3tParticleVersion_Flags2		*def_flags2;
	P3tParticleVersion_Decal		*def_decal;
	P3tParticleVersion_MoreEvents	*def_moreevents;
	P3tParticleVersion_Attractor	*def_attractor;
	P3tParticleVersion_AttractorAngle*def_attractorangle;
	P3tParticleVersion_AttractorPtr	*def_attractorptr;
	P3tParticleVersion_DecalWrap	*def_decalwrap;
	P3tParticleVersion_Dodge		*def_dodge;

	if (inSwapBytes) {
		// byte-swap the size and version information
		UUrSwap_2Byte((void *) &((*ioDefinition)->definition_size));
		UUrSwap_2Byte((void *) &((*ioDefinition)->version));
	}

	*outOldVersion = (*ioDefinition)->version;

	if ((*ioDefinition)->definition_size != inSize) {
		UUrPrintWarning("Particle class '%s' corrupt! Says size is %d but really is %d",
						inIdentifier, (*ioDefinition)->definition_size, inSize);
		return UUcError_Generic;
	}

	if ((*ioDefinition)->version > P3cVersion_Current) {
		UUrPrintWarning("Particle class '%s' unknown version %d (most recent is %d)",
						inIdentifier, (*ioDefinition)->version, P3cVersion_Current);
		return UUcError_Generic;
	}

	if ((*ioDefinition)->version == P3cVersion_Current) {
		// current version, nothing to change
  		if (inSwapBytes) {
			UUrSwap_2Byte((void *) &((*ioDefinition)->version));
			UUrSwap_2Byte((void *) &((*ioDefinition)->definition_size));
		}

		return UUcError_None;
	}

	switch((*ioDefinition)->version) {
	case P3cVersion_Initial:
		// must move everything starting from 'lifetime' along to make
		// room for the changes: actionlist_everyframe (size P3tActionList) went
		// to eventlist (size P3cMaxNumEvents_Version0 * P3tActionList)

		new_size = (*ioDefinition)->definition_size + (P3cMaxNumEvents_Version0 - 1) * sizeof(P3tActionList);
		P3iResizeParticleDefinition(ioDefinition, ioMemory, new_size, UUcFalse);

		def_initial	= (P3tParticleVersion_Initial *) (*ioDefinition);
		def_eventlist = (P3tParticleVersion_EventList *) (*ioDefinition);
		copyfrom = (UUtUns8 *) &def_initial->lifetime;
		copyto = (UUtUns8 *) &def_eventlist->lifetime;
		UUmAssert(copyto - copyfrom == (P3cMaxNumEvents_Version0 - 1) * sizeof(P3tActionList));

		copysize = new_size - (copyto - ((UUtUns8 *) *ioDefinition));

		UUrMemory_MoveOverlap(copyfrom, copyto, copysize);
		UUrMemory_Clear(copyfrom, copyto - copyfrom);

		// set up the event lists that we've inserted
		for (itr = 1; itr < P3cMaxNumEvents_Version0; itr++) {
			def_eventlist->eventlist[itr].start_index = def_eventlist->eventlist[0].end_index;
			def_eventlist->eventlist[itr].end_index	= def_eventlist->eventlist[0].end_index;
		}

		(*ioDefinition)->version = P3cVersion_EventList;
		// fall through to further versioning updates

	case P3cVersion_EventList:
		// must move everything starting from 'appearance' along to make
		// room for the inserted P3tValue collision_radius

		new_size = (*ioDefinition)->definition_size + sizeof(P3tValue);
		P3iResizeParticleDefinition(ioDefinition, ioMemory, new_size, UUcFalse);

		def_eventlist = (P3tParticleVersion_EventList *) (*ioDefinition);
		def_collision = (P3tParticleVersion_Collision *) (*ioDefinition);
		copyfrom = (UUtUns8 *) &def_eventlist->appearance;
		copyto = (UUtUns8 *) &def_collision->appearance;
		UUmAssert(copyto - copyfrom == sizeof(P3tValue));

		copysize = new_size - (copyto - ((UUtUns8 *) *ioDefinition));

		UUrMemory_MoveOverlap(copyfrom, copyto, copysize);
		UUrMemory_Clear(copyfrom, copyto - copyfrom);

		// set up the new collision radius that we've inserted
		def_collision->collision_radius.type = P3cValueType_Float;
		def_collision->collision_radius.u.float_const.val = 0.0f;
		if (inSwapBytes) {
			P3iSwap_Value((void *) &def_collision->collision_radius, UUcTrue);
		}

		(*ioDefinition)->version = P3cVersion_Collision;
		// fall through to further versioning updates

	case P3cVersion_Collision:
		// every action instance now has an extra field in it. we will need to
		// move around some of the variable-length arrays - therefore we must extract
		// the lengths of these arrays.
		num_variables = ((P3tParticleVersion_Collision *) (*ioDefinition))->num_variables;
		num_actions   = ((P3tParticleVersion_Collision *) (*ioDefinition))->num_actions;
		num_emitters  = ((P3tParticleVersion_Collision *) (*ioDefinition))->num_emitters;
		if (inSwapBytes) {
			UUrSwap_2Byte((void *) &num_variables);
			UUrSwap_2Byte((void *) &num_actions);
			UUrSwap_2Byte((void *) &num_emitters);
		}

		if (num_actions > 0) {
			// every action expands by sizeof(void *)
			new_size = (*ioDefinition)->definition_size + num_actions * sizeof(void *);
			P3iResizeParticleDefinition(ioDefinition, ioMemory, new_size, UUcFalse);

			def_collision = (P3tParticleVersion_Collision *) (*ioDefinition);
			def_actionptr = (P3tParticleVersion_ActionPtr *) (*ioDefinition);

			// determine whereabouts the action data starts - in the same place in both
			def_collision->action = (P3tActionInstance_Version0 *)	((UUtUns8 *) (*ioDefinition) +
				sizeof(P3tParticleVersion_Collision) + num_variables * sizeof(P3tVariableInfo));
			def_actionptr->action = (P3tActionInstance *)			((UUtUns8 *) (*ioDefinition) +
				sizeof(P3tParticleVersion_ActionPtr) + num_variables * sizeof(P3tVariableInfo));
			UUmAssert(((void *) def_actionptr->action) == ((void *) def_collision->action));

			// move down all data following the actions
			copyfrom = (UUtUns8 *) &def_collision->action[num_actions];
			copyto = (UUtUns8 *) &def_actionptr->action[num_actions];
			UUmAssert(copyto == copyfrom + num_actions * sizeof(void *));

			copysize = new_size - (copyto - ((UUtUns8 *) *ioDefinition));
			UUrMemory_MoveOverlap(copyfrom, copyto, copysize);

			// expand all of the actions, in order from last to first.
			for (itr = num_actions - 1; ; itr--) {
				UUrMemory_MoveOverlap(def_collision->action[itr].action_value, def_actionptr->action[itr].action_value,
									  P3cActionMaxParameters * sizeof(P3tValue));

				UUrMemory_MoveOverlap(def_collision->action[itr].action_var, def_actionptr->action[itr].action_var,
									  P3cActionMaxParameters * sizeof(P3tVariableReference));

				def_actionptr->action[itr].action_data = 0;
				def_actionptr->action[itr].action_type = def_collision->action[itr].action_type;

				if (itr == 0)
					break;
			}

			// nothing before the actions needs to be changed
		}

		(*ioDefinition)->version = P3cVersion_ActionPtr;
		// fall through to further versioning updates

	case P3cVersion_ActionPtr:
		// must move everything after 'appearance' along to make
		// room for the two inserted P3tValues inside appearance

		new_size = (*ioDefinition)->definition_size + 2 * sizeof(P3tValue);
		P3iResizeParticleDefinition(ioDefinition, ioMemory, new_size, UUcFalse);

		def_actionptr = (P3tParticleVersion_ActionPtr *) (*ioDefinition);
		def_xoffset   = (P3tParticleVersion_XOffset   *) (*ioDefinition);
		copyfrom = (UUtUns8 *) &def_actionptr->appearance + sizeof(P3tAppearanceVersion_Initial);
		copyto = (UUtUns8 *) &def_xoffset->appearance + sizeof(P3tAppearanceVersion_XOffset);
		UUmAssert(copyto - copyfrom == 2 * sizeof(P3tValue));

		copysize = new_size - (copyto - ((UUtUns8 *) *ioDefinition));

		UUrMemory_MoveOverlap(copyfrom, copyto, copysize);
		UUrMemory_Clear(copyfrom, copyto - copyfrom);

		// set up the new values that we've inserted in appearance
		def_xoffset->appearance.x_offset.type = P3cValueType_Float;
		def_xoffset->appearance.x_offset.u.float_const.val = 0.0f;
		def_xoffset->appearance.x_shorten.type = P3cValueType_Float;
		def_xoffset->appearance.x_shorten.u.float_const.val = 0.0f;
		if (inSwapBytes) {
			P3iSwap_Value((void *) &def_xoffset->appearance.x_offset, UUcTrue);
			P3iSwap_Value((void *) &def_xoffset->appearance.x_shorten, UUcTrue);
		}

		(*ioDefinition)->version = P3cVersion_XOffset;
		// fall through to further versioning updates

	case P3cVersion_XOffset:
		// must move everything after 'appearance' along to make
		// room for the three inserted P3tValues inside appearance

		new_size = (*ioDefinition)->definition_size + 3 * sizeof(P3tValue);
		P3iResizeParticleDefinition(ioDefinition, ioMemory, new_size, UUcFalse);

		def_xoffset   = (P3tParticleVersion_XOffset   *) (*ioDefinition);
		def_tint	  = (P3tParticleVersion_Tint *) (*ioDefinition);
		copyfrom = (UUtUns8 *) &def_xoffset->appearance + sizeof(P3tAppearanceVersion_XOffset);
		copyto = (UUtUns8 *) &def_tint->appearance + sizeof(P3tAppearanceVersion_Tint);
		UUmAssert(copyto - copyfrom == 3 * sizeof(P3tValue));

		copysize = new_size - (copyto - ((UUtUns8 *) *ioDefinition));

		UUrMemory_MoveOverlap(copyfrom, copyto, copysize);
		UUrMemory_Clear(copyfrom, copyto - copyfrom);

		// set up the new values that we've inserted in appearance
		def_tint->appearance.tint.type = P3cValueType_Shade;
		def_tint->appearance.tint.u.shade_const.val = IMcShade_White;
		def_tint->appearance.edgefade_min.type = P3cValueType_Float;
		def_tint->appearance.edgefade_min.u.float_const.val = 0.0f;
		def_tint->appearance.edgefade_max.type = P3cValueType_Float;
		def_tint->appearance.edgefade_max.u.float_const.val = 0.0f;
		if (inSwapBytes) {
			P3iSwap_Value((void *) &def_tint->appearance.tint, UUcTrue);
			P3iSwap_Value((void *) &def_tint->appearance.edgefade_min, UUcTrue);
			P3iSwap_Value((void *) &def_tint->appearance.edgefade_max, UUcTrue);
		}

		(*ioDefinition)->version = P3cVersion_Tint;
		// fall through to further versioning updates

	case P3cVersion_Tint:
		// must insert room for the new field UUtUns32 actionmask, which comes immediately
		// before num_variables
		new_size = (*ioDefinition)->definition_size + sizeof(UUtUns32);
		P3iResizeParticleDefinition(ioDefinition, ioMemory, new_size, UUcFalse);

		def_tint	  = (P3tParticleVersion_Tint       *) (*ioDefinition);
		def_actionmask= (P3tParticleVersion_ActionMask *) (*ioDefinition);
		copyfrom = (UUtUns8 *) &def_tint->num_variables;
		copyto = (UUtUns8 *) &def_actionmask->num_variables;
		UUmAssert(copyto - copyfrom == sizeof(UUtUns32));

		copysize = new_size - (copyto - ((UUtUns8 *) *ioDefinition));

		UUrMemory_MoveOverlap(copyfrom, copyto, copysize);
		UUrMemory_Clear(copyfrom, copyto - copyfrom);

		// set up the actionmask
		def_actionmask->actionmask = 0;

		(*ioDefinition)->version = P3cVersion_ActionMask;
		// fall through to further versioning updates

	case P3cVersion_ActionMask:
		// every emitter now has an extra field in it. we will need to
		// move around some of the variable-length arrays - therefore we must extract
		// the lengths of these arrays.
		num_variables = ((P3tParticleVersion_ActionMask *) (*ioDefinition))->num_variables;
		num_actions   = ((P3tParticleVersion_ActionMask *) (*ioDefinition))->num_actions;
		num_emitters  = ((P3tParticleVersion_ActionMask *) (*ioDefinition))->num_emitters;
		if (inSwapBytes) {
			UUrSwap_2Byte((void *) &num_variables);
			UUrSwap_2Byte((void *) &num_actions);
			UUrSwap_2Byte((void *) &num_emitters);
		}

		if (num_emitters > 0) {
			// every emitter expands by sizeof(P3tEmitterLinkType)
			new_size = (*ioDefinition)->definition_size + num_emitters * sizeof(P3tEmitterLinkType);
			P3iResizeParticleDefinition(ioDefinition, ioMemory, new_size, UUcFalse);

			def_actionmask	= (P3tParticleVersion_ActionMask *) (*ioDefinition);
			def_emitterlink = (P3tParticleVersion_EmitterLink *) (*ioDefinition);

			// determine whereabouts the emitter data starts - in the same place in both
			def_actionmask->emitter = (P3tEmitter_Version0 *)	((UUtUns8 *) (*ioDefinition) +
				sizeof(P3tParticleVersion_ActionMask) + num_variables * sizeof(P3tVariableInfo)
													  + num_actions * sizeof(P3tActionInstance));
			def_emitterlink->emitter = (P3tEmitter_Version7 *)	((UUtUns8 *) (*ioDefinition) +
				sizeof(P3tParticleVersion_EmitterLink) + num_variables * sizeof(P3tVariableInfo)
													   + num_actions * sizeof(P3tActionInstance));
			UUmAssert(((void *) def_actionmask->emitter) == ((void *) def_emitterlink->emitter));

			// there is no data following the emitters!

			// expand all of the emitters, in order from last to first.
			for (itr = num_emitters - 1; ; itr--) {
				// move the data following linktype
				copyfrom = (UUtUns8 *) &def_actionmask->emitter[itr].rate_type;
				copyto = (UUtUns8 *) &def_emitterlink->emitter[itr].rate_type;
				UUmAssert((UUtUns32) (copyto - copyfrom) == (itr + 1) * sizeof(P3tEmitterLinkType));
				copysize = sizeof(P3tEmitterRateType) + sizeof(P3tEmitterPositionType) +
							  sizeof(P3tEmitterDirectionType) + sizeof(P3tEmitterSpeedType) +
							  P3cEmitterValue_NumVals * sizeof(P3tValue);

				UUrMemory_MoveOverlap(copyfrom, copyto, copysize);

				// move the data before linktype
				copyfrom = (UUtUns8 *) &def_actionmask->emitter[itr];
				copyto = (UUtUns8 *) &def_emitterlink->emitter[itr];
				UUmAssert((UUtUns32) (copyto - copyfrom) == itr * sizeof(P3tEmitterLinkType));

				copysize = (P3cParticleClassNameLength + 1) + sizeof(P3tParticleClass *) +
						sizeof(UUtUns32) + sizeof(UUtUns16) + sizeof(UUtUns16);

				UUrMemory_MoveOverlap(copyfrom, copyto, copysize);

				// set up the linktype
				def_emitterlink->emitter[itr].link_type = P3cEmitterLinkType_None;

				if (itr == 0)
					break;
			}

			// nothing before the emitters needs to be changed
		}

		(*ioDefinition)->version = P3cVersion_EmitterLink;
		// fall through to further versioning updates

	case P3cVersion_EmitterLink:
		// must move everything after 'appearance' along to make
		// room for the inserted P3tValue inside appearance

		new_size = (*ioDefinition)->definition_size + sizeof(P3tValue);
		P3iResizeParticleDefinition(ioDefinition, ioMemory, new_size, UUcFalse);

		def_emitterlink = (P3tParticleVersion_EmitterLink *) (*ioDefinition);
		def_maxcontrail = (P3tParticleVersion_MaxContrail *) (*ioDefinition);
		copyfrom = (UUtUns8 *) &def_emitterlink->appearance + sizeof(P3tAppearanceVersion_Tint);
		copyto = (UUtUns8 *) &def_maxcontrail->appearance + sizeof(P3tAppearanceVersion_MaxContrail);
		UUmAssert(copyto - copyfrom == sizeof(P3tValue));

		copysize = new_size - (copyto - ((UUtUns8 *) *ioDefinition));

		UUrMemory_MoveOverlap(copyfrom, copyto, copysize);
		UUrMemory_Clear(copyfrom, copyto - copyfrom);

		// set up the new values that we've inserted in appearance
		def_maxcontrail->appearance.max_contrail.type = P3cValueType_Float;
		def_maxcontrail->appearance.max_contrail.u.float_const.val = 0.0f;
		if (inSwapBytes) {
			P3iSwap_Value((void *) &def_maxcontrail->appearance.max_contrail, UUcTrue);
		}

		(*ioDefinition)->version = P3cVersion_MaxContrail;
		// fall through to further versioning updates

	case P3cVersion_MaxContrail:
		// every emitter now has some changed fields. we will need to
		// move around some of the variable-length arrays - therefore we must extract
		// the lengths of these arrays.
		num_variables = ((P3tParticleVersion_ActionMask *) (*ioDefinition))->num_variables;
		num_actions   = ((P3tParticleVersion_ActionMask *) (*ioDefinition))->num_actions;
		num_emitters  = ((P3tParticleVersion_ActionMask *) (*ioDefinition))->num_emitters;
		if (inSwapBytes) {
			UUrSwap_2Byte((void *) &num_variables);
			UUrSwap_2Byte((void *) &num_actions);
			UUrSwap_2Byte((void *) &num_emitters);
		}

		if (num_emitters > 0) {
			// every emitter expands by several new variables - total of 
			// 2 * sizeof(P3tEmitterOrientationType) + sizeof(float)
			new_size = (*ioDefinition)->definition_size + num_emitters
							* (2 * sizeof(P3tEmitterOrientationType) + sizeof(float));
			P3iResizeParticleDefinition(ioDefinition, ioMemory, new_size, UUcFalse);

			def_maxcontrail	  = (P3tParticleVersion_MaxContrail   *) (*ioDefinition);
			def_emitterorient = (P3tParticleVersion_EmitterOrient *) (*ioDefinition);

			// determine whereabouts the emitter data starts - in the same place in both
			def_maxcontrail->emitter = (P3tEmitter_Version7 *)	((UUtUns8 *) (*ioDefinition) +
				sizeof(P3tParticleVersion_MaxContrail) + num_variables * sizeof(P3tVariableInfo)
													  + num_actions * sizeof(P3tActionInstance));
			def_emitterorient->emitter = (P3tEmitter *)			((UUtUns8 *) (*ioDefinition) +
				sizeof(P3tParticleVersion_EmitterOrient) + num_variables * sizeof(P3tVariableInfo)
													   + num_actions * sizeof(P3tActionInstance));
			UUmAssert(((void *) def_maxcontrail->emitter) == ((void *) def_emitterorient->emitter));

			// there is no data following the emitters!

			// expand all of the emitters, in order from last to first.
			for (itr = num_emitters - 1; ; itr--) {
				// store the previous value of burst_size
				burst_size = def_maxcontrail->emitter[itr].burst_size;

				// move the data following the orientation types
				copyfrom	= (UUtUns8 *) &def_maxcontrail  ->emitter[itr].emitter_value;
				copyto		= (UUtUns8 *) &def_emitterorient->emitter[itr].emitter_value;

				UUmAssert((UUtUns32) (copyto - copyfrom) == (itr + 1) *
						(2 * sizeof(P3tEmitterOrientationType) + sizeof(float)));
				copysize = P3cEmitterValue_NumVals * sizeof(P3tValue);

				UUrMemory_MoveOverlap(copyfrom, copyto, copysize);

				// move the data between burst_size and the orientation types
				copyfrom	= (UUtUns8 *) &def_maxcontrail  ->emitter[itr].link_type;
				copyto		= (UUtUns8 *) &def_emitterorient->emitter[itr].link_type;

				UUmAssert((UUtUns32) (copyto - copyfrom) == (itr + 1) *
						(2 * sizeof(P3tEmitterOrientationType) + sizeof(float)) - 2 * sizeof(P3tEmitterOrientationType));

				copysize = sizeof(P3tEmitterLinkType) + sizeof(P3tEmitterRateType) +
					sizeof(P3tEmitterPositionType) + sizeof(P3tEmitterDirectionType) + 
					sizeof(P3tEmitterSpeedType);

				UUrMemory_MoveOverlap(copyfrom, copyto, copysize);

				// move the data before burst_size
				copyfrom	= (UUtUns8 *) &def_maxcontrail  ->emitter[itr];
				copyto		= (UUtUns8 *) &def_emitterorient->emitter[itr];
				UUmAssert((UUtUns32) (copyto - copyfrom) == itr * 
					(2 * sizeof(P3tEmitterOrientationType) + sizeof(float)));

				copysize = (P3cParticleClassNameLength + 1) + sizeof(P3tParticleClass *) +
						sizeof(UUtUns32) + sizeof(UUtUns16) + sizeof(UUtUns16);

				UUrMemory_MoveOverlap(copyfrom, copyto, copysize);

				// emit chance is always
				def_emitterorient->emitter[itr].emit_chance = UUcMaxUns16;
				if (inSwapBytes) {
					UUrSwap_2Byte((void *) &def_emitterorient->emitter[itr].emit_chance);
				}
				
				// burst size is the floating-point equivalent of what it was before
				if (inSwapBytes) {
					UUrSwap_2Byte((void *) &burst_size);
				}
				def_emitterorient->emitter[itr].burst_size = (burst_size == 0) ? 1.0f : (float) burst_size;
				if (inSwapBytes) {
					UUrSwap_4Byte((void *) &def_emitterorient->emitter[itr].burst_size);
				}

				// orientation types are standard
				def_emitterorient->emitter[itr].orientdir_type = P3cEmitterOrientation_ParentPosY;
				def_emitterorient->emitter[itr].orientup_type  = P3cEmitterOrientation_ParentPosZ;
				if (inSwapBytes) {
					UUrSwap_4Byte((void *) &def_emitterorient->emitter[itr].orientdir_type);
					UUrSwap_4Byte((void *) &def_emitterorient->emitter[itr].orientup_type);
				}

				if (itr == 0)
					break;
			}

			// nothing before the emitters needs to be changed
		}

		(*ioDefinition)->version = P3cVersion_EmitterOrient;
		// fall through to further versioning updates

	case P3cVersion_EmitterOrient:
		// must move everything after 'appearance' along to make
		// room for the inserted variables (P3tValue and two UUtUns16) inside appearance

		new_size = (*ioDefinition)->definition_size + sizeof(P3tValue) + 2 * sizeof(UUtUns16);
		P3iResizeParticleDefinition(ioDefinition, ioMemory, new_size, UUcFalse);

		def_emitterorient = (P3tParticleVersion_EmitterOrient *) (*ioDefinition);
		def_lensflaredist = (P3tParticleVersion_LensflareDist *) (*ioDefinition);
		copyfrom = (UUtUns8 *) &def_emitterorient->appearance + sizeof(P3tAppearanceVersion_MaxContrail);
		copyto   = (UUtUns8 *) &def_lensflaredist->appearance + sizeof(P3tAppearanceVersion_LensflareDist);
		UUmAssert(copyto - copyfrom == (sizeof(P3tValue) + 2 * sizeof(UUtUns16)));

		copysize = new_size - (copyto - ((UUtUns8 *) *ioDefinition));

		UUrMemory_MoveOverlap(copyfrom, copyto, copysize);
		UUrMemory_Clear(copyfrom, copyto - copyfrom);

		// set up the new values that we've inserted in appearance
		def_lensflaredist->appearance.lensflare_dist.type = P3cValueType_Float;
		def_lensflaredist->appearance.lensflare_dist.u.float_const.val = 0.0f;
		def_lensflaredist->appearance.lens_inframes = 0;
		def_lensflaredist->appearance.lens_outframes = 0;
		if (inSwapBytes) {
			P3iSwap_Value((void *) &def_lensflaredist->appearance.lensflare_dist, UUcTrue);
			UUrSwap_2Byte((void *) &def_lensflaredist->appearance.lens_inframes);
			UUrSwap_2Byte((void *) &def_lensflaredist->appearance.lens_outframes);
		}

		(*ioDefinition)->version = P3cVersion_LensflareDist;
		// fall through to further versioning updates

	case P3cVersion_LensflareDist:
		// must move everything from actionmask onwards along to make room for flags2

		new_size = (*ioDefinition)->definition_size + sizeof(UUtUns32);
		P3iResizeParticleDefinition(ioDefinition, ioMemory, new_size, UUcFalse);

		def_lensflaredist	= (P3tParticleVersion_LensflareDist *) (*ioDefinition);
		def_flags2			= (P3tParticleVersion_Flags2		*) (*ioDefinition);
		copyfrom = (UUtUns8 *) &def_lensflaredist->actionmask;
		copyto   = (UUtUns8 *) &def_flags2		 ->actionmask;
		UUmAssert(copyto - copyfrom == sizeof(UUtUns32));

		copysize = new_size - (copyto - ((UUtUns8 *) *ioDefinition));

		UUrMemory_MoveOverlap(copyfrom, copyto, copysize);
		UUrMemory_Clear(copyfrom, copyto - copyfrom);

		// set up the new values that we've inserted in appearance
		def_flags2->flags2 = 0;
		if (inSwapBytes) {
			UUrSwap_4Byte((void *) &def_flags2->flags2);
		}

		(*ioDefinition)->version = P3cVersion_Flags2;
		// fall through to further versioning updates

	case P3cVersion_Flags2:
		// must move everything after 'appearance' along to make
		// room for the two inserted UUtUns16 inside appearance

		new_size = (*ioDefinition)->definition_size + 2 * sizeof(UUtUns16);
		P3iResizeParticleDefinition(ioDefinition, ioMemory, new_size, UUcFalse);

		def_flags2 = (P3tParticleVersion_Flags2 *) (*ioDefinition);
		def_decal  = (P3tParticleVersion_Decal  *) (*ioDefinition);
		copyfrom = (UUtUns8 *) &def_flags2->appearance + sizeof(P3tAppearanceVersion_LensflareDist);
		copyto = (UUtUns8 *) &def_decal->appearance + sizeof(P3tAppearanceVersion_Decal);
		UUmAssert(copyto - copyfrom == 2 * sizeof(UUtUns16));

		copysize = new_size - (copyto - ((UUtUns8 *) *ioDefinition));

		UUrMemory_MoveOverlap(copyfrom, copyto, copysize);
		UUrMemory_Clear(copyfrom, copyto - copyfrom);

		// set up the new values that we've inserted in appearance
		def_decal->appearance.max_decals = 100;
		def_decal->appearance.decal_fadeframes = 60;
		if (inSwapBytes) {
			UUrSwap_2Byte((void *) &def_decal->appearance.max_decals);
			UUrSwap_2Byte((void *) &def_decal->appearance.decal_fadeframes);
		}

		(*ioDefinition)->version = P3cVersion_Decal;
		// fall through to further versioning updates

	case P3cVersion_Decal:
		// must move everything after the eventlists along to make
		// room for the four new eventlists

		new_size = (*ioDefinition)->definition_size + 4 * sizeof(P3tActionList);
		P3iResizeParticleDefinition(ioDefinition, ioMemory, new_size, UUcFalse);

		def_decal      = (P3tParticleVersion_Decal      *) (*ioDefinition);
		def_moreevents = (P3tParticleVersion_MoreEvents *) (*ioDefinition);
		copyfrom = (UUtUns8 *) &def_decal->lifetime;
		copyto = (UUtUns8 *) &def_moreevents->lifetime;
		UUmAssert(copyto - copyfrom == 4 * sizeof(P3tActionList));

		copysize = new_size - (copyto - ((UUtUns8 *) *ioDefinition));

		UUrMemory_MoveOverlap(copyfrom, copyto, copysize);
		UUrMemory_Clear(copyfrom, copyto - copyfrom);

		// set up the four new actionlists
		for (itr = P3cMaxNumEvents_Version0; itr < P3cMaxNumEvents; itr++) {
			def_moreevents->eventlist[itr].start_index = def_moreevents->eventlist[P3cMaxNumEvents_Version0 - 1].end_index;
			def_moreevents->eventlist[itr].end_index   = def_moreevents->eventlist[P3cMaxNumEvents_Version0 - 1].end_index;
			if (inSwapBytes) {
				UUrSwap_2Byte((void *) &def_moreevents->eventlist[itr].start_index);
				UUrSwap_2Byte((void *) &def_moreevents->eventlist[itr].end_index);
			}
		}

		(*ioDefinition)->version = P3cVersion_MoreEvents;
		// fall through to further versioning updates

	case P3cVersion_MoreEvents:
		// must move everything after appearance along to make
		// room for the new attractor structure

		new_size = (*ioDefinition)->definition_size + sizeof(P3tAttractor_Version14);
		P3iResizeParticleDefinition(ioDefinition, ioMemory, new_size, UUcFalse);

		def_moreevents = (P3tParticleVersion_MoreEvents *) (*ioDefinition);
		def_attractor  = (P3tParticleVersion_Attractor  *) (*ioDefinition);
		copyfrom = (UUtUns8 *) &def_moreevents->variable;
		copyto = (UUtUns8 *) &def_attractor->variable;
		UUmAssert(copyto - copyfrom == sizeof(P3tAttractor_Version14));

		copysize = new_size - (copyto - ((UUtUns8 *) *ioDefinition));

		UUrMemory_MoveOverlap(copyfrom, copyto, copysize);
		UUrMemory_Clear(copyfrom, copyto - copyfrom);

		// set up the new attractor structure
		def_attractor->attractor.iterator_type = P3cAttractorIterator_None;
		def_attractor->attractor.selector_type = P3cAttractorSelector_Distance;
		UUrString_Copy(def_attractor->attractor.attractor_name, "", P3cParticleClassNameLength + 1);
		def_attractor->attractor.max_distance.type = P3cValueType_Float;
		def_attractor->attractor.max_distance.u.float_const.val = 150.0f;
		def_attractor->attractor.max_angle.type = P3cValueType_Float;
		def_attractor->attractor.max_angle.u.float_const.val = 30.0f;
		
		if (inSwapBytes) {
			UUrSwap_4Byte((void *) &def_attractor->attractor.iterator_type);
			UUrSwap_4Byte((void *) &def_attractor->attractor.selector_type);
			P3iSwap_Value((void *) &def_attractor->attractor.max_distance, UUcTrue);
			P3iSwap_Value((void *) &def_attractor->attractor.max_angle, UUcTrue);
		}

		// move some flags across to the flags2 variable
		mask = ((1 << 9) - 1) << 23;
		def_attractor->flags2 |= def_attractor->flags & mask;
		def_attractor->flags &= ~mask;

		(*ioDefinition)->version = P3cVersion_Attractor;
		// fall through to further versioning updates

	case P3cVersion_Attractor:
		// must move everything after attractor along to make
		// room for the two new values inside the attractor structure

		new_size = (*ioDefinition)->definition_size + 3 * sizeof(P3tValue);
		P3iResizeParticleDefinition(ioDefinition, ioMemory, new_size, UUcFalse);

		def_attractor      = (P3tParticleVersion_Attractor      *) (*ioDefinition);
		def_attractorangle = (P3tParticleVersion_AttractorAngle *) (*ioDefinition);
		copyfrom = (UUtUns8 *) &def_attractor->variable;
		copyto = (UUtUns8 *) &def_attractorangle->variable;
		UUmAssert(copyto - copyfrom == 3 * sizeof(P3tValue));

		copysize = new_size - (copyto - ((UUtUns8 *) *ioDefinition));

		UUrMemory_MoveOverlap(copyfrom, copyto, copysize);
		UUrMemory_Clear(copyfrom, copyto - copyfrom);

		// set up the new attractor structure
		def_attractorangle->attractor.select_thetamin.type = P3cValueType_Float;
		def_attractorangle->attractor.select_thetamin.u.float_const.val = 3.0f;
		def_attractorangle->attractor.select_thetamax.type = P3cValueType_Float;
		def_attractorangle->attractor.select_thetamax.u.float_const.val = 20.0f;
		def_attractorangle->attractor.select_thetaweight.type = P3cValueType_Float;
		def_attractorangle->attractor.select_thetaweight.u.float_const.val = 3.0f;
		
		if (inSwapBytes) {
			P3iSwap_Value((void *) &def_attractorangle->attractor.select_thetamin, UUcTrue);
			P3iSwap_Value((void *) &def_attractorangle->attractor.select_thetamax, UUcTrue);
			P3iSwap_Value((void *) &def_attractorangle->attractor.select_thetaweight, UUcTrue);
		}

		(*ioDefinition)->version = P3cVersion_AttractorAngle;
		// fall through to further versioning updates

	case P3cVersion_AttractorAngle:
		// must move everything after attractor.attractor_name along to make
		// room for the new pointer inside the attractor structure

		new_size = (*ioDefinition)->definition_size + sizeof(void *);
		P3iResizeParticleDefinition(ioDefinition, ioMemory, new_size, UUcFalse);

		def_attractorangle = (P3tParticleVersion_AttractorAngle *) (*ioDefinition);
		def_attractorptr   = (P3tParticleVersion_AttractorPtr   *) (*ioDefinition);
		copyfrom = (UUtUns8 *) &def_attractorangle->attractor.attractor_name;
		copyto = (UUtUns8 *) &def_attractorptr->attractor.attractor_name;
		UUmAssert(copyto - copyfrom == sizeof(void *));

		copysize = new_size - (copyto - ((UUtUns8 *) *ioDefinition));

		UUrMemory_MoveOverlap(copyfrom, copyto, copysize);
		UUrMemory_Clear(copyfrom, copyto - copyfrom);

		// set up the new attractor structure
		def_attractorptr->attractor.attractor_ptr = NULL;
		(*ioDefinition)->version = P3cVersion_AttractorPtr;
		// fall through to further versioning updates

	case P3cVersion_AttractorPtr:
		// must move everything after 'appearance' along to make
		// room for the inserted P3tValue inside appearance

		new_size = (*ioDefinition)->definition_size + sizeof(P3tValue);
		P3iResizeParticleDefinition(ioDefinition, ioMemory, new_size, UUcFalse);

		def_attractorptr = (P3tParticleVersion_AttractorPtr *) (*ioDefinition);
		def_decalwrap    = (P3tParticleVersion_DecalWrap    *) (*ioDefinition);
		copyfrom = (UUtUns8 *) &def_attractorptr->appearance + sizeof(P3tAppearanceVersion_Decal);
		copyto   = (UUtUns8 *) &def_decalwrap->appearance + sizeof(P3tAppearanceVersion_DecalWrap);
		UUmAssert(copyto - copyfrom == sizeof(P3tValue));

		copysize = new_size - (copyto - ((UUtUns8 *) *ioDefinition));

		UUrMemory_MoveOverlap(copyfrom, copyto, copysize);
		UUrMemory_Clear(copyfrom, copyto - copyfrom);

		// set up the new values that we've inserted in appearance
		def_decalwrap->appearance.decal_wrap.type = P3cValueType_Float;
		def_decalwrap->appearance.decal_wrap.u.float_const.val = 60.0f;
		if (inSwapBytes) {
			P3iSwap_Value((void *) &def_decalwrap->appearance.decal_wrap, UUcTrue);
		}

		(*ioDefinition)->version = P3cVersion_DecalWrap;
		// fall through to further versioning updates

	case P3cVersion_DecalWrap:
		// must move everything from 'appearance' along to make
		// room for the two inserted floats and a P3tString

		new_size = (*ioDefinition)->definition_size + 2*sizeof(float) + sizeof(P3tString);
		P3iResizeParticleDefinition(ioDefinition, ioMemory, new_size, UUcFalse);

		def_decalwrap = (P3tParticleVersion_DecalWrap *) (*ioDefinition);
		def_dodge     = (P3tParticleVersion_Dodge     *) (*ioDefinition);
		copyfrom = (UUtUns8 *) &def_decalwrap->appearance;
		copyto   = (UUtUns8 *) &def_dodge->appearance;
		UUmAssert(copyto - copyfrom == (2*sizeof(float) + sizeof(P3tString)));

		copysize = new_size - (copyto - ((UUtUns8 *) *ioDefinition));

		UUrMemory_MoveOverlap(copyfrom, copyto, copysize);
		UUrMemory_Clear(copyfrom, copyto - copyfrom);

		// set up the new values that we've inserted
		def_dodge->dodge_radius = 0;
		def_dodge->alert_radius = 0;
		UUrString_Copy(def_dodge->flyby_soundname, "", P3cStringVarSize + 1);

		if (inSwapBytes) {
			UUrSwap_4Byte((void *) &def_dodge->dodge_radius);
			UUrSwap_4Byte((void *) &def_dodge->alert_radius);
		}

		(*ioDefinition)->version = P3cVersion_Dodge;
		// fall through to further versioning updates

	case P3cVersion_Current:
		// nothing more to change
		UUmAssert((*ioDefinition)->version == P3cVersion_Current);
		break;

	default:
		UUmAssert(!"unknown particle definition version!");
	}

	// store the new version
	UUmAssert((*ioDefinition)->version == P3cVersion_Current);
	if (inSwapBytes) {
		UUrSwap_2Byte((void *) &((*ioDefinition)->version));
		UUrSwap_2Byte((void *) &((*ioDefinition)->definition_size));
	}

	return UUcError_None;
}

// check that the actionlist indices are correct
static void P3iCheckActionListIndices(P3tParticleClass *inClass)
{
	UUtUns16 itr, last_index;
	P3tActionList *actionlist;

	last_index = 0;
	for (itr = 0, actionlist = inClass->definition->eventlist; itr < P3cMaxNumEvents; itr++, actionlist++) {
		if (actionlist->start_index != last_index) {
			if (actionlist->start_index == actionlist->end_index) {
				// we can move this actionlist around with impunity, it is empty
				actionlist->start_index = last_index;
				actionlist->end_index = last_index;
			} else {
				UUrPrintWarning("Particle class '%s': action list %d [%d, %d] does not match up with previous endpoint (%d)",
								inClass->classname, itr, actionlist->start_index, actionlist->end_index, last_index);

			}
		}

		last_index = actionlist->end_index;
	}
}

// read a particle class's definition from a binary file,
// performing byte-swapping as necessary.
UUtError P3rLoadParticleDefinition(const char	*inIdentifier,
									BDtData		*ioBinaryData,
									UUtBool		inSwapIt,
									UUtBool		inLocked,
									UUtBool		inAllocated)
{
	P3tParticleClass *		new_class;
	P3tParticleDefinition *	new_def;
	P3tMemoryBlock			class_mem;

	// this must be Particle3 binary data
	UUmAssert(ioBinaryData->header.class_type == P3cBinaryDataClass);

	class_mem.size	= (UUtUns16) ioBinaryData->header.data_size;
	class_mem.block = (inAllocated) ? ioBinaryData : NULL;
	new_def = (P3tParticleDefinition *) ioBinaryData->data;

	if (strlen(inIdentifier) > P3cParticleClassNameLength) {
		UUrPrintWarning("Particle class '%s' name exceeds maximum size (%d chars)",
						inIdentifier, P3cParticleClassNameLength);
		goto abort;
	}

	// FIXME: P3gClassTable should be dynamic?
	if (P3gNumClasses >= P3cMaxParticleClasses) {
		P3gOverflowedClasses++;
		goto abort;
	}

	new_class = &P3gClassTable[P3gNumClasses++];

	strcpy(new_class->classname, inIdentifier);
	new_class->read_index = P3gNumClasses;
	new_class->definition = new_def;
	new_class->non_persistent_flags = 0;
	if (inLocked) {
		new_class->non_persistent_flags |= P3cParticleClass_NonPersistentFlag_Locked;
	}
	if (inSwapIt) {
		new_class->non_persistent_flags |= P3cParticleClass_NonPersistentFlag_ByteSwap;
	}
	new_class->data_size = ioBinaryData->header.data_size;
	new_class->memory = class_mem;

	return UUcError_None;

abort:
	// bail out; particle class was not loaded
	if (class_mem.block != NULL) {
		UUrMemory_Block_Delete(class_mem.block);
	}
	return UUcError_None;
}

// find a particle class in the global particle class array
P3tParticleClass *P3rGetParticleClass(char *inIdentifier)
{
	UUtUns32 itr, base;

#if 1
	// binary search on sorted lookup table
	UUtInt32 min, max, midpoint, compare;
	P3tParticleClass *test_class;

	min = 0;
	max = P3gNumClasses - 1;

	while(min <= max) {
		midpoint = (min + max) >> 1; // S.S. / 2;

		test_class = P3gClassLookupTable[midpoint];
		compare = strcmp(inIdentifier, test_class->classname);

		if (compare == 0) {
			// found, return
			return test_class;

		} else if (compare < 0) {
			// must lie below here
			max = midpoint - 1;

		} else {
			// must lie above here
			min = midpoint + 1;
		}
	}

	// could not find the particle class in those classes that were loaded
	// initially and are covered by the lookup table. look for it in the
	// allocated section of the class table
	base = P3gAllocatedClassesIndex;
#else
	// just do a linear search on the whole class table
	base = 0;
#endif

	for (itr = base; itr < P3gNumClasses; itr++) {
		UUmAssertReadPtr(P3gClassTable[itr].definition, sizeof(P3tParticleDefinition));

		if (strcmp(inIdentifier, P3gClassTable[itr].classname) == 0) {
			return &P3gClassTable[itr];
		}
	}

	return NULL;
}

// byte-swap the particle definition
UUtError P3iSwap_ParticleDefinition(void *ioPtr, UUtBool inCurrentlyUsable)
{
	P3tParticleDefinition *definition = (P3tParticleDefinition *) ioPtr;
	UUtUns8 *dataptr;
	UUtUns16 itr;

	UUmAssertReadPtr(definition, sizeof(P3tParticleDefinition));

	if (inCurrentlyUsable == UUcTrue) {
		// swap children first, before num_actions etc get swapped
		dataptr = (UUtUns8 *) definition + sizeof(P3tParticleDefinition);

		for (itr = 0; itr < definition->num_variables; itr++) {
			P3iSwap_Variable((void *) dataptr, inCurrentlyUsable);

			dataptr += sizeof(P3tVariableInfo);
		}
		for (itr = 0; itr < definition->num_actions; itr++) {
			P3iSwap_ActionInstance((void *) dataptr, inCurrentlyUsable);

			dataptr += sizeof(P3tActionInstance);
		}
		for (itr = 0; itr < definition->num_emitters; itr++) {
			P3iSwap_Emitter((void *) dataptr, inCurrentlyUsable);

			dataptr += sizeof(P3tEmitter);
		}
	}

	// swap the definition's components
	UUrSwap_2Byte((void *) &definition->definition_size);
	UUrSwap_2Byte((void *) &definition->version);
	UUrSwap_4Byte((void *) &definition->flags);
	UUrSwap_4Byte((void *) &definition->flags2);
	UUrSwap_4Byte((void *) &definition->actionmask);
	UUrSwap_2Byte((void *) &definition->num_variables);
	UUrSwap_2Byte((void *) &definition->num_actions);
	UUrSwap_2Byte((void *) &definition->num_emitters);
	UUrSwap_2Byte((void *) &definition->max_particles);
	for (itr = 0; itr < P3cMaxNumEvents; itr++) {
		UUrSwap_2Byte((void *) &definition->eventlist[itr].start_index);
		UUrSwap_2Byte((void *) &definition->eventlist[itr].end_index);
	}

	P3iSwap_Value((void *) &definition->lifetime, inCurrentlyUsable);
	P3iSwap_Value((void *) &definition->collision_radius, inCurrentlyUsable);
	
	UUrSwap_4Byte((void *) &definition->dodge_radius);
	UUrSwap_4Byte((void *) &definition->alert_radius);

	// swap the particle's appearance
	P3iSwap_Value((void *) &definition->appearance.scale, inCurrentlyUsable);
	P3iSwap_Value((void *) &definition->appearance.y_scale, inCurrentlyUsable);
	P3iSwap_Value((void *) &definition->appearance.rotation, inCurrentlyUsable);
	P3iSwap_Value((void *) &definition->appearance.alpha, inCurrentlyUsable);
	P3iSwap_Value((void *) &definition->appearance.x_offset, inCurrentlyUsable);
	P3iSwap_Value((void *) &definition->appearance.x_shorten, inCurrentlyUsable);
	P3iSwap_Value((void *) &definition->appearance.tint, inCurrentlyUsable);
	P3iSwap_Value((void *) &definition->appearance.edgefade_min, inCurrentlyUsable);
	P3iSwap_Value((void *) &definition->appearance.edgefade_max, inCurrentlyUsable);
	P3iSwap_Value((void *) &definition->appearance.max_contrail, inCurrentlyUsable);
	P3iSwap_Value((void *) &definition->appearance.lensflare_dist, inCurrentlyUsable);
	UUrSwap_2Byte((void *) &definition->appearance.lens_inframes);
	UUrSwap_2Byte((void *) &definition->appearance.lens_outframes);
	UUrSwap_2Byte((void *) &definition->appearance.max_decals);
	UUrSwap_2Byte((void *) &definition->appearance.decal_fadeframes);
	P3iSwap_Value((void *) &definition->appearance.decal_wrap, inCurrentlyUsable);

	UUrSwap_4Byte((void *) &definition->attractor.iterator_type);
	UUrSwap_4Byte((void *) &definition->attractor.selector_type);
	P3iSwap_Value((void *) &definition->attractor.max_distance, inCurrentlyUsable);
	P3iSwap_Value((void *) &definition->attractor.max_angle, inCurrentlyUsable);
	P3iSwap_Value((void *) &definition->attractor.select_thetamin, inCurrentlyUsable);
	P3iSwap_Value((void *) &definition->attractor.select_thetamax, inCurrentlyUsable);
	P3iSwap_Value((void *) &definition->attractor.select_thetaweight, inCurrentlyUsable);

	if (inCurrentlyUsable == UUcFalse) {
		// it is now safe to swap children
		dataptr = (UUtUns8 *) definition + sizeof(P3tParticleDefinition);

		for (itr = 0; itr < definition->num_variables; itr++) {
			P3iSwap_Variable((void *) dataptr, inCurrentlyUsable);

			dataptr += sizeof(P3tVariableInfo);
		}
		for (itr = 0; itr < definition->num_actions; itr++) {
			P3iSwap_ActionInstance((void *) dataptr, inCurrentlyUsable);

			dataptr += sizeof(P3tActionInstance);
		}
		for (itr = 0; itr < definition->num_emitters; itr++) {
			P3iSwap_Emitter((void *) dataptr, inCurrentlyUsable);

			dataptr += sizeof(P3tEmitter);
		}
	}

	return UUcError_None;
}

// byte-swap a particle class's variable definition
UUtError P3iSwap_Variable(void *ioPtr, UUtBool inCurrentlyUsable)
{
	P3tVariableInfo *	varinfo = (P3tVariableInfo *) ioPtr;

	UUmAssertReadPtr(varinfo, sizeof(P3tVariableInfo));

	UUrSwap_4Byte((void *) &varinfo->type);
	P3iSwap_Value((void *) &varinfo->initialiser, inCurrentlyUsable);

	return UUcError_None;
}

// byte-swap a variable reference
UUtError P3iSwap_VarRef(void *ioPtr, UUtBool inCurrentlyUsable)
{
	P3tVariableReference *		varref = (P3tVariableReference *) ioPtr;
	
	UUrSwap_4Byte((void *) &varref->type);
	
	return UUcError_None;
}

// byte-swap a particle class's action instance
UUtError P3iSwap_ActionInstance(void *ioPtr, UUtBool inCurrentlyUsable)
{
	P3tActionInstance *			action = (P3tActionInstance *) ioPtr;
	const P3tActionTemplate *	templateptr;
	UUtUns16					itr;

	UUmAssertReadPtr(action, sizeof(P3tActionInstance));

	UUrSwap_4Byte((void *) &action->action_type);

#if defined(DEBUGGING) && DEBUGGING
	if ((action->action_type < 0) || (action->action_type >= P3cNumActionTypes)) {
		UUmError_ReturnOnErrorMsgP(UUcError_Generic, "Particle action type %d outside allowable range (0-%d)",
									action->action_type, P3cNumActionTypes, 0);
	}
#endif

	templateptr = &P3gActionInfo[action->action_type];

	// variable references have nothing to byte-swap, they are
	// just a name when loaded
	for (itr = 0; itr < templateptr->num_parameters; itr++) {
		P3iSwap_Value(&action->action_value[itr], inCurrentlyUsable);
	}

	for (itr = 0; itr < templateptr->num_variables; itr++) {
		P3iSwap_VarRef(&action->action_var[itr], inCurrentlyUsable);
	}

	return UUcError_None;
}

// byte-swap an emitter
UUtError P3iSwap_Emitter(void *ioPtr, UUtBool inCurrentlyUsable)
{
	UUtUns16 itr;
	P3tEmitter *emitter = (P3tEmitter *) ioPtr;
	UUmAssertReadPtr(emitter, sizeof(P3tEmitter));

	UUrSwap_4Byte((void *) &emitter->flags);
	UUrSwap_2Byte((void *) &emitter->particle_limit);
	UUrSwap_2Byte((void *) &emitter->emit_chance);
	UUrSwap_4Byte((void *) &emitter->burst_size);
	UUrSwap_4Byte((void *) &emitter->link_type);

	UUrSwap_4Byte((void *) &emitter->rate_type);
	UUrSwap_4Byte((void *) &emitter->position_type);
	UUrSwap_4Byte((void *) &emitter->direction_type);
	UUrSwap_4Byte((void *) &emitter->speed_type);
	UUrSwap_4Byte((void *) &emitter->orientdir_type);
	UUrSwap_4Byte((void *) &emitter->orientup_type);

	for (itr = 0; itr < P3cEmitterValue_NumVals; itr++) {
		P3iSwap_Value((void *) &emitter->emitter_value[itr], inCurrentlyUsable);
	}

	return UUcError_None;
}

// byte-swap an effect specification
UUtError P3iSwap_EffectSpecification(void *ioPtr, UUtBool inCurrentlyUsable)
{
	P3tEffectSpecification *effect_spec = (P3tEffectSpecification *) ioPtr;
	UUmAssertReadPtr(effect_spec, sizeof(P3tEffectSpecification));

	UUrSwap_4Byte((void *) &effect_spec->collision_orientation);

	if (!inCurrentlyUsable) {
		// swap before we access the value
		UUrSwap_4Byte((void *) &effect_spec->location_type);
	}
	switch(effect_spec->location_type) {
		case P3cEffectLocationType_Default:
		case P3cEffectLocationType_AttachedToObject: 
			// nothing to swap
			break;

		case P3cEffectLocationType_CollisionOffset:
			UUrSwap_4Byte((void *) &effect_spec->location_data.collisionoffset.offset);
			break;

		case P3cEffectLocationType_FindWall:
			// look_along_orientation is a 1-byte UUtBool and doesn't require swapping
			UUrSwap_4Byte((void *) &effect_spec->location_data.findwall.radius);
			break;

		case P3cEffectLocationType_Decal: // S.S. no clue if this is correct ahem cough cough 
			break;
		
		default:
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "P3iSwap_EffectSpecification: unknown location type");
			break;
	}
	if (inCurrentlyUsable) {
		// swap after we access the value
		UUrSwap_4Byte((void *) &effect_spec->location_type);
	}

	return UUcError_None;
}

// byte-swap a particle class's value
UUtError P3iSwap_Value(void *ioPtr, UUtBool inCurrentlyUsable)
{
	UUtError		error;
	P3tValue *		value = (P3tValue *) ioPtr;

	UUmAssertReadPtr(value, sizeof(P3tValue));

	if (inCurrentlyUsable == UUcFalse) {
		// must swap type before it can be used for indexing
		UUrSwap_4Byte((void *) &value->type);
	}

	switch(value->type) {
	case P3cValueType_Variable:
		error = P3iSwap_VarRef((void *) &value->u.variable, inCurrentlyUsable);
		UUmError_ReturnOnError(error);
		break;

	case P3cValueType_Integer:
		UUrSwap_2Byte((void *) &value->u.integer_const.val);
		break;

	case P3cValueType_IntegerRange:
		UUrSwap_2Byte((void *) &value->u.integer_range.val_low);
		UUrSwap_2Byte((void *) &value->u.integer_range.val_hi);
		break;

	case P3cValueType_Float:
		UUrSwap_4Byte((void *) &value->u.float_const.val);
		break;

	case P3cValueType_FloatRange:
		UUrSwap_4Byte((void *) &value->u.float_range.val_low);
		UUrSwap_4Byte((void *) &value->u.float_range.val_hi);
		break;

	case P3cValueType_FloatBellcurve:
		UUrSwap_4Byte((void *) &value->u.float_bellcurve.val_mean);
		UUrSwap_4Byte((void *) &value->u.float_bellcurve.val_stddev);
		break;

	case P3cValueType_FloatTimeBased:
		UUrSwap_4Byte((void *) &value->u.float_timebased.cycle_length);
		UUrSwap_4Byte((void *) &value->u.float_timebased.cycle_max);
		break;

	case P3cValueType_String:
		// nothing to swap
		break;

	case P3cValueType_Shade:
		UUrSwap_4Byte((void *) &value->u.shade_const.val);
		break;

	case P3cValueType_ShadeRange:
		UUrSwap_4Byte((void *) &value->u.shade_range.val_low);
		UUrSwap_4Byte((void *) &value->u.shade_range.val_hi);
		break;

	case P3cValueType_ShadeBellcurve:
		UUrSwap_4Byte((void *) &value->u.shade_bellcurve.val_mean);
		UUrSwap_4Byte((void *) &value->u.shade_bellcurve.val_stddev);
		break;

	case P3cValueType_Enum:
		UUrSwap_4Byte((void *) &value->u.enum_const.val);
		break;
		
	default:
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "P3iSwap_Value: unknown patricle type");
	}

	if (inCurrentlyUsable == UUcTrue) {
		// must swap type _after_ it is used for indexing
		UUrSwap_4Byte((void *) &value->type);
	}

	return UUcError_None;
}

// get the size of a variable
UUtUns16 P3iGetDataSize(P3tDataType inDataType)
{
	switch(inDataType) {
	case P3cDataType_Integer:
		return sizeof(UUtUns16);

	case P3cDataType_Float:
		return sizeof(float);

	case P3cDataType_String:
	case P3cDataType_Sound_Ambient:
	case P3cDataType_Sound_Impulse:
		return sizeof(P3tString);

	case P3cDataType_Shade:
		return sizeof(IMtShade);

	default:
		if (inDataType & P3cDataType_Enum) {
			return sizeof(UUtUns32);		// enums are 32-bit
		} else {
			return 0;
		}
	}
}

// Find the location of a variable in a particle, from its name
UUtError P3iFindVariable(P3tParticleClass *inParticleClass, P3tVariableReference *ioVarRef)
{
	UUtUns16	itr;
	P3tParticleDefinition *def = inParticleClass->definition;

	UUmAssertReadPtr(ioVarRef, sizeof(P3tVariableReference));

	if (ioVarRef->name[0] == '\0') {
		ioVarRef->offset = P3cNoVar;
		ioVarRef->type = P3cDataType_None;
		return UUcError_None;
	}

	for (itr = 0; itr < def->num_variables; itr++) {
		if (strcmp(def->variable[itr].name, ioVarRef->name) == 0) {
			ioVarRef->offset	= def->variable[itr].offset;
			ioVarRef->type		= def->variable[itr].type;
			return UUcError_None;
		}
	}

	// Can't find the variable.
	return UUcError_Generic;
}

/*
 * particle update
 */

// traverse any chains of position-locked particles, stored in user_ref.
M3tPoint3D *P3rGetPositionPtr(P3tParticleClass *inClass, P3tParticle *inParticle, UUtBool *inDead)
{
	P3tParticle *cur_particle, *next_particle;
	P3tParticleClass *cur_class, *next_class;
//	M3tVector3D *offsetpos;
//	M3tMatrix4x3 *dynmatrix;
//	UUtUns16 offsetpos_offset, dynmatrix_offset;
	UUtUns32 ref;

	cur_particle = inParticle;
	cur_class = inClass;
	*inDead = UUcFalse;

	// iterate up the chain of locked particles as necessary
	while (cur_class->definition->flags2 & P3cParticleClassFlag2_LockToLinkedParticle) {
		ref = cur_particle->header.user_ref;

		if (ref == P3cParticleReference_Null) {
			// we can't go any further down the chain - stop
			break;
		}

		P3mUnpackParticleReference(ref, next_class, next_particle);

		if ((next_particle->header.self_ref != ref) || (next_particle->header.flags & P3cParticleFlag_BreakLinksToMe)) {
			// the link is dead - break it. the buck stops here.
			cur_particle->header.user_ref = P3cParticleReference_Null;

			// if we are currently in the middle of processing a collision event
			// then it's likely that we should set our position to the location of the
			// collision! this is rather ugly but necessary, alas
			if (P3gCurrentCollision.collided) {
				MUmVector_Copy(inParticle->header.position, P3gCurrentCollision.hit_point);
			}
			
			*inDead = P3rSendEvent(cur_class, cur_particle, P3cEvent_BrokenLink, cur_particle->header.current_time);
			break;
		}

		cur_particle = next_particle;
		cur_class = next_class;
	}

	if (!(*inDead) && (cur_particle != inParticle)) {
		// lock the in particle to the position of the particle that we found at the end
		// of the chain
		P3rGetRealPosition(cur_class, cur_particle, &inParticle->header.position);
	}

	return &inParticle->header.position;
}

// this gets the real position of a particle, taking everything into account. returns is_dead.
UUtBool P3rGetRealPosition(P3tParticleClass *inClass, P3tParticle *inParticle, M3tPoint3D *outPosition)
{
	M3tPoint3D *p_position, *p_offset;
	M3tMatrix4x3 **p_dynmatrix, *dynmatrix;
	UUtBool is_dead;

	p_position = P3rGetPositionPtr(inClass, inParticle, &is_dead);
	if (is_dead)
		return UUcTrue;
			
	p_offset = P3rGetOffsetPosPtr(inClass, inParticle);
	p_dynmatrix = P3rGetDynamicMatrixPtr(inClass, inParticle);
	dynmatrix = (p_dynmatrix == NULL) ? NULL : *p_dynmatrix;

	if (p_offset == NULL) {
		if (dynmatrix == NULL) {
			MUmVector_Copy(*outPosition, *p_position);
		} else {
			MUrMatrix_MultiplyPoint(p_position, dynmatrix, outPosition);
		}
	} else {
		MUmVector_Add(*outPosition, *p_offset, *p_position);

		if (dynmatrix != NULL) {
			MUrMatrix_MultiplyPoint(outPosition, dynmatrix, outPosition);
		}
	}

	return UUcFalse;
}

// this gets the velocity by following lock-to-link references, if there are any
M3tVector3D *P3rGetRealVelocityPtr(P3tParticleClass *inClass, P3tParticle *inParticle)
{
	P3tParticle *cur_particle, *next_particle;
	P3tParticleClass *cur_class, *next_class;
	UUtUns16 vel_offset;
	UUtUns32 ref;

	cur_particle = inParticle;
	cur_class = inClass;

	// iterate up the chain of locked particles as necessary
	while (cur_class->definition->flags2 & P3cParticleClassFlag2_LockToLinkedParticle) {
		ref = cur_particle->header.user_ref;

		if (ref == P3cParticleReference_Null) {
			// we can't go any further down the chain - stop
			break;
		}

		P3mUnpackParticleReference(ref, next_class, next_particle);

		if ((next_particle->header.self_ref != ref) || (next_particle->header.flags & P3cParticleFlag_BreakLinksToMe)) {
			// the link is dead. it's not our responsibility to break it - P3rGetPositionPtr does that.
			break;
		}

		if ((next_class->definition->flags & P3cParticleClassFlag_HasVelocity) == 0) {
			break;
		}

		cur_particle = next_particle;
		cur_class = next_class;
	}

	if (cur_class->definition->flags & P3cParticleClassFlag_HasVelocity) {
		vel_offset = cur_class->optionalvar_offset[P3cParticleOptionalVar_Velocity];
		UUmAssert((vel_offset >= 0) && 
				(vel_offset < (UUtUns16) (cur_class->particle_size - sizeof(P3tParticleHeader))));

		return (M3tVector3D *) &cur_particle->vararray[vel_offset];
	} else {
		return NULL;
	}
}

// does a value depend on which particle is evaluating it?
UUtBool P3iValueIsDynamic(P3tValue *inValue)
{
	switch(inValue->type) {
	case P3cValueType_Integer:
	case P3cValueType_Float:
	case P3cValueType_Shade:
	case P3cValueType_String:
	case P3cValueType_FloatTimeBased:
		return UUcFalse;

	default:
		if (inValue->type & P3cValueType_Enum)
			return UUcFalse;
		else
			return UUcTrue;
	}
}

// calculates random values normally distributed about 0, std deviation 1, range ~ +/- 3
float P3rNormalDistribution(void)
{
	float dist_val, interp_val, final_val;
	UUtUns16 index;

	dist_val = 0.999f * P3mSignedRandom();
	interp_val = P3gInverseNormalTableSize * (float)fabs(dist_val);
	index = (UUtUns16) interp_val;
	interp_val -= index;

	UUmAssert((index >= 0) && (index < P3gInverseNormalTableSize));
	final_val = (1 - interp_val) * P3gInverseNormalTable[index] + interp_val * P3gInverseNormalTable[index + 1];

	if (dist_val < 0)
		final_val = -final_val;

	return final_val;
}

// calculate the value of a variable from the stored rule
UUtBool P3iCalculateValue(P3tParticle *inParticle, P3tValue *inValueDef,
						   void *outStore)
{
	UUtUns16 offset;
	P3tParticleClass *particle_class;
	void *var_read;
	float random_val, a, r, g, b, cycle_val;
	IMtShade mean_shade, stddev_shade;

#if PARTICLE_PERF_ENABLE
	if (inParticle != NULL) {
		P3tParticleClass *classptr;

		UUmAssert((inParticle->header.class_index >= 0) && (inParticle->header.class_index < P3gNumClasses));
		classptr = &P3gClassTable[inParticle->header.class_index];
		P3mPerfTrack(classptr, P3cPerfEvent_VarEval);
	}
#endif

	switch(inValueDef->type) {

	// copy the value from a variable of the particle
	case P3cValueType_Variable:
		// check that this is a valid variable
		UUmAssert(inParticle != NULL);
		offset = inValueDef->u.variable.offset;

		if (offset == P3cNoVar)
			return UUcFalse;	// a NULL variable reference

		particle_class = &P3gClassTable[inParticle->header.class_index];
		if (offset + sizeof(P3tParticleHeader) >= particle_class->particle_size)
			return UUcFalse;

#if defined(DEBUGGING) && DEBUGGING
		{
			UUtUns32 itr;

			// check that this corresponds to an actual variable
			for (itr = 0; itr < particle_class->definition->num_variables; itr++) {
				if (particle_class->definition->variable[itr].offset == offset)
					break;
			}

			if (itr >= particle_class->definition->num_variables) {
				// this isn't a valid variable offset
				UUmAssert(0);
			}
		}
#endif

		var_read = (void *) &inParticle->vararray[offset];

		switch(inValueDef->u.variable.type) {
		case P3cDataType_Float:
			*((float *) outStore) = *((float *) var_read);
			break;

		case P3cDataType_Shade:
			*((IMtShade *) outStore) = *((IMtShade *) var_read);
			break;

		case P3cDataType_String:
		case P3cDataType_Sound_Ambient:
		case P3cDataType_Sound_Impulse:
			*((char **) outStore) = (char *) var_read;	// unlike the others, copy the pointer, not the value
			break;

		default:
			UUmAssert(inValueDef->u.variable.type & P3cDataType_Enum);
			*((UUtUns32 *) outStore) = *((UUtUns32 *) var_read);
			break;
		}
		break;

	case P3cValueType_Integer:
	case P3cValueType_IntegerRange:
		UUmAssert(!"particle integer values are now deprecated");
		break;

	// constant floating point value
	case P3cValueType_Float:
		*((float *) outStore) = inValueDef->u.float_const.val;
		break;

	// floating point value randomly distributed in some range
	case P3cValueType_FloatRange:
		*((float *) outStore) = inValueDef->u.float_range.val_low + P3rRandom() *
			(inValueDef->u.float_range.val_hi - inValueDef->u.float_range.val_low);
		break;

	// floating point value normally distributed
	case P3cValueType_FloatBellcurve:
		*((float *) outStore) = inValueDef->u.float_bellcurve.val_mean + P3rNormalDistribution() *
			inValueDef->u.float_bellcurve.val_stddev;
		break;

	// constant string
	case P3cValueType_String:
		*((char **) outStore) = inValueDef->u.string_const.val;
		break;

	// constant shade
	case P3cValueType_Shade:
		*((IMtShade *) outStore) = inValueDef->u.shade_const.val;
		break;

	// shade randomly distributed in some range
	case P3cValueType_ShadeRange:
		*((IMtShade *) outStore) = IMrShade_Interpolate(inValueDef->u.shade_range.val_low,
														inValueDef->u.shade_range.val_hi,
														P3rRandom());
		break;

	// shade normally distributed
	case P3cValueType_ShadeBellcurve:
		random_val = P3rNormalDistribution();
		mean_shade = inValueDef->u.shade_bellcurve.val_mean;
		stddev_shade = inValueDef->u.shade_bellcurve.val_stddev;

		a = ((mean_shade & 0xFF000000) >> 24) + random_val * ((stddev_shade & 0xFF000000) >> 24);
		r = ((mean_shade & 0x00FF0000) >> 16) + random_val * ((stddev_shade & 0x00FF0000) >> 16);
		g = ((mean_shade & 0x0000FF00) >>  8) + random_val * ((stddev_shade & 0x0000FF00) >>  8);
		b = ((mean_shade & 0x000000FF)      ) + random_val * ((stddev_shade & 0x000000FF)      );

		*((IMtShade *) outStore) = (MUrUnsignedSmallFloat_To_Uns_Round(a) << 24) |
								   (MUrUnsignedSmallFloat_To_Uns_Round(r) << 16) |
								   (MUrUnsignedSmallFloat_To_Uns_Round(g) <<  8) |
								    MUrUnsignedSmallFloat_To_Uns_Round(b);
		break;

	// constant enumerated type
	case P3cValueType_Enum:
		*((UUtUns32 *) outStore) = inValueDef->u.enum_const.val;
		break;
	
	// floating point value that cycles over gametime
	case P3cValueType_FloatTimeBased:
		cycle_val = P3gApproxCurrentTime / inValueDef->u.float_timebased.cycle_length;
		cycle_val -= (float)floor(cycle_val);
		*((float *) outStore) = cycle_val * inValueDef->u.float_timebased.cycle_max;
		break;

	default:
		UUmAssert(!"unknown particle value type!");
	}

	return UUcTrue;
}

// perform a list of actions on a particle, for a given time period.
// if the particle dies, returns UUcTrue
static UUtBool P3iPerformActionList(P3tParticleClass *inClass, P3tParticle *inParticle,
							UUtUns16 inEvent, P3tActionList *inList, float inCurrentT, float inDeltaT,
							UUtBool inExpectCollision)
{
	UUtBool is_dead, stop_actions;
	UUtUns16 itr, itr2, offset;
	UUtUns32 mask, flagmask;
	void *action_varptr[P3cActionMaxParameters];
	P3tActionInstance *action;
	P3tActionTemplate *act_template;

	UUmAssertReadPtr(inClass, sizeof(P3tParticleClass));

	UUmAssert(inList->start_index >= 0);
	UUmAssert(inList->end_index <= inClass->definition->num_actions);

	// run all actions unless we're the update list
	mask = (inEvent == P3cEvent_Update) ? inParticle->header.actionmask : 0;

	// what templates do we not call?
	flagmask = 0;
	if (inParticle->header.flags & P3cParticleFlag_Stopped)
		flagmask |= P3cActionTemplate_NotIfStopped;
	if (inParticle->header.flags & P3cParticleFlag_Chop)
		flagmask |= P3cActionTemplate_NotIfChopped;
	if (inExpectCollision)
		flagmask |= P3cActionTemplate_NotIfCollision;

	stop_actions = UUcFalse;
	for (itr = inList->start_index, action = &inClass->definition->action[itr];
		 itr < inList->end_index; itr++, action++) {

		// if this is the update eventlist, don't run actions that are turned off for this
		// particle
		if (mask & (1 << itr))
			continue;

		// work out what kind of action this is
		UUmAssert((action->action_type >= 0) && (action->action_type < P3cNumActionTypes));
		act_template = &P3gActionInfo[action->action_type];
		UUmAssert(act_template->functionproc != NULL);		// action must not be a divider or blank

		// don't run actions of types that are currently disallowed
		if (flagmask & act_template->flags)
			continue;

		P3mPerfTrack(inClass, P3cPerfEvent_Action);

		// construct the pointers for the variables
		for (itr2 = 0; itr2 < act_template->num_variables; itr2++) {
			offset = action->action_var[itr2].offset;
			action_varptr[itr2] = (offset == P3cNoVar) ? NULL : &inParticle->vararray[offset];
		}

		// call the action
		if (act_template->functionproc != NULL) {
			is_dead = act_template->functionproc(inClass, inParticle, action, action_varptr,
														inCurrentT, inDeltaT, &stop_actions);
			if (is_dead) {
				return UUcTrue;
			}
		}

		if (stop_actions)
			break;
	}

	return UUcFalse;
}

// send an event to a particle
UUtBool P3rSendEvent(P3tParticleClass *inClass, P3tParticle *inParticle, UUtUns16 inEventIndex, float inTime)
{
	UUtBool is_dead;

	UUmAssert((inEventIndex >= 0) && (inEventIndex < P3cMaxNumEvents));

	if (inEventIndex == P3cEvent_Create) {
		// the particle is finished its setup phase, perform pre-update calculations
		P3rFindAttractor(inClass, inParticle, 0);
	}

	is_dead = P3iPerformActionList(inClass, inParticle, inEventIndex,
								&inClass->definition->eventlist[inEventIndex],
								 inTime, 0, UUcFalse);

	if ((inEventIndex == P3cEvent_Create) && (P3gParticleCreationCallback != NULL) && (!is_dead)) {
		P3gParticleCreationCallback(inClass, inParticle);
	}

	return is_dead;
}

// check to see if a particle must emit another particle before a given time
static void P3iCheckEmitter(P3tParticleClass *inClass, P3tParticle *inParticle,
					 UUtUns16 inEmitterNum, P3tParticleEmitterData *inEmitterData, float inCurrentTime,
					 UUtBool *outEmit, UUtUns16 *outEmitterNum, float *ioEndTime)
{
	P3tEmitter *emitter;
	M3tPoint3D *p_position, *emitter_position, calculated_emitter_position, calculated_lastparticle_position;
	M3tVector3D *p_offset, delta_position;
	M3tMatrix4x3 **p_dynmatrix;
	P3tParticleClass *last_class;
	P3tParticle *last_particle;
	UUtBool emit_now, is_dead;
	UUtUns32 ref, check_ticks;
	float emit_rate, emit_maxrate, next_emit_time, next_emit_maxtime, t_base, threshold, dist_sq;
	float recharge_time, check_attractor_time;

	UUmAssert((inEmitterNum >= 0) && (inEmitterNum < inClass->definition->num_emitters));
	emitter = &inClass->definition->emitter[inEmitterNum];

	emit_now = UUcFalse;
	if ((inEmitterData->time_last_emission == P3cEmitImmediately) && (*ioEndTime > inCurrentTime)) {
		if ((emitter->rate_type == P3cEmitterRate_Attractor) || (emitter->rate_type == P3cEmitterRate_Random)) {
			// we don't obey the emit-immediately rules
			inEmitterData->time_last_emission = 0;
		} else {
			emit_now = UUcTrue;
		}
	}

	switch(emitter->rate_type) {
	case P3cEmitterRate_Continuous:
		// calculate our rate of emission
		if (P3iCalculateValue(inParticle, &emitter->emitter_value[P3cEmitterValue_RateValOffset + 0],
								(void *) &emit_rate) != UUcTrue)
			break;

		// work out when we will next emit
		next_emit_time = inEmitterData->time_last_emission + emit_rate;
		if (next_emit_time < inCurrentTime) {
			next_emit_time = inCurrentTime;
		}

		if (next_emit_time < *ioEndTime) {
			*outEmit = UUcTrue;
			*outEmitterNum = inEmitterNum;
			*ioEndTime = next_emit_time;
		}
		break;

	case P3cEmitterRate_Attractor:
		// calculate our recharge time
		if ((P3iCalculateValue(inParticle, &emitter->emitter_value[P3cEmitterValue_RateValOffset + 0],
								(void *) &recharge_time) != UUcTrue) ||
			(P3iCalculateValue(inParticle, &emitter->emitter_value[P3cEmitterValue_RateValOffset + 1],
								(void *) &check_attractor_time) != UUcTrue))
			break;

		// calculate when we could next emit
		next_emit_time = inEmitterData->time_last_emission + recharge_time;
		if (next_emit_time < inCurrentTime) {
			next_emit_time = inCurrentTime;
		}

		if (next_emit_time >= *ioEndTime) {
			// we are recharging and cannot emit now
			break;
		}
		
		// find attractors nearby, and store them in the global buffer
		check_ticks = MUrUnsignedSmallFloat_To_Uns_Round(check_attractor_time * UUcFramesPerSecond);
		P3rIterateAttractors(inClass, inParticle, check_ticks);
		emit_now = (P3gAttractorBuf_Valid && (P3gAttractorBuf_NumAttractors > 0));
		break;

	case P3cEmitterRate_Random:
		// calculate our min and max rates of emission
		if ((P3iCalculateValue(inParticle, &emitter->emitter_value[P3cEmitterValue_RateValOffset + 0],
								(void *) &emit_rate) != UUcTrue) ||
			(P3iCalculateValue(inParticle, &emitter->emitter_value[P3cEmitterValue_RateValOffset + 1],
								(void *) &emit_maxrate) != UUcTrue))
			break;

		// calculate the interval of next emission
		next_emit_time    = inEmitterData->time_last_emission + emit_rate;

		if (emit_maxrate > emit_rate) {
			// we will be emitting somewhere in an interval.
			next_emit_maxtime = inEmitterData->time_last_emission + emit_maxrate;

			if (*ioEndTime < next_emit_time) {
				// the emission interval doesn't start until after our end time.
				return;
			} else if (inCurrentTime >= next_emit_maxtime) {
				// we have passed the end of the emission interval and must emit now
				*outEmit = UUcTrue;
				*outEmitterNum = inEmitterNum;
				*ioEndTime = inCurrentTime;
				return;
			} else if (inCurrentTime > next_emit_time) {
				// we are already inside this time period and so the random
				// emission time will be biased (as it must be after the current time)
				t_base = inCurrentTime;
			} else {
				// the random emission time is not biased
				t_base = next_emit_time;
			}

			// calculate the random emission time
			next_emit_time = t_base + (next_emit_maxtime - t_base) * P3rRandom();
		} else {
			// the input parameters are improperly formed. just treat this as a continuous
			// emission at the minimum interval rate.
		}

		next_emit_time = UUmMax(inCurrentTime, next_emit_time);

		if (next_emit_time < *ioEndTime) {
			*outEmit = UUcTrue;
			*outEmitterNum = inEmitterNum;
			*ioEndTime = next_emit_time;
		}
		break;

	case P3cEmitterRate_Distance:
		if (P3iCalculateValue(inParticle, &emitter->emitter_value[P3cEmitterValue_RateValOffset + 0],
								(void *) &threshold) != UUcTrue)
			break;
		
		ref = inEmitterData->last_particle;
		if (ref == P3cParticleReference_Null) {
			// we have not emitted a particle, emit one now
			emit_now = UUcTrue;

		} else {
			// calculate emitter's current position
			emitter_position = P3rGetPositionPtr(inClass, inParticle, &is_dead);
			if (is_dead) {
				break;
			}
			
			p_offset = P3rGetOffsetPosPtr(inClass, inParticle);
			if (p_offset != NULL) {
				MUmVector_Add(calculated_emitter_position, *emitter_position, *p_offset);
				emitter_position = &calculated_emitter_position;
			}

			p_dynmatrix = P3rGetDynamicMatrixPtr(inClass, inParticle);
			if (p_dynmatrix != NULL) {
				MUrMatrix_MultiplyPoint(emitter_position, *p_dynmatrix, &calculated_emitter_position);
				emitter_position = &calculated_emitter_position;
			}

			// calculate last emitted particle's current position
			P3mUnpackParticleReference(ref, last_class, last_particle);
			if ((last_particle->header.self_ref != ref) || (last_particle->header.flags & P3cParticleFlag_BreakLinksToMe)) {
				break;
			}

			p_position = P3rGetPositionPtr(last_class, last_particle, &is_dead);
			if (is_dead) {
				break;
			}
			
			p_offset = P3rGetOffsetPosPtr(last_class, last_particle);
			if (p_offset != NULL) {
				MUmVector_Add(calculated_lastparticle_position, *p_position, *p_offset);
				p_position = &calculated_lastparticle_position;
			}

			p_dynmatrix = P3rGetDynamicMatrixPtr(last_class, last_particle);
			if (p_dynmatrix != NULL) {
				MUrMatrix_MultiplyPoint(p_position, *p_dynmatrix, &calculated_lastparticle_position);
				p_position = &calculated_lastparticle_position;
			}

			// is the particle far enough away from the emitter that we should emit again?
			MUmVector_Subtract(delta_position, *p_position, *emitter_position);
			dist_sq = MUmVector_GetLengthSquared(delta_position);

			if (dist_sq > UUmSQR(threshold)) {
				emit_now = UUcTrue;
			}
		}

		break;

	case P3cEmitterRate_Instant:
		emit_now = UUcTrue;
		break;

	default:
		UUmAssert(!"unknown particle emitter rate type!");
	}

	if (emit_now && (*outEmit != UUcTrue)) {
		// emit 'immediately' - maximum of 100 particles per tick
		next_emit_time = inCurrentTime + 0.01f / UUcFramesPerSecond;

		if (next_emit_time < *ioEndTime) {
			*outEmit = UUcTrue;
			*outEmitterNum = inEmitterNum;
			*ioEndTime = next_emit_time;
		}
	}
}

// get a particle direction somewhere within a cone
static void P3iConicalDirection(float inAngle, float inCenterBias, float *outCharacter, M3tVector3D *outVector)
{
	float character, theta, phi, sinphi;

	character = P3rRandom();
	if (inCenterBias > 0) {
		// use a power function to bias towards zero
		character = (float) pow(character, inCenterBias + 1.0f);
	} else if (inCenterBias < 0) {
		// use a power function to bias away from zero and towards one
		character = (float) pow(character, 1.0f / (1.0f - inCenterBias));
	}

	phi = character * inAngle * 0.01745329252f; // S.S. M3c2Pi / 360.0f;
	UUmTrig_Clip(phi);

	sinphi = MUrSin(phi);

	theta = M3c2Pi * P3rRandom();

	*outCharacter = character;
	MUmVector_Set(*outVector, MUrCos(theta) * sinphi, MUrCos(phi), MUrSin(theta) * sinphi);
}

// emit a particle or burst of particles
static UUtBool P3iEmitParticle(P3tParticleClass *inClass, P3tParticle *inParticle, P3tEmitter *inEmitter,
							UUtUns16 inEmitterIndex, float inEmitTime)
{
	UUtUns16					itr, num_particles, num_per_attractor, num_attractors, random_val;
	UUtUns32					*p_lensframes, *p_texture, current_tick, other_emitter_index, attractor_index, attractor_id;
	UUtBool						is_dead, require_attractor, particle_pos_worldspace, particle_dir_worldspace, success, has_worldspace_orientation;
	float						particle_character;		// whereabouts in the stream of particles this one is
	float						particle_speed, secondary_speed, phi, sinphi, theta, random_particle;
	float						emit_radius, r_inner, r_outer, center_bias, x, y, z, inaccuracy, height;
	M3tPoint3D					attractor_pt, our_world_pt;
	M3tVector3D					emit_offset, particle_dir, *p_position, *p_velocity, *p_offset, *parent_vel;
	M3tVector3D					*parent_offset, emitter_pos, *parent_position;
	M3tContrailData				*p_contraildata;
	P3tParticle *				new_particle;
	M3tMatrix3x3 				*p_orientation, *parent_orientation, transformed_orientation, worldspace_orientation;
	M3tMatrix4x3				**p_matrix, **parent_matrix, *apply_matrix, *override_matrix;
	P3tOwner					*p_owner, *parent_owner, owner;
	P3tParticleEmitterData *	emitter_data, *other_emitter_data;
	P3tPrevPosition *			p_prev_position;
	P3tAttractorData			*p_attractordata, *parent_attractordata;
	AKtOctNodeIndexCache		*p_envdata;
	P3tEmitterOrientationType	orient_up, orient_dir;
	IMtShade *					p_tint;

	emitter_data = ((P3tParticleEmitterData *) inParticle->vararray) + inEmitterIndex;
	
	if (inEmitter->emittedclass == NULL)
		return UUcFalse;

	// if we're being killed by the fact that we are entering a cutscene, then don't emit any
	// further dangerous particles
	if ((inParticle->header.flags & P3cParticleFlag_CutsceneKilled) &&
		(inEmitter->emittedclass->definition->flags2 & (P3cParticleClassFlag2_ExpireOnCutscene | P3cParticleClassFlag2_DieOnCutscene))) {
		return UUcFalse;
	}

	// there may be a random chance of us not emitting
	if (inEmitter->emit_chance < UUcMaxUns16) {
		random_val = UUrLocalRandom();
		if (random_val >= inEmitter->emit_chance)
			return UUcFalse;
	}

	// check to make sure that the parent isn't dead
	parent_position = P3rGetPositionPtr(inClass, inParticle, &is_dead);
	if (is_dead) {
		return UUcTrue;
	}

	num_particles = (UUtUns16) inEmitter->burst_size;

	// burst_size is a floating point number which means that bursts may vary
	random_particle = inEmitter->burst_size - num_particles;
	if (random_particle > 0.001f) {
		if (P3rRandom() < random_particle)
			num_particles++;
	}

	require_attractor = ((inEmitter->direction_type == P3cEmitterDirection_Attractor) || 
						(inEmitter->emittedclass->definition->attractor.iterator_type == P3cAttractorIterator_EmittedTowards));
	if (require_attractor) {
		P3rIterateAttractors(inClass, inParticle, 0);
	}

	if (P3gAttractorBuf_Valid && (inEmitter->flags & P3cEmitterFlag_OnePerAttractor)) {
		num_per_attractor = num_particles;
		num_attractors = (UUtUns16) P3gAttractorBuf_NumAttractors;
		if (inEmitter->flags & P3cEmitterFlag_AtLeastOne) {
			num_attractors = UUmMax(num_attractors, 1);
		}

		num_particles = num_per_attractor * num_attractors;
	} else {
		num_per_attractor = 0;
	}

	if (num_particles == 0)
		return UUcFalse;

	if (inEmitter->direction_type == P3cEmitterDirection_Attractor) {
		is_dead = P3rGetRealPosition(inClass, inParticle, &our_world_pt);
		if (is_dead) {
			return UUcFalse;
		}
	}

	// sanity check
	UUmAssert(num_particles < 1000);

	for (itr = 0; itr < num_particles; itr++) {
		// no information about 'whereabouts' this particle is
		particle_character = 0;
		particle_pos_worldspace = UUcFalse;
		particle_dir_worldspace = UUcFalse;
		has_worldspace_orientation = UUcFalse;
		override_matrix = NULL;

		attractor_id = (UUtUns32) -1;
		if (P3gAttractorBuf_Valid && (P3gAttractorBuf_NumAttractors > 0) && require_attractor) {
			// determine which attractor we are emitting towards
			if ((num_per_attractor > 0) && (inEmitter->flags & P3cEmitterFlag_OnePerAttractor)) {
				attractor_index = itr / num_per_attractor;
				UUmAssert(attractor_index < P3gAttractorBuf_NumAttractors);
			} else {
				attractor_index = itr;
				if (inEmitter->flags & P3cEmitterFlag_RotateAttractors) {
					attractor_index += emitter_data->num_emitted;
				}
				attractor_index %= P3gAttractorBuf_NumAttractors;
			}
			if (attractor_index < P3gAttractorBuf_NumAttractors) {
				attractor_id = P3gAttractorBuf_Storage[attractor_index];
			}
		}

		// determine the particle's position relative to its parent
		switch(inEmitter->position_type) {
		case P3cEmitterPosition_Point:
			MUmVector_Set(emit_offset, 0, 0, 0);
			break;

		case P3cEmitterPosition_Line:
			// emit from -r to +r along the X axis
			if (P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_PositionValOffset + 0],
				                  (void *) &emit_radius) == UUcTrue) {
				particle_character = P3mSignedRandom();
				MUmVector_Set(emit_offset, emit_radius * particle_character, 0, 0);
				particle_character = (float)fabs(particle_character);
			} else {
				MUmVector_Set(emit_offset, 0, 0, 0);
			}
			break;

		case P3cEmitterPosition_Circle:
			// emit in a circle or annulus, with r = [r_inner, r_outer] in the XZ plane
			if ((P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_PositionValOffset + 0],
				                  (void *) &r_inner) == UUcTrue) &&
				(P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_PositionValOffset + 1],
				                  (void *) &r_outer) == UUcTrue)) {

				theta = M3c2Pi * P3rRandom();
				particle_character = P3rRandom();
				emit_radius = r_inner + (r_outer - r_inner) * particle_character;

				emit_offset.x = MUrCos(theta) * emit_radius;
				emit_offset.y = 0;
				emit_offset.z = MUrSin(theta) * emit_radius;
			} else {
				MUmVector_Set(emit_offset, 0, 0, 0);
			}
			break;

		case P3cEmitterPosition_Sphere:
			// emit in a spherical shell, with r = [r_inner, r_outer]
			if ((P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_PositionValOffset + 0],
				                  (void *) &r_inner) == UUcTrue) &&
				(P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_PositionValOffset + 1],
				                  (void *) &r_outer) == UUcTrue)) {

				particle_character = P3rRandom();
				emit_radius = r_inner + (r_outer - r_inner) * particle_character;
				P3rGetRandomUnitVector3D(&emit_offset);
				MUmVector_Scale(emit_offset, emit_radius);
			} else {
				MUmVector_Set(emit_offset, 0, 0, 0);
			}
			break;

		case P3cEmitterPosition_Offset:
			// emit from an offset position
			if ((P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_PositionValOffset + 0],
				                  (void *) &x) == UUcTrue) &&
				(P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_PositionValOffset + 1],
				                  (void *) &y) == UUcTrue) &&
				(P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_PositionValOffset + 2],
				                  (void *) &z) == UUcTrue)) {

				MUmVector_Set(emit_offset, x, y, z);
			} else {
				MUmVector_Set(emit_offset, 0, 0, 0);
			}
			break;

		case P3cEmitterPosition_Cylinder:
			// emit from a cylindrical shell
			if ((P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_PositionValOffset + 0],
				                  (void *) &height) == UUcTrue) &&
				(P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_PositionValOffset + 1],
				                  (void *) &r_inner) == UUcTrue) &&
				(P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_PositionValOffset + 2],
				                  (void *) &r_outer) == UUcTrue)) {

				theta = M3c2Pi * P3rRandom();
				particle_character = P3rRandom();
				emit_radius = r_inner + (r_outer - r_inner) * particle_character;

				emit_offset.x = MUrCos(theta) * emit_radius;
				emit_offset.y = height * P3rRandom();
				emit_offset.z = MUrSin(theta) * emit_radius;
			} else {
				MUmVector_Set(emit_offset, 0, 0, 0);
			}
			break;

		case P3cEmitterPosition_BodySurface:
			MUmVector_Set(emit_offset, 0, 0, 0);

			parent_owner = P3rGetOwnerPtr(inClass, inParticle);
			if (parent_owner != NULL) {
				owner = *parent_owner;
				if ((owner & (WPcDamageOwner_CharacterWeapon | WPcDamageOwner_CharacterMelee)) &&
					(P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_PositionValOffset + 0],
									  (void *) &emit_radius) == UUcTrue)) {

					override_matrix = NULL;
					if (ONrParticle3_FindBodySurfacePosition(owner, emit_radius, &emit_offset, &override_matrix, &worldspace_orientation)) {
						// we successfully found a point on the character's body surface
						has_worldspace_orientation = UUcTrue;
						particle_pos_worldspace = UUcTrue;
					}
				}
			}
			break;

		case P3cEmitterPosition_BodyBones:
			MUmVector_Set(emit_offset, 0, 0, 0);

			parent_owner = P3rGetOwnerPtr(inClass, inParticle);
			if (parent_owner != NULL) {
				owner = *parent_owner;
				if ((owner & (WPcDamageOwner_CharacterWeapon | WPcDamageOwner_CharacterMelee)) &&
					(P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_PositionValOffset + 0],
									  (void *) &emit_radius) == UUcTrue)) {

					override_matrix = NULL;
					if (ONrParticle3_FindBodyBonePosition(owner, emit_radius, &emit_offset, &override_matrix)) {
						// we successfully found a point on the character's bones
						particle_pos_worldspace = UUcTrue;
					}
				}
			}
			break;

		default:
			UUmAssert(!"unknown emitter position type");
		}

		// determine the particle's velocity
		switch(inEmitter->direction_type) {
		case P3cEmitterDirection_Linear:
			MUmVector_Set(particle_dir, 0, 1, 0);
			break;
			
		case P3cEmitterDirection_Random:
			P3rGetRandomUnitVector3D(&particle_dir);
			break;
 
		case P3cEmitterDirection_Conical:
			if ((P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_DirectionValOffset + 0],
				                  (void *) &r_outer) == UUcTrue) &&
				(P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_DirectionValOffset + 1],
				                  (void *) &center_bias) == UUcTrue)) {

				P3iConicalDirection(r_outer, center_bias, &particle_character, &particle_dir);
			} else {
				MUmVector_Set(particle_dir, 0, 1, 0);
			}
			break;

		case P3cEmitterDirection_Ring:
			if ((P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_DirectionValOffset + 0],
				                  (void *) &phi) == UUcTrue) &&
				(P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_DirectionValOffset + 1],
				                  (void *) &theta) == UUcTrue)) {
				phi *= M3cDegToRad;
				theta *= M3cDegToRad;

				// particles in a burst spread out around the ring
				theta += itr * M3c2Pi / num_particles;

				UUmTrig_Clip(theta);
				UUmTrig_Clip(phi);

				sinphi = MUrSin(phi);
				MUmVector_Set(particle_dir, MUrCos(theta) * sinphi, MUrCos(phi), MUrSin(theta) * sinphi);
			} else {
				MUmVector_Set(particle_dir, 0, 1, 0);
			}
			break;

		case P3cEmitterDirection_OffsetDir:
			if ((P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_DirectionValOffset + 0],
				                  (void *) &x) == UUcTrue) &&
				(P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_DirectionValOffset + 1],
				                  (void *) &y) == UUcTrue) &&
				(P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_DirectionValOffset + 2],
				                  (void *) &z) == UUcTrue)) {
				MUmVector_Set(particle_dir, x, y, z);
			} else {
				MUmVector_Set(particle_dir, 0, 1, 0);
			}
			break;

		case P3cEmitterDirection_Inaccurate:
			if ((P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_DirectionValOffset + 0],
				                  (void *) &r_outer) == UUcTrue) &&
				(P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_DirectionValOffset + 1],
				                  (void *) &inaccuracy) == UUcTrue) &&
				(P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_DirectionValOffset + 2],
				                  (void *) &center_bias) == UUcTrue)) {

				p_owner = P3rGetOwnerPtr(inClass, inParticle);
				if (p_owner != NULL) {
					r_outer += inaccuracy * WPrParticleInaccuracy(*p_owner);
				}

				P3iConicalDirection(r_outer, center_bias, &particle_character, &particle_dir);
			} else {
				MUmVector_Set(particle_dir, 0, 1, 0);
			}
			break;

		case P3cEmitterDirection_Attractor:
			if (attractor_id == (UUtUns32) -1) {
				// no attractor, just emit straight
				MUmVector_Set(particle_dir, 0, 1, 0);
			} else {
				success = P3rDecodeAttractorIndex(attractor_id, UUcFalse, &attractor_pt, NULL);
				if (success) {
					// get the direction to the attractor
					MUmVector_Subtract(particle_dir, attractor_pt, our_world_pt);
					MUmVector_Normalize(particle_dir);
					particle_dir_worldspace = UUcTrue;
				} else {
					// no attractor, just emit straight
					MUmVector_Set(particle_dir, 0, 1, 0);
				}
			}
			break;

		default:
			UUmAssert(!"unknown emitter direction type");	
		}

		// determine the particle's speed
		switch(inEmitter->speed_type) {
		case P3cEmitterSpeed_Uniform:
			P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_SpeedValOffset + 0],
								(void *) &particle_speed);
			break;
			
		case P3cEmitterSpeed_Stratified:
			if (P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_SpeedValOffset + 0],
				                  (void *) &particle_speed) == UUcTrue) {
				if (P3iCalculateValue(inParticle, &inEmitter->emitter_value[P3cEmitterValue_SpeedValOffset + 1],
				                  (void *) &secondary_speed) == UUcTrue) {
					// use particle_character (set by the position and/or direction above) to interpolate
					// between two different speed settings
					particle_speed += particle_character * (secondary_speed - particle_speed);
				}
			} else {
				particle_speed = 0;
			}
			break;

		default:
			UUmAssert(!"unknown emitter speed type");	
		}

		// create a new particle
		current_tick = MUrUnsignedSmallFloat_To_Uns_Round(inEmitTime * UUcFramesPerSecond);
		new_particle = P3rCreateParticle(inEmitter->emittedclass, current_tick);
		if (new_particle == NULL) {
			// if we cannot create this particle, we won't be able to create any others in this burst either
			return UUcFalse;
		}

		if (inEmitter->flags & P3cEmitterFlag_OrientToVelocity) {
			new_particle->header.flags |= P3cParticleFlag_OrientToVelocity;
		}

		if (inEmitter->flags & P3cEmitterFlag_InheritTint) {
			p_tint = P3rGetTintPtr(inEmitter->emittedclass, new_particle);
			if (p_tint != NULL) {
				// store the parent's tint in the child particle
				P3iCalculateValue(inParticle, &inClass->definition->appearance.tint, (void *) p_tint);
			}
		}

		if (inParticle->header.flags & P3cParticleFlag_Temporary) { 
			// temporary particles emit temporary particles
			new_particle->header.flags |= P3cParticleFlag_Temporary;
			inEmitter->emittedclass->non_persistent_flags |=
				P3cParticleClass_NonPersistentFlag_HasTemporaryParticles;
		}
		if (inParticle->header.flags & P3cParticleFlag_CutsceneKilled) {
			// particles spawned by a cutscene-kill command recursively propagate
			// this flag, so that none of their children emit dangerous particles
			new_particle->header.flags |= P3cParticleFlag_CutsceneKilled;
		}

		// determine what our particle reference should be
		switch(inEmitter->link_type) {
		case P3cEmitterLinkType_None:
			new_particle->header.user_ref = P3cParticleReference_Null;
			break;
			
		case P3cEmitterLinkType_Emitter:
			new_particle->header.user_ref = inParticle->header.self_ref;
			break;
			
		case P3cEmitterLinkType_OurLink:
			new_particle->header.user_ref = inParticle->header.user_ref;
			break;
			
		default:
			other_emitter_index = inEmitter->link_type - P3cEmitterLinkType_LastEmission;
			if ((other_emitter_index < 0) || (other_emitter_index >= inClass->definition->num_emitters)) {
				// no such emitter exists for this particle!
				new_particle->header.user_ref = P3cParticleReference_Null;
			} else {
				other_emitter_data = ((P3tParticleEmitterData *) inParticle->vararray) + other_emitter_index;
				new_particle->header.user_ref = other_emitter_data->last_particle;
			}
			break;
		}

		parent_orientation = P3rGetOrientationPtr(inClass, inParticle);

		p_owner = P3rGetOwnerPtr(inEmitter->emittedclass, new_particle);
		if (p_owner != NULL) {
			parent_owner = P3rGetOwnerPtr(inClass, inParticle);
			if (parent_owner == NULL) {
				*p_owner = 0;
			} else {
				*p_owner = *parent_owner;
			}
		}

		parent_matrix = P3rGetDynamicMatrixPtr(inClass, inParticle);		
		p_matrix = P3rGetDynamicMatrixPtr(inEmitter->emittedclass, new_particle);
		if (override_matrix) {
			if (p_matrix) {
				// set the override matrix
				*p_matrix = override_matrix;
				apply_matrix = NULL;
			} else {
				// apply the override matrix
				apply_matrix = override_matrix;
			}
		} else if ((parent_matrix == NULL) || (particle_pos_worldspace)) {
			// the parent has no dynamic matrix
			apply_matrix = NULL;
			if (p_matrix) {
				*p_matrix = NULL;
			}
		} else {
			if (p_matrix) {
				// both parent and child have a dynamic matrix, transmit it and don't apply it
				apply_matrix = NULL;
				*p_matrix = *parent_matrix;
				if (inParticle->header.flags & P3cParticleFlag_AlwaysUpdatePosition) {
					new_particle->header.flags |= P3cParticleFlag_AlwaysUpdatePosition;
				}
			} else {
				// the parent has a dynamic matrix but the child doesn't. apply it.
				apply_matrix = *parent_matrix;
			}
		}

		MUmVector_Copy(emitter_pos, *parent_position);

		if (particle_pos_worldspace) {
			p_position = P3rGetPositionPtr(inEmitter->emittedclass, new_particle, &is_dead);
			if (is_dead) {
				// the child particle has died before we could set it up
				continue;
			}

			// set up the particle's position exactly where desired in worldspace, and zero its
			// offset position
			if (apply_matrix) {
				MUrMatrix_MultiplyPoint(&emit_offset, apply_matrix, p_position);
			} else {
				MUmVector_Copy(*p_position, emit_offset);
			}
			p_offset = P3rGetOffsetPosPtr(inEmitter->emittedclass, new_particle);
			if (p_offset != NULL) {
				MUmVector_Set(*p_offset, 0, 0, 0);
			}

		} else {
			// apply the parent's offset position if there is one
			parent_offset = P3rGetOffsetPosPtr(inClass, inParticle);
			if (parent_offset != NULL) {
				MUmVector_Add(emitter_pos, emitter_pos, *parent_offset);
			}

			// rotate the emitted offset by the parent's orientation
			if (parent_orientation != NULL) {
				MUrMatrix3x3_MultiplyPoint(&emit_offset, parent_orientation, &emit_offset);
			}

			// apply the dynamic matrix if necessary to the emitted offset and the parent's position
			if (apply_matrix) {
				MUrMatrix_MultiplyPoint(&emitter_pos, apply_matrix, &emitter_pos);
				MUrMatrix3x3_MultiplyPoint(&emit_offset, (M3tMatrix3x3 *) apply_matrix, &emit_offset);
			}


			p_position = P3rGetPositionPtr(inEmitter->emittedclass, new_particle, &is_dead);
			if (is_dead) {
				// the child particle has died before we could set it up
				continue;
			}

			p_offset = P3rGetOffsetPosPtr(inEmitter->emittedclass, new_particle);
			if (p_offset == NULL) {
				// add the emit offset to the emitter's position to get the particle's position
				MUmVector_Add(*p_position, emit_offset, emitter_pos);
			} else {
				// set the emitted particle's position and offset separately
				MUmVector_Copy(*p_position, emitter_pos);
				MUmVector_Copy(*p_offset, emit_offset);
			}
		}

		p_velocity = P3rGetVelocityPtr(inEmitter->emittedclass, new_particle);
		if (p_velocity != NULL) {
			if (!particle_dir_worldspace) {
				if (parent_orientation != NULL) {
					MUrMatrix3x3_MultiplyPoint(&particle_dir, parent_orientation, &particle_dir);
				}

				// particle velocity needs to be transformed by dynamic matrix too, but since it's
				// a difference vector we don't apply the translation part
				if (apply_matrix) {
					MUrMatrix3x3_MultiplyPoint(&particle_dir, (M3tMatrix3x3 *) apply_matrix, &particle_dir);
				}
			}

			MUmVector_ScaleCopy(*p_velocity, particle_speed, particle_dir);

			// this comes after dynamic matrix is applied, because particles that are attached
			// to an object and have a velocity field aren't storing _their_ velocity relative
			// to the object, but rather the object's velocity
			//
			// also note that this isn't applied to particle_dir
			if (inEmitter->flags & P3cEmitterFlag_AddParentVelocity) {
				parent_vel = P3rGetVelocityPtr(inClass, inParticle);
				if (parent_vel != NULL) {
					MUmVector_Add(*p_velocity, *p_velocity, *parent_vel);
				}
			}
		}

		// determine the orientation of the newly emitted particle.
		// NB: this may alter the parent_orientation pointer - don't use it after this code
		p_orientation = P3rGetOrientationPtr(inEmitter->emittedclass, new_particle);
		if (p_orientation != NULL) {
			if (has_worldspace_orientation) {
				// just copy the given orientation
				if (apply_matrix == NULL) {
					*p_orientation = worldspace_orientation;
				} else {
					MUrMatrix3x3_Multiply((M3tMatrix3x3 *) apply_matrix, &worldspace_orientation, p_orientation);
				}
			} else {
				orient_dir = inEmitter->orientdir_type;
				orient_up = inEmitter->orientup_type;

				if (parent_orientation == NULL) {
					// make sure we don't try to use the parent's orientation
					if ((orient_dir >= P3cEmitterOrientation_ParentPosX) &&
						(orient_dir <= P3cEmitterOrientation_ParentNegZ)) {
						orient_dir += (P3cEmitterOrientation_WorldPosX - P3cEmitterOrientation_ParentPosX);
					}
					if ((orient_up >= P3cEmitterOrientation_ParentPosX) &&
						(orient_up <= P3cEmitterOrientation_ParentNegZ)) {
						orient_up += (P3cEmitterOrientation_WorldPosX - P3cEmitterOrientation_ParentPosX);
					}
				} else if (apply_matrix != NULL) {
					// transform the parent's orientation
					MUrMatrix3x3_Multiply((M3tMatrix3x3 *) apply_matrix, parent_orientation, &transformed_orientation);
					parent_orientation = &transformed_orientation;
				}

				P3rConstructOrientation(orient_dir, orient_up, parent_orientation, &emit_offset, &particle_dir, p_orientation);
			}
		}

		// randomise texture start index
		p_texture = P3rGetTextureIndexPtr(inEmitter->emittedclass, new_particle);
		if (p_texture != NULL) {
			*p_texture = (UUtUns32) UUrLocalRandom();
		}

		// set texture time index to be now
		p_texture = P3rGetTextureTimePtr(inEmitter->emittedclass, new_particle);
		if (p_texture != NULL) {
			*p_texture = current_tick;
		}

		// contrail data is initially empty
		p_contraildata = P3rGetContrailDataPtr(inEmitter->emittedclass, new_particle);
		if (p_contraildata != NULL) {
			MUmVector_Copy(p_contraildata->position, new_particle->header.position);
			MUmVector_Set(p_contraildata->width, 0, 0, 0);
			p_contraildata->tint = IMcShade_White;
			p_contraildata->alpha = 0;
		}

		// no previous position
		p_prev_position = P3rGetPrevPositionPtr(inEmitter->emittedclass, new_particle);
		if (p_prev_position != NULL) {
			p_prev_position->time = 0;
		}

		// lensflares are not initially visible
		p_lensframes = P3rGetLensFramesPtr(inEmitter->emittedclass, new_particle);
		if (p_lensframes != NULL) {
			*p_lensframes = 0;
		}

		switch(inEmitter->emittedclass->definition->attractor.iterator_type) {
			case P3cAttractorIterator_EmittedTowards:
			case P3cAttractorIterator_Parent:
				p_attractordata = P3rGetAttractorDataPtr(inEmitter->emittedclass, new_particle);
				if (p_attractordata != NULL) {
					p_attractordata->delay_tick = current_tick + (P3cAttractor_RandomDelay * UUrLocalRandom()) / UUcMaxUns16;
					if (inEmitter->emittedclass->definition->attractor.iterator_type == P3cAttractorIterator_EmittedTowards) {
						p_attractordata->index = attractor_id;
					} else {
						parent_attractordata = P3rGetAttractorDataPtr(inClass, inParticle);
						p_attractordata->index = (parent_attractordata == NULL) ? (UUtUns32) -1 : parent_attractordata->index;
					}

					if (p_attractordata->index != (UUtUns32) -1) {
						new_particle->header.flags |= P3cParticleFlag_HasAttractor;
					}
				}
			break;
		}

		// initialize the environment data cache
		p_envdata = P3rGetEnvironmentCache(inEmitter->emittedclass, new_particle);
		if (p_envdata != NULL) {
			AKrCollision_InitializeSpatialCache(p_envdata);
		}

		// set the emitter's particle reference to be the newly emitted particle
		emitter_data->last_particle = new_particle->header.self_ref;

		P3rSendEvent(inEmitter->emittedclass, new_particle, P3cEvent_Create, inEmitTime);
	}

	return UUcFalse;
}

// define temporary macros
#define TRANSPOSE_AXES(m_in, x, y, z, m_out) {						\
	m_out->m[0][0] = ((x & 0x01) ? -1 : +1) * m_in->m[x >> 1][0];	\
	m_out->m[0][1] = ((x & 0x01) ? -1 : +1) * m_in->m[x >> 1][1];	\
	m_out->m[0][2] = ((x & 0x01) ? -1 : +1) * m_in->m[x >> 1][2];	\
	m_out->m[1][0] = ((y & 0x01) ? -1 : +1) * m_in->m[y >> 1][0];	\
	m_out->m[1][1] = ((y & 0x01) ? -1 : +1) * m_in->m[y >> 1][1];	\
	m_out->m[1][2] = ((y & 0x01) ? -1 : +1) * m_in->m[y >> 1][2];	\
	m_out->m[2][0] = ((z & 0x01) ? -1 : +1) * m_in->m[z >> 1][0];	\
	m_out->m[2][1] = ((z & 0x01) ? -1 : +1) * m_in->m[z >> 1][1];	\
	m_out->m[2][2] = ((z & 0x01) ? -1 : +1) * m_in->m[z >> 1][2];	\
}

#define WORLD_MATRIX(x, y, z, m_out) {				\
	UUrMemory_Clear(m_out, sizeof(M3tMatrix3x3));	\
	m_out->m[0][x >> 1] = ((x & 0x01) ? -1 : +1);	\
	m_out->m[1][y >> 1] = ((y & 0x01) ? -1 : +1);	\
	m_out->m[2][z >> 1] = ((z & 0x01) ? -1 : +1);	\
}

#define POS_X    0
#define NEG_X    1
#define POS_Y    2
#define NEG_Y    3
#define POS_Z    4
#define NEG_Z    5

// build an orientation for a particle
void P3rConstructOrientation(P3tEmitterOrientationType inOrientDir, P3tEmitterOrientationType inUpDir,
							 M3tMatrix3x3 *inParentOrientation, M3tVector3D *inEmitOffset, M3tVector3D *inParticleVel,
							 M3tMatrix3x3 *outOrientation)
{
	float right_len;
	M3tVector3D orient_dir, up_dir, right_dir;

	// note: there is a huge amount of code here that basically just checks
	// for special cases where we don't actually have to do the cross products etc
	// since we know the requested directions are perpendicular and normalized
	// already.
	//
	// this code also calculates and normalizes the direction of the particle (its +Y axis)
	switch(inOrientDir) {
	case P3cEmitterOrientation_ParentPosX:
		switch(inUpDir) {
		case P3cEmitterOrientation_ParentPosX:
		case P3cEmitterOrientation_ParentNegX:
			// degenerate cases - fall through

		case P3cEmitterOrientation_ParentPosY:
			TRANSPOSE_AXES(inParentOrientation, POS_Z, POS_X, POS_Y, outOrientation);
			return;

		case P3cEmitterOrientation_ParentNegY:
			TRANSPOSE_AXES(inParentOrientation, NEG_Z, POS_X, NEG_Y, outOrientation);
			return;

		case P3cEmitterOrientation_ParentPosZ:
			TRANSPOSE_AXES(inParentOrientation, NEG_Y, POS_X, POS_Z, outOrientation);
			return;

		case P3cEmitterOrientation_ParentNegZ:
			TRANSPOSE_AXES(inParentOrientation, POS_Y, POS_X, NEG_Z, outOrientation);
			return;
		}

		orient_dir = MUrMatrix_GetXAxis((M3tMatrix4x3 *) inParentOrientation);
	break;

	case P3cEmitterOrientation_ParentNegX:
		switch(inUpDir) {
		case P3cEmitterOrientation_ParentPosX:
		case P3cEmitterOrientation_ParentNegX:
			// degenerate cases - fall through

		case P3cEmitterOrientation_ParentPosY:
			TRANSPOSE_AXES(inParentOrientation, NEG_Z, NEG_X, POS_Y, outOrientation);
			return;

		case P3cEmitterOrientation_ParentNegY:
			TRANSPOSE_AXES(inParentOrientation, POS_Z, NEG_X, NEG_Y, outOrientation);
			return;

		case P3cEmitterOrientation_ParentPosZ:
			TRANSPOSE_AXES(inParentOrientation, POS_Y, NEG_X, POS_Z, outOrientation);
			return;

		case P3cEmitterOrientation_ParentNegZ:
			TRANSPOSE_AXES(inParentOrientation, NEG_Y, NEG_X, NEG_Z, outOrientation);
			return;
		}
		orient_dir = MUrMatrix_GetXAxis((M3tMatrix4x3 *) inParentOrientation);
		MUmVector_Negate(orient_dir);
	break;

	case P3cEmitterOrientation_ParentPosY:
		switch(inUpDir) {
		case P3cEmitterOrientation_ParentPosX:
			TRANSPOSE_AXES(inParentOrientation, NEG_Z, POS_Y, POS_X, outOrientation);
			return;

		case P3cEmitterOrientation_ParentNegX:
			TRANSPOSE_AXES(inParentOrientation, POS_Z, POS_Y, NEG_X, outOrientation);
			return;

		case P3cEmitterOrientation_ParentPosY:
		case P3cEmitterOrientation_ParentNegY:
			// degenerate cases - fall through

		case P3cEmitterOrientation_ParentPosZ:
			TRANSPOSE_AXES(inParentOrientation, POS_X, POS_Y, POS_Z, outOrientation);
			return;

		case P3cEmitterOrientation_ParentNegZ:
			TRANSPOSE_AXES(inParentOrientation, NEG_X, POS_Y, NEG_Z, outOrientation);
			return;
		}
		orient_dir = MUrMatrix_GetYAxis((M3tMatrix4x3 *) inParentOrientation);
	break;

	case P3cEmitterOrientation_ParentNegY:
		switch(inUpDir) {
		case P3cEmitterOrientation_ParentPosX:
			TRANSPOSE_AXES(inParentOrientation, POS_Z, NEG_Y, POS_X, outOrientation);
			return;

		case P3cEmitterOrientation_ParentNegX:
			TRANSPOSE_AXES(inParentOrientation, NEG_Z, NEG_Y, NEG_X, outOrientation);
			return;

		case P3cEmitterOrientation_ParentPosY:
		case P3cEmitterOrientation_ParentNegY:
			// degenerate cases - fall through

		case P3cEmitterOrientation_ParentPosZ:
			TRANSPOSE_AXES(inParentOrientation, NEG_X, NEG_Y, POS_Z, outOrientation);
			return;

		case P3cEmitterOrientation_ParentNegZ:
			TRANSPOSE_AXES(inParentOrientation, POS_X, NEG_Y, NEG_Z, outOrientation);
			return;
		}
		orient_dir = MUrMatrix_GetYAxis((M3tMatrix4x3 *) inParentOrientation);
		MUmVector_Negate(orient_dir);
	break;

	case P3cEmitterOrientation_ParentPosZ:
		switch(inUpDir) {
		case P3cEmitterOrientation_ParentPosZ:
		case P3cEmitterOrientation_ParentNegZ:
			// degenerate cases - fall through

		case P3cEmitterOrientation_ParentPosX:
			TRANSPOSE_AXES(inParentOrientation, POS_Y, POS_Z, POS_X, outOrientation);
			return;

		case P3cEmitterOrientation_ParentNegX:
			TRANSPOSE_AXES(inParentOrientation, NEG_Y, POS_Z, NEG_X, outOrientation);
			return;

		case P3cEmitterOrientation_ParentPosY:
			TRANSPOSE_AXES(inParentOrientation, NEG_X, POS_Z, POS_Y, outOrientation);
			return;

		case P3cEmitterOrientation_ParentNegY:
			TRANSPOSE_AXES(inParentOrientation, POS_X, POS_Z, NEG_Y, outOrientation);
			return;
		}
		orient_dir = MUrMatrix_GetZAxis((M3tMatrix4x3 *) inParentOrientation);
	break;

	case P3cEmitterOrientation_ParentNegZ:
		switch(inUpDir) {
		case P3cEmitterOrientation_ParentPosZ:
		case P3cEmitterOrientation_ParentNegZ:
			// degenerate cases - fall through

		case P3cEmitterOrientation_ParentPosX:
			TRANSPOSE_AXES(inParentOrientation, NEG_Y, NEG_Z, POS_X, outOrientation);
			return;

		case P3cEmitterOrientation_ParentNegX:
			TRANSPOSE_AXES(inParentOrientation, POS_Y, NEG_Z, NEG_X, outOrientation);
			return;

		case P3cEmitterOrientation_ParentPosY:
			TRANSPOSE_AXES(inParentOrientation, POS_X, NEG_Z, POS_Y, outOrientation);
			return;

		case P3cEmitterOrientation_ParentNegY:
			TRANSPOSE_AXES(inParentOrientation, NEG_X, NEG_Z, NEG_Y, outOrientation);
			return;
		}
		orient_dir = MUrMatrix_GetZAxis((M3tMatrix4x3 *) inParentOrientation);
		MUmVector_Negate(orient_dir);
	break;

	case P3cEmitterOrientation_WorldPosX:
		switch(inUpDir) {
		case P3cEmitterOrientation_WorldPosX:
		case P3cEmitterOrientation_WorldNegX:
			// degenerate cases - fall through

		case P3cEmitterOrientation_WorldPosY:
			WORLD_MATRIX(POS_Z, POS_X, POS_Y, outOrientation);
			return;

		case P3cEmitterOrientation_WorldNegY:
			WORLD_MATRIX(NEG_Z, POS_X, NEG_Y, outOrientation);
			return;

		case P3cEmitterOrientation_WorldPosZ:
			WORLD_MATRIX(NEG_Z, POS_X, POS_Z, outOrientation);
			return;

		case P3cEmitterOrientation_WorldNegZ:
			WORLD_MATRIX(POS_Z, POS_X, NEG_Z, outOrientation);
			return;
		}
		MUmVector_Set(orient_dir, 1, 0, 0);
	break;

	case P3cEmitterOrientation_WorldNegX:
		switch(inUpDir) {
		case P3cEmitterOrientation_WorldPosX:
		case P3cEmitterOrientation_WorldNegX:
			// degenerate cases - fall through

		case P3cEmitterOrientation_WorldPosY:
			WORLD_MATRIX(NEG_Z, NEG_X, POS_Y, outOrientation);
			return;

		case P3cEmitterOrientation_WorldNegY:
			WORLD_MATRIX(POS_Z, NEG_X, NEG_Y, outOrientation);
			return;

		case P3cEmitterOrientation_WorldPosZ:
			WORLD_MATRIX(POS_Z, NEG_X, POS_Z, outOrientation);
			return;

		case P3cEmitterOrientation_WorldNegZ:
			WORLD_MATRIX(NEG_Z, NEG_X, NEG_Z, outOrientation);
			return;
		}
		MUmVector_Set(orient_dir, -1, 0, 0);
	break;

	case P3cEmitterOrientation_WorldPosY:
		switch(inUpDir) {
		case P3cEmitterOrientation_WorldPosX:
			WORLD_MATRIX(NEG_Z, POS_Y, POS_X, outOrientation);
			return;

		case P3cEmitterOrientation_WorldNegX:
			WORLD_MATRIX(POS_Z, POS_Y, NEG_X, outOrientation);
			return;

		case P3cEmitterOrientation_WorldPosY:
		case P3cEmitterOrientation_WorldNegY:
			// degenerate cases - fall through

		case P3cEmitterOrientation_WorldPosZ:
			WORLD_MATRIX(POS_X, POS_Y, POS_Z, outOrientation);
			return;

		case P3cEmitterOrientation_WorldNegZ:
			WORLD_MATRIX(NEG_X, POS_Y, NEG_Z, outOrientation);
			return;
		}
		MUmVector_Set(orient_dir, 0, 1, 0);
	break;

	case P3cEmitterOrientation_WorldNegY:
		switch(inUpDir) {
		case P3cEmitterOrientation_WorldPosX:
			WORLD_MATRIX(POS_Z, NEG_Y, POS_X, outOrientation);
			return;

		case P3cEmitterOrientation_WorldNegX:
			WORLD_MATRIX(NEG_Z, NEG_Y, NEG_X, outOrientation);
			return;

		case P3cEmitterOrientation_WorldPosY:
		case P3cEmitterOrientation_WorldNegY:
			// degenerate cases - fall through

		case P3cEmitterOrientation_WorldPosZ:
			WORLD_MATRIX(NEG_X, NEG_Y, POS_Z, outOrientation);
			return;

		case P3cEmitterOrientation_WorldNegZ:
			WORLD_MATRIX(POS_X, NEG_Y, NEG_Z, outOrientation);
			return;
		}
		MUmVector_Set(orient_dir, 0, -1, 0);
	break;

	case P3cEmitterOrientation_WorldPosZ:
		switch(inUpDir) {
		case P3cEmitterOrientation_WorldPosZ:
		case P3cEmitterOrientation_WorldNegZ:
			// degenerate cases - fall through

		case P3cEmitterOrientation_WorldPosX:
			WORLD_MATRIX(POS_Y, POS_Z, POS_X, outOrientation);
			return;

		case P3cEmitterOrientation_WorldNegX:
			WORLD_MATRIX(NEG_Y, POS_Z, NEG_X, outOrientation);
			return;

		case P3cEmitterOrientation_WorldPosY:
			WORLD_MATRIX(NEG_X, POS_Z, POS_Y, outOrientation);
			return;

		case P3cEmitterOrientation_WorldNegY:
			WORLD_MATRIX(POS_X, POS_Z, NEG_Y, outOrientation);
			return;
		}
		MUmVector_Set(orient_dir, 0, 0, 1);
	break;

	case P3cEmitterOrientation_WorldNegZ:
		switch(inUpDir) {
		case P3cEmitterOrientation_WorldPosZ:
		case P3cEmitterOrientation_WorldNegZ:
			// degenerate cases - fall through

		case P3cEmitterOrientation_WorldPosX:
			WORLD_MATRIX(NEG_Y, NEG_Z, POS_X, outOrientation);
			return;

		case P3cEmitterOrientation_WorldNegX:
			WORLD_MATRIX(POS_Y, NEG_Z, NEG_X, outOrientation);
			return;

		case P3cEmitterOrientation_WorldPosY:
			WORLD_MATRIX(POS_X, NEG_Z, POS_Y, outOrientation);
			return;

		case P3cEmitterOrientation_WorldNegY:
			WORLD_MATRIX(NEG_X, NEG_Z, NEG_Y, outOrientation);
			return;
		}
		MUmVector_Set(orient_dir, 0, 0, -1);
	break;

	case P3cEmitterOrientation_Velocity:
		MUmVector_Copy(orient_dir, *inParticleVel);
		if (MUrVector_Normalize_ZeroTest(&orient_dir)) {
			orient_dir = MUrMatrix_GetYAxis((M3tMatrix4x3 *) inParentOrientation);
		}
	break;

	case P3cEmitterOrientation_ReverseVelocity:
		MUmVector_Copy(orient_dir, *inParticleVel);
		if (MUrVector_Normalize_ZeroTest(&orient_dir)) {
			orient_dir = MUrMatrix_GetYAxis((M3tMatrix4x3 *) inParentOrientation);
		} else {
			MUmVector_Negate(orient_dir);
		}
	break;

	case P3cEmitterOrientation_TowardsEmitter:
		MUmVector_Copy(orient_dir, *inEmitOffset);
		if (MUrVector_Normalize_ZeroTest(&orient_dir)) {
			orient_dir = MUrMatrix_GetZAxis((M3tMatrix4x3 *) inParentOrientation);
		} else {
			MUmVector_Negate(orient_dir);
		}
	break;

	case P3cEmitterOrientation_AwayFromEmitter:
		MUmVector_Copy(orient_dir, *inEmitOffset);
		if (MUrVector_Normalize_ZeroTest(&orient_dir)) {
			orient_dir = MUrMatrix_GetZAxis((M3tMatrix4x3 *) inParentOrientation);
		}
	break;

	default:
		UUmAssert(!"P3rConstructOrientation: unknown orient-dir type!");
	break;
	}

#if defined(DEBUGGING) && DEBUGGING
	{
		// check that the supplied orientation is approximately normalized - this is
		// done so that we don't get a spurious assertion in MUrConstructOrthonormalBasis
		// (which tests to a much higher tolerance than we ideally want)
		float orient_len = MUmVector_GetLength(orient_dir);
		UUmAssert((orient_len > 0.9) && (orient_len < 1.1f));
		MUmVector_Scale(orient_dir, 1.0f / orient_len);
	}
#endif

	// calculates (but don't normalize) the direction that we would like the upvector
	// of the particle (its +Z axis) to face in.
	switch(inUpDir) {
	case P3cEmitterOrientation_ParentPosX:
		up_dir = MUrMatrix_GetXAxis((M3tMatrix4x3 *) inParentOrientation);
	break;

	case P3cEmitterOrientation_ParentNegX:
		up_dir = MUrMatrix_GetXAxis((M3tMatrix4x3 *) inParentOrientation);
		MUmVector_Negate(up_dir);
	break;

	case P3cEmitterOrientation_ParentPosY:
		up_dir = MUrMatrix_GetYAxis((M3tMatrix4x3 *) inParentOrientation);
	break;

	case P3cEmitterOrientation_ParentNegY:
		up_dir = MUrMatrix_GetYAxis((M3tMatrix4x3 *) inParentOrientation);
		MUmVector_Negate(up_dir);
	break;

	case P3cEmitterOrientation_ParentPosZ:
		up_dir = MUrMatrix_GetZAxis((M3tMatrix4x3 *) inParentOrientation);
	break;

	case P3cEmitterOrientation_ParentNegZ:
		up_dir = MUrMatrix_GetZAxis((M3tMatrix4x3 *) inParentOrientation);
		MUmVector_Negate(up_dir);
	break;

	case P3cEmitterOrientation_WorldPosX:
		MUmVector_Set(up_dir, 1, 0, 0);
	break;

	case P3cEmitterOrientation_WorldNegX:
		MUmVector_Set(up_dir, -1, 0, 0);
	break;

	case P3cEmitterOrientation_WorldPosY:
		MUmVector_Set(up_dir, 0, 1, 0);
	break;

	case P3cEmitterOrientation_WorldNegY:
		MUmVector_Set(up_dir, 0, -1, 0);
	break;

	case P3cEmitterOrientation_WorldPosZ:
		MUmVector_Set(up_dir, 0, 0, 1);
	break;

	case P3cEmitterOrientation_WorldNegZ:
		MUmVector_Set(up_dir, 0, 0, -1);
	break;

	case P3cEmitterOrientation_Velocity:
		MUmVector_Copy(up_dir, *inParticleVel);
	break;

	case P3cEmitterOrientation_ReverseVelocity:
		MUmVector_Copy(up_dir, *inParticleVel);
		MUmVector_Negate(up_dir);
	break;

	case P3cEmitterOrientation_TowardsEmitter:
		MUmVector_Copy(up_dir, *inEmitOffset);
		MUmVector_Negate(up_dir);
	break;

	case P3cEmitterOrientation_AwayFromEmitter:
		MUmVector_Copy(up_dir, *inEmitOffset);
	break;

	default:
		UUmAssert(!"P3rConstructOrientation: unknown up-dir type!");
	break;
	}

	// calculate the supposed right-vector for the orientation
	MUrVector_CrossProduct(&orient_dir, &up_dir, &right_dir);

	right_len = MUmVector_GetLengthSquared(right_dir);
	if (UUmFloat_TightCompareZero(right_len)) {
		// the up and orientation directions are aligned. try an upvector of global Y...
		// cross product of a vector (vx, vy, vz) with (0, 1, 0) is (-vz, 0, vx)
		right_dir.x = -orient_dir.z;
		right_dir.y = 0;
		right_dir.z = orient_dir.x;

		right_len = MUmVector_GetLengthSquared(right_dir);
		if (UUmFloat_TightCompareZero(right_len)) {
			// the orientation direction is along +Y!
			if (UUmFloat_TightCompareZero(orient_dir.y)) {
				// the orientation direction is zero! there are meant to be
				// checks for this above... return identity matrix.
				MUrMatrix3x3_Identity(outOrientation);
				return;
			} else {
				// use an upvector of +Z
				MUmVector_Set(right_dir, 0, 0, 1);
			}
		} else {
			// normalize the calculated right-dir produced by up-dir of +Y
			right_len = MUrOneOverSqrt(right_len);
			MUmVector_Scale(right_dir, right_len);
		}
	} else {
		// normalize the calculated right-dir
		right_len = MUrOneOverSqrt(right_len);
		MUmVector_Scale(right_dir, right_len);
	}

	// calculate what upvector would be required for this
	MUrVector_CrossProduct(&right_dir, &orient_dir, &up_dir);
	MUmVector_Normalize(up_dir);

	// form the orthonormal basis
	MUrMatrix3x3_FormOrthonormalBasis(&right_dir, &orient_dir, &up_dir, outOrientation);
}

// undefine temporary macros
#undef TRANSPOSE_AXES
#undef WORLD_MATRIX
#undef POS_X
#undef NEG_X
#undef POS_Y
#undef NEG_Y
#undef POS_Z
#undef NEG_Z


// enable or disable collision debugging
void P3rDebugCollision(UUtBool inEnable)
{
	P3gDebugCollisionEnable = inEnable;
	
	if (P3gDebugCollisionEnable) {
		if (P3gDebugCollisionPoints == NULL) {
			P3gDebugCollisionPoints = UUrMemory_Block_New(P3cDebugCollisionMaxPoints * sizeof(M3tPoint3D));
		}

		P3gDebugCollision_NextPoint = 0;
		P3gDebugCollision_UsedPoints = 0;
	}
}

// look for collisions for a particle, with its environment
//
// NB: return value is whether the particle is dead!
static UUtBool P3iCheckCollisions(P3tParticleClass *inClass, P3tParticle *inParticle,
								  float inCurrentT, float *inEndT, M3tPoint3D *outEndPoint)
{
	M3tVector3D tempvec, vector_from_sphere, thiscollision_point, hit_point;
	M3tVector3D *p_velocity, *p_position, *complete_position, hit_normal;
	M3tMatrix3x3 *p_orientation;
	PHtSphereTree particle_tree;
	AKtEnvironment *environment;
	AKtGQ_Collision *gqCollision;
	M3tPlaneEquation plane_equ, *plane_equptr;
	AKtOctNodeIndexCache *p_envcache;
	float plane_dist, delta_t, timestep_dist, d_from_sphere, d_from_sphere_sq, equ_a, equ_b, equ_c, disc, totalrad;
	float thiscollision_t, temp_t, delta_position_sq, new_delta_t;
	UUtBool use_prev_position, is_dead, point_intersect, env_point_intersect, stationary, hit_context, repeat_collision, hit_character;
	UUtUns32 *p_owner;
	UUtUns16 itr;

	// store the particle's last collision
	P3gPreviousCollision = P3gCurrentCollision;
	P3gCurrentCollision.collided = UUcFalse;

	// get the particle's position
	p_position = P3rGetPositionPtr(inClass, inParticle, &is_dead);
	if (is_dead) {
		return UUcTrue;
	}

	// defaults
	use_prev_position = UUcFalse;
	complete_position = p_position;

	delta_t = *inEndT - inCurrentT;

#if ENABLE_PREVPOSITION_COLLISION
	// CB: delta-pos collision calculation was never really a good idea and the weapons
	// for which it was useful (scramble cannon, phase rifle) have since been reworked to
	// solve their collision problems in an analytic fashion. I am disabling this code
	// for now because there are better solutions which should be used instead. 12 september 2000.

	if (inClass->definition->flags & P3cParticleClassFlag_HasPrevPosition) {
		P3tPrevPosition *prev_position;
		M3tMatrix4x3 **p_dynmatrix;
		M3tVector3D calculated_position, *p_offset;
		float prev_delta_t;

		// we compute collision based on the position of the particle last update
		// this is useful for particles that aren't travelling in anything approximating a
		// straight line (e.g. wide spirals).
		prev_position = P3rGetPrevPositionPtr(inClass, inParticle);
		UUmAssert(prev_position != NULL);

		// work out what the complete position of the particle is this frame
		p_offset = P3rGetOffsetPosPtr(inClass, inParticle);
		if (p_offset != NULL) {
			MUmVector_Add(calculated_position, *complete_position, *p_offset);
			complete_position = &calculated_position;
		}

		p_dynmatrix = P3rGetDynamicMatrixPtr(inClass, inParticle);
		if (p_dynmatrix != NULL) {
			MUrMatrix_MultiplyPoint(complete_position, *p_dynmatrix, &calculated_position);
			complete_position = &calculated_position;
		}

		if (prev_position->time == 0) {
			// first update - we can't know what the motion of the particle is. use velocity
			// instead.
		} else {
			prev_delta_t = inCurrentT - prev_position->time;

			if (prev_delta_t > 1e-06f) {
				MUmVector_Subtract(P3gCurrentCollision.hit_direction, *complete_position, prev_position->point);
				MUmVector_Scale(P3gCurrentCollision.hit_direction, delta_t / prev_delta_t);
				use_prev_position = UUcTrue;
				stationary = UUcFalse;
			}
		}

		// set the position as calculated at this time
		prev_position->time = inCurrentT;
		prev_position->point = *complete_position;
	}
#endif

	if (delta_t <= 0) {
		*outEndPoint = *complete_position;
		return UUcFalse;
	}

	if (!use_prev_position) {
		// work out delta position for this frame based on velocity

		// note that this does not include other forms of motion e.g.
		// offset position or dynamic matrix

		// get the particle's motion
		p_velocity = P3rGetVelocityPtr(inClass, inParticle);
		if (p_velocity == NULL) {
			stationary = UUcTrue;
			MUmVector_Set(P3gCurrentCollision.hit_direction, 0, 0, 0);
		} else {
			stationary = UUcFalse;
			MUmVector_Copy(P3gCurrentCollision.hit_direction, *p_velocity);
			MUmVector_Scale(P3gCurrentCollision.hit_direction, delta_t);
		}
	}

	// work out the endpoint of the collision interval
	if (outEndPoint != NULL) {
		MUmVector_Add(*outEndPoint, *complete_position, P3gCurrentCollision.hit_direction);
	}

	if (P3gDebugCollisionEnable) {
		// track this timestep's collision and store it in the global debugging buffer
		P3gDebugCollisionPoints[P3gDebugCollision_NextPoint] = *complete_position;
		MUmVector_Add(P3gDebugCollisionPoints[P3gDebugCollision_NextPoint + 1], *complete_position, P3gCurrentCollision.hit_direction);
		
		P3gDebugCollision_NextPoint += 2;
		P3gDebugCollision_UsedPoints = UUmMax(P3gDebugCollision_UsedPoints, P3gDebugCollision_NextPoint);
		if (P3gDebugCollision_NextPoint >= P3cDebugCollisionMaxPoints) {
			P3gDebugCollision_NextPoint = 0;
		}
	}

	// set up a collision volume for the particle
	if ((inClass->definition->collision_radius.type == P3cValueType_Float) &&
		(inClass->definition->collision_radius.u.float_const.val > 0)) {
		point_intersect = UUcFalse;

		particle_tree.child = NULL;
		particle_tree.sibling = NULL;
		particle_tree.sphere.center = *complete_position;
		P3iCalculateValue(inParticle, &inClass->definition->collision_radius, &particle_tree.sphere.radius);
	} else {
		point_intersect = UUcTrue;
	}

	// don't like to do this, but it seems to be the only way
	environment = ONrGameState_GetEnvironment();

	if (!stationary && (inClass->definition->flags & P3cParticleClassFlag_Physics_CollideEnv)) {
		// CB: AKrCollision_Sphere is just too expensive, so I'm disabling the ability for particles
		// to collide with their environment as if they were spheres. the collision radius now only
		// affects particles colliding with objects and characters.
		env_point_intersect = UUcTrue;

		if (env_point_intersect) {
			P3mPerfTrack(inClass, P3cPerfEvent_EnvPointCol);
			p_envcache = P3rGetEnvironmentCache(inClass, inParticle);
			if (p_envcache == NULL) {
				COrConsole_Printf("### warning: particle class '%s' trying to collide without an environment cache", inClass->classname);
				AKrCollision_Point(environment, complete_position, &P3gCurrentCollision.hit_direction, AKcGQ_Flag_Obj_Col_Skip, 1);
			} else {
				AKrCollision_Point_SpatialCache(environment, complete_position, &P3gCurrentCollision.hit_direction,
												AKcGQ_Flag_Obj_Col_Skip, 1, p_envcache, p_envcache);
			}
		} else {
			P3mPerfTrack(inClass, P3cPerfEvent_EnvSphereCol);
			AKrCollision_Sphere(environment, &particle_tree.sphere, &P3gCurrentCollision.hit_direction, 
								AKcGQ_Flag_Obj_Col_Skip, 1);
		}

		if (AKgNumCollisions > 0) {
			// grab the first collision off the sorted akira collision list
			P3gCurrentCollision.collided = UUcTrue;
			P3gCurrentCollision.env_collision = UUcTrue;
			P3gCurrentCollision.damaged_hit = UUcFalse;
			P3gCurrentCollision.hit_gqindex = AKgCollisionList[0].gqIndex;
			P3gCurrentCollision.hit_point = AKgCollisionList[0].collisionPoint;

			if (P3gDrawEnvCollisions) {
				environment->gqGeneralArray->gqGeneral[P3gCurrentCollision.hit_gqindex].flags |= AKcGQ_Flag_Draw_Flash;
			}

			gqCollision = environment->gqCollisionArray->gqCollision + P3gCurrentCollision.hit_gqindex;
			AKmPlaneEqu_GetComponents(gqCollision->planeEquIndex,
									environment->planeArray->planes,
									P3gCurrentCollision.hit_normal.x,
									P3gCurrentCollision.hit_normal.y,
									P3gCurrentCollision.hit_normal.z, plane_dist);

			if (MUrVector_DotProduct(&P3gCurrentCollision.hit_normal, &P3gCurrentCollision.hit_direction) > 0) {
				// we have to do this when hitting two-sided glass quads
				MUmVector_Negate(P3gCurrentCollision.hit_normal);
			}

			// NB: AKrCollision_Point sets up float_distance using manhattan distance instead of
			// euclidean distance
			if (env_point_intersect) {
				timestep_dist = (float)fabs(P3gCurrentCollision.hit_direction.x) + (float)fabs(P3gCurrentCollision.hit_direction.y) + (float)fabs(P3gCurrentCollision.hit_direction.z);
			} else {
				timestep_dist = MUmVector_GetLength(P3gCurrentCollision.hit_direction);
			}
#if 0
			if (AKgCollisionList[0].float_distance < 0) {
				COrConsole_Print("### particle collision warning: float distance < 0");
			} else if (AKgCollisionList[0].float_distance > timestep_dist) {
				COrConsole_Print("### particle collision warning: float distance > timestep distance");
			}
#endif

			if (UUmFloat_CompareEqu(timestep_dist, 0.0f)) {
				// we aren't moving - hit at the end of the timestep, to prevent more collision calls than we
				// need
				P3gCurrentCollision.hit_t = *inEndT;
			} else {
				P3gCurrentCollision.hit_t = inCurrentT + delta_t * (AKgCollisionList[0].float_distance / timestep_dist);
			}
		}
	}

	// collide with physics contexts in the environment
	if (inClass->definition->flags & P3cParticleClassFlag_Physics_CollideChar) {
		if (P3gCurrentCollision.collided) {
			// we have collided already in this step. set our delta position correctly so that
			// we don't go past this timepoint when looking for character collisions.
			new_delta_t = P3gCurrentCollision.hit_t - inCurrentT;			
			MUmVector_Scale(P3gCurrentCollision.hit_direction, new_delta_t / delta_t);
			delta_t = new_delta_t;
		} else {
			// we don't already have a collision - hit_t shouldn't be less than anything
			P3gCurrentCollision.hit_t = 1e09;
		}

		if (delta_t > 1e-06f) {

			for(itr = 0; itr < P3gPhysicsCount; itr++)
			{
				PHtPhysicsContext *collide_context = P3gPhysicsList[itr];
				PHtSphereTree *hit_tree= NULL;

				if (collide_context->flags & PHcFlags_Physics_FaceCollision) {
					if (point_intersect) {
						if (MUrPoint_Distance_Squared(complete_position, &collide_context->sphereTree->sphere.center) >
							UUmSQR(collide_context->sphereTree->sphere.radius)) {
							// can't intersect with this context - continue looking
							continue;
						}

						P3mPerfTrack(inClass, P3cPerfEvent_PhyFaceCol);
						hit_context = PHrCollision_Volume_Ray(collide_context->worldAlignedBounds, complete_position,
									&P3gCurrentCollision.hit_direction, &plane_equ, &thiscollision_point);
						if (!hit_context) {
							// no intersections - continue looking
							continue;
						}

					} else {
						// optimization
						if (MUrPoint_Distance_Squared(complete_position, &collide_context->sphereTree->sphere.center) >
							UUmSQR(particle_tree.sphere.radius + collide_context->sphereTree->sphere.radius)) {
							// can't intersect with this context - continue looking
							continue;
						}

						P3mPerfTrack(inClass, P3cPerfEvent_PhyFaceCol);
						P3gNumColliders = 0;
						hit_context = PHrCollision_Volume_SphereTree(collide_context->worldAlignedBounds,
										&P3gCurrentCollision.hit_direction, (collide_context->level == PHcPhysics_Animate),
										&particle_tree,
										&hit_tree, &plane_equptr,
										&thiscollision_point, P3gColliders, &P3gNumColliders, collide_context);

						if (!hit_context) {
							// no intersections - continue looking
							continue;
						}
					}

					// determine the t-value of this hit point
					delta_position_sq = MUmVector_GetLengthSquared(P3gCurrentCollision.hit_direction);

					MUmVector_Subtract(tempvec, thiscollision_point, *complete_position);
					thiscollision_t = MUrVector_DotProduct(&tempvec, &P3gCurrentCollision.hit_direction) / delta_position_sq;
					thiscollision_t = UUmPin(thiscollision_t, 0, 1);

					thiscollision_t = inCurrentT + thiscollision_t * delta_t;
					thiscollision_t = UUmPin(thiscollision_t, inCurrentT, *inEndT);
					if (thiscollision_t >= P3gCurrentCollision.hit_t) {
						// we already have an earlier collision
						continue;
					}
	
					// use this newly found collision
					P3gCurrentCollision.hit_t = thiscollision_t;
					MUmVector_Copy(P3gCurrentCollision.hit_point, thiscollision_point);
					P3gCurrentCollision.hit_normal.x = plane_equ.a;
					P3gCurrentCollision.hit_normal.y = plane_equ.b;
					P3gCurrentCollision.hit_normal.z = plane_equ.c;
					MUmVector_Verify(P3gCurrentCollision.hit_normal);

				} else {				
					if (collide_context->callback->type == PHcCallback_Character) {
						// perform exact intersection test with character's body parts
						P3mPerfTrack(inClass, P3cPerfEvent_PhyCharCol);
						if (point_intersect) {
							// character - ray intersection
							hit_character = ONrCharacter_Collide_Ray((ONtCharacter *) collide_context->callbackData, complete_position,
																	&P3gCurrentCollision.hit_direction, UUcTrue, &P3gCurrentCollision.hit_partindex,
																	&thiscollision_t, &hit_point, &hit_normal);
						} else {
							// character - sphereray intersection
							hit_character = ONrCharacter_Collide_SphereRay((ONtCharacter *) collide_context->callbackData, complete_position,
																			&P3gCurrentCollision.hit_direction, particle_tree.sphere.radius,
																			&P3gCurrentCollision.hit_partindex, &thiscollision_t, &hit_point, &hit_normal);
						}

						if (!hit_character) {
							// no intersection - continue looking
							continue;
						}

						// get time from the t-value along the ray
						thiscollision_t = inCurrentT + delta_t * thiscollision_t;
						if (thiscollision_t >= P3gCurrentCollision.hit_t) {
							// we already have an earlier intersection, keep looking
							continue;
						}

						P3gCurrentCollision.hit_t = thiscollision_t;
						P3gCurrentCollision.hit_point = hit_point;
						P3gCurrentCollision.hit_normal = hit_normal;
					} else {
						// collide with a general physics context
						P3mPerfTrack(inClass, P3cPerfEvent_PhySphereCol);
						if (point_intersect) {
							hit_context = PHrCollision_Point_SphereTree(complete_position, &P3gCurrentCollision.hit_direction, 
								collide_context->sphereTree, &hit_tree);
						
						} else {
							hit_context = (NULL != PHrCollision_SphereTree_SphereTree(&particle_tree, &P3gCurrentCollision.hit_direction, 
								collide_context->sphereTree, &hit_tree));
						}

						if (!hit_context) {
							// no intersections - continue looking
							continue;
						}

						// construct the hit point from the sphere that we hit				
						UUmAssert(hit_tree != NULL);
							
						// CB: none of this should be necessary, but the spheretree code is set up to only do
						// bounding tests and not actual intersection tests. agh!
							
						// how far from the hit sphere is the particle center?
						MUmVector_Subtract(vector_from_sphere, *complete_position, hit_tree->sphere.center);
						d_from_sphere_sq = MUmVector_GetLengthSquared(vector_from_sphere);
						d_from_sphere = MUrSqrt(d_from_sphere_sq);
						
						if (point_intersect) {
							if (d_from_sphere < hit_tree->sphere.radius) {
								// particle is inside the bounding sphere to start with
								thiscollision_t = inCurrentT;
								if (thiscollision_t >= P3gCurrentCollision.hit_t) {
									// we already have an earlier collision
									continue;
								}
		
								// use this newly found collision
								P3gCurrentCollision.hit_t = thiscollision_t;
								MUmVector_Copy(P3gCurrentCollision.hit_point, *complete_position);
								MUmVector_Copy(P3gCurrentCollision.hit_normal, P3gCurrentCollision.hit_direction);
								MUmVector_Verify(P3gCurrentCollision.hit_normal);
							} else {
								// compute proportion of timestep until particle hits bounding sphere
								// quadratic equation: (u + vt - x)^2 = r^2 where particle = (u, v) and sphere = (x, r)
								//
								// this is (v^2)t^2 + (2 (u - x) v)t + ((u - x)^2 - r^2) = 0
								equ_a = MUrVector_DotProduct(&P3gCurrentCollision.hit_direction, &P3gCurrentCollision.hit_direction);
								equ_b = 2 * MUrVector_DotProduct(&P3gCurrentCollision.hit_direction, &vector_from_sphere);
								equ_c = d_from_sphere_sq - UUmSQR(hit_tree->sphere.radius);
								
								// if particle is heading away from sphere, no intersection
								if (equ_b > 0)
									continue;
								
								disc = UUmSQR(equ_b) - 4 * equ_a * equ_c;
								if (disc < 0)
									continue; // this should never happen...
								
								temp_t = (- equ_b - MUrSqrt(disc)) / (2 * equ_a);
								
								// build the actual time of collision
								thiscollision_t = temp_t;
								thiscollision_t = inCurrentT + thiscollision_t * delta_t;
								thiscollision_t = UUmPin(thiscollision_t, inCurrentT, *inEndT);
								if (thiscollision_t >= P3gCurrentCollision.hit_t) {
									// we already have an earlier collision
									continue;
								}
		
								// use this newly found collision
								P3gCurrentCollision.hit_t = thiscollision_t;

								// build the hit point
								MUmVector_ScaleCopy(P3gCurrentCollision.hit_point, temp_t, P3gCurrentCollision.hit_direction);
								MUmVector_Add(P3gCurrentCollision.hit_point, P3gCurrentCollision.hit_point, *complete_position);							
								MUmVector_Copy(P3gCurrentCollision.hit_normal, vector_from_sphere);
								MUmVector_Verify(P3gCurrentCollision.hit_normal);
							}
							
						} else {
							// calculate the time of collision
							if (stationary) {
								thiscollision_t = inCurrentT;
							} else {
								d_from_sphere -= particle_tree.sphere.radius + hit_tree->sphere.radius;
								thiscollision_t = d_from_sphere / MUmVector_GetLength(P3gCurrentCollision.hit_direction);
		
								thiscollision_t = inCurrentT + thiscollision_t * delta_t;
								thiscollision_t = UUmPin(thiscollision_t, inCurrentT, *inEndT);
							}
							if (thiscollision_t >= P3gCurrentCollision.hit_t) {
								// we already have an earlier collision
								continue;
							}
		
							// use this newly found collision
							P3gCurrentCollision.hit_t = thiscollision_t;

							// an exact computation of the spheres' contact points is possible, but for now just take a
							// point along the line that joins them. this point is defined at the ratio where they would be
							// touching if the distance was right.
							
							totalrad = hit_tree->sphere.radius + particle_tree.sphere.radius;
							
							MUmVector_ScaleCopy(P3gCurrentCollision.hit_point, hit_tree->sphere.radius / totalrad,
												*complete_position);
							MUmVector_ScaleIncrement(P3gCurrentCollision.hit_point, particle_tree.sphere.radius / totalrad,
													hit_tree->sphere.center);
													
							MUmVector_Copy(P3gCurrentCollision.hit_normal, vector_from_sphere);
							MUmVector_Verify(P3gCurrentCollision.hit_normal);
						}	
					}
				}

				// make sure that characters don't shoot themselves when running forwards
				p_owner = P3rGetOwnerPtr(inClass, inParticle);
				if ((p_owner != NULL) && (WPrParticleBelongsToSelf(*p_owner, collide_context))) {
					if (MUrVector_DotProduct(&P3gCurrentCollision.hit_normal, &P3gCurrentCollision.hit_direction) > 0) {
						// reject this intersection, we are hitting our owner while moving away from them.
						continue;
					}
				}

				// if we reach here then the collision that we found is nearer than the current one
				P3gCurrentCollision.collided = UUcTrue;
				P3gCurrentCollision.env_collision = UUcFalse;
				P3gCurrentCollision.damaged_hit = UUcFalse;
				P3gCurrentCollision.hit_context = collide_context;

				// reset delta position so that we don't go past this point
				// when looking for further collisions.
				new_delta_t = P3gCurrentCollision.hit_t - inCurrentT;
				if (new_delta_t < 0.001f) {
					delta_t = 0.0f;
					break;
				}
	
				MUmVector_Scale(P3gCurrentCollision.hit_direction, new_delta_t / delta_t);
				delta_t = new_delta_t;

				// keep looking for more collisions
			}
		}
	}

	if (!P3gCurrentCollision.collided)
		return UUcFalse;

	if (P3gPreviousCollision.collided) {
		// make sure we don't go into an infinite loop colliding with
		// the same object
		repeat_collision = UUcFalse;
		if (P3gPreviousCollision.env_collision == P3gCurrentCollision.env_collision) {
			if (P3gPreviousCollision.env_collision) {
				if (P3gPreviousCollision.hit_gqindex == P3gCurrentCollision.hit_gqindex) {
					repeat_collision = UUcTrue;
				}
			} else {
				if (P3gPreviousCollision.hit_context == P3gCurrentCollision.hit_context) {
					repeat_collision = UUcTrue;
				}
			}
		}

		if (repeat_collision) {
			// we can't collide until at least one frame passes
			P3gCurrentCollision.hit_t = UUmMax(P3gCurrentCollision.hit_t,
												P3gPreviousCollision.hit_t + (1.0f / UUcFramesPerSecond));
		}

	}

	if (P3gCurrentCollision.hit_t > *inEndT) {
		// we found an invalid collision. ignore it.
		P3gCurrentCollision.collided = UUcFalse;
		return UUcFalse;
	}

	// make sure we return a normalized normal vector
	if (MUrVector_Normalize_ZeroTest(&P3gCurrentCollision.hit_normal)) {
		// the hit normal is zero! this can happen if we are a stationary particle that is
		// inside a physics context's bounding sphere.
		p_orientation = P3rGetOrientationPtr(inClass, inParticle);
		if (p_orientation == NULL) {
			MUmVector_Set(P3gCurrentCollision.hit_normal, 0, 1, 0);
		} else {
			P3gCurrentCollision.hit_normal = MUrMatrix_GetYAxis((M3tMatrix4x3 *) p_orientation);
		}
	}
	MUmVector_Verify(P3gCurrentCollision.hit_normal);

	// we have collided before the end of the interval!
	*inEndT = P3gCurrentCollision.hit_t;
	return UUcFalse;
}

// perform a collision for a particle, using the data stored in P3gCollisionData by a previous call to
// P3iCheckCollisions
static UUtBool P3iDoCollision(P3tParticleClass *inClass, P3tParticle *inParticle, float collide_t)
{
	UUtUns16 event;

	if (!P3gCurrentCollision.collided)
		return UUcFalse;

	event = (inParticle->header.flags & P3cParticleFlag_ExplodeOnContact) ? P3cEvent_Explode :
		((P3gCurrentCollision.env_collision) ? P3cEvent_CollideEnv : P3cEvent_CollideChar);

	return P3iPerformActionList(inClass, inParticle, event,
								&inClass->definition->eventlist[event],
								collide_t, 0, UUcTrue);
}

// update a particle class's particles for a certain amount of time
static void P3iUpdateParticleClass(P3tParticleClass *inClass, UUtUns32 inNewTime)
{
	P3tParticle *particle, *next_particle;
	UUtBool is_dead, check_lockedvel, check_emit, do_collide, do_emit;
	UUtUns16 itr, emit_num, last_emit_num, vel_offset, orient_offset;
	UUtUns32 check_collisions, check_link;
	M3tMatrix3x3 *p_orientation;
	M3tVector3D *p_velocity;
	P3tActionList *actionlist;
	P3tEmitter *emitter;
	P3tParticleEmitterData *emitter_data;
	float run_t, final_t, current_t, last_t, delta_t;
#if PARTICLE_PERF_ENABLE
	UUtInt64 start_machinetime, elapsed_machinetime;
#endif

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Enter(P3g_ParticleUpdate);
#endif

	// we should never get into this function unless there is at least one particle
	// of that class - as it checks num_particles > 0
	UUmAssert(inClass->num_particles > 0);
	UUmAssert(inClass->non_persistent_flags & P3cParticleClass_NonPersistentFlag_Active);
	UUmAssert(inClass->headptr != NULL);

#if PARTICLE_PERF_ENABLE
	start_machinetime = UUrMachineTime_High();
#endif

	final_t = ((float) inNewTime) / UUcFramesPerSecond;
	actionlist = inClass->definition->eventlist;
	check_emit = (inClass->definition->num_emitters > 0) ? UUcTrue : UUcFalse;
	check_collisions = inClass->definition->flags & (P3cParticleClassFlag_Physics_CollideEnv |
													P3cParticleClassFlag_Physics_CollideChar);
	check_link = inClass->definition->flags2 & P3cParticleClassFlag2_LockToLinkedParticle;

	if ((inClass->definition->flags & P3cParticleClassFlag_HasOrientation) &&
		(inClass->definition->flags & P3cParticleClassFlag_HasVelocity)) {
		check_lockedvel = UUcTrue;

		vel_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_Velocity];
		orient_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_Orientation];

		UUmAssert((vel_offset >= 0) && (vel_offset < inClass->particle_size - sizeof(P3tParticleHeader)));
		UUmAssert((orient_offset >= 0) && (orient_offset < inClass->particle_size - sizeof(P3tParticleHeader)));
	} else {
		check_lockedvel = UUcFalse;
	}

	// update all particles in this class
	UUmAssert((inClass->headptr == NULL) || ((inClass->headptr->header.flags & P3cParticleFlag_Deleted) == 0));
	for (particle = inClass->headptr; particle != NULL; particle = next_particle) {
		next_particle = particle->header.nextptr;
		current_t = particle->header.current_time;
		is_dead = UUcFalse;

		P3mPerfTrack(inClass, P3cPerfEvent_Particle);

		UUmAssert((next_particle == NULL) || ((next_particle->header.flags & P3cParticleFlag_Deleted) == 0));
		UUmAssert((particle->header.flags & P3cParticleFlag_Deleted) == 0);

		// global collisions only store information for the current particle and are reset whenever we go to the next particle
		P3gPreviousCollision.collided = UUcFalse;
		P3gCurrentCollision.collided = UUcFalse;
		P3gCurrentStep_EndPointValid = UUcFalse;

		last_t = 0.0f;
		last_emit_num = UUcMaxUns16;
		
		while ((current_t < final_t - 0.001f) && (!is_dead)) {
			UUmAssert((particle->header.flags & P3cParticleFlag_Deleted) == 0);

			P3mPerfTrack(inClass, P3cPerfEvent_Update);

			/*
			 * set up for this timestep
			 */

			P3gCurrentStep_EndPointValid = UUcFalse;
			do_emit = UUcFalse;
			do_collide = UUcFalse;
			P3gAttractorBuf_Valid = UUcFalse;
			run_t = final_t;

			/*
			 * determine how long we should run the timestep for
			 */
			
			// if this particle has emitters, call them to determine whether they will emit during this
			// time period; if so, we need to only update until this time
			if (check_emit) {
				emit_num = 0;
				for (itr = 0, emitter_data = (P3tParticleEmitterData *) particle->vararray;
					itr < inClass->definition->num_emitters; itr++, emitter_data++) {
					if (emitter_data->num_emitted == P3cEmitterDisabled)
						continue;
					
					// check to see if we will emit before this time - sets do_emit, emit_num
					// and changes run_t if yes
					P3iCheckEmitter(inClass, particle, itr, emitter_data, current_t, &do_emit, &emit_num, &run_t);
					//					UUmAssert(run_t <= final_t);
				}
			}

			UUmAssert((particle->header.flags & P3cParticleFlag_Deleted) == 0);

			if (check_collisions && ((particle->header.flags & (P3cParticleFlag_Stopped | P3cParticleFlag_Chop)) == 0)) {
#if PERFORMANCE_TIMER
				UUrPerformanceTimer_Enter(P3g_ParticleCollision);
#endif
				is_dead = P3iCheckCollisions(inClass, particle, current_t, &run_t, &P3gCurrentStep_EndPoint);
#if PERFORMANCE_TIMER
				UUrPerformanceTimer_Exit(P3g_ParticleCollision);
#endif
				if (is_dead) {
					break;
				}

				if (P3gCurrentCollision.collided) {
					// we will collide before end of the timestep, defer any emission until next timestep
					do_collide = UUcTrue;
					do_emit = UUcFalse;
				} else {
					P3gCurrentStep_EndPointValid = UUcTrue;
				}

				UUmAssert(run_t <= final_t);
				UUmAssert(run_t >= current_t);
			}

			/*
			 * run the update cycle
			 */

			// run the particle up to this point
			delta_t = run_t - current_t;
			is_dead = P3iPerformActionList(inClass, particle, P3cEvent_Update, actionlist + P3cEvent_Update, current_t, delta_t, do_collide);				
			if (is_dead) {
				break;
			}

			// the particle has moved, the endpoint is no longer valid
			P3gCurrentStep_EndPointValid = UUcFalse;
			
			// check to see if the particle's lifetime has expired
			if (particle->header.lifetime != 0) {
				particle->header.lifetime -= delta_t;
				if (particle->header.lifetime < (1.0f / UUcFramesPerSecond)) {
					particle->header.lifetime = 0;
					
					// if there is a 'lifetime' action list, run that. otherwise die.
					// don't run the action list if we are already fading out or being chopped.
					if (((particle->header.flags & (P3cParticleFlag_FadeOut | P3cParticleFlag_Chop)) == 0) &&
						(actionlist[P3cEvent_Lifetime].end_index > actionlist[P3cEvent_Lifetime].start_index)) {
						is_dead = P3iPerformActionList(inClass, particle, P3cEvent_Lifetime,
														actionlist + P3cEvent_Lifetime,
														run_t + particle->header.lifetime, 0, UUcFalse);
					} else {
						P3rKillParticle(inClass, particle);
						is_dead = UUcTrue;
					}
				}
			}
			if (is_dead) {
				break;
			}
			
			/*
			 * perform post-update computation
			 */

			// if we have to update the particle's orientation to match a changed velocity, do so
			if (check_lockedvel &&
				((particle->header.flags & P3cParticleMask_VelocityRequiresRecompute) ==
											P3cParticleMask_VelocityRequiresRecompute)) {
				p_velocity = (M3tVector3D *) &particle->vararray[vel_offset];
				p_orientation = (M3tMatrix3x3 *) &particle->vararray[orient_offset];

				P3rConstructOrientation(P3cEmitterOrientation_Velocity, P3cEmitterOrientation_ParentPosZ, 
										p_orientation, NULL, p_velocity, p_orientation);
			}

			// emit a particle or burst of particles
			if (do_emit) {
				UUmAssert((emit_num >= 0) && (emit_num < inClass->definition->num_emitters));
				emitter = &inClass->definition->emitter[emit_num];
				
				is_dead = P3iEmitParticle(inClass, particle, emitter, emit_num, run_t);
				if (is_dead) {
					break;
				}
				
				// track the number of particles that have been emitted
				emitter_data = ((P3tParticleEmitterData *) particle->vararray) + emit_num;
				if (emitter->flags & P3cEmitterFlag_IncreaseEmitCount) {
					emitter_data->num_emitted++;
					
					if (emitter->flags & P3cEmitterFlag_DeactivateWhenEmitCountThreshold) {
						if (emitter_data->num_emitted >= emitter->particle_limit) {
							emitter_data->num_emitted = P3cEmitterDisabled;
						}
					} else {
						// check that we don't just keep incrementing all the way up to
						// the sentinel value
						if (emitter_data->num_emitted == P3cEmitterDisabled) {
							emitter_data->num_emitted = 0;
						}
					}
				}
				emitter_data->time_last_emission = run_t;
				last_emit_num = emit_num;
			}
				
			// run our deferred collision
			if (do_collide) {
				is_dead = P3iDoCollision(inClass, particle, run_t);
				if (is_dead) {
					break;
				}
			}

			/*
			 * set up for the next time
			 */

			// make absolutely sure we don't get locked into an infinite loop emitting!
			current_t = run_t;
			if (((current_t - last_t) < 0.0001f) && (emit_num == last_emit_num)) {
				current_t += 1.0f / UUcFramesPerSecond;
			}
			
			last_t = current_t;

			UUmAssert((particle->header.flags & P3cParticleFlag_Deleted) == 0);
		}

		if (!is_dead) {
			particle->header.current_tick = inNewTime;
			particle->header.current_time = final_t;

			if (P3gParticleUpdateCallback != NULL) {
				P3gParticleUpdateCallback(inClass, particle);
			}

			if (particle->header.flags & P3cParticleFlag_PlayingAmbientSound) {
				if (particle->header.flags & (P3cParticleFlag_PositionChanged | P3cParticleFlag_AlwaysUpdatePosition)) {
					// update any ambient sounds being played by this particle
					is_dead = P3iAmbientSound_Update(inClass, particle, NULL);
				}
			}

			// particle's position has not changed
			particle->header.flags &= ~P3cParticleFlag_PositionChanged;
		}

		// we are moving to a new particle
		P3gAttractorBuf_Valid = UUcFalse;
		P3gCurrentStep_EndPointValid = UUcFalse;
	}

	// clear P3gCurrentCollision so that it isn't set when we aren't updating
	P3gCurrentCollision.collided = UUcFalse;

#if PARTICLE_PERF_ENABLE
	elapsed_machinetime = UUrMachineTime_High() - start_machinetime;
	inClass->perf_t[P3gPerfIndex] = (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round((float) (P3cPerfTimeScale * UUrMachineTime_High_To_Seconds(elapsed_machinetime)));
#endif

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Exit(P3g_ParticleUpdate);
#endif

	return;
}

// fade out a decal
static UUtBool P3iDecalFade(P3tParticleClass *inClass, P3tParticle *inParticle)
{
	UUtBool returnval;
	float current_alpha;

	if ((inParticle->header.flags & P3cParticleFlag_FadeOut) || 
		(inClass->definition->appearance.decal_fadeframes == 0)) {
		// kill the decal
		P3rKillParticle(inClass, inParticle);
		return UUcTrue;

	} else {
		// calculate the particle's current alpha
		P3mAssertFloatValue(inClass->definition->appearance.alpha);
		returnval = P3iCalculateValue(inParticle, &inClass->definition->appearance.alpha, (void *) &current_alpha);
		if (!returnval)
			return UUcFalse;

		// if we're already faded out, die right here and now
		if (current_alpha <= 0) {
			P3rKillParticle(inClass, inParticle);
			return UUcTrue;
		}

		// set the time remaining in the fade
		inParticle->header.lifetime = ((float) inClass->definition->appearance.decal_fadeframes) / UUcFramesPerSecond;

		// calculate what the fade time would have been from alpha of 1. this is done so that
		// the particle starts from its current alpha.
		inParticle->header.fadeout_time = inParticle->header.lifetime / current_alpha;

		// set the fadeout flag
		inParticle->header.flags |= P3cParticleFlag_FadeOut;

		return UUcFalse;
	}
}

// update a decal class's particles for a certain amount of time
static void P3iUpdateDecalClass(P3tParticleClass *inClass, UUtUns32 inNewTime)
{
	P3tParticle *particle, *next_particle;
	P3tDecalData *decal_data;
	UUtBool is_dead;
	UUtUns16 itr, overflow_particles, max_particles;
	float final_t, delta_t;

	// we should never get into this function unless there is at least one particle
	// of that class - as it checks num_particles > 0
	UUmAssert(inClass->num_particles > 0);
	UUmAssert(inClass->headptr != NULL);
	UUmAssert(inClass->non_persistent_flags & P3cParticleClass_NonPersistentFlag_Active);
	UUmAssert(inClass->definition->flags2 & P3cParticleClassFlag2_Appearance_Decal);

	max_particles = inClass->definition->appearance.max_decals;
	if (max_particles > 0) {
		// maximum number of decal particles is controlled by the particlesystem quality settings
		switch(P3gQuality) {
			case P3cQuality_Medium:
				max_particles /= 2;
			break;

			case P3cQuality_Low:
				max_particles /= 4;
			break;
		}
	}

	final_t = ((float) inNewTime) / UUcFramesPerSecond;
	if ((max_particles == 0) || (inClass->num_particles < max_particles)) {
		overflow_particles = 0;
	} else {
		overflow_particles = (UUtUns16) inClass->num_particles - max_particles;
	}

	// update all particles in this class
	itr = 0;
	particle = inClass->headptr;
	UUmAssert((particle == NULL) || ((particle->header.flags & P3cParticleFlag_Deleted) == 0));

	while (particle != NULL) {
		next_particle = particle->header.nextptr;
		is_dead = UUcFalse;

		decal_data = P3rGetDecalDataPtr(inClass, particle);
		if ((decal_data == NULL) && (decal_data->decal_header == NULL)) {
			// this decal has been deleted, we should never get here but tidy up anyway
			P3rKillParticle(inClass, particle);
			particle = next_particle;
			itr++;
			continue;
		}


		delta_t = final_t - particle->header.current_time;

		UUmAssert((next_particle == NULL) || ((next_particle->header.flags & P3cParticleFlag_Deleted) == 0));
		UUmAssert((particle->header.flags & P3cParticleFlag_Deleted) == 0);
		UUmAssertReadPtr(decal_data->decal_header, sizeof(M3tDecalHeader));

		// has the decal's lifetime expired?
		if (particle->header.lifetime != 0) {
			particle->header.lifetime -= delta_t;
			if (particle->header.lifetime < (1.0f / UUcFramesPerSecond)) {
				particle->header.lifetime = 0;
				
				is_dead = P3iDecalFade(inClass, particle);
			}
		}
		
		if (!is_dead && ((particle->header.flags & P3cParticleFlag_FadeOut) == 0) && (itr < overflow_particles)) {
			// fade out any non-faded particles that are over the limit, starting from the head
			UUmAssertReadPtr(decal_data->decal_header, sizeof(M3tDecalHeader));
			is_dead = P3iDecalFade(inClass, particle);
		}
				
		if (!is_dead) {
			UUmAssertReadPtr(decal_data->decal_header, sizeof(M3tDecalHeader));
			if (particle->header.flags & P3cParticleFlag_FadeOut) 
			{
				if ((decal_data != NULL) && (decal_data->decal_header != NULL)) {
					decal_data->decal_header->alpha = (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(M3cMaxAlpha * particle->header.lifetime
																									/ particle->header.fadeout_time);
				}
			} 

			particle->header.current_tick = inNewTime;
			particle->header.current_time = final_t;
		}

		itr++;
		particle = next_particle;
	}
}

static void P3rBuildPhysicsList(void)
{
	PHtPhysicsContextArray *physics_array = PHgPhysicsContextArray;
	PHtPhysicsContext *context = physics_array->contexts;
	UUtUns32 count = physics_array->num_contexts;
	UUtUns32 itr;

	P3gPhysicsCount = 0;

	for(itr = 0; itr < count; itr++, context++)
	{
		if (((context->flags & PHcFlags_Physics_InUse) == 0) ||
			(context->flags & PHcFlags_Physics_DontBlock) ||
			(context->flags & PHcFlags_Physics_PointCollision) ||
			(context->flags & PHcFlags_Physics_NoCollision))
		{
			continue;
		}

		UUmAssert(P3gPhysicsCount < 256);
		P3gPhysicsList[P3gPhysicsCount++] = context;
	}
}

#if PARTICLE_PERF_ENABLE
void P3rPerformanceToggle(UUtUns32 inMask)
{
	if ((P3gPerfMask & inMask) == inMask) {
		P3gPerfMask &= ~inMask;
	} else {
		P3gPerfMask |= inMask;
	}
}

void P3rPerformanceBegin(UUtUns32 inTime)
{
	P3tParticleClass *particle_class;
	UUtUns16 index, itr;

	if (inTime != P3gPerfTime) {
		P3rPerformanceEnd();
		P3gPerfTime = inTime;

		// go to the next index in the performance buffer
		P3gPerfIndex = (P3gPerfIndex + 1) % P3cPerfHistoryFrames;

		// set up performance for a new tick
		index = P3gFirstActiveClass;
		while (index != P3cClass_None) {
			particle_class = P3gClassTable + index;
			index = particle_class->next_active_class;

			// decrement the performance accumulator with the existing values of the performance counters
			for (itr = 0; itr < P3cPerfEvent_Max; itr++) {
				UUmAssert(particle_class->perf_accum[itr] >= particle_class->perf_data[P3gPerfIndex][itr]);
				particle_class->perf_accum[itr] -= particle_class->perf_data[P3gPerfIndex][itr];
			}
			UUmAssert(particle_class->perf_t_accum >= particle_class->perf_t[P3gPerfIndex]);
			particle_class->perf_t_accum -= particle_class->perf_t[P3gPerfIndex];

			// clear performance counters for this tick
			particle_class->perf_t[P3gPerfIndex] = 0;
			UUrMemory_Clear(&particle_class->perf_data[P3gPerfIndex], P3cPerfEvent_Max * sizeof(UUtUns16));
		}
	}
}

void P3rPerformanceEnd(void)
{
	P3tParticleClass *particle_class;
	UUtUns32 itr, itr2, realmask, memory_watermark, len, avg, spaces, out_spaces;
	char text[512], tempbuf[16], *ptr, *destptr;
	UUtUns16 index;

	// calculate the performance values for this tick
	index = P3gFirstActiveClass;
	while (index != P3cClass_None) {
		particle_class = P3gClassTable + index;
		index = particle_class->next_active_class;

		// increment the performance accumulator with the new values of the performance counters
		for (itr = 0; itr < P3cPerfEvent_Max; itr++) {
			particle_class->perf_accum[itr] += particle_class->perf_data[P3gPerfIndex][itr];
		}
		particle_class->perf_t_accum += particle_class->perf_t[P3gPerfIndex];
	}

	if (P3gPerfDisplay) {
		// set up the header of the performance display area
		UUrString_Copy(P3gPerfDisplayArea[0].text, " *** particles ***", COcMaxStatusLine);

		memory_watermark = (P3gMemory.num_blocks * P3cMemory_BlockSize) - P3gMemory.curblock_free;
		sprintf(text, "total %d, mem %d/%dk (%02d%%f, %02d%%m)",
					P3gTotalParticles, P3gTotalMemoryUsage >> 10, (P3cMemory_BlockSize * P3gMemory.num_blocks) >> 10,
					(memory_watermark == 0) ? 0 : ((100 * P3gTotalMemoryUsage) / memory_watermark),
					(100 * P3gMemory.num_blocks) / P3cMemory_MaxBlocks);
		UUrString_Copy(P3gPerfDisplayArea[1].text, text, COcMaxStatusLine);

		realmask = P3gPerfMask;
		strcpy(text, "particle class      t ");
		for (itr = 0; itr < P3cPerfEvent_Max; itr++) {
			if ((realmask & (1 << itr)) == 0) {
				continue;
			}

			if (strlen(text) + 4 >= COcMaxStatusLine) {
				// no room for this event!
				realmask &= ~(1 << itr);
				continue;
			}

			strcat(text, " ");
			strcat(text, P3cPerfEventName[itr]);
		}
		UUrString_Copy(P3gPerfDisplayArea[2].text, text, COcMaxStatusLine);

		if (!P3gPerfLockClasses) {
			P3gPerfDisplayNumClasses = 0;
			index = P3gFirstActiveClass;
			while (index != P3cClass_None) {
				particle_class = P3gClassTable + index;
				index = particle_class->next_active_class;

				for (itr = 0; itr < P3gPerfDisplayNumClasses; itr++) {
					if (particle_class->perf_t_accum > P3gPerfDisplayClasses[itr]->perf_t_accum) {
						// this class has a higher priority, it is consuming more time
						break;
					}
				}

				if (itr < P3cPerfDisplayClasses) {
					// move all the other particle classes down
					if (P3gPerfDisplayNumClasses < P3cPerfDisplayClasses) {
						P3gPerfDisplayNumClasses++;
					}

					for (itr2 = P3gPerfDisplayNumClasses; itr2 > itr; itr2--) {
						P3gPerfDisplayClasses[itr2] = P3gPerfDisplayClasses[itr2 - 1];
					}
					P3gPerfDisplayClasses[itr] = particle_class;
				}
			}
		}
		
		for (itr = 0; itr < P3gPerfDisplayNumClasses; itr++) {
			particle_class = P3gPerfDisplayClasses[itr];

			avg = (particle_class->perf_t_accum + (P3cPerfHistoryFrames/2)) / P3cPerfHistoryFrames;
			sprintf(text, "%3d %s", avg, particle_class->classname);

			len = strlen(text);
			while (len <= 20) {
				text[len++] = ' ';
			}
			// truncate classname if longer than 16 characters
			text[21] = '\0';

			for (itr2 = 0; itr2 < P3cPerfEvent_Max; itr2++) {
				if ((realmask & (1 << itr2)) == 0) {
					continue;
				}

				avg = (particle_class->perf_accum[itr2] + (P3cPerfHistoryFrames/2)) / P3cPerfHistoryFrames;
				sprintf(tempbuf, " %3d", avg);
				strcat(text, tempbuf);
			}

			UUmAssert(itr + 3 < P3cPerfDisplayLines);
			spaces = 0;
			for (ptr = text, destptr = P3gPerfDisplayArea[itr + 3].text, len = 0; (len < (COcMaxStatusLine - 1)); ptr++) {
				if (*ptr == ' ') {
					spaces++;
				} else {
					out_spaces = MUrUnsignedSmallFloat_To_Uns_Round(1.6f * spaces);
					out_spaces = UUmMin(((COcMaxStatusLine - 1) - len), out_spaces);
					while (out_spaces > 0) {
						*destptr++ = ' ';
						len++;
						out_spaces--;
					}
					UUmAssert(len < (COcMaxStatusLine - 1));
					spaces = 0;

					if (*ptr == '\0') {
						break;
					} else if (len < COcMaxStatusLine - 1) {
						*destptr++ = *ptr;
						len++;
					}
				}
			}
			*destptr = '\0';
		}

		for ( ; (itr + 3) < P3cPerfDisplayLines; itr++) {
			// clear the remaining lines
			P3gPerfDisplayArea[itr + 3].text[0] = '\0';
		}
	}
}
#endif

// update all particles. called every tick for essential particles, called once every display update
// for decorative particles
void P3rUpdate(UUtUns32 inNewTime, UUtBool inEssential)
{
	P3tParticleClass *particle_class;
	UUtUns16 index;

#if PARTICLE_PROFILE_UPDATE
	UUrProfile_State_Set(UUcProfile_State_On);
#endif

#if PARTICLE_PERF_ENABLE
	P3rPerformanceBegin(inNewTime);
#endif

	P3mVerifyDecals();

	if (P3gRemoveDangerousProjectiles == UUcTrue) {
		P3iRemoveDangerousProjectiles();
	}

	if (!inEssential) {
		if (inNewTime == P3gLastDecorativeTick) {
			// we don't need to update for zero time
			return;
		} else {
			P3gLastDecorativeTick = inNewTime;
		}
	}

	P3rBuildPhysicsList();
	P3gApproxCurrentTime = ((float) inNewTime) / UUcFramesPerSecond;

	P3iActiveList_Lock();

	// essential particle classes are placed at the front of this list, decorative ones at the end of the list
	// therefore if we start from the relevant end, we can stop when we reach one that is incorrect
	if (inEssential) {
		index = P3gFirstActiveClass;
		while (index != P3cClass_None) {
			particle_class = P3gClassTable + index;
			index = particle_class->next_active_class;

			if (particle_class->definition->flags & P3cParticleClassFlag_Decorative)
				break;

			if ((particle_class->non_persistent_flags & P3cParticleClass_NonPersistentFlag_DeferredRemoval) == 0) {
				P3iUpdateParticleClass(particle_class, inNewTime);
			}
		}
	} else {
		index = P3gLastActiveClass;
		while (index != P3cClass_None) {
			particle_class = P3gClassTable + index;
			index = particle_class->prev_active_class;

			if ((particle_class->definition->flags & P3cParticleClassFlag_Decorative) == 0)
				break;

			if ((particle_class->non_persistent_flags & P3cParticleClass_NonPersistentFlag_DeferredRemoval) == 0) {
				if (particle_class->definition->flags2 & P3cParticleClassFlag2_Appearance_Decal) {
					P3iUpdateDecalClass(particle_class, inNewTime);
				} else {
					P3iUpdateParticleClass(particle_class, inNewTime);
				}
			}
		}
	}

	P3iActiveList_Unlock();

#if PARTICLE_PROFILE_UPDATE
	UUrProfile_State_Set(UUcProfile_State_Off);
#endif
}

/*
 * interface
 */

// iterate over particle classes
UUtBool P3rIterateClasses(P3tParticleClassIterateState *ioState,
						  P3tParticleClass **			outClass)
{
	if (*ioState < P3gNumClasses) {
		*outClass = &P3gClassTable[*ioState];

		*ioState += 1;
		return UUcTrue;
	} else {
		*ioState = 0;
		return UUcFalse;
	}
}

// build a binary-data representation of a particle class
UUtUns8 *P3rBuildBinaryData(UUtBool					inSwapBytes,
							P3tParticleDefinition *	inDefinition,
							UUtUns16 *				outDataSize)
{
	P3tParticleDefinition *	new_def;

	*outDataSize = sizeof(P3tParticleDefinition)
			+ inDefinition->num_variables * sizeof(P3tVariableInfo)
			+ inDefinition->num_actions   * sizeof(P3tActionInstance)
			+ inDefinition->num_emitters  * sizeof(P3tEmitter);

	new_def = (P3tParticleDefinition *) UUrMemory_Block_New(*outDataSize);
	UUrMemory_MoveFast((void *) inDefinition, (void *) new_def, *outDataSize);

	if (inSwapBytes) {
		P3iSwap_ParticleDefinition((void *) new_def, UUcTrue);
	}

	return (UUtUns8 *) new_def;
}

// mark that a particle's definition has been changed
void P3rMakeDirty(P3tParticleClass *ioClass, UUtBool inDirty)
{
	UUmAssertReadPtr(ioClass, sizeof(P3tParticleClass));

	if (inDirty) {
		if (ioClass->non_persistent_flags & P3cParticleClass_NonPersistentFlag_Dirty) {
			// already dirty
			return;
		}

		P3gDirty = UUcTrue;

		UUmAssert(ioClass->originalcopy.block == NULL);

		// allocate a copy of the particle class so that we
		// can revert to its original state if necessary
		ioClass->originalcopy.size = ioClass->definition->definition_size;
		ioClass->originalcopy.block = UUrMemory_Block_New(ioClass->originalcopy.size + sizeof(BDtHeader));
		UUrMemory_MoveFast(ioClass->definition, P3mSkipHeader(ioClass->originalcopy.block),
							ioClass->originalcopy.size);

		ioClass->non_persistent_flags |= P3cParticleClass_NonPersistentFlag_Dirty;
	} else {
		if ((ioClass->non_persistent_flags & P3cParticleClass_NonPersistentFlag_Dirty) == 0) {
			// wasn't dirty to begin with
			return;
		}

		// this class has been saved to disk and so is no longer dirty.
		// we no longer need the copy of its original state
		if (ioClass->originalcopy.block != NULL) {
			UUrMemory_Block_Delete(ioClass->originalcopy.block);
		}
		ioClass->originalcopy.size	= 0;
		ioClass->originalcopy.block = NULL;

		ioClass->non_persistent_flags &= ~P3cParticleClass_NonPersistentFlag_Dirty;
	}
}

// are there any dirty particles? does not check each individualy
UUtBool P3rAnyDirty(void)
{
	return P3gDirty;
}

// search all particles to determine if there are any dirty
UUtBool P3rUpdateDirty(void)
{
	UUtUns16	itr;
	UUtBool		newDirty = UUcFalse;
	UUtBool		return_changed;

	for (itr = 0; itr < P3gNumClasses; itr++) {
		if (P3gClassTable[itr].non_persistent_flags & P3cParticleClass_NonPersistentFlag_Dirty) {
			newDirty = UUcTrue;
		}
	}

	return_changed = (newDirty == P3gDirty) ? UUcFalse : UUcTrue;

	P3gDirty = newDirty;

	return return_changed;
}

// is a particle dirty?
UUtBool P3rIsDirty(P3tParticleClass *inClass)
{
	return ((inClass->non_persistent_flags & P3cParticleClass_NonPersistentFlag_Dirty) > 0);
}

// revert a particle to its previous state
UUtBool P3rRevert(P3tParticleClass *ioClass)
{
	void *mem_block;

	UUmAssertReadPtr(ioClass, sizeof(P3tParticleClass));

	if ((ioClass->non_persistent_flags & P3cParticleClass_NonPersistentFlag_Dirty) == 0) {
		// hasn't changed since being loaded
		return UUcFalse;
	}

	if (ioClass->originalcopy.block == NULL) {
		// this class is dirty, but it has no saved copy to revert to
		// this implies that it was created from scratch rather than
		// loaded from disk
		return UUcFalse;
	}

	// get rid of the memory allocated earlier
	mem_block = ioClass->memory.block;
	if (mem_block != NULL)
		UUrMemory_Block_Delete(mem_block);

	// the main particle class is now the copy that we made earlier.
	// we no longer have a second copy	
	ioClass->memory = ioClass->originalcopy;
	ioClass->originalcopy.size = 0;
	ioClass->originalcopy.block = NULL;

	// skip over the BDtHeader stored at the front of the block
	// to get to the particle data
	ioClass->definition = P3mSkipHeader(ioClass->memory.block);

	// remember to set up the variable-array pointers
	P3iCalculateArrayPointers(ioClass->definition);

	// we are no longer dirty, we have reverted
	ioClass->non_persistent_flags &= ~P3cParticleClass_NonPersistentFlag_Dirty;

	return UUcTrue;
}

// print out a variable for the variable list
void P3rDescribeVariable(P3tParticleClass *inClass, P3tVariableInfo *inVar, char *outString)
{
	UUtUns16 itr;

	// describe the variable type, pad out to 20 columns
	P3rDescribeDataType(inVar->type, outString);
	strcat(outString, " ");
	for (itr = strlen(outString); itr < 20; itr++)
		outString[itr] = ' ';
	outString[itr] = '\0';

	// concatenate the variable name, pad out to 40 columns
	strcat(outString, inVar->name);
	strcat(outString, " ");
	for (itr = strlen(outString); itr < 40; itr++)
		outString[itr] = ' ';
	outString[itr] = '\0';

	// describe the variable's initial value
	strcat(outString, "[");
	P3rDescribeValue(inClass, inVar->type, &inVar->initialiser, &outString[itr + 1]);
	strcat(outString, "]");
}

// print out a variable type for the variable list
void P3rDescribeDataType(P3tDataType inVarType, char *outString)
{
	UUtUns16 enum_index;

	UUmAssertWritePtr(outString, P3cStringVarSize);

	switch(inVarType) {
	case P3cDataType_Integer:
		strcpy(outString, "integer");
		break;

	case P3cDataType_Float:
		strcpy(outString, "number");
		break;

	case P3cDataType_String:
	case P3cDataType_Sound_Ambient:
	case P3cDataType_Sound_Impulse:
		strcpy(outString, "string");
		break;

	case P3cDataType_Shade:
		strcpy(outString, "color");
		break;

	default:
		if (inVarType & P3cDataType_Enum) {
			enum_index = (inVarType & ~P3cDataType_Enum) >> P3cEnumShift;
			if ((enum_index >= 0) && (enum_index < P3cEnumType_Max >> P3cEnumShift)) {
				strcpy(outString, P3gEnumTypeInfo[enum_index].name);
				break;
			}
		}

		strcpy(outString, "UNKNOWN");
		break;
	}
}

// print out a value to a string
UUtBool P3rDescribeValue(P3tParticleClass *inClass, P3tDataType inType, P3tValue *inValue, char *outString)
{
	float shade1[4], shade2[4];

	switch(inValue->type) {
	case P3cValueType_Variable:
		strcpy(outString, inValue->u.variable.name);
		break;

	case P3cValueType_Integer:
		sprintf(outString, "%d", inValue->u.integer_const.val);
		break;

	case P3cValueType_IntegerRange:
		sprintf(outString, "range %d - %d", inValue->u.integer_range.val_low, inValue->u.integer_range.val_hi);
		break;

	case P3cValueType_Float:
		sprintf(outString, "%.3f", inValue->u.float_const.val);
		break;

	case P3cValueType_FloatRange:
		sprintf(outString, "range %.3f - %.3f", inValue->u.float_range.val_low, inValue->u.float_range.val_hi);
		break;

	case P3cValueType_FloatBellcurve:
		sprintf(outString, "%.3f std.dev %.3f", inValue->u.float_bellcurve.val_mean,
				inValue->u.float_bellcurve.val_stddev);
		break;

	case P3cValueType_String:
		sprintf(outString, "\"%s\"", inValue->u.string_const.val);
		break;

	case P3cValueType_Shade:
		IMrPixel_Decompose(IMrPixel_FromShade(IMcPixelType_ARGB8888, inValue->u.shade_const.val),
							IMcPixelType_ARGB8888,
							&shade1[0], &shade1[1], &shade1[2], &shade1[3]);
		sprintf(outString, "a%.2f r%.2f g%.2f b%.2f", shade1[0], shade1[1], shade1[2], shade1[3]);
		break;

	case P3cValueType_ShadeRange:
		IMrPixel_Decompose(IMrPixel_FromShade(IMcPixelType_ARGB8888, inValue->u.shade_range.val_low),
							IMcPixelType_ARGB8888,
							&shade1[0], &shade1[1], &shade1[2], &shade1[3]);

		IMrPixel_Decompose(IMrPixel_FromShade(IMcPixelType_ARGB8888, inValue->u.shade_range.val_hi),
							IMcPixelType_ARGB8888,
							&shade2[0], &shade2[1], &shade2[2], &shade2[3]);

		sprintf(outString, "range (a%.2f r%.2f g%.2f b%.2f) - (a%.2f r%.2f g%.2f b%.2f)",
				shade1[0], shade1[1], shade1[2], shade1[3],
				shade2[0], shade2[1], shade2[2], shade2[3]);
		break;

	case P3cValueType_ShadeBellcurve:
		IMrPixel_Decompose(IMrPixel_FromShade(IMcPixelType_ARGB8888, inValue->u.shade_bellcurve.val_mean),
							IMcPixelType_ARGB8888,
							&shade1[0], &shade1[1], &shade1[2], &shade1[3]);

		IMrPixel_Decompose(IMrPixel_FromShade(IMcPixelType_ARGB8888, inValue->u.shade_bellcurve.val_stddev),
							IMcPixelType_ARGB8888,
							&shade2[0], &shade2[1], &shade2[2], &shade2[3]);

		sprintf(outString, "(a%.2f r%.2f g%.2f b%.2f) std.dev (a%.2f r%.2f g%.2f b%.2f)",
				shade1[0], shade1[1], shade1[2], shade1[3],
				shade2[0], shade2[1], shade2[2], shade2[3]);
		break;

	case P3cValueType_Enum:
		if (P3rDescribeEnum(inClass, (P3tEnumType)inType, inValue->u.enum_const.val, outString) != UUcTrue) {
			sprintf(outString, "UNKNOWN ENUM %d/%d", inType, inValue->u.enum_const.val);
			return UUcFalse;
		}
		break;

	case P3cValueType_FloatTimeBased:
		sprintf(outString, "cycle, length %.3f max %.3f",
				inValue->u.float_timebased.cycle_length, inValue->u.float_timebased.cycle_max);
		break;

	default:
		sprintf(outString, "UNKNOWN VALTYPE %d", inType);
		return UUcFalse;
	}

	return UUcTrue;
}

// print out a description of an emitter's link type
UUtBool P3rDescribeEmitterLinkType(P3tParticleClass *inClass, P3tEmitter *inEmitter, P3tEmitterLinkType inLinkType,
								char *outString)
{
	UUtUns32 emitter_index;

	switch(inLinkType) {
	case P3cEmitterLinkType_None:
		strcpy(outString, "none");
		return UUcTrue;
			
	case P3cEmitterLinkType_Emitter:
		strcpy(outString, "back to emitter");
		return UUcTrue;

	case P3cEmitterLinkType_OurLink:
		strcpy(outString, "same as us");
		return UUcTrue;
			
	default:
		emitter_index = inLinkType - P3cEmitterLinkType_LastEmission;
		if ((emitter_index < 0) || (emitter_index >= inClass->definition->num_emitters)) {
			if ((emitter_index >= 0) && (emitter_index < P3cMaxEmitters)) {
				sprintf(outString, "<no such emitter: %d>", emitter_index);
			} else {
				sprintf(outString, "<unknown link type: %d>", inLinkType);
			}
			return UUcFalse;
		} else {
			sprintf(outString, "last particle emitted from %d: %s", 
				emitter_index, inClass->definition->emitter[emitter_index].classname);
			return UUcTrue;
		}
		break;
	}
}

// allocate space for a new variable in a particle definition
UUtError P3rClass_NewVar(P3tParticleClass *ioClass, P3tVariableInfo **outVar)
{
	UUtUns16 copyoffset, new_size;
	UUtError error;
	UUtUns8 *copyfrom, *copyto;
	P3tParticleDefinition *def;

	UUmAssertReadPtr(ioClass, sizeof(P3tParticleClass));

	UUmAssert(P3mVerifyDefinitionSize(ioClass->definition));

	// resize the particle definition to accomodate the new variable
	new_size = ioClass->definition->definition_size + sizeof(P3tVariableInfo);
	error = P3iResizeParticleDefinition(&ioClass->definition, &ioClass->memory, new_size, UUcTrue);
	UUmError_ReturnOnErrorMsgP(error, "can't resize particle definition to %d bytes", new_size, 0, 0);
	def = ioClass->definition;

	// place the new variable
	*outVar = &(def->variable[def->num_variables++]);

	// all data following this variable must now be moved down.
	copyoffset = sizeof(P3tParticleDefinition) + def->num_variables * sizeof(P3tVariableInfo);
	copyto = ((UUtUns8 *) def) + copyoffset;
	copyfrom = copyto - sizeof(P3tVariableInfo);

	// move definition_size minus this much
	UUrMemory_MoveOverlap((void *) copyfrom, (void *) copyto, def->definition_size - copyoffset);

	// zero the new space
	memset((void *) *outVar, 0x00, sizeof(P3tVariableInfo));

	// recalculate the array pointers because data has moved
	P3iCalculateArrayPointers(def);

	UUmAssert(P3mVerifyDefinitionSize(def));

	return UUcError_None;
}

// set up the default initialiser for a newly created variable
void P3rSetDefaultInitialiser(P3tDataType inVarType, P3tValue *outValue)
{
	UUtUns16	enum_index;

	switch(inVarType) {
	case P3cDataType_None:
	case P3cDataType_Integer:
		outValue->type = P3cValueType_Integer;
		outValue->u.integer_const.val = 0;
		return;

	case P3cDataType_Float:
		outValue->type = P3cValueType_Float;
		outValue->u.float_const.val = 0.0f;
		return;

	case P3cDataType_String:
	case P3cDataType_Sound_Ambient:
	case P3cDataType_Sound_Impulse:
		outValue->type = P3cValueType_String;
		outValue->u.string_const.val[0] = '\0';
		return;

	case P3cDataType_Shade:
		outValue->type = P3cValueType_Shade;
		outValue->u.shade_const.val = IMcShade_Red;
		return;

	default:
		if (inVarType & P3cDataType_Enum) {
			outValue->type = P3cValueType_Enum;

			enum_index = (inVarType & ~P3cDataType_Enum) >> P3cEnumShift;
			if ((enum_index >= 0) && (enum_index < P3cEnumType_Max >> P3cEnumShift)) {
				outValue->u.enum_const.val = P3gEnumTypeInfo[enum_index].defaultval;
				return;
			}
		}

		UUmAssert(!"unknown data type");
	}
}

// recurse over all action lists in the particle definition, and update their indices
// so that they are still correct after a grow or shrink operation.
static void P3iUpdateActionLists(P3tParticleDefinition *inDefinition, UUtUns16 inListIndex, 
						  UUtUns16 inLocationIndex, UUtInt32 inOffsetIndex)
{
	UUtUns16 itr;

	UUmAssert((inListIndex >= 0) && (inListIndex < P3cMaxNumEvents));

	// update the list that is growing or shrinking.
	UUmAssert(inDefinition->eventlist[inListIndex].start_index	<= inLocationIndex);
	UUmAssert(inDefinition->eventlist[inListIndex].end_index	>= inLocationIndex);

	inDefinition->eventlist[inListIndex].end_index
		= (UUtUns16) (inDefinition->eventlist[inListIndex].end_index + inOffsetIndex);

	// update all lists after this
	for (itr = inListIndex + 1; itr < P3cMaxNumEvents; itr++) {
		inDefinition->eventlist[itr].start_index
			= (UUtUns16) (inDefinition->eventlist[itr].start_index + inOffsetIndex);

		inDefinition->eventlist[itr].end_index
			= (UUtUns16) (inDefinition->eventlist[itr].end_index + inOffsetIndex);
	}
}

// grow an action list. requires that we resize the particle definition, and move down
// every action list below the point that's growing.
UUtError P3rClass_NewAction(P3tParticleClass *ioClass, UUtUns16 inListIndex, UUtUns16 inActionIndex)
{
	UUtUns16 copyoffset, old_size, new_size;
	UUtUns32 actionmask;
	UUtError error;
	UUtUns8 *copyfrom, *copyto;
	P3tParticleDefinition *def;

	UUmAssertReadPtr(ioClass, sizeof(P3tParticleClass));

	UUmAssert(P3mVerifyDefinitionSize(ioClass->definition));
	UUmAssert((inActionIndex >= 0) && (inActionIndex <= ioClass->definition->num_actions));
	UUmAssert((inActionIndex >= ioClass->definition->eventlist[inListIndex].start_index) && 
			  (inActionIndex <= ioClass->definition->eventlist[inListIndex].end_index));

	// resize the particle class to accomodate the new action
	old_size = ioClass->definition->definition_size;
	new_size = old_size + sizeof(P3tActionInstance);
	error = P3iResizeParticleDefinition(&ioClass->definition, &ioClass->memory, new_size, UUcTrue);
	UUmError_ReturnOnErrorMsgP(error, "can't resize particle class to %d bytes", new_size, 0, 0);
	def = ioClass->definition;

	// all data following the location of the new action must now be moved.
	copyoffset = sizeof(P3tParticleDefinition) + def->num_variables * sizeof(P3tVariableInfo)
										       + inActionIndex * sizeof(P3tActionInstance);
	copyfrom = ((UUtUns8 *) def) + copyoffset;
	copyto = copyfrom + sizeof(P3tActionInstance);

	// move old_size minus this much, and zero the new space created
	UUrMemory_MoveOverlap((void *) copyfrom, (void *) copyto, old_size - copyoffset);
	memset((void *) copyfrom, 0x00, sizeof(P3tActionInstance));

	// recalculate the array pointers because data has moved
	def->num_actions++;
	P3iCalculateArrayPointers(def);

	UUmAssert(P3mVerifyDefinitionSize(def));

	// update all of the actionlist indices so that they still point
	// to the same locations
	P3iUpdateActionLists(def, inListIndex, inActionIndex, +1);

	if ((inListIndex == P3cEvent_Update) && (inActionIndex >= 0) && (inActionIndex < 32)){
		// move the upper portion of the actionmask along one
		actionmask = ioClass->definition->actionmask & (0xFFFFFFFF << inActionIndex);
		actionmask <<= 1;

		// turn on the new action by default, and add in the remainder of the actionmask
		actionmask |= (1 << inActionIndex);
		actionmask |= ioClass->definition->actionmask & ((1 << inActionIndex) - 1);
		ioClass->definition->actionmask = actionmask;
	}

	return UUcError_None;
}

// set up the defaults for a newly created action
void P3rSetActionDefaults(P3tActionInstance *ioAction)
{
	P3tActionTemplate *action_template;
	P3tDataType value_type;
	UUtUns16 itr;

	UUmAssertReadPtr(ioAction, sizeof(P3tActionInstance));
	UUmAssert((ioAction->action_type >= 0) && (ioAction->action_type < P3cNumActionTypes));

	action_template = &P3gActionInfo[ioAction->action_type];
	UUmAssert(action_template->type == ioAction->action_type);		// i.e. not a menu divider line (has type -1)

	for (itr = 0; itr < P3cActionMaxParameters; itr++) {
		// set up an empty variable reference
		strcpy(ioAction->action_var[itr].name, "");
		ioAction->action_var[itr].offset = P3cNoVar;
		ioAction->action_var[itr].type = action_template->info[itr].datatype;
	}

	for (itr = 0; itr < P3cActionMaxParameters; itr++) {
		// set up an empty parameter
		value_type = (itr < action_template->num_parameters) ? action_template->info[action_template->num_variables + itr].datatype : P3cDataType_None;
		P3rSetDefaultInitialiser(value_type, &ioAction->action_value[itr]);
	}
}

// allocate space for a new emitter in a particle definition
UUtError P3rClass_NewEmitter(P3tParticleClass *ioClass, P3tEmitter **outEmitter)
{
	UUtUns16 copyoffset, new_size;
	UUtError error;
	UUtUns8 *copyfrom, *copyto;
	P3tParticleDefinition *def;

	UUmAssertReadPtr(ioClass, sizeof(P3tParticleClass));

	UUmAssert(P3mVerifyDefinitionSize(ioClass->definition));

	if (ioClass->definition->num_emitters >= P3cMaxEmitters) {
		// can't allocate more than the fixed limit of emitters
		return UUcError_Generic;
	}

	// resize the particle definition to accomodate the new variable
	new_size = ioClass->definition->definition_size + sizeof(P3tEmitter);
	error = P3iResizeParticleDefinition(&ioClass->definition, &ioClass->memory, new_size, UUcTrue);
	UUmError_ReturnOnErrorMsgP(error, "can't resize particle definition to %d bytes", new_size, 0, 0);
	def = ioClass->definition;

	// place the new emitter
	*outEmitter = &(def->emitter[def->num_emitters++]);

	// all data following this variable must now be moved down.
	copyoffset = sizeof(P3tParticleDefinition) + def->num_variables	* sizeof(P3tVariableInfo)
												+ def->num_actions		* sizeof(P3tActionInstance)
												+ def->num_emitters		* sizeof(P3tEmitter);
	copyto = ((UUtUns8 *) def) + copyoffset;
	copyfrom = copyto - sizeof(P3tEmitter);

	// move definition_size minus this much
	UUrMemory_MoveOverlap((void *) copyfrom, (void *) copyto, def->definition_size - copyoffset);

	// zero the new space
	memset((void *) *outEmitter, 0x00, sizeof(P3tEmitter));

	// set default values for those emitter parameters that aren't default 0
	(*outEmitter)->emit_chance = UUcMaxUns16;
	(*outEmitter)->orientdir_type = P3cEmitterOrientation_ParentPosY;
	(*outEmitter)->orientup_type = P3cEmitterOrientation_ParentPosZ;

	// recalculate the array pointers because data has moved
	P3iCalculateArrayPointers(def);

	UUmAssert(P3mVerifyDefinitionSize(def));

	return UUcError_None;
}

// delete a variable from a particle definition
UUtError P3rClass_DeleteVar(P3tParticleClass *ioClass, UUtUns16 inVarNum)
{
	UUtUns16 copyoffset, new_size;
	UUtError error;
	UUtUns8 *copyfrom, *copyto;
	P3tParticleDefinition *def;

	UUmAssertReadPtr(ioClass, sizeof(P3tParticleClass));
	def = ioClass->definition;

	UUmAssert((inVarNum >= 0) && (inVarNum < def->num_variables));

	UUmAssert(P3mVerifyDefinitionSize(def));

	// all data following this variable must be moved up.
	copyoffset = sizeof(P3tParticleDefinition) + (inVarNum + 1) * sizeof(P3tVariableInfo);
	copyfrom = ((UUtUns8 *) def) + copyoffset;
	copyto = copyfrom - sizeof(P3tVariableInfo);

	// move definition_size minus this much
	UUrMemory_MoveOverlap((void *) copyfrom, (void *) copyto, def->definition_size - copyoffset);

	// now one less variable
	def->num_variables--;

	// resize the particle class - we no longer need this variable
	new_size = def->definition_size - sizeof(P3tVariableInfo);
	error = P3iResizeParticleDefinition(&ioClass->definition, &ioClass->memory, new_size, UUcTrue);
	UUmError_ReturnOnErrorMsgP(error, "can't resize particle class to %d bytes", new_size, 0, 0);

	// recalculate the array pointers because data has moved
	P3iCalculateArrayPointers(ioClass->definition);

	UUmAssert(P3mVerifyDefinitionSize(ioClass->definition));

	return UUcError_None;
}

// delete an action from a particle class
UUtError P3rClass_DeleteAction(P3tParticleClass *ioClass, UUtUns16 inListIndex, UUtUns16 inActionIndex)
{
	UUtUns16 copyoffset, new_size;
	UUtError error;
	UUtUns8 *copyfrom, *copyto;
	P3tParticleDefinition *def;

	UUmAssertReadPtr(ioClass, sizeof(P3tParticleClass));
	def = ioClass->definition;

	UUmAssert((inActionIndex >= 0) && (inActionIndex < def->num_actions));
	UUmAssert((inListIndex >= 0) && (inListIndex < P3cMaxNumEvents));
	UUmAssert((inActionIndex >= ioClass->definition->eventlist[inListIndex].start_index) && 
			  (inActionIndex <  ioClass->definition->eventlist[inListIndex].end_index));

	UUmAssert(P3mVerifyDefinitionSize(def));

	// all data following this action must be moved up.
	copyoffset = sizeof(P3tParticleDefinition) + def->num_variables * sizeof(P3tVariableInfo)
										  + (inActionIndex + 1) * sizeof(P3tActionInstance);
	copyfrom = ((UUtUns8 *) def) + copyoffset;
	copyto = copyfrom - sizeof(P3tActionInstance);

	// move definition_size minus this much
	UUrMemory_MoveOverlap((void *) copyfrom, (void *) copyto, def->definition_size - copyoffset);

	// now one less action
	def->num_actions--;

	// resize the particle class - we no longer need this action
	new_size = def->definition_size - sizeof(P3tActionInstance);
	error = P3iResizeParticleDefinition(&ioClass->definition, &ioClass->memory, new_size, UUcTrue);
	UUmError_ReturnOnErrorMsgP(error, "can't resize particle class to %d bytes", new_size, 0, 0);

	// recalculate the array pointers because data has moved
	P3iCalculateArrayPointers(ioClass->definition);

	UUmAssert(P3mVerifyDefinitionSize(ioClass->definition));

	// update all of the actionlist indices so that they still point
	// to the same locations
	P3iUpdateActionLists(def, inListIndex, inActionIndex, -1);

	return UUcError_None;
}

// delete an emitter from a particle definition
UUtError P3rClass_DeleteEmitter(P3tParticleClass *ioClass, UUtUns16 inEmitterNum)
{
	UUtUns16 copyoffset, new_size;
	UUtError error;
	UUtUns8 *copyfrom, *copyto;
	P3tParticleDefinition *def;

	UUmAssertReadPtr(ioClass, sizeof(P3tParticleClass));
	def = ioClass->definition;

	UUmAssert((inEmitterNum >= 0) && (inEmitterNum < def->num_emitters));

	UUmAssert(P3mVerifyDefinitionSize(def));

	// all data following this variable must be moved up.
	copyoffset = sizeof(P3tParticleDefinition) + def->num_variables * sizeof(P3tVariableInfo)
												 + def->num_actions * sizeof(P3tActionInstance)
												 + (inEmitterNum + 1) * sizeof(P3tEmitter);
	copyfrom = ((UUtUns8 *) def) + copyoffset;
	copyto = copyfrom - sizeof(P3tEmitter);

	// move definition_size minus this much
	UUrMemory_MoveOverlap((void *) copyfrom, (void *) copyto, def->definition_size - copyoffset);

	// now one less emitter
	def->num_emitters--;

	// resize the particle class - we no longer need this emitter
	new_size = def->definition_size - sizeof(P3tEmitter);
	error = P3iResizeParticleDefinition(&ioClass->definition, &ioClass->memory, new_size, UUcTrue);
	UUmError_ReturnOnErrorMsgP(error, "can't resize particle class to %d bytes", new_size, 0, 0);

	// recalculate the array pointers because data has moved
	P3iCalculateArrayPointers(ioClass->definition);

	UUmAssert(P3mVerifyDefinitionSize(ioClass->definition));

	return UUcError_None;
}

// remap the variables in all particles of a class
static void P3iRemapVariables(P3tParticleClass *ioClass, UUtUns16 new_vararray_size, UUtUns16 *remap_table)
{
	P3tParticle *particle;
	UUtUns16 itr, fromindex;

	for (particle = ioClass->headptr; particle != NULL; particle = particle->header.nextptr) {
		// make two passes through the remapping array. first, move all elements up in a pass from the start
		// to the end. then, move all elements down and zero-fill in another pass from the end to the start.
		for (itr = 0; itr < new_vararray_size; itr++) {
			fromindex = remap_table[itr];
			if ((fromindex != P3cNoVar) && (fromindex > itr))
				particle->vararray[itr] = particle->vararray[fromindex];
		}
		do {
			itr--;
			fromindex = remap_table[itr];
			if (fromindex == P3cNoVar) {
				particle->vararray[itr] = 0x00;
			} else if (fromindex < itr) {
				particle->vararray[itr] = particle->vararray[fromindex];
			}
		} while (itr > 0);
	}
}

// a particle class is changing to a larger size class and all of its particles
// must be reallocated in the new size class.
static void P3iReallocateParticles(P3tParticleClass *ioClass, UUtUns16 new_size_class,
							UUtUns16 new_vararray_size, UUtUns16 *remap_table)
{
	UUtUns16 old_size_class, from_index, itr;
	P3tSizeClassInfo *old_sci, *new_sci;
	P3tParticle *particle, *next_particle, *new_particle;

	old_size_class = ioClass->size_class;
	old_sci = &P3gMemory.sizeclass_info[old_size_class];

	ioClass->size_class = new_size_class;
	new_sci = &P3gMemory.sizeclass_info[new_size_class];

	// make a new copy of every existing particle
	for (particle = ioClass->headptr; particle != NULL; particle = next_particle) {
		next_particle = particle->header.nextptr;

		new_particle = P3rNewParticle(ioClass);
		new_particle->header = particle->header;
		
		// map the new particle's variables to the old values
		for (itr = 0; itr < new_vararray_size; itr++) {
			from_index = remap_table[itr];
			if (from_index == P3cNoVar) {
				new_particle->vararray[itr] = 0x00;
			} else {
				new_particle->vararray[itr] = particle->vararray[from_index];
			}
		}

		// insert this particle into the class's linked list instead
		if (new_particle->header.prevptr == NULL) {
			ioClass->headptr = new_particle;
		} else {
			new_particle->header.prevptr->header.nextptr = new_particle;
		}
		if (new_particle->header.nextptr == NULL) {
			ioClass->tailptr = new_particle;
		} else {
			new_particle->header.nextptr->header.prevptr = new_particle;
		}

		if (ioClass->definition->flags & P3cParticleClassFlag_Decorative) {
			// remove the old particle from the old decorative-particle list
			if (new_particle->header.prev_decorative == NULL) {
				old_sci->decorative_head = new_particle->header.next_decorative;
			} else {
				new_particle->header.prev_decorative->header.next_decorative
					= new_particle->header.next_decorative;
			}
			if (new_particle->header.next_decorative == NULL) {
				old_sci->decorative_tail = new_particle->header.prev_decorative;
			} else {
				new_particle->header.next_decorative->header.prev_decorative
					= new_particle->header.prev_decorative;
			}
			
			// add the new particle to the tail of the new decorative-particle list
			new_particle->header.prev_decorative = new_sci->decorative_tail;
			if (new_particle->header.prev_decorative == NULL) {
				new_sci->decorative_head = new_particle;
			} else {
				new_particle->header.prev_decorative->header.next_decorative = new_particle;
			}
			new_particle->header.next_decorative = NULL;
			new_sci->decorative_tail = new_particle;
		}

		// remove the old particle from the particle memory. we can't use
		// P3rMarkParticleAsFree because ioClass->size_class is the new size class.
		particle->header.nextptr = old_sci->freelist_head;
		old_sci->freelist_head = particle;
		old_sci->freelist_length++;
	}
}

static P3tParticleClass *P3gUpdateOffsetClass = NULL;

// update variable offsets for every variable reference in a particle class.
// this function called by P3rTraverseVarRef for each var ref
static void P3iTraverseVarUpdateOffsets(P3tVariableReference *inVarRef, char *inLocation)
{
	P3iFindVariable(P3gUpdateOffsetClass, inVarRef);
}

// re-pack the variables in the particle class.
void P3rPackVariables(P3tParticleClass *ioClass, UUtBool inChangedVars)
{
	UUtUns16	current_length, itr, itr2, new_particle_size, new_size_class;
	UUtUns16	newoptional_offset[P3cNumParticleOptionalVars], newvar_offset[64];
	UUtUns16	*remap_table, num_emitters_to_copy;

	UUmAssertReadPtr(ioClass, sizeof(P3tParticleClass));
	UUmAssert(ioClass->definition->num_variables <= 64);

	// we have to set up all of the VariableInfo structures with
	// the offset to each particular variable.
	current_length = 0;

	// account for the emitters' data
	current_length += ioClass->definition->num_emitters * sizeof(P3tParticleEmitterData);

	// check to see if this particle class requires this particular optional variable
	for (itr = 0; itr < P3cNumParticleOptionalVars; itr++) {
		if (ioClass->definition->flags & (1 << (P3cParticleClassFlags_OptionShift + itr))) {
			newoptional_offset[itr] = current_length;
			current_length += P3gOptionalVarSize[itr];
		} else {
			newoptional_offset[itr] = P3cNoVar;
		}
	}

	// fill in all of the custom variables
	for (itr = 0; itr < ioClass->definition->num_variables; itr++) {
		newvar_offset[itr] = current_length;
		current_length += P3iGetDataSize(ioClass->definition->variable[itr].type);
	}

	// NB: particle_size is the whole particle including header and vararray
	new_particle_size = current_length + sizeof(P3tParticleHeader);

	// if the variables are changing around (i.e. we aren't just packing them for
	// the first time at level load) then we may need to move data around inside
	// existing particles.
	if (inChangedVars) {
		// calculate a remapping table for the new particle's vararray
		remap_table = (UUtUns16 *) UUrMemory_Block_New(current_length * sizeof(UUtUns16));
		UUrMemory_Set16(remap_table, P3cNoVar, current_length * sizeof(UUtUns16));		// means initialize to zero

		// the emitter data that was already here must stay intact
		num_emitters_to_copy = ioClass->prev_packed_emitters;
		if (num_emitters_to_copy > ioClass->definition->num_emitters)
			num_emitters_to_copy = ioClass->definition->num_emitters;

		for (itr = 0; itr < num_emitters_to_copy * sizeof(P3tParticleEmitterData); itr++) {
			remap_table[itr] = itr;
		}

		for (itr = 0; itr < P3cNumParticleOptionalVars; itr++) {
			if (newoptional_offset[itr] != P3cNoVar) {
				if (ioClass->optionalvar_offset[itr] != P3cNoVar) {
					// this optional variable existed before and its value must be copied across
					for (itr2 = 0; itr2 < P3gOptionalVarSize[itr]; itr2++)
						remap_table[newoptional_offset[itr] + itr2] = ioClass->optionalvar_offset[itr] + itr2;
				}
			}
		}

		for (itr = 0; itr < ioClass->definition->num_variables; itr++) {
			if (ioClass->definition->variable[itr].offset != P3cNoVar) {
				// this variable existed before and its value must be copied across
				for (itr2 = 0; itr2 < P3iGetDataSize(ioClass->definition->variable[itr].type); itr2++)
					remap_table[newvar_offset[itr] + itr2] = ioClass->definition->variable[itr].offset + itr2;
			}
		}

		// check to see if we can still fit in the same size class
		for (itr = 0; itr < P3cMemory_ParticleSizeClasses; itr++) {
			if (new_particle_size <= P3gParticleSizeClass[itr])
				break;
		}
		if (itr >= P3cMemory_ParticleSizeClasses) {
			UUrPrintWarning("Particle class '%s' is too large (%d) for largest size class (%d)!",
							ioClass->classname, new_particle_size, P3gParticleSizeClass[P3cMemory_ParticleSizeClasses - 1]);

			new_size_class = ioClass->size_class;
		} else {
			new_size_class = itr;
		}

#if defined(DEBUGGING) && DEBUGGING
		// perform sanity checks on remap table
		for (itr = 0; itr < new_particle_size - sizeof(P3tParticleHeader); itr++) {
			UUmAssert((remap_table[itr] == P3cNoVar) ||
					  ((remap_table[itr] >= 0) && 
					   (remap_table[itr] < ioClass->particle_size - sizeof(P3tParticleHeader))));
		}
#endif

		if (new_size_class <= ioClass->size_class) {
			P3iRemapVariables(ioClass, new_particle_size - sizeof(P3tParticleHeader), remap_table);
		} else {
			// we have to reallocate all of these particles in a larger size class
			P3iReallocateParticles(ioClass, new_size_class,
						new_particle_size - sizeof(P3tParticleHeader), remap_table);
		}
	}

	// copy the new offsets into the class
	UUrMemory_MoveFast(newoptional_offset, ioClass->optionalvar_offset,
							P3cNumParticleOptionalVars * sizeof(UUtUns16));
	for (itr = 0; itr < ioClass->definition->num_variables; itr++) {
		ioClass->definition->variable[itr].offset = newvar_offset[itr];
	}

	ioClass->particle_size = new_particle_size;
	ioClass->prev_packed_emitters = ioClass->definition->num_emitters;

	// Update the offsets in every VariableReference.
	P3gUpdateOffsetClass = ioClass;
	P3rTraverseVarRef(ioClass->definition, &P3iTraverseVarUpdateOffsets, UUcFalse);
}

// traverse over all variable references in the particle definition
void P3rTraverseVarRef(P3tParticleDefinition *inDefinition,
					   P3tVarTraverseProc inProc, UUtBool inGiveLocation)
{
	UUtUns16 itr, itr2, itr3, num_vars, num_params, offset;
	P3tActionInstance *action;
	P3tValue *val;
	P3tEmitter *emitter;
	P3tEmitterSettingDescription *class_desc;
	UUtUns32 *current_setting;
	char location[256], section_name[32];

	strcpy(location, "");

	// traverse over all actions
	for (itr = 0, action = inDefinition->action;
		itr < inDefinition->num_actions; itr++, action++) {
		
		// traverse over all variables for each action
		num_vars = P3gActionInfo[action->action_type].num_variables;
		for (itr2 = 0; itr2 < num_vars; itr2++) {
			if (inGiveLocation) {
				sprintf(location, "action %d (%s) variable %d (%s)",
						itr, P3gActionInfo[action->action_type].name,
						itr2, P3gActionInfo[action->action_type].info[itr2].name);
			}
			inProc(&action->action_var[itr2], location);
		}
		
		// traverse over all parameters for each action - look for parameters that are a variable
		num_params = P3gActionInfo[action->action_type].num_parameters;
		for (itr2 = 0, val = action->action_value; itr2 < num_params; itr2++, val++) {
			if (val->type != P3cValueType_Variable)
				continue;

			if (inGiveLocation) {
				sprintf(location, "action %d (%s) parameter %d (%s)",
						itr, P3gActionInfo[action->action_type].name,
						itr2, P3gActionInfo[action->action_type].info[itr2 + num_vars].name);
			}
			inProc(&val->u.variable, location);
		}
	}

	// traverse over variable references in the appearance
	val = &inDefinition->appearance.scale;
	if (val->type == P3cValueType_Variable) {
		if (inGiveLocation) {
			sprintf(location, "appearance scale");
		}
		inProc(&val->u.variable, location);
	}

	if (inDefinition->flags & P3cParticleClassFlag_Appearance_UseSeparateYScale) {
		val = &inDefinition->appearance.y_scale;
		if (val->type == P3cValueType_Variable) {
			if (inGiveLocation) {
				sprintf(location, "appearance y scale");
			}
			inProc(&val->u.variable, location);
		}
	}

	val = &inDefinition->appearance.rotation;
	if (val->type == P3cValueType_Variable) {
		if (inGiveLocation) {
			sprintf(location, "appearance rotation");
		}
		inProc(&val->u.variable, location);
	}

	val = &inDefinition->appearance.alpha;
	if (val->type == P3cValueType_Variable) {
		if (inGiveLocation) {
			sprintf(location, "appearance alpha");
		}
		inProc(&val->u.variable, location);
	}

	val = &inDefinition->appearance.x_offset;
	if (val->type == P3cValueType_Variable) {
		if (inGiveLocation) {
			sprintf(location, "appearance x_offset");
		}
		inProc(&val->u.variable, location);
	}

	val = &inDefinition->appearance.x_shorten;
	if (val->type == P3cValueType_Variable) {
		if (inGiveLocation) {
			sprintf(location, "appearance x_shorten");
		}
		inProc(&val->u.variable, location);
	}

	val = &inDefinition->appearance.tint;
	if (val->type == P3cValueType_Variable) {
		if (inGiveLocation) {
			sprintf(location, "appearance tint");
		}
		inProc(&val->u.variable, location);
	}

	val = &inDefinition->appearance.edgefade_min;
	if (val->type == P3cValueType_Variable) {
		if (inGiveLocation) {
			sprintf(location, "appearance edgefade_min");
		}
		inProc(&val->u.variable, location);
	}

	val = &inDefinition->appearance.edgefade_max;
	if (val->type == P3cValueType_Variable) {
		if (inGiveLocation) {
			sprintf(location, "appearance edgefade_max");
		}
		inProc(&val->u.variable, location);
	}

	val = &inDefinition->appearance.max_contrail;
	if (val->type == P3cValueType_Variable) {
		if (inGiveLocation) {
			sprintf(location, "appearance max_contrail");
		}
		inProc(&val->u.variable, location);
	}

	val = &inDefinition->appearance.lensflare_dist;
	if (val->type == P3cValueType_Variable) {
		if (inGiveLocation) {
			sprintf(location, "appearance lensflare_dist");
		}
		inProc(&val->u.variable, location);
	}

	val = &inDefinition->appearance.decal_wrap;
	if (val->type == P3cValueType_Variable) {
		if (inGiveLocation) {
			sprintf(location, "appearance decal_wrap");
		}
		inProc(&val->u.variable, location);
	}

	// traverse over variable references in the emitter(s)
	for (itr = 0, emitter = inDefinition->emitter; itr < inDefinition->num_emitters; itr++, emitter++) {
		for (itr2 = 0; itr2 < 4; itr2++) {
			P3rEmitterSubsection(itr2, emitter, &current_setting, &class_desc, &offset, section_name);

			for (itr3 = 0, val = &emitter->emitter_value[offset];
					itr3 < class_desc[*current_setting].num_params; itr3++, val++) {
				if (val->type == P3cValueType_Variable) {
					if (inGiveLocation) {
						sprintf(location, "emitter %d %s(%s) param %d(%s)",
								itr, section_name, class_desc[*current_setting].setting_name,
								itr3, class_desc[*current_setting].param_desc[itr3].param_name);
					}
					inProc(&val->u.variable, location);
				}
			}
		}
	}
}

// get the template for an action
UUtError P3rGetActionInfo(P3tActionInstance *inAction, P3tActionTemplate **outTemplate)
{
	UUmAssertReadPtr(inAction, sizeof(P3tActionInstance));

	if ((inAction->action_type < 0) || (inAction->action_type >= P3cNumActionTypes))
		return UUcError_Generic;

	*outTemplate = &P3gActionInfo[inAction->action_type];
	return UUcError_None;
}

// swap the actions in the particle class
void P3rSwapTwoActions(P3tParticleClass *inClass, UUtUns16 inIndex1, UUtUns16 inIndex2)
{
	P3tActionInstance swapaction;

	UUmAssertReadPtr(inClass, sizeof(P3tParticleDefinition));
	UUmAssert((inIndex1 >= 0) && (inIndex1 < inClass->definition->num_actions));
	UUmAssert((inIndex2 >= 0) && (inIndex2 < inClass->definition->num_actions));

	swapaction = inClass->definition->action[inIndex1];
	inClass->definition->action[inIndex1] = inClass->definition->action[inIndex2];
	inClass->definition->action[inIndex2] = swapaction;
}

// get the enum info for an enumerated constant
void P3rGetEnumInfo(P3tParticleClass *inClass, P3tEnumType inType, P3tEnumTypeInfo *outInfo)
{
	inType &= ~P3cDataType_Enum;

	UUmAssert((inType >= 0) && (inType < P3cEnumType_Max));
	*outInfo = P3gEnumTypeInfo[inType >> P3cEnumShift];

	switch(inType) {
	case P3cEnumType_Emitter:
		if (inClass->definition->num_emitters == 0) {
			outInfo->min = outInfo->max = outInfo->defaultval = (UUtUns32) -1;
		} else {
			outInfo->min = outInfo->defaultval = 0;
			outInfo->max = inClass->definition->num_emitters - 1;
		}
		break;

	case P3cEnumType_ActionIndex:
		outInfo->min = outInfo->defaultval = 0;
		outInfo->max = inClass->definition->eventlist[P3cEvent_Update].end_index - 1;
		break;
	}
}

static const char *damage_type_to_string(P3tEnumDamageType in_damage_type)
{
	typedef struct damage_type_to_string_table
	{
		P3tEnumDamageType damage_type;
		const char *string;
	} damage_type_to_string_table;

	damage_type_to_string_table table[] =
	{
		{ P3cEnumDamageType_Normal, "normal" },
		{ P3cEnumDamageType_MinorStun, "minor stun" },
		{ P3cEnumDamageType_MajorStun, "major stun" },
		{ P3cEnumDamageType_MinorKnockdown, "minor knockdown" },
		{ P3cEnumDamageType_MajorKnockdown, "major knockdown" },
		{ P3cEnumDamageType_Blownup, "blownup" },
		{ P3cEnumDamageType_PickUp, "pickup" },
		{ 0, NULL }
	};

	const char *result = NULL;

	UUtUns32 itr;

	for(itr = 0; table[itr].string != NULL; itr++)
	{
		if (table[itr].damage_type == in_damage_type) {
			result = table[itr].string;
			break;
		}
	}

	return result;
}

// describe an enumerated constant
UUtBool P3rDescribeEnum(P3tParticleClass *inClass, P3tEnumType inType, UUtUns32 inVal, char *outDescription)
{
	P3tActionInstance *action;
	UUtUns32 start_index;

	inType &= ~P3cDataType_Enum;

	switch(inType) {
	case P3cEnumType_Pingpong:
		switch(inVal) {
		case P3cEnumPingpong_Up:
			strcpy(outDescription, "up");
			return UUcTrue;

		case P3cEnumPingpong_Down:
			strcpy(outDescription, "down");
			return UUcTrue;

		default:
			return UUcFalse;
		}

	case P3cEnumType_ActionIndex:
		start_index = inClass->definition->eventlist[P3cEvent_Update].start_index;
		inVal -= start_index;
		if ((inVal < 0) || (inVal >= inClass->definition->eventlist[P3cEvent_Update].end_index - start_index)) {
			sprintf(outDescription, "%d: no such action", inVal);
			return UUcTrue;
		}

		action = &inClass->definition->action[inVal];
		if ((action->action_type < 0) || (action->action_type >= P3cNumActionTypes)) {
			sprintf(outDescription, "%d: unknown action type %d", inVal, action->action_type);
			return UUcTrue;
		}

		sprintf(outDescription, "%d: %s", inVal, P3gActionInfo[action->action_type].name);
		return UUcTrue;		

	case P3cEnumType_Emitter:
		UUmAssert(inClass != NULL);

		if (inVal == (UUtUns32) -1) {
			strcpy(outDescription, "<none>");
		} else if ((inVal < 0) || (inVal >= inClass->definition->num_emitters)) {
			sprintf(outDescription, "emitter %d (not present)", inVal);
		} else {
			sprintf(outDescription, "emitter %d (%s)", inVal, inClass->definition->emitter[inVal].classname);
		}
		return UUcTrue;		

	case P3cEnumType_Falloff:
		switch(inVal) {
		case P3cEnumFalloff_None:
			strcpy(outDescription, "none");
			return UUcTrue;

		case P3cEnumFalloff_Linear:
			strcpy(outDescription, "linear");
			return UUcTrue;

		case P3cEnumFalloff_InvSquare:
			strcpy(outDescription, "inverse-square");
			return UUcTrue;

		default:
			return UUcFalse;
		}
	case P3cEnumType_CoordFrame:
		switch(inVal) {
		case P3cEnumCoordFrame_Local:
			strcpy(outDescription, "local");
			return UUcTrue;

		case P3cEnumCoordFrame_World:
			strcpy(outDescription, "world");
			return UUcTrue;

		default:
			return UUcFalse;
		}

	case P3cEnumType_CollisionOrientation:
		switch(inVal) {
		case P3cEnumCollisionOrientation_Unchanged:
			strcpy(outDescription, "unchanged");
			return UUcTrue;

		case P3cEnumCollisionOrientation_Reversed:
			strcpy(outDescription, "reversed");
			return UUcTrue;

		case P3cEnumCollisionOrientation_Reflected:
			strcpy(outDescription, "reflected");
			return UUcTrue;

		case P3cEnumCollisionOrientation_Normal:
			strcpy(outDescription, "normal");
			return UUcTrue;

		default:
			return UUcFalse;
		}

	case P3cEnumType_Bool:
		switch(inVal) {
		case P3cEnumBool_True:
			strcpy(outDescription, "yes");
			return UUcTrue;

		case P3cEnumBool_False:
			strcpy(outDescription, "no");
			return UUcTrue;

		default:
			return UUcFalse;
		}
	case P3cEnumType_ImpactModifier:
		if ((inVal >= 0) && (inVal < ONcIEModType_NumTypes)) {
			strcpy(outDescription, ONrIEModType_GetName(inVal));
			return UUcTrue;
		} else {
			return UUcFalse;
		}

	case P3cEnumType_ImpulseSoundID:
	case P3cEnumType_AmbientSoundID:
		// sounds can't be described by the particle system
		// they must be handled by Oni windowing code
		return UUcFalse;

	case P3cEnumType_DamageType:
		{
			const char *damage_type_string = damage_type_to_string(inVal);

			if (damage_type_string != NULL) {
				strcpy(outDescription, damage_type_to_string(inVal));
				return UUcTrue;
			}
			else {
				return UUcFalse;
			}
		}

	case P3cEnumType_Axis:
		switch(inVal) {
		case P3cEnumAxis_PosX:
			strcpy(outDescription, "+X");
			return UUcTrue;

		case P3cEnumAxis_NegX:
			strcpy(outDescription, "-X");
			return UUcTrue;

		case P3cEnumAxis_PosY:
			strcpy(outDescription, "+Y");
			return UUcTrue;

		case P3cEnumAxis_NegY:
			strcpy(outDescription, "-Y");
			return UUcTrue;

		case P3cEnumAxis_PosZ:
			strcpy(outDescription, "+Z");
			return UUcTrue;

		case P3cEnumAxis_NegZ:
			strcpy(outDescription, "+Z");
			return UUcTrue;

		default:
			return UUcFalse;
		}

	default:
		return UUcFalse;		
	}
}

/*
 * particle display
 */

// notify particlesystem of sky visibility
void P3rNotifySkyVisible(UUtBool inVisible)
{
	P3gSkyVisible = inVisible;
}

// display a class of particles
void P3iDisplayParticleClass(P3tParticleClass *inClass, UUtUns32 inCurrentTick, UUtUns32 inDrawTicks,
							 UUtBool inLensFlares, UUtUns32 *ioNumContrailEmitters)
{
	M3tGeomState_SpriteMode sprite_mode;
	UUtBool sky_class = UUcFalse;;

	UUmAssert(inClass->num_particles > 0);
	UUmAssert(inClass->non_persistent_flags & P3cParticleClass_NonPersistentFlag_Active);

	// cull out classes that do not need to be drawn
	if ((inClass->definition->flags2 & P3cParticleClassFlag2_Appearance_Invisible) ||
		(inClass->definition->flags2 & P3cParticleClassFlag2_Appearance_Decal))
		return;

	if (inClass->definition->flags2 & P3cParticleClassFlag2_Appearance_Sky) {
		sky_class = UUcTrue;
		if (!P3gSkyVisible) {
			// don't draw this particle class
			return;
		}
	}
		
	if (!inLensFlares && (inClass->definition->flags2 & P3cParticleClassFlag2_Appearance_LensFlare)) {
		// this is a lens-flare class and must be drawn in a separate pass
		if (P3gNumLensFlares >= P3cMaxLensFlares) {
			COrConsole_Printf("### exceeded maximum number of lensflare classes (%d)",
							P3cMaxLensFlares);
		} else {
			P3gLensFlares[P3gNumLensFlares++] = inClass;
		}
		return;
	}

	if (sky_class) {
		// disable fog for sky particles
		M3rDraw_State_Push();
		M3rDraw_State_SetInt(M3cDrawStateIntType_Fog, M3cDrawStateFogDisable);
		M3rDraw_State_Commit();
	}

	if (inClass->definition->flags & P3cParticleClassFlag_Appearance_Geometry) {
		P3rDisplayClass_Geometry(inClass, inCurrentTick, inDrawTicks);

	} else if (inClass->definition->flags2 & P3cParticleClassFlag2_Appearance_Vector) {
		P3rDisplayClass_Vector(inClass, inCurrentTick, inDrawTicks);

	} else {
		sprite_mode = (M3tGeomState_SpriteMode)
			((inClass->definition->flags & P3cParticleClassFlag_Appearance_SpriteTypeMask)
			>> P3cParticleClassFlags_SpriteTypeShift);
		
		if ((sprite_mode == M3cGeomState_SpriteMode_Contrail) ||
			(sprite_mode == M3cGeomState_SpriteMode_ContrailRotated)) {
			if (inClass->definition->flags2 & P3cParticleClassFlag2_ContrailEmitter) {
				// contrail emitters must be displayed after all contrails - so that their contrails
				// have already determined where they are
				if (*ioNumContrailEmitters >= P3cMaxContrailEmitters) {
					COrConsole_Printf("### exceeded maximum number of contrail emitting classes (%d)",
									P3cMaxContrailEmitters);
				} else {
					P3gContrailEmitters[*ioNumContrailEmitters] = inClass;
					*ioNumContrailEmitters += 1;
				}
			} else {
				P3rDisplayClass_Contrail(inClass, inCurrentTick, inDrawTicks, sprite_mode, UUcFalse);
			}
			
		} else {
			P3rDisplayClass_Sprite(inClass, inCurrentTick, inDrawTicks, sprite_mode);
		}
	}

	if (sky_class) {
		// restore previous draw state
		M3rDraw_State_Pop();
	}
}

// display all particles
void P3rDisplay( UUtBool inLensFlares )
{
	UUtUns16			index;
	UUtUns32			itr, draw_ticks, current_tick, num_contrailemitters;
	P3tParticleClass *	current_class;
	M3tGeomState_SpriteMode sprite_mode;
        
#if PARTICLE_PROFILE_DISPLAY
	UUrProfile_State_Set(UUcProfile_State_On);
#endif

	current_tick = ONgGameState->gameTime; // S.S. unreliable-> M3rDraw_State_GetInt(M3cDrawStateIntType_Time);
	draw_ticks = current_tick - P3gLastDrawTick;

	if (inLensFlares && (P3gNumLensFlares == 0))
		goto done;

	// particles are drawn using alpha triangles, which are sorted by motoko.
	// note that particles never write anything into the Z buffer. lensflares
	// turn off the depth test and instead sample depth directly at their center point.
	M3rGeom_State_Push();
	M3rGeom_State_Set(M3cGeomStateIntType_SubmitMode, M3cGeomState_SubmitMode_SortAlphaTris);
	M3rDraw_State_SetInt(M3cDrawStateIntType_ZWrite, M3cDrawState_ZWrite_Off);
	if (inLensFlares) {
		M3rDraw_State_SetInt(M3cDrawStateIntType_ZCompare, M3cDrawState_ZCompare_Off);		
	}
	M3rGeom_State_Commit();

	num_contrailemitters = 0;
	if (inLensFlares) {
		// draw the separate lensflare pass, which comes at the end of most other game display.
		// the lensflares particle classes found here 
		for (itr = 0; itr < P3gNumLensFlares; itr++) {
			P3iDisplayParticleClass(P3gLensFlares[itr], current_tick, draw_ticks, UUcTrue, &num_contrailemitters);
		}

		// this is the last pass, set up the last draw tick counter
		P3gLastDrawTick	= current_tick;

	} else {
		// draw the first particle pass
		P3gNumLensFlares = 0;

		P3iActiveList_Lock();

		index = P3gFirstActiveClass;
		while (index != P3cClass_None) {
			current_class = P3gClassTable + index;
			index = current_class->next_active_class;

			P3iDisplayParticleClass(current_class, current_tick, draw_ticks, UUcFalse, &num_contrailemitters);
		}

		P3iActiveList_Unlock();

		// collision debugging display is also drawn during the first pass
		if (P3gDebugCollisionEnable) {
			for (itr = 0; itr < P3gDebugCollision_UsedPoints; itr += 2) {
				M3rGeom_Line_Light(&P3gDebugCollisionPoints[itr], &P3gDebugCollisionPoints[itr + 1], IMcShade_Orange);
			}
		}
	}

	// draw contrail emitters found during this pass
	for (itr = 0; itr < num_contrailemitters; itr++) {
		current_class = P3gContrailEmitters[itr];

		sprite_mode = (M3tGeomState_SpriteMode)
			((current_class->definition->flags & P3cParticleClassFlag_Appearance_SpriteTypeMask)
			>> P3cParticleClassFlags_SpriteTypeShift);
			
		P3rDisplayClass_Contrail(current_class, current_tick, draw_ticks, sprite_mode, UUcTrue);
	}

	M3rGeom_State_Pop();

done:
#if PARTICLE_PROFILE_DISPLAY
	UUrProfile_State_Set(UUcProfile_State_Off);
#endif

	return;
}

enum P3tGeometryMatrixEnum {
	P3cMatrix_Orientation,
	P3cMatrix_Velocity,
	P3cMatrix_Identity
};

static UUtUns16 P3rModifyAlphaByFogFromDepth(
	UUtUns16 alpha,
	M3tPoint3D *point)
{
	extern float gl_calculate_fog_factor(M3tPoint3D *point); // from gl_utility.c
	float fog_factor= gl_calculate_fog_factor(point);

	if (fog_factor > 0.f)
	{
		// decrease alpha by as much as an object at this range would have been fogged
		alpha= (UUtUns16)((float)alpha * fog_factor);
		alpha= UUmMin(alpha, 0xFF);
	}
	
	return alpha;
}

// display a class of geometry particles
void P3rDisplayClass_Geometry(P3tParticleClass *inClass, UUtUns32 inTime, UUtUns32 inDeltaTime)
{
	UUtUns16	constalpha, realalpha, currentalpha;
	UUtUns16	vel_offset, orient_offset, offsetpos_offset, dynmatrix_offset, lensframes_offset;
	P3tParticle *particle, *next_particle;
	M3tGeometry *geometry;
	P3tAppearance *appearance_params;
	float scale, floatalpha, lensflare_tolerance;
	UUtBool do_scale, dynamic_scale, dynamic_alpha;
	UUtBool is_dead, is_lensflare, lensflare_visible;
	UUtError error;
	M3tPoint3D *positionptr, attached_position, *lenstestptr;
	M3tMatrix4x3 *matrixptr, geom_matrix;
	enum P3tGeometryMatrixEnum matrixtype;

	currentalpha = UUcMaxUns16;		// nonsense value to ensure we set alpha on first pass through loop

	is_lensflare = ((inClass->definition->flags2 & P3cParticleClassFlag2_Appearance_LensFlare) != 0);
	lensflare_tolerance = 0.0f;
	if (is_lensflare) {
		P3iCalculateValue(NULL, &inClass->definition->appearance.lensflare_dist, (void *) &lensflare_tolerance);

		if (inClass->definition->flags & P3cParticleClassFlag_HasLensFrames) {
			lensframes_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_LensFrames];
			UUmAssert((lensframes_offset >= 0) && 
				(lensframes_offset < (UUtUns16) (inClass->particle_size - sizeof(P3tParticleHeader))));
		} else {
			lensframes_offset = P3cNoVar;
		}
	}

	// determine how we will be finding the matrix for each particle
	if (inClass->definition->flags & P3cParticleClassFlag_HasOrientation) {
		// use matrix by preference, because it's already normalised
		matrixtype = P3cMatrix_Orientation;

		orient_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_Orientation];
		UUmAssert((orient_offset >= 0) && 
			(orient_offset < (UUtUns16) (inClass->particle_size - sizeof(P3tParticleHeader))));

	} else if (inClass->definition->flags & P3cParticleClassFlag_HasVelocity) {
		// use velocity vector otherwise
		matrixtype = P3cMatrix_Velocity;

		vel_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_Velocity];
		UUmAssert((vel_offset >= 0) && 
  					(vel_offset < (UUtUns16) (inClass->particle_size - sizeof(P3tParticleHeader))));
	} else {
		// no orientation; use identity
		matrixtype = P3cMatrix_Identity;
					
	}

	// is there an offset position?
	if (inClass->definition->flags & P3cParticleClassFlag_HasOffsetPos) {
		offsetpos_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_OffsetPos];
		UUmAssert((offsetpos_offset >= 0) && 
			(offsetpos_offset < (UUtUns16) (inClass->particle_size - sizeof(P3tParticleHeader))));
	} else {
		offsetpos_offset = P3cNoVar;
	}
	
	// is there a dynamic matrix?
	if (inClass->definition->flags & P3cParticleClassFlag_HasDynamicMatrix) {
		dynmatrix_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_DynamicMatrix];
		UUmAssert((dynmatrix_offset >= 0) && 
			(dynmatrix_offset < (UUtUns16) (inClass->particle_size - sizeof(P3tParticleHeader))));
	} else {
		dynmatrix_offset = P3cNoVar;
	}
			
	appearance_params = (P3tAppearance *) &inClass->definition->appearance;
			
	dynamic_scale = P3iValueIsDynamic(&appearance_params->scale);
	if (dynamic_scale) {
		do_scale = UUcTrue;
	} else {
		P3iCalculateValue(NULL, &appearance_params->scale, (void *) &scale);
		if (fabs(scale - 1.0f) < 0.01f) {
			do_scale = UUcFalse;
		} else {
			do_scale = UUcTrue;
		}
	}
	
	dynamic_alpha = P3iValueIsDynamic(&appearance_params->alpha);
	if (!dynamic_alpha) {
		P3iCalculateValue(NULL, &appearance_params->alpha, (void *) &floatalpha);
		constalpha = (UUtUns16) (floatalpha * M3cMaxAlpha);
		M3rGeom_State_Set(M3cGeomStateIntType_Alpha, constalpha);
		M3rGeom_State_Commit();

		currentalpha = constalpha;
	}
	
	error = TMrInstance_GetDataPtr(M3cTemplate_Geometry, appearance_params->instance, &geometry);
	if (error != UUcError_None) {
		if ((inClass->non_persistent_flags & P3cParticleClass_NonPersistentFlag_WarnedAboutNotFound) == 0) {
			COrConsole_Printf("GEOMETRY NOT FOUND: particle '%s' geometry '%s'", inClass->classname, appearance_params->instance);
			inClass->non_persistent_flags |= P3cParticleClass_NonPersistentFlag_WarnedAboutNotFound;
		}

		// default geometry if none can be found
		error = TMrInstance_GetDataPtr(M3cTemplate_Geometry, "notfound", &geometry);
		if (error != UUcError_None) {
			// give up - no geometry found
			return;
		}
	}
	
	
	for (particle = inClass->headptr; particle != NULL; particle = next_particle) {
		next_particle = particle->header.nextptr;
		
		if (particle->header.flags & P3cParticleFlag_Hidden)
			continue;
		
		// calculate the translational component of the position matrix
		// (offset position is taken care of below, when the matrix is actually computed)
		positionptr = P3rGetPositionPtr(inClass, particle, &is_dead);
		if (is_dead) {
			continue;
		}
		
		// if we are fading out then use the current lifetime and fade time to calculate alpha
		if (particle->header.flags & P3cParticleFlag_FadeOut) {
			realalpha = (UUtUns16)  (M3cMaxAlpha * particle->header.lifetime / particle->header.fadeout_time);
			M3rGeom_State_Set(M3cGeomStateIntType_Alpha, realalpha);
			M3rGeom_State_Commit();

			currentalpha = realalpha;
			
		} else {
			if (dynamic_alpha) {
				P3iCalculateValue(particle, &appearance_params->alpha, (void *) &floatalpha);
				realalpha = (UUtUns16) (floatalpha * M3cMaxAlpha);
			} else {
				realalpha = constalpha;
			}
		}

		if (is_lensflare) {
			if (dynmatrix_offset != P3cNoVar) {
				// apply the dynamic matrix before testing lensflare culling
				matrixptr = *((M3tMatrix4x3 **) &particle->vararray[dynmatrix_offset]);
				MUrMatrix_MultiplyPoint(positionptr, matrixptr, &attached_position);
				lenstestptr = &attached_position;
			} else {
				lenstestptr = positionptr;
			}

			lensflare_visible = P3rPointVisible(lenstestptr, lensflare_tolerance);

			if (lensframes_offset == P3cNoVar) {
				// lensflares don't fade in and out
				if (!lensflare_visible)
					continue;
			} else {
				// calculate lensflare fading
				realalpha = P3rLensFlare_Fade(appearance_params, lensflare_visible, inDeltaTime,
													realalpha, (UUtUns32 *) &particle->vararray[lensframes_offset]);
				if (realalpha == 0)
				{
					continue;
				}
				else if ((realalpha= P3rModifyAlphaByFogFromDepth(realalpha, lenstestptr)) == 0)
				{
					continue;
				}
			}
		}

		if (realalpha != currentalpha) {
			M3rGeom_State_Set(M3cGeomStateIntType_Alpha, realalpha);
			M3rGeom_State_Commit();
		}

		// evaluate any dynamic appearance parameters for this particle
		if (dynamic_scale)
			P3iCalculateValue(particle, &appearance_params->scale, (void *) &scale);
		
		switch(matrixtype) {
		case P3cMatrix_Orientation:
			UUrMemory_MoveFast(&particle->vararray[orient_offset], (M3tMatrix3x3 *) &geom_matrix,
								sizeof(M3tMatrix3x3));
			break;
			
		case P3cMatrix_Velocity:
			// build an orientation matrix along velocity and upvector -> world Y
			P3rConstructOrientation(P3cEmitterOrientation_Velocity, P3cEmitterOrientation_WorldPosY,
									NULL, NULL, (M3tVector3D *) &particle->vararray[vel_offset],
									(M3tMatrix3x3 *) &geom_matrix);
			break;
			
		case P3cMatrix_Identity:
			MUrMatrix3x3_Identity((M3tMatrix3x3 *) &geom_matrix);
			break;
		}
		
		geom_matrix.m[3][0] = positionptr->x;
		geom_matrix.m[3][1] = positionptr->y;
		geom_matrix.m[3][2] = positionptr->z;
		if (offsetpos_offset != P3cNoVar) 
		{
			positionptr = (M3tVector3D *) &particle->vararray[offsetpos_offset];
			geom_matrix.m[3][0] += positionptr->x;
			geom_matrix.m[3][1] += positionptr->y;
			geom_matrix.m[3][2] += positionptr->z;
		}
		
		M3rMatrixStack_Push();
		
		// apply the dynamic matrix if there is one
		if (dynmatrix_offset != P3cNoVar) {
			matrixptr = *((M3tMatrix4x3 **) &particle->vararray[dynmatrix_offset]);
			if (matrixptr) {
				M3rMatrixStack_Multiply(matrixptr);
			}
		}
		
		M3rMatrixStack_Multiply(&geom_matrix);
		
		if (do_scale) {
			M3rMatrixStack_UniformScale(scale);
		}
		
		M3rGeometry_Draw(geometry);
		M3rMatrixStack_Pop();
	}
}

// display a class of vector particles
void P3rDisplayClass_Vector(P3tParticleClass *inClass, UUtUns32 inTime, UUtUns32 inDeltaTime)
{
	UUtUns16	currentalpha, constalpha, realalpha;
	UUtUns16	vel_offset, offsetpos_offset, dynmatrix_offset, lensframes_offset;
	P3tParticle *particle, *next_particle;
	P3tAppearance *appearance_params;
	float scale, floatalpha, lensflare_tolerance;
	IMtShade tint;
	UUtBool do_scale, dynamic_scale, dynamic_alpha, is_lensflare, lensflare_visible;
	UUtBool dynamic_tint, is_dead;
	M3tPoint3D calculatedposition, *positionptr, endpoint;
	M3tMatrix4x3 *matrixptr;

	currentalpha = UUcMaxUns16;		// nonsense value to ensure we set alpha on first pass through loop

	is_lensflare = ((inClass->definition->flags2 & P3cParticleClassFlag2_Appearance_LensFlare) != 0);
	lensflare_tolerance = 0.0f;
	if (is_lensflare) {
		P3iCalculateValue(NULL, &inClass->definition->appearance.lensflare_dist, (void *) &lensflare_tolerance);

		if (inClass->definition->flags & P3cParticleClassFlag_HasLensFrames) {
			lensframes_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_LensFrames];
			UUmAssert((lensframes_offset >= 0) && 
				(lensframes_offset < (UUtUns16) (inClass->particle_size - sizeof(P3tParticleHeader))));
		} else {
			lensframes_offset = P3cNoVar;
		}
	}

	// get the velocity
	if (inClass->definition->flags & P3cParticleClassFlag_HasVelocity) {
		vel_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_Velocity];
		UUmAssert((vel_offset >= 0) && 
			(vel_offset < (UUtUns16) (inClass->particle_size - sizeof(P3tParticleHeader))));
	} else {
		// vector particles cannot be drawn without a velocity!
		COrConsole_Printf("### '%s' vector particles require a velocity!",
							inClass->classname);
		return;
	}
	
	// is there an offset position?
	if (inClass->definition->flags & P3cParticleClassFlag_HasOffsetPos) {
		offsetpos_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_OffsetPos];
		UUmAssert((offsetpos_offset >= 0) && 
			(offsetpos_offset < (UUtUns16) (inClass->particle_size - sizeof(P3tParticleHeader))));
		
		// set up for inside loop
		positionptr = &calculatedposition;
	} else {
		offsetpos_offset = P3cNoVar;
	}
	
	// is there a dynamic matrix?
	if (inClass->definition->flags & P3cParticleClassFlag_HasDynamicMatrix) {
		dynmatrix_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_DynamicMatrix];
		UUmAssert((dynmatrix_offset >= 0) && 
			(dynmatrix_offset < (UUtUns16) (inClass->particle_size - sizeof(P3tParticleHeader))));
	} else {
		dynmatrix_offset = P3cNoVar;
	}
	
	appearance_params = (P3tAppearance *) &inClass->definition->appearance;
	
	dynamic_scale = P3iValueIsDynamic(&appearance_params->scale);
	if (dynamic_scale) {
		do_scale = UUcTrue;
	} else {
		P3iCalculateValue(NULL, &appearance_params->scale, (void *) &scale);
	}
	
	dynamic_tint = P3iValueIsDynamic(&appearance_params->tint);
	if (!dynamic_tint)
		P3iCalculateValue(NULL, &appearance_params->tint, (void *) &tint);
	
	dynamic_alpha = P3iValueIsDynamic(&appearance_params->alpha);
	if (!dynamic_alpha) {
		P3iCalculateValue(NULL, &appearance_params->alpha, (void *) &floatalpha);
		constalpha = (UUtUns16) (floatalpha * M3cMaxAlpha);
		M3rGeom_State_Set(M3cGeomStateIntType_Alpha, constalpha);
		M3rGeom_State_Commit();

		currentalpha = constalpha;
	}
	
	for (particle = inClass->headptr; particle != NULL; particle = next_particle) {
		next_particle = particle->header.nextptr;
		
		if (particle->header.flags & P3cParticleFlag_Hidden)
			continue;
		
		positionptr = P3rGetPositionPtr(inClass, particle, &is_dead);
		if (is_dead) {
			continue;
		}
		
		if (offsetpos_offset != P3cNoVar) {
			MUmVector_Add(calculatedposition, *positionptr,
				*((M3tVector3D *) &particle->vararray[offsetpos_offset]));
			positionptr = &calculatedposition;
		}
		
		if (dynamic_scale)
			P3iCalculateValue(particle, &appearance_params->scale, (void *) &scale);
		
		MUmVector_Copy(endpoint, *positionptr);
		MUmVector_ScaleIncrement(endpoint, scale, *((M3tVector3D *) &particle->vararray[vel_offset]));
		
		if (dynmatrix_offset != P3cNoVar) {
			matrixptr = (M3tMatrix4x3 *) &particle->vararray[dynmatrix_offset];
			MUrMatrix_MultiplyPoint(positionptr, matrixptr, &calculatedposition);
			positionptr = &calculatedposition;
			MUrMatrix_MultiplyPoint(&endpoint, matrixptr, &endpoint);
		}
		
		// if we are fading out then use the current lifetime and fade time to calculate alpha
		if (particle->header.flags & P3cParticleFlag_FadeOut) {
			realalpha = (UUtUns16)  (M3cMaxAlpha * particle->header.lifetime / particle->header.fadeout_time);
			M3rGeom_State_Set(M3cGeomStateIntType_Alpha, realalpha);
			M3rGeom_State_Commit();

			currentalpha = realalpha;
			
		} else {
			if (dynamic_alpha) {
				P3iCalculateValue(particle, &appearance_params->alpha, (void *) &floatalpha);
				realalpha = (UUtUns16) (floatalpha * M3cMaxAlpha);
			} else {
				realalpha = constalpha;
			}
		}

		if (is_lensflare) {
			lensflare_visible = P3rPointVisible(positionptr, lensflare_tolerance);

			if (lensframes_offset == P3cNoVar) {
				// lensflares don't fade in and out
				if (!lensflare_visible)
					continue;
			} else {
				// calculate lensflare fading
				realalpha = P3rLensFlare_Fade(appearance_params, lensflare_visible, inDeltaTime,
													realalpha, (UUtUns32 *) &particle->vararray[lensframes_offset]);
				if (realalpha == 0)
				{
					continue;
				}
				else if ((realalpha= P3rModifyAlphaByFogFromDepth(realalpha, positionptr)) == 0)
				{
					continue;
				}
			}
		}

		if (realalpha != currentalpha) {
			M3rGeom_State_Set(M3cGeomStateIntType_Alpha, realalpha);
			M3rGeom_State_Commit();
		}

		// evaluate any dynamic appearance parameters for this particle
		if (dynamic_tint)
			P3iCalculateValue(particle, &appearance_params->tint, (void *) &tint);
		
		M3rGeom_Line_Light(positionptr, &endpoint, tint);
	}
}

// display a class of sprite particles
void P3rDisplayClass_Sprite(P3tParticleClass *inClass, UUtUns32 inTime, UUtUns32 inDeltaTime,
							M3tGeomState_SpriteMode inMode)
{
	UUtUns16	constalpha, realalpha;
	UUtUns16	vel_offset, orient_offset, offsetpos_offset, dynmatrix_offset, texindex_offset, textime_offset, lensframes_offset;
	UUtUns32	texval;
	P3tParticle *particle, *next_particle;
	M3tTextureMap *texturemap;
	P3tAppearance *appearance_params;
	float scale, currentscale, floatalpha, y_scale, rotation, x_offset, x_shorten, chopval;
	float edgefade_min, edgefade_max, edgefade_val, lensflare_tolerance;
	IMtShade tint;
	UUtBool dynamic_scale, dynamic_alpha, dynamic_yscale, dynamic_rotation, use_yscale, scale_to_velocity;
	UUtBool dynamic_xoffset, dynamic_xshorten, dynamic_tint, is_dead, do_edgefade, is_lensflare;
	UUtBool twosided_edgefade, lensflare_visible;
	UUtError error;
	M3tPoint3D calculatedposition, *positionptr;
	M3tVector3D *velocityptr, new_velocity, perp_dir;
	M3tMatrix3x3 *orientationptr, new_orientation;
	M3tMatrix4x3 *matrixptr;
	M3tVector3D cameraPos, cameraDir;
	M3tGeomCamera *activeCamera;

	is_lensflare = ((inClass->definition->flags2 & P3cParticleClassFlag2_Appearance_LensFlare) != 0);
	lensflare_tolerance = 0.0f;
	if (is_lensflare) {
		P3iCalculateValue(NULL, &inClass->definition->appearance.lensflare_dist, (void *) &lensflare_tolerance);

		if (inClass->definition->flags & P3cParticleClassFlag_HasLensFrames) {
			lensframes_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_LensFrames];
			UUmAssert((lensframes_offset >= 0) && 
				(lensframes_offset < (UUtUns16) (inClass->particle_size - sizeof(P3tParticleHeader))));
		} else {
			lensframes_offset = P3cNoVar;
		}
	}

	appearance_params = (P3tAppearance *) &inClass->definition->appearance;
				
	do_edgefade = ((inClass->definition->flags2 & P3cParticleClassFlag2_Appearance_EdgeFade) != 0);
	twosided_edgefade = ((inClass->definition->flags2 & P3cParticleClassFlag2_Appearance_EdgeFade_OneSided) == 0);
	if (do_edgefade) {
		if (P3iValueIsDynamic(&appearance_params->edgefade_min) || P3iValueIsDynamic(&appearance_params->edgefade_max)) {
			COrConsole_Printf("### '%s': edgefade values must be constants, not random or variables");
			do_edgefade = UUcFalse;
		} else {
			P3iCalculateValue(NULL, &appearance_params->edgefade_min, (void *) &edgefade_min);
			P3iCalculateValue(NULL, &appearance_params->edgefade_max, (void *) &edgefade_max);
		}

		// get the camera's location
		M3rCamera_GetActive(&activeCamera);
		M3rCamera_GetViewData(activeCamera, &cameraPos, NULL, NULL);
	}

	switch(inMode) {
		case M3cGeomState_SpriteMode_Normal:
			// orientation only required if we are edgefading
			if (!do_edgefade) {
				vel_offset = orient_offset = P3cNoVar;
				break;
			} else {
				// fall through to next case - which determines orientation
			}
					
		case M3cGeomState_SpriteMode_Rotated:
		case M3cGeomState_SpriteMode_Billboard :
			// this sprite drawing mode requires an orientation, either from a 3x3 matrix or
			// a velocity vector.
			if (inClass->definition->flags & P3cParticleClassFlag_HasOrientation) {
				// use matrix by preference, because it's already normalised
				orient_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_Orientation];
				UUmAssert((orient_offset >= 0) && 
					(orient_offset < (UUtUns16) (inClass->particle_size - sizeof(P3tParticleHeader))));
				vel_offset = P3cNoVar;
				
			} else if (inClass->definition->flags & P3cParticleClassFlag_HasVelocity) {
				// use velocity vector otherwise
				vel_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_Velocity];
				UUmAssert((vel_offset >= 0) && 
					(vel_offset < (UUtUns16) (inClass->particle_size - sizeof(P3tParticleHeader))));
				orient_offset = P3cNoVar;
						
			} else {
				COrConsole_Printf("### '%s' requires velocity or orientation to use this sprite mode!",
					inClass->classname);
				return;
			}
			break;
					
		case M3cGeomState_SpriteMode_Arrow:
		case M3cGeomState_SpriteMode_Discus:
		case M3cGeomState_SpriteMode_Flat:
			// this sprite drawing mode requires an orientation matrix.
			if (inClass->definition->flags & P3cParticleClassFlag_HasOrientation) {
				orient_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_Orientation];
				UUmAssert((orient_offset >= 0) && 
					(orient_offset < (UUtUns16) (inClass->particle_size - sizeof(P3tParticleHeader))));
				vel_offset = P3cNoVar;
			} else {
				COrConsole_Printf("### '%s' requires orientation to use this sprite mode!",
					inClass->classname);
				return;
			}
			break;
					
		default:
			COrConsole_Printf("### '%s' unknown sprite mode!", inClass->classname);
			break;
	}
				
	// is there an offset position?
	if (inClass->definition->flags & P3cParticleClassFlag_HasOffsetPos) {
		offsetpos_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_OffsetPos];
		UUmAssert((offsetpos_offset >= 0) && 
			(offsetpos_offset < (UUtUns16) (inClass->particle_size - sizeof(P3tParticleHeader))));
	} else {
		offsetpos_offset = P3cNoVar;
	}
				
	// is there a dynamic matrix?
	if (inClass->definition->flags & P3cParticleClassFlag_HasDynamicMatrix) {
		dynmatrix_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_DynamicMatrix];
		UUmAssert((dynmatrix_offset >= 0) && 
			(dynmatrix_offset < (UUtUns16) (inClass->particle_size - sizeof(P3tParticleHeader))));
	} else {
		dynmatrix_offset = P3cNoVar;
	}
	
	// is there a texture index?
	if (inClass->definition->flags & P3cParticleClassFlag_HasTextureIndex) {
		texindex_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_TextureIndex];
		UUmAssert((texindex_offset >= 0) && 
			(texindex_offset < (UUtUns16) (inClass->particle_size - sizeof(P3tParticleHeader))));
	} else {
		texindex_offset = P3cNoVar;
	}			
				
	// is there a texture time?
	if (inClass->definition->flags & P3cParticleClassFlag_HasTextureTime) {
		textime_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_TextureTime];
		UUmAssert((textime_offset >= 0) && 
			(textime_offset < (UUtUns16) (inClass->particle_size - sizeof(P3tParticleHeader))));
	} else {
		textime_offset = P3cNoVar;
	}			
			
	// set up drawing parameters for this class
	M3rGeom_State_Set(M3cGeomStateIntType_SpriteMode, inMode);
	M3rGeom_State_Commit();
				
	dynamic_scale = P3iValueIsDynamic(&appearance_params->scale);
	if (!dynamic_scale)
		P3iCalculateValue(NULL, &appearance_params->scale, (void *) &scale);
				
	dynamic_xoffset = P3iValueIsDynamic(&appearance_params->x_offset);
	if (!dynamic_xoffset)
		P3iCalculateValue(NULL, &appearance_params->x_offset, (void *) &x_offset);
				
	dynamic_xshorten = P3iValueIsDynamic(&appearance_params->x_shorten);
	if (!dynamic_xshorten)
		P3iCalculateValue(NULL, &appearance_params->x_shorten, (void *) &x_shorten);
				
	if (inClass->definition->flags & P3cParticleClassFlag_Appearance_UseSeparateYScale) {
		use_yscale = UUcTrue;
		
		dynamic_yscale = P3iValueIsDynamic(&appearance_params->y_scale);
		if (!dynamic_yscale)
			P3iCalculateValue(NULL, &appearance_params->y_scale, (void *) &y_scale);
	} else {
		use_yscale = dynamic_yscale = UUcFalse;
	}
				
	dynamic_rotation = P3iValueIsDynamic(&appearance_params->rotation);
	if (!dynamic_rotation)
		P3iCalculateValue(NULL, &appearance_params->rotation, (void *) &rotation);
	
	if (inClass->definition->flags & P3cParticleClassFlag_Appearance_ScaleToVelocity) {
		scale_to_velocity = UUcTrue;
	} else {
		scale_to_velocity = UUcFalse;
	}
				
	dynamic_alpha = P3iValueIsDynamic(&appearance_params->alpha);
	if (!dynamic_alpha) {
		P3iCalculateValue(NULL, &appearance_params->alpha, (void *) &floatalpha);
		constalpha = (UUtUns16) (floatalpha * M3cMaxAlpha);
	}
				
	dynamic_tint = P3iValueIsDynamic(&appearance_params->tint);
	if (!dynamic_tint)
		P3iCalculateValue(NULL, &appearance_params->tint, (void *) &tint);
				
	error = TMrInstance_GetDataPtr(M3cTemplate_TextureMap, appearance_params->instance, &texturemap);
	if (error != UUcError_None) {
		if ((inClass->non_persistent_flags & P3cParticleClass_NonPersistentFlag_WarnedAboutNotFound) == 0) {
			COrConsole_Printf("PARTICLE TEXTURE NOT FOUND: particle '%s' texture '%s'", inClass->classname, appearance_params->instance);
			inClass->non_persistent_flags |= P3cParticleClass_NonPersistentFlag_WarnedAboutNotFound;
		}

		// default texture if none can be found
		error = TMrInstance_GetDataPtr(M3cTemplate_TextureMap, "notfoundtex", &texturemap);
		if (error != UUcError_None) {
			// give up
			return;
		}
	}
				
				
	for (particle = inClass->headptr; particle != NULL; particle = next_particle) {
		next_particle = particle->header.nextptr;
		
		if (particle->header.flags & P3cParticleFlag_Hidden)
			continue;
		
		// calculate this particle's position
		positionptr = P3rGetPositionPtr(inClass, particle, &is_dead);
		if (is_dead) {
			continue;
		}

		// find the dynamic matrix
		if (dynmatrix_offset != P3cNoVar) {
			matrixptr = *((M3tMatrix4x3 **) &particle->vararray[dynmatrix_offset]);
		} else {
			matrixptr = NULL;
		}

		if (offsetpos_offset != P3cNoVar) {
			MUmVector_Add(calculatedposition, *positionptr,
				*((M3tVector3D *) &particle->vararray[offsetpos_offset]));
			positionptr = &calculatedposition;
		}
		
		// apply the dynamic matrix to the position
		if (matrixptr) {
			MUrMatrix_MultiplyPoint(positionptr, matrixptr, &calculatedposition);
			positionptr = &calculatedposition;
		}
		
		if (dynamic_alpha) {
			P3iCalculateValue(particle, &appearance_params->alpha, (void *) &floatalpha);
			realalpha = (UUtUns16) (floatalpha * M3cMaxAlpha);
		} else {
			realalpha = constalpha;
		}

		if (is_lensflare) {
			lensflare_visible = P3rPointVisible(positionptr, lensflare_tolerance);

			if (lensframes_offset == P3cNoVar) {
				// lensflares don't fade in and out
				if (!lensflare_visible)
					continue;
			} else {
				// calculate lensflare fading
				realalpha = P3rLensFlare_Fade(appearance_params, lensflare_visible, inDeltaTime,
												realalpha, (UUtUns32 *) &particle->vararray[lensframes_offset]);
				if (realalpha == 0)
				{
					continue;
				}
				else if ((realalpha= P3rModifyAlphaByFogFromDepth(realalpha, positionptr)) == 0)
				{
					continue;
				}
			}
		}

		// evaluate any dynamic appearance parameters for this particle
		if (dynamic_scale)
			P3iCalculateValue(particle, &appearance_params->scale, (void *) &scale);
		
		if (dynamic_xoffset)
			P3iCalculateValue(particle, &appearance_params->x_offset, (void *) &x_offset);
		
		if (dynamic_xshorten)
			P3iCalculateValue(particle, &appearance_params->x_shorten, (void *) &x_shorten);
		
		if (dynamic_rotation)
			P3iCalculateValue(particle, &appearance_params->rotation, (void *) &rotation);
		
		if (dynamic_yscale)
			P3iCalculateValue(particle, &appearance_params->y_scale, (void *) &y_scale);
		
		if (dynamic_tint)
			P3iCalculateValue(particle, &appearance_params->tint, (void *) &tint);
		
		// calculate the velocity and orientation
		if (vel_offset == P3cNoVar) {
			velocityptr = NULL;
		} else {
			velocityptr = (M3tVector3D *) &particle->vararray[vel_offset];
		}
		if (orient_offset == P3cNoVar) {
			orientationptr = NULL;
		} else {
			orientationptr = (M3tMatrix3x3 *) &particle->vararray[orient_offset];
		}
		
		currentscale = scale;
		if (scale_to_velocity && (velocityptr != NULL)) {
			currentscale *= 1.0f + MUmVector_GetLength(*velocityptr);
		}
		
		// apply the dynamic matrix to the velocity and orientation if there is one
		// this is done in a different place to the matrix application to position to speed
		// up lensflare culling
		if (matrixptr) {
			if (velocityptr != NULL) {
				MUrMatrix3x3_MultiplyPoint(velocityptr, (M3tMatrix3x3 *) matrixptr, &new_velocity);
				velocityptr = &new_velocity;
			}
			if (orientationptr != NULL) {
				MUrMatrix3x3_Multiply((M3tMatrix3x3 *) matrixptr, orientationptr, &new_orientation);
				orientationptr = &new_orientation;
			}
		}
		
		// set up the texture instance index
		if (texindex_offset != P3cNoVar) {
			texval = *((UUtUns32 *) &particle->vararray[texindex_offset]);
			M3rDraw_State_SetInt(M3cDrawStateIntType_TextureInstance, texval);
		}
		
		// set up the texture time index
		if (textime_offset != P3cNoVar) {
			texval = *((UUtUns32 *) &particle->vararray[textime_offset]);
			M3rDraw_State_SetInt(M3cDrawStateIntType_Time, inTime - texval);
		}
		
		// if we are fading out then use the current lifetime and fade time to calculate alpha
		// NB: this does *NOT* use the previous value of realalpha because when fading out, we calculate
		// fadeout_time to take our dynamic alpha into account. this means that lensflares can't fade out
		// by dying and also by being occluded at the same time, but this shouldn't be too much of a problem.
		if (particle->header.flags & P3cParticleFlag_FadeOut) {
			realalpha = (UUtUns16) (M3cMaxAlpha * particle->header.lifetime / particle->header.fadeout_time);
		}
		
		// are we being chopped?
		if (particle->header.flags & P3cParticleFlag_Chop) {
			chopval = 1.0f - particle->header.lifetime / particle->header.fadeout_time;
		} else {
			chopval = 0.0f;
		}
		
		if (do_edgefade) {
			MUmVector_Subtract(cameraDir, cameraPos, *positionptr);
			MUmVector_Normalize(cameraDir);

			switch(inMode) {
			case M3cGeomState_SpriteMode_Normal:
			case M3cGeomState_SpriteMode_Rotated:
				// although we don't use a direction to draw ourselves with,
				// we can still use it to edgefade out with
				if (orientationptr != NULL) {
					// get dot product with Y axis
					MUrMatrix_GetCol((M3tMatrix4x3 *) orientationptr, 1, &perp_dir);
				} else if (velocityptr != NULL) {
					// get dot product with velocity
					MUmVector_Copy(perp_dir, *velocityptr);
					MUmVector_Normalize(perp_dir);
				} else {
					// no direction to this sprite at all
					do_edgefade = UUcFalse;
				}
				break;

			case M3cGeomState_SpriteMode_Billboard:
				// edgefading is dependent on the dot product between our direction
				// and the camera direction
				//
				// because the sprite is drawn parallel to our direction, edgefade_val = 1 - dot
				if (orientationptr != NULL) {
					// get dot product with Y axis
					MUrMatrix_GetCol((M3tMatrix4x3 *) orientationptr, 1, &perp_dir);
				} else {
					// get dot product with velocity
					MUmVector_Copy(perp_dir, *velocityptr);
					MUmVector_Normalize(perp_dir);
				}
				break;

			case M3cGeomState_SpriteMode_Arrow:
				// sprite is drawn perpendicular to X
				MUrMatrix_GetCol((M3tMatrix4x3 *) orientationptr, 0, &perp_dir);
				break;

			case M3cGeomState_SpriteMode_Flat:
				// sprite is drawn perpendicular to Y
				MUrMatrix_GetCol((M3tMatrix4x3 *) orientationptr, 1, &perp_dir);
				break;

			case M3cGeomState_SpriteMode_Discus:
				// sprite is drawn perpendicular to Z
				MUrMatrix_GetCol((M3tMatrix4x3 *) orientationptr, 2, &perp_dir);
				break;

			default:
				do_edgefade = UUcFalse;
				break;
			}

			if (do_edgefade) {
				edgefade_val = MUrVector_DotProduct(&cameraDir, &perp_dir);
				if (twosided_edgefade) {
					edgefade_val = (float)fabs(edgefade_val);
				}
				if (inMode == M3cGeomState_SpriteMode_Billboard) {
					edgefade_val = 1.0f - edgefade_val;
				}


				if (edgefade_val < edgefade_min) {
					// fade out totally - don't draw
					continue;
				} else if (edgefade_val < edgefade_max) {
					// fade out partially
					realalpha = (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(realalpha *
								(edgefade_val - edgefade_min) / (edgefade_max - edgefade_min));
				}
			}
		}

		M3rSprite_Draw(texturemap, positionptr, currentscale, (use_yscale) ? y_scale : currentscale,
			tint, realalpha, rotation, velocityptr, orientationptr, x_offset, x_shorten, chopval);
	}
}

// display a class of contrail particles
void P3rDisplayClass_Contrail(P3tParticleClass *inClass, UUtUns32 inTime, UUtUns32 inDeltaTime,
							  M3tGeomState_SpriteMode inMode, UUtBool inEmitter)
{
	UUtUns16	constalpha, realalpha;
	UUtUns16	orient_offset, offsetpos_offset, dynmatrix_offset, texindex_offset, textime_offset, lensframes_offset;
	UUtUns16	contraildata_offset, linkcontraildata_offset;
	UUtUns32	texval, ref, emit_num;
	P3tEmitter *emitter;
	P3tParticleEmitterData *emitter_data;
	P3tParticleClass *link_class;
	P3tParticle *particle, *link_particle, *next_particle;
	M3tTextureMap *texturemap;
	P3tAppearance *appearance_params;
	float scale, floatalpha, start_v, contrail_width, emit_interval, time_since_emission, lensflare_tolerance;
	float edgefade_min, edgefade_max, edgefade_val, max_contrail_length, maxlength_sq;
	IMtShade tint;
	UUtBool dynamic_scale, dynamic_alpha, do_edgefade, is_lensflare;
	UUtBool dynamic_tint, is_dead, check_length, lensflare_visible;
	UUtError error;
	M3tPoint3D *positionptr;
	M3tContrailData *linkcontraildataptr, *contraildataptr;
	M3tMatrix3x3 *orientationptr, new_orientation;
	M3tMatrix4x3 *matrixptr;
	M3tVector3D cameraPos, cameraDir, contrailDir;
	M3tGeomCamera *activeCamera;

	// get the contrail-data offset
	if ((inClass->definition->flags & P3cParticleClassFlag_HasContrailData) == 0) {
		COrConsole_Printf("### '%s' requires contrail-data to draw as a contrail!",
			inClass->classname);
		return;
	}

	contraildata_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_ContrailData];
	UUmAssert((contraildata_offset >= 0) && 
			(contraildata_offset < (UUtUns16) (inClass->particle_size - sizeof(P3tParticleHeader))));
				
	if (inMode == M3cGeomState_SpriteMode_Contrail) {
		// this sprite drawing mode requires an orientation matrix
		if (inClass->definition->flags & P3cParticleClassFlag_HasOrientation) {
			orient_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_Orientation];
			UUmAssert((orient_offset >= 0) && 
				(orient_offset < (UUtUns16) (inClass->particle_size - sizeof(P3tParticleHeader))));
		} else {
			COrConsole_Printf("### '%s' requires orientation to use this contrail mode!",
				inClass->classname);
			return;
		}
	} else {
		orient_offset = P3cNoVar;
	}
				
	is_lensflare = ((inClass->definition->flags2 & P3cParticleClassFlag2_Appearance_LensFlare) != 0);
	lensflare_tolerance = 0.0f;
	if (is_lensflare) {
		P3iCalculateValue(NULL, &inClass->definition->appearance.lensflare_dist, (void *) &lensflare_tolerance);

		if (inClass->definition->flags & P3cParticleClassFlag_HasLensFrames) {
			lensframes_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_LensFrames];
			UUmAssert((lensframes_offset >= 0) && 
				(lensframes_offset < (UUtUns16) (inClass->particle_size - sizeof(P3tParticleHeader))));
		} else {
			lensframes_offset = P3cNoVar;
		}
	}

	// is there an offset position?
	if (inClass->definition->flags & P3cParticleClassFlag_HasOffsetPos) {
		offsetpos_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_OffsetPos];
		UUmAssert((offsetpos_offset >= 0) && 
			(offsetpos_offset < (UUtUns16) (inClass->particle_size - sizeof(P3tParticleHeader))));
	} else {
		offsetpos_offset = P3cNoVar;
	}
				
	// is there a dynamic matrix?
	if (inClass->definition->flags & P3cParticleClassFlag_HasDynamicMatrix) {
		dynmatrix_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_DynamicMatrix];
		UUmAssert((dynmatrix_offset >= 0) && 
				(dynmatrix_offset < (UUtUns16) (inClass->particle_size - sizeof(P3tParticleHeader))));
	} else {
		dynmatrix_offset = P3cNoVar;
	}
				
	// is there a texture index?
	if (inClass->definition->flags & P3cParticleClassFlag_HasTextureIndex) {
		texindex_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_TextureIndex];
		UUmAssert((texindex_offset >= 0) && 
				(texindex_offset < (UUtUns16) (inClass->particle_size - sizeof(P3tParticleHeader))));
	} else {
		texindex_offset = P3cNoVar;
	}			
				
	// is there a texture time?
	if (inClass->definition->flags & P3cParticleClassFlag_HasTextureTime) {
		textime_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_TextureTime];
		UUmAssert((textime_offset >= 0) && 
				(textime_offset < (UUtUns16) (inClass->particle_size - sizeof(P3tParticleHeader))));
	} else {
		textime_offset = P3cNoVar;
	}			
				
	appearance_params = (P3tAppearance *) &inClass->definition->appearance;
				
	// set up drawing parameters for this class
	dynamic_scale = P3iValueIsDynamic(&appearance_params->scale);
	if (!dynamic_scale)
		P3iCalculateValue(NULL, &appearance_params->scale, (void *) &scale);
				
	dynamic_alpha = P3iValueIsDynamic(&appearance_params->alpha);
	if (!dynamic_alpha) {
		P3iCalculateValue(NULL, &appearance_params->alpha, (void *) &floatalpha);
		constalpha = (UUtUns16) (floatalpha * M3cMaxAlpha);
	}
				
	dynamic_tint = P3iValueIsDynamic(&appearance_params->tint);
	if (!dynamic_tint)
		P3iCalculateValue(NULL, &appearance_params->tint, (void *) &tint);
			
	do_edgefade = ((inClass->definition->flags2 & P3cParticleClassFlag2_Appearance_EdgeFade) != 0);
	if (do_edgefade) {
		if (P3iValueIsDynamic(&appearance_params->edgefade_min) || P3iValueIsDynamic(&appearance_params->edgefade_max)) {
			COrConsole_Printf("### '%s': edgefade values must be constants, not random or variables");
			do_edgefade = UUcFalse;
		} else {
			P3iCalculateValue(NULL, &appearance_params->edgefade_min, (void *) &edgefade_min);
			P3iCalculateValue(NULL, &appearance_params->edgefade_max, (void *) &edgefade_max);
		}
	}

	if (P3iValueIsDynamic(&appearance_params->max_contrail)) {
		COrConsole_Printf("### '%s': max contrail length must be a constant, not random or a variable");
		check_length = UUcFalse;
	} else {
		P3iCalculateValue(NULL, &appearance_params->max_contrail, (void *) &max_contrail_length);
		maxlength_sq = UUmSQR(max_contrail_length);
		check_length = (maxlength_sq > 0.001);
	}

	error = TMrInstance_GetDataPtr(M3cTemplate_TextureMap, appearance_params->instance, &texturemap);
	if (error != UUcError_None) {
		if ((inClass->non_persistent_flags & P3cParticleClass_NonPersistentFlag_WarnedAboutNotFound) == 0) {
			COrConsole_Printf("PARTICLE TEXTURE NOT FOUND: particle '%s' texture '%s'", inClass->classname, appearance_params->instance);
			inClass->non_persistent_flags |= P3cParticleClass_NonPersistentFlag_WarnedAboutNotFound;
		}

		// default texture if none can be found
		error = TMrInstance_GetDataPtr(M3cTemplate_TextureMap, "notfoundtex", &texturemap);
		if (error != UUcError_None) {
			// give up
			return;
		}
	}
	
	// do we need to know where the camera is?
	if ((inMode == M3cGeomState_SpriteMode_ContrailRotated) || (do_edgefade)) {
		// get the camera's location
		M3rCamera_GetActive(&activeCamera);
		M3rCamera_GetViewData(activeCamera, &cameraPos, NULL, NULL);
	}

	if (inEmitter) {
		/*
		 * find which emitter is our contrail
		 */

		for (emit_num = 0, emitter = inClass->definition->emitter; emit_num < inClass->definition->num_emitters;
			 emit_num++, emitter++) {

			 if (emitter->link_type == (P3tEmitterLinkType) (P3cEmitterLinkType_LastEmission + emit_num)) {
				 // found it!
				 break;
			 }
		}
		if (emit_num >= inClass->definition->num_emitters) {
			COrConsole_Printf("### '%s': can't find contrail emitter", inClass->classname);
			return;
		}

		// get interval between particle emissions
		if (!P3iCalculateValue(inClass->headptr, &emitter->emitter_value[P3cEmitterValue_RateValOffset + 0],
								&emit_interval)) {
			emit_interval = 1.0f;
		}

		/*
		 * retrieve offsets into the emitted particle
		 */

		link_class = emitter->emittedclass;

		if (link_class->definition->flags & P3cParticleClassFlag_HasContrailData) {
			linkcontraildata_offset = link_class->optionalvar_offset[P3cParticleOptionalVar_ContrailData];
			UUmAssert((linkcontraildata_offset >= 0) && 
					(linkcontraildata_offset < (UUtUns16) (link_class->particle_size - sizeof(P3tParticleHeader))));
		} else {
			COrConsole_Printf("### '%s': emitted contrail particle has no contrail data!", inClass->classname);
			return;
		}
				

	} else {
		linkcontraildata_offset = contraildata_offset;
	}


	// set up drawing parameters for this class
	M3rGeom_State_Set(M3cGeomStateIntType_SpriteMode, inMode);
	M3rGeom_State_Commit();
								
	for (particle = inClass->headptr; particle != NULL; particle = next_particle) {
		next_particle = particle->header.nextptr;
					
		if (particle->header.flags & P3cParticleFlag_Hidden)
			continue;
					
		// calculate this particle's position
		positionptr = P3rGetPositionPtr(inClass, particle, &is_dead);
		if (is_dead) {
			continue;
		}

		// find the dynamic matrix if there is one
		if (dynmatrix_offset != P3cNoVar) {
			matrixptr = *((M3tMatrix4x3 **) &particle->vararray[dynmatrix_offset]);
		} else {
			matrixptr = NULL;
		}

		// calculate the contrail-data pointer that we will be storing our data in
		contraildataptr = (M3tContrailData *) &particle->vararray[contraildata_offset];

		MUmVector_Copy(contraildataptr->position, *positionptr);

		if (offsetpos_offset != P3cNoVar) {
			MUmVector_Add(contraildataptr->position, contraildataptr->position,
				*((M3tVector3D *) &particle->vararray[offsetpos_offset]));
		}
		
		// apply the dynamic matrix to position
		if (matrixptr) {
			MUrMatrix_MultiplyPoint(&contraildataptr->position, matrixptr, &contraildataptr->position);
		}

		if (dynamic_alpha) {
			P3iCalculateValue(particle, &appearance_params->alpha, (void *) &floatalpha);
			realalpha = (UUtUns16) (floatalpha * M3cMaxAlpha);
		} else {
			realalpha = constalpha;
		}

		if (is_lensflare) {
			lensflare_visible = P3rPointVisible(&contraildataptr->position, lensflare_tolerance);

			if (lensframes_offset == P3cNoVar) {
				// lensflares don't fade in and out
				if (!lensflare_visible)
					continue;
			} else {
				// calculate lensflare fading
				realalpha = P3rLensFlare_Fade(appearance_params, lensflare_visible, inDeltaTime,
												realalpha, (UUtUns32 *) &particle->vararray[lensframes_offset]);
				if (realalpha == 0)
				{
					continue;
				}
				else if ((realalpha= P3rModifyAlphaByFogFromDepth(realalpha, &contraildataptr->position)) == 0)
				{
					continue;
				}
			}
		}

		if (dynamic_scale)
			P3iCalculateValue(particle, &appearance_params->scale, (void *) &scale);
					
		// calculate the orientation
		if (orient_offset == P3cNoVar) {
			orientationptr = NULL;
		} else {
			orientationptr = (M3tMatrix3x3 *) &particle->vararray[orient_offset];
		}
					
		// apply the dynamic matrix to orientation
		if (matrixptr && (orientationptr != NULL)) {
			MUrMatrix3x3_Multiply((M3tMatrix3x3 *) matrixptr, orientationptr, &new_orientation);
			orientationptr = &new_orientation;
		}
					
		if (dynamic_tint) {
			P3iCalculateValue(particle, &appearance_params->tint, (void *) &contraildataptr->tint);
		} else {
			contraildataptr->tint = tint;
		}
					
		// if we are fading out then use the current lifetime and fade time to calculate alpha
		if (particle->header.flags & P3cParticleFlag_FadeOut) {
			contraildataptr->alpha = (UUtUns16) (M3cMaxAlpha * particle->header.lifetime
													/ particle->header.fadeout_time);
		} else {
			contraildataptr->alpha = realalpha;
		}
		
		if (inEmitter) {
			emitter_data = ((P3tParticleEmitterData *) particle->vararray) + emit_num;

			ref = emitter_data->last_particle;
			if (ref == P3cParticleReference_Null)
				continue;

			P3mUnpackParticleReference(ref, link_class, link_particle);

			if ((link_particle->header.self_ref != ref) || (link_particle->header.flags & P3cParticleFlag_BreakLinksToMe)) {
				// the link is dead - break it.
				emitter_data->last_particle = P3cParticleReference_Null;
				continue;
			}

			linkcontraildataptr = (M3tContrailData *) &link_particle->vararray[linkcontraildata_offset];

			// work out how much of the contrail needs to be drawn
			time_since_emission = (((float) inTime) / UUcFramesPerSecond) - emitter_data->time_last_emission;
			if (time_since_emission < 0) {
				start_v = 1.0f;
			} else if (time_since_emission > emit_interval) {
				start_v = 0.0f;
			} else {
				start_v = 1.0f - time_since_emission / emit_interval;
			}
		} else {
			// we can only draw a section of contrail if we aren't the last particle in the chain
			ref = particle->header.user_ref;
			if (ref == P3cParticleReference_Null)
				continue;
			
			P3mUnpackParticleReference(ref, link_class, link_particle);
			if (link_class != inClass) {
				COrConsole_Printf("### '%s' contrail-link points to a different class!", inClass->classname);
				continue;
			}

			if ((link_particle->header.self_ref != ref) || (link_particle->header.flags & P3cParticleFlag_BreakLinksToMe)) {
				// the contrail is dead - break it.
				particle->header.user_ref = P3cParticleReference_Null;
				P3rSendEvent(inClass, particle, P3cEvent_BrokenLink, particle->header.current_time);
				continue;
			}

			linkcontraildataptr = (M3tContrailData *) &link_particle->vararray[contraildata_offset];

			// contrail is full length
			start_v = 0.0f;
		}

		MUmVector_Subtract(contrailDir, linkcontraildataptr->position, contraildataptr->position);
		if (check_length && (MUmVector_GetLengthSquared(contrailDir) > maxlength_sq)) {
			// don't draw contrail links that get ridiculously long
			continue;
		}

		// we can now work out the width of the contrail at our point
		if (inMode == M3cGeomState_SpriteMode_Contrail) {
			// the contrail is oriented towards its X
			MUrMatrix_GetCol((M3tMatrix4x3 *) orientationptr, 0, &contraildataptr->width);
			MUmVector_Scale(contraildataptr->width, scale);

			if (do_edgefade) {
				// get the direction to the camera
				MUmVector_Subtract(cameraDir, contraildataptr->position, cameraPos);
			}
		} else {
			// get the direction to the camera and the direction of the contrail
			MUmVector_Subtract(cameraDir, contraildataptr->position, cameraPos);

			// the contrail is still oriented from one point towards the other,
			// but rotates to face the camera (i.e. perpendicular to cameraDir)
			MUrVector_CrossProduct(&contrailDir, &cameraDir, &contraildataptr->width);
			contrail_width = MUmVector_GetLength(contraildataptr->width);

			if (contrail_width > 0.01f) 
				MUmVector_Scale(contraildataptr->width, scale / contrail_width);
		}
			
		// if we are the first section of the contrail (i.e. our linked particle has a link to
		// a null reference) then our linked particle never got to set up its contraildata's width.
		// copy ours.
		//
		// FIXME: we need to insert some code here to deal with multiple duplicate contrail links
		// being created in the same place
		if (link_particle->header.user_ref == P3cParticleReference_Null) {
			MUmVector_Copy(linkcontraildataptr->width, contraildataptr->width);
		}

#if 0
		// TEMPORARY DEBUGGING CODE
		{
			M3tVector3D tempdir0, tempdir1;

			MUmVector_Copy(tempdir0, contraildataptr->width);
			MUmVector_Copy(tempdir1, linkcontraildataptr->width);
			MUmVector_Normalize(tempdir0);
			MUmVector_Normalize(tempdir1);

			if (MUrVector_DotProduct(&tempdir0, &tempdir1) < 0.5f) {
/*				COrConsole_Printf("%f %f %f / %f %f %f - %f %f %f / %f %f %f",
								contraildataptr->position.x,
								contraildataptr->position.y,
								contraildataptr->position.z,
								contraildataptr->width.x,
								contraildataptr->width.y,
								contraildataptr->width.z,
								linkcontraildataptr->position.x,
								linkcontraildataptr->position.y,
								linkcontraildataptr->position.z,
								linkcontraildataptr->width.x,
								linkcontraildataptr->width.y,
								linkcontraildataptr->width.z);*/
			}
		}
#endif

		// fade out the contrail if it's edge-on, according to the edgefade appearance parameters
		// this dot product is 1 for edge-on, 0 for perpendicular
		if (do_edgefade) {
			MUmVector_Normalize(contrailDir);
			MUmVector_Normalize(cameraDir);
			edgefade_val = 1.0f - (float)fabs(MUrVector_DotProduct(&contrailDir, &cameraDir));
			if (edgefade_val < edgefade_min) {
				// fade out totally
				continue;
			} else if (edgefade_val < edgefade_max) {
				// fade out partially
				contraildataptr->alpha = (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(contraildataptr->alpha *
							(edgefade_val - edgefade_min) / (edgefade_max - edgefade_min));
			}
		}

		// set up the texture instance index
		if (texindex_offset != P3cNoVar) {
			texval = *((UUtUns32 *) &particle->vararray[texindex_offset]);
			M3rDraw_State_SetInt(M3cDrawStateIntType_TextureInstance, texval);
		}
					
		// set up the texture time index
		if (textime_offset != P3cNoVar) {
			texval = *((UUtUns32 *) &particle->vararray[textime_offset]);
			M3rDraw_State_SetInt(M3cDrawStateIntType_Time, inTime - texval);
		}
					
#if 0
		if ((inTime % 30) == 0) {
		//	COrConsole_Printf("draw %f - %f", contraildataptr->position.x, linkcontraildataptr->position.x);
		}



		{
			M3tPoint3D pt0, pt1;

			MUmVector_Copy(pt0, contraildataptr->position);
			MUmVector_Copy(pt1, linkcontraildataptr->position);
			M3rGeom_Line_Light(&pt0, &pt1, IMcShade_Red);

			pt0.y -= 2.0f;
			pt1.y += 2.0f;
			M3rGeom_Line_Light(&pt0, &pt1, IMcShade_Blue);

			pt0.y += 4.0f;
			pt1.y -= 4.0f;
			M3rGeom_Line_Light(&pt0, &pt1, IMcShade_Blue);
		}
#endif

		M3rContrail_Draw(texturemap, start_v, 1.0f, contraildataptr, linkcontraildataptr);
	}
}

// get the alpha multiplier due to lensflare fading
UUtUns16 P3rLensFlare_Fade(P3tAppearance *inAppearance, UUtBool inVisible, UUtUns32 inDeltaTime,
						   UUtUns16 inAlpha, UUtUns32 *ioLensFrames)
{
	UUtUns32 type;
	UUtUns16 frames, fadein_frames, fadeout_frames;

	// ensure no divide-by-zero
	UUmAssertReadPtr(inAppearance, sizeof(P3tAppearance));
	fadein_frames  = UUmMax(inAppearance->lens_inframes, 1);
	fadeout_frames = UUmMax(inAppearance->lens_outframes, 1);

	UUmAssertReadPtr(ioLensFrames, sizeof(UUtUns32));
	type = *ioLensFrames & P3cLensFrames_TypeMask;
	frames = (UUtUns16) (*ioLensFrames & P3cLensFrames_FrameMask);
	UUmAssert(frames < UUmMax(fadein_frames, fadeout_frames));

	if (inVisible) {
		// the lensflare is visible this frame
		switch(type) {
		case P3cLensFrames_Unset:
			// start of a level
			*ioLensFrames = P3cLensFrames_Visible;

		case P3cLensFrames_Visible:
			return inAlpha;

		case P3cLensFrames_Invisible:
			// start to fade in
			frames = 0;
			break;

		case P3cLensFrames_FadeIn:
			// continue fading in
			break;

		case P3cLensFrames_FadeOut:
			// work out how far through our fade-out we were before becoming visible again
			// and so how far through our fade-in this makes us
			frames = fadein_frames - (frames * fadein_frames) / fadeout_frames;
			break;

		default:
			UUmAssert(!"P3rLensFlare_Fade: unknown lensflare type!");
			frames = 0;
			break;
		}

		// if we reach here, we are fading in
		frames += (UUtUns16) inDeltaTime;
		if (frames >= fadein_frames) {
			// completely visible
			*ioLensFrames = P3cLensFrames_Visible;
			return inAlpha;
		} else {
			// only partially visible
			*ioLensFrames = (P3cLensFrames_FadeIn | ((UUtUns32) frames));
			return ((inAlpha * frames) / fadein_frames);
		}

	} else {
		// the lensflare is not visible this frame
		switch(type) {
		case P3cLensFrames_Unset:
			// start of a level
			*ioLensFrames = P3cLensFrames_Invisible;

		case P3cLensFrames_Invisible:
			return 0;

		case P3cLensFrames_Visible:
			// start to fade out
			frames = 0;
			break;

		case P3cLensFrames_FadeOut:
			// continue fading out
			break;

		case P3cLensFrames_FadeIn:
			// work out how far through our fade-in we were before becoming invisible
			// and so how far through our fade-out this makes us
			frames = fadeout_frames - (frames * fadeout_frames) / fadein_frames;
			break;

		default:
			UUmAssert(!"P3rLensFlare_Fade: unknown lensflare type!");
			frames = 0;
			break;
		}

		// if we reach here, we are fading out
		frames += (UUtUns16) inDeltaTime;
		if (frames >= fadeout_frames) {
			// completely invisible
			*ioLensFrames = P3cLensFrames_Invisible;
			return 0;
		} else {
			// only partially visible
			*ioLensFrames = (P3cLensFrames_FadeOut | ((UUtUns32) frames));
			return ((inAlpha * (fadeout_frames - frames)) / fadeout_frames);
		}
	}
}

// get data about a subsection of the emitter's properties (rate, position, etc)
void P3rEmitterSubsection(UUtUns16 inSection, P3tEmitter *inEmitter, UUtUns32 **outPropertyPtr,
						  P3tEmitterSettingDescription **outClass, UUtUns16 *outOffset,
						  char *outDesc)
{
	switch(inSection) {
	case 0:
		*outPropertyPtr = (UUtUns32 *)&inEmitter->rate_type;
		*outClass = P3gEmitterRateDesc;
		*outOffset = P3cEmitterValue_RateValOffset;
		strcpy(outDesc, "rate");
		break;
		
	case 1:
		*outPropertyPtr = (UUtUns32 *)&inEmitter->position_type;
		*outClass = P3gEmitterPositionDesc;
		*outOffset = P3cEmitterValue_PositionValOffset;
		strcpy(outDesc, "position");
		break;
		
	case 2:
		*outPropertyPtr = (UUtUns32 *)&inEmitter->direction_type;
		*outClass = P3gEmitterDirectionDesc;
		*outOffset = P3cEmitterValue_DirectionValOffset;
		strcpy(outDesc, "direction");
		break;
		
	case 3:
		*outPropertyPtr = (UUtUns32 *)&inEmitter->speed_type;
		*outClass = P3gEmitterSpeedDesc;
		*outOffset = P3cEmitterValue_SpeedValOffset;
		strcpy(outDesc, "speed");
		break;
		
	case 4:
		*outPropertyPtr = (UUtUns32 *)&inEmitter->orientdir_type;
		*outClass = P3gEmitterOrientationDesc;
		*outOffset = P3cEmitterValue_OrientationValOffset;
		strcpy(outDesc, "orient dir");
		break;
		
	case 5:
		*outPropertyPtr = (UUtUns32 *)&inEmitter->orientup_type;
		*outClass = P3gEmitterOrientationDesc;
		*outOffset = P3cEmitterValue_OrientationValOffset;
		strcpy(outDesc, "orient up");
		break;
		
	default:
		UUmAssert(0);
	}
}	

// create space for a new particle class
static P3tParticleClass *P3iAllocateParticleClass(void)
{
	if (P3gNumClasses >= P3cMaxParticleClasses)
		return NULL;

	return &P3gClassTable[P3gNumClasses++];
}

// create a new (blank) particle class
P3tParticleClass *P3rNewParticleClass(char *inName, UUtBool inMakeDirty)
{
	P3tParticleClass *new_class;
	UUtUns16 itr;

	UUmAssert(strlen(inName) <= P3cParticleClassNameLength);

	new_class = P3iAllocateParticleClass();
	if (new_class == NULL)
		return NULL;

	P3gDirty = inMakeDirty;
	strcpy(new_class->classname, inName);
	new_class->non_persistent_flags = 0;
	if (inMakeDirty) {
		new_class->non_persistent_flags |= P3cParticleClass_NonPersistentFlag_Dirty;
	}

	new_class->num_particles = 0;
	new_class->headptr = new_class->tailptr = NULL;
	new_class->prev_active_class = new_class->next_active_class = P3cClass_None;
	new_class->particle_size = sizeof(P3tParticleHeader);
	new_class->flyby_sound = NULL;

	new_class->originalcopy.block = NULL;
	new_class->originalcopy.size = 0;
	new_class->memory.size = sizeof(P3tParticleDefinition);
	new_class->memory.block = UUrMemory_Block_New(new_class->memory.size + sizeof(BDtHeader));
	if (new_class->memory.block == NULL) {
		// bail out; particle class cannot be created
		P3gNumClasses--;
		return NULL;
	}

	new_class->definition = P3mSkipHeader(new_class->memory.block);
	new_class->definition->definition_size = sizeof(P3tParticleDefinition);

	// set up defaults for the new particle class
	for (itr = 0; itr < P3cMaxNumEvents; itr++) {
		new_class->definition->eventlist[itr].start_index = 0;
		new_class->definition->eventlist[itr].end_index = 0;
	}

	new_class->definition->flags = 0;
	new_class->definition->flags2 = 0;
	new_class->definition->version = P3cVersion_Current;
	new_class->definition->num_actions = 0;
	new_class->definition->num_emitters = 0;
	new_class->definition->num_variables = 0;
	new_class->definition->max_particles = 256;

	new_class->definition->lifetime.type = P3cValueType_Float;
	new_class->definition->lifetime.u.float_const.val = 10.0f;

	new_class->definition->collision_radius.type = P3cValueType_Float;
	new_class->definition->collision_radius.u.float_const.val = 0.0f;

	new_class->definition->dodge_radius = 0;
	new_class->definition->alert_radius = 0;
	UUrString_Copy(new_class->definition->flyby_soundname, "", P3cStringVarSize + 1);

	new_class->definition->appearance.alpha.type = P3cValueType_Float;
	new_class->definition->appearance.alpha.u.float_const.val = 1.0f;

	new_class->definition->appearance.rotation.type = P3cValueType_Float;
	new_class->definition->appearance.rotation.u.float_const.val = 0.0f;

	new_class->definition->appearance.scale.type = P3cValueType_Float;
	new_class->definition->appearance.scale.u.float_const.val = 1.0f;

	new_class->definition->appearance.y_scale.type = P3cValueType_Float;
	new_class->definition->appearance.y_scale.u.float_const.val = 1.0f;

	new_class->definition->appearance.x_shorten.type = P3cValueType_Float;
	new_class->definition->appearance.x_shorten.u.float_const.val = 0.0f;

	new_class->definition->appearance.x_offset.type = P3cValueType_Float;
	new_class->definition->appearance.x_offset.u.float_const.val = 0.0f;

	new_class->definition->appearance.tint.type = P3cValueType_Shade;
	new_class->definition->appearance.tint.u.shade_const.val = IMcShade_White;

	new_class->definition->appearance.edgefade_min.type = P3cValueType_Float;
	new_class->definition->appearance.edgefade_min.u.float_const.val = 0.0f;

	new_class->definition->appearance.edgefade_max.type = P3cValueType_Float;
	new_class->definition->appearance.edgefade_max.u.float_const.val = 0.5f;

	new_class->definition->appearance.max_contrail.type = P3cValueType_Float;
	new_class->definition->appearance.max_contrail.u.float_const.val = 0.0f;

	new_class->definition->appearance.lensflare_dist.type = P3cValueType_Float;
	new_class->definition->appearance.lensflare_dist.u.float_const.val = 0.0f;

	new_class->definition->appearance.lens_inframes = 0;
	new_class->definition->appearance.lens_outframes = 0;

	new_class->definition->appearance.max_decals = 100;
	new_class->definition->appearance.decal_fadeframes = 60;

	new_class->definition->appearance.decal_wrap.type = P3cValueType_Float;
	new_class->definition->appearance.decal_wrap.u.float_const.val = 60.0f;

	strcpy(new_class->definition->appearance.instance, "notfoundtex");

	new_class->definition->attractor.iterator_type = P3cAttractorIterator_None;
	new_class->definition->attractor.selector_type = P3cAttractorSelector_Distance;
	UUrString_Copy(new_class->definition->attractor.attractor_name, "", P3cParticleClassNameLength + 1);
	new_class->definition->attractor.max_distance.type = P3cValueType_Float;
	new_class->definition->attractor.max_distance.u.float_const.val = 150.0f;
	new_class->definition->attractor.max_angle.type = P3cValueType_Float;
	new_class->definition->attractor.max_angle.u.float_const.val = 30.0f;
	new_class->definition->attractor.select_thetamin.type = P3cValueType_Float;
	new_class->definition->attractor.select_thetamin.u.float_const.val = 3.0f;
	new_class->definition->attractor.select_thetamax.type = P3cValueType_Float;
	new_class->definition->attractor.select_thetamax.u.float_const.val = 20.0f;
	new_class->definition->attractor.select_thetaweight.type = P3cValueType_Float;
	new_class->definition->attractor.select_thetaweight.u.float_const.val = 3.0f;

	new_class->prev_packed_emitters = (UUtUns16) -1;
	P3rPackVariables(new_class, UUcFalse);
	P3iCalculateArrayPointers(new_class->definition);

	// determine the size class
	for (itr = 0; itr < P3cMemory_ParticleSizeClasses; itr++) {
		if (new_class->particle_size <= P3gParticleSizeClass[itr])
			break;
	}
	if (itr >= P3cMemory_ParticleSizeClasses) {
		UUrPrintWarning("Particle class '%s' is too large (%d) for largest size class (%d)!",
						inName, new_class->particle_size, P3gParticleSizeClass[P3cMemory_ParticleSizeClasses - 1]);

		// bail out; particle class cannot be created
		UUrMemory_Block_Delete(new_class->memory.block);
		P3gNumClasses--;
		return NULL;
	} else {
		new_class->size_class = itr;
	}

	return new_class;
}

// duplicate an existing particle class
P3tParticleClass *P3rDuplicateParticleClass(P3tParticleClass *inClass, char *inName)
{
	P3tParticleClass *new_class;

	UUmAssert(strlen(inName) <= P3cParticleClassNameLength);

	new_class = P3iAllocateParticleClass();
	if (new_class == NULL)
		return NULL;

	P3gDirty = UUcTrue;
	strcpy(new_class->classname, inName);

	// copy certain of the old class's flags
	new_class->non_persistent_flags = inClass->non_persistent_flags &
		(P3cParticleClass_NonPersistentFlag_HasSoundPointers | P3cParticleClass_NonPersistentFlag_OldVersion |
		 P3cParticleClass_NonPersistentFlag_Disabled);
	new_class->non_persistent_flags |= P3cParticleClass_NonPersistentFlag_Dirty;

	new_class->num_particles = 0;
	new_class->headptr = new_class->tailptr = NULL;
	new_class->prev_active_class = new_class->next_active_class = P3cClass_None;
	new_class->particle_size = inClass->particle_size;
	new_class->size_class = inClass->size_class;
	
	// there must be enough room for the incoming class in the block that it has had allocated
	UUmAssert(inClass->memory.size >= inClass->definition->definition_size);

	new_class->originalcopy.block = NULL;
	new_class->originalcopy.size = 0;
	new_class->memory.size = inClass->memory.size;
	new_class->memory.block = UUrMemory_Block_New(new_class->memory.size + sizeof(BDtHeader));
	if (new_class->memory.block == NULL) {
		// bail out; particle class cannot be created
		P3gNumClasses--;
		return NULL;
	}

	new_class->definition = P3mSkipHeader(new_class->memory.block);
	UUrMemory_MoveFast(inClass->definition, new_class->definition, inClass->definition->definition_size);
	P3iCalculateArrayPointers(new_class->definition);
	P3rPackVariables(new_class, UUcFalse);

	return new_class;
}

// damage a quad in the environment
UUtBool P3iDamageQuad(AKtEnvironment *inEnvironment, UUtUns32 inGQIndex, float inDamage,
					  M3tPoint3D *inBlastCenter, M3tVector3D *inBlastDir, float inBlastRadius,
					  UUtBool inForceBreak)
{
	M3tVector3D quad_normal, *blast_dir;
	AKtGQ_Collision *collision_gq;
	float plane_dist, blast_side;
	UUtBool breakable;

	breakable = P3gEverythingBreakable;
	breakable = breakable || ((inEnvironment->gqGeneralArray->gqGeneral[inGQIndex].flags & AKcGQ_Flag_Breakable) > 0);
	breakable = breakable || (P3gFurnitureBreakable && ((inEnvironment->gqGeneralArray->gqGeneral[inGQIndex].flags & AKcGQ_Flag_Furniture) > 0));
	if (!breakable) {
		return UUcFalse;
	}

#if defined(DEBUGGING) && DEBUGGING
	if (!P3gEverythingBreakable && !P3gFurnitureBreakable) {
		AKtGQ_Render *render_gq;
		M3tTextureMap *texture;
		UUtUns32 tex_index;

		// get the texture off this GQ and check that it is marked as breakable - if it isn't,
		// then an error has occurred and the breakable flag got set on a GQ incorrectly
		render_gq = inEnvironment->gqRenderArray->gqRender + inGQIndex;
		tex_index = render_gq->textureMapIndex;
		UUmAssert(tex_index < inEnvironment->textureMapArray->numMaps);

		texture = inEnvironment->textureMapArray->maps[tex_index];
		UUmAssert(MArMaterialType_IsBreakable((MAtMaterialType) texture->materialType));
	}
#endif

	collision_gq = inEnvironment->gqCollisionArray->gqCollision + inGQIndex;
	AKmPlaneEqu_GetComponents(collision_gq->planeEquIndex,
							inEnvironment->planeArray->planes,
							quad_normal.x, quad_normal.y, quad_normal.z, plane_dist);

	if (NULL != inBlastCenter) {
		// make sure the normal points in the direction that the blast came from - fixes issues
		// with two-sided glass quads emitting glass shards back towards the blast
		blast_side = MUrVector_DotProduct(inBlastCenter, &quad_normal) + plane_dist;
	}
	else {
		blast_side = 0;
	}

	if (blast_side < -P3cParallelBlastTolerance) {
		// the blast occurred 'behind' the quad. reverse our normal, and blow out in the
		// direction of the normal.
		MUmVector_Negate(quad_normal);
		blast_dir = NULL;
	} else if (blast_side > P3cParallelBlastTolerance) {
		// the blast occurred 'in front of' the quad. leave our normal unchanged and blow out
		// in this direction.
		blast_dir = NULL;
	} else {
		// the blast basically occurred in the plane of the glass. blow in the direction that
		// the blast came in from.
		if ((inBlastDir != NULL) && (MUrVector_DotProduct(inBlastDir, &quad_normal) > 0)) {
			MUmVector_Negate(quad_normal);
		}
		blast_dir = inBlastDir;
	}

	ONrParticle3_EmitGlassPieces(&inEnvironment->gqGeneralArray->gqGeneral[inGQIndex], inDamage,
								inBlastCenter, &quad_normal, blast_dir, inBlastRadius);
	return UUcTrue;
}

/*
 * particle attractors
 */

// set up attractor pointers for one or many classes
void P3rSetupAttractorPointers(P3tParticleClass *inClass)
{
	UUtUns32 itr, num_classes;

	// if changes take place during the level-unload process, we don't need to know about them
	if (P3gUnloading) {
		return;
	}

	if (inClass == NULL) {
		inClass = P3gClassTable;
		num_classes = P3gNumClasses;
	} else {
		num_classes = 1;
	}

	for (itr = 0; itr < num_classes; inClass++, itr++) {
		switch(inClass->definition->attractor.iterator_type) {
			case P3cAttractorIterator_ParticleClass:
				inClass->definition->attractor.attractor_ptr = (void *)
						P3rGetParticleClass(inClass->definition->attractor.attractor_name);
			break;

			case P3cAttractorIterator_ParticleTag:
				inClass->definition->attractor.attractor_ptr = (void *)
						EPrTagHash_Retrieve(inClass->definition->attractor.attractor_name);
			break;

			default:
				inClass->definition->attractor.attractor_ptr = NULL;
			break;
		}
	}
}

// get the location of a character to attract to
static void P3rGetCharacterAttractor(ONtCharacter *inCharacter, M3tPoint3D *outPoint)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCharacter);

	if (active_character == NULL) {
		ONrCharacter_GetPelvisPosition(inCharacter, outPoint);
	} else {
		ONrCharacter_GetPartLocation(inCharacter, active_character, ONcSpine1_Index, outPoint);
	}
}

// decode an attractor index
UUtBool P3rDecodeAttractorIndex(UUtUns32 inAttractorID, UUtBool inPredictVelocity, M3tPoint3D *outPoint, M3tPoint3D *outVelocity)
{
	if (inAttractorID == (UUtUns32) -1) {
		return UUcFalse;

	} else if (inAttractorID & P3cAttractor_ParticleReference) {
		P3tParticleClass *classptr;
		P3tParticle *particle;
		
		P3mUnpackParticleReference(inAttractorID, classptr, particle);
		if ((particle->header.self_ref != inAttractorID) || (P3rGetRealPosition(classptr, particle, outPoint))) {
			// attractor is dead
			return UUcFalse;
		} else {
			if (inPredictVelocity && (outVelocity != NULL)) {
				// get the particle's velocity
				M3tVector3D *velocity = P3rGetVelocityPtr(classptr, particle);

				if (velocity == NULL) {
					MUmVector_Set(*outVelocity, 0, 0, 0);
				} else {
					MUmVector_Copy(*outVelocity, *velocity);
				}
			}
			return UUcTrue;
		}
	} else if (inAttractorID & P3cAttractor_CharacterIndex) {
		UUtUns32 character_index = inAttractorID & ~P3cAttractor_CharacterIndex;
		UUtUns32 raw_index = character_index & ONcCharacterIndexMask_RawIndex;
		ONtCharacter *character;

		UUmAssert((raw_index >= 0) && (raw_index < ONcMaxCharacters));
		character = ONrGameState_GetCharacterList() + raw_index;
		if (((character->flags & ONcCharacterFlag_InUse) == 0) || (character->index != character_index)) {
			return UUcFalse;
		}
		P3rGetCharacterAttractor(character, outPoint);

		if (inPredictVelocity && (outVelocity != NULL)) {
			// get the character's approximate velocity
			MUmVector_Subtract(*outVelocity, character->location, character->prev_location);
			MUmVector_Scale(*outVelocity, UUcFramesPerSecond);
		}

		return UUcTrue;

	} else {
		// unknown kind of attractor ID
		return UUcFalse;
	}
}

// find all potential attractors and store them in a global buffer
void P3rIterateAttractors(P3tParticleClass *inClass, P3tParticle *inParticle, UUtUns32 inDelay)
{
	UUtBool is_dead, found, success, obey_max_angle;
	UUtUns32 candidate_index, num_collisions;
	P3tAttractorData *attractor_data;
	P3tAttractor *attractor;
	P3tAttractorIteratorFunction iterator_func;
	M3tMatrix3x3 *p_orientation;
	M3tMatrix4x3 **p_dynmatrix;
	M3tVector3D *p_velocity, our_orientation, delta_pos, dir_to_target;
	M3tPoint3D our_point, candidate_point;
	float candidate_dist, max_distance, max_angle;
	AKtEnvironment *env;

	attractor_data = P3rGetAttractorDataPtr(inClass, inParticle);
	if (attractor_data == NULL) {
		return;
	}

	// check to see that we aren't currently disabled from checking
	if ((inDelay != 0) && (inParticle->header.current_tick < attractor_data->delay_tick)) {
		return;
	}

	// get the iteration function to use
	attractor = &inClass->definition->attractor;
	UUmAssert((attractor->iterator_type >= 0) && (attractor->iterator_type < P3cAttractorIterator_Max));
	iterator_func = P3gAttractorIteratorTable[attractor->iterator_type].function;

	// nothing found yet
	P3gAttractorBuf_NumAttractors = 0;
	P3gAttractorBuf_Valid = UUcTrue;
	
	if (iterator_func == NULL) {
		if (attractor_data->index != (UUtUns32) -1) {
			P3gAttractorBuf_Storage[0] = attractor_data->index;
			P3gAttractorBuf_NumAttractors++;
		}
		return;
	}

	// initialize the iterator for the pass
	P3mAssertFloatValue(attractor->max_distance);
	P3iCalculateValue(inParticle, &attractor->max_distance, (void *) &max_distance);
	success = iterator_func(inClass, inParticle, attractor, NULL, &max_distance, NULL);
	if (!success) {
		goto exit;
	}

	// are we restricted to lie within a certain angle?
	obey_max_angle = UUcFalse;
	if (attractor->selector_type == P3cAttractorSelector_Conical) {
		P3mAssertFloatValue(attractor->max_angle);
		success = P3iCalculateValue(inParticle, &attractor->max_angle, (void *) &max_angle);
		if (success && (max_angle < 180.0f)) {
			max_angle *= M3cDegToRad;
			obey_max_angle = UUcTrue;
		}
	}

	if (obey_max_angle) {
		is_dead = P3rGetRealPosition(inClass, inParticle, &our_point);
		if (is_dead) {
			return;
		}

		p_orientation = P3rGetOrientationPtr(inClass, inParticle);
		if (p_orientation == NULL) {
			p_velocity = P3rGetVelocityPtr(inClass, inParticle);
			if (p_velocity == NULL) {
				obey_max_angle = UUcFalse;
			} else {
				MUmVector_Copy(our_orientation, *p_velocity);
			}
		} else {
			our_orientation = MUrMatrix_GetYAxis((M3tMatrix4x3 *) p_orientation);
		}

		if (obey_max_angle) {
			p_dynmatrix = P3rGetDynamicMatrixPtr(inClass, inParticle);
			if (p_dynmatrix != NULL) {
				MUrMatrix3x3_MultiplyPoint(&our_orientation, (M3tMatrix3x3 *) *p_dynmatrix, &our_orientation);
			}

			MUmVector_Normalize(our_orientation);
		}
	}
	env = ONrGameState_GetEnvironment();

	// run the iteration pass
	while (P3gAttractorBuf_NumAttractors < P3cAttractorBuf_Max) {
		found = iterator_func(inClass, inParticle, attractor, &candidate_point, &candidate_dist, &candidate_index);
		if (!found) {
			break;
		}

		if (obey_max_angle) {
			// check that this attractor doesn't lie outside the maximum angle allowed
			MUmVector_Subtract(delta_pos, candidate_point, our_point);
			MUmVector_Copy(dir_to_target, delta_pos);
			MUmVector_Normalize(dir_to_target);
			if (MUrAngleBetweenVectors3D(&our_orientation, &dir_to_target) > max_angle) {
				continue;
			}
		}

		if (inClass->definition->flags2 & P3cParticleClassFlag2_Attractor_CheckEnvironment) {
			AKtOctNodeIndexCache *p_envcache = P3rGetEnvironmentCache(inClass, inParticle);

			if (p_envcache == NULL) {
				num_collisions = AKrCollision_Point(env, &our_point, &delta_pos, AKcGQ_Flag_Obj_Col_Skip, 0);
			} else {
				num_collisions = AKrCollision_Point_SpatialCache(env, &our_point, &delta_pos, AKcGQ_Flag_Obj_Col_Skip,
																 0, p_envcache, NULL);
			}

			if (num_collisions > 0) {
				// there is a wall blocking us, don't consider this candidate
				continue;
			}
		}

		P3gAttractorBuf_Storage[P3gAttractorBuf_NumAttractors++] = candidate_index;
	}

exit:
	// set up a delay so that this function isn't called again too soon
	attractor_data->index = (P3gAttractorBuf_NumAttractors == 0) ? (UUtUns32) -1 : P3gAttractorBuf_Storage[0];
	attractor_data->delay_tick = inParticle->header.current_tick + inDelay;
}

// find the object that we are being attracted to
void P3rFindAttractor(P3tParticleClass *inClass, P3tParticle *inParticle, UUtUns32 inDelay)
{
	UUtBool found, stop, success;
	UUtUns32 attractor_index, candidate_index, delay;
	P3tAttractorData *attractor_data;
	P3tAttractor *attractor;
	P3tAttractorIteratorFunction iterator_func;
	P3tAttractorSelectorFunction selector_func;
	M3tPoint3D candidate_point;
	float candidate_dist, max_distance;

	attractor_data = P3rGetAttractorDataPtr(inClass, inParticle);
	if (attractor_data == NULL) {
		return;
	}

	if (inDelay == 0) {
		if (inParticle->header.flags & P3cParticleFlag_HasAttractor) {
			// our parent has already set up an attractor for us, bail
			return;
		}

		// initial creation - set the delay tick
		attractor_data->index = (UUtUns32) -1;
		attractor_data->delay_tick = inParticle->header.current_tick;
	} else {
		// check to see that we aren't currently disabled from checking
		if (inParticle->header.current_tick < attractor_data->delay_tick) {
			return;
		}
	}

	// get the iteration and selection functions to use
	attractor = &inClass->definition->attractor;
	UUmAssert((attractor->iterator_type >= 0) && (attractor->iterator_type < P3cAttractorIterator_Max));
	UUmAssert((attractor->selector_type >= 0) && (attractor->selector_type < P3cAttractorSelector_Max));
	iterator_func = P3gAttractorIteratorTable[attractor->iterator_type].function;
	selector_func = P3gAttractorSelectorTable[attractor->selector_type].function;

	if (iterator_func == NULL) {
		return;
	}

	// nothing found yet
	attractor_index = (UUtUns32) -1;
	
	P3mAssertFloatValue(attractor->max_distance);
	P3iCalculateValue(inParticle, &attractor->max_distance, (void *) &max_distance);

	// initialize the iterator and selector for the pass
	success = iterator_func(inClass, inParticle, attractor, NULL, &max_distance, NULL);
	if (!success) {
		goto exit;
	}
	success = selector_func(inClass, inParticle, attractor, NULL, max_distance, attractor_data->index, NULL);
	if (!success) {
		goto exit;
	}

	// run the selection pass
	while (1) {
		found = iterator_func(inClass, inParticle, attractor, &candidate_point, &candidate_dist, &candidate_index);
		if (!found) {
			break;
		}

		stop = selector_func(inClass, inParticle, attractor, &candidate_point, candidate_dist, candidate_index, &attractor_index);
		if (stop) {
			break;
		}
	}

exit:
	delay = inDelay;
	if ((inParticle->header.flags & P3cParticleFlag_HasAttractor) == 0) {
		// add a small random delay - this is so that we don't check attractors on the
		// same frame for all particles created at the same time
		delay += (P3cAttractor_RandomDelay * UUrLocalRandom()) / UUcMaxUns16;
	}

	// store the attractor that we found (or didn't find) in the particle and set up a delay so that this function
	// isn't called again too soon
	attractor_data->index = attractor_index;
	if (attractor_index == (UUtUns32) -1) {
		inParticle->header.flags &= ~P3cParticleFlag_HasAttractor;
	} else {
		inParticle->header.flags |= P3cParticleFlag_HasAttractor;
	}
	attractor_data->delay_tick = inParticle->header.current_tick + delay;
}

UUtBool P3iAttractorIterator_None(P3tParticleClass *inClass, P3tParticle *inParticle, P3tAttractor *inAttractor,
												M3tPoint3D *outFoundPoint, float *ioDistance, UUtUns32 *ioIterator)
{
	return UUcFalse;
}

UUtBool P3iAttractorIterator_Link(P3tParticleClass *inClass, P3tParticle *inParticle, P3tAttractor *inAttractor,
												M3tPoint3D *outFoundPoint, float *ioDistance, UUtUns32 *ioIterator)
{
	P3tParticleReference ref;
	P3tParticleClass *classptr;
	P3tParticle *particleptr;
	float dist_sq;

	// static iteration-sequence state variables
	static M3tPoint3D our_point;
	static float distance_threshold;

	if (ioIterator == NULL) {
		// perform setup for an iteration sequence
		if (P3rGetRealPosition(inClass, inParticle, &our_point)) {
			// we have died
			return UUcFalse;
		}
		distance_threshold = UUmSQR(*ioDistance);

		return UUcTrue;
	}

	ref = inParticle->header.user_ref;
	if (ref == P3cParticleReference_Null) {
		return UUcFalse;
	}

	if (*ioIterator == ref) {
		// we have already returned this link, don't return it again
		return UUcFalse;
	}

	P3mUnpackParticleReference(ref, classptr, particleptr);
	if ((particleptr->header.self_ref != ref) || (particleptr->header.flags & P3cParticleFlag_BreakLinksToMe)) {
		// the link has been broken, give up
		return UUcFalse;
	}

	// get the particle's location
	if (P3rGetRealPosition(classptr, particleptr, outFoundPoint)) {
		// particle has died
		return UUcFalse;
	}

	dist_sq = MUmVector_GetDistanceSquared(our_point, *outFoundPoint);
	if (dist_sq > distance_threshold) {
		return UUcFalse;
	}

	// this is an acceptable candidate
	*ioIterator = ref;
	*ioDistance = MUrSqrt(dist_sq);
	return UUcTrue;
}

UUtBool P3iAttractorIterator_Class(P3tParticleClass *inClass, P3tParticle *inParticle, P3tAttractor *inAttractor,
												M3tPoint3D *outFoundPoint, float *ioDistance, UUtUns32 *ioIterator)
{
	float dist_sq;

	// static iteration-sequence state variables
	static M3tPoint3D our_point;
	static float distance_threshold;
	static P3tParticleClass *classptr;
	static P3tParticle *next_particleptr;

	if (ioIterator == NULL) {
		// perform setup for an iteration sequence
		classptr = (P3tParticleClass *) inClass->definition->attractor.attractor_ptr;
		if (classptr == NULL) {
			return UUcFalse;
		}
		next_particleptr = classptr->headptr;

		// get our position
		if (P3rGetRealPosition(inClass, inParticle, &our_point)) {
			// we have died
			return UUcFalse;
		}
		distance_threshold = UUmSQR(*ioDistance);

		return UUcTrue;
	}

	for( ; next_particleptr != NULL; next_particleptr = next_particleptr->header.nextptr) {
		// get the particle's location
		if (P3rGetRealPosition(classptr, next_particleptr, outFoundPoint)) {
			// particle has died
			continue;
		}

		dist_sq = MUmVector_GetDistanceSquared(our_point, *outFoundPoint);
		if (dist_sq > distance_threshold) {
			continue;
		}

		// this is an acceptable candidate
		*ioIterator = next_particleptr->header.self_ref;
		*ioDistance = MUrSqrt(dist_sq);

		next_particleptr = next_particleptr->header.nextptr;
		return UUcTrue;
	}

	return UUcFalse;
}

UUtBool P3iAttractorIterator_Tag(P3tParticleClass *inClass, P3tParticle *inParticle, P3tAttractor *inAttractor,
												M3tPoint3D *outFoundPoint, float *ioDistance, UUtUns32 *ioIterator)
{
	float dist_sq;

	// static iteration-sequence state variables
	static M3tPoint3D our_point;
	static float distance_threshold;
	static EPtEnvParticle *next_particleptr;

	if (ioIterator == NULL) {
		// perform setup for an iteration sequence
		next_particleptr = (EPtEnvParticle *) inClass->definition->attractor.attractor_ptr;
		if (next_particleptr == NULL) {
			return UUcFalse;
		}

		// get our position
		if (P3rGetRealPosition(inClass, inParticle, &our_point)) {
			// we have died
			return UUcFalse;
		}
		distance_threshold = UUmSQR(*ioDistance);

		return UUcTrue;
	}

	for( ; next_particleptr != NULL; next_particleptr = next_particleptr->taglist) {
		if ((next_particleptr->particle_class == NULL) || (next_particleptr->particle == NULL)) {
			// no particle to attract to
			continue;
		}

		// get the particle's location
		if (P3rGetRealPosition(next_particleptr->particle_class, next_particleptr->particle, outFoundPoint)) {
			// particle has died
			continue;
		}

		dist_sq = MUmVector_GetDistanceSquared(our_point, *outFoundPoint);
		if (dist_sq > distance_threshold) {
			continue;
		}

		// this is an acceptable candidate
		*ioIterator = next_particleptr->particle->header.self_ref;
		*ioDistance = MUrSqrt(dist_sq);

		next_particleptr = next_particleptr->taglist;
		return UUcTrue;
	}

	return UUcFalse;
}

UUtBool P3iAttractorIterator_Character(P3tParticleClass *inClass, P3tParticle *inParticle, P3tAttractor *inAttractor,
												M3tPoint3D *outFoundPoint, float *ioDistance, UUtUns32 *ioIterator)
{
	ONtCharacter *this_character;
	P3tOwner *p_owner;
	float dist_sq;

	// static iteration-sequence state variables
	static UUtUns32 active_character_count, active_itr;
	static ONtCharacter **active_character_list;
	static M3tPoint3D our_point;
	static float distance_threshold;
	static UUtUns16 our_team;
	static ONtCharacter *our_character;

	if (ioIterator == NULL) {
		// perform setup for an iteration sequence
		if (P3rGetRealPosition(inClass, inParticle, &our_point)) {
			// we have died
			return UUcFalse;
		}
		distance_threshold = UUmSQR(*ioDistance);
		active_character_count = ONrGameState_ActiveCharacterList_Count();
		active_character_list = ONrGameState_ActiveCharacterList_Get();
		active_itr = 0;
		our_team = (UUtUns16) -1;
		our_character = NULL;

		if (inAttractor->iterator_type != P3cAttractorIterator_AllCharacters) {
			// find out which character and team we belong to
			p_owner = P3rGetOwnerPtr(inClass, inParticle);
			if (p_owner != NULL) {
				if (WPrOwner_IsCharacter(*p_owner, &our_character)) {
					our_team = (our_character == NULL) ? OBJcCharacter_TeamSyndicate : our_character->teamNumber;
				}
			}
		}

		return UUcTrue;
	}

	while (active_itr < active_character_count) {
		this_character = active_character_list[active_itr++];

		// don't track onto dead characters
		if (this_character->flags & ONcCharacterFlag_Dead) {
			continue;
		}

		if (inAttractor->iterator_type == P3cAttractorIterator_Character) {
			// skip only our owner
			if (this_character == our_character) {
				continue;
			}
		} else if (inAttractor->iterator_type == P3cAttractorIterator_HostileCharacter) {
			// don't track onto characters that we are friendly towards
			if ((our_team != (UUtUns16) -1) &&
				((this_character == our_character) || (AI2rTeam_FriendOrFoe(our_team, this_character->teamNumber) == AI2cTeam_Friend))) {
				continue;
			}
		}

		// get the character's location
		P3rGetCharacterAttractor(this_character, outFoundPoint);
		dist_sq = MUmVector_GetDistanceSquared(our_point, *outFoundPoint);
		if (dist_sq > distance_threshold) {
			// too far away
			continue;
		}

		// this is an acceptable candidate to home on
		*ioIterator = P3cAttractor_CharacterIndex | this_character->index;
		*ioDistance = MUrSqrt(dist_sq);
		return UUcTrue;
	}

	// no more characters
	return UUcFalse;

}

// selector functions
UUtBool P3iAttractorSelector_Distance(P3tParticleClass *inClass, P3tParticle *inParticle, P3tAttractor *inAttractor,
												M3tPoint3D *inFoundPoint, float inDistance, UUtUns32 inFoundID, UUtUns32 *ioChoice)
{
	float score;
	UUtBool is_dead;
	M3tVector3D delta_pos;

	// static selection-sequence state variables
	static float current_score;	
	static UUtUns32 previous_id;
	static M3tPoint3D our_point;
	static AKtEnvironment *env;

	if (ioChoice == NULL) {
		// perform setup for a selection sequence
		current_score = 1e09;
		previous_id = inFoundID;
		is_dead = P3rGetRealPosition(inClass, inParticle, &our_point);
		if (is_dead) {
			return UUcFalse;
		}
		env = ONrGameState_GetEnvironment();
		return UUcTrue;
	}

	// score is purely based on distance - lower scores are better!
	score = inDistance;
	if (inFoundID == previous_id) {
		// stick with our previous choice unless the new one is significantly better
		score *= P3cSelector_PreviousScoreWeight;

	} else if (inClass->definition->flags2 & P3cParticleClassFlag2_Attractor_CheckEnvironment) {
		AKtOctNodeIndexCache *p_envcache = P3rGetEnvironmentCache(inClass, inParticle);
		UUtUns32 num_collisions;

		MUmVector_Subtract(delta_pos, *inFoundPoint, our_point);

		if (p_envcache == NULL) {
			num_collisions = AKrCollision_Point(env, &our_point, &delta_pos, AKcGQ_Flag_Obj_Col_Skip, 0);
		} else {
			num_collisions = AKrCollision_Point_SpatialCache(env, &our_point, &delta_pos, AKcGQ_Flag_Obj_Col_Skip,
															 0, p_envcache, NULL);
		}

		if (num_collisions > 0) {
			// there is a wall blocking us, don't consider this candidate
			return UUcFalse;
		}
	}

	if (score > current_score) {
		// reject this choice, it is further away
		return UUcFalse;
	}

	// accept this choice and keep looking
	current_score = score;
	*ioChoice = inFoundID;
	return UUcFalse;
}

UUtBool P3iAttractorSelector_Conical(P3tParticleClass *inClass, P3tParticle *inParticle, P3tAttractor *inAttractor,
												M3tPoint3D *inFoundPoint, float inDistance, UUtUns32 inFoundID, UUtUns32 *ioChoice)
{
	UUtBool returnval;
	float score, theta;
	M3tVector3D delta_pos, dir_to_target;
	M3tMatrix4x3 **p_dynmatrix;

	// static selection-sequence state variables
	static float current_score, select_thetamin, select_thetamax, select_thetaweight, max_angle;
	static UUtUns32 previous_id;
	static M3tVector3D our_orientation;
	static M3tPoint3D our_point;
	static AKtEnvironment *env;

	if (ioChoice == NULL) {
		UUtBool is_dead;
		M3tMatrix3x3 *p_orientation;
		M3tVector3D *p_velocity;

		// perform setup for a selection sequence
		current_score = 1e09;
		previous_id = inFoundID;
		is_dead = P3rGetRealPosition(inClass, inParticle, &our_point);
		if (is_dead) {
			return UUcFalse;
		}

		p_orientation = P3rGetOrientationPtr(inClass, inParticle);
		if (p_orientation == NULL) {
			p_velocity = P3rGetVelocityPtr(inClass, inParticle);
			if (p_velocity == NULL) {
				return UUcFalse;
			} else {
				MUmVector_Copy(our_orientation, *p_velocity);
			}
		} else {
			our_orientation = MUrMatrix_GetYAxis((M3tMatrix4x3 *) p_orientation);
		}
		p_dynmatrix = P3rGetDynamicMatrixPtr(inClass, inParticle);
		if (p_dynmatrix != NULL) {
			MUrMatrix3x3_MultiplyPoint(&our_orientation, (M3tMatrix3x3 *) *p_dynmatrix, &our_orientation);
		}
		MUmVector_Normalize(our_orientation);

		P3mAssertFloatValue(inAttractor->select_thetamin);
		P3mAssertFloatValue(inAttractor->select_thetamax);
		P3mAssertFloatValue(inAttractor->select_thetaweight);
		P3mAssertFloatValue(inAttractor->max_angle);
		returnval = P3iCalculateValue(inParticle, &inAttractor->select_thetamin, (void *) &select_thetamin);
		if (!returnval)
			return UUcFalse;
		returnval = P3iCalculateValue(inParticle, &inAttractor->select_thetamax, (void *) &select_thetamax);
		if (!returnval)
			return UUcFalse;
		returnval = P3iCalculateValue(inParticle, &inAttractor->select_thetaweight, (void *) &select_thetaweight);
		if (!returnval)
			return UUcFalse;
		returnval = P3iCalculateValue(inParticle, &inAttractor->max_angle, (void *) &max_angle);
		if (!returnval)
			return UUcFalse;

		select_thetamin *= M3cDegToRad;
		select_thetamax *= M3cDegToRad;
		max_angle *= M3cDegToRad;
		env = ONrGameState_GetEnvironment();

		return UUcTrue;
	}

	// score is based on distance - lower scores are better!
	score = inDistance;

	// weight score by angle
	MUmVector_Subtract(delta_pos, *inFoundPoint, our_point);
	MUmVector_Copy(dir_to_target, delta_pos);
	MUmVector_Normalize(dir_to_target);
	theta = MUrAngleBetweenVectors3D(&our_orientation, &dir_to_target);
	if (theta < select_thetamin) {
		// don't increase score
	} else if (theta < select_thetamax) {
		// increase score by the angle away that we are from the target
		score *= 1.0f + (select_thetaweight - 1.0f) * (theta - select_thetamin) / (select_thetamax - select_thetamin);
	} else if (theta < max_angle) {
		// increase score by a fixed maximum amount
		score *= select_thetaweight;
	} else {
		// too far away! don't consider this choice
		return UUcFalse;
	}

	if (inFoundID == previous_id) {
		// stick with our previous choice unless the new one is significantly better
		score *= P3cSelector_PreviousScoreWeight;

	} else if (inClass->definition->flags2 & P3cParticleClassFlag2_Attractor_CheckEnvironment) {
		AKtOctNodeIndexCache *p_envcache = P3rGetEnvironmentCache(inClass, inParticle);
		UUtUns32 num_collisions;

		if (p_envcache == NULL) {
			num_collisions = AKrCollision_Point(env, &our_point, &delta_pos, AKcGQ_Flag_Obj_Col_Skip, 0);
		} else {
			num_collisions = AKrCollision_Point_SpatialCache(env, &our_point, &delta_pos, AKcGQ_Flag_Obj_Col_Skip,
															 0, p_envcache, NULL);
		}

		if (num_collisions > 0) {
			// there is a wall blocking us, don't consider this candidate
			return UUcFalse;
		}
	}

	if (score > current_score) {
		// reject this choice, it is further away
		return UUcFalse;
	}

	// accept this choice and keep looking
	current_score = score;
	*ioChoice = inFoundID;
	return UUcFalse;
}

/*
 * particle action procedures
 */

// animate a variable' value linearly over time
//
// var 0:	float: variable to animate
// param 0: float: rate to animate at
UUtBool P3iAction_AnimateLinear(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	float *param = (float *) inVariables[0];
	float rate;
	UUtBool returnval;

	if (param == NULL)
		return UUcFalse;		// can't modify a NULL reference

	P3mAssertFloatValue(inAction->action_value[0]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &rate);
	if (!returnval)
		return UUcFalse;	// if a NULL value then bail, don't change anything

	*param += inDeltaT * rate;

	return UUcFalse;
}

// animate a variable' value in an accelerating fashion over time
//
// var 0:	float: variable to animate
// var 1:	float: variable to animate as rate of change
// param 0: float: rate to accelerate at
UUtBool P3iAction_AnimateAccel(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	float *param = (float *) inVariables[0];
	float *velocity = (float *) inVariables[1];
	float rate;
	UUtBool returnval;

	if ((param == NULL) || (velocity == NULL))
		return UUcFalse;		// can't modify a NULL reference

	P3mAssertFloatValue(inAction->action_value[0]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &rate);
	if (!returnval)
		return UUcFalse;	// if a NULL value then bail, don't change anything

	*velocity += inDeltaT * rate;
	*param += inDeltaT * *velocity;

	return UUcFalse;
}

// animate a variable' value by doing a biased random walk
//
// var 0:	float: variable to animate
// param 0: float: min value
// param 1: float: max value
// param 2: float: rate to vary at
UUtBool P3iAction_AnimateRandom(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	float *param = (float *) inVariables[0];
	float min, max, rate, bias, delta_rate;
	UUtBool returnval;

	if (param == NULL)
		return UUcFalse;		// can't modify a NULL reference

	P3mAssertFloatValue(inAction->action_value[0]);
	P3mAssertFloatValue(inAction->action_value[1]);
	P3mAssertFloatValue(inAction->action_value[2]);

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &min);
	if (!returnval)
		return UUcFalse;

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &max);
	if (!returnval)
		return UUcFalse;

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[2], (void *) &rate);
	if (!returnval)
		return UUcFalse;

	bias = (*param - min) / (max - min);
	if (bias < 0)
		bias = 0;
	else if (bias > 1)
		bias = 1;

	delta_rate = P3rRandom();
	if (bias < 0.5)
		delta_rate = rate * (+1 - 4 * bias * delta_rate);
	else
		delta_rate = rate * (-1 + 4 * (1 - bias) * delta_rate);

	*param += inDeltaT * delta_rate;

	return UUcFalse;
}

// animate a variable' value to pingpong back and forth
//
// var 0:	float:			variable to animate
// var 1:	enum pingpong:	whether we are going up or down
// param 0: float:			min value
// param 1: float:			max value
// param 2: float:			rate to vary at
UUtBool P3iAction_AnimatePingpong(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	float *param = (float *) inVariables[0];
	float min, max, rate;
	UUtUns32 *pp_dir = (UUtUns32 *) inVariables[1];
	UUtBool returnval;

	if ((param == NULL) || (pp_dir == NULL))
		return UUcFalse;		// can't modify a NULL reference

	P3mAssertFloatValue(inAction->action_value[0]);
	P3mAssertFloatValue(inAction->action_value[1]);
	P3mAssertFloatValue(inAction->action_value[2]);

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &min);
	if (!returnval)
		return UUcFalse;

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &max);
	if (!returnval)
		return UUcFalse;

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[2], (void *) &rate);
	if (!returnval)
		return UUcFalse;

	UUmAssertReadPtr(pp_dir, sizeof(UUtUns32));
	UUmAssertWritePtr(param, sizeof(float));
	COmAssert(max > min);
	if (*pp_dir == P3cEnumPingpong_Up) {
		*param += inDeltaT * rate;
		if (*param > max) {
			*param = max;
			*pp_dir = P3cEnumPingpong_Down;
		}
	} else {
		UUmAssert(*pp_dir == P3cEnumPingpong_Down);
		*param -= inDeltaT * rate;
		if (*param < min) {
			*param = min;
			*pp_dir = P3cEnumPingpong_Up;
		}
	}

	return UUcFalse;
}

// kill a particle
UUtBool P3iAction_Die(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	P3rKillParticle(inClass, inParticle);
	return UUcTrue;
}

// tell a particle to fade out over time
//
// param 0: float:			fade time
UUtBool P3iAction_FadeOut(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	float fade_time, current_alpha;
	UUtBool returnval;

	// can't fade an already fading particle
	if (inParticle->header.flags & P3cParticleFlag_FadeOut)
		return UUcFalse;

	// get the fade time
	P3mAssertFloatValue(inAction->action_value[0]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &fade_time);
	if (!returnval)
		return UUcFalse;

	// calculate the particle's current alpha
	P3mAssertFloatValue(inClass->definition->appearance.alpha);
	returnval = P3iCalculateValue(inParticle, &inClass->definition->appearance.alpha, (void *) &current_alpha);
	if (!returnval)
		return UUcFalse;

	// if we're already faded out, die right here and now
	if (current_alpha <= 0) {
		P3rKillParticle(inClass, inParticle);
		return UUcTrue;
	}

	// set the time remaining in the fade
	inParticle->header.lifetime = fade_time;

	// calculate what the fade time would have been from alpha of 1. this is done so that
	// the particle starts from its current alpha.
	inParticle->header.fadeout_time = fade_time / current_alpha;

	// set the fadeout flag
	inParticle->header.flags |= P3cParticleFlag_FadeOut;

	return UUcFalse;
}

// animate a variable' value to loop around a range
//
// var 0:	float:			variable to animate
// param 0: float:			min value
// param 1: float:			max value
// param 2: float:			rate to vary at
UUtBool P3iAction_AnimateLoop(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	float *param = (float *) inVariables[0];
	float min, max, rate;
	UUtBool returnval;

	if (param == NULL)
		return UUcFalse;		// can't modify a NULL reference

	P3mAssertFloatValue(inAction->action_value[0]);
	P3mAssertFloatValue(inAction->action_value[1]);
	P3mAssertFloatValue(inAction->action_value[2]);

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &min);
	if (!returnval)
		return UUcFalse;

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &max);
	if (!returnval)
		return UUcFalse;

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[2], (void *) &rate);
	if (!returnval)
		return UUcFalse;

	UUmAssertWritePtr(param, sizeof(float));
	COmAssert(max > min);

	*param += inDeltaT * rate;

	if (rate > 0) {
		if (*param > max) {
			*param = min;
		}
	} else if (rate < 0) {
		if (*param < min) {
			*param = max;
		}
	}

	return UUcFalse;
}

// animate a variable' value to an endpoint
//
// var 0:	float:			variable to animate
// param 0: float:			rate to vary at
// param 1: float:			endpoint
UUtBool P3iAction_AnimateToValue(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	float *param = (float *) inVariables[0];
	float rate, endpoint;
	UUtBool returnval;

	if (param == NULL)
		return UUcFalse;		// can't modify a NULL reference

	P3mAssertFloatValue(inAction->action_value[0]);
	P3mAssertFloatValue(inAction->action_value[1]);

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &rate);
	if (!returnval)
		return UUcFalse;

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &endpoint);
	if (!returnval)
		return UUcFalse;

	UUmAssertWritePtr(param, sizeof(float));

	*param += inDeltaT * rate;

	if (rate > 0) {
		if (*param > endpoint) {
			*param = endpoint;
		}
	} else if (rate < 0) {
		if (*param < endpoint) {
			*param = endpoint;
		}
	}

	return UUcFalse;
}

// interpolate between two colors
//
// var 0:	shade:			color to animate
// param 0: shade:			color 0
// param 1: shade:			color 1
// param 2: float:			interpolation value
UUtBool P3iAction_ColorInterpolate(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	IMtShade *output = (IMtShade *) inVariables[0];
	float interp_val;
	IMtShade color_0, color_1;
	UUtBool returnval;

	if (output == NULL)
		return UUcFalse;		// can't modify a NULL reference

	P3mAssertShadeValue(inAction->action_value[0]);
	P3mAssertShadeValue(inAction->action_value[1]);
	P3mAssertFloatValue(inAction->action_value[2]);

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &color_0);
	if (!returnval)
		return UUcFalse;

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &color_1);
	if (!returnval)
		return UUcFalse;

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[2], (void *) &interp_val);
	if (!returnval)
		return UUcFalse;

	UUmAssertWritePtr(output, sizeof(IMtShade));

	*output = IMrShade_Interpolate(color_0, color_1, interp_val);

	return UUcFalse;
}

// move a particle in a straight line
UUtBool P3iAction_MoveLine(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	const UUtUns32 required_flags = (P3cParticleClassFlag_Physics_CollideEnv | P3cParticleClassFlag_Physics_CollideChar);
	const UUtUns32 excluded_flags = (P3cParticleClassFlag_HasOffsetPos | P3cParticleClassFlag_HasDynamicMatrix | P3cParticleClassFlag_HasPrevPosition);
	M3tVector3D *velocity;
	UUtUns32 flags = inClass->definition->flags;
	UUtBool use_precalculated_point = UUcTrue;

	use_precalculated_point = use_precalculated_point && P3gCurrentStep_EndPointValid;
	use_precalculated_point = use_precalculated_point && ((flags & required_flags) > 0) && ((flags & excluded_flags) == 0);
	use_precalculated_point = use_precalculated_point && !P3gCurrentCollision.collided;

	if (use_precalculated_point) {
#if defined(DEBUGGING) && DEBUGGING
		{
			float dist = MUrPoint_Distance(&inParticle->header.position, &P3gCurrentStep_EndPoint);
			velocity = P3rGetVelocityPtr(inClass, inParticle);

			if ((dist > 50.0f) && ((velocity == NULL) || (inDeltaT * MUmVector_GetLength(*velocity) < 25.0f))) {
				UUrEnterDebugger("teleporting movement");
			}
		}
#endif
		// we are running simple collision, use the endpoint determined by our collision timestep as our new point instead of separately
		// calculating delta-v... this prevents collision paths from becoming disjoint
		MUmVector_Copy(inParticle->header.position, P3gCurrentStep_EndPoint);
	} else {
		// calculate the change in our position based on our velocity
		velocity = P3rGetVelocityPtr(inClass, inParticle);
		if (velocity == NULL)
			return UUcFalse;

		// NB: this does **NOT** use P3rGetPositionPtr because we don't want to change position
		// through links
		MUmVector_ScaleIncrement(inParticle->header.position, inDeltaT, *velocity);
	}

	inParticle->header.flags |= P3cParticleFlag_PositionChanged;

	return UUcFalse;
}

// move a particle - apply the force of gravity (worldspace down)
//
// param 0: float:			fraction of gravity to apply
UUtBool P3iAction_MoveGravity(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	M3tVector3D *velocity;
	UUtUns16 vel_offset;
	float gravity_fraction;
	UUtBool returnval;

	vel_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_Velocity];
	if (vel_offset == P3cNoVar)
		return UUcFalse;

	P3mAssertFloatValue(inAction->action_value[0]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &gravity_fraction);
	if (!returnval)
		return UUcFalse;

	velocity = (M3tVector3D *) &inParticle->vararray[vel_offset];
	UUmAssertReadPtr(velocity, sizeof(M3tVector3D));

	// NB: gravity doesn't work as you would expect on a particle that is
	// attached to an object. it pulls 'down' in the space of the attachment.
	velocity->y -= P3gGravityAccel * gravity_fraction * inDeltaT;

	inParticle->header.flags |= P3cParticleFlag_VelocityChanged;

	return UUcFalse;
}

// move a particle - apply viscous resistance
//
// param 0: float:			resistance factor
// param 1: float:			minimum velocity to apply resistance over
UUtBool P3iAction_MoveResistance(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	M3tVector3D *velocity;
	UUtUns16 vel_offset;
	float resistance, minimum_vel, vel_mag, delta_v;
	UUtBool returnval;

	vel_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_Velocity];
	if (vel_offset == P3cNoVar)
		return UUcFalse;

	P3mAssertFloatValue(inAction->action_value[0]);
	P3mAssertFloatValue(inAction->action_value[1]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &resistance);
	if (!returnval)
		return UUcFalse;

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &minimum_vel);
	if (!returnval)
		return UUcFalse;

	velocity = (M3tVector3D *) &inParticle->vararray[vel_offset];
	UUmAssertReadPtr(velocity, sizeof(M3tVector3D));

	vel_mag = MUmVector_GetLength(*velocity);
	if (vel_mag > minimum_vel) {
		// instead of using finite-timestep euler integration like other
		// dynamic code, we implicitly calculate from the exact solution to
		// the differential equation. this prevents us flip-flopping around zero.
		delta_v = minimum_vel + (vel_mag - minimum_vel) * (float) exp(- resistance * inDeltaT);
		MUmVector_Scale(*velocity, delta_v / vel_mag);

		inParticle->header.flags |= P3cParticleFlag_VelocityChanged;
	}

	return UUcFalse;
}

// drift a particle - accelerate in a direction up to a max speed
//
// param 0: float:			resistance factor
// param 1: float:			minimum velocity to apply resistance over
UUtBool P3iAction_MoveDrift(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	M3tVector3D *velocity, direction, v_tangential;
	UUtUns16 vel_offset;
	float acceleration, max_speed, sideways_decay, v_axial, v_difference, decayterm;
	M3tMatrix3x3 *orientation;
	P3tEnumCoordFrame space;
	UUtBool returnval;

	// particle must have a velocity
	vel_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_Velocity];
	if (vel_offset == P3cNoVar)
		return UUcFalse;

	// read the parameters
	P3mAssertFloatValue(inAction->action_value[0]);
	P3mAssertFloatValue(inAction->action_value[1]);
	P3mAssertFloatValue(inAction->action_value[2]);
	P3mAssertFloatValue(inAction->action_value[3]);
	P3mAssertFloatValue(inAction->action_value[4]);
	P3mAssertFloatValue(inAction->action_value[5]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &acceleration);
	if (!returnval)
		return UUcFalse;

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &max_speed);
	if (!returnval)
		return UUcFalse;

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[2], (void *) &sideways_decay);
	if (!returnval)
		return UUcFalse;

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[3], (void *) &direction.x);
	if (!returnval)
		return UUcFalse;

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[4], (void *) &direction.y);
	if (!returnval)
		return UUcFalse;

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[5], (void *) &direction.z);
	if (!returnval)
		return UUcFalse;

	P3mAssertEnumValue(inAction->action_value[6], P3cEnumType_CoordFrame);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[6], (void *) &space);
	if (!returnval)
		return UUcFalse;

	// work out what the normalized direction is, in worldspace
	if (space == P3cEnumCoordFrame_Local) {
		orientation = P3rGetOrientationPtr(inClass, inParticle);
		if (orientation != NULL) {
			MUrMatrix3x3_MultiplyPoint(&direction, orientation, &direction);
		}
	}

	// if the direction hasn't been set yet, return
	if ((direction.x == 0) && (direction.y == 0) && (direction.z == 0))
		return UUcFalse;

	MUmVector_Normalize(direction);

	// get the particle's velocity
	velocity = (M3tVector3D *) &inParticle->vararray[vel_offset];
	UUmAssertReadPtr(velocity, sizeof(M3tVector3D));

	// break the velocity down into axial and tangential
	v_axial = MUrVector_DotProduct(&direction, velocity);
	v_tangential = direction;
	MUmVector_Scale(v_tangential, -v_axial);
	MUmVector_Add(v_tangential, v_tangential, *velocity);

	if (max_speed == 0) {
		// accelerate continuously
		v_axial += acceleration * inDeltaT;
	} else {
		// accelerate proportional to the difference between max_speed and v_axial
		// this turns out to be an exponential curve of the form
		//
		// v = max_speed * (1 - exp(- acceleration * t))
		v_difference = v_axial - max_speed;
		v_difference *= (float) exp(- acceleration * inDeltaT);
		v_axial = v_difference + max_speed;
	}

	if (sideways_decay != 0) {
		// decay the tangential motion
		decayterm = (float) exp(- sideways_decay * inDeltaT);
		MUmVector_Scale(v_tangential, decayterm);
	}

	// re-composite the velocity
	*velocity = direction;
	MUmVector_Scale(*velocity, v_axial);
	MUmVector_Add(*velocity, *velocity, v_tangential);

	inParticle->header.flags |= P3cParticleFlag_VelocityChanged;

	return UUcFalse;
}

// set a particle's velocity
//
// param 0: float:			speed
// param 1: coordframe:		world: sets along current velocity. local: sets along orientation
// param 2: bool:			kill sideways velocity?
UUtBool P3iAction_SetVelocity(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	M3tVector3D *velocity, direction, v_tangential;
	UUtUns16 vel_offset;
	float speed, v_axial;
	P3tEnumBool kill_sideways;
	P3tEnumCoordFrame space;
	M3tMatrix3x3 *orientation;
	UUtBool returnval;

	// particle must have a velocity
	vel_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_Velocity];
	if (vel_offset == P3cNoVar)
		return UUcFalse;

	// read the parameters
	P3mAssertFloatValue(inAction->action_value[0]);
	P3mAssertEnumValue(inAction->action_value[1], P3cEnumType_CoordFrame);
	P3mAssertEnumValue(inAction->action_value[2], P3cEnumType_Bool);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &speed);
	if (!returnval)
		return UUcFalse;

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &space);
	if (!returnval)
		return UUcFalse;

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[2], (void *) &kill_sideways);
	if (!returnval)
		return UUcFalse;

	// get the particle's velocity
	velocity = (M3tVector3D *) &inParticle->vararray[vel_offset];
	UUmAssertReadPtr(velocity, sizeof(M3tVector3D));

	orientation = P3rGetOrientationPtr(inClass, inParticle);
	if (orientation == NULL) {
		space = P3cEnumCoordFrame_World;
	}

	if (space == P3cEnumCoordFrame_Local) {
		// direction along orientation y
		MUrMatrix_GetCol((M3tMatrix4x3 *) orientation, 1, &direction);

		// break the velocity down into axial and tangential
		v_axial = MUrVector_DotProduct(&direction, velocity);
		v_tangential = direction;
		MUmVector_Scale(v_tangential, -v_axial);
		MUmVector_Add(v_tangential, v_tangential, *velocity);

		// set the velocity to be 'speed' axially
		MUmVector_ScaleCopy(*velocity, speed, direction);
		if (!kill_sideways) {
			// add tangential speed back in
			MUmVector_Add(*velocity, *velocity, v_tangential);
		}
	} else {
		MUrVector_SetLength(velocity, speed);
	}

	inParticle->header.flags |= P3cParticleFlag_VelocityChanged;

	return UUcFalse;
}

// move a particle - spiral offset-pos about its direction
//
// var 0:	float:			variable to use for theta
// param 0: float:			radius of rotation
// param 1: float:			angular velocity
UUtBool P3iAction_MoveSpiral(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	M3tVector3D *velocity, *offsetpos, horiz_axis, vert_axis;
	M3tMatrix3x3 *orientation;
	UUtUns16 vel_offset, ori_offset, offsetpos_offset;
	float radius, rotation_rate, costheta, sintheta, *theta = (float *) inVariables[0];
	UUtBool returnval;

	if (theta == NULL)
		return UUcFalse;

	offsetpos_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_OffsetPos];
	if (offsetpos_offset == P3cNoVar)
		return UUcFalse;

	vel_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_Velocity];
	ori_offset = inClass->optionalvar_offset[P3cParticleOptionalVar_Orientation];
	if ((vel_offset == P3cNoVar) && (ori_offset == P3cNoVar))
		return UUcFalse;

	P3mAssertFloatValue(inAction->action_value[0]);
	P3mAssertFloatValue(inAction->action_value[1]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &radius);
	if (!returnval)
		return UUcFalse;

	offsetpos = (M3tVector3D *) &inParticle->vararray[offsetpos_offset];
	UUmAssertReadPtr(offsetpos, sizeof(M3tVector3D));

	if (fabs(radius) < 0.01) {
		// radius of zero means "use current radius"
		radius = MUmVector_GetLength(*offsetpos);
	}

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &rotation_rate);
	if (!returnval)
		return UUcFalse;

	// get theta - between +180 and -180
	*theta += inDeltaT * rotation_rate;
	while (*theta < 0.0f)
		*theta += 360.0f;

	while (*theta > 360.0f)
		*theta -= 360.0f;

	costheta = MUrCos(*theta * M3cPi / 180.0f);
	sintheta = MUrSqrt(1.0f - UUmSQR(costheta));
	if (*theta < 180.0f)
		sintheta = -sintheta;

	// get the offset position by constructing perpendicular axes to the particle
	if (ori_offset == P3cNoVar) {
		UUmAssert(vel_offset != P3cNoVar);

		velocity = (M3tVector3D *) &inParticle->vararray[vel_offset];
		UUmAssertReadPtr(velocity, sizeof(M3tVector3D));

		// this is velocity cross upvector (0 1 0)
		MUmVector_Set(horiz_axis, -velocity->z, 0, velocity->x);
		MUmVector_Normalize(horiz_axis);

		MUrVector_CrossProduct(&horiz_axis, velocity, &vert_axis);
		MUmVector_Normalize(vert_axis);

		// set up offset position
		offsetpos->x = radius * (costheta * horiz_axis.x - sintheta * vert_axis.x);
		offsetpos->y = radius * (costheta * horiz_axis.y - sintheta * vert_axis.y);
		offsetpos->z = radius * (costheta * horiz_axis.z - sintheta * vert_axis.z);
	} else {
		orientation = (M3tMatrix3x3 *) &inParticle->vararray[ori_offset];
		UUmAssertReadPtr(orientation, sizeof(M3tMatrix3x3));
		
		// set up offset position directly from orientation's X and Z axes
		offsetpos->x = radius * (costheta * orientation->m[0][0] - sintheta * orientation->m[2][0]);
		offsetpos->y = radius * (costheta * orientation->m[0][1] - sintheta * orientation->m[2][1]);
		offsetpos->z = radius * (costheta * orientation->m[0][2] - sintheta * orientation->m[2][2]);
	}

	inParticle->header.flags |= P3cParticleFlag_PositionChanged;

	return UUcFalse;
}

// activate a particle's emitter
//
// param 0: enum:			index of emitter
UUtBool P3iAction_EmitActivate(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	UUtUns32 emitter_index;
	P3tParticleEmitterData *emitterdata;
	P3mAssertEnumValue(inAction->action_value[0], P3cEnumType_Emitter);

	emitter_index = inAction->action_value[0].u.enum_const.val;

	if ((emitter_index < 0) || (emitter_index >= inClass->definition->num_emitters))
		return UUcFalse;

	emitterdata = ((P3tParticleEmitterData *) inParticle->vararray) + emitter_index;

	// activate the emitter
	emitterdata->num_emitted = 0;
	emitterdata->time_last_emission = P3cEmitImmediately;
	emitterdata->last_particle = P3cParticleReference_Null;

	return UUcFalse;
}

// deactivate a particle's emitter
//
// param 0: enum:			index of emitter
UUtBool P3iAction_EmitDeactivate(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	UUtUns32 emitter_index;
	P3tParticleEmitterData *emitterdata;
	P3mAssertEnumValue(inAction->action_value[0], P3cEnumType_Emitter);

	emitter_index = inAction->action_value[0].u.enum_const.val;

	if ((emitter_index < 0) || (emitter_index >= inClass->definition->num_emitters))
		return UUcFalse;

	emitterdata = ((P3tParticleEmitterData *) inParticle->vararray) + emitter_index;

	// deactivate the emitter
	emitterdata->num_emitted = P3cEmitterDisabled;

	return UUcFalse;
}

// pulse a particle's emitter
//
// param 0: enum:			index of emitter
// param 1: float:			number of particles
UUtBool P3iAction_EmitParticles(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	UUtUns16 itr;
	UUtUns32 emitter_index, emit_count;
	UUtBool is_dead;
	float emit_val;
	P3tEmitter *emitter;
	P3tParticleEmitterData *emitter_data;

	P3mAssertEnumValue(inAction->action_value[0], P3cEnumType_Emitter);
	emitter_index = inAction->action_value[0].u.enum_const.val;

	if ((emitter_index < 0) || (emitter_index >= inClass->definition->num_emitters))
		return UUcFalse;

	emitter = &inClass->definition->emitter[emitter_index];
	emitter_data = ((P3tParticleEmitterData *) inParticle->vararray) + emitter_index;

	P3mAssertFloatValue(inAction->action_value[1]);
	emit_count = (UUtUns32) inAction->action_value[1].u.float_const.val;

	// get the fractional part of the emitter value - check for random chance
	// if it lies between 0.001 and 0.999
	emit_val = inAction->action_value[0].u.float_const.val - emit_count;
	if (emit_val > 0.999) {
		// always
		emit_count++;
	} else if (emit_val > 0.001) {
		if (P3rRandom() < emit_val) {
			emit_count++;
		}
	}

	if (emit_count == 0)
		return UUcFalse;

	// emit this number of particles
	for (itr = 0; itr < emit_count; itr++) {
		is_dead = P3iEmitParticle(inClass, inParticle, emitter, (UUtUns16) emitter_index, inCurrentT);
		if (is_dead) {
			break;
		}
	}

	// increment the number of particles that have been emitted
	if ((emitter_data->num_emitted != P3cEmitterDisabled) && (emitter->flags & P3cEmitterFlag_IncreaseEmitCount)) {
		emitter_data->num_emitted += (UUtUns16) emit_count;

		if (emitter->flags & P3cEmitterFlag_DeactivateWhenEmitCountThreshold) {
			if (emitter_data->num_emitted >= emitter->particle_limit) {
				emitter_data->num_emitted = P3cEmitterDisabled;
			}
		} else {
			// check that we don't just keep incrementing all the way up to
			// the sentinel value
			if (emitter_data->num_emitted == P3cEmitterDisabled) {
				emitter_data->num_emitted = 0;
			}
		}
	}
	emitter_data->time_last_emission = inCurrentT;

	return UUcFalse;
}

// change into a particle of a different class
//
// param 0: enum:			index of emitter
UUtBool P3iAction_ChangeClass(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	P3tParticleClass *new_class;
	P3tParticle *new_particle;
	UUtUns32 *p_lensframes, emitter_index, current_tick;
	P3tOwner *p_owner, *our_owner;
	M3tMatrix4x3 **p_matrix, **our_matrix;
	M3tPoint3D *p_position, *our_position, *p_offset, *our_offset;
	M3tVector3D *p_velocity, *our_velocity;
	M3tMatrix3x3 *p_orientation, *our_orientation;
	P3tPrevPosition *p_prev_position;
	AKtOctNodeIndexCache *p_envcache;
	UUtUns32 *p_texture, *our_texture;
	UUtBool is_dead;

	P3mAssertEnumValue(inAction->action_value[0], P3cEnumType_Emitter);
	emitter_index = inAction->action_value[0].u.enum_const.val;

	if ((emitter_index < 0) || (emitter_index >= inClass->definition->num_emitters))
		return UUcFalse;

	new_class = inClass->definition->emitter[emitter_index].emittedclass;
	if (new_class == NULL)
		return UUcFalse;

	// create a particle of this new class
	current_tick = MUrUnsignedSmallFloat_To_Uns_Round(inCurrentT * UUcFramesPerSecond);
	new_particle = P3rCreateParticle(new_class, current_tick);
	if (new_particle == NULL)
		return UUcFalse;

	// the new particle has its own lifetime, it can't be dying already
	new_particle->header.flags = inParticle->header.flags & ~P3cParticleFlag_FadeOut;

	// keep the same link as before
	new_particle->header.user_ref = inParticle->header.user_ref;

	p_owner = P3rGetOwnerPtr(new_class, new_particle);
	if (p_owner != NULL) {
		our_owner = P3rGetOwnerPtr(inClass, inParticle);
		if (our_owner == NULL) {
			*p_owner = 0;
		} else {
			*p_owner = *our_owner;
		}
	}

	p_matrix = P3rGetDynamicMatrixPtr(new_class, new_particle);
	if (p_matrix != NULL) {
		our_matrix = P3rGetDynamicMatrixPtr(inClass, inParticle);		
		if (our_matrix == NULL) {
			*p_matrix = NULL;
		} else {
			*p_matrix = *our_matrix;
			if (inParticle->header.flags & P3cParticleFlag_AlwaysUpdatePosition) {
				new_particle->header.flags |= P3cParticleFlag_AlwaysUpdatePosition;
			}
		}
	}

	p_position = P3rGetPositionPtr(new_class, new_particle, &is_dead);
	if (is_dead) {
		return UUcFalse;
	}

	p_offset = P3rGetOffsetPosPtr(new_class, new_particle);
	our_position = P3rGetPositionPtr(inClass, inParticle, &is_dead);
	if (is_dead) {
		return UUcTrue;
	}

	our_offset = P3rGetOffsetPosPtr(inClass, inParticle);
	if (p_offset == NULL) {
		if (our_offset == NULL) {
			*p_position = *our_position;
		} else {
			MUmVector_Add(*p_position, *our_position, *our_offset);
		}
	} else {
		*p_position = *our_position;
		if (our_offset == NULL) {
			MUmVector_Set(*p_offset, 0, 0, 0);
		} else {
			*p_offset = *our_offset;
		}
	}

	p_velocity = P3rGetVelocityPtr(new_class, new_particle);
	if (p_velocity != NULL) {
		our_velocity = P3rGetVelocityPtr(inClass, inParticle);
		if (our_velocity == NULL) {
			MUmVector_Set(*p_velocity, 0, 0, 0);
		} else {
			*p_velocity = *our_velocity;
		}
	}

	p_orientation = P3rGetOrientationPtr(new_class, new_particle);
	if (p_orientation != NULL) {
		our_orientation = P3rGetOrientationPtr(inClass, inParticle);
		if (our_orientation == NULL) {
			if (p_velocity == NULL) {
				MUrMatrix3x3_Identity(p_orientation);
			} else {
				// build an orientation along our velocity with upvector -> world Y
				P3rConstructOrientation(P3cEmitterOrientation_Velocity, P3cEmitterOrientation_WorldPosY,
										NULL, NULL, p_velocity, p_orientation);
			}
		} else {
			*p_orientation = *our_orientation;
		}
	}

	// get unique texture index
	p_texture = P3rGetTextureIndexPtr(new_class, new_particle);
	if (p_texture != NULL) {
		our_texture = P3rGetTextureIndexPtr(inClass, inParticle);
		if (our_texture == NULL) {
			*p_texture = (UUtUns32) UUrLocalRandom();
		} else {
			*p_texture = *our_texture;
		}
	}

	// get unique texture index
	p_texture = P3rGetTextureTimePtr(new_class, new_particle);
	if (p_texture != NULL) {
		our_texture = P3rGetTextureTimePtr(inClass, inParticle);
		if (our_texture == NULL) {
			*p_texture = current_tick;
		} else {
			*p_texture = *our_texture;
		}
	}

	// no previous position
	p_prev_position = P3rGetPrevPositionPtr(new_class, new_particle);
	if (p_prev_position != NULL) {
		p_prev_position->time = 0;
	}

	// lensflares are not initially visible
	p_lensframes = P3rGetLensFramesPtr(new_class, new_particle);
	if (p_lensframes != NULL) {
		*p_lensframes = 0;
	}

	// set up the environment cache
	p_envcache = P3rGetEnvironmentCache(new_class, new_particle);
	if (p_envcache != NULL) {
		AKrCollision_InitializeSpatialCache(p_envcache);
	}

	P3rSendEvent(new_class, new_particle, P3cEvent_Create, inCurrentT);

	return UUcFalse;
}

// kill the last particle emitted from an emitter
//
// param 0: enum:			index of emitter
UUtBool P3iAction_KillLastEmitted(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	P3tParticleClass *kill_class;
	P3tParticle *kill_particle;
	P3tParticleEmitterData *emitter_data;
	UUtUns32 ref;
	UUtUns32 emitter_index;

	P3mAssertEnumValue(inAction->action_value[0], P3cEnumType_Emitter);
	emitter_index = inAction->action_value[0].u.enum_const.val;

	if ((emitter_index < 0) || (emitter_index >= inClass->definition->num_emitters))
		return UUcFalse;

	emitter_data = ((P3tParticleEmitterData *) inParticle->vararray) + emitter_index;
	ref = emitter_data->last_particle;
	if (ref == P3cParticleReference_Null)
		return UUcFalse;

	P3mUnpackParticleReference(ref, kill_class, kill_particle);
	UUmAssert(kill_class == inClass->definition->emitter[emitter_index].emittedclass);
	if (kill_particle->header.self_ref != ref) {
		// the particle is already dead
		return UUcFalse;
	}

	P3rKillParticle(kill_class, kill_particle);
	return UUcFalse;
}

// explode the last particle emitted from an emitter
//
// param 0: enum:			index of emitter
UUtBool P3iAction_ExplodeLastEmitted(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	P3tParticleClass *explode_class;
	P3tParticle *explode_particle;
	P3tParticleEmitterData *emitter_data;
	UUtUns32 ref;
	UUtUns32 emitter_index;

	P3mAssertEnumValue(inAction->action_value[0], P3cEnumType_Emitter);
	emitter_index = inAction->action_value[0].u.enum_const.val;

	if ((emitter_index < 0) || (emitter_index >= inClass->definition->num_emitters))
		return UUcFalse;

	emitter_data = ((P3tParticleEmitterData *) inParticle->vararray) + emitter_index;
	ref = emitter_data->last_particle;
	if (ref == P3cParticleReference_Null)
		return UUcFalse;

	P3mUnpackParticleReference(ref, explode_class, explode_particle);
	UUmAssert(explode_class == inClass->definition->emitter[emitter_index].emittedclass);
	if (explode_particle->header.self_ref != ref) {
		// the particle is already dead
		return UUcFalse;
	}

	return P3iPerformActionList(explode_class, explode_particle, P3cEvent_Explode,
				&explode_class->definition->eventlist[P3cEvent_Explode], inCurrentT, 0, UUcFalse);
}

// bounce off the object that was just collided with
//
// param 0: elastic fraction for direct collisions
// param 1: elastic fraction for glancing collisions
UUtBool P3iAction_Bounce(P3tParticleClass *inClass, P3tParticle *inParticle,
						P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	M3tVector3D *p_position, *p_velocity, v_offset;
	float v_dot_n, n_dot_n, elastic_direct, elastic_glancing;
	UUtBool returnval, is_dead;

	if (!P3gCurrentCollision.collided)
		return UUcFalse;

	// set the particle to be at the position of the hit point plus 0.01 * hit_normal
	p_position = P3rGetPositionPtr(inClass, inParticle, &is_dead);
	if (is_dead) {
		return UUcTrue;
	}

	if (MUmVector_GetDistanceSquared(*p_position, P3gCurrentCollision.hit_point) > 25.0f) {
		UUrEnterDebugger("teleporting bounce");
	}

	MUmVector_Copy(*p_position, P3gCurrentCollision.hit_normal);
	MUmVector_Scale(*p_position, 0.01f);
	MUmVector_Add(*p_position, *p_position, P3gCurrentCollision.hit_point);

	p_velocity = P3rGetVelocityPtr(inClass, inParticle);
	if (!p_velocity) {
		return UUcFalse;
	}
	
#if 0 && defined(DEBUGGING) && DEBUGGING
	{
		float mag_0, mag_1;
		mag_0 = MUmVector_GetLength(*p_velocity);
#endif

	v_dot_n = MUrVector_DotProduct(p_velocity, &P3gCurrentCollision.hit_normal);
	if (v_dot_n > 0) {
		// particle is already heading away
		return UUcFalse;
	}

	// calculate the new velocity vector
	n_dot_n = MUmVector_GetLengthSquared(P3gCurrentCollision.hit_normal);
	MUmVector_Copy(v_offset, P3gCurrentCollision.hit_normal);
	MUmVector_Scale(v_offset, - 2 * v_dot_n / n_dot_n);
	MUmVector_Add(*p_velocity, *p_velocity, v_offset);

#if 0 && defined(DEBUGGING) && DEBUGGING
		mag_1 = MUmVector_GetLength(*p_velocity);

		if ((mag_0 / mag_1 > 1.01f) || (mag_0 / mag_1 < 0.99f)) {
			COrConsole_Printf("### bounce %f -> %f, norm %f %f %f (%s)", mag_0, mag_1, 
				P3gCurrentCollision.hit_normal.x, P3gCurrentCollision.hit_normal.y, P3gCurrentCollision.hit_normal.z,
				(P3gCurrentCollision.env_collision) ? "env" : "phy");

		}
#endif

	P3mAssertFloatValue(inAction->action_value[0]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &elastic_direct);
	if (!returnval)
		return UUcFalse;

	P3mAssertFloatValue(inAction->action_value[1]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &elastic_glancing);
	if (!returnval)
		return UUcFalse;

	// calculate the new velocity magnitude
	// FIXME: account for glancing / directness
	MUmVector_Scale(*p_velocity, elastic_direct);

#if 0 && defined(DEBUGGING) && DEBUGGING
		COrConsole_Printf("### bounce %f -> %f (%s)", mag_0, MUmVector_GetLength(*p_velocity), (P3gCurrentCollision.env_collision) ? "env" : "phy");
	}
#endif

	if (P3gCurrentCollision.env_collision && (MUmVector_GetLengthSquared(*p_velocity) < UUmSQR(P3cBounceStopVelocity))) {
		// set the velocity to (close to) zero and stop the particle (no further collisions or movement)
		MUmVector_Set(*p_velocity, 0, 0, 0);
		inParticle->header.flags |= P3cParticleFlag_Stopped;
		inParticle->header.flags &= ~P3cParticleFlag_OrientToVelocity;
	}

#if 0 && defined(DEBUGGING) && DEBUGGING
	if (P3gCurrentCollision.hit_t - inCurrentT < 0.01f) {
		COrConsole_Printf("### particle hit_t %f < timestep start %f", P3gCurrentCollision.hit_t, inCurrentT);
	} else if (P3gCurrentCollision.hit_t - inCurrentT - inDeltaT > 0.01f) {
		COrConsole_Printf("### particle hit_t %f < timestep end %f (delta %f)", P3gCurrentCollision.hit_t, inCurrentT + inDeltaT, inDeltaT);
	}
#endif

	inParticle->header.current_tick = MUrUnsignedSmallFloat_To_Uns_Round(P3gCurrentCollision.hit_t * UUcFramesPerSecond);
	inParticle->header.current_time = P3gCurrentCollision.hit_t;

	inParticle->header.flags |= P3cParticleFlag_VelocityChanged;

	return UUcFalse;
}

// stick to a wall that was just collided with
UUtBool P3iAction_StickToWall(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	M3tVector3D *p_position, *p_velocity;
	UUtBool is_dead;

	if (!P3gCurrentCollision.collided)
		return UUcFalse;

	p_position = P3rGetPositionPtr(inClass, inParticle, &is_dead);
	if (is_dead) {
		return UUcTrue;
	}

	MUmVector_Copy(*p_position, P3gCurrentCollision.hit_point);

	p_velocity = P3rGetVelocityPtr(inClass, inParticle);
	if (p_velocity) {
		MUmVector_Set(*p_velocity, 0, 0, 0);

		inParticle->header.flags |= (P3cParticleFlag_VelocityChanged | P3cParticleFlag_Stopped);
		inParticle->header.flags &= ~P3cParticleFlag_OrientToVelocity;
	}
	
	return UUcFalse;
}

// create a particle at the location of the last collision
UUtBool P3iAction_CollisionEffect(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	P3tParticleClass			*created_class;
	P3tOwner					*parent_owner;
	UUtBool						returnval, is_dead, attach;
	P3tEffectSpecification		effect_spec;
	P3tEffectData				effect_data;

	created_class = (P3tParticleClass *) inAction->action_data;
	if (created_class == NULL)
		return UUcFalse;

	UUrMemory_Clear(&effect_data, sizeof(effect_data));
	UUrMemory_Clear(&effect_spec, sizeof(effect_spec));

	effect_data.parent_class = inClass;
	effect_data.parent_particle = inParticle;

	// must be a valid particle class pointer
	effect_spec.particle_class = created_class;
	UUmAssertReadPtr(effect_spec.particle_class, sizeof(P3tParticleClass));
	UUmAssert(effect_spec.particle_class->definition->version == P3cVersion_Current);

	// get the collision orientation - this  is a recent addition and may not be correct.
	// so only evaluate it if we know it's the right kind of value
	if (inAction->action_value[2].type == P3cValueType_Enum) {
		effect_spec.collision_orientation = (P3tEnumCollisionOrientation) inAction->action_value[2].u.enum_const.val;
	} else {
		effect_spec.collision_orientation = P3cEnumCollisionOrientation_Normal;
	}

	// get the value of 'attach to object' - this  is a recent addition and may not be correct.
	// so only evaluate it if we know it's the right kind of value
	if (inAction->action_value[3].type == P3cValueType_Enum) {
		attach = (UUtBool) inAction->action_value[3].u.enum_const.val;
	} else {
		attach = UUcFalse;
	}

	if (P3gCurrentCollision.collided) {
		// get the collision location
		MUmVector_Copy(effect_data.position, P3gCurrentCollision.hit_point);
		MUmVector_Copy(effect_data.normal, P3gCurrentCollision.hit_normal);

		if (P3gCurrentCollision.env_collision) {
			attach = UUcFalse;
		}

		if (attach) {
			effect_spec.location_type = P3cEffectLocationType_AttachedToObject;
		} else {
			// get the collision offset
			P3mAssertFloatValue(inAction->action_value[1]);
			returnval = P3iCalculateValue(inParticle, &inAction->action_value[1],
							(void *) &effect_spec.location_data.collisionoffset.offset);

			if (returnval && (effect_spec.location_data.collisionoffset.offset != 0)) {
				effect_spec.location_type = P3cEffectLocationType_CollisionOffset;
			} else {
				effect_spec.location_type = P3cEffectLocationType_Default;
			}
		}

		if (P3gCurrentCollision.env_collision) {
			effect_data.cause_type = P3cEffectCauseType_ParticleHitWall;
			effect_data.cause_data.particle_wall.gq_index = P3gCurrentCollision.hit_gqindex;
		} else {
			effect_data.cause_type = P3cEffectCauseType_ParticleHitPhysics;
			effect_data.cause_data.particle_physics.context = P3gCurrentCollision.hit_context;
		}
	} else {
		// this particle will be created at the current particle's location (in midair?)
		if (inParticle->header.flags & P3cParticleFlag_CutsceneKilled) {
			effect_data.cause_type = P3cEffectCauseType_CutsceneKilled;
		} else {
			effect_data.cause_type = P3cEffectCauseType_None;
		}

		is_dead = P3rGetRealPosition(inClass, inParticle, &effect_data.position);
		if (is_dead) {
			return UUcTrue;
		}

		effect_spec.location_type = P3cEffectLocationType_Default;
		if ((effect_spec.collision_orientation == P3cEnumCollisionOrientation_Reflected) ||
			(effect_spec.collision_orientation == P3cEnumCollisionOrientation_Normal)) {
			// no normal -> reflected and normal are not allowed
			effect_spec.collision_orientation = P3cEnumCollisionOrientation_Unchanged;
		}
	}

	MUmVector_Set(effect_data.velocity, 0, 0, 0);
	parent_owner = P3rGetOwnerPtr(inClass, inParticle);
	effect_data.owner = (parent_owner == NULL) ? 0 : *parent_owner;

	effect_data.parent_orientation	= P3rGetOrientationPtr(inClass, inParticle);
	effect_data.parent_velocity		= P3rGetVelocityPtr(inClass, inParticle);

	effect_data.time = MUrUnsignedSmallFloat_To_Uns_Round(inCurrentT * UUcFramesPerSecond);

	if( created_class->definition->flags2 & P3cParticleClassFlag2_Appearance_Decal )
	{
		effect_spec.location_type						= P3cEffectLocationType_Decal;
		effect_spec.location_data.decal.static_decal	= UUcFalse;
		effect_spec.location_data.decal.random_rotation	= UUcTrue;
		effect_spec.collision_orientation				= P3cEnumCollisionOrientation_Normal;

		effect_data.decal_xscale						= 1.0f;
		effect_data.decal_yscale						= 1.0f;
	}

	P3rCreateEffect(&effect_spec, &effect_data);

	return UUcFalse;
}

static MAtMaterialType P3iPhysicsGetMaterialType(PHtPhysicsContext *inContext)
{
	MAtMaterialType material_type = MAcMaterial_Base;

	switch(inContext->callback->type)
	{
		case PHcCallback_Character:
			{
				ONtCharacter *character = inContext->callbackData;
				UUtUns32 closest_character_part_index = ONrCharacter_FindClosestPart(character, &P3gCurrentCollision.hit_point);

				material_type = ONrCharacter_GetMaterialType(character, closest_character_part_index, UUcFalse);
			}
		break;

		case PHcCallback_Object:
			{
				OBtObject *object = inContext->callbackData;

				if (object->geometry_count > 0) {
					material_type = (MAtMaterialType) object->geometry_list[0]->baseMap->materialType;
				}
			}
		break;

		case PHcCallback_Weapon:
		case PHcCallback_Camera:
		case PHcCallback_Ammo:
		case PHcCallback_Particle:
		case PHcCallback_Powerup:
		default:
		break;
	}

	return material_type;
}

// create an impact effect
UUtBool P3iAction_ImpactEffect(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	P3tOwner					*parent_owner;
	UUtBool						returnval, is_dead;
//	P3tEffectSpecification		effect_spec;
	P3tEffectData				effect_data;
	AKtEnvironment				*environment;
	AKtGQ_Render				*render_gq;
	MAtImpactType				impact_type;
	MAtMaterialType				material_type;
	void						*owning_character;
	UUtUns32					tex_index, impact_modifier, tex_material;

	impact_type = (MAtImpactType) ((UUtUns32) inAction->action_data);
	if ((impact_type == MAcInvalidID) || (impact_type < 0) || (impact_type >= MArImpacts_GetNumTypes()))
		return UUcFalse;

	P3mAssertEnumValue(inAction->action_value[1], P3cEnumType_ImpactModifier);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &impact_modifier);
	if (!returnval || (impact_modifier < 0) || (impact_modifier >= ONcIEModType_NumTypes))
		return UUcFalse;

	// set up the effect data
	UUrMemory_Clear(&effect_data, sizeof(effect_data));
	effect_data.parent_class = inClass;
	effect_data.parent_particle = inParticle;
	effect_data.parent_orientation	= P3rGetOrientationPtr(inClass, inParticle);
	effect_data.parent_velocity		= P3rGetVelocityPtr(inClass, inParticle);

	parent_owner = P3rGetOwnerPtr(inClass, inParticle);
	effect_data.owner = (parent_owner == NULL) ? 0 : *parent_owner;
	effect_data.time = MUrUnsignedSmallFloat_To_Uns_Round(inCurrentT * UUcFramesPerSecond);
	effect_data.decal_xscale = 1.0f;
	effect_data.decal_yscale = 1.0f;

	material_type = MAcMaterial_Base;
	if (P3gCurrentCollision.collided) {
		// get the collision location
		MUmVector_Copy(effect_data.position, P3gCurrentCollision.hit_point);
		MUmVector_Copy(effect_data.normal, P3gCurrentCollision.hit_normal);

		// generate the impact effect a short distance off the wall
		MUmVector_ScaleIncrement(effect_data.position, 0.2f, effect_data.normal);

		if (P3gCurrentCollision.env_collision) {
			effect_data.cause_type = P3cEffectCauseType_ParticleHitWall;
			effect_data.cause_data.particle_wall.gq_index = P3gCurrentCollision.hit_gqindex;

			// get the texture off this GQ
			environment = ONrGameState_GetEnvironment();
			render_gq = environment->gqRenderArray->gqRender + P3gCurrentCollision.hit_gqindex;
			tex_index = render_gq->textureMapIndex;
			if (tex_index < environment->textureMapArray->numMaps) {
				// determine the material type for this texture
				tex_material = environment->textureMapArray->maps[tex_index]->materialType;
				if ((tex_material >= 0) || (tex_material < MArMaterials_GetNumTypes())) {
					material_type = (MAtMaterialType) tex_material;
				}
			}

		} else {
			effect_data.cause_type = P3cEffectCauseType_ParticleHitPhysics;
			effect_data.cause_data.particle_physics.context = P3gCurrentCollision.hit_context;

			// FIXME: get the material for this physics context (somehow)
			// this is tricky because it might be a character, which has materials, or some
			// other kind of physics context
			
			material_type = P3iPhysicsGetMaterialType(effect_data.cause_data.particle_physics.context);
		}
	} else {
		// this particle will be created at the current particle's location (in midair?)
		if (inParticle->header.flags & P3cParticleFlag_CutsceneKilled) {
			effect_data.cause_type = P3cEffectCauseType_CutsceneKilled;
		} else {
			effect_data.cause_type = P3cEffectCauseType_None;
		}
		is_dead = P3rGetRealPosition(inClass, inParticle, &effect_data.position);
		if (is_dead) {
			return UUcTrue;
		}
	}

	// get the character that caused this impact effect
	if (!WPrOwner_IsCharacter(effect_data.owner, &owning_character)) {
		owning_character = NULL;
	}

	ONrImpactEffect(impact_type, material_type, (UUtUns16) impact_modifier, &effect_data, 1.0f, (ONtCharacter *) owning_character, NULL);
	return UUcFalse;
}

// hurt a character that was hit by a particle
UUtBool P3iAction_DamageChar(P3tParticleClass *inClass, P3tParticle *inParticle,
							P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	float damage, stundamage, knockback;
	UUtBool returnval, self_immune, can_damage_multiple, duplicate_collision;
	P3tOwner *p_owner, owner;
	void *hit;
	P3tEnumDamageType damage_type;

	if (inParticle->header.flags & P3cParticleFlag_CutsceneKilled) {
		// don't kill people with exploding munitions caused by a cutscene
		return UUcFalse;
	}
	
	if ((!P3gCurrentCollision.collided) || (P3gCurrentCollision.env_collision) ||
		(P3gCurrentCollision.damaged_hit) || (P3gCurrentCollision.hit_context->callback->type != PHcCallback_Character))
		return UUcFalse;

	// get a pointer to the character in question
	hit = P3gCurrentCollision.hit_context->callbackData;


	/*
	 * read the parameters of the hit
	 */

	P3mAssertFloatValue(inAction->action_value[0]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &damage);
	if (!returnval)
		return UUcFalse;
	
	P3mAssertFloatValue(inAction->action_value[1]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &stundamage);
	if (!returnval)
		return UUcFalse;
	
	P3mAssertFloatValue(inAction->action_value[2]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[2], (void *) &knockback);
	if (!returnval)
		return UUcFalse;

	if (inAction->action_value[3].type == P3cValueType_Enum) {
		damage_type = (P3tEnumDamageType) inAction->action_value[3].u.enum_const.val;
		if ((damage_type < 0) || (damage_type >= P3cEnumDamageType_Max)) {
			damage_type = P3cEnumDamageType_Normal;
		}
	} else {
		damage_type = P3cEnumDamageType_Normal;
	}

	P3mAssertEnumValue(inAction->action_value[4], P3cEnumType_Bool);
	self_immune = (UUtBool) inAction->action_value[4].u.enum_const.val;
	
	if (inAction->action_value[5].type == P3cValueType_Enum) {
		P3mAssertEnumValue(inAction->action_value[5], P3cEnumType_Bool);
		can_damage_multiple = (UUtBool) inAction->action_value[5].u.enum_const.val;
	} else {
		can_damage_multiple = UUcTrue;
	}

	p_owner = P3rGetOwnerPtr(inClass, inParticle);
	if (NULL == p_owner) {
		owner = 0;
	} 
	else {
		owner = *p_owner;
	}

	// COrConsole_Printf("class name doing damage %s %d", inClass->classname, damage_type);

	if (WPcDamageOwner_ActOfGod == (owner & WPcDamageOwner_TypeMask)) {
		COrConsole_Printf("particle %s doing 'ActOfGod' damage", inClass->classname) ;

		owner = 0;
	}
	
	if (WPcDamageOwner_Falling == (owner & WPcDamageOwner_TypeMask)) {
		COrConsole_Printf("particle %s doing 'Falling' damage", inClass->classname) ;

		owner = 0;
	}

	duplicate_collision = WPrHitChar(hit, &P3gCurrentCollision.hit_point, &P3gCurrentCollision.hit_direction, &P3gCurrentCollision.hit_normal,
									damage_type, damage, stundamage, knockback, owner,
									(can_damage_multiple) ? P3cParticleReference_Null : inParticle->header.self_ref, self_immune);
	if (duplicate_collision) {
		// this is not actually a valid collision, stop executing
		*outStop = UUcTrue;
	}

	return UUcFalse;
}

// create a blast effect at the location of the last collision, or our current location
UUtBool P3iAction_DamageBlast(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	float damage, stundamage, knockback, radius;
	P3tEnumFalloff falloff_type;
	UUtBool returnval, is_dead, self_immune, damage_environment;
	UUtUns32 itr;
	float velocity_mag;
	AKtEnvironment *env;
	M3tVector3D velocity, blast_normal, *p_velocity, *blastptr;
	M3tBoundingSphere blast_sphere;
	P3tOwner *p_owner, owner;
	void *hit;
	P3tEnumDamageType damage_type;

	/*
	 * read the parameters of the blast
	 */

	P3mAssertFloatValue(inAction->action_value[0]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &damage);
	if (!returnval)
		return UUcFalse;
	
	P3mAssertFloatValue(inAction->action_value[1]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &stundamage);
	if (!returnval)
		return UUcFalse;
	
	P3mAssertFloatValue(inAction->action_value[2]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[2], (void *) &knockback);
	if (!returnval)
		return UUcFalse;
	
	P3mAssertFloatValue(inAction->action_value[3]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[3], (void *) &radius);
	if (!returnval)
		return UUcFalse;
	
	P3mAssertEnumValue(inAction->action_value[4], P3cEnumType_Falloff);
	falloff_type = (P3tEnumFalloff)inAction->action_value[4].u.enum_const.val;
	
	if (inAction->action_value[5].type == P3cValueType_Enum) {
		damage_type = (P3tEnumDamageType) inAction->action_value[5].u.enum_const.val;
		if ((damage_type < 0) || (damage_type >= P3cEnumDamageType_Max)) {
			damage_type = P3cEnumDamageType_Normal;
		}
	} else {
		damage_type = P3cEnumDamageType_Normal;
	}

	P3mAssertEnumValue(inAction->action_value[6], P3cEnumType_Bool);
	self_immune = (UUtBool) inAction->action_value[6].u.enum_const.val;
	
	if (inAction->action_value[7].type == P3cValueType_Enum) {
		P3mAssertEnumValue(inAction->action_value[7], P3cEnumType_Bool);
		damage_environment = (UUtBool) inAction->action_value[7].u.enum_const.val;
	} else {
		damage_environment = UUcTrue;
	}

	p_owner = P3rGetOwnerPtr(inClass, inParticle);
	if (p_owner == NULL) {
		owner = 0;
	} else {
		owner = *p_owner;
	}

	hit = NULL;
	if (P3gCurrentCollision.collided) {
		if (!P3gCurrentCollision.env_collision && P3gCurrentCollision.damaged_hit) {
			// we have already damaged the character / object that we hit
			if (P3gCurrentCollision.hit_context->callback->type == PHcCallback_Character) {
				// get the character pointer so that it can be skipped
#if PARTICLE_DEBUG_BLAST
				COrConsole_Printf("particle %s blast skipping hit character, already damaged", inClass->classname);
#endif
				hit = P3gCurrentCollision.hit_context->callbackData;
			}
		} 
	}

	// look for all quads within the blast radius and damage them
	env = ONrGameState_GetEnvironment();
	blast_sphere.radius = radius;

	if (P3gCurrentCollision.collided) {
		// create blast at location of collision, plus an offset along normal of collision
		MUmVector_Copy(blast_sphere.center, P3gCurrentCollision.hit_point);
		MUmVector_ScaleIncrement(blast_sphere.center, P3cBlastCenter_NormalOffset, P3gCurrentCollision.hit_normal);
	} else {
		// create blast at current location
		is_dead = P3rGetRealPosition(inClass, inParticle, &blast_sphere.center);
		if (is_dead) {
#if PARTICLE_DEBUG_BLAST
			COrConsole_Printf("particle %s blast ignored, particle is dead", inClass->classname);
#endif
			return UUcTrue;
		}
	}

	if (damage_environment) {
		blastptr = NULL;
		p_velocity = P3rGetVelocityPtr(inClass, inParticle);
		if (p_velocity != NULL) {
			velocity_mag = MUmVector_GetLength(*p_velocity);
			if (velocity_mag > 4.0f) {
				MUmVector_ScaleCopy(blast_normal, 1.0f / velocity_mag, *p_velocity);
				blastptr = &blast_normal;
			}
		}

		MUmVector_Set(velocity, 0, 0, 0);
		AKrCollision_Sphere(env, &blast_sphere, &velocity, AKcGQ_Flag_Obj_Col_Skip, AKcMaxNumCollisions);
		for (itr = 0; itr < AKgNumCollisions; itr++) {
			P3iDamageQuad(env, AKgCollisionList[itr].gqIndex, damage, &blast_sphere.center, blastptr, radius, UUcFalse);
		}
	}

	if (inParticle->header.flags & P3cParticleFlag_CutsceneKilled) {
		// don't kill people with exploding munitions caused by a cutscene
		// (but do damage the environment)
#if PARTICLE_DEBUG_BLAST
		COrConsole_Printf("particle %s blast ignored, particle was killed in cutscene", inClass->classname);
#endif
		return UUcFalse;
	}
	
	// COrConsole_Printf("p3 blast damage %s %d", inClass->classname, damage_type);

#if PARTICLE_DEBUG_BLAST
	COrConsole_Printf("particle %s causing blast at (%f %f %f) radius %f - dmg %f stun %f knockback %f",
						inClass->classname, blast_sphere.center.x, blast_sphere.center.y, blast_sphere.center.z,
						radius, damage, stundamage, knockback);
#endif
	WPrBlast(&blast_sphere.center, damage_type, damage, stundamage, knockback, radius,
			falloff_type, owner, hit, inParticle->header.self_ref, self_immune);			

	return UUcFalse;
}

// explode as if we were a glass charge
UUtBool P3iAction_GlassCharge(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	float blast_vel, radius;
	UUtBool returnval, is_dead;
	UUtUns32 itr;
	M3tMatrix3x3 *p_orientation;
	AKtEnvironment *env;
	M3tVector3D velocity, blast_normal, *blast_dir, *blast_location;
	M3tBoundingSphere blast_sphere;

	/*
	 * read the parameters of the blast
	 */

	P3mAssertFloatValue(inAction->action_value[0]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &blast_vel);
	if (!returnval)
		return UUcFalse;
	
	P3mAssertFloatValue(inAction->action_value[1]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &radius);
	if (!returnval)
		return UUcFalse;
	
	// create blast at current location
	blast_sphere.radius = radius;
	is_dead = P3rGetRealPosition(inClass, inParticle, &blast_sphere.center);
	if (is_dead)
		return UUcFalse;

	env = ONrGameState_GetEnvironment();

	if (blast_vel == 0) {
		// this is just a shatter effect as if the quad was hit by a bullet
		blast_location = NULL;
		blast_dir = NULL;
	} else {
		p_orientation = P3rGetOrientationPtr(inClass, inParticle);
		if (p_orientation == NULL) {
			blast_dir = NULL;
		} else {
			blast_normal = MUrMatrix_GetYAxis((M3tMatrix4x3 *) p_orientation);
			blast_dir = &blast_normal;
		}
		blast_location = &blast_sphere.center;
	}

	// look for all quads within the blast radius and damage them
	MUmVector_Set(velocity, 0, 0, 0);
	AKrCollision_Sphere(env, &blast_sphere, &velocity, AKcGQ_Flag_Obj_Col_Skip, AKcMaxNumCollisions);

	for (itr = 0; itr < AKgNumCollisions; itr++) {
		P3iDamageQuad(env, AKgCollisionList[itr].gqIndex, blast_vel, blast_location, blast_dir, radius, UUcTrue);
	}

	return UUcFalse;
}

// trigger the 'explode' action list
UUtBool P3iAction_Explode(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	return P3iPerformActionList(inClass, inParticle, P3cEvent_Explode, &inClass->definition->eventlist[P3cEvent_Explode], inCurrentT, 0, UUcFalse);
}

// trigger the 'stop' action list
UUtBool P3iAction_Stop(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	return P3iPerformActionList(inClass, inParticle, P3cEvent_Stop, &inClass->definition->eventlist[P3cEvent_Stop], inCurrentT, 0, UUcFalse);
}

// damage the environment (glass, etc) from a point collision
UUtBool P3iAction_DamageEnvironment(P3tParticleClass *inClass, P3tParticle *inParticle,
							P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	AKtEnvironment *env;
	float damage;
	UUtBool returnval;

	if ((!P3gCurrentCollision.collided) || (!P3gCurrentCollision.env_collision))
		return UUcFalse;

	P3mAssertFloatValue(inAction->action_value[0]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &damage);
	if (!returnval)
		return UUcFalse;
	
	// damage just the quad that we hit
	env = ONrGameState_GetEnvironment();
	P3iDamageQuad(env, P3gCurrentCollision.hit_gqindex, damage, NULL, NULL, 0, UUcFalse);

	return UUcFalse;
}

// show a particle
UUtBool P3iAction_Show(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	inParticle->header.flags &= ~P3cParticleFlag_Hidden;

	return UUcFalse;
}

// hide a particle
UUtBool P3iAction_Hide(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	inParticle->header.flags |= P3cParticleFlag_Hidden;

	return UUcFalse;
}

// set the current texture time of a particle so that it is currently displaying a certain tick
//
// param 0: float: tick to set to
UUtBool P3iAction_SetTextureTick(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	float tickval;
	UUtUns32 tick_index, current_tick, *p_texval;
	UUtBool returnval;

	P3mAssertFloatValue(inAction->action_value[0]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &tickval);
	if (!returnval)
		return UUcFalse;

	tick_index = MUrUnsignedSmallFloat_To_Uns_Round(tickval);
	current_tick = MUrUnsignedSmallFloat_To_Uns_Round(inCurrentT * UUcFramesPerSecond);

	p_texval = P3rGetTextureTimePtr(inClass, inParticle);
	if (p_texval != NULL) {
		// store an appropriate texture start time that will give us tick_index at the current game tick
		*p_texval = current_tick - tick_index;
	}

	return UUcFalse;
}

// randomize the current texture time of a particle
UUtBool P3iAction_RandomTextureFrame(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	UUtUns32 *p_texval;

	p_texval = P3rGetTextureTimePtr(inClass, inParticle);
	if (p_texval != NULL) {
		// generate a random 32-bit number
		*p_texval = UUrLocalRandom() | (((UUtUns32) UUrLocalRandom()) << 16);
	}

	return UUcFalse;
}

// set the current lifetime of a particle
//
// param 0: float: value to set to (in seconds)
UUtBool P3iAction_SetLifetime(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	P3mAssertFloatValue(inAction->action_value[0]);
	P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &inParticle->header.lifetime);

	return UUcFalse;
}

// set a float variable to a value
//
// var 0: float variable to set
// param 1: float: value to set to
UUtBool P3iAction_SetVariable(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	P3iCalculateValue(inParticle, &inAction->action_value[0], inVariables[0]);

	return UUcFalse;
}

// recalculate all variables in a particle
UUtBool P3iAction_RecalculateAll(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	P3tVariableInfo *var;
	UUtUns32 itr;

	// initialise all of the variables attached to this particle
	var = inClass->definition->variable;
	for (itr = 0; itr < inClass->definition->num_variables; itr++, var++) {
		P3iCalculateValue(NULL, &var->initialiser, (void *) &inParticle->vararray[var->offset]);
	}

	return UUcFalse;
}

// enable an action at the designated lifetime
//
// param 0: actionindex: action
// param 1: float:		 value of remaining lifetime
UUtBool P3iAction_EnableAtTime(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	UUtUns32 actionindex;
	float target_time;
	UUtBool returnval;

	P3mAssertEnumValue(inAction->action_value[0], P3cEnumType_ActionIndex);
	P3mAssertFloatValue(inAction->action_value[1]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &actionindex);
	if (!returnval)
		return UUcFalse;
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &target_time);
	if (!returnval)
		return UUcFalse;

	UUmAssert((actionindex >= 0) && (actionindex < 31));

	// enable if we have less than this target amount of time left to go
	if (inParticle->header.lifetime < target_time) {
		inParticle->header.actionmask &= ~(1 << actionindex);
	} else {
		inParticle->header.actionmask |= (1 << actionindex);
	}

	return UUcFalse;
}

// disable an action at the designated lifetime
//
// param 0: actionindex: action
// param 1: float:		 value of remaining lifetime
UUtBool P3iAction_DisableAtTime(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	UUtUns32 actionindex;
	float target_time;
	UUtBool returnval;

	P3mAssertEnumValue(inAction->action_value[0], P3cEnumType_ActionIndex);
	P3mAssertFloatValue(inAction->action_value[1]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &actionindex);
	if (!returnval)
		return UUcFalse;
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &target_time);
	if (!returnval)
		return UUcFalse;

	UUmAssert((actionindex >= 0) && (actionindex < 31));

	// enable if we have more than this target amount of time left to go
	if (inParticle->header.lifetime > target_time) {
		inParticle->header.actionmask &= ~(1 << actionindex);
	} else {
		inParticle->header.actionmask |= (1 << actionindex);
	}

	return UUcFalse;
}

// enable an action when a variable is above a threshold
//
// param 0: actionindex: action
// param 1: float:		 variable
// param 2: float:		 threshold value
UUtBool P3iAction_EnableAbove(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	UUtUns32 actionindex;
	float variable, threshold;
	UUtBool returnval;

	P3mAssertEnumValue(inAction->action_value[0], P3cEnumType_ActionIndex);
	P3mAssertFloatValue(inAction->action_value[1]);
	P3mAssertFloatValue(inAction->action_value[2]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &actionindex);
	if (!returnval)
		return UUcFalse;
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &variable);
	if (!returnval)
		return UUcFalse;
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[2], (void *) &threshold);
	if (!returnval)
		return UUcFalse;

	UUmAssert((actionindex >= 0) && (actionindex < 31));

	if (variable > threshold) {
		inParticle->header.actionmask &= ~(1 << actionindex);
	} else {
		inParticle->header.actionmask |= (1 << actionindex);
	}

	return UUcFalse;
}

// enable an action when a variable is below a threshold
//
// param 0: actionindex: action
// param 1: float:		 variable
// param 2: float:		 threshold value
UUtBool P3iAction_EnableBelow(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	UUtUns32 actionindex;
	float variable, threshold;
	UUtBool returnval;

	P3mAssertEnumValue(inAction->action_value[0], P3cEnumType_ActionIndex);
	P3mAssertFloatValue(inAction->action_value[1]);
	P3mAssertFloatValue(inAction->action_value[2]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &actionindex);
	if (!returnval)
		return UUcFalse;
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &variable);
	if (!returnval)
		return UUcFalse;
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[2], (void *) &threshold);
	if (!returnval)
		return UUcFalse;

	UUmAssert((actionindex >= 0) && (actionindex < 31));

	if (variable < threshold) {
		inParticle->header.actionmask &= ~(1 << actionindex);
	} else {
		inParticle->header.actionmask |= (1 << actionindex);
	}

	return UUcFalse;
}

// enable an action
//
// param 0: actionindex: action
UUtBool P3iAction_EnableNow(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	UUtUns32 actionindex;
	UUtBool returnval;

	P3mAssertEnumValue(inAction->action_value[0], P3cEnumType_ActionIndex);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &actionindex);
	if (!returnval)
		return UUcFalse;

	UUmAssert((actionindex >= 0) && (actionindex < 31));
	inParticle->header.actionmask &= ~(1 << actionindex);

	return UUcFalse;
}

// disable an action
//
// param 0: actionindex: action
UUtBool P3iAction_DisableNow(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	UUtUns32 actionindex;
	UUtBool returnval;

	P3mAssertEnumValue(inAction->action_value[0], P3cEnumType_ActionIndex);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &actionindex);
	if (!returnval)
		return UUcFalse;

	UUmAssert((actionindex >= 0) && (actionindex < 31));
	inParticle->header.actionmask |= (1 << actionindex);

	return UUcFalse;
}

// rotate a particle about the X axis
//
// param 0: coord frame:	worldspace is tumbling, localspace is turning
// param 1: float:			rate in degrees per second
// param 2: bool:			whether to rotate velocity as well
UUtBool P3iAction_RotateX(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	UUtBool returnval;
	P3tEnumCoordFrame space;
	P3tEnumBool rotate_vel;
	float rotate_rate, theta, costheta, sintheta;
	M3tMatrix3x3 *orientation, new_orientation;
	M3tVector3D *velocity, new_velocity;

	P3mAssertEnumValue(inAction->action_value[0], P3cEnumType_CoordFrame);
	P3mAssertFloatValue(inAction->action_value[1]);
	P3mAssertEnumValue(inAction->action_value[2], P3cEnumType_Bool);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &space);
	if (!returnval)
		return UUcFalse;
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &rotate_rate);
	if (!returnval)
		return UUcFalse;
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[2], (void *) &rotate_vel);
	if (!returnval)
		return UUcFalse;

	theta = M3cDegToRad * rotate_rate * inDeltaT;
	UUmTrig_Clip(theta);

	costheta = MUrCos(theta);
	sintheta = MUrSin(theta);

	orientation = P3rGetOrientationPtr(inClass, inParticle);
	if (orientation != NULL) {
		// rotate the orientation... either about its own X axis (postmultiply by rotation matrix)
		// or about the global X axis (premultiply by rotation matrix).
		if (space == P3cEnumCoordFrame_Local) {
			new_orientation.m[0][0] = orientation->m[0][0];
			new_orientation.m[0][1] = orientation->m[0][1];
			new_orientation.m[0][2] = orientation->m[0][2];

			new_orientation.m[1][0] =   costheta * orientation->m[1][0] + sintheta * orientation->m[2][0];
			new_orientation.m[1][1] =   costheta * orientation->m[1][1] + sintheta * orientation->m[2][1];
			new_orientation.m[1][2] =   costheta * orientation->m[1][2] + sintheta * orientation->m[2][2];

			new_orientation.m[2][0] = - sintheta * orientation->m[1][0] + costheta * orientation->m[2][0];
			new_orientation.m[2][1] = - sintheta * orientation->m[1][1] + costheta * orientation->m[2][1];
			new_orientation.m[2][2] = - sintheta * orientation->m[1][2] + costheta * orientation->m[2][2];
		} else {
			new_orientation.m[0][0] = orientation->m[0][0];
			new_orientation.m[0][1] = costheta * orientation->m[0][1] - sintheta * orientation->m[0][2];
			new_orientation.m[0][2] = sintheta * orientation->m[0][1] + costheta * orientation->m[0][2];

			new_orientation.m[1][0] = orientation->m[1][0];
			new_orientation.m[1][1] = costheta * orientation->m[1][1] - sintheta * orientation->m[1][2];
			new_orientation.m[1][2] = sintheta * orientation->m[1][1] + costheta * orientation->m[1][2];

			new_orientation.m[2][0] = orientation->m[2][0];
			new_orientation.m[2][1] = costheta * orientation->m[2][1] - sintheta * orientation->m[2][2];
			new_orientation.m[2][2] = sintheta * orientation->m[2][1] + costheta * orientation->m[2][2];
		}
	} else {
		// no orientation - therefore must be in world coordinates
		space = P3cEnumCoordFrame_World;
	}

	velocity = P3rGetVelocityPtr(inClass, inParticle);
	if (rotate_vel && (velocity != NULL)) {
		if (space == P3cEnumCoordFrame_Local) {
			// rotate velocity in the same way as our orientation was - apply the inverse of the
			// old orientation, then the global rotation, then the new orientation.
			MUrMatrix3x3_TransposeMultiplyPoint(velocity, orientation, velocity);
		}

		// rotate velocity about the global X axis
		new_velocity.x = velocity->x;
		new_velocity.y = costheta * velocity->y - sintheta * velocity->z;
		new_velocity.z = sintheta * velocity->y + costheta * velocity->z;

		if (space == P3cEnumCoordFrame_Local) {
			MUrMatrix3x3_MultiplyPoint(&new_velocity, &new_orientation, velocity);
		} else {
			MUmVector_Copy(*velocity, new_velocity);
		}

		inParticle->header.flags |= P3cParticleFlag_VelocityChanged;
	}

	// set the new orientation
	if (orientation != NULL) {
		*orientation = new_orientation;
	}

	return UUcFalse;
}

// rotate a particle about the Y axis
//
// param 0: coord frame:	worldspace is tumbling, localspace is turning
// param 1: float:			rate in degrees per second
// param 2: bool:			whether to rotate velocity as well
UUtBool P3iAction_RotateY(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	UUtBool returnval;
	P3tEnumCoordFrame space;
	P3tEnumBool rotate_vel;
	float rotate_rate, theta, costheta, sintheta;
	M3tMatrix3x3 *orientation, new_orientation;
	M3tVector3D *velocity, new_velocity;

	P3mAssertEnumValue(inAction->action_value[0], P3cEnumType_CoordFrame);
	P3mAssertFloatValue(inAction->action_value[1]);
	P3mAssertEnumValue(inAction->action_value[2], P3cEnumType_Bool);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &space);
	if (!returnval)
		return UUcFalse;
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &rotate_rate);
	if (!returnval)
		return UUcFalse;
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[2], (void *) &rotate_vel);
	if (!returnval)
		return UUcFalse;

	theta = M3cDegToRad * rotate_rate * inDeltaT;
	UUmTrig_Clip(theta);

	costheta = MUrCos(theta);
	sintheta = MUrSin(theta);

	orientation = P3rGetOrientationPtr(inClass, inParticle);
	if (orientation != NULL) {
		// rotate the orientation... either about its own Y axis (postmultiply by rotation matrix)
		// or about the global Y axis (premultiply by rotation matrix).
		if (space == P3cEnumCoordFrame_Local) {
			new_orientation.m[0][0] =   costheta * orientation->m[0][0] + sintheta * orientation->m[2][0];
			new_orientation.m[0][1] =   costheta * orientation->m[0][1] + sintheta * orientation->m[2][1];
			new_orientation.m[0][2] =   costheta * orientation->m[0][2] + sintheta * orientation->m[2][2];

			new_orientation.m[1][0] = orientation->m[1][0];
			new_orientation.m[1][1] = orientation->m[1][1];
			new_orientation.m[1][2] = orientation->m[1][2];

			new_orientation.m[2][0] = - sintheta * orientation->m[0][0] + costheta * orientation->m[2][0];
			new_orientation.m[2][1] = - sintheta * orientation->m[0][1] + costheta * orientation->m[2][1];
			new_orientation.m[2][2] = - sintheta * orientation->m[0][2] + costheta * orientation->m[2][2];
		} else {
			new_orientation.m[0][0] = costheta * orientation->m[0][0] - sintheta * orientation->m[0][2];
			new_orientation.m[0][1] = orientation->m[0][1];
			new_orientation.m[0][2] = sintheta * orientation->m[0][0] + costheta * orientation->m[0][2];

			new_orientation.m[1][0] = costheta * orientation->m[1][0] - sintheta * orientation->m[1][2];
			new_orientation.m[1][1] = orientation->m[1][1];
			new_orientation.m[1][2] = sintheta * orientation->m[1][0] + costheta * orientation->m[1][2];

			new_orientation.m[2][0] = costheta * orientation->m[2][0] - sintheta * orientation->m[2][2];
			new_orientation.m[2][1] = orientation->m[2][1];
			new_orientation.m[2][2] = sintheta * orientation->m[2][0] + costheta * orientation->m[2][2];
		}
	} else {
		// no orientation - therefore must be in world coordinates
		space = P3cEnumCoordFrame_World;
	}

	velocity = P3rGetVelocityPtr(inClass, inParticle);
	if (rotate_vel && (velocity != NULL)) {
		if (space == P3cEnumCoordFrame_Local) {
			// rotate velocity in the same way as our orientation was - apply the inverse of the
			// old orientation, then the global rotation, then the new orientation.
			MUrMatrix3x3_TransposeMultiplyPoint(velocity, orientation, velocity);
		}

		// rotate velocity about the global Y axis
		new_velocity.x = costheta * velocity->x - sintheta * velocity->z;
		new_velocity.y = velocity->y;
		new_velocity.z = sintheta * velocity->x + costheta * velocity->z;

		if (space == P3cEnumCoordFrame_Local) {
			MUrMatrix3x3_MultiplyPoint(&new_velocity, &new_orientation, velocity);
		} else {
			MUmVector_Copy(*velocity, new_velocity);
		}

		inParticle->header.flags |= P3cParticleFlag_VelocityChanged;
	}

	// set the new orientation
	if (orientation != NULL) {
		*orientation = new_orientation;
	}

	return UUcFalse;
}

// rotate a particle about the Z axis
//
// param 0: coord frame:	worldspace is tumbling, localspace is turning
// param 1: float:			rate in degrees per second
// param 2: bool:			whether to rotate velocity as well
UUtBool P3iAction_RotateZ(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	UUtBool returnval;
	P3tEnumCoordFrame space;
	P3tEnumBool rotate_vel;
	float rotate_rate, theta, costheta, sintheta;
	M3tMatrix3x3 *orientation, new_orientation;
	M3tVector3D *velocity, new_velocity;

	P3mAssertEnumValue(inAction->action_value[0], P3cEnumType_CoordFrame);
	P3mAssertFloatValue(inAction->action_value[1]);
	P3mAssertEnumValue(inAction->action_value[2], P3cEnumType_Bool);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &space);
	if (!returnval)
		return UUcFalse;
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &rotate_rate);
	if (!returnval)
		return UUcFalse;
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[2], (void *) &rotate_vel);
	if (!returnval)
		return UUcFalse;

	theta = M3cDegToRad * rotate_rate * inDeltaT;
	UUmTrig_Clip(theta);

	costheta = MUrCos(theta);
	sintheta = MUrSin(theta);

	orientation = P3rGetOrientationPtr(inClass, inParticle);
	if (orientation != NULL) {
		// rotate the orientation... either about its own Z axis (postmultiply by rotation matrix)
		// or about the global Z axis (premultiply by rotation matrix).
		if (space == P3cEnumCoordFrame_Local) {
			new_orientation.m[0][0] =   costheta * orientation->m[0][0] + sintheta * orientation->m[1][0];
			new_orientation.m[0][1] =   costheta * orientation->m[0][1] + sintheta * orientation->m[1][1];
			new_orientation.m[0][2] =   costheta * orientation->m[0][2] + sintheta * orientation->m[1][2];

			new_orientation.m[1][0] = - sintheta * orientation->m[0][0] + costheta * orientation->m[1][0];
			new_orientation.m[1][1] = - sintheta * orientation->m[0][1] + costheta * orientation->m[1][1];
			new_orientation.m[1][2] = - sintheta * orientation->m[0][2] + costheta * orientation->m[1][2];

			new_orientation.m[2][0] = orientation->m[2][0];
			new_orientation.m[2][1] = orientation->m[2][1];
			new_orientation.m[2][2] = orientation->m[2][2];
		} else {
			new_orientation.m[0][0] = costheta * orientation->m[0][0] - sintheta * orientation->m[0][1];
			new_orientation.m[0][1] = sintheta * orientation->m[0][0] + costheta * orientation->m[0][1];
			new_orientation.m[0][2] = orientation->m[0][2];

			new_orientation.m[1][0] = costheta * orientation->m[1][0] - sintheta * orientation->m[1][1];
			new_orientation.m[1][1] = sintheta * orientation->m[1][0] + costheta * orientation->m[1][1];
			new_orientation.m[1][2] = orientation->m[1][2];

			new_orientation.m[2][0] = costheta * orientation->m[2][0] - sintheta * orientation->m[2][1];
			new_orientation.m[2][1] = sintheta * orientation->m[2][0] + costheta * orientation->m[2][1];
			new_orientation.m[2][2] = orientation->m[2][2];
		}
	} else {
		// no orientation - therefore must be in world coordinates
		space = P3cEnumCoordFrame_World;
	}

	velocity = P3rGetVelocityPtr(inClass, inParticle);
	if (rotate_vel && (velocity != NULL)) {
		if (space == P3cEnumCoordFrame_Local) {
			// rotate velocity in the same way as our orientation was - apply the inverse of the
			// old orientation, then the global rotation, then the new orientation.
			MUrMatrix3x3_TransposeMultiplyPoint(velocity, orientation, velocity);
		}

		// rotate velocity about the global Z axis
		new_velocity.x = costheta * velocity->x - sintheta * velocity->y;
		new_velocity.y = sintheta * velocity->x + costheta * velocity->y;
		new_velocity.z = velocity->z;

		if (space == P3cEnumCoordFrame_Local) {
			MUrMatrix3x3_MultiplyPoint(&new_velocity, &new_orientation, velocity);
		} else {
			MUmVector_Copy(*velocity, new_velocity);
		}

		inParticle->header.flags |= P3cParticleFlag_VelocityChanged;
	}

	// set the new orientation
	if (orientation != NULL) {
		*orientation = new_orientation;
	}

	return UUcFalse;
}

// chop a particle to its point of impact
UUtBool P3iAction_Chop(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	float speed, radius, chop_time;
	M3tVector3D *velocity;

	// can't chop a fading or already-chopped particle
	if (inParticle->header.flags & (P3cParticleFlag_FadeOut | P3cParticleFlag_Chop))
		return UUcFalse;

	// can't chop a particle with no velocity
	velocity = P3rGetVelocityPtr(inClass, inParticle);
	if (velocity == NULL)
		return UUcFalse;

	// calculate the particle's current speed
	speed = MUmVector_GetLength(*velocity);
	if (speed < 0.01f)
		return UUcFalse;

	// calculate how long until we've passed through the wall
	P3iCalculateValue(inParticle, &inClass->definition->appearance.scale, &radius);
	chop_time = 2 * radius / speed;

	// set up the chop
	inParticle->header.lifetime = chop_time;
	inParticle->header.fadeout_time = chop_time;
	inParticle->header.flags |= P3cParticleFlag_Chop;

	return UUcFalse;	
}

// make a particle fly away from its spiral path. called once - translates spiral position into linear velocity
UUtBool P3iAction_SpiralTangent(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	M3tMatrix3x3 *p_orientation;
	M3tVector3D *p_velocity, horiz_axis, vert_axis;
	float theta, radius, rotation_rate, fly_speed, costheta, sintheta;
	UUtBool returnval;

	P3mAssertFloatValue(inAction->action_value[0]);
	P3mAssertFloatValue(inAction->action_value[1]);
	P3mAssertFloatValue(inAction->action_value[2]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &theta);
	if (!returnval)
		return UUcFalse;
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &radius);
	if (!returnval)
		return UUcFalse;
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[2], (void *) &rotation_rate);
	if (!returnval)
		return UUcFalse;

	// work out our velocity and/or orientation
	p_velocity = P3rGetVelocityPtr(inClass, inParticle);
	if (p_velocity == NULL)
		return UUcFalse;

	p_orientation = P3rGetOrientationPtr(inClass, inParticle);

	theta *= M3cDegToRad;
	UUmTrig_Clip(theta);

	costheta = MUrCos(theta);
	sintheta = MUrSin(theta);

	fly_speed = M3cDegToRad * rotation_rate * radius;

	// we increment the velocity by fly_speed in the tangential direction to the spiral
	if (p_orientation == NULL) {
		// this is velocity cross upvector (0 1 0)
		MUmVector_Set(horiz_axis, -p_velocity->z, 0, p_velocity->x);
		MUmVector_Normalize(horiz_axis);

		MUrVector_CrossProduct(&horiz_axis, p_velocity, &vert_axis);
		MUmVector_Normalize(vert_axis);

		// calculate the tangential direction from the perpendicular axes
		p_velocity->x += - fly_speed * (sintheta * horiz_axis.x + costheta * vert_axis.x);
		p_velocity->y += - fly_speed * (sintheta * horiz_axis.y + costheta * vert_axis.y);
		p_velocity->z += - fly_speed * (sintheta * horiz_axis.z + costheta * vert_axis.z);
	} else {
		// calculate the tangential direction directly from orientation's X and Z axes
		p_velocity->x += - fly_speed * (sintheta * p_orientation->m[0][0] + costheta * p_orientation->m[2][0]);
		p_velocity->y += - fly_speed * (sintheta * p_orientation->m[0][1] + costheta * p_orientation->m[2][1]);
		p_velocity->z += - fly_speed * (sintheta * p_orientation->m[0][2] + costheta * p_orientation->m[2][2]);
	}

	inParticle->header.flags |= P3cParticleFlag_VelocityChanged;

	return UUcFalse;
}

// make a particle fly away from its spiral path. called once - translates spiral position into linear velocity
UUtBool P3iAction_KillBeyondPoint(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	P3tEnumAxis axis;
	float threshold, value;
	M3tPoint3D position;
	UUtBool returnval;

	P3mAssertEnumValue(inAction->action_value[0], P3cEnumType_Axis);
	P3mAssertFloatValue(inAction->action_value[1]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &threshold);
	if (!returnval)
		return UUcFalse;

	returnval = P3rGetRealPosition(inClass, inParticle, &position);
	if (returnval)
		return UUcTrue;

	axis = inAction->action_value[0].u.enum_const.val;
	switch(axis) {
		case P3cEnumAxis_PosX:
			value = threshold - position.x;
			break;

		case P3cEnumAxis_NegX:
			value = position.x - threshold;
			break;

		case P3cEnumAxis_PosY:
			value = threshold - position.y;
			break;

		case P3cEnumAxis_NegY:
			value = position.y - threshold;
			break;

		case P3cEnumAxis_PosZ:
			value = threshold - position.z;
			break;

		case P3cEnumAxis_NegZ:
			value = position.z - threshold;
			break;

		default:
			return UUcFalse;
	}

	if (value < 0) {
		// we have hit the threshold, die
		P3rKillParticle(inClass, inParticle);
		return UUcTrue;
	} else {
		// there is still room for us to move
		return UUcFalse;
	}
}

// the superball gun's trigger has just been released
//
// param 0: enum:			index of emitter
// param 1: float:			time threshold for contact-fuse
UUtBool P3iAction_SuperballTrigger(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	P3tParticleClass *ball_class;
	P3tParticle *ball_particle;
	P3tParticleEmitterData *emitter_data;
	float threshold_time;
	UUtUns32 ref, emitter_index;

	P3mAssertEnumValue(inAction->action_value[0], P3cEnumType_Emitter);
	emitter_index = inAction->action_value[0].u.enum_const.val;

	P3mAssertFloatValue(inAction->action_value[1]);
	if (!P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &threshold_time))
		return UUcFalse;

	if ((emitter_index < 0) || (emitter_index >= inClass->definition->num_emitters))
		return UUcFalse;

	emitter_data = ((P3tParticleEmitterData *) inParticle->vararray) + emitter_index;
	ref = emitter_data->last_particle;
	if (ref == P3cParticleReference_Null)
		return UUcFalse;

	P3mUnpackParticleReference(ref, ball_class, ball_particle);
	UUmAssert(ball_class == inClass->definition->emitter[emitter_index].emittedclass);
	if (ball_particle->header.self_ref != ref) {
		// the ball is already dead
		return UUcFalse;
	}

	if (inCurrentT >= emitter_data->time_last_emission + threshold_time) {
		// the trigger has been released after being held down for a while. explode the ball.
		return P3iPerformActionList(ball_class, ball_particle, P3cEvent_Explode,
					&ball_class->definition->eventlist[P3cEvent_Explode], inCurrentT, 0, UUcFalse);
	} else {
		// the trigger was just pulled for a very brief time. set the ball to be contact-fused.
		ball_particle->header.flags |= P3cParticleFlag_ExplodeOnContact;
		return UUcFalse;
	}
}

// stop the current action list if the quad that we hit was breakable... used for the mercury bow to punch
// through glass
UUtBool P3iAction_StopIfBreakable(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	AKtEnvironment *environment;

	if (!P3gCurrentCollision.collided)
		return UUcFalse;

	if (!P3gCurrentCollision.env_collision)
		return UUcFalse;

	environment = ONrGameState_GetEnvironment();
	if ((P3gEverythingBreakable) || (environment->gqGeneralArray->gqGeneral[P3gCurrentCollision.hit_gqindex].flags & AKcGQ_Flag_Breakable)) {
		*outStop = UUcTrue;
	}

	return UUcFalse;
}

// start playing an ambient sound
//
// param 0:	enum:		ambient sound ID
UUtBool P3iAction_AmbientSound(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	const SStAmbient *ambient_sound;

	// param 0 has already been turned into an ambient sound pointer
	ambient_sound = (const SStAmbient *) inAction->action_data;
	if (ambient_sound == NULL)
		return UUcFalse;

	// if there is already a sound being played from this particle, stop it
	if (inParticle->header.flags & P3cParticleFlag_PlayingAmbientSound) {
#if PERFORMANCE_TIMER
		UUrPerformanceTimer_Enter(P3g_ParticleSound);
#endif

#if PARTICLE_DEBUG_SOUND
		COrConsole_Printf("P3iAction_AmbientSound (stop existing %d) from 0x%08X", inParticle->header.current_sound, inParticle->header.self_ref);
#endif
		UUmAssert(inParticle->header.current_sound != SScInvalidID);
		OSrAmbient_Stop(inParticle->header.current_sound);
		inParticle->header.current_sound = SScInvalidID;
		inParticle->header.flags &= ~P3cParticleFlag_PlayingAmbientSound;

#if PERFORMANCE_TIMER
		UUrPerformanceTimer_Exit(P3g_ParticleSound);
#endif
	}

#if PARTICLE_DEBUG_SOUND
	COrConsole_Printf("P3iAction_AmbientSound (%s) from 0x%08X", ambient_sound->ambient_name, inParticle->header.self_ref);
#endif
	P3iAmbientSound_Update(inClass, inParticle, ambient_sound);

	if (inParticle->header.current_sound != SScInvalidID) {
		inParticle->header.flags |= P3cParticleFlag_PlayingAmbientSound;
	}

	return UUcFalse;
}

static UUtBool P3iAmbientSound_Update(P3tParticleClass *inClass, P3tParticle *inParticle, const SStAmbient *inAmbientSound)
{
	UUtBool is_dead;
	M3tMatrix3x3 *p_orientation;
	M3tVector3D position, *directionptr, direction, *velocityptr;
	
#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Enter(P3g_ParticleSound);
#endif

	P3mPerfTrack(inClass, P3cPerfEvent_SndAmbient);

	// find the particle's real position
	is_dead = P3rGetRealPosition(inClass, inParticle, &position);
	if (is_dead) {
#if PERFORMANCE_TIMER
		UUrPerformanceTimer_Exit(P3g_ParticleSound);
#endif
		return UUcTrue;
	}

	directionptr = NULL;
	p_orientation = P3rGetOrientationPtr(inClass, inParticle);
	if (p_orientation != NULL) {
		// sound is directed along Y axis of orientation
		MUrMatrix_GetCol((M3tMatrix4x3 *) p_orientation, 1, &direction);
		directionptr = &direction;
	}

	velocityptr = P3rGetVelocityPtr(inClass, inParticle);

	if (inAmbientSound) {
		// start a new ambient sound
		UUmAssert(inParticle->header.current_sound == SScInvalidID);
		inParticle->header.current_sound = OSrAmbient_Start(inAmbientSound, &position, directionptr, velocityptr, NULL, NULL);
	} else {
		UUmAssert(inParticle->header.current_sound != SScInvalidID);
		OSrAmbient_Update(inParticle->header.current_sound, &position, directionptr, velocityptr);
	}
#if PARTICLE_DEBUG_SOUND
	COrConsole_Printf("P3iAmbientSound_Update (%s | %d) from 0x%08X", (inAmbientSound == NULL) ? "update" : inAmbientSound->ambient_name,
						inParticle->header.current_sound, inParticle->header.self_ref);
#endif
	
#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Exit(P3g_ParticleSound);
#endif

	return UUcFalse;
}

// stop playing an ambient sound
UUtBool P3iAction_EndAmbientSound(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	SStPlayID sound_id = inParticle->header.current_sound;

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Enter(P3g_ParticleSound);
#endif

	if (sound_id != SScInvalidID) {
#if PARTICLE_DEBUG_SOUND
		COrConsole_Printf("P3iAction_EndAmbientSound (%d) from 0x%08X", sound_id, inParticle->header.self_ref);
#endif
		OSrAmbient_Stop(sound_id);
		inParticle->header.current_sound = SScInvalidID;
	}
	inParticle->header.flags &= ~P3cParticleFlag_PlayingAmbientSound;

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Exit(P3g_ParticleSound);
#endif

	return UUcFalse;
}

// play an impulse sound
//
// param 0:	enum:		impulse sound ID
UUtBool P3iAction_ImpulseSound(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	UUtBool is_dead;
	M3tMatrix3x3 *p_orientation;
	M3tVector3D position, *directionptr, direction, *velocityptr;
	SStImpulse *impulse_sound;
	
#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Enter(P3g_ParticleSound);
#endif

	// param 0 has already been turned into an impulse sound pointer
	impulse_sound = (SStImpulse *) inAction->action_data;
	if (impulse_sound == NULL) {
#if PERFORMANCE_TIMER
		UUrPerformanceTimer_Exit(P3g_ParticleSound);
#endif
		return UUcFalse;
	}
	
	// find the particle's real position
	is_dead = P3rGetRealPosition(inClass, inParticle, &position);
	if (is_dead) {
#if PERFORMANCE_TIMER
		UUrPerformanceTimer_Exit(P3g_ParticleSound);
#endif
		return UUcTrue;
	}

	P3mPerfTrack(inClass, P3cPerfEvent_SndImpulse);

	directionptr = NULL;
	p_orientation = P3rGetOrientationPtr(inClass, inParticle);
	if (p_orientation != NULL) {
		// sound is directed along Y axis of orientation
		MUrMatrix_GetCol((M3tMatrix4x3 *) p_orientation, 1, &direction);
		directionptr = &direction;
	}

	velocityptr = P3rGetVelocityPtr(inClass, inParticle);

	OSrImpulse_Play(impulse_sound, &position, directionptr, velocityptr, NULL);
	
#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Exit(P3g_ParticleSound);
#endif

	return UUcFalse;
}

// find a new attractor
//
// param 0: float:		seconds between checks
UUtBool P3iAction_FindAttractor(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	UUtBool potential_new_attractor;
	float delay_secs;
	UUtUns32 delay_ticks;

	potential_new_attractor = ((inParticle->header.flags & P3cParticleFlag_HasAttractor) == 0);

	P3mAssertFloatValue(inAction->action_value[0]);
	if (!P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &delay_secs))
		return UUcFalse;

	delay_ticks = MUrUnsignedSmallFloat_To_Uns_Round(delay_secs * UUcFramesPerSecond);
	P3rFindAttractor(inClass, inParticle, delay_ticks);

	// if we didn't have an attractor before, and we do now, send the NewAttractor event
	potential_new_attractor = potential_new_attractor && ((inParticle->header.flags & P3cParticleFlag_HasAttractor) > 0);
	if (potential_new_attractor) {
		return P3rSendEvent(inClass, inParticle, P3cEvent_NewAttractor, inCurrentT);
	} else {
		return UUcFalse;
	}
}

// apply gravity towards our attractor
//
// param 0: float:		gravity fraction
// param 1: float:		max g
// param 2: bool:		horizontal only
UUtBool P3iAction_AttractGravity(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	P3tAttractorData *attractor;
	M3tPoint3D our_pt, attractor_pt;
	M3tVector3D *velocity, delta_pos;
	float gravity_fraction, max_g;
	UUtBool found, returnval;
	P3tEnumBool horiz_only;

	if ((inParticle->header.flags & P3cParticleFlag_HasAttractor) == 0) {
		return UUcFalse;
	}

	attractor = P3rGetAttractorDataPtr(inClass, inParticle);
	UUmAssertReadPtr(attractor, sizeof(*attractor));

	found = P3rDecodeAttractorIndex(attractor->index, UUcFalse, &attractor_pt, NULL);
	if (!found) {
		return UUcFalse;
	}

	velocity = P3rGetVelocityPtr(inClass, inParticle);
	if (velocity == NULL) {
		return UUcFalse;
	}

	P3mAssertFloatValue(inAction->action_value[0]);
	P3mAssertFloatValue(inAction->action_value[1]);
	P3mAssertEnumValue(inAction->action_value[2], P3cEnumType_Bool);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &gravity_fraction);
	if (!returnval)
		return UUcFalse;
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &max_g);
	if (!returnval)
		return UUcFalse;
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[2], (void *) &horiz_only);
	if (!returnval)
		return UUcFalse;

	returnval = P3rGetRealPosition(inClass, inParticle, &our_pt);
	if (returnval) {
		// we are dead
		return UUcTrue;
	}

	MUmVector_Subtract(delta_pos, attractor_pt, our_pt);
	gravity_fraction *= P3gGravityDistSq / MUmVector_GetLengthSquared(delta_pos);
	gravity_fraction = UUmMin(gravity_fraction, max_g);

	if (horiz_only) {
		delta_pos.y = 0;
	}

	MUmVector_ScaleIncrement(*velocity, gravity_fraction, delta_pos);

	inParticle->header.flags |= P3cParticleFlag_VelocityChanged;

	return UUcFalse;
}

// home towards our attractor
//
// param 0: float:		maximum rotation speed
// param 1: bool:		predict target's position
// param 2: bool:		horizontal only
UUtBool P3iAction_AttractHoming(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	P3tAttractorData *attractor;
	P3tEnumBool predict_pos, horiz_only;
	M3tQuaternion rotate_q;
	M3tPoint3D our_pt, attractor_pt;
	M3tVector3D *velocity, orig_vel, delta_pos, target_vel;
	M3tMatrix3x3 *orientation;
	M3tMatrix4x3 rotate_matrix;
	float turn_speed, vel_mag, dist_sq, dist, qx, qy, qz, q_angle, hit_time;
	UUtBool found, returnval;

	if ((inParticle->header.flags & P3cParticleFlag_HasAttractor) == 0) {
		return UUcFalse;
	}

	// work out whereabouts we are relative to the attractor
	returnval = P3rGetRealPosition(inClass, inParticle, &our_pt);
	if (returnval) {
		// we are dead
		return UUcTrue;
	}

	// get our homing parameters
	P3mAssertFloatValue(inAction->action_value[0]);
	P3mAssertEnumValue(inAction->action_value[1], P3cEnumType_Bool);
	P3mAssertEnumValue(inAction->action_value[2], P3cEnumType_Bool);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &turn_speed);
	if (!returnval)
		return UUcFalse;
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &predict_pos);
	if (!returnval)
		return UUcFalse;
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[2], (void *) &horiz_only);
	if (!returnval)
		return UUcFalse;


	attractor = P3rGetAttractorDataPtr(inClass, inParticle);
	UUmAssertReadPtr(attractor, sizeof(*attractor));

	found = P3rDecodeAttractorIndex(attractor->index, (UUtBool) predict_pos, &attractor_pt, &target_vel);
	if (!found) {
		return UUcFalse;
	}

	MUmVector_Subtract(delta_pos, attractor_pt, our_pt);
	dist_sq = MUmVector_GetLengthSquared(delta_pos);
	if (dist_sq < 1e-04f) {
		// we are actually at the attractor - don't home
		return UUcFalse;
	}

	// work out what direction we are currently moving in
	orientation = P3rGetOrientationPtr(inClass, inParticle);
	velocity = P3rGetVelocityPtr(inClass, inParticle);
	if (velocity == NULL) {
		return UUcFalse;
	}

	orig_vel = *velocity;
	vel_mag = MUmVector_GetLength(orig_vel);
	dist = MUrSqrt(dist_sq);
	if (vel_mag < 1e-04f) {
		// we have no velocity to speak of. give us a slight velocity either along our
		// orientation, or towards the attractor.
		if (orientation == NULL) {
			MUmVector_ScaleCopy(*velocity, 1e-03f / MUrSqrt(dist_sq), delta_pos);
		} else {
			*velocity = MUrMatrix_GetYAxis((M3tMatrix4x3 *) orientation);
			MUmVector_Scale(*velocity, 1e-03f);
		}
	} else if (predict_pos) {
		// calculate approximate time to strike the target
		hit_time = dist / vel_mag;
		hit_time = UUmMin(hit_time, 1.0f);		// don't predict position plus more than one second

		MUmVector_ScaleIncrement(delta_pos, hit_time, target_vel);
	}

	// calculate a quaternion that rotates from our current velocity towards the attractor
	MUrQuat_SetFromTwoVectors(velocity, &delta_pos, &rotate_q);
	MUrQuat_GetValue(&rotate_q, &qx, &qy, &qz, &q_angle);

	// convert turn speed from degrees per second into radians for this update
	turn_speed *= M3cDegToRad * inDeltaT;

	if (q_angle > turn_speed) {
		MUrQuat_SetValue(&rotate_q, qx, qy, qz, turn_speed);
	}
	
	MUrQuatToMatrix(&rotate_q, &rotate_matrix);
	MUrMatrix3x3_MultiplyPoint(velocity, (M3tMatrix3x3 *) &rotate_matrix, velocity);

	if (horiz_only) {
		velocity->y = orig_vel.y;
	}

	inParticle->header.flags |= P3cParticleFlag_VelocityChanged;

	return UUcFalse;
}

// move towards our attractor as if on a spring
//
// param 0: float:		acceleration constant
// param 1: float:		maximum acceleration
// param 2: float:		desired distance
UUtBool P3iAction_AttractSpring(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	P3tAttractorData *attractor;
	M3tPoint3D our_pt, attractor_pt;
	M3tVector3D *velocity, delta_pos, attractor_dir;
	float accel, accel_ratio, max_accel, desired_dist, cur_dist;
	UUtBool returnval, found;

	if ((inParticle->header.flags & P3cParticleFlag_HasAttractor) == 0) {
		return UUcFalse;
	}

	// work out whereabouts we are relative to the attractor
	returnval = P3rGetRealPosition(inClass, inParticle, &our_pt);
	if (returnval) {
		// we are dead
		return UUcTrue;
	}

	// work out what direction we are currently moving in
	velocity = P3rGetVelocityPtr(inClass, inParticle);
	if (velocity == NULL) {
		return UUcFalse;
	}

	// get our attraction parameters
	P3mAssertFloatValue(inAction->action_value[0]);
	P3mAssertFloatValue(inAction->action_value[1]);
	P3mAssertFloatValue(inAction->action_value[2]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &accel_ratio);
	if (!returnval)
		return UUcFalse;
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &max_accel);
	if (!returnval)
		return UUcFalse;
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[2], (void *) &desired_dist);
	if (!returnval)
		return UUcFalse;

	attractor = P3rGetAttractorDataPtr(inClass, inParticle);
	UUmAssertReadPtr(attractor, sizeof(*attractor));

	found = P3rDecodeAttractorIndex(attractor->index, UUcFalse, &attractor_pt, NULL);
	if (!found) {
		return UUcFalse;
	}

	MUmVector_Subtract(delta_pos, attractor_pt, our_pt);
	cur_dist = MUmVector_GetLength(delta_pos);
	if (cur_dist < 1e-03f) {
		if (desired_dist < cur_dist) {
			// we are at the attractor, do nothing
			return UUcFalse;
		}

		// pick a random direction to believe that the attractor is in
		P3rGetRandomUnitVector3D(&attractor_dir);
	} else {
		// normalize the attractor direction
		MUmVector_ScaleCopy(attractor_dir, 1.0f / cur_dist, delta_pos);
	}

	// calculate the acceleration amount towards the attractor
	accel = (cur_dist - desired_dist) * accel_ratio * inDeltaT;
	accel = UUmMin(accel, max_accel);

	// apply this acceleration
	MUmVector_ScaleIncrement(*velocity, accel, attractor_dir);
	inParticle->header.flags |= P3cParticleFlag_VelocityChanged;

	return UUcFalse;
}

// null out a particle's link
UUtBool P3iAction_StopLink(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	UUtBool is_dead;
	M3tPoint3D *positionptr;

	if (inParticle->header.user_ref == P3cParticleReference_Null) {
		// no link to stop
		return UUcFalse;
	}

	if (inClass->definition->flags2 & P3cParticleClassFlag2_LockToLinkedParticle) {
		// get our position, one last time
		positionptr = P3rGetPositionPtr(inClass, inParticle, &is_dead);
		if (is_dead) {
			return UUcTrue;
		}

		MUmVector_Copy(inParticle->header.position, *positionptr);
	}

	// break our link
	inParticle->header.user_ref = P3cParticleReference_Null;
	return P3rSendEvent(inClass, inParticle, P3cEvent_BrokenLink, inCurrentT);
}

// check to see that a particle's link is still valid
UUtBool P3iAction_CheckLink(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	P3tParticleClass *classptr;
	P3tParticle *particleptr;
	UUtUns32 ref;

	ref = inParticle->header.user_ref;
	if (ref == P3cParticleReference_Null) {
		// no link to check
		return UUcFalse;
	}

	P3mUnpackParticleReference(ref, classptr, particleptr);
	if ((particleptr->header.self_ref == ref) && ((particleptr->header.flags & P3cParticleFlag_BreakLinksToMe) == 0)) {
		// our link is still valid
		return UUcFalse;
	} else {
		// our link points to an incorrect particle now, break it
		return P3iAction_StopLink(inClass, inParticle, inAction, inVariables, inCurrentT, inDeltaT, outStop);
	}
}

// tell all particles that link to this particle that they should break their links
UUtBool P3iAction_BreakLink(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	inParticle->header.flags |= P3cParticleFlag_BreakLinksToMe;
	return UUcFalse;
}

// make the particle avoid walls
//
// param 0: float:		sense distance
// param 1: float:		turning speed
UUtBool P3iAction_AvoidWalls(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	M3tQuaternion rotate_q;
	M3tPoint3D our_pt;
	M3tVector3D *velocity, delta_pos, plane_normal, rotate_ax;
	M3tMatrix3x3 *orientation;
	M3tMatrix4x3 rotate_matrix;
	float sense_dist, turn_speed, vel_mag, plane_dist, turn_decay;
	float *axis_x, *axis_y, *axis_z, *rotate_speed, *t_until_check;
	AKtEnvironment *env;
	AKtOctNodeIndexCache *p_envcache;
	AKtGQ_Collision *gq_collision;
	UUtBool returnval;

	// get pointers to our rotation-data cache storage
	axis_x = (float *) inVariables[0];
	axis_y = (float *) inVariables[1];
	axis_z = (float *) inVariables[2];
	rotate_speed = (float *) inVariables[3];
	t_until_check = (float *) inVariables[4];

	if ((axis_x == NULL) || (axis_y == NULL) || (axis_z == NULL) || (rotate_speed == NULL) || (t_until_check == NULL))
		return UUcFalse;

	if (*t_until_check > inDeltaT) {
		// we don't check this timestep
		*t_until_check -= inDeltaT;
		return UUcFalse;
	} else {
		// we will check this timestep, reset our time between checks
		*t_until_check = P3cAvoidWalls_CheckCollisionMinInterval;
	}

	// work out whereabouts we are and how we're travelling
	returnval = P3rGetRealPosition(inClass, inParticle, &our_pt);
	if (returnval) {
		// we are dead
		return UUcTrue;
	}

	velocity = P3rGetVelocityPtr(inClass, inParticle);
	if (velocity == NULL) {
		return UUcFalse;
	}

	// get our evasion parameters
	P3mAssertFloatValue(inAction->action_value[0]);
	P3mAssertFloatValue(inAction->action_value[1]);
	P3mAssertFloatValue(inAction->action_value[2]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &sense_dist);
	if (!returnval)
		return UUcFalse;

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &turn_speed);
	if (!returnval)
		return UUcFalse;
	turn_speed *= M3cDegToRad;

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[2], (void *) &turn_decay);
	if (!returnval)
		return UUcFalse;
	turn_decay *= M3cDegToRad;

	// check for collisions
	env = ONrGameState_GetEnvironment();
	MUmVector_Copy(delta_pos, *velocity);
	vel_mag = MUmVector_GetLength(delta_pos);
	if (vel_mag < 1e-03f) {
		// we aren't moving
		return UUcFalse;
	} else {
		MUmVector_Scale(delta_pos, sense_dist / vel_mag);
	}

	p_envcache = P3rGetEnvironmentCache(inClass, inParticle);
	if (p_envcache == NULL) {
		AKrCollision_Point(env, &our_pt, &delta_pos, AKcGQ_Flag_Obj_Col_Skip, 1);
	} else {
		AKrCollision_Point_SpatialCache(env, &our_pt, &delta_pos, AKcGQ_Flag_Obj_Col_Skip, 1,
							p_envcache, NULL);
	}

	if (AKgNumCollisions > 0) {
		// get the normal of the quad that we hit
		gq_collision = env->gqCollisionArray->gqCollision + AKgCollisionList[0].gqIndex;
		AKmPlaneEqu_GetComponents(gq_collision->planeEquIndex, env->planeArray->planes,
								plane_normal.x, plane_normal.y, plane_normal.z, plane_dist);

		// get the direction in which we have to turn in order not to hit the wall
		MUrVector_CrossProduct(&plane_normal, &delta_pos, &rotate_ax);
		if (MUrVector_Normalize_ZeroTest(&rotate_ax)) {
			// choose a horizontal rotation axis
			*axis_x =   delta_pos.z / sense_dist;
			*axis_y = 0;
			*axis_z = -(delta_pos.x / sense_dist);
		} else {
			*axis_x = rotate_ax.x;
			*axis_y = rotate_ax.y;
			*axis_z = rotate_ax.z;
		}

		// turn away at maximum speed
		*rotate_speed = turn_speed;
	} else {
		// use the previous cached value of rotate_speed, if we are currently evading a wall
		*rotate_speed -= turn_decay * inDeltaT;
		if (*rotate_speed < 1e-03f) {
			*rotate_speed = 0;
			return UUcFalse;
		}
	}

	// form a quaternion from the calculated rotation and apply it
	MUrQuat_SetValue(&rotate_q, *axis_x, *axis_y, *axis_z, *rotate_speed * inDeltaT);
	MUrQuatToMatrix(&rotate_q, &rotate_matrix);
	MUrMatrix3x3_MultiplyPoint(velocity, (M3tMatrix3x3 *) &rotate_matrix, velocity);

	orientation = P3rGetOrientationPtr(inClass, inParticle);
	if (orientation != NULL) {
		MUrMatrix3x3_Multiply((M3tMatrix3x3 *) &rotate_matrix, orientation, orientation);
	}

	inParticle->header.flags |= P3cParticleFlag_VelocityChanged;

	return UUcFalse;
}

UUtBool P3iAction_RandomSwirl(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	M3tQuaternion rotate_q;
	M3tVector3D *velocity, rotate_ax;
	M3tMatrix3x3 *orientation;
	M3tMatrix4x3 rotate_matrix;
	float swirl_baserate, swirl_deltarate, *swirl_angle, swirl_speed, sintheta, costheta;
	UUtBool returnval;

	velocity = P3rGetVelocityPtr(inClass, inParticle);
	orientation = P3rGetOrientationPtr(inClass, inParticle);
	if (orientation == NULL) {
		return UUcFalse;
	}

	// get our rotation parameters
	P3mAssertFloatValue(inAction->action_value[0]);
	P3mAssertFloatValue(inAction->action_value[1]);
	P3mAssertFloatValue(inAction->action_value[2]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &swirl_baserate);
	if (!returnval)
		return UUcFalse;
	swirl_baserate *= M3cDegToRad * inDeltaT;

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &swirl_deltarate);
	if (!returnval)
		return UUcFalse;
	swirl_deltarate *= M3cDegToRad * inDeltaT;

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[2], (void *) &swirl_speed);
	if (!returnval)
		return UUcFalse;
	swirl_speed *= M3cDegToRad * inDeltaT;

	// get pointers to our rotation-data cache storage
	swirl_angle = (float *) inVariables[0];
	if (swirl_angle == NULL)
		return UUcFalse;

	// calculate how our rotation changes and form a rotation axis
	*swirl_angle += (swirl_baserate + swirl_deltarate);
	UUmTrig_Clip(*swirl_angle);
	costheta = MUrCos(*swirl_angle);		sintheta = MUrSin(*swirl_angle);

	rotate_ax.x = costheta * orientation->m[0][0] + sintheta * orientation->m[2][0];
	rotate_ax.y = costheta * orientation->m[0][1] + sintheta * orientation->m[2][1];
	rotate_ax.z = costheta * orientation->m[0][2] + sintheta * orientation->m[2][2];

	MUrQuat_SetValue(&rotate_q, rotate_ax.x, rotate_ax.y, rotate_ax.z, swirl_speed);
	MUrQuatToMatrix(&rotate_q, &rotate_matrix);

	if (velocity != NULL) {
		MUrMatrix3x3_MultiplyPoint(velocity, (M3tMatrix3x3 *) &rotate_matrix, velocity);
	}
	MUrMatrix3x3_Multiply((M3tMatrix3x3 *) &rotate_matrix, orientation, orientation);

	inParticle->header.flags |= P3cParticleFlag_VelocityChanged;

	return UUcFalse;
}

UUtBool P3iAction_FloatAbovePlayer(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	float height;
	UUtBool returnval;
	ONtCharacter *player_character;
	
	P3mAssertFloatValue(inAction->action_value[0]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &height);
	if (!returnval)
		return UUcFalse;

	player_character = ONrGameState_GetPlayerCharacter();
	if (player_character == NULL) {
		return UUcFalse;
	}

	MUmVector_Copy(inParticle->header.position, player_character->actual_position);
	inParticle->header.position.y += height;

	return UUcFalse;
}

UUtBool P3iAction_StopIfSlow(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	float speed, cur_speed;
	UUtBool returnval;
	M3tVector3D *velocity;
	
	P3mAssertFloatValue(inAction->action_value[0]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &speed);
	if (!returnval)
		return UUcFalse;

	velocity = P3rGetVelocityPtr(inClass, inParticle);
	if (velocity == NULL) {
		cur_speed = 0;
	} else {
		cur_speed = MUmVector_GetLengthSquared(*velocity);
	}

	if (cur_speed < UUmSQR(speed)) {
		*outStop = UUcTrue;
	}

	return UUcFalse;
}

UUtBool P3iAction_SuperParticle(P3tParticleClass *inClass, P3tParticle *inParticle,
								P3tActionInstance *inAction, void **inVariables, float inCurrentT, float inDeltaT, UUtBool *outStop)
{
	float *variable, base_value, delta_value, min_value, max_value, super_amount;
	ONtCharacter *owner_character;
	P3tOwner *p_owner;
	UUtBool returnval;
	UUtUns32 owner_type;

	// get our rotation parameters
	P3mAssertFloatValue(inAction->action_value[0]);
	P3mAssertFloatValue(inAction->action_value[1]);
	P3mAssertFloatValue(inAction->action_value[2]);
	P3mAssertFloatValue(inAction->action_value[3]);
	returnval = P3iCalculateValue(inParticle, &inAction->action_value[0], (void *) &base_value);
	if (!returnval)
		return UUcFalse;

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[1], (void *) &delta_value);
	if (!returnval)
		return UUcFalse;

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[2], (void *) &min_value);
	if (!returnval)
		return UUcFalse;

	returnval = P3iCalculateValue(inParticle, &inAction->action_value[3], (void *) &max_value);
	if (!returnval)
		return UUcFalse;

	// get a pointer to our variable to set
	variable = (float *) inVariables[0];
	if (variable == NULL) {
		return UUcFalse;
	}

	// calculate our owner's 'super amount'
	super_amount = 0.0f;
	p_owner = P3rGetOwnerPtr(inClass, inParticle);
	if (p_owner != NULL) {
		owner_type = (*p_owner & WPcDamageOwner_TypeMask);
		if ((owner_type == WPcDamageOwner_CharacterWeapon) || (owner_type == WPcDamageOwner_CharacterMelee)) {
			owner_character = WPrOwner_GetCharacter(*p_owner);
			if (owner_character != NULL) {
				super_amount = owner_character->super_amount;
			}
		}
	}

	*variable = UUmPin(base_value + delta_value * super_amount, min_value, max_value);
	return UUcFalse;
}
