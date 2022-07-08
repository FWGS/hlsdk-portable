//---------------------------------------------------------
//---------------------------------------------------------
//-														---
//-					musicgstreamer.cpp							---
//-														---
//---------------------------------------------------------
//---------------------------------------------------------
//-	by Roy, based on the code by									-----------
//-	by JujU									-----------
//-		julien.lecorre@free.fr					-----------
//---------------------------------------------------------
//- mp3 player code for HL mod				-----------
//---------------------------------------------------------
//-														---
//- compatible with version 1.0 of Gstreamer		---
//-		http://www.gstreamer.freedesktop.org/							---
//-														---
//---------------------------------------------------------


/*//---------------

Don't forget to link the actual library to GStreamer.

GStreamer 1.0 or better required.

Tested
with 32-bit GStreamer on Debian

For playlist format:
see the bottom of the file

*///---------------



//---------------------------------------------------------
// inclusions

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "musicgstreamer.h"

CMusic g_MusicPlayer;

//---------------------------------------------------------
// initialisation

void CMusic :: Init ( void )
{
	int argc = 0;
        char** argv = nullptr;
	GError *error = nullptr;

	if( m_bInit == TRUE ){
		return; //Do not re-init.
	}

	if (gst_init_check(&argc,&argv,&error)!=TRUE)
	{
		ALERT ( at_console, "\\\nMUSICPLAYER : unable to initialize\n\\\n" );
		return;
	}

	m_bInit = TRUE;
}




//---------------------------------------------------------
// monitoring the bus


void CMusic :: updateBus ( )
{
	if(gstBus == NULL) return; //Do not react if bus doesn't exist.
	gstMsg = gst_bus_pop (gstBus);
	if(gstMsg != NULL){
    		if (GST_MESSAGE_TYPE (gstMsg) == GST_MESSAGE_ERROR) {
			ALERT( at_console, "\\\nMUSICPLAYER : A GStreamer error has occured.\n\\\n" );
    		}else if (GST_MESSAGE_TYPE (gstMsg) == GST_MESSAGE_EOS) {
			ALERT( at_console, "\\\nMUSICPLAYER : A song has ended.\n\\\n" );
			songEnd();
		}
	}
	if(gstMsg != NULL) gst_message_unref (gstMsg);
}




//---------------------------------------------------------
// playing an audio file


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
// play a list of audio files


void CMusic :: OpenList ( const char *filename )
{
	
	// open text file

	FILE *myfile = fopen ( filename, "r" );

	if ( myfile == NULL )
	{
		ALERT ( at_console, "\\\nMUSICPLAYER : impossible to load %s\n\\\n", filename );
		return;
	}

	// saving songs to the list

	int total = 0;

	if ( fscanf ( myfile, "%i", &total ) != EOF )
	{
		for ( int i=0; i<total; i++ )
		{
			char	ctitle [128];
			int		irepeat;

			// reading the title

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

	// close text file

	fclose ( myfile );
}


//---------------------------------------------------------
// end of the song


void CMusic :: songEnd ( )
{
	// end of the song

	g_MusicPlayer.Stop ();

	// search for the first song in the list

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
		ALERT ( at_console, "\\\nMUSICPLAYER : no song in the list\n\\\n" );
		return; 
	}

	// d

	p->repeat --;

	// removal of songs whose r

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

	// close player if list is empty

	if ( g_MusicPlayer.m_pTrack == NULL )
	{
		g_MusicPlayer.Reset ();
	}

	// next track start

	else
	{
		g_MusicPlayer.Play();
	}

	return;
}


//---------------------------------------------------------
// instruction


void CMusic :: Play	( void )
{
	if ( m_IsPlaying == TRUE )
		return;

	if ( m_bInit == FALSE )
	{
		Init ();

		if ( m_bInit == FALSE )
		{
			ALERT ( at_console, "\\\nMUSICPLAYER : unable to initialize\n\\\n" );
			return;
		}
	}

	// search for the first song in the list

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
		ALERT ( at_console, "\\\nMUSICPLAYER : no song in the list\n\\\n" );
		return; 
	}

	//Stop previous pipeline activity
	if (gstPipeline != NULL){
		gst_element_set_state (gstPipeline, GST_STATE_NULL);
		gst_object_unref (gstPipeline);
		gstPipeline = NULL;
	}
	if (gstBus != NULL){
		gst_object_unref (gstBus);
		gstBus = NULL;
	}

	// loading file
	char pipelinePayload [512];
	sprintf(pipelinePayload, "filesrc location=%s ! decodebin ! audioconvert ! audioresample ! autoaudiosink", p->name);
	gstPipeline = gst_parse_launch(pipelinePayload, NULL);


	if (gstPipeline == NULL)
	{
		ALERT ( at_console, "\\\nMUSICPLAYER : %s : can not start playing the file\n\\\n", p->name );
		return; 
	}

	// playback

	gst_element_set_state (gstPipeline, GST_STATE_PLAYING);
	m_IsPlaying = TRUE;

	// callback at the end of the song

	gstBus = gst_element_get_bus (gstPipeline); //Get bus to monitor
}


void CMusic :: Stop ( void )
{
	if ( m_IsPlaying == TRUE )
	{
		m_IsPlaying = FALSE;
		if (gstPipeline != NULL){
			gst_element_set_state (gstPipeline, GST_STATE_READY);
		}
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
		if (gstPipeline != NULL){
			gst_element_set_state (gstPipeline, GST_STATE_NULL);
			gst_object_unref (gstPipeline);
			gstPipeline = NULL;
		}
		if (gstBus != NULL){
			gst_object_unref (gstBus);
			gstBus = NULL;
		}
		
		//complete
		//we don't actually de-initialize gst here
	}
}

void CMusic :: Terminate ( void ) //Cleanup and dereference
{
	Stop ();
	if ( m_bInit == TRUE )
	{
		ALERT ( at_console, "\\\nMUSICPLAYER : de-initializing and dereferencing\n\\\n" );
		if (gstPipeline != NULL){
			gst_element_set_state (gstPipeline, GST_STATE_NULL);
			gst_object_unref (gstPipeline);
			gstPipeline = NULL;
		}
		if (gstBus != NULL){
			gst_object_unref (gstBus);
			gstBus = NULL;
		}
		
		gst_deinit ();
		g_MusicPlayer.m_bInit = FALSE; //Neither.

		//complete
	}
}


//---------------------------------------------------------
// entity class



class CTriggerMusic : public CPointEntity
{
public:

	void	Spawn		( void );

	void	KeyValue	( KeyValueData *pkvd );
	void	Use			( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	void EXPORT Think( void );

	virtual int	Save	( CSave &save );
	virtual int	Restore	( CRestore &restore );


	static	TYPEDESCRIPTION m_SaveData[];


	string_t	m_iFileName;		// file path
	int			m_iFileType;		// text file (list) or audio file

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

	SetThink( &CTriggerMusic::Think );
	pev->nextthink = gpGlobals->time;
}

void CTriggerMusic::Think( void ) //We need to monitor gst message bus for updates
{
	//if(g_MusicPlayer == NULL) return;
	g_MusicPlayer.updateBus();
	pev->nextthink = gpGlobals->time + 0.25f; // Think again in 1/4 second
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
	filetype(choices) : "File type" : 0 = 
	[
		0: "File list (*.txt)"
		1: "File wav mp2 mp3 ogg raw"
	]
	filename(string) : "Name (mod/folder/file.extension)"
]

*///---------------


/*//---------------
composing lists of audio files

example: music01.txt file:

//

3

monmod/sound/mp3/music01_debut.mp3		1
monmod/sound/mp3/music01_boucle.mp3		3
monmod/sound/mp3/music01_fin.mp3		1


//

composition :
	- total number of diff chunks
	- address of the first music file
	- number of readings of this file
	- address of the second
	- etc ...

*///---------------
