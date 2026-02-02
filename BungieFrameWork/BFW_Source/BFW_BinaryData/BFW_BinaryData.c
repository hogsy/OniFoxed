// ======================================================================
// BFW_BinaryData.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_BinaryData.h"
#include "BFW_TemplateManager.h"
#include "BFW_FileManager.h"

// ======================================================================
// typedefs
// ======================================================================
typedef struct BDtClass
{
	BDtClassType			class_type;
	BDtMethods				methods;

} BDtClass;

// ======================================================================
// globals
// ======================================================================
TMtPrivateData				*BDgBinaryData_PrivateData = NULL;

static UUtMemory_Array		*BDgClassArray;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
BDiBinaryData_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void					*inPrivateData)
{
	UUtError				error;
	BFtFile					*separate_data_file;
	BDtBinaryData			*binary_data;
	const char				*instance_name;
	void					*binary_storage;

	binary_data = (BDtBinaryData*)inInstancePtr;

	switch (inMessage)
	{
		case TMcTemplateProcMessage_LoadPostProcess:
			instance_name = TMrInstance_GetInstanceName(inInstancePtr);

			// CB: advance the instance name by four bytes to skip the binary data class which is prepended
			// to avoid name collisions in the template manager
			UUmAssert(strlen(instance_name) >= 4);
			instance_name += 4;

			separate_data_file = TMrInstance_GetSeparateFile(inInstancePtr);
			if ((separate_data_file != NULL) && (binary_data->data_size > 0)) {
				// allocate new memory for the binary data
				binary_storage = UUrMemory_Block_New(binary_data->data_size);
				UUmError_ReturnOnNull(binary_storage);

				// load the binary data from disk
				error = BFrFile_ReadPos(separate_data_file, binary_data->data_index, binary_data->data_size, binary_storage);
				UUmError_ReturnOnError(error);

				// call the binary loader
				error =
					BDrBinaryLoader(
						instance_name,
						binary_storage,
						binary_data->data_size,
						UUcFalse,
						UUcTrue);
				UUmError_ReturnOnError(error);
			}
		break;
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
BDrBinaryData_Write(
	BDtClassType			inClassType,
	UUtUns8					*inData,
	UUtUns32				inDataSize,
	BFtFile					*inFile)
{
	UUtError				error;
	BDtHeader				header;

	// fill in the header
	header.class_type = inClassType;
	header.data_size = inDataSize;

#if (UUmEndian == UUmEndian_Big)
	UUrSwap_4Byte(&header.class_type);
	UUrSwap_4Byte(&header.data_size);
#endif

	// start at the beginning of the file
	error =	BFrFile_SetPos(inFile, 0);
	UUmError_ReturnOnError(error);

	// write the header
	error =
		BFrFile_Write(
			inFile,
			sizeof(BDtHeader),
			&header);
	UUmError_ReturnOnError(error);

	// write the data
	error =
		BFrFile_Write(
			inFile,
			inDataSize,
			inData);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
BDrBinaryLoader(
	const char				*inIdentifier,
	UUtUns8					*inBuffer,
	UUtUns32				inBufferSize,
	UUtBool					inLocked,
	UUtBool					inAllocated)
{
	UUtError				error;
	BDtData					*data;
	UUtBool					swap_it;
	UUtUns32				num_classes;
	BDtClass				*class_list;
	UUtUns32				i;

	// get a pointer to the data
	data = (BDtData*)inBuffer;

#if (UUmEndian == UUmEndian_Big)
	swap_it = UUcTrue;

	UUrSwap_4Byte(&data->header.class_type);
	UUrSwap_4Byte(&data->header.data_size);
#else
	swap_it = UUcFalse;
#endif

	// do a quick check
	UUmAssert(inBufferSize == (data->header.data_size + sizeof(BDtHeader)));

	// find the appropriate class handler
	num_classes = UUrMemory_Array_GetUsedElems(BDgClassArray);
	class_list = (BDtClass*)UUrMemory_Array_GetMemory(BDgClassArray);
	if (class_list == NULL) { return UUcError_Generic; }

	for (i = 0; i < num_classes; i++)
	{
		if (class_list[i].class_type == data->header.class_type)
		{
			error =
				class_list[i].methods.rLoad(
					inIdentifier,
					data,
					swap_it,
					inLocked,
					inAllocated);
			UUmError_ReturnOnError(error);

			break;
		}
	}

	// CB: if we didn't recognise this class type, fire off an assertion
	UUmAssert(i < num_classes);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
BDrRegisterClass(
	BDtClassType			inClassType,
	BDtMethods				*inMethods)
{
	UUtError				error;
	BDtClass				*class_list;
	UUtUns32				index;
	UUtBool					mem_moved;

	UUmAssert(BDgClassArray);
	UUmAssert(inMethods);

	class_list = (BDtClass*)UUrMemory_Array_GetMemory(BDgClassArray);
	if (class_list)
	{
		UUtUns32				num_classes;
		UUtUns32				i;

		// determine if another class of this type has already been registered
		num_classes = UUrMemory_Array_GetUsedElems(BDgClassArray);
		for (i = 0; i < num_classes; i++)
		{
			if (class_list[i].class_type == inClassType)
			{
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "Class already registered");
			}
		}
	}

	// get a space for the class
	error = UUrMemory_Array_GetNewElement(BDgClassArray, &index, &mem_moved);
	UUmError_ReturnOnError(error);

	// get a pointer to the class list, this is necessary because
	// class_list may have been NULL the first time through
	class_list = (BDtClass*)UUrMemory_Array_GetMemory(BDgClassArray);

	// add the class
	class_list[index].class_type = inClassType;
	class_list[index].methods = *inMethods;

	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
BDrInitialize(
	void)
{
	UUtError				error;

	// register the templates
	error = BDrRegisterTemplates();
	UUmError_ReturnOnError(error);

	// install the proc handler
	error =
		TMrTemplate_PrivateData_New(
			BDcTemplate_BinaryData,
			0,
			BDiBinaryData_ProcHandler,
			&BDgBinaryData_PrivateData);
	UUmError_ReturnOnError(error);

	// initialize the memory array
	BDgClassArray =
		UUrMemory_Array_New(
			sizeof(BDtClass),
			1,
			0,
			0);
	UUmError_ReturnOnNull(BDgClassArray);

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
BDrTerminate(
	void)
{
	UUrMemory_Array_Delete(BDgClassArray);
}
