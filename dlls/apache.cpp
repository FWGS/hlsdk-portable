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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "effects.h"
#include "triggers.h"

#define SF_WAITFORTRIGGER 0x44
#define SF_NOWRECKAGE 8

extern DLL_GLOBAL int g_iSkillLevel;

class CApache : public CBaseMonster
{
public:
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
	CApache() : m_idlesound(0), m_deathsound(0), m_firesound(0), m_quiet(FALSE) { m_classtype = CLASS_VEHICLE; }
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	void Spawn() override;
	void Precache() override;
	int Classify() override;
	int BloodColor() override;
	void Killed( entvars_t* attacker, int gib ) override;
	void GibMonster() override;
	void SetObjectCollisionBox() override;
	void EXPORT HuntThink();
	void EXPORT FlyTouch( CBaseEntity* other );
	void EXPORT CrashTouch( CBaseEntity* other );
	void EXPORT DyingThink();
	void EXPORT StartupUse( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value );
	void EXPORT NullThink();
	void ShowDamage();
	void Flight();
	void FireRocket();
	BOOL FireGun();
	int TakeDamage( entvars_t* inflictor, entvars_t* attacker, float damage, int damageType ) override;
	void TraceAttack( entvars_t* attacker, float damage, Vector direction, TraceResult* trace, int damageType ) override;
	void KeyValue( KeyValueData* data ) override;
	BOOL IsPlayerHidden() override;
	int m_iRockets;
	float m_flForce;
	float m_flNextRocket;
	Vector m_vecTarget;
	Vector m_posTarget;
	Vector m_vecDesired;
	Vector m_posDesired;
	Vector m_vecGoal;
	Vector m_angGun;
	float m_flLastSeen;
	float m_flPrevSeen;
	int m_iSoundState; // don't save this
	int m_iSpriteTexture;
	int m_iExplode;
	int m_iBodyGibs;
	float m_flGoalSpeed;
	int m_iDoSmokePuff;
	int m_idlesound;
	int m_deathsound;
	int m_firesound;
	BOOL m_quiet;
	CBeam* m_pBeam;
};

LINK_ENTITY_TO_CLASS( monster_apache, CApache );

TYPEDESCRIPTION CApache::m_SaveData[] =
{
	DEFINE_FIELD( CApache, m_iRockets, FIELD_INTEGER ),
	DEFINE_FIELD( CApache, m_flForce, FIELD_FLOAT ),
	DEFINE_FIELD( CApache, m_flNextRocket, FIELD_TIME ),
	DEFINE_FIELD( CApache, m_vecTarget, FIELD_VECTOR ),
	DEFINE_FIELD( CApache, m_posTarget, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CApache, m_vecDesired, FIELD_VECTOR ),
	DEFINE_FIELD( CApache, m_posDesired, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CApache, m_vecGoal, FIELD_VECTOR ),
	DEFINE_FIELD( CApache, m_angGun, FIELD_VECTOR ),
	DEFINE_FIELD( CApache, m_flLastSeen, FIELD_TIME ),
	DEFINE_FIELD( CApache, m_flPrevSeen, FIELD_TIME ),
//	DEFINE_FIELD( CApache, m_iSoundState, FIELD_INTEGER ),		// Don't save, precached
//	DEFINE_FIELD( CApache, m_iSpriteTexture, FIELD_INTEGER ),
//	DEFINE_FIELD( CApache, m_iExplode, FIELD_INTEGER ),
//	DEFINE_FIELD( CApache, m_iBodyGibs, FIELD_INTEGER ),
	DEFINE_FIELD( CApache, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CApache, m_flGoalSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CApache, m_iDoSmokePuff, FIELD_INTEGER ),
	DEFINE_FIELD( CApache, m_idlesound, FIELD_STRING ),
	DEFINE_FIELD( CApache, m_firesound, FIELD_STRING ),
	DEFINE_FIELD( CApache, m_deathsound, FIELD_STRING ),
	DEFINE_FIELD( CApache, m_quiet, FIELD_BOOLEAN )
};

IMPLEMENT_SAVERESTORE( CApache, CBaseMonster );

int CApache::Classify()
{
	return m_classtype;
}

int CApache::BloodColor()
{
	return DONT_BLEED;
}

void CApache::GibMonster()
{
}

void CApache::SetObjectCollisionBox()
{
	pev->absmin = pev->origin + Vector( -300, -300, -172 );
	pev->absmax = pev->origin + Vector( 300, 300, 8 );
}

void CApache::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "idlesound" ) )
		m_idlesound = FStrEq( data->szValue, "default" ) ? 0 : ALLOC_STRING( data->szValue );
	else if ( FStrEq( data->szKeyName, "deathsound" ) )
		m_deathsound = FStrEq( data->szValue, "default" ) ? 0 : ALLOC_STRING( data->szValue );
	else if ( FStrEq( data->szKeyName, "quiet" ) )
		m_quiet = atoi( data->szValue ) != 0;
	else if ( FStrEq( data->szKeyName, "firesound" ) )
		m_firesound = FStrEq( data->szValue, "default" ) ? 0 : ALLOC_STRING( data->szValue );
	else
	{
		CBaseMonster::KeyValue( data );
		return;
	}
	data->fHandled = TRUE;
}

void CApache::Precache()
{
	PRECACHE_MODEL( pev->model ? (char*)STRING( pev->model ) : "models/apache.mdl" );
	PRECACHE_SOUND( "apache/ap_rotor2.wav" );
	PRECACHE_SOUND( "turret/tu_fire1.wav" );
	PRECACHE_SOUND( "weapons/mortarhit.wav" );
	if ( m_idlesound )
		PRECACHE_SOUND( (char*)STRING( m_idlesound ) );
	if ( m_deathsound )
		PRECACHE_SOUND( (char*)STRING( m_deathsound ) );
	if ( m_firesound )
		PRECACHE_SOUND( (char*)STRING( m_firesound ) );
	PRECACHE_MODEL( "sprites/lgtning.spr" );
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/white.spr" );
	m_iExplode = PRECACHE_MODEL( "sprites/fexplo.spr" );
	m_iBodyGibs = PRECACHE_MODEL( "models/metalplategibs_green.mdl" );
	UTIL_PrecacheOther( "hvr_rocket" );
}

void CApache::Spawn()
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	SET_MODEL( edict(), pev->model ? STRING( pev->model ) : "models/apache.mdl" );
	UTIL_SetSize( pev, Vector( -64, -64, -128 ), Vector( 64, 64, 0 ) );
	UTIL_SetOrigin( pev, pev->origin );
	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_AIM;
	if ( pev->health == 0 )
		pev->health = gSkillData.apacheHealth;
	m_flFieldOfView = -0.707f; // 270 degrees
	pev->sequence = 0;
	ResetSequenceInfo();
	pev->frame = (float)RANDOM_LONG( 0, 255 );
	InitBoneControllers();
	if ( pev->spawnflags & SF_WAITFORTRIGGER )
		SetUse( &CApache::StartupUse );
	else
	{
		SetThink( &CApache::HuntThink );
		SetTouch( &CApache::FlyTouch );
		pev->nextthink = gpGlobals->time + 1.0;
	}
	m_iRockets = 10;
}

void CApache::StartupUse( CBaseEntity*, CBaseEntity*, USE_TYPE, float )
{
	SetThink( &CApache::HuntThink );
	SetTouch( &CApache::FlyTouch );
	pev->nextthink = gpGlobals->time + 0.1;
	SetUse( NULL );
}

void CApache::NullThink()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.5;
	FCheckAITrigger();
}

BOOL CApache::IsPlayerHidden()
{
	CBasePlayer* player = (CBasePlayer*)CBaseEntity::Instance( INDEXENT( 1 ) );
	if ( !player->m_stealthTrigger )
		return FALSE;
	BOOL hidden = player->m_stealthTrigger->IsPlayerHidden();
	if ( hidden )
		return TRUE;
	player->m_stealthTrigger = NULL;
	return hidden;
}

void CApache::Killed( entvars_t*, int )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->gravity = 0.3f;
	STOP_SOUND( edict(), CHAN_STATIC, m_idlesound ? STRING( m_idlesound ) : "apache/ap_rotor2.wav" );
	UTIL_SetSize( pev, Vector( -32, -32, -64 ), Vector( 32, 32, 0 ) );
	SetThink( &CApache::DyingThink );
	SetTouch( &CApache::CrashTouch );
	pev->nextthink = gpGlobals->time + 0.1;
	pev->health = 0;
	pev->takedamage = DAMAGE_NO;
	if ( pev->spawnflags & SF_NOWRECKAGE )
		m_flNextRocket = gpGlobals->time + 4.0;
	else
		m_flNextRocket = gpGlobals->time + 15.0;
}

void CApache::FlyTouch( CBaseEntity* other )
{
	// bounce if we hit something solid
	if ( other->pev->solid == SOLID_BSP )
	{
		TraceResult trace = UTIL_GetGlobalTrace();
		pev->velocity = pev->velocity + trace.vecPlaneNormal * (pev->velocity.Length() + 200);
	}
}

void CApache::CrashTouch( CBaseEntity* other )
{
	// only crash if we hit something solid
	if ( other->pev->solid == SOLID_BSP )
	{
		SetTouch( NULL );
		m_flNextRocket = gpGlobals->time;
		pev->nextthink = gpGlobals->time;
	}
}

int CApache::TakeDamage( entvars_t* inflictor, entvars_t* attacker, float damage, int damageType )
{
	if ( inflictor->owner == edict() )
		return 0;
	if ( damageType & DMG_BLAST )
		damage *= 2;
	return CBaseEntity::TakeDamage( inflictor, attacker, damage, damageType );
}

void CApache::TraceAttack( entvars_t* attacker, float damage, Vector, TraceResult* trace, int damageType )
{
	// ALERT( at_console, "%d %.0f\n", ptr->iHitgroup, flDamage );

	// ignore blades
	if ( trace->iHitgroup == 6 && (damageType & (DMG_ENERGYBEAM | DMG_BULLET | DMG_CLUB)) )
		return;

	// hit hard, hits cockpit, hits engines
	if ( damage > 50 || trace->iHitgroup == 1 || trace->iHitgroup == 2 )
	{
		AddMultiDamage( attacker, this, damage, damageType );
		m_iDoSmokePuff = (int)(damage / 5.0 + 3);
	}
	else
		UTIL_Ricochet( trace->vecEndPos, 2 );
}

void CApache::ShowDamage()
{
	if ( m_iDoSmokePuff > 0 || RANDOM_LONG( 0, 99 ) > pev->health )
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z - 32 );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 20 + RANDOM_LONG( 0, 9 ) ); // scale * 10
			WRITE_BYTE( 12 ); // framerate
		MESSAGE_END();
	}
	if ( m_iDoSmokePuff > 0 )
		--m_iDoSmokePuff;
}

void CApache::HuntThink()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;
	ShowDamage();
	if ( !m_pGoalEnt && pev->target )
	{
		m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
		if ( m_pGoalEnt )
		{
			m_posDesired = m_pGoalEnt->pev->origin;
			UTIL_MakeAimVectors( m_pGoalEnt->pev->angles );
			m_vecGoal = gpGlobals->v_forward;
		}
	}
	Look( 4092 );
	m_hEnemy = BestVisibleEnemy();

	// generic speed up
	if ( m_flGoalSpeed < 800 )
		m_flGoalSpeed += 5;
	if ( m_hEnemy )
	{
		// ALERT( at_console, "%s\n", STRING( m_hEnemy->pev->classname ) );
		if ( FVisible( m_hEnemy ) )
		{
			if ( m_flLastSeen < gpGlobals->time - 5 )
				m_flPrevSeen = gpGlobals->time;
			m_flLastSeen = gpGlobals->time;
			m_posTarget = m_hEnemy->Center();
		}
		else
			m_hEnemy = NULL;
	}
	m_vecTarget = (m_posTarget - pev->origin).Normalize();
	float distance = (pev->origin - m_posDesired).Length();
	if ( m_pGoalEnt )
	{
		// ALERT( at_console, "%.0f\n", flLength );

		if ( distance < 128 )
		{
			m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( m_pGoalEnt->pev->target ) );
			if ( m_pGoalEnt )
			{
				m_posDesired = m_pGoalEnt->pev->origin;
				UTIL_MakeAimVectors( m_pGoalEnt->pev->angles );
				m_vecGoal = gpGlobals->v_forward;
				distance = (pev->origin - m_posDesired).Length();
			}
		}
	}
	else
		m_posDesired = pev->origin;
	if ( distance > 250 )
	{
		if ( m_flLastSeen + 90 > gpGlobals->time
			&& DotProduct( (m_posTarget - pev->origin).Normalize(), (m_posDesired - pev->origin).Normalize() ) > 0.25f )
			m_vecDesired = (m_posTarget - pev->origin).Normalize();
		else
			m_vecDesired = (m_posDesired - pev->origin).Normalize();
	}
	else
		m_vecDesired = m_vecGoal;
	Flight();

	// ALERT( at_console, "%.0f %.0f %.0f\n", gpGlobals->time, m_flLastSeen, m_flPrevSeen );
	if ( m_flLastSeen + 1 > gpGlobals->time && m_flPrevSeen + 2 < gpGlobals->time )
	{
		if ( FireGun() && m_flGoalSpeed > 400 )
			m_flGoalSpeed = 400;

		// don't fire rockets and gun on easy mode
		if ( g_iSkillLevel == SKILL_EASY )
			m_flNextRocket = gpGlobals->time + 10.0;
	}
	UTIL_MakeAimVectors( pev->angles );
	Vector estimate = (gpGlobals->v_forward * 800 + pev->velocity).Normalize();
	if ( m_iRockets % 2 == 1 )
	{
		FireRocket();
		m_flNextRocket = gpGlobals->time + 0.5;
		if ( m_iRockets <= 0 )
		{
			m_flNextRocket = gpGlobals->time + 10;
			m_iRockets = 10;
		}
	}
	else if ( pev->angles.x < 0 && DotProduct( pev->velocity, gpGlobals->v_forward ) > -100
		&& m_flNextRocket < gpGlobals->time && m_flLastSeen + 60 > gpGlobals->time )
	{
		TraceResult trace;
		bool traceTarget = true;
		if ( m_hEnemy )
		{
			if ( DotProduct( m_vecTarget, estimate ) > 0.965 )
				UTIL_TraceLine( pev->origin, pev->origin + estimate * 4096, ignore_monsters, edict(), &trace );
			else
				traceTarget = false;
		}
		else
			UTIL_TraceLine( pev->origin, pev->origin + estimate * 4096, dont_ignore_monsters, edict(), &trace );
		if ( traceTarget && (trace.vecEndPos - m_posTarget).Length() < 512 )
			FireRocket();
	}
	FCheckAITrigger();
}

void CApache::Flight()
{
	Vector adjustment( 5, 0, 0 );
	UTIL_MakeAimVectors( pev->angles + pev->avelocity * 2 + adjustment );
	float side = DotProduct( m_vecDesired, gpGlobals->v_right );
	if ( side < 0 )
	{
		if ( pev->avelocity.y < 60 )
			pev->avelocity.y += 8; // 9 * (3.0/2.0);
	}
	else if ( pev->avelocity.y > -60 )
		pev->avelocity.y -= 8; // 9 * (3.0/2.0);
	pev->avelocity.y = (float)(pev->avelocity.y * 0.98);
	UTIL_MakeAimVectors( pev->angles + pev->avelocity + adjustment );
	Vector estimate = pev->origin + pev->velocity * 2 + gpGlobals->v_up * m_flForce * 20 - Vector( 0, 0, 768 );
	UTIL_MakeAimVectors( pev->angles + adjustment );
	pev->velocity = pev->velocity + gpGlobals->v_up * m_flForce;
	pev->velocity.z = (float)(pev->velocity.z - 38.4);
	float speed = pev->velocity.Length();
	if ( gpGlobals->v_forward.x * pev->velocity.x + gpGlobals->v_forward.y * pev->velocity.y < 0 )
		speed = -speed;
	float distance = DotProduct( m_posDesired - estimate, gpGlobals->v_forward );
	float slip = -DotProduct( m_posDesired - estimate, gpGlobals->v_right );
	if ( slip > 0 )
	{
		if ( pev->angles.z > -30 && pev->avelocity.z > -15 )
			pev->avelocity.z -= 4;
		else
			pev->avelocity.z += 2;
	}
	else
	{
		if ( pev->angles.z < 30 && pev->avelocity.z < 15 )
			pev->avelocity.z += 4;
		else
			pev->avelocity.z -= 2;
	}
	pev->velocity.x = (float)(pev->velocity.x * (1 - fabs( gpGlobals->v_right.x ) * 0.05));
	pev->velocity.y = (float)(pev->velocity.y * (1 - fabs( gpGlobals->v_right.y ) * 0.05));
	pev->velocity.z = (float)(pev->velocity.z * (1 - fabs( gpGlobals->v_right.z ) * 0.05));
	pev->velocity = pev->velocity * 0.995f;
	if ( m_flForce < 80 && estimate.z < m_posDesired.z )
		m_flForce += 12;
	else if ( m_flForce > 30 && estimate.z > m_posDesired.z )
		m_flForce -= 8;
	if ( distance > 0 && speed < m_flGoalSpeed && pev->angles.x + pev->avelocity.x > -40 )
		pev->avelocity.x -= 12.0;
	else if ( distance < 0 && speed > -50 && pev->angles.x + pev->avelocity.x < 20 )
		pev->avelocity.x += 12.0;
	else if ( pev->angles.x + pev->avelocity.x > 0 )
		pev->avelocity.x -= 4.0;
	else if ( pev->angles.x + pev->avelocity.x < 0 )
		pev->avelocity.x += 4.0;
	const char* rotor = m_idlesound ? STRING( m_idlesound ) : "apache/ap_rotor2.wav";
	if ( !m_iSoundState )
	{
		if ( !m_quiet )
			EMIT_SOUND_DYN( edict(), CHAN_STATIC, rotor, 1, 0.3f, 0, 110 );
		// EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "apache/ap_whine1.wav", 0.5, 0.2, 0, 110 );

		m_iSoundState = SND_CHANGE_PITCH; // hack for going through level transitions
	}
	else
	{
		CBaseEntity* player = UTIL_FindEntityByClassname( NULL, "player" );
		if ( player )
		{
			float pitch = DotProduct( pev->velocity - player->pev->velocity, (player->pev->origin - pev->origin).Normalize() );
			pitch = (int)(100 + pitch / 50.0);
			if ( pitch > 250 )
				pitch = 250;
			if ( pitch < 50 )
				pitch = 50;
			if ( pitch == 100 )
				pitch = 101;
			if ( !m_quiet )
				EMIT_SOUND_DYN( edict(), CHAN_STATIC, rotor, 1, 0.3f, SND_CHANGE_PITCH | SND_CHANGE_VOL, pitch );
		}
		// EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "apache/ap_whine1.wav", flVol, 0.2, SND_CHANGE_PITCH | SND_CHANGE_VOL, pitch);

		// ALERT( at_console, "%.0f %.2f\n", pitch, flVol );
	}
}

void CApache::FireRocket()
{
	static float side = 1;
	if ( m_iRockets <= 0 )
		return;
	UTIL_MakeAimVectors( pev->angles );
	Vector source = pev->origin + (gpGlobals->v_forward * 21 + gpGlobals->v_right * 70 * side - gpGlobals->v_up * 79) * 1.5f;
	switch ( m_iRockets % 5 )
	{
	case 0: source = source + gpGlobals->v_right * 10; break;
	case 1: source = source - gpGlobals->v_right * 10; break;
	case 2: source = source + gpGlobals->v_up * 10; break;
	case 3: source = source - gpGlobals->v_up * 10; break;
	default: break;
	}
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, source );
		WRITE_BYTE( TE_SMOKE );
		WRITE_COORD( source.x );
		WRITE_COORD( source.y );
		WRITE_COORD( source.z );
		WRITE_SHORT( g_sModelIndexSmoke );
		WRITE_BYTE( 20 ); // scale * 10
		WRITE_BYTE( 12 ); // framerate
	MESSAGE_END();
	CBaseEntity* rocket = CBaseEntity::Create( "hvr_rocket", source, pev->angles, edict() );
	if ( rocket )
		rocket->pev->velocity = pev->velocity + gpGlobals->v_forward * 100;
	--m_iRockets;
	side = -side;
}

BOOL CApache::FireGun()
{
	UTIL_MakeAimVectors( pev->angles );
	Vector gun, gunAngles;
	GetAttachment( 1, gun, gunAngles );
	Vector target = (m_posTarget - gun).Normalize();
	Vector direction;
	direction.x = DotProduct( gpGlobals->v_forward, target );
	direction.y = -DotProduct( gpGlobals->v_right, target );
	direction.z = DotProduct( gpGlobals->v_up, target );
	Vector angles = UTIL_VecToAngles( direction );
	angles.x = -angles.x;
	if ( angles.y > 180 )
		angles.y -= 360;
	if ( angles.y < -180 )
		angles.y += 360;
	if ( angles.x > 180 )
		angles.x -= 360;
	if ( angles.x < -180 )
		angles.x += 360;
	if ( angles.x > m_angGun.x )
		m_angGun.x = min( angles.x, m_angGun.x + 12 );
	if ( angles.x < m_angGun.x )
		m_angGun.x = max( angles.x, m_angGun.x - 12 );
	if ( angles.y > m_angGun.y )
		m_angGun.y = min( angles.y, m_angGun.y + 12 );
	if ( angles.y < m_angGun.y )
		m_angGun.y = max( angles.y, m_angGun.y - 12 );
	m_angGun.y = SetBoneController( 0, m_angGun.y );
	m_angGun.x = SetBoneController( 1, m_angGun.x );
	Vector barrel, barrelAngles;
	GetAttachment( 0, barrel, barrelAngles );
	Vector gunDirection = (barrel - gun).Normalize();
	if ( DotProduct( gunDirection, target ) > 0.98 )
	{
		FireBullets( 1, gun, gunDirection, VECTOR_CONE_10DEGREES, 8192, BULLET_MONSTER_12MM, 1 );
		if ( !m_quiet )
			EMIT_SOUND( edict(), CHAN_WEAPON, m_firesound ? STRING( m_firesound ) : "turret/tu_fire1.wav", 1, 0.3f );
		return TRUE;
	}
	if ( m_pBeam )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}
	return FALSE;
}

void CApache::DyingThink()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;
	pev->avelocity = pev->avelocity * 1.02f;

	// still falling?
	if ( m_flNextRocket > gpGlobals->time )
	{
		// random explosions
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION );		// This just makes a dynamic light now
			WRITE_COORD( pev->origin.x + RANDOM_FLOAT( -150, 150 ) );
			WRITE_COORD( pev->origin.y + RANDOM_FLOAT( -150, 150 ) );
			WRITE_COORD( pev->origin.z + RANDOM_FLOAT( -150, -50 ) );
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( 30 + RANDOM_LONG( 0, 29 ) ); // scale * 10
			WRITE_BYTE( 12 ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NONE );
		MESSAGE_END();

		// lots of smoke
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x + RANDOM_FLOAT( -150, 150 ) );
			WRITE_COORD( pev->origin.y + RANDOM_FLOAT( -150, 150 ) );
			WRITE_COORD( pev->origin.z + RANDOM_FLOAT( -150, -50 ) );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 100 ); // scale * 10
			WRITE_BYTE( 10 ); // framerate
		MESSAGE_END();
		Vector spot = pev->origin + (pev->mins + pev->maxs) * 0.5f;
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, spot );
			WRITE_BYTE( TE_BREAKMODEL );
			WRITE_COORD( spot.x );
			WRITE_COORD( spot.y );
			WRITE_COORD( spot.z );

			// size
			WRITE_COORD( 400 );
			WRITE_COORD( 400 );
			WRITE_COORD( 132 );

			// velocity
			WRITE_COORD( pev->velocity.x );
			WRITE_COORD( pev->velocity.y );
			WRITE_COORD( pev->velocity.z );

			// randomization
			WRITE_BYTE( 50 );

			// Model
			WRITE_SHORT( m_iBodyGibs );	//model id#

			// # of shards
			WRITE_BYTE( 4 );	// let client decide

			// duration
			WRITE_BYTE( 30 ); // 3.0 seconds

			// flags

			WRITE_BYTE( BREAK_METAL );
		MESSAGE_END();

		// don't stop it we touch a entity
		pev->flags &= ~FL_ONGROUND;
		pev->nextthink = gpGlobals->time + 0.2;
		return;
	}
	Vector spot = pev->origin + (pev->mins + pev->maxs) * 0.5f;
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, spot );
		WRITE_BYTE( TE_SPRITE );
		WRITE_COORD( spot.x );
		WRITE_COORD( spot.y );
		WRITE_COORD( spot.z + 256 );
		WRITE_SHORT( m_iExplode );
		WRITE_BYTE( 120 ); // scale * 10
		WRITE_BYTE( 255 ); // brightness
	MESSAGE_END();

	// big smoke
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, spot );
		WRITE_BYTE( TE_SMOKE );
		WRITE_COORD( spot.x );
		WRITE_COORD( spot.y );
		WRITE_COORD( spot.z + 512 );
		WRITE_SHORT( g_sModelIndexSmoke );
		WRITE_BYTE( 250 ); // scale * 10
		WRITE_BYTE( 5 ); // framerate
	MESSAGE_END();
	if ( !m_quiet )
		EMIT_SOUND( edict(), CHAN_STATIC, m_deathsound ? STRING( m_deathsound ) : "weapons/mortarhit.wav", 1, 0.3f );
	RadiusDamage( pev->origin, pev, pev, 300, CLASS_NONE, DMG_BLAST );
	if ( pev->flags & FL_ONGROUND )
	{
		CBaseEntity* wreckage = Create( "cycler_wreckage", pev->origin, pev->angles );
		UTIL_SetSize( wreckage->pev, Vector( -200, -200, -128 ), Vector( 200, 200, -32 ) );
		wreckage->pev->frame = pev->frame;
		wreckage->pev->sequence = pev->sequence;
		wreckage->pev->framerate = 0;
		wreckage->pev->dmgtime = gpGlobals->time + 5;
	}
	spot = pev->origin + (pev->mins + pev->maxs) * 0.5f;
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, spot );
		WRITE_BYTE( TE_BREAKMODEL );
		WRITE_COORD( spot.x );
		WRITE_COORD( spot.y );
		WRITE_COORD( spot.z + 64 );

		// size
		WRITE_COORD( 400 );
		WRITE_COORD( 400 );
		WRITE_COORD( 128 );

		// velocity
		WRITE_COORD( 0 );
		WRITE_COORD( 0 );
		WRITE_COORD( 200 );

		// randomization
		WRITE_BYTE( 30 );

		// Model
		WRITE_SHORT( m_iBodyGibs );	//model id#

		// # of shards
		WRITE_BYTE( 200 );

		// duration
		WRITE_BYTE( 200 ); // 10.0 seconds

		// flags

		WRITE_BYTE( BREAK_METAL );
	MESSAGE_END();
	SetThink( &CBaseEntity::SUB_Remove );
	pev->deadflag = DEAD_DEAD;
	pev->nextthink = gpGlobals->time + 0.1;
	FCheckAITrigger();
}

class CApacheHVR : public CGrenade
{
	void Spawn() override;
	void Precache() override;
	void EXPORT IgniteThink();
	void EXPORT AccelerateThink();
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	int m_iTrail;
	Vector m_vecForward;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( hvr_rocket, CApacheHVR );

TYPEDESCRIPTION CApacheHVR::m_SaveData[] =
{
//	DEFINE_FIELD( CApacheHVR, m_iTrail, FIELD_INTEGER ),	// Dont' save, precache
	DEFINE_FIELD( CApacheHVR, m_vecForward, FIELD_VECTOR )
};

IMPLEMENT_SAVERESTORE( CApacheHVR, CGrenade );

void CApacheHVR::Precache()
{
	PRECACHE_MODEL( "models/HVR.mdl" );
	m_iTrail = PRECACHE_MODEL( "sprites/smoke.spr" );
	PRECACHE_SOUND( "weapons/rocket1.wav" );
}

void CApacheHVR::Spawn()
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	SET_MODEL( edict(), "models/HVR.mdl" );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	UTIL_SetOrigin( pev, pev->origin );
	SetThink( &CApacheHVR::IgniteThink );
	SetTouch( &CGrenade::ExplodeTouch );
	UTIL_MakeAimVectors( pev->angles );
	m_vecForward = gpGlobals->v_forward;
	pev->gravity = 0.5f;
	pev->nextthink = gpGlobals->time + 0.1;
	pev->dmg = 150;
}

void CApacheHVR::IgniteThink()
{
	// pev->movetype = MOVETYPE_TOSS;

	// pev->movetype = MOVETYPE_FLY;
	pev->effects |= EF_LIGHT;

	// make rocket sound
	EMIT_SOUND( edict(), CHAN_VOICE, "weapons/rocket1.wav", 1, 0.5f );

	// rocket trail
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() );	// entity
		WRITE_SHORT( m_iTrail );	// model
		WRITE_BYTE( 15 ); // life
		WRITE_BYTE( 5 );  // width
		WRITE_BYTE( 224 );   // r, g, b
		WRITE_BYTE( 224 );   // r, g, b
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 255 );	// brightness
	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

	// set to accelerate
	SetThink( &CApacheHVR::AccelerateThink );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CApacheHVR::AccelerateThink()
{
	if ( !(pev->origin.x >= -4096 && pev->origin.x <= 4096 && pev->origin.y >= -4096 && pev->origin.y <= 4096
		&& pev->origin.z >= -4096 && pev->origin.z <= 4096) )
	{
		UTIL_Remove( this );
		return;
	}
	if ( pev->velocity.Length() < 1800 )
		pev->velocity = pev->velocity + m_vecForward * 200;

	// re-aim
	pev->angles = UTIL_VecToAngles( pev->velocity );
	pev->nextthink = gpGlobals->time + 0.1;
}
