#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

LINK_ENTITY_TO_CLASS( tf_weapon_railgun, CTFRailgun )

void CTFRailgun::Spawn()
{
    Precache();

    //pev->classname = STRING("tf_weapon_railgun");
    SET_MODEL( ENT( pev ), "models/w_gauss.mdl" );
    m_iId = 21;
    m_iDefaultAmmo = 17;
    pev->solid = SOLID_TRIGGER;
}

int CTFRailgun::GetItemInfo( ItemInfo *p )
{
    p->pszAmmo1 = "";
    p->pszName = STRING( pev->classname );
    p->iMaxAmmo1 = 50;
    p->pszAmmo2 = 0;
    p->iMaxAmmo2 = -1;
    p->iSlot = 1;
    p->iPosition = 2;
    p->iFlags = 0;
    p->iMaxClip = -1;
    p->iId = 21;
    p->iWeight = 10;
    return 1;
}

void CTFRailgun::Precache()
{
    PRECACHE_MODEL("models/v_tfc_railgun.mdl");
    PRECACHE_MODEL("models/w_gauss.mdl");
    PRECACHE_MODEL("models/p_9mmhandgun.mdl");
    PRECACHE_MODEL("models/p_9mmhandgun2.mdl");
    PRECACHE_SOUND("weapons/railgun.wav");
    m_usFireRail = PRECACHE_EVENT(1, "events/wpn/tf_rail.sc");
}

BOOL CTFRailgun::Deploy()
{
	return DefaultDeploy( "models/v_tfc_railgun.mdl", "models/p_9mmhandgun.mdl", 2, "onehanded", 1 );
}

void CTFRailgun::PrimaryAttack()
{
    if(m_pPlayer->ammo_bolts <= 0)
    {
        PlayEmptySound();
        m_flNextPrimaryAttack = 0.2;
        return;
    }

    m_pPlayer->m_iWeaponVolume = 600;
    m_pPlayer->m_iWeaponFlash = 256;
    PLAYBACK_EVENT_FULL(1, ENT(m_pPlayer->pev), m_usFireRail, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0);
    m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
    m_pPlayer->SetAnimation(PLAYER_ATTACK1);
    m_pPlayer->ammo_bolts--;
    m_flNextPrimaryAttack = 0.4;
    m_flTimeWeaponIdle = 12.5;
}

void CTFRailgun::WeaponIdle( void )
{
    ResetEmptySound();

    if(m_flTimeWeaponIdle <= 0.0)
    {
        m_flTimeWeaponIdle = 12.5;
        SendWeaponAnim(0, 1);
    }
}
