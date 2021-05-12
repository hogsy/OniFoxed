// ======================================================================
// Oni_ObjectFile.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_FileManager.h"

#include "Oni_Object.h"
#include "Oni_ObjectTypes.h"
#include "Oni_ObjectFile.h"


// ======================================================================
// defines
// ======================================================================
#define OBJcObjectFileSuffix			"ONO"

#define OBJcMaxTypeToFiles				(OBJcMaxObjectTypes)
#define OBJcMaxTTFObjectsTypes			10

#define OBJcSwap						'SW'

// ======================================================================
// typedefs
// ======================================================================
typedef struct OBJtTypeToFile
{
	char					sub_name[OBJcMaxSubNameLength];
	UUtUns32				num_object_types;
	UUtUns32				object_types[OBJcMaxTTFObjectsTypes];
	BFtFileRef				*file_ref;
	BFtFile					*file;
	
} OBJtTypeToFile;


// ======================================================================
// globals
// ======================================================================
static UUtUns32				OBJgNumTypeToFiles;
static OBJtTypeToFile		OBJgTypeToFile[OBJcMaxTypeToFiles];

// ======================================================================
// prototypes
// ======================================================================
static UUtError
OBJiObjectFile_Open(
	BFtFileRef				*inObjectFileRef,
	BFtFile					**outObjectFile);

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
OBJiObjectFile_Process(
	void					*inData,
	UUtUns32				inDataLength)
{
	UUtUns8					*buffer;
	UUtUns16				version;
	UUtBool					swap_it;
	UUtUns16				uns16;
	UUtUns32				num_bytes;
	
	UUmAssert(inData);
	UUmAssert(inDataLength >= 4);
	
	// read the swap flag
	buffer = (UUtUns8*)inData;
	
	// determine if the file needs to be swapped
	swap_it = UUcFalse;
	uns16 = *(UUtUns16*)(buffer);
	if (uns16 != OBJcSwap)
	{
		UUrSwap_2Byte(&uns16);
		if (uns16 != OBJcSwap) { return UUcError_Generic; }
		
		swap_it = UUcTrue;
	}
	buffer += 2;
	
	// read the version number
	OBJmGet2BytesFromBuffer(buffer, version, UUtUns16, swap_it);
	
	// get the size of the data chunk
	OBJmGet4BytesFromBuffer(buffer, num_bytes, UUtUns32, swap_it);
	while (num_bytes > 0)
	{
		UUtUns32			read_bytes;
		
		// process the data chunck
		OBJrObject_CreateFromBuffer(version, swap_it, buffer, &read_bytes);
		UUmAssert(read_bytes == num_bytes);
		
		// advance to next data chunk
		buffer += num_bytes;
		
		// get the size of the data chunk
		OBJmGet4BytesFromBuffer(buffer, num_bytes, UUtUns32, swap_it);
	}
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OBJiObjectFile_Close(
	BFtFile					*inObjectFile)
{
	// close the object file
	BFrFile_Close(inObjectFile);
}

// ----------------------------------------------------------------------
static UUtBool
OBJiObjectFile_FileExists(
	BFtFileRef				*inObjectFileRef)
{
	return BFrFileRef_FileExists(inObjectFileRef);
}

// ----------------------------------------------------------------------
static UUtError
OBJiObjectFile_Load(
	BFtFileRef				*inObjectFileRef)
{
	UUtError				error;
	
	error = UUcError_None;

	// check to see if the file exists
	if (OBJiObjectFile_FileExists(inObjectFileRef))
	{
		BFtFile				*file;
		UUtUns32			length;
		void				*data;
		
		// open the file
		error = OBJiObjectFile_Open(inObjectFileRef, &file);
		UUmError_ReturnOnError(error);
		
		// load the file into memory
		error = BFrFileRef_LoadIntoMemory(inObjectFileRef, &length, &data);
		UUmError_ReturnOnError(error);
		
		// process the file
		error = OBJiObjectFile_Process(data, length);
		
		// dispose of the memory
		UUrMemory_Block_Delete(data);
		data = NULL;
		
		// close the file
		OBJiObjectFile_Close(file);
	}
	
	return error;
}

// ----------------------------------------------------------------------
static void
OBJiObjectFile_MakeFileName(
	UUtUns16				inLevelNumber,
	char					*inSubName,
	char					*outName)
{
	UUmAssert(inSubName);
	UUmAssert(strlen(inSubName) <= OBJcMaxSubNameLength);
	UUmAssertWritePtr(outName, (OBJcMaxFileNameLength + 1));
	
	// clear the filename
	outName[0] = '\0';
	
	// make the file name
	sprintf(outName, "L%d_%s.%s", inLevelNumber, inSubName, OBJcObjectFileSuffix);
}

// ----------------------------------------------------------------------
static UUtError
OBJiObjectFile_MakeFileRef(
	UUtUns16				inLevelNumber,
	char					*inFileName,
	BFtFileRef				**outObjectFileRef)
{
	UUtError				error;
	BFtFileRef				*object_file_ref;
	
	UUmAssert(outObjectFileRef);
	
	*outObjectFileRef = NULL;
	
	// find the appropriate directory
	
	// make the file ref
	error = BFrFileRef_MakeFromName(inFileName, &object_file_ref);
	UUmError_ReturnOnError(error);
	
	*outObjectFileRef = object_file_ref;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObjectFile_New(
	BFtFileRef				*inObjectFileRef)
{
	UUtError				error;
	
	// create the object file
	error = BFrFile_Create(inObjectFileRef);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObjectFile_Open(
	BFtFileRef				*inObjectFileRef,
	BFtFile					**outObjectFile)
{
	UUtError				error;
	BFtFile					*object_file;
	
	// open the object file
	error = BFrFile_Open(inObjectFileRef, "rw", &object_file);
	UUmError_ReturnOnError(error);
	
	*outObjectFile = object_file;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObjectFile_SaveBegin(
	BFtFileRef				*inObjectFileRef,
	BFtFile					**inObjectFile)
{
	UUtError				error;
	UUtUns16				swap_flag;
	UUtUns16				version;
	
	swap_flag = OBJcSwap;
	version = OBJcCurrentVersion;
		
	// open the file
	if (*inObjectFile == NULL)
	{
		error = OBJiObjectFile_Open(inObjectFileRef, inObjectFile);
		UUmError_ReturnOnError(error);
	}
	
	// set the position to the beginning of the file
	error = BFrFile_SetPos(*inObjectFile, 0);
	UUmError_ReturnOnError(error);
	
	// write the swap flag into the file
	error = BFrFile_Write(*inObjectFile, sizeof(UUtUns16), &swap_flag);
	UUmError_ReturnOnError(error);

	error = BFrFile_Write(*inObjectFile, sizeof(UUtUns16), &version);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObjectFile_SaveEnd(
	BFtFile					*inObjectFile)
{
	UUtError				error;
	UUtUns32				zero;
	
	zero = 0;
	
	// write an UUtUns32 with value 0 to indicate the end of the file
	error = BFrFile_Write(inObjectFile, sizeof(UUtUns32), &zero);
	UUmError_ReturnOnError(error);
	
	// close the file
	BFrFile_Close(inObjectFile);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObjectFile_Write(
	BFtFile					*inObjectFile,
	UUtUns8					*inBuffer,
	UUtUns32				inNumBytes)
{
	UUtError				error;
	
	// write the number of bytes
	error = BFrFile_Write(inObjectFile, sizeof(UUtUns32), &inNumBytes);
	UUmError_ReturnOnError(error);
	
	// write the data to the file
	error = BFrFile_Write(inObjectFile, inNumBytes, (void*)inBuffer);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
OBJrObjectFiles_Close(
	void)
{
	UUtUns32				i;
	
	for (i = 0; i < OBJgNumTypeToFiles; i++)
	{
		if (OBJgTypeToFile[i].file == NULL)	{ continue; }
		
		OBJiObjectFile_Close(OBJgTypeToFile[i].file);
		OBJgTypeToFile[i].file = NULL;
	}
}

// ----------------------------------------------------------------------
UUtError
OBJrObjectFiles_Initialize(
	void)
{
	// initialize the OBJgTypeToFiles
	UUrMemory_Clear(OBJgTypeToFile, sizeof(OBJtTypeToFile) * OBJcMaxTypeToFiles);
	OBJgNumTypeToFiles = 0;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OBJrObjectFiles_LevelLoad(
	UUtUns16				inLevelNumber)
{
	UUtError				error;
	UUtUns32				i;
	
	// create file refs for each OBJgTypeToFiles
	for (i = 0; i < OBJgNumTypeToFiles; i++)
	{
		char				file_name[OBJcMaxFileNameLength + 1];
		BFtFileRef			*object_file_ref;
		
		// initialize the object file
		OBJgTypeToFile[i].file_ref = NULL;
		
		// make the file name
		OBJiObjectFile_MakeFileName(
			inLevelNumber,
			OBJgTypeToFile[i].sub_name,
			file_name);
		
		// make the file ref for the file
		error = 
			OBJiObjectFile_MakeFileRef(
				inLevelNumber,
				file_name,
				&object_file_ref);
		UUmError_ReturnOnError(error);
		
		// save the object file ref
		OBJgTypeToFile[i].file_ref = object_file_ref;
		
		// load the objects from the file
		OBJiObjectFile_Load(OBJgTypeToFile[i].file_ref);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OBJrObjectFiles_LevelUnload(
	void)
{
	UUtUns32				i;
		
	// dispose of all of the file refs
	for (i = 0; i < OBJgNumTypeToFiles; i++)
	{
		BFrFileRef_Dispose(OBJgTypeToFile[i].file_ref);
		OBJgTypeToFile[i].file_ref = NULL;
	}
}

// ----------------------------------------------------------------------
void
OBJrObjectFiles_Open(
	void)
{
	UUtUns32				i;
	UUtError				error;
	
	for (i = 0; i < OBJgNumTypeToFiles; i++)
	{
		if (OBJiObjectFile_FileExists(OBJgTypeToFile[i].file_ref) == UUcFalse)
		{
			continue;
		}
		
		// open the file
		error = OBJiObjectFile_Open(OBJgTypeToFile[i].file_ref, &OBJgTypeToFile[i].file);
		if (error != UUcError_None) { continue; }
	}
}

// ----------------------------------------------------------------------
UUtError
OBJrObjectFiles_SaveBegin(
	void)
{
	UUtError				error;
	UUtUns32				i;

	// set the pos to 0
	for (i = 0; i < OBJgNumTypeToFiles; i++)
	{
		if (OBJiObjectFile_FileExists(OBJgTypeToFile[i].file_ref) == UUcFalse)
		{
			continue;
		}
		
		// open the file and prepare for writing of objects
		error =
			OBJiObjectFile_SaveBegin(
				OBJgTypeToFile[i].file_ref,
				&OBJgTypeToFile[i].file);
		UUmError_ReturnOnError(error);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OBJrObjectFiles_SaveEnd(
	void)
{
	UUtError				error;
	UUtUns32				i;

	// set the pos to 0
	for (i = 0; i < OBJgNumTypeToFiles; i++)
	{
		if (OBJiObjectFile_FileExists(OBJgTypeToFile[i].file_ref) == UUcFalse)
		{
			continue;
		}
		
		// close the file
		error =	OBJiObjectFile_SaveEnd(OBJgTypeToFile[i].file);
		UUmError_ReturnOnError(error);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OBJrObjectFiles_RegisterObjectType(
	UUtUns32				inObjectType,
	char					*inSubName)
{
	UUtUns32				i;
	UUtBool					is_associated;
	OBJtTypeToFile			*type_to_file;
	
	// see if inObjectType is already associated with an OBJgTypeToFile
	is_associated = UUcFalse;
	for (i = 0; i < OBJgNumTypeToFiles; i++)
	{
		UUtUns32			j;
		
		// get a pointer to the OBJgTypeToFile
		type_to_file = &OBJgTypeToFile[i];
		
		for (j = 0; j < type_to_file->num_object_types; j++)
		{
			if (type_to_file->object_types[j] == inObjectType)
			{
				is_associated = UUcTrue;
				break;
			}
		}
	}
	
	// if it is associated with an OBJgTypeToFile make sure that the OBJgTypeToFile
	// that it is associated with has the same sub name
	if (is_associated)
	{
		if (strcmp(inSubName, OBJgTypeToFile[i].sub_name) != 0)
		{
			return UUcError_Generic;
		}
		
		// inObjectType is already associated with the correct sub name,
		// so there is nothing left to do except return
		return UUcError_None;
	}
	
	// no OBJgTypeToFiles are associated with inObjectType, add a new one
	type_to_file = &OBJgTypeToFile[OBJgNumTypeToFiles];
	UUrString_Copy(type_to_file->sub_name, inSubName, OBJcMaxSubNameLength);
	type_to_file->object_types[type_to_file->num_object_types] = inObjectType;
	type_to_file->num_object_types++;
	OBJgNumTypeToFiles++;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OBJrObjectFiles_Terminate(
	void)
{
}

// ----------------------------------------------------------------------
UUtError
OBJrObjectFiles_Write(
	UUtUns32				inObjectType,
	UUtUns8					*inBuffer,
	UUtUns32				inNumBytes)
{
	UUtError				error;
	UUtUns32				i;
	OBJtTypeToFile			*type_to_file;
	UUtBool					found;
	
	// find the associated object file
	found = UUcFalse;
	for (i = 0; i < OBJgNumTypeToFiles; i++)
	{
		UUtUns32			j;
		
		// get a pointer to the OBJgTypeToFile
		type_to_file = &OBJgTypeToFile[i];

		for (j = 0; j < type_to_file->num_object_types; j++)
		{
			if (type_to_file->object_types[j] == inObjectType)
			{
				found = UUcTrue;
			}
		}
		
		if (found)
		{
			break;
		}
	}
	
	// if type_to_file is NULL return an error 
	if (type_to_file == NULL) { return UUcError_Generic; }
	
	// if type_to_file's file is NULL then open the file and prepare it for saving
	if (type_to_file->file == NULL)
	{
		error = OBJiObjectFile_SaveBegin(OBJgTypeToFile[i].file_ref, &OBJgTypeToFile[i].file);
		UUmError_ReturnOnError(error);
	}
	
	// write the data into the appropriate file
	error = OBJiObjectFile_Write(type_to_file->file, inBuffer, inNumBytes);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;	
}
