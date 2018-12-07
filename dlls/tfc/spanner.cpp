#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

LINK_ENTITY_TO_CLASS( tf_weapon_spanner, CTFSpanner )

void CTFSpanner::Spawn()
{
    Precache();

    m_iId = 4;
    SET_MODEL(ENT(pev), "models/w_egon.mdl");
    m_iDefaultAmmo = 50;
    pev->solid = SOLID_TRIGGER;
    classid = 1;
}

void CTFSpanner::Precache()
{
    PRECACHE_MODEL("models/v_tfc_spanner.mdl");
    PRECACHE_MODEL("models/p_spanner.mdl");
    PRECACHE_MODEL("models/p_spanner2.mdl");
    m_usAxe = PRECACHE_EVENT(1, "events/wpn/tf_axe.sc");
    m_usAxeDecal = PRECACHE_EVENT(1, "events/wpn/tf_axedecal.sc");
}

void CTFSpanner::Holster()
{
    SendWeaponAnim(5, 1);
    m_pPlayer->m_flNextAttack = 0.5;
}

int CTFSpanner::GetItemInfo( ItemInfo *p )
{
    p->pszAmmo1 = "uranium";
    p->pszName = STRING( pev->classname );
    p->iMaxAmmo1 = -1;
    p->pszAmmo2 = 0;
    p->iMaxAmmo2 = -1;
    p->iSlot = 0;
    p->iPosition = 2;
    p->iFlags = 0;
    p->iMaxClip = -1;
    p->iId = 4;
    p->iWeight = 0;
    return 1;
}

BOOL CTFSpanner::Deploy()
{
	return DefaultDeploy( "models/v_tfc_spanner.mdl", "models/p_spanner.mdl", 4, "crowbar", 1 );
}

void CTFSpanner::WeaponIdle( void )
{
    ResetEmptySound();

    if(m_flTimeWeaponIdle <= 0.0)
    {
        m_flTimeWeaponIdle = 12.5;
        SendWeaponAnim(0, 1);
    }
}