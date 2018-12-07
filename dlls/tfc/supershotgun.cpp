#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

LINK_ENTITY_TO_CLASS( tf_weapon_supershotgun, CTFSuperShotgun )

void CTFSuperShotgun::Spawn()
{
    Precache();
    
    m_iId = 9;
    m_iDefaultAmmo = 16;
    m_iMaxClipSize = 16;
    m_iShellsReloaded = 2;
    m_fReloadTime = 0.375;
    m_flPumpTime = 1000;
    pev->solid = SOLID_TRIGGER;
}

void CTFSuperShotgun::Precache( void )
{
    PRECACHE_MODEL("models/v_tfc_shotgun.mdl");
    PRECACHE_MODEL("models/p_shotgun.mdl");
    PRECACHE_MODEL("models/p_shotgun2.mdl");
    m_iShell = PRECACHE_MODEL("models/shotgunshell.mdl");
    PRECACHE_SOUND("weapons/dbarrel1.wav");
    PRECACHE_SOUND("weapons/sbarrel1.wav");
    PRECACHE_SOUND("weapons/reload1.wav");
    PRECACHE_SOUND("weapons/reload3.wav");
    PRECACHE_SOUND("weapons/357_cock1.wav");
    PRECACHE_SOUND("weapons/scock1.wav");
    PRECACHE_SOUND("weapons/shotgn2.wav");
    m_usFireSuperShotgun = PRECACHE_EVENT(1, "events/wpn/tf_ssg.sc");
    m_usReloadShotgun = PRECACHE_EVENT(1, "events/wpn/tf_sgreload.sc");
    m_usPumpShotgun = PRECACHE_EVENT(1, "events/wpn/tf_sgpump.sc");
}

int CTFSuperShotgun::GetItemInfo( ItemInfo *p )
{
    p->pszAmmo1 = "buckshot";
    p->pszName = STRING( pev->classname );
    p->iMaxAmmo1 = 200;
    p->pszAmmo2 = 0;
    p->iMaxAmmo2 = -1;
    p->iSlot = 2;
    p->iPosition = 2;
    p->iFlags = 0;
    p->iMaxClip = m_iMaxClipSize;
    p->iId = 9;
    p->iWeight = 15;
    return 1;
}

BOOL CTFSuperShotgun::Deploy()
{
	return DefaultDeploy( "models/v_tfc_shotgun.mdl", "models/p_shotgun.mdl", 6, "shotgun", 1 );
}

void CTFSuperShotgun::PrimaryAttack()
{
    float flDelta;
    vec3_t *p_vecDirShooting;
    vec3_t vecSrc, vecAiming;
    vec3_t spread;

    if ( m_iClip == 1 )
    {
      CTFShotgun::PrimaryAttack();
      return;
    }

    if ( m_iClip <= 0 )
    {
        Reload();
        if( m_iClip == 0 )
            PlayEmptySound();
    }
    else
    {
        m_pPlayer->m_iWeaponVolume = 1000;
        m_pPlayer->m_iWeaponFlash = 256;
        PLAYBACK_EVENT_FULL(1, ENT(m_pPlayer->pev), m_usFireSuperShotgun, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0);
        m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
        vecSrc = m_pPlayer->GetGunPosition();
        vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
        spread.x = 0.04;
        spread.y = 0.04;
        spread.z = 0;
        m_pPlayer->FireBullets(14, vecSrc, vecAiming, spread, 2048.0, 6, 4, 4, 0);
        if ( !m_iClip && m_pPlayer->ammo_buckshot <= 0 )
        {
            p_vecDirShooting = 0;
            flDelta = 0.0;
        }
        m_iClip -= 2;
        pev->effects |= EF_MUZZLEFLASH;
        m_fInSpecialReload = 0;
        m_flTimeWeaponIdle = 5;
        //m_pPlayer->tfstate &= 2;
        if ( m_iClip )
            m_flPumpTime = 0.7;
        m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
    }
}