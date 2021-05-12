/**********
* Oni_Combos.c
*
* Hand to hand combo lookup tables
*
* c1998 Bungie
*/


#include "BFW.h"
#include "BFW_Totoro.h"
#include "BFW_LocalInput.h"

#include "Oni_AI.h"
#include "Oni_Combos.h"
#include "Oni_Character_Animation.h"

ONtMoveLookup ONgComboTable[] = {
		// 2 down mask w/ 1 move combos

		// 1 down mask w/ 3 move combos
		{ LIc_BitMask_Kick,		LIc_BitMask_Forward,	ONcAnimType_Kick2,		ONcAnimType_Kick3_Forward, UUcTrue },

		// 1 down mask w/ 2 moves combos

		// 1 down mask w/ 1 moves combo getup moves
		{ LIc_BitMask_Kick,		LIc_BitMask_Backward,	ONcAnimType_None,	ONcAnimType_Getup_Kick_Back, UUcTrue },
		
		// 1 down mask w/ 1 moves combo
		{ LIc_BitMask_Punch,	LIc_BitMask_Forward,	ONcAnimType_None,	ONcAnimType_Punch_Forward, UUcTrue },
		{ LIc_BitMask_Punch,	LIc_BitMask_StepLeft,	ONcAnimType_None,	ONcAnimType_Punch_Left, UUcTrue },
		{ LIc_BitMask_Punch,	LIc_BitMask_StepRight,	ONcAnimType_None,	ONcAnimType_Punch_Right, UUcTrue },
		{ LIc_BitMask_Punch,	LIc_BitMask_Backward,	ONcAnimType_None,	ONcAnimType_Punch_Back, UUcTrue },
		{ LIc_BitMask_Punch,	LIc_BitMask_Crouch,		ONcAnimType_None,	ONcAnimType_Punch_Low, UUcTrue },

		{ LIc_BitMask_Kick,		LIc_BitMask_Forward,	ONcAnimType_None,	ONcAnimType_Kick_Forward, UUcTrue },
		{ LIc_BitMask_Kick,		LIc_BitMask_StepLeft,	ONcAnimType_None,	ONcAnimType_Kick_Left, UUcTrue },
		{ LIc_BitMask_Kick,		LIc_BitMask_Backward,	ONcAnimType_None,	ONcAnimType_Kick_Back, UUcTrue },
		{ LIc_BitMask_Kick,		LIc_BitMask_StepRight,	ONcAnimType_None,	ONcAnimType_Kick_Right, UUcTrue },
		{ LIc_BitMask_Kick,		LIc_BitMask_Crouch,		ONcAnimType_None,	ONcAnimType_Kick_Low, UUcTrue },

		{ LIc_BitMask_Crouch,	LIc_BitMask_Forward,	ONcAnimType_None,	ONcAnimType_Crouch_Forward, UUcTrue },
		{ LIc_BitMask_Crouch,	LIc_BitMask_Backward,	ONcAnimType_None,	ONcAnimType_Crouch_Back, UUcTrue },
		{ LIc_BitMask_Crouch,	LIc_BitMask_StepLeft,	ONcAnimType_None,	ONcAnimType_Crouch_Left, UUcTrue },
		{ LIc_BitMask_Crouch,	LIc_BitMask_StepRight,	ONcAnimType_None,	ONcAnimType_Crouch_Right, UUcTrue },

		{ LIc_BitMask_Forward,	LIc_BitMask_Crouch,		ONcAnimType_None,	ONcAnimType_Crouch_Forward, UUcTrue },
		{ LIc_BitMask_Backward,	LIc_BitMask_Crouch,		ONcAnimType_None,	ONcAnimType_Crouch_Back, UUcTrue },
		{ LIc_BitMask_StepLeft,	LIc_BitMask_Crouch,		ONcAnimType_None,	ONcAnimType_Crouch_Left, UUcTrue },
		{ LIc_BitMask_StepRight,LIc_BitMask_Crouch,		ONcAnimType_None,	ONcAnimType_Crouch_Right, UUcTrue },

		// running moves
		{ LIc_BitMask_Punch,	LIc_BitMask_Forward,	ONcAnimType_None,	ONcAnimType_Run_Punch, UUcFalse },
		{ LIc_BitMask_Kick,		LIc_BitMask_Forward,	ONcAnimType_None,	ONcAnimType_Run_Kick, UUcFalse },

		{ LIc_BitMask_Punch,	LIc_BitMask_Backward,	ONcAnimType_None,	ONcAnimType_Run_Back_Punch, UUcFalse },
		{ LIc_BitMask_Kick,		LIc_BitMask_Backward,	ONcAnimType_None,	ONcAnimType_Run_Back_Kick, UUcFalse },

		{ LIc_BitMask_Punch,	LIc_BitMask_StepLeft,	ONcAnimType_None,	ONcAnimType_Sidestep_Left_Punch, UUcFalse },
		{ LIc_BitMask_Kick,		LIc_BitMask_StepLeft,	ONcAnimType_None,	ONcAnimType_Sidestep_Left_Kick, UUcFalse },

		{ LIc_BitMask_Punch,	LIc_BitMask_StepRight,	ONcAnimType_None,	ONcAnimType_Sidestep_Right_Punch, UUcFalse },
		{ LIc_BitMask_Kick,		LIc_BitMask_StepRight,	ONcAnimType_None,	ONcAnimType_Sidestep_Right_Kick, UUcFalse },

		// 0 down mask w/ 6 move combos
		{ LIc_BitMask_Kick,		0,						ONcAnimType_PPKKK,	ONcAnimType_PPKKKK, UUcFalse },

		// 0 down mask w/ 5 move combos
		{ LIc_BitMask_Kick,		0,						ONcAnimType_PPKK,	ONcAnimType_PPKKK, UUcFalse },

		// 0 down mask w/ 4 move combos
		{ LIc_BitMask_Punch,	0,						ONcAnimType_Punch3,	ONcAnimType_Punch4, UUcFalse },
		{ LIc_BitMask_Kick,		0,						ONcAnimType_PPK,	ONcAnimType_PPKK, UUcFalse },

		// 0 down mask w/ 3 move combos
		{ LIc_BitMask_Punch,	0,						ONcAnimType_PK,		ONcAnimType_PKP, UUcFalse },
		{ LIc_BitMask_Punch,	0,						ONcAnimType_KP,		ONcAnimType_KPP, UUcFalse },
		{ LIc_BitMask_Punch,	0,						ONcAnimType_Kick2,	ONcAnimType_KKP, UUcFalse },
		{ LIc_BitMask_Punch,	0,						ONcAnimType_Punch2,	ONcAnimType_Punch3, UUcFalse },
		{ LIc_BitMask_Kick,		0,						ONcAnimType_Punch2,	ONcAnimType_PPK, UUcFalse },
		{ LIc_BitMask_Kick,		0,						ONcAnimType_PK,		ONcAnimType_PKK, UUcFalse },
		{ LIc_BitMask_Kick,		0,						ONcAnimType_KP,		ONcAnimType_KPK, UUcFalse },
		{ LIc_BitMask_Kick,		0,						ONcAnimType_Kick2,	ONcAnimType_Kick3, UUcFalse },

		// 0 down mask w/ 2 move combos
		{ LIc_BitMask_Punch,	0,						ONcAnimType_Punch,	ONcAnimType_Punch2, UUcFalse },
		{ LIc_BitMask_Kick,		0,						ONcAnimType_Kick,	ONcAnimType_Kick2, UUcFalse },
		{ LIc_BitMask_Kick,		0,						ONcAnimType_Punch,	ONcAnimType_PK, UUcFalse },
		{ LIc_BitMask_Punch,	0,						ONcAnimType_Kick,	ONcAnimType_KP, UUcFalse },

		// weapon moves
		{ LIc_BitMask_Punch,	0,						ONcAnimType_None,		ONcAnimType_Punch_Rifle, UUcFalse },

		// 0 down mask w/ 1 move combos
		{ LIc_BitMask_Punch,	0,						ONcAnimType_None,	ONcAnimType_Punch, UUcFalse },
		{ LIc_BitMask_Kick,		0,						ONcAnimType_None,	ONcAnimType_Kick, UUcFalse },
		{ LIc_BitMask_Crouch,	0,						ONcAnimType_None,	ONcAnimType_Flip, UUcFalse },

		// termination
		{ 0,					0,						ONcAnimType_None,	ONcAnimType_None, UUcFalse }
};

