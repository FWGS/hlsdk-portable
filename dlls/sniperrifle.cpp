/***
*
*	This weapon was made by XF-Alien (Edited by FreeSlave)	
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
#include "shake.h"


enum sniperrifle_e
{
	SNIPERRIFLE_SLOWIDLE = 0,
	SNIPERRIFLE_FIRE1,
	SNIPERRIFLE_FIRE2,
	SNIPERRIFLE_RELOAD,
	SNIPERRIFLE_RELOAD2,
	SNIPERRIFLE_DRAW,
	SNIPERRIFLE_DRAW_EMPTY,
	SNIPERRIFLE_HOLSTER,
	SNIPERRIFLE_HOLSTER_EMPTY,
};



LINK_ENTITY_TO_CLASS( weapon_barrett_m82a1, CSniperrifle );


//=========================================================
//=========================================================
void CSniperrifle::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_barrett_m82a1"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/w_barrett_m82.mdl");
	m_iId = WEAPON_BARRETT_M82A1;

	m_iDefaultAmmo = RANDOM_LONG(3,5);

	FallInit();// get ready to fall down.
}


void CSniperrifle::Precache( void )
{
	PRECACHE_MODEL("models/v_barrett_m82.mdl");
	PRECACHE_MODEL("models/w_barrett_m82.mdl");
	PRECACHE_MODEL("models/p_barrett_m82.mdl");

	m_iShell = PRECACHE_MODEL ("models/snipershell.mdl");// brass shell


	PRECACHE_MODEL("models/w_14mmclip.mdl");
	PRECACHE_SOUND("items/50BMG_pickup.wav");              

	PRECACHE_SOUND("weapons/sniper_mag_out.wav");
	PRECACHE_SOUND("weapons/sniper_mag_in.wav");
	PRECACHE_SOUND("weapons/sniper_bolt2.wav");
	PRECACHE_SOUND("weapons/sniper_bolt3.wav");

	PRECACHE_SOUND(	"fvox/ammo_low.wav");

	PRECACHE_SOUND("weapons/sniper_optic1.wav");
	PRECACHE_SOUND("weapons/sniper_optic2.wav");

	PRECACHE_SOUND ("weapons/sniper_fire.wav");// Fire sound

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usSniperrifle = PRECACHE_EVENT( 1, "events/barrett_m82.sc" );

	m_flUnzoomTime = 0;
}

int CSniperrifle::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "14mm";
	p->iMaxAmmo1 = _14MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SNIPERRIFLE_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_BARRETT_M82A1;
	p->iWeight = SNIPERRIFLE_WEIGHT;

	return 1;
}

int CSniperrifle::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL CSniperrifle::Deploy( )
{
	g_engfuncs.pfnSetClientMaxspeed(m_pPlayer->edict(), 230 );

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.75;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75;

	if (m_iClip <= 0)
	{
		return DefaultDeploy( "models/v_barrett_m82.mdl", "models/p_barrett_m82.mdl", SNIPERRIFLE_DRAW_EMPTY, "sniperrifle" );
	}
	else
	{
		return DefaultDeploy( "models/v_barrett_m82.mdl", "models/p_barrett_m82.mdl", SNIPERRIFLE_DRAW, "sniperrifle" );
	}
}

void CSniperrifle::Holster( int skiplocal /* = 0 */ )
{
	g_engfuncs.pfnSetClientMaxspeed(m_pPlayer->edict(), 230 );
   	m_fInReload = FALSE;// cancel any reload in progress.

	if (m_pPlayer->pev->fov != 0 )
	{
		SetZoom(0);

	#ifndef CLIENT_DLL
		UTIL_ScreenFade( m_pPlayer, Vector(0,0,0), 0.25, 0.05, 255, FFADE_IN );
	#endif

		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/sniper_optic2.wav", 1.0, ATTN_NORM);
	}

   m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

   	if (m_iClip <= 0)
	{
		SendWeaponAnim( SNIPERRIFLE_HOLSTER_EMPTY );
	}
	else
	{
		SendWeaponAnim( SNIPERRIFLE_HOLSTER );
	}
	
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CSniperrifle::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

#ifndef CLIENT_DLL

	UTIL_ScreenShake( pev->origin, 10, 150.0, 0.2, 120 ); 
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );	// player "shoot" animation

#endif

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	if (m_iClip == 1)
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_STATIC, "fvox/ammo_low.wav", 1.0, ATTN_NORM);

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector vecDir;

	m_flUnzoomTime = gpGlobals->time + 0.4;
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_0DEGREES, 8192, BULLET_PLAYER_14MM, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );


  int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSniperrifle, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );
	
	if (m_pPlayer->pev->fov == 0 )
	{
		m_flUnzoomTime = 0;
	}

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 2.150;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2.200;


	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CSniperrifle::SecondaryAttack()
{
	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
	{
		WeaponIdle();
		return;
	}
 
#ifndef CLIENT_DLL
	UTIL_ScreenFade( m_pPlayer, Vector(0,0,0), 0.25, 0.05, 255, FFADE_IN );
#endif

	if ( m_pPlayer->m_iFOV == 65 )
	{
		SetZoom(35);

		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/sniper_optic1.wav", 1.0, ATTN_NORM);
	}
	else if ( m_pPlayer->m_iFOV == 0 )
	{
		SetZoom(65);

		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/sniper_optic1.wav", 1.0, ATTN_NORM);
	}	
	else
	{
		SetZoom(0);

		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/sniper_optic2.wav", 1.0, ATTN_NORM);
	}

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
}

void CSniperrifle::Reload( void )
{
	if (m_pPlayer->m_iFOV != 0 )
	{
		SetZoom(0);

#ifndef CLIENT_DLL
		UTIL_ScreenFade( m_pPlayer, Vector(0,0,0), 0.25, 0.05, 255, FFADE_IN );
#endif

		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/sniper_optic2.wav", 1.0, ATTN_NORM);
	}

	int iResult;

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == SNIPERRIFLE_MAX_CLIP )
	{
		return;
	}
	else
	{
		if (m_iClip == 0)
		{
			iResult = DefaultReload( 5, SNIPERRIFLE_RELOAD2, 2.1 );
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = 3.25;
		}
		else
		{
			iResult = DefaultReload( 5, SNIPERRIFLE_RELOAD, 2.1 );
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = 2.55;
		}
	}

	if (iResult)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CSniperrifle::WeaponIdle( void )
{

	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if (m_iClip != 0)
	{
	SendWeaponAnim( SNIPERRIFLE_SLOWIDLE );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
	}
}

void CSniperrifle::ItemPostFrame()
{
	if (m_pPlayer->m_iFOV != 0 && m_flUnzoomTime && m_flUnzoomTime < gpGlobals->time) 
	{
		SetZoom(0);
		m_flUnzoomTime = 0;

#ifndef CLIENT_DLL
		UTIL_ScreenFade( m_pPlayer, Vector(0,0,0), 0.25, 0.05, 255, FFADE_IN );
#endif
	}

	CBasePlayerWeapon::ItemPostFrame();
}

void CSniperrifle::SetZoom(int fov)
{
	if (fov)
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = fov;
		m_pPlayer->pev->viewmodel = iStringNull;
	}
	else
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;
		m_pPlayer->pev->viewmodel = MAKE_STRING("models/v_barrett_m82.mdl");
	}
}

class CSniperrifleAmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_14mmclip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_14mmclip.mdl");
		PRECACHE_SOUND("items/556mm_pickup.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( SNIPERRIFLE_DEFAULT_GIVE, "14mm", _14MM_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/50BMG_pickup.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( ammo_14mmclip, CSniperrifleAmmoClip );
