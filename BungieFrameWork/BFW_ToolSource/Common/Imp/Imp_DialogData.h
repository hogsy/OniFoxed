// ======================================================================
// Imp_DialogData.h
// ======================================================================
#ifndef IMP_DIALOGDATA_H
#define IMP_DIALOGDATA_H

#pragma once

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

// ======================================================================
// enums
// ======================================================================
enum
{
	cInfoFont,
	cInfoSize,
	cInfoStyle,
	cInfoShade
};

// ======================================================================
// globals
// ======================================================================
extern AUtFlagElement		IMPgFontStyleTypes[];
extern AUtFlagElement		IMPgFontShadeTypes[];

// ======================================================================
// prototypes
// ======================================================================
UUtError
Imp_AddDialogData(
	BFtFileRef*				inSourceFile,
	UUtUns32				inSourceFileModDate,
	GRtGroup*				inGroup,
	char*					inInstanceName);

UUtError
IMPrProcess_FontInfo(
	void					*inData,
	GRtElementType			inElementType,
	TStFontInfo				*outFontInfo);
	
// ======================================================================
#endif /* IMP_DIALOGDATA_H */