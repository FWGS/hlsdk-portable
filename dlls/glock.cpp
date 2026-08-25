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

#ifndef CLIENT_DLL
#define BOLT_AIR_VELOCITY	1800
#define BOLT_WATER_VELOCITY	1200


class CPropglock : public CBaseEntity
{
	void Spawn(void);
	void Precache(void);
	int  Classify(void);
	void EXPORT BubbleThink(void);
	void EXPORT BoltTouch(CBaseEntity *pOther);
	void EXPORT ExplodeThink(void);
	int touchcounter = 0;

	int m_iTrail;

public:
	static CPropglock *BoltCreate(void);
};
LINK_ENTITY_TO_CLASS(propglock, CPropglock);

CPropglock *CPropglock::BoltCreate(void)
{
	// Create a new entity with CCrossbowBolt private data
	CPropglock *pBolt = GetClassPtr((CPropglock *)NULL);
	pBolt->pev->classname = MAKE_STRING("propglock");
	pBolt->Spawn();
	pBolt->touchcounter = 0;

	return pBolt;
}

void CPropglock::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.8;

	SET_MODEL(ENT(pev), "models/w_9mmhandgun.mdl");

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-1, -2, -1), Vector(1, 2, 1));

	SetTouch(&CPropglock::BoltTouch);
	SetThink(&CPropglock::BubbleThink);
	pev->nextthink = gpGlobals->time + 0.2;
}


void CPropglock::Precache()
{
	PRECACHE_MODEL("models/w_9mmhandgun.mdl");
	PRECACHE_SOUND("items/gunpickup4.wav");
	PRECACHE_SOUND("items/weapondrop1.wav");
	m_iTrail = PRECACHE_MODEL("sprites/streak.spr");
}


int	CPropglock::Classify(void)
{
	return	CLASS_NONE;
}

void CPropglock::BoltTouch(CBaseEntity *pOther)
{
	pev->angles.x = 0;
	pev->angles.z = 0;
	touchcounter++;

	if (touchcounter == 1)
	{
		SetThink(&CPropglock::ExplodeThink);
		pev->nextthink = gpGlobals->time + 3;
	}


	if (touchcounter > 50)
		UTIL_Remove(this);

	if (pev->velocity.z == 0)
	{
		UTIL_Remove(this);
	}

	if (pOther->edict() == pev->owner)
		return;
	// pev->avelocity = Vector (300, 300, 300);

	if (pev->flags & FL_ONGROUND)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.15;

		if (pev->velocity <= Vector(1, 1, 1))
		{
			UTIL_Remove(this);
		}
	}
	else
	{
		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "items/gunpickup4.wav", 0.35, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 7)); break;
		case 1:
			EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "items/weapondrop1.wav", 0.35, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 7)); break;
		}
		
	}
}

void CPropglock::BubbleThink(void)
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->waterlevel == 0)
		return;

	UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1, pev->origin, 1);
}

void CPropglock::ExplodeThink(void)
{
	UTIL_Remove(this);
}
#endif

enum glock_e
{
	GLOCK_IDLE1 = 0,
	GLOCK_IDLE2,
	GLOCK_IDLE3,
	GLOCK_SHOOT,
	GLOCK_SHOOT_EMPTY,
	GLOCK_RELOAD,
	GLOCK_RELOAD_NOT_EMPTY,
	GLOCK_DRAW,
	GLOCK_HOLSTER,
	GLOCK_ADD_SILENCER,
	GLOCK_SHOOT2
};

LINK_ENTITY_TO_CLASS( weapon_glock, CGlock )
LINK_ENTITY_TO_CLASS( weapon_9mmhandgun, CGlock )

void CGlock::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_9mmhandgun" ); // hack to allow for old names
	Precache();
	m_iId = WEAPON_GLOCK;
	SET_MODEL( ENT( pev ), "models/w_9mmhandgun.mdl" );

	m_iDefaultAmmo = GLOCK_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CGlock::Precache( void )
{
	PRECACHE_MODEL( "models/v_9mmhandgun.mdl" );
	PRECACHE_MODEL( "models/w_9mmhandgun.mdl" );
	PRECACHE_MODEL( "models/p_9mmhandgun.mdl" );

	m_iShell = PRECACHE_MODEL( "models/shell.mdl" );// brass shell

	PRECACHE_SOUND( "items/9mmclip1.wav" );
	PRECACHE_SOUND( "items/9mmclip2.wav" );

	PRECACHE_SOUND( "weapons/pl_gun1.wav" );//silenced handgun
	PRECACHE_SOUND( "weapons/pl_gun2.wav" );//silenced handgun
	PRECACHE_SOUND( "weapons/pl_gun3.wav" );//handgun

	m_usFireGlock1 = PRECACHE_EVENT( 1, "events/glock1.sc" );
	m_usFireGlock2 = PRECACHE_EVENT( 1, "events/glock2.sc" );

	UTIL_PrecacheOther("propglock");
}

int CGlock::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = GLOCK_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_GLOCK;
	p->iWeight = GLOCK_WEIGHT;

	return 1;
}

int CGlock::AddToPlayer( CBasePlayer *pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL CGlock::Deploy()
{
	// pev->body = 1;
	return DefaultDeploy( "models/v_9mmhandgun.mdl", "models/p_9mmhandgun.mdl", GLOCK_DRAW, "onehanded", /*UseDecrement() ? 1 : 0*/ 0 );
}

void CGlock::SecondaryAttack( void )
{
	GlockFire( 0.02f, 0.1f, FALSE );
}

void CGlock::PrimaryAttack( void )
{
	GlockFire( 0.01f, 0.2f, TRUE );
}

void CGlock::GlockFire( float flSpread, float flCycleTime, BOOL fUseAutoAim )
{
	if( m_iClip <= 0 )
	{
		if( m_fFireOnEmpty )
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = GetNextAttackDelay( 0.2f );
		}

		return;
	}

	m_iClip--;

	m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// silenced
	if( pev->body == 1 )
	{
		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
	}
	else
	{
		// non-silenced
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	}

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming;

	if( fUseAutoAim )
	{
		vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	}
	else
	{
		vecAiming = gpGlobals->v_forward;
	}

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_9MM, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), fUseAutoAim ? m_usFireGlock1 : m_usFireGlock2, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay( flCycleTime );

	if( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

	if (m_iClip % 2 == 0)
		SendWeaponAnim(GLOCK_SHOOT);
	else
		SendWeaponAnim(GLOCK_SHOOT2);
}


void EXPORT CGlock::spawnprop(void)
{
#ifndef CLIENT_DLL
	UTIL_MakeVectors(m_pPlayer->pev->angles);
	for (int i = 0; i < 2; i++)
	{
		CPropglock *pBolt = CPropglock::BoltCreate();
		if (i == 0)
		{
			pBolt->pev->origin = m_pPlayer->pev->origin + gpGlobals->v_up * 24 + gpGlobals->v_right * 8 + gpGlobals->v_forward * 8;
		}
		else
		{
			pBolt->pev->origin = m_pPlayer->pev->origin + gpGlobals->v_up * 24 - gpGlobals->v_right * 8 + gpGlobals->v_forward * 8;
		}

		pBolt->pev->movetype = MOVETYPE_BOUNCE;
		pBolt->pev->gravity = 0.5;
		pBolt->pev->friction = 0.8;
		pBolt->pev->angles = m_pPlayer->pev->angles;
		pBolt->pev->owner = m_pPlayer->edict();

		pBolt->pev->velocity = m_pPlayer->pev->velocity + gpGlobals->v_forward * 64 + gpGlobals->v_up * 8 + gpGlobals->v_right * RANDOM_LONG(-8, 8);
		pBolt->pev->speed = 12;

		pBolt->pev->avelocity.x = -600;
		pBolt->pev->avelocity.y = RANDOM_LONG(-300, 500);
		pBolt->pev->avelocity.z = RANDOM_LONG(-100, 200);
	}
#endif
}

void CGlock::Reload( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == GLOCK_MAX_CLIP )
		return;

	int iResult;

	if( m_iClip == 0 )
		iResult = DefaultReload( GLOCK_MAX_CLIP, GLOCK_RELOAD, 1.5f );
	else
		iResult = DefaultReload( GLOCK_MAX_CLIP, GLOCK_RELOAD_NOT_EMPTY, 1.5f );

	if( iResult )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		pev->nextthink = gpGlobals->time + 0.2f;
		SetThink(&CGlock::spawnprop);
	}
}

void CGlock::WeaponIdle( void )
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	// only idle if the slid isn't back
	if( m_iClip != 0 )
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

		if( flRand <= 0.3f + 0 * 0.75f )
		{
			iAnim = GLOCK_IDLE3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 49.0f / 16.0f;
		}
		else if( flRand <= 0.6f + 0 * 0.875f )
		{
			iAnim = GLOCK_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0f / 16.0f;
		}
		else
		{
			iAnim = GLOCK_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0f / 16.0f;
		}
		SendWeaponAnim( iAnim, 1 );
	}
}

class CGlockAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/w_9mmclip.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_9mmclip.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if( pOther->GiveAmmo( AMMO_GLOCKCLIP_GIVE, "9mm", _9MM_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_glockclip, CGlockAmmo )
LINK_ENTITY_TO_CLASS( ammo_9mmclip, CGlockAmmo )
