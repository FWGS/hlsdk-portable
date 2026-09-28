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


#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"squadmonster.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"
#include	"player.h"
#include	"shake.h"
#include	"scripted.h"

extern DLL_GLOBAL int		g_iSkillLevel;

int	 heal_spr;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_GRUNT_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_GRUNT_COVER_AND_RELOAD,
	SCHED_GRUNT_SWEEP,
	SCHED_GRUNT_FOUND_ENEMY,
	SCHED_GRUNT_WAIT_FACE_ENEMY,
	SCHED_GRUNT_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_GRUNT_ELOF_FAIL,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_GRUNT_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_GRUNT_CHECK_FIRE,
	TASK_FORGET_ENEMY,
};


//=========================================================
// Black ball attack
//=========================================================
class CBlackMagicBall : public CBaseMonster
{
	void Spawn( void );
	void Precache( void );
	void EXPORT HuntThink( void );
	void EXPORT DieThink( void );
	void EXPORT BounceTouch( CBaseEntity *pOther );
	void MovetoTarget( Vector vecTarget );
	int m_iTrail;
	int m_flNextAttack;
	Vector m_vecIdeal;
	EHANDLE m_hOwner;
};
LINK_ENTITY_TO_CLASS( black_magic_ball, CBlackMagicBall );//��ɫħ��ʯ!

void CBlackMagicBall :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/black_ball.mdl");

	pev->frame = 0;
	pev->framerate = 1.0;

	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin( pev, pev->origin );

	SetThink( &CBlackMagicBall::HuntThink );
	SetTouch( &CBlackMagicBall::BounceTouch );

	m_vecIdeal = Vector( 0, 0, 0 );

	FX_Trail(pev->origin, entindex(), 88 );

	pev->nextthink = gpGlobals->time + 0.1;

	m_hOwner = Instance( pev->owner );
	pev->dmgtime = gpGlobals->time;
	pev->flags |= FL_NOTARGET;
}


void CBlackMagicBall :: Precache( void )
{
	PRECACHE_MODEL("models/black_ball.mdl");
}


void CBlackMagicBall :: HuntThink( void  )
{
	pev->nextthink = gpGlobals->time + 0.1;

	// check world boundaries
	if (pev->frags >= 5 || gpGlobals->time - pev->dmgtime > 6 || m_hOwner == NULL)
	{
		SetTouch( NULL );
		StopAnimation();
		SetThink ( NULL );
		FX_Trail( pev->origin, entindex(), PROJ_REMOVE );
		SUB_StartFadeOut();
		return;
	}

	if(m_hEnemy != NULL){
		MovetoTarget( m_hEnemy->Center( ) );

		if (pev->frags < 5 && m_flNextAttack < gpGlobals->time 
		&& (m_hEnemy->Center() - pev->origin).Length() <= 90)
		{
			m_hEnemy->TakeDamage( pev, m_hOwner->pev, 80, DMG_DARK );
			FX_Trail( pev->origin, entindex(), 90);

			UTIL_EmitAmbientSound( ENT(pev), pev->origin, "blues/black_ball_hit.wav", 1.0, 0.7, 0, 100 );

			m_flNextAttack = gpGlobals->time + 0.5;
			pev->nextthink = gpGlobals->time + 0.5;
			pev->frags++;
		}
	}
	else{
			CBaseMonster *pAlly;
			pAlly = m_hOwner->MyMonsterPointer();
			if(pAlly){
			m_hEnemy = pAlly->m_hEnemy;
			}

			MovetoTarget( m_hOwner->Center( ) );
	}
}


void CBlackMagicBall :: DieThink( void  )
{
	UTIL_Remove( this );
}


void CBlackMagicBall :: MovetoTarget( Vector vecTarget )
{
	// accelerate
	float flSpeed = m_vecIdeal.Length();
	if (flSpeed == 0)
	{
		m_vecIdeal = pev->velocity;
		flSpeed = m_vecIdeal.Length();
	}

	if (flSpeed > 600)
	{
		m_vecIdeal = m_vecIdeal.Normalize( ) * 600;
	}
	m_vecIdeal = m_vecIdeal + (vecTarget - pev->origin).Normalize() * 200;
	pev->velocity = m_vecIdeal;
}

void CBlackMagicBall::BounceTouch( CBaseEntity *pOther )
{
	Vector vecDir = m_vecIdeal.Normalize( );

	TraceResult tr = UTIL_GetGlobalTrace( );

	float n = -DotProduct(tr.vecPlaneNormal, vecDir);

	vecDir = 4.0 * tr.vecPlaneNormal * n + vecDir;

	m_vecIdeal = vecDir * m_vecIdeal.Length();
}

class CMagicHeal : public CBaseEntity
{
public:
	void Spawn( void );

	void EXPORT AnimateThink( void );
};

LINK_ENTITY_TO_CLASS( magic_heal, CMagicHeal );
void CMagicHeal:: Spawn( void )
{
	SET_MODEL(ENT(pev), "models/mfz.mdl");

	UTIL_SetSize( pev, g_vecZero, g_vecZero);

	pev->frame = 0;
	pev->framerate = 1.0;

	pev->health = 70;
	pev->frags = 1;
	pev->effects |= EF_DIMLIGHT;

	pev->classname = MAKE_STRING( "magic_heal" );

	SetThink ( &CMagicHeal::AnimateThink );
	pev->nextthink = gpGlobals->time + 0.1;
}

void HealEffect( Vector org )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, org );
	WRITE_BYTE( TE_SPRITE );
	WRITE_COORD( org.x);	// pos
	WRITE_COORD( org.y);
	WRITE_COORD( org.z);
	WRITE_SHORT( heal_spr );		// model
	WRITE_BYTE( 4 );				// size * 10
	WRITE_BYTE( 255 );			// brightness
	MESSAGE_END();
}

void CMagicHeal::AnimateThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;
	pev->health--;

	if(pev->health <= 0){
		UTIL_Remove( this );
		return;
	}
}

//=========================================================
// Bullsquid's spit projectile
//=========================================================
class CFaBall : public CBaseEntity
{
public:
	void Spawn( void );

	static void Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	void Touch( CBaseEntity *pOther );
	void EXPORT Animate( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int  m_maxFrame;
};

LINK_ENTITY_TO_CLASS( fa_ball, CFaBall );

TYPEDESCRIPTION	CFaBall::m_SaveData[] = 
{
	DEFINE_FIELD( CFaBall, m_maxFrame, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CFaBall, CBaseEntity );

void CFaBall:: Spawn( void )
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING( "fa_ball" );
	
	pev->solid = SOLID_BBOX;
	
	SET_MODEL(ENT(pev), "sprites/anim_spr2.spr");
	pev->frame = 0;
	pev->scale = 0.1;

	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 200;
	pev->rendercolor.x = 64;
	pev->rendercolor.y = 255;
	pev->rendercolor.z = 64;

	UTIL_SetSize( pev, g_vecZero, g_vecZero);

	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;

	FX_Trail(pev->origin, entindex(), 87);
}

void CFaBall::Animate( void )
{
		pev->nextthink = gpGlobals->time + 0.1;

		pev->armorvalue += 1;
		if(pev->armorvalue > 40){
			FX_Trail( pev->origin, entindex(), PROJ_REMOVE );
			UTIL_Remove( this );
			return;
		}

		if(pev->waterlevel > 0){
			FX_Trail( pev->origin, entindex(), PROJ_REMOVE );
			UTIL_Remove( this );
			return;
		}

		if ( pev->frame++ )
		{
			if ( pev->frame > m_maxFrame )
			{
				pev->frame = 0;
			}
		}
}

void CFaBall::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CFaBall *pSpit = GetClassPtr( (CFaBall *)NULL );
	pSpit->Spawn();
	
	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);
	pSpit->pev->angles = UTIL_VecToAngles (pSpit->pev->velocity);
	pSpit->SetThink ( &CFaBall::Animate );
	pSpit->pev->nextthink = gpGlobals->time + 0.1;
}

void CFaBall :: Touch ( CBaseEntity *pOther )
{
	if(pev->armortype > 0){
	return;
	}

	int		iPitch;

	if ( UTIL_PointContents(pev->origin) == CONTENT_SKY )
	{
		FX_Trail( pev->origin, entindex(), PROJ_REMOVE );
		UTIL_Remove( this );
		return;
	}

	// splat sound
	iPitch = RANDOM_FLOAT( 90, 110 );	

//	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/shock_blast.wav", 1, ATTN_NORM, 0, iPitch );	

	TraceResult tr = UTIL_GetGlobalTrace( );
	if (tr.pHit == pOther->edict())//ֱ������!
	{
		ClearMultiDamage( );
		pOther->TraceAttack(pev, 50, gpGlobals->v_forward, &tr, DMG_GENERIC ); 

		UTIL_MakeVectors ( pev->angles );
		Vector vecSrc = Center();
		Vector vecEnd	= vecSrc + gpGlobals->v_forward * 30;
		int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
		int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,0,CLASS_PLAYER);
		FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

		if(!FNullEnt(pev->owner)){
		entvars_t	*pevOwner;
		pevOwner = VARS( pev->owner );
		ApplyMultiDamage( pev, pevOwner );
		}
		else{
		ApplyMultiDamage( pev, pev );
		}

	}

	FX_Trail( pev->origin, entindex(), PROJ_REMOVE );
	FX_Trail( pev->origin, entindex(), 89);

	SetThink ( &CFaBall::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}



//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_GRUNT_NOFIRE	( bits_COND_SPECIAL1 )

class CBlues : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed ( void );
	int  Classify ( void );
	int ISoundMask ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL FCanCheckAttacks ( void );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );

	void RunAI( void );
	void Killed( entvars_t *pevAttacker, int iGib );

	void SetActivity ( Activity NewActivity );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	void DeathSound( void );

	Vector GetGunPosition( void );

	void GibMonster( void );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	
	Schedule_t	*GetSchedule( void );
	Schedule_t  *GetScheduleOfType ( int Type );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	virtual int	ObjectCaps( void ) { return CBaseMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	void EXPORT		FollowerUse2( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int IRelationship ( CBaseEntity *pTarget );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float m_flNextShootTime;
	float m_flNextShoot2Time;
	float m_flNextHealTime;
	float m_flLastEnemySightTime;
//	float m_flNextZhaoHuanTime;

	BOOL	m_fHealAll;

	EHANDLE	mps_ent;

	float	m_dyingtime;
	int		m_dyinguse;

	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;
};


LINK_ENTITY_TO_CLASS( monster_blues, CBlues );

void CBlues :: FollowerUse2( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	
	if ( IsAlive() && pCaller != NULL && pCaller->IsPlayer())
	{
			if ( m_MonsterState == MONSTERSTATE_SCRIPT || m_IdealMonsterState == MONSTERSTATE_SCRIPT )
			{
				if(!m_pCine->CanInterrupt()){
					return;
				}
			}

			/*
			if(m_rpgms_inteam > 0 && m_enemyfollower == 0){//���¸���
			m_enemyfollower = 1;
			return;
			}
			else if(m_enemyfollower_combat == 0 && m_enemyfollower == 1){//ԭ�ش���
			m_enemyfollower = 0;
			m_enemyfollower_combat = 0;
			m_hEnemy = NULL;
			return;
			}
			*/

			/*	Bug Fix 3.0
			if(m_enemyfollower == 0 && CVAR_GET_FLOAT( "cshl623_debug_mode" ) == 1999){
					if(m_rpgms_type >= 1){
						CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pCaller->pev);
						if(pPlayer){
							if(pPlayer->HasTeamMate_CanAdd(this)){
							pPlayer->TeamMate_add(this);//�����ڶ����У�������Ҥζ���
							m_enemyfollower = 1;//debug mode
							}
						}
					}	
			}
			*/
			
			if(m_hEnemy != NULL){
				if(!m_hEnemy->IsPlayer() || !m_igonre_npc){
				m_igonre_npc = 10;//��ʱ����NPC
				m_cover_fromplayer = TRUE;
				m_hEnemy = NULL;
				m_hOldEnemy[0] = NULL;
				m_hOldEnemy[1] = NULL;
				m_hOldEnemy[2] = NULL;
				m_hOldEnemy[3] = NULL;
				ClearSchedule();
				SetYawSpeed();
				}
				else{
				m_cover_fromplayer = FALSE;
				m_igonre_npc = 0;
				}
			}
			else{
				m_alert = 100;
			}
	}
}

TYPEDESCRIPTION	CBlues::m_SaveData[] = 
{
	DEFINE_FIELD( CBlues, m_fHealAll, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBlues, m_flNextHealTime, FIELD_TIME ),
	DEFINE_FIELD( CBlues, m_flNextShootTime, FIELD_TIME ),
	DEFINE_FIELD( CBlues, m_flNextShoot2Time, FIELD_TIME ),
//	DEFINE_FIELD( CBlues, m_flNextZhaoHuanTime, FIELD_TIME ),
	DEFINE_FIELD( CBlues, m_dyingtime, FIELD_TIME ),
	DEFINE_FIELD( CBlues, m_dyinguse, FIELD_INTEGER ),
	DEFINE_FIELD( CBlues, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CBlues, m_lastAttackCheck, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CBlues, CSquadMonster );

//=========================================================
// IRelationship - overridden because Alien Grunts are 
// Human Grunt's nemesis.
//=========================================================
int CBlues::IRelationship ( CBaseEntity *pTarget )
{
	return CSquadMonster::IRelationship( pTarget );
}


//=========================================================
// RunAI
//=========================================================
void CBlues :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if ( m_MonsterState == MONSTERSTATE_PRONE || m_IdealMonsterState == MONSTERSTATE_PRONE ){
		m_dyingtime++;
		if(m_dyingtime > 30){//��������
		BarnacleVictimReleased();
		m_dyingtime = -20;
		m_canbarnacle_mode  = 0;
		}
	}
	else if(m_dyingtime > 0){
		m_dyingtime--;
	}
	else if(m_dyingtime < 0){
		m_dyingtime++;
		if(m_dyingtime == 0){
		m_canbarnacle_mode  = 1;
		}
	}

	if(pev->deadflag == DEAD_NO){
			if(pev->health < pev->max_health){//����
			TakeHealth(1, DMG_GENERIC);
			}

			if(m_flNextHealTime <= gpGlobals->time){
				if(m_hPlayer != NULL  
				&& pev->sequence != LookupActivity ( ACT_RANGE_ATTACK2 )
				&& pev->sequence != LookupActivity ( ACT_HOP )
				&& pev->sequence != LookupActivity ( ACT_LEAP )
				&& !m_fHealAll){//����֮��
						m_PlayerHealth = m_hPlayer->pev->health;
						if( (m_PlayerHealth < m_hPlayer->pev->max_health * 0.8)
						|| (pev->health < pev->max_health * 0.6) ){//��һ��Լ������ˣ���Ҫ����!
							m_fHealAll = TRUE;
							m_igonre_npc = 50;
						}
						else if(m_hTeamMate1 != NULL && m_hTeamMate1->pev->deadflag == DEAD_NO 
							&& m_hTeamMate1->pev->health < m_hTeamMate1->pev->max_health * 0.8){
							m_fHealAll = TRUE;
							m_igonre_npc = 50;
						}
						else if(m_hTeamMate2 != NULL && m_hTeamMate2->pev->deadflag == DEAD_NO 
							&& m_hTeamMate2->pev->health < m_hTeamMate2->pev->max_health * 0.8){
							m_fHealAll = TRUE;
							m_igonre_npc = 50;
						}
						else if(m_hTeamMate3 != NULL && m_hTeamMate3->pev->deadflag == DEAD_NO 
							&& m_hTeamMate3->pev->health < m_hTeamMate3->pev->max_health * 0.8){
							m_fHealAll = TRUE;
							m_igonre_npc = 50;
						}
						else if(m_hTeamMate4 != NULL && m_hTeamMate4->pev->deadflag == DEAD_NO 
							&& m_hTeamMate4->pev->health < m_hTeamMate4->pev->max_health * 0.8){
							m_fHealAll = TRUE;
							m_igonre_npc = 50;
						}
				}
			}
	}
		
		if(pev->sequence == LookupActivity ( ACT_RUN )){
		   m_flGroundSpeed = 300;
		}
		if(pev->sequence == LookupActivity ( ACT_WALK )){
		    m_flGroundSpeed = 100;
		}

		if ( pev->movetype == MOVETYPE_TOSS && !m_groundElev )
		{
			if (pev->flags & FL_ONGROUND)
			{
				pev->movetype = MOVETYPE_STEP;
			}
		}
		
}


void CBlues::Killed( entvars_t *pevAttacker, int iGib )
{
	CSquadMonster::Killed( pevAttacker, GIB_NEVER );
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CBlues :: GibMonster ( void )
{
	CBaseMonster :: GibMonster();
}

//=========================================================
// ISoundMask - Overidden for human grunts because they 
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CBlues :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER	|
			bits_SOUND_DANGER;
}

//=========================================================
// FCanCheckAttacks - this is overridden for human grunts
// because they can throw/shoot grenades when they can't see their
// target and the base class doesn't check attacks if the monster
// cannot see its enemy.
//
// !!!BUGBUG - this gets called before a 3-round burst is fired
// which means that a friendly can still be hit with up to 2 rounds. 
// ALSO, grenades will not be tossed if there is a friendly in front,
// this is a bad bug. Friendly machine gun fire avoidance
// will unecessarily prevent the throwing of a grenade as well.
//=========================================================
BOOL CBlues :: FCanCheckAttacks ( void )
{
	if ( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CBlues :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	return FALSE;
}

//=========================================================
// CheckRangeAttack1 - overridden for HGrunt, cause 
// FCanCheckAttacks() doesn't disqualify all attacks based
// on whether or not the enemy is occluded because unlike
// the base class, the HGrunt can attack when the enemy is
// occluded (throw grenade over wall, etc). We must 
// disqualify the machine gun attack if the enemy is occluded.
//=========================================================
BOOL CBlues :: CheckRangeAttack1 ( float flDot, float flDist )
{
	float dist = 2048;
	if(m_flNextShootTime > gpGlobals->time){
	return FALSE;
	}

			if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= dist && flDot >= 0.6  )
			{

			//���˼��
			TraceResult	tr;

			if ( gpGlobals->time > m_checkAttackTime )
			{
				TraceResult tr;

				Vector shootOrigin = pev->origin + Vector(0,0,48);
				CBaseEntity *pEnemy = m_hEnemy;
				if ( !pEnemy )
				{
					m_lastAttackCheck = FALSE;
					m_checkAttackTime = gpGlobals->time + 0.5;
					return FALSE;
				}

				if(FClassnameIs( m_hEnemy->pev, "npc_attack_flag" ) && m_hEnemy->pev->impulse == 1){
					m_lastAttackCheck = FALSE;
					m_checkAttackTime = gpGlobals->time + 0.5;
					return FALSE;
				}

				Vector shootTarget = ( (pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin) + m_vecEnemyLKP );
				UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, ignore_glass, ENT(pev), &tr );
				if ( tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy) ){
					m_checkAttackTime = gpGlobals->time + 0.2;
					m_lastAttackCheck = TRUE;
				}
				else{
						CBaseEntity *pHitEntity = CBaseEntity::Instance(tr.pHit);
						if(pHitEntity){
							if(pHitEntity->Classify() == CLASS_ALIEN_MONSTER
							|| pHitEntity->Classify() == CLASS_ALIEN_MILITARY
							|| pHitEntity->Classify() == CLASS_MACHINE
							|| pHitEntity->Classify() == CLASS_MACHINE_ASS
							|| pHitEntity->Classify() == CLASS_HUMAN_ASS
							|| pHitEntity->Classify() == CLASS_HUMAN_MILITARY){
							m_checkAttackTime = gpGlobals->time + 0.25;
							m_lastAttackCheck = TRUE;
							}
							else{
							m_lastAttackCheck = FALSE;
							m_checkAttackTime = gpGlobals->time + 0.5;
							}
						}
						else{
							m_lastAttackCheck = FALSE;
							m_checkAttackTime = gpGlobals->time + 0.5;
						}
				}
			}
			return m_lastAttackCheck;


			}


	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack. 
//=========================================================
BOOL CBlues :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if(m_flNextShoot2Time > gpGlobals->time){
	return FALSE;
	}

	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048)
	{
		return TRUE;
	}

	return FALSE;
}


//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
void CBlues :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
int CBlues :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	m_alert	= 100;

	if(pev->sequence == LookupActivity ( ACT_RELOAD )){//��֮�赸����Ч��
		if(!(bitsDamageType & (DMG_AIR | DMG_FALL | DMG_DROWN | DMG_NERVEGAS))){
			if (RANDOM_LONG(0,100) <= 45){
			return 0;
			}
		}
	}

	//ħ������
	if ( (bitsDamageType & DMG_ENERGYBEAM)
	|| (bitsDamageType & DMG_DARK)
	|| (bitsDamageType & DMG_SONIC)
	|| (bitsDamageType & DMG_FREEZE)
	|| (bitsDamageType & DMG_BURN)
	|| (bitsDamageType & DMG_SHOCK)
	|| (bitsDamageType & DMG_ENERGYBLAST)){
		flDamage *= 0.5;
	}

	if(pev->sequence == LookupActivity ( ACT_USE )){//���Ƽ���ʹ��ʱ����!
	flDamage *= 0.5;
	}

	Forget( bits_MEMORY_INCOVER );

	return CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBlues :: SetYawSpeed ( void )
{
	pev->yaw_speed = 240;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CBlues :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

Vector CBlues :: GetGunPosition( )
{
	return pev->origin + Vector( 0, 0, 60 );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CBlues :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case 1:
		{
			if(m_fHealAll){//����֮����!
				if(m_flNextHealTime <= gpGlobals->time){
					pev->velocity = g_vecZero;
					CBaseEntity *pHealent = Create( "magic_heal", pev->origin, pev->angles, edict());
					m_flNextHealTime = gpGlobals->time + 60;
					EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "blues/heal_use.wav", 1.0, 0.6,0,100);
				}
			}
		}
		break;

		case 7:
		{//����֮��ر�!
			m_fHealAll = FALSE;
		}
		break;

		case 8:
		{
			ClearSchedule();
			SetYawSpeed();
		}
		break;

		case 2:
		{//����֮������
				CBaseMonster *pAlly;
				if(pev->deadflag == DEAD_NO &&
				pev->health != pev->max_health){//������Ѫ
				HealEffect(Center());
				TakeHealth(pev->max_health * 0.1, DMG_GENERIC);
				}
				if(m_hPlayer != NULL){
					if(m_hPlayer->pev->deadflag == DEAD_NO && 
					m_hPlayer->pev->health != m_hPlayer->pev->max_health){
					HealEffect(m_hPlayer->Center());
					m_hPlayer->TakeHealth(m_hPlayer->pev->max_health * 0.05, DMG_GENERIC);
					}
				}
				if(m_hTeamMate1 != NULL){
					pAlly = m_hTeamMate1->MyMonsterPointer();
					if(pAlly->pev->deadflag == DEAD_NO &&
					pAlly->pev->health != pAlly->pev->max_health){
					HealEffect(pAlly->Center());
					pAlly->TakeHealth(pAlly->pev->max_health * 0.05, DMG_GENERIC);
					}
				}
				if(m_hTeamMate2 != NULL){
					pAlly = m_hTeamMate2->MyMonsterPointer();
					if(pAlly->pev->deadflag == DEAD_NO &&
					pAlly->pev->health != pAlly->pev->max_health){
					HealEffect(pAlly->Center());
					pAlly->TakeHealth(pAlly->pev->max_health * 0.05, DMG_GENERIC);
					}
				}
				if(m_hTeamMate3 != NULL){
					pAlly = m_hTeamMate3->MyMonsterPointer();
					if(pAlly->pev->deadflag == DEAD_NO &&
					pAlly->pev->health != pAlly->pev->max_health){
					HealEffect(pAlly->Center());
					pAlly->TakeHealth(pAlly->pev->max_health * 0.05, DMG_GENERIC);
					}
				}
				if(m_hTeamMate4 != NULL){
					pAlly = m_hTeamMate4->MyMonsterPointer();
					if(pAlly->pev->deadflag == DEAD_NO &&
					pAlly->pev->health != pAlly->pev->max_health){
					HealEffect(pAlly->Center());
					pAlly->TakeHealth(pAlly->pev->max_health * 0.05, DMG_GENERIC);
					}
				}
		}
		break;

		case 3:
		{
			if(m_hEnemy != NULL){
				if(( pev->origin - m_hEnemy->pev->origin).Length2D() < 128){
				pev->flags &= ~FL_ONGROUND;
				UTIL_MakeVectors(pev->angles);
				pev->velocity = gpGlobals->v_forward * -384;
				pev->velocity.z += 128;
				}
			}
		}
		break;

		case 4:
		{
			if(m_hEnemy != NULL){
				if(m_hEnemy->IsPlayer() && m_lovehate > 0){
				ClearSchedule();
				SetYawSpeed();
				}
				else{
				UTIL_MakeVectors(pev->angles);
				Vector vecShootOrigin = pev->origin + Vector(0,0,48);
				Vector vecShootDir = ShootAtEnemy( vecShootOrigin );
				Vector angDir = UTIL_VecToAngles( vecShootDir );
				SetBlending( 0, angDir.x );
				}
			}
			else{
				ClearSchedule();
				SetYawSpeed();
			}
		}
		break;

		case 5:
		{
			if(m_flNextShootTime <= gpGlobals->time && m_hEnemy != NULL){
			Vector	vecSpitOffset,vangle;
			Vector	vecSpitDir;

			UTIL_MakeVectors ( pev->angles );

			// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
			// we should be able to read the position of bones at runtime for this info.
			GetAttachment( 0, vecSpitOffset, vangle );

			vecSpitDir = ( m_hEnemy->Center() - vecSpitOffset ).Normalize();

			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "blues/fa_ball_fire.wav", 1.0, ATTN_NORM,0,100 + RANDOM_LONG(-5,5) );

			CFaBall::Shoot( pev, vecSpitOffset, vecSpitDir * 1200 );
			m_flNextShootTime = gpGlobals->time + 2;
			}
			else{
				if(m_hEnemy == NULL || (m_hEnemy->IsPlayer() && m_lovehate > 0) ){
				ClearSchedule();
				SetYawSpeed();
				}
			}
		}
		break;

		case 6:
		{
			if(m_flNextShoot2Time <= gpGlobals->time && m_hEnemy != NULL){
				if( !m_hEnemy->IsPlayer() ){
				Vector	vecSpitOffset,vangle;
				Vector	vecSpitDir;

				GetAttachment( 0, vecSpitOffset, vangle );

				CBaseMonster *pBall = (CBaseMonster*)Create( "black_magic_ball", vecSpitOffset, pev->angles, edict() );

				pBall->pev->velocity = Vector( 0, 0, 32 );

				pBall->m_hEnemy = m_hEnemy;

				m_flNextShoot2Time = gpGlobals->time + 20;

				EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "blues/black_ball_atk.wav", 1.0, ATTN_NORM,0,100 + RANDOM_LONG(-5,5) );
				}
			}
			else{
				ClearSchedule();
				SetYawSpeed();
			}
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
void CBlues :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/blues.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;

	pev->health			= 500;
	m_lovehate			= 50;

	m_flFieldOfView		 = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		 = MONSTERSTATE_NONE;

	m_flNextShootTime    = gpGlobals->time + 2;
	m_flNextShoot2Time   = gpGlobals->time + 2;
	m_flNextHealTime     = gpGlobals->time + 2;

	m_afCapability		= bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;

	m_HackedGunPos = Vector ( 0, 0, 55 );

	m_fHealAll = FALSE;

	m_cClipSize		= 1;

	m_cAmmoLoaded		= m_cClipSize;
	m_canheadcrab_mode  = 0;
	m_canbarnacle_mode  = 1;
	m_aimenemy_mod		= 0;

	m_no_victdance		= 1;
	
	m_dyingtime			= 0;
	m_dyinguse			= 0;

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();

	m_follow_mode		= 1;
	m_ignoredamage		= 1;
	m_headdef			= 2;

	m_candrownwater = 1;
//	m_forcefuckdoor = TRUE;
	m_chase_mode = 2;
	m_chase_failed_max = 4;

	SetUse( &CBlues::FollowerUse2 );

	pev->body = 0;
	SetTouch( &CBlues::DeadTouch );

	m_rpgms_actor = 20;
	m_rpgms_level = 60;
	m_rpgms_exp = 0;
	m_rpgms_type = 1;

	m_new_ally_type = TRUE;
	pev->netname = MAKE_STRING( "Blues" );

	m_rpgms_skill1_learn = 63;//����֮��
	m_rpgms_skill2_learn = 64;//����
	m_rpgms_skill3_learn = 65;//����
	m_rpgms_skill4_learn = 38;//ħ������
	m_rpgms_skill5_learn = 66;//��֮�赸
	m_rpgms_skill6_learn = 67;//����

	pev->takedamage = DAMAGE_YES;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBlues :: Precache()
{
	PRECACHE_MODEL("models/blues.mdl");
	PRECACHE_MODEL("models/mfz.mdl");

	PRECACHE_SOUND("blues/black_ball_atk.wav" );
	PRECACHE_SOUND("blues/black_ball_hit.wav" );
	PRECACHE_SOUND("blues/fa_ball_fire.wav" );
	PRECACHE_SOUND("blues/heal_use.wav" );

	UTIL_PrecacheOther( "black_magic_ball" );

	heal_spr = PRECACHE_MODEL("sprites/heal.spr");
}	

//=========================================================
// start task
//=========================================================
void CBlues :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_FORGET_ENEMY:
		m_hEnemy = NULL;
		m_hOldEnemy[0] = NULL;
		m_hOldEnemy[1] = NULL;
		m_hOldEnemy[2] = NULL;
		m_hOldEnemy[3] = NULL;
		ClearSchedule();
		SetYawSpeed();
		//SetConditions( bits_COND_SCHEDULE_DONE );
		//m_iTaskStatus = TASKSTATUS_COMPLETE;
		TaskComplete();
		break;

	case TASK_GRUNT_CHECK_FIRE:
		if ( !m_lastAttackCheck )
		{
			SetConditions( bits_COND_GRUNT_NOFIRE );
		}
		TaskComplete();
		break;
	
	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
	case TASK_STRAFE_PATH:
		// grunt no longer assumes he is covered if he moves
		Forget( bits_MEMORY_INCOVER );
		CSquadMonster ::StartTask( pTask );
		break;

	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	case TASK_GRUNT_FACE_TOSS_DIR:
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CSquadMonster :: StartTask( pTask );
		break;

	default: 
		CSquadMonster :: StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CBlues :: RunTask ( Task_t *pTask )
{
	CSquadMonster :: RunTask( pTask );
}

//=========================================================
// DeathSound 
//=========================================================
void CBlues :: DeathSound ( void )
{
	//EMIT_SOUND_DYN( ENT(pev), 6, "misaliya/die2.wav", 1, 0.7, 0, 100);
}

//=========================================================
// GruntFail
//=========================================================
Task_t	tlBLUEFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)1		},
	{ TASK_FORGET_ENEMY,		(float)0	},
};

Schedule_t	slBLUEFail[] =
{
	{
		tlBLUEFail,
		ARRAYSIZE ( tlBLUEFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"Grunt Fail"
	},
};

//=========================================================
// Grunt Combat Fail
//=========================================================
Task_t	tlBLUECombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)1		},
};

Schedule_t	slBLUECombatFail[] =
{
	{
		tlBLUECombatFail,
		ARRAYSIZE ( tlBLUECombatFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"Grunt Combat Fail"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t	tlBLUEVictoryDance[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
};

Schedule_t	slBLUEVictoryDance[] =
{
	{ 
		tlBLUEVictoryDance,
		ARRAYSIZE ( tlBLUEVictoryDance ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_SEE_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE  |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"GruntVictoryDance"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the grunt to attack.
//=========================================================
Task_t tlBLUEEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_GRUNT_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slBLUEEstablishLineOfFire[] =
{
	{ 
		tlBLUEEstablishLineOfFire,
		ARRAYSIZE ( tlBLUEEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"GruntEstablishLineOfFire"
	},
};

//=========================================================
// GruntFoundEnemy - grunt established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t	tlBLUEFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slBLUEFoundEnemy[] =
{
	{ 
		tlBLUEFoundEnemy,
		ARRAYSIZE ( tlBLUEFoundEnemy ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"GruntFoundEnemy"
	},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t	tlBLUECombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_RELOAD			},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)0.5					},
};

Schedule_t	slBLUECombatFace[] =
{
	{ 
		tlBLUECombatFace1,
		ARRAYSIZE ( tlBLUECombatFace1 ), 
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_RANGE_ATTACK2		|
		0,
		0,
		"Combat Face"
	},
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or grunt gets hurt.
//=========================================================
Task_t	tlBLUESignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
};

Schedule_t	slBLUESignalSuppress[] =
{
	{ 
		tlBLUESignalSuppress,
		ARRAYSIZE ( tlBLUESignalSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"SignalSuppress"
	},
};

Task_t	tlBLUESuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slBLUESuppress[] =
{
	{ 
		tlBLUESuppress,
		ARRAYSIZE ( tlBLUESuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Suppress"
	},
};


//=========================================================
// grunt wait in cover - we don't allow danger or the ability
// to attack to break a grunt's run to cover schedule, but
// when a grunt is in cover, we do want them to attack if they can.
//=========================================================
Task_t	tlBLUEWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slBLUEWaitInCover[] =
{
	{ 
		tlBLUEWaitInCover,
		ARRAYSIZE ( tlBLUEWaitInCover ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2,

		bits_SOUND_DANGER,
		"GruntWaitInCover"
	},
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t	tlBLUETakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_GRUNT_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_GRUNT_WAIT_FACE_ENEMY	},
};

Schedule_t	slBLUETakeCover[] =
{
	{ 
		tlBLUETakeCover1,
		ARRAYSIZE ( tlBLUETakeCover1 ), 
		bits_COND_ENEMY_DEAD,
		0,
		"TakeCover"
	},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlBLUEGrenadeCover1[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FIND_COVER_FROM_ENEMY,			(float)99							},
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY,	(float)384							},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_SPECIAL_ATTACK1			},
	{ TASK_CLEAR_MOVE_WAIT,					(float)0							},
	{ TASK_RUN_PATH,						(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_GRUNT_WAIT_FACE_ENEMY	},
};

Schedule_t	slBLUEGrenadeCover[] =
{
	{ 
		tlBLUEGrenadeCover1,
		ARRAYSIZE ( tlBLUEGrenadeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlBLUETossGrenadeCover1[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slBLUETossGrenadeCover[] =
{
	{ 
		tlBLUETossGrenadeCover1,
		ARRAYSIZE ( tlBLUETossGrenadeCover1 ), 
		0,
		0,
		"TossGrenadeCover"
	},
};
//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlBLUETakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slBLUETakeCoverFromBestSound[] =
{
	{ 
		tlBLUETakeCoverFromBestSound,
		ARRAYSIZE ( tlBLUETakeCoverFromBestSound ), 
		0,
		0,
		"TakeCoverFromBestSound"
	},
};


//=========================================================
// Grunt reload schedule
//=========================================================
Task_t	tlBLUEHideReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_USE				},
};

Schedule_t slBLUEHideReload[] = 
{
	{
		tlBLUEHideReload,
		ARRAYSIZE ( tlBLUEHideReload ),
		0,
		0,
		"Blues Heal"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlBLUESweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_FORGET_ENEMY,		(float)0	},
};

Schedule_t	slBLUESweep[] =
{
	{ 
		tlBLUESweep,
		ARRAYSIZE ( tlBLUESweep ), 
		
		bits_COND_SEE_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD		|// sound flags
		bits_SOUND_DANGER		|
		bits_SOUND_PLAYER,

		"Grunt Sweep"
	},
};

//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlBLUERangeAttack1A[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
};

Schedule_t	slBLUERangeAttack1A[] =
{
	{ 
		tlBLUERangeAttack1A,
		ARRAYSIZE ( tlBLUERangeAttack1A ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND		|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,
		
		bits_SOUND_DANGER,
		"Range Attack1A"
	},
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlBLUERangeAttack1B[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slBLUERangeAttack1B[] =
{
	{ 
		tlBLUERangeAttack1B,
		ARRAYSIZE ( tlBLUERangeAttack1B ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"Range Attack1B"
	},
};

//=========================================================
// secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlBLUERangeAttack2[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
};

Schedule_t	slBLUERangeAttack2[] =
{
	{ 
		tlBLUERangeAttack2,
		ARRAYSIZE ( tlBLUERangeAttack2 ), 
		0,
		0,
		"RangeAttack2"
	},
};


DEFINE_CUSTOM_SCHEDULES( CBlues )
{
	slBLUEFail,
	slBLUECombatFail,
	slBLUEVictoryDance,
	slBLUEEstablishLineOfFire,
	slBLUEFoundEnemy,
	slBLUECombatFace,
	slBLUESignalSuppress,
	slBLUESuppress,
	slBLUEWaitInCover,
	slBLUETakeCover,
	slBLUEGrenadeCover,
	slBLUETossGrenadeCover,
	slBLUETakeCoverFromBestSound,
	slBLUEHideReload,
	slBLUESweep,
	slBLUERangeAttack1A,
	slBLUERangeAttack1B,
	slBLUERangeAttack2,
};

IMPLEMENT_CUSTOM_SCHEDULES( CBlues, CSquadMonster );

//=========================================================
// SetActivity 
//=========================================================
void CBlues :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RUN:
		iSequence = LookupActivity ( NewActivity );
		break;
	case ACT_WALK:
		iSequence = LookupActivity ( NewActivity );
		break;
	case ACT_IDLE:
		iSequence = LookupActivity ( NewActivity );
		break;
	default:
		iSequence = LookupActivity ( NewActivity );
		break;
	}
	
	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present
	m_IdealActivity = m_Activity;

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT ( at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
	//	pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CBlues :: GetSchedule( void )
{
	// grunts place HIGH priority on running away from danger sounds.

	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound)
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
			{
				// dangerous sound nearby!
				
				//!!!KELLY - currently, this is the grunt's signal that a grenade has landed nearby,
				// and the grunt should find cover from the blast
				// good place for "SHIT!" or some other colorful verbal indicator of dismay.
				// It's not safe to play a verbal order here "Scatter", etc cause 
				// this may only affect a single individual in a squad. 
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
			/*
			if (!HasConditions( bits_COND_SEE_ENEMY ) && ( pSound->m_iType & (bits_SOUND_PLAYER | bits_SOUND_COMBAT) ))
			{
				MakeIdealYaw( pSound->m_vecOrigin );
			}
			*/
		}
	}
	switch	( m_MonsterState )
	{
		case MONSTERSTATE_ALERT:
		case MONSTERSTATE_IDLE:
		{
			break;
		}
		case MONSTERSTATE_COMBAT:
			{
	// dead enemy
				if ( HasConditions( bits_COND_ENEMY_DEAD ) )
				{
					// call base class, all code to handle dead enemies is centralized there.
					return CBaseMonster :: GetSchedule();
				}
	// new enemy
				if ( HasConditions(bits_COND_NEW_ENEMY) )
				{
					if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
					{
						return GetScheduleOfType ( SCHED_GRUNT_SUPPRESS );
					}
					else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ))
					{
						return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
					}
					else
					{
						if(m_flNextShootTime > gpGlobals->time){
							return GetScheduleOfType ( SCHED_COMBAT_FACE );
						}
						else{
							if( m_HenemyEnemyMe == 4){
							return GetScheduleOfType ( SCHED_COMBAT_FACE );
							}
							else{
							return GetScheduleOfType ( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
							}
						}
					}
				}
	// can grenade launch
				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) )
				{
					// shoot a grenade if you can
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
	// can shoot
				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
				{
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}
	// heal
				else if ( m_fHealAll )
				{
					//!!!KELLY - this individual just realized he's out of bullet ammo. 
					// He's going to try to find cover to run to and reload, but rarely, if 
					// none is available, he'll drop and reload in the open here. 
					return GetScheduleOfType ( SCHED_GRUNT_COVER_AND_RELOAD );
				}
	// can't see enemy
				else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
				{
					return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
				}
				
				if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
				{
						if(m_flNextShootTime > gpGlobals->time){
							return GetScheduleOfType ( SCHED_COMBAT_FACE );
						}
						else{
							if( m_HenemyEnemyMe == 4){
							return GetScheduleOfType ( SCHED_COMBAT_FACE );
							}
							else{
							return GetScheduleOfType ( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
							}
						}
				}
			}
			break;
	}
	
	// no special cases here, call the base class
	return CSquadMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CBlues :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			return &slBLUETakeCover[ 0 ];
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slBLUETakeCoverFromBestSound[ 0 ];
		}
	case SCHED_GRUNT_TAKECOVER_FAILED:
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ))
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}
			else if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ))
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}	
			else{
				return GetScheduleOfType ( SCHED_FAIL );
			}
		}
		break;
	case SCHED_GRUNT_ELOF_FAIL:
		{
			if(m_groundElev){
			return &slBLUECombatFace[ 0 ];
			}
			else{
			return GetScheduleOfType ( SCHED_CHASE_ENEMY_FAILED );
			}
		}
		break;
	case SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE:
		{
			if(m_groundElev){
			return &slBLUECombatFace[ 0 ];
			}
			else{
			return &slBLUEEstablishLineOfFire[ 0 ];
			}
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			return &slBLUERangeAttack1A[ 0 ];
		}
	case SCHED_RANGE_ATTACK2:
		{
			return &slBLUERangeAttack2[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slBLUECombatFace[ 0 ];
		}
	case SCHED_GRUNT_WAIT_FACE_ENEMY:
		{
			return &slBLUEWaitInCover[ 0 ];
		}
	case SCHED_GRUNT_SWEEP:
		{
			return &slBLUESweep[ 0 ];
		}
	case SCHED_GRUNT_COVER_AND_RELOAD:
		{
			return &slBLUEHideReload[ 0 ];
		}
	case SCHED_GRUNT_FOUND_ENEMY:
		{
			return &slBLUEFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			return &slBLUEVictoryDance[ 0 ];
		}
	case SCHED_GRUNT_SUPPRESS:
		{
			return &slBLUESuppress[ 0 ];
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slBLUECombatFail[ 0 ];
			}

			return &slBLUEFail[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}