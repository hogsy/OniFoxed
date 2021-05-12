// ======================================================================
// BFW_Bink.h
// ======================================================================
#pragma once

#ifndef BFW_BINK_H
#define BFW_BINK_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_FileManager.h"

/*---------- constants */

// define this to render bink thru OpenGL
//#define USE_OPENGL_WITH_BINK

enum {
	BKcScale_Default,
	BKcScale_Fill_Window,
	BKcScale_Half,
	BKcScale_Double
};

typedef UUtUns32 BKtScale;


// ======================================================================
// prototypes
// ======================================================================
UUtError
BKrMovie_Play(
	BFtFileRef					*inMovieRef,
	UUtWindow					inWindow,
	BKtScale					inScale);

// ======================================================================
#endif /* BFW_BINK_H */