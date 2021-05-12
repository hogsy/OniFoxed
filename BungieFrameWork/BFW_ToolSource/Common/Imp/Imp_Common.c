// Imp_Common.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Akira.h"
#include "BFW_FileManager.h"
#include "BFW_TemplateManager.h"
#include "BFW_TM_Construction.h"
#include "BFW_LocalInput.h"
#include "BFW_Totoro.h"
#include "BFW_TextSystem.h"
#include "BFW_Group.h"
#include "BFW_SoundSystem.h"
#include "BFW_SoundSystem2.h"
#include "BFW_AppUtilities.h"
#include "BFW_BinaryData.h"
#include "BFW_WindowManager.h"
#include "BFW_Materials.h"

#include "Imp_Common.h"
#include "Imp_Model.h"
#include "Imp_Texture.h"
#include "Imp_Env2.h"
#include "Imp_Input.h"
#include "Imp_Character.h"
#include "Imp_Font.h"
//#include "Imp_AI_HHT.h"

#include "Oni_Templates.h"
#include "Oni_Weapon.h"
#include "Oni.h"
#include "Oni_InGameUI.h"
#include "Oni_Sound2.h"

// control what messages are printed
IMPtMsg_Importance IMPgMin_Importance = IMPcMsg_Important;

ONtCommandLine	ONgCommandLine;	// Needed so importer will compile under VC


UUtError
Imp_Initialize(
	void)
{
	UUtError	error;

		
	/*
	 * Initialize the other managers
	 */
		error = TMrRegisterTemplates();
		UUmError_ReturnOnError(error);
		
		error = IMPrEnv_Initialize();
		UUmError_ReturnOnError(error);
		
		error = IMPrModel_Initialize();
		UUmError_ReturnOnError(error);
		
		error = ONrRegisterTemplates();	// register all our oni templates

		error = M3rRegisterTemplates();
		UUmError_ReturnOnError(error);
		
		error = LIrRegisterTemplates();
		UUmError_ReturnOnError(error);
		
		error = AKrRegisterTemplates();
		UUmError_ReturnOnError(error);
		
		error = TRrRegisterTemplates();
		UUmError_ReturnOnError(error);
		
		error = TSrRegisterTemplates();
		UUmError_ReturnOnError(error);
				
		error = SS2rRegisterTemplates();
		UUmError_ReturnOnError(error);
		
		error = BDrRegisterTemplates();
		UUmError_ReturnOnError(error);
		
		error = WMrRegisterTemplates();
		UUmError_ReturnOnError(error);
				
		error = FXrRegisterTemplates();
		UUmError_ReturnOnError(error);

		error = EPrRegisterTemplates();
		UUmError_ReturnOnError(error);

		error = IMrInitialize();
		UUmError_ReturnOnError(error);
		
		error = MArMaterials_RegisterTemplates();
		UUmError_ReturnOnError(error);
		
		error = ONrInGameUI_RegisterTemplates();
		UUmError_ReturnOnError(error);
		
		error = OSrRegisterTemplates();
		UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

void
Imp_Terminate(
	void)
{
	// close everything
	
	
	Imp_Character_Terminate();
	IMPrEnv_Terminate();
	IMPrModel_Terminate();
}


UUtError
Imp_Common_GetFileRefAndModDate(
	BFtFileRef*		inSourceFile,
	GRtGroup*		inGroup,
	char*			inVariableName,
	BFtFileRef*		*outFileRef)
{
	UUtError		error;
	GRtElementType	elementType;
	char*			fileName;
	
	error =
		GRrGroup_GetElement(
			inGroup,
			inVariableName,
			&elementType,
			&fileName);
	if(error != UUcError_None || elementType != GRcElementType_String)
	{
		UUmError_ReturnOnErrorMsgP(UUcError_Generic, "Could not get \"%s\" name string", (UUtUns32)inVariableName, 0, 0);
	}
	
	error = BFrFileRef_DuplicateAndReplaceName(inSourceFile, fileName, outFileRef);
	UUmError_ReturnOnErrorMsgP(error, "Could not find file \"%s\"", (UUtUns32)fileName, 0, 0);
	
	if(!BFrFileRef_FileExists(*outFileRef))
	{
		Imp_PrintWarning("source file %s could not find data file \"%s\"", BFrFileRef_GetLeafName(inSourceFile), fileName);
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Could not find file");
	}
	
	return UUcError_None;
}

UUtError
Imp_Common_BuildInstance(
	BFtFileRef*			inSourceFileRef,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	TMtTemplateTag		inTemplateTag,
	char*				inInstanceName,
	char*				inCompileDate,
	BFtFileRef*			*outFileRef,
	UUtBool				*outBuildInstance)
{
	UUtError		error;
	
	UUtBool			buildInstance;
	
	buildInstance = !TMrConstruction_Instance_CheckExists(inTemplateTag, inInstanceName);
	
	if (!BFrFileRef_FileExists(inSourceFileRef))
	{
		Imp_PrintWarning("inInstanceName %s, Could not find file %s"UUmNL, inInstanceName, BFrFileRef_GetLeafName(inSourceFileRef));
		return UUcError_Generic;
	}
		
	error =
		Imp_Common_GetFileRefAndModDate(
			inSourceFileRef,
			inGroup,
			"file",
			outFileRef);
	UUmError_ReturnOnErrorMsg(error, "Could not get file name");
	
	//fprintf(stderr, "data file mod time: %s", ctime((time_t*)&fileModDate));
	
	*outBuildInstance = buildInstance;
	
	return UUcError_None;
}

void UUcArglist_Call Imp_PrintWarning(
	 const char *format,
	...)
{
	char buffer[512];
	va_list arglist;
	int return_value;

	va_start(arglist, format);
	return_value= vsprintf(buffer, format, arglist);
	va_end(arglist);

	fprintf(stderr, "%s"UUmNL, buffer);

	if (IMPgSuppressErrors) {
		IMPgSuppressedWarnings++;
	} else {
		AUrMessageBox(AUcMBType_OK, "%s", buffer);
	}
}

void UUcArglist_Call Imp_PrintMessage(IMPtMsg_Importance importance, const char *format, ...)
{
	if (importance >= IMPgMin_Importance)
	{
		char buffer[512];
		va_list arglist;
		int return_value;
		static FILE *logFile = NULL;

		va_start(arglist, format);
		return_value= vsprintf(buffer, format, arglist);
		va_end(arglist);

		fprintf(stderr, "%s", buffer);

		if (NULL == logFile) {
			logFile = fopen("import_log.txt", "w");
		}

		if (NULL != logFile) {
			fprintf(logFile, "%s", buffer);
			fflush(logFile);
		}
	}

	return;
}

static UUtBool IsStringInList(
	const char **list,
	const char *string)
{
	const char **itr;

	for(itr = list; *itr != NULL; itr++) {
		if (0 == strcmp(*itr, string)) {
			return UUcTrue;
		}
	}

	return UUcFalse;
}

UUtError
Imp_Common_Scan_Group(
	const GRtGroup *inGroup,
	const char **inValidNames,
	const char **outBadName)
{
	UUtUns32 itr;
	UUtUns32 count = GRrGroup_GetNumVariables(inGroup);

	for(itr = 0; itr < count; itr++)
	{
		const char *curString = GRrGroup_GetVarName(inGroup, itr);
		UUtBool inList;

		inList = IsStringInList(inValidNames, curString);

		if (!inList) {
			*outBadName = curString;
			return UUcError_Generic;
		}
	}

	return UUcError_None;
}

UUtError Imp_OpenGroupStack(
	BFtFileRef	*inSourceFileRef,
	GRtGroup *ioGroup, 
	GRtGroup_Context *inContext,
	const char *parentVarName)
{
	UUtError error;
	GRtGroup *curGroup;

	curGroup = ioGroup;
	
	while(1)
	{
		BFtFileRef *fileRef;
		char *fileName;
		UUtBool fileExists;
		GRtGroup *newGroup;

		error = GRrGroup_GetString(curGroup, parentVarName, &fileName);
		if (error != UUcError_None) { break; }

		error = BFrFileRef_DuplicateAndReplaceName(
			inSourceFileRef,
			fileName,
			&fileRef);
		if (error != UUcError_None) { break; }

		fileExists = BFrFileRef_FileExists(fileRef);
		if (!fileExists) { return UUcError_Generic; }

		error = GRrGroup_Context_NewFromFileRef(
			fileRef,
			NULL,
			inContext,
			NULL,
			&newGroup);
		UUmError_ReturnOnError(error);
		
		BFrFileRef_Dispose(fileRef);
		
		GRrGroup_SetRecursive(curGroup, newGroup);
		curGroup = newGroup;
	}

	return UUcError_None;
}

#define M3cTextureMap_GetPlaceholder_UpperCase 0x01
#define M3cTextureMap_GetPlaceholder_LowerCase 0x02
#define M3cTextureMap_GetPlaceholder_StripExtension 0x04

M3tTextureMap *M3rTextureMap_GetPlaceholder(const char *inTextureName)
{
	UUtError error;
	TMtPlaceHolder place_holder;
	M3tTextureMap *result = NULL;

	{
		const char *name = inTextureName ? inTextureName : "unique";

		// Imp_PrintMessage(IMPcMsg_Critical, "**** adding texture placeholder %s"UUmNL, inTextureName);
	}

	error = TMrConstruction_Instance_GetPlaceHolder(M3cTemplate_TextureMap, inTextureName, &place_holder);
	if (UUcError_None == error) {
		result = (M3tTextureMap *) place_holder;
	}

	return result;
}

M3tTextureMap *M3rTextureMap_GetPlaceholder_Internal(const char *inTextureName, UUtUns32 inFlags)
{
	M3tTextureMap *result = NULL;
	char buffer[512];
	UUtBool upper_case = (inFlags & M3cTextureMap_GetPlaceholder_UpperCase) > 0;
	UUtBool lower_case = (inFlags & M3cTextureMap_GetPlaceholder_LowerCase) > 0;
	UUtBool strip_extension = (inFlags & M3cTextureMap_GetPlaceholder_StripExtension) > 0;

	strcpy(buffer, inTextureName);

	UUmAssert(!(upper_case && lower_case));

	if (upper_case) {
		UUrString_Capitalize(buffer, sizeof(buffer));
	}
	else if (lower_case) {
		UUrString_MakeLowerCase(buffer, sizeof(buffer));
	}

	if (strip_extension) {
		UUrString_StripExtension(buffer);
	}

	result = M3rTextureMap_GetPlaceholder(buffer);

	return result;
}

M3tTextureMap *M3rTextureMap_GetPlaceholder_UpperCase(const char *inTextureName)
{
	return M3rTextureMap_GetPlaceholder_Internal(inTextureName, M3cTextureMap_GetPlaceholder_UpperCase);
}

M3tTextureMap *M3rTextureMap_GetPlaceholder_LowerCase(const char *inTextureName)
{
	return M3rTextureMap_GetPlaceholder_Internal(inTextureName, M3cTextureMap_GetPlaceholder_LowerCase);
}

M3tTextureMap *M3rTextureMap_GetPlaceholder_StripExtension(const char *inTextureName)
{
	return M3rTextureMap_GetPlaceholder_Internal(inTextureName, M3cTextureMap_GetPlaceholder_StripExtension);
}

M3tTextureMap *M3rTextureMap_GetPlaceholder_StripExtension_UpperCase(const char *inTextureName)
{
	return M3rTextureMap_GetPlaceholder_Internal(inTextureName, M3cTextureMap_GetPlaceholder_UpperCase | M3cTextureMap_GetPlaceholder_StripExtension);
}

M3tTextureMap *M3rTextureMap_GetPlaceholder_StripExtension_LowerCase(const char *inTextureName)
{
	return M3rTextureMap_GetPlaceholder_Internal(inTextureName, M3cTextureMap_GetPlaceholder_LowerCase | M3cTextureMap_GetPlaceholder_StripExtension);
}
