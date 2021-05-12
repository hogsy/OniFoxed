; MS_Mac_CallAsm.s

	MACRO
	InternalFunction	&functionName		; function with no TOC vector
	
		EXPORT		.&functionName[PR]
		
		CSECT		.&functionName[PR]
		FUNCTION	.&functionName[PR]
		
	ENDM

	InternalFunction	MSrPPCAsm_CallProc
	
	mtctr	r3
	bctr
	

