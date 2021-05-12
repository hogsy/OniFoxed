// ======================================================================
// Imp_Sound2.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_TM_Construction.h"
#include "BFW_Group.h"
#include "BFW_AppUtilities.h"
#include "BFW_SoundSystem2.h"
#include "BFW_SS2_IMA.h"
#include "BFW_SS2_MSADPCM.h"
#include "BFW_BinaryData.h"

#include "Imp_Sound2.h"
#include "Imp_Common.h"
#include "Imp_BatchFile.h"

#include "Oni_Sound2.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
IMPiIsAIFFFile(
	BFtFileRef				*inFileRef)
{
	char					*extension;
	const char				*leaf_name;
	
	leaf_name = BFrFileRef_GetLeafName(inFileRef);
	extension = strrchr(leaf_name, '.');
	if (UUrString_Compare_NoCase(extension, ".aiff") == 0)
	{
		return UUcTrue;
	}
	else if (UUrString_Compare_NoCase(extension, ".aifc") == 0)
	{
		return UUcTrue;
	}
	else if (UUrString_Compare_NoCase(extension, ".aif") == 0)
	{
		return UUcTrue;
	}
	
	return UUcFalse;
}

// ----------------------------------------------------------------------
static UUtBool
IMPiIsWavFile(
	BFtFileRef				*inFileRef)
{
	char					*extension;
	const char				*leaf_name;
	
	leaf_name = BFrFileRef_GetLeafName(inFileRef);
	extension = strrchr(leaf_name, '.');
	if (UUrString_Compare_NoCase(extension, ".wav") == 0)
	{
		return UUcTrue;
	}
	
	return UUcFalse;
}

// ----------------------------------------------------------------------
static UUtError
IMPiLoadCallback_AllocSoundData_ImportTime(
	BFtFileRef					*inFileRef,
	SStSoundData				**outSoundData)
{
	UUtError					error;
	char						instance_name[BFcMaxFileNameLength];

	*outSoundData = NULL;

	UUrString_Copy(instance_name, BFrFileRef_GetLeafName(inFileRef), BFcMaxFileNameLength);
	UUrString_MakeLowerCase(instance_name, BFcMaxFileNameLength);
	error = 
		TMrConstruction_Instance_Renew(
			SScTemplate_SoundData,
			instance_name,
			0,
			outSoundData);
	IMPmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiLoadCallback_SoundStoreData(
	SStSoundData				*inSoundData,
	const char					*inSoundName)
{
	UUtError					error;
	char						instance_name[BFcMaxFileNameLength];
	SStSoundData				*sound_data;
	
	// make the instance name
	UUrString_Copy(instance_name, inSoundName, BFcMaxFileNameLength);
	UUrString_MakeLowerCase(instance_name, BFcMaxFileNameLength);
	
	// create the template instance
	error =
		TMrConstruction_Instance_Renew(
			SScTemplate_SoundData,
			instance_name,
			0,
			&sound_data);
	UUmError_ReturnOnError(error);
	
	// copy inSoundData into the sound_data
	UUrMemory_MoveFast(inSoundData, sound_data, sizeof(SStSoundData));

	// create the raw data
	sound_data->data =
		TMrConstruction_Raw_New(
			inSoundData->num_bytes,
			16,
			SScTemplate_SoundData);
	if (sound_data->data == NULL) {
		return UUcError_Generic;
	}
	
	// copy inSoundData data into sound_data data
	UUrMemory_MoveFast(inSoundData->data, sound_data->data, inSoundData->num_bytes);
	
	// save the sound data
	sound_data->data = TMrConstruction_Raw_Write(sound_data->data);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiProcess_SoundData_File(
	BFtFileRef				*inFileRef)
{
	UUtError				error;
	SStSoundData			*sound_data;
	
	Imp_PrintMessage(IMPcMsg_Important, "\t\tsoundFileName: %s"UUmNL, BFrFileRef_GetLeafName(inFileRef));
	
	// load the sound data from file
	error =
		SSrSoundData_Load(
			inFileRef,
			IMPiLoadCallback_SoundStoreData,
			&sound_data);
	IMPmError_ReturnOnError(error);
	
	// dispose of the sound data
	if (sound_data != NULL)
	{
		if (sound_data->data != NULL)
		{
			UUrMemory_Block_Delete(sound_data->data);
		}
		UUrMemory_Block_Delete(sound_data);
		sound_data = NULL;
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiProcess_SoundData_Directory(
	BFtFileRef				*inDirRef)
{
	UUtError				error;
	BFtFileIterator			*file_iterator;
	
	UUmAssert(inDirRef);
	
	// create a directory iterator
	error =	BFrDirectory_FileIterator_New(inDirRef, NULL, NULL, &file_iterator);
	IMPmError_ReturnOnError(error);
	
	// process all of the files in the directory
	while (1)
	{
		BFtFileRef file_ref;
		
		// get the next file
		error = BFrDirectory_FileIterator_Next(file_iterator, &file_ref);
		if (error != UUcError_None) { break; }
		
		if (BFrFileRef_IsDirectory(&file_ref) == UUcTrue)
		{
			// process the directory
			error = IMPiProcess_SoundData_Directory(&file_ref);
		}
		else if (IMPiIsWavFile(&file_ref))
		{
			// process the file
			IMPiProcess_SoundData_File(&file_ref);
		}
		else if (IMPiIsAIFFFile(&file_ref))
		{
			// process the file
			IMPiProcess_SoundData_File(&file_ref);
		}
		
		
		IMPmError_ReturnOnError(error);	
		continue;
	}
	
	// delete the file iterator
	BFrDirectory_FileIterator_Delete(file_iterator);
	file_iterator = NULL;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiProcess_SoundDirectory(
	BFtFileRef				*inDirRef,
	AUtSharedStringArray	*inFileNameList)
{
	UUtError				error;
	BFtFileIterator			*file_iterator;
	
	// build a file iterator
	error = BFrDirectory_FileIterator_New(inDirRef, NULL, NULL, &file_iterator);
	IMPmError_ReturnOnErrorMsg(error, "Unable to create file iterator for directory");
	
	// loop over the entire directory
	while (1)
	{
		BFtFileRef				file_ref;
		UUtBool					result;
		UUtUns32				index;
		
		// get a file ref
		error = BFrDirectory_FileIterator_Next(file_iterator, &file_ref);
		if (error != UUcError_None) { break; }
		
		// determine if this is a file or directory
		if (BFrFileRef_IsDirectory(&file_ref) == UUcTrue)
		{
			IMPiProcess_SoundDirectory(&file_ref, inFileNameList);
		}
		else if (strcmp(BFrFileRef_GetSuffixName(&file_ref), "aif") == 0)
		{
			char					file_name[BFcMaxPathLength];
			
			// make the name lowercase
			UUrString_Copy(file_name, BFrFileRef_GetLeafName(&file_ref), BFcMaxPathLength);
			UUrString_MakeLowerCase(file_name, BFcMaxPathLength);
			
			// determine if the file is in the list
			result = AUrSharedStringArray_GetIndex(inFileNameList, file_name, &index);
			if (result == UUcFalse) { continue; }
			
			// process the file
			IMPiProcess_SoundData_File(&file_ref);
		}
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiProcess_FileList(
	BFtFileRef				*inDirRef,
	GRtElementArray			*inFileList)
{
	UUtError				error;
	AUtSharedStringArray	*string_array;
	UUtUns32				num_elements;
	UUtUns32				itr;
	
	// build a shared string list from the file list
	string_array = AUrSharedStringArray_New();
	UUmError_ReturnOnNull(string_array);
	
	num_elements = GRrGroup_Array_GetLength(inFileList);
	for (itr = 0; itr < num_elements; itr++)
	{
		UUtUns32				index;
		char					*string;
		GRtElementType			element_type;
		
		// get the element from the array
		error =
			GRrGroup_Array_GetElement(
				inFileList,
				itr,
				&element_type,
				&string);
		if ((error != UUcError_None) || (element_type != GRcElementType_String))
		{
			Imp_PrintWarning("Unable to get element from file list array");
			continue;
		}
		
		UUrString_MakeLowerCase(string, BFcMaxFileNameLength);
		
		// add the file name to the list
		error =
			AUrSharedStringArray_AddString(
				string_array,
				string,
				0,
				&index);
		if (error != UUcError_None)
		{
			Imp_PrintWarning("Unable to add file name to the string array");
			continue;
		}
	}
	
	// process the directory
	error = IMPiProcess_SoundDirectory(inDirRef, string_array);
	UUmError_ReturnOnErrorMsg(error, "Unable to process the sound directory");	
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
IMPrImport_AddSound2(
	BFtFileRef*				inSourceFileRef,
	UUtUns32				inSourceFileModDate,
	GRtGroup*				inGroup,
	char*					inInstanceName)
{
	UUtError				error;
	BFtFileRef				*dir_ref;
	GRtElementArray			*file_list_array;
	GRtElementType			element_type;
	
	// don't process sounds
	if (IMPgNoSound == UUcTrue) { return UUcError_None; }
	
	// get the file list
	file_list_array = NULL;
	error =
		GRrGroup_GetElement(
			inGroup,
			"file_list",
			&element_type,
			&file_list_array);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "invalid file_list was found");
	}
	else if ((file_list_array != NULL) && (element_type == GRcElementType_Array))
	{
		char					*dir_name;
		
		// get the name of the directory the aif's are in.  It can have sub-folders
		error = GRrGroup_GetString(inGroup, "folder", &dir_name);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get the directory name");
		
		// create a dir ref
		error = BFrFileRef_DuplicateAndReplaceName(inSourceFileRef, dir_name, &dir_ref);
		IMPmError_ReturnOnErrorMsg(error, "Unable to create directory reference.");
		
		// process the file list
		error = IMPiProcess_FileList(dir_ref, file_list_array);
		IMPmError_ReturnOnErrorMsg(error, "Unable to process the aiff list");
	}
	else
	{
		// create a dir ref
		error = BFrFileRef_DuplicateAndReplaceName(inSourceFileRef, inInstanceName, &dir_ref);
		IMPmError_ReturnOnErrorMsg(error, "Unable to create directory reference.");
		
		// process the sound data directory
		error = IMPiProcess_SoundData_Directory(dir_ref);
		IMPmError_ReturnOnErrorMsg(error, "Error processing sound data");
	}
	
	// delete the directory ref
	UUrMemory_Block_Delete(dir_ref);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
Imp_Process_SoundBin(
	BFtFileRef				*inSourceFileRef)
{
	UUtError				error;
	const char				*curFileName;
	void					*data;
	void					*separate_data;
	UUtUns32				data_length;

	curFileName = BFrFileRef_GetLeafName(inSourceFileRef);

	// allocate memory and copy the file's contents into that memory
	error =
		BFrFileRef_LoadIntoMemory(
			inSourceFileRef,
			&data_length,
			&data);
	IMPmError_ReturnOnErrorMsg(error, "Unable to load binary file into memory");
	
	if (data_length > 0)
	{
		OStBinaryData		*binary_data;
		
		// create an instance of a binary data template
		error =
			TMrConstruction_Instance_Renew(
				OScTemplate_SoundBinaryData,
				curFileName,
				0,
				&binary_data);
		IMPmError_ReturnOnErrorMsg(error, "Unable to create dialog data instance");
		
		// copy the file data into the raw template data
		separate_data = TMrConstruction_Separate_New(data_length, OScTemplate_SoundBinaryData);
		UUrMemory_MoveFast(data, separate_data, data_length);
		
		binary_data->data_size = data_length;
		binary_data->data_index = (TMtSeparateFileOffset) TMrConstruction_Separate_Write(separate_data);
		
		Imp_PrintMessage(IMPcMsg_Important, "\tsoundBinaryFileName: %s"UUmNL, curFileName);
	}
	
	// dispose of the allocated memory
	if (data != NULL)
	{
		UUrMemory_Block_Delete(data);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiImport_SoundBins(
	BFtFileRef				*inDirectoryRef)
{
	UUtError				error;
	BFtFileIterator			*file_iterator;

	// create a file iterator
	error = BFrDirectory_FileIterator_New(inDirectoryRef, NULL, NULL, &file_iterator);
	UUmError_ReturnOnErrorMsg(error, "Unable to make a file iterator.");
	
	while (1)
	{
		BFtFileRef ref;
		
		// get the next ref
		error = BFrDirectory_FileIterator_Next(file_iterator, &ref);
		if (error != UUcError_None) { break; }
		
		if (BFrFileRef_IsDirectory(&ref) == UUcTrue)
		{
			error = IMPiImport_SoundBins(&ref);
			UUmError_ReturnOnError(error);
		}
		else
		{
			const char			*file_ext;
			
			file_ext = BFrFileRef_GetSuffixName(&ref);
			if ((UUrString_Compare_NoCase(file_ext, OScAmbientSuffix) == 0) ||
				(UUrString_Compare_NoCase(file_ext, OScGroupSuffix) == 0) ||
				(UUrString_Compare_NoCase(file_ext, OScImpulseSuffix) == 0))
			{
				error = Imp_Process_SoundBin(&ref);
				UUmError_ReturnOnError(error);
			}
		}
	}
	
	BFrDirectory_FileIterator_Delete(file_iterator);
	file_iterator = NULL;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
IMPrImport_AddSoundBin(
	BFtFileRef*				inSourceFileRef,
	UUtUns32				inSourceFileModDate,
	GRtGroup*				inGroup,
	char*					inInstanceName)
{
	UUtError				error;
	BFtFileRef				*dir_ref;
	
	// get the parent directory
	error = BFrFileRef_GetParentDirectory(inSourceFileRef, &dir_ref);
	UUmError_ReturnOnErrorMsg(error, "Unable to get the files parent directory");
	UUmAssert(BFrFileRef_IsDirectory(dir_ref));
	
	// import all of the sound bin files in the hierarchy starting at dir_ref
	IMPiImport_SoundBins(dir_ref);
	
	BFrFileRef_Dispose(dir_ref);
	dir_ref = NULL;
	
	return UUcError_None;
}

UUtError
IMPrImport_AddSubtitles(
	BFtFileRef*				inSourceFileRef,
	UUtUns32				inSourceFileModDate,
	GRtGroup*				inGroup,
	char*					inInstanceName)
{
	UUtError error;
	BFtFileRef *file_ref;
	SStSubtitle *subtitle;
	UUtUns32 subtitle_offset[1024];
	UUtUns32 subtitle_count = 0;
	UUtUns32 data_count = 0;
	char *data = NULL;

	if(TMrConstruction_Instance_CheckExists(SScTemplate_Subtitle, inInstanceName)) {
		Imp_PrintMessage(IMPcMsg_Important, "subtitles already imported"UUmNL);
		return UUcError_None;
	}

	// ok read the file
	{
		BFtTextFile *text_file;
		
		error = Imp_Common_GetFileRefAndModDate(inSourceFileRef, inGroup, "file", &file_ref);
		if (error) { return error; }

		// ok, now we need to read the file, line by line 
		error = BFrTextFile_MapForRead(file_ref, &text_file);
		if (error) { return error; }

		while(1)
		{
			char *subtitle_instance = BFrTextFile_GetNextStr(text_file);
			char *subtitle_text = BFrTextFile_GetNextStr(text_file);
			char *end_marker = BFrTextFile_GetNextStr(text_file);
			
			if ((NULL == subtitle_instance) ||
				(NULL == subtitle_text) ||
				(NULL == end_marker)) {
				// ok, we are done with this file
				break;
			}

			if (strcmp(end_marker, "END") != 0) {
				Imp_PrintMessage(IMPcMsg_Critical, "%s"UUmNL, subtitle_instance);
				Imp_PrintMessage(IMPcMsg_Critical, "%s"UUmNL, subtitle_text);
				Imp_PrintMessage(IMPcMsg_Critical, "%s"UUmNL, end_marker);
				Imp_PrintWarning("failed to find end marker, at chunk %d", subtitle_count);
				BFrTextFile_Close(text_file);
				return UUcError_Generic;
			}

			subtitle_offset[subtitle_count++] = data_count;

			UUmAssert(0 == strlen(""));
			data = UUrMemory_Block_Realloc(data, data_count + strlen(subtitle_instance) + strlen(subtitle_text) + 2);
			if (NULL == data) { return UUcError_OutOfMemory; }

			// add the two string to the subtitle data block
			strcpy(data + data_count, subtitle_instance);
			data_count += strlen(subtitle_instance) + 1;

			strcpy(data + data_count, subtitle_text);
			data_count += strlen(subtitle_text) + 1;
		}

		BFrTextFile_Close(text_file);
	}

	// sort the instance next

	error = TMrConstruction_Instance_Renew(SScTemplate_Subtitle, inInstanceName, subtitle_count, &subtitle);
	IMPmError_ReturnOnError(error);

	subtitle->data = TMrConstruction_Raw_New(data_count, 1, SScTemplate_Subtitle);
	UUrMemory_MoveFast(data, subtitle->data, data_count);
	subtitle->data = TMrConstruction_Raw_Write(subtitle->data);

	UUrMemory_MoveFast(subtitle_offset, subtitle->subtitles, sizeof(UUtUns32) * subtitle_count);
	subtitle->numSubtitles = subtitle_count;

	UUrMemory_Block_Delete(data);
	data = NULL;

	BFrFileRef_Dispose(file_ref);

	return UUcError_None;
}
