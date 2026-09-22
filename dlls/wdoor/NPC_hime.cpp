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
// monster-specific conditions
//=========================================================
#define bits_COND_GRUNT_NOFIRE	( bits_COND_SPECIAL1 )

class CStarAura : public CBaseEntity
{
public:
	void Spawn( void );

	void EXPORT Thinking( void );
};

LINK_ENTITY_TO_CLASS( star_aura, CStarAura );

void CStarAura:: Spawn( void )
{
	pev->movetype = MOVETYPE_NONE;
	pev->classname = MAKE_STRING( "star_aura" );
	
	pev->solid = SOLID_NOT;
	
	SET_MODEL(ENT(pev), "models/wind_scythe.mdl");
	pev->body = 1;

	UTIL_SetSize( pev, g_vecZero, g_vecZero );

	SetThink ( &CStarAura::Thinking );
	pev->nextthink = gpGlobals->time + 2.0;

	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0;
	pev->frame = 0;
	pev->sequence = 0;

	SetBits(pev->effects, EF_DIMLIGHT);
}

void CStarAura::Thinking( void )
{
	entvars_t *pevOwner = VARS( pev->owner );
	::RadiusDamage_limit( pev->origin, pev, pevOwner, 120, 200, CLASS_PLAYER_ALLY, DMG_ENERGYBEAM);
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "rmxp/136-Light02.wav", VOL_NORM, ATTN_NORM); 
	FX_Explosion( Center(), 44);
	SetThink ( NULL );
	UTIL_Remove( this );
	return;
}


class CStarSlayer : public CBaseEntity
{
public:
	void Spawn( void );

	void EXPORT Thinking( void );
	void Touch( CBaseEntity *pOther );
	static void Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
};

LINK_ENTITY_TO_CLASS( star_slayer, CStarSlayer );

void CStarSlayer:: Spawn( void )
{
	pev->movetype = MOVETYPE_BOUNCEMISSILE;
	pev->classname = MAKE_STRING( "star_slayer" );
	
	pev->solid = SOLID_BBOX;
	
	SET_MODEL(ENT(pev), "models/wind_scythe.mdl");
	pev->body = 3;

	UTIL_SetSize( pev, Vector( -16, -16, -4), Vector(16, 16, 4) );

	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0;
	pev->frame = 0;
	pev->sequence = 0;

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_BEAMFOLLOW );
	WRITE_SHORT( entindex() );		// entity, attachment
	WRITE_SHORT(g_sModelIndexTrail );	// model
	WRITE_BYTE( 8 ); // life
	WRITE_BYTE( 16 );  // width
	WRITE_BYTE( 255 );	// R
	WRITE_BYTE( 255 );	// G
	WRITE_BYTE( 255 );	// B
	WRITE_BYTE( 192 );	// brightness
	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

	SetThink ( &CStarSlayer::SUB_Remove );
	pev->nextthink = gpGlobals->time + 6.0;
}

void CStarSlayer::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CStarSlayer *pSpit = GetClassPtr( (CStarSlayer *)NULL );
	pSpit->Spawn();
	
	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);
	pSpit->pev->angles = UTIL_VecToAngles (pSpit->pev->velocity);
}

void CStarSlayer :: Touch ( CBaseEntity *pOther )
{
	if ( UTIL_PointContents(pev->origin) == CONTENT_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	if(pev->body == 4){
		return;
	}

	if(pOther->pev->takedamage){
		if ( pOther->pev->flags & (FL_MONSTER|FL_CLIENT) )
		{
			if (pOther->Classify() == CLASS_PLAYER 
			|| pOther->Classify() == CLASS_PLAYER_ALLY){
			//�Ѿ�����!!
			return;
			}

			entvars_t	*pevOwner;
			pevOwner = VARS( pev->owner );
			pOther->TakeDamage ( pev, pevOwner, 180, DMG_GENERIC);

			FX_Explosion( Center(), EXPLOSION_FLASHBANG );
			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hime/skill_hit.wav", 1, 0.5, 0, 100 );

			if ( (pOther->pev->flags & FL_MONSTER)  ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pOther->MyMonsterPointer();
				if(pEnemyMonster){
				pEnemyMonster->Freeze_Monster(50);
				}
			}
 
			if(pev->body == 3){
			pev->origin = pOther->Center();
			pev->velocity = g_vecZero;
			pev->movetype = MOVETYPE_NONE;
			pev->solid = SOLID_NOT;
			
			pev->body = 4;
			SetBits(pev->effects, EF_DIMLIGHT);
			SetThink ( &CStarSlayer::Thinking );
			pev->nextthink = gpGlobals->time + 4.0;
			}
		}
	}
	
}

void CStarSlayer::Thinking( void )
{
	SetThink ( NULL );
	UTIL_Remove( this );
	return;
}


class CStarArrow : public CBaseEntity
{
public:
	void Spawn( void );

	static void Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	void Touch( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( star_arrow, CStarArrow );

void CStarArrow:: Spawn( void )
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING( "star_arrow" );
	
	pev->solid = SOLID_BBOX;
	
	SET_MODEL(ENT(pev), "models/wind_scythe.mdl");
	pev->body = 2;
	pev->frame = 0;
	pev->framerate = 1.0;

//	pev->effects		= EF_DIMLIGHT;

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_BEAMFOLLOW );
	WRITE_SHORT( entindex() );		// entity, attachment
	WRITE_SHORT(g_sModelIndexTrail );	// model
	WRITE_BYTE( 4 ); // life
	WRITE_BYTE( 4 );  // width
	WRITE_BYTE( 255 );	// R
	WRITE_BYTE( 255 );	// G
	WRITE_BYTE( 128 );	// B
	WRITE_BYTE( 192 );	// brightness
	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

	UTIL_SetSize( pev, Vector( -1, -1, 1), Vector(1, 1, 1) );
}

void CStarArrow::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CStarArrow *pSpit = GetClassPtr( (CStarArrow *)NULL );
	pSpit->Spawn();
	
	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);
	pSpit->pev->angles = UTIL_VecToAngles (pSpit->pev->velocity);
}

void CStarArrow :: Touch ( CBaseEntity *pOther )
{
	if ( UTIL_PointContents(pev->origin) == CONTENT_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hime/wb_hit.wav", 1, ATTN_NORM, 0, 100 );	

	TraceResult tr = UTIL_GetGlobalTrace( );
	if (tr.pHit == pOther->edict())//ֱ������!
	{
		Vector vecSrc = Center();
		Vector vecEnd	= vecSrc + pev->velocity * 10;
		int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
		int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,0,CLASS_PLAYER);
		FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

		UTIL_Sparks( tr.vecEndPos );

		ClearMultiDamage( );
		pOther->TraceAttack(pev, 60, gpGlobals->v_forward, &tr, DMG_ENERGYBEAM); 

		entvars_t	*pevOwner;
		pevOwner = VARS( pev->owner );
		ApplyMultiDamage( pev, pevOwner );
	}

	FX_Explosion( pev->origin, 49);

	SetThink ( &CStarArrow::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

class CHime : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed ( void );
	int  Classify ( void );
	int ISoundMask ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL FCanCheckAttacks ( void );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );

	void RunAI( void );
	void Killed( entvars_t *pevAttacker, int iGib );

	void SetActivity ( Activity NewActivity );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	void DeathSound( void );
	void PainSound( void );
	void Shoot ( int post );

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
	float m_flNextPainTime;
	float m_flNextMeleeTime;
	float m_flLastEnemySightTime;

	float m_flNextFireTime;
	float m_flNextFire2Time;
	float m_flNextFire3Time;
	float m_flNextFire4Time;
	float m_flNextFire5Time;
	float m_flNextJumpTime;

	BOOL	m_fThrowGrenade;
	BOOL	m_fStanding;
	BOOL	m_fFirstEncounter;// only put on the handsign show in the squad's first encounter.

	float	m_dyingtime;

	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;
	BOOL	m_fHealPlayer;
	BOOL	m_fHealPlayer2;
	BOOL	m_fHealMyself;
};

LINK_ENTITY_TO_CLASS( monster_hime, CHime );

void CHime :: FollowerUse2( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( IsAlive() && pCaller != NULL && pCaller->IsPlayer() )
	{
			if ( m_MonsterState == MONSTERSTATE_SCRIPT || m_IdealMonsterState == MONSTERSTATE_SCRIPT )
			{
				if(!m_pCine->CanInterrupt()){
					return;
				}
			}

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

TYPEDESCRIPTION	CHime::m_SaveData[] = 
{
	DEFINE_FIELD( CHime, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CHime, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHime, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHime, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CHime, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHime, m_fHealPlayer, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHime, m_fHealPlayer2, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHime, m_fHealMyself, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHime, m_dyingtime, FIELD_TIME ),
	DEFINE_FIELD( CHime, m_flNextFireTime, FIELD_TIME ),
	DEFINE_FIELD( CHime, m_flNextFire2Time, FIELD_TIME ),
	DEFINE_FIELD( CHime, m_flNextFire3Time, FIELD_TIME ),
	DEFINE_FIELD( CHime, m_flNextFire4Time, FIELD_TIME ),
	DEFINE_FIELD( CHime, m_flNextFire5Time, FIELD_TIME ),
	DEFINE_FIELD( CHime, m_flNextJumpTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CHime, CSquadMonster );

//=========================================================
// IRelationship - overridden because Alien Grunts are 
// Human Grunt's nemesis.
//=========================================================
int CHime::IRelationship ( CBaseEntity *pTarget )
{
	return CSquadMonster::IRelationship( pTarget );
}

//=========================================================
// RunAI
//=========================================================
void CHime :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(pev->sequence == LookupActivity ( ACT_RUN )){
		m_flGroundSpeed = 300;
	}
	else if(pev->sequence == LookupActivity ( ACT_WALK )){
		m_flGroundSpeed = 120;
	}

	if ( m_MonsterState == MONSTERSTATE_PRONE || m_IdealMonsterState == MONSTERSTATE_PRONE ){
		m_dyingtime++;
		if(m_dyingtime > 40){//��������
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

	if(pev->skin == 0){
		if(m_rpgms_skill6_learn == 67 && pev->health < pev->max_health){//����
		TakeHealth(1, DMG_GENERIC);
		}

		if(m_hPlayer != NULL && m_flNextFire3Time <= gpGlobals->time 
		&& pev->sequence != LookupActivity ( ACT_RANGE_ATTACK1 )
		&& pev->sequence != LookupActivity ( ACT_HOP )
		&& pev->sequence != LookupActivity ( ACT_LEAP )
		&& !m_fHealPlayer2 && !m_fHealPlayer && m_rpgms_skill5_learn == 37){//ȫ��ʥ��
				m_PlayerHealth = m_hPlayer->pev->health;
				if( (m_PlayerHealth < m_hPlayer->pev->max_health * 0.8)
				|| (pev->health < pev->max_health * 0.8) ){//��һ��Լ������ˣ���Ҫ����!
					m_fHealPlayer = TRUE;
					m_igonre_npc = 10;
				}
				else if(m_hTeamMate1 != NULL && m_hTeamMate1->pev->deadflag == DEAD_NO 
					&& m_hTeamMate1->pev->health < m_hTeamMate1->pev->max_health * 0.8){
					m_fHealPlayer = TRUE;
					m_igonre_npc = 10;
				}
				else if(m_hTeamMate2 != NULL && m_hTeamMate2->pev->deadflag == DEAD_NO 
					&& m_hTeamMate2->pev->health < m_hTeamMate2->pev->max_health * 0.8){
					m_fHealPlayer = TRUE;
					m_igonre_npc = 10;
				}
				else if(m_hTeamMate3 != NULL && m_hTeamMate3->pev->deadflag == DEAD_NO 
					&& m_hTeamMate3->pev->health < m_hTeamMate3->pev->max_health * 0.8){
					m_fHealPlayer = TRUE;
					m_igonre_npc = 10;
				}
				else if(m_hTeamMate4 != NULL && m_hTeamMate4->pev->deadflag == DEAD_NO 
					&& m_hTeamMate4->pev->health < m_hTeamMate4->pev->max_health * 0.8){
					m_fHealPlayer = TRUE;
					m_igonre_npc = 10;
				}
		}
		else if(m_hPlayer != NULL && m_flNextFire4Time <= gpGlobals->time 
		&& pev->sequence != LookupActivity ( ACT_RANGE_ATTACK1 )
		&& pev->sequence != LookupActivity ( ACT_HOP )
		&& pev->sequence != LookupActivity ( ACT_LEAP )
		&& !m_fHealPlayer2 && !m_fHealPlayer){//ʥ����
				m_PlayerHealth = m_hPlayer->pev->health;
				if(pev->health < pev->max_health * 0.6){//����!��Σ����
					m_fHealPlayer2 = TRUE;
					m_fHealMyself = TRUE;
					m_igonre_npc = 10;
				}
				else if( m_hPlayer->pev->deadflag == DEAD_NO && 
					m_PlayerHealth < m_hPlayer->pev->max_health * 0.8){
					m_fHealPlayer2 = TRUE;
					m_igonre_npc = 10;
				}
				else if(m_hTeamMate1 != NULL && m_hTeamMate1->pev->deadflag == DEAD_NO 
					&& m_hTeamMate1->pev->health < m_hTeamMate1->pev->max_health * 0.8){
					m_fHealPlayer2 = TRUE;
					m_igonre_npc = 10;
				}
				else if(m_hTeamMate2 != NULL && m_hTeamMate2->pev->deadflag == DEAD_NO 
					&& m_hTeamMate2->pev->health < m_hTeamMate2->pev->max_health * 0.8){
					m_fHealPlayer2 = TRUE;
					m_igonre_npc = 10;
				}
				else if(m_hTeamMate3 != NULL && m_hTeamMate3->pev->deadflag == DEAD_NO 
					&& m_hTeamMate3->pev->health < m_hTeamMate3->pev->max_health * 0.8){
					m_fHealPlayer2 = TRUE;
					m_igonre_npc = 10;
				}
				else if(m_hTeamMate4 != NULL && m_hTeamMate4->pev->deadflag == DEAD_NO 
					&& m_hTeamMate4->pev->health < m_hTeamMate4->pev->max_health * 0.8){
					m_fHealPlayer2 = TRUE;
					m_igonre_npc = 10;
				}
				else if(pev->health < pev->max_health * 0.8){//����!
					m_fHealPlayer2 = TRUE;
					m_fHealMyself = TRUE;
					m_igonre_npc = 10;
				}
		}
	}
	
	if(pev->skin == 1 && pev->deadflag == DEAD_DEAD
	&& pev->sequence != LookupActivity (ACT_DIESIMPLE) ){//վ����ȥ�� Bug Fix 1.0
	SetActivity ( ACT_DIESIMPLE );
	}

	//���ܤν���
	if(m_rpgms_level >= 60 && m_rpgms_skill6_learn == 0){
		m_rpgms_skill5_learn = 37;
		m_rpgms_skill6_learn = 67;
		m_rpgms_skill7_learn = 76;
	}
	if(m_rpgms_level >= 45 && m_rpgms_skill5_learn == 0){
		m_rpgms_skill5_learn = 37;
	}
}

void CHime::Killed( entvars_t *pevAttacker, int iGib )
{
	pev->skin = 1;
	m_fHealPlayer = FALSE;
	m_fHealPlayer2 = FALSE;
	m_fHealMyself = FALSE;

	CSquadMonster::Killed( pevAttacker, GIB_NEVER );
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CHime :: GibMonster ( void )
{
	CBaseMonster :: GibMonster();
}

//=========================================================
// ISoundMask - Overidden for human grunts because they 
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CHime :: ISoundMask ( void )
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
BOOL CHime :: FCanCheckAttacks ( void )
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
// CheckRangeAttack1 - overridden for HGrunt, cause 
// FCanCheckAttacks() doesn't disqualify all attacks based
// on whether or not the enemy is occluded because unlike
// the base class, the HGrunt can attack when the enemy is
// occluded (throw grenade over wall, etc). We must 
// disqualify the machine gun attack if the enemy is occluded.
//=========================================================
BOOL CHime :: CheckRangeAttack1 ( float flDot, float flDist )
{
	float dist = 2048;
	if(m_flNextFireTime > gpGlobals->time){
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

BOOL CHime :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	float cover_dist = 256;

	CBaseMonster *pEnemy;

	if ( m_hEnemy != NULL )
	{
		pEnemy = m_hEnemy->MyMonsterPointer();

		if ( !pEnemy )
		{
			return FALSE;
		}

		cover_dist += m_hEnemy->pev->maxs.x;
	}

	if(pev->sequence == LookupActivity ( ACT_HOP ) 
	|| pev->sequence == LookupActivity ( ACT_LEAP ) 
	|| m_cleardally_enemy == 0){
		return FALSE;
	}

	if(flDist <= cover_dist && flDot >= 0.8 && m_flNextFire2Time <= gpGlobals->time ){
			if(pev->sequence != LookupActivity ( ACT_LEAP )){
			SetActivity ( ACT_LEAP );
			}
	}


	return FALSE;
}

//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
void CHime :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
int CHime :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if(pev->sequence == LookupActivity ( ACT_USE ) || pev->sequence == LookupActivity ( ACT_TWITCH )){
	flDamage *= 0.5;//���Ƽ����ͷ��У��˺����٣�
	}

	//ħ������
	if ( (bitsDamageType & DMG_SHOCK) || (bitsDamageType & DMG_ENERGYBLAST) 
	|| (bitsDamageType & DMG_FREEZE) || (bitsDamageType & DMG_BURN)){
		flDamage *= 0.3;
	}
	else if ( (bitsDamageType & DMG_ENERGYBEAM)
	|| (bitsDamageType & DMG_DARK)
	|| (bitsDamageType & DMG_SONIC)){
		flDamage *= 0.5;
	}


	m_alert	= 100;

	Forget( bits_MEMORY_INCOVER );

	return CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHime :: SetYawSpeed ( void )
{
	pev->yaw_speed = 240;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CHime :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHime :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case 1:
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

		case 2:
		{
			if(m_flNextFireTime <= gpGlobals->time && m_hEnemy != NULL){
			Vector	vecSpitOffset,vangle;
			Vector	vecSpitDir;

			UTIL_MakeVectors ( pev->angles );

			// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
			// we should be able to read the position of bones at runtime for this info.
			GetAttachment( 0, vecSpitOffset, vangle );

			vecSpitDir = ( m_hEnemy->Center() - vecSpitOffset ).Normalize();

			// do stuff for this event.
			//AttackSound();

			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hime/wb_shoot.wav", 0.8, ATTN_NORM,0,100 + RANDOM_LONG(-5,5) );

			if(m_rpgms_skill7_learn == 76 && m_flNextFire5Time <= gpGlobals->time){
			CStarSlayer::Shoot( pev, vecSpitOffset, vecSpitDir * 2000 );
			m_flNextFire5Time = gpGlobals->time + 30;
			}
			else{
			CStarArrow::Shoot( pev, vecSpitOffset, vecSpitDir * 3000 );
			}
			m_flNextFireTime = gpGlobals->time + 2;

			}
			else{
				if(m_hEnemy == NULL || (m_hEnemy->IsPlayer() && m_lovehate > 0) ){
				ClearSchedule();
				SetYawSpeed();
				}
			}
		}
		break;

		case 3://ȫ��ʥ��
		{
			if (m_flNextFire3Time < gpGlobals->time)
			{
				if(m_hPlayer != NULL && m_hPlayer->pev->deadflag == DEAD_NO){
				m_hPlayer->TakeHealth(m_hPlayer->pev->max_health * 0.25, DMG_GENERIC);
				FX_Explosion( m_hPlayer->Center(), 45);
				}
				if(m_hTeamMate1 != NULL && m_hTeamMate1->pev->deadflag == DEAD_NO){
				m_hTeamMate1->TakeHealth(m_hTeamMate1->pev->max_health * 0.25, DMG_GENERIC);
				FX_Explosion( m_hTeamMate1->Center(), 45);
				}
				if(m_hTeamMate2 != NULL && m_hTeamMate2->pev->deadflag == DEAD_NO){
				m_hTeamMate2->TakeHealth(m_hTeamMate2->pev->max_health * 0.25, DMG_GENERIC);
				FX_Explosion( m_hTeamMate2->Center(), 45);
				}
				if(m_hTeamMate3 != NULL && m_hTeamMate3->pev->deadflag == DEAD_NO){
				m_hTeamMate3->TakeHealth(m_hTeamMate3->pev->max_health * 0.25, DMG_GENERIC);
				FX_Explosion( m_hTeamMate3->Center(), 45);
				}
				if(m_hTeamMate4 != NULL && m_hTeamMate4->pev->deadflag == DEAD_NO){
				m_hTeamMate4->TakeHealth(m_hTeamMate4->pev->max_health * 0.25, DMG_GENERIC);
				FX_Explosion( m_hTeamMate4->Center(), 45);
				}
				FX_Explosion( Center(), 45);
				TakeHealth(pev->max_health * 0.25, DMG_GENERIC);//����˫����Ѫ
			
				/*
				MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_LARGEFUNNEL );
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );
				WRITE_SHORT( g_sModelIndexFlareGlow );
				WRITE_SHORT( 1 );
				MESSAGE_END();
				*/

				m_flNextFire3Time = gpGlobals->time + 45;
				m_fHealPlayer = FALSE;
				m_fHealPlayer2 = FALSE;
				m_fHealMyself = FALSE;
			}
		}
		break;

		case 4://��Ծ������
		{
			if (m_flNextJumpTime < gpGlobals->time && FBitSet (pev->flags, FL_ONGROUND ))
			{
				pev->flags &= ~FL_ONGROUND;
				UTIL_MakeVectors(pev->angles);
				pev->velocity = gpGlobals->v_forward * -512;
				pev->velocity.z += 128;

				m_flNextJumpTime = gpGlobals->time + 4;

				Vector ice_org;
				ice_org = pev->origin + gpGlobals->v_forward * 32;
				if(m_hEnemy != NULL){
					if(( pev->origin - m_hEnemy->pev->origin).Length() <= 384){
					ice_org = m_hEnemy->EyePosition();
					CBaseEntity::Create( "star_aura", ice_org, pev->angles, edict() );

					m_hEnemy->TakeDamage( pev, pev, 5, DMG_ENERGYBEAM | DMG_CONCUSSION);

					m_flNextFire2Time = gpGlobals->time + 4;
					}
				}
			}
		}
		break;

		case 6:
		{
			m_cAmmoLoaded = 114;
			pev->body = 1;
		}
		break;

		case 7:
		{
			m_flNextFire2Time = gpGlobals->time + 9;
			pev->body = 0;
			ClearSchedule();
			SetYawSpeed();
		}
		break;

		case 8://ʥ������Զ������
		{
			if (m_flNextFire4Time < gpGlobals->time)
			{
				Vector	vecSpitOffset,vangle;
				GetAttachment( 0, vecSpitOffset, vangle );

				m_flNextFire4Time = gpGlobals->time + 10;

				if(m_fHealMyself){
				TakeHealth(pev->max_health * 0.5, DMG_GENERIC);
				FX_Explosion(Center(), 45);
				m_flNextFire4Time = gpGlobals->time + 30;
				FX_Explosion( vecSpitOffset, 101);
				}
				else if(m_hPlayer != NULL && m_hPlayer->pev->deadflag == DEAD_NO
				&& m_hPlayer->pev->health < m_hPlayer->pev->max_health * 0.8){
				m_hPlayer->TakeHealth(m_hPlayer->pev->max_health * 0.25, DMG_GENERIC);
				FX_Explosion(m_hPlayer->Center(), 45);
				m_flNextFire4Time = gpGlobals->time + 30;
				FX_Explosion( vecSpitOffset, 101);
				}
				else if(m_hTeamMate1 != NULL && m_hTeamMate1->pev->deadflag == DEAD_NO
				&& m_hTeamMate1->pev->health < m_hTeamMate1->pev->max_health * 0.8){
				m_hTeamMate1->TakeHealth(m_hTeamMate1->pev->max_health * 0.25, DMG_GENERIC);
				FX_Explosion( m_hTeamMate1->Center(), 45);
				m_flNextFire4Time = gpGlobals->time + 30;
				FX_Explosion( vecSpitOffset, 101);
				}
				else if(m_hTeamMate2 != NULL && m_hTeamMate2->pev->deadflag == DEAD_NO
				&& m_hTeamMate2->pev->health < m_hTeamMate2->pev->max_health * 0.8){
				m_hTeamMate2->TakeHealth(m_hTeamMate2->pev->max_health * 0.25, DMG_GENERIC);
				FX_Explosion( m_hTeamMate2->Center(), 45);
				m_flNextFire4Time = gpGlobals->time + 30;
				FX_Explosion( vecSpitOffset, 101);
				}
				else if(m_hTeamMate3 != NULL && m_hTeamMate3->pev->deadflag == DEAD_NO
				&& m_hTeamMate3->pev->health < m_hTeamMate3->pev->max_health * 0.8){
				m_hTeamMate3->TakeHealth(m_hTeamMate3->pev->max_health * 0.25, DMG_GENERIC);
				FX_Explosion( m_hTeamMate3->Center(), 45);
				m_flNextFire4Time = gpGlobals->time + 30;
				FX_Explosion( vecSpitOffset, 101);
				}
				else if(m_hTeamMate4 != NULL && m_hTeamMate4->pev->deadflag == DEAD_NO
				&& m_hTeamMate4->pev->health < m_hTeamMate4->pev->max_health * 0.8){
				m_hTeamMate4->TakeHealth(m_hTeamMate4->pev->max_health * 0.25, DMG_GENERIC);
				FX_Explosion( m_hTeamMate4->Center(), 45);
				m_flNextFire4Time = gpGlobals->time + 30;
				FX_Explosion( vecSpitOffset, 101);
				}
				m_fHealPlayer = FALSE;
				m_fHealPlayer2 = FALSE;
				m_fHealMyself = FALSE;
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
void CHime :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/hime.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;

	pev->health			= 240;
	m_lovehate			= 90;

	m_flFieldOfView		 = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		 = MONSTERSTATE_NONE;

	m_flNextFireTime	 = gpGlobals->time + 2;//���
	m_flNextFire2Time	 = gpGlobals->time + 2;//����
	m_flNextFire3Time	 = gpGlobals->time + 2;//ʥ����
	m_flNextFire4Time	 = gpGlobals->time + 2;//ȫ��ʥ��
	m_flNextFire5Time	 = gpGlobals->time + 2;//��֮����

	m_afCapability		= bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 48 );

	m_cClipSize			= 1;
	m_cAmmoLoaded		= 1;
	m_canheadcrab_mode  = 0;
	m_canbarnacle_mode  = 1;

	m_no_victdance		= 1;

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();

	m_follow_mode		= 1;
	m_headdef			= 2;

	m_fHealPlayer = FALSE;
	m_fHealPlayer2 = FALSE;

	m_candrownwater = 1;
//	m_forcefuckdoor = TRUE;
	m_chase_mode = 1;
	m_chase_failed_max = 4;

	SetUse( &CHime::FollowerUse2 );

	m_aimenemy_mod = 6;

	pev->body = 0;
	m_fStanding = TRUE;
	SetTouch( &CHime::DeadTouch );

	m_rpgms_actor = 15;
	m_rpgms_level = 30;
	m_rpgms_exp = 0;
	m_rpgms_type = 1;

	m_new_ally_type = TRUE;
	pev->netname = MAKE_STRING( "Hime" );

	m_rpgms_skill1_learn = 35;//���
	m_rpgms_skill2_learn = 36;//����
	m_rpgms_skill3_learn = 48;//ʥ����
	m_rpgms_skill4_learn = 38;//ħ������
	m_rpgms_skill5_learn = 0;//ȫ��ʥ��37
	m_rpgms_skill6_learn = 0;//����67
	m_rpgms_skill7_learn = 0;//��֮����76

	pev->takedamage = DAMAGE_YES;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHime :: Precache()
{
	PRECACHE_MODEL("models/hime.mdl");
	PRECACHE_MODEL("models/wind_scythe.mdl");

	PRECACHE_SOUND("hime/pain1.wav");
	PRECACHE_SOUND("hime/die1.wav");
	PRECACHE_SOUND("hime/wb_shoot.wav");
	PRECACHE_SOUND("hime/wb_hit.wav");
	PRECACHE_SOUND("hime/skill_hit.wav");

	PRECACHE_SOUND("rmxp/136-Light02.wav");
	PRECACHE_SOUND("rmxp/111-Heal07.wav");

	PRECACHE_SOUND("weapons/m249_fire.wav");// because we use the basemonster SWIPE animation event
	PRECACHE_SOUND ("tank/tank_fire.wav");
}	

//=========================================================
// start task
//=========================================================
void CHime :: StartTask ( Task_t *pTask )
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
		if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;

	default: 
		CSquadMonster :: StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CHime :: RunTask ( Task_t *pTask )
{
	CSquadMonster :: RunTask( pTask );
}

void CHime :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{
		EMIT_SOUND_DYN( ENT(pev), 6, "hime/pain1.wav", 1, 0.7, 0, 100);

		m_flNextPainTime = gpGlobals->time + 1;
	}
}


//=========================================================
// DeathSound 
//=========================================================
void CHime :: DeathSound ( void )
{
	EMIT_SOUND_DYN( ENT(pev), 6, "hime/die1.wav", 1, 0.7, 0, 100);
}

//=========================================================
// GruntFail
//=========================================================
Task_t	tlHimeFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)1		},
	{ TASK_FORGET_ENEMY,		(float)0	},
};

Schedule_t	slHimeFail[] =
{
	{
		tlHimeFail,
		ARRAYSIZE ( tlHimeFail ),
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
Task_t	tlHimeCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)1		},
};

Schedule_t	slHimeCombatFail[] =
{
	{
		tlHimeCombatFail,
		ARRAYSIZE ( tlHimeCombatFail ),
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
Task_t	tlHimeVictoryDance[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_WAIT,							(float)1.5					},
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,		(float)0					},
	{ TASK_WALK_PATH,						(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
};

Schedule_t	slHimeVictoryDance[] =
{
	{ 
		tlHimeVictoryDance,
		ARRAYSIZE ( tlHimeVictoryDance ), 
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
Task_t tlHimeEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_GRUNT_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slHimeEstablishLineOfFire[] =
{
	{ 
		tlHimeEstablishLineOfFire,
		ARRAYSIZE ( tlHimeEstablishLineOfFire ),
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
Task_t	tlHimeFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slHimeFoundEnemy[] =
{
	{ 
		tlHimeFoundEnemy,
		ARRAYSIZE ( tlHimeFoundEnemy ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"GruntFoundEnemy"
	},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t	tlHimeCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_COMBAT_IDLE		},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)0.5					},
};

Schedule_t	slHimeCombatFace[] =
{
	{ 
		tlHimeCombatFace1,
		ARRAYSIZE ( tlHimeCombatFace1 ), 
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_RANGE_ATTACK2		|
		bits_COND_CAN_MELEE_ATTACK1		|
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"Combat Face"
	},
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or grunt gets hurt.
//=========================================================
Task_t	tlHimeSignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
};

Schedule_t	slHimeSignalSuppress[] =
{
	{ 
		tlHimeSignalSuppress,
		ARRAYSIZE ( tlHimeSignalSuppress ), 
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

Task_t	tlHimeSuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slHimeSuppress[] =
{
	{ 
		tlHimeSuppress,
		ARRAYSIZE ( tlHimeSuppress ), 
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
Task_t	tlHimeWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slHimeWaitInCover[] =
{
	{ 
		tlHimeWaitInCover,
		ARRAYSIZE ( tlHimeWaitInCover ), 
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
Task_t	tlHimeTakeCover1[] =
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

Schedule_t	slHimeTakeCover[] =
{
	{ 
		tlHimeTakeCover1,
		ARRAYSIZE ( tlHimeTakeCover1 ), 
		bits_COND_ENEMY_DEAD,
		0,
		"TakeCover"
	},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlHimeGrenadeCover1[] =
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

Schedule_t	slHimeGrenadeCover[] =
{
	{ 
		tlHimeGrenadeCover1,
		ARRAYSIZE ( tlHimeGrenadeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlHimeTossGrenadeCover1[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slHimeTossGrenadeCover[] =
{
	{ 
		tlHimeTossGrenadeCover1,
		ARRAYSIZE ( tlHimeTossGrenadeCover1 ), 
		0,
		0,
		"TossGrenadeCover"
	},
};
//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlHimeTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slHimeTakeCoverFromBestSound[] =
{
	{ 
		tlHimeTakeCoverFromBestSound,
		ARRAYSIZE ( tlHimeTakeCoverFromBestSound ), 
		0,
		0,
		"TakeCoverFromBestSound"
	},
};


//=========================================================
// Grunt reload schedule
//=========================================================
Task_t	tlHimeHideReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_USE				},
};

Schedule_t slHimeHideReload[] = 
{
	{
		tlHimeHideReload,
		ARRAYSIZE ( tlHimeHideReload ),
		0,
		0,
		"HimeHeal"
	}
};

Task_t	tlHimeHideReload2[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_TWITCH			},
};

Schedule_t slHimeHideReload2[] = 
{
	{
		tlHimeHideReload2,
		ARRAYSIZE ( tlHimeHideReload2 ),
		0,
		0,
		"HimeHeal2"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlHimeSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_FORGET_ENEMY,		(float)0	},
};

Schedule_t	slHimeSweep[] =
{
	{ 
		tlHimeSweep,
		ARRAYSIZE ( tlHimeSweep ), 
		
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
Task_t	tlHimeRangeAttack1A[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
};

Schedule_t	slHimeRangeAttack1A[] =
{
	{ 
		tlHimeRangeAttack1A,
		ARRAYSIZE ( tlHimeRangeAttack1A ), 
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
Task_t	tlHimeRangeAttack1B[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
};

Schedule_t	slHimeRangeAttack1B[] =
{
	{ 
		tlHimeRangeAttack1B,
		ARRAYSIZE ( tlHimeRangeAttack1B ), 
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


DEFINE_CUSTOM_SCHEDULES( CHime )
{
	slHimeFail,
	slHimeCombatFail,
	slHimeVictoryDance,
	slHimeEstablishLineOfFire,
	slHimeFoundEnemy,
	slHimeCombatFace,
	slHimeSignalSuppress,
	slHimeSuppress,
	slHimeWaitInCover,
	slHimeTakeCover,
	slHimeGrenadeCover,
	slHimeTossGrenadeCover,
	slHimeTakeCoverFromBestSound,
	slHimeHideReload,
	slHimeHideReload2,
	slHimeSweep,
	slHimeRangeAttack1A,
	slHimeRangeAttack1B,
};

IMPLEMENT_CUSTOM_SCHEDULES( CHime, CSquadMonster );

//=========================================================
// SetActivity 
//=========================================================
void CHime :: SetActivity ( Activity NewActivity )
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
Schedule_t *CHime :: GetSchedule( void )
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
		case MONSTERSTATE_COMBAT:
			{
	// dead enemy
				if ( HasConditions( bits_COND_ENEMY_DEAD ) )
				{
					// call base class, all code to handle dead enemies is centralized there.
					return CBaseMonster :: GetSchedule();
				}
	// new enemy	
				if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
				{
					//���˶���
					return GetScheduleOfType( SCHED_SMALL_FLINCH );
				}
				else if ( m_fHealPlayer || m_fHealPlayer2 )
				{
					//!!!KELLY - this individual just realized he's out of bullet ammo. 
					// He's going to try to find cover to run to and reload, but rarely, if 
					// none is available, he'll drop and reload in the open here. 
					return GetScheduleOfType ( SCHED_GRUNT_COVER_AND_RELOAD );
				}
				else if ( HasConditions(bits_COND_NEW_ENEMY) )
				{
					if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
					{
						return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
					}
					else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
					{
						return GetScheduleOfType ( SCHED_GRUNT_SUPPRESS );
					}
					else
					{
						if(m_flNextFireTime > gpGlobals->time){
							if( m_HenemyEnemyMe == 4){
							return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
							}
							else{
							return GetScheduleOfType ( SCHED_COMBAT_FACE );
							}
						}
						else{
							if( m_HenemyEnemyMe == 4){
							return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
							}
							else{
								if(m_hEnemy != NULL){
									if(FClassnameIs(m_hEnemy->pev,"player_aim_flag") 
									|| m_hEnemy->IsPlayer()){
									return GetScheduleOfType ( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
									}
								}
								return GetScheduleOfType ( SCHED_COMBAT_FACE );
							}
						}
					}
				}
	// can kick
				else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
				{
					return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
				}
	// can shoot
				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
				{
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}
	// can't see enemy
				else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
				{
					return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
				}
				
				if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
				{
						if(m_flNextFireTime > gpGlobals->time){
							if( m_HenemyEnemyMe == 4){
							return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
							}
							else{
							return GetScheduleOfType ( SCHED_COMBAT_FACE );
							}
						}
						else{
								if(m_hEnemy != NULL){
									if(FClassnameIs(m_hEnemy->pev,"player_aim_flag") 
									|| m_hEnemy->IsPlayer()){
									return GetScheduleOfType ( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
									}
								}

								if( m_HenemyEnemyMe == 4){
								return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
								}

								return GetScheduleOfType ( SCHED_COMBAT_FACE );
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
Schedule_t* CHime :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			return &slHimeTakeCover[ 0 ];
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slHimeTakeCoverFromBestSound[ 0 ];
		}
	case SCHED_GRUNT_TAKECOVER_FAILED:
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}
			else if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}
			else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK2 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK2 );
			}
			else{
				return GetScheduleOfType ( SCHED_FAIL );
			}
		}
		break;
	case SCHED_GRUNT_ELOF_FAIL:
		{
			if(m_groundElev){
			return &slHimeCombatFace[ 0 ];
			}
			else{
			return GetScheduleOfType ( SCHED_CHASE_ENEMY_FAILED );
			}
		}
		break;
	case SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE:
		{
			if(m_groundElev){
			return &slHimeCombatFace[ 0 ];
			}
			else{
			return &slHimeEstablishLineOfFire[ 0 ];
			}
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			return &slHimeRangeAttack1A[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slHimeCombatFace[ 0 ];
		}
	case SCHED_GRUNT_WAIT_FACE_ENEMY:
		{
			return &slHimeWaitInCover[ 0 ];
		}
	case SCHED_GRUNT_SWEEP:
		{
			return &slHimeSweep[ 0 ];
		}
	case SCHED_GRUNT_COVER_AND_RELOAD:
		{
			if(m_fHealPlayer){
			return &slHimeHideReload[ 0 ];
			}
			else{
			return &slHimeHideReload2[ 0 ];
			}
		}
	case SCHED_GRUNT_FOUND_ENEMY:
		{
			return &slHimeFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			return &slHimeVictoryDance[ 0 ];
		}
	case SCHED_GRUNT_SUPPRESS:
		{
			return &slHimeSuppress[ 0 ];
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slHimeCombatFail[ 0 ];
			}

			return &slHimeFail[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}