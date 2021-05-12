// ======================================================================
// Oni_ObjectGroup.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

#include "Oni_Object.h"
#include "Oni_Object_Private.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
UUtError
OBJrRegisterTemplates(
	void)
{
	UUtError				error;
	
	error =
		TMrTemplate_Register(
			OBJcTemplate_LSData,
			sizeof(OBJtLSData),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			OBJcTemplate_FurnGeomArray,
			sizeof(OBJtFurnGeomArray),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			OBJcTemplate_TriggerClass,
			sizeof(OBJtTriggerClass),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			OBJcTemplate_TriggerEmitterClass,
			sizeof(OBJtTriggerEmitterClass),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			OBJcTemplate_TurretClass,
			sizeof(OBJtTurretClass),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			OBJcTemplate_ConsoleClass,
			sizeof(OBJtConsoleClass),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			OBJcTemplate_DoorClass,
			sizeof(OBJtDoorClass),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}
