/*
	FILE:	EM_DrawEngine_Platform.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 13, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef EM_DRAWENGINE_PLATFORM_H
#define EM_DRAWENGINE_PLATFORM_H

UUtError
EDrDrawEngine_Platform_SetupDrawContextPrivate(
	M3tDrawContextDescriptor*	inDrawContextDescriptor,
	EDtDrawContextPrivate*		inDrawContextPrivate);

void
EDrDrawEngine_Platform_DestroyDrawContextPrivate(
	EDtDrawContextPrivate*		inDrawContextPrivate);
	
#endif /* EM_DRAWENGINE_PLATFORM_H */
