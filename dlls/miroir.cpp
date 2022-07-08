//-------------------------------------------------
//-												---
//-				miroir.cpp						---
//-												---
//-------------------------------------------------
//			par Julien		-----------------------
//-------------------------------------------------
//- code du miroir de la s
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


#define	MIROIR_MAX_SEQ	5

char miroir_seqname [MIROIR_MAX_SEQ] [64] =
{
	"1_act",
	"2_act",
	"3_act",
	"4_act",
	"5_act",
};


class CMiroir : public CBaseAnimating
{
public:
	void Spawn( void );
	void Precache ( void );

	void EXPORT UseMiroir ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int	m_iPosition;

};


TYPEDESCRIPTION	CMiroir::m_SaveData[] = 
{
	DEFINE_FIELD( CMiroir, m_iPosition, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CMiroir, CBaseAnimating );

LINK_ENTITY_TO_CLASS( env_outromiroir , CMiroir );


void CMiroir::Precache( void )
{
	PRECACHE_MODEL("models/mapmodels/outro_mirror.mdl");
}

void CMiroir :: Spawn( void )
{
	Precache();

	SET_MODEL(ENT(pev), "models/mapmodels/outro_mirror.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	UTIL_SetOrigin( pev, pev->origin );

	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_SLIDEBOX;

	m_iPosition = MIROIR_MAX_SEQ-1;

	SetUse ( &CMiroir::UseMiroir );

	// initialisation de l'anim
	UseMiroir ( this,this, USE_ON, 0 );
}




void CMiroir :: UseMiroir ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	pev->sequence = LookupSequence( miroir_seqname [m_iPosition] );

	m_iPosition = (m_iPosition + 1) % MIROIR_MAX_SEQ;
}

