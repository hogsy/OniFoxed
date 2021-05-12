/*
	FILE:	BFW_Totoro.h
	
	AUTHOR:	Michael Evans
	
	CREATED: Sept 25, 1997
	
	PURPOSE: 
	
	Copyright 1997, 1998

*/
#ifndef BFW_TOTORO_PRIVATE_H
#define BFW_TOTORO_PRIVATE_H

#include "BFW_Totoro.h"
#include "BFW_SoundSystem2.h"

#define TRmAnimationTimeToFrame_Internal(inAnimation, inTime) ((UUtUns16) (((inTime) * (inAnimation->fps)) / 60))

#if 0
#if defined(DEBUGGING) && DEBUGGING
#define TRmAnimationTimeToFrame(inAnimation, inTime) (((inAnimation) != NULL) ? TRmAnimationTimeToFrame_Internal((inAnimation), (inTime)) : 0)
#else
#define TRmAnimationTimeToFrame(inAnimation, inTime) TRmAnimationTimeToFrame_Internal((inAnimation), (inTime))
#endif

#define TRmAnimationTimeIsFrame(inAnimation, inTime) ((0 == (inTime)) ? (UUcTrue) : ((TRmAnimationTimeToFrame((inAnimation), (inTime))) != (TRmAnimationTimeToFrame((inAnimation), (inTime - 1)))))
#else
#define TRmAnimationTimeToFrame(inAnimation, inTime) (inTime)
#define TRmAnimationTimeIsFrame(inAnimation, inTime) (UUcTrue)
#endif

typedef unsigned char M3tCompressedQuaternion3;

typedef struct M3tCompressedQuaternion4
{
	UUtInt8 x;
	UUtInt8 y;
	UUtInt8 z;
	UUtInt8 w;
} M3tCompressedQuaternion4;

typedef struct M3tCompressedQuaternion6
{
	UUtUns16 rx;
	UUtUns16 ry;
	UUtUns16 rz;
} M3tCompressedQuaternion6;			

typedef struct M3tCompressedQuaternion8
{
	UUtInt16 x;
	UUtInt16 y;
	UUtInt16 z;
	UUtInt16 w;
} M3tCompressedQuaternion8;				

enum {
	TRcAnimationFlag_Reversable		= 0x01
};

enum {
	TRcAnimationCompression3		= 3,
	TRcAnimationCompression4		= 4,
	TRcAnimationCompression6		= 6,
	TRcAnimationCompression8		= 8,
	TRcAnimationCompression16		= 16
};

#define TRcMaxSounds			((UUtUns8) 3)
#define TRcMaxShortcuts			((UUtUns8) 8)
#define TRcMaxDirectAnimations	((UUtUns8) 2)
#define TRcMaxBlurs				((UUtUns8) 2)
#define TRcMaxTakeDamages		((UUtUns8) 3)
#define TRcMaxFootsteps			((UUtUns8) 4)

// 10 bytes
typedef tm_struct TRtBlur
{
	UUtUns32	blurParts;
	UUtUns16	firstBlurFrame;	
	UUtUns16	lastBlurFrame;
	UUtUns8		blurQuantity;
	UUtUns8		blurAmount;
	UUtUns8		blurSkip;
	UUtUns8		blurPad;
} TRtBlur;

typedef tm_struct TRtTakeDamage
{
	UUtUns16	damage;
	UUtUns16	frame;
} TRtTakeDamage;

typedef struct TRtShortcut
{
	TRtAnimState	state;
	UUtUns16		length;
	UUtUns32		flags;
} TRtShortcut;

typedef struct TRtThrowInfo
{
	M3tVector3D				throwOffset;
	float					relativeThrowFacing;			
	float					throwDistance;

	UUtUns16				throwType;
} TRtThrowInfo;

typedef struct TRtFootstep
{
	UUtUns16				frame;
	UUtUns16				type;
} TRtFootstep;

typedef struct TRtSound
{
	char					impulseName[SScMaxNameLength];
	UUtUns16				frameNum;
	
} TRtSound;

// an animation of a translation, rotation Body
tm_template('T', 'R', 'A', 'M', "Totoro Animation Sequence")
TRtAnimation
{
	tm_raw(char *)			instanceName;
	tm_raw(float *)			heights;
	tm_raw(float *)			positions;
	tm_raw(TRtAttack *)		attacks;
	tm_raw(TRtTakeDamage *)	takeDamages;
	tm_raw(TRtBlur *)		blurs;
	tm_raw(TRtShortcut *)	shortcuts;
	tm_raw(TRtThrowInfo *)	throwInfo;
	tm_raw(TRtFootstep *)	footsteps;
	tm_raw(TRtParticle *)	particles;
	tm_raw(TRtPosition *)	positionPoints;
	tm_raw(void *)			data;
	tm_raw(TRtSound *)		newSounds;

	UUtUns32				flags;					
	TRtAnimation			*directAnimation[2];	// skips pause & end interpolation
	UUtUns32				usedParts;				
	UUtUns32				replaceParts;

	float					finalRotation;

	UUtUns16				moveDirection;			// TRtDirection: used by AI and character code for facing
	UUtUns16				vocalizationType;

	TRtExtentInfo			extentInfo;

	char					attackName[16];			// must match ONcCharacterAttackNameLength + 1

	UUtUns16				hardPause;				// pause (no block)
	UUtUns16				softPause;				// pause (can block)
	
	UUtUns32				numNewSounds;			// number of items in the new_sounds list
	UUtUns32				soundName;				// only valid at runtime (typecast to char*)
	UUtUns16				soundFrame;				// only valid at runtime
	
	UUtUns16				fps;				

	UUtUns16				compressionSize;	
	UUtUns16				type;

	UUtUns16				aimingType;
	UUtUns16				fromState;
	
	UUtUns16				toState;		
	UUtUns16				numParts;		

	UUtUns16				numFrames;		
	UUtUns16				duration;

	UUtUns16				varient;
	UUtUns16				varientEnd;

	UUtUns16				atomicStart;			// atomic over this range
	UUtUns16				atomicEnd;

	UUtUns16				endInterpolation;		// amount of interpolation at the end
	UUtUns16				maxInterpolation;

	UUtInt16				actionFrame;			// swap weapon, pickup ...
	UUtUns16				firstLevel;

	UUtUns8					invulnerableStart;
	UUtUns8					invulnerableLength;

	UUtUns8					numAttacks;
	UUtUns8					numTakeDamages;
	UUtUns8					numBlurs;
	UUtUns8					numShortcuts;

	UUtUns8					numFootsteps;
	UUtUns8					numParticles;
};

tm_template('T', 'R', 'A', 'S', "Totoro Aiming Screen")
TRtAimingScreen
{
	
	TRtAnimation *animation;

	float		negativeYawDelta;
	float		positiveYawDelta;

	UUtInt16	negativeYawFrameCount;
	UUtInt16	positiveYawFrameCount;

	float		negativePitchDelta;
	float		positivePitchDelta;

	UUtInt16	negativePitchFrameCount;
	UUtInt16	positivePitchFrameCount;
};


/* 
 * functions for getting quaternion arrays
 */

#define TRcMaxParts 32

enum
{
	TRcWeight_OnlyEntry = 0
};

tm_struct TRtAnimationCollectionPart
{
	UUtUns16		weight;
	UUtUns16		virtualFromState;		// virtual=real for animation, virutal for shortcut
	UUtUns32		lookupValue;

	TRtAnimation	*animation;
};

tm_template('T', 'R', 'A', 'C', "Animation Collection")
TRtAnimationCollection
{
	tm_pad									pad0[16];

	TRtAnimationCollection					*recursiveLookup;
	UUtUns16								recursiveOnly;

	tm_varindex UUtUns16					numAnimations;
	tm_vararray TRtAnimationCollectionPart	entry[1];
};

tm_template('T', 'R', 'S', 'C', "Screen (aiming) Collection")
TRtScreenCollection
{
	tm_pad								pad0[22];

	tm_varindex UUtUns16				numScreens;
	tm_vararray TRtAimingScreen			*screen[1];	
};

void TRrVerifyAnimation(const TRtAnimation *inAnimation);

void TRrAnimation_SwapHeights(TRtAnimation *animation);
void TRrAnimation_SwapPositions(TRtAnimation *animation);
void TRrAnimation_SwapAttacks(TRtAnimation *animation);
void TRrAnimation_SwapTakeDamages(TRtAnimation *animation);
void TRrAnimation_SwapBlurs(TRtAnimation *animation);
void TRrAnimation_SwapShortcuts(TRtAnimation *animation);
void TRrAnimation_SwapThrowInfo(TRtAnimation *animation);
void TRrAnimation_SwapFootsteps(TRtAnimation *animation);
void TRrAnimation_SwapParticles(TRtAnimation *animation);
void TRrAnimation_SwapPositionPoints(TRtAnimation *animation);
void TRrAnimation_SwapExtentInfo(TRtAnimation *animation);
void TRrAnimation_SwapNewSounds(TRtAnimation *animation);

UUtError TRrInitialize_IntersectionTables(void);


#endif // BFW_TOTORO_PRIVATE_H
