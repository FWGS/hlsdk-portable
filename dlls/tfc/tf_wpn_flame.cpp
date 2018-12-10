#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "tf_defs.h"

LINK_ENTITY_TO_CLASS( tf_weapon_flamethrower, CTFFlamethrower )

void CTFFlamethrower::Spawn()
{
    Precache();
    m_iId = WEAPON_FLAMETHROWER;
    SET_MODEL(ENT(pev), "models/w_egon.mdl");
    m_iDefaultAmmo = 50;
    pev->solid = SOLID_TRIGGER;
}

int CTFFlamethrower::GetItemInfo( ItemInfo *p )
{
    p->pszAmmo1 = "uranium";
    p->pszName = STRING(pev->classname);
    p->iMaxAmmo1 = 200;
    p->pszAmmo2 = 0;
    p->iMaxAmmo2 = -1;
    p->iSlot = 3;
    p->iPosition = 2;
    p->iFlags = 0;
    p->iMaxClip = -1;
    p->iId = WEAPON_FLAMETHROWER;
    p->iWeight = 20;
    return 1;
}

void CTFFlamethrower::Holster()
{
    m_fInReload = 0;
    SendWeaponAnim(10, 1);
    m_pPlayer->m_flNextAttack = 0.5;
}

void CTFFlamethrower::Precache()
{
    PRECACHE_MODEL("models/w_egon.mdl");
    PRECACHE_MODEL("models/v_flame.mdl");
    PRECACHE_MODEL("models/p_egon.mdl");
    PRECACHE_MODEL("models/p_egon2.mdl");
    PRECACHE_MODEL("models/w_9mmclip.mdl");
    PRECACHE_SOUND("weapons/9mmclip1.wav");
    UTIL_PrecacheOther("tf_flamethrower_burst");
    PRECACHE_SOUND("weapons/flmfire2.wav");
    m_usFireFlame = PRECACHE_EVENT(1, "events/wpn/tf_flame.sc");
}

void CTFFlamethrower::WeaponIdle( void )
{
    ResetEmptySound();

    if(m_flTimeWeaponIdle <= 0.0)
    {
        if(UTIL_SharedRandomLong(m_pPlayer->random_seed, 0, 4))
        {
            m_flTimeWeaponIdle = 12.5;
            SendWeaponAnim(FT_IDLE, 1);
        }
        else
        {
            m_flTimeWeaponIdle = 3;
            SendWeaponAnim(FT_FIDGET, 1);
        }
    }
}

BOOL CTFFlamethrower::Deploy()
{
	return DefaultDeploy( "models/v_flame.mdl", "models/p_egon.mdl", 9, "egon", 1 );
}

int CTFFlamethrower::AddToPlayer( CBasePlayer *pPlayer )
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

void CTFFlamethrower::PrimaryAttack()
{
    BOOL bUnderwater;

    if(m_pPlayer->ammo_uranium <= 0)
    {
        PlayEmptySound();
        return;
    }

    m_pPlayer->m_iWeaponVolume = 1000;
    m_pPlayer->m_iWeaponFlash = 512;
    bUnderwater = m_pPlayer->pev->waterlevel > 2;
    PLAYBACK_EVENT_FULL(1, ENT(m_pPlayer->pev), m_usFireFlame, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, bUnderwater, 0);
    m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
    if(bUnderwater)
    {
        m_flTimeWeaponIdle = 1;
        m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1;
    }
    else
    {
        m_flTimeWeaponIdle = 0.15;
        m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
    }
    m_pPlayer->ammo_uranium--;
}