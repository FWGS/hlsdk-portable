//-------------------------------------------------------------
//-------------------------------------------------------------
//-
//-				music.cpp
//-
//-------------------------------------------------------------
//-------------------------------------------------------------
//- by Roy at suggestion by nekonomicon, based on code by JujU
//-------------------------------------------------------------
//- mp3 player code for HL mod; trigger_music implementation
//-------------------------------------------------------------
//-
//- This is the server-side code.
//- It implements trigger_music, which simply informs the
//- client when and what music needs to be played.
//- No actual playback happens here.
//- We just send a message containing file type and filename.
//-
//-------------------------------------------------------------

//---------------------------------------------------------
// inclusions

#include "extdll.h"
#include "util.h"
#include "cbase.h"

extern int gmsgCMusicMessage; //This is simply a "handle" for the message. It's defined in player.cpp, can be defined here, but we'll follow the conventions.


//---------------------------------------------------------
// entity class



class CTriggerMusic : public CPointEntity
{
public:

	void	Spawn		( void );

	void	KeyValue	( KeyValueData *pkvd );
	void	Use			( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );


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
	MESSAGE_BEGIN( MSG_ALL, gmsgCMusicMessage, NULL ); //Inform the client side, we have some music to play.
		WRITE_BYTE( m_iFileType ); //Send file type.
		WRITE_STRING( STRING(m_iFileName) ); //Send file name.
	MESSAGE_END();
}

/*
FGD file entity code 


@PointClass base( Targetname ) = trigger_music : "Trigger Music"
[
	filetype(choices) : "File type" : 0 = 
	[
		0: "File list (*.txt)"
		1: "File wav mp2 mp3 ogg raw"
	]
	filename(string) : "Name (mod/folder/file.extension)"
]

*/