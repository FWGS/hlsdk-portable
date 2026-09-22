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
#define		HC_AE_JUMPATTACK	( 2 )

Task_t	tlHCRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slHCRangeAttack1[] =
{
	{ 
		tlHCRangeAttack1,
		ARRAYSIZE ( tlHCRangeAttack1 ), 
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED,
		0,
		"HCRangeAttack1"
	},
};

Task_t	tlHCRangeAttack1Fast[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slHCRangeAttack1Fast[] =
{
	{ 
		tlHCRangeAttack1Fast,
		ARRAYSIZE ( tlHCRangeAttack1Fast ), 
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED,
		0,
		"HCRAFast"
	},
};

class CHeadCrab : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void RunTask ( Task_t *pTask );
	void StartTask ( Task_t *pTask );
	void SetYawSpeed ( void );
	void EXPORT LeapTouch ( CBaseEntity *pOther );
	Vector Center( void );
	Vector BodyTarget( const Vector &posSrc );
	void PainSound( void );
	void DeathSound( void );
	void IdleSound( void );
	void AlertSound( void );
	void PrescheduleThink( void );
	int  Classify ( void );
	void RunAI( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

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
LINK_ENTITY_TO_CLASS( monster_headcrab, CHeadCrab );
LINK_ENTITY_TO_CLASS( monster_headcrab_throw, CHeadCrab );

DEFINE_CUSTOM_SCHEDULES( CHeadCrab )
{
	slHCRangeAttack1,
	slHCRangeAttack1Fast,
};

IMPLEMENT_CUSTOM_SCHEDULES( CHeadCrab, CBaseMonster );

const char *CHeadCrab::pIdleSounds[] = 
{
	"headcrab/hc_idle1.wav",
	"headcrab/hc_idle2.wav",
	"headcrab/hc_idle3.wav",
};
const char *CHeadCrab::pAlertSounds[] = 
{
	"headcrab/hc_alert1.wav",
};
const char *CHeadCrab::pPainSounds[] = 
{
	"headcrab/hc_pain1.wav",
	"headcrab/hc_pain2.wav",
	"headcrab/hc_pain3.wav",
};
const char *CHeadCrab::pAttackSounds[] = 
{
	"headcrab/hc_attack1.wav",
	"headcrab/hc_attack2.wav",
	"headcrab/hc_attack3.wav",
};

const char *CHeadCrab::pDeathSounds[] = 
{
	"headcrab/hc_die1.wav",
	"headcrab/hc_die2.wav",
};

const char *CHeadCrab::pBiteSounds[] = 
{
	"headcrab/hc_headbite.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CHeadCrab :: Classify ( void )
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// Center - returns the real center of the headcrab.  The 
// bounding box is much larger than the actual creature so 
// this is needed for targeting
//=========================================================
Vector CHeadCrab :: Center ( void )
{
	return Vector( pev->origin.x, pev->origin.y, pev->origin.z + 6 );
}


Vector CHeadCrab :: BodyTarget( const Vector &posSrc ) 
{ 
	return Center( );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHeadCrab :: SetYawSpeed ( void )
{
	pev->yaw_speed = 60;
}

void CHeadCrab :: RunAI( void )
{
	if(pev->deadflag == DEAD_NO){
			if(pev->sequence == LookupActivity ( ACT_RUN )){
			m_flGroundSpeed = 60;
			}

			if(pev->movetype == MOVETYPE_FLY){
				if ( m_MonsterState == MONSTERSTATE_PRONE || m_IdealMonsterState == MONSTERSTATE_PRONE ){
				pev->health = 0;
				pev->movetype = MOVETYPE_STEP;
				pev->owner = NULL;
				}
			}
			else{
				//Bug Fix 3.0 ͷз����ͻ���ж�����ֹ����ͷ�������
				if( (pev->flags & FL_ONGROUND) && pev->sequence == LookupActivity ( ACT_IDLE )){
					if(m_hEnemy != NULL){
						if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) <= 80 )
						{
							if(( pev->origin - m_hEnemy->pev->origin).Length2D() <= 10){
								if(pev->velocity.Length() <= 100){
								pev->velocity.x += RANDOM_LONG(-300,300);
								pev->velocity.y += RANDOM_LONG(-300,300);
								}
							}
						}
					}
				}
			}

			if ( pev->movetype == MOVETYPE_TOSS)
			{
				if (pev->flags & FL_ONGROUND)
				{
					pev->movetype = MOVETYPE_STEP;
				}
			}

			if ( FClassnameIs(pev, "monster_headcrab_throw" ) ){
				if ( !FNullEnt(pev->owner)){
					if(!(VARS(pev->owner)->flags & FL_MONSTER)){
					pev->owner = NULL;
					}
					else if(!VARS(pev->owner)->deadflag == DEAD_NO || VARS(pev->owner)->health <= 0){
					pev->owner = NULL;
					}
				}
			}
	}

	CBaseMonster :: RunAI();
}


void CHeadCrab::Killed( entvars_t *pevAttacker, int iGib )
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
void CHeadCrab :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case HC_AE_JUMPATTACK:
		{
			ClearBits( pev->flags, FL_ONGROUND );

			//UTIL_SetOrigin (pev, pev->origin + Vector ( 0 , 0 , 1) );// take him off ground so engine doesn't instantly reset onground 
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
void CHeadCrab :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/headcrab.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 24;
	}
	else{
	pev->health			= 18;
	}
	
	pev->view_ofs		= Vector ( 0, 0, 20 );// position of the eyes relative to monster's origin.
	pev->yaw_speed		= 5;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();

	if ( FClassnameIs(pev, "monster_headcrab_throw" ) ){
	SetTouch ( &CHeadCrab::LeapTouch );
	}
	m_killed_exp = 20;
	m_rpgms_level = 20;
	pev->netname = MAKE_STRING( "Headcrab" );
	m_canbarnacle_mode = 1;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHeadCrab :: Precache()
{
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pBiteSounds);

	PRECACHE_MODEL("models/headcrab.mdl");
}	


//=========================================================
// RunTask 
//=========================================================
void CHeadCrab :: RunTask ( Task_t *pTask )
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

void CHeadCrab :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	//Bug Fix 3.0 ͷзͶ�֤�ģʽ
	if (pev->impulse == 278 && pev->deadflag == DEAD_NO)
	{
		FX_Explosion( ptr->vecEndPos, 49);
	}
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

//=========================================================
// LeapTouch - this is the headcrab's touch function when it
// is in the air
//=========================================================
void CHeadCrab :: LeapTouch ( CBaseEntity *pOther )
{
	int dmg;
	dmg			= 10;

	if(pev->impulse == 278 && pev->deadflag == DEAD_NO){//Bug Fix 3.0Ͷ����ͷз����
		if ( pOther->pev->takedamage ){
		EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/fist_hearvy_hit1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
		pOther->TakeDamage( pev, pev, 60, DMG_SLASH | DMG_CONCUSSION);
		}
		CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
		if ( pEntity ){
			CBasePlayer *player = GetClassPtr((CBasePlayer *)pEntity->pev);
			player->m_game_rate += 1;//Miss!
		}
		FX_Explosion(Center(), 47 );
		UTIL_Remove( this );
		SetTouch( NULL );
		return;
	}

	if(pev->velocity.Length() >= 1200){//���ٳ��
	dmg *= 2;
	pev->owner = NULL;
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

	TraceResult tr;

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

				if(pEnemyMonster->pev->health <= 0 && pEnemyMonster->m_killbyheadcrab == 0){
					if(pEnemyMonster->m_canheadcrab_mode == 1){
					pEnemyMonster->m_killbyheadcrab = 1;
					UTIL_Remove( this );
					return;
					}
				}

				if ( pOther->IsPlayer() ){
					CBasePlayer *player = GetClassPtr((CBasePlayer *)pOther->pev);
					if(player->m_needleheal == 0 && player->m_barnacle_RTP == 0 && player->m_barnacle_god_time <= gpGlobals->time
					&& player->pev->movetype == MOVETYPE_WALK){
					pev->velocity = g_vecZero;
					player->m_barnacle_RTP = 1;
					player->m_barnacle_Level = 1;
					player->m_barnacle_catchme = this;
					player->pev->punchangle.x += RANDOM_FLOAT(-25, 25);
					player->pev->punchangle.y += RANDOM_FLOAT(-25, 25);
					player->pev->punchangle.z += RANDOM_FLOAT(-25, 25);
					}
				}
			}
			else{
					pOther->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH | DMG_NEVERGIB); 
					EMIT_SOUND_DYN( edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pBiteSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
					ApplyMultiDamage( pev, pev );	
			
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pOther->MyMonsterPointer();
					if(pEnemyMonster->pev->health <= 0 && pEnemyMonster->m_killbyheadcrab == 0){
						if(pEnemyMonster->m_canheadcrab_mode == 1){
						pEnemyMonster->m_killbyheadcrab = 1;
						UTIL_Remove( this );
						return;
						}
					}
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
void CHeadCrab :: PrescheduleThink ( void )
{
	// make the crab coo a little bit in combat state
	if ( m_MonsterState == MONSTERSTATE_COMBAT && RANDOM_FLOAT( 0, 5 ) < 0.1 )
	{
		IdleSound();
	}
}

void CHeadCrab :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		{
			EMIT_SOUND_DYN( edict(), CHAN_WEAPON, pAttackSounds[0], GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
			m_IdealActivity = ACT_RANGE_ATTACK1;
			SetTouch ( &CHeadCrab::LeapTouch );
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
BOOL CHeadCrab :: CheckRangeAttack2 ( float flDot, float flDist )
{
	return FALSE;
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CHeadCrab :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( pev->sequence != 20 && FBitSet( pev->flags, FL_ONGROUND ) && flDist <= 256 && flDot >= 0.65 )
	{
		return TRUE;
	}
	return FALSE;
}

int CHeadCrab :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
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
void CHeadCrab :: IdleSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// AlertSound 
//=========================================================
void CHeadCrab :: AlertSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// AlertSound 
//=========================================================
void CHeadCrab :: PainSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// DeathSound 
//=========================================================
void CHeadCrab :: DeathSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

Schedule_t* CHeadCrab :: GetScheduleOfType ( int Type )
{
	switch	( Type )
	{
		case SCHED_RANGE_ATTACK1:
		{
			return &slHCRangeAttack1[ 0 ];
		}
		break;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}

class CDeadCrab : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_ALIEN_MONSTER; }
};

LINK_ENTITY_TO_CLASS( monster_headcrab_dead, CDeadCrab );

//=========================================================
// ********** DeadBarney SPAWN **********
//=========================================================
void CDeadCrab :: Spawn( )
{
	PRECACHE_MODEL("models/headcrab.mdl");
	SET_MODEL(ENT(pev), "models/headcrab.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( "dead_crab" );
	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead headcrab with bad pose\n" );
	}

	pev->body = 0;

	pev->health			= 5;

	m_die_dont_move = 1;

	MonsterInitDead();
}

