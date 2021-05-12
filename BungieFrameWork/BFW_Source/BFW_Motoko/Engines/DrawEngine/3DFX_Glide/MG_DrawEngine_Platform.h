/*
	FILE:	MG_DrawEngine_Platform.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 13, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef MG_DRAWENGINE_PLATFORM_H
#define MG_DRAWENGINE_PLATFORM_H

UUtError
MGrDrawEngine_Platform_SetupDrawContextPrivate(
	M3tDrawContextDescriptor*	inDrawContextDescriptor);

void
MGrDrawEngine_Platform_DestroyDrawContextPrivate(
	void);
	
#endif /* MG_DRAWENGINE_PLATFORM_H */
