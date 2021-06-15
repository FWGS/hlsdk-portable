//jackal.cpp
//created with Gott`s weapon creator by Cid Highwind www.warineuropemod.com

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum Jackal_e
{
  Jackal_LONGIDLE = 0,
  Jackal_IDLE1,
  Jackal_IDLE2,
  Jackal_FIRE1,
  Jackal_FIRE2,
  Jackal_RELOAD,
  Jackal_RELOAD2,
  Jackal_DEPLOY,
  Jackal_LAUNCH,
};

LINK_ENTITY_TO_CLASS( weapon_jackal, CJackal );
LINK_ENTITY_TO_CLASS( weapon_357, CJackal );

void CJackal::Spawn( )
{
  pev->classname = MAKE_STRING("weapon_jackal");
  Precache( );
  m_iId = WEAPON_JACKAL;
  SET_MODEL(ENT(pev), "models/w_jackal.mdl");
  m_iDefaultAmmo = 12; 
  FallInit();
}

void CJackal::Precache( void )
{
  PRECACHE_MODEL("models/v_jackal.mdl");
  PRECACHE_MODEL("models/w_jackal.mdl");
  PRECACHE_MODEL("models/p_jackal.mdl");

  m_iShell = PRECACHE_MODEL ("models/shell.mdl");

  PRECACHE_SOUND("weapons/jackal-1.wav");

  m_usFireJackal = PRECACHE_EVENT( 1, "events/jackal.sc" );
}

int CJackal::GetItemInfo(ItemInfo *p)
{
  p->pszName = STRING(pev->classname);
  p->pszAmmo1 = "jackal";
  p->iMaxAmmo1 = JACKAL_MAX_CARRY;
  p->iMaxClip = JACKAL_MAX_CLIP;
  p->iSlot = 1;
  p->iPosition = 3;
  p->iFlags = 0;
  p->iId = m_iId = WEAPON_JACKAL;
  p->iWeight = JACKAL_WEIGHT;

  return 1;
}

BOOL CJackal::Deploy( )
{
  return DefaultDeploy( "models/v_jackal.mdl", "models/p_jackal.mdl", Jackal_DEPLOY, "jackal",  0 );
}

void CJackal::SecondaryAttack( void )
{
  //spaceholder
}

void CJackal::PrimaryAttack( void )
{
  if (!(m_pPlayer->m_afButtonPressed & IN_ATTACK))
  return;
  JackalFire( 0.385,(60/600) , FALSE );
}

void CJackal::JackalFire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
{
  if (m_iClip <= 0)
  {
    if (m_fFireOnEmpty)
    {
      PlayEmptySound();
      m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2f;
    }
    return;
  }

  m_iClip--;
  m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
  int flags;

#if CLIENT_WEAPONS
  flags = FEV_NOTHOST;
#else
  flags = 0;
#endif

  PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), fUseAutoAim ? m_usFireJackal : m_usFireJackal, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, 0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );

  m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
  Vector vecSrc	 = m_pPlayer->GetGunPosition( );
  Vector vecAiming;

  if ( fUseAutoAim )
  {
    vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
  }
  else
  {
    vecAiming = gpGlobals->v_forward;
  }

  m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_JACKAL, 2 );
  m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0f;

  if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
  m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

  m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

  int jackx = RANDOM_LONG(10,40);
  int jacky = RANDOM_LONG(10,40); //Jacky Boy.

  m_pPlayer->pev->punchangle.x -= jackx;
  m_pPlayer->pev->punchangle.y -= jacky;

}

void CJackal::Reload( void )
{
  int iResult;

  if (m_iClip == JACKAL_MAX_CLIP)
  return;

  if (m_iClip == 0)
  {
    iResult = DefaultReload( JACKAL_MAX_CLIP, Jackal_RELOAD2, 2.7, 0  );
  }
  else
  {
    iResult = DefaultReload( JACKAL_MAX_CLIP, Jackal_RELOAD2, 2.7, 0  );
  }
  if (iResult)
  {
    m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
  }
}

void CJackal::WeaponIdle( void )
{
  ResetEmptySound( );
  m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

  if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
    return;

  if (m_iClip != 0)
  {
    int iAnim;
    float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

    if (flRand <= 0.3f)
    {
      iAnim = Jackal_IDLE1;
      m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1 / 16;
    }
    else if (flRand <= 0.6f)
    {
      iAnim = Jackal_LONGIDLE;
      m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1 / 16;
    }
    else
    {
      iAnim = Jackal_IDLE1;
      m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1 / 16;
    }
  SendWeaponAnim( iAnim, 1 );
  }
}

class CJackalAmmoClip : public CBasePlayerAmmo
{
  void Spawn( void )
  {
    Precache( );
    SET_MODEL(ENT(pev), "models/w_jackalclip.mdl");
    CBasePlayerAmmo::Spawn( );
  }
  void Precache( void )
  {
    PRECACHE_MODEL ("models/w_jackalclip.mdl");
    //PRECACHE_SOUND("items/jackalclip.wav");
  }
  BOOL AddAmmo( CBaseEntity *pOther ) 
  {
    int bResult = (pOther->GiveAmmo( AMMO_JACKAL_GIVE, "jackal", JACKAL_MAX_CARRY) != -1);
    if(bResult)
    {
      EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip2.wav", 1, ATTN_NORM);
    }
  return bResult;
  }
};
LINK_ENTITY_TO_CLASS( ammo_jackal, CJackalAmmoClip );
LINK_ENTITY_TO_CLASS( ammo_357, CJackalAmmoClip );
