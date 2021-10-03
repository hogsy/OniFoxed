// ======================================================================
// BFW_Bink.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_AppUtilities.h"
#include "BFW_Bink.h"
#include "BFW_LocalInput.h"
#include "BFW_FileManager.h"
#include "Oni_Motoko.h"
#include "Oni_Persistance.h"

#include "bink.h"
#include "Rad.h"

#include "gl_engine.h"

#if UUmPlatform == UUmPlatform_Mac

#include <Files.h>

#elif UUmPlatform == UUmPlatform_Win32

#include "BFW_SS2_Platform_Win32.h"

#else

#error "Platform not defined"

#endif

// ======================================================================
// typedefs
// ======================================================================
typedef struct BKtBuffer
{
	HBINKBUFFER					bink_buffer;

} BKtBuffer;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------

static void BKrBink_SetVolume(HBINK inBink)
{
#if defined(BINK_VIDEO) && BINK_VIDEO
	if (NULL != inBink) {
		float float_volume = UUmPin(ONrPersist_GetOverallVolume(), 0.f, 1.f);
		UUtInt32 bink_volume = MUrUnsignedSmallFloat_To_Uns_Round(float_volume * 32768);

		BinkSetVolume(inBink, bink_volume);
	}
#endif

	return;
}

#if UUmPlatform == UUmPlatform_Mac

static HBINK
BKrBink_Open(
	BFtFileRef					*inMovieRef,
	UUtUns32					inFlags)
{
#if defined(BINK_VIDEO) && BINK_VIDEO
	HBINK						bink;
	FSSpec						*movie_fsspec;

	// get a pointer to the fsspec of the BFtFileRef
	movie_fsspec = BFrFileRef_GetFSSpec(inMovieRef);
	if (movie_fsspec == NULL)
	{
		UUrDebuggerMessage("could not create FSSpec for bink movie '%s'", BFrFileRef_GetLeafName(inMovieRef));
		bink= NULL;
	}
	else
	{
		bink= BinkMacOpen(movie_fsspec, (BINKFILEHANDLE | inFlags));
	}

	BKrBink_SetVolume(bink);

	return bink;
#else
	return NULL;
#endif
}

#elif UUmPlatform == UUmPlatform_Win32

static HBINK
BKrBink_Open(
	BFtFileRef					*inMovieRef,
	UUtUns32					inFlags)
{
#if defined(BINK_VIDEO) && BINK_VIDEO
	HBINK						bink;
	const char					*file_name;
	LPDIRECTSOUND				direct_sound;
	UUtBool						file_exists;

	file_exists = BFrFileRef_FileExists(inMovieRef);
	if (!file_exists)
	{
		UUrDebuggerMessage("bink movie file '%s' does not exist", inMovieRef->name);
		return NULL;
	}

	direct_sound = SS2rPlatform_GetDirectSound();
	if (direct_sound != NULL)
	{
		BinkSoundUseDirectSound(direct_sound);
	}

	file_name = BFrFileRef_GetFullPath(inMovieRef);
	if ((file_name == NULL) || (file_name[0] == '\0'))
	{
		UUrDebuggerMessage("could not get full path to bink movie '%s'", inMovieRef->name);
		return NULL;
	}

	bink= BinkOpen(file_name, inFlags);

	if (bink && !direct_sound)
	{
		BinkSetSoundOnOff(bink, 0);
	}

	BKrBink_SetVolume(bink);

	return bink;
#else
	return NULL;
#endif
}

#endif

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
BKiBuffer_CopyFromBink(
	BKtBuffer					*inBuffer,
	HBINK						inBink)
{
#if defined(BINK_VIDEO) && BINK_VIDEO
	BinkCopyToBuffer(
		inBink,
		inBuffer->bink_buffer->Buffer,
		inBuffer->bink_buffer->BufferPitch,
		inBuffer->bink_buffer->Height,
		0,
		0,
		inBuffer->bink_buffer->SurfaceType);
#endif
}

// ----------------------------------------------------------------------
static void
BKiBuffer_Delete(
	BKtBuffer					*inBuffer)
{
#if defined(BINK_VIDEO) && BINK_VIDEO
	UUmAssert(inBuffer);

	if (inBuffer->bink_buffer)
	{
		BinkBufferClose(inBuffer->bink_buffer);
		inBuffer->bink_buffer = NULL;
	}

	UUrMemory_Block_Delete(inBuffer);
#endif
}

// ----------------------------------------------------------------------
static void
BKiBuffer_Draw(
	BKtBuffer					*inBuffer,
	HBINK						inBink)
{
#if defined(BINK_VIDEO) && BINK_VIDEO
	UUtUns32					num_rects;

	// blit the data onto the screen (only for off-screen and DIBs)
	num_rects = BinkGetRects(inBink, inBuffer->bink_buffer->SurfaceType);
	BinkBufferBlit(inBuffer->bink_buffer, inBink->FrameRects, num_rects);
#endif
}

// ----------------------------------------------------------------------
static BKtBuffer*
BKiBuffer_New(
	UUtWindow					inWindow,
	UUtInt32					inMovieWidth,
	UUtInt32					inMovieHeight)
{
#if defined(BINK_VIDEO) && BINK_VIDEO
	BKtBuffer					*buffer;
	HBINKBUFFER					bink_buffer;
	UUtUns32					buffer_flags;

	buffer = (BKtBuffer*)UUrMemory_Block_New(sizeof(BKtBuffer));
	if (buffer == NULL) { return NULL; }

	buffer_flags = (BINKBUFFERAUTO);
	bink_buffer = BinkBufferOpen(inWindow, inMovieWidth, inMovieHeight, buffer_flags);
	if (bink_buffer == NULL)
	{
		UUrMemory_Block_Delete(buffer);
		buffer = NULL;
		return NULL;
	}

	buffer->bink_buffer = bink_buffer;

	return buffer;
#else
	return NULL;
#endif
}

// ----------------------------------------------------------------------
static UUtBool
BKiBuffer_Lock(
	BKtBuffer					*inBuffer)
{
#if defined(BINK_VIDEO) && BINK_VIDEO
	UUtBool						result;

	UUmAssert(inBuffer);

	if (inBuffer->bink_buffer == NULL) { return UUcFalse; }

	result = (BinkBufferLock(inBuffer->bink_buffer) != 0);

	return result;
#else
	return UUcFalse;
#endif
}

// ----------------------------------------------------------------------
static void
BKiBuffer_SetPosition(
	BKtBuffer					*inBuffer,
	UUtInt32					inX,
	UUtInt32					inY)
{
#if defined(BINK_VIDEO) && BINK_VIDEO
	BinkBufferSetOffset(inBuffer->bink_buffer, inX, inY);
#endif
}

// ----------------------------------------------------------------------
static void
BKiBuffer_Unlock(
	BKtBuffer					*inBuffer)
{
#if defined(BINK_VIDEO) && BINK_VIDEO
	UUmAssert(inBuffer);

	BinkBufferUnlock(inBuffer->bink_buffer);
#endif
}

// ======================================================================
#if defined(BINK_VIDEO) && BINK_VIDEO
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
BKiBink_NextFrame(
	HBINK						inBink,
	BKtBuffer					*inBinkBuffer,
	UUtWindow					inWindow)
{
#if defined(BINK_VIDEO) && BINK_VIDEO
	BinkDoFrame(inBink);

	if (BKiBuffer_Lock(inBinkBuffer) == UUcTrue)
	{
		BKiBuffer_CopyFromBink(inBinkBuffer, inBink);
		BKiBuffer_Unlock(inBinkBuffer);
	}

	BKiBuffer_Draw(inBinkBuffer, inBink);

	// stop if this was the last frame
	if (inBink->FrameNum == (inBink->Frames - 1)) { return UUcFalse; }

	BinkNextFrame(inBink);
	return UUcTrue;
#else
	return UUcFalse;
#endif
}

static UUtBool BKrBinkIsLoaded(
	void)
{
#if defined(BINK_VIDEO) && BINK_VIDEO
	UUtBool loaded= UUcFalse;

#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Win32)
	if (GetModuleHandle("binkw32.dll") != NULL)
	{
		loaded= UUcTrue;
	}
#elif defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Mac)
	// Bink Shared Library should be weak linked
	if (BinkMacOpen != 0)
	{
		loaded= UUcTrue;
	}
#endif

	return loaded;
#else
	return UUcFalse;
#endif
}

// ----------------------------------------------------------------------

static void BKrMovie_Statistics(HBINK bink)
{
#if 0
#if TOOL_VERSION
	BINKSUMMARY summary;

	BinkGetSummary(bink, &summary);

	UUrStartupMessage("movie information:");
	UUrStartupMessage("Width %d", summary.Width);
	UUrStartupMessage("Height %d", summary.Height);
	UUrStartupMessage("TotalTime %d", summary.TotalTime);
	UUrStartupMessage("FileFrameRate %d", summary.FileFrameRate);
	UUrStartupMessage("FileFrameRateDiv %d", summary.FileFrameRateDiv);
	UUrStartupMessage("FrameRate %d", summary.FrameRate);
	UUrStartupMessage("FrameRateDiv %d", summary.FrameRateDiv);
	UUrStartupMessage("TotalOpenTime %d", summary.TotalOpenTime);
	UUrStartupMessage("TotalFrames %d", summary.TotalFrames);
	UUrStartupMessage("TotalPlayedFrames %d", summary.TotalPlayedFrames);
	UUrStartupMessage("SkippedFrames %d", summary.SkippedFrames);
	UUrStartupMessage("SkippedBlits %d", summary.SkippedBlits);
	UUrStartupMessage("SoundSkips %d", summary.SoundSkips);
	UUrStartupMessage("TotalBlitTime %d", summary.TotalBlitTime);
	UUrStartupMessage("TotalReadTime %d", summary.TotalReadTime);
	UUrStartupMessage("TotalVideoDecompTime %d", summary.TotalVideoDecompTime);
	UUrStartupMessage("TotalAudioDecompTime %d", summary.TotalAudioDecompTime);
	UUrStartupMessage("TotalIdleReadTime %d", summary.TotalIdleReadTime);
	UUrStartupMessage("TotalBackReadTime %d", summary.TotalBackReadTime);
	UUrStartupMessage("TotalReadSpeed %d", summary.TotalReadSpeed);
	UUrStartupMessage("SlowestFrameTime %d", summary.SlowestFrameTime);
	UUrStartupMessage("Slowest2FrameTime %d", summary.Slowest2FrameTime);
	UUrStartupMessage("SlowestFrameNum %d", summary.SlowestFrameNum);
	UUrStartupMessage("Slowest2FrameNum %d", summary.Slowest2FrameNum);
	UUrStartupMessage("AverageDataRate %d", summary.AverageDataRate);
	UUrStartupMessage("AverageFrameSize %d", summary.AverageFrameSize);
	UUrStartupMessage("HighestMemAmount %d", summary.HighestMemAmount);
	UUrStartupMessage("TotalIOMemory %d", summary.TotalIOMemory);
	UUrStartupMessage("HighestIOUsed %d", summary.HighestIOUsed);
	UUrStartupMessage("Highest1SecRate %d", summary.Highest1SecRate);
	UUrStartupMessage("Highest1SecFrame %d", summary.Highest1SecFrame);
#endif
#endif

	return;
}

UUtError
BKrMovie_Play(
	BFtFileRef					*inMovieRef,
	UUtWindow					inWindow,
	BKtScale					inScale) // unused
{
#if defined(BINK_VIDEO) && BINK_VIDEO
	HBINK						bink;
	BKtBuffer					*bink_buffer;
	UUtBool						done;
	LItMode						mode;

	UUtUns16					width;
	UUtUns16					height;

	UUtInt32					x;
	UUtInt32					y;

	UUtUns32					flags;

	UUmAssert(inMovieRef);
	UUmAssert(inWindow);

	if (!BKrBinkIsLoaded()) return UUcError_None;

	bink = NULL;
	bink_buffer = NULL;
	flags = 0;

	// create the bink handle
	bink = BKrBink_Open(inMovieRef, 0);
	if (bink == NULL) goto cleanup;

	// determine how big to draw the movie
	UUrWindow_GetSize(inWindow, &width, &height);
#if defined (UUmPlatform) &&  (UUmPlatform != UUmPlatform_Mac) // don't blit at 2X on the Mac; it's too slow
	if ((width >= (bink->Width << 1)) &&
		(height >= (bink->Height << 1)))
	{
		flags |= BINKCOPY2XWH;
	}
#endif
	// close bink
	BinkClose(bink);
	bink = NULL;

	// create the bink handle
	bink = BKrBink_Open(inMovieRef, flags);
	if (bink == NULL) goto cleanup;

	// create the buffer
	bink_buffer = BKiBuffer_New(inWindow, bink->Width, bink->Height);
	if (bink_buffer == NULL) goto cleanup;

	// set the position of the movie within the window
	x = (UUtInt32)(((UUtUns32)width - bink->Width) >> 1);
	y = (UUtInt32)(((UUtUns32)height - bink->Height) >> 1);
	BKiBuffer_SetPosition(bink_buffer, x, y);

	// set the input mode to normal
	mode = LIrMode_Get();
	LIrMode_Set(LIcMode_Normal);

	// play the movie
	done = UUcFalse;
	do
	{
		LItInputEvent				event;
		UUtBool						was_event;

#if 0
		// check for an event
		was_event = LIrInputEvent_Get(&event);
		if (was_event == UUcTrue)
		{
			switch (event.type)
			{
				case LIcInputEvent_KeyDown:
				case LIcInputEvent_LMouseDown:
					done = UUcTrue;
				break;
			}
		}
#endif

		if (done == UUcFalse)
		{
			// display the movie
			if (BinkWait(bink) == UUcFalse)
			{
				done = (BKiBink_NextFrame(bink, bink_buffer, inWindow) == UUcFalse);
			}
		}
	}
	while (done == UUcFalse);

	// restore the input mode
	LIrMode_Set(mode);

	BKrMovie_Statistics(bink);

cleanup:
	if (bink_buffer != NULL)
	{
		BKiBuffer_Delete(bink_buffer);
		bink_buffer = NULL;
	}

	if (bink != NULL)
	{
		BinkClose(bink);
		bink = NULL;
	}
#endif

	return UUcError_None;
}

/*
	Bink through OpenGL
*/

// type that contains the textures
typedef struct RADTEXTURES
{
	UUtUns32 total;
	UUtUns32 textures_across;
	UUtUns32 textures_down;
	UUtInt32 width_remnant;
	UUtInt32 height_remnant;
	UUtUns32 width_left;
	UUtUns32 height_left;
	UUtUns32* textures;
	void* buffer;
	UUtUns32 pitch;
	UUtUns32 width;
	UUtUns32 height;
	UUtUns32 pixel_size;
	UUtInt32 surface_type;
	UUtUns32 opengl_format;
	UUtUns32 maxsize;
	float scale;
} RADTEXTURES, *HRADTEXTURES;

static HRADTEXTURES bink_rad_textures=0;

// bind and setup up the texture
static void gl_setup_bink_textures(
	HRADTEXTURES texture,
	UUtUns32 *texture_name)
{
	GL_FXN(glBindTexture)(GL_TEXTURE_2D, *texture_name);

	GL_FXN(glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	GL_FXN(glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// GL_CLAMP_TO_EDGE not supported on all systems; we only use it to hide bugs in the ATI Radeon
	if (gl_GetError() != GL_NO_ERROR)
	{
		GL_FXN(glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		GL_FXN(glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}
	GL_FXN(glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GL_FXN(glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	GL_FXN(glPixelStorei)(GL_UNPACK_ROW_LENGTH, texture->pitch / texture->pixel_size);
	GL_FXN(glPixelStorei)(GL_UNPACK_ALIGNMENT, (texture->pitch % texture->pixel_size) + 1);

	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return;
}

// round up to the next power of two
static UUtUns32 next_power_of_two(
	UUtUns32 val)
{
	if (val>16)
		if (val>64)
			if (val>128)
				if (val>256)
					if (val>512)
						return (1024);
					else
						return (512);
				else
					return (256);
			else
				return (128);
		else
			if (val>32)
				return (64);
			else
				return (32);
	else
		if (val>4)
			if (val>8)
				return (16);
			else
				return (8);
		else
			if (val>2)
				return (4);
	return (val);
}

// open the RAD textures handle (which abstracts as one big texture)
static HRADTEXTURES gl_create_bink_textures(
	UUtUns32 maxsize,
	UUtUns32 width,
	UUtUns32 height,
	float scale)
{
	HRADTEXTURES t;
	UUtUns32 tot,pitch,maxw,maxh;
	UUtUns32 pixel_size= 4; // 3 doesn't work; glPixelStorei(GL_UNPACK_ROW_LENGTH) gives an error
	UUtUns32 extraw= width%maxsize;
	UUtUns32 extrah= height%maxsize;

	extraw=next_power_of_two(extraw);
	extrah=next_power_of_two(extrah);

	// calculate the total number of textures we're going to need
	tot= ((width+(maxsize-1))/maxsize)*((height+(maxsize-1))/maxsize);
	pitch= ((width*pixel_size)+15)&~15;

	maxw= (extraw*pixel_size);
	if (maxw < pitch)
		maxw= pitch;
	maxh= (height>extrah)?height:extrah;

	// memory for the structure, the texture pointers, and the buffer
	t=(HRADTEXTURES)UUrMemory_Block_NewClear(sizeof(RADTEXTURES)+(tot*sizeof(UUtUns32))+(maxw*maxh)+31);
	if (t==0)
		return(0);

	t->textures= (UUtUns32 *)(t+1);
	t->buffer= (void*)((((UUtUns32)(t->textures+tot))+31)&~31);;
	t->total= tot;
	t->pitch= pitch;
	t->width= width;
	t->height= height;
	t->pixel_size= pixel_size;
	t->textures_across= width/maxsize;
	t->textures_down= height/maxsize;
	t->width_remnant= extraw;
	t->height_remnant= extrah;
	t->width_left= width%maxsize;
	t->height_left= height%maxsize;
	t->maxsize= maxsize;
	t->surface_type= (pixel_size == 4) ? BINKSURFACE32RA : BINKSURFACE24R;
	t->opengl_format= (pixel_size == 4) ? GL_RGBA : GL_RGB;
	t->scale= scale;

	t->textures[tot-1]=0;
	GL_FXN(glGenTextures)(tot, (unsigned int*)t->textures);

	if (t->textures[tot-1])
	{
		u32 x,y;
		u32* ts=t->textures;

		// go through and init each texture (setting each of their sizes)
		for(y=0;y<t->textures_down;y++)
		{
			for(x=0;x<t->textures_across;x++)
			{
				gl_setup_bink_textures(t,ts);

				GL_FXN(glTexImage2D )(GL_TEXTURE_2D, 0,
					t->opengl_format, maxsize, maxsize, 0,
					t->opengl_format, GL_UNSIGNED_BYTE, t->buffer);
				UUmAssert(gl_GetError() == GL_NO_ERROR);
				++ts;
			}
			// do any rememnant at the end of the scanline
			if (t->width_remnant)
			{
				gl_setup_bink_textures(t,ts);

				GL_FXN(glTexImage2D )(GL_TEXTURE_2D, 0,
					t->opengl_format, t->width_remnant, maxsize, 0,
					t->opengl_format, GL_UNSIGNED_BYTE, t->buffer);
				UUmAssert(gl_GetError() == GL_NO_ERROR);
				++ts;
			}
		}

		// do any remnants along the bottom edge
		if (t->height_remnant)
		{
			for (x=0;x<t->textures_across;x++)
			{
				gl_setup_bink_textures(t,ts);

				GL_FXN(glTexImage2D )(GL_TEXTURE_2D, 0,
					t->opengl_format, maxsize, t->height_remnant, 0,
					t->opengl_format, GL_UNSIGNED_BYTE, t->buffer);
				UUmAssert(gl_GetError() == GL_NO_ERROR);
				++ts;
			}
			if (t->width_remnant)
			{
				gl_setup_bink_textures(t,ts);

				GL_FXN(glTexImage2D )(GL_TEXTURE_2D, 0,
					t->opengl_format, t->width_remnant, t->height_remnant, 0,
					t->opengl_format, GL_UNSIGNED_BYTE, t->buffer);
				UUmAssert(gl_GetError() == GL_NO_ERROR);
			}
		}

		return(t);
	}

	UUrMemory_Block_Delete(t);

	return (0);
}

// frees the RAD textures object (all textures and memory)
static void gl_dispose_bink_textures(
	HRADTEXTURES t)
{
	if (t)
	{
		if (t->textures)
		{
			// delete the texture
			GL_FXN(glDeleteTextures)(t->total, (unsigned int*)t->textures);
			t->textures=0;

			UUrMemory_Block_Delete(t);
		}
	}

	return;
}

// send down the vertices for one block of the image
static void gl_draw_bink_movie_quad(
	int xp,
	int yp,
	int width,
	int height)
{
	GL_FXN(glBegin)(GL_QUADS);
	GL_FXN(glColor4f)(1.f, 1.f, 1.f, 1.f);

	GL_FXN(glTexCoord2f)(0,0);
	GL_FXN(glVertex2i)(xp, yp);

	GL_FXN(glTexCoord2f)(1.f, 0.f);
	GL_FXN(glVertex2i)((xp+width), yp);

	GL_FXN(glTexCoord2f)(1.f, 1.f);
	GL_FXN(glVertex2i)((xp+width), (yp+height));

	GL_FXN(glTexCoord2f)(0.f, 1.f);
	GL_FXN(glVertex2i)(xp, (yp+height));
	GL_FXN(glEnd)();
/* for debugging
	// draw a little X
	GL_FXN(glDisable)(GL_TEXTURE_2D);
	GL_FXN(glBegin)(GL_LINES);

	GL_FXN(glColor4f)(1.f, 1.f, 1.f, 1.f);

	GL_FXN(glVertex2i)(xp, yp);
	GL_FXN(glVertex2i)((xp+width), yp);

	GL_FXN(glVertex2i)((xp+width), yp);
	GL_FXN(glVertex2i)((xp+width), (yp+height));

	GL_FXN(glVertex2i)(xp, yp);
	GL_FXN(glVertex2i)((xp+width), (yp+height));

	GL_FXN(glVertex2i)(xp, (yp+height));
	GL_FXN(glVertex2i)((xp+width), yp);

	GL_FXN(glEnd)();
	GL_FXN(glEnable)(GL_TEXTURE_2D);
*/
	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return;
}


// send down all the vertices for the entire image
static void gl_draw_bink_movie_frame(
	HBINK bink,
	HRADTEXTURES t,
	UUtInt32 xofs,
	UUtInt32 yofs)
{
#if defined(BINK_VIDEO) && BINK_VIDEO
	UUtUns32* ts, x, y;
	float xp= 0.f, yp= 0.f;

	ts= t->textures;

	for (y=0; y<t->textures_down; y++)
	{
		for (x=0; x<t->textures_across; x++)
		{
			BinkService(bink);

			GL_FXN(glBindTexture)(GL_TEXTURE_2D, *ts);

			// draw the vertices
			gl_draw_bink_movie_quad((int)(xofs+t->scale*xp),
				(int)(yofs+t->scale*yp), (int)(t->scale*t->maxsize),
				(int)(t->scale*t->maxsize));
			++ts;
			xp+= t->maxsize;
		}

		if (t->width_remnant)
		{
			GL_FXN(glBindTexture)(GL_TEXTURE_2D, *ts);

			// draw the vertices
			gl_draw_bink_movie_quad((int)(xofs+t->scale*xp),
				(int)(yofs+t->scale*yp), (int)(t->scale*t->width_remnant),
				(int)(t->scale*t->maxsize));
			++ts;
		}
		yp+= t->maxsize;
		xp= 0.f;
	}

	if (t->height_remnant)
	{
		xp= 0.f;
		for (x=0; x<t->textures_across; x++)
		{
			GL_FXN(glBindTexture)(GL_TEXTURE_2D, *ts);

			// draw the vertices
			gl_draw_bink_movie_quad((int)(xofs+t->scale*xp),
				(int)(yofs+t->scale*yp), (int)(t->scale*t->maxsize),
				(int)(t->scale*t->height_remnant));
			++ts;
			xp+= t->maxsize;
		}
		if (t->width_remnant)
		{
			GL_FXN(glBindTexture)(GL_TEXTURE_2D, *ts);

			// draw the vertices
			gl_draw_bink_movie_quad((int)(xofs+t->scale*xp),
				(int)(yofs+t->scale*yp), (int)(t->scale*t->width_remnant),
				(int)(t->scale*t->height_remnant));
			++ts;
		}
	}
#endif

	return;
}


// send down the texture for one chunk of the image
static void gl_download_bink_texture(
	HRADTEXTURES t,
	void* buffer,
	UUtUns32 width,
	UUtUns32 height)
{
	GL_FXN(glTexSubImage2D)(GL_TEXTURE_2D, 0, 0, 0, width, height, t->opengl_format, GL_UNSIGNED_BYTE, buffer);
	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return;
}


// send down the entire image as multiple textures
static void gl_send_bink_textures(
	HBINK bink,
	HRADTEXTURES t)
{
#if defined(BINK_VIDEO) && BINK_VIDEO
	UUtUns32 *ts, x, y;
	UUtUns8 *buffer;
	UUtUns32 bufadj;

	bufadj= (t->pitch*t->maxsize)-(t->textures_across*t->maxsize*t->pixel_size);

	buffer=t->buffer;

	ts=t->textures;

	for (y=0; y <t->textures_down; y++)
	{
		for(x=0; x < t->textures_across; x++)
		{
			BinkService(bink);

			GL_FXN(glBindTexture)(GL_TEXTURE_2D, *ts);

			// update the pixels, if necessary
			gl_download_bink_texture(t,buffer,t->maxsize,t->maxsize);
			++ts;
			buffer+=(t->maxsize*t->pixel_size);
		}

		if (t->width_remnant)
		{
			GL_FXN(glBindTexture)(GL_TEXTURE_2D, *ts);
			gl_download_bink_texture(t,buffer,t->width_left,t->maxsize);
			++ts;
		}

		buffer+=bufadj;
	}

	if (t->height_remnant)
	{
		for (x=0;x<t->textures_across;x++)
		{
			GL_FXN(glBindTexture)(GL_TEXTURE_2D, *ts);

			// update the pixels, if necessary
			gl_download_bink_texture(t,buffer,t->maxsize,t->height_left);
			++ts;
			buffer+=(t->maxsize*t->pixel_size);
		}
		if (t->width_remnant)
		{
			GL_FXN(glBindTexture)(GL_TEXTURE_2D, *ts);
			gl_download_bink_texture(t,buffer,t->width_left,t->height_left);
			++ts;
		}
	}
#endif

	return;
}

// draw a bitmap by sending down textures and vertices
static void gl_draw_bink_image(
	HBINK bink,
	HRADTEXTURES t,
	UUtInt32 updatetexture,
	UUtInt32 xofs,
	UUtInt32 yofs)
{
	if (updatetexture) {
		gl_send_bink_textures(bink, t);
	}

	gl_draw_bink_movie_frame(bink, t, xofs, yofs);

	return;
}

// advance to the next Bink Frame
static void gl_render_bink_movie_frame(
	HBINK bink,
	int xo,
	int yo)
{
#if defined(BINK_VIDEO) && BINK_VIDEO
	// decompress a frame
	BinkDoFrame(bink);

	// blit over the YUV data into the big linear texture
	BinkCopyToBuffer(bink,bink_rad_textures->buffer, bink_rad_textures->pitch,
		bink_rad_textures->height, 0, 0, bink_rad_textures->surface_type);

	// clear the GL buffer
	GL_FXN(glClear)(GL_COLOR_BUFFER_BIT);
	UUmAssert(gl_GetError() == GL_NO_ERROR);

	// dump the textures to the card
	gl_draw_bink_image(bink, bink_rad_textures,1,xo,yo);

	// advance to the next Bink frame
	BinkNextFrame(bink);
#endif

	return;
}

static int original_depth= 0;
static int current_depth= 0;
static int changed_depth= 0;

static void BKrForce32Bit(
	void)
{

#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Win32)

	UUtBool success;
	DEVMODE desired_display_mode= {0}, current_display_mode= {0};

	current_display_mode.dmSize= sizeof(DEVMODE);
	success= EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &current_display_mode);
	if (success)
	{
		original_depth= current_display_mode.dmBitsPerPel;
		if ((current_display_mode.dmBitsPerPel < 32) && M3gResolutionSwitch)
		{
			desired_display_mode.dmSize= sizeof(DEVMODE);
			desired_display_mode.dmBitsPerPel= 32;
			desired_display_mode.dmPelsWidth= current_display_mode.dmPelsWidth;
			desired_display_mode.dmPelsHeight= current_display_mode.dmPelsHeight;
			desired_display_mode.dmFields= DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

			if (ChangeDisplaySettings(&desired_display_mode, CDS_TEST) == DISP_CHANGE_SUCCESSFUL)
			{
				if (ChangeDisplaySettings(&desired_display_mode, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL)
				{
					current_depth= 32;
					changed_depth= 1;
				}
			}
		}
	}

#else if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Mac)

#endif

	return;
}

static void BKrRestoreBitDepth(
	void)
{

#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Win32)

	if (changed_depth)
	{
		UUtBool success;
		DEVMODE desired_display_mode= {0}, current_display_mode= {0};

		current_display_mode.dmSize= sizeof(DEVMODE);
		success= EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &current_display_mode);
		if (success)
		{
			desired_display_mode.dmSize= sizeof(DEVMODE);
			desired_display_mode.dmBitsPerPel= original_depth;
			desired_display_mode.dmPelsWidth= current_display_mode.dmPelsWidth;
			desired_display_mode.dmPelsHeight= current_display_mode.dmPelsHeight;
			desired_display_mode.dmFields= DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

			if (ChangeDisplaySettings(&desired_display_mode, CDS_TEST) == DISP_CHANGE_SUCCESSFUL)
			{
				if (ChangeDisplaySettings(&desired_display_mode, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL)
				{
					current_depth= original_depth;
					changed_depth= 0;
				}
			}
		}
	}

#else if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Mac)

#endif

	return;
}

/*
	Please don't break this !!!
	Do not change how the bink playback works unless you are prepared to test
	on all video card configurations and OSes. You have been warned! :)
*/

static UUtError BKrMovie_Play_Software(
	BFtFileRef *inMovieRef,
	UUtWindow inWindow,
	BKtScale inScale) // unused
{
#if defined(BINK_VIDEO) && BINK_VIDEO
	HBINK bink= NULL;
	BKtBuffer *bink_buffer= NULL;
	UUtBool done;
	LItMode mode;

	UUtUns16 width;
	UUtUns16 height;

	UUtUns32 flags= 0;

	UUmAssert(inMovieRef);
	UUmAssert(inWindow);

	// create the bink handle
	if (BKrBinkIsLoaded() && ((bink = BKrBink_Open(inMovieRef, 0)) != NULL))
	{
		// we must create a window specifically for playing Bink movies into
		// since you cannot reliably draw 2-D graphics using GDI or QuickDraw into a
		// window being used as an OpenGL context
		UUtWindow window= NULL;
		UUtRect window_rect;

		// make the movie window the same dimensions as the size of the movie when played back
		window_rect.top= window_rect.left= 0;
		window_rect.right= (short)bink->Width;
		window_rect.bottom= (short)bink->Height;

		// determine how big to draw the movie
		UUrWindow_GetSize(inWindow, &width, &height);
		if ((width >= (bink->Width * 2)) &&
			(height >= (bink->Height * 2)))
		{
			flags|= BINKCOPY2XWH;
			window_rect.right*= 2;
			window_rect.bottom*= 2;
		}

		// center window_rect inside of inWindow
		{
			int dx, dy;

			dx= (width - (window_rect.right - window_rect.left)) >> 1;
			dy= (height - (window_rect.bottom - window_rect.top)) >> 1;
			window_rect.top+= dy;
			window_rect.left+= dx;
			window_rect.bottom+= dy;
			window_rect.right+= dx;
		}

		// close bink
		BinkClose(bink);

	/* arg... some video cards explode when we try this... ahh well.
		BKrForce32Bit();
	*/

		// create the window, bink handle, buffer

		if (((window= (UUtWindow)AUrWindow_New(&window_rect)) != NULL) &&
			((bink= BKrBink_Open(inMovieRef, flags)) != NULL) &&
			((bink_buffer= BKiBuffer_New(window, bink->Width, bink->Height)) != NULL))
		{
			// set the position of the movie within the window
			BKiBuffer_SetPosition(bink_buffer, 0, 0);

			// set the input mode to normal
			mode= LIrMode_Get();
			LIrMode_Set(LIcMode_Normal);

			// play the movie
			done= UUcFalse;
			do
			{
				LItInputEvent event= {0};

				// check for an event
				if (LIrInputEvent_Get(&event) == UUcTrue)
				{
					switch (event.type)
					{
						case LIcInputEvent_KeyDown:
						case LIcInputEvent_LMouseDown:
							done= UUcTrue;
						break;
					}
				}

				if (done == UUcFalse)
				{
					// display the movie
					if (BinkWait(bink) == UUcFalse)
					{
						done= (BKiBink_NextFrame(bink, bink_buffer, window) == UUcFalse);
					}
				}

			#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Win32)
				// double check for a key-down to exit the movie
				// (if our bink movie is exactly the same size as the main game window,
				// no events will reach the main game window and hence the above checks
				// will always fail)
				{
					if (GetAsyncKeyState(VK_SPACE) >> 8)
					{
						done= UUcTrue;
					}
				}
			#endif
			}
			while (done == UUcFalse);

			// restore the input mode
			LIrMode_Set(mode);
		}

		if (window)
		{
			AUrWindow_Delete(window);
		}
	/*
		BKrRestoreBitDepth();
	*/
	}

	BKrMovie_Statistics(bink);

	if (bink_buffer != NULL)
	{
		BKiBuffer_Delete(bink_buffer);
	}
	if (bink != NULL)
	{
		BinkClose(bink);
	}
#endif

	return UUcError_None;
}

/*
	Please don't break this !!!
	Do not change how the bink playback works unless you are prepared to test
	on all video card configurations and OSes. You have been warned! :)
*/

UUtError BKrMovie_Play_OpenGL(
	BFtFileRef *movie_ref,
	UUtWindow window,
	BKtScale scale_type)
{
#if defined(BINK_VIDEO) && BINK_VIDEO
	HBINK bink= NULL;
	UUtBool done= UUcFalse;
	LItMode mode;
	UUtBool use_hardware= UUcTrue;

	UUmAssert(movie_ref);
	UUmAssert(window);
	UUmAssert(gl->engine_initialized);

	if (!BKrBinkIsLoaded())
	{
		// do nothing
	}
	else
	{
		if (strstr(gl->vendor, "S3")) // must be vendor NOT renderer don't change this please
		{
			use_hardware = UUcFalse;
		}

		if (!use_hardware)
		{
			BKrMovie_Play_Software(movie_ref, window, scale_type);
		}
		else
		{
			BinkSetIOSize(800000);

			// create the bink handle
			if ((bink= BKrBink_Open(movie_ref, BINKIOSIZE)) != NULL)
			{
				int xo, yo;
				UUtUns16 window_width, window_height;
				float w_win, h_win, w_bink, h_bink, w_scale, h_scale, scale;

				UUrWindow_GetSize(window, &window_width, &window_height);

				w_win= (float)window_width;
				h_win= (float)window_height;
				w_bink= (float)bink->Width;
				h_bink= (float)bink->Height;
				w_scale= w_win/w_bink;
				h_scale= h_win/h_bink;

				switch (scale_type)
				{
					case BKcScale_Default: scale= 1.f; break;
					case BKcScale_Fill_Window: scale= (w_scale<h_scale) ? w_scale : h_scale; break;
					case BKcScale_Half: scale= 0.5f; break;
					case BKcScale_Double: scale= 2.f; break;
					default: scale= 1.f; break;
				}

				if (w_win > (scale * w_bink))
				{
					xo= (int)((w_win - (scale * w_bink)) * 0.5f);
				}
				else
				{
					xo= 0;
				}
				if (h_win > (scale * h_bink))
				{
					yo= (int)((h_win - (scale * h_bink)) * 0.5f);
				}
				else
				{
					yo= 0;
				}

				// set the input mode to normal
				mode= LIrMode_Get();
				LIrMode_Set(LIcMode_Normal);

				bink_rad_textures= gl_create_bink_textures(128, bink->Width, bink->Height, scale);
				UUmAssert(gl_GetError() == GL_NO_ERROR);

				if (bink_rad_textures)
				{
					// set some GL states
					gl_set_textures(NULL, NULL, GL_ONE, GL_ZERO);
					gl->texture0= gl->texture1= NULL;
					if (gl->fog_enabled) GL_FXN(glDisable)(GL_FOG);
					GL_FXN(glEnable)(GL_TEXTURE_2D);
					gl_depth_mode_set(FALSE, FALSE);
					// play the movie
					done = UUcFalse;
					do
					{
						LItInputEvent event;

						// check for an event
						done= (LIrInputEvent_Get(&event) &&
							((event.type == LIcInputEvent_KeyDown) ||
							(event.type == LIcInputEvent_LMouseDown)));

						if (!done)
						{
							// stop if this was the last frame
							if (bink->FrameNum == (bink->Frames - 1))
							{
								done= TRUE;
							}
							// display the movie
							if (BinkWait(bink) == UUcFalse)
							{
								UUtError err;
								long unused;

								err= gl->draw_context_methods.frameStart(0);
								UUmAssert(err == UUcError_None);
								gl_render_bink_movie_frame(bink, xo, yo);
								err= gl->draw_context_methods.frameEnd((unsigned long*)&unused);
								UUmAssert(err == UUcError_None);
							}
						}
					}
					while (done == UUcFalse);

					gl_dispose_bink_textures(bink_rad_textures);

					// restore OpenGL states
					if (gl->fog_enabled) GL_FXN(glEnable)(GL_FOG);
					gl_depth_mode_set(TRUE, TRUE);
					GL_FXN(glPixelStorei)(GL_UNPACK_ALIGNMENT, 1);
					GL_FXN(glPixelStorei)(GL_UNPACK_ROW_LENGTH, 0);
				}
				else
				{
					UUrDebuggerMessage("could not create GL textures for bink movie");
				}

				// restore the input mode
				LIrMode_Set(mode);

				BKrMovie_Statistics(bink);

				BinkClose(bink);
			}
			else
			{
				UUrDebuggerMessage("could not load bink movie");
			}
		}
	}
#endif

	return UUcError_None;
}
