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

class CWindScythe : public CBaseEntity
{
public:
	void Spawn( void );

	static void Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	void Touch( CBaseEntity *pOther );
	void EXPORT Remove_Thinking( void );
};

LINK_ENTITY_TO_CLASS( wind_scythe, CWindScythe );

void CWindScythe:: Spawn( void )
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING( "wind_scythe" );
	
	pev->solid = SOLID_BBOX;
	
	SET_MODEL(ENT(pev), "models/wind_scythe.mdl");
	pev->body = 0;
	pev->frame = 0;
	pev->framerate = 1.0;

//	pev->effects		= EF_DIMLIGHT;

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_BEAMFOLLOW );
	WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
	WRITE_SHORT(g_sModelIndexTrail );	// model
	WRITE_BYTE( 6 ); // life
	WRITE_BYTE( 4 );  // width
	WRITE_BYTE( 128 );	// R
	WRITE_BYTE( 255 );	// G
	WRITE_BYTE( 192 );	// B
	WRITE_BYTE( 192 );	// brightness
	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_BEAMFOLLOW );
	WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
	WRITE_SHORT(g_sModelIndexTrail );	// model
	WRITE_BYTE( 6 ); // life
	WRITE_BYTE( 4 );  // width
	WRITE_BYTE( 128 );	// R
	WRITE_BYTE( 255 );	// G
	WRITE_BYTE( 192 );	// B
	WRITE_BYTE( 192 );	// brightness
	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)


	UTIL_SetSize( pev, Vector( -1, -1, 1), Vector(1, 1, 1) );
}

void CWindScythe::Remove_Thinking( void )
{
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_KILLBEAM );
	WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_KILLBEAM );
	WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
	MESSAGE_END();

	UTIL_Remove( this );
	return;
}

void CWindScythe::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CWindScythe *pSpit = GetClassPtr( (CWindScythe *)NULL );
	pSpit->Spawn();
	
	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);
	pSpit->pev->angles = UTIL_VecToAngles (pSpit->pev->velocity);
}

void CWindScythe :: Touch ( CBaseEntity *pOther )
{
	if(pev->frags == 1){
	return;
	}

	if ( UTIL_PointContents(pev->origin) == CONTENT_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	// splat sound
	int iPitch = RANDOM_FLOAT( 90, 110 );	

	TraceResult tr = UTIL_GetGlobalTrace( );
	if (tr.pHit == pOther->edict())//ֱ������!
	{
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "dragon/wind_slash.wav", 1, ATTN_NORM, 0, iPitch );
		
		Vector vecSrc = Center();
		Vector vecEnd	= vecSrc + pev->velocity * 10;
		int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
		int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,0,CLASS_PLAYER);
		FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

		ClearMultiDamage( );
		pOther->TraceAttack(pev, 180, gpGlobals->v_forward, &tr, DMG_SLASH); 

		if(!FNullEnt(pev->owner)){
		entvars_t	*pevOwner;
		pevOwner = VARS( pev->owner );
		ApplyMultiDamage( pev, pevOwner );
		}
		else{
		ApplyMultiDamage( pev, pev );
		}

		if ( pOther->pev->flags & (FL_MONSTER|FL_CLIENT) )
		{
			if(pOther->pev->gravity <= 1.5){//����Ч��
			pOther->pev->velocity = (pOther->pev->origin - pev->origin).Normalize() + pev->velocity * 0.1;
			}
		}

	}

	pev->solid = SOLID_NOT;
	FX_Explosion( pev->origin, 43);
	pev->frags = 1;
	SetThink ( &CWindScythe::Remove_Thinking );
	pev->nextthink = gpGlobals->time + 0.5;
}


class CDragon : public CSquadMonster
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
	BOOL CheckMeleeAttack2 ( float flDot, float flDist );

	void RunAI( void );
	void Killed( entvars_t *pevAttacker, int iGib );

	void SetActivity ( Activity NewActivity );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	void DeathSound( void );
	void IdleSound ( void );

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
	float m_dyingtime;
	float m_flNextPainTime;
	float m_flNextMeleeTime;
	float m_flLastEnemySightTime;

	float m_flNextFireTime;
	float m_flNextFire2Time;
	float m_flNextFire3Time;
	float m_flNextFire4Time;
	float m_flyingtime;

	BOOL	m_fStanding;

	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;
};

LINK_ENTITY_TO_CLASS( monster_dragon, CDragon );

void CDragon :: FollowerUse2( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
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

TYPEDESCRIPTION	CDragon::m_SaveData[] = 
{
	DEFINE_FIELD( CDragon, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CDragon, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CDragon, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CDragon, m_flNextFireTime, FIELD_TIME ),
	DEFINE_FIELD( CDragon, m_flNextFire2Time, FIELD_TIME ),
	DEFINE_FIELD( CDragon, m_flNextFire3Time, FIELD_TIME ),
	DEFINE_FIELD( CDragon, m_flNextFire4Time, FIELD_TIME ),
	DEFINE_FIELD( CDragon, m_flyingtime, FIELD_TIME ),
	DEFINE_FIELD( CDragon, m_dyingtime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CDragon, CSquadMonster );

//=========================================================
// IRelationship - overridden because Alien Grunts are 
// Human Grunt's nemesis.
//=========================================================
int CDragon::IRelationship ( CBaseEntity *pTarget )
{
	return CSquadMonster::IRelationship( pTarget );
}


//=========================================================
// RunAI
//=========================================================
void CDragon :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(pev->sequence == LookupActivity ( ACT_RUN )){
	   m_flGroundSpeed = 450;
	}
	else if(pev->sequence == LookupActivity ( ACT_WALK )){
	   m_flGroundSpeed = 120;
	}

	if ( m_MonsterState == MONSTERSTATE_PRONE || m_IdealMonsterState == MONSTERSTATE_PRONE ){
		m_dyingtime++;
		if(m_dyingtime > 30){//��������
		BarnacleVictimReleased();
		m_dyingtime = -20;
		m_canbarnacle_mode  = 0;
		m_freeze_def = 0;
		}
	}
	else if(m_dyingtime > 0){
		m_dyingtime--;
	}
	else if(m_dyingtime < 0){
		m_dyingtime++;
		if(m_dyingtime == 0){
		m_canbarnacle_mode  = 1;
		m_freeze_def = 0;
		}
	}

	//���ܤν���
	if(m_rpgms_level >= 72 && m_rpgms_skill5_learn == 0){
		m_rpgms_skill5_learn = 43;//�����ɳ�
		m_rpgms_skill6_learn = 42;//Ѫ������
		pev->max_health = 1800;
		pev->health = pev->max_health;
	}

	if(m_rpgms_level >= 48 && m_rpgms_skill4_learn == 0){
		m_rpgms_skill4_learn = 61;//��ն
	}
}


void CDragon::Killed( entvars_t *pevAttacker, int iGib )
{
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_KILLBEAM );
	WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_KILLBEAM );
	WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
	MESSAGE_END();

	CSquadMonster::Killed( pevAttacker, GIB_NEVER );
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CDragon :: GibMonster ( void )
{
	CBaseMonster :: GibMonster();
}

//=========================================================
// ISoundMask - Overidden for human grunts because they 
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CDragon :: ISoundMask ( void )
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
BOOL CDragon :: FCanCheckAttacks ( void )
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
BOOL CDragon :: CheckRangeAttack1 ( float flDot, float flDist )
{
	float dist = 768;

	if(m_flNextFire3Time <= gpGlobals->time ||
	(m_rpgms_skill4_learn == 61 && m_flNextFire4Time <= gpGlobals->time)){
	dist = 2048;
	}
	else if(m_flNextFireTime > gpGlobals->time || m_playerguardian_mode == 1){
	return FALSE;
	}
	
			if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= dist && flDot >= 0.5)
			{

			//���˼��
			TraceResult	tr;

			if ( gpGlobals->time > m_checkAttackTime )
			{
				TraceResult tr;

				Vector shootOrigin = pev->origin + Vector(0,0,57);
				CBaseEntity *pEnemy = m_hEnemy;
				if ( !pEnemy )
				{
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

BOOL CDragon :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	float cover_dist = 128;

	if(m_playerguardian_mode == 1){
	cover_dist = 256;
	}

	if ( m_hEnemy != NULL )
	{
		if(FClassnameIs(m_hEnemy->pev, "monster_doma_boss")){
		cover_dist += 160;
		}
	}

	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= cover_dist && flDot >= 0.5)
	{
		return TRUE;
	}

	return FALSE;
}

BOOL CDragon :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	if (m_flNextFire2Time > gpGlobals->time || m_playerguardian_mode == 1 || m_rpgms_skill6_learn == 0){
	return FALSE;
	}

	float cover_dist = 128;

	CBaseEntity *pEnemy = m_hEnemy;
	if ( !pEnemy )
	{
		return FALSE;
	}

	if ( m_hEnemy != NULL )
	{
		if(FClassnameIs(m_hEnemy->pev, "monster_doma_boss")){
		cover_dist += 160;
		}
	}

	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= cover_dist && flDot >= 0.5)
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
void CDragon :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
int CDragon :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if(pev->sequence == LookupActivity ( ACT_RANGE_ATTACK1 )){
	flDamage *= 0.5;//�����ͷ��У��˺����٣�
	}
	if(pev->sequence == LookupActivity ( ACT_MELEE_ATTACK2 )){
	flDamage *= 0.2;//����ɱ�ͷ��У��˺�������٣�
	}

	m_alert	= 100;

	Forget( bits_MEMORY_INCOVER );

	return CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CDragon :: SetYawSpeed ( void )
{
	pev->yaw_speed = 450;
}

void CDragon :: IdleSound( void )
{

}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CDragon :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CDragon :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	int atk_dist = 145;
	int atk_dist2 = 305;

	if(m_playerguardian_mode == 1){
	atk_dist = 300;
	atk_dist2 = 450;
	}

	if(m_hEnemy != NULL){//����������
	atk_dist += m_hEnemy->pev->maxs.x;
	atk_dist2 += m_hEnemy->pev->maxs.x;
	}

	switch( pEvent->event )
	{
		case 1://�չ�
		{
			if(m_hEnemy != NULL){
				
				if(( pev->origin - m_hEnemy->pev->origin).Length() <= atk_dist){

				if(m_hEnemy->pev->takedamage){
				m_hEnemy->TakeDamage( pev, pev, 180, DMG_SLASH );
				TakeHealth(36, DMG_GENERIC);//��Ѫ

				if(m_hEnemy->pev->gravity <= 1.5){
				m_hEnemy->pev->velocity = (m_hEnemy->pev->origin - pev->origin).Normalize() * 200;
				}

				FX_Explosion(m_hEnemy->Center(), 46 );
				}
				
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "rmxp/170-Skill14.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}
		}
		break;

		case 2://����
		{
			if(m_hEnemy != NULL){
				if(m_playerguardian_mode == 1){
				atk_dist = 256;
				}
				if(( pev->origin - m_hEnemy->pev->origin).Length() <= atk_dist){
				
				if(m_hEnemy->pev->takedamage){
				m_hEnemy->TakeDamage( pev, pev, 100, DMG_SLASH );
				TakeHealth(20, DMG_GENERIC);//��Ѫ

				if(m_hEnemy->pev->gravity <= 1.5){
				m_hEnemy->pev->velocity = (m_hEnemy->pev->origin - pev->origin).Normalize() * 150;
				}

				FX_Explosion(m_hEnemy->Center(), 46 );
				}
				
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "rmxp/170-Skill14.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}
		}
		break;

		case 3://���
		{
			m_flNextFireTime = gpGlobals->time + 10.0;

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex());		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail );	// model
			WRITE_BYTE( 5 ); // life
			WRITE_BYTE( 5 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 255 );	// G
			WRITE_BYTE( 255 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			pev->movetype = MOVETYPE_FLY;
			m_flyingtime = 20;

			UTIL_MakeVectors(pev->angles);
			if(m_hEnemy != NULL){
				if(( pev->origin - m_hEnemy->pev->origin).Length2D() > atk_dist){
				pev->velocity = gpGlobals->v_forward * 3000;
				}
				else if(( pev->origin - m_hEnemy->pev->origin).Length() > atk_dist){
				pev->velocity = gpGlobals->v_up * 1500;
				}
			}

			
		}
		break;

		case 4://�ػ�
		{
			pev->movetype = MOVETYPE_STEP;

			if(m_hEnemy != NULL){
				if(( pev->origin - m_hEnemy->pev->origin).Length() <= atk_dist2){
				
				if(m_hEnemy->pev->takedamage){
				TakeHealth(120, DMG_GENERIC);//��Ѫ
				m_hEnemy->TakeDamage( pev, pev, 600, DMG_SLASH );
					if(m_hEnemy->pev->gravity <= 1.5){
					m_hEnemy->pev->velocity = (m_hEnemy->pev->origin - pev->origin).Normalize() * 450;
					}
				}

				FX_Explosion(m_hEnemy->Center(), 47 );
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "rmxp/158-Skill02.wav", 1.0, 0.7, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}
		}
		break;

		case 5://������
		{
			::RadiusDamage_limit( Center(), pev, pev, 80, 240, CLASS_PLAYER_ALLY, DMG_BLOOD);

			if(m_hEnemy != NULL){

				if(m_playerguardian_mode == 1){
				atk_dist = 192;
				}
				if(( pev->origin - m_hEnemy->pev->origin).Length() <= atk_dist){
				
				if(m_hEnemy->pev->takedamage){
				m_hEnemy->TakeDamage( pev, pev, 60, DMG_SLASH );
				TakeHealth(24, DMG_GENERIC);//��Ѫ

				if(m_hEnemy->pev->gravity <= 1.5){
				m_hEnemy->pev->velocity = (m_hEnemy->pev->origin - pev->origin).Normalize() * 150;
				}

				FX_Explosion(m_hEnemy->Center(), 46 );
				}
				
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "rmxp/170-Skill14.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}
		}
		break;

		case 6://��β1
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail );	// model
			WRITE_BYTE( 3 ); // life
			WRITE_BYTE( 3 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 32 );	// G
			WRITE_BYTE( 32 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
			}
		break;

		case 7://��β1 ���
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_KILLBEAM );
			WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
			MESSAGE_END();
		}
		break;

		case 8://��β2
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail );	// model
			WRITE_BYTE( 3 ); // life
			WRITE_BYTE( 3 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 32 );	// G
			WRITE_BYTE( 32 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
			}
		break;

		case 9://��β1+2
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail );	// model
			WRITE_BYTE( 3 ); // life
			WRITE_BYTE( 3 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 32 );	// G
			WRITE_BYTE( 32 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail );	// model
			WRITE_BYTE( 3 ); // life
			WRITE_BYTE( 3 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 32 );	// G
			WRITE_BYTE( 32 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
		}
		break;

		case 10://��β1+2 ���
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_KILLBEAM );
			WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
			MESSAGE_END();

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_KILLBEAM );
			WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
			MESSAGE_END();
		}
		break;

		case 11:
			if(m_hEnemy != NULL){
				if(m_hEnemy->IsPlayer()){
				ClearSchedule();
				SetYawSpeed();

				MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_KILLBEAM );
				WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
				MESSAGE_END();

				MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_KILLBEAM );
				WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
				MESSAGE_END();
				}
			}
			else{
				ClearSchedule();
				SetYawSpeed();

				MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_KILLBEAM );
				WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
				MESSAGE_END();

				MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_KILLBEAM );
				WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
				MESSAGE_END();
			}
		break;

		case 12://2 ���
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_KILLBEAM );
			WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
			MESSAGE_END();
		}
		break;

		case 13://�չ���Ѫ������
		{	
			if(m_hEnemy != NULL){

				if(( pev->origin - m_hEnemy->pev->origin).Length() <= atk_dist){

				if(m_hEnemy->pev->takedamage){
				m_hEnemy->TakeDamage( pev, pev, 180, DMG_SLASH );
				TakeHealth(36, DMG_GENERIC);//��Ѫ
				FX_Explosion(m_hEnemy->Center(), 46 );
				}
				
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "rmxp/170-Skill14.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}
		}
		break;

		case 14://�ػ���Ѫ�������ػ�
		{
			if(m_hEnemy != NULL){

				if(( pev->origin - m_hEnemy->pev->origin).Length() <= atk_dist){

				if(m_hEnemy->pev->takedamage){
				m_hEnemy->TakeDamage( pev, pev, 480, DMG_SLASH );
				TakeHealth(96, DMG_GENERIC);//��Ѫ
				FX_Explosion(m_hEnemy->Center(), 47 );

				if(m_hEnemy->pev->gravity <= 1.5){
				m_hEnemy->pev->velocity = (m_hEnemy->pev->origin - pev->origin).Normalize() * 300;
				}

				}
				
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "rmxp/158-Skill02.wav", 1.0, 0.7, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}
		}
		break;

		case 15://Ѫ��������ʼ������ȴ
		{
			m_dyingtime = -30;
			m_canbarnacle_mode  = 0;//���߿���
			m_freeze_def = 2;//����ο���LV2
			m_flNextFire2Time = gpGlobals->time + 45;
		}
		break;

		case 16:
		{
				if(m_hEnemy != NULL){
					
					if(( pev->origin - m_hEnemy->pev->origin).Length() <= atk_dist){

					if(m_hEnemy->pev->takedamage){
					m_hEnemy->TakeDamage( pev, pev, 180, DMG_SLASH );
					TakeHealth(36, DMG_GENERIC);//��Ѫ

					if(m_hEnemy->pev->gravity <= 1.5){
					m_hEnemy->pev->velocity = (m_hEnemy->pev->origin - pev->origin).Normalize() * 200;
					}

					FX_Explosion(m_hEnemy->Center(), 46 );
					}
					
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "rmxp/170-Skill14.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
				}

				if(m_hEnemy != NULL){
					if((m_rpgms_skill4_learn == 61 && m_flNextFire4Time <= gpGlobals->time)){//��ն
						Vector	vecArmPos,vecArmDir;
						GetAttachment( 0, vecArmPos, vecArmDir );

							Vector vecDirToEnemy;
							Vector angDir;

							if (HasConditions( bits_COND_SEE_ENEMY))
							{
								vecDirToEnemy = ( ( m_vecEnemyLKP ) - pev->origin );
								angDir = UTIL_VecToAngles( vecDirToEnemy );
								vecDirToEnemy = vecDirToEnemy.Normalize();
							}
							else
							{
								angDir = pev->angles;
								UTIL_MakeAimVectors( angDir );
								vecDirToEnemy = gpGlobals->v_forward;
							}

						vecArmPos = vecArmPos + vecDirToEnemy * 8;
						CBaseEntity *pHornet = CBaseEntity::Create( "dragon_strike", vecArmPos, UTIL_VecToAngles( vecDirToEnemy ), edict() );
						UTIL_MakeVectors ( pHornet->pev->angles );
						pHornet->pev->velocity = gpGlobals->v_forward * 600;
						EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "dragon/dragon_strike.wav", 1.0, 0.6, 0, 100 );
						m_flNextFire4Time = gpGlobals->time + 20;
					}
					else if(m_flNextFire3Time <= gpGlobals->time){//��������

						Vector	vecSpitOffset,vangle;
						Vector	vecSpitDir;

						UTIL_MakeVectors ( pev->angles );

						// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
						// we should be able to read the position of bones at runtime for this info.
						GetAttachment( 0, vecSpitOffset, vangle );

						vecSpitDir = ( m_hEnemy->Center() - vecSpitOffset ).Normalize();

						// do stuff for this event.
						//AttackSound();

						EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "rmxp/132-Wind01.wav", 1.0, ATTN_NORM,0,100 + RANDOM_LONG(-5,5) );

						CWindScythe::Shoot( pev, vecSpitOffset, vecSpitDir * 3000 );
						m_flNextFire3Time = gpGlobals->time + 6;
					}
				}
		}
		break;

		case 17://�ػ���Ѫ������׷���ػ�������
		{
			m_freeze_def = 0;
			pev->movetype = MOVETYPE_STEP;

			if(m_hEnemy != NULL){

				if(( pev->origin - m_hEnemy->pev->origin).Length() <= atk_dist2){

				if(m_hEnemy->pev->takedamage){
				m_hEnemy->TakeDamage( pev, pev, 600, DMG_SLASH );
				TakeHealth(120, DMG_GENERIC);//��Ѫ
				FX_Explosion(m_hEnemy->Center(), 51 );

				if(m_hEnemy->pev->gravity <= 1.5){
				m_hEnemy->pev->velocity = (m_hEnemy->pev->origin - pev->origin).Normalize() * 450;
				}

				}
				
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "rmxp/160-Skill04.wav", 1.0, 0.7, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}
		}
		break;

		case 18://���2
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex());		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail );	// model
			WRITE_BYTE( 5 ); // life
			WRITE_BYTE( 5 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 255 );	// G
			WRITE_BYTE( 255 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail );	// model
			WRITE_BYTE( 3 ); // life
			WRITE_BYTE( 3 );  // width
			WRITE_BYTE( 192 );	// R
			WRITE_BYTE( 192 );	// G
			WRITE_BYTE( 255 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail );	// model
			WRITE_BYTE( 3 ); // life
			WRITE_BYTE( 3 );  // width
			WRITE_BYTE( 192 );	// R
			WRITE_BYTE( 192 );	// G
			WRITE_BYTE( 255 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			pev->movetype = MOVETYPE_FLY;
			m_flyingtime = 10;

			UTIL_MakeVectors(pev->angles);
			if(m_hEnemy != NULL){
				if(( pev->origin - m_hEnemy->pev->origin).Length2D() > atk_dist){
				pev->velocity = gpGlobals->v_forward * 3000;
				}
				else if(( pev->origin - m_hEnemy->pev->origin).Length() > atk_dist){
				pev->velocity = gpGlobals->v_up * 1500;
				}
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
void CDragon :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/dragon.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;

	pev->health			= 1200;
	m_lovehate			= 120;

	m_flFieldOfView		 = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		 = MONSTERSTATE_NONE;

	m_flNextFireTime	 = gpGlobals->time + 2;
	m_flNextFire2Time	 = gpGlobals->time + 2;
	m_flNextFire3Time	 = gpGlobals->time + 2;
	m_flNextFire4Time	 = gpGlobals->time + 2;//��ն
	m_flyingtime		 = gpGlobals->time + 2;

	m_afCapability		= bits_CAP_SQUAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;

	m_HackedGunPos = Vector ( 0, 0, 55 );

	m_cClipSize			= 114;
	m_cAmmoLoaded		= 114;
	m_canheadcrab_mode  = 0;
	m_canbarnacle_mode  = 1;

	m_no_victdance		= 1;

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();

	m_follow_mode = 1;

	m_ignoredamage		= 1;
	m_headdef			= 2;

	m_candrownwater = 1;
//	m_forcefuckdoor = TRUE;
	m_chase_mode = 3;
	m_chase_failed_max = 4;

	SetUse( &CDragon::FollowerUse2 );

	m_aimenemy_mod = 6;

	pev->body = 0;
	m_fStanding = TRUE;
	SetTouch( &CDragon::DeadTouch );

	m_longming = 1;

	m_rpgms_actor = 16;
	m_rpgms_level = 36;
	m_rpgms_exp = 0;
	m_rpgms_type = 1;

	m_new_ally_type = TRUE;
	pev->netname = MAKE_STRING( "Dragon" );

	m_rpgms_skill1_learn = 33;//��Ѫ
	m_rpgms_skill2_learn = 34;//���ն
	m_rpgms_skill3_learn = 45;//��������
	m_rpgms_skill4_learn = 0;//��ն
	m_rpgms_skill5_learn = 0;//�����ɳ�
	m_rpgms_skill6_learn = 0;//Ѫ������

	pev->takedamage = DAMAGE_YES;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CDragon :: Precache()
{
	PRECACHE_MODEL("models/dragon.mdl");
	PRECACHE_SOUND("dragon/die1.wav");
	//PRECACHE_SOUND("dragon/katanad_hit1.wav");
	//PRECACHE_SOUND("dragon/skill_hit1.wav");
	PRECACHE_SOUND("rmxp/170-Skill14.wav");
	PRECACHE_SOUND("rmxp/158-Skill02.wav");
	PRECACHE_SOUND("rmxp/160-Skill04.wav");
	PRECACHE_SOUND("rmxp/132-Wind01.wav");

	PRECACHE_MODEL("models/wind_scythe.mdl");
	PRECACHE_SOUND("dragon/wind_slash.wav");
	PRECACHE_SOUND("dragon/dragon_strike.wav");
	UTIL_PrecacheOther( "dragon_strike" );
}	

//=========================================================
// start task
//=========================================================
void CDragon :: StartTask ( Task_t *pTask )
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
		if(m_hEnemy == NULL)
		{
			SetConditions( bits_COND_GRUNT_NOFIRE );
		}
		else if(m_hEnemy->IsPlayer() && m_lovehate > 0)
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
void CDragon :: RunTask ( Task_t *pTask )
{
	CSquadMonster :: RunTask( pTask );
}

//=========================================================
// DeathSound 
//=========================================================
void CDragon :: DeathSound ( void )
{
	EMIT_SOUND( ENT(pev), 6, "dragon/die1.wav", 1, 0.6 );	
}

//=========================================================
// GruntFail
//=========================================================
Task_t	tlDragonFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)1		},
	{ TASK_FORGET_ENEMY,		(float)0		},
};

Schedule_t	slDragonFail[] =
{
	{
		tlDragonFail,
		ARRAYSIZE ( tlDragonFail ),
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
Task_t	tlDragonCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)1		},
};

Schedule_t	slDragonCombatFail[] =
{
	{
		tlDragonCombatFail,
		ARRAYSIZE ( tlDragonCombatFail ),
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
Task_t	tlDragonVictoryDance[] =
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

Schedule_t	slDragonVictoryDance[] =
{
	{ 
		tlDragonVictoryDance,
		ARRAYSIZE ( tlDragonVictoryDance ), 
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
Task_t tlDragonEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_GRUNT_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slDragonEstablishLineOfFire[] =
{
	{ 
		tlDragonEstablishLineOfFire,
		ARRAYSIZE ( tlDragonEstablishLineOfFire ),
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
Task_t	tlDragonFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slDragonFoundEnemy[] =
{
	{ 
		tlDragonFoundEnemy,
		ARRAYSIZE ( tlDragonFoundEnemy ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"GruntFoundEnemy"
	},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t	tlDragonCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)0.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_GRUNT_SWEEP	},
};

Schedule_t	slDragonCombatFace[] =
{
	{ 
		tlDragonCombatFace1,
		ARRAYSIZE ( tlDragonCombatFace1 ), 
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
Task_t	tlDragonMeleeAttack1[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_MELEE_ATTACK1,		(float)0					},
};

Schedule_t	slDragonMeleeAttack1[] =
{
	{ 
		tlDragonMeleeAttack1,
		ARRAYSIZE ( tlDragonMeleeAttack1 ), 
		bits_COND_GRUNT_NOFIRE,
		0,
		"Melee1"
	},
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or grunt gets hurt.
//=========================================================
Task_t	tlDragonMeleeAttack2[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_MELEE_ATTACK2,		(float)0					},
};

Schedule_t	slDragonMeleeAttack2[] =
{
	{ 
		tlDragonMeleeAttack2,
		ARRAYSIZE ( tlDragonMeleeAttack2 ), 
		bits_COND_GRUNT_NOFIRE,
		0,
		"Melee1"
	},
};

Task_t	tlDragonSuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slDragonSuppress[] =
{
	{ 
		tlDragonSuppress,
		ARRAYSIZE ( tlDragonSuppress ), 
		bits_COND_GRUNT_NOFIRE,
		0,
		"Suppress"
	},
};

Task_t	tlDragonRangeSC[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_RANGE_ATTACK2,		(float)0					},
};

Schedule_t	slDragonRangeSC[] =
{
	{ 
		tlDragonRangeSC,
		ARRAYSIZE ( tlDragonRangeSC ), 
		bits_COND_GRUNT_NOFIRE,
		0,
		"Suppress SC"
	},
};

//=========================================================
// grunt wait in cover - we don't allow danger or the ability
// to attack to break a grunt's run to cover schedule, but
// when a grunt is in cover, we do want them to attack if they can.
//=========================================================
Task_t	tlDragonWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slDragonWaitInCover[] =
{
	{ 
		tlDragonWaitInCover,
		ARRAYSIZE ( tlDragonWaitInCover ), 
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
Task_t	tlDragonTakeCover1[] =
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

Schedule_t	slDragonTakeCover[] =
{
	{ 
		tlDragonTakeCover1,
		ARRAYSIZE ( tlDragonTakeCover1 ), 
		bits_COND_ENEMY_DEAD,
		0,
		"TakeCover"
	},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlDragonGrenadeCover1[] =
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

Schedule_t	slDragonGrenadeCover[] =
{
	{ 
		tlDragonGrenadeCover1,
		ARRAYSIZE ( tlDragonGrenadeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlDragonTossGrenadeCover1[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slDragonTossGrenadeCover[] =
{
	{ 
		tlDragonTossGrenadeCover1,
		ARRAYSIZE ( tlDragonTossGrenadeCover1 ), 
		bits_COND_ENEMY_DEAD,
		0,
		"TossGrenadeCover"
	},
};
//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlDragonTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slDragonTakeCoverFromBestSound[] =
{
	{ 
		tlDragonTakeCoverFromBestSound,
		ARRAYSIZE ( tlDragonTakeCoverFromBestSound ), 
		0,
		0,
		"TakeCoverFromBestSound"
	},
};


//=========================================================
// Grunt reload schedule
//=========================================================
Task_t	tlDragonHideReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t slDragonHideReload[] = 
{
	{
		tlDragonHideReload,
		ARRAYSIZE ( tlDragonHideReload ),
		bits_COND_CAN_MELEE_ATTACK1,
		bits_SOUND_DANGER,
		"GruntHideReload"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlDragonSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_FORGET_ENEMY,		(float)0	},
};

Schedule_t	slDragonSweep[] =
{
	{ 
		tlDragonSweep,
		ARRAYSIZE ( tlDragonSweep ), 
		
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

DEFINE_CUSTOM_SCHEDULES( CDragon )
{
	slDragonFail,
	slDragonCombatFail,
	slDragonVictoryDance,
	slDragonEstablishLineOfFire,
	slDragonFoundEnemy,
	slDragonCombatFace,
	slDragonMeleeAttack1,
	slDragonMeleeAttack2,
	slDragonSuppress,
	slDragonWaitInCover,
	slDragonTakeCover,
	slDragonGrenadeCover,
	slDragonTossGrenadeCover,
	slDragonTakeCoverFromBestSound,
	slDragonHideReload,
	slDragonSweep,
	slDragonRangeSC,
};

IMPLEMENT_CUSTOM_SCHEDULES( CDragon, CSquadMonster );

//=========================================================
// SetActivity 
//=========================================================
void CDragon :: SetActivity ( Activity NewActivity )
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
Schedule_t *CDragon :: GetSchedule( void )
{

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

				if ( HasConditions(bits_COND_NEW_ENEMY) )
				{
					if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
					{
						return GetScheduleOfType ( SCHED_GRUNT_SUPPRESS );
					}
					else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK2 ) )
					{
						return GetScheduleOfType ( SCHED_MELEE_ATTACK2 );
					}
					else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
					{
						return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
					}
					else
					{
						return GetScheduleOfType ( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
					}
				}
				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
				{
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}
				else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK2 ) )
				{
					return GetScheduleOfType ( SCHED_MELEE_ATTACK2 );
				}
				else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
				{
					return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
				}
	// can't see enemy
				else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
				{
					return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
				}
				
				if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
				{
					return GetScheduleOfType ( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
				}
			}
			break;
	}
	
	// no special cases here, call the base class
	return CSquadMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CDragon :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			return &slDragonTakeCover[ 0 ];
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slDragonTakeCoverFromBestSound[ 0 ];
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
			return &slDragonCombatFace[ 0 ];
			}
			else{
			return GetScheduleOfType ( SCHED_CHASE_ENEMY_FAILED );
			}
		}
		break;
	case SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE:
		{
			if(m_groundElev){
			return &slDragonCombatFace[ 0 ];
			}
			else{
			return &slDragonEstablishLineOfFire[ 0 ];
			}
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			if(m_flNextFire3Time <= gpGlobals->time || 
			(m_rpgms_skill4_learn == 61 && m_flNextFire4Time <= gpGlobals->time) ){
			return &slDragonRangeSC[ 0 ];
			}
			else{
			return &slDragonSuppress[ 0 ];
			}
		}
	case SCHED_COMBAT_FACE:
		{
			return &slDragonCombatFace[ 0 ];
		}
	case SCHED_GRUNT_WAIT_FACE_ENEMY:
		{
			return &slDragonWaitInCover[ 0 ];
		}
	case SCHED_GRUNT_SWEEP:
		{
			return &slDragonSweep[ 0 ];
		}
	case SCHED_GRUNT_COVER_AND_RELOAD:
		{
			return &slDragonHideReload[ 0 ];
		}
	case SCHED_GRUNT_FOUND_ENEMY:
		{
			return &slDragonFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			return &slDragonVictoryDance[ 0 ];
		}
	case SCHED_MELEE_ATTACK1:
		{
			return &slDragonMeleeAttack1[ 0 ];
		}
	case SCHED_MELEE_ATTACK2:
		{
			return &slDragonMeleeAttack2[ 0 ];
		}
	case SCHED_GRUNT_SUPPRESS:
		{
			if(m_flNextFire3Time <= gpGlobals->time || 
			(m_rpgms_skill4_learn == 61 && m_flNextFire4Time <= gpGlobals->time) ){
			return &slDragonRangeSC[ 0 ];
			}
			else{
			return &slDragonSuppress[ 0 ];
			}
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slDragonCombatFail[ 0 ];
			}

			return &slDragonFail[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}