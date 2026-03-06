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
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "shake.h"
#include "gamerules.h"
#include "game.h"

#define	GAUSS_PRIMARY_CHARGE_VOLUME	256// how loud gauss is while charging
#define GAUSS_PRIMARY_FIRE_VOLUME	450// how loud gauss is when discharged

enum gauss_e
{
	GAUSS_IDLE = 0,
	GAUSS_IDLE2,
	GAUSS_FIDGET,
	GAUSS_SPINUP,
	GAUSS_SPIN,
	GAUSS_FIRE,
	GAUSS_FIRE2,
	GAUSS_HOLSTER,
	GAUSS_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_gauss, CGauss )

float CGauss::GetFullChargeTime( void )
{
#if CLIENT_DLL
	if( bIsMultiplayer() )
#else
	if( g_pGameRules->IsMultiplayer() )
#endif
	{
		return 1.5f;
	}

	return 4.0f;
}

#if CLIENT_DLL
extern int g_irunninggausspred;
#endif

void CGauss::Spawn()
{
	Precache();
	m_iId = WEAPON_GAUSS;
	SET_MODEL( ENT( pev ), "models/w_gauss.mdl" );

	m_iDefaultAmmo = GAUSS_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CGauss::Precache( void )
{
	PRECACHE_MODEL( "models/w_gauss.mdl" );
	PRECACHE_MODEL( "models/v_gauss.mdl" );
	PRECACHE_MODEL( "models/p_gauss.mdl" );

	PRECACHE_SOUND( "items/9mmclip1.wav" );
	PRECACHE_SOUND("items/deploy_lightgun.wav");
	PRECACHE_SOUND("weapons/common_hand1.wav");

	PRECACHE_SOUND("weapons/lightgun_ray1.wav");
	PRECACHE_SOUND("weapons/lightgun_charge.wav");
	PRECACHE_SOUND( "weapons/electro4.wav" );
	PRECACHE_SOUND( "weapons/electro5.wav" );
	PRECACHE_SOUND( "weapons/electro6.wav" );
	PRECACHE_SOUND( "ambience/pulsemachine.wav" );

	m_iGlow = PRECACHE_MODEL( "sprites/hotglow.spr" );
	m_iBalls = PRECACHE_MODEL( "sprites/hotglow.spr" );
	m_iBeam = PRECACHE_MODEL( "sprites/lightbeam.spr" );

	m_usGaussFire = PRECACHE_EVENT( 1, "events/gauss.sc" );
	m_usGaussSpin = PRECACHE_EVENT( 1, "events/gaussspin.sc" );
}

int CGauss::AddToPlayer( CBasePlayer *pPlayer )
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

int CGauss::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "uranium";
	p->iMaxAmmo1 = URANIUM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = GAUSS_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_GAUSS;
	p->iFlags = 0;
	p->iWeight = GAUSS_WEIGHT;

	return 1;
}

BOOL CGauss::Deploy()
{
	g_engfuncs.pfnSetClientMaxspeed(m_pPlayer->edict(), 230 );
	m_pPlayer->m_flPlayAftershock = 0.0;
	return DefaultDeploy( "models/v_gauss.mdl", "models/p_gauss.mdl", GAUSS_DRAW, "gauss" );
}

void CGauss::Holster( int skiplocal /* = 0 */ )
{
	g_engfuncs.pfnSetClientMaxspeed(m_pPlayer->edict(), 230 );
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.35;

	m_fInReload = FALSE;// cancel any reload in progress.

	SendWeaponAnim( GAUSS_HOLSTER );
	m_fInAttack = 0;
}

void CGauss::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD)
	{
		PlayEmptySound();
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = GetNextAttackDelay(0.15f);
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
		return;
	}

	m_pPlayer->m_iWeaponVolume = GAUSS_PRIMARY_FIRE_VOLUME;
	m_fPrimaryFire = TRUE;

	m_iClip--;

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == 2)
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_STATIC, "fvox/ammo_low.wav", 1.0, ATTN_NORM);

	StartFire();
	m_fInAttack = 0;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.1f;

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
	// HEV suit - indicate out of ammo condition
	m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
}

void CGauss::SecondaryAttack()
{
	return;
}

//=========================================================
// StartFire- since all of this code has to run and then 
// call Fire(), it was easier at this point to rip it out 
// of weaponidle() and make its own function then to try to
// merge this into Fire(), which has some identical variable names 
//=========================================================
void CGauss::StartFire( void )
{
	float flDamage;
	flDamage = 200;

	if( m_pPlayer->m_flStartCharge > gpGlobals->time )
		m_pPlayer->m_flStartCharge = gpGlobals->time;
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition( );

	Fire( vecSrc, vecAiming, flDamage );
}

void CGauss::Fire( Vector vecOrigSrc, Vector vecDir, float flDamage )
{
	m_pPlayer->m_iWeaponVolume = GAUSS_PRIMARY_FIRE_VOLUME;
	TraceResult tr, beam_tr;
	Vector vecSrc = vecOrigSrc;
	Vector vecDest = vecSrc + vecDir * 8192.0f;
	edict_t	*pentIgnore;
	float flMaxFrac = 1.0f;
	int nTotal = 0;
	int fHasPunched = 0;
	int nMaxHits = 10;

	pentIgnore = ENT( m_pPlayer->pev );

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	// The main firing event is sent unreliably so it won't be delayed.
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usGaussFire, 0.0, (float *)&m_pPlayer->pev->origin, (float *)&m_pPlayer->pev->angles, gSkillData.plrDmgGauss, 0.0, 0, 0, m_fPrimaryFire ? 1 : 0, 0 );

	// This reliable event is used to stop the spinning sound
	// It's delayed by a fraction of second to make sure it is delayed by 1 frame on the client
	// It's sent reliably anyway, which could lead to other delays

	//	PLAYBACK_EVENT_FULL( FEV_NOTHOST | FEV_RELIABLE, m_pPlayer->edict(), m_usGaussFire, 0.01, (float *)&m_pPlayer->pev->origin, (float *)&m_pPlayer->pev->angles, 0.0, 0.0, 0, 0, 0, 1 );

#if !CLIENT_DLL
	while( flDamage > 10 && nMaxHits > 0 )
	{
		nMaxHits--;

		// ALERT( at_console, "." );
		UTIL_TraceLine( vecSrc, vecDest, dont_ignore_monsters, pentIgnore, &tr );

		if( tr.fAllSolid )
			break;

		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		if( pEntity == NULL )
			break;

		vecSrc = tr.vecEndPos + vecDir;
		pentIgnore = ENT( pEntity->pev );

		if( pEntity->pev->takedamage )
		{
			ClearMultiDamage();
			if( pEntity->pev == m_pPlayer->pev )
				tr.iHitgroup = 0;
			pEntity->TraceAttack( m_pPlayer->pev, gSkillData.plrDmgGauss, vecDir, &tr, DMG_ENERGYBEAM );
			ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );
		}
	}
#endif
	// ALERT( at_console, "%d bytes\n", nTotal );
}

void CGauss::WeaponIdle( void )
{
	ResetEmptySound();

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if( m_fInAttack != 0 )
	{
		StartFire();
		m_fInAttack = 0;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;
	}
	else
	{
		int iAnim;
		float flRand = RANDOM_FLOAT( 0.0f, 1.0f );
		if( flRand <= 0.5f )
		{
			iAnim = GAUSS_IDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10.0f, 15.0f );
		}
		else if( flRand <= 0.75f )
		{
			iAnim = GAUSS_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10.0f, 15.0f );
		}
		else
		{
			iAnim = GAUSS_FIDGET;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
		}
#if !CLIENT_DLL
		SendWeaponAnim( iAnim );
#endif
	}
}

void CGauss::Reload(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == GAUSS_MAX_CLIP)
		return;
	else
	{
		DefaultReload(GAUSS_MAX_CLIP, GAUSS_SPIN, 0.82);
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = 1.5;
	}
}

class CGaussAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/w_gaussammo.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_gaussammo.mdl" );
		PRECACHE_SOUND("items/lightgun_ammo_pickup.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if( pOther->GiveAmmo( AMMO_URANIUMBOX_GIVE, "uranium", URANIUM_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/lightgun_ammo_pickup.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_gaussclip, CGaussAmmo )
#endif
