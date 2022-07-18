//-------------------------------------------------------------
//-------------------------------------------------------------
//-
//-			clientmusic.cpp
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

/*
Don't forget to update the miniaudio submodule.

Miniaudio 0.11.9 or better required.

Tested on Debian.

For playlist format see the bottom of the file.
*/



//---------------------------------------------------------
// inclusions


#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "parsemsg.h"

#ifndef DISABLE_MINIAUDIO //Use this to exclude the player in it's entirety. Will use empty "trigger_music" with no playback.
#include "clientmusic.h"

CMusic g_MusicPlayer; //Instantiate.

//These are just initial ones. If the actual track has different ones, they will be re-applied during Play().
#define SAMPLE_FORMAT   ma_format_f32
#define CHANNEL_COUNT   2
#define SAMPLE_RATE     48000

//---------------------------------------------------------
// implementation CHudMusic class methods (defined in hud.h)

//CHudMusic is declared in hud.h and is needed to receive messages.
//hud.cpp also contains and Init function call.
//The implementation goes here:

#endif //The code above can be disabled if we don't actually use a music player, but this
//section here must be present anyway, since we need to implement things hud.h and hud.cpp
//expect us to implement.

DECLARE_MESSAGE(m_MusicPlayer, CMusicOpen );

int CHudMusic :: Init( void ){
	HOOK_MESSAGE( CMusicOpen );
	return 1;
}

int CHudMusic :: MsgFunc_CMusicOpen ( const char *pszName, int iSize, void *pbuf ){

	char* fnString;
	char filename[512];
	int filetype = 0;

	BEGIN_READ( pbuf, iSize );
	filetype = READ_BYTE();
	fnString = READ_STRING();

	sprintf(filename,"%s",fnString);

#ifndef DISABLE_MINIAUDIO //This part must only be present if we actually have a music player.
	//gEngfuncs.Con_Printf ( "MUSICPLAYER : Received message from server: File type is %d, file name is: %s\n", filetype, filename );

	if(filetype != MUSIC_AUDIO_FILE)
		g_MusicPlayer.OpenList ( filename );
	else
		g_MusicPlayer.OpenFile ( filename, 1 );
	g_MusicPlayer.Play();

#else //Otherwise we do nothing here.
	//gEngfuncs.Con_Printf ( "MUSICPLAYER : Received message, but player is disabled. Discarding.\n" );
#endif

	return 1;
}

#ifndef DISABLE_MINIAUDIO //The code below can, once again, be disabled if we don't
//actually use a music player.

//Define callback, so we can use it during init. The implementation can be found below.
void CMusic_DecoderCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

//---------------------------------------------------------
// initialisation

void CMusic :: Init ( void )
{
	if( m_bInit == true ){
		return; //Do not re-init.
	}

	deviceConfig = ma_device_config_init(ma_device_type_playback);
	deviceConfig.playback.format   = SAMPLE_FORMAT;
	deviceConfig.playback.channels = CHANNEL_COUNT;
	deviceConfig.sampleRate        = SAMPLE_RATE;
	deviceConfig.dataCallback      = CMusic_DecoderCallback; // this contains the callback that monitors the end of the song
	deviceConfig.pUserData         = NULL;

	if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
		gEngfuncs.Con_Printf ( "MUSICPLAYER : unable to initialize\n" );
		return;
	}

	m_bInit = true;
	return;
}

//---------------------------------------------------------
// Callback being called during playback

void CMusic_DecoderCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	if(g_MusicPlayer.m_IsPlaying == false){
		return; //We are paused or stopped, let's exit now.
	}

	ma_decoder* pDecoder = (ma_decoder*)&g_MusicPlayer.decoder;
	if (pDecoder == NULL) {
		return;
	}

	if(frameCount<=0) return;

	ma_uint64 framesRead;

	ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, &framesRead);
	if(framesRead < frameCount) //This happens when the song ends.
		g_MusicPlayer.songEnd();

	(void)pInput;
}

//---------------------------------------------------------
// playing an audio file


void CMusic :: OpenFile ( char *filename, int repeat )
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


void CMusic :: OpenList ( char *filename )
{
	
	// open text file

	FILE *myfile = fopen ( filename, "r" );

	if ( myfile == NULL )
	{
		gEngfuncs.Con_Printf ( "MUSICPLAYER : impossible to load %s\n", filename );
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
		gEngfuncs.Con_Printf ( "MUSICPLAYER : no song in the list\n" );
		return; 
	}

	// decrease repeat count

	p->repeat --;

	// removal of songs whose repeats ran off

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
		g_MusicPlayer.Reset();
	}

	// next track start

	else
	{
		g_MusicPlayer.Play();
	}

	return;
}


//---------------------------------------------------------
// initiate playback


void CMusic :: Play	( void )
{
	if ( m_IsPlaying == true )
		return;

	if ( m_bInit == false )
	{
		Init ();

		if ( m_bInit == false )
		{
			gEngfuncs.Con_Printf ( "MUSICPLAYER : unable to initialize\n" );
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
		gEngfuncs.Con_Printf ( "MUSICPLAYER : no song in the list\n" );
		return; 
	}

	//Stop playback
	m_IsPlaying = false; //Pause playback.
	ma_decoder_seek_to_pcm_frame(&decoder, 0); //Reset the file to start.

	// loading file
	char payload [512];
	sprintf(payload, "%s", p->name);

	gEngfuncs.Con_Printf ( "MUSICPLAYER : Opening file %s.\n", payload );

	result = ma_decoder_init_file(payload, NULL, &decoder);
	if (result != MA_SUCCESS) {
		gEngfuncs.Con_Printf ( "MUSICPLAYER : %s : can not load file\n", p->name );
		return;
	}

	//If the new track has different properties to the previous one.
	if(
		deviceConfig.playback.format != decoder.outputFormat ||
		deviceConfig.playback.channels != decoder.outputChannels ||
		deviceConfig.sampleRate != decoder.outputSampleRate
	){
		deviceConfig.playback.format   = decoder.outputFormat; //Change device settings
		deviceConfig.playback.channels = decoder.outputChannels;
		deviceConfig.sampleRate        = decoder.outputSampleRate;

		gEngfuncs.Con_Printf ( "MUSICPLAYER : Changing format to %d, channels to %d and sample rate to %d.\n", deviceConfig.playback.format, deviceConfig.playback.channels, deviceConfig.sampleRate);
		
		//Now we need to recreate the device to apply.
		ma_device_uninit(&device); //This is crucial, failing to do this results in segFault.
		if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) { //Apply new config.
			gEngfuncs.Con_Printf ( "MUSICPLAYER : Failed to change playback device configuration.\n" );
			g_MusicPlayer.m_bInit = false; //We have been deinitialized. This is NOT ideal.
			return;
		}else
			gEngfuncs.Con_Printf ( "MUSICPLAYER : New configuration applied successfully.\n");
	}

	// playback
	if (ma_device_start(&device) != MA_SUCCESS) {
		gEngfuncs.Con_Printf ( "MUSICPLAYER : Failed to start playback device.\n" );
		m_IsPlaying = false; //Pause playback.
		ma_decoder_seek_to_pcm_frame(&decoder, 0); //Reset the file to start.
		return;
	}else{
		m_IsPlaying = true;
	}

	return;	
}


void CMusic :: Stop ( void )
{
	if ( m_IsPlaying == true )
	{
		m_IsPlaying = false; //Pause playback.
		ma_decoder_seek_to_pcm_frame(&decoder, 0); //Reset the file to start.
	}
}


void CMusic :: Reset ( void ) //Should instead be called "Next Track", but we keep Julien's naming.
{
	//Reset the player.
	if ( m_bInit == true )
		gEngfuncs.Con_Printf ( "MUSICPLAYER : Player reset.\n" );

	Stop();

	audiofile_t *p = NULL;
	
	while ( m_pTrack != NULL )
	{
		p = m_pTrack;
		m_pTrack = p->next;
		delete p;
	}
}

void CMusic :: Terminate ( void ) //Cleanup and dereference
{
	gEngfuncs.Con_Printf ( "MUSICPLAYER : Terminating and unloading.\n" );
	ma_device_uninit(&device);
	ma_decoder_uninit(&decoder);
	g_MusicPlayer.m_bInit = false;
}

#endif //End if #ifndef DISABLE_MINIAUDIO

/*//---------------
Playlist contents

example: music01.txt file:

//

3

monmod/sound/mp3/music01_debut.mp3		1
monmod/sound/mp3/music01_boucle.mp3		3
monmod/sound/mp3/music01_fin.mp3		1


//

composition :
	- total number of tracks
	- path of the first music file
	- times to repeat that file
	- path of the second
	- etc ...

*///---------------
