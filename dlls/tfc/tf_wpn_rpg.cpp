#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

LINK_ENTITY_TO_CLASS( tf_weapon_rpg, CTFRpg )

void CTFRpg::Spawn()
{
    Precache();

    m_iId = 14;
    SET_MODEL(ENT(pev), "models/w_rpg.mdl");
    m_iDefaultAmmo = 50;
    m_fReloadTime = 1.25;
    pev->solid = SOLID_TRIGGER;
}

int CTFRpg::GetItemInfo( ItemInfo *p )
{
    p->pszAmmo1 = "rockets";
    p->pszName = STRING( pev->classname );
    p->iMaxAmmo1 = 50;
    p->pszAmmo2 = 0;
    p->iMaxAmmo2 = -1;
    p->iSlot = 4;
    p->iPosition = 1;
    p->iFlags = 0;
    p->iMaxClip = 4;
    p->iId = 14;
    p->iWeight = 20;
    return 1;
}

void CTFRpg::Holster()
{
    //тф стейт чот там
    m_fInSpecialReload = 0;
    m_pPlayer->m_flNextAttack = 0.5;
}

void CTFRpg::Precache()
{
    PRECACHE_MODEL("models/v_tfc_rpg.mdl");
    PRECACHE_MODEL("models/p_srpg.mdl");
    PRECACHE_MODEL("models/p_shotgun2.mdl");
    PRECACHE_MODEL("models/p_rpg2.mdl");
    PRECACHE_SOUND("weapons/9mmclip1.wav");
    UTIL_PrecacheOther("tf_rpg_rocket");
    PRECACHE_SOUND("weapons/rocketfire1.wav");
    PRECACHE_SOUND("weapons/glauncher.wav");
    m_usFireRPG = PRECACHE_EVENT(1, "events/wpn/tf_rpg.sc");
}

void CTFRpg::Reload( void )
{
    if ( m_pPlayer->ammo_rockets == 0 || m_iClip == 4 || m_flNextReload > 0.0 || m_flNextPrimaryAttack > 0.0)
        return;

    if ( m_fInSpecialReload )
    {
        if ( m_fInSpecialReload == 1 )
        {
            if ( m_flTimeWeaponIdle <= gpGlobals->time )
            {
                m_fInSpecialReload = 2;
                SendWeaponAnim(TFCRPG_RELCYCLE, 1);
                m_flNextReload = UTIL_WeaponTimeBase() + m_fReloadTime;
                m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + m_fReloadTime;
            }
        }
        else
        {
            m_iClip += 1;
            m_pPlayer->ammo_rockets -= 1;
            m_fInSpecialReload = 1;
            //m_pPlayer->tfstate &= 2;
        }
    }
    else
    {
        SendWeaponAnim(TFCRPG_RELSTART, 1);
        //m_pPlayer->tfstate |= 2;
        m_fInSpecialReload = 1;
        m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
        m_flTimeWeaponIdle = 0.1;
        m_flNextSecondaryAttack = 0.1;
        m_flNextPrimaryAttack = 0.0;
    }
}

void CTFRpg::WeaponIdle( void )
{
    ResetEmptySound();
    if (m_flTimeWeaponIdle > 0.0)
        return;
    
    if(m_iClip > 0)
    {
        if (!m_fInSpecialReload)
        {
            if (m_pPlayer->ammo_rockets > 0)
            {
                if(m_iClip == 0)
                    SendWeaponAnim(TFCRPG_FIDGET2, 1);
                else
                    SendWeaponAnim(TFCRPG_IDLE, 1);
                m_flTimeWeaponIdle = 3;
            }
            else
                m_flTimeWeaponIdle = 1;
            return;
        }
        if (m_iClip == 4)
        {
            SendWeaponAnim(TFCRPG_RELEND, 1);
            m_fInSpecialReload = 0;
            m_flTimeWeaponIdle = 1.5;
            return;
        }
    }
    else if (!m_fInSpecialReload)
    {
        if (m_pPlayer->ammo_rockets > 0)
        {
            Reload();
            return; 
        }

        if (m_pPlayer->ammo_rockets > 0)
        {
            if(m_iClip == 0)
                SendWeaponAnim(TFCRPG_FIDGET2, 1);
            else
                SendWeaponAnim(TFCRPG_IDLE, 1);
            m_flTimeWeaponIdle = 3;
        }
        else
            m_flTimeWeaponIdle = 1;
        return;
    }

    if (m_pPlayer->ammo_rockets > 0)
    {
        Reload();
        return; 
    }

    SendWeaponAnim(TFCRPG_RELEND, 1);
    m_fInSpecialReload = 0;
    m_flTimeWeaponIdle = 1.5;
    return;
}

BOOL CTFRpg::Deploy()
{
    if(m_iClip >= 0)
	    return DefaultDeploy( "models/v_tfc_rpg.mdl", "models/p_srpg.mdl", TFCRPG_DRAW1, "rpg", 1 );
    else
	    return DefaultDeploy( "models/v_tfc_rpg.mdl", "models/p_srpg.mdl", TFCRPG_DRAW2, "rpg", 1 );
}

int CTFRpg::AddToPlayer( CBasePlayer *pPlayer )
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

void CTFRpg::PrimaryAttack()
{
    if(m_iClip <= 0)
        return;

    m_pPlayer->m_iWeaponVolume = 1000;
    m_pPlayer->m_iWeaponFlash = 512;
    PLAYBACK_EVENT_FULL(1, ENT(m_pPlayer->pev), m_usFireRPG, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0);
    m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
    m_fInSpecialReload = 0;
    //tfstatatata
    m_iClip--;
    m_flTimeWeaponIdle = 0.8;
    m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.8;
}