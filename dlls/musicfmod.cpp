//---------------------------------------------------------
//---------------------------------------------------------
//-														---
//-					musicfmod.cpp							---
//-														---
//---------------------------------------------------------
//---------------------------------------------------------
//-	par JujU									-----------
//-		julien.lecorre@free.fr					-----------
//---------------------------------------------------------
//- code du lecteur mp3	pour mod HL				-----------
//---------------------------------------------------------
//-														---
//- compatible avec la version 3.6.1 de fmod.dll		---
//-		http://www.fmod.org/							---
//-														---
//---------------------------------------------------------


/*//---------------

mettre la dll fmod.dll dans le dossier half life.

changer l'adresse de la dll dans le #define FMOD_DLL_PATH
du fichier .h
attention, mettre des \\ et pas des \ dans l'adresse.

code 
fin de ce fichier.

composition des fichiers texte listes de fichiers audio :
voir 

*///---------------



//---------------------------------------------------------
// inclusions

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "musicfmod.h"

CMusic g_MusicPlayer;

//---------------------------------------------------------
// initialisation

void CMusic :: Init ( void )
{
	m_hFmodDll = LoadLibrary ( FMOD_DLL_PATH );

	if ( m_hFmodDll == NULL )
		return;

	(FARPROC&)FSOUND_INIT				= GetProcAddress(m_hFmodDll, "_FSOUND_Init@12");
	(FARPROC&)FSOUND_CLOSE				= GetProcAddress(m_hFmodDll, "_FSOUND_Close@0");
	(FARPROC&)FSOUND_STREAM_OPENFILE	= GetProcAddress(m_hFmodDll, "_FSOUND_Stream_OpenFile@12");
	(FARPROC&)FSOUND_STREAM_CLOSE		= GetProcAddress(m_hFmodDll, "_FSOUND_Stream_Close@4");
	(FARPROC&)FSOUND_STREAM_PLAY		= GetProcAddress(m_hFmodDll, "_FSOUND_Stream_Play@8");
	(FARPROC&)FSOUND_STREAM_ENDCALLBACK	= GetProcAddress(m_hFmodDll, "_FSOUND_Stream_SetEndCallback@12");


	if ( !( FSOUND_INIT && FSOUND_CLOSE && FSOUND_STREAM_OPENFILE && FSOUND_STREAM_CLOSE && FSOUND_STREAM_PLAY && FSOUND_STREAM_ENDCALLBACK ))
	{
		FreeLibrary( m_hFmodDll );
		return;
	}


	if (!FSOUND_INIT(44100, 1, 0))
		return; 

	
	m_bInit = TRUE;
}




//---------------------------------------------------------
// lecture d'un fichier audio


void CMusic :: OpenFile ( const char *filename, int repeat )
{
	audiofile_t *p = NULL;
	p = new audiofile_t;

	sprintf ( p->name, filename );
	p->repeat	= repeat;
	p->next		= m_pTrack;

	m_pTrack	= p;
}



//---------------------------------------------------------
// lecture d'une liste de fichiers audio


void CMusic :: OpenList ( const char *filename )
{
	
	// ouverture du fichier texte

	FILE *myfile = fopen ( filename, "r" );

	if ( myfile == NULL )
	{
		ALERT ( at_console, "\\\nMUSICPLAYER : impossible d'ouvrir %s\n\\\n", filename );
		return;
	}

	// enregistrement des morceaux dans la liste

	int total = 0;

	if ( fscanf ( myfile, "%i", &total ) != EOF )
	{
		for ( int i=0; i<total; i++ )
		{
			char	ctitle [128];
			int		irepeat;

			// lecture du titre

			if ( fscanf ( myfile, "%s", ctitle ) != EOF )
			{
				if ( fscanf ( myfile, "%i", &irepeat ) != EOF )
					OpenFile ( ctitle, irepeat );

				else
					break;
			}
			else
				break;
		}
	}

	// fermeture du fichier texte

	fclose ( myfile );
}


//---------------------------------------------------------
// fin du morceau


signed char EndCallback ( FSOUND_STREAM *stream, void *buff, int len, int param )
{
	// fin du morceau

	g_MusicPlayer.Stop ();

	// recherche du premier morceau de la liste

	audiofile_t *p = NULL;
	p = g_MusicPlayer.m_pTrack;

	while ( p != NULL )
	{
		if ( p->next == NULL )
			break;
		else
			p = p->next;
	}

	if ( p == NULL )
	{
		ALERT ( at_console, "\\\nMUSICPLAYER : aucun morceau dans la liste\n\\\n" );
		return TRUE; 
	}

	// d

	p->repeat --;

	// suppression des morceaux dont la r

	if ( p->repeat < 1 )
	{
		if ( g_MusicPlayer.m_pTrack == p )
		{
			delete g_MusicPlayer.m_pTrack;
			g_MusicPlayer.m_pTrack = NULL;
		}
		else
		{
			audiofile_t *q = NULL;
			q = g_MusicPlayer.m_pTrack;

			while ( q->next != p )
				q = q->next;

			delete q->next;
			q->next = NULL;
		}
	}

	// fermeture du lecteur si la liste est vide

	if ( g_MusicPlayer.m_pTrack == NULL )
	{
		g_MusicPlayer.Reset ();
	}

	// lancement du morceau suivant

	else
	{
		g_MusicPlayer.Play();
	}

	return TRUE;
}


//---------------------------------------------------------
// lecture


void CMusic :: Play	( void )
{
	if ( m_IsPlaying == TRUE )
		return;

	if ( m_bInit == FALSE )
	{
		Init ();

		if ( m_bInit == FALSE )
		{
			ALERT ( at_console, "\\\nMUSICPLAYER : initialisation impossible\n\\\n" );
			return;
		}
	}

	// recherche du premier morceau de la liste

	audiofile_t *p = NULL;
	p = m_pTrack;

	while ( p != NULL )
	{
		if ( p->next == NULL )
			break;
		else
			p = p->next;
	}

	if ( p == NULL )
	{
		ALERT ( at_console, "\\\nMUSICPLAYER : aucun morceau dans la liste\n\\\n" );
		return; 
	}

	// chargement du fichier
	
	m_fsound = FSOUND_STREAM_OPENFILE( p->name, FSOUND_NORMAL | FSOUND_LOOP_OFF, 0 );

	if (!m_fsound)
	{
		ALERT ( at_console, "\\\nMUSICPLAYER : %s : fichier introuvable\n\\\n", p->name );
		return; 
	}

	// lecture

	FSOUND_STREAM_PLAY ( FSOUND_FREE, m_fsound );
	m_IsPlaying = TRUE;

	// callback en fin de morceau

	FSOUND_STREAM_ENDCALLBACK ( m_fsound, EndCallback, 0 );
}


void CMusic :: Stop ( void )
{
	if ( m_IsPlaying == TRUE )
	{
		m_IsPlaying = FALSE;
		FSOUND_STREAM_CLOSE ( m_fsound );
	}
}


void CMusic :: Reset ( void )
{
	//r

	Stop ();

	audiofile_t *p = NULL;
	
	while ( m_pTrack != NULL )
	{
		p = m_pTrack;
		m_pTrack = p->next;
		delete p;
	}

	if ( m_bInit == TRUE )
	{
		FSOUND_CLOSE();
		g_MusicPlayer.m_bInit = FALSE;

	//	FreeLibrary( g_MusicPlayer.m_hFmodDll );
		g_MusicPlayer.m_hFmodDll = NULL;
	}
}




//---------------------------------------------------------
// classe de l'entit



class CTriggerMusic : public CPointEntity
{
public:

	void	Spawn		( void );

	void	KeyValue	( KeyValueData *pkvd );
	void	Use			( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );


	virtual int	Save	( CSave &save );
	virtual int	Restore	( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];


	string_t	m_iFileName;		// chemin du fichier
	int			m_iFileType;		// fichier texte ( liste ) ou fichier audio

};

LINK_ENTITY_TO_CLASS( trigger_music, CTriggerMusic );



TYPEDESCRIPTION CTriggerMusic::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerMusic, m_iFileType, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerMusic, m_iFileName, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CTriggerMusic, CPointEntity );



void CTriggerMusic :: Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
}

void CTriggerMusic :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "filetype"))
	{
		m_iFileType = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "filename"))
	{
		m_iFileName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CTriggerMusic :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( g_MusicPlayer.m_IsPlaying == TRUE )
		return;

	if ( m_iFileType == MUSIC_AUDIO_FILE )
	{
		g_MusicPlayer.OpenFile ( STRING(m_iFileName), 1 );
	}
	else
	{
		g_MusicPlayer.OpenList ( STRING(m_iFileName) );
	}

	g_MusicPlayer.Play();
}






/*//---------------
code 


@PointClass base( Targetname ) = trigger_music : "Trigger Music"
[
	filetype(choices) : "Type de fichier" : 0 = 
	[
		0: "Liste de fichiers (*.txt)"
		1: "Fichier wav mp2 mp3 ogg raw"
	]
	filename(string) : "Nom (mod/dossier/fichier.extension)"
]

*///---------------


/*//---------------
composition des listes de fichiers audio

exemple : fichier music01.txt :

//

3

monmod/sound/mp3/music01_debut.mp3		1
monmod/sound/mp3/music01_boucle.mp3		3
monmod/sound/mp3/music01_fin.mp3		1


//

composition :
	- nombre total de morceaux diff
	- adresse du premier fichier musique 
	- nombre de lectures de ce fichier
	- adresse du deuxi
	- etc ...

*///---------------
