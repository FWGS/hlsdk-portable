/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "shake.h"
#include "gamerules.h"
#include "effects.h"

#include "lightning_gun.h"


enum LightningGun_e {
	LIGHTNING_GUN_IDLE = 0,
	LIGHTNING_GUN_DRAW,
	LIGHTNING_GUN_HOLSTER,
	LIGHTNING_GUN_ZAP
};

LINK_ENTITY_TO_CLASS( weapon_lightninggun, CLightningGun );


TYPEDESCRIPTION	CLightningGun::m_SaveData[] = 
{
	DEFINE_FIELD( CLightningGun, m_fInAttack, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CLightningGun, CBasePlayerWeapon );


void CLightningGun::Spawn( )
{
	Precache( );
	m_iId = WEAPON_LIGHTNING_GUN;
	SET_MODEL(ENT(pev), "models/w_lightning_gun.mdl");

	m_iDefaultAmmo = LIGHTNING_GUN_DEFAULT_GIVE;

	FallInit();// get ready to fall down.

	m_iBeams = 0;
}

void CLightningGun::Precache( void )
{
	PRECACHE_MODEL("models/w_lightning_gun.mdl");
	PRECACHE_MODEL("models/v_lightning_gun.mdl");
//	PRECACHE_MODEL("models/p_lightning_gun.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");

//	PRECACHE_SOUND("weapons/lightning_gun2.wav");
	PRECACHE_SOUND("weapons/electro4.wav");
//	PRECACHE_SOUND("weapons/electro5.wav");
//	PRECACHE_SOUND("weapons/electro6.wav");
	PRECACHE_SOUND("debris/zap4.wav");
	PRECACHE_SOUND("hassault/hw_shoot1.wav");
	
	PRECACHE_MODEL( "sprites/lgtning.spr" ); 
}

int CLightningGun::AddToPlayer( CBasePlayer *pPlayer )
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

int CLightningGun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "lightning";
	p->iMaxAmmo1 = LIGHTNING_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_LIGHTNING_GUN;
	p->iFlags = 0;
	p->iWeight = LIGHTNING_GUN_WEIGHT;

	return 1;
}


BOOL CLightningGun::Deploy( )
{
	return DefaultDeploy( "models/v_lightning_gun.mdl", "", LIGHTNING_GUN_DRAW, "lightning gun" );
}


void CLightningGun::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	// m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	SendWeaponAnim( LIGHTNING_GUN_HOLSTER );
	m_fInAttack = 0;
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}


void CLightningGun::PrimaryAttack()
{
	if (m_fInAttack != 0)
	{
		if ( m_fInAttack == 1 )
		{
			ZapShoot();
		}
		else // == 2
		{
			ZapDone();
		}
		return;
	}

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 1)
	{
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}

	ZapPowerUp();
}

void CLightningGun::ZapPowerUp()
{
	m_fInAttack = 1;

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "debris/zap4.wav", 1, ATTN_NORM, 0, 150 );

	m_flTimeWeaponIdle = gpGlobals->time + 0.5;
}


void CLightningGun::ZapShoot()
{
	ClearBeams( );

	ClearMultiDamage();

	UTIL_MakeAimVectors( pev->angles );

	ZapBeam( -1 );
	ZapBeam( 1 );

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 130, 160 ) );
	// STOP_SOUND( ENT(pev), CHAN_WEAPON, "debris/zap4.wav" );
	ApplyMultiDamage(pev, pev);

	m_flTimeWeaponIdle = gpGlobals->time + 0.2;
	m_flNextPrimaryAttack = gpGlobals->time + 0.2;

	m_fInAttack = 2;
}

void CLightningGun::ZapDone()
{
	ClearBeams( );

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

	m_fInAttack = 0;
	m_flTimeWeaponIdle = gpGlobals->time + 0.8;
	m_flNextPrimaryAttack = gpGlobals->time + 0.8;
}

//=========================================================
// ZapBeam - heavy damage directly forward
//=========================================================
void CLightningGun :: ZapBeam( int side )
{
	TraceResult tr;
	CBaseEntity *pEntity;

	if (m_iBeams >= LIGHTNING_GUN_MAX_BEAMS)
		return;

	// this should be attachment 0 (???)
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecDest = vecSrc + vecAiming * 2048;
	UTIL_TraceLine( vecSrc, vecDest, dont_ignore_monsters, m_pPlayer->edict(), &tr );

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 50 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, m_pPlayer->entindex( ) );
	m_pBeam[m_iBeams]->SetEndAttachment( 1 );
	m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 20 );
	m_pBeam[m_iBeams]->pev->spawnflags |= SF_BEAM_TEMPORARY;	// Flag these to be destroyed on save/restore or level transition
	m_iBeams++;

	pEntity = CBaseEntity::Instance(tr.pHit);
	if (pEntity != NULL && pEntity->pev->takedamage)
	{
		pEntity->TraceAttack( pev, gSkillData.plrDmgLightningGun, vecAiming, &tr, DMG_SHOCK );
	}
	UTIL_EmitAmbientSound( ENT(pev), tr.vecEndPos, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ) );
}

void CLightningGun :: ClearBeams( )
{
	for (int i = 0; i < LIGHTNING_GUN_MAX_BEAMS; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}
	m_iBeams = 0;

	STOP_SOUND( ENT(pev), CHAN_WEAPON, "debris/zap4.wav" );
}

void CLightningGun::SecondaryAttack()
{
}

//=========================================================
// StartFire- since all of this code has to run and then 
// call Fire(), it was easier at this point to rip it out 
// of weaponidle() and make its own function then to try to
// merge this into Fire(), which has some identical variable names 
//=========================================================

void CLightningGun::WeaponIdle( void )
{
	ResetEmptySound( );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	if (m_fInAttack != 0)
	{
		if ( m_fInAttack == 1 )
		{
			ZapShoot();
		}
		else // == 2
		{
			ZapDone();
		}
	}
	else
	{
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 2, 3 );

		SendWeaponAnim( LIGHTNING_GUN_IDLE );		
	}
}

///////////////////////////////////////////////////////////////////////////////////////

void CLightningGunAmmo::Spawn( void )
{ 
	Precache( );
	SET_MODEL(ENT(pev), "models/w_lightning_gun_ammo.mdl");
	CBasePlayerAmmo::Spawn( );
}

void CLightningGunAmmo::Precache( void )
{
	PRECACHE_MODEL ("models/w_lightning_gun_ammo.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");
}

BOOL CLightningGunAmmo::AddAmmo( CBaseEntity *pOther ) 
{ 
	if (pOther->GiveAmmo( AMMO_LIGHTNING_GIVE, "lightning", LIGHTNING_MAX_CARRY ) != -1)
	{
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		return TRUE;
	}
	return FALSE;
}


LINK_ENTITY_TO_CLASS( ammo_lightning, CLightningGunAmmo );

#endif