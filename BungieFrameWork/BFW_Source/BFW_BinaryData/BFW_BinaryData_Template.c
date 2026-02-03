// ======================================================================
// BFW_BinaryData_Template.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_BinaryData.h"
#include "BFW_TemplateManager.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
UUtError
BDrRegisterTemplates(
	void)
{
	UUtError	error;

	error = TMrTemplate_Register(BDcTemplate_BinaryData, sizeof(BDtBinaryData), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}
