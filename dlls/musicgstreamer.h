//---------------------------------------------------------
//---------------------------------------------------------
//-														---
//-					musicgstreamer.h							---
//-														---
//---------------------------------------------------------
//---------------------------------------------------------
//-	by Roy, based on the code by JujU									-----------
//---------------------------------------------------------
//- tee file
//---------------------------------------------------------
//-														---
//- compatible with version 1.0 of Gstreamer		---
//-		http://www.gstreamer.freedesktop.org/							---
//-														---
//---------------------------------------------------------



#ifndef MUSIC_H
#define MUSIC_H

#include <gst/gst.h>


//---------------------------------------------------------
// defines

#define	MUSIC_AUDIO_FILE		1
#define MUSIC_LIST_FILE			0

//---------------------------------------------------------
// audio file structure

struct audiofile_t
{
	char name [128];
	int repeat;
	audiofile_t *next;
};

//---------------------------------------------------------
// reader class


class CMusic
{
public:

	// reading functions

	void OpenFile			( const char *filename, int repeat );	// open a single file
	void OpenList			( const char *filename );						// opening a text file containing the files

	void Init				( void );		// initialization

	void Play				( void );		// playback
	void Stop				( void );		// stop
	void Reset				( void );		// closing, but not really
	void Terminate				( void );		// actually closing

	// variables

	BOOL m_IsPlaying;						// t
	BOOL m_bInit;							// t

	audiofile_t *m_pTrack;					// parts 

	// constructor / destructor

	CMusic	()	{ m_bInit = FALSE; m_IsPlaying = FALSE; m_pTrack = NULL; Reset(); };
	~CMusic ()	{ Terminate(); };

	// import functions

	GstElement *gstPipeline;
	GstBus *gstBus;
	GstMessage *gstMsg;
	
	void updateBus();
	void songEnd();
};

extern CMusic g_MusicPlayer;
#endif // MUSIC_H
