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

#include "Shrivelling.h"

#define	SHRIVELLING_PRIMARY_VOLUME		450
#define SHRIVELLING_BEAM_SPRITE			"sprites/xbeam1.spr"
#define SHRIVELLING_FLARE_SPRITE		"sprites/XSpark1.spr"
#define SHRIVELLING_SOUND_OFF			"weapons/egon_off1.wav"
#define SHRIVELLING_SOUND_RUN			"weapons/egon_run3.wav"
#define SHRIVELLING_SOUND_STARTUP		"weapons/egon_windup2.wav"

#define SHRIVELLING_SWITCH_NARROW_TIME			0.75			// Time it takes to switch fire modes
#define SHRIVELLING_SWITCH_WIDE_TIME			1.5


enum Shrivelling_e {
	SHRIVELLING_OPEN = 0,
	SHRIVELLING_IDLE1,
	SHRIVELLING_IDLE2,
	SHRIVELLING_IDLE3,
	SHRIVELLING_CAST,
	SHRIVELLING_CLOSE
};


LINK_ENTITY_TO_CLASS( weapon_shrivelling, CShrivelling );

TYPEDESCRIPTION	CShrivelling::m_SaveData[] = 
{
	DEFINE_FIELD( CShrivelling, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CShrivelling, m_pNoise, FIELD_CLASSPTR ),
	DEFINE_FIELD( CShrivelling, m_pSprite, FIELD_CLASSPTR ),
	DEFINE_FIELD( CShrivelling, m_shootTime, FIELD_TIME ),
	DEFINE_FIELD( CShrivelling, m_fireState, FIELD_INTEGER ),
	DEFINE_FIELD( CShrivelling, m_fireMode, FIELD_INTEGER ),
	DEFINE_FIELD( CShrivelling, m_shakeTime, FIELD_TIME ),
	DEFINE_FIELD( CShrivelling, m_flAmmoUseTime, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CShrivelling, CBasePlayerWeapon );


void CShrivelling::Spawn( )
{
	Precache( );
	m_iId = WEAPON_SHRIVELLING;
	SET_MODEL(ENT(pev), "models/w_shrivelling.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CShrivelling::Precache( void )
{
	PRECACHE_MODEL("models/w_shrivelling.mdl");
	PRECACHE_MODEL("models/v_shrivelling.mdl");
//	PRECACHE_MODEL("models/p_shrivelling.mdl");

	PRECACHE_MODEL("models/w_9mmclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND( SHRIVELLING_SOUND_OFF );
	PRECACHE_SOUND( SHRIVELLING_SOUND_RUN );
	PRECACHE_SOUND( SHRIVELLING_SOUND_STARTUP );

	PRECACHE_MODEL( SHRIVELLING_BEAM_SPRITE );
	PRECACHE_MODEL( SHRIVELLING_FLARE_SPRITE );

	PRECACHE_SOUND ("weapons/357_cock1.wav");
}


BOOL CShrivelling::Deploy( void )
{
	m_deployed = FALSE;
	return DefaultDeploy( "models/v_shrivelling.mdl", "", SHRIVELLING_OPEN, "shrivelling" );
}

int CShrivelling::AddToPlayer( CBasePlayer *pPlayer )
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



void CShrivelling::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	// m_flTimeWeaponIdle = gpGlobals->time + UTIL_RandomFloat ( 10, 15 );
	SendWeaponAnim( SHRIVELLING_CLOSE );

	if ( m_fireState != FIRE_OFF )
		EndAttack();
}

int CShrivelling::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_SHRIVELLING;
	p->iFlags = 0;
	p->iWeight = SHRIVELLING_WEIGHT;

	return 1;
}


//#define SHRIVELLING_PULSE_INTERVAL			0.25
//#define SHRIVELLING_DISCHARGE_INTERVAL		0.5

#define SHRIVELLING_PULSE_INTERVAL			0.1
#define SHRIVELLING_DISCHARGE_INTERVAL		0.1

float CShrivelling::GetPulseInterval( void )
{
	if ( g_pGameRules->IsMultiplayer() )
	{
		return 0.1;
	}

	return SHRIVELLING_PULSE_INTERVAL;
}

float CShrivelling::GetDischargeInterval( void )
{
	if ( g_pGameRules->IsMultiplayer() )
	{
		return 0.1;
	}

	return SHRIVELLING_DISCHARGE_INTERVAL;
}

void CShrivelling::Attack( void )
{
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );

	switch( m_fireState )
	{
		case FIRE_OFF:
		{
			m_flAmmoUseTime = gpGlobals->time;// start using ammo ASAP.

			SendWeaponAnim( SHRIVELLING_CAST );
			m_shakeTime = 0;

			m_pPlayer->m_iWeaponVolume = SHRIVELLING_PRIMARY_VOLUME;
			m_flTimeWeaponIdle = gpGlobals->time + 0.1;
			m_shootTime = gpGlobals->time + 2;

			if ( m_fireMode == FIRE_WIDE )
			{
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, SHRIVELLING_SOUND_STARTUP, 0.98, ATTN_NORM, 0, 125 );
			}
			else
			{
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, SHRIVELLING_SOUND_STARTUP, 0.9, ATTN_NORM, 0, 100 );
			}

			pev->dmgtime = gpGlobals->time + GetPulseInterval();
			m_fireState = FIRE_CHARGE;
		}
		break;

		case FIRE_CHARGE:
		{
			Fire( vecSrc, vecAiming );
			m_pPlayer->m_iWeaponVolume = SHRIVELLING_PRIMARY_VOLUME;

			if ( m_shootTime != 0 && gpGlobals->time > m_shootTime )
			{
				if ( m_fireMode == FIRE_WIDE )
				{
					EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_STATIC, SHRIVELLING_SOUND_RUN, 0.98, ATTN_NORM, 0, 125 );
				}
				else
				{
					EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_STATIC, SHRIVELLING_SOUND_RUN, 0.9, ATTN_NORM, 0, 100 );
				}

				m_shootTime = 0;
			}
		}
		break;
	}
}

void CShrivelling::PrimaryAttack( void )
{
	m_fireMode = FIRE_WIDE;
	Attack();
}

void CShrivelling::Fire( const Vector &vecOrigSrc, const Vector &vecDir )
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

	switch ( m_fireMode )
	{
	case FIRE_NARROW:
		if ( pev->dmgtime < gpGlobals->time )
		{
			// Narrow mode only does damage to the entity it hits
			ClearMultiDamage();
			if (pEntity->pev->takedamage)
			{
				pEntity->TraceAttack( m_pPlayer->pev, gSkillData.plrDmgShrivellingNarrow, vecDir, &tr, DMG_ENERGYBEAM );
			}
			ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);

			if ( g_pGameRules->IsMultiplayer() )
			{
				// multiplayer uses 1 ammo every 1/10th second
				if ( gpGlobals->time >= m_flAmmoUseTime )
				{
					m_pPlayer->LoseSanity(1);
					m_flAmmoUseTime = gpGlobals->time + 0.1;
				}
			}
			else
			{
				// single player, use 3 ammo/second
				if ( gpGlobals->time >= m_flAmmoUseTime )
				{
					m_pPlayer->LoseSanity(1);
					m_flAmmoUseTime = gpGlobals->time + 0.166;
				}
			}

			pev->dmgtime = gpGlobals->time + GetPulseInterval();
		}
		timedist = ( pev->dmgtime - gpGlobals->time ) / GetPulseInterval();
		break;
	
	case FIRE_WIDE:
		if ( pev->dmgtime < gpGlobals->time )
		{
			// wide mode does damage to the ent, and radius damage
			ClearMultiDamage();
			if (pEntity->pev->takedamage)
			{
				pEntity->TraceAttack( m_pPlayer->pev, gSkillData.plrDmgShrivellingWide, vecDir, &tr, DMG_ENERGYBEAM | DMG_ALWAYSGIB);
			}
			ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);

			if ( g_pGameRules->IsMultiplayer() )
			{
				// radius damage a little more potent in multiplayer.
				::RadiusDamage( tr.vecEndPos, pev, m_pPlayer->pev, gSkillData.plrDmgShrivellingWide/4, 128, CLASS_NONE, DMG_ENERGYBEAM | DMG_BLAST | DMG_ALWAYSGIB );
			}

			if ( !m_pPlayer->IsAlive() )
				return;

			if ( g_pGameRules->IsMultiplayer() )
			{
				//multiplayer uses 5 ammo/second
				if ( gpGlobals->time >= m_flAmmoUseTime )
				{
					m_pPlayer->LoseSanity(1);
					m_flAmmoUseTime = gpGlobals->time + 0.2;
				}
			}
			else
			{
				// Wide mode uses 10 charges per second in single player
				if ( gpGlobals->time >= m_flAmmoUseTime )
				{
					m_pPlayer->LoseSanity(1);
					m_flAmmoUseTime = gpGlobals->time + 0.1;
				}
			}

			pev->dmgtime = gpGlobals->time + GetDischargeInterval();
			if ( m_shakeTime < gpGlobals->time )
			{
				UTIL_ScreenShake( tr.vecEndPos, 5.0, 150.0, 0.75, 250.0 );
				m_shakeTime = gpGlobals->time + 1.5;
			}
		}
		timedist = ( pev->dmgtime - gpGlobals->time ) / GetDischargeInterval();
		break;
	}

	if ( timedist < 0 )
		timedist = 0;
	else if ( timedist > 1 )
		timedist = 1;
	timedist = 1-timedist;

	UpdateEffect( tmpSrc, tr.vecEndPos, timedist );
}


void CShrivelling::UpdateEffect( const Vector &startPoint, const Vector &endPoint, float timeBlend )
{
	if ( !m_pBeam )
	{
		CreateEffect();
	}

	m_pBeam->SetStartPos( endPoint );
	m_pBeam->SetBrightness( 255 - (timeBlend*180) );
	m_pBeam->SetWidth( 40 - (timeBlend*20) );

	if ( m_fireMode == FIRE_WIDE )
		m_pBeam->SetColor( 30 + (25*timeBlend), 30 + (30*timeBlend), 64 + 80*fabs(sin(gpGlobals->time*10)) );
	else
		m_pBeam->SetColor( 60 + (25*timeBlend), 120 + (30*timeBlend), 64 + 80*fabs(sin(gpGlobals->time*10)) );


	UTIL_SetOrigin( m_pSprite, endPoint );
	m_pSprite->pev->frame += 8 * gpGlobals->frametime;
	if ( m_pSprite->pev->frame > m_pSprite->Frames() )
		m_pSprite->pev->frame = 0;

	m_pNoise->SetStartPos( endPoint );
}


void CShrivelling::CreateEffect( void )
{
	DestroyEffect();

	m_pBeam = CBeam::BeamCreate( SHRIVELLING_BEAM_SPRITE, 40 );
	m_pBeam->PointEntInit( pev->origin, m_pPlayer->entindex() );
	m_pBeam->SetFlags( BEAM_FSINE );
	m_pBeam->SetEndAttachment( 1 );
	m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;	// Flag these to be destroyed on save/restore or level transition

	m_pNoise = CBeam::BeamCreate( SHRIVELLING_BEAM_SPRITE, 55 );
	m_pNoise->PointEntInit( pev->origin, m_pPlayer->entindex() );
	m_pNoise->SetScrollRate( 25 );
	m_pNoise->SetBrightness( 100 );
	m_pNoise->SetEndAttachment( 1 );
	m_pNoise->pev->spawnflags |= SF_BEAM_TEMPORARY;

	m_pSprite = CSprite::SpriteCreate( SHRIVELLING_FLARE_SPRITE, pev->origin, FALSE );
	m_pSprite->pev->scale = 1.0;
	m_pSprite->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation );
	m_pSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;

	if ( m_fireMode == FIRE_WIDE )
	{
		m_pBeam->SetScrollRate( 50 );
		m_pBeam->SetNoise( 20 );
		m_pNoise->SetColor( 50, 255, 50 );
		m_pNoise->SetNoise( 8 );
	}
	else
	{
		m_pBeam->SetScrollRate( 110 );
		m_pBeam->SetNoise( 5 );
		m_pNoise->SetColor( 80, 255, 120 );
		m_pNoise->SetNoise( 2 );
	}
}


void CShrivelling::DestroyEffect( void )
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
		if ( m_fireMode == FIRE_WIDE )
			m_pSprite->Expand( 10, 500 );
		else
			UTIL_Remove( m_pSprite );
		m_pSprite = NULL;
	}
}

void CShrivelling::WeaponIdle( void )
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
		iAnim = SHRIVELLING_IDLE1;
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT(2,3);
	}
	else if ( flRand <= 0.75 )
	{
		iAnim = SHRIVELLING_IDLE2;
		m_flTimeWeaponIdle = gpGlobals->time + 3;
	}
	else 
	{
		iAnim = SHRIVELLING_IDLE3;
		m_flTimeWeaponIdle = gpGlobals->time + 3;
	}

	SendWeaponAnim( iAnim );
	m_deployed = TRUE;
}

void CShrivelling::EndAttack( void )
{
	STOP_SOUND( ENT(m_pPlayer->pev), CHAN_STATIC, SHRIVELLING_SOUND_RUN );
	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, SHRIVELLING_SOUND_OFF, 0.98, ATTN_NORM, 0, 100); 
	m_fireState = FIRE_OFF;
	m_flTimeWeaponIdle = gpGlobals->time + 2.0;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + 0.5;
	DestroyEffect();
}



#endif