
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"effects.h"

#define	SF_DROP_TO_FLOOR		32

//=========================================================
// Furniture - this is the cool comment I cut-and-pasted
//=========================================================
class CFurniture : public CBaseMonster
{
public:
	void Spawn ( void );
	void KeyValue( KeyValueData *pkvd );
	void Die( void );
	int	 Classify ( void );

	virtual int	ObjectCaps( void ) { return (CBaseMonster :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

private:
	Vector		m_vecSize;

};


LINK_ENTITY_TO_CLASS( monster_furniture, CFurniture );

TYPEDESCRIPTION	CFurniture::m_SaveData[] = 
{
	DEFINE_FIELD( CFurniture, m_vecSize, FIELD_VECTOR),
};

IMPLEMENT_SAVERESTORE( CFurniture, CBaseMonster );


//=========================================================
// Furniture is killed
//=========================================================
void CFurniture :: Die ( void )
{
	SetThink ( SUB_Remove );
	SetNextThink( 0 );
}

void CFurniture :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_vecSize"))
	{
		UTIL_StringToVector((float*)m_vecSize, pkvd->szValue);
		m_vecSize = m_vecSize/2;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

//=========================================================
// This used to have something to do with bees flying, but 
// now it only initializes moving furniture in scripted sequences
//=========================================================
void CFurniture :: Spawn( )
{
	PRECACHE_MODEL((char *)STRING(pev->model));
	SET_MODEL(ENT(pev),	STRING(pev->model));

	//pev->movetype	= MOVETYPE_NONE;
	pev->solid		= SOLID_BBOX;
	pev->health		= 80000;
	pev->takedamage = DAMAGE_AIM;
	pev->effects		= 0;
	pev->yaw_speed		= 0;
	pev->sequence		= 0;
	pev->frame			= 0;

	m_bloodColor = DONT_BLEED;

	if (pev->spawnflags & SF_DROP_TO_FLOOR)
	{
	    pev->movetype	= MOVETYPE_NONE;
	}
	else
	{
	    pev->movetype	= MOVETYPE_FLY;
		ClearBits( pev->spawnflags, SF_MONSTER_FALL_TO_GROUND );
	}

//	pev->nextthink += 1.0;
//	SetThink (WalkMonsterDelay);

	if (m_vecSize != Vector(0,0,0))
	{
		Vector vTemp;
		vTemp.z = m_vecSize.z;

		UTIL_SetSize( pev, -m_vecSize + vTemp, m_vecSize + vTemp );
	}
	
	// we probably do not need this...
	//pev->solid		= SOLID_SLIDEBOX;

	ResetSequenceInfo( );
	pev->frame = 0;
	MonsterInit();
}

//=========================================================
// ID's Furniture as neutral (noone will attack it)
//=========================================================
int CFurniture::Classify ( void )
{
	return m_iClass?m_iClass:CLASS_NONE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_CANDLE_FLAMES		8
#define	SF_CANDLE_STARTON		8
#define CANDLE_SPRITE_NAME		"sprites/flames.spr"



class CCandle : public CFurniture
{
public:
	void Spawn ( void );
	void KeyValue( KeyValueData *pkvd );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void	FlamesOn();
	void	FlamesOff();

	void EXPORT CandleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

private:
	int			m_nNumFlames;
	float		m_fFlameSize;
	CSprite*	m_pFlame[MAX_CANDLE_FLAMES];
};


LINK_ENTITY_TO_CLASS( monster_candle, CCandle );

TYPEDESCRIPTION	CCandle::m_SaveData[] = 
{
	DEFINE_FIELD( CCandle, m_nNumFlames, FIELD_INTEGER ),
	DEFINE_FIELD( CCandle, m_fFlameSize, FIELD_FLOAT ),
	DEFINE_ARRAY( CCandle, m_pFlame, FIELD_CLASSPTR, MAX_CANDLE_FLAMES ),
};

IMPLEMENT_SAVERESTORE( CCandle, CFurniture );

//////////////////////////////////////////////////

void CCandle :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "numflames"))
	{
		m_nNumFlames = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "flamesize"))
	{
		m_fFlameSize = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CFurniture::KeyValue( pkvd );
}

void CCandle :: Spawn( )
{
	for (int f = 0; f < MAX_CANDLE_FLAMES; f++)
	{
		m_pFlame[f] = NULL;
	}

	CFurniture::Spawn();

	SetUse(CandleUse);

	if ( pev->spawnflags & SF_CANDLE_STARTON )
	{
		FlamesOn();
	}
}

void CCandle::FlamesOff()
{
	for (int f = 0; f < MAX_CANDLE_FLAMES; f++)
	{
		if (m_pFlame) 
		{
			UTIL_Remove(m_pFlame[f]);
			m_pFlame[f] = NULL;
		}
	}
}

void CCandle::FlamesOn()
{
	Vector vecPosition;
	Vector vecJunk;

	for (int f = 0; f < m_nNumFlames; f++)
	{
		GetAttachment( f, vecPosition, vecJunk );

		m_pFlame[f] = CSprite::SpriteCreate( CANDLE_SPRITE_NAME, vecPosition, TRUE );
		m_pFlame[f]->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNone );
		m_pFlame[f]->SetAttachment( edict(), f+1 );
		m_pFlame[f]->pev->scale = m_fFlameSize;
		m_pFlame[f]->pev->framerate = 10.0;
		m_pFlame[f]->TurnOn();
	}
}

void CCandle::CandleUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// players cannot turn these on or off
	if ( pCaller && pCaller->IsPlayer() ) return;

	// do we have flames to turn on and off
	if (m_nNumFlames == 0) return;

	// are they already on
	if (m_pFlame[0])
	{
		FlamesOff();
	}
	else
	{
		FlamesOn();
	}
}

