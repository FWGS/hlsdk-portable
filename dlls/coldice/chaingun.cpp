/*
	Copyright (c) 1999, Cold Ice Modification. 
	
	This code has been written by SlimShady ( darcuri@optonline.net )

    Use, distribution, and modification of this source code and/or resulting
    object code is restricted to non-commercial enhancements to products from
    Valve LLC.  All other use, distribution, or modification is prohibited
    without written permission from Valve LLC and from the Cold Ice team.

    Please if you use this code in any public form, please give us credit.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

enum chaingun_e
{
    CHAINGUN_IDLE = 0,
	CHAINGUN_IDLE1,
	CHAINGUN_SPINUP,
	CHAINGUN_SPINDOWN,
	CHAINGUN_FIRE,
	CHAINGUN_DRAW,
	CHAINGUN_HOLSTER,
};


class CChaingun : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 3; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void ChaingunFire( float flSpread, float flCycleTime, BOOL fUseAutoAim ,int b_no);
	BOOL Deploy( void );
	void Reload( void );
	void BringUp( void );
	void WeaponIdle( void );
	void slowdown();
	void EXPORT firemore(void);
	int phase;
	float cycletime;
	int fps;
	int m_iShell;
};
LINK_ENTITY_TO_CLASS( weapon_chaingun, CChaingun );
LINK_ENTITY_TO_CLASS( weapon_9mmAR, CChaingun );

//=========================================================
//=========================================================

void CChaingun::Spawn( )
{	 
	pev->classname = MAKE_STRING("weapon_chaingun"); 
	Precache( );
	SET_MODEL(ENT(pev), "models/wmodels/w_mini.mdl");
	m_iId = WEAPON_CHAINGUN;

	m_iDefaultAmmo = CHAINGUN_DEFAULT_GIVE;

	FallInit();
}


void CChaingun::Precache( void )
{
	PRECACHE_MODEL("models/vmodels/v_mini.mdl");
	PRECACHE_MODEL("models/wmodels/w_mini.mdl");
	PRECACHE_MODEL("models/pmodels/p_mini.mdl");
	m_iShell = PRECACHE_MODEL ("models/shell.mdl");

	PRECACHE_SOUND("weapons/chaingun1.wav"); 
	PRECACHE_SOUND("weapons/chaingun_spinup.wav"); 
	PRECACHE_SOUND("weapons/chaingun_spindown.wav"); 
    PRECACHE_SOUND("weapons/357_cock1.wav");
}

int CChaingun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "chain";
	p->iMaxAmmo1 = CHAINGUN_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = CHAINGUN_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 5;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_CHAINGUN;
	p->iWeight = CHAINGUN_WEIGHT;

	return 1;
}

int CChaingun::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CChaingun::Deploy( )
{
	return DefaultDeploy( "models/vmodels/v_mini.mdl", "models/pmodels/p_mini.mdl", CHAINGUN_DRAW, "mp5" );
}


void CChaingun::PrimaryAttack()
{
	m_flNextPrimaryAttack = gpGlobals->time + 0.1;
	
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == 0 && m_iClip == 0) 
	{
		PlayEmptySound();
		RetireWeapon();
		return;
	}
	
	m_flTimeWeaponIdle = gpGlobals->time + 0.1;
	
	if((phase==0)&&(m_iClip > 0))
	{
		SendWeaponAnim( CHAINGUN_SPINUP );
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/chaingun_spinup.wav", RANDOM_FLOAT(0.92, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));	
		m_flNextPrimaryAttack = gpGlobals->time + 0.4;
		phase=1;
		return;
	}
	
	slowdown();
	
	if (fps == 0) 
		fps = 5;
	
	if (fps < 22) 
		fps++;
	
	if((phase = 1)&&(m_iClip > 0))	
		ChaingunFire( 0.04, 1/(float) fps, TRUE ,1);
}

void CChaingun::ChaingunFire( float flSpread , float flCycleTime, BOOL fUseAutoAim,int bulletsno )
{
	if (m_iClip <= 0)
	{
		SendWeaponAnim( CHAINGUN_IDLE );
		PlayEmptySound();
		m_flNextPrimaryAttack = gpGlobals->time + 0.3;
		return;
	}

	
	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	SendWeaponAnim( CHAINGUN_FIRE );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_iClip--;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
		
	Vector	vecShellVelocity = m_pPlayer->pev->velocity 
							 + gpGlobals->v_right * RANDOM_FLOAT(50,70) 
							 + gpGlobals->v_up * RANDOM_FLOAT(100,150) 
							 + gpGlobals->v_forward * 25;
	EjectBrass ( pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -12 + gpGlobals->v_forward * 32 + gpGlobals->v_right * 6 , vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL ); 

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/chaingun1.wav", RANDOM_FLOAT(0.92, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));

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

	m_pPlayer->FireBullets(bulletsno,  vecSrc, vecAiming, VECTOR_CONE_5DEGREES, 8192, BULLET_PLAYER_9MM, 0 );
	m_flNextPrimaryAttack = gpGlobals->time + flCycleTime;

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

}


void CChaingun::slowdown()
{
	if ( m_iClip != 0 )
	{
		Vector xy=Vector(m_pPlayer->pev->velocity.x,m_pPlayer->pev->velocity.y,0).Normalize();
	
		if(m_pPlayer->pev->velocity.Length2D()>50)
			m_pPlayer->pev->velocity=Vector(xy.x*50,xy.y*50,m_pPlayer->pev->velocity.z);
	}
}
void CChaingun::Reload( void )
{
	if ( m_iClip <= 99  )
	{
		if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 0 )
			return;

		DefaultReload( CHAINGUN_MAX_CLIP, CHAINGUN_HOLSTER, 1.5 );
		SetThink( BringUp );
		pev->nextthink = gpGlobals->time + 1.5;
	}
}

void CChaingun::BringUp( void )
{
	SendWeaponAnim( CHAINGUN_DRAW );
	m_flNextPrimaryAttack = gpGlobals->time + 1.0; 
}

void CChaingun::WeaponIdle( void )
{
	cycletime=0;
	fps=0;
	
	if(phase==1)
	{
		m_flTimeWeaponIdle = m_flNextPrimaryAttack = gpGlobals->time + 1;
		SendWeaponAnim( CHAINGUN_SPINDOWN );
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/chaingun_spindown.wav", RANDOM_FLOAT(0.92, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
		phase=0;
		return;
	}
	
	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;
	
	ResetEmptySound( );
	
	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	int iAnim;
	
	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:	
		iAnim = CHAINGUN_IDLE;	
		break;
	
	default:
	case 1:
		iAnim = CHAINGUN_IDLE1;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
}



class CChaingunBox : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/ammo/w_chain.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/ammo/w_chain.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_CHAINGUNBOX_GIVE, "chain", CHAINGUN_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_chaingunbox, CChaingunBox );