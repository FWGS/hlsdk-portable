#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "tf_defs.h"

LINK_ENTITY_TO_CLASS( tf_weapon_tranq, CTFTranq )

void CTFTranq::Spawn()
{
    Precache();
    pev->classname = MAKE_STRING("tf_weapon_tranq");
    m_iId = WEAPON_TRANQ;
    m_iDefaultAmmo = 17;
    pev->solid = SOLID_TRIGGER;
}

void CTFTranq::Precache()
{
    PRECACHE_MODEL("models/v_tfc_pistol.mdl");
    PRECACHE_MODEL("models/p_spygun.mdl");
    PRECACHE_MODEL("models/p_9mmhandgun2.mdl");
    PRECACHE_SOUND("weapons/dartgun.wav");
    m_usFireTranquilizer = PRECACHE_EVENT(1, "events/wpn/tf_tranq.sc");
}

int CTFTranq::GetItemInfo( ItemInfo *p )
{
    p->pszAmmo1 = "buckshot";
    p->pszName = STRING( pev->classname );
    p->iMaxAmmo1 = 40;
    p->pszAmmo2 = 0;
    p->iMaxAmmo2 = -1;
    p->iSlot = 1;
    p->iPosition = 4;
    p->iFlags = 0;
    p->iMaxClip = -1;
    p->iId = WEAPON_TRANQ;
    p->iWeight = 10;
    return 1;
}

BOOL CTFTranq::Deploy()
{
    pev->body = 1;
	return DefaultDeploy( "models/v_tfc_pistol.mdl", "models/p_spygun.mdl", TRANQ_DRAW, 0);
}

void CTFTranq::WeaponIdle( void )
{
    ResetEmptySound();

    if(m_flTimeWeaponIdle <= 0.0)
    {
        switch(RANDOM_LONG(0, 2))
        {
            case 1:
            {
                m_flTimeWeaponIdle = 3.75;
                SendWeaponAnim(TRANQ_IDLE1, 1, 1);
            }
            break;
            case 2:
            {
                m_flTimeWeaponIdle = 3.0625;
                SendWeaponAnim(TRANQ_IDLE3, 1, 1);
            }
            break;
            default:
            {
                m_flTimeWeaponIdle = 2.5;
                SendWeaponAnim(TRANQ_IDLE2, 1, 1);
            }
            break;
        }
    }
}

void CTFTranq::PrimaryAttack()
{
    if(m_pPlayer->ammo_buckshot <= 0)
    {
        PlayEmptySound();
        m_flNextPrimaryAttack = 0.2;
        return;
    }

    m_pPlayer->m_iWeaponVolume = 600;
    m_pPlayer->m_iWeaponFlash = 256;
    PLAYBACK_EVENT_FULL(1, ENT(pev), m_usFireTranquilizer, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0);
    m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
    m_pPlayer->SetAnimation(PLAYER_ATTACK1);
    //CreateNail
    m_flTimeWeaponIdle = 12.5;
    m_flNextPrimaryAttack = 1.5;
}