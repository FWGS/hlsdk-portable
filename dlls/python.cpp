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
#if !OEM_BUILD && !HLDEMO_BUILD

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"

#ifndef CLIENT_DLL
#define BOLT_AIR_VELOCITY	900
#define BOLT_WATER_VELOCITY	600


class CPythonFurniture : public CBaseEntity
{
	void Spawn(void);
	void Precache(void);
	int  Classify(void);
	void EXPORT BubbleThink(void);
	void EXPORT BoltTouch(CBaseEntity *pOther);
	void EXPORT ExplodeThink(void);
	int touchcounter = 0;
	int pGibName[3];

	int m_iTrail;

public:
	static CPythonFurniture *BoltCreate(void);
};
LINK_ENTITY_TO_CLASS(pythonfurniture, CPythonFurniture);

CPythonFurniture *CPythonFurniture::BoltCreate(void)
{
	// Create a new entity with CCrossbowBolt private data
	CPythonFurniture *pBolt = GetClassPtr((CPythonFurniture *)NULL);
	pBolt->pev->classname = MAKE_STRING("furniture");
	pBolt->Spawn();
	pBolt->touchcounter = 0;

	return pBolt;
}

void CPythonFurniture::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->body = RANDOM_LONG(0, 3);

	pev->gravity = 0.8;

	SET_MODEL(ENT(pev), "models/furniture.mdl");

	UTIL_SetOrigin(pev, pev->origin);

	switch (pev->body)
	{
	case 0: UTIL_SetSize(pev, Vector(0, 0, 0), Vector(32, 32, 32)); break;
	case 1: UTIL_SetSize(pev, Vector(0, 0, 0), Vector(16, 16, 32)); break;
	case 2: UTIL_SetSize(pev, Vector(0, 0, 0), Vector(16, 16, 32)); break;
	case 3: UTIL_SetSize(pev, Vector(0, 0, 0), Vector(8, 8, 48)); break;
	}
	

	SetTouch(&CPythonFurniture::BoltTouch);
	SetThink(&CPythonFurniture::BubbleThink);
	pev->nextthink = gpGlobals->time + 0.2;
}


void CPythonFurniture::Precache()
{
	PRECACHE_MODEL("models/furniture.mdl");
	m_iTrail = PRECACHE_MODEL("sprites/streak.spr");
	pGibName[0] = PRECACHE_MODEL("models/woodgibs.mdl");
	pGibName[1] = PRECACHE_MODEL("models/metalplategibs.mdl");
	pGibName[2] = PRECACHE_MODEL("models/woodgibs.mdl");
	pGibName[3] = PRECACHE_MODEL("models/computergibs.mdl");
}


int	CPythonFurniture::Classify(void)
{
	return	CLASS_NONE;
}

void CPythonFurniture::BoltTouch(CBaseEntity *pOther)
{

	if (pOther->edict() == pev->owner)
		return;

	if (pev->flags & FL_ONGROUND)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.15;
	}
	else
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "weapons/cbar_hit1.wav", 0.35, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 7));
	}

	pev->angles.x = 0;
	pev->angles.z = 0;
	touchcounter++;
	SetThink(NULL);

	if (pOther->pev->takedamage)
	{
		ExplodeThink();
		SetTouch(NULL);
		SetThink(NULL);
		UTIL_Remove(this);
	}
	else
	{
		if ((touchcounter > 3) || (pev->velocity.z == 0))
		{
			ExplodeThink();
		}

		if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
		{
			UTIL_Sparks(pev->origin);
		}
	}
}

void CPythonFurniture::BubbleThink(void)
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->waterlevel == 0)
		return;

	UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1, pev->origin, 1);
}

void CPythonFurniture::ExplodeThink(void)
{
	int iContents = UTIL_PointContents(pev->origin);
	int iScale;

	pev->dmg = 90;
	iScale = 45;

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_BREAKMODEL);

	// position
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);

	// size
	WRITE_COORD(pev->size.x * 2);
	WRITE_COORD(pev->size.y * 2);
	WRITE_COORD(pev->size.z * 2);

	// velocity
	WRITE_COORD(pev->velocity.x);
	WRITE_COORD(pev->velocity.y);
	WRITE_COORD(pev->velocity.z);

	// randomization
	WRITE_BYTE(25);

	// Model
	WRITE_SHORT(pGibName[pev->body]);	//model id#

	// # of shards
	WRITE_BYTE(0);	// let client decide

	// duration
	WRITE_BYTE(25);// 2.5 seconds

	// flags
	WRITE_BYTE(BREAK_METAL);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_EXPLOSION);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	if (iContents != CONTENTS_WATER)
	{
		WRITE_SHORT(g_sModelIndexFireball);
	}
	else
	{
		WRITE_SHORT(g_sModelIndexWExplosion);
	}
	WRITE_BYTE(iScale); // scale * 10
	WRITE_BYTE(15); // framerate
	WRITE_BYTE(TE_EXPLFLAG_NONE);
	MESSAGE_END();

	entvars_t *pevOwner;

	if (pev->owner)
		pevOwner = VARS(pev->owner);
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	::RadiusDamage(pev->origin, pev, pevOwner, pev->dmg, 160, CLASS_NONE, DMG_BLAST | DMG_ALWAYSGIB);

	UTIL_Remove(this);
}
#endif

enum python_e
{
	PYTHON_IDLE1 = 0,
	PYTHON_FIDGET,
	PYTHON_FIRE1,
	PYTHON_RELOAD,
	PYTHON_HOLSTER,
	PYTHON_DRAW,
	PYTHON_IDLE2,
	PYTHON_IDLE3
};

LINK_ENTITY_TO_CLASS( weapon_python, CPython )
LINK_ENTITY_TO_CLASS( weapon_357, CPython )

int CPython::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "357";
	p->iMaxAmmo1 = _357_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = PYTHON_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 1;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_PYTHON;
	p->iWeight = PYTHON_WEIGHT;

	return 1;
}

int CPython::AddToPlayer( CBasePlayer *pPlayer )
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

void CPython::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_357" ); // hack to allow for old names
	Precache();
	m_iId = WEAPON_PYTHON;
	SET_MODEL( ENT( pev ), "models/w_357.mdl" );

	m_iDefaultAmmo = PYTHON_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CPython::Precache( void )
{
	PRECACHE_MODEL( "models/v_357.mdl" );
	PRECACHE_MODEL( "models/w_357.mdl" );
	PRECACHE_MODEL( "models/p_357.mdl" );

	PRECACHE_MODEL( "models/w_357ammobox.mdl" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );

	PRECACHE_SOUND( "weapons/357_reload1.wav" );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	PRECACHE_SOUND( "weapons/357_shot1.wav" );
	PRECACHE_SOUND( "weapons/357_shot2.wav" );

	UTIL_PrecacheOther("pythonfurniture");

	m_usFirePython = PRECACHE_EVENT( 1, "events/python.sc" );
}

BOOL CPython::Deploy()
{
#if CLIENT_DLL
	if( bIsMultiplayer() )
#else
	if( g_pGameRules->IsMultiplayer() )
#endif
	{
		// enable laser sight geometry.
		pev->body = 1;
	}
	else
	{
		pev->body = 0;
	}

	return DefaultDeploy( "models/v_357.mdl", "models/p_357.mdl", PYTHON_DRAW, "python", UseDecrement(), pev->body );
}

void CPython::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	if( m_fInZoom )
	{
		SecondaryAttack();
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10.0f, 15.0f );
	SendWeaponAnim( PYTHON_HOLSTER );
}

void CPython::SecondaryAttack( void )
{
#if CLIENT_DLL
	if( !bIsMultiplayer() )
#else
	if( !g_pGameRules->IsMultiplayer() )
#endif
	{
		return;
	}

	if( m_pPlayer->pev->fov != 0 )
	{
		m_fInZoom = FALSE;
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;  // 0 means reset to default fov
	}
	else if( m_pPlayer->pev->fov != 40 )
	{
		m_fInZoom = TRUE;
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 40;
	}

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
}

void CPython::PrimaryAttack()
{
	// don't fire underwater
	if( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	if( m_iClip <= 0 )
	{
		if( !m_fFireOnEmpty )
			Reload();
		else
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		}

		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 24.0f;
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	Vector vecDir = gpGlobals->v_forward;

	SendWeaponAnim(PYTHON_FIRE1);
	
	if (RANDOM_LONG(0,1))
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_shot1.wav", 1, ATTN_NORM);
	else
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_shot2.wav", 1, ATTN_NORM);

#ifndef CLIENT_DLL
	CPythonFurniture *pBolt = CPythonFurniture::BoltCreate();
	pBolt->pev->origin = vecSrc;
	pBolt->pev->movetype = MOVETYPE_BOUNCE;
	pBolt->pev->gravity = 0.5;
	pBolt->pev->friction = 0.8;
	pBolt->pev->angles = m_pPlayer->pev->angles;
	pBolt->pev->owner = m_pPlayer->edict();

	if (m_pPlayer->pev->waterlevel == 3)
	{
		pBolt->pev->velocity = vecDir * BOLT_WATER_VELOCITY;
		pBolt->pev->speed = BOLT_WATER_VELOCITY;
	}
	else
	{
		pBolt->pev->velocity = vecDir * BOLT_AIR_VELOCITY;
		pBolt->pev->speed = BOLT_AIR_VELOCITY;
	}
	pBolt->pev->avelocity.x = -100;
	pBolt->pev->avelocity.y = RANDOM_LONG(-100, 100);
	pBolt->pev->avelocity.z = RANDOM_LONG(-100, 100);
#endif

	/*vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_357, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFirePython, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );*/

	if( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flNextPrimaryAttack = 0.75f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10.0f, 15.0f );
}

void CPython::Reload( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == PYTHON_MAX_CLIP )
		return;

	if( m_pPlayer->pev->fov != 0 )
	{
		m_fInZoom = FALSE;
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;  // 0 means reset to default fov
	}

	int bUseScope = FALSE;
#if CLIENT_DLL
	bUseScope = bIsMultiplayer();
#else
	bUseScope = g_pGameRules->IsMultiplayer();
#endif
	if( DefaultReload( PYTHON_MAX_CLIP, PYTHON_RELOAD, 2.0f, bUseScope ) )
	{
		m_flSoundDelay = 1.5f;
	}
}

void CPython::WeaponIdle( void )
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	// ALERT( at_console, "%.2f\n", gpGlobals->time - m_flSoundDelay );
	if( m_flSoundDelay != 0 && m_flSoundDelay <= UTIL_WeaponTimeBase() )
	{
		EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/357_reload1.wav", RANDOM_FLOAT( 0.8f, 0.9f ), ATTN_NORM );
		m_flSoundDelay = 0.0f;
	}

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0f, 1.0f );
	if( flRand <= 0.5f )
	{
		iAnim = PYTHON_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 70.0f / 30.0f );
	}
	else if( flRand <= 0.7f )
	{
		iAnim = PYTHON_IDLE2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 60.0f / 30.0f );
	}
	else if( flRand <= 0.9f )
	{
		iAnim = PYTHON_IDLE3;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 88.0f / 30.0f );
	}
	else
	{
		iAnim = PYTHON_FIDGET;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 170.0f / 30.0f );
	}
	
	int bUseScope = FALSE;
#if CLIENT_DLL
	bUseScope = bIsMultiplayer();
#else
	bUseScope = g_pGameRules->IsMultiplayer();
#endif
	SendWeaponAnim( iAnim, UseDecrement() ? 1 : 0, bUseScope );
}

class CPythonAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT(pev), "models/w_357ammobox.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_357ammobox.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{ 
		if( pOther->GiveAmmo( AMMO_357BOX_GIVE, "357", _357_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_357, CPythonAmmo )
#endif
