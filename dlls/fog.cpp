//---------------------------------------------------------
//---------------------------------------------------------
//-														---
//-					fog.cpp								---
//-														---
//---------------------------------------------------------
//---------------------------------------------------------
//- partie serveur de l'effet de brouillard				---
//---------------------------------------------------------


//---------------------------------------------------------
// inclusions

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"

extern int gmsgFog;

int	g_bFogUpdate;


//---------------------------------------------------------
// classe de l'entit



class CTriggerFog : public CPointEntity
{
public:

	void	Spawn		( void );

	void	KeyValue	( KeyValueData *pkvd );
	void	Use			( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );


	virtual int	Save	( CSave &save );
	virtual int	Restore	( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	float	m_flminDist;
	float	m_flmaxDist;
	float	m_flFadeInTime;
	float	m_flFadeOutTime;

	int		m_bActive;
};

LINK_ENTITY_TO_CLASS( trigger_fog, CTriggerFog );



TYPEDESCRIPTION CTriggerFog::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerFog, m_flminDist, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerFog, m_flmaxDist, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerFog, m_flFadeInTime, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerFog, m_flFadeOutTime, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerFog, m_bActive, FIELD_INTEGER ),
};



void CTriggerFog :: Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	m_bActive = 0;

	g_bFogUpdate = 0;
}

void CTriggerFog :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "mindist"))
	{
		m_flminDist = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "maxdist"))
	{
		m_flmaxDist = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fadein"))
	{
		m_flFadeInTime = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fadeout"))
	{
		m_flFadeOutTime = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CTriggerFog :: Use ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// value = 0 : activation
	// value = 18686 : update du client

	if ( value != 18686 )
	{
		CBaseEntity *pPlayer = NULL;
		pPlayer = UTIL_FindEntityByClassname( NULL, "player" );

		MESSAGE_BEGIN( MSG_ONE, gmsgFog, NULL, pPlayer->pev );

			if ( m_bActive == 0 )
			{
				WRITE_BYTE ( 1 );				// appartion
				WRITE_COORD ( m_flFadeInTime );		// temps
			}
			else
			{
				WRITE_BYTE ( 0 );				// disparition
				WRITE_COORD ( m_flFadeOutTime );	// temps
			}

			WRITE_COORD ( m_flminDist );				// distances
			WRITE_COORD ( m_flmaxDist );

			WRITE_COORD ( pev->rendercolor.x );			// couleur
			WRITE_COORD ( pev->rendercolor.y );
			WRITE_COORD ( pev->rendercolor.z );

		MESSAGE_END();

		m_bActive = m_bActive == 1 ? 0 : 1;
	}

	else if ( value == 18686 )
	{
		CBaseEntity *pPlayer = NULL;
		pPlayer = UTIL_FindEntityByClassname( NULL, "player" );

		MESSAGE_BEGIN( MSG_ONE, gmsgFog, NULL, pPlayer->pev );

			WRITE_BYTE ( m_bActive );
			WRITE_COORD ( 0 );
			WRITE_COORD ( m_flminDist );
			WRITE_COORD ( m_flmaxDist );
			WRITE_COORD ( pev->rendercolor.x );
			WRITE_COORD ( pev->rendercolor.y );
			WRITE_COORD ( pev->rendercolor.z );

		MESSAGE_END();
	}
}


//------------------------------------------
// sauvegarde et restauration


int CTriggerFog::Save( CSave &save )
{
	if ( !CPointEntity::Save(save) )
		return 0;

	return save.WriteFields( "CTriggerFog", this, m_SaveData, ARRAYSIZE(m_SaveData) );
}

int CTriggerFog::Restore( CRestore &restore )
{
	if ( !CPointEntity::Restore(restore) )
		return 0;

	int status = restore.ReadFields( "CTriggerFog", this, m_SaveData, ARRAYSIZE(m_SaveData) );

	//-----------------------

	g_bFogUpdate = 1;		// force le rafraichissement des donnees client

	//----------------------
	return status;
}
