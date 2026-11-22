/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
// hgrunt
//=========================================================

//=========================================================
// Hit groups!	
//=========================================================
/*

  1 - Head
  2 - Stomach
  3 - Gun

*/


#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "animation.h"
#include "squadmonster.h"
#include "weapons.h"
#include "talkmonster.h"
#include "soundent.h"
#include "effects.h"
#include "customentity.h"

#ifdef _WIN32

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define GRUNT_CLIP_SIZE			36 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!
#define HGRUNT_LIMP_HEALTH		20
#define HGRUNT_SENTENCE_VOLUME	0.35f
#define HGRUNT_9MMAR				( 1 << 0 )
#define HGRUNT_HANDGRENADE		( 1 << 1 )
#define HGRUNT_GRENADELAUNCHER	( 1 << 2 )
#define HGRUNT_SHOTGUN			( 1 << 3 )
#define HEAD_GROUP				1
#define HEAD_GRUNT				0
#define HEAD_COMMANDER			1
#define HEAD_SHOTGUN				2
#define HEAD_M203				3
#define GUN_GROUP				2
#define GUN_MP5					0
#define GUN_SHOTGUN				1
#define GUN_NONE				2
#define bits_COND_GRUNT_NOFIRE	bits_COND_SPECIAL1

enum
{
	HGRUNT_AE_RELOAD = 2,
	HGRUNT_AE_KICK,
	HGRUNT_AE_BURST1,
	HGRUNT_AE_BURST2,
	HGRUNT_AE_BURST3,
	HGRUNT_AE_GREN_TOSS,
	HGRUNT_AE_GREN_LAUNCH,
	HGRUNT_AE_GREN_DROP,
	HGRUNT_AE_CAUGHT_ENEMY,
	HGRUNT_AE_DROP_GUN
};

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_GRUNT_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE, // move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_GRUNT_COVER_AND_RELOAD,
	SCHED_GRUNT_SWEEP,
	SCHED_GRUNT_FOUND_ENEMY,
	SCHED_GRUNT_REPEL,
	SCHED_GRUNT_REPEL_ATTACK,
	SCHED_GRUNT_REPEL_LAND,
	SCHED_GRUNT_WAIT_FACE_ENEMY,
	SCHED_GRUNT_TAKECOVER_FAILED, // special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_GRUNT_ELOF_FAIL
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_GRUNT_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_GRUNT_SPEAK_SENTENCE,
	TASK_GRUNT_CHECK_FIRE
};

enum
{
	HGRUNT_SENT_NONE = -1,
	HGRUNT_SENT_GREN,
	HGRUNT_SENT_ALERT,
	HGRUNT_SENT_MONSTER,
	HGRUNT_SENT_COVER,
	HGRUNT_SENT_THROW,
	HGRUNT_SENT_CHARGE,
	HGRUNT_SENT_TAUNT
};

int g_fGruntQuestion;				// true if an idle grunt asked a question. Cleared when someone answers.

class CHGrunt : public CSquadMonster
{
public:
	void Spawn();
	void Precache();
	void SetYawSpeed();
	int Classify();
	int ISoundMask();
	void HandleAnimEvent( MonsterEvent_t* event );
	BOOL FCanCheckAttacks();
	BOOL CheckMeleeAttack1( float dot, float distance );
	BOOL CheckRangeAttack1( float dot, float distance );
	BOOL CheckRangeAttack2( float dot, float distance );
	void CheckAmmo();
	void SetActivity( Activity activity );
	void StartTask( Task_t* task );
	void RunTask( Task_t* task );
	void DeathSound();
	void PainSound();
	void IdleSound();
	Vector GetGunPosition();
	void Shoot();
	void Shotgun();
	void PrescheduleThink();
	void GibMonster();
	void SpeakSentence();
	int Save( CSave& save );
	int Restore( CRestore& restore );
	CBaseEntity* Kick();
	Schedule_t* GetSchedule();
	Schedule_t* GetScheduleOfType( int type );
	void TraceAttack( entvars_t* attacker, float damage, Vector direction, TraceResult* trace, int damageType );
	int TakeDamage( entvars_t* inflictor, entvars_t* attacker, float damage, int damageType );
	int IRelationship( CBaseEntity* target );
	BOOL FOkToSpeak();
	void JustSpoke();
	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];
	static const char* pGruntSentences[];

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float	m_flNextGrenadeCheck;
	float	m_flNextPainTime;
	float	m_flLastEnemySightTime;
	Vector	m_vecTossVelocity;
	BOOL	m_fThrowGrenade;
	BOOL	m_fStanding;
	BOOL	m_fFirstEncounter; // only put on the handsign show in the squad's first encounter.
	int		m_cClipSize;
	int		m_voicePitch;
	int		m_iBrassShell;
	int		m_iShotgunShell;
	int		m_iSentence;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_human_grunt, CHGrunt );

TYPEDESCRIPTION CHGrunt::m_SaveData[] =
{
	DEFINE_FIELD( CHGrunt, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CHGrunt, m_flNextPainTime, FIELD_TIME ),
//	DEFINE_FIELD( CHGrunt, m_flLastEnemySightTime, FIELD_TIME ), // don't save, go to zero
	DEFINE_FIELD( CHGrunt, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CHGrunt, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHGrunt, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHGrunt, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHGrunt, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( CHGrunt, m_voicePitch, FIELD_INTEGER ),
//  DEFINE_FIELD( CShotgun, m_iBrassShell, FIELD_INTEGER ),
//  DEFINE_FIELD( CShotgun, m_iShotgunShell, FIELD_INTEGER ),
	DEFINE_FIELD( CHGrunt, m_iSentence, FIELD_INTEGER )
};

IMPLEMENT_SAVERESTORE( CHGrunt, CSquadMonster );

const char* CHGrunt::pGruntSentences[] =
{
	"HG_GREN", "HG_ALERT", "HG_MONSTER", "HG_COVER", "HG_THROW", "HG_CHARGE", "HG_TAUNT"
};

void CHGrunt::SpeakSentence()
{
	if ( m_iSentence != HGRUNT_SENT_NONE && FOkToSpeak() )
	{
		SayNamedSentence( pGruntSentences[m_iSentence], HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
		JustSpoke();
	}
}

int CHGrunt::IRelationship( CBaseEntity* target )
{
	if ( FClassnameIs( target->pev, "monster_alien_grunt" ) ||
		FClassnameIs( target->pev, "monster_gargantua" ) )
		return R_NM;

	return CSquadMonster::IRelationship( target );
}

int CHGrunt::ISoundMask()
{
	return bits_SOUND_WORLD | bits_SOUND_COMBAT | bits_SOUND_PLAYER | bits_SOUND_DANGER;
}

BOOL CHGrunt::FOkToSpeak()
{
	return CTalkMonster::g_talkWaitTime < gpGlobals->time &&
		( !( pev->spawnflags & SF_MONSTER_GAG ) || m_MonsterState == MONSTERSTATE_COMBAT );
}

//=========================================================
//=========================================================
void CHGrunt::JustSpoke()
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT( 1.5, 2.0 );
	m_iSentence = HGRUNT_SENT_NONE;
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void CHGrunt::PrescheduleThink()
{
	if ( InSquad() && m_hEnemy != NULL )
	{
		if ( HasConditions( bits_COND_SEE_ENEMY ) )
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
		else if ( gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5 )
			MySquadLeader()->m_fEnemyEluded = TRUE;
	}
}

BOOL CHGrunt::FCanCheckAttacks()
{
	return !HasConditions( bits_COND_ENEMY_TOOFAR );
}

BOOL CHGrunt::CheckMeleeAttack1( float dot, float distance )
{
	if ( m_hEnemy == NULL )
		return FALSE;

	CBaseMonster* enemy = m_hEnemy->MyMonsterPointer();
	if ( !enemy )
		return FALSE;

	return distance <= 64 && dot >= 0.7 && enemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		enemy->Classify() != CLASS_PLAYER_BIOWEAPON;
}

BOOL CHGrunt::CheckRangeAttack1( float dot, float distance )
{
	if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) || distance > 2048 || dot < 0.5 || !NoFriendlyFire() )
		return FALSE;

	if ( m_hEnemy->IsPlayer() || distance > 64 )
	{
		TraceResult trace;
		Vector origin = GetGunPosition();
		UTIL_TraceLine( origin, m_hEnemy->BodyTarget( origin ), ignore_monsters, ignore_glass, ENT( pev ), &trace );
		return trace.flFraction == 1.0;
	}

	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack. 
//=========================================================
BOOL CHGrunt::CheckRangeAttack2( float, float )
{
	if ( !( pev->weapons & ( HGRUNT_HANDGRENADE | HGRUNT_GRENADELAUNCHER ) ) )
		return FALSE;

	// if the grunt isn't moving, it's ok to check.
	if ( m_flGroundSpeed != 0 )
	{
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	// assume things haven't changed too much since last time
	if ( gpGlobals->time < m_flNextGrenadeCheck )
		return m_fThrowGrenade;
	if ( !( m_hEnemy->pev->flags & FL_ONGROUND ) && m_hEnemy->pev->waterlevel == 0 &&
		m_vecEnemyLKP.z > pev->absmax.z )
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to 
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	Vector target;
	if ( pev->weapons & HGRUNT_HANDGRENADE )
	{
		if ( RANDOM_LONG( 0, 1 ) )
			target = Vector( m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z );
		else
			target = m_vecEnemyLKP;
	}
	else
	{
		// find target
		// vecTarget = m_hEnemy->BodyTarget( pev->origin );
		target = m_vecEnemyLKP + ( m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin );
		if ( HasConditions( bits_COND_SEE_ENEMY ) )
			target = target + ( ( target - pev->origin ).Length() / gSkillData.hgruntGrenadeSpeed ) *
				m_hEnemy->pev->velocity;
	}

	if ( InSquad() && SquadMemberInRange( target, 256 ) )
	{
		// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = FALSE;
	}
	if ( ( target - pev->origin ).Length2D() <= 256 )
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	if ( pev->weapons & HGRUNT_HANDGRENADE )
	{
		Vector toss = VecCheckToss( pev, GetGunPosition(), target, 0.5 );
		if ( toss != g_vecZero )
		{
			m_vecTossVelocity = toss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}
	else
	{
		Vector toss = VecCheckThrow( pev, GetGunPosition(), target, gSkillData.hgruntGrenadeSpeed, 0.5 );
		if ( toss != g_vecZero )
		{
			m_vecTossVelocity = toss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 0.3; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}

	return m_fThrowGrenade;
}

int CHGrunt::TakeDamage( entvars_t* inflictor, entvars_t* attacker, float damage, int damageType )
{
	Forget( bits_MEMORY_INCOVER );
	return CSquadMonster::TakeDamage( inflictor, attacker, damage, damageType );
}

void CHGrunt::TraceAttack( entvars_t* attacker, float damage, Vector direction, TraceResult* trace, int damageType )
{
	// check for helmet shot
	if ( trace->iHitgroup == 11 )
	{
		if ( GetBodygroup( HEAD_GROUP ) == HEAD_GRUNT &&
			( damageType & ( DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB ) ) )
		{
			damage -= 20;
			if ( damage <= 0 )
			{
				UTIL_Ricochet( trace->vecEndPos, 1.0 );
				damage = 0.01;
			}
		}
		// it's head shot anyways
		trace->iHitgroup = HITGROUP_HEAD;
	}
	CSquadMonster::TraceAttack( attacker, damage, direction, trace, damageType );
}

void CHGrunt::SetYawSpeed()
{
	int speed;
	switch ( m_Activity )
	{
	case ACT_IDLE:
	case ACT_RUN:
		speed = 150;
		break;
	case ACT_WALK:
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		speed = 180;
		break;
	case ACT_GLIDE:
	case ACT_FLY:
		speed = 30;
		break;
	case ACT_RANGE_ATTACK1:
	case ACT_RANGE_ATTACK2:
	case ACT_MELEE_ATTACK1:
	case ACT_MELEE_ATTACK2:
		speed = 120;
		break;
	default:
		speed = 90;
		break;
	}
	pev->yaw_speed = speed;
}

void CHGrunt::CheckAmmo()
{
	if ( m_cAmmoLoaded <= 0 )
		SetConditions( bits_COND_NO_AMMO_LOADED );
}

void CHGrunt::GibMonster()
{
	if ( GetBodygroup( GUN_GROUP ) != GUN_NONE )
	{
		Vector origin, angles;
		GetAttachment( 0, origin, angles );
		if ( pev->weapons & HGRUNT_GRENADELAUNCHER )
		{
			CBaseEntity* grenades = DropItem( "ammo_ARgrenades", origin, angles );
			if ( grenades )
			{
				Vector velocity;
				velocity.x = RANDOM_FLOAT( -100, 100 );
				velocity.y = RANDOM_FLOAT( -100, 100 );
				velocity.z = RANDOM_FLOAT( 200, 300 );
				grenades->pev->velocity = velocity;
				grenades->pev->avelocity = Vector( 0, RANDOM_FLOAT( 200, 400 ), 0 );
			}
		}
	}
	CBaseMonster::GibMonster();
}

void CHGrunt::IdleSound()
{
	if ( !FOkToSpeak() || !( g_fGruntQuestion || RANDOM_LONG( 0, 1 ) ) )
		return;

	if ( !g_fGruntQuestion )
	{
		// ask question or make statement
		switch ( RANDOM_LONG( 0, 2 ) )
		{
		case 0: // check in
			SayNamedSentence( "HG_CHECK", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
			g_fGruntQuestion = 1;
			break;
		case 1: // question
			SayNamedSentence( "HG_QUEST", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
			g_fGruntQuestion = 2;
			break;
		case 2: // statement
			SayNamedSentence( "HG_IDLE", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
			break;
		}
	}
	else
	{
		switch ( g_fGruntQuestion )
		{
		case 1: // check in
			SayNamedSentence( "HG_CLEAR", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
			break;
		case 2:
			SayNamedSentence( "HG_ANSWER", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
			break;
		}
		g_fGruntQuestion = 0;
	}
	JustSpoke();
}

int CHGrunt::Classify()
{
	return CLASS_HUMAN_MILITARY;
}

CBaseEntity* CHGrunt::Kick()
{
	UTIL_MakeVectors( pev->angles );
	Vector start = pev->origin;
	start.z += pev->size.z * 0.5;
	Vector end = start + gpGlobals->v_forward * 70;
	TraceResult trace;
	UTIL_TraceHull( start, end, dont_ignore_monsters, head_hull, ENT( pev ), &trace );
	return trace.pHit ? CBaseEntity::Instance( trace.pHit ) : NULL;
}

//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

Vector CHGrunt::GetGunPosition()
{
	if ( m_fStanding )
		return pev->origin + Vector( 0, 0, 60 );
	else
		return pev->origin + Vector( 0, 0, 48 );
}

//=========================================================
// Shoot
//=========================================================
void CHGrunt::Shoot()
{
	if ( m_hEnemy == NULL )
		return;

	Vector origin = GetGunPosition();
	Vector direction = ShootAtEnemy( origin );
	UTIL_MakeVectors( pev->angles );
	Vector forward = gpGlobals->v_forward * RANDOM_FLOAT( -40, 40 );
	Vector up = gpGlobals->v_up * RANDOM_FLOAT( 75, 200 );
	Vector shellVelocity = gpGlobals->v_right * RANDOM_FLOAT( 40, 90 ) + up + forward;
	EjectBrass( origin - direction * 24, shellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL );
	FireBullets( 1, origin, direction, VECTOR_CONE_10DEGREES, 2048, BULLET_MONSTER_MP5 );
	pev->effects |= EF_MUZZLEFLASH;
	m_cAmmoLoaded--;
	Vector angles = UTIL_VecToAngles( direction );
	SetBlending( 0, angles.x );
}

//=========================================================
// Shoot
//=========================================================
void CHGrunt::Shotgun()
{
	if ( m_hEnemy == NULL )
		return;

	Vector origin = GetGunPosition();
	Vector direction = ShootAtEnemy( origin );
	UTIL_MakeVectors( pev->angles );
	Vector forward = gpGlobals->v_forward * RANDOM_FLOAT( -40, 40 );
	Vector up = gpGlobals->v_up * RANDOM_FLOAT( 75, 200 );
	Vector shellVelocity = gpGlobals->v_right * RANDOM_FLOAT( 40, 90 ) + up + forward;
	EjectBrass( origin - direction * 24, shellVelocity, pev->angles.y, m_iShotgunShell, TE_BOUNCE_SHOTSHELL );
	FireBullets( gSkillData.hgruntShotgunPellets, origin, direction, VECTOR_CONE_15DEGREES, 2048,
		BULLET_PLAYER_BUCKSHOT, 0 );
	pev->effects |= EF_MUZZLEFLASH;
	m_cAmmoLoaded--;
	Vector angles = UTIL_VecToAngles( direction );
	SetBlending( 0, angles.x );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHGrunt::HandleAnimEvent( MonsterEvent_t* event )
{
	switch ( event->event )
	{
	case HGRUNT_AE_DROP_GUN:
	{
		Vector origin, angles;
		GetAttachment( 0, origin, angles );
		SetBodygroup( GUN_GROUP, GUN_NONE );
		break;
	}
	case HGRUNT_AE_RELOAD:
		EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "hgrunt/gr_reload1.wav", 1, ATTN_NORM );
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions( bits_COND_NO_AMMO_LOADED );
		break;
	case HGRUNT_AE_GREN_TOSS:
		UTIL_MakeVectors( pev->angles );
		// CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 34 + Vector (0, 0, 32), m_vecTossVelocity, 3.5 );
		CGrenade::ShootTimed( pev, GetGunPosition(), m_vecTossVelocity, 3.5, 35, 0 );
		m_fThrowGrenade = FALSE;
		m_flNextGrenadeCheck = gpGlobals->time + 6;
		break;
	case HGRUNT_AE_GREN_LAUNCH:
		EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "weapons/glauncher.wav", 0.8, ATTN_NORM );
		CGrenade::ShootContact( pev, GetGunPosition(), m_vecTossVelocity );
		m_fThrowGrenade = FALSE;
		if ( g_iSkillLevel == SKILL_HARD )
			m_flNextGrenadeCheck = gpGlobals->time + RANDOM_FLOAT( 2, 5 ); // wait a random amount of time before shooting again
		else
			m_flNextGrenadeCheck = gpGlobals->time + 6; // wait six seconds before even looking again to see if a grenade can be thrown.
		break;
	case HGRUNT_AE_GREN_DROP:
		UTIL_MakeVectors( pev->angles );
		CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27 +
			gpGlobals->v_up * 6, g_vecZero, 3, 35, 0 );
		break;
	case HGRUNT_AE_BURST1:
		if ( pev->weapons & HGRUNT_9MMAR )
		{
			Shoot();
			if ( RANDOM_LONG( 0, 1 ) )
				EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "hgrunt/gr_mgun1.wav", 1, ATTN_NORM );
			else
				EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "hgrunt/gr_mgun2.wav", 1, ATTN_NORM );
		}
		else
		{
			Shotgun();
			EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "weapons/sbarrel1.wav", 1, ATTN_NORM );
		}
		CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
		break;
	case HGRUNT_AE_BURST2:
	case HGRUNT_AE_BURST3:
		Shoot();
		break;
	case HGRUNT_AE_KICK:
	{
		CBaseEntity* victim = Kick();
		if ( victim )
		{
			UTIL_MakeVectors( pev->angles );
			victim->pev->punchangle.x = 15;
			victim->pev->velocity = victim->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
			victim->TakeDamage( pev, pev, gSkillData.hgruntDmgKick, DMG_CLUB );
		}
		break;
	}
	case HGRUNT_AE_CAUGHT_ENEMY:
		if ( FOkToSpeak() )
		{
			SayNamedSentence( "HG_ALERT", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
			JustSpoke();
		}
	default:
		CSquadMonster::HandleAnimEvent( event );
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CHGrunt::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/hgrunt.mdl" );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.hgruntHealth;
	m_flFieldOfView = 0.2; // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime = gpGlobals->time;
	m_HackedGunPos = Vector( 0, 0, 55 );
	m_iSentence = HGRUNT_SENT_NONE;
	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	m_fEnemyEluded = FALSE;
	m_fFirstEncounter = TRUE; // this is true when the grunt spawns, because he hasn't encountered an enemy yet.
	if ( pev->weapons == 0 )
		pev->weapons = HGRUNT_9MMAR | HGRUNT_HANDGRENADE;
	if ( pev->weapons & HGRUNT_SHOTGUN )
	{
		SetBodygroup( GUN_GROUP, GUN_SHOTGUN );
		m_cClipSize = 8;
	}
	else
		m_cClipSize = GRUNT_CLIP_SIZE;
	m_cAmmoLoaded = m_cClipSize;
	pev->skin = RANDOM_LONG( 0, 99 ) >= 80;
	if ( pev->weapons & HGRUNT_SHOTGUN )
		SetBodygroup( HEAD_GROUP, HEAD_SHOTGUN );
	else if ( pev->weapons & HGRUNT_GRENADELAUNCHER )
	{
		SetBodygroup( HEAD_GROUP, HEAD_M203 );
		pev->skin = 1; // alway dark skin
	}
	CTalkMonster::g_talkWaitTime = 0;
	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHGrunt::Precache()
{
	PRECACHE_MODEL( "models/hgrunt.mdl" );
	PRECACHE_SOUND( "hgrunt/gr_mgun1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_mgun2.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die2.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die3.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain2.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain3.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain4.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain5.wav" );
	PRECACHE_SOUND( "hgrunt/gr_reload1.wav" );
	PRECACHE_SOUND( "weapons/glauncher.wav" );
	PRECACHE_SOUND( "weapons/sbarrel1.wav" );
	PRECACHE_SOUND( "zombie/claw_miss2.wav" ); // because we use the basemonster SWIPE animation event

	// get voice pitch
	if ( RANDOM_LONG( 0, 1 ) )
		m_voicePitch = 109 + RANDOM_LONG( 0, 7 );
	else
		m_voicePitch = 100;
	m_iBrassShell = PRECACHE_MODEL( "models/shell.mdl" ); // brass shell
	m_iShotgunShell = PRECACHE_MODEL( "models/shotgunshell.mdl" );
}

//=========================================================
// start task
//=========================================================
void CHGrunt::StartTask( Task_t* task )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;
	switch ( task->iTask )
	{
	case TASK_GRUNT_CHECK_FIRE:
		if ( !NoFriendlyFire() )
			SetConditions( bits_COND_GRUNT_NOFIRE );
		TaskComplete();
		break;
	case TASK_GRUNT_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;
	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		Forget( bits_MEMORY_INCOVER );
		CSquadMonster::StartTask( task );
		break;
	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;
	case TASK_GRUNT_FACE_TOSS_DIR:
		break;
	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CSquadMonster::StartTask( task );
		if ( pev->movetype == MOVETYPE_FLY )
			m_IdealActivity = ACT_GLIDE;
		break;
	default:
		CSquadMonster::StartTask( task );
		break;
	}
}

void CHGrunt::RunTask( Task_t* task )
{
	if ( task->iTask == TASK_GRUNT_FACE_TOSS_DIR )
	{
		// project a point along the toss vector and turn to face that point.
		MakeIdealYaw( pev->origin + m_vecTossVelocity * 64 );
		ChangeYaw( pev->yaw_speed );
		if ( FacingIdeal() )
			m_iTaskStatus = TASKSTATUS_COMPLETE;
	}
	else
		CSquadMonster::RunTask( task );
}

//=========================================================
// PainSound
//=========================================================
void CHGrunt::PainSound()
{
	if ( gpGlobals->time > m_flNextPainTime )
	{
		switch ( RANDOM_LONG( 0, 6 ) )
		{
		case 0:
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hgrunt/gr_pain3.wav", 1, ATTN_NORM );
			break;
		case 1:
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hgrunt/gr_pain4.wav", 1, ATTN_NORM );
			break;
		case 2:
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hgrunt/gr_pain5.wav", 1, ATTN_NORM );
			break;
		case 3:
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hgrunt/gr_pain1.wav", 1, ATTN_NORM );
			break;
		case 4:
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hgrunt/gr_pain2.wav", 1, ATTN_NORM );
			break;
		}
		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CHGrunt::DeathSound()
{
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hgrunt/gr_die1.wav", 1, ATTN_IDLE );
		break;
	case 1:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hgrunt/gr_die2.wav", 1, ATTN_IDLE );
		break;
	case 2:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hgrunt/gr_die3.wav", 1, ATTN_IDLE );
		break;
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// GruntFail
//=========================================================
Task_t tlGruntFail[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, ACT_IDLE },
	{ TASK_WAIT, 2 },
	{ TASK_WAIT_PVS, 0 }
};

Schedule_t slGruntFail[] =
{
	{ tlGruntFail, ARRAYSIZE( tlGruntFail ), bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 | bits_COND_CAN_MELEE_ATTACK2, 0, "Grunt Fail" }
};

//=========================================================
// Grunt Combat Fail
//=========================================================
Task_t tlGruntCombatFail[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY, 2 },
	{ TASK_WAIT_PVS, 0 }
};

Schedule_t slGruntCombatFail[] =
{
	{ tlGruntCombatFail, ARRAYSIZE( tlGruntCombatFail ), bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2, 0, "Grunt Combat Fail" }
};

//=========================================================
// Victory dance!
//=========================================================
Task_t tlGruntVictoryDance[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_WAIT, 1.5 },
	{ TASK_GET_PATH_TO_ENEMY_CORPSE, 0 },
	{ TASK_WALK_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE }
};

Schedule_t slGruntVictoryDance[] =
{
	{ tlGruntVictoryDance, ARRAYSIZE( tlGruntVictoryDance ), bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE, 0, "GruntVictoryDance" }
};

//=========================================================
// Establish line of fire - move to a position that allows
// the grunt to attack.
//=========================================================
Task_t tlGruntEstablishLineOfFire[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_GRUNT_ELOF_FAIL },
	{ TASK_GET_PATH_TO_ENEMY, 0 },
	{ TASK_GRUNT_SPEAK_SENTENCE, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 }
};

Schedule_t slGruntEstablishLineOfFire[] =
{
	{ tlGruntEstablishLineOfFire, ARRAYSIZE( tlGruntEstablishLineOfFire ), bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD | bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 | bits_COND_CAN_MELEE_ATTACK2 | bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER, "GruntEstablishLineOfFire" }
};

//=========================================================
// GruntFoundEnemy - grunt established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t tlGruntFoundEnemy[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_SIGNAL1 }
};

Schedule_t slGruntFoundEnemy[] =
{
	{ tlGruntFoundEnemy, ARRAYSIZE( tlGruntFoundEnemy ), bits_COND_HEAR_SOUND, bits_SOUND_DANGER, "GruntFoundEnemy" }
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t tlGruntCombatFace1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, ACT_IDLE },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_WAIT, 1.5 },
	{ TASK_SET_SCHEDULE, SCHED_GRUNT_SWEEP }
};

Schedule_t slGruntCombatFace[] =
{
	{ tlGruntCombatFace1, ARRAYSIZE( tlGruntCombatFace1 ), bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD |
		bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2, 0, "Combat Face" }
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or grunt gets hurt.
//=========================================================
Task_t tlGruntSignalSuppress[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_SIGNAL2 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_GRUNT_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_GRUNT_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_GRUNT_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_GRUNT_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_GRUNT_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 }
};

Schedule_t slGruntSignalSuppress[] =
{
	{ tlGruntSignalSuppress, ARRAYSIZE( tlGruntSignalSuppress ), bits_COND_ENEMY_DEAD | bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND | bits_COND_GRUNT_NOFIRE | bits_COND_NO_AMMO_LOADED,
		bits_SOUND_DANGER, "SignalSuppress" }
};

Task_t tlGruntSuppress[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_GRUNT_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_GRUNT_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_GRUNT_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_GRUNT_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_GRUNT_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 }
};

Schedule_t slGruntSuppress[] =
{
	{ tlGruntSuppress, ARRAYSIZE( tlGruntSuppress ), bits_COND_ENEMY_DEAD | bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND | bits_COND_GRUNT_NOFIRE | bits_COND_NO_AMMO_LOADED,
		bits_SOUND_DANGER, "Suppress" }
};


//=========================================================
// grunt wait in cover - we don't allow danger or the ability
// to attack to break a grunt's run to cover schedule, but
// when a grunt is in cover, we do want them to attack if they can.
//=========================================================
Task_t tlGruntWaitInCover[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY, 1 }
};

Schedule_t slGruntWaitInCover[] =
{
	{ tlGruntWaitInCover, ARRAYSIZE( tlGruntWaitInCover ), bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND |
		bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2 | bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2, bits_SOUND_DANGER, "GruntWaitInCover" }
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t tlGruntTakeCover1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_FAIL_SCHEDULE, SCHED_GRUNT_TAKECOVER_FAILED },
	{ TASK_WAIT, 0.2f },
	{ TASK_FIND_COVER_FROM_ENEMY, 0 },
	{ TASK_GRUNT_SPEAK_SENTENCE, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_REMEMBER, bits_MEMORY_INCOVER },
	{ TASK_SET_SCHEDULE, SCHED_GRUNT_WAIT_FACE_ENEMY }
};

Schedule_t slGruntTakeCover[] =
{
	{ tlGruntTakeCover1, ARRAYSIZE( tlGruntTakeCover1 ), 0, 0, "TakeCover" }
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t tlGruntGrenadeCover1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FIND_COVER_FROM_ENEMY, 99 },
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY, 384 },
	{ TASK_PLAY_SEQUENCE, ACT_SPECIAL_ATTACK1 },
	{ TASK_CLEAR_MOVE_WAIT, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_SET_SCHEDULE, SCHED_GRUNT_WAIT_FACE_ENEMY }
};

Schedule_t slGruntGrenadeCover[] =
{
	{ tlGruntGrenadeCover1, ARRAYSIZE( tlGruntGrenadeCover1 ), 0, 0, "GrenadeCover" }
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t tlGruntTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_RANGE_ATTACK2, 0 },
	{ TASK_SET_SCHEDULE, SCHED_TAKE_COVER_FROM_ENEMY }
};

Schedule_t slGruntTossGrenadeCover[] =
{
	{ tlGruntTossGrenadeCover1, ARRAYSIZE( tlGruntTossGrenadeCover1 ), 0, 0, "TossGrenadeCover" }
};

//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t tlGruntTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_COWER },
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FIND_COVER_FROM_BEST_SOUND, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_REMEMBER, bits_MEMORY_INCOVER },
	{ TASK_TURN_LEFT, 179 }
};

Schedule_t slGruntTakeCoverFromBestSound[] =
{
	{ tlGruntTakeCoverFromBestSound, ARRAYSIZE( tlGruntTakeCoverFromBestSound ), 0, 0, "GruntTakeCoverFromBestSound" }
};

//=========================================================
// Grunt reload schedule
//=========================================================
Task_t tlGruntHideReload[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_FAIL_SCHEDULE, SCHED_RELOAD },
	{ TASK_FIND_COVER_FROM_ENEMY, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_REMEMBER, bits_MEMORY_INCOVER },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_RELOAD }
};

Schedule_t slGruntHideReload[] =
{
	{ tlGruntHideReload, ARRAYSIZE( tlGruntHideReload ), bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER, "GruntHideReload" }
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t tlGruntSweep[] =
{
	{ TASK_TURN_LEFT, 179 },
	{ TASK_WAIT, 1 },
	{ TASK_TURN_LEFT, 179 },
	{ TASK_WAIT, 1 }
};

Schedule_t slGruntSweep[] =
{
	{ tlGruntSweep, ARRAYSIZE( tlGruntSweep ), bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE | bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2 | bits_COND_HEAR_SOUND,
		bits_SOUND_WORLD | bits_SOUND_DANGER | bits_SOUND_PLAYER, "Grunt Sweep" }
};

//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t tlGruntRangeAttack1A[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_CROUCH },
	{ TASK_GRUNT_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_GRUNT_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_GRUNT_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_GRUNT_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 }
};

Schedule_t slGruntRangeAttack1A[] =
{
	{ tlGruntRangeAttack1A, ARRAYSIZE( tlGruntRangeAttack1A ), bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD |
		bits_COND_HEAVY_DAMAGE | bits_COND_ENEMY_OCCLUDED | bits_COND_HEAR_SOUND | bits_COND_GRUNT_NOFIRE |
		bits_COND_NO_AMMO_LOADED, bits_SOUND_DANGER, "Range Attack1A" }
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t tlGruntRangeAttack1B[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_IDLE_ANGRY },
	{ TASK_GRUNT_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_GRUNT_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_GRUNT_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_GRUNT_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 }
};

Schedule_t slGruntRangeAttack1B[] =
{
	{ tlGruntRangeAttack1B, ARRAYSIZE( tlGruntRangeAttack1B ), bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD |
		bits_COND_HEAVY_DAMAGE | bits_COND_ENEMY_OCCLUDED | bits_COND_NO_AMMO_LOADED | bits_COND_GRUNT_NOFIRE |
		bits_COND_HEAR_SOUND, bits_SOUND_DANGER, "Range Attack1B" }
};

//=========================================================
// secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t tlGruntRangeAttack2[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_GRUNT_FACE_TOSS_DIR, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_RANGE_ATTACK2 },
	{ TASK_SET_SCHEDULE, SCHED_GRUNT_WAIT_FACE_ENEMY }
};

Schedule_t slGruntRangeAttack2[] =
{
	{ tlGruntRangeAttack2, ARRAYSIZE( tlGruntRangeAttack2 ), 0, 0, "RangeAttack2" }
};


//=========================================================
// repel 
//=========================================================
Task_t tlGruntRepel[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_GLIDE }
};

Schedule_t slGruntRepel[] =
{
	{ tlGruntRepel, ARRAYSIZE( tlGruntRepel ), bits_COND_SEE_ENEMY | bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER | bits_SOUND_COMBAT | bits_SOUND_PLAYER, "Repel" }
};


//=========================================================
// repel 
//=========================================================
Task_t tlGruntRepelAttack[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_FLY }
};

Schedule_t slGruntRepelAttack[] =
{
	{ tlGruntRepelAttack, ARRAYSIZE( tlGruntRepelAttack ), bits_COND_ENEMY_OCCLUDED, 0, "Repel Attack" }
};

//=========================================================
// repel land
//=========================================================
Task_t tlGruntRepelLand[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_LAND },
	{ TASK_GET_PATH_TO_LASTPOSITION, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_CLEAR_LASTPOSITION, 0 }
};

Schedule_t slGruntRepelLand[] =
{
	{ tlGruntRepelLand, ARRAYSIZE( tlGruntRepelLand ), bits_COND_SEE_ENEMY | bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER | bits_SOUND_COMBAT | bits_SOUND_PLAYER, "Repel Land" }
};

DEFINE_CUSTOM_SCHEDULES( CHGrunt )
{
	slGruntFail,
	slGruntCombatFail,
	slGruntVictoryDance,
	slGruntEstablishLineOfFire,
	slGruntFoundEnemy,
	slGruntCombatFace,
	slGruntSignalSuppress,
	slGruntSuppress,
	slGruntWaitInCover,
	slGruntTakeCover,
	slGruntGrenadeCover,
	slGruntTossGrenadeCover,
	slGruntTakeCoverFromBestSound,
	slGruntHideReload,
	slGruntSweep,
	slGruntRangeAttack1A,
	slGruntRangeAttack1B,
	slGruntRangeAttack2,
	slGruntRepel,
	slGruntRepelAttack,
	slGruntRepelLand
};

IMPLEMENT_CUSTOM_SCHEDULES( CHGrunt, CSquadMonster );

void CHGrunt::SetActivity( Activity activity )
{
	int sequence;
	GET_MODEL_PTR( ENT( pev ) );
	switch ( activity )
	{
	case ACT_RANGE_ATTACK1:
		if ( pev->weapons & HGRUNT_9MMAR )
			sequence = LookupSequence( m_fStanding ? "standing_mp5" : "crouching_mp5" );
		else
			sequence = LookupSequence( m_fStanding ? "standing_shotgun" : "crouching_shotgun" );
		break;
	case ACT_RANGE_ATTACK2:
		sequence = LookupSequence( ( pev->weapons & HGRUNT_HANDGRENADE ) ? "throwgrenade" : "launchgrenade" );
		break;
	case ACT_RUN:
		sequence = LookupActivity( pev->health <= HGRUNT_LIMP_HEALTH ? ACT_RUN_HURT : activity );
		break;
	case ACT_WALK:
		sequence = LookupActivity( pev->health <= HGRUNT_LIMP_HEALTH ? ACT_WALK_HURT : activity );
		break;
	case ACT_IDLE:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
			activity = ACT_IDLE_ANGRY;
	default:
		sequence = LookupActivity( activity );
		break;
	}

	m_Activity = activity;
	if ( sequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != sequence || !m_fSequenceLoops )
			pev->frame = 0;
		pev->sequence = sequence;
		ResetSequenceInfo();
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT( at_console, "%s has no sequence for act:%d\n", STRING( pev->classname ), activity );
		pev->sequence = 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t* CHGrunt::GetSchedule()
{

	// clear old sentence
	m_iSentence = HGRUNT_SENT_NONE;

	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling. 
	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if ( pev->flags & FL_ONGROUND )
		{
			// just landed
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType( SCHED_GRUNT_REPEL_LAND );
		}
		return GetScheduleOfType( m_MonsterState == MONSTERSTATE_COMBAT ?
			SCHED_GRUNT_REPEL_ATTACK : SCHED_GRUNT_REPEL );
	}

	// grunts place HIGH priority on running away from danger sounds.
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound* sound = PBestSound();
		if ( sound && ( sound->m_iType & bits_SOUND_DANGER ) )
		{
			// dangerous sound nearby!

			//!!!KELLY - currently, this is the grunt's signal that a grenade has landed nearby,
			// and the grunt should find cover from the blast
			// good place for "SHIT!" or some other colorful verbal indicator of dismay.
			// It's not safe to play a verbal order here "Scatter", etc cause 
			// this may only affect a single individual in a squad. 

			if ( FOkToSpeak() )
			{
				SayNamedSentence( "HG_GREN", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
				JustSpoke();
			}
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
		}
	}
	if ( m_MonsterState == MONSTERSTATE_COMBAT )
	{
// dead enemy
		if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			return CBaseMonster::GetSchedule();

// new enemy
		if ( HasConditions( bits_COND_NEW_ENEMY ) )
		{
			if ( InSquad() )
			{
				MySquadLeader()->m_fEnemyEluded = FALSE;
				if ( !IsLeader() )
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				if ( FOkToSpeak() )
				{
					if ( m_hEnemy != NULL && m_hEnemy->IsPlayer() )
						SayNamedSentence( "HG_ALERT", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
					else if ( m_hEnemy != NULL && m_hEnemy->Classify() != CLASS_PLAYER_ALLY &&
						m_hEnemy->Classify() != CLASS_HUMAN_PASSIVE && m_hEnemy->Classify() != CLASS_MACHINE )
						SayNamedSentence( "HG_MONST", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
					JustSpoke();
				}
				return GetScheduleOfType( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) ?
					SCHED_GRUNT_SUPPRESS : SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
			}
		}
// no ammo
		else if ( HasConditions( bits_COND_NO_AMMO_LOADED ) )
			return GetScheduleOfType( SCHED_GRUNT_COVER_AND_RELOAD );

// damaged just a little
		else if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
		{
			if ( RANDOM_LONG( 0, 99 ) <= 90 && m_hEnemy != NULL )
			{
				if ( FOkToSpeak() )
					m_iSentence = HGRUNT_SENT_COVER;
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
			}
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}
// can kick
		else if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
		else if ( ( pev->weapons & HGRUNT_GRENADELAUNCHER ) && HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) &&
			OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
			return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
		else if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
		{
			if ( InSquad() && MySquadLeader()->m_fEnemyEluded && !HasConditions( bits_COND_ENEMY_FACING_ME ) )
			{
				MySquadLeader()->m_fEnemyEluded = FALSE;
				return GetScheduleOfType( SCHED_GRUNT_FOUND_ENEMY );
			}
			if ( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		}
// can't see enemy
		else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
			{
				//!!!KELLY - this grunt is about to throw or fire a grenade at the player. Great place for "fire in the hole"  "frag out" etc
				if ( FOkToSpeak() )
				{
					SayNamedSentence( "HG_THROW", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
					JustSpoke();
				}
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}
			if ( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
			{
				if ( FOkToSpeak() )
					m_iSentence = HGRUNT_SENT_CHARGE;
				return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
			}
			//!!!KELLY - grunt is going to stay put for a couple seconds to see if
			// the enemy wanders back out into the open, or approaches the
			// grunt's covered position. Good place for a taunt, I guess?
			if ( FOkToSpeak() && RANDOM_LONG( 0, 1 ) )
			{
				SayNamedSentence( "HG_TAUNT", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
				JustSpoke();
			}
			return GetScheduleOfType( SCHED_STANDOFF );
		}
		if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
	}

	// no special cases here, call the base class
	return CSquadMonster::GetSchedule();
}

Schedule_t* CHGrunt::GetScheduleOfType( int type )
{
	switch ( type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		if ( InSquad() )
		{
			if ( g_iSkillLevel == SKILL_HARD && HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) &&
				OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
			{
				if ( FOkToSpeak() )
				{
					SayNamedSentence( "HG_THROW", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
					JustSpoke();
				}
				return slGruntTossGrenadeCover;
			}
			return slGruntTakeCover;
		}
		return RANDOM_LONG( 0, 1 ) ? slGruntTakeCover : slGruntGrenadeCover;
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		return slGruntTakeCoverFromBestSound;
	case SCHED_GRUNT_TAKECOVER_FAILED:
		if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
			return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
		return GetScheduleOfType( SCHED_FAIL );
	case SCHED_GRUNT_ELOF_FAIL:
		return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
	case SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE:
		return slGruntEstablishLineOfFire;
	case SCHED_RANGE_ATTACK1:
		// randomly stand or crouch
		if ( RANDOM_LONG( 0, 9 ) == 0 )
			m_fStanding = RANDOM_LONG( 0, 1 );
		return m_fStanding ? slGruntRangeAttack1B : slGruntRangeAttack1A;
	case SCHED_RANGE_ATTACK2:
		return slGruntRangeAttack2;
	case SCHED_COMBAT_FACE:
		return slGruntCombatFace;
	case SCHED_GRUNT_WAIT_FACE_ENEMY:
		return slGruntWaitInCover;
	case SCHED_GRUNT_SWEEP:
		return slGruntSweep;
	case SCHED_GRUNT_COVER_AND_RELOAD:
		return slGruntHideReload;
	case SCHED_GRUNT_FOUND_ENEMY:
		return slGruntFoundEnemy;
	case SCHED_VICTORY_DANCE:
		if ( InSquad() && !IsLeader() )
			return slGruntFail;
		return slGruntVictoryDance;
	case SCHED_GRUNT_SUPPRESS:
		if ( m_hEnemy->IsPlayer() && m_fFirstEncounter )
		{
			m_fFirstEncounter = FALSE; // after first encounter, leader won't issue handsigns anymore when he has a new enemy
			return slGruntSignalSuppress;
		}
		return slGruntSuppress;
	case SCHED_FAIL:
		return m_hEnemy != NULL ? slGruntCombatFail : slGruntFail;
	case SCHED_GRUNT_REPEL:
		if ( pev->velocity.z > -128 )
			pev->velocity.z -= 32;
		return slGruntRepel;
	case SCHED_GRUNT_REPEL_ATTACK:
		if ( pev->velocity.z > -128 )
			pev->velocity.z -= 32;
		return slGruntRepelAttack;
	case SCHED_GRUNT_REPEL_LAND:
		return slGruntRepelLand;
	default:
		return CSquadMonster::GetScheduleOfType( type );
	}
}


//=========================================================
// CHGruntRepel - when triggered, spawns a monster_human_grunt
// repelling down a line.
//=========================================================

class CHGruntRepel : public CBaseMonster
{
public:
	void Spawn();
	void Precache();
	void EXPORT RepelUse( CBaseEntity*, CBaseEntity*, USE_TYPE, float );
	int m_iSpriteTexture;	// Don't save, precache
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_grunt_repel, CHGruntRepel );

void CHGruntRepel::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	SetUse( &CHGruntRepel::RepelUse );
}

void CHGruntRepel::Precache()
{
	UTIL_PrecacheOther( "monster_human_grunt" );
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/rope.spr" );
}

void CHGruntRepel::RepelUse( CBaseEntity*, CBaseEntity*, USE_TYPE, float )
{
	TraceResult trace;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -4096 ), dont_ignore_monsters, ENT( pev ), &trace );
	CBaseEntity* entity = Create( "monster_human_grunt", pev->origin, pev->angles );
	CBaseMonster* grunt = entity->MyMonsterPointer();
	grunt->pev->movetype = MOVETYPE_FLY;
	grunt->pev->velocity = Vector( 0, 0, RANDOM_FLOAT( -196, -128 ) );
	grunt->SetActivity( ACT_GLIDE );
	grunt->m_vecLastPosition = trace.vecEndPos;
	CBeam* beam = CBeam::BeamCreate( "sprites/rope.spr", 10 );
	beam->PointEntInit( pev->origin + Vector( 0, 0, 112 ), grunt->entindex() );
	beam->SetFlags( BEAM_FSOLID );
	beam->SetColor( 255, 255, 255 );
	beam->SetThink( &CBaseEntity::SUB_Remove );
	beam->pev->nextthink = gpGlobals->time + -4096.0 * trace.flFraction / grunt->pev->velocity.z + 0.5;
	UTIL_Remove( this );
}



//=========================================================
// DEAD HGRUNT PROP
//=========================================================
class CDeadHGrunt : public CBaseMonster
{
public:
	void Spawn();
	int Classify() { return CLASS_HUMAN_MILITARY; }
	void KeyValue( KeyValueData* data );
	int m_iPose;
	static const char* m_szPoses[3];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_hgrunt_dead, CDeadHGrunt );

const char* CDeadHGrunt::m_szPoses[] = { "deadstomach", "deadside", "deadsitting" };

void CDeadHGrunt::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "pose" ) )
	{
		m_iPose = atoi( data->szValue );
		data->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( data );
}

void CDeadHGrunt::Spawn()
{
	PRECACHE_MODEL( "models/hgrunt.mdl" );
	SET_MODEL( ENT( pev ), "models/hgrunt.mdl" );
	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if ( pev->sequence == -1 )
		ALERT( at_console, "Dead hgrunt with bad pose\n" );

	// Corpses have less health
	pev->health = 8;

	// map old bodies onto new bodies
	switch ( pev->body )
	{
	case 0: // Grunt with Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( HEAD_GROUP, HEAD_GRUNT );
		SetBodygroup( GUN_GROUP, GUN_MP5 );
		break;
	case 1: // Commander with Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( HEAD_GROUP, HEAD_COMMANDER );
		SetBodygroup( GUN_GROUP, GUN_MP5 );
		break;
	case 2: // Grunt no Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( HEAD_GROUP, HEAD_GRUNT );
		SetBodygroup( GUN_GROUP, GUN_NONE );
		break;
	case 3: // Commander no Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( HEAD_GROUP, HEAD_COMMANDER );
		SetBodygroup( GUN_GROUP, GUN_NONE );
		break;
	}
	MonsterInitDead();
}

#endif
