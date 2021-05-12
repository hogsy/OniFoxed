/**********
* Oni_Combos.h
*
* Hand to hand combo lookup tables
*
* c1998 Bungie
*/

#ifndef ONI_COMBOS_H
#define ONI_COMBOS_H

#include "BFW_LocalInput.h"
#include "BFW_Totoro.h"

#include "Oni_AI.h"

#define ONcComboTypes 4
#define ONcMaxComboEntries 16

typedef struct ONtMoveLookup {
	LItButtonBits newMask;
	LItButtonBits moveMask;
	TRtAnimType oldType;
	TRtAnimType moveType;
	UUtBool backInTime;
} ONtMoveLookup;

extern ONtMoveLookup ONgComboTable[];

#endif
