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

enum ak47_e
{
	AK47_LONGIDLE = 0,
	AK47_IDLE1,
	AK47_RELOAD,
	AK47_DEPLOY,
	AK47_FIRE1,
	AK47_FIRE2,
	AK47_FIRE3,
	AK47_KNIFE,
};

LINK_ENTITY_TO_CLASS( weapon_ak47, CAK47 );
LINK_ENTITY_TO_CLASS( weapon_ak47_drop, CAK47 );
void CAK47::Spawn( )
{
	Precache( );

	SET_MODEL(ENT(pev), "models/w_all_items4.mdl");
	pev->body = 15;

	m_iId = WEAPON_AK47;

	m_iDefaultAmmo = 30;

	#ifndef CLIENT_DLL
	if ( FClassnameIs( pev, "weapon_ak47_drop" ) ){
	m_iDefaultAmmo = 15;
	}
	#endif

	pev->classname = MAKE_STRING("weapon_ak47"); // hack to allow for old names
	FallInit();// get ready to fall down.
}


void CAK47::Precache( void )
{
	PRECACHE_MODEL("models/v_t562.mdl");

	PRECACHE_SOUND ("weapons/ak47-fire.wav");

	PRECACHE_SOUND ("weapons/knife_hitwall.wav");
	PRECACHE_SOUND ("weapons/knife_w_stab.wav");
	PRECACHE_SOUND ("weapons/knife_miss2.wav");

	m_usAK47 = PRECACHE_EVENT( 1, "events/ak47.sc" );
}

int CAK47::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "762nato";
	p->iMaxAmmo1 = AK_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 30;
	p->iSlot = 2;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_AK47;
	p->iWeight = AK47_WEIGHT;

	return 1;
}

int CAK47::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CAK47::Deploy( )
{
	m_pPlayer->m_newcross_active = 1;
	return DefaultDeploy( "models/v_t562.mdl", 0, AK47_DEPLOY, "mp5" );
}

void CAK47::SecondaryAttack( void )
{
	if ( !TriggerReleased ){
	return;
	}

	TraceResult tr;

	UTIL_MakeVectors (m_pPlayer->pev->v_angle);
	Vector vecSrc	= m_pPlayer->GetGunPosition( );
	Vector vecEnd	= vecSrc + gpGlobals->v_forward * 75;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

	SendWeaponAnim( AK47_KNIFE );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	if ( tr.flFraction >= 1.0 )
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;

		#ifndef CLIENT_DLL
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/knife_miss2.wav", 1, ATTN_NORM);
		#endif
	}
	else
	{
		
#ifndef CLIENT_DLL
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
		int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,m_pPlayer->Classify(),0);
		FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

		ClearMultiDamage( );

		pEntity->TraceAttack(m_pPlayer->pev, 30, gpGlobals->v_forward, &tr, DMG_CLUB ); 

		ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		if (pEntity)
		{
			if ( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE
				&& pEntity->Classify() != CLASS_MACHINE_ASS && pEntity->Classify() != CLASS_MACHINE_BLACK)
			{
				EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/knife_w_stab.wav", 1, ATTN_NORM);

				m_pPlayer->m_iWeaponVolume = 128;

			    flVol = 0.1;

				fHitWorld = FALSE;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if (fHitWorld)
		{
			float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd-vecSrc)*2, 512);

			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/knife_hitwall.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 

			// delay the decal a bit
			DecalGunshot( &tr, BULLET_PLAYER_CROWBAR );
		}

		m_pPlayer->m_iWeaponVolume = flVol * 512;
#endif

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75;

		
	}
}

void CAK47::PrimaryAttack()
{
	// don't fire underwater
	//if (m_pPlayer->pev->waterlevel == 3)
	//{
	//	PlayEmptySound();
	//	m_flNextPrimaryAttack = 0.15;
	//	return;
//	}
	TriggerReleased = FALSE;

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector vecDir;

	if ( m_pPlayer->pev->flags & FL_ONGROUND ) 
	{	
		if ( m_pPlayer->pev->flags & FL_DUCKING ) {
			vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector(0.011,0.011,0.011), 6000, BULLET_762Nato, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
		}
		else if ( m_pPlayer->pev->velocity.Length2D() >= 200)
		{
			vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector(0.015,0.015,0.015), 6000, BULLET_762Nato, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
		}
		else{
			vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector(0.012,0.012,0.012), 6000, BULLET_762Nato, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
		}
	}
	else{
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector(0.018,0.018,0.018), 6000, BULLET_762Nato, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}
	
   int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usAK47, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.095;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

	m_pPlayer->pev->punchangle.x -= 0.3;

	if (m_pPlayer->pev->velocity.Length2D() > 200)
		KickBack(1.5, 0.45, 0.225, 0.05, 6.5, 2.5, 7);
	else if (!FBitSet(m_pPlayer->pev->flags, FL_ONGROUND))
		KickBack(2.0, 1.0, 0.5, 0.35, 9.0, 6.0, 5);
	else if (FBitSet(m_pPlayer->pev->flags, FL_DUCKING))
		KickBack(0.9, 0.35, 0.15, 0.025, 5.5, 1.5, 9);
	else
		KickBack(1.0, 0.375, 0.175, 0.0375, 5.75, 1.75, 8);
}

void CAK47::Reload( void )
{
	if ( m_pPlayer->ammo_762nato <= 0 )
		return;

	DefaultReload( 30, AK47_RELOAD, 2.0 );
}


void CAK47::WeaponIdle( void )
{
	TriggerReleased = TRUE;
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:	
		iAnim = AK47_LONGIDLE;	
		break;
	
	default:
	case 1:
		iAnim = AK47_IDLE1;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}



class CAK47AmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_all_items4.mdl");
		pev->body = 16;
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( 30, "762nato", AK_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_762, CAK47AmmoClip );
LINK_ENTITY_TO_CLASS( ammo_ak47clip, CAK47AmmoClip );