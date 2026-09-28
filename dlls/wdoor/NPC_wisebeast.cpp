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
#define	WISE_SENTENCE_VOLUME			(float)0.35 // volume of grunt sentences

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
// monster-specific conditions
//=========================================================
#define bits_COND_GRUNT_NOFIRE	( bits_COND_SPECIAL1 )


class CGrassTip : public CBaseEntity
{
public:
	void Spawn( void );

	void EXPORT Thinking( void );
};

LINK_ENTITY_TO_CLASS( grass_tip, CGrassTip );


void CGrassTip:: Spawn( void )
{
	pev->movetype = MOVETYPE_NONE;
	pev->classname = MAKE_STRING( "grass_tip" );
	
	pev->solid = SOLID_NOT;
	
	SET_MODEL(ENT(pev), "models/grass_tip.mdl");

	UTIL_SetSize( pev, g_vecZero, g_vecZero);

	SetThink ( &CGrassTip::Thinking );
	pev->nextthink = gpGlobals->time + 3.0;

	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0;
	pev->frame = 0;
	pev->sequence = 0;

	SetBits(pev->effects, EF_DIMLIGHT);

//	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "wisebeast/grass_hit.wav", VOL_NORM, ATTN_NORM); 
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "war3/impalehit.wav", VOL_NORM, ATTN_NORM); 
}


void CGrassTip::Thinking( void )
{//ûʲô���õ�����
	SetThink ( NULL );
	UTIL_Remove( this );
	return;

}

class CWiseBeast : public CSquadMonster
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
	BOOL CheckMeleeAttack2 ( float flDot, float flDist );

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
	float m_flNextJumpTime;
	float m_flNextPainTime;
	float m_flNextMeleeTime;
	float m_flLastEnemySightTime;

	float m_flNextFireTime;
	float m_flNextFire2Time;
	float m_flNextFire3Time;
	float m_flNextGrasstip;
	float m_flNanomachineTime;

	float m_dyingtime;

	BOOL	m_fThrowGrenade;
	BOOL	m_fStanding;
	BOOL	m_fFirstEncounter;// only put on the handsign show in the squad's first encounter.

	int		m_voicePitch;
	int		m_iBrassShell;

	int		m_iSentence;

	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;

	static const char *pGruntSentences[];
};

LINK_ENTITY_TO_CLASS( monster_wisebeast, CWiseBeast );

void CWiseBeast :: FollowerUse2( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	
	if ( IsAlive() && pCaller != NULL && pCaller->IsPlayer() )
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

TYPEDESCRIPTION	CWiseBeast::m_SaveData[] = 
{
	DEFINE_FIELD( CWiseBeast, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CWiseBeast, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CWiseBeast, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CWiseBeast, m_voicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( CWiseBeast, m_iSentence, FIELD_INTEGER ),
	DEFINE_FIELD( CWiseBeast, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CWiseBeast, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CWiseBeast, m_flNextFireTime, FIELD_TIME ),
	DEFINE_FIELD( CWiseBeast, m_flNextFire2Time, FIELD_TIME ),
	DEFINE_FIELD( CWiseBeast, m_flNextFire3Time, FIELD_TIME ),
	DEFINE_FIELD( CWiseBeast, m_flNanomachineTime, FIELD_TIME ),
	DEFINE_FIELD( CWiseBeast, m_flNextGrasstip, FIELD_TIME ),
	DEFINE_FIELD( CWiseBeast, m_flNextJumpTime, FIELD_TIME ),
	DEFINE_FIELD( CWiseBeast, m_dyingtime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CWiseBeast, CSquadMonster );

const char *CWiseBeast::pGruntSentences[] = 
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
	WISE_SENT_NONE = -1,
	WISE_SENT_GREN = 0,
	WISE_SENT_ALERT,
	WISE_SENT_MONSTER,
	WISE_SENT_COVER,
	WISE_SENT_THROW,
	WISE_SENT_CHARGE,
	WISE_SENT_TAUNT,
} WISE_SENTENCE_TYPES;

//=========================================================
// IRelationship - overridden because Alien Grunts are 
// Human Grunt's nemesis.
//=========================================================
int CWiseBeast::IRelationship ( CBaseEntity *pTarget )
{
	return CSquadMonster::IRelationship( pTarget );
}


//=========================================================
// RunAI
//=========================================================
void CWiseBeast :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(pev->frags == 1 && !FBitSet( pev->flags, FL_NOTARGET )){//��ʬ
	pev->flags		|= FL_NOTARGET;
	pev->spawnflags |= SF_MONSTER_PRISONER;
	}

	if(pev->skin == 1){
		if(pev->armorvalue > 0){
		pev->armorvalue--;
		}
		else{
		pev->skin = 0;
		}
	}

	if(m_flNPC_Pain > 0){
		if(m_flNPC_Pain > 40)
		m_flNPC_Pain = 40;

		m_flNPC_Pain--;
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
			if(m_hEnemy->IsPlayer() 
			&& (pev->sequence == LookupActivity ( ACT_RANGE_ATTACK1 ) || pev->sequence == LookupActivity ( ACT_MELEE_ATTACK2 )) ){
			SetActivity( ACT_IDLE );//�Ѿ��˺�ֹͣ����
			ClearSchedule();
			}
		}
	}

		if(pev->sequence == LookupActivity ( ACT_RUN )){
		   m_flGroundSpeed = 270;
		}
		else if(pev->sequence == LookupActivity ( ACT_WALK )){
		    m_flGroundSpeed = 120;
		}

	//���ܤν���
	if(m_rpgms_level >= 80 && m_rpgms_skill6_learn == 0){
	m_rpgms_skill6_learn = 49;
	m_rpgms_skill7_learn = 50;
	m_rpgms_skill8_learn = 51;
	pev->frags = 2;//����Ҳ����Ҫ
	}
}


void CWiseBeast::Killed( entvars_t *pevAttacker, int iGib )
{
	if(m_die == 0){
		
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
void CWiseBeast :: GibMonster ( void )
{
	CBaseMonster :: GibMonster();
}

//=========================================================
// ISoundMask - Overidden for human grunts because they 
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CWiseBeast :: ISoundMask ( void )
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
BOOL CWiseBeast :: FCanCheckAttacks ( void )
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
BOOL CWiseBeast :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if (m_flNextFireTime > gpGlobals->time){
	return FALSE;
	}

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

BOOL CWiseBeast :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if (m_flNextJumpTime > gpGlobals->time){
	return FALSE;
	}

	float cover_dist = 256;

	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= cover_dist && flDot >= 0.5)
	{
		pev->flags &= ~FL_ONGROUND;
		UTIL_MakeVectors(pev->angles);
		pev->velocity = gpGlobals->v_forward * -512;
		pev->velocity.z += 256;

		m_flNextJumpTime = gpGlobals->time + 4;
	}

	return FALSE;
}

BOOL CWiseBeast :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	if (m_flNextFire2Time > gpGlobals->time){
	return FALSE;
	}

	float dist = 2000;

	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= dist && flDot >= 0.5)
	{
		return m_lastAttackCheck;
	}

	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack. 
//=========================================================
BOOL CWiseBeast :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if (m_flNextGrasstip > gpGlobals->time){
	return FALSE;
	}

	float dist = 768;

	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= dist && flDot >= 0 && flDist >= 128)
	{
		return TRUE;
	}

	return FALSE;
}


//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
void CWiseBeast :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
int CWiseBeast :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( (bitsDamageType & DMG_ENERGYBEAM) 
	|| (bitsDamageType & DMG_ENERGYBLAST)
	|| (bitsDamageType & DMG_SHOCK)
	|| (bitsDamageType & DMG_FREEZE) ){
		flDamage *= 0.8;//ħ������
	}
	else if ( (bitsDamageType & DMG_BURN)
	|| (bitsDamageType & DMG_DARK)
	|| (bitsDamageType & DMG_SONIC)){
		flDamage *= 0.1;
	}

	if(m_flNanomachineTime < gpGlobals->time && m_rpgms_skill7_learn == 50 && pev->skin == 0
	&& pev->deadflag == DEAD_NO){
	pev->skin = 1;
	pev->armorvalue = 150;
	m_flNanomachineTime = gpGlobals->time + 45;
	}

	if(pev->skin == 1){
	flDamage *= 0.1;
	}

	m_alert	= 100;

	Forget( bits_MEMORY_INCOVER );

	return CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CWiseBeast :: SetYawSpeed ( void )
{
	pev->yaw_speed = 240;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CWiseBeast :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

//=========================================================
// Shoot
//=========================================================
void CWiseBeast :: Shoot ( int post )
{
	if ( m_hEnemy == NULL )
	{
		return;
	}

	Vector vecShootOrigin,vecdir;
	UTIL_MakeVectors(pev->angles);
	GetAttachment( post, vecShootOrigin,vecdir);

	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	if(post == 0){
		if(m_rpgms_skill6_learn == 49 && m_flNextFire3Time < gpGlobals->time){//��˯����
		FireBullets( 1, vecShootOrigin, vecShootDir, Vector(0.01,0.01,0.01), 2048, 814, 0 );
		m_flNextFire3Time = gpGlobals->time + 18;
		FireBeam(vecShootOrigin, vecShootDir, BEAM_IONTURRET, 100, pev);
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/gauss_fire2.wav", 1, 0.4);
		}
		else if(m_flNextFireTime < gpGlobals->time){//���ձ���
		m_flNextFireTime = gpGlobals->time + 3;
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "tank/tank_fire.wav", 1, 0.4);
		FireBeam(vecShootOrigin, vecShootDir, BEAM_BLASTER, 100, pev);
			if(m_rpgms_skill8_learn == 51 && pev->health < pev->max_health * 0.5){//һת����
			FireBullets( 1, vecShootOrigin, vecShootDir, Vector(0.01,0.01,0.01), 2048, 811, 0,114 );
			}
			else{
			FireBullets( 1, vecShootOrigin, vecShootDir, Vector(0.01,0.01,0.01), 2048, 811, 0,514 );
			}
		}
	}
	else if(m_cAmmoLoaded > 0){
		Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
		if(post == 1){
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/m249_fire.wav", 1, 0.6);
		vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(-40,-90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
		}

	m_cAmmoLoaded--;// take away a bullet!
	
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_NULL); 

	if(m_rpgms_skill8_learn == 51 && pev->health < pev->max_health * 0.5){//һת����
	FireBullets(1, vecShootOrigin, vecShootDir, Vector(0.0114,0.0114,0.0114), 2048, BULLET_12MM,1,114); // shoot +-5 degrees
	}
	else{
	FireBullets(1, vecShootOrigin, vecShootDir, Vector(0.0114,0.0114,0.0114), 2048, BULLET_12MM,1); // shoot +-5 degrees
	}

	pev->effects |= EF_MUZZLEFLASH;
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CWiseBeast :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case 1://���ձ���/��˯����
			if (m_flNextFireTime < gpGlobals->time || m_flNextFire3Time < gpGlobals->time)
			{
				Shoot(0);
			}
		break;

		case 2://��Ծ
		{
			if (m_flNextJumpTime < gpGlobals->time)
			{
				pev->flags &= ~FL_ONGROUND;
				UTIL_MakeVectors(pev->angles);
				pev->velocity = gpGlobals->v_forward * -512;
				pev->velocity.z += 256;

				m_flNextJumpTime = gpGlobals->time + 4;
			}
		}
		break;

		case 3:
		{
			if(!m_cAmmoLoaded){//��ҩ�ľ�
			m_flNextFire2Time = gpGlobals->time + 9;
			pev->body = 0;
			ClearSchedule();
			SetYawSpeed();
			}
			else{
			Shoot(1);
			}
		}
		break;

		case 4:
		{
			Shoot(2);
		}
		break;

		case 5://���շ�ӡ
		{
			
		}
		break;

		case 6:
		{
			m_cAmmoLoaded = 114;
			//pev->body = 1;
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

		case 8://����
		{
			if(m_flNextGrasstip <= gpGlobals->time ){
					Vector ice_org;
					ice_org = pev->origin + gpGlobals->v_forward * 64;
					if(m_hEnemy != NULL){
						if(FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND )){
							if(( pev->origin - m_hEnemy->pev->origin).Length() <= 2048){
							ice_org = m_hEnemy->pev->origin;
							if(m_rpgms_skill8_learn == 51 && pev->health < pev->max_health * 0.5){//һת����
							m_hEnemy->TakeDamage ( pev, pev, 135, DMG_CONCUSSION | DMG_NEVERGIB );//����!
							}
							else{
							m_hEnemy->TakeDamage ( pev, pev, 90, DMG_CONCUSSION | DMG_NEVERGIB );//����!
							}
							CBaseEntity::Create( "grass_tip", ice_org, pev->angles, edict() );
							}
						}
					}

					m_flNextGrasstip = gpGlobals->time + 6;
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
void CWiseBeast :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/wisebeast.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;

	pev->health			= 810;//�޵Ф����״�����
	m_lovehate			= 90;

	m_flFieldOfView		 = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		 = MONSTERSTATE_NONE;

	m_flNextFireTime	 = gpGlobals->time + 2;
	m_flNextFire2Time	 = gpGlobals->time + 2;
	m_flNextFire3Time	 = gpGlobals->time + 2;
	m_flNanomachineTime	 = gpGlobals->time + 2;
	m_flNextGrasstip	 = gpGlobals->time + 2;

	m_iSentence			 = WISE_SENT_NONE;

	m_afCapability		= bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 55 );

	m_cClipSize			= 114;
	m_cAmmoLoaded		= 114;
	m_canheadcrab_mode  = 0;
	m_canbarnacle_mode  = 1;

	m_no_victdance		= 1;

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();

	m_follow_mode		= 1;
	m_ignoredamage		= 1;
	m_headdef			= 2;

	m_candrownwater = 1;
//	m_forcefuckdoor = TRUE;
	m_chase_mode = 1;
	m_chase_failed_max = 4;

	SetUse( &CWiseBeast::FollowerUse2 );

	m_aimenemy_mod = 6;

	pev->body = 0;
	pev->skin = 0;

	m_fStanding = TRUE;
	SetTouch( &CWiseBeast::DeadTouch );

	//pev->gravity = 1.2;//������
	m_longming = 1;

	m_rpgms_actor = 13;
	m_rpgms_level = 45;
	m_rpgms_exp = 0;
	m_rpgms_type = 1;

	m_new_ally_type = TRUE;
	pev->netname = MAKE_STRING( "WiseBeast" );

	m_rpgms_skill1_learn = 29;
	m_rpgms_skill2_learn = 30;
	m_rpgms_skill3_learn = 31;//�������
	m_rpgms_skill4_learn = 32;
	m_rpgms_skill5_learn = 38;
	m_rpgms_skill6_learn = 0;//��˯����49
	m_rpgms_skill7_learn = 0;//������50
	m_rpgms_skill8_learn = 0;//һת����51

	pev->takedamage = DAMAGE_YES;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CWiseBeast :: Precache()
{
	PRECACHE_MODEL("models/wisebeast.mdl");
	PRECACHE_MODEL("models/grass_tip.mdl");

	PRECACHE_SOUND("war3/impalehit.wav");
	
	PRECACHE_SOUND("weapons/m249_fire.wav");// because we use the basemonster SWIPE animation event
	PRECACHE_SOUND ("tank/tank_fire.wav");
	PRECACHE_SOUND("weapons/gauss_fire2.wav" );
	m_voicePitch = 100 + RANDOM_LONG(-5,10);

	m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
}	

//=========================================================
// start task
//=========================================================
void CWiseBeast :: StartTask ( Task_t *pTask )
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
void CWiseBeast :: RunTask ( Task_t *pTask )
{
	CSquadMonster :: RunTask( pTask );
}

//=========================================================
// PainSound
//=========================================================
void CWiseBeast :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{

		switch ( RANDOM_LONG(0,1) )
		{
		case 0:	
//			EMIT_SOUND( ENT(pev), 6, "saintna/pain1.wav", 1, 0.7 );	
			break;
		case 1:
//			EMIT_SOUND( ENT(pev), 6, "saintna/pain2.wav", 1, 0.7 );	
			break;
		}

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CWiseBeast :: DeathSound ( void )
{
//	EMIT_SOUND( ENT(pev), 6, "saintna/die1.wav", 1, 0.7 );	
}

//=========================================================
// GruntFail
//=========================================================
Task_t	tlWiseFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)1		},
	{ TASK_FORGET_ENEMY,		(float)0	},
};

Schedule_t	slWiseFail[] =
{
	{
		tlWiseFail,
		ARRAYSIZE ( tlWiseFail ),
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
Task_t	tlWiseCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)1		},
};

Schedule_t	slWiseCombatFail[] =
{
	{
		tlWiseCombatFail,
		ARRAYSIZE ( tlWiseCombatFail ),
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
Task_t	tlWiseVictoryDance[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
};

Schedule_t	slWiseVictoryDance[] =
{
	{ 
		tlWiseVictoryDance,
		ARRAYSIZE ( tlWiseVictoryDance ), 
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
Task_t tlWiseEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_GRUNT_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slWiseEstablishLineOfFire[] =
{
	{ 
		tlWiseEstablishLineOfFire,
		ARRAYSIZE ( tlWiseEstablishLineOfFire ),
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
Task_t	tlWiseFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slWiseFoundEnemy[] =
{
	{ 
		tlWiseFoundEnemy,
		ARRAYSIZE ( tlWiseFoundEnemy ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"GruntFoundEnemy"
	},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t	tlWiseCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)0.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_GRUNT_SWEEP	},
};

Schedule_t	slWiseCombatFace[] =
{
	{ 
		tlWiseCombatFace1,
		ARRAYSIZE ( tlWiseCombatFace1 ), 
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
Task_t	tlWiseSignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
};

Schedule_t	slWiseSignalSuppress[] =
{
	{ 
		tlWiseSignalSuppress,
		ARRAYSIZE ( tlWiseSignalSuppress ), 
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

Task_t	tlWiseSuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slWiseSuppress[] =
{
	{ 
		tlWiseSuppress,
		ARRAYSIZE ( tlWiseSuppress ), 
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
Task_t	tlWiseWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slWiseWaitInCover[] =
{
	{ 
		tlWiseWaitInCover,
		ARRAYSIZE ( tlWiseWaitInCover ), 
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
Task_t	tlWiseTakeCover1[] =
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

Schedule_t	slWiseTakeCover[] =
{
	{ 
		tlWiseTakeCover1,
		ARRAYSIZE ( tlWiseTakeCover1 ), 
		bits_COND_ENEMY_DEAD,
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2,
		"TakeCover"
	},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlWiseGrenadeCover1[] =
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

Schedule_t	slWiseGrenadeCover[] =
{
	{ 
		tlWiseGrenadeCover1,
		ARRAYSIZE ( tlWiseGrenadeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlWiseTossGrenadeCover1[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slWiseTossGrenadeCover[] =
{
	{ 
		tlWiseTossGrenadeCover1,
		ARRAYSIZE ( tlWiseTossGrenadeCover1 ), 
		0,
		0,
		"TossGrenadeCover"
	},
};
//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlWiseTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slWiseTakeCoverFromBestSound[] =
{
	{ 
		tlWiseTakeCoverFromBestSound,
		ARRAYSIZE ( tlWiseTakeCoverFromBestSound ), 
		0,
		0,
		"TakeCoverFromBestSound"
	},
};


//=========================================================
// Grunt reload schedule
//=========================================================
Task_t	tlWiseHideReload[] =
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

Schedule_t slWiseHideReload[] = 
{
	{
		tlWiseHideReload,
		ARRAYSIZE ( tlWiseHideReload ),
		bits_COND_CAN_MELEE_ATTACK1,
		bits_SOUND_DANGER,
		"GruntHideReload"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlWiseSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_FORGET_ENEMY,		(float)0	},
};

Schedule_t	slWiseSweep[] =
{
	{ 
		tlWiseSweep,
		ARRAYSIZE ( tlWiseSweep ), 
		
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
Task_t	tlWiseRangeAttack1A[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
};

Schedule_t	slWiseRangeAttack1A[] =
{
	{ 
		tlWiseRangeAttack1A,
		ARRAYSIZE ( tlWiseRangeAttack1A ), 
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
Task_t	tlWiseRangeAttack1B[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
};

Schedule_t	slWiseRangeAttack1B[] =
{
	{ 
		tlWiseRangeAttack1B,
		ARRAYSIZE ( tlWiseRangeAttack1B ), 
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


DEFINE_CUSTOM_SCHEDULES( CWiseBeast )
{
	slWiseFail,
	slWiseCombatFail,
	slWiseVictoryDance,
	slWiseEstablishLineOfFire,
	slWiseFoundEnemy,
	slWiseCombatFace,
	slWiseSignalSuppress,
	slWiseSuppress,
	slWiseWaitInCover,
	slWiseTakeCover,
	slWiseGrenadeCover,
	slWiseTossGrenadeCover,
	slWiseTakeCoverFromBestSound,
	slWiseHideReload,
	slWiseSweep,
	slWiseRangeAttack1A,
	slWiseRangeAttack1B,
};

IMPLEMENT_CUSTOM_SCHEDULES( CWiseBeast, CSquadMonster );

//=========================================================
// SetActivity 
//=========================================================
void CWiseBeast :: SetActivity ( Activity NewActivity )
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
Schedule_t *CWiseBeast :: GetSchedule( void )
{
	// clear old sentence
	m_iSentence = WISE_SENT_NONE;

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

				if( m_HenemyEnemyMe == 4 && (m_flNPC_Pain || pev->health < pev->max_health * 0.4) ){
					if(m_hEnemy != NULL){
						//if(FClassnameIs(m_hEnemy->pev, "monster_hydra_boss")){
						return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
						//}
					}
				}

				if ( HasConditions(bits_COND_NEW_ENEMY) )
				{
					if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
					{
						return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
					}
					else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) )
					{
						return GetScheduleOfType ( SCHED_RANGE_ATTACK2 );
					}
					else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
					{
						return GetScheduleOfType ( SCHED_GRUNT_SUPPRESS );
					}
					else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK2 ) )
					{
						return GetScheduleOfType ( SCHED_MELEE_ATTACK2 );
					}
					else
					{
						if(m_flNextFireTime > gpGlobals->time){
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
				else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
				{
					return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
				}
				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) )
				{
					return GetScheduleOfType ( SCHED_RANGE_ATTACK2 );
				}
				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
				{
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}
				else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK2 ) )
				{
					return GetScheduleOfType ( SCHED_MELEE_ATTACK2 );
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
									return GetScheduleOfType ( SCHED_COMBAT_FACE );
							}
							else{
									return GetScheduleOfType ( SCHED_COMBAT_FACE );
							}
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
Schedule_t* CWiseBeast :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			return &slWiseTakeCover[ 0 ];
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slWiseTakeCoverFromBestSound[ 0 ];
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
			return &slWiseCombatFace[ 0 ];
			}
			else{
			return GetScheduleOfType ( SCHED_CHASE_ENEMY_FAILED );
			}
		}
		break;
	case SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE:
		{
			if(m_groundElev){
			return &slWiseCombatFace[ 0 ];
			}
			else{
			return &slWiseEstablishLineOfFire[ 0 ];
			}
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			return &slWiseRangeAttack1A[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slWiseCombatFace[ 0 ];
		}
	case SCHED_GRUNT_WAIT_FACE_ENEMY:
		{
			return &slWiseWaitInCover[ 0 ];
		}
	case SCHED_GRUNT_SWEEP:
		{
			return &slWiseSweep[ 0 ];
		}
	case SCHED_GRUNT_COVER_AND_RELOAD:
		{
			return &slWiseHideReload[ 0 ];
		}
	case SCHED_GRUNT_FOUND_ENEMY:
		{
			return &slWiseFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			return &slWiseVictoryDance[ 0 ];
		}
	case SCHED_GRUNT_SUPPRESS:
		{
			return &slWiseSuppress[ 0 ];
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slWiseCombatFail[ 0 ];
			}

			return &slWiseFail[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}