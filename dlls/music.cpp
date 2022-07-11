//---------------------------------------------------------
//---------------------------------------------------------
//-														---
//-					music.cpp							---
//-														---
//---------------------------------------------------------
//---------------------------------------------------------
//-	by Roy, based on the code by JujU									-----------
//---------------------------------------------------------
//- fake and null mp3 player code for HL mod				-----------
//---------------------------------------------------------

/*//---------------

This code is a placeholder for systems that support neither gstreamer nor fmod.

*///---------------

#ifdef USE_GSTREAMER
#include "musicgstreamer.cpp"
#elif defined(USE_MINIAUDIO)
#include "musicminiaudio.cpp"
#elif defined(USE_FMOD)
#include "musicfmod.cpp"
#else

//---------------------------------------------------------
// inclusions

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "music.h"

CMusic g_MusicPlayer;


//Fake functions to have something to work with on Linux
//---------------------------------------------------------

void CMusic :: Init ( void ){}
void CMusic :: OpenFile ( const char *filename, int repeat ){}
void CMusic :: OpenList ( const char *filename ){}
signed char EndCallback ( void *stream, void *buff, int len, int param )
{
	return TRUE;
}
void CMusic :: Play	( void ){}
void CMusic :: Stop ( void ){}
void CMusic :: Reset ( void ){}

//---------------------------------------------------------
// The actual game entity



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
	return;
}
#endif