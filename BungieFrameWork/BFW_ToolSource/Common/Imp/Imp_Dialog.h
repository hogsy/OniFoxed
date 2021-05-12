// ======================================================================
// Imp_Dialog.h
// ======================================================================
#ifndef IMP_DIALOG_H
#define IMP_DIALOG_H

// ======================================================================
// Includes
// ======================================================================
#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_TemplateManager.h"
#include "BFW_Group.h"
#include "BFW_DialogManager.h"
#include "BFW_AppUtilities.h"

// ======================================================================
// prototypes
// ======================================================================
UUtError
Imp_ProcessDialogFlags(
	GRtGroup			*inGroup,
	AUtFlagElement		*inFlagList,
	UUtUns16			*outFlags);
	
UUtError
Imp_ProcessTextures(
	BFtFileRef*			inSourceFile,
	GRtGroup			*inGroup,
	VMtTextureList		**inTextureList,
	char				*inTextureListName);
	
UUtError
Imp_ProcessLocation(
	GRtGroup			*inGroup,
	IMtPoint2D			*outLocation);
	
UUtError
Imp_AddView(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);
	
UUtError
Imp_AddDialogData(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);
	
UUtError
Imp_AddDialog(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddDialogTypeList(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);
	
// ----------------------------------------------------------------------
UUtError
Imp_AddControl_Button(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddControl_EditField(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);
	
UUtError
Imp_AddControl_Picture(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddControl_Tab(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);
	
UUtError
Imp_AddControl_Text(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

// ----------------------------------------------------------------------
UUtError
Imp_AddCursor(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);
	
UUtError
Imp_AddCursorTypeList(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

// ======================================================================
#endif /* IMP_DIALOG_H */