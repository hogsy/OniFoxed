/*
	FILE:	RV_DrawEngine_Platform.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 13, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef RV_DRAWENGINE_PLATFORM_H
#define RV_DRAWENGINE_PLATFORM_H

UUtError
RVrDrawEngine_Platform_SetupDrawContextPrivate(
	M3tDrawContextDescriptor*	inDrawContextDescriptor,
	RVtDrawContextPrivate*		inDrawContextPrivate);

void
RVrDrawEngine_Platform_DestroyDrawContextPrivate(
	RVtDrawContextPrivate*		inDrawContextPrivate);
	
#endif /* RV_DRAWENGINE_PLATFORM_H */
