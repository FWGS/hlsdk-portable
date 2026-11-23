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
#include "monsters.h"
#include "weapons.h"
#include "effects.h"

class CTripmineGrenade : public CGrenade
{
	void Spawn() override;
	void Precache() override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	int TakeDamage( entvars_t* inflictor, entvars_t* attacker, float damage, int damageType ) override;
	void EXPORT WarningThink();
	void EXPORT PowerupThink();
	void EXPORT BeamBreakThink();
	void EXPORT DelayDeathThink();
	void Killed( entvars_t* attacker, int gib ) override;
	void MakeBeam();
	void KillBeam();
	float m_flPowerUp;
	Vector m_vecDir;
	Vector m_vecEnd;
	float m_flBeamLength;
	EHANDLE m_hOwner;
	CBeam* m_pBeam;
	Vector m_posOwner;
	Vector m_angleOwner;
	edict_t* m_pRealOwner; // tracelines don't hit PEV->OWNER, which means a player couldn't detonate his own trip mine, so we store the owner here.
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_tripmine, CTripmineGrenade );

TYPEDESCRIPTION CTripmineGrenade::m_SaveData[] =
{
	DEFINE_FIELD( CTripmineGrenade, m_flPowerUp, FIELD_TIME ),
	DEFINE_FIELD( CTripmineGrenade, m_vecDir, FIELD_VECTOR ),
	DEFINE_FIELD( CTripmineGrenade, m_vecEnd, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CTripmineGrenade, m_flBeamLength, FIELD_FLOAT ),
	DEFINE_FIELD( CTripmineGrenade, m_hOwner, FIELD_EHANDLE ),
	DEFINE_FIELD( CTripmineGrenade, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CTripmineGrenade, m_posOwner, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CTripmineGrenade, m_angleOwner, FIELD_VECTOR ),
	DEFINE_FIELD( CTripmineGrenade, m_pRealOwner, FIELD_EDICT )
};

IMPLEMENT_SAVERESTORE( CTripmineGrenade, CGrenade );

void CTripmineGrenade::Precache()
{
	PRECACHE_MODEL( "models/v_tripmine.mdl" );
	PRECACHE_SOUND( "ambience/trip_cord.wav" );
}

void CTripmineGrenade::Spawn()
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_NOT;
	SET_MODEL( edict(), "models/v_tripmine.mdl" );
	pev->frame = 0;
	pev->body = 3;
	pev->sequence = 7;
	ResetSequenceInfo();
	pev->framerate = 0;
	UTIL_SetSize( pev, Vector( -8, -8, -8 ), Vector( 8, 8, 8 ) );
	UTIL_SetOrigin( pev, pev->origin );
	if ( pev->spawnflags & 1 )
		m_flPowerUp = gpGlobals->time + 1.0;
	else
		m_flPowerUp = gpGlobals->time + 2.5;
	SetThink( &CTripmineGrenade::PowerupThink );
	pev->nextthink = gpGlobals->time + 0.2;
	pev->takedamage = DAMAGE_YES;
	pev->dmg = 100;
	pev->health = 1; // don't let die normally
	if ( pev->owner )
		m_pRealOwner = pev->owner; // see CTripmineGrenade for why.
	UTIL_MakeAimVectors( pev->angles );
	m_vecDir = gpGlobals->v_forward;
	m_vecEnd = pev->origin + m_vecDir * 2048;
}

int CTripmineGrenade::TakeDamage( entvars_t* inflictor, entvars_t* attacker, float damage, int damageType )
{
	if ( m_flPowerUp <= gpGlobals->time || pev->health <= damage )
		return CBaseMonster::TakeDamage( inflictor, attacker, damage, damageType );
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1;
	KillBeam();
	return 0;
}

void CTripmineGrenade::Killed( entvars_t* attacker, int )
{
	pev->takedamage = DAMAGE_NO;
	if ( attacker && (attacker->flags & FL_CLIENT) )
		pev->owner = ENT( attacker );
	SetThink( &CTripmineGrenade::DelayDeathThink );
	pev->nextthink = gpGlobals->time + 0.35;
	EMIT_SOUND( edict(), CHAN_BODY, "common/null.wav", 0.5f, ATTN_NORM );
	EMIT_SOUND( edict(), CHAN_BODY, "ambience/trip_cord.wav", 1, ATTN_NORM );
}

void CTripmineGrenade::WarningThink()
{
	// play warning sound
	// EMIT_SOUND( ENT(pev), CHAN_VOICE, "buttons/Blip2.wav", 1.0, ATTN_NORM );

	// set to power up
	SetThink( &CTripmineGrenade::PowerupThink );
	pev->nextthink = gpGlobals->time + 1.0;
}

void CTripmineGrenade::PowerupThink()
{
	if ( !m_hOwner )
	{
		edict_t* owner = pev->owner;
		pev->owner = NULL;
		TraceResult trace;
		UTIL_TraceLine( pev->origin + m_vecDir * 8, pev->origin - m_vecDir * 32,
			dont_ignore_monsters, edict(), &trace );
		if ( trace.fStartSolid || (owner && owner == trace.pHit) )
		{
			pev->owner = owner;
			m_flPowerUp += 0.1;
			pev->nextthink = gpGlobals->time + 0.1f;
			return;
		}
		if ( trace.flFraction >= 1 )
		{
			STOP_SOUND( edict(), CHAN_VOICE, "weapons/mine_deploy.wav" );
			STOP_SOUND( edict(), CHAN_BODY, "weapons/mine_charge.wav" );
			SetThink( &CBaseEntity::SUB_Remove );
			pev->nextthink = gpGlobals->time + 0.1;
			ALERT( at_console, "WARNING:Tripmine at %.0f, %.0f, %.0f removed\n", pev->origin.x, pev->origin.y, pev->origin.z );
			KillBeam();
			return;
		}
		pev->owner = trace.pHit;
		m_hOwner = CBaseEntity::Instance( pev->owner );
		m_posOwner = m_hOwner->pev->origin;
		m_angleOwner = m_hOwner->pev->angles;
	}
	else if ( m_posOwner != m_hOwner->pev->origin || m_angleOwner != m_hOwner->pev->angles )
	{
		STOP_SOUND( edict(), CHAN_VOICE, "weapons/mine_deploy.wav" );
		STOP_SOUND( edict(), CHAN_BODY, "weapons/mine_charge.wav" );
		CBaseEntity* mine = CBaseEntity::Create( "weapon_tripmine", pev->origin + m_vecDir * 24, pev->angles );
		mine->pev->spawnflags |= SF_NORESPAWN;
		SetThink( &CBaseEntity::SUB_Remove );
		KillBeam();
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}
	// ALERT( at_console, "%d %.0f %.0f %0.f\n", pev->owner, m_pOwner->pev->origin.x, m_pOwner->pev->origin.y, m_pOwner->pev->origin.z );

	if ( gpGlobals->time > m_flPowerUp )
	{
		// make solid
		pev->solid = SOLID_BBOX;
		UTIL_SetOrigin( pev, pev->origin );
		MakeBeam();
	}
	pev->nextthink = gpGlobals->time + 0.1;
}

void CTripmineGrenade::MakeBeam()
{
	TraceResult trace;
	UTIL_TraceLine( pev->origin, m_vecEnd, dont_ignore_monsters, edict(), &trace );

	// set to follow laser spot
	SetThink( &CTripmineGrenade::BeamBreakThink );
	m_flBeamLength = trace.flFraction;
	pev->nextthink = gpGlobals->time + 0.1;
	Vector end = pev->origin + m_vecDir * 2048 * m_flBeamLength;
	m_pBeam = CBeam::BeamCreate( g_pModelNameLaser, 2 );
	m_pBeam->PointEntInit( end, entindex() );
	m_pBeam->SetColor( 128, 128, 128 );
	m_pBeam->SetScrollRate( 255 );
	m_pBeam->SetBrightness( 64 );
}

void CTripmineGrenade::KillBeam()
{
	if ( m_pBeam )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}
}

void CTripmineGrenade::BeamBreakThink()
{

	// HACKHACK Set simple box using this really nice global!
	gpGlobals->trace_flags = FTRACE_SIMPLEBOX;
	TraceResult trace;
	UTIL_TraceLine( pev->origin, m_vecEnd, dont_ignore_monsters, edict(), &trace );

	// ALERT( at_console, "%f : %f\n", tr.flFraction, m_flBeamLength );

	// respawn detect. 
	if ( !m_pBeam )
	{
		MakeBeam();
		if ( trace.pHit )
			m_hOwner = CBaseEntity::Instance( trace.pHit );
	}
	if ( fabs( m_flBeamLength - trace.flFraction ) <= 0.001 && m_hOwner
		&& m_posOwner == m_hOwner->pev->origin && m_angleOwner == m_hOwner->pev->angles )
	{
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}
	// a bit of a hack, but all CGrenade code passes pev->owner along to make sure the proper player gets credit for the kill
	// so we have to restore pev->owner from pRealOwner, because an entity's tracelines don't strike it's pev->owner which meant
	// that a player couldn't trigger his own tripmine. Now that the mine is exploding, it's safe the restore the owner so the 
	// CGrenade code knows who the explosive really belongs to.
	pev->owner = m_pRealOwner;
	pev->health = 0;
	Killed( VARS( pev->owner ), GIB_NORMAL );
}

void CTripmineGrenade::DelayDeathThink()
{
	KillBeam();
	TraceResult trace;
	UTIL_TraceLine( pev->origin + m_vecDir * 8, pev->origin - m_vecDir * 64,
		dont_ignore_monsters, edict(), &trace );
	Explode3( &trace, DMG_BLAST );
}
