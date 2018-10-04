//=========================================================
// Opposing Forces Weapon Desert eagle (use Python ammo)
//
// Made by Demiurge
//
//FGD weapon_deserteagle
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"


enum desert_e
{
EAGLE_IDLE1,
EAGLE_IDLE2,
EAGLE_IDLE3,
EAGLE_IDLE4,
EAGLE_IDLE5,
EAGLE_FIRE,
EAGLE_FIRE_EMPTY,
EAGLE_RELOAD,
EAGLE_RELOAD_EMPTY,
EAGLE_HOLSTER,
EAGLE_DRAW,
};

LINK_ENTITY_TO_CLASS( weapon_eagle, CEagle );
//-----------------------------\// SPAWN FUNCTION OF EAGLE

void CEagle::Spawn( )
{
pev->classname = MAKE_STRING("weapon_eagle"); // hack to allow for old names
Precache( );
m_iId = WEAPON_EAGLE;
SET_MODEL(ENT(pev), "models/w_desert_eagle.mdl");

m_iDefaultAmmo = EAGLE_DEFAULT_GIVE;

FallInit();// get ready to fall down.
}

//-------------------------------\// DESERT

void CEagle::Precache( void )
{
	PRECACHE_MODEL("models/v_desert_eagle.mdl");
	PRECACHE_MODEL("models/w_desert_eagle.mdl");
	PRECACHE_MODEL("models/p_desert_eagle.mdl");
	PRECACHE_MODEL("sprites/laserdot_e.spr");

	PRECACHE_SOUND ("weapons/desert_eagle_fire.wav");//silenced handgun
	PRECACHE_SOUND ("weapons/desert_eagle_reload.wav");//silenced handgun
	PRECACHE_SOUND ("weapons/desert_eagle_sight.wav");//handgun
	PRECACHE_SOUND ("weapons/desert_eagle_sight2.wav");//handgun

	m_usFireEagle = PRECACHE_EVENT( 1, "events/eagle.sc" );
}

int CEagle::GetItemInfo(ItemInfo *p)
{
p->pszName = STRING(pev->classname);
p->pszAmmo1 = "357";
p->iMaxAmmo1 = _EAGLE_MAX_CARRY;
p->pszAmmo2 = NULL;
p->iMaxAmmo2 = -1;
p->iMaxClip = EAGLE_MAX_CLIP;
p->iSlot = 1;
p->iPosition = 2;
p->iFlags = 0;
p->iId = m_iId = WEAPON_EAGLE;
p->iWeight = EAGLE_WEIGHT;

return 1;
}

BOOL CEagle::Deploy( )
{
pev->body = 1;
return DefaultDeploy( "models/v_desert_eagle.mdl", "models/p_desert_eagle.mdl", EAGLE_DRAW, "357" );
}

int CEagle::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

void CEagle::SecondaryAttack( void )
{
	m_fDotActive = ! m_fDotActive;

#ifndef CLIENT_DLL
	if (!m_fDotActive && m_pDot)
	{
		m_pDot->Killed( NULL, GIB_NORMAL );
		m_pDot = NULL;
	}
#endif
	if (!m_fDotActive)
	{
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/desert_eagle_sight2.wav", RANDOM_FLOAT(0.9, 1.0), ATTN_NORM);
	}
	else
	{
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/desert_eagle_sight.wav", RANDOM_FLOAT(0.9, 1.0), ATTN_NORM);
	}

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2;
}


void CEagle::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

#ifndef CLIENT_DLL
	if (m_pDot)
	{
		m_pDot->Killed( NULL, GIB_NEVER );
		m_pDot = NULL;
	}
#endif

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = gpGlobals->time + 10 + RANDOM_FLOAT ( 0, 5 );
	SendWeaponAnim( EAGLE_HOLSTER );
}

void CEagle::PrimaryAttack( void )
{
	if (!m_fDotActive)
	{
	EagleFire( 0.3, TRUE );
	}
	else
	{
	EagleFire( 0.4, FALSE );
	}
}

void CEagle::EagleFire( float flCycleTime, BOOL fUseAutoAim )
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

#if defined ( OLD_WEAPONS )
if (m_iClip != 0)
SendWeaponAnim( EAGLE_FIRE );
else
SendWeaponAnim( EAGLE_FIRE_EMPTY );
#endif

int flags;

#if defined( CLIENT_WEAPONS )
flags = FEV_NOTHOST;
#else
flags = 0;
#endif
PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), fUseAutoAim ? m_usFireEagle : m_usFireEagle, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );

// player "shoot" animation
m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#if defined ( OLD_WEAPONS )
UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
UTIL_Ricochet( ptr->vecStartPos, 1.0 );
UTIL_Ricochet( ptr->vecEndPos, 2.0 ); // ADDED RICHOSHET MODE
+ gpGlobals->v_right * RANDOM_FLOAT(9999,100) 
Vector vecShellVelocity = m_pPlayer->pev->velocity 
+ gpGlobals->v_right * RANDOM_FLOAT(999,300) 
+ gpGlobals->v_up * RANDOM_FLOAT(450,150) 
+ gpGlobals->v_forward * 9999;
#endif

if (pev->body == 0)
{
m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
#if defined ( OLD_WEAPONS )

EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/desert_eagle_fire.wav", RANDOM_FLOAT(0.9, 1.0), ATTN_NORM);
#endif
}

Vector vecSrc = m_pPlayer->GetGunPosition( );
Vector vecAiming;

if ( fUseAutoAim )
{
vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
}
else
{
vecAiming = gpGlobals->v_forward;
}

	if (!m_fDotActive)
	{
		m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_15DEGREES, 8192, BULLET_PLAYER_357, 0 );
	}
	else
	{
		m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_357, 0 );		
	}
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
// HEV suit - indicate out of ammo condition
m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

	UpdateDot( );

#if defined ( OLD_WEAPONS )
m_pPlayer->pev->punchangle.x -= 2;
#endif
}

void CEagle::Reload( void )
{
	int iResult;

	if (m_iClip == 0)
	iResult = DefaultReload( 17, EAGLE_RELOAD_EMPTY, 1.5 );
else
	iResult = DefaultReload( 18, EAGLE_RELOAD, 1.5 );

	if (iResult)
	{
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
}

void CEagle::WeaponIdle( void )
{
	UpdateDot( );

	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
	return;

// only idle if the slid isn''t back
	if (m_iClip != 0)
{
	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

	if (flRand <= 0.3 + 0 * 0.75)
{

}
	else 
	switch (RANDOM_LONG (0, 4))
{
	case 0:
		iAnim = EAGLE_IDLE1 ;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 16.0;
		SendWeaponAnim( iAnim, 1 );
	break;

	case 1:
		iAnim = EAGLE_IDLE2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0 / 16.0;
		SendWeaponAnim( iAnim, 1 );
	break;

	case 2:
		iAnim = EAGLE_IDLE3;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 16.0;
		SendWeaponAnim( iAnim, 1 );
	break;

	case 3:
		iAnim = EAGLE_IDLE4;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 16.0;
		SendWeaponAnim( iAnim, 1 );
	break;

	case 4:
		iAnim = EAGLE_IDLE5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 16.0;
		SendWeaponAnim( iAnim, 1 );
	break;
		}
	}
}

void CEagle::UpdateDot( void )
{

#ifndef CLIENT_DLL
	if (m_fDotActive)
	{
		if (!m_pDot)
		{
			m_pDot = CLaserDot::CreateDot();
		}

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		Vector vecSrc = m_pPlayer->GetGunPosition( );;
		Vector vecAiming = gpGlobals->v_forward;

		TraceResult tr;
		UTIL_TraceLine ( vecSrc, vecSrc + vecAiming * 8192, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr );
		
		UTIL_SetOrigin( m_pDot->pev, tr.vecEndPos );
	}
#endif

}


LINK_ENTITY_TO_CLASS( laser_dot, CLaserDot );

//=========================================================
//=========================================================
CLaserDot *CLaserDot::CreateDot( void )
{
	CLaserDot *pDot = GetClassPtr( (CLaserDot *)NULL );
	pDot->Spawn();

	pDot->pev->classname = MAKE_STRING("laser_dot");

	return pDot;
}

//=========================================================
//=========================================================
void CLaserDot::Spawn( void )
{
	Precache( );
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	pev->rendermode = kRenderGlow;
	pev->renderfx = kRenderFxNoDissipation;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), "sprites/laserdot_e.spr");
	UTIL_SetOrigin( pev, pev->origin );
};

//=========================================================
// Suspend- make the laser sight invisible. 
//=========================================================
void CLaserDot::Suspend( float flSuspendTime )
{
	pev->effects |= EF_NODRAW;
	
	SetThink( Revive );
	pev->nextthink = gpGlobals->time + flSuspendTime;
}

//=========================================================
// Revive - bring a suspended laser sight back.
//=========================================================
void CLaserDot::Revive( void )
{
	pev->effects &= ~EF_NODRAW;

	SetThink( NULL );
}

void CLaserDot::Precache( void )
{
	PRECACHE_MODEL("sprites/laserdot_e.spr");
};