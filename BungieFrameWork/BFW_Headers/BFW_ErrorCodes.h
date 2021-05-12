#pragma once

/*
	FILE:	BFW_Error.h
	
	AUTHOR:	Michael Evans, Brent H. Pease
	
	CREATED: July 16, 1998
	
	PURPOSE: Error type and error codes
	
	Copyright 1998

*/

#ifndef BFW_ERRORCODES_H
#define BFW_ERRORCODES_H

#include "BFW.h"

//#define UUcError_None				((UUtError) 0x0000)
//#define UUcError_Generic			((UUtError) 0x0001)
//#define UUcError_OutOfMemory		((UUtError) 0x0002)
//#define UUcError_DirectDraw		((UUtError) 0x0003)
#define UUcError_AssertionFailure	((UUtError) 0x0004)
#define UUcError_BadCommandLine		((UUtError) 0x0005)


#define TMcError_Generic			((UUtError) 0x0100)
#define TMcError_DataCorrupt		((UUtError) 0x0101)
#define TMcError_NoDatFile			((UUtError) 0x0102)

#define M3cError_Generic			((UUtError) 0x0200)

#define BFcError_Generic			((UUtError) 0x0300)
#define BFcError_FileNotFound		((UUtError) 0x0301)

#define ONcError_NoDataFolder		((UUtError)	0x0400)

#define GRcError_ElementNotFound	((UUtError) 0x0450)

#define AUcError_FlagNotFound		((UUtError) 0x0500)

// if UUtError is not UUtUns16 change this
// XXX - This needs to be internationalized
typedef tm_struct UUtErrorBinding
{
	char		errorText[126];
	UUtUns16	error;
	
} UUtErrorBinding;

#define TMcTemplate_ErrorBindingArray UUm4CharToUns32('U', 'U', 'E', 'A')
typedef tm_template('U', 'U', 'E', 'A', "Error Binding Array")
TMtErrorBindingArray
{
	tm_pad						pad0[20];
	
	tm_varindex	UUtUns32		numErrorBindings;
	tm_vararray	UUtErrorBinding	errorBindings[1];
	
} TMtErrorBindingArray;

#endif // BFW_ERRORCODES_H