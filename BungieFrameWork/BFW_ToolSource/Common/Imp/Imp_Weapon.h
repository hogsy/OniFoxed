/*
	FILE:	Imp_Weapon.h
	
	AUTHOR:	Michael Evans
	
	CREATED: 3/18/1998
	
	PURPOSE: 
	
	Copyright 1998 Bungie Software

*/

#ifndef IMP_WEAPON_H
#define IMP_WEAPON_H

#include "BFW.h"
#include "BFW_Group.h"

#include "Oni_AI2.h"

UUtError
Imp_AddWeapon(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError Imp_Weapon_ReadAIParameters(
	BFtFileRef*				inSourceFile,
	UUtUns32				inSourceFileModDate,
	GRtGroup*				inGroup,
	char*					inInstanceName,
	UUtBool					inAlternateParameters,
	AI2tWeaponParameters	*inParameters);

#endif