/*
	FILE:	Imp_Character.c
	
	AUTHOR:	Brent H. Pease, Michael Evans
	
	CREATED: Nov 1, 1997
	
	PURPOSE: 
	
	Copyright 1997 - 2000
*/

//#ifdef UUmPlatform_SonyPSX2_T
#if defined(UUmPlatform) && ((UUmPlatform == UUmPlatform_SonyPSX2) || (UUmPlatform == UUmPlatform_Win32))
#define LOSSY_ANIM
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_Totoro.h"
#include "BFW_Totoro_Private.h"
#include "BFW_TemplateManager.h"
#include "BFW_TM_Construction.h"
#include "BFW_Group.h"
#include "BFW_MathLib.h"
#include "BFW_Motoko.h"
#include "BFW_AppUtilities.h"
#include "BFW_ScriptLang.h"

#include "EulerAngles.h"
#include "ONI_Character.h"
#include "Oni_Character_Animation.h"
#include "Oni_ImpactEffect.h"

#include "Imp_Common.h"
#include "Imp_Character.h"
#include "Imp_Model.h"
#include "Imp_Texture.h"
#include "Imp_AI_HHT.h"
#include "Imp_AI2.h"
#include "Imp_ParseEnvFile.h"

static void iCompressQuaternion(const M3tQuaternion *inQuaternion, void *outData, UUtUns16 compressionSize);

#define IMPcMaxCharacterParticles	64
#define HIT_STUN_PER_DAMAGE (0.5f)
#define BLOCK_STUN_PER_DAMAGE (0.5f)
#define BLOCK_STUN_DEFAULT_MIN 8

typedef struct TRtFootstepList
{
	UUtInt32	numFootsteps;
	TRtFootstep footsteps[1];
} TRtFootstepList;

typedef struct TRtFootstepClass
{
	TRtAnimType anim_type;
	UUtBool single_footstep_at_start;
	float foostep_initial_clear_threshold;
	float footstep_clear_threshold;
	float footstep_active_threshold;
} TRtFootstepClass;

TRtFootstepClass footstep_class_table[] = 
{
	{ ONcAnimType_Standing_Turn_Left,			UUcTrue,  0.0f, 0.0f, 0.0f  },
	{ ONcAnimType_Standing_Turn_Left,			UUcTrue,  0.0f, 0.0f, 0.0f  },

	{ ONcAnimType_Run_Start,					UUcTrue, 0.2f, 0.3f, 0.05f },
	{ ONcAnimType_Run_Sidestep_Left_Start,		UUcTrue, 0.2f, 0.3f, 0.05f },
	{ ONcAnimType_Run_Sidestep_Right_Start,		UUcTrue, 0.2f, 0.3f, 0.05f },
	{ ONcAnimType_Run_Backwards_Start,			UUcTrue, 0.2f, 0.3f, 0.05f },
	{ ONcAnimType_Walk_Start,					UUcFalse, 0.2f, 0.3f, 0.05f },
	{ ONcAnimType_Walk_Backwards_Start,			UUcFalse, 0.2f, 0.3f, 0.05f },		

	{ ONcAnimType_Walk,							UUcFalse, 0.2f, 0.3f, 0.05f },
	{ ONcAnimType_Walk_Backwards,				UUcFalse, 0.2f, 0.3f, 0.05f },		
	{ ONcAnimType_Walk_Sidestep_Left,			UUcFalse, 0.2f, 0.3f, 0.05f },
	{ ONcAnimType_Walk_Sidestep_Right,			UUcFalse, 0.2f, 0.3f, 0.05f },
	{ ONcAnimType_Walk_Stop,					UUcTrue, 0.0f, 0.0f, 0.0f },

	{ ONcAnimType_Run,							UUcFalse, 1.f, 1.5f, 0.5f },
	{ ONcAnimType_Run_Backwards,				UUcFalse, 1.f, 1.5f, 0.5f },
	{ ONcAnimType_Run_Sidestep_Left,			UUcFalse, 1.f, 1.5f, 0.5f },
	{ ONcAnimType_Run_Sidestep_Right,			UUcFalse, 1.f, 1.5f, 0.5f },
	{ ONcAnimType_Run_Stop,						UUcTrue, 0.0f, 0.0f, 0.0f },
	{ ONcAnimType_1Step_Stop,					UUcTrue, 0.0f, 0.0f, 0.0f  },

	{ ONcAnimType_Crouch_Turn_Left,				UUcTrue,  0.0f, 0.0f, 0.0f  },
	{ ONcAnimType_Crouch_Turn_Right,			UUcTrue,  0.0f, 0.0f, 0.0f  },
	{ ONcAnimType_Crouch_Run,					UUcFalse, 0.2f, 0.3f, 0.05f },
	{ ONcAnimType_Crouch_Walk,					UUcFalse, 0.2f, 0.3f, 0.05f },
	{ ONcAnimType_Crouch_Run_Backwards,			UUcFalse, 0.1f, 0.1f, 0.05f },
	{ ONcAnimType_Crouch_Walk_Backwards,		UUcFalse, 0.2f, 0.3f, 0.05f },
	{ ONcAnimType_Crouch_Run_Sidestep_Left,		UUcFalse, 0.2f, 0.3f, 0.05f },
	{ ONcAnimType_Crouch_Run_Sidestep_Right,	UUcFalse, 0.2f, 0.3f, 0.05f },
	{ ONcAnimType_Crouch_Walk_Sidestep_Left,	UUcFalse, 0.2f, 0.3f, 0.05f },
	{ ONcAnimType_Crouch_Walk_Sidestep_Right,	UUcFalse, 0.2f, 0.3f, 0.05f },

	{ ONcAnimType_Land,							UUcTrue, 0.0f, 0.0f, 0.0f },
	{ ONcAnimType_Land_Forward,					UUcTrue, 0.0f, 0.0f, 0.0f },
	{ ONcAnimType_Land_Right,					UUcTrue, 0.0f, 0.0f, 0.0f },
	{ ONcAnimType_Land_Left,					UUcTrue, 0.0f, 0.0f, 0.0f },
	{ ONcAnimType_Land_Back,					UUcTrue, 0.0f, 0.0f, 0.0f },
	{ ONcAnimType_Land_Hard,					UUcTrue, 0.0f, 0.0f, 0.0f },
	{ ONcAnimType_Land_Forward_Hard,			UUcTrue, 0.0f, 0.0f, 0.0f },
	{ ONcAnimType_Land_Right_Hard,				UUcTrue, 0.0f, 0.0f, 0.0f },
	{ ONcAnimType_Land_Left_Hard,				UUcTrue, 0.0f, 0.0f, 0.0f },
	{ ONcAnimType_Land_Back_Hard,				UUcTrue, 0.0f, 0.0f, 0.0f },
	{ ONcAnimType_Land_Dead,					UUcTrue, 0.0f, 0.0f, 0.0f },

	{ ONcAnimType_Kick,							UUcFalse, 3.0f, 4.0f, 0.05f },

	{ TRcAnimType_None,							UUcFalse, 0.f, 0.f, 0.f }
};

static const char *cParticle_PartName[] = {
	"pelvis",
	"l_upperleg",
	"l_lowerleg",
	"l_foot",
	"r_upperleg",
	"r_lowerleg",
	"r_foot",
	"abdomen",
	"chest",
	"neck",
	"head",
	"l_shoulder",
	"l_upperarm",
	"l_lowerarm",
	"l_hand",
	"r_shoulder",
	"r_upperarm",
	"r_lowerarm",
	"r_hand",
	"weapon",
	"muzzle",
	NULL};

#define cANDFile_VERSION 29

TRtFootstepClass *iGetFootstepClass(TRtAnimType inType)
{
	TRtFootstepClass *footstep_class = NULL;
	TRtFootstepClass *footstep_class_itr;

	for(footstep_class_itr = footstep_class_table; footstep_class_itr->anim_type != TRcAnimType_None; footstep_class_itr++)
	{
		if (footstep_class_itr->anim_type == inType) {
			footstep_class = footstep_class_itr;
			break;
		}
	}

	return footstep_class;	
}


extern UUtBool IMPgDebugFootsteps;

TMtStringArray *gStringToAnimTypeTable = NULL;
TMtStringArray *gStringToAnimStateTable = NULL;
TMtStringArray *gStringToAnimVarientTable = NULL;
TMtStringArray *gStringToAnimFlagTable = NULL;
TMtStringArray *gStringToShortcutFlagTable = NULL;
TMtStringArray *gStringToAttackFlagTable = NULL;
TMtStringArray *gStringToBodyFlagTable = NULL;

// these handle single text items
static UUtError iStringToAnimState(const char *inString, TRtAnimState *result);
static UUtError iStringToAnimFlag(const char *inString, UUtUns32 *result);

// these handle lists of text
static UUtError iParamListToVarients(const char *varientStr, TRtAnimVarient *varients);
static UUtError iParamListToAnimFlags(const char *inFlagStr, UUtUns32 *outFlags);
static UUtError iParamListToShortcutFlags(const char *inFlagStr, UUtUns32 *outFlags);
static UUtError iParamListToAttackFlags(const char *inFlagStr, UUtUns32 *outFlags);
static UUtError iParamListToBodyFlags(const char *inFlagStr, UUtUns32 *outFlags);

static char *iStateToString(TRtAnimType type);

// flag set by command line
extern UUtBool IMPgForceANDRebuild;


static UUtError Imp_CreateBody(
	BFtFileRef *inFileRef,
	TRtBody **outBody);

static UUtError iLoadTables(void);
static UUtUns16 iNumAnimStates(void);

extern UUtUns8 IMPgDefaultCompressionSize = TRcAnimationCompression6;

enum {
	cDefaultScreenCompressionSize = TRcAnimationCompression16 
};

enum {
	cDefaultFPS = 60
};

#define IMPmAnimFrames(duration, fps) (((duration * fps) + cDefaultFPS - 1) / cDefaultFPS)

// info saved about each frame, from the character's positions
// stored in the AND file and turned into various fields in the TRtAnimation
typedef struct IMPtFramePositionInfo
{
	TRtPosition		position;

	UUtBool			is_attack_frame;
	UUtUns32		attack_index;
	TRtAttackExtent	attack_extent;

} IMPtFramePositionInfo;

#define IMPcAttackPartIndex_None	((UUtUns32) -1)
#define IMPcAttackPartIndex_Max		ONcWeapon_Index

// this is hardcoded rather than being read in from a hierarchy file
// because we can't guarantee that the hierarchy will have been read
// before we import animations
static const UUtUns32 IMPgAttackPartHierarchy[IMPcAttackPartIndex_Max] = {
	IMPcAttackPartIndex_None,
	ONcPelvis_Index,
	ONcLLeg_Index,
	ONcLLeg1_Index,
	ONcPelvis_Index,
	ONcRLeg_Index,
	ONcRLeg1_Index,
	ONcPelvis_Index,
	ONcSpine_Index,
	ONcSpine1_Index,
	ONcNeck_Index,
	ONcSpine1_Index,
	ONcLArm_Index,
	ONcLArm1_Index,
	ONcLArm2_Index,
	ONcSpine1_Index,
	ONcRArm_Index,
	ONcRArm1_Index,
	ONcRArm2_Index
};
	
// pointer to the last biped we read in - used when constructing AND files
// to determine hierarchy and geometry bounding boxes
static TRtBody *IMPgLastBiped = NULL;

//=======================================================================
//
// Animation compression code docs.
//
// First, go to BFW_Totoro and look at the animation compression docs
// there. It will give you some background on the decompression.
//
// The compression algoirthm describes a process for finding "good"
// key frames that approximate the data in the original animation.
// The algorithm is considered lossy, in that there is some error
// in the results. The error is bounding by a constant, DEGREE_TOL,
// which represents the maximum number of degrees between the result
// of the decompression and the original data.
//
// During execution, the algoirthm begins by writing out the first
// quaternion in the original data as the first key-frame. Once a
// key frame quaternion is written, it looks for the next key frame 
// by looking at the next quaternions in the original data and testing
// them to see if they'd make good key-frames. When a quaternion is
// found that will not make a good key-frame the last "good" quaternion
// is written out and the process begins again.
//
// A quaternion is considered to make a "good" key-frame if the quaternions
// that result from interpolating all of the frames between that quaternion 
// and the last key-frame are within DEGREE_TOL degrees of the original 
// data. We can compute the number of degrees difference between two
// quaternions by inverting one quaternion, multiplying the two quaternions
// together, and looking at the real component of the quaternion, which
// represents cos(2/pheta), where pheta is the angle of rotation needed
// to map the compressed result on to the original data. The constant
// DEGREE_TOL is represented in degrees because I feel that degrees are
// more intuitive than radians or cos(2/pheta) values.
//
// One more note, the compression subroutine is a two pass alogirthm.
// On the first pass it figures out how big the data it wants to
// compress is so it can allocate it. The second pass writes out the
// actual structure. 

// This is a temperary array used to hold the original data. The Bungie
// compression routines do some transformations as pre-processing, so 
// this array reflects the result of those transformations.

M3tQuaternion tmpQuatArray[16384]; 

//
// The maximum number of degrees between the compressed data and the
// original data.


//
// This subroutine figures out the maximum difference between the
// result of interpolating quat_array[first] and quat_array[last] 
// between first and last and the actual values of quat_array[first..last]
//

float get_max_angle(M3tQuaternion *quat_array, int first, int last)
{
  //
  // max angle represents the value that results from cos(pheta/2). By
  // starting the value out at 1 we effectively represent pheta=0.
  // It is conceptually an angle, it's just gone through a strange
  // mapping.
  //

  float max_angle=1.0f;	
  int i;
  M3tQuaternion current;

  for (i=first; i<=last; ++i) {
      M3tQuaternion diff;

      // What percent shall we interpolate?

      float t=((float) (i-first))/((float) (last-first));

      // get the interpolated value...

      MUrQuat_Lerp(quat_array+first, quat_array+last, t, &current);

      // Compare the array value to the interpolated value. We'll
      // invert the current and multiply them together to get the
      // quaternion that transforms from the array to the interpolated
      // value (or vice versa, it really doesn't matter).

      // Do the inverse by negating the imaginary componenet. I
      // don't understand or trust the implementation of the
      // Quaternion inverse function in the math library.

      current.x=-current.x;
      current.y=-current.y;
      current.z=-current.z;

      // once this multiply is done the only value we're going to be
      // interested in is the w component of the quaternion.

      MUrQuat_Mul(&current, quat_array+i, &diff);

      // The resulting w is equal to cos(pheta/2), where pheta is the
      // angle of difference. If cos(pheta/2) is less than zero then
      // pheta is PI .. 2*PI. For the purposes of measuring the angle
      // between the two quaternions we can map pheta to 2*PI-pheta if
      // cos(pheta/2) is less than zero. cos(pheta/2) == -cos(2*PI-pheta/2)
      // so we just negate w. 

      if (diff.w<0)
          diff.w=-diff.w;

      // Remember, the biggest values of w represent the smallest angles.
      // if w is smaller than the current "max" then record it.

      if (diff.w<max_angle)
          max_angle=diff.w;
  }
  return max_angle;
}

//
// The code to actually do the compression.
//
// ioAnimation is the bungie animation structure.
//
// dst is the destination that we write our compressed data to. If
// dst is equal to NULL then this is a "dry run". Don't write anything
// out.
//
// inDirection is the direction the animation goes it (it's a bungie
// thing)
//
// inQuaternionArray is the input quaternion data for the animation. It
// If you want to find the quaternion for a frame, frame, and a body
// part, body_part, use inQuatArray[frame*ioAnimation->numParts+body_part]
// rotationDataSize is the size of the data once we're through writing
// it all out.
//
//

static UUtBool is_fight_stance(TRtAnimation *ioAnimation)
{
	UUtBool result;

	result = ioAnimation->type == ONcAnimType_Stand;
	result &= (ioAnimation->varient & ONcAnimVarientMask_Fight) > 0;
	result &= ioAnimation->fromState == ioAnimation->toState;
	result &= ioAnimation->fromState == ONcAnimState_Standing;

	return result;
}

extern float IMPgAnimAngle;

void do_lossy_anim_compress (
  TRtAnimation *ioAnimation,
  void *dst,
  UUtInt8 inDirection,
  const M3tQuaternion *inQuatArray,
  UUtUns32 *rotationDataSize)
{
  // The current body part we're processing.

  float degree_tol = is_fight_stance(ioAnimation) ? 0.f : IMPgAnimAngle;
  float rad_tol = (degree_tol)*M3cPi*2.0f/360.0f;
  float cos_tol = MUrCos(rad_tol*.5f);

  UUtUns32 nodeItr;

  // Bungie stuff. I believe it's the current frame.

  TRtAnimTime srcTime;
  //TRtAnimTime nextSrcTime;

  // transform from max coordinates to bungie cordinates.

  M3tMatrix4x3 max2oni;

  // the memory address we're currently writing to.

  UUtUns8 *dst_ptr=dst;

  // Bungie stuff to transform between max and oni co-ordinates.

  max2oni.m[0][0] = 1.f;
  max2oni.m[1][0] = 0.0;
  max2oni.m[2][0] = 0.0;
  max2oni.m[3][0] = 0.0;

  max2oni.m[0][1] = 0.0;
  max2oni.m[1][1] = 0.0;
  max2oni.m[2][1] = 1.f;
  max2oni.m[3][1] = 0.0;

  max2oni.m[0][2] = 0.0;
  max2oni.m[1][2] = -1.f;
  max2oni.m[2][2] = 0.0;
  max2oni.m[3][2] = 0.0;

  // Leave some space for the indexes into the frame data. The animation
  // data for any particular body part varies in size, depending on
  // our luck compressing it, so it has to be indexed.

  dst_ptr+=ioAnimation->numParts*sizeof(unsigned short);

  // For each body part....

  for(nodeItr = 0; nodeItr < ioAnimation->numParts; nodeItr++) {

      // What direction is this animation? 1 for forward, -1 for backward.

      int direction;

      // These are used by the compression routine. Interval is used to
      // represent the number of frames since the last key-frame.
      // ie, if we take a a a a b b b, where a and b are unique quaternions,
      // and compress it down to a 3 a 1 b 2 b, as per the docts in
      // the decompress routine, the 3, 1, and 2 would be the interval
      // values after the key frames a, a, and b were written out,
      // respectively.
      //
      // the skip next value holds the number of frames we want to skip
      // after writing out a key-frame. It is initially set to whatever
      // the interval is - 1. So, in the previous example, skip next would
      // be set to 2 after the first a keyframe is written out, causing the
      // next 2 'a' quaternions to the skipped.

      int skip_next=0;
      int interval=0;

      // the current frame of the animation that we're processing. 

      UUtUns32 frameItr;

      // Write out the index value, if needed.

      if (dst!=NULL) {
          ((unsigned short *) dst)[nodeItr] =
              dst_ptr- ((UUtUns8 *) dst);
      }

      // Figure out what direction we're going in.

      if (1 == inDirection) {
          srcTime = 0;
          direction=1;
      }
      else {
          srcTime = ioAnimation->duration;
          direction=-1;
      }

      // Populate tmpQuatArray with the animation values. If the animation
      // is "backwards" this is where we reverse it. This is also where
      // the Max to Oni mapping is applied.

      for(frameItr = 0; frameItr < ioAnimation->numFrames; srcTime = srcTime+direction, frameItr++) {

          M3tQuaternion thisQuat;
          UUtUns32 srcIndex;

          srcIndex = (srcTime * ioAnimation->numParts) + nodeItr;

          thisQuat = inQuatArray[srcIndex];

          // fix coordiante system
          if ((0 == nodeItr) && !(ioAnimation->flags & (1 << ONcAnimFlag_Overlay))) {
              M3tMatrix4x3 rootMatrix;

              MUrQuatToMatrix(&thisQuat, &rootMatrix);
              MUrMatrix_Multiply(&max2oni, &rootMatrix, &rootMatrix);
              MUrMatrixToQuat(&rootMatrix, &thisQuat);
          }

          MUmQuat_VerifyUnit(&thisQuat);
          tmpQuatArray[frameItr]=thisQuat;
      }

      // Now do the actual compression.

      for(frameItr = 0; frameItr < ioAnimation->numFrames; frameItr++) {
          int i;

          // skip over the compressed quaternions.

          if (skip_next) {
              skip_next--;
              continue;
          }

          // If this isn't the first frame write out the number of frames
          // between the last quaternions and the quaternions we're about
          // to write.

          if (frameItr!=0) {

              // note the check to make sure this isn't a "dry run"

              if (dst!=NULL) {
                  *dst_ptr=interval;
              }
              dst_ptr++;
          }

          // Write out the actual quaternion if it isn't a dry run...

          if (dst!=NULL) {
              iCompressQuaternion(
                  tmpQuatArray+frameItr,
                  dst_ptr,
                  //&(((UUtUns8 *) dst)[dstIndex]),
                  ioAnimation->compressionSize);
          }

          // and adjust the destination pointer

          dst_ptr+=ioAnimation->compressionSize;

          // Okay, figure out the interval to the next key-frame quaternion.
          // We do this by marching ahead until we find an unsuitable
          // key-frame.

          for (i=frameItr+1;i<ioAnimation->numFrames;++i) {
              float max_angle;
              max_angle=get_max_angle(tmpQuatArray, frameItr, i);
              if (max_angle<cos_tol) {
                  break;
              }
          }

          // Set interval accordingly. Remember, i is the value of the
          // first quaternion that the algorithm didn't like, so go back
          // to i-1.

          interval=i-frameItr-1;
          if (interval==0) {
              interval=1;
		  }
		  else if (interval > 255) {
			  // we are going to write this into a byte
			  interval = 255;
		  }

          // a paranoia check

          if (frameItr+interval>=ioAnimation->numFrames) {
              interval=ioAnimation->numFrames-frameItr-1;
          }

          // set skip next appropriately.

          skip_next=interval-1;

      }
  }

  // compute the size of this animation using some pointer subtraction.

  *rotationDataSize = dst_ptr- ((UUtUns8 *) dst);
}

// ==================================================================
//
// Animation code changes end here.
//
// ==================================================================


static char *GetLocalFileName(char *filename)
{
	char *output = filename;

	for(; *filename != '\0'; filename++)
	{
		if ('\\' == *filename)
		{
			output = filename + 1;
		}
	}

	return output;
}
							

static M3tMatrix4x3
ReadMatrix(BFtTextFile *inFileRef)
{
	float c1,c2,c3,c4;
	char *curLine;
	M3tMatrix4x3 matrix;
	int parsed;

	curLine = BFrTextFile_GetNextStr(inFileRef);
	parsed = sscanf(curLine, "%f %f %f %f", &c1, &c2, &c3, &c4);
	UUmAssert(4 == parsed);
	matrix.m[0][0] = c1;
	matrix.m[1][0] = c2;
	matrix.m[2][0] = c3;
	matrix.m[3][0] = c4;


	curLine = BFrTextFile_GetNextStr(inFileRef);
	parsed = sscanf(curLine, "%f %f %f %f", &c1, &c2, &c3, &c4);
	UUmAssert(4 == parsed);
	matrix.m[0][1] = c1;
	matrix.m[1][1] = c2;
	matrix.m[2][1] = c3;
	matrix.m[3][1] = c4;

	curLine = BFrTextFile_GetNextStr(inFileRef);
	parsed = sscanf(curLine, "%f %f %f %f", &c1, &c2, &c3, &c4);
	UUmAssert(4 == parsed);
	matrix.m[0][2] = c1;
	matrix.m[1][2] = c2;
	matrix.m[2][2] = c3;
	matrix.m[3][2] = c4;

	return matrix;
}

static M3tQuaternion
ReadQuaternion(BFtTextFile *inFileRef)
{
	char *line;
	int argumentsParsed;
	M3tQuaternion quat;

	line = BFrTextFile_GetNextStr(inFileRef);
	argumentsParsed = sscanf(line, "%f %f %f %f", &quat.x, &quat.y, &quat.z, &quat.w);
	UUmAssert(4 == argumentsParsed);

	return quat;
}

static M3tPoint3D
ReadPoint3D(BFtTextFile *inFileRef)
{
	char *line;
	int argumentsParsed;
	M3tPoint3D point;

	line = BFrTextFile_GetNextStr(inFileRef);
	argumentsParsed = sscanf(line, "%f %f %f", &point.x, &point.y, &point.z);
	UUmAssert(3 == argumentsParsed);

	return point;
}

static float
ReadFloat(BFtTextFile *inFileRef)
{
	char *line;
	int argumentsParsed;
	float f;

	line = BFrTextFile_GetNextStr(inFileRef);
	argumentsParsed = sscanf(line, "%f", &f);
	UUmAssert(1 == argumentsParsed);

	return f;
}

static void iBody_BuildParents(
	TRtBody			*inBody,
	UUtUns8			inParent,
	UUtUns8			inPartIndex)
{
	TRtIndexBlock *indexBlock= inBody->indexBlocks + inPartIndex;

	UUmAssert(NULL != inBody);

	// draw sibling
	if (0 != indexBlock->sibling)
	{
		iBody_BuildParents(inBody, inParent, indexBlock->sibling);
	}

	indexBlock->parent = inParent;

	// draw our child
	if (0 != indexBlock->child)
	{
		iBody_BuildParents(inBody, inPartIndex, indexBlock->child);
	}

	return;
}

static const char *iGetBodySuffix(TRtBodySelector selector)
{
	switch(selector)
	{
		case TRcBody_SuperLow:
			return "xlow.mme";
		case TRcBody_Low:
			return "low.mme";
		case TRcBody_Medium:
			return "med.mme";
		case TRcBody_High:
			return "high.mme";
		case TRcBody_SuperHigh:
			return "xhigh.mme";
		default:
			UUmAssert("invalid body suffix");
			return "error.mme";
	}

	UUmAssert(!"should not be reached");
}

UUtError
Imp_AddBiped(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate, 
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError error;
	TRtBody *body;
	BFtFileRef *baseRef;
	char *body_file_name;

	error =	GRrGroup_GetString(inGroup,	"file",	&body_file_name);
	UUmError_ReturnOnError(error);

	error = BFrFileRef_DuplicateAndReplaceName(inSourceFile, body_file_name, &baseRef);
	if (BFcError_FileNotFound == error) { error = UUcError_None; }
	UUmError_ReturnOnError(error);

	// construct the body instance (overwrite it if previously hanging around)
	error = TMrConstruction_Instance_Renew(
			TRcTemplate_Body,
			inInstanceName,
			0,
			&body);
	IMPmError_ReturnOnError(error);

	// read the body from file
	error = Imp_CreateBody(baseRef, &body);
	UUmError_ReturnOnError(error);

	BFrFileRef_Dispose(baseRef);

	// store the newly constructed biped
	IMPgLastBiped = body;
	
	return UUcError_None;
}

UUtError
Imp_AddBody(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate, 
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError error;
	TRtBodySelector itr;
	TRtBodySelector max_body = IMPgLowMemory ? TRcBody_Medium : TRcBody_SuperHigh;
	TRtBodySet *bodySet;
	BFtFileRef *baseRef;
	TRtBody *bodySpread;

	if(TMrConstruction_Instance_CheckExists(TRcTemplate_BodySet, inInstanceName)) {
		Imp_PrintMessage(IMPcMsg_Important, "body already imported"UUmNL);
		return UUcError_None;
	}

	{
		char *base_file_name;

		error =	GRrGroup_GetString(inGroup,	"file",	&base_file_name);
		UUmError_ReturnOnError(error);

		if (base_file_name[strlen(base_file_name) - 1] != '_')
		{
			Imp_PrintWarning("prefix %s invalid, patching up",base_file_name);

			base_file_name[strlen(base_file_name) - strlen("high.mme")] = '\0';

			Imp_PrintWarning("prefix is now %s",base_file_name);
		}
		
		error = BFrFileRef_DuplicateAndReplaceName(inSourceFile, base_file_name, &baseRef);
		if (BFcError_FileNotFound == error) { error = UUcError_None; }
		UUmError_ReturnOnError(error);
	}

	error = TMrConstruction_Instance_Renew(
			TRcTemplate_BodySet,
			inInstanceName,
			0,
			&bodySet);
	IMPmError_ReturnOnError(error);

	{
		UUtBool found_a_body = UUcFalse;
		const char *prefix = BFrFileRef_GetLeafName(baseRef);

		for(itr = TRcBody_SuperLow; itr <= TRcBody_SuperHigh; itr++)
		{
			char curName[BFcMaxFileNameLength + 50]; // 50 slop, why be in danger
			const char *suffix = iGetBodySuffix(itr);
			BFtFileRef *curFileRef;

			bodySet->body[itr] = NULL;

			if ((strlen(prefix) + strlen(suffix)) >= BFcMaxFileNameLength)
			{
				Imp_PrintWarning("file name to long %s%s",prefix,suffix);
				continue;
			}

			if (found_a_body && (itr > max_body)) {
				continue;
			}

			UUrString_Copy(curName, prefix, BFcMaxFileNameLength);
			UUrString_Cat(curName, suffix, BFcMaxFileNameLength);

			error = BFrFileRef_DuplicateAndReplaceName(
				baseRef,
				curName,
				&curFileRef);
			UUmError_ReturnOnError(error);

			error = Imp_CreateBody(curFileRef, bodySet->body + itr);
			UUmError_ReturnOnError(error);

			BFrFileRef_Dispose(curFileRef);

			if (bodySet->body[itr] != NULL) {
				found_a_body = UUcTrue;
			}
		}
	}
		
	BFrFileRef_Dispose(baseRef);
	
	bodySpread = NULL;
	for(itr = TRcBody_SuperHigh; 1; itr--)
	{
		if (NULL == bodySet->body[itr])
		{
			bodySet->body[itr] = bodySpread;
		}
		else
		{
			bodySpread = bodySet->body[itr];
		}

		if (TRcBody_SuperLow == itr) { break; }
	}

	bodySpread = NULL;
	for(itr = TRcBody_SuperLow; itr <= TRcBody_SuperHigh; itr++)
	{
		if (NULL == bodySet->body[itr])
		{
			bodySet->body[itr] = bodySpread;
		}
		else
		{
			bodySpread = bodySet->body[itr];
		}
	}

	for(itr = TRcBody_SuperLow; itr <= TRcBody_SuperHigh; itr++)
	{
		UUmAssert(bodySet->body[itr] != NULL);
	}

	if (NULL == bodySet->body[TRcBody_SuperLow]) {
		error = UUcError_Generic;
		UUmError_ReturnOnErrorMsg(error, "failed to find a body!");
	}


	return UUcError_None;
}

static UUtError Imp_CreateBody(
	BFtFileRef *inFileRef,
	TRtBody **outBody)
{
	UUtError		error;
	UUtUns16		nodeItr;
	TRtBody			*body;
	MXtHeader		*header;

	// start with NULL, out so an error just leads to skip this LOD
	*outBody = NULL;

	if (BFrFileRef_FileExists(inFileRef))
	{
		Imp_PrintMessage(IMPcMsg_Cosmetic, "\treading %s..."UUmNL, BFrFileRef_GetLeafName(inFileRef));
	}
	else
	{
		Imp_PrintMessage(IMPcMsg_Cosmetic, "\tskipping %s..."UUmNL, BFrFileRef_GetLeafName(inFileRef));

		return UUcError_None;
	}
	
	error = Imp_ParseEnvFile(inFileRef, &header);
	IMPmError_ReturnOnError(error);

	// create the template with the correctly sized variable array

	error =
		TMrConstruction_Instance_NewUnique(
			TRcTemplate_Body,
			0,
			&body);
	IMPmError_ReturnOnError(error);
	body->numParts = header->numNodes;
	UUrString_Copy(body->debug_name, BFrFileRef_GetLeafName(inFileRef), 32);

	error =
		TMrConstruction_Instance_NewUnique(
			TRcTemplate_GeometryArray,
			header->numNodes,
			&body->geometryStorage);
	IMPmError_ReturnOnError(error);

	error =
		TMrConstruction_Instance_NewUnique(
			TRcTemplate_TranslationArray,
			header->numNodes,
			&body->translationStorage);
	IMPmError_ReturnOnError(error);
	
	error =
		TMrConstruction_Instance_NewUnique(
			TRcTemplate_IndexArray,
			header->numNodes,
			&body->indexBlockStorage);
	IMPmError_ReturnOnError(error);

	body->geometries = body->geometryStorage->geometries;
	body->translations = body->translationStorage->translations;
	body->indexBlocks = body->indexBlockStorage->indexBlocks;

	body->polygonCount = 0;

	for(nodeItr = 0; nodeItr < header->numNodes; nodeItr++)
	{
		MXtNode *curNode = header->nodes + nodeItr;
		M3tGeometry **part_geometries = body->geometries + nodeItr;
		M3tPoint3D *part_translations = body->translations + nodeItr;
		TRtIndexBlock *part_indexBlocks = body->indexBlocks + nodeItr;

		*part_translations = MUrMatrix_GetTranslation(&curNode->matrix);

		if (0 == nodeItr) {
			part_translations->x = 0.f;
			part_translations->y = 0.f;
			part_translations->z = 0.f;
		}
		
		UUmAssert(curNode->child < UUcMaxUns8);
		UUmAssert(curNode->sibling < UUcMaxUns8);

		part_indexBlocks->parent = 0;
		part_indexBlocks->child = (UUtUns8) curNode->child;
		part_indexBlocks->sibling = (UUtUns8) curNode->sibling;

		error =
			TMrConstruction_Instance_NewUnique(
				M3cTemplate_Geometry,
				0,
				part_geometries);
		IMPmError_ReturnOnError(error);

		error = Imp_Node_To_Geometry(curNode, *part_geometries);
		UUmError_ReturnOnErrorMsg(error, "failed to find process model");

		(*part_geometries)->baseMap = NULL;
	}


	iBody_BuildParents(body, 0, 0);

	Imp_EnvFile_Delete(header);

	*outBody = body;
	
	return UUcError_None;
}

typedef struct IMPtTextureName
{
	char texture_name[256];
} IMPtTextureName;

/*

  cache format:

  version
  mod time
  count
  texture name

  */

#define MODEL_TEXTURE_CACHE_VERSION 0x0001

static IMPtTextureName *Imp_GetTextureList(BFtFileRef *inMMEFile, UUtUns32 *outCount, UUtUns32 *mod_time)
{
	IMPtTextureName *texture_list;
	UUtUns32 count;

	BFtFileRef cache_reference;
	UUtBool cache_obsolete;
	UUtUns32 mme_mod_time;
	UUtUns32 cache_mod_time;
	UUtError error;

	cache_reference = *inMMEFile;
	BFrFileRef_SetLeafNameSuffex(&cache_reference, ".txl");

	if (!BFrFileRef_FileExists(&cache_reference)) {
		cache_obsolete = UUcTrue;
	}
	else {
		BFrFileRef_GetModTime(inMMEFile, &mme_mod_time);
		BFrFileRef_GetModTime(&cache_reference, &cache_mod_time);

		cache_obsolete = cache_mod_time < mme_mod_time;
	}
	
	// try reading cache
	if (!cache_obsolete) {
		BFtTextFile *cache_read_file;
		UUtUns32 version;

		BFrTextFile_OpenForRead(&cache_reference, &cache_read_file);
		version = BFrTextFile_GetUUtUns32(cache_read_file);
		cache_mod_time = BFrTextFile_GetUUtUns32(cache_read_file);
		count = BFrTextFile_GetUUtUns32(cache_read_file);

		if ((cache_mod_time < mme_mod_time) || (version != MODEL_TEXTURE_CACHE_VERSION)) {
			cache_obsolete = UUcTrue;
		}
		else {
			UUtUns32 read_itr;

			texture_list = UUrMemory_Block_New(sizeof(IMPtTextureName) * count);
			UUmAssert(NULL != texture_list);

			for(read_itr = 0; read_itr < count; read_itr++)
			{
				char *cache_texture = BFrTextFile_GetNextStr(cache_read_file);

				UUmAssert(NULL != cache_texture);

				strncpy(texture_list[read_itr].texture_name, cache_texture, 256);
				texture_list[read_itr].texture_name[255] = '\0';
				UUmAssert(strlen(texture_list[read_itr].texture_name) < 128);
			}
		}

		BFrTextFile_Close(cache_read_file);
	}

	if (cache_obsolete) {
		// read the actual file
		{
			MXtHeader *header;
			UUtUns16 mxtNodeItr;
			
			error = Imp_ParseEnvFile(inMMEFile, &header);
			UUmAssert(UUcError_None == error);

			count = header->numNodes;

			texture_list = UUrMemory_Block_New(sizeof(IMPtTextureName) * count);
			UUmAssert(NULL != texture_list);

			for(mxtNodeItr = 0; mxtNodeItr < count; mxtNodeItr++)
			{
				MXtNode *curNode = header->nodes + mxtNodeItr;
				char *diffuseTexture;

				diffuseTexture = curNode->materials[0].maps[MXcMapping_DI].name;
				strcpy(texture_list[mxtNodeItr].texture_name, diffuseTexture);
			}

			Imp_EnvFile_Delete(header);
		}

		// write the cache
		{
			BFtFile *cache_write_file;
			UUtUns32 write_node_itr;
			BFrFile_Delete(&cache_reference);

			BFrFile_Open(&cache_reference, "w", &cache_write_file);
			if (cache_write_file == NULL) {
				Imp_PrintWarning("could not write .txl file %s", BFrFileRef_GetLeafName(&cache_reference));
			} else {
				BFrFile_Printf(cache_write_file, "%d"UUmNL, MODEL_TEXTURE_CACHE_VERSION);
				BFrFile_Printf(cache_write_file, "%lud"UUmNL, mme_mod_time);
				BFrFile_Printf(cache_write_file, "%d"UUmNL, count);

				for(write_node_itr = 0; write_node_itr < count; write_node_itr++)
				{
					BFrFile_Printf(cache_write_file, "%s"UUmNL, texture_list[write_node_itr].texture_name);
				}

				BFrFile_Close(cache_write_file);
			}
		}
	}

	*outCount = count;
	*mod_time = mme_mod_time;

	UUrMemory_Block_VerifyList();

	return texture_list;
}

static UUtError
Imp_ProcessBodyTextures(
	BFtFileRef*				inMMEFile,
	BFtFileRef*				inDirectory,
	char*					inInstanceName)
{
#define cMaxNameLength		256
	UUtError				error;
	char **					textureFileNames;
	char **					texturePlaceholderNames;
	UUtUns16				nodeItr;
	TRtBodyTextures*		bodyTextures;
	UUtUns32				fileModDate;
	const char *			directoryLeafName = BFrFileRef_GetLeafName(inDirectory);
	UUtUns32				count;
	IMPtTextureName			*texture_list;

	tTextureFlags			flags;
	
	texture_list = Imp_GetTextureList(inMMEFile, &count, &fileModDate);

	if(TMrConstruction_Instance_CheckExists(TRcTemplate_BodyTextures, inInstanceName)) {
		Imp_PrintMessage(IMPcMsg_Important, "body textures already imported"UUmNL);
		return UUcError_None;
	}

	error = TMrConstruction_Instance_Renew(
				TRcTemplate_BodyTextures,
				inInstanceName,
				count,
				&bodyTextures);
	IMPmError_ReturnOnError(error);
	
	// read the texture names
	textureFileNames = (char **) UUrMemory_Block_New(sizeof(char *) * count);
	UUmError_ReturnOnNull(textureFileNames);

	texturePlaceholderNames = (char **) UUrMemory_Block_New(sizeof(char *) * count);
	UUmError_ReturnOnNull(texturePlaceholderNames);

	for(nodeItr = 0; nodeItr < count; nodeItr++)
	{
		const char *diffuseTexture;

		diffuseTexture = texture_list[nodeItr].texture_name;
		
		textureFileNames[nodeItr] = UUrMemory_Block_New(cMaxNameLength);
		UUmError_ReturnOnNull(textureFileNames[nodeItr]);
		
		sprintf(textureFileNames[nodeItr], "%s", diffuseTexture);

		texturePlaceholderNames[nodeItr] = UUrMemory_Block_New(cMaxNameLength * 2 * sizeof(char));
		UUmError_ReturnOnNull(texturePlaceholderNames[nodeItr]);
		
		UUmAssert(strlen(textureFileNames[nodeItr]) < cMaxNameLength);
		UUmAssert(strlen(directoryLeafName) < cMaxNameLength);

		UUrString_Copy(texturePlaceholderNames[nodeItr], directoryLeafName, cMaxNameLength);
		UUrString_Cat(texturePlaceholderNames[nodeItr], "/", cMaxNameLength);
		UUrString_Cat(texturePlaceholderNames[nodeItr], diffuseTexture, cMaxNameLength);
	}
	
	// set the texture flags outside of the loop
	Imp_ClearTextureFlags(&flags);
	flags.hasFlagFile = cTextureFlagFile_Yes;
	flags.flags = M3cTextureFlags_HasMipMap;
		
	// ==== load in all the textures ====
	for(nodeItr = 0; nodeItr < count; nodeItr++)
	{
		BFtFileRef		*BMPFileRef;

		UUmAssert(NULL != inDirectory);

		error = BFrFileRef_DuplicateAndAppendName(
					inDirectory,
					textureFileNames[nodeItr],
					&BMPFileRef);
		IMPmError_ReturnOnError(error);
		
		error = Imp_ProcessTexture(
					BMPFileRef,
					IMPgLowMemory ? 2 : 0,
					&flags,
					NULL,
					texturePlaceholderNames[nodeItr],
					IMPcIgnoreTextureRef);
		IMPmError_ReturnOnError(error);
		
		UUmAssert(strchr(texturePlaceholderNames[nodeItr], '.') == NULL);
		bodyTextures->maps[nodeItr] = M3rTextureMap_GetPlaceholder(texturePlaceholderNames[nodeItr]);
		
		BFrFileRef_Dispose(BMPFileRef);
	}

	for(nodeItr = 0; nodeItr < count; nodeItr++)
	{
		UUrMemory_Block_Delete(texturePlaceholderNames[nodeItr]);
		texturePlaceholderNames[nodeItr] = NULL;

		UUrMemory_Block_Delete(textureFileNames[nodeItr]);
		textureFileNames[nodeItr] = NULL;
	}

	// delete the texture names
	UUrMemory_Block_Delete(textureFileNames);
	UUrMemory_Block_Delete(texturePlaceholderNames);
	UUrMemory_Block_Delete(texture_list);
	
	return UUcError_None;
}

UUtError 
Imp_AddBodyTextures(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError				error;

	char*					modelFileName;
	BFtFileRef*				modelFileRef;

	char*					directoryFileName;
	BFtFileRef*				directoryFileRef;
	
	// build the model file ref	
	error = GRrGroup_GetString(inGroup, "file", &modelFileName);
	IMPmError_ReturnOnError(error);

	error = BFrFileRef_MakeFromName(modelFileName, &modelFileRef);
	IMPmError_ReturnOnError(error);

	// build the directory file ref
	error =	GRrGroup_GetString(inGroup, "directory", &directoryFileName);
	if (UUcError_None == error)
	{
		error = BFrFileRef_MakeFromName(directoryFileName, &directoryFileRef);
		IMPmError_ReturnOnError(error);
	}
	else
	{
		directoryFileRef = NULL;
	}

	// setup is done actually make the textures
	error = Imp_ProcessBodyTextures(
		modelFileRef,
		directoryFileRef,
		inInstanceName);
	IMPmError_ReturnOnError(error);

	BFrFileRef_Dispose(modelFileRef);

	if (NULL != directoryFileRef)
	{
		BFrFileRef_Dispose(directoryFileRef);
	}

	return UUcError_None;
}


static void iCompressQuaternion(const M3tQuaternion *inQuaternion, void *outData, UUtUns16 compressionSize)
{
	UUmAssert(NULL != inQuaternion);
	UUmAssert(NULL != outData);

	MUmQuat_VerifyUnit(inQuaternion);

	if (TRcAnimationCompression3 == compressionSize) {
		double x,y,z,w,sinval;
		double xyrad,a_long,a_lat;
		int i_long, i_lat, i_angle;
		unsigned char *result3 = outData;

		w=acos(inQuaternion->w)/M3cPi;
		sinval=sin(w*M3cPi);		// sin(pheta/2)

		// An arbitrary constant for a small sinval.

		if (sinval>.000001) {
			x=inQuaternion->x/sinval;
			y=inQuaternion->y/sinval;
			z=inQuaternion->z/sinval;
		}
		else {
			x=1.0;
			y=0.0;
			z=0.0;
			w=0.0;	// identity transformation, 0 degree rotation around a random axis.
		}

		//UUmAssert(x>=-1.0 && x<=1.01);
		//UUmAssert(y>=-1.0 && y<=1.01);
		//UUmAssert(z>=-1.0 && z<=1.01);
		//UUmAssert(w>=0 && w<=1.01);

		// Okay, convert the components into euler angles

		xyrad=sqrt(x*x+y*y);
		a_lat=atan2(z,xyrad)+M3cPi;
		a_long=atan2(y,x)+M3cPi;

		i_long = (int) (a_long / (M3cPi*2.0f) * 255.0f + 0.5f );
		i_lat = (int) (a_lat  / (M3cPi*2.0f) * 255.0f + 0.5f);
		i_angle = (int) (w*255.0f+0.5f);

		result3[0]= i_long;
		result3[1]= i_lat;
		result3[2]= i_angle;

		//*result4 = (i_long & 255) | ((i_lat & 255) << 8) | ((i_angle & 255) << 16);

//		i_long = (int) (a_long / (M3cPi*2.0f) * 1023.0f + 0.5f );
//		i_lat = (int) (a_lat  / (M3cPi*2.0f) * 1023.0f + 0.5f);
//		i_angle = (int) (w*4095.0f+0.5f);
//
//		*result4 = (i_long & 1023) | ((i_lat & 1023) << 10) | ((i_angle & 4095) << 20);
	}

	if (TRcAnimationCompression4 == compressionSize) {
		M3tCompressedQuaternion4 *result4 = outData;

		result4->x = (UUtInt8) UUmPin((inQuaternion->x * UUcMaxInt8), UUcMinInt8, UUcMaxInt8);
		result4->y = (UUtInt8) UUmPin((inQuaternion->y * UUcMaxInt8), UUcMinInt8, UUcMaxInt8);
		result4->z = (UUtInt8) UUmPin((inQuaternion->z * UUcMaxInt8), UUcMinInt8, UUcMaxInt8);
		result4->w = (UUtInt8) UUmPin((inQuaternion->w * UUcMaxInt8), UUcMinInt8, UUcMaxInt8);
	}
	else if (TRcAnimationCompression6 == compressionSize) {
		M3tCompressedQuaternion6 *result6 = outData;
		MUtEuler euler;

		euler = MUrQuatToEuler(inQuaternion, MUcEulerOrderXYZr);
		UUmTrig_Clip(euler.x);
		UUmTrig_Clip(euler.y);
		UUmTrig_Clip(euler.z);

		result6->rx = (UUtUns16) UUmPin((euler.x * (((float) UUcMaxUns16) / M3c2Pi)), UUcMinUns16, UUcMaxUns16);
		result6->ry = (UUtUns16) UUmPin((euler.y * (((float) UUcMaxUns16) / M3c2Pi)), UUcMinUns16, UUcMaxUns16);
		result6->rz = (UUtUns16) UUmPin((euler.z * (((float) UUcMaxUns16) / M3c2Pi)), UUcMinUns16, UUcMaxUns16);

		UUmSwapLittle_2Byte(&(result6->rx));
		UUmSwapLittle_2Byte(&(result6->ry));
		UUmSwapLittle_2Byte(&(result6->rz));
	}
	else if (TRcAnimationCompression8 == compressionSize) {
		M3tCompressedQuaternion8 *result8 = outData;

		result8->x = (UUtInt16) UUmPin((inQuaternion->x * UUcMaxInt16), UUcMinInt16, UUcMaxInt16);
		result8->y = (UUtInt16) UUmPin((inQuaternion->y * UUcMaxInt16), UUcMinInt16, UUcMaxInt16);
		result8->z = (UUtInt16) UUmPin((inQuaternion->z * UUcMaxInt16), UUcMinInt16, UUcMaxInt16);
		result8->w = (UUtInt16) UUmPin((inQuaternion->w * UUcMaxInt16), UUcMinInt16, UUcMaxInt16);

		// if you remove this promise me you know what you are doing
		UUmSwapLittle_2Byte(&(result8->x));
		UUmSwapLittle_2Byte(&(result8->y));
		UUmSwapLittle_2Byte(&(result8->z));
		UUmSwapLittle_2Byte(&(result8->w));
	}
	else if (TRcAnimationCompression16 == compressionSize) {
		M3tQuaternion *result16 = outData;

		result16->x = inQuaternion->x;
		result16->y = inQuaternion->y;
		result16->z = inQuaternion->z;
		result16->w = inQuaternion->w;

		// if you remove this promise me you know what you are doing
		UUmSwapLittle_4Byte(&(result16->x));
		UUmSwapLittle_4Byte(&(result16->y));
		UUmSwapLittle_4Byte(&(result16->z));
		UUmSwapLittle_4Byte(&(result16->w));
	}
}

static TRtAnimTime iNextTime(TRtAnimTime inTime, UUtUns16 fps, UUtInt8 inDirection)
{
	TRtAnimTime curTime = inTime;

	UUmAssert(fps > 0);

	while(1)
	{
		curTime += inDirection;

		// now if curTime is a frame stop

		if (0 == curTime) break;
		if (((curTime * fps) / 60) != (((curTime + 1) * fps) / 60)) break;
	}

	return curTime;
}

static UUtError iInsertAnimationArrays(	TRtAnimation *ioAnimation,
									UUtInt8	inDirection,
									UUtBool	inTurnOveride,
									float inTurnOverideValue,
									const M3tQuaternion *inQuatArray,
									const M3tPoint3D *inPositionArray,
									const TRtAttack *attacks,
									const TRtTakeDamage *takeDamages,
									const TRtBlur *blurs,
									const TRtShortcut *shortcuts,
									const TRtThrowInfo *throwInfo,
									const TRtParticle *particles,
									const IMPtFramePositionInfo *framePositionInfo,
									const TRtExtentInfo *inExtentInfo,
									UUtBool inReverseFootsteps,
									const TRtFootstepList *inFootsteps)

{
	UUtUns32 frameItr;
	UUtUns32 attackExtentItr;
	TRtDirection anim_direction;
	M3tMatrix4x3 max2oni;
	TRtAnimTime srcTime;
	TRtAnimTime nextSrcTime;
	M3tVector3D initialPt;
	TRtPosition *position_pt;
	UUtInt16 max_attack_dist, this_attack_dist;
	UUtUns16 numFrames = IMPmAnimFrames(ioAnimation->duration, ioAnimation->fps);
#ifndef LOSSY_ANIM
	UUtUns32 nodeItr;
	UUtUns32 rotationDataSize = numFrames * ioAnimation->numParts  * ioAnimation->compressionSize;
#endif
	UUtBool suppress_attack_warnings = UUcFalse;

	max2oni.m[0][0] = 1.f;
	max2oni.m[1][0] = 0.0;
	max2oni.m[2][0] = 0.0;
	max2oni.m[3][0] = 0.0;

	max2oni.m[0][1] = 0.0;
	max2oni.m[1][1] = 0.0;
	max2oni.m[2][1] = 1.f;
	max2oni.m[3][1] = 0.0; 

	max2oni.m[0][2] = 0.0;
	max2oni.m[1][2] = -1.f;
	max2oni.m[2][2] = 0.0;
	max2oni.m[3][2] = 0.0;

	//
	// step 1 build in max coorindate system
	// 

#ifndef LOSSY_ANIM
	ioAnimation->data = TMrConstruction_Raw_New(rotationDataSize, 4, TRcTemplate_Animation);
#else
	ioAnimation->data = NULL;
#endif

	ioAnimation->attacks = TMrConstruction_Raw_New(sizeof(TRtAttack) * ioAnimation->numAttacks, 4, TRcTemplate_Animation);
	UUrMemory_MoveFast(attacks, ioAnimation->attacks, sizeof(TRtAttack) * ioAnimation->numAttacks);

	ioAnimation->takeDamages = TMrConstruction_Raw_New(sizeof(TRtTakeDamage) * ioAnimation->numTakeDamages, 4, TRcTemplate_Animation);
	UUrMemory_MoveFast(takeDamages, ioAnimation->takeDamages, sizeof(TRtTakeDamage) * ioAnimation->numTakeDamages);

	ioAnimation->blurs = TMrConstruction_Raw_New(sizeof(TRtBlur) * ioAnimation->numBlurs, 4, TRcTemplate_Animation);
	UUrMemory_MoveFast(blurs, ioAnimation->blurs, sizeof(TRtBlur) * ioAnimation->numBlurs);

	ioAnimation->shortcuts = TMrConstruction_Raw_New(sizeof(TRtShortcut) * ioAnimation->numShortcuts, 4, TRcTemplate_Animation);
	UUrMemory_MoveFast(shortcuts, ioAnimation->shortcuts, sizeof(TRtShortcut) * ioAnimation->numShortcuts);

	if (inFootsteps == NULL) {
		ioAnimation->numFootsteps = 0;
		ioAnimation->footsteps = NULL;
	} else {
		ioAnimation->numFootsteps = (UUtUns8) inFootsteps->numFootsteps;
		ioAnimation->footsteps = TMrConstruction_Raw_New(sizeof(TRtFootstep) * inFootsteps->numFootsteps, 4, TRcTemplate_Animation);
		if (inFootsteps->numFootsteps > 0) {
			UUrMemory_MoveFast(inFootsteps->footsteps, ioAnimation->footsteps, ioAnimation->numFootsteps * sizeof(TRtFootstep));
			if ((-1 == inDirection) && (inReverseFootsteps)) {
				UUtUns32 footstepItr;

				for(footstepItr = 0; footstepItr < ioAnimation->numFootsteps; footstepItr++)
				{
					if (ioAnimation->footsteps[footstepItr].type == TRcFootstep_Single) {
						// "single" footsteps are always at the start of the animation
						continue;
					}

					UUmAssert(ioAnimation->footsteps[footstepItr].frame <= ioAnimation->duration);
					ioAnimation->footsteps[footstepItr].frame = ioAnimation->duration - ioAnimation->footsteps[footstepItr].frame;
				}
			}
		}
	}

	if (ioAnimation->flags & (1 << ONcAnimFlag_ThrowSource)) { 
		ioAnimation->throwInfo = TMrConstruction_Raw_New(sizeof(TRtThrowInfo), 4, TRcTemplate_Animation);
		UUrMemory_MoveFast(throwInfo, ioAnimation->throwInfo, sizeof(TRtThrowInfo));
	}
	else {
		ioAnimation->throwInfo = NULL;
	}

	if (ioAnimation->flags & (1 << ONcAnimFlag_RealWorld)) {
		ioAnimation->heights = NULL;
	}
	else {
		ioAnimation->heights = TMrConstruction_Raw_New(sizeof(float) * ioAnimation->numFrames, 4, TRcTemplate_Animation);
	}

	ioAnimation->positions = TMrConstruction_Raw_New(sizeof(float) * 2 * ioAnimation->numFrames, 4, TRcTemplate_Animation);

	ioAnimation->particles = TMrConstruction_Raw_New(sizeof(TRtParticle) * ioAnimation->numParticles, 4, TRcTemplate_Animation);
	UUrMemory_MoveFast(particles, ioAnimation->particles, sizeof(TRtParticle) * ioAnimation->numParticles);

	ioAnimation->extentInfo = *inExtentInfo;
	if (ioAnimation->extentInfo.numAttackExtents > 0) {
		ioAnimation->extentInfo.attackExtents = TMrConstruction_Raw_New(sizeof(TRtAttackExtent) * ioAnimation->extentInfo.numAttackExtents, 4, TRcTemplate_Animation);
	} else {
		ioAnimation->extentInfo.attackExtents = NULL;
	}

	ioAnimation->positionPoints = TMrConstruction_Raw_New(sizeof(TRtPosition) * ioAnimation->numFrames, 4, TRcTemplate_Animation);

	// heights just sample
	// points take delta over correct number of frames

	if (1 == inDirection) {
		srcTime = 0;
	}
	else {
		srcTime = ioAnimation->duration; 
	}

	if (ioAnimation->flags & (1 << ONcAnimFlag_ThrowTarget)) {
		// this is necessary, because throw target characters have wacky orientation
		suppress_attack_warnings = UUcTrue;
	}
	
	if (ioAnimation->numAttacks > 0) {
		// work out the direction of this attack
		anim_direction = (TRtDirection) ioAnimation->moveDirection;
		if (anim_direction == TRcDirection_None) {
			anim_direction = inExtentInfo->computed_attack_direction;

			UUmAssert(anim_direction != TRcDirection_None);
		}
	}

	// no attacks found yet
	attackExtentItr = 0;
	ioAnimation->extentInfo.firstAttack.frame_index = (UUtUns16) -1;
	ioAnimation->extentInfo.maxAttack.frame_index = (UUtUns16) -1;
	max_attack_dist = 0;

	initialPt = inPositionArray[srcTime];
	for(frameItr = 0; frameItr < ioAnimation->numFrames; srcTime = nextSrcTime, frameItr++)
	{
		M3tPoint3D delta, cumulativeDelta, srcPt, dstPt;
		TRtAttackExtent *extent;
		float float_dist;

		nextSrcTime = iNextTime(srcTime, ioAnimation->fps, inDirection);
		nextSrcTime = UUmMin(nextSrcTime, ioAnimation->duration);

		// calculate delta-position for this tick
		srcPt = inPositionArray[srcTime];
		dstPt = inPositionArray[nextSrcTime];

		MUmVector_Subtract(cumulativeDelta, dstPt, initialPt);
		MUmVector_Subtract(delta, dstPt, srcPt);

		if (ioAnimation->flags & (1 << ONcAnimFlag_RealWorld)) {
		}
		else {
			ioAnimation->heights[frameItr] = inPositionArray[srcTime].z;

			// copy in information from the relevant place in the framePositionInfo
			position_pt = ioAnimation->positionPoints + frameItr;
			*position_pt = framePositionInfo[srcTime].position;

			// we flip the Y axis in preparation for the transformation into oni coordinates
			position_pt->location_y *= -1;

			if (framePositionInfo[srcTime].is_attack_frame) {
				extent = (TRtAttackExtent *) &framePositionInfo[srcTime].attack_extent;

				if (ioAnimation->extentInfo.firstAttack.frame_index == (UUtUns16) -1) {
					// store the first frame of attack in the animation's global extentInfo
					ioAnimation->extentInfo.firstAttack.frame_index		= (UUtUns16) frameItr;
					ioAnimation->extentInfo.firstAttack.attack_index	= (UUtUns8) framePositionInfo[srcTime].attack_index;
					ioAnimation->extentInfo.firstAttack.extent_index	= (UUtUns8) attackExtentItr;

					ioAnimation->extentInfo.firstAttack.position.x		= TRcPositionGranularity * framePositionInfo[srcTime].position.location_x;
					ioAnimation->extentInfo.firstAttack.position.y		= TRcPositionGranularity * framePositionInfo[srcTime].position.location_y;
					ioAnimation->extentInfo.firstAttack.position.z		= ioAnimation->heights[frameItr];
					ioAnimation->extentInfo.firstAttack.attack_dist		= TRcPositionGranularity * extent->attack_distance;
					ioAnimation->extentInfo.firstAttack.attack_minheight= TRcPositionGranularity * extent->attack_minheight;
					ioAnimation->extentInfo.firstAttack.attack_maxheight= TRcPositionGranularity * extent->attack_maxheight;
					ioAnimation->extentInfo.firstAttack.attack_angle	= TRcAngleGranularity    * extent->attack_angle;
				}

				// the attack's distance from the animation's starting location is given by its distance
				// in the attack direction, plus the distance of our location that frame from the starting location.
				this_attack_dist = extent->attack_distance;
				switch(anim_direction) {
					// distance is measured omnidirectionally
					case TRcDirection_360:
						float_dist = MUrSqrt((float) (UUmSQR(position_pt->location_x) + UUmSQR(position_pt->location_y)));
						this_attack_dist += (UUtInt16) MUrUnsignedSmallFloat_To_Uns_Round(float_dist);
					break;

					case TRcDirection_Forwards:
						this_attack_dist += position_pt->location_y;
					break;

					case TRcDirection_Left:
						this_attack_dist += position_pt->location_x;
					break;

					case TRcDirection_Right:
						this_attack_dist -= position_pt->location_x;
					break;

					case TRcDirection_Back:
						this_attack_dist -= position_pt->location_y;
					break;

					default:
						UUmAssert(0);
					break;
				}

				if (this_attack_dist > max_attack_dist) {
					// this frame is the furthest that we have attacked from our starting point!
					// store this frame in the animation's global extentInfo, as the max attack
					ioAnimation->extentInfo.maxAttack.frame_index		= (UUtUns16) frameItr;
					ioAnimation->extentInfo.maxAttack.attack_index		= (UUtUns8) framePositionInfo[srcTime].attack_index;
					ioAnimation->extentInfo.maxAttack.extent_index		= (UUtUns8) attackExtentItr;

					ioAnimation->extentInfo.maxAttack.position.x		= TRcPositionGranularity * framePositionInfo[srcTime].position.location_x;
					ioAnimation->extentInfo.maxAttack.position.y		= TRcPositionGranularity * framePositionInfo[srcTime].position.location_y;
					ioAnimation->extentInfo.maxAttack.position.z		= ioAnimation->heights[frameItr];
					ioAnimation->extentInfo.maxAttack.attack_dist		= TRcPositionGranularity * extent->attack_distance;
					ioAnimation->extentInfo.maxAttack.attack_minheight	= TRcPositionGranularity * extent->attack_minheight;
					ioAnimation->extentInfo.maxAttack.attack_maxheight	= TRcPositionGranularity * extent->attack_maxheight;
					ioAnimation->extentInfo.maxAttack.attack_angle		= TRcAngleGranularity    * extent->attack_angle;

					max_attack_dist = this_attack_dist;
				}

				// copy height and distance extent into the extent buffer
				ioAnimation->extentInfo.attackExtents[attackExtentItr++] = framePositionInfo[srcTime].attack_extent;
			}
		}

		// we flip the Y axis in preparation for the transformation into oni coordinates
		ioAnimation->positions[(2 * frameItr)] = delta.x;
		ioAnimation->positions[(2 * frameItr) + 1] = -delta.y;

#ifndef LOSSY_ANIM
		for(nodeItr = 0; nodeItr < ioAnimation->numParts; nodeItr++)
		{
			M3tQuaternion thisQuat;
			UUtUns32 srcIndex;
			UUtUns32 dstIndex;

			srcIndex = (srcTime * ioAnimation->numParts) + nodeItr;
			dstIndex = (frameItr * ioAnimation->numParts * ioAnimation->compressionSize) + nodeItr * ioAnimation->compressionSize;

			thisQuat = inQuatArray[srcIndex];

			// fix coordiante system
			if ((0 == nodeItr) && !(ioAnimation->flags & (1 << ONcAnimFlag_Overlay))) {
				M3tMatrix4x3 rootMatrix;

				MUrQuatToMatrix(&thisQuat, &rootMatrix);
				MUrMatrix_Multiply(&max2oni, &rootMatrix, &rootMatrix);
				MUrMatrixToQuat(&rootMatrix, &thisQuat);
			}

			MUmQuat_VerifyUnit(&thisQuat);
	
			iCompressQuaternion(
				&thisQuat, 
				&(((UUtUns8 *)(ioAnimation->data)) [dstIndex]), 
				ioAnimation->compressionSize);
		}
#endif
	}

#ifdef LOSSY_ANIM

  // The actual hook into the bungie code for the animation compression.
  // We make a pass to get the size of the data we're going to need,
  // allocate the memory, make a pass to fill the memory, and then we're
  // done.

  // Given that we're assuming 60 fps, make sure.

  UUmAssert(ioAnimation->fps==60);
  {
      // get the size of the animation...

      UUtUns32 rotationDataSize;
      rotationDataSize=0;
      do_lossy_anim_compress(ioAnimation, NULL,
          inDirection, inQuatArray,&rotationDataSize);

      // now get the memory.

      ioAnimation->data = TMrConstruction_Raw_New(rotationDataSize, 4, TRcTemplate_Animation);

      // now fill it in. Simple, no?

      do_lossy_anim_compress(ioAnimation, ioAnimation->data,
          inDirection, inQuatArray,&rotationDataSize);
  }
#endif


	UUmAssert(attackExtentItr == ioAnimation->extentInfo.numAttackExtents);
	if (attackExtentItr > UUcMaxUns8) {
		Imp_PrintWarning("too many attack frames in this animation (max 256)");
		return UUcError_Generic;
	}

	if (!suppress_attack_warnings && (ioAnimation->numAttacks > 0)) {
		if (ioAnimation->extentInfo.firstAttack.frame_index == (UUtUns16) -1) {
			Imp_PrintWarning("did not find the first attack, but anim was flagged as attack!");
			return UUcError_Generic;
		}

		if (ioAnimation->extentInfo.maxAttack.frame_index == (UUtUns16) -1) {
			Imp_PrintWarning("did not find any attacking parts in the attack's direction!");
			return UUcError_Generic;
		}
	}

	TRrAnimation_SwapHeights(ioAnimation);
	TRrAnimation_SwapPositions(ioAnimation);
	TRrAnimation_SwapAttacks(ioAnimation);
	TRrAnimation_SwapTakeDamages(ioAnimation);
	TRrAnimation_SwapBlurs(ioAnimation);
	TRrAnimation_SwapShortcuts(ioAnimation);
	TRrAnimation_SwapParticles(ioAnimation);
	TRrAnimation_SwapPositionPoints(ioAnimation);
	TRrAnimation_SwapExtentInfo(ioAnimation);

	ioAnimation->heights = TMrConstruction_Raw_Write(ioAnimation->heights);
	ioAnimation->positions = TMrConstruction_Raw_Write(ioAnimation->positions);
	ioAnimation->attacks = TMrConstruction_Raw_Write(ioAnimation->attacks);
	ioAnimation->takeDamages = TMrConstruction_Raw_Write(ioAnimation->takeDamages);
	ioAnimation->blurs = TMrConstruction_Raw_Write(ioAnimation->blurs);
	ioAnimation->shortcuts = TMrConstruction_Raw_Write(ioAnimation->shortcuts);
	ioAnimation->throwInfo = TMrConstruction_Raw_Write(ioAnimation->throwInfo); 
	ioAnimation->footsteps = TMrConstruction_Raw_Write(ioAnimation->footsteps); 
	ioAnimation->particles = TMrConstruction_Raw_Write(ioAnimation->particles);
	ioAnimation->positionPoints = TMrConstruction_Raw_Write(ioAnimation->positionPoints);
	ioAnimation->extentInfo.attackExtents = TMrConstruction_Raw_Write(ioAnimation->extentInfo.attackExtents);
	ioAnimation->data = TMrConstruction_Raw_Write(ioAnimation->data);

	return UUcError_None;
}

static UUtError iCreateBlankAnimation(
						char *inInstanceName, 
						UUtUns16 inDuration, 
						UUtUns16 inFPS,
						UUtUns16 inNumNodes, 
						UUtUns8 inCompressionSize, 
						TMtPlaceHolder inSoundPlaceHolders[TRcMaxSounds], 
						UUtUns16 inSoundFrames[TRcMaxSounds], 

						TRtAttack inAttacks[TRcMaxAttacks],
						TRtBlur inBlurs[TRcMaxBlurs],
						TMtPlaceHolder inDirectAnimation[TRcMaxDirectAnimations],

						TRtAnimation **outAnimation)
{
	UUtError		error;
	UUtUns16		numFrames = IMPmAnimFrames(inDuration, inFPS);
	UUtUns32		rotationDataSize = numFrames * inNumNodes * inCompressionSize;
	UUtUns16		itr;

	UUmAssert(inFPS > 0);

	// create the template with the correctly sized variable array
	error = 
		TMrConstruction_Instance_Renew(
			TRcTemplate_Animation,
			inInstanceName,
			0,
			outAnimation);
	UUmError_ReturnOnErrorMsgP(error, "failed to renew animation name = %s", (UUtUns32) inInstanceName, 0, 0);

	
	(*outAnimation)->positions = NULL;
	(*outAnimation)->heights = NULL;	
	(*outAnimation)->particles = NULL;
	(*outAnimation)->extentInfo.attackExtents = NULL;
	(*outAnimation)->positionPoints = NULL;
	(*outAnimation)->compressionSize = inCompressionSize;
	(*outAnimation)->numFrames = numFrames;
	(*outAnimation)->duration = inDuration;
	(*outAnimation)->fps = inFPS;
	(*outAnimation)->numParts = inNumNodes;

/*	for(itr = 0; itr < TRcMaxSounds; itr++)
	{
		(*outAnimation)->sound[itr] = (SStSound *) inSoundPlaceHolders[itr];
		(*outAnimation)->soundFrame[itr] = inSoundFrames[itr];
	}*/
	(*outAnimation)->soundName = 0;
	(*outAnimation)->soundFrame = 0;

	for(itr = 0; itr < TRcMaxDirectAnimations; itr++)
	{
		(*outAnimation)->directAnimation[itr] = (TRtAnimation *) inDirectAnimation[itr];
	}

	UUmAssert((*outAnimation)->fps > 0);

	return UUcError_None;
}

typedef struct ANMHeader {
	UUtUns16 numNodes;
	UUtUns16 numFileFrames;
	UUtUns16 duration;
	UUtUns16 pad;
	UUtInt32 startFrame;
	UUtInt32 endFrame;
	UUtUns32 ticksPerFrame;
	UUtUns32 startTime;
	UUtUns32 endTime;
	M3tPoint3D rootTranslation[2];
} ANMHeader;

static UUtError Imp_ANMFile_Read(
	BFtFileRef		*inDataFile,
	UUtUns32		inFlags,
	ANMHeader		*outHeader,
	M3tPoint3D		**outPositionArray,
	M3tQuaternion	**outQuaternionArray,
	M3tMatrix4x3	**outWorldMatrixArray)
{
	M3tPoint3D *positionArray;
	M3tQuaternion *quatArray;
	M3tMatrix4x3 *worldMatrixArray, *matrixPtr, *thisnode;
	UUtUns16	nodeItr;
	UUtUns16	frameItr;
	UUtUns32	numComments;
	BFtTextFile	*textFile;

	UUtUns16	numFileFrames;
	UUtUns16	numNodes;

	UUtError error;
	UUtBool warnOnce = UUcTrue;

	UUmAssertReadPtr(inDataFile, 1);
	UUmAssertWritePtr(outHeader, sizeof(*outHeader));
	UUmAssertWritePtr(outPositionArray, sizeof(*outPositionArray));
	UUmAssertWritePtr(outQuaternionArray, sizeof(*outQuaternionArray));
	UUmAssertWritePtr(outWorldMatrixArray, sizeof(*outWorldMatrixArray));

	// open the file
	error = BFrTextFile_MapForRead(inDataFile, &textFile);
	IMPmError_ReturnOnError(error);

	// ============ process the header first
	error = BFrTextFile_VerifyNextStr(textFile, "Animation File");
	IMPmError_ReturnOnError(error);

	error = BFrTextFile_VerifyNextStr(textFile, "Version");
	IMPmError_ReturnOnError(error);

	error = BFrTextFile_VerifyNextStr(textFile, "1.4");
	IMPmError_ReturnOnError(error);

	error = BFrTextFile_VerifyNextStr(textFile, "comments");
	IMPmError_ReturnOnError(error);

	numComments = BFrTextFile_GetUUtUns32(textFile);
	for(; numComments > 0; numComments--)
	{
		BFrTextFile_GetNextStr(textFile);
	}

	error = BFrTextFile_VerifyNextStr(textFile, "Timestamp");
	IMPmError_ReturnOnError(error);

	BFrTextFile_GetNextStr(textFile);

	error = BFrTextFile_VerifyNextStr(textFile, "node count =");
	IMPmError_ReturnOnError(error);

	numNodes = BFrTextFile_GetUUtUns16(textFile);
	outHeader->numNodes = numNodes;

	error = BFrTextFile_VerifyNextStr(textFile, "frame count=");
	IMPmError_ReturnOnError(error);

	numFileFrames = BFrTextFile_GetUUtUns16(textFile);

	outHeader->numFileFrames = numFileFrames;
	outHeader->duration = outHeader->numFileFrames - 1;

	error = BFrTextFile_VerifyNextStr(textFile, "start frame");
	IMPmError_ReturnOnError(error);

	outHeader->startFrame = BFrTextFile_GetUUtInt32(textFile);	

	error = BFrTextFile_VerifyNextStr(textFile, "end frame");
	IMPmError_ReturnOnError(error);

	outHeader->endFrame = BFrTextFile_GetUUtInt32(textFile);	

	error = BFrTextFile_VerifyNextStr(textFile, "ticks per frame");
	IMPmError_ReturnOnError(error);

	outHeader->ticksPerFrame = BFrTextFile_GetUUtUns32(textFile);	

	error = BFrTextFile_VerifyNextStr(textFile, "start time");
	IMPmError_ReturnOnError(error);

	outHeader->startTime = BFrTextFile_GetUUtUns32(textFile);	

	error = BFrTextFile_VerifyNextStr(textFile, "end time");
	IMPmError_ReturnOnError(error);

	outHeader->endTime = BFrTextFile_GetUUtUns32(textFile);	

	error = BFrTextFile_VerifyNextStr(textFile, "");
	IMPmError_ReturnOnError(error);

	// ============ account for flags
	if (inFlags & (1 << ONcAnimFlag_ThrowSource)) {
		outHeader->numNodes /= 2;
	}
	else if (inFlags & (1 << ONcAnimFlag_ThrowTarget)) {
		outHeader->numNodes /= 2;
	}

	// ============ process the main arrays

	positionArray = UUrMemory_Block_New(sizeof(M3tPoint3D) * outHeader->numFileFrames);
	UUmError_ReturnOnNull(positionArray);

	quatArray = UUrMemory_Block_New(sizeof(M3tQuaternion) * outHeader->numFileFrames * outHeader->numNodes);
	UUmError_ReturnOnNull(quatArray);

	worldMatrixArray = UUrMemory_Block_New(sizeof(M3tMatrix4x3) * outHeader->numFileFrames * outHeader->numNodes);
	UUmError_ReturnOnNull(worldMatrixArray);

	for(frameItr = 0; frameItr < numFileFrames; frameItr++)
	{
		UUtUns32 time;
		UUtUns16 nodeWriteCount = 0;;

		error = BFrTextFile_VerifyNextStr(textFile, "time");
		IMPmError_ReturnOnError(error);

		time = BFrTextFile_GetUUtUns32(textFile);

		matrixPtr = worldMatrixArray + outHeader->numNodes * frameItr;

		for(nodeItr = 0; nodeItr < numNodes; nodeItr++)
		{
			int bipIndex;
			UUtBool rootNode;
			UUtUns8 parent_index;

			M3tMatrix4x3 matrix;

			M3tQuaternion	quaternion;
			M3tPoint3D		translation;
			MUtEuler		euler;
			UUtBool			writeThisNode = UUcTrue;
			char			*nodeName;

			error = BFrTextFile_VerifyNextStr(textFile, "name");
			IMPmError_ReturnOnError(error);

			// read name
			nodeName = BFrTextFile_GetNextStr(textFile);

			// are we a root node ?
			rootNode = UUcFalse;
			rootNode |= (0 == strcmp(nodeName, "Bip01 Pelvis"));
			rootNode |= (0 == strcmp(nodeName, "Bip02 Pelvis"));

			// calculate biped index
			bipIndex = 0;

			if (UUrString_HasPrefix(nodeName, "Bip01")) {
				bipIndex = 0;
			}
			else if (UUrString_HasPrefix(nodeName, "Bip02")) {
				bipIndex = 1;
			}
			else {
				if (warnOnce) {
					Imp_PrintWarning("invalid biped prefix");
					warnOnce = UUcFalse;
				}
			}

			// should we be writing this node ?
			if (inFlags & (1 << ONcAnimFlag_ThrowSource)) {
				writeThisNode = 1 == bipIndex;
			}
			else if (inFlags & (1 << ONcAnimFlag_ThrowTarget)) {
				writeThisNode = 0 == bipIndex;
			}

			// read matrix for this node
			matrix = ReadMatrix(textFile);

			translation = MUrMatrix_GetTranslation(&matrix);

			// compute the translations for each root node (frame 0 only)
			if ((rootNode) && (0 == frameItr))
			{
				outHeader->rootTranslation[bipIndex] = translation;
			}

			// compute euler, make quaternion
			if (writeThisNode) 
			{
				euler = MUrMatrixToEuler(&matrix, MUcEulerOrderZYZs);
				quaternion = MUrEulerToQuat(&euler);
				MUmQuat_VerifyUnit(&quaternion);

				quatArray[(frameItr * outHeader->numNodes) + nodeWriteCount] = quaternion;

				// fill in the temporaray arrays
				if (0 == nodeWriteCount)
				{
					positionArray[frameItr] = translation;
				}

				// compute the worldspace translation for this node
				thisnode = matrixPtr + nodeWriteCount;

				// find this node's location in the hierarchy of the biped
				UUmAssert(nodeWriteCount < IMPgLastBiped->indexBlockStorage->numIndexBlocks);

				parent_index = IMPgLastBiped->indexBlocks[nodeWriteCount].parent;
				if (parent_index == nodeWriteCount) {
					// this is the root node
					*thisnode = matrix;
				} else {
					// get the worldspace location by multiplying our relative matrix by our parent's
					// worldspace matrix
					MUrMatrix_Multiply(&matrixPtr[parent_index], &matrix, thisnode);
				}

				nodeWriteCount++;
			}
		}

		if (outHeader->numNodes != nodeWriteCount) {
			if (warnOnce) {
				Imp_PrintWarning("incorrect number of nodes found!");
				warnOnce = UUcFalse;
			}
		}

		error = BFrTextFile_VerifyNextStr(textFile, "");
		IMPmError_ReturnOnError(error);
	}

	BFrTextFile_Close(textFile);

	*outPositionArray = positionArray;
	*outQuaternionArray = quatArray;
	*outWorldMatrixArray = worldMatrixArray;

	return UUcError_None;
}

typedef struct tANDFileDateVersion
{
	UUtUns32 version;
	UUtUns32 timestamp;
} tANDFileDateVersion;


static UUtError Imp_ANDFile_Write(
	BFtFileRef				*inDataFile,
	UUtUns32				inTimestamp,
	ANMHeader				*inHeader,
	M3tPoint3D				*inPositionArray,
	M3tQuaternion			*inQuaternionArray,
	IMPtFramePositionInfo	*inFramePositionArray,
	TRtExtentInfo			*inExtentInfo,
	TRtFootstepList			*inFootsteps)
{
	UUtError error;
	BFtFile *dstFile;
	UUtUns32 numPos;
	UUtUns32 numQuat;
	tANDFileDateVersion stamp;

	stamp.version = cANDFile_VERSION;
	stamp.timestamp = inTimestamp;

	UUmAssertReadPtr(inDataFile, 4);
	UUmAssertReadPtr(inHeader, sizeof(*inHeader));
	UUmAssertReadPtr(inPositionArray, sizeof(*inPositionArray));
	UUmAssertReadPtr(inQuaternionArray, sizeof(*inQuaternionArray));
	UUmAssertReadPtr(inFramePositionArray, sizeof(*inFramePositionArray));

	numPos = inHeader->numFileFrames;
	numQuat = inHeader->numFileFrames * inHeader->numNodes;

	error = BFrFile_Open(inDataFile, "w", &dstFile);
	UUmError_ReturnOnErrorMsgP(error, "failed to open %s w/ write priv", 
		(UUtUns32) BFrFileRef_GetLeafName(inDataFile), 0, 0);

	error = BFrFile_Write(dstFile, sizeof(stamp), &stamp);
	IMPmError_ReturnOnError(error);

	error = BFrFile_Write(dstFile, sizeof(ANMHeader), inHeader);
	IMPmError_ReturnOnError(error);

	error = BFrFile_Write(dstFile, sizeof(M3tPoint3D) * numPos, inPositionArray);
	IMPmError_ReturnOnError(error);

	error = BFrFile_Write(dstFile, sizeof(M3tQuaternion) * numQuat, inQuaternionArray);
	IMPmError_ReturnOnError(error);

	error = BFrFile_Write(dstFile, sizeof(IMPtFramePositionInfo) * numPos, inFramePositionArray);
	IMPmError_ReturnOnError(error);

	error = BFrFile_Write(dstFile, sizeof(TRtExtentInfo), inExtentInfo);
	IMPmError_ReturnOnError(error);

	{
		UUtUns32 sizeof_block = sizeof(TRtFootstepList) + sizeof(TRtFootstep) * inFootsteps->numFootsteps;

		error = BFrFile_Write(dstFile, sizeof_block, inFootsteps);
	}

	BFrFile_Close(dstFile);

	return UUcError_None;
}

static UUtError Imp_ANDFile_Read(
	BFtFileRef			*inDataFile,
	UUtUns32			inTimestamp,
	BFtFileMapping		**outFileMapping,
	ANMHeader			**outHeader,
	M3tPoint3D			**outPositionArray,
	M3tQuaternion		**outQuaternionArray,
	IMPtFramePositionInfo **outFramePositionArray,
	TRtExtentInfo		*outExtentInfo,
	TRtFootstepList		**outFootstepList)
{
	UUtError error;
	UUtUns32 numPos;
	UUtUns32 numQuat;
	BFtFileMapping *fileMapping;
	ANMHeader		*header;
	M3tPoint3D		*positionArray;
	M3tQuaternion	*quaternionArray;
	IMPtFramePositionInfo *framePositionArray;
	TRtExtentInfo	*extentInfo;

	tANDFileDateVersion *stamp;
	UUtUns32 fileLength;
	UUtUns32 totalSize = 0;
	UUtUns8 *data;

	UUmAssertReadPtr(inDataFile, 4);
	UUmAssertReadPtr(outHeader, sizeof(*outHeader));
	UUmAssertReadPtr(outPositionArray, sizeof(*outPositionArray));
	UUmAssertReadPtr(outQuaternionArray, sizeof(*outQuaternionArray));
	UUmAssertReadPtr(outFramePositionArray, sizeof(*outFramePositionArray));
	UUmAssertWritePtr(outExtentInfo, sizeof(*outExtentInfo));

	error = BFrFile_Map(inDataFile, 0, &fileMapping, &data, &fileLength);
	if (error != UUcError_None) { return error; }

	stamp = (tANDFileDateVersion *) data;
	data += sizeof(*stamp);
	totalSize += sizeof(*stamp);

	if ((stamp->version != cANDFile_VERSION) || (inTimestamp > stamp->timestamp)) {
		Imp_PrintMessage(IMPcMsg_Cosmetic, "out of date AND file");
		BFrFile_UnMap(fileMapping);

		// delete the old version of the file
		error = BFrFile_Delete(inDataFile);
		if (error != UUcError_None) {
			Imp_PrintMessage(IMPcMsg_Cosmetic, "can't delete old-version AND file");
		}
		return UUcError_Generic;
	}

	header = (ANMHeader *) data;
	data += sizeof(ANMHeader);
	totalSize += sizeof(ANMHeader);
	IMPmError_ReturnOnError(error);

	// calculate the array sizes
	numPos = header->numFileFrames;
	numQuat = header->numFileFrames * header->numNodes;

	// read the arrays
	positionArray = (M3tPoint3D *) data;
	data += sizeof(M3tPoint3D) * numPos;
	totalSize += sizeof(M3tPoint3D) * numPos;

	quaternionArray = (M3tQuaternion *) data;
	data += sizeof(M3tQuaternion) * numQuat;
	totalSize += sizeof(M3tQuaternion) * numQuat;

	framePositionArray = (IMPtFramePositionInfo *) data;
	data += sizeof(IMPtFramePositionInfo) * numPos;
	totalSize += sizeof(IMPtFramePositionInfo) * numPos;

	extentInfo = (TRtExtentInfo *) data;
	data += sizeof(TRtExtentInfo);
	totalSize += sizeof(TRtExtentInfo);

	*outFootstepList = (TRtFootstepList *) data;
	data += sizeof(TRtFootstepList) + sizeof(TRtFootstep) + (*outFootstepList)->numFootsteps;
	totalSize += sizeof(TRtFootstepList);

	if (totalSize > fileLength) {
		Imp_PrintMessage(IMPcMsg_Cosmetic, "File size to small!");
		BFrFile_UnMap(fileMapping);

		// delete the old version of the file
		error = BFrFile_Delete(inDataFile);
		if (error != UUcError_None) {
			Imp_PrintMessage(IMPcMsg_Cosmetic, "can't delete incorrect-size AND file");
		}
		return UUcError_Generic;
	}

	*outFileMapping = fileMapping;
	*outHeader = header;
	*outPositionArray = positionArray;
	*outQuaternionArray = quaternionArray;
	*outFramePositionArray = framePositionArray;
	*outExtentInfo = *extentInfo;
	outExtentInfo->attackExtents = NULL;		// array not yet set up

	return UUcError_None;
}

static BFtFileRef *iMakeANDFileRef(BFtFileRef *inFileRef)
{
	UUtError error;
	BFtFileRef *newFileRef;
	const char *oldLeafName = BFrFileRef_GetLeafName(inFileRef);
	char newLeafName[BFcMaxFileNameLength + 1];
	char *scan;

	UUmAssertReadPtr(inFileRef, 4);

	UUrString_Copy(newLeafName, oldLeafName, BFcMaxFileNameLength);
	
	scan = newLeafName;

	while((*scan != '.') && (*scan != '\0')) { scan++; }

	UUmAssert('.' == scan[0]);
	if ('.' != scan[0]) return NULL;

	scan[0] = '.';
	scan[1] = 'a';
	scan[2] = 'n';
	scan[3] = 'd';

	error = BFrFileRef_DuplicateAndReplaceName(inFileRef, newLeafName, &newFileRef);
	UUmAssert(UUcError_None == error);
	if (error) return NULL;

	return newFileRef;
}

static UUtBool iSetAttackDirection_From_State(TRtAnimState state, TRtDirection *outDirection)
{
	switch(state) 
	{
		case ONcAnimState_Running_Right_Down:
		case ONcAnimState_Running_Left_Down:
		case ONcAnimState_Falling_Forward:
			*outDirection = TRcDirection_Forwards;
		return UUcTrue;

		case ONcAnimState_Run_Sidestep_Left:
		case ONcAnimState_Sidestep_Left_Left_Down:
		case ONcAnimState_Sidestep_Left_Right_Down:
		case ONcAnimState_Walking_Sidestep_Left:
		case ONcAnimState_Falling_Left:
			*outDirection = TRcDirection_Left;
		return UUcTrue;

		case ONcAnimState_Run_Sidestep_Right:
		case ONcAnimState_Sidestep_Right_Left_Down:
		case ONcAnimState_Sidestep_Right_Right_Down:
		case ONcAnimState_Walking_Sidestep_Right:
		case ONcAnimState_Falling_Right:
			*outDirection = TRcDirection_Right;
		return UUcTrue;

		case ONcAnimState_Running_Back_Right_Down:
		case ONcAnimState_Running_Back_Left_Down:
		case ONcAnimState_Falling_Back:
			*outDirection = TRcDirection_Back;
		return UUcTrue;
	}

	return UUcFalse;
}


static void iSetAttackDirection(UUtUns32 inFlags, UUtUns8 inNumAttacks, TRtAnimType inAnimType, TRtAnimState inFromState,
								UUtUns8 inNumShortcuts, TRtShortcut *inShortcuts, TRtDirection *outDirection)
{
	const UUtUns32 aim_mask = 
		(1 << ONcAnimFlag_Aim_Forward) | 
		(1 << ONcAnimFlag_Aim_Left) | 
		(1 << ONcAnimFlag_Aim_Right) | 
		(1 << ONcAnimFlag_Aim_Backward) |
		(1 << ONcAnimFlag_Aim_360);
	UUtUns32 itr;

	*outDirection = TRcDirection_None;

	// does it have a direction specified in the INS ?
	if (inFlags & aim_mask) {

		if (inFlags & (1 << ONcAnimFlag_Aim_360)) {
			*outDirection = TRcDirection_360;

		} else if (inFlags & (1 << ONcAnimFlag_Aim_Forward)) {
			*outDirection = TRcDirection_Forwards;

		} else if (inFlags & (1 << ONcAnimFlag_Aim_Left)) {
			*outDirection = TRcDirection_Left;

		} else if (inFlags & (1 << ONcAnimFlag_Aim_Right)) {
			*outDirection = TRcDirection_Right;

		} else if (inFlags & (1 << ONcAnimFlag_Aim_Backward)) {
			*outDirection = TRcDirection_Back;

		} else {
			UUmAssert(0);
		}

		return;
	}

	// is this not an attack ?
	if (inNumAttacks == 0) {
		return;
	}

	// ok build the attack
	switch(inAnimType)
	{
		case ONcAnimType_Kick_Forward:
		case ONcAnimType_Punch_Forward:
		case ONcAnimType_Run_Kick:
		case ONcAnimType_Run_Punch:
			*outDirection = TRcDirection_Forwards;
			return;

		case ONcAnimType_Kick_Left:
		case ONcAnimType_Punch_Left:
		case ONcAnimType_Sidestep_Left_Kick:
		case ONcAnimType_Sidestep_Left_Punch:
			*outDirection = TRcDirection_Left;
			return;
				
		case ONcAnimType_Kick_Right:
		case ONcAnimType_Punch_Right:
		case ONcAnimType_Sidestep_Right_Kick:
		case ONcAnimType_Sidestep_Right_Punch:
			*outDirection = TRcDirection_Right;
			return;

		case ONcAnimType_Kick_Back:
		case ONcAnimType_Punch_Back:
		case ONcAnimType_Run_Back_Punch:
		case ONcAnimType_Run_Back_Kick:
		case ONcAnimType_Getup_Kick_Back:
			*outDirection = TRcDirection_Back;
			return;
	}

	if (iSetAttackDirection_From_State(inFromState, outDirection))
		return;

	for(itr = 0; itr < inNumShortcuts; itr++)
	{
		if (iSetAttackDirection_From_State(inShortcuts[itr].state, outDirection))
			return;
	}

	// if we've gotten to here, then we don't know what direction the attack is in - set it to be
	// forwards by default.
	*outDirection = TRcDirection_Forwards;

	return;
}

static void iCloseAnimationFile(
			BFtFileMapping	*inMappingRef,
			ANMHeader			*inHeader,
			M3tPoint3D			*inPositionArray,
			M3tQuaternion		*inQuatArray,
			IMPtFramePositionInfo *inFramePositionArray,
			TRtFootstepList		*inFootstepList)
{
	if (NULL != inMappingRef)
	{
		BFrFile_UnMap(inMappingRef);
	}
	else
	{
		UUrMemory_Block_Delete(inFootstepList);
		UUrMemory_Block_Delete(inHeader);
		UUrMemory_Block_Delete(inPositionArray);
		UUrMemory_Block_Delete(inQuatArray);
		UUrMemory_Block_Delete(inFramePositionArray);
	}

	return;
}

typedef struct IMPtTempFrameInfo {
	float		min_height;
	float		max_height;
	UUtUns32	attackParts;
} IMPtTempFrameInfo;

void iGetTransformedBBox(M3tGeometry *inGeometry,  M3tMatrix4x3 *inMatrix, M3tBoundingBox_MinMax *outBBox)
{
	UUtUns32 count = inGeometry->pointArray->numPoints;
	UUtUns32 itr;
	M3tPoint3D *points = inGeometry->pointArray->points;
	M3tPoint3D local;

	if (0 == count) {
		outBBox->minPoint.x = 0.f;
		outBBox->minPoint.y = 0.f;
		outBBox->minPoint.z = 0.f;
		outBBox->maxPoint.x = 0.f;
		outBBox->maxPoint.y = 0.f;
		outBBox->maxPoint.z = 0.f;
	}
	else {
		MUrMatrix_MultiplyPoint(points + 0, inMatrix, &local);

		outBBox->minPoint = local;
		outBBox->maxPoint = local;
	}

	for(itr = 0; itr < count; itr++)
	{
		MUrMatrix_MultiplyPoint(points + itr, inMatrix, &local);

		outBBox->minPoint.x = UUmMin(outBBox->minPoint.x, local.x);
		outBBox->minPoint.y = UUmMin(outBBox->minPoint.y, local.y);
		outBBox->minPoint.z = UUmMin(outBBox->minPoint.z, local.z);
		outBBox->maxPoint.x = UUmMax(outBBox->maxPoint.x, local.x);
		outBBox->maxPoint.y = UUmMax(outBBox->maxPoint.y, local.y);
		outBBox->maxPoint.z = UUmMax(outBBox->maxPoint.z, local.z);		
	}

	return;
}


void HandleFootstep(TRtFootstepClass *inTypeClass, TRtFootstepClass *inDetectClass, UUtBool *is_foot_ready, UUtUns32 inFrameItr,
					UUtUns32 in_foot_index, UUtUns16 inTypeCode, M3tMatrix4x3 *inMatricies, M3tPoint3D *offset, TRtFootstepList **ioFootStepList)
{
	TRtFootstepList *footstep_list = *ioFootStepList;
	M3tBoundingBox_MinMax foot_bbox;

	if (inTypeClass->single_footstep_at_start) {
		if ((inFrameItr == 0) && (in_foot_index == ONcLFoot_Index) && (inFrameItr == 0)) {
			// add a single footstep at the start of the animation
			footstep_list = UUrMemory_Block_Realloc(footstep_list, (footstep_list->numFootsteps + 1)* sizeof(TRtFootstep) + sizeof(TRtFootstepList));
			footstep_list->footsteps[footstep_list->numFootsteps].frame = (UUtUns16) inFrameItr;
			footstep_list->footsteps[footstep_list->numFootsteps].type = TRcFootstep_Single;
			footstep_list->numFootsteps++;

			if (IMPgPrintFootsteps) {
				Imp_PrintMessage(IMPcMsg_Important, "    single footstep frame 0"UUmNL);
			}
		}
		goto exit;
	}

	iGetTransformedBBox(IMPgLastBiped->geometries[in_foot_index], inMatricies + in_foot_index, &foot_bbox);

	if (IMPgDebugFootsteps) {
		Imp_PrintMessage(IMPcMsg_Important, "%d (%2.2f, %2.2f, %2.2f) (%2.2f, %2.2f, %2.2f)"UUmNL, inFrameItr, 
			foot_bbox.minPoint.x, foot_bbox.minPoint.y, foot_bbox.minPoint.z,
			foot_bbox.maxPoint.x, foot_bbox.maxPoint.y, foot_bbox.maxPoint.z);
	}

	if (*is_foot_ready) {
		// are we close enough to the ground to make a footstep
		if (foot_bbox.minPoint.z < inDetectClass->footstep_active_threshold) {
			footstep_list = UUrMemory_Block_Realloc(footstep_list, (footstep_list->numFootsteps + 1)* sizeof(TRtFootstep) + sizeof(TRtFootstepList));
			footstep_list->footsteps[footstep_list->numFootsteps].frame = (UUtUns16) inFrameItr;
			footstep_list->footsteps[footstep_list->numFootsteps].type = inTypeCode;
			footstep_list->numFootsteps++;

			if (IMPgPrintFootsteps) {
				Imp_PrintMessage(IMPcMsg_Important, "    %s footstep frame %d"UUmNL,
					(inTypeCode == TRcFootstep_Left) ? "left" : (inTypeCode == TRcFootstep_Right) ? "right" : "unknown", inFrameItr);
			}
			*is_foot_ready = UUcFalse;
		}
	}
	else {
		// did we get high enough to make this foot ready
		if (foot_bbox.minPoint.z > inDetectClass->footstep_clear_threshold) {
			*is_foot_ready = UUcTrue;
		}
	}

exit:
	*ioFootStepList = footstep_list;
	return;
}

// parse frame positions stored in worldMatrixArray, and build the framePositionArray.
static UUtError iBuildFramePositions(UUtUns32 inFlags, TRtAnimType inAnimType, ANMHeader *inHeader, M3tMatrix4x3 *inMatrixArray, M3tPoint3D *inPointArray,
								 IMPtFramePositionInfo **outFramePositionInfo, UUtUns32 inNumAttacks, TRtAttack *inAttacks,
								TRtDirection inDirection, TRtExtentInfo *outExtentInfo, TRtAnimType inFootstepDetectType, TRtFootstepList **outFootstepList)
{
	UUtUns32 frameItr, itr, itr2, attack_num, numAttackFrames, numPoints, part_index, numPointsPerFrame, ring_itr;
	M3tMatrix4x3 *matrixPtr;
	IMPtTempFrameInfo *tempframeinfo, *frameTempPtr;
	M3tPoint3D initial_pt, *pointbuf, *locationptr, *pointptr, center_pt, corner_pt, *minbound, *maxbound, delta;
	M3tGeometry *geometryPtr;
	IMPtFramePositionInfo *frameInfoArray, *framePositionInfoPtr;
	float radius, max_dist, attack_dist, this_dist, this_angle, attack_minheight, attack_maxheight, attack_angle, delta_theta;
	float delta_h, delta_x, delta_y, max_dir_dist[4], dir_dist[4];
	UUtBool found_dist, already_warned = UUcFalse;
	TRtDirection computed_direction;
	TRtExtentRing attack_ring;

	UUmAssertWritePtr(outFramePositionInfo, sizeof(*outFramePositionInfo));
	UUmAssert(inHeader->numFileFrames > 0);
	frameInfoArray = UUrMemory_Block_New(sizeof(IMPtFramePositionInfo) * inHeader->numFileFrames);
	UUmError_ReturnOnNull(frameInfoArray);
	UUmAssert(frameInfoArray != NULL);

	// set up temporary buffers
	pointbuf = (M3tPoint3D *) UUrMemory_Block_NewClear(8 * inHeader->numFileFrames * inHeader->numNodes
																* sizeof(M3tPoint3D));
	UUmError_ReturnOnNull(pointbuf);

	tempframeinfo = (IMPtTempFrameInfo *) UUrMemory_Block_NewClear(inHeader->numFileFrames * sizeof(IMPtTempFrameInfo));
	UUmError_ReturnOnNull(tempframeinfo);

	// so far, no extent information
	attack_ring.max_distance = 0;
	attack_ring.min_height =  1e+09;
	attack_ring.max_height = -1e+09;
	for (itr = 0; itr < TRcExtentRingSamples; itr++) {
		attack_ring.distance[itr] = 0;
	}

	/*
	 * build the temp buffers and set up attack frame info in the framePositionInfo
	 *
	 * FIXME: this could be improved by only calculating points for attacking parts, like it was
	 *        originally. storing all points is no longer required now that the non-attack extent ring is gone.
	 */

	numPoints = 0;
	numPointsPerFrame = 8 * inHeader->numNodes;
	framePositionInfoPtr = frameInfoArray;
	frameTempPtr = tempframeinfo;
	matrixPtr = inMatrixArray;
	numAttackFrames = 0;
	computed_direction = TRcDirection_None;
	locationptr = inPointArray;
	initial_pt = *locationptr;
	pointptr = pointbuf;

	// no attack has an index into the attackextents buffer yet
	for (itr = 0; itr < inNumAttacks; itr++) {
		inAttacks[itr].attackExtentIndex = (UUtUns16) -1;
	}

	// no attack-extent info yet
	max_dist = 0;
	for (itr = 0; itr < 4; itr++)
		max_dir_dist[itr] = 0;

	for (frameItr = 0; frameItr < inHeader->numFileFrames; frameItr++) {
		// is this an attack frame?
		frameTempPtr->attackParts = 0;
		attack_num = (UUtUns32) -1;
		for (itr = 0; itr < inNumAttacks; itr++) {
			if ((frameItr >= inAttacks[itr].firstDamageFrame) && (frameItr <= inAttacks[itr].lastDamageFrame)) {
				// there must be only one attack active at any given frame
				if (frameTempPtr->attackParts != 0) {
					if (!already_warned) {
						Imp_PrintWarning("multiple attacks active at the same frame - not allowed!");
						already_warned = UUcTrue;
					}
					break;
				}

				// set up that an attack is currently active
				frameTempPtr->attackParts = inAttacks[itr].damageBits;
				attack_num = itr;

				if (inAttacks[itr].attackExtentIndex == (UUtUns16) -1) {
					inAttacks[itr].attackExtentIndex = (UUtUns16) numAttackFrames;
				}
			}
		}
		framePositionInfoPtr->is_attack_frame = (frameTempPtr->attackParts > 0);
		framePositionInfoPtr->attack_index = attack_num;

		frameTempPtr->min_height = +1e09;
		frameTempPtr->max_height = -1e09;

		UUmAssert(inHeader->numNodes <= IMPgLastBiped->geometryStorage->numGeometries);

		// loop over all parts of the character and add them to the point buffer
		for (itr = 0; itr < inHeader->numNodes; itr++) {
			// get the bounds of the biped's geometry for this part
			geometryPtr = IMPgLastBiped->geometries[itr];
			MUrMatrix_MultiplyPoint(&geometryPtr->pointArray->boundingSphere.center, &matrixPtr[itr], &center_pt);
			radius = geometryPtr->pointArray->boundingSphere.radius;
			if (center_pt.z + radius > frameTempPtr->max_height)
				frameTempPtr->max_height = center_pt.z + radius;

			if (center_pt.z - radius < frameTempPtr->min_height)
				frameTempPtr->min_height = center_pt.z - radius;

			// store the eight points that make up this bounding box in the temp array
			minbound = &geometryPtr->pointArray->minmax_boundingBox.minPoint;
			maxbound = &geometryPtr->pointArray->minmax_boundingBox.maxPoint;

			MUmVector_Set(corner_pt, minbound->x, minbound->y, minbound->z);
			MUrMatrix_MultiplyPoint(&corner_pt, &matrixPtr[itr], &pointbuf[numPoints++]);

			MUmVector_Set(corner_pt, maxbound->x, minbound->y, minbound->z);
			MUrMatrix_MultiplyPoint(&corner_pt, &matrixPtr[itr], &pointbuf[numPoints++]);

			MUmVector_Set(corner_pt, minbound->x, maxbound->y, minbound->z);
			MUrMatrix_MultiplyPoint(&corner_pt, &matrixPtr[itr], &pointbuf[numPoints++]);

			MUmVector_Set(corner_pt, maxbound->x, maxbound->y, minbound->z);
			MUrMatrix_MultiplyPoint(&corner_pt, &matrixPtr[itr], &pointbuf[numPoints++]);

			MUmVector_Set(corner_pt, minbound->x, minbound->y, maxbound->z);
			MUrMatrix_MultiplyPoint(&corner_pt, &matrixPtr[itr], &pointbuf[numPoints++]);

			MUmVector_Set(corner_pt, maxbound->x, minbound->y, maxbound->z);
			MUrMatrix_MultiplyPoint(&corner_pt, &matrixPtr[itr], &pointbuf[numPoints++]);

			MUmVector_Set(corner_pt, minbound->x, maxbound->y, maxbound->z);
			MUrMatrix_MultiplyPoint(&corner_pt, &matrixPtr[itr], &pointbuf[numPoints++]);

			MUmVector_Set(corner_pt, maxbound->x, maxbound->y, maxbound->z);
			MUrMatrix_MultiplyPoint(&corner_pt, &matrixPtr[itr], &pointbuf[numPoints++]);
		}

		// loop over the point buffer and get the distances of all points for this frame
		// note that at this point, pointptr is the start of the point storage for this frame
		for (itr = 0; itr < numPointsPerFrame; itr++, pointptr++) {
			// skip to the next point if we aren't an attacking point
			part_index = itr / 8;
			if ((frameTempPtr->attackParts & (1 << part_index)) == 0)
				continue;

			// get the point's distance from the animation start
			delta.x = pointptr->x - initial_pt.x;
			delta.y = pointptr->y - initial_pt.y;
			delta.z = pointptr->z;
			this_dist = MUrSqrt(UUmSQR(delta.x) + UUmSQR(delta.y));
			this_angle = MUrATan2(delta.x, -delta.y);
			UUmTrig_Clip(this_angle);
			
			// update our attack's extent ring
			attack_ring.max_distance			= UUmMax(this_dist, attack_ring.max_distance);
			attack_ring.max_height				= UUmMax(delta.z,	attack_ring.max_height);
			attack_ring.min_height				= UUmMin(delta.z,	attack_ring.min_height);
			for (ring_itr = 0; ring_itr < TRcExtentRingSamples; ring_itr++) {
				delta_theta = (ring_itr * M3c2Pi / TRcExtentRingSamples) - this_angle;
				UUmTrig_ClipAbsPi(delta_theta);

				if ((float)fabs(delta_theta) < TRcExtentSampleBracket) {
					attack_ring.distance[ring_itr] = UUmMax(this_dist, attack_ring.distance[ring_itr]);
				}
			}

			// skip to the next point if we don't need to compute a direction for this attack
			if (inDirection != TRcDirection_None)
				continue;

			// get the point's distances forward, left, right and back
			// note that this is NOT yet reversed in Y
			dir_dist[0] = - (pointptr->y - locationptr->y);
			dir_dist[1] =   (pointptr->x - locationptr->x);
			dir_dist[2] = - (pointptr->x - locationptr->x);
			dir_dist[3] =   (pointptr->y - locationptr->y);

			for (itr2 = 0; itr2 < 4; itr2++) {
				if (dir_dist[itr2] > max_dir_dist[itr2]) {
					max_dir_dist[itr2] = dir_dist[itr2];

					if (dir_dist[itr2] > max_dist) {
						max_dist = dir_dist[itr2];
					}
				}
			}
		}

		UUmAssert(pointptr = &pointbuf[numPoints]);

		if (framePositionInfoPtr->is_attack_frame) {
			numAttackFrames++;
		}

		framePositionInfoPtr++;
		frameTempPtr++;
		matrixPtr += inHeader->numNodes;
	}

	/*
	 * compute a direction for the attack if necessary
	 */

	if ((numAttackFrames > 0) && (inDirection == TRcDirection_None)) {
			
		if (max_dist == max_dir_dist[0]) {
			inDirection = computed_direction = TRcDirection_Forwards;

		} else if (max_dist == max_dir_dist[1]) {
			inDirection = computed_direction = TRcDirection_Left;

		} else if (max_dist == max_dir_dist[2]) {
			inDirection = computed_direction = TRcDirection_Right;

		} else if (max_dist == max_dir_dist[3]) {
			inDirection = computed_direction = TRcDirection_Back;

		} else {
			Imp_PrintWarning("error computing the direction of attack!");
			return UUcError_Generic;
		}
	}

	/*
	 * traverse the positioninfo buffer and set up final values
	 */

	// parse each frame in turn
	framePositionInfoPtr = frameInfoArray;
	frameTempPtr = tempframeinfo;
	pointptr = pointbuf;
	locationptr = inPointArray;

	for (frameItr = 0; frameItr < inHeader->numFileFrames; frameItr++) {

		// set up the position for this frame
		// note that the y coordinate will be negated when we place these arrays into the animation
		// note also that this is computed relative to the position at the start of the anim
		delta_h = frameTempPtr->max_height - frameTempPtr->min_height;
		UUmAssert(delta_h > 0);
		framePositionInfoPtr->position.toe_to_head_height	= (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(delta_h				/ TRcPositionGranularity);
		framePositionInfoPtr->position.floor_to_toe_offset	= (UUtInt16) MUrFloat_Round_To_Int(frameTempPtr->min_height			/ TRcPositionGranularity);
		framePositionInfoPtr->position.location_x			= (UUtInt16) MUrFloat_Round_To_Int((locationptr->x - initial_pt.x)	/ TRcPositionGranularity);
		framePositionInfoPtr->position.location_y			= (UUtInt16) MUrFloat_Round_To_Int((locationptr->y - initial_pt.y)	/ TRcPositionGranularity);

		if (framePositionInfoPtr->is_attack_frame) {
			// find the point that is furthest away in the desired direction
			pointptr = &pointbuf[frameItr * numPointsPerFrame];

			attack_minheight = +1e09;
			attack_maxheight = -1e09;
			attack_dist = -1e09;
			found_dist = UUcFalse;
			UUmAssert(frameTempPtr->attackParts > 0);

			for (itr = 0; itr < numPointsPerFrame; itr++, pointptr++) {
				// skip to the next point if we aren't an attacking point
				part_index = itr / 8;
				if ((frameTempPtr->attackParts & (1 << part_index)) == 0)
					continue;

				delta_x = pointptr->x - locationptr->x;
				delta_y = pointptr->y - locationptr->y;

				switch(inDirection) {
					case TRcDirection_360:
						// distance is omni-directional
						this_dist = MUrSqrt(UUmSQR(delta_x) + UUmSQR(delta_y));
					break;

					case TRcDirection_Forwards:
						// attack is oriented along -Y (as this calculation takes place before negation of the Y axis)
						this_dist = - delta_y;
					break;

					case TRcDirection_Left:
						// attack is oriented along +X
						this_dist =   delta_x;
					break;

					case TRcDirection_Right:
						// attack is oriented along -X
						this_dist = - delta_x;
					break;

					case TRcDirection_Back:
						// attack is oriented along +Y (as this calculation takes place before negation of the Y axis)
						this_dist =   delta_y;
					break;

					default:
						UUmAssert(0);
				}

				attack_minheight = UUmMin(attack_minheight, pointptr->z);
				attack_maxheight = UUmMax(attack_maxheight, pointptr->z);

				if (this_dist > attack_dist) {
					attack_dist = this_dist;
					attack_angle = MUrATan2(delta_x, -delta_y);
					found_dist = UUcTrue;
				}

/*				if (this_dist < 0) {
					Imp_PrintMessage(IMPcMsg_Important, "dir %d (%s): frame %d point %d of %d: delta (%f, %f) -> dist %f < 0\n",
							inDirection, (computed_direction == TRcDirection_None) ? "supplied" : "computed",
							frameItr, itr, frameTempPtr->num_attack_points, pointptr->x - locationptr->x,
							pointptr->y - locationptr->y, this_dist);
				}*/
			}

			// store the maximum extent of the attack this frame
			UUmAssert(found_dist);
			attack_dist = UUmMax(attack_dist, 0);
			UUmTrig_ClipLow(attack_angle);

			framePositionInfoPtr->attack_extent.frame_index		= (UUtUns16) frameItr;
			framePositionInfoPtr->attack_extent.attack_angle	= (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round	(attack_angle		/ TRcAngleGranularity);
			framePositionInfoPtr->attack_extent.attack_minheight= (UUtInt16) MUrFloat_Round_To_Int				(attack_minheight	/ TRcPositionGranularity);
			framePositionInfoPtr->attack_extent.attack_maxheight= (UUtInt16) MUrFloat_Round_To_Int				(attack_maxheight	/ TRcPositionGranularity);
			framePositionInfoPtr->attack_extent.attack_distance	= (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round	(attack_dist		/ TRcPositionGranularity);
			framePositionInfoPtr->attack_extent.pad				= 0;
		}
		framePositionInfoPtr++;
		frameTempPtr++;
		locationptr++;
	}

	// deallocate temporary arrays
	UUrMemory_Block_Delete(pointbuf);
	UUrMemory_Block_Delete(tempframeinfo);

	// NB! because the last frame of every animation is not in fact used in the engine,
	// we don't include it in the number of attack extents
	if (frameInfoArray[inHeader->numFileFrames - 1].is_attack_frame) {
		frameInfoArray[inHeader->numFileFrames - 1].is_attack_frame = UUcFalse;
		numAttackFrames -= 1;
	}

	// write output values
	*outFramePositionInfo = frameInfoArray;
	outExtentInfo->numAttackExtents = numAttackFrames;
	outExtentInfo->attack_ring = attack_ring;
	outExtentInfo->computed_attack_direction = computed_direction;


	// build the footsteps 
	
	{
		TRtFootstepClass *footstep_type_class, *footstep_detect_class;
		TRtFootstepList *footstep_list;

		footstep_list = UUrMemory_Block_NewClear(sizeof(TRtFootstepList));
		footstep_type_class = iGetFootstepClass(inAnimType);
		footstep_detect_class = iGetFootstepClass(inFootstepDetectType);

		if ((footstep_detect_class == NULL) && (inNumAttacks > 0)) {
			// attacks get detected as kicks, so that we can look for landings
			footstep_detect_class = iGetFootstepClass(ONcAnimType_Kick);
		}

		if (footstep_type_class == NULL) {
			// if we have been set to detect footsteps and we don't have a type, take the detection type
			footstep_type_class = footstep_detect_class;
		}

		if (NULL != footstep_detect_class) {
			UUtBool is_left_foot_ready;
			UUtBool is_right_foot_ready;
			
			M3tPoint3D *root_location_this_frame = inPointArray + 0;

			{
				M3tBoundingBox_MinMax initial_bbox;
				
				iGetTransformedBBox(IMPgLastBiped->geometries[ONcLFoot_Index], inMatrixArray + ONcLFoot_Index, &initial_bbox);
				is_left_foot_ready = (initial_bbox.minPoint.z) >= footstep_detect_class->foostep_initial_clear_threshold;

				iGetTransformedBBox(IMPgLastBiped->geometries[ONcRFoot_Index], inMatrixArray + ONcRFoot_Index, &initial_bbox);
				is_right_foot_ready = (initial_bbox.minPoint.z) >= footstep_detect_class->foostep_initial_clear_threshold;
			}


			for (frameItr = 0, matrixPtr = inMatrixArray; frameItr < inHeader->numFileFrames; matrixPtr += inHeader->numNodes, frameItr++) 
			{
				HandleFootstep(footstep_type_class, footstep_detect_class, &is_left_foot_ready, frameItr, ONcLFoot_Index, TRcFootstep_Left, matrixPtr, inPointArray + frameItr, &footstep_list);
				HandleFootstep(footstep_type_class, footstep_detect_class, &is_right_foot_ready, frameItr, ONcRFoot_Index, TRcFootstep_Right, matrixPtr, root_location_this_frame, &footstep_list);
			}			
		}

		*outFootstepList = footstep_list;
	}

	
	return UUcError_None;
}




static UUtError iProcessAnimationFile(
			BFtFileRef			*inANMFileRef, 
			UUtUns32			inFlags,
			TRtAnimType			inAnimType,
			TRtAnimType			inFootstepDetectType,
			BFtFileMapping		**outMappingRef,
			ANMHeader			**outHeader,
			M3tPoint3D			**outPositionArray,
			M3tQuaternion		**outQuatArray,
			UUtUns32			inNumAttacks,
			TRtAttack			*inAttacks,
			TRtExtentInfo		*outExtentInfo,
			TRtFootstepList		**outFootsteps,
			TRtDirection		inDirection,
			IMPtFramePositionInfo **outFramePositionArray)
{
	UUtError	error;
	UUtUns32	ANMTimestamp;
	M3tMatrix4x3 *worldMatrixArray;
	BFtFileRef	ANDFileRef;
	UUtBool		build_andfile;
	const char *newSuffix;
	ANMHeader *header;
	TRtFootstepList *local_footsteps = NULL;

	*outMappingRef = NULL;

	if (inFlags & (1 << ONcAnimFlag_ThrowSource)) {
		newSuffix = "ans";
	}
	else if (inFlags & (1 << ONcAnimFlag_ThrowTarget)) {
		newSuffix = "ant";
	}
	else {
		newSuffix = "and";
	}

	ANDFileRef = *inANMFileRef;
	BFrFileRef_SetLeafNameSuffex(&ANDFileRef, newSuffix);

	if (!BFrFileRef_FileExists(inANMFileRef)) {
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "ANM file does not exist");
	}

	error = BFrFileRef_GetModTime(inANMFileRef, &ANMTimestamp);
	UUmError_ReturnOnErrorMsg(error, "failed to get anm file mod time");
	
	if (IMPgForceANDRebuild) {
		// don't even bother looking for the file
		build_andfile = UUcTrue;
	} else {
		error = Imp_ANDFile_Read(&ANDFileRef, ANMTimestamp, outMappingRef, &header, outPositionArray,
								outQuatArray, outFramePositionArray, outExtentInfo, &local_footsteps);
		build_andfile = (error != UUcError_None);
	}

	if (build_andfile) {
		header = (ANMHeader *) UUrMemory_Block_NewClear(sizeof(ANMHeader));

		header->rootTranslation[0] = MUgZeroPoint;
		header->rootTranslation[1] = MUgZeroPoint;

		if (IMPgLastBiped == NULL) {
			Imp_PrintWarning("animations must be preceded by a biped file");
			return UUcError_Generic;
		}

		error = Imp_ANMFile_Read(inANMFileRef, inFlags, header, outPositionArray, outQuatArray, &worldMatrixArray);
		UUmError_ReturnOnErrorMsg(error, "failed to read ANM file");

		// parse frame positions stored in worldMatrixArray, and build the framePositionArray.
		error = iBuildFramePositions(inFlags, inAnimType, header, worldMatrixArray, *outPositionArray, outFramePositionArray,
									inNumAttacks, inAttacks, inDirection, outExtentInfo, inFootstepDetectType, &local_footsteps);
		UUmError_ReturnOnErrorMsg(error, "failed to build framePositionInfo");

		UUrMemory_Block_Delete(worldMatrixArray);

		// save AND data for later use
		error = Imp_ANDFile_Write(&ANDFileRef, ANMTimestamp, header, *outPositionArray, *outQuatArray, *outFramePositionArray, outExtentInfo, local_footsteps);
		UUmError_ReturnOnErrorMsg(error, "failed to write AND file");
	}

	if (NULL == *outFootsteps) {
		*outFootsteps = local_footsteps;
	}
	else if (build_andfile) {
		if (local_footsteps != NULL) {
			UUrMemory_Block_Delete(local_footsteps);
		}
	}

	*outHeader = header;

	return UUcError_None;
}

static UUtError iMakeAnimationOverlay(
			ANMHeader		*outHeader,
			M3tPoint3D		*outPositionArray,
			M3tQuaternion	*outQuatArray,
			UUtUns32		inReplaceParts,
			UUtUns32		*outUsedParts)
{
	UUtUns32 usedParts = 0;
	UUtUns16 frameItr;
	UUtUns16 nodeItr;
	M3tPoint3D frame0Point;
	M3tQuaternion *frame0Quats;

	UUmAssertReadPtr(outHeader, sizeof(*outHeader));
	UUmAssertReadPtr(outPositionArray, sizeof(*outPositionArray));
	UUmAssertReadPtr(outQuatArray, sizeof(*outQuatArray));
	UUmAssertReadPtr(outUsedParts, sizeof(*outUsedParts));

	frame0Point = outPositionArray[0];
	frame0Quats = UUrMemory_Block_New(sizeof(M3tQuaternion) * outHeader->numNodes);
	UUmError_ReturnOnNull(frame0Quats);

	UUrMemory_MoveFast(outQuatArray, frame0Quats, sizeof(M3tQuaternion) * outHeader->numNodes);

	// move the data up to where the old frame 0 data was
	UUrMemory_MoveOverlap(
		outQuatArray + outHeader->numNodes, 
		outQuatArray, 
		sizeof(M3tQuaternion) * outHeader->numNodes * (outHeader->numFileFrames - 1)); 

	UUrMemory_MoveOverlap(
		outPositionArray + 1, 
		outPositionArray, 
		sizeof(M3tPoint3D) * (outHeader->numFileFrames - 1));

	outHeader->numFileFrames -= 1;
	outHeader->duration -= 1;

	for(frameItr = 0; frameItr < outHeader->numFileFrames; frameItr++)
	{
		M3tQuaternion *frameQuats = outQuatArray + ((outHeader->numNodes) * frameItr);

		MUmVector_Subtract(outPositionArray[frameItr], outPositionArray[frameItr], frame0Point);

		for(nodeItr = 0; nodeItr < outHeader->numNodes; nodeItr++)
		{
			if (inReplaceParts & (1 << nodeItr)) {
				// do nothing
			}
			else {
				M3tMatrix4x3 first_rotation_matrix;
				M3tMatrix4x3 rotation_matrix;
				M3tMatrix4x3 rotation_delta_matrix;

				if (!MUrQuat_IsEqual(frame0Quats + nodeItr, frameQuats + nodeItr)) {
					usedParts |= 1 << nodeItr;
					UUmAssert(nodeItr < 32);
				}
				
				// subtract off frame 0, node n
				MUrQuatToMatrix(frame0Quats + nodeItr, &first_rotation_matrix);
				MUrMatrix_Inverse(&first_rotation_matrix, &first_rotation_matrix);
				MUrQuatToMatrix(frameQuats + nodeItr, &rotation_matrix);
				MUrMatrix_Multiply(&first_rotation_matrix, &rotation_matrix, &rotation_delta_matrix);
				MUrMatrixToQuat(&rotation_delta_matrix, frameQuats + nodeItr);
			}
		}
	}
	
	UUrMemory_Block_Delete(frame0Quats);
	
	*outUsedParts = usedParts;

	return UUcError_None;
}

UUtError Imp_GetAnimState(GRtGroup *inGroup, const char *inName, TRtAnimState *outState)
{
	UUtError error;
	char *stateString;
	TRtAnimState state;

	error = GRrGroup_GetString(inGroup, inName, &stateString);
	if (error) { 
		Imp_PrintWarning("invalid field %s", inName); 
		return error;
	}

	error = iStringToAnimState(stateString, &state);
	if (error) { 
		Imp_PrintWarning("'%s' is an invalid state", stateString); 
		return error;
	}

	*outState = state;

	return UUcError_None;
}

UUtError Imp_GetAnimType(GRtGroup *inGroup, const char *inName, TRtAnimType *outType)
{
	UUtError error;
	char *typeString;
	TRtAnimType type;

	// find the type of animation this is and the type that follows
	error = GRrGroup_GetString(inGroup, inName, &typeString);
	if (error) { 
		return error;
	}

	error = IMPrStringToAnimType(typeString, &type);
	if (error) { 
		Imp_PrintWarning("'%s' is an invalid animType", typeString); 
		return error;
	}

	*outType = type;

	return UUcError_None;
}

static UUtError iGetAnimStateQuietly(GRtGroup *inGroup, const char *inName, TRtAnimState *outState)
{
	UUtError error;
	char *stateString;
	TRtAnimState state;

	error = GRrGroup_GetString(inGroup, inName, &stateString);
	if (error) { 
		return UUcError_None;
	}

	error = iStringToAnimState(stateString, &state);
	if (error) { 
		Imp_PrintWarning("'%s' is an invalid state", stateString); 
		return error;
	}

	*outState = state;

	return UUcError_None;
}

static void iZeroShortcuts(UUtUns32 count, TRtShortcut *outShortcuts)
{
	UUtUns32 itr;

	for(itr = 0; itr < count; itr++)
	{
		outShortcuts[itr].flags = 0;
		outShortcuts[itr].length = 0;
		outShortcuts[itr].state = 0;
	}

	return;
}

static UUtBool iGetShortcut(TRtAnimState inFromState, GRtGroup *inGroup, const char *inPrefix, UUtUns32 inNumber, TRtShortcut *outShortcut)
{
	UUtError error;
	char varName[255];
	char *shortcutStateString;
	char *shortcutFlagsString;

	// get the state
	sprintf(varName, "%sState%d", inPrefix, inNumber+1);
	error = GRrGroup_GetString(inGroup, varName, &shortcutStateString);

	if (error != UUcError_None) {
		return UUcFalse;
	}

	// ok, if we did get the state zero everything
	outShortcut->state = TRcAnimState_None;
	outShortcut->length = 0;
	outShortcut->flags = 0;

	error = iStringToAnimState(shortcutStateString, &(outShortcut->state));
	if (error != UUcError_None) {
		outShortcut->state = TRcAnimState_None;
	}

	if (TRcAnimState_None == outShortcut->state) {
		AUrMessageBox(AUcMBType_OK, "ShortcutState %d (%s) is none/invalid!", inNumber, shortcutStateString);
		return UUcFalse;
	}
	
	if (inFromState == outShortcut->state) {
		AUrMessageBox(AUcMBType_OK, "ShortcutState %d (%s) matches fromState!", inNumber, shortcutStateString);
		return UUcFalse;
	}

	// flags
	shortcutFlagsString = "";

	sprintf(varName, "%sFlag%d", inPrefix, inNumber+1);
	error = GRrGroup_GetString(inGroup, varName, &shortcutFlagsString);

	if (error != UUcError_None) {
		sprintf(varName, "%sFlags%d", inPrefix, inNumber+1);
		error = GRrGroup_GetString(inGroup, varName, &shortcutFlagsString);
	}

	if (UUcError_None == error) {
		error = iParamListToShortcutFlags(shortcutFlagsString, &(outShortcut->flags));
	}

	// shortcut length
	sprintf(varName, "%sLength%d", inPrefix, inNumber+1);
	error = GRrGroup_GetUns16(inGroup, varName, &(outShortcut->length));

	Imp_PrintMessage(IMPcMsg_Cosmetic, "shortcut %d state %s length %d"UUmNL, 
		inNumber, 
		iStateToString(outShortcut->state), 
		outShortcut->length);

	if (UUcError_None != error)	{
		AUrMessageBox(AUcMBType_OK, "could not find shortcutCount for state %s", shortcutStateString);
		outShortcut->length = 6;
		return UUcFalse;
	}

	return UUcTrue;
}

static UUtError iGetPartMask(GRtGroup *inGroup, const char *inVarName, UUtUns32 *outMask)
{
	UUtError error;
	char *partMaskString;

	error = GRrGroup_GetString(inGroup, inVarName, &partMaskString);
	if (UUcError_None == error) {
		if ('0' == partMaskString[0]) {
			error = GRrGroup_GetUns32(inGroup, inVarName, outMask);
		}
		else {
			error = iParamListToBodyFlags(partMaskString, outMask);
		}
	}

	return error;
}

typedef struct tBPMbuild
{
	char					*name;
	UUtUns16				part_index;
	
} tBPMbuild;

static UUtError
Imp_BodyPartMaterials(
	GRtGroup				*inGroup,
	ONtBodyPartMaterials	**outBodyPartMaterials)
{
	UUtError				error;
	GRtElementType			element_type;
	GRtGroup				*body_part_group;
	ONtBodyPartMaterials	*body_part_materials;
	tBPMbuild				*part;
	tBPMbuild				body_parts[] =
	{
		{ "Pelvis", ONcPelvis_Index },
		{ "LThigh", ONcLLeg_Index },
		{ "LShin", ONcLLeg1_Index },
		{ "LFoot", ONcLFoot_Index },
		{ "RThigh", ONcRLeg_Index },
		{ "RShin", ONcRLeg1_Index },
		{ "RFoot", ONcRFoot_Index },
		{ "Abdomen", ONcSpine_Index },
		{ "Chest", ONcSpine1_Index },
		{ "Neck", ONcNeck_Index },
		{ "Head", ONcHead_Index },
		{ "LShoulder", ONcLArm_Index },
		{ "LUpperArm", ONcLArm1_Index },
		{ "LForeArm", ONcLArm2_Index },
		{ "LHand", ONcLHand_Index },
		{ "RShoulder", ONcRArm_Index },
		{ "RUpperArm", ONcRArm1_Index },
		{ "RForeArm", ONcRArm2_Index },
		{ "RHand", ONcRHand_Index },
		{ NULL, 0 }
	};
	
	*outBodyPartMaterials = NULL;
	
	// create an instance of a body part material template
	error =
		TMrConstruction_Instance_NewUnique(
			ONcTemplate_BodyPartMaterials,
			0,
			&body_part_materials);
	UUmError_ReturnOnErrorMsg(error, "Unable to create body part materials");
	
	// get a pointer to the body part materials group
	error =
		GRrGroup_GetElement(
			inGroup,
			"bodyPartMaterials",
			&element_type,
			&body_part_group);
	if ((error != UUcError_None) || (element_type != GRcElementType_Group))
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to find bodyPartMaterials");
	}
	
	// get the elements from the group
	for (part = body_parts; part->name != NULL; part++)
	{
		char				*material_name;
		TMtPlaceHolder		material;
		
		// get the material name for the body part
		error = GRrGroup_GetString(body_part_group, part->name, &material_name);
		UUmError_ReturnOnErrorMsg(error, "Unable to get the name of a body part material");
		
		error =
			TMrConstruction_Instance_GetPlaceHolder(
				MAcTemplate_Material,
				material_name,
				&material);
		UUmError_ReturnOnErrorMsg(error, "Unable to create placeholder for material");
		
		body_part_materials->materials[part->part_index] = (MAtMaterial*)material;
	}
	
	*outBodyPartMaterials = body_part_materials;
	
	return UUcError_None;
}

static UUtError
Imp_BodyPartImpacts(
	GRtGroup				*inGroup,
	ONtBodyPartImpacts		**outBodyPartImpacts)
{
	UUtError				error;
	GRtElementType			element_type;
	GRtGroup				*body_part_group;
	ONtBodyPartImpacts		*body_part_impacts;
	tBPMbuild				*part;
	tBPMbuild				body_parts[] =
	{
		{ "Pelvis", ONcPelvis_Index },
		{ "LThigh", ONcLLeg_Index },
		{ "LShin", ONcLLeg1_Index },
		{ "LFoot", ONcLFoot_Index },
		{ "RThigh", ONcRLeg_Index },
		{ "RShin", ONcRLeg1_Index },
		{ "RFoot", ONcRFoot_Index },
		{ "Abdomen", ONcSpine_Index },
		{ "Chest", ONcSpine1_Index },
		{ "Neck", ONcNeck_Index },
		{ "Head", ONcHead_Index },
		{ "LShoulder", ONcLArm_Index },
		{ "LUpperArm", ONcLArm1_Index },
		{ "LForeArm", ONcLArm2_Index },
		{ "LHand", ONcLHand_Index },
		{ "RShoulder", ONcRArm_Index },
		{ "RUpperArm", ONcRArm1_Index },
		{ "RForeArm", ONcRArm2_Index },
		{ "RHand", ONcRHand_Index },
		{ NULL, 0 }
	};
	
	*outBodyPartImpacts = NULL;
	
	// create an instance of a body part impacts template
	error =
		TMrConstruction_Instance_NewUnique(
			ONcTemplate_BodyPartImpacts,
			0,
			&body_part_impacts);
	UUmError_ReturnOnErrorMsg(error, "Unable to create body part impacts");
	
	/*
	 * read the hit impacts
	 */

	// get a pointer to the hit impacts group
	error =
		GRrGroup_GetElement(
			inGroup,
			"hitImpacts",
			&element_type,
			&body_part_group);
	if ((error != UUcError_None) || (element_type != GRcElementType_Group))
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to find hitImpacts");
	}
	
	// get the elements from the group
	for (part = body_parts; part->name != NULL; part++)
	{
		char				*impact_name;
		TMtPlaceHolder		impact;
		
		// get the impact name for the body part
		error = GRrGroup_GetString(body_part_group, part->name, &impact_name);
		UUmError_ReturnOnErrorMsg(error, "Unable to get the name of a body part impact");
		
		error =
			TMrConstruction_Instance_GetPlaceHolder(
				MAcTemplate_Impact,
				impact_name,
				&impact);
		UUmError_ReturnOnErrorMsg(error, "Unable to create placeholder for impact");
		
		body_part_impacts->hit_impacts[part->part_index] = (MAtImpact*)impact;
	}
	
	/*
	 * read the blocked impacts
	 */

	// get a pointer to the body part impacts group
	error =
		GRrGroup_GetElement(
			inGroup,
			"blockedImpacts",
			&element_type,
			&body_part_group);
	if ((error != UUcError_None) || (element_type != GRcElementType_Group))
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to find blockedImpacts");
	}
	
	// get the elements from the group
	for (part = body_parts; part->name != NULL; part++)
	{
		char				*impact_name;
		TMtPlaceHolder		impact;
		
		// get the impact name for the body part
		error = GRrGroup_GetString(body_part_group, part->name, &impact_name);
		UUmError_ReturnOnErrorMsg(error, "Unable to get the name of a body part impact");
		
		error =
			TMrConstruction_Instance_GetPlaceHolder(
				MAcTemplate_Impact,
				impact_name,
				&impact);
		UUmError_ReturnOnErrorMsg(error, "Unable to create placeholder for impact");
		
		body_part_impacts->blocked_impacts[part->part_index] = (MAtImpact*)impact;
	}
	
	/*
	 * read the killed impacts
	 */

	// get a pointer to the killed impacts group
	error =
		GRrGroup_GetElement(
			inGroup,
			"killedImpacts",
			&element_type,
			&body_part_group);
	if ((error != UUcError_None) || (element_type != GRcElementType_Group))
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to find killedImpacts");
	}
	
	// get the elements from the group
	for (part = body_parts; part->name != NULL; part++)
	{
		char				*impact_name;
		TMtPlaceHolder		impact;
		
		// get the impact name for the body part
		error = GRrGroup_GetString(body_part_group, part->name, &impact_name);
		UUmError_ReturnOnErrorMsg(error, "Unable to get the name of a body part impact");
		
		error =
			TMrConstruction_Instance_GetPlaceHolder(
				MAcTemplate_Impact,
				impact_name,
				&impact);
		UUmError_ReturnOnErrorMsg(error, "Unable to create placeholder for impact");
		
		body_part_impacts->killed_impacts[part->part_index] = (MAtImpact*)impact;
	}
	
	*outBodyPartImpacts = body_part_impacts;

	return UUcError_None;
}

static int UUcExternal_Call
iCompareFrameNum(
	const void					*inItem1,
	const void					*inItem2)
{
	const TRtSound				*sound1;
	const TRtSound				*sound2;
	
	sound1 = (TRtSound*)inItem1;
	sound2 = (TRtSound*)inItem2;
		
	return sound1->frameNum - sound2->frameNum;
}

static UUtError
iParseAnimationParticles(
	GRtElementArray *			inArray,
	UUtUns8 *					outNumParticles,
	TRtParticle *				outParticles,
	UUtUns8						inNumAttacks,
	TRtAttack *					inAttacks)
{
	enum {
		cParticleArray_Name,
		cParticleArray_PartName,
		cParticleArray_StartFrame,
		cParticleArray_EndFrame,

		cParticleArray_Length
	};

	UUtError error;
	UUtUns32 itr, itr2;
	UUtBool read_frame_indices;
	GRtElementArray *sub_array;
	GRtElementType element_type;
	void *element;
	TRtParticle *particle;

	// get the number of items in the array
	*outNumParticles = (UUtUns8) GRrGroup_Array_GetLength(inArray);
	if (*outNumParticles > TRcMaxParticles) {
		Imp_PrintWarning("animation particles %d exceeds max %d!", *outNumParticles, TRcMaxParticles);
		*outNumParticles = TRcMaxParticles;
	}

	for (itr = 0, particle = outParticles; itr < *outNumParticles; itr++, particle++) {
		// get the sub-array
		error =	GRrGroup_Array_GetElement(inArray, itr, &element_type, &element);
		IMPmError_ReturnOnError(error);
		if (element_type != GRcElementType_Array) {
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "animation particle array: elements of array must themselves be arrays");
		}
		sub_array = (GRtElementArray *) element;

		read_frame_indices = UUcTrue;

		if (GRrGroup_Array_GetLength(sub_array) == cParticleArray_Length - 2) {
			// the line has everything except the start and end frames. fill them in automatically from the
			// first attack, if one exists
			if (inNumAttacks == 0) {
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "animation particle: no frames specified, and no attack to get frames from either");
			}
			particle->startFrame = inAttacks[0].firstDamageFrame;
			particle->stopFrame = inAttacks[0].lastDamageFrame;
			read_frame_indices = UUcFalse;

		} else if (GRrGroup_Array_GetLength(sub_array) != cParticleArray_Length) {
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "animation particle: array has incorrect number of elements");
		}

		// read the particle tag
		error =	GRrGroup_Array_GetElement(sub_array, cParticleArray_Name, &element_type, &element);
		IMPmError_ReturnOnError(error);
		if (element_type != GRcElementType_String) {
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "animation particle: name must be a string");
		}
		UUrString_Copy(particle->particleName, (const char *) element, ONcCharacterParticleNameLength + 1);

		// read the particle part name
		error =	GRrGroup_Array_GetElement(sub_array, cParticleArray_PartName, &element_type, &element);
		IMPmError_ReturnOnError(error);
		if (element_type != GRcElementType_String) {
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "animation particle: part-name must be a string");
		}

		// convert this to a part index
		for (itr2 = 0; cParticle_PartName[itr2] != NULL; itr2++) {
			if (strcmp(cParticle_PartName[itr2], (const char *) element) == 0)
				break;
		}
		if (cParticle_PartName[itr2] == NULL) {
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "animation particle: unknown part name");
		}
		particle->partIndex = (UUtUns16) itr2;

		if (read_frame_indices) {
			// read the particle start frame
			error =	GRrGroup_Array_GetElement(sub_array, cParticleArray_StartFrame, &element_type, &element);
			IMPmError_ReturnOnError(error);
			if (element_type != GRcElementType_String) {
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "animation particle: start frame must be a string");
			}
			if (sscanf((char *) element, "%hd", &particle->startFrame) == 0) {
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "animation particle: start frame must be a number");
			}
		
			// read the particle start frame
			error =	GRrGroup_Array_GetElement(sub_array, cParticleArray_EndFrame, &element_type, &element);
			IMPmError_ReturnOnError(error);
			if (element_type != GRcElementType_String) {
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "animation particle: stop frame must be a string");
			}
			if (sscanf((char *) element, "%hd", &particle->stopFrame) == 0) {
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "animation particle: stop frame must be a number");
			}
		}

		particle->pad = 0;
	}
	
	return UUcError_None;
}

UUtError
Imp_AddAnimation(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError				error;

	UUtUns16				itr;
	GRtElementType			elemType;
	void *					element;

	// parsed header
	ANMHeader	*header;

	// _ins infor
	char *					animTypeStr;
	char *					reverseName = NULL;

	TRtShortcut				shortcuts[TRcMaxShortcuts];
	TRtShortcut				reverseShortcuts[TRcMaxShortcuts];

	UUtUns16				soundFrames[TRcMaxSounds];
	TMtPlaceHolder			soundPlaceHolders[TRcMaxSounds];

	char *					directAnimationNames[TRcMaxDirectAnimations];
	TMtPlaceHolder			directAnimationPlaceHolder[TRcMaxDirectAnimations];

	UUtUns8					numAttacks;
	UUtUns8					numBlurs;
	UUtUns8					numTakeDamages;
	UUtUns8					numParticles;

	TRtAttack				attacks[TRcMaxAttacks];
	TRtBlur					blurs[TRcMaxBlurs];
	TRtTakeDamage			takeDamages[TRcMaxTakeDamages];
	TRtParticle				particles[TRcMaxParticles];

	TRtAnimType				animType;
	TRtAnimType				footstepDetectType;
	TRtAnimType				aimingScreenAnimType;

	TRtAnimType				reverseAnimType;

	TRtAnimState			fromState;
	TRtAnimState			toState;

	TRtAnimState			revFromState;
	TRtAnimState			revToState;

	char					*flagStr;
	UUtUns32				flags;

	UUtUns8					compressionSize;

	UUtBool					turnOveride;
	float					turnOverideValue;

	char					*varientStr;
	TRtAnimVarient			varient;

	UUtUns16				fps;

	// the template we are making
	TRtAnimation			*animation;
	TRtAnimation			*reverseAnimation = NULL;

	UUtUns16				atomicStart;
	UUtUns16				atomicEnd;

	BFtFileRef*				ANMFileRef;
	BFtFileMapping*			ANMMappingRef;
	UUtBool					buildInstance;

	UUtUns16				endInterpolation;
	UUtUns16				maxInterpolation;
	UUtUns16				softPause;
	UUtUns16				hardPause;

	UUtUns32				usedParts;
	UUtUns32				replaceParts;

	TRtExtentInfo			extentInfo;
	TRtFootstepList			*automaticFootstepList = NULL;
	TRtFootstepList			*forcedFootstepList = NULL;
	TRtFootstepList			*forcedReverseFootstepList = NULL;

	UUtInt16				actionFrame;
	UUtUns16				firstLevel;

	UUtBool					useFootstepsReversed;
	TRtFootstepList			*useFootsteps;

	float finalRotation;

	UUtUns8 numShortcuts = 0;
	UUtUns8 numReverseShortcuts = 0;

	TRtThrowInfo throwInfo;
	UUtBool hasThrowInfo = UUcFalse;

	TRtDirection fwd_direction, reverseDirection;

	char					*vocalizationStr;
	UUtUns16				vocalizationType;

	UUtUns8 invulnerableStart = 0;
	UUtUns8 invulnerableLength = 0;

	// sounds
	TRtSound				*new_sounds = NULL;
	UUtUns32				num_new_sounds = 0;
	
	// these are storage before we build the arrays we will actually use
	M3tPoint3D		*positionArray = NULL;
	M3tQuaternion	*quatArray = NULL;
	IMPtFramePositionInfo *framePositionArray = NULL;
	char *attack_impact_name;
	char varName[255];
	char suffix[80];	// 80 is to capture "1"

	{
		char *badName;
		const char *validNames[] = 
		{
			"template",
			"instance",
			"file",

			"animType",
			"fromState",
			"toState",
			"varient",
			"varients",
			"flag",
			"flags",
			"finalRotation",
			"turnAmt",
			"fps",
			"sound",
			"sound1",
			"sound2",
			"sound3",
			"soundFrame",
			"soundFrame1",
			"soundFrame2",
			"soundFrame3",
			"new_sound",
			"new_sound1",
			"new_sound2",
			"new_sound3",
			"new_sound4",
			"new_sound5",
			"new_sound6",
			"new_sound7",
			"new_sound8",
			"new_sound9",
			"new_sound10",
			"new_soundFrame",
			"new_soundFrame1",
			"new_soundFrame2",
			"new_soundFrame3",
			"new_soundFrame4",
			"new_soundFrame5",
			"new_soundFrame6",
			"new_soundFrame7",
			"new_soundFrame8",
			"new_soundFrame9",
			"new_soundFrame10",
			"direct1",
			"direct2",
			"maxInterpolation",
			"endInterpolation",

			"shortcutState1",
			"shortcutFlag1",
			"shortcutFlags1",
			"shortcutLength1",

			"shortcutState2",
			"shortcutFlag2",
			"shortcutFlags2",
			"shortcutLength2",

			"shortcutState3",
			"shortcutFlag3",
			"shortcutFlags3",
			"shortcutLength3",

			"shortcutState4",
			"shortcutFlag4",
			"shortcutFlags4",
			"shortcutLength4",

			"shortcutState5",
			"shortcutFlag5",
			"shortcutFlags5",
			"shortcutLength5",

			"shortcutState6",
			"shortcutFlag6",
			"shortcutFlags6",
			"shortcutLength6",

			"shortcutState7",
			"shortcutFlag7",
			"shortcutFlags7",
			"shortcutLength7",

			"shortcutState8",
			"shortcutFlag8",
			"shortcutFlags8",
			"shortcutLength8",

			"reverseShortcutState1",
			"reverseShortcutFlag1",
			"reverseShortcutFlags1",
			"reverseShortcutLength1",

			"reverseShortcutState2",
			"reverseShortcutFlag2",
			"reverseShortcutFlags2",
			"reverseShortcutLength2",

			"reverseShortcutState3",
			"reverseShortcutFlag3",
			"reverseShortcutFlags3",
			"reverseShortcutLength3",

			"reverseShortcutState4",
			"reverseShortcutFlag4",
			"reverseShortcutFlags4",
			"reverseShortcutLength4",

			"reverseShortcutState5",
			"reverseShortcutFlag5",
			"reverseShortcutFlags5",
			"reverseShortcutLength5",

			"reverseShortcutState6",
			"reverseShortcutFlag6",
			"reverseShortcutFlags6",
			"reverseShortcutLength6",

			"reverseShortcutState7",
			"reverseShortcutFlag7",
			"reverseShortcutFlags7",
			"reverseShortcutLength7",

			"reverseShortcutState8",
			"reverseShortcutFlag8",
			"reverseShortcutFlags8",
			"reverseShortcutLength8",
			
			"damage",
			"damageBits",
			"firstDamageFrame",
			"lastDamageFrame",
			"hitSound",
			"damageAnimation",
			"attackFlags",

			"damage1",
			"damageBits1",
			"firstDamageFrame1",
			"lastDamageFrame1",
			"hitSound1",
			"damageAnimation1",
			"attacksFlags1",

			"damage2",
			"damageBits2",
			"firstDamageFrame2",
			"lastDamageFrame2",
			"hitSound2",
			"damageAnimation2",
			"attackFlags2",

			"blurParts1",
			"firstBlurFrame1",
			"lastBlurFrame1",
			"blurQuantity1",
			"blurAmount1",
			"blurSkip1",

			"blurParts2",
			"firstBlurFrame2",
			"lastBlurFrame2",
			"blurQuantity2",
			"blurAmount2",
			"blurSkip2",

			"blockStun",
			"blockStun1",
			"blockStun2",
			"hitStun",
			"hitStun1",
			"hitStun2",
			"stagger",
			"stagger1",
			"stagger2",

			"softPause", 	
			"hardPause", 
			"pause",
			"compression", 
			"reverseInstance",  
			"reverseAnimType",
			"reverseFromState",
			"reverseToState",

			"atomicStart",
			"atomicEnd",

			"throwType",
			"relativeThrowFacing",

			"takeDamageFrame1",
			"takeDamageFrame2",
			"takeDamageFrame3",

			"takeDamage1",
			"takeDamage2",
			"takeDamage3",

			"throwDistance",
			"swapWeaponFrame",
			"actionFrame",

			"aimingScreenType",
			"replaceParts",

			"particles",
			"attackImpactName",

			"firstLevel",
			"footsteps",
			"reverseFootsteps",
			"footstepDetectType",

			"invulnerableStart",
			"invulnerableEnd",

			"vocalization",

			NULL
		};

		error = Imp_Common_Scan_Group(inGroup, validNames, (const char **)(&badName));
		UUmError_ReturnOnErrorMsgP(error, "Invalid group string %s", (UUtUns32) badName, 0, 0);
	}

	error = 
		Imp_Common_BuildInstance(
			inSourceFile,
			inSourceFileModDate,
			inGroup,
			TRcTemplate_Animation,
			inInstanceName,
			UUmCompileTimeString,
			&ANMFileRef,
			&buildInstance);
	IMPmError_ReturnOnError(error);

	if(buildInstance != UUcTrue)
	{
		BFrFileRef_Dispose(ANMFileRef);
		return UUcError_None;
	}

	if (UUmString_IsEqual(inInstanceName, "KONOKOcrouch_turn_lt")) {
		Imp_PrintMessage(IMPcMsg_Important, "at turn anim");
	}

	{
		char *footstep_string;
		
		error = GRrGroup_GetString(inGroup, "footsteps", &footstep_string);
	
		
		if (UUcError_None == error) {
			char *cur_footstep_string;
			char *strtok_internal;

			// ex string L20.R50.L52

			forcedFootstepList = UUrMemory_Block_NewClear(sizeof(TRtFootstepList));

			cur_footstep_string = UUrString_Tokenize1(footstep_string, ".", &strtok_internal); 

			while(cur_footstep_string != NULL)
			{
				TRtFootstepKind footstep_kind = TRcFootstep_None;
				UUtInt32 frame_number = 0;

				switch(*cur_footstep_string)
				{
					case 'l':
					case 'L':
						footstep_kind = TRcFootstep_Left;
						cur_footstep_string++;
					break;

					case 'r':
					case 'R':
						footstep_kind = TRcFootstep_Right;
						cur_footstep_string++;
					break;

					case 's':
					case 'S':
						footstep_kind = TRcFootstep_Single;
						cur_footstep_string++;
					break;

					default:
						Imp_PrintWarning("failed to parse footstep string");
				}

				error = UUrString_To_Int32(cur_footstep_string, &frame_number);
				if (error) {
					Imp_PrintWarning("failed to parse footstep string");
				}

				// add a footstep

				forcedFootstepList = UUrMemory_Block_Realloc(forcedFootstepList, sizeof(TRtFootstepList) + sizeof(TRtFootstep) * (forcedFootstepList->numFootsteps + 1));
				forcedFootstepList->footsteps[forcedFootstepList->numFootsteps].frame = (UUtUns16) frame_number;
				forcedFootstepList->footsteps[forcedFootstepList->numFootsteps].type = footstep_kind;
				forcedFootstepList->numFootsteps++;


				cur_footstep_string = UUrString_Tokenize1(NULL, ".", &strtok_internal);
			}
		}
	}
	
	{
		char *footstep_string;
		
		error = GRrGroup_GetString(inGroup, "reverseFootsteps", &footstep_string);
	
		
		if (UUcError_None == error) {
			char *cur_footstep_string;
			char *strtok_internal;

			// ex string L20.R50.L52

			forcedReverseFootstepList = UUrMemory_Block_NewClear(sizeof(TRtFootstepList));

			cur_footstep_string = UUrString_Tokenize1(footstep_string, ".", &strtok_internal); 

			while(cur_footstep_string != NULL)
			{
				TRtFootstepKind footstep_kind = TRcFootstep_None;
				UUtInt32 frame_number = 0;

				switch(*cur_footstep_string)
				{
					case 'l':
					case 'L':
						footstep_kind = TRcFootstep_Left;
						cur_footstep_string++;
					break;

					case 'r':
					case 'R':
						footstep_kind = TRcFootstep_Right;
						cur_footstep_string++;
					break;

					case 's':
					case 'S':
						footstep_kind = TRcFootstep_Single;
						cur_footstep_string++;
					break;

					default:
						Imp_PrintWarning("failed to parse footstep string");
				}

				error = UUrString_To_Int32(cur_footstep_string, &frame_number);
				if (error) {
					Imp_PrintWarning("failed to parse footstep string");
				}

				// add a footstep

				forcedReverseFootstepList = UUrMemory_Block_Realloc(forcedReverseFootstepList, sizeof(TRtFootstepList) +
					sizeof(TRtFootstep) * (forcedReverseFootstepList->numFootsteps + 1));
				forcedReverseFootstepList->footsteps[forcedReverseFootstepList->numFootsteps].frame = (UUtUns16) frame_number;
				forcedReverseFootstepList->footsteps[forcedReverseFootstepList->numFootsteps].type = footstep_kind;
				forcedReverseFootstepList->numFootsteps++;


				cur_footstep_string = UUrString_Tokenize1(NULL, ".", &strtok_internal);
			}
		}
	}

	{
		UUtUns32 invulnerable_start;
		UUtUns32 invulnerable_end;

		error = GRrGroup_GetUns32(inGroup, "invulnerableStart", &invulnerable_start);

		if (UUcError_None == error) {
			error = GRrGroup_GetUns32(inGroup, "invulnerableEnd", &invulnerable_end);
		}

		if (UUcError_None == error) {
			if (invulnerable_start < 255) {
				invulnerableStart = (UUtUns8) invulnerable_start;
			}
			else {
				Imp_PrintWarning("invulnerable start out of bounds");
			}

			if (invulnerable_end >= invulnerable_start) {
				UUtUns32 invulnerable_length = 1 + invulnerable_end - invulnerable_start;

				if (invulnerable_length < 255) {
					invulnerableLength = (UUtUns8) invulnerable_length;
				}
				else {
					Imp_PrintWarning("invulnerable length to long");
				}
			}
			else {
					Imp_PrintWarning("invulnerable length negative");
			}
		}
	}

//	if (invulnerableLength > 0) {
//		Imp_PrintMessage(IMPcMsg_Important, "invulnerable %d %d"UUmNL, invulnerableStart, invulnerableLength);
//	}

	vocalizationType = (UUtUns16) -1;
	error = GRrGroup_GetString(inGroup, "vocalization", &vocalizationStr);
	if (error == UUcError_None) {
		for (itr = 0; itr < AI2cVocalization_Max; itr++) {
			if (UUrString_Compare_NoCase(vocalizationStr, AI2cVocalizationTypeName[itr]) == 0) {
				break;
			}
		}

		if (itr >= AI2cVocalization_Max) {
			Imp_PrintWarning("%s: unknown vocalization type %s", inInstanceName, vocalizationStr);
		} else {
			vocalizationType = itr;
		}
	}

	error = iGetPartMask(inGroup, "replaceParts", &replaceParts);
	if (error != UUcError_None) {
		replaceParts = 0;
	}

	error = Imp_GetAnimType(inGroup, "animType", &animType);
	UUmError_ReturnOnError(error);

	error = Imp_GetAnimType(inGroup, "footstepDetectType", &footstepDetectType);
	if (error != UUcError_None) {
		footstepDetectType = animType;
	}

	error = Imp_GetAnimType(inGroup, "aimingScreenType", &aimingScreenAnimType);
	if (UUcError_None != error) {
		aimingScreenAnimType = animType;
	}

	error = Imp_GetAnimState(inGroup, "fromState", &fromState);
	UUmError_ReturnOnError(error);

	error = Imp_GetAnimState(inGroup, "toState", &toState);
	UUmError_ReturnOnError(error);

	error = GRrGroup_GetFloat(inGroup, "finalRotation", &finalRotation);
	if (error) { finalRotation = 0; }
	else { 
		if (3.14f == finalRotation) { 
			Imp_PrintWarning("finalRotation should be in degrees"); 
		}

		finalRotation *= M3c2Pi / 360.f; 
	}

	error = GRrGroup_GetString(inGroup, "varients", &varientStr);
	if (error) { 
		error = GRrGroup_GetString(inGroup, "varient", &varientStr);
		if (error) {
			Imp_PrintWarning("no varients field"); 
			return error; 
		}
	}

	error = iParamListToVarients(varientStr, &varient);
	UUmError_ReturnOnErrorMsg(error, "invalid varients");

	error = GRrGroup_GetString(inGroup, "flags", &flagStr);
	if (error) {
		error = GRrGroup_GetString(inGroup, "flag", &flagStr);
		if (error) {
			Imp_PrintWarning("no flags field");
			return error;
		}
	}

	error = iParamListToAnimFlags(flagStr, &flags);
	UUmError_ReturnOnErrorMsg(error, "invalid flags");

	// atomic
	error = GRrGroup_GetUns16(inGroup, "atomicEnd", &atomicEnd);
	if (UUcError_None == error) {
		error = GRrGroup_GetUns16(inGroup, "atomicStart", &atomicStart);

		if (UUcError_None != error) {
			atomicStart = 0;
		}
	}
	else if (flags & (1 << ONcAnimFlag_Atomic)) {
		atomicStart = 0;
		atomicEnd = 0xffff;
	}
	else {
		atomicEnd = 0;
		atomicStart = 1;
	}

	error = GRrGroup_GetFloat(inGroup, "turnAmt", &turnOverideValue);
	turnOveride = (UUcError_None == error);

	error = GRrGroup_GetUns16(inGroup, "fps", &fps);
	if (UUcError_None != error) {
		fps = cDefaultFPS;
	}

	error = GRrGroup_GetUns16(inGroup, "endInterpolation", &endInterpolation);
	if (UUcError_None != error) {
		endInterpolation = 0;
	}

	error = GRrGroup_GetUns16(inGroup, "maxInterpolation", &maxInterpolation);
	if (UUcError_None != error) {
		maxInterpolation = UUcMaxUns16;
	}

	//// sounds
	{
		UUtMemory_Array					*sound_array;
		TRtSound						*sound_array_list;
		
		sound_array = UUrMemory_Array_New(sizeof(TRtSound), 1, 0, 10);
		UUmError_ReturnOnNull(sound_array);
		for (itr = 0; ; itr++)
		{
			char						*sound_name;
			UUtUns16					frame_num;
			UUtUns32					index;
			
			sprintf(suffix, "%d", itr + 1);
			sprintf(varName, "new_sound%s", suffix);
			error = GRrGroup_GetString(inGroup, varName, &sound_name);
			
			// for itr 0 try sound as awell as sound1
			if ((UUcError_None != error) && (0 == itr)) {
				sprintf(suffix, "");
				sprintf(varName, "new_sound%s", suffix);
				error = GRrGroup_GetString(inGroup, varName, &sound_name);
			}
			if (UUcError_None != error) { break; }
			
			sprintf(varName, "new_soundFrame%s", suffix);
			error = GRrGroup_GetUns16(inGroup, varName, &frame_num);
			UUmError_ReturnOnErrorMsg(error, "invalid soundFrame");
			
			// add the new_sound to the sound_array
			error = UUrMemory_Array_GetNewElement(sound_array, &index, NULL);
			UUmError_ReturnOnErrorMsg(error, "unable to add new sound to sound array");
			
			sound_array_list = (TRtSound*)UUrMemory_Array_GetMemory(sound_array);
			
			UUrString_Copy(sound_array_list[index].impulseName, sound_name, SScMaxNameLength);
			sound_array_list[index].frameNum = frame_num;
			
			// byte swap the frame number
			UUmSwapLittle_2Byte(&sound_array_list[index].frameNum);
		}
		
		// add the sounds to the raw data
		new_sounds = NULL;
		num_new_sounds = UUrMemory_Array_GetUsedElems(sound_array);
		if (num_new_sounds > 0)
		{
			sound_array_list = (TRtSound*)UUrMemory_Array_GetMemory(sound_array);
			
			// sort by frame number
			qsort(sound_array_list, num_new_sounds, sizeof(TRtSound), iCompareFrameNum);
			
			// create the raw data
			new_sounds = TMrConstruction_Raw_New((sizeof(TRtSound) * num_new_sounds), 4, TRcTemplate_Animation);
			UUmError_ReturnOnNull(new_sounds);
			
			// copy the sound_array_list into the raw data
			UUrMemory_MoveFast(sound_array_list, new_sounds, (sizeof(TRtSound) * num_new_sounds));
			
			// write the raw data
			new_sounds = TMrConstruction_Raw_Write(new_sounds);
		}
		
		// cleanup
		UUrMemory_Array_Delete(sound_array);
		sound_array = NULL;
	}
	
	//// direct animations
	for(itr = 0; itr < TRcMaxDirectAnimations; itr++)
	{
		directAnimationNames[itr] = NULL;
		directAnimationPlaceHolder[itr] = 0;

		sprintf(suffix, "%d", itr + 1);
		sprintf(varName, "direct%s", suffix);
		error = GRrGroup_GetString(inGroup, varName, &(directAnimationNames[itr]));

		// for itr 0 try sound as awell as sound1
		if ((UUcError_None != error) && (0 == itr)) {
			sprintf(suffix, "");
			sprintf(varName, "direct%s", suffix);
			error = GRrGroup_GetString(inGroup, varName, &(directAnimationNames[itr]));
		}

		if (UUcError_None == error) {
			error = TMrConstruction_Instance_GetPlaceHolder(
				TRcTemplate_Animation, 
				directAnimationNames[itr], 
				&(directAnimationPlaceHolder[itr]));
			UUmError_ReturnOnErrorMsg(error, "Imp_AddAnimation: could not make an animation placeholder");	
		}
	}

	//// shortcuts
	iZeroShortcuts(TRcMaxShortcuts, shortcuts);
	iZeroShortcuts(TRcMaxShortcuts, reverseShortcuts);

	for(itr = 0; itr < TRcMaxShortcuts; itr++)
	{
		UUtBool got_shortcut;

		got_shortcut = iGetShortcut(fromState, inGroup, "shortcut", itr, shortcuts + numShortcuts);

		if (got_shortcut) {
			numShortcuts++;
		}
	}

	for(itr = 0; itr < TRcMaxShortcuts; itr++)
	{
		UUtBool got_reverse_shortcut;

		got_reverse_shortcut = iGetShortcut(fromState, inGroup, "reverseShortcut", itr, reverseShortcuts + numReverseShortcuts);

		if (got_reverse_shortcut) {
			numReverseShortcuts++;
		}
	}

	numAttacks = 0;

	for(itr = 0; itr < TRcMaxAttacks; itr++)
	{
		char *attackFlagString;
		float knockbackMultiple;
//		char *hitSoundName;
//		TMtPlaceHolder hitSoundPlaceHolder;

		// zero this attack
		attacks[numAttacks].damageBits = 0;
		attacks[numAttacks].knockback = 0.f;
		attacks[numAttacks].flags = 0;
		attacks[numAttacks].damage = 0;
		attacks[numAttacks].firstDamageFrame = 0;
		attacks[numAttacks].lastDamageFrame = 0;
		attacks[numAttacks].damageAnimation = 0;
//		attacks[numAttacks].hitSound = NULL;
		attacks[numAttacks].hitStun = 0;
		attacks[numAttacks].blockStun = 0;
		attacks[numAttacks].stagger = 0;


		sprintf(suffix, "%d", itr + 1);
		sprintf(varName, "damage%s", suffix);
		error = GRrGroup_GetUns16(inGroup, varName, &(attacks[numAttacks].damage));

		// 'damage1' and 'damage' are both valid
		if ((error != UUcError_None) && (0 == itr)) {
			sprintf(suffix, "");
			sprintf(varName, "damage%s", suffix);
			error = GRrGroup_GetUns16(inGroup, varName, &(attacks[numAttacks].damage));
		}

		// if we couldn't read damage, this is not an attack
		if (error != UUcError_None) { continue; }

		if (0 == flags && (1 << ONcAnimFlag_Attack)) {
			Imp_PrintWarning("Please flag this animation as an attack.");
			flags |= (1 << ONcAnimFlag_Attack);
		}

		sprintf(varName, "attackFlags%s", suffix);
		error = GRrGroup_GetString(inGroup, varName, &attackFlagString);
		if (UUcError_None == error) {
			iParamListToAttackFlags(attackFlagString, &(attacks[numAttacks].flags));
		}

		sprintf(varName, "knockbackMultiple%s", suffix);
		error = GRrGroup_GetFloat(inGroup, varName, &knockbackMultiple);
		if (error != UUcError_None) {
			knockbackMultiple = 0.4f;
		}
		attacks[numAttacks].knockback = attacks[numAttacks].damage * knockbackMultiple;

		sprintf(varName, "damageBits%s", suffix);
		error = iGetPartMask(inGroup, varName, &(attacks[numAttacks].damageBits));
		
		if (error != UUcError_None) {
			Imp_PrintWarning("could not find valid %s", varName);
			return error;
		}

		sprintf(varName, "firstDamageFrame%s", suffix);
		error = GRrGroup_GetUns16(inGroup, varName, &(attacks[numAttacks].firstDamageFrame));
		if (error != UUcError_None) {
			Imp_PrintWarning("could not find valid %s", varName);
			return error;
		}

		sprintf(varName, "lastDamageFrame%s", suffix);
		error = GRrGroup_GetUns16(inGroup, varName, &(attacks[numAttacks].lastDamageFrame));
		if (error != UUcError_None) {
			Imp_PrintWarning("could not find valid %s", varName);
			return error;
		}

/*#if 0
		sprintf(varName, "hitSound%s", suffix);
		error = GRrGroup_GetString(inGroup, varName, &(hitSoundName));
		if (error != UUcError_None) {
			Imp_PrintWarning("could not find valid %s", varName);
			return error;
		}
		else {
			error = TMrConstruction_Instance_GetPlaceHolder(
				SScTemplate_Sound, 
				hitSoundName, 
				&hitSoundPlaceHolder);
			UUmError_ReturnOnErrorMsg(error, "Imp_AddAnimation: could not make the hit sound placeholder");

			attacks[numAttacks].hitSound = (SStSound *) hitSoundPlaceHolder;
		}
#else
		attacks[numAttacks].hitSound = NULL;
#endif*/

		sprintf(varName, "damageAnimation%s", suffix);
		error = Imp_GetAnimType(inGroup, varName, &(attacks[numAttacks].damageAnimation));
		UUmError_ReturnOnError(error);

		sprintf(varName, "hitStun%s", suffix);
		error = GRrGroup_GetUns16(inGroup, varName, &(attacks[numAttacks].hitStun));
		if ((error) && (0 == itr)) {
			error = GRrGroup_GetUns16(inGroup, "hitStun", &(attacks[numAttacks].hitStun));
		}
		if (error) {
			attacks[numAttacks].hitStun = (UUtUns16) (attacks[numAttacks].damage * HIT_STUN_PER_DAMAGE);
		}

		sprintf(varName, "blockStun%s", suffix);
		error = GRrGroup_GetUns16(inGroup, varName, &(attacks[numAttacks].blockStun));
		if ((error) && (0 == itr)) {
			error = GRrGroup_GetUns16(inGroup, "blockStun", &(attacks[numAttacks].blockStun));
		}
		if (error) {
			attacks[numAttacks].blockStun = (UUtUns16) (attacks[numAttacks].damage * BLOCK_STUN_PER_DAMAGE);
			attacks[numAttacks].blockStun = UUmMax(attacks[numAttacks].blockStun, BLOCK_STUN_DEFAULT_MIN);
		}

		sprintf(varName, "stagger%s", suffix);
		error = GRrGroup_GetUns16(inGroup, varName, &(attacks[numAttacks].stagger));
		if ((error) && (0 == itr)) {
			error = GRrGroup_GetUns16(inGroup, "stagger", &(attacks[numAttacks].stagger));
		}
		if (error) {
			attacks[numAttacks].stagger = 0;
		}

		numAttacks++;
	}


	if (flags & (1 << ONcAnimFlag_DoAim)) {
		// do nothing
	}
	else if (flags & (1 << ONcAnimFlag_DontAim)) {
		// do nothing
	}
	else if (flags & ((1 << ONcAnimFlag_RealWorld) | (1 << ONcAnimFlag_Attack) | (1 << ONcAnimFlag_ThrowSource) | (1 << ONcAnimFlag_ThrowTarget))) {
		flags |= 1 << ONcAnimFlag_DontAim;
	}
	else {
		flags |= 1 << ONcAnimFlag_DoAim;
	}

	// motion blur information
	numBlurs = 0;

	for(itr = 0; itr < TRcMaxBlurs; itr++)
	{
		float floatBlurAmount;

		// clear the blur 
		blurs[numBlurs].blurParts = 0;
		blurs[numBlurs].firstBlurFrame = 1;
		blurs[numBlurs].lastBlurFrame = 0;
		blurs[numBlurs].blurQuantity = 0;
		blurs[numBlurs].blurAmount = 0;
		blurs[numBlurs].blurSkip = 1;

		// scan for a blur
		sprintf(suffix, "%d", itr + 1);
		sprintf(varName, "blurParts%s", suffix);
		error = GRrGroup_GetUns32(inGroup, varName, &(blurs[numBlurs].blurParts));
		if (error) { continue; }

		sprintf(varName, "firstBlurFrame%s", suffix);
		error = GRrGroup_GetUns16(inGroup, varName, &(blurs[numBlurs].firstBlurFrame));
		UUmError_ReturnOnErrorMsgP(error, "could not find valid %s", (UUtUns32) varName, 0, 0);

		sprintf(varName, "lastBlurFrame%s", suffix);
		error = GRrGroup_GetUns16(inGroup, varName, &(blurs[numBlurs].lastBlurFrame));
		UUmError_ReturnOnErrorMsgP(error, "could not find valid %s", (UUtUns32) varName, 0, 0);

		sprintf(varName, "blurQuantity%s", suffix);
		error = GRrGroup_GetUns8(inGroup, varName, &(blurs[numBlurs].blurQuantity));
		UUmError_ReturnOnErrorMsgP(error, "could not find valid %s", (UUtUns32) varName, 0, 0);

		sprintf(varName, "blurAmount%s", suffix);
		error = GRrGroup_GetFloat(inGroup, varName, &floatBlurAmount);
		UUmError_ReturnOnErrorMsgP(error, "could not find valid %s", (UUtUns32) varName, 0, 0);

		sprintf(varName, "blurSkip%s", suffix);
		error = GRrGroup_GetUns8(inGroup, varName, &(blurs[numBlurs].blurSkip));
		if (error != UUcError_None) {
			blurs[numBlurs].blurSkip = 1;
		}

		floatBlurAmount = UUmPin(floatBlurAmount, 0.f, 1.f);		
		blurs[numBlurs].blurAmount = (UUtUns8) (floatBlurAmount * 255.f);

		numBlurs++;
	}

	error = GRrGroup_GetUns16(inGroup, "softPause", &softPause);
	if (UUcError_None != error) {
		error = GRrGroup_GetUns16(inGroup, "pause", &softPause);
	}

	if (UUcError_None != error) {
		softPause = 0;
	}

	error = GRrGroup_GetUns16(inGroup, "hardPause", &hardPause);
	if (UUcError_None != error) {
		hardPause = 0;
	}

	error = GRrGroup_GetUns8(inGroup, "compression", &compressionSize);
	if (UUcError_None != error) {
		if (flags & (1 << ONcAnimFlag_Overlay)) {
			compressionSize = cDefaultScreenCompressionSize;
		}
		else {
			compressionSize = IMPgDefaultCompressionSize;
		}	
	}

	// get the actionFrame which may also be called the swapWeaponFrame
	error = GRrGroup_GetInt16(inGroup, "swapWeaponFrame", &actionFrame);

	if (UUcError_None != error) {
		error = GRrGroup_GetInt16(inGroup, "actionFrame", &actionFrame);
	}

	if (UUcError_None != error) {
		actionFrame = -1;
	}

	error = GRrGroup_GetString(inGroup, "reverseInstance", &reverseName);
	if (UUcError_None != error) {
		reverseName = NULL;
	}

	if (NULL != reverseName) {
		error = GRrGroup_GetString(inGroup, "reverseAnimType", &animTypeStr);
		UUmError_ReturnOnErrorMsg(error, "missing reverseAnimType");

		error = IMPrStringToAnimType(animTypeStr, &reverseAnimType);
		UUmError_ReturnOnErrorMsg(error, "invalid animType");

		revFromState = toState;
		revToState = fromState;

		error = iGetAnimStateQuietly(inGroup, "reverseFromState", &revFromState);
		UUmError_ReturnOnError(error);

		error = iGetAnimStateQuietly(inGroup, "reverseToState", &revToState);
		UUmError_ReturnOnError(error);

		Imp_PrintMessage(IMPcMsg_Cosmetic, "adding reversed version (%s)"UUmNL, reverseName);

		if (aimingScreenAnimType != animType)
		{
			Imp_PrintWarning("AimingScreenAnimType is not legal for reversed animations, it would be easy to fix");
		}
	}

	// take damage structure
	numTakeDamages = 0;

	for(itr = 0; itr < TRcMaxTakeDamages; itr++)
	{
		takeDamages[numTakeDamages].damage = 0;
		takeDamages[numTakeDamages].frame = 0;

		sprintf(suffix, "%d", itr + 1);
		sprintf(varName, "takeDamage%s", suffix);
		error = GRrGroup_GetUns16(inGroup, varName, &(takeDamages[numTakeDamages].damage));
		if (error) { continue; }

		sprintf(suffix, "%d", itr + 1);
		sprintf(varName, "takeDamageFrame%s", suffix);
		error = GRrGroup_GetUns16(inGroup, varName, &(takeDamages[numTakeDamages].frame));
		if (error) { continue; }

		numTakeDamages++;
	}	

	// animation particles
	numParticles = 0;
	error = GRrGroup_GetElement(inGroup, "particles", &elemType, &element);
	if (error == UUcError_None) {
		if (elemType != GRcElementType_Array) {
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "animation 'particles' must be an array");
		}

		error = iParseAnimationParticles((GRtElementArray *) element, &numParticles, particles, numAttacks, attacks);
		UUmError_ReturnOnErrorMsg(error, "failed to parse animation particles");
	}

	error = GRrGroup_GetString(inGroup, "attackImpactName", &attack_impact_name);
	if (error != UUcError_None) {
		attack_impact_name = "";
	} else {
		if (strlen(attack_impact_name) > ONcCharacterAttackNameLength) {
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "attack impact names may be at most 15 characters long (sorry)");
		}
	}

	error = GRrGroup_GetUns16(inGroup, "firstLevel", &firstLevel);
	if (error != UUcError_None) {
		firstLevel = 0;
	}

	/*
	 * set the attack directions
	 */

	iSetAttackDirection(flags, numAttacks, animType, fromState, numShortcuts, shortcuts, &fwd_direction);
	if (reverseName != NULL) {
		iSetAttackDirection(flags, numAttacks, reverseAnimType, revFromState, numReverseShortcuts, reverseShortcuts, &reverseDirection);
	}

	/*
	 * read and process the animation file
	 */

	UUrMemory_Clear(&extentInfo, sizeof(extentInfo));
	error = iProcessAnimationFile(
			ANMFileRef, 
			flags,
			animType,
			footstepDetectType,
			&ANMMappingRef,
			&header,
			&positionArray,
			&quatArray,
			numAttacks,
			attacks,
			&extentInfo,
			&automaticFootstepList,
			fwd_direction,
			&framePositionArray);
	UUmError_ReturnOnErrorMsg(error, "failed to process animation file");


	if (IMPgDebugFootsteps) {
		UUtInt32 itr;

		if (forcedFootstepList) {
			for(itr = 0; itr < forcedFootstepList->numFootsteps; itr++)
			{
				Imp_PrintMessage(IMPcMsg_Important, "fwd footstep %d %d"UUmNL, forcedFootstepList->footsteps[itr].frame, forcedFootstepList->footsteps[itr].type);
			}
		}
		if (forcedReverseFootstepList) {
			for(itr = 0; itr < forcedReverseFootstepList->numFootsteps; itr++)
			{
				Imp_PrintMessage(IMPcMsg_Important, "rev footstep %d %d"UUmNL, forcedReverseFootstepList->footsteps[itr].frame, forcedReverseFootstepList->footsteps[itr].type);
			}
		}
		if (forcedFootstepList == NULL) {
			for(itr = 0; itr < automaticFootstepList->numFootsteps; itr++)
			{
				Imp_PrintMessage(IMPcMsg_Important, "auto footstep %d %d"UUmNL, automaticFootstepList->footsteps[itr].frame, automaticFootstepList->footsteps[itr].type);
			}
		}
	}

	if (header->numNodes >= TRcMaxParts) {
		AUrMessageBox(AUcMBType_OK, "ANMFile %s had %d nodes, %d is the maximum allowed!", 
			BFrFileRef_GetLeafName(ANMFileRef),
			header->numNodes,
			TRcMaxParts);
	}


	/*
	 * create the animation templated structures
	 */

	error = iCreateBlankAnimation(inInstanceName, header->duration, fps, header->numNodes, compressionSize, 
		soundPlaceHolders, soundFrames, attacks, blurs, directAnimationPlaceHolder, &animation);
	IMPmError_ReturnOnError(error);
	
	UUmAssertWritePtr(animation, sizeof(*animation));
	animation->instanceName = NULL;
	animation->type = animType;
	animation->fromState = fromState;
	animation->toState = toState;
	animation->flags = flags;
	animation->varient = varient;
	animation->finalRotation = finalRotation;
	animation->atomicStart = atomicStart;
	animation->atomicEnd = atomicEnd;
	animation->endInterpolation = endInterpolation;
	animation->maxInterpolation = maxInterpolation;
	animation->softPause = softPause;
	animation->hardPause = hardPause;
	animation->replaceParts = replaceParts;
	animation->actionFrame = actionFrame;
	animation->aimingType = aimingScreenAnimType;
	animation->moveDirection = fwd_direction;
	animation->numAttacks = numAttacks;
	animation->numTakeDamages = numTakeDamages;
	animation->numBlurs = numBlurs;
	animation->numShortcuts = numShortcuts;
	animation->numParticles = numParticles;
	animation->firstLevel = firstLevel;
	animation->invulnerableStart = invulnerableStart;
	animation->invulnerableLength = invulnerableLength;
	animation->vocalizationType = vocalizationType;
	animation->newSounds = new_sounds;
	animation->numNewSounds = num_new_sounds;
	UUrString_Copy(animation->attackName, attack_impact_name, ONcCharacterAttackNameLength + 1);

	if (NULL != reverseName) {
		error = iCreateBlankAnimation(reverseName, header->duration, fps, header->numNodes, compressionSize, 
			soundPlaceHolders, soundFrames, attacks, blurs, directAnimationPlaceHolder, &reverseAnimation);
		IMPmError_ReturnOnError(error);
		
		UUmAssertWritePtr(reverseAnimation, sizeof(*reverseAnimation));
		reverseAnimation->instanceName = NULL;
		reverseAnimation->type = reverseAnimType;
		reverseAnimation->fromState = revFromState;
		reverseAnimation->toState = revToState;
		reverseAnimation->flags = flags;
		reverseAnimation->varient = varient;
		reverseAnimation->finalRotation = finalRotation;
		reverseAnimation->atomicStart = atomicStart;
		reverseAnimation->atomicEnd = atomicEnd;
		reverseAnimation->endInterpolation = endInterpolation;
		reverseAnimation->maxInterpolation = maxInterpolation;
		reverseAnimation->softPause = softPause;
		reverseAnimation->hardPause = hardPause;
		reverseAnimation->replaceParts = replaceParts;
		reverseAnimation->actionFrame = actionFrame;
		reverseAnimation->aimingType = reverseAnimType;
		reverseAnimation->moveDirection = reverseDirection;
		reverseAnimation->numAttacks = numAttacks;
		reverseAnimation->numTakeDamages = numTakeDamages;
		reverseAnimation->numBlurs = numBlurs;
		reverseAnimation->numShortcuts = numReverseShortcuts;
		reverseAnimation->numParticles = numParticles;
		reverseAnimation->firstLevel = firstLevel;
		reverseAnimation->invulnerableStart = invulnerableStart;
		reverseAnimation->invulnerableLength = invulnerableLength;
		reverseAnimation->vocalizationType = vocalizationType;
		reverseAnimation->newSounds = new_sounds;
		reverseAnimation->numNewSounds = num_new_sounds;
		UUrString_Copy(reverseAnimation->attackName, attack_impact_name, ONcCharacterAttackNameLength + 1);
	}

	// special stuff for throws
	if (flags & (1 << ONcAnimFlag_ThrowSource)) {
		M3tPoint3D throwSource, throwTarget;

		// step 1 fix translation
		throwSource.x = header->rootTranslation[1].x;
		throwSource.y = header->rootTranslation[1].z;
		throwSource.z = -header->rootTranslation[1].y;

		throwTarget.x = header->rootTranslation[0].x;
		throwTarget.y = header->rootTranslation[0].z;
		throwTarget.z = -header->rootTranslation[0].y;

		MUmVector_Subtract(throwInfo.throwOffset, throwTarget, throwSource);

		error = Imp_GetAnimType(inGroup, "throwType", &throwInfo.throwType);
		UUmError_ReturnOnError(error);

		error = GRrGroup_GetFloat(inGroup, "relativeThrowFacing", &throwInfo.relativeThrowFacing);
		throwInfo.relativeThrowFacing *= M3c2Pi / 360.f;
		if (UUcError_None != error) { Imp_PrintWarning("missing relativeThrowFacing"); }
		UUmError_ReturnOnError(error);

		error = GRrGroup_GetFloat(inGroup, "throwDistance", &throwInfo.throwDistance);
		if (UUcError_None != error) { Imp_PrintWarning("missing throwDistance"); }
		UUmError_ReturnOnError(error);
	}

	// check that actionFrame is correct
	if (animation->actionFrame > (header->duration - 2)) {
		Imp_PrintMessage(IMPcMsg_Important, "actionFrame was %d truncated to %d"UUmNL, actionFrame, header->duration - 2);
		actionFrame = header->duration - 2;

	} else if ((reverseAnimation != NULL) && (reverseAnimation->actionFrame > (header->duration - 2))) {
		Imp_PrintMessage(IMPcMsg_Important, "actionFrame was %d truncated to %d"UUmNL, reverseAnimation->actionFrame, header->duration - 2);
		reverseAnimation->actionFrame = header->duration - 2;
	}

	// get overlay and the parts that it uses
	usedParts = 0;
	if (flags & (1 << ONcAnimFlag_Overlay)) {
		error = iMakeAnimationOverlay(header, positionArray, quatArray, replaceParts, &usedParts);
		UUmError_ReturnOnErrorMsg(error, "failed to convert animation to overlay");
	}
	animation->usedParts = usedParts;
	if (reverseAnimation != NULL) {
		reverseAnimation->usedParts = usedParts;
	}

	/*
	 * perform final build of animation
	 */

	error = iInsertAnimationArrays(	animation,
									+1,
									turnOveride,
									turnOverideValue,
									quatArray,
									positionArray,
									attacks,
									takeDamages,
									blurs,
									shortcuts,
									&throwInfo,
									particles,
									framePositionArray,
									&extentInfo,
									UUcFalse,
									(forcedFootstepList != NULL) ? forcedFootstepList : automaticFootstepList);
	UUmError_ReturnOnError(error);

	if (NULL != reverseAnimation) {
		if (forcedReverseFootstepList == NULL) {
			useFootstepsReversed = UUcTrue;
			useFootsteps = automaticFootstepList;
		} else {
			useFootstepsReversed = UUcFalse;
			useFootsteps = forcedReverseFootstepList;
		}

		error = iInsertAnimationArrays(	reverseAnimation,
										-1,
										turnOveride,
										turnOverideValue,
										quatArray,
										positionArray,
										attacks,
										takeDamages,
										blurs,
										reverseShortcuts,
										&throwInfo,
										particles,
										framePositionArray,
										&extentInfo,
										useFootstepsReversed,
										useFootsteps);
		UUmError_ReturnOnError(error);
	}
	
	if (forcedFootstepList != NULL) {
		UUrMemory_Block_Delete(forcedFootstepList);
	}
	if (forcedReverseFootstepList != NULL) {
		UUrMemory_Block_Delete(forcedReverseFootstepList);
	}

	iCloseAnimationFile(ANMMappingRef, header, positionArray, quatArray, framePositionArray, automaticFootstepList);
	BFrFileRef_Dispose(ANMFileRef);
	
	return UUcError_None;
}

UUtError
Imp_AddAnimationCollection(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError		error;
	UUtUns16		itr;
	TRtAnimationCollection *collection;
	GRtElementType	elementType;

	UUtUns32			numAnimations;
	GRtElementArray*	animationList;

	AUtMB_ButtonChoice buttonChoice;
	char *recursiveLookupString;
	
	if(TMrConstruction_Instance_CheckExists(TRcTemplate_AnimationCollection, inInstanceName)) {
		Imp_PrintMessage(IMPcMsg_Important, "animation collection already imported"UUmNL);
		return UUcError_None;
	}
	
	// open our group array
	error = GRrGroup_GetElement(
				inGroup, 
				"tagList", 
				&elementType, 
				&animationList);

	if ((error != UUcError_None) || (elementType != GRcElementType_Array))
	{
		buttonChoice = AUrMessageBox(AUcMBType_AbortRetryIgnore, "Could not get animation list group");

		if (AUcMBChoice_Ignore == buttonChoice) {
			return UUcError_None;
		}

		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Could not get animation list group");
	}

	numAnimations = GRrGroup_Array_GetLength(animationList);

	// create our instance
	error = TMrConstruction_Instance_Renew(
				TRcTemplate_AnimationCollection,
				inInstanceName,
				numAnimations,
				&collection);
	IMPmError_ReturnOnError(error);

	// clear our pad
	collection->recursiveOnly = 0;

	// check for a recursiveSearch table
	error = GRrGroup_GetString(inGroup, "lookup", &recursiveLookupString);
	
	if (UUcError_None == error) {
		Imp_PrintMessage(IMPcMsg_Cosmetic, "recursive lookup table %s"UUmNL, recursiveLookupString);

		error = TMrConstruction_Instance_GetPlaceHolder(
			TRcTemplate_AnimationCollection, 
			recursiveLookupString, 
			(TMtPlaceHolder*)&(collection->recursiveLookup));
		UUmError_ReturnOnErrorMsg(error, "Imp_AddAnimationCollection: could not make a placeholder");
	}

	// read the data
	for(itr = 0; itr < numAnimations; itr++)
	{
		GRtGroup*		animListEntryGroup;
		char*			instanceTag;
		UUtUns16		weight;
		TRtAnimation	*animation;
		UUtInt16		initial_level;

		error = GRrGroup_Array_GetElement(
			animationList,
			itr,
			&elementType,
			&animListEntryGroup);
		if (elementType != GRcElementType_Group) {
			error = UUcError_Generic;
		}
		UUmError_ReturnOnErrorMsg(error, "could not get animation list entry");

		error = GRrGroup_GetInt16(animListEntryGroup, "level", &initial_level);
		if (error) {
			initial_level = -1;
		}

		error =	GRrGroup_GetString(animListEntryGroup, "tag", &instanceTag);
		UUmError_ReturnOnErrorMsg(error, "could not find instance tag");

		if (0 == strcmp(instanceTag, "")) {
			Imp_PrintWarning("animation tags should not be blank");
		}
		
		error = GRrGroup_GetUns16(animListEntryGroup, "weight", &weight);
		UUmError_ReturnOnErrorMsg(error, "could not find weight");
		
		error = TMrConstruction_Instance_GetPlaceHolder(
			TRcTemplate_Animation, 
			instanceTag, 
			(TMtPlaceHolder*)&animation);
		UUmError_ReturnOnErrorMsg(error, "Imp_AddAnimationCollection: could not make a placeholder");
		
		collection->entry[itr].animation = animation;
		collection->entry[itr].weight = weight;
		collection->entry[itr].virtualFromState = TRcAnimState_None;
	}

	return UUcError_None;
}

UUtError
Imp_AddScreenCollection(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError		error;
	UUtUns16		itr;
	TRtScreenCollection *collection;
	GRtElementType	elementType;

	UUtUns16			numScreens;
	GRtElementArray*	animationList;

	AUtMB_ButtonChoice buttonChoice;
	
	if(TMrConstruction_Instance_CheckExists(TRcTemplate_ScreenCollection, inInstanceName)) {
		Imp_PrintMessage(IMPcMsg_Important, "screen collection already imported"UUmNL);
		return UUcError_None;
	}

	// open our group array
	error = GRrGroup_GetElement(
				inGroup, 
				"tagList", 
				&elementType, 
				&animationList);

	if ((error != UUcError_None) || (elementType != GRcElementType_Array))
	{
		buttonChoice = AUrMessageBox(AUcMBType_AbortRetryIgnore, "Could not get animation list group");

		if (AUcMBChoice_Ignore == buttonChoice) {
			return UUcError_None;
		}

		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Could not get animation list group");
	}

	numScreens = (UUtUns16) GRrGroup_Array_GetLength(animationList);

	// create our instance
	error = TMrConstruction_Instance_Renew(
				TRcTemplate_ScreenCollection,
				inInstanceName,
				numScreens,
				&collection);
	IMPmError_ReturnOnError(error);

	// read the data
	for(itr = 0; itr < numScreens; itr++)
	{
		GRtGroup*		animListEntryGroup;
		char*			instanceTag;
		TRtAimingScreen	*screen;

		error = GRrGroup_Array_GetElement(
			animationList,
			itr,
			&elementType,
			&animListEntryGroup);
		if (elementType != GRcElementType_Group) {
			error = UUcError_Generic;
		}
		UUmError_ReturnOnErrorMsg(error, "could not get animation list entry");

		error =	GRrGroup_GetString(animListEntryGroup, "tag", &instanceTag);
		UUmError_ReturnOnErrorMsg(error, "could not find instance tag");

		if (0 == strcmp(instanceTag, "")) {
			Imp_PrintWarning("aiming screen tags should not be blank");
		}
			
		error = TMrConstruction_Instance_GetPlaceHolder(
			TRcTemplate_AimingScreen, 
			instanceTag, 
			(TMtPlaceHolder*)&screen);
		UUmError_ReturnOnErrorMsg(error, "Imp_AddAnimationCollection: could not make a placeholder");
		
		collection->screen[itr] = screen;
	}

	return UUcError_None;
}

enum {
	cCharacterParticle_TagName,
	cCharacterParticle_ClassName,
	cCharacterParticle_PartIndex,

	cCharacterParticle_ArrayLength
};

static UUtError
IMPiParseCharacterParticleElement(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModData,
	GRtElementArray*	inArray,
	UUtUns32*			ioNumParticles,
	UUtUns32			inMaxNumParticles,
	UUtBool*			outOverflowedArray,
	ONtCharacterParticle*ioParticles)
{
	UUtError error;
	UUtUns32 itr, itr2;
	GRtElementType element_type;
	void *element;
	ONtCharacterParticle particle;

	if (GRrGroup_Array_GetLength(inArray) < cCharacterParticle_ArrayLength - 1) {
		IMPmError_ReturnOnErrorMsg(UUcError_Generic, "character particle: array has too few elements");
	}

	// read the particle tag
	error =	GRrGroup_Array_GetElement(inArray, cCharacterParticle_TagName, &element_type, &element);
	IMPmError_ReturnOnError(error);
	if (element_type != GRcElementType_String) {
		IMPmError_ReturnOnErrorMsg(UUcError_Generic, "character particle: tag must be a string");
	}

	// look for particles in the array that already have this name
	for (itr = 0; itr < *ioNumParticles; itr++) {
		if (UUmString_IsEqual(ioParticles[itr].name, (const char *) element)) {
			break;
		}
	}
	if (itr >= inMaxNumParticles) {
		// we cannot add another particle to the array
		if (*outOverflowedArray == 0) {
			*outOverflowedArray = UUcTrue;
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "character particle: exceeded maximum number of particles");
		} else {
			// fail silently on subsequent attempts
			return UUcError_Generic;
		}
	}

	/*
	 * set up the particle
	 */

	UUrString_Copy(particle.name, (const char *) element, ONcCharacterParticleNameLength + 1);

	// read the particle class name
	error =	GRrGroup_Array_GetElement(inArray, cCharacterParticle_ClassName, &element_type, &element);
	IMPmError_ReturnOnError(error);
	if (element_type != GRcElementType_String) {
		IMPmError_ReturnOnErrorMsg(UUcError_Generic, "character particle: particle class must be a string");
	}
	UUrString_Copy(particle.particle_classname, (const char *) element, P3cParticleClassNameLength + 1);

	// initially no override part-index
	particle.override_partindex = (UUtUns16) -1;
	if (GRrGroup_Array_GetLength(inArray) > cCharacterParticle_PartIndex) {
		// read the part name
		error = GRrGroup_Array_GetElement(inArray, cCharacterParticle_PartIndex, &element_type, &element);
		IMPmError_ReturnOnError(error);
		if (element_type != GRcElementType_String) {
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "character particle: part index must be a string");
		}

		if (UUmString_IsEqual((const char *) element, "hitflash")) {
			// this particle behaves as if it was a hitflash
			particle.override_partindex = ONcCharacterParticle_HitFlash;
		} else {
			// convert this string to a part index
			for (itr2 = 0; cParticle_PartName[itr2] != NULL; itr2++) {
				if (strcmp(cParticle_PartName[itr2], (const char *) element) == 0)
					break;
			}
			if (cParticle_PartName[itr2] == NULL) {
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "character particle: unknown part name");
			}

			particle.override_partindex = (UUtUns16) itr2;
		}
	}

	// particle class will be set up at runtime
	particle.particle_class = NULL;
	
	// success, copy the particle into the array and increment the number of particles if we have to
	ioParticles[itr] = particle;
	if (itr >= *ioNumParticles) {
		*ioNumParticles = itr + 1;
		UUmAssert(*ioNumParticles <= inMaxNumParticles);
	}

	return UUcError_None;
}

static UUtError
IMPiParseCharacterParticles(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName,
	ONtCharacterClass	*inClass)
{
	UUtError error;
	UUtUns32 itr, array_length, num_particles;
	UUtBool array_full;
	ONtCharacterParticle particle_storage[IMPcMaxCharacterParticles];
	GRtElementType element_type;
	GRtElementArray *array;
	void *element;

	// no particles initially
	num_particles = 0;
	array_full = UUcFalse;
	UUrMemory_Clear(&particle_storage, sizeof(particle_storage));

	/*
	 * parse the particles - first defaultParticles (from the class defaults) then particles
	 */

	error = GRrGroup_GetElement(inGroup, "defaultParticles", &element_type, &element);
	if (error == UUcError_None) {
		if (element_type == GRcElementType_Array) {
			array = (GRtElementArray *) element;

			array_length = GRrGroup_Array_GetLength(array);
			for (itr = 0; itr < array_length; itr++) {
				error = GRrGroup_Array_GetElement(array, itr, &element_type, &element);

				if ((error == UUcError_None) && (element_type == GRcElementType_Array)) {
					IMPiParseCharacterParticleElement(inSourceFile, inSourceFileModDate, (GRtElementArray *) element,
														&num_particles, IMPcMaxCharacterParticles, &array_full, particle_storage);
				}
			}
		}
	}

	error = GRrGroup_GetElement(inGroup, "particles", &element_type, &element);
	if (error == UUcError_None) {
		if (element_type == GRcElementType_Array) {
			array = (GRtElementArray *) element;

			array_length = GRrGroup_Array_GetLength(array);
			for (itr = 0; itr < array_length; itr++) {
				error = GRrGroup_Array_GetElement(array, itr, &element_type, &element);

				if ((error == UUcError_None) && (element_type == GRcElementType_Array)) {
					IMPiParseCharacterParticleElement(inSourceFile, inSourceFileModDate, (GRtElementArray *) element,
														&num_particles, IMPcMaxCharacterParticles, &array_full, particle_storage);
				}
			}
		}
	}

	/*
	 * write the particles into the character class
	 */

	if (num_particles == 0) {
		inClass->particles = NULL;
		return UUcError_None;
	}

	// create the particle array
	error =
		TMrConstruction_Instance_NewUnique(
			ONcTemplate_CharacterParticleArray,
			num_particles,
			&inClass->particles);
	UUmError_ReturnOnError(error);

	// copy the constructed data into the particle array
	inClass->particles->numParticles = num_particles;
	UUrMemory_MoveFast(&particle_storage, &inClass->particles->particle, num_particles * sizeof(ONtCharacterParticle));
	inClass->particles->classes_found = UUcFalse;
	
	return UUcError_None;
}

enum {
	cCharacterImpact_AttackName,
	cCharacterImpact_ImpactName,
	cCharacterImpact_ModifierName,

	cCharacterImpact_ArrayLength
};

static UUtError
IMPiParseCharacterImpacts(
	BFtFileRef*					inSourceFile,
	UUtUns32					inSourceFileModDate,
	char*						inInstanceName,
	GRtElementArray *			inArray,
	ONtCharacterImpactArray		**outImpacts)
{
	UUtError error;
	UUtUns32 itr, num_impacts;
	GRtElementType element_type;
	GRtElementArray *sub_array;
	void *element;
	ONtCharacterImpactArray *impact_array;
	ONtCharacterAttackImpact *impact;

	// get the number of items in the array
	num_impacts = GRrGroup_Array_GetLength(inArray);
	if (num_impacts == 0) {
		*outImpacts = NULL;
		return UUcError_None;
	}

	// create the impact array
	error =
		TMrConstruction_Instance_NewUnique(
			ONcTemplate_CharacterImpactArray,
			num_impacts,
			&impact_array);
	UUmError_ReturnOnError(error);

	*outImpacts = impact_array;

	impact_array->numImpacts = num_impacts;
	impact_array->indices_found = UUcFalse;
	for (itr = 0, impact = impact_array->impact; itr < num_impacts; itr++, impact++) {
		// get the sub-array
		error =	GRrGroup_Array_GetElement(inArray, itr, &element_type, &element);
		IMPmError_ReturnOnError(error);
		if (element_type != GRcElementType_Array) {
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "character impact array: elements of array must themselves be arrays");
		}
		sub_array = (GRtElementArray *) element;

		if (GRrGroup_Array_GetLength(sub_array) != cCharacterImpact_ArrayLength) {
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "character impact: array has incorrect number of elements");
		}

		// read the attack animation's "attack name"
		error =	GRrGroup_Array_GetElement(sub_array, cCharacterImpact_AttackName, &element_type, &element);
		IMPmError_ReturnOnError(error);
		if (element_type != GRcElementType_String) {
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "character impact: attack name must be a string");
		}
		UUrString_Copy(impact->name, (const char *) element, ONcCharacterAttackNameLength + 1);

		// read the impact type name
		error =	GRrGroup_Array_GetElement(sub_array, cCharacterImpact_ImpactName, &element_type, &element);
		IMPmError_ReturnOnError(error);
		if (element_type != GRcElementType_String) {
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "character impact: impact type name must be a string");
		}
		UUrString_Copy(impact->impact_name, (const char *) element, 128);

		// read the modifier type name
		error =	GRrGroup_Array_GetElement(sub_array, cCharacterImpact_ModifierName, &element_type, &element);
		if (error == UUcError_None) {
			if (element_type != GRcElementType_String) {
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "character impact: modifier type name must be a string");
			}
			UUrString_Copy(impact->modifier_name, (const char *) element, 16);
		} else {
			UUrString_Copy(impact->modifier_name, ONrIEModType_GetName(ONcIEModType_Any), 16);
		}

		// actual type indices will be set up at runtime
		impact->impact_type = MAcInvalidID;
		impact->modifier_type = ONcIEModType_Any;
	}
	
	return UUcError_None;
}

static UUtError
IMPiParseCharacterAnimationImpacts(
	BFtFileRef*					inSourceFile,
	UUtUns32					inSourceFileModDate,
	char*						inInstanceName,
	char*						inModifierName,
	GRtElementArray *			inArray,
	ONtCharacterAnimationImpacts *outImpacts)
{
	enum {
		cCharacterAnimationImpact_AnimType,
		cCharacterAnimationImpact_ImpactName,

		cCharacterAnimationImpact_ArrayLength
	};

	UUtUns32 itr, num_items;
	UUtError error;
	GRtElementType element_type;
	GRtElementArray *sub_array;
	void *element;
	ONtCharacterAnimationImpact *impact;
	const ONtCharacterAnimationImpactType *type_ptr;

	num_items = GRrGroup_Array_GetLength(inArray);

	UUrString_Copy(outImpacts->modifier_name, inModifierName, 16);

	for (itr = 0; itr < num_items; itr++) {
		// get the sub-array
		error =	GRrGroup_Array_GetElement(inArray, itr, &element_type, &element);
		IMPmError_ReturnOnError(error);
		if (element_type != GRcElementType_Array) {
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "character animation impact array: elements of array must themselves be arrays");
		}
		sub_array = (GRtElementArray *) element;

		if (GRrGroup_Array_GetLength(sub_array) != cCharacterAnimationImpact_ArrayLength) {
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "character animation impact: array has incorrect number of elements");
		}

		// read the animation type
		error =	GRrGroup_Array_GetElement(sub_array, cCharacterAnimationImpact_AnimType, &element_type, &element);
		IMPmError_ReturnOnError(error);
		if (element_type != GRcElementType_String) {
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "character animation impact: anim type must be a string");
		}

		// work out which index in the array this corresponds to
		for (type_ptr = ONcAnimImpactTypes; type_ptr->name != NULL; type_ptr++) {
			if (UUmString_IsEqual(type_ptr->name, (const char *) element)) {
				break;
			}
		}
		if (type_ptr == NULL) {
			Imp_PrintMessage(IMPcMsg_Important, "character animation impact: unknown impact anim type %s. valid impact anim types are:\n",
							(const char *) element);
			for (type_ptr = ONcAnimImpactTypes; type_ptr->name != NULL; type_ptr++) {
				Imp_PrintMessage(IMPcMsg_Important, "  %s\n", type_ptr->name);
			}
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "character animation impact: unknown impact anim type");
		}		

		UUmAssert((type_ptr->type >= 0) && (type_ptr->type < ONcAnimationImpact_Max));
		impact = &outImpacts->impact[type_ptr->type];

		// read the impact type name
		error =	GRrGroup_Array_GetElement(sub_array, cCharacterAnimationImpact_ImpactName, &element_type, &element);
		IMPmError_ReturnOnError(error);
		if (element_type != GRcElementType_String) {
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "character animation impact: impact type name must be a string");
		}
		UUrString_Copy(impact->impact_name, (const char *) element, 128);

		// actual type indices will be set up at runtime
		impact->impact_type = MAcInvalidID;
	}

	outImpacts->indices_found = UUcFalse;
	return UUcError_None;
}

UUtError
Imp_AddCharacterClass(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError error;
	ONtCharacterClass *characterClass;
	ONtPainConstants *pain_constants;
	UUtUns8 numOpenGroup = 0;
	GRtGroup_Context *context;
	GRtElementType elemType;
	void *element;

	char	*modifierName;
	char	*variantName;
	char	*bodyTag;
	char	*textureTag;
	char	*animationsTag;
	char	*screenTag;
	char	*scriptName;
	char	*particleClass;
	char	*soundName;
	char	keyname[64];

	float	jumpHeight;
	float	shadowAlpha;

	if(TMrConstruction_Instance_CheckExists(TRcTemplate_CharacterClass, inInstanceName)) {
		Imp_PrintMessage(IMPcMsg_Important, "character class already imported"UUmNL);
		return UUcError_None;
	}

	error = GRrGroup_Context_New(&context);
	UUmError_ReturnOnError(error);

	error = Imp_OpenGroupStack(inSourceFile, inGroup, context, "lookup");
	UUmAssert(UUcError_None == error);
	UUmError_ReturnOnError(error);

	// create our instance
	error = TMrConstruction_Instance_Renew(
			TRcTemplate_CharacterClass,
			inInstanceName,
			0,								// variable length array whoha
			&characterClass);
	UUmError_ReturnOnError(error);

	// get the variant tag
	error = GRrGroup_GetString(inGroup, "variant", &variantName);
	if (error != UUcError_None) {
		Imp_PrintWarning("character class %s has no 'variant' tag in any of its definition files!", inInstanceName);
		variantName = "unknown";
	}
	
	// model
	error =	GRrGroup_GetString(inGroup, "bodyTag", &bodyTag);
	UUmError_ReturnOnErrorMsg(error, "could not find bodyTag");

	if (0 == strcmp(bodyTag, "")) {
		Imp_PrintWarning("bodyTag should not be blank");
	}

	error = TMrConstruction_Instance_GetPlaceHolder(TRcTemplate_BodySet, bodyTag, (TMtPlaceHolder*)&(characterClass->body));
	UUmError_ReturnOnErrorMsg(error, "could not find body");

	// textures
	error =	GRrGroup_GetString(inGroup, "textureTag", &textureTag);
	UUmError_ReturnOnErrorMsg(error, "could not find textureTag");

	error = TMrConstruction_Instance_GetPlaceHolder(TRcTemplate_BodyTextures, textureTag, (TMtPlaceHolder*)&(characterClass->textures));
	UUmError_ReturnOnErrorMsg(error, "could not find body textures");
	
	// body part materials
	error = Imp_BodyPartMaterials(inGroup, &characterClass->bodyPartMaterials);
	UUmError_ReturnOnErrorMsg(error, "could not process the body part materials");
	
	// body part impacts
	error = Imp_BodyPartImpacts(inGroup, &characterClass->bodyPartImpacts);
	UUmError_ReturnOnErrorMsg(error, "could not process the body part impacts");
	
	// fight mode duration
	error = GRrGroup_GetUns32(inGroup, "fightModeDuration", &(characterClass->fightModeDuration));
	UUmError_ReturnOnErrorMsg(error, "could not find fightModeDuration");

	// idle delay
	error = GRrGroup_GetUns32(inGroup, "idleDelay", &(characterClass->idleDelay));
	UUmError_ReturnOnErrorMsg(error, "could not find idleDelay");

	error = GRrGroup_GetUns32(inGroup, "idleRestartDelay", &(characterClass->idleRestartDelay));
	UUmError_ReturnOnErrorMsg(error, "could not find idleRestartDelay");

	// feet
	error = GRrGroup_GetUns32(inGroup, "feet", &(characterClass->feetBits));
	UUmError_ReturnOnErrorMsg(error, "could not find feet");

	// maximum hit points
	error = GRrGroup_GetUns32(inGroup, "hits", &(characterClass->class_max_hit_points));
	UUmError_ReturnOnErrorMsg(error, "could not find hits");

	// animation
	error =	GRrGroup_GetString(inGroup, "animationTag", &animationsTag);
	UUmError_ReturnOnErrorMsg(error, "could not find animation collection tag");

	error = TMrConstruction_Instance_GetPlaceHolder(TRcTemplate_AnimationCollection, animationsTag, (TMtPlaceHolder*)&(characterClass->animations));
	UUmError_ReturnOnErrorMsg(error, "could not find animation collection");

	// screen
	error =	GRrGroup_GetString(inGroup, "screenTag", &screenTag);
	UUmError_ReturnOnErrorMsg(error, "could not find animation screen tag");

	error = TMrConstruction_Instance_GetPlaceHolder(TRcTemplate_ScreenCollection, screenTag, (TMtPlaceHolder*)&(characterClass->aimingScreens));
	UUmError_ReturnOnErrorMsg(error, "could not find animation screen");

	// air constants
	error = GRrGroup_GetFloat(inGroup, "air_fallGravity", &(characterClass->airConstants.fallGravity));
	UUmError_ReturnOnErrorMsg(error, "could not air fall gravity");

	error = GRrGroup_GetFloat(inGroup, "air_jumpGravity", &(characterClass->airConstants.jumpGravity));
	UUmError_ReturnOnErrorMsg(error, "could not find air jump gravity");

	error = GRrGroup_GetFloat(inGroup, "air_jumpAmount", &(characterClass->airConstants.jumpAmount));
	UUmError_ReturnOnErrorMsg(error, "could not find air jump amount");

	error = GRrGroup_GetFloat(inGroup, "air_maxVelocity", &(characterClass->airConstants.maxVelocity));
	UUmError_ReturnOnErrorMsg(error, "could not find air max velocity");

	error = GRrGroup_GetFloat(inGroup, "air_jumpAccelAmount", &(characterClass->airConstants.jumpAccelAmount));
	UUmError_ReturnOnErrorMsg(error, "could not find air jump accel amount");

	error = GRrGroup_GetUns16(inGroup, "air_framesFallGravity", &(characterClass->airConstants.framesFallGravity));
	UUmError_ReturnOnErrorMsg(error, "could not find air frames fall gravity");

	error = GRrGroup_GetUns16(inGroup, "air_numJumpAccelFrames", &(characterClass->airConstants.numJumpAccelFrames));
	UUmError_ReturnOnErrorMsg(error, "could not find air num jump accel frames");

	error = GRrGroup_GetFloat(inGroup, "air_damage_start", &(characterClass->airConstants.damage_start));
	UUmError_ReturnOnErrorMsg(error, "could not find air_damage_start");

	error = GRrGroup_GetFloat(inGroup, "air_damage_end", &(characterClass->airConstants.damage_end));
	UUmError_ReturnOnErrorMsg(error, "could not find air_damage_end");

	// Shooting
	error = GRrGroup_GetUns16(inGroup, "shottime", &(characterClass->shottime));
	UUmError_ReturnOnErrorMsg(error, "could not find shottime");

	/*
	 * shadow
	 */
	// shadow
	error =	GRrGroup_GetString( inGroup, "shadow_texture", &scriptName );
	if( error == UUcError_None )
	{
		characterClass->shadowConstants.shadow_texture = M3rTextureMap_GetPlaceholder(scriptName);
	}
	else
	{
		characterClass->shadowConstants.shadow_texture = NULL;
	}

	error = GRrGroup_GetFloat(inGroup, "shadow_heightMax", &(characterClass->shadowConstants.height_max));
	if (error != UUcError_None) {
		characterClass->shadowConstants.height_max = 12.0f;
	}

	error = GRrGroup_GetFloat(inGroup, "shadow_heightFade", &(characterClass->shadowConstants.height_fade));
	if (error != UUcError_None) {
		characterClass->shadowConstants.height_fade = 8.0f;
	}

	error = GRrGroup_GetFloat(inGroup, "shadow_sizeMax", &(characterClass->shadowConstants.size_max));
	if (error != UUcError_None) {
		characterClass->shadowConstants.size_max = 4.0f;
	}

	error = GRrGroup_GetFloat(inGroup, "shadow_sizeFade", &(characterClass->shadowConstants.size_fade));
	if (error != UUcError_None) {
		characterClass->shadowConstants.size_fade = 3.0f;
	}

	error = GRrGroup_GetFloat(inGroup, "shadow_sizeMin", &(characterClass->shadowConstants.size_min));
	if (error != UUcError_None) {
		characterClass->shadowConstants.size_min = 2.0f;
	}

	error = GRrGroup_GetFloat(inGroup, "shadow_alphaMax", &shadowAlpha);
	if (error != UUcError_None) {
		shadowAlpha = 0.7f;
	}
	characterClass->shadowConstants.alpha_max = (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(M3cMaxAlpha * shadowAlpha);

	error = GRrGroup_GetFloat(inGroup, "shadow_alphaFade", &shadowAlpha);
	if (error != UUcError_None) {
		shadowAlpha = 0.5;
	}
	characterClass->shadowConstants.alpha_fade = (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(M3cMaxAlpha * shadowAlpha);

	error = GRrGroup_GetBool(inGroup, "leftHanded", &(characterClass->leftHanded));
	UUmError_ReturnOnErrorMsg(error, "could not find leftHanded");

	error = GRrGroup_GetBool(inGroup, "superHypo", &(characterClass->superHypo));
	if (error != UUcError_None) { characterClass->superHypo = UUcFalse; }

	error = GRrGroup_GetBool(inGroup, "superShield", &(characterClass->superShield));
	if (error != UUcError_None) { characterClass->superShield = UUcFalse; }

	error = GRrGroup_GetBool(inGroup, "knockdownResistant", &(characterClass->knockdownResistant));
	if (error != UUcError_None) { characterClass->knockdownResistant = UUcFalse; }

	error = GRrGroup_GetFloat(inGroup, "bossShieldProtectAmount", &(characterClass->bossShieldProtectAmount));
	if (error != UUcError_None) { characterClass->bossShieldProtectAmount = 0.5f; }

	// jump constants
	error = GRrGroup_GetFloat(inGroup, "jump_up", &jumpHeight);
	if (error != UUcError_None)
	{
		UUtInt32 frameNumber;
		float velocity;

		// Not specified, so estimate
		jumpHeight = 0.0f;

		velocity = characterClass->airConstants.jumpAmount;

		for(frameNumber = 0; velocity > 0; frameNumber++)
		{
			jumpHeight += velocity;

			if (frameNumber < characterClass->airConstants.numJumpAccelFrames)
			{
				velocity += characterClass->airConstants.jumpAccelAmount;
			}

			velocity -= characterClass->airConstants.jumpGravity;
		}

		jumpHeight += 4.0f;	// Account for collision help

		Imp_PrintMessage(IMPcMsg_Important, "calculated jump height: %3.2f units"UUmNL,jumpHeight);
	}
	characterClass->jumpConstants.jumpHeight = (UUtUns8)jumpHeight;

	error = GRrGroup_GetFloat(inGroup, "jump_forward", &(characterClass->jumpConstants.jumpDistance));
	UUmError_ReturnOnErrorMsg(error, "could not find jump forward distance");
	characterClass->jumpConstants.jumpDistanceSquares = 
		(UUtUns8)(characterClass->jumpConstants.jumpDistance / PHcSquareSize);

	// Cover finding constants
	error = GRrGroup_GetFloat(inGroup, "cover_rayIncrement", &(characterClass->coverConstants.rayIncrement));
	UUmError_ReturnOnErrorMsg(error, "could not find cover ray increment");

	error = GRrGroup_GetFloat(inGroup, "cover_rayMax", &(characterClass->coverConstants.rayMax));
	UUmError_ReturnOnErrorMsg(error, "could not find cover ray max");

	error = GRrGroup_GetFloat(inGroup, "cover_rayAngle", &(characterClass->coverConstants.rayAngle));
	UUmError_ReturnOnErrorMsg(error, "could not find cover ray angle");

	error = GRrGroup_GetFloat(inGroup, "cover_rayAngleMax", &(characterClass->coverConstants.rayAngleMax));
	UUmError_ReturnOnErrorMsg(error, "could not find cover ray angle max");
	characterClass->coverConstants.rayAngle *= M3cDegToRad;
	characterClass->coverConstants.rayAngleMax *= M3cDegToRad;

	// Autofreeze constants
	error = GRrGroup_GetFloat(inGroup, "autofreeze_h", &(characterClass->autofreezeConstants.distance_xz));
	UUmError_ReturnOnErrorMsg(error, "could not find autofreeze H distance");

	error = GRrGroup_GetFloat(inGroup, "autofreeze_v", &(characterClass->autofreezeConstants.distance_y));
	UUmError_ReturnOnErrorMsg(error, "could not find autofreeze V distance");

	// Inventory constants	
	error = GRrGroup_GetUns16(inGroup, "inv_hypoTime", &(characterClass->inventoryConstants.hypoTime));
	UUmError_ReturnOnErrorMsg(error, "could not find hypo time");

	// lod constants
	error = GRrGroup_GetFloat(inGroup, "lod_distance_xlow", &(characterClass->lodConstants.distance_squared[TRcBody_SuperLow]));
	UUmError_ReturnOnError(error);
	error = GRrGroup_GetFloat(inGroup, "lod_distance_low", &(characterClass->lodConstants.distance_squared[TRcBody_Low]));
	UUmError_ReturnOnError(error);
	error = GRrGroup_GetFloat(inGroup, "lod_distance_med", &(characterClass->lodConstants.distance_squared[TRcBody_Medium]));
	UUmError_ReturnOnError(error);
	error = GRrGroup_GetFloat(inGroup, "lod_distance_high", &(characterClass->lodConstants.distance_squared[TRcBody_High]));
	UUmError_ReturnOnError(error);
	error = GRrGroup_GetFloat(inGroup, "lod_distance_xhigh", &(characterClass->lodConstants.distance_squared[TRcBody_SuperHigh]));
	UUmError_ReturnOnError(error);

	{
		UUtInt32 lod_distance_itr;

		for(lod_distance_itr = TRcBody_SuperLow; lod_distance_itr <= TRcBody_SuperHigh; lod_distance_itr++)
		{
			characterClass->lodConstants.distance_squared[lod_distance_itr] = UUmSQR(characterClass->lodConstants.distance_squared[lod_distance_itr]);
		}
	}
	

	// scale constants
	error = GRrGroup_GetFloat(inGroup, "scale_low", &(characterClass->scaleLow));
	UUmError_ReturnOnError(error);

	error = GRrGroup_GetFloat(inGroup, "scale_high", &(characterClass->scaleHigh));
	UUmError_ReturnOnError(error);

	// vision constants
	error = GRrGroup_GetFloat(inGroup, "vision_distance", &(characterClass->vision.central_distance));
	UUmError_ReturnOnError(error);

	error = GRrGroup_GetFloat(inGroup, "vision_peripheral_distance", &(characterClass->vision.periph_distance));
	if (error == UUcError_None) {
		// central distance MUST be at least as long as peripheral distance
		characterClass->vision.central_distance = UUmMax(characterClass->vision.periph_distance, characterClass->vision.central_distance);
	} else {
		characterClass->vision.periph_distance = characterClass->vision.central_distance;
	}	

	error = GRrGroup_GetFloat(inGroup, "vision_vertical", &(characterClass->vision.vertical_range));
	UUmError_ReturnOnError(error);

	error = GRrGroup_GetFloat(inGroup, "vision_central_min_angle", &(characterClass->vision.central_range));
	UUmError_ReturnOnError(error);

	error = GRrGroup_GetFloat(inGroup, "vision_central_max_angle", &(characterClass->vision.central_max));
	UUmError_ReturnOnError(error);

	error = GRrGroup_GetFloat(inGroup, "vision_peripheral_min_angle", &(characterClass->vision.periph_range));
	UUmError_ReturnOnError(error);

	error = GRrGroup_GetFloat(inGroup, "vision_peripheral_max_angle", &(characterClass->vision.periph_max));
	UUmError_ReturnOnError(error);

	characterClass->vision.vertical_range = MUrCos(characterClass->vision.vertical_range * M3cDegToRad);
	characterClass->vision.central_range = MUrCos(characterClass->vision.central_range * M3cDegToRad);
	characterClass->vision.central_max = MUrCos(characterClass->vision.central_max * M3cDegToRad);
	characterClass->vision.periph_range = MUrCos(characterClass->vision.periph_range * M3cDegToRad);
	characterClass->vision.periph_max = MUrCos(characterClass->vision.periph_max * M3cDegToRad);

	// hearing constants
	error = GRrGroup_GetFloat(inGroup, "hearing_acuity", &(characterClass->hearing.acuity));
	UUmError_ReturnOnError(error);

	// memory constants
	error = GRrGroup_GetUns32(inGroup, "memory_definite_to_strong", &(characterClass->memory.definite_to_strong));
	UUmError_ReturnOnError(error);

	error = GRrGroup_GetUns32(inGroup, "memory_strong_to_weak", &(characterClass->memory.strong_to_weak));
	UUmError_ReturnOnError(error);

	error = GRrGroup_GetUns32(inGroup, "memory_weak_to_forgotten", &(characterClass->memory.weak_to_forgotten));
	UUmError_ReturnOnError(error);

	// alert status decay
	error = GRrGroup_GetUns32(inGroup, "alert_high_to_med", &(characterClass->memory.high_to_med));
	UUmError_ReturnOnError(error);

	error = GRrGroup_GetUns32(inGroup, "alert_med_to_low", &(characterClass->memory.med_to_low));
	UUmError_ReturnOnError(error);

	error = GRrGroup_GetUns32(inGroup, "alert_low_to_lull", &(characterClass->memory.low_to_lull));
	UUmError_ReturnOnError(error);

	// read the class's AI2 behavior
	error = Imp_AI2_ReadClassBehavior(inSourceFile, inSourceFileModDate, inGroup, inInstanceName,
										characterClass);
	UUmError_ReturnOnError(error);

	// get the character class's particles
	error = IMPiParseCharacterParticles(inSourceFile, inSourceFileModDate, inGroup, inInstanceName,
										characterClass);
	UUmError_ReturnOnError(error);

	// get the impacts tag
	error = GRrGroup_GetElement(inGroup, "impacts", &elemType, &element);
	if (error == UUcError_None) {
		if (elemType != GRcElementType_Array) {
			characterClass->impacts = NULL;
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "'impacts' field is not an array!");
		}

		error = IMPiParseCharacterImpacts(inSourceFile, inSourceFileModDate, inInstanceName,
						(GRtElementArray *) element, &characterClass->impacts);
		UUmError_ReturnOnError(error);
	} else {
		characterClass->impacts = NULL;
	}

	error = GRrGroup_GetString(inGroup, "class_impact_modifier", &modifierName);
	if (error != UUcError_None) {
		modifierName = "Medium";
	}

	// get the animation-impacts tag
	error = GRrGroup_GetElement(inGroup, "anim_impacts", &elemType, &element);
	UUrMemory_Clear(&characterClass->anim_impacts, sizeof(characterClass->anim_impacts));

	if (error == UUcError_None) {
		if (elemType != GRcElementType_Array) {
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "'impacts' field is not an array!");
		}

		error = IMPiParseCharacterAnimationImpacts(inSourceFile, inSourceFileModDate, inInstanceName,
						modifierName, (GRtElementArray *) element, &characterClass->anim_impacts);
		UUmError_ReturnOnError(error);
	}

	// get a reference to the template
	error = TMrConstruction_Instance_GetPlaceHolder(ONcTemplate_CharacterVariant, variantName, (TMtPlaceHolder*)&(characterClass->variant));
	UUmError_ReturnOnErrorMsg(error, "could not set variant placeholder");

	// get the death particle effect
	error = GRrGroup_GetString(inGroup, "death_effect", &particleClass);
	if (error != UUcError_None) {
		particleClass = "";
	}
	UUrString_Copy(characterClass->death_effect, particleClass, P3cParticleClassNameLength + 1);
	characterClass->death_particleclass = NULL;
	characterClass->body_surface = NULL;

	// get the death delete delay
	error = GRrGroup_GetUns16(inGroup, "death_deleteframes", &characterClass->death_deletedelay);
	if (error != UUcError_None) {
		characterClass->death_deletedelay = 0;
	}

	{
		static const char *resistance_names[P3cEnumDamageType_Max] = {"normal", "minorstun", "majorstun", "minorknockdown", "majorknockdown", "blownup", "pickup"};
		UUtUns32 itr;

		// get the damage resistances
		for (itr = 0; itr < P3cEnumDamageType_Max; itr++) {
			sprintf(keyname, "resist_%s", resistance_names[itr]);
			error = GRrGroup_GetFloat(inGroup, keyname, &characterClass->damageResistance[itr]);
			if (error != UUcError_None) {
				characterClass->damageResistance[itr] = 0;
			}
		}
	}

	/*
	 * pain constants
	 */

	pain_constants = &characterClass->painConstants;

	error = GRrGroup_GetUns16(inGroup, "hurt_base_percentage", &pain_constants->hurt_base_percentage);
	if (error != UUcError_None) { pain_constants->hurt_base_percentage = 30; }

	error = GRrGroup_GetUns16(inGroup, "hurt_max_percentage", &pain_constants->hurt_max_percentage);
	if (error != UUcError_None) { pain_constants->hurt_max_percentage = 60; }

	error = GRrGroup_GetUns16(inGroup, "hurt_percentage_threshold", &pain_constants->hurt_percentage_threshold);
	if (error != UUcError_None) { pain_constants->hurt_percentage_threshold = 30; }

	error = GRrGroup_GetUns16(inGroup, "hurt_timer", &pain_constants->hurt_timer);
	if (error != UUcError_None) { pain_constants->hurt_timer = 180; }

	error = GRrGroup_GetUns16(inGroup, "hurt_min_timer", &pain_constants->hurt_min_timer);
	if (error != UUcError_None) { pain_constants->hurt_timer = 30; }

	error = GRrGroup_GetUns16(inGroup, "hurt_max_light", &pain_constants->hurt_max_light);
	if (error != UUcError_None) { pain_constants->hurt_max_light = 1; }

	error = GRrGroup_GetUns16(inGroup, "hurt_max_medium", &pain_constants->hurt_max_medium);
	if (error != UUcError_None) { pain_constants->hurt_max_medium = 2; }

	error = GRrGroup_GetUns16(inGroup, "hurt_death_chance", &pain_constants->hurt_death_chance);
	if (error != UUcError_None) { pain_constants->hurt_death_chance = 35; }

	error = GRrGroup_GetUns16(inGroup, "hurt_volume_threshold", &pain_constants->hurt_volume_threshold);
	if (error != UUcError_None) { pain_constants->hurt_volume_threshold = 15; }

	error = GRrGroup_GetFloat(inGroup, "hurt_min_volume", &pain_constants->hurt_min_volume);
	if (error != UUcError_None) { pain_constants->hurt_min_volume = 0.3f; }

	error = GRrGroup_GetUns16(inGroup, "hurt_medium_threshold", &pain_constants->hurt_medium_threshold);
	if (error != UUcError_None) { pain_constants->hurt_medium_threshold = 10; }

	error = GRrGroup_GetUns16(inGroup, "hurt_heavy_threshold", &pain_constants->hurt_heavy_threshold);
	if (error != UUcError_None) { pain_constants->hurt_heavy_threshold = 20; }

	error = GRrGroup_GetString(inGroup, "hurt_light", &soundName);
	if (error != UUcError_None) { soundName = ""; }
	UUrString_Copy(pain_constants->hurt_light_name, soundName, SScMaxNameLength);

	error = GRrGroup_GetString(inGroup, "hurt_medium", &soundName);
	if (error != UUcError_None) { soundName = ""; }
	UUrString_Copy(pain_constants->hurt_medium_name, soundName, SScMaxNameLength);

	error = GRrGroup_GetString(inGroup, "hurt_heavy", &soundName);
	if (error != UUcError_None) { soundName = ""; }
	UUrString_Copy(pain_constants->hurt_heavy_name, soundName, SScMaxNameLength);

	error = GRrGroup_GetString(inGroup, "death_sound", &soundName);
	if (error != UUcError_None) { soundName = ""; }
	UUrString_Copy(pain_constants->death_sound_name, soundName, SScMaxNameLength);

	pain_constants->sounds_found = UUcFalse;
	pain_constants->hurt_light = NULL;
	pain_constants->hurt_medium = NULL;
	pain_constants->hurt_heavy = NULL;
	pain_constants->death_sound = NULL;

	GRrGroup_Context_Delete(context);

	return UUcError_None;
}


UUtError
Imp_AddFacingTable(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	TRtFacingTable *facingTable;
	UUtError error;
	UUtBool buildInstance;

	UUtUns16				itr;
	UUtUns16				tableSize;
	UUtUns16				numAnimStates;
	GRtElementType			elementType;
	GRtElementArray*		facingList;
	
	buildInstance = !TMrConstruction_Instance_CheckExists(TRcTemplate_FacingTable, inInstanceName);
	
	if (!buildInstance)
	{
		return UUcError_None;
	}

	//
	// build the table of joy
	//

	// open our group array
	error = GRrGroup_GetElement(
				inGroup, 
				"facingList", 
				&elementType, 
				&facingList);

	if (elementType != GRcElementType_Array) {
		error = UUcError_Generic;
	}
	UUmError_ReturnOnErrorMsg(error, "Could not get facing list group");

	tableSize = (UUtUns16) GRrGroup_Array_GetLength(facingList);
	if (GRrGroup_Array_GetLength(facingList) > UUcMaxUns16) {
		
	}
	UUmError_ReturnOnErrorMsg(error, "facing list to long");

	numAnimStates = iNumAnimStates();
	if (tableSize != numAnimStates) {
		AUrMessageBox(AUcMBType_OK, "facing table has %d entries but there are %d states.", tableSize, numAnimStates);
	}
	UUmAssert(numAnimStates >= tableSize);

	// create our instance
	error = TMrConstruction_Instance_Renew(
				TRcTemplate_FacingTable,
				inInstanceName,
				numAnimStates,
				&facingTable);
	UUmError_ReturnOnErrorMsg(error, "Failed to renew facing table");

	facingTable->tableSize = tableSize;

	for(itr = 0; itr < numAnimStates; itr++)
	{
		facingTable->facing[itr] = 0.f;
	}

	// read the data
	for(itr = 0; itr < tableSize; itr++)
	{
		GRtGroup		*tableElement;
		char *animStateString;
		float facing;
		UUtUns16 animState;

		error = GRrGroup_Array_GetElement(
			facingList,
			itr,
			&elementType,
			&tableElement);

		error = GRrGroup_GetFloat(tableElement, "facing", &facing);
		UUmError_ReturnOnErrorMsg(error, "Failed to get facing table float");

		error = GRrGroup_GetString(tableElement, "state", &animStateString);
		UUmError_ReturnOnErrorMsg(error, "Failed to get facing table state");

		error = iStringToAnimState(animStateString, &animState);
		if (UUcError_None != error) {
			AUrMessageBox(AUcMBType_OK, "could not convert %s into a state.", animStateString);
			animState = itr;
		}

		if (itr != animState) {
			AUrMessageBox(AUcMBType_OK, "table entry #%d offset from state %s (%d).", itr, animStateString, animState);
		}

		if ((facing < 0) || (facing >= M3c2Pi)) {
			AUrMessageBox(AUcMBType_OK, "facing %f out of legal 0 to 2 Pi range.", facing);
			facing = 0.f;
		}

		facingTable->facing[itr] = facing;

		if (elementType != GRcElementType_Group) {
			error = UUcError_Generic;
		}
		UUmError_ReturnOnErrorMsg(error, "could not get facing table entry");
	}

	return UUcError_None;
}


static UUtError MakeStringArray(GRtGroup *inGroup, UUtBool inBuild, char *inInstanceName, TMtStringArray **outString)
{
	UUtError				error;
	UUtUns16				itr;
	GRtElementType			elementType;

	UUtUns16				numStrings;
	GRtElementArray*		stringList;
	
	// open our group array
	error = GRrGroup_GetElement(
				inGroup, 
				"stringList", 
				&elementType, 
				&stringList);

	if (elementType != GRcElementType_Array) {
		error = UUcError_Generic;
	}
	UUmError_ReturnOnErrorMsg(error, "Could not get string list group");

	numStrings = (UUtUns16) GRrGroup_Array_GetLength(stringList);
	if (GRrGroup_Array_GetLength(stringList) > UUcMaxUns16)
	{
		error = UUcError_Generic;
	}
	UUmError_ReturnOnErrorMsg(error, "string list to long");

	// create our instance
	if (inBuild) {
		error = TMrConstruction_Instance_Renew(
					TMcTemplate_StringArray,
					inInstanceName,
					numStrings,
					(void **) outString);
		UUmError_ReturnOnErrorMsg(error, "Failed to renew string list");
	}
	else {
		// i used malloc because it is supposed to leak, not really a long term fix
		*outString = UUrMemory_Block_New(sizeof(TMtStringArray) + (numStrings * sizeof(TMtString)));
		UUmError_ReturnOnNull(*outString);
	}

	(*outString)->numStrings = numStrings;

	// read the data
	for(itr = 0; itr < numStrings; itr++)
	{
		TMtString		*stringTemplate;
		char *			string;

		error = GRrGroup_Array_GetElement(
			stringList,
			itr,
			&elementType,
			&string);

		if (elementType != GRcElementType_String) {
			error = UUcError_Generic;
		}
		UUmError_ReturnOnErrorMsg(error, "could not get string list entry");

		Imp_PrintMessage(IMPcMsg_Cosmetic, "%s"UUmNL, string);

		if (inBuild) {
			error = TMrConstruction_Instance_NewUnique(
					TMcTemplate_String,
					0,
					&stringTemplate);
			UUmError_ReturnOnErrorMsg(error, "Failed to create template for string"UUmNL);
		}
		else {
			stringTemplate = UUrMemory_Block_New(sizeof(TMtString));
			UUmError_ReturnOnNull(stringTemplate);
		}

		UUrString_Copy(stringTemplate->string, string, 128);

		(*outString)->string[itr] = stringTemplate;
	}

	return UUcError_None;
}

UUtError
Imp_AddStringList(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError				error;
	TMtStringArray			*outStrings;
	UUtBool					buildInstance;
	
	
	buildInstance = !TMrConstruction_Instance_CheckExists(TMcTemplate_StringArray, inInstanceName);

	if (buildInstance)
	{	
		error = MakeStringArray(inGroup, UUcTrue, inInstanceName, &outStrings);
	}

	return error;
}

static UUtError LoadStringTable(const char *inPath, TMtStringArray **outArray)
{
	GRtGroup			*listGroup;
	GRtGroup_Context	*context;
	UUtError error;
	TMtStringArray *array = NULL;
	BFtFileRef *fileRef;
	
	error = BFrFileRef_MakeFromName(inPath, &fileRef);
	UUmError_ReturnOnErrorMsg(error, "could not open shortcut flags");

	if (!BFrFileRef_FileExists(fileRef)) { 
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "could not open shortcut flags file");
	}

	GRrGroup_Context_NewFromFileRef(fileRef, gPreferencesGroup, NULL, &context, &listGroup);

	error = MakeStringArray(listGroup, UUcFalse, NULL, outArray);
	IMPmError_ReturnOnError(error);

	GRrGroup_Context_Delete(context);
	BFrFileRef_Dispose(fileRef);

	return UUcError_None;
}

static UUtError iLoadTables(void)
{
	UUtError			error;
	char				*dataFileDir;

	char			path[512];

	error = GRrGroup_GetString(gPreferencesGroup, "DataFileDir", &dataFileDir);
	IMPmError_ReturnOnError(error);

	if (NULL == gStringToAnimTypeTable)
	{
		sprintf(path, "%s\\stubs\\anim_types_ins.txt", dataFileDir);
		error = LoadStringTable(path, &gStringToAnimTypeTable);
		UUmError_ReturnOnErrorMsg(error, "could not open anim types");
	}

	if (NULL == gStringToAnimStateTable)
	{
		sprintf(path, "%s\\stubs\\anim_states_ins.txt", dataFileDir);
		error = LoadStringTable(path, &gStringToAnimStateTable);
		UUmError_ReturnOnErrorMsg(error, "could not open anim states");
	}

	if (NULL == gStringToAnimVarientTable)
	{
		sprintf(path, "%s\\stubs\\anim_varients_ins.txt", dataFileDir);
		error = LoadStringTable(path, &gStringToAnimVarientTable);
		UUmError_ReturnOnErrorMsg(error, "could not open anim varients");
	}

	if (NULL == gStringToAnimFlagTable)
	{
		sprintf(path, "%s\\stubs\\anim_flags_ins.txt", dataFileDir);
		error = LoadStringTable(path, &gStringToAnimFlagTable);
		UUmError_ReturnOnErrorMsg(error, "could not open anim flags");
	}

	if (NULL == gStringToShortcutFlagTable)
	{
		sprintf(path, "%s\\stubs\\shortcut_flags_ins.txt", dataFileDir);
		error = LoadStringTable(path, &gStringToShortcutFlagTable);
		UUmError_ReturnOnErrorMsg(error, "could not open shortcut flags");
	}

	if (NULL == gStringToAttackFlagTable)
	{
		sprintf(path, "%s\\stubs\\attack_flags_ins.txt", dataFileDir);
		error = LoadStringTable(path, &gStringToAttackFlagTable);
		UUmError_ReturnOnErrorMsg(error, "could not open attack flags");
	}

	if (NULL == gStringToBodyFlagTable)
	{
		sprintf(path, "%s\\stubs\\body_flags_ins.txt", dataFileDir);
		error = LoadStringTable(path, &gStringToBodyFlagTable);
		UUmError_ReturnOnErrorMsg(error, "could not open body flags");
	}
	
	return UUcError_None;
}



static UUtError iParamListToVarients(const char *varientStr, UUtUns16 *varients)
{
	UUtError error;
	UUtUns32 tempVarients;
	
	UUmAssert(gStringToAnimVarientTable);	// should have been loaded by init

	error = AUrParamListToBitfield(varientStr, gStringToAnimVarientTable, &tempVarients);

	// varients are stored in reverse order
	*varients = AUrBits16_Reverse((UUtUns16) tempVarients);

	return error;
}

static UUtError iParamListToAnimFlags(const char *inFlagStr, UUtUns32 *outFlags)
{
	UUtError error;
	
	UUmAssert(gStringToAnimFlagTable);

	error = AUrParamListToBitfield(inFlagStr, gStringToAnimFlagTable, outFlags);

	return error;
}

static UUtError iParamListToShortcutFlags(const char *inFlagStr, UUtUns32 *outFlags)
{
	UUtError error;
	
	UUmAssert(gStringToShortcutFlagTable);

	error = AUrParamListToBitfield(inFlagStr, gStringToShortcutFlagTable, outFlags);

	return error;
}

static UUtError iParamListToAttackFlags(const char *inFlagStr, UUtUns32 *outFlags)
{
	UUtError error;
	
	UUmAssert(gStringToAttackFlagTable);

	error = AUrParamListToBitfield(inFlagStr, gStringToAttackFlagTable, outFlags);

	return error;
}

static UUtError iParamListToBodyFlags(const char *inFlagStr, UUtUns32 *outFlags)
{
	UUtError error;
	
	UUmAssert(gStringToAttackFlagTable);

	error = AUrParamListToBitfield(inFlagStr, gStringToBodyFlagTable, outFlags);

	return error;
}


UUtError IMPrStringToAnimType(const char *inString, TRtAnimType *result)
{
	UUtError error = UUcError_None;
	UUtInt16 lookup;
	
	UUmAssert(gStringToAnimTypeTable);

	lookup = AUrLookupString(inString, gStringToAnimTypeTable);

	if (AUcNoSuchString == lookup) {
		error = UUcError_Generic;
	}
	else {
		*result = (TRtAnimType) lookup;
	}

	return error;
}

static char *iStateToString(TRtAnimType type)
{
	
	UUmAssert(gStringToAnimStateTable);

	UUmAssert(type < gStringToAnimStateTable->numStrings);

	return gStringToAnimStateTable->string[type]->string;
}

UUtError iStringToAnimState(const char *inString, TRtAnimState *result)
{
	UUtError error = UUcError_None;
	UUtInt16 lookup;
	
	UUmAssert(gStringToAnimStateTable);

	lookup = AUrLookupString(inString, gStringToAnimStateTable);

	if (AUcNoSuchString == lookup) {
		error = UUcError_Generic;
	}
	else {
		*result = (TRtAnimState) lookup;
	}

	return error;
}

static UUtUns16 iNumAnimStates(void)
{
	UUtError error = UUcError_None;
	
	UUmAssert(gStringToAnimStateTable);

	return gStringToAnimStateTable->numStrings;
}

static void iDisposeStringArray(TMtStringArray **ioStringArray)
{
	TMtStringArray *stringArray;
	UUtUns16 itr;

	if (NULL == *ioStringArray) {
		return;
	}

	stringArray = *ioStringArray;

	for(itr = 0; itr < stringArray->numStrings; itr++)
	{
		UUrMemory_Block_Delete(stringArray->string[itr]);
	}

	UUrMemory_Block_Delete(stringArray);

	*ioStringArray = NULL;

	return;
}

UUtError
Imp_AddAimingScreen(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError error;
	TRtAimingScreen aimingScreen;
	UUtBool buildInstance;

	char	*animationTag;
	
	buildInstance = !TMrConstruction_Instance_CheckExists(TRcTemplate_AimingScreen, inInstanceName);
	
	if (!buildInstance)
	{
		return UUcError_None;
	}

	// animation
	error =	GRrGroup_GetString(inGroup, "animation", &animationTag);
	UUmError_ReturnOnErrorMsg(error, "could not find animationTag");

	if (0 == strcmp(animationTag, "")) {
		AUrMessageBox(AUcMBType_OK, "animationTag should not be blank");
	}

	error = TMrConstruction_Instance_GetPlaceHolder(TRcTemplate_Animation, animationTag, (TMtPlaceHolder*)&(aimingScreen.animation));
	UUmError_ReturnOnErrorMsg(error, "could not find animation");

	// frame counts
	error = GRrGroup_GetInt16(inGroup, "negativeYawCount", &(aimingScreen.negativeYawFrameCount));
	UUmError_ReturnOnErrorMsg(error, "could not find negativeYawFrameCount");

	error = GRrGroup_GetInt16(inGroup, "positiveYawCount", &(aimingScreen.positiveYawFrameCount));
	UUmError_ReturnOnErrorMsg(error, "could not find positiveYawFrameCount");

	error = GRrGroup_GetInt16(inGroup, "negativePitchCount", &(aimingScreen.negativePitchFrameCount));
	UUmError_ReturnOnErrorMsg(error, "could not find negativePitchFrameCount");

	error = GRrGroup_GetInt16(inGroup, "positivePitchCount", &(aimingScreen.positivePitchFrameCount));
	UUmError_ReturnOnErrorMsg(error, "could not find positivePitchFrameCount");

	// deltas
	error = GRrGroup_GetFloat(inGroup, "negativeYawDelta", &(aimingScreen.negativeYawDelta));
	UUmError_ReturnOnErrorMsg(error, "could not find negativeYawFrameCount");

	error = GRrGroup_GetFloat(inGroup, "positiveYawDelta", &(aimingScreen.positiveYawDelta));
	UUmError_ReturnOnErrorMsg(error, "could not find positiveYawFrameCount");

	error = GRrGroup_GetFloat(inGroup, "negativePitchDelta", &(aimingScreen.negativePitchDelta));
	UUmError_ReturnOnErrorMsg(error, "could not find negativePitchFrameCount");

	error = GRrGroup_GetFloat(inGroup, "positivePitchDelta", &(aimingScreen.positivePitchDelta));
	UUmError_ReturnOnErrorMsg(error, "could not find positivePitchFrameCount");

	aimingScreen.negativeYawDelta *= M3cDegToRad;
	aimingScreen.positiveYawDelta *= M3cDegToRad;
	aimingScreen.negativePitchDelta *= M3cDegToRad;
	aimingScreen.positivePitchDelta *= M3cDegToRad;

	// create the instance

	{
		TRtAimingScreen *newAimingScreen;

		error = TMrConstruction_Instance_Renew(
				TRcTemplate_AimingScreen		,
				inInstanceName,
				0,								// variable length array whoha
				&newAimingScreen);
		UUmError_ReturnOnErrorMsg(error, "failed to create the aiming scren");

		UUrMemory_MoveFast(&aimingScreen, newAimingScreen, sizeof(TRtAimingScreen));
	}


	return UUcError_None;
}

UUtError
Imp_AddSavedFilm(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError error;
	ONtFilm_DiskFormat *filmData;
	ONtFilmKey *filmDataKeys;
	ONtFilm *newFilm;
	BFtFileMapping *mappingRef;
	BFtFileRef *fileRef;
	UUtUns32 fileSize;
	UUtBool buildInstance;
	char *specialAnim0;
	char *specialAnim1;
	char *basePtr;

	char *fileName;

	buildInstance = !TMrConstruction_Instance_CheckExists(ONcTemplate_Film, inInstanceName);

	if (!buildInstance) {
		return UUcError_None;
	}

	error =	GRrGroup_GetString(inGroup,	"file",	&fileName);
	UUmError_ReturnOnError(error);

	error = BFrFileRef_DuplicateAndReplaceName(inSourceFile, fileName, &fileRef);
	if (BFcError_FileNotFound == error) { error = UUcError_None; }
	UUmError_ReturnOnError(error);

	if (!BFrFileRef_FileExists(fileRef)) {
		Imp_PrintWarning("could not find file %s", BFrFileRef_GetLeafName(fileRef));
		return UUcError_Generic;
	}

	error = BFrFile_Map(fileRef, 0, &mappingRef, &basePtr, &fileSize);
	UUmError_ReturnOnError(error);

	specialAnim0 = basePtr;
	basePtr += 128;

	specialAnim1 = basePtr;
	basePtr += 128;

	filmData = (ONtFilm_DiskFormat *) basePtr;
	basePtr += sizeof(ONtFilm_DiskFormat);

	filmDataKeys = (ONtFilmKey *) basePtr;

	ONrFilm_Swap_DiskFormat(filmData, filmDataKeys);

	error = TMrConstruction_Instance_Renew(ONcTemplate_Film, inInstanceName, filmData->numKeys,	&newFilm);
	IMPmError_ReturnOnError(error);

	filmData->special_anim[0] = NULL;
	filmData->special_anim[1] = NULL;

	if (specialAnim0[0] != '\0') {
		error = TMrConstruction_Instance_GetPlaceHolder(TRcTemplate_Animation, specialAnim0, (TMtPlaceHolder*) &(newFilm->special_anim[0]));
		UUmError_ReturnOnError(error);

		Imp_PrintMessage(IMPcMsg_Important, "special animation 0: %s %x" UUmNL, specialAnim0, newFilm->special_anim[0]);
	}

	if (specialAnim1[0] != '\0') {
		error = TMrConstruction_Instance_GetPlaceHolder(TRcTemplate_Animation, specialAnim1, (TMtPlaceHolder*) &(newFilm->special_anim[1]));
		UUmError_ReturnOnError(error);

		Imp_PrintMessage(IMPcMsg_Important, "special animation 1: %s %x" UUmNL, specialAnim1, newFilm->special_anim[1]);
	}

	newFilm->initialAimingLR = filmData->initialAimingLR;
	newFilm->initialAimingUD = filmData->initialAimingUD;
	newFilm->initialDesiredFacing = filmData->initialDesiredFacing;
	newFilm->initialFacing = filmData->initialFacing;
	newFilm->initialLocation = filmData->initialLocation;
	newFilm->length = filmData->length;
	newFilm->numKeys = filmData->numKeys;
	//newFilm->pad2 = 0;
	//newFilm->pad3 = 0;
	//newFilm->pad4 = 0;

	UUrMemory_MoveFast(
		filmDataKeys, 
		newFilm->keys, 
		filmData->numKeys * sizeof(ONtFilmKey));

	BFrFile_UnMap(mappingRef);
	BFrFileRef_Dispose(fileRef);

	return UUcError_None;
}

typedef struct AnimCacheEntry
{
	TRtAnimation animation;
	UUtUns32 modTime;
	UUtUns32 dataIndex;
} AnimCacheEntry;

typedef struct AnimCacheHeader
{
	AnimCacheEntry entries[1024];
	UUtUns32 numEntries;
	UUtUns32 curWritePtr;
} AnimCacheHeader;

void Imp_OpenAnimCache(BFtFileRef *inFileRef)
{

	return;
}

void Imp_CloseAnimCache(void)
{

	return;
}

UUtError
Imp_AddAnimCache(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError error;
	BFtFileRef *cacheRef;
	char *name;

	error =	GRrGroup_GetString(inGroup,	"file",	&name);
	UUmError_ReturnOnError(error);

	error = BFrFileRef_DuplicateAndReplaceName(inSourceFile, name, &cacheRef);
	UUmError_ReturnOnError(error);

	Imp_OpenAnimCache(cacheRef);

	BFrFileRef_Dispose(cacheRef);
	
	return UUcError_None;
}

UUtError
Imp_AddVariant(
	BFtFileRef			*inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup			*inGroup,
	char				*inInstanceName)
{
	UUtError			error;

	UUtBool				build_instance;
	
	
	// check to see if the dialogs need to be built
	build_instance = !TMrConstruction_Instance_CheckExists(ONcTemplate_VariantList,	inInstanceName);
	
	if (build_instance)
	{
		char					*variant_name;
		char					*upgrade_name;
		char					*parent_name;
		ONtCharacterVariant		*variant;
		
		// get the name of the variant
		error = GRrGroup_GetString(inGroup, "name", &variant_name);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get variant name");
		
		// get the name of the upgraded variant
		error = GRrGroup_GetString(inGroup, "upgrade_name", &upgrade_name);
		if (error != UUcError_None) { upgrade_name = ""; }
		
		// get the name of the parent
		error = GRrGroup_GetString(inGroup, "parent", &parent_name);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get parent name");
		
		// build an instance of the variant name list
		error =
			TMrConstruction_Instance_Renew(
				ONcTemplate_CharacterVariant,
				inInstanceName,
				0,
				&variant);
		IMPmError_ReturnOnErrorMsg(error, "Unable to create dialog data instance");
		
		// copy the variant names into the variant
		UUrString_Copy(variant->name, variant_name, ONcMaxVariantNameLength);
		UUrString_Copy(variant->upgrade_name, upgrade_name, ONcMaxVariantNameLength);
		
		// set the parent
		if (UUrString_Compare_NoCase(parent_name, "none") != 0)
		{
			// get an instance placeholder for the parent
			error =
				TMrConstruction_Instance_GetPlaceHolder(
					ONcTemplate_CharacterVariant,
					parent_name,
					(TMtPlaceHolder*)&(variant->parent));
			UUmError_ReturnOnErrorMsg(error, "could not set variant's parent placeholder");
		}
		else
		{
			variant->parent = NULL;
		}
	}
	
	return UUcError_None;
}

UUtError
Imp_AddVariantList(
	BFtFileRef			*inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup			*inGroup,
	char				*inInstanceName)
{
	UUtError			error;

	UUtBool				build_instance;
		
	// check to see if the dialogs need to be built
	build_instance = !TMrConstruction_Instance_CheckExists(ONcTemplate_VariantList, inInstanceName);
	
	if (build_instance)
	{
		GRtElementType			element_type;
		GRtElementArray			*variants_array;
		UUtUns32				num_items;
		ONtVariantList			*variant_list;
		UUtUns32				i;

		error =
			GRrGroup_GetElement(
				inGroup,
				"variants",
				&element_type,
				&variants_array);
		if ((error != UUcError_None) || (element_type != GRcElementType_Array))
		{
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to get variants array");
		}
		
		num_items = GRrGroup_Array_GetLength(variants_array);
		if (num_items == 0)
		{
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "The variants array is empty");
		}
		
		// build an instance of the variant name list
		error =
			TMrConstruction_Instance_Renew(
				ONcTemplate_VariantList,
				inInstanceName,
				num_items,
				&variant_list);
		IMPmError_ReturnOnErrorMsg(error, "Unable to create variant list instance");
		
		for (i = 0; i < num_items; i++)
		{
			char				*variant_name;
			
			// get the ith item from the variants array
			error =
				GRrGroup_Array_GetElement(
					variants_array,
					i,
					&element_type,
					&variant_name);
			IMPmError_ReturnOnErrorMsg(error, "unable to get variant name from array");
			
			if (element_type != GRcElementType_String)
			{
				IMPmError_ReturnOnErrorMsg(error, "variant array item was not a string");
			}
			
			// get a reference to the template
			error =
				TMrConstruction_Instance_GetPlaceHolder(
					ONcTemplate_CharacterVariant,
					variant_name,
					(TMtPlaceHolder*)&(variant_list->variant[i]));
			UUmError_ReturnOnErrorMsg(error, "could not set variant placeholder");
		}
	}
	
	return UUcError_None;
}

static void AnimCache_New(BFtFileRef *inFileRef)
{

}

UUtError
Imp_Character_Initialize(
	void)
{
	UUtError	error;
	
	error = iLoadTables();
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

void Imp_Character_Terminate(void)
{
	if(gStringToAnimTypeTable != NULL) iDisposeStringArray(&gStringToAnimTypeTable);
	if(gStringToAnimStateTable != NULL) iDisposeStringArray(&gStringToAnimStateTable);
	if(gStringToAnimVarientTable != NULL) iDisposeStringArray(&gStringToAnimVarientTable);
	if(gStringToAnimFlagTable != NULL) iDisposeStringArray(&gStringToAnimFlagTable);
	if(gStringToShortcutFlagTable != NULL) iDisposeStringArray(&gStringToShortcutFlagTable);
	if(gStringToAttackFlagTable != NULL) iDisposeStringArray(&gStringToAttackFlagTable);
	if(gStringToBodyFlagTable != NULL) iDisposeStringArray(&gStringToBodyFlagTable);

	return;
}
