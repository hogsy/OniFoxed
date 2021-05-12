// ======================================================================
// Oni_UnitViewer.h
// ======================================================================
#ifndef ONI_UNITVIEWER_H
#define ONI_UNITVIEWER_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_DialogManager.h"

// ======================================================================
// defines
// ======================================================================
#define UVcMaxDataNameLength		32
#define UVcMaxInstNameLength		32

#define ONcUnitViewer_Level			127

// ======================================================================
// enums
// ======================================================================
enum
{
	ONcDialogID_UnitViewer			= 20000,
	ONcDialogID_UnitViewerControls	= 20001
};

enum
{
	UVcDataType_None,
	UVcDataType_Character,
	UVcDataType_Weapon,
	UVcDataType_URL
	
};

// ======================================================================
// typedefs
// ======================================================================
typedef tm_struct UVtData
{
	char					instance_name[32];	/* UVcMaxInstNameLength */
	char					name[32];			/* UVcMaxDataNameLength */
	
} UVtData;

#define UVcTemplate_DataList			UUm4CharToUns32('U', 'V', 'D', 'L')
typedef tm_template('U', 'V', 'D', 'L', "UV Data List")
UVtDataList
{
	tm_pad					pad[16];
	
	UUtUns32				data_type;
	
	tm_varindex UUtUns32	num_data_entries;
	tm_vararray UVtData		data[1];
	
} UVtDataList;


// ======================================================================
// includes
// ======================================================================
UUtBool
ONrDialogCallback_UnitViewer(
	DMtDialog				*inDialog,
	DMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtError
ONrUnitViewer_RegisterTemplates(
	void);

UUtError
ONrUnitViewer_Character_Initialize(
	void);

void
ONrUnitViewer_Character_Terminate(
	void);

void
ONrUnitViewer_Character_Update(
	UUtUns16 numActionsInBuffer,
	LItAction *actionBuffer);

// ======================================================================
#endif /* ONI_UNITVIEWER_H */