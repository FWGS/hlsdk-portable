#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

LINK_ENTITY_TO_CLASS( laser_spot, CLaserSpot )
LINK_ENTITY_TO_CLASS( tf_weapon_sniperrifle, CTFSniperRifle )

CLaserSpot *CLaserSpot::CreateSpot( void )
{
	CLaserSpot *pSpot = GetClassPtr( (CLaserSpot *)NULL );
	pSpot->Spawn();
	pSpot->pev->classname = MAKE_STRING( "laser_spot" );
	return pSpot;
}

void CLaserSpot::Spawn( void )
{
	Precache();
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->rendermode = kRenderGlow;
	pev->renderfx = kRenderFxNoDissipation;
	pev->renderamt = 255;
	SET_MODEL( ENT( pev ), "sprites/laserdot.spr" );
	UTIL_SetOrigin( pev, pev->origin );
}

void CLaserSpot::Suspend( float flSuspendTime )
{
	pev->effects |= EF_NODRAW;
	SetThink( &CLaserSpot::Revive );
	pev->nextthink = gpGlobals->time + flSuspendTime;
}

void CLaserSpot::Revive()
{
	pev->effects &= ~EF_NODRAW;
	SetThink( NULL );
}

void CLaserSpot::Precache()
{
	PRECACHE_MODEL( "sprites/laserdot.spr" );
}

void CTFSniperRifle::Spawn()
{
    Precache();

    m_iId = 6;
    m_iDefaultAmmo = 5;
    m_fAimedDamage = 0;
    m_fNextAimBonus = -1;
    pev->solid = SOLID_TRIGGER;
}

void CTFSniperRifle::Precache()
{
    PRECACHE_MODEL("models/v_tfc_sniper.mdl");
    PRECACHE_MODEL("models/p_sniper.mdl");
    PRECACHE_MODEL("models/p_sniper2.mdl");
    PRECACHE_SOUND("ambience/rifle1.wav");
    m_usFireSniper = PRECACHE_EVENT(1, "events/wpn/tf_sniper.sc");
    m_usSniperHit = PRECACHE_EVENT(1, "events/wpn/tf_sniperhit.sc");
}

int CTFSniperRifle::GetItemInfo( ItemInfo *p )
{
    p->pszAmmo1 = "buckshot";
    p->pszName = STRING( pev->classname );
    p->iMaxAmmo1 = 75;
    p->pszAmmo2 = 0;
    p->iMaxAmmo2 = -1;
    p->iSlot = 1;
    p->iPosition = 1;
    p->iFlags = 0;
    p->iMaxClip = -1;
    p->iId = 6;
    p->iWeight = 10;
    return 1;
}

void CTFSniperRifle::SecondaryAttack()
{
    if(m_fInZoom)
    {
        m_pPlayer->m_iFOV = 0;
        pev->fov = 0;
        m_fInZoom = 0;
    }
    else
    {
        m_pPlayer->m_iFOV = 20;
        pev->fov = 20;
        m_fInZoom = 1;
    }

    pev->nextthink = gpGlobals->time + 0.1;
    m_flNextSecondaryAttack = 0.3;
}

void CTFSniperRifle::Holster()
{
    m_fInReload = 0;
    if(m_fInZoom)
        SecondaryAttack();

    //m_pPlayer->tfstate &= 2;
    //тф сет спид

    if(m_pSpot)
        m_pSpot->Killed(0, 0);

    m_fAimedDamage = 0;
    m_pPlayer->m_flNextAttack = 0.5;
    SendWeaponAnim(SNIPER_HOLSTER, 1);
}

BOOL CTFSniperRifle::Deploy()
{
    m_pSpot = 0;
    m_flTimeWeaponIdle = 0.5;
    m_fAimedDamage = 0;
    m_iSpotActive = 0;

    //Velaron: "autosniper" draw animation
	return DefaultDeploy( "models/v_tfc_sniper.mdl", "models/p_sniper.mdl", SNIPER_DRAW, "mp5", 1 );
}

int CTFSniperRifle::AddToPlayer( CBasePlayer *pPlayer )
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

void CTFSniperRifle::UpdateSpot()
{
    vec3_t vecSrc, vecEnd;
    TraceResult tr;

    if(!m_iSpotActive)
        return;

    if(!m_pSpot)
    {
        m_pSpot = CLaserSpot::CreateSpot();
        //Velaron: effect 256
        //m_pSpot->pev->effects |= 256;
        m_pSpot->pev->owner = ENT(m_pPlayer->pev);
    }

    UTIL_MakeVectors(m_pPlayer->pev->v_angle);
    vecSrc = m_pPlayer->GetGunPosition();
    vecEnd.x = gpGlobals->v_forward.x * 8192.0 + vecSrc.x;
    vecEnd.y = gpGlobals->v_forward.y * 8192.0 + vecSrc.y;
    vecEnd.z = gpGlobals->v_forward.z * 8192.0 + vecSrc.z;
    UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);
    UTIL_SetOrigin(m_pSpot->pev, tr.vecEndPos);
    m_pSpot->pev->renderamt = 0.25 * m_fAimedDamage + 150.0;
}

void CTFSniperRifle::WeaponIdle( void )
{
    if(m_iSpotActive)
        UpdateSpot();

    if(m_flTimeWeaponIdle < 0.0)
    {
        m_flTimeWeaponIdle = 12.5;
        SendWeaponAnim(SNIPER_IDLE, 1);
    }
}

//Velaron: finish this nightmare
void CTFSniperRifle::PrimaryAttack()
{
    vec3_t anglesAim;
    vec3_t vecAim;
    vec3_t vecSrc, vecEnd;
    TraceResult tr;

    if(m_pPlayer->ammo_buckshot <= 0)
    {
        PlayEmptySound();
        return;
    }

    m_pPlayer->m_iWeaponVolume = 200;
    m_pPlayer->m_iWeaponFlash = 512;
    PLAYBACK_EVENT_FULL(1, ENT(m_pPlayer->pev), m_usFireSniper, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, m_fAimedDamage, 0, 0, 0);
    m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
    m_flTimeWeaponIdle = 0.5;
    anglesAim.x = m_pPlayer->pev->v_angle.x + m_pPlayer->pev->punchangle.x;
    anglesAim.y = m_pPlayer->pev->v_angle.y + m_pPlayer->pev->punchangle.y;
    anglesAim.z = m_pPlayer->pev->v_angle.z + m_pPlayer->pev->punchangle.z;
    UTIL_MakeVectors(anglesAim);
    vecAim = m_pPlayer->GetGunPosition();
    UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

    PLAYBACK_EVENT_FULL(1, ENT(m_pPlayer->pev), m_usSniperHit, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0);

    m_pPlayer->ammo_buckshot--;
}

void CTFSniperRifle::ItemPostFrame()
{
/*
    if(!m_pPlayer->pev->button & 8 || m_flNextSecondaryAttack > 0.0)
    {
        if(m_pPlayer->m_afButtonReleased & 1 && m_flNextPrimaryAttack <= 0.0)
        {
            if(tfstate & 8)
            {}
        }
    }
*/
}