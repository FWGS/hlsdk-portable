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

===== items.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "items.h"
#include "skill.h"
#include "gamerules.h"

extern int gEvilImpulse101;

class CWorldItem : public CBaseEntity
{
public:
	void Spawn() override;
	void KeyValue( KeyValueData* data ) override;
	int m_iType;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( world_items, CWorldItem );

void CWorldItem::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "type" ) )
	{
		m_iType = atoi( data->szValue );
		data->fHandled = TRUE;
	}
	else
		data->fHandled = FALSE;
}

void CWorldItem::Spawn()
{
	CBaseEntity* item = NULL;
	switch ( m_iType )
	{
	case 42:
		item = Create( "item_antidote", pev->origin, pev->angles );
		break;
	case 43:
		item = Create( "item_security", pev->origin, pev->angles );
		break;
	case 44:
		item = Create( "item_battery", pev->origin, pev->angles );
		break;
	case 45:
		item = Create( "item_suit", pev->origin, pev->angles );
		break;
	}
	if ( item )
	{
		item->pev->target = pev->target;
		item->pev->targetname = pev->targetname;
		item->pev->spawnflags = pev->spawnflags;
	}
	else
		ALERT( at_console, "unable to create world_item %d\n", m_iType );
	REMOVE_ENTITY( edict() );
}

void CItem::Spawn()
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ) );
	SetTouch( &CItem::ItemTouch );
	if ( !DROP_TO_FLOOR( edict() ) )
	{
		ALERT( at_error, "Item %s fell out of level at %f,%f,%f", STRING( pev->classname ),
			pev->origin.x, pev->origin.y, pev->origin.z );
		UTIL_Remove( this );
	}
	else if ( CVAR_GET_FLOAT( "lightmultiplier" ) >= 0.1f )
	{
		pev->renderfx = kRenderFxLightMultiplier;
		pev->iuser4 = (int)CVAR_GET_FLOAT( "lightmultiplier" );
	}
}

int CItem::Save( CSave& save )
{
	return CBaseEntity::Save( save ) != 0;
}

int CItem::Restore( CRestore& restore )
{
	if ( !CBaseEntity::Restore( restore ) )
		return 0;
	if ( CVAR_GET_FLOAT( "lightmultiplier" ) >= 0.1f )
	{
		pev->renderfx = kRenderFxLightMultiplier;
		pev->iuser4 = (int)CVAR_GET_FLOAT( "lightmultiplier" );
	}
	return 1;
}

void CItem::ItemTouch( CBaseEntity* other )
{
	if ( !other->IsPlayer() || !g_pGameRules->CanHaveItem( (CBasePlayer*)other, this ) )
		return;
	if ( MyTouch( (CBasePlayer*)other ) )
	{
		SUB_UseTargets( other, USE_TOGGLE, 0 );
		SetTouch( NULL );
		g_pGameRules->PlayerGotItem( (CBasePlayer*)other, this );
		if ( g_pGameRules->ItemShouldRespawn( this ) == GR_ITEM_RESPAWN_YES )
			Respawn();
		else
			UTIL_Remove( this );
	}
	else if ( gEvilImpulse101 )
		UTIL_Remove( this );
}

CBaseEntity* CItem::Respawn()
{
	SetTouch( NULL );
	pev->effects |= EF_NODRAW;
	UTIL_SetOrigin( pev, g_pGameRules->VecItemRespawnSpot( this ) ); // blip to whereever you should respawn.
	SetThink( &CItem::Materialize );
	pev->nextthink = g_pGameRules->FlItemRespawnTime( this );
	return this;
}

void CItem::Materialize()
{
	if ( pev->effects & EF_NODRAW )
	{
		// changing from invisible state to visible.
		EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150 );
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}
	SetTouch( &CItem::ItemTouch );
}

class CItemSuit : public CItem
{
public:
	void Spawn() override;
	void Precache() override;
	BOOL MyTouch( CBasePlayer* player ) override;
};

LINK_ENTITY_TO_CLASS( item_suit, CItemSuit );

void CItemSuit::Spawn()
{
	Precache();
	SET_MODEL( edict(), "models/w_suit.mdl" );
	CItem::Spawn();
}

void CItemSuit::Precache()
{
	PRECACHE_MODEL( "models/w_suit.mdl" );
}

BOOL CItemSuit::MyTouch( CBasePlayer* player )
{
	if ( player->pev->weapons & ( 1 << WEAPON_SUIT ) )
		return FALSE;
	if ( pev->spawnflags & 1 )
		EMIT_SOUND_SUIT( player->edict(), "!HEV_A0" );
	else
		EMIT_SOUND_SUIT( player->edict(), "!HEV_AAx" );
	player->pev->weapons |= 1 << WEAPON_SUIT;
	return TRUE;
}

class CItemBattery : public CItem
{
public:
	void Spawn() override;
	void Precache() override;
	BOOL MyTouch( CBasePlayer* player ) override;
};

LINK_ENTITY_TO_CLASS( item_battery, CItemBattery );

void CItemBattery::Spawn()
{
	Precache();
	SET_MODEL( edict(), "models/w_battery.mdl" );
	CItem::Spawn();
}

void CItemBattery::Precache()
{
	PRECACHE_MODEL( "models/w_battery.mdl" );
	PRECACHE_SOUND( "items/gunpickup2.wav" );
}

BOOL CItemBattery::MyTouch( CBasePlayer* player )
{
	if ( player->pev->deadflag != DEAD_NO || !( player->pev->armorvalue < 100 ) ||
		!( player->pev->weapons & ( 1 << WEAPON_SUIT ) ) )
		return FALSE;
	player->pev->armorvalue += gSkillData.batteryCapacity;
	if ( player->pev->armorvalue > 100 )
		player->pev->armorvalue = 100;
	if ( !player->m_equipSilent )
		EMIT_SOUND( player->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );
	MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, player->pev );
		WRITE_STRING( STRING( pev->classname ) );
	MESSAGE_END();
	int charge = (int)( 100.0 * player->pev->armorvalue * 0.01 + 0.5 ) / 5;
	if ( charge > 0 )
		charge--;
	char sentence[64];
	sprintf( sentence, "!HEV_%1dP", charge );
	player->SetSuitUpdate( sentence, FALSE, 30 );
	return TRUE;
}

class CItemArmor : public CItem
{
public:
	void Spawn() override;
	void Precache() override;
	BOOL MyTouch( CBasePlayer* player ) override;
};

LINK_ENTITY_TO_CLASS( item_armor, CItemArmor );

void CItemArmor::Spawn()
{
	Precache();
	SET_MODEL( edict(), "models/w_armor.mdl" );
	CItem::Spawn();
}

void CItemArmor::Precache()
{
	PRECACHE_MODEL( "models/w_armor.mdl" );
	PRECACHE_SOUND( "items/gunpickup2.wav" );
}

BOOL CItemArmor::MyTouch( CBasePlayer* player )
{
	if ( player->pev->deadflag != DEAD_NO || !( player->pev->armorvalue < 100 ) )
		return FALSE;
	player->pev->armorvalue = 100;
	if ( !player->m_equipSilent )
		EMIT_SOUND( player->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );
	MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, player->pev );
		WRITE_STRING( "item_battery" );
	MESSAGE_END();
	char sentence[64];
	player->SetSuitUpdate( sentence, FALSE, 30 );
	player->m_iKevlar = 1;
	return TRUE;
}

class CItemAntidote : public CItem
{
public:
	void Spawn() override;
	void Precache() override;
	BOOL MyTouch( CBasePlayer* player ) override;
};

LINK_ENTITY_TO_CLASS( item_antidote, CItemAntidote );

void CItemAntidote::Spawn()
{
	Precache();
	SET_MODEL( edict(), "models/w_antidote.mdl" );
	CItem::Spawn();
}

void CItemAntidote::Precache()
{
	PRECACHE_MODEL( "models/w_antidote.mdl" );
}

BOOL CItemAntidote::MyTouch( CBasePlayer* player )
{
	player->SetSuitUpdate( "!HEV_DET4", FALSE, 60 );
	player->m_rgItems[2]++;
	return TRUE;
}

class CItemSecurity : public CItem
{
public:
	void Spawn() override;
	void Precache() override;
	BOOL MyTouch( CBasePlayer* player ) override;
};

LINK_ENTITY_TO_CLASS( item_security, CItemSecurity );

void CItemSecurity::Spawn()
{
	Precache();
	SET_MODEL( edict(), "models/w_security.mdl" );
	CItem::Spawn();
}

void CItemSecurity::Precache()
{
	PRECACHE_MODEL( "models/w_security.mdl" );
}

BOOL CItemSecurity::MyTouch( CBasePlayer* player )
{
	player->m_rgItems[3]++;
	return TRUE;
}

class CItemLongJump : public CItem
{
public:
	void Spawn() override;
	void Precache() override;
	BOOL MyTouch( CBasePlayer* player ) override;
};

LINK_ENTITY_TO_CLASS( item_longjump, CItemLongJump );

void CItemLongJump::Spawn()
{
	Precache();
	SET_MODEL( edict(), "models/w_longjump.mdl" );
	CItem::Spawn();
}

void CItemLongJump::Precache()
{
	PRECACHE_MODEL( "models/w_longjump.mdl" );
}

BOOL CItemLongJump::MyTouch( CBasePlayer* player )
{
	if ( player->m_fLongJump || !( player->pev->weapons & ( 1 << WEAPON_SUIT ) ) )
		return FALSE;
	player->m_fLongJump = TRUE;
	g_engfuncs.pfnSetPhysicsKeyValue( player->edict(), "slj", "1" );
	MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, player->pev );
		WRITE_STRING( STRING( pev->classname ) );
	MESSAGE_END();
	if ( !player->m_equipSilent )
		EMIT_SOUND_SUIT( player->edict(), "!HEV_A1" );
	return TRUE;
}

class CItemHealthKit : public CBasePlayerAmmo
{
public:
	void Spawn() override;
	void Precache() override;
	BOOL AddAmmo( CBaseEntity* other ) override;
};

LINK_ENTITY_TO_CLASS( item_healthkit, CItemHealthKit );

void CItemHealthKit::Spawn()
{
	Precache();
	SET_MODEL( edict(), "models/healthkit.mdl" );
	CBasePlayerAmmo::Spawn();
}

void CItemHealthKit::Precache()
{
	PRECACHE_MODEL( "models/healthkit.mdl" );
	PRECACHE_SOUND( "items/healthpickup.wav" );
}

BOOL CItemHealthKit::AddAmmo( CBaseEntity* other )
{
	if ( !other || other->pev->health == 100 )
		return FALSE;
	other->TakeHealth( 25, DMG_GENERIC );
	EMIT_SOUND( edict(), CHAN_ITEM, "items/healthpickup.wav", 1, ATTN_NORM );
	return TRUE;
}

class CWorldMap : public CBaseToggle
{
public:
	void Spawn() override;
	void Precache() override;
	void KeyValue( KeyValueData* data ) override;
	void Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value ) override;
	int ObjectCaps() override { return FCAP_ONOFF_USE; }
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	BOOL m_On;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( item_worldmap, CWorldMap );

TYPEDESCRIPTION CWorldMap::m_SaveData[] =
{
	DEFINE_FIELD( CWorldMap, m_On, FIELD_INTEGER )
};

IMPLEMENT_SAVERESTORE( CWorldMap, CBaseEntity );

void CWorldMap::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "junk" ) )
		data->fHandled = TRUE;
	else
		CBaseToggle::KeyValue( data );
}

void CWorldMap::Spawn()
{
	Precache();
	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	SET_MODEL( edict(), STRING( pev->model ) );
}

void CWorldMap::Precache()
{
}

void CWorldMap::Use( CBaseEntity* activator, CBaseEntity*, USE_TYPE, float )
{
	if ( activator && activator->IsPlayer() )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWorldMap, NULL, activator->pev );
		MESSAGE_END();
	}
}
