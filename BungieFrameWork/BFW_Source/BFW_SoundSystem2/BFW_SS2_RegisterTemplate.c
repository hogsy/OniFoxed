// ======================================================================
// BFW_SS2_RegisterTemplate.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_TemplateManager.h"

#include "BFW_SoundSystem2.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
UUtError
SS2rRegisterTemplates(
	void)
{
	UUtError	error;

	error =	TMrTemplate_Register(SScTemplate_SoundData, sizeof(SStSoundData), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error =	TMrTemplate_Register(SScTemplate_Subtitle, sizeof(SStSubtitle), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}
