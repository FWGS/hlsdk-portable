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

class CNobita : public CSquadMonster
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
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );

	void CheckAmmo ( void );
	void RunAI( void );
	void Killed( entvars_t *pevAttacker, int iGib );

	void SetActivity ( Activity NewActivity );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	void DeathSound( void );
	void PainSound( void );
	void Shoot ( void );

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
	float m_flNextJumpTime;
	float m_flNextPainTime;
	float m_flNextMeleeTime;
	float m_flLastEnemySightTime;

	float m_flNextFireTime;
	float m_flNextFire2Time;
	float m_flLastSkillTime;
	float m_dyingtime;

	Vector	m_vecTossVelocity;
	float	m_flNextGrenadeCheck;

	BOOL	m_fThrowGrenade;
	BOOL	m_fStanding;
	BOOL	m_fFirstEncounter;// only put on the handsign show in the squad's first encounter.

	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;
};

LINK_ENTITY_TO_CLASS( monster_nobita, CNobita );

void CNobita :: FollowerUse2( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
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

TYPEDESCRIPTION	CNobita::m_SaveData[] = 
{
	DEFINE_FIELD( CNobita, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CNobita, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNobita, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNobita, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CNobita, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNobita, m_flNextFireTime, FIELD_TIME ),
	DEFINE_FIELD( CNobita, m_dyingtime, FIELD_TIME ),
	DEFINE_FIELD( CNobita, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CNobita, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CNobita, m_flNextJumpTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CNobita, CSquadMonster );

//=========================================================
// IRelationship - overridden because Alien Grunts are 
// Human Grunt's nemesis.
//=========================================================
int CNobita::IRelationship ( CBaseEntity *pTarget )
{
	if ( FClassnameIs( pTarget->pev, "monster_doraemon_boss" ) )
	{
		return R_NM;
	}

	return CSquadMonster::IRelationship( pTarget );
}

//=========================================================
// RunAI
//=========================================================
void CNobita :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(pev->sequence == LookupActivity ( ACT_RUN )){
		m_flGroundSpeed = 330;
	}
	else if(pev->sequence == LookupActivity ( ACT_WALK )){
	    m_flGroundSpeed = 120;
	}

	if(m_flNPC_Pain > 0){
		if(m_flNPC_Pain > 40)
		m_flNPC_Pain = 40;

		m_flNPC_Pain--;
	}

	if(m_hEnemy != NULL){//����BUG
		if(m_enemyfollower == 1 && m_lovehate > 0){
			if(m_hEnemy->IsPlayer() && pev->sequence == LookupActivity ( ACT_RANGE_ATTACK1 )){
			SetActivity( ACT_IDLE );//�Ѿ��˺�ֹͣ����
			ClearSchedule();
			}
		}
	}

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

	if(pev->armorvalue > 0){
		pev->armorvalue--;
		if(pev->armorvalue <= 0){
			pev->renderfx = 0;
			pev->takedamage = DAMAGE_YES;
		}
	}

	//���ܤν���
	if(m_rpgms_level >= 60 && m_rpgms_skill7_learn == 0){
	m_rpgms_skill7_learn = 74;//�������74
	}
	if(m_rpgms_level >= 45 && m_rpgms_skill6_learn == 0){
	m_rpgms_skill6_learn = 46;//���޵�46
	}
}


void CNobita::Killed( entvars_t *pevAttacker, int iGib )
{
	if(m_die == 0 && m_rpgms_skill7_learn == 74 && m_flLastSkillTime < gpGlobals->time){
	pev->deadflag = DEAD_NO;
	pev->health = pev->max_health * 0.5;
	m_flNextFireTime = 0;
	FX_Explosion( Center(), 42);
	m_flLastSkillTime = gpGlobals->time + 90;
	ClearSchedule();
	pev->renderfx = kRenderFxGlowShell;
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 255;
	pev->renderamt = 2;
	pev->takedamage = DAMAGE_NO;
	pev->armorvalue = 100;
	return;
	}

	CSquadMonster::Killed( pevAttacker, GIB_NEVER );
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CNobita :: GibMonster ( void )
{
	CBaseMonster :: GibMonster();
}

//=========================================================
// ISoundMask - Overidden for human grunts because they 
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CNobita :: ISoundMask ( void )
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
BOOL CNobita :: FCanCheckAttacks ( void )
{
	return TRUE;
}


//=========================================================
// CheckRangeAttack1 - overridden for HGrunt, cause 
// FCanCheckAttacks() doesn't disqualify all attacks based
// on whether or not the enemy is occluded because unlike
// the base class, the HGrunt can attack when the enemy is
// occluded (throw grenade over wall, etc). We must 
// disqualify the machine gun attack if the enemy is occluded.
//=========================================================
BOOL CNobita :: CheckRangeAttack1 ( float flDot, float flDist )
{
	float dist = 2048;
	
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

BOOL CNobita :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	float cover_dist = 128;

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
	|| pev->sequence == LookupActivity ( ACT_SLEEP ) 
	|| m_cleardally_enemy == 0){
		return FALSE;
	}

	if(flDist <= cover_dist && flDot >= 0.8 && m_flNextJumpTime <= gpGlobals->time ){
		if(pev->sequence != LookupActivity ( ACT_LEAP )){
		SetActivity ( ACT_LEAP );
		}
	}


	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack. 
//=========================================================
BOOL CNobita :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if ( m_hEnemy != NULL )
	{
		if (m_hEnemy->Classify() == CLASS_ALIEN_BIOWEAPON || m_hEnemy->Classify() == CLASS_PLAYER_BIOWEAPON )
		{
		return FALSE;
		}
	}
	
	// if the grunt isn't moving, it's ok to check.
	if ( m_flGroundSpeed != 0 )
	{
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	// assume things haven't changed too much since last time
	if (gpGlobals->time < m_flNextGrenadeCheck )
	{
		return m_fThrowGrenade;
	}

	if ( !FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) && m_hEnemy->pev->waterlevel == 0 && m_vecEnemyLKP.z > pev->absmax.z  )
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to 
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}
	
	Vector vecTarget;

	// find target
	// vecTarget = m_hEnemy->BodyTarget( pev->origin );
	vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget_o( pev->origin ) - m_hEnemy->pev->origin);
	// estimate position
	if (HasConditions( bits_COND_SEE_ENEMY))
		vecTarget = vecTarget + ((vecTarget - pev->origin).Length() / 800) * m_hEnemy->pev->velocity;

	// are any of my squad members near the intended grenade impact area?
	if ( InSquad() )
	{
		if (SquadMemberInRange( vecTarget, 256 ))
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
			m_fThrowGrenade = FALSE;
		}
	}
	
	if ( ( vecTarget - pev->origin ).Length2D() <= 256 )
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}
	else if ( ( vecTarget - pev->origin ).Length2D() >= 2560 )
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}
		
	Vector vecToss = VecCheckThrow( pev, GetGunPosition(), vecTarget, 800, 0.5 );

	if ( vecToss != g_vecZero )
	{
			m_vecTossVelocity = vecToss;

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

	

	return m_fThrowGrenade;
}


//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
void CNobita :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
int CNobita :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if(pev->sequence == LookupActivity ( ACT_LEAP )
	|| pev->sequence == LookupActivity ( ACT_SLEEP )){
	return 0;//�޵�֡
	}

	CBaseEntity *pEnemy = m_hEnemy;
	if ( pEnemy )
	{
		if(FClassnameIs( m_hEnemy->pev, "monster_doraemon_boss" )){//�Զ���A��ս����!
			flDamage *= 0.5;
		}
	}

	m_alert	= 100;

	Forget( bits_MEMORY_INCOVER );

	return CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CNobita :: SetYawSpeed ( void )
{
	pev->yaw_speed = 350;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CNobita :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

//=========================================================
// Shoot
//=========================================================
void CNobita :: Shoot ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	return;

	Vector vecShootOrigin,vecShootOrigin2,vecdir;
	UTIL_MakeVectors(pev->angles);
	
	vecShootOrigin2 = pev->origin;

	GetAttachment( 0, vecShootOrigin,vecdir);

	CBaseEntity *pEnemy = m_hEnemy;
	if ( pEnemy )
	{//��׼����μӳ�
	m_vecEnemyLKP = pEnemy->pev->origin;
	}

	m_aimenemy_mod = 4;
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	m_aimenemy_mod = 3;
	Vector vecShootDir2 = ShootAtEnemy( vecShootOrigin );

	m_aimenemy_mod = 5;
	Vector vecShootDir3 = ShootAtEnemy( vecShootOrigin );

	if (gpGlobals->time > m_flNextFireTime )
	{
		m_flNextFireTime = gpGlobals->time + 6;
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "nobita/skill_fire.wav", 1, 0.6 );
		FireBullets(1, vecShootOrigin, vecShootDir, Vector(0,0,0), 2560, 812,1); // shoot +-5 degrees
		FireBeam(vecShootOrigin, vecShootDir, 21, 100, pev);

		if(m_hHitEnemy == NULL){
		FireBullets(1, vecShootOrigin, vecShootDir2, Vector(0,0,0), 2560, 812,0); // shoot +-5 degrees
			if(m_hHitEnemy == NULL){
			vecShootOrigin.x = vecShootOrigin2.x;
			vecShootOrigin.y = vecShootOrigin2.y;
			FireBullets(1, vecShootOrigin, vecShootDir3, Vector(0,0,0), 2560, 812,0); // shoot +-5 degrees
			}
		}
		
	}
	else{
		/*
		if (gpGlobals->time > m_flNextFire2Time )//��˯��
		{
			FX_Explosion(vecShootOrigin, EXPLOSION_BIOMASSIMPACT);
			m_flNextFire2Time = gpGlobals->time + 10;
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "nobita/skill_fire2.wav", 1, 0.6 );
			FireBullets(1, vecShootOrigin, vecShootDir3, Vector(0,0,0), 2560, 814,0); // shoot +-5 degrees
		}
		else{
		*/
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/357_shot1.wav", 1, ATTN_NORM );
			FireBullets(1, vecShootOrigin, vecShootDir, Vector(0,0,0), 2560, BULLET_338Magnum,1); // shoot +-5 degrees

			if(m_hHitEnemy == NULL){
			FireBullets(1, vecShootOrigin, vecShootDir2, Vector(0,0,0), 2560, BULLET_338Magnum,0); // shoot +-5 degrees
				if(m_hHitEnemy == NULL){
				vecShootOrigin.x = vecShootOrigin2.x;
				vecShootOrigin.y = vecShootOrigin2.y;
				FireBullets(1, vecShootOrigin, vecShootDir3, Vector(0,0,0), 2560, BULLET_338Magnum,0); // shoot +-5 degrees
				}
			}
	//	}
	}

	m_hHitEnemy = NULL;

	pev->effects |= EF_MUZZLEFLASH;
	
	if(m_rpgms_skill6_learn != 46){
	m_cAmmoLoaded--;// take away a bullet!
	}

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CNobita :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case 2://����
		{
			if(pev->body == 1){
			pev->body = 0;
			}

			Shoot();
		}
		break;

		case 3://����
		{
			if(pev->body == 1){
			pev->body = 0;
			}

			m_cAmmoLoaded = m_cClipSize;
			ClearConditions(bits_COND_NO_AMMO_LOADED);
		}
		break;

		case 4://��
		{
			if(pev->body == 0){
			pev->body = 1;
			}

			if(m_listenlong < 20){
			m_listenlong = 20;
			}

			CBaseMonster *pAlly;

			if(m_hTeamMate1 != NULL){
				pAlly = m_hTeamMate1->MyMonsterPointer();
				if(pAlly->m_listenlong < 20){
				pAlly->m_listenlong = 20;
				}
			}
			if(m_hTeamMate2 != NULL){
			pAlly = m_hTeamMate2->MyMonsterPointer();
				if(pAlly->m_listenlong < 20){
				pAlly->m_listenlong = 20;
				}
			}
			if(m_hTeamMate3 != NULL){
			pAlly = m_hTeamMate3->MyMonsterPointer();
				if(pAlly->m_listenlong < 20){
				pAlly->m_listenlong = 20;
				}
			}
			if(m_hTeamMate4 != NULL){
			pAlly = m_hTeamMate4->MyMonsterPointer();
				if(pAlly->m_listenlong < 20){
				pAlly->m_listenlong = 20;
				}
			}

			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/glauncher.wav", 0.8, ATTN_NORM);
			CGrenade::ShootContact_height( pev, GetGunPosition(), m_vecTossVelocity );
			m_fThrowGrenade = FALSE;
			m_flNextGrenadeCheck = gpGlobals->time + 6;
		}
		break;

		case 5://����ǹ
		{
			pev->body = 1;
		}
		break;

		case 6://������ǹ
		{
			m_hHitEnemy = NULL;
			pev->body = 0;
		}
		break;

		case 7:
		{
			pev->flags &= ~FL_ONGROUND;
			UTIL_MakeVectors(pev->angles);
			pev->velocity = gpGlobals->v_forward * -512;
			pev->velocity.z += 256;

			m_flNextJumpTime = gpGlobals->time + 4;
		}
		break;

		case 8://����
		{
			if(m_hEnemy != NULL){
				if(( pev->origin - m_hEnemy->pev->origin).Length2D() <= 192){
				m_hEnemy->TakeDamage( pev, pev, 30, DMG_SLASH );
				m_hEnemy->pev->velocity = m_hEnemy->pev->velocity + (m_hEnemy->pev->origin - pev->origin).Normalize() * 200;
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/fist_hitbod3.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}
		}
		break;

		case 9:
		{
			ClearSchedule();
			SetYawSpeed();
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
void CNobita :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/nobita.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;

	pev->health			= 600;
	m_lovehate			= 90;

	m_flFieldOfView		 = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		 = MONSTERSTATE_NONE;

	m_flNextFireTime	 = gpGlobals->time + 2;
	m_flNextGrenadeCheck = gpGlobals->time + 2;
	m_flLastSkillTime = gpGlobals->time + 2;
	m_dyingtime			 = 0;

	m_afCapability		= bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 55 );

	m_cClipSize			= 6;
	m_cAmmoLoaded		= 6;
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
	m_chase_mode = 2;
	m_chase_failed_max = 4;

	SetUse( &CNobita::FollowerUse2 );

	m_aimenemy_mod = 4;

	pev->body = 0;
	m_fStanding = TRUE;
	SetTouch( &CNobita::DeadTouch );

	m_rpgms_actor = 17;
	m_rpgms_level = 30;
	m_rpgms_exp = 0;
	m_rpgms_type = 1;

	m_new_ally_type = TRUE;
	pev->netname = MAKE_STRING( "Nobita" );

	m_rpgms_skill1_learn = 26;
	m_rpgms_skill2_learn = 27;
	m_rpgms_skill3_learn = 28;
	m_rpgms_skill4_learn = 47;
	m_rpgms_skill5_learn = 44;
	m_rpgms_skill6_learn = 0;//���޵�46
	m_rpgms_skill7_learn = 0;//�������74
	pev->takedamage = DAMAGE_YES;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNobita :: Precache()
{
	PRECACHE_MODEL("models/nobita.mdl");
	PRECACHE_MODEL("sprites/sleep_z.spr");

	PRECACHE_SOUND("nobita/pain1.wav");
	PRECACHE_SOUND("nobita/die1.wav");
	PRECACHE_SOUND("nobita/skill_fire.wav");
	PRECACHE_SOUND("nobita/skill_fire2.wav");
}	

//=========================================================
// start task
//=========================================================
void CNobita :: StartTask ( Task_t *pTask )
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
void CNobita :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_GRUNT_FACE_TOSS_DIR:
		{
			// project a point along the toss vector and turn to face that point.
			MakeIdealYaw( pev->origin + m_vecTossVelocity * 64 );
			ChangeYaw( pev->yaw_speed );

			if ( FacingIdeal() )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			break;
		}
	case TASK_RANGE_ATTACK1:
		if (pev->health <= pev->max_health * 0.5 && m_rpgms_skill5_learn == 44)
		{
			pev->framerate = 1.5;
		}
	case TASK_RANGE_ATTACK2:
		if (pev->health <= pev->max_health * 0.5 && m_rpgms_skill5_learn == 44)
		{
			pev->framerate = 1.5;
		}
	default:
		{
			CSquadMonster :: RunTask( pTask );
			break;
		}
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CNobita :: DeathSound ( void )
{
	EMIT_SOUND( ENT(pev), 6, "nobita/die1.wav", 1, 0.7 );	
}


//=========================================================
// PainSound
//=========================================================
void CNobita :: PainSound ( void )
{
	if ( pev->spawnflags & SF_MONSTER_GAG ){
	return;
	}

	if ( gpGlobals->time > m_flNextPainTime )
	{
		EMIT_SOUND( ENT(pev), 6, "nobita/pain1.wav", 1, 0.7);

		m_flNextPainTime = gpGlobals->time + 1;
	}
}


//=========================================================
// GruntFail
//=========================================================
Task_t	tlNobitaFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)1		},
	{ TASK_FORGET_ENEMY,		(float)0	},
};

Schedule_t	slNobitaFail[] =
{
	{
		tlNobitaFail,
		ARRAYSIZE ( tlNobitaFail ),
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
Task_t	tlNobitaCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)1		},
};

Schedule_t	slNobitaCombatFail[] =
{
	{
		tlNobitaCombatFail,
		ARRAYSIZE ( tlNobitaCombatFail ),
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
Task_t	tlNobitaVictoryDance[] =
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

Schedule_t	slNobitaVictoryDance[] =
{
	{ 
		tlNobitaVictoryDance,
		ARRAYSIZE ( tlNobitaVictoryDance ), 
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
Task_t tlNobitaEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_GRUNT_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slNobitaEstablishLineOfFire[] =
{
	{ 
		tlNobitaEstablishLineOfFire,
		ARRAYSIZE ( tlNobitaEstablishLineOfFire ),
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
Task_t	tlNobitaFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slNobitaFoundEnemy[] =
{
	{ 
		tlNobitaFoundEnemy,
		ARRAYSIZE ( tlNobitaFoundEnemy ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"GruntFoundEnemy"
	},
};

Task_t	tlNobitaDyingShoot[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SLEEP			},
};

Schedule_t	slNobitaDyingShoot[] =
{
	{ 
		tlNobitaDyingShoot,
		ARRAYSIZE ( tlNobitaDyingShoot ), 
		0,
		0,
		"Nobita Dying Shoot"
	},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t	tlNobitaCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_COMBAT_IDLE		},
	{ TASK_FACE_ENEMY,				(float)0					},
};

Schedule_t	slNobitaCombatFace[] =
{
	{ 
		tlNobitaCombatFace1,
		ARRAYSIZE ( tlNobitaCombatFace1 ), 
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

Task_t	tlNobitaRangeAttack2[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_GRUNT_FACE_TOSS_DIR,		(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RANGE_ATTACK2	},
	{ TASK_SET_SCHEDULE,			(float)SCHED_GRUNT_WAIT_FACE_ENEMY	},// don't run immediately after throwing grenade.
};

Schedule_t	slNobitaRangeAttack2[] =
{
	{ 
		tlNobitaRangeAttack2,
		ARRAYSIZE ( tlNobitaRangeAttack2 ), 
		0,
		0,
		"RangeAttack2"
	},
};


//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or grunt gets hurt.
//=========================================================
Task_t	tlNobitaSignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
};

Schedule_t	slNobitaSignalSuppress[] =
{
	{ 
		tlNobitaSignalSuppress,
		ARRAYSIZE ( tlNobitaSignalSuppress ), 
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

Task_t	tlNobitaSuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slNobitaSuppress[] =
{
	{ 
		tlNobitaSuppress,
		ARRAYSIZE ( tlNobitaSuppress ), 
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
Task_t	tlNobitaWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slNobitaWaitInCover[] =
{
	{ 
		tlNobitaWaitInCover,
		ARRAYSIZE ( tlNobitaWaitInCover ), 
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
Task_t	tlNobitaTakeCover1[] =
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

Schedule_t	slNobitaTakeCover[] =
{
	{ 
		tlNobitaTakeCover1,
		ARRAYSIZE ( tlNobitaTakeCover1 ), 
		bits_COND_ENEMY_DEAD,
		0,
		"TakeCover"
	},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlNobitaGrenadeCover1[] =
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

Schedule_t	slNobitaGrenadeCover[] =
{
	{ 
		tlNobitaGrenadeCover1,
		ARRAYSIZE ( tlNobitaGrenadeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlNobitaTossGrenadeCover1[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slNobitaTossGrenadeCover[] =
{
	{ 
		tlNobitaTossGrenadeCover1,
		ARRAYSIZE ( tlNobitaTossGrenadeCover1 ), 
		0,
		0,
		"TossGrenadeCover"
	},
};
//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlNobitaTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slNobitaTakeCoverFromBestSound[] =
{
	{ 
		tlNobitaTakeCoverFromBestSound,
		ARRAYSIZE ( tlNobitaTakeCoverFromBestSound ), 
		0,
		0,
		"TakeCoverFromBestSound"
	},
};


//=========================================================
// Grunt reload schedule
//=========================================================
Task_t	tlNobitaHideReload[] =
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

Schedule_t slNobitaHideReload[] = 
{
	{
		tlNobitaHideReload,
		ARRAYSIZE ( tlNobitaHideReload ),
		bits_COND_CAN_MELEE_ATTACK1,
		bits_SOUND_DANGER,
		"GruntHideReload"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlNobitaSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_FORGET_ENEMY,		(float)0	},
};

Schedule_t	slNobitaSweep[] =
{
	{ 
		tlNobitaSweep,
		ARRAYSIZE ( tlNobitaSweep ), 
		
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
Task_t	tlNobitaRangeAttack1A[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
};

Schedule_t	slNobitaRangeAttack1A[] =
{
	{ 
		tlNobitaRangeAttack1A,
		ARRAYSIZE ( tlNobitaRangeAttack1A ), 
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


DEFINE_CUSTOM_SCHEDULES( CNobita )
{
	slNobitaFail,
	slNobitaCombatFail,
	slNobitaVictoryDance,
	slNobitaEstablishLineOfFire,
	slNobitaFoundEnemy,
	slNobitaCombatFace,
	slNobitaSignalSuppress,
	slNobitaSuppress,
	slNobitaWaitInCover,
	slNobitaTakeCover,
	slNobitaGrenadeCover,
	slNobitaTossGrenadeCover,
	slNobitaTakeCoverFromBestSound,
	slNobitaHideReload,
	slNobitaSweep,
	slNobitaRangeAttack1A,
	slNobitaRangeAttack2,
	slNobitaDyingShoot,
};

IMPLEMENT_CUSTOM_SCHEDULES( CNobita, CSquadMonster );

//=========================================================
// SetActivity 
//=========================================================
void CNobita :: SetActivity ( Activity NewActivity )
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

void CNobita :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}


//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CNobita :: GetSchedule( void )
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
			if ( m_cleardally_enemy_long == 0){
				if (m_cAmmoLoaded < m_cClipSize){
				return GetScheduleOfType ( SCHED_RELOAD_DEEP );
				}
			}
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

				if( pev->armorvalue > 50 ){
					return GetScheduleOfType( SCHED_SLEEP );
				}

				if( m_HenemyEnemyMe == 4 && (m_flNPC_Pain || pev->health < pev->max_health * 0.4) ){
					if(m_hEnemy != NULL){
						return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
					}
				}

				if ( HasConditions(bits_COND_NEW_ENEMY) )
				{
					if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) )
					{
						return GetScheduleOfType ( SCHED_RANGE_ATTACK2 );
					}
					else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
					{
						return GetScheduleOfType ( SCHED_GRUNT_SUPPRESS );
					}
					else
					{
						if( m_HenemyEnemyMe == 4){
							return GetScheduleOfType ( SCHED_COMBAT_FACE );
						}
						else{
							return GetScheduleOfType ( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
						}
					}
				}
				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) )
				{
					return GetScheduleOfType ( SCHED_RANGE_ATTACK2 );
				}
				else if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
				{ 
					return GetScheduleOfType ( SCHED_GRUNT_COVER_AND_RELOAD );
				}
				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
				{
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}
				else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
				{
					return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
				}
				else if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
				{
					if( m_HenemyEnemyMe == 4){
						return GetScheduleOfType ( SCHED_COMBAT_FACE );
					}
					else{
						return GetScheduleOfType ( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
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
Schedule_t* CNobita :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			return &slNobitaTakeCover[ 0 ];
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slNobitaTakeCoverFromBestSound[ 0 ];
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
			return &slNobitaCombatFace[ 0 ];
			}
			else{
			return GetScheduleOfType ( SCHED_CHASE_ENEMY_FAILED );
			}
		}
		break;
	case SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE:
		{
			if(m_groundElev){
			return &slNobitaCombatFace[ 0 ];
			}
			else{
			return &slNobitaEstablishLineOfFire[ 0 ];
			}
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			return &slNobitaRangeAttack1A[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slNobitaCombatFace[ 0 ];
		}
	case SCHED_SLEEP:
		{
			return &slNobitaDyingShoot[ 0 ];
		}
	case SCHED_RANGE_ATTACK2:
		{
			return &slNobitaRangeAttack2[ 0 ];
		}
	case SCHED_GRUNT_WAIT_FACE_ENEMY:
		{
			return &slNobitaWaitInCover[ 0 ];
		}
	case SCHED_GRUNT_SWEEP:
		{
			return &slNobitaSweep[ 0 ];
		}
	case SCHED_GRUNT_COVER_AND_RELOAD:
		{
			return &slNobitaHideReload[ 0 ];
		}
	case SCHED_GRUNT_FOUND_ENEMY:
		{
			return &slNobitaFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			return &slNobitaVictoryDance[ 0 ];
		}
	case SCHED_GRUNT_SUPPRESS:
		{
			return &slNobitaSuppress[ 0 ];
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slNobitaCombatFail[ 0 ];
			}

			return &slNobitaFail[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}