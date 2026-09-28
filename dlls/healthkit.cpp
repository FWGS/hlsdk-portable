/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "items.h"
#include "gamerules.h"
#include "game.h"

extern int gmsgItemPickup;

class CHealthKit : public CItem
{
	void Spawn( void );
	void Precache( void );
	BOOL MyTouch( CBasePlayer *pPlayer );
/*
	virtual int Save( CSave &save ); 
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
*/
};

LINK_ENTITY_TO_CLASS( item_healthkit, CHealthKit )
LINK_ENTITY_TO_CLASS( item_healthkit2, CHealthKit );

/*
TYPEDESCRIPTION	CHealthKit::m_SaveData[] =
{

};

IMPLEMENT_SAVERESTORE( CHealthKit, CItem )
*/

void CHealthKit::Spawn( void )
{
	Precache();
	if ( FClassnameIs( pev, "item_healthkit2" ) )
	{
		SET_MODEL(ENT(pev), "models/props_all.mdl");
		pev->body = 3;
	}
	else
	{
		SET_MODEL(ENT(pev), "models/w_all_items1.mdl");
		pev->body = 0;
	}

	CItem::Spawn();
}

void CHealthKit::Precache( void )
{
	PRECACHE_SOUND( "items/smallmedkit1.wav" );
}

BOOL CHealthKit::MyTouch( CBasePlayer *pPlayer )
{
	if( pPlayer->pev->deadflag != DEAD_NO )
	{
		return FALSE;
	}


	if ( FClassnameIs( pev, "item_healthkit2" ) )
	{
		if ( pPlayer->TakeHealth( 30 , DMG_GENERIC ) )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
				WRITE_STRING( "item_healthkit" );
			MESSAGE_END();

			EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/smallmedkit1.wav", 1, ATTN_NORM);

			pev->health += 1;
			if(pev->health >= 2)
			{
				if ( g_pGameRules->ItemShouldRespawn( this ) )
				{
					Respawn();
				}
				else
				{
					UTIL_Remove(this);	
				}
				return TRUE;
			}

		}
	}
	else
	{
		if( pPlayer->TakeHealth( 30, DMG_GENERIC ) )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
				WRITE_STRING( STRING( pev->classname ) );
			MESSAGE_END();

			EMIT_SOUND( ENT( pPlayer->pev ), CHAN_ITEM, "items/smallmedkit1.wav", 1, ATTN_NORM );

			if( g_pGameRules->ItemShouldRespawn( this ) )
			{
				Respawn();
			}
			else
			{
				UTIL_Remove( this );	
			}

			return TRUE;
		}
	}

	return FALSE;
}

//-------------------------------------------------------------
// Wall mounted health kit
//-------------------------------------------------------------
class CWallHealth : public CBaseToggle
{
public:
	void Spawn();
	void Precache( void );
	void EXPORT Off( void );
	void EXPORT Recharge( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int ObjectCaps( void ) { return ( CBaseToggle::ObjectCaps() | FCAP_CONTINUOUS_USE ) & ~FCAP_ACROSS_TRANSITION; }
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	float m_flNextCharge; 
	int m_iReactivate ; // DeathMatch Delay until reactvated
	int m_iJuice;
	int m_iOn;			// 0 = off, 1 = startup, 2 = going
	float m_flSoundTime;
};

TYPEDESCRIPTION CWallHealth::m_SaveData[] =
{
	DEFINE_FIELD( CWallHealth, m_flNextCharge, FIELD_TIME ),
	DEFINE_FIELD( CWallHealth, m_iReactivate, FIELD_INTEGER ),
	DEFINE_FIELD( CWallHealth, m_iJuice, FIELD_INTEGER ),
	DEFINE_FIELD( CWallHealth, m_iOn, FIELD_INTEGER ),
	DEFINE_FIELD( CWallHealth, m_flSoundTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CWallHealth, CBaseToggle )

LINK_ENTITY_TO_CLASS( func_healthcharger, CWallHealth )
LINK_ENTITY_TO_CLASS(func_healthcharger_cz, CWallHealth)
LINK_ENTITY_TO_CLASS(func_healthcharger_pointer, CWallHealth)

void CWallHealth::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq(pkvd->szKeyName, "style" ) ||
		FStrEq( pkvd->szKeyName, "height" ) ||
		FStrEq( pkvd->szKeyName, "value1" ) ||
		FStrEq( pkvd->szKeyName, "value2" ) ||
		FStrEq( pkvd->szKeyName, "value3" ) )
	{
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "dmdelay" ) )
	{
		m_iReactivate = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "juice"))
	{
		m_iJuice = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

void CWallHealth::Spawn()
{
	Precache();

	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;

	UTIL_SetOrigin( pev, pev->origin );		// set size and link into world
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );
	//m_iJuice = (int)gSkillData.healthchargerCapacity;
	pev->frame = 0;
	if ( FClassnameIs( pev, "func_healthcharger_cz" ) )
	{
		m_iJuice = 1;	
	}
}

void CWallHealth::Precache()
{
	PRECACHE_SOUND( "items/medshot4.wav" );
	PRECACHE_SOUND( "items/medshotno1.wav" );
	PRECACHE_SOUND( "items/medcharge4.wav" );
}

void CWallHealth::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{ 
	// Make sure that we have a caller
	if( !pActivator )
		return;
	// if it's not a player, ignore
	if( !pActivator->IsPlayer() )
		return;

	if ( FClassnameIs( pev, "func_healthcharger_cz" )  )
	{
		if(pev->frame == 0)
		{
		
			CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pActivator->pev);
			float take_health = pPlayer->pev->max_health * 0.4;

			if (g_iSkillLevel == SKILL_EASY)
			{
				take_health = pPlayer->pev->max_health * 0.5;
			}

			if ( pPlayer->TakeHealth( take_health , DMG_GENERIC ) )
			{
			//	EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/smallmedkit1.wav", 1, ATTN_NORM);
				FX_Explosion( pPlayer->Center(), 45);
				pPlayer->m_flVelocityModifier = 0;

				pev->frame = 1;			
				Off();
			}
		}

		return;
	}

	// if there is no juice left, turn it off
	if( m_iJuice <= 0 )
	{
		pev->body = 1;
		pev->frame = 1;			
		Off();
	}

	// if the player doesn't have the suit, or there is no juice left, make the deny noise
	if( ( m_iJuice <= 0 ) || ( !( pActivator->pev->weapons & ( 1 << WEAPON_SUIT ) ) ) || ( ( chargerfix.value ) && ( pActivator->pev->health >= pActivator->pev->max_health ) ) )
	{
		if( m_flSoundTime <= gpGlobals->time )
		{
			m_flSoundTime = gpGlobals->time + 0.62f;
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/medshotno1.wav", 1.0, ATTN_NORM );
		}
		return;
	}

	pev->nextthink = pev->ltime + 0.25f;
	SetThink( &CWallHealth::Off );

	// Time to recharge yet?
	if( m_flNextCharge >= gpGlobals->time )
		return;

	// Play the on sound or the looping charging sound
	if( !m_iOn )
	{
		m_iOn++;
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM );
		m_flSoundTime = 0.56f + gpGlobals->time;
	}
	if( ( m_iOn == 1 ) && ( m_flSoundTime <= gpGlobals->time ) )
	{
		m_iOn++;
		EMIT_SOUND( ENT( pev ), CHAN_STATIC, "items/medcharge4.wav", 1.0, ATTN_NORM );
	}

	// charge the player
	if( pActivator->TakeHealth( 1, DMG_GENERIC ) )
	{
		m_iJuice--;
	}

	// govern the rate of charge
	m_flNextCharge = gpGlobals->time + 0.05f;
}

void CWallHealth::Recharge( void )
{
	EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM );
	m_iJuice = 50;
	pev->frame = 0;			
	SetThink( &CBaseEntity::SUB_DoNothing );
}

void CWallHealth::Off( void )
{
	// Stop looping sound.
	if( m_iOn > 1 )
		STOP_SOUND( ENT( pev ), CHAN_STATIC, "items/medcharge4.wav" );

	m_iOn = 0;

	if( ( !m_iJuice ) && ( ( m_iReactivate = (int)g_pGameRules->FlHealthChargerRechargeTime() ) > 0 ) )
	{
		pev->nextthink = pev->ltime + m_iReactivate;
		SetThink( &CWallHealth::Recharge );
	}
	else
		SetThink( &CBaseEntity::SUB_DoNothing );
}
