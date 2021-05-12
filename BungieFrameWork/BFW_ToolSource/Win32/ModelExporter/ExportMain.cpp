/*
 *
 * ExportMain.cp
 *
 */

#include "ExportMain.h"
#include "Max.h"

HINSTANCE hInstance = NULL;

 // Jaguar interface code

BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved) 
{
	static int controlsInit = FALSE;

	hInstance = hinstDLL;

	if ( !controlsInit ) {
		controlsInit = TRUE;

		InitCustomControls(hInstance);
		InitCommonControls();
	}

	switch(fdwReason) {
		case DLL_PROCESS_ATTACH:
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
		}


	return(TRUE);
}
//------------------------------------------------------
// This is the interface to Jaguar:
//------------------------------------------------------

__declspec( dllexport ) const TCHAR *LibDescription() 
{
	return "oni export DLL"; 
}

#define kLibNumberClasses 4

__declspec( dllexport ) int LibNumberClasses() 
{
	return kLibNumberClasses; 
}

__declspec( dllexport ) ClassDesc *LibClassDesc(int i) 
{
	ClassDesc *table[kLibNumberClasses] = 
	{
		pMADesc,
		pMMDesc,
		pEnvironmentDesc,
		pEnvAnimDesc
	};
	ClassDesc *result = NULL;

	assert(i >= 0);
	assert(i < kLibNumberClasses);

	result = table[i];

	return result;
}

// Return version so can detect obsolete DLLs
__declspec( dllexport ) ULONG LibVersion() 
{
	return VERSION_3DSMAX; 
}
