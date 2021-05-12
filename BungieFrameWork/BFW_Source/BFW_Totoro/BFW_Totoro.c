/*
	FILE:	BFW_Totoro.c
	
	AUTHOR:	Michael Evans
	
	CREATED: Sept 25, 1997
	
	PURPOSE: animation engine
	
	Copyright 1997

*/

/* 

  optimzation notes

  - MacOS: handle byte swapping faster in decompress / preprocess it out

*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//#include <math.h>
#include "bfw_math.h"

#include "BFW.h"
#include "BFW_TemplateManager.h"
#include "BFW_Motoko.h"
#include "BFW_Totoro.h"
#include "BFW_Totoro_Private.h"
#include "BFW_MathLib.h"
#include "BFW_BitVector.h"
#include "BFW_Console.h"
#include "BFW_Timer.h"
#include "EulerAngles.h"

UUtUns16 TRgLevelNumber;

#if PERFORMANCE_TIMER
UUtPerformanceTimer *TRg_SetAnimation_Timer = NULL;
UUtPerformanceTimer *TRg_BinarySearch_Timer = NULL;
#endif

TMtPrivateData*	TRgAnimationCollection_PrivateData = NULL;
TMtPrivateData*	TRgAnimation_PrivateData = NULL;
TMtPrivateData*	TRgBody_PrivateData = NULL;
static FILE *gLookupStream = NULL;


#define ANIM_PRINT_TABLE 0

static UUtError
TRrTemplateHandler_AnimationCollection(
	TMtTemplateProc_Message	inMessage,
	void*					inDataPtr,
	void*					inPrivateData);

static UUtError
TRrTemplateHandler_Animation(
	TMtTemplateProc_Message	inMessage,
	void*					inDataPtr,
	void*					inPrivateData);

static UUtError
TRrTemplateHandler_Body(
	TMtTemplateProc_Message	inMessage,
	void*					inDataPtr,
	void*					inPrivateData);

M3tQuaternion TRgQuatArrayA[TRcMaxParts];
M3tQuaternion TRgQuatArrayB[TRcMaxParts];
M3tQuaternion TRgQuatArrayC[TRcMaxParts];
M3tQuaternion TRgQuatArrayD[TRcMaxParts];
M3tQuaternion TRgQuatArrayE[TRcMaxParts];

M3tQuaternion TRgQuatArrayPrivate_Aiming_1[TRcMaxParts];
M3tQuaternion TRgQuatArrayPrivate_Aiming_2[TRcMaxParts];
M3tQuaternion TRgQuatArrayPrivate_Aiming_3[TRcMaxParts];
M3tQuaternion TRgQuatArrayPrivate_Aiming_4[TRcMaxParts];

M3tQuaternion TRgQuatArrayPrivateA[TRcMaxParts];
M3tQuaternion TRgQuatAraryPrivate_SetAnimation[TRcMaxParts];

const char *TRcIntersectRejectionName[TRcRejection_Max] =
	{"no-attack", "distance", "angle", "above-head", "below-feet", "behind"};

const char *TRcDirectionName[TRcDirection_Max] =
	{"none", "forwards", "back", "left", "right", "360"};

static void TRrAnimation_Prepare(TRtAnimation *animation);
static const TRtAttack *TRrAnimation_GetAttack(const TRtAnimation *inAnimation, TRtAnimTime inTime, UUtUns32 inPartIndex);

//
// Danger, Danger, these tables aren't really sin(pheta) and cos(pheta), 
// but rather sin(pheta-PI) cos(pheta-PI). This matches the data file
// format.
//

static float anim_sintable[256]={
-1.22515E-16f,
-0.01231966f,
-0.024637449f,
-0.036951499f,
-0.049259941f,
-0.061560906f,
-0.073852527f,
-0.086132939f,
-0.098400278f,
-0.110652682f,
-0.122888291f,
-0.135105247f,
-0.147301698f,
-0.159475791f,
-0.171625679f,
-0.183749518f,
-0.195845467f,
-0.207911691f,
-0.219946358f,
-0.231947641f,
-0.24391372f,
-0.255842778f,
-0.267733003f,
-0.279582593f,
-0.291389747f,
-0.303152674f,
-0.314869589f,
-0.326538713f,
-0.338158275f,
-0.349726511f,
-0.361241666f,
-0.372701992f,
-0.384105749f,
-0.395451207f,
-0.406736643f,
-0.417960345f,
-0.429120609f,
-0.440215741f,
-0.451244057f,
-0.462203884f,
-0.473093557f,
-0.483911424f,
-0.494655843f,
-0.505325184f,
-0.515917826f,
-0.526432163f,
-0.536866598f,
-0.547219547f,
-0.557489439f,
-0.567674716f,
-0.577773831f,
-0.587785252f,
-0.597707459f,
-0.607538946f,
-0.617278221f,
-0.626923806f,
-0.636474236f,
-0.645928062f,
-0.65528385f,
-0.664540179f,
-0.673695644f,
-0.682748855f,
-0.691698439f,
-0.700543038f,
-0.709281308f,
-0.717911923f,
-0.726433574f,
-0.734844967f,
-0.743144825f,
-0.75133189f,
-0.759404917f,
-0.767362681f,
-0.775203976f,
-0.78292761f,
-0.790532412f,
-0.798017227f,
-0.805380919f,
-0.812622371f,
-0.819740483f,
-0.826734175f,
-0.833602385f,
-0.840344072f,
-0.846958211f,
-0.853443799f,
-0.859799851f,
-0.866025404f,
-0.872119511f,
-0.878081248f,
-0.88390971f,
-0.889604013f,
-0.895163291f,
-0.900586702f,
-0.905873422f,
-0.911022649f,
-0.916033601f,
-0.920905518f,
-0.92563766f,
-0.930229309f,
-0.934679767f,
-0.938988361f,
-0.943154434f,
-0.947177357f,
-0.951056516f,
-0.954791325f,
-0.958381215f,
-0.961825643f,
-0.965124085f,
-0.968276041f,
-0.971281032f,
-0.974138602f,
-0.976848318f,
-0.979409768f,
-0.981822563f,
-0.984086337f,
-0.986200747f,
-0.988165472f,
-0.989980213f,
-0.991644696f,
-0.993158666f,
-0.994521895f,
-0.995734176f,
-0.996795325f,
-0.99770518f,
-0.998463604f,
-0.999070481f,
-0.99952572f,
-0.99982925f,
-0.999981027f,
-0.999981027f,
-0.99982925f,
-0.99952572f,
-0.999070481f,
-0.998463604f,
-0.99770518f,
-0.996795325f,
-0.995734176f,
-0.994521895f,
-0.993158666f,
-0.991644696f,
-0.989980213f,
-0.988165472f,
-0.986200747f,
-0.984086337f,
-0.981822563f,
-0.979409768f,
-0.976848318f,
-0.974138602f,
-0.971281032f,
-0.968276041f,
-0.965124085f,
-0.961825643f,
-0.958381215f,
-0.954791325f,
-0.951056516f,
-0.947177357f,
-0.943154434f,
-0.938988361f,
-0.934679767f,
-0.930229309f,
-0.92563766f,
-0.920905518f,
-0.916033601f,
-0.911022649f,
-0.905873422f,
-0.900586702f,
-0.895163291f,
-0.889604013f,
-0.88390971f,
-0.878081248f,
-0.872119511f,
-0.866025404f,
-0.859799851f,
-0.853443799f,
-0.846958211f,
-0.840344072f,
-0.833602385f,
-0.826734175f,
-0.819740483f,
-0.812622371f,
-0.805380919f,
-0.798017227f,
-0.790532412f,
-0.78292761f,
-0.775203976f,
-0.767362681f,
-0.759404917f,
-0.75133189f,
-0.743144825f,
-0.734844967f,
-0.726433574f,
-0.717911923f,
-0.709281308f,
-0.700543038f,
-0.691698439f,
-0.682748855f,
-0.673695644f,
-0.664540179f,
-0.65528385f,
-0.645928062f,
-0.636474236f,
-0.626923806f,
-0.617278221f,
-0.607538946f,
-0.597707459f,
-0.587785252f,
-0.577773831f,
-0.567674716f,
-0.557489439f,
-0.547219547f,
-0.536866598f,
-0.526432163f,
-0.515917826f,
-0.505325184f,
-0.494655843f,
-0.483911424f,
-0.473093557f,
-0.462203884f,
-0.451244057f,
-0.440215741f,
-0.429120609f,
-0.417960345f,
-0.406736643f,
-0.395451207f,
-0.384105749f,
-0.372701992f,
-0.361241666f,
-0.349726511f,
-0.338158275f,
-0.326538713f,
-0.314869589f,
-0.303152674f,
-0.291389747f,
-0.279582593f,
-0.267733003f,
-0.255842778f,
-0.24391372f,
-0.231947641f,
-0.219946358f,
-0.207911691f,
-0.195845467f,
-0.183749518f,
-0.171625679f,
-0.159475791f,
-0.147301698f,
-0.135105247f,
-0.122888291f,
-0.110652682f,
-0.098400278f,
-0.086132939f,
-0.073852527f,
-0.061560906f,
-0.049259941f,
-0.036951499f,
-0.024637449f,
-0.01231966f,
0.f
};

static float anim_costable[256]={
-1.f,
-0.99992411f,
-0.999696452f,
-0.99931706f,
-0.998785992f,
-0.998103329f,
-0.997269173f,
-0.996283653f,
-0.995146916f,
-0.993859137f,
-0.99242051f,
-0.990831253f,
-0.989091608f,
-0.98720184f,
-0.985162233f,
-0.9829731f,
-0.98063477f,
-0.978147601f,
-0.975511968f,
-0.972728272f,
-0.969796936f,
-0.966718404f,
-0.963493144f,
-0.960121645f,
-0.95660442f,
-0.952942f,
-0.949134944f,
-0.945183828f,
-0.941089253f,
-0.936851839f,
-0.932472229f,
-0.92795109f,
-0.923289106f,
-0.918486986f,
-0.913545458f,
-0.908465272f,
-0.903247199f,
-0.897892032f,
-0.892400583f,
-0.886773686f,
-0.881012194f,
-0.875116983f,
-0.869088946f,
-0.862929f,
-0.856638078f,
-0.850217136f,
-0.843667148f,
-0.836989108f,
-0.830184031f,
-0.823252948f,
-0.816196912f,
-0.809016994f,
-0.801714284f,
-0.79428989f,
-0.786744938f,
-0.779080575f,
-0.771297962f,
-0.763398283f,
-0.755382735f,
-0.747252535f,
-0.739008917f,
-0.730653133f,
-0.72218645f,
-0.713610154f,
-0.704925547f,
-0.696133946f,
-0.687236686f,
-0.678235117f,
-0.669130606f,
-0.659924535f,
-0.6506183f,
-0.641213315f,
-0.631711006f,
-0.622112817f,
-0.612420203f,
-0.602634636f,
-0.592757602f,
-0.582790599f,
-0.57273514f,
-0.562592752f,
-0.552364973f,
-0.542053356f,
-0.531659467f,
-0.521184883f,
-0.510631193f,
-0.5f,
-0.489292917f,
-0.478511569f,
-0.467657593f,
-0.456732636f,
-0.445738356f,
-0.434676422f,
-0.423548513f,
-0.412356317f,
-0.401101535f,
-0.389785873f,
-0.37841105f,
-0.366978792f,
-0.355490833f,
-0.343948919f,
-0.332354799f,
-0.320710236f,
-0.309016994f,
-0.297276851f,
-0.285491586f,
-0.27366299f,
-0.261792857f,
-0.24988299f,
-0.237935195f,
-0.225951287f,
-0.213933083f,
-0.201882409f,
-0.189801093f,
-0.17769097f,
-0.165553876f,
-0.153391655f,
-0.141206152f,
-0.128999217f,
-0.116772702f,
-0.104528463f,
-0.092268359f,
-0.079994251f,
-0.067708001f,
-0.055411475f,
-0.043106538f,
-0.030795059f,
-0.018478905f,
-0.006159947f,
0.006159947f,
0.018478905f,
0.030795059f,
0.043106538f,
0.055411475f,
0.067708001f,
0.079994251f,
0.092268359f,
0.104528463f,
0.116772702f,
0.128999217f,
0.141206152f,
0.153391655f,
0.165553876f,
0.17769097f,
0.189801093f,
0.201882409f,
0.213933083f,
0.225951287f,
0.237935195f,
0.24988299f,
0.261792857f,
0.27366299f,
0.285491586f,
0.297276851f,
0.309016994f,
0.320710236f,
0.332354799f,
0.343948919f,
0.355490833f,
0.366978792f,
0.37841105f,
0.389785873f,
0.401101535f,
0.412356317f,
0.423548513f,
0.434676422f,
0.445738356f,
0.456732636f,
0.467657593f,
0.478511569f,
0.489292917f,
0.5f,
0.510631193f,
0.521184883f,
0.531659467f,
0.542053356f,
0.552364973f,
0.562592752f,
0.57273514f,
0.582790599f,
0.592757602f,
0.602634636f,
0.612420203f,
0.622112817f,
0.631711006f,
0.641213315f,
0.6506183f,
0.659924535f,
0.669130606f,
0.678235117f,
0.687236686f,
0.696133946f,
0.704925547f,
0.713610154f,
0.72218645f,
0.730653133f,
0.739008917f,
0.747252535f,
0.755382735f,
0.763398283f,
0.771297962f,
0.779080575f,
0.786744938f,
0.79428989f,
0.801714284f,
0.809016994f,
0.816196912f,
0.823252948f,
0.830184031f,
0.836989108f,
0.843667148f,
0.850217136f,
0.856638078f,
0.862929f,
0.869088946f,
0.875116983f,
0.881012194f,
0.886773686f,
0.892400583f,
0.897892032f,
0.903247199f,
0.908465272f,
0.913545458f,
0.918486986f,
0.923289106f,
0.92795109f,
0.932472229f,
0.936851839f,
0.941089253f,
0.945183828f,
0.949134944f,
0.952942f,
0.95660442f,
0.960121645f,
0.963493144f,
0.966718404f,
0.969796936f,
0.972728272f,
0.975511968f,
0.978147601f,
0.98063477f,
0.9829731f,
0.985162233f,
0.98720184f,
0.989091608f,
0.990831253f,
0.99242051f,
0.993859137f,
0.995146916f,
0.996283653f,
0.997269173f,
0.998103329f,
0.998785992f,
0.99931706f,
0.999696452f,
0.99992411f,
1.f
};

//
// These two tables are sin(pheta/2) and cos(pheta/2). (Sin Table Over 2)
//

static float anim_sintableo2[256]={
0.f,
0.006159947f,
0.01231966f,
0.018478905f,
0.024637449f,
0.030795059f,
0.036951499f,
0.043106538f,
0.049259941f,
0.055411475f,
0.061560906f,
0.067708001f,
0.073852527f,
0.079994251f,
0.086132939f,
0.092268359f,
0.098400278f,
0.104528463f,
0.110652682f,
0.116772702f,
0.122888291f,
0.128999217f,
0.135105247f,
0.141206152f,
0.147301698f,
0.153391655f,
0.159475791f,
0.165553876f,
0.171625679f,
0.17769097f,
0.183749518f,
0.189801093f,
0.195845467f,
0.201882409f,
0.207911691f,
0.213933083f,
0.219946358f,
0.225951287f,
0.231947641f,
0.237935195f,
0.24391372f,
0.24988299f,
0.255842778f,
0.261792857f,
0.267733003f,
0.27366299f,
0.279582593f,
0.285491586f,
0.291389747f,
0.297276851f,
0.303152674f,
0.309016994f,
0.314869589f,
0.320710236f,
0.326538713f,
0.332354799f,
0.338158275f,
0.343948919f,
0.349726511f,
0.355490833f,
0.361241666f,
0.366978792f,
0.372701992f,
0.37841105f,
0.384105749f,
0.389785873f,
0.395451207f,
0.401101535f,
0.406736643f,
0.412356317f,
0.417960345f,
0.423548513f,
0.429120609f,
0.434676422f,
0.440215741f,
0.445738356f,
0.451244057f,
0.456732636f,
0.462203884f,
0.467657593f,
0.473093557f,
0.478511569f,
0.483911424f,
0.489292917f,
0.494655843f,
0.5f,
0.505325184f,
0.510631193f,
0.515917826f,
0.521184883f,
0.526432163f,
0.531659467f,
0.536866598f,
0.542053356f,
0.547219547f,
0.552364973f,
0.557489439f,
0.562592752f,
0.567674716f,
0.57273514f,
0.577773831f,
0.582790599f,
0.587785252f,
0.592757602f,
0.597707459f,
0.602634636f,
0.607538946f,
0.612420203f,
0.617278221f,
0.622112817f,
0.626923806f,
0.631711006f,
0.636474236f,
0.641213315f,
0.645928062f,
0.6506183f,
0.65528385f,
0.659924535f,
0.664540179f,
0.669130606f,
0.673695644f,
0.678235117f,
0.682748855f,
0.687236686f,
0.691698439f,
0.696133946f,
0.700543038f,
0.704925547f,
0.709281308f,
0.713610154f,
0.717911923f,
0.72218645f,
0.726433574f,
0.730653133f,
0.734844967f,
0.739008917f,
0.743144825f,
0.747252535f,
0.75133189f,
0.755382735f,
0.759404917f,
0.763398283f,
0.767362681f,
0.771297962f,
0.775203976f,
0.779080575f,
0.78292761f,
0.786744938f,
0.790532412f,
0.79428989f,
0.798017227f,
0.801714284f,
0.805380919f,
0.809016994f,
0.812622371f,
0.816196912f,
0.819740483f,
0.823252948f,
0.826734175f,
0.830184031f,
0.833602385f,
0.836989108f,
0.840344072f,
0.843667148f,
0.846958211f,
0.850217136f,
0.853443799f,
0.856638078f,
0.859799851f,
0.862929f,
0.866025404f,
0.869088946f,
0.872119511f,
0.875116983f,
0.878081248f,
0.881012194f,
0.88390971f,
0.886773686f,
0.889604013f,
0.892400583f,
0.895163291f,
0.897892032f,
0.900586702f,
0.903247199f,
0.905873422f,
0.908465272f,
0.911022649f,
0.913545458f,
0.916033601f,
0.918486986f,
0.920905518f,
0.923289106f,
0.92563766f,
0.92795109f,
0.930229309f,
0.932472229f,
0.934679767f,
0.936851839f,
0.938988361f,
0.941089253f,
0.943154434f,
0.945183828f,
0.947177357f,
0.949134944f,
0.951056516f,
0.952942f,
0.954791325f,
0.95660442f,
0.958381215f,
0.960121645f,
0.961825643f,
0.963493144f,
0.965124085f,
0.966718404f,
0.968276041f,
0.969796936f,
0.971281032f,
0.972728272f,
0.974138602f,
0.975511968f,
0.976848318f,
0.978147601f,
0.979409768f,
0.98063477f,
0.981822563f,
0.9829731f,
0.984086337f,
0.985162233f,
0.986200747f,
0.98720184f,
0.988165472f,
0.989091608f,
0.989980213f,
0.990831253f,
0.991644696f,
0.99242051f,
0.993158666f,
0.993859137f,
0.994521895f,
0.995146916f,
0.995734176f,
0.996283653f,
0.996795325f,
0.997269173f,
0.99770518f,
0.998103329f,
0.998463604f,
0.998785992f,
0.999070481f,
0.99931706f,
0.99952572f,
0.999696452f,
0.99982925f,
0.99992411f,
0.999981027f,
1.f
};

static float anim_costableo2[256]={
1.f,
0.999981027f,
0.99992411f,
0.99982925f,
0.999696452f,
0.99952572f,
0.99931706f,
0.999070481f,
0.998785992f,
0.998463604f,
0.998103329f,
0.99770518f,
0.997269173f,
0.996795325f,
0.996283653f,
0.995734176f,
0.995146916f,
0.994521895f,
0.993859137f,
0.993158666f,
0.99242051f,
0.991644696f,
0.990831253f,
0.989980213f,
0.989091608f,
0.988165472f,
0.98720184f,
0.986200747f,
0.985162233f,
0.984086337f,
0.9829731f,
0.981822563f,
0.98063477f,
0.979409768f,
0.978147601f,
0.976848318f,
0.975511968f,
0.974138602f,
0.972728272f,
0.971281032f,
0.969796936f,
0.968276041f,
0.966718404f,
0.965124085f,
0.963493144f,
0.961825643f,
0.960121645f,
0.958381215f,
0.95660442f,
0.954791325f,
0.952942f,
0.951056516f,
0.949134944f,
0.947177357f,
0.945183828f,
0.943154434f,
0.941089253f,
0.938988361f,
0.936851839f,
0.934679767f,
0.932472229f,
0.930229309f,
0.92795109f,
0.92563766f,
0.923289106f,
0.920905518f,
0.918486986f,
0.916033601f,
0.913545458f,
0.911022649f,
0.908465272f,
0.905873422f,
0.903247199f,
0.900586702f,
0.897892032f,
0.895163291f,
0.892400583f,
0.889604013f,
0.886773686f,
0.88390971f,
0.881012194f,
0.878081248f,
0.875116983f,
0.872119511f,
0.869088946f,
0.866025404f,
0.862929f,
0.859799851f,
0.856638078f,
0.853443799f,
0.850217136f,
0.846958211f,
0.843667148f,
0.840344072f,
0.836989108f,
0.833602385f,
0.830184031f,
0.826734175f,
0.823252948f,
0.819740483f,
0.816196912f,
0.812622371f,
0.809016994f,
0.805380919f,
0.801714284f,
0.798017227f,
0.79428989f,
0.790532412f,
0.786744938f,
0.78292761f,
0.779080575f,
0.775203976f,
0.771297962f,
0.767362681f,
0.763398283f,
0.759404917f,
0.755382735f,
0.75133189f,
0.747252535f,
0.743144825f,
0.739008917f,
0.734844967f,
0.730653133f,
0.726433574f,
0.72218645f,
0.717911923f,
0.713610154f,
0.709281308f,
0.704925547f,
0.700543038f,
0.696133946f,
0.691698439f,
0.687236686f,
0.682748855f,
0.678235117f,
0.673695644f,
0.669130606f,
0.664540179f,
0.659924535f,
0.65528385f,
0.6506183f,
0.645928062f,
0.641213315f,
0.636474236f,
0.631711006f,
0.626923806f,
0.622112817f,
0.617278221f,
0.612420203f,
0.607538946f,
0.602634636f,
0.597707459f,
0.592757602f,
0.587785252f,
0.582790599f,
0.577773831f,
0.57273514f,
0.567674716f,
0.562592752f,
0.557489439f,
0.552364973f,
0.547219547f,
0.542053356f,
0.536866598f,
0.531659467f,
0.526432163f,
0.521184883f,
0.515917826f,
0.510631193f,
0.505325184f,
0.5f,
0.494655843f,
0.489292917f,
0.483911424f,
0.478511569f,
0.473093557f,
0.467657593f,
0.462203884f,
0.456732636f,
0.451244057f,
0.445738356f,
0.440215741f,
0.434676422f,
0.429120609f,
0.423548513f,
0.417960345f,
0.412356317f,
0.406736643f,
0.401101535f,
0.395451207f,
0.389785873f,
0.384105749f,
0.37841105f,
0.372701992f,
0.366978792f,
0.361241666f,
0.355490833f,
0.349726511f,
0.343948919f,
0.338158275f,
0.332354799f,
0.326538713f,
0.320710236f,
0.314869589f,
0.309016994f,
0.303152674f,
0.297276851f,
0.291389747f,
0.285491586f,
0.279582593f,
0.27366299f,
0.267733003f,
0.261792857f,
0.255842778f,
0.24988299f,
0.24391372f,
0.237935195f,
0.231947641f,
0.225951287f,
0.219946358f,
0.213933083f,
0.207911691f,
0.201882409f,
0.195845467f,
0.189801093f,
0.183749518f,
0.17769097f,
0.171625679f,
0.165553876f,
0.159475791f,
0.153391655f,
0.147301698f,
0.141206152f,
0.135105247f,
0.128999217f,
0.122888291f,
0.116772702f,
0.110652682f,
0.104528463f,
0.098400278f,
0.092268359f,
0.086132939f,
0.079994251f,
0.073852527f,
0.067708001f,
0.061560906f,
0.055411475f,
0.049259941f,
0.043106538f,
0.036951499f,
0.030795059f,
0.024637449f,
0.018478905f,
0.01231966f,
0.006159947f,
6.12574E-17f
};

static void M3rQuaternion_Uncompress3_Single(M3tCompressedQuaternion3 *inCompressed, M3tQuaternion *outQuat)
{
	float cos_long, sin_long;
	float cos_lat, sin_lat;
	float cos_angle, sin_angle;
	int a_long, a_lat, angle;

	// A rotation quaternion represents a rotation around a unit axis.
	// I tried storing this axis as a longtitude and latitude. The
	// rotation is stored as an angle. a_long and a_lat represent the
	// longitude and latitude of the axis. w represents the angle.
	//
	// I used 8.8.8 format to pack the data into 3 bytes. This is
	// a change from the original code, which used an 8.8.8 format
	// to pack the quaternion <x,y,z,w>. The new format worked a bit
	// better (at least 8 bytes of the old format are redundant
	// information), but it still suffered from "stair stepping."
	// ie, the quatization of the quaternion's space was so course
	// that you could visibily see when the quaternion, and by extension
	// the animation, moved from one state to another.
	//

	// pick up the entries for latitude, longtitude, and angle.

	a_long=inCompressed[0];
	a_lat=inCompressed[1];
	angle=inCompressed[2];

	cos_long=anim_costable[a_long];
	cos_lat=anim_costable[a_lat];
	cos_angle=anim_costableo2[angle];
	sin_long=anim_sintable[a_long];
	sin_lat=anim_sintable[a_lat];
	sin_angle=anim_sintableo2[angle];

	// Note that the w component of the quaternion is equal to
	// the angle/2. Since w exists from 0 .. 1 we multiply by
	// 2*PI to get the angle in radians. The final result is w*2*PI/2,
	// or w*PI. Ditto s, which is used to scale the x,y, and z component
	// of the quaternion.

	outQuat->w = cos_angle;
	outQuat->x = cos_long * cos_lat * sin_angle;
	outQuat->y = sin_long * cos_lat * sin_angle;
	outQuat->z = sin_lat * sin_angle;
 

}

static void M3rQuaternion_Uncompress3(M3tCompressedQuaternion3 *inCompressed, UUtUns16 inCount, M3tQuaternion *outQuat)
{
	int itr;

	for(itr = 0; itr < inCount; itr++) {
		M3rQuaternion_Uncompress3_Single(inCompressed, outQuat);
		inCompressed += 3;
		outQuat += 1;
	}
}

// uncompresses a single array
static void M3rQuaternion_Uncompress4(M3tCompressedQuaternion4 *inCompressed, UUtUns16 inCount, M3tQuaternion *outQuat)
{
	UUtUns16 itr;
	const static float div = 1.f / ((float) UUcMaxInt8);

	UUmAssertReadPtr(inCompressed, sizeof(M3tCompressedQuaternion4));
	UUmAssertWritePtr(outQuat, sizeof(M3tQuaternion));

	for(itr = 0; itr < inCount; itr++) {
		outQuat->x = inCompressed->x * div;
		outQuat->y = inCompressed->y * div;
		outQuat->z = inCompressed->z * div;
		outQuat->w = inCompressed->w * div;

		inCompressed += 1;
		outQuat += 1;
	}
}

static void M3rQuaternion_Uncompress6(M3tCompressedQuaternion6 *inCompressed, UUtUns16 inCount, M3tQuaternion *outQuat)
{
	UUtUns16 itr;
	static const float div = 1.f / ((float) UUcMaxInt12);

	UUmAssertReadPtr(inCompressed, sizeof(M3tCompressedQuaternion4));
	UUmAssertWritePtr(outQuat, sizeof(M3tQuaternion));

	for(itr = 0; itr < inCount; itr++) {
		float float_x, float_y, float_z;
		UUtUns16 x,y,z;

		x = inCompressed->rx;
		UUmSwapLittle_2Byte(&x);

		y = inCompressed->ry;
		UUmSwapLittle_2Byte(&y);

		z = inCompressed->rz;
		UUmSwapLittle_2Byte(&z);

		float_x = ((float) x) * (M3c2Pi / ((float) UUcMaxUns16));
		float_y = ((float) y) * (M3c2Pi / ((float) UUcMaxUns16));
		float_z = ((float) z) * (M3c2Pi / ((float) UUcMaxUns16));

		MUrXYZrEulerToQuat(float_x, float_y, float_z, outQuat);

		inCompressed += 1;
		outQuat += 1;
	}
}

static void M3rQuaternion_Uncompress8(M3tCompressedQuaternion8 *inCompressed, UUtUns16 inCount, M3tQuaternion *outQuat)
{
	UUtUns16 itr;
	UUtInt16 inx;
	UUtInt16 iny;
	UUtInt16 inz;
	UUtInt16 inw;
	static const float div = 1.f / ((float) UUcMaxInt16);

	UUmAssertReadPtr(inCompressed, sizeof(M3tCompressedQuaternion8));
	UUmAssertWritePtr(outQuat, sizeof(M3tQuaternion));

	for(itr = 0; itr < inCount; itr++) {
		inx = inCompressed->x;
		iny = inCompressed->y;
		inz = inCompressed->z;
		inw = inCompressed->w;

		UUmSwapLittle_2Byte(&inx);
		UUmSwapLittle_2Byte(&iny);
		UUmSwapLittle_2Byte(&inz);
		UUmSwapLittle_2Byte(&inw);

		outQuat->x = inx * div;
		outQuat->y = iny * div;
		outQuat->z = inz * div;
		outQuat->w = inw * div;

		MUmQuat_VerifyUnit(outQuat);

		inCompressed += 1;
		outQuat += 1;
	}
}

static void M3rQuaternion_Uncompress16(M3tQuaternion *inCompressed, UUtUns16 inCount, M3tQuaternion *outQuat)
{
	int itr;

	UUrMemory_MoveFast(inCompressed, outQuat, sizeof(M3tQuaternion) * inCount);

	for(itr  = 0; itr < inCount; itr++) {
		UUmSwapLittle_4Byte(&(outQuat[itr].x));
		UUmSwapLittle_4Byte(&(outQuat[itr].y));
		UUmSwapLittle_4Byte(&(outQuat[itr].z));
		UUmSwapLittle_4Byte(&(outQuat[itr].w));
	}
}

UUtError
TRrInitialize(
	void)
{
	UUtError error;

	UUrStartupMessage("initializing animation system...");

	error = TRrRegisterTemplates();
	UUmError_ReturnOnError(error);
	
	error = TMrTemplate_PrivateData_New(TRcTemplate_AnimationCollection, 0, TRrTemplateHandler_AnimationCollection, &TRgAnimationCollection_PrivateData);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_PrivateData_New(TRcTemplate_Animation, 0, TRrTemplateHandler_Animation, &TRgAnimation_PrivateData);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_PrivateData_New(TRcTemplate_Body, 0, TRrTemplateHandler_Body, &TRgBody_PrivateData);
	UUmError_ReturnOnError(error);
	
	error = TRrInitialize_IntersectionTables();
	UUmError_ReturnOnError(error);

#if PERFORMANCE_TIMER
	TRg_SetAnimation_Timer = UUrPerformanceTimer_New("TR_SetAnimation");
	TRg_BinarySearch_Timer = UUrPerformanceTimer_New("TR_BinarySearch");
#endif

	return UUcError_None;
}
	
void
TRrTerminate(
	void)
{

}

static void iBuildMatriciesRecursive(
	const TRtBody			*inBody,
	const M3tQuaternion		*inRotations,
	const M3tMatrix4x3		*inMatrix,
	UUtUns16				partIndex,
	UUtUns32				inMask,
	M3tMatrix4x3			*outMatricies)
{
	M3tPoint3D *translation = inBody->translations + partIndex;
	TRtIndexBlock *indexBlock = inBody->indexBlocks + partIndex;
	const M3tQuaternion *rotation = inRotations + partIndex;

	UUmAssert(NULL != inBody);
	UUmAssert(NULL != inRotations);

	// draw sibling
	if (0 != indexBlock->sibling) {
		iBuildMatriciesRecursive(inBody, inRotations, inMatrix, indexBlock->sibling, inMask, outMatricies);
	}

	if (0 == ((1 << partIndex) & inMask)) {
		return;
	}

	outMatricies[partIndex] = *inMatrix;
	MUrMatrixStack_Translate(outMatricies + partIndex, translation);
	MUrMatrixStack_Quaternion(outMatricies + partIndex, rotation);

	// draw our child
	if (0 != indexBlock->child) {
		iBuildMatriciesRecursive(inBody, inRotations, outMatricies + partIndex, indexBlock->child, inMask, outMatricies);
	}

	return;
}

void TRrBody_BuildMatricies(
	const TRtBody*		inBody,
	const TRtExtraBody*	inExtraBody,
	M3tQuaternion		*inQuaternions,
	const M3tMatrix4x3	*inMatrix,
	UUtUns32			inMask, 
	M3tMatrix4x3*		outMatricies)
{
	UUmAssert(NULL != inBody);

	iBuildMatriciesRecursive(inBody, inQuaternions, inMatrix, 0 /*root*/, inMask, outMatricies);

	if (NULL != inExtraBody) {
		UUtUns32 extraItr;

		// walk the extra list and draw any that need drawing
		// clean this up later
		for(extraItr = 0; extraItr < inExtraBody->numParts; extraItr++)
		{			
			const TRtExtraBodyPart *extraPart = inExtraBody->parts + extraItr;

			if (NULL != extraPart->geometry) {
				M3tMatrix4x3 *extraMatrix = outMatricies + inBody->numParts + extraItr;

				*extraMatrix = outMatricies[extraPart->index];

				if (NULL != extraPart->matrix) {
					MUrMatrixStack_Matrix(extraMatrix, extraPart->matrix);
				}
				else {
					MUrMatrixStack_Quaternion(extraMatrix, &extraPart->quaternion);
				}
			}
		}
	}

}

void TRrBody_Draw(
	const TRtBody*		inBody,
	const TRtExtraBody*	inExtraBody,
	const M3tMatrix4x3*	inMatricies,
	UUtUns32			drawBits,
	UUtUns32			redBits,
	UUtUns32			greenBits,
	UUtUns32			blueBits,
	UUtUns32			whiteBits)
{
	UUtUns16 itr;
        
	M3rGeom_State_Commit();

	for(itr = 0; itr < inBody->numParts; itr++)
	{
		M3tGeometry *geometry = inBody->geometries[itr];

		M3rMatrixStack_Push();
		M3rMatrixStack_Multiply(inMatricies + itr);

		if (NULL != geometry) {
			UUtUns32 mask = 1 << itr;

			if (drawBits & mask) {
				if (redBits & mask) {
					M3rGeometry_Draw_BoundingBox(0x1f << 10, geometry);
				}
				else if (greenBits & mask) {
					M3rGeometry_Draw_BoundingBox(0x1f <<  5, geometry);
				}
				else if (blueBits & mask) {
					M3rGeometry_Draw_BoundingBox(0x1f <<  0, geometry);
				}
				else if (whiteBits & mask) {
					M3rGeometry_Draw_BoundingBox(0xffff, geometry);
				}
				else {
					M3rGeometry_Draw(geometry);
				}
			}
		}
	
		M3rMatrixStack_Pop();
	}

	if ((NULL != inExtraBody) && (drawBits & TRcBody_DrawExtra)) {
		UUtUns32 extraItr;

		// walk the extra list and draw any that need drawing
		// clean this up later
		for(extraItr = 0; extraItr < inExtraBody->numParts; extraItr++)
		{		
			const TRtExtraBodyPart *extraPart = inExtraBody->parts +	extraItr;

			if (NULL == extraPart->geometry) { continue; }

			M3rMatrixStack_Push();
			M3rMatrixStack_Multiply(inMatricies + inBody->numParts + extraItr);

			M3rGeometry_Draw(extraPart->geometry);

			M3rMatrixStack_Pop();
		}
	}
}


void TRrBody_DrawMagic(
	const TRtBody*			inBody,
	const M3tMatrix4x3*		inMatricies,
	M3tTextureMap*			inTexture,
	float					inScale,
	const UUtUns8 *			inPartAlpha)
{
	UUtUns16 itr;
	
	M3rGeom_State_Push();
	M3rGeom_State_Commit();

	for(itr = 0; itr < inBody->numParts; itr++)
	{
		M3tGeometry *geometry = inBody->geometries[itr];
		M3tTextureMap *old_texture_map = geometry->baseMap;

		geometry->baseMap = inTexture;
		M3rMatrixStack_Push();
		M3rMatrixStack_Multiply(inMatricies + itr);
		M3rMatrixStack_UniformScale(inScale);

		if (NULL != geometry) {
			if (NULL != inPartAlpha) {
				M3rDraw_State_SetInt(M3cDrawStateIntType_Alpha, inPartAlpha[itr]);
			}
			M3rGeometry_Draw(geometry);
		}

		geometry->baseMap = old_texture_map;
	
		M3rMatrixStack_Pop();
	}

	M3rGeom_State_Pop();

	return;
}


void TRrQuatArray_Zero(
		UUtUns16				inNumParts,
		M3tQuaternion			*quaternions)
{
	UUtUns16		itr;

	for(itr = 0; itr < inNumParts; itr++) {
		quaternions[itr].x = 0.f;
		quaternions[itr].y = 0.f;
		quaternions[itr].z = 0.f;
		quaternions[itr].w = 1.f;
	}

	UUmAssert(NULL != quaternions);
}

#if UUmPlatform == UUmPlatform_SonyPSX2
#define PSX2_ASM_QUATERNION_LERPS
#endif

#if defined(UUmPlatform) && ((UUmPlatform == UUmPlatform_SonyPSX2) || (UUmPlatform == UUmPlatform_Win32) || (UUmPlatform == UUmPlatform_Mac))

//
// The Rock Star Canada animation uncompression routine.
//
// As far as I can tell, all of the character animation in Oni is quaternion
// based. Each quaternion in an animation frame specifies the rotation of
// some joint on a biped model with respect to it's "parent" joint. IE,
// a quaternion may specify the orientation of a hand with respect to the
// arm, or a lower arm with respect to the upper arm. The model is 
// hierarchitcal, in the sense that the hand depends on the lower
// arm, which depends on the upper arm, which depends on the body, etc.
//
// This set of subroutines pulls out all of these "body part" quaternions
// for a particular frame in the animation. In the original Oni code the
// data is stored in a two dimensional array of quaternions. IE, 
// quaternion = animation_quaternions[frame][body_part]. All of the
// quaternions for a particular frame can be passed to a decompression
// routine which will unpack the quaternion from whatever bit format
// it's in (I believe 8, 12, 16, and 32 bits are supported right now)
// into a structure of 4 floats, x,y,z and w. While this gave some
// compression, I believed it was possible to do at least an order
// of magnitude better using a lossy compression algorithm. 
//
// How the algorithm works:
//
// Each body part's animation data is grouped together in one block
// of data. The compression algorithm finds "good" key frames for
// a particular body part and writes out the animation in the following
// format...
//
// Key_Frame Time Key_Frame Time Key_Frame Time ... Time Key_Frame
//
// Each Time value represents the number of frames between the two
// key frames. All 'frames' are assumed to be 60 frames per second
// right now. Values of intermediate frames are calculated by
// interpolating between the two key frame values. 
//
// Example:
// 
// I want to represent 4 unique quaternions, A, B, C, and D, in the 
// animation sequence A A A A B B C D D D
//
// If we look at this animation as a signal being sampled, we might
// represent it as so...
//
//				Original	"Compressed"
//
// Frame 0 		A			A
//
// Frame 1		A			
//							3
// Frame 2		A
//
// Frame 3		A			A
//							1
// Frame 4		B			B
//							1
// Frame 5		B			B
//							1 
// Frame 6		C			C
//							1
// Frame 7		D			D
// 
// Frame 8		D			2
//
// Frame 9		D			D
//
//
// Which gives a compressed sequence A3A1B1B1C1D2D
//
// If I want to compute frame 2 I start at frame 0, I see that the
// "gap" between frame 0's key frame and frame 3's key frame is 3 frames,
// I interpolate (lerp) 2/3rds of the way between the key frame quaternions,
// and I get the value A. If I want the value for frame 3.25 I interpolate 
// 25% between the second A keyframe and the first B key frame.
//
// The length of each body part's compressed animation sequence now
// varies, depending on how well the animation compressed worked.
// In order to index each body part properly an array of offsets is
// the first thing written into the animation data. I suppose that
// technically this should be a part of the template structure and
// not the raw data, but I'm trying to tread lightly on the core
// code while Bungie are still working on their stuff. Note that
// There will be a problem with endian on the mac version.
//

static void TRrQuatArray_SetAnimationInternal(
		const TRtAnimation*		inAnimation,
		UUtUns16				inAnimFrame,
		M3tQuaternion			*quaternions)
{
	int i;

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Enter(TRg_SetAnimation_Timer);
#endif

	UUmAssertReadPtr(inAnimation, sizeof(*inAnimation));
	UUmAssert(inAnimFrame < inAnimation->numFrames);
	UUmAssert(inAnimation->numParts <= TRcMaxParts);

	// For each part...

	for (i=0; i<inAnimation->numParts;++i) {

		// The play station has some alignmnet issues. You may not
		// randomly cast byte aligned pointers to char into pointers
		// to int and expect it to actually work worth crap.

		char tmem[32];

		// a pointer to the current spot in the compressed data. We're
		// mixing between unsigned char values and quaternions of
		// unknown size, so I made this variable unsigned char.

		unsigned char *compressed;

		// This points to the "last" and "current" quaternion. We're
		// more or less going to skip through the frame data until the
		// last quaternion's frame number is greater than or equal to
		// the frame we want and the current quaternion's frame number
		// is less than the frame we want. At this point we know that
		// the desired frame is somewhere between the two values.

		char *last_quaternion;
		char *current_quaternion;

		// The frame number of 'current quaternion'

		int current_frame;

		// frame_gap is the gap between the two key-frames.
		// into-gap is how many "frames" past current quaternion the
		// key frame was. 

		int frame_gap;
		int into_gap;

		// The two quaternions that we're going to interpolate between.

#ifndef PSX2_ASM_QUATERNION_LERPS
		M3tQuaternion interp[2];
#else
		M3tQuaternion interp[2] __attribute__((aligned(128)));
//		M3tQuaternion target __attribute__((aligned(128)));
		int frame_vector[4] __attribute__((aligned(128)));
#endif

		// The size of the quaternions, in bytes. I'm declaring this
		// here so it's on the local frame (and this should run faster).

		int compressionSize= inAnimation->compressionSize;

		// Get the location of the start of the frame data.
	#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Mac)
		{
			unsigned short swapped= ((unsigned short *)inAnimation->data)[i];
			swapped= (((swapped & 0xFF00) >> 8) | ((swapped & 0x00FF) << 8));
			compressed= ((UUtUns8 *)inAnimation->data) + swapped;
		}
	#else 
		compressed=((UUtUns8 *) inAnimation->data) + ((unsigned short *) inAnimation->data)[i];
 	#endif
		// Set up the initial quaternion. "current quaternion" is the last
		// key frame quaternion. "current frame" is the last key frame.
		// Note that we start numbering at 0 instead of 1, for what
		// it's worth.
		//
		// The size of the quaternions varies depending on compressionSize.
		// We're going to keep everything is unsigned char for now.

		current_quaternion=(char*)compressed;
		current_frame=0;
		compressed+=compressionSize;

		// Skip forward and find the appropriate frame.

		do {
			frame_gap=*compressed++;
			last_quaternion=current_quaternion;
			current_quaternion=(char*)compressed;
			compressed+=compressionSize;
			current_frame+=frame_gap;
		}
		while (inAnimFrame>current_frame);

		into_gap=inAnimFrame-(current_frame-frame_gap);

		// This gets rather ugly. In the switch statement, for some of
		// the enties, Memcpy the two quaternions into
		// the temp data structure to make sure everything is aligned
		// well when we pass the pointer to the decompress routine.
		//
		// there are probably serious games we could play to try to
		// reduce the amount of CPU this process takes, all of which
		// are bloody evil. If game state update starts chewing CPU
		// in this subroutine I'll take us down the road to hell.

		// do the appropriate decompression.

		switch(compressionSize)
		{
			case TRcAnimationCompression3:
				{
					// Special optimized code.
					if (into_gap==0) {
						M3rQuaternion_Uncompress3_Single((UUtUns8*)last_quaternion, quaternions+i);
					}
					else if (into_gap==frame_gap) {
						M3rQuaternion_Uncompress3_Single((UUtUns8*)current_quaternion, quaternions+i);
					}
					else {
						M3rQuaternion_Uncompress3_Single((UUtUns8*)last_quaternion, interp);
						M3rQuaternion_Uncompress3_Single((UUtUns8*)current_quaternion, interp+1);
#ifdef PSX2_ASM_QUATERNION_LERPS

						frame_vector[0]=into_gap;
						frame_vector[1]=frame_gap;

        				asm __volatile__("
            				lqc2 vf01, 0(%0)
            				lqc2 vf02, 0(%1)
            				lqc2 vf29, 0(%3)
            				vcallms quat_lerp
							mtsab	$0,8
            				qmfc2.i $6, vf13
							qfsrv	$5, $5, $6
							nop
							nop
							sdl $6, 0(%2)
							sdr $6, 0(%2)
							sdl $5, 8(%2)
							sdr $5, 8(%2)
							nop
							nop
							
            				" :: "r" (interp), "r" (interp+1), "r" (quaternions+i), "r" (frame_vector): "$5","$6");


#else
						MUrQuat_Lerp(interp,interp+1, ((float) into_gap)/((float) frame_gap), quaternions+i);
#endif

					}
					continue;
				}

			case TRcAnimationCompression4:
				memcpy(tmem,last_quaternion,compressionSize);
				memcpy(tmem+compressionSize,current_quaternion,
					compressionSize);

				M3rQuaternion_Uncompress4((void *)tmem, 2, interp);
				break;
	
			case TRcAnimationCompression6:
				memcpy(tmem,last_quaternion,compressionSize);
				memcpy(tmem+compressionSize,current_quaternion,
					compressionSize);
				M3rQuaternion_Uncompress6((void *)tmem, 2, interp);
				break;

			case TRcAnimationCompression8:
				memcpy(tmem,last_quaternion,compressionSize);
				memcpy(tmem+compressionSize,current_quaternion,
					compressionSize);
				M3rQuaternion_Uncompress8((void *)tmem, 2, interp);
			break;

			case TRcAnimationCompression16:
				memcpy(tmem,last_quaternion,compressionSize);
				memcpy(tmem+compressionSize,current_quaternion,
					compressionSize);
				M3rQuaternion_Uncompress16((void *)tmem, 2, interp);
			break;
		}

		//
		// Danger, Danger, in order to optimize this code I moved the
		// tail end of this loop into case TRcAnimationCompression3 in
		// the case statement. I'd like to nix the other conditions,
		// but I'm afriad it would break stuff.
		//

		if (into_gap==0) {

			// Hmm, this case happens if frame == 0.

			quaternions[i]=interp[0];
		}
		else if (into_gap==frame_gap) {
			quaternions[i]=interp[1];
		} 
		else {
			MUrQuat_Lerp(interp,interp+1, ((float) into_gap)/((float) frame_gap), quaternions+i);
		}
	}

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Exit(TRg_SetAnimation_Timer);
#endif

	return;
}

#else

static void TRrQuatArray_SetAnimationInternal(
		const TRtAnimation*		inAnimation,
		UUtUns16				inAnimFrame,
		M3tQuaternion			*quaternions)
{
	void *compressed;

	UUmAssertReadPtr(inAnimation, sizeof(*inAnimation));
	UUmAssert(inAnimFrame < inAnimation->numFrames);
	UUmAssert(inAnimation->numParts <= TRcMaxParts);

	compressed = ((UUtUns8 *) inAnimation->data) + (inAnimFrame * inAnimation->numParts * inAnimation->compressionSize);

	switch(inAnimation->compressionSize)
	{
		case TRcAnimationCompression4:
			M3rQuaternion_Uncompress4(compressed, inAnimation->numParts, quaternions);
		break;

		case TRcAnimationCompression6:
			M3rQuaternion_Uncompress6(compressed, inAnimation->numParts, quaternions);
		break;

		case TRcAnimationCompression8:
			M3rQuaternion_Uncompress8(compressed, inAnimation->numParts, quaternions);
		break;

		case TRcAnimationCompression16:
			M3rQuaternion_Uncompress16(compressed, inAnimation->numParts, quaternions);
		break;
	}
}

#endif

void TRrQuatArray_SetAnimation(
		const TRtAnimation*			inAnimation,
		TRtAnimTime				inTime,
		M3tQuaternion			*inDest)
{
//	float floatFrameNum = UUmMin(inAnimation->numFrames - 1, ((inTime) * (inAnimation->fps)) / 60.f);
//	UUtUns16 frameNum = TRmAnimationTimeToFrame(inAnimation, inTime);	// always rounds down

//	UUmAssert(floatFrameNum >= frameNum);
//	UUmAssert(floatFrameNum < (frameNum+1));

	// if not 60 fps, we compress to two different internal arrays and LERP to 
	UUmAssertReadPtr(inAnimation, sizeof(*inAnimation));
	UUmAssertWritePtr(inDest, inAnimation->numParts * sizeof(M3tQuaternion));
	UUmAssert(inTime < inAnimation->numFrames);

	TRrQuatArray_SetAnimationInternal(inAnimation, inTime, inDest);

#if 0
	if (floatFrameNum == frameNum) {
		TRrQuatArray_SetAnimationInternal(inAnimation, frameNum, inDest);
	}
	else {
		float distance = floatFrameNum - frameNum;

		TRrQuatArray_SetAnimationInternal(inAnimation, frameNum, TRgQuatAraryPrivate_SetAnimation);
		TRrQuatArray_SetAnimationInternal(inAnimation, frameNum + 1, inDest);

		TRrQuatArray_Slerp(inAnimation->numParts, distance, TRgQuatAraryPrivate_SetAnimation, inDest, inDest);
	}
#endif
}

void TRrQuatArray_Lerp(
		UUtUns16				inNumParts,
		float					inAmt1,
		const M3tQuaternion		*inSrc1,
		const M3tQuaternion		*inSrc2,
		M3tQuaternion			*inDest)
{
	UUtUns16 itr;

	for(itr = 0; itr < inNumParts; itr++)
	{
		MUrQuat_Lerp(
			inSrc1 + itr,
			inSrc2 + itr,
			inAmt1,
			inDest + itr);
	}
}

#if 0
void TRrQuatArray_Lerp4(
		UUtUns16				inNumParts,
		float					*inAmts,
		const M3tQuaternion		*inSrc1,
		const M3tQuaternion		*inSrc2,
		const M3tQuaternion		*inSrc3,
		const M3tQuaternion		*inSrc4,
		M3tQuaternion			*inDest)
{
	UUtUns16 itr;
	const M3tQuaternion *src5 = NULL;
	const M3tQuaternion *src6 = NULL;
	float amt01;
	float amt23;

	// basic path: 
	// lerp 1 & 2
	// lerp 3 & 4
	// lerp (1&2) & (3&4)

	amt01 = inAmts[0] + inAmts[1];
	amt23 = inAmts[2] + inAmts[3];

	if (amt01 > UUcEpsilon) {
		float lerp01;
		
		TRrQuatArray_Lerp(inNumParts, amountA, inSrc1, inSrc2, TRgQuatArrayPrivateA);
		src5 = TRgQuatArrayPrivateA;
	}

	if (amt23 > UUcEpsilon) {
		TRrQuatArray_Lerp(inNumParts, amountA, inSrc3, inSrc4, inDest);
		src6 = inDest;
	}

	if ((src5 != NULL) && (src6 != NULL)) {
		TRrQuatArray_Lerp(inNumParts, amountC, src5, src6, inDest);
	}
	else if (src5 != NULL) {
		TRrQuatArray_Move(inNumParts, src5, inDest);
	}
	else if (src6 != NULL) {
		TRrQuatArray_Move(inNumParts, src6, inDest);
	}
	else {
		UUmAssert(!"bad default case in Lerp4");
	}

	return;
}
#endif

void TRrQuatArray_Slerp(
		UUtUns16				inNumParts,
		float					inAmt1,
		const M3tQuaternion		*inSrc1,
		const M3tQuaternion		*inSrc2,
		M3tQuaternion			*inDest)
{
	UUtUns16 itr;

	for(itr = 0; itr < inNumParts; itr++)
	{
		MUrQuat_Slerp(
			inSrc1 + itr,
			inSrc2 + itr,
			inAmt1,
			inDest + itr);
	}
}


void TRrQuatArray_Multiply(
		UUtUns16				inNumParts,
		const M3tQuaternion		*inSrc1,
		const M3tQuaternion		*inSrc2,
		M3tQuaternion			*inDest)
{
	UUtUns16 itr;
	for(itr = 0; itr < inNumParts; itr++) {
		MUrQuat_Mul(inSrc1 + itr, inSrc2 + itr,	inDest + itr);
	}
}

void TRrBody_SetMaps(
	TRtBody*			inBody,
	TRtBodyTextures		*inMaps)
{
	UUtUns16 itr;

	UUmAssert(NULL != inBody);
	UUmAssert(NULL != inMaps);
	UUmAssert(inBody->numParts == inMaps->numMaps);

	for(itr = 0; itr < inBody->numParts; itr++)
	{
		inBody->geometries[itr]->baseMap = inMaps->maps[itr];
	}
}

#if 0
UUtUns16 TRrCollection_BinarySearch_Slow(const TRtAnimationCollection *inCollection, TRtAnimType inType, TRtAnimState inState)
{
	UUtUns16 itr;

	for(itr = 0; itr < inCollection->numAnimations; itr++)
	{
		TRtAnimState	curState = inCollection->entry[itr].virtualFromState;
		TRtAnimType		curType = inCollection->entry[itr].animation->type;

		if ((inType == curType) && (inState == curState))
		{
			return itr;
		}
	}

	return 0xffff;
}
#endif

static UUtUns16 TRrCollection_BinarySearch(const TRtAnimationCollection *inCollection, TRtAnimType inType, TRtAnimState inState)
{
	UUtInt32 top;
	UUtInt32 bottom;
	UUtUns32 lookupTarget = (inType << 16) | (inState << 0);
	UUtUns16 result = 0xFFFF;

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Enter(TRg_BinarySearch_Timer);
#endif

	top = 0;
	bottom = inCollection->numAnimations - 1;

	while(top <= bottom) {
		const TRtAnimationCollectionPart *entry;
		UUtInt16 guess;
		UUtBool found;

		guess = (UUtInt16)((top + bottom) >> 1 /* S.S. / 2*/);

		entry = inCollection->entry + guess;
		found = lookupTarget == entry->lookupValue;

		if (found) {
			// ok, found, walk up
			while(guess > 0) {
				entry = inCollection->entry + guess - 1;
				found = lookupTarget == entry->lookupValue;
		
				if (!found) {
					result = guess;
					goto exit;
				}

				guess -= 1;
			}

			result = 0;
			goto exit;
		}

		if (entry->lookupValue > lookupTarget) {
			bottom = guess - 1;
		}
		else {
			top = guess + 1;
		}
	}

exit:

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Exit(TRg_BinarySearch_Timer);
#endif

	return result;
}

static const TRtAnimation *TRrCollection_Lookup_Internal(const TRtAnimationCollection *inCollection, TRtAnimType inType, TRtAnimState inState, TRtAnimVarient inVarient)
{
#if defined(DEBUGGING) && DEBUGGING
	const char *instanceName = inCollection->recursiveOnly ? "recursive" : TMrInstance_GetInstanceName(inCollection);
#endif

	UUtUns16		itr;
	const TRtAnimation	*animation;
	const TRtAnimation	*result = NULL;
	UUtInt32		total;
	UUtBool			foundFirstValidAnimation = UUcFalse;
	TRtAnimVarient	badVarientFlags = ~inVarient;
	TRtAnimVarient	foundVarient;
	TRtAnimType		foundType;
	UUtUns16		start;

	if (inType != 0)
	{
		start = TRrCollection_BinarySearch(inCollection, inType, inState);
		// UUmAssert(start == TRrCollection_BinarySearch_Slow(inCollection, inType, inState));
	}
	else
	{
		start = 0;
	}

	for(itr = start; itr < inCollection->numAnimations; itr++)
	{
		const TRtAnimationCollectionPart *entry = inCollection->entry + itr;
		animation = entry->animation;

		if (entry->virtualFromState != inState) {
			if (inType != 0) { break; }
			else { continue; }
		}

		if ((inType != animation->type) && (inType != 0)) { 
			if (inType != 0) { break; }
			else { continue; }
		}

		if (animation->varient & badVarientFlags) { 
			continue; 
		}

		if (animation->firstLevel > TRgLevelNumber) {
			// we need to skip this animation
			
#if defined(DEBUGGING) && DEBUGGING
			if ((itr + 1) < inCollection->numAnimations) {			
				UUtBool same_from_state = entry->virtualFromState == inCollection->entry[itr + 1].virtualFromState;
				UUtBool same_type = entry->animation->type == inCollection->entry[itr + 1].animation->type;
				UUtBool same_varient = entry->animation->varient == inCollection->entry[itr + 1].animation->varient;

				UUmAssert("level number issues" && !(same_from_state && same_type && same_varient));
			}
#endif

			continue;
		}

		// already found ours
		if (foundFirstValidAnimation)
		{
			// this was not random picked
			if (entry->weight > total) {
				break;
			}

			// halt that was the end of the varient streak
			if (foundVarient != animation->varient) {
				break;
			}

			// halt that was the end of the type streak
			if (foundType != animation->type) {
				break;
			}
		}

		result = animation;

		if (TRcWeight_OnlyEntry == entry->weight)
		{
			break; 
		}

		if (!foundFirstValidAnimation)
		{
			total = UUmRandomRange(0, entry->weight);
			foundVarient = animation->varient;
			foundType = animation->type;
			foundFirstValidAnimation = UUcTrue;
		}
	}

	// if not a fit or not a perfect fit try to find a better fit via recursive lookup
	if ((NULL == result) || (result->varient != inVarient)) {
		const TRtAnimation *recursiveResult = NULL;

		if (inCollection->recursiveLookup != NULL) {
			recursiveResult = TRrCollection_Lookup(inCollection->recursiveLookup, inType, inState, inVarient);
		}

		if (NULL == recursiveResult) {
			result = result;
		}
		else if (NULL == result) {
			result = recursiveResult;
		}
		else if (recursiveResult->varient > result->varient) {
			result = recursiveResult;
		}
	}

	return result;
}

void 
TRrCollection_Lookup_Range(
		TRtAnimationCollection *inCollection, 
		TRtAnimType inType, 
		TRtAnimState inState, 
		TRtAnimationCollectionPart **outFirst, 
		UUtInt32 *outCount)
{
	UUtInt32 loop;
	UUtInt32 start;
	UUtInt32 count;
	UUtUns32 lookupTarget = (inType << 16) | (inState << 0);

	start = TRrCollection_BinarySearch(inCollection, inType, inState);

	for(count = 0, loop = start; loop < inCollection->numAnimations; count++, loop++) {
		TRtAnimationCollectionPart *part = inCollection->entry + loop;

		if (part->lookupValue != lookupTarget) { break; }
	}

	if (count > 0) {
		*outFirst = inCollection->entry + start;
		*outCount = count;
	}
	else {
		*outFirst = NULL;
		*outCount = 0;
	}

	return;
}

TRtAnimationCollection *TRrCollection_GetRecursive(
		TRtAnimationCollection *inCollection)
{
	return inCollection->recursiveLookup;
}

UUtUns32 TRgCollectionLookupCounter = 0;

const TRtAnimation *TRrCollection_Lookup(const TRtAnimationCollection *inCollection, TRtAnimType inType, TRtAnimState inState, TRtAnimVarient inVarient)
{
	const TRtAnimation *lookupResult = NULL;
	static UUtUns32 lookup_depth = 0;

	if (0 == lookup_depth) {
		TRgCollectionLookupCounter++;

		if (NULL != gLookupStream) {
			fprintf(gLookupStream, "\nlookup in %s type %d state %d varient %x\n", TMrInstance_GetInstanceName(inCollection), inType, inState, inVarient);
		}
	}

	lookup_depth++;

	// two steps, 
	UUmAssert(inType != TRcAnimState_Anything);
	
	// look for a real hit
	lookupResult = TRrCollection_Lookup_Internal(inCollection, inType, inState, inVarient);

	// look for a hit that will match any anim state
	if (NULL == lookupResult) {
		lookupResult = TRrCollection_Lookup_Internal(inCollection, inType, TRcAnimState_Anything, inVarient);
	}

	lookup_depth--;

	if (0 == lookup_depth) {
		if (NULL != gLookupStream) {
			fprintf(gLookupStream, "found %s\n", (NULL == lookupResult) ? "nothing" : lookupResult->instanceName);
		}
	}

	return lookupResult;
}


#define imGetAnimSortID(part)  ((((part)->animation->type) << 16) + ((part)->virtualFromState))

static int UUcExternal_Call iCollectionCompare(const void *elem1, const void *elem2)
{
	int result;
	TRtAnimationCollectionPart *part1 = (TRtAnimationCollectionPart *) elem1;
	TRtAnimationCollectionPart *part2 = (TRtAnimationCollectionPart *) elem2;
	UUtUns32 num1 = imGetAnimSortID(part1);
	UUtUns32 num2 = imGetAnimSortID(part2);

	if (num1 < num2) {
		result = -1;
	}
	else if (num1 == num2) {
		TRtAnimVarient num1Varient = part1->animation->varient;
		TRtAnimVarient num2Varient = part2->animation->varient;

		// varients sort in the opposite order
		if (num1Varient > num2Varient) {
			result = -1;
		}
		else if (num1Varient < num2Varient) {
			result = 1;
		}
		else {
			result = 0;
		}
	}
	else if (num1 > num2) {
		result = 1;
	}
	else {
		UUmDebugStr("should not be here");
	}

	return result;
}

static void iVerifyCollections(void)
{
#define cMaxCollections 100
	UUtUns32 numCollections;
	UUtUns32 itr;
	TRtAnimationCollection *collectionList[cMaxCollections];

	TMrInstance_GetDataPtr_List(TRcTemplate_AnimationCollection, cMaxCollections, &numCollections, collectionList);

	for(itr = 0; itr < numCollections; itr++)
	{
		char *tableName = TMrInstance_GetInstanceName(collectionList[itr]);
	}
}

static void iAnimationCollection_SortAndWeight(TRtAnimationCollection *ioCollection, char *tableName)
{
	UUtUns32 rangeStart, rangeStop;
	UUtUns16 itr;
#if ANIM_PRINT_TABLE
	static FILE *stream = NULL;
#endif

		// sort the table by type and fromState
	qsort(	ioCollection->entry, 
			ioCollection->numAnimations, 
			sizeof(TRtAnimationCollectionPart), 
			iCollectionCompare);

#if ANIM_PRINT_TABLE
	if (NULL == stream) {
		stream = fopen("anim_table.txt", "w");
	}

	fprintf(stream, "----------------------------------\n");
	fprintf(stream, "%s\n", tableName);
	fprintf(stream, "----------------------------------\n");

	// debug info
	for(itr = 0; itr < ioCollection->numAnimations; itr++)
	{
		char *instanceName = TMrInstance_GetInstanceName(ioCollection->entry[itr].animation);

		fprintf(stream, "[%d] (%x) {%x} wt = %d name =%s\n", itr, imGetAnimSortID(ioCollection->entry + itr), ioCollection->entry[itr].animation->varient, ioCollection->entry[itr].weight, instanceName);
	}

	fprintf(stream, "----------------------------------\n");
#endif

	// build the weights
	rangeStart = 0;
	for(itr = 0; itr < ioCollection->numAnimations; itr++)
	{
		UUtUns32 thisID;
		TRtAnimVarient thisVarient;
		UUtUns32 nextID;
		TRtAnimVarient nextVarient;

		thisID = imGetAnimSortID(ioCollection->entry + itr);
		thisVarient = ioCollection->entry[itr].animation->varient;
		
		if (((UUtUns32)(itr + 1)) < ioCollection->numAnimations) {
			nextID = imGetAnimSortID(ioCollection->entry + itr + 1);
			nextVarient = ioCollection->entry[itr + 1].animation->varient;
		}
		else {
			nextID = 0;
			nextVarient = 0;
		}

		if ((thisID != nextID) || (thisVarient != nextVarient))
		{
			UUtUns32 rangeItr;
			UUtUns16 weightTotal;

			rangeStop = itr;

			UUmAssert(rangeStop >= rangeStart);

			if (rangeStart == rangeStop)
			{
				ioCollection->entry[rangeStart].weight = 0;
			}
			else
			{
				// process rangeStart .. rangeStop
				weightTotal = 0;
				for(rangeItr = rangeStart; rangeItr <= rangeStop; rangeItr++)
				{
					UUtUns16 myWeight = ioCollection->entry[rangeItr].weight;

					ioCollection->entry[rangeItr].weight = weightTotal;

					UUmAssert(((UUtUns16) weightTotal + myWeight) > weightTotal);
					weightTotal += myWeight;
				}

				if (rangeStart != rangeStop) {
					ioCollection->entry[rangeStart].weight = weightTotal;
				}
			}

			rangeStart = rangeStop + 1;
		}
	}

#if ANIM_PRINT_TABLE
	// debug info
	for(itr = 0; itr < ioCollection->numAnimations; itr++)
	{
		char *instanceName = TMrInstance_GetInstanceName(ioCollection->entry[itr].animation);

		fprintf(stream, "[%d] (%x) {%x} wt = %d name =%s\n", itr, imGetAnimSortID(ioCollection->entry + itr), ioCollection->entry[itr].animation->varient, ioCollection->entry[itr].weight, instanceName);
	}
	fprintf(stream, "----------------------------------\n");
	fflush(stream);
#endif
}							 

static void iAnimationCollection_DiskToMemory(TRtAnimationCollection *ioCollection)
{
	UUtUns16 itr;
	UUtUns16 shortcutItr;
	TRtAnimationCollection *recursiveCollection = NULL;
	UUtUns16 recursiveCount = 0;

	// remove the NULL animations
	itr = 0;
	while(itr < ioCollection->numAnimations)
	{
		if (NULL == ioCollection->entry[itr].animation) {
			ioCollection->numAnimations--;
			ioCollection->entry[itr] = ioCollection->entry[ioCollection->numAnimations];

			continue;
		}

		itr++;
	}

	for(itr = 0; itr < ioCollection->numAnimations; itr++) {
		TRtAnimationCollectionPart *part;
		UUmAssert(ioCollection->entry[itr].animation != NULL);
		
		part = ioCollection->entry + itr;

		part->virtualFromState = ioCollection->entry[itr].animation->fromState;
		part->lookupValue = (part->animation->type << 16) | (part->virtualFromState << 0);
	}

	// make this collection final
	iAnimationCollection_SortAndWeight(ioCollection, TMrInstance_GetInstanceName(ioCollection));

	// builds its sub (interpolation) collection and insert it into the recursive lookup chain
	for(itr = 0; itr < ioCollection->numAnimations; itr++) {
		TRtAnimation *curAnimation = ioCollection->entry[itr].animation;

		TRrAnimation_Prepare(curAnimation);
		
		for(shortcutItr = 0; shortcutItr < curAnimation->numShortcuts; shortcutItr++) {
			TRtAnimationCollectionPart *recursivePart; 

			recursiveCollection = UUrMemory_Block_Realloc(recursiveCollection, 
				sizeof(TRtAnimationCollection) + (recursiveCount * sizeof(TRtAnimationCollectionPart)));

			recursivePart = recursiveCollection->entry + recursiveCount;

			recursivePart->weight = 100;
			recursivePart->virtualFromState = curAnimation->shortcuts[shortcutItr].state;
			recursivePart->animation  = curAnimation;
			recursivePart->lookupValue = (curAnimation->type << 16) | (recursivePart->virtualFromState << 0);

			recursiveCount += 1;			

			recursiveCollection->numAnimations = recursiveCount;
		}
	}

	if (NULL != recursiveCollection) {
		recursiveCollection->recursiveOnly = UUcTrue;
		recursiveCollection->recursiveLookup = ioCollection->recursiveLookup;
		ioCollection->recursiveLookup = recursiveCollection;

		iAnimationCollection_SortAndWeight(recursiveCollection, "recursive");
	}
}


static void iAnimationCollection_Unload(TRtAnimationCollection *ioCollection)
{
	TRtAnimationCollection *recursiveCollection = ioCollection->recursiveLookup;

	if (NULL == recursiveCollection) {
		goto exit;
	}

	if (!recursiveCollection->recursiveOnly) {
		goto exit;
	}

	ioCollection->recursiveLookup = recursiveCollection->recursiveLookup;
	UUrMemory_Block_Delete(recursiveCollection);

exit:
	return;
}

static UUtError
TRrTemplateHandler_AnimationCollection(
	TMtTemplateProc_Message	inMessage,
	void*					inDataPtr,
	void*					inPrivateData)
{
	TRtAnimationCollection			*collection;
	
	collection = (TRtAnimationCollection*)(inDataPtr);

	switch(inMessage)
	{
		case TMcTemplateProcMessage_DisposePreProcess:
			iAnimationCollection_Unload(collection);
			break;
		
		case TMcTemplateProcMessage_LoadPostProcess:
			iAnimationCollection_DiskToMemory(collection);
			break;
		
		case TMcTemplateProcMessage_NewPostProcess:
		case TMcTemplateProcMessage_Update:
		default:
			UUmDebugStr("you can't new, update or watchimicallit this template with some more C mister.");
	}
		
	return UUcError_None;	
}

static UUtError
TRrTemplateHandler_Body(
	TMtTemplateProc_Message	inMessage,
	void*					inDataPtr,
	void*					inPrivateData)
{
	TRtBody *body;
	
	body = (TRtBody*)(inDataPtr);

	switch(inMessage)
	{
		case TMcTemplateProcMessage_DisposePreProcess:
			break;
		
		case TMcTemplateProcMessage_LoadPostProcess:
			body->geometries = body->geometryStorage->geometries;
			body->translations = body->translationStorage->translations;
			body->indexBlocks = body->indexBlockStorage->indexBlocks;
			break;
		
		case TMcTemplateProcMessage_NewPostProcess:
		case TMcTemplateProcMessage_Update:
		default:
			UUmDebugStr("you can't new, update or watchimicallit this template with some more C mister.");
	}
		
	return UUcError_None;	
}


static void TRrAnimation_Prepare(TRtAnimation *animation)
{
	void *raw_offset;

	if (animation->flags & (1 << TRcAnimFlag_Prepared))
	{
		return;
	}

	animation->instanceName = TMrInstance_GetInstanceName(animation);

	raw_offset = TMrInstance_GetRawOffset(animation);

	if (NULL != animation->attacks) {
		animation->attacks = UUmOffsetPtr(animation->attacks, raw_offset);
	}

	if (NULL != animation->takeDamages) {
		animation->takeDamages = UUmOffsetPtr(animation->takeDamages, raw_offset);
	}

	if (NULL != animation->blurs) {
		animation->blurs = UUmOffsetPtr(animation->blurs, raw_offset);
	}

	if (NULL != animation->shortcuts) {
		animation->shortcuts = UUmOffsetPtr(animation->shortcuts, raw_offset);
	}

	if (NULL != animation->heights) {
		animation->heights = UUmOffsetPtr(animation->heights, raw_offset);
	}

	if (NULL != animation->positions) {
		animation->positions = UUmOffsetPtr(animation->positions, raw_offset);
	}

	if (NULL != animation->footsteps) {
		animation->footsteps = UUmOffsetPtr(animation->footsteps, raw_offset);
	}

	if (NULL != animation->throwInfo) {
		animation->throwInfo = UUmOffsetPtr(animation->throwInfo, raw_offset);
	}

	if (NULL != animation->particles) {
		animation->particles = UUmOffsetPtr(animation->particles, raw_offset);
	}

	if (NULL != animation->positionPoints) {
		animation->positionPoints = UUmOffsetPtr(animation->positionPoints, raw_offset);
	}

	if (NULL != animation->extentInfo.attackExtents) {
		animation->extentInfo.attackExtents = UUmOffsetPtr(animation->extentInfo.attackExtents, raw_offset);
	}

	if (NULL != animation->newSounds) {
		animation->newSounds = UUmOffsetPtr(animation->newSounds, raw_offset);
	}

	animation->data = UUmOffsetPtr(animation->data, raw_offset);

	TRrAnimation_SwapHeights(animation);
	TRrAnimation_SwapPositions(animation);
	TRrAnimation_SwapAttacks(animation);
	TRrAnimation_SwapTakeDamages(animation);
	TRrAnimation_SwapBlurs(animation);
	TRrAnimation_SwapShortcuts(animation);
	TRrAnimation_SwapThrowInfo(animation);
	TRrAnimation_SwapFootsteps(animation);
	TRrAnimation_SwapParticles(animation);
	TRrAnimation_SwapPositionPoints(animation);
	TRrAnimation_SwapNewSounds(animation);

	TRrAnimation_SwapExtentInfo(animation);

	animation->flags |= (1 << TRcAnimFlag_Prepared);

	return;
}

static UUtError
TRrTemplateHandler_Animation(
	TMtTemplateProc_Message	inMessage,
	void*					inDataPtr,
	void*					inPrivateData)
{
	TRtAnimation			*animation;
	
	animation = (TRtAnimation*)(inDataPtr);

	switch(inMessage)
	{
		case TMcTemplateProcMessage_DisposePreProcess:
			break;
		
		case TMcTemplateProcMessage_LoadPostProcess:
			TRrAnimation_Prepare(animation);
			break;
		
		case TMcTemplateProcMessage_NewPostProcess:
		case TMcTemplateProcMessage_Update:
			break;
	}
		
	return UUcError_None;
	
}

TRtAnimVarient	TRrAnimation_GetVarient(const TRtAnimation *inAnimation)
{
	UUmAssertReadPtr(inAnimation, sizeof(TRtAnimation));

	return inAnimation->varient;
}

TRtAnimType TRrAnimation_GetType(const TRtAnimation *inAnimation)
{
	UUmAssertReadPtr(inAnimation, sizeof(TRtAnimation));

	return inAnimation->type;
}

TRtAnimState TRrAnimation_GetFrom(const TRtAnimation *inAnimation)
{
	UUmAssertReadPtr(inAnimation, sizeof(TRtAnimation));

	return inAnimation->fromState;
}

TRtAnimState TRrAnimation_GetTo(const TRtAnimation *inAnimation)
{
	UUmAssertReadPtr(inAnimation, sizeof(TRtAnimation));

	return inAnimation->toState;
}


TRtAnimTime TRrAnimation_GetDuration(const TRtAnimation *inAnimation)
{
	UUmAssertReadPtr(inAnimation, sizeof(TRtAnimation));

	return inAnimation->duration;
}

void TRrAnimation_GetPosition(const TRtAnimation *inAnimation, TRtAnimTime inTime, M3tVector3D *outPosition)
{
	UUtUns16 frameNum = TRmAnimationTimeToFrame(inAnimation, inTime);
	UUtBool	 isFrame = TRmAnimationTimeIsFrame(inAnimation, inTime);

	UUmAssertReadPtr(inAnimation, sizeof(*inAnimation));
	UUmAssertWritePtr(outPosition, sizeof(*outPosition));
	UUmAssert(frameNum < inAnimation->numFrames);
	UUmAssert(inTime < inAnimation->duration);

	if (isFrame) {
		outPosition->x = inAnimation->positions[(frameNum * 2)];
		outPosition->z = inAnimation->positions[(frameNum * 2) + 1];
		outPosition->y = 0.f;
		
	}
	else {
		outPosition->x = 0;
		outPosition->y = 0;
		outPosition->z = 0;
	}

	UUmAssert(0.f == outPosition->y);

	return;
}

UUtBool TRrAnimation_IsSoundFrame(const TRtAnimation *inAnimation, TRtAnimTime inTime)
{
	UUtUns16 frameNum = TRmAnimationTimeToFrame(inAnimation, inTime);
	UUtBool  isFrame = TRmAnimationTimeIsFrame(inAnimation, inTime);
	UUtBool  result = UUcFalse;
	
	UUmAssertReadPtr(inAnimation, sizeof(*inAnimation));
	UUmAssert(frameNum < inAnimation->numFrames);
	UUmAssert(inTime < inAnimation->duration);

	if (isFrame) {
		if (NULL == (char*)inAnimation->soundName) {
			if (0 == frameNum) {
				result = UUcTrue;
			}
		}
		else if (frameNum == inAnimation->soundFrame) {
			result = UUcTrue;
		}
	}
	
	return result;
}

const char* TRrAnimation_GetSoundName(const TRtAnimation *inAnimation)
{
	UUmAssertReadPtr(inAnimation, sizeof(*inAnimation));
	return (const char*)inAnimation->soundName;
}

const char* TRrAnimation_GetNewSoundForFrame(const TRtAnimation *inAnimation, TRtAnimTime inTime)
{
	UUtUns16 frameNum = TRmAnimationTimeToFrame(inAnimation, inTime);
	UUtBool  isFrame = TRmAnimationTimeIsFrame(inAnimation, inTime);
	const char *result = NULL;
	
	UUmAssertReadPtr(inAnimation, sizeof(*inAnimation));
	UUmAssert(frameNum < inAnimation->numFrames);
	UUmAssert(inTime < inAnimation->duration);

	if ((isFrame) && (inAnimation->numNewSounds > 0) && (inAnimation->newSounds != NULL)) {
		UUtUns32 itr;		
		for (itr = 0; itr < inAnimation->numNewSounds; itr++) {
			if (frameNum != inAnimation->newSounds[itr].frameNum) { continue; }
			result = inAnimation->newSounds[itr].impulseName;
			break;
		}
	}
	
	return result;
}


void TRrAnimation_SetSoundName(TRtAnimation *inAnimation, const char *inSoundName, UUtUns16 inFrame)
{
	UUmAssert(inAnimation);
	
	inAnimation->soundName = (UUtUns32)inSoundName;
	inAnimation->soundFrame = inFrame;
}

const char *TRrAnimation_GetName(const TRtAnimation *inAnimation)
{
	return inAnimation->instanceName;
}

float TRrAnimation_GetHeight(const TRtAnimation *inAnimation, TRtAnimTime inTime)
{
	float floatFrameNum = UUmMin(inAnimation->numFrames - 1, ((inTime) * (inAnimation->fps)) / 60.f);
	UUtUns16 frameNum = TRmAnimationTimeToFrame(inAnimation, inTime);
	float height;

	if (NULL == inAnimation->heights) {
		return 6.f;
	}

	if (floatFrameNum == frameNum) {
		height = inAnimation->heights[frameNum];
	}
	else {
		float from = inAnimation->heights[frameNum];
		float to = inAnimation->heights[frameNum + 1];
		float dist = floatFrameNum - frameNum;
		
		height = (from * (1 - dist)) + (to * dist);
	}

	UUmAssert((height > -128.f) && (height < 128.f));

	return height;
}

static const TRtAttack *TRrAnimation_GetAttack(const TRtAnimation *inAnimation, TRtAnimTime inTime, UUtUns32 inPartIndex)
{
	UUtUns16 frameNum = TRmAnimationTimeToFrame(inAnimation, inTime);
	UUtUns16 attackIndex;
	const TRtAttack *result = NULL;

	for(attackIndex = 0; attackIndex < inAnimation->numAttacks; attackIndex++) {
		const TRtAttack *attack = inAnimation->attacks + attackIndex;

		if ((frameNum >= attack->firstDamageFrame) && (frameNum <= attack->lastDamageFrame))
		{
			result = attack;
			break;
		}
	}

	return result;
}

UUtUns16 TRrAnimation_GetActiveParticles(const TRtAnimation *inAnimation, TRtAnimTime inTime)
{
	UUtUns16 frameNum = TRmAnimationTimeToFrame(inAnimation, inTime);
	UUtUns16 itr, result = 0;
	TRtParticle *particle;

	for (itr = 0, particle = inAnimation->particles; itr < inAnimation->numParticles; itr++, particle++) {
		if ((frameNum >= particle->startFrame) && (frameNum <= particle->stopFrame))
		{
			result |= (1 << itr);
		}
	}

	return result;
}

UUtUns32 TRrAnimation_GetDamage(const TRtAnimation *inAnimation, TRtAnimTime inTime, UUtUns32 inPartIndex)
{
	UUtUns16 frameNum = TRmAnimationTimeToFrame(inAnimation, inTime);
	UUtUns16 attackIndex;
	UUtUns32 damage = 0;

	for(attackIndex = 0; attackIndex < inAnimation->numAttacks; attackIndex++) {
		const TRtAttack *attack = inAnimation->attacks + attackIndex;

		if ((frameNum >= attack->firstDamageFrame) && (frameNum <= attack->lastDamageFrame))
		{
			if (attack->damageBits & (1 << inPartIndex)) {
				damage = attack->damage;
			}
		}
	}

	return damage;
}

UUtUns32 TRrAnimation_GetMaximumDamage(const TRtAnimation *inAnimation)
{
	UUtUns16 attackIndex;
	UUtUns32 damage = 0;

	for(attackIndex = 0; attackIndex < inAnimation->numAttacks; attackIndex++) {
		const TRtAttack *attack = inAnimation->attacks + attackIndex;
		
		damage = UUmMax(damage, attack->damage);
	}
		
	return damage;
}

UUtUns32 TRrAnimation_GetMaximumSelfDamage(const TRtAnimation *inAnimation)
{
	UUtUns32 itr, damage = 0;

	for(itr = 0; itr < inAnimation->numTakeDamages; itr++) {
		damage += inAnimation->takeDamages[itr].damage;
	}
		
	return damage;
}

UUtUns16 TRrAnimation_GetThrowType(const TRtAnimation *inAnimation)
{
	if (inAnimation->throwInfo == NULL) {
		return 0;
	} else {
		return inAnimation->throwInfo->throwType;
	}
}

UUtUns16 TRrAnimation_GetVocalization(const TRtAnimation *inAnimation)
{
	return inAnimation->vocalizationType;
}

UUtBool TRrAnimation_TestAttackFlag(const TRtAnimation *inAnimation, TRtAnimTime inTime, UUtUns32 inPartIndex, TRtAttackFlag inWhichFlag)
{
	UUtBool result = UUcFalse;
	UUtUns32 mask = 1 << inWhichFlag;

	if (inTime == (TRtAnimTime) -1) {
		// sample all attacks
		UUtUns16 attackIndex;

		for(attackIndex = 0; attackIndex < inAnimation->numAttacks; attackIndex++) {
			if (inAnimation->attacks[attackIndex].flags & mask) {
				return UUcTrue;
			}
		}

	} else {
		// sample the attack that is active at a specific time
		const TRtAttack *attack  = TRrAnimation_GetAttack(inAnimation, inTime, inPartIndex);

		if (NULL != attack)
		{
			result = (attack->flags & mask) > 0;
		}
	}

	return result;
}

float TRrAnimation_GetKnockback(const TRtAnimation *inAnimation, TRtAnimTime inTime, UUtUns32 inPartIndex)
{
	UUtUns16 frameNum = TRmAnimationTimeToFrame(inAnimation, inTime);
	UUtUns16 attackIndex;
	float knockback = 0.f;

	for(attackIndex = 0; attackIndex < inAnimation->numAttacks; attackIndex++) {
		const TRtAttack *attack = inAnimation->attacks + attackIndex;

		if ((frameNum >= attack->firstDamageFrame) && (frameNum <= attack->lastDamageFrame))
		{
			if (attack->damageBits & (1 << inPartIndex)) {
				knockback = attack->knockback;
			}
		}
	}

	return knockback;
}



TRtAnimType TRrAnimation_GetDamageAnimation(const TRtAnimation *inAnimation, TRtAnimTime inTime, UUtUns32 inPartIndex)
{
	UUtUns16 frameNum = TRmAnimationTimeToFrame(inAnimation, inTime);
	UUtUns16 attackIndex;
	TRtAnimType damageAnimation = 0;

	for(attackIndex = 0; attackIndex < inAnimation->numAttacks; attackIndex++) {
		const TRtAttack *attack = inAnimation->attacks + attackIndex;

		if ((frameNum >= attack->firstDamageFrame) && (frameNum <= attack->lastDamageFrame))
		{
			if (attack->damageBits & (1 << inPartIndex)) {
				damageAnimation = attack->damageAnimation;
			}
		}
	}

	return damageAnimation;
}

UUtBool	TRrAnimation_TestFlag(const TRtAnimation *inAnimation, TRtAnimFlag inWhichFlag)
{
	UUtBool set = (inAnimation->flags & (1 << inWhichFlag)) > 0;

	return set;
}

/*SStSound *TRrAnimation_GetHitSound(const TRtAnimation *inAnimation, TRtAnimTime inTime, UUtUns32 inPartIndex)
{
	UUtUns16 frameNum = TRmAnimationTimeToFrame(inAnimation, inTime);
	UUtUns16 attackIndex;
	SStSound *hitSound = NULL;

	for(attackIndex = 0; attackIndex < inAnimation->numAttacks; attackIndex++) {
		const TRtAttack *attack = inAnimation->attacks + attackIndex;

		if ((frameNum >= attack->firstDamageFrame) && (frameNum <= attack->lastDamageFrame))
		{
			if (attack->damageBits & (1 << inPartIndex)) {
				hitSound = attack->hitSound;
			}
		}
	}

	return hitSound;
}*/

UUtBool		TRrAnimation_IsDirect(const TRtAnimation *inFromAnim, const TRtAnimation *inToAnim)
{
	UUtUns16 itr;

	for(itr = 0; itr < TRcMaxDirectAnimations; itr++) {
		if (inFromAnim->directAnimation[itr] == inToAnim) {
			return UUcTrue;
		}
	}

	return UUcFalse;
}

UUtUns16	TRrAnimation_GetHardPause(const TRtAnimation *inFromAnim, const TRtAnimation *inToAnim)
{
	UUtUns16 hardPause;
	
	if (TRrAnimation_IsDirect(inFromAnim, inToAnim)) {
		hardPause = 0;
	}
	else {
		hardPause = inFromAnim->hardPause;
	}

	return hardPause;
}

UUtUns16	TRrAnimation_GetSoftPause(const TRtAnimation *inFromAnim, const TRtAnimation *inToAnim)
{
	UUtUns16 softPause;
	
	if (TRrAnimation_IsDirect(inFromAnim, inToAnim)) {
		softPause = 0;
	}
	else {
		softPause = inFromAnim->softPause;

	}

	return softPause;
}

UUtBool	TRrAnimation_IsAttack(const TRtAnimation *inAnimation)
{
	UUtBool isAttack = UUcFalse;
	UUtUns16 itr;

	for(itr = 0; itr < inAnimation->numAttacks; itr++) {
		const TRtAttack *attack = inAnimation->attacks + itr;

		if (attack->firstDamageFrame > attack->lastDamageFrame) {
			continue;
		}

		if (0 == attack->damageBits) {
			continue;
		}

		if (0 == attack->damage) {
			continue;
		}

		isAttack = UUcTrue;
		break;
	}

	if (inAnimation->throwInfo != NULL) {
		isAttack = UUcTrue;
	}

	return isAttack;

}

#define TRcAnimation_NoSuchShortcut ((UUtUns16) 0xffff)

static UUtUns16 TRrAnimation_GetShortcutIndex(const TRtAnimation *inAnimation, TRtAnimState fromState)
{
	UUtUns16 curShortcut;

	if (fromState == inAnimation->fromState) {
		return TRcAnimation_NoSuchShortcut;
	}

	// look for a normal shortcut
	for(curShortcut = 0; curShortcut < inAnimation->numShortcuts; curShortcut++)
	{
		if (inAnimation->shortcuts[curShortcut].state == fromState) {
			return curShortcut;
		}
	}

	// look for an anything shortcut
	for(curShortcut = 0; curShortcut < inAnimation->numShortcuts; curShortcut++)
	{
		if (inAnimation->shortcuts[curShortcut].state == TRcAnimState_Anything) {
			return curShortcut;
		}
	}

	return TRcAnimation_NoSuchShortcut;
}

UUtBool TRrAnimation_IsShortcut(const TRtAnimation *inAnimation, TRtAnimState fromState)
{
	UUtUns16 shortcutIndex = TRrAnimation_GetShortcutIndex(inAnimation, fromState);
	UUtBool isShortcut = shortcutIndex != TRcAnimation_NoSuchShortcut;

	return isShortcut;
}

UUtUns8	TRrAnimation_GetShortcutLength(const TRtAnimation *inAnimation, TRtAnimState fromState)
{
	UUtUns16 shortcutIndex = TRrAnimation_GetShortcutIndex(inAnimation, fromState);
	UUtUns8 shortcutLength = 0;

	UUmAssert(shortcutIndex != TRcAnimation_NoSuchShortcut);

	if (shortcutIndex != TRcAnimation_NoSuchShortcut) {
		shortcutLength = (UUtUns8) (inAnimation->shortcuts[shortcutIndex].length);
	}

	UUmAssert(shortcutLength > 0);

	return shortcutLength;
}

UUtBool	TRrAnimation_TestShortcutFlag(const TRtAnimation *inAnimation, TRtAnimState fromState, TRtShorcutFlag inWhichFlag)
{
	UUtUns16 shortcutIndex = TRrAnimation_GetShortcutIndex(inAnimation, fromState);
	UUtBool flagSet = UUcFalse;

	UUmAssert(shortcutIndex != TRcAnimation_NoSuchShortcut);

	if (shortcutIndex != TRcAnimation_NoSuchShortcut) {
		flagSet = (inAnimation->shortcuts[shortcutIndex].flags & (1 << inWhichFlag)) > 0;
	}

	return flagSet;
}

float TRrAnimation_GetFinalRotation(const TRtAnimation *inAnimation)
{
	UUmAssertReadPtr(inAnimation, sizeof(*inAnimation));

	return inAnimation->finalRotation;
}

UUtBool TRrAnimation_IsAtomic(const TRtAnimation *inAnimation, TRtAnimTime inTime)
{
	UUtBool atomic;

	UUmAssertReadPtr(inAnimation, sizeof(*inAnimation));

	atomic = (inTime >= inAnimation->atomicStart) && (inTime <= inAnimation->atomicEnd);

	return atomic;
}

UUtUns16	TRrAnimation_GetEndInterpolation(const TRtAnimation *inFromAnim, const TRtAnimation *inToAnim)
{	
#define cVarientInterpolation 8
	UUtUns16 endInterpolation = 0;

	if (TRrAnimation_IsShortcut(inToAnim, inFromAnim->toState)) {
		endInterpolation = TRrAnimation_GetShortcutLength(inToAnim, inFromAnim->toState);
	}
	else if (inFromAnim->toState != inToAnim->fromState) {
		COrConsole_Printf("invalid end interpolation, handling");
		endInterpolation = cVarientInterpolation;
	}

	if (inFromAnim->varient != inToAnim->varient) {
		endInterpolation = UUmMax(endInterpolation, cVarientInterpolation);
	}

	if (!TRrAnimation_IsDirect(inFromAnim, inToAnim)) {
		endInterpolation = UUmMax(endInterpolation, inFromAnim->endInterpolation);
	}

	UUmAssert(endInterpolation < 60);
	
	return endInterpolation;
}

UUtUns16	TRrAnimation_GetMaxInterpolation(const TRtAnimation	*inNewAnim)
{
	return inNewAnim->maxInterpolation;
}



void TRrOverlay_Apply(
		const TRtAnimation *inAnimation,
		TRtAnimTime inTime,
		const M3tQuaternion *inQuaternions,
		M3tQuaternion *outQuaternions,
		UUtUns16 inNumParts)
{
	UUtUns16	interpolation_frames = 6;
	UUtUns16	node_index;
	float		interpolate_amount = 1.f;

	if (inTime < interpolation_frames) {
		interpolate_amount = ((float) inTime) / ((float) interpolation_frames);
	}
	else if (inTime >= (inAnimation->duration - interpolation_frames)) {
		interpolate_amount = ((float) inAnimation->duration - inTime) / ((float) interpolation_frames);
	}
	else {
		interpolate_amount = 1.f;
	}
	
	TRrQuatArray_SetAnimation(inAnimation, inTime, TRgQuatArrayPrivate_Aiming_1);

	for (node_index= 0; node_index < inNumParts; ++node_index)
	{
		if (inAnimation->replaceParts & (1 << node_index)) {
			if (interpolate_amount != 1.f) {
				MUrQuat_Lerp(
					inQuaternions + node_index,
					TRgQuatArrayPrivate_Aiming_1 + node_index,
					interpolate_amount,
					outQuaternions + node_index);
			}
			else {
				outQuaternions[node_index] = TRgQuatArrayPrivate_Aiming_1[node_index];
			}
		}
		else if (inAnimation->usedParts & (1 << node_index)) {
			if (interpolate_amount != 1.f) {
				M3tQuaternion interpolated_quaternion;

				MUrQuat_Lerp(&MUgZeroQuat, TRgQuatArrayPrivate_Aiming_1 + node_index, interpolate_amount, &interpolated_quaternion);
				MUrQuat_Mul(&interpolated_quaternion, inQuaternions + node_index, outQuaternions + node_index);
			}
			else {
				MUrQuat_Mul(TRgQuatArrayPrivate_Aiming_1 + node_index, inQuaternions + node_index, outQuaternions + node_index);
			}
		}
		else {
			outQuaternions[node_index] = inQuaternions[node_index];
		}

	}

	return;
}

UUtBool TRrAnimation_GetBlur(const TRtAnimation *inAnimation, TRtAnimTime inTime, UUtUns32 *outParts, UUtUns8 *outQuantity, UUtUns8 *outAmount)
{
	UUtBool found = UUcFalse;
	UUtUns8 itr;

	for(itr = 0; itr < inAnimation->numBlurs; itr++)
	{
		const TRtBlur *curBlur = inAnimation->blurs + itr;

		if ((inTime >= curBlur->firstBlurFrame) && (inTime <= curBlur->lastBlurFrame))
		{
			TRtAnimTime delta_time = inTime - curBlur->firstBlurFrame;

			if (0 == (delta_time % curBlur->blurSkip))
			{
				*outParts = curBlur->blurParts;
				*outQuantity = curBlur->blurQuantity;
				*outAmount = curBlur->blurAmount;
				found = UUcTrue;

				break;
			}
		}
	}

	if (!found)
	{
		*outParts = 0;
		*outQuantity = 0;
		*outAmount = 0;
	}

	return found;
}

const TRtAimingScreen *TRrAimingScreen_Lookup(
	const TRtScreenCollection *inScreenCollection,
	TRtAnimType inAnimType,
	TRtAnimVarient inVarient)
{
	UUtUns16 index;
	const TRtAimingScreen *result = NULL;
	UUtUns16 skipVarient = ~inVarient;

	UUmAssertReadPtr(inScreenCollection, sizeof(inScreenCollection));

	for(index = 0;index < inScreenCollection->numScreens; index++)
	{
		UUtBool animTypeMatch;
		const TRtAimingScreen *curScreen = inScreenCollection->screen[index];
#if defined(DEBUGGING) && DEBUGGING
		const char *instanceName;
#endif

		if (NULL == curScreen) { continue; }

#if defined(DEBUGGING) && DEBUGGING
		instanceName = TMrInstance_GetInstanceName(curScreen);
#endif

		if (curScreen->animation->varient & skipVarient) {
			continue;
		}

		{
			TRtAnimType curType = curScreen->animation->type;

			animTypeMatch = (curType == inAnimType);
		}

		if (!animTypeMatch) {
			continue;
		}

		if (NULL == result) {
			result = curScreen;
		}

		if (curScreen->animation->varient > result->animation->varient) {
			result = curScreen;
		}
	}

	return result;
}


void TRrAnimation_GetAttacks(
		const TRtAnimation *inAnimation, 
		TRtAnimTime inTime, 
		UUtUns8 *outNumAttacks, 
		TRtAttack *outAttacks,
		UUtUns8 *outindices)
{
	UUtUns16 frameNum = TRmAnimationTimeToFrame(inAnimation, inTime);
	UUtUns8 attackIndex;
	UUtUns32 damage = 0;

	UUmAssertReadPtr(inAnimation, sizeof(*inAnimation));
	UUmAssertWritePtr(outNumAttacks, sizeof(*outNumAttacks));
	UUmAssertWritePtr(outAttacks, sizeof(TRtAttack) * TRcMaxAttacks);
	UUmAssertWritePtr(outindices, sizeof(UUtUns8) * TRcMaxAttacks);

	*outNumAttacks = 0;

	for(attackIndex = 0; attackIndex < inAnimation->numAttacks; attackIndex++) {
		const TRtAttack *attack = inAnimation->attacks + attackIndex;

		if ((frameNum >= attack->firstDamageFrame) && (frameNum <= attack->lastDamageFrame))
		{
			*outAttacks = *attack;
			*outindices = attackIndex;
			outAttacks++;

			*outNumAttacks += 1;
		}
	}

	return;
}

void TRrAnimation_GetParticles(
		const TRtAnimation *inAnimation,
		UUtUns8 *outNumParticles,
		TRtParticle **outParticles)
{
	*outNumParticles = inAnimation->numParticles;
	*outParticles = inAnimation->particles;

	return;
}

UUtUns16 TRrAnimation_GetSelfDamage(const TRtAnimation *inAnim, TRtAnimTime inTime)
{
	UUtUns16 itr;
	UUtUns16 damage = 0;

	for(itr = 0; itr < inAnim->numTakeDamages; itr++) {
		if (inAnim->takeDamages[itr].frame == inTime) {
			damage += inAnim->takeDamages[itr].damage;
		}
	}

	return damage;
}

void TRrVerifyAnimation(const TRtAnimation *inAnimation)
{
	M3tQuaternion verifyQuatArray[TRcMaxParts];
	UUtUns16 frame;
	UUtUns16 part;

	for(frame = 0; frame < inAnimation->numFrames; frame++)
	{
		TRrQuatArray_SetAnimationInternal(inAnimation, frame, verifyQuatArray);

		for(part = 0; part < inAnimation->numParts; part++)
		{
			MUmQuat_VerifyUnit(verifyQuatArray + part);			
		}
	}

	return;
}

const TRtAnimation *TRrAnimation_GetFromName(const char *inTemplateName)
{
	UUtError error;
	void *dataPtr;
	TRtAnimation *animation = NULL;

	error = TMrInstance_GetDataPtr(TRcTemplate_Animation, inTemplateName, &dataPtr);

	if (UUcError_None == error) {
		animation = dataPtr;
	}
	

	return animation;
}


float TRrAnimation_GetThrowDistance(const TRtAnimation *inAnim)
{
	UUmAssertReadPtr(inAnim, sizeof(*inAnim));
	if (inAnim->throwInfo == NULL) {
		return 0;
	} else {
		return inAnim->throwInfo->throwDistance;
	}
}

TRtDirection TRrAnimation_GetDirection(const TRtAnimation *inAnim, UUtBool inCheckAttackDirection)
{
	TRtDirection direction;

	UUmAssertReadPtr(inAnim, sizeof(*inAnim));

	direction = (TRtDirection)inAnim->moveDirection;

	if ((direction == TRcDirection_None) && inCheckAttackDirection) {
		direction = (TRtDirection)inAnim->extentInfo.computed_attack_direction;
	}

	return direction;
}

const TRtExtentInfo *TRrAnimation_GetExtentInfo(const TRtAnimation *inAnim)
{
	UUmAssertReadPtr(inAnim, sizeof(*inAnim));

	return &inAnim->extentInfo;
}

const char *TRrAnimation_GetAttackName(const TRtAnimation *inAnimation)
{
	UUmAssertReadPtr(inAnimation, sizeof(*inAnimation));
	return inAnimation->attackName;
}

UUtBool	TRrAnimation_HasBlockStun(const TRtAnimation *inAnimation, UUtUns16 inAmount)
{
	UUtUns32 attackIndex;
	const TRtAttack *attack;

	for (attackIndex = 0, attack = inAnimation->attacks; attackIndex < inAnimation->numAttacks; attackIndex++, attack++) {
		if (attack->blockStun >= inAmount) {
			return UUcTrue;
		}
	}

	return UUcFalse;
}

UUtBool	TRrAnimation_HasStagger(const TRtAnimation *inAnimation)
{
	UUtUns32 attackIndex;
	const TRtAttack *attack;

	for (attackIndex = 0, attack = inAnimation->attacks; attackIndex < inAnimation->numAttacks; attackIndex++, attack++) {
		if (attack->stagger > 0) {
			return UUcTrue;
		}
	}

	return UUcFalse;
}

UUtUns16 TRrAnimation_GetActionFrame(const TRtAnimation *inAnimation)
{
	return inAnimation->actionFrame;
}

TRtAnimTime TRrAnimation_GetFirstAttackFrame(const TRtAnimation *inAnim)
{
	UUmAssertReadPtr(inAnim, sizeof(*inAnim));
	UUmAssertReadPtr(inAnim->extentInfo.attackExtents, inAnim->extentInfo.numAttackExtents * sizeof(TRtAttackExtent));

	return inAnim->extentInfo.attackExtents[0].frame_index;
}

TRtAnimTime TRrAnimation_GetLastAttackFrame(const TRtAnimation *inAnim)
{
	UUmAssertReadPtr(inAnim, sizeof(*inAnim));
	UUmAssertReadPtr(inAnim->extentInfo.attackExtents, inAnim->extentInfo.numAttackExtents * sizeof(TRtAttackExtent));

	if (NULL == inAnim->extentInfo.attackExtents) {
		UUmAssert(!"TRrAnimation_GetLastAttackFrame called w/o attack");
		return 0;
	}

	return inAnim->extentInfo.attackExtents[inAnim->extentInfo.numAttackExtents - 1].frame_index;
}

TRtPosition *TRrAnimation_GetPositionPoint(const TRtAnimation *inAnimation, TRtAnimTime inTime)
{
	UUtUns16 frameNum = TRmAnimationTimeToFrame(inAnimation, inTime);

	if (NULL == inAnimation->positionPoints) {
		return NULL;
	}

	return inAnimation->positionPoints + frameNum;
}

void TRrAimingScreen_Clip(
	const TRtAimingScreen *inAimingScreen,
	float *ioDirection,
	float *ioElevation)
{
	if (NULL == inAimingScreen) {
		*ioDirection = 0.f;
		*ioElevation = 0.f;
	}
	else { 
		float min_direction = -(inAimingScreen->negativeYawDelta * inAimingScreen->negativeYawFrameCount);
		float max_direction = inAimingScreen->positiveYawDelta * inAimingScreen->positiveYawFrameCount;
		float min_elevation = -(inAimingScreen->negativePitchDelta * inAimingScreen->negativePitchFrameCount);
		float max_elevation = inAimingScreen->positivePitchDelta * inAimingScreen->positivePitchFrameCount;

		*ioDirection = UUmPin(*ioDirection, min_direction, max_direction);
		*ioElevation = UUmPin(*ioElevation, min_elevation, max_elevation);
	}

	return;
}

const TRtAnimation *TRrAimingScreen_GetAnimation(
	const TRtAimingScreen *inAimingScreen)
{
	return inAimingScreen->animation;
}

void TRrAnimation_SwapAttacks(TRtAnimation *animation)
{
	UUtUns16 itr;

	for(itr = 0; itr < animation->numAttacks; itr++)
	{
		TRtAttack *attack = animation->attacks + itr;

		UUmSwapLittle_4Byte(&attack->damageBits);
		UUmSwapLittle_4Byte(&attack->knockback);
		UUmSwapLittle_4Byte(&attack->flags);
		UUmSwapLittle_2Byte(&attack->damage);
		UUmSwapLittle_2Byte(&attack->firstDamageFrame);
		UUmSwapLittle_2Byte(&attack->lastDamageFrame);
		UUmSwapLittle_2Byte(&attack->damageAnimation);
		UUmSwapLittle_2Byte(&attack->hitStun);
		UUmSwapLittle_2Byte(&attack->blockStun);
		UUmSwapLittle_2Byte(&attack->stagger);
		UUmSwapLittle_2Byte(&attack->precedence);
		UUmSwapLittle_2Byte(&attack->attackExtentIndex);

		UUmAssert(4 == sizeof(attack->damageBits));
		UUmAssert(4 == sizeof(attack->knockback));
		UUmAssert(4 == sizeof(attack->flags));
		UUmAssert(2 == sizeof(attack->damage));
		UUmAssert(2 == sizeof(attack->firstDamageFrame));
		UUmAssert(2 == sizeof(attack->lastDamageFrame));
		UUmAssert(2 == sizeof(attack->damageAnimation));
		UUmAssert(2 == sizeof(attack->hitStun));
		UUmAssert(2 == sizeof(attack->blockStun));
		UUmAssert(2 == sizeof(attack->stagger));
		UUmAssert(2 == sizeof(attack->precedence));
		UUmAssert(2 == sizeof(attack->attackExtentIndex));

		UUmAssert(32 == sizeof(TRtAttack));	// if you change TRtAttack you need to update this byteswapping
	}

	return;
}

void TRrAnimation_SwapTakeDamages(TRtAnimation *animation)
{
	UUtUns16 itr;

	for(itr = 0; itr < animation->numTakeDamages; itr++)
	{
		TRtTakeDamage *takeDamage = animation->takeDamages + itr;

		UUmSwapLittle_2Byte(&takeDamage->damage);
		UUmSwapLittle_2Byte(&takeDamage->frame);

		UUmAssert(2 == sizeof(takeDamage->damage));
		UUmAssert(2 == sizeof(takeDamage->frame));
	}

	return;
}

void TRrAnimation_SwapBlurs(TRtAnimation *animation)
{
	UUtUns16 itr;

	for(itr = 0; itr < animation->numBlurs; itr++)
	{
		TRtBlur *blur = animation->blurs + itr;

		UUmSwapLittle_4Byte(&blur->blurParts);
		UUmSwapLittle_2Byte(&blur->firstBlurFrame);
		UUmSwapLittle_2Byte(&blur->lastBlurFrame);

		UUmAssert(4 == sizeof(blur->blurParts));
		UUmAssert(2 == sizeof(blur->firstBlurFrame));
		UUmAssert(2 == sizeof(blur->lastBlurFrame));
	}

	return;
}

void TRrAnimation_SwapShortcuts(TRtAnimation *animation)
{
	UUtUns16 itr;

	for(itr = 0; itr < animation->numShortcuts; itr++)
	{
		TRtShortcut *shortcut = animation->shortcuts + itr;

		UUmSwapLittle_2Byte(&shortcut->state);
		UUmSwapLittle_2Byte(&shortcut->length);
		UUmSwapLittle_4Byte(&shortcut->flags);

		UUmAssert(2 == sizeof(shortcut->state));
		UUmAssert(2 == sizeof(shortcut->length));
		UUmAssert(4 == sizeof(shortcut->flags));
	}

	return;
}

void TRrAnimation_SwapHeights(TRtAnimation *animation)
{
	if (NULL != animation->heights) {
		UUmSwapLittle_4Byte_Array(animation->heights, animation->numFrames);
	}

	return;
}

void TRrAnimation_SwapPositions(TRtAnimation *animation)
{
	if (NULL != animation->positions) {
		UUmSwapLittle_4Byte_Array(animation->positions, animation->numFrames * 2);
	}

	return;
}

void TRrAnimation_SwapFootsteps(TRtAnimation *animation)
{
	UUtUns16 itr;

	for(itr = 0; itr < animation->numFootsteps; itr++) 
	{
		TRtFootstep *footstep = animation->footsteps + itr;

		UUmSwapLittle_2Byte(&footstep->frame);
		UUmSwapLittle_2Byte(&footstep->type);
	}

	return;
}

void TRrAnimation_SwapThrowInfo(TRtAnimation *animation)
{
	if (NULL != animation->throwInfo) {
		TRtThrowInfo *throwInfo = animation->throwInfo;

		UUmSwapLittle_4Byte(&throwInfo->throwOffset.x);
		UUmSwapLittle_4Byte(&throwInfo->throwOffset.y);
		UUmSwapLittle_4Byte(&throwInfo->throwOffset.z);

		UUmSwapLittle_2Byte(&throwInfo->throwType);
		UUmSwapLittle_4Byte(&throwInfo->relativeThrowFacing);
		UUmSwapLittle_4Byte(&throwInfo->throwDistance);
	}

	return;
}

void TRrAnimation_SwapParticles(TRtAnimation *animation)
{
	UUtUns16 itr;
	TRtParticle *particle;

	for(itr = 0, particle = animation->particles; itr < animation->numParticles; itr++, particle++)
	{
		UUmSwapLittle_2Byte(&particle->startFrame);
		UUmSwapLittle_2Byte(&particle->stopFrame);
		UUmSwapLittle_2Byte(&particle->partIndex);

		UUmAssert(2 == sizeof(particle->startFrame));
		UUmAssert(2 == sizeof(particle->stopFrame));
		UUmAssert(2 == sizeof(particle->partIndex));
	}

	return;
}

void TRrAnimation_SwapExtentInfo(TRtAnimation *animation)
{
	UUtUns16 itr;
	TRtAttackExtent *extent;

	for(itr = 0, extent = animation->extentInfo.attackExtents; itr < animation->extentInfo.numAttackExtents; itr++, extent++)
	{
		UUmSwapLittle_2Byte(&extent->frame_index);
		UUmSwapLittle_2Byte(&extent->attack_angle);
		UUmSwapLittle_2Byte(&extent->attack_distance);
		UUmSwapLittle_2Byte(&extent->attack_minheight);
		UUmSwapLittle_2Byte(&extent->attack_maxheight);

		UUmAssert(2 == sizeof(extent->frame_index));
		UUmAssert(2 == sizeof(extent->attack_angle));
		UUmAssert(2 == sizeof(extent->attack_distance));
		UUmAssert(2 == sizeof(extent->attack_minheight));
		UUmAssert(2 == sizeof(extent->attack_maxheight));
	}

	return;
}

void TRrAnimation_SwapPositionPoints(TRtAnimation *animation)
{
	UUtUns16 itr;
	TRtPosition *position;

	for(itr = 0, position = animation->positionPoints; itr < animation->numFrames; itr++, position++)
	{
		UUmSwapLittle_2Byte(&position->toe_to_head_height);
		UUmSwapLittle_2Byte(&position->floor_to_toe_offset);
		UUmSwapLittle_2Byte(&position->location_x);
		UUmSwapLittle_2Byte(&position->location_y);

		UUmAssert(2 == sizeof(position->toe_to_head_height));
		UUmAssert(2 == sizeof(position->floor_to_toe_offset));
		UUmAssert(2 == sizeof(position->location_x));
		UUmAssert(2 == sizeof(position->location_y));
	}

	return;
}

void TRrAnimation_SwapNewSounds(TRtAnimation *animation)
{
	UUtUns32 itr;
	TRtSound *newSound;
	
	for (itr = 0, newSound = animation->newSounds; itr < animation->numNewSounds; itr++, newSound++)
	{
		UUmSwapLittle_2Byte(&newSound->frameNum);
		
		UUmAssert(2 == sizeof(newSound->frameNum));
	}
}


static void TRrAimingScreen_Prepare(
	const TRtAimingScreen *inAimingScreen,
	float inDirection,
	float inElevation,
	short inNumParts,
	M3tQuaternion *outQuat)
{
#if defined(DEBUGGING) && DEBUGGING
	const char *instance_name = TMrInstance_GetInstanceName(inAimingScreen);
#endif

	UUtInt16 direction_width= inAimingScreen->positiveYawFrameCount + inAimingScreen->negativeYawFrameCount + 1;
	UUtInt16 elevation_width= inAimingScreen->positivePitchFrameCount + inAimingScreen->negativePitchFrameCount + 1;
	TRtAnimation *animation = inAimingScreen->animation;
	M3tQuaternion *frame_d0_e0 = TRgQuatArrayPrivate_Aiming_1;
	M3tQuaternion *frame_d1_e0 = TRgQuatArrayPrivate_Aiming_2;
	M3tQuaternion *frame_d0_e1 = TRgQuatArrayPrivate_Aiming_3;
	M3tQuaternion *frame_d1_e1 = TRgQuatArrayPrivate_Aiming_4;
	float d0, e0;
	UUtBool valid= UUcFalse;

	UUmAssert(inNumParts < 32);	// we use an Uns32 as a bitvector

	inDirection *= -1;
	inElevation *= -1;

	UUmAssert(animation->numParts == inNumParts);
	UUmAssert(animation->numFrames >= elevation_width*direction_width);

	if (animation->numParts != inNumParts) {
		return;
	}

	if (animation->numFrames < elevation_width*direction_width) {
		return;
	}

	// compute e0, e1, d0, d1 and their respective frames
	{
		float direction_index_real, elevation_index_real;
		UUtInt16 direction_index, elevation_index;

	#if defined(USE_FAST_TRIG) && defined(USE_FAST_MATH) // see bfw_math.h
		float int_part;
	#else
		double int_part;
	#endif

		if (inDirection < 0.f) {
			if (0 == inAimingScreen->negativeYawFrameCount) {
				d0 = 0.f;
				direction_index = 0;
			}
			else {
				direction_index_real= inDirection / inAimingScreen->negativeYawDelta;
				d0= (float) modf(direction_index_real, &int_part);
				direction_index= (UUtInt16) -MUrUnsignedSmallFloat_To_Int_Round((float) -int_part);

				d0= 1.f+d0;
				direction_index-= 1;

				if (direction_index < -inAimingScreen->negativeYawFrameCount) {
					direction_index= -inAimingScreen->negativeYawFrameCount;
					d0= 0.f;
				}
			}
		}
		else {
			if (0 == inAimingScreen->positiveYawFrameCount) {
				d0 = 0.f;
				direction_index = 0;
			}
			else {
				direction_index_real= inDirection/inAimingScreen->positiveYawDelta;
				d0= (float) modf(direction_index_real, &int_part);
				direction_index= (UUtInt16) MUrUnsignedSmallFloat_To_Int_Round((float) int_part);

				if (direction_index >= inAimingScreen->positiveYawFrameCount) {
					direction_index= inAimingScreen->positiveYawFrameCount-1;
					d0= 1.f;
				}
			}
		}

		direction_index+= inAimingScreen->negativeYawFrameCount;

		if (inElevation < 0.f) {
			if (0 == inAimingScreen->negativePitchFrameCount) {
				e0 = 0.f;
				elevation_index = 0;
			}
			else {
				elevation_index_real= inElevation / inAimingScreen->negativePitchDelta;
				e0= (float) modf(elevation_index_real, &int_part);
				elevation_index = (UUtInt16) -MUrUnsignedSmallFloat_To_Int_Round((float) -int_part);

				e0= 1.f+e0;
				elevation_index-= 1;

				if (elevation_index < -inAimingScreen->negativePitchFrameCount) {
					elevation_index= -inAimingScreen->negativePitchFrameCount;
					e0= 0.f;
				}
			}
		}
		else {
			if (0 == inAimingScreen->positivePitchFrameCount) {
				e0 = 0.f;
				elevation_index = 0;
			}
			else {
				elevation_index_real= inElevation / inAimingScreen->positivePitchDelta;
				e0= (float) modf(elevation_index_real, &int_part);
				elevation_index = (UUtInt16) MUrUnsignedSmallFloat_To_Int_Round((float) int_part);

				if (elevation_index >= inAimingScreen->positivePitchFrameCount) {
					elevation_index= inAimingScreen->positivePitchFrameCount-1;
					e0= 1.f;
				}
			}
		}

		//COrConsole_Printf("%1.3f (%d %1.3f)", elevation_index + e0, elevation_index, e0);

		elevation_index+= inAimingScreen->negativePitchFrameCount;

		if (elevation_index>=0 && elevation_index+1<elevation_width &&
			direction_index>=0 && direction_index+1<direction_width)
		{
			TRrQuatArray_SetAnimation(animation, elevation_index*direction_width + direction_index, frame_d0_e0);
			TRrQuatArray_SetAnimation(animation, elevation_index*direction_width + (direction_index+1), frame_d1_e0);
			TRrQuatArray_SetAnimation(animation, (elevation_index+1)*direction_width + direction_index, frame_d0_e1);
			TRrQuatArray_SetAnimation(animation, (elevation_index+1)*direction_width + (direction_index+1), frame_d1_e1);
			
			valid= UUcTrue;
		}
	}

	if (!valid) {
		COrConsole_Printf("aiming screen is not valid");
	}

	// apply
	if (valid)
	{
		UUtInt16 node_index;

		for (node_index= 0; node_index < inNumParts; ++node_index)
		{
			M3tQuaternion e0_quaternion, e1_quaternion;
			M3tQuaternion interpolated_quaternion;

			// ** if this part isn't used we can skip it **
			if (0 == (animation->usedParts & (1 << node_index))) {
				continue;
			}

			// interpolate rotation
			//quaternions_interpolate(&frame_d0_e0->nodes[node_index].rotation, &frame_d1_e0->nodes[node_index].rotation, &e0_quaternion, d0);
			MUrQuat_Lerp(frame_d0_e0 + node_index, frame_d1_e0 + node_index, d0, &e0_quaternion); 
			//quaternions_interpolate(&frame_d0_e1->nodes[node_index].rotation, &frame_d1_e1->nodes[node_index].rotation, &e1_quaternion, d0);
			MUrQuat_Lerp(frame_d0_e1 + node_index, frame_d1_e1 + node_index, d0, &e1_quaternion); 
			//quaternions_interpolate(&e0_quaternion, &e1_quaternion, &interpolated_quaternion, e0);
			MUrQuat_Lerp(&e0_quaternion, &e1_quaternion, e0, &interpolated_quaternion); 

			outQuat[node_index] = interpolated_quaternion;
		}
	}

	return;
}

void TRrAimingScreen_Apply(
	const TRtAimingScreen *inAimingScreen,
	float inDirection,
	float inElevation,
	const M3tQuaternion *inQuaternions,
	M3tQuaternion *outQuaternions,
	short inNumParts)
{
	M3tQuaternion interpolated_quaternion[32];
	TRtAnimation *animation = inAimingScreen->animation;

	TRrAimingScreen_Prepare(inAimingScreen, inDirection, inElevation, inNumParts, interpolated_quaternion);

	// apply
	{
		UUtInt16 node_index;

		for (node_index= 0; node_index < inNumParts; ++node_index)
		{
			// ** if this part isn't used we can skip it **
			if (0 == (animation->usedParts & (1 << node_index))) {
				continue;
			}

			MUrQuat_Mul(interpolated_quaternion + node_index, inQuaternions + node_index, outQuaternions + node_index);
		}
	}

	return;
}

void TRrAimingScreen_Apply_Lerp(
	const TRtAimingScreen *inAimingScreen1,
	const TRtAimingScreen *inAimingScreen2,
	float inAmount2,
	float inDirection,
	float inElevation,
	const M3tQuaternion *inQuaternions,
	M3tQuaternion *outQuaternions,
	UUtInt16 inNumParts)
{
	UUtInt16 node_index;
	M3tQuaternion interpolated_quaternion_1[32];
	M3tQuaternion interpolated_quaternion_2[32];
	TRtAnimation *animation1 = NULL;
	TRtAnimation *animation2 = NULL;
	UUtUns32 used_1 = 0;
	UUtUns32 used_2 = 0;
	UUtUns32 used_either;

	if (inAimingScreen1 != NULL) {
		TRrAimingScreen_Prepare(inAimingScreen1, inDirection, inElevation, inNumParts, interpolated_quaternion_1);
		animation1 = inAimingScreen1->animation;
		if (animation1)
			used_1 = animation1->usedParts;
	}

	if (inAimingScreen2 != NULL) {
		TRrAimingScreen_Prepare(inAimingScreen2, inDirection, inElevation, inNumParts, interpolated_quaternion_2);
		animation2 = inAimingScreen2->animation;
		if (animation2)
			used_2 = animation2->usedParts;
	}


	used_either = used_1 | used_2;

	for (node_index= 0; node_index < inNumParts; ++node_index)
	{
		M3tQuaternion interpolated_quaternion;
		UUtBool is_used_1 = (used_1 & (1 << node_index)) > 0;
		UUtBool is_used_2 = (used_2 & (1 << node_index)) > 0;

		// ** if this part isn't used we can skip it **
		if (0 == (used_either & (1 << node_index))) {
			continue;
		}

		if (is_used_1 && is_used_2) {
			MUrQuat_Lerp(interpolated_quaternion_1 + node_index, interpolated_quaternion_2 + node_index, inAmount2, &interpolated_quaternion);
		}
		else if (is_used_1) {
			MUrQuat_Lerp(interpolated_quaternion_1 + node_index, &MUgZeroQuat, inAmount2, &interpolated_quaternion);
		}
		else if (is_used_2) {
			MUrQuat_Lerp(&MUgZeroQuat, interpolated_quaternion_2 + node_index, inAmount2, &interpolated_quaternion);
		}
		else {
			UUmAssert(!"invalid case in TRrAimingScreen_Apply_Lerp");
		}

		MUrQuat_Mul(&interpolated_quaternion, inQuaternions + node_index, outQuaternions + node_index);
	}

	return;
}

// ------------------------------------------------------------------------------------
// -- animation intersection

// *** debugging stuff for intersections
#define DEBUG_PRINTINTERSECTIONS		0

#if DEBUG_SHOWINTERSECTIONS
enum {
	cExit_NotAttack,
	cExit_NoLongerDamaging,
	cExit_IsLeft,
	cExit_IsBehind,
	cExit_IsRight,
	cExit_AirCyl_NoHit,
	cExit_AirCyl_LateHit,
	cExit_AirCyl_HeadingAway,
	cExit_OutsideDist_NoDot,
	cExit_OutsideDist_WithDot,
	cExit_AboveHead,
	cExit_UnderFeet,
	cExit_MissRing,
	cHit
};

typedef struct tExactPoint {
	M3tVector3D	attacker_location, target_location;

	float		target_minh, target_maxh;

	UUtBool		is_attack;
	float		attack_minheight, attack_maxheight, attack_dist;
	UUtUns16	attack_angle;

	UUtBool		hit, moved;
} tExactPoint;

typedef struct tIntersectionGlobals {
	UUtUns32 exit_type;
	UUtBool rejected_by_dir;
	UUtBool rejected_by_airhit;

	TRtAnimIntersectContext last_context;
	TRtExtentInfo *extentptr;
	UUtUns16 num_danger_frames;

	M3tPoint3D start_pt, end_pt;
	float targetvel_x, targetvel_y;

	TRtDirection attack_direction;
	float attackrel_x, attackrel_y, attackrel_vel_x, attackrel_vel_y;

	float bounds_sum, a, b, c, disc, t_0, t_1;
	float closingvel_x, closingvel_y;

	float distsq_to_start, distsq_to_end, distsq_to_reject, start_dot, end_dot;
	float mid_x, mid_y, distsq_to_mid;

	UUtUns32 start_itr, hit_line;


	UUtUns32 num_exact_points;
	tExactPoint exact_points[200];
} tIntersectionGlobals;

UUtUns32 gFramesSinceIntersection = (UUtUns32) -1;
tIntersectionGlobals gIntersection;
#endif

// *** globally-computed constants
static float TRgRingDirectionTable[TRcExtentRingSamples][2];
static float TRgAttackDirectionTable[256][2];

UUtError TRrInitialize_IntersectionTables(void)
{
	UUtUns32 itr;
	float theta;

	for (itr = 0; itr < TRcExtentRingSamples; itr++) {
		theta = itr * M3c2Pi / TRcExtentRingSamples;

		TRgRingDirectionTable[itr][0] = MUrSin(theta);
		TRgRingDirectionTable[itr][1] = MUrCos(theta);
	}

	for (itr = 0; itr < 256; itr++) {
		theta = (itr * 256 + 128) * TRcAngleGranularity;

		TRgAttackDirectionTable[itr][0] = MUrSin(theta);
		TRgAttackDirectionTable[itr][1] = MUrCos(theta);
	}

	return UUcError_None;
}

// *** constants for animation intersection
#define TRcIntersect_ArcRangeFraction		1.2
#define TRcIntersect_MinHeightOverlap		1.0f
#define TRcAttackSafetyMargin				3.0f

// quickly check the bounds of an attack animation
UUtBool TRrCheckAnimationBounds(TRtAnimIntersectContext *inContext)
{
	TRtDirection attack_direction;
	const TRtExtentInfo *attack_extentinfo;
	M3tVector3D target_pt, end_pt, target_vel;
	float target_radius;
	UUtUns16 num_danger_frames, last_danger_frame;
	UUtUns32 start_itr, itr;

	UUmAssert(TRrAnimation_IsAttack(inContext->attacker.animation));
	attack_extentinfo = (const TRtExtentInfo *) &inContext->attacker.animation->extentInfo;

#if DEBUG_SHOWINTERSECTIONS
	if (TRgStoreThisIntersection) {
		gFramesSinceIntersection = 0;
		gIntersection.last_context = *inContext;
		gIntersection.extentptr = (TRtExtentInfo *) attack_extentinfo;
		gIntersection.num_exact_points = 0;
		gIntersection.rejected_by_dir = UUcFalse;
		gIntersection.rejected_by_airhit = UUcFalse;
	}
#endif

	if (attack_extentinfo->numAttackExtents == 0) {
		// this isn't a real attack, it must be a throw. we shouldn't ever
		// consider throws using this function. and we can't respond to them using
		// it, anyway.
		UUmAssert(!inContext->consider_attack);
#if DEBUG_SHOWINTERSECTIONS
		if (TRgStoreThisIntersection) {
			gIntersection.exit_type = cExit_NotAttack;
		}
#endif
		return UUcFalse;
	}

	// how long do we have to worry about the attack?
	last_danger_frame = TRrAnimation_GetLastAttackFrame(inContext->attacker.animation);
	if (last_danger_frame < inContext->attacker.animFrame) {
		// this attack is past the point where it is damaging.
		UUmAssert(inContext->attacker.animFrame < TRrAnimation_GetDuration(inContext->attacker.animation));
#if DEBUG_SHOWINTERSECTIONS
		if (TRgStoreThisIntersection) {
			gIntersection.exit_type = cExit_NoLongerDamaging;
		}
#endif
		return UUcFalse;
	}
	num_danger_frames = last_danger_frame - inContext->attacker.animFrame;
	if (inContext->attacker.position.inAir) {
		// this is a jumping attack, we can attack at any time throughout the jump
		num_danger_frames = UUmMax(num_danger_frames, inContext->attacker.position.expectedJumpFrames);
	}

#if DEBUG_SHOWINTERSECTIONS
	if (TRgStoreThisIntersection) {
		gIntersection.num_danger_frames = num_danger_frames;
	}
#endif

	// transform target into attacker space
	target_pt = MUrMatrix_GetTranslation(&inContext->current_location_matrix);
	MUrMatrix3x3_MultiplyPoint(&inContext->target_velocity_estimate, (M3tMatrix3x3 *) &inContext->current_location_matrix,
								&target_vel);
	target_radius = inContext->target_cylinder_radius;

#if DEBUG_SHOWINTERSECTIONS
	if (TRgStoreThisIntersection) {
		gIntersection.targetvel_x = target_vel.x;
		gIntersection.targetvel_y = target_vel.y;
	}
#endif

#if DEBUG_PRINTINTERSECTIONS
	{
		M3tVector3D our_vector;

		MUrMatrix_GetCol(&inContext->current_location_matrix, 1, &our_vector);

		COrConsole_Printf("target in attackerspace: at %f %f %f, facing %f %f %f, velocity est %f %f",
			target_pt.x, target_pt.y, target_pt.z, our_vector.x, our_vector.y, our_vector.z, target_vel.x, target_vel.y);
	}
#endif

	attack_direction = TRrAnimation_GetDirection(inContext->attacker.animation, UUcTrue);
	UUmAssert(attack_direction != TRcDirection_None);
#if DEBUG_SHOWINTERSECTIONS
	if (TRgStoreThisIntersection) {
		gIntersection.attack_direction = attack_direction;
	}
#endif

	/*
	// CB: this direction discarding doesn't work well at all when in a close melee type situation, so I'm disabling it


	if (attack_direction != TRcDirection_360) {
		if (inContext->consider_attack) {
			// we will already have discarded attacks that were in the wrong direction, so we don't need
			// to consider this here.
		} else {
			float attackrel_x, attackrel_y, attackrel_vel_x, attackrel_vel_y;
			// we are considering whether we'll get hit by an incoming attack. trivial reject if
			// we are outside the cone of the attack (given by TRcIntersect_ArcRangeFraction).

			// get the position of the target relative to the attack's arc
			switch(attack_direction) {
				case TRcDirection_Forwards:
					attackrel_x =  target_pt.x;		attackrel_vel_x =  target_vel.x;
					attackrel_y =  target_pt.y;		attackrel_vel_y =  target_vel.y;
				break;

				case TRcDirection_Left:
					attackrel_x = -target_pt.y;		attackrel_vel_x = -target_vel.y;
					attackrel_y =  target_pt.x;		attackrel_vel_y =  target_vel.x;
				break;

				case TRcDirection_Back:
					attackrel_x = -target_pt.x;		attackrel_vel_x = -target_vel.x;
					attackrel_y = -target_pt.y;		attackrel_vel_y = -target_vel.y;
				break;

				case TRcDirection_Right:
					attackrel_x =  target_pt.y;		attackrel_vel_x =  target_vel.y;
					attackrel_y = -target_pt.x;		attackrel_vel_y = -target_vel.x;
				break;

				default:
					UUmAssert(0);
				break;
			}

#if DEBUG_SHOWINTERSECTIONS
			if (TRgStoreThisIntersection) {
				gIntersection.attackrel_x = attackrel_x;
				gIntersection.attackrel_y = attackrel_y;
				gIntersection.attackrel_vel_x = attackrel_vel_x;
				gIntersection.attackrel_vel_y = attackrel_vel_y;
			}
#endif
#if DEBUG_PRINTINTERSECTIONS
			COrConsole_Printf("check target vs attack's direction: at %f, %f, velocity est %f %f",
				attackrel_x, attackrel_y, attackrel_vel_x, attackrel_vel_y);
#endif

			if ((attackrel_y < 0) && (attackrel_y + attackrel_vel_y * num_danger_frames < 0)) {
				// the target lies behind the attack's arc
#if DEBUG_PRINTINTERSECTIONS
				COrConsole_Printf("lies behind arc, no hit.");
#endif
#if DEBUG_SHOWINTERSECTIONS
				if (TRgStoreThisIntersection) {
					gIntersection.exit_type = cExit_IsBehind;
					gIntersection.rejected_by_dir = UUcTrue;
				}
#endif
				return UUcFalse;
			}

			if (attackrel_x > attackrel_y * TRcIntersect_ArcRangeFraction) {
				// the target lies to the left of the attack's arc.
				if (attackrel_x + attackrel_vel_x * num_danger_frames >
					 (attackrel_y + attackrel_vel_y * num_danger_frames) * TRcIntersect_ArcRangeFraction) {
					// it will still lie to the left by the time that the danger has passed.
#if DEBUG_PRINTINTERSECTIONS
					COrConsole_Printf("lies to left of arc, no hit.");
#endif
#if DEBUG_SHOWINTERSECTIONS
					if (TRgStoreThisIntersection) {
						gIntersection.exit_type = cExit_IsLeft;
						gIntersection.rejected_by_dir = UUcTrue;
					}
#endif
					return UUcFalse;
				}
			} else if (attackrel_x < -attackrel_y * TRcIntersect_ArcRangeFraction) {
				// the target lies to the right of the attack's arc.
				if (attackrel_x + attackrel_vel_x * num_danger_frames <
					-(attackrel_y + attackrel_vel_y * num_danger_frames) * TRcIntersect_ArcRangeFraction) {
					// it will still lie to the left by the time that the danger has passed.
#if DEBUG_PRINTINTERSECTIONS
					COrConsole_Printf("lies to right of arc, no hit.");
#endif
#if DEBUG_SHOWINTERSECTIONS
					if (TRgStoreThisIntersection) {
						gIntersection.exit_type = cExit_IsRight;
						gIntersection.rejected_by_dir = UUcTrue;
					}
#endif
					return UUcFalse;
				}
			}
		}
	}
*/

	if (inContext->attacker.position.inAir) {
		float bounds_sum, a, b, c, disc, t_0, t_1;
		float closingvel_x, closingvel_y;

		// we can't trivially reject based on height or distance, because jumps are too hard to predict at this stage
		// and will need to be stepped through. so instead intersect 2D bounds of the target and our attack's
		// max extent to see if we can reject.
		bounds_sum = target_radius + attack_extentinfo->attack_ring.max_distance;

		// closing speed is the difference in velocities. note that airVelocity is in oni-coordinates relative
		// to the attacker so z -> y for Max space
		closingvel_x = target_vel.x - inContext->attacker.position.airVelocity.x;
		closingvel_y = target_vel.y - inContext->attacker.position.airVelocity.z;

#if DEBUG_PRINTINTERSECTIONS
		COrConsole_Printf("check bounds by sum - radius %f, closing vel %f, %f", bounds_sum, closingvel_x, closingvel_y);
#endif

		// work out if the target will come within bounds_sum of our position before the attack ends. ray vs cylinder,
		// in 2D, solve via quadratic equation (v^2) t^2 + (2uv) t + (u^2 - r^2) = 0
		a = UUmSQR(closingvel_x) + UUmSQR(closingvel_y);
		b = 2 * (target_pt.x * closingvel_x + target_pt.y * closingvel_y);
		c = UUmSQR(target_pt.x) + UUmSQR(target_pt.y) - UUmSQR(bounds_sum);

		disc = UUmSQR(b) - 4 * a * c;

#if DEBUG_SHOWINTERSECTIONS
		if (TRgStoreThisIntersection) {
			gIntersection.bounds_sum = bounds_sum;
			gIntersection.a = a;
			gIntersection.b = b;
			gIntersection.c = c;
			gIntersection.disc = disc;
			gIntersection.closingvel_x = closingvel_x;
			gIntersection.closingvel_y = closingvel_y;
		}
#endif
		if (c < 0) {
			// target_pt is in fact inside the circle to start with. don't even try rejecting.
			t_0 = 0;
			t_1 = 0;
#if DEBUG_PRINTINTERSECTIONS
			COrConsole_Printf("target_pt %f, %f inside bounds sum %f - don't bother checking quadratic. keep checking.",
				target_pt.x, target_pt.y, bounds_sum);
#endif
#if DEBUG_SHOWINTERSECTIONS
			if (TRgStoreThisIntersection) {
				gIntersection.t_0 = t_0;
				gIntersection.t_1 = t_1;
			}
#endif
		} else {
			if (disc < 0) {
				// we never get close enough together. no hit.
#if DEBUG_PRINTINTERSECTIONS
				COrConsole_Printf("quadratic equation: no hit. (%f %f %f -> disc %f)", a, b, c, disc);
#endif
#if DEBUG_SHOWINTERSECTIONS
				if (TRgStoreThisIntersection) {
					gIntersection.exit_type = cExit_AirCyl_NoHit;
					gIntersection.rejected_by_airhit = UUcTrue;
				}
#endif
				return UUcFalse;
			}

			// get the t-value of the first intersection
			if (a > 0) {
				t_0 = (-b - MUrSqrt(disc)) / (2 * a);
			} else {
				t_0 = (-b + MUrSqrt(disc)) / (2 * a);
			}

#if DEBUG_SHOWINTERSECTIONS
			if (TRgStoreThisIntersection) {
				gIntersection.t_0 = t_0;
			}
#endif
			if (t_0 > num_danger_frames) {
				// we won't get close enough together until after the attack is over. no hit.
#if DEBUG_PRINTINTERSECTIONS
				COrConsole_Printf("hit at %f frames > num danger frames %d. exit - no hit.", t_0, num_danger_frames);
#endif
#if DEBUG_SHOWINTERSECTIONS
				if (TRgStoreThisIntersection) {
					gIntersection.exit_type = cExit_AirCyl_LateHit;
					gIntersection.rejected_by_airhit = UUcTrue;
					gIntersection.t_1 = t_0;
				}
#endif
				return UUcFalse;
			}

			if (t_0 < 0.01f) {
				t_1 = c / (a * t_0);
#if DEBUG_SHOWINTERSECTIONS
				if (TRgStoreThisIntersection) {
					gIntersection.t_1 = t_1;
				}
#endif
				if (t_1 < 0) {
					// both intersections are less than zero - we are heading away. no hit.
#if DEBUG_PRINTINTERSECTIONS
					COrConsole_Printf("both hits at %f, %f frames < zero. exit - no hit.", t_0, t_1);
#endif
#if DEBUG_SHOWINTERSECTIONS
					if (TRgStoreThisIntersection) {
						gIntersection.exit_type = cExit_AirCyl_HeadingAway;
						gIntersection.rejected_by_airhit = UUcTrue;
					}
#endif
					return UUcFalse;
				}
			} else {
#if DEBUG_PRINTINTERSECTIONS
				COrConsole_Printf("first hit at %f frames < num danger frames %d. keep checking.", t_0, num_danger_frames);
#endif
#if DEBUG_SHOWINTERSECTIONS
				if (TRgStoreThisIntersection) {
					gIntersection.t_1 = t_0;
				}
#endif
			}
		}

	} else {
		float distsq_to_start, distsq_to_end, distsq_to_reject, start_dot, end_dot, interp, dot_delta;
		float mid_x, mid_y, distsq_to_mid;
		UUtBool no_hits, first_iteration;

		// get the target's end point with respect to the start of the attack animation
		end_pt.x = target_pt.x + num_danger_frames * target_vel.x;
		end_pt.y = target_pt.y + num_danger_frames * target_vel.y;

		distsq_to_start = UUmSQR(target_pt.x) + UUmSQR(target_pt.y);
		distsq_to_end	= UUmSQR(end_pt.x) + UUmSQR(end_pt.y);

		distsq_to_reject = UUmSQR(attack_extentinfo->attack_ring.max_distance + target_radius);

#if DEBUG_SHOWINTERSECTIONS
		if (TRgStoreThisIntersection) {
			gIntersection.start_pt = target_pt;
			gIntersection.end_pt = end_pt;
			gIntersection.distsq_to_start = distsq_to_start;
			gIntersection.distsq_to_end = distsq_to_end;
			gIntersection.distsq_to_reject = distsq_to_reject;
		}
#endif

		if ((distsq_to_start > distsq_to_reject) && (distsq_to_end > distsq_to_reject)) {
			// we can reject this attack, unless there is a point in the middle of the
			// target's movement that is close enough to fall into the attack ring
			start_dot = target_pt.x * target_vel.x + target_pt.y * target_vel.y;
			end_dot = end_pt.x * target_vel.x + end_pt.y * target_vel.y;

#if DEBUG_SHOWINTERSECTIONS
			if (TRgStoreThisIntersection) {
				gIntersection.start_dot = start_dot;
				gIntersection.end_dot = end_dot;
			}
#endif
			dot_delta = (end_dot - start_dot);
			if ((start_dot * end_dot > 0) || (fabs(dot_delta) < 1e-03)) {
				// the point with dot-product zero doesn't lie between the two endpoints, or the two endpoints are
				// very close together. forget about checking the midpoint, and trivial reject: the attacker is
				// too far away to stand a chance.
#if DEBUG_PRINTINTERSECTIONS
				COrConsole_Printf("dist to target %f/%f (dot %f,%f), attack range %f + target cyl %f isn't enough. no hit.",
								MUrSqrt(distsq_to_start), MUrSqrt(distsq_to_end), start_dot, end_dot,
								attack_extentinfo->attack_ring.max_distance, target_radius);
#endif
#if DEBUG_SHOWINTERSECTIONS
				if (TRgStoreThisIntersection) {
					gIntersection.exit_type = cExit_OutsideDist_NoDot;
				}
#endif
				return UUcFalse;
			} else {
				// get the point in the middle that is the closest on the line (has zero dot product)
				interp = (0 - start_dot) / dot_delta;
				
				mid_x = target_pt.x + interp * (end_pt.x - target_pt.x);
				mid_y = target_pt.y + interp * (end_pt.y - target_pt.y);

				distsq_to_mid = UUmSQR(mid_x) + UUmSQR(mid_y);

#if DEBUG_SHOWINTERSECTIONS
				if (TRgStoreThisIntersection) {
					gIntersection.mid_x = mid_x;
					gIntersection.mid_y = mid_y;
					gIntersection.distsq_to_mid = distsq_to_mid;
				}
#endif
				if (distsq_to_mid > distsq_to_reject) {
					// trivial reject: even the midpoint is too far away.
#if DEBUG_PRINTINTERSECTIONS
					COrConsole_Printf("dist to midpoint %f: ends were %f/%f (dot %f,%f). attack range %f + target cyl %f isn't enough. no hit.",
									MUrSqrt(distsq_to_mid), MUrSqrt(distsq_to_start), MUrSqrt(distsq_to_end),
									start_dot, end_dot, attack_extentinfo->attack_ring.max_distance, target_radius);
#endif
#if DEBUG_SHOWINTERSECTIONS
					if (TRgStoreThisIntersection) {
						gIntersection.exit_type = cExit_OutsideDist_WithDot;
					}
#endif
					return UUcFalse;
				}
			}
		}

		if (attack_extentinfo->attack_ring.min_height > target_pt.z + inContext->target_height_max) {
			// trivial reject: the attack is over the target's head
#if DEBUG_SHOWINTERSECTIONS
			if (TRgStoreThisIntersection) {
				gIntersection.exit_type = cExit_AboveHead;
			}
#endif
#if DEBUG_PRINTINTERSECTIONS
			COrConsole_Printf("attack minheight %f > target maxheight (%f + delta %f = %f). no hit.",
							attack_extentinfo->attack_ring.min_height, inContext->target_height_max,
							target_pt.z, inContext->target_height_max + target_pt.z);
#endif
			return UUcFalse;
		}

		if (attack_extentinfo->attack_ring.max_height < target_pt.z + inContext->target_height_min) {
			// trivial reject: the attack is under the target's feet
#if DEBUG_SHOWINTERSECTIONS
			if (TRgStoreThisIntersection) {
				gIntersection.exit_type = cExit_UnderFeet;
			}
#endif
#if DEBUG_PRINTINTERSECTIONS
			COrConsole_Printf("attack maxheight %f < target minheight (%f + delta %f = %f). no hit.",
							attack_extentinfo->attack_ring.max_height, inContext->target_height_min,
							target_pt.z, inContext->target_height_min + target_pt.z);
#endif
			return UUcFalse;
		}

		// for every line in the attack ring, check to see if it intersects the target. if none of them do,
		// we can reject this attack. start at the maximum extent of the attack.
		start_itr = MUrUnsignedSmallFloat_To_Uns_Round(TRcExtentRingSamples *
							attack_extentinfo->maxAttack.attack_angle / M3c2Pi) % TRcExtentRingSamples;
		itr = start_itr;
		first_iteration = UUcTrue;
		no_hits = UUcFalse;
#if DEBUG_SHOWINTERSECTIONS
		if (TRgStoreThisIntersection) {
			gIntersection.start_itr = start_itr;
		}
#endif
		while (1) {
			float dir_x, dir_y, start_x, start_y, end_x, end_y, closest_x, line_length;

			if ((itr == start_itr) && !first_iteration) {
				no_hits = UUcTrue;
				break;
			}
			first_iteration = UUcFalse;

			line_length = attack_extentinfo->attack_ring.distance[itr];
			if (inContext->consider_attack) {
				// reduce the extent of the attack by a safety margin so that
				// we don't whiff with attacks
				line_length -= TRcAttackSafetyMargin;
			}
			dir_x = TRgRingDirectionTable[itr][0];
			dir_y = TRgRingDirectionTable[itr][1];
			itr = (itr + 1) % TRcExtentRingSamples;
			if (line_length < 0.01f) {
				continue;
			}

			// get the location of the target, and the location of the target at the end of the attack.
			// these coordinates are in attacker-centered animationspace, rotated so that the ring line in
			// question lies along +X.
			start_x =  target_pt.x * dir_x + target_pt.y * dir_y;
			start_y = -target_pt.x * dir_y + target_pt.y * dir_x;

			if (fabs(start_y) < target_radius) {
				if (start_x > line_length + target_radius) {
					// this point can't touch the line
				} else if (start_x > line_length) {
					// this point might touch the line
					closest_x = start_x - MUrSqrt(UUmSQR(target_radius) - UUmSQR(start_y));
					if (closest_x < line_length)
						break;
				} else if (start_x > 0) {
					// this point does touch the line
					break;
				} else if (start_x > -target_radius) {
					// this point might touch the line
					closest_x = start_x + MUrSqrt(UUmSQR(target_radius) - UUmSQR(start_y));
					if (closest_x > 0)
						break;
				} else {
					// this point can't touch the line.
				}
			}

			end_x =  end_pt.x * dir_x + end_pt.y * dir_y;
			end_y = -end_pt.x * dir_y + end_pt.y * dir_x;

			if (fabs(end_y) < target_radius) {
				if (end_x > line_length + target_radius) {
					// this point can't touch the line
				} else if (end_x > line_length) {
					// this point might touch the line
					closest_x = end_x - MUrSqrt(UUmSQR(target_radius) - UUmSQR(end_y));
					if (closest_x < line_length)
						break;
				} else if (end_x > 0) {
					// this point does touch the line
					break;
				} else if (end_x > -target_radius) {
					// this point might touch the line
					closest_x = end_x + MUrSqrt(UUmSQR(target_radius) - UUmSQR(end_y));
					if (closest_x > 0)
						break;
				} else {
					// this point can't touch the line.
				}
			}

			// neither point touches the line. check to see if the midpoint might.
			if ((end_y * start_y < 0) && 
				!((end_x < -target_radius) && (start_x < -target_radius)) &&
				!((end_x > line_length + target_radius) && (start_x > line_length + target_radius))) {
				mid_x = start_x + ((0 - start_y) / (end_y - start_y)) * (end_x - start_x);

				if ((mid_x > -target_radius) && (mid_x < line_length + target_radius)) {
					// this point does touch the line.
					break;
				}
			}
		}

		if (no_hits) {
			// we have checked every line in the ring; the target's cylinder-vector does not
			// touch the attack ring.
#if DEBUG_PRINTINTERSECTIONS
			COrConsole_Printf("checked every line in attack ring, none touch. no hit.");
#endif
#if DEBUG_SHOWINTERSECTIONS
			if (TRgStoreThisIntersection) {
				gIntersection.exit_type = cExit_MissRing;
			}
#endif
			return UUcFalse;
		} else {
			// the target's cylinder-vector does touch the attack ring.
#if DEBUG_SHOWINTERSECTIONS
			if (TRgStoreThisIntersection) {
				gIntersection.hit_line = (itr == 0) ? (TRcExtentRingSamples - 1) : itr - 1;
			}
#endif
#if DEBUG_PRINTINTERSECTIONS
			COrConsole_Printf("line %d in the attack ring touches (after %d checks). we must step through anims.",
				(itr == 0) ? (TRcExtentRingSamples - 1) : itr - 1,
				(itr > start_itr) ? (itr - start_itr) : (TRcExtentRingSamples + itr - start_itr));
#endif
		}
	}
	
	// the bounds of the attack animation overlap the target, return true
#if DEBUG_SHOWINTERSECTIONS
	if (TRgStoreThisIntersection) {
		gIntersection.exit_type = cHit;
	}
#endif
#if DEBUG_PRINTINTERSECTIONS
	COrConsole_Printf("bounds overlap, returning true.");
#endif
	return UUcTrue;
}

// precisely determine whether an attack animation will hit
UUtBool TRrIntersectAnimations(TRtAnimIntersectContext *inContext)
{
	UUtBool attack_hits, current_validheight_attack;
	UUtUns8 angle_index;
	UUtUns32 attackExtentItr, attack_duration, target_duration;
	UUtUns32 max_reject, num_reject_abovehead, num_reject_belowfeet, num_reject_angle, num_reject_distance, num_reject_behind;
	TRtAttackExtent *cur_extent;
	M3tVector3D target_vel;
	const TRtExtentInfo *attack_extentinfo;
	TRtPosition *target_position, *attacker_position;
	TRtDirection attack_direction;
	float targetspace_minheight, targetspace_maxheight, target_feet, target_head;
	float attacker_feet, attacker_head, *positionptr;
	float delta_x, delta_y, b, c, disc, t, touch_dist_sq, cur_dist_sq;
#if DEBUG_SHOWINTERSECTIONS
	tExactPoint *exact_pt;
#endif

	// the attacker is the center of coordinates for this check - initialize its starting values
	MUmVector_Set(inContext->attacker.offsetLocation, 0, 0, 0);
	inContext->attacker.currentAnimFrame		= inContext->attacker.animFrame;
	if (inContext->attacker.position.inAir) {
		inContext->attacker.currentNumFramesInAir	= inContext->attacker.position.numFramesInAir;
		inContext->attacker.currentAirVelocity.x	= inContext->attacker.position.airVelocity.x;
		inContext->attacker.currentAirVelocity.y	= inContext->attacker.position.airVelocity.z;
		inContext->attacker.currentAirVelocity.z	= inContext->attacker.position.airVelocity.y;
		inContext->attacker.currentJumped			= inContext->attacker.position.jumped;
	}

	// transform the target's starting values by the target location matrix into attackerspace.
	// note that this is *not* attacker animspace - we use the actual location of the attacker
	inContext->target.offsetLocation		= MUrMatrix_GetTranslation(&inContext->current_location_matrix);
	inContext->target.currentAnimFrame		= inContext->target.animFrame;
	if (inContext->target.position.inAir) {
		inContext->target.currentNumFramesInAir = inContext->target.position.numFramesInAir;
		inContext->target.currentAirVelocity.x	= inContext->target.position.airVelocity.x;
		inContext->target.currentAirVelocity.y	= inContext->target.position.airVelocity.z;
		inContext->target.currentAirVelocity.z	= inContext->target.position.airVelocity.y;
		inContext->target.currentJumped			= inContext->target.position.jumped;

		MUrMatrix3x3_MultiplyPoint(&inContext->target.currentAirVelocity,
			(M3tMatrix3x3 *) &inContext->current_location_matrix, &inContext->target.currentAirVelocity);
	}

	MUrMatrix3x3_MultiplyPoint(&inContext->target_velocity_estimate, (M3tMatrix3x3 *) &inContext->current_location_matrix,
								&target_vel);

	// find the next attack-extent frame in the attacker's animation
	UUmAssert(TRrAnimation_IsAttack(inContext->attacker.animation));
	attack_extentinfo = (const TRtExtentInfo *) &inContext->attacker.animation->extentInfo;
	cur_extent = attack_extentinfo->attackExtents;

	for (attackExtentItr = 0; attackExtentItr < attack_extentinfo->numAttackExtents; attackExtentItr++, cur_extent++) {
		if (cur_extent->frame_index >= inContext->attacker.currentAnimFrame)
			break;
	}

	if (attackExtentItr >= attack_extentinfo->numAttackExtents) {
		// no attack frames remain.
		inContext->reject_reason = TRcRejection_NoAttack;
		return UUcFalse;
	}

	// how close do we have to get horizontally before the two characters touch? note: we don't use the target's
	// collision sphere here because the player's is huge
	touch_dist_sq = UUmSQR(inContext->attacker.collision_leafsphere_radius + inContext->target_cylinder_radius);

	attack_direction = TRrAnimation_GetDirection(inContext->attacker.animation, UUcTrue);
	attack_duration = TRrAnimation_GetDuration(inContext->attacker.animation);
	target_duration = TRrAnimation_GetDuration(inContext->target.animation);

	// get the characters' current positions in their animations
	UUmAssert((inContext->attacker.currentAnimFrame >= 0) && (inContext->attacker.currentAnimFrame < attack_duration));
	UUmAssert((inContext->target.currentAnimFrame >= 0) && (inContext->target.currentAnimFrame < target_duration));
	attacker_position = inContext->attacker.animation->positionPoints + inContext->attacker.currentAnimFrame;
	target_position	= inContext->target.animation->positionPoints + inContext->target.currentAnimFrame;

	positionptr = inContext->attacker.animation->positions + 2 * inContext->attacker.currentAnimFrame;

#if DEBUG_SHOWINTERSECTIONS
	if (TRgStoreThisIntersection) {
		gIntersection.num_exact_points = 0;
		exact_pt = gIntersection.exact_points;
	}
#endif

	num_reject_abovehead = num_reject_belowfeet = num_reject_distance = num_reject_angle = num_reject_behind = 0;

	// check through every frame remaining in the attacker's animation
	while (1) {
#if DEBUG_SHOWINTERSECTIONS
		if (TRgStoreThisIntersection) {
			MUmVector_Copy(exact_pt->attacker_location, inContext->attacker.offsetLocation);
			MUmVector_Copy(exact_pt->target_location, inContext->target.offsetLocation);
			exact_pt->target_minh = target_position->floor_to_toe_offset * TRcPositionGranularity
								+ inContext->target.offsetLocation.z;
			exact_pt->target_maxh = target_position->floor_to_toe_offset * TRcPositionGranularity
								+ target_position->toe_to_head_height * TRcPositionGranularity
								+ inContext->target.offsetLocation.z;
			exact_pt->is_attack = UUcFalse;
			exact_pt->moved = UUcFalse;
		}
#endif
		current_validheight_attack = UUcFalse;

		if (inContext->attacker.currentAnimFrame == cur_extent->frame_index) {
			// this frame is an attack
			current_validheight_attack = UUcTrue;
			targetspace_minheight = cur_extent->attack_minheight * TRcPositionGranularity
				+ (inContext->attacker.offsetLocation.z - inContext->target.offsetLocation.z);
			targetspace_maxheight = cur_extent->attack_maxheight * TRcPositionGranularity
				+ (inContext->attacker.offsetLocation.z - inContext->target.offsetLocation.z);

			// work out what the target's height is this frame
			target_feet = target_position->floor_to_toe_offset * TRcPositionGranularity;
			target_head = target_feet + target_position->toe_to_head_height * TRcPositionGranularity;

#if DEBUG_SHOWINTERSECTIONS
			if (TRgStoreThisIntersection) {
				exact_pt->is_attack = UUcTrue;
				exact_pt->attack_minheight = cur_extent->attack_minheight * TRcPositionGranularity;
				exact_pt->attack_maxheight = cur_extent->attack_maxheight * TRcPositionGranularity;
				exact_pt->attack_dist = cur_extent->attack_distance * TRcPositionGranularity;
				exact_pt->attack_angle = cur_extent->attack_angle;
			}
#endif		
			if (targetspace_maxheight < target_feet + TRcIntersect_MinHeightOverlap) {
				// this attack is below the target's height bounds
				num_reject_belowfeet++;
				current_validheight_attack = UUcFalse;

			} else if (targetspace_minheight > target_head - TRcIntersect_MinHeightOverlap) {
				// this attack is above the target's height bounds
				num_reject_abovehead++;
				current_validheight_attack = UUcFalse;

			} else {
				// the attack is good as far as height goes, check distance and direction.
				angle_index = (cur_extent->attack_angle >> 8) & 0xFF;
				delta_x = inContext->attacker.offsetLocation.x - inContext->target.offsetLocation.x;
				delta_y = inContext->attacker.offsetLocation.y - inContext->target.offsetLocation.y;

				// form terms of the quadratic equation
				c = UUmSQR(delta_x) + UUmSQR(delta_y) - UUmSQR(inContext->target_cylinder_radius);
				if (c < 0) {
					// we are actually standing inside the range - we will hit for sure
					attack_hits = UUcTrue;
				} else {
					b = 2 * (delta_x * TRgAttackDirectionTable[angle_index][0] + delta_y * TRgAttackDirectionTable[angle_index][1]);
					if (b > 0) {
						// the target is behind us
						attack_hits = UUcFalse;
						num_reject_behind++;
					} else {
						// a = 1 because TRgAttackDirection is guaranteed to have a unit direction
						disc = UUmSQR(b) - 4 * c;
						if (disc < 0) {
							// we won't hit the target.
							attack_hits = UUcFalse;
							num_reject_angle++;
						} else {
							// work out what the t-value of the first intersection is (distance to target)
							t = (-b - MUrSqrt(disc)) / 2.0f;
							attack_hits = (t < cur_extent->attack_distance * TRcPositionGranularity);

							if (!attack_hits) {
								num_reject_distance++;
							}
						}
					}
				}

				if (attack_hits) {
#if DEBUG_SHOWINTERSECTIONS
					if (TRgStoreThisIntersection) {
						exact_pt->hit = UUcTrue;
						gIntersection.num_exact_points++;
					}
#endif
					inContext->frames_to_hit = inContext->attacker.currentAnimFrame - inContext->attacker.animFrame;
					return UUcTrue;
				}
			}

			// move to looking for the next attacking frame
			attackExtentItr++;
			cur_extent++;
			if (attackExtentItr >= attack_extentinfo->numAttackExtents) {
				// we have checked all attacks - no hits. determine what the most common
				// rejection reason was.
				max_reject = UUmMax5(num_reject_abovehead, num_reject_belowfeet, num_reject_angle, num_reject_distance, num_reject_behind);

				if (max_reject == num_reject_abovehead) {
					inContext->reject_reason = TRcRejection_AboveHead;

				} else if (max_reject == num_reject_belowfeet) {
					inContext->reject_reason = TRcRejection_BelowFeet;

				} else if (max_reject == num_reject_angle) {
					inContext->reject_reason = TRcRejection_Angle;

				} else if (max_reject == num_reject_distance) {
					inContext->reject_reason = TRcRejection_Distance;

				} else {
					inContext->reject_reason = TRcRejection_Behind;
				}
				return UUcFalse;
			}
		}

#if DEBUG_SHOWINTERSECTIONS
		if (TRgStoreThisIntersection) {
			exact_pt->hit = UUcFalse;
			gIntersection.num_exact_points++;
			exact_pt++;

			UUmAssert(gIntersection.num_exact_points < 200);
		}
#endif

		// move to the next frame
		if (inContext->target.position.inAir) {
			// calculate the target's motion in midair
			inContext->target.currentNumFramesInAir++;

			if ((inContext->attacker.position.jumpKeyDown) &&
				(inContext->target.currentNumFramesInAir < inContext->target.numJumpAccelFrames)) {
				inContext->target.currentAirVelocity.z += inContext->target.jumpAccelAmount;
			}
			if (inContext->target.currentNumFramesInAir > inContext->target.framesFallGravity) {
				inContext->target.currentJumped = UUcTrue;
			}

			inContext->target.currentAirVelocity.z -= inContext->target.currentJumped
				? inContext->target.jumpGravity : inContext->target.fallGravity;

			MUrMatrix3x3_MultiplyPoint(&inContext->target.currentAirVelocity,
				(M3tMatrix3x3 *) &inContext->current_location_matrix, &target_vel);
		}

		// use the target's velocity estimate (original or updated) to calculate its new location
		MUmVector_Add(inContext->target.offsetLocation, inContext->target.offsetLocation, target_vel);

		if (inContext->attacker.position.inAir) {
			// calculate the target's motion in midair
/*			COrConsole_Printf("# attacker after %d frames in air, vel = %f ht = %f",
					inContext->attacker.currentNumFramesInAir - inContext->attacker.position.numFramesInAir,
					inContext->attacker.currentAirVelocity.z, inContext->attacker.offsetLocation.z +
					inContext->attacker.position.location.z);*/
			inContext->attacker.currentNumFramesInAir++;

			if ((inContext->attacker.position.jumpKeyDown) &&
				(inContext->attacker.currentNumFramesInAir < inContext->attacker.numJumpAccelFrames)) {
				inContext->attacker.currentAirVelocity.z += inContext->attacker.jumpAccelAmount;
			}
			if (inContext->attacker.currentNumFramesInAir > inContext->attacker.framesFallGravity) {
				inContext->attacker.currentJumped = UUcTrue;
			}

			inContext->attacker.currentAirVelocity.z -= inContext->attacker.currentJumped
				? inContext->attacker.jumpGravity : inContext->attacker.fallGravity;

			MUmVector_Add(inContext->attacker.offsetLocation, inContext->attacker.offsetLocation,
							inContext->attacker.currentAirVelocity);
		} else {
			inContext->attacker.offsetLocation.x += positionptr[0];
			inContext->attacker.offsetLocation.y += positionptr[1];
		}

		if (inContext->attacker.currentAnimFrame < attack_duration - 1) {
			// advance through the attacker's animation
			inContext->attacker.currentAnimFrame++;
			attacker_position++;
			positionptr += 2;
		} else {
			// we have reached the end of the attack animation without any hits.
			return UUcFalse;
		}

		if (inContext->target.currentAnimFrame < target_duration - 1) {
			// only advance the target through their animation until they reach the
			// end, then stop there
			inContext->target.currentAnimFrame++;
			target_position++;
		}

		/*
		 * collide the attacker against the target... makes sure that we don't think that the attacker will
		 * just go straight through the target's position and miss with their attacks as a result
		 */

		// don't collide if we are leading with an attack
		if (current_validheight_attack)
			continue;

		delta_x = inContext->attacker.offsetLocation.x - inContext->target.offsetLocation.x;
		delta_y = inContext->attacker.offsetLocation.y - inContext->target.offsetLocation.y;

		cur_dist_sq = UUmSQR(delta_x) + UUmSQR(delta_y);
		if (cur_dist_sq > touch_dist_sq) {
			// the characters are too far apart horizontally to touch.
			continue;
		}

		// check that the characters' heights overlap
		target_feet = inContext->target.offsetLocation.z + target_position->floor_to_toe_offset * TRcPositionGranularity;
		target_head = inContext->target.offsetLocation.z + target_feet + target_position->toe_to_head_height * TRcPositionGranularity;

		attacker_feet = inContext->attacker.offsetLocation.z + attacker_position->floor_to_toe_offset * TRcPositionGranularity;
		attacker_head = inContext->attacker.offsetLocation.z + attacker_feet + attacker_position->toe_to_head_height * TRcPositionGranularity;

		if ((attacker_feet > target_head) || (attacker_head < target_feet)) {
			// the characters don't overlap vertically
			continue;
		}

		// move the attacker back along its direction of motion.
#if DEBUG_SHOWINTERSECTIONS
		if (TRgStoreThisIntersection) {
			exact_pt[-1].moved = UUcTrue;
		}
#endif
		switch (attack_direction) {
			case TRcDirection_None:
			case TRcDirection_360:
			case TRcDirection_Forwards:
				inContext->attacker.offsetLocation.y = inContext->target.offsetLocation.y
					- MUrSqrt(touch_dist_sq - UUmSQR(delta_x));
			break;

			case TRcDirection_Left:
				inContext->attacker.offsetLocation.x = inContext->target.offsetLocation.x
					- MUrSqrt(touch_dist_sq - UUmSQR(delta_y));
			break;

			case TRcDirection_Right:
				inContext->attacker.offsetLocation.x = inContext->target.offsetLocation.x
					+ MUrSqrt(touch_dist_sq - UUmSQR(delta_y));
			break;

			case TRcDirection_Back:
				inContext->attacker.offsetLocation.y = inContext->target.offsetLocation.y
					+ MUrSqrt(touch_dist_sq - UUmSQR(delta_x));
			break;

			default:
				UUmAssert(!"TRrIntersectAnimations: unknown attack direction");
			break;
		}
	}
}

#if DEBUG_SHOWINTERSECTIONS
#include "Oni_Character.h"
UUtBool TRgDisplayIntersections = UUcFalse;
UUtBool TRgStoreThisIntersection = UUcTrue;

static void TRiDrawCircle(float radius, M3tPoint3D *center_pt, IMtShade shade)
{
	M3tPoint3D last_pt, this_pt;
	UUtUns32 itr;

	MUmVector_Set(last_pt, center_pt->x + radius * TRgRingDirectionTable[TRcExtentRingSamples - 1][0],
							center_pt->y + radius * TRgRingDirectionTable[TRcExtentRingSamples - 1][1], center_pt->z);
	for (itr = 0; itr < TRcExtentRingSamples; itr++) {
		MUmVector_Set(this_pt, center_pt->x + radius * TRgRingDirectionTable[itr][0],
								center_pt->y + radius * TRgRingDirectionTable[itr][1], center_pt->z);		
		M3rGeom_Line_Light(&last_pt, &this_pt, shade);
		last_pt = this_pt;
	}
}

void TRrDisplayLastIntersection(void)
{
	static const M3tMatrix4x3 max2oni = {{{1, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0, 0, 0}}};
	M3tMatrix4x3 inv_matrix;
//	ONtCharacter *target = (ONtCharacter *) gIntersection.last_context.target.character;
	UUtUns32 itr;
	UUtUns8 angle_index;
	UUtInt32 from_start, hit_at, from_hit;
	IMtShade shade;
	float radius, draw_height, temp_x, temp_y, temp_len;
	M3tPoint3D pt0, pt1, center_pt, last_pt, this_pt;
	tExactPoint *exact_pt;

	// set up a transformation from targetspace to worldspace
	M3rMatrixStack_Push();
	M3rMatrixStack_Translate(gIntersection.last_context.target.position.location.x,
							gIntersection.last_context.target.position.location.y, 
							gIntersection.last_context.target.position.location.z);
	M3rMatrixStack_Rotate(gIntersection.last_context.target.position.facing, 0, 1, 0);
	M3rMatrixStack_Multiply(&max2oni);

	// draw the target's bounding cylinder
	radius = gIntersection.last_context.target_cylinder_radius;
	MUmVector_Set(center_pt, 0, 0, gIntersection.last_context.target_height_min);
	TRiDrawCircle(radius, &center_pt, IMcShade_Blue);
	MUmVector_Set(center_pt, 0, 0, gIntersection.last_context.target_height_max);
	TRiDrawCircle(radius, &center_pt, IMcShade_Blue);

	MUmVector_Set(pt0, 0, 0, (gIntersection.last_context.target_height_min + gIntersection.last_context.target_height_max) / 2);
	pt1 = pt0;
	pt1.x = gIntersection.last_context.target_velocity_estimate.x;
	pt1.y = gIntersection.last_context.target_velocity_estimate.y;
	M3rGeom_Line_Light(&pt0, &pt1, IMcShade_Blue);
	M3rMatrixStack_Pop();
	
	// set up a transformation from attacker space to worldspace
	M3rMatrixStack_Push();
	M3rMatrixStack_Translate(gIntersection.last_context.attacker.position.location.x,
							gIntersection.last_context.attacker.position.location.y, 
							gIntersection.last_context.attacker.position.location.z);
	M3rMatrixStack_Rotate(gIntersection.last_context.attacker.position.facing, 0, 1, 0);
	M3rMatrixStack_Multiply(&max2oni);

	draw_height = (gIntersection.extentptr->attack_ring.min_height + gIntersection.extentptr->attack_ring.max_height) / 2;

	if (gIntersection.rejected_by_dir) {
		// push the stack; add an attack-facing rotation
		M3rMatrixStack_Push();

		switch(gIntersection.attack_direction) {
			case TRcDirection_Forwards:
				// nothing
			break;

			case TRcDirection_Left:
				M3rMatrixStack_Rotate(M3cHalfPi, 0, 0, 1);
			break;

			case TRcDirection_Back:
				M3rMatrixStack_Rotate(M3cPi, 0, 0, 1);
			break;

			case TRcDirection_Right:
				M3rMatrixStack_Rotate(M3c2Pi - M3cHalfPi, 0, 0, 1);
			break;

			default:
				UUmAssert(0);
			break;
		}

		// draw target's position and velocity
		MUmVector_Set(this_pt, gIntersection.attackrel_x, gIntersection.attackrel_y, draw_height);

		pt0 = this_pt;
		pt0.z += 4.0f;
		pt1 = this_pt;
		pt1.z -= 4.0f;
		M3rGeom_Line_Light(&pt0, &pt1, IMcShade_Orange);

		last_pt = this_pt;
		last_pt.x += gIntersection.attackrel_vel_x;
		last_pt.y += gIntersection.attackrel_vel_y;

		pt0 = last_pt;
		pt0.z += 4.0f;
		pt1 = last_pt;
		pt1.z -= 4.0f;
		M3rGeom_Line_Light(&pt0, &pt1, IMcShade_Orange);

		M3rGeom_Line_Light(&this_pt, &last_pt, IMcShade_Orange);

		if (gIntersection.exit_type == cExit_IsBehind) {
			MUmVector_Set(pt0, -10, 0, draw_height);
			MUmVector_Set(pt1, +10, 0, draw_height);
			M3rGeom_Line_Light(&pt0, &pt1, IMcShade_Orange);

		} else if (gIntersection.exit_type == cExit_IsLeft) {
			MUmVector_Set(pt0, +10 * TRcIntersect_ArcRangeFraction, +10, draw_height);
			MUmVector_Set(pt1, -10 * TRcIntersect_ArcRangeFraction, -10, draw_height);
			M3rGeom_Line_Light(&pt0, &pt1, IMcShade_Orange);

		} else if (gIntersection.exit_type == cExit_IsRight) {
			MUmVector_Set(pt0, -10 * TRcIntersect_ArcRangeFraction, +10, draw_height);
			MUmVector_Set(pt1, +10 * TRcIntersect_ArcRangeFraction, -10, draw_height);
			M3rGeom_Line_Light(&pt0, &pt1, IMcShade_Orange);

		} else {
			UUmAssert(0);
		}

		// pop off the attack-facing
		M3rMatrixStack_Pop();

		// pop off the attacker-space transform
		M3rMatrixStack_Pop();
		return;
	}

	// draw the air cylinder
	if (gIntersection.last_context.attacker.position.inAir) {
		if (gIntersection.rejected_by_airhit) {
			shade = IMcShade_Red;
		} else {
			shade = IMcShade_Green;
		}

		MUmVector_Set(this_pt, 0, 0, draw_height);
		MUmVector_Set(last_pt, gIntersection.num_danger_frames * gIntersection.last_context.attacker.position.airVelocity.x,
								gIntersection.num_danger_frames * gIntersection.last_context.attacker.position.airVelocity.z,
					draw_height);
		M3rGeom_Line_Light(&this_pt, &last_pt, shade);

		temp_x = -gIntersection.last_context.attacker.position.airVelocity.z;
		temp_y = gIntersection.last_context.attacker.position.airVelocity.x;
		temp_len = gIntersection.extentptr->attack_ring.max_distance / MUrSqrt(UUmSQR(temp_x) + UUmSQR(temp_y));
		temp_x *= temp_len;
		temp_y *= temp_len;

		this_pt.x += temp_x;
		this_pt.y += temp_y;
		last_pt.x += temp_x;
		last_pt.y += temp_y;
		M3rGeom_Line_Light(&this_pt, &last_pt, shade);

		this_pt.x -= 2 * temp_x;
		this_pt.y -= 2 * temp_y;
		last_pt.x -= 2 * temp_x;
		last_pt.y -= 2 * temp_y;
		M3rGeom_Line_Light(&this_pt, &last_pt, shade);

		MUmVector_Set(center_pt, gIntersection.t_0 * gIntersection.last_context.attacker.position.airVelocity.x,
								gIntersection.t_0 * gIntersection.last_context.attacker.position.airVelocity.z,
					draw_height);
		TRiDrawCircle(gIntersection.extentptr->attack_ring.max_distance, &center_pt, IMcShade_Purple);

		if (gIntersection.t_1 != gIntersection.t_0) {
			MUmVector_Set(center_pt, gIntersection.t_1 * gIntersection.last_context.attacker.position.airVelocity.x,
									gIntersection.t_1 * gIntersection.last_context.attacker.position.airVelocity.z,
						draw_height);
			this_pt = center_pt;
			this_pt.z += 4.0f;
			last_pt = center_pt;
			last_pt.z += 4.0f;
			M3rGeom_Line_Light(&this_pt, &last_pt, IMcShade_Purple);
		}

		if (gIntersection.rejected_by_airhit) {
			// pop off the attacker-space transform
			M3rMatrixStack_Pop();
			return;
		}
	} else {

		if ((gIntersection.exit_type == cExit_OutsideDist_NoDot) || (gIntersection.exit_type == cExit_OutsideDist_WithDot)) {
			// don't draw the abovehead or underfeet rings, because we didn't ever get to that part of the check
		} else {
			if ((gIntersection.exit_type == cExit_AboveHead) || (gIntersection.exit_type == cExit_UnderFeet)) {
				shade = IMcShade_Red;
			} else {
				shade = IMcShade_Green;
			}

			// draw attack extent height bar
			center_pt = MUrMatrix_GetTranslation(&gIntersection.last_context.current_location_matrix);
			
			this_pt = center_pt;	last_pt = center_pt;
			this_pt.z = gIntersection.extentptr->attack_ring.min_height;
			last_pt.z = gIntersection.extentptr->attack_ring.max_height;
			M3rGeom_Line_Light(&this_pt, &last_pt, shade);

			// draw side bars
			last_pt = this_pt;
			this_pt.x -= 5.0f;
			last_pt.x += 5.0f;
			M3rGeom_Line_Light(&this_pt, &last_pt, shade);

			last_pt = center_pt;
			last_pt.z = gIntersection.extentptr->attack_ring.max_height;
			this_pt = last_pt;
			this_pt.x -= 5.0f;
			last_pt.x += 5.0f;
			M3rGeom_Line_Light(&this_pt, &last_pt, shade);

			if (shade == IMcShade_Red) {
				// exit here

				// pop off the attacker-space transform
				M3rMatrixStack_Pop();
				return;
			}
		}
	}

	M3rMatrixStack_Pop();

	// build a matrix which lets us draw things from the base location of the animation
	// in order to do this, we construct a draw-in-target-animspace matrix and then apply
	// the inverse of the matrix that takes points from targetanimspace into animbase space
	M3rMatrixStack_Push();
	M3rMatrixStack_Translate(gIntersection.last_context.target.position.location.x,
							gIntersection.last_context.target.position.location.y, 
							gIntersection.last_context.target.position.location.z);
	M3rMatrixStack_Rotate(gIntersection.last_context.target.position.facing, 0, 1, 0);
	M3rMatrixStack_Multiply(&max2oni);

	MUrMatrix_Inverse(&gIntersection.last_context.current_location_matrix, &inv_matrix);
	M3rMatrixStack_Multiply(&inv_matrix);

	if ((gIntersection.exit_type == cExit_OutsideDist_NoDot) || (gIntersection.exit_type == cExit_OutsideDist_WithDot)) {
		shade = IMcShade_Pink;
	} else {
		shade = IMcShade_Gray75;
	}

	// draw rejection ring
	MUmVector_Set(center_pt, 0, 0, draw_height);
	TRiDrawCircle(MUrSqrt(gIntersection.distsq_to_reject), &center_pt, shade);

	// draw little crosses for the two or three points that we sampled
	center_pt = gIntersection.start_pt;
	center_pt.z = draw_height;
	MUmVector_Set(this_pt, center_pt.x - 3.0f, center_pt.y, center_pt.z - 3.0f);
	MUmVector_Set(last_pt, center_pt.x + 3.0f, center_pt.y, center_pt.z + 3.0f);
	M3rGeom_Line_Light(&this_pt, &last_pt, shade);
	MUmVector_Set(this_pt, center_pt.x + 3.0f, center_pt.y, center_pt.z - 3.0f);
	MUmVector_Set(last_pt, center_pt.x - 3.0f, center_pt.y, center_pt.z + 3.0f);
	M3rGeom_Line_Light(&this_pt, &last_pt, shade);

	center_pt = gIntersection.end_pt;
	center_pt.z = draw_height;
	MUmVector_Set(this_pt, center_pt.x - 3.0f, center_pt.y, center_pt.z - 3.0f);
	MUmVector_Set(last_pt, center_pt.x + 3.0f, center_pt.y, center_pt.z + 3.0f);
	M3rGeom_Line_Light(&this_pt, &last_pt, shade);
	MUmVector_Set(this_pt, center_pt.x + 3.0f, center_pt.y, center_pt.z - 3.0f);
	MUmVector_Set(last_pt, center_pt.x - 3.0f, center_pt.y, center_pt.z + 3.0f);
	M3rGeom_Line_Light(&this_pt, &last_pt, shade);

	if (gIntersection.exit_type == cExit_OutsideDist_WithDot) {
		MUmVector_Set(center_pt, gIntersection.mid_x, gIntersection.mid_y, draw_height);
		MUmVector_Set(this_pt, center_pt.x - 3.0f, center_pt.y, center_pt.z - 3.0f);
		MUmVector_Set(last_pt, center_pt.x + 3.0f, center_pt.y, center_pt.z + 3.0f);
		M3rGeom_Line_Light(&this_pt, &last_pt, shade);
		MUmVector_Set(this_pt, center_pt.x + 3.0f, center_pt.y, center_pt.z - 3.0f);
		MUmVector_Set(last_pt, center_pt.x - 3.0f, center_pt.y, center_pt.z + 3.0f);
		M3rGeom_Line_Light(&this_pt, &last_pt, shade);
	}

	if (shade == IMcShade_Pink) {
		// exit here

		// pop off the attacker-space transform
		M3rMatrixStack_Pop();
		return;
	}

	MUmVector_Set(center_pt, 0, 0, 1.0f);
	for (itr = 0; itr < TRcExtentRingSamples; itr++) {

		if (gIntersection.exit_type == cExit_MissRing) {
			shade = IMcShade_Red;
		} else {
			from_start = itr - gIntersection.start_itr;
			if (from_start < 0)
				from_start += TRcExtentRingSamples;

			hit_at = gIntersection.hit_line - gIntersection.start_itr;
			if (hit_at < 0)
				hit_at += TRcExtentRingSamples;
			
			from_hit = hit_at - from_start;

			if (from_hit == 0) {
				shade = IMcShade_Green;
			} else if (from_hit > 0) {
				shade = IMcShade_Red;
			} else {
				shade = IMcShade_Gray75;
			}
		}

		last_pt = center_pt;
		last_pt.x += TRgRingDirectionTable[itr][0] * gIntersection.extentptr->attack_ring.distance[itr];
		last_pt.y += TRgRingDirectionTable[itr][1] * gIntersection.extentptr->attack_ring.distance[itr];
		M3rGeom_Line_Light(&center_pt, &last_pt, shade);
	}

	if (gIntersection.exit_type == cExit_MissRing) {
		// exit here - pop off the attacker-base transform
		M3rMatrixStack_Pop();
		return;
	}

	// draw the exact-animation-test points if there are any
	exact_pt = gIntersection.exact_points;
	for (itr = 0; itr < gIntersection.num_exact_points; itr++, exact_pt++) {
		// draw attacker
		shade = (exact_pt->moved) ? IMcShade_Red : IMcShade_Orange;
		center_pt = exact_pt->attacker_location;
		this_pt = center_pt;
		this_pt.x -= 2.0f;
		this_pt.y -= 2.0f;
		last_pt = center_pt;
		last_pt.x += 2.0f;
		last_pt.y += 2.0f;
		M3rGeom_Line_Light(&this_pt, &last_pt, shade);
		this_pt.x += 4.0f;
		last_pt.x -= 4.0f;
		M3rGeom_Line_Light(&this_pt, &last_pt, shade);

		// draw target
		shade = IMcShade_LightBlue;
		center_pt = exact_pt->target_location;
		this_pt = center_pt;
		this_pt.x -= 2.0f;
		this_pt.y -= 2.0f;
		last_pt = center_pt;
		last_pt.x += 2.0f;
		last_pt.y += 2.0f;
		M3rGeom_Line_Light(&this_pt, &last_pt, shade);
		this_pt.x += 4.0f;
		last_pt.x -= 4.0f;
		M3rGeom_Line_Light(&this_pt, &last_pt, shade);

		// draw target height
		shade = IMcShade_LightBlue;
		center_pt = exact_pt->target_location;
		this_pt = center_pt;
		last_pt = center_pt;
		this_pt.z = exact_pt->target_minh;
		last_pt.z = exact_pt->target_maxh;
		M3rGeom_Line_Light(&this_pt, &last_pt, shade);

		if (exact_pt->is_attack) {
			shade = exact_pt->hit ? IMcShade_Yellow : IMcShade_Pink;

			angle_index = (exact_pt->attack_angle >> 8) & 0xff;

			pt0 = exact_pt->attacker_location;
			pt0.z += exact_pt->attack_minheight;

			pt1 = pt0;
			pt1.x += TRgAttackDirectionTable[angle_index][0] * exact_pt->attack_dist;
			pt1.y += TRgAttackDirectionTable[angle_index][1] * exact_pt->attack_dist;

			this_pt = pt0;
			last_pt = pt1;
			M3rGeom_Line_Light(&this_pt, &last_pt, shade);

			this_pt = pt1;
			this_pt.z += exact_pt->attack_maxheight - exact_pt->attack_minheight;
			M3rGeom_Line_Light(&this_pt, &last_pt, shade);

			last_pt = pt0;
			last_pt.z += exact_pt->attack_maxheight - exact_pt->attack_minheight;
			M3rGeom_Line_Light(&this_pt, &last_pt, shade);

			this_pt = pt0;
			M3rGeom_Line_Light(&this_pt, &last_pt, shade);
		}
	}

	M3rMatrixStack_Pop();
}
#endif

TRtFootstepKind TRrGetFootstep(const TRtAnimation *inAnimation, TRtAnimTime inFrame)
{
	TRtFootstep *footsteps = inAnimation->footsteps;
	TRtFootstepKind result = TRcFootstep_None;
	UUtUns32 itr;

	if (NULL == footsteps) {
		goto exit;
	}

	for(itr = 0; itr < inAnimation->numFootsteps; itr++)
	{
		if (inFrame == footsteps[itr].frame) {
			result = footsteps[itr].type;
		}
	}

exit:
	return result;
}


static void TRrDumpAnimation(FILE *stream, const TRtAnimation *inAnimation)
{
	UUtUns32 itr;
	fprintf(stream, "%s\n", inAnimation->instanceName);

	fprintf(stream, "heights\n");
	if (NULL != inAnimation->heights) {
		for(itr = 0; itr < inAnimation->numFrames; itr++)
		{
			fprintf(stream, "%f\n", inAnimation->heights[itr]);
		}
	}

	fprintf(stream, "positions\n");
	if (NULL != inAnimation->positions) {
		for(itr = 0; itr < inAnimation->numFrames; itr++)
		{
			fprintf(stream, "%f,%f\n", inAnimation->positions[itr * 2], inAnimation->positions[itr * 2 + 1]);
		}
	}

	fprintf(stream, "shortcuts\n");
	if (NULL != inAnimation->shortcuts) {
		for(itr = 0; itr < inAnimation->numShortcuts; itr++)
		{
			fprintf(stream, "%d %d %d", inAnimation->shortcuts[itr].state, inAnimation->shortcuts[itr].length, inAnimation->shortcuts[itr].flags);
		}
	}

	fprintf(stream, "flags %x\n", inAnimation->flags);
	fprintf(stream, "directAnimation[0] %s\n", (inAnimation->directAnimation[0] != NULL) ? TMrInstance_GetInstanceName(inAnimation->directAnimation[0]) : "");
	fprintf(stream, "directAnimation[1] %s\n", (inAnimation->directAnimation[1] != NULL) ? TMrInstance_GetInstanceName(inAnimation->directAnimation[1]) : "");
	fprintf(stream, "usedParts %x\n", inAnimation->usedParts);
	fprintf(stream, "replaceParts %x\n", inAnimation->replaceParts);
	fprintf(stream, "finalRotation %f\n", inAnimation->finalRotation);
	fprintf(stream, "moveDirection %d\n", inAnimation->moveDirection);

//	TRtExtentInfo			extentInfo;

	fprintf(stream, "attackName %s\n", inAnimation->attackName);
	fprintf(stream, "hardPause %d\n", inAnimation->hardPause);
	fprintf(stream, "softPause %d\n", inAnimation->softPause);
	fprintf(stream, "fps %d\n", inAnimation->fps);
	fprintf(stream, "compressionSize %d\n", inAnimation->compressionSize);
	fprintf(stream, "type %d\n", inAnimation->type);
	fprintf(stream, "aimingType %d\n", inAnimation->aimingType);
	fprintf(stream, "fromState %d\n", inAnimation->fromState);
	fprintf(stream, "toState %d\n", inAnimation->toState);
	fprintf(stream, "numParts %d\n", inAnimation->numParts);
	fprintf(stream, "numFrames %d\n", inAnimation->numFrames);
	fprintf(stream, "duration %d\n", inAnimation->duration);
	fprintf(stream, "varient %x\n", inAnimation->varient);
	fprintf(stream, "varientEnd %x\n", inAnimation->varientEnd);
	fprintf(stream, "atomicStart %d\n", inAnimation->atomicStart);
	fprintf(stream, "atomicEnd %d\n", inAnimation->atomicEnd);
	fprintf(stream, "endInterpolation %d\n", inAnimation->endInterpolation);
	fprintf(stream, "maxInterpolation %d\n", inAnimation->maxInterpolation);

	fprintf(stream, "actionFrame %d\n", inAnimation->actionFrame);
	fprintf(stream, "firstLevel %d\n", inAnimation->firstLevel);
	fprintf(stream, "numAttacks %d\n", inAnimation->numAttacks);
	fprintf(stream, "numTakeDamages %d\n", inAnimation->numTakeDamages);
	fprintf(stream, "numShortcuts %d\n", inAnimation->numShortcuts);
	fprintf(stream, "numFootsteps %d\n", inAnimation->numFootsteps);
	fprintf(stream, "numParticles %d\n", inAnimation->numParticles);

	return;
}

static void TRrDumpAnimationCollection(FILE *stream, const TRtAnimationCollection *inCollection)
{
	
	UUtUns32 itr;

	for(itr = 0; itr < inCollection->numAnimations; itr++)
	{
		const TRtAnimationCollectionPart *current_part = inCollection->entry + itr;

		fprintf(stream, "%d %d %d %s\n", current_part->weight, current_part->virtualFromState, current_part->lookupValue, (current_part->animation != NULL) ? TMrInstance_GetInstanceName(current_part->animation) : "NULL");
	}

	if (NULL != inCollection->recursiveLookup) {
		fprintf(stream, "recursive\n");
		TRrDumpAnimationCollection(stream, inCollection->recursiveLookup);
	}

	return;
}


static UUtError
DumpAnimation(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	char *animation_name = inParameterList[0].val.str;
	char file_name[128];
	const TRtAnimation *animation = TRrAnimation_GetFromName(animation_name);

	sprintf(file_name, "%s.txt", inParameterList[1].val.str);

	if (NULL == animation) {
		COrConsole_Printf("failed to find animation %s", animation_name);
	}
	else {
		FILE *stream = fopen(file_name, "w");

		if (NULL == stream) {
			COrConsole_Printf("failed to open file %s", file_name);
		}
		else {
			TRrDumpAnimation(stream, animation);
			fclose(stream);

			COrConsole_Printf("wrote animation %s to file %s", animation_name, file_name);
		}
	}

	return UUcError_None;
}

static UUtError
DumpCollection(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	char *animation_collection_name = inParameterList[0].val.str;
	char file_name[128];
	const TRtAnimationCollection *animation_collection = TMrInstance_GetFromName(TRcTemplate_AnimationCollection, animation_collection_name);

	sprintf(file_name, "%s.txt", inParameterList[1].val.str);

	if (NULL == animation_collection) {
		COrConsole_Printf("failed to find animation collection %s", animation_collection_name);
	}
	else {
		FILE *stream = fopen(file_name, "w");

		if (NULL == stream) {
			COrConsole_Printf("failed to open file %s", file_name);
		}
		else {
			TRrDumpAnimationCollection(stream, animation_collection);
			fclose(stream);
			COrConsole_Printf("wrote animation %s to file %s", animation_collection_name, file_name);
		}
	}

	return UUcError_None;
}

static UUtError
DumpLookup(
	SLtErrorContext			*inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual		*inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStall,
	SLtParameter_Actual		*ioReturnValue)
{
	char file_name[128];
	
	sprintf(file_name, "%s.txt", inParameterList[0].val.str);
	gLookupStream = fopen(file_name, "w");

	if (NULL == gLookupStream) {
		COrConsole_Printf("failed to open %s", file_name);
	}
	else {
		COrConsole_Printf("opened %s", file_name);
	}
	return UUcError_None;
}

static UUtError
StopDumpLookup(
	SLtErrorContext			*inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual		*inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStall,
	SLtParameter_Actual		*ioReturnValue)
{
	if (NULL == gLookupStream) {
		COrConsole_Printf("lookup stream was not open");
	}
	else {
		fclose(gLookupStream);
		gLookupStream = NULL;
		COrConsole_Printf("closed");
	}

	return UUcError_None;
}

void TRrInstallConsoleVariables(void)
{
	UUtError error;

	error = SLrScript_Command_Register_Void("tr_write_animation", "bla bla bla", "anim_name:string file_name:string", DumpAnimation);
	UUmAssert(UUcError_None == error);

	error = SLrScript_Command_Register_Void("tr_write_collection", "bla bla bla", "collection_name:string file_name:string", DumpCollection);
	UUmAssert(UUcError_None == error);

	error = SLrScript_Command_Register_Void("tr_write_lookup", "bla bla bla", "file_name:string", DumpLookup);
	UUmAssert(UUcError_None == error);

	error = SLrScript_Command_Register_Void("tr_stop_lookup", "bla bla bla", "", StopDumpLookup);
	UUmAssert(UUcError_None == error);

	

	return;
}

UUtBool TRrAnimation_IsInvulnerable(const TRtAnimation *inAnimation, TRtAnimTime inTime)
{
	UUtUns32 start = inAnimation->invulnerableStart;
	UUtUns32 end = inAnimation->invulnerableStart + inAnimation->invulnerableLength;

	UUtBool is_invulnerable;

	is_invulnerable = (inTime >= start) && (inTime < end);

	return is_invulnerable;
}
