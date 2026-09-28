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
// headcrab.cpp - tiny, jumpy alien parasite
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"game.h"
#include    "weapons.h"
#include	"player.h"

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		HR_AE_JUMPATTACK	( 2 )

Task_t	tlHRRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slHRRangeAttack1[] =
{
	{ 
		tlHRRangeAttack1,
		ARRAYSIZE ( tlHRRangeAttack1 ), 
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED,
		0,
		"HRRangeAttack1"
	},
};

Task_t	tlHRRangeAttack1Fast[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slHRRangeAttack1Fast[] =
{
	{ 
		tlHRRangeAttack1Fast,
		ARRAYSIZE ( tlHRRangeAttack1Fast ), 
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED,
		0,
		"HRRAFast"
	},
};

class Chrhr : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void RunTask ( Task_t *pTask );
	void StartTask ( Task_t *pTask );
	void SetYawSpeed ( void );
	void EXPORT LeapTouch ( CBaseEntity *pOther );
	Vector BodyTarget( const Vector &posSrc );
	void PainSound( void );
	void DeathSound( void );
	void IdleSound( void );
	void AlertSound( void );
	void PrescheduleThink( void );
	int  Classify ( void );
	void RunAI( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	void Killed( entvars_t *pevAttacker, int iGib );

	//virtual float GetDamageAmount( void ) { return gSkillData.headcrabDmgBite; }
	virtual int GetVoicePitch( void ) { return 100; }
	virtual float GetSoundVolue( void ) { return 1.0; }
	Schedule_t* GetScheduleOfType ( int Type );

	CUSTOM_SCHEDULES;

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackSounds[];
	static const char *pDeathSounds[];
	static const char *pBiteSounds[];
};
LINK_ENTITY_TO_CLASS( monster_hrhr, Chrhr );
LINK_ENTITY_TO_CLASS( monster_hrhr_mini, Chrhr );

DEFINE_CUSTOM_SCHEDULES( Chrhr )
{
	slHRRangeAttack1,
	slHRRangeAttack1Fast,
};

IMPLEMENT_CUSTOM_SCHEDULES( Chrhr, CBaseMonster );

const char *Chrhr::pIdleSounds[] = 
{
	"hrhr/hc_idle1.wav",
	"hrhr/hc_idle2.wav",
	"hrhr/hc_idle3.wav",
};
const char *Chrhr::pAlertSounds[] = 
{
	"hrhr/hc_alert1.wav",
};
const char *Chrhr::pPainSounds[] = 
{
	"hrhr/hc_pain1.wav",
	"hrhr/hc_pain2.wav",
	"hrhr/hc_pain3.wav",
};
const char *Chrhr::pAttackSounds[] = 
{
	"hrhr/hc_attack1.wav",
	"hrhr/hc_attack2.wav",
	"hrhr/hc_attack3.wav",
};

const char *Chrhr::pDeathSounds[] = 
{
	"hrhr/hc_die1.wav",
	"hrhr/hc_die2.wav",
};

const char *Chrhr::pBiteSounds[] = 
{
	"hrhr/hc_headbite.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	Chrhr :: Classify ( void )
{
	return	CLASS_ALIEN_MONSTER;
}

Vector Chrhr :: BodyTarget( const Vector &posSrc ) 
{ 
	return Center( );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void Chrhr :: SetYawSpeed ( void )
{
	if(FClassnameIs( pev, "monster_hrhr_mini") ){
	pev->yaw_speed = 90;
	}
	else{
	pev->yaw_speed = 120;
	}
}

void Chrhr :: RunAI( void )
{
	
	if(pev->sequence == LookupActivity ( ACT_WALK ) || pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
		if(FClassnameIs( pev, "monster_hrhr_mini") ){
		m_flGroundSpeed = 240;
		}
		else{
		m_flGroundSpeed = 270;
		}
	}

	if(pev->movetype == MOVETYPE_FLY){
		if ( m_MonsterState == MONSTERSTATE_PRONE || m_IdealMonsterState == MONSTERSTATE_PRONE ){
		pev->health = 0;
		pev->movetype = MOVETYPE_STEP;
		pev->owner = NULL;
		}
	}

	if ( pev->movetype == MOVETYPE_TOSS)
	{
		if (pev->flags & FL_ONGROUND)
		{
			pev->movetype = MOVETYPE_STEP;
		}
	}

	CBaseMonster :: RunAI();
}


void Chrhr::Killed( entvars_t *pevAttacker, int iGib )
{
	if(pev->movetype == MOVETYPE_FLY){
	pev->movetype = MOVETYPE_STEP;
	}
	CBaseMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void Chrhr :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg;
	if (g_iSkillLevel == SKILL_HARD){
	dmg	= 25;
	}
	else if (g_iSkillLevel == SKILL_EASY){
	dmg	= 15;
	}
	else{
	dmg = 20;
	}

	switch( pEvent->event )
	{
		case 1:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_c(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 80;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH | DMG_NEVERGIB ); 
			ApplyMultiDamage( pev, pev );
			EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "hrhr/hc_headbite.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 75, dmg, DMG_SLASH | DMG_NEVERGIB );
				if(pHurt){
					if (pHurt->Classify() == CLASS_PLAYER || pHurt->Classify() == CLASS_PLAYER_ALLY
					|| pHurt->Classify() == CLASS_HUMAN_ASS || pHurt->Classify() == CLASS_HUMAN_PASSIVE
					|| pHurt->Classify() == CLASS_HUMAN_MILITARY){
					FX_Explosion( pHurt->Center(), 234 );
					}
					else if (pHurt->Classify() == CLASS_ALIEN_MONSTER || pHurt->Classify() == CLASS_ALIEN_MILITARY){
					FX_Explosion( pHurt->Center(), 235 );
					}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "hrhr/hc_headbite.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}
			
		}
		break;

		case HR_AE_JUMPATTACK:
		{
			ClearBits( pev->flags, FL_ONGROUND );

			UTIL_SetOrigin (pev, pev->origin + Vector ( 0 , 0 , 1) );// take him off ground so engine doesn't instantly reset onground 
			UTIL_MakeVectors ( pev->angles );

			Vector vecJumpDir;
			if (m_hEnemy != NULL)
			{
				float gravity = g_psv_gravity->value;
				if (gravity <= 1)
					gravity = 1;

				// How fast does the headcrab need to travel to reach that height given gravity?
				float height = (m_hEnemy->pev->origin.z + m_hEnemy->pev->view_ofs.z - pev->origin.z);

				if ( m_hEnemy->pev->flags & FL_CLIENT ){
				height += 2;
				}
				float flDist = ( pev->origin - m_hEnemy->pev->origin).Length();
				if(flDist >= 120)
				height += 1;
				else if(flDist >= 80)
				height += 4;
				else if(flDist >= 60)
				height += 8;
				else
				height += 16;

			//	char text[256];
			//	sprintf( text, "- DIST %f HEIGHT %f\n",flDist,height );
			//	UTIL_SayTextAll( text,this );

				if (height < 10){
					height = 10;
				}
				float speed = sqrt( 2 * gravity * height );
				float time = speed / gravity;

				// Scale the sideways velocity to get there at the right time
				vecJumpDir = (m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - pev->origin);
				vecJumpDir = vecJumpDir * ( 1.0 / time );

				// Speed to offset gravity at the desired height
				vecJumpDir.z = speed;

				// Don't jump too far/fast
				float distance = vecJumpDir.Length();
				
				if (distance > 650)
				{
					vecJumpDir = vecJumpDir * ( 650.0 / distance );
				}
			}
			else
			{
				// jump hop, don't care where
				vecJumpDir = Vector( gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_up.z ) * 350;
			}

			int iSound = RANDOM_LONG(0,2);
			if ( iSound != 0 )
				EMIT_SOUND_DYN( edict(), CHAN_VOICE, pAttackSounds[iSound], GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );

			pev->velocity = vecJumpDir;
			m_flNextAttack = gpGlobals->time + 2;
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void Chrhr :: Spawn()
{
	Precache( );

	if(FClassnameIs( pev, "monster_hrhr_mini") ){
			SET_MODEL(ENT(pev), "models/hrhr_mini.mdl");
			UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 36));

			if (g_iSkillLevel == SKILL_HARD){
			pev->health			= 50;
			}
			else if (g_iSkillLevel == SKILL_EASY){
			pev->health			= 30;
			}
			else{
			pev->health			= 40;
			}

			m_killed_exp = 4;
	}
	else{
		SET_MODEL(ENT(pev), "models/hrhr.mdl");
		UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 72));

		if (g_iSkillLevel == SKILL_HARD){
		pev->health			= 110;
		}
		else if (g_iSkillLevel == SKILL_EASY){
		pev->health			= 70;
		}
		else{
		pev->health			= 90;
		}

		m_killed_exp = 6;
	}

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_YELLOW;
	pev->effects		= 0;

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	pev->yaw_speed		= 5;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();

	m_headdef			= 2;
	m_ignoredamage		= 1;

	//m_forcefuckdoor  = TRUE;
	m_selfmode = TRUE;
	m_MoveFail_SimpleRoad = TRUE;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void Chrhr :: Precache()
{
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pBiteSounds);

	PRECACHE_MODEL("models/hrhr.mdl");
	PRECACHE_MODEL("models/hrhr_mini.mdl");
}	


//=========================================================
// RunTask 
//=========================================================
void Chrhr :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
	case TASK_RANGE_ATTACK2:
		{
			if ( m_fSequenceFinished )
			{
				TaskComplete();
				SetTouch( NULL );
				m_IdealActivity = ACT_IDLE;
			}
			break;
		}
	default:
		{
			CBaseMonster :: RunTask(pTask);
		}
	}
}

//=========================================================
// LeapTouch - this is the headcrab's touch function when it
// is in the air
//=========================================================
void Chrhr :: LeapTouch ( CBaseEntity *pOther )
{
	int dmg;
	if (g_iSkillLevel == SKILL_HARD){
	dmg			= 12;
	}
	else if (g_iSkillLevel == SKILL_EASY){
	dmg			= 8;
	}
	else{
	dmg			= 10;
	}

	if ( pev->movetype == MOVETYPE_TOSS){
	pev->velocity = g_vecZero;
	}

	if ( !pOther->pev->takedamage || pev->deadflag != DEAD_NO){
		return;
	}

	if ( pOther->Classify() == Classify() ){
		return;
	}

	TraceResult tr = UTIL_GetGlobalTrace( );
	TraceResult tr2;

	// Don't hit if back on ground
	if ( !FBitSet( pev->flags, FL_ONGROUND ) )
	{
		ClearMultiDamage( );

	//	UTIL_MakeVectors ( pev->angles );

		Vector vecDir = pOther->pev->origin - pev->origin;
		vecDir = vecDir.Normalize( );
		Vector monsterangles = UTIL_VecToAngles( vecDir );
		monsterangles.x = 0;
		monsterangles.z = 0;
		UTIL_MakeVectors(monsterangles);

		Vector vecSrc = Center();
		Vector vecEnd	= vecSrc + gpGlobals->v_forward * 48;
		UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
		if (tr.iHitgroup != 1 && tr.iHitgroup != 10 && tr.iHitgroup != 11){
		vecSrc = pev->origin;
		vecEnd	= vecSrc + gpGlobals->v_forward * 48;
		UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
		}

		int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
		int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),0);
		FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

		if ( pOther->pev->flags & (FL_MONSTER|FL_CLIENT) ){
			Vector hit_origin = pOther->BodyTarget_l(pOther->pev->origin);
			if(pev->origin.z > hit_origin.z ){//�������˱Ƚϸߵ�λ�ã�������ͷ��������+�˺�������

				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pOther->MyMonsterPointer();

				if ( (pOther->pev->flags & FL_CLIENT) || pEnemyMonster->m_headdef == 2 ){//ͷ����Ӳ
				dmg *= 2.0;//3.0
				}
				else if ( pEnemyMonster->m_headdef == 1 ){//ͷ������
				dmg *= 3.0;//7.5 ��ʵ��ͷ�˺�������
				}
				else{//Ĭ��
				dmg *= 2.5;//5.0
				}

				pOther->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH | DMG_NEVERGIB); 

				EMIT_SOUND_DYN( edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pBiteSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
				ApplyMultiDamage( pev, pev );

			}
			else{
					pOther->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH | DMG_NEVERGIB); 
					EMIT_SOUND_DYN( edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pBiteSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
					ApplyMultiDamage( pev, pev );	
			}
		}
		else{
			pOther->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH | DMG_NEVERGIB ); 
			ApplyMultiDamage( pev, pev );
		}
	}
	SetTouch( NULL );
}

//=========================================================
// PrescheduleThink
//=========================================================
void Chrhr :: PrescheduleThink ( void )
{
	// make the crab coo a little bit in combat state
	if ( m_MonsterState == MONSTERSTATE_COMBAT && RANDOM_FLOAT( 0, 5 ) < 0.1 )
	{
		IdleSound();
	}
}

void Chrhr :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		{
			EMIT_SOUND_DYN( edict(), CHAN_WEAPON, pAttackSounds[0], GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
			m_IdealActivity = ACT_RANGE_ATTACK1;
			SetTouch ( &Chrhr::LeapTouch );
			break;
		}
	default:
		{
			CBaseMonster :: StartTask( pTask );
		}
	}
}

//=========================================================
// CheckRangeAttack2
//=========================================================
BOOL Chrhr :: CheckRangeAttack2 ( float flDot, float flDist )
{
	return FALSE;
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL Chrhr :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( FClassnameIs( pev, "monster_hrhr_mini") && pev->sequence != LookupActivity ( ACT_EAT ) 
	&& FBitSet( pev->flags, FL_ONGROUND ) && flDist <= 256 && flDot >= 0.65 )
	{
		return TRUE;
	}
	return FALSE;
}

BOOL Chrhr :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( FClassnameIs( pev, "monster_hrhr") )
	{
			float dist = 80;
			if(m_hEnemy != NULL){
				if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 60 )
				{
					if (m_hEnemy->pev->flags & FL_ONGROUND)
					{
					dist += 40;
					}
				}
			}

			if (flDist <= dist && m_hEnemy != NULL && flDot >= 0.5)
			{
				if (m_hEnemy->IsAlive() ){
					if(pev->sequence == LookupActivity ( ACT_WALK )){
					pev->sequence = LookupActivity ( ACT_WALK_SCARED );
					ResetSequenceInfo( );
					pev->frame = 0;
					}
				}
			}
			else if (flDist >= dist + 48 )
			{
					if(pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
					pev->sequence = LookupActivity ( ACT_WALK );
					ResetSequenceInfo( );
					pev->frame = 0;
					}
			}
	}

	return FALSE;
}

int Chrhr :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Don't take any acid damage -- BigMomma's mortar is acid
	if ( bitsDamageType & DMG_ACID ){
	return 0;
	}
	if(pev->sequence == 20){
		flDamage *= 2.0;
	}
	else{
		if ( !FBitSet( pev->flags, FL_ONGROUND ) ){
			UTIL_MakeVectors( pev->angles );
			if ( bitsDamageType & DMG_CLUB ){
				if(pev->velocity.z > 0){
				flDamage *= 2.0;
				pev->velocity = -pev->velocity;
				}
				else{
				flDamage *= 1.5;
				pev->velocity.x = 0;
				pev->velocity.y = 0;
				}
			}
			else{
				flDamage *= 1.5;
				float Dam = flDamage;
				if(Dam > 60)
				Dam = 60;
				pev->velocity = pev->velocity + gpGlobals->v_forward * -(Dam * 20);
			}
			if(flDamage >= pev->max_health * 0.35){
			SetTouch( NULL );
			}
		}
	}
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// IdleSound
//=========================================================
#define CRAB_ATTN_IDLE (float)1.5
void Chrhr :: IdleSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// AlertSound 
//=========================================================
void Chrhr :: AlertSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// AlertSound 
//=========================================================
void Chrhr :: PainSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// DeathSound 
//=========================================================
void Chrhr :: DeathSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

Schedule_t* Chrhr :: GetScheduleOfType ( int Type )
{
	switch	( Type )
	{
		case SCHED_RANGE_ATTACK1:
		{
			return &slHRRangeAttack1[ 0 ];
		}
		break;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}
