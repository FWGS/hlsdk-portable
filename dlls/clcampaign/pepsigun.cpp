#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

enum pepsigun_e
{
	PEPSIGUN_IDLE = 0,
	PEPSIGUN_FIRE,
	PEPSIGUN_OPEN,
	PEPSIGUN_INSERT,
	PEPSIGUN_CLOSE,
	PEPSIGUN_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_pepsigun, CPepsigun )

void CPepsigun::Spawn()
{
	Precache();
	m_iId = WEAPON_PEPSIGUN;
	SET_MODEL( ENT( pev ), "models/w_pepsigun.mdl" );

	m_iDefaultAmmo = PEPSIGUN_DEFAULT_GIVE;

	FallInit();// get ready to fall
}

void CPepsigun::Precache( void )
{
	PRECACHE_MODEL( "models/v_pepsigun.mdl" );
	PRECACHE_MODEL( "models/w_grenade.mdl" );
	PRECACHE_MODEL( "models/w_pepsigun.mdl" );
	PRECACHE_MODEL( "models/p_pepsigun.mdl" );

	PRECACHE_SOUND( "items/9mmclip1.wav" );

	PRECACHE_SOUND( "weapons/pepsigun_shoot.wav" );//shotgun

	PRECACHE_SOUND( "weapons/reload1.wav" );	// shotgun reload
	PRECACHE_SOUND( "weapons/reload3.wav" );	// shotgun reload
	
	PRECACHE_SOUND( "weapons/357_cock1.wav" ); // gun empty sound
	PRECACHE_SOUND( "weapons/scock1.wav" );	// cock gun

	m_usPepsigun = PRECACHE_EVENT( 1, "events/pepsigun.sc" );
}

int CPepsigun::AddToPlayer( CBasePlayer *pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ))
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

int CPepsigun::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "Hand Grenade";
	p->iMaxAmmo1 = PEPSIGUN_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = PEPSIGUN_MAX_CLIP;
	p->iSlot = 4;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_PEPSIGUN;
	p->iWeight = PEPSIGUN_WEIGHT;

	return 1;
}

BOOL CPepsigun::Deploy()
{
	return DefaultDeploy( "models/v_pepsigun.mdl", "models/p_pepsigun.mdl", PEPSIGUN_DRAW, "pepsigun" );
}

void CPepsigun::PrimaryAttack()
{
	// don't fire underwater
	if( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextAttackDelay( 0.15f );
		return;
	}

	if( m_iClip <= 0 )
	{
		Reload();
		if( m_iClip == 0 )
			PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	SendWeaponAnim( PEPSIGUN_FIRE );

	EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/pepsigun_shoot.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG( 0, 0x1f ) );
#if !CLIENT_DLL
	CGrenade::ShootTimed( m_pPlayer->pev, m_pPlayer->pev->origin + gpGlobals->v_forward * 34 + Vector( 0, 0, 32 ), gpGlobals->v_forward * 800, 2.5 );
#endif
	if( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flNextPrimaryAttack = GetNextAttackDelay( 0.75f );
	if( m_iClip != 0 )
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75f;
	m_fInSpecialReload = 0;
}

void CPepsigun::Reload( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == PEPSIGUN_MAX_CLIP )
		return;

	// don't reload until recoil is done
	if( m_flNextPrimaryAttack > UTIL_WeaponTimeBase() )
		return;

	// check to see if we're ready to reload
	if( m_fInSpecialReload == 0 )
	{
		SendWeaponAnim( PEPSIGUN_OPEN );
		m_fInSpecialReload = 1;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.6f;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.6f;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0f;
		return;
	}
	else if( m_fInSpecialReload == 1 )
	{
		if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
			return;
		// was waiting for gun to move to side
		m_fInSpecialReload = 2;

		if( RANDOM_LONG( 0, 1 ) )
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/reload1.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG( 0, 0x1f ) );
		else
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/reload3.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG( 0, 0x1f ) );

		SendWeaponAnim( PEPSIGUN_INSERT );

		m_flNextReload = UTIL_WeaponTimeBase() + 0.5f;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;
	}
	else
	{
		// Add them to the clip
		m_iClip += 1;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_fInSpecialReload = 1;
	}
}

void CPepsigun::WeaponIdle( void )
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if( m_flTimeWeaponIdle <  UTIL_WeaponTimeBase() )
	{
		if( m_iClip == 0 && m_fInSpecialReload == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			Reload();
		}
		else if( m_fInSpecialReload != 0 )
		{
			if( m_iClip != 8 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
			{
				Reload();
			}
			else
			{
				// reload debounce has timed out
				SendWeaponAnim( PEPSIGUN_CLOSE );
				
				m_fInSpecialReload = 0;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5f;
			}
		}
		else
		{
			int iAnim;
			float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
			if( flRand <= 0.8f )
			{
				iAnim = PEPSIGUN_IDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 60.0f / 12.0f );// * RANDOM_LONG( 2, 5 );
			}
			else if( flRand <= 0.95f )
			{
				iAnim = PEPSIGUN_IDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 20.0f / 9.0f );
			}
			else
			{
				iAnim = PEPSIGUN_IDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 20.0f / 9.0f );
			}
			SendWeaponAnim( iAnim );
		}
	}
}
