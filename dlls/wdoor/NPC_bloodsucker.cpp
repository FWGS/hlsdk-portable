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
#define		BL_AE_JUMPATTACK	( 2 )

Task_t	tlBLRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slBLRangeAttack1[] =
{
	{ 
		tlBLRangeAttack1,
		ARRAYSIZE ( tlBLRangeAttack1 ), 
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED,
		0,
		"BLRangeAttack1"
	},
};

Task_t	tlBLRangeAttack1Fast[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slBLRangeAttack1Fast[] =
{
	{ 
		tlBLRangeAttack1Fast,
		ARRAYSIZE ( tlBLRangeAttack1Fast ), 
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED,
		0,
		"BLRAFast"
	},
};

class CBloodSucker : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void RunTask ( Task_t *pTask );
	void StartTask ( Task_t *pTask );
	void SetYawSpeed ( void );
	void EXPORT LeapTouch ( CBaseEntity *pOther );

	void PainSound( void );
	void DeathSound( void );
	int  Classify ( void );
	void RunAI( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	void Killed( entvars_t *pevAttacker, int iGib );

	//virtual float GetDamageAmount( void ) { return gSkillData.headcrabDmgBite; }
	virtual int GetVoicePitch( void ) { return 100; }
	virtual float GetSoundVolue( void ) { return 1.0; }
	Schedule_t* GetScheduleOfType ( int Type );

	CUSTOM_SCHEDULES;

	static const char *pPainSounds[];
	static const char *pDeathSounds[];
	static const char *pBiteSounds[];
};
LINK_ENTITY_TO_CLASS( monster_bloodsucker, CBloodSucker );

DEFINE_CUSTOM_SCHEDULES( CBloodSucker )
{
	slBLRangeAttack1,
	slBLRangeAttack1Fast,
};

IMPLEMENT_CUSTOM_SCHEDULES( CBloodSucker, CBaseMonster );

const char *CBloodSucker::pPainSounds[] = 
{
	"monster/bl_pain1.wav",
	"monster/bl_pain2.wav",
};

const char *CBloodSucker::pDeathSounds[] = 
{
	"monster/bl_die1.wav",
	"monster/bl_die2.wav",
	"monster/bl_die3.wav",
};

const char *CBloodSucker::pBiteSounds[] = 
{
	"headcrab/hc_headbite.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CBloodSucker :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBloodSucker :: SetYawSpeed ( void )
{
	pev->yaw_speed = 120;
}

void CBloodSucker :: RunAI( void )
{
	if(pev->weapons == 1){//��ˮģʽ��δ���ֵ���ǰ����������ͻϮ
			m_flDistLook = 768.0;
			m_singdelay_max = 0;
			m_singdelay_use = m_singdelay_max;
			pev->weapons = 2;
			pev->takedamage = DAMAGE_NO;
			pev->effects |= EF_NODRAW;
			m_longming = 1;
			m_selfmode = TRUE;
			pev->solid	= SOLID_NOT;
	}
	else if(pev->weapons >= 2){
			if(pev->weapons == 2 && m_hEnemy != NULL){
			pev->weapons = 3;
			pev->takedamage = DAMAGE_AIM;
			pev->effects &= ~EF_NODRAW;
			pev->solid	= SOLID_SLIDEBOX;
			}
			if ( pev->waterlevel >= 1){
			Killed( pev, GIB_ALWAYS );//��ʬ!
			return;
			}
	}

	if(pev->sequence == LookupActivity ( ACT_RUN ) || pev->sequence == LookupActivity ( ACT_RUN_SCARED )){
	m_flGroundSpeed = 240;
	}

	if(pev->body == 1 && m_killed_exp != 100){//������ɫ��Ѫħ
		if (g_iSkillLevel == SKILL_HARD){
		pev->health			= 360;
		}
		else{
		pev->health			= 300;
		}
		pev->max_health			= pev->health;
		m_killed_exp = 100;
		m_rpgms_level = 60;
		m_flFieldOfView	= 0;
		m_facing_fucking_mode = 1;
		pev->netname = MAKE_STRING( "Golden.Blood.Drinker" );
	}

	CBaseMonster :: RunAI();
}


void CBloodSucker::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, iGib );
}


BOOL CBloodSucker :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	float dist = 85;

	if(pev->body == 1){
			float dist = 150;
			if(m_hEnemy != NULL){
				if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 60 )
				{
					if (m_hEnemy->pev->flags & FL_ONGROUND)
					{
					dist += 60;
					}
				}
			}

					if (flDist <= dist && m_hEnemy != NULL && flDot >= 0.5)
					{
						if (m_hEnemy->IsAlive() ){
							if(pev->sequence == LookupActivity ( ACT_RUN )){
							pev->sequence = LookupActivity ( ACT_RUN_SCARED );
							ResetSequenceInfo( );
							pev->frame = 0;
							}
						}
					}
					else if (flDist >= dist + 90 )
					{
							if(pev->sequence == LookupActivity ( ACT_RUN_SCARED )){
							pev->sequence = LookupActivity ( ACT_RUN );
							ResetSequenceInfo( );
							pev->frame = 0;
							}
					}

			return FALSE;
	}

	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
	if ( flDist <= dist && flDot >= 0.7 && m_hEnemy != NULL)
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CBloodSucker :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg1,dmg2;
	dmg1 = 30;//��ȭ
	dmg2 = 12;//��ȭ����

	switch( pEvent->event )
	{
		case 1:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_o(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 90;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg1, gpGlobals->v_forward, &tr, DMG_SLASH ); 
			ApplyMultiDamage( pev, pev );

				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/fist_hitbod2.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

				if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) ){
				pEntity->pev->velocity = pEntity->pev->velocity + (pEntity->pev->origin - pev->origin).Normalize() * 300;
				}
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 90, dmg1, DMG_SLASH );
				if(pHurt){
					if (pHurt->Classify() == CLASS_PLAYER || pHurt->Classify() == CLASS_PLAYER_ALLY
					|| pHurt->Classify() == CLASS_HUMAN_ASS || pHurt->Classify() == CLASS_HUMAN_PASSIVE
					|| pHurt->Classify() == CLASS_HUMAN_MILITARY){
					FX_Explosion( pHurt->Center(), 236 );
					}
					else if (pHurt->Classify() == CLASS_ALIEN_MONSTER || pHurt->Classify() == CLASS_ALIEN_MILITARY){
					FX_Explosion( pHurt->Center(), 237 );
					}
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/fist_hitbod2.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

					if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) ){
					pHurt->pev->velocity = pHurt->pev->velocity + (pHurt->pev->origin - pev->origin).Normalize() * 300;
					}
				}
			}

		}
		break;

		case 2:
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

			pev->velocity = vecJumpDir;
			m_flNextAttack = gpGlobals->time + 2;
		}
		break;

		case 3:
		{
				ClearSchedule();
				SetYawSpeed();
		}
		break;

		case 4:
			{
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/fist_hitbod2.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				::RadiusDamage_limit( Center(), pev, pev, 16, 128, CLASS_HUMAN_ASS, DMG_SLASH);
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
void CBloodSucker :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/bloodsucker.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 96));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 240;
	}
	else{
	pev->health			= 200;
	}

	pev->view_ofs		= Vector ( 0, 0, 20 );// position of the eyes relative to monster's origin.
	pev->yaw_speed		= 5;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	pev->gravity		= 1.5;

	MonsterInit();

	m_ignoredamage = 1;
	m_headdef		= 2;//ͷ��Ӳ��

	m_MoveFail_FuckRoad = TRUE;
	m_MoveFail_SimpleRoad = TRUE;
	
	m_killed_exp = 40;
	m_rpgms_level = 40;
	pev->netname = MAKE_STRING( "Blood.Drinker" );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBloodSucker :: Precache()
{
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pBiteSounds);

	PRECACHE_SOUND("zombie/claw_miss1.wav");
	PRECACHE_SOUND("zombie/claw_miss2.wav");

	PRECACHE_MODEL("models/bloodsucker.mdl");
}	


//=========================================================
// RunTask 
//=========================================================
void CBloodSucker :: RunTask ( Task_t *pTask )
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
void CBloodSucker :: LeapTouch ( CBaseEntity *pOther )
{
	if ( !pOther->pev->takedamage || pev->deadflag != DEAD_NO){
		return;
	}

	if ( pOther->Classify() == Classify() || pOther->pev->deadflag != DEAD_NO ){
		return;
	}

	TraceResult tr;

	// Don't hit if back on ground
	if ( !FBitSet( pev->flags, FL_ONGROUND ) )
	{
		ClearMultiDamage( );

		Vector vecDir = pOther->pev->origin - pev->origin;
		vecDir = vecDir.Normalize( );
		Vector monsterangles = UTIL_VecToAngles( vecDir );
		monsterangles.x = 0;
		monsterangles.z = 0;
		UTIL_MakeVectors(monsterangles);

		Vector vecSrc = Center();
		Vector vecEnd	= vecSrc + gpGlobals->v_forward * 48;
		UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

		int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
		int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),0);
		FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

		pOther->TraceAttack(pev, 45, gpGlobals->v_forward, &tr, DMG_SLASH); 
		ApplyMultiDamage( pev, pev );

		EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/fist_hitbod2.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

		if ( pOther->pev->flags & (FL_MONSTER|FL_CLIENT) ){
		pOther->pev->velocity = pOther->pev->velocity + (pOther->pev->origin - pev->origin).Normalize() * 450;
		}
	}
	SetTouch( NULL );
}

void CBloodSucker :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		{
			m_IdealActivity = ACT_RANGE_ATTACK1;
			SetTouch ( &CBloodSucker::LeapTouch );
			break;
		}
	default:
		{
			CBaseMonster :: StartTask( pTask );
		}
	}
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CBloodSucker :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( FBitSet( pev->flags, FL_ONGROUND ) 
	&& flDist >= 128 && flDist <= 256 && flDot >= 0.7 && pev->body == 0 )
	{
		return TRUE;
	}
	return FALSE;
}

int CBloodSucker :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// DeathSound 
//=========================================================
void CBloodSucker :: PainSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

void CBloodSucker :: DeathSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

Schedule_t* CBloodSucker :: GetScheduleOfType ( int Type )
{
	switch	( Type )
	{
		case SCHED_RANGE_ATTACK1:
		{
			return &slBLRangeAttack1[ 0 ];
		}
		break;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}