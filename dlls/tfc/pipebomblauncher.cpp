#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

LINK_ENTITY_TO_CLASS( tf_weapon_pl, CTFPipebombLauncher )

void CTFPipebombLauncher::Spawn()
{
    Precache();

    SET_MODEL( ENT( pev ), "models/w_gauss.mdl" );
    m_iId = 22;
    m_iDefaultAmmo = 50;
    m_fReloadTime = 0.666667;
    pev->solid = SOLID_TRIGGER;
    m_usFireGL = PRECACHE_EVENT(1, "events/wpn/tf_gl.sc");
}

int CTFPipebombLauncher::GetItemInfo( ItemInfo *p )
{
    p->pszAmmo1 = "rockets";
    p->pszName = STRING( pev->classname );
    p->iMaxAmmo1 = 50;
    p->pszAmmo2 = 0;
    p->iMaxAmmo2 = -1;
    p->iSlot = 4;
    p->iPosition = 4;
    p->iFlags = 0;
    p->iMaxClip = 6;
    p->iId = 22;
    p->iWeight = 15;
    return 1;
}

void CTFPipebombLauncher::PrimaryAttack()
{
    if(m_pPlayer->ammo_rockets <= 0)
    {
        m_flNextPrimaryAttack = 1;
        SendWeaponAnim(0, 1);
        PlayEmptySound();
        m_fInSpecialReload = 0;
        //tfstate
        return;
    }

    m_pPlayer->m_iWeaponVolume = 1000;
    m_pPlayer->m_iWeaponFlash = 512;
    PLAYBACK_EVENT_FULL(1, ENT(m_pPlayer->pev), m_usFireGL, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0);
    m_fInSpecialReload = 0;
    //tfstate
    m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
    m_iClip--;
    m_flTimeWeaponIdle = 0.6;
    m_flNextPrimaryAttack = 0.6;
    m_flNextSecondaryAttack = 0.6;
}
