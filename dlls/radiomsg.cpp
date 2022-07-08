//-------------------------------------------------
//-												---
//-			radiomsg.cpp						---
//-												---
//-------------------------------------------------
//			par Julien		-----------------------
//-------------------------------------------------
//- code serveur de la radio du hud   -------------
//-------------------------------------------------


//----------------------------------------
// inclusions

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"


extern int gmsgRadioMsg;

#define SCANNE_CHAR if ( fscanf ( myfile, "%s", cscan ) == EOF ) break

#define TEXT_FILE_PATH			"invasion/texts.txt" //Backward slash replaced with forward slash modif de Roy



int	GetRadiomsgText ( int iszMessage )
{
	// ouverture du fichier texte

	char filename [128];
	sprintf ( filename, TEXT_FILE_PATH );

	char messagename [62];
	sprintf ( messagename, STRING (iszMessage) );


	FILE *myfile = fopen ( filename, "r" );

	if ( myfile == NULL )
	{
		ALERT ( at_console, "\\\nRADIOMSG : impossible d'ouvrir %s\n\\\n", filename );
		return 0;
	}


	char	cscan [128];
	char	messagetext [1000];
	int		startoffset = 0;
	int		stopoffset = 0;


	while ( 1 )
	{
		// titre

		SCANNE_CHAR;		
		if ( strcmp ( cscan, messagename ) != 0 ) continue;

		// point d'entr

		SCANNE_CHAR;
		if ( strcmp ( cscan, "{" ) != 0 ) continue;

		// offsets de d

		int startoffset = (int)ftell ( myfile );
		
		while ( 1 )
		{	
			SCANNE_CHAR;
			if ( strcmp ( cscan, "}" ) != 0 ) continue;
			break;
		}

		int stopoffset = (int)ftell ( myfile ) - 4;

		// r

		fseek ( myfile, startoffset, SEEK_SET );

		int i = 0;  //Loop iterator fix. Must be outside of the loop to still be accessible afterwards.
		for ( i=0; i<(int)(stopoffset-startoffset)+1; i++ ) //Added +1, stopoffset seems to be too low.
		{
			messagetext [i] = getc ( myfile );
		}

		messagetext [i] = '\0';
	}

	// fermeture du fichier texte

	fclose ( myfile );

	return ALLOC_STRING( messagetext );

}


//-----------------------------------------------------------------
//
//

class CRadiomsg : public CPointEntity
{
public:
	void	Spawn		( void );
	void	Precache	( void );

	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	KeyValue( KeyValueData *pkvd );

	int		m_iszSentence;
	int		m_iszMessage;
	int		m_iszText;
	int		m_iHead;

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

};

LINK_ENTITY_TO_CLASS( trigger_radio_message, CRadiomsg );


TYPEDESCRIPTION	CRadiomsg::m_SaveData[] = 
{
	DEFINE_FIELD( CRadiomsg, m_iszMessage, FIELD_STRING ),
	DEFINE_FIELD( CRadiomsg, m_iszSentence, FIELD_STRING ),
	DEFINE_FIELD( CRadiomsg, m_iszText, FIELD_STRING ),
	DEFINE_FIELD( CRadiomsg, m_iHead, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CRadiomsg, CPointEntity );



void CRadiomsg::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "radiomsg"))
	{
		m_iszMessage = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "head"))
	{
		m_iHead = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "sentence"))
	{
		m_iszSentence = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}


	else
		CPointEntity::KeyValue( pkvd );
}


void CRadiomsg :: Spawn( void )
{
	Precache ( );

	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

}


void CRadiomsg :: Precache ( void )
{
	m_iszText = GetRadiomsgText ( m_iszMessage );
}


void CRadiomsg :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{

	CBaseEntity *pPlayer = UTIL_FindEntityByClassname ( NULL,"player" );

	char	txt [256];
	sprintf ( txt, STRING(m_iszText));
	int		len = strlen ( txt );


	MESSAGE_BEGIN( MSG_ONE, gmsgRadioMsg, NULL, pPlayer->pev );

		WRITE_COORD ( gpGlobals->time );
		WRITE_LONG	( m_iHead );
		WRITE_LONG	( len );

		for ( int i=0; i<180; i++ ) {
			WRITE_BYTE	( txt[i] );	}


	MESSAGE_END();


	if ( FStringNull ( m_iszSentence ) )
		return;

//	EMIT_SOUND_SUIT(pPlayer->edict(), STRING(m_iszSentence) );

	EMIT_SOUND_DYN(pPlayer->edict(), CHAN_STATIC, STRING(m_iszSentence), 1.0, ATTN_NORM, 0, 100);


}
