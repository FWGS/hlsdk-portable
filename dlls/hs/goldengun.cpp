//goldengun.cpp
//created with Gott`s weapon creator by Cid Highwind www.warineuropemod.com

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum GOLDENGUN_e
{
  GOLDENGUN_LONGIDLE = 0,
  GOLDENGUN_IDLE1,
  GOLDENGUN_IDLE2,
  GOLDENGUN_FIRE1,
  GOLDENGUN_FIRE2,
  GOLDENGUN_RELOAD,
  GOLDENGUN_RELOAD2,
  GOLDENGUN_DEPLOY,
  GOLDENGUN_LAUNCH,
};

LINK_ENTITY_TO_CLASS( weapon_goldengun, CGOLDENGUN );

void CGOLDENGUN::Spawn( )
{
  pev->classname = MAKE_STRING("weapon_goldengun");
  Precache( );
  m_iId = WEAPON_GOLDENGUN;
  SET_MODEL(ENT(pev), "models/w_goldengun.mdl");
  m_iDefaultAmmo = GOLDENGUN_MAX_DEFAULT_GIVE; 
  FallInit();
}

void CGOLDENGUN::Precache( void )
{
  PRECACHE_MODEL("models/v_goldengun.mdl");
  PRECACHE_MODEL("models/w_goldengun.mdl");
  PRECACHE_MODEL("models/p_goldengun.mdl");

  m_iShell = PRECACHE_MODEL ("models/shell.mdl");

  //PRECACHE_SOUND("items/goldengunclip.wav");
  PRECACHE_SOUND("weapons/ggun_fire.wav");

  m_usFireGOLDENGUN = PRECACHE_EVENT( 1, "events/goldengun.sc" );
}

int CGOLDENGUN::GetItemInfo(ItemInfo *p)
{
  p->pszName = STRING(pev->classname);
  p->pszAmmo1 = "gold";
  p->iMaxAmmo1 = GOLDENGUN_MAX_CARRY;
//  p->pszAmmo2 = "Nothing";
//  p->iMaxAmmo2 = 0;
  p->iMaxClip = GOLDENGUN_MAX_CLIP;
  p->iSlot = 1;
  p->iPosition = 1;
  p->iFlags = 0;
  p->iId = m_iId = WEAPON_GOLDENGUN;
  p->iWeight = GOLDENGUN_WEIGHT;

  return 1;
}

BOOL CGOLDENGUN::Deploy( )
{
  return DefaultDeploy( "models/v_goldengun.mdl", "models/p_goldengun.mdl", GOLDENGUN_DEPLOY, "goldengun",  0 );
}

void CGOLDENGUN::SecondaryAttack( void )
{
  //spaceholder
}

void CGOLDENGUN::PrimaryAttack( void )
{
  if (!(m_pPlayer->m_afButtonPressed & IN_ATTACK))
  return;
  GOLDENGUNFire( 0.085,(60/600) , FALSE );
}

void CGOLDENGUN::GOLDENGUNFire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
{
  if (m_iClip <= 0)
  {
    if (m_fFireOnEmpty)
    {
      PlayEmptySound();
      m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;
    }
    return;
  }

  m_iClip--;
  m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
  int flags;

#if defined( CLIENT_WEAPONS )
  flags = FEV_NOTHOST;
#else
  flags = 0;
#endif

  PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), fUseAutoAim ? m_usFireGOLDENGUN : m_usFireGOLDENGUN, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );

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

  m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_GOLDENGUN, 2 );
  m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

  if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
  m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

  m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

  if(m_pPlayer->pev->flags & FL_DUCKING)
  {
    switch (RANDOM_LONG(0,1))
    {
 	 case 0: m_pPlayer->pev->punchangle.y -= 3; break;
 	 case 1: m_pPlayer->pev->punchangle.y += 3; break;
    }
  }
  else if (m_pPlayer->pev->velocity.Length() > .01)
  {
    switch (RANDOM_LONG(0,1))
    {
 	 case 0: m_pPlayer->pev->punchangle.y -= 3; break;
 	 case 1: m_pPlayer->pev->punchangle.y += 3; break;
    }
  }
else
  {
    switch (RANDOM_LONG(0,1))
    {
 	 case 0: m_pPlayer->pev->punchangle.y -= 3; break;
 	 case 1: m_pPlayer->pev->punchangle.y += 3; break;
    }
  }
}

void CGOLDENGUN::Reload( void )
{
  int iResult;

  if (m_iClip == GOLDENGUN_MAX_CLIP)
  return;

  if (m_iClip == 0)
  {
    iResult = DefaultReload( GOLDENGUN_MAX_CLIP, GOLDENGUN_RELOAD2, 2.7, 0  );
  }
  else
  {
    iResult = DefaultReload( GOLDENGUN_MAX_CLIP, GOLDENGUN_RELOAD2, 2.7, 0  );
  }
  if (iResult)
  {
    m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
  }
}

void CGOLDENGUN::WeaponIdle( void )
{
  ResetEmptySound( );
  m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

  if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
    return;

  if (m_iClip != 0)
  {
    int iAnim;
    float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

    if (flRand <= 0.3 + 0 * 0.75)
    {
      iAnim = GOLDENGUN_IDLE1;
      m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1 / 16;
    }
    else if (flRand <= 0.6 + 0 * 0.875)
    {
      iAnim = GOLDENGUN_LONGIDLE;
      m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1 / 16;
    }
    else
    {
      iAnim = GOLDENGUN_IDLE1;
      m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1 / 16;
    }
  SendWeaponAnim( iAnim, 1 );
  }
}

class CGOLDENGUNAmmoClip : public CBasePlayerAmmo
{
  void Spawn( void )
  {
    Precache( );
    SET_MODEL(ENT(pev), "models/w_goldengunclip.mdl");
    CBasePlayerAmmo::Spawn( );
  }
  void Precache( void )
  {
    PRECACHE_MODEL ("models/w_goldengunclip.mdl");
    //PRECACHE_SOUND("items/goldengunclip.wav");
  }
  BOOL AddAmmo( CBaseEntity *pOther ) 
  {
    int bResult = (pOther->GiveAmmo( AMMO_GOLDENGUNCLIP_GIVE, "gold", GOLDENGUN_MAX_CARRY) != -1);
    if(bResult)
    {
      EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip2.wav", 1, ATTN_NORM);
    }
  return bResult;
  }
};
LINK_ENTITY_TO_CLASS( ammo_goldengun, CGOLDENGUNAmmoClip );
