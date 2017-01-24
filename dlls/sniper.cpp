//fixed (maybe)
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h" 
#include "util.h" 
#include "cbase.h" 
#include "weapons.h" 
#include "monsters.h" 
#include "player.h" 
#include "gamerules.h" 
#include "shake.h"

#include "nodes.h" 
#include "soundent.h"

enum gauss_e 
{ 
SNIPARS_IDLE = 0, 
SNIPARS_DRAW,
SNIPARS_FIRE,
SNIPARS_RELOAD
};

LINK_ENTITY_TO_CLASS( weapon_snipars, CSnipars );

int CSnipars::GetItemInfo(ItemInfo *p) 
{ 
p->pszName = STRING(pev->classname); 
p->pszAmmo1 = "snipars"; 
p->iMaxAmmo1 = _SNIPARS_MAX_CARRY; 
p->pszAmmo2 = NULL; 
p->iMaxAmmo2 = -1; 
p->iMaxClip = SNIPARS_MAX_CLIP; 
p->iFlags = 0; 
p->iSlot = 3; 
p->iPosition = 4; 
p->iId = m_iId = WEAPON_SNIPARS; 
p->iWeight = SNIPARS_WEIGHT;

return 1; 
}

int CSnipars::AddToPlayer( CBasePlayer *pPlayer )
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

void CSnipars::Spawn( ) 
{ 
pev->classname = MAKE_STRING("weapon_snipars");
Precache( ); 
m_iId = WEAPON_SNIPARS; 
SET_MODEL(ENT(pev), "models/w_sniper.mdl");

m_iDefaultAmmo = SNIPARS_DEFAULT_GIVE;

FallInit();
}

void CSnipars::Precache( void ) 
{ 
PRECACHE_MODEL("models/v_sniper.mdl"); 
PRECACHE_MODEL("models/w_sniper.mdl"); 
PRECACHE_MODEL("models/p_sniper.mdl");

PRECACHE_MODEL("models/w_357ammo.mdl"); 
PRECACHE_SOUND("items/9mmclip1.wav");

PRECACHE_SOUND ("weapons/sniper_shoot1.wav");
PRECACHE_SOUND ("weapons/sniper_shoot2.wav");  
PRECACHE_SOUND ("weapons/sniper_shoot3.wav"); 
PRECACHE_SOUND ("weapons/sniper_shoot4.wav"); 
PRECACHE_SOUND ("weapons/sniper_shoot5.wav"); 

m_usFireSniper = PRECACHE_EVENT( 1, "events/glock2.sc" ); 
}

BOOL CSnipars::Deploy( ) 
{ 
return DefaultDeploy( "models/v_sniper.mdl", "models/p_sniper.mdl", SNIPARS_DRAW, "snipars", UseDecrement() );
}

void CSnipars::Holster( int skiplocal /* = 0 */ ) 
{ 
m_fInReload = FALSE;

if ( m_fInZoom ) 
{ 
SecondaryAttack(); 
} 
m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0; 
m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); 
}

void CSnipars::SecondaryAttack( void ) 
{
if ( m_pPlayer->pev->fov != 0 ) 
{ 
m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0; 
m_fInZoom = 0; 
#ifndef CLIENT_DLL UTIL_ScreenFade( m_pPlayer, Vector(0,0,0), 0.5, 0.25, 255, FFADE_IN ); 
#endif 
} 
else if ( m_pPlayer->pev->fov != 15 ) 
{ 
m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 15; 
m_fInZoom = 1; 
#ifndef CLIENT_DLL UTIL_ScreenFade( m_pPlayer, Vector(0,0,0), 0.5, 0.25, 255, FFADE_IN ); 
#endif 
} 
pev->nextthink = UTIL_WeaponTimeBase() + 0.1; 
m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0; 
}

void CSnipars::PrimaryAttack( void ) 
{
Shoot( 0.0001, 1.5, TRUE );
	switch( RANDOM_LONG( 0, 3 ) )
			{
			case 0:
				EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/sniper_shoot1.wav", 1, ATTN_NORM );
				break;
			case 1:
				EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/sniper_shoot2.wav", 1, ATTN_NORM );
				break;
			case 2:
				EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/sniper_shoot3.wav", 1, ATTN_NORM );
				break;
			case 3:
				EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/sniper_shoot4.wav", 1, ATTN_NORM );
				break;
			case 4:
				EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/sniper_shoot5.wav", 1, ATTN_NORM );
				break;
			};
}

void CSnipars::Shoot( float flSpread , float flCycleTime, BOOL fUseAutoAim ) 
{ 
if (m_pPlayer->pev->waterlevel == 3) 
{
EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/sniper_cock1.wav", 0.8, ATTN_NORM); 
m_flNextPrimaryAttack = 0.15; return; 
} 
if (m_iClip <= 0) 
{ 
if (m_fFireOnEmpty) 
{ 
m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2; 
} 
return; 
}
m_iClip -= 1;

m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

int flags;

#if defined( CLIENT_WEAPONS ) 
flags = FEV_NOTHOST; 
#else flags = 0; 
#endif

// player "shoot" animation 
m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME; 
m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

Vector vecSrc = m_pPlayer->GetGunPosition( ); 
Vector vecAiming;

if ( fUseAutoAim ) 
{ 
vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES ); 
} 
else 
{ 
vecAiming = gpGlobals->v_forward; 
}

Vector vecDir; vecDir = m_pPlayer->FireBulletsPlayer( 5, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_SNIPARS, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), fUseAutoAim ? m_usFireSniper : m_usFireSniper, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );

m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0) 

m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); 
}

void CSnipars::Reload( void ) 
{ 
if ( m_fInZoom ) 
{
 SecondaryAttack(); 
}
if ( m_pPlayer->ammo_snipars <= 0 ) 
return;

if ( m_pPlayer->pev->fov != 0 ) 
{ 
m_fInZoom = FALSE; 
m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0; // 0 means reset to default fov 
}
if (m_iClip == 0) 
DefaultReload( 5, SNIPARS_RELOAD, 1.5 ); 
else 
DefaultReload( 5, SNIPARS_RELOAD, 1.5 );
}

void CSnipars::WeaponIdle( void ) 
{ 
m_pPlayer->GetAutoaimVector( AUTOAIM_2DEGREES ); 

if ( m_flTimeWeaponIdle < UTIL_WeaponTimeBase() ) 
{ 
float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
if (flRand <= 0.75) 
{ 
if (m_iClip) 
{ 
SendWeaponAnim( SNIPARS_IDLE ); 
} 
else 
{
SendWeaponAnim( SNIPARS_IDLE ); 
} 
m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 12.0;
}

} 
}

class CSniparsAmmo : public CBasePlayerAmmo 
{ 
void Spawn( void ) 
{
Precache( ); 
SET_MODEL(ENT(pev), "models/w_sniper.mdl"); 
CBasePlayerAmmo::Spawn( ); 
} 
void Precache( void ) 
{ 
PRECACHE_MODEL ("models/w_357ammo.mdl"); 
PRECACHE_SOUND("items/9mmclip1.wav"); 
} 
BOOL AddAmmo( CBaseEntity *pOther ) 
{
if (pOther->GiveAmmo( AMMO_SNIPARSBOX_GIVE, "snipars", _SNIPARS_MAX_CARRY ) != -1) 
{ 
EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM); 
return TRUE; 
} 
return FALSE; 
} 
}; 
LINK_ENTITY_TO_CLASS( ammo_snipars2, CSniparsAmmo );

#endif
