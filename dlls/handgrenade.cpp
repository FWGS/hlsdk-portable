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

#define	HANDGRENADE_PRIMARY_VOLUME		450

enum handgrenade_e {
	HANDGRENADE_IDLE = 0,
	HANDGRENADE_FIDGET,
	HANDGRENADE_PINPULL,
	HANDGRENADE_THROW1,	// toss
	HANDGRENADE_THROW_NEW,
	HANDGRENADE_HOLSTER,
	HANDGRENADE_HOLSTER2, // player isn't carrying any grenades
	HANDGRENADE_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_handgrenade, CHandGrenade )

void CHandGrenade::Spawn()
{
	Precache();
	m_iId = WEAPON_HANDGRENADE;
	SET_MODEL( ENT( pev ), "models/w_grenade.mdl" );

#if !CLIENT_DLL
	pev->dmg = gSkillData.plrDmgHandGrenade;
#endif
	m_iDefaultAmmo = HANDGRENADE_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CHandGrenade::Precache( void )
{
	PRECACHE_MODEL( "models/w_grenade.mdl" );
	PRECACHE_MODEL( "models/v_grenade.mdl" );
	PRECACHE_MODEL( "models/p_grenade.mdl" );

	PRECACHE_SOUND("weapons/he_pinpull.wav");
}

int CHandGrenade::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "Hand Grenade";
	p->iMaxAmmo1 = HANDGRENADE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_HANDGRENADE;
	p->iWeight = HANDGRENADE_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}

BOOL CHandGrenade::Deploy()
{
	g_engfuncs.pfnSetClientMaxspeed(m_pPlayer->edict(), 230);
	m_flReleaseThrow = -1;
	return DefaultDeploy( "models/v_grenade.mdl", "models/p_grenade.mdl", HANDGRENADE_DRAW, "crowbar" );
}

BOOL CHandGrenade::CanHolster( void )
{
	// can only holster hand grenades when not primed!
	return ( m_flStartThrow == 0 );
}

void CHandGrenade::Holster( int skiplocal /* = 0 */ )
{
	g_engfuncs.pfnSetClientMaxspeed(m_pPlayer->edict(), 230);
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.56f;

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= 1)
		SendWeaponAnim( HANDGRENADE_HOLSTER );

	if( m_flStartThrow )
	{
		m_flStartThrow = 0.0f;
		m_flReleaseThrow = 0.0f;
	}

	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "common/null.wav", 1.0f, ATTN_NORM );
}

void CHandGrenade::PrimaryAttack()
{
	if( !m_flStartThrow && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 )
	{
		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = 0.0f;

		SendWeaponAnim( HANDGRENADE_PINPULL );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.85f;
	}
}

void CHandGrenade::WeaponIdle( void )
{
	if( m_flReleaseThrow == 0.0f && m_flStartThrow )
		 m_flReleaseThrow = gpGlobals->time;

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if( m_flStartThrow )
	{
		Vector angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		if( angThrow.x < 0.0f )
			angThrow.x = -10.0f + angThrow.x * ( ( 90.0f - 10.0f ) / 90.0f );
		else
			angThrow.x = -10.0f + angThrow.x * ( ( 90.0f + 10.0f ) / 90.0f );

		float flVel = ( 90.0f - angThrow.x ) * 6.5f;
		if( flVel > 1000.0f )
			flVel = 1000.0f;

		UTIL_MakeVectors( angThrow );

		Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16.0f;

		Vector vecThrow = gpGlobals->v_forward * flVel + m_pPlayer->pev->velocity;

		// alway explode 3 seconds after the pin was pulled
		float time = m_flStartThrow - gpGlobals->time + 3.0f;
		if( time < 0.0f )
			time = 0.0f;

		CGrenade::ShootTimed( m_pPlayer->pev, vecSrc, vecThrow, time );

		if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] == 1 )
		{
			SendWeaponAnim( HANDGRENADE_THROW1 );
		}
		else
		{
			SendWeaponAnim( HANDGRENADE_THROW_NEW );
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#if !HANDGRENADE_DEPLOY_FIX
		m_flReleaseThrow = 0.0f;
#endif
		m_flStartThrow = 0.0f;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.95;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.95;

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

		if( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			// just threw last grenade
			// set attack times in the future, and weapon idle in the future so we can see the whole throw
			// animation, weapon idle will automatically retire the weapon for us.
			m_flTimeWeaponIdle = m_flNextSecondaryAttack = m_flNextPrimaryAttack = GetNextAttackDelay( 0.5f );// ensure that the animation can finish playing
		}
		return;
	}

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0f, 1.0f );
		if( flRand <= 0.75f )
		{
			iAnim = HANDGRENADE_IDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10.0f, 15.0f );// how long till we do this again.
		}
		else
		{
			iAnim = HANDGRENADE_FIDGET;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 75.0f / 30.0f;
		}

		SendWeaponAnim( iAnim );
	}
}
