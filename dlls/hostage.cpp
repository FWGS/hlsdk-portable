#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "game.h"
#include "hostage.h"
#include "hostage_localnav.h"

LINK_ENTITY_TO_CLASS( hostage_entity, CHostage );
LINK_ENTITY_TO_CLASS( monster_hostage, CHostage );

int g_iHostageNumber;

void UTIL_DrawBeamPoints( Vector source, Vector destination, int lifetime, byte red, byte green, byte blue );

enum
{
	HOSTAGE_INJURED_HINT = 1 << 5,
	HOSTAGE_CT_FOLLOW_HINT = 1 << 21,
};

TYPEDESCRIPTION CHostage::m_SaveData[] =
{
	DEFINE_FIELD( CHostage, m_bTouched, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHostage, m_bRescueMe, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHostage, m_rescued, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHostage, m_flFlinchTime, FIELD_FLOAT ),
	DEFINE_FIELD( CHostage, m_flNextChange, FIELD_FLOAT ),
	DEFINE_FIELD( CHostage, m_flMarkPosition, FIELD_FLOAT ),
	DEFINE_FIELD( CHostage, m_State, FIELD_INTEGER ),
	DEFINE_FIELD( CHostage, m_vStart, FIELD_VECTOR ),
	DEFINE_FIELD( CHostage, m_vStartAngles, FIELD_VECTOR ),
	DEFINE_ARRAY( CHostage, m_vPathToFollow, FIELD_VECTOR, 20 ),
	DEFINE_FIELD( CHostage, m_voicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( CHostage, m_target, FIELD_CLASSPTR ),
	DEFINE_FIELD( CHostage, nTargetNode, FIELD_INTEGER ),
	DEFINE_ARRAY( CHostage, vecNodes, FIELD_VECTOR, 100 ),
	DEFINE_FIELD( CHostage, m_hStoppedTargetEnt, FIELD_EHANDLE ),
	DEFINE_FIELD( CHostage, m_flNextFullThink, FIELD_FLOAT ),
	DEFINE_FIELD( CHostage, m_flPathCheckInterval, FIELD_FLOAT ),
	DEFINE_FIELD( CHostage, m_flLastPathCheck, FIELD_FLOAT ),
	DEFINE_FIELD( CHostage, m_nPathNodes, FIELD_INTEGER ),
	DEFINE_FIELD( CHostage, m_fHasPath, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHostage, m_flPathCheckInterval, FIELD_FLOAT ),
	DEFINE_FIELD( CHostage, m_rescueTarget, FIELD_STRING ),
	DEFINE_FIELD( CHostage, m_rescueAnim, FIELD_STRING ),
	DEFINE_FIELD( CHostage, m_noAutoRemove, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHostage, m_iHostageIndex, FIELD_INTEGER ),
	DEFINE_FIELD( CHostage, m_vOldPos, FIELD_VECTOR ),
	DEFINE_FIELD( CHostage, m_flNextRadarTime, FIELD_FLOAT ),
	DEFINE_FIELD( CHostage, m_Activity, FIELD_INTEGER ),
	DEFINE_FIELD( CHostage, m_flPathAcquired, FIELD_FLOAT ),
	DEFINE_FIELD( CHostage, m_flNextPainTime, FIELD_TIME ),
};

int CHostage::Save( CSave& save )
{
	if ( !CBaseMonster::Save( save ) )
		return 0;
	return save.WriteFields( "CHostage", this, m_SaveData, ARRAYSIZE( m_SaveData ) );
}

int CHostage::Restore( CRestore& restore )
{
	if ( !CBaseMonster::Restore( restore ) )
		return 0;
	int result = restore.ReadFields( "CHostage", this, m_SaveData, ARRAYSIZE( m_SaveData ) );
	m_LocalNav = new CLocalNav( this );
	return result;
}

int CHostage::BloodColor()
{
	return BLOOD_COLOR_RED;
}

int CHostage::Classify()
{
	return CLASS_HUMAN_PASSIVE;
}

int CHostage::ObjectCaps()
{
	return FCAP_ACROSS_TRANSITION | FCAP_IMPULSE_USE;
}

Schedule_t* CHostage::GetSchedule()
{
	return GetScheduleOfType( SCHED_IDLE_STAND );
}

void CHostage::Restart()
{
	CLocalNav::Reset();
}

void CHostage::Precache()
{
	PRECACHE_MODEL( pev->model ? STRING( pev->model ) : "models/npc-scientist.mdl" );
	PRECACHE_MODEL( "sprites/smoke.spr" );
}

void CHostage::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "rescuetarget" ) )
	{
		m_rescueTarget = ALLOC_STRING( data->szValue );
		data->fHandled = TRUE;
	}
	if ( FStrEq( data->szKeyName, "rescueanim" ) )
	{
		m_rescueAnim = ALLOC_STRING( data->szValue );
		data->fHandled = TRUE;
	}
	if ( FStrEq( data->szKeyName, "noautoremove" ) )
	{
		m_noAutoRemove = atoi( data->szValue );
		data->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( data );
}

void CHostage::SetActivity( int activity )
{
	if ( m_Activity == activity )
		return;
	int sequence = LookupActivity( activity );
	if ( sequence == -1 )
		return;
	if ( pev->sequence != sequence )
	{
		if ( ( activity != ACT_WALK && activity != ACT_RUN ) ||
			( m_Activity != ACT_WALK && m_Activity != ACT_RUN ) )
			pev->frame = 0;
		pev->sequence = sequence;
	}
	m_Activity = activity;
	ResetSequenceInfo();
}

void CHostage::SetYawSpeed()
{
	switch ( m_Activity )
	{
	case ACT_IDLE:
	case ACT_RUN:
		pev->yaw_speed = 150;
		break;
	case ACT_WALK:
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		pev->yaw_speed = 180;
		break;
	case ACT_FLY:
	case ACT_GLIDE:
		pev->yaw_speed = 30;
		break;
	case ACT_RANGE_ATTACK1:
	case ACT_RANGE_ATTACK2:
	case ACT_MELEE_ATTACK1:
	case ACT_MELEE_ATTACK2:
		pev->yaw_speed = 120;
		break;
	default:
		pev->yaw_speed = 90;
		break;
	}
}

void CHostage::PlayPainSound()
{
	if ( gpGlobals->time > m_flNextPainTime )
	{
		SayNamedSentence( "H_PAIN", 1, ATTN_NORM, 0, (int)m_voicePitch );
		m_flNextPainTime = gpGlobals->time + 1;
	}
}

float CHostage::GetModifiedDamage( float damage, int hitgroup )
{
	switch ( hitgroup )
	{
	case HITGROUP_GENERIC:
	case HITGROUP_STOMACH:
		return (float)( damage * 1.75 );
	case HITGROUP_HEAD:
		return (float)( damage * 2.5 );
	case HITGROUP_LEFTARM:
	case HITGROUP_RIGHTARM:
		return (float)( damage * 0.75 );
	case HITGROUP_LEFTLEG:
	case HITGROUP_RIGHTLEG:
		return (float)( damage * 0.6 );
	default:
		return (float)( damage * 1.5 );
	}
}

void CHostage::SetFlinchActivity()
{
	m_flFlinchTime = gpGlobals->time + 0.75f;
	switch ( m_LastHitGroup )
	{
	case HITGROUP_LEFTARM:
		SetActivity( ACT_FLINCH_LEFTARM );
		break;
	case HITGROUP_RIGHTARM:
		SetActivity( ACT_FLINCH_RIGHTARM );
		break;
	case HITGROUP_LEFTLEG:
		SetActivity( ACT_FLINCH_LEFTLEG );
		break;
	case HITGROUP_RIGHTLEG:
		SetActivity( ACT_FLINCH_RIGHTLEG );
		break;
	default:
		SetActivity( ACT_SMALL_FLINCH );
		break;
	}
}

void CHostage::SetDeathActivity()
{
	switch ( m_LastHitGroup )
	{
	case HITGROUP_CHEST:
	case HITGROUP_RIGHTARM:
		SetActivity( ACT_DIESIMPLE );
		break;
	case HITGROUP_STOMACH:
	case HITGROUP_RIGHTLEG:
		SetActivity( ACT_DIEFORWARD );
		break;
	case HITGROUP_LEFTARM:
	case HITGROUP_LEFTLEG:
		SetActivity( ACT_DIEBACKWARD );
		break;
	default:
		SetActivity( ACT_DIE_HEADSHOT );
		break;
	}
}

BOOL CHostage::IsOnLadder()
{
	return pev->movetype == MOVETYPE_FLY;
}

void CHostage::PointAt( Vector& point )
{
	pev->angles.x = 0;
	pev->angles.y = UTIL_VecToAngles( point - pev->origin ).y;
	pev->angles.z = 0;
}

void CHostage::Remove()
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	pev->nextthink = -1;
	m_flNextFullThink = -1;
}

void CHostage::RePosition()
{
	pev->health = pev->max_health;
	pev->movetype = MOVETYPE_STEP;
	pev->solid = SOLID_SLIDEBOX;
	pev->takedamage = DAMAGE_YES;
	pev->deadflag = DEAD_NO;
	pev->velocity = g_vecZero;
	pev->angles = m_vStartAngles;
	pev->effects &= ~EF_NODRAW;
	m_hTargetEnt = NULL;
	m_hStoppedTargetEnt = NULL;
	m_bTouched = FALSE;
	m_bRescueMe = FALSE;
	m_flNextRadarTime = 0;
	m_vOldPos = Vector( 9999, 9999, 9999 );
	UTIL_SetOrigin( pev, m_vStart );
	UTIL_SetSize( pev, Vector( -10, -10, 0 ), Vector( 10, 10, 62 ) );
	DROP_TO_FLOOR( edict() );
	SetActivity( ACT_IDLE );
	SetThink( &CHostage::IdleThink );
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 0.1f, 0.2f );
	m_flNextFullThink = gpGlobals->time + RANDOM_FLOAT( 0.1f, 0.2f );
	nTargetNode = -1;
	m_fHasPath = FALSE;
	m_flLastPathCheck = -1;
	m_flPathAcquired = -1;
	m_flPathCheckInterval = 0.1f;
}

void CHostage::Spawn()
{
	if ( pev->model && ( strstr( STRING( pev->model ), "us_gal" ) || strstr( STRING( pev->model ), "schoolgirl" ) ) )
		m_femaleFlag = TRUE;
	Precache();
	pev->classname = MAKE_STRING( "monster_hostage" );
	pev->movetype = MOVETYPE_STEP;
	pev->solid = SOLID_SLIDEBOX;
	pev->takedamage = DAMAGE_YES;
	pev->flags |= FL_MONSTER;
	pev->deadflag = DEAD_NO;
	pev->max_health = 100;
	pev->health = pev->max_health;
	pev->gravity = 1;
	pev->view_ofs = Vector( 0, 0, 12 );
	pev->velocity = g_vecZero;
	m_flNextPainTime = gpGlobals->time;
	if ( pev->spawnflags & SF_MONSTER_HITMONSTERCLIP )
		pev->flags |= FL_MONSTERCLIP;
	if ( pev->skin < 0 )
		pev->skin = 0;
	if ( m_head > 0 )
		pev->body = m_head;
	SET_MODEL( edict(), pev->model ? STRING( pev->model ) : "models/npc-scientist.mdl" );
	SetActivity( ACT_IDLE );
	m_State = STAND;
	m_flNextChange = 0;
	m_hTargetEnt = NULL;
	m_hStoppedTargetEnt = NULL;
	m_flFlinchTime = 0;
	m_vPathToFollow[0] = g_vecZero;
	m_bRescueMe = FALSE;
	m_rescued = FALSE;
	UTIL_SetSize( pev, Vector( -10, -10, 0 ), Vector( 10, 10, 62 ) );
	UTIL_MakeVectors( pev->v_angle );
	SetBoneController( 0, UTIL_VecToYaw( gpGlobals->v_forward ) );
	DROP_TO_FLOOR( edict() );
	MonsterInit();
	SetThink( &CHostage::IdleThink );
	SetUse( &CHostage::FollowUse );
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 0.1f, 0.2f );
	m_flNextFullThink = gpGlobals->time + RANDOM_FLOAT( 0.1f, 0.2f );
	m_vStart = pev->origin;
	m_vStartAngles = pev->angles;
	m_vOldPos = Vector( 9999, 9999, 9999 );
	m_iHostageIndex = ++g_iHostageNumber;
	m_flNextRadarTime = gpGlobals->time + RANDOM_FLOAT( 0, 1 );
	nTargetNode = -1;
	m_fHasPath = FALSE;
	m_flLastPathCheck = -1;
	m_flPathAcquired = -1;
	m_flPathCheckInterval = 0.1f;
	m_LocalNav = new CLocalNav( this );
	m_voicePitch = 100;
	if ( !m_language )
		m_language = ALLOC_STRING( "AM" );
}

void CHostage::Touch( CBaseEntity* other )
{
	if ( m_rescued )
		return;
	if ( other->IsPlayer() == TRUE )
	{
		if ( ( (CBasePlayer*)other )->m_iTeam != CT )
			return;
	}
	else if ( !FClassnameIs( other->pev, "monster_hostage" ) )
		return;
	Vector push( pev->origin.x - other->pev->origin.x, pev->origin.y - other->pev->origin.y, pev->origin.z - pev->origin.z );
	float length = push.Length();
	pev->velocity.x += push.x / length * 50;
	pev->velocity.y += push.y / length * 50;
	pev->velocity.z += push.z / length * 50;
	UTIL_DrawBeamPoints( pev->origin, pev->origin + pev->velocity.Normalize() * 500, 10, 0, 255, 0 );
}

void CHostage::Wiggle()
{
	Vector offsets[] =
	{
		Vector( 50, 0, 0 ), Vector( -50, 0, 0 ),
		Vector( 0, 50, 0 ), Vector( 0, -50, 0 ),
		Vector( 50, 50, 0 ), Vector( 50, -50, 0 ),
		Vector( -50, 50, 0 ), Vector( -50, -50, 0 ),
	};
	Vector push = g_vecZero;
	for ( int i = 0; i < ARRAYSIZE( offsets ); ++i )
	{
		Vector destination = pev->origin + offsets[i];
		if ( !m_LocalNav->PathTraversable( pev->origin, destination, 1 ) )
			push = push - offsets[i];
	}
	push.y += RANDOM_FLOAT( -3, 3 );
	push.x += RANDOM_FLOAT( -3, 3 );
	pev->velocity = pev->velocity + push.Normalize() * 100;
	UTIL_DrawBeamPoints( pev->origin, pev->origin + pev->velocity.Normalize() * 500, 10, 0, 0, 255 );
}

int CHostage::TakeDamage( entvars_t* inflictor, entvars_t* attacker, float damage, int damageType )
{
	damage = GetModifiedDamage( damage, m_LastHitGroup );
	if ( damage > pev->health )
		damage = pev->health;
	pev->health -= damage;
	PlayPainSound();
	CBasePlayer* player = NULL;
	if ( attacker && GetClassPtr( (CBaseEntity*)attacker )->IsPlayer() )
		player = GetClassPtr( (CBasePlayer*)attacker );
	if ( pev->health <= 0 )
	{
		pev->health = 0;
		pev->movetype = MOVETYPE_TOSS;
		pev->flags &= ~FL_ONGROUND;
		SetDeathActivity();
		SayNamedSentence( "H_DEATH", 1, ATTN_NORM, 0, (int)m_voicePitch );
		pev->takedamage = DAMAGE_NO;
		pev->deadflag = DEAD_DEAD;
		pev->solid = SOLID_NOT;
		pev->nextthink = gpGlobals->time + 3;
		FCheckAITrigger();
		SetThink( &CHostage::Remove );
		return 0;
	}
	SetFlinchActivity();
	if ( player )
	{
		if ( !( player->m_flDisplayHistory & HOSTAGE_INJURED_HINT ) )
			player->m_flDisplayHistory |= HOSTAGE_INJURED_HINT;
		return 1;
	}
	return 0;
}

void CHostage::FollowUse( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value )
{
	if ( m_rescued || !activator || !activator->IsPlayer() || !pev->takedamage )
		return;
	CBasePlayer* player = (CBasePlayer*)activator;
	if ( FStrEq( STRING( player->GetActiveWeaponClassname() ), "weapon_syringe" ) && m_healthModifier != 0 )
	{
		m_healthModifier = 0;
		return;
	}
	if ( player->m_iTeam != CT )
	{
		player->m_flDisplayHistory |= HOSTAGE_CT_FOLLOW_HINT;
		return;
	}
	if ( m_flNextChange > gpGlobals->time )
		return;
	m_flNextChange = gpGlobals->time + 1;
	m_flPathAcquired = gpGlobals->time;
	if ( activator == m_hTargetEnt )
	{
		if ( m_State != FOLLOW )
		{
			m_State = FOLLOW;
			SayNamedSentence( "H_HELP", 0.75f, ATTN_NORM, 0, (int)m_voicePitch );
			return;
		}
		m_State = STAND;
		m_hTargetEnt = NULL;
		m_hStoppedTargetEnt = activator;
		SayNamedSentence( "NPC_WAIT", 0.75f, ATTN_NORM, 0, (int)m_voicePitch );
	}
	else
	{
		m_State = FOLLOW;
		m_idleAnim = ALLOC_STRING( "default" );
		m_hTargetEnt = activator;
		m_hStoppedTargetEnt = NULL;
	}
	if ( m_State == FOLLOW )
		SayNamedSentence( "H_HELP", 0.75f, ATTN_NORM, 0, (int)m_voicePitch );
}

void CHostage::IdleThink()
{
	CBaseMonster::MonsterThink();
	if ( m_rescued )
		return;
	if ( pev->deadflag == DEAD_DEAD )
	{
		UTIL_SetSize( pev, g_vecZero, g_vecZero );
		return;
	}
	if ( m_hTargetEnt && m_hTargetEnt->pev->deadflag != DEAD_NO )
	{
		m_State = STAND;
		m_hTargetEnt = NULL;
	}
	if ( m_hTargetEnt && GetClassPtr( (CBasePlayer*)m_hTargetEnt->pev )->m_iTeam == CT )
	{
		CHalfLifeRules* rules = (CHalfLifeRules*)g_pGameRules;
		CHostageRescue* spot = NULL;
		if ( !rules->m_bMapHasRescueZone )
		{
			while ( ( spot = (CHostageRescue*)UTIL_FindEntityByClassname( spot, "info_hostage_rescue" ) ) != NULL )
			{
				if ( ( spot->pev->origin - pev->origin ).Length() < 256 )
				{
					m_bRescueMe = TRUE;
					break;
				}
			}
		}
		if ( m_bRescueMe && !m_rescued )
		{
			if ( m_rescueTarget )
				FireTargets( STRING( m_rescueTarget ), this, this, USE_TOGGLE, 0 );
			if ( !spot || !spot->quiet )
				SayNamedSentence( "H_THANK", 1, ATTN_NORM, 0, (int)m_voicePitch );
			if ( m_noAutoRemove )
			{
				m_bRescueMe = FALSE;
				m_hTargetEnt = NULL;
			}
			else if ( m_rescueAnim )
			{
				pev->sequence = LookupSequence( STRING( m_rescueAnim ) );
				m_bRescueMe = FALSE;
				m_hTargetEnt = NULL;
			}
			else
			{
				pev->effects |= EF_NODRAW;
				Remove();
			}
			++rules->m_iHostagesRescued;
			m_rescued = TRUE;
			m_idleAnim = 0;
			SetActivity( ACT_IDLE );
			m_IdealMonsterState = MONSTERSTATE_IDLE;
			return;
		}
	}
	DoFollow();
	if ( gpGlobals->time >= m_flFlinchTime )
	{
		if ( !( pev->velocity.Length() <= 160 ) )
			SetActivity( ACT_RUN );
		else if ( pev->velocity.Length() < 1 )
			SetActivity( ACT_IDLE );
		else
			SetActivity( ACT_WALK );
	}
}

void CHostage::MoveToward( Vector& point )
{
	CBaseEntity* target = GetClassPtr( (CBaseEntity*)m_hTargetEnt->pev );
	Vector angles( 0, UTIL_VecToAngles( point - pev->origin ).y, 0 );
	Vector forward;
	g_engfuncs.pfnAngleVectors( angles, forward, NULL, NULL );
	float step = ( forward * CLocalNav::s_flStepSize ).Length2D();
	float distance = ( point - pev->origin ).Length2D();
	if ( distance > step )
		distance = step;
	Vector destination = pev->origin + forward * distance;
	int movement = m_LocalNav->PathTraversable( pev->origin, destination, 0 );
	if ( !movement )
		return;
	float speed = 250;
	if ( pev->flags & FL_ONGROUND )
	{
		Vector targetPoint = target->pev->origin;
		targetPoint.z += target->pev->mins.z;
		speed = ( targetPoint - pev->origin ).Length();
		if ( speed >= 110 )
			speed = speed >= 250 ? 400.0f : 300.0f;
	}
	pev->velocity.x = forward.x * speed;
	pev->velocity.y = forward.y * speed;
	UTIL_DrawBeamPoints( pev->origin, pev->origin + pev->velocity.Normalize() * 500, 10, 255, 0, 0 );
	if ( movement == 2 )
		pev->velocity.z = 180;
	else if ( movement == 3 && ( pev->flags & FL_ONGROUND ) )
		pev->velocity.z = 270;
}

void CHostage::DoFollow()
{
	if ( !m_hTargetEnt )
		return;
	CBaseEntity* target = GetClassPtr( (CBaseEntity*)m_hTargetEnt->pev );
	m_LocalNav->SetTargetEnt( target );
	Vector destination = target->pev->origin;
	destination.z += target->pev->mins.z;
	if ( !( ( destination - pev->origin ).Length() >= 80 ||
		( !m_fHasPath && !m_LocalNav->PathTraversable( pev->origin, destination, 1 ) ) ) )
		return;
	if ( ( pev->flags & FL_ONGROUND ) &&
		gpGlobals->time > m_flLastPathCheck + m_flPathCheckInterval &&
		( !m_fHasPath || !( target->pev->velocity.Length2D() <= 1 ) ) )
	{
		m_flLastPathCheck = gpGlobals->time;
		m_LocalNav->RequestNav( this );
	}
	if ( m_fHasPath )
	{
		nTargetNode = m_LocalNav->GetFurthestTraversableNode( pev->origin, vecNodes, m_nPathNodes, 1 );
		if ( nTargetNode == 0 && ( vecNodes[0] - pev->origin ).Length2D() < 26 )
			--nTargetNode;
		if ( nTargetNode == -1 )
		{
			m_fHasPath = FALSE;
			m_flPathCheckInterval = 0.1f;
			if ( m_flFlinchTime > gpGlobals->time )
				return;
			goto checkMovement;
		}
	}
	if ( m_flFlinchTime > gpGlobals->time )
		return;
	if ( nTargetNode != -1 )
	{
		if ( pev->flags & FL_ONGROUND )
			PointAt( vecNodes[nTargetNode] );
		if ( pev->movetype == MOVETYPE_FLY )
			pev->v_angle.x = -60;
		MoveToward( vecNodes[nTargetNode] );
	}

checkMovement:
	if ( ( pev->flags & FL_ONGROUND ) && m_flPathAcquired != -1 &&
		m_flPathAcquired + 2 > gpGlobals->time )
	{
		float speedSquared = pev->velocity.x * pev->velocity.x + pev->velocity.y * pev->velocity.y;
		if ( speedSquared < 0 )
			(void)sqrt( speedSquared );
	}
}

void CHostage::NavReady()
{
	if ( !m_hTargetEnt )
		return;
	CBaseEntity* target = GetClassPtr( (CBaseEntity*)m_hTargetEnt->pev );
	Vector destination = target->pev->origin;
	if ( !( target->pev->flags & FL_ONGROUND ) )
	{
		Vector drop = destination;
		drop.z -= 300;
		TraceResult trace;
		UTIL_TraceHull( destination, drop, ignore_monsters, human_hull, target->edict(), &trace );
		if ( trace.fStartSolid || trace.flFraction == 1 )
			return;
		destination = trace.vecEndPos;
	}
	destination.z += target->pev->mins.z;
	m_LocalNav->SetTargetEnt( target );
	node_index_t path = m_LocalNav->FindPath( pev->origin, destination, 40, 1 );
	if ( path == -1 )
	{
		if ( !m_fHasPath )
		{
			float interval = 0.5f;
			if ( m_flPathCheckInterval + 0.1 <= 0.5 )
				interval = (float)( m_flPathCheckInterval + 0.1 );
			m_flPathCheckInterval = interval;
		}
	}
	else
	{
		m_fHasPath = TRUE;
		nTargetNode = -1;
		m_flPathCheckInterval = 0.5f;
		m_flPathAcquired = gpGlobals->time;
		m_nPathNodes = m_LocalNav->SetupPathNodes( path, vecNodes, 1 );
	}
}

void CHostage::PreThink()
{
	if ( !( pev->flags & FL_ONGROUND ) || !( pev->velocity.Length2D() >= 1 ) )
		return;
	float frameTime = gpGlobals->frametime;
	Vector source = pev->origin;
	Vector destination = source;
	destination.x += pev->velocity.x * frameTime;
	destination.y += pev->velocity.y * frameTime;
	TraceResult trace;
	g_engfuncs.pfnTraceMonsterHull( edict(), source, destination, 0, edict(), &trace );
	if ( trace.fStartSolid || trace.flFraction == 1 || !( trace.vecPlaneNormal.z <= 0.7 ) )
		return;
	float distance = ( trace.vecEndPos - pev->origin ).Length2D();
	source.z += CLocalNav::s_flStepSize;
	Vector forward = pev->velocity.Normalize();
	destination.x = source.x + forward.x * 0.1f;
	destination.y = source.y + forward.y * 0.1f;
	destination.z = source.z;
	g_engfuncs.pfnTraceMonsterHull( edict(), source, destination, 0, edict(), &trace );
	if ( trace.fStartSolid )
		return;
	source = trace.vecEndPos;
	destination = source;
	destination.z -= CLocalNav::s_flStepSize;
	g_engfuncs.pfnTraceMonsterHull( edict(), source, destination, 0, edict(), &trace );
	if ( trace.vecPlaneNormal.z >= 0.7 && ( trace.vecEndPos - pev->origin ).Length2D() > distance )
	{
		Vector origin = pev->origin;
		origin.z = trace.vecEndPos.z;
		UTIL_SetOrigin( pev, origin );
		pev->velocity.z += g_psv_gravity->value * pev->gravity * frameTime;
	}
}
