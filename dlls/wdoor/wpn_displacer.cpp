/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"
#include "shake.h"

enum displacer_e 
{
	DISPLACER_IDLE1,
	DISPLACER_IDLE2,
	DISPLACER_SPINUP,
	DISPLACER_SPIN,
	DISPLACER_FIRE,
	DISPLACER_DRAW,
	DISPLACER_HOLSTER
};

LINK_ENTITY_TO_CLASS( weapon_displacer, CDisplacer );

//=========================================================
//=========================================================

void CDisplacer::Spawn( )
{
	Precache( );
	SET_MODEL(ENT(pev), "models/w_all_items3.mdl");
	pev->body = 0;
	m_iId = WEAPON_DISPLACER;

	m_iDefaultAmmo = 60;

	FallInit();// get ready to fall down.
}


void CDisplacer::Precache( void )
{
	PRECACHE_MODEL("models/v_displacer.mdl");

	PRECACHE_SOUND("weapons/displacer_fire.wav");
	PRECACHE_SOUND("weapons/displacer_spin.wav");
	PRECACHE_SOUND("weapons/displacer_teleport.wav");
	PRECACHE_SOUND("weapons/displacer_teleportblast.wav");
	PRECACHE_SOUND("weapons/displacer_self.wav");
}

int CDisplacer::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "uranium";
	p->iMaxAmmo1 = URANIUM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_DISPLACER;
	p->iWeight = 30;

	return 1;
}

int CDisplacer::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CDisplacer::Deploy( )
{
	m_pPlayer->m_newcross_active = 1;
	m_fldisplacer = 0;
	return DefaultDeploy( "models/v_displacer.mdl", 0, DISPLACER_DRAW, "gauss",0,114 );
}

void CDisplacer::Holster( int skiplocal )
{
	#ifndef CLIENT_DLL	
	FX_FireGun(m_pPlayer->pev->v_angle, m_pPlayer->entindex(), 200, 2, FIREGUN_DISPLACER );
	#endif
	m_pPlayer->m_flNextAttack = gpGlobals->time + 0.7;
	m_fldisplacer = 0;
	SendWeaponAnim( DISPLACER_HOLSTER );
}

void CDisplacer::PrimaryAttack()
{
	if(m_fldisplacer >= 1){
		if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() ){
		WeaponIdle( );
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1;
		}
		return;
	}

	if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] < 20 )
	{
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}

	
	SendWeaponAnim( DISPLACER_SPINUP );
	m_fldisplacer = 1;

	#ifndef CLIENT_DLL	
	FX_FireGun(m_pPlayer->pev->v_angle, m_pPlayer->entindex(), 200, 1, FIREGUN_DISPLACER );

	if(m_pPlayer->m_darkposion > 0){
	m_pPlayer->m_darkposion = 0;//隐身取消
	}
	#endif

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1;
}

void CDisplacer::SecondaryAttack( void )
{
	m_fldisplacer = 0;
	if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] < 60 )
	{
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}
	/*
	if ( m_pPlayer->m_teleprort_in_xen == 1 )
	{
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}
	
	if ( m_pPlayer->pev->flags & FL_DUCKING ) {
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}
	*/

	SendWeaponAnim( DISPLACER_SPINUP );
	m_fldisplacer = 2;

	#ifndef CLIENT_DLL	
	FX_FireGun(m_pPlayer->pev->v_angle, m_pPlayer->entindex(), 200, 1, FIREGUN_DISPLACER );

	if(m_pPlayer->m_darkposion > 0){
	m_pPlayer->m_darkposion = 0;//隐身取消
	}
	#endif

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1;
}

void CDisplacer::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;


	if(m_fldisplacer == 1){
				

					m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
					m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

					m_pPlayer->m_iExtraSoundTypes = bits_SOUND_DANGER;
					m_pPlayer->m_flStopExtraSoundTime = UTIL_WeaponTimeBase() + 0.2;
							
					m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 20;

				// player "shoot" animation
				m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

 				UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

			#ifndef CLIENT_DLL
					Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 32 + gpGlobals->v_right * 4 + gpGlobals->v_up * -4;
					Vector vecThrow = gpGlobals->v_forward * 600;
					CBaseEntity *pSatchel = Create( "displacer_ball", vecSrc, Vector( 0, 0, 0), m_pPlayer->edict() );
					pSatchel->pev->velocity = vecThrow;
					pSatchel->pev->owner = m_pPlayer->edict();

					FX_FireGun(m_pPlayer->pev->v_angle, m_pPlayer->entindex(), 200, 0, FIREGUN_DISPLACER );
			#endif

				SendWeaponAnim( DISPLACER_FIRE );
				m_fldisplacer = 0;
				//m_pPlayer->pev->punchangle.x -= 20;
				m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 2;
				m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;// idle pretty soon after shooting.

				return;
	}
	else if(m_fldisplacer == 2){
		SendWeaponAnim( DISPLACER_FIRE );
		m_fldisplacer = 0;

		#ifndef CLIENT_DLL

		FX_FireGun(m_pPlayer->pev->v_angle, m_pPlayer->entindex(), 200, 2, FIREGUN_DISPLACER );

		if(m_pPlayer->m_teleprort_in_xen == 2){
		return;
		}

		CBaseEntity *pEntity = NULL;
		while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)//搜索小范围
		{
			if ( FClassnameIs(pEntity->pev, "info_displacer_xen_target")
			|| FClassnameIs(pEntity->pev, "info_displacer_earth_target") )
			{
					Vector vecSrc = m_pPlayer->GetGunPosition();
					Vector vecThrow = gpGlobals->v_up * -600;
					CBaseEntity *pSatchel = Create( "displacer_ball", vecSrc, Vector( 0, 0, 0), m_pPlayer->edict() );
					pSatchel->pev->velocity = vecThrow;
					pSatchel->pev->owner = m_pPlayer->edict();

			m_pPlayer->m_old_teleprort_origin = pev->origin;

			if(FClassnameIs(pEntity->pev, "info_displacer_earth_target")){
			m_pPlayer->m_teleprort_in_xen = 0;
			}
			else{
			m_pPlayer->m_teleprort_in_xen = 1;
			}

			m_pPlayer->pev->origin = pEntity->pev->origin;
			m_pPlayer->pev->velocity = g_vecZero;
			m_pPlayer->m_flVelocityModifier -= 1;
			m_pPlayer->m_god_time = gpGlobals->time + 1.0;

			m_pPlayer->pev->angles = pEntity->pev->angles;
			m_pPlayer->pev->fixangle = TRUE;

			UTIL_ScreenFade( m_pPlayer, Vector(0, 200, 0), 0.5, 0.5, 255, FFADE_IN );
			FX_Explosion(pEntity->pev->origin, EXPLOSION_DISPTELEPORT );
			
			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 60;			
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 3;
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1;

			EMIT_SOUND_DYN ( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/displacer_self.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			break;
			}
		}
		#endif
	}

	int iAnim;
	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:	
		iAnim = DISPLACER_IDLE1;	
		break;
	
	default:
	case 1:
		iAnim = DISPLACER_IDLE2;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}







