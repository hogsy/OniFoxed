
/*
	FILE:	Imp_Character.h

	AUTHOR:	Brent H. Pease, Michael Evans

	CREATED: Nov 1, 1997

	PURPOSE:

	Copyright 1997, 1998

*/
#ifndef IMP_CHARACTER_H
#define IMP_CHARACTER_H

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_TemplateManager.h"
#include "BFW_Group.h"
#include "BFW_Totoro.h"

void Imp_OpenAnimCache(BFtFileRef *inFileRef);
void Imp_CloseAnimCache(void);

UUtError
Imp_AddAnimCache(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddBiped(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddBody(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddBody(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddAnimation(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddBodyTextures(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddAnimationCollection(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddStringList(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddCharacterClass(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddFacingTable(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddAimingScreen(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddScreenCollection(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddSavedFilm(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddSavedGhostReplay(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddVariant(
	BFtFileRef			*inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup			*inGroup,
	char				*inInstanceName);

UUtError
Imp_AddVariantList(
	BFtFileRef			*inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup			*inGroup,
	char				*inInstanceName);

UUtError
Imp_Character_Initialize(
	void);

void
Imp_Character_Terminate(void);

UUtError Imp_GetAnimState(GRtGroup *inGroup, const char *inName, TRtAnimState *outState);
UUtError Imp_GetAnimType(GRtGroup *inGroup, const char *inName, TRtAnimType *outType);

UUtError IMPrStringToAnimType(const char *inString, UUtUns16 *result);

extern UUtUns8 IMPgDefaultCompressionSize;

#endif /* IMP_CHARACTER_H */
