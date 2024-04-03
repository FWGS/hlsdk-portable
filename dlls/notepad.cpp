#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "items.h"
#include "gamerules.h"


class CNotepad : public CBaseToggle
{
public:
	void Spawn( );
	void Precache( void );
	void EXPORT Off(void);
	void EXPORT Recharge(void);
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	ObjectCaps( void ) { return (CBaseToggle :: ObjectCaps() | FCAP_CONTINUOUS_USE) & ~FCAP_ACROSS_TRANSITION; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	char  m_iszText[256];
	int   m_iTitle;

	static	TYPEDESCRIPTION m_SaveData[];

	float m_flNextCharge; 
	int		m_iReactivate ; // DeathMatch Delay until reactvated
	int		m_iJuice;
	int		m_iOn;			// 0 = off, 1 = startup, 2 = going
	float   m_flSoundTime;
};

TYPEDESCRIPTION CNotepad::m_SaveData[] =
{
	DEFINE_FIELD( CNotepad, m_flNextCharge, FIELD_TIME),
	DEFINE_FIELD( CNotepad, m_iReactivate, FIELD_INTEGER),
	DEFINE_FIELD( CNotepad, m_iJuice, FIELD_INTEGER),
	DEFINE_FIELD( CNotepad, m_iOn, FIELD_INTEGER),
	DEFINE_FIELD( CNotepad, m_flSoundTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE( CNotepad, CBaseEntity );

LINK_ENTITY_TO_CLASS(func_notepad, CNotepad);

extern int gmsgNotepad;
extern int gmsgSparePlayer;

void CNotepad::KeyValue( KeyValueData *pkvd )
{
    if (FStrEq(pkvd->szKeyName, "dmdelay"))
	{
		m_iReactivate = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "title"))
	{
		m_iTitle = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );

	FStrEq(pkvd->szKeyName, "text");
	sprintf(m_iszText, "%s", pkvd->szValue);
}

void CNotepad::Spawn()
{
	Precache( );

	pev->solid		= SOLID_BSP;
	pev->movetype	= MOVETYPE_PUSH;

	UTIL_SetOrigin(pev, pev->origin);		// set size and link into world
	UTIL_SetSize(pev, pev->mins, pev->maxs);
	SET_MODEL(ENT(pev), STRING(pev->model) );
	m_iJuice = gSkillData.healthchargerCapacity;
	pev->frame = 0;			

}

void CNotepad::Precache()
{
}


void CNotepad::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{ 
	// Make sure that we have a caller
	if (!pActivator)
		return;
	// if it's not a player, ignore
	if ( !pActivator->IsPlayer() )
		return;


	pev->nextthink = pev->ltime + 0.25;
	SetThink(Off);

	// Time to recharge yet?

	if (m_flNextCharge >= gpGlobals->time)
		return;

	MESSAGE_BEGIN( MSG_ONE, gmsgNotepad, NULL, GetClassPtr((CBasePlayer *)pActivator->pev)->pev );
		WRITE_STRING( (char[256])m_iszText  );
		WRITE_BYTE( m_iTitle );
	MESSAGE_END();
	
	// govern the rate of charge
	m_flNextCharge = gpGlobals->time + 0.1;
}

void CNotepad::Recharge(void)
{
	pev->frame = 0;			
	SetThink( SUB_DoNothing );
}

void CNotepad::Off(void)
{
	m_iOn = 0;

	if ((!m_iJuice) &&  ( ( m_iReactivate = g_pGameRules->FlHealthChargerRechargeTime() ) > 0) )
	{
		pev->nextthink = pev->ltime + m_iReactivate;
		SetThink(Recharge);
	}
	else
		SetThink( SUB_DoNothing );
}
