//zapper.cpp

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum ZAPPER_e
{
	ZAPPER_IDLE1 = 0,
	ZAPPER_IDLE2,
	ZAPPER_IDLE3,
	ZAPPER_SHOOT,
	ZAPPER_SHOOT_EMPTY,
	ZAPPER_RELOAD,
	ZAPPER_RELOAD_NOT_EMPTY,
	ZAPPER_DRAW,
	ZAPPER_HOLSTER,
	ZAPPER_ADD_SILENCER
};

LINK_ENTITY_TO_CLASS( weapon_zapper, CZAPPER );

void CZAPPER::Spawn( )
{
  pev->classname = MAKE_STRING("weapon_zapper");
  Precache( );
  m_iId = WEAPON_ZAPPER;
  SET_MODEL(ENT(pev), "models/w_zapper.mdl");
  m_iDefaultAmmo = ZAPPER_MAX_DEFAULT_GIVE; 
  FallInit();
}

void CZAPPER::Precache( void )
{
  PRECACHE_MODEL("models/v_zapper.mdl");
  PRECACHE_MODEL("models/w_zapper.mdl");
  PRECACHE_MODEL("models/p_zapper.mdl");

  PRECACHE_SOUND("weapons/zapper.wav");

  m_usFireZAPPER = PRECACHE_EVENT( 1, "events/zapper.sc" );
}

int CZAPPER::GetItemInfo(ItemInfo *p)
{
  p->pszName = STRING(pev->classname);
  p->pszAmmo1 = "zap";
  p->iMaxAmmo1 = ZAPPER_MAX_CARRY;
  p->iMaxClip = ZAPPER_MAX_CLIP;
  p->iSlot = 1;
  p->iPosition = 0;
  p->iFlags = 0;
  p->iId = m_iId = WEAPON_ZAPPER;
  p->iWeight = ZAPPER_WEIGHT;

  return 1;
}

BOOL CZAPPER::Deploy( )
{
  return DefaultDeploy( "models/v_zapper.mdl", "models/p_zapper.mdl", ZAPPER_DRAW, "onehanded",  0 );
}

void CZAPPER::SecondaryAttack( void )
{
  //spaceholder
}

void CZAPPER::PrimaryAttack( void )
{
  if (!(m_pPlayer->m_afButtonPressed & IN_ATTACK))
  return;
  ZAPPERFire( 0.085,(60/300) , FALSE );
}

void CZAPPER::ZAPPERFire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
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

#if defined( CLIENT_WEAPONS )
  flags = FEV_NOTHOST;
#else
  flags = 0;
#endif

  PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), fUseAutoAim ? m_usFireZAPPER : m_usFireZAPPER, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, 0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );

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

  m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_ZAPPER, 2 );
  m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

  if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
  m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

  m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

  if(m_pPlayer->pev->flags & FL_DUCKING)
  {
    switch (RANDOM_LONG(0,1))
    {
 	 case 0: m_pPlayer->pev->punchangle.y -= 1; break;
 	 case 1: m_pPlayer->pev->punchangle.y += 1; break;
    }
  }
  else if (m_pPlayer->pev->velocity.Length() > 0.01f)
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
 	 case 0: m_pPlayer->pev->punchangle.y -= 2; break;
 	 case 1: m_pPlayer->pev->punchangle.y += 2; break;
	}
  }
}

void CZAPPER::Reload( void )
{
  int iResult;

  if (m_iClip == ZAPPER_MAX_CLIP)
  return;

  if (m_iClip == 0)
  {
    iResult = DefaultReload( ZAPPER_MAX_CLIP, ZAPPER_RELOAD, 1.5, 0 );
  }
  else
  {
    iResult = DefaultReload( ZAPPER_MAX_CLIP, ZAPPER_RELOAD, 1.5, 0 );
  }
  if (iResult)
  {
    m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
  }
}

void CZAPPER::WeaponIdle( void )
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
      iAnim = ZAPPER_IDLE1;
      m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 61 / 16;
    }
    else if (flRand <= 0.6f)
    {
      iAnim = ZAPPER_IDLE2;
      m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 41 / 16;
    }
    else
    {
      iAnim = ZAPPER_IDLE3;
      m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 50 / 14;
    }
  SendWeaponAnim( iAnim, 1 );
  }
}

class CZAPPERAmmoClip : public CBasePlayerAmmo
{
  void Spawn( void )
  {
    Precache( );
    SET_MODEL(ENT(pev), "models/w_zapperclip.mdl");
    CBasePlayerAmmo::Spawn( );
  }
  void Precache( void )
  {
    PRECACHE_MODEL ("models/w_zapperclip.mdl");
  }
  BOOL AddAmmo( CBaseEntity *pOther ) 
  {
    int bResult = (pOther->GiveAmmo( AMMO_ZAPPERCLIP_GIVE, "zap", ZAPPER_MAX_CARRY) != -1);
    if(bResult)
    {
      EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip2.wav", 1, ATTN_NORM);
    }
  return bResult;
  }
};
LINK_ENTITY_TO_CLASS( ammo_zapper, CZAPPERAmmoClip );
