/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// Alien slave monster
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"squadmonster.h"
#include	"schedule.h"
#include	"effects.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"islave.h"

class CPanther : public CISlave
{
public:
	void Spawn( void );
	void Precache( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	void DeathSound( void );
	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );

	void ClearBeams();
	void ArmBeam( int side );
	void WackBeam( int side, CBaseEntity *pEntity );
	void ZapBeam( int side );

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
	static const char *pPainSounds[];
	static const char *pDeathSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
};

LINK_ENTITY_TO_CLASS( monster_alien_panther, CPanther )

const char *CPanther::pAttackHitSounds[] =
{
	"panther/pclaw_strike1.wav",
	"panther/pclaw_strike2.wav",
	"panther/pclaw_strike3.wav",
};

const char *CPanther::pAttackMissSounds[] =
{
	"panther/pclaw_miss1.wav",
	"panther/pclaw_miss2.wav",
};

const char *CPanther::pPainSounds[] =
{
	"panther/p_pain1.wav",
	"panther/p_pain2.wav",
};

const char *CPanther::pDeathSounds[] =
{
	"panther/p_die1.wav",
	"panther/p_die2.wav",
};

const char *CPanther::pIdleSounds[] =
{
	"panther/p_idle1.wav",
	"panther/p_idle2.wav",
};

const char *CPanther::pAlertSounds[] =
{
	"panther/p_alert1.wav",
	"panther/p_alert2.wav",
};

//=========================================================
// ALertSound - scream
//=========================================================
void CPanther::AlertSound( void )
{
	if( m_hEnemy != 0 )
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pAlertSounds[RANDOM_LONG( 0, ARRAYSIZE( pDeathSounds ) - 1 )], 1.0, ATTN_NORM, 0, m_voicePitch );
	}
}

//=========================================================
// IdleSound
//=========================================================
void CPanther::IdleSound( void )
{
	if( RANDOM_LONG( 0, 2 ) == 0 )
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pIdleSounds[RANDOM_LONG( 0, ARRAYSIZE( pDeathSounds ) - 1 )], 1.0, ATTN_NORM, 0, m_voicePitch );
	}
#if 0
	int side = RANDOM_LONG( 0, 1 ) * 2 - 1;

	ClearBeams();
	ArmBeam( side );

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = pev->origin + gpGlobals->v_right * 2 * side;
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
		WRITE_BYTE( TE_DLIGHT );
		WRITE_COORD( vecSrc.x );	// X
		WRITE_COORD( vecSrc.y );	// Y
		WRITE_COORD( vecSrc.z );	// Z
		WRITE_BYTE( 8 );		// radius * 0.1
		WRITE_BYTE( 255 );		// r
		WRITE_BYTE( 180 );		// g
		WRITE_BYTE( 96 );		// b
		WRITE_BYTE( 10 );		// time * 10
		WRITE_BYTE( 0 );		// decay * 0.1
	MESSAGE_END();

	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "debris/zap1.wav", 1, ATTN_NORM, 0, 100 );
#endif
}

//=========================================================
// PainSound
//=========================================================
void CPanther::PainSound( void )
{
	if( RANDOM_LONG( 0, 2 ) == 0 )
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pPainSounds[RANDOM_LONG( 0, ARRAYSIZE( pPainSounds ) - 1 )], 1.0, ATTN_NORM, 0, m_voicePitch );
	}
}

//=========================================================
// DieSound
//=========================================================
void CPanther::DeathSound( void )
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pDeathSounds[RANDOM_LONG( 0, ARRAYSIZE( pDeathSounds ) - 1 )], 1.0, ATTN_NORM, 0, m_voicePitch );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CPanther::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	// ALERT( at_console, "event %d : %f\n", pEvent->event, pev->frame );
	switch( pEvent->event )
	{
		case ISLAVE_AE_CLAW:
		{
			// SOUND HERE!
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.pantherDmgClaw, DMG_SLASH );
			if( pHurt )
			{
				if( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) )
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
				}
				// Play a random attack hit sound
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackHitSounds ) - 1 )], 1.0, ATTN_NORM, 0, m_voicePitch );
			}
			else
			{
				// Play a random attack miss sound
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackMissSounds ) - 1 )], 1.0, ATTN_NORM, 0, m_voicePitch );
			}
		}
			break;
		case ISLAVE_AE_CLAWRAKE:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.pantherDmgClawRake, DMG_SLASH );
			if( pHurt )
			{
				if( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) )
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
				}
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackHitSounds ) - 1 )], 1.0, ATTN_NORM, 0, m_voicePitch );
			}
			else
			{
				EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackMissSounds ) - 1 )], 1.0, ATTN_NORM, 0, m_voicePitch );
			}
		}
			break;
		case ISLAVE_AE_ZAP_POWERUP:
		{
			// speed up attack when on hard
			if( g_iSkillLevel == SKILL_HARD )
				pev->framerate = 1.5;

			UTIL_MakeAimVectors( pev->angles );

			if( m_iBeams == 0 )
			{
				Vector vecSrc = pev->origin + gpGlobals->v_forward * 2;

				MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
					WRITE_BYTE( TE_DLIGHT );
					WRITE_COORD( vecSrc.x );	// X
					WRITE_COORD( vecSrc.y );	// Y
					WRITE_COORD( vecSrc.z );	// Z
					WRITE_BYTE( 12 );		// radius * 0.1
					WRITE_BYTE( 192 );		// r
					WRITE_BYTE( 64 );		// g
					WRITE_BYTE( 8 );		// b
					WRITE_BYTE( 20 / pev->framerate );		// time * 10
					WRITE_BYTE( 0 );		// decay * 0.1
				MESSAGE_END();
			}
			if( m_hDead != 0 )
			{
				WackBeam( -1, m_hDead );
				WackBeam( 1, m_hDead );
			}
			else
			{
				ArmBeam( -1 );
				ArmBeam( 1 );
				BeamGlow();
			}

			EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "panther/p_zap2.wav", 1, ATTN_NORM, 0, 100 + m_iBeams * 10 );
			pev->skin = m_iBeams / 2;
		}
			break;
		case ISLAVE_AE_ZAP_SHOOT:
		{
			ClearBeams();

			if( m_hDead != 0 )
			{
				Vector vecDest = m_hDead->pev->origin + Vector( 0, 0, 38 );
				TraceResult trace;
				UTIL_TraceHull( vecDest, vecDest, dont_ignore_monsters, human_hull, m_hDead->edict(), &trace );

				if( !trace.fStartSolid )
				{
					CBaseEntity *pNew = Create( STRING( pev->classname ), m_hDead->pev->origin, m_hDead->pev->angles );
					//CBaseMonster *pNewMonster = pNew->MyMonsterPointer();
					pNew->pev->spawnflags |= 1;
					WackBeam( -1, pNew );
					WackBeam( 1, pNew );
					UTIL_Remove( m_hDead );
					EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "panther/p_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 130, 160 ) );
					/*
					CBaseEntity *pEffect = Create( "test_effect", pNew->Center(), pev->angles );
					pEffect->Use( this, this, USE_ON, 1 );
					*/
					break;
				}
			}
			ClearMultiDamage();

			UTIL_MakeAimVectors( pev->angles );

			ZapBeam( -1 );
			ZapBeam( 1 );

			EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "panther/p_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 130, 160 ) );
			// STOP_SOUND( ENT( pev ), CHAN_WEAPON, "panther/p_zap2.wav" );
			ApplyMultiDamage( pev, pev );

			m_flNextAttack = gpGlobals->time + RANDOM_FLOAT( 0.5, 4.0 );
		}
			break;
		case ISLAVE_AE_ZAP_DONE:
		{
			ClearBeams();
		}
			break;
		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CPanther::Spawn()
{
	Precache();

	SET_MODEL( ENT( pev ), "models/panther.mdl" );

	UTIL_SetSize( pev, Vector( -32, -32, 0 ), Vector( 32, 32, 64 ) );

	pev->solid		= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	pev->effects		= 0;
	pev->health		= gSkillData.pantherHealth;
	m_bloodColor		= BLOOD_COLOR_GREEN;
        m_voicePitch		= RANDOM_LONG( 85, 110 );
	pev->view_ofs		= Vector( 0, 0, 80 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_RANGE_ATTACK2 | bits_CAP_DOORS_GROUP;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CPanther::Precache()
{
	size_t i;

	PRECACHE_MODEL( "models/panther.mdl" );
	PRECACHE_SOUND( "panther/p_zap1.wav" );
	PRECACHE_SOUND( "panther/p_zap2.wav" );
	PRECACHE_SOUND( "panther/p_electro1.wav" );
	PRECACHE_SOUND( "panther/p_shoot1.wav" );
	PRECACHE_SOUND( "panther/p_pain2.wav" );
	PRECACHE_SOUND( "panther/p_headbite.wav" );
	PRECACHE_SOUND( "panther/pclaw_miss1.wav" );

	for( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND( pAttackHitSounds[i] );

	for( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND( pAttackMissSounds[i] );

	for( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND( pIdleSounds[i] );

	for( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND( pAlertSounds[i] );

	for( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND( pPainSounds[i] );

	for( i = 0; i < ARRAYSIZE( pDeathSounds ); i++ )
		PRECACHE_SOUND( pDeathSounds[i] );

	UTIL_PrecacheOther( "test_effect" );
}

void CPanther::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	if( bitsDamageType & DMG_SHOCK )
		return;

	if( ptr->iHitgroup != HITGROUP_HEAD )
	{
		if( bitsDamageType & ( DMG_BULLET | DMG_SLASH | DMG_CLUB ) )
		{
			UTIL_Ricochet( ptr->vecEndPos, 1.0 );
			flDamage = 0.01;
		}
	}

	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

//=========================================================
// ArmBeam - small beam from arm to nearby geometry
//=========================================================
void CPanther::ArmBeam( int side )
{
	TraceResult tr;
	int r, g, b;
	float flDist = 1.0;

	if( m_iBeams >= ISLAVE_MAX_BEAMS )
		return;

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * side * 16 + gpGlobals->v_forward * 32;

	for( int i = 0; i < 3; i++ )
	{
		Vector vecAim = gpGlobals->v_right * side * RANDOM_FLOAT( 0, 1 ) + gpGlobals->v_up * RANDOM_FLOAT( -1, 1 );
		TraceResult tr1;
		UTIL_TraceLine( vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, ENT( pev ), &tr1 );
		if( flDist > tr1.flFraction )
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	}

	// Couldn't find anything close enough
	if( flDist == 1.0f )
		return;

	DecalGunshot( &tr, BULLET_PLAYER_CROWBAR );

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
	if( !m_pBeam[m_iBeams] )
		return;

	m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, entindex() );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );
	// m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );

	r = 192, g = 64, b = 8;

	m_pBeam[m_iBeams]->SetColor( r, g, b );
	m_pBeam[m_iBeams]->SetBrightness( 64 );
	m_pBeam[m_iBeams]->SetNoise( 80 );
	m_iBeams++;
}

//=========================================================
// WackBeam - regenerate dead colleagues
//=========================================================
void CPanther::WackBeam( int side, CBaseEntity *pEntity )
{
	//Vector vecDest;
	//float flDist = 1.0;
	int r, g, b;

	if( m_iBeams >= ISLAVE_MAX_BEAMS )
		return;

	if( pEntity == NULL )
		return;

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
	if( !m_pBeam[m_iBeams] )
		return;

	m_pBeam[m_iBeams]->PointEntInit( pEntity->Center(), entindex() );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );

	r = 192, g = 64, b = 8;

	m_pBeam[m_iBeams]->SetColor( r, g, b );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 80 );
	m_iBeams++;
}

//=========================================================
// ZapBeam - heavy damage directly forward
//=========================================================
void CPanther::ZapBeam( int side )
{
	Vector vecSrc, vecAim;
	int r, g, b;
	TraceResult tr;
	CBaseEntity *pEntity;

	if( m_iBeams >= ISLAVE_MAX_BEAMS )
		return;

	vecSrc = pev->origin + gpGlobals->v_up * 36;
	vecAim = ShootAtEnemy( vecSrc );
	float deflection = 0.01;
	vecAim = vecAim + side * gpGlobals->v_right * RANDOM_FLOAT( 0, deflection ) + gpGlobals->v_up * RANDOM_FLOAT( -deflection, deflection );
	UTIL_TraceLine( vecSrc, vecSrc + vecAim * 1024, dont_ignore_monsters, ENT( pev ), &tr );

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 50 );
	if( !m_pBeam[m_iBeams] )
		return;

	m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, entindex() );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );

	r = 192, g = 64, b = 8;

	m_pBeam[m_iBeams]->SetColor( r, g, b );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 20 );
	m_iBeams++;

	pEntity = CBaseEntity::Instance( tr.pHit );
	if( pEntity != NULL && pEntity->pev->takedamage )
	{
		pEntity->TraceAttack( pev, gSkillData.pantherDmgZap, vecAim, &tr, DMG_SHOCK );
	}
	UTIL_EmitAmbientSound( ENT( pev ), tr.vecEndPos, "panther/p_electro1.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ) );
}

//=========================================================
// ClearBeams - remove all beams
//=========================================================
void CPanther::ClearBeams()
{
	for( int i = 0; i < ISLAVE_MAX_BEAMS; i++ )
	{
		if( m_pBeam[i] )
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}
	m_iBeams = 0;
	pev->skin = 0;

	STOP_SOUND( ENT( pev ), CHAN_WEAPON, "panther/p_zap2.wav" );
}
