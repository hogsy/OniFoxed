/*
	FILE:	MS_DrawEngine_Platform.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 13, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef MS_DRAWENGINE_PLATFORM_H
#define MS_DRAWENGINE_PLATFORM_H

UUtError
MSrDrawEngine_Platform_SetupDrawContextPrivate(
	M3tDrawContextDescriptor*	inDrawContextDescriptor,
	MStDrawContextPrivate*		inDrawContextPrivate,
	UUtBool						inFullScreen);

void
MSrDrawEngine_Platform_DestroyDrawContextPrivate(
	MStDrawContextPrivate*		inDrawContextPrivate);

UUtError
MSrDrawEngine_Platform_SetupEngineCaps(
	M3tDrawEngineCaps	*ioDrawEngineCaps);
	

#endif /* MS_DRAWENGINE_PLATFORM_H */
