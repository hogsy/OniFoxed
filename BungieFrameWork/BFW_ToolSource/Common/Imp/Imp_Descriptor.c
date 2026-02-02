// ======================================================================
// Imp_Descriptor.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_TM_Construction.h"
#include "BFW_Group.h"

#include "Imp_Descriptor.h"
#include "Imp_Common.h"
#include "Imp_Sound.h"

#include "Oni_GameStatePrivate.h"

// ======================================================================
// globals
// ======================================================================
static char*	gDescriptorCompileTime = __DATE__" "__TIME__;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
UUtError
Imp_AddLevelDescriptor(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;

	UUtBool				build_instance;


	// check to see if the dialogs need to be built
	build_instance = !TMrConstruction_Instance_CheckExists(ONcTemplate_Level_Descriptor, inInstanceName);

	if (build_instance)
	{
		ONtLevel_Descriptor		*descriptor;
		char					*level_name;

		// create a template instance
		error =
			TMrConstruction_Instance_Renew(
				ONcTemplate_Level_Descriptor,
				inInstanceName,
				0,
				&descriptor);
		IMPmError_ReturnOnErrorMsg(error, "Could not create a descriptor template");

		// default initialization
//		descriptor->default_reverb	= SScRP_None;
		descriptor->level_number	= 0;
		descriptor->level_name[0]	= '\0';

		// get the default reverb
/*		error =
			IMPrProcessReverb(
				inGroup,
				"default_reverb",
				SScRP_None,
				&descriptor->default_reverb);
		IMPmError_ReturnOnErrorMsg(error, "Could not process the default reverb");*/

		// get the level number
		error =
			GRrGroup_GetUns16(
				inGroup,
				"level_number",
				&descriptor->level_number);
		IMPmError_ReturnOnErrorMsg(error, "Could not get the descriptor's level number");

		error =
			GRrGroup_GetUns16(
				inGroup,
				"next_level_number",
				&descriptor->next_level_number);
		if (error) {
			descriptor->next_level_number = 0;
		}

		// get the name
		error =
			GRrGroup_GetString(
				inGroup,
				"level_name",
				&level_name);
		IMPmError_ReturnOnErrorMsg(error, "Could not get the descriptor's level name");

		UUrString_Copy(descriptor->level_name, level_name, ONcMaxLevelName);

	}

	return UUcError_None;
}
