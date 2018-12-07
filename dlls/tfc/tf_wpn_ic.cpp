#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

LINK_ENTITY_TO_CLASS( tf_weapon_ic, CTFIncendiaryC )

void CTFIncendiaryC::Spawn()
{
    Precache();

    m_iId = 15;
    SET_MODEL(ENT(pev), "models/w_rpg.mdl");
    m_iDefaultAmmo = 50;
    pev->solid = SOLID_TRIGGER;
}

int CTFIncendiaryC::GetItemInfo( ItemInfo *p )
{
    p->pszAmmo1 = "rockets";
    p->pszName = STRING( pev->classname );
    p->iMaxAmmo1 = 20;
    p->pszAmmo2 = 0;
    p->iMaxAmmo2 = -1;
    p->iSlot = 4;
    p->iPosition = 2;
    p->iFlags = 0;
    p->iMaxClip = -1;
    p->iId = 15;
    p->iWeight = 20;
    return 1;
}

void CTFIncendiaryC::Holster()
{
    m_fInReload = 0;
    SendWeaponAnim(4, 1);
    m_pPlayer->m_flNextAttack = 0.5;
}

void CTFIncendiaryC::Precache()
{
    PRECACHE_MODEL("models/w_rpg.mdl");
    PRECACHE_MODEL("models/v_rpg.mdl");
    PRECACHE_MODEL("models/p_rpg.mdl");
    PRECACHE_MODEL("models/p_rpg2.mdl");
    PRECACHE_SOUND("weapons/9mmclip1.wav");
    UTIL_PrecacheOther("tf_ic_rocket");
    PRECACHE_SOUND("weapons/rocketfire1.wav");
    PRECACHE_SOUND("weapons/glauncher.wav");
    m_usFireIC = PRECACHE_EVENT(1, "events/wpn/tf_ic.sc");
}

void CTFIncendiaryC::WeaponIdle( void )
{
    if(m_flTimeWeaponIdle <= 0.0)
    {
        if(m_pPlayer->ammo_rockets <= 0)
            m_flTimeWeaponIdle = 1;
        else
        {
            m_flTimeWeaponIdle = 3;
            SendWeaponAnim(1, 1);
        }
    }
}

BOOL CTFIncendiaryC::Deploy()
{
    if(m_iClip >= 0)
	    return DefaultDeploy( "models/v_rpg.mdl", "models/p_rpg.mdl", 5, "rpg", 1 );
    else
	    return DefaultDeploy( "models/v_tfc_rpg.mdl", "models/p_rpg.mdl", 7, "rpg", 1 );
}

int CTFIncendiaryC::AddToPlayer( CBasePlayer *pPlayer )
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

void CTFIncendiaryC::PrimaryAttack()
{
    if(m_pPlayer->ammo_rockets <= 0)
    {
        PlayEmptySound();
        return;
    }

    m_pPlayer->m_iWeaponVolume = 1000;
    m_pPlayer->m_iWeaponFlash = 512;
    PLAYBACK_EVENT_FULL(1, ENT(m_pPlayer->pev), m_usFireIC, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0);
    m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
    m_pPlayer->ammo_rockets--;
    m_flTimeWeaponIdle = 1.2;
    m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.2;
}
