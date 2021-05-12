// Imp_Common.h

#ifndef IMP_COMMON_H
#define IMP_COMMON_H

//#define PROFILING

#include "BFW.h"
#include "BFW_TemplateManager.h"
#include "BFW_Group.h"

extern UUtUns32	IMPgCurLevel;

UUtError
Imp_Initialize(
	void);

void
Imp_Terminate(
	void);

UUtError
Imp_Common_GetFileRefAndModDate(
	BFtFileRef*		inSourceFileRef,
	GRtGroup*		inGroup,
	char*			inVariableName,
	BFtFileRef*		*outFileRef);

UUtError
Imp_Common_BuildInstance(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	TMtTemplateTag		inTemplateTag,
	char*				inName,
	char*				inCompileDate,
	BFtFileRef*			*outFileRef,
	UUtBool				*outBuild);

UUtError
Imp_Common_Scan_Group(
	const GRtGroup *inGroup,
	const char **inValidNames,
	const char **outBadName);

UUtError Imp_OpenGroupStack(
	BFtFileRef	*inSourceFileRef,
	GRtGroup *ioGroup, 
	GRtGroup_Context *inContext,
	const char *parentVarName);

#if 0
	#define IMPmError_ReturnOnError(error) if (UUcError_None != error) { Imp_PrintWarning("File %s Line %d Error %d", __FILE__, __LINE__, (error)); return error; }
	#define IMPmError_ReturnOnErrorMsg(error, msg) if (UUcError_None != error) { Imp_PrintWarning("File %s Line %d Error %d Msg %s", __FILE__, __LINE__, (error), (msg)); return error; }
	#define IMPmError_ReturnOnErrorMsgP(error, msg, p0, p1, p2) if (UUcError_None != error) { Imp_PrintWarning(msg, p0, p1, p2); return error; }
#else
	#define IMPmError_ReturnOnError(error)																\
		if ((error != UUcError_None) && (IMPgSuppressErrors)) {											\
			Imp_PrintWarning("File %s Line %d Error %d", __FILE__, __LINE__, (error));					\
			return error;																				\
		} else {																						\
			UUmError_ReturnOnError(error);																\
		}

	#define IMPmError_ReturnOnErrorMsg(error, msg)														\
		if ((error != UUcError_None) && (IMPgSuppressErrors)) {											\
			Imp_PrintWarning("File %s Line %d Error %d Msg %s", __FILE__, __LINE__, (error), (msg));	\
			return error;																				\
		} else {																						\
			UUmError_ReturnOnErrorMsg(error, msg);														\
		}

 	#define IMPmError_ReturnOnErrorMsgP(error, msg, p0, p1, p2)											\
		if ((error != UUcError_None) && (IMPgSuppressErrors)) {											\
			Imp_PrintWarning("File %s Line %d Error %d", __FILE__, __LINE__, (error));					\
			Imp_PrintWarning(msg, p0, p1, p2);															\
			return error;																				\
		} else {																						\
			UUmError_ReturnOnErrorMsgP(error, msg, p0, p1, p2);											\
		} 
#endif

void UUcArglist_Call Imp_PrintWarning(const char *format, ...);

typedef enum {
	IMPcMsg_Cosmetic,
	IMPcMsg_Important,
	IMPcMsg_Critical
} IMPtMsg_Importance;

extern IMPtMsg_Importance IMPgMin_Importance;
// prints a message to the console if importance >= IMPgMin_Importance
void UUcArglist_Call Imp_PrintMessage(IMPtMsg_Importance importance, const char *format, ...);

extern UUtBool	IMPgLightmap_OutputPrepFile;
extern UUtBool	IMPgLightmap_OutputPrepFileOne;
extern UUtBool	IMPgLightmap_Output;
extern UUtBool	IMPgLightmaps;
extern UUtBool	IMPgConstructing;
extern UUtBool	IMPgSuppressErrors;
extern UUtBool	IMPgBuildTextFiles;
extern UUtBool	IMPgLightMap_HighQuality;
extern UUtBool	IMPgLowMemory;
extern UUtBool	IMPgDoPathfinding;
extern UUtBool	IMPgPrintFootsteps;
extern GRtGroup *gPreferencesGroup;
extern UUtBool	IMPgNoSound;
extern UUtBool	IMPgBuildPathDebugInfo;
extern UUtBool	IMPgEnvironmentDebug;

extern UUtUns32 IMPgSuppressedWarnings;
extern UUtUns32 IMPgSuppressedErrors;

extern UUtWindow	gDebugWindow;

#endif /* IMP_COMMON_H */
