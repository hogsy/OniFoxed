/*
	FILE:	BFW_TextSystem_Template.c
	
	AUTHOR:	Kevin Armstrong
	
	CREATED: ????

	PURPOSE: text
	
	Copyright 1997-1998

*/

#include "BFW.h"
#include "BFW_TemplateManager.h"

#include "BFW_TextSystem.h"

UUtError
TSrRegisterTemplates(
	void)
{
	UUtError	error;
	
	error = TMrTemplate_Register(TScTemplate_Font, sizeof(TStFont), TMcFolding_Allow);
	UUmError_ReturnOnError(error);
	error = TMrTemplate_Register(TScTemplate_GlyphArray, sizeof(TStGlyphArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);
	error = TMrTemplate_Register(TScTemplate_FontLanguage, sizeof(TStFontLanguage), TMcFolding_Allow);
	UUmError_ReturnOnError(error);
	error = TMrTemplate_Register(TScTemplate_FontFamily, sizeof(TStFontFamily), TMcFolding_Allow);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}
