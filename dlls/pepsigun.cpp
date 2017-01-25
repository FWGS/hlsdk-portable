/*
Здесь был rainbow
*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

#define	HANDGRENADE_PRIMARY_VOLUME		450
#define 	WEAPON_PEPSIGUN	22


enum handgrenade_e
{
	HANDGRENADE_IDLE = 0,
	HANDGRENADE_FIRE,
	HANDGRENADE_OPEN,
	HANDGRENADE_INSERT,
	HANDGRENADE_CLOSE,
	HANDGRENADE_DRAW

};

LINK_ENTITY_TO_CLASS( weapon_pepsigun, CPepsigun )
LINK_ENTITY_TO_CLASS( ammo_pepsi, CPepsigun )

void CPepsigun::Spawn()
{
	Precache();
	m_iId = WEAPON_PEPSIGUN;
	SET_MODEL( ENT( pev ), "models/w_pepsigun.mdl" );

#ifndef CLIENT_DLL
	pev->dmg = 80;
#endif
	m_iDefaultAmmo = 100;

	FallInit();// get ready to fall down.
}

void CPepsigun::Precache( void )
{
	PRECACHE_MODEL( "models/w_pepsigun.mdl" );
	PRECACHE_MODEL( "models/v_pepsigun.mdl" );
	PRECACHE_MODEL( "models/p_pepsigun.mdl" );
	PRECACHE_SOUND( "weapons/pepsigun_shoot.wav");
}

int CPepsigun::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "pepsi";
	p->iMaxAmmo1 = 20000;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 8;
	p->iSlot = 2;
	p->iPosition = 4;
	p->iId = m_iId = WEAPON_PEPSIGUN;
	p->iWeight = 10;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}

BOOL CPepsigun::Deploy()
{
	m_flReleaseThrow = -1;
	return DefaultDeploy( "models/v_pepsigun.mdl", "models/p_pepsigun.mdl", HANDGRENADE_DRAW, "crowbar" );
}

BOOL CPepsigun::CanHolster( void )
{
	// can only holster hand grenades when not primed!
	return ( m_flStartThrow == 0 );
}

void CPepsigun::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{

	}
	else
	{
		// no more grenades!
		m_pPlayer->pev->weapons &= ~( 1 << WEAPON_PEPSIGUN );
		SetThink( &CBasePlayerItem::DestroyItem );
		pev->nextthink = gpGlobals->time + 0.1;
	}

	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM );
}

void CPepsigun::PrimaryAttack()
{

		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = 0;
		m_flTimeWeaponIdle = gpGlobals->time + 0.25;

	if(m_iClip > 0)
	{
	m_flStartThrow = gpGlobals->time;
	m_flReleaseThrow = 0;
	m_flTimeWeaponIdle = gpGlobals->time + 0.25;
	}
}
void CPepsigun::Reload( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == SHOTGUN_MAX_CLIP )
		return;

	// don't reload until recoil is done
	if( m_flNextPrimaryAttack > gpGlobals->time)
		return;

	// check to see if we're ready to reload
	if( m_fInSpecialReload == 0 )
	{
		SendWeaponAnim( HANDGRENADE_OPEN );
		m_fInSpecialReload = 1;
		m_pPlayer->m_flNextAttack = gpGlobals->time+ 0.6;
		m_flTimeWeaponIdle = gpGlobals->time + 0.6;
		m_flNextPrimaryAttack = gpGlobals->time + 1.0;
		m_flNextSecondaryAttack = gpGlobals->time + 1.0;
		return;
	}
	else if( m_fInSpecialReload == 1 )
	{
		if( m_flTimeWeaponIdle > gpGlobals->time )
			return;
		// was waiting for gun to move to side
		m_fInSpecialReload = 2;

		if( RANDOM_LONG( 0, 1 ) )
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/reload1.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG( 0, 0x1f ) );
		else
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/reload3.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG( 0, 0x1f ) );

		SendWeaponAnim( HANDGRENADE_INSERT );

		m_flNextReload = gpGlobals->time + 0.5;
		m_flTimeWeaponIdle = gpGlobals->time + 0.5;
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
				SendWeaponAnim( HANDGRENADE_CLOSE );
				m_fInSpecialReload = 0;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
			}
		}
	if( m_flReleaseThrow == 0 && m_flStartThrow )
		 m_flReleaseThrow = gpGlobals->time;

	if( m_flTimeWeaponIdle > gpGlobals->time )
		return;

	if( m_flStartThrow )
	{
		Vector angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		if( angThrow.x < 0 )
			angThrow.x = -10 + angThrow.x * ( ( 90 - 10 ) / 90.0 );
		else
			angThrow.x = -10 + angThrow.x * ( ( 90 + 10 ) / 90.0 );

		float flVel = ( 90 - angThrow.x ) * 4;
		if( flVel > 500 )
			flVel = 1400;

		UTIL_MakeVectors( angThrow );

		Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16;

		Vector vecThrow = gpGlobals->v_forward * flVel + m_pPlayer->pev->velocity;

		// alway explode 3 seconds after the pin was pulled
		float time = m_flStartThrow - gpGlobals->time + 3.0;
		if( time < 0 )
			time = 0;

		CGrenade::ShootTimed( m_pPlayer->pev, vecSrc, vecThrow, time );
		m_iClip--;
		if( flVel < 500 )
		{
		EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/pepsigun_shoot.wav", 1, ATTN_NORM );
		SendWeaponAnim( HANDGRENADE_FIRE );
		}
		else if( flVel < 1000 )
		{
			EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/pepsigun_shoot.wav", 1, ATTN_NORM );
			SendWeaponAnim( HANDGRENADE_FIRE );
		}
		else
		{
			EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/pepsigun_shoot.wav", 1, ATTN_NORM );
			SendWeaponAnim( HANDGRENADE_FIRE );
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_flReleaseThrow = 0;
		m_flStartThrow = 0;
		m_flNextPrimaryAttack = gpGlobals->time + 0.5;
		m_flTimeWeaponIdle = gpGlobals->time + 0.5;


		if( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			// just threw last grenade
			// set attack times in the future, and weapon idle in the future so we can see the whole throw
			// animation, weapon idle will automatically retire the weapon for us.
			m_flTimeWeaponIdle = m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->time + 0.5;// ensure that the animation can finish playing
		}
		return;
	}
	else if( m_flReleaseThrow > 0 )
	{
		// we've finished the throw, restart.
		m_flStartThrow = 0;

		if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			SendWeaponAnim( HANDGRENADE_DRAW );
		}
		else
		{
			RetireWeapon();
			return;
		}

		m_flTimeWeaponIdle = gpGlobals->time + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		m_flReleaseThrow = -1;
		return;
	}

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if( flRand <= 0.75 )
		{
			iAnim = HANDGRENADE_IDLE;
			m_flTimeWeaponIdle = gpGlobals->time + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );// how long till we do this again.
		}
		else
		{
			m_flTimeWeaponIdle = gpGlobals->time + 75.0 / 30.0;
		}
	if( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
}
}