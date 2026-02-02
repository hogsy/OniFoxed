// ======================================================================
// Oni_BinaryData.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_BinaryData.h"
#include "BFW_Particle3.h"
#include "BFW_Console.h"

#include "Oni.h"
#include "Oni_BinaryData.h"
#include "Oni_Object.h"
#include "Oni_Win_Particle.h"
#include "Oni_Sound2.h"
#include "Oni_Sound_Animation.h"
#include "Oni_ImpactEffect.h"
#include "Oni_TextureMaterials.h"

#include <ctype.h>

// ======================================================================
// defines
// ======================================================================
#define OBDcMaxFileRefs				2048

// 5 minutes between autosaves
#define OBDcAutosaveTime			5 * 60 * UUcFramesPerSecond

#define OBDgBinaryPath				"Binary"

// ======================================================================
// typedef
// ======================================================================
typedef struct OBDtClassPath
{
	BDtClassType		class_type;
	char				*path;

} OBDtClassPath;

// ======================================================================
// globals
// ======================================================================
static OBDtClassPath	OBDgClassPaths[] =
{
	{ ONcTMBinaryDataClass,							"TextureMaterials" },
	{ ONcIEBinaryDataClass,							"ImpactEffects" },
	{ OScSABinaryDataClass,							"SoundAnims" },
	{ OScBinaryDataClass,							"Sounds" },
	{ P3cBinaryDataClass,							"Particles" },
	{ OBJcBinaryDataClass,							"Objects" },
	{ 0,											NULL }
};

static UUtBool OBDgLookedForBinaryData = UUcFalse;
static BFtFileRef *OBDgBinaryDataFolder = NULL;

#if TOOL_VERSION
static UUtBool OBDgDirtyBinaryData = UUcFalse;
static UUtUns32 OBDgLastSaveTime = 0;
#endif

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
OBDiGetDirectoryRef(
	BDtClassType		inClassType,
	UUtUns16			inLevelNumber,
	UUtBool				inCreate,
	BFtFileRef			**outFileRef)
{
	UUtError			error;
	OBDtClassPath		*class_path;
	BFtFileRef			prefFileRef;
	BFtFileRef			subdirFileRef;
	GRtGroup_Context*	prefGroupContext;
	GRtGroup*			prefGroup;
	char				*binaryDirName;
	char				binaryDirNameStr[256];

	*outFileRef = NULL;

#if STAND_ALONE_ONLY
	return UUcError_Generic;
#endif

	if (OBDgBinaryDataFolder == NULL) {
		// CB: if we've already tried to find the binary data folder,
		// and failed, give up
		if ((OBDgLookedForBinaryData) && (inCreate == UUcFalse))
			return UUcError_Generic;

		OBDgLookedForBinaryData = UUcTrue;

		// CB: find preferences.txt
	#if defined(SHIPPING_VERSION) && (!SHIPPING_VERSION)
		error = BFrFileRef_Search("Preferences.txt", &prefFileRef);
	#else
		error= UUcError_Generic;
	#endif
		if (error != UUcError_None)
			return error;

		error = GRrGroup_Context_NewFromFileRef(&prefFileRef, NULL, NULL,
												&prefGroupContext, &prefGroup);

		if (error != UUcError_None)
			return error;

		// look for the BinDir in the preferences.txt
		error = GRrGroup_GetString(prefGroup, "BinFileDir", &binaryDirName);
		if (error != UUcError_None)
		{
			char *				dataFileDir;

			// if it wasn't found get teh DataFileDir
			error = GRrGroup_GetString(prefGroup, "DataFileDir", &dataFileDir);
			if (error != UUcError_None) { return error; }

			// and append the binary data path to the data file dir name
			sprintf(binaryDirNameStr, "%s\\%s", dataFileDir, OBDgBinaryPath);
			binaryDirName = binaryDirNameStr;
		}

		// make a file ref for the binary data folder
		error = BFrFileRef_MakeFromName(binaryDirName, &OBDgBinaryDataFolder);
		if (error != UUcError_None) { return error; }

		GRrGroup_Context_Delete(prefGroupContext);

		// CB: check that the binary directory exists
		if (BFrFileRef_FileExists(OBDgBinaryDataFolder) == UUcFalse)
		{
			if (inCreate) {
				// CB: make the directory.
				error = BFrDirectory_Create(OBDgBinaryDataFolder, NULL);
				UUmError_ReturnOnErrorMsg(error, "Could not make binary data folder");
			} else {
				// CB: can't find the directory - bail. delete the file reference.
				UUrMemory_Block_Delete(OBDgBinaryDataFolder);
				OBDgBinaryDataFolder = NULL;

				return UUcError_Generic;
			}
		}
	}

	// CB: recurse to appropriate subdir based on level
	sprintf(binaryDirNameStr, "Level%d", inLevelNumber);
	binaryDirName = binaryDirNameStr;

	subdirFileRef = *OBDgBinaryDataFolder;
	error = BFrFileRef_AppendName(&subdirFileRef, binaryDirName);
	UUmError_ReturnOnErrorMsg(error, "Could not create binarydata level-folder file ref");

	// CB: check that the directory exists
	if (BFrFileRef_FileExists(&subdirFileRef) == UUcFalse)
	{
		if (inCreate) {
			// CB: make the directory.
			error = BFrDirectory_Create(&subdirFileRef, NULL);
			UUmError_ReturnOnErrorMsg(error, "Could not make binarydata level-folder");
		} else {
			// CB: can't find the directory - bail. delete the file reference.
			return UUcError_Generic;
		}
	}

	// CB: which class are we reading/writing?
	for (class_path = OBDgClassPaths;
		 class_path->path != NULL;
		 class_path++)
	{
		if (class_path->class_type == inClassType)
		{
			break;
		}
	}

	// CB: must not be unknown class type.
	UUmAssert(class_path->path != NULL);

	// CB: build directory for this class
	error = BFrFileRef_DuplicateAndAppendName(&subdirFileRef,
											class_path->path,
											outFileRef);
	UUmError_ReturnOnErrorMsg(error, "Could not form binarydata class-folder");

	// CB: check that the directory exists
	if (BFrFileRef_FileExists(*outFileRef) == UUcFalse)
	{
		if (inCreate) {
			// CB: make the directory.
			error = BFrDirectory_Create(*outFileRef, NULL);
			UUmError_ReturnOnErrorMsg(error, "Could not make binarydata class-folder");
		} else {
			// CB: can't find the directory - bail. delete the file reference.
			UUrMemory_Block_Delete(*outFileRef);
			*outFileRef = NULL;
			return UUcError_Generic;
		}
	}

	// CB: if we get to here, we have found and/or created the directory
	// $DataFileDir\Binary\Level0\Particles\ or whatever, using
	// preferences.txt, the level number, and the class's path. Phew!
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
OBDrBinaryData_Load(
	BFtFileRef			*inFileRef)
{
	UUtError			error;

	// if the file exists
	if (BFrFileRef_FileExists(inFileRef))
	{
		void			*data;
		UUtUns32		data_size;
		char			identifier[BFcMaxFileNameLength];
		UUtBool			locked;

		// load the file into memory
		error = BFrFileRef_LoadIntoMemory(inFileRef, &data_size, &data);
		UUmError_ReturnOnError(error);

		// get the identifier
		UUrString_Copy(identifier, BFrFileRef_GetLeafName(inFileRef), BFcMaxFileNameLength);
		UUrString_StripExtension(identifier);

		// get the locked status of the file
		locked = BFrFileRef_IsLocked(inFileRef);

		// process the file
		error =
			BDrBinaryLoader(
				identifier,
				data,
				data_size,
				locked,
				UUcTrue);
		UUmError_ReturnOnError(error);
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OBDrBinaryData_Save(
	BDtClassType		inClassType,
	const char			*inIdentifier,
	UUtUns8				*inBuffer,
	UUtUns32			inBufferSize,
	UUtUns16			inLevelNumber,
	UUtBool				inAutosave)
{
	UUtError			error;
	BFtFileRef			*dir_ref;
	BFtFileRef			*file_ref;
	BFtFile				*file;
	char				file_name[BFcMaxFileNameLength];

	UUrMemory_Block_VerifyList();

	// create the diretory ref
	error = OBDiGetDirectoryRef(inClassType, inLevelNumber, UUcTrue, &dir_ref);
	if (error != UUcError_None) { return error; }
	if (dir_ref == NULL) { return UUcError_Generic; }

	// make the identifier into a file name
	UUrString_Copy(file_name, inIdentifier, BFcMaxFileNameLength);
	UUrString_Cat(file_name, (inAutosave) ? ".sav" : ".bin", BFcMaxFileNameLength);

	// create the file ref
	error =
		BFrFileRef_DuplicateAndAppendName(
			dir_ref,
			file_name,
			&file_ref);
	UUmError_ReturnOnError(error);

	// dispose of the directory ref
	UUrMemory_Block_Delete(dir_ref);

	// create the file if it doesn't exist
	if (BFrFileRef_FileExists(file_ref) == UUcFalse)
	{
		error = BFrFile_Create(file_ref);
		UUmError_ReturnOnErrorMsg(error, "Could not create the file.");
	}

	// open the file for writing
	error =
		BFrFile_Open(
			file_ref,
			"w",
			&file);
	if (error != UUcError_None) {
		COrConsole_Printf("### cannot %ssave %s binary data in level %d, file is locked", (inAutosave) ? "auto" : "",
							inIdentifier, inLevelNumber);
		return UUcError_None;
	}

	// write to the file
	error =
		BDrBinaryData_Write(
			inClassType,
			inBuffer,
			inBufferSize,
			file);
	UUmError_ReturnOnErrorMsg(error, "Unable to write to file.");

	// set the EOF
	BFrFile_SetEOF(file);

	// close the file
	BFrFile_Close(file);
	UUrMemory_Block_Delete(file_ref);

	return error;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
OBDrLevel_Load(
	UUtUns16				inLevelNumber)
{
	UUtError				error;
	OBDtClassPath			*class_path;

	// load the binary data files
	for (class_path = OBDgClassPaths;
		 class_path->path != NULL;
		 class_path++)
	{
		BFtFileRef			*dir_ref;
		BFtFileIterator		*file_iterator;

		// create a directory ref for the class path
		error = OBDiGetDirectoryRef(class_path->class_type, inLevelNumber, UUcFalse, &dir_ref);
		if (error != UUcError_None)
		{
			// if the directory doesn't exists, then no data will be loaded from it.
			// continue on to the next directory.
			continue;
		}
		if (dir_ref == NULL)
		{
			continue;
		}

		// see if it exists
		if (BFrFileRef_FileExists(dir_ref))
		{
			// create a directory file iterator
			error =
				BFrDirectory_FileIterator_New(
					dir_ref,
					NULL,
					NULL,
					&file_iterator);
			if (error != UUcError_None) { continue; }

			// iterate over the files
			while (1)
			{
				BFtFileRef		file_ref;
				const char		*file_ext;

				// get the file ref
				error = BFrDirectory_FileIterator_Next(file_iterator, &file_ref);
				if (error != UUcError_None) { break; }

				file_ext = BFrFileRef_GetSuffixName(&file_ref);
				if ((file_ext != NULL) && (UUrString_Compare_NoCase(file_ext, "bin") == 0)) {
					// load the file
					error = OBDrBinaryData_Load(&file_ref);
					if (error != UUcError_None) { continue; }
				}
			}

			// dispose of the directory file iterator
			BFrDirectory_FileIterator_Delete(file_iterator);
		}
		// dispose of the directory ref
		UUrMemory_Block_Delete(dir_ref);
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OBDrLevel_Unload(
	void)
{
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
OBDrMakeDirty(
	void)
{
#if TOOL_VERSION
	OBDgDirtyBinaryData = UUcTrue;
#endif
}

void
OBDrAutosave(
	void)
{
#if TOOL_VERSION
	COrConsole_Printf("Autosaving... please wait...");
	OWrSave_Particles(UUcTrue);
	COrConsole_Printf("...done!");

	OBDgDirtyBinaryData = UUcFalse;
	OBDgLastSaveTime = 0;
#endif
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
OBDrInitialize(
	void)
{
	// initialize globals
	OBDgBinaryDataFolder = NULL;
	OBDgLookedForBinaryData = UUcFalse;

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OBDrTerminate(
	void)
{
	if (OBDgBinaryDataFolder)
	{
		UUrMemory_Block_Delete(OBDgBinaryDataFolder);
	}
}

// ----------------------------------------------------------------------
// CB: check to see if we have to autosave
void
OBDrUpdate(
	UUtUns32 inTime)
{
#if TOOL_VERSION
	if (OBDgDirtyBinaryData)
	{
		if (OBDgLastSaveTime == 0) {
			// CB: first update since we became dirty, store the current time
			OBDgLastSaveTime = inTime;
		}
		else if (inTime - OBDgLastSaveTime > OBDcAutosaveTime)
		{
			OBDrAutosave();
		}
	}
#endif
}
