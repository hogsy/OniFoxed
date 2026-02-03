/*
	Oni_AI.h

	Top level AI header stuff

	Author: Quinn Dunki
	c1998 Bungie
*/

#ifndef ONI_AI_H
#define ONI_AI_H

#include "BFW_LocalInput.h"

//#include "Oni_Character.h"
#include "Oni_Path.h"
#include "Oni_Weapon.h"
#include "Oni_AStar.h"



//**** Globals
extern UUtBool						AIgAIActive,AIgVerbose,AIgScript_Verbose;
extern UUtBool						AIgLogCombos;
extern UUtBool						AIgBeCute;

//**** Constants
#define AIcActivity_StackDepth		10
#define AIcBehaviour_StackDepth		5
#define AIcDefaultH2HDelay 			1
#define AIcBNVTestFudge				8.0f
#define AIcBNVTestFudge_Small		1.0f
#define AIcAutoFreezeDistance		5.0f//100.0f
#define AIcMaxWeaponName			64			// Don't change without changing refs in Oni_AI_Setup.h

#define AIcMaxTeams 32

typedef struct AItActivity AItActivity;
typedef struct AItBehaviour AItBehaviour;
typedef struct AItGroup AItGroup;

typedef struct AItHandToHand
{
	LItButtonBits keyMask;
	UUtUns32 postWait;
} AItHandToHand;


typedef enum
{
	AIcDir_Front,
	AIcDir_Back,
	AIcDir_Left,
	AIcDir_Right,
	AIcDir_Any
} AItDirection;

typedef enum
{
	AIcHHState_Stand,
	AIcHHState_Crouch,
	AIcHHState_Fallen
} AItHHState;



//********* Tactical profiles

typedef enum AItTacticalZone
{
	AIcTacticalZone_None,
	AIcTacticalZone_Brawl,
	AIcTacticalZone_Overlap,
	AIcTacticalZone_Charge
} AItTacticalZone;

typedef enum AItArmed
{
	AIcArmed_No,
	AIcArmed_Yes,
	AIcArmed_Any
} AItArmed;

//**** Pathfinding
#define AIcMax_Waypoints			256

enum
{
	AIcPath_Idle			= 0,
	AIcPath_Follow			= 1	// on if path following is enabled
};


//**** Prototypes

UUtError
AIrInitialize(
	void);

void
AIrTerminate(
	void);

UUtError AIrRegisterTemplates(
	void);

void AIrDisplay(
	void);

void AIrDisplay_Character(
	ONtCharacter *inCharacter);

UUtBool AIrH2H_ShouldBlock(
	ONtCharacter	*inCharacter);

void AIrFrameStart(
	ONtCharacter *inCharacter);

void AIrFrameEnd(
	ONtCharacter *inCharacter);

UUtError AIrInitialize_Display(
	void);

void AIrNotify_Collision_Object(
	ONtCharacter *inCharacter);

void AIrNotify_Collision_Environment(
	ONtCharacter *inCharacter);


UUtError AIrLevelBegin(
	void);

void AIrLevelEnd(
	void);

void AIrDisplay_GraphNode(
	AKtBNVNode *inNode,
	UUtUns32 inShade);


#endif
