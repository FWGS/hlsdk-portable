#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

LINK_ENTITY_TO_CLASS( tf_weapon_superng, CTFSuperNailgun )

void CTFSuperNailgun::Precache()
{
    CTFNailgun::Precache();
    m_usFireSuperNailGun = PRECACHE_EVENT(1, "events/wpn/tf_snail.sc");
}

int CTFSuperNailgun::GetItemInfo( ItemInfo *p )
{
    p->pszAmmo1 = "";
    p->pszName = STRING( pev->classname );
    p->iMaxAmmo1 = 150;
    p->pszAmmo2 = 0;
    p->iMaxAmmo2 = -1;
    p->iSlot = 3;
    p->iPosition = 1;
    p->iFlags = 0;
    p->iMaxClip = 0;
    p->iId = 11;
    p->iWeight = 15;
    return 1;
}

BOOL CTFSuperNailgun::Deploy()
{
	return DefaultDeploy( "models/v_tfc_supernailgun.mdl", "models/p_snailgun.mdl", NAILGUN_DEPLOY, "mp5", 1 );
}

void CTFSuperNailgun::PrimaryAttack()
{
    if(m_pPlayer->ammo_bolts <= 0)
    {
        PlayEmptySound();
        return;
    }

    m_pPlayer->m_iWeaponVolume = 1000;
    m_pPlayer->m_iWeaponFlash = 512;
    PLAYBACK_EVENT_FULL(1, ENT(m_pPlayer->pev), m_usFireSuperNailGun, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0);
    m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
    m_pPlayer->ammo_bolts -= 2;
    if(m_pPlayer->ammo_bolts < 0)
        m_pPlayer->ammo_bolts = 0;
    m_flTimeWeaponIdle = 10;
    m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;
}