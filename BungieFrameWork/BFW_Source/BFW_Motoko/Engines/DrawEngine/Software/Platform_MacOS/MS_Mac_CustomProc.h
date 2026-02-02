/*
	FILE:	MS_Mac_CustomProc.h

	AUTHOR:	Brent H. Pease

	CREATED: May 19, 1997

	PURPOSE: Interface to the Motoko 3D engine

	Copyright 1997

*/
#ifndef MS_MAC_CUSTOMPROC_H
#define MS_MAC_CUSTOMPROC_H

UUtError  MSrMacCustomProc_BuildZBufferClear(
	MStDrawContextPrivate	*drawContextPrivate);

UUtError  MSrMacCustomProc_BuildImageBufferClear(
	MStDrawContextPrivate	*drawContextPrivate);

UUtError  MSrMacCustomProc_BuildImageBufferCopy(
	MStDrawContextPrivate	*drawContextPrivate);

UUtError  MSrMacCustomProc_BuildImageBufferCopyAndClear(
	MStDrawContextPrivate	*drawContextPrivate);

#endif /* MS_MAC_CUSTOMPROC_H */
