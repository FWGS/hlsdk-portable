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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "items.h"
#include "skill.h"
#include "gamerules.h"

class CHealthKit : public CItem
{
	void Spawn() override;
	void Precache() override;
	BOOL MyTouch( CBasePlayer* player ) override;
};

LINK_ENTITY_TO_CLASS( item_healthkitold, CHealthKit );

void CHealthKit::Spawn()
{
	Precache();
	SET_MODEL( edict(), "models/w_medkit.mdl" );
	CItem::Spawn();
}

void CHealthKit::Precache()
{
	PRECACHE_MODEL( "models/w_medkit.mdl" );
	PRECACHE_SOUND( "items/smallmedkit1.wav" );
}

BOOL CHealthKit::MyTouch( CBasePlayer* player )
{
	if ( player->pev->deadflag != DEAD_NO || !player->TakeHealth( gSkillData.healthkitCapacity, DMG_GENERIC ) )
		return FALSE;
	MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, player->pev );
		WRITE_STRING( STRING( pev->classname ) );
	MESSAGE_END();
	if ( !player->m_equipSilent )
		EMIT_SOUND( player->edict(), CHAN_ITEM, "items/smallmedkit1.wav", 1, ATTN_NORM );
	if ( g_pGameRules->ItemShouldRespawn( this ) )
		Respawn();
	else
		UTIL_Remove( this );
	return TRUE;
}



//-------------------------------------------------------------
// Wall mounted health kit
//-------------------------------------------------------------
class CWallHealth : public CBaseToggle
{
public:
	void Spawn() override;
	void Precache() override;
	void KeyValue( KeyValueData* data ) override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	int ObjectCaps() override { return FCAP_CONTINUOUS_USE; }
	void Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value ) override;

	static TYPEDESCRIPTION m_SaveData[];
	float	m_flNextCharge;
	int		m_iReactivate; // DeathMatch Delay until reactvated
	int		m_iJuice;
	int		m_iOn;			// 0 = off, 1 = startup, 2 = going
	float	m_flSoundTime;
	float	m_flUseTime;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( func_healthcharger, CWallHealth );

TYPEDESCRIPTION CWallHealth::m_SaveData[] =
{
	DEFINE_FIELD( CWallHealth, m_flNextCharge, FIELD_TIME ),
	DEFINE_FIELD( CWallHealth, m_iReactivate, FIELD_INTEGER ),
	DEFINE_FIELD( CWallHealth, m_iJuice, FIELD_INTEGER ),
	DEFINE_FIELD( CWallHealth, m_iOn, FIELD_INTEGER ),
	DEFINE_FIELD( CWallHealth, m_flSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CWallHealth, m_flUseTime, FIELD_TIME )
};

IMPLEMENT_SAVERESTORE( CWallHealth, CBaseEntity );

void CWallHealth::KeyValue( KeyValueData* data )
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
	else if ( FStrEq( data->szKeyName, "charges" ) )
	{
		m_iJuice = atoi( data->szValue );
		data->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( data );
}

void CWallHealth::Spawn()
{
	Precache();
	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;
	UTIL_SetOrigin( pev, pev->origin );		// set size and link into world
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	SET_MODEL( edict(), STRING( pev->model ) );
	if ( !m_iJuice )
		m_iJuice = 1;
	pev->frame = 0;
}

void CWallHealth::Precache()
{
	PRECACHE_SOUND( "items/medshot5.wav" );
	PRECACHE_SOUND( "items/medshotno1.wav" );
}

void CWallHealth::Use( CBaseEntity* activator, CBaseEntity*, USE_TYPE, float )
{
	if ( activator && activator->IsPlayer() && m_flUseTime <= gpGlobals->time )
	{
		m_flUseTime = gpGlobals->time + 1.5f;

		// if there is no juice left, turn it off
		if ( m_iJuice <= 0 )
			EMIT_SOUND( edict(), CHAN_ITEM, "items/medshotno1.wav", 1, ATTN_NORM );
		else if ( activator->TakeHealth( 50, DMG_GENERIC ) )
		{
			EMIT_SOUND( edict(), CHAN_ITEM, "items/medshot5.wav", 1, ATTN_NORM );
			if ( --m_iJuice <= 0 )
				pev->frame = 2;
		}
	}
}
