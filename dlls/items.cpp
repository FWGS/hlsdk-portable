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
/*

===== items.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "skill.h"
#include "items.h"
#include "gamerules.h"

extern int gmsgItemPickup;

class CWorldItem : public CBaseEntity
{
public:
	void KeyValue( KeyValueData *pkvd ); 
	void Spawn( void );
	int m_iType;
};

LINK_ENTITY_TO_CLASS( world_items, CWorldItem )

void CWorldItem::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "type" ) )
	{
		m_iType = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CWorldItem::Spawn( void )
{
	CBaseEntity *pEntity = NULL;

	switch( m_iType ) 
	{
	case 44: // ITEM_BATTERY:
		pEntity = CBaseEntity::Create( "item_battery", pev->origin, pev->angles );
		break;
	case 42: // ITEM_ANTIDOTE:
		pEntity = CBaseEntity::Create( "item_antidote", pev->origin, pev->angles );
		break;
	case 43: // ITEM_SECURITY:
		pEntity = CBaseEntity::Create( "item_security", pev->origin, pev->angles );
		break;
	case 45: // ITEM_SUIT:
		pEntity = CBaseEntity::Create( "item_suit", pev->origin, pev->angles );
		break;
	}

	if( !pEntity )
	{
		ALERT( at_console, "unable to create world_item %d\n", m_iType );
	}
	else
	{
		pEntity->pev->target = pev->target;
		pEntity->pev->targetname = pev->targetname;
		pEntity->pev->spawnflags = pev->spawnflags;
	}

	REMOVE_ENTITY( edict() );
}

void CItem::SetObjectCollisionBox( void )
{
	pev->absmin = pev->origin + Vector(-16, -16, 0);
	pev->absmax = pev->origin + Vector(16, 16, 16); 
}

void CItem::Spawn( void )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	SetTouch( &CItem::ItemTouch );

	if( DROP_TO_FLOOR(ENT( pev ) ) == 0 )
	{
		ALERT(at_error, "Item %s fell out of level at %f,%f,%f\n", STRING( pev->classname ), (double)pev->origin.x, (double)pev->origin.y, (double)pev->origin.z);
		UTIL_Remove( this );
		return;
	}
}

extern int gEvilImpulse101;

void CItem::ItemTouch( CBaseEntity *pOther )
{
	// if it's not a player, ignore
	if( !pOther->IsPlayer() )
	{
		return;
	}

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	// ok, a player is touching this item, but can he have it?
	if( !g_pGameRules->CanHaveItem( pPlayer, this ) )
	{
		// no? Ignore the touch.
		return;
	}

	if( MyTouch( pPlayer ) )
	{
		SUB_UseTargets( pOther, USE_TOGGLE, 0 );
		SetTouch( NULL );
		
		// player grabbed the item. 
		g_pGameRules->PlayerGotItem( pPlayer, this );
		if( g_pGameRules->ItemShouldRespawn( this ) == GR_ITEM_RESPAWN_YES )
		{
			Respawn(); 
		}
		else
		{
			UTIL_Remove( this );
		}
	}
	else if( gEvilImpulse101 )
	{
		UTIL_Remove( this );
	}
}

CBaseEntity* CItem::Respawn( void )
{
	SetTouch( NULL );
	pev->effects |= EF_NODRAW;

	UTIL_SetOrigin( pev, g_pGameRules->VecItemRespawnSpot( this ) );// blip to whereever you should respawn.

	SetThink( &CItem::Materialize );
	pev->nextthink = g_pGameRules->FlItemRespawnTime( this ); 
	return this;
}

void CItem::Materialize( void )
{
	if( pev->effects & EF_NODRAW )
	{
		// changing from invisible state to visible.
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150 );
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	SetTouch( &CItem::ItemTouch );
	SetThink( NULL );
}

#define SF_SUIT_SHORTLOGON		0x0001

class CItemSuit : public CItem
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/w_all_items3.mdl" );
		pev->body = 5;
		CItem::Spawn();
	}
	void Precache( void )
	{
		//PRECACHE_MODEL( "models/w_suit.mdl" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if( pPlayer->pev->weapons & ( 1<<WEAPON_SUIT ) )
			return FALSE;

		/*if( pev->spawnflags & SF_SUIT_SHORTLOGON )
			EMIT_SOUND_SUIT( pPlayer->edict(), "!HEV_A0" );		// short version of suit logon,
		else
			EMIT_SOUND_SUIT( pPlayer->edict(), "!HEV_AAx" );*/	// long version of suit logon

		pPlayer->pev->weapons |= ( 1 << WEAPON_SUIT );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( item_suit, CItemSuit )

class CItemHealthUper : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_all_items5.mdl");
		pev->body = 2;
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_SOUND( "rmxp/111-Heal07.wav" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( pPlayer->pev->deadflag != DEAD_NO )
		{
			return FALSE;
		}

		if(!pPlayer->HasMenuItem_Full())
		{
			pPlayer->MenuItem_add(13);

			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( STRING(pev->classname) );
			MESSAGE_END();

			return TRUE;		
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS(item_respawn, CItemHealthUper);

class CItemGodWater : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_all_items5.mdl");
		pev->body = 16;
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_SOUND( "rmxp/111-Heal07.wav" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( pPlayer->pev->deadflag != DEAD_NO )
		{
			return FALSE;
		}

		if(!pPlayer->HasMenuItem_Full())
		{
			pPlayer->MenuItem_add(21);

			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( STRING(pev->classname) );
			MESSAGE_END();

			return TRUE;		
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS(item_godwater, CItemGodWater);

class CItemNuke : public CItem
{
	void Spawn( void )
	{ 
		SET_MODEL(ENT(pev), "models/nuke_box.mdl");
		CItem::Spawn( );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( pPlayer->pev->deadflag != DEAD_NO )
		{
			return FALSE;
		}

		if(!pPlayer->HasMenuItem_Full())
		{
			pPlayer->MenuItem_add(23);

			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );

			//MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			//WRITE_STRING( STRING(pev->classname) );
			//MESSAGE_END();

			return TRUE;		
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS(item_nuke, CItemNuke);

class CItemArmor3 : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_all_items4.mdl");
		pev->body = 1;
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_SOUND( "items/ammopickup.wav" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( pPlayer->pev->deadflag != DEAD_NO )
		{
			return FALSE;
		}
	
		if (pPlayer->m_skill_maxarmor <= 200 && pPlayer->pev->armorvalue < pPlayer->m_skill_maxarmor || pPlayer->pev->armorvalue == 0)
		{
			pPlayer->m_skill_maxarmor = 200;
			pPlayer->pev->armorvalue = pPlayer->m_skill_maxarmor;

			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( STRING(pev->classname) );
			MESSAGE_END();

			return TRUE;		
		}
		else if(!pPlayer->HasMenuItem_Full())
		{
			pPlayer->MenuItem_add(12);
			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );
			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( STRING(pev->classname) );
			MESSAGE_END();

			return TRUE;
		}

		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS(item_armor3, CItemArmor3);

class CItemArmor2 : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_all_items1.mdl");
		pev->body = 1;
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_SOUND( "items/ammopickup.wav" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( pPlayer->pev->deadflag != DEAD_NO )
		{
			return FALSE;
		}

		if (pPlayer->m_skill_maxarmor <= 150 && pPlayer->pev->armorvalue < pPlayer->m_skill_maxarmor || pPlayer->pev->armorvalue == 0)
		{
			pPlayer->m_skill_maxarmor = 150;
			pPlayer->pev->armorvalue = pPlayer->m_skill_maxarmor;

			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( STRING(pev->classname) );
			MESSAGE_END();

			return TRUE;		
		}
		else if(!pPlayer->HasMenuItem_Full())
		{
			pPlayer->MenuItem_add(11);
			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );
			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( STRING(pev->classname) );
			MESSAGE_END();

			return TRUE;
		}

		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS(item_armor2, CItemArmor2);

class CItemArmor1 : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_all_items4.mdl");
		pev->body = 0;
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_SOUND( "items/ammopickup.wav" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( pPlayer->pev->deadflag != DEAD_NO )
		{
			return FALSE;
		}

		if (pPlayer->m_skill_maxarmor <= 100 && pPlayer->pev->armorvalue < pPlayer->m_skill_maxarmor || pPlayer->pev->armorvalue == 0)
		{
			pPlayer->m_skill_maxarmor = 100;
			pPlayer->pev->armorvalue = pPlayer->m_skill_maxarmor;

			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( STRING(pev->classname) );
			MESSAGE_END();

			return TRUE;		
		}
		else if(!pPlayer->HasMenuItem_Full())
		{
			pPlayer->MenuItem_add(9);
			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );
			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( STRING(pev->classname) );
			MESSAGE_END();

			return TRUE;
		}

		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS(item_armor1, CItemArmor1);

class CItemArmor4 : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_all_items2.mdl");
		pev->body = 10;
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_SOUND( "items/ammopickup.wav" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( pPlayer->pev->deadflag != DEAD_NO )
		{
			return FALSE;
		}

		if (pPlayer->m_skill_maxarmor <= 120 && pPlayer->pev->armorvalue < pPlayer->m_skill_maxarmor || pPlayer->pev->armorvalue == 0)
		{
			pPlayer->m_skill_maxarmor = 120;
			pPlayer->pev->armorvalue = pPlayer->m_skill_maxarmor;

			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( STRING(pev->classname) );
			MESSAGE_END();

			return TRUE;		
		}
		else if(!pPlayer->HasMenuItem_Full())
		{
			pPlayer->MenuItem_add(10);
			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );
			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( STRING(pev->classname) );
			MESSAGE_END();

			return TRUE;
		}

		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS(item_armor4, CItemArmor4);

class CItemLevelUper : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_all_items5.mdl");
		pev->body = 5;
		pev->effects		= EF_DIMLIGHT;
		if ( FClassnameIs(pev, "item_leveluper_s")){
		pev->effects		|= EF_BRIGHTFIELD;
		}
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_SOUND( "items/ammopickup.wav" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( pPlayer->pev->deadflag != DEAD_NO )
		{
			return FALSE;
		}

		if (pPlayer->m_kadoma_level < 100)
		{
			if ( FClassnameIs(pev, "item_leveluper_s"))
			{
				pPlayer->m_kadoma_exp += 20000;
			}
			else
			{
				pPlayer->m_kadoma_exp += 1000;
			}

			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );

			return TRUE;		
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS(item_leveluper, CItemLevelUper);
LINK_ENTITY_TO_CLASS(item_leveluper_s, CItemLevelUper);

class CItemBattery : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL( ENT( pev ), "models/w_all_items3.mdl" );
		pev->body = 6;
		CItem::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_SOUND( "items/gunpickup2.wav" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( item_battery, CItemBattery )

class CItemFlashlight : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_all_items4.mdl");
		pev->body = 18;
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_SOUND( "items/gunpickup2.wav" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( pPlayer->pev->deadflag != DEAD_NO )
		{
			return FALSE;
		}

		if (!pPlayer->m_hasflashlight)
		{
			if(!pPlayer->HasMenuItem_Full())
			{
				pPlayer->MenuItem_add(1);
				EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );

				pPlayer->m_hasflashlight = TRUE;

				return TRUE;		
			}
			else
			{
				return FALSE;
			}
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS(item_flashlight, CItemFlashlight);

class CItemAntidote : public CItem
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/w_antidote.mdl" );
		CItem::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_antidote.mdl" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		//pPlayer->SetSuitUpdate( "!HEV_DET4", FALSE, SUIT_NEXT_IN_1MIN );

		pPlayer->m_rgItems[ITEM_ANTIDOTE] += 1;
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( item_antidote, CItemAntidote )

class CItemSecurity : public CItem
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/w_security.mdl" );
		CItem::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_security.mdl" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		pPlayer->m_rgItems[ITEM_SECURITY] += 1;
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( item_security, CItemSecurity )

class CItemLongJump : public CItem
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/w_longjump.mdl" );
		CItem::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_longjump.mdl" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if( pPlayer->m_fLongJump )
		{
			return FALSE;
		}

		if( ( pPlayer->pev->weapons & ( 1 << WEAPON_SUIT ) ) )
		{
			pPlayer->m_fLongJump = TRUE;// player now has longjump module

			g_engfuncs.pfnSetPhysicsKeyValue( pPlayer->edict(), "slj", "1" );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
				WRITE_STRING( STRING( pev->classname ) );
			MESSAGE_END();

			EMIT_SOUND_SUIT( pPlayer->edict(), "!HEV_A1" );	// Play the longjump sound UNDONE: Kelly? correct sound?
			return TRUE;		
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( item_longjump, CItemLongJump )
