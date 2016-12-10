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
#include "gamerules.h"

// special deathmatch shotgun spreads
#define VECTOR_CONE_DM_SHOTGUN	Vector( 0.08716, 0.04362, 0.00  )// 10 degrees by 5 degrees
#define VECTOR_CONE_DM_DOUBLESHOTGUN Vector( 0.17365, 0.04362, 0.00 ) // 20 degrees by 5 degrees

enum sshotgun_e {
	SHOTGUN_FIREBOTH,
	SHOTGUN_FIRELEFT,
	SHOTGUN_FIRERIGHT,
	SHOTGUN_RELOAD,
	SHOTGUN_DEPOLY,
	SHOTGUN_HOLSTER,
	SHOTGUN_IDLE,
};

class CSshotgun : public CBasePlayerWeapon
{
public:

	void Spawn( void );
	void Precache( void );
	int iItemSlot( ) { return 3; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( );
	void Reload( void );
	void WeaponIdle( void );
	int m_iShell;

};
LINK_ENTITY_TO_CLASS( weapon_sshotgun, CSshotgun );

void CSshotgun::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_sshotgun");
	Precache( );
	m_iId = WEAPON_SSHOTGUN;
	SET_MODEL(ENT(pev), "models/wmodels/w_sshotgun.mdl");

	m_iDefaultAmmo = SSHOTGUN_DEFAULT_GIVE;

	FallInit();
}


void CSshotgun::Precache( void )
{
	PRECACHE_MODEL("models/vmodels/v_sshotgun.mdl");
	PRECACHE_MODEL("models/wmodels/w_sshotgun.mdl");
	PRECACHE_MODEL("models/pmodels/p_sshotgun.mdl");

	m_iShell = PRECACHE_MODEL ("models/shotgunshell.mdl");           

	PRECACHE_SOUND ("weapons/sshotgun1.wav");
	PRECACHE_SOUND ("weapons/sshotgun2.wav");
}

int CSshotgun::AddToPlayer( CBasePlayer *pPlayer )
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


int CSshotgun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = BUCKSHOT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SSHOTGUN_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SSHOTGUN;
	p->iWeight = SSHOTGUN_WEIGHT;

	return 1;
}



BOOL CSshotgun::Deploy( )
{
	return DefaultDeploy( "models/vmodels/v_sshotgun.mdl", "models/pmodels/p_sshotgun.mdl", SHOTGUN_DEPOLY, "shotgun" );
}


void CSshotgun::PrimaryAttack()
{
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		Reload( );
		if (m_iClip == 0)
			PlayEmptySound( );
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;
	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	SendWeaponAnim( SHOTGUN_FIREBOTH );

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector	vecShellVelocity = m_pPlayer->pev->velocity 
							 + gpGlobals->v_right * RANDOM_FLOAT(50,70) 
							 + gpGlobals->v_up * RANDOM_FLOAT(100,150) 
							 + gpGlobals->v_forward * 25;

	EjectBrass ( m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -12 + gpGlobals->v_forward * 20 + gpGlobals->v_right * 4 , vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHOTSHELL); 

	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/sshotgun1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0,0x1f));
	

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	m_pPlayer->FireBullets( 8, vecSrc, vecAiming, VECTOR_CONE_DM_SHOTGUN, 2048, BULLET_PLAYER_BUCKSHOT, 0 );


	m_flNextPrimaryAttack = gpGlobals->time + 0.75;
	m_flNextSecondaryAttack = gpGlobals->time + 0.75;
	m_flTimeWeaponIdle = gpGlobals->time + 5.0;


	m_pPlayer->pev->punchangle.x -= 5;
}


void CSshotgun::SecondaryAttack( void )
{
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	if (m_iClip <= 1)
	{
		Reload( );
		PlayEmptySound( );
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip -= 2;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	SendWeaponAnim( SHOTGUN_FIREBOTH );

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
		
	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/sshotgun2.wav", RANDOM_FLOAT(0.98, 1.0), ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f));
	
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	
	m_pPlayer->FireBullets( 16, vecSrc, vecAiming, VECTOR_CONE_DM_DOUBLESHOTGUN, 2048, BULLET_PLAYER_BUCKSHOT, 0 );

	m_flNextPrimaryAttack = gpGlobals->time + 1.5;
	m_flNextSecondaryAttack = gpGlobals->time + 1.5;
	
	m_flTimeWeaponIdle = gpGlobals->time + 6.0;

	m_pPlayer->pev->punchangle.x -= 10;
}


void CSshotgun::Reload( void )
{
	int iResult;

	if (m_iClip == 0)
		iResult = DefaultReload( 2, SHOTGUN_RELOAD, 3.0 );
	else
		iResult = DefaultReload( 2, SHOTGUN_RELOAD, 3.0 );

	if (iResult)
	{
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	}
}
void CSshotgun::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	int iAnim;
	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:	
		iAnim = SHOTGUN_IDLE;	
		break;
	
	default:
	case 1:
		iAnim = SHOTGUN_IDLE;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
}


