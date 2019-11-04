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
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "effects.h"
#include "customentity.h"
#include "gamerules.h"

#include "drainlife.h"

#define	DRAINLIFE_PRIMARY_VOLUME		250
#define DRAINLIFE_BEAM_SPRITE			"sprites/xbeam1.spr"
#define DRAINLIFE_FLARE_SPRITE		"sprites/XSpark1.spr"
#define DRAINLIFE_SOUND_OFF			"weapons/egon_off1.wav"
#define DRAINLIFE_SOUND_RUN			"weapons/egon_run3.wav"
#define DRAINLIFE_SOUND_STARTUP		"weapons/egon_windup2.wav"


enum DrainLife_e {
	DRAINLIFE_OPEN = 0,
	DRAINLIFE_IDLE1,
	DRAINLIFE_IDLE2,
	DRAINLIFE_IDLE3,
	DRAINLIFE_CAST,
	DRAINLIFE_CLOSE
};


LINK_ENTITY_TO_CLASS( weapon_drainlife, CDrainLife );

TYPEDESCRIPTION	CDrainLife::m_SaveData[] = 
{
	DEFINE_FIELD( CDrainLife, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CDrainLife, m_pNoise, FIELD_CLASSPTR ),
	DEFINE_FIELD( CDrainLife, m_pSprite, FIELD_CLASSPTR ),
	DEFINE_FIELD( CDrainLife, m_shootTime, FIELD_TIME ),
	DEFINE_FIELD( CDrainLife, m_fireState, FIELD_INTEGER ),
	DEFINE_FIELD( CDrainLife, m_flAmmoUseTime, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CDrainLife, CBasePlayerWeapon );


void CDrainLife::Spawn( )
{
	Precache( );
	m_iId = WEAPON_DRAINLIFE;
	SET_MODEL(ENT(pev), "models/w_drainlife.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CDrainLife::Precache( void )
{
	PRECACHE_MODEL("models/w_drainlife.mdl");
	PRECACHE_MODEL("models/v_drainlife.mdl");
//	PRECACHE_MODEL("models/p_drainlife.mdl");

	PRECACHE_MODEL("models/w_9mmclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND( DRAINLIFE_SOUND_OFF );
	PRECACHE_SOUND( DRAINLIFE_SOUND_RUN );
	PRECACHE_SOUND( DRAINLIFE_SOUND_STARTUP );

	PRECACHE_MODEL( DRAINLIFE_BEAM_SPRITE );
	PRECACHE_MODEL( DRAINLIFE_FLARE_SPRITE );

	PRECACHE_SOUND ("weapons/357_cock1.wav");
}


BOOL CDrainLife::Deploy( void )
{
	m_deployed = FALSE;
	return DefaultDeploy( "models/v_drainlife.mdl", "", DRAINLIFE_OPEN, "shrivelling" );
}

int CDrainLife::AddToPlayer( CBasePlayer *pPlayer )
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



void CDrainLife::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	// m_flTimeWeaponIdle = gpGlobals->time + UTIL_RandomFloat ( 10, 15 );
	SendWeaponAnim( DRAINLIFE_CLOSE );

	if ( m_fireState != FIRE_OFF )
		EndAttack();
}

int CDrainLife::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_DRAINLIFE;
	p->iFlags = 0;
	p->iWeight = DRAINLIFE_WEIGHT;

	return 1;
}


//#define DRAINLIFE_PULSE_INTERVAL			0.25
//#define DRAINLIFE_DISCHARGE_INTERVAL		0.5

#define DRAINLIFE_PULSE_INTERVAL			0.1
#define DRAINLIFE_DISCHARGE_INTERVAL		0.1

float CDrainLife::GetPulseInterval( void )
{
	if ( g_pGameRules->IsMultiplayer() )
	{
		return 0.1;
	}

	return DRAINLIFE_PULSE_INTERVAL;
}

float CDrainLife::GetDischargeInterval( void )
{
	if ( g_pGameRules->IsMultiplayer() )
	{
		return 0.1;
	}

	return DRAINLIFE_DISCHARGE_INTERVAL;
}

void CDrainLife::Attack( void )
{
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );

	switch( m_fireState )
	{
		case FIRE_OFF:
		{
			m_flAmmoUseTime = gpGlobals->time;// start using ammo ASAP.

			SendWeaponAnim( DRAINLIFE_CAST );

			m_pPlayer->m_iWeaponVolume = DRAINLIFE_PRIMARY_VOLUME;
			m_flTimeWeaponIdle = gpGlobals->time + 0.1;
			m_shootTime = gpGlobals->time + 2;

			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, DRAINLIFE_SOUND_STARTUP, 0.2, ATTN_NORM, 0, 100 );

			pev->dmgtime = gpGlobals->time + GetPulseInterval();
			m_fireState = FIRE_CHARGE;
		}
		break;

		case FIRE_CHARGE:
		{
			Fire( vecSrc, vecAiming );
			m_pPlayer->m_iWeaponVolume = DRAINLIFE_PRIMARY_VOLUME;

			if ( m_shootTime != 0 && gpGlobals->time > m_shootTime )
			{
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_STATIC, DRAINLIFE_SOUND_RUN, 0.2, ATTN_NORM, 0, 100 );

				m_shootTime = 0;
			}
		}
		break;
	}
}

void CDrainLife::PrimaryAttack( void )
{
	Attack();
}

void CDrainLife::Fire( const Vector &vecOrigSrc, const Vector &vecDir )
{
	Vector vecDest = vecOrigSrc + vecDir * 2048;
	edict_t		*pentIgnore;
	TraceResult tr;

	pentIgnore = m_pPlayer->edict();
	Vector tmpSrc = vecOrigSrc + gpGlobals->v_up * -8 + gpGlobals->v_right * 3;

	// ALERT( at_console, "." );
	
	UTIL_TraceLine( vecOrigSrc, vecDest, dont_ignore_monsters, pentIgnore, &tr );

	if (tr.fAllSolid)
		return;

	CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

	if (pEntity == NULL)
		return;

	if ( g_pGameRules->IsMultiplayer() )
	{
		if ( m_pSprite && pEntity->pev->takedamage )
		{
			m_pSprite->pev->effects &= ~EF_NODRAW;
		}
		else if ( m_pSprite )
		{
			m_pSprite->pev->effects |= EF_NODRAW;
		}
	}

	float timedist;

	if ( pev->dmgtime < gpGlobals->time )
	{
		bool bHuman = false;

		if (FClassnameIs(pEntity->pev, "player")) bHuman = true;
		else if (FClassnameIs(pEntity->pev, "monster_cultist")) bHuman = true;
		else if (FClassnameIs(pEntity->pev, "monster_priest")) bHuman = true;
		else if (FClassnameIs(pEntity->pev, "monster_policeman")) bHuman = true;
		else if (FClassnameIs(pEntity->pev, "monster_civilian")) bHuman = true;
		else if (FClassnameIs(pEntity->pev, "monster_butler")) bHuman = true;
		else if (FClassnameIs(pEntity->pev, "monster_sirhenry")) bHuman = true;
		else if (FClassnameIs(pEntity->pev, "monster_ranulf")) bHuman = true;
		else if (FClassnameIs(pEntity->pev, "monster_gangster")) bHuman = true;
		else if (FClassnameIs(pEntity->pev, "monster_sitting_scientist")) bHuman = true;
		else if (FClassnameIs(pEntity->pev, "monster_scientist")) bHuman = true;
		else if (FClassnameIs(pEntity->pev, "monster_sitting_butler")) bHuman = true;
		else if (FClassnameIs(pEntity->pev, "monster_sitting_civilian")) bHuman = true;
		else if (FClassnameIs(pEntity->pev, "monster_sitting_sirhenry")) bHuman = true;

		if (bHuman)
		{
			float flOldTgtHealth = pEntity->pev->health;
			pEntity->TakeDamage(m_pPlayer->pev,m_pPlayer->pev, gSkillData.plrDmgDrainLife, DMG_ENERGYBEAM);
			float flTgtDamage = flOldTgtHealth - max(0,pEntity->pev->health);

			// lose one point of san for every 5 health taken
			m_pPlayer->LoseSanity(flTgtDamage/5.0);

			float flOldHealth = m_pPlayer->pev->health;
			m_pPlayer->TakeHealth(flTgtDamage, DMG_ENERGYBEAM);
		}

		pev->dmgtime = gpGlobals->time + GetPulseInterval();
	}
	timedist = ( pev->dmgtime - gpGlobals->time ) / GetPulseInterval();

	if ( timedist < 0 )
		timedist = 0;
	else if ( timedist > 1 )
		timedist = 1;
	timedist = 1-timedist;

	UpdateEffect( tmpSrc, tr.vecEndPos, timedist );
}


void CDrainLife::UpdateEffect( const Vector &startPoint, const Vector &endPoint, float timeBlend )
{
	if ( !m_pBeam )
	{
		CreateEffect();
	}

	m_pBeam->SetStartPos( endPoint );
	m_pBeam->SetBrightness( 255 - (timeBlend*180) );
	m_pBeam->SetWidth( 40 - (timeBlend*20) );

	m_pBeam->SetColor( 120 + (25*timeBlend), 60 + (30*timeBlend), 64 + 80*fabs(sin(gpGlobals->time*10)) );

	UTIL_SetOrigin( m_pSprite, endPoint );
	m_pSprite->pev->frame += 8 * gpGlobals->frametime;
	if ( m_pSprite->pev->frame > m_pSprite->Frames() )
		m_pSprite->pev->frame = 0;

	m_pNoise->SetStartPos( endPoint );
}


void CDrainLife::CreateEffect( void )
{
	DestroyEffect();

	m_pBeam = CBeam::BeamCreate( DRAINLIFE_BEAM_SPRITE, 40 );
	m_pBeam->PointEntInit( pev->origin, m_pPlayer->entindex() );
	m_pBeam->SetFlags( BEAM_FSINE );
	m_pBeam->SetEndAttachment( 1 );
	m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;	// Flag these to be destroyed on save/restore or level transition

	m_pNoise = CBeam::BeamCreate( DRAINLIFE_BEAM_SPRITE, 55 );
	m_pNoise->PointEntInit( pev->origin, m_pPlayer->entindex() );
	m_pNoise->SetScrollRate( 25 );
	m_pNoise->SetBrightness( 100 );
	m_pNoise->SetEndAttachment( 1 );
	m_pNoise->pev->spawnflags |= SF_BEAM_TEMPORARY;

	m_pSprite = CSprite::SpriteCreate( DRAINLIFE_FLARE_SPRITE, pev->origin, FALSE );
	m_pSprite->pev->scale = 1.0;
	m_pSprite->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation );
	m_pSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;

	m_pBeam->SetScrollRate( 110 );
	m_pBeam->SetNoise( 5 );
	m_pNoise->SetColor( 255, 80, 120 );
	m_pNoise->SetNoise( 2 );
}


void CDrainLife::DestroyEffect( void )
{
	if ( m_pBeam )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}
	if ( m_pNoise )
	{
		UTIL_Remove( m_pNoise );
		m_pNoise = NULL;
	}
	if ( m_pSprite )
	{
		UTIL_Remove( m_pSprite );
		m_pSprite = NULL;
	}
}

void CDrainLife::WeaponIdle( void )
{
	ResetEmptySound( );

	if ( m_flTimeWeaponIdle > gpGlobals->time )
		return;

	if ( m_fireState != FIRE_OFF )
		EndAttack();


	int iAnim;

	float flRand = RANDOM_FLOAT(0,1);

	if ( flRand <= 0.4 )
	{
		iAnim = DRAINLIFE_IDLE1;
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT(2,3);
	}
	else if ( flRand <= 0.75 )
	{
		iAnim = DRAINLIFE_IDLE2;
		m_flTimeWeaponIdle = gpGlobals->time + 3;
	}
	else 
	{
		iAnim = DRAINLIFE_IDLE3;
		m_flTimeWeaponIdle = gpGlobals->time + 3;
	}

	SendWeaponAnim( iAnim );
	m_deployed = TRUE;
}

void CDrainLife::EndAttack( void )
{
	STOP_SOUND( ENT(m_pPlayer->pev), CHAN_STATIC, DRAINLIFE_SOUND_RUN );
	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, DRAINLIFE_SOUND_OFF, 0.2, ATTN_NORM, 0, 100); 
	m_fireState = FIRE_OFF;
	m_flTimeWeaponIdle = gpGlobals->time + 2.0;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + 0.5;
	DestroyEffect();
}



#endif