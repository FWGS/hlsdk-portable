#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

LINK_ENTITY_TO_CLASS( tf_weapon_ng, CTFNailgun )

void CTFNailgun::Spawn()
{
    Precache();

    m_iId = 4;
    m_iDefaultAmmo = 50;
    pev->solid = SOLID_TRIGGER;
}

void CTFNailgun::Precache( void )
{
    PRECACHE_MODEL("models/v_tfc_nailgun.mdl");
    PRECACHE_MODEL("models/v_tfc_supernailgun.mdl");
    PRECACHE_MODEL("models/p_nailgun.mdl");
    PRECACHE_MODEL("models/p_nailgun2.mdl");    
    PRECACHE_MODEL("models/p_snailgun.mdl");    
    PRECACHE_MODEL("models/p_snailgun2.mdl");
    PRECACHE_SOUND("items/9mmclip1.wav");
    PRECACHE_SOUND("items/clipinsert1.wav");
    PRECACHE_SOUND("items/cliprelease1.wav");
    PRECACHE_SOUND("weapons/airgun_1.wav");
    PRECACHE_SOUND("weapons/spike2.wav");
    PRECACHE_SOUND("weapons/357_cock1.wav");
    m_usFireNailGun = PRECACHE_EVENT(1, "events/wpn/tf_nail.sc");
}

int CTFNailgun::GetItemInfo( ItemInfo *p )
{
    p->pszAmmo1 = "";
    p->pszName = STRING( pev->classname );
    p->iMaxAmmo1 = 200;
    p->pszAmmo2 = 0;
    p->iMaxAmmo2 = -1;
    p->iSlot = 3;
    p->iPosition = 4;
    p->iFlags = 0;
    p->iMaxClip = 0;
    p->iId = 10;
    p->iWeight = 15;
    return 1;
}

BOOL CTFNailgun::Deploy()
{
	return DefaultDeploy( "models/v_tfc_nailgun.mdl", "models/p_nailgun.mdl", NG_DEPLOY, "mp5", 1 );
}

void CTFNailgun::WeaponIdle( void )
{
    if(m_flTimeWeaponIdle < 0.0)
    {
        m_flTimeWeaponIdle = 15;
        SendWeaponAnim(NG_IDLE1, 1);
    }
}

int CTFNailgun::AddToPlayer( CBasePlayer *pPlayer )
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

void CTFNailgun::PrimaryAttack()
{
    if(m_pPlayer->ammo_bolts <= 0)
    {
        PlayEmptySound();
        return;
    }

    m_pPlayer->m_iWeaponVolume = 1000;
    m_pPlayer->m_iWeaponFlash = 512;
    PLAYBACK_EVENT_FULL(1, ENT(m_pPlayer->pev), m_usFireNailGun, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0);
    m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
    m_pPlayer->ammo_bolts--;
    m_flTimeWeaponIdle = 10;
    m_flNextPrimaryAttack = 0.1;
}

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
	return DefaultDeploy( "models/v_tfc_supernailgun.mdl", "models/p_snailgun.mdl", NG_DEPLOY, "mp5", 1 );
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
    m_flNextPrimaryAttack = 0.1;
}
