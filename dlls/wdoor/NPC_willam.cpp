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



class CWillam : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed ( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL FCanCheckAttacks ( void );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );

	void RunAI( void );
	void Killed( entvars_t *pevAttacker, int iGib );

	void SetActivity ( Activity NewActivity );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	void DeathSound( void );
	void BloodSonic ( void );

	void GibMonster( void );

	void SetObjectCollisionBox( void )
	{
		pev->absmin = pev->origin + Vector( -32, -32, 0 );
		pev->absmax = pev->origin + Vector( 32, 32, 96 );
	}

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

	float m_flNextGodTime;
	float m_flNextShakeTime;
	float m_flNextLoadTime;

	int		m_iSpriteTexture;

	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;
};

LINK_ENTITY_TO_CLASS( monster_willam, CWillam );

void CWillam :: FollowerUse2( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
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

TYPEDESCRIPTION	CWillam::m_SaveData[] = 
{
	DEFINE_FIELD( CWillam, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CWillam, m_flNextGodTime, FIELD_TIME ),
	DEFINE_FIELD( CWillam, m_flNextShakeTime, FIELD_TIME ),
	DEFINE_FIELD( CWillam, m_flNextLoadTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CWillam, CSquadMonster );

//=========================================================
// IRelationship - overridden because Alien Grunts are 
// Human Grunt's nemesis.
//=========================================================
int CWillam::IRelationship ( CBaseEntity *pTarget )
{
	return CSquadMonster::IRelationship( pTarget );
}


//=========================================================
// RunAI
//=========================================================
void CWillam :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(pev->sequence == LookupActivity ( ACT_WALK )){
	m_flGroundSpeed = 280;
	}
	else if(pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
	m_flGroundSpeed = 240;
	}

	if(m_chaofhate > 0){
		m_chaofhate--;
	}
	else if(pev->team != 0){
		pev->team = 0;
	}

	if(pev->body == 1 || pev->armortype == 1){
		pev->armorvalue--;
		if(pev->armorvalue <= 0){
			if(pev->armortype == 1){
			pev->armortype = 2;
			}
			pev->body = 0;
			pev->renderfx = 0;
			pev->takedamage = DAMAGE_YES;
		}
	}

	if(pev->weapons > 0){//��������
		pev->weapons--;
		if(pev->armorvalue <= 0 && pev->weapons <= 0){
		pev->renderfx = 0;
		}
	}

	//���ܤν���
	if(m_rpgms_level >= 80 && m_rpgms_skill6_learn == 0){
	m_rpgms_skill5_learn = 24;
	m_rpgms_skill6_learn = 43;
	m_rpgms_skill7_learn = 75;
	pev->max_health = 4000;//Ѫ������
	pev->health = pev->max_health;
	}
	if(m_rpgms_level >= 55 && m_rpgms_skill5_learn == 0){
	m_rpgms_skill5_learn = 24;
	}

	//��������ģʽ
	/*
	if(m_playerguardian_mode == 1 && !FBitSet( pev->spawnflags, SF_MONSTER_PRISONER ) ){
	pev->spawnflags |= SF_MONSTER_PRISONER;
	}
	else if(m_playerguardian_mode == 0 && FBitSet( pev->spawnflags, SF_MONSTER_PRISONER ) ){
	pev->spawnflags &= ~SF_MONSTER_PRISONER;
	}
	*/
}


void CWillam::Killed( entvars_t *pevAttacker, int iGib )
{
	if(pev->armortype == 0 && m_rpgms_skill5_learn == 24){
	pev->armortype = 1;
	pev->deadflag = DEAD_NO;
	pev->health = 1;
	pev->armorvalue = 100;
	pev->renderfx = kRenderFxGlowShell;
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 0;
	pev->rendercolor.z = 0;
	pev->renderamt = 2;
	pev->takedamage = DAMAGE_NO;//��Ѫģʽ
	m_flNextGodTime = gpGlobals->time + 60.0;
	m_flNPC_Pain = 0;
	return;
	}

	if(pev->weapons > 0){
	pev->weapons = 0;
	pev->renderfx = 0;
	pev->deadflag = DEAD_NO;
	pev->health = 1;
	return;
	}

	CSquadMonster::Killed( pevAttacker, GIB_NEVER );
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CWillam :: GibMonster ( void )
{
	CBaseMonster :: GibMonster();
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
BOOL CWillam :: FCanCheckAttacks ( void )
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
BOOL CWillam :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if (m_playerguardian_mode == 1 || m_flNextShakeTime > gpGlobals->time){
	return FALSE;
	}

	float dist = 768;
	
	if (flDist <= dist)
	{
	return TRUE;
	}

	return FALSE;
}

BOOL CWillam :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if (m_flNextLoadTime > gpGlobals->time || m_rpgms_skill7_learn != 75
	|| !pev->takedamage || m_playerguardian_mode == 1){
	return FALSE;
	}

	return TRUE;
}

BOOL CWillam :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	float cover_dist = 128;

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
void CWillam :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if(!pev->takedamage){
		UTIL_Sparks(ptr->vecEndPos);
		return;
	}

	if(ptr->iHitgroup == 0){
		if ( pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0,100) < 20) )
		{
				flDamage -= 60;

				pev->dmgtime = gpGlobals->time;
				
				if (RANDOM_LONG(0, 1))
					EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/ric_metal-1.wav", 1, ATTN_NORM);
				else
					EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/ric_metal-2.wav", 1, ATTN_NORM);

				UTIL_Sparks(ptr->vecEndPos);

				if(flDamage < 1){
				return;
				}
		}
	}

	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
int CWillam :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if(m_flNextGodTime <= gpGlobals->time && pev->armorvalue == 0 && pev->deadflag == DEAD_NO
	&& pev->weapons == 0){
		if (pevAttacker)
		{
			if(m_flNPC_Pain > 0){
			pev->armorvalue = 100;
			pev->body = 1;
			pev->renderfx = kRenderFxGlowShell;
			pev->rendercolor.x = 255;
			pev->rendercolor.y = 255;
			pev->rendercolor.z = 0;
			pev->renderamt = 2;
			pev->takedamage = DAMAGE_NO;//�޵�ģʽ
			pev->armortype = 0;
			m_flNextGodTime = gpGlobals->time + 60.0;
			EMIT_SOUND( ENT(pev), 6, "willam/holy_shield.wav", 1, 0.4 );
			m_flNPC_Pain = 0;
			return 0;
			}
		}
	}

	if(pev->sequence == LookupActivity ( ACT_RANGE_ATTACK1 )
	|| pev->sequence == LookupActivity ( ACT_RANGE_ATTACK2 )){
	flDamage *= 0.1;//�����ͷ��д����˺����٣�
	}

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

	m_alert	= 100;

	Forget( bits_MEMORY_INCOVER );

	return CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CWillam :: SetYawSpeed ( void )
{
	pev->yaw_speed = 120;
}


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CWillam :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

void CWillam :: BloodSonic ( void )
{
	// blast circles
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 32);
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 32 + 384 / .2); // reach damage radius over .3 seconds
		WRITE_SHORT( m_iSpriteTexture );
		WRITE_BYTE( 0 ); // startframe
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 3 ); // life
		WRITE_BYTE( 15 );  // width
		WRITE_BYTE( 0 );   // noise

		WRITE_BYTE( 255   );
		WRITE_BYTE( 192 );
		WRITE_BYTE( 32  );

		WRITE_BYTE( 255 ); //brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 32);
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 32 + ( 384 / 2 ) / .2); // reach damage radius over .3 seconds
		WRITE_SHORT( m_iSpriteTexture );
		WRITE_BYTE( 0 ); // startframe
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 3 ); // life
		WRITE_BYTE( 15 );  // width
		WRITE_BYTE( 0 );   // noise
		
		WRITE_BYTE( 255   );
		WRITE_BYTE( 192 );
		WRITE_BYTE( 32  );

		WRITE_BYTE( 255 ); //brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CWillam :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;
	int atk_dist = 125;

	switch( pEvent->event )
	{
		case 1://�չ�
		{
			if(m_hEnemy != NULL){
				atk_dist += m_hEnemy->pev->maxs.x;

				if(( pev->origin - m_hEnemy->pev->origin).Length() <= atk_dist){
				m_hEnemy->TakeDamage( pev, pev, 60, DMG_SLASH );
				FX_Explosion(m_hEnemy->Center(), 46 );
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/knife_hitbody.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}
		}
		break;

		case 2://�����
		{
			BloodSonic();
		}
		break;

		case 3://����
		{
			if(m_flNextShakeTime <= gpGlobals->time){
			m_chaofhate = 80;
			pev->team = 666;
			BloodSonic();
			m_flNextShakeTime = gpGlobals->time + 20.0;
			}
		}
		break;

		case 4://��������׼��ʹ��
		{
			if(m_flNextLoadTime <= gpGlobals->time){
			MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE(  TE_IMPLOSION);
			WRITE_COORD( pev->origin.x);
			WRITE_COORD( pev->origin.y);
			WRITE_COORD( pev->origin.z + 48);
			WRITE_BYTE(400);  // radius
			WRITE_BYTE(60); // count
			WRITE_BYTE(20); // life
			MESSAGE_END();

			MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE(  TE_IMPLOSION);
			WRITE_COORD( pev->origin.x);
			WRITE_COORD( pev->origin.y);
			WRITE_COORD( pev->origin.z + 48);
			WRITE_BYTE(-400);  // radius
			WRITE_BYTE(60); // count
			WRITE_BYTE(20); // life
			MESSAGE_END();

			EMIT_SOUND( ENT(pev), 6, "willam/danmu_start.wav", 1, 0.4 );
			}
		}
		break;

		case 5:
		{
			if(m_flNextLoadTime <= gpGlobals->time){
			FX_Explosion( Center(), 52);
			m_flNextLoadTime = gpGlobals->time + 80.0;
			pev->weapons = 200;
			TakeHealth(pev->max_health * 0.1, DMG_GENERIC);

			pev->renderfx = kRenderFxGlowShell;
			pev->rendercolor.x = 0;
			pev->rendercolor.y = 255;
			pev->rendercolor.z = 128;
			pev->renderamt = 2;

			EMIT_SOUND( ENT(pev), 6, "willam/superarmor.wav", 1, 0.4 );

				//����������ػ�
				CBaseMonster *pAlly;
				CBaseMonster *pPlayer;
				if(m_hPlayer != NULL){
					pPlayer = GetClassPtr((CBasePlayer *)m_hPlayer->pev);
					if(pPlayer->m_hPortecter != this){
					pPlayer->m_hPortecter = this;
					}
				}
				if(m_hTeamMate1 != NULL){
					pAlly = m_hTeamMate1->MyMonsterPointer();
					if(pAlly->m_hPortecter != this){
					pAlly->m_hPortecter = this;
					}
				}
				if(m_hTeamMate2 != NULL){
				pAlly = m_hTeamMate2->MyMonsterPointer();
					if(pAlly->m_hPortecter != this){
					pAlly->m_hPortecter = this;
					}
				}
				if(m_hTeamMate3 != NULL){
				pAlly = m_hTeamMate3->MyMonsterPointer();
					if(pAlly->m_hPortecter != this){
					pAlly->m_hPortecter = this;
					}
				}
				if(m_hTeamMate4 != NULL){
				pAlly = m_hTeamMate4->MyMonsterPointer();
					if(pAlly->m_hPortecter != this){
					pAlly->m_hPortecter = this;
					}
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
void CWillam :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/willam.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;

	pev->health			= 3000;//Ѫ�غ��̹��
	m_lovehate			= 90;

	m_flFieldOfView		 = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		 = MONSTERSTATE_NONE;

	m_flNextShakeTime	= gpGlobals->time + 2;
	m_flNextGodTime		= gpGlobals->time + 2;
	m_flNextLoadTime    = gpGlobals->time + 2;

	m_afCapability		= bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;

	m_HackedGunPos = Vector ( 0, 0, 55 );

	m_canheadcrab_mode  = 0;
	m_canbarnacle_mode  = 0;

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

	SetUse( &CWillam::FollowerUse2 );

	m_aimenemy_mod = 6;

	pev->body = 0;

	SetTouch( &CWillam::DeadTouch );

	pev->gravity = 2.0;//������
	pev->friction = 2.0;
	m_longming = 1;

	m_rpgms_actor = 14;
	m_rpgms_level = 45;
	m_rpgms_exp = 0;
	m_rpgms_type = 1;

	m_new_ally_type = TRUE;
	pev->netname = MAKE_STRING( "Willam" );

	m_rpgms_skill1_learn = 19;
	m_rpgms_skill2_learn = 22;
	m_rpgms_skill3_learn = 23;
	m_rpgms_skill4_learn = 62;//����
	m_rpgms_skill5_learn = 0;//Ӳ��24
	m_rpgms_skill6_learn = 0;//�����ɳ�43
	m_rpgms_skill7_learn = 0;//��������75

	pev->takedamage = DAMAGE_YES;
	m_freeze_def = 1;//����ο���LV1
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CWillam :: Precache()
{
	PRECACHE_MODEL("models/willam.mdl");

	PRECACHE_SOUND("weapons/knife_hitbody.wav");

	PRECACHE_SOUND("willam/die1.wav");
	PRECACHE_SOUND("willam/holy_shield.wav");
	PRECACHE_SOUND("willam/danmu_start.wav");
	PRECACHE_SOUND("willam/superarmor.wav");

	m_iSpriteTexture = PRECACHE_MODEL( "sprites/shockwave.spr" );
}	

//=========================================================
// start task
//=========================================================
void CWillam :: StartTask ( Task_t *pTask )
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
void CWillam :: RunTask ( Task_t *pTask )
{
	CSquadMonster :: RunTask( pTask );
}

//=========================================================
// DeathSound 
//=========================================================
void CWillam :: DeathSound ( void )
{
	EMIT_SOUND( ENT(pev), 6, "willam/die1.wav", 1, 0.6 );	
}

//=========================================================
// GruntFail
//=========================================================
Task_t	tlWillamFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)1		},
	{ TASK_FORGET_ENEMY,		(float)0	},
};

Schedule_t	slWillamFail[] =
{
	{
		tlWillamFail,
		ARRAYSIZE ( tlWillamFail ),
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
Task_t	tlWillamCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)1		},
};

Schedule_t	slWillamCombatFail[] =
{
	{
		tlWillamCombatFail,
		ARRAYSIZE ( tlWillamCombatFail ),
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
Task_t	tlWillamVictoryDance[] =
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

Schedule_t	slWillamVictoryDance[] =
{
	{ 
		tlWillamVictoryDance,
		ARRAYSIZE ( tlWillamVictoryDance ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_SEE_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE  |
		0,
		0,
		"GruntVictoryDance"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the grunt to attack.
//=========================================================
Task_t tlWillamEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_GRUNT_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slWillamEstablishLineOfFire[] =
{
	{ 
		tlWillamEstablishLineOfFire,
		ARRAYSIZE ( tlWillamEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		0,
		0,
		"GruntEstablishLineOfFire"
	},
};

//=========================================================
// GruntFoundEnemy - grunt established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t	tlWillamFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slWillamFoundEnemy[] =
{
	{ 
		tlWillamFoundEnemy,
		ARRAYSIZE ( tlWillamFoundEnemy ), 
		0,
		0,
		"GruntFoundEnemy"
	},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t	tlWillamCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)0.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_GRUNT_SWEEP	},
};

Schedule_t	slWillamCombatFace[] =
{
	{ 
		tlWillamCombatFace1,
		ARRAYSIZE ( tlWillamCombatFace1 ), 
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
Task_t	tlWillamSignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
};

Schedule_t	slWillamSignalSuppress[] =
{
	{ 
		tlWillamSignalSuppress,
		ARRAYSIZE ( tlWillamSignalSuppress ), 
		0,
		0,
		"SignalSuppress"
	},
};

Task_t	tlWillamSuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slWillamSuppress[] =
{
	{ 
		tlWillamSuppress,
		ARRAYSIZE ( tlWillamSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,
		0,
		"Suppress"
	},
};


//=========================================================
// grunt wait in cover - we don't allow danger or the ability
// to attack to break a grunt's run to cover schedule, but
// when a grunt is in cover, we do want them to attack if they can.
//=========================================================
Task_t	tlWillamWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slWillamWaitInCover[] =
{
	{ 
		tlWillamWaitInCover,
		ARRAYSIZE ( tlWillamWaitInCover ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2,

		0,
		"GruntWaitInCover"
	},
};


//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlWillamSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_FORGET_ENEMY,		(float)0	},
};

Schedule_t	slWillamSweep[] =
{
	{ 
		tlWillamSweep,
		ARRAYSIZE ( tlWillamSweep ), 
		
		bits_COND_SEE_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		0,
		0,
		"Grunt Sweep"
	},
};

//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlWillamRangeAttack1A[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
};

Schedule_t	slWillamRangeAttack1A[] =
{
	{ 
		tlWillamRangeAttack1A,
		ARRAYSIZE ( tlWillamRangeAttack1A ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND		|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,
		
		0,
		"Range Attack1A"
	},
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlWillamRangeAttack1B[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
};

Schedule_t	slWillamRangeAttack1B[] =
{
	{ 
		tlWillamRangeAttack1B,
		ARRAYSIZE ( tlWillamRangeAttack1B ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_HEAR_SOUND,
		
		0,
		"Range Attack1B"
	},
};


DEFINE_CUSTOM_SCHEDULES( CWillam )
{
	slWillamFail,
	slWillamCombatFail,
	slWillamVictoryDance,
	slWillamEstablishLineOfFire,
	slWillamFoundEnemy,
	slWillamCombatFace,
	slWillamSignalSuppress,
	slWillamSuppress,
	slWillamSweep,
	slWillamRangeAttack1A,
	slWillamRangeAttack1B,
};

IMPLEMENT_CUSTOM_SCHEDULES( CWillam, CSquadMonster );

//=========================================================
// SetActivity 
//=========================================================
void CWillam :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RUN:
		iSequence = LookupActivity ( NewActivity );
		break;
	case ACT_WALK:
		if ( m_enemyfollower_combat == 1  )
		{
			// limp!
			iSequence = LookupActivity ( ACT_WALK_SCARED );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
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
Schedule_t *CWillam :: GetSchedule( void )
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
					if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) )
					{
						return GetScheduleOfType ( SCHED_RANGE_ATTACK2 );
					}
					else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
					{
						return GetScheduleOfType ( SCHED_GRUNT_SUPPRESS );
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
				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) )
				{
					return GetScheduleOfType ( SCHED_RANGE_ATTACK2 );
				}
				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
				{
					return GetScheduleOfType ( SCHED_GRUNT_SUPPRESS );
				}
				else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
				{
					return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
				}
				else if ( HasConditions( bits_COND_SEE_ENEMY ) || HasConditions( bits_COND_ENEMY_OCCLUDED ) )
				{
					return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
				}
			}
			break;
	}
	
	// no special cases here, call the base class
	return CSquadMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CWillam :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_GRUNT_ELOF_FAIL:
		{
			if(m_groundElev){
			return &slWillamCombatFace[ 0 ];
			}
			else{
			return GetScheduleOfType ( SCHED_CHASE_ENEMY_FAILED );
			}
		}
		break;
	case SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE:
		{
			if(m_groundElev){
			return &slWillamCombatFace[ 0 ];
			}
			else{
			return &slWillamEstablishLineOfFire[ 0 ];
			}
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			return &slWillamRangeAttack1A[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slWillamCombatFace[ 0 ];
		}
	case SCHED_GRUNT_WAIT_FACE_ENEMY:
		{
			return &slWillamWaitInCover[ 0 ];
		}
	case SCHED_GRUNT_SWEEP:
		{
			return &slWillamSweep[ 0 ];
		}
	case SCHED_GRUNT_FOUND_ENEMY:
		{
			return &slWillamFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			return &slWillamVictoryDance[ 0 ];
		}
	case SCHED_GRUNT_SUPPRESS:
		{
			return &slWillamSuppress[ 0 ];
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slWillamCombatFail[ 0 ];
			}

			return &slWillamFail[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}