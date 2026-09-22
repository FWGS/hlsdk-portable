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
// monster-specific DEFINE's
//=========================================================
#define	GRUNT_CLIP_SIZE					36 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!
#define GRUNT_VOL						0.35		// volume of grunt sounds
#define GRUNT_ATTN						ATTN_NORM	// attenutation of grunt sentences
#define HGRUNT_LIMP_HEALTH				30
#define HGRUNT_DMG_HEADSHOT				( DMG_BULLET | DMG_CLUB )	// damage types that can kill a grunt with a single headshot.
#define HGRUNT_NUM_HEADS				2 // how many grunt heads are there? 
#define HGRUNT_MINIMUM_HEADSHOT_DAMAGE	15 // must do at least this much damage in one shot to head to score a headshot kill
#define	SANT_SENTENCE_VOLUME			(float)0.35 // volume of grunt sentences

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
	TASK_GRUNT_SPEAK_SENTENCE,
	TASK_GRUNT_CHECK_FIRE,
	TASK_FORGET_ENEMY,
};

//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_GRUNT_NOFIRE	( bits_COND_SPECIAL1 )

class CSaintna : public CSquadMonster
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

	void CheckAmmo ( void );
	void RunAI( void );
	void Killed( entvars_t *pevAttacker, int iGib );

	void SetActivity ( Activity NewActivity );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	void DeathSound( void );
	void PainSound( void );
	Vector GetGunPosition( void );
	void Shoot ( void );

	void GibMonster( void );
	void SpeakSentence( void );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	
	CBaseEntity	*Kick( void );
	Schedule_t	*GetSchedule( void );
	Schedule_t  *GetScheduleOfType ( int Type );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	virtual int	ObjectCaps( void ) { return CBaseMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	void EXPORT		FollowerUse2( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int IRelationship ( CBaseEntity *pTarget );

	BOOL FOkToSpeak( void );
	void JustSpoke( void );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float m_flNextJumpTime;
	float m_flNextPainTime;
	float m_flNextMeleeTime;
	float m_flLastEnemySightTime;

	BOOL	m_fThrowGrenade;
	BOOL	m_fStanding;
	BOOL	m_fFirstEncounter;// only put on the handsign show in the squad's first encounter.

	int m_voicePitch;
	BOOL	m_fGunDrawn;
	int		m_iBrassShell;

	float	m_dyingtime;
	int		m_dyinguse;

	int		m_iSentence;

	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;

	static const char *pGruntSentences[];
};

LINK_ENTITY_TO_CLASS( monster_saintna, CSaintna );

void CSaintna :: FollowerUse2( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
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
							if(pPlayer->HasTeamMate_CanAdd(this) == TRUE){
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

TYPEDESCRIPTION	CSaintna::m_SaveData[] = 
{
	DEFINE_FIELD( CSaintna, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSaintna, m_flNextJumpTime, FIELD_TIME ),
	DEFINE_FIELD( CSaintna, m_flNextMeleeTime, FIELD_TIME ),
	DEFINE_FIELD( CSaintna, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CSaintna, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSaintna, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSaintna, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSaintna, m_voicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( CSaintna, m_dyingtime, FIELD_TIME ),
	DEFINE_FIELD( CSaintna, m_dyinguse, FIELD_INTEGER ),
	DEFINE_FIELD( CSaintna, m_iSentence, FIELD_INTEGER ),
	DEFINE_FIELD( CSaintna, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CSaintna, m_lastAttackCheck, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CSaintna, CSquadMonster );

const char *CSaintna::pGruntSentences[] = 
{
	"HG_GREN", // grenade scared grunt
	"HG_ALERT", // sees player
	"HG_MONSTER", // sees monster
	"HG_COVER", // running to cover
	"HG_THROW", // about to throw grenade
	"HG_CHARGE",  // running out to get the enemy
	"HG_TAUNT", // say rude things
};

enum
{
	SANT_SENT_NONE = -1,
	SANT_SENT_GREN = 0,
	SANT_SENT_ALERT,
	SANT_SENT_MONSTER,
	SANT_SENT_COVER,
	SANT_SENT_THROW,
	SANT_SENT_CHARGE,
	SANT_SENT_TAUNT,
} SANT_SENTENCE_TYPES;

//=========================================================
// Speak Sentence - say your cued up sentence.
//
// Some grunt sentences (take cover and charge) rely on actually
// being able to execute the intended action. It's really lame
// when a grunt says 'COVER ME' and then doesn't move. The problem
// is that the sentences were played when the decision to TRY
// to move to cover was made. Now the sentence is played after 
// we know for sure that there is a valid path. The schedule
// may still fail but in most cases, well after the grunt has 
// started moving.
//=========================================================
void CSaintna :: SpeakSentence( void )
{
	if ( m_iSentence == SANT_SENT_NONE )
	{
		// no sentence cued up.
		return; 
	}
}

//=========================================================
// IRelationship - overridden because Alien Grunts are 
// Human Grunt's nemesis.
//=========================================================
int CSaintna::IRelationship ( CBaseEntity *pTarget )
{
	if ( FClassnameIs( pTarget->pev, "monster_cof_ms3" ) && pev->weapons == 5 )
	{
		if(m_alert == 0)
		m_alert = 100;

		return R_NM;
	}

	if ( FClassnameIs( pTarget->pev, "monster_cof_ms2" ) && pTarget->pev->frags == 1 )
	{
		return R_NO;
	}
	

	return CSquadMonster::IRelationship( pTarget );
}


//=========================================================
// RunAI
//=========================================================
void CSaintna :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(pev->sequence == LookupActivity ( ACT_FALL )){
		TraceResult tr;
		UTIL_MakeVectors(pev->angles);
		Vector vecSrc	= Center();
		Vector vecEnd	= vecSrc + gpGlobals->v_forward * -24;
		UTIL_TraceLine( vecSrc, vecEnd, ignore_monsters, ENT( pev ), &tr );
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
		if ( pEntity && tr.flFraction < 1.0 ){
			if ( pEntity->pev->solid == SOLID_BSP){
			pev->velocity = -pev->velocity * 0.1;
			FireTargets( "saintna_bad_hitwall", this, this, USE_TOGGLE, 0 );
			FX_Explosion( Center(), 238 );
			SetActivity( ACT_BIG_FLINCH );
			/*
			pev->sequence = LookupActivity ( ACT_BIG_FLINCH );
			ResetSequenceInfo( );
			pev->frame = 0;*/
			}
		}
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

	if(m_hEnemy != NULL){//����BUG
		if(m_enemyfollower == 1 && m_lovehate > 0){
			if(m_hEnemy->IsPlayer() && pev->sequence == LookupActivity ( ACT_RANGE_ATTACK1 )){
			SetActivity( ACT_IDLE );//�Ѿ��˺�ֹͣ����
			ClearSchedule();
			}
		}
	}

	if(m_hEnemy != NULL && m_FTSmod == 4){
		if (FBitSet(pev->effects, EF_DIMLIGHT)){
			if(m_cleardally >= 15){
				if(m_enemyfollower == 1){
				m_enemyfollower = 0;
				m_enemyfollower_combat = 0;
				}
				if(FClassnameIs( m_hEnemy->pev, "npc_aim_flag" ) && m_movementActivity == ACT_RUN ){//ֻ��������
				m_movementActivity = ACT_WALK_SCARED;
				}
			}
			else if(m_enemyfollower == 0){//���û���ϣ�ȥ������
				m_enemyfollower = 1;
				//RouteClear();
				//m_alert = 100;
			}
		}
	}

		if(pev->sequence == LookupActivity ( ACT_RUN )){
		   m_flGroundSpeed = 300;
		}
		else if(pev->sequence == LookupActivity ( ACT_RUN_HURT )){
		   m_flGroundSpeed = 250;
		}

		if(pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
		   m_flGroundSpeed = 110;
		}
		else if(pev->sequence == LookupActivity ( ACT_WALK )){
			if(pev->weapons == 5){
			m_flGroundSpeed = 45;
			}
			else{
		    m_flGroundSpeed = 90;
			}
		}
		else if(pev->sequence == LookupActivity ( ACT_WALK_HURT )){
			if(pev->weapons == 5){
			m_flGroundSpeed = 45;
			}
			else{
			m_flGroundSpeed = 80;
			}
		}

			// flying?
			if ( pev->movetype == MOVETYPE_TOSS && !m_groundElev )
			{
				if (pev->flags & FL_ONGROUND)
				{
					pev->movetype = MOVETYPE_STEP;
				}
			}
			

			if(GetBodygroup( 5 ) == 2){//ս���ֵ�Ͳ
				if(!FBitSet(pev->effects, EF_DIMLIGHT)){
				SetBits(pev->effects, EF_DIMLIGHT);
				}
			}
			else{
				if(FBitSet(pev->effects, EF_DIMLIGHT)){
				ClearBits(pev->effects, EF_DIMLIGHT);
				}
			}

	//���ܤν���
	if(m_rpgms_level >= 60 && m_rpgms_skill6_learn == 0){
	m_rpgms_skill3_learn = 39;
	m_rpgms_skill4_learn = 43;
	m_rpgms_skill5_learn = 68;
	m_rpgms_skill6_learn = 18;
	m_rpgms_skill7_learn = 67;
	pev->frags = 2;//����Ҳ����Ҫ
	pev->max_health = 400;
	pev->health = pev->max_health;
	}

	if(m_rpgms_skill7_learn == 67 && pev->health < pev->max_health){//����
	TakeHealth(1, DMG_GENERIC);
	}
}


void CSaintna::Killed( entvars_t *pevAttacker, int iGib )
{
	if(m_die == 0){
			if(pev->skin != 1){
			pev->skin = 1;
			SetUse( NULL );
			}

			if(pev->frags == 0){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
				if(pEntity){
					CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
					if(pPlayer){//��ҪNPC����ʧ�ܡ�����
					pPlayer->Clear_SayText();
					pPlayer->EnableControl(FALSE);
					pPlayer->m_trainning = 1;
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 4.0, 6.0, 255, FFADE_OUT );
					pPlayer->pev->health = 0;//��ֹ�浵!
					pPlayer->m_iClient_Gameover = -1;
					pPlayer->m_fGameOverTime = gpGlobals->time + 4;
					}
				}
			}
	}

	CSquadMonster::Killed( pevAttacker, GIB_NEVER );
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CSaintna :: GibMonster ( void )
{
	CBaseMonster :: GibMonster();
}

//=========================================================
// ISoundMask - Overidden for human grunts because they 
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CSaintna :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER	|
			bits_SOUND_DANGER;
}

//=========================================================
// someone else is talking - don't speak
//=========================================================
BOOL CSaintna :: FOkToSpeak( void )
{
	return FALSE;
}

//=========================================================
//=========================================================
void CSaintna :: JustSpoke( void )
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
	m_iSentence = SANT_SENT_NONE;
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
BOOL CSaintna :: FCanCheckAttacks ( void )
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
BOOL CSaintna :: CheckMeleeAttack1 ( float flDot, float flDist )
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

		if ( pev->weapons == 5){
		cover_dist += 128;
		}
	}

	if(pev->sequence == LookupActivity ( ACT_LEAP ) 
	|| m_cleardally_enemy == 0){
		return FALSE;
	}

	if ( flDist <= 90 && flDot >= 0.5 &&
	pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
	pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON && m_flNextMeleeTime <= gpGlobals->time
	&& pev->sequence != LookupActivity ( ACT_LEAP )
	&& pEnemy->pev->gravity <= 1.5//̫�ص��߲���!
	&& FBitSet (pev->flags, FL_ONGROUND ) )
	{
		return TRUE;
	}

	if(flDist <= cover_dist && flDot >= 0.8 && m_flNextJumpTime <= gpGlobals->time 
	&& pev->sequence != LookupActivity ( ACT_MELEE_ATTACK1 ) 
	&& pev->sequence != LookupActivity ( ACT_RELOAD )
	&& pev->sequence != LookupActivity ( ACT_LEAP )
	&& FBitSet (pev->flags, FL_ONGROUND )
	&& !m_groundElev){
		SetActivity ( ACT_LEAP );
	}

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
BOOL CSaintna :: CheckRangeAttack1 ( float flDot, float flDist )
{
	float dist = 1280;

			if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= dist && flDot >= 0.6  )
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

//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack. 
//=========================================================
BOOL CSaintna :: CheckRangeAttack2 ( float flDot, float flDist )
{
	return FALSE;
}


//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
void CSaintna :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
int CSaintna :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	m_alert	= 100;

	Forget( bits_MEMORY_INCOVER );

	if(pev->sequence == LookupActivity ( ACT_LEAP )
	|| pev->sequence == LookupActivity ( ACT_MELEE_ATTACK1 )){
	flDamage *= 0.5;
	}

	if(m_rpgms_skill3_learn == 39 && pev->deadflag == DEAD_NO){//����Ч��
		if(!(bitsDamageType & (DMG_AIR | DMG_FALL | DMG_DROWN | DMG_NERVEGAS))){
			if (RANDOM_LONG(0,100) <= 15){
			return 0;
			}
		}
	}

	return CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CSaintna :: SetYawSpeed ( void )
{
	pev->yaw_speed = 320;
}

//=========================================================
// CheckAmmo - overridden for the grunt because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CSaintna :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CSaintna :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

//=========================================================
//=========================================================
CBaseEntity *CSaintna :: Kick( void )
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * 70);

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );
	
	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		return pEntity;
	}

	return NULL;
}

//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

Vector CSaintna :: GetGunPosition( )
{
	if (m_fStanding )
	{
		return pev->origin + Vector( 0, 0, 60 );
	}
	else
	{
		return pev->origin + Vector( 0, 0, 48 );
	}
}

//=========================================================
// Shoot
//=========================================================
void CSaintna :: Shoot ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	return;

	UTIL_MakeVectors(pev->angles);
	Vector vecShootOrigin = pev->origin + Vector(0,0,57);

	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_NULL); 

	
	if(pev->weapons == 1){
			FireBullets(1, vecShootOrigin, vecShootDir, Vector(0,0,0), 2048, BULLET_MONSTER_MP5,1); // shoot +-5 degrees
	}
	else if(m_rpgms_skill5_learn == 68){//ǹеǿ��
			if(m_rpgms_skill6_learn == 18 && RANDOM_LONG(0,100) <= 20){//����һ��
			FireBullets(1, vecShootOrigin, vecShootDir, Vector(0.02,0.02,0.02), 2048, BULLET_338Magnum,1,623);
			}
			else{
			FireBullets(1, vecShootOrigin, vecShootDir, Vector(0.02,0.02,0.02), 2048, BULLET_338Magnum,1); // shoot +-5 degrees
			}
	}
	else{
			if(m_rpgms_skill6_learn == 18 && RANDOM_LONG(0,100) <= 20){//����һ��
			FireBullets(1, vecShootOrigin, vecShootDir, Vector(0.02,0.02,0.02), 2048, BULLET_30mm,1,623);
			}
			else{
			FireBullets(1, vecShootOrigin, vecShootDir, Vector(0.02,0.02,0.02), 2048, BULLET_30mm,1); // shoot +-5 degrees
			}
	}

	pev->effects |= EF_MUZZLEFLASH;
	
	//if(m_rpgms_skill6_learn != 46){
	m_cAmmoLoaded--;// take away a bullet!
	//}

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CSaintna :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;
	int dmg1 = 30;

	switch( pEvent->event )
	{
		case 18:
		{
			if(pev->waterlevel >= 1){
			EMIT_SOUND( edict(), CHAN_BODY, "player/pl_wade2.wav", 1.0, ATTN_IDLE );
			}
			else{
			EMIT_SOUND( edict(), CHAN_BODY, "common/npc_step2.wav", 1.0, ATTN_IDLE );
			}
		}
		break;

		case 19:
		{
			if(pev->waterlevel >= 1){
			EMIT_SOUND( edict(), CHAN_BODY, "player/pl_wade4.wav", 1.0, ATTN_IDLE );
			}
			else{
			EMIT_SOUND( edict(), CHAN_BODY, "common/npc_step4.wav", 1.0, ATTN_IDLE );
			}
		}
		break;

		case 15:
			ClearSchedule();
			SetYawSpeed();
			//ClearSchedule();
			//SetConditions( bits_COND_SCHEDULE_DONE );
			//m_iTaskStatus = TASKSTATUS_COMPLETE;
		break;

		case 5:
			if(m_flNextJumpTime <= gpGlobals->time){
			pev->movetype = MOVETYPE_TOSS;
			pev->flags &= ~FL_ONGROUND;
			UTIL_MakeVectors(pev->angles);
			pev->velocity = gpGlobals->v_forward * -512;
			pev->velocity.z += 128;
			m_flNextJumpTime = gpGlobals->time + 2;
			}
		break;

		case 7:
			if(m_flNextJumpTime <= gpGlobals->time){
			pev->movetype = MOVETYPE_TOSS;
			pev->flags &= ~FL_ONGROUND;
			UTIL_MakeVectors(pev->angles);
			pev->velocity = gpGlobals->v_right * -512;
			pev->velocity.z += 128;
			m_flNextJumpTime = gpGlobals->time + 2;
			}
		break;

		case 8:
			if(m_flNextJumpTime <= gpGlobals->time){
			pev->movetype = MOVETYPE_TOSS;
			pev->flags &= ~FL_ONGROUND;
			UTIL_MakeVectors(pev->angles);
			pev->velocity = gpGlobals->v_right * 512;
			pev->velocity.z += 128;
			m_flNextJumpTime = gpGlobals->time + 2;
			}
		break;

		case 9:
		{
			SetActivity( ACT_IDLE );
			ClearSchedule();
			SetYawSpeed();
			RouteClear();

			if ( m_cAmmoLoaded <= 0 )
			{
			m_cAmmoLoaded = m_cClipSize;
			ClearConditions(bits_COND_NO_AMMO_LOADED);
			}

			m_flNextMeleeTime = gpGlobals->time + 2.0;
		}
		break;

		case 12:
		{
			//	if(GetBodygroup( 5 ) == 1)
			//	SetBodygroup( 5, 0 );
		}
		break;

		case 13:
		{
			FireTargets( "tasfoo_door", this, this, USE_TOGGLE, 0 );
		}
		break;

		case 11:
		{
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
		WRITE_SHORT(g_sModelIndexTrail );	// model
		WRITE_BYTE( 2 ); // life
		WRITE_BYTE( 1 );  // width
		WRITE_BYTE( 223 );	// R
		WRITE_BYTE( 223 );	// G
		WRITE_BYTE( 255 );	// B
		WRITE_BYTE( 157 );	// brightness
	    MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
		}
		break;

		case 10:
		{
			if(m_longming == 1){
			UTIL_MakeVectors(pev->angles);
		//	ClearBits( pev->flags, FL_ONGROUND );
			//pev->origin = pev->origin + gpGlobals->v_forward * 80;
			UTIL_SetOrigin (pev, pev->origin + gpGlobals->v_forward * 80 );
			m_longming = 2;
			}
		}
		break;

		case 6:
		{
			Vector vheadOrigin,vecdir;
			GetAttachment( 1, vheadOrigin,vecdir);
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= pev->origin;
			Vector vecEnd	= vheadOrigin;
			vecSrc.z += 4;
			vecEnd.z += 4;
			UTIL_TraceLine( vecSrc, vecEnd, ignore_monsters, ENT( pev ), &tr );
			if ( tr.flFraction != 1.0 ){
				pev->origin.x = tr.vecEndPos.x + (tr.vecPlaneNormal.x * 40);
				pev->origin.y = tr.vecEndPos.y + (tr.vecPlaneNormal.y * 40);
			}
		}
		break;

		case 2:
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "saintna/reload1.wav", 1, ATTN_NORM );
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		//ClearSchedule();
		//SetYawSpeed();
		//SetConditions( bits_COND_SCHEDULE_DONE );
		//m_iTaskStatus = TASKSTATUS_COMPLETE;
		break;

		case 4:
			if(GetBodygroup( 5 ) == 1)
			SetBodygroup( 5, 0 );

			if(m_hEnemy != NULL){//����BUG
				if(m_enemyfollower == 1 && m_lovehate > 0){
					if(m_hEnemy->IsPlayer() && pev->sequence == LookupActivity ( ACT_RANGE_ATTACK1 )){
					SetActivity( ACT_IDLE );//�Ѿ��˺�ֹͣ����
					ClearSchedule();
					return;
					}
				}
			}

		Shoot();
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/de_shot1.wav", 1, ATTN_NORM );
		CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
		break;

		case 3:
		{
			if(m_flNextMeleeTime <= gpGlobals->time){
				if(m_rpgms_skill6_learn == 18 && RANDOM_LONG(0,100) <= 20){//����һ��
				dmg1 *= 3;
				}

				if(m_hEnemy != NULL){
					if(( pev->origin - m_hEnemy->pev->origin).Length() <= 110){
						TraceResult tr;
						UTIL_MakeVectors(pev->angles);
						Vector vecSrc	= BodyTarget(pev->origin);
						Vector vecEnd	= m_hEnemy->Center();
						UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
						CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

						if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
						int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
						int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
						FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

						ClearMultiDamage( );
						pEntity->TraceAttack(pev, dmg1, gpGlobals->v_forward, &tr, DMG_SLASH); 

						if ( (pEntity->pev->flags & FL_MONSTER) )
						{
							if(pEntity->pev->gravity <= 1.5){
								pEntity->pev->velocity = pEntity->pev->velocity + (pEntity->pev->origin - pev->origin).Normalize() * 300;
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pEntity->MyMonsterPointer();
								if(pEnemyMonster){
								pEnemyMonster->Freeze_Monster(10);
								}
							}
						}

						ApplyMultiDamage( pev, pev );
						EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/fist_hitbod3.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
						}
					}
				}
			}
			else{
				SetActivity( ACT_IDLE );
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
void CSaintna :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/saintna.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;

	pev->health			= 200;//Ůһ�δ�Ƥ
	m_lovehate			= 20;

	m_flFieldOfView		 = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		 = MONSTERSTATE_NONE;

	m_flNextJumpTime	 = gpGlobals->time + 2;
	m_flNextPainTime	 = gpGlobals->time;
	m_flNextMeleeTime    = gpGlobals->time;

	m_iSentence			 = SANT_SENT_NONE;

	m_afCapability		= bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 55 );

	m_fGunDrawn = FALSE;

	m_cClipSize		= 7;

	m_cAmmoLoaded		= m_cClipSize;
	m_canheadcrab_mode  = 0;
	m_canbarnacle_mode  = 1;
	m_aimenemy_mod		= 2;

	m_no_victdance		= 1;
	
	m_dyingtime			= 0;
	m_dyinguse			= 0;

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();

	m_follow_mode = 1;
	m_ignoredamage		= 1;

	m_candrownwater = 1;
	m_forcefuckdoor = TRUE;
	m_chase_mode = 2;
	m_chase_failed_max = 4;

	SetUse( &CSaintna::FollowerUse2 );

	pev->body = 0;
	m_fStanding = TRUE;
	SetTouch( &CSaintna::DeadTouch );

	m_rpgms_actor = 2;
	m_rpgms_level = 10;
	m_rpgms_exp = 0;
	m_rpgms_type = 1;

	m_new_ally_type = TRUE;
	pev->netname = MAKE_STRING( "Saintna" );

	m_rpgms_skill1_learn = 16;//�����ر�
	m_rpgms_skill2_learn = 17;//�߼�
	m_rpgms_skill3_learn = 0;//����39
	m_rpgms_skill4_learn = 0;//�����ɳ�43
	m_rpgms_skill5_learn = 0;//ǹеǿ��68
	m_rpgms_skill6_learn = 0;//����һ��18
	m_rpgms_skill7_learn = 0;//����67

	pev->takedamage = DAMAGE_YES;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSaintna :: Precache()
{
	PRECACHE_MODEL("models/saintna.mdl");
	
	PRECACHE_SOUND("saintna/reload1.wav" );
	PRECACHE_SOUND("saintna/pain1.wav" );
	PRECACHE_SOUND("saintna/pain2.wav" );
	PRECACHE_SOUND("saintna/die1.wav" );

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	m_voicePitch = 100 + RANDOM_LONG(-5,10);

	m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
}	

//=========================================================
// start task
//=========================================================
void CSaintna :: StartTask ( Task_t *pTask )
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

	case TASK_GRUNT_SPEAK_SENTENCE:
		SpeakSentence();
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
void CSaintna :: RunTask ( Task_t *pTask )
{
	CSquadMonster :: RunTask( pTask );
}

//=========================================================
// PainSound
//=========================================================
void CSaintna :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{

		switch ( RANDOM_LONG(0,1) )
		{
		case 0:	
			EMIT_SOUND( ENT(pev), 6, "saintna/pain1.wav", 1, 0.7 );	
			break;
		case 1:
			EMIT_SOUND( ENT(pev), 6, "saintna/pain2.wav", 1, 0.7 );	
			break;
		}

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CSaintna :: DeathSound ( void )
{
	EMIT_SOUND( ENT(pev), 6, "saintna/die1.wav", 1, 0.7 );	
}

//=========================================================
// GruntFail
//=========================================================
Task_t	tlSantFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)1		},
	{ TASK_FORGET_ENEMY,		(float)0	},
};

Schedule_t	slSantFail[] =
{
	{
		tlSantFail,
		ARRAYSIZE ( tlSantFail ),
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
Task_t	tlSantCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)1		},
};

Schedule_t	slSantCombatFail[] =
{
	{
		tlSantCombatFail,
		ARRAYSIZE ( tlSantCombatFail ),
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
Task_t	tlSantVictoryDance[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
};

Schedule_t	slSantVictoryDance[] =
{
	{ 
		tlSantVictoryDance,
		ARRAYSIZE ( tlSantVictoryDance ), 
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
Task_t tlSantEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_GRUNT_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_GRUNT_SPEAK_SENTENCE,(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slSantEstablishLineOfFire[] =
{
	{ 
		tlSantEstablishLineOfFire,
		ARRAYSIZE ( tlSantEstablishLineOfFire ),
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
Task_t	tlSantFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slSantFoundEnemy[] =
{
	{ 
		tlSantFoundEnemy,
		ARRAYSIZE ( tlSantFoundEnemy ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"GruntFoundEnemy"
	},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t	tlSantCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)1.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_GRUNT_SWEEP	},
};

Schedule_t	slSantCombatFace[] =
{
	{ 
		tlSantCombatFace1,
		ARRAYSIZE ( tlSantCombatFace1 ), 
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_MELEE_ATTACK1,
		0,
		"Combat Face"
	},
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or grunt gets hurt.
//=========================================================
Task_t	tlSantSignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
};

Schedule_t	slSantSignalSuppress[] =
{
	{ 
		tlSantSignalSuppress,
		ARRAYSIZE ( tlSantSignalSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"SignalSuppress"
	},
};

Task_t	tlSantSuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slSantSuppress[] =
{
	{ 
		tlSantSuppress,
		ARRAYSIZE ( tlSantSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_CAN_MELEE_ATTACK1 |
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
Task_t	tlSantWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slSantWaitInCover[] =
{
	{ 
		tlSantWaitInCover,
		ARRAYSIZE ( tlSantWaitInCover ), 
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
Task_t	tlSantTakeCover1[] =
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

Schedule_t	slSantTakeCover[] =
{
	{ 
		tlSantTakeCover1,
		ARRAYSIZE ( tlSantTakeCover1 ), 
		bits_COND_ENEMY_DEAD,
		0,
		"TakeCover"
	},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlSantGrenadeCover1[] =
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

Schedule_t	slSantGrenadeCover[] =
{
	{ 
		tlSantGrenadeCover1,
		ARRAYSIZE ( tlSantGrenadeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlSantTossGrenadeCover1[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slSantTossGrenadeCover[] =
{
	{ 
		tlSantTossGrenadeCover1,
		ARRAYSIZE ( tlSantTossGrenadeCover1 ), 
		0,
		0,
		"TossGrenadeCover"
	},
};
//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlSantTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slSantTakeCoverFromBestSound[] =
{
	{ 
		tlSantTakeCoverFromBestSound,
		ARRAYSIZE ( tlSantTakeCoverFromBestSound ), 
		0,
		0,
		"TakeCoverFromBestSound"
	},
};


//=========================================================
// Grunt reload schedule
//=========================================================
Task_t	tlSantHideReload[] =
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

Schedule_t slSantHideReload[] = 
{
	{
		tlSantHideReload,
		ARRAYSIZE ( tlSantHideReload ),
		bits_COND_CAN_MELEE_ATTACK1,
		bits_SOUND_DANGER,
		"GruntHideReload"
	}
};

Task_t	tlSantHideReload_2[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t slSantHideReload_2[] = 
{
	{
		tlSantHideReload_2,
		ARRAYSIZE ( tlSantHideReload_2 ),
		bits_COND_CAN_MELEE_ATTACK1,
		bits_SOUND_DANGER,
		"GruntHideReload"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlSantSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_FORGET_ENEMY,		(float)0	},
};

Schedule_t	slSantSweep[] =
{
	{ 
		tlSantSweep,
		ARRAYSIZE ( tlSantSweep ), 
		
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
Task_t	tlSantRangeAttack1A[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
};

Schedule_t	slSantRangeAttack1A[] =
{
	{ 
		tlSantRangeAttack1A,
		ARRAYSIZE ( tlSantRangeAttack1A ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND		|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_NO_AMMO_LOADED,
		
		bits_SOUND_DANGER,
		"Range Attack1A"
	},
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlSantRangeAttack1B[] =
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

Schedule_t	slSantRangeAttack1B[] =
{
	{ 
		tlSantRangeAttack1B,
		ARRAYSIZE ( tlSantRangeAttack1B ), 
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
Task_t	tlSantRangeAttack2[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_GRUNT_FACE_TOSS_DIR,		(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RANGE_ATTACK2	},
	{ TASK_SET_SCHEDULE,			(float)SCHED_GRUNT_WAIT_FACE_ENEMY	},// don't run immediately after throwing grenade.
};

Schedule_t	slSantRangeAttack2[] =
{
	{ 
		tlSantRangeAttack2,
		ARRAYSIZE ( tlSantRangeAttack2 ), 
		0,
		0,
		"RangeAttack2"
	},
};

DEFINE_CUSTOM_SCHEDULES( CSaintna )
{
	slSantFail,
	slSantCombatFail,
	slSantVictoryDance,
	slSantEstablishLineOfFire,
	slSantFoundEnemy,
	slSantCombatFace,
	slSantSignalSuppress,
	slSantSuppress,
	slSantWaitInCover,
	slSantTakeCover,
	slSantGrenadeCover,
	slSantTossGrenadeCover,
	slSantTakeCoverFromBestSound,
	slSantHideReload,
	slSantHideReload_2,
	slSantSweep,
	slSantRangeAttack1A,
	slSantRangeAttack1B,
	slSantRangeAttack2,
};

IMPLEMENT_CUSTOM_SCHEDULES( CSaintna, CSquadMonster );

//=========================================================
// SetActivity 
//=========================================================
void CSaintna :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RUN:
		if ( pev->health <= pev->max_health * 0.5 )
		{
			// limp!
			iSequence = LookupActivity ( ACT_RUN_HURT );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_WALK:
		if(FBitSet(pev->effects, EF_DIMLIGHT)){
			iSequence = LookupActivity ( ACT_WALK_SCARED );
		}
		else if ( pev->health <= pev->max_health * 0.5 )
		{
			// limp!
			iSequence = LookupActivity ( ACT_WALK_HURT );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_IDLE:
		if ( m_MonsterState == MONSTERSTATE_COMBAT && m_hEnemy != NULL )
		{
			if(m_hEnemy->IsPlayer()){
			NewActivity = ACT_COMBAT_IDLE;
			}
			else{
			NewActivity = ACT_IDLE_ANGRY;
			}
		}
		else if ( pev->health <= pev->max_health * 0.5 )
		{
			NewActivity = ACT_STAND;
		}
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
Schedule_t *CSaintna :: GetSchedule( void )
{
	// clear old sentence
	m_iSentence = SANT_SENT_NONE;

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
				if ( HasConditions(bits_COND_NEW_ENEMY) )
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
						if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) ){
						return GetScheduleOfType ( SCHED_GRUNT_COVER_AND_RELOAD );
						}
						else{
						return GetScheduleOfType ( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
						}
					}
				}
	// no ammo
				else if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
				{
					//!!!KELLY - this individual just realized he's out of bullet ammo. 
					// He's going to try to find cover to run to and reload, but rarely, if 
					// none is available, he'll drop and reload in the open here. 
					if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
					{
						return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
					}
					else{
						return GetScheduleOfType ( SCHED_GRUNT_COVER_AND_RELOAD );
					}
				}
	// damaged just a little
				else if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
				{
					if (!FBitSet(pev->effects, EF_DIMLIGHT) && m_hEnemy != NULL )
					{
						return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
					}
				}
	// can kick
				else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
				{
					return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
				}
	// can grenade launch
	// can shoot
				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
				{
					if ( OccupySlot ( bits_SLOTS_HGRUNT_ENGAGE ) )
					{
						// try to take an available ENGAGE slot
						return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
					}
					else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
					{
						// throw a grenade if can and no engage slots are available
						return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
					}
					else if (!FBitSet(pev->effects, EF_DIMLIGHT))
					{
						// hide!
						return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
					}
				}
	// can't see enemy
				else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
				{
					if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
					{
						return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
					}
					//else if ( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
					//{
					//	return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
					//}
					else
					{
						return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
					}
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
Schedule_t* CSaintna :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			return &slSantTakeCover[ 0 ];
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slSantTakeCoverFromBestSound[ 0 ];
		}
	case SCHED_GRUNT_TAKECOVER_FAILED:
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}
			else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}
			else{
				return GetScheduleOfType ( SCHED_FAIL );
			}
		}
		break;
	case SCHED_GRUNT_ELOF_FAIL:
		{
			if(m_groundElev){
			return &slSantCombatFace[ 0 ];
			}
			else{
			return GetScheduleOfType ( SCHED_CHASE_ENEMY_FAILED );
			}
		}
		break;
	case SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE:
		{
			if(m_groundElev){
			return &slSantCombatFace[ 0 ];
			}
			else{
			return &slSantEstablishLineOfFire[ 0 ];
			}
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			return &slSantRangeAttack1A[ 0 ];
		}
	case SCHED_RANGE_ATTACK2:
		{
			return &slSantRangeAttack2[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slSantCombatFace[ 0 ];
		}
	case SCHED_GRUNT_WAIT_FACE_ENEMY:
		{
			return &slSantWaitInCover[ 0 ];
		}
	case SCHED_GRUNT_SWEEP:
		{
			return &slSantSweep[ 0 ];
		}
	case SCHED_GRUNT_COVER_AND_RELOAD:
		{
			if (FBitSet(pev->effects, EF_DIMLIGHT)){
				return &slSantHideReload_2[ 0 ];
			}
			else if(m_hEnemy != NULL){
				float flDist = ( m_hEnemy->pev->origin - pev->origin).Length();

				if ( flDist < 384 && fabs( pev->origin.z - m_hEnemy->pev->origin.z ) < 128 && HasConditions ( bits_COND_ENEMY_FACING_ME ) ){
				return &slSantHideReload[ 0 ];
				}
				else{
				return &slSantHideReload_2[ 0 ];
				}
			}
			else{
				return &slSantHideReload_2[ 0 ];
			}
		}
	case SCHED_GRUNT_FOUND_ENEMY:
		{
			return &slSantFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			return &slSantVictoryDance[ 0 ];
		}
	case SCHED_GRUNT_SUPPRESS:
		{
			return &slSantSuppress[ 0 ];
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slSantCombatFail[ 0 ];
			}

			return &slSantFail[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}