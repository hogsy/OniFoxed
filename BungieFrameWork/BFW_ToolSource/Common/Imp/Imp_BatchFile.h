// Imp_BatchFile.h

UUtError
Imp_BatchFile_Process(
	BFtFileRef*			inBatchFileRef);

void
Imp_BatchFile_Activate_All(
	UUtBool inImport);

UUtError
Imp_BatchFile_Activate_Type(
	const char *inType, UUtBool inImport);

UUtError Imp_Process_Bin(BFtFileRef *inSourceFileRef);
