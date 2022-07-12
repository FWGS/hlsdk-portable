//-------------------------------------------------------------
//-------------------------------------------------------------
//-
//-			clientmusic.h
//-
//-------------------------------------------------------------
//-------------------------------------------------------------
//- by Roy at suggestion by nekonomicon, based on code by JujU
//-------------------------------------------------------------
//- mp3 player code for HL mod
//-------------------------------------------------------------
//-
//- compatible with version 0.11.9 of Miniaudio
//- https://github.com/mackron/miniaudio
//-
//-------------------------------------------------------------

#ifndef CLIENTMUSIC_H
#define CLIENTMUSIC_H

#define MINIAUDIO_IMPLEMENTATION
#include "../miniaudio/miniaudio.h"

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


class CMusic //: public CHudBase
{
public:

	// reading functions

	void OpenFile			( char *filename, int repeat );	// open a single file
	void OpenList			( char *filename );						// opening a text file containing the files

	void Init				( void );		// initialization

	void Play				( void );		// playback
	void Stop				( void );		// stop
	void Reset				( void );		// pause and switch to next track
	void Terminate				( void );		// clean-up during termination

	// variables

	bool m_IsPlaying;						// monitors whether the music is played, used to pause the music
	bool m_bInit;							// checks if the player is initialized

	audiofile_t *m_pTrack;					// playlist items

	// constructor / destructor

	CMusic	()	{ m_bInit = false; m_IsPlaying = false; m_pTrack = NULL; Reset(); };
	~CMusic ()	{ Terminate(); };

	// object instances

	ma_result result;
	ma_decoder decoder;
	ma_device_config deviceConfig;
	ma_device device;

	// messages

	int MsgFunc_CMusicOpen ( const char *pszName, int iSize, void *pbuf );

	// monitoring functions

	void songEnd();
};

extern CMusic g_MusicPlayer;
#endif // CLIENTMUSIC_H
