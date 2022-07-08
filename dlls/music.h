//---------------------------------------------------------
//---------------------------------------------------------
//-														---
//-					music.h							---
//-														---
//---------------------------------------------------------
//---------------------------------------------------------
//-	by Roy, based on the code by JujU									-----------
//---------------------------------------------------------
//- tee file for null player
//---------------------------------------------------------


#ifndef MUSIC_H
#define MUSIC_H

//Temporary plug to have something to work with on Linux modif de Roy
//---------------------------------------------------------
// defines

#define	MUSIC_AUDIO_FILE		1
#define MUSIC_LIST_FILE			0

//---------------------------------------------------------
// structure of the audio file entity

struct audiofile_t
{
	char name [128];
	int repeat;
	audiofile_t *next;
};

//---------------------------------------------------------
// music class


class CMusic
{
public:

	// fonctions de lecture

	void OpenFile			( const char *filename, int repeat );	// ouverture d'un simple fichier
	void OpenList			( const char *filename );						// ouverture d'un fichier texte contenant les fichiers 

	void Init				( void );		// initialisation

	void Play				( void );		// lecture
	void Stop				( void );		// arr
	void Reset				( void );		// fermeture

	// variables

	
	int m_fsound; //We don't actually have FMOD, so just an int handle.

	BOOL m_IsPlaying;						// t
	BOOL m_bInit;							// t

	audiofile_t *m_pTrack;	//current track

	// constructor & destructor

	CMusic	()	{ m_bInit = FALSE; m_IsPlaying = FALSE; m_pTrack = NULL; Reset(); };
	~CMusic ()	{};

	// functions import
	// none, see window / Julien's code.
};

extern CMusic g_MusicPlayer;
#endif // MUSIC_H
