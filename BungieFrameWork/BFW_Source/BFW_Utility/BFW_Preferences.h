/*
	FILE:	BFW_Preferences.h

	AUTHOR:	Michael Evans

	CREATED: 8/18, 1997

	PURPOSE: Interface to the windows style preference files

	Copyright 1997

*/


#ifndef BFW_PREFERENCES_H
#define BFW_PREFERENCES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "BFW.h"

UUtError UUrPreferences_Initialize(
	void);

const char *UUrPreferences_GetString(const char *inWhichPreference);

UUtUns32 UUrPreferences_GetNumber(const char *inWhichPreference);

void UUrPreferences_Terminate(
	void);

#ifdef __cplusplus
}
#endif

#endif /* BFW_PREFERENCES_H */
