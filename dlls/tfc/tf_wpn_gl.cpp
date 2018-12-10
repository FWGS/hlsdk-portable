#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "tf_defs.h"

enum tf_gl_e
{
    GL_IDLE = 0,
    PL_IDLE,
    GL_FIRE,
    PL_FIRE,
    GL_RELOAD1,
    GL_RELOAD2,
    PL_RELOAD1,
    PL_RELOAD2,
    GL_DRAW,
    PL_DRAW,
    GL_HOLSTER,
    PL_HOLSTER
};

LINK_ENTITY_TO_CLASS( tf_weapon_gl, CTFGrenadeLauncher )

void CTFGrenadeLauncher::Spawn()
{
    Precache();
    SET_MODEL( ENT( pev ), "models/w_gauss.mdl" );
    m_iId = 12;
    m_iDefaultAmmo = 50;
    m_fReloadTime = 0.666667;
    pev->solid = SOLID_TRIGGER;
    m_iAnim_Deploy = GL_DRAW;
    m_iAnim_Holster = GL_HOLSTER;
    m_iAnim_Idle = GL_IDLE;
    m_iAnim_ReloadDown = GL_RELOAD1;
    m_iAnim_ReloadUp = GL_RELOAD2;
}

void CTFGrenadeLauncher::Precache()
{
    PRECACHE_MODEL("models/v_tfgl.mdl");
    PRECACHE_MODEL("models/p_glauncher.mdl");
    PRECACHE_MODEL("models/p_p_glauncher2rpg.mdl");
    PRECACHE_MODEL("models/pipebomb.mdl");
    PRECACHE_SOUND("weapons/9mmclip1.wav");
    PRECACHE_SOUND("items/clipinsert1.wav");
    PRECACHE_SOUND("items/cliprelease1.wav");
    PRECACHE_SOUND("weapons/hks1.wav");
    PRECACHE_SOUND("weapons/hks2.wav");
    PRECACHE_SOUND("weapons/hks3.wav");
    PRECACHE_SOUND("weapons/glauncher.wav");
    PRECACHE_SOUND("weapons/glauncher2.wav");
    PRECACHE_SOUND("weapons/357_cock1.wav");
    m_usFireGL = PRECACHE_EVENT(1, "events/wpn/tf_gl.sc");
}

int CTFGrenadeLauncher::GetItemInfo( ItemInfo *p )
{
    p->pszAmmo1 = "rockets";
    p->pszName = STRING( pev->classname );
    p->iMaxAmmo1 = 50;
    p->pszAmmo2 = 0;
    p->iMaxAmmo2 = -1;
    p->iSlot = 3;
    p->iPosition = 3;
    p->iFlags = 0;
    p->iMaxClip = 6;
    p->iId = 12;
    p->iWeight = 15;
    return 1;
}

void CTFGrenadeLauncher::Holster()
{
    //m_pPlayer->m_iGLClip = m_iClip;
    //tfstate?
    m_fInSpecialReload = 0;
    SendWeaponAnim(m_iAnim_Holster, 1);
    m_flTimeWeaponIdle = 1000;
    m_pPlayer->m_flNextAttack = 0.5;
}

BOOL CTFGrenadeLauncher::Deploy()
{
	return DefaultDeploy( "models/v_tfgl.mdl", "models/p_glauncher.mdl", m_iAnim_Deploy, "mp5", 1 );
}

void CTFGrenadeLauncher::Reload()
{
    if ( m_pPlayer->ammo_rockets > 0 )
    {
        if ( m_iClip != 6  && m_flNextPrimaryAttack <= 0.0 ) //&& m_flNextReload <= 0.0
        {
            if (m_fInSpecialReload)
            {
                if ( m_fInSpecialReload == 1 )
                {
                    if ( m_flTimeWeaponIdle <= 0.0 )
                        {
                            m_fInSpecialReload = 2;
                            m_flNextReload = m_fReloadTime;
                            m_flTimeWeaponIdle = m_fReloadTime;
                        }
                }
                else
                {
                    m_iClip++;
                    m_pPlayer->ammo_rockets--;
                    m_fInSpecialReload = 1;
                    //m_pPlayer->tfstate &= 2;
                }
            }
            else
            {
                SendWeaponAnim(m_iAnim_ReloadDown, 1);
                //m_pPlayer->tfstate |= 2;
                m_fInSpecialReload = 1;
                m_pPlayer->m_flNextAttack = 0.1;
                m_flTimeWeaponIdle = 0.1;
                m_flNextSecondaryAttack = 0.1;
                m_flNextPrimaryAttack = 0.0;
            }
        }
    }
}

int CTFGrenadeLauncher::AddToPlayer( CBasePlayer *pPlayer )
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

void CTFGrenadeLauncher::WeaponIdle( void )
{
    ResetEmptySound();

    if(m_flTimeWeaponIdle > 0.0)
        return;

    if(m_iClip > 0)
    {
        if(m_fInSpecialReload)
        {
            if(m_iClip == 6)
            {
                SendWeaponAnim(m_iAnim_ReloadUp, 1);
                m_fInSpecialReload = 0;
                m_flTimeWeaponIdle = 1.5;
                return;
            }
            if(m_pPlayer->ammo_rockets > 0)
            {
                Reload();
                return;
            }
        }
    }
    m_flTimeWeaponIdle = 3;
    SendWeaponAnim(m_iAnim_Idle, 1);
}

void CTFGrenadeLauncher::PrimaryAttack()
{
    if(m_iClip <= 0)
    {
        m_flNextPrimaryAttack = 1;
        SendWeaponAnim(m_iAnim_Idle, 1);
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

LINK_ENTITY_TO_CLASS( tf_weapon_pl, CTFPipebombLauncher )

void CTFPipebombLauncher::Spawn()
{
    Precache();
    SET_MODEL( ENT( pev ), "models/w_gauss.mdl" );
    m_iId = WEAPON_PIPEBOMB_LAUNCHER;
    m_iDefaultAmmo = 50;
    m_fReloadTime = 0.666667;
    pev->solid = SOLID_TRIGGER;
    m_usFireGL = PRECACHE_EVENT(1, "events/wpn/tf_pipel.sc");
    m_iAnim_Holster = PL_HOLSTER;
    m_iAnim_Idle = PL_IDLE;
    m_iAnim_ReloadDown = PL_RELOAD1;
    m_iAnim_ReloadUp = PL_RELOAD2;
    m_iAnim_Deploy = PL_DRAW;
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
    p->iId = WEAPON_PIPEBOMB_LAUNCHER;
    p->iWeight = 15;
    return 1;
}

void CTFPipebombLauncher::PrimaryAttack()
{
    if(m_iClip <= 0)
    {
        m_flNextPrimaryAttack = 1;
        SendWeaponAnim(m_iAnim_Idle, 1);
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
