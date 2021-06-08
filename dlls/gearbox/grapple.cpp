/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
#include "weapons.h"
#include "gamerules.h"
#include "player.h"
#include "skill.h"
#include "effects.h"
#include "customentity.h"

#if !CLIENT_DLL
#include "grapple_tonguetip.h"
#endif

LINK_ENTITY_TO_CLASS( weapon_grapple, CBarnacleGrapple )

enum bgrap_e {
	BGRAPPLE_BREATHE = 0,
	BGRAPPLE_LONGIDLE,
	BGRAPPLE_SHORTIDLE,
	BGRAPPLE_COUGH,
	BGRAPPLE_DOWN,
	BGRAPPLE_UP,
	BGRAPPLE_FIRE,
	BGRAPPLE_FIREWAITING,
	BGRAPPLE_FIREREACHED,
	BGRAPPLE_FIRETRAVEL,
	BGRAPPLE_FIRERELEASE
};

void CBarnacleGrapple::Precache( void )
{
	PRECACHE_MODEL( "models/v_bgrap.mdl" );
	PRECACHE_MODEL( "models/w_bgrap.mdl" );
	PRECACHE_MODEL( "models/p_bgrap.mdl" );

	PRECACHE_SOUND( "weapons/bgrapple_release.wav" );
	PRECACHE_SOUND( "weapons/bgrapple_impact.wav" );
	PRECACHE_SOUND( "weapons/bgrapple_fire.wav" );
	PRECACHE_SOUND( "weapons/bgrapple_cough.wav" );
	PRECACHE_SOUND( "weapons/bgrapple_pull.wav" );
	PRECACHE_SOUND( "weapons/bgrapple_wait.wav" );
	PRECACHE_SOUND( "weapons/alienweap_draw.wav" );
	PRECACHE_SOUND( "barnacle/bcl_chew1.wav" );
	PRECACHE_SOUND( "barnacle/bcl_chew2.wav" );
	PRECACHE_SOUND( "barnacle/bcl_chew3.wav" );

	PRECACHE_MODEL( "sprites/tongue.spr" );

	UTIL_PrecacheOther( "grapple_tip" );
}

void CBarnacleGrapple::Spawn( void )
{
	Precache();
	m_iId = WEAPON_GRAPPLE;
	SET_MODEL( ENT(pev), "models/w_bgrap.mdl" );
	m_pTip = NULL;
	m_bGrappling = FALSE;
	m_iClip = -1;

	FallInit();
}

int CBarnacleGrapple::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_GRAPPLE;
	p->iWeight = GRAPPLE_WEIGHT;
	return 1;
}

int CBarnacleGrapple::AddToPlayer( CBasePlayer* pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL CBarnacleGrapple::Deploy()
{
	int r = DefaultDeploy("models/v_bgrap.mdl", "models/p_bgrap.mdl", BGRAPPLE_UP, "gauss" );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.1;
	return r;
}

void CBarnacleGrapple::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;

	if( m_fireState != OFF )
		EndAttack();

	SendWeaponAnim( BGRAPPLE_DOWN );
}

void CBarnacleGrapple::WeaponIdle( void )
{
	ResetEmptySound();

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if( m_fireState != OFF )
	{
		EndAttack();
		return;
	}

	m_bMissed = FALSE;

	const float flNextIdle = RANDOM_FLOAT( 0.0, 1.0 );

	int iAnim;

	if( flNextIdle <= 0.5 )
	{
		iAnim = BGRAPPLE_LONGIDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 10.0;
	}
	else if( flNextIdle > 0.95 )
	{
		EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_STATIC, "weapons/bgrapple_cough.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM );

		iAnim = BGRAPPLE_COUGH;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4.6;
	}
	else
	{
		iAnim = BGRAPPLE_BREATHE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.566;
	}

	SendWeaponAnim( iAnim );
}

void CBarnacleGrapple::PrimaryAttack( void )
{
	if( m_bMissed )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;
		return;
	}

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
#if !CLIENT_DLL
	if( m_pTip )
	{
		if( m_pTip->IsStuck() )
		{
			CBaseEntity* pTarget = m_pTip->GetGrappleTarget();

			if( !pTarget )
			{
				EndAttack();
				return;
			}

			if( m_pTip->GetGrappleType() > GRAPPLE_SMALL )
			{
				m_pPlayer->pev->movetype = MOVETYPE_FLY;
				m_pPlayer->pev->flags |= FL_IMMUNE_SLIME;
				//Tells the physics code that the player is not on a ladder - Solokiller
			}

			if( m_bMomentaryStuck )
			{
				SendWeaponAnim( BGRAPPLE_FIRETRAVEL );

				EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_STATIC, "weapons/bgrapple_impact.wav", 0.98, ATTN_NORM, 0, 125 );

				if( pTarget->IsPlayer() )
				{
					EMIT_SOUND_DYN( ENT(pTarget->pev), CHAN_STATIC,"weapons/bgrapple_impact.wav", 0.98, ATTN_NORM, 0, 125 );
				}

				m_bMomentaryStuck = FALSE;
			}

			switch( m_pTip->GetGrappleType() )
			{
			case GRAPPLE_NOT_A_TARGET: break;

			case GRAPPLE_SMALL:
				//pTarget->BarnacleVictimGrabbed( this );
				m_pTip->pev->origin = pTarget->Center();

				pTarget->pev->velocity = pTarget->pev->velocity + ( m_pPlayer->pev->origin - pTarget->pev->origin );

				if( pTarget->pev->velocity.Length() > 450.0 )
				{
					pTarget->pev->velocity = pTarget->pev->velocity.Normalize() * 450.0;
				}

				break;

			case GRAPPLE_MEDIUM:
			case GRAPPLE_LARGE:
			case GRAPPLE_FIXED:
				//pTarget->BarnacleVictimGrabbed( this );

				if( m_pTip->GetGrappleType() != GRAPPLE_FIXED )
					UTIL_SetOrigin( m_pTip->pev, pTarget->Center() );

				m_pPlayer->pev->velocity =
					m_pPlayer->pev->velocity + ( m_pTip->pev->origin - m_pPlayer->pev->origin );

				if( m_pPlayer->pev->velocity.Length() > 450.0 )
				{
					m_pPlayer->pev->velocity = m_pPlayer->pev->velocity.Normalize() * 450.0;

					Vector vecPitch = UTIL_VecToAngles( m_pPlayer->pev->velocity );

					if( (vecPitch.x > 55.0 && vecPitch.x < 205.0) || vecPitch.x < -55.0 )
					{
						m_bGrappling = FALSE;
						m_pPlayer->SetAnimation( PLAYER_IDLE );
					}
					else
					{
						if (!m_bGrappling)
							EMIT_SOUND_DYN(  ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/bgrapple_pull.wav", 0.98, ATTN_NORM, 0, 125 );
						m_bGrappling = TRUE;
						m_pPlayer->m_afPhysicsFlags |= PFLAG_LATCHING;
					}
				}
				else
				{
					m_bGrappling = FALSE;
					m_pPlayer->SetAnimation( PLAYER_IDLE );
				}

				break;
			}
		}

		if( m_pTip->HasMissed() )
		{
			EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/bgrapple_release.wav", 0.98, ATTN_NORM, 0, 125 );

			EndAttack();
			return;
		}
	}
#endif
	if( m_fireState != OFF )
	{
		m_pPlayer->m_iWeaponVolume = 450;

		if( m_flShootTime != 0.0 && gpGlobals->time > m_flShootTime )
		{
			SendWeaponAnim( BGRAPPLE_FIREWAITING );

			Vector vecPunchAngle = m_pPlayer->pev->punchangle;

			vecPunchAngle.x += 2.0;

			m_pPlayer->pev->punchangle = vecPunchAngle;

			Fire( m_pPlayer->GetGunPosition(), gpGlobals->v_forward );
			EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/bgrapple_pull.wav", 0.98, ATTN_NORM, 0, 125 );
			m_flShootTime = 0;
		}
	}
	else
	{
		m_bMomentaryStuck = TRUE;

		SendWeaponAnim( BGRAPPLE_FIRE );

		m_pPlayer->m_iWeaponVolume = 450;

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;
#if !CLIENT_DLL
		if( g_pGameRules->IsMultiplayer() )
		{
			m_flShootTime = gpGlobals->time;
		}
		else
		{
			m_flShootTime = gpGlobals->time + 0.35;
		}
#endif
		EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/bgrapple_fire.wav", 0.98, ATTN_NORM, 0, 125 );
		m_fireState = CHARGE;
	}

	if( !m_pTip )
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;
		return;
	}

#if !CLIENT_DLL
	if( m_pTip->GetGrappleType() != GRAPPLE_FIXED && m_pTip->IsStuck() )
	{
		UTIL_MakeVectors( m_pPlayer->pev->v_angle );

		Vector vecSrc = m_pPlayer->GetGunPosition();

		Vector vecEnd = vecSrc + gpGlobals->v_forward * 16.0;

		TraceResult tr;

		UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, m_pPlayer->edict(), &tr );

		if( tr.flFraction >= 1.0 )
		{
			UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, m_pPlayer->edict(), &tr );
			if( tr.flFraction < 1.0 )
			{
				CBaseEntity* pHit = Instance( tr.pHit );
/*
				if( !pHit )
					pHit = CWorld::GetInstance();

				if( !pHit )
				{
					FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer );
				}
*/
			}
		}

		if( tr.flFraction < 1.0 )
		{
			CBaseEntity* pHit = Instance( tr.pHit );
/*
			if( !pHit )
				pHit = CWorld::GetInstance();
*/
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

			if( pHit )
			{
				if( m_pTip )
				{
					bool bValidTarget = FALSE;
#if !CLIENT_DLL
					if( pHit->IsPlayer() )
					{
						m_pTip->SetGrappleTarget( pHit );
						bValidTarget = TRUE;
					}
					else if( m_pTip->CheckTarget( pHit ) != GRAPPLE_NOT_A_TARGET )
					{
						bValidTarget = TRUE;
					}
#endif
					if( bValidTarget )
					{
						if( m_flDamageTime + 0.5 < gpGlobals->time )
						{
#if !CLIENT_DLL
							ClearMultiDamage();

							float flDamage = gSkillData.plrDmgGrapple;

							if( g_pGameRules->IsMultiplayer() )
							{
								flDamage *= 2;
							}

							pHit->TraceAttack( m_pPlayer->pev, flDamage, gpGlobals->v_forward, &tr, DMG_CLUB );

							ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );
#endif

							m_flDamageTime = gpGlobals->time;

							const char* pszSample;

							switch( RANDOM_LONG( 0, 2 ) )
							{
							default:
							case 0: pszSample = "barnacle/bcl_chew1.wav"; break;
							case 1: pszSample = "barnacle/bcl_chew2.wav"; break;
							case 2: pszSample = "barnacle/bcl_chew3.wav"; break;
							}
							EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_VOICE, pszSample, VOL_NORM, ATTN_NORM, 0, 125 );
						}
					}
				}
			}
		}
	}
#endif

	//TODO: CTF support - Solokiller
	/*
	if( g_pGameRules->IsMultiplayer() && g_pGameRules->IsCTF() )
	{
		m_flNextPrimaryAttack = gpGlobals->time;
	}
	else
	*/
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.01;
	}
}

void CBarnacleGrapple::Fire( Vector vecOrigin, Vector vecDir )
{
#if !CLIENT_DLL
	Vector vecSrc = vecOrigin;

	Vector vecEnd = vecSrc + vecDir * 2048.0;

	TraceResult tr;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, m_pPlayer->edict(), &tr );

	if( !tr.fAllSolid )
	{
		CBaseEntity* pHit = Instance( tr.pHit );
/*
		if( !pHit )
			pHit = CWorld::GetInstance();
*/
		if( pHit )
		{
			UpdateEffect();

			m_flDamageTime = gpGlobals->time;
		}
	}
#endif
}

void CBarnacleGrapple::EndAttack( void )
{
	m_fireState = OFF;
	SendWeaponAnim( BGRAPPLE_FIRERELEASE );

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/bgrapple_release.wav", 1, ATTN_NORM);

	EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/bgrapple_pull.wav", 0.0, ATTN_NONE, SND_STOP, 100 );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.9;

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.01;

	DestroyEffect();

	if( m_bGrappling && m_pPlayer->IsAlive() )
	{
		m_pPlayer->SetAnimation( PLAYER_IDLE );
	}

	m_pPlayer->pev->movetype = MOVETYPE_WALK;
	m_pPlayer->pev->flags &= ~(FL_IMMUNE_SLIME);
	m_pPlayer->m_afPhysicsFlags &= ~PFLAG_LATCHING;
}

void CBarnacleGrapple::CreateEffect( void )
{
#if !CLIENT_DLL
	DestroyEffect();

	m_pTip = GetClassPtr((CBarnacleGrappleTip *)NULL);
	m_pTip->Spawn();

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );

	Vector vecOrigin =
		m_pPlayer->GetGunPosition() +
		gpGlobals->v_forward * 16.0 +
		gpGlobals->v_right * 8.0 +
		gpGlobals->v_up * -8.0;

	Vector vecAngles = m_pPlayer->pev->v_angle;

	vecAngles.x = -vecAngles.x;

	m_pTip->SetPosition( vecOrigin, vecAngles, m_pPlayer );

	if( !m_pBeam )
	{
		m_pBeam = CBeam::BeamCreate( "sprites/tongue.spr", 16 );

		m_pBeam->EntsInit( m_pTip->entindex(), m_pPlayer->entindex() );

		m_pBeam->SetFlags( BEAM_FSOLID );

		m_pBeam->SetBrightness( 100.0 );

		m_pBeam->SetEndAttachment( 1 );

		m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;
	}
#endif
}

void CBarnacleGrapple::UpdateEffect( void )
{
#if !CLIENT_DLL
	if( !m_pBeam || !m_pTip )
		CreateEffect();
#endif
}

void CBarnacleGrapple::DestroyEffect( void )
{
	if( m_pBeam )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}
#if !CLIENT_DLL
	if( m_pTip )
	{
		m_pTip->Killed( NULL, GIB_NEVER );
		m_pTip = NULL;
	}
#endif
}
