#pragma once

#ifndef BFW_MASTERHEADER_H
#define BFW_MASTERHEADER_H


/*
	FILE:	BFW.h

	AUTHOR:	Michael Evans

	CREATED: Oct 13, 1999

	PURPOSE: Master Include File

	Copyright 1999

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
// S.S. prep for bfw_math.h #include <math.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include <stdarg.h>

#include "BFW.h"
#include "BFW_Types.h"
#include "BFW_FileManager.h"
#include "BFW_Image.h"
#include "BFW_Group.h"
#include "BFW_Motoko.h"
#include "BFW_MathLib.h"

#include "BFW_Akira.h"
#include "BFW_AppUtilities.h"
#include "BFW_BitVector.h"
#include "BFW_Collision.h"
#include "BFW_CommandLine.h"
#include "BFW_Console.h"
#include "BFW_DebuggerSymbols.h"
#include "BFW_Doors.h"
#include "BFW_Effect.h"
#include "BFW_Effect_Private.h"
#include "BFW_ErrorCodes.h"
#include "BFW_FileFormat.h"
#include "BFW_LocalInput.h"
#include "BFW_Object.h"
#include "BFW_Particle3.h"
#include "BFW_Path.h"
#include "BFW_Physics.h"
#include "BFW_ScriptLang.h"
#include "BFW_SoundSystem.h"
#include "BFW_SoundSystem2.h"
#include "BFW_TemplateManager.h"
#include "BFW_TextSystem.h"
#include "BFW_Totoro.h"
#include "BFW_WindowManager.h"

#include "gl_code_version.h"


#endif //BFW_MASTERHEADER_H
