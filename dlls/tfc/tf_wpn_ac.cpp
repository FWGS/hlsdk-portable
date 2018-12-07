#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

LINK_ENTITY_TO_CLASS( tf_weapon_ac, CTFAssaultC )

void CTFAssaultC::Spawn()
{
    Precache();

    m_iId = 17;
    m_iDefaultAmmo = 25;
    m_iWeaponState = 0;
    pev->solid = SOLID_TRIGGER;
}

int CTFAssaultC::GetItemInfo( ItemInfo *p )
{
    p->pszAmmo1 = "buckshot";
    p->pszName = STRING( pev->classname );
    p->iMaxAmmo1 = 200;
    p->pszAmmo2 = 0;
    p->iMaxAmmo2 = -1;
    p->iSlot = 4;
    p->iPosition = 3;
    p->iFlags = 0;
    p->iMaxClip = -1;
    p->iId = 17;
    p->iWeight = 15;
    return 1;
}

void CTFAssaultC::Holster()
{
    PLAYBACK_EVENT_FULL(1, ENT(m_pPlayer->pev), m_usWindDown, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 1, 0);
    m_iWeaponState = 0;
    //tfstate
    //setspeed
    m_fInReload = 0;
    m_flTimeWeaponIdle = 2;
    m_pPlayer->m_flNextAttack = 0.1;
    SendWeaponAnim(ASSAULT_HOLSTER, 1);
}

void CTFAssaultC::Precache()
{
    PRECACHE_MODEL("models/v_tfac.mdl");
    PRECACHE_MODEL("models/p_mini.mdl");
    PRECACHE_MODEL("models/p_mini2.mdl");
    m_iShell = PRECACHE_MODEL("models/shell.mdl");
    PRECACHE_SOUND("weapons/357_cock1.wav");
    PRECACHE_SOUND("weapons/asscan1.wav");
    PRECACHE_SOUND("weapons/asscan2.wav");
    PRECACHE_SOUND("weapons/asscan3.wav");
    PRECACHE_SOUND("weapons/asscan4.wav");
    m_usWindUp = PRECACHE_EVENT(1, "events/wpn/tf_acwu.sc");
    m_usWindDown = PRECACHE_EVENT(1, "events/wpn/tf_acwd.sc");
    m_usFire = PRECACHE_EVENT(1, "events/wpn/tf_acfire.sc");
    m_usStartSpin = PRECACHE_EVENT(1, "events/wpn/tf_acsspin.sc");
    m_usSpin = PRECACHE_EVENT(1, "events/wpn/tf_acspin.sc");
    m_usACStart = PRECACHE_EVENT(1, "events/wpn/tf_acstart.sc");
}

void CTFAssaultC::WeaponIdle( void )
{
    ResetEmptySound();

    if(m_flTimeWeaponIdle <= 0.0)
    {
        if(m_iWeaponState)
        {
            PLAYBACK_EVENT_FULL(1, ENT(m_pPlayer->pev), m_usWindDown, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0);
            m_iWeaponState = 0;
            //tfstate
            //setspeed
            m_flTimeWeaponIdle = 2;
        }
        else
        {
            SendWeaponAnim(RANDOM_LONG(0, 1), 1);
            m_flTimeWeaponIdle = 12.5;
        }
    }
}

BOOL CTFAssaultC::Deploy()
{
    //Velaron: "ac" deploy
	return DefaultDeploy( "models/v_tfac.mdl", "models/p_mini.mdl", ASSAULT_DRAW, "mp5", 1 );
}

int CTFAssaultC::AddToPlayer( CBasePlayer *pPlayer )
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

void CTFAssaultC::PrimaryAttack()
{
    if(m_iWeaponState == 2)
    {
        if(m_pPlayer->ammo_buckshot <= 0)
        {
            PLAYBACK_EVENT_FULL(1, ENT(m_pPlayer->pev), m_usStartSpin, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0);
            m_iWeaponState = 3;
            m_pPlayer->SetAnimation(PLAYER_ATTACK1);
            pev->effects &= ~EF_MUZZLEFLASH;
        }
        else
            Fire();

        m_flTimeWeaponIdle = 0.1;
        m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;
        return;
    }
    if(m_iWeaponState == 3)
    {
        if(m_pPlayer->ammo_buckshot <= 0)
        {
            PLAYBACK_EVENT_FULL(1, ENT(m_pPlayer->pev), m_usSpin, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0);
            m_pPlayer->SetAnimation(PLAYER_ATTACK1);
        }
        else
        {
            m_iWeaponState = 1;
        }
        m_flTimeWeaponIdle = 0.1;
        m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;
        return;
    }
    if(m_iWeaponState == 1)
    {
        if(m_flNextPrimaryAttack > 0.0)
            return;

        PLAYBACK_EVENT_FULL(1, ENT(m_pPlayer->pev), m_usACStart, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0);
        m_iWeaponState = 2;
        m_flTimeWeaponIdle = 0.1;
        m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;
        return;
    }

    if(m_pPlayer->pev->button & 1)
    {
        PLAYBACK_EVENT_FULL(1, ENT(m_pPlayer->pev), m_usWindUp, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0);
        m_iWeaponState = 1;
        //tfstate
        //setspeed
    }

    m_flTimeWeaponIdle = 0.6;
    m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
}

void CTFAssaultC::Fire()
{
    vec3_t p_vecSrc, vecAiming, vecSpread;

    if(m_flNextPrimaryAttack > 0)
        return;

    PLAYBACK_EVENT_FULL(1, ENT(m_pPlayer->pev), m_usWindUp, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, m_pPlayer->ammo_buckshot & 1, 0);
    m_pPlayer->m_iWeaponVolume = 600;
    m_pPlayer->m_iWeaponFlash = 256;
    m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
    m_pPlayer->SetAnimation(PLAYER_ATTACK1);
    UTIL_MakeVectors(m_pPlayer->pev->v_angle);
    p_vecSrc.x = gpGlobals->v_up.x * -4.0 + gpGlobals->v_right.x + gpGlobals->v_right.x + m_pPlayer->pev->origin.x + m_pPlayer->pev->view_ofs.x;
    p_vecSrc.y = gpGlobals->v_up.y * -4.0 + gpGlobals->v_right.y + gpGlobals->v_right.y + m_pPlayer->pev->origin.y + m_pPlayer->pev->view_ofs.y;
    p_vecSrc.z = gpGlobals->v_up.z * -4.0 + gpGlobals->v_right.z + gpGlobals->v_right.z + m_pPlayer->pev->origin.z + m_pPlayer->pev->view_ofs.z;
    vecAiming = m_pPlayer->GetAutoaimVector(0.087155744);
    vecSpread.x = 0.1;
    vecSpread.y = 0.1;
    vecSpread.z = 0.0;
    FireBullets(5, p_vecSrc, vecAiming, vecSpread, 8192.0, 7, 8, 7, 0);
    m_pPlayer->ammo_buckshot--;
}