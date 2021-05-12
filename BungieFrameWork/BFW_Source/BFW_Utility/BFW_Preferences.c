/*
	FILE:	BFW_Preferences.c
	
	AUTHOR:	Michael Evans
	
	CREATED: 8/18, 1997
	
	PURPOSE: Interface to the windows style preference files
	
	Copyright 1997

*/

#include <string.h>

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_Preferences.h"

#define PREFERENCE_FILE_NAME	"config.txt"
#define MAX_PREFERENCE			1024
#define MAX_VALUE				1024

typedef struct PrefEntry { 
	char *preference;
	char *value;
} PrefEntry;


PrefEntry	*gPreferences = NULL;
UUtUns32	gPreferencesCount = 0;
UUtBool		gPrefInitialized = 0;

static UUtBool	isWhiteSpace(char c)
{
	UUtBool result;

	switch(c)
	{
		case ' ':
		case '\t':
			result = UUcTrue;
			break;

		default:
			result = UUcFalse;
			break;
	}

	return result;
}

static UUtBool isEOL(char c)
{
	UUtBool result;

	switch(c)
	{
		case '\r':
		case '\n':
		case '\0':
			result = UUcTrue;
			break;

		default:
			result = UUcFalse;
			break;
	}

	return result;
}

/*
 *
 * scan the preference part out and return inString bumped past the preference part 
 *
 */

static const char *ReadPreference(const char *inString, char *outPreference)
{
	UUtUns32 count = 0;

	UUmAssert(NULL != inString);
	UUmAssert(NULL != outPreference);

	while(1) {
		if (isWhiteSpace(*inString)) { break; }
		if (isEOL(*inString)) { break ; }
		
		outPreference[count] = *inString;

		count++;
		inString++;
	}

	outPreference[count] = '\0';

	return inString;
}

static const char *SkipWhiteSpace(const char *inString)
{
	UUmAssert(NULL != inString);

	while(1) {
		if (isEOL(*inString)) { break; }
		if (!isWhiteSpace(*inString)) { break; }

		inString++;
	}

	return inString;
}

static void ReadValue(const char *inString, char *outValue)
{
	UUtUns32 count = 0;

	UUmAssert(NULL != inString);
	UUmAssert(NULL != outValue);

	while(1)
	{
		if (isEOL(*inString)) { break; }

		outValue[count] = *inString;

		count++;
		inString++;
	}

	outValue[count] = '\0';
}

UUtError UUrPreferences_Initialize(void)
{
	UUtError error;
	BFtFileRef *prefDataRef;
	BFtTextFile *prefFile;

	UUmAssert(NULL == gPreferences);
	UUmAssert(0 == gPreferencesCount);

	// Initialize the preferences management system
	error =	BFrFileRef_MakeFromName(PREFERENCE_FILE_NAME, &prefDataRef);
	UUmError_ReturnOnError(error);

	error = BFrTextFile_OpenForRead(prefDataRef, &prefFile);

	if (UUcError_None != error) {
		BFrFileRef_Dispose(prefDataRef);

		return UUcError_None;	// ok to not have a prefs file
	}

	// scan the file getting one line at a time
	while(1) {
		char preference[MAX_PREFERENCE];
		char value[MAX_VALUE];
		const char *curStr;

		curStr = BFrTextFile_GetNextStr(prefFile);
		if (NULL == curStr) { break; }

		curStr = ReadPreference(curStr, preference);
		curStr = SkipWhiteSpace(curStr);
		ReadValue(curStr, value);			

		// step 4 grow the list
		gPreferences = UUrMemory_Block_Realloc(gPreferences, sizeof(PrefEntry) * (gPreferencesCount + 1));
		UUmAssert(NULL != gPreferences);	// some day might want to handle low memory here

		gPreferences[gPreferencesCount].preference = UUrMemory_Block_New(MAX_PREFERENCE);
		UUmAssert(NULL != gPreferences[gPreferencesCount].preference);
		UUrMemory_MoveFast(preference, gPreferences[gPreferencesCount].preference, MAX_PREFERENCE);

		gPreferences[gPreferencesCount].value = UUrMemory_Block_New(MAX_VALUE);
		UUmAssert(NULL != gPreferences[gPreferencesCount].value);
		UUrMemory_MoveFast(value, gPreferences[gPreferencesCount].value, MAX_VALUE);

		gPreferencesCount++;
	}

	BFrTextFile_Close(prefFile);
	BFrFileRef_Dispose(prefDataRef);

	return UUcError_None;
}

static PrefEntry *FindEntry(const char *inWhichPreference)
{
	UUtUns32 i;

	for(i = 0; i < gPreferencesCount; i++)
	{
		int result;

		result = strcmp(inWhichPreference, gPreferences[i].preference);

		if (0 == result) {
			PrefEntry *entry = gPreferences + i;

			return entry;
		}
	}

	// entry not found
	return NULL;
}

const char *UUrPreferences_GetString(const char *inWhichPreference)
{
	PrefEntry *entry;

	if (NULL == gPreferences) {
		return NULL;
	}

	entry = FindEntry(inWhichPreference);

	if (entry != NULL) {
		return entry->value;
	}

	return NULL;
}

UUtUns32 UUrPreferences_GetNumber(const char *inWhichPreference)
{
	UUmAssert(NULL != gPreferences);
	UUmAssert(!"implemented");

	return 0xdeadbeef;
}
	
void UUrPreferences_Terminate(void)
{
	UUtUns32 i;
	
	if (gPreferences != NULL) {
		for(i = 0; i < gPreferencesCount; i++) {
			UUrMemory_Block_Delete(gPreferences[i].preference);
			UUrMemory_Block_Delete(gPreferences[i].value);
		}

		UUrMemory_Block_Delete(gPreferences);
	}

	gPreferences = NULL;
	gPreferencesCount = 0;
}
