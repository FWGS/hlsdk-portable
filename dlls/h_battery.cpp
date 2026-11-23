/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== h_battery.cpp ========================================================

  battery-related code

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "skill.h"
#include "gamerules.h"

class CRecharge : public CBaseToggle
{
public:
	void Spawn() override;
	void Precache() override;
	void KeyValue( KeyValueData* data ) override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	int ObjectCaps() override { return FCAP_CONTINUOUS_USE; }
	void Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value ) override;
	void EXPORT Off();
	void EXPORT Recharge();

	static TYPEDESCRIPTION m_SaveData[];
	float	m_flNextCharge;
	int		m_iReactivate; // DeathMatch Delay until reactvated
	int		m_iJuice;
	int		m_iOn;			// 0 = off, 1 = startup, 2 = going
	float	m_flSoundTime;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( func_recharge, CRecharge );

TYPEDESCRIPTION CRecharge::m_SaveData[] =
{
	DEFINE_FIELD( CRecharge, m_flNextCharge, FIELD_TIME ),
	DEFINE_FIELD( CRecharge, m_iReactivate, FIELD_INTEGER ),
	DEFINE_FIELD( CRecharge, m_iJuice, FIELD_INTEGER ),
	DEFINE_FIELD( CRecharge, m_iOn, FIELD_INTEGER ),
	DEFINE_FIELD( CRecharge, m_flSoundTime, FIELD_TIME )
};

IMPLEMENT_SAVERESTORE( CRecharge, CBaseEntity );

void CRecharge::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "style" ) || FStrEq( data->szKeyName, "height" ) ||
		FStrEq( data->szKeyName, "value1" ) || FStrEq( data->szKeyName, "value2" ) ||
		FStrEq( data->szKeyName, "value3" ) )
	{
		data->fHandled = TRUE;
	}
	else if ( FStrEq( data->szKeyName, "dmdelay" ) )
	{
		m_iReactivate = atoi( data->szValue );
		data->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( data );
}

void CRecharge::Spawn()
{
	Precache();
	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;
	UTIL_SetOrigin( pev, pev->origin );		// set size and link into world
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	SET_MODEL( edict(), STRING( pev->model ) );
	m_iJuice = (int)gSkillData.suitchargerCapacity;
	pev->frame = 0;
}

void CRecharge::Precache()
{
	PRECACHE_SOUND( "items/suitcharge1.wav" );
	PRECACHE_SOUND( "items/suitchargeno1.wav" );
	PRECACHE_SOUND( "items/suitchargeok1.wav" );
}

void CRecharge::Use( CBaseEntity* activator, CBaseEntity*, USE_TYPE, float )
{
	// if it's not a player, ignore
	if ( !FClassnameIs( activator->pev, "player" ) )
		return;

	// if there is no juice left, turn it off
	if ( m_iJuice <= 0 )
	{
		pev->frame = 1;
		Off();
	}

	// if the player doesn't have the suit, or there is no juice left, make the deny noise
	if ( m_iJuice <= 0 || !( activator->pev->weapons & ( 1 << WEAPON_SUIT ) ) )
	{
		if ( gpGlobals->time >= m_flSoundTime )
		{
			m_flSoundTime = gpGlobals->time + 0.62;
			EMIT_SOUND( edict(), CHAN_ITEM, "items/suitchargeno1.wav", 0.85f, ATTN_NORM );
		}
		return;
	}
	pev->nextthink = pev->ltime + 0.25;
	SetThink( &CRecharge::Off );
	if ( m_flNextCharge < gpGlobals->time && activator )
	{
		m_hActivator = activator;
		if ( m_hActivator->IsPlayer() )
		{
			if ( !m_iOn )
			{
				m_iOn = 1;
				EMIT_SOUND( edict(), CHAN_ITEM, "items/suitchargeok1.wav", 0.85f, ATTN_NORM );
				m_flSoundTime = gpGlobals->time + 0.56;
			}
			if ( m_iOn == 1 && gpGlobals->time >= m_flSoundTime )
			{
				m_iOn = 2;
				EMIT_SOUND( edict(), CHAN_STATIC, "items/suitcharge1.wav", 0.85f, ATTN_NORM );
			}


			// charge the player
			if ( m_hActivator->pev->armorvalue < 100 )
			{
				--m_iJuice;
				m_hActivator->pev->armorvalue += 1;
				if ( m_hActivator->pev->armorvalue > 100 )
					m_hActivator->pev->armorvalue = 100;
			}

			// govern the rate of charge
			m_flNextCharge = gpGlobals->time + 0.1;
		}
	}
}

void CRecharge::Off()
{
	if ( m_iOn > 1 )
		STOP_SOUND( edict(), CHAN_STATIC, "items/suitcharge1.wav" );
	m_iOn = 0;
	if ( !m_iJuice && ( m_iReactivate = (int)g_pGameRules->FlHEVChargerRechargeTime() ) > 0 )
	{
		pev->nextthink = pev->ltime + m_iReactivate;
		SetThink( &CRecharge::Recharge );
	}
	else
		SetThink( &CRecharge::SUB_DoNothing );
}

void CRecharge::Recharge()
{
	m_iJuice = (int)gSkillData.suitchargerCapacity;
	pev->frame = 0;
	SetThink( &CRecharge::SUB_DoNothing );
}
