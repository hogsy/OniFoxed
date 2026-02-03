/*
	FILE:	BFW_FF_DDS_PRIV_H.h

	AUTHOR:	Michael Evans

	CREATED: Jan 11, 1999

	PURPOSE:

	Copyright 1999

*/

#ifndef BFW_FF_DDS_PRIV_H
#define BFW_FF_DDS_PRIV_H

#include "BFW.h"

#define FFcDDS_FileSignature		UUm4CharToUns32('D', 'D', 'S', ' ')
#define FFcDDS_FormatSignature_DXT1	UUm4CharToUns32('D', 'X', 'T', '1')
#define FFcDDS_FileVersion		(1)

// 0x70 bytes of header
typedef struct FFtDDS_FileHeader_Raw
{
	UUtUns32	signature;		// 0..3  Always equal to FFcDDS_FileSignature
	char		unknown1[8];	// 4..11 unknown
	UUtUns16	height;			// 12..13
	char		unknown2[2];	// 14..15
	UUtUns16	width;			// 16..17
	char		unknown3[10];	// 18..27
	UUtUns8		mipMapLevels;	// 28	i.e. 1 means no extra map levels
	char		unknown4[3];
	char		unknown5[0x10];
	char		unknown6[0x10];
	char		unknown7[0x10];
	char		unknown8[4];
	UUtUns32	format;			// example: DXT1
	char		unknown9[8];
	char		unknown10[0x10];
	char		unknown11[0x10];
} FFtDDS_FileHeader_Raw;

#endif /* BFW_FF_PRIV_PSD_H */
