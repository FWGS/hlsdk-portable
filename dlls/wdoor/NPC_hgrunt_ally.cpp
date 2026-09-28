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
#include	"defaultai.h"
#include	"animation.h"
#include	"squadmonster.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"
#include	"player.h"

int g_fGruntAllyQuestion;				// true if an idle grunt asked a question. Cleared when someone answers.

extern DLL_GLOBAL int		g_iSkillLevel;
extern DLL_GLOBAL int		g_gibexp_max;


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
#define	HGRUNT_SENTENCE_VOLUME			(float)0.35 // volume of grunt sentences

#define HGRUNT_9MMAR				( 1 << 0)
#define HGRUNT_HANDGRENADE			( 1 << 1)
#define HGRUNT_GRENADELAUNCHER		( 1 << 2)
#define HGRUNT_SHOTGUN				( 1 << 3)
#define HGRUNT_AK47					( 1 << 4)

#define HEAD_GROUP					1
#define HEAD_GRUNT					0
#define HEAD_COMMANDER				1
#define HEAD_SHOTGUN				2
#define HEAD_M203					3
#define HEAD_CRAB					4

#define GUN_GROUP					2
#define GUN_MP5						0
#define GUN_SHOTGUN					1
#define GUN_AK47					2
#define GUN_NONE					3

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		HGRUNT_AE_RELOAD		( 2 )
#define		HGRUNT_AE_KICK			( 3 )
#define		HGRUNT_AE_BURST1		( 4 )
#define		HGRUNT_AE_BURST2		( 5 ) 
#define		HGRUNT_AE_BURST3		( 6 ) 
#define		HGRUNT_AE_GREN_TOSS		( 7 )
#define		HGRUNT_AE_GREN_LAUNCH	( 8 )
#define		HGRUNT_AE_GREN_DROP		( 9 )
#define		HGRUNT_AE_CAUGHT_ENEMY	( 10) // grunt established sight with an enemy (player only) that had previously eluded the squad.
#define		HGRUNT_AE_DROP_GUN		( 11) // grunt (probably dead) is dropping his mp5.

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
	SCHED_GRUNT_REPEL,
	SCHED_GRUNT_REPEL_ATTACK,
	SCHED_GRUNT_REPEL_LAND,
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

class CHGruntAlly : public CSquadMonster
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
	Vector GetGunPosition( void );
	void Shoot ( void );
	void Shotgun ( void );
	void PrescheduleThink ( void );
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
	float m_flNextGrenadeCheck;
	float m_flLastEnemySightTime;

	Vector	m_vecTossVelocity;

	BOOL	m_fThrowGrenade;
	BOOL	m_fStanding;
	BOOL	m_fFirstEncounter;// only put on the handsign show in the squad's first encounter.

	int m_voicePitch;
	BOOL	m_fGunDrawn;
	int		m_iBrassShell;
	int		m_iShotgunShell;

	int		m_iSentence;

	float   m_flKillBeamTime;
	float   m_flSkillTime;
	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;

	static const char *pGruntSentences[];
};

LINK_ENTITY_TO_CLASS( monster_human_grunt_ally, CHGruntAlly );

void CHGruntAlly :: FollowerUse2( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
		if ( IsAlive() && pCaller != NULL && pCaller->IsPlayer() && m_lovehate > 0 && m_selfmode == FALSE )
		{
			ClearSchedule();
			m_alert	= 100;

			if(m_hEnemy != NULL){
			m_hEnemy = NULL;
			m_hOldEnemy[0] = NULL;
			m_hOldEnemy[1] = NULL;
			m_hOldEnemy[2] = NULL;
			m_hOldEnemy[3] = NULL;
			}

			if ( IsAlive() && m_enemyfollower == 0 )
			{
					if(m_rpgms_type >= 1){
						CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pCaller->pev);
						if(pPlayer){
							if(pPlayer->HasTeamMate_CanAdd(this)){
							pPlayer->TeamMate_add(this);//�����ڶ����У�������Ҥζ���
							m_enemyfollower = 1;
							}
						}
					}
			}
			else if(pev->frags == 0)
			{
				CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pCaller->pev);
				if(pPlayer){//���������
				pPlayer->TeamMate_remove(this);
				}

				m_enemyfollower = 0;
				m_enemyfollower_combat = 0;
			}
		}
}

TYPEDESCRIPTION	CHGruntAlly::m_SaveData[] = 
{
	DEFINE_FIELD( CHGruntAlly, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHGruntAlly, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CHGruntAlly, m_flKillBeamTime, FIELD_TIME ),
	DEFINE_FIELD( CHGruntAlly, m_flSkillTime, FIELD_TIME ),
//	DEFINE_FIELD( CHGruntAlly, m_flLastEnemySightTime, FIELD_TIME ), // don't save, go to zero
	DEFINE_FIELD( CHGruntAlly, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CHGruntAlly, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHGruntAlly, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHGruntAlly, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHGruntAlly, m_voicePitch, FIELD_INTEGER ),
//  DEFINE_FIELD( CShotgun, m_iBrassShell, FIELD_INTEGER ),
//  DEFINE_FIELD( CShotgun, m_iShotgunShell, FIELD_INTEGER ),
	DEFINE_FIELD( CHGruntAlly, m_iSentence, FIELD_INTEGER ),
	DEFINE_FIELD( CHGruntAlly, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CHGruntAlly, m_lastAttackCheck, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CHGruntAlly, CSquadMonster );

const char *CHGruntAlly::pGruntSentences[] = 
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
	HGRUNT_SENT_NONE = -1,
	HGRUNT_SENT_GREN = 0,
	HGRUNT_SENT_ALERT,
	HGRUNT_SENT_MONSTER,
	HGRUNT_SENT_COVER,
	HGRUNT_SENT_THROW,
	HGRUNT_SENT_CHARGE,
	HGRUNT_SENT_TAUNT,
} HGRUNT_ALLY_SENTENCE_TYPES;

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
void CHGruntAlly :: SpeakSentence( void )
{
	if ( m_iSentence == HGRUNT_SENT_NONE )
	{
		// no sentence cued up.
		return; 
	}
}

//=========================================================
// IRelationship - overridden because Alien Grunts are 
// Human Grunt's nemesis.
//=========================================================
int CHGruntAlly::IRelationship ( CBaseEntity *pTarget )
{
	if ( FClassnameIs ( pTarget->pev, "monster_scihead_boss" ) )
	{
		return R_HT;
	}
	return CSquadMonster::IRelationship( pTarget );
}


//=========================================================
// RunAI
//=========================================================
void CHGruntAlly :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(pev->sequence == LookupActivity ( ACT_RUN )){
	m_flGroundSpeed = 280;
	}
	else if(pev->sequence == LookupActivity ( ACT_RUN_HURT )){
	m_flGroundSpeed = 220;
	}

	if(pev->sequence == LookupActivity ( ACT_COWER ) || pev->sequence == LookupActivity ( ACT_RELOAD )){
	m_duckseq = 2;
	}
	else if(pev->sequence == LookupActivity ( ACT_RUN_HURT ) ){
	m_duckseq = 1;
	}
	else if(m_duckseq != 0){
	m_duckseq = 0;
	}

	if(pev->sequence == LookupActivity ( ACT_WALK )){
	m_flGroundSpeed = 80;
	}
	else if(pev->sequence == LookupActivity ( ACT_WALK_HURT )){
	m_flGroundSpeed = 70;
	}

	if(m_flKillBeamTime > 0 && m_flKillBeamTime < gpGlobals->time){
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_KILLBEAM );
	WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
	MESSAGE_END();
	m_flKillBeamTime = 0;
	}

	if(m_makerspawn_call == 1){
		if (FBitSet( pev->weapons, HGRUNT_SHOTGUN ))
		{
			SetBodygroup( GUN_GROUP, GUN_SHOTGUN );
			m_cClipSize		= 8;
		}
		if ( FBitSet( pev->weapons, HGRUNT_AK47 ))
		{
			SetBodygroup( GUN_GROUP, GUN_AK47 );
			m_cClipSize		= 30;
		}
		else if ( FBitSet( pev->weapons, HGRUNT_9MMAR ))
		{
			SetBodygroup( GUN_GROUP, GUN_MP5 );
			m_cClipSize		= 36;
		}

		m_makerspawn_call = 0;
	}

	if ( pev->skin < 0 )
	{
		pev->skin = RANDOM_LONG(0,4);
	}
}


void CHGruntAlly::Killed( entvars_t *pevAttacker, int iGib )
{
	
	Vector	vecGunPos;
	Vector	vecGunAngles;

	if ( GetBodygroup( GUN_GROUP ) != GUN_NONE && g_gibexp_max < 120 && !m_undropgun)
	{// ����װ���Ƚ϶ࣿ
		GetAttachment( 0, vecGunPos, vecGunAngles );
		SetBodygroup( GUN_GROUP, GUN_NONE );
		CBaseEntity *pGun;

			if (FBitSet( pev->weapons, HGRUNT_SHOTGUN ))
			{
				pGun = DropItem( "weapon_shotgun", vecGunPos, vecGunAngles );
			}
			else if (FBitSet( pev->weapons, HGRUNT_AK47 ))
			{
				pGun = DropItem( "weapon_ak47", vecGunPos, vecGunAngles );
			}
			else
			{
				pGun = DropItem( "weapon_9mmAR", vecGunPos, vecGunAngles );
			}
			if ( pGun )
			{
				g_gibexp_max += 1;

				pGun->pev->owner = ENT( pev );
				pGun->pev->velocity = pev->velocity + Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
				pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
			}
			if(m_diefadeout == 1){
			pGun->pev->armorvalue = 100;
			}

		

			if (FBitSet( pev->weapons, HGRUNT_GRENADELAUNCHER ))
			{
				CBaseEntity *pGrenade;
				pGrenade = DropItem( "ammo_mp5grenades", vecGunPos, vecGunAngles );
				if ( pGrenade )
				{
					g_gibexp_max += 1;

					pGrenade->pev->owner = ENT( pev );
					pGrenade->pev->velocity = pev->velocity + Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
					pGrenade->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
				}
				if(m_diefadeout == 1){
				pGrenade->pev->armorvalue = 100;
				}
			}
			else if (FBitSet( pev->weapons, HGRUNT_HANDGRENADE ))
			{
				CBaseEntity *pGrenade;
				pGrenade = DropItem( "weapon_handgrenade", vecGunPos, vecGunAngles );
				if ( pGrenade )
				{
					g_gibexp_max += 1;

					pGrenade->pev->owner = ENT( pev );
					pGrenade->pev->velocity = pev->velocity + Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
					pGrenade->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
				}
				if(m_diefadeout == 1){
				pGrenade->pev->armorvalue = 100;
				}
			}

	}

	SetUse( NULL );	

	if ( (pevAttacker->flags & FL_CLIENT) && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if(m_die == 0){
			CBaseEntity *pEntity = GetClassPtr((CBaseEntity *)pevAttacker);
			if(pEntity){
			Alert_Clients(pEntity);
			}
		}
	}

	CSquadMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CHGruntAlly :: GibMonster ( void )
{
	CBaseMonster :: GibMonster();
}

//=========================================================
// ISoundMask - Overidden for human grunts because they 
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CHGruntAlly :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER	|
			bits_SOUND_DANGER;
}

//=========================================================
// someone else is talking - don't speak
//=========================================================
BOOL CHGruntAlly :: FOkToSpeak( void )
{
	return FALSE;
}

//=========================================================
//=========================================================
void CHGruntAlly :: JustSpoke( void )
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
	m_iSentence = HGRUNT_SENT_NONE;
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void CHGruntAlly :: PrescheduleThink ( void )
{
	if ( InSquad() && m_hEnemy != NULL )
	{
		if ( HasConditions ( bits_COND_SEE_ENEMY ) )
		{
			// update the squad's last enemy sighting time.
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
		}
		else
		{
			if ( gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5 )
			{
				// been a while since we've seen the enemy
				MySquadLeader()->m_fEnemyEluded = TRUE;
			}
		}
	}
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
BOOL CHGruntAlly :: FCanCheckAttacks ( void )
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
BOOL CHGruntAlly :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if(m_flSkillTime > gpGlobals->time){
	return FALSE;
	}

	CBaseMonster *pEnemy;

	if ( m_hEnemy != NULL )
	{
		pEnemy = m_hEnemy->MyMonsterPointer();

		if ( !pEnemy )
		{
			return FALSE;
		}
	}

	if ( flDist <= 80 && flDot >= 0.6	&& 
		 pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		 pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON )
	{

		return TRUE;
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
BOOL CHGruntAlly :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( m_cAmmoLoaded <= 0 ){
	return FALSE;
	}

			float dist = 1024;

			if (FBitSet( pev->weapons, HGRUNT_AK47 )){
			dist = 1280;
			}

		if (FBitSet( pev->weapons, HGRUNT_SHOTGUN )){
				dist = 768;

					if(m_aimenemy_mod != 5 && flDist < (dist * 0.25) ){//������
					m_aimenemy_mod = 5;
					}
					else if(m_aimenemy_mod != 1 && flDist < (dist * 0.5) ){//�о���
					m_aimenemy_mod = 1;
					}
					else if(m_aimenemy_mod != 9 && flDist < (dist * 0.75) ){//��Զ����
					m_aimenemy_mod = 9;
					}
					else if(m_aimenemy_mod != 6){//Զ����
					m_aimenemy_mod = 6;
					}
		}
		else{
				if(m_aimenemy_mod != 3 && flDist < (dist * 0.25) ){//������
				m_aimenemy_mod = 3;
				}
				else if(m_aimenemy_mod != 5 && flDist < (dist * 0.5) ){//�о���
				m_aimenemy_mod = 5;
				}
				else if(m_aimenemy_mod != 1 && flDist < (dist * 0.75) ){//��Զ����
				m_aimenemy_mod = 1;
				}
				else if(m_aimenemy_mod != 9){//Զ����
				m_aimenemy_mod = 9;
				}
		}

		if(m_hEnemy != NULL){
			if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 256 ){//վ�ڸߴ������ӹ�������
			dist += 256;
			}
		}

		if(m_guard_mode == TRUE){
		dist += 256;
		}

			if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= dist && flDot >= 0.5)
			{
				TraceResult	tr;

			//���˼��
				if ( gpGlobals->time > m_checkAttackTime )
				{
					Vector vecShootOrigin = pev->origin + Vector(0,0,60);
					TraceResult tr;

					Vector shootOrigin = vecShootOrigin;
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
						m_checkAttackTime = gpGlobals->time + 0.25;
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

				Vector vecSrc,vecdir;
				vecSrc = pev->origin + Vector(0,0,60);

				// verify that a bullet fired from the gun will hit the enemy before the world.
				UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);

				if ( tr.flFraction == 1.0 )
				{
					return TRUE;
				}
			}


	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack. 
//=========================================================
BOOL CHGruntAlly :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if (! FBitSet(pev->weapons, (HGRUNT_HANDGRENADE | HGRUNT_GRENADELAUNCHER)))
	{
		return FALSE;
	}

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

	if (FBitSet( pev->weapons, HGRUNT_HANDGRENADE))
	{
		vecTarget = m_vecEnemyLKP;
		// vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
		// estimate position
		// vecTarget = vecTarget + m_hEnemy->pev->velocity * 2;
	}
	else
	{
		// find target
		// vecTarget = m_hEnemy->BodyTarget( pev->origin );
		vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget_o( pev->origin ) - m_hEnemy->pev->origin);
		// estimate position
		if (HasConditions( bits_COND_SEE_ENEMY))
			vecTarget = vecTarget + ((vecTarget - pev->origin).Length() / 800) * m_hEnemy->pev->velocity;
	}

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
	else if ( ( vecTarget - pev->origin ).Length2D() >= 1536 )
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}
		
	if (FBitSet( pev->weapons, HGRUNT_HANDGRENADE))
	{
		Vector vecToss = VecCheckToss( pev, GetGunPosition(), vecTarget, 0.5 );

		if ( vecToss != g_vecZero )
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}
	else
	{
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
	}

	

	return m_fThrowGrenade;
}


//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
void CHGruntAlly :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
int CHGruntAlly :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( pev->takedamage && pev->deadflag == DEAD_NO && 
	((bitsDamageType & DMG_SLASH) || (bitsDamageType & DMG_CLUB)) ){
	//���ˣ������˺����벢����
			flDamage *= 0.5;
			if (pevAttacker)
			{
				CBaseEntity *pEntity = GetClassPtr((CBaseEntity *)pevAttacker);

				if(pEntity->pev->deadflag == DEAD_NO && pEntity->pev->takedamage){
				pEntity->TakeDamage ( pev, pev, flDamage, DMG_GENERIC | DMG_NEVERGIB );//����!
				}
			}
	}

	m_alert = 100;

	Forget( bits_MEMORY_INCOVER );

	return CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHGruntAlly :: SetYawSpeed ( void )
{
	pev->yaw_speed = 180;
}

//=========================================================
// CheckAmmo - overridden for the grunt because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CHGruntAlly :: CheckAmmo ( void )
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
int	CHGruntAlly :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

//=========================================================
//=========================================================
CBaseEntity *CHGruntAlly :: Kick( void )
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

Vector CHGruntAlly :: GetGunPosition( )
{
	return pev->origin + Vector( 0, 0, 60 );
}

//=========================================================
// Shoot
//=========================================================
void CHGruntAlly :: Shoot ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	return;

	Vector vecShootOrigin;
	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector(0,0,60);

	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_NULL); 

	if ( FBitSet( pev->weapons, HGRUNT_AK47 ))
	{
	FireBullets(1, vecShootOrigin, vecShootDir, Vector(0.03,0.03,0.03), 2560, BULLET_556,1);
	}
	else{
	FireBullets(1, vecShootOrigin, vecShootDir, Vector(0.05,0.05,0.05), 2048, BULLET_MONSTER_12MM,1); // shoot +-5 degrees
	}

	pev->effects |= EF_MUZZLEFLASH;
	
	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// Shoot
//=========================================================
void CHGruntAlly :: Shotgun ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	return;

	Vector vecShootOrigin,vecdir;
	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector(0,0,60);

	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShotgunShell, TE_BOUNCE_SHOTSHELL); 

	FireBullets(5, vecShootOrigin, vecShootDir, Vector(0.08,0.08,0.08), 2048, BULLET_12MM,1); // shoot +-7.5 degrees

	pev->effects |= EF_MUZZLEFLASH;
	
	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHGruntAlly :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case HGRUNT_AE_DROP_GUN:
			{
			
			}
			break;

		case HGRUNT_AE_RELOAD:
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "hgrunt/gr_reload1.wav", 1, ATTN_NORM );
			m_cAmmoLoaded = m_cClipSize;
			ClearConditions(bits_COND_NO_AMMO_LOADED);
			break;

		case HGRUNT_AE_GREN_TOSS:
		{
			if(m_listenlong < 20)
			m_listenlong = 20;

			UTIL_MakeVectors( pev->angles );
			// CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 34 + Vector (0, 0, 32), m_vecTossVelocity, 3.5 );
			CGrenade::ShootTimed( pev, GetGunPosition(), m_vecTossVelocity, 3.5 );

			m_fThrowGrenade = FALSE;
			m_flNextGrenadeCheck = gpGlobals->time + 5;
		}
		break;

		case HGRUNT_AE_GREN_LAUNCH:
		{
			if(m_listenlong < 20)
			m_listenlong = 20;

			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/glauncher.wav", 0.8, ATTN_NORM);
			CGrenade::ShootContact( pev, GetGunPosition(), m_vecTossVelocity );
			m_fThrowGrenade = FALSE;
			m_flNextGrenadeCheck = gpGlobals->time + 5;
		}
		break;

		case HGRUNT_AE_GREN_DROP:
		{
			UTIL_MakeVectors( pev->angles );
			CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27 + gpGlobals->v_up * 6, g_vecZero, 3 );
		}
		break;

		case HGRUNT_AE_BURST1:
		{
			if(m_hEnemy != NULL){//����BUG
				if(m_enemyfollower == 1 && m_lovehate > 0){
					if(m_hEnemy->IsPlayer()){
					SetActivity( ACT_IDLE );//�Ѿ��˺�ֹͣ����
					ClearSchedule();
					return;
					}
				}
			}

			if ( FBitSet( pev->weapons, HGRUNT_AK47 ))
			{
				Shoot();
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/ak47-fire.wav", 1, ATTN_NORM );

			}
			else if ( FBitSet( pev->weapons, HGRUNT_9MMAR ))
			{
				Shoot();
					// the first round of the three round burst plays the sound and puts a sound in the world sound list.
					if ( RANDOM_LONG(0,1) )
					{
						EMIT_SOUND( ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun1.wav", 1, ATTN_NORM );
					}
					else
					{
						EMIT_SOUND( ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun2.wav", 1, ATTN_NORM );
					}

			}
			else if ( FBitSet( pev->weapons, HGRUNT_SHOTGUN ))
			{
				Shotgun( );
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/sbarrel1.wav", 1, ATTN_NORM );
			}
		
			CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
		}
		break;

		case HGRUNT_AE_BURST2:
			{
				if ( FBitSet( pev->weapons, HGRUNT_AK47 )){
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/ak47-fire.wav", 1, ATTN_NORM );
				}
				Shoot();
			}
			break;

		case HGRUNT_AE_BURST3:
		{
			Shoot();
		}
			break;

		case 3://��ȭ
		{
			if(m_flSkillTime < gpGlobals->time){
				if(m_hEnemy != NULL){
					TraceResult tr;
					UTIL_MakeVectors(pev->angles);
					
					Vector vecSrc	= BodyTarget( pev->origin );
					Vector vecEnd	= m_hEnemy->Center();
					UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
					CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

					if ( tr.flFraction < 1.0 ){
						if(( vecSrc - tr.vecEndPos).Length() <= 90){
						int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
						int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
						FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

						if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) )
						{
							if(pEntity->pev->gravity <= 1.5){
							pEntity->pev->velocity = pEntity->pev->velocity + (pEntity->pev->origin - pev->origin).Normalize() * 400;
							}
						}

						ClearMultiDamage( );
						pEntity->TraceAttack(pev, 80, gpGlobals->v_forward, &tr, DMG_CRUSH); 

						if ( (pEntity->pev->flags & FL_MONSTER) )
						{
							if(pEntity->pev->gravity <= 1.5){
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pEntity->MyMonsterPointer();
								if(pEnemyMonster){
								pEnemyMonster->Freeze_Monster(10);
								}
							}
						}

						ApplyMultiDamage( pev, pev );
						EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/fist_hearvy_hit1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
						}
					}

					m_flSkillTime = gpGlobals->time + 3.0;
				}
			}

		}
		break;

		case 15:
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail);	// model
			WRITE_BYTE( 4 ); // life
			WRITE_BYTE( 4 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 255 );	// G
			WRITE_BYTE( 255 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			m_flKillBeamTime = gpGlobals->time + 1.0;
		}
		break;

		case 10:
		{
		RouteClear();
		}
		break;

		case 12:
		{
			Vector vecShootOrigin,vecdir;
			UTIL_MakeVectors(pev->angles);
			GetAttachment( 1, vecShootOrigin,vecdir);

			Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

			Vector angDir = UTIL_VecToAngles( vecShootDir );
			SetBlending( 0, angDir.x );
		}
		break;

		case 13:
		{
		m_fGunDrawn = TRUE;
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
void CHGruntAlly :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/hgrunt_ally.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	
	pev->health			= 240;//�ֹ��ǿ׳���壬Ѫ�غ�
	m_lovehate			= 60;
	
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	m_flSkillTime		= gpGlobals->time + 2;

	m_flNextGrenadeCheck = gpGlobals->time + 2;
	m_flKillBeamTime	= gpGlobals->time;
	m_iSentence			= HGRUNT_SENT_NONE;

	m_afCapability		= bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 60 );

	m_fGunDrawn = FALSE;

	if ( pev->skin < 0 )
	{
		pev->skin = RANDOM_LONG(0,4);
	}

	if (pev->weapons <= 0)
	{
		int rd_num = RANDOM_LONG(0,3);
		if(rd_num == 0){
		pev->weapons = HGRUNT_9MMAR | HGRUNT_HANDGRENADE;
		}
		else if(rd_num == 1){
		pev->weapons = HGRUNT_SHOTGUN | HGRUNT_HANDGRENADE;
		}
		else if(rd_num == 2){
		pev->weapons = HGRUNT_9MMAR | HGRUNT_GRENADELAUNCHER;
		}
		else if(rd_num == 3){
		pev->weapons = HGRUNT_AK47 | HGRUNT_HANDGRENADE;
		}
	}

	pev->body = 0;
	if (FBitSet( pev->weapons, HGRUNT_SHOTGUN ))
	{
		SetBodygroup( GUN_GROUP, GUN_SHOTGUN );
		m_cClipSize		= 8;
	}
	if ( FBitSet( pev->weapons, HGRUNT_AK47 ))
	{
		SetBodygroup( GUN_GROUP, GUN_AK47 );
		m_cClipSize		= 30;
	}
	else if ( FBitSet( pev->weapons, HGRUNT_9MMAR ))
	{
		SetBodygroup( GUN_GROUP, GUN_MP5 );
		m_cClipSize		= 36;
	}

	m_cAmmoLoaded		= m_cClipSize;
	m_canheadcrab_mode  = 0;
	m_canbarnacle_mode  = 1;

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();

	m_follow_mode = 1;

	m_aimenemy_mod = 5;
	m_headdef = 2;//��Ӳͷ��

	SetUse( &CHGruntAlly::FollowerUse2 );
	m_chase_mode = 1;
	m_chase_failed_max = 2;
	m_ignoredamage = 1;
	m_MoveFail_SimpleRoad = TRUE;

	m_candrownwater = 1;
	m_forcefuckdoor  = TRUE;
	SetTouch( &CHGruntAlly::DeadTouch );
	m_killed_exp = 200;

	m_rpgms_actor = 11;
	m_rpgms_level = 18;
	m_rpgms_exp = 0;
	m_rpgms_type = 1;

	m_new_ally_type = TRUE;
	pev->netname = MAKE_STRING( "Aniki.Grunt" );

	m_rpgms_skill1_learn = 19;//����
	m_rpgms_skill2_learn = 82;//��ɱȭ
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHGruntAlly :: Precache()
{
	PRECACHE_MODEL("models/hgrunt_ally.mdl");

	PRECACHE_SOUND( "hgrunt/gr_mgun1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_mgun2.wav" );

	PRECACHE_SOUND("newadd/fist_hearvy_hit1.wav");

//	PRECACHE_SOUND( "hgrunt/gr_die4.wav" );//��

//	PRECACHE_SOUND( "hgrunt/gr_pain1.wav" );
//	PRECACHE_SOUND( "hgrunt/gr_pain2.wav" );
//	PRECACHE_SOUND( "hgrunt/gr_pain3.wav" );
//	PRECACHE_SOUND( "hgrunt/gr_pain4.wav" );
//	PRECACHE_SOUND( "hgrunt/gr_pain5.wav" );

	PRECACHE_SOUND( "hgrunt/gr_reload1.wav" );

	PRECACHE_SOUND( "weapons/glauncher.wav" );
	PRECACHE_SOUND ("weapons/ak47-fire.wav");
	PRECACHE_SOUND( "weapons/sbarrel1.wav" );

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	m_voicePitch = 100 + RANDOM_LONG(-5,10);

	UTIL_PrecacheOther( "monster_lelite" );

	m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
	m_iShotgunShell = PRECACHE_MODEL ("models/shotgunshell.mdl");
}	

//=========================================================
// start task
//=========================================================
void CHGruntAlly :: StartTask ( Task_t *pTask )
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
		TaskComplete();
		break;

	case TASK_GRUNT_CHECK_FIRE:
		if ( !m_lastAttackCheck )
		{
			SetConditions( bits_COND_GRUNT_NOFIRE );
		}
		else if ( !NoFriendlyFire() )
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
void CHGruntAlly :: RunTask ( Task_t *pTask )
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
void CHGruntAlly :: DeathSound ( void )
{
//	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_die4.wav", 1, ATTN_NORM );	
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlGruntAllyFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slGruntAllyFaceTarget[] =
{
	{
		tlGruntAllyFaceTarget,
		ARRAYSIZE ( tlGruntAllyFaceTarget ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_SEE_ENEMY |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};

Task_t	tlGruntAllyFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)100		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slGruntAllyFollow[] =
{
	{
		tlGruntAllyFollow,
		ARRAYSIZE ( tlGruntAllyFollow ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_SEE_ENEMY |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow Grunt"
	},
};

//=========================================================
// GruntFail
//=========================================================
Task_t	tlGruntAllyFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)1		},
	{ TASK_FORGET_ENEMY,		(float)0	},
};

Schedule_t	slGruntAllyFail[] =
{
	{
		tlGruntAllyFail,
		ARRAYSIZE ( tlGruntAllyFail ),
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
Task_t	tlGruntAllyCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)1		},
};

Schedule_t	slGruntAllyCombatFail[] =
{
	{
		tlGruntAllyCombatFail,
		ARRAYSIZE ( tlGruntAllyCombatFail ),
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
Task_t	tlGruntAllyVictoryDance[] =
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

Schedule_t	slGruntAllyVictoryDance[] =
{
	{ 
		tlGruntAllyVictoryDance,
		ARRAYSIZE ( tlGruntAllyVictoryDance ), 
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
Task_t tlGruntAllyEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_GRUNT_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	//{ TASK_GRUNT_SPEAK_SENTENCE,(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slGruntAllyEstablishLineOfFire[] =
{
	{ 
		tlGruntAllyEstablishLineOfFire,
		ARRAYSIZE ( tlGruntAllyEstablishLineOfFire ),
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

Task_t tlGruntAllyEstablishLineOfFire_strap[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_GRUNT_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_STRAFE_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slGruntAllyEstablishLineOfFire_strap[] =
{
	{ 
		tlGruntAllyEstablishLineOfFire_strap,
		ARRAYSIZE ( tlGruntAllyEstablishLineOfFire_strap ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"GruntEstablishLineOfFire Strap"
	},
};

//=========================================================
// GruntFoundEnemy - grunt established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t	tlGruntAllyFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slGruntAllyFoundEnemy[] =
{
	{ 
		tlGruntAllyFoundEnemy,
		ARRAYSIZE ( tlGruntAllyFoundEnemy ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"GruntFoundEnemy"
	},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t	tlGruntAllyCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)1.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_GRUNT_SWEEP	},
};

Schedule_t	slGruntAllyCombatFace[] =
{
	{ 
		tlGruntAllyCombatFace1,
		ARRAYSIZE ( tlGruntAllyCombatFace1 ), 
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Combat Face"
	},
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or grunt gets hurt.
//=========================================================
Task_t	tlGruntAllySignalSuppress[] =
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

Schedule_t	slGruntAllySignalSuppress[] =
{
	{ 
		tlGruntAllySignalSuppress,
		ARRAYSIZE ( tlGruntAllySignalSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"SignalSuppress"
	},
};

Task_t	tlGruntAllySuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slGruntAllySuppress[] =
{
	{ 
		tlGruntAllySuppress,
		ARRAYSIZE ( tlGruntAllySuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_MELEE_ATTACK1 |
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
Task_t	tlGruntAllyWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slGruntAllyWaitInCover[] =
{
	{ 
		tlGruntAllyWaitInCover,
		ARRAYSIZE ( tlGruntAllyWaitInCover ), 
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
Task_t	tlGruntAllyTakeCover1[] =
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

Schedule_t	slGruntAllyTakeCover[] =
{
	{ 
		tlGruntAllyTakeCover1,
		ARRAYSIZE ( tlGruntAllyTakeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlGruntAllyGrenadeCover1[] =
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

Schedule_t	slGruntAllyGrenadeCover[] =
{
	{ 
		tlGruntAllyGrenadeCover1,
		ARRAYSIZE ( tlGruntAllyGrenadeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlGruntAllyTossGrenadeCover1[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slGruntAllyTossGrenadeCover[] =
{
	{ 
		tlGruntAllyTossGrenadeCover1,
		ARRAYSIZE ( tlGruntAllyTossGrenadeCover1 ), 
		0,
		0,
		"TossGrenadeCover"
	},
};
//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlGruntAllyTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slGruntAllyTakeCoverFromBestSound[] =
{
	{ 
		tlGruntAllyTakeCoverFromBestSound,
		ARRAYSIZE ( tlGruntAllyTakeCoverFromBestSound ), 
		0,
		0,
		"TakeCoverFromBestSound"
	},
};

Task_t	tlGruntAllyTakeCoverFromBestSound2[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_LONGMING		},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};



Schedule_t	slGruntAllyTakeCoverFromBestSound2[] =
{
	{ 
		tlGruntAllyTakeCoverFromBestSound2,
		ARRAYSIZE ( tlGruntAllyTakeCoverFromBestSound2 ), 
		0,
		0,
		"TakeCoverFromBestSound2"
	},
};

//=========================================================
// Grunt reload schedule
//=========================================================
Task_t	tlGruntAllyHideReload[] =
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

Schedule_t slGruntAllyHideReload[] = 
{
	{
		tlGruntAllyHideReload,
		ARRAYSIZE ( tlGruntAllyHideReload ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"GruntHideReload"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlGruntAllySweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_FORGET_ENEMY,		(float)0	},
};

Schedule_t	slGruntAllySweep[] =
{
	{ 
		tlGruntAllySweep,
		ARRAYSIZE ( tlGruntAllySweep ), 
		
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
Task_t	tlGruntAllyRangeAttack1A[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
};

Schedule_t	slGruntAllyRangeAttack1A[] =
{
	{ 
		tlGruntAllyRangeAttack1A,
		ARRAYSIZE ( tlGruntAllyRangeAttack1A ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_MELEE_ATTACK1 |
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
Task_t	tlGruntAllyRangeAttack1B[] =
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

Schedule_t	slGruntAllyRangeAttack1B[] =
{
	{ 
		tlGruntAllyRangeAttack1B,
		ARRAYSIZE ( tlGruntAllyRangeAttack1B ), 
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
Task_t	tlGruntAllyRangeAttack2[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_GRUNT_FACE_TOSS_DIR,		(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RANGE_ATTACK2	},
	{ TASK_SET_SCHEDULE,			(float)SCHED_GRUNT_WAIT_FACE_ENEMY	},// don't run immediately after throwing grenade.
};

Schedule_t	slGruntAllyRangeAttack2[] =
{
	{ 
		tlGruntAllyRangeAttack2,
		ARRAYSIZE ( tlGruntAllyRangeAttack2 ), 
		0,
		0,
		"RangeAttack2"
	},
};


//=========================================================
// repel 
//=========================================================
Task_t	tlGruntAllyRepel[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_GLIDE 	},
};

Schedule_t	slGruntAllyRepel[] =
{
	{ 
		tlGruntAllyRepel,
		ARRAYSIZE ( tlGruntAllyRepel ), 
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_PLAYER, 
		"Repel"
	},
};


//=========================================================
// repel 
//=========================================================
Task_t	tlGruntAllyRepelAttack[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_FLY 	},
};

Schedule_t	slGruntAllyRepelAttack[] =
{
	{ 
		tlGruntAllyRepelAttack,
		ARRAYSIZE ( tlGruntAllyRepelAttack ), 
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Repel Attack"
	},
};

//=========================================================
// repel land
//=========================================================
Task_t	tlGruntAllyRepelLand[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_LAND	},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_RUN_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t	slGruntAllyRepelLand[] =
{
	{ 
		tlGruntAllyRepelLand,
		ARRAYSIZE ( tlGruntAllyRepelLand ), 
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_PLAYER, 
		"Repel Land"
	},
};


DEFINE_CUSTOM_SCHEDULES( CHGruntAlly )
{
	slGruntAllyFail,
	slGruntAllyCombatFail,
	slGruntAllyVictoryDance,
	slGruntAllyEstablishLineOfFire,
	slGruntAllyFoundEnemy,
	slGruntAllyCombatFace,
	slGruntAllySignalSuppress,
	slGruntAllySuppress,
	slGruntAllyWaitInCover,
	slGruntAllyTakeCover,
	slGruntAllyGrenadeCover,
	slGruntAllyTossGrenadeCover,
	slGruntAllyTakeCoverFromBestSound,
	slGruntAllyTakeCoverFromBestSound2,
	slGruntAllyHideReload,
	slGruntAllySweep,
	slGruntAllyRangeAttack1A,
	slGruntAllyRangeAttack1B,
	slGruntAllyRangeAttack2,
	slGruntAllyRepel,
	slGruntAllyRepelAttack,
	slGruntAllyRepelLand,
	slGruntAllyEstablishLineOfFire_strap,
	slGruntAllyFollow,
	slGruntAllyFaceTarget,
};

IMPLEMENT_CUSTOM_SCHEDULES( CHGruntAlly, CSquadMonster );

//=========================================================
// SetActivity 
//=========================================================
void CHGruntAlly :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		if (FBitSet( pev->weapons, HGRUNT_9MMAR))
		{
			iSequence = LookupSequence( "standing_mp5" );
		}
		else if (FBitSet( pev->weapons, HGRUNT_AK47))
		{
			iSequence = LookupSequence( "standing_saw" );
		}
		else
		{
			iSequence = LookupSequence( "standing_shotgun" );
		}
		break;
	case ACT_RANGE_ATTACK2:
		// grunt is going to a secondary long range attack. This may be a thrown 
		// grenade or fired grenade, we must determine which and pick proper sequence
		if ( pev->weapons & HGRUNT_HANDGRENADE )
		{
			// get toss anim
			iSequence = LookupSequence( "throwgrenade" );
		}
		else
		{
			// get launch anim
			iSequence = LookupSequence( "launchgrenade" );
		}
		break;
	case ACT_RUN:
		if ( pev->health <= pev->max_health * 0.4 )
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
		if ( pev->health <= pev->max_health * 0.4 )
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
			NewActivity = ACT_IDLE_ANGRY;
		}
		iSequence = LookupActivity ( NewActivity );
		break;
	case ACT_RELOAD:
		iSequence = LookupSequence( "reload_mp5" );
		break;
	default:
		iSequence = LookupActivity ( NewActivity );
		break;
	}
	
	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

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
		if(pev->deadflag == DEAD_NO){
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
		}
	}
}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CHGruntAlly :: GetSchedule( void )
{
	// clear old sentence
	m_iSentence = HGRUNT_SENT_NONE;

	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling. 
	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if (pev->flags & FL_ONGROUND)
		{
			// just landed
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType ( SCHED_GRUNT_REPEL_LAND );
		}
		else
		{
			// repel down a rope, 
			if ( m_MonsterState == MONSTERSTATE_COMBAT )
				return GetScheduleOfType ( SCHED_GRUNT_REPEL_ATTACK );
			else
				return GetScheduleOfType ( SCHED_GRUNT_REPEL );
		}
	}

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
				if(m_running_rangeattack == 1 || m_no_cover_mode == 1){
					return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
				}
				else{
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
				}
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
				if ( m_cAmmoLoaded < m_cClipSize){
				return GetScheduleOfType ( SCHED_RELOAD_DEEP );//������
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
				else if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
				{
					return GetScheduleOfType ( SCHED_RELOAD_DEEP );
				}
				else
				{
					return GetScheduleOfType ( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
				}
			}
// no ammo
			else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}
			else if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				//!!!KELLY - this individual just realized he's out of bullet ammo. 
				// He's going to try to find cover to run to and reload, but rarely, if 
				// none is available, he'll drop and reload in the open here. 
				return GetScheduleOfType ( SCHED_GRUNT_COVER_AND_RELOAD );
			}
// can grenade launch

			else if ( FBitSet( pev->weapons, HGRUNT_GRENADELAUNCHER) && HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
			{
				// shoot a grenade if you can
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}
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
				else
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
				else if ( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
				}
				else
				{
					return GetScheduleOfType( SCHED_STANDOFF );
				}
			}
			
			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
			}
		}
	}
	
	// no special cases here, call the base class
	return CSquadMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CHGruntAlly :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			return &slGruntAllyTakeCover[ 0 ];
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{	
			return &slGruntAllyTakeCoverFromBestSound2[ 0 ];
		}
	case SCHED_GRUNT_TAKECOVER_FAILED:
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}
			return GetScheduleOfType ( SCHED_FAIL );
		}
		break;
	case SCHED_GRUNT_ELOF_FAIL:
		{
			// human grunt is unable to move to a position that allows him to attack the enemy.
			return GetScheduleOfType ( SCHED_CHASE_ENEMY_FAILED );
		}
		break;
	case SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE:
		{
			return &slGruntAllyEstablishLineOfFire[ 0 ];
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			return &slGruntAllyRangeAttack1A[ 0 ];
		}
	case SCHED_RANGE_ATTACK2:
		{
			return &slGruntAllyRangeAttack2[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slGruntAllyCombatFace[ 0 ];
		}
	case SCHED_GRUNT_WAIT_FACE_ENEMY:
		{
			return &slGruntAllyWaitInCover[ 0 ];
		}
	case SCHED_GRUNT_SWEEP:
		{
			return &slGruntAllySweep[ 0 ];
		}
	case SCHED_GRUNT_COVER_AND_RELOAD:
		{
			if(m_hEnemy != NULL){
				float flDist = ( m_hEnemy->pev->origin - pev->origin).Length();

				if ( flDist < 768 && fabs( pev->origin.z - m_hEnemy->pev->origin.z ) < 256 && HasConditions ( bits_COND_ENEMY_FACING_ME ) ){
				return &slGruntAllyHideReload[ 0 ];
				}
				else{
				return &slReload[ 0 ];
				}
			}
			else{
				return &slReload[ 0 ];
			}
		}
	case SCHED_GRUNT_FOUND_ENEMY:
		{
			return &slGruntAllyFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			return &slGruntAllyVictoryDance[ 0 ];
		}
	case SCHED_GRUNT_SUPPRESS:
		{
			return &slGruntAllySuppress[ 0 ];
		}
	case SCHED_TARGET_FACE:
		{
		return slGruntAllyFaceTarget;
		}
	case SCHED_TARGET_CHASE:
		{
		return slGruntAllyFollow;
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slGruntAllyCombatFail[ 0 ];
			}

			return &slGruntAllyFail[ 0 ];
		}
	case SCHED_GRUNT_REPEL:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slGruntAllyRepel[ 0 ];
		}
	case SCHED_GRUNT_REPEL_ATTACK:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slGruntAllyRepelAttack[ 0 ];
		}
	case SCHED_GRUNT_REPEL_LAND:
		{
			return &slGruntAllyRepelLand[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}
