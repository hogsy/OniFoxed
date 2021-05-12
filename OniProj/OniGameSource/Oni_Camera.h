#pragma once

/*
	Oni_Camera.h
	
	Header file for camera AI stuff
	
	Author: Quinn Dunki
	c1998 Bungie
*/

#ifndef ONI_CAMERA_H
#define ONI_CAMERA_H

#include "BFW_Motoko.h"
#include "BFW_Object_Templates.h"
#include "BFW_Physics.h"
#include "Oni_GameState.h"

typedef struct CAtViewData
{
	M3tPoint3D	location;
	M3tPoint3D	viewVector;
	M3tPoint3D	upVector;
} CAtViewData;

typedef struct CAtOrbitData
{
	float			current_angle;		
	float			stop_angle;
	float			speed;
} CAtOrbitData;

typedef struct CAtFollowData
{
	M3tPoint3D		focus;
	float			canter;
	float			distance;
	UUtBool			rigid;
} CAtFollowData;

typedef struct CAtAnimateData
{
	OBtAnimationContext animation;
} CAtAnimateData;

typedef struct CAtManualData
{
	UUtUns32 useMouseLook;
} CAtManualData;

typedef struct CAtInterpolateData
{
	CAtViewData		initial;
	CAtViewData		ending;

	UUtUns32		currentFrame;
	UUtUns32		endFrame;
} CAtInterpolateData;

typedef union CAtModeData
{
	CAtOrbitData		orbit;
	CAtFollowData		follow;
	CAtAnimateData		animate;
	CAtManualData		manual;
	CAtInterpolateData	interpolate;
} CAtModeData;

typedef enum CAtCameraMode
{
	CAcMode_Orbit,
	CAcMode_Follow,
	CAcMode_Animate,
	CAcMode_Manual,
	CAcMode_Interpolate
} CAtCameraMode;

typedef struct
{
	CAtCameraMode	mode;
	CAtModeData		modeData;

	ONtCharacter	*star;			// The character we're looking at

	CAtViewData		viewData;

	float			mainCharacterAlpha;

	UUtBool			is_busy;
} CAtCamera;

extern CAtCamera	CAgCamera;

UUtBool CArIsBusy(void);

CAtCameraMode CArGetMode(void);
void CArGetMatrix(const CAtCamera *inCamera, M3tMatrix4x3 *outMatrix);
void CArFollow_Enter(void);
void CArAnimate_Enter(OBtAnimation *inAnimation);
void CArOrbit_Enter(float inSpeed, float inStopAngle); 
void CArManual_Enter(void);
void CArManual_LookAt(M3tPoint3D *inLocation, float	inDistance);
void CArInterpolate_Enter_Matrix(const M3tMatrix4x3 *inMatrix, UUtUns32 inNumFrames);
void CArInterpolate_Enter_ViewData(CAtViewData *inViewData, UUtUns32 inNumFrames);
void CArInterpolate_Enter_Detached(void);

M3tVector3D CArBuildUpVector(const M3tVector3D *inViewVector);

void CArUpdate(
	UUtUns32 ticks, 
	UUtUns32 numActions,
	const struct LItAction *actionBuffer);

UUtError CArLevelBegin(
	void);

void CArLevelEnd(
	void);
	
UUtError CArInitialize(
	void);

void CArTerminate(
	void);

static UUcInline M3tPoint3D CArGetLocation(void)
{
	return CAgCamera.viewData.location;
}

static UUcInline M3tPoint3D CArGetFacing(void)
{
	return CAgCamera.viewData.viewVector;
}
	
void CArEnableJello(UUtBool inJello);

#endif