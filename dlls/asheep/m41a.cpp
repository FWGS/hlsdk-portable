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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

enum m41a_e
{
	M41A_LONGIDLE = 0,
	M41A_IDLE1,
	M41A_LAUNCH,
	M41A_RELOAD,
	M41A_DEPLOY,
	M41A_FIRE1,
	M41A_FIRE2,
	M41A_FIRE3,
	M41A_HOLSTER
};


LINK_ENTITY_TO_CLASS( weapon_9mmm41a, CM41A );

//=========================================================
//=========================================================
int CM41A::SecondaryAmmoIndex( void )
{
	return m_iSecondaryAmmoType;
}

void CM41A::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_9mmm41a"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/w_9mmm41a.mdl");
	m_iId = WEAPON_M41A;

	m_iDefaultAmmo = MP5_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CM41A::Precache( void )
{
	PRECACHE_MODEL("models/v_9mmM41A.mdl");
	PRECACHE_MODEL("models/w_9mmM41A.mdl");
	PRECACHE_MODEL("models/p_9mmM41A.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_MODEL("models/grenade.mdl");	// grenade

	PRECACHE_MODEL("models/w_9mmARclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND("items/clipinsert1.wav");
	PRECACHE_SOUND("items/cliprelease1.wav");

	PRECACHE_SOUND ("weapons/m41ahks1.wav");// H to the K

	PRECACHE_SOUND( "weapons/m41aglauncher.wav" );
	PRECACHE_SOUND( "weapons/m41aglauncher2.wav" );

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usM41A = PRECACHE_EVENT( 1, "events/m41a.sc" );
}

int CM41A::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = "ARgrenades";
	p->iMaxAmmo2 = M203_GRENADE_MAX_CARRY;
	p->iMaxClip = MP5_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 5;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_M41A;
	p->iWeight = M41A_WEIGHT;

	return 1;
}

int CM41A::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CM41A::Deploy( )
{
	return DefaultDeploy( "models/v_9mmm41a.mdl", "models/p_9mmm41a.mdl", M41A_DEPLOY, "M41A" );
}

void CM41A::Holster( int skiplocal /* = 0 */ )
{
        DefaultHolster( M41A_HOLSTER, 0.9 );
}

void CM41A::PrimaryAttack()
{
	if( m_pPlayer->m_bIsHolster )
        {
                WeaponIdle();
                return;
        }

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector vecDir;

#ifdef CLIENT_DLL
	if ( bIsMultiplayer() )
#else
	if ( g_pGameRules->IsMultiplayer() )
#endif
	{
		// optimized multiplayer. Widened to make it easier to hit a moving player
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_6DEGREES, 8192, BULLET_PLAYER_M41A, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}
	else
	{
		// single player spread
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_M41A, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}

  int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usM41A, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1f;

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1f;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}



void CM41A::SecondaryAttack( void )
{
	if( m_pPlayer->m_bIsHolster )
        {
                WeaponIdle();
                return;
        }

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	if (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] == 0)
	{
		PlayEmptySound( );
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_DANGER;
	m_pPlayer->m_flStopExtraSoundTime = UTIL_WeaponTimeBase() + 0.2f;
			
	m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]--;

	SendWeaponAnim( M41A_LAUNCH );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// play this sound through BODY channel so we can hear it if player didn't stop firing MP3
	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, RANDOM_LONG( 0, 1 ) ? "weapons/m41aglauncher.wav" : "weapons/m41aglauncher2.wav", 0.8, ATTN_NORM );

 	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	// we don't add in player velocity anymore.
	CGrenade::ShootContact( m_pPlayer->pev, 
				m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16, 
				gpGlobals->v_forward * 800, gSkillData.plrDmgM41AGrenade );

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0f;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;// idle pretty soon after shooting.

	if (!m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType])
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_pPlayer->pev->punchangle.x -= 10;
}

void CM41A::Reload( void )
{
	if( m_pPlayer->m_bIsHolster )
        {
                WeaponIdle();
                return;
        }

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == MP5_MAX_CLIP )
		return;

	DefaultReload( MP5_MAX_CLIP, M41A_RELOAD, 2.0f );
}


void CM41A::WeaponIdle( void )
{
	if( m_pPlayer->m_bIsHolster )
        {
                if( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
                {
                        m_pPlayer->m_bIsHolster = FALSE;
                        Deploy();
                }
		return;
        }

	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:	
		iAnim = M41A_LONGIDLE;	
		break;
	
	default:
	case 1:
		iAnim = M41A_IDLE1;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}

BOOL CM41A::IsUseable()
{
	//Can be used if the player has AR grenades. - Solokiller
	return CBasePlayerWeapon::IsUseable() || m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] > 0;
}
