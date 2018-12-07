#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

LINK_ENTITY_TO_CLASS( tf_weapon_axe, CTFAxe )

void CTFAxe::Spawn()
{
    Precache();

    m_iId = 5;
    m_iClip = -1;
    pev->solid = SOLID_TRIGGER;
}

int CTFAxe::GetItemInfo( ItemInfo *p )
{
    p->pszAmmo1 = 0;
    p->pszName = STRING( pev->classname );
    p->iMaxAmmo1 = -1;
    p->pszAmmo2 = 0;
    p->iMaxAmmo2 = -1;
    p->iSlot = 0;
    p->iPosition = 3;
    p->iFlags = 0;
    p->iMaxClip = -1;
    p->iId = 5;
    p->iWeight = 0;
    return 1;
}

void CTFAxe::Holster()
{
    SendWeaponAnim(AXE_HOLSTER, 1);
    m_pPlayer->m_flNextAttack = 0.5;
}

void CTFAxe::Precache()
{
    PRECACHE_MODEL("models/v_umbrella.mdl");
    PRECACHE_MODEL("models/p_umbrella.mdl");
    PRECACHE_MODEL("models/v_tfc_crowbar.mdl");
    PRECACHE_MODEL("models/p_crowbar.mdl");
    PRECACHE_MODEL("models/p_crowbar2.mdl");
    PRECACHE_SOUND("weapons/cbar_hit1.wav");
    PRECACHE_SOUND("weapons/cbar_hit2.wav");
    PRECACHE_SOUND("weapons/cbar_hitbod1.wav");
    PRECACHE_SOUND("weapons/cbar_hitbod2.wav");
    PRECACHE_SOUND("weapons/cbar_hitbod3.wav");
    PRECACHE_SOUND("weapons/cbar_miss1.wav");
    classid = 3;
    m_usAxe = PRECACHE_EVENT(1, "events/wpn/tf_axe.sc");
    m_usAxeDecal = PRECACHE_EVENT(1, "events/wpn/tf_axedecal.sc");
}

BOOL CTFAxe::Deploy()
{
	if(m_pPlayer->pev->playerclass == 11)
	    return DefaultDeploy( "models/v_umbrella.mdl", "models/p_umbrella.mdl", AXE_DRAW, "crowbar", 1 );
    else
	    return DefaultDeploy( "models/v_tfc_crowbar.mdl", "models/p_crowbar.mdl", AXE_DRAW, "crowbar", 1 );
}
//Velaron: finish this?
void CTFAxe::PrimaryAttack()
{
    vec3_t vecSrc, vecEnd;
    TraceResult tr;
    
    m_bHullHit = 0;
    UTIL_MakeVectors(m_pPlayer->pev->v_angle);
    vecSrc = m_pPlayer->GetGunPosition();
    vecEnd.x = gpGlobals->v_forward.x * 32.0 + vecSrc.x;
    vecEnd.y = gpGlobals->v_forward.y * 32.0 + vecSrc.y;
    vecEnd.z = gpGlobals->v_forward.z * 32.0 + vecSrc.z;
    PLAYBACK_EVENT_FULL(1, ENT(m_pPlayer->pev), m_usAxe, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, classid, 0, 0, 0);
    if ( tr.flFraction >= 1.0 )
    {
        m_bHullHit = 1;
        //m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.4;
        m_flNextPrimaryAttack = 1.0;
        m_flTimeWeaponIdle = 5;
        m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
        return;
    }
    m_flNextPrimaryAttack = 1.0;
    m_flTimeWeaponIdle = 5;
    m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
}

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
