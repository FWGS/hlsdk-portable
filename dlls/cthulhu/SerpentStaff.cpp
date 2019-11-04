/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

#include "snake.h"

enum squeak_e {
	SERPENT_STAFF_DRAW = 0,
	SERPENT_STAFF_IDLE1,
	SERPENT_STAFF_IDLE2,
	SERPENT_STAFF_CAST,
	SERPENT_STAFF_HOLSTER
};


#include "SerpentStaff.h"


LINK_ENTITY_TO_CLASS( weapon_serpentstaff, CSerpentStaff );


void CSerpentStaff::Spawn( )
{
	Precache( );
	m_iId = WEAPON_SERPENT_STAFF;
	SET_MODEL(ENT(pev), "models/w_serpentstaff.mdl");

	FallInit();//get ready to fall down.

	m_iDefaultAmmo = SERPENT_STAFF_DEFAULT_GIVE;
		
	pev->sequence = 1;
	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0;
}


void CSerpentStaff::Precache( void )
{
	PRECACHE_MODEL("models/w_serpentstaff.mdl");
	PRECACHE_MODEL("models/v_serpentstaff.mdl");
//	PRECACHE_MODEL("models/p_serpentstaff.mdl");
	UTIL_PrecacheOther("monster_snake");
}


int CSerpentStaff::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Serpent Staff";
	p->iMaxAmmo1 = SERPENT_STAFF_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_SERPENT_STAFF;
	p->iWeight = SERPENT_STAFF_WEIGHT;
	p->iFlags = 0;

	return 1;
}



BOOL CSerpentStaff::Deploy( )
{
	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

	return DefaultDeploy( "models/v_serpentstaff.mdl", "", SERPENT_STAFF_DRAW, "serpent_staff" );
}


void CSerpentStaff::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;
	
	if (!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_SERPENT_STAFF);
		SetThink( DestroyItem );
		SetNextThink( 0.1 );
		return;
	}
	
	SendWeaponAnim( SERPENT_STAFF_HOLSTER );
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

int CSerpentStaff::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

void CSerpentStaff::PrimaryAttack()
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		TraceResult tr;
		Vector trace_origin;

		// HACK HACK:  Ugly hacks to handle change in origin based on new physics code for players
		// Move origin up if crouched and start trace a bit outside of body ( 20 units instead of 16 )
		trace_origin = m_pPlayer->pev->origin;
		if ( m_pPlayer->pev->flags & FL_DUCKING )
		{
			trace_origin = trace_origin - ( VEC_HULL_MIN - VEC_DUCK_HULL_MIN );
		}

		// find place to toss monster - this is some horrible code I have written here...
//		UTIL_TraceLine( trace_origin + gpGlobals->v_forward * 20, trace_origin + gpGlobas->v_forward * 64, dont_ignore_monsters, NULL, &tr );
		// firstly check all areas within the box to be created
		int box = 24;
		int spawndist = 64;
		BOOL b1, b2, b3, b4;
		UTIL_TraceLine( trace_origin + gpGlobals->v_forward * 20, trace_origin + gpGlobals->v_forward * spawndist + Vector(box,box,0), dont_ignore_monsters, NULL, &tr );
		b1 = (tr.fAllSolid == 0 && tr.fStartSolid == 0 && tr.flFraction > 0.99);
		UTIL_TraceLine( trace_origin + gpGlobals->v_forward * 20, trace_origin + gpGlobals->v_forward * spawndist + Vector(-box,-box,0), dont_ignore_monsters, NULL, &tr );
		b2 = (tr.fAllSolid == 0 && tr.fStartSolid == 0 && tr.flFraction > 0.99);
		UTIL_TraceLine( trace_origin + gpGlobals->v_forward * 20, trace_origin + gpGlobals->v_forward * spawndist + Vector(-box,box,0), dont_ignore_monsters, NULL, &tr );
		b3 = (tr.fAllSolid == 0 && tr.fStartSolid == 0 && tr.flFraction > 0.99);
		UTIL_TraceLine( trace_origin + gpGlobals->v_forward * 20, trace_origin + gpGlobals->v_forward * spawndist + Vector(box,-box,0), dont_ignore_monsters, NULL, &tr );
		b4 = (tr.fAllSolid == 0 && tr.fStartSolid == 0 && tr.flFraction > 0.99);
		UTIL_TraceLine( trace_origin + gpGlobals->v_forward * 20, trace_origin + gpGlobals->v_forward * spawndist, dont_ignore_monsters, NULL, &tr );

		// if we are clear at all corners of the box the snake is going to be created in
		if (b1 && b2 && b3 && b4)
		{
			SendWeaponAnim( SERPENT_STAFF_CAST );

			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

			Vector temp;
			temp.x = temp.y = temp.z = 0.0;
			temp.y = m_pPlayer->pev->v_angle.y;

			//CBaseEntity *pSnake = CBaseEntity::Create( "monster_snake", tr.vecEndPos, temp, m_pPlayer->edict() );
			CBaseEntity *pSnake = CBaseEntity::Create( "monster_snake", tr.vecEndPos, temp );
			((CSnake*)pSnake)->SetOwner(m_pPlayer);
			pSnake->pev->spawnflags |= 1;
			pSnake->pev->spawnflags |= SF_MONSTER_NO_YELLOW_BLOBS;
			// drop to floor, to check if we have landed on top of an entity...
			DROP_TO_FLOOR( ENT( pSnake->pev ) );

			CBaseEntity *pList[2];
			int buf = 24;
			// if there are any no other entities nearby (particularly under it!), then...
			int count = UTIL_EntitiesInBox( pList, 2, pSnake->pev->origin - Vector(buf,buf,buf), pSnake->pev->origin + Vector(buf,buf,buf), FL_CLIENT|FL_MONSTER );
			//...put the snake there and decrement ammo
			if ( count <= 1 )
			{
				pSnake->pev->velocity = gpGlobals->v_forward * 100 + m_pPlayer->pev->velocity;

				m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

				m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
			}
			// otherwise we do not want this snake...as it will cause yellow blobs!
			else
			{
				UTIL_Remove(pSnake);
			}

			m_fJustThrown = 1;

			m_flNextPrimaryAttack = gpGlobals->time + 0.5;
			m_flTimeWeaponIdle = gpGlobals->time + 0.5;
		}
	}
}


void CSerpentStaff::SecondaryAttack( void )
{

}


void CSerpentStaff::WeaponIdle( void )
{
	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	if (m_fJustThrown)
	{
		m_fJustThrown = 0;

		if ( !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] )
		{
			RetireWeapon();
			return;
		}

		//SendWeaponAnim( SERPENT_STAFF_DRAW );
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 3,5 );
		return;
	}

	int iAnim;

	float flRand = RANDOM_FLOAT(0, 1);
	if (flRand <= 0.75)
	{
		iAnim = SERPENT_STAFF_IDLE1;
		m_flTimeWeaponIdle = gpGlobals->time + 30.0 / 16 * (2);
	}
	else
	{
		iAnim = SERPENT_STAFF_IDLE2;
		m_flTimeWeaponIdle = gpGlobals->time + 70.0 / 16.0;
	}

	SendWeaponAnim( iAnim );
}

#endif