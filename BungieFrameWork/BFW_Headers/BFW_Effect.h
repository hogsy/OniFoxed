#pragma once
/*
	FILE:	BFW_Effect.h

	AUTHOR:	Brent Pease

	CREATED: Oct 8, 1999

	PURPOSE: effects engine

	Copyright 1999

 */

#ifndef BFW_EFFECTS_H
#define BFW_EFFECTS_H

#include "BFW.h"
#include "BFW_TemplateManager.h"
#include "BFW_Motoko.h"

// Laser triggers

	typedef tm_enum FXtLaser_Flags
	{
		FXcLaserFlag_None					= 0,
		FXcLaserFlag_StopsAtFirstCollision	= 1,	// otherwise it is fixed length based on geometry
		FXcLaserFlag_Procedural				= 2

	} FXtLaser_Flags;

	#define FXcTemplate_Laser	UUm4CharToUns32('F', 'X', 'L', 'R')
	typedef tm_template('F', 'X', 'L', 'R', "FX Laser effect")
	FXtLaser
	{
		FXtLaser_Flags	flags;

		M3tTextureMap*	texture;
		M3tGeometry*	geometry;

		//AItScript*		script;

	} FXtLaser;




// Functions
UUtError
FXrRegisterTemplates(
	void);

#endif
