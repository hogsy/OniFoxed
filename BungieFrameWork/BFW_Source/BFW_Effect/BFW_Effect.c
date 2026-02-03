/*
	FILE:	BFW_Effects.c

	AUTHOR:	Brent Pease

	CREATED: Oct 8, 1999

	PURPOSE: effects engine

	Copyright 1999

 */

#include "BFW.h"
#include "BFW_TemplateManager.h"
#include "BFW_Motoko.h"
#include "BFW_Effect.h"
#include "BFW_BitVector.h"
#include "BFW_Effect_Private.h"
#include "BFW_Console.h"
#include "BFW_MathLib.h"


/**************************************************************************************/

UUtError
FXrRegisterTemplates(
	void)
{
	UUtError error;

	error = TMrTemplate_Register(FXcTemplate_Laser, sizeof(FXtLaser), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}
