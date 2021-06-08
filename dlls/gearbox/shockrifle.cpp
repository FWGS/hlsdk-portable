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
#if !OEM_BUILD && !HLDEMO_BUILD

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#if !CLIENT_DLL
#include "shock.h"
#endif

enum shockrifle_e {
	SHOCK_IDLE1 = 0,
	SHOCK_FIRE,
	SHOCK_DRAW,
	SHOCK_HOLSTER,
	SHOCK_IDLE3,
};

LINK_ENTITY_TO_CLASS(weapon_shockrifle, CShockrifle)

void CShockrifle::Spawn()
{
	Precache();
	m_iId = WEAPON_SHOCKRIFLE;
	SET_MODEL(ENT(pev), "models/w_shock.mdl");

	m_iDefaultAmmo = SHOCKRIFLE_DEFAULT_GIVE;
	m_iFirePhase = 0;

	FallInit();// get ready to fall down.
}


void CShockrifle::Precache(void)
{
	PRECACHE_MODEL("models/v_shock.mdl");
	PRECACHE_MODEL("models/w_shock.mdl");
	PRECACHE_MODEL("models/p_shock.mdl");

	PRECACHE_SOUND("weapons/shock_discharge.wav");
	PRECACHE_SOUND("weapons/shock_draw.wav");
	PRECACHE_SOUND("weapons/shock_fire.wav");
	PRECACHE_SOUND("weapons/shock_impact.wav");
	PRECACHE_SOUND("weapons/shock_recharge.wav");

	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_MODEL("sprites/flare3.spr");

	m_usShockFire = PRECACHE_EVENT(1, "events/shock.sc");

	UTIL_PrecacheOther("shock_beam");
}

int CShockrifle::AddToPlayer(CBasePlayer *pPlayer)
{
	if (CBasePlayerWeapon::AddToPlayer(pPlayer))
	{

#if !CLIENT_DLL
		if (g_pGameRules->IsMultiplayer())
		{
			// in multiplayer, all hivehands come full. 
			pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = SHOCK_MAX_CARRY;
		}
#endif

		MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev);
		WRITE_BYTE(m_iId);
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

int CShockrifle::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Shocks";
	p->iMaxAmmo1 = SHOCK_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 6;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_SHOCKRIFLE;
	p->iFlags = ITEM_FLAG_NOAUTOSWITCHEMPTY | ITEM_FLAG_NOAUTORELOAD;
	p->iWeight = HORNETGUN_WEIGHT;

	return 1;
}


BOOL CShockrifle::Deploy()
{
#if CLIENT_DLL
	if( bIsMultiplayer() )
#else
	if( g_pGameRules->IsMultiplayer() )
#endif
		m_flRechargeTime = gpGlobals->time + 0.25;
	else
		m_flRechargeTime = gpGlobals->time + 0.5;

	return DefaultDeploy("models/v_shock.mdl", "models/p_shock.mdl", SHOCK_DRAW, "shockrifle");
}

void CShockrifle::Holster(int skiplocal /* = 0 */)
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(SHOCK_HOLSTER);

	//!!!HACKHACK - can't select hornetgun if it's empty! no way to get ammo for it, either.
	if (!m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()])
	{
		m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = 1;
	}
}

void CShockrifle::PrimaryAttack()
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return;

	if (m_pPlayer->pev->waterlevel == 3)
	{
#if !CLIENT_DLL
		int attenuation = 150 * m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType];
		int dmg = 100 * m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType];
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/shock_discharge.wav", VOL_NORM, ATTN_NORM);
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = 0;
		RadiusDamage(m_pPlayer->pev->origin, m_pPlayer->pev, m_pPlayer->pev, dmg, attenuation, CLASS_NONE, DMG_SHOCK | DMG_ALWAYSGIB );
#endif
		return;
	}

	CreateChargeEffect();

#if !CLIENT_DLL
	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	anglesAim.x = -anglesAim.x;
	UTIL_MakeVectors(m_pPlayer->pev->v_angle);

	Vector vecSrc;
	vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 8 + gpGlobals->v_right * 12 + gpGlobals->v_up * -12;

	CShock::Shoot(m_pPlayer->pev, anglesAim, vecSrc, gpGlobals->v_forward * 2000);

	m_flRechargeTime = gpGlobals->time + 1;
#endif
	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;


	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usShockFire, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 0, 0);

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
#if CLIENT_DLL
	if( bIsMultiplayer() )
#else
	if( g_pGameRules->IsMultiplayer() )
#endif
		m_flNextPrimaryAttack = GetNextAttackDelay(0.1);
	else
		m_flNextPrimaryAttack = GetNextAttackDelay(0.2);

	SetThink( &CShockrifle::ClearBeams );
	pev->nextthink = gpGlobals->time + 0.08;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.33;
}

void CShockrifle::SecondaryAttack(void)
{
}

void CShockrifle::Reload(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= SHOCK_MAX_CARRY)
		return;

	while (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < SHOCK_MAX_CARRY && m_flRechargeTime < gpGlobals->time)
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/shock_recharge.wav", 1, ATTN_NORM);

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]++;
#if !CLIENT_DLL
		if( g_pGameRules->IsMultiplayer() )
			m_flRechargeTime += 0.25;
		else
			m_flRechargeTime += 0.5;
#endif
	}
}


void CShockrifle::WeaponIdle(void)
{
	Reload();

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
	if (flRand <= 0.8) {
		SendWeaponAnim(SHOCK_IDLE3);
	} else {
		SendWeaponAnim(SHOCK_IDLE1);
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.3f;
}

void CShockrifle::CreateChargeEffect( void )
{
#if !CLIENT_DLL
	if( g_pGameRules->IsMultiplayer())
		return;
	int iBeam = 0;

	for( int i = 2; i < 5; i++)
	{
		if( !m_pBeam[iBeam] )
			m_pBeam[iBeam] = CBeam::BeamCreate("sprites/lgtning.spr", 16);
		m_pBeam[iBeam]->EntsInit( m_pPlayer->entindex(), m_pPlayer->entindex() );
		m_pBeam[iBeam]->SetStartAttachment(1);
		m_pBeam[iBeam]->SetEndAttachment(i);
		m_pBeam[iBeam]->SetNoise( 75 );
		m_pBeam[iBeam]->pev->scale= 10;
		m_pBeam[iBeam]->SetColor( 0, 253, 253 );
		m_pBeam[iBeam]->SetScrollRate( 30 );
		m_pBeam[iBeam]->SetBrightness( 190 );
		iBeam++;
	}
#endif
}

void CShockrifle::ClearBeams( void )
{
#if !CLIENT_DLL
	if( g_pGameRules->IsMultiplayer())
		return;

	for( int i = 0; i < 3; i++ )
	{
		if( m_pBeam[i] )
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}
	SetThink( NULL );
#endif
}

#endif
