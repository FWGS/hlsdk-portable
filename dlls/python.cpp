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
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

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

#ifndef CLIENT_DLL

LINK_ENTITY_TO_CLASS( python_spot, CPythonSpot );

//=========================================================
//=========================================================
CPythonSpot *CPythonSpot::CreateSpot( void )
{
	CPythonSpot *pSpot = GetClassPtr( (CPythonSpot *)NULL );
	pSpot->Spawn();

	pSpot->pev->classname = MAKE_STRING("python_spot");

	return pSpot;
}

//=========================================================
//=========================================================
CPythonSpot *CPythonSpot::CreateSpot( const char* spritename )
{
	CPythonSpot *pSpot = CreateSpot();
	SET_MODEL(ENT(pSpot->pev), spritename);
	return pSpot;
}

//=========================================================
//=========================================================
void CPythonSpot::Spawn( void )
{
	Precache( );
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	pev->rendermode = kRenderGlow;
	pev->renderfx = kRenderFxNoDissipation;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), "sprites/laserdot.spr");
	UTIL_SetOrigin( this, pev->origin );
};

//=========================================================
// Suspend- make the Python sight invisible. 
//=========================================================
void CPythonSpot::Suspend( float flSuspendTime )
{
	pev->effects |= EF_NODRAW;
	
	//LRC: -1 means suspend indefinitely
	if (flSuspendTime == -1)
	{
		SetThink( NULL );
	}
	else
	{
		SetThink(&CPythonSpot:: Revive );
		SetNextThink( flSuspendTime );
	}
}

//=========================================================
// Revive - bring a suspended Python sight back.
//=========================================================
void CPythonSpot::Revive( void )
{
	pev->effects &= ~EF_NODRAW;

	SetThink( NULL );
}

void CPythonSpot::Precache( void )
{
	PRECACHE_MODEL("sprites/laserdot.spr");
};

#endif

//=========================================================

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
	m_fSpotActive = 1;

	m_iDefaultAmmo = RANDOM_LONG(3, 6);

	FallInit();// get ready to fall down.
}

void CPython::Precache( void )
{
	PRECACHE_MODEL( "models/v_357.mdl" );
	PRECACHE_MODEL( "models/w_357.mdl" );
	PRECACHE_MODEL( "models/p_357.mdl" );

	PRECACHE_MODEL( "models/w_357ammobox.mdl" );
	PRECACHE_SOUND("items/magnum_box.wav");

	PRECACHE_SOUND("weapons/357_bulletsout.wav");
	PRECACHE_SOUND("weapons/magnum_ammo_out.wav");
	PRECACHE_SOUND("weapons/magnum_ammo_in.wav");
	PRECACHE_SOUND( "weapons/357_reload1.wav" );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	PRECACHE_SOUND( "weapons/357_shot1.wav" );
	PRECACHE_SOUND( "weapons/357_shot2.wav" );

	PRECACHE_SOUND ("fvox/ammo_low.wav");
		
	UTIL_PrecacheOther( "python_spot" );
	m_flCanShootTime = 0;

	m_usFirePython = PRECACHE_EVENT( 1, "events/python.sc" );
}

BOOL CPython::Deploy()
{
	m_pPlayer->m_highNoonKills = 0;
	g_engfuncs.pfnSetClientMaxspeed(m_pPlayer->edict(), 230 );
	m_flCanShootTime = 0;
	m_fCanShoot = 1;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = 0.5;
	return DefaultDeploy( "models/v_357.mdl", "models/p_357.mdl", PYTHON_DRAW, "python" );
}

void CPython::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_highNoonKills = 0;
	if ( m_pPlayer->m_iFOV != 0 )
		SecondaryAttack( );

	g_engfuncs.pfnSetClientMaxspeed(m_pPlayer->edict(), 230 );
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.35;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	SendWeaponAnim(PYTHON_HOLSTER);

#ifndef CLIENT_DLL
	if (m_pSpot)
	{
		m_pSpot->Killed(NULL, GIB_NEVER);
		m_pSpot = NULL;
	}
#endif
}

void CPython::SecondaryAttack( void )
{
	UpdateSpot();

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
	{
		SetZoom(0);
		if (!m_fSpotActive)
			m_fSpotActive = !m_fSpotActive;
		WeaponIdle();
		return;
	}

	if( m_pPlayer->pev->fov != 0 )
	{
		SetZoom(0);
	}
	else if ( m_pPlayer->m_iFOV != 65 )
	{
		SetZoom(65);
	}

	m_fSpotActive = !m_fSpotActive;

#ifndef CLIENT_DLL
	if (!m_fSpotActive && m_pSpot)
	{
		m_pSpot->Killed(NULL, GIB_NORMAL);
		m_pSpot = NULL;
	}
#endif

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
}

void CPython::PrimaryAttack()
{
	UpdateSpot();

	if (m_fCanShoot == 1)
	{
		m_flCanShootTime = gpGlobals->time + 0.7;
		m_fCanShoot = 0;

		// don't fire underwater
		if (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD)
		{
			PlayEmptySound();
			m_flCanShootTime = gpGlobals->time + 0.7;
			m_fCanShoot = 0;
			return;
		}

		if( m_iClip <= 0 )
		{
			PlayEmptySound();
			m_flCanShootTime = gpGlobals->time + 0.7;
			m_fCanShoot = 0;

			return;
		}

		m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
		m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

		m_iClip--;

		if (m_iClip == 2)
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_STATIC, "fvox/ammo_low.wav", 1.0, ATTN_NORM);

#ifndef CLIENT_DLL

		UTIL_ScreenShake(pev->origin, 4, 150.0, 0.2, 120);
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);	// player "shoot" animation

#endif

		UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

		Vector vecSrc = m_pPlayer->GetGunPosition();
		Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

		Vector vecDir;
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_357, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

		int flags;
#if CLIENT_WEAPONS
		flags = FEV_NOTHOST;
#else
		flags = 0;
#endif
		PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFirePython, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

		if( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
			// HEV suit - indicate out of ammo condition
			m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

		m_flTimeWeaponIdle = 0.7;
	}
}

void CPython::Reload(void)
{
	UpdateSpot();

	if (m_pPlayer->m_iFOV != 0)
		SecondaryAttack();

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == PYTHON_MAX_CLIP)
		return;
	else
	{
		m_pPlayer->m_highNoonKills = 0;
		DefaultReload(6, PYTHON_RELOAD, 2.45);
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = 3.55;
		m_flTimeWeaponIdle = 3.55;
	}

#ifndef CLIENT_DLL
	if (m_pSpot && m_fSpotActive)
	{
		m_pSpot->Suspend(3.55);
	}
#endif
}

// Hack to deal with laser delay
void CPython::ItemPostFrame()
{
	if ( m_flCanShootTime < gpGlobals->time) 
	{
		m_fCanShoot = 1;
		m_flCanShootTime = 0;
	}
	CBasePlayerWeapon::ItemPostFrame();
}

void CPython::WeaponIdle( void )
{
	UpdateSpot();
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );
	if (flRand <= 0.3 + 0 * 0.75)
	{
		iAnim = PYTHON_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (70.0 / 30.0);
	}
	else if (flRand <= 0.6 + 0 * 0.875)
	{
		iAnim = PYTHON_IDLE2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (60.0 / 30.0);
	}
	else
	{
		iAnim = PYTHON_FIDGET;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (170.0 / 30.0);
	}

	SendWeaponAnim(iAnim);
}

void CPython::UpdateSpot( void )
{
#ifndef CLIENT_DLL
	if (m_fSpotActive)
	{
		if (!m_pSpot)
		{
			m_pSpot = CPythonSpot::CreateSpot();
		}

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		Vector vecSrc = m_pPlayer->GetGunPosition( );;
		Vector vecAiming = gpGlobals->v_forward;

		TraceResult tr;
		UTIL_TraceLine ( vecSrc, vecSrc + vecAiming * 8192, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr );
		
		UTIL_SetOrigin( m_pSpot, tr.vecEndPos );
	}
#endif
}

void CPython::SetZoom(int fov)
{
	if (fov)
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = fov;
		m_pPlayer->pev->viewmodel = iStringNull;
	}
	else
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;
		m_pPlayer->pev->viewmodel = MAKE_STRING("models/v_357.mdl");
	}
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
		PRECACHE_SOUND("items/magnum_box.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{ 
		if( pOther->GiveAmmo( AMMO_357BOX_GIVE, "357", _357_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/magnum_box.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_357, CPythonAmmo )
#endif
