#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h" 
#include "util.h" 
#include "cbase.h" 
#include "weapons.h" 
#include "monsters.h" 
#include "player.h" 
#include "gamerules.h" 

enum snipars_e
{
	SNIPARS_IDLE = 0,
	SNIPARS_DRAW,
	SNIPARS_FIRE,
	SNIPARS_RELOAD
};

LINK_ENTITY_TO_CLASS( weapon_snipars, CSnipars );

int CSnipars::GetItemInfo( ItemInfo *p ) 
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "snipars";
	p->iMaxAmmo1 = SNIPARS_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SNIPARS_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 2;
	p->iPosition = 4;
	p->iId = m_iId = WEAPON_SNIPARS;
	p->iWeight = SNIPARS_WEIGHT;

	return 1;
}

int CSnipars::AddToPlayer( CBasePlayer *pPlayer )
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

void CSnipars::Spawn( void )
{
	pev->classname = MAKE_STRING( "weapon_snipars" );
	Precache();
	m_iId = WEAPON_SNIPARS;
	SET_MODEL( ENT( pev ), "models/w_sniper.mdl" );

	m_iDefaultAmmo = SNIPARS_DEFAULT_GIVE;

	FallInit();
}

void CSnipars::Precache( void ) 
{
	PRECACHE_MODEL( "models/v_sniper.mdl" );
	PRECACHE_MODEL( "models/w_sniper.mdl" );
	PRECACHE_MODEL( "models/p_sniper.mdl" );
	PRECACHE_MODEL( "models/w_357ammobox.mdl" );
	PRECACHE_MODEL( "models/w_357ammo.mdl" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );
	PRECACHE_SOUND( "weapons/357_reload1.wav" );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	PRECACHE_SOUND( "weapons/sniper_shoot1.wav" );
	PRECACHE_SOUND( "weapons/sniper_shoot1.wav" );
	PRECACHE_SOUND( "weapons/sniper_shoot2.wav" );
	PRECACHE_SOUND( "weapons/sniper_shoot3.wav" );
	PRECACHE_SOUND( "weapons/sniper_shoot4.wav" );
	PRECACHE_SOUND( "weapons/sniper_shoot5.wav" );

	m_usFireSniper = PRECACHE_EVENT( 1, "events/snipars.sc" );
}

BOOL CSnipars::Deploy( void )
{
	return DefaultDeploy( "models/v_sniper.mdl", "models/p_sniper.mdl", SNIPARS_DRAW, "snipars" );
}

void CSnipars::Holster( int skiplocal /* = 0 */ ) 
{
	m_fInReload = FALSE;

	if( m_fInZoom )
	{
		SecondaryAttack();
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0f;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CSnipars::SecondaryAttack( void ) 
{
	if( m_pPlayer->pev->fov != 0 )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;
		m_fInZoom = 0;
	}
	else if( m_pPlayer->pev->fov != 40 )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 40;
		m_fInZoom = 1;
	}
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
}

void CSnipars::PrimaryAttack( void ) 
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

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_357, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFireSniper, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.5f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10.0f, 15.0f );
}

void CSnipars::Reload( void ) 
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == SNIPARS_MAX_CLIP )
		return;

	if( m_pPlayer->pev->fov != 0 )
	{
		m_fInZoom = FALSE;
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;  // 0 means reset to default fov
	}

	if( DefaultReload( SNIPARS_MAX_CLIP, SNIPARS_RELOAD, 2.0f ))
	{
		m_flSoundDelay = UTIL_WeaponTimeBase() + 1.5f;
	}
}

void CSnipars::WeaponIdle( void ) 
{ 
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

        // ALERT( at_console, "%.2f\n", gpGlobals->time - m_flSoundDelay );
	if( m_flSoundDelay != 0 && m_flSoundDelay <= UTIL_WeaponTimeBase() )
	{
		EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/357_reload1.wav", RANDOM_FLOAT( 0.8f, 0.9f ), ATTN_NORM );
		m_flSoundDelay = 0.0f;
	}

	if( m_flTimeWeaponIdle < UTIL_WeaponTimeBase() )
	{
		SendWeaponAnim( SNIPARS_IDLE );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 170.0f / 30.0f );
	}
}

class CSniparsAmmo : public CBasePlayerAmmo 
{ 
	void Spawn( void )
	{
		Precache();
		SET_MODEL( ENT( pev ), "models/w_357ammo.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_357ammo.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if( pOther->GiveAmmo( AMMO_SNIPARSBOX_GIVE, "snipars", SNIPARS_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
}; 
LINK_ENTITY_TO_CLASS( ammo_snipars, CSniparsAmmo );

#endif
