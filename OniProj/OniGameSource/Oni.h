#pragma once
/*
	FILE:	Oni.h

	AUTHOR:	Brent H. Pease

	CREATED: Jan 10, 1998

	PURPOSE: main .c file for Oni

	Copyright 1997

*/
#ifndef ONI_H
#define ONI_H

#include "BFW_Motoko.h"
#include "BFW_LocalInput.h"
#include "BFW_Console.h"

#define ONcActionInstance	"ONIA"

#define ONcGameDataFolder1	"GameDataFolder"
#define ONcGameDataFolder2	"OniEngine\\GameDataFolder"

extern UUtBool					ONgTerminateGame;
extern UUtBool					ONgRunMainMenu;

extern BFtFileRef ONgGameDataFolder;

typedef struct
{
	TMtAllowPrivateData			allowPrivateData;
	COtReadConfigFile			readConfigFile;
	UUtBool						logCombos;
	UUtBool						filmPlayback;
	UUtBool						useOpenGL;
	UUtBool						useGlide;
	UUtBool						useSound;
} ONtCommandLine;

extern ONtCommandLine	ONgCommandLine;

#endif
