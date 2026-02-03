//

#include "BFW.h"
#include "BFW_FileManager.h"

#include "BFW_LSSolution.h"

int
main(
	void)
{
	UUtError			error;
	BFtFileRef*			directory;
	BFtFileRef*			sourceFileRef;
	BFtFileRef*			destFileRef;
	BFtFileIterator*	fileIterator;
	LStData*			lsData;

	UUrInitialize(UUcFalse);

	error =
		BFrFileRef_SearchAndMakeFromName(
			"LS2LSDPlease",
			&directory);
	if(error != UUcError_None)
	{
		return 1;
	}

	error =
		BFrDirectory_FileIterator_New(
			directory,
			NULL,
			".ls",
			&fileIterator);
	if(error != UUcError_None)
	{
		return 1;
	}

	while(1)
	{
		error =
			BFrDirectory_FileIterator_Next(
				fileIterator,
				&sourceFileRef);
		if(error != UUcError_None) break;

		fprintf(stderr, "Processing file: %s"UUmNL, BFrFileRef_GetLeafName(sourceFileRef));

		error =
			BFrFileRef_Duplicate(
				sourceFileRef,
				&destFileRef);
		UUmAssert(error == UUcError_None);

		BFrFileRef_SetLeafNameSuffex(
			destFileRef,
			"lsd");

		error = LSrData_CreateFromLSFile(BFrFileRef_GetFullPath(sourceFileRef), &lsData);
		UUmAssert(error == UUcError_None);

		// Spit out the spewData
		fprintf(stderr, "\tNum Points: %d\n", lsData->numPoints);
		fprintf(stderr, "\tNum Faces: %d\n", lsData->numFaces);

		if(LSrData_GetSize(lsData) >= LScMaxDataSize)
		{
			fprintf(stderr, "WARNING: This LS file is too big(>50Meg)"UUmNL);
		}

		error = LSrData_WriteBinary(destFileRef, lsData);
		UUmAssert(error == UUcError_None);

		LSrData_Delete(lsData);

		BFrFileRef_Dispose(destFileRef);

		BFrFileRef_Dispose(sourceFileRef);
	}

	BFrFileRef_Dispose(directory);

	BFrDirectory_FileIterator_Delete(fileIterator);

	UUrTerminate();

	#if UUmPlatform == UUmPlatform_Win32

	fprintf(stdout, "done, press any key that is labeled enter");

	fgetc(stdin);

	#endif

	return 0;
}
